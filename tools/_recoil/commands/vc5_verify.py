from __future__ import annotations

import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse
import contextlib
import ctypes
from dataclasses import asdict, dataclass, field, is_dataclass, replace
from functools import lru_cache
import io
import json
import os
import re
import shlex
import shutil
import subprocess
from pathlib import Path
import sys
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import (
    CoffObject,
    IMAGE_SCN_CNT_CODE,
    ObjectByteComparison,
    bytes_from_hexdump,
    compare_bn_data_to_obj,
    compare_bn_to_obj,
    format_byte_dump,
    instructions_to_bytes,
    parse_assembly,
)
from _recoil.lib.binja import (
    DEFAULT_BN_CALL_BUDGET,
    DEFAULT_BRIDGE_URL,
    BinaryNinjaBridge,
    BridgeBudgetExceeded,
    BridgeError,
)
from _recoil.lib.owner_entries import OwnerEntryIndex, OwnerEntry, normalize_address
from _recoil.lib.pe import parse_pe_headers
from _recoil.lib.profiles import DEFAULT_PROFILES, profiles_by_name
from _recoil.lib.reference_images import reference_image, reference_image_keys, resolve_owner_ledger_path
from _recoil.lib.source_owners import (
    DEFAULT_OWNER_LEDGER,
    SourceOwner,
    SourceOwnerDocument,
    owner_data_address_records,
    primary_owners_for_entry,
)
from _recoil.lib.source_emission_markers import (
    ANCHOR_KINDS,
    EmissionAnchor,
    collect_source_closure,
    normalize_anchor_path,
    validate_source_emission_marker,
)
from _recoil.lib import source_emission_markers as source_emission_markers_lib
from _recoil.lib import source_traceability as source_traceability_lib
from _recoil.lib.authored_icf import (
    select_authored_icf_translation_unit_object_symbol,
    validate_authored_icf_physical_source_artifacts,
)
from _recoil.lib.progress import ProgressError, ProgressStore
from _recoil.lib.repository_paths import (
    GitTrackedPathInventory,
    RepositoryPathError,
    TrackedRepositoryPath,
    diagnose_historical_repository_path,
    load_git_tracked_path_inventory,
    resolve_tracked_repository_file,
    validate_repository_relative_path,
)
from _recoil.lib.source_traceability import (
    SourceTraceArtifact,
    SourceTraceDocument,
    artifact_address,
    merge_source_trace_documents,
    normalize_artifact_id,
    parse_source_trace_path,
)
from _recoil.lib.source_fragments import (
    production_closure_fragment_findings,
    require_clean_production_closure,
)
from _recoil.lib.tooling import (
    CommandScriptResult,
    REPO_ROOT,
    display_path,
    optional_bool,
    quote_cmd_arg,
    response_line,
    repo_path,
    require_string,
    require_string_list,
    run_cmd_script as run_tool_cmd_script,
)
from _recoil.lib.vc5_compile_topology import reject_raw_topology_flags
from _recoil.lib.windows_identity import StableReadHandle, physical_identity
from _recoil.lib.worktree_control import routed_machine_local_path


DEFAULT_MANIFEST_DIR = REPO_ROOT / "tools" / "vc5_verify_targets"
DEFAULT_PROGRESS_PATH = routed_machine_local_path(
    executing_worktree_root=REPO_ROOT,
    relative_path=".agent/RECONSTRUCTION_PROGRESS.sqlite3",
)
DEFAULT_BUILD_ROOT = REPO_ROOT / "build" / "vc5-verify"
MACHINE_RETAIL_REFERENCE = routed_machine_local_path(
    executing_worktree_root=REPO_ROOT,
    relative_path="support/Recoil.exe",
)
DEFAULT_VC5_ROOT = MACHINE_RETAIL_REFERENCE.parents[1].parent / "Compiler" / "VC5SP3"
DEFAULT_VC5_ENV = Path(os.environ.get("RECOIL_VC5_ENV", str(DEFAULT_VC5_ROOT / "vc5sp3-env.cmd")))
PROJECT_GENERATED_FILE_PREFIXES = ("src/", "GameZRecoil/", "Battlesport/", "recoil/")
QUOTED_INCLUDE_RE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.MULTILINE)
COD_LISTING_LABEL_RE = re.compile(r"^\s*([A-Za-z_?$@][^\s:]*):\s*(?:;.*)?$")
COD_LISTING_PROC_RE = re.compile(r"^\s*([^\s]+)\s+PROC\b")
COD_LISTING_ENDP_RE = re.compile(r"^\s*([^\s]+)\s+ENDP\b")
COD_LISTING_OFFSET_RE = re.compile(r"^\s*([0-9A-Fa-f]{5})\b")
BN_SYMBOL_LOOKUP_CALL_ESTIMATE = 2
DEFAULT_TARGET_BINARY = "recoil"
FULL_ORDER_INCLUDE_DIRECTIVE_RE = re.compile(
    r'^\s*#\s*(?:line\s+)?\d+\s+"([^"]+)"(?:\s|$)',
    re.IGNORECASE,
)
IMAGE_SCN_MEM_EXECUTE = 0x20000000
IMAGE_SCN_LNK_COMDAT = 0x00001000
IMAGE_SYM_DTYPE_FUNCTION = 0x20
IMAGE_SYM_CLASS_WEAK_EXTERNAL = 105
FUNCTION_PROVENANCE_PROVIDER_BOUNDARY = "provider-boundary"
FUNCTION_PROVENANCE_COMPILER_EMITTED_NONCOVERING = "compiler-emitted-noncovering"
FUNCTION_PROVENANCE_VALUES = {
    FUNCTION_PROVENANCE_PROVIDER_BOUNDARY,
    FUNCTION_PROVENANCE_COMPILER_EMITTED_NONCOVERING,
}
PROVIDER_BOUNDARY_POLICY_MARKER_RE = re.compile(
    r"\bProvider(?:-|\s+)boundary\s+(0x[0-9A-Fa-f]+)\s*:",
    re.IGNORECASE,
)
COMPILER_EMITTED_POLICY_MARKER_RE = re.compile(
    r"\bCompiler(?:-|\s+)emitted\s+(0x[0-9A-Fa-f]+)\s*:",
    re.IGNORECASE,
)
PIPELINE_CLASS_VALUES = {"authored", "authored-lifecycle", "non-authored", "unresolved"}
AUTHORED_PIPELINE_CLASSES = {"authored", "authored-lifecycle"}
ORDER_SCOPE_VALUES = {"authored", "full"}
AUTHORED_ORDER_ROLE_VALUES = {
    "authored-body",
    "authored-lifecycle-body",
    "compiler-generated-deleting-variant",
    "compiler-generated-eh-helper",
    "compiler-generated-thunk",
    "compiler-generated-implicit-cleanup",
    "compiler-generated-icf-representative",
    "non-authored",
    "unresolved",
}
AUTHORED_ORDER_GATING_ROLES = {"authored-body", "authored-lifecycle-body"}
ICF_ALIAS_MANIFEST_FOLD_STATUSES = {
    "selected-winner",
    "proven-fold-alias",
    "not-established",
}


@lru_cache(maxsize=1)
def canonical_tracker_artifact_index() -> source_traceability_lib.SourceArtifactIndex:
    """Load the production tracker artifact index once per verifier process."""

    return source_traceability_lib.load_artifact_rows(DEFAULT_PROGRESS_PATH)


@lru_cache(maxsize=1)
def canonical_tracker_data() -> Mapping[str, Any]:
    """Load current tracker state once for production-manifest source policy."""

    data = ProgressStore(DEFAULT_PROGRESS_PATH).load().data
    if not isinstance(data, Mapping):
        raise ValueError(f"{DEFAULT_PROGRESS_PATH}: progress root must be an object")
    return data


def canonical_tracker_data_for_manifest(
    manifest_path: Path,
) -> Mapping[str, Any] | None:
    try:
        manifest_path.resolve().relative_to(DEFAULT_MANIFEST_DIR.resolve())
    except ValueError:
        return None
    return canonical_tracker_data()


@lru_cache(maxsize=1)
def registered_vc5_manifest_paths() -> frozenset[str]:
    """Return current tracker-registered VC5 manifest logical paths."""

    targets = canonical_tracker_data().get("verification_targets", {})
    if not isinstance(targets, Mapping):
        raise ValueError("progress verification_targets must be an object")
    manifest_paths: set[str] = set()
    for target in targets.values():
        if not isinstance(target, Mapping) or target.get("kind") != "vc5":
            continue
        registration = target.get("registration")
        manifest_path = (
            registration.get("manifest_path")
            if isinstance(registration, Mapping)
            else None
        )
        if isinstance(manifest_path, str) and manifest_path:
            manifest_paths.add(manifest_path)
    return frozenset(manifest_paths)


@lru_cache(maxsize=1)
def canonical_tracker_function_metadata() -> dict[str, tuple[str, str, str | None]]:
    """Return current tracker class/role for repository-owned VC5 manifests.

    Manifest-local ``pipeline_class`` remains useful for portable diagnostics,
    but the unified tracker is authoritative in the production workspace.  In
    particular, old manifests commonly omit ``function_defaults`` and therefore
    parse as ``unresolved`` even after the physical artifact was reviewed.
    """

    index = canonical_tracker_artifact_index()
    metadata: dict[str, tuple[str, str, str | None]] = {}
    for artifact_id, artifact in index.rows.items():
        if artifact.kind != "function":
            continue
        pipeline_class = artifact.row.get("pipeline_class")
        authored_order_role = artifact.row.get("authored_order_role")
        if pipeline_class in PIPELINE_CLASS_VALUES:
            if authored_order_role not in AUTHORED_ORDER_ROLE_VALUES:
                authored_order_role = {
                    "authored": "authored-body",
                    "authored-lifecycle": "authored-lifecycle-body",
                    "non-authored": "non-authored",
                    "unresolved": "unresolved",
                }[pipeline_class]
            trace = artifact.row.get("source_traceability")
            trace_state = (
                trace.get("state")
                if isinstance(trace, Mapping)
                and isinstance(trace.get("state"), str)
                else None
            )
            metadata[artifact_id] = (
                pipeline_class,
                authored_order_role,
                trace_state,
            )
    return metadata


def effective_source_trace_function(
    *,
    manifest_path: Path,
    function: VerifyFunction,
    target_binary: str,
) -> VerifyFunction:
    """Use tracker authority for production manifests, local metadata elsewhere."""

    try:
        manifest_path.resolve().relative_to(DEFAULT_MANIFEST_DIR.resolve())
    except ValueError:
        return function
    artifact_id = canonical_function_artifact_id(
        function,
        target_binary=target_binary,
    )
    metadata = canonical_tracker_function_metadata().get(artifact_id)
    if metadata is None:
        return function
    return replace(
        function,
        pipeline_class=metadata[0],
        authored_order_role=metadata[1],
    )


def canonical_tracker_metadata_for_manifest(
    *,
    manifest_path: Path,
    artifact_id: str,
) -> tuple[str, str, str | None] | None:
    """Return tracker metadata only for repository-owned production manifests."""

    try:
        manifest_path.resolve().relative_to(DEFAULT_MANIFEST_DIR.resolve())
    except ValueError:
        return None
    return canonical_tracker_function_metadata().get(artifact_id)


def tracker_artifact_index_for_manifest(
    manifest_path: Path,
) -> source_traceability_lib.SourceArtifactIndex:
    """Use process-cached tracker state only for production manifests."""

    try:
        manifest_path.resolve().relative_to(DEFAULT_MANIFEST_DIR.resolve())
    except ValueError:
        return source_traceability_lib.load_artifact_rows(DEFAULT_PROGRESS_PATH)
    return canonical_tracker_artifact_index()


def _has_addressed_comment_policy_marker(
    text: str,
    address: str,
    pattern: re.Pattern[str],
) -> bool:
    expected = normalize_address(address)
    for comment in source_traceability_lib._scan_comments(text):
        for match in pattern.finditer(comment.text):
            if normalize_address(match.group(1)) == expected:
                return True
    return False


def has_provider_boundary_policy_marker(text: str, address: str) -> bool:
    """Return whether a source comment records the exact provider boundary."""

    return _has_addressed_comment_policy_marker(
        text,
        address,
        PROVIDER_BOUNDARY_POLICY_MARKER_RE,
    )


def has_compiler_emitted_policy_marker(text: str, address: str) -> bool:
    """Return whether a source comment records the exact compiler contribution."""

    return _has_addressed_comment_policy_marker(
        text,
        address,
        COMPILER_EMITTED_POLICY_MARKER_RE,
    )
COMPILER_GENERATED_AUTHORED_ORDER_ROLES = {
    "compiler-generated-deleting-variant",
    "compiler-generated-eh-helper",
    "compiler-generated-thunk",
    "compiler-generated-implicit-cleanup",
    "compiler-generated-icf-representative",
}
COMPILER_GENERATED_ROLE_ANCHOR_KINDS = {
    "compiler-generated-deleting-variant": {"type-definition"},
    "compiler-generated-eh-helper": {
        "function-definition",
        "data-definition",
        "type-definition",
    },
    "compiler-generated-thunk": {
        "function-definition",
        "data-definition",
        "type-definition",
    },
    "compiler-generated-implicit-cleanup": {
        "function-definition",
        "data-definition",
        "type-definition",
    },
    "compiler-generated-icf-representative": {"function-definition"},
}
@dataclass(frozen=True)
class VerifyFunction:
    address: str
    symbol: str
    name: str
    bn_byte_length: int | None = None
    vc5_byte_length: int | None = None
    symbol_regex: str | None = None
    listing_label_regex: str | None = None
    provenance: str = ""
    source_order_gate: bool = True
    pipeline_class: str = "unresolved"
    authored_order_role: str = ""
    required_presence: bool = True
    full_order_gate: bool = True
    logical_identity_key: str = ""
    icf_fold_status: str = ""
    emission_anchor: EmissionAnchor | None = None


@dataclass(frozen=True)
class SourceEmissionWarning:
    address: str
    code: str
    message: str
    source_from: str


@dataclass(frozen=True)
class CandidateOnlyExtra:
    name: str
    pipeline_class: str
    symbol: str = ""
    symbol_regex: str | None = None


@dataclass(frozen=True)
class VerifyDataSymbol:
    address: str
    symbol: str
    name: str
    byte_length: int
    object_offset: int = 0
    bn_name: str = ""
    symbol_regex: str | None = None
    logical_identity_key: str = ""


@dataclass(frozen=True)
class TranslationUnitFunctionOrderEntry:
    source_from: str
    functions: tuple[VerifyFunction, ...]
    order_scope: str = "full"
    candidate_only_extras: tuple[CandidateOnlyExtra, ...] = ()
    inventory_only: bool = False


@dataclass(frozen=True)
class LinkedSectionBoundary:
    section: str
    address: str


@dataclass(frozen=True)
class LinkedFunctionInterval:
    name: str
    predecessor: VerifyFunction | None
    functions: tuple[VerifyFunction, ...]
    successor: VerifyFunction
    predecessor_section_boundary: LinkedSectionBoundary | None = None
    order_scope: str = "full"
    candidate_only_extras: tuple[CandidateOnlyExtra, ...] = ()
    retail_start: str = ""
    retail_end_exclusive: str = ""


@dataclass(frozen=True)
class LinkedOrderControlledIdentity:
    name: str
    symbol: str
    expected_object: str


@dataclass(frozen=True)
class LinkedOrderDiagnosticMode:
    kind: str = ""
    required_link_profile: str = ""
    nonblocking_reason: str = ""
    nonblocking_predicates: tuple[str, ...] = ()
    controlled_identities: tuple[LinkedOrderControlledIdentity, ...] = ()
    forbidden_objects: tuple[str, ...] = ()


@dataclass(frozen=True)
class ProfileGuardEntry:
    profile: str
    sentinel_addresses: tuple[str, ...] = ()
    evidence: str = ""
    reason: str = ""


@dataclass(frozen=True)
class ProfileGuard:
    scope: str = ""
    policy: str = ""
    accepted_profiles: tuple[ProfileGuardEntry, ...] = ()
    disqualified_profiles: tuple[ProfileGuardEntry, ...] = ()


@dataclass(frozen=True)
class VerifyTarget:
    name: str
    description: str
    source_filename: str
    source_text: str
    source_from: str
    compare_mode: str
    trim_trailing_nops: bool
    compiler_profile: str
    compiler_env: str
    compiler_flags: tuple[str, ...]
    include_dirs: tuple[str, ...]
    source_files: tuple[str, ...]
    generated_files: tuple[tuple[str, str], ...]
    functions: tuple[VerifyFunction, ...]
    data_symbols: tuple[VerifyDataSymbol, ...]
    manifest_path: Path
    check_function_order: bool = False
    function_order_scope: str = "full"
    target_binary: str = DEFAULT_TARGET_BINARY
    check_translation_unit_function_order: bool = False
    translation_unit_function_order: tuple[TranslationUnitFunctionOrderEntry, ...] = ()
    linked_function_intervals: tuple[LinkedFunctionInterval, ...] = ()
    profile_guard: ProfileGuard = field(default_factory=ProfileGuard)
    compile_context_from: str = ""
    source_emission_warnings: tuple[SourceEmissionWarning, ...] = ()
    source_emission_policy_strict: bool = False
    source_traceability_policy_strict: bool = False
    compile_defines: tuple[str, ...] = ()
    retail_start: str = ""
    retail_end_exclusive: str = ""
    source_compile_profiles: tuple[tuple[str, str], ...] = ()
    source_compile_flags: tuple[tuple[str, tuple[str, ...]], ...] = ()
    linked_order_base_target: str = ""
    linked_order_diagnostic_mode: LinkedOrderDiagnosticMode = field(
        default_factory=LinkedOrderDiagnosticMode
    )
    order_edit_paths: tuple[str, ...] = ()


def target_declared_source_paths(target: VerifyTarget) -> tuple[str, ...]:
    paths = [target.source_from, *target.source_files, *target.order_edit_paths]
    paths.extend(entry.source_from for entry in target.translation_unit_function_order)
    return tuple(dict.fromkeys(path for path in paths if path))


def target_source_fragment_findings(target: VerifyTarget) -> tuple[dict[str, object], ...]:
    return production_closure_fragment_findings(
        target_declared_source_paths(target),
        repo_root=REPO_ROOT,
    )


def require_clean_target_source_fragments(target: VerifyTarget) -> None:
    require_clean_production_closure(
        target_declared_source_paths(target),
        repo_root=REPO_ROOT,
        context=f"{target.manifest_path}: VC5 target {target.name!r}",
    )


def default_authored_order_role(pipeline_class: str) -> str:
    return {
        "authored": "authored-body",
        "authored-lifecycle": "authored-lifecycle-body",
        "non-authored": "non-authored",
        "unresolved": "unresolved",
    }.get(pipeline_class, "unresolved")


def authored_order_role(function: VerifyFunction) -> str:
    return function.authored_order_role or default_authored_order_role(function.pipeline_class)


def function_authored_order_gate(function: VerifyFunction) -> bool:
    return authored_order_role(function) in AUTHORED_ORDER_GATING_ROLES


def authored_relative_order_gate(
    *,
    authored_order_gate: bool,
    logical_identity_key: str,
    icf_fold_status: str,
    cached: bool | None = None,
) -> bool:
    """Return the shared authored-relative-order decision.

    Registered tracker targets cache this decision.  Older registrations do
    not, so callers can omit ``cached`` and derive it from the durable role and
    ICF fields instead.  A cache can only narrow an authored gate; it can never
    turn a non-authored contribution into one.
    """

    if not authored_order_gate:
        return False
    if isinstance(cached, bool):
        return cached
    return not logical_identity_key or icf_fold_status in {
        "selected-winner",
        "not-established",
    }


def function_authored_relative_order_gate(function: VerifyFunction) -> bool:
    return authored_relative_order_gate(
        authored_order_gate=function_authored_order_gate(function),
        logical_identity_key=function.logical_identity_key,
        icf_fold_status=function.icf_fold_status,
    )


def function_required_in_scope(function: VerifyFunction, order_scope: str) -> bool:
    return function.required_presence and (
        order_scope == "full" or function_authored_order_gate(function)
    )


@dataclass(frozen=True)
class VerifySelection:
    target: VerifyTarget
    functions: tuple[VerifyFunction, ...]
    data_symbols: tuple[VerifyDataSymbol, ...] = ()


@dataclass(frozen=True)
class CompiledTarget:
    target: VerifyTarget
    build_dir: Path
    source_path: Path
    cod_path: Path
    obj_path: Path
    compiler_env: Path
    compiler_version: str
    compile_command: str
    effective_compiler_profile: str = ""
    effective_compiler_flags: tuple[str, ...] = ()
    compiler_receipt: Mapping[str, Any] = field(default_factory=dict)


@dataclass(frozen=True)
class CompiledTranslationUnit:
    manifest_index: int
    source_from: str
    source_path: Path
    cod_path: Path
    obj_path: Path
    effective_compiler_profile: str = ""
    effective_compiler_flags: tuple[str, ...] = ()
    compile_command: str = ""
    compiler_receipt: Mapping[str, Any] = field(default_factory=dict)


@dataclass(frozen=True)
class CodListingLabel:
    label: str
    proc_symbol: str
    section_number: int | None
    value: int | None


@dataclass(frozen=True)
class CodListingLabelIndex:
    labels: tuple[CodListingLabel, ...]


@dataclass(frozen=True)
class FunctionOrderRow:
    manifest_index: int
    function: VerifyFunction
    symbol: str
    section_number: int
    value: int
    object_index: int = 0
    source_from: str = ""
    order_source: str = "coff-symbol"


@dataclass(frozen=True)
class FunctionOrderBreak:
    previous: FunctionOrderRow
    current: FunctionOrderRow


@dataclass(frozen=True)
class FunctionOrderCheck:
    target: VerifyTarget
    rows: tuple[FunctionOrderRow, ...]
    breaks: tuple[FunctionOrderBreak, ...]
    expected_functions: tuple[VerifyFunction, ...] = ()
    diagnostics: tuple[str, ...] = ()
    blocking_diagnostics: tuple[str, ...] = ()
    contributions: tuple["DefinedFunctionContribution", ...] = ()
    order_scope: str = "full"
    required_presence_passed: bool = False
    authored_relative_order_passed: bool = False
    full_relative_order_passed: bool = False

    @property
    def ok(self) -> bool:
        return not self.breaks and not self.blocking_diagnostics


@dataclass(frozen=True)
class DefinedFunctionContribution:
    object_index: int
    section_number: int
    section_name: str
    value: int
    symbols: tuple[str, ...]
    comdat: bool
    weak: bool
    manifest_index: int | None
    selected_provider: str = ""
    linked_rva: int | None = None
    disposition: str = ""


@dataclass(frozen=True)
class VerificationResult:
    target: VerifyTarget
    function: VerifyFunction | VerifyDataSymbol
    item_kind: str
    mode: str
    mismatches: int
    relocation_or_text_metric: int
    secondary_metric: int
    bn_size_or_normalized: int
    vc5_size_or_diff_count: int
    evidence_path: Path
    triage_path: Path | None
    comparison: ObjectByteComparison


@dataclass(frozen=True)
class ProfileSweepRow:
    profile_name: str
    result: VerificationResult | None
    rc: int
    status: str = "compile"


@dataclass(frozen=True)
class OwnerVc5Issue:
    address: str
    kind: str
    message: str


@dataclass(frozen=True)
class OwnerVc5Scope:
    owner: SourceOwner
    required_entries: tuple[OwnerEntry, ...]
    selections: tuple[VerifySelection, ...]
    issues: tuple[OwnerVc5Issue, ...]

    @property
    def function_entry_count(self) -> int:
        return sum(1 for entry in self.required_entries if not entry.is_data_entry)

    @property
    def data_entry_count(self) -> int:
        return sum(1 for entry in self.required_entries if entry.is_data_entry)

    @property
    def selected_item_count(self) -> int:
        return sum(
            len(selection.functions) + len(selection.data_symbols)
            for selection in self.selections
        )


def normalize_generated_path(path_text: str) -> str:
    return validate_repository_relative_path(
        path_text,
        context="generated file path",
    )


ORDER_EDIT_PATH_SUFFIXES = {
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".h",
    ".hh",
    ".hpp",
    ".hxx",
    ".inl",
}


TrackedRepositoryFile = TrackedRepositoryPath


def normalize_order_edit_paths(
    value: Any,
    *,
    context: str,
    repository_root: Path = REPO_ROOT,
    inventory: GitTrackedPathInventory | None = None,
) -> tuple[str, ...]:
    if value is None:
        return ()
    if not isinstance(value, list):
        raise ValueError(f"{context}: order_edit_paths must be a list")
    result: list[str] = []
    seen: set[str] = set()
    tracked = inventory or load_git_tracked_path_inventory(repository_root)
    for index, path_text in enumerate(value):
        if not isinstance(path_text, str) or not path_text:
            raise ValueError(
                f"{context}: order_edit_paths[{index}] must be a non-empty string"
            )
        resolved = resolve_tracked_repository_file(
            path_text,
            context=f"{context}: order_edit_paths[{index}]",
            repository_root=tracked.repository_root,
            allowed_suffixes=ORDER_EDIT_PATH_SUFFIXES,
            inventory=tracked,
        )
        key = resolved.git_path.casefold()
        if key in seen:
            raise ValueError(f"{context}: duplicate order_edit_paths entry: {path_text}")
        seen.add(key)
        result.append(resolved.git_path)
    return tuple(result)


def _exact_tracked_manifest_path(
    path_text: str,
    *,
    inventory: GitTrackedPathInventory,
    context: str,
    allowed_suffixes: set[str] | None = None,
) -> str:
    return resolve_tracked_repository_file(
        path_text,
        context=context,
        repository_root=inventory.repository_root,
        allowed_suffixes=allowed_suffixes,
        inventory=inventory,
    ).git_path


def generated_file_shadows_project(relative_path: str) -> bool:
    normalized = normalize_generated_path(relative_path)
    return normalized.startswith(PROJECT_GENERATED_FILE_PREFIXES)


def source_from_text(source_from: str, manifest_path: Path) -> str:
    source_path = repo_path(source_from)
    if not source_path.exists():
        raise ValueError(f"{manifest_path}: source_from does not exist: {source_path}")
    return source_path.read_text(encoding="utf-8", errors="ignore")


def resolve_project_include(include_text: str, including_source: Path) -> Path | None:
    include_path = Path(include_text)
    candidates = (
        including_source.parent / include_path,
        REPO_ROOT / include_path,
        REPO_ROOT / "src" / include_path,
    )
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    return None


def _source_from_policy_text_uncached(
    source_from: str,
    manifest_path: Path,
) -> str:
    source_path = repo_path(source_from)
    if not source_path.exists():
        raise ValueError(f"{manifest_path}: source_from does not exist: {source_path}")
    visited: set[Path] = set()

    def collect(path: Path) -> list[str]:
        resolved = path.resolve()
        if resolved in visited:
            return []
        visited.add(resolved)

        text = path.read_text(encoding="utf-8", errors="ignore")
        chunks = [text]
        for match in QUOTED_INCLUDE_RE.finditer(text):
            include_text = match.group(1)
            if not include_text.lower().endswith((".h", ".hpp", ".inl", ".c", ".cpp")):
                continue
            include_path = resolve_project_include(include_text, path)
            if include_path is not None:
                chunks.extend(collect(include_path))
        return chunks

    return "\n".join(collect(source_path))


@lru_cache(maxsize=None)
def _canonical_source_from_policy_text(source_from: str) -> str:
    return _source_from_policy_text_uncached(
        source_from,
        DEFAULT_MANIFEST_DIR / "_canonical_source_policy_cache.json",
    )


def source_from_policy_text(source_from: str, manifest_path: Path) -> str:
    try:
        manifest_path.resolve().relative_to(DEFAULT_MANIFEST_DIR.resolve())
    except ValueError:
        return _source_from_policy_text_uncached(source_from, manifest_path)
    return _canonical_source_from_policy_text(source_from)


def validate_source_emission_anchor(
    *,
    source_from: str,
    anchor: EmissionAnchor,
    address: str,
) -> None:
    closure = collect_source_closure(source_from, repo_root=REPO_ROOT)
    normalized_path = normalize_anchor_path(anchor.path)
    anchor_path = (REPO_ROOT / normalized_path).resolve()
    anchor_files = tuple(
        source_file
        for source_file in closure
        if source_file.path.resolve() == anchor_path
    )
    if len(anchor_files) != 1:
        raise ValueError(
            f"emission_anchor.path {normalized_path!r} is not reachable from "
            f"source_from {source_from!r}"
        )

    anchor_file = anchor_files[0]
    constructs = source_emission_markers_lib._constructs(
        anchor_file.text,
        kind=anchor.kind,
        name=anchor.name,
    )
    if len(constructs) != 1:
        raise ValueError(
            f"emission_anchor {anchor.kind} {anchor.name!r} must resolve exactly once "
            f"in {normalized_path}; found {len(constructs)}"
        )
    if source_emission_markers_lib._attached_docblock(
        anchor_file.text,
        constructs[0],
    ) is None:
        raise ValueError(
            f"emission_anchor {anchor.kind} {anchor.name!r} in {normalized_path} "
            "has no immediately attached /** */ docblock"
        )

    legacy_marker = re.compile(
        rf"\bEmits\s+{re.escape(address)}:",
        re.IGNORECASE,
    )
    if any(legacy_marker.search(source_file.text) for source_file in closure):
        validate_source_emission_marker(
            source_from=source_from,
            repo_root=REPO_ROOT,
            anchor=anchor,
            address=address,
        )


def _load_source_trace_documents(
    source_paths: tuple[str, ...],
) -> tuple[SourceTraceDocument, ...]:
    documents: list[SourceTraceDocument] = []
    seen: set[Path] = set()
    for source_path in source_paths:
        for source_file in collect_source_closure(source_path, repo_root=REPO_ROOT):
            resolved = source_file.path.resolve()
            if resolved in seen:
                continue
            seen.add(resolved)
            documents.append(parse_source_trace_path(resolved, repo_root=REPO_ROOT))
    return tuple(documents)


@lru_cache(maxsize=None)
def _canonical_source_closure(source_path: str) -> tuple[Any, ...]:
    return collect_source_closure(source_path, repo_root=REPO_ROOT)


@lru_cache(maxsize=None)
def _canonical_source_trace_document(path: str) -> SourceTraceDocument:
    return parse_source_trace_path(Path(path), repo_root=REPO_ROOT)


@lru_cache(maxsize=None)
def _canonical_source_trace_documents(
    source_paths: tuple[str, ...],
) -> tuple[SourceTraceDocument, ...]:
    """Cache immutable production closures and parsed files per source root."""

    documents: list[SourceTraceDocument] = []
    seen: set[Path] = set()
    for source_path in source_paths:
        for source_file in _canonical_source_closure(source_path):
            resolved = source_file.path.resolve()
            if resolved in seen:
                continue
            seen.add(resolved)
            documents.append(
                _canonical_source_trace_document(str(resolved))
            )
    return tuple(documents)


def source_trace_documents(
    source_paths: tuple[str, ...],
    *,
    manifest_path: Path,
) -> tuple[SourceTraceDocument, ...]:
    try:
        try:
            manifest_path.resolve().relative_to(DEFAULT_MANIFEST_DIR.resolve())
        except ValueError:
            return _load_source_trace_documents(source_paths)
        return _canonical_source_trace_documents(source_paths)
    except (OSError, ValueError) as exc:
        raise ValueError(
            f"{manifest_path}: cannot parse canonical source-trace closure: {exc}"
        ) from exc


def canonical_function_artifact_id(
    function: VerifyFunction,
    *,
    target_binary: str,
) -> str:
    return normalize_artifact_id(
        function.logical_identity_key
        or f"{target_binary}:function:{function.address}"
    )


def _source_trace_finding_text(
    finding: Any,
) -> str:
    return (
        f"{finding.path}:{finding.line}: {finding.code}: {finding.message}"
    )


@dataclass(frozen=True)
class _SourceTraceLookup:
    artifacts_by_id: Mapping[str, tuple[SourceTraceArtifact, ...]]
    artifacts_by_address: Mapping[str, tuple[SourceTraceArtifact, ...]]
    findings: tuple[Any, ...]


_CANONICAL_SOURCE_TRACE_LOOKUPS: dict[
    tuple[str, ...], _SourceTraceLookup
] = {}


def _build_source_trace_lookup(
    documents: tuple[SourceTraceDocument, ...],
) -> _SourceTraceLookup:
    by_id: dict[str, list[SourceTraceArtifact]] = {}
    by_address: dict[str, list[SourceTraceArtifact]] = {}
    for document in documents:
        for artifact in document.artifacts:
            by_id.setdefault(artifact.artifact_id, []).append(artifact)
            if artifact.address is not None:
                by_address.setdefault(artifact.address, []).append(artifact)
    findings = (
        *(finding for document in documents for finding in document.findings),
        *merge_source_trace_documents(documents),
    )
    return _SourceTraceLookup(
        artifacts_by_id={
            key: tuple(value) for key, value in by_id.items()
        },
        artifacts_by_address={
            key: tuple(value) for key, value in by_address.items()
        },
        findings=tuple(findings),
    )


def _source_trace_lookup(
    documents: tuple[SourceTraceDocument, ...],
    *,
    manifest_path: Path,
) -> _SourceTraceLookup:
    try:
        manifest_path.resolve().relative_to(DEFAULT_MANIFEST_DIR.resolve())
    except ValueError:
        return _build_source_trace_lookup(documents)
    key = tuple(document.path for document in documents)
    lookup = _CANONICAL_SOURCE_TRACE_LOOKUPS.get(key)
    if lookup is None:
        lookup = _build_source_trace_lookup(documents)
        _CANONICAL_SOURCE_TRACE_LOOKUPS[key] = lookup
    return lookup


def find_canonical_source_artifact(
    *,
    documents: tuple[SourceTraceDocument, ...],
    artifact_id: str,
    relation: str | frozenset[str] | None,
    expected_section: str | None,
    manifest_path: Path,
    context: str,
    allowed_construct_kinds: frozenset[str],
    emission_anchor: EmissionAnchor | None = None,
) -> SourceTraceArtifact | None:
    normalized_id = normalize_artifact_id(artifact_id)
    lookup = _source_trace_lookup(
        documents,
        manifest_path=manifest_path,
    )
    matches = lookup.artifacts_by_id.get(normalized_id, ())
    normalized_address = artifact_address(normalized_id)
    same_address = (
        lookup.artifacts_by_address.get(normalized_address, ())
        if normalized_address is not None
        else ()
    )
    if not matches:
        if same_address:
            actual = ", ".join(sorted({artifact.artifact_id for artifact in same_address}))
            raise ValueError(
                f"{manifest_path}: {context} requires exact canonical artifact id "
                f"{normalized_id!r}; source declares {actual}"
            )
        return None

    findings = lookup.findings
    if findings:
        raise ValueError(
            f"{manifest_path}: {context} has invalid canonical source-trace directives: "
            + "; ".join(_source_trace_finding_text(finding) for finding in findings)
        )
    if len(matches) != 1:
        locations = ", ".join(f"{item.path}:{item.line}" for item in matches)
        raise ValueError(
            f"{manifest_path}: {context} canonical artifact {normalized_id!r} must "
            f"occur exactly once; found {len(matches)} at {locations}"
        )

    artifact = matches[0]
    allowed_relations = (
        relation
        if isinstance(relation, frozenset)
        else (frozenset({relation}) if relation is not None else None)
    )
    if allowed_relations is not None and artifact.relation not in allowed_relations:
        relation_expectation = (
            repr(relation)
            if isinstance(relation, str)
            else f"one of {sorted(allowed_relations)!r}"
        )
        raise ValueError(
            f"{manifest_path}: {context} canonical artifact {normalized_id!r} must use "
            f"{relation_expectation}, not {artifact.relation!r}"
        )
    if expected_section is not None and artifact.section != expected_section:
        raise ValueError(
            f"{manifest_path}: {context} canonical artifact {normalized_id!r} must name "
            f"exact output section {expected_section!r}, not {artifact.section!r}"
        )
    if expected_section is None and artifact.entity_kind == "data":
        raise ValueError(
            f"{manifest_path}: {context} canonical data artifact {normalized_id!r} "
            "has no exact registered tracker output section"
        )
    if not artifact.direct or artifact.construct is None:
        raise ValueError(
            f"{manifest_path}: {context} canonical artifact {normalized_id!r} is not "
            "directly attached to a supported source definition"
        )
    if artifact.construct.kind not in allowed_construct_kinds:
        raise ValueError(
            f"{manifest_path}: {context} canonical artifact {normalized_id!r} is attached "
            f"to {artifact.construct.kind!r}; expected one of "
            f"{sorted(allowed_construct_kinds)}"
        )

    if emission_anchor is not None:
        normalized_path = normalize_anchor_path(emission_anchor.path)
        expected_kind = {
            "type-definition": "type",
            "function-definition": "function",
            "data-definition": "data",
        }[emission_anchor.kind]
        if artifact.path != normalized_path:
            raise ValueError(
                f"{manifest_path}: {context} canonical artifact {normalized_id!r} is "
                f"attached in {artifact.path!r}, not emission_anchor.path "
                f"{normalized_path!r}"
            )
        if artifact.construct.kind != expected_kind:
            raise ValueError(
                f"{manifest_path}: {context} canonical artifact {normalized_id!r} is "
                f"attached to {artifact.construct.kind!r}, not emission_anchor.kind "
                f"{emission_anchor.kind!r}"
            )
        if artifact.construct.name != emission_anchor.name:
            raise ValueError(
                f"{manifest_path}: {context} canonical artifact {normalized_id!r} is "
                f"attached to {artifact.construct.name!r}, not emission_anchor.name "
                f"{emission_anchor.name!r}"
            )
    return artifact


def validate_current_authored_icf_physical_source_policy(
    *,
    manifest_path: Path,
    function: VerifyFunction,
    target_binary: str,
    documents: tuple[SourceTraceDocument, ...],
    context: str,
    select_single_logical_member: bool,
) -> bool:
    """Validate one current physical ICF gate through its logical mirrors."""

    if function.logical_identity_key:
        return False
    tracker_data = canonical_tracker_data_for_manifest(manifest_path)
    if tracker_data is None:
        return False
    physical_artifact_id = canonical_function_artifact_id(
        function,
        target_binary=target_binary,
    )
    try:
        logical_mirrors = validate_authored_icf_physical_source_artifacts(
            tracker_data,
            physical_symbol_id=physical_artifact_id,
            documents=documents,
            select_single_logical_member=select_single_logical_member,
        )
    except ProgressError as exc:
        raise ValueError(
            f"{manifest_path}: {context} physical authored-ICF source mirrors "
            f"are invalid: {exc}"
        ) from exc
    return logical_mirrors is not None


def current_authored_icf_translation_unit_object_symbol(
    *,
    manifest_path: Path,
    function: VerifyFunction,
    target_binary: str,
    translation_unit: str,
) -> str | None:
    """Return the current proof-bound COFF selector for one physical TU gate."""

    if function.logical_identity_key:
        return None
    tracker_data = canonical_tracker_data_for_manifest(manifest_path)
    if tracker_data is None:
        return None
    physical_artifact_id = canonical_function_artifact_id(
        function,
        target_binary=target_binary,
    )
    try:
        selection = select_authored_icf_translation_unit_object_symbol(
            tracker_data,
            physical_symbol_id=physical_artifact_id,
            translation_unit=translation_unit,
        )
    except ProgressError as exc:
        raise ValueError(
            f"{manifest_path}: physical authored-ICF gate {function.address} "
            f"cannot select a current object symbol for translation unit "
            f"{translation_unit!r}: {exc}"
        ) from exc
    return selection[1] if selection is not None else None


def validate_generated_source_emission_policy(
    *,
    function: VerifyFunction,
    source_from: str,
    manifest_path: Path,
    context: str,
    strict_source_emissions: bool,
    strict_source_traceability: bool,
    target_binary: str,
    trace_documents: tuple[SourceTraceDocument, ...],
    tracker_source_trace_state: str | None = None,
) -> SourceEmissionWarning | None:
    if function.authored_order_role not in COMPILER_GENERATED_AUTHORED_ORDER_ROLES:
        return None
    required_relation = (
        "emits"
        if function.authored_order_role == "compiler-generated-thunk"
        and function.emission_anchor is not None
        and function.emission_anchor.kind == "data-definition"
        else None
    )
    canonical = find_canonical_source_artifact(
        documents=trace_documents,
        artifact_id=canonical_function_artifact_id(
            function,
            target_binary=target_binary,
        ),
        relation=required_relation,
        expected_section=".text",
        manifest_path=manifest_path,
        context=context,
        allowed_construct_kinds=frozenset(
            kind.removesuffix("-definition")
            for kind in COMPILER_GENERATED_ROLE_ANCHOR_KINDS[
                function.authored_order_role
            ]
        ),
        emission_anchor=function.emission_anchor,
    )
    if canonical is not None:
        if tracker_source_trace_state == "unresolved":
            raise ValueError(
                f"{manifest_path}: {context} {function.address} has an explicitly "
                "unresolved tracker source_trace state but carries a canonical "
                "source edge"
            )
        return None
    if function.emission_anchor is None:
        warning = SourceEmissionWarning(
            address=function.address,
            code="missing-source-emission-anchor",
            source_from=source_from,
            message=(
                f"{context} compiler-generated role {function.authored_order_role!r} lacks "
                "emission_anchor metadata"
            ),
        )
        if (
            strict_source_emissions or strict_source_traceability
        ) and tracker_source_trace_state != "unresolved":
            raise ValueError(
                f"{manifest_path}: {warning.address} {warning.message}; "
                + (
                    "strict source-traceability policy is enabled"
                    if strict_source_traceability
                    else "strict source-emission policy is enabled"
                )
            )
        return warning
    try:
        validate_source_emission_anchor(
            source_from=source_from,
            anchor=function.emission_anchor,
            address=function.address,
        )
    except (OSError, ValueError) as exc:
        raise ValueError(
            f"{manifest_path}: {context} invalid emission_anchor for {function.address}: {exc}"
        ) from exc
    if strict_source_traceability:
        raise ValueError(
            f"{manifest_path}: {context} compiler-generated role "
            f"{function.authored_order_role!r} at {function.address} uses legacy "
            "Emits inventory; strict source-traceability policy requires an attached "
            f"'@recoil-artifact <defines|emits> .text "
            f"{canonical_function_artifact_id(function, target_binary=target_binary)}: "
            "<description>' directive, using defines for a direct source body or emits "
            "for an artifact caused by another legitimate source construct"
        )
    return None


def validate_source_policy(
    *,
    data: dict[str, Any],
    manifest_path: Path,
    source_from: str,
    functions: tuple[VerifyFunction, ...],
    data_symbols: tuple[VerifyDataSymbol, ...],
    generated_files: tuple[tuple[str, str], ...],
    target_binary: str,
    strict_source_emissions: bool = False,
    strict_source_traceability: bool = False,
    tracked_path_inventory: GitTrackedPathInventory | None = None,
) -> tuple[SourceEmissionWarning, ...]:
    has_source_from = bool(source_from)
    warnings: list[SourceEmissionWarning] = []

    if source_from and "source" in data:
        raise ValueError(f"{manifest_path}: use either 'source_from' or 'source', not both")

    if has_source_from:
        # Multi-file order targets (split TU hosts) declare exact writable paths in
        # order_edit_paths. Provenance may live on any of those hosts, not only the
        # primary source_from compile host.
        policy_paths: list[str] = [source_from]
        raw_order_edit_paths = data.get("order_edit_paths")
        if raw_order_edit_paths is not None:
            for path in normalize_order_edit_paths(
                raw_order_edit_paths,
                context=str(manifest_path),
                inventory=tracked_path_inventory,
            ):
                if path not in policy_paths:
                    policy_paths.append(path)
        production_text = "\n".join(
            source_from_policy_text(path, manifest_path) for path in policy_paths
        )
        canonical_present = "@recoil-" in production_text
        trace_documents = (
            source_trace_documents(tuple(policy_paths), manifest_path=manifest_path)
            if canonical_present or strict_source_traceability
            else ()
        )
        for function in functions:
            trace_function = effective_source_trace_function(
                manifest_path=manifest_path,
                function=function,
                target_binary=target_binary,
            )
            tracker_metadata = canonical_tracker_metadata_for_manifest(
                manifest_path=manifest_path,
                artifact_id=canonical_function_artifact_id(
                    trace_function,
                    target_binary=target_binary,
                ),
            )
            tracker_source_trace_state = (
                tracker_metadata[2] if tracker_metadata is not None else None
            )
            trace_pipeline_class = trace_function.pipeline_class
            exact_artifact_id = canonical_function_artifact_id(
                trace_function,
                target_binary=target_binary,
            )
            exact_source_rows = tuple(
                artifact
                for document in trace_documents
                for artifact in document.artifacts
                if artifact.artifact_id == exact_artifact_id
            )
            # Applicability is decided before generated-role shape.  A reviewed
            # physical non-authored/provider row has no production source edge,
            # even if its compiler-shaped role resembles an emitted helper.
            # An authored logical alias at the same physical address reaches
            # this point with its own exact tracker metadata and is still
            # validated below.
            if (
                function.provenance == FUNCTION_PROVENANCE_PROVIDER_BOUNDARY
                or trace_pipeline_class == "non-authored"
            ):
                if exact_source_rows:
                    raise ValueError(
                        f"{manifest_path}: non-authored/provider function "
                        f"{function.address} is not-applicable for a production source "
                        f"edge and cannot carry canonical artifact {exact_artifact_id!r}"
                    )
                continue
            generated_warning = validate_generated_source_emission_policy(
                function=trace_function,
                source_from=source_from,
                manifest_path=manifest_path,
                context="function",
                strict_source_emissions=strict_source_emissions,
                strict_source_traceability=strict_source_traceability,
                target_binary=target_binary,
                trace_documents=trace_documents,
                tracker_source_trace_state=tracker_source_trace_state,
            )
            if trace_function.authored_order_role in COMPILER_GENERATED_AUTHORED_ORDER_ROLES:
                if generated_warning is not None:
                    warnings.append(generated_warning)
                continue
            compiler_emitted_provenance = (
                function.provenance == FUNCTION_PROVENANCE_COMPILER_EMITTED_NONCOVERING
                and trace_function.authored_order_role
                in COMPILER_GENERATED_AUTHORED_ORDER_ROLES
            )
            if compiler_emitted_provenance:
                canonical = find_canonical_source_artifact(
                    documents=trace_documents,
                    artifact_id=canonical_function_artifact_id(
                        function,
                        target_binary=target_binary,
                    ),
                    relation="emits",
                    expected_section=".text",
                    manifest_path=manifest_path,
                    context="function",
                    allowed_construct_kinds=frozenset({"function", "data", "type"}),
                )
                if canonical is not None:
                    if trace_pipeline_class not in AUTHORED_PIPELINE_CLASSES:
                        raise ValueError(
                            f"{manifest_path}: compiler-emitted function "
                            f"{function.address} has pipeline_class "
                            f"{trace_pipeline_class!r} and cannot claim a canonical "
                            "source edge until its authored identity is resolved"
                        )
                    continue
                if (
                    strict_source_traceability
                    and trace_pipeline_class in AUTHORED_PIPELINE_CLASSES
                ):
                    raise ValueError(
                        f"{manifest_path}: function {function.address} requires an attached "
                        "canonical '@recoil-artifact emits .text "
                        f"{canonical_function_artifact_id(function, target_binary=target_binary)}: "
                        "<description>' directive; legacy compiler-emitted markers are "
                        "migration inventory only"
                    )
                continue
            if function.provenance and not (
                function.provenance == FUNCTION_PROVENANCE_COMPILER_EMITTED_NONCOVERING
                and trace_function.authored_order_role
                not in COMPILER_GENERATED_AUTHORED_ORDER_ROLES
            ):
                raise ValueError(
                    f"{manifest_path}: function provenance marker {function.provenance!r} "
                    f"for {function.address} is only supported in "
                    "translation_unit_function_order functions"
                )
            if validate_current_authored_icf_physical_source_policy(
                manifest_path=manifest_path,
                function=function,
                target_binary=target_binary,
                documents=trace_documents,
                context="function",
                select_single_logical_member=True,
            ):
                continue
            canonical = find_canonical_source_artifact(
                documents=trace_documents,
                artifact_id=canonical_function_artifact_id(
                    function,
                    target_binary=target_binary,
                ),
                relation="defines",
                expected_section=".text",
                manifest_path=manifest_path,
                context="function",
                allowed_construct_kinds=frozenset({"function"}),
            )
            if canonical is not None:
                if trace_pipeline_class not in AUTHORED_PIPELINE_CLASSES:
                    raise ValueError(
                        f"{manifest_path}: function {function.address} has pipeline_class "
                        f"{trace_pipeline_class!r} and cannot claim a canonical authored "
                        "source edge until its authored identity is resolved"
                    )
                continue
            if (
                strict_source_traceability
                and trace_pipeline_class in AUTHORED_PIPELINE_CLASSES
            ):
                raise ValueError(
                    f"{manifest_path}: function {function.address} requires an attached "
                    "canonical '@recoil-artifact defines .text "
                    f"{canonical_function_artifact_id(function, target_binary=target_binary)}: "
                    "<description>' directive; legacy Reimplements markers are migration "
                    "inventory only"
                )
        if strict_source_traceability or strict_source_emissions:
            for data_symbol in data_symbols:
                artifact_id = normalize_artifact_id(
                    data_symbol.logical_identity_key
                    or f"{target_binary}:data:{data_symbol.address}"
                )
                matching_rows = tuple(
                    artifact
                    for document in trace_documents
                    for artifact in document.artifacts
                    if artifact.artifact_id == artifact_id
                )
                if not matching_rows:
                    continue
                try:
                    tracker_row = tracker_artifact_index_for_manifest(
                        manifest_path
                    ).resolve(artifact_id)
                except (OSError, ValueError, json.JSONDecodeError) as exc:
                    raise ValueError(
                        f"{manifest_path}: data symbol {artifact_id!r} has a canonical "
                        f"source row but exact tracker artifacts cannot be loaded: {exc}"
                    ) from exc
                if tracker_row is None:
                    raise ValueError(
                        f"{manifest_path}: data symbol {artifact_id!r} does not resolve "
                        "through the exact physical/logical tracker artifact index, so "
                        "its exact data output section cannot be validated"
                    )
                if tracker_row.output_section is None:
                    raise ValueError(
                        f"{manifest_path}: data symbol {artifact_id!r} has no exact "
                        "registered tracker output section"
                    )
                canonical = find_canonical_source_artifact(
                    documents=trace_documents,
                    artifact_id=artifact_id,
                    relation=frozenset({"defines", "emits"}),
                    expected_section=tracker_row.output_section,
                    manifest_path=manifest_path,
                    context="data symbol",
                    allowed_construct_kinds=frozenset(
                        {"data", "function", "type", "macro"}
                    ),
                )
                # VC data rows do not carry the tracker source-trace state or an
                # authored/compiler-generated relation.  Validate an inline
                # canonical direct definition exactly when one exists; the
                # repository-wide migrated graph audit owns required/omitted
                # decisions for resolved, unresolved, and not-applicable rows.
    else:
        raise ValueError(f"{manifest_path}: expected 'source_from' for production verification")

    for generated_path, _contents in generated_files:
        normalized_generated_path = normalize_generated_path(generated_path)
        if not generated_file_shadows_project(normalized_generated_path):
            continue
        raise ValueError(
            f"{manifest_path}: generated file shadows project header '{generated_path}'; "
            "fix VC5SP3 compatibility in production headers"
        )
    return tuple(warnings)


def optional_positive_int(data: dict[str, Any], key: str, *, manifest_path: Path) -> int | None:
    value = data.get(key)
    if value is None:
        return None
    if isinstance(value, bool) or not isinstance(value, int):
        raise ValueError(f"{manifest_path}: expected '{key}' to be a positive integer")
    if value <= 0:
        raise ValueError(f"{manifest_path}: expected '{key}' to be a positive integer")
    return value


def optional_nonnegative_int(
    data: dict[str, Any],
    key: str,
    *,
    manifest_path: Path,
) -> int | None:
    value = data.get(key)
    if value is None:
        return None
    if isinstance(value, bool) or not isinstance(value, int) or value < 0:
        raise ValueError(f"{manifest_path}: expected '{key}' to be a non-negative integer")
    return value


def optional_symbol_regex(data: dict[str, Any], address: str, *, manifest_path: Path) -> str | None:
    symbol_regex = data.get("symbol_regex")
    if symbol_regex is None:
        return None
    if not isinstance(symbol_regex, str) or not symbol_regex:
        raise ValueError(f"{manifest_path}: expected non-empty symbol_regex string for {address}")
    try:
        re.compile(symbol_regex)
    except re.error as exc:
        raise ValueError(f"{manifest_path}: invalid symbol_regex for {address}: {exc}") from exc
    return symbol_regex


def optional_listing_label_regex(
    data: dict[str, Any],
    address: str,
    *,
    manifest_path: Path,
    allow: bool,
) -> str | None:
    listing_label_regex = data.get("listing_label_regex")
    if listing_label_regex is None:
        return None
    if not allow:
        raise ValueError(
            f"{manifest_path}: listing_label_regex is only supported in "
            f"translation_unit_function_order diagnostics for {address}"
        )
    if not isinstance(listing_label_regex, str) or not listing_label_regex:
        raise ValueError(f"{manifest_path}: expected non-empty listing_label_regex string for {address}")
    try:
        re.compile(listing_label_regex)
    except re.error as exc:
        raise ValueError(f"{manifest_path}: invalid listing_label_regex for {address}: {exc}") from exc
    return listing_label_regex


def optional_emission_anchor(
    data: dict[str, Any],
    address: str,
    *,
    manifest_path: Path,
) -> EmissionAnchor | None:
    raw = data.get("emission_anchor")
    if raw is None:
        return None
    if not isinstance(raw, dict):
        raise ValueError(f"{manifest_path}: emission_anchor must be an object for {address}")
    unknown = sorted(set(raw) - {"path", "kind", "name"})
    if unknown:
        raise ValueError(
            f"{manifest_path}: unsupported emission_anchor keys for {address}: {', '.join(unknown)}"
        )
    missing = sorted({"path", "kind", "name"} - set(raw))
    if missing:
        raise ValueError(
            f"{manifest_path}: emission_anchor for {address} is missing: {', '.join(missing)}"
        )
    path = raw["path"]
    kind = raw["kind"]
    name = raw["name"]
    if not isinstance(path, str):
        raise ValueError(f"{manifest_path}: emission_anchor.path must be a string for {address}")
    try:
        normalized_path = normalize_anchor_path(path)
    except ValueError as exc:
        raise ValueError(f"{manifest_path}: {exc} for {address}") from exc
    if kind not in ANCHOR_KINDS:
        raise ValueError(
            f"{manifest_path}: emission_anchor.kind must be one of {sorted(ANCHOR_KINDS)} "
            f"for {address}"
        )
    if not isinstance(name, str) or not name.strip():
        raise ValueError(f"{manifest_path}: emission_anchor.name must be a non-empty string for {address}")
    if name != name.strip():
        raise ValueError(
            f"{manifest_path}: emission_anchor.name must not have surrounding whitespace for {address}"
        )
    return EmissionAnchor(path=normalized_path, kind=kind, name=name)


def parse_verify_function(
    item: Any,
    *,
    manifest_path: Path,
    seen_addresses: set[str],
    context: str,
    allow_listing_label_regex: bool = False,
    allow_source_order_gate: bool = False,
    default_pipeline_class: str = "unresolved",
    allow_logical_identity: bool = False,
) -> VerifyFunction:
    if not isinstance(item, dict):
        raise ValueError(f"{manifest_path}: {context} entries must be objects")
    address = normalize_address(require_string(item, "address", manifest_path=manifest_path))
    logical_identity_key = item.get("logical_identity_key", "")
    if not isinstance(logical_identity_key, str):
        raise ValueError(f"{manifest_path}: logical_identity_key must be a string for {address}")
    if logical_identity_key and not allow_logical_identity:
        raise ValueError(
            f"{manifest_path}: logical_identity_key is supported only in authored-order entries for {address}"
        )
    existing_address_token = address in seen_addresses
    existing_logical_tokens = any(token.startswith(address + "#logical:") for token in seen_addresses)
    identity_token = (
        f"{address}#logical:{logical_identity_key}" if logical_identity_key else address
    )
    if identity_token in seen_addresses or (
        logical_identity_key and existing_address_token
    ) or (
        not logical_identity_key and existing_logical_tokens
    ):
        raise ValueError(f"{manifest_path}: duplicate function identity at {address}")
    seen_addresses.add(identity_token)
    icf_fold_status = item.get("icf_fold_status", "")
    if not isinstance(icf_fold_status, str):
        raise ValueError(f"{manifest_path}: icf_fold_status must be a string for {address}")
    if logical_identity_key:
        if icf_fold_status not in ICF_ALIAS_MANIFEST_FOLD_STATUSES:
            raise ValueError(
                f"{manifest_path}: logical identity {logical_identity_key} requires "
                "icf_fold_status selected-winner, proven-fold-alias, or not-established"
            )
    elif icf_fold_status:
        raise ValueError(f"{manifest_path}: icf_fold_status requires logical_identity_key for {address}")
    symbol = item.get("symbol", "")
    if not isinstance(symbol, str):
        raise ValueError(f"{manifest_path}: expected function symbol as a string for {address}")
    symbol_regex = optional_symbol_regex(item, address, manifest_path=manifest_path)
    listing_label_regex = optional_listing_label_regex(
        item,
        address,
        manifest_path=manifest_path,
        allow=allow_listing_label_regex,
    )
    if not symbol and symbol_regex is None and listing_label_regex is None:
        raise ValueError(
            f"{manifest_path}: expected function symbol, symbol_regex, or listing_label_regex "
            f"for {address}"
        )
    name = item.get("name", symbol)
    if not isinstance(name, str) or not name:
        raise ValueError(f"{manifest_path}: expected non-empty function name for {address}")
    provenance = ""
    if "provenance" in item:
        provenance_value = item["provenance"]
        if not isinstance(provenance_value, str) or not provenance_value:
            raise ValueError(
                f"{manifest_path}: expected non-empty function provenance marker for {address}"
            )
        if provenance_value not in FUNCTION_PROVENANCE_VALUES:
            valid = ", ".join(sorted(FUNCTION_PROVENANCE_VALUES))
            raise ValueError(
                f"{manifest_path}: unknown function provenance marker {provenance_value!r} "
                f"for {address}; expected one of: {valid}"
            )
        provenance = provenance_value
    if "source_order_gate" in item and not allow_source_order_gate:
        raise ValueError(
            f"{manifest_path}: source_order_gate is only supported in "
            f"translation_unit_function_order diagnostics for {address}"
        )
    source_order_gate = optional_bool(item, "source_order_gate", True, manifest_path=manifest_path)
    pipeline_class = item.get("pipeline_class", default_pipeline_class)
    if pipeline_class not in PIPELINE_CLASS_VALUES:
        raise ValueError(
            f"{manifest_path}: invalid pipeline_class {pipeline_class!r} for {address}; "
            f"expected one of {sorted(PIPELINE_CLASS_VALUES)}"
        )
    authored_order_role_value = item.get(
        "authored_order_role",
        default_authored_order_role(pipeline_class),
    )
    if authored_order_role_value not in AUTHORED_ORDER_ROLE_VALUES:
        raise ValueError(
            f"{manifest_path}: invalid authored_order_role {authored_order_role_value!r} "
            f"for {address}; expected one of {sorted(AUTHORED_ORDER_ROLE_VALUES)}"
        )
    compatible_roles = {
        "authored": {"authored-body"},
        "authored-lifecycle": {
            "authored-lifecycle-body",
            *COMPILER_GENERATED_AUTHORED_ORDER_ROLES,
        },
        "non-authored": {"non-authored", *COMPILER_GENERATED_AUTHORED_ORDER_ROLES},
        "unresolved": {"unresolved"},
    }
    if authored_order_role_value not in compatible_roles[pipeline_class]:
        raise ValueError(
            f"{manifest_path}: authored_order_role {authored_order_role_value!r} is not "
            f"compatible with pipeline_class {pipeline_class!r} for {address}"
        )
    required_presence = optional_bool(item, "required_presence", True, manifest_path=manifest_path)
    full_order_gate = optional_bool(
        item,
        "full_order_gate",
        required_presence,
        manifest_path=manifest_path,
    )
    if authored_order_role_value in COMPILER_GENERATED_AUTHORED_ORDER_ROLES and (
        not required_presence or not full_order_gate
    ):
        raise ValueError(
            f"{manifest_path}: deferred compiler-generated authored-order row {address} "
            "must keep required_presence=true and full_order_gate=true for later full order"
        )
    emission_anchor = optional_emission_anchor(item, address, manifest_path=manifest_path)
    if emission_anchor is not None:
        if authored_order_role_value not in COMPILER_GENERATED_AUTHORED_ORDER_ROLES:
            raise ValueError(
                f"{manifest_path}: emission_anchor is supported only for compiler-generated "
                f"authored-order roles at {address}"
            )
        allowed_anchor_kinds = COMPILER_GENERATED_ROLE_ANCHOR_KINDS[authored_order_role_value]
        if emission_anchor.kind not in allowed_anchor_kinds:
            raise ValueError(
                f"{manifest_path}: authored_order_role {authored_order_role_value!r} requires "
                f"emission_anchor.kind in {sorted(allowed_anchor_kinds)} for {address}, "
                f"not {emission_anchor.kind!r}"
            )
    if logical_identity_key and (
        pipeline_class not in AUTHORED_PIPELINE_CLASSES
        or authored_order_role_value not in AUTHORED_ORDER_GATING_ROLES
        or full_order_gate
    ):
        raise ValueError(
            f"{manifest_path}: logical ICF identity {logical_identity_key} must be an authored "
            "presence gate with full_order_gate=false"
        )
    return VerifyFunction(
        address=address,
        symbol=symbol,
        name=name,
        bn_byte_length=optional_positive_int(item, "bn_byte_length", manifest_path=manifest_path),
        vc5_byte_length=optional_positive_int(item, "vc5_byte_length", manifest_path=manifest_path),
        symbol_regex=symbol_regex,
        listing_label_regex=listing_label_regex,
        provenance=provenance,
        source_order_gate=source_order_gate,
        pipeline_class=pipeline_class,
        authored_order_role=authored_order_role_value,
        required_presence=required_presence,
        full_order_gate=full_order_gate,
        logical_identity_key=logical_identity_key,
        icf_fold_status=icf_fold_status,
        emission_anchor=emission_anchor,
    )


def parse_order_scope(data: dict[str, Any], *, context: str, manifest_path: Path) -> str:
    value = data.get("order_scope", "full")
    if value not in ORDER_SCOPE_VALUES:
        raise ValueError(
            f"{manifest_path}: {context}.order_scope must be one of {sorted(ORDER_SCOPE_VALUES)}"
        )
    return value


def parse_candidate_only_extras(
    data: dict[str, Any],
    *,
    context: str,
    manifest_path: Path,
) -> tuple[CandidateOnlyExtra, ...]:
    raw = data.get("candidate_only_extras", [])
    if not isinstance(raw, list):
        raise ValueError(f"{manifest_path}: {context}.candidate_only_extras must be a list")
    rows: list[CandidateOnlyExtra] = []
    for index, item in enumerate(raw):
        if not isinstance(item, dict):
            raise ValueError(f"{manifest_path}: {context}.candidate_only_extras[{index}] must be an object")
        if "address" in item:
            raise ValueError(
                f"{manifest_path}: {context}.candidate_only_extras[{index}] must not fabricate a retail address"
            )
        name = require_string(item, "name", manifest_path=manifest_path)
        pipeline_class = item.get("pipeline_class")
        if pipeline_class not in PIPELINE_CLASS_VALUES:
            raise ValueError(
                f"{manifest_path}: {context}.candidate_only_extras[{index}] has invalid pipeline_class"
            )
        symbol = item.get("symbol", "")
        symbol_regex = item.get("symbol_regex")
        if not isinstance(symbol, str) or (symbol_regex is not None and not isinstance(symbol_regex, str)):
            raise ValueError(f"{manifest_path}: {context}.candidate_only_extras[{index}] selector must be a string")
        if bool(symbol) == bool(symbol_regex):
            raise ValueError(
                f"{manifest_path}: {context}.candidate_only_extras[{index}] requires exactly one of symbol or symbol_regex"
            )
        if symbol_regex is not None:
            re.compile(symbol_regex)
        rows.append(
            CandidateOnlyExtra(
                name=name,
                pipeline_class=pipeline_class,
                symbol=symbol,
                symbol_regex=symbol_regex,
            )
        )
    return tuple(rows)


def parse_translation_unit_function_order(
    data: dict[str, Any],
    *,
    manifest_path: Path,
    reusable_functions: tuple[VerifyFunction, ...] = (),
    tracked_path_inventory: GitTrackedPathInventory | None = None,
    strict_tracked_paths: bool = False,
) -> tuple[bool, tuple[TranslationUnitFunctionOrderEntry, ...]]:
    check_translation_unit_function_order = optional_bool(
        data,
        "check_translation_unit_function_order",
        False,
        manifest_path=manifest_path,
    )
    entries_data = data.get("translation_unit_function_order", [])
    if entries_data and not check_translation_unit_function_order:
        raise ValueError(
            f"{manifest_path}: translation_unit_function_order requires "
            "'check_translation_unit_function_order': true"
        )
    if not isinstance(entries_data, list):
        raise ValueError(f"{manifest_path}: expected 'translation_unit_function_order' as a list")
    if check_translation_unit_function_order and not entries_data:
        raise ValueError(
            f"{manifest_path}: check_translation_unit_function_order requires "
            "translation_unit_function_order entries"
        )

    entries: list[TranslationUnitFunctionOrderEntry] = []
    seen_addresses: set[str] = set()
    for entry_index, entry_data in enumerate(entries_data):
        if not isinstance(entry_data, dict):
            raise ValueError(f"{manifest_path}: translation_unit_function_order entries must be objects")
        source_from = require_string(entry_data, "source_from", manifest_path=manifest_path)
        if strict_tracked_paths:
            if tracked_path_inventory is None:
                raise ValueError(
                    f"{manifest_path}: tracked inventory is required for current manifest paths"
                )
            source_from = _exact_tracked_manifest_path(
                source_from,
                inventory=tracked_path_inventory,
                context=(
                    f"{manifest_path}: translation_unit_function_order[{entry_index}]."
                    "source_from"
                ),
                allowed_suffixes=ORDER_EDIT_PATH_SUFFIXES,
            )
        inventory_only = optional_bool(
            entry_data,
            "inventory_only",
            False,
            manifest_path=manifest_path,
        )
        functions_data = entry_data.get("functions", [])
        function_address_refs = entry_data.get("function_address_refs", [])
        if inventory_only:
            if "functions" not in entry_data or functions_data != []:
                raise ValueError(
                    f"{manifest_path}: translation_unit_function_order[{entry_index}] with "
                    "inventory_only=true requires explicit functions: []"
                )
            if function_address_refs:
                raise ValueError(
                    f"{manifest_path}: translation_unit_function_order[{entry_index}] with "
                    "inventory_only=true cannot use function_address_refs"
                )
        if functions_data and function_address_refs:
            raise ValueError(
                f"{manifest_path}: translation_unit_function_order[{entry_index}] cannot combine "
                "functions and function_address_refs"
            )
        if function_address_refs:
            if not isinstance(function_address_refs, list) or any(not isinstance(item, str) for item in function_address_refs):
                raise ValueError(
                    f"{manifest_path}: translation_unit_function_order[{entry_index}].function_address_refs "
                    "must be a string list"
                )
            reusable_by_address = {function.address: function for function in reusable_functions}
            normalized_refs = [normalize_address(item) for item in function_address_refs]
            if len(set(normalized_refs)) != len(normalized_refs):
                raise ValueError(
                    f"{manifest_path}: translation_unit_function_order[{entry_index}].function_address_refs "
                    "contains duplicates"
                )
            missing_refs = [item for item in normalized_refs if item not in reusable_by_address]
            if missing_refs:
                raise ValueError(
                    f"{manifest_path}: translation_unit_function_order[{entry_index}] references unknown "
                    f"top-level function addresses: {', '.join(missing_refs)}"
                )
            functions_data = [
                {
                    "address": reusable_by_address[item].address,
                    **(
                        {"symbol": reusable_by_address[item].symbol}
                        if reusable_by_address[item].symbol
                        else {}
                    ),
                    **(
                        {"symbol_regex": reusable_by_address[item].symbol_regex}
                        if reusable_by_address[item].symbol_regex is not None
                        else {}
                    ),
                    "name": reusable_by_address[item].name,
                    "pipeline_class": reusable_by_address[item].pipeline_class,
                    "authored_order_role": reusable_by_address[item].authored_order_role,
                    "required_presence": reusable_by_address[item].required_presence,
                    "full_order_gate": reusable_by_address[item].full_order_gate,
                    **(
                        {"provenance": reusable_by_address[item].provenance}
                        if reusable_by_address[item].provenance
                        else {}
                    ),
                    **(
                        {
                            "emission_anchor": {
                                "path": reusable_by_address[item].emission_anchor.path,
                                "kind": reusable_by_address[item].emission_anchor.kind,
                                "name": reusable_by_address[item].emission_anchor.name,
                            }
                        }
                        if reusable_by_address[item].emission_anchor is not None
                        else {}
                    ),
                }
                for item in normalized_refs
            ]
        if not isinstance(functions_data, list):
            raise ValueError(
                f"{manifest_path}: expected translation_unit_function_order[{entry_index}].functions as a list"
            )
        if not functions_data and not inventory_only:
            raise ValueError(
                f"{manifest_path}: translation_unit_function_order[{entry_index}].functions must not be empty"
            )
        if "exact_defined_function_set" in entry_data:
            raise ValueError(
                f"{manifest_path}: translation_unit_function_order[{entry_index}]."
                "exact_defined_function_set is retired; unlisted raw definitions are always "
                "diagnostic-only while expected identities and relative order remain gating"
            )
        entry_order_scope = parse_order_scope(
            entry_data,
            context=f"translation_unit_function_order[{entry_index}]",
            manifest_path=manifest_path,
        )
        functions = tuple(
            parse_verify_function(
                item,
                manifest_path=manifest_path,
                seen_addresses=seen_addresses,
                context=f"translation_unit_function_order[{entry_index}].functions",
                allow_listing_label_regex=True,
                allow_source_order_gate=True,
                allow_logical_identity=entry_order_scope == "authored",
            )
            for item in functions_data
        )
        entries.append(
            TranslationUnitFunctionOrderEntry(
                source_from=source_from,
                functions=functions,
                order_scope=entry_order_scope,
                candidate_only_extras=parse_candidate_only_extras(
                    entry_data,
                    context=f"translation_unit_function_order[{entry_index}]",
                    manifest_path=manifest_path,
                ),
                inventory_only=inventory_only,
            )
        )
    return check_translation_unit_function_order, tuple(entries)


NOICF_CONTROLLED_DIAGNOSTIC_KIND = "ref-noicf-controlled-identity-order"
NOICF_CONTROLLED_DIAGNOSTIC_REASON = "noicf_expands_declared_icf_fold_families"
NOICF_CONTROLLED_NONBLOCKING_PREDICATES = frozenset(
    {
        "block_precedence",
        "boundary_sentinels",
        "declared_icf_fold_family_geometry",
    }
)


def parse_linked_order_diagnostic_mode(
    data: dict[str, Any],
    *,
    manifest_path: Path,
) -> LinkedOrderDiagnosticMode:
    raw = data.get("linked_order_diagnostic_mode")
    if raw is None:
        return LinkedOrderDiagnosticMode()
    if not isinstance(raw, dict):
        raise ValueError(f"{manifest_path}: linked_order_diagnostic_mode must be an object")
    allowed = {
        "kind",
        "required_link_profile",
        "nonblocking_reason",
        "nonblocking_predicates",
        "controlled_identities",
        "forbidden_objects",
    }
    unknown = sorted(set(raw) - allowed)
    if unknown:
        raise ValueError(
            f"{manifest_path}: unsupported linked_order_diagnostic_mode keys: "
            + ", ".join(unknown)
        )
    kind = require_string(raw, "kind", manifest_path=manifest_path)
    if kind != NOICF_CONTROLLED_DIAGNOSTIC_KIND:
        raise ValueError(
            f"{manifest_path}: linked_order_diagnostic_mode.kind must be "
            f"{NOICF_CONTROLLED_DIAGNOSTIC_KIND!r}"
        )
    required_link_profile = require_string(
        raw,
        "required_link_profile",
        manifest_path=manifest_path,
    )
    if required_link_profile != "vc5sp3_ref_noicf":
        raise ValueError(
            f"{manifest_path}: {kind} requires link profile 'vc5sp3_ref_noicf'"
        )
    nonblocking_reason = require_string(
        raw,
        "nonblocking_reason",
        manifest_path=manifest_path,
    )
    if nonblocking_reason != NOICF_CONTROLLED_DIAGNOSTIC_REASON:
        raise ValueError(
            f"{manifest_path}: {kind} requires nonblocking_reason "
            f"{NOICF_CONTROLLED_DIAGNOSTIC_REASON!r}"
        )
    predicates = require_string_list(
        raw,
        "nonblocking_predicates",
        manifest_path=manifest_path,
    )
    if set(predicates) != NOICF_CONTROLLED_NONBLOCKING_PREDICATES or len(predicates) != len(
        NOICF_CONTROLLED_NONBLOCKING_PREDICATES
    ):
        raise ValueError(
            f"{manifest_path}: {kind} nonblocking_predicates must name exactly "
            + ", ".join(sorted(NOICF_CONTROLLED_NONBLOCKING_PREDICATES))
        )
    raw_identities = raw.get("controlled_identities")
    if not isinstance(raw_identities, list) or len(raw_identities) < 2:
        raise ValueError(
            f"{manifest_path}: {kind} requires at least two controlled_identities"
        )
    identities: list[LinkedOrderControlledIdentity] = []
    seen_symbols: set[str] = set()
    for index, item in enumerate(raw_identities):
        if not isinstance(item, dict):
            raise ValueError(
                f"{manifest_path}: controlled_identities[{index}] must be an object"
            )
        item_unknown = sorted(set(item) - {"name", "symbol", "expected_object"})
        if item_unknown:
            raise ValueError(
                f"{manifest_path}: unsupported controlled_identities[{index}] keys: "
                + ", ".join(item_unknown)
            )
        symbol = require_string(item, "symbol", manifest_path=manifest_path)
        if symbol in seen_symbols:
            raise ValueError(
                f"{manifest_path}: duplicate controlled identity symbol {symbol!r}"
            )
        seen_symbols.add(symbol)
        identities.append(
            LinkedOrderControlledIdentity(
                name=require_string(item, "name", manifest_path=manifest_path),
                symbol=symbol,
                expected_object=require_string(
                    item,
                    "expected_object",
                    manifest_path=manifest_path,
                ),
            )
        )
    forbidden_objects = require_string_list(
        raw,
        "forbidden_objects",
        manifest_path=manifest_path,
    )
    if not forbidden_objects:
        raise ValueError(f"{manifest_path}: {kind} requires forbidden_objects")
    if len(set(item.casefold() for item in forbidden_objects)) != len(forbidden_objects):
        raise ValueError(f"{manifest_path}: forbidden_objects contains duplicates")
    forbidden_keys = {item.casefold() for item in forbidden_objects}
    if any(identity.expected_object.casefold() in forbidden_keys for identity in identities):
        raise ValueError(
            f"{manifest_path}: a controlled identity expected_object is also forbidden"
        )
    return LinkedOrderDiagnosticMode(
        kind=kind,
        required_link_profile=required_link_profile,
        nonblocking_reason=nonblocking_reason,
        nonblocking_predicates=predicates,
        controlled_identities=tuple(identities),
        forbidden_objects=forbidden_objects,
    )


def parse_linked_function_intervals(
    data: dict[str, Any],
    *,
    manifest_path: Path,
    reusable_functions: tuple[VerifyFunction, ...] = (),
) -> tuple[LinkedFunctionInterval, ...]:
    intervals_data = data.get("linked_function_intervals", [])
    if not isinstance(intervals_data, list):
        raise ValueError(f"{manifest_path}: expected 'linked_function_intervals' as a list")

    intervals: list[LinkedFunctionInterval] = []
    seen_names: set[str] = set()
    for interval_index, interval_data in enumerate(intervals_data):
        context = f"linked_function_intervals[{interval_index}]"
        if not isinstance(interval_data, dict):
            raise ValueError(f"{manifest_path}: {context} must be an object")
        if bool(interval_data.get("retail_start")) != bool(interval_data.get("retail_end_exclusive")):
            raise ValueError(f"{manifest_path}: {context} retail bounds must be specified together")
        interval_retail_start = (
            normalize_address(interval_data["retail_start"])
            if interval_data.get("retail_start")
            else ""
        )
        interval_retail_end = (
            normalize_address(interval_data["retail_end_exclusive"])
            if interval_data.get("retail_end_exclusive")
            else ""
        )
        if interval_retail_start and int(interval_retail_start, 16) >= int(interval_retail_end, 16):
            raise ValueError(f"{manifest_path}: {context} retail range is invalid")
        name = require_string(interval_data, "name", manifest_path=manifest_path)
        if name in seen_names:
            raise ValueError(f"{manifest_path}: duplicate linked function interval name {name!r}")
        seen_names.add(name)
        functions_data = interval_data.get("functions", [])
        function_address_refs = interval_data.get("function_address_refs", [])
        if functions_data and function_address_refs:
            raise ValueError(f"{manifest_path}: {context} cannot combine functions and function_address_refs")
        if function_address_refs:
            if not isinstance(function_address_refs, list) or any(not isinstance(item, str) for item in function_address_refs):
                raise ValueError(f"{manifest_path}: {context}.function_address_refs must be a string list")
            reusable_by_address = {function.address: function for function in reusable_functions}
            normalized_refs = [normalize_address(item) for item in function_address_refs]
            if len(set(normalized_refs)) != len(normalized_refs):
                raise ValueError(f"{manifest_path}: {context}.function_address_refs contains duplicates")
            missing_refs = [item for item in normalized_refs if item not in reusable_by_address]
            if missing_refs:
                raise ValueError(
                    f"{manifest_path}: {context} references unknown top-level function addresses: "
                    + ", ".join(missing_refs)
                )
            functions_data = [
                {
                    "address": reusable_by_address[item].address,
                    **(
                        {"symbol": reusable_by_address[item].symbol}
                        if reusable_by_address[item].symbol
                        else {}
                    ),
                    **(
                        {"symbol_regex": reusable_by_address[item].symbol_regex}
                        if reusable_by_address[item].symbol_regex is not None
                        else {}
                    ),
                    "name": reusable_by_address[item].name,
                    "pipeline_class": reusable_by_address[item].pipeline_class,
                    "authored_order_role": reusable_by_address[item].authored_order_role,
                    "required_presence": reusable_by_address[item].required_presence,
                    "full_order_gate": reusable_by_address[item].full_order_gate,
                    **(
                        {"provenance": reusable_by_address[item].provenance}
                        if reusable_by_address[item].provenance
                        else {}
                    ),
                    **(
                        {
                            "emission_anchor": {
                                "path": reusable_by_address[item].emission_anchor.path,
                                "kind": reusable_by_address[item].emission_anchor.kind,
                                "name": reusable_by_address[item].emission_anchor.name,
                            }
                        }
                        if reusable_by_address[item].emission_anchor is not None
                        else {}
                    ),
                }
                for item in normalized_refs
            ]
        if not isinstance(functions_data, list) or not functions_data:
            raise ValueError(f"{manifest_path}: {context}.functions must be a non-empty list")
        predecessor_data = interval_data.get("predecessor")
        boundary_data = interval_data.get("predecessor_section_boundary")
        successor_data = interval_data.get("successor")
        if isinstance(predecessor_data, dict) == isinstance(boundary_data, dict):
            raise ValueError(
                f"{manifest_path}: {context} requires exactly one of predecessor or "
                "predecessor_section_boundary"
            )
        if not isinstance(successor_data, dict):
            raise ValueError(f"{manifest_path}: {context}.successor must be an object")

        interval_order_scope = parse_order_scope(
            interval_data,
            context=context,
            manifest_path=manifest_path,
        )

        seen_addresses: set[str] = set()
        predecessor = (
            parse_verify_function(
                predecessor_data,
                manifest_path=manifest_path,
                seen_addresses=seen_addresses,
                context=f"{context}.predecessor",
            )
            if isinstance(predecessor_data, dict)
            else None
        )
        boundary = None
        if isinstance(boundary_data, dict):
            boundary_section = require_string(boundary_data, "section", manifest_path=manifest_path)
            boundary_address = normalize_address(
                require_string(boundary_data, "address", manifest_path=manifest_path)
            )
            if boundary_section != ".text":
                raise ValueError(
                    f"{manifest_path}: {context}.predecessor_section_boundary.section "
                    "must be '.text'"
                )
            boundary = LinkedSectionBoundary(boundary_section, boundary_address)
        functions = tuple(
            parse_verify_function(
                item,
                manifest_path=manifest_path,
                seen_addresses=seen_addresses,
                context=f"{context}.functions",
                allow_logical_identity=interval_order_scope == "authored",
            )
            for item in functions_data
        )
        successor = parse_verify_function(
            successor_data,
            manifest_path=manifest_path,
            seen_addresses=seen_addresses,
            context=f"{context}.successor",
        )
        intervals.append(
            LinkedFunctionInterval(
                name=name,
                predecessor=predecessor,
                functions=functions,
                successor=successor,
                predecessor_section_boundary=boundary,
                order_scope=interval_order_scope,
                candidate_only_extras=parse_candidate_only_extras(
                    interval_data,
                    context=context,
                    manifest_path=manifest_path,
                ),
                retail_start=interval_retail_start,
                retail_end_exclusive=interval_retail_end,
            )
        )
    return tuple(intervals)


def optional_profile_guard_string(
    item: dict[str, Any],
    key: str,
    *,
    manifest_path: Path,
    context: str,
) -> str:
    value = item.get(key, "")
    if value is None:
        return ""
    if not isinstance(value, str):
        raise ValueError(f"{manifest_path}: expected {context}.{key} as a string")
    return value


def optional_sentinel_addresses(
    item: dict[str, Any],
    *,
    manifest_path: Path,
    context: str,
) -> tuple[str, ...]:
    raw = item.get("sentinel_addresses", item.get("addresses", []))
    if raw is None:
        return ()
    if not isinstance(raw, list) or not all(isinstance(address, str) and address for address in raw):
        raise ValueError(f"{manifest_path}: expected {context}.sentinel_addresses as a string list")
    return tuple(normalize_address(address) for address in raw)


def parse_profile_guard_entry(
    item: Any,
    *,
    manifest_path: Path,
    context: str,
    known_profiles: set[str],
) -> ProfileGuardEntry:
    if isinstance(item, str):
        profile = item
        sentinel_addresses: tuple[str, ...] = ()
        evidence = ""
        reason = ""
    elif isinstance(item, dict):
        profile = require_string(item, "profile", manifest_path=manifest_path)
        sentinel_addresses = optional_sentinel_addresses(
            item,
            manifest_path=manifest_path,
            context=context,
        )
        evidence = optional_profile_guard_string(
            item,
            "evidence",
            manifest_path=manifest_path,
            context=context,
        )
        reason = optional_profile_guard_string(
            item,
            "reason",
            manifest_path=manifest_path,
            context=context,
        )
    else:
        raise ValueError(f"{manifest_path}: {context} entries must be profile strings or objects")

    if profile not in known_profiles:
        raise ValueError(f"{manifest_path}: unknown profile_guard profile {profile}")
    return ProfileGuardEntry(
        profile=profile,
        sentinel_addresses=sentinel_addresses,
        evidence=evidence,
        reason=reason,
    )


def parse_profile_guard_entries(
    raw_guard: dict[str, Any],
    key: str,
    *,
    manifest_path: Path,
    known_profiles: set[str],
) -> tuple[ProfileGuardEntry, ...]:
    raw_entries = raw_guard.get(key, [])
    if raw_entries is None:
        return ()
    if not isinstance(raw_entries, list):
        raise ValueError(f"{manifest_path}: expected profile_guard.{key} as a list")
    entries = tuple(
        parse_profile_guard_entry(
            item,
            manifest_path=manifest_path,
            context=f"profile_guard.{key}[{index}]",
            known_profiles=known_profiles,
        )
        for index, item in enumerate(raw_entries)
    )
    seen: set[str] = set()
    for entry in entries:
        if entry.profile in seen:
            raise ValueError(f"{manifest_path}: duplicate profile_guard.{key} profile {entry.profile}")
        seen.add(entry.profile)
    return entries


def parse_profile_guard(data: dict[str, Any], *, manifest_path: Path) -> ProfileGuard:
    has_singular = "profile_guard" in data
    has_plural = "profile_guards" in data
    if has_singular and has_plural:
        raise ValueError(f"{manifest_path}: use either profile_guard or profile_guards, not both")
    raw_guard = data.get("profile_guard", data.get("profile_guards"))
    if raw_guard is None:
        return ProfileGuard()
    if not isinstance(raw_guard, dict):
        raise ValueError(f"{manifest_path}: expected profile_guard as an object")

    known_profiles = set(profiles_by_name())
    accepted_profiles = parse_profile_guard_entries(
        raw_guard,
        "accepted_profiles",
        manifest_path=manifest_path,
        known_profiles=known_profiles,
    )
    disqualified_profiles = parse_profile_guard_entries(
        raw_guard,
        "disqualified_profiles",
        manifest_path=manifest_path,
        known_profiles=known_profiles,
    )
    accepted_names = {entry.profile for entry in accepted_profiles}
    disqualified_names = {entry.profile for entry in disqualified_profiles}
    overlap = sorted(accepted_names & disqualified_names)
    if overlap:
        raise ValueError(
            f"{manifest_path}: profile_guard accepted/disqualified overlap: {', '.join(overlap)}"
        )

    return ProfileGuard(
        scope=optional_profile_guard_string(
            raw_guard,
            "scope",
            manifest_path=manifest_path,
            context="profile_guard",
        ),
        policy=optional_profile_guard_string(
            raw_guard,
            "policy",
            manifest_path=manifest_path,
            context="profile_guard",
        ),
        accepted_profiles=accepted_profiles,
        disqualified_profiles=disqualified_profiles,
    )


def validate_translation_unit_source_policy(
    *,
    manifest_path: Path,
    entries: tuple[TranslationUnitFunctionOrderEntry, ...],
    target_binary: str,
    strict_source_emissions: bool = False,
    strict_source_traceability: bool = False,
    policy_closure_paths: tuple[str, ...] = (),
) -> tuple[SourceEmissionWarning, ...]:
    warnings: list[SourceEmissionWarning] = []
    for entry_index, entry in enumerate(entries):
        # When the parent manifest lists multi-file order_edit_paths, provenance
        # may live on any host in that closure (split physical blocks).
        policy_paths: list[str] = [entry.source_from]
        for path in policy_closure_paths:
            if path not in policy_paths:
                policy_paths.append(path)
        production_text = "\n".join(
            source_from_policy_text(path, manifest_path) for path in policy_paths
        )
        canonical_present = "@recoil-" in production_text
        trace_documents = (
            source_trace_documents(tuple(policy_paths), manifest_path=manifest_path)
            if canonical_present or strict_source_traceability
            else ()
        )
        for function in entry.functions:
            trace_function = effective_source_trace_function(
                manifest_path=manifest_path,
                function=function,
                target_binary=target_binary,
            )
            tracker_metadata = canonical_tracker_metadata_for_manifest(
                manifest_path=manifest_path,
                artifact_id=canonical_function_artifact_id(
                    trace_function,
                    target_binary=target_binary,
                ),
            )
            tracker_source_trace_state = (
                tracker_metadata[2] if tracker_metadata is not None else None
            )
            trace_pipeline_class = trace_function.pipeline_class
            exact_artifact_id = canonical_function_artifact_id(
                trace_function,
                target_binary=target_binary,
            )
            exact_source_rows = tuple(
                artifact
                for document in trace_documents
                for artifact in document.artifacts
                if artifact.artifact_id == exact_artifact_id
            )
            if (
                function.provenance == FUNCTION_PROVENANCE_PROVIDER_BOUNDARY
                or trace_pipeline_class == "non-authored"
            ):
                if exact_source_rows:
                    raise ValueError(
                        f"{manifest_path}: translation_unit_function_order"
                        f"[{entry_index}] non-authored/provider function "
                        f"{function.address} is not-applicable for a production source edge"
                    )
                continue
            generated_warning = validate_generated_source_emission_policy(
                function=trace_function,
                source_from=entry.source_from,
                manifest_path=manifest_path,
                context=f"translation_unit_function_order[{entry_index}]",
                strict_source_emissions=strict_source_emissions,
                strict_source_traceability=strict_source_traceability,
                target_binary=target_binary,
                trace_documents=trace_documents,
                tracker_source_trace_state=tracker_source_trace_state,
            )
            if trace_function.authored_order_role in COMPILER_GENERATED_AUTHORED_ORDER_ROLES:
                if generated_warning is not None:
                    warnings.append(generated_warning)
                continue
            compiler_emitted_provenance = (
                function.provenance == FUNCTION_PROVENANCE_COMPILER_EMITTED_NONCOVERING
                and trace_function.authored_order_role
                in COMPILER_GENERATED_AUTHORED_ORDER_ROLES
            )
            if compiler_emitted_provenance:
                canonical = find_canonical_source_artifact(
                    documents=trace_documents,
                    artifact_id=canonical_function_artifact_id(
                        function,
                        target_binary=target_binary,
                    ),
                    relation="emits",
                    expected_section=".text",
                    manifest_path=manifest_path,
                    context=f"translation_unit_function_order[{entry_index}]",
                    allowed_construct_kinds=frozenset({"function", "data", "type"}),
                )
                if canonical is not None:
                    if trace_pipeline_class not in AUTHORED_PIPELINE_CLASSES:
                        raise ValueError(
                            f"{manifest_path}: translation_unit_function_order"
                            f"[{entry_index}] compiler-emitted function "
                            f"{function.address} has pipeline_class "
                            f"{trace_pipeline_class!r} and cannot claim a canonical "
                            "source edge until its authored identity is resolved"
                        )
                    continue
                if (
                    strict_source_traceability
                    and trace_pipeline_class in AUTHORED_PIPELINE_CLASSES
                ):
                    raise ValueError(
                        f"{manifest_path}: translation_unit_function_order[{entry_index}] "
                        f"compiler-emitted function {function.address} requires an "
                        "attached canonical '@recoil-artifact emits .text "
                        f"{canonical_function_artifact_id(function, target_binary=target_binary)}: "
                        "<description>' directive"
                    )
                continue
            if validate_current_authored_icf_physical_source_policy(
                manifest_path=manifest_path,
                function=function,
                target_binary=target_binary,
                documents=trace_documents,
                context=f"translation_unit_function_order[{entry_index}]",
                select_single_logical_member=False,
            ):
                continue
            if (
                function.logical_identity_key
                and function.icf_fold_status == "proven-fold-alias"
                and not strict_source_traceability
            ):
                continue
            canonical = find_canonical_source_artifact(
                documents=trace_documents,
                artifact_id=canonical_function_artifact_id(
                    function,
                    target_binary=target_binary,
                ),
                relation=frozenset({"defines", "emits"}),
                expected_section=".text",
                manifest_path=manifest_path,
                context=f"translation_unit_function_order[{entry_index}]",
                allowed_construct_kinds=frozenset(
                    {"function", "data", "type", "macro"}
                ),
            )
            if canonical is not None:
                if (
                    canonical.relation == "defines"
                    and canonical.construct is not None
                    and canonical.construct.kind != "function"
                ):
                    raise ValueError(
                        f"{manifest_path}: translation_unit_function_order"
                        f"[{entry_index}] direct defines artifact {canonical.artifact_id!r} "
                        "must attach to a function definition"
                    )
                if (
                    canonical.relation == "emits"
                    and canonical.construct is not None
                    and canonical.construct.kind != "macro"
                ):
                    raise ValueError(
                        f"{manifest_path}: translation_unit_function_order"
                        f"[{entry_index}] emits artifact {canonical.artifact_id!r} is "
                        "valid here only on a recognized source-generation macro region"
                    )
                if trace_pipeline_class not in AUTHORED_PIPELINE_CLASSES:
                    raise ValueError(
                        f"{manifest_path}: translation_unit_function_order[{entry_index}] "
                        f"{function.address} has pipeline_class {trace_pipeline_class!r} "
                        "and cannot claim a canonical authored source edge until its "
                        "authored identity is resolved"
                    )
                continue
            if (
                strict_source_traceability
                and trace_pipeline_class in AUTHORED_PIPELINE_CLASSES
            ):
                raise ValueError(
                    f"{manifest_path}: translation_unit_function_order[{entry_index}] "
                    f"{function.address} requires an attached canonical "
                    f"'@recoil-artifact defines .text "
                    f"{canonical_function_artifact_id(function, target_binary=target_binary)}: "
                    "<description>' directive; legacy Reimplements markers are migration "
                    "inventory only"
                )
    return tuple(warnings)


def inferred_target_binary(
    *,
    source_from: str,
    functions: tuple[VerifyFunction, ...],
    data_symbols: tuple[VerifyDataSymbol, ...],
) -> str:
    if source_from.replace("\\", "/").lower().startswith("src/messages/"):
        return "messages"

    addresses = [item.address for item in (*functions, *data_symbols)]
    if not addresses:
        return DEFAULT_TARGET_BINARY

    image_bases = sorted(
        ((key, reference_image(key).image_base) for key in reference_image_keys()),
        key=lambda item: item[1],
        reverse=True,
    )
    inferred: set[str] = set()
    for address in addresses:
        value = int(normalize_address(address), 16)
        for key, image_base in image_bases:
            if value >= image_base:
                inferred.add(key)
                break
    if len(inferred) == 1:
        return next(iter(inferred))
    return DEFAULT_TARGET_BINARY


def manifest_target_binary(
    data: dict[str, Any],
    *,
    manifest_path: Path,
    source_from: str,
    functions: tuple[VerifyFunction, ...],
    data_symbols: tuple[VerifyDataSymbol, ...],
) -> str:
    target_binary = data.get("target_binary")
    if target_binary is None:
        target_binary = inferred_target_binary(
            source_from=source_from,
            functions=functions,
            data_symbols=data_symbols,
        )
    if not isinstance(target_binary, str) or not target_binary:
        raise ValueError(f"{manifest_path}: expected 'target_binary' as a non-empty string")
    if target_binary not in reference_image_keys():
        valid = ", ".join(reference_image_keys())
        raise ValueError(f"{manifest_path}: unknown target_binary {target_binary!r}; expected one of: {valid}")
    return target_binary


def is_defined_code_symbol(coff_object: CoffObject, symbol: Any) -> bool:
    if symbol.section_number <= 0:
        return False
    try:
        section = coff_object.section(symbol.section_number)
    except ValueError:
        return False
    return (section.characteristics & IMAGE_SCN_CNT_CODE) != 0


def format_coff_symbol_match(coff_object: CoffObject, symbol: Any) -> str:
    if symbol.section_number == 0:
        location = "UNDEF"
    elif symbol.section_number < 0:
        location = f"special-section{symbol.section_number}"
    else:
        try:
            section = coff_object.section(symbol.section_number)
        except ValueError:
            location = f"SECT{symbol.section_number}"
        else:
            location = f"{section.name or 'SECT' + str(symbol.section_number)}+0x{symbol.value:x}"
    return f"{symbol.name} ({location})"


def resolve_coff_symbol_regex(
    coff_object: CoffObject,
    pattern_text: str,
    *,
    item_label: str,
    require_defined_code: bool = False,
) -> str:
    pattern = re.compile(pattern_text)
    matches = [
        symbol
        for symbol in coff_object.symbols
        if pattern.fullmatch(symbol.name)
    ]
    resolved_matches = matches
    ignored_matches: list[Any] = []
    if require_defined_code:
        resolved_matches = [
            symbol
            for symbol in matches
            if is_defined_code_symbol(coff_object, symbol)
        ]
        ignored_matches = [
            symbol
            for symbol in matches
            if not is_defined_code_symbol(coff_object, symbol)
        ]
    if len(resolved_matches) != 1:
        if not matches:
            detail = "no matches"
        else:
            label = "defined code matches" if require_defined_code else "matches"
            detail = f"{label}: " + ", ".join(
                format_coff_symbol_match(coff_object, symbol)
                for symbol in resolved_matches[:8]
            )
            if len(resolved_matches) > 8:
                detail += f", ... ({len(resolved_matches)} total)"
            if require_defined_code and ignored_matches:
                detail += "; ignored non-emitted matches: " + ", ".join(
                    format_coff_symbol_match(coff_object, symbol)
                    for symbol in ignored_matches[:8]
                )
                if len(ignored_matches) > 8:
                    detail += f", ... ({len(ignored_matches)} total)"
        raise ValueError(f"symbol_regex for {item_label} did not resolve uniquely: {detail}")
    return resolved_matches[0].name


def resolve_function_symbol_for_coff(coff_object: CoffObject, function: VerifyFunction) -> str:
    if function.symbol_regex is not None:
        return resolve_coff_symbol_regex(
            coff_object,
            function.symbol_regex,
            item_label=function.address,
            require_defined_code=True,
        )
    return function.symbol


def resolve_defined_code_symbol_by_name(
    coff_object: CoffObject,
    symbol_name: str,
    *,
    item_label: str,
) -> Any:
    matches = [
        symbol
        for symbol in getattr(coff_object, "symbols", ())
        if symbol.name == symbol_name
    ]
    if not matches:
        symbol = coff_object.symbols_by_name.get(symbol_name)
        if symbol is None:
            raise ValueError(f"Symbol not found in COFF object: {symbol_name}")
        matches = [symbol]

    defined_code_matches = [
        symbol
        for symbol in matches
        if is_defined_code_symbol(coff_object, symbol)
    ]
    if len(defined_code_matches) == 1:
        return defined_code_matches[0]
    if len(defined_code_matches) > 1:
        detail = ", ".join(
            format_coff_symbol_match(coff_object, symbol)
            for symbol in defined_code_matches[:8]
        )
        if len(defined_code_matches) > 8:
            detail += f", ... ({len(defined_code_matches)} total)"
        raise ValueError(f"Symbol did not resolve to one defined code symbol for {item_label}: {detail}")

    detail = ", ".join(format_coff_symbol_match(coff_object, symbol) for symbol in matches[:8])
    if len(matches) > 8:
        detail += f", ... ({len(matches)} total)"
    raise ValueError(f"Symbol is not in a code section: {symbol_name} ({detail})")


def function_order_gate_suffix(function: VerifyFunction) -> str:
    legacy = " legacy_source_order_gate=false" if not function.source_order_gate else ""
    logical = (
        f" logical_identity_key={function.logical_identity_key} icf_fold_status={function.icf_fold_status}"
        if function.logical_identity_key
        else ""
    )
    return (
        f" [pipeline_class={function.pipeline_class} required_presence={str(function.required_presence).lower()} "
        f"authored_order_role={authored_order_role(function)} "
        f"authored_order_gate={str(function_authored_order_gate(function)).lower()} "
        f"full_order_gate={str(function.full_order_gate).lower()}{logical}{legacy}]"
    )


def parse_cod_listing_label_index(cod_path: Path, coff_object: CoffObject) -> CodListingLabelIndex:
    labels: list[CodListingLabel] = []
    pending_labels: list[str] = []
    current_proc = ""

    for line in cod_path.read_text(encoding="utf-8", errors="ignore").splitlines():
        proc_match = COD_LISTING_PROC_RE.match(line)
        if proc_match:
            current_proc = proc_match.group(1)
            pending_labels.clear()
            continue

        if COD_LISTING_ENDP_RE.match(line):
            current_proc = ""
            pending_labels.clear()
            continue

        label_match = COD_LISTING_LABEL_RE.match(line)
        if label_match:
            pending_labels.append(label_match.group(1))
            continue

        offset_match = COD_LISTING_OFFSET_RE.match(line)
        if not offset_match or not pending_labels:
            continue

        section_number: int | None = None
        if current_proc:
            try:
                symbol = resolve_defined_code_symbol_by_name(
                    coff_object,
                    current_proc,
                    item_label=current_proc,
                )
            except ValueError:
                symbol = None
            if symbol is not None:
                section_number = symbol.section_number

        value = int(offset_match.group(1), 16) if section_number is not None else None
        labels.extend(
            CodListingLabel(
                label=label,
                proc_symbol=current_proc,
                section_number=section_number,
                value=value,
            )
            for label in pending_labels
        )
        pending_labels.clear()

    return CodListingLabelIndex(labels=tuple(labels))


def resolve_cod_listing_label_regex(
    label_index: CodListingLabelIndex,
    pattern_text: str,
    *,
    item_label: str,
) -> CodListingLabel:
    pattern = re.compile(pattern_text)
    matches = [
        label
        for label in label_index.labels
        if pattern.fullmatch(label.label)
    ]
    if len(matches) != 1:
        detail = "no matches" if not matches else "matches: " + ", ".join(
            label.label for label in matches[:8]
        )
        if len(matches) > 8:
            detail += f", ... ({len(matches)} total)"
        raise ValueError(f"listing_label_regex for {item_label} did not resolve uniquely: {detail}")
    label = matches[0]
    if label.section_number is None or label.value is None:
        raise ValueError(
            f"listing_label_regex for {item_label} resolved to {label.label}, "
            "but the COD listing label is not inside a resolvable COFF code PROC"
        )
    return label


def check_function_order(
    *,
    target: VerifyTarget,
    functions: tuple[VerifyFunction, ...],
    coff_object: CoffObject,
) -> FunctionOrderCheck:
    rows: list[FunctionOrderRow] = []
    diagnostics: list[str] = []
    blocking_diagnostics: list[str] = []
    order_scope = getattr(target, "function_order_scope", "full")
    for index, function in enumerate(functions):
        if (
            order_scope == "authored"
            and function_required_in_scope(function, order_scope)
            and function.pipeline_class == "unresolved"
        ):
            diagnostic = (
                f"manifest#{index:02d} {function.address} {function.name}: "
                "required authored-scope row has unresolved pipeline_class"
            )
            diagnostics.append(diagnostic)
            blocking_diagnostics.append(diagnostic)
        try:
            symbol_name = resolve_function_symbol_for_coff(coff_object, function)
            symbol = resolve_defined_code_symbol_by_name(
                coff_object,
                symbol_name,
                item_label=function.address,
            )
        except ValueError as exc:
            diagnostic = (
                f"manifest#{index:02d} {function.address} {function.name}"
                f"{function_order_gate_suffix(function)}: {exc}"
            )
            diagnostics.append(diagnostic)
            if function_required_in_scope(function, order_scope):
                blocking_diagnostics.append(diagnostic)
            continue
        rows.append(
            FunctionOrderRow(
                manifest_index=index,
                function=replace(function, symbol=symbol_name),
                symbol=symbol_name,
                section_number=symbol.section_number,
                value=symbol.value,
                source_from=target.source_from,
            )
        )

    breaks: list[FunctionOrderBreak] = []
    gated_rows = [
        row
        for row in rows
        if (
            function_authored_relative_order_gate(row.function)
            if order_scope == "authored"
            else row.function.full_order_gate
        )
    ]
    for previous, current in zip(gated_rows, gated_rows[1:]):
        if (current.section_number, current.value) <= (previous.section_number, previous.value):
            breaks.append(FunctionOrderBreak(previous=previous, current=current))
    return FunctionOrderCheck(
        target=target,
        rows=tuple(rows),
        breaks=tuple(breaks),
        expected_functions=functions,
        diagnostics=tuple(diagnostics),
        blocking_diagnostics=tuple(blocking_diagnostics),
        order_scope=order_scope,
        required_presence_passed=not blocking_diagnostics,
        authored_relative_order_passed=not breaks if order_scope == "authored" else True,
        full_relative_order_passed=not breaks if order_scope == "full" else False,
    )


def check_translation_unit_function_order(
    *,
    target: VerifyTarget,
    coff_objects: tuple[CoffObject, ...],
    cod_label_indexes: tuple[CodListingLabelIndex, ...] | None = None,
) -> FunctionOrderCheck:
    rows: list[FunctionOrderRow] = []
    diagnostics: list[str] = []
    blocking_diagnostics: list[str] = []
    contributions: list[DefinedFunctionContribution] = []
    # SimpleNamespace-based callers from older integrations predate the explicit
    # scope field.  Treat those as the historical full-order check while parsed
    # schema-v4 manifests always carry an explicit value.
    scopes = {
        getattr(entry, "order_scope", "full")
        for entry in target.translation_unit_function_order
    }
    if len(scopes) != 1:
        raise ValueError("translation_unit_function_order entries must use one common order_scope")
    order_scope = next(iter(scopes), "full")
    manifest_index = 0
    if len(coff_objects) != len(target.translation_unit_function_order):
        raise ValueError(
            "translation-unit COFF object count does not match "
            "translation_unit_function_order entries"
        )
    if cod_label_indexes is not None and len(cod_label_indexes) != len(coff_objects):
        raise ValueError(
            "translation-unit COD listing label index count does not match "
            "translation_unit_function_order entries"
        )
    for object_index, (entry, coff_object) in enumerate(zip(target.translation_unit_function_order, coff_objects)):
        inventory_only = getattr(entry, "inventory_only", False)
        if inventory_only != (not entry.functions):
            raise ValueError(
                "translation_unit_function_order entries with no expected functions must set "
                "inventory_only=true, and inventory-only entries must have no expected functions"
            )
        label_index = cod_label_indexes[object_index] if cod_label_indexes is not None else None
        entry_rows: list[FunctionOrderRow] = []
        for function in entry.functions:
            if (
                order_scope == "authored"
                and function_required_in_scope(function, order_scope)
                and function.pipeline_class == "unresolved"
            ):
                diagnostic = (
                    f"manifest#{manifest_index:02d} obj#{object_index:02d} {function.address} "
                    f"{function.name}: required authored-scope row has unresolved pipeline_class"
                )
                diagnostics.append(diagnostic)
                blocking_diagnostics.append(diagnostic)
            order_source = "coff-symbol"
            try:
                projected_object_symbol = (
                    current_authored_icf_translation_unit_object_symbol(
                        manifest_path=target.manifest_path,
                        function=function,
                        target_binary=target.target_binary,
                        translation_unit=entry.source_from,
                    )
                    if order_scope == "authored"
                    else None
                )
                candidate_function = (
                    replace(
                        function,
                        symbol=projected_object_symbol,
                        symbol_regex=None,
                        listing_label_regex=None,
                    )
                    if projected_object_symbol is not None
                    else function
                )
                if candidate_function.listing_label_regex is not None:
                    if label_index is None:
                        raise ValueError(
                            f"listing_label_regex for {function.address} requires a COD listing label index"
                        )
                    label = resolve_cod_listing_label_regex(
                        label_index,
                        candidate_function.listing_label_regex,
                        item_label=function.address,
                    )
                    symbol_name = label.label
                    section_number = label.section_number
                    value = label.value
                    order_source = f"cod-listing-label in {label.proc_symbol}"
                else:
                    symbol_name = resolve_function_symbol_for_coff(
                        coff_object,
                        candidate_function,
                    )
                    symbol = resolve_defined_code_symbol_by_name(
                        coff_object,
                        symbol_name,
                        item_label=function.address,
                    )
                    section_number = symbol.section_number
                    value = symbol.value
            except ValueError as exc:
                diagnostic = (
                    f"manifest#{manifest_index:02d} obj#{object_index:02d} "
                    f"{function.address} {function.name}"
                    f"{function_order_gate_suffix(function)}: {exc}"
                )
                diagnostics.append(diagnostic)
                if function_required_in_scope(function, order_scope):
                    blocking_diagnostics.append(diagnostic)
                manifest_index += 1
                continue
            row = FunctionOrderRow(
                manifest_index=manifest_index,
                function=replace(candidate_function, symbol=symbol_name),
                symbol=symbol_name,
                section_number=section_number,
                value=value,
                object_index=object_index,
                source_from=entry.source_from,
                order_source=order_source,
            )
            rows.append(row)
            entry_rows.append(row)
            manifest_index += 1

        actual = defined_function_contributions(coff_object, object_index=object_index)
        expected_by_key: dict[tuple[int, int], FunctionOrderRow] = {}
        for row in entry_rows:
            if row.order_source != "coff-symbol":
                continue
            key = (row.section_number, row.value)
            previous = expected_by_key.get(key)
            if previous is not None:
                diagnostic = (
                    f"obj#{object_index:02d} manifest rows {previous.function.address} and "
                    f"{row.function.address} resolve to the same defined-function alias group "
                    f"SECT{key[0]:02X}+0x{key[1]:x}"
                )
                diagnostics.append(diagnostic)
                intentional_logical_alias_group = (
                    previous.function.address == row.function.address
                    and bool(previous.function.logical_identity_key)
                    and bool(row.function.logical_identity_key)
                )
                if (
                    not intentional_logical_alias_group
                    and (
                    function_required_in_scope(previous.function, order_scope)
                    or function_required_in_scope(row.function, order_scope)
                    )
                ):
                    blocking_diagnostics.append(diagnostic)
            else:
                expected_by_key[key] = row

        for item in actual:
            manifest_row = expected_by_key.get((item.section_number, item.value))
            contributions.append(
                replace(
                    item,
                    manifest_index=manifest_row.manifest_index if manifest_row is not None else None,
                    disposition=(
                        "manifest-function" if manifest_row is not None else "unlisted-defined-function"
                    ),
                )
            )

    breaks: list[FunctionOrderBreak] = []
    if order_scope == "authored":
        gated_rows = [
            row for row in rows
            if function_authored_relative_order_gate(row.function)
        ]
    else:
        gated_rows = [row for row in rows if row.function.full_order_gate]
    for previous, current in zip(gated_rows, gated_rows[1:]):
        previous_key = (previous.object_index, previous.section_number, previous.value)
        current_key = (current.object_index, current.section_number, current.value)
        if current_key <= previous_key:
            breaks.append(FunctionOrderBreak(previous=previous, current=current))
    return FunctionOrderCheck(
        target=target,
        rows=tuple(rows),
        breaks=tuple(breaks),
        expected_functions=tuple(
            function
            for entry in target.translation_unit_function_order
            for function in entry.functions
        ),
        diagnostics=tuple(diagnostics),
        blocking_diagnostics=tuple(blocking_diagnostics),
        contributions=tuple(contributions),
        order_scope=order_scope,
        required_presence_passed=not blocking_diagnostics,
        authored_relative_order_passed=not breaks if order_scope == "authored" else True,
        full_relative_order_passed=not breaks if order_scope == "full" else False,
    )


def defined_function_contributions(
    coff_object: CoffObject,
    *,
    object_index: int,
) -> tuple[DefinedFunctionContribution, ...]:
    grouped: dict[tuple[int, int], list[Any]] = {}
    for symbol in coff_object.symbols:
        if symbol.section_number <= 0 or (getattr(symbol, "type", 0) & IMAGE_SYM_DTYPE_FUNCTION) == 0:
            continue
        try:
            section = coff_object.section(symbol.section_number)
        except ValueError:
            continue
        if (section.characteristics & (IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE)) == 0:
            continue
        grouped.setdefault((symbol.section_number, symbol.value), []).append(symbol)

    rows: list[DefinedFunctionContribution] = []
    for (section_number, value), aliases in sorted(grouped.items()):
        section = coff_object.section(section_number)
        aliases.sort(key=lambda symbol: (getattr(symbol, "index", 0), symbol.name))
        rows.append(
            DefinedFunctionContribution(
                object_index=object_index,
                section_number=section_number,
                section_name=getattr(section, "name", f"SECT{section_number}"),
                value=value,
                symbols=tuple(symbol.name for symbol in aliases),
                comdat=bool(section.characteristics & IMAGE_SCN_LNK_COMDAT),
                weak=any(
                    getattr(symbol, "storage_class", 0) == IMAGE_SYM_CLASS_WEAK_EXTERNAL
                    for symbol in aliases
                ),
                manifest_index=None,
            )
        )
    return tuple(rows)


def format_function_order_location(row: FunctionOrderRow, *, include_object: bool) -> str:
    if include_object:
        return f"obj#{row.object_index:02d} SECT{row.section_number:02X}+0x{row.value:x}"
    return f"SECT{row.section_number:02X}+0x{row.value:x}"


def _function_order_row_summary(
    row: FunctionOrderRow,
    *,
    include_object: bool,
) -> dict[str, Any]:
    return {
        "manifest_index": row.manifest_index,
        "address": row.function.address,
        "name": row.function.name,
        "symbol": row.symbol,
        "location": format_function_order_location(row, include_object=include_object),
        "source_from": row.source_from or None,
    }


def _function_order_neighbor_window(
    rows: list[FunctionOrderRow],
    index: int,
    *,
    include_object: bool,
) -> list[dict[str, Any]]:
    start = max(0, index - 1)
    end = min(len(rows), index + 2)
    return [
        _function_order_row_summary(row, include_object=include_object)
        for row in rows[start:end]
    ]


def function_order_first_divergence(order_check: FunctionOrderCheck) -> dict[str, Any] | None:
    """Return the first source-worker-actionable identity/order failure."""
    include_object = bool(order_check.target.check_translation_unit_function_order) or any(
        row.object_index for row in order_check.rows
    )
    gated_rows = [
        row
        for row in order_check.rows
        if (
            function_authored_relative_order_gate(row.function)
            if order_check.order_scope == "authored"
            else row.function.full_order_gate
        )
    ]
    expected_rows = sorted(gated_rows, key=lambda row: row.manifest_index)
    actual_rows = sorted(
        gated_rows,
        key=lambda row: (
            row.object_index,
            row.section_number,
            row.value,
            row.manifest_index,
        ),
    )

    if order_check.blocking_diagnostics:
        diagnostic = order_check.blocking_diagnostics[0]
        match = re.search(r"manifest#([0-9]+)", diagnostic)
        manifest_index = int(match.group(1)) if match is not None else None
        actual_index = 0
        if manifest_index is not None:
            actual_index = next(
                (
                    index
                    for index, row in enumerate(actual_rows)
                    if row.manifest_index > manifest_index
                ),
                max(0, len(actual_rows) - 1),
            )
        return {
            "kind": "identity-resolution",
            "message": diagnostic,
            "manifest_index": manifest_index,
            "expected_neighbors": [
                _function_order_row_summary(row, include_object=include_object)
                for row in expected_rows
                if manifest_index is not None and abs(row.manifest_index - manifest_index) <= 1
            ],
            "actual_neighbors": (
                _function_order_neighbor_window(
                    actual_rows,
                    actual_index,
                    include_object=include_object,
                )
                if actual_rows
                else []
            ),
        }

    for index, (expected, actual) in enumerate(zip(expected_rows, actual_rows)):
        if expected.manifest_index == actual.manifest_index:
            continue
        return {
            "kind": "reordered",
            "message": (
                f"expected manifest#{expected.manifest_index:02d} {expected.function.address} "
                f"{expected.function.name}, but the candidate emits manifest#"
                f"{actual.manifest_index:02d} {actual.function.address} {actual.function.name} "
                "at that relative position"
            ),
            "expected": _function_order_row_summary(
                expected,
                include_object=include_object,
            ),
            "actual": _function_order_row_summary(
                actual,
                include_object=include_object,
            ),
            "expected_neighbors": _function_order_neighbor_window(
                expected_rows,
                index,
                include_object=include_object,
            ),
            "actual_neighbors": _function_order_neighbor_window(
                actual_rows,
                index,
                include_object=include_object,
            ),
        }

    if order_check.breaks:
        first_break = order_check.breaks[0]
        previous = first_break.previous
        current = first_break.current
        return {
            "kind": "same-or-reversed-location",
            "message": (
                f"manifest#{current.manifest_index:02d} {current.function.address} "
                f"{current.function.name} is at {format_function_order_location(current, include_object=include_object)}, "
                f"which is not after manifest#{previous.manifest_index:02d} "
                f"{previous.function.address} {previous.function.name} at "
                f"{format_function_order_location(previous, include_object=include_object)}"
            ),
            "previous": _function_order_row_summary(
                previous,
                include_object=include_object,
            ),
            "current": _function_order_row_summary(
                current,
                include_object=include_object,
            ),
            "expected_neighbors": [
                _function_order_row_summary(row, include_object=include_object)
                for row in (previous, current)
            ],
            "actual_neighbors": [
                _function_order_row_summary(row, include_object=include_object)
                for row in sorted(
                    (previous, current),
                    key=lambda row: (
                        row.object_index,
                        row.section_number,
                        row.value,
                        row.manifest_index,
                    ),
                )
            ],
        }
    return None


def _format_function_order_neighbor(row: Mapping[str, Any]) -> str:
    return (
        f"manifest#{int(row['manifest_index']):02d} {row['address']} {row['name']} "
        f"@ {row['location']}"
    )


def print_function_order_check(
    order_check: FunctionOrderCheck,
    *,
    translation_unit: bool = False,
    summary_only: bool = False,
) -> None:
    print("Translation-unit function order check:" if translation_unit else "Function order check:")
    print(f"- target: {order_check.target.name}")
    if translation_unit:
        print("- mode: diagnostic/provenance only; not byte evidence")
    print(f"- order scope: {order_check.order_scope}")
    print(f"- result: {'PASS' if order_check.ok else 'FAIL'}")
    print(f"- resolved manifest identities: {len(order_check.rows)}")
    first_divergence = function_order_first_divergence(order_check)
    if first_divergence is None:
        print("- first blocking divergence: none")
    else:
        print(
            f"- first blocking divergence [{first_divergence['kind']}]: "
            f"{first_divergence['message']}"
        )
        expected_neighbors = first_divergence.get("expected_neighbors", [])
        actual_neighbors = first_divergence.get("actual_neighbors", [])
        if expected_neighbors:
            print(
                "- expected neighbors: "
                + " -> ".join(_format_function_order_neighbor(row) for row in expected_neighbors)
            )
        if actual_neighbors:
            print(
                "- candidate neighbors: "
                + " -> ".join(_format_function_order_neighbor(row) for row in actual_neighbors)
            )
    if summary_only:
        return
    if order_check.diagnostics:
        print("- unresolved/ambiguous order entries:")
        for item in order_check.diagnostics:
            print(f"  {item}")
    if translation_unit and order_check.contributions:
        print("- complete raw defined-function inventory:")
        for item in order_check.contributions:
            manifest = "unlisted" if item.manifest_index is None else f"manifest#{item.manifest_index:02d}"
            aliases = " | ".join(item.symbols)
            linked_rva = "none" if item.linked_rva is None else f"0x{item.linked_rva:x}"
            provider = item.selected_provider or "none"
            print(
                f"  obj#{item.object_index:02d} SECT{item.section_number:02X}+0x{item.value:x} "
                f"section={item.section_name} {manifest} aliases=[{aliases}] "
                f"comdat={str(item.comdat).lower()} weak={str(item.weak).lower()} "
                f"selected_provider={provider} linked_rva={linked_rva} "
                f"disposition={item.disposition}"
            )
    if not order_check.rows:
        print("- no resolved functions selected")
        return
    print("- manifest retail order:")
    include_object = translation_unit or any(row.object_index for row in order_check.rows)
    for row in order_check.rows:
        source = f" {row.source_from}" if translation_unit and row.source_from else ""
        print(
            f"  manifest#{row.manifest_index:02d} "
            f"{row.function.address} {row.function.name} "
            f"{format_function_order_location(row, include_object=include_object)}"
            f"{source} "
            f"{row.symbol}"
            f"{'' if row.order_source == 'coff-symbol' else ' [' + row.order_source + ']'}"
            f"{function_order_gate_suffix(row.function)}"
        )
    if order_check.breaks:
        print("- order breaks:")
        for item in order_check.breaks:
            print(
                f"  manifest#{item.previous.manifest_index:02d} "
                f"{item.previous.function.address} {item.previous.function.name} "
                f"{format_function_order_location(item.previous, include_object=include_object)} "
                f"before manifest#{item.current.manifest_index:02d} "
                f"{item.current.function.address} {item.current.function.name} "
                f"{format_function_order_location(item.current, include_object=include_object)}"
            )


def run_function_order_checks(
    *,
    compiled: CompiledTarget,
    selections: list[VerifySelection],
    summary_only: bool = False,
) -> int:
    order_selections = [
        selection
        for selection in selections
        if selection.target.check_function_order and selection.functions
    ]
    if not order_selections:
        return 0

    try:
        coff_object = CoffObject.from_path(compiled.obj_path)
    except ValueError as exc:
        print(f"Function order check failed: {exc}", file=sys.stderr)
        return 1

    overall = 0
    for selection in order_selections:
        try:
            order_check = check_function_order(
                target=selection.target,
                functions=selection.functions,
                coff_object=coff_object,
            )
        except ValueError as exc:
            print(f"Function order check failed for {selection.target.name}: {exc}", file=sys.stderr)
            overall = 1
            continue
        print_function_order_check(order_check, summary_only=summary_only)
        report_path = compiled.build_dir / f"{selection.target.name}.function_order.json"
        report_path.write_text(
            json.dumps(translation_unit_order_report_data(order_check), indent=2) + "\n",
            encoding="utf-8",
        )
        print(f"- function order report: {report_path}")
        if not order_check.ok:
            overall = 1
    return overall


def run_translation_unit_function_order_checks(
    *,
    compiled: CompiledTarget,
    summary_only: bool = False,
) -> int:
    target = compiled.target
    if not target.check_translation_unit_function_order:
        return 0

    compiled_units, rc = compile_translation_unit_order(
        target=target,
        build_dir=compiled.build_dir,
        compiler_env=compiled.compiler_env,
    )
    if rc != 0:
        return rc

    coff_objects: list[CoffObject] = []
    cod_label_indexes: list[CodListingLabelIndex] = []
    for unit in compiled_units:
        try:
            coff_object = CoffObject.from_path(unit.obj_path)
            coff_objects.append(coff_object)
            cod_label_indexes.append(parse_cod_listing_label_index(unit.cod_path, coff_object))
        except ValueError as exc:
            print(
                f"Translation-unit function order check failed for "
                f"{target.name} {unit.source_from}: {exc}",
                file=sys.stderr,
            )
            return 1

    try:
        order_check = check_translation_unit_function_order(
            target=target,
            coff_objects=tuple(coff_objects),
            cod_label_indexes=tuple(cod_label_indexes),
        )
    except ValueError as exc:
        print(f"Translation-unit function order check failed for {target.name}: {exc}", file=sys.stderr)
        return 1
    print_function_order_check(
        order_check,
        translation_unit=True,
        summary_only=summary_only,
    )
    report_path = compiled.build_dir / f"{target.name}.translation_unit_function_order.json"
    report_path.write_text(
        json.dumps(translation_unit_order_report_data(order_check), indent=2) + "\n",
        encoding="utf-8",
    )
    print(f"- translation-unit order report: {report_path}")
    return 0 if order_check.ok else 1


def translation_unit_order_report_data(order_check: FunctionOrderCheck) -> dict[str, Any]:
    resolved_by_index = {row.manifest_index: row for row in order_check.rows}
    expected: list[dict[str, Any]] = []
    for manifest_index, function in enumerate(order_check.expected_functions):
        resolved = resolved_by_index.get(manifest_index)
        expected.append({
            "address": function.address,
            "selector": (
                {"symbol_regex": function.symbol_regex}
                if function.symbol_regex is not None
                else {"listing_label_regex": function.listing_label_regex}
                if function.listing_label_regex is not None
                else {"symbol": function.symbol}
            ),
            "symbol": resolved.symbol if resolved is not None else function.symbol,
            "resolved": resolved is not None,
            "name": function.name,
            "provenance": function.provenance or "authored",
            "pipeline_class": function.pipeline_class,
            "authored_order_role": authored_order_role(function),
            "authored_order_gate": function_authored_order_gate(function),
            "required_presence": function.required_presence,
            "full_order_gate": function.full_order_gate,
            "logical_identity_key": function.logical_identity_key or None,
            "icf_fold_status": function.icf_fold_status or None,
            "authored_relative_order_gate": function_authored_relative_order_gate(function),
        })
    actual = (
        [list(item.symbols) for item in order_check.contributions]
        if order_check.contributions
        else [[row.symbol] for row in order_check.rows]
    )
    expected_identities = [item["symbol"] for item in expected]
    actual_identities = [symbols[0] if symbols else "" for symbols in actual]
    missing_all = [
        item["symbol"] or item["selector"]
        for item in expected
        if not item["resolved"] or not any(item["symbol"] in group for group in actual)
    ]
    missing = [
        item["symbol"] or item["selector"]
        for item in expected
        if item["required_presence"]
        and (order_check.order_scope == "full" or item["authored_order_gate"])
        and (not item["resolved"] or not any(item["symbol"] in group for group in actual))
    ]
    extras = [group[0] for group in actual if group and not any(symbol in expected_identities for symbol in group)]
    duplicate = set(
        symbol
        for symbol in set(actual_identities)
        if symbol and actual_identities.count(symbol) > 1
    )
    expected_locations: dict[tuple[int, int, int], list[FunctionOrderRow]] = {}
    for row in order_check.rows:
        expected_locations.setdefault((row.object_index, row.section_number, row.value), []).append(row)
    for location_rows in expected_locations.values():
        if len(location_rows) > 1:
            intentional_logical_alias_group = all(
                row.function.logical_identity_key
                and row.function.address == location_rows[0].function.address
                for row in location_rows
            )
            if not intentional_logical_alias_group:
                duplicate.update(row.symbol for row in location_rows)
    raw_exact_match = not missing_all and not extras and not duplicate and len(actual) == len(expected)
    identity_passed = order_check.required_presence_passed
    authored_relative_order_passed = order_check.authored_relative_order_passed
    full_relative_order_passed = order_check.full_relative_order_passed
    if order_check.contributions:
        contribution_rows = [
            {
                "raw_object_position": {
                    "object_index": item.object_index,
                    "section_number": item.section_number,
                    "section_name": item.section_name,
                    "value": item.value,
                },
                "decorated_identities": list(item.symbols),
                "alias_group": list(item.symbols),
                "comdat": item.comdat,
                "weak": item.weak,
                "manifest_index": item.manifest_index,
                "selected_provider": item.selected_provider or None,
                "linked_rva": item.linked_rva,
                "disposition": item.disposition,
            }
            for item in order_check.contributions
        ]
    else:
        contribution_rows = [
            {
                "raw_object_position": {
                    "object_index": row.object_index,
                    "section_number": row.section_number,
                    "section_name": "",
                    "value": row.value,
                },
                "decorated_identities": [row.symbol],
                "alias_group": [row.symbol],
                "comdat": None,
                "weak": None,
                "manifest_index": row.manifest_index,
                "selected_provider": None,
                "linked_rva": None,
                "disposition": "expected-resolved",
            }
            for row in order_check.rows
        ]
    return {
        "report_version": 1,
        "kind": "vc5-function-order-report",
        "target": order_check.target.name,
        "binary": order_check.target.target_binary,
        "retail_start": order_check.target.retail_start,
        "retail_end_exclusive": order_check.target.retail_end_exclusive,
        "order_scope": order_check.order_scope,
        "ok": order_check.ok,
        "passed": order_check.ok,
        "order_gate_policy": "required-presence-plus-scope-relative-order",
        "unlisted_raw_contributions_blocking": False,
        "required_presence_passed": identity_passed,
        # Compatibility aliases retained for callers that consume the semantic report.
        "expected_identity_resolution_passed": identity_passed,
        "authored_relative_order_passed": authored_relative_order_passed,
        "full_relative_order_passed": full_relative_order_passed,
        "expected_relative_order_passed": (
            authored_relative_order_passed
            if order_check.order_scope == "authored"
            else full_relative_order_passed
        ),
        "exact_full_order_claimed": order_check.order_scope == "full",
        "function_order_passed": order_check.ok,
        "first_divergence": function_order_first_divergence(order_check),
        "expected_contributions": expected,
        "expected_count": len(expected),
        "actual_count": len(actual),
        "missing_contributions": missing,
        "optional_missing_contributions": [item for item in missing_all if item not in missing],
        "extra_contributions": extras,
        "duplicate_contributions": sorted(duplicate),
        "raw_defined_function_set_matches": raw_exact_match,
        "reorder_breaks": [
            {
                "previous": item.previous.function.address,
                "current": item.current.function.address,
            }
            for item in order_check.breaks
        ],
        "diagnostics": list(order_check.diagnostics),
        "blocking_diagnostics": list(order_check.blocking_diagnostics),
        "actual_contributions": contribution_rows,
        "contributions": contribution_rows,
    }


SOURCE_COMPILE_PROFILE_GLOB_CHARS = "*?[]{}"


def canonical_source_key(source: str | Path) -> str:
    path_text = source if isinstance(source, str) else source.as_posix()
    if Path(path_text).is_absolute():
        return "fixture-physical:" + os.path.normcase(path_text).replace("\\", "/")
    return validate_repository_relative_path(
        path_text,
        context="source compile profile path",
    ).casefold()


def compile_source_identity(source: str | Path) -> str:
    path_text = source if isinstance(source, str) else source.as_posix()
    if Path(path_text).is_absolute():
        return "fixture-physical:" + os.path.normcase(path_text).replace("\\", "/")
    return validate_repository_relative_path(
        path_text,
        context="compile source identity",
    ).casefold()


@lru_cache(maxsize=1)
def final_build_profile_rows() -> dict[str, dict[str, Any]]:
    data = json.loads(DEFAULT_PROFILES.read_text(encoding="utf-8"))
    rows = data.get("verification_profiles", [])
    if not isinstance(rows, list):
        raise ValueError(f"{DEFAULT_PROFILES}: verification_profiles must be a list")
    return {
        str(row.get("name")): row
        for row in rows
        if isinstance(row, dict) and isinstance(row.get("name"), str)
    }


def reviewed_final_build_profile_flags(profile_name: str) -> tuple[str, ...]:
    row = final_build_profile_rows().get(profile_name)
    if row is None:
        raise ValueError(f"{DEFAULT_PROFILES}: unknown verification profile {profile_name}")
    flags = row.get("final_build_compile_flags")
    if not isinstance(flags, list) or not flags or not all(isinstance(item, str) and item for item in flags):
        raise ValueError(
            f"{DEFAULT_PROFILES}: profile {profile_name} has no reviewed "
            "final_build_compile_flags"
        )
    normalized = tuple(flags)
    reject_raw_topology_flags(
        normalized,
        label=f"{DEFAULT_PROFILES}: {profile_name}.final_build_compile_flags",
    )
    if any(flag.upper() == "/FACS" or flag.upper() == "/I" or flag.upper().startswith("/I.") for flag in normalized):
        raise ValueError(
            f"{DEFAULT_PROFILES}: profile {profile_name} contains verifier-only "
            "listing/include flags in final_build_compile_flags"
        )
    return normalized


def parse_source_compile_profiles(
    raw_profiles: object,
    *,
    configured_sources: tuple[str, ...],
    manifest_path: Path,
    compiler_env: str,
    profile_guard: ProfileGuard,
    final_build_context: bool,
    compile_defines: tuple[str, ...] = (),
) -> tuple[tuple[tuple[str, str], ...], tuple[tuple[str, tuple[str, ...]], ...]]:
    if not isinstance(raw_profiles, dict):
        raise ValueError(f"{manifest_path}: source_compile_profiles must be an object")
    if not raw_profiles:
        return (), ()

    configured_keys = {compile_source_identity(source) for source in configured_sources}
    available = profiles_by_name()
    mappings: dict[str, str] = {}
    flags_by_source: dict[str, tuple[str, ...]] = {}
    expected_env = repo_path(compiler_env).resolve() if compiler_env else None
    guarded_source_key: str | None = None
    if profile_guard.scope:
        try:
            candidate_guarded_source_key = canonical_source_key(profile_guard.scope)
        except ValueError:
            candidate_guarded_source_key = ""
        if candidate_guarded_source_key in configured_keys:
            guarded_source_key = candidate_guarded_source_key
    for source_text, profile_name in raw_profiles.items():
        if (
            not isinstance(source_text, str)
            or not source_text
            or not isinstance(profile_name, str)
            or not profile_name
        ):
            raise ValueError(
                f"{manifest_path}: source_compile_profiles requires non-empty SOURCE: PROFILE strings"
            )
        if any(char in source_text for char in SOURCE_COMPILE_PROFILE_GLOB_CHARS):
            raise ValueError(
                f"{manifest_path}: source_compile_profiles requires exact source paths"
            )
        source_key = canonical_source_key(source_text)
        if source_key not in configured_keys:
            raise ValueError(
                f"{manifest_path}: source_compile_profiles references an unconfigured source: "
                f"{source_text}"
            )
        if source_key in mappings:
            raise ValueError(
                f"{manifest_path}: duplicate normalized source_compile_profiles path: {source_text}"
            )
        profile = available.get(profile_name)
        if profile is None:
            raise ValueError(
                f"{manifest_path}: unknown source_compile_profiles profile {profile_name}"
            )
        # An exact source scope constrains only that source's per-TU mapping.
        # Empty, malformed, and non-source scopes retain the previous fail-closed
        # target-wide behavior rather than silently weakening the guard.
        if guarded_source_key is None or source_key == guarded_source_key:
            disqualification = profile_guard_disqualification(profile_guard, profile_name)
            if disqualification is not None:
                raise ValueError(
                    f"{manifest_path}: source_compile_profiles profile {profile_name} is "
                    "disqualified by profile_guard"
                )
            if not profile_guard_allows_profile(profile_guard, profile_name):
                raise ValueError(
                    f"{manifest_path}: source_compile_profiles profile {profile_name} is not listed "
                    "in profile_guard.accepted_profiles"
                )
        profile_env = repo_path(profile.compiler_env).resolve()
        if expected_env is not None and profile_env != expected_env:
            raise ValueError(
                f"{manifest_path}: compiler environment mismatch for source profile {profile_name}"
            )
        flags = (
            (*reviewed_final_build_profile_flags(profile_name), *(f"/D{item}" for item in compile_defines), "/FAcs")
            if final_build_context
            else profile.compiler_flags
        )
        reject_raw_topology_flags(
            flags,
            label=f"{manifest_path}: source_compile_profiles[{source_text}]",
        )
        mappings[source_key] = profile_name
        flags_by_source[source_key] = tuple(flags)
    return tuple(sorted(mappings.items())), tuple(sorted(flags_by_source.items()))


def effective_source_compile_context(
    target: VerifyTarget,
    source_from: str = "",
) -> tuple[str, tuple[str, ...]]:
    logical_source = source_from or target.source_from
    if logical_source and target.source_compile_profiles:
        source_key = canonical_source_key(logical_source)
        profile_name = dict(target.source_compile_profiles).get(source_key)
        flags = dict(target.source_compile_flags).get(source_key)
        if profile_name is not None and flags is not None:
            return profile_name, flags
    return target.compiler_profile, target.compiler_flags


def load_manifest(
    path: Path,
    *,
    enforce_source_policy: bool = True,
    strict_source_emissions: bool = False,
    strict_source_traceability: bool = False,
    tracked_path_inventory: GitTrackedPathInventory | None = None,
    strict_tracked_paths: bool | None = None,
) -> VerifyTarget:
    operation_inventory = tracked_path_inventory
    authenticate_current_paths = (
        tracked_path_inventory is not None
        if strict_tracked_paths is None
        else strict_tracked_paths
    )
    try:
        with path.open("r", encoding="utf-8") as handle:
            data = json.load(handle)
    except json.JSONDecodeError as exc:
        raise ValueError(
            f"{path}: invalid JSON: {exc.msg}: line {exc.lineno} column {exc.colno}"
        ) from exc
    if not isinstance(data, dict):
        raise ValueError(f"{path}: manifest root must be an object")
    if operation_inventory is None and (
        authenticate_current_paths or data.get("order_edit_paths") is not None
    ):
        operation_inventory = load_git_tracked_path_inventory(REPO_ROOT)
    if bool(data.get("retail_start")) != bool(data.get("retail_end_exclusive")):
        raise ValueError(f"{path}: retail_start and retail_end_exclusive must be specified together")
    if data.get("retail_start") and int(normalize_address(data["retail_start"]), 16) >= int(
        normalize_address(data["retail_end_exclusive"]), 16
    ):
        raise ValueError(f"{path}: retail block range is invalid")

    source_from = data.get("source_from", "")
    if source_from and not isinstance(source_from, str):
        raise ValueError(f"{path}: expected 'source_from' as a string")
    if source_from and authenticate_current_paths:
        if operation_inventory is None:
            raise ValueError(f"{path}: tracked inventory is required")
        source_from = _exact_tracked_manifest_path(
            source_from,
            inventory=operation_inventory,
            context=f"{path}: source_from",
            allowed_suffixes=ORDER_EDIT_PATH_SUFFIXES,
        )
    source_text = ""
    order_edit_paths = normalize_order_edit_paths(
        data.get("order_edit_paths"),
        context=str(path),
        inventory=operation_inventory,
    )

    generated_files_data = data.get("generated_files", {})
    generated_files: list[tuple[str, str]] = []
    if not isinstance(generated_files_data, dict):
        raise ValueError(f"{path}: expected 'generated_files' as an object")
    for generated_path, generated_source in sorted(generated_files_data.items()):
        if not isinstance(generated_path, str):
            raise ValueError(f"{path}: generated file paths must be strings")
        if isinstance(generated_source, list) and all(isinstance(line, str) for line in generated_source):
            generated_files.append((generated_path, "\n".join(generated_source) + "\n"))
        elif isinstance(generated_source, str):
            generated_files.append((generated_path, generated_source))
        else:
            raise ValueError(f"{path}: generated file '{generated_path}' must be a string or string list")

    functions: list[VerifyFunction] = []
    functions_data = data.get("functions", [])
    if not isinstance(functions_data, list):
        raise ValueError(f"{path}: expected 'functions' as a list")
    seen_addresses: set[str] = set()
    function_defaults = data.get("function_defaults", {})
    if not isinstance(function_defaults, dict):
        raise ValueError(f"{path}: function_defaults must be an object")
    unknown_function_defaults = sorted(set(function_defaults) - {"pipeline_class"})
    if unknown_function_defaults:
        raise ValueError(
            f"{path}: unsupported function_defaults keys: {', '.join(unknown_function_defaults)}"
        )
    default_pipeline_class = function_defaults.get("pipeline_class", "unresolved")
    if default_pipeline_class not in PIPELINE_CLASS_VALUES:
        raise ValueError(
            f"{path}: function_defaults.pipeline_class must be one of {sorted(PIPELINE_CLASS_VALUES)}"
        )
    for item in functions_data:
        functions.append(
            parse_verify_function(
                item,
                manifest_path=path,
                seen_addresses=seen_addresses,
                context="function",
                default_pipeline_class=default_pipeline_class,
            )
        )

    (
        check_translation_unit_function_order,
        translation_unit_function_order,
    ) = parse_translation_unit_function_order(
        data,
        manifest_path=path,
        reusable_functions=tuple(functions),
        tracked_path_inventory=operation_inventory,
        strict_tracked_paths=authenticate_current_paths,
    )
    translation_scopes = {entry.order_scope for entry in translation_unit_function_order}
    inherited_function_order_scope = (
        next(iter(translation_scopes))
        if len(translation_scopes) == 1
        else "full"
    )
    function_order_scope = parse_order_scope(
        {"order_scope": data.get("function_order_scope", inherited_function_order_scope)},
        context="function_order_scope",
        manifest_path=path,
    )
    if optional_bool(data, "check_function_order", False, manifest_path=path) and translation_unit_function_order:
        if len(translation_scopes) == 1 and function_order_scope != next(iter(translation_scopes)):
            raise ValueError(
                f"{path}: function_order_scope must match translation_unit_function_order "
                "order_scope when both checks are enabled"
            )
    linked_function_intervals = parse_linked_function_intervals(
        data,
        manifest_path=path,
        reusable_functions=tuple(functions),
    )
    linked_order_base_target = data.get("linked_order_base_target", "")
    if not isinstance(linked_order_base_target, str):
        raise ValueError(f"{path}: linked_order_base_target must be a string")
    linked_order_diagnostic_mode = parse_linked_order_diagnostic_mode(
        data,
        manifest_path=path,
    )
    if bool(linked_order_base_target) != bool(linked_order_diagnostic_mode.kind):
        raise ValueError(
            f"{path}: linked_order_base_target and linked_order_diagnostic_mode "
            "must be specified together"
        )
    profile_guard = parse_profile_guard(data, manifest_path=path)
    if authenticate_current_paths and profile_guard.scope:
        scope_matches = operation_inventory.casefolded_paths.get(
            profile_guard.scope.casefold(),
            (),
        )
        if scope_matches:
            exact_scope = _exact_tracked_manifest_path(
                profile_guard.scope,
                inventory=operation_inventory,
                context=f"{path}: profile_guard.scope",
                allowed_suffixes=ORDER_EDIT_PATH_SUFFIXES,
            )
            profile_guard = replace(profile_guard, scope=exact_scope)

    data_symbols: list[VerifyDataSymbol] = []
    data_symbols_data = data.get("data_symbols", [])
    if not isinstance(data_symbols_data, list):
        raise ValueError(f"{path}: expected 'data_symbols' as a list")
    for item in data_symbols_data:
        if not isinstance(item, dict):
            raise ValueError(f"{path}: data symbol entries must be objects")
        address = normalize_address(require_string(item, "address", manifest_path=path))
        if address in seen_addresses:
            raise ValueError(f"{path}: duplicate verification address {address}")
        seen_addresses.add(address)
        symbol = item.get("symbol", "")
        if not isinstance(symbol, str):
            raise ValueError(f"{path}: expected data symbol as a string for {address}")
        symbol_regex = optional_symbol_regex(item, address, manifest_path=path)
        if not symbol and symbol_regex is None:
            raise ValueError(f"{path}: expected data symbol or symbol_regex for {address}")
        name = item.get("name", symbol)
        if not isinstance(name, str) or not name:
            raise ValueError(f"{path}: expected non-empty data symbol name for {address}")
        bn_name = item.get("bn_name", "")
        if bn_name and not isinstance(bn_name, str):
            raise ValueError(f"{path}: expected data symbol bn_name as a string for {address}")
        logical_identity_key = item.get("logical_identity_key", "")
        if not isinstance(logical_identity_key, str):
            raise ValueError(
                f"{path}: logical_identity_key must be a string for data {address}"
            )
        if logical_identity_key:
            logical_identity_key = normalize_artifact_id(logical_identity_key)
            if (
                source_traceability_lib.LOGICAL_DATA_RE.fullmatch(logical_identity_key)
                is None
                or artifact_address(logical_identity_key) != address
            ):
                raise ValueError(
                    f"{path}: data logical_identity_key must be an exact logical-data "
                    f"artifact at {address}"
                )
        data_symbols.append(
            VerifyDataSymbol(
                address=address,
                symbol=symbol,
                name=name,
                byte_length=optional_positive_int(item, "byte_length", manifest_path=path) or 0,
                object_offset=optional_nonnegative_int(
                    item,
                    "object_offset",
                    manifest_path=path,
                )
                or 0,
                bn_name=bn_name or "",
                symbol_regex=symbol_regex,
                logical_identity_key=logical_identity_key,
            )
        )
        if data_symbols[-1].byte_length <= 0:
            raise ValueError(f"{path}: expected 'byte_length' to be a positive integer")

    if linked_order_base_target and (
        functions
        or data_symbols
        or translation_unit_function_order
        or linked_function_intervals
    ):
        raise ValueError(
            f"{path}: linked-order diagnostic overlay must inherit its functions and intervals "
            "from linked_order_base_target"
        )
    if (
        not functions
        and not data_symbols
        and not translation_unit_function_order
        and not linked_function_intervals
        and not linked_order_base_target
    ):
        raise ValueError(
            f"{path}: expected at least one function, data_symbols, "
            "translation_unit_function_order, or linked_function_intervals entry"
        )
    has_compile_work = bool(functions or data_symbols or translation_unit_function_order)

    compare_mode = data.get("compare_mode", "coff_bytes")
    if not isinstance(compare_mode, str) or compare_mode != "coff_bytes":
        raise ValueError(f"{path}: expected 'compare_mode' to be 'coff_bytes'")
    compiler_profile = data.get("compiler_profile", "")
    if compiler_profile and not isinstance(compiler_profile, str):
        raise ValueError(f"{path}: expected 'compiler_profile' as a string")
    compile_context_from = data.get("compile_context_from", "")
    if compile_context_from and not isinstance(compile_context_from, str):
        raise ValueError(f"{path}: compile_context_from must be a string")
    if compile_context_from and authenticate_current_paths:
        compile_context_from = _exact_tracked_manifest_path(
            compile_context_from,
            inventory=operation_inventory,
            context=f"{path}: compile_context_from",
        )
    raw_source_compile_profiles = data.get("source_compile_profiles", {})
    if not isinstance(raw_source_compile_profiles, dict):
        raise ValueError(f"{path}: source_compile_profiles must be an object")
    if raw_source_compile_profiles and authenticate_current_paths:
        exact_profiles: dict[str, Any] = {}
        for raw_source_path, profile_name in raw_source_compile_profiles.items():
            if not isinstance(raw_source_path, str):
                exact_profiles[raw_source_path] = profile_name
                continue
            exact_source_path = _exact_tracked_manifest_path(
                raw_source_path,
                inventory=operation_inventory,
                context=f"{path}: source_compile_profiles source",
                allowed_suffixes=ORDER_EDIT_PATH_SUFFIXES,
            )
            exact_profiles[exact_source_path] = profile_name
        raw_source_compile_profiles = exact_profiles
    if compile_context_from and compiler_profile:
        raise ValueError(f"{path}: compile_context_from is mutually exclusive with compiler_profile")
    if compile_context_from and raw_source_compile_profiles:
        raise ValueError(
            f"{path}: source_compile_profiles is inherited from compile_context_from and "
            "cannot be specified locally"
        )
    if compile_context_from and ("compiler_env" in data or "compiler_flags" in data):
        raise ValueError(f"{path}: compile_context_from is mutually exclusive with compiler_env/compiler_flags")
    if compiler_profile and ("compiler_env" in data or "compiler_flags" in data):
        raise ValueError(f"{path}: compiler_profile is mutually exclusive with compiler_env/compiler_flags")

    compile_defines: tuple[str, ...] = ()
    source_compile_profiles: tuple[tuple[str, str], ...] = ()
    source_compile_flags: tuple[tuple[str, tuple[str, ...]], ...] = ()
    selected_sources = tuple(
        dict.fromkeys(
            [
                *((source_from,) if source_from else ()),
                *(entry.source_from for entry in translation_unit_function_order),
            ]
        )
    )
    if compile_context_from and has_compile_work:
        context_path = repo_path(compile_context_from)
        try:
            context_data = json.loads(context_path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            raise ValueError(f"{path}: cannot load compile_context_from {context_path}: {exc}") from exc
        if not isinstance(context_data, dict):
            raise ValueError(f"{path}: compile_context_from root must be an object")
        source_rows = require_string_list(context_data, "sources", manifest_path=context_path)
        if authenticate_current_paths:
            reconciled_source_rows: list[str] = []
            for source in source_rows:
                resolution = diagnose_historical_repository_path(
                    source,
                    inventory=operation_inventory,
                    context=f"{context_path}: sources",
                )
                if resolution.current_git_path is None:
                    raise ValueError(
                        f"{context_path}: source row has no unique current Git path: "
                        f"{source} ({resolution.status})"
                    )
                reconciled_source_rows.append(resolution.current_git_path)
            source_rows = tuple(reconciled_source_rows)
        source_row_keys = {compile_source_identity(source) for source in source_rows}
        for required_source in selected_sources:
            if compile_source_identity(required_source) not in source_row_keys:
                raise ValueError(
                    f"{path}: compile_context_from does not compile source row {required_source}"
                )
        compiler_env = require_string(context_data, "vc5_env", manifest_path=context_path)
        base_flags = require_string_list(context_data, "compile_flags", manifest_path=context_path)
        compile_defines = require_string_list(context_data, "defines", manifest_path=context_path)
        # /FAcs is reporting-only. All code-generation flags, defines and include
        # search order are copied verbatim from the canonical final manifest.
        compiler_flags = tuple((*base_flags, *(f"/D{item}" for item in compile_defines), "/FAcs"))
        include_dirs = require_string_list(context_data, "include_dirs", manifest_path=context_path)
        source_compile_profiles, source_compile_flags = parse_source_compile_profiles(
            context_data.get("source_compile_profiles", {}),
            configured_sources=source_rows,
            manifest_path=context_path,
            compiler_env=compiler_env,
            profile_guard=profile_guard,
            final_build_context=True,
            compile_defines=compile_defines,
        )
    elif compiler_profile and has_compile_work:
        profile = profiles_by_name().get(compiler_profile)
        if profile is None:
            raise ValueError(f"{path}: unknown compiler_profile {compiler_profile}")
        disqualification = profile_guard_disqualification(profile_guard, compiler_profile)
        if disqualification is not None:
            raise ValueError(f"{path}: compiler_profile {compiler_profile} is disqualified by profile_guard")
        if not profile_guard_allows_profile(profile_guard, compiler_profile):
            raise ValueError(
                f"{path}: compiler_profile {compiler_profile} is not listed in "
                "profile_guard.accepted_profiles"
            )
        compiler_env = profile.compiler_env
        compiler_flags = profile.compiler_flags
    elif has_compile_work:
        compiler_env = data.get("compiler_env", "")
        if compiler_env and not isinstance(compiler_env, str):
            raise ValueError(f"{path}: expected 'compiler_env' as a string")
        compiler_flags = require_string_list(
            data,
            "compiler_flags",
            manifest_path=path,
            allow_empty_items=True,
        )
    else:
        compiler_env = ""
        compiler_flags = ()

    if raw_source_compile_profiles and has_compile_work:
        source_compile_profiles, source_compile_flags = parse_source_compile_profiles(
            raw_source_compile_profiles,
            configured_sources=selected_sources,
            manifest_path=path,
            compiler_env=compiler_env,
            profile_guard=profile_guard,
            final_build_context=False,
        )
        if not compiler_env:
            first_profile_name = source_compile_profiles[0][1]
            compiler_env = profiles_by_name()[first_profile_name].compiler_env

    generated_files_tuple = tuple(generated_files)
    functions_tuple = tuple(functions)
    data_symbols_tuple = tuple(data_symbols)
    target_binary = manifest_target_binary(
        data,
        manifest_path=path,
        source_from=source_from or "",
        functions=functions_tuple,
        data_symbols=data_symbols_tuple,
    )
    source_emission_warnings: list[SourceEmissionWarning] = []
    if enforce_source_policy and has_compile_work:
        source_emission_warnings.extend(validate_source_policy(
            data=data,
            manifest_path=path,
            source_from=source_from or "",
            functions=functions_tuple,
            data_symbols=data_symbols_tuple,
            generated_files=generated_files_tuple,
            target_binary=target_binary,
            strict_source_emissions=strict_source_emissions,
            strict_source_traceability=strict_source_traceability,
            tracked_path_inventory=operation_inventory,
        ))
        source_emission_warnings.extend(validate_translation_unit_source_policy(
            manifest_path=path,
            entries=translation_unit_function_order,
            target_binary=target_binary,
            strict_source_emissions=strict_source_emissions,
            strict_source_traceability=strict_source_traceability,
            policy_closure_paths=order_edit_paths,
        ))
    deduped_source_emission_warnings: dict[tuple[str, str, str], SourceEmissionWarning] = {}
    for warning in source_emission_warnings:
        deduped_source_emission_warnings.setdefault(
            (warning.address, warning.code, warning.source_from),
            warning,
        )
    source_emission_warnings_tuple = tuple(deduped_source_emission_warnings.values())

    source_filename = data.get("source_filename", "")
    if has_compile_work:
        source_filename = require_string(data, "source_filename", manifest_path=path)
    elif not isinstance(source_filename, str):
        raise ValueError(f"{path}: expected 'source_filename' as a string when present")

    manifest_include_dirs = (
        include_dirs
        if compile_context_from and has_compile_work
        else require_string_list(
            data,
            "include_dirs",
            manifest_path=path,
            allow_empty_items=True,
        )
    )
    manifest_source_files = require_string_list(
        data,
        "source_files",
        manifest_path=path,
        allow_empty_items=True,
    )
    if authenticate_current_paths:
        manifest_source_files = tuple(
            _exact_tracked_manifest_path(
                source_file,
                inventory=operation_inventory,
                context=f"{path}: source_files",
                allowed_suffixes=ORDER_EDIT_PATH_SUFFIXES,
            )
            for source_file in manifest_source_files
            if source_file
        )

    return VerifyTarget(
        name=require_string(data, "name", manifest_path=path),
        description=require_string(data, "description", manifest_path=path),
        source_filename=source_filename,
        source_text=source_text,
        source_from=source_from or "",
        compare_mode=compare_mode,
        trim_trailing_nops=optional_bool(data, "trim_trailing_nops", True, manifest_path=path),
        compiler_profile=compiler_profile or "",
        compiler_env=compiler_env or "",
        compiler_flags=compiler_flags,
        include_dirs=manifest_include_dirs,
        source_files=manifest_source_files,
        generated_files=generated_files_tuple,
        functions=functions_tuple,
        data_symbols=data_symbols_tuple,
        check_function_order=optional_bool(data, "check_function_order", False, manifest_path=path),
        function_order_scope=function_order_scope,
        target_binary=target_binary,
        check_translation_unit_function_order=check_translation_unit_function_order,
        translation_unit_function_order=translation_unit_function_order,
        linked_function_intervals=linked_function_intervals,
        manifest_path=path,
        profile_guard=profile_guard,
        compile_context_from=compile_context_from or "",
        source_emission_warnings=source_emission_warnings_tuple,
        source_emission_policy_strict=strict_source_emissions,
        source_traceability_policy_strict=strict_source_traceability,
        compile_defines=compile_defines,
        retail_start=normalize_address(data["retail_start"]) if data.get("retail_start") else "",
        retail_end_exclusive=(
            normalize_address(data["retail_end_exclusive"])
            if data.get("retail_end_exclusive")
            else ""
        ),
        source_compile_profiles=source_compile_profiles,
        source_compile_flags=source_compile_flags,
        linked_order_base_target=linked_order_base_target,
        linked_order_diagnostic_mode=linked_order_diagnostic_mode,
        order_edit_paths=order_edit_paths,
    )


def load_manifests(
    manifest_dir: Path,
    *,
    enforce_source_policy: bool = True,
    strict_source_emissions: bool = False,
    strict_source_traceability: bool = False,
    tracked_path_inventory: GitTrackedPathInventory | None = None,
) -> list[VerifyTarget]:
    candidate_dir = manifest_dir if manifest_dir.is_absolute() else REPO_ROOT / manifest_dir
    inventory = tracked_path_inventory
    if inventory is None:
        try:
            inventory = load_git_tracked_path_inventory(REPO_ROOT)
        except RepositoryPathError:
            if (REPO_ROOT / ".git").exists():
                raise
            inventory = None
    if inventory is None:
        manifest_paths = tuple(sorted(candidate_dir.glob("*.json")))
        strict_tracked_paths = False
    else:
        try:
            relative_dir = candidate_dir.absolute().relative_to(
                inventory.repository_root.absolute()
            ).as_posix()
        except ValueError:
            relative_dir = ""
        if relative_dir:
            prefix = relative_dir.rstrip("/") + "/"
            manifest_git_paths = tuple(
                sorted(
                    git_path
                    for git_path in inventory.exact_paths
                    if git_path.startswith(prefix)
                    and "/" not in git_path[len(prefix) :]
                    and git_path.endswith(".json")
                )
            )
            manifest_paths = tuple(
                resolve_tracked_repository_file(
                    git_path,
                    repository_root=inventory.repository_root,
                    inventory=inventory,
                    context="registered VC5 manifest",
                ).physical_path
                for git_path in manifest_git_paths
            )
            strict_tracked_paths = None
        else:
            manifest_paths = tuple(sorted(candidate_dir.glob("*.json")))
            strict_tracked_paths = False
    registered_paths = registered_vc5_manifest_paths() if inventory is not None else frozenset()
    manifests = [
        load_manifest(
            path,
            enforce_source_policy=enforce_source_policy,
            strict_source_emissions=strict_source_emissions,
            strict_source_traceability=strict_source_traceability,
            tracked_path_inventory=inventory,
            strict_tracked_paths=(
                path.absolute().relative_to(inventory.repository_root.absolute()).as_posix()
                in registered_paths
                if strict_tracked_paths is None and inventory is not None
                else bool(strict_tracked_paths)
            ),
        )
        for path in manifest_paths
    ]
    names: set[str] = set()
    for manifest in manifests:
        if manifest.name in names:
            raise ValueError(f"Duplicate VC verification target name: {manifest.name}")
        names.add(manifest.name)
    return manifests


def print_source_emission_warnings(manifests: list[VerifyTarget]) -> None:
    seen: set[tuple[str, str, str, str]] = set()
    for target in manifests:
        for warning in target.source_emission_warnings:
            key = (
                str(target.manifest_path),
                warning.address,
                warning.code,
                warning.source_from,
            )
            if key in seen:
                continue
            seen.add(key)
            print(
                f"source-emission-warning: {target.manifest_path}: {warning.address} "
                f"{warning.code}: {warning.message} [source_from={warning.source_from}]",
                file=sys.stderr,
            )


def profile_guard_disqualification(
    guard: ProfileGuard,
    profile_name: str,
) -> ProfileGuardEntry | None:
    for entry in guard.disqualified_profiles:
        if entry.profile == profile_name:
            return entry
    return None


def profile_guard_accepted_entry(
    guard: ProfileGuard,
    profile_name: str,
) -> ProfileGuardEntry | None:
    for entry in guard.accepted_profiles:
        if entry.profile == profile_name:
            return entry
    return None


def profile_guard_allows_profile(
    guard: ProfileGuard,
    profile_name: str,
) -> bool:
    if not guard.accepted_profiles:
        return True
    return profile_guard_accepted_entry(guard, profile_name) is not None


def target_profile_guard_disqualification(
    target: VerifyTarget,
    profile_name: str,
) -> ProfileGuardEntry | None:
    return profile_guard_disqualification(target.profile_guard, profile_name)


def format_profile_guard_disqualification(
    target: VerifyTarget,
    profile_name: str,
    entry: ProfileGuardEntry,
) -> str:
    details: list[str] = []
    if target.profile_guard.scope:
        details.append(f"scope={target.profile_guard.scope}")
    if entry.sentinel_addresses:
        details.append("sentinels=" + ",".join(entry.sentinel_addresses))
    if entry.evidence:
        details.append(f"evidence={entry.evidence}")
    if entry.reason:
        details.append(f"reason={entry.reason}")
    suffix = "; " + "; ".join(details) if details else ""
    return (
        f"{target.manifest_path}: compiler profile {profile_name} is disqualified "
        f"by profile_guard for {target.name}{suffix}"
    )


def format_profile_guard_not_accepted(target: VerifyTarget, profile_name: str) -> str:
    accepted = ", ".join(entry.profile for entry in target.profile_guard.accepted_profiles)
    details: list[str] = []
    if target.profile_guard.scope:
        details.append(f"scope={target.profile_guard.scope}")
    if accepted:
        details.append(f"accepted_profiles={accepted}")
    suffix = "; " + "; ".join(details) if details else ""
    return (
        f"{target.manifest_path}: compiler profile {profile_name} is not listed in "
        f"profile_guard.accepted_profiles for {target.name}{suffix}"
    )


def profile_guard_block_message(target: VerifyTarget, profile_name: str) -> str | None:
    disqualification = target_profile_guard_disqualification(target, profile_name)
    if disqualification is not None:
        return format_profile_guard_disqualification(target, profile_name, disqualification)
    if not profile_guard_allows_profile(target.profile_guard, profile_name):
        return format_profile_guard_not_accepted(target, profile_name)
    return None


def find_target(
    manifests: list[VerifyTarget],
    selector: str,
) -> tuple[VerifyTarget, tuple[VerifyFunction, ...], tuple[VerifyDataSymbol, ...], str]:
    normalized_address = None
    if selector.lower().startswith("0x"):
        normalized_address = normalize_address(selector)

    if normalized_address:
        function_matches = [
            (manifest, function)
            for manifest in manifests
            for function in manifest.functions
            if function.address == normalized_address
        ]
        data_matches = [
            (manifest, data_symbol)
            for manifest in manifests
            for data_symbol in manifest.data_symbols
            if data_symbol_covers_address(data_symbol, normalized_address)
        ]
        matches = [*function_matches, *data_matches]
        if not matches:
            raise ValueError(f"No VC verification manifest covers {normalized_address}")
        if len(matches) > 1:
            names = ", ".join(manifest.name for manifest, _item in matches)
            raise ValueError(f"Multiple VC verification manifests cover {normalized_address}: {names}")
        if function_matches:
            manifest, function = function_matches[0]
            return manifest, (function,), (), normalized_address
        manifest, data_symbol = data_matches[0]
        return manifest, (), (data_symbol,), normalized_address

    for manifest in manifests:
        if manifest.name == selector:
            if not target_has_compile_work(manifest):
                raise ValueError(
                    f"VC verification target {selector} contains linked_function_intervals only; "
                    "use 'python tools/recoil.py verify final-build -- --order-target "
                    f"{selector}'"
                )
            return manifest, manifest.functions, manifest.data_symbols, manifest.name
    raise ValueError(f"Unknown VC verification target: {selector}")


def covering_targets(manifests: list[VerifyTarget], address: str) -> list[tuple[VerifyTarget, VerifyFunction]]:
    normalized_address = normalize_address(address)
    return [
        (manifest, function)
        for manifest in manifests
        for function in manifest.functions
        if function.address == normalized_address
    ]


def covering_data_symbols(manifests: list[VerifyTarget], address: str) -> list[tuple[VerifyTarget, VerifyDataSymbol]]:
    normalized_address = normalize_address(address)
    return [
        (manifest, data_symbol)
        for manifest in manifests
        for data_symbol in manifest.data_symbols
        if data_symbol_covers_address(data_symbol, normalized_address)
    ]


def data_symbol_range(data_symbol: VerifyDataSymbol) -> tuple[int, int]:
    start = int(normalize_address(data_symbol.address), 16)
    return start, start + data_symbol.byte_length


def data_symbol_covers_address(data_symbol: VerifyDataSymbol, address: str) -> bool:
    value = int(normalize_address(address), 16)
    start, end = data_symbol_range(data_symbol)
    return start <= value < end


def data_symbol_covers_entry(data_symbol: VerifyDataSymbol, entry: OwnerEntry) -> bool:
    if not entry.is_data_entry:
        return False
    entry_start = int(normalize_address(entry.address), 16)
    entry_size = 1
    if entry.data_size:
        try:
            parsed_size = int(str(entry.data_size).strip(), 0)
        except ValueError:
            parsed_size = 0
        if parsed_size > 0:
            entry_size = parsed_size
    entry_end = entry_start + entry_size
    symbol_start, symbol_end = data_symbol_range(data_symbol)
    return symbol_start <= entry_start and entry_end <= symbol_end


def data_symbol_coverage_matches(
    manifests: list[VerifyTarget],
    entry: OwnerEntry,
) -> list[tuple[VerifyTarget, VerifyDataSymbol]]:
    return [
        (manifest, data_symbol)
        for manifest in manifests
        for data_symbol in manifest.data_symbols
        if data_symbol_covers_entry(data_symbol, entry)
    ]


def with_compiler_profile_override(
    target: VerifyTarget,
    profile_name: str,
    *,
    allow_disqualified_profile: bool = False,
) -> VerifyTarget:
    profile = profiles_by_name().get(profile_name)
    if profile is None:
        raise ValueError(f"Unknown compiler profile override: {profile_name}")
    guard_message = profile_guard_block_message(target, profile_name)
    if guard_message is not None and not allow_disqualified_profile:
        raise ValueError(guard_message)
    return replace(
        target,
        compiler_profile=profile.name,
        compiler_env=profile.compiler_env,
        compiler_flags=profile.compiler_flags,
        source_compile_profiles=(),
        source_compile_flags=(),
    )


def parse_profile_sweep_spec(spec: str) -> list[str]:
    available = profiles_by_name()
    if spec == "*":
        return [name for name in available if "raw_asm" not in name]
    names = [name.strip() for name in spec.split(",") if name.strip()]
    if not names:
        raise ValueError("--profile-sweep requires at least one profile name")
    for name in names:
        if name not in available:
            raise ValueError(f"Unknown compiler profile override: {name}")
    return names


def owner_data_entry_byte_length(entry: OwnerEntry) -> int:
    if entry.data_size:
        try:
            parsed_size = int(str(entry.data_size).strip(), 0)
        except ValueError:
            parsed_size = 0
        if parsed_size > 0:
            return parsed_size
    return 1


def data_entry_covers_address(entry: OwnerEntry, address: str) -> bool:
    if not entry.is_data_entry:
        return False
    start = int(normalize_address(entry.address), 16)
    end = start + owner_data_entry_byte_length(entry)
    value = int(normalize_address(address), 16)
    return start <= value < end


def owner_data_entry_for_address(address: str, owners_path: Path | None) -> OwnerEntry | None:
    if owners_path is None:
        return None
    try:
        entry_index = OwnerEntryIndex.load(owners_path)
    except (OSError, ValueError):
        return None
    normalized_address = normalize_address(address)
    exact_entry = entry_index.entries.get(normalized_address)
    if exact_entry is not None and exact_entry.is_data_entry:
        return exact_entry
    for entry in entry_index.entries.values():
        if data_entry_covers_address(entry, normalized_address):
            return entry
    return None


def data_manifest_skeleton(address: str, entry: OwnerEntry) -> dict[str, object]:
    normalized_address = normalize_address(entry.address or address)
    safe_address = normalized_address[2:]
    source_from = entry.reimplemented_file if entry.reimplemented_file and entry.reimplemented_file != "pending" else "src/TODO_SOURCE_FILE.cpp"
    byte_length = owner_data_entry_byte_length(entry)
    bn_name = entry.reconstructed_name or "TODO_BN_NAME"
    data_symbol = VerifyDataSymbol(
        address=normalized_address,
        symbol="TODO_DECORATED_VC5_SYMBOL",
        name="TODO_SOURCE_NAME",
        byte_length=byte_length,
        bn_name=bn_name,
    )
    target_binary = inferred_target_binary(source_from=source_from, functions=(), data_symbols=(data_symbol,))
    return {
        "name": f"verify_{safe_address}",
        "description": f"VC data-symbol verification target for {normalized_address}.",
        "target_binary": target_binary,
        "source_filename": f"verify_{safe_address}.cpp",
        "compiler_profile": "vc5_o2_ob0_facs",
        "include_dirs": ["src"],
        "source_files": [],
        "source_from": source_from,
        "data_symbols": [
            {
                "address": normalized_address,
                "symbol": "TODO_DECORATED_VC5_SYMBOL",
                "symbol_regex": "TODO_DECORATED_VC5_SYMBOL_REGEX",
                "name": "TODO_SOURCE_NAME",
                "bn_name": bn_name,
                "byte_length": byte_length,
            }
        ],
    }


def manifest_skeleton(address: str, owner_entry: OwnerEntry | None = None) -> dict[str, object]:
    if owner_entry is not None and owner_entry.is_data_entry:
        return data_manifest_skeleton(address, owner_entry)
    normalized_address = normalize_address(address)
    safe_address = normalized_address[2:]
    target_binary = inferred_target_binary(source_from="", functions=(), data_symbols=(
        VerifyDataSymbol(
            address=normalized_address,
            symbol="TODO_DECORATED_VC5_SYMBOL",
            name="TODO_SOURCE_NAME",
            byte_length=1,
        ),
    ))
    return {
        "name": f"verify_{safe_address}",
        "description": f"VC verification target for {normalized_address}.",
        "target_binary": target_binary,
        "source_filename": f"verify_{safe_address}.cpp",
        "compiler_profile": "vc5_o2_ob0_facs",
        "include_dirs": ["src"],
        "source_files": [],
        "source_from": "src/TODO_SOURCE_FILE.cpp",
        "functions": [
            {
                "address": normalized_address,
                "symbol": "TODO_DECORATED_VC5_SYMBOL",
                "name": "TODO_SOURCE_NAME",
            }
        ],
    }


def print_missing_explanation(manifests: list[VerifyTarget], address: str, owners_path: Path | None = None) -> None:
    normalized_address = normalize_address(address)
    matches = covering_targets(manifests, normalized_address)
    data_matches = covering_data_symbols(manifests, normalized_address)
    if matches or data_matches:
        print(f"{normalized_address} is covered by:")
        for manifest, function in matches:
            print(f"- {manifest.name}: {function.symbol} ({manifest.manifest_path})")
        for manifest, data_symbol in data_matches:
            symbol = data_symbol.symbol or f"symbol_regex={data_symbol.symbol_regex}"
            print(f"- {manifest.name}: {symbol} ({manifest.manifest_path})")
        return

    print(f"No VC verification manifest covers {normalized_address}.")
    print("Create or extend a JSON manifest under tools/vc5_verify_targets/.")
    print("Suggested starting point:")
    owner_entry = owner_data_entry_for_address(normalized_address, owners_path)
    print(json.dumps(manifest_skeleton(normalized_address, owner_entry), indent=2))


def resolve_repo_path(path_text: str) -> Path:
    return repo_path(path_text)


def build_compile_command(
    target: VerifyTarget,
    source_path: Path,
    vc5_env: Path,
    build_dir: Path | None = None,
    *,
    source_from: str = "",
) -> str:
    include_args = " ".join(f"/I {quote_cmd_arg(resolve_repo_path(include_dir))}" for include_dir in target.include_dirs)
    _profile_name, effective_flags = effective_source_compile_context(target, source_from)
    flags = " ".join(effective_flags)
    if build_dir is not None:
        try:
            source_arg = source_path.relative_to(build_dir)
        except ValueError:
            source_arg = source_path
    else:
        source_arg = source_path.name
    return f"call {quote_cmd_arg(vc5_env)} && cl {flags} {include_args} /c {quote_cmd_arg(source_arg)}"


def ordered_compile_argv(
    target: VerifyTarget,
    source_path: Path,
    build_dir: Path | None = None,
    *,
    source_from: str = "",
) -> tuple[str, ...]:
    """Return the exact explicit compiler token order used by the VC5 driver.

    This intentionally retains duplicates.  It is a compiler receipt input,
    not an unordered option set and not an assertion about options inherited
    from ``CL``/``_CL_`` or response files.
    """

    _profile_name, effective_flags = effective_source_compile_context(
        target, source_from
    )
    if build_dir is not None:
        try:
            source_arg = source_path.relative_to(build_dir)
        except ValueError:
            source_arg = source_path
    else:
        source_arg = Path(source_path.name)
    argv: list[str] = ["cl"]
    argv.extend(str(flag) for flag in effective_flags)
    for include_dir in target.include_dirs:
        argv.extend(("/I", str(resolve_repo_path(include_dir))))
    argv.extend(("/c", str(source_arg)))
    return tuple(argv)


def _split_windows_option_text(value: str) -> tuple[str, ...]:
    if not value.strip():
        return ()
    return tuple(shlex.split(value, posix=False))


def _expand_response_file_tokens(
    tokens: Sequence[str],
    *,
    cwd: Path,
    _active: tuple[str, ...] = (),
) -> tuple[tuple[str, ...], tuple[dict[str, Any], ...], tuple[str, ...]]:
    """Expand ordered compiler response-file tokens through stable reads.

    The returned token stream retains duplicates.  Physical identities and
    parsed tokens are recorded directly for each occurrence; no content
    summary is computed.
    """

    expanded: list[str] = []
    rows: list[dict[str, Any]] = []
    errors: list[str] = []
    for ordinal, token in enumerate(tokens):
        if not token.startswith("@") or len(token) == 1:
            expanded.append(token)
            continue
        raw_path = token[1:].strip().strip('"')
        response_path = Path(raw_path)
        if not response_path.is_absolute():
            response_path = cwd / response_path
        response_path = response_path.resolve()
        key = str(response_path).casefold()
        if key in _active:
            errors.append(f"response-file-cycle:{response_path}")
            continue
        try:
            with StableReadHandle(response_path) as stream:
                raw = stream.read()
                identity = stream.identity.to_dict()
        except OSError as exc:
            errors.append(f"response-file-unavailable:{response_path}:{exc}")
            continue
        try:
            text = raw.decode("mbcs" if os.name == "nt" else "latin-1")
            direct_tokens = _split_windows_option_text(text)
        except (UnicodeError, ValueError) as exc:
            errors.append(f"response-file-unparseable:{response_path}:{exc}")
            continue
        nested_tokens, nested_rows, nested_errors = _expand_response_file_tokens(
            direct_tokens,
            cwd=response_path.parent,
            _active=(*_active, key),
        )
        rows.append({
            "ordinal": ordinal,
            "path": str(response_path),
            "physical_identity": identity,
            "size": identity["file_size"],
            "tokens": list(direct_tokens),
            "expanded_tokens": list(nested_tokens),
        })
        rows.extend(nested_rows)
        errors.extend(nested_errors)
        expanded.extend(nested_tokens)
    return tuple(expanded), tuple(rows), tuple(errors)


def _capture_vc5_environment(
    compiler_env: Path,
    *,
    cwd: Path,
) -> tuple[dict[str, str], str | None]:
    try:
        completed = run_tool_cmd_script(
            f"call {quote_cmd_arg(compiler_env)} && set",
            cwd=cwd,
            script_name="_capture_vc5_environment.cmd",
            capture_output=True,
        )
    except (OSError, RuntimeError, ValueError) as exc:
        return {}, str(exc)
    if completed.returncode != 0:
        return {}, f"environment command exited {completed.returncode}"
    environment: dict[str, str] = {}
    for raw_line in completed.stdout.splitlines():
        if "=" in raw_line:
            key, value = raw_line.split("=", 1)
            if key:
                environment[key.upper()] = value
    if "PATH" not in environment:
        return environment, "captured VC5 environment has no PATH"
    return environment, None


def _windows_file_version(path: Path) -> str:
    if os.name != "nt":
        return "<unavailable-non-windows>"
    try:
        version = ctypes.windll.version
        size = version.GetFileVersionInfoSizeW(str(path), None)
        if not size:
            return "<not detected>"
        buffer = ctypes.create_string_buffer(size)
        if not version.GetFileVersionInfoW(str(path), 0, size, buffer):
            return "<not detected>"
        pointer = ctypes.c_void_p()
        length = ctypes.c_uint()
        if not version.VerQueryValueW(buffer, "\\", ctypes.byref(pointer), ctypes.byref(length)):
            return "<not detected>"
        class VS_FIXEDFILEINFO(ctypes.Structure):
            _fields_ = [
                ("dwSignature", ctypes.c_uint32), ("dwStrucVersion", ctypes.c_uint32),
                ("dwFileVersionMS", ctypes.c_uint32), ("dwFileVersionLS", ctypes.c_uint32),
                ("dwProductVersionMS", ctypes.c_uint32), ("dwProductVersionLS", ctypes.c_uint32),
                ("dwFileFlagsMask", ctypes.c_uint32), ("dwFileFlags", ctypes.c_uint32),
                ("dwFileOS", ctypes.c_uint32), ("dwFileType", ctypes.c_uint32),
                ("dwFileSubtype", ctypes.c_uint32), ("dwFileDateMS", ctypes.c_uint32),
                ("dwFileDateLS", ctypes.c_uint32),
            ]
        info = ctypes.cast(pointer, ctypes.POINTER(VS_FIXEDFILEINFO)).contents
        return ".".join(str(value) for value in (
            info.dwFileVersionMS >> 16, info.dwFileVersionMS & 0xFFFF,
            info.dwFileVersionLS >> 16, info.dwFileVersionLS & 0xFFFF,
        ))
    except (AttributeError, OSError, ValueError):
        return "<not detected>"


def _resolved_program(path_value: str, *names: str) -> Path | None:
    for name in names:
        found = shutil.which(name, path=path_value)
        if found:
            return Path(found).resolve()
    return None


def _toolchain_component_row(
    *,
    role: str,
    path: Path | None,
    status: str,
    version_override: str | None = None,
    invocation: Mapping[str, Any],
) -> dict[str, Any]:
    row: dict[str, Any] = {
        "role": role,
        "status": status,
        "invocation": dict(invocation),
        "path": str(path) if path is not None else None,
        "configured_absolute_path": str(path) if path is not None else None,
        "physical_identity": None,
        "size": None,
        "version": "<not applicable>" if status == "not-applicable" else "<unresolved>",
    }
    if path is None or not path.is_file():
        return row
    identity = physical_identity(path)
    row["physical_identity"] = identity.to_dict()
    row["size"] = identity.file_size
    row["version"] = version_override or _windows_file_version(path)
    return row


def _resolve_toolchain_components(
    environment: Mapping[str, str],
    *,
    compiler_version: str,
) -> tuple[tuple[dict[str, Any], ...], tuple[str, ...]]:
    """Inventory every governed VC5 component for this compile-only operation.

    CL, its selected language front end, and its back end are invoked.  The
    linker, librarian, resource compiler, and assembler are deliberately
    represented as not applicable because the exact command contains ``/c``
    and no assembly or resource input.  Their installed paths are recorded
    when PATH resolves them, but they are never implied to have run.
    """

    path_value = str(environment.get("PATH", ""))
    compiler_path = _resolved_program(path_value, "cl.exe", "cl")
    compiler_dir = compiler_path.parent if compiler_path is not None else None
    front_end = (
        next(
            (
                path
                for path in (
                    compiler_dir / "c1xx.dll",
                    compiler_dir / "c1.dll",
                    compiler_dir / "c1xx.exe",
                    compiler_dir / "c1.exe",
                )
                if path.is_file()
            ),
            None,
        )
        if compiler_dir is not None
        else None
    )
    back_end = (
        next(
            (
                path
                for path in (compiler_dir / "c2.dll", compiler_dir / "c2.exe")
                if path.is_file()
            ),
            None,
        )
        if compiler_dir is not None
        else None
    )
    internal_invocation = {
        "invoked": True,
        "form": "internal-driver-dispatch",
        "argv": "see compiler.argv",
    }
    rows = [
        _toolchain_component_row(
            role="compiler-driver",
            path=compiler_path,
            status="invoked" if compiler_path is not None else "unresolved",
            version_override=compiler_version,
            invocation={"invoked": True, "form": "command-line", "argv": "see compiler.argv"},
        ),
        _toolchain_component_row(
            role="compiler-front-end",
            path=front_end,
            status="invoked-by-driver" if front_end is not None else "unresolved",
            invocation=internal_invocation,
        ),
        _toolchain_component_row(
            role="compiler-back-end",
            path=back_end,
            status="invoked-by-driver" if back_end is not None else "unresolved",
            invocation=internal_invocation,
        ),
    ]
    for role, names in (
        ("linker", ("link.exe", "link")),
        ("librarian", ("lib.exe", "lib")),
        ("resource-compiler", ("rc.exe", "rc")),
        ("assembler", ("ml.exe", "ml")),
    ):
        rows.append(
            _toolchain_component_row(
                role=role,
                path=_resolved_program(path_value, *names),
                status="not-applicable",
                invocation={
                    "invoked": False,
                    "form": "not-applicable",
                    "reason": "compile-only-/c-operation",
                    "argv": [],
                },
            )
        )
    errors = [
        f"{row['role']}-unresolved"
        for row in rows[:3]
        if row["status"] == "unresolved"
    ]
    return tuple(rows), tuple(errors)


_INCLUDE_DIRECTIVE_RE = re.compile(
    rb"(?m)^\s*#\s*include\s*([<\"])([^>\"\r\n]+)[>\"]"
)


def _environment_search_paths(value: str) -> tuple[Path, ...]:
    rows: list[Path] = []
    seen: set[str] = set()
    for raw_path in value.split(";"):
        if not raw_path.strip():
            continue
        path = Path(os.path.expandvars(raw_path.strip().strip('"'))).resolve()
        key = str(path).casefold()
        if key not in seen:
            seen.add(key)
            rows.append(path)
    return tuple(rows)


def _governed_header_inputs(
    *,
    source_path: Path,
    target: VerifyTarget,
    environment: Mapping[str, str],
) -> tuple[tuple[dict[str, Any], ...], tuple[dict[str, Any], ...], tuple[str, ...]]:
    """Resolve direct textual include inputs without deriving content summaries."""

    project_roots = tuple(resolve_repo_path(path).resolve() for path in target.include_dirs)
    environment_roots = _environment_search_paths(str(environment.get("INCLUDE", "")))
    search_roots = project_roots + tuple(
        path for path in environment_roots if path not in project_roots
    )
    root_rows: list[dict[str, Any]] = []
    errors: list[str] = []
    for index, path in enumerate(search_roots):
        identity = None
        if path.is_dir():
            try:
                identity = physical_identity(path, directory=True).to_dict()
            except OSError as exc:
                errors.append(f"include-search-root-identity-unavailable:{path}:{exc}")
        else:
            errors.append(f"include-search-root-unavailable:{path}")
        root_rows.append(
            {
                "ordinal": index,
                "path": str(path),
                "source": "manifest" if path in project_roots else "INCLUDE",
                "physical_identity": identity,
            }
        )

    pending = [source_path.resolve()]
    visited: set[str] = set()
    header_rows: list[dict[str, Any]] = []
    while pending:
        including_path = pending.pop()
        including_key = str(including_path).casefold()
        if including_key in visited:
            continue
        visited.add(including_key)
        try:
            with StableReadHandle(including_path) as source_stream:
                source_data = source_stream.read()
        except OSError as exc:
            errors.append(f"include-input-unreadable:{including_path}:{exc}")
            continue
        for match in _INCLUDE_DIRECTIVE_RE.finditer(source_data):
            delimiter = match.group(1)
            include_text = match.group(2).decode("mbcs" if os.name == "nt" else "latin-1")
            candidates: list[Path] = []
            if delimiter == b'"':
                candidates.append(including_path.parent / include_text)
            candidates.extend(root / include_text for root in search_roots)
            resolved = next((candidate.resolve() for candidate in candidates if candidate.is_file()), None)
            if resolved is None:
                errors.append(f"include-input-unresolved:{including_path}:{include_text}")
                continue
            key = str(resolved).casefold()
            if key in visited or any(str(row["path"]).casefold() == key for row in header_rows):
                continue
            try:
                identity = physical_identity(resolved).to_dict()
            except OSError as exc:
                errors.append(f"include-input-identity-unavailable:{resolved}:{exc}")
                continue
            try:
                resolved.relative_to(REPO_ROOT.resolve())
                role = "project-header"
            except ValueError:
                role = "toolchain-header"
            header_rows.append(
                {
                    "path": str(resolved),
                    "role": role,
                    "included_from": str(including_path),
                    "physical_identity": identity,
                    "size": identity["file_size"],
                }
            )
            pending.append(resolved)
    return tuple(header_rows), tuple(root_rows), tuple(sorted(set(errors)))


def _target_manifest_projection(
    target: VerifyTarget,
    *,
    source_from: str,
    manifest_index: int,
) -> dict[str, Any]:
    profile_name, flags = effective_source_compile_context(target, source_from)
    return {
        "name": target.name,
        "target_binary": target.target_binary,
        "manifest_path": str(Path(target.manifest_path).resolve()),
        "source_from": source_from,
        "manifest_index": manifest_index,
        "include_dirs": list(target.include_dirs),
        "compiler_profile": profile_name,
        "compiler_flags": list(flags),
        "compile_context_from": str(target.compile_context_from or ""),
    }


def build_compiler_receipt(
    *,
    target: VerifyTarget,
    source_path: Path,
    source_from: str,
    manifest_index: int,
    compiler_env: Path,
    build_dir: Path,
    compiler_version: str,
) -> dict[str, Any]:
    """Record exact configured paths, physical identities, versions and argv."""
    reasons: list[str] = []
    explicit_argv = ordered_compile_argv(target, source_path, build_dir, source_from=source_from)
    environment, environment_error = _capture_vc5_environment(compiler_env, cwd=build_dir)
    if environment_error:
        reasons.append(f"compiler-environment-unavailable:{environment_error}")
    inherited_cl = str(environment.get("CL", ""))
    inherited_post = str(environment.get("_CL_", ""))
    try:
        inherited_cl_tokens = _split_windows_option_text(inherited_cl)
        inherited_post_tokens = _split_windows_option_text(inherited_post)
    except ValueError as exc:
        inherited_cl_tokens = ()
        inherited_post_tokens = ()
        reasons.append(f"inherited-options-unparseable:{exc}")
    effective_tokens = explicit_argv[:1] + inherited_cl_tokens + explicit_argv[1:] + inherited_post_tokens
    expanded_tokens, response_files, response_errors = _expand_response_file_tokens(
        effective_tokens,
        cwd=build_dir,
    )
    reasons.extend(response_errors)
    exact_command_line = build_compile_command(
        target,
        source_path,
        compiler_env,
        build_dir,
        source_from=source_from,
    )
    components, component_errors = _resolve_toolchain_components(environment, compiler_version=compiler_version)
    bound_components: list[dict[str, Any]] = []
    for raw_component in components:
        component = dict(raw_component)
        invocation = dict(component.get("invocation", {}))
        if invocation.get("invoked") is True:
            invocation["driver_argv"] = list(effective_tokens)
            invocation["exact_driver_command_line"] = exact_command_line
        component["invocation"] = invocation
        bound_components.append(component)
    components = tuple(bound_components)
    reasons.extend(component_errors)
    unavailable_versions = {
        "", "<not detected>", "<unresolved>", "<unavailable-non-windows>",
    }
    for component in components[:3]:
        role = str(component.get("role", "required-component"))
        path_value = component.get("configured_absolute_path")
        identity_value = component.get("physical_identity")
        size_value = component.get("size")
        version_value = component.get("version")
        if (
            not isinstance(path_value, str)
            or not path_value
            or not Path(path_value).is_absolute()
        ):
            reasons.append(f"{role}-absolute-path-unavailable")
        if not isinstance(identity_value, Mapping):
            reasons.append(f"{role}-physical-identity-unavailable")
        if (
            isinstance(size_value, bool)
            or not isinstance(size_value, int)
            or size_value <= 0
        ):
            reasons.append(f"{role}-size-unavailable")
        if (
            not isinstance(version_value, str)
            or version_value.casefold() in unavailable_versions
        ):
            reasons.append(f"{role}-version-unavailable")
    authored_source = Path(source_from)
    if not authored_source.is_absolute():
        authored_source = resolve_repo_path(source_from)
    authored_source = authored_source.resolve()
    manifest_path = Path(target.manifest_path).resolve()
    source_identity = physical_identity(authored_source).to_dict() if authored_source.is_file() else None
    input_identity = physical_identity(source_path).to_dict() if source_path.is_file() else None
    manifest_identity = physical_identity(manifest_path).to_dict() if manifest_path.is_file() else None
    compiler_env_path = Path(compiler_env).resolve()
    compiler_env_identity = (
        physical_identity(compiler_env_path).to_dict()
        if compiler_env_path.is_file()
        else None
    )
    if source_identity is None:
        reasons.append("authored-source-input-unavailable")
    if input_identity is None:
        reasons.append("compiled-input-unavailable")
    if manifest_identity is None:
        reasons.append("manifest-unavailable")
    if compiler_env_identity is None:
        reasons.append("compiler-environment-script-unavailable")
    header_inputs, include_search_roots, header_errors = _governed_header_inputs(
        source_path=source_path,
        target=target,
        environment=environment,
    )
    reasons.extend(header_errors)
    profile_name, profile_flags = effective_source_compile_context(target, source_from)
    manifest_entry_id = f"{manifest_path}#{manifest_index}"
    return {
        "contract_version": 3,
        "tu_context_id": f"recoil:call-contract-tu:{target.name}:{manifest_index}",
        "manifest_entry_id": manifest_entry_id,
        "source_from": source_from,
        "source": {"path": str(authored_source), "physical_identity": source_identity},
        "dependencies": [
            {"path": str(authored_source), "role": "authored-source", "physical_identity": source_identity},
            {"path": str(source_path.resolve()), "role": "compiled-input", "physical_identity": input_identity},
            {"path": str(compiler_env_path), "role": "compiler-environment-script", "physical_identity": compiler_env_identity},
            *list(header_inputs),
        ],
        "compiler": {
            "executable_path": next(
                (
                    str(row["path"])
                    for row in components
                    if row.get("role") == "compiler-driver"
                    and isinstance(row.get("path"), str)
                ),
                "",
            ),
            "explicit_argv": list(explicit_argv),
            "argv": list(effective_tokens),
            "expanded_argv": list(expanded_tokens),
            "response_files": list(response_files),
            "exact_command_line": exact_command_line,
            "cwd": "${RECOIL_ISOLATED_BUILD_ROOT}",
            "observed_cwd": str(build_dir.resolve()),
            "inherited_options": {"CL": inherited_cl, "_CL_": inherited_post, "CL_tokens": list(inherited_cl_tokens), "_CL__tokens": list(inherited_post_tokens)},
            "environment_capture_complete": environment_error is None,
        },
        "toolchain": {
            "profile_id": profile_name,
            "profile_flags": list(profile_flags),
            "components": list(components),
            "compiler_version": compiler_version,
            "include_search_roots": list(include_search_roots),
            "header_inputs": list(header_inputs),
            "library_inputs": {
                "status": "not-applicable",
                "reason": "compile-only-/c-operation",
                "rows": [],
            },
        },
        "manifest": {"path": str(manifest_path), "physical_identity": manifest_identity, "projection": _target_manifest_projection(target, source_from=source_from, manifest_index=manifest_index)},
        "verification_eligible": not reasons,
        "ineligibility_reasons": sorted(set(reasons)),
    }


def _direct_observation_differences(
    before: object,
    after: object,
    *,
    field: str = "$",
) -> list[str]:
    """Return exact field paths that differ between two direct observations."""

    if type(before) is not type(after):
        return [field]
    if isinstance(before, Mapping):
        before_keys = set(before)
        after_keys = set(after)  # type: ignore[arg-type]
        differences = [f"{field}.{key}" for key in sorted(before_keys ^ after_keys)]
        for key in sorted(before_keys & after_keys):
            differences.extend(_direct_observation_differences(
                before[key], after[key], field=f"{field}.{key}"  # type: ignore[index]
            ))
        return differences
    if isinstance(before, (list, tuple)):
        if len(before) != len(after):  # type: ignore[arg-type]
            return [field + ".length"]
        differences: list[str] = []
        for index, (left, right) in enumerate(zip(before, after)):  # type: ignore[arg-type]
            differences.extend(_direct_observation_differences(
                left, right, field=f"{field}[{index}]"
            ))
        return differences
    return [] if before == after else [field]


def compiler_receipt_stability(
    before: Mapping[str, Any],
    after: Mapping[str, Any],
) -> dict[str, Any]:
    """Build a direct pre/post stability receipt with no derived summary."""

    differences = _direct_observation_differences(before, after)
    eligible = (
        before.get("verification_eligible") is True
        and after.get("verification_eligible") is True
        and not differences
    )
    reasons = [str(row) for row in before.get("ineligibility_reasons", ())]
    reasons.extend(str(row) for row in after.get("ineligibility_reasons", ()))
    reasons.extend(f"toolchain-drift:{field}" for field in differences)
    return {
        "contract_version": 4,
        "pre_observation": dict(before),
        "post_observation": dict(after),
        "direct_difference_fields": differences,
        "verification_eligible": eligible,
        "ineligibility_reasons": sorted(set(reasons)),
    }


@contextlib.contextmanager
def bind_compiler_observation(observation: Mapping[str, Any]):
    """Hold required observed files read-only through the compiler invocation."""

    paths: list[str] = []
    toolchain = observation.get("toolchain")
    if isinstance(toolchain, Mapping):
        for component in toolchain.get("components", ()):
            if (
                isinstance(component, Mapping)
                and component.get("status") in {"invoked", "invoked-by-driver"}
                and isinstance(component.get("path"), str)
            ):
                paths.append(str(component["path"]))
        for header in toolchain.get("header_inputs", ()):
            if isinstance(header, Mapping) and isinstance(header.get("path"), str):
                paths.append(str(header["path"]))
    compiler = observation.get("compiler")
    if isinstance(compiler, Mapping):
        for response in compiler.get("response_files", ()):
            if isinstance(response, Mapping) and isinstance(response.get("path"), str):
                paths.append(str(response["path"]))
    for dependency in observation.get("dependencies", ()):
        if isinstance(dependency, Mapping) and isinstance(dependency.get("path"), str):
            paths.append(str(dependency["path"]))
    manifest = observation.get("manifest")
    if isinstance(manifest, Mapping) and isinstance(manifest.get("path"), str):
        paths.append(str(manifest["path"]))
    with contextlib.ExitStack() as stack:
        for path in dict.fromkeys(paths):
            stack.enter_context(StableReadHandle(Path(path)))
        yield

def vc5_script_log_paths(*, cwd: Path, diagnostic_stem: str) -> tuple[Path, Path]:
    safe_stem = safe_path_component(diagnostic_stem)
    logs_dir = cwd / "logs"
    return logs_dir / f"{safe_stem}.out.log", logs_dir / f"{safe_stem}.err.log"


def run_vc5_script(
    command: str,
    *,
    cwd: Path,
    diagnostic_stem: str = "compile",
) -> CommandScriptResult:
    completed = run_tool_cmd_script(
        command,
        cwd=cwd,
        script_name="_run_vc5_verify.cmd",
        capture_output=True,
    )
    stdout_log, stderr_log = vc5_script_log_paths(
        cwd=cwd,
        diagnostic_stem=diagnostic_stem,
    )
    stdout_log.parent.mkdir(parents=True, exist_ok=True)
    stdout_log.write_text(completed.stdout, encoding="utf-8", errors="replace")
    stderr_log.write_text(completed.stderr, encoding="utf-8", errors="replace")

    if completed.returncode == 0:
        if completed.stdout:
            print(completed.stdout, end="")
        if completed.stderr:
            print(completed.stderr, end="", file=sys.stderr)
    else:
        # vc5-order suppresses ordinary compiler stdout so a successful run
        # emits only its semantic result.  VC5 reports compile errors on that
        # same stream, so mirror both streams to stderr on failure and retain
        # the complete, unmodified captures under the isolated build root.
        if completed.stdout:
            print("VC5 compiler stdout:", file=sys.stderr)
            print(completed.stdout.rstrip("\r\n"), file=sys.stderr)
        if completed.stderr:
            print("VC5 compiler stderr:", file=sys.stderr)
            print(completed.stderr.rstrip("\r\n"), file=sys.stderr)
        print(f"VC5 compiler stdout log: {display_path(stdout_log)}", file=sys.stderr)
        print(f"VC5 compiler stderr log: {display_path(stderr_log)}", file=sys.stderr)
    return completed


def prepare_clean_build_dir(build_root: Path, relative_dir: str) -> Path:
    resolved_build_root = build_root.resolve()
    build_dir = (resolved_build_root / relative_dir).resolve()
    if resolved_build_root != build_dir and resolved_build_root not in build_dir.parents:
        raise ValueError(f"Refusing to clean VC5SP3 build directory outside build root: {build_dir}")
    if build_dir.exists():
        shutil.rmtree(build_dir)
    build_dir.mkdir(parents=True, exist_ok=True)
    return build_dir


def compiler_env_path(target: VerifyTarget, vc5_env: Path) -> Path:
    return resolve_repo_path(target.compiler_env) if target.compiler_env else vc5_env


def target_compile_key(target: VerifyTarget, vc5_env: Path) -> tuple[object, ...]:
    compiler_env = compiler_env_path(target, vc5_env).resolve()
    source_identity = target.source_from or f"<inline>\n{target.source_text}"
    return (
        source_identity.replace("\\", "/"),
        target.source_filename,
        str(compiler_env).replace("\\", "/").lower(),
        target.compiler_flags,
        target.source_compile_profiles,
        target.source_compile_flags,
        target.include_dirs,
        target.generated_files,
        target.check_translation_unit_function_order,
        tuple(
            (
                entry.source_from.replace("\\", "/"),
                tuple(
                    (
                        function.address,
                        function.symbol,
                        function.symbol_regex,
                        function.listing_label_regex,
                    )
                    for function in entry.functions
                ),
            )
            for entry in target.translation_unit_function_order
        ),
    )


def safe_path_component(text: str) -> str:
    safe = re.sub(r"[^A-Za-z0-9_.-]+", "_", text).strip("._")
    return safe[:80] or "target"


def with_target_binary(target: VerifyTarget, target_binary: str | None) -> VerifyTarget:
    if target_binary is None or target_binary == target.target_binary:
        return target
    if target_binary not in reference_image_keys():
        valid = ", ".join(reference_image_keys())
        raise ValueError(f"unknown target binary {target_binary!r}; expected one of: {valid}")
    return replace(target, target_binary=target_binary)


def apply_target_binary_override(
    selections: list[VerifySelection],
    target_binary: str | None,
) -> list[VerifySelection]:
    if target_binary is None:
        return selections
    return [
        VerifySelection(
            target=with_target_binary(selection.target, target_binary),
            functions=selection.functions,
            data_symbols=selection.data_symbols,
        )
        for selection in selections
    ]


def binja_binary_selector(target: VerifyTarget) -> str | None:
    return Path(reference_image(target.target_binary).bndb_path).name


def make_binja_bridge(
    *,
    bridge_url: str,
    bn_call_budget: int,
    target: VerifyTarget,
) -> BinaryNinjaBridge:
    selector = binja_binary_selector(target)
    if selector is None:
        return BinaryNinjaBridge(bridge_url, call_budget=bn_call_budget)
    return BinaryNinjaBridge(bridge_url, call_budget=bn_call_budget, binary=selector)


def target_binary_cli_args(target: VerifyTarget) -> tuple[str, ...]:
    if target.target_binary == DEFAULT_TARGET_BINARY:
        return ()
    return ("--binary", target.target_binary)


def group_selections_by_compile_key(
    selections: list[VerifySelection],
    vc5_env: Path,
) -> list[tuple[tuple[object, ...], list[VerifySelection]]]:
    grouped: dict[tuple[object, ...], list[VerifySelection]] = {}
    order: list[tuple[object, ...]] = []
    for selection in selections:
        key = target_compile_key(selection.target, vc5_env)
        if key not in grouped:
            grouped[key] = []
            order.append(key)
        grouped[key].append(selection)
    return [(key, grouped[key]) for key in order]


def source_from_matches(
    target: VerifyTarget,
    source_from: str,
    *,
    tracked_path_inventory: GitTrackedPathInventory | None = None,
) -> bool:
    if not target_has_compile_work(target) or not target.source_from:
        return False
    target_path = Path(target.source_from)
    supplied_path = Path(source_from)
    if target_path.is_absolute() or supplied_path.is_absolute():
        if not (target_path.is_absolute() and supplied_path.is_absolute()):
            raise ValueError(
                "--source-from must be an exact Git repository-relative path for a "
                "tracked target"
            )
        return target_path.resolve() == supplied_path.resolve()
    inventory = tracked_path_inventory or load_git_tracked_path_inventory(REPO_ROOT)
    target_git_path = resolve_tracked_repository_file(
        target.source_from,
        repository_root=inventory.repository_root,
        inventory=inventory,
        context=f"{target.name}: source_from",
        allowed_suffixes=ORDER_EDIT_PATH_SUFFIXES,
    ).git_path
    supplied_git_path = resolve_tracked_repository_file(
        source_from,
        repository_root=inventory.repository_root,
        inventory=inventory,
        context="--source-from",
        allowed_suffixes=ORDER_EDIT_PATH_SUFFIXES,
    ).git_path
    return target_git_path == supplied_git_path


def selected_targets_for_source_from(
    manifests: list[VerifyTarget],
    source_from: str,
    *,
    tracked_path_inventory: GitTrackedPathInventory | None = None,
) -> list[VerifySelection]:
    needs_inventory = any(
        target.source_from and not Path(target.source_from).is_absolute()
        for target in manifests
    ) and not Path(source_from).is_absolute()
    inventory = (
        tracked_path_inventory or load_git_tracked_path_inventory(REPO_ROOT)
        if needs_inventory
        else None
    )
    return [
        VerifySelection(target=manifest, functions=manifest.functions, data_symbols=manifest.data_symbols)
        for manifest in manifests
        if source_from_matches(
            manifest,
            source_from,
            tracked_path_inventory=inventory,
        )
    ]


def selected_targets_for_all(manifests: list[VerifyTarget]) -> list[VerifySelection]:
    return [
        VerifySelection(target=manifest, functions=manifest.functions, data_symbols=manifest.data_symbols)
        for manifest in manifests
        if target_has_compile_work(manifest)
    ]


def target_has_compile_work(target: VerifyTarget) -> bool:
    return bool(target.functions or target.data_symbols or target.translation_unit_function_order)


def selected_targets_covering_address(manifests: list[VerifyTarget], address: str) -> list[VerifySelection]:
    function_selections = [
        VerifySelection(target=manifest, functions=(function,))
        for manifest, function in covering_targets(manifests, address)
    ]
    data_selections = [
        VerifySelection(target=manifest, functions=(), data_symbols=(data_symbol,))
        for manifest, data_symbol in covering_data_symbols(manifests, address)
    ]
    return [*function_selections, *data_selections]


def selector_dedup_key(selector: str) -> str:
    stripped = selector.strip()
    if stripped.lower().startswith("0x"):
        return normalize_address(stripped)
    return stripped


def parse_targets_json(payload: str) -> list[str]:
    try:
        data = json.loads(payload)
    except json.JSONDecodeError as exc:
        raise ValueError(f"--targets-json: invalid JSON: {exc}") from exc
    if isinstance(data, dict):
        data = data.get("targets")
    if not isinstance(data, list):
        raise ValueError("--targets-json: expected JSON list or object with a targets list")
    targets = [str(item).strip() for item in data]
    if any(not item for item in targets):
        raise ValueError("--targets-json entries must be non-empty target strings")
    if not targets:
        raise ValueError("--targets-json requires at least one target")
    return targets


def load_explicit_selectors(args: argparse.Namespace) -> list[str]:
    selectors = [str(target).strip() for target in args.targets]
    selectors.extend(str(target).strip() for target in args.target_aliases)
    if any(not selector for selector in selectors):
        raise ValueError("verify vc5 selectors must be non-empty")
    if args.targets_json:
        selectors.extend(parse_targets_json(args.targets_json))
    keys = [selector_dedup_key(selector) for selector in selectors]
    if len(set(keys)) != len(keys):
        raise ValueError("verify vc5 received duplicate selectors")
    return selectors


def selections_for_explicit_selectors(
    manifests: list[VerifyTarget],
    selectors: list[str],
) -> list[VerifySelection]:
    selections: list[VerifySelection] = []
    for selector in selectors:
        target, functions, data_symbols, _resolved = find_target(manifests, selector)
        selections.append(
            VerifySelection(
                target=target,
                functions=functions,
                data_symbols=data_symbols,
            )
        )
    return selections


def owner_by_selector(
    owner_doc: SourceOwnerDocument,
    entry_index: OwnerEntryIndex,
    selector: str,
) -> SourceOwner:
    if selector.lower().startswith("0x"):
        address = normalize_address(selector)
        entry = entry_index.entries.get(address)
        if entry is None:
            raise ValueError(f"{address}: no owner-entry entry exists for owner-scoped VC5 verification")
        owners = primary_owners_for_entry(owner_doc, entry)
        if not owners:
            raise ValueError(f"{address}: no primary source owner link exists")
        if len(owners) > 1:
            owner_ids = ", ".join(owner.id for owner in owners)
            raise ValueError(f"{address}: multiple primary source owner links: {owner_ids}")
        return owners[0]
    return owner_doc.owner(selector)


def manifest_coverage_indexes(
    manifests: list[VerifyTarget],
) -> tuple[dict[str, list[tuple[VerifyTarget, VerifyFunction]]], tuple[tuple[VerifyTarget, VerifyDataSymbol], ...]]:
    function_index: dict[str, list[tuple[VerifyTarget, VerifyFunction]]] = {}
    data_symbols: list[tuple[VerifyTarget, VerifyDataSymbol]] = []
    for manifest in manifests:
        for function in manifest.functions:
            function_index.setdefault(function.address, []).append((manifest, function))
        for data_symbol in manifest.data_symbols:
            data_symbols.append((manifest, data_symbol))
    return function_index, tuple(data_symbols)


@lru_cache(maxsize=None)
def reference_section_for_address(binary: str, address: str) -> str | None:
    try:
        image = reference_image(binary)
        reference_path = repo_path(image.reference_path)
        headers = parse_pe_headers(reference_path.read_bytes(), source=str(reference_path))
    except Exception:
        return None
    value = int(normalize_address(address), 16)
    for section in headers.sections:
        start = headers.image_base + section.virtual_address
        end = start + max(section.virtual_size, section.raw_size)
        if start <= value < end:
            return section.name
    return None


def owner_entries_strict(owner: SourceOwner, entry_index: OwnerEntryIndex) -> tuple[tuple[OwnerEntry, ...], list[OwnerVc5Issue]]:
    owner_addresses = owner.addresses()
    issues: list[OwnerVc5Issue] = []
    for address in sorted(owner_addresses):
        if address not in entry_index.entries:
            issues.append(
                OwnerVc5Issue(
                    address=address,
                    kind="owner-entry-missing",
                    message=f"{address}: linked owner address has no owner-entry entry",
                )
            )
    entries = tuple(
        entry_index.entries[address]
        for address in entry_index.order
        if address.lower() in owner_addresses
    )
    if not entries:
        issues.append(
            OwnerVc5Issue(
                address="-",
                kind="owner-empty",
                message=f"{owner.id}: source owner has no linked owner-entry entries",
            )
        )
    for entry in entries:
        if entry.is_provider_boundary:
            issues.append(
                OwnerVc5Issue(
                    address=entry.address,
                    kind="provider-boundary",
                    message=f"{entry.address}: provider-boundary row is not authored owner VC5 scope",
                )
            )
        if entry.is_retired_data_entry:
            issues.append(
                OwnerVc5Issue(
                    address=entry.address,
                    kind="retired-data",
                    message=f"{entry.address}: retired data row is not owner VC5 scope",
                )
            )
    return entries, issues


def untracked_rdata_data_symbol_matches(
    *,
    owner: SourceOwner,
    entry_index: OwnerEntryIndex,
    data_symbols: tuple[tuple[VerifyTarget, VerifyDataSymbol], ...],
    binary: str,
) -> tuple[dict[str, tuple[VerifyTarget, VerifyDataSymbol]], set[str], list[OwnerVc5Issue]]:
    matches_by_address: dict[str, tuple[VerifyTarget, VerifyDataSymbol]] = {}
    covered_addresses: set[str] = set()
    issues: list[OwnerVc5Issue] = []
    for relationship in owner_data_address_records(owner):
        address = relationship.address
        if address in entry_index.entries:
            continue
        if reference_section_for_address(binary, address) != ".rdata":
            continue
        matches = [
            (target, data_symbol)
            for target, data_symbol in data_symbols
            if data_symbol_covers_address(data_symbol, address)
        ]
        if not matches:
            continue
        covered_addresses.add(address)
        if len(matches) > 1:
            names = ", ".join(target.name for target, _item in matches)
            issues.append(
                OwnerVc5Issue(
                    address=address,
                    kind="multiple-coverage",
                    message=f"{address}: multiple VC5 manifests cover non-canonical .rdata owner evidence: {names}",
                )
            )
            continue
        matches_by_address[address] = matches[0]
    return matches_by_address, covered_addresses, issues


def resolve_owner_vc5_scope(
    *,
    owner_doc: SourceOwnerDocument,
    entry_index: OwnerEntryIndex,
    manifests: list[VerifyTarget],
    owner_selector: str,
    binary: str = DEFAULT_TARGET_BINARY,
) -> OwnerVc5Scope:
    owner = owner_by_selector(owner_doc, entry_index, owner_selector)
    entries, issues = owner_entries_strict(owner, entry_index)
    function_index, data_symbols = manifest_coverage_indexes(manifests)
    untracked_rdata_matches, untracked_rdata_covered, untracked_rdata_issues = untracked_rdata_data_symbol_matches(
        owner=owner,
        entry_index=entry_index,
        data_symbols=data_symbols,
        binary=binary,
    )
    if untracked_rdata_covered:
        issues = [
            issue
            for issue in issues
            if not (issue.kind == "owner-entry-missing" and issue.address in untracked_rdata_covered)
        ]
    issues.extend(untracked_rdata_issues)
    selected_functions: dict[str, list[VerifyFunction]] = {}
    selected_data_symbols: dict[str, list[VerifyDataSymbol]] = {}
    targets_by_name: dict[str, VerifyTarget] = {}

    for target, data_symbol in untracked_rdata_matches.values():
        targets_by_name[target.name] = target
        bucket = selected_data_symbols.setdefault(target.name, [])
        if data_symbol not in bucket:
            bucket.append(data_symbol)

    for entry in entries:
        if entry.is_provider_boundary or entry.is_retired_data_entry:
            continue
        correct_matches = (
            [
                (target, data_symbol)
                for target, data_symbol in data_symbols
                if data_symbol_covers_entry(data_symbol, entry)
            ]
            if entry.is_data_entry
            else function_index.get(entry.address, [])
        )
        wrong_matches = (
            function_index.get(entry.address, [])
            if entry.is_data_entry
            else [
                (target, data_symbol)
                for target, data_symbol in data_symbols
                if data_symbol_covers_address(data_symbol, entry.address)
            ]
        )
        expected_kind = "data_symbols" if entry.is_data_entry else "functions"
        wrong_kind = "functions" if entry.is_data_entry else "data_symbols"
        if not correct_matches:
            if wrong_matches:
                names = ", ".join(target.name for target, _item in wrong_matches)
                issues.append(
                    OwnerVc5Issue(
                        address=entry.address,
                        kind="wrong-kind",
                        message=(
                            f"{entry.address}: VC5 coverage exists only as {wrong_kind}, "
                            f"expected {expected_kind}: {names}"
                        ),
                    )
                )
            else:
                issues.append(
                    OwnerVc5Issue(
                        address=entry.address,
                        kind="missing-coverage",
                        message=f"{entry.address}: no VC5 {expected_kind} manifest coverage",
                    )
                )
            continue
        if len(correct_matches) > 1 and entry.functional_target and entry.functional_target != "pending":
            target_matches = [
                (target, item)
                for target, item in correct_matches
                if target.name == entry.functional_target
            ]
            if target_matches:
                correct_matches = target_matches
        if len(correct_matches) > 1:
            names = ", ".join(target.name for target, _item in correct_matches)
            issues.append(
                OwnerVc5Issue(
                    address=entry.address,
                    kind="multiple-coverage",
                    message=f"{entry.address}: multiple VC5 manifests cover the owner row: {names}",
                )
            )
            continue

        target, item = correct_matches[0]
        targets_by_name[target.name] = target
        if entry.is_data_entry:
            bucket = selected_data_symbols.setdefault(target.name, [])
            if item not in bucket:
                bucket.append(item)  # type: ignore[arg-type]
        else:
            selected_functions.setdefault(target.name, []).append(item)  # type: ignore[arg-type]

    selections = tuple(
        VerifySelection(
            target=target,
            functions=tuple(selected_functions.get(target_name, ())),
            data_symbols=tuple(selected_data_symbols.get(target_name, ())),
        )
        for target_name, target in sorted(targets_by_name.items())
    )
    return OwnerVc5Scope(
        owner=owner,
        required_entries=entries,
        selections=selections,
        issues=tuple(issues),
    )


def owner_verify_command(
    *,
    owner_id: str,
    binary: str,
    auto_chunk: bool = True,
) -> str:
    args: list[str | Path] = ["python", Path("tools") / "recoil.py", "verify", "vc5"]
    if binary != DEFAULT_TARGET_BINARY:
        args.extend(["--binary", binary])
    args.extend(["--owner", owner_id])
    if auto_chunk:
        args.append("--auto-chunk")
    return " ".join(response_line(arg) for arg in args)


def print_owner_vc5_scope(scope: OwnerVc5Scope, *, binary: str) -> None:
    print("owner_vc5_scope")
    print(f"owner_id: {scope.owner.id}")
    print(f"kind: {scope.owner.kind}")
    print(f"binary: {binary}")
    print(f"linked_owner_entries: {len(scope.required_entries)}")
    print(f"function_entries: {scope.function_entry_count}")
    print(f"data_entries: {scope.data_entry_count}")
    print(f"selected_targets: {len(scope.selections)}")
    print(f"selected_items: {scope.selected_item_count}")
    print(f"missing_or_invalid: {len(scope.issues)}")
    print(f"owner_verify_command: {owner_verify_command(owner_id=scope.owner.id, binary=binary)}")
    if not scope.selections:
        print("selected_target_names: -")
    else:
        print("selected_target_names: " + ", ".join(selection.target.name for selection in scope.selections))
    if scope.issues:
        print("coverage_issues:")
        for issue in scope.issues:
            print(f"- {issue.address} {issue.kind}: {issue.message}")
        explain_addresses = [
            issue.address
            for issue in scope.issues
            if issue.address.startswith("0x") and issue.kind in {"missing-coverage", "wrong-kind", "multiple-coverage"}
        ]
        if explain_addresses:
            print("missing_guidance:")
            for address in explain_addresses:
                print(f"- python tools/recoil.py verify vc5 --explain-missing {address}")


def write_generated_files(target: VerifyTarget, build_dir: Path) -> None:
    for relative_path, contents in target.generated_files:
        generated_path = build_dir / relative_path
        generated_path.parent.mkdir(parents=True, exist_ok=True)
        generated_path.write_text(contents, encoding="ascii")


def write_compile_inputs(target: VerifyTarget, build_dir: Path) -> Path:
    source_path = build_dir / target.source_filename
    source_path.parent.mkdir(parents=True, exist_ok=True)
    if target.source_from:
        shutil.copyfile(resolve_repo_path(target.source_from), source_path)
    else:
        source_path.write_text(target.source_text, encoding="ascii")

    write_generated_files(target, build_dir)
    return source_path


def translation_unit_source_path(
    *,
    build_dir: Path,
    source_from: str,
    index: int,
) -> Path:
    original_name = Path(source_from.replace("\\", "/")).name
    safe_name = safe_path_component(original_name)
    if not Path(safe_name).suffix:
        safe_name = f"{safe_name}.cpp"
    return build_dir / "_tu_order" / f"{index:02d}_{safe_name}"


def compile_translation_unit_order(
    *,
    target: VerifyTarget,
    build_dir: Path,
    compiler_env: Path,
    capture_verification_receipt: bool = False,
) -> tuple[tuple[CompiledTranslationUnit, ...], int]:
    write_generated_files(target, build_dir)
    compiled_units: list[CompiledTranslationUnit] = []
    for index, entry in enumerate(target.translation_unit_function_order):
        if target.compile_context_from:
            # A final-build context names the physical compile host and its
            # exact include search order.  Compile that host in place so
            # quote-includes and __FILE__ observe the same source location as
            # the final build instead of a verifier staging copy.
            source_path = resolve_repo_path(entry.source_from)
        else:
            source_path = translation_unit_source_path(
                build_dir=build_dir,
                source_from=entry.source_from,
                index=index,
            )
            source_path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(resolve_repo_path(entry.source_from), source_path)

        effective_profile, effective_flags = effective_source_compile_context(
            target,
            entry.source_from,
        )
        compile_cmd = build_compile_command(
            target,
            source_path,
            compiler_env,
            build_dir,
            source_from=entry.source_from,
        )
        pre_observation: dict[str, Any] = {}
        if capture_verification_receipt:
            pre_version = detect_compiler_version(compiler_env, cwd=build_dir)
            pre_observation = build_compiler_receipt(
                target=target,
                source_path=source_path,
                source_from=entry.source_from,
                manifest_index=index,
                compiler_env=compiler_env,
                build_dir=build_dir,
                compiler_version=pre_version,
            )
            if pre_observation["verification_eligible"] is not True:
                print(
                    "VC5 toolchain/invocation observation is unavailable: "
                    + "; ".join(pre_observation["ineligibility_reasons"]),
                    file=sys.stderr,
                )
                return tuple(compiled_units), 4
        source_stem = Path(entry.source_from.replace("\\", "/")).stem
        with (
            bind_compiler_observation(pre_observation)
            if capture_verification_receipt
            else contextlib.nullcontext()
        ):
            completed = run_vc5_script(
                compile_cmd,
                cwd=build_dir,
                diagnostic_stem=f"translation-unit-{index:02d}-{source_stem}",
            )
        if completed.returncode != 0:
            return tuple(compiled_units), completed.returncode
        compiler_receipt: Mapping[str, Any] = {}
        if capture_verification_receipt:
            post_version = detect_compiler_version(compiler_env, cwd=build_dir)
            post_observation = build_compiler_receipt(
                target=target,
                source_path=source_path,
                source_from=entry.source_from,
                manifest_index=index,
                compiler_env=compiler_env,
                build_dir=build_dir,
                compiler_version=post_version,
            )
            compiler_receipt = compiler_receipt_stability(
                pre_observation, post_observation
            )
            if compiler_receipt["verification_eligible"] is not True:
                print(
                    "VC5 toolchain/invocation drift made the compilation "
                    "ineligible: "
                    + "; ".join(compiler_receipt["ineligibility_reasons"]),
                    file=sys.stderr,
                )
                return tuple(compiled_units), 4

        cod_path = source_path.with_suffix(".cod")
        if not cod_path.exists():
            root_cod_path = build_dir / f"{source_path.stem}.cod"
            if root_cod_path.exists():
                cod_path = root_cod_path
        if not cod_path.exists():
            print(f"Expected COD listing was not emitted: {cod_path}", file=sys.stderr)
            return tuple(compiled_units), 3
        obj_path = source_path.with_suffix(".obj")
        if not obj_path.exists():
            root_obj_path = build_dir / f"{source_path.stem}.obj"
            if root_obj_path.exists():
                obj_path = root_obj_path
        if not obj_path.exists():
            print(f"Expected COFF object was not emitted: {obj_path}", file=sys.stderr)
            return tuple(compiled_units), 3
        compiled_units.append(
            CompiledTranslationUnit(
                manifest_index=index,
                source_from=entry.source_from,
                source_path=source_path,
                cod_path=cod_path,
                obj_path=obj_path,
                effective_compiler_profile=effective_profile,
                effective_compiler_flags=effective_flags,
                compile_command=compile_cmd,
                compiler_receipt=compiler_receipt,
            )
        )
    return tuple(compiled_units), 0


def detect_compiler_version(compiler_env: Path, *, cwd: Path) -> str:
    completed = run_tool_cmd_script(
        f"call {quote_cmd_arg(compiler_env)} && cl",
        cwd=cwd,
        script_name="_detect_vc5_version.cmd",
        capture_output=True,
    )
    output = "\n".join((completed.stdout, completed.stderr))
    for line in output.splitlines():
        if "Compiler Version" in line:
            return line.strip()
    return "<not detected>"


def compile_target(
    *,
    target: VerifyTarget,
    build_dir: Path,
    vc5_env: Path,
    capture_verification_receipt: bool = False,
) -> tuple[CompiledTarget | None, int]:
    compile_in_place = bool(target.compile_context_from and target.source_from)
    if compile_in_place:
        # Keep the final-build physical host and quote-include context intact.
        # VC5 still emits the object/listing in build_dir because that is the
        # compiler working directory.
        source_path = resolve_repo_path(target.source_from)
        write_generated_files(target, build_dir)
    else:
        source_path = write_compile_inputs(target, build_dir)

    compiler_env = compiler_env_path(target, vc5_env)
    if not compiler_env.exists():
        print(f"VC environment not found: {compiler_env}", file=sys.stderr)
        return None, 2

    effective_profile, effective_flags = effective_source_compile_context(
        target,
        target.source_from,
    )
    compile_cmd = build_compile_command(
        target,
        source_path,
        compiler_env,
        build_dir,
        source_from=target.source_from,
    )
    pre_observation: dict[str, Any] = {}
    if capture_verification_receipt:
        pre_version = detect_compiler_version(compiler_env, cwd=build_dir)
        pre_observation = build_compiler_receipt(
            target=target,
            source_path=source_path,
            source_from=target.source_from,
            manifest_index=0,
            compiler_env=compiler_env,
            build_dir=build_dir,
            compiler_version=pre_version,
        )
        if pre_observation["verification_eligible"] is not True:
            print(
                "VC5 toolchain/invocation observation is unavailable: "
                + "; ".join(pre_observation["ineligibility_reasons"]),
                file=sys.stderr,
            )
            return None, 4
    with (
        bind_compiler_observation(pre_observation)
        if capture_verification_receipt
        else contextlib.nullcontext()
    ):
        completed = run_vc5_script(
            compile_cmd,
            cwd=build_dir,
            diagnostic_stem=f"compile-{source_path.stem}",
        )
    if completed.returncode != 0:
        return None, completed.returncode

    cod_path = build_dir / f"{source_path.stem}.cod" if compile_in_place else source_path.with_suffix(".cod")
    if not cod_path.exists():
        print(f"Expected COD listing was not emitted: {cod_path}", file=sys.stderr)
        return None, 3
    obj_path = build_dir / f"{source_path.stem}.obj" if compile_in_place else source_path.with_suffix(".obj")
    if not obj_path.exists():
        print(f"Expected COFF object was not emitted: {obj_path}", file=sys.stderr)
        return None, 3

    compiler_version = detect_compiler_version(compiler_env, cwd=build_dir)
    compiler_receipt: Mapping[str, Any] = {}
    if capture_verification_receipt:
        post_observation = build_compiler_receipt(
            target=target,
            source_path=source_path,
            source_from=target.source_from,
            manifest_index=0,
            compiler_env=compiler_env,
            build_dir=build_dir,
            compiler_version=compiler_version,
        )
        compiler_receipt = compiler_receipt_stability(
            pre_observation, post_observation
        )
        if compiler_receipt["verification_eligible"] is not True:
            print(
                "VC5 toolchain/invocation drift made the compilation "
                "ineligible: "
                + "; ".join(compiler_receipt["ineligibility_reasons"]),
                file=sys.stderr,
            )
            return None, 4
    return (
        CompiledTarget(
            target=target,
            build_dir=build_dir,
            source_path=source_path,
            cod_path=cod_path,
            obj_path=obj_path,
            compiler_env=compiler_env,
            compiler_version=compiler_version,
            compile_command=compile_cmd,
            effective_compiler_profile=effective_profile,
            effective_compiler_flags=effective_flags,
            compiler_receipt=compiler_receipt,
        ),
        0,
    )


def function_tracker_identity(target: VerifyTarget, function: VerifyFunction) -> str:
    """Return the stable tracker-facing identity represented by a manifest row."""
    return function.logical_identity_key or f"{target.target_binary}:function:{function.address}"


def _order_scope(target: VerifyTarget) -> str:
    if target.check_translation_unit_function_order:
        scopes = {entry.order_scope for entry in target.translation_unit_function_order}
        if len(scopes) != 1:
            raise ValueError(
                f"{target.name}: translation-unit order target must use one order_scope"
            )
        return next(iter(scopes))
    if target.check_function_order:
        return target.function_order_scope
    raise ValueError(f"{target.name!r} is not a function-order target")


def _gates_order(function: VerifyFunction, scope: str) -> bool:
    return (
        function_authored_relative_order_gate(function)
        if scope == "authored"
        else function.full_order_gate
    )


def _live_order_sequences(
    order_check: FunctionOrderCheck,
) -> tuple[list[str], list[str], int, dict[str, Any] | None]:
    target = order_check.target
    scope = order_check.order_scope
    expected_functions = [
        function
        for function in order_check.expected_functions
        if function_required_in_scope(function, scope) and _gates_order(function, scope)
    ]
    expected_sequence = [
        function_tracker_identity(target, function) for function in expected_functions
    ]
    candidate_rows = sorted(
        (
            row
            for row in order_check.rows
            if function_required_in_scope(row.function, scope)
            and _gates_order(row.function, scope)
        ),
        key=lambda row: (
            row.object_index,
            row.section_number,
            row.value,
            row.manifest_index,
        ),
    )
    candidate_sequence = [
        function_tracker_identity(target, row.function) for row in candidate_rows
    ]
    matched_prefix_count = 0
    for expected, candidate in zip(expected_sequence, candidate_sequence):
        if expected != candidate:
            break
        matched_prefix_count += 1

    expected_counts = {identity: expected_sequence.count(identity) for identity in expected_sequence}
    candidate_counts = {identity: candidate_sequence.count(identity) for identity in candidate_sequence}
    first_divergence: dict[str, Any] | None = None

    if order_check.blocking_diagnostics:
        message = order_check.blocking_diagnostics[0]
        manifest_match = re.search(r"manifest#([0-9]+)", message)
        manifest_index = int(manifest_match.group(1)) if manifest_match else None
        identity = None
        if manifest_index is not None and manifest_index < len(order_check.expected_functions):
            identity = function_tracker_identity(
                target,
                order_check.expected_functions[manifest_index],
            )
        kind = "duplicate" if "same defined-function alias group" in message else "missing"
        first_divergence = {
            "kind": kind,
            "index": matched_prefix_count,
            "expected_identity": identity,
            "candidate_identity": (
                candidate_sequence[matched_prefix_count]
                if matched_prefix_count < len(candidate_sequence)
                else None
            ),
            "message": message,
            "expected_neighbors": expected_sequence[
                max(0, matched_prefix_count - 1):matched_prefix_count + 2
            ],
            "candidate_neighbors": candidate_sequence[
                max(0, matched_prefix_count - 1):matched_prefix_count + 2
            ],
        }
    else:
        for index, identity in enumerate(candidate_sequence):
            if candidate_counts[identity] > expected_counts.get(identity, 0):
                first_divergence = {
                    "kind": "duplicate" if identity in expected_counts else "extra",
                    "index": index,
                    "expected_identity": (
                        expected_sequence[index] if index < len(expected_sequence) else None
                    ),
                    "candidate_identity": identity,
                    "message": f"candidate emits unexpected identity {identity} at position {index}",
                    "expected_neighbors": expected_sequence[max(0, index - 1):index + 2],
                    "candidate_neighbors": candidate_sequence[max(0, index - 1):index + 2],
                }
                break
        if first_divergence is None:
            for index, identity in enumerate(expected_sequence):
                if expected_counts[identity] > candidate_counts.get(identity, 0):
                    first_divergence = {
                        "kind": "missing",
                        "index": index,
                        "expected_identity": identity,
                        "candidate_identity": (
                            candidate_sequence[index] if index < len(candidate_sequence) else None
                        ),
                        "message": f"candidate is missing required identity {identity}",
                        "expected_neighbors": expected_sequence[max(0, index - 1):index + 2],
                        "candidate_neighbors": candidate_sequence[max(0, index - 1):index + 2],
                    }
                    break
        if first_divergence is None and candidate_sequence != expected_sequence:
            index = matched_prefix_count
            first_divergence = {
                "kind": "reordered",
                "index": index,
                "expected_identity": (
                    expected_sequence[index] if index < len(expected_sequence) else None
                ),
                "candidate_identity": (
                    candidate_sequence[index] if index < len(candidate_sequence) else None
                ),
                "message": "candidate identities are not in retail relative order",
                "expected_neighbors": expected_sequence[max(0, index - 1):index + 2],
                "candidate_neighbors": candidate_sequence[max(0, index - 1):index + 2],
            }

    return expected_sequence, candidate_sequence, matched_prefix_count, first_divergence


def apply_source_fragment_order_gate(
    *,
    target: VerifyTarget,
    expected: list[str],
    candidate: list[str],
    divergence: dict[str, Any] | None,
    passed: bool,
) -> tuple[bool, dict[str, Any] | None]:
    """Preserve order divergence feedback; block only an otherwise matching order."""

    fragment_findings = target_source_fragment_findings(target)
    if not passed or not fragment_findings:
        return passed, divergence
    first = fragment_findings[0]
    location = first.get("path") or (
        f"{first.get('source')}:{first.get('line')} -> {first.get('target')}"
    )
    return False, {
        "kind": "source-fragment-blocker",
        "message": (
            f"order matches, but {len(fragment_findings)} forbidden production "
            f"source-fragment closure finding(s) remain; first is {first['kind']}: {location}"
        ),
        "expected_neighbors": expected[max(0, len(expected) - 2):] or ["<none>"],
        "candidate_neighbors": candidate[max(0, len(candidate) - 2):] or ["<none>"],
        "source_fragment_findings": list(fragment_findings),
    }


def live_order_result(
    *,
    target: VerifyTarget,
    build_root: Path,
    vc5_env: Path,
) -> dict[str, Any]:
    """Freshly compile one target and return live semantic order facts."""
    scope = _order_scope(target)
    if not target.retail_start or not target.retail_end_exclusive:
        raise ValueError(f"{target.name}: live order target requires an exact retail interval")
    build_dir = prepare_clean_build_dir(build_root, target.name)
    compiler_env = compiler_env_path(target, vc5_env)
    if not compiler_env.is_file():
        raise ValueError(f"VC environment not found: {compiler_env}")

    if target.check_translation_unit_function_order:
        compiled_units, rc = compile_translation_unit_order(
            target=target,
            build_dir=build_dir,
            compiler_env=compiler_env,
        )
        if rc != 0:
            raise ValueError(f"VC5 translation-unit compilation failed with exit code {rc}")
        coff_objects = tuple(CoffObject.from_path(unit.obj_path) for unit in compiled_units)
        cod_indexes = tuple(
            parse_cod_listing_label_index(unit.cod_path, coff_object)
            for unit, coff_object in zip(compiled_units, coff_objects)
        )
        order_check = check_translation_unit_function_order(
            target=target,
            coff_objects=coff_objects,
            cod_label_indexes=cod_indexes,
        )
    else:
        compiled, rc = compile_target(target=target, build_dir=build_dir, vc5_env=vc5_env)
        if rc != 0 or compiled is None:
            raise ValueError(f"VC5 compilation failed with exit code {rc or 3}")
        order_check = check_function_order(
            target=target,
            functions=target.functions,
            coff_object=CoffObject.from_path(compiled.obj_path),
        )

    expected, candidate, prefix, divergence = _live_order_sequences(order_check)
    passed = order_check.ok and divergence is None
    passed, divergence = apply_source_fragment_order_gate(
        target=target,
        expected=expected,
        candidate=candidate,
        divergence=divergence,
        passed=passed,
    )
    return {
        "kind": "vc5-order-live-result",
        "target_id": target.name,
        "phase": (
            "authored-function-order" if scope == "authored" else "full-function-order"
        ),
        "physical_block_id": f"{target.target_binary}:block:{target.retail_start}",
        "passed": passed,
        "expected_sequence": expected,
        "candidate_sequence": candidate,
        "matched_prefix_count": prefix,
        "first_divergence": divergence,
    }


def _validate_linked_feedback_pair(
    object_target: VerifyTarget,
    linked_target: VerifyTarget,
) -> LinkedFunctionInterval:
    """Require one authored object projection paired with one exact full linked interval."""

    if _order_scope(object_target) != "authored":
        raise ValueError(
            f"{object_target.name}: --linked-target requires an authored object-order target"
        )
    if not object_target.compile_context_from:
        raise ValueError(
            f"{object_target.name}: --linked-target requires compile_context_from for the "
            "whole-project build configuration"
        )
    if object_target.target_binary != linked_target.target_binary:
        raise ValueError(
            f"linked feedback target binary {linked_target.target_binary!r} does not match "
            f"object target binary {object_target.target_binary!r}"
        )
    intervals = tuple(
        interval
        for interval in linked_target.linked_function_intervals
        if interval.order_scope == "full"
    )
    if len(intervals) != 1 or len(linked_target.linked_function_intervals) != 1:
        raise ValueError(
            f"{linked_target.name}: --linked-target requires one exact full linked interval"
        )
    interval = intervals[0]
    object_bounds = (object_target.retail_start, object_target.retail_end_exclusive)
    linked_bounds = (linked_target.retail_start, linked_target.retail_end_exclusive)
    interval_bounds = (interval.retail_start, interval.retail_end_exclusive)
    if not all(object_bounds) or object_bounds != linked_bounds or object_bounds != interval_bounds:
        raise ValueError(
            f"linked feedback interval {interval_bounds} does not match object target interval "
            f"{object_bounds} and linked target interval {linked_bounds}"
        )
    return interval


def _actionable_linked_divergence(value: Any, *, passed: bool) -> dict[str, Any] | None:
    if passed:
        if value is not None:
            raise ValueError("passing linked feedback report must not contain first_divergence")
        return None
    if not isinstance(value, dict):
        raise ValueError("failing linked feedback report lacks a typed first_divergence")
    kind = value.get("kind")
    message = value.get("message")
    expected_neighbors = value.get("expected_neighbors")
    candidate_neighbors = value.get("candidate_neighbors")
    if (
        not isinstance(kind, str)
        or not kind
        or not isinstance(message, str)
        or not message
        or not isinstance(expected_neighbors, list)
        or not expected_neighbors
        or not isinstance(candidate_neighbors, list)
        or not candidate_neighbors
    ):
        raise ValueError(
            "failing linked feedback divergence requires kind, message, and both neighbor windows"
        )
    return dict(value)


def live_linked_feedback_result(
    *,
    object_target: VerifyTarget,
    linked_target: VerifyTarget,
    object_result: dict[str, Any],
    build_root: Path,
    progress_path: Path,
) -> dict[str, Any]:
    """Run exact full linked-order feedback after the compiling object projection passes."""

    interval = _validate_linked_feedback_pair(object_target, linked_target)
    if object_result.get("passed") is not True:
        raise ValueError("linked feedback cannot run before the object-order projection passes")
    root = build_root if build_root.is_absolute() else REPO_ROOT / build_root
    linked_root = (root / "_linked").resolve()
    try:
        linked_relative = linked_root.relative_to(REPO_ROOT.resolve())
    except ValueError as exc:
        raise ValueError("linked feedback build root must remain below the repository") from exc
    if len(linked_relative.parts) < 2 or linked_relative.parts[0].casefold() != "build":
        raise ValueError("linked feedback build root must name an isolated child below build/")

    command = [
        sys.executable,
        str(REPO_ROOT / "tools" / "recoil.py"),
        "verify",
        "linked-order",
        linked_target.name,
        "--scope",
        "full",
        "--build-root",
        linked_relative.as_posix(),
        "--manifest",
        object_target.compile_context_from,
        "--progress",
        str(progress_path),
    ]
    completed = subprocess.run(
        command,
        cwd=REPO_ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    returncode = completed.returncode

    summary_path = linked_root / "summary.json"
    if not summary_path.is_file():
        detail = completed.stderr.strip() or completed.stdout.strip()[-1000:]
        raise ValueError(
            f"linked feedback produced no summary at {display_path(summary_path)}: {detail}"
        )
    summary = json.loads(summary_path.read_text(encoding="utf-8"))
    if not isinstance(summary, dict) or (
        summary.get("kind") != "linked-function-order-run"
        or summary.get("binary") != object_target.target_binary
        or summary.get("order_scope") != "full"
    ):
        raise ValueError("linked feedback summary has the wrong kind, binary, or scope")
    report_rows = summary.get("order_reports")
    if not isinstance(report_rows, list) or len(report_rows) != 1:
        raise ValueError("linked feedback summary must expose one exact report")
    report_value = (
        report_rows[0].get("path") if isinstance(report_rows[0], dict) else None
    )
    if not isinstance(report_value, str) or not report_value:
        raise ValueError("linked feedback summary report path is missing")
    report_path = Path(report_value).resolve()
    try:
        report_path.relative_to(linked_root)
    except ValueError as exc:
        raise ValueError("linked feedback report escaped the isolated build root") from exc
    report = json.loads(report_path.read_text(encoding="utf-8"))
    if not isinstance(report, dict):
        raise ValueError("linked feedback report must be an object")
    required = {
        "kind": "linked-function-order-report",
        "target": linked_target.name,
        "interval": interval.name,
        "order_scope": "full",
        "binary": object_target.target_binary,
        "retail_start": object_target.retail_start,
        "retail_end_exclusive": object_target.retail_end_exclusive,
    }
    for key, expected in required.items():
        if report.get(key) != expected:
            raise ValueError(
                f"linked feedback report {key}={report.get(key)!r}, expected {expected!r}"
            )
    if (
        report.get("linked_exact_selected_population_evaluated") is not True
        or report.get("linked_seams_and_rvas_evaluated") is not True
    ):
        raise ValueError(
            "linked feedback report did not evaluate exact selected population and seams/RVAs"
        )
    passed = report.get("passed") is True
    if bool(summary.get("success")) != passed:
        raise ValueError("linked feedback summary and report disagree on success")
    expected_returncode = 0 if passed else 1
    if returncode != expected_returncode:
        raise ValueError(
            f"linked feedback validator exited {returncode}, expected {expected_returncode}"
        )
    divergence = _actionable_linked_divergence(
        report.get("first_divergence"),
        passed=passed,
    )
    result = dict(object_result)
    result.update(
        {
            "phase": "full-function-order",
            "physical_block_id": (
                f"{object_target.target_binary}:block:{object_target.retail_start}"
            ),
            "passed": passed,
            "first_divergence": divergence,
            "feedback_dimension": "linked-selected-population-and-seams",
            "object_order_passed": True,
            "linked_target_id": linked_target.name,
            "linked_report": display_path(report_path),
            "linked_summary": display_path(summary_path),
        }
    )
    return result


def _linked_neighbor_text(row: Any) -> str:
    if not isinstance(row, dict):
        return str(row)
    address = str(row.get("retail_address") or row.get("linked_address") or "<no-address>")
    identities = row.get("identities")
    if isinstance(identities, list) and identities:
        identity = " | ".join(str(item) for item in identities)
    else:
        identity = str(row.get("identity") or row.get("name") or "<no-identity>")
    providers = row.get("providers")
    provider = (
        f" ({' | '.join(str(item) for item in providers)})"
        if isinstance(providers, list) and providers
        else ""
    )
    return f"{address} {identity}{provider}"


def print_live_order_result(result: dict[str, Any]) -> None:
    if result.get("feedback_dimension") == "linked-selected-population-and-seams":
        print(
            f"{result['phase']} {result['physical_block_id']} "
            f"{'PASS' if result['passed'] else 'FAIL'}; object projection PASS "
            f"{result['matched_prefix_count']}/{len(result['expected_sequence'])}; "
            f"linked target {result['linked_target_id']}"
        )
        divergence = result.get("first_divergence")
        if isinstance(divergence, dict):
            print(
                f"first linked divergence [{divergence['kind']}]: "
                f"{divergence['message']}"
            )
            print("expected neighbors:")
            for row in divergence["expected_neighbors"]:
                print(f"  {_linked_neighbor_text(row)}")
            print("candidate neighbors:")
            for row in divergence["candidate_neighbors"]:
                print(f"  {_linked_neighbor_text(row)}")
        return
    print(
        f"{result['phase']} {result['physical_block_id']} "
        f"{'PASS' if result['passed'] else 'FAIL'}; "
        f"matched prefix {result['matched_prefix_count']}/"
        f"{len(result['expected_sequence'])}"
    )
    if result["first_divergence"] is not None:
        divergence = result["first_divergence"]
        print(f"first divergence [{divergence['kind']}]: {divergence['message']}")
        print("expected neighbors: " + " -> ".join(divergence["expected_neighbors"]))
        print("candidate neighbors: " + " -> ".join(divergence["candidate_neighbors"]))


def print_target_list(manifests: list[VerifyTarget]) -> None:
    for manifest in manifests:
        addresses = ", ".join(
            [function.address for function in manifest.functions]
            + [data_symbol.address for data_symbol in manifest.data_symbols]
            + [
                function.address
                for entry in manifest.translation_unit_function_order
                for function in entry.functions
            ]
        )
        print(f"{manifest.name}: {addresses}")
        print(f"  target_binary: {manifest.target_binary}")
        print(f"  {manifest.description}")


def print_compiled_target_info(target: VerifyTarget, compiled: CompiledTarget) -> None:
    print(f"VC target: {target.name}")
    print(f"Manifest: {target.manifest_path}")
    print(f"BN target binary: {target.target_binary}")
    print(f"Source files: {', '.join(target.source_files) if target.source_files else '<manifest source only>'}")
    if target.check_translation_unit_function_order:
        print(
            "Translation-unit function order sources: "
            + ", ".join(entry.source_from for entry in target.translation_unit_function_order)
        )
    print(f"Compare mode: {target.compare_mode}")
    if target.compiler_profile:
        print(f"Compiler profile: {target.compiler_profile}")
    if target.source_compile_profiles:
        print("Source compiler profiles:")
        for source_key, profile_name in target.source_compile_profiles:
            print(f"- {source_key}: {profile_name}")
    print(f"Compiler env: {compiled.compiler_env}")
    print(f"Compiler version: {compiled.compiler_version}")
    print(
        "Compiler flags: "
        + " ".join(compiled.effective_compiler_flags or target.compiler_flags)
    )
    print(f"VC listing: {compiled.cod_path}")
    if compiled.obj_path.exists():
        print(f"VC object: {compiled.obj_path}")


def compare_compiled_selections(
    *,
    compiled: CompiledTarget,
    selections: list[VerifySelection],
    bridge_url: str,
    bridge: BinaryNinjaBridge | None = None,
    bn_call_budget: int = DEFAULT_BN_CALL_BUDGET,
) -> tuple[list[VerificationResult], int]:
    verify_dir = compiled.build_dir / "verify"
    results: list[VerificationResult] = []
    overall = 0
    shared_bridge = bridge
    shared_bridges: dict[str, BinaryNinjaBridge] = {}
    coff_object: CoffObject | None = None
    if any(function.symbol_regex for selection in selections for function in selection.functions) or any(
        data_symbol.symbol_regex
        for selection in selections
        for data_symbol in selection.data_symbols
    ):
        if not compiled.obj_path.exists():
            print(
                "symbol_regex requires a compiled COFF object for symbol resolution",
                file=sys.stderr,
            )
            return results, 3
        coff_object = CoffObject.from_path(compiled.obj_path)

    for selection in selections:
        target = selection.target
        for function in selection.functions:
            try:
                resolved_function = function
                if function.symbol_regex is not None:
                    assert coff_object is not None
                    resolved_function = replace(
                        function,
                        symbol=resolve_coff_symbol_regex(
                            coff_object,
                            function.symbol_regex,
                            item_label=function.address,
                            require_defined_code=True,
                        ),
                    )

                if shared_bridge is None:
                    key = binja_binary_selector(target) or DEFAULT_TARGET_BINARY
                    if key not in shared_bridges:
                        shared_bridges[key] = make_binja_bridge(
                            bridge_url=bridge_url,
                            bn_call_budget=bn_call_budget,
                            target=target,
                        )
                    bridge_arg = shared_bridges[key]
                else:
                    bridge_arg = shared_bridge
                comparison = compare_bn_to_obj(
                    address=resolved_function.address,
                    obj_path=compiled.obj_path,
                    symbol=resolved_function.symbol,
                    out_dir=verify_dir,
                    bridge_url=bridge_url,
                    bridge=bridge_arg,
                    cod_path=compiled.cod_path,
                    trim_padding_nops=target.trim_trailing_nops,
                    bn_byte_length=resolved_function.bn_byte_length,
                    vc5_byte_length=resolved_function.vc5_byte_length,
                )
                results.append(
                    VerificationResult(
                        target=target,
                        function=resolved_function,
                        item_kind="function",
                        mode="bytes",
                        mismatches=comparison.mismatch_count,
                        relocation_or_text_metric=comparison.relocation_masked_bytes,
                        secondary_metric=comparison.trailing_vc5_nops_trimmed,
                        bn_size_or_normalized=comparison.bn_size,
                        vc5_size_or_diff_count=comparison.vc5_size,
                        evidence_path=comparison.diff_path,
                        triage_path=comparison.triage_path,
                        comparison=comparison,
                    )
                )
            except BridgeBudgetExceeded as exc:
                print(f"{function.address} {function.name}: {exc}", file=sys.stderr)
                print(
                    "Binary Ninja comparison stopped because the active BN call budget was exhausted; "
                    "rerun a narrower target or use --skip-bn-compare for compile-only coverage.",
                    file=sys.stderr,
                )
                return results, 1
            except (BridgeError, ValueError) as exc:
                print(f"{function.address} {function.name}: {exc}", file=sys.stderr)
                overall = 1
                continue
            if results[-1].mismatches:
                overall = 1
        for data_symbol in selection.data_symbols:
            try:
                resolved_data_symbol = data_symbol
                if data_symbol.symbol_regex is not None:
                    assert coff_object is not None
                    resolved_data_symbol = replace(
                        data_symbol,
                        symbol=resolve_coff_symbol_regex(
                            coff_object,
                            data_symbol.symbol_regex,
                            item_label=data_symbol.address,
                        ),
                    )

                if shared_bridge is None:
                    key = binja_binary_selector(target) or DEFAULT_TARGET_BINARY
                    if key not in shared_bridges:
                        shared_bridges[key] = make_binja_bridge(
                            bridge_url=bridge_url,
                            bn_call_budget=bn_call_budget,
                            target=target,
                        )
                    bridge_arg = shared_bridges[key]
                else:
                    bridge_arg = shared_bridge
                comparison = compare_bn_data_to_obj(
                    address=resolved_data_symbol.address,
                    obj_path=compiled.obj_path,
                    symbol=resolved_data_symbol.symbol,
                    byte_length=resolved_data_symbol.byte_length,
                    object_offset=resolved_data_symbol.object_offset,
                    out_dir=verify_dir,
                    bridge_url=bridge_url,
                    bridge=bridge_arg,
                    name=resolved_data_symbol.bn_name or resolved_data_symbol.name,
                )
                results.append(
                    VerificationResult(
                        target=target,
                        function=resolved_data_symbol,
                        item_kind="data",
                        mode="data",
                        mismatches=comparison.mismatch_count,
                        relocation_or_text_metric=comparison.relocation_masked_bytes,
                        secondary_metric=0,
                        bn_size_or_normalized=comparison.bn_size,
                        vc5_size_or_diff_count=comparison.vc5_size,
                        evidence_path=comparison.diff_path,
                        triage_path=comparison.triage_path,
                        comparison=comparison,
                    )
                )
            except BridgeBudgetExceeded as exc:
                print(f"{data_symbol.address} {data_symbol.name}: {exc}", file=sys.stderr)
                print(
                    "Binary Ninja comparison stopped because the active BN call budget was exhausted; "
                    "rerun a narrower target or use --skip-bn-compare for compile-only coverage.",
                    file=sys.stderr,
                )
                return results, 1
            except (BridgeError, ValueError) as exc:
                print(f"{data_symbol.address} {data_symbol.name}: {exc}", file=sys.stderr)
                overall = 1
                continue
            if results[-1].mismatches:
                overall = 1
    return results, overall


def selection_item_count(selections: list[VerifySelection]) -> int:
    return sum(len(selection.functions) + len(selection.data_symbols) for selection in selections)


def selection_data_symbol_count(selections: list[VerifySelection]) -> int:
    return sum(len(selection.data_symbols) for selection in selections)


def should_auto_chunk_by_default(
    *,
    selections: list[VerifySelection],
    skip_bn_compare: bool,
    bn_call_budget: int,
    chunk_size: int | None,
    auto_chunk: bool,
) -> bool:
    if skip_bn_compare:
        return False
    if chunk_size is not None or auto_chunk:
        return False
    if bn_call_budget <= 0:
        return False
    return selection_data_symbol_count(selections) > 1


def chunk_selections_by_item_count(
    selections: list[VerifySelection],
    chunk_size: int,
) -> list[list[VerifySelection]]:
    if chunk_size <= 0:
        raise ValueError("--chunk-size must be a positive integer")

    chunks: list[list[VerifySelection]] = []
    current: list[VerifySelection] = []
    current_count = 0

    def append_item(selection: VerifySelection) -> None:
        nonlocal current, current_count
        if current_count >= chunk_size:
            chunks.append(current)
            current = []
            current_count = 0
        current.append(selection)
        current_count += 1

    for selection in selections:
        for function in selection.functions:
            append_item(
                VerifySelection(
                    target=selection.target,
                    functions=(function,),
                    data_symbols=(),
                )
            )
        for data_symbol in selection.data_symbols:
            append_item(
                VerifySelection(
                    target=selection.target,
                    functions=(),
                    data_symbols=(data_symbol,),
                )
            )

    if current:
        chunks.append(current)
    return chunks


def chunk_selections_by_bn_call_budget(
    *,
    compiled: CompiledTarget,
    selections: list[VerifySelection],
    bn_call_budget: int,
) -> list[list[VerifySelection]]:
    if bn_call_budget <= 0:
        raise ValueError("--auto-chunk requires a positive --bn-call-budget")

    needs_coff = any(
        function.symbol_regex
        for selection in selections
        for function in selection.functions
    ) or any(
        True
        for selection in selections
        for _data_symbol in selection.data_symbols
    )
    coff_object = CoffObject.from_path(compiled.obj_path) if needs_coff else None

    chunks: list[list[VerifySelection]] = []
    current: list[VerifySelection] = []
    current_calls = 0
    current_needs_symbols = False

    def append_item(selection: VerifySelection, base_calls: int, needs_symbols: bool, label: str) -> None:
        nonlocal current, current_calls, current_needs_symbols
        extra_symbols = BN_SYMBOL_LOOKUP_CALL_ESTIMATE if needs_symbols and not current_needs_symbols else 0
        estimated_calls = base_calls + extra_symbols
        if estimated_calls > bn_call_budget:
            raise ValueError(
                f"--auto-chunk cannot fit {label}: estimated {estimated_calls} BN call(s) "
                f"with --bn-call-budget {bn_call_budget}"
            )
        if current and current_calls + estimated_calls > bn_call_budget:
            chunks.append(current)
            current = []
            current_calls = 0
            current_needs_symbols = False
            extra_symbols = BN_SYMBOL_LOOKUP_CALL_ESTIMATE if needs_symbols else 0
            estimated_calls = base_calls + extra_symbols
        current.append(selection)
        current_calls += estimated_calls
        current_needs_symbols = current_needs_symbols or needs_symbols

    for selection in selections:
        for function in selection.functions:
            resolved_function = function
            if function.symbol_regex is not None:
                assert coff_object is not None
                resolved_function = replace(
                    function,
                    symbol=resolve_coff_symbol_regex(
                        coff_object,
                        function.symbol_regex,
                        item_label=function.address,
                        require_defined_code=True,
                    ),
                )
            base_calls = 1 + (1 if resolved_function.bn_byte_length else 0)
            append_item(
                VerifySelection(
                    target=selection.target,
                    functions=(resolved_function,),
                    data_symbols=(),
                ),
                base_calls,
                False,
                f"{resolved_function.address} {resolved_function.name}",
            )

        for data_symbol in selection.data_symbols:
            resolved_data_symbol = data_symbol
            if data_symbol.symbol_regex is not None:
                assert coff_object is not None
                resolved_data_symbol = replace(
                    data_symbol,
                    symbol=resolve_coff_symbol_regex(
                        coff_object,
                        data_symbol.symbol_regex,
                        item_label=data_symbol.address,
                    ),
                )
            base_calls = 1
            needs_symbols = False
            assert coff_object is not None
            data_bytes = coff_object.data_symbol_bytes(
                resolved_data_symbol.symbol,
                byte_length=resolved_data_symbol.byte_length,
                object_offset=resolved_data_symbol.object_offset,
            )
            needs_symbols = bool(data_bytes.relocations)
            append_item(
                VerifySelection(
                    target=selection.target,
                    functions=(),
                    data_symbols=(resolved_data_symbol,),
                ),
                base_calls,
                needs_symbols,
                f"{resolved_data_symbol.address} {resolved_data_symbol.name}",
            )

    if current:
        chunks.append(current)
    return chunks


def compare_compiled_selections_in_chunks(
    *,
    compiled: CompiledTarget,
    selections: list[VerifySelection],
    bridge_url: str,
    bn_call_budget: int,
    chunk_size: int | None,
    auto_chunk: bool = False,
    include_target: bool = False,
) -> tuple[list[VerificationResult], int]:
    if auto_chunk:
        chunks = chunk_selections_by_bn_call_budget(
            compiled=compiled,
            selections=selections,
            bn_call_budget=bn_call_budget,
        )
        chunk_detail = f"auto chunks bounded by BN call budget {bn_call_budget}"
    else:
        if chunk_size is None:
            raise ValueError("--chunk-size is required unless --auto-chunk is active")
        chunks = chunk_selections_by_item_count(selections, chunk_size)
        chunk_detail = f"chunk size {chunk_size}, BN call budget {bn_call_budget} per chunk"
    total_items = selection_item_count(selections)
    print(
        f"VC compare chunks: {len(chunks)} chunk(s), {total_items} item(s), "
        f"{chunk_detail}."
    )

    all_results: list[VerificationResult] = []
    overall = 0
    for index, chunk in enumerate(chunks, start=1):
        chunk_items = selection_item_count(chunk)
        print()
        print(f"Compare chunk {index}/{len(chunks)}: {chunk_items} item(s)")
        results, compare_rc = compare_compiled_selections(
            compiled=compiled,
            selections=chunk,
            bridge_url=bridge_url,
            bridge=None,
            bn_call_budget=bn_call_budget,
        )
        all_results.extend(results)
        print_verification_summary(results, include_target=include_target)
        if compare_rc and overall == 0:
            overall = compare_rc
        if compare_rc and len(results) < chunk_items:
            print(
                f"Compare chunk {index}/{len(chunks)} stopped before all selected items were compared.",
                file=sys.stderr,
            )
            break
    return all_results, overall


def print_verification_summary(results: list[VerificationResult], *, include_target: bool = False) -> None:
    print("Verification summary:")
    if not results:
        print("- no items compared")
        return

    prefix = "target                                      " if include_target else ""
    print(
        f"{prefix}kind      address    status  mismatches  reloc-bytes  trim-nops  "
        "bn-size  vc5-size  evidence  triage"
    )
    for result in results:
        status = "FAIL" if result.mismatches else "OK"
        target_prefix = f"{result.target.name:42}  " if include_target else ""
        print(
            f"{target_prefix}"
            f"{result.item_kind:8}  "
            f"{result.function.address}  {status:5}  "
            f"{result.mismatches:10}  "
            f"{result.relocation_or_text_metric:11}  "
            f"{result.secondary_metric:9}  "
            f"{result.bn_size_or_normalized:7}  "
            f"{result.vc5_size_or_diff_count:8}  "
            f"{result.evidence_path}  "
            f"{result.triage_path}"
        )
    classified_paths = [
        result.comparison.classified_text_path
        for result in results
        if result.comparison.classified_text_path is not None
    ]
    if classified_paths:
        print("Classified asm drift:")
        for path in classified_paths:
            print(f"- {path}")
    print("Relocation bytes are masked from COFF relocation records; unmasked byte differences are failures.")


def print_evidence_block(compiled: CompiledTarget, result: VerificationResult) -> None:
    if result.mismatches:
        return

    target = result.target
    item = result.function
    source = target.source_from or str(compiled.source_path)
    print()
    if result.item_kind == "data":
        print("Data symbol evidence block:")
        print(f"- Address: {item.address}")
        print(f"- Data symbol: {item.name}")
        print(f"- Manifest: {target.manifest_path}")
        print(f"- Source: {source}")
        print(f"- Generated symbol: {item.symbol}")
        if target.compiler_profile:
            print(f"- Compiler profile: {target.compiler_profile}")
        print(f"- Compiler env: {compiled.compiler_env}")
        print(f"- Compiler version: {compiled.compiler_version}")
        print(f"- Compiler flags: {' '.join(target.compiler_flags)}")
        print("- Target architecture: x86")
        print(f"- VC object: {compiled.obj_path}")
        print(f"- Relocation mask: {result.comparison.mask_path}")
        print(f"- Relocation identity: {result.comparison.relocation_identity_path}")
        print(f"- Byte diff: {result.comparison.diff_path}")
        print(f"- Triage: {result.comparison.triage_path}")
        print("- Result: zero unmasked data-byte mismatches after COFF relocation masking.")
        return

    print("Tier S evidence block:")
    print(f"- Address: {item.address}")
    print(f"- Function: {item.name}")
    print(f"- Manifest: {target.manifest_path}")
    print(f"- Source: {source}")
    print(f"- Generated symbol: {item.symbol}")
    if target.compiler_profile:
        print(f"- Compiler profile: {target.compiler_profile}")
    print(f"- Compiler env: {compiled.compiler_env}")
    print(f"- Compiler version: {compiled.compiler_version}")
    print(f"- Compiler flags: {' '.join(target.compiler_flags)}")
    print("- Target architecture: x86")
    print(f"- VC object: {compiled.obj_path}")
    print(f"- VC listing: {compiled.cod_path}")
    print(f"- Relocation mask: {result.comparison.mask_path}")
    natural_vc5_size = getattr(
        result.comparison,
        "natural_vc5_size",
        result.vc5_size_or_diff_count,
    )
    excluded_vc5_tail_bytes = getattr(result.comparison, "excluded_vc5_tail_bytes", 0)
    excluded_vc5_tail_relocations = getattr(
        result.comparison,
        "excluded_vc5_tail_relocations",
        0,
    )
    print(f"- Natural COFF extent: {natural_vc5_size} bytes")
    print(f"- Compared VC5 body extent: {result.vc5_size_or_diff_count} bytes")
    print(f"- Excluded natural COFF tail: {excluded_vc5_tail_bytes} bytes")
    print(f"- Excluded natural COFF tail relocations: {excluded_vc5_tail_relocations}")
    if excluded_vc5_tail_bytes:
        print("- Excluded tail disposition: inventory only; not compared or accepted.")
    print(f"- Byte diff: {result.comparison.diff_path}")
    print(f"- Triage: {result.comparison.triage_path}")
    if result.comparison.text_diff_path:
        print(f"- Normalized asm diff: {result.comparison.text_diff_path}")
    if result.comparison.classified_text_path:
        print(f"- Classified asm drift: {result.comparison.classified_text_path}")
    print("- Result: zero unmasked byte mismatches after COFF relocation masking.")


def run_target(
    *,
    target: VerifyTarget,
    selected_functions: tuple[VerifyFunction, ...],
    selected_data_symbols: tuple[VerifyDataSymbol, ...],
    build_root: Path,
    build_subdir: str | None = None,
    vc5_env: Path,
    bridge_url: str,
    skip_bn_compare: bool,
    bn_call_budget: int,
    chunk_size: int | None = None,
    auto_chunk: bool = False,
    order_summary: bool = False,
) -> int:
    try:
        require_clean_target_source_fragments(target)
    except ValueError as exc:
        print(exc, file=sys.stderr)
        return 1
    try:
        build_dir = prepare_clean_build_dir(build_root, build_subdir or target.name)
    except ValueError as exc:
        print(exc, file=sys.stderr)
        return 2

    compiled, rc = compile_target(target=target, build_dir=build_dir, vc5_env=vc5_env)
    if rc != 0:
        return rc
    if compiled is None:
        return 3

    print_compiled_target_info(target, compiled)

    order_rc = run_function_order_checks(
        compiled=compiled,
        selections=[
            VerifySelection(
                target=target,
                functions=selected_functions,
                data_symbols=selected_data_symbols,
            )
        ],
        summary_only=order_summary,
    )
    overall = order_rc
    translation_unit_order_rc = run_translation_unit_function_order_checks(
        compiled=compiled,
        summary_only=order_summary,
    )
    if translation_unit_order_rc and overall == 0:
        overall = translation_unit_order_rc

    if skip_bn_compare:
        return overall

    selection = VerifySelection(
        target=target,
        functions=selected_functions,
        data_symbols=selected_data_symbols,
    )
    auto_chunk = auto_chunk or should_auto_chunk_by_default(
        selections=[selection],
        skip_bn_compare=skip_bn_compare,
        bn_call_budget=bn_call_budget,
        chunk_size=chunk_size,
        auto_chunk=auto_chunk,
    )
    if chunk_size is None and not auto_chunk:
        results, compare_rc = compare_compiled_selections(
            compiled=compiled,
            selections=[selection],
            bridge_url=bridge_url,
            bn_call_budget=bn_call_budget,
        )
        print_verification_summary(results)
    else:
        results, compare_rc = compare_compiled_selections_in_chunks(
            compiled=compiled,
            selections=[selection],
            bridge_url=bridge_url,
            bn_call_budget=bn_call_budget,
            chunk_size=chunk_size,
            auto_chunk=auto_chunk,
        )
        print()
        print("Aggregate verification summary:")
        print_verification_summary(results)
    if compare_rc and overall == 0:
        overall = compare_rc
    print(f"Verification evidence: {compiled.build_dir / 'verify'}")
    for result in results:
        print_evidence_block(compiled, result)
    return overall


def print_profile_sweep_summary(rows: list[ProfileSweepRow]) -> None:
    print()
    print("Profile sweep summary:")
    print(
        "profile                         address    status        mismatches  reloc/text  "
        "trim/sched  bn/norm  vc5/diff  evidence"
    )
    def sort_key(row: ProfileSweepRow) -> tuple[int, int, int, int, str]:
        profile_name = row.profile_name
        result = row.result
        if result is None:
            return (1, row.rc, 0, 0, profile_name)
        return (
            0,
            result.mismatches,
            abs(result.vc5_size_or_diff_count - result.bn_size_or_normalized),
            result.secondary_metric,
            profile_name,
        )

    for row in sorted(rows, key=sort_key):
        profile_name = row.profile_name
        result = row.result
        if result is None:
            if row.status == "bn_budget":
                status = "BN-BUDGET"
            elif row.status == "not_compared":
                status = "NOT-COMPARED"
            elif row.status == "order":
                status = "ORDER-FAIL"
            elif row.status == "profile_guard":
                status = "GUARDED"
            else:
                status = f"COMPILE({row.rc})"
            print(f"{profile_name:30}  <none>     {status}")
            continue
        status = "FAIL" if result.mismatches else "OK"
        print(
            f"{profile_name:30}  "
            f"{result.function.address}  {status:12}  "
            f"{result.mismatches:10}  "
            f"{result.relocation_or_text_metric:10}  "
            f"{result.secondary_metric:10}  "
            f"{result.bn_size_or_normalized:7}  "
            f"{result.vc5_size_or_diff_count:8}  "
            f"{result.evidence_path}"
        )


def profile_sweep_rerun_command(
    *,
    target: VerifyTarget,
    profile_names: list[str],
    build_root: Path,
    vc5_env: Path,
    bridge_url: str,
) -> str:
    args: list[str | Path] = [
        "python",
        Path("tools") / "recoil.py",
        "verify",
        "vc5",
        target.name,
        *target_binary_cli_args(target),
        "--profile-sweep",
        ",".join(profile_names),
    ]
    if build_root != DEFAULT_BUILD_ROOT:
        args.extend(["--build-root", build_root])
    if vc5_env != DEFAULT_VC5_ENV:
        args.extend(["--vc5-env", vc5_env])
    if bridge_url != DEFAULT_BRIDGE_URL:
        args.extend(["--bridge-url", bridge_url])
    return " ".join(quote_cmd_arg(arg) for arg in args)


def run_profile_sweep(
    *,
    target: VerifyTarget,
    selected_functions: tuple[VerifyFunction, ...],
    selected_data_symbols: tuple[VerifyDataSymbol, ...],
    profile_names: list[str],
    build_root: Path,
    vc5_env: Path,
    bridge_url: str,
    skip_bn_compare: bool,
    bn_call_budget: int,
    allow_disqualified_profiles: bool = False,
) -> int:
    try:
        require_clean_target_source_fragments(target)
    except ValueError as exc:
        print(exc, file=sys.stderr)
        return 1
    rows: list[ProfileSweepRow] = []
    any_success = False
    overall = 0
    bridge = None if skip_bn_compare else make_binja_bridge(
        bridge_url=bridge_url,
        bn_call_budget=bn_call_budget,
        target=target,
    )
    pending_after_budget: list[str] = []
    order_failed = False
    for profile_index, profile_name in enumerate(profile_names):
        guard_message = profile_guard_block_message(target, profile_name)
        if guard_message is not None and not allow_disqualified_profiles:
            print()
            print("Profile sweep skipped: " + guard_message)
            rows.append(ProfileSweepRow(profile_name, None, 0, "profile_guard"))
            if overall == 0:
                overall = 2
            continue
        sweep_target = with_compiler_profile_override(
            target,
            profile_name,
            allow_disqualified_profile=allow_disqualified_profiles,
        )
        build_subdir = safe_path_component(f"{target.name}__{profile_name}")
        try:
            build_dir = prepare_clean_build_dir(build_root, build_subdir)
        except ValueError as exc:
            print(exc, file=sys.stderr)
            return 2

        print()
        print(f"Profile sweep compile: {profile_name}")
        compiled, rc = compile_target(target=sweep_target, build_dir=build_dir, vc5_env=vc5_env)
        if rc != 0:
            rows.append(ProfileSweepRow(profile_name, None, rc))
            overall = rc if overall == 0 else overall
            continue
        if compiled is None:
            rows.append(ProfileSweepRow(profile_name, None, 3))
            overall = 3 if overall == 0 else overall
            continue
        print_compiled_target_info(sweep_target, compiled)

        selection = VerifySelection(
            target=sweep_target,
            functions=selected_functions,
            data_symbols=selected_data_symbols,
        )
        order_rc = run_function_order_checks(compiled=compiled, selections=[selection])
        translation_unit_order_rc = run_translation_unit_function_order_checks(compiled=compiled)
        if translation_unit_order_rc and order_rc == 0:
            order_rc = translation_unit_order_rc
        if order_rc:
            order_failed = True
            rows.append(ProfileSweepRow(profile_name, None, order_rc, "order"))
            if overall == 0:
                overall = order_rc

        if skip_bn_compare:
            if order_rc == 0:
                any_success = True
            continue

        results, compare_rc = compare_compiled_selections(
            compiled=compiled,
            selections=[selection],
            bridge_url=bridge_url,
            bridge=bridge,
            bn_call_budget=bn_call_budget,
        )
        for result in results:
            rows.append(ProfileSweepRow(profile_name, result, compare_rc, "compare"))
            if result.mismatches == 0:
                any_success = True
        if (
            compare_rc
            and bridge is not None
            and bridge.budget_state().remaining == 0
        ):
            if overall == 0:
                overall = compare_rc
            current_profile_incomplete = len(results) < (len(selected_functions) + len(selected_data_symbols))
            if current_profile_incomplete:
                rows.append(ProfileSweepRow(profile_name, None, compare_rc, "bn_budget"))
            pending_after_budget = profile_names[
                profile_index if current_profile_incomplete else profile_index + 1:
            ]
            for pending_profile in pending_after_budget:
                if pending_profile == profile_name and current_profile_incomplete:
                    continue
                rows.append(ProfileSweepRow(pending_profile, None, 0, "not_compared"))
            break
        if compare_rc and overall == 0:
            overall = compare_rc

    if not skip_bn_compare:
        print_profile_sweep_summary(rows)
        if pending_after_budget:
            print()
            print("Profile sweep stopped after the Binary Ninja bridge call budget was exhausted.")
            print(
                "Rerun remaining profiles: "
                + profile_sweep_rerun_command(
                    target=target,
                    profile_names=pending_after_budget,
                    build_root=build_root,
                    vc5_env=vc5_env,
                    bridge_url=bridge_url,
                )
            )
        if any_success and not order_failed:
            return 0
    if skip_bn_compare and any_success and not order_failed:
        return 0
    return overall


def run_batch(
    *,
    selections: list[VerifySelection],
    build_root: Path,
    vc5_env: Path,
    bridge_url: str,
    skip_bn_compare: bool,
    bn_call_budget: int,
    chunk_size: int | None = None,
    auto_chunk: bool = False,
) -> int:
    if not selections:
        print("No VC verification targets matched the requested batch.")
        return 2
    grouped = group_selections_by_compile_key(selections, vc5_env)
    total_targets = sum(len(group) for _key, group in grouped)
    total_items = sum(
        len(selection.functions) + len(selection.data_symbols)
        for _key, group in grouped
        for selection in group
    )
    print(
        f"VC batch: {total_targets} target(s), {total_items} item(s), "
        f"{len(grouped)} unique compile(s)."
    )

    overall = 0
    all_results: list[VerificationResult] = []
    batch_auto_chunk = auto_chunk or should_auto_chunk_by_default(
        selections=selections,
        skip_bn_compare=skip_bn_compare,
        bn_call_budget=bn_call_budget,
        chunk_size=chunk_size,
        auto_chunk=auto_chunk,
    )
    bridge = None
    for group_index, (_key, group) in enumerate(grouped, start=1):
        representative = group[0].target
        try:
            require_clean_target_source_fragments(representative)
        except ValueError as exc:
            print(exc, file=sys.stderr)
            return 1
        group_label = f"{group_index:03d}"
        group_name = f"_batch/{group_label}_{safe_path_component(representative.name)}"
        try:
            build_dir = prepare_clean_build_dir(build_root, group_name)
        except ValueError as exc:
            print(exc, file=sys.stderr)
            return 2

        names = ", ".join(selection.target.name for selection in group)
        print()
        print(f"Compile group {group_label}: {names}")
        compiled, rc = compile_target(target=representative, build_dir=build_dir, vc5_env=vc5_env)
        if rc != 0:
            overall = rc if overall == 0 else overall
            continue
        if compiled is None:
            overall = 3 if overall == 0 else overall
            continue
        print_compiled_target_info(representative, compiled)

        order_rc = run_function_order_checks(compiled=compiled, selections=group)
        if order_rc and overall == 0:
            overall = order_rc
        translation_unit_order_rc = run_translation_unit_function_order_checks(compiled=compiled)
        if translation_unit_order_rc and overall == 0:
            overall = translation_unit_order_rc

        if skip_bn_compare:
            continue

        if chunk_size is None and not batch_auto_chunk:
            results, compare_rc = compare_compiled_selections(
                compiled=compiled,
                selections=group,
                bridge_url=bridge_url,
                bridge=bridge,
                bn_call_budget=bn_call_budget,
            )
        else:
            results, compare_rc = compare_compiled_selections_in_chunks(
                compiled=compiled,
                selections=group,
                bridge_url=bridge_url,
                bn_call_budget=bn_call_budget,
                chunk_size=chunk_size,
                auto_chunk=batch_auto_chunk,
                include_target=True,
            )
        all_results.extend(results)
        if compare_rc:
            overall = compare_rc if overall == 0 else overall

    if not skip_bn_compare:
        print()
        if chunk_size is not None or batch_auto_chunk:
            print("VC batch aggregate:")
        print_verification_summary(all_results, include_target=True)
    return overall


def enforce_strict_source_emission_selections(
    selections: list[VerifySelection],
    *,
    enabled: bool,
    strict_source_traceability: bool = False,
) -> list[VerifySelection]:
    if not enabled and not strict_source_traceability:
        return selections
    debt: list[str] = []
    for selection in selections:
        for warning in selection.target.source_emission_warnings:
            debt.append(
                f"{selection.target.manifest_path}: {warning.address} {warning.code}: {warning.message}"
            )
    if enabled and debt:
        raise ValueError(
            "strict source-emission policy failed for selected VC5 target(s):\n- "
            + "\n- ".join(dict.fromkeys(debt))
        )
    strict_targets: dict[Path, VerifyTarget] = {}
    if strict_source_traceability:
        for selection in selections:
            manifest_path = selection.target.manifest_path.resolve()
            if manifest_path not in strict_targets:
                strict_targets[manifest_path] = load_manifest(
                    selection.target.manifest_path,
                    strict_source_emissions=enabled,
                    strict_source_traceability=True,
                )
    return [
        replace(
            selection,
            target=(
                strict_targets[selection.target.manifest_path.resolve()]
                if strict_source_traceability
                else replace(selection.target, source_emission_policy_strict=True)
            ),
        )
        for selection in selections
    ]


def run_owner_mode(
    *,
    args: argparse.Namespace,
    manifests: list[VerifyTarget],
    chunk_size: int | None,
    auto_chunk: bool,
) -> int:
    selected_binary = args.binary or DEFAULT_TARGET_BINARY
    owners_path = Path(resolve_owner_ledger_path(selected_binary, args.progress))
    owner_doc = SourceOwnerDocument.load(owners_path)
    entry_index = OwnerEntryIndex.load(owners_path, binary=selected_binary)
    scope = resolve_owner_vc5_scope(
        owner_doc=owner_doc,
        entry_index=entry_index,
        manifests=manifests,
        owner_selector=args.owner,
        binary=selected_binary,
    )
    selections = apply_target_binary_override(list(scope.selections), args.binary)
    selections = enforce_strict_source_emission_selections(
        selections,
        enabled=args.strict_source_emissions,
        strict_source_traceability=args.strict_source_traceability,
    )
    scope = OwnerVc5Scope(
        owner=scope.owner,
        required_entries=scope.required_entries,
        selections=tuple(selections),
        issues=scope.issues,
    )
    print_owner_vc5_scope(scope, binary=selected_binary)
    if scope.issues:
        return 2
    return run_batch(
        selections=list(scope.selections),
        build_root=Path(args.build_root),
        vc5_env=Path(args.vc5_env),
        bridge_url=args.bridge_url,
        skip_bn_compare=args.skip_bn_compare,
        bn_call_budget=args.bn_call_budget,
        chunk_size=chunk_size,
        auto_chunk=auto_chunk,
    )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Compile one or more VC verification targets and compare relocation-masked COFF "
            "function or data-symbol bytes to BN."
        )
    )
    parser.add_argument(
        "targets",
        nargs="*",
        metavar="target",
        help="Target manifest name or covered function/data address, e.g. zsys_cpu or 0x4b3510",
    )
    parser.add_argument(
        "--order-only",
        action="store_true",
        help=(
            "Run one registered function-order target without Binary Ninja or byte comparison, "
            "using a fresh isolated VC5 compile and a live semantic identity/order comparison."
        ),
    )
    parser.add_argument(
        "--linked-target",
        metavar="TARGET",
        help=(
            "After an object-order PASS, compile/link the configured whole project and report "
            "the exact full linked selected-population/seam divergence for TARGET. Valid only "
            "with --order-only."
        ),
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Emit the live vc5-order result as one JSON object (valid only with --order-only).",
    )
    parser.add_argument(
        "--owner",
        metavar="OWNER_ID_OR_ADDRESS",
        help="Run strict VC5 verification for every projected address row linked to one unified-progress source owner.",
    )
    parser.add_argument("--progress", default=str(DEFAULT_OWNER_LEDGER), help="Path to unified reconstruction progress for --owner.")
    parser.add_argument(
        "--target",
        dest="target_aliases",
        action="append",
        default=[],
        metavar="target",
        help="Target manifest name or covered function/data address. Alias for the positional target selector.",
    )
    parser.add_argument("--targets-json", help="JSON array of target names or covered function/data addresses.")
    parser.add_argument("--all", action="store_true", help="Run every VC verification target, grouping identical compiles.")
    parser.add_argument(
        "--source-from",
        metavar="PATH",
        help="Run every target whose source_from resolves to PATH, grouping identical compiles.",
    )
    parser.add_argument(
        "--all-covering",
        action="store_true",
        help="When target is an address, run every manifest covering that address instead of requiring a unique target.",
    )
    parser.add_argument("--list", action="store_true", help="List available VC verification targets.")
    parser.add_argument(
        "--explain-missing",
        metavar="ADDRESS",
        help="Explain VC5SP3 coverage for an address and print a skeleton manifest if none exists.",
    )
    parser.add_argument("--manifest-dir", default=str(DEFAULT_MANIFEST_DIR))
    parser.add_argument("--build-root", default=str(DEFAULT_BUILD_ROOT))
    parser.add_argument("--vc5-env", default=str(DEFAULT_VC5_ENV))
    parser.add_argument("--bridge-url", default=DEFAULT_BRIDGE_URL)
    parser.add_argument(
        "--binary",
        choices=reference_image_keys(),
        default=None,
        help=(
            "Override the manifest target_binary for live Binary Ninja hexdumps "
            "(use messages for messages.dll/messages.bndb targets)."
        ),
    )
    parser.add_argument(
        "--bn-call-budget",
        type=int,
        default=DEFAULT_BN_CALL_BUDGET,
        help="Maximum Binary Ninja bridge calls for this invocation; 0 disables the limit.",
    )
    parser.add_argument(
        "--chunk-size",
        type=int,
        metavar="N",
        help=(
            "Compare selected BN items in chunks of at most N items, creating a fresh bounded "
            "BN bridge invocation for each chunk."
        ),
    )
    parser.add_argument(
        "--auto-chunk",
        action="store_true",
        help=(
            "Automatically split BN comparisons into internal chunks estimated to stay within "
            "the active --bn-call-budget, preserving compile grouping. Multi-data-symbol "
            "BN comparisons select this mode by default."
        ),
    )
    parser.add_argument(
        "--skip-bn-compare",
        action="store_true",
        help=(
            "Compile and run compile-time diagnostics only. Required for "
            "check_translation_unit_function_order manifests."
        ),
    )
    parser.add_argument(
        "--compiler-profile",
        metavar="NAME",
        help="Temporarily compile one selected target with a profile from tools/_recoil/config/compiler_linker_profiles.json.",
    )
    parser.add_argument(
        "--profile-sweep",
        nargs="?",
        const="*",
        metavar="NAMES",
        help="Temporarily compile one selected target across all profiles, or a comma-separated profile list.",
    )
    parser.add_argument(
        "--allow-disqualified-profile",
        action="store_true",
        help=(
            "Allow an explicit compiler-profile override or sweep to run profiles that the selected "
            "manifest disqualifies or omits from profile_guard.accepted_profiles. Use only when "
            "reopening compiler provenance with new sentinel evidence."
        ),
    )
    parser.add_argument(
        "--strict-source-emissions",
        action="store_true",
        help=(
            "Require every compiler-generated authored-order row to carry a valid "
            "source-anchored emission_anchor; validate canonical or supplied legacy "
            "emission markers when present."
        ),
    )
    parser.add_argument(
        "--strict-source-traceability",
        action="store_true",
        help=(
            "Require canonical source-trace directives for resolved authored VC rows; "
            "validate exact relation, artifact id, section, and direct attachment. "
            "Repository-wide tracker mirror/state enforcement remains audit source-trace "
            "--policy migrated."
        ),
    )
    return parser


def resolve_compare_chunking(args: argparse.Namespace, parser: argparse.ArgumentParser) -> tuple[int | None, bool]:
    chunk_size = args.chunk_size
    if chunk_size is not None and chunk_size <= 0:
        parser.error("--chunk-size must be a positive integer")
    if args.auto_chunk:
        if chunk_size is not None:
            parser.error("--auto-chunk cannot be combined with --chunk-size")
        if args.bn_call_budget <= 0:
            parser.error("--auto-chunk requires a positive --bn-call-budget")
        return None, True
    return chunk_size, False


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        if args.json and not args.order_only:
            parser.error("--json is valid only with --order-only")
        if args.linked_target and not args.order_only:
            parser.error("--linked-target is valid only with --order-only")
        if args.order_only:
            args.skip_bn_compare = True
            args.strict_source_emissions = True
            incompatible = [
                (args.owner, "--owner"),
                (args.all, "--all"),
                (args.source_from, "--source-from"),
                (args.all_covering, "--all-covering"),
                (args.list, "--list"),
                (args.explain_missing, "--explain-missing"),
                (args.compiler_profile, "--compiler-profile"),
                (args.profile_sweep, "--profile-sweep"),
                (args.chunk_size, "--chunk-size"),
                (args.auto_chunk, "--auto-chunk"),
            ]
            rejected = [name for value, name in incompatible if value]
            if rejected:
                parser.error(
                    "--order-only runs one canonical order target and cannot be combined with "
                    + ", ".join(rejected)
                )
        if args.owner:
            if args.targets or args.target_aliases or args.targets_json:
                parser.error(
                    "--owner cannot be combined with explicit target selectors, "
                    "--target, or --targets-json"
                )
            if args.all or args.source_from or args.all_covering:
                parser.error("--owner cannot be combined with --all, --source-from, or --all-covering")
            if args.list or args.explain_missing:
                parser.error("--owner cannot be combined with --list or --explain-missing")
            if args.compiler_profile or args.profile_sweep:
                parser.error("--owner cannot be combined with --compiler-profile or --profile-sweep")
        try:
            operation_inventory = load_git_tracked_path_inventory(REPO_ROOT)
        except RepositoryPathError:
            if (REPO_ROOT / ".git").exists():
                raise
            operation_inventory = None
        manifests = load_manifests(
            Path(args.manifest_dir),
            tracked_path_inventory=operation_inventory,
        )
        if not args.json:
            print_source_emission_warnings(manifests)
        chunk_size, auto_chunk = resolve_compare_chunking(args, parser)
        if args.owner:
            return run_owner_mode(
                args=args,
                manifests=manifests,
                chunk_size=chunk_size,
                auto_chunk=auto_chunk,
            )
        if args.list:
            enforce_strict_source_emission_selections(
                selected_targets_for_all(manifests),
                enabled=args.strict_source_emissions,
                strict_source_traceability=args.strict_source_traceability,
            )
            print_target_list(manifests)
            return 0
        if args.explain_missing:
            owners_path = Path(args.progress) if args.progress else REPO_ROOT / resolve_owner_ledger_path(args.binary or DEFAULT_TARGET_BINARY)
            print_missing_explanation(manifests, args.explain_missing, owners_path=owners_path)
            return 0
        selectors = load_explicit_selectors(args)
        if args.order_only and len(selectors) != 1:
            parser.error("--order-only requires exactly one registered target name")
        if args.all or args.source_from:
            if selectors:
                parser.error("target cannot be combined with --all or --source-from")
            if args.compiler_profile:
                parser.error("--compiler-profile is only supported for one selected target")
            if args.profile_sweep:
                parser.error("--profile-sweep is only supported for one selected target")
            selections = (
                selected_targets_for_all(manifests)
                if args.all
                else selected_targets_for_source_from(
                    manifests,
                    args.source_from,
                    tracked_path_inventory=operation_inventory,
                )
            )
            selections = apply_target_binary_override(selections, args.binary)
            selections = enforce_strict_source_emission_selections(
                selections,
                enabled=args.strict_source_emissions,
                strict_source_traceability=args.strict_source_traceability,
            )
            return run_batch(
                selections=selections,
                build_root=Path(args.build_root),
                vc5_env=Path(args.vc5_env),
                bridge_url=args.bridge_url,
                skip_bn_compare=args.skip_bn_compare,
                bn_call_budget=args.bn_call_budget,
                chunk_size=chunk_size,
                auto_chunk=auto_chunk,
            )
        if not selectors:
            parser.error("target is required unless --list, --explain-missing, --all, or --source-from is used")
        if len(selectors) > 1:
            if args.all_covering:
                parser.error("--all-covering is only supported for one selected target")
            if args.compiler_profile:
                parser.error("--compiler-profile is only supported for one selected target")
            if args.profile_sweep:
                parser.error("--profile-sweep is only supported for one selected target")
            selections = apply_target_binary_override(
                selections_for_explicit_selectors(manifests, selectors),
                args.binary,
            )
            selections = enforce_strict_source_emission_selections(
                selections,
                enabled=args.strict_source_emissions,
                strict_source_traceability=args.strict_source_traceability,
            )
            return run_batch(
                selections=selections,
                build_root=Path(args.build_root),
                vc5_env=Path(args.vc5_env),
                bridge_url=args.bridge_url,
                skip_bn_compare=args.skip_bn_compare,
                bn_call_budget=args.bn_call_budget,
                chunk_size=chunk_size,
                auto_chunk=auto_chunk,
            )
        target_selector = selectors[0]
        if args.all_covering:
            if not target_selector.lower().startswith("0x"):
                parser.error("--all-covering requires an address target")
            if args.compiler_profile:
                parser.error("--compiler-profile is only supported for one selected target")
            if args.profile_sweep:
                parser.error("--profile-sweep is only supported for one selected target")
            selections = apply_target_binary_override(
                selected_targets_covering_address(manifests, target_selector),
                args.binary,
            )
            selections = enforce_strict_source_emission_selections(
                selections,
                enabled=args.strict_source_emissions,
                strict_source_traceability=args.strict_source_traceability,
            )
            return run_batch(
                selections=selections,
                build_root=Path(args.build_root),
                vc5_env=Path(args.vc5_env),
                bridge_url=args.bridge_url,
                skip_bn_compare=args.skip_bn_compare,
                bn_call_budget=args.bn_call_budget,
                chunk_size=chunk_size,
                auto_chunk=auto_chunk,
            )
        target, functions, data_symbols, selector = find_target(manifests, target_selector)
        target = with_target_binary(target, args.binary)
        if args.order_only:
            if selector != target.name:
                parser.error("--order-only requires the registered target name, not an address selector")
            if not (target.check_function_order or target.check_translation_unit_function_order):
                parser.error(
                    f"{target.name!r} is not a function-order target; use verify vc5 for byte/data verification"
                )
        strict_selection = enforce_strict_source_emission_selections(
            [VerifySelection(target=target, functions=functions, data_symbols=data_symbols)],
            enabled=args.strict_source_emissions,
            strict_source_traceability=args.strict_source_traceability,
        )[0]
        target = strict_selection.target
        functions = strict_selection.functions
        data_symbols = strict_selection.data_symbols
        if args.order_only:
            linked_target = None
            if args.linked_target:
                linked_target = next(
                    (
                        manifest
                        for manifest in manifests
                        if manifest.name == args.linked_target
                    ),
                    None,
                )
                if linked_target is None:
                    raise ValueError(
                        f"Unknown linked VC verification target: {args.linked_target}"
                    )
                linked_target = with_target_binary(linked_target, args.binary)
                _validate_linked_feedback_pair(target, linked_target)
            with contextlib.redirect_stdout(io.StringIO()):
                result = live_order_result(
                    target=target,
                    build_root=Path(args.build_root),
                    vc5_env=Path(args.vc5_env),
                )
            if result["passed"] and linked_target is not None:
                result = live_linked_feedback_result(
                    object_target=target,
                    linked_target=linked_target,
                    object_result=result,
                    build_root=Path(args.build_root),
                    progress_path=Path(args.progress),
                )
            if args.json:
                print(json.dumps(result, ensure_ascii=False, sort_keys=True))
            else:
                print_live_order_result(result)
            return 0 if result["passed"] else 1
        if selector != target.name:
            print(f"Resolved {selector} to VC target {target.name}")
        if args.compiler_profile and args.profile_sweep:
            parser.error("--compiler-profile cannot be combined with --profile-sweep")
        if args.profile_sweep:
            return run_profile_sweep(
                target=target,
                selected_functions=functions,
                selected_data_symbols=data_symbols,
                profile_names=parse_profile_sweep_spec(args.profile_sweep),
                build_root=Path(args.build_root),
                vc5_env=Path(args.vc5_env),
                bridge_url=args.bridge_url,
                skip_bn_compare=args.skip_bn_compare,
                bn_call_budget=args.bn_call_budget,
                allow_disqualified_profiles=args.allow_disqualified_profile,
            )
        if args.compiler_profile:
            target = with_compiler_profile_override(
                target,
                args.compiler_profile,
                allow_disqualified_profile=args.allow_disqualified_profile,
            )
            print(f"Compiler profile override: {args.compiler_profile}")
        build_subdir = None
        if args.compiler_profile:
            build_subdir = safe_path_component(f"{target.name}__{args.compiler_profile}")
        run_auto_chunk = auto_chunk or should_auto_chunk_by_default(
            selections=[
                VerifySelection(
                    target=target,
                    functions=functions,
                    data_symbols=data_symbols,
                )
            ],
            skip_bn_compare=args.skip_bn_compare,
            bn_call_budget=args.bn_call_budget,
            chunk_size=chunk_size,
            auto_chunk=auto_chunk,
        )
        return run_target(
            target=target,
            selected_functions=functions,
            selected_data_symbols=data_symbols,
            build_root=Path(args.build_root),
            build_subdir=build_subdir,
            vc5_env=Path(args.vc5_env),
            bridge_url=args.bridge_url,
            skip_bn_compare=args.skip_bn_compare,
            bn_call_budget=args.bn_call_budget,
            chunk_size=chunk_size,
            auto_chunk=run_auto_chunk,
        )
    except ValueError as exc:
        print(str(exc), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
