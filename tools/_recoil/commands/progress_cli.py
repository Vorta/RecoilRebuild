from __future__ import annotations

import argparse
from collections import Counter
from contextlib import nullcontext
from contextvars import ContextVar
from copy import deepcopy
from functools import wraps
import json
import os
from pathlib import Path
import re
import secrets
import subprocess
import sys
from typing import Any, Iterable, Mapping

from _recoil.commands.vc5_verify import (
    COMPILER_GENERATED_AUTHORED_ORDER_ROLES,
    FUNCTION_PROVENANCE_COMPILER_EMITTED_NONCOVERING,
    FUNCTION_PROVENANCE_PROVIDER_BOUNDARY,
    authored_relative_order_gate,
    generated_file_shadows_project,
    has_compiler_emitted_policy_marker,
    has_provider_boundary_policy_marker,
    load_manifest as load_vc5_manifest,
    normalize_generated_path,
    normalize_order_edit_paths,
    require_clean_target_source_fragments,
    source_trace_documents,
    source_from_policy_text,
    validate_generated_source_emission_policy,
)
from _recoil.commands.progress_v2 import (
    accept_live_call_contract_symbols,
    accept_live_byte_groups,
    accept_live_order_block,
    add_live_evidence,
)
from _recoil.commands.workspace_packet_handoff import (
    WorkspacePacketHandoffError,
    render_workspace_issue_handoff,
)
from _recoil.commands.call_contract_verify import (
    WOL_PROFILE_MATRIX_TARGET_ID,
    WOL_PROFILE_SOURCE_HANDOFF_EXPECTED_TRUTH,
    WOL_PROFILE_SOURCE_HANDOFF_WRITE_PATHS,
    _decorated_coff_name,
    _finite_literal_symbol_regex_alternatives,
    _valid_wol_profile_source_handoff_route,
    _valid_intentionally_inlined_absence_proof,
    call_contract_source_closure,
    call_contract_source_write_paths,
    prospective_wol_profile_source_handoff_route,
    source_dependency_paths,
)
from _recoil.commands.call_contract_convergence import (
    CONVERGENCE_CONTRACT_VERSION,
    CONVERGENCE_EXPECTED_TRUTH,
    CONVERGENCE_MIGRATION_KEY,
    CONVERGENCE_VERIFIER_SEMANTIC_PATHS,
    RETAIL_FACT_PACKET_TYPE,
    _valid_retail_fact_scope,
    _normalized_semantic_projection,
    carry_current_generation_across_work_ledger_mutation,
    compact_convergence_census,
    convergence_generation_state,
    current_call_contract_verifier_semantic_identity,
    dependent_owner_repair_launchability,
    derive_convergence_census,
    prepare_live_convergence,
    prospective_wol_profile_convergence_route,
    retail_fact_packet_scopes,
)
from _recoil.commands.call_contract_continuation import (
    CONTINUATION_MIGRATION_KEY,
    CONTINUATION_PACKET_TYPE,
    CONTINUATION_PRODUCER_TYPE,
    LINKED_TOOL_ISSUE,
    PRODUCER_RESULT_SCHEMA,
    RETURN_PROVENANCE_FIELD,
    activate_continuation_child,
    archive_continuation_checkpoint,
    capture_continuation_input_snapshot,
    continuation_snapshots_equal,
    continuation_state,
    finalize_continuation_child,
    prepare_repair_continuation,
    returned_tool_blocked_provenance,
    validate_continuation_checkpoint,
)
from _recoil.commands import storage_contribution_progress as _authored_storage_progress
from _recoil.commands.storage_contribution_progress import (
    load_payload as load_authored_storage_payload,
)
from _recoil.lib.progress import (
    AUTHORED_BYTE_DIMENSIONS,
    AUTHORED_ORDER_DIMENSIONS,
    CALL_CONTRACT_DIMENSION,
    CALL_CONTRACT_CONTRACT_VERSION,
    CALL_CONTRACT_EXPECTED_TRUTH,
    CALL_CONTRACT_VALIDATION_MODE,
    EXACT_LINK_DIMENSIONS,
    FULL_ORDER_DIMENSIONS,
    ConcurrentProgressUpdate,
    ProgressDocument,
    ProgressError,
    ProgressStore,
    accept_live_authored_non_gating_blocks,
    address_value,
    bind_work_packet_contract,
    bind_progress_packet_native_git,
    CLAIM_CURRENT_COMMAND,
    EXPLICIT_RESULT_MAX_BYTES,
    EXPLICIT_OUTPUT_MARKER_NAME,
    EXPLICIT_ALLOCATION_JOURNAL_MIGRATION_KEY,
    EXPLICIT_ALLOCATION_JOURNAL_SCHEMA,
    EXPLICIT_MAINTENANCE_PACKET_TYPE,
    activate_explicit_maintenance_work_item,
    authenticate_explicit_output_marker,
    authenticate_explicit_output_root,
    construct_explicit_maintenance_work_item,
    create_and_reserve_claim_current_work_item,
    create_and_reserve_explicit_maintenance_work_item,
    create_and_reserve_repair_continuation_work_item,
    is_current_accepted_state,
    fail_explicit_maintenance_allocation,
    explicit_output_sidecar_path,
    explicit_output_marker_record,
    _issue_explicit_cleanup_recovery_receipt,
    _path_has_reparse_component,
    invalidate_order_dependencies,
    normalize_address,
    normalize_resource_claims,
    resource_claim_conflicts,
    recover_expired_explicit_maintenance_work_item,
    recover_explicit_maintenance_cleanup_debt,
    reserve_work_item,
    retail_fact_packet_contract_problem,
    state_record,
    symbol_authored_order_gate,
    validate_authored_order_role,
    validate_claim_provenance,
    validate_owner_invariants,
    return_explicit_maintenance_work_item,
    work_resource_claims,
    progress_packet_tracked_write_paths,
)
from _recoil.lib.repository_paths import (
    GitTrackedPathInventory,
    RepositoryPathError,
    TrackedRepositoryPath,
    diagnose_historical_repository_path,
    load_git_tracked_path_inventory,
    resolve_tracked_repository_file,
)
from _recoil.lib.live_progress import ConcurrentRevisionUpdate
from _recoil.lib.binja import (
    BinaryNinjaBridge,
    BridgeError,
    validate_authenticated_recoil_snapshot_receipt,
)
from _recoil.lib.pe import parse_pe_headers, rva_to_offset
from _recoil.lib.source_traceability import parse_source_trace_text
from _recoil.lib.issue_sqlite import read_issue_metadata
from _recoil.lib.progress_sqlite import (
    DELETE_FACET,
    ProgressSQLiteError,
    ProgressSQLiteStore,
    read_progress_metadata,
)
from _recoil.lib.call_contract_generations import (
    current_generations,
    evidence_generations_current,
)
from _recoil.commands.source_trace_progress import normalize_source_traceability
from _recoil.lib.authored_icf import (
    AUTHORED_ICF_GROUP_MODEL,
    AUTHORED_ICF_MEMBER_GATE_MODE,
    audit_authored_icf_groups,
    validate_authored_icf_proof,
    validate_authored_icf_source_mirrors,
)
from _recoil.lib.tooling import REPO_ROOT, configure_stdio, display_path
from _recoil.lib.worktree_control import (
    WorktreeAssociation,
    WorktreeControlError,
    authenticate_build_root,
    authenticated_validation_command_tokens,
    create_build_root,
    create_linked_worktree,
    derive_packet_locations,
    remove_authenticated_build_root,
    remove_linked_worktree,
    reauthenticate_canonical_control_root,
    require_clean_worktree,
    resolve_canonical_control_root,
    resolve_topology,
    resolve_exact_packet_worktree,
    routed_machine_local_path,
)
from _recoil.lib.git_change_control import (
    GitChangeControlError,
    capture_clean_git_baseline,
    reauthenticate_clean_git_baseline,
)


PROGRESS_AUTHORITY_RELATIVE_PATH = ".agent/RECONSTRUCTION_PROGRESS.sqlite3"
ISSUE_AUTHORITY_RELATIVE_PATH = ".agent/WORKSPACE_ISSUES.sqlite3"
TOOL_BLOCKED_PROVENANCE_FIELD = "tool_blocked_return_provenance"
CALL_CONTRACT_CONTINUATION_PREDECESSOR_TYPES = frozenset(
    {
        "call-contract-edit-v1",
        "call-contract-converge-edit-v1",
        "call-contract-dependent-owner-edit-v1",
    }
)
DEFAULT_PROGRESS = routed_machine_local_path(
    executing_worktree_root=REPO_ROOT,
    relative_path=PROGRESS_AUTHORITY_RELATIVE_PATH,
)
DEFAULT_ISSUE_LEDGER = routed_machine_local_path(
    executing_worktree_root=REPO_ROOT,
    relative_path=ISSUE_AUTHORITY_RELATIVE_PATH,
)
MACHINE_RETAIL_REFERENCE = routed_machine_local_path(
    executing_worktree_root=REPO_ROOT,
    relative_path="support/Recoil.exe",
)
MAX_PROGRESS_PAYLOAD_FILE_BYTES = 16 * 1024 * 1024

CALL_CONTRACT_VERIFICATION_ACCEPTANCE_ENABLED = True
CALL_CONTRACT_VERIFICATION_ACCEPTANCE_DISABLED_REASON = None
CALL_CONTRACT_CURRENTNESS_AUDIT_ENABLED = False
CALL_CONTRACT_CURRENTNESS_AUDIT_DISABLED_REASON = (
    "call-contract verification currentness audit is disabled until an "
    "in-process governed expected-fact adapter can transcript every retail "
    "read under one authenticated begin/end Binary Ninja session"
)
CALL_CONTRACT_CONVERGENCE_ENABLED = False
CALL_CONTRACT_CONVERGENCE_DISABLED_REASON = (
    "call-contract convergence is disabled until its complete retail expected-fact "
    "read set is produced through the governed authenticated BN session"
)


CALL_CONTRACT_VERIFICATION_ACCEPTANCE_POLICY = {
    "contract_version": 1,
    "containment_status": "active-fresh-direct-comparison",
    "runtime_acceptance_enabled": True,
    "explicit_expensive_surfaces": {
        "currentness_audit": "verify call-contract --verification-currentness-audit",
        "acceptance": "progress advance-live-call-contract",
        "reservation_preflight_required": True,
        "isolated_build_root_required": True,
        "binary_ninja_read_claim_required": True,
    },
    "acceptance_authority": "fresh-parent-direct-retail",
    "identity_role": "explicit-invalidation-and-generations",
    "slice_role": "cursor-window",
    "worker_acceptance_allowed": False,
    "partial_body_acceptance": True,
    "phase_closeout_required": True,
    "phase_closeout_no_reuse": True,
    "phase_closeout_global_clean": True,
    "lease_stales_semantic_evidence": False,
    "obligation_conflict_authority": "resource-claims",
    "obligation_packet_types": {
        "source": "call-contract-source-obligation-v1",
        "profile": "call-contract-profile-obligation-v1",
        "verifier": "call-contract-verifier-obligation-v1",
        "linker": "call-contract-linker-obligation-v1",
        "retail": RETAIL_FACT_PACKET_TYPE,
    },
    "verifier_result_policy": {
        "all_caller_divergences_collected": True,
        "candidate_expected_truth": False,
        "verification_eligible": True,
    },
    "verification_route": {
        "direct_body_results": True,
        "translation_unit_context": True,
        "unrelated_failure_blocks_passing_leaf": False,
        "slice_role": "cursor-window",
        "parent_acceptance_allowed": True,
        "worker_acceptance_allowed": False,
        "identity_role": "explicit-invalidation-and-generations",
        "semantic_revision_guard": True,
        "evidence_generation_revision_guard": True,
        "transaction_revision_guard": False,
    },
}
CALL_CONTRACT_OBLIGATION_PACKET_CONTRACTS = {
    "call-contract-source-obligation-v1": {
        "obligation_kind": "source",
        "handoff_role": "recoil_source_worker",
        "source_writes": True,
    },
    "call-contract-profile-obligation-v1": {
        "obligation_kind": "profile",
        "handoff_role": "recoil_tool_maintainer",
        "source_writes": False,
    },
    "call-contract-verifier-obligation-v1": {
        "obligation_kind": "verifier",
        "handoff_role": "recoil_tool_maintainer",
        "source_writes": False,
    },
    "call-contract-linker-obligation-v1": {
        "obligation_kind": "linker",
        "handoff_role": "recoil_tool_maintainer",
        "source_writes": False,
    },
}
ORDER_PHASES = {"authored-function-order", "full-function-order"}
SOURCE_POLICY_BOOTSTRAP_STATE = "pending-source-placement"
BYTE_LANES = {"object", "authored", "linked"}
BYTE_VERIFY_COMMANDS = {
    "object": "authored-object-byte",
    "authored": "authored-byte",
    "linked": "linked-byte",
}
CLAIM_LANES = ("all", "primary", "authored", "object")
DIVERGENCE_KINDS = {"missing", "extra", "duplicate", "reordered"}
PIPELINE_CLASSES = {"authored", "authored-lifecycle", "non-authored", "unresolved"}


class OrderTargetRoleGateError(ProgressError):
    """A whole-block order target contains an unresolved selected row."""

    reason_code = "order-target-role-gate-blocked"

    def __init__(
        self,
        *,
        target_id: str,
        phase: str,
        address: str,
        identity: str,
        label: str,
        problems: Iterable[str],
    ) -> None:
        problem_list = [str(problem) for problem in problems]
        self.blocker = {
            "kind": "order-target-role-gate",
            "target_id": target_id,
            "phase": phase,
            "address": address,
            "identity": identity,
            "label": label,
            "problems": problem_list,
        }
        super().__init__(
            f"target {target_id!r} cannot cover a whole block while order row "
            f"{address} ({identity}; {label}) is unresolved: "
            + "; ".join(problem_list)
        )


def _add_progress_path(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--progress", type=Path, default=DEFAULT_PROGRESS)


def _add_mutation_controls(
    parser: argparse.ArgumentParser,
    *,
    expected_revision_required: bool = True,
    expected_revision_mode: str = "public",
) -> None:
    if expected_revision_mode == "none":
        parser.set_defaults(expected_revision=None)
    else:
        parser.add_argument(
            "--expected-revision",
            type=int,
            required=expected_revision_required,
            help=(
                argparse.SUPPRESS
                if expected_revision_mode == "fixture-only"
                else None
            ),
        )
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--dry-run", action="store_true")
    mode.add_argument("--apply", action="store_true")
    parser.add_argument("--json", action="store_true")


def _add_call_contract_domain_guards(parser: argparse.ArgumentParser) -> None:
    """Add the paired scoped-CAS guards required by live call-contract routes."""

    parser.add_argument("--expected-semantic-revision", type=int, required=True)
    parser.add_argument(
        "--expected-evidence-generation-revision", type=int, required=True
    )


def _call_contract_expected_domain_revisions(
    args: argparse.Namespace,
) -> dict[str, int] | None:
    semantic = getattr(args, "expected_semantic_revision", None)
    evidence = getattr(args, "expected_evidence_generation_revision", None)
    if (semantic is None) != (evidence is None):
        raise ProgressError(
            "call-contract domain CAS requires both --expected-semantic-revision "
            "and --expected-evidence-generation-revision"
        )
    legacy = getattr(args, "expected_revision", None)
    if legacy is not None:
        raise ProgressError(
            "call-contract mutation requires semantic/evidence domain CAS; "
            "--expected-revision is not accepted"
        )
    if semantic is not None:
        return {
            "semantic": int(semantic),
            "evidence_generation": int(evidence),
        }
    raise ProgressError(
        "provide both --expected-semantic-revision and "
        "--expected-evidence-generation-revision"
    )


def _json_pointer_token(value: str) -> str:
    return value.replace("~", "~0").replace("/", "~1")


def _call_contract_scoped_patch_commit(
    *,
    args: argparse.Namespace,
    document: ProgressDocument,
    transform: Any,
    expected_domains: Mapping[str, int],
    increment_domains: Iterable[str] | None = None,
) -> Any:
    """Apply one transform as narrow entity/facet patches under domain CAS."""

    proposed = deepcopy(document.data)
    transform(proposed)
    entity_patches: dict[str, dict[str, dict[str, Any]]] = {}
    for collection in (
        "symbols",
        "physical_blocks",
        "semantic_spans",
        "source_owners",
        "verification_targets",
        "work_items",
        "evidence",
    ):
        before_rows = document.data.get(collection, {})
        after_rows = proposed.get(collection, {})
        if not isinstance(before_rows, Mapping) or not isinstance(after_rows, Mapping):
            continue
        for entity_id in sorted(set(before_rows) | set(after_rows)):
            before = before_rows.get(entity_id, DELETE_FACET)
            after = after_rows.get(entity_id, DELETE_FACET)
            if before == after:
                continue
            entity_patches.setdefault(collection, {})[str(entity_id)] = {
                "": DELETE_FACET if after is DELETE_FACET else deepcopy(after)
            }
    top_level_patches: dict[str, dict[str, Any]] = {}
    for top_key in ("id_sequences", "migration"):
        before = document.data.get(top_key, {})
        after = proposed.get(top_key, {})
        if not isinstance(before, Mapping) or not isinstance(after, Mapping):
            if before != after:
                top_level_patches[top_key] = {"": deepcopy(after)}
            continue
        for child_key in sorted(set(before) | set(after)):
            old = before.get(child_key, DELETE_FACET)
            new = after.get(child_key, DELETE_FACET)
            if old == new:
                continue
            top_level_patches.setdefault(top_key, {})[
                "/" + _json_pointer_token(str(child_key))
            ] = DELETE_FACET if new is DELETE_FACET else deepcopy(new)
    sql = ProgressSQLiteStore(Path(args.progress))
    return sql.persist_scoped_changes(
        expected_domain_revisions=dict(expected_domains),
        entity_patches=entity_patches,
        top_level_patches=top_level_patches,
        increment_domains=increment_domains,
        apply=bool(args.apply),
    )


def _precheck_call_contract_revisions(
    args: argparse.Namespace,
    document: ProgressDocument,
) -> dict[str, int]:
    expected_domains = _call_contract_expected_domain_revisions(args)
    vector = ProgressSQLiteStore(Path(args.progress)).read_revision_vector()
    observed = {
        "semantic": vector.semantic_revision,
        "evidence_generation": vector.evidence_generation_revision,
    }
    if observed != expected_domains:
        raise ConcurrentProgressUpdate(
            "call-contract revision domains changed: expected "
            f"{expected_domains}, found {observed}"
        )
    return expected_domains


def _precheck_scheduler_revision(
    args: argparse.Namespace,
) -> tuple[ProgressDocument | None, dict[str, int] | None]:
    expected_scheduler = getattr(args, "expected_scheduler_revision", None)
    legacy_revision = getattr(args, "expected_revision", None)
    if expected_scheduler is not None and legacy_revision is not None:
        raise ProgressError(
            "scheduler-domain CAS rejects mixed --expected-scheduler-revision "
            "and --expected-revision guards"
        )
    if expected_scheduler is None:
        if legacy_revision is None:
            raise ProgressError(
                "provide --expected-scheduler-revision"
            )
        progress_path = Path(args.progress)
        if not progress_path.is_absolute():
            raise ProgressError(
                "legacy --expected-revision is fixture-only and requires an explicit "
                "absolute noncanonical progress fixture"
            )
        try:
            supplied = progress_path.resolve(strict=True)
            canonical = Path(DEFAULT_PROGRESS).resolve()
        except OSError as exc:
            raise ProgressError(
                "legacy --expected-revision requires a readable schema-valid fixture"
            ) from exc
        if supplied == canonical:
            raise ProgressError(
                "live canonical progress mutations require --expected-scheduler-revision"
            )
        try:
            ProgressStore(supplied).load()
        except (OSError, ValueError, ProgressSQLiteError) as exc:
            raise ProgressError(
                "legacy --expected-revision requires a schema-valid progress fixture"
            ) from exc
        return None, None
    document = ProgressStore(args.progress).load()
    vector = ProgressSQLiteStore(Path(args.progress)).read_revision_vector()
    if vector.scheduler_revision != int(expected_scheduler):
        raise ConcurrentProgressUpdate(
            "scheduler revision changed: expected "
            f"{expected_scheduler}, found {vector.scheduler_revision}"
        )
    return document, {"scheduler": int(expected_scheduler)}


def _scheduler_revision_for_document(document: ProgressDocument) -> int:
    path = getattr(document, "path", None)
    if isinstance(path, Path) and path.suffix.casefold() == ".sqlite3" and path.exists():
        try:
            return int(read_progress_metadata(path).scheduler_revision)
        except (OSError, ValueError):
            pass
    return int(document.revision)


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Inspect unified progress and atomically advance it from live validators."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    for name, help_text in (
        ("next", "Select the authoritative next Recoil.exe task."),
        ("status", "Show derived pipeline status."),
        ("report", "Render an on-demand progress summary."),
    ):
        child = subparsers.add_parser(name, help=help_text)
        _add_progress_path(child)
        if name == "next":
            child.add_argument(
                "--issue-ledger",
                type=Path,
                default=DEFAULT_ISSUE_LEDGER,
            )
        if name == "status":
            child.add_argument("selector", nargs="?")
        child.add_argument("--binary", default="recoil")
        child.add_argument("--json", action="store_true")

    show = subparsers.add_parser("show", help="Show a joined entity/address view.")
    _add_progress_path(show)
    show.add_argument("selector")
    show.add_argument("--json", action="store_true")

    find = subparsers.add_parser("find", help="Search all progress entities.")
    _add_progress_path(find)
    find.add_argument("query")
    find.add_argument("--limit", type=int, default=100)
    find.add_argument("--json", action="store_true")

    audit = subparsers.add_parser("audit", help="Audit schema-v5 progress invariants.")
    _add_progress_path(audit)
    audit.add_argument("--scope", default="all")
    audit.add_argument("--strict", action="store_true")
    audit.add_argument("--json", action="store_true")

    compact = subparsers.add_parser(
        "compact",
        help="Parent-only active-only schema-v5 tracker compaction.",
    )
    _add_progress_path(compact)
    _add_mutation_controls(compact)

    handoff = subparsers.add_parser("handoff", help="Render the current structured handoff.")
    _add_progress_path(handoff)
    selection = handoff.add_mutually_exclusive_group()
    selection.add_argument("--packet-id")
    selection.add_argument("--authored-byte", action="store_true")
    selection.add_argument("--fallback-authored-byte", action="store_true")
    selection.add_argument("--authored-object-byte", action="store_true")
    handoff.add_argument("--json", action="store_true")
    handoff.add_argument(
        "--issue-ledger",
        type=Path,
        default=DEFAULT_ISSUE_LEDGER,
    )

    for name in ("output-section", "storage", "block", "semantic"):
        parent = subparsers.add_parser(name, help=f"Inspect {name} records.")
        children = parent.add_subparsers(dest=f"{name.replace('-', '_')}_command", required=True)
        child = children.add_parser("show")
        _add_progress_path(child)
        child.add_argument("selector")
        child.add_argument("--json", action="store_true")
        if name == "storage":
            register_authored_data = children.add_parser(
                "register-authored-data",
                help=(
                    "Parent-only registration of one exact non-overlapping "
                    "authored data-symbol storage contribution."
                ),
            )
            _add_progress_path(register_authored_data)
            authored_storage_payload = (
                register_authored_data.add_mutually_exclusive_group(
                    required=True
                )
            )
            authored_storage_payload.add_argument(
                "--payload-json",
                help=(
                    "One recoil-authored-data-storage-register-v1 object "
                    "guarding the exact existing data symbol and primary-data "
                    "owner relationship."
                ),
            )
            authored_storage_payload.add_argument(
                "--payload-file",
                type=Path,
                help=(
                    "Path to the same reviewed UTF-8 payload object."
                ),
            )
            _add_mutation_controls(register_authored_data)
        if name == "block":
            provider_reclassify = children.add_parser(
                "reclassify-provider",
                help=(
                    "Parent-only reviewed reclassification of one exact stale authored "
                    "physical block to a provider boundary."
                ),
            )
            _add_progress_path(provider_reclassify)
            provider_reclassify.add_argument(
                "--payload-json",
                required=True,
                help=(
                    "One recoil-provider-block-reclassify-v1 object containing the complete "
                    "current block snapshot, exact accepted provider-owner ids, explicit "
                    "placement-clearing acknowledgement, and replacement provider labels."
                ),
            )
            _add_mutation_controls(provider_reclassify)
            accept_non_gating = children.add_parser(
                "accept-authored-non-gating",
                help=(
                    "Parent-only reviewed acceptance of one exact contiguous live-cursor "
                    "batch whose physical blocks contain no authored gating identities."
                ),
            )
            _add_progress_path(accept_non_gating)
            accept_payload = accept_non_gating.add_mutually_exclusive_group(required=True)
            accept_payload.add_argument(
                "--payload-json",
                help=(
                    "One recoil-authored-non-gating-block-accept-v1 object containing "
                    "complete current block snapshots and the exact expected cursor-after."
                ),
            )
            accept_payload.add_argument(
                "--payload-file",
                type=Path,
                help=(
                    "Path to the same v1 JSON object; use this for complete snapshots whose "
                    "membership inventory exceeds the Windows command-line limit."
                ),
            )
            _add_mutation_controls(accept_non_gating)
            replace_block = children.add_parser(
                "replace",
                help=(
                    "Parent-only reviewed replacement of one exact physical block, "
                    "its symbol assignments, and its semantic spans."
                ),
            )
            _add_progress_path(replace_block)
            replace_payload = replace_block.add_mutually_exclusive_group(required=True)
            replace_payload.add_argument(
                "--payload-json",
                help=(
                    "One recoil-physical-block-replace-v1 object containing exact current "
                    "block guards, contiguous replacements, and complete semantic assignments."
                ),
            )
            replace_payload.add_argument(
                "--payload-file",
                type=Path,
                help=(
                    "Path under workspace build/ to the same UTF-8 v1 JSON object; use this "
                    "for reviewed replacement packets that exceed the Windows command-line limit."
                ),
            )
            _add_mutation_controls(replace_block)

    symbol = subparsers.add_parser(
        "symbol",
        help="Apply reviewed function-row classification mutations.",
    )
    symbol_children = symbol.add_subparsers(dest="symbol_command", required=True)
    symbol_class_batch = symbol_children.add_parser(
        "set-pipeline-class-batch",
        help="Set exact reviewed pipeline classifications with current-value staleness guards.",
    )
    _add_progress_path(symbol_class_batch)
    symbol_class_batch.add_argument(
        "--payload-json",
        required=True,
        help=(
            "Non-empty JSON array of exact symbol_id/address rows with reviewed=true, "
            "current_pipeline_class/current_authored_order_role, and replacement "
            "pipeline_class/authored_order_role."
        ),
    )
    _add_mutation_controls(symbol_class_batch)
    symbol_alias_group = symbol_children.add_parser(
        "set-logical-alias-group",
        help=(
            "Parent-only reviewed ICF logical-alias group mutation with exact "
            "current-state guards."
        ),
    )
    _add_progress_path(symbol_alias_group)
    symbol_alias_payload = symbol_alias_group.add_mutually_exclusive_group(required=True)
    symbol_alias_payload.add_argument(
        "--payload-json",
        help=(
            "One recoil-logical-alias-group-v1, v2, v3, or v4 object containing an exact "
            "physical function row/current-state snapshot and either one selected "
            "winner plus proven authored fold aliases, or a winner-unknown group "
            "containing only proven authored fold aliases. V2 atomically creates one "
            "candidate-independent current evidence row, derives its exact physical/"
            "alias/owner scope, and assigns its generated id to the group and aliases. "
            "V3 refreshes only an existing winner-unknown group: its full current "
            "group/alias snapshot is a deep-CAS guard, recovered or provisional name "
            "status is preserved without upgrade, exactly one synchronized complete "
            "governed target is required, and only generated evidence ids may change."
        ),
    )
    symbol_alias_payload.add_argument(
        "--payload-file",
        type=Path,
        help=(
            "Path under workspace build/ to the same UTF-8 v1, v2, v3, or v4 JSON "
            "object; use this for reviewed logical-alias packets that exceed the "
            "Windows command-line limit."
        ),
    )
    _add_mutation_controls(symbol_alias_group)
    symbol_replace_padding = symbol_children.add_parser(
        "replace-padding",
        help=(
            "Parent-only reviewed removal of one false function identity whose exact "
            "immutable-retail extent is NOP padding."
        ),
    )
    _add_progress_path(symbol_replace_padding)
    symbol_replace_padding.add_argument(
        "--payload-json",
        required=True,
        help=(
            "One recoil-function-padding-correction-v1 object with exact current "
            "function/block/span guards and an exact replacement_padding record."
        ),
    )
    _add_mutation_controls(symbol_replace_padding)

    owner = subparsers.add_parser("owner", help="Inspect source-owner records.")
    owner_children = owner.add_subparsers(dest="owner_command", required=True)
    for name in ("show", "relationships"):
        child = owner_children.add_parser(name)
        _add_progress_path(child)
        child.add_argument("selector")
        child.add_argument("--json", action="store_true")
    owner_find = owner_children.add_parser("find")
    _add_progress_path(owner_find)
    owner_find.add_argument("query")
    owner_find.add_argument("--limit", type=int, default=100)
    owner_find.add_argument("--json", action="store_true")
    owner_audit = owner_children.add_parser("audit")
    _add_progress_path(owner_audit)
    owner_audit.add_argument("--strict", action="store_true")
    owner_audit.add_argument("--json", action="store_true")
    owner_repair_data_tier = owner_children.add_parser(
        "repair-primary-data-tier-x",
        help=(
            "Parent-only conservative initialization of absent tier-X records for "
            "exact existing same-owner authored primary-data relationships."
        ),
    )
    _add_progress_path(owner_repair_data_tier)
    owner_repair_data_tier.add_argument(
        "--payload-json",
        required=True,
        help=(
            "One recoil-owner-primary-data-tier-x-repair-v1 object guarding the "
            "exact current owner and complete primary-data relationship rows."
        ),
    )
    _add_mutation_controls(owner_repair_data_tier)
    owner_replace = owner_children.add_parser(
        "replace-batch",
        help=(
            "Parent-only atomic replacement of exact reviewed current owner records, "
            "including guarded primary-function/data reassignment and stale tier removal."
        ),
    )
    _add_progress_path(owner_replace)
    owner_replace_payload = owner_replace.add_mutually_exclusive_group(required=True)
    owner_replace_payload.add_argument(
        "--payload-json",
        help=(
            "One recoil-owner-replace-batch-v1 or v2 object containing exact current "
            "owner snapshots and complete replacement owner records."
        ),
    )
    owner_replace_payload.add_argument(
        "--payload-file",
        type=Path,
        help=(
            "Path under workspace build/ to the same UTF-8 v1 or v2 JSON object; use "
            "this for reviewed owner snapshots that exceed the Windows command-line limit."
        ),
    )
    _add_mutation_controls(owner_replace)
    owner_downgrade = owner_children.add_parser(
        "downgrade",
        help=(
            "Parent-only atomic conservative downgrade of selected gates and "
            "primary-entry tiers on one exact current authored owner."
        ),
    )
    _add_progress_path(owner_downgrade)
    owner_downgrade.add_argument(
        "--payload-json",
        required=True,
        help=(
            "One recoil-owner-downgrade-v1 object with exact current gate states "
            "and primary-entry tiers plus strictly lower replacement states."
        ),
    )
    _add_mutation_controls(owner_downgrade)

    work = subparsers.add_parser("work", help="Inspect and reserve structured work packets.")
    work_children = work.add_subparsers(dest="work_command", required=True)
    work_show = work_children.add_parser(
        "show",
        help="Show one exact structured work item.",
    )
    _add_progress_path(work_show)
    work_show.add_argument(
        "work_item_id",
        help="Exact structured work-item id.",
    )
    work_show.add_argument("--json", action="store_true")
    work_leases = work_children.add_parser("leases")
    _add_progress_path(work_leases)
    work_leases.add_argument("--id")
    work_leases.add_argument(
        "--issue-ledger",
        type=Path,
        default=DEFAULT_ISSUE_LEDGER,
    )
    work_leases.add_argument("--json", action="store_true")
    work_claim = work_children.add_parser(
        "claim-current",
        help=(
            "Atomically create and reserve current compact live worker packets "
            "with deterministic claim provenance."
        ),
    )
    _add_progress_path(work_claim)
    work_claim.add_argument("--lane", choices=CLAIM_LANES, default="primary")
    work_claim.add_argument(
        "--max-packets",
        type=int,
        default=3,
        help="Maximum compatible packets to reserve when --lane all is selected.",
    )
    work_claim.add_argument(
        "--issue-ledger",
        type=Path,
        default=DEFAULT_ISSUE_LEDGER,
    )
    _add_mutation_controls(work_claim, expected_revision_mode="none")
    claim_revision_guard = work_claim.add_mutually_exclusive_group(required=True)
    claim_revision_guard.add_argument("--expected-scheduler-revision", type=int)
    claim_revision_guard.add_argument(
        "--expected-revision",
        type=int,
        help=argparse.SUPPRESS,
    )
    work_explicit = work_children.add_parser(
        "create-explicit",
        help=(
            "Parent-only atomic construction and reservation of one exact "
            "user-selected maintenance or diagnostic packet."
        ),
    )
    _add_progress_path(work_explicit)
    explicit_payload = work_explicit.add_mutually_exclusive_group(required=True)
    explicit_payload.add_argument(
        "--payload-json",
        help="One recoil-explicit-maintenance-packet-v1 object.",
    )
    explicit_payload.add_argument(
        "--payload-file",
        type=Path,
        help="UTF-8 v1 payload under workspace build/.",
    )
    work_explicit.add_argument(
        "--issue-ledger", type=Path, default=DEFAULT_ISSUE_LEDGER
    )
    _add_mutation_controls(work_explicit, expected_revision_required=False)
    work_explicit.add_argument("--expected-scheduler-revision", type=int)
    work_explicit.add_argument("--expected-semantic-revision", type=int)
    work_reserve = work_children.add_parser("reserve")
    _add_progress_path(work_reserve)
    work_reserve.add_argument("--id", required=True)
    work_reserve.add_argument(
        "--issue-ledger", type=Path, default=DEFAULT_ISSUE_LEDGER
    )
    _add_mutation_controls(work_reserve, expected_revision_required=False)
    work_reserve.add_argument("--expected-scheduler-revision", type=int)
    work_close = work_children.add_parser("close")
    _add_progress_path(work_close)
    work_close.add_argument("work_id")
    work_close.add_argument(
        "--outcome",
        choices=("returned", "returned-tool-blocked", "closed", "abandoned"),
        default="closed",
    )
    work_close.add_argument("--abandonment-reason")
    work_close.add_argument("--linked-tool-issue")
    work_close.add_argument(
        "--issue-ledger",
        type=Path,
        default=DEFAULT_ISSUE_LEDGER,
    )
    _add_mutation_controls(work_close, expected_revision_required=False)
    work_close.add_argument("--expected-scheduler-revision", type=int)
    work_return = work_children.add_parser(
        "return", help="Return one active explicit packet with bounded nonaccepting feedback."
    )
    _add_progress_path(work_return)
    work_return.add_argument("--id", required=True)
    work_return.add_argument("--result-json", required=True)
    work_return.add_argument(
        "--issue-ledger", type=Path, default=DEFAULT_ISSUE_LEDGER
    )
    _add_mutation_controls(work_return, expected_revision_required=False)
    work_return.add_argument("--expected-scheduler-revision", type=int)
    work_return_binja = work_children.add_parser(
        "return-binja",
        help=(
            "Return one active BN-enabled explicit packet after one bounded, "
            "reservation-authenticated governed read plan."
        ),
    )
    _add_progress_path(work_return_binja)
    work_return_binja.add_argument("--id", required=True)
    work_return_binja.add_argument("--read-plan-json", required=True)
    work_return_binja.add_argument("--result-json", required=True)
    _add_mutation_controls(work_return_binja, expected_revision_required=False)
    work_return_binja.add_argument("--expected-scheduler-revision", type=int)
    work_recover = work_children.add_parser(
        "recover-expired", help="Release one expired explicit reservation back to ready."
    )
    _add_progress_path(work_recover)
    work_recover.add_argument("--id", required=True)
    _add_mutation_controls(work_recover, expected_revision_required=False)
    work_recover.add_argument("--expected-scheduler-revision", type=int)
    work_recover_allocation = work_children.add_parser(
        "recover-allocation",
        help=(
            "Independently authenticate and recover one failed explicit output allocation."
        ),
    )
    _add_progress_path(work_recover_allocation)
    work_recover_allocation.add_argument("--id", required=True)
    work_recover_allocation.add_argument(
        "--issue-ledger", type=Path, default=DEFAULT_ISSUE_LEDGER
    )
    _add_mutation_controls(
        work_recover_allocation, expected_revision_required=False
    )
    work_recover_allocation.add_argument("--expected-scheduler-revision", type=int)
    work_recover_allocation.add_argument("--expected-semantic-revision", type=int)

    verification_target = subparsers.add_parser(
        "verification-target",
        help="Refresh semantic verification-target registrations.",
    )
    verification_children = verification_target.add_subparsers(
        dest="verification_target_command",
        required=True,
    )
    verification_sync = verification_children.add_parser("sync")
    _add_progress_path(verification_sync)
    verification_sync.add_argument("--binary", default="recoil")
    verification_sync.add_argument("--target", action="append", default=[])
    verification_sync.add_argument(
        "--source-policy-bootstrap",
        action="store_true",
        help=(
            "Parent-only registration bootstrap for one reviewed complete order target "
            "whose exact order_edit_paths closure must be edited before final source "
            "placement can satisfy the ordinary verifier."
        ),
    )
    verification_sync.add_argument(
        "--revalidate-accepted-order",
        action="store_true",
        help=(
            "Explicitly invalidate accepted order facts for exactly one existing "
            "selected VC5 target even when its registration is already current."
        ),
    )
    _add_mutation_controls(verification_sync)
    verification_retire = verification_children.add_parser(
        "retire",
        help=(
            "Retire exactly one stale verification-target registration and "
            "invalidate its dependent tracker facts."
        ),
    )
    _add_progress_path(verification_retire)
    verification_retire.add_argument(
        "--target",
        required=True,
        help=(
            "Exact tracker verification-target id (preferred), or one globally "
            "unique registered target name."
        ),
    )
    _add_mutation_controls(verification_retire)

    advance_order = subparsers.add_parser(
        "advance-live-order",
        help="Run the registered VC5 order validator and commit its safe semantic prefix.",
    )
    _add_progress_path(advance_order)
    advance_order.add_argument("--target", required=True)
    advance_order.add_argument(
        "--object-target",
        help=(
            "Exact registered object compile target: authored-order override during authored "
            "order, and required worker-target identity paired with linked acceptance during "
            "full order."
        ),
    )
    advance_order.add_argument(
        "--linked-target",
        help="Exact registered full-order target override for an empty or stale block binding.",
    )
    advance_order.add_argument("--build-root", type=Path, required=True)
    _add_mutation_controls(advance_order)

    advance_byte = subparsers.add_parser(
        "advance-live-byte",
        help="Run one live byte lane and commit only explicitly matched physical groups.",
    )
    _add_progress_path(advance_byte)
    advance_byte.add_argument("--lane", choices=sorted(BYTE_LANES), required=True)
    advance_byte.add_argument("--build-root", type=Path, required=True)
    _add_mutation_controls(advance_byte)

    advance_call_contract = subparsers.add_parser(
        "advance-live-call-contract",
        help=(
            "Run the current VC5/Binary Ninja authored invocation-contract "
            "validator and commit only call_contract state."
        ),
    )
    _add_progress_path(advance_call_contract)
    advance_call_contract.add_argument("--slice", required=True)
    advance_call_contract.add_argument(
        "--packet-id",
        required=True,
        help="Active reservation owning the exact build root and BN read resource.",
    )
    advance_call_contract.add_argument("--build-root", type=Path, required=True)
    _add_mutation_controls(
        advance_call_contract,
        expected_revision_required=False,
        expected_revision_mode="none",
    )
    _add_call_contract_domain_guards(advance_call_contract)

    call_contract = subparsers.add_parser(
        "call-contract",
        help="Parent-owned initialization of the permanent authored call-contract state.",
    )
    call_contract_children = call_contract.add_subparsers(
        dest="call_contract_command",
        required=True,
    )
    call_contract_initialize = call_contract_children.add_parser(
        "initialize",
        help=(
            "Revision-atomically initialize exactly the reviewed 3,380 authored "
            "gating bodies without changing order or byte facts."
        ),
    )
    _add_progress_path(call_contract_initialize)
    _add_mutation_controls(call_contract_initialize)

    call_contract_convergence = call_contract_children.add_parser(
        "prepare-live-convergence",
        help=(
            "Refresh the exact affected target/consumer fixed point when a strict "
            "current base is safe, otherwise compile the full accepted census, and "
            "record one nonaccepting repair generation."
        ),
    )
    _add_progress_path(call_contract_convergence)
    call_contract_convergence.add_argument(
        "--packet-id",
        required=True,
        help=(
            "Active explicit reservation owning the convergence build root, "
            "whole-project build window, and governed BN reader."
        ),
    )
    call_contract_convergence.add_argument("--build-root", type=Path, required=True)
    call_contract_convergence.add_argument("--jobs", type=int, required=True)
    call_contract_convergence.add_argument(
        "--closeout",
        action="store_true",
        help=(
            "Require a fresh no-reuse, globally clean complete-census scan and "
            "bind it to every current per-body verification before phase transition."
        ),
    )
    call_contract_convergence.add_argument(
        "--issue-ledger",
        type=Path,
        default=DEFAULT_ISSUE_LEDGER,
    )
    _add_mutation_controls(
        call_contract_convergence,
        expected_revision_required=False,
        expected_revision_mode="none",
    )
    _add_call_contract_domain_guards(call_contract_convergence)

    repair_continuation = call_contract_children.add_parser(
        "prepare-repair-continuation",
        help=(
            "Run one fresh exact-target diagnostic for a retained terminal "
            "tool-blocked predecessor and create at most one same-scope "
            "nonaccepting continuation packet."
        ),
    )
    _add_progress_path(repair_continuation)
    repair_continuation.add_argument("--producer-packet", required=True)
    repair_continuation.add_argument("--returned-work-item", required=True)
    repair_continuation.add_argument("--build-root", type=Path, required=True)
    repair_continuation.add_argument(
        "--issue-ledger",
        type=Path,
        default=DEFAULT_ISSUE_LEDGER,
    )
    _add_mutation_controls(repair_continuation)

    return parser


def _load(path: Path) -> ProgressDocument:
    return ProgressStore(path).load()


def _print_json(value: Any) -> None:
    print(json.dumps(value, indent=2, ensure_ascii=False))


def _print_pipeline(value: Mapping[str, Any]) -> None:
    for key in (
        "phase",
        "primary_lane",
        "cursor",
        "physical_block_id",
        "authored_order_prefix_end",
        "authored_byte_cursor",
        "full_order_prefix_end",
        "linked_byte_match_prefix_end",
        "parallel_authored_byte_cursor",
        "parallel_authored_object_byte_cursor",
        "next_command",
        "tracker_revision",
    ):
        if key in value:
            print(f"{key}={value[key]}")


def _scheduler_domain_guarded_call_contract_commands(
    document: ProgressDocument | None,
    payload: Mapping[str, Any],
) -> dict[str, Any]:
    """Project scheduler-authored call-contract commands onto revision domains.

    Legacy command parsing remains available, but no newly rendered SQLite
    scheduler/status route may collapse semantic and evidence-generation CAS
    back into the transaction revision.
    """

    stored_vector = payload.get("revision_vector")
    if (
        isinstance(stored_vector, Mapping)
        and all(
            isinstance(stored_vector.get(key), int)
            and not isinstance(stored_vector.get(key), bool)
            for key in (
                "semantic_revision",
                "evidence_generation_revision",
                "scheduler_revision",
            )
        )
    ):
        semantic_revision = int(stored_vector["semantic_revision"])
        evidence_generation_revision = int(
            stored_vector["evidence_generation_revision"]
        )
    else:
        path = getattr(document, "path", None)
        if not isinstance(path, Path) or path.suffix.casefold() != ".sqlite3":
            return deepcopy(dict(payload))
        vector = ProgressSQLiteStore(path).read_revision_vector()
        semantic_revision = int(vector.semantic_revision)
        evidence_generation_revision = int(vector.evidence_generation_revision)
    guard = (
        f"--expected-semantic-revision {semantic_revision} "
        "--expected-evidence-generation-revision "
        f"{evidence_generation_revision}"
    )

    def project(value: Any) -> Any:
        if isinstance(value, Mapping):
            return {str(key): project(item) for key, item in value.items()}
        if isinstance(value, list):
            return [project(item) for item in value]
        if not isinstance(value, str):
            return deepcopy(value)
        if not (
            "progress advance-live-call-contract" in value
            or "progress call-contract prepare-live-convergence" in value
        ):
            return value
        if "--apply" in value and not re.search(r"--packet-id\s+\S+", value):
            raise ProgressError(
                "scheduler call-contract applying command lacks an active packet identity"
            )
        legacy_guards = re.findall(r"--expected-revision\s+\d+", value)
        semantic_guards = re.findall(
            r"--expected-semantic-revision\s+\d+", value
        )
        evidence_guards = re.findall(
            r"--expected-evidence-generation-revision\s+\d+", value
        )
        if semantic_guards or evidence_guards:
            if (
                legacy_guards
                or len(semantic_guards) != 1
                or len(evidence_guards) != 1
            ):
                raise ProgressError(
                    "scheduler call-contract command has mixed or incomplete domain guards"
                )
            rewritten, semantic_count = re.subn(
                r"--expected-semantic-revision\s+\d+",
                f"--expected-semantic-revision {semantic_revision}",
                value,
                count=1,
            )
            rewritten, evidence_count = re.subn(
                r"--expected-evidence-generation-revision\s+\d+",
                "--expected-evidence-generation-revision "
                f"{evidence_generation_revision}",
                rewritten,
                count=1,
            )
            count = int(semantic_count == 1 and evidence_count == 1)
        else:
            if len(legacy_guards) != 1:
                raise ProgressError(
                    "scheduler call-contract command lacks one complete revision guard"
                )
            rewritten, count = re.subn(
                r"--expected-revision\s+\d+",
                guard,
                value,
                count=1,
            )
        if count != 1:
            raise ProgressError(
                "scheduler call-contract command lacks one complete revision guard"
            )
        return rewritten

    return project(payload)


def _commit_payload(commit: Any, details: Mapping[str, Any]) -> dict[str, Any]:
    return {"commit": commit.to_dict(), **deepcopy(dict(details))}


def _resolve_collection_row(document: ProgressDocument, collection: str, selector: str) -> dict[str, Any]:
    rows = document.collection(collection)
    row = rows.get(selector)
    if isinstance(row, dict):
        return {"id": selector, "record": deepcopy(row)}
    joined = document.show(selector)
    matches = [item for item in joined["matches"] if item["collection"] == collection]
    if len(matches) != 1:
        raise ProgressError(f"expected one {collection} row for {selector!r}, found {len(matches)}")
    return {"id": matches[0]["id"], "record": matches[0]["record"]}


def _owner_logical_function_members(
    document: ProgressDocument,
    owner_id: str,
) -> list[dict[str, Any]]:
    members: list[dict[str, Any]] = []
    for physical_symbol_id, symbol in document.collection("symbols").items():
        if not isinstance(symbol, Mapping):
            continue
        aliases = symbol.get("logical_aliases")
        if not isinstance(aliases, Mapping):
            continue
        for logical_identity_key, alias in aliases.items():
            if not isinstance(alias, Mapping) or alias.get("owner_id") != owner_id:
                continue
            members.append(
                {
                    "logical_identity_key": str(logical_identity_key),
                    "physical_symbol_id": str(physical_symbol_id),
                    "physical_address": str(symbol.get("address", "")),
                    "original_name": str(alias.get("original_name", "")),
                    "original_name_status": str(alias.get("original_name_status", "")),
                    "pipeline_class": str(alias.get("pipeline_class", "")),
                    "authored_order_role": str(alias.get("authored_order_role", "")),
                    "gate_mode": str(alias.get("gate_mode", "")),
                    "source_traceability": deepcopy(alias.get("source_traceability")),
                }
            )
    return sorted(
        members,
        key=lambda row: (
            row["physical_address"],
            row["logical_identity_key"],
        ),
    )


def _owner_view(document: ProgressDocument, selector: str) -> dict[str, Any]:
    owners = document.collection("owners")
    if selector in owners:
        selected = [(selector, owners[selector])]
    else:
        try:
            point = address_value(selector)
        except ProgressError:
            point = None
        selected = []
        if point is not None:
            symbol_ids = {
                item["id"]
                for item in document.show(selector)["matches"]
                if item["collection"] == "symbols"
            }
            selected = [
                (owner_id, owner)
                for owner_id, owner in owners.items()
                if isinstance(owner, Mapping)
                and (
                    any(
                        isinstance(relationship, Mapping)
                        and relationship.get("kind") == "primary-function"
                        and relationship.get("symbol_id") in symbol_ids
                        for relationship in owner.get("relationships", [])
                    )
                    or any(
                        member["physical_symbol_id"] in symbol_ids
                        for member in _owner_logical_function_members(
                            document, str(owner_id)
                        )
                    )
                )
            ]
    return document.scheduler_output(
        {
            "selector": selector,
            "owners": [
                {
                    "id": owner_id,
                    "record": deepcopy(owner),
                    "derived_tier": document.owner_tier(owner),
                    "logical_function_members": _owner_logical_function_members(
                        document, str(owner_id)
                    ),
                }
                for owner_id, owner in selected
                if isinstance(owner, Mapping)
            ],
        }
    )


def _absolute_fresh_build_root(value: Path) -> Path:
    resolved = (REPO_ROOT / value).resolve() if not value.is_absolute() else value.resolve()
    try:
        resolved.relative_to((REPO_ROOT / "build").resolve())
    except ValueError as exc:
        raise ProgressError("--build-root must resolve below the repository build directory") from exc
    if resolved.exists():
        raise ProgressError(f"--build-root must be fresh and not already exist: {display_path(resolved)}")
    return resolved


def _absolute_allocated_build_root(value: Path) -> Path:
    resolved = (REPO_ROOT / value).resolve() if not value.is_absolute() else value.resolve()
    try:
        resolved.relative_to((REPO_ROOT / "build").resolve())
    except ValueError as exc:
        raise ProgressError("--build-root must resolve below the repository build directory") from exc
    if not resolved.is_dir():
        raise ProgressError(
            f"--build-root must be the packet's allocated directory: {display_path(resolved)}"
        )
    return resolved


def _run_json_process(
    command: list[str],
    *,
    env: Mapping[str, str] | None = None,
) -> tuple[int, dict[str, Any], str]:
    completed = subprocess.run(
        command,
        cwd=REPO_ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
        env=dict(env) if env is not None else None,
    )
    try:
        payload = json.loads(completed.stdout)
    except json.JSONDecodeError as exc:
        detail = completed.stderr.strip() or completed.stdout.strip()[:500]
        raise ProgressError(f"live validator did not emit one JSON object: {detail}") from exc
    if not isinstance(payload, dict):
        raise ProgressError("live validator JSON must be an object")
    return completed.returncode, payload, completed.stderr.strip()


def _order_target_write_paths(
    document: ProgressDocument,
    contract: Mapping[str, Any],
) -> list[str]:
    paths: dict[str, str] = {}
    tracked_inventory = _load_progress_git_inventory(document)

    def add_current_path(value: str) -> None:
        exact = _resolve_tracked_progress_file(
            value,
            context="current order packet writable path",
            inventory=tracked_inventory,
        ).git_path
        paths.setdefault(exact, exact)

    target = contract["target"]
    registration = target.get("registration", {})
    if isinstance(registration, Mapping):
        order_edit_paths = normalize_order_edit_paths(
            registration.get("order_edit_paths"),
            context=f"order target {contract.get('target_id', '<unknown>')}",
        )
        for path in order_edit_paths:
            add_current_path(path)
        bootstrap = registration.get("source_policy_bootstrap")
        if bootstrap is not None:
            if not isinstance(bootstrap, Mapping):
                raise ProgressError("registered source-policy bootstrap state must be an object")
            if (
                bootstrap.get("state") != SOURCE_POLICY_BOOTSTRAP_STATE
                or bootstrap.get("registration_only") is not True
            ):
                raise ProgressError("registered source-policy bootstrap state is invalid")
            bootstrap_closure = normalize_order_edit_paths(
                bootstrap.get("writable_closure"),
                context=(
                    f"order target {contract.get('target_id', '<unknown>')} "
                    "source-policy bootstrap"
                ),
            )
            if bootstrap_closure != order_edit_paths or not bootstrap_closure:
                raise ProgressError(
                    "registered source-policy bootstrap writable closure is stale or incomplete"
                )
        source_from = registration.get("source_from")
        if isinstance(source_from, str) and source_from:
            add_current_path(source_from)
        for entry in registration.get("translation_unit_function_order", []):
            if not isinstance(entry, Mapping):
                continue
            source = entry.get("source_from")
            if isinstance(source, str) and source:
                add_current_path(source)
    current_allowed_paths = frozenset(paths)

    def add_historical_path(value: str) -> None:
        try:
            exact = resolve_tracked_repository_file(
                value,
                repository_root=REPO_ROOT,
                inventory=tracked_inventory,
                context="current order block source path",
            )
        except RepositoryPathError as exc:
            if exc.kind != "wrong-case":
                raise ProgressError(str(exc)) from exc
        else:
            paths.setdefault(exact.git_path, exact.git_path)
            return
        try:
            diagnosis = diagnose_historical_repository_path(
                value,
                inventory=tracked_inventory,
                current_allowed_paths=current_allowed_paths,
                context="historical order block source path",
            )
        except RepositoryPathError as exc:
            raise ProgressError(str(exc)) from exc
        if diagnosis.current_git_path is None:
            raise ProgressError(
                "historical order block source path is absent or ambiguous in "
                f"the current registered writable closure: {value}"
            )
        # A wrong-case historical alias can identify only an already
        # authenticated current registration member for packet deduplication.
        # It never introduces a new current path or rewrites the tracker row.
        paths.setdefault(diagnosis.current_git_path, diagnosis.current_git_path)
    # The worker validates the complete registered translation-unit target even
    # when the scheduler is accepting only one of its separated physical
    # slices.  Keep the writable closure target-wide while block reservations
    # remain limited to the current slice in _order_claim_candidate.
    for block_id in contract.get("target_owned_block_ids", contract["covered_block_ids"]):
        block = document.collection("physical_blocks")[block_id]
        source = block.get("agent_source_path") or block.get("source_path")
        if isinstance(source, str) and source:
            add_historical_path(source)
        for item in block.get("source_shape_inputs", []):
            if not isinstance(item, Mapping):
                continue
            path = item.get("path")
            if isinstance(path, str) and path:
                add_historical_path(path)
    result = sorted(paths.values(), key=lambda path: (path.casefold(), path))
    if not result:
        raise ProgressError("current order target has no writable source/header closure")
    return result


def _call_contract_obligation_packet_contract_problem(
    work: Mapping[str, Any],
    claims: Iterable[Mapping[str, Any]],
) -> str:
    """Return the first mixed-obligation packet contract defect, if any.

    These packets share a target but never share an implicit target mutex.  The
    exact writable paths and isolated build root are therefore security-
    relevant handoff inputs, not merely scheduler hints.
    """

    packet_type = str(work.get("packet_type", ""))
    contract = CALL_CONTRACT_OBLIGATION_PACKET_CONTRACTS.get(packet_type)
    if contract is None:
        return ""
    obligation_id = str(work.get("obligation_id", ""))
    target_id = str(work.get("target_id", ""))
    if not obligation_id or not target_id:
        return "mixed obligation packet lacks an obligation or target identity"
    if (
        work.get("obligation_kind") != contract["obligation_kind"]
        or work.get("handoff_role") != contract["handoff_role"]
    ):
        return "mixed obligation packet kind/role does not match its packet type"
    if (
        work.get("nonaccepting") is not True
        or work.get("acceptance_eligible") is not False
        or work.get("candidate_expected_truth") is not False
        or work.get("worker_acceptance_allowed") is not False
    ):
        return "mixed obligation packet weakens its nonaccepting worker contract"
    if work.get("target_ids") != [target_id]:
        return "mixed obligation packet target_ids do not exactly bind its target"

    allowed_paths = work.get("allowed_paths")
    source_edit_paths = work.get("source_edit_paths")
    dependency_paths = work.get("dependency_paths")
    if not (
        isinstance(allowed_paths, list)
        and allowed_paths
        and all(isinstance(path, str) and path for path in allowed_paths)
        and len(allowed_paths) == len(set(allowed_paths))
        and isinstance(source_edit_paths, list)
        and isinstance(dependency_paths, list)
        and all(isinstance(path, str) and path for path in dependency_paths)
        and len(dependency_paths) == len(set(dependency_paths))
    ):
        return "mixed obligation packet lacks exact unique writable/dependency paths"
    if contract["source_writes"]:
        if source_edit_paths != allowed_paths:
            return "source obligation does not expose its exact source write closure"
    elif source_edit_paths:
        return "non-source obligation incorrectly exposes source edit paths"

    try:
        normalized = normalize_resource_claims(claims)
        normalized_allowed = normalize_resource_claims(
            {"kind": "path", "id": path, "access": "write"}
            for path in allowed_paths
        )
        normalized_dependencies = normalize_resource_claims(
            {"kind": "path", "id": path, "access": "read"}
            for path in dependency_paths
            if path not in allowed_paths
        )
    except ProgressError:
        return "mixed obligation packet has malformed resource claims"
    path_writes = [
        row
        for row in normalized
        if row["access"] == "write"
        and row["kind"] in {"path", "source-path", "header-path"}
    ]
    path_reads = [
        row
        for row in normalized
        if row["access"] == "read"
        and row["kind"] in {"path", "source-path", "header-path"}
    ]
    if path_writes != normalized_allowed:
        return "mixed obligation packet writable claims do not equal allowed_paths"
    if path_reads != normalized_dependencies:
        return "mixed obligation packet dependency claims do not equal dependency_paths"
    output_writes = [
        row
        for row in normalized
        if row["access"] == "write" and row["kind"] == "output-root"
    ]
    if len(output_writes) != 1:
        return "mixed obligation packet requires one isolated writable build root"
    required_reads = {
        ("verification-target", target_id),
        ("reference", "support/Recoil.exe"),
        ("tracker", "recoil"),
    }
    actual_reads = {
        (row["kind"], row["id"])
        for row in normalized
        if row["access"] == "read"
    }
    if not required_reads.issubset(actual_reads):
        return "mixed obligation packet lacks required target/retail/tracker reads"
    requires_binary_ninja = work.get("requires_binary_ninja") is True
    has_binary_ninja = ("binary-ninja-db", "Recoil.bndb") in actual_reads
    if requires_binary_ninja != has_binary_ninja:
        return "mixed obligation packet Binary Ninja claim does not match its route"
    return ""


def _compact_reserved_packet(
    work_id: str,
    work: Mapping[str, Any],
    *,
    progress_path: str | Path | None = None,
) -> dict[str, Any]:
    claims, complete, _source = work_resource_claims(work)
    if not complete or not claims:
        raise ProgressError(f"active work item {work_id} has incomplete resource claims")
    reservation = work.get("reservation")
    if not isinstance(reservation, Mapping) or reservation.get("state") != "active":
        raise ProgressError(f"work item {work_id} has no active reservation")
    reservation_id = reservation.get("id")
    if not isinstance(reservation_id, str) or not reservation_id:
        raise ProgressError(f"work item {work_id} reservation has no stable id")
    validation_commands = work.get("validation_commands")
    if (
        not isinstance(validation_commands, list)
        or len(validation_commands) != 1
        or not isinstance(validation_commands[0], str)
        or not validation_commands[0].strip()
    ):
        raise ProgressError(f"work item {work_id} requires exactly one worker validation command")
    worker_command = validation_commands[0].strip()
    try:
        command_tokens = authenticated_validation_command_tokens(
            worker_command,
            require_public_route=False,
            resource_claims=claims,
        )
    except WorktreeControlError as exc:
        raise ProgressError(f"work item {work_id} validation command is invalid: {exc}") from exc
    write_paths = sorted(
        claim["id"]
        for claim in claims
        if claim["access"] == "write"
        and claim["kind"] in {"path", "source-path", "header-path"}
    )
    retail_fact_packet = work.get("packet_type") == RETAIL_FACT_PACKET_TYPE
    explicit_packet = work.get("packet_type") == EXPLICIT_MAINTENANCE_PACKET_TYPE
    continuation_producer = work.get("packet_type") == CONTINUATION_PRODUCER_TYPE
    obligation_contract = CALL_CONTRACT_OBLIGATION_PACKET_CONTRACTS.get(
        str(work.get("packet_type", ""))
    )
    native_git_execution_context: dict[str, Any] | None = None
    if retail_fact_packet:
        problem = retail_fact_packet_contract_problem(work, claims)
        if problem:
            raise ProgressError(f"work item {work_id} {problem}")
        if write_paths:
            raise ProgressError(
                f"work item {work_id} retail fact packet exposes writable source/header paths"
            )
    elif obligation_contract is not None:
        problem = _call_contract_obligation_packet_contract_problem(work, claims)
        if problem:
            raise ProgressError(f"work item {work_id} {problem}")
        if not write_paths:
            raise ProgressError(
                f"work item {work_id} mixed obligation has no governed writable paths"
            )
    elif explicit_packet:
        if not (
            work.get("nonaccepting") is True
            and work.get("acceptance_eligible") is False
            and work.get("worker_acceptance_allowed") is False
            and work.get("candidate_expected_truth") is False
        ):
            raise ProgressError(f"work item {work_id} weakens the explicit nonaccepting contract")
        if work.get("explicit_kind") == "source-maintenance" and not write_paths:
            raise ProgressError(f"work item {work_id} source maintenance has no write closure")
        if work.get("explicit_kind") == "read-only-diagnostic" and write_paths:
            raise ProgressError(f"work item {work_id} diagnostic exposes source writes")
        provenance = work.get("explicit_provenance")
        allocation = (
            provenance.get("output_allocation")
            if isinstance(provenance, Mapping)
            else None
        )
        if not isinstance(allocation, Mapping):
            raise ProgressError(
                f"work item {work_id} has no authenticated output allocation"
            )
        if progress_path is None:
            raise ProgressError(
                f"work item {work_id} output authentication lacks its tracker authority"
            )
        authenticate_explicit_output_root(work, progress_path=progress_path)
    elif continuation_producer:
        if not (
            work.get("branchless") is True
            and work.get("nonaccepting") is True
            and work.get("acceptance_eligible") is False
            and work.get("candidate_expected_truth") is False
            and not write_paths
        ):
            raise ProgressError(f"work item {work_id} weakens the branchless producer contract")
    elif not write_paths:
        raise ProgressError(f"work item {work_id} has no writable source/header paths")
    if write_paths and not explicit_packet:
        if work.get("packet_contract_version") != 4:
            raise ProgressError(
                f"active tracked-write work item {work_id} is a terminal legacy packet and cannot relaunch"
            )
        if work.get("progress_packet_adapter") == "native-git-v1-planned":
            raise ProgressError(
                f"active tracked-write work item {work_id} has only a planned "
                "native-git-v1 allocation and is not handoff-visible"
            )
        binding = work.get("native_git")
        if not isinstance(binding, Mapping) or binding.get("adapter") != "native-git-v1":
            raise ProgressError(f"active tracked-write work item {work_id} lacks native-git-v1")
        descriptor = binding.get("git_workspace_baseline")
        association = binding.get("association")
        if not isinstance(descriptor, Mapping) or not isinstance(association, Mapping):
            raise ProgressError(f"work item {work_id} native Git binding is incomplete")
        try:
            packet_root, observed = resolve_exact_packet_worktree(
                REPO_ROOT,
                descriptor,
                packet_id=work_id,
                writable_paths=write_paths,
                authority="progress",
            )
            if observed is None or observed.to_dict() != dict(association):
                raise ProgressError(f"work item {work_id} worktree association changed")
            baseline = reauthenticate_clean_git_baseline(
                packet_root,
                descriptor,
                packet_id=work_id,
                writable_paths=write_paths,
            )
            authenticate_build_root(
                observed.external_build_root,
                authority="progress",
                packet_id=work_id,
                branch=str(descriptor.get("branch", "")),
                worktree_root=packet_root,
            )
            if "{progress_path}" in worker_command:
                if progress_path is None:
                    raise ProgressError(
                        f"work item {work_id} has an unbound progress authority placeholder"
                    )
                worker_command = _bind_native_git_progress_authority(
                    worker_command,
                    Path(progress_path),
                )
                command_tokens = authenticated_validation_command_tokens(
                    worker_command,
                    require_public_route=False,
                    resource_claims=claims,
                )
            _validate_native_git_progress_authority(
                command_tokens,
                progress_path=progress_path,
            )
            native_git_execution_context = {
                "baseline_schema": baseline.get("schema"),
                "baseline_commit": baseline.get("baseline_commit"),
                "branch": baseline.get("branch"),
                "repository_root": str(REPO_ROOT.resolve()),
                "worktree_root": str(packet_root.resolve()),
                "external_build_root": str(
                    Path(observed.external_build_root).resolve()
                ),
                "git_object_ids_are_opaque": True,
                "git_restrictions": {
                    "worker_git_mutation_allowed": True,
                    "worker_may_stage_exact_writable_paths": True,
                    "worker_may_create_one_packet_commit": True,
                    "packet_commit_message_must_contain_packet_id": work_id,
                    "worker_branch_worktree_integration_allowed": False,
                    "branch_worktree_merge_integration_parent_owned": True,
                    "writable_closure_only": True,
                },
            }
        except (GitChangeControlError, WorktreeControlError, OSError) as exc:
            raise ProgressError(f"work item {work_id} native Git authentication failed: {exc}") from exc
    raw_claim_provenance = work.get("claim_provenance")
    claim_provenance = (
        validate_claim_provenance(raw_claim_provenance)
        if raw_claim_provenance is not None
        else None
    )
    packet = {
        "packet_type": str(work.get("packet_type") or "live-edit-v1"),
        "packet_id": work_id,
        "reservation_id": reservation_id,
        "handoff_role": str(work.get("handoff_role", "")),
        "phase": str(work.get("phase", "")),
        "lane": str(work.get("lane", "")),
        "byte_lane": str(work.get("byte_lane", "")),
        "cursor": str(work.get("cursor", "")),
        "slice_id": str(work.get("slice_id", "")),
        "target": str(work.get("target_id", "")),
        "linked_target_id": str(work.get("linked_target_id", "")),
        "object_target_id": str(work.get("object_target_id", "")),
        "worker_target_id": str(work.get("worker_target_id", "")),
        "target_ids": list(work.get("target_ids", [])),
        "scope_ids": list(work.get("scope_ids", [])),
        "covered_block_ids": list(work.get("covered_block_ids", [])),
        "write_paths": write_paths,
        "worker_command": worker_command,
        "objective": str(work.get("objective", "")),
        "stop_condition": str(work.get("stop_condition", "")),
        "required_return_fields": list(work.get("required_return_fields", [])),
    }
    if explicit_packet:
        provenance = work.get("explicit_provenance")
        if not isinstance(provenance, Mapping) or not isinstance(
            provenance.get("closure"), Mapping
        ):
            raise ProgressError(f"work item {work_id} lacks explicit closure provenance")
        output_roots = [
            claim["id"]
            for claim in claims
            if claim["kind"] == "output-root" and claim["access"] == "write"
        ]
        if len(output_roots) != 1:
            raise ProgressError(f"work item {work_id} requires one isolated output root")
        packet.update(
            {
                "explicit_kind": work.get("explicit_kind"),
                "source_owner_ids": list(work.get("source_owner_ids", [])),
                "related_source_owner_ids": list(
                    work.get("related_source_owner_ids", [])
                ),
                "related_source_owner_snapshot": deepcopy(
                    provenance["closure"]
                    .get("relationship_snapshot", {})
                    .get("related_source_owner_snapshot", [])
                ),
                "reviewed_cross_owner_overrides": deepcopy(
                    provenance["closure"].get(
                        "reviewed_cross_owner_overrides", []
                    )
                ),
                "read_paths": list(provenance["closure"].get("read_only_paths", [])),
                "build_root": output_roots[0],
                "resource_claims": deepcopy(claims),
                "closure": deepcopy(dict(provenance["closure"])),
                "user_selected_rationale": provenance.get("user_selected_rationale"),
                "scheduler_inappropriate_reason": provenance.get(
                    "scheduler_inappropriate_reason"
                ),
                "binary_ninja_read_protocol": (
                    "governed-session-equal-authenticated-begin-end-v1"
                    if any(
                        claim["kind"] == "binary-ninja-db"
                        and claim["id"] == "Recoil.bndb"
                        and claim["access"] == "read"
                        for claim in claims
                    )
                    else "not-requested"
                ),
                "read_only": work.get("explicit_kind") == "read-only-diagnostic",
                "nonaccepting": True,
                "acceptance_eligible": False,
                "worker_acceptance_allowed": False,
                "candidate_expected_truth": False,
            }
        )
    elif retail_fact_packet:
        packet.update(
            {
                "read_only": True,
                "nonaccepting": True,
                "candidate_expected_truth": False,
                "expected_truth": CONVERGENCE_EXPECTED_TRUTH,
                "retail_fact_scope": deepcopy(work["retail_fact_scope"]),
            }
        )
    elif obligation_contract is not None:
        read_paths = sorted(
            claim["id"]
            for claim in claims
            if claim["access"] == "read"
            and claim["kind"] in {"path", "source-path", "header-path"}
        )
        output_roots = sorted(
            claim["id"]
            for claim in claims
            if claim["access"] == "write" and claim["kind"] == "output-root"
        )
        packet.update(
            {
                "obligation_id": str(work["obligation_id"]),
                "obligation_kind": str(work["obligation_kind"]),
                "read_paths": read_paths,
                "build_root": output_roots[0],
                "nonaccepting": True,
                "acceptance_eligible": False,
                "candidate_expected_truth": False,
                "worker_acceptance_allowed": False,
            }
        )
    prospective_profile = work.get("prospective_profile_handoff")
    if prospective_profile is not None:
        read_paths = sorted(
            claim["id"]
            for claim in claims
            if claim["access"] == "read"
            and claim["kind"] in {"path", "source-path", "header-path"}
        )
        required_read_paths = (
            prospective_profile.get("registered_read_paths")
            if isinstance(prospective_profile, Mapping)
            else None
        )
        command_tokens = worker_command.split()
        if (
            work.get("packet_type") != "call-contract-edit-v1"
            or not _valid_wol_profile_source_handoff_route(prospective_profile)
            or write_paths != sorted(WOL_PROFILE_SOURCE_HANDOFF_WRITE_PATHS)
            or work.get("target_ids") != [WOL_PROFILE_MATRIX_TARGET_ID]
            or not isinstance(required_read_paths, list)
            or not set(required_read_paths).issubset(read_paths)
            or set(write_paths) & set(read_paths)
            or command_tokens[:5]
            != [
                "python",
                "tools/recoil.py",
                "verify",
                "call-contract",
                "--target",
            ]
            or len(command_tokens) < 11
            or command_tokens[5] != WOL_PROFILE_MATRIX_TARGET_ID
            or command_tokens.count("--profile-matrix") != 1
            or command_tokens.count("--progress") != 1
            or command_tokens.count("--build-root") != 1
            or command_tokens.count("--json") != 1
            or "--slice" in command_tokens
            or "--all-authored-bodies" in command_tokens
            or work.get("nonaccepting") is not True
            or work.get("acceptance_eligible") is not False
            or work.get("manifest_mutation_allowed") is not False
            or work.get("candidate_expected_truth") is not False
            or work.get("expected_truth")
            != WOL_PROFILE_SOURCE_HANDOFF_EXPECTED_TRUTH
        ):
            raise ProgressError(
                f"work item {work_id} has a malformed or accepting prospective "
                "WOL profile handoff"
            )
        packet.update(
            {
                "read_paths": read_paths,
                "nonaccepting": True,
                "acceptance_eligible": False,
                "manifest_mutation_allowed": False,
                "candidate_expected_truth": False,
                "expected_truth": WOL_PROFILE_SOURCE_HANDOFF_EXPECTED_TRUTH,
                "prospective_profile_handoff": deepcopy(
                    dict(prospective_profile)
                ),
            }
        )
    if claim_provenance is not None:
        packet["claim_provenance"] = claim_provenance
    if native_git_execution_context is not None:
        packet.update(native_git_execution_context)
    return packet


def _issue_packet_handoff(
    document: ProgressDocument,
    args: argparse.Namespace,
    packet_id: str,
) -> dict[str, Any] | None:
    ledger_path = Path(
        getattr(
            args,
            "issue_ledger",
            DEFAULT_ISSUE_LEDGER,
        )
    )
    try:
        projection = render_workspace_issue_handoff(
            repository_root=REPO_ROOT,
            issue_ledger_path=ledger_path,
            packet_id=packet_id,
        )
    except WorkspacePacketHandoffError as exc:
        raise ProgressError(str(exc)) from exc
    if projection is None:
        return None
    compact = projection["work_item"]
    state = document.pipeline("recoil", resolve_order_target=False)
    result = {
        "binary": "recoil",
        "phase": "workspace-issue",
        "primary_lane": state.get("primary_lane"),
        "cursor": "",
        "physical_block_id": "",
        "work_item_id": packet_id,
        "reservation_id": compact["reservation_id"],
        "handoff_role": compact["handoff_role"],
        "packet_source": "workspace-issues",
        "issue_ledger": display_path(ledger_path, REPO_ROOT),
        "issue_ledger_revision": projection["issue_ledger_revision"],
        "work_item": compact,
        "joined_cursor": document.show(str(compact["issue_id"])),
    }
    return document.scheduler_output(result)


def _command_arg(value: str) -> str:
    return f'"{value.replace(chr(34), chr(34) * 2)}"' if any(ch.isspace() for ch in value) else value


def _progress_command_path(path: Path) -> str:
    """Render a machine-local SQLite path without assigning Git identity."""

    resolved = path.resolve()
    try:
        return resolved.relative_to(REPO_ROOT.resolve()).as_posix()
    except ValueError:
        return resolved.as_posix()


def _bind_native_git_progress_authority(command: str, progress_path: Path) -> str:
    """Bind a tracked-write worker to the authenticated live authority.

    Candidate construction happens before the packet worktree exists, so its
    command carries an explicit placeholder instead of a repository-relative
    ``.agent`` path. Native-Git allocation calls this helper only after
    authenticating ``progress_path`` as the canonical live SQLite authority.
    An already-rendered relative override fails closed rather than being
    interpreted relative to the newly created linked checkout.
    """

    placeholder = "{progress_path}"
    count = command.count(placeholder)
    if count > 1:
        raise ProgressError(
            "tracked-write worker command has more than one progress authority placeholder"
        )
    if count == 1:
        authority = progress_path.resolve(strict=True)
        return command.replace(placeholder, _command_arg(authority.as_posix()))
    tokens = authenticated_validation_command_tokens(
        command,
        require_public_route=False,
    )
    if "--progress" in tokens:
        raise ProgressError(
            "tracked-write worker command binds a live progress authority before "
            "authenticated native-Git allocation"
        )
    return command


def _validate_native_git_progress_authority(
    command_tokens: list[str],
    *,
    progress_path: str | Path | None,
) -> None:
    """Reject linked-worktree-relative live SQLite overrides at handoff."""

    positions = [
        index for index, token in enumerate(command_tokens) if token == "--progress"
    ]
    if not positions:
        return
    if len(positions) != 1 or positions[0] + 1 >= len(command_tokens):
        raise ProgressError(
            "native-Git worker command requires exactly one complete --progress option"
        )
    if progress_path is None:
        raise ProgressError(
            "native-Git worker command progress authority lacks its tracker binding"
        )
    observed = Path(command_tokens[positions[0] + 1])
    if not observed.is_absolute():
        raise ProgressError(
            "native-Git worker command uses a linked-worktree-relative live progress authority"
        )
    try:
        expected = Path(progress_path).resolve(strict=True)
        actual = observed.resolve(strict=True)
    except OSError as exc:
        raise ProgressError(
            f"native-Git worker command progress authority cannot be authenticated: {exc}"
        ) from exc
    if actual != expected:
        raise ProgressError(
            "native-Git worker command does not bind the authenticated canonical "
            "progress authority"
        )


def _is_call_contract_continuation_predecessor(work: Mapping[str, Any]) -> bool:
    return bool(
        work.get("packet_type") in CALL_CONTRACT_CONTINUATION_PREDECESSOR_TYPES
        and work.get("phase") == "authored-call-contract"
        and str(work.get("target_id", ""))
    )


def _generic_tool_blocked_return_provenance(
    work_id: str,
    work: Mapping[str, Any],
    linked_issue_id: str,
) -> dict[str, Any]:
    """Retain a non-call-contract packet's exact tool-blocked linkage."""

    return {
        "schema_version": 1,
        "kind": "reconstruction-packet-tool-blocked-return-provenance",
        "predecessor_work_item_id": work_id,
        "linked_issue_id": linked_issue_id,
        "packet_type": str(work.get("packet_type", "")),
        "phase": str(work.get("phase", "")),
        "lane": str(work.get("lane", "")),
        "byte_lane": str(work.get("byte_lane", "")),
        "cursor": str(work.get("cursor", "")),
        "block_id": str(work.get("block_id", "")),
        "covered_block_ids": deepcopy(list(work.get("covered_block_ids", []))),
        "scope_ids": deepcopy(list(work.get("scope_ids", []))),
        "target_ids": deepcopy(list(work.get("target_ids", []))),
        "noncurrent": True,
        "nonaccepting": True,
        "acceptance_eligible": False,
    }


_PROGRESS_GIT_DOCUMENT: ContextVar[ProgressDocument | None] = ContextVar(
    "progress-git-path-operation-document",
    default=None,
)


def _load_progress_git_inventory(
    document: ProgressDocument | None = None,
) -> GitTrackedPathInventory:
    if document is None:
        document = _PROGRESS_GIT_DOCUMENT.get()
    cache_key = ("git-tracked-path-inventory", str(REPO_ROOT))
    request_cache = getattr(document, "_request_cache", None)
    if isinstance(request_cache, dict):
        cached = request_cache.get(cache_key)
        if isinstance(cached, GitTrackedPathInventory):
            return cached
    try:
        inventory = load_git_tracked_path_inventory(REPO_ROOT)
    except RepositoryPathError as exc:
        raise ProgressError(str(exc)) from exc
    if isinstance(request_cache, dict):
        request_cache[cache_key] = inventory
    return inventory


def _with_progress_git_inventory(function):
    """Scope one immutable executing-worktree inventory to one operation."""

    @wraps(function)
    def wrapped(document: ProgressDocument, *args, **kwargs):
        if _PROGRESS_GIT_DOCUMENT.get() is document:
            return function(document, *args, **kwargs)
        token = _PROGRESS_GIT_DOCUMENT.set(document)
        try:
            return function(document, *args, **kwargs)
        finally:
            _PROGRESS_GIT_DOCUMENT.reset(token)

    return wrapped


def _resolve_tracked_progress_file(
    path_text: str,
    *,
    context: str,
    inventory: GitTrackedPathInventory | None = None,
) -> TrackedRepositoryPath:
    current_inventory = inventory or _load_progress_git_inventory()
    try:
        return resolve_tracked_repository_file(
            path_text,
            repository_root=REPO_ROOT,
            inventory=current_inventory,
            context=context,
        )
    except RepositoryPathError as exc:
        raise ProgressError(str(exc)) from exc


def _unique_work_id(document: ProgressDocument, stem: str) -> str:
    work_id = stem
    suffix = 1
    while work_id in document.collection("work_items"):
        suffix += 1
        work_id = f"{stem}-{suffix}"
    return work_id


def _registration_paths(
    registration: Mapping[str, Any],
    *,
    inventory: GitTrackedPathInventory | None = None,
) -> tuple[set[str], set[str]]:
    source_paths: set[str] = set()
    read_paths: set[str] = set()
    for key in ("source_from", "source_path", "agent_source_path"):
        value = registration.get(key)
        if isinstance(value, str) and value:
            source_paths.add(value)
    manifest = registration.get("manifest_path")
    if isinstance(manifest, str) and manifest:
        read_paths.add(manifest)
    for collection_name in (
        "translation_unit_function_order",
        "linked_function_intervals",
    ):
        values = registration.get(collection_name, [])
        if not isinstance(values, list):
            continue
        for value in values:
            if not isinstance(value, Mapping):
                continue
            source = value.get("source_from")
            if isinstance(source, str) and source:
                source_paths.add(source)
    if inventory is not None:
        source_paths = {
            _resolve_tracked_progress_file(
                path,
                context="registered current source path",
                inventory=inventory,
            ).git_path
            for path in source_paths
        }
        read_paths = {
            _resolve_tracked_progress_file(
                path,
                context="registered current manifest path",
                inventory=inventory,
            ).git_path
            for path in read_paths
        }
    return source_paths, read_paths


def _call_contract_slice_write_paths(
    document: ProgressDocument,
    slice_row: Mapping[str, Any],
) -> list[str]:
    return call_contract_source_write_paths(document, slice_row)


def _byte_scope(
    document: ProgressDocument,
    *,
    cursor: str,
    preflight: Mapping[str, Any],
) -> dict[str, Any]:
    tracked_inventory = _load_progress_git_inventory(document)
    point = address_value(cursor)
    group = next(
        (
            row
            for row in document._physical_groups("recoil", gating_only=False)
            if row["range"][0] == point
        ),
        None,
    )
    if group is None:
        raise ProgressError(f"no physical function group exists at {cursor}")
    block_id = str(group.get("physical_block_id", ""))
    block = document.collection("physical_blocks").get(block_id)
    if not isinstance(block, Mapping):
        raise ProgressError(f"byte cursor {cursor} has no physical block")
    source_paths: set[str] = set()
    read_paths: set[str] = set()
    for key in ("agent_source_path", "original_source_path", "provisional_original_path", "source_path"):
        value = block.get(key)
        if isinstance(value, str) and value:
            source_paths.add(value)
            break
    for item in block.get("source_shape_inputs", []):
        if isinstance(item, Mapping):
            value = item.get("path")
            if isinstance(value, str) and value:
                source_paths.add(value)
    scope_ids = {str(value) for value in group.get("scope_ids", [])}
    target_ids: set[str] = set(str(value) for value in preflight.get("target_ids", []))
    for scope_id in scope_ids:
        symbol = document.collection("symbols").get(scope_id)
        if isinstance(symbol, Mapping):
            for value in symbol.get("verification_target_ids", []):
                target = document.collection("verification_targets").get(str(value))
                if isinstance(target, Mapping) and target.get("kind") == "vc5":
                    target_ids.add(str(value))
    for target_id in sorted(target_ids):
        target = document.collection("verification_targets").get(target_id)
        if not isinstance(target, Mapping) or target.get("kind") != "vc5":
            continue
        registration = target.get("registration", {})
        if isinstance(registration, Mapping):
            target_sources, target_reads = _registration_paths(
                registration,
                inventory=tracked_inventory,
            )
            source_paths.update(target_sources)
            read_paths.update(target_reads)
    source_paths.update(
        str(value)
        for value in preflight.get("source_paths", [])
        if isinstance(value, str) and value
    )
    owner_ids: set[str] = set()
    for owner_id, owner in document.collection("owners").items():
        if not isinstance(owner, Mapping):
            continue
        primary_ids = {str(value) for value in owner.get("primary_function_ids", [])}
        primary_entries = owner.get("primary_entries", {})
        if isinstance(primary_entries, Mapping):
            primary_ids.update(str(value) for value in primary_entries)
        if not scope_ids.intersection(primary_ids):
            continue
        owner_ids.add(str(owner_id))
        for key in ("source_path", "agent_source_path", "header_path"):
            value = owner.get(key)
            if isinstance(value, str) and value:
                source_paths.add(value)
        for key in ("source_paths", "header_paths", "implementation_paths"):
            values = owner.get(key, [])
            if isinstance(values, list):
                source_paths.update(
                    str(value)
                    for value in values
                    if isinstance(value, str) and value
                )
    source_paths = {
        _resolve_tracked_progress_file(
            value,
            context="current byte packet source/header path",
            inventory=tracked_inventory,
        ).git_path
        for value in source_paths
        if Path(value).suffix.casefold() in {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".inl"}
    }
    read_paths = {
        _resolve_tracked_progress_file(
            value,
            context="current byte packet read dependency",
            inventory=tracked_inventory,
        ).git_path
        for value in read_paths
    }
    if not source_paths:
        raise ProgressError(f"byte cursor {cursor} has no writable source/header closure")
    return {
        "group": group,
        "block_id": block_id,
        "scope_ids": sorted(scope_ids),
        "target_ids": sorted(target_ids),
        "owner_ids": sorted(owner_ids),
        "source_paths": sorted(source_paths),
        "read_paths": sorted(read_paths),
    }


def _focused_live_byte_bindings(
    document: ProgressDocument,
    *,
    scope_ids: Iterable[str],
    target_addresses: Iterable[str] = (),
    existing: Mapping[str, list[Any]] | None = None,
) -> dict[str, list[Any]]:
    from _recoil.commands import live_byte_verify
    from _recoil.commands.relocation_expectations import (
        RelocationExpectationError,
        build_object_binding_snapshot,
        normalize_relocation_target_binding,
        relocation_target_row_context,
    )
    from _recoil.commands.vc5_verify import load_manifest
    from _recoil.lib.verification_targets import vc5_target_registration

    result = {key: list(values) for key, values in (existing or {}).items()}
    loaded_targets: dict[Path, Any] = {}
    selected_ids = {str(value) for value in scope_ids}
    selected_addresses = {normalize_address(value) for value in target_addresses}
    targets = document.collection("verification_targets")
    tracked_inventory = _load_progress_git_inventory(document)

    source_closure: dict[tuple[str, str], dict[str, Any]] = {}
    if selected_addresses:
        selected_points = {address_value(value) for value in selected_addresses}
        for target_symbol_id, target_symbol in document.collection("symbols").items():
            if not isinstance(target_symbol, Mapping) or target_symbol.get("binary") != "recoil":
                continue
            try:
                target_start = address_value(str(target_symbol.get("address", "")))
                target_end = address_value(str(target_symbol.get("end_exclusive", "")))
            except ValueError:
                continue
            if target_end <= target_start or not any(
                target_start <= point < target_end for point in selected_points
            ):
                continue
            raw_bindings = target_symbol.get("relocation_target_binding")
            if isinstance(raw_bindings, Mapping):
                reviewed_bindings: Iterable[Any] = (raw_bindings,)
            elif isinstance(raw_bindings, list):
                reviewed_bindings = raw_bindings
            else:
                reviewed_bindings = ()
            for raw_binding in reviewed_bindings:
                if not isinstance(raw_binding, Mapping) or raw_binding.get("reviewed") is not True:
                    continue
                try:
                    normalized = normalize_relocation_target_binding(raw_binding)
                    current_target = relocation_target_row_context(
                        symbol_id=str(target_symbol_id),
                        row=target_symbol,
                        object_symbol=str(normalized["object_symbol"]),
                    )
                except (RelocationExpectationError, TypeError, ValueError) as exc:
                    raise ProgressError(
                        f"focused relocation target {target_symbol_id!r} has an invalid "
                        f"reviewed binding: {exc}"
                    ) from exc
                context = normalized["binding_context"]
                if context["target"] != current_target:
                    raise ProgressError(
                        f"focused relocation target {target_symbol_id!r} has a stale target snapshot"
                    )
                stored_source = context["source_binding"]
                key = (
                    str(stored_source["symbol_id"]),
                    str(stored_source["object_symbol"]),
                )
                prior = source_closure.get(key)
                if prior is not None and prior != stored_source:
                    raise ProgressError(
                        f"focused relocation target bindings conflict for source {key[0]!r} "
                        f"and object symbol {key[1]!r}"
                    )
                source_closure[key] = dict(stored_source)
        selected_ids.update(symbol_id for symbol_id, _object_symbol in source_closure)

    closure_source_ids = {symbol_id for symbol_id, _object_symbol in source_closure}
    synchronized_closure_targets: dict[str, set[str]] = {
        symbol_id: set() for symbol_id in closure_source_ids
    }
    seen_closure_source_rows: set[str] = set()

    def target_functions(target: Any) -> list[tuple[Any, str]]:
        rows: list[tuple[Any, str]] = [
            (function, str(target.source_from)) for function in target.functions
        ]
        rows.extend(
            (function, str(entry.source_from))
            for entry in target.translation_unit_function_order
            for function in entry.functions
        )
        rows.extend(
            (function, str(target.source_from))
            for interval in target.linked_function_intervals
            for function in interval.functions
        )
        deduplicated: list[tuple[Any, str]] = []
        seen: set[tuple[str, str, str, str]] = set()
        for function, source_from in rows:
            key = (
                normalize_address(function.address),
                str(function.symbol),
                str(function.logical_identity_key or ""),
                source_from,
            )
            if key not in seen:
                seen.add(key)
                deduplicated.append((function, source_from))
        return deduplicated

    for symbol_id, symbol in document.collection("symbols").items():
        if not isinstance(symbol, Mapping):
            continue
        address = normalize_address(str(symbol.get("address", "")))
        if str(symbol_id) not in selected_ids and address not in selected_addresses:
            continue
        if str(symbol_id) in closure_source_ids:
            seen_closure_source_rows.add(str(symbol_id))
        is_data = str(symbol.get("kind", "")).endswith("data") or symbol.get("kind") in {
            "data",
            "data-symbol",
        }
        raw_target_ids = symbol.get("verification_target_ids", [])
        if str(symbol_id) in closure_source_ids and (
            not isinstance(raw_target_ids, list)
            or any(not isinstance(item, str) or not item for item in raw_target_ids)
            or len(raw_target_ids) != len(set(raw_target_ids))
        ):
            raise ProgressError(
                f"focused binding source {symbol_id!r} has an invalid or ambiguous "
                "verification_target_ids registration"
            )
        for target_id in raw_target_ids:
            target_row = targets.get(str(target_id))
            if not isinstance(target_row, Mapping):
                if str(symbol_id) in closure_source_ids:
                    raise ProgressError(
                        f"focused binding source {symbol_id!r} references unregistered "
                        f"verification target {target_id!r}"
                    )
                continue
            if target_row.get("kind") != "vc5":
                continue
            registration = target_row.get("registration", {})
            if not isinstance(registration, Mapping):
                if str(symbol_id) in closure_source_ids:
                    raise ProgressError(
                        f"focused binding source {symbol_id!r} has an invalid VC5 "
                        f"registration {target_id!r}"
                    )
                continue
            manifest_text = registration.get("manifest_path")
            if isinstance(manifest_text, str) and manifest_text:
                tracked_manifest = _resolve_tracked_progress_file(
                    manifest_text,
                    context=f"focused binding target {target_id!r} manifest",
                    inventory=tracked_inventory,
                )
            else:
                if str(symbol_id) in closure_source_ids:
                    raise ProgressError(
                        f"focused binding source {symbol_id!r} VC5 registration "
                        f"{target_id!r} has no manifest_path"
                    )
                name = str(registration.get("name") or target_row.get("name") or "")
                try:
                    tracked_manifest = _resolve_tracked_progress_file(
                        f"tools/vc5_verify_targets/{name}.json",
                        context=f"focused binding target {target_id!r} inferred manifest",
                        inventory=tracked_inventory,
                    )
                except ProgressError:
                    if str(symbol_id) in closure_source_ids:
                        raise
                    continue
            if not tracked_manifest.git_path.startswith("tools/vc5_verify_targets/"):
                raise ProgressError(
                    f"focused binding source {symbol_id!r} VC5 manifest is outside "
                    "tools/vc5_verify_targets"
                )
            manifest_path = tracked_manifest.physical_path
            if str(symbol_id) in closure_source_ids:
                try:
                    current_target_id, current_registration = vc5_target_registration(
                        manifest_path
                    )
                except (OSError, ValueError) as exc:
                    raise ProgressError(
                        f"focused binding source {symbol_id!r} VC5 registration "
                        f"{target_id!r} cannot be synchronized: {exc}"
                    ) from exc
                current_registration_context = current_registration.get("registration")
                stable_identity_matches = (
                    current_target_id == str(target_id)
                    and current_registration.get("binary") == target_row.get("binary") == "recoil"
                    and current_registration.get("kind") == target_row.get("kind") == "vc5"
                    and current_registration.get("name") == target_row.get("name")
                    and isinstance(current_registration_context, Mapping)
                    and current_registration_context.get("binary") == registration.get("binary")
                    and current_registration_context.get("name") == registration.get("name")
                    and current_registration_context.get("manifest_path")
                    == registration.get("manifest_path")
                    and address
                    in {
                        normalize_address(item)
                        for item in current_registration.get("registered_addresses", [])
                        if isinstance(item, str)
                    }
                )
                if not stable_identity_matches:
                    raise ProgressError(
                        f"focused binding source {symbol_id!r} VC5 registration "
                        f"{target_id!r} is stale or conflicting"
                    )
                synchronized_closure_targets[str(symbol_id)].add(str(target_id))
            target = loaded_targets.get(manifest_path)
            if target is None:
                target = load_manifest(manifest_path, enforce_source_policy=True)
                loaded_targets[manifest_path] = target
            rows = (
                [(item, str(target.source_from)) for item in getattr(target, "data_symbols", ())]
                if is_data
                else target_functions(target)
            )
            for function, source_from in rows:
                if normalize_address(function.address) != address:
                    continue
                binding = live_byte_verify.TargetBinding(
                    target=target,
                    function=function,
                    target_id=str(target_id),
                    scope_id=str(symbol_id),
                    source_from=source_from,
                )
                current = result.setdefault(str(symbol_id), [])
                key = (
                    str(binding.target_id),
                    str(binding.function.symbol),
                    str(getattr(binding.function, "logical_identity_key", "") or ""),
                    str(binding.source_from),
                )
                if all(
                    key
                    != (
                        str(item.target_id),
                        str(item.function.symbol),
                        str(getattr(item.function, "logical_identity_key", "") or ""),
                        str(item.source_from),
                    )
                    for item in current
                ):
                    current.append(binding)

    missing_source_rows = sorted(closure_source_ids - seen_closure_source_rows)
    if missing_source_rows:
        raise ProgressError(
            "focused relocation target binding source rows are absent: "
            + ", ".join(missing_source_rows)
        )
    for source_symbol_id in sorted(closure_source_ids):
        if not synchronized_closure_targets[source_symbol_id]:
            raise ProgressError(
                f"focused binding source {source_symbol_id!r} has no synchronized "
                "tracker-attached VC5 registration"
            )
        expected_sources = {
            object_symbol: stored
            for (symbol_id, object_symbol), stored in source_closure.items()
            if symbol_id == source_symbol_id
        }
        current = result.get(source_symbol_id, [])
        current_symbols = {
            str(item.function.symbol)
            for item in current
            if isinstance(getattr(item.function, "symbol", None), str)
            and str(item.function.symbol)
        }
        if current_symbols != set(expected_sources):
            raise ProgressError(
                f"focused binding source {source_symbol_id!r} has conflicting or wrong-symbol "
                f"VC5 identities: expected {sorted(expected_sources)}, found {sorted(current_symbols)}"
            )
        for object_symbol, stored_source in expected_sources.items():
            matching = [
                item for item in current if str(item.function.symbol) == object_symbol
            ]
            if len(matching) != 1:
                raise ProgressError(
                    f"focused binding source {source_symbol_id!r} object symbol "
                    f"{object_symbol!r} resolves to {len(matching)} synchronized VC5 bindings; "
                    "expected exactly one"
                )
            try:
                current_source = build_object_binding_snapshot(
                    document,
                    result,
                    symbol_id=source_symbol_id,
                    object_symbol=object_symbol,
                )
            except RelocationExpectationError as exc:
                raise ProgressError(
                    f"focused binding source {source_symbol_id!r} snapshot is invalid: {exc}"
                ) from exc
            if current_source != stored_source:
                raise ProgressError(
                    f"focused binding source {source_symbol_id!r} stored registration snapshot "
                    "is stale or conflicting"
                )
    return result


def _byte_lane_preflight(
    document: ProgressDocument,
    *,
    lane: str,
    cursor: str,
    bindings: Mapping[str, list[Any]] | None = None,
    authored_order_prefix_end: str | None = None,
) -> dict[str, Any]:
    from _recoil.commands import live_byte_verify

    try:
        rows = live_byte_verify._rows(
            document,
            lane,
            cursor,
            authored_order_prefix_end=authored_order_prefix_end,
        )
        if len(rows) != 1:
            raise ProgressError(f"{cursor}: expected one physical byte group, found {len(rows)}")
        row = rows[0]
        bindings = bindings or _focused_live_byte_bindings(
            document, scope_ids=row.get("scope_ids", [])
        )
        authored_rows = [
            item
            for item in row.get("physical_rows", [])
            if not live_byte_verify._is_provider_or_compiler_row(item)
        ]
        selected: list[Any] = []
        if authored_rows:
            authored_group = dict(row)
            authored_group["physical_rows"] = authored_rows
            authored_group["scope_ids"] = [str(item["symbol_id"]) for item in authored_rows]
            selected = live_byte_verify._select_bindings(bindings, authored_group)
        if lane != "linked" and not selected:
            raise ProgressError(f"{cursor}: byte group has no exact authored target binding")
        for provider_row in (
            item
            for item in row.get("physical_rows", [])
            if live_byte_verify._is_provider_or_compiler_row(item)
        ):
            binding = provider_row.get("linked_provider_binding")
            if lane == "linked" and not isinstance(binding, Mapping):
                raise ProgressError(
                    f"{cursor}: provider/compiler row lacks an exact linked provider binding"
                )
        target_ids = sorted({str(item.target_id) for item in selected})
        source_paths = sorted(
            {
                str(item.source_from or getattr(item.target, "source_from", ""))
                for item in selected
                if str(item.source_from or getattr(item.target, "source_from", ""))
            }
        )
        report: Mapping[str, Any] | None = None
        if lane == "authored":
            from _recoil.commands.relocation_expectations import derive_relocation_expectations

            reports: list[dict[str, Any]] = []
            seen_symbols: set[str] = set()
            for binding in selected:
                object_symbol = str(binding.function.symbol)
                if object_symbol in seen_symbols:
                    continue
                seen_symbols.add(object_symbol)
                reports.append(
                    derive_relocation_expectations(
                        document=document,
                        row=row,
                        object_symbol=object_symbol,
                        bindings=bindings,
                        reference=MACHINE_RETAIL_REFERENCE,
                    )
                )
            missing_addresses = {
                str(unresolved.get("retail_target"))
                for item in reports
                for unresolved in item.get("unresolved", [])
                if isinstance(unresolved, Mapping)
                and unresolved.get("kind") == "missing-target-identity"
                and isinstance(unresolved.get("retail_target"), str)
            }
            if missing_addresses:
                expanded = _focused_live_byte_bindings(
                    document,
                    scope_ids=row.get("scope_ids", []),
                    target_addresses=missing_addresses,
                    existing=bindings,
                )
                if expanded != bindings:
                    bindings = expanded
                    reports = [
                        derive_relocation_expectations(
                            document=document,
                            row=row,
                            object_symbol=str(binding.function.symbol),
                            bindings=bindings,
                            reference=MACHINE_RETAIL_REFERENCE,
                        )
                        for binding in selected
                        if str(binding.function.symbol)
                    ]
            report = {
                "report_version": 1,
                "kind": "relocation-expectations-audit",
                "validation_mode": "live-retail-derived",
                "candidate_independent": True,
                "tracker_revision": document.revision,
                "address": cursor,
                "passed": bool(reports) and all(item["passed"] for item in reports),
                "reports": reports,
            }
            if report.get("passed") is not True:
                first_unresolved = next(
                    (
                        unresolved
                        for item in report.get("reports", [])
                        if isinstance(item, Mapping)
                        for unresolved in item.get("unresolved", [])
                        if isinstance(unresolved, Mapping)
                    ),
                    {},
                )
                return {
                    "passed": False,
                    "reason_code": "relocation-expectations-unresolved",
                    "reason": str(
                        first_unresolved.get("message")
                        or "retail relocation expectations are unresolved"
                    ),
                    "target_ids": target_ids,
                    "source_paths": source_paths,
                    "report": report,
                }
        return {
            "passed": True,
            "reason_code": "live-byte-preflight-ready",
            "reason": "exact target/provider bindings and retail expected facts are available",
            "target_ids": target_ids,
            "source_paths": source_paths,
            "binding_count": len(selected),
            "report": report,
        }
    except (OSError, RuntimeError, ValueError) as exc:
        return {
            "passed": False,
            "reason_code": "byte-preflight-blocked",
            "reason": str(exc),
            "target_ids": [],
            "source_paths": [],
        }


def _order_claim_candidate(
    document: ProgressDocument,
    pipeline: Mapping[str, Any],
) -> tuple[str, dict[str, Any]]:
    resolution = pipeline.get("order_target_resolution", {})
    if not isinstance(resolution, Mapping) or resolution.get("status") != "ready":
        raise ProgressError(
            str(
                resolution.get("reason")
                if isinstance(resolution, Mapping)
                else "current primary order target is blocked"
            )
        )
    target_id = str(resolution["target_id"])
    contract = _current_order_contract(
        document,
        target_id,
        override_selector=(
            str(resolution.get("override_selector") or "") or None
            if str(resolution.get("phase") or pipeline.get("phase") or "")
            == "authored-function-order"
            else None
        ),
        object_selector=(
            str(resolution.get("object_target_id") or "") or None
            if str(resolution.get("phase") or pipeline.get("phase") or "")
            == "full-function-order"
            else None
        ),
    )
    worker_target = contract.get("worker_target", contract["target"])
    worker_target_id = str(contract.get("worker_target_id") or target_id)
    target_name = str(
        worker_target.get("name")
        or worker_target.get("registration", {}).get("name")
        or ""
    )
    if not target_name:
        raise ProgressError(f"worker object target {worker_target_id!r} has no verifier name")
    cursor = str(pipeline["cursor"])
    label = target_id.rsplit(":", 1)[-1]
    worker_root = document._fresh_root("worker-order", label, document.revision)
    work_id = _unique_work_id(
        document,
        f"recoil:work:live-order-{cursor.replace('0x', '')}-r{document.revision + 1}",
    )
    worker_contract = dict(contract)
    worker_contract.update({"target_id": worker_target_id, "target": worker_target})
    write_paths = _order_target_write_paths(document, worker_contract)
    linked_feedback_option = ""
    if contract["phase"] == "full-function-order":
        linked_target = contract.get("linked_target")
        linked_target_name = (
            str(
                linked_target.get("name")
                or linked_target.get("registration", {}).get("name")
                or ""
            )
            if isinstance(linked_target, Mapping)
            else ""
        )
        if not linked_target_name:
            raise ProgressError("full-order worker has no exact linked feedback target name")
        linked_feedback_option = f" --linked-target {_command_arg(linked_target_name)}"
    worker_command = (
        f"python tools/recoil.py verify vc5-order {_command_arg(target_name)}"
        f"{linked_feedback_option} "
        f"--build-root {worker_root}"
    )
    registration = worker_target.get("registration", {})
    _sources, read_paths = (
        _registration_paths(
            registration,
            inventory=_load_progress_git_inventory(document),
        )
        if isinstance(registration, Mapping)
        else (set(), set())
    )
    claims = [
        *({"kind": "path", "id": path, "access": "write"} for path in write_paths),
        *(
            {"kind": "block", "id": covered_id, "access": "write"}
            for covered_id in contract["covered_block_ids"]
        ),
        *({"kind": "path", "id": path, "access": "read"} for path in sorted(read_paths)),
        {"kind": "lane", "id": "current-order-lane", "access": "write"},
        *(
            {"kind": "verification-target", "id": current_id, "access": "read"}
            for current_id in sorted(
                {
                    str(contract.get("linked_target_id") or ""),
                    str(contract.get("object_target_id") or ""),
                }
                - {""}
            )
        ),
        {"kind": "output-root", "id": worker_root, "access": "write"},
        {"kind": "tu-build", "id": worker_target_id, "access": "write"},
        {"kind": "reference", "id": "support/Recoil.exe", "access": "read"},
        {"kind": "tracker", "id": "recoil", "access": "read"},
    ]
    work = bind_work_packet_contract(
        document,
        {
            "binary": "recoil",
            "handoff_role": "recoil_source_worker",
            "packet_type": "order-edit-v1",
            "state": "ready",
            "phase": contract["phase"],
            "lane": "primary",
            "cursor": cursor,
            "block_id": contract["cursor_block_id"],
            "covered_block_ids": contract["covered_block_ids"],
            "target_id": target_id,
            "linked_target_id": str(contract.get("linked_target_id") or ""),
            "object_target_id": str(contract.get("object_target_id") or ""),
            "worker_target_id": worker_target_id,
            "source_policy_bootstrap": (
                deepcopy(registration.get("source_policy_bootstrap"))
                if isinstance(registration, Mapping)
                and isinstance(registration.get("source_policy_bootstrap"), Mapping)
                else None
            ),
            "allowed_paths": write_paths,
            "validation_commands": [worker_command],
            "objective": (
                "Make the authored object projection and exact full linked selected "
                "population/seams pass."
                if contract["phase"] == "full-function-order"
                else "Make the registered natural VC5 order comparison pass."
            ),
            "stop_condition": (
                "PASS both object and linked feedback, or return the first linked divergence "
                "or a concrete out-of-scope source/block/header contradiction."
                if contract["phase"] == "full-function-order"
                else "PASS or a concrete out-of-scope source/block/header contradiction."
            ),
            "required_return_fields": [
                "packet_id",
                "outcome",
                "changed_paths",
                "validation_command",
                "passed",
                "first_divergence",
                "scope_contradiction",
            ],
            "resource_claims": claims,
        },
    )
    return work_id, work


def _byte_claim_candidate(
    document: ProgressDocument,
    *,
    packet_lane: str,
    verifier_lane: str,
    cursor: str,
    phase: str,
    progress_path: Path,
    preflight: Mapping[str, Any],
) -> tuple[str, dict[str, Any]]:
    scope = _byte_scope(document, cursor=cursor, preflight=preflight)
    worker_root = document._fresh_root(
        f"worker-{verifier_lane}-byte", cursor.replace("0x", ""), document.revision
    )
    verify_name = BYTE_VERIFY_COMMANDS[verifier_lane]
    worker_command = (
        f"python tools/recoil.py verify {verify_name} --at {cursor} "
        f"--build-root {worker_root} --progress {{progress_path}} --json"
    )
    stem = (
        f"recoil:work:live-{packet_lane}-{verifier_lane}-byte-"
        f"{cursor.replace('0x', '')}-r{document.revision + 1}"
    )
    work_id = _unique_work_id(document, stem)
    claims: list[dict[str, str]] = [
        *(
            {"kind": "path", "id": path, "access": "write"}
            for path in scope["source_paths"]
        ),
        *(
            {"kind": "path", "id": path, "access": "read"}
            for path in scope["read_paths"]
        ),
        *(
            {"kind": "owner", "id": owner_id, "access": "write"}
            for owner_id in scope["owner_ids"]
        ),
        {"kind": "block", "id": scope["block_id"], "access": "write"},
        {
            "kind": "lane",
            "id": (
                "current-byte-lane"
                if packet_lane == "primary"
                else f"parallel-{packet_lane}-byte"
            ),
            "access": "write",
        },
        {"kind": "output-root", "id": worker_root, "access": "write"},
        *(
            [{"kind": "whole-project-build", "id": "recoil", "access": "write"}]
            if verifier_lane in {"authored", "linked"}
            else [
                {"kind": "tu-build", "id": target_id, "access": "write"}
                for target_id in scope["target_ids"]
            ]
        ),
        {"kind": "path", "id": "tools/_recoil/config/vc5_final_build.json", "access": "read"},
        {"kind": "reference", "id": "support/Recoil.exe", "access": "read"},
        {"kind": "tracker", "id": "recoil", "access": "read"},
        *(
            {"kind": "verification-target", "id": target_id, "access": "read"}
            for target_id in scope["target_ids"]
        ),
    ]
    work = bind_work_packet_contract(
        document,
        {
            "binary": "recoil",
            "handoff_role": "recoil_source_worker",
            "packet_type": "byte-edit-v1",
            "state": "ready",
            "phase": phase,
            "lane": packet_lane,
            "byte_lane": verifier_lane,
            "cursor": cursor,
            "block_id": scope["block_id"],
            "covered_block_ids": [scope["block_id"]],
            "scope_ids": scope["scope_ids"],
            "target_ids": scope["target_ids"],
            "allowed_paths": scope["source_paths"],
            "validation_commands": [worker_command],
            "objective": (
                f"Make the exact live {verifier_lane} byte comparison pass for the physical "
                f"address group at {cursor}."
            ),
            "stop_condition": (
                f"The {cursor} group appears in matched_groups, or the verifier reports a "
                "concrete out-of-scope owner/source/provider contradiction."
            ),
            "required_return_fields": [
                "packet_id",
                "outcome",
                "changed_paths",
                "validation_command",
                "passed",
                "matched_groups",
                "first_divergence",
                "scope_contradiction",
            ],
            "preflight": {
                key: deepcopy(preflight[key])
                for key in ("reason_code", "reason", "target_ids", "source_paths", "binding_count")
                if key in preflight
            },
            "resource_claims": claims,
        },
    )
    return work_id, work


def _call_contract_claim_candidate(
    document: ProgressDocument,
    pipeline: Mapping[str, Any],
    *,
    progress_path: Path,
) -> tuple[str, dict[str, Any]]:
    slice_id = str(pipeline.get("authored_call_contract_slice_id", ""))
    matches = [
        row
        for row in document.authored_call_contract_slices()
        if str(row.get("id", "")) == slice_id
    ]
    if len(matches) != 1:
        raise ProgressError(
            f"current authored call-contract slice {slice_id!r} must resolve exactly once"
        )
    slice_row = matches[0]
    cursor = str(slice_row["start"])
    worker_root = document._fresh_root(
        "worker-call-contract", cursor.replace("0x", ""), document.revision
    )
    worker_command = (
        "python tools/recoil.py verify call-contract "
        f"--slice {_command_arg(slice_id)} "
        "--progress {progress_path} "
        f"--build-root {_command_arg(worker_root)} --json"
    )
    work_id = _unique_work_id(
        document,
        f"recoil:work:live-call-contract-{cursor.replace('0x', '')}-r{document.revision + 1}",
    )
    source_closure = call_contract_source_closure(document, slice_row)
    source_paths = list(source_closure.source_edit_paths)
    definition_source_paths = list(
        source_closure.definition_source_paths
    )
    dependency_paths = list(source_closure.dependency_paths)
    claims: list[dict[str, str]] = [
        *(
            {"kind": "path", "id": path, "access": "write"}
            for path in source_paths
        ),
        *(
            {"kind": "path", "id": path, "access": "read"}
            for path in dependency_paths
            if path not in source_paths
        ),
        *(
            {"kind": "block", "id": block_id, "access": "write"}
            for block_id in slice_row["physical_block_ids"]
        ),
        *(
            {"kind": "verification-target", "id": target_id, "access": "read"}
            for target_id in slice_row["target_ids"]
        ),
        {"kind": "lane", "id": "current-call-contract-lane", "access": "write"},
        {"kind": "output-root", "id": worker_root, "access": "write"},
        {"kind": "whole-project-build", "id": "recoil", "access": "read"},
        {"kind": "reference", "id": "support/Recoil.exe", "access": "read"},
        {"kind": "binary-ninja-db", "id": "Recoil.bndb", "access": "read"},
        {"kind": "tracker", "id": "recoil", "access": "read"},
    ]
    work = bind_work_packet_contract(
        document,
        {
            "binary": "recoil",
            "handoff_role": "recoil_source_worker",
            "packet_type": "call-contract-edit-v1",
            "state": "ready",
            "phase": "authored-call-contract",
            "lane": "primary",
            "cursor": cursor,
            "block_id": str(slice_row["physical_block_ids"][0]),
            "covered_block_ids": list(slice_row["physical_block_ids"]),
            "scope_ids": list(slice_row["symbol_ids"]),
            "target_ids": list(slice_row["target_ids"]),
            "slice_id": slice_id,
            "allowed_paths": source_paths,
            "source_edit_paths": source_paths,
            "definition_source_paths": definition_source_paths,
            "dependency_paths": dependency_paths,
            "closure_v2": {
                "contract_version": 2,
                "definition_resolution": deepcopy(
                    source_closure.definition_resolution
                ),
            },
            "validation_commands": [worker_command],
            "objective": (
                "Make the exact retail-ordered static invocation contract pass for "
                f"all {slice_row['body_count']} authored bodies in {slice_id}."
            ),
            "stop_condition": (
                "PASS the complete slice, or return its first exact call target/form/"
                "dispatch/storage/slot/cleanup divergence or a concrete scope contradiction."
            ),
            "required_return_fields": [
                "packet_id",
                "outcome",
                "changed_paths",
                "validation_command",
                "passed",
                "first_divergence",
                "scope_contradiction",
            ],
            "resource_claims": claims,
        },
    )
    return work_id, work


def _call_contract_convergence_repair_candidates(
    document: ProgressDocument,
    *,
    progress_path: Path,
    issue_ledger: Path,
) -> tuple[list[tuple[str, dict[str, Any]]], list[dict[str, Any]]]:
    if not isinstance(getattr(document, "data", None), Mapping):
        return [], []
    state = convergence_generation_state(document, copy_generation=False)
    generation = state.get("generation")
    if (
        state.get("current") is not True
        or not isinstance(generation, Mapping)
        or generation.get("status") != "failed-targets"
    ):
        return [], []
    ordinary_descriptors = generation.get("repair_descriptors", [])
    dependent_descriptors = generation.get(
        "dependent_owner_repair_descriptors", []
    )
    dependent_blocker_descriptors = generation.get(
        "dependent_owner_blocker_descriptors", []
    )
    retail_blocker_descriptors = generation.get(
        "retail_blocker_descriptors", []
    )
    if not isinstance(ordinary_descriptors, list) or not isinstance(
        dependent_descriptors, list
    ) or not isinstance(dependent_blocker_descriptors, list) or not isinstance(
        retail_blocker_descriptors, list
    ):
        raise ProgressError(
            "call-contract convergence generation repair descriptors must be lists"
        )
    retail_target_ids = [
        str(descriptor.get("target_id", ""))
        for descriptor in retail_blocker_descriptors
        if isinstance(descriptor, Mapping)
    ]
    if (
        len(retail_target_ids) != len(retail_blocker_descriptors)
        or any(not target_id for target_id in retail_target_ids)
        or len(retail_target_ids) != len(set(retail_target_ids))
    ):
        raise ProgressError(
            "call-contract convergence retail blocker target partition is malformed"
        )
    dependent_blockers_by_target: dict[str, list[Mapping[str, Any]]] = {}
    for blocker_descriptor in dependent_blocker_descriptors:
        if isinstance(blocker_descriptor, Mapping):
            dependent_blockers_by_target.setdefault(
                str(blocker_descriptor.get("target_id", "")), []
            ).append(blocker_descriptor)
    target_results_by_id = {
        str(row.get("target_id", "")): row
        for row in generation.get("target_results", [])
        if isinstance(row, Mapping)
    }

    def dependent_route_order(
        descriptor: Mapping[str, Any],
    ) -> tuple[int, int, str, str]:
        addresses = descriptor.get("addresses", [])
        address_ordinals = {
            str(address): ordinal
            for ordinal, address in enumerate(addresses)
        } if isinstance(addresses, list) else {}
        positions: list[tuple[int, int]] = []
        for route in descriptor.get("routed_identities", []):
            if not isinstance(route, Mapping):
                continue
            caller_address = str(route.get("caller_address", ""))
            call_site = route.get("candidate_call_site_address")
            if caller_address not in address_ordinals or not isinstance(
                call_site, str
            ):
                continue
            try:
                positions.append(
                    (
                        address_ordinals[caller_address],
                        address_value(call_site),
                    )
                )
            except (TypeError, ValueError):
                continue
        owner_pair_key = descriptor.get("owner_pair_key")
        declaration_path = (
            str(owner_pair_key.get("declaration_path", ""))
            if isinstance(owner_pair_key, Mapping)
            else ""
        )
        definition_path = (
            str(owner_pair_key.get("definition_path", ""))
            if isinstance(owner_pair_key, Mapping)
            else ""
        )
        first_position = min(positions) if positions else (sys.maxsize, sys.maxsize)
        return (
            first_position[0],
            first_position[1],
            declaration_path.casefold(),
            definition_path.casefold(),
        )

    dependent_descriptors = sorted(
        dependent_descriptors,
        key=lambda descriptor: (
            str(descriptor.get("target_id", ""))
            if isinstance(descriptor, Mapping)
            else "",
            dependent_route_order(descriptor)
            if isinstance(descriptor, Mapping)
            else (sys.maxsize, sys.maxsize, "", ""),
        ),
    )
    descriptor_rows = [
        (descriptor, "call-contract-converge-edit-v1", ordinal)
        for ordinal, descriptor in enumerate(ordinary_descriptors)
    ] + [
        (
            descriptor,
            "call-contract-dependent-owner-edit-v1",
            len(ordinary_descriptors) + ordinal,
        )
        for ordinal, descriptor in enumerate(dependent_descriptors)
    ]
    blocked: list[dict[str, Any]] = []
    retail_by_target = {
        str(descriptor["target_id"]): descriptor
        for descriptor in retail_blocker_descriptors
    }

    def retail_precedes_repair(
        repair: Mapping[str, Any],
        retail: Mapping[str, Any],
    ) -> dict[str, Any]:
        repair_divergence = repair.get("first_divergence")
        if not isinstance(repair_divergence, Mapping):
            repair_rows = repair.get("caller_divergences")
            repair_divergence = (
                repair_rows[0]
                if isinstance(repair_rows, list)
                and repair_rows
                and isinstance(repair_rows[0], Mapping)
                else None
            )
        retail_rows = retail.get("divergences")
        retail_divergence = (
            retail_rows[0]
            if isinstance(retail_rows, list)
            and retail_rows
            and isinstance(retail_rows[0], Mapping)
            else None
        )
        try:
            repair_address = address_value(
                str(repair_divergence.get("address", ""))
            )
            retail_address = address_value(
                str(retail_divergence.get("address", ""))
            )
        except (AttributeError, TypeError, ValueError):
            return {
                "state": "blocked",
                "reason_code": "ambiguous-same-target-retail-source-order",
            }
        if repair_address < retail_address:
            return {
                "state": "launchable",
                "reason_code": "earlier-source-divergence",
            }
        return {
            "state": "blocked",
            "reason_code": (
                "earlier-or-same-caller-candidate-independent-retail-blocker"
            ),
            "retail_address": normalize_address(retail_address),
            "repair_address": normalize_address(repair_address),
        }

    retail_launchability_by_target: dict[str, dict[str, Any]] = {}
    for descriptor, _packet_type, _ordinal in descriptor_rows:
        if not isinstance(descriptor, Mapping):
            continue
        target_id = str(descriptor.get("target_id", ""))
        retail_descriptor = retail_by_target.get(target_id)
        if retail_descriptor is None:
            continue
        retail_launchability_by_target[target_id] = retail_precedes_repair(
            descriptor, retail_descriptor
        )
    blocked_retail_targets = sorted(
        target_id
        for target_id, launchability in retail_launchability_by_target.items()
        if launchability.get("state") != "launchable"
    )
    if blocked_retail_targets:
        descriptor_rows = [
            row
            for row in descriptor_rows
            if not (
                isinstance(row[0], Mapping)
                and str(row[0].get("target_id", ""))
                in blocked_retail_targets
            )
        ]
        blocked.extend(
            {
                "target_id": target_id,
                "reason_code": (
                    "candidate-independent-retail-blocker-before-source"
                ),
                "retail_fact_packet_required": True,
                "launchability": retail_launchability_by_target[target_id],
            }
            for target_id in blocked_retail_targets
        )
    prospective_profile = prospective_wol_profile_convergence_route(
        document, generation
    )
    if prospective_profile.get("state") == "blocked":
        descriptor_rows = [
            row
            for row in descriptor_rows
            if not (
                isinstance(row[0], Mapping)
                and row[0].get("target_id") == WOL_PROFILE_MATRIX_TARGET_ID
            )
        ]
        blocked.append(deepcopy(dict(prospective_profile)))
    elif prospective_profile.get("state") == "launchable":
        route = prospective_profile.get("route")
        source_descriptor = prospective_profile.get("descriptor")
        if not (
            isinstance(route, Mapping)
            and isinstance(source_descriptor, Mapping)
        ):
            raise ProgressError(
                "launchable WOL prospective profile route lacks exact facts"
            )
        routed_descriptor = deepcopy(dict(source_descriptor))
        for stale_field in (
            "source_atomic_definition_paths",
            "source_atomic_header_definition_resolution",
            "dependent_header_provenance",
        ):
            routed_descriptor.pop(stale_field, None)
        routed_descriptor.update(
            {
                "packet_type": "call-contract-edit-v1",
                "routing_kind": "wol-prospective-profile-source-v1",
                "source_edit_paths": list(route["source_edit_paths"]),
                "target_source_edit_paths": list(route["source_edit_paths"]),
                "registered_source_paths": list(
                    route["registered_source_paths"]
                ),
                "header_paths": [],
                "definition_source_paths": list(
                    route["definition_source_paths"]
                ),
                "dependency_paths": list(route["dependency_paths"]),
                "symbol_ids": list(route["scope_ids"]),
                "physical_block_ids": list(route["physical_block_ids"]),
                "prospective_profile_handoff": deepcopy(dict(route)),
            }
        )
        descriptor_rows = [
            row
            for row in descriptor_rows
            if not (
                isinstance(row[0], Mapping)
                and row[0].get("target_id") == WOL_PROFILE_MATRIX_TARGET_ID
            )
        ]
        descriptor_rows.append(
            (
                routed_descriptor,
                "call-contract-edit-v1",
                len(ordinary_descriptors) + len(dependent_descriptors),
            )
        )

    def repair_priority(row: tuple[Any, str, int]) -> tuple[int, int]:
        descriptor, _packet_type, original_ordinal = row
        provenance = (
            descriptor.get("dependent_header_provenance")
            if isinstance(descriptor, Mapping)
            else None
        )
        exact_c2373_definition = bool(
            isinstance(provenance, Mapping)
            and provenance.get("recovery_kind")
            == "exact-selected-target-definition-abi"
            and provenance.get("repair_scope")
            == "registered-definition-only"
            and isinstance(
                provenance.get("compile_diagnostic_recovery_rows"), list
            )
            and provenance.get("compile_diagnostic_recovery_rows")
        )
        return (0 if exact_c2373_definition else 1, original_ordinal)

    descriptor_rows.sort(key=repair_priority)
    candidates: list[tuple[str, dict[str, Any]]] = []
    for ordinal, (
        descriptor,
        default_packet_type,
        _original_ordinal,
        ) in enumerate(descriptor_rows, 1):
        if not isinstance(descriptor, Mapping):
            raise ProgressError(
                "call-contract convergence repair descriptor must be an object"
            )
        target_id = str(descriptor.get("target_id", ""))
        if not target_id:
            continue
        if default_packet_type == "call-contract-dependent-owner-edit-v1":
            owner_pair_key = deepcopy(descriptor.get("owner_pair_key"))
            target_result = target_results_by_id.get(target_id)
            target_has_divergence_partition = bool(
                isinstance(target_result, Mapping)
                and isinstance(
                    target_result.get("caller_divergences"), list
                )
            )
            target_divergences = (
                list(target_result.get("caller_divergences", []))
                if target_has_divergence_partition
                else []
            )
            unmatched_divergences = list(target_divergences)
            partition_malformed = False
            if target_has_divergence_partition:
                for routed_divergence in descriptor.get(
                    "caller_divergences", []
                ):
                    try:
                        unmatched_divergences.remove(routed_divergence)
                    except ValueError:
                        partition_malformed = True
                        break
            target_launchability = []
            if partition_malformed:
                target_launchability.append(
                    {
                        "kind": "call-contract-target-wide-repair-launchability",
                        "contract_version": 1,
                        "state": "blocked",
                        "reason_code": "malformed-target-wide-repair-partition",
                        "blocking_divergence_key": None,
                        "first_unreachable_route_key": None,
                    }
                )
            elif unmatched_divergences:
                target_launchability.append(
                    dependent_owner_repair_launchability(
                        descriptor,
                        {
                            "target_id": target_id,
                            "divergences": unmatched_divergences,
                        },
                    )
                )
            target_launchability.extend(
                dependent_owner_repair_launchability(descriptor, blocker)
                for blocker in dependent_blockers_by_target.get(target_id, [])
            )
            blocking_launchability = next(
                (
                    row
                    for row in target_launchability
                    if row.get("state") != "launchable"
                ),
                None,
            )
            if blocking_launchability is not None:
                blocked.append(
                    {
                        "target_id": target_id,
                        "owner_pair_key": owner_pair_key,
                        "reason_code": "target-wide-verifier-blocked-before-repair",
                        "launchability": deepcopy(blocking_launchability),
                    }
                )
                continue
        source_edit_paths = [
            str(value) for value in descriptor.get("source_edit_paths", [])
        ]
        definition_source_paths = [
            str(value)
            for value in descriptor.get("definition_source_paths", [])
        ]
        dependency_paths = [
            str(value) for value in descriptor.get("dependency_paths", [])
        ]
        header_paths = [
            str(value) for value in descriptor.get("header_paths", [])
        ]
        target_source_edit_paths = [
            str(value)
            for value in descriptor.get("target_source_edit_paths", [])
        ]
        source_atomic_definition_paths = [
            str(value)
            for value in descriptor.get(
                "source_atomic_definition_paths", []
            )
        ]
        source_atomic_resolution = descriptor.get(
            "source_atomic_header_definition_resolution"
        )
        dependent_header_provenance = descriptor.get(
            "dependent_header_provenance"
        )
        compile_recovery_kind = (
            str(dependent_header_provenance.get("recovery_kind", ""))
            if isinstance(dependent_header_provenance, Mapping)
            else ""
        )
        selected_target_compile_definition = (
            compile_recovery_kind
            == "exact-selected-target-definition-abi"
        )
        if not source_edit_paths:
            raise ProgressError(
                f"call-contract convergence repair {target_id} has no exact writes"
            )
        cursor = str(descriptor.get("cursor", ""))
        packet_type = str(
            descriptor.get(
                "packet_type",
                default_packet_type,
            )
        )
        if packet_type not in {
            "call-contract-edit-v1",
            "call-contract-converge-edit-v1",
            "call-contract-dependent-owner-edit-v1",
        }:
            raise ProgressError(
                f"call-contract convergence repair {target_id} has invalid packet type"
            )
        dependent_owner = packet_type == "call-contract-dependent-owner-edit-v1"
        ordinary_source_atomic = bool(
            not dependent_owner
            and descriptor.get("routing_kind") is None
        )
        if ordinary_source_atomic and not (
            target_source_edit_paths
            and isinstance(source_atomic_resolution, Mapping)
            and source_edit_paths
            == list(
                dict.fromkeys(
                    (
                        *target_source_edit_paths,
                        *source_atomic_definition_paths,
                    )
                )
            )
            and set(source_atomic_definition_paths).issubset(
                definition_source_paths
            )
            and set(source_atomic_definition_paths).issubset(
                dependency_paths
            )
        ):
            raise ProgressError(
                f"call-contract convergence repair {target_id} lacks its "
                "exact source-atomic writable-header definition closure"
            )
        if selected_target_compile_definition:
            declaration_paths = [
                str(value)
                for value in dependent_header_provenance.get(
                    "declaration_paths", []
                )
            ]
            if not (
                packet_type == "call-contract-converge-edit-v1"
                and descriptor.get("routing_kind")
                == "exact-dependent-declaration-header-definition"
                and dependent_header_provenance.get("target_id") == target_id
                and dependent_header_provenance.get("repair_scope")
                == "registered-definition-only"
                and dependent_header_provenance.get(
                    "candidate_expected_truth"
                )
                is False
                and len(source_edit_paths) == 1
                and source_edit_paths == definition_source_paths
                and len(declaration_paths) == 1
                and declaration_paths == header_paths
                and declaration_paths[0] in dependency_paths
                and declaration_paths[0] not in source_edit_paths
            ):
                raise ProgressError(
                    f"selected-target compile repair {target_id} lacks its exact definition-only closure"
                )
        if dependent_owner:
            dependency_paths = [
                str(value)
                for value in generation.get("dependency_paths", [])
            ]
            if not set(source_edit_paths).issubset(dependency_paths):
                raise ProgressError(
                    f"dependent-owner repair {target_id} writes outside the signed dependency closure"
                )
            if not definition_source_paths:
                definition_source_paths = sorted(
                    {
                        str(path)
                        for route in descriptor.get("routed_identities", [])
                        if isinstance(route, Mapping)
                        for path in route.get("owner_resolution", {}).get(
                            "definition_paths", []
                        )
                    },
                    key=lambda path: (path.casefold(), path),
                )
        worker_root = document._fresh_root(
            "worker-call-contract-convergence",
            f"{ordinal:04d}",
            document.revision,
        )
        worker_command = (
            "python tools/recoil.py verify call-contract "
            f"--target {_command_arg(target_id)} --all-authored-bodies "
            "--progress {progress_path} "
            f"--build-root {_command_arg(worker_root)} --json"
        )
        prospective_profile_handoff = descriptor.get(
            "prospective_profile_handoff"
        )
        if prospective_profile_handoff is not None:
            if not (
                packet_type == "call-contract-edit-v1"
                and descriptor.get("routing_kind")
                == "wol-prospective-profile-source-v1"
                and _valid_wol_profile_source_handoff_route(
                    prospective_profile_handoff
                )
            ):
                raise ProgressError(
                    f"prospective WOL repair {target_id} lacks its exact route"
                )
            worker_command = (
                "python tools/recoil.py verify call-contract "
                f"--target {_command_arg(target_id)} --profile-matrix "
                "--progress {progress_path} "
                f"--build-root {_command_arg(worker_root)} --json"
            )
        if selected_target_compile_definition:
            objective_prefix = (
                "Repair only the selected registered target definition "
                "identified by the exact VC5 declaration/definition "
                "calling-convention diagnostic; keep the registered "
                "declaration header read-only; then make target "
            )
        elif dependent_owner:
            objective_prefix = (
                "Repair only the exact dependent declaration/definition "
                "owner identified by candidate-observed ABI provenance; "
                "then make target "
            )
        else:
            objective_prefix = (
                "Make every authored caller in the exact registered target "
            )
        if prospective_profile_handoff is not None:
            objective_prefix = (
                "Using only the prospective in-memory WOL-only /Ob1 comparison, "
                "repair the exact WOL.cpp and event-sink source closure so "
            )
        work_id = _unique_work_id(
            document,
            "recoil:work:call-contract-converge-"
            f"{ordinal:04d}-r{document.revision + 1}",
        )
        claims: list[dict[str, str]] = [
            *(
                {"kind": "path", "id": path, "access": "write"}
                for path in source_edit_paths
            ),
            *(
                {"kind": "path", "id": path, "access": "read"}
                for path in dependency_paths
                if path not in source_edit_paths
            ),
            {"kind": "output-root", "id": worker_root, "access": "write"},
            {"kind": "whole-project-build", "id": "recoil", "access": "read"},
            {"kind": "verification-target", "id": target_id, "access": "read"},
            {"kind": "reference", "id": "support/Recoil.exe", "access": "read"},
            {"kind": "binary-ninja-db", "id": "Recoil.bndb", "access": "read"},
            {"kind": "tracker", "id": "recoil", "access": "read"},
        ]
        work = bind_work_packet_contract(
            document,
            {
                "binary": "recoil",
                "handoff_role": "recoil_source_worker",
                "packet_type": packet_type,
                "state": "ready",
                "phase": "authored-call-contract",
                "lane": "primary",
                "generation_id": str(generation.get("generation_id", "")),
                "target_id": target_id,
                "cursor": cursor,
                "block_id": str(
                    next(iter(descriptor.get("physical_block_ids", [])), "")
                ),
                "covered_block_ids": list(
                    descriptor.get("physical_block_ids", [])
                ),
                "scope_ids": list(descriptor.get("symbol_ids", [])),
                "target_ids": [target_id],
                "original_slice_ids": list(
                    descriptor.get("original_slice_ids", [])
                ),
                "allowed_paths": source_edit_paths,
                "source_edit_paths": source_edit_paths,
                "definition_source_paths": definition_source_paths,
                "dependency_paths": dependency_paths,
                **(
                    {
                        "owner_pair_key": deepcopy(
                            descriptor.get("owner_pair_key")
                        ),
                    }
                    if dependent_owner
                    else {}
                ),
                **(
                    {
                        "prospective_profile_handoff": deepcopy(
                            dict(prospective_profile_handoff)
                        ),
                        "repair_route": "wol-prospective-profile-source-v1",
                        "routing_kind": "wol-prospective-profile-source-v1",
                        "nonaccepting": True,
                        "acceptance_eligible": False,
                        "manifest_mutation_allowed": False,
                        "candidate_expected_truth": False,
                        "expected_truth": (
                            WOL_PROFILE_SOURCE_HANDOFF_EXPECTED_TRUTH
                        ),
                    }
                    if prospective_profile_handoff is not None
                    else {}
                ),
                **(
                    {
                        "target_source_edit_paths": (
                            target_source_edit_paths
                        ),
                        "source_atomic_definition_paths": (
                            source_atomic_definition_paths
                        ),
                        "source_atomic_header_definition_resolution": deepcopy(
                            source_atomic_resolution
                        ),
                    }
                    if ordinary_source_atomic
                    else {}
                ),
                **(
                    {
                        "repair_route": (
                            "selected-target-compile-definition-v1"
                        ),
                        "routing_kind": str(
                            descriptor.get("routing_kind", "")
                        ),
                        "compile_recovery_kind": compile_recovery_kind,
                        "declaration_paths": declaration_paths,
                    }
                    if selected_target_compile_definition
                    else {}
                ),
                "validation_commands": [worker_command],
                "objective": objective_prefix
                + (
                    f"{target_id} pass the nonaccepting retail call-contract verifier."
                ),
                "stop_condition": (
                    (
                        "PASS the complete nonaccepting WOL-only/P1 profile matrix "
                        "with all 109 retail-normalized bodies and the inline sentinel, "
                        "or return its first regression/repair and any concrete "
                        "out-of-scope source contradiction."
                    )
                    if prospective_profile_handoff is not None
                    else (
                        "PASS the complete target-wide verifier, or return every deterministic "
                        "caller divergence and any concrete out-of-scope source contradiction."
                    )
                ),
                "first_divergence": deepcopy(
                    descriptor.get("first_divergence")
                    or next(
                        iter(descriptor.get("caller_divergences", [])), None
                    )
                ),
                "caller_divergences": deepcopy(
                    descriptor.get("caller_divergences", [])
                ),
                "routed_identities": deepcopy(
                    descriptor.get("routed_identities", [])
                ),
                "required_return_fields": [
                    "packet_id",
                    "outcome",
                    "changed_paths",
                    "validation_command",
                    "passed",
                    "first_divergence",
                    "caller_divergences",
                    "scope_contradiction",
                    *(
                        ["profile_matrix_result", "retail_normalization"]
                        if prospective_profile_handoff is not None
                        else []
                    ),
                ],
                "resource_claims": claims,
            },
        )
        conflicts, incomplete = _candidate_active_conflicts(
            document,
            work_id=work_id,
            work=work,
            issue_ledger=issue_ledger,
        )
        if incomplete or conflicts:
            blocked.append(
                {
                    "target_id": target_id,
                    "reason_code": "active-resource-conflict",
                    "incomplete": incomplete,
                    "conflicts": conflicts,
                }
            )
            continue
        candidates.append((work_id, work))
    return candidates, blocked


def _call_contract_continuation_producer_candidates(
    document: ProgressDocument,
    *,
    progress_path: Path,
    issue_ledger: Path,
) -> tuple[list[tuple[str, dict[str, Any]]], list[dict[str, Any]]]:
    """Derive the sole branchless diagnostic producer from terminal debt."""

    if continuation_state(document).get("state") != "none":
        return [], []
    retained = [
        (str(work_id), work)
        for work_id, work in document.collection("work_items").items()
        if isinstance(work, Mapping)
        and work.get("state") == "returned-tool-blocked"
        and isinstance(work.get(RETURN_PROVENANCE_FIELD), Mapping)
        and work[RETURN_PROVENANCE_FIELD].get("linked_issue_id") == LINKED_TOOL_ISSUE
    ]
    active_or_terminal_producers = [
        str(work_id)
        for work_id, work in document.collection("work_items").items()
        if isinstance(work, Mapping)
        and work.get("packet_type") == CONTINUATION_PRODUCER_TYPE
        and work.get("state") not in {"abandoned", "archived"}
    ]
    if active_or_terminal_producers:
        return [], [{"reason_code": "continuation-producer-already-retained", "work_item_ids": active_or_terminal_producers}]
    if len(retained) != 1:
        return [], ([{"reason_code": "continuation-predecessor-not-unique", "count": len(retained)}] if retained else [])
    predecessor_id, predecessor = retained[0]
    provenance = predecessor[RETURN_PROVENANCE_FIELD]
    target_id = str(provenance.get("target_id", ""))
    if not target_id:
        return [], [{"reason_code": "continuation-predecessor-target-missing"}]
    ordinal = 1
    work_id = _unique_work_id(
        document,
        f"recoil:work:call-contract-continuation-producer-{ordinal:04d}-r{document.revision + 1}",
    )
    build_root = document._fresh_root(
        "worker-call-contract-continuation-producer", f"{ordinal:04d}", document.revision
    )
    command = (
        "python tools/recoil.py verify call-contract "
        f"--target {target_id} --all-authored-bodies --all-caller-divergences "
        f"--packet-id {work_id} --progress {_command_arg(_progress_command_path(progress_path))} "
        f"--build-root {_command_arg(build_root)} --json"
    )
    work = bind_work_packet_contract(
        document,
        {
            "binary": "recoil",
            "handoff_role": "recoil_verifier",
            "packet_type": CONTINUATION_PRODUCER_TYPE,
            "state": "ready",
            "phase": "authored-call-contract",
            "lane": "primary",
            "target_id": target_id,
            "cursor": str(predecessor.get("cursor", "")),
            "block_id": str(predecessor.get("block_id", "")),
            "covered_block_ids": deepcopy(list(predecessor.get("covered_block_ids", []))),
            "scope_ids": deepcopy(list(predecessor.get("scope_ids", []))),
            "target_ids": deepcopy(list(predecessor.get("target_ids", []))),
            "original_slice_ids": deepcopy(list(predecessor.get("original_slice_ids", []))),
            "allowed_paths": [],
            "source_edit_paths": [],
            "definition_source_paths": deepcopy(list(predecessor.get("definition_source_paths", []))),
            "dependency_paths": deepcopy(list(predecessor.get("dependency_paths", []))),
            "predecessor_work_item_id": predecessor_id,
            "branchless": True,
            "nonaccepting": True,
            "acceptance_eligible": False,
            "worker_acceptance_allowed": False,
            "candidate_expected_truth": False,
            "validation_commands": [command],
            "objective": "Produce one fresh exact-target exhaustive divergence diagnostic for parent-only repair routing.",
            "stop_condition": "Return the complete target result, including every caller divergence, or one exact verifier blocker.",
            "required_return_fields": [
                "packet_id", "outcome", "validation_command", "passed",
                "first_divergence", "caller_divergences", "scope_contradiction",
            ],
            "resource_claims": [
                {"kind": "binary-ninja-db", "id": "Recoil.bndb", "access": "read"},
                {"kind": "tracker", "id": "recoil", "access": "read"},
                {"kind": "verification-target", "id": target_id, "access": "read"},
                {"kind": "output-root", "id": build_root, "access": "write"},
            ],
        },
    )
    conflicts, incomplete = _candidate_active_conflicts(
        document, work_id=work_id, work=work, issue_ledger=issue_ledger
    )
    if incomplete or conflicts:
        return [], [{"reason_code": "active-resource-conflict", "incomplete": incomplete, "conflicts": conflicts}]
    return [(work_id, work)], []


def _call_contract_continuation_child_candidate(
    document: ProgressDocument,
) -> tuple[str, dict[str, Any]] | None:
    state = continuation_state(document)
    if state.get("state") != "descriptor-ready":
        return None
    checkpoint = state.get("checkpoint")
    child = checkpoint.get("child_descriptor") if isinstance(checkpoint, Mapping) else None
    if not isinstance(child, Mapping):
        raise ProgressError("descriptor-ready continuation has no child descriptor")
    work_id = _unique_work_id(
        document,
        f"recoil:work:call-contract-repair-continuation-0001-r{document.revision + 1}",
    )
    packet = deepcopy(dict(child))
    target_id = str(packet.get("target_id", ""))
    packet["validation_commands"] = [
        "python tools/recoil.py verify call-contract "
        f"--target {target_id} --all-authored-bodies --all-caller-divergences "
        f"--packet-id {work_id} --progress {{progress_path}} "
        "--build-root {packet_build_root} --json"
    ]
    return work_id, packet


def _call_contract_retail_fact_candidates(
    document: ProgressDocument,
    *,
    issue_ledger: Path,
) -> tuple[list[tuple[str, dict[str, Any]]], list[dict[str, Any]]]:
    """Build descriptor-only read packets for current retail BN blockers."""

    if not isinstance(getattr(document, "data", None), Mapping):
        return [], []
    state = convergence_generation_state(document, copy_generation=False)
    generation = state.get("generation")
    if state.get("current") is not True or not isinstance(
        generation, Mapping
    ):
        return [], []
    scopes = retail_fact_packet_scopes(
        generation.get("retail_blocker_descriptors")
    )
    active_scope_ids = {
        str(work.get("retail_fact_scope", {}).get("scope_id", ""))
        for work in document.collection("work_items").values()
        if isinstance(work, Mapping)
        and work.get("packet_type") == RETAIL_FACT_PACKET_TYPE
        and work.get("generation_id") == generation.get("generation_id")
        and isinstance(work.get("retail_fact_scope"), Mapping)
        and isinstance(work.get("reservation"), Mapping)
        and work["reservation"].get("state") == "active"
    }
    candidates: list[tuple[str, dict[str, Any]]] = []
    blocked: list[dict[str, Any]] = []
    for ordinal, scope in enumerate(scopes, 1):
        scope_id = str(scope["scope_id"])
        if scope_id in active_scope_ids:
            continue
        target_id = str(scope["target_id"])
        cursor = str(scope["cursor"])
        work_id = _unique_work_id(
            document,
            "recoil:work:call-contract-retail-fact-"
            f"{ordinal:04d}-{cursor.replace('0x', '')}-r{document.revision + 1}",
        )
        claims: list[dict[str, str]] = [
            *(
                {"kind": "block", "id": block_id, "access": "read"}
                for block_id in scope["physical_block_ids"]
            ),
            {
                "kind": "verification-target",
                "id": target_id,
                "access": "read",
            },
            {
                "kind": "binary-ninja-db",
                "id": "Recoil.bndb",
                "access": "read",
            },
            {"kind": "tracker", "id": "recoil", "access": "read"},
            {
                "kind": "lane",
                "id": scope_id,
                "access": "write",
            },
        ]
        work = bind_work_packet_contract(
            document,
            {
                "binary": "recoil",
                "handoff_role": "recoil_bn_fact_mapper",
                "packet_type": RETAIL_FACT_PACKET_TYPE,
                "state": "ready",
                "phase": "authored-call-contract",
                "lane": "primary",
                "generation_id": str(scope["generation_id"]),
                "target_id": target_id,
                "cursor": cursor,
                "block_id": str(scope["physical_block_ids"][0]),
                "covered_block_ids": list(scope["physical_block_ids"]),
                "scope_ids": list(scope["symbol_ids"]),
                "target_ids": [target_id],
                "allowed_paths": [],
                "source_edit_paths": [],
                "read_only": True,
                "nonaccepting": True,
                "candidate_expected_truth": False,
                "expected_truth": CONVERGENCE_EXPECTED_TRUTH,
                "retail_fact_scope": deepcopy(scope),
                "validation_commands": [
                    "python tools/recoil.py binja preflight --binary recoil --strict"
                ],
                "objective": (
                    "Inspect only the already-open Recoil.bndb and return complete "
                    "raw assembly/xref/call-provenance facts for the exact retail "
                    f"blocker addresses {', '.join(scope['addresses'])}; do not "
                    "guess an identity or derive expected truth from candidate output."
                ),
                "stop_condition": (
                    "Return complete address-labeled raw BN facts for every blocker, "
                    "including explicit unresolved/truncation state and direct evidence "
                    "transcript paths, without mutating BN, source, tracker, or ledgers."
                ),
                "required_return_fields": [
                    "packet_id",
                    "outcome",
                    "changed_paths",
                    "validation_command",
                    "passed",
                    "retail_fact_rows",
                    "unresolved_facts",
                    "evidence_transcript_paths",
                    "scope_contradiction",
                ],
                "resource_claims": claims,
            },
        )
        conflicts, incomplete = _candidate_active_conflicts(
            document,
            work_id=work_id,
            work=work,
            issue_ledger=issue_ledger,
        )
        if incomplete or conflicts:
            blocked.append(
                {
                    "target_id": target_id,
                    "scope_id": scope_id,
                    "reason_code": "active-resource-conflict",
                    "incomplete": incomplete,
                    "conflicts": conflicts,
                }
            )
            continue
        candidates.append((work_id, work))
    return candidates, blocked


def _call_contract_mixed_obligation_candidates(
    document: ProgressDocument,
    *,
    progress_path: Path,
    issue_ledger: Path,
) -> tuple[list[tuple[str, dict[str, Any]]], list[dict[str, Any]]]:
    """Route independently writable source/profile/verifier/linker obligations.

    Target identity is deliberately a read claim, not a mutex.  Compatibility
    is decided only by the normalized resource claims, so two obligations for
    one target may coexist when their actual writable paths are disjoint.
    """

    if not isinstance(getattr(document, "data", None), Mapping):
        return [], []
    state = convergence_generation_state(document, copy_generation=False)
    generation = state.get("generation")
    if state.get("current") is not True or not isinstance(generation, Mapping):
        return [], []
    descriptors = generation.get("obligation_descriptors")
    if descriptors is None:
        return [], []
    if not isinstance(descriptors, list):
        raise ProgressError(
            "call-contract convergence obligation_descriptors must be a list"
        )
    packet_types = CALL_CONTRACT_VERIFICATION_ACCEPTANCE_POLICY[
        "obligation_packet_types"
    ]
    candidates: list[tuple[str, dict[str, Any]]] = []
    blocked: list[dict[str, Any]] = []
    seen_ids: set[str] = set()
    for ordinal, descriptor in enumerate(descriptors, 1):
        if not isinstance(descriptor, Mapping):
            raise ProgressError("call-contract obligation descriptor must be an object")
        obligation_id = str(descriptor.get("obligation_id", ""))
        kind = str(descriptor.get("obligation_kind", ""))
        target_id = str(descriptor.get("target_id", ""))
        if (
            not obligation_id
            or obligation_id in seen_ids
            or kind not in {"source", "profile", "verifier", "linker"}
            or not target_id
        ):
            raise ProgressError(
                "call-contract obligation descriptor has invalid identity/kind/target"
            )
        seen_ids.add(obligation_id)
        write_paths = [str(path) for path in descriptor.get("write_paths", [])]
        dependency_paths = [
            str(path) for path in descriptor.get("dependency_paths", [])
        ]
        worker_root = document._fresh_root(
            "worker-call-contract-obligation",
            f"{ordinal:04d}",
            document.revision,
        )
        validation_commands = [
            str(command)
            .replace("{packet_build_root}", _command_arg(worker_root))
            for command in descriptor.get("validation_commands", [])
        ]
        if not write_paths or not validation_commands:
            blocked.append(
                {
                    "obligation_id": obligation_id,
                    "target_id": target_id,
                    "reason_code": "obligation-lacks-governed-write-or-validation",
                }
            )
            continue
        role = (
            "recoil_source_worker"
            if kind == "source"
            else "recoil_tool_maintainer"
        )
        packet_type = str(packet_types[kind])
        work_id = _unique_work_id(
            document,
            "recoil:work:call-contract-obligation-"
            f"{ordinal:04d}-r{document.revision + 1}",
        )
        claims = [
            *(
                {"kind": "path", "id": path, "access": "write"}
                for path in write_paths
            ),
            *(
                {"kind": "path", "id": path, "access": "read"}
                for path in dependency_paths
                if path not in write_paths
            ),
            {"kind": "output-root", "id": worker_root, "access": "write"},
            {"kind": "whole-project-build", "id": "recoil", "access": "read"},
            {"kind": "verification-target", "id": target_id, "access": "read"},
            {"kind": "reference", "id": "support/Recoil.exe", "access": "read"},
            {"kind": "tracker", "id": "recoil", "access": "read"},
        ]
        if descriptor.get("requires_binary_ninja") is True:
            claims.append(
                {"kind": "binary-ninja-db", "id": "Recoil.bndb", "access": "read"}
            )
        work = bind_work_packet_contract(
            document,
            {
                "binary": "recoil",
                "handoff_role": role,
                "packet_type": packet_type,
                "state": "ready",
                "phase": "authored-call-contract",
                "lane": "primary",
                "generation_id": str(generation.get("generation_id", "")),
                "obligation_id": obligation_id,
                "obligation_kind": kind,
                "target_id": target_id,
                "target_ids": [target_id],
                "cursor": str(descriptor.get("cursor", "")),
                "scope_ids": list(descriptor.get("symbol_ids", [])),
                "covered_block_ids": list(
                    descriptor.get("physical_block_ids", [])
                ),
                "allowed_paths": write_paths,
                "source_edit_paths": write_paths if kind == "source" else [],
                "dependency_paths": dependency_paths,
                "validation_commands": validation_commands,
                "objective": str(descriptor.get("objective", "")),
                "stop_condition": str(descriptor.get("stop_condition", "")),
                "first_divergence": deepcopy(
                    descriptor.get("first_divergence")
                ),
                "nonaccepting": True,
                "acceptance_eligible": False,
                "candidate_expected_truth": False,
                "worker_acceptance_allowed": False,
                "requires_binary_ninja": descriptor.get("requires_binary_ninja")
                is True,
                "required_return_fields": [
                    "packet_id",
                    "obligation_id",
                    "outcome",
                    "changed_paths",
                    "validation_command",
                    "passed",
                    "first_divergence",
                    "scope_contradiction",
                ],
                "resource_claims": claims,
            },
        )
        conflicts, incomplete = _candidate_active_conflicts(
            document,
            work_id=work_id,
            work=work,
            issue_ledger=issue_ledger,
        )
        if incomplete or conflicts:
            blocked.append(
                {
                    "obligation_id": obligation_id,
                    "target_id": target_id,
                    "reason_code": "active-resource-conflict",
                    "incomplete": incomplete,
                    "conflicts": conflicts,
                }
            )
            continue
        candidates.append((work_id, work))
    return candidates, blocked


def _active_matching_work(
    document: ProgressDocument,
    *,
    packet_lane: str,
    phase: str,
    cursor: str,
) -> tuple[str, Mapping[str, Any]] | None:
    matches: list[tuple[str, Mapping[str, Any]]] = []
    for work_id, work in document.collection("work_items").items():
        if not isinstance(work, Mapping):
            continue
        reservation = work.get("reservation")
        if (
            work.get("state") == "active"
            and isinstance(reservation, Mapping)
            and reservation.get("state") == "active"
            and str(work.get("binary", "recoil")) == "recoil"
            and str(work.get("lane", "primary")) == packet_lane
            and str(work.get("phase", "")) == phase
            and str(work.get("cursor", "")) == cursor
        ):
            matches.append((str(work_id), work))
    if len(matches) > 1:
        raise ProgressError(
            f"multiple active packets exist for {packet_lane} {phase} {cursor}: "
            + ", ".join(work_id for work_id, _work in matches)
        )
    return matches[0] if matches else None


def _candidate_active_conflicts(
    document: ProgressDocument,
    *,
    work_id: str,
    work: Mapping[str, Any],
    issue_ledger: Path,
) -> tuple[list[dict[str, Any]], list[str]]:
    claims, complete, _source = work_resource_claims(work)
    if not complete:
        return [], ["candidate packet has incomplete normalized resource claims"]
    conflicts: list[dict[str, Any]] = []
    incomplete: list[str] = []
    for other_id, other in document.collection("work_items").items():
        if not isinstance(other, Mapping):
            continue
        reservation = other.get("reservation")
        if not isinstance(reservation, Mapping) or reservation.get("state") != "active":
            continue
        other_claims, other_complete, _ = work_resource_claims(other)
        if not other_complete:
            incomplete.append(str(other_id))
            continue
        conflicts.extend(
            resource_claim_conflicts(
                claims,
                str(other_id),
                other_claims,
                second_owner_id=str(other.get("owner_id", "")),
                second_block_id=str(other.get("block_id", "")),
            )
        )
    try:
        from _recoil.commands.workspace_issues import workspace_issue_reservation_conflicts

        conflicts.extend(
            workspace_issue_reservation_conflicts(issue_ledger, work_id, claims)
        )
    except (OSError, ValueError) as exc:
        incomplete.append(str(exc))
    return conflicts, incomplete


def _blocked_opportunity(
    lane: str,
    *,
    cursor: str,
    state: str,
    reason_code: str,
    reason: str,
    required_parent_action: str,
) -> dict[str, Any]:
    return {
        "lane": lane,
        "cursor": cursor,
        "state": state,
        "launchability": state,
        "reason_code": reason_code,
        "reason": reason,
        "required_parent_action": required_parent_action,
        "prerequisite_state": "satisfied" if state in {"complete", "caught-up"} else "missing",
        "resource_claims": [],
        "conflicts": [],
        "work_item_id": "",
        "packet": None,
    }


def _blocked_convergence_repair_opportunity(
    *,
    cursor: str,
    repair_blockers: Iterable[Mapping[str, Any]],
    retail_fact_blockers: Iterable[Mapping[str, Any]],
) -> dict[str, Any]:
    """Report a current repair generation with no launchable primary packet."""

    blockers = [
        deepcopy(dict(blocker))
        for blocker in (*tuple(repair_blockers), *tuple(retail_fact_blockers))
        if isinstance(blocker, Mapping)
    ]
    reason_codes = {
        str(blocker.get("reason_code", "")) for blocker in blockers
    }
    nested_reason_codes = {
        str(launchability.get("reason_code", ""))
        for blocker in blockers
        for launchability in (blocker.get("launchability"),)
        if isinstance(launchability, Mapping)
    }
    if reason_codes & {
        "active-generation-repair-reservation",
    }:
        parent_action = "handoff-active-packet"
    elif reason_codes & {"active-resource-conflict"}:
        parent_action = "wait-or-close-conflicting-packet"
    elif reason_codes & {
        "target-wide-verifier-blocked-before-repair",
    } or nested_reason_codes & {
        "earlier-target-wide-verifier-blocker",
        "earlier-same-caller-verifier-blocker",
        "unresolved-same-caller-verifier-blocker",
        "malformed-target-wide-repair-partition",
    }:
        parent_action = "resolve-target-wide-verifier-blocker"
    elif reason_codes & {
        "candidate-independent-retail-blocker-before-source",
    }:
        parent_action = "claim-retail-fact-packets"
    elif reason_codes & {
        "incomplete-prospective-profile-proof",
    }:
        parent_action = "repair-prospective-profile-proof"
    else:
        parent_action = "resolve-convergence-repair-blockers"
    first_reason_code = next(
        (
            str(blocker.get("reason_code", ""))
            for blocker in blockers
            if str(blocker.get("reason_code", ""))
        ),
        "convergence-repairs-not-launchable",
    )
    return {
        "lane": "primary",
        "cursor": cursor,
        "state": "blocked",
        "launchability": "blocked",
        "reason_code": first_reason_code,
        "reason": (
            "the current convergence generation has repair descriptors but "
            f"no launchable primary packet; first blocker is {first_reason_code}"
        ),
        "required_parent_action": parent_action,
        "prerequisite_state": "missing",
        "resource_claims": [],
        "conflicts": [],
        "work_item_id": "",
        "packet": None,
        "repair_packet_count": 0,
        "retail_fact_packet_count": 0,
        "repair_blockers": blockers,
    }


def _current_claim_opportunities(
    document: ProgressDocument,
    pipeline: Mapping[str, Any],
    *,
    progress_path: Path,
    issue_ledger: Path,
    lanes: Iterable[str] = ("primary", "authored", "object"),
) -> dict[str, dict[str, Any]]:
    requested_lanes = tuple(dict.fromkeys(str(lane) for lane in lanes))
    invalid_lanes = sorted(set(requested_lanes) - {"primary", "authored", "object"})
    if invalid_lanes:
        raise ProgressError(f"invalid claim opportunity lane(s): {', '.join(invalid_lanes)}")
    result: dict[str, dict[str, Any]] = {}
    specs: list[tuple[str, str, str, str]] = []
    primary_lane = str(pipeline.get("primary_lane", ""))
    primary_cursor = str(pipeline.get("cursor", ""))
    primary_phase = str(pipeline.get("phase", ""))
    if "primary" in requested_lanes:
        if pipeline.get("complete"):
            result["primary"] = _blocked_opportunity(
                "primary",
                cursor=primary_cursor,
                state="complete",
                reason_code="pipeline-complete",
                reason="pipeline complete",
                required_parent_action="none",
            )
        elif primary_lane == "order":
            specs.append(("primary", "order", primary_cursor, primary_phase))
        elif primary_lane == "call-contract":
            convergence = pipeline.get("authored_call_contract_convergence", {})
            convergence_mode = (
                str(convergence.get("mode", ""))
                if isinstance(convergence, Mapping)
                else ""
            )
            result["primary"] = _blocked_opportunity(
                "primary",
                cursor=primary_cursor,
                state="blocked",
                reason_code=convergence_mode or "call-contract-convergence-required",
                reason=(
                    "call-contract primary work is governed by the phase-wide "
                    f"convergence scheduler ({convergence_mode})"
                ),
                required_parent_action=(
                    "prepare-live-convergence"
                    if convergence_mode == "converging/no-current-generation"
                    else (
                        "claim-convergence-repairs"
                        if convergence_mode == "repairing/failed-targets"
                        else (
                                "advance-live-call-contract"
                                if convergence_mode == "convergence-clean"
                                else (
                                "claim-retail-fact-packets"
                                if convergence_mode == "retail-blocked"
                                else (
                                    "resolve-dependent-owner-ambiguity"
                                    if convergence_mode
                                    == "dependent-owner-blocked"
                                    else (
                                        "resolve-dependent-header-routing"
                                        if convergence_mode
                                        == "dependent-header-blocked"
                                        else "repair-call-contract-readiness"
                                    )
                                )
                            )
                        )
                    )
                ),
            )
        elif primary_lane == "authored-byte":
            specs.append(("primary", "authored", primary_cursor, primary_phase))
        elif primary_lane == "linked-byte":
            specs.append(("primary", "linked", primary_cursor, primary_phase))
        else:
            result["primary"] = _blocked_opportunity(
                "primary",
                cursor=primary_cursor,
                state="blocked",
                reason_code="primary-worker-mode-unavailable",
                reason=f"no compact source-worker packet exists for primary lane {primary_lane}",
                required_parent_action="run-parent-validation",
            )

    authored_cursor = str(pipeline.get("parallel_authored_byte_cursor", ""))
    if "authored" in requested_lanes:
        if authored_cursor:
            specs.append(("authored", "authored", authored_cursor, "authored-byte-match"))
        else:
            authored_lane = pipeline.get("authored_byte_lane", {})
            authored_state = str(authored_lane.get("state", "blocked")) if isinstance(authored_lane, Mapping) else "blocked"
            result["authored"] = _blocked_opportunity(
                "authored",
                cursor=str(pipeline.get("authored_byte_cursor", "")),
                state=authored_state,
                reason_code=authored_state,
                reason=str(authored_lane.get("blocked_reason", "")) if isinstance(authored_lane, Mapping) else "",
                required_parent_action="advance-primary-prefix",
            )

    object_cursor = str(pipeline.get("parallel_authored_object_byte_cursor", ""))
    if "object" in requested_lanes:
        if object_cursor:
            specs.append(("object", "object", object_cursor, "authored-byte-match"))
        else:
            object_lane = pipeline.get("authored_object_byte_lane", {})
            object_state = str(object_lane.get("state", "blocked")) if isinstance(object_lane, Mapping) else "blocked"
            result["object"] = _blocked_opportunity(
                "object",
                cursor=str(object_lane.get("cursor", "")) if isinstance(object_lane, Mapping) else "",
                state=object_state,
                reason_code=object_state,
                reason=str(object_lane.get("blocked_reason", "")) if isinstance(object_lane, Mapping) else "",
                required_parent_action="advance-primary-prefix",
            )

    for packet_lane, verifier_lane, cursor, phase in specs:
        active = _active_matching_work(
            document, packet_lane=packet_lane, phase=phase, cursor=cursor
        )
        if active is not None:
            work_id, _work = active
            result[packet_lane] = {
                **_blocked_opportunity(
                    packet_lane,
                    cursor=cursor,
                    state="active",
                    reason_code="packet-already-active",
                    reason=f"active packet {work_id} already owns this lane cursor",
                    required_parent_action="handoff-active-packet",
                ),
                "work_item_id": work_id,
            }
            continue
        try:
            preflight: Mapping[str, Any] = {
                "passed": True,
                "reason_code": "order-target-ready",
                "reason": "registered order target is ready",
            }
            if verifier_lane == "order":
                work_id, work = _order_claim_candidate(document, pipeline)
            elif verifier_lane == "call-contract":
                preflight = {
                    "passed": True,
                    "reason_code": "call-contract-slice-ready",
                    "reason": "deterministic authored call-contract slice is ready",
                }
                work_id, work = _call_contract_claim_candidate(
                    document,
                    pipeline,
                    progress_path=progress_path,
                )
            else:
                preflight = _byte_lane_preflight(
                    document,
                    lane=verifier_lane,
                    cursor=cursor,
                    authored_order_prefix_end=(
                        str(pipeline.get("authored_order_prefix_end"))
                        if isinstance(
                            pipeline.get("authored_order_prefix_end"), str
                        )
                        else None
                    ),
                )
                if preflight.get("passed") is not True:
                    result[packet_lane] = _blocked_opportunity(
                        packet_lane,
                        cursor=cursor,
                        state="blocked",
                        reason_code=str(preflight.get("reason_code", "byte-preflight-blocked")),
                        reason=str(preflight.get("reason", "byte preflight blocked")),
                        required_parent_action="repair-byte-expected-facts",
                    )
                    result[packet_lane]["preflight"] = deepcopy(dict(preflight))
                    continue
                work_id, work = _byte_claim_candidate(
                    document,
                    packet_lane=packet_lane,
                    verifier_lane=verifier_lane,
                    cursor=cursor,
                    phase=phase,
                    progress_path=progress_path,
                    preflight=preflight,
                )
            conflicts, incomplete = _candidate_active_conflicts(
                document,
                work_id=work_id,
                work=work,
                issue_ledger=issue_ledger,
            )
            claims, _complete, _source = work_resource_claims(work)
            if incomplete or conflicts:
                reason = (
                    "cannot prove non-overlap with " + ", ".join(incomplete)
                    if incomplete
                    else f"candidate conflicts with {len(conflicts)} active resource claim(s)"
                )
                result[packet_lane] = {
                    **_blocked_opportunity(
                        packet_lane,
                        cursor=cursor,
                        state="blocked",
                        reason_code="active-resource-conflict",
                        reason=reason,
                        required_parent_action="wait-or-close-conflicting-packet",
                    ),
                    "resource_claims": claims,
                    "conflicts": conflicts,
                }
                continue
            result[packet_lane] = {
                "lane": packet_lane,
                "byte_lane": (
                    verifier_lane
                    if verifier_lane not in {"order", "call-contract"}
                    else ""
                ),
                "cursor": cursor,
                "state": "launchable",
                "launchability": "launchable",
                "reason_code": str(preflight.get("reason_code", "live-validation-ready")),
                "reason": str(preflight.get("reason", "live validation ready")),
                "required_parent_action": "claim-and-launch",
                "prerequisite_state": "satisfied",
                "resource_claims": claims,
                "conflicts": [],
                "work_item_id": work_id,
                "packet": work,
            }
        except (OSError, RuntimeError, ValueError) as exc:
            result[packet_lane] = _blocked_opportunity(
                packet_lane,
                cursor=cursor,
                state="blocked",
                reason_code=(
                    str(pipeline.get("order_target_resolution", {}).get("reason_code", ""))
                    if verifier_lane == "order"
                    else "packet-construction-blocked"
                )
                or "packet-construction-blocked",
                reason=str(exc),
                required_parent_action=(
                    (
                        "repair-order-target"
                        if verifier_lane == "order"
                        else (
                            "repair-call-contract-slice"
                            if verifier_lane == "call-contract"
                            else "repair-byte-packet-inputs"
                        )
                    )
                ),
            )
    return result


def _select_compatible_opportunities(
    opportunities: Mapping[str, Mapping[str, Any]],
    *,
    max_packets: int,
) -> tuple[list[tuple[str, str, dict[str, Any]]], list[dict[str, Any]]]:
    if max_packets < 1:
        raise ProgressError("--max-packets must be at least 1")
    selected: list[tuple[str, str, dict[str, Any]]] = []
    skipped: list[dict[str, Any]] = []
    for lane in ("primary", "authored", "object"):
        opportunity = opportunities.get(lane, {})
        packet = opportunity.get("packet") if isinstance(opportunity, Mapping) else None
        if opportunity.get("launchability") != "launchable" or not isinstance(packet, dict):
            continue
        work_id = str(opportunity.get("work_item_id", ""))
        if len(selected) >= max_packets:
            skipped.append(
                {"lane": lane, "cursor": opportunity.get("cursor", ""), "reason_code": "capacity"}
            )
            continue
        claims, complete, _source = work_resource_claims(packet)
        if not complete:
            skipped.append(
                {"lane": lane, "cursor": opportunity.get("cursor", ""), "reason_code": "incomplete-resource-claims"}
            )
            continue
        conflicts: list[dict[str, Any]] = []
        for selected_lane, selected_id, selected_packet in selected:
            selected_claims, selected_complete, _ = work_resource_claims(selected_packet)
            if not selected_complete:
                conflicts.append({"kind": "incomplete-selected-claims", "lane": selected_lane})
                continue
            conflicts.extend(resource_claim_conflicts(claims, selected_id, selected_claims))
        if conflicts:
            skipped.append(
                {
                    "lane": lane,
                    "cursor": opportunity.get("cursor", ""),
                    "reason_code": "higher-priority-resource-conflict",
                    "conflicts": conflicts,
                }
            )
            continue
        selected.append((lane, work_id, packet))
    return selected, skipped


def describe_current_claim_opportunities(
    document: ProgressDocument,
    pipeline: Mapping[str, Any],
    *,
    issue_ledger: str | Path,
) -> dict[str, Any]:
    progress_path = document.path or DEFAULT_PROGRESS
    issue_ledger_path = Path(issue_ledger)
    call_contract_primary = (
        pipeline.get("phase") == "authored-call-contract"
        and pipeline.get("primary_lane") == "call-contract"
    )
    opportunities = _current_claim_opportunities(
        document,
        pipeline,
        progress_path=progress_path,
        issue_ledger=issue_ledger_path,
    )
    if call_contract_primary:
        try:
            repair_continuation = continuation_state(document)
        except AttributeError:
            repair_continuation = {
                "state": "none", "active": False, "checkpoint": None
            }
    else:
        # A target revalidation deliberately regresses the primary scheduler
        # to authored order before the old call-contract census is derivable
        # again.  Do not consult or project continuation/convergence state
        # from the later phase while describing that order frontier.
        repair_continuation = {
            "state": "none", "active": False, "checkpoint": None
        }
    continuation_blocks_primary = _continuation_blocks_primary_scheduler(
        repair_continuation
    )
    repair_candidates: list[tuple[str, dict[str, Any]]] = []
    repair_blockers: list[dict[str, Any]] = []
    if not continuation_blocks_primary and call_contract_primary:
        repair_candidates, repair_blockers = (
            _call_contract_mixed_obligation_candidates(
                document,
                progress_path=progress_path,
                issue_ledger=issue_ledger_path,
            )
        )
        if not repair_candidates and not repair_blockers:
            repair_candidates, repair_blockers = (
                _call_contract_convergence_repair_candidates(
                    document,
                    progress_path=progress_path,
                    issue_ledger=issue_ledger_path,
                )
            )
    retail_fact_candidates: list[tuple[str, dict[str, Any]]] = []
    retail_fact_blockers: list[dict[str, Any]] = []
    if (
        call_contract_primary
        and not repair_candidates
        and not continuation_blocks_primary
    ):
        retail_fact_candidates, retail_fact_blockers = (
            _call_contract_retail_fact_candidates(
                document,
                issue_ledger=issue_ledger_path,
            )
        )
    primary_candidates = repair_candidates or retail_fact_candidates
    if continuation_blocks_primary:
        checkpoint = repair_continuation.get("checkpoint")
        child = (
            checkpoint.get("child")
            if isinstance(checkpoint, Mapping)
            and isinstance(checkpoint.get("child"), Mapping)
            else {}
        )
        state = str(repair_continuation.get("state", ""))
        opportunities["primary"] = {
            "lane": "primary",
            "cursor": "",
            "state": "blocked",
            "launchability": "blocked",
            "reason_code": (
                "repair-continuation-child-active"
                if state == "child-active"
                else "repair-continuation-full-convergence-required"
            ),
            "reason": (
                "the exact one-hop repair continuation is already active"
                if state == "child-active"
                else "a fresh full convergence generation is required before normal call-contract scheduling"
            ),
            "required_parent_action": (
                "await-continuation-return"
                if state == "child-active"
                else "prepare-live-convergence"
            ),
            "prerequisite_state": "missing",
            "resource_claims": [],
            "conflicts": [],
            "work_item_id": str(child.get("work_item_id", "")),
            "repair_packet_count": 0,
            "retail_fact_packet_count": 0,
            "repair_blockers": [],
            "continuation_checkpoint_id": (
                str(checkpoint.get("checkpoint_id", ""))
                if isinstance(checkpoint, Mapping)
                else ""
            ),
        }
    elif primary_candidates:
        work_id, work = primary_candidates[0]
        claims, _complete, _source = work_resource_claims(work)
        retail_fact = work.get("packet_type") == RETAIL_FACT_PACKET_TYPE
        opportunities["primary"] = {
            "lane": "primary",
            "cursor": work.get("cursor", ""),
            "state": "launchable",
            "launchability": "launchable",
            "reason_code": (
                "call-contract-retail-facts-ready"
                if retail_fact
                else "call-contract-convergence-repairs-ready"
            ),
            "reason": (
                f"{len(primary_candidates)} compatible "
                + (
                    "read-only retail fact packet(s) are ready"
                    if retail_fact
                    else "convergence repair packet(s) are ready"
                )
            ),
            "required_parent_action": "claim-and-launch",
            "prerequisite_state": "satisfied",
            "resource_claims": claims,
            "conflicts": [],
            "work_item_id": work_id,
            "packet": work,
            "repair_packet_count": len(repair_candidates),
            "retail_fact_packet_count": len(retail_fact_candidates),
            "repair_blockers": [
                *repair_blockers,
                *retail_fact_blockers,
            ],
        }
    elif repair_blockers or retail_fact_blockers:
        opportunities["primary"] = _blocked_convergence_repair_opportunity(
            cursor=str(pipeline.get("cursor", "")),
            repair_blockers=repair_blockers,
            retail_fact_blockers=retail_fact_blockers,
        )
    selected, skipped = _select_compatible_opportunities(opportunities, max_packets=3)
    selected_lanes = {lane for lane, _work_id, _packet in selected}
    skipped_by_lane = {str(item["lane"]): item for item in skipped}
    launch_plan: list[dict[str, Any]] = []
    cursor_keys = {
        "primary": "primary",
        "authored": "parallel_authored_byte",
        "object": "parallel_authored_object_byte",
    }
    launchability: dict[str, Any] = {}
    scheduler_revision = _scheduler_revision_for_document(document)
    for order, lane in enumerate(("primary", "authored", "object"), 1):
        opportunity = deepcopy(dict(opportunities[lane]))
        opportunity.pop("packet", None)
        preflight = opportunity.get("preflight")
        if isinstance(preflight, dict):
            preflight.pop("report", None)
        launchability[cursor_keys[lane]] = opportunity
        selected_opportunity = lane in selected_lanes
        skip = skipped_by_lane.get(lane)
        claim_command = (
            "python tools/recoil.py progress work claim-current "
            f"--lane {lane} --expected-scheduler-revision {scheduler_revision} --apply --json"
        )
        launch_plan.append(
            {
                "order": order,
                "lane": lane,
                "phase": str(pipeline.get("phase", "")),
                "cursor": opportunity.get("cursor", ""),
                "launchability": opportunity.get("launchability", "blocked"),
                "reason_code": (
                    skip.get("reason_code") if skip is not None else opportunity.get("reason_code", "")
                ),
                "prerequisite_state": opportunity.get("prerequisite_state", "missing"),
                "selected_opportunity": selected_opportunity,
                "actions": (
                    [{"kind": "claim-current", "command": claim_command}]
                    if selected_opportunity
                    else []
                ),
            }
        )
    blockers = [
        {
            "lane": lane,
            "cursor": opportunity.get("cursor", ""),
            "reason_code": opportunity.get("reason_code", "blocked"),
            "reason": opportunity.get("reason", ""),
        }
        for lane, opportunity in opportunities.items()
        if opportunity.get("launchability") not in {"launchable", "complete", "caught-up"}
    ]
    return {
        "cursor_launchability": launchability,
        "launch_plan": launch_plan,
        "launch_blockers": blockers,
        "launch_skipped": skipped,
        "claim_all_command": (
            "python tools/recoil.py progress work claim-current --lane all --max-packets 3 "
            f"--expected-scheduler-revision {scheduler_revision} --apply --json"
        ),
    }


def claim_current_work(args: argparse.Namespace) -> dict[str, Any]:
    max_packets = int(getattr(args, "max_packets", 3))
    issue_ledger = Path(
        getattr(args, "issue_ledger", DEFAULT_ISSUE_LEDGER)
    )
    if max_packets < 1:
        raise ProgressError("--max-packets must be at least 1")
    scheduler_document, scheduler_domains = _precheck_scheduler_revision(args)
    details: dict[str, Any] = {}
    allocated_worktrees: list[dict[str, Any]] = []

    def is_authenticated_canonical_progress_authority(path: Path) -> bool:
        """Recognize only the live canonical SQLite authority for allocation.

        Existing fixture or synthetic SQLite files remain valid command inputs,
        but they cannot authorize real branches, linked worktrees, or external
        build roots.  The canonical resolver authenticates both the control
        root and the exact machine-local progress file, and reauthentication
        closes the identity window immediately before topology mutation.
        """

        if path.suffix.casefold() != ".sqlite3" or not path.is_file():
            return False
        try:
            resolution = resolve_canonical_control_root(
                executing_worktree_root=REPO_ROOT,
                required_machine_local_paths=(
                    PROGRESS_AUTHORITY_RELATIVE_PATH,
                ),
            )
            canonical_authority = (
                resolution.canonical_control_root
                / PROGRESS_AUTHORITY_RELATIVE_PATH
            ).resolve(strict=True)
            supplied_authority = path.resolve(strict=True)
            if supplied_authority != canonical_authority:
                return False
            reauthenticate_canonical_control_root(resolution)
        except (OSError, WorktreeControlError):
            return False
        return True

    def allocate_native_git(
        data: dict[str, Any], document: ProgressDocument, work_id: str
    ) -> dict[str, Any]:
        work = data["work_items"][work_id]
        writable = progress_packet_tracked_write_paths(work)
        if not writable:
            work["progress_packet_adapter"] = "branchless-generated-output-v1"
            return {"adapter": "branchless-generated-output-v1"}
        progress_authority = Path(args.progress)
        if not is_authenticated_canonical_progress_authority(progress_authority):
            # Legacy in-memory/JSON unit stores are not runtime authorities and
            # missing-path/arbitrary-SQLite command doubles must never allocate
            # real Git topology. Their planned projection remains
            # non-handoff-visible; production has one authenticated canonical
            # SQLite authority since cutover.
            work["progress_packet_adapter"] = "native-git-v1-planned"
            return {
                "adapter": "native-git-v1",
                "planned": True,
                "runtime_authority": False,
                "writable_paths": writable,
            }
        if not args.apply:
            work["progress_packet_adapter"] = "native-git-v1-planned"
            return {"adapter": "native-git-v1", "planned": True, "writable_paths": writable}
        topology = resolve_topology(REPO_ROOT)
        masters = [row for row in topology.worktrees if row.branch == "master"]
        if len(masters) != 1 or masters[0].root != topology.integration_root:
            raise ProgressError("progress packet allocation requires one canonical master worktree")
        require_clean_worktree(topology.integration_root)
        branch, packet_root, external_root = derive_packet_locations(
            topology,
            authority="progress",
            packet_id=work_id,
            revision=document.revision + 1,
        )
        association = WorktreeAssociation("progress", work_id, str(external_root))
        created_worktree = False
        created_build = False
        try:
            create_linked_worktree(
                topology,
                branch=branch,
                worktree_root=packet_root,
                start_point="master",
                association=association,
            )
            created_worktree = True
            marker = create_build_root(
                external_root,
                authority="progress",
                packet_id=work_id,
                branch=branch,
                worktree_root=packet_root,
            )
            created_build = True
            baseline = capture_clean_git_baseline(
                packet_root, packet_id=work_id, writable_paths=writable
            )
            claims, complete, _source = work_resource_claims(work)
            if not complete:
                raise ProgressError("progress packet claims changed during allocation")
            old_roots = [
                row["id"] for row in claims
                if row["kind"] == "output-root" and row["access"] == "write"
            ]
            claims = [
                row for row in claims
                if not (row["kind"] == "output-root" and row["access"] == "write")
            ]
            claims.append({"kind": "output-root", "id": str(external_root), "access": "write"})
            work["resource_claims"] = normalize_resource_claims(claims)
            reservation = work.get("reservation")
            if isinstance(reservation, dict):
                reservation["resource_claims"] = deepcopy(work["resource_claims"])
            commands = list(work.get("validation_commands", []))
            if len(commands) != 1:
                raise ProgressError("progress packet requires exactly one validation command")
            command = commands[0]
            command = command.replace("{packet_build_root}", _command_arg(str(external_root)))
            command = _bind_native_git_progress_authority(
                command,
                Path(args.progress),
            )
            for old_root in old_roots:
                command = command.replace(old_root, _command_arg(str(external_root)))
            work["validation_commands"] = [command]
            operation_id = f"{work_id}:allocation:r{document.revision + 1}"
            binding = bind_progress_packet_native_git(
                data,
                work_id=work_id,
                baseline=baseline,
                association=association.to_dict(),
                build_root_marker=marker,
                operation_id=operation_id,
            )
            allocated = {
                "topology": topology,
                "branch": branch,
                "packet_root": packet_root,
                "external_root": external_root,
                "work_id": work_id,
                "binding": binding,
            }
            allocated_worktrees.append(allocated)
            return binding
        except Exception:
            if created_build and external_root.exists():
                remove_authenticated_build_root(
                    external_root, authority="progress", packet_id=work_id
                )
            if created_worktree and packet_root.exists():
                remove_linked_worktree(
                    topology.integration_root,
                    worktree_root=packet_root,
                    branch=branch,
                )
            raise

    def transform(data: dict[str, Any]) -> None:
        document = ProgressDocument(data, path=args.progress)
        migration = data.get("migration", {})
        carry_candidate = (
            migration.get(CONVERGENCE_MIGRATION_KEY)
            if isinstance(migration, Mapping)
            else None
        )
        semantic_projection_before = (
            _normalized_semantic_projection(document)
            if scheduler_domains is None
            and isinstance(carry_candidate, Mapping)
            and carry_candidate.get("contract_version")
            == CONVERGENCE_CONTRACT_VERSION
            else None
        )
        pipeline = document.pipeline("recoil")
        requested_lanes = (
            ("primary", "authored", "object")
            if args.lane == "all"
            else (str(args.lane),)
        )
        continuation = continuation_state(document)
        continuation_child = (
            _call_contract_continuation_child_candidate(document)
            if "primary" in requested_lanes
            else None
        )
        continuation_blocks_primary = (
            _continuation_blocks_primary_scheduler(continuation)
            and continuation_child is None
        )
        repair_candidates: list[tuple[str, dict[str, Any]]] = []
        repair_blockers: list[dict[str, Any]] = []
        if continuation_child is not None:
            repair_candidates = [continuation_child]
        elif "primary" in requested_lanes and not continuation_blocks_primary:
            repair_candidates, repair_blockers = (
                _call_contract_continuation_producer_candidates(
                    document,
                    progress_path=Path(args.progress),
                    issue_ledger=issue_ledger,
                )
            )
        if (
            "primary" in requested_lanes
            and not continuation_blocks_primary
            and not repair_candidates
            and not repair_blockers
        ):
            repair_candidates, repair_blockers = (
                _call_contract_mixed_obligation_candidates(
                    document,
                    progress_path=args.progress,
                    issue_ledger=issue_ledger,
                )
            )
            if not repair_candidates and not repair_blockers:
                repair_candidates, repair_blockers = (
                    _call_contract_convergence_repair_candidates(
                        document,
                        progress_path=args.progress,
                        issue_ledger=issue_ledger,
                    )
                )
        retail_fact_candidates: list[tuple[str, dict[str, Any]]] = []
        retail_fact_blockers: list[dict[str, Any]] = []
        if (
            "primary" in requested_lanes
            and not repair_candidates
            and not continuation_blocks_primary
        ):
            retail_fact_candidates, retail_fact_blockers = (
                _call_contract_retail_fact_candidates(
                    document,
                    issue_ledger=issue_ledger,
                )
            )
        primary_candidates = repair_candidates or retail_fact_candidates
        selected_repairs: list[tuple[str, str, dict[str, Any]]] = []
        repair_skipped: list[dict[str, Any]] = []
        for work_id, work in primary_candidates:
            if len(selected_repairs) >= max_packets:
                repair_skipped.append(
                    {
                        "lane": "primary",
                        "target_id": work.get("target_id", ""),
                        "reason_code": "capacity",
                    }
                )
                continue
            claims, complete, _source = work_resource_claims(work)
            if not complete:
                repair_skipped.append(
                    {
                        "lane": "primary",
                        "target_id": work.get("target_id", ""),
                        "reason_code": "incomplete-resource-claims",
                    }
                )
                continue
            conflicts = [
                conflict
                for _lane, selected_id, selected_work in selected_repairs
                for selected_claims, selected_complete, _ in [
                    work_resource_claims(selected_work)
                ]
                for conflict in (
                    resource_claim_conflicts(
                        claims,
                        selected_id,
                        selected_claims,
                    )
                    if selected_complete
                    else [{"kind": "incomplete-selected-claims"}]
                )
            ]
            if conflicts:
                repair_skipped.append(
                    {
                        "lane": "primary",
                        "target_id": work.get("target_id", ""),
                        "reason_code": "higher-priority-resource-conflict",
                        "conflicts": conflicts,
                    }
                )
                continue
            selected_repairs.append(("primary", work_id, work))

        opportunities = _current_claim_opportunities(
            document,
            pipeline,
            progress_path=args.progress,
            issue_ledger=issue_ledger,
            lanes=requested_lanes,
        )
        if (
            "primary" in requested_lanes
            and not continuation_blocks_primary
            and not primary_candidates
            and (repair_blockers or retail_fact_blockers)
        ):
            opportunities["primary"] = (
                _blocked_convergence_repair_opportunity(
                    cursor=str(pipeline.get("cursor", "")),
                    repair_blockers=repair_blockers,
                    retail_fact_blockers=retail_fact_blockers,
                )
            )
        if "primary" in requested_lanes and continuation_blocks_primary:
            checkpoint = continuation.get("checkpoint")
            opportunities["primary"] = {
                "lane": "primary",
                "cursor": "",
                "launchability": "blocked",
                "reason_code": "repair-continuation-active",
                "reason": (
                    "repair continuation blocks ordinary primary scheduling "
                    "until its child returns and full convergence closes out"
                ),
                "conflicts": [],
            }
        blockers = [
            {
                "lane": lane,
                "cursor": opportunity.get("cursor", ""),
                "reason_code": opportunity.get("reason_code", "blocked"),
                "reason": opportunity.get("reason", ""),
                "conflicts": opportunity.get("conflicts", []),
            }
            for lane, opportunity in opportunities.items()
            if opportunity.get("launchability") not in {"launchable", "complete", "caught-up"}
        ]
        if selected_repairs:
            selected = selected_repairs
            skipped = repair_skipped
            blockers.extend(repair_blockers)
            blockers.extend(retail_fact_blockers)
        elif args.lane == "all":
            selected, skipped = _select_compatible_opportunities(
                opportunities, max_packets=max_packets
            )
            blockers.extend(repair_blockers)
            blockers.extend(retail_fact_blockers)
        else:
            opportunity = opportunities[args.lane]
            packet = opportunity.get("packet")
            if opportunity.get("launchability") != "launchable" or not isinstance(packet, dict):
                raise ProgressError(
                    f"{args.lane} lane is not launchable: "
                    f"{opportunity.get('reason_code')}: {opportunity.get('reason')}"
                )
            selected = [(args.lane, str(opportunity["work_item_id"]), packet)]
            skipped = []
        if not selected:
            raise ProgressError(
                "no compatible current packets are launchable: "
                + json.dumps(blockers, sort_keys=True)
            )
        unauthorized_lanes = sorted(
            lane for lane, _work_id, _work in selected if lane not in requested_lanes
        )
        if unauthorized_lanes:
            raise ProgressError(
                "claim selection included lane(s) outside the explicit request: "
                + ", ".join(unauthorized_lanes)
            )
        work_items = data.get("work_items")
        if not isinstance(work_items, dict):
            raise ProgressError("progress work_items collection must be an object")
        packets: list[dict[str, Any]] = []
        reservations: list[dict[str, Any]] = []
        for selected_lane, work_id, work in selected:
            if work.get("packet_type") == CONTINUATION_PACKET_TYPE:
                provenance = work.get("continuation_provenance", {})
                reserved = create_and_reserve_repair_continuation_work_item(
                    data,
                    work_id=work_id,
                    work=work,
                    predecessor_work_item_id=str(
                        provenance.get("predecessor_work_item_id", "")
                    ),
                    checkpoint_id=str(provenance.get("checkpoint_id", "")),
                )
                migration = data.get("migration", {})
                checkpoint = migration.get(CONTINUATION_MIGRATION_KEY)
                migration[CONTINUATION_MIGRATION_KEY] = activate_continuation_child(
                    checkpoint, child_work_item_id=work_id
                )
            else:
                reserved = create_and_reserve_claim_current_work_item(
                    data,
                    work_id=work_id,
                    work=work,
                    command=CLAIM_CURRENT_COMMAND,
                    requested_lane=str(args.lane),
                    selected_lane=selected_lane,
                    max_packets=max_packets,
                )
            adapter = allocate_native_git(data, document, work_id)
            reservations.append(reserved)
            if adapter.get("planned") is True:
                planned_work = work_items[work_id]
                packets.append(
                    {
                        "packet_id": work_id,
                        "reservation_id": reserved["reservation"]["id"],
                        "packet_type": str(planned_work.get("packet_type", "")),
                        "lane": str(planned_work.get("lane", "")),
                        "claim_provenance": deepcopy(
                            planned_work.get("claim_provenance")
                        ),
                        "progress_packet_adapter": adapter,
                        "planned": True,
                        "handoff_visible": False,
                    }
                )
            else:
                packets.append(
                    _compact_reserved_packet(work_id, work_items[work_id])
                )
                packets[-1]["progress_packet_adapter"] = adapter
        generation_carried = (
            scheduler_domains is None
            and semantic_projection_before is not None
        ) and (
            carry_current_generation_across_work_ledger_mutation(
                data,
                expected_revision=document.revision,
                semantic_projection_before=semantic_projection_before,
            )
        )
        details.update(
            {
                "packets": packets,
                "reservations": reservations,
                "blockers": blockers,
                "skipped": skipped,
                "convergence_generation_carried": generation_carried,
            }
        )
        if len(packets) == 1:
            details.update(
                {
                    "packet": packets[0],
                    "work_item_id": packets[0]["packet_id"],
                    "reservation_id": packets[0]["reservation_id"],
                }
            )

    try:
        commit = (
            _call_contract_scoped_patch_commit(
                args=args,
                document=scheduler_document,
                transform=transform,
                expected_domains=scheduler_domains,
                increment_domains={"scheduler"},
            )
            if scheduler_domains is not None
            else ProgressStore(args.progress).mutate(
                transform,
                expected_revision=args.expected_revision,
                apply=args.apply,
            )
        )
    except Exception:
        cleanup_errors: list[str] = []
        for allocation in reversed(allocated_worktrees):
            try:
                remove_authenticated_build_root(
                    allocation["external_root"],
                    authority="progress",
                    packet_id=allocation["work_id"],
                )
                remove_linked_worktree(
                    allocation["topology"].integration_root,
                    worktree_root=allocation["packet_root"],
                    branch=allocation["branch"],
                )
            except Exception as exc:
                cleanup_errors.append(str(exc))
        if cleanup_errors:
            raise ProgressError(
                "progress packet allocation rollback left cleanup debt: "
                + "; ".join(cleanup_errors)
            )
        raise
    return _commit_payload(commit, details)


def _load_explicit_maintenance_payload(args: argparse.Namespace) -> dict[str, Any]:
    if getattr(args, "payload_file", None) is None:
        source = str(args.payload_json)
    else:
        payload_path = Path(args.payload_file)
        resolved = (
            payload_path.resolve()
            if payload_path.is_absolute()
            else (REPO_ROOT / payload_path).resolve()
        )
        try:
            resolved.relative_to((REPO_ROOT / "build").resolve())
        except ValueError as exc:
            raise ProgressError(
                "explicit maintenance --payload-file must resolve under workspace build/"
            ) from exc
        try:
            if not resolved.is_file() or resolved.stat().st_size > MAX_PROGRESS_PAYLOAD_FILE_BYTES:
                raise ProgressError("explicit maintenance payload file is absent or too large")
            source = resolved.read_text(encoding="utf-8")
        except OSError as exc:
            raise ProgressError(f"cannot read explicit maintenance payload file: {exc}") from exc
    try:
        payload = json.loads(source)
    except json.JSONDecodeError as exc:
        raise ProgressError(f"explicit maintenance payload is invalid JSON: {exc}") from exc
    if not isinstance(payload, dict):
        raise ProgressError("explicit maintenance payload must be a JSON object")
    return payload


def create_explicit_maintenance_work(args: argparse.Namespace) -> dict[str, Any]:
    from _recoil.commands.workspace_issues import (
        cross_ledger_reservation_critical_section,
        workspace_issue_reservation_conflicts,
    )

    payload = _load_explicit_maintenance_payload(args)
    issue_ledger = Path(args.issue_ledger)
    details: dict[str, Any] = {}
    # A preview has no allocation operation and therefore no nonce.  The
    # unpredictable nonce is created only inside the applying path.
    operation_nonce = secrets.token_hex(32) if args.apply else None
    lock = (
        cross_ledger_reservation_critical_section(Path(args.progress), issue_ledger)
        if args.apply
        else nullcontext()
    )
    with lock:
        try:
            issue_metadata = read_issue_metadata(issue_ledger)
        except Exception as exc:
            raise ProgressError(
                f"cannot bind explicit packet issue-ledger identity: {exc}"
            ) from exc
        issue_identity = {
            "path": str(issue_ledger.resolve()),
            "application_id": issue_metadata.application_id,
            "user_version": issue_metadata.user_version,
            "schema_version": issue_metadata.schema_version,
            "ledger_version": issue_metadata.ledger_version,
            "cutover_pair_id": issue_metadata.cutover_pair_id,
        }
        scheduler_document, scheduler_domains = _precheck_scheduler_revision(args)
        if scheduler_domains is not None:
            expected_semantic = getattr(args, "expected_semantic_revision", None)
            if expected_semantic is None:
                raise ProgressError(
                    "revision-domain explicit creation requires --expected-semantic-revision"
                )
            vector = ProgressSQLiteStore(Path(args.progress)).read_revision_vector()
            if vector.semantic_revision != int(expected_semantic):
                raise ConcurrentProgressUpdate(
                    "semantic revision changed while deriving explicit maintenance closure: "
                    f"expected {expected_semantic}, found {vector.semantic_revision}"
                )
            scheduler_domains = {
                **scheduler_domains,
                "semantic": int(expected_semantic),
            }

        def transform(data: dict[str, Any]) -> None:
            document = ProgressDocument(data, path=args.progress)
            work_id, candidate = construct_explicit_maintenance_work_item(
                document, payload
            )
            claims, complete, _source = work_resource_claims(candidate)
            if not complete:
                raise ProgressError("explicit maintenance candidate has incomplete claims")
            try:
                issue_conflicts = workspace_issue_reservation_conflicts(
                    issue_ledger, work_id, claims
                )
            except ValueError as exc:
                raise ProgressError(str(exc)) from exc
            if issue_conflicts:
                raise ProgressError(
                    "explicit maintenance packet conflicts with an active workspace-issue "
                    "reservation: " + json.dumps(issue_conflicts, sort_keys=True)
                )
            if args.apply:
                intent = create_and_reserve_explicit_maintenance_work_item(
                    data,
                    payload,
                    progress_path=args.progress,
                    operation_nonce=str(operation_nonce),
                    issue_ledger_identity=issue_identity,
                )
                stored = intent["work_item"]
                output_allocation = deepcopy(
                    stored["explicit_provenance"]["output_allocation"]
                )
                reservation_id = str(intent["reservation_id"])
            else:
                stored = deepcopy(candidate)
                stored["state"] = "allocation-preview"
                stored["allocation_state"] = "allocation-preview"
                stored["allocation_intent"] = {
                    "schema": "recoil-explicit-output-allocation-intent-preview-v1",
                    "reservation_id": f"{work_id}:attempt:1",
                    "allocation_nonce": None,
                    "allocation_nonce_state": "generated-only-during-apply",
                    "resource_claim_state": "inactive-preview",
                }
                stored["explicit_provenance"]["output_allocation"] = {
                    "schema": "recoil-explicit-output-allocation-preview-v1",
                    "packet_id": work_id,
                    "reservation_id": f"{work_id}:attempt:1",
                    "normalized_output_root": stored["explicit_provenance"][
                        "closure"
                    ]["output_root"],
                    "allocation_nonce": None,
                    "allocation_nonce_state": "generated-only-during-apply",
                }
                data["work_items"][work_id] = stored
                output_allocation = deepcopy(
                    stored["explicit_provenance"]["output_allocation"]
                )
                reservation_id = f"{work_id}:attempt:1"
            stored["explicit_provenance"]["issue_ledger_identity"] = deepcopy(issue_identity)
            details.update(
                {
                    "work_item_id": work_id,
                    "reservation_id": reservation_id,
                    "closure": deepcopy(stored["explicit_provenance"]["closure"]),
                    "resource_claims": deepcopy(stored["resource_claims"]),
                    "output_allocation": output_allocation,
                    "issue_ledger": display_path(issue_ledger),
                    "nonaccepting": True,
                    "acceptance_changed": False,
                    "build_root_created": False,
                    "allocation_state": (
                        "allocating" if args.apply else "allocation-preview"
                    ),
                }
            )

        allocation_commit = (
            _call_contract_scoped_patch_commit(
                args=args,
                document=scheduler_document,
                transform=transform,
                expected_domains=scheduler_domains,
                increment_domains={"scheduler"},
            )
            if scheduler_domains is not None
            else ProgressStore(args.progress).mutate(
                transform,
                expected_revision=args.expected_revision,
                apply=args.apply,
            )
        )
    if not args.apply:
        return _commit_payload(allocation_commit, details)

    work_id = str(details["work_item_id"])
    allocation = deepcopy(details["output_allocation"])
    output_root = REPO_ROOT / str(allocation["normalized_output_root"])
    marker_path = output_root / EXPLICIT_OUTPUT_MARKER_NAME
    sidecar_path = REPO_ROOT / str(allocation["ownership_sidecar"])
    created_root = False
    created_sidecar = False
    activation_completed = False

    def _scheduler_followup(
        transform_followup: Any,
    ) -> Any:
        current_document = ProgressStore(args.progress).load()
        if scheduler_domains is not None:
            vector = ProgressSQLiteStore(Path(args.progress)).read_revision_vector()
            expected_semantic = int(scheduler_domains["semantic"])
            if vector.semantic_revision != expected_semantic:
                raise ConcurrentProgressUpdate(
                    "semantic revision changed during explicit output allocation: "
                    f"expected {expected_semantic}, found {vector.semantic_revision}"
                )
            return _call_contract_scoped_patch_commit(
                args=args,
                document=current_document,
                transform=transform_followup,
                expected_domains={
                    "scheduler": vector.scheduler_revision,
                    "semantic": expected_semantic,
                },
                increment_domains={"scheduler"},
            )
        return ProgressStore(args.progress).mutate(
            transform_followup,
            expected_revision=current_document.revision,
            apply=True,
        )

    def _cleanup_owned_failed_root() -> str:
        """Remove only a sidecar-authenticated pre-activation allocation."""

        if not created_root and not created_sidecar:
            return "absent"
        try:
            if (
                not sidecar_path.is_file()
                or sidecar_path.is_symlink()
                or json.loads(sidecar_path.read_text(encoding="utf-8")) != allocation
            ):
                return "quarantined"
            if not created_root:
                # Exclusive directory creation never succeeded; the path, if
                # present, is not ours. Remove only our independently owned
                # sidecar and never touch the raced/pre-existing directory.
                sidecar_path.unlink()
                return "absent"
            if output_root.exists():
                if not output_root.is_dir() or output_root.is_symlink():
                    return "quarantined"
                entries = list(output_root.iterdir())
                if entries:
                    if entries != [marker_path] or not marker_path.is_file():
                        return "quarantined"
                    observed = json.loads(marker_path.read_text(encoding="utf-8"))
                    authenticate_explicit_output_marker(
                        observed, allocation, output_root
                    )
                    marker_path.unlink()
                output_root.rmdir()
            sidecar_path.unlink()
            return "absent"
        except (OSError, UnicodeError, json.JSONDecodeError):
            # The journal and independently created sidecar remain the recovery
            # authority even when the in-root marker never became durable.
            if not sidecar_path.exists():
                try:
                    with sidecar_path.open("x", encoding="utf-8", newline="\n") as stream:
                        json.dump(allocation, stream, sort_keys=True, separators=(",", ":"))
                        stream.write("\n")
                        stream.flush()
                        os.fsync(stream.fileno())
                except OSError:
                    pass
            return "quarantined"

    try:
        parent = output_root.parent
        build_directory = (REPO_ROOT / "build").resolve()
        try:
            output_root.resolve(strict=False).relative_to(build_directory)
        except (OSError, ValueError) as exc:
            raise ProgressError(
                "explicit maintenance output-root parent escapes build/"
            ) from exc
        parent_attributes = int(
            getattr(parent.lstat(), "st_file_attributes", 0)
        ) if parent.exists() else 0
        if (
            not parent.is_dir()
            or parent.is_symlink()
            or parent_attributes & 0x400
        ):
            raise ProgressError(
                "explicit maintenance output-root parent must already be a real directory"
            )
        if sidecar_path.exists() or sidecar_path.is_symlink():
            raise ProgressError("explicit maintenance ownership sidecar already exists")
        sidecar_stream = sidecar_path.open("x", encoding="utf-8", newline="\n")
        created_sidecar = True
        with sidecar_stream:
            json.dump(allocation, sidecar_stream, sort_keys=True, separators=(",", ":"))
            sidecar_stream.write("\n")
            sidecar_stream.flush()
            os.fsync(sidecar_stream.fileno())
        if json.loads(sidecar_path.read_text(encoding="utf-8")) != allocation:
            raise ProgressError("explicit maintenance ownership sidecar failed authentication")
        output_root.mkdir(mode=0o700, exist_ok=False)
        created_root = True
        marker_record = explicit_output_marker_record(allocation, output_root)
        with marker_path.open("x", encoding="utf-8", newline="\n") as marker_stream:
            json.dump(marker_record, marker_stream, sort_keys=True, separators=(",", ":"))
            marker_stream.write("\n")
            marker_stream.flush()
            os.fsync(marker_stream.fileno())
        allocating_document = ProgressStore(args.progress).load()
        journal_registry = allocating_document.data.get("migration", {}).get(
            EXPLICIT_ALLOCATION_JOURNAL_MIGRATION_KEY, {}
        )
        allocating_journal = (
            journal_registry.get("rows", {}).get(work_id)
            if isinstance(journal_registry, Mapping)
            else None
        )
        allocating_work = (
            allocating_journal.get("candidate_packet")
            if isinstance(allocating_journal, Mapping)
            else None
        )
        if not isinstance(allocating_work, Mapping):
            raise ProgressError("explicit allocation journal disappeared")
        authenticate_explicit_output_root(
            allocating_work,
            progress_path=args.progress,
            require_active=False,
        )

        activation_commit = None
        success_details: dict[str, Any] | None = None

        # Re-enter the cross-ledger critical section and recheck the issue
        # identity/conflicts immediately before the only transaction that
        # creates an active reservation and resource claims.
        with cross_ledger_reservation_critical_section(
            Path(args.progress), issue_ledger
        ):
            current_issue_metadata = read_issue_metadata(issue_ledger)
            if {
                "path": str(issue_ledger.resolve()),
                "application_id": current_issue_metadata.application_id,
                "user_version": current_issue_metadata.user_version,
                "schema_version": current_issue_metadata.schema_version,
                "ledger_version": current_issue_metadata.ledger_version,
                "cutover_pair_id": current_issue_metadata.cutover_pair_id,
            } != issue_identity:
                raise ConcurrentProgressUpdate(
                    "issue-ledger identity changed during explicit output allocation"
                )
            current_document = ProgressStore(args.progress).load()
            current_registry = current_document.data.get("migration", {}).get(
                EXPLICIT_ALLOCATION_JOURNAL_MIGRATION_KEY, {}
            )
            current_journal = (
                current_registry.get("rows", {}).get(work_id)
                if isinstance(current_registry, Mapping)
                else None
            )
            current_candidate = (
                current_journal.get("candidate_packet")
                if isinstance(current_journal, Mapping)
                else None
            )
            if not isinstance(current_candidate, Mapping):
                raise ProgressError("explicit allocation journal disappeared before activation")
            current_claims, complete, _source = work_resource_claims(current_candidate)
            if not complete:
                raise ProgressError("explicit allocation journal claims became incomplete")
            issue_conflicts = workspace_issue_reservation_conflicts(
                issue_ledger, work_id, current_claims
            )
            if issue_conflicts:
                raise ProgressError(
                    "explicit maintenance packet conflicts before activation: "
                    + json.dumps(issue_conflicts, sort_keys=True)
                )

            # Construct the complete success projection and exercise the exact
            # activation transform on a detached copy before the final CAS.
            # Nothing after a successful CAS reauthenticates or compensates the
            # root; later handoff/worker/BN entry performs its own authentication.
            preview_data = deepcopy(current_document.data)
            preview_activation = activate_explicit_maintenance_work_item(
                preview_data,
                work_id,
                progress_path=args.progress,
            )
            preview_work = preview_data["work_items"][work_id]
            success_details = {
                **details,
                **preview_activation,
                "packet": _compact_reserved_packet(
                    work_id, preview_work, progress_path=args.progress
                ),
                "build_root_created": True,
                "allocation_state": "active",
                "activation_is_final_semantic_operation": True,
            }

            def activate_transform(data: dict[str, Any]) -> None:
                observed = activate_explicit_maintenance_work_item(
                    data,
                    work_id,
                    progress_path=args.progress,
                )
                if observed != preview_activation:
                    raise ProgressError(
                        "explicit maintenance activation projection changed before CAS"
                    )

            activation_commit = _scheduler_followup(activate_transform)
            activation_completed = True
    except Exception as exc:
        if activation_completed:
            raise ProgressError(
                "explicit maintenance activation committed; response or lock finalization failed; "
                f"query packet {work_id!r} from authority state"
            ) from exc
        failure_details: dict[str, Any] = {}
        cleanup_state = _cleanup_owned_failed_root()

        def fail_transform(data: dict[str, Any]) -> None:
            failure_details.update(
                fail_explicit_maintenance_allocation(
                    data,
                    work_id,
                    reason=str(exc),
                    cleanup_state=cleanup_state,
                )
            )

        failure_commit = None
        failure_exc: Exception | None = None
        for _attempt in range(4):
            try:
                failure_commit = _scheduler_followup(fail_transform)
                failure_exc = None
                break
            except Exception as current_failure_exc:
                failure_exc = current_failure_exc
        if failure_commit is None:
            raise ProgressError(
                "explicit maintenance allocation failed and terminalization also failed: "
                f"allocation={exc}; terminalization={failure_exc}"
            ) from failure_exc
        failure_details["cleanup_state"] = cleanup_state
        failure_details["cleanup_removed_owned_root"] = cleanup_state == "absent"
        failure_details["quarantined_cleanup_debt"] = cleanup_state == "quarantined"
        failure_details["failure_commit_revision"] = failure_commit.revision
        raise ProgressError(
            "explicit maintenance output allocation failed closed: "
            f"{json.dumps(failure_details, sort_keys=True)}"
        ) from exc
    else:
        # Deliberately outside the compensation handler: serialization/stdout
        # failures after the activation CAS must never destroy active work.
        if activation_commit is None or success_details is None:
            raise ProgressError("explicit maintenance activation produced no durable result")
        return _commit_payload(activation_commit, success_details)


def recover_explicit_maintenance_allocation(args: argparse.Namespace) -> dict[str, Any]:
    """Independently inspect and recover one journal-owned allocation root."""

    from _recoil.commands.workspace_issues import (
        cross_ledger_reservation_critical_section,
    )

    issue_ledger = Path(args.issue_ledger)
    scheduler_document, scheduler_domains = _precheck_scheduler_revision(args)
    if scheduler_domains is not None:
        expected_semantic = getattr(args, "expected_semantic_revision", None)
        if expected_semantic is None:
            raise ProgressError(
                "revision-domain allocation recovery requires --expected-semantic-revision"
            )
        vector = ProgressSQLiteStore(Path(args.progress)).read_revision_vector()
        if vector.semantic_revision != int(expected_semantic):
            raise ConcurrentProgressUpdate(
                "semantic revision changed before allocation recovery"
            )
        scheduler_domains = {
            **scheduler_domains,
            "semantic": int(expected_semantic),
        }

    document = scheduler_document or ProgressStore(args.progress).load()
    registry = document.data.get("migration", {}).get(
        EXPLICIT_ALLOCATION_JOURNAL_MIGRATION_KEY, {}
    )
    journal = (
        registry.get("rows", {}).get(args.id)
        if isinstance(registry, Mapping)
        else None
    )
    if (
        not isinstance(journal, Mapping)
        or journal.get("schema") != EXPLICIT_ALLOCATION_JOURNAL_SCHEMA
    ):
        raise ProgressError(f"unknown explicit allocation journal {args.id}")
    if journal.get("state") == "activated":
        raise ProgressError("active explicit work requires abandon, not allocation recovery")
    allocation = journal.get("expected_ownership_marker")
    if not isinstance(allocation, Mapping):
        raise ProgressError("explicit allocation journal lacks ownership evidence")
    allocation = deepcopy(dict(allocation))
    issue_identity = journal.get("issue_ledger_identity")
    if not isinstance(issue_identity, Mapping):
        raise ProgressError("explicit allocation journal lacks issue-ledger binding")
    current_issue_metadata = read_issue_metadata(issue_ledger)
    observed_issue_identity = {
        "path": str(issue_ledger.resolve()),
        "application_id": current_issue_metadata.application_id,
        "user_version": current_issue_metadata.user_version,
        "schema_version": current_issue_metadata.schema_version,
        "ledger_version": current_issue_metadata.ledger_version,
        "cutover_pair_id": current_issue_metadata.cutover_pair_id,
    }
    if dict(issue_identity) != observed_issue_identity:
        raise ProgressError("explicit allocation recovery issue-ledger identity changed")

    sealed_repo = Path(str(allocation.get("repository_root_identity", ""))).resolve()
    build_root = (sealed_repo / "build").resolve()
    root = sealed_repo / str(allocation.get("normalized_output_root", ""))
    sidecar = sealed_repo / str(allocation.get("ownership_sidecar", ""))
    for candidate, label in ((root, "output root"), (sidecar, "ownership sidecar")):
        try:
            candidate.resolve(strict=False).relative_to(build_root)
        except (OSError, ValueError) as exc:
            raise ProgressError(f"explicit allocation {label} escapes build/") from exc
        if _path_has_reparse_component(candidate, stop=build_root):
            raise ProgressError(f"explicit allocation {label} traverses a reparse point")

    def read_exact(path: Path, label: str, *, root_marker: bool = False) -> None:
        if not path.is_file() or path.is_symlink():
            raise ProgressError(f"explicit allocation {label} is unavailable")
        try:
            observed = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, UnicodeError, json.JSONDecodeError) as exc:
            raise ProgressError(f"explicit allocation {label} is invalid") from exc
        if root_marker:
            authenticate_explicit_output_marker(observed, allocation, root)
        elif observed != allocation:
            raise ProgressError(f"explicit allocation {label} changed")

    if journal.get("state") == "recovered":
        if root.exists() or sidecar.exists():
            raise ProgressError("recovered allocation journal still has filesystem state")
        return document.scheduler_output(
            {
                "work_item_id": args.id,
                "outcome": "already-clean",
                "applied": False,
                "nonaccepting": True,
            }
        )

    if root.exists():
        if not root.is_dir() or root.is_symlink():
            raise ProgressError("explicit allocation root is not an owned directory")
        read_exact(sidecar, "ownership sidecar")
        entries = list(root.iterdir())
        if entries:
            marker = root / EXPLICIT_OUTPUT_MARKER_NAME
            if entries != [marker]:
                raise ProgressError("explicit allocation root contains unexpected content")
            read_exact(marker, "ownership marker", root_marker=True)
        observed_outcome = "owned-root-removed"
    else:
        if sidecar.exists():
            read_exact(sidecar, "ownership sidecar")
        observed_outcome = "already-absent"

    preview = {
        "work_item_id": args.id,
        "outcome": "cleanup-recovery-preview" if not args.apply else observed_outcome,
        "root": display_path(root),
        "ownership_sidecar": display_path(sidecar),
        "nonaccepting": True,
        "acceptance_changed": False,
        "would_remove_owned_root": root.exists(),
    }
    if not args.apply:
        return document.scheduler_output({**preview, "applied": False})

    with cross_ledger_reservation_critical_section(Path(args.progress), issue_ledger):
        # Reauthenticate immediately before the only allowed filesystem deletion.
        current_issue_metadata = read_issue_metadata(issue_ledger)
        if {
            "path": str(issue_ledger.resolve()),
            "application_id": current_issue_metadata.application_id,
            "user_version": current_issue_metadata.user_version,
            "schema_version": current_issue_metadata.schema_version,
            "ledger_version": current_issue_metadata.ledger_version,
            "cutover_pair_id": current_issue_metadata.cutover_pair_id,
        } != dict(issue_identity):
            raise ConcurrentProgressUpdate(
                "issue-ledger identity changed during allocation recovery"
            )
        if root.exists():
            read_exact(sidecar, "ownership sidecar")
            entries = list(root.iterdir())
            if entries:
                marker = root / EXPLICIT_OUTPUT_MARKER_NAME
                if entries != [marker]:
                    raise ProgressError(
                        "explicit allocation root changed before recovery"
                    )
                read_exact(marker, "ownership marker", root_marker=True)
                marker.unlink()
            root.rmdir()
        if sidecar.exists():
            read_exact(sidecar, "ownership sidecar")
            sidecar.unlink()
        if root.exists() or sidecar.exists():
            raise ProgressError("explicit allocation cleanup did not reach verified absence")

        opaque_receipt = _issue_explicit_cleanup_recovery_receipt(
            packet_id=str(args.id),
            allocation=allocation,
            outcome=observed_outcome,
        )
        recovery_details: dict[str, Any] = {}

        def transform(data: dict[str, Any]) -> None:
            recovery_details.update(
                recover_explicit_maintenance_cleanup_debt(
                    data,
                    str(args.id),
                    recovery_receipt=opaque_receipt,
                )
            )

        current_document = ProgressStore(args.progress).load()
        if scheduler_domains is not None:
            vector = ProgressSQLiteStore(Path(args.progress)).read_revision_vector()
            expected_semantic = int(scheduler_domains["semantic"])
            if vector.semantic_revision != expected_semantic:
                raise ConcurrentProgressUpdate(
                    "semantic revision changed during allocation recovery"
                )
            commit = _call_contract_scoped_patch_commit(
                args=args,
                document=current_document,
                transform=transform,
                expected_domains={
                    "scheduler": vector.scheduler_revision,
                    "semantic": expected_semantic,
                },
                increment_domains={"scheduler"},
            )
        else:
            commit = ProgressStore(args.progress).mutate(
                transform,
                expected_revision=current_document.revision,
                apply=True,
            )
    recovery_details.update(
        {
            "root_verified_absent": True,
            "ownership_sidecar_verified_absent": True,
            "nonaccepting": True,
            "acceptance_changed": False,
        }
    )
    return _commit_payload(commit, recovery_details)


def _load_governed_binja_return_inputs(
    read_plan_json: str,
    result_json: str,
) -> tuple[list[dict[str, Any]], dict[str, Any]]:
    """Parse a finite read plan/result before any governed bridge exists."""

    from _recoil.lib.binja import (
        GOVERNED_HEXDUMP_MAX_BYTES,
        GOVERNED_READ_ENDPOINTS,
        GOVERNED_READ_PLAN_MAX_BYTES,
        GOVERNED_READ_PLAN_MAX_REQUESTS,
        GOVERNED_READ_PLAN_SCHEMA,
        GOVERNED_SELECTOR_OVERRIDE_KEYS,
    )

    if len(read_plan_json.encode("utf-8")) > GOVERNED_READ_PLAN_MAX_BYTES:
        raise ProgressError("governed Binary Ninja read plan exceeds its bounded limit")
    try:
        plan = json.loads(read_plan_json)
    except json.JSONDecodeError as exc:
        raise ProgressError(f"governed Binary Ninja read plan is invalid JSON: {exc}") from exc
    if not isinstance(plan, Mapping) or set(plan) != {"schema", "requests"}:
        raise ProgressError(
            "governed Binary Ninja read plan must contain exactly schema and requests"
        )
    if plan.get("schema") != GOVERNED_READ_PLAN_SCHEMA:
        raise ProgressError("governed Binary Ninja read plan schema is unsupported")
    raw_requests = plan.get("requests")
    if (
        not isinstance(raw_requests, list)
        or len(raw_requests) > GOVERNED_READ_PLAN_MAX_REQUESTS
    ):
        raise ProgressError("governed Binary Ninja read plan request count is invalid")
    requests: list[dict[str, Any]] = []
    for index, raw in enumerate(raw_requests):
        if not isinstance(raw, Mapping):
            raise ProgressError(f"governed Binary Ninja request {index} must be an object")
        transport = raw.get("transport")
        if transport == "json":
            if set(raw) != {"transport", "endpoint", "parameters"}:
                raise ProgressError(
                    f"governed Binary Ninja JSON request {index} has the wrong shape"
                )
            endpoint = raw.get("endpoint")
            parameters = raw.get("parameters")
            if endpoint not in GOVERNED_READ_ENDPOINTS:
                raise ProgressError(
                    f"governed Binary Ninja request {index} endpoint is not registered"
                )
            if not isinstance(parameters, Mapping) or any(
                not isinstance(key, str)
                or isinstance(value, (Mapping, list))
                or value is None
                for key, value in parameters.items()
            ):
                raise ProgressError(
                    f"governed Binary Ninja request {index} parameters must be finite JSON scalars"
                )
            forbidden_selectors = sorted(
                key
                for key in parameters
                if key.casefold() in GOVERNED_SELECTOR_OVERRIDE_KEYS
            )
            if forbidden_selectors:
                raise ProgressError(
                    f"governed Binary Ninja request {index} cannot override the Recoil saved view"
                )
            requests.append(
                {
                    "transport": "json",
                    "endpoint": str(endpoint),
                    "parameters": dict(parameters),
                }
            )
        elif transport == "hexdump":
            if set(raw) != {"transport", "address", "length"}:
                raise ProgressError(
                    f"governed Binary Ninja hexdump request {index} has the wrong shape"
                )
            address = raw.get("address")
            length = raw.get("length")
            if not isinstance(address, str) or not address.strip():
                raise ProgressError(
                    f"governed Binary Ninja hexdump request {index} needs an address"
                )
            if (
                isinstance(length, bool)
                or not isinstance(length, int)
                or length <= 0
                or length > GOVERNED_HEXDUMP_MAX_BYTES
            ):
                raise ProgressError(
                    f"governed Binary Ninja hexdump request {index} length is invalid"
                )
            requests.append(
                {
                    "transport": "hexdump",
                    "address": address.strip(),
                    "length": length,
                }
            )
        else:
            raise ProgressError(
                f"governed Binary Ninja request {index} transport is unsupported"
            )

    if len(result_json.encode("utf-8")) > EXPLICIT_RESULT_MAX_BYTES:
        raise ProgressError("explicit maintenance result exceeds the bounded result limit")
    try:
        result = json.loads(result_json)
    except json.JSONDecodeError as exc:
        raise ProgressError(f"explicit maintenance result is invalid JSON: {exc}") from exc
    if not isinstance(result, dict):
        raise ProgressError("explicit maintenance result must be an object")
    return requests, result


def return_explicit_maintenance_work_with_binja(
    args: argparse.Namespace,
) -> dict[str, Any]:
    """Execute one governed BN plan, then CAS-return its active packet."""

    from _recoil.lib.binja import GovernedBinaryNinjaReadSession

    requests, result = _load_governed_binja_return_inputs(
        args.read_plan_json, args.result_json
    )
    scheduler_document, scheduler_domains = _precheck_scheduler_revision(args)
    preflight_document = scheduler_document or ProgressStore(args.progress).load()
    preflight_work = preflight_document.collection("work_items").get(args.id)
    if (
        not isinstance(preflight_work, Mapping)
        or preflight_work.get("packet_type") != EXPLICIT_MAINTENANCE_PACKET_TYPE
    ):
        raise ProgressError(f"unknown explicit maintenance work item {args.id}")
    preflight_reservation = preflight_work.get("reservation")
    if (
        preflight_work.get("state") != "active"
        or not isinstance(preflight_reservation, Mapping)
        or preflight_reservation.get("state") != "active"
    ):
        raise ProgressError("only an active explicit maintenance packet may return")
    required_fields = preflight_work.get("required_return_fields")
    if not isinstance(required_fields, list) or set(result) != set(required_fields):
        raise ProgressError(
            "explicit maintenance result fields differ from the packet return schema"
        )
    preflight_claims, complete, _source = work_resource_claims(preflight_work)
    if not complete or {
        "kind": "binary-ninja-db",
        "id": "Recoil.bndb",
        "access": "read",
    } not in preflight_claims:
        raise ProgressError(
            "progress work return-binja requires an active BN-enabled explicit packet"
        )
    # The session constructor independently proves the packet and reservation
    # are active before it constructs any bridge.
    session = GovernedBinaryNinjaReadSession(
        args.progress,
        args.id,
    )
    for request in requests:
        if request["transport"] == "json":
            session.get_json(request["endpoint"], **request["parameters"])
        else:
            session.hexdump(request["address"], request["length"])
    receipt = session.finish()
    details: dict[str, Any] = {}

    def transform(data: dict[str, Any]) -> None:
        details.update(
            return_explicit_maintenance_work_item(
                data,
                args.id,
                result,
                binja_receipt=receipt,
                progress_path=args.progress,
            )
        )

    commit = (
        _call_contract_scoped_patch_commit(
            args=args,
            document=scheduler_document,
            transform=transform,
            expected_domains=scheduler_domains,
            increment_domains={"scheduler"},
        )
        if scheduler_domains is not None
        else ProgressStore(args.progress).mutate(
            transform,
            expected_revision=args.expected_revision,
            apply=args.apply,
        )
    )
    details.update(
        {
            "binary_ninja_read_count": len(receipt.fact_reads),
            "binary_ninja_begin_snapshot": dict(receipt.begin_snapshot),
            "binary_ninja_end_snapshot": dict(receipt.end_snapshot),
            "binary_ninja_fact_reads": [dict(row) for row in receipt.fact_reads],
            "nonaccepting": True,
            "acceptance_changed": False,
        }
    )
    return _commit_payload(commit, details)


def _target_functions(registration: Mapping[str, Any], phase: str) -> list[dict[str, Any]]:
    if phase == "authored-function-order":
        groups = registration.get("translation_unit_function_order", [])
        if isinstance(groups, list) and groups:
            rows = [
                dict(row)
                for group in groups
                if isinstance(group, Mapping)
                for row in group.get("functions", [])
                if isinstance(row, Mapping)
            ]
        else:
            rows = [dict(row) for row in registration.get("functions", []) if isinstance(row, Mapping)]
    else:
        intervals = registration.get("linked_function_intervals", [])
        rows = [
            dict(row)
            for interval in intervals
            if isinstance(interval, Mapping)
            for row in interval.get("functions", [])
            if isinstance(row, Mapping)
        ]
    return rows


def _registered_order_scope(registration: Mapping[str, Any], phase: str) -> str:
    if phase == "authored-function-order":
        groups = registration.get("translation_unit_function_order", [])
        if isinstance(groups, list) and groups:
            scopes = {
                str(group.get("order_scope") or registration.get("function_order_scope") or "")
                for group in groups
                if isinstance(group, Mapping)
            }
        else:
            scopes = {str(registration.get("function_order_scope") or "")}
        return scopes.pop() if len(scopes) == 1 else ""
    intervals = registration.get("linked_function_intervals", [])
    scopes = {
        str(interval.get("order_scope") or "full")
        for interval in intervals
        if isinstance(interval, Mapping)
    }
    return scopes.pop() if len(scopes) == 1 else ""


def _registered_order_interval(target: Mapping[str, Any]) -> tuple[str, str] | None:
    registration = target.get("registration", {})
    if not isinstance(registration, Mapping):
        return None
    manifest_value = registration.get("manifest_path")
    if not isinstance(manifest_value, str) or not manifest_value:
        return None
    tracked_manifest = _resolve_tracked_progress_file(
        manifest_value,
        context="registered order manifest",
    )
    manifest_path = tracked_manifest.physical_path
    try:
        payload = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ProgressError(
            f"cannot read registered order manifest {tracked_manifest.git_path}: {exc}"
        ) from exc
    if not isinstance(payload, Mapping):
        raise ProgressError(
            f"registered order manifest {tracked_manifest.git_path} must contain an object"
        )
    start = payload.get("retail_start")
    end = payload.get("retail_end_exclusive")
    if not isinstance(start, str) or not isinstance(end, str) or not start or not end:
        raise ProgressError(
            f"registered order manifest {tracked_manifest.git_path} lacks an exact retail interval"
        )
    return normalize_address(start), normalize_address(end)


def _synchronized_order_target(
    document: ProgressDocument,
    target_id: str,
) -> dict[str, Any]:
    """Return one VC5 target only when its tracker registration is current."""

    target = document.collection("verification_targets").get(target_id)
    if not isinstance(target, dict):
        raise ProgressError(f"order target {target_id!r} is not registered")
    if target.get("binary") != "recoil" or target.get("kind") != "vc5":
        raise ProgressError(
            f"order target {target_id!r} must be one recoil VC5 registration"
        )
    registration = target.get("registration")
    if not isinstance(registration, Mapping):
        raise ProgressError(f"order target {target_id!r} has no semantic registration")
    manifest_value = registration.get("manifest_path")
    if not isinstance(manifest_value, str) or not manifest_value:
        raise ProgressError(f"order target {target_id!r} has no registered manifest path")
    tracked_manifest = _resolve_tracked_progress_file(
        manifest_value,
        context=f"order target {target_id!r} registered manifest",
    )
    if not tracked_manifest.git_path.startswith("tools/vc5_verify_targets/"):
        raise ProgressError(
            f"order target {target_id!r} manifest is outside tools/vc5_verify_targets"
        )
    manifest_path = tracked_manifest.physical_path

    from _recoil.lib.verification_targets import vc5_target_registration

    try:
        current_id, current = vc5_target_registration(manifest_path)
    except (OSError, ValueError) as exc:
        raise ProgressError(
            f"order target {target_id!r} current registration cannot be loaded: {exc}"
        ) from exc
    stored_projection = {
        key: deepcopy(target.get(key))
        for key in ("binary", "kind", "name", "registration", "registered_addresses")
    }
    current_projection = {
        key: deepcopy(current.get(key))
        for key in ("binary", "kind", "name", "registration", "registered_addresses")
    }
    if current_id != target_id or current_projection != stored_projection:
        raise ProgressError(
            f"order target {target_id!r} tracker registration is stale or conflicting"
        )
    return target


def _registered_authored_relative_gate(
    row: Mapping[str, Any],
    *,
    fallback_authored_gate: bool = False,
) -> bool:
    cached = row.get("authored_relative_order_gate")
    authored_gate = row.get("authored_order_gate")
    if not isinstance(authored_gate, bool):
        authored_gate = (
            symbol_authored_order_gate(row)
            if "authored_order_role" in row or "pipeline_class" in row
            else fallback_authored_gate
        )
    return authored_relative_order_gate(
        authored_order_gate=authored_gate,
        logical_identity_key=str(row.get("logical_identity_key") or ""),
        icf_fold_status=str(row.get("icf_fold_status") or ""),
        cached=cached if isinstance(cached, bool) else None,
    )


def _order_row_role_gate(
    *,
    target_id: str,
    phase: str,
    row: Mapping[str, Any],
    tracker_row: Mapping[str, Any] | None,
    address: str,
    identity: str,
) -> None:
    """Require classification/role closure before whole-block acceptance.

    The raw VC5 verifier may still compare the resolved authored subset.  This
    stricter gate belongs to the target contract because a subset result cannot
    authorize a packet or acceptance for the complete covered block.
    """

    label = str(row.get("name") or row.get("symbol") or identity)
    registered_class = row.get("pipeline_class")
    tracker_class = tracker_row.get("pipeline_class") if tracker_row is not None else None
    problems: list[str] = []
    if registered_class == "unresolved":
        problems.append("registration pipeline_class='unresolved'")
    if tracker_class == "unresolved":
        problems.append("tracker pipeline_class='unresolved'")
    explicit_class_unresolved = registered_class == "unresolved" or tracker_class == "unresolved"
    effective_class = next(
        (
            value
            for value in (registered_class, tracker_class)
            if isinstance(value, str) and value and value != "unresolved"
        ),
        "",
    )
    if not effective_class and not explicit_class_unresolved:
        problems.append("pipeline_class is missing or unresolved in both registration and tracker")

    if phase == "authored-function-order":
        registered_role = row.get("authored_order_role")
        tracker_role = (
            tracker_row.get("authored_order_role") if tracker_row is not None else None
        )
        if registered_role == "unresolved":
            problems.append("registration authored_order_role='unresolved'")
        if tracker_role == "unresolved":
            problems.append("tracker authored_order_role='unresolved'")
        explicit_role_unresolved = (
            registered_role == "unresolved" or tracker_role == "unresolved"
        )
        effective_role = next(
            (
                value
                for value in (registered_role, tracker_role)
                if isinstance(value, str) and value and value != "unresolved"
            ),
            "",
        )
        if not effective_role:
            if not explicit_role_unresolved:
                problems.append(
                    "authored_order_role is missing or unresolved in both registration and tracker"
                )
        elif effective_class:
            try:
                validate_authored_order_role(effective_class, effective_role)
            except ProgressError as exc:
                problems.append(str(exc))

    if problems:
        raise OrderTargetRoleGateError(
            target_id=target_id,
            phase=phase,
            address=address,
            identity=identity,
            label=label,
            problems=problems,
        )


@_with_progress_git_inventory
def _target_order_contract(
    document: ProgressDocument,
    target_id: str,
    *,
    require_explicit_interval: bool = False,
    role_gate_diagnostics: list[OrderTargetRoleGateError] | None = None,
) -> dict[str, Any]:
    pipeline = document.pipeline("recoil", resolve_order_target=False)
    phase = str(pipeline.get("phase", ""))
    if phase not in ORDER_PHASES:
        raise ProgressError(f"live order advancement requires an order phase, found {phase!r}")
    cursor_block_id = str(pipeline.get("physical_block_id", ""))
    cursor_block = document.collection("physical_blocks").get(cursor_block_id)
    if not isinstance(cursor_block, dict):
        raise ProgressError(
            f"current order cursor has no physical block {cursor_block_id!r}"
        )
    target = document.collection("verification_targets").get(target_id)
    if not isinstance(target, dict):
        raise ProgressError(
            "--target must be the exact tracker verification-target id, not a manifest name"
        )
    key = "object" if phase == "authored-function-order" else "linked"
    if target.get("binary", target.get("registration", {}).get("binary")) != "recoil":
        raise ProgressError(f"target {target_id!r} does not belong to recoil")
    registration = target.get("registration", {})
    if not isinstance(registration, Mapping):
        raise ProgressError(f"target {target_id!r} has no semantic registration")
    registered_name = target.get("name") or registration.get("name")
    if not isinstance(registered_name, str) or not registered_name:
        raise ProgressError(f"target {target_id!r} has no registered verifier name")
    name_matches = [
        other_id
        for other_id, other in document.collection("verification_targets").items()
        if isinstance(other, Mapping)
        and (other.get("name") or other.get("registration", {}).get("name"))
        == registered_name
    ]
    if name_matches != [target_id]:
        raise ProgressError(
            f"target {target_id!r} verifier name {registered_name!r} is not unique: {name_matches}"
        )
    rows = _target_functions(registration, phase)
    if not rows:
        raise ProgressError(f"target {target_id!r} has no {phase} identity sequence")
    registered_scope = _registered_order_scope(registration, phase)
    expected_scope = "authored" if phase == "authored-function-order" else "full"
    if registered_scope and registered_scope != expected_scope:
        raise ProgressError(
            f"target {target_id!r} has {registered_scope!r} order scope, expected {expected_scope!r}"
        )
    if require_explicit_interval and registered_scope != expected_scope:
        raise ProgressError(
            f"target {target_id!r} lacks one explicit {expected_scope!r} order scope"
        )

    explicit_interval = _registered_order_interval(target)
    if require_explicit_interval and explicit_interval is None:
        raise ProgressError(f"target {target_id!r} has no registered exact retail interval")
    interval_start = address_value(explicit_interval[0]) if explicit_interval is not None else None
    interval_end = address_value(explicit_interval[1]) if explicit_interval is not None else None

    blocks = document._blocks_for_binary("recoil")
    block_indexes = {block_id: index for index, (block_id, _block) in enumerate(blocks)}
    symbols = document.collection("symbols")
    block_symbol_rows: dict[str, dict[str, Mapping[str, Any]]] = {}
    block_addresses: dict[str, set[str]] = {}
    address_blocks: dict[str, list[str]] = {}
    for block_id, block in blocks:
        selected = {
            symbol_id: symbols.get(symbol_id)
            for symbol_id in block.get("contribution_ids", [])
            if isinstance(symbols.get(symbol_id), Mapping)
        }
        block_symbol_rows[block_id] = selected
        addresses = {
            normalize_address(symbol.get("address"))
            for symbol in selected.values()
            if symbol.get("address")
        }
        block_addresses[block_id] = addresses
        for address in addresses:
            address_blocks.setdefault(address, []).append(block_id)

    identities: list[str] = []
    required_inventory_identities: list[str] = []
    identities_by_block: dict[str, list[str]] = {}
    covered_addresses_by_block: dict[str, set[str]] = {}
    row_order_covered_block_ids: list[str] = []
    deferred_role_gate_errors: list[OrderTargetRoleGateError] = []
    for row in rows:
        address = row.get("address")
        normalized = normalize_address(address)
        logical_identity = row.get("logical_identity_key")
        if logical_identity and (
            not isinstance(logical_identity, str)
            or not logical_identity.startswith(f"recoil:logical-function:{normalized}:")
        ):
            raise ProgressError(
                f"target {target_id!r} has an invalid logical identity at {normalized}"
            )
        in_explicit_interval = (
            explicit_interval is None
            or interval_start <= address_value(normalized) < interval_end
        )
        if in_explicit_interval:
            tracker_row: Mapping[str, Any] | None = None
            physical_tracker_row = symbols.get(f"recoil:function:{normalized}")
            if logical_identity and isinstance(physical_tracker_row, Mapping):
                aliases = physical_tracker_row.get("logical_aliases", {})
                if isinstance(aliases, Mapping) and isinstance(
                    aliases.get(logical_identity), Mapping
                ):
                    tracker_row = aliases[logical_identity]
            elif isinstance(physical_tracker_row, Mapping):
                tracker_row = physical_tracker_row
            try:
                _order_row_role_gate(
                    target_id=target_id,
                    phase=phase,
                    row=row,
                    tracker_row=tracker_row,
                    address=normalized,
                    identity=str(logical_identity or f"recoil:function:{normalized}"),
                )
            except OrderTargetRoleGateError as exc:
                # Role closure is a target-atomic acceptance gate, but it is
                # not proof that this target owns the scheduler cursor.  Keep
                # the diagnostic while the physical contract establishes the
                # exact current slice, then elevate it only for that owner.
                if not deferred_role_gate_errors:
                    deferred_role_gate_errors.append(exc)
                    if role_gate_diagnostics is not None:
                        role_gate_diagnostics.append(exc)
        if row.get("required_presence", True) is not True:
            continue
        # A proven folded alias is required source/COFF inventory, but its
        # address is the selected representative's retail address.  It neither
        # occupies a second authored-relative position nor extends/re-enters
        # this target's physical block coverage.  The live verifier still
        # resolves every such row and fails if it is absent or duplicated.
        if (
            phase == "authored-function-order"
            and logical_identity
            and not _registered_authored_relative_gate(row)
        ):
            required_inventory_identities.append(logical_identity)
            continue
        candidates = address_blocks.get(normalized, [])
        if len(candidates) != 1:
            raise ProgressError(
                f"target row at {normalized} resolves to {len(candidates)} physical blocks: "
                f"{candidates}"
            )
        block_id = candidates[0]
        if (
            row_order_covered_block_ids
            and block_id != row_order_covered_block_ids[-1]
            and block_id in row_order_covered_block_ids
        ):
            raise ProgressError(
                f"target {target_id!r} re-enters physical block {block_id}; block slices must be ordered"
            )
        if (
            not row_order_covered_block_ids
            or block_id != row_order_covered_block_ids[-1]
        ):
            row_order_covered_block_ids.append(block_id)
        covered_addresses_by_block.setdefault(block_id, set()).add(normalized)
        block_symbols = block_symbol_rows[block_id]
        physical_rows = [
            (symbol_id, symbol)
            for symbol_id, symbol in block_symbols.items()
            if normalize_address(symbol.get("address")) == normalized
        ]
        if logical_identity:
            identity = logical_identity
            gates = (
                _registered_authored_relative_gate(row)
                if phase == "authored-function-order"
                else row.get("full_order_gate") is True
            )
        else:
            exact_physical = [
                symbol_id for symbol_id, _symbol in physical_rows if symbol_id == f"recoil:function:{normalized}"
            ]
            if len(exact_physical) != 1:
                raise ProgressError(
                    f"target {target_id!r} does not resolve one tracker physical identity at {normalized}"
                )
            identity = exact_physical[0]
            if phase == "authored-function-order":
                gates = _registered_authored_relative_gate(
                    row,
                    fallback_authored_gate=any(
                        symbol_authored_order_gate(symbol)
                        for _symbol_id, symbol in physical_rows
                    ),
                )
            else:
                gates = row.get("full_order_gate", True) is not False
        if not gates:
            continue
        identities.append(identity)
        identities_by_block.setdefault(block_id, []).append(identity)
    all_required_identities = [*identities, *required_inventory_identities]
    if len(set(all_required_identities)) != len(all_required_identities):
        raise ProgressError(f"target {target_id!r} contains duplicate expected identities")

    if not row_order_covered_block_ids:
        raise ProgressError(f"target {target_id!r} has no target-owned physical blocks")
    # Authored-order rows preserve each translation unit's raw VC5 emission
    # sequence.  Concatenating two translation-unit groups therefore need not
    # be retail-monotonic even when their physical contributions form one
    # exact contiguous retail envelope.  Physical ownership and acceptance
    # use tracker retail order; the verifier identity sequence above remains
    # untouched in translation-unit order.
    covered_block_ids = sorted(
        row_order_covered_block_ids,
        key=block_indexes.__getitem__,
    )
    covered_indexes = [block_indexes[block_id] for block_id in covered_block_ids]
    if len(set(covered_indexes)) != len(covered_indexes):
        raise ProgressError(
            f"target {target_id!r} contains duplicate target-owned physical blocks"
        )
    target_start = normalize_address(
        explicit_interval[0]
        if explicit_interval is not None
        else document.collection("physical_blocks")[covered_block_ids[0]]["start"]
    )
    target_end = normalize_address(
        explicit_interval[1]
        if explicit_interval is not None
        else document.collection("physical_blocks")[covered_block_ids[-1]]["end_exclusive"]
    )
    envelope = blocks[covered_indexes[0] : covered_indexes[-1] + 1]
    envelope_start = normalize_address(envelope[0][1]["start"])
    envelope_end = normalize_address(envelope[-1][1]["end_exclusive"])
    if (target_start, target_end) != (envelope_start, envelope_end):
        raise ProgressError(
            f"target {target_id!r} interval [{target_start},{target_end}) does not exactly cover "
            f"the tracker block envelope [{envelope_start},{envelope_end})"
        )
    for (previous_id, previous), (current_id, current) in zip(envelope, envelope[1:]):
        if normalize_address(previous.get("end_exclusive")) != normalize_address(
            current.get("start")
        ):
            raise ProgressError(
                f"target {target_id!r} interval has an untracked gap between "
                f"{previous_id} and {current_id}"
            )

    target_slices: list[list[str]] = []
    for block_id, block_index in zip(covered_block_ids, covered_indexes):
        if not target_slices:
            target_slices.append([block_id])
            continue
        previous_id = target_slices[-1][-1]
        previous_index = block_indexes[previous_id]
        previous = document.collection("physical_blocks")[previous_id]
        current = document.collection("physical_blocks")[block_id]
        if (
            block_index == previous_index + 1
            and normalize_address(previous.get("end_exclusive"))
            == normalize_address(current.get("start"))
        ):
            target_slices[-1].append(block_id)
        else:
            target_slices.append([block_id])
    if len(target_slices) > 1:
        declared_slice_intervals = []
        linked_intervals = registration.get("linked_function_intervals", [])
        if isinstance(linked_intervals, list):
            for interval in linked_intervals:
                if not isinstance(interval, Mapping):
                    continue
                interval_scope = str(interval.get("order_scope") or "full")
                interval_start_value = interval.get("retail_start")
                interval_end_value = interval.get("retail_end_exclusive")
                function_addresses = interval.get("function_addresses", [])
                if (
                    (not isinstance(interval_start_value, str) or not interval_start_value)
                    and isinstance(function_addresses, list)
                    and function_addresses
                    and isinstance(function_addresses[0], str)
                ):
                    interval_start_value = function_addresses[0]
                if not isinstance(interval_end_value, str) or not interval_end_value:
                    interval_end_value = interval.get("successor_address")
                if (
                    interval_scope == expected_scope
                    and isinstance(interval_start_value, str)
                    and interval_start_value
                    and isinstance(interval_end_value, str)
                    and interval_end_value
                ):
                    declared_slice_intervals.append(
                        (
                            normalize_address(interval_start_value),
                            normalize_address(interval_end_value),
                        )
                    )
        actual_slice_intervals = [
            (
                normalize_address(
                    document.collection("physical_blocks")[block_slice[0]]["start"]
                ),
                normalize_address(
                    document.collection("physical_blocks")[block_slice[-1]][
                        "end_exclusive"
                    ]
                ),
            )
            for block_slice in target_slices
        ]
        if declared_slice_intervals != actual_slice_intervals:
            raise ProgressError(
                f"target {target_id!r} crosses an unrelated physical block without exact "
                f"explicit slice intervals: owned={actual_slice_intervals}, "
                f"declared={declared_slice_intervals}"
            )
    group_name = "authored" if phase == "authored-function-order" else "full"
    dimensions = (
        AUTHORED_ORDER_DIMENSIONS
        if phase == "authored-function-order"
        else FULL_ORDER_DIMENSIONS
    )
    current_slice_indexes = [
        index for index, block_slice in enumerate(target_slices) if cursor_block_id in block_slice
    ]
    if len(current_slice_indexes) != 1:
        raise ProgressError(
            f"target {target_id!r} does not own exactly one physical slice at scheduler "
            f"cursor {cursor_block_id}"
        )
    current_slice_index = current_slice_indexes[0]
    current_slice = target_slices[current_slice_index]
    if current_slice[0] != cursor_block_id:
        raise ProgressError(
            f"target {target_id!r} current physical slice starts at {current_slice[0]}, "
            f"not scheduler cursor {cursor_block_id}"
        )
    for slice_index, block_slice in enumerate(target_slices):
        current_states = [
            document._order_group_current(
                document.collection("physical_blocks")[block_id],
                group_name,
                dimensions,
            )
            for block_id in block_slice
        ]
        if slice_index < current_slice_index and not all(current_states):
            raise ProgressError(
                f"target {target_id!r} earlier physical slice {block_slice} is not fully accepted"
            )
        if slice_index == current_slice_index and any(current_states):
            raise ProgressError(
                f"target {target_id!r} current physical slice {block_slice} is already partially accepted"
            )
        if slice_index > current_slice_index:
            if all(current_states):
                for block_id in block_slice:
                    block = document.collection("physical_blocks")[block_id]
                    facts = block.get("accepted_order_facts")
                    expected_identities = identities_by_block.get(block_id, [])
                    if not (
                        isinstance(facts, Mapping)
                        and facts.get("validation_mode") == "live"
                        and facts.get("target_id") == target_id
                        and facts.get("phase") == phase
                        and facts.get("covered_block_ids") == block_slice
                        and facts.get("matched_identities") == expected_identities
                    ):
                        raise ProgressError(
                            f"target {target_id!r} later physical slice {block_slice} is "
                            "current but lacks complete exact symbol/order facts"
                        )
                continue
            if any(current_states):
                raise ProgressError(
                    f"target {target_id!r} later physical slice {block_slice} has mixed "
                    "current and pending blocks"
                )
            for block_id in block_slice:
                block = document.collection("physical_blocks")[block_id]
                order = block.get("order", {})
                group = order.get(group_name, {}) if isinstance(order, Mapping) else {}
                states = (
                    [group.get(dimension) for dimension in dimensions]
                    if isinstance(group, Mapping)
                    else []
                )
                untouched = len(states) == len(dimensions) and all(
                    isinstance(state, Mapping)
                    and state.get("result") == "pending"
                    and state.get("disposition") != "accepted"
                    for state in states
                ) and block.get("accepted_order_facts") is None
                if not untouched:
                    raise ProgressError(
                        f"target {target_id!r} later physical slice {block_slice} has "
                        "partial, stale, mixed, or ambiguous order state"
                    )

    # The verifier registration owns the semantic sequence.  In particular,
    # logical ICF contributors can intentionally follow the selected physical
    # representative even though they share an earlier retail address.
    for block_id in covered_block_ids:
        block_symbols = block_symbol_rows[block_id]
        covered_addresses = covered_addresses_by_block.get(block_id, set())
        gating_addresses = {
            normalize_address(symbol.get("address"))
            for symbol in block_symbols.values()
            if symbol_authored_order_gate(symbol)
        }
        if phase == "authored-function-order" and not gating_addresses.issubset(
            covered_addresses
        ):
            raise ProgressError(
                f"target {target_id!r} omits a covered-block authored gating identity in {block_id}"
            )
        if phase == "full-function-order" and not block_addresses[block_id].issubset(
            covered_addresses
        ):
            raise ProgressError(
                f"target {target_id!r} omits a selected covered-block identity in {block_id}"
            )
    if deferred_role_gate_errors:
        raise deferred_role_gate_errors[0]
    return {
        "phase": phase,
        "cursor_block_id": cursor_block_id,
        "covered_block_ids": list(current_slice),
        "target_owned_block_ids": covered_block_ids,
        "target_slices": target_slices,
        "current_slice_index": current_slice_index,
        "binary": "recoil",
        "order_key": key,
        "retail_start": target_start,
        "retail_end_exclusive": target_end,
        "target_id": target_id,
        "target": target,
        "identities": identities,
        "required_inventory_identities": required_inventory_identities,
        "identities_by_block": identities_by_block,
    }


def _resolve_order_target_selector(document: ProgressDocument, selector: str) -> str:
    matches = [
        target_id
        for target_id, target in document.collection("verification_targets").items()
        if isinstance(target, Mapping)
        and (target_id == selector or target.get("name") == selector)
    ]
    if len(matches) != 1:
        raise ProgressError(
            f"order target selector {selector!r} resolved to {len(matches)} registered targets: {matches}"
        )
    return matches[0]


def _order_contract_signature(contract: Mapping[str, Any]) -> tuple[Any, ...]:
    return (
        contract.get("binary"),
        contract.get("phase"),
        contract.get("retail_start"),
        contract.get("retail_end_exclusive"),
        tuple(contract.get("covered_block_ids", [])),
        tuple(contract.get("identities", [])),
        tuple(contract.get("required_inventory_identities", [])),
    )


@_with_progress_git_inventory
def _full_order_object_contract(
    document: ProgressDocument,
    acceptance: Mapping[str, Any],
    object_target_id: str,
) -> dict[str, Any]:
    """Validate the compiling object target paired with one full linked target."""

    target_owned_block_ids = list(
        acceptance.get("target_owned_block_ids", acceptance["covered_block_ids"])
    )

    target = _synchronized_order_target(document, object_target_id)
    registration = target["registration"]
    groups = registration.get("translation_unit_function_order", [])
    if (
        registration.get("check_translation_unit_function_order") is not True
        or not isinstance(groups, list)
        or not groups
    ):
        linked_only = bool(registration.get("linked_function_intervals"))
        qualifier = "linked-only " if linked_only else ""
        raise ProgressError(
            f"full-order object target {object_target_id!r} is {qualifier}and cannot run "
            "the worker vc5-order compile loop"
        )
    scope = _registered_order_scope(registration, "authored-function-order")
    if scope != "authored":
        raise ProgressError(
            f"full-order object target {object_target_id!r} has {scope!r} object-order "
            "scope, expected 'authored'"
        )
    interval = _registered_order_interval(target)
    if interval is None:
        raise ProgressError(
            f"full-order object target {object_target_id!r} has no exact retail interval"
        )
    expected_interval = (
        str(acceptance["retail_start"]),
        str(acceptance["retail_end_exclusive"]),
    )
    if interval != expected_interval:
        raise ProgressError(
            f"full-order object target {object_target_id!r} interval {interval} does not "
            f"match linked acceptance interval {expected_interval}"
        )

    symbols = document.collection("symbols")
    address_blocks: dict[str, list[str]] = {}
    block_addresses: dict[str, set[str]] = {}
    for block_id in target_owned_block_ids:
        block = document.collection("physical_blocks").get(block_id)
        if not isinstance(block, Mapping):
            raise ProgressError(
                f"full-order linked acceptance references missing block {block_id!r}"
            )
        addresses = {
            normalize_address(symbol.get("address"))
            for symbol_id in block.get("contribution_ids", [])
            for symbol in [symbols.get(symbol_id)]
            if isinstance(symbol, Mapping) and symbol.get("address")
        }
        block_addresses[str(block_id)] = addresses
        for address in addresses:
            address_blocks.setdefault(address, []).append(str(block_id))

    covered_by_object: dict[str, set[str]] = {}
    identities: list[str] = []
    required_inventory_identities: list[str] = []
    for row in _target_functions(registration, "authored-function-order"):
        if row.get("required_presence", True) is not True:
            continue
        address = normalize_address(row.get("address"))
        block_matches = address_blocks.get(address, [])
        if len(block_matches) != 1:
            raise ProgressError(
                f"full-order object target {object_target_id!r} row {address} resolves "
                f"to {len(block_matches)} linked acceptance blocks: {block_matches}"
            )
        block_id = block_matches[0]
        covered_by_object.setdefault(block_id, set()).add(address)
        logical_identity = str(row.get("logical_identity_key") or "")
        identity = logical_identity or f"recoil:function:{address}"
        if logical_identity and not _registered_authored_relative_gate(row):
            required_inventory_identities.append(identity)
        elif _registered_authored_relative_gate(row):
            identities.append(identity)

    covered_block_ids = [
        block_id
        for block_id in target_owned_block_ids
        if covered_by_object.get(block_id)
    ]
    if covered_block_ids != target_owned_block_ids:
        raise ProgressError(
            f"full-order object target {object_target_id!r} does not cover the exact linked "
            f"target-owned block set {target_owned_block_ids}"
        )
    for block_id, expected_addresses in block_addresses.items():
        actual_addresses = covered_by_object.get(block_id, set())
        if actual_addresses != expected_addresses:
            raise ProgressError(
                f"full-order object target {object_target_id!r} coverage for {block_id} is "
                f"{sorted(actual_addresses)}, expected {sorted(expected_addresses)}"
            )
    if not identities:
        raise ProgressError(
            f"full-order object target {object_target_id!r} has no authored vc5-order sequence"
        )
    all_identities = [*identities, *required_inventory_identities]
    if len(all_identities) != len(set(all_identities)):
        raise ProgressError(
            f"full-order object target {object_target_id!r} contains duplicate identities"
        )
    return {
        "binary": "recoil",
        "phase": "authored-function-order",
        "retail_start": interval[0],
        "retail_end_exclusive": interval[1],
        "covered_block_ids": list(acceptance["covered_block_ids"]),
        "target_owned_block_ids": target_owned_block_ids,
        "target_id": object_target_id,
        "target": target,
        "identities": identities,
        "required_inventory_identities": required_inventory_identities,
    }


def _current_order_contract(
    document: ProgressDocument,
    target_id: str,
    *,
    override_selector: str | None = None,
    object_selector: str | None = None,
) -> dict[str, Any]:
    acceptance = _target_order_contract(
        document,
        target_id,
        require_explicit_interval=bool(override_selector),
    )
    key = str(acceptance["order_key"])
    if acceptance["phase"] == "full-function-order":
        _synchronized_order_target(document, target_id)
        linked_selectors: list[str] = []
        object_selectors: list[str] = []
        for block_id in acceptance["covered_block_ids"]:
            block = document.collection("physical_blocks")[block_id]
            configured = block.get("order_targets", {})
            linked_value = configured.get("linked") if isinstance(configured, Mapping) else None
            object_value = configured.get("object") if isinstance(configured, Mapping) else None
            if not isinstance(linked_value, str) or not linked_value:
                raise ProgressError(
                    f"full-order target {target_id!r} has no configured linked target for "
                    f"covered block {block_id}"
                )
            if not isinstance(object_value, str) or not object_value:
                raise ProgressError(
                    f"full-order target {target_id!r} has no configured object target for "
                    f"covered block {block_id}"
                )
            linked_selectors.append(linked_value)
            object_selectors.append(object_value)
        if len(set(linked_selectors)) != 1:
            raise ProgressError(
                f"full-order covered blocks have ambiguous linked targets: {linked_selectors}"
            )
        if len(set(object_selectors)) != 1:
            raise ProgressError(
                f"full-order covered blocks have ambiguous object targets: {object_selectors}"
            )
        # Backward-compatible internal callers pass the scheduler's single
        # override selector positionally.  During full order that scheduler
        # value now names the object worker target, not the linked authority.
        if object_selector is None and override_selector:
            scheduled_override_id = _resolve_order_target_selector(
                document, override_selector
            )
            configured_object_id = _resolve_order_target_selector(
                document, object_selectors[0]
            )
            if scheduled_override_id == configured_object_id:
                object_selector = override_selector
                override_selector = None
        linked_target_id = _resolve_order_target_selector(
            document, override_selector or linked_selectors[0]
        )
        if linked_target_id != target_id:
            raise ProgressError(
                f"full-order acceptance target {target_id!r} does not match configured linked "
                f"target {linked_target_id!r}"
            )
        object_target_id = _resolve_order_target_selector(
            document, object_selector or object_selectors[0]
        )
        object_contract = _full_order_object_contract(
            document,
            acceptance,
            object_target_id,
        )
        result = dict(acceptance)
        result.update(
            {
                "acceptance_target_id": target_id,
                "linked_target_id": linked_target_id,
                "linked_target": acceptance["target"],
                "object_target_id": object_target_id,
                "object_target": object_contract["target"],
                "object_contract": object_contract,
                "worker_target_id": object_target_id,
                "worker_target": object_contract["target"],
                "verifier_target_id": linked_target_id,
                "verifier_target": acceptance["target"],
                "override_selector": override_selector or "",
                "object_selector": object_selector or "",
                "configured_selectors": {
                    "linked": linked_selectors,
                    "object": object_selectors,
                },
            }
        )
        return result
    if override_selector:
        verifier_target_id = _resolve_order_target_selector(document, override_selector)
        verifier = _target_order_contract(
            document,
            verifier_target_id,
            require_explicit_interval=True,
        )
        selectors = [override_selector]
    else:
        selectors = []
        verifier_target_ids: list[str] = []
        for block_id in acceptance["covered_block_ids"]:
            block = document.collection("physical_blocks")[block_id]
            configured = block.get("order_targets", {})
            selector = configured.get(key) if isinstance(configured, Mapping) else None
            if not isinstance(selector, str) or not selector:
                raise ProgressError(
                    f"target {target_id!r} has no configured {key} target for covered block {block_id}; "
                    f"use --{key}-target with one exact registered target"
                )
            selectors.append(selector)
            verifier_target_ids.append(_resolve_order_target_selector(document, selector))
        if len(set(verifier_target_ids)) != 1:
            raise ProgressError(
                f"target {target_id!r} covered blocks disagree on the configured {key} target: "
                f"{selectors}"
            )
        verifier_target_id = verifier_target_ids[0]
        verifier = _target_order_contract(document, verifier_target_id)
    if _order_contract_signature(verifier) != _order_contract_signature(acceptance):
        raise ProgressError(
            f"{key} target {verifier_target_id!r} does not match acceptance target {target_id!r} "
            "in binary, phase, exact interval, contiguous blocks, and tracker identities"
        )
    result = dict(acceptance)
    result.update(
        {
            "acceptance_target_id": target_id,
            "linked_target_id": "",
            "linked_target": None,
            "object_target_id": verifier_target_id,
            "object_target": verifier["target"],
            "worker_target_id": verifier_target_id,
            "worker_target": verifier["target"],
            "verifier_target": verifier["target"],
            "verifier_target_id": verifier_target_id,
            "override_selector": override_selector or "",
            "configured_selectors": selectors if not override_selector else [],
        }
    )
    return result


@_with_progress_git_inventory
def resolve_current_order_target(document: ProgressDocument) -> dict[str, Any]:
    """Resolve the current order cursor to one executable target or a typed blocker."""

    pipeline = document.pipeline("recoil", resolve_order_target=False)
    phase = str(pipeline.get("phase", ""))
    if phase not in ORDER_PHASES:
        return {"status": "not-applicable", "phase": phase}
    block_id = str(pipeline.get("physical_block_id", ""))
    block = document.collection("physical_blocks").get(block_id)
    if not isinstance(block, Mapping):
        return {
            "status": "blocked",
            "reason_code": "order-block-missing",
            "reason": f"current order cursor lacks physical block {block_id!r}",
            "phase": phase,
        }
    key = "object" if phase == "authored-function-order" else "linked"
    configured = block.get("order_targets", {})
    if phase == "full-function-order":
        linked_selector = configured.get("linked") if isinstance(configured, Mapping) else None
        object_selector = configured.get("object") if isinstance(configured, Mapping) else None
        if not isinstance(linked_selector, str) or not linked_selector:
            return {
                "status": "blocked",
                "phase": phase,
                "reason_code": "full-order-linked-target-missing",
                "reason": f"current full-order block {block_id} has no linked acceptance target",
            }
        if not isinstance(object_selector, str) or not object_selector:
            return {
                "status": "blocked",
                "phase": phase,
                "reason_code": "full-order-object-target-missing",
                "reason": f"current full-order block {block_id} has no object compile target",
            }
        try:
            linked_target_id = _resolve_order_target_selector(document, linked_selector)
            object_target_id = _resolve_order_target_selector(document, object_selector)
            contract = _current_order_contract(
                document,
                linked_target_id,
                object_selector=object_target_id,
            )
        except OrderTargetRoleGateError as exc:
            return {
                "status": "blocked",
                "phase": phase,
                "reason_code": exc.reason_code,
                "reason": str(exc),
                "blocker": deepcopy(exc.blocker),
            }
        except (ProgressError, ValueError) as exc:
            return {
                "status": "blocked",
                "phase": phase,
                "reason_code": "full-order-dual-target-invalid",
                "reason": str(exc),
            }
        return {
            "status": "ready",
            "phase": phase,
            "target_id": linked_target_id,
            "linked_target_id": linked_target_id,
            "linked_target_name": str(contract["linked_target"].get("name") or ""),
            "object_target_id": object_target_id,
            "object_target_name": str(contract["object_target"].get("name") or ""),
            "target_name": str(contract["linked_target"].get("name") or ""),
            "worker_target_name": str(contract["object_target"].get("name") or ""),
            "covered_block_ids": list(contract["covered_block_ids"]),
            "override_option": "--object-target",
            "override_selector": object_target_id,
        }
    selector = configured.get(key) if isinstance(configured, Mapping) else None
    configured_error = ""
    if isinstance(selector, str) and selector:
        try:
            target_id = _resolve_order_target_selector(document, selector)
            contract = _current_order_contract(document, target_id)
            return {
                "status": "ready",
                "phase": phase,
                "target_id": target_id,
                "target_name": str(contract["verifier_target"].get("name") or ""),
                "covered_block_ids": list(contract["covered_block_ids"]),
                "override_option": "",
                "override_selector": "",
            }
        except OrderTargetRoleGateError as exc:
            return {
                "status": "blocked",
                "phase": phase,
                "reason_code": exc.reason_code,
                "reason": str(exc),
                "blocker": deepcopy(exc.blocker),
                "configured_error": str(exc),
            }
        except ProgressError as exc:
            configured_error = str(exc)

    candidates: list[tuple[str, dict[str, Any]]] = []
    rejected: list[str] = []
    role_gate_blockers: list[OrderTargetRoleGateError] = []
    expected_scope = "authored" if phase == "authored-function-order" else "full"
    for candidate_id, candidate in document.collection("verification_targets").items():
        if (
            not isinstance(candidate, Mapping)
            or candidate.get("binary") != "recoil"
            or candidate.get("kind") != "vc5"
        ):
            continue
        candidate_role_diagnostics: list[OrderTargetRoleGateError] = []
        try:
            interval = _registered_order_interval(candidate)
            registration = candidate.get("registration", {})
            if (
                interval is None
                or not isinstance(registration, Mapping)
                or _registered_order_scope(registration, phase) != expected_scope
            ):
                continue
            contract = _target_order_contract(
                document,
                candidate_id,
                require_explicit_interval=True,
                role_gate_diagnostics=candidate_role_diagnostics,
            )
        except OrderTargetRoleGateError as exc:
            role_gate_blockers.append(exc)
            rejected.append(f"{candidate_id}: {exc}")
            continue
        except (ProgressError, ValueError) as exc:
            diagnostic = ""
            if candidate_role_diagnostics:
                diagnostic = (
                    "; role-gate diagnostic: "
                    + " | ".join(str(item) for item in candidate_role_diagnostics)
                )
            rejected.append(f"{candidate_id}: {exc}{diagnostic}")
            continue
        candidates.append((candidate_id, contract))
    exact_target_ids = [target_id for target_id, _contract in candidates]
    exact_target_ids.extend(
        str(blocker.blocker["target_id"]) for blocker in role_gate_blockers
    )
    if len(exact_target_ids) > 1:
        return {
            "status": "blocked",
            "phase": phase,
            "reason_code": "order-target-ambiguous",
            "reason": (
                f"current {phase} cursor {block_id} resolves to {len(exact_target_ids)} "
                f"exact registered targets: {exact_target_ids}"
            ),
            "configured_error": configured_error,
            "candidate_count": len(exact_target_ids),
            "candidate_ids": exact_target_ids,
            "rejected_count": len(rejected),
            "rejected_examples": rejected[:10],
        }
    if len(candidates) == 1 and not role_gate_blockers:
        target_id, contract = candidates[0]
        return {
            "status": "ready",
            "phase": phase,
            "target_id": target_id,
            "target_name": str(contract["target"].get("name") or ""),
            "covered_block_ids": list(contract["covered_block_ids"]),
            "override_option": f"--{key}-target",
            "override_selector": target_id,
            "configured_error": configured_error,
            "rejected_count": len(rejected),
            "rejected_examples": rejected[:10],
        }
    if not candidates and role_gate_blockers:
        blocker = role_gate_blockers[0]
        return {
            "status": "blocked",
            "phase": phase,
            "reason_code": blocker.reason_code,
            "reason": str(blocker),
            "blocker": deepcopy(blocker.blocker),
            "configured_error": configured_error,
            "candidate_count": 0,
            "candidate_ids": [],
            "rejected_count": len(rejected),
            "rejected_examples": rejected[:10],
        }
    reason_code = "order-target-ambiguous" if len(candidates) > 1 else "order-target-unresolved"
    reason = (
        f"current {phase} cursor {block_id} resolves to {len(candidates)} exact registered targets: "
        f"{[target_id for target_id, _contract in candidates]}"
    )
    return {
        "status": "blocked",
        "phase": phase,
        "reason_code": reason_code,
        "reason": reason,
        "configured_error": configured_error,
        "candidate_count": len(candidates),
        "candidate_ids": [target_id for target_id, _contract in candidates],
        "rejected_count": len(rejected),
        "rejected_examples": rejected[:10],
    }


def _validate_divergence(value: Any, *, passed: bool) -> dict[str, Any] | None:
    if value is None:
        if not passed:
            raise ProgressError("a failing live result requires first_divergence")
        return None
    if passed or not isinstance(value, Mapping) or value.get("kind") not in DIVERGENCE_KINDS:
        raise ProgressError("first_divergence must be null on pass or a typed divergence on failure")
    return dict(value)


def _validate_order_result(
    result: Mapping[str, Any],
    *,
    verifier_target_id: str,
    phase: str,
    block_id: str,
    covered_block_ids: list[str],
    expected: list[str],
) -> dict[str, Any]:
    if result.get("kind") != "vc5-order-live-result":
        raise ProgressError("VC5 order validator returned the wrong result kind")
    for key, expected_value in (
        ("target_id", verifier_target_id),
        ("phase", phase),
        ("physical_block_id", block_id),
    ):
        if result.get(key) != expected_value:
            raise ProgressError(f"VC5 order result has wrong {key}: {result.get(key)!r}")
    reported_blocks = result.get("covered_block_ids")
    if reported_blocks is not None and reported_blocks != covered_block_ids:
        raise ProgressError(
            "VC5 order result covered_block_ids disagrees with the tracker target"
        )
    if result.get("expected_sequence") != expected:
        raise ProgressError("VC5 order result expected_sequence disagrees with the tracker target")
    candidate = result.get("candidate_sequence")
    if not isinstance(candidate, list) or any(not isinstance(item, str) for item in candidate):
        raise ProgressError("VC5 order result candidate_sequence must be a string list")
    matched = result.get("matched_prefix_count")
    if not isinstance(matched, int) or isinstance(matched, bool) or not 0 <= matched <= len(expected):
        raise ProgressError("VC5 order matched_prefix_count is invalid")
    common = 0
    for wanted, actual in zip(expected, candidate):
        if wanted != actual:
            break
        common += 1
    if matched != common:
        raise ProgressError(
            f"VC5 order matched_prefix_count {matched} disagrees with common prefix {common}"
        )
    passed = result.get("passed") is True
    divergence = _validate_divergence(result.get("first_divergence"), passed=passed)
    if passed and (candidate != expected or matched != len(expected)):
        raise ProgressError("passing VC5 order result is not an exact sequence match")
    return {
        "passed": passed,
        "candidate_sequence": candidate,
        "matched_prefix_count": matched,
        "first_divergence": divergence,
        "covered_block_ids": list(covered_block_ids),
    }


def _delegated_vc5_build_failure(summary: Mapping[str, Any]) -> str | None:
    """Describe an execution-stage VC5 failure written before linked-order reporting.

    The delegated linked-order command uses the ordinary VC5 builder.  When
    compile, resource, or link execution fails, that builder writes its
    command-result summary and returns before it can write the typed
    ``linked-function-order-run`` envelope.  Recognize only that narrow early
    failure shape here; successful or malformed summaries must continue into
    the linked-order schema checks below.
    """

    if summary.get("kind") is not None or summary.get("dry_run") is not False:
        return None
    results = summary.get("results")
    if not isinstance(results, list):
        return None
    for row in results:
        if not isinstance(row, Mapping):
            continue
        returncode = row.get("returncode")
        if (
            not isinstance(returncode, int)
            or isinstance(returncode, bool)
            or returncode == 0
        ):
            continue
        name = row.get("name")
        if not isinstance(name, str) or not (
            name == "compile"
            or name.startswith("compile:")
            or name == "resource"
            or name == "link"
        ):
            return None
        diagnostic = (
            "full linked-order delegated VC5 build failed: "
            f"first_nonzero_result={name!r}, exit_code={returncode}"
        )
        log_fields = []
        for key in ("stdout_log", "stderr_log"):
            value = row.get(key)
            if isinstance(value, str) and value:
                log_fields.append(f"{key}={value!r}")
        if log_fields:
            diagnostic += "; " + ", ".join(log_fields)
        return diagnostic
    return None


def _run_full_linked_order_validation(
    *,
    contract: Mapping[str, Any],
    build_root: Path,
    progress_path: Path,
) -> tuple[int, dict[str, Any], str]:
    """Run and validate the authoritative full linked-order report."""

    target = contract["linked_target"]
    target_name = str(target.get("name") or target.get("registration", {}).get("name") or "")
    if not target_name:
        raise ProgressError("full-order linked acceptance target has no verifier name")
    registration = target.get("registration", {})
    intervals = registration.get("linked_function_intervals", [])
    if not isinstance(intervals, list) or len(intervals) != 1:
        raise ProgressError(
            f"full-order linked target {contract['linked_target_id']!r} must contain one "
            "exact linked interval"
        )
    try:
        linked_build_root = build_root.resolve().relative_to(REPO_ROOT.resolve())
    except ValueError as exc:
        raise ProgressError(
            "full linked-order build root must resolve below the repository"
        ) from exc
    if (
        len(linked_build_root.parts) < 2
        or linked_build_root.parts[0].casefold() != "build"
    ):
        raise ProgressError(
            "full linked-order build root must name one isolated child below build/"
        )
    command = [
        sys.executable,
        str(REPO_ROOT / "tools" / "recoil.py"),
        "verify",
        "linked-order",
        target_name,
        "--scope",
        "full",
        "--build-root",
        str(linked_build_root),
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
    summary_path = build_root / "summary.json"
    if not summary_path.is_file():
        detail = completed.stderr.strip() or completed.stdout.strip()[-1000:]
        raise ProgressError(
            f"full linked-order validator produced no summary at "
            f"{display_path(summary_path)}: {detail}"
        )
    try:
        summary = json.loads(summary_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ProgressError(f"cannot read full linked-order summary: {exc}") from exc
    if not isinstance(summary, Mapping):
        raise ProgressError("full linked-order summary must be an object")
    delegated_build_failure = _delegated_vc5_build_failure(summary)
    if delegated_build_failure is not None:
        raise ProgressError(delegated_build_failure)
    if (
        summary.get("kind") != "linked-function-order-run"
        or summary.get("binary") != "recoil"
        or summary.get("order_scope") != "full"
    ):
        raise ProgressError("full linked-order summary has the wrong kind, binary, or scope")
    report_rows = summary.get("order_reports")
    if not isinstance(report_rows, list) or len(report_rows) != 1:
        raise ProgressError("full linked-order summary must expose one exact report")
    report_value = report_rows[0].get("path") if isinstance(report_rows[0], Mapping) else None
    if not isinstance(report_value, str) or not report_value:
        raise ProgressError("full linked-order summary report path is missing")
    report_path = Path(report_value).resolve()
    try:
        report_path.relative_to(build_root.resolve())
    except ValueError as exc:
        raise ProgressError("full linked-order report escaped the fresh build root") from exc
    try:
        report = json.loads(report_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ProgressError(f"cannot read full linked-order report: {exc}") from exc
    if not isinstance(report, Mapping):
        raise ProgressError("full linked-order report must be an object")
    interval = intervals[0]
    interval_name = str(interval.get("name") or "") if isinstance(interval, Mapping) else ""
    required_fields = {
        "kind": "linked-function-order-report",
        "target": target_name,
        "interval": interval_name,
        "order_scope": "full",
        "binary": "recoil",
        "retail_start": contract["retail_start"],
        "retail_end_exclusive": contract["retail_end_exclusive"],
    }
    for key, expected_value in required_fields.items():
        if report.get(key) != expected_value:
            raise ProgressError(
                f"full linked-order report {key}={report.get(key)!r}, expected {expected_value!r}"
            )
    passed = report.get("passed") is True
    if bool(summary.get("success")) != passed:
        raise ProgressError("full linked-order summary and report disagree on success")
    expected_returncode = 0 if passed else 1
    if completed.returncode != expected_returncode:
        raise ProgressError(
            f"full linked-order validator exited {completed.returncode}, expected "
            f"{expected_returncode}: {completed.stderr.strip()}"
        )
    if passed:
        gating_fields = (
            "linked_order_evaluated",
            "linked_order_passed",
            "required_presence_passed",
            "block_precedence_passed",
            "exact_selected_sequence_matches_manifest",
            "exact_sequence_address_seam_claimed",
            "boundary_sentinels_passed",
            "linked_exact_selected_population_evaluated",
            "linked_exact_selected_population_passed",
            "linked_seams_and_rvas_evaluated",
            "linked_seams_and_rvas_passed",
            "raw_definition_inventory_complete",
        )
        failed_fields = [key for key in gating_fields if report.get(key) is not True]
        if failed_fields:
            raise ProgressError(
                "passing full linked-order report lacks required typed gates: "
                + ", ".join(failed_fields)
            )
    expected = list(contract["identities"])
    return completed.returncode, {
        "passed": passed,
        "expected_sequence": expected,
        "candidate_sequence": expected if passed else [],
        "matched_prefix_count": len(expected) if passed else 0,
        "first_divergence": None if passed else deepcopy(report.get("first_divergence")),
        "covered_block_ids": list(contract["covered_block_ids"]),
        "linked_report": display_path(report_path),
        "linked_summary": display_path(summary_path),
        "linked_report_contract": deepcopy(dict(report)),
    }, completed.stderr.strip()


def _require_order_contract_source_fragments_clean(contract: Mapping[str, Any]) -> None:
    checked_manifests: set[Path] = set()
    inventory = _load_progress_git_inventory()
    for key in ("verifier_target", "object_target", "linked_target"):
        record = contract.get(key)
        if not isinstance(record, Mapping):
            continue
        registration = record.get("registration")
        if not isinstance(registration, Mapping):
            continue
        manifest_text = registration.get("manifest_path")
        if not isinstance(manifest_text, str) or not manifest_text:
            continue
        manifest_path = _resolve_tracked_progress_file(
            manifest_text,
            context=f"live order acceptance {key} manifest",
            inventory=inventory,
        ).physical_path
        if manifest_path in checked_manifests:
            continue
        checked_manifests.add(manifest_path)
        try:
            target = load_vc5_manifest(manifest_path, enforce_source_policy=False)
            require_clean_target_source_fragments(target)
        except (OSError, ValueError) as exc:
            raise ProgressError(
                f"live order acceptance source-fragment blocker for {key}: {exc}"
            ) from exc


def advance_live_order(args: argparse.Namespace) -> tuple[int, dict[str, Any]]:
    store = ProgressStore(args.progress)
    document = store.load()
    if document.revision != args.expected_revision:
        raise ConcurrentProgressUpdate(
            f"revision changed: expected {args.expected_revision}, found {document.revision}"
        )
    preliminary_phase = str(document.pipeline("recoil", resolve_order_target=False).get("phase", ""))
    object_override = getattr(args, "object_target", None)
    linked_override = getattr(args, "linked_target", None)
    if preliminary_phase == "authored-function-order":
        if linked_override:
            raise ProgressError("--linked-target is valid only during full-function-order")
        override_selector = object_override
        contract_object_selector = None
    elif preliminary_phase == "full-function-order":
        override_selector = linked_override
        contract_object_selector = object_override
    else:
        override_selector = None
        contract_object_selector = None
    contract = _current_order_contract(
        document,
        args.target,
        override_selector=override_selector,
        object_selector=contract_object_selector,
    )
    _require_order_contract_source_fragments_clean(contract)
    phase = contract["phase"]
    block_id = contract["cursor_block_id"]
    covered_block_ids = contract["covered_block_ids"]
    expected = contract["identities"]
    required_inventory_identities = contract["required_inventory_identities"]
    build_root = _absolute_fresh_build_root(args.build_root)
    if phase == "full-function-order":
        returncode, result, stderr = _run_full_linked_order_validation(
            contract=contract,
            build_root=build_root,
            progress_path=args.progress,
        )
        verifier_target_name = str(
            contract["linked_target"].get("name")
            or contract["linked_target"].get("registration", {}).get("name")
            or ""
        )
    else:
        target = contract["verifier_target"]
        verifier_target_name = str(
            target.get("name") or target.get("registration", {}).get("name") or ""
        )
        command = [
            sys.executable,
            str(REPO_ROOT / "tools" / "recoil.py"),
            "verify",
            "vc5-order",
            verifier_target_name,
            "--build-root",
            str(build_root),
            "--json",
        ]
        returncode, raw, stderr = _run_json_process(command)
        result = _validate_order_result(
            raw,
            verifier_target_id=verifier_target_name,
            phase=phase,
            # vc5-order reports the registered manifest envelope start.  A
            # later target-owned slice has a different scheduler cursor but is
            # still authorized only by the complete target comparison.
            block_id=f"recoil:block:{contract['retail_start']}",
            covered_block_ids=covered_block_ids,
            expected=expected,
        )
        if returncode not in ({0} if result["passed"] else {1}):
            raise ProgressError(f"VC5 order validator exited {returncode}: {stderr}")

    details = {
        "kind": "live-order-advance",
        "validation_mode": "live",
        "target_id": args.target,
        "verifier_target_id": contract["verifier_target_id"],
        "linked_target_id": str(contract.get("linked_target_id") or ""),
        "object_target_id": str(contract.get("object_target_id") or ""),
        "target_name": verifier_target_name,
        "target_override": contract["override_selector"],
        "object_target_override": str(contract.get("object_selector") or ""),
        "phase": phase,
        "physical_block_id": block_id,
        "covered_block_ids": covered_block_ids,
        "target_owned_block_ids": list(
            contract.get("target_owned_block_ids", covered_block_ids)
        ),
        "target_slices": deepcopy(contract.get("target_slices", [covered_block_ids])),
        "current_slice_index": int(contract.get("current_slice_index", 0)),
        "required_inventory_identities": required_inventory_identities,
        "verified_inventory_identity_count": len(required_inventory_identities),
        "validated_identity_count": len(expected),
        "build_root": display_path(build_root),
        **result,
        "accepted_block_ids": [],
        "committed_identity_count": 0,
        "mutation_planned": False,
    }
    # Validation is target-atomic.  A divergent whole-target comparison cannot
    # authorize even the current exact physical slice.
    if not result["passed"]:
        details["partial_prefix_disposition"] = (
            "no mutation: the matched identities do not complete the exact target"
        )
        details["commit"] = {
            "applied": False,
            "path": args.progress.as_posix(),
            "previous_revision": document.revision,
            "revision": document.revision,
        }
        return 1, details

    def transform(data: dict[str, Any]) -> None:
        evidence_id = add_live_evidence(
            data,
            kind="live-order-validation",
            summary=(
                f"Live {phase} comparison passed for {len(covered_block_ids)} "
                "contiguous physical blocks"
            ),
            scope_ids=[*covered_block_ids, args.target],
            provenance={
                "target_id": args.target,
                "phase": phase,
                "physical_block_id": block_id,
                "covered_block_ids": covered_block_ids,
                "target_owned_block_ids": list(
                    contract.get("target_owned_block_ids", covered_block_ids)
                ),
                "target_slices": deepcopy(
                    contract.get("target_slices", [covered_block_ids])
                ),
                "current_slice_index": int(contract.get("current_slice_index", 0)),
                "expected_sequence": expected,
                "candidate_sequence": result["candidate_sequence"],
                "matched_prefix_count": result["matched_prefix_count"],
                "required_inventory_identities": required_inventory_identities,
                "linked_target_id": str(contract.get("linked_target_id") or ""),
                "object_target_id": str(contract.get("object_target_id") or ""),
                "linked_report": result.get("linked_report", ""),
            },
        )
        for covered_block_id in covered_block_ids:
            accept_live_order_block(
                data,
                block_id=covered_block_id,
                phase=phase,
                evidence_id=evidence_id,
                facts={
                    "validation_mode": "live",
                    "target_id": args.target,
                    "phase": phase,
                    "covered_block_ids": covered_block_ids,
                    "matched_identities": contract["identities_by_block"].get(
                        covered_block_id, []
                    ),
                },
            )
        details["evidence_id"] = evidence_id

    commit = store.mutate(
        transform,
        expected_revision=args.expected_revision,
        apply=args.apply,
    )
    details.update(
        {
            "accepted_block_ids": covered_block_ids,
            "committed_identity_count": sum(
                len(contract["identities_by_block"].get(covered_block_id, []))
                for covered_block_id in covered_block_ids
            ),
            "mutation_planned": True,
        }
    )
    return 0, _commit_payload(commit, details)


def _byte_groups(document: ProgressDocument, lane: str) -> list[dict[str, Any]]:
    pipeline = document.pipeline("recoil")
    if lane == "object":
        prefix = address_value(str(pipeline.get("authored_order_prefix_end", "0x0")))
        groups = document._physical_groups("recoil", gating_only=True, eligible_end=prefix)
    elif lane == "authored":
        groups = document._physical_groups("recoil", gating_only=True)
    else:
        groups = document._physical_groups("recoil", gating_only=False)
    return groups


def _validate_byte_lane_eligibility(document: ProgressDocument, lane: str) -> None:
    pipeline = document.pipeline("recoil")
    if lane == "object":
        state = pipeline.get("authored_object_byte_lane", {}).get("state")
        if state != "ready":
            raise ProgressError(f"authored-object-byte lane is not eligible: {state}")
    elif lane == "authored":
        state = pipeline.get("authored_byte_lane", {}).get("state")
        if state not in {"ready"}:
            raise ProgressError(f"authored-byte lane is not eligible: {state}")
    elif pipeline.get("phase") != "linked-byte-match":
        raise ProgressError(
            f"linked-byte advancement requires linked-byte-match, found {pipeline.get('phase')!r}"
        )


def _normalize_matched_groups(value: Any) -> list[list[str]]:
    if not isinstance(value, list):
        raise ProgressError(
            "live byte result lacks matched_groups; required contract is an ordered list of "
            "objects shaped {scope_ids:[stable tracker symbol ids]} for every successful "
            "physical group strictly before first_divergence"
        )
    result: list[list[str]] = []
    for index, item in enumerate(value):
        if not isinstance(item, Mapping):
            raise ProgressError(f"matched_groups[{index}] must be an object")
        scope_ids = item.get("scope_ids")
        if (
            not isinstance(scope_ids, list)
            or not scope_ids
            or any(not isinstance(scope_id, str) or not scope_id for scope_id in scope_ids)
        ):
            raise ProgressError(f"matched_groups[{index}].scope_ids must be a non-empty string list")
        result.append(list(scope_ids))
    return result


def _validate_byte_result(
    result: Mapping[str, Any],
    *,
    lane: str,
    expected_groups: list[dict[str, Any]],
) -> dict[str, Any]:
    if result.get("kind") != "live-byte-lane" or result.get("lane") != lane:
        raise ProgressError("live byte validator returned the wrong lane result")
    matched = _normalize_matched_groups(result.get("matched_groups"))
    if len(matched) > len(expected_groups):
        raise ProgressError("live byte result matched more groups than the tracker lane contains")
    for index, scope_ids in enumerate(matched):
        expected = list(expected_groups[index]["scope_ids"])
        if scope_ids != expected:
            raise ProgressError(
                f"matched_groups[{index}] disagrees with tracker physical group: "
                f"expected {expected}, found {scope_ids}"
            )
    passed = result.get("passed") is True
    divergence = result.get("first_divergence")
    if passed:
        if divergence is not None or len(matched) != len(expected_groups):
            raise ProgressError("passing live byte result must match the complete selected lane")
    elif not isinstance(divergence, Mapping):
        raise ProgressError("failing live byte result requires a typed first_divergence object")
    return {
        "passed": passed,
        "matched_groups": matched,
        "first_divergence": deepcopy(divergence),
    }


def advance_live_byte(args: argparse.Namespace) -> tuple[int, dict[str, Any]]:
    store = ProgressStore(args.progress)
    document = store.load()
    if document.revision != args.expected_revision:
        raise ConcurrentProgressUpdate(
            f"revision changed: expected {args.expected_revision}, found {document.revision}"
        )
    _validate_byte_lane_eligibility(document, args.lane)
    groups = _byte_groups(document, args.lane)
    if not groups:
        raise ProgressError(f"the {args.lane} byte lane has no selected physical groups")
    build_root = _absolute_fresh_build_root(args.build_root)
    command = [
        sys.executable,
        str(REPO_ROOT / "tools" / "recoil.py"),
        "verify",
        BYTE_VERIFY_COMMANDS[args.lane],
        "--progress",
        display_path(args.progress),
        "--build-root",
        display_path(build_root),
    ]
    packet_id = str(getattr(args, "packet_id", "") or "")
    if packet_id:
        command.extend(("--packet-id", packet_id))
    command.append("--json")
    returncode, raw, stderr = _run_json_process(command)
    try:
        result = _validate_byte_result(raw, lane=args.lane, expected_groups=groups)
    except ProgressError as exc:
        if "lacks matched_groups" not in str(exc):
            raise
        return 2, {
            "kind": "live-byte-advance",
            "status": "contract-blocked",
            "lane": args.lane,
            "validation_mode": "live",
            "mutation_planned": False,
            "build_root": display_path(build_root),
            "blocker": str(exc),
            "required_verifier_change": (
                "Add matched_groups to the live byte JSON: one ordered "
                "{scope_ids:[...]} row per successful physical group, excluding the divergent group."
            ),
            "commit": {
                "applied": False,
                "path": args.progress.as_posix(),
                "previous_revision": document.revision,
                "revision": document.revision,
            },
        }
    if returncode not in ({0} if result["passed"] else {1}):
        raise ProgressError(f"live byte validator exited {returncode}: {stderr}")
    if args.lane == "object":
        dimensions = ("object_byte",)
    elif args.lane == "authored":
        dimensions = AUTHORED_BYTE_DIMENSIONS
    else:
        dimensions = EXACT_LINK_DIMENSIONS
    groups_to_commit = [
        scope_ids
        for scope_ids in result["matched_groups"]
        if not all(
            all(
                is_current_accepted_state(
                    document.collection("symbols")[symbol_id].get("binary_state", {}).get(dimension)
                )
                for dimension in dimensions
            )
            for symbol_id in scope_ids
        )
    ]
    details = {
        "kind": "live-byte-advance",
        "status": "passed" if result["passed"] else "diverged",
        "lane": args.lane,
        "validation_mode": "live",
        "build_root": display_path(build_root),
        **result,
        "committed_groups": groups_to_commit,
        "mutation_planned": bool(groups_to_commit),
    }
    if not groups_to_commit:
        details["commit"] = {
            "applied": False,
            "path": args.progress.as_posix(),
            "previous_revision": document.revision,
            "revision": document.revision,
        }
        return (0 if result["passed"] else 1), details

    flat_scope = [symbol_id for group in groups_to_commit for symbol_id in group]

    def transform(data: dict[str, Any]) -> None:
        evidence_id = add_live_evidence(
            data,
            kind="live-byte-validation",
            summary=f"Live {args.lane} byte comparison accepted {len(groups_to_commit)} physical groups",
            scope_ids=flat_scope,
            provenance={
                "lane": args.lane,
                "matched_groups": groups_to_commit,
                "first_divergence": result["first_divergence"],
            },
        )
        accept_live_byte_groups(
            data,
            lane=args.lane,
            groups=groups_to_commit,
            evidence_id=evidence_id,
            facts={"validation_mode": "live", "lane": args.lane},
        )
        details["evidence_id"] = evidence_id

    commit = store.mutate(
        transform,
        expected_revision=args.expected_revision,
        apply=args.apply,
    )
    return (0 if result["passed"] else 1), _commit_payload(commit, details)


def _validate_call_contract_result(
    result: Mapping[str, Any],
    *,
    expected_slice: Mapping[str, Any],
    expected_source_write_paths: list[str],
    expected_definition_source_paths: list[str],
    expected_compiled_definition_sources: list[str],
    expected_dependency_paths: list[str],
    expected_packet_id: str,
) -> dict[str, Any]:
    if (
        result.get("kind") != "authored-call-contract-live-result"
        or result.get("contract_version") != CALL_CONTRACT_CONTRACT_VERSION
        or result.get("packet_id") != expected_packet_id
        or result.get("all_caller_divergences_collected") is not True
    ):
        raise ProgressError("call-contract validator returned the wrong governed direct result")
    for field in ("slice_id", "symbol_ids", "target_ids", "physical_block_ids"):
        expected = expected_slice.get("id") if field == "slice_id" else expected_slice.get(field)
        if result.get(field) != expected:
            raise ProgressError(f"call-contract result {field} disagrees with the current tracker slice")
    if result.get("body_count") != expected_slice.get("body_count"):
        raise ProgressError("call-contract result body_count disagrees with the current slice")
    if result.get("candidate_expected_truth") is not False:
        raise ProgressError("call-contract result must reject candidate-derived expected truth")
    if (
        result.get("source_edit_paths") != expected_source_write_paths
        or result.get("definition_source_paths") != expected_definition_source_paths
        or result.get("dependency_paths") != expected_dependency_paths
        or result.get("source_changed_during_validation") is not False
        or result.get("dependency_states_before") != result.get("dependency_states_after")
    ):
        raise ProgressError("call-contract direct result lacks one stable exact source closure")
    compile_rows = result.get("definition_compile_results")
    if not isinstance(compile_rows, list):
        raise ProgressError("call-contract direct result lacks definition compilation rows")
    compiled_sources = [
        str(row.get("source", ""))
        for row in compile_rows
        if isinstance(row, Mapping) and row.get("returncode") == 0
    ]
    if compiled_sources != expected_compiled_definition_sources:
        raise ProgressError("call-contract direct result did not compile the exact definition closure")
    session = result.get("binary_ninja_session")
    if not isinstance(session, Mapping):
        raise ProgressError("call-contract direct result lacks an authenticated stable BN session")
    try:
        begin_snapshot = validate_authenticated_recoil_snapshot_receipt(
            session.get("begin"), stage="direct call-contract begin"
        )
        end_snapshot = validate_authenticated_recoil_snapshot_receipt(
            session.get("end"), stage="direct call-contract end"
        )
    except BridgeError as exc:
        raise ProgressError(
            "call-contract direct result has binary-ninja-snapshot-invalid: "
            f"{exc}"
        ) from exc
    if (
        session.get("snapshot_equal") is not True
        or begin_snapshot != end_snapshot
        or not isinstance(session.get("exact_fact_transcript"), list)
    ):
        raise ProgressError("call-contract direct result lacks an authenticated stable BN session")
    raw_results = result.get("body_results")
    symbol_ids = list(expected_slice.get("symbol_ids", []))
    addresses = list(expected_slice.get("addresses", []))
    if not isinstance(raw_results, list) or len(raw_results) != len(symbol_ids):
        raise ProgressError("call-contract direct result lacks one result per selected body")
    body_results: list[dict[str, Any]] = []
    passing: list[str] = []
    for symbol_id, address, raw in zip(symbol_ids, addresses, raw_results):
        if (
            not isinstance(raw, Mapping)
            or raw.get("symbol_id") != symbol_id
            or raw.get("address") != normalize_address(str(address))
            or raw.get("expected_contract") is None
            or raw.get("candidate_contract") is None
            or not evidence_generations_current(raw)
        ):
            raise ProgressError(f"call-contract direct body result is incomplete for {symbol_id}")
        status = raw.get("status")
        if status not in {"passed", "divergent", "blocked", "not-evaluated"}:
            raise ProgressError(f"call-contract body result has invalid status {status!r}")
        if status == "passed":
            if raw.get("comparison_passed") is not True or raw.get("expected_contract") != raw.get("candidate_contract"):
                raise ProgressError(f"call-contract passing body lacks exact direct equality: {symbol_id}")
            passing.append(str(symbol_id))
        elif raw.get("comparison_passed") is True:
            raise ProgressError(f"call-contract nonpassing body claims equality: {symbol_id}")
        body_results.append(deepcopy(dict(raw)))
    return {
        "passed": result.get("passed") is True,
        "first_divergence": deepcopy(result.get("first_divergence")),
        "body_results": body_results,
        "passing_symbol_ids": passing,
        "binary_ninja_session": {
            **deepcopy(dict(session)),
            "begin": begin_snapshot,
            "end": end_snapshot,
        },
    }

def _preflight_call_contract_expensive_operation(
    document: ProgressDocument,
    *,
    packet_id: str,
    slice_id: str,
    build_root: Path,
    acceptance: bool,
) -> dict[str, Any]:
    """Authenticate the reservation before any compiler or BN object exists."""

    work = document.collection("work_items").get(packet_id)
    if not isinstance(work, Mapping):
        raise ProgressError(
            "explicit call-contract verification requires an existing work packet"
        )
    reservation = work.get("reservation")
    if work.get("state") != "active" or not isinstance(reservation, Mapping):
        raise ProgressError(
            f"work packet {packet_id!r} does not own an active reservation"
        )
    if reservation.get("state") != "active" or not str(reservation.get("id", "")):
        raise ProgressError(
            f"work packet {packet_id!r} does not own an active reservation"
        )
    if str(work.get("slice_id", "")) != slice_id:
        raise ProgressError(
            f"work packet {packet_id!r} does not own slice {slice_id!r}"
        )
    claims, complete, source = work_resource_claims(work)
    if not complete or source != "explicit":
        raise ProgressError(
            f"work packet {packet_id!r} lacks complete explicit resource claims"
        )
    reservation_claims = reservation.get("resource_claims")
    if not isinstance(reservation_claims, list):
        raise ProgressError(
            f"work packet {packet_id!r} reservation lacks captured resource claims"
        )
    try:
        normalized_reservation_claims = normalize_resource_claims(
            row for row in reservation_claims if isinstance(row, Mapping)
        )
    except ProgressError as exc:
        raise ProgressError(
            f"work packet {packet_id!r} reservation has malformed resource claims"
        ) from exc
    if (
        len(normalized_reservation_claims) != len(reservation_claims)
        or normalized_reservation_claims != claims
    ):
        raise ProgressError(
            f"work packet {packet_id!r} reservation claims do not match packet claims"
        )
    claimed = {
        (str(row["kind"]), str(row["id"]), str(row["access"])) for row in claims
    }
    required = {
        ("binary-ninja-db", "Recoil.bndb", "read"),
        ("reference", "support/Recoil.exe", "read"),
        ("tracker", "recoil", "write" if acceptance else "read"),
        ("output-root", display_path(build_root).replace("\\", "/"), "write"),
    }
    missing = sorted(required - claimed)
    if missing:
        raise ProgressError(
            f"work packet {packet_id!r} does not own required verification resources: "
            + ", ".join(f"{kind}:{identity}:{access}" for kind, identity, access in missing)
        )
    authenticate_explicit_output_root(
        work,
        progress_path=document.path or DEFAULT_PROGRESS,
    )
    if acceptance and (
        work.get("packet_type") != "call-contract-acceptance-v1"
        or work.get("parent_only") is not True
    ):
        raise ProgressError(
            "call-contract acceptance requires a parent-only "
            "call-contract-acceptance-v1 reservation"
        )
    return {
        "packet_id": packet_id,
        "reservation_id": str(reservation["id"]),
        "progress_revision": int(document.revision),
        "resource_claims": deepcopy(claims),
        "work": work,
    }


def _preflight_call_contract_convergence_operation(
    document: ProgressDocument,
    *,
    packet_id: str,
    build_root: Path,
) -> dict[str, Any]:
    """Authenticate the complete reservation before convergence starts."""

    work = document.collection("work_items").get(packet_id)
    if (
        not isinstance(work, Mapping)
        or work.get("packet_type") != EXPLICIT_MAINTENANCE_PACKET_TYPE
    ):
        raise ProgressError(
            "call-contract convergence requires an explicit governed packet"
        )
    reservation = work.get("reservation")
    if (
        work.get("state") != "active"
        or not isinstance(reservation, Mapping)
        or reservation.get("state") != "active"
        or not str(reservation.get("id", ""))
    ):
        raise ProgressError("call-contract convergence packet is not active")
    claims, complete, source = work_resource_claims(work)
    raw_reservation_claims = reservation.get("resource_claims")
    if not complete or source != "explicit" or not isinstance(raw_reservation_claims, list):
        raise ProgressError("call-contract convergence packet claims are incomplete")
    normalized_reservation = normalize_resource_claims(
        row for row in raw_reservation_claims if isinstance(row, Mapping)
    )
    if normalized_reservation != claims or len(normalized_reservation) != len(
        raw_reservation_claims
    ):
        raise ProgressError("call-contract convergence reservation claims changed")
    claimed = {
        (str(row["kind"]), str(row["id"]), str(row["access"])) for row in claims
    }
    required = {
        ("binary-ninja-db", "Recoil.bndb", "read"),
        ("reference", "support/Recoil.exe", "read"),
        ("tracker", "recoil", "read"),
        ("whole-project-build", "recoil", "write"),
        ("output-root", display_path(build_root).replace("\\", "/"), "write"),
    }
    missing = sorted(required - claimed)
    if missing:
        raise ProgressError(
            "call-contract convergence packet lacks resources: "
            + ", ".join(
                f"{kind}:{identity}:{access}"
                for kind, identity, access in missing
            )
        )
    authenticate_explicit_output_root(work, progress_path=document.path or DEFAULT_PROGRESS)
    return {
        "packet_id": packet_id,
        "reservation_id": str(reservation["id"]),
        "resource_claims": deepcopy(claims),
    }


def audit_call_contract_verification_currentness(
    args: argparse.Namespace,
) -> tuple[int, dict[str, Any]]:
    """Perform the only explicit, nonaccepting expensive currentness audit."""

    build_root = _absolute_allocated_build_root(args.build_root)
    document = ProgressStore(args.progress).load()
    preflight = _preflight_call_contract_expensive_operation(
        document,
        packet_id=str(args.packet_id),
        slice_id=str(args.slice),
        build_root=build_root,
        acceptance=False,
    )
    # No alternate bridge, snapshot mapping, or child-process expected-fact
    # producer remains reachable.  A future implementation must be a reviewed
    # in-process adapter over GovernedBinaryNinjaReadSession; changing a Boolean
    # cannot resurrect the contained legacy path.
    raise ProgressError(CALL_CONTRACT_CURRENTNESS_AUDIT_DISABLED_REASON)


def advance_live_call_contract(args: argparse.Namespace) -> tuple[int, dict[str, Any]]:
    build_root = _absolute_allocated_build_root(args.build_root)
    store = ProgressStore(args.progress)
    document = store.load()
    expected_domains = _precheck_call_contract_revisions(args, document)
    continuation = continuation_state(document)
    checkpoint = continuation.get("checkpoint")
    archive_after_acceptance = bool(
        continuation.get("state") == "awaiting-parent-acceptance"
        and isinstance(checkpoint, Mapping)
        and isinstance(checkpoint.get("route_descriptor"), Mapping)
        and str(args.slice) in checkpoint["route_descriptor"].get("slice_ids", [])
    )
    if archive_after_acceptance and expected_domains is not None:
        expected_domains = {
            **expected_domains,
            "scheduler": ProgressSQLiteStore(
                Path(args.progress)
            ).read_revision_vector().scheduler_revision,
        }
    _preflight_call_contract_expensive_operation(
        document,
        packet_id=str(args.packet_id),
        slice_id=str(args.slice),
        build_root=build_root,
        acceptance=True,
    )
    slices = document.authored_call_contract_slices()
    slice_row = next((row for row in slices if row.get("id") == args.slice), None)
    if not isinstance(slice_row, Mapping):
        raise ProgressError(f"unknown authored call-contract slice {args.slice!r}")
    closure = call_contract_source_closure(document, slice_row)
    command = [
        sys.executable,
        str(REPO_ROOT / "tools" / "recoil.py"),
        "verify",
        "call-contract",
        "--slice",
        str(args.slice),
        "--progress",
        str(Path(args.progress).resolve(strict=True)),
        "--build-root",
        display_path(build_root),
        "--all-caller-divergences",
        "--json",
    ]
    returncode, raw, stderr = _run_json_process(command)
    result = _validate_call_contract_result(
        raw,
        expected_slice=slice_row,
        expected_source_write_paths=list(closure.source_edit_paths),
        expected_definition_source_paths=list(closure.definition_source_paths),
        expected_compiled_definition_sources=list(closure.definition_source_paths),
        expected_dependency_paths=list(closure.dependency_paths),
        expected_packet_id=str(args.packet_id),
    )
    if returncode not in ({0} if result["passed"] else {1}):
        raise ProgressError(f"live call-contract validator exited {returncode}: {stderr}")
    passing = [
        symbol_id
        for symbol_id in result["passing_symbol_ids"]
        if not document.call_contract_body_currentness(symbol_id).get("current")
    ]
    details = {
        "kind": "live-call-contract-advance",
        "status": "passed" if result["passed"] else "diverged",
        "slice_id": str(args.slice),
        "build_root": display_path(build_root),
        "fresh_build": True,
        "reuse": False,
        "passing_symbol_ids": passing,
        "first_divergence": result["first_divergence"],
        "mutation_planned": bool(passing or (archive_after_acceptance and result["passed"])),
    }
    if not passing and not (archive_after_acceptance and result["passed"]):
        details["commit"] = {
            "applied": False,
            "path": args.progress.as_posix(),
            "previous_revision": document.revision,
            "revision": document.revision,
        }
        return (0 if result["passed"] else 1), details

    results_by_symbol = {
        str(row["symbol_id"]): deepcopy(dict(row))
        for row in result["body_results"]
        if isinstance(row, Mapping) and str(row.get("symbol_id", "")) in passing
    }

    def transform(data: dict[str, Any]) -> None:
        evidence_ids: dict[str, str] = {}
        session = result["binary_ninja_session"]
        for symbol_id in passing:
            body = results_by_symbol[symbol_id]
            symbol = data["symbols"].get(symbol_id, {})
            transcript = [
                deepcopy(row)
                for row in session["exact_fact_transcript"]
                if isinstance(row, Mapping) and row.get("symbol_id") == symbol_id
            ]
            provenance = {
                "symbol_id": symbol_id,
                "address": body["address"],
                "target_id": body["target_id"],
                "physical_block_id": str(symbol.get("physical_block_id", "")),
                "slice_id": str(args.slice),
                "expected_truth": CALL_CONTRACT_EXPECTED_TRUTH,
                "fresh_build": True,
                "reuse": False,
                "comparison_passed": True,
                "expected_contract": deepcopy(body["expected_contract"]),
                "candidate_contract": deepcopy(body["candidate_contract"]),
                "normalizers": deepcopy(body["normalizers"]),
                "binary_ninja_session": {
                    "begin": deepcopy(session["begin"]),
                    "end": deepcopy(session["end"]),
                    "snapshot_equal": True,
                    "exact_fact_transcript": transcript,
                },
                **current_generations(),
            }
            evidence_id = add_live_evidence(
                data,
                kind="live-authored-call-contract-validation",
                summary=f"Fresh direct retail comparison accepted {symbol_id}",
                scope_ids=[symbol_id],
                provenance=provenance,
            )
            accept_live_call_contract_symbols(
                data,
                symbol_ids=[symbol_id],
                evidence_id=evidence_id,
                facts={
                    "validation_mode": "live",
                    "slice_id": str(args.slice),
                    **current_generations(),
                },
            )
            evidence_ids[symbol_id] = evidence_id
        details["evidence_ids"] = evidence_ids
        if archive_after_acceptance and result["passed"]:
            migration = data.get("migration", {})
            current_checkpoint = (
                migration.get(CONTINUATION_MIGRATION_KEY)
                if isinstance(migration, dict)
                else None
            )
            if (
                not isinstance(current_checkpoint, Mapping)
                or current_checkpoint.get("state") != "awaiting-parent-acceptance"
                or current_checkpoint.get("checkpoint_id")
                != checkpoint.get("checkpoint_id")
            ):
                raise ProgressError(
                    "repair continuation changed before fresh parent acceptance CAS"
                )
            migration[CONTINUATION_MIGRATION_KEY] = archive_continuation_checkpoint(
                current_checkpoint
            )
            details["continuation_archived"] = True
            details["child_result_accepted_directly"] = False

    increment_domains = (
        ({"semantic", "evidence_generation"} if passing else set())
        | ({"scheduler"} if archive_after_acceptance else set())
    )
    commit = _call_contract_scoped_patch_commit(
        args=args,
        document=document,
        transform=transform,
        expected_domains=expected_domains,
        increment_domains=increment_domains,
    )
    return (0 if result["passed"] else 1), _commit_payload(commit, details)


def initialize_authored_call_contract(args: argparse.Namespace) -> dict[str, Any]:
    """Parent-owned additive schema-v5 initialization for the reviewed census."""
    store = ProgressStore(args.progress)
    document = store.load()
    if document.revision != args.expected_revision:
        raise ConcurrentProgressUpdate(
            f"revision changed: expected {args.expected_revision}, found {document.revision}"
        )
    pipeline = document.pipeline("recoil", resolve_order_target=False)
    if pipeline["authored_function_order_counts"]["remaining"] != 0:
        raise ProgressError(
            "call-contract initialization requires complete authored function order"
        )
    if pipeline["full_function_order_counts"]["accepted"] != 0:
        raise ProgressError(
            "call-contract initialization requires full order to remain at zero"
        )
    slices = document.authored_call_contract_slices()
    symbol_ids = [symbol_id for row in slices for symbol_id in row["symbol_ids"]]
    symbols = document.collection("symbols")
    preexisting = [
        symbol_id
        for symbol_id in symbol_ids
        if (
            isinstance(symbols[symbol_id].get("binary_state"), Mapping)
            and "call_contract" in symbols[symbol_id]["binary_state"]
        )
        or "accepted_call_contract_facts" in symbols[symbol_id]
    ]
    if preexisting:
        raise ProgressError(
            "call-contract initialization requires a clean uninitialized census; "
            f"found {len(preexisting)} preexisting rows"
        )
    before_order = deepcopy(document.collection("physical_blocks"))
    before_other_binary_state = {
        symbol_id: deepcopy(symbols[symbol_id].get("binary_state", {}))
        for symbol_id in symbol_ids
    }
    details = {
        "kind": "authored-call-contract-initialization",
        "body_count": len(symbol_ids),
        "slice_count": len(slices),
        "slice_body_counts": [row["body_count"] for row in slices],
        "max_slice_bodies": max(row["body_count"] for row in slices),
        "full_order_accepted_before": 0,
        "preserved": {
            "physical_blocks_and_order": True,
            "existing_symbol_binary_dimensions": True,
            "owners_providers_tiers_storage": True,
        },
        "mutation_planned": True,
    }

    def transform(data: dict[str, Any]) -> None:
        current = ProgressDocument(data, path=args.progress)
        current_slices = current.authored_call_contract_slices()
        current_ids = [
            symbol_id for row in current_slices for symbol_id in row["symbol_ids"]
        ]
        if current_ids != symbol_ids:
            raise ProgressError(
                "authored call-contract census/order changed before initialization CAS"
            )
        for symbol_id in symbol_ids:
            symbol = data["symbols"][symbol_id]
            binary_state = symbol.setdefault("binary_state", {})
            if "call_contract" in binary_state or "accepted_call_contract_facts" in symbol:
                raise ProgressError(
                    f"call-contract row {symbol_id} changed before initialization CAS"
                )
            binary_state["call_contract"] = state_record(
                "pending", "observed", "changed", []
            )
        migration = data.setdefault("migration", {})
        if not isinstance(migration, dict):
            raise ProgressError("tracker migration metadata must be an object")
        if "authored_call_contract_v1" in migration:
            raise ProgressError("authored call-contract migration metadata already exists")
        migration["authored_call_contract_v1"] = {
            "body_count": len(symbol_ids),
            "slice_count": len(slices),
            "max_slice_bodies": max(row["body_count"] for row in slices),
            "state": "initialized-pending",
        }
        if data["physical_blocks"] != before_order:
            raise ProgressError("call-contract initialization changed physical/order facts")
        for symbol_id in symbol_ids:
            remaining = {
                key: value
                for key, value in data["symbols"][symbol_id]["binary_state"].items()
                if key != "call_contract"
            }
            if remaining != before_other_binary_state[symbol_id]:
                raise ProgressError(
                    f"call-contract initialization changed existing binary state for {symbol_id}"
                )

    commit = store.mutate(
        transform,
        expected_revision=args.expected_revision,
        apply=args.apply,
    )
    return _commit_payload(commit, details)


def _active_call_contract_tracker_leases(
    document: ProgressDocument,
) -> list[str]:
    try:
        work_items = document.collection("work_items")
    except (AttributeError, ProgressError):
        work_items = getattr(document, "data", {}).get("work_items", {})
    if not isinstance(work_items, Mapping):
        return []
    return sorted(
        str(work_id)
        for work_id, work in work_items.items()
        if isinstance(work, Mapping)
        and (
            work.get("phase") == "authored-call-contract"
            or work.get("packet_type") == CONTINUATION_PACKET_TYPE
        )
        and isinstance(work.get("reservation"), Mapping)
        and work["reservation"].get("state") == "active"
    )


def _continuation_state_or_none(document: Any) -> dict[str, Any]:
    try:
        return continuation_state(document)
    except AttributeError:
        # Workflow/unit command-envelope doubles do not expose tracker data.
        # Production ProgressDocument instances always take the strict path.
        return {"state": "none", "active": False, "checkpoint": None}


def _continuation_blocks_primary_scheduler(value: Mapping[str, Any]) -> bool:
    """Recognize only an explicitly active, structurally valid continuation."""

    if not isinstance(value, Mapping):
        raise ProgressError("repair continuation state must be an object")
    state = str(value.get("state", ""))
    active = value.get("active")
    active_states = {
        "producer-required", "producer-active", "descriptor-ready",
        "child-active", "child-returned", "awaiting-parent-integration",
        "awaiting-parent-acceptance",
    }
    inactive_states = {"none", "route-blocked", "archived"}
    if active is True:
        if state not in active_states:
            raise ProgressError(
                "active repair continuation has an invalid scheduler state"
            )
        return True
    if state in active_states:
        raise ProgressError(
            "repair continuation active scheduler state lacks active=true"
        )
    if active not in {False, None} or state not in inactive_states:
        raise ProgressError("repair continuation scheduler state is malformed")
    return False


def _active_call_contract_tool_leases(issue_ledger: Path) -> list[str]:
    from _recoil.commands.workspace_issues import (
        _active_issue_lease_rows,
        _load_valid_issue_ledger,
    )

    issue_data = _load_valid_issue_ledger(issue_ledger)
    semantic_paths = {
        str(path).replace("\\", "/").casefold()
        for path in CONVERGENCE_VERIFIER_SEMANTIC_PATHS
    }
    return sorted(
        str(row.get("packet_id", ""))
        for row in _active_issue_lease_rows(issue_data)
        if any(
            isinstance(claim, Mapping)
            and claim.get("kind") == "path"
            and claim.get("access") == "write"
            and str(claim.get("id", "")).replace("\\", "/").casefold()
            in semantic_paths
            for claim in row.get("resource_claims", [])
        )
    )


def _validate_convergence_verifier_semantics(
    generation: Mapping[str, Any],
) -> None:
    """Fail closed when a prepared generation no longer names current verifier code."""

    try:
        current = current_call_contract_verifier_semantic_identity()
    except (OSError, ValueError) as exc:
        raise ProgressError(
            "cannot recheck call-contract verifier semantics before CAS: " f"{exc}"
        ) from exc
    if generation.get("verifier_semantic_identity") != current:
        raise ProgressError(
            "call-contract verifier semantics changed before convergence CAS"
        )


def prepare_live_call_contract_convergence(
    args: argparse.Namespace,
) -> dict[str, Any]:
    """Prepare and CAS-record one nonaccepting phase-wide generation."""

    store = ProgressStore(args.progress)
    document = store.load()
    build_root = _absolute_allocated_build_root(args.build_root)
    _preflight_call_contract_convergence_operation(
        document,
        packet_id=str(args.packet_id),
        build_root=build_root,
    )
    # The legacy generation remains present only as reviewed closeout data-model
    # work.  It is not reachable through a Boolean toggle: a separately
    # accepted governed expected-fact producer must replace this containment
    # gate before convergence can execute.
    raise ProgressError(CALL_CONTRACT_CONVERGENCE_DISABLED_REASON)


def prepare_call_contract_repair_continuation(
    args: argparse.Namespace,
) -> dict[str, Any]:
    """Run the active producer and CAS-record only its noncurrent route."""

    from _recoil.commands.workspace_issues import issue_store

    store = ProgressStore(args.progress)
    document = store.load()
    if document.revision != args.expected_revision:
        raise ConcurrentProgressUpdate(
            f"revision changed: expected {args.expected_revision}, found {document.revision}"
        )
    predecessor = document.collection("work_items").get(args.returned_work_item)
    producer = document.collection("work_items").get(args.producer_packet)
    if not isinstance(predecessor, Mapping) or predecessor.get("state") != "returned-tool-blocked":
        raise ProgressError("returned continuation predecessor is not retained terminal debt")
    if not isinstance(producer, Mapping) or producer.get("packet_type") != CONTINUATION_PRODUCER_TYPE:
        raise ProgressError("prepare continuation requires the exact active producer packet")
    if producer.get("state") != "active" or producer.get("predecessor_work_item_id") != args.returned_work_item:
        raise ProgressError("continuation producer is not active for this predecessor")
    commands = producer.get("validation_commands")
    if not isinstance(commands, list) or len(commands) != 1:
        raise ProgressError("continuation producer requires one exact validation command")
    tokens = authenticated_validation_command_tokens(
        commands[0], require_public_route=False,
        resource_claims=producer.get("resource_claims", []),
    )
    returncode, live_result, stderr = _run_json_process(tokens)
    if returncode not in {0, 1}:
        raise ProgressError(f"continuation producer verifier exited {returncode}: {stderr}")
    producer_result = {
        "schema": PRODUCER_RESULT_SCHEMA,
        "packet_id": args.producer_packet,
        "target_id": str(producer.get("target_id", "")),
        "all_authored_bodies": live_result.get("all_authored_bodies"),
        "all_caller_divergences_collected": live_result.get("all_caller_divergences_collected"),
        "candidate_expected_truth": live_result.get("candidate_expected_truth"),
        "first_divergence": deepcopy(live_result.get("first_divergence")),
        "caller_divergences": deepcopy(live_result.get("caller_divergences", [])),
        "binary_ninja_session": deepcopy(live_result.get("binary_ninja_session")),
        "dependency_states_before": deepcopy(live_result.get("dependency_states_before")),
        "dependency_states_after": deepcopy(live_result.get("dependency_states_after")),
        "nonaccepting": True,
    }
    issue_ledger = issue_store(Path(args.issue_ledger)).load()
    preparation_document = ProgressDocument(deepcopy(document.data), path=args.progress)
    preparation_document.data["work_items"][args.producer_packet]["state"] = "returned"
    preparation = prepare_repair_continuation(
        preparation_document,
        args.returned_work_item,
        predecessor,
        issue_ledger,
        Path(args.build_root),
        producer_work_item_id=args.producer_packet,
        producer_result=producer_result,
    )
    checkpoint = deepcopy(dict(preparation.checkpoint))
    details = {**deepcopy(dict(preparation.output)), "producer_result": producer_result}

    def transform(data: dict[str, Any]) -> None:
        current = ProgressDocument(data, path=args.progress)
        current_predecessor = current.collection("work_items").get(args.returned_work_item)
        current_producer = current.collection("work_items").get(args.producer_packet)
        if deepcopy(current_predecessor) != deepcopy(predecessor) or deepcopy(current_producer) != deepcopy(producer):
            raise ProgressError("continuation predecessor/producer changed before CAS")
        current_issue = issue_store(Path(args.issue_ledger)).load()
        if current_issue.get("revision") != issue_ledger.get("revision"):
            raise ProgressError("linked issue ledger changed before continuation CAS")
        reservation = current_producer.get("reservation")
        if isinstance(reservation, dict):
            reservation["state"] = "released"
            reservation["outcome"] = "returned"
        current_producer["state"] = "returned"
        current_producer["producer_result"] = deepcopy(producer_result)
        current_producer["nonaccepting"] = True
        current_producer["acceptance_eligible"] = False
        migration = data.setdefault("migration", {})
        if not isinstance(migration, dict) or continuation_state(current).get("state") != "none":
            raise ProgressError("another continuation became active before CAS")
        migration[CONTINUATION_MIGRATION_KEY] = deepcopy(checkpoint)
        details.update({
            "checkpoint": deepcopy(checkpoint),
            "child_created": False,
            "child_requires_later_claim_current": preparation.child_descriptor is not None,
            "full_convergence_required": True,
            "noncurrent": True, "nonaccepting": True, "acceptance_eligible": False,
        })

    commit = store.mutate(transform, expected_revision=args.expected_revision, apply=args.apply)
    return _commit_payload(commit, details)


def _next_work_with_issue_ledger(
    document: ProgressDocument,
    binary: str,
    *,
    issue_ledger: str | Path,
) -> dict[str, Any]:
    if isinstance(document, ProgressDocument):
        return document.next_work(binary, issue_ledger=Path(issue_ledger))
    # Scheduler-cache and workflow-contract audits use deliberately minimal
    # document doubles.  They exercise the command envelope, not live
    # cross-ledger composition; production documents always take the explicit
    # branch above.
    return document.next_work(binary)


def _handoff(document: ProgressDocument, args: argparse.Namespace) -> dict[str, Any]:
    packet_id = str(getattr(args, "packet_id", "") or "")
    if packet_id:
        state = document.pipeline("recoil", resolve_order_target=False)
        work = document.collection("work_items").get(packet_id)
        if not isinstance(work, Mapping):
            issue_handoff = _issue_packet_handoff(document, args, packet_id)
            if issue_handoff is None:
                raise ProgressError(f"unknown work packet {packet_id}")
            return issue_handoff
        packet = _compact_reserved_packet(
            packet_id,
            work,
            progress_path=getattr(args, "progress", DEFAULT_PROGRESS),
        )
        cursor = str(work.get("cursor", ""))
        joined_scope = (
            {
                "target_ids": list(work.get("target_ids", [])),
                "covered_block_ids": list(work.get("covered_block_ids", [])),
                "source_owner_ids": list(work.get("source_owner_ids", [])),
            }
            if work.get("packet_type") == EXPLICIT_MAINTENANCE_PACKET_TYPE
            else document.show(cursor)
        )
        result = {
            "binary": str(work.get("binary", "recoil")),
            "phase": str(work.get("phase", "")),
            "primary_lane": state.get("primary_lane"),
            "cursor": cursor,
            "physical_block_id": str(work.get("block_id", "")),
            "work_item_id": packet_id,
            "reservation_id": packet["reservation_id"],
            "work_item": packet,
            "joined_cursor": joined_scope,
        }
        return document.scheduler_output(result)
    state = _next_work_with_issue_ledger(
        document,
        "recoil",
        issue_ledger=Path(
            getattr(args, "issue_ledger", DEFAULT_ISSUE_LEDGER)
        ),
    )
    if args.authored_object_byte:
        cursor = str(state.get("parallel_authored_object_byte_cursor", ""))
        expected_phase = "authored-byte-match"
        expected_lane = "object"
    elif args.authored_byte or args.fallback_authored_byte:
        cursor = str(state.get("parallel_authored_byte_cursor", ""))
        expected_phase = "authored-byte-match"
        expected_lane = "authored"
    else:
        cursor = str(state.get("cursor", ""))
        expected_phase = str(state.get("phase", ""))
        expected_lane = "primary"
    if not cursor:
        raise ProgressError("selected handoff lane has no launchable cursor")
    matches: list[tuple[str, Mapping[str, Any]]] = []
    for work_id, work in document.collection("work_items").items():
        if not isinstance(work, Mapping):
            continue
        reservation = work.get("reservation")
        if (
            work.get("state") != "active"
            or not isinstance(reservation, Mapping)
            or reservation.get("state") != "active"
        ):
            continue
        if (
            str(work.get("binary", "recoil")) == "recoil"
            and str(work.get("phase", "")) == expected_phase
            and str(work.get("cursor", "")) == cursor
            and str(work.get("lane", "primary")) == expected_lane
        ):
            matches.append((str(work_id), work))
    if len(matches) != 1:
        raise ProgressError(
            f"selected handoff requires exactly one active reserved work item for "
            f"{expected_phase} {cursor}; found {[work_id for work_id, _work in matches]}"
        )
    work_id, work = matches[0]
    packet = _compact_reserved_packet(
        work_id,
        work,
        progress_path=getattr(args, "progress", document.path),
    )
    result = {
        "binary": "recoil",
        "phase": expected_phase,
        "primary_lane": state.get("primary_lane"),
        "cursor": cursor,
        "physical_block_id": str(work.get("block_id", state.get("physical_block_id", ""))),
        "work_item_id": work_id,
        "reservation_id": packet["reservation_id"],
        "work_item": packet,
        "joined_cursor": document.show(cursor),
    }
    return document.scheduler_output(result)


def _direct_compile_host_policy_text(source_from: str) -> str:
    source_path = _resolve_tracked_progress_file(
        source_from,
        context="same-host bootstrap compile host",
    ).physical_path
    try:
        return source_path.read_text(encoding="utf-8", errors="ignore")
    except OSError as exc:
        raise ProgressError(f"cannot read same-host bootstrap compile host {source_from}: {exc}") from exc


def _same_host_nondefining_anchor_bootstrap(
    *,
    target_id: str,
    manifest_path: Path,
    compile_sources: set[str],
    closure: set[str],
    source_policy_failure: str,
) -> dict[str, Any]:
    if len(compile_sources) != 1 or closure != compile_sources:
        raise ProgressError(
            f"target {target_id!r} source-policy bootstrap requires at least one exact "
            "additional source/header path beyond its compile host"
        )
    if not any(
        marker in source_policy_failure
        for marker in (
            "does not contain provider-boundary provenance docblock/comment",
            "does not contain compiler-emitted provenance docblock/comment",
        )
    ):
        raise ProgressError(
            f"target {target_id!r} same-host source-policy bootstrap is limited to a missing "
            "non-defining provider/compiler provenance anchor"
        )

    try:
        target = load_vc5_manifest(manifest_path, enforce_source_policy=False)
    except ValueError as exc:
        raise ProgressError(
            f"target {target_id!r} same-host source-policy bootstrap cannot load its complete "
            f"manifest contract: {exc}"
        ) from exc

    compile_host = next(iter(compile_sources))
    if str(target.source_from) != compile_host:
        raise ProgressError(
            f"target {target_id!r} same-host source-policy bootstrap compile host disagrees "
            "with its manifest source_from"
        )
    direct_compile_host_text = _direct_compile_host_policy_text(compile_host)
    target_binary = str(
        getattr(target, "target_binary", target_id.split(":", 1)[0])
    )

    policy_texts: dict[str, str] = {}

    def policy_text(source_from: str) -> str:
        normalized = str(source_from)
        if normalized != compile_host:
            raise ProgressError(
                f"target {target_id!r} same-host source-policy bootstrap encountered an "
                f"out-of-closure translation-unit host: {normalized}"
            )
        if normalized not in policy_texts:
            try:
                policy_texts[normalized] = source_from_policy_text(normalized, manifest_path)
            except (OSError, ValueError) as exc:
                raise ProgressError(
                    f"target {target_id!r} cannot inspect same-host source policy: {exc}"
                ) from exc
        return policy_texts[normalized]

    pending: dict[tuple[str, str], dict[str, str]] = {}

    def inspect_function(
        function: Any,
        *,
        source_from: str,
        context: str,
        require_nondefining_anchor: bool,
    ) -> None:
        try:
            trace_documents = ()
            if function.authored_order_role in COMPILER_GENERATED_AUTHORED_ORDER_ROLES:
                trace_documents = source_trace_documents(
                    (source_from,),
                    manifest_path=manifest_path,
                )
            validate_generated_source_emission_policy(
                function=function,
                source_from=source_from,
                manifest_path=manifest_path,
                context=context,
                strict_source_emissions=bool(target.source_emission_policy_strict),
                strict_source_traceability=bool(
                    getattr(target, "source_traceability_policy_strict", False)
                ),
                target_binary=target_binary,
                trace_documents=trace_documents,
            )
        except (OSError, ValueError) as exc:
            raise ProgressError(
                f"target {target_id!r} same-host source-policy bootstrap cannot bypass "
                f"compiler-generated emission policy: {exc}"
            ) from exc
        if function.authored_order_role in COMPILER_GENERATED_AUTHORED_ORDER_ROLES:
            return

        provenance = str(function.provenance or "")
        address = normalize_address(function.address)
        if provenance in {
            FUNCTION_PROVENANCE_PROVIDER_BOUNDARY,
            FUNCTION_PROVENANCE_COMPILER_EMITTED_NONCOVERING,
        }:
            if (
                function.pipeline_class != "non-authored"
                or function.authored_order_role != "non-authored"
            ):
                raise ProgressError(
                    f"target {target_id!r} same-host source-policy bootstrap refuses "
                    f"authored stand-in provenance at {address}"
                )
            text = policy_text(source_from)
            has_marker = (
                has_provider_boundary_policy_marker(text, address)
                if provenance == FUNCTION_PROVENANCE_PROVIDER_BOUNDARY
                else has_compiler_emitted_policy_marker(text, address)
            )
            if require_nondefining_anchor and not has_marker:
                pending[(address, provenance)] = {
                    "address": address,
                    "provenance": provenance,
                }
            return
        if provenance:
            raise ProgressError(
                f"target {target_id!r} same-host source-policy bootstrap encountered unsupported "
                f"provenance {provenance!r} at {address}"
            )
        artifact_id = str(function.logical_identity_key or "")
        if not artifact_id:
            artifact_id = f"{target_binary}:function:{address}"
        try:
            direct_trace = parse_source_trace_text(
                direct_compile_host_text,
                path=compile_host,
                legacy_binary=target_binary,
            )
        except ValueError as exc:
            raise ProgressError(
                f"target {target_id!r} cannot parse direct compile-host source trace: {exc}"
            ) from exc
        if direct_trace.findings:
            details = "; ".join(
                f"{finding.path}:{finding.line}:{finding.code}"
                for finding in direct_trace.findings
            )
            raise ProgressError(
                f"target {target_id!r} direct compile-host canonical source trace is invalid: "
                f"{details}"
            )
        direct_definitions = [
            artifact
            for artifact in direct_trace.artifacts
            if artifact.artifact_id == artifact_id
            and artifact.relation == "defines"
            and artifact.section == ".text"
            and artifact.direct
            and artifact.entity_kind == "function"
            and artifact.construct is not None
            and artifact.construct.kind == "function"
        ]
        if len(direct_definitions) != 1:
            raise ProgressError(
                f"target {target_id!r} same-host source-policy bootstrap cannot relocate or "
                f"supply missing authored definition {address}; add the exact source/header "
                "path to a reviewed multi-path writable closure and attach exactly one "
                f"'@recoil-artifact defines .text {artifact_id}: ...' row"
            )

    for function in target.functions:
        inspect_function(
            function,
            source_from=target.source_from,
            context="function",
            require_nondefining_anchor=True,
        )
    for entry_index, entry in enumerate(target.translation_unit_function_order):
        for function in entry.functions:
            if function.logical_identity_key and function.icf_fold_status == "proven-fold-alias":
                continue
            inspect_function(
                function,
                source_from=entry.source_from,
                context=f"translation_unit_function_order[{entry_index}]",
                require_nondefining_anchor=False,
            )
    for generated_path, _contents in target.generated_files:
        normalized_generated_path = normalize_generated_path(generated_path)
        if generated_file_shadows_project(normalized_generated_path):
            raise ProgressError(
                f"target {target_id!r} same-host source-policy bootstrap cannot bypass generated "
                f"project-header shadow {generated_path!r}"
            )

    if not pending:
        raise ProgressError(
            f"target {target_id!r} same-host source-policy bootstrap found no missing "
            "non-defining provider/compiler provenance anchor"
        )
    if not any(address in source_policy_failure for address, _provenance in pending):
        raise ProgressError(
            f"target {target_id!r} same-host source-policy failure does not match its reviewed "
            "non-defining anchor set"
        )
    return {
        "bootstrap_reason": "same-host-nondefining-provenance-anchor",
        "pending_nondefining_provenance_anchors": [
            pending[key] for key in sorted(pending)
        ],
    }


def _source_policy_bootstrap_metadata(
    *,
    target_id: str,
    record: Mapping[str, Any],
    manifest_path: Path,
    manifest_data: Mapping[str, Any],
    order_edit_paths: tuple[str, ...],
) -> dict[str, Any]:
    if record.get("kind") != "vc5" or record.get("binary") != "recoil":
        raise ProgressError(
            "--source-policy-bootstrap is valid only for one Recoil VC5 order target"
        )
    registration = record.get("registration")
    if not isinstance(registration, Mapping):
        raise ProgressError(f"target {target_id!r} has no VC5 registration")
    start = manifest_data.get("retail_start")
    end = manifest_data.get("retail_end_exclusive")
    if not isinstance(start, str) or not isinstance(end, str) or not start or not end:
        raise ProgressError(
            f"target {target_id!r} source-policy bootstrap requires one exact retail interval"
        )
    normalized_start = normalize_address(start)
    normalized_end = normalize_address(end)
    if address_value(normalized_start) >= address_value(normalized_end):
        raise ProgressError(f"target {target_id!r} has an invalid retail interval")

    authored_groups = registration.get("translation_unit_function_order", [])
    authored_rows = [
        row
        for group in authored_groups
        if isinstance(group, Mapping)
        and str(group.get("order_scope") or registration.get("function_order_scope") or "")
        == "authored"
        and not group.get("inventory_only")
        for row in group.get("functions", [])
        if isinstance(row, Mapping)
    ] if isinstance(authored_groups, list) else []
    full_intervals = registration.get("linked_function_intervals", [])
    full_rows = [
        row
        for interval in full_intervals
        if isinstance(interval, Mapping)
        and str(interval.get("order_scope") or "full") == "full"
        for row in interval.get("functions", [])
        if isinstance(row, Mapping)
    ] if isinstance(full_intervals, list) else []
    order_scopes: list[str] = []
    if registration.get("check_translation_unit_function_order") is True and authored_rows:
        order_scopes.append("authored")
    if full_rows:
        order_scopes.append("full")
    if not order_scopes:
        raise ProgressError(
            f"target {target_id!r} source-policy bootstrap requires a complete authored or full "
            "order sequence"
        )
    for scope, rows in (("authored", authored_rows), ("full", full_rows)):
        if scope not in order_scopes:
            continue
        for row in rows:
            address = normalize_address(row.get("address"))
            if not (
                address_value(normalized_start)
                <= address_value(address)
                < address_value(normalized_end)
            ):
                raise ProgressError(
                    f"target {target_id!r} {scope} order row {address} lies outside its exact "
                    "retail interval"
                )
            if row.get("pipeline_class") in {None, "", "unresolved"}:
                raise ProgressError(
                    f"target {target_id!r} {scope} order row {address} has unresolved "
                    "pipeline classification"
                )
            if scope == "authored" and row.get("authored_order_role") in {
                None,
                "",
                "unresolved",
            }:
                raise ProgressError(
                    f"target {target_id!r} authored order row {address} has an unresolved role"
                )

    if not order_edit_paths:
        raise ProgressError(
            f"target {target_id!r} source-policy bootstrap requires exact order_edit_paths"
        )
    if any(not path.startswith("src/") for path in order_edit_paths):
        raise ProgressError(
            f"target {target_id!r} source-policy bootstrap writable closure must contain only "
            "production src/ paths"
        )
    compile_sources = {
        str(registration.get("source_from") or ""),
        *(
            str(group.get("source_from") or "")
            for group in authored_groups
            if isinstance(group, Mapping)
        ),
    } - {""}
    closure = set(order_edit_paths)
    missing_compile_sources = sorted(compile_sources - closure)
    if missing_compile_sources:
        raise ProgressError(
            f"target {target_id!r} source-policy bootstrap order_edit_paths omit compile hosts: "
            + ", ".join(missing_compile_sources)
        )
    try:
        load_vc5_manifest(manifest_path, enforce_source_policy=True)
    except ValueError as exc:
        source_policy_failure = str(exc)
        if not any(
            marker in source_policy_failure
            for marker in (
                "does not contain provenance docblock/comment",
                "does not contain provider-boundary provenance docblock/comment",
                "does not contain compiler-emitted provenance docblock/comment",
            )
        ):
            raise ProgressError(
                f"target {target_id!r} source-policy bootstrap cannot bypass this manifest "
                f"failure: {source_policy_failure}"
            ) from exc
    else:
        raise ProgressError(
            f"target {target_id!r} already satisfies source policy; use ordinary "
            "verification-target sync"
        )

    same_host_metadata: dict[str, Any] = {}
    if not closure - compile_sources:
        same_host_metadata = _same_host_nondefining_anchor_bootstrap(
            target_id=target_id,
            manifest_path=manifest_path,
            compile_sources=compile_sources,
            closure=closure,
            source_policy_failure=source_policy_failure,
        )

    return {
        "state": SOURCE_POLICY_BOOTSTRAP_STATE,
        "registration_only": True,
        "order_scopes": order_scopes,
        "retail_start": normalized_start,
        "retail_end_exclusive": normalized_end,
        "writable_closure": list(order_edit_paths),
        **same_host_metadata,
    }


def _sync_verification_targets(
    data: dict[str, Any],
    *,
    binary: str,
    selectors: Iterable[str],
    source_policy_bootstrap: bool = False,
    revalidate_accepted_order: bool = False,
) -> dict[str, Any]:
    from _recoil.lib.verification_targets import load_target_registrations

    registrations = load_target_registrations(
        functional_manifest_dir=REPO_ROOT / "tools" / "functional_verify_targets",
        vc5_manifest_dir=REPO_ROOT / "tools" / "vc5_verify_targets",
    )
    tracked_inventory = _load_progress_git_inventory()
    requested = {str(item) for item in selectors if str(item)}
    selected = {
        target_id: record
        for target_id, record in registrations.items()
        if record.get("binary") == binary
        and (
            not requested
            or target_id in requested
            or str(record.get("name", "")) in requested
        )
    }
    unresolved = requested - {
        value
        for target_id, record in selected.items()
        for value in (target_id, str(record.get("name", "")))
    }
    if unresolved:
        raise ProgressError(
            "unknown or wrong-binary verification targets: " + ", ".join(sorted(unresolved))
        )
    if not selected:
        raise ProgressError(f"no {binary} verification targets selected")
    if source_policy_bootstrap and (len(selected) != 1 or not requested):
        raise ProgressError(
            "--source-policy-bootstrap requires exactly one explicit --target selector"
        )
    if revalidate_accepted_order and (
        source_policy_bootstrap or len(selected) != 1 or not requested
    ):
        raise ProgressError(
            "--revalidate-accepted-order requires exactly one explicit existing "
            "VC5 --target and cannot be combined with --source-policy-bootstrap"
        )
    source_policy_bootstrapped: list[str] = []
    source_policy_enforced: list[str] = []
    for target_id, record in selected.items():
        if record.get("kind") != "vc5":
            if source_policy_bootstrap:
                raise ProgressError(
                    "--source-policy-bootstrap is valid only for one Recoil VC5 order target"
                )
            continue
        registration = record.get("registration")
        if not isinstance(registration, dict):
            raise ProgressError(f"target {target_id!r} has no VC5 registration")
        manifest_value = registration.get("manifest_path")
        if not isinstance(manifest_value, str) or not manifest_value:
            raise ProgressError(f"target {target_id!r} has no VC5 manifest_path")
        tracked_manifest = _resolve_tracked_progress_file(
            manifest_value,
            context=f"verification target {target_id!r} registered manifest",
            inventory=tracked_inventory,
        )
        if not tracked_manifest.git_path.startswith("tools/vc5_verify_targets/"):
            raise ProgressError(
                f"target {target_id!r} manifest is outside tools/vc5_verify_targets"
            )
        manifest_path = tracked_manifest.physical_path
        if source_policy_bootstrap:
            manifest_data = json.loads(manifest_path.read_text(encoding="utf-8"))
            if not isinstance(manifest_data, Mapping):
                raise ProgressError(
                    f"VC5 manifest {display_path(manifest_path)} must be an object"
                )
            canonical_order_edit_paths = registration.get("order_edit_paths", [])
            if not isinstance(canonical_order_edit_paths, list) or any(
                not isinstance(path, str) or not path
                for path in canonical_order_edit_paths
            ):
                raise ProgressError(
                    f"target {target_id!r} has invalid canonical order_edit_paths"
                )
            order_edit_paths = tuple(canonical_order_edit_paths)
            registration["source_policy_bootstrap"] = _source_policy_bootstrap_metadata(
                target_id=target_id,
                record=record,
                manifest_path=manifest_path,
                manifest_data=manifest_data,
                order_edit_paths=order_edit_paths,
            )
            source_policy_bootstrapped.append(target_id)
        else:
            load_vc5_manifest(manifest_path, enforce_source_policy=True)
            registration.pop("source_policy_bootstrap", None)
            source_policy_enforced.append(target_id)
    targets = data.get("verification_targets")
    symbols = data.get("symbols")
    blocks = data.get("physical_blocks")
    if not isinstance(targets, dict) or not isinstance(symbols, dict) or not isinstance(blocks, dict):
        raise ProgressError("verification-target sync requires target, symbol, and block collections")
    added: list[str] = []
    updated: list[str] = []
    unchanged: list[str] = []
    current_by_target: dict[str, Mapping[str, Any] | None] = {}
    order_semantics_changed_ids: set[str] = set()
    for target_id, record in selected.items():
        current = targets.get(target_id)
        if current is not None and not isinstance(current, Mapping):
            raise ProgressError(
                f"target {target_id!r} has a malformed current registration"
            )
        current_by_target[target_id] = current
        if current is None:
            added.append(target_id)
            order_semantics_changed_ids.add(target_id)
        elif current != record:
            updated.append(target_id)
            current_order_contract = deepcopy(dict(current))
            selected_order_contract = deepcopy(dict(record))
            for candidate in (current_order_contract, selected_order_contract):
                registration = candidate.get("registration")
                if isinstance(registration, dict):
                    # ``order_edit_paths`` is the parent-managed worker write
                    # closure.  It controls packet authority, not target
                    # population, order, or already accepted semantic facts.
                    registration.pop("order_edit_paths", None)
            if current_order_contract != selected_order_contract:
                order_semantics_changed_ids.add(target_id)
        else:
            unchanged.append(target_id)
    if revalidate_accepted_order:
        target_id, record = next(iter(selected.items()))
        if current_by_target[target_id] is None or record.get("kind") != "vc5":
            raise ProgressError(
                "--revalidate-accepted-order requires one existing VC5 target"
            )

    selected_addresses = {
        target_id: {
            normalize_address(value)
            for value in record.get("registered_addresses", [])
        }
        for target_id, record in selected.items()
    }
    planned_symbol_target_ids: list[tuple[dict[str, Any], list[str]]] = []

    for symbol in symbols.values():
        if not isinstance(symbol, dict):
            continue
        ids = [
            str(value)
            for value in symbol.get("verification_target_ids", [])
            if str(value) not in selected
        ]
        address = symbol.get("address")
        if isinstance(address, str):
            normalized = normalize_address(address)
            ids.extend(
                target_id
                for target_id in selected
                if normalized in selected_addresses[target_id]
            )
        planned_symbol_target_ids.append((symbol, sorted(set(ids))))

    match_values_by_target: dict[str, set[str]] = {}
    for target_id, record in selected.items():
        values = {target_id}
        for candidate in (record, current_by_target[target_id]):
            name = (
                candidate.get("name")
                if isinstance(candidate, Mapping)
                else None
            )
            if isinstance(name, str) and name:
                values.add(name)
        match_values_by_target[target_id] = values
    changed_match_values = {
        value
        for target_id in order_semantics_changed_ids
        for value in match_values_by_target[target_id]
    }
    revalidation_match_values = (
        next(iter(match_values_by_target.values()))
        if revalidate_accepted_order
        else set()
    )
    affected_blocks: list[str] = []
    revalidated_blocks: list[str] = []
    for block_id, block in blocks.items():
        if not isinstance(block, dict):
            raise ProgressError(
                f"physical block {block_id!r} must be an object for verification-target sync"
            )
        configured = block.get("order_targets", {})
        if configured is None:
            configured = {}
        if not isinstance(configured, Mapping) or any(
            not isinstance(value, str) for value in configured.values()
        ):
            raise ProgressError(
                f"physical block {block_id!r} has invalid order_targets; "
                "verification-target sync cannot prove its order dependencies"
            )
        accepted_facts = block.get("accepted_order_facts")
        if accepted_facts is not None and not isinstance(
            accepted_facts, Mapping
        ):
            raise ProgressError(
                f"physical block {block_id!r} has invalid accepted_order_facts; "
                "verification-target sync cannot prove its order dependencies"
            )
        configured_values = {
            value for value in configured.values() if value
        }
        accepted_values: set[str] = set()
        if isinstance(accepted_facts, Mapping):
            for field in ("target_id", "target_name", "name"):
                value = accepted_facts.get(field)
                if value is None:
                    continue
                if not isinstance(value, str) or not value:
                    raise ProgressError(
                        f"physical block {block_id!r} has invalid "
                        f"accepted_order_facts.{field}"
                    )
                accepted_values.add(value)
        changed_match = bool(
            (configured_values | accepted_values) & changed_match_values
        )
        revalidation_match = bool(
            accepted_values & revalidation_match_values
        )
        if changed_match or revalidation_match:
            affected_blocks.append(str(block_id))
        if revalidation_match:
            revalidated_blocks.append(str(block_id))
    if revalidate_accepted_order and not revalidated_blocks:
        raise ProgressError(
            "--revalidate-accepted-order found no exact accepted order facts "
            "for the selected target"
        )

    for target_id, record in selected.items():
        targets[target_id] = deepcopy(record)
    for symbol, target_ids in planned_symbol_target_ids:
        symbol["verification_target_ids"] = target_ids
    invalidated = invalidate_order_dependencies(data, block_ids=affected_blocks)
    return {
        "binary": binary,
        "selected_target_ids": sorted(selected),
        "added": sorted(added),
        "updated": sorted(updated),
        "unchanged": sorted(unchanged),
        "invalidated": invalidated,
        "revalidated_accepted_order_target_ids": (
            sorted(selected) if revalidate_accepted_order else []
        ),
        "revalidated_accepted_order_block_ids": sorted(revalidated_blocks),
        "source_policy_bootstrapped": source_policy_bootstrapped,
        "source_policy_enforced": source_policy_enforced,
    }


def _retire_verification_target(
    data: dict[str, Any],
    *,
    selector: str,
) -> dict[str, Any]:
    targets = data.get("verification_targets")
    symbols = data.get("symbols")
    blocks = data.get("physical_blocks")
    if not isinstance(targets, dict) or not isinstance(symbols, dict) or not isinstance(blocks, dict):
        raise ProgressError(
            "verification-target retire requires target, symbol, and block collections"
        )
    requested = str(selector).strip()
    if not requested:
        raise ProgressError("verification-target retire requires a non-empty --target")

    if requested in targets:
        target_id = requested
    else:
        name_matches = [
            str(target_id)
            for target_id, target in targets.items()
            if isinstance(target, Mapping) and str(target.get("name") or "") == requested
        ]
        if not name_matches:
            raise ProgressError(f"unknown verification target {requested!r}")
        if len(name_matches) != 1:
            raise ProgressError(
                f"verification target name {requested!r} resolves to "
                f"{len(name_matches)} registrations; use the exact tracker target id: "
                + ", ".join(sorted(name_matches))
            )
        target_id = name_matches[0]

    target = targets.get(target_id)
    if not isinstance(target, Mapping):
        raise ProgressError(f"verification target {target_id!r} must be an object")
    target_name = str(target.get("name") or "")

    detached_symbol_ids: list[str] = []
    detached_symbols: dict[str, dict[str, Any]] = {}
    symbol_target_ids: dict[str, list[str]] = {}
    for symbol_id, symbol in symbols.items():
        if not isinstance(symbol, dict):
            raise ProgressError(
                f"verification-target retire cannot inspect non-object symbol {symbol_id!r}"
            )
        raw_ids = symbol.get("verification_target_ids", [])
        if (
            not isinstance(raw_ids, list)
            or any(not isinstance(value, str) or not value for value in raw_ids)
            or len(raw_ids) != len(set(raw_ids))
        ):
            raise ProgressError(
                f"symbol {symbol_id!r} has invalid verification_target_ids; "
                "retirement cannot prove an exact relationship detachment"
            )
        retained = [value for value in raw_ids if value != target_id]
        symbol_target_ids[str(symbol_id)] = retained
        if len(retained) != len(raw_ids):
            detached_symbol_ids.append(str(symbol_id))
            detached_symbols[str(symbol_id)] = symbol

    affected_block_ids: list[str] = []
    for block_id, block in blocks.items():
        if not isinstance(block, dict):
            raise ProgressError(
                f"verification-target retire cannot inspect non-object block {block_id!r}"
            )
        configured = block.get("order_targets", {})
        if configured is None:
            configured = {}
        if not isinstance(configured, Mapping):
            raise ProgressError(
                f"physical block {block_id!r} has invalid order_targets; "
                "retirement cannot prove its dependent order facts"
            )
        accepted_facts = block.get("accepted_order_facts")
        configured_values = {
            str(value)
            for value in configured.values()
            if isinstance(value, str) and value
        }
        directly_accepted = (
            isinstance(accepted_facts, Mapping)
            and str(accepted_facts.get("target_id") or "") == target_id
        )
        if (
            target_id in configured_values
            or (target_name and target_name in configured_values)
            or directly_accepted
        ):
            affected_block_ids.append(str(block_id))

    del targets[target_id]
    for symbol_id in detached_symbol_ids:
        detached_symbols[symbol_id]["verification_target_ids"] = symbol_target_ids[symbol_id]
    invalidated = invalidate_order_dependencies(
        data,
        block_ids=affected_block_ids,
        symbol_ids=detached_symbol_ids,
    )
    return {
        "requested_target": requested,
        "retired_target_id": target_id,
        "retired_target_name": target_name,
        "retired_target_binary": str(target.get("binary") or ""),
        "retired_target_kind": str(target.get("kind") or ""),
        "detached_symbol_ids": sorted(detached_symbol_ids),
        "detached_symbol_count": len(detached_symbol_ids),
        "invalidated": invalidated,
        "preserved_block_order_target_block_ids": sorted(affected_block_ids),
    }


_SYMBOL_CLASSIFICATION_BATCH_FIELDS = {
    "symbol_id",
    "address",
    "reviewed",
    "current_pipeline_class",
    "current_authored_order_role",
    "pipeline_class",
    "authored_order_role",
}


def _parse_symbol_classification_batch(payload_json: str) -> list[dict[str, Any]]:
    try:
        payload = json.loads(payload_json)
    except json.JSONDecodeError as exc:
        raise ProgressError(f"--payload-json is not valid JSON: {exc.msg}") from exc
    if not isinstance(payload, list) or not payload:
        raise ProgressError("--payload-json must be a non-empty JSON array")

    result: list[dict[str, Any]] = []
    seen_symbol_ids: set[str] = set()
    seen_addresses: set[str] = set()
    for index, raw_item in enumerate(payload):
        if not isinstance(raw_item, Mapping):
            raise ProgressError(f"classification batch item {index} must be an object")
        item = dict(raw_item)
        missing = sorted(_SYMBOL_CLASSIFICATION_BATCH_FIELDS - set(item))
        extra = sorted(set(item) - _SYMBOL_CLASSIFICATION_BATCH_FIELDS)
        if missing or extra:
            details = []
            if missing:
                details.append("missing " + ", ".join(missing))
            if extra:
                details.append("unsupported " + ", ".join(extra))
            raise ProgressError(
                f"classification batch item {index} has invalid fields: " + "; ".join(details)
            )
        if item["reviewed"] is not True:
            raise ProgressError(f"classification batch item {index} requires reviewed=true")
        symbol_id = item["symbol_id"]
        address = item["address"]
        if not isinstance(symbol_id, str) or not symbol_id:
            raise ProgressError(f"classification batch item {index} requires a non-empty symbol_id")
        if not isinstance(address, str):
            raise ProgressError(f"classification batch item {index} address must be a string")
        try:
            normalized_address = normalize_address(address)
        except (ProgressError, ValueError) as exc:
            raise ProgressError(
                f"classification batch item {index} has invalid address {address!r}"
            ) from exc
        if symbol_id in seen_symbol_ids:
            raise ProgressError(f"duplicate classification symbol_id {symbol_id!r}")
        if normalized_address in seen_addresses:
            raise ProgressError(f"duplicate classification address {normalized_address}")
        seen_symbol_ids.add(symbol_id)
        seen_addresses.add(normalized_address)

        for field in ("pipeline_class", "authored_order_role"):
            if not isinstance(item[field], str) or not item[field]:
                raise ProgressError(
                    f"classification batch item {index} requires non-empty {field}"
                )
        for field in ("current_pipeline_class", "current_authored_order_role"):
            if item[field] is not None and not isinstance(item[field], str):
                raise ProgressError(
                    f"classification batch item {index} {field} must be a string or null"
                )
        pipeline_class = str(item["pipeline_class"])
        authored_order_role = str(item["authored_order_role"])
        if pipeline_class not in PIPELINE_CLASSES:
            raise ProgressError(
                f"classification batch item {index} has invalid pipeline_class {pipeline_class!r}"
            )
        validate_authored_order_role(pipeline_class, authored_order_role)
        item["address"] = normalized_address
        result.append(item)
    return result


def _set_symbol_pipeline_class_batch(
    data: dict[str, Any],
    batch: Iterable[Mapping[str, Any]],
) -> dict[str, Any]:
    symbols = data.get("symbols")
    if not isinstance(symbols, dict):
        raise ProgressError("symbol classification requires the tracker symbols collection")

    planned: list[tuple[dict[str, Any], dict[str, str]]] = []
    changes: list[dict[str, Any]] = []
    for item in batch:
        symbol_id = str(item["symbol_id"])
        row = symbols.get(symbol_id)
        if not isinstance(row, dict):
            raise ProgressError(f"unknown function symbol {symbol_id!r}")
        if row.get("binary") != "recoil" or row.get("kind") != "function":
            raise ProgressError(
                f"classification target {symbol_id!r} must be an existing Recoil function row"
            )
        stored_address = row.get("address")
        if not isinstance(stored_address, str) or normalize_address(stored_address) != item["address"]:
            raise ProgressError(
                f"classification target {symbol_id!r} address is stale: "
                f"expected {item['address']}, found {stored_address!r}"
            )
        current_pipeline_class = row.get("pipeline_class")
        current_authored_order_role = row.get("authored_order_role")
        if (
            current_pipeline_class != item["current_pipeline_class"]
            or current_authored_order_role != item["current_authored_order_role"]
        ):
            raise ProgressError(
                f"classification target {symbol_id!r} current values are stale: expected "
                f"pipeline_class={item['current_pipeline_class']!r}, "
                f"authored_order_role={item['current_authored_order_role']!r}; found "
                f"pipeline_class={current_pipeline_class!r}, "
                f"authored_order_role={current_authored_order_role!r}"
            )
        pipeline_class = str(item["pipeline_class"])
        authored_order_role = str(item["authored_order_role"])
        if (
            current_pipeline_class == pipeline_class
            and current_authored_order_role == authored_order_role
        ):
            raise ProgressError(
                f"classification target {symbol_id!r} already has the requested current values"
            )
        planned.append(
            (
                row,
                {
                    "pipeline_class": pipeline_class,
                    "authored_order_role": authored_order_role,
                },
            )
        )
        changes.append(
            {
                "symbol_id": symbol_id,
                "address": str(item["address"]),
                "reviewed": True,
                "previous": {
                    "pipeline_class": current_pipeline_class,
                    "authored_order_role": current_authored_order_role,
                },
                "replacement": {
                    "pipeline_class": pipeline_class,
                    "authored_order_role": authored_order_role,
                },
            }
        )

    for row, replacement in planned:
        row["pipeline_class"] = replacement["pipeline_class"]
        row["authored_order_role"] = replacement["authored_order_role"]
    return {
        "kind": "symbol-pipeline-class-batch",
        "reviewed": True,
        "changed_fields": ["pipeline_class", "authored_order_role"],
        "change_count": len(changes),
        "changes": changes,
    }


_LOGICAL_ALIAS_GROUP_SCHEMA_V1 = "recoil-logical-alias-group-v1"
_LOGICAL_ALIAS_GROUP_SCHEMA_V2 = "recoil-logical-alias-group-v2"
_LOGICAL_ALIAS_GROUP_SCHEMA_V3 = "recoil-logical-alias-group-v3"
_LOGICAL_ALIAS_GROUP_SCHEMA_V4 = "recoil-logical-alias-group-v4"
_LOGICAL_ALIAS_GROUP_FIELDS = {
    "schema",
    "reviewed",
    "parent_reviewed",
    "reason",
    "symbol_id",
    "address",
    "current",
    "icf_address_group",
    "logical_aliases",
}
_LOGICAL_ALIAS_GROUP_V2_FIELDS = _LOGICAL_ALIAS_GROUP_FIELDS | {"new_evidence"}
_LOGICAL_ALIAS_GROUP_V3_FIELDS = (
    _LOGICAL_ALIAS_GROUP_FIELDS - {"icf_address_group", "logical_aliases"}
) | {"new_evidence"}
_LOGICAL_ALIAS_GROUP_V4_FIELDS = _LOGICAL_ALIAS_GROUP_FIELDS | {
    "new_evidence",
    "authored_icf_proof",
}
_LOGICAL_ALIAS_CURRENT_FIELDS = {
    "pipeline_class",
    "authored_order_role",
    "physical_block_id",
    "linked_address_group",
    "icf_address_group",
    "logical_aliases",
}
_ICF_ADDRESS_GROUP_FIELDS = {
    "winner_status",
    "winner_identity_key",
    "evidence_ids",
}
_ICF_ADDRESS_GROUP_V2_REQUIRED_FIELDS = _ICF_ADDRESS_GROUP_FIELDS - {"evidence_ids"}
_ICF_ADDRESS_GROUP_V4_REQUIRED_FIELDS = (
    _ICF_ADDRESS_GROUP_FIELDS - {"evidence_ids"}
) | {"model", "physical_gate_symbol_id"}
_LOGICAL_ALIAS_FIELDS = {
    "object_symbol",
    "original_name",
    "original_name_status",
    "source_owner_status",
    "owner_id",
    "pipeline_class",
    "authored_order_role",
    "fold_status",
    "evidence_ids",
}
_LOGICAL_ALIAS_V2_REQUIRED_FIELDS = _LOGICAL_ALIAS_FIELDS - {"evidence_ids"}
_LOGICAL_ALIAS_V4_REQUIRED_FIELDS = (
    _LOGICAL_ALIAS_FIELDS - {"evidence_ids"}
) | {"gate_mode", "source_traceability", "retail_target_selectors"}
_LOGICAL_ALIAS_NEW_EVIDENCE_FIELDS = {
    "summary",
    "provenance",
    "artifacts",
    "validation_context",
}
_LOGICAL_ALIAS_EVIDENCE_KIND = "authored-order-icf-logical-alias-review"
_AUTHORED_ICF_EVIDENCE_KIND = "authored-icf-logical-member-review"
_LOGICAL_ALIAS_EVIDENCE_ROOT = "build/reconstruction-evidence/runs/"
_LOGICAL_ALIAS_V3_EVIDENCE_CONTRACT = (
    "existing-winner-unknown-physical-group-refresh-v1"
)
_LOGICAL_ALIAS_V3_AUTHORITY_SCOPE = "physical-icf-group-only"


def _require_evidence_ids(value: Any, *, label: str) -> list[str]:
    if not isinstance(value, list) or not value:
        raise ProgressError(f"{label} must be a non-empty JSON array")
    result: list[str] = []
    for index, raw_item in enumerate(value):
        if not isinstance(raw_item, str) or not raw_item.strip():
            raise ProgressError(f"{label}[{index}] must be a non-empty string")
        item = raw_item.strip()
        if item in result:
            raise ProgressError(f"{label} contains duplicate evidence id {item!r}")
        result.append(item)
    return result


def _require_v2_generated_evidence_field(
    value: Mapping[str, Any],
    *,
    required_fields: set[str],
    label: str,
) -> dict[str, Any]:
    fields = set(value)
    if fields == required_fields:
        return deepcopy(dict(value))
    if fields == required_fields | {"evidence_ids"} and value.get("evidence_ids") == []:
        result = deepcopy(dict(value))
        result.pop("evidence_ids")
        return result
    raise ProgressError(
        f"{label} fields must be exactly {sorted(required_fields)!r}; "
        "v2 evidence_ids are generated by the command and, if present for "
        "transport compatibility, must be an empty array"
    )


def _reject_alias_evidence_identity_fields(value: Any, *, label: str) -> None:
    if isinstance(value, Mapping):
        for raw_key, item in value.items():
            key = str(raw_key)
            if key in {"evidence_id", "evidence_ids", "scope_ids"}:
                raise ProgressError(
                    f"{label} must not supply {key}; evidence identity and scope are derived"
                )
            if key == "candidate_independent" and item is not True:
                raise ProgressError(f"{label}.{key} must be true")
            if key == "candidate_output_used" and item is not False:
                raise ProgressError(f"{label}.{key} must be false")
            if (
                "candidate" in key.lower()
                and key not in {"candidate_independent", "candidate_output_used"}
            ):
                raise ProgressError(
                    f"{label} must not supply candidate output/artifact field {key!r}"
                )
            _reject_alias_evidence_identity_fields(item, label=f"{label}.{key}")
    elif isinstance(value, list):
        for index, item in enumerate(value):
            _reject_alias_evidence_identity_fields(item, label=f"{label}[{index}]")


def _parse_logical_alias_new_evidence(value: Any) -> dict[str, Any]:
    if not isinstance(value, Mapping):
        raise ProgressError("logical alias group payload.new_evidence must be an object")
    evidence = _require_exact_payload_fields(
        value,
        _LOGICAL_ALIAS_NEW_EVIDENCE_FIELDS,
        label="logical alias group payload.new_evidence",
    )
    evidence["summary"] = _require_payload_string(
        evidence["summary"],
        label="logical alias group payload.new_evidence.summary",
    )
    provenance = evidence["provenance"]
    if not isinstance(provenance, Mapping) or not provenance:
        raise ProgressError(
            "logical alias group payload.new_evidence.provenance must be a non-empty object"
        )
    if provenance.get("candidate_independent") is not True:
        raise ProgressError(
            "logical alias group payload.new_evidence.provenance requires "
            "candidate_independent=true"
        )
    validation_context = evidence["validation_context"]
    if not isinstance(validation_context, Mapping) or not validation_context:
        raise ProgressError(
            "logical alias group payload.new_evidence.validation_context must be a non-empty object"
        )
    if validation_context.get("candidate_output_used") is not False:
        raise ProgressError(
            "logical alias group payload.new_evidence.validation_context requires "
            "candidate_output_used=false"
        )
    _reject_alias_evidence_identity_fields(
        provenance,
        label="logical alias group payload.new_evidence.provenance",
    )
    _reject_alias_evidence_identity_fields(
        validation_context,
        label="logical alias group payload.new_evidence.validation_context",
    )

    raw_artifacts = evidence["artifacts"]
    if not isinstance(raw_artifacts, list) or not raw_artifacts:
        raise ProgressError(
            "logical alias group payload.new_evidence.artifacts must be a non-empty array"
        )
    artifacts: list[dict[str, Any]] = []
    paths: set[str] = set()
    for index, raw_artifact in enumerate(raw_artifacts):
        label = f"logical alias group payload.new_evidence.artifacts[{index}]"
        if not isinstance(raw_artifact, Mapping):
            raise ProgressError(f"{label} must be an object")
        artifact = _require_exact_payload_fields(
            raw_artifact, {"path", "size"}, label=label
        )
        raw_path = artifact["path"]
        if (
            not isinstance(raw_path, str)
            or not raw_path
            or raw_path.strip() != raw_path
            or Path(raw_path).is_absolute()
            or ".." in Path(raw_path).parts
        ):
            raise ProgressError(
                f"{label}.path must be a trimmed repository-relative path without '..'"
            )
        path = Path(raw_path).as_posix()
        lower_path = path.lower()
        if path != "support/Recoil.exe" and not path.startswith(
            _LOGICAL_ALIAS_EVIDENCE_ROOT
        ):
            raise ProgressError(
                f"{label}.path must name immutable support/Recoil.exe or a durable "
                f"{_LOGICAL_ALIAS_EVIDENCE_ROOT} artifact"
            )
        if path != "support/Recoil.exe" and (
            "candidate" in lower_path
            or "vc5-final" in lower_path
            or "live-validation" in lower_path
            or Path(path).suffix.lower()
            in {
                ".asm",
                ".c",
                ".cod",
                ".cpp",
                ".dll",
                ".exe",
                ".ilk",
                ".lib",
                ".map",
                ".obj",
                ".pdb",
                ".res",
            }
        ):
            raise ProgressError(f"{label}.path must not reference a candidate output/artifact")
        size = artifact["size"]
        if not isinstance(size, int) or isinstance(size, bool) or size <= 0:
            raise ProgressError(f"{label}.size must be a positive integer")
        if path in paths:
            raise ProgressError(
                "logical alias group payload.new_evidence.artifacts contains "
                f"duplicate path {path!r}"
            )
        paths.add(path)
        artifacts.append({"path": path, "size": size})
    return {
        "summary": evidence["summary"],
        "provenance": deepcopy(dict(provenance)),
        "artifacts": artifacts,
        "validation_context": deepcopy(dict(validation_context)),
    }


def _parse_authored_icf_new_evidence(value: Any) -> dict[str, Any]:
    """Parse v4 evidence while keeping retail truth and candidate proof distinct."""

    if not isinstance(value, Mapping):
        raise ProgressError("logical alias group payload.new_evidence must be an object")
    evidence = _require_exact_payload_fields(
        value,
        _LOGICAL_ALIAS_NEW_EVIDENCE_FIELDS,
        label="logical alias group payload.new_evidence",
    )
    evidence["summary"] = _require_payload_string(
        evidence["summary"],
        label="logical alias group payload.new_evidence.summary",
    )
    provenance = evidence["provenance"]
    if not isinstance(provenance, Mapping) or not provenance:
        raise ProgressError(
            "authored ICF new_evidence.provenance must be a non-empty object"
        )
    if provenance.get("candidate_independent_retail_truth") is not True:
        raise ProgressError(
            "authored ICF evidence requires candidate_independent_retail_truth=true"
        )
    validation_context = evidence["validation_context"]
    if not isinstance(validation_context, Mapping) or not validation_context:
        raise ProgressError(
            "authored ICF new_evidence.validation_context must be a non-empty object"
        )
    if validation_context.get("candidate_output_used_as_expected") is not False:
        raise ProgressError(
            "authored ICF evidence requires candidate_output_used_as_expected=false"
        )
    if validation_context.get("candidate_output_used_for_mechanism_proof") is not True:
        raise ProgressError(
            "authored ICF evidence requires candidate_output_used_for_mechanism_proof=true"
        )

    raw_artifacts = evidence["artifacts"]
    if not isinstance(raw_artifacts, list) or not raw_artifacts:
        raise ProgressError(
            "authored ICF new_evidence.artifacts must be a non-empty array"
        )
    artifacts: list[dict[str, Any]] = []
    paths: set[str] = set()
    for index, raw_artifact in enumerate(raw_artifacts):
        label = f"logical alias group payload.new_evidence.artifacts[{index}]"
        if not isinstance(raw_artifact, Mapping):
            raise ProgressError(f"{label} must be an object")
        artifact = _require_exact_payload_fields(
            raw_artifact, {"path", "size", "role"}, label=label
        )
        raw_path = artifact["path"]
        if (
            not isinstance(raw_path, str)
            or not raw_path
            or raw_path.strip() != raw_path
            or Path(raw_path).is_absolute()
            or ".." in Path(raw_path).parts
        ):
            raise ProgressError(
                f"{label}.path must be a trimmed repository-relative path without '..'"
            )
        path = Path(raw_path).as_posix()
        role = artifact["role"]
        if role == "immutable-retail-truth":
            if path != "support/Recoil.exe":
                raise ProgressError(
                    f"{label} immutable-retail-truth must name support/Recoil.exe"
                )
        elif role == "candidate-mechanism-transcript":
            if (
                not path.startswith(_LOGICAL_ALIAS_EVIDENCE_ROOT)
                or Path(path).suffix.lower() not in {".json", ".txt", ".map"}
            ):
                raise ProgressError(
                    f"{label} candidate mechanism transcript must be durable JSON, "
                    f"text, or MAP evidence under {_LOGICAL_ALIAS_EVIDENCE_ROOT}"
                )
        else:
            raise ProgressError(
                f"{label}.role must be immutable-retail-truth or "
                "candidate-mechanism-transcript"
            )
        size = artifact["size"]
        if not isinstance(size, int) or isinstance(size, bool) or size <= 0:
            raise ProgressError(f"{label}.size must be a positive integer")
        if path in paths:
            raise ProgressError(f"authored ICF evidence contains duplicate path {path!r}")
        paths.add(path)
        artifacts.append({"path": path, "size": size, "role": role})
    if "support/Recoil.exe" not in paths:
        raise ProgressError("authored ICF evidence must include immutable support/Recoil.exe")
    if not any(row["role"] == "candidate-mechanism-transcript" for row in artifacts):
        raise ProgressError("authored ICF evidence requires candidate mechanism transcripts")
    return {
        "summary": evidence["summary"],
        "provenance": deepcopy(dict(provenance)),
        "artifacts": artifacts,
        "validation_context": deepcopy(dict(validation_context)),
    }


def _parse_authored_icf_selectors(value: Any, *, label: str) -> dict[str, Any]:
    selectors = _require_exact_payload_fields(
        value,
        {"direct_call_sites", "vtable_entries"},
        label=label,
    )
    raw_calls = selectors["direct_call_sites"]
    if not isinstance(raw_calls, list):
        raise ProgressError(f"{label}.direct_call_sites must be an array")
    calls = [
        _normalize_payload_address(item, label=f"{label}.direct_call_sites[{index}]")
        for index, item in enumerate(raw_calls)
    ]
    if len(calls) != len(set(calls)):
        raise ProgressError(f"{label}.direct_call_sites contains duplicates")
    raw_vtables = selectors["vtable_entries"]
    if not isinstance(raw_vtables, list):
        raise ProgressError(f"{label}.vtable_entries must be an array")
    vtables: list[dict[str, Any]] = []
    keys: set[tuple[str, int]] = set()
    for index, raw_selector in enumerate(raw_vtables):
        selector_label = f"{label}.vtable_entries[{index}]"
        selector = _require_exact_payload_fields(
            raw_selector,
            {"storage_identity", "slot_index", "entry_address"},
            label=selector_label,
        )
        selector["storage_identity"] = _require_payload_string(
            selector["storage_identity"], label=f"{selector_label}.storage_identity"
        )
        slot = selector["slot_index"]
        if not isinstance(slot, int) or isinstance(slot, bool) or slot < 0:
            raise ProgressError(f"{selector_label}.slot_index must be non-negative")
        entry_address = selector["entry_address"]
        if entry_address is not None:
            entry_address = _normalize_payload_address(
                entry_address, label=f"{selector_label}.entry_address"
            )
        key = (selector["storage_identity"], slot)
        if key in keys:
            raise ProgressError(f"{label}.vtable_entries contains duplicate selector {key!r}")
        keys.add(key)
        selector["entry_address"] = entry_address
        vtables.append(selector)
    if not calls and not vtables:
        raise ProgressError(
            f"{label} requires at least one immutable retail call-site or vtable selector"
        )
    return {"direct_call_sites": calls, "vtable_entries": vtables}


def _require_decorated_object_symbol(value: Any, *, label: str) -> str:
    symbol = _require_payload_string(value, label=label)
    if symbol[0] not in {"?", "_", "@"} or any(character.isspace() for character in symbol):
        raise ProgressError(
            f"{label} must be an exact decorated VC5 object symbol beginning with ?, _, or @"
        )
    return symbol


def _parse_logical_alias_group_payload(
    payload_json: str,
    *,
    source_label: str = "--payload-json",
) -> dict[str, Any]:
    try:
        raw_payload = json.loads(payload_json)
    except json.JSONDecodeError as exc:
        raise ProgressError(f"{source_label} is not valid JSON: {exc.msg}") from exc
    if not isinstance(raw_payload, Mapping):
        raise ProgressError(f"{source_label} must be one JSON object")
    schema = raw_payload.get("schema")
    if schema == _LOGICAL_ALIAS_GROUP_SCHEMA_V1:
        expected_fields = _LOGICAL_ALIAS_GROUP_FIELDS
    elif schema == _LOGICAL_ALIAS_GROUP_SCHEMA_V2:
        expected_fields = _LOGICAL_ALIAS_GROUP_V2_FIELDS
    elif schema == _LOGICAL_ALIAS_GROUP_SCHEMA_V3:
        expected_fields = _LOGICAL_ALIAS_GROUP_V3_FIELDS
    elif schema == _LOGICAL_ALIAS_GROUP_SCHEMA_V4:
        expected_fields = _LOGICAL_ALIAS_GROUP_V4_FIELDS
    else:
        raise ProgressError(
            "logical alias group payload schema must be "
            f"{_LOGICAL_ALIAS_GROUP_SCHEMA_V1!r}, "
            f"{_LOGICAL_ALIAS_GROUP_SCHEMA_V2!r}, or "
            f"{_LOGICAL_ALIAS_GROUP_SCHEMA_V3!r}, or "
            f"{_LOGICAL_ALIAS_GROUP_SCHEMA_V4!r}"
        )
    payload = _require_exact_payload_fields(
        raw_payload,
        expected_fields,
        label="logical alias group payload",
    )
    is_v2 = schema == _LOGICAL_ALIAS_GROUP_SCHEMA_V2
    is_v3 = schema == _LOGICAL_ALIAS_GROUP_SCHEMA_V3
    is_v4 = schema == _LOGICAL_ALIAS_GROUP_SCHEMA_V4
    if payload["reviewed"] is not True or payload["parent_reviewed"] is not True:
        raise ProgressError(
            "logical alias group payload requires reviewed=true and parent_reviewed=true"
        )
    payload["reason"] = _require_payload_string(
        payload["reason"], label="logical alias group payload.reason"
    )
    payload["symbol_id"] = _require_payload_string(
        payload["symbol_id"], label="logical alias group payload.symbol_id"
    )
    payload["address"] = _normalize_payload_address(
        payload["address"], label="logical alias group payload.address"
    )

    if not isinstance(payload["current"], Mapping):
        raise ProgressError("logical alias group payload.current must be an object")
    current = _require_exact_payload_fields(
        payload["current"],
        _LOGICAL_ALIAS_CURRENT_FIELDS,
        label="logical alias group payload.current",
    )
    for field in ("pipeline_class", "authored_order_role", "physical_block_id"):
        current[field] = _require_payload_string(
            current[field], label=f"logical alias group payload.current.{field}"
        )
    for field in ("linked_address_group", "icf_address_group", "logical_aliases"):
        if current[field] is not None and not isinstance(current[field], Mapping):
            raise ProgressError(
                f"logical alias group payload.current.{field} must be an object or null"
            )
        current[field] = deepcopy(current[field])
    payload["current"] = current

    if is_v3:
        if not isinstance(current["icf_address_group"], Mapping) or not isinstance(
            current["logical_aliases"], Mapping
        ):
            raise ProgressError(
                "logical alias group v3 is existing-group-only and requires exact "
                "current icf_address_group and logical_aliases objects"
            )
        payload["icf_address_group"] = deepcopy(current["icf_address_group"])
        payload["logical_aliases"] = deepcopy(current["logical_aliases"])

    if not isinstance(payload["icf_address_group"], Mapping):
        raise ProgressError("logical alias group payload.icf_address_group must be an object")
    if is_v4:
        group = _require_v2_generated_evidence_field(
            payload["icf_address_group"],
            required_fields=_ICF_ADDRESS_GROUP_V4_REQUIRED_FIELDS,
            label="logical alias group payload.icf_address_group",
        )
    elif is_v2:
        group = _require_v2_generated_evidence_field(
            payload["icf_address_group"],
            required_fields=_ICF_ADDRESS_GROUP_V2_REQUIRED_FIELDS,
            label="logical alias group payload.icf_address_group",
        )
    else:
        group = _require_exact_payload_fields(
            payload["icf_address_group"],
            _ICF_ADDRESS_GROUP_FIELDS,
            label="logical alias group payload.icf_address_group",
        )
    if group["winner_status"] not in {"selected-winner", "winner-unknown"}:
        raise ProgressError(
            "logical alias group payload.icf_address_group.winner_status must be "
            "'selected-winner' or 'winner-unknown'"
        )
    if group["winner_status"] == "selected-winner":
        group["winner_identity_key"] = _require_payload_string(
            group["winner_identity_key"],
            label="logical alias group payload.icf_address_group.winner_identity_key",
        )
    elif group["winner_identity_key"] is not None:
        raise ProgressError(
            "logical alias group payload.icf_address_group.winner_identity_key "
            "must be null when winner_status='winner-unknown'"
        )
    if not is_v2:
        if not is_v4:
            group["evidence_ids"] = _require_evidence_ids(
                group["evidence_ids"],
                label="logical alias group payload.icf_address_group.evidence_ids",
            )
    if is_v4:
        if group["model"] != AUTHORED_ICF_GROUP_MODEL:
            raise ProgressError(
                "logical alias group v4 icf_address_group.model must be "
                f"{AUTHORED_ICF_GROUP_MODEL!r}"
            )
        if group["physical_gate_symbol_id"] != payload["symbol_id"]:
            raise ProgressError(
                "logical alias group v4 physical_gate_symbol_id must identify the "
                "physical mutation target"
            )
    payload["icf_address_group"] = group

    raw_aliases = payload["logical_aliases"]
    if not isinstance(raw_aliases, Mapping) or len(raw_aliases) < 2:
        raise ProgressError(
            "logical alias group payload.logical_aliases must contain at least two keyed identities"
        )
    aliases: dict[str, dict[str, Any]] = {}
    object_symbols: set[str] = set()
    selected_winners: list[str] = []
    proven_aliases: list[str] = []
    identity_prefix = f"recoil:logical-function:{payload['address']}:"
    for raw_identity_key, raw_alias in raw_aliases.items():
        if not isinstance(raw_identity_key, str) or not raw_identity_key.startswith(identity_prefix):
            raise ProgressError(
                "logical alias identity keys must be address-qualified as "
                f"{identity_prefix}<name>"
            )
        if raw_identity_key == identity_prefix:
            raise ProgressError("logical alias identity keys require a non-empty name suffix")
        if not isinstance(raw_alias, Mapping):
            raise ProgressError(f"logical alias {raw_identity_key!r} must be an object")
        if is_v4:
            alias = _require_v2_generated_evidence_field(
                raw_alias,
                required_fields=_LOGICAL_ALIAS_V4_REQUIRED_FIELDS,
                label=f"logical alias {raw_identity_key!r}",
            )
        elif is_v2:
            alias = _require_v2_generated_evidence_field(
                raw_alias,
                required_fields=_LOGICAL_ALIAS_V2_REQUIRED_FIELDS,
                label=f"logical alias {raw_identity_key!r}",
            )
        else:
            alias = _require_exact_payload_fields(
                raw_alias,
                _LOGICAL_ALIAS_FIELDS,
                label=f"logical alias {raw_identity_key!r}",
            )
        alias["object_symbol"] = _require_decorated_object_symbol(
            alias["object_symbol"],
            label=f"logical alias {raw_identity_key!r}.object_symbol",
        )
        if is_v3 and not _decorated_coff_name(alias["object_symbol"]):
            raise ProgressError(
                f"logical alias {raw_identity_key!r}.object_symbol must be one exact "
                "stable decorated VC5 symbol"
            )
        if alias["object_symbol"] in object_symbols:
            raise ProgressError(
                f"logical aliases contain duplicate object_symbol {alias['object_symbol']!r}"
            )
        object_symbols.add(alias["object_symbol"])
        alias["original_name"] = _require_payload_string(
            alias["original_name"],
            label=f"logical alias {raw_identity_key!r}.original_name",
        )
        allowed_name_statuses = (
            {"recovered", "provisional"}
            if is_v3 or is_v4
            else {"recovered"}
        )
        if alias["original_name_status"] not in allowed_name_statuses:
            raise ProgressError(
                f"logical alias {raw_identity_key!r}.original_name_status must be "
                + (
                    "the preserved existing value 'recovered' or 'provisional'"
                    if is_v3 or is_v4
                    else "'recovered'"
                )
            )
        if alias["source_owner_status"] != "authored-owner":
            raise ProgressError(
                f"logical alias {raw_identity_key!r}.source_owner_status must be 'authored-owner'"
            )
        alias["owner_id"] = _require_payload_string(
            alias["owner_id"], label=f"logical alias {raw_identity_key!r}.owner_id"
        )
        alias_classification = (
            alias["pipeline_class"],
            alias["authored_order_role"],
        )
        if alias_classification not in {
            ("authored", "authored-body"),
            ("authored-lifecycle", "authored-lifecycle-body"),
        }:
            raise ProgressError(
                f"logical alias {raw_identity_key!r} must use one exact authored "
                "classification pair: pipeline_class='authored' with "
                "authored_order_role='authored-body', or "
                "pipeline_class='authored-lifecycle' with "
                "authored_order_role='authored-lifecycle-body'"
            )
        validate_authored_order_role(
            str(alias["pipeline_class"]), str(alias["authored_order_role"])
        )
        fold_status = alias["fold_status"]
        if fold_status == "selected-winner":
            selected_winners.append(raw_identity_key)
        elif fold_status == "proven-fold-alias":
            proven_aliases.append(raw_identity_key)
        else:
            raise ProgressError(
                f"logical alias {raw_identity_key!r}.fold_status must be 'selected-winner' "
                "or 'proven-fold-alias'"
            )
        if is_v4:
            if alias["gate_mode"] != AUTHORED_ICF_MEMBER_GATE_MODE:
                raise ProgressError(
                    f"logical alias {raw_identity_key!r}.gate_mode must be "
                    f"{AUTHORED_ICF_MEMBER_GATE_MODE!r}"
                )
            try:
                source_trace = normalize_source_traceability(
                    alias["source_traceability"]
                )
            except ValueError as exc:
                raise ProgressError(
                    f"logical alias {raw_identity_key!r} has invalid source_traceability: {exc}"
                ) from exc
            defining_edges = [
                edge
                for edge in source_trace["source_edges"]
                if edge["relation"] == "defines"
            ]
            if source_trace["state"] != "resolved" or len(defining_edges) != 1:
                raise ProgressError(
                    f"logical alias {raw_identity_key!r} requires one exact resolved "
                    "defines source edge"
                )
            if len(source_trace["source_edges"]) != 1:
                raise ProgressError(
                    f"logical alias {raw_identity_key!r} source trace must be exclusive"
                )
            if source_trace["source_edges"][0]["evidence_ids"]:
                raise ProgressError(
                    f"logical alias {raw_identity_key!r} v4 source-edge evidence_ids "
                    "are generated by the command and must be empty"
                )
            alias["source_traceability"] = source_trace
            alias["retail_target_selectors"] = _parse_authored_icf_selectors(
                alias["retail_target_selectors"],
                label=f"logical alias {raw_identity_key!r}.retail_target_selectors",
            )
        elif not is_v2:
            alias["evidence_ids"] = _require_evidence_ids(
                alias["evidence_ids"],
                label=f"logical alias {raw_identity_key!r}.evidence_ids",
            )
        aliases[raw_identity_key] = alias

    if group["winner_status"] == "selected-winner":
        if len(selected_winners) != 1:
            raise ProgressError(
                "selected-winner logical alias group must contain exactly one "
                "selected-winner identity"
            )
        if not proven_aliases:
            raise ProgressError(
                "selected-winner logical alias group must contain at least one "
                "proven-fold-alias identity"
            )
        if group["winner_identity_key"] != selected_winners[0]:
            raise ProgressError(
                "icf_address_group.winner_identity_key must identify the sole "
                "selected-winner alias"
            )
    elif selected_winners or len(proven_aliases) != len(aliases):
        raise ProgressError(
            "winner-unknown logical alias group requires winner_identity_key=null "
            "and every logical alias fold_status='proven-fold-alias'"
        )
    if is_v3 and group["winner_status"] != "winner-unknown":
        raise ProgressError(
            "logical alias group v3 refresh requires an existing winner-unknown group"
        )
    if is_v4:
        owner_ids = [str(alias["owner_id"]) for alias in aliases.values()]
        if len(owner_ids) != len(set(owner_ids)):
            raise ProgressError(
                "logical alias group v4 requires one exclusive authored owner per logical member"
            )
        source_edges: set[tuple[str, str]] = set()
        direct_call_sites: set[str] = set()
        vtable_selectors: set[tuple[str, int]] = set()
        for identity_key, alias in aliases.items():
            edge = alias["source_traceability"]["source_edges"][0]
            edge_key = (
                str(edge["anchor_id"]),
                str(edge["emission_context"]["translation_unit"]),
            )
            if edge_key in source_edges:
                raise ProgressError(
                    "logical alias group v4 source definitions must be exclusive; "
                    f"duplicate edge {edge_key!r}"
                )
            source_edges.add(edge_key)
            selectors = alias["retail_target_selectors"]
            for call_site in selectors["direct_call_sites"]:
                if call_site in direct_call_sites:
                    raise ProgressError(
                        f"logical alias group v4 direct call selector {call_site} is ambiguous"
                    )
                direct_call_sites.add(call_site)
            for selector in selectors["vtable_entries"]:
                key = (selector["storage_identity"], selector["slot_index"])
                if key in vtable_selectors:
                    raise ProgressError(
                        f"logical alias group v4 vtable selector {key!r} is ambiguous"
                    )
                vtable_selectors.add(key)
        payload["authored_icf_proof"] = validate_authored_icf_proof(
            payload["authored_icf_proof"],
            physical_address=payload["address"],
            aliases=aliases,
        )
    payload["logical_aliases"] = aliases
    if is_v4:
        payload["new_evidence"] = _parse_authored_icf_new_evidence(
            payload["new_evidence"]
        )
    elif is_v2 or is_v3:
        payload["new_evidence"] = _parse_logical_alias_new_evidence(
            payload["new_evidence"]
        )
    return payload


def _load_logical_alias_group_payload(args: argparse.Namespace) -> dict[str, Any]:
    if args.payload_file is None:
        return _parse_logical_alias_group_payload(str(args.payload_json))

    payload_path = args.payload_file
    build_root = (REPO_ROOT / "build").resolve()
    resolved_path = (
        (REPO_ROOT / payload_path).resolve()
        if not payload_path.is_absolute()
        else payload_path.resolve()
    )
    try:
        resolved_path.relative_to(build_root)
    except ValueError as exc:
        raise ProgressError(
            "logical alias group --payload-file must resolve under workspace build/"
        ) from exc
    try:
        file_size = resolved_path.stat().st_size
    except OSError as exc:
        raise ProgressError(
            "logical alias group payload file is missing or unreadable: "
            f"{payload_path}"
        ) from exc
    if not resolved_path.is_file():
        raise ProgressError(
            f"logical alias group payload file is not a regular file: {payload_path}"
        )
    if file_size > MAX_PROGRESS_PAYLOAD_FILE_BYTES:
        raise ProgressError(
            "logical alias group payload file exceeds the "
            f"{MAX_PROGRESS_PAYLOAD_FILE_BYTES}-byte limit: {payload_path}"
        )
    try:
        payload_json = resolved_path.read_bytes().decode("utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        if isinstance(exc, UnicodeDecodeError):
            message = "is not valid UTF-8"
        else:
            message = "is unreadable"
        raise ProgressError(
            f"logical alias group payload file {message}: {payload_path}"
        ) from exc
    return _parse_logical_alias_group_payload(
        payload_json,
        source_label="--payload-file",
    )


def _logical_alias_v3_governed_target(
    data: dict[str, Any],
    *,
    address: str,
    aliases: Mapping[str, Mapping[str, Any]],
) -> str:
    """Require one synchronized complete VC5 authority for an existing group."""
    targets = data.get("verification_targets")
    if not isinstance(targets, Mapping):
        raise ProgressError(
            "logical alias group v3 requires the verification-target collection"
        )
    alias_ids = set(aliases)
    matching_target_ids: list[str] = []
    for target_id, target in targets.items():
        if (
            not isinstance(target, Mapping)
            or target.get("binary") != "recoil"
            or target.get("kind") != "vc5"
        ):
            continue
        registration = target.get("registration")
        if not isinstance(registration, Mapping):
            continue
        rows = _target_functions(registration, "authored-function-order")
        if any(row.get("logical_identity_key") in alias_ids for row in rows):
            matching_target_ids.append(str(target_id))
    if len(matching_target_ids) != 1:
        raise ProgressError(
            "logical alias group v3 requires exactly one governed VC5 target; found "
            + str(len(matching_target_ids))
        )

    target_id = matching_target_ids[0]
    target = _synchronized_order_target(ProgressDocument(data), target_id)
    registration = target.get("registration")
    if not isinstance(registration, Mapping):
        raise ProgressError(
            f"logical alias group v3 target {target_id!r} has no registration"
        )
    rows = _target_functions(registration, "authored-function-order")
    selected_rows = [
        row for row in rows if row.get("logical_identity_key") in alias_ids
    ]
    address_rows = [
        row
        for row in rows
        if isinstance(row.get("address"), str)
        and normalize_address(str(row["address"])) == address
        and isinstance(row.get("logical_identity_key"), str)
        and row.get("logical_identity_key")
    ]
    if (
        len(selected_rows) != len(alias_ids)
        or {str(row.get("logical_identity_key")) for row in selected_rows}
        != alias_ids
        or len(address_rows) != len(alias_ids)
        or {str(row.get("logical_identity_key")) for row in address_rows}
        != alias_ids
    ):
        raise ProgressError(
            f"logical alias group v3 target {target_id!r} is not the complete exact "
            f"governed alias population at {address}"
        )

    by_identity = {
        str(row["logical_identity_key"]): row for row in selected_rows
    }
    for alias_id, alias in aliases.items():
        row = by_identity[alias_id]
        alternatives = _finite_literal_symbol_regex_alternatives(
            row.get("symbol_regex")
        )
        registered_symbols = (
            {str(row.get("symbol"))}
            if isinstance(row.get("symbol"), str) and row.get("symbol")
            else set(alternatives or ())
        )
        if (
            normalize_address(str(row.get("address", ""))) != address
            or row.get("pipeline_class") != alias.get("pipeline_class")
            or row.get("authored_order_role")
            != alias.get("authored_order_role")
            or row.get("authored_order_gate") is not True
            or row.get("required_presence") is not True
            or row.get("icf_fold_status") != "proven-fold-alias"
            or alias.get("object_symbol") not in registered_symbols
        ):
            raise ProgressError(
                f"logical alias group v3 target {target_id!r} has stale, patterned, "
                f"or incomplete exact decorated authority for {alias_id!r}"
            )
    return target_id


def _set_symbol_logical_alias_group(
    data: dict[str, Any],
    payload: Mapping[str, Any],
) -> dict[str, Any]:
    symbols = data.get("symbols")
    blocks = data.get("physical_blocks")
    evidence = data.get("evidence")
    owners = data.get("owners")
    if (
        not isinstance(symbols, dict)
        or not isinstance(blocks, dict)
        or not isinstance(evidence, dict)
        or not isinstance(owners, dict)
    ):
        raise ProgressError(
            "logical alias group mutation requires tracker symbol, physical-block, evidence, "
            "and owner collections"
        )
    symbol_id = str(payload["symbol_id"])
    row = symbols.get(symbol_id)
    if not isinstance(row, dict):
        raise ProgressError(f"unknown function symbol {symbol_id!r}")
    if row.get("binary") != "recoil" or row.get("kind") != "function":
        raise ProgressError(
            f"logical alias group target {symbol_id!r} must be an existing Recoil function row"
        )
    stored_address = row.get("address")
    if not isinstance(stored_address, str) or normalize_address(stored_address) != payload["address"]:
        raise ProgressError(
            f"logical alias group target {symbol_id!r} address is stale: "
            f"expected {payload['address']}, found {stored_address!r}"
        )

    current = payload["current"]
    current_actual = {
        "pipeline_class": row.get("pipeline_class"),
        "authored_order_role": row.get("authored_order_role"),
        "physical_block_id": row.get("physical_block_id"),
        "linked_address_group": deepcopy(row.get("linked_address_group")),
        "icf_address_group": deepcopy(row.get("icf_address_group")),
        "logical_aliases": deepcopy(row.get("logical_aliases")),
    }
    if current_actual != current:
        raise ProgressError(
            f"logical alias group target {symbol_id!r} current state is stale: "
            f"expected {json.dumps(current, sort_keys=True)}, "
            f"found {json.dumps(current_actual, sort_keys=True)}"
        )
    is_v4 = payload["schema"] == _LOGICAL_ALIAS_GROUP_SCHEMA_V4
    if is_v4:
        physical_classification = (
            current_actual["pipeline_class"],
            current_actual["authored_order_role"],
        )
        if physical_classification not in {
            ("authored", "authored-body"),
            ("authored-lifecycle", "authored-lifecycle-body"),
        }:
            raise ProgressError(
                f"logical alias group v4 target {symbol_id!r} must preserve one "
                "authored physical gating body"
            )
    elif (
        current_actual["pipeline_class"] != "non-authored"
        or current_actual["authored_order_role"] != "compiler-generated-icf-representative"
    ):
        raise ProgressError(
            f"logical alias group target {symbol_id!r} must already be classified as "
            "pipeline_class='non-authored' and "
            "authored_order_role='compiler-generated-icf-representative'"
        )
    if current_actual["linked_address_group"] is not None:
        raise ProgressError(
            f"logical alias group target {symbol_id!r} already has a conflicting linked_address_group"
        )
    block_id = str(current_actual["physical_block_id"])
    block = blocks.get(block_id)
    if not isinstance(block, Mapping):
        raise ProgressError(
            f"logical alias group target {symbol_id!r} references unknown physical block {block_id!r}"
        )
    if block.get("binary") != "recoil" or symbol_id not in block.get("contribution_ids", []):
        raise ProgressError(
            f"logical alias group target {symbol_id!r} is not an exact contribution of "
            f"physical block {block_id!r}"
        )

    is_v2 = payload["schema"] == _LOGICAL_ALIAS_GROUP_SCHEMA_V2
    is_v3 = payload["schema"] == _LOGICAL_ALIAS_GROUP_SCHEMA_V3
    if not is_v2 and not is_v4:
        referenced_evidence_ids = {
            str(evidence_id)
            for evidence_id in payload["icf_address_group"]["evidence_ids"]
        }
        referenced_evidence_ids.update(
            str(evidence_id)
            for alias in payload["logical_aliases"].values()
            for evidence_id in alias["evidence_ids"]
        )
        unknown_evidence_ids = sorted(referenced_evidence_ids - set(evidence))
        if unknown_evidence_ids:
            raise ProgressError(
                "logical alias group references unknown tracker evidence ids: "
                + ", ".join(unknown_evidence_ids)
            )
    owner_ids: set[str] = set()
    for identity_key, alias in payload["logical_aliases"].items():
        owner_id = str(alias["owner_id"])
        owner_ids.add(owner_id)
        owner = owners.get(owner_id)
        if not isinstance(owner, Mapping):
            raise ProgressError(
                f"logical alias {identity_key!r} references unknown owner {owner_id!r}"
            )
        gates = owner.get("gates")
        provider_state = owner.get("provider_state")
        if (
            owner.get("binary") != "recoil"
            or owner.get("kind") == "provider-boundary"
            or provider_state in {"accepted", "provider-boundary", "provider-owned"}
            or not isinstance(gates, Mapping)
            or gates.get("source") != "accepted"
            or gates.get("owner_linkage") != "accepted"
        ):
            raise ProgressError(
                f"logical alias {identity_key!r} owner {owner_id!r} must be an existing "
                "non-provider Recoil owner with accepted source and owner_linkage gates"
            )

    if is_v4:
        physical_primary_owners: list[str] = []
        for owner_id, owner in owners.items():
            if not isinstance(owner, Mapping) or owner.get("binary") != "recoil":
                continue
            relationships = owner.get("relationships")
            if not isinstance(relationships, list):
                raise ProgressError(
                    f"logical alias group v4 owner {owner_id!r} relationships must be an array"
                )
            if any(
                isinstance(relationship, Mapping)
                and relationship.get("kind") == "primary-function"
                and relationship.get("symbol_id") == symbol_id
                and isinstance(relationship.get("address"), str)
                and normalize_address(str(relationship["address"])) == payload["address"]
                for relationship in relationships
            ):
                physical_primary_owners.append(str(owner_id))
        if len(physical_primary_owners) != 1:
            raise ProgressError(
                "logical alias group v4 requires exactly one address-exclusive physical "
                f"primary owner; found {len(physical_primary_owners)}"
            )
        if physical_primary_owners[0] not in owner_ids:
            raise ProgressError(
                "logical alias group v4 physical primary owner must own exactly one "
                "logical member"
            )
        validate_authored_icf_source_mirrors(payload["logical_aliases"])

    governed_target_id: str | None = None
    if is_v3:
        governed_target_id = _logical_alias_v3_governed_target(
            data,
            address=str(payload["address"]),
            aliases=payload["logical_aliases"],
        )

    replacement_group = deepcopy(dict(payload["icf_address_group"]))
    replacement_aliases = deepcopy(dict(payload["logical_aliases"]))
    if not is_v2 and not is_v3 and (
        current_actual["icf_address_group"] == replacement_group
        and current_actual["logical_aliases"] == replacement_aliases
    ):
        raise ProgressError(
            f"logical alias group target {symbol_id!r} already has the requested current values"
        )
    evidence_id: str | None = None
    evidence_scope_ids: list[str] = []
    if is_v2 or is_v3 or is_v4:
        new_evidence = payload["new_evidence"]
        for artifact in new_evidence["artifacts"]:
            if artifact["path"] == "support/Recoil.exe":
                artifact_path = MACHINE_RETAIL_REFERENCE.resolve()
            else:
                artifact_path = (REPO_ROOT / artifact["path"]).resolve()
                try:
                    artifact_path.relative_to(REPO_ROOT.resolve())
                except ValueError as exc:
                    raise ProgressError(
                        "logical alias group evidence artifact escapes repository: "
                        f"{artifact['path']!r}"
                    ) from exc
            if not artifact_path.is_file():
                raise ProgressError(
                    "logical alias group evidence artifact does not exist: "
                    f"{artifact['path']!r}"
                )
            actual_size = artifact_path.stat().st_size
            if actual_size != artifact["size"]:
                raise ProgressError(
                    "logical alias group evidence artifact size changed for "
                    f"{artifact['path']!r}: expected {artifact['size']}, "
                    f"found {actual_size}"
                )
        provenance = deepcopy(dict(new_evidence["provenance"]))
        if "validation_context" in provenance:
            raise ProgressError(
                "logical alias group payload.new_evidence.provenance must not "
                "supply validation_context separately"
            )
        validation_context = deepcopy(dict(new_evidence["validation_context"]))
        if is_v3:
            derived_provenance_fields = {
                "evidence_contract",
                "governed_target_id",
            }
            derived_context_fields = {
                "authority_scope",
                "original_name_used_as_authority",
                "winner_identity_claimed",
            }
            if derived_provenance_fields & set(provenance):
                raise ProgressError(
                    "logical alias group v3 new_evidence must not supply derived "
                    "evidence_contract or governed_target_id"
                )
            if derived_context_fields & set(validation_context):
                raise ProgressError(
                    "logical alias group v3 new_evidence must not supply derived "
                    "physical-group authority fields"
                )
            provenance["evidence_contract"] = (
                _LOGICAL_ALIAS_V3_EVIDENCE_CONTRACT
            )
            provenance["governed_target_id"] = governed_target_id
            validation_context.update(
                {
                    "authority_scope": _LOGICAL_ALIAS_V3_AUTHORITY_SCOPE,
                    "original_name_used_as_authority": False,
                    "winner_identity_claimed": False,
                }
            )
        if is_v4:
            derived_provenance_fields = {
                "evidence_contract",
                "physical_gate_symbol_id",
                "authored_icf_proof",
            }
            if derived_provenance_fields & set(provenance):
                raise ProgressError(
                    "logical alias group v4 new_evidence must not supply derived "
                    "evidence contract, physical gate, or proof fields"
                )
            proof = deepcopy(dict(payload["authored_icf_proof"]))
            candidate = proof["candidate_mechanism"]
            proof_paths = {
                str(candidate["icf_link"]["transcript_path"]),
                str(candidate["noicf_link"]["transcript_path"]),
                str(candidate["negative_control"]["object_report_path"]),
                *(
                    str(member["object_report_path"])
                    for member in candidate["object_members"].values()
                ),
                *(
                    str(observation["object_report_path"])
                    for observations in candidate["selector_bindings"].values()
                    for observation in observations
                ),
            }
            artifact_paths = {
                str(artifact["path"])
                for artifact in new_evidence["artifacts"]
                if artifact["role"] == "candidate-mechanism-transcript"
            }
            if proof_paths != artifact_paths:
                raise ProgressError(
                    "logical alias group v4 proof transcripts must exactly match the "
                    "candidate-mechanism evidence artifacts"
                )
            provenance.update(
                {
                    "evidence_contract": "authored-linker-coalesced-logical-members-v1",
                    "physical_gate_symbol_id": symbol_id,
                    "authored_icf_proof": proof,
                }
            )
            validation_context.update(
                {
                    "authority_scope": "retail-logical-selection-and-candidate-fold-mechanism",
                    "candidate_addresses_used_as_retail_truth": False,
                    "physical_gate_count": 1,
                }
            )
        provenance["validation_context"] = validation_context
        evidence_scope_ids = sorted(
            {symbol_id, *replacement_aliases.keys(), *owner_ids}
        )
        evidence_id = add_live_evidence(
            data,
            kind=(
                _AUTHORED_ICF_EVIDENCE_KIND
                if is_v4
                else _LOGICAL_ALIAS_EVIDENCE_KIND
            ),
            summary=str(new_evidence["summary"]),
            scope_ids=evidence_scope_ids,
            provenance=provenance,
        )
        created_evidence = evidence.get(evidence_id)
        if not isinstance(created_evidence, dict):
            raise ProgressError(
                f"logical alias group evidence allocation did not create {evidence_id!r}"
            )
        created_evidence["artifacts"] = deepcopy(new_evidence["artifacts"])
        replacement_group["evidence_ids"] = [evidence_id]
        for alias in replacement_aliases.values():
            alias["evidence_ids"] = [evidence_id]
            if is_v4:
                for source_edge in alias["source_traceability"]["source_edges"]:
                    source_edge["evidence_ids"] = [evidence_id]
    if is_v3:
        current_group_non_evidence = {
            key: deepcopy(value)
            for key, value in current_actual["icf_address_group"].items()
            if key != "evidence_ids"
        }
        replacement_group_non_evidence = {
            key: deepcopy(value)
            for key, value in replacement_group.items()
            if key != "evidence_ids"
        }
        current_aliases_non_evidence = {
            alias_id: {
                key: deepcopy(value)
                for key, value in alias.items()
                if key != "evidence_ids"
            }
            for alias_id, alias in current_actual["logical_aliases"].items()
        }
        replacement_aliases_non_evidence = {
            alias_id: {
                key: deepcopy(value)
                for key, value in alias.items()
                if key != "evidence_ids"
            }
            for alias_id, alias in replacement_aliases.items()
        }
        if (
            current_group_non_evidence != replacement_group_non_evidence
            or current_aliases_non_evidence != replacement_aliases_non_evidence
        ):
            raise ProgressError(
                "logical alias group v3 deep-CAS forbids creating, renaming, or "
                "changing any non-evidence group/alias field"
            )
    row["icf_address_group"] = replacement_group
    row["logical_aliases"] = replacement_aliases
    invalidated = invalidate_order_dependencies(
        data,
        block_ids=[block_id],
        symbol_ids=[symbol_id],
    )
    row["accepted_order_facts"] = None
    row["accepted_byte_facts"] = None
    result = {
        "kind": "symbol-logical-alias-group",
        "schema": str(payload["schema"]),
        "reviewed": True,
        "parent_reviewed": True,
        "reason": str(payload["reason"]),
        "symbol_id": symbol_id,
        "address": str(payload["address"]),
        "physical_block_id": block_id,
        "changed_fields": (
            ["evidence", "icf_address_group", "logical_aliases"]
            if is_v2 or is_v3 or is_v4
            else ["icf_address_group", "logical_aliases"]
        ),
        "winner_status": replacement_group["winner_status"],
        "selected_winner_identity_key": replacement_group["winner_identity_key"],
        "logical_identity_keys": list(replacement_aliases),
        "proven_fold_alias_count": sum(
            alias["fold_status"] == "proven-fold-alias"
            for alias in replacement_aliases.values()
        ),
        "invalidated": invalidated,
    }
    if is_v2 or is_v3 or is_v4:
        result.update(
            {
                "evidence_created": True,
                "evidence_id": evidence_id,
                "evidence_scope_ids": evidence_scope_ids,
            }
        )
    if is_v3:
        result.update(
            {
                "existing_group_only": True,
                "governed_target_id": governed_target_id,
                "authority_scope": _LOGICAL_ALIAS_V3_AUTHORITY_SCOPE,
                "non_evidence_fields_preserved": True,
            }
        )
    if is_v4:
        result.update(
            {
                "model": AUTHORED_ICF_GROUP_MODEL,
                "physical_authored_gate_preserved": True,
                "physical_primary_owner_id": physical_primary_owners[0],
                "logical_owner_edges_exclusive": True,
                "logical_source_edges_exclusive": True,
                "retail_selector_truth_candidate_independent": True,
                "candidate_mechanism_proof_required": True,
            }
        )
    return result


_PHYSICAL_BLOCK_REPLACE_SCHEMA = "recoil-physical-block-replace-v1"
_PHYSICAL_BLOCK_REPLACE_FIELDS = {
    "schema",
    "reviewed",
    "parent_reviewed",
    "reason",
    "binary",
    "current_block",
    "replacement_blocks",
    "replacement_semantic_spans",
}
_CURRENT_BLOCK_GUARD_FIELDS = {
    "id",
    "start",
    "end_exclusive",
    "source_path",
    "agent_source_path",
    "original_source_path",
    "provisional_original_path",
    "mapping_state",
    "mapping_status",
    "contribution_ids",
    "semantic_span_ids",
    "source_shape_inputs",
    "candidate_header_contributors",
}
_REPLACEMENT_BLOCK_FIELDS = {
    "id",
    "start",
    "end_exclusive",
    "source_path",
    "agent_source_path",
    "original_source_path",
    "provisional_original_path",
    "mapping_state",
    "mapping_status",
    "mapping_confidence",
    "contribution_ids",
    "semantic_span_ids",
    "source_shape_inputs",
    "candidate_header_contributors",
}
_REPLACEMENT_SEMANTIC_SPAN_FIELDS = {
    "id",
    "start",
    "end_exclusive",
    "physical_block_id",
    "source_path",
    "status",
    "confidence",
    "symbol_ids",
}
_BLOCK_RELATIONSHIP_COLLECTIONS = (
    "owners",
    "verification_targets",
    "blockers",
    "storage_contributions",
    "output_sections",
)

_FUNCTION_PADDING_CORRECTION_SCHEMA = "recoil-function-padding-correction-v1"
_FUNCTION_PADDING_CORRECTION_FIELDS = {
    "schema",
    "reviewed",
    "parent_reviewed",
    "reason",
    "binary",
    "current_function",
    "current_block",
    "current_semantic_span",
    "replacement_padding",
}
_FUNCTION_PADDING_ENTITY_GUARD_FIELDS = {"id", "record"}
_FUNCTION_PADDING_BLOCK_GUARD_FIELDS = {
    "id",
    "binary",
    "start",
    "end_exclusive",
    "row_kind",
    "contribution_kind",
    "source_path",
    "agent_source_path",
    "original_source_path",
    "provisional_original_path",
    "mapping_state",
    "mapping_status",
    "accepted_order_facts",
    "expected_contribution_count",
    "expected_contains_function_id",
    "expected_function_membership_count",
    "expected_semantic_span_count",
    "expected_contains_semantic_span_id",
    "expected_semantic_span_membership_count",
    "source_shape_input_count",
    "candidate_header_contributor_count",
}
_FUNCTION_PADDING_REPLACEMENT_FIELDS = {
    "start",
    "end_exclusive",
    "retail_bytes_hex",
    "keep_physical_block_id",
    "keep_semantic_span_id",
    "replacement_symbol_ids",
    "remove_function_id",
}


def _parse_function_padding_correction_payload(payload_json: str) -> dict[str, Any]:
    try:
        raw = json.loads(payload_json)
    except json.JSONDecodeError as exc:
        raise ProgressError(f"--payload-json is not valid JSON: {exc.msg}") from exc
    if not isinstance(raw, Mapping):
        raise ProgressError("--payload-json must be one function-padding correction object")
    payload = _require_exact_payload_fields(
        raw,
        _FUNCTION_PADDING_CORRECTION_FIELDS,
        label="function-padding correction payload",
    )
    if payload["schema"] != _FUNCTION_PADDING_CORRECTION_SCHEMA:
        raise ProgressError(
            f"function-padding correction schema must be "
            f"{_FUNCTION_PADDING_CORRECTION_SCHEMA!r}"
        )
    if payload["reviewed"] is not True or payload["parent_reviewed"] is not True:
        raise ProgressError(
            "function-padding correction requires reviewed=true and parent_reviewed=true"
        )
    payload["reason"] = _require_payload_string(payload["reason"], label="reason")
    payload["binary"] = _require_payload_string(payload["binary"], label="binary")
    if payload["binary"] != "recoil":
        raise ProgressError("function-padding correction is limited to binary='recoil'")

    for field in ("current_function", "current_semantic_span"):
        raw_guard = payload[field]
        if not isinstance(raw_guard, Mapping):
            raise ProgressError(f"{field} must be an object")
        guard = _require_exact_payload_fields(
            raw_guard,
            _FUNCTION_PADDING_ENTITY_GUARD_FIELDS,
            label=field,
        )
        guard["id"] = _require_payload_string(guard["id"], label=f"{field}.id")
        if not isinstance(guard["record"], Mapping):
            raise ProgressError(f"{field}.record must be an exact current row object")
        guard["record"] = deepcopy(dict(guard["record"]))
        payload[field] = guard

    raw_block = payload["current_block"]
    if not isinstance(raw_block, Mapping):
        raise ProgressError("current_block must be an object")
    block = _require_exact_payload_fields(
        raw_block,
        _FUNCTION_PADDING_BLOCK_GUARD_FIELDS,
        label="current_block",
    )
    for field in (
        "id",
        "binary",
        "source_path",
        "agent_source_path",
        "mapping_state",
        "mapping_status",
        "expected_contains_function_id",
        "expected_contains_semantic_span_id",
    ):
        block[field] = _require_payload_string(
            block[field],
            label=f"current_block.{field}",
        )
    for field in ("row_kind", "contribution_kind"):
        if block[field] is not None and not isinstance(block[field], str):
            raise ProgressError(f"current_block.{field} must be a string or null")
    for field in ("original_source_path", "provisional_original_path"):
        if block[field] is not None and not isinstance(block[field], str):
            raise ProgressError(f"current_block.{field} must be a string or null")
    block["start"] = _normalize_payload_address(
        block["start"],
        label="current_block.start",
    )
    block["end_exclusive"] = _normalize_payload_address(
        block["end_exclusive"],
        label="current_block.end_exclusive",
    )
    for field in (
        "expected_contribution_count",
        "expected_function_membership_count",
        "expected_semantic_span_count",
        "expected_semantic_span_membership_count",
        "source_shape_input_count",
        "candidate_header_contributor_count",
    ):
        value = block[field]
        if not isinstance(value, int) or isinstance(value, bool) or value < 0:
            raise ProgressError(f"current_block.{field} must be a non-negative integer")
    payload["current_block"] = block

    raw_replacement = payload["replacement_padding"]
    if not isinstance(raw_replacement, Mapping):
        raise ProgressError("replacement_padding must be an object")
    replacement = _require_exact_payload_fields(
        raw_replacement,
        _FUNCTION_PADDING_REPLACEMENT_FIELDS,
        label="replacement_padding",
    )
    replacement["start"] = _normalize_payload_address(
        replacement["start"],
        label="replacement_padding.start",
    )
    replacement["end_exclusive"] = _normalize_payload_address(
        replacement["end_exclusive"],
        label="replacement_padding.end_exclusive",
    )
    for field in (
        "retail_bytes_hex",
        "keep_physical_block_id",
        "keep_semantic_span_id",
        "remove_function_id",
    ):
        replacement[field] = _require_payload_string(
            replacement[field],
            label=f"replacement_padding.{field}",
        )
    replacement["retail_bytes_hex"] = replacement["retail_bytes_hex"].lower()
    try:
        bytes.fromhex(replacement["retail_bytes_hex"])
    except ValueError as exc:
        raise ProgressError(
            "replacement_padding.retail_bytes_hex must be an even-length hexadecimal string"
        ) from exc
    if (
        len(replacement["retail_bytes_hex"]) % 2
        or not replacement["retail_bytes_hex"]
        or any(char not in "0123456789abcdef" for char in replacement["retail_bytes_hex"])
    ):
        raise ProgressError(
            "replacement_padding.retail_bytes_hex must be a non-empty contiguous "
            "even-length hexadecimal string"
        )
    replacement["replacement_symbol_ids"] = _require_unique_string_list(
        replacement["replacement_symbol_ids"],
        label="replacement_padding.replacement_symbol_ids",
        allow_empty=True,
    )
    if replacement["replacement_symbol_ids"]:
        raise ProgressError(
            "replacement_padding.replacement_symbol_ids must be [] for a padding correction"
        )
    payload["replacement_padding"] = replacement
    return payload


def _require_exact_payload_fields(
    value: Mapping[str, Any],
    expected: set[str],
    *,
    label: str,
) -> dict[str, Any]:
    item = dict(value)
    missing = sorted(expected - set(item))
    extra = sorted(set(item) - expected)
    if missing or extra:
        details: list[str] = []
        if missing:
            details.append("missing " + ", ".join(missing))
        if extra:
            details.append("unsupported " + ", ".join(extra))
        raise ProgressError(f"{label} has invalid fields: " + "; ".join(details))
    return item


def _require_payload_string(value: Any, *, label: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise ProgressError(f"{label} must be a non-empty string")
    return value.strip()


def _normalize_payload_address(value: Any, *, label: str) -> str:
    if not isinstance(value, str):
        raise ProgressError(f"{label} must be an address string")
    try:
        return normalize_address(value)
    except (ProgressError, ValueError) as exc:
        raise ProgressError(f"{label} has invalid address {value!r}") from exc


def _require_unique_string_list(
    value: Any,
    *,
    label: str,
    allow_empty: bool = False,
) -> list[str]:
    if not isinstance(value, list) or (not value and not allow_empty):
        qualifier = "a string list" if allow_empty else "a non-empty string list"
        raise ProgressError(f"{label} must be {qualifier}")
    result: list[str] = []
    for index, item in enumerate(value):
        if not isinstance(item, str) or not item:
            raise ProgressError(f"{label}[{index}] must be a non-empty string")
        if item in result:
            raise ProgressError(f"{label} contains duplicate id {item!r}")
        result.append(item)
    return result


def _require_unique_relationship_rows(
    value: Any,
    *,
    label: str,
) -> list[dict[str, Any]]:
    if not isinstance(value, list):
        raise ProgressError(f"{label} must be an array")
    result: list[dict[str, Any]] = []
    seen: set[str] = set()
    for index, item in enumerate(value):
        if not isinstance(item, Mapping):
            raise ProgressError(f"{label}[{index}] must be an object")
        row = deepcopy(dict(item))
        identity = json.dumps(row, sort_keys=True, separators=(",", ":"))
        if identity in seen:
            raise ProgressError(f"{label} contains a duplicate relationship row")
        seen.add(identity)
        result.append(row)
    return result


_AUTHORED_NON_GATING_BLOCK_ACCEPT_SCHEMA = (
    "recoil-authored-non-gating-block-accept-v1"
)
_AUTHORED_NON_GATING_BLOCK_ACCEPT_FIELDS = {
    "schema",
    "reviewed",
    "parent_reviewed",
    "reason",
    "binary",
    "current_cursor",
    "expected_cursor_after",
    "blocks",
}
_AUTHORED_NON_GATING_BLOCK_ROW_FIELDS = {"id", "current"}


def _parse_authored_non_gating_block_accept_payload(
    payload_json: str,
) -> dict[str, Any]:
    try:
        raw_payload = json.loads(payload_json)
    except json.JSONDecodeError as exc:
        raise ProgressError(f"--payload-json is not valid JSON: {exc.msg}") from exc
    if not isinstance(raw_payload, Mapping):
        raise ProgressError(
            "--payload-json must be one authored non-gating block acceptance object"
        )
    payload = _require_exact_payload_fields(
        raw_payload,
        _AUTHORED_NON_GATING_BLOCK_ACCEPT_FIELDS,
        label="authored non-gating block acceptance payload",
    )
    if payload.get("schema") != _AUTHORED_NON_GATING_BLOCK_ACCEPT_SCHEMA:
        raise ProgressError(
            "authored non-gating block acceptance schema must be "
            f"{_AUTHORED_NON_GATING_BLOCK_ACCEPT_SCHEMA!r}"
        )
    if payload.get("reviewed") is not True or payload.get("parent_reviewed") is not True:
        raise ProgressError(
            "authored non-gating block acceptance requires reviewed=true and "
            "parent_reviewed=true"
        )
    payload["reason"] = _require_payload_string(payload["reason"], label="reason")
    payload["binary"] = _require_payload_string(payload["binary"], label="binary")
    if payload["binary"] != "recoil":
        raise ProgressError(
            "authored non-gating block acceptance is limited to the recoil binary"
        )
    payload["current_cursor"] = _normalize_payload_address(
        payload["current_cursor"], label="current_cursor"
    )
    payload["expected_cursor_after"] = _normalize_payload_address(
        payload["expected_cursor_after"], label="expected_cursor_after"
    )
    raw_blocks = payload.get("blocks")
    if not isinstance(raw_blocks, list) or not raw_blocks:
        raise ProgressError("authored non-gating block acceptance blocks must be non-empty")
    blocks: list[dict[str, Any]] = []
    seen: set[str] = set()
    for index, raw_row in enumerate(raw_blocks):
        if not isinstance(raw_row, Mapping):
            raise ProgressError(f"blocks[{index}] must be an object")
        row = _require_exact_payload_fields(
            raw_row,
            _AUTHORED_NON_GATING_BLOCK_ROW_FIELDS,
            label=f"blocks[{index}]",
        )
        block_id = _require_payload_string(row["id"], label=f"blocks[{index}].id")
        if not block_id.startswith("recoil:block:"):
            raise ProgressError(f"blocks[{index}].id must be a canonical recoil:block: id")
        if block_id in seen:
            raise ProgressError(f"blocks contains duplicate block id {block_id!r}")
        current = row.get("current")
        if not isinstance(current, Mapping):
            raise ProgressError(f"blocks[{index}].current must be a complete block object")
        current_record = deepcopy(dict(current))
        if current_record.get("binary") != payload["binary"]:
            raise ProgressError(
                f"blocks[{index}].current binary must be {payload['binary']!r}"
            )
        seen.add(block_id)
        blocks.append({"id": block_id, "current": current_record})
    payload["blocks"] = blocks
    return payload


def _load_authored_non_gating_block_accept_payload(args: argparse.Namespace) -> dict[str, Any]:
    if args.payload_file is None:
        return _parse_authored_non_gating_block_accept_payload(str(args.payload_json))
    try:
        payload_json = args.payload_file.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        raise ProgressError(
            f"authored non-gating block payload file is unreadable: {args.payload_file}"
        ) from exc
    return _parse_authored_non_gating_block_accept_payload(payload_json)


_PROVIDER_BLOCK_RECLASSIFY_SCHEMA = "recoil-provider-block-reclassify-v1"
_PROVIDER_BLOCK_RECLASSIFY_FIELDS = {
    "schema",
    "reviewed",
    "parent_reviewed",
    "reason",
    "binary",
    "block_id",
    "current_block",
    "expected_provider_owner_ids",
    "clear_provisional_compile_source_placement",
    "replacement",
}
_PROVIDER_BLOCK_REPLACEMENT_FIELDS = {
    "contribution_kind",
    "source_path",
    "agent_source_path",
    "provisional_original_path",
    "mapping_status",
    "mapping_confidence",
}


def _parse_provider_block_reclassify_payload(payload_json: str) -> dict[str, Any]:
    try:
        raw_payload = json.loads(payload_json)
    except json.JSONDecodeError as exc:
        raise ProgressError(f"--payload-json is not valid JSON: {exc.msg}") from exc
    if not isinstance(raw_payload, Mapping):
        raise ProgressError("--payload-json must be one provider block reclassification object")
    payload = _require_exact_payload_fields(
        raw_payload,
        _PROVIDER_BLOCK_RECLASSIFY_FIELDS,
        label="provider block reclassification payload",
    )
    if payload.get("schema") != _PROVIDER_BLOCK_RECLASSIFY_SCHEMA:
        raise ProgressError(
            "provider block reclassification schema must be "
            f"{_PROVIDER_BLOCK_RECLASSIFY_SCHEMA!r}"
        )
    if payload.get("reviewed") is not True or payload.get("parent_reviewed") is not True:
        raise ProgressError(
            "provider block reclassification requires reviewed=true and parent_reviewed=true"
        )
    if payload.get("clear_provisional_compile_source_placement") is not True:
        raise ProgressError(
            "provider block reclassification requires explicit "
            "clear_provisional_compile_source_placement=true"
        )
    payload["reason"] = _require_payload_string(payload["reason"], label="reason")
    payload["binary"] = _require_payload_string(payload["binary"], label="binary")
    if payload["binary"] != "recoil":
        raise ProgressError("provider block reclassification is limited to the recoil binary")
    payload["block_id"] = _require_payload_string(payload["block_id"], label="block_id")
    if not payload["block_id"].startswith("recoil:block:"):
        raise ProgressError("block_id must be a canonical recoil:block: id")
    current_block = payload.get("current_block")
    if not isinstance(current_block, Mapping):
        raise ProgressError("current_block must be a complete physical-block object")
    payload["current_block"] = deepcopy(dict(current_block))
    expected_owner_ids = _require_unique_string_list(
        payload.get("expected_provider_owner_ids"),
        label="expected_provider_owner_ids",
    )
    if expected_owner_ids != sorted(expected_owner_ids):
        raise ProgressError("expected_provider_owner_ids must use canonical sorted order")
    if any(not owner_id.startswith("recoil:owner:") for owner_id in expected_owner_ids):
        raise ProgressError("expected_provider_owner_ids must contain canonical recoil:owner: ids")
    payload["expected_provider_owner_ids"] = expected_owner_ids
    raw_replacement = payload.get("replacement")
    if not isinstance(raw_replacement, Mapping):
        raise ProgressError("replacement must be an object")
    replacement = _require_exact_payload_fields(
        raw_replacement,
        _PROVIDER_BLOCK_REPLACEMENT_FIELDS,
        label="replacement",
    )
    if replacement.get("contribution_kind") != "provider":
        raise ProgressError("replacement.contribution_kind must be 'provider'")
    replacement["source_path"] = _require_payload_string(
        replacement.get("source_path"), label="replacement.source_path"
    )
    replacement["agent_source_path"] = _require_payload_string(
        replacement.get("agent_source_path"), label="replacement.agent_source_path"
    )
    if (
        replacement["source_path"] != replacement["agent_source_path"]
        or not replacement["source_path"].startswith("provider:")
    ):
        raise ProgressError(
            "replacement source_path and agent_source_path must be the same provider: label"
        )
    if replacement.get("provisional_original_path") is not None:
        raise ProgressError("replacement.provisional_original_path must be null")
    if replacement.get("mapping_status") != "provider-boundary":
        raise ProgressError("replacement.mapping_status must be 'provider-boundary'")
    replacement["mapping_confidence"] = _require_payload_string(
        replacement.get("mapping_confidence"), label="replacement.mapping_confidence"
    )
    if not replacement["mapping_confidence"].casefold().startswith("high "):
        raise ProgressError("replacement.mapping_confidence must begin with 'high '")
    payload["replacement"] = replacement
    return payload


def _provider_primary_owner_ids(
    data: Mapping[str, Any],
    *,
    binary: str,
    symbol_id: str,
    symbol: Mapping[str, Any],
) -> list[str]:
    if symbol.get("ownership_state") != "primary-owned":
        raise ProgressError(
            f"provider block member {symbol_id} must have ownership_state='primary-owned'"
        )
    address = normalize_address(symbol.get("address", ""))
    owner_ids: list[str] = []
    owners = data.get("owners")
    if not isinstance(owners, Mapping):
        raise ProgressError("provider block reclassification requires owners")
    for owner_id, owner in owners.items():
        if not isinstance(owner, Mapping):
            continue
        relationships = owner.get("relationships")
        if not isinstance(relationships, list):
            continue
        matching_rows = [
            row
            for row in relationships
            if isinstance(row, Mapping)
            and row.get("kind") == "primary-function"
            and row.get("symbol_id") == symbol_id
        ]
        if not matching_rows:
            continue
        if len(matching_rows) != 1:
            raise ProgressError(
                f"provider block member {symbol_id} has duplicate primary-function rows "
                f"in owner {owner_id}"
            )
        row_address = matching_rows[0].get("address")
        if not isinstance(row_address, str) or normalize_address(row_address) != address:
            raise ProgressError(
                f"provider block member {symbol_id} primary owner {owner_id} has a stale address"
            )
        gates = owner.get("gates")
        if (
            owner.get("binary") != binary
            or owner.get("kind") != "provider-boundary"
            or owner.get("provider_state") != "accepted"
            or owner.get("lifecycle_state") != "accepted"
            or not isinstance(gates, Mapping)
            or gates.get("boundary") != "accepted"
        ):
            raise ProgressError(
                f"provider block member {symbol_id} primary owner {owner_id} is not an "
                "accepted provider-boundary owner"
            )
        owner_ids.append(str(owner_id))
    if len(owner_ids) != 1:
        raise ProgressError(
            f"provider block member {symbol_id} must resolve to exactly one accepted "
            f"provider-boundary primary owner; found {sorted(owner_ids)}"
        )
    return owner_ids


def _active_provider_block_reclassification_conflicts(
    document: ProgressDocument,
    block_id: str,
) -> list[str]:
    candidate_claims = [
        {"kind": "tracker", "id": "recoil", "access": "write"},
        {"kind": "block", "id": block_id, "access": "write"},
    ]
    conflicts: list[str] = []
    for work_id, work in document.collection("work_items").items():
        if not isinstance(work, Mapping):
            continue
        reservation = work.get("reservation")
        if not (
            work.get("state") == "active"
            or (isinstance(reservation, Mapping) and reservation.get("state") == "active")
        ):
            continue
        claims, complete, _source = work_resource_claims(work)
        if not complete:
            conflicts.append(f"{work_id} has incomplete resource claims")
            continue
        if resource_claim_conflicts(
            candidate_claims,
            str(work_id),
            claims,
            second_owner_id=str(work.get("owner_id", "")),
            second_block_id=str(work.get("block_id", "")),
        ):
            conflicts.append(str(work_id))
    return sorted(set(conflicts))


def _reclassify_provider_block(
    data: dict[str, Any],
    payload: Mapping[str, Any],
) -> dict[str, Any]:
    before = deepcopy(data)
    before_document = ProgressDocument(before)
    binary = str(payload["binary"])
    block_id = str(payload["block_id"])
    before_pipeline = before_document.pipeline(binary, resolve_order_target=False)
    blocks = data.get("physical_blocks")
    symbols = data.get("symbols")
    if not isinstance(blocks, dict) or not isinstance(symbols, dict):
        raise ProgressError("provider block reclassification requires physical_blocks and symbols")
    block = blocks.get(block_id)
    if not isinstance(block, dict):
        raise ProgressError(f"unknown physical block {block_id!r}")
    if block != payload["current_block"]:
        raise ProgressError(f"physical block snapshot is stale for {block_id}")
    if block.get("binary") != binary:
        raise ProgressError(f"physical block {block_id} has wrong binary")
    start = normalize_address(block.get("start", ""))
    end = normalize_address(block.get("end_exclusive", ""))
    if address_value(end) <= address_value(start) or block_id != f"recoil:block:{start}":
        raise ProgressError(f"physical block {block_id} has an invalid canonical interval")
    if block.get("contribution_kind") != "authored":
        raise ProgressError(f"physical block {block_id} is not a stale authored contribution")
    if block.get("original_source_path") is not None:
        raise ProgressError(f"physical block {block_id} has accepted original-source provenance")
    if not isinstance(block.get("provisional_original_path"), str) or not str(
        block.get("provisional_original_path")
    ).strip():
        raise ProgressError(
            f"physical block {block_id} lacks provisional source placement to clear"
        )
    mapping = block.get("mapping")
    if not isinstance(mapping, dict):
        raise ProgressError(f"physical block {block_id} mapping must be an object")
    if mapping.get("state") != "unresolved":
        raise ProgressError(
            f"physical block {block_id} mapping provenance must remain unaccepted/unresolved"
        )
    order_targets = block.get("order_targets")
    if not isinstance(order_targets, Mapping) or any(
        (isinstance(value, str) and bool(value.strip()))
        or (not isinstance(value, str) and value is not None)
        for value in order_targets.values()
    ):
        raise ProgressError(f"physical block {block_id} has an active configured order target")
    contribution_ids = _require_unique_string_list(
        block.get("contribution_ids"),
        label=f"physical block {block_id} contribution_ids",
    )
    linked_symbol_ids = sorted(
        str(symbol_id)
        for symbol_id, row in symbols.items()
        if isinstance(row, Mapping) and row.get("physical_block_id") == block_id
    )
    if linked_symbol_ids != sorted(contribution_ids):
        raise ProgressError(
            f"physical block {block_id} contribution membership is incomplete or stale"
        )
    member_owner_ids: dict[str, str] = {}
    for symbol_id in contribution_ids:
        symbol = symbols.get(symbol_id)
        if not isinstance(symbol, Mapping):
            raise ProgressError(f"physical block {block_id} has unknown contribution {symbol_id}")
        if (
            symbol.get("binary") != binary
            or symbol.get("pipeline_class") != "non-authored"
            or symbol.get("authored_order_role") != "non-authored"
        ):
            raise ProgressError(
                f"provider block member {symbol_id} must be resolved non-authored/non-authored"
            )
        owner_ids = _provider_primary_owner_ids(
            data,
            binary=binary,
            symbol_id=symbol_id,
            symbol=symbol,
        )
        member_owner_ids[symbol_id] = owner_ids[0]
    actual_owner_ids = sorted(set(member_owner_ids.values()))
    if actual_owner_ids != payload["expected_provider_owner_ids"]:
        raise ProgressError(
            "expected_provider_owner_ids is stale: expected "
            f"{payload['expected_provider_owner_ids']}, found {actual_owner_ids}"
        )
    conflicts = _active_provider_block_reclassification_conflicts(before_document, block_id)
    if conflicts:
        raise ProgressError(
            "provider block reclassification conflicts with active work: "
            + ", ".join(conflicts)
        )

    replacement = payload["replacement"]
    block["contribution_kind"] = replacement["contribution_kind"]
    block["source_path"] = replacement["source_path"]
    block["agent_source_path"] = replacement["agent_source_path"]
    block["provisional_original_path"] = None
    mapping["status"] = replacement["mapping_status"]
    mapping["confidence"] = replacement["mapping_confidence"]

    unchanged_projection = deepcopy(data)
    projected = unchanged_projection["physical_blocks"][block_id]
    current = before["physical_blocks"][block_id]
    for field in (
        "contribution_kind",
        "source_path",
        "agent_source_path",
        "provisional_original_path",
    ):
        projected[field] = deepcopy(current.get(field))
    projected["mapping"]["status"] = deepcopy(current["mapping"].get("status"))
    projected["mapping"]["confidence"] = deepcopy(current["mapping"].get("confidence"))
    if unchanged_projection != before:
        raise ProgressError(
            "provider block reclassification unexpectedly changed state outside the exact "
            "physical mapping/provider label fields"
        )
    after_pipeline = ProgressDocument(data).pipeline(binary, resolve_order_target=False)
    if after_pipeline != before_pipeline:
        raise ProgressError(
            "provider block reclassification unexpectedly changed the derived scheduler"
        )
    return {
        "kind": "provider-block-reclassification",
        "schema": _PROVIDER_BLOCK_RECLASSIFY_SCHEMA,
        "binary": binary,
        "block_id": block_id,
        "interval": {"start": start, "end_exclusive": end},
        "contribution_ids": contribution_ids,
        "member_provider_owner_ids": member_owner_ids,
        "provider_owner_ids": actual_owner_ids,
        "reason": payload["reason"],
        "changed_fields": [
            "contribution_kind",
            "source_path",
            "agent_source_path",
            "provisional_original_path",
            "mapping.status",
            "mapping.confidence",
        ],
        "preserved_navigation": {
            "symbol_navigation_names": True,
            "semantic_spans": list(block.get("semantic_span_ids", [])),
            "mapping_evidence_ids": list(mapping.get("evidence_ids", [])),
        },
        "scheduler_before": before_pipeline,
        "scheduler_after": after_pipeline,
        "invariants": {
            "exact_current_block_snapshot": True,
            "immutable_interval_and_membership": True,
            "resolved_non_authored_members": True,
            "exact_accepted_provider_primary_owners": True,
            "unaccepted_original_source_provenance": True,
            "no_active_conflicting_work": True,
            "no_configured_order_target": True,
            "semantic_spans_unchanged": True,
            "order_byte_owner_tier_storage_section_symbol_facts_unchanged": True,
            "scheduler_unchanged": True,
        },
    }


def _high_confidence_mapping(
    block_id: str,
    block: Mapping[str, Any],
) -> tuple[str, str]:
    mapping = block.get("mapping")
    if not isinstance(mapping, Mapping):
        raise ProgressError(f"physical block {block_id} mapping must be an object")
    status = str(mapping.get("status", "")).strip()
    confidence = str(mapping.get("confidence", "")).strip()
    if not confidence.casefold().startswith("high "):
        raise ProgressError(
            f"physical block {block_id} requires explicit high-confidence mapping"
        )
    return status, confidence


def _active_authored_non_gating_conflicts(
    document: ProgressDocument,
    block_ids: Iterable[str],
) -> list[str]:
    candidate_claims = [
        {"kind": "tracker", "id": "recoil", "access": "write"},
        {"kind": "lane", "id": "authored-function-order", "access": "write"},
        *(
            {"kind": "block", "id": block_id, "access": "write"}
            for block_id in block_ids
        ),
    ]
    conflicts: list[str] = []
    for work_id, work in document.collection("work_items").items():
        if not isinstance(work, Mapping):
            continue
        reservation = work.get("reservation")
        if not (
            work.get("state") == "active"
            or (isinstance(reservation, Mapping) and reservation.get("state") == "active")
        ):
            continue
        claims, complete, _source = work_resource_claims(work)
        if not complete:
            conflicts.append(f"{work_id} has incomplete resource claims")
            continue
        rows = resource_claim_conflicts(
            candidate_claims,
            str(work_id),
            claims,
            second_owner_id=str(work.get("owner_id", "")),
            second_block_id=str(work.get("block_id", "")),
        )
        if rows:
            conflicts.append(str(work_id))
    return sorted(set(conflicts))


def _accept_authored_non_gating_blocks(
    data: dict[str, Any],
    payload: Mapping[str, Any],
) -> dict[str, Any]:
    before = deepcopy(data)
    before_document = ProgressDocument(before)
    binary = str(payload["binary"])
    before_pipeline = before_document.pipeline(binary, resolve_order_target=False)
    if before_pipeline.get("phase") != "authored-function-order":
        raise ProgressError(
            "authored non-gating block acceptance requires authored-function-order; "
            f"found {before_pipeline.get('phase')!r}"
        )
    if before_pipeline.get("cursor") != payload["current_cursor"]:
        raise ProgressError(
            "authored non-gating block acceptance current_cursor is stale: expected "
            f"{payload['current_cursor']}, found {before_pipeline.get('cursor')}"
        )

    physical_blocks = data.get("physical_blocks")
    symbols = data.get("symbols")
    if not isinstance(physical_blocks, dict) or not isinstance(symbols, dict):
        raise ProgressError(
            "authored non-gating block acceptance requires physical_blocks and symbols"
        )
    block_ids = [str(row["id"]) for row in payload["blocks"]]
    if before_pipeline.get("physical_block_id") != block_ids[0]:
        raise ProgressError(
            "authored non-gating block acceptance must begin with the live cursor block"
        )

    sorted_blocks = before_document._blocks_for_binary(binary)
    sorted_ids = [block_id for block_id, _block in sorted_blocks]
    try:
        first_index = sorted_ids.index(block_ids[0])
    except ValueError as exc:
        raise ProgressError(f"unknown physical block {block_ids[0]!r}") from exc
    if sorted_ids[first_index : first_index + len(block_ids)] != block_ids:
        raise ProgressError(
            "authored non-gating block acceptance blocks must be the exact contiguous "
            "physical-block prefix at the live cursor"
        )

    membership_counts: dict[str, int] = {}
    mapping_statuses: dict[str, str] = {}
    classification_counts: dict[str, int] = {}
    previous_end = ""
    for index, row in enumerate(payload["blocks"]):
        block_id = str(row["id"])
        block = physical_blocks.get(block_id)
        if not isinstance(block, dict):
            raise ProgressError(f"unknown physical block {block_id!r}")
        if block != row["current"]:
            raise ProgressError(f"physical block snapshot is stale for {block_id}")
        start = normalize_address(block.get("start", ""))
        end = normalize_address(block.get("end_exclusive", ""))
        if address_value(end) <= address_value(start):
            raise ProgressError(f"physical block {block_id} has an invalid interval")
        if index == 0 and start != payload["current_cursor"]:
            raise ProgressError(
                f"first physical block {block_id} does not start at current_cursor"
            )
        if previous_end and start != previous_end:
            raise ProgressError(
                f"authored non-gating block batch has a gap before {block_id}"
            )
        previous_end = end

        order_targets = block.get("order_targets")
        if not isinstance(order_targets, Mapping) or any(
            (isinstance(value, str) and bool(value.strip()))
            or (not isinstance(value, str) and value is not None)
            for value in order_targets.values()
        ):
            raise ProgressError(
                f"physical block {block_id} has an active configured order target"
            )
        order = block.get("order")
        authored = order.get("authored") if isinstance(order, Mapping) else None
        if not isinstance(authored, Mapping) or any(
            dimension not in authored for dimension in AUTHORED_ORDER_DIMENSIONS
        ):
            raise ProgressError(
                f"physical block {block_id} lacks the complete authored-order dimensions"
            )
        if any(
            is_current_accepted_state(authored[dimension])
            for dimension in AUTHORED_ORDER_DIMENSIONS
        ):
            raise ProgressError(
                f"physical block {block_id} already has current authored-order acceptance"
            )

        contribution_ids = _require_unique_string_list(
            block.get("contribution_ids"),
            label=f"physical block {block_id} contribution_ids",
            allow_empty=True,
        )
        linked_symbol_ids = [
            str(symbol_id)
            for symbol_id, symbol in symbols.items()
            if isinstance(symbol, Mapping)
            and symbol.get("physical_block_id") == block_id
        ]
        if set(linked_symbol_ids) != set(contribution_ids):
            raise ProgressError(
                f"physical block {block_id} contribution membership is incomplete or stale"
            )
        status, _confidence = _high_confidence_mapping(block_id, block)
        contribution_kind = str(block.get("contribution_kind", ""))
        if contribution_ids:
            if status != "provider-boundary" or contribution_kind != "provider":
                raise ProgressError(
                    f"nonzero physical block {block_id} must be an explicit "
                    "high-confidence provider-boundary"
                )
            for symbol_id in contribution_ids:
                symbol = symbols.get(symbol_id)
                if not isinstance(symbol, Mapping):
                    raise ProgressError(
                        f"physical block {block_id} has unknown contribution {symbol_id}"
                    )
                if symbol.get("binary") != binary:
                    raise ProgressError(
                        f"physical block {block_id} contribution {symbol_id} has wrong binary"
                    )
                pipeline_class = str(symbol.get("pipeline_class", ""))
                authored_order_role = str(symbol.get("authored_order_role", ""))
                if (
                    pipeline_class == "unresolved"
                    or authored_order_role == "unresolved"
                    or not pipeline_class
                    or not authored_order_role
                    or symbol_authored_order_gate(symbol)
                ):
                    raise ProgressError(
                        f"physical block {block_id} contribution {symbol_id} must be "
                        "fully resolved with a non-gating authored-order role"
                    )
                try:
                    validate_authored_order_role(pipeline_class, authored_order_role)
                except ProgressError as exc:
                    raise ProgressError(
                        f"physical block {block_id} contribution {symbol_id} has an "
                        "incompatible pipeline classification/authored-order role"
                    ) from exc
                classification = f"{pipeline_class}|{authored_order_role}"
                classification_counts[classification] = (
                    classification_counts.get(classification, 0) + 1
                )
        elif not (
            (status == "provider-data" and contribution_kind == "provider")
            or (status == "padding" and contribution_kind == "padding")
        ):
            raise ProgressError(
                f"zero-row physical block {block_id} must be typed provider-data or padding"
            )
        membership_counts[block_id] = len(contribution_ids)
        mapping_statuses[block_id] = status

    if previous_end != payload["expected_cursor_after"]:
        raise ProgressError(
            "authored non-gating block acceptance expected_cursor_after must equal "
            f"the exact batch end {previous_end}"
        )
    conflicts = _active_authored_non_gating_conflicts(before_document, block_ids)
    if conflicts:
        raise ProgressError(
            "authored non-gating block acceptance conflicts with active work: "
            + ", ".join(conflicts)
        )

    evidence_before = set(before.get("evidence", {}))
    evidence_id = add_live_evidence(
        data,
        kind="live-authored-non-gating-block-acceptance",
        summary=(
            f"Reviewed live zero-gate authored-order acceptance for {len(block_ids)} "
            "contiguous physical blocks"
        ),
        scope_ids=block_ids,
        provenance={
            "schema": _AUTHORED_NON_GATING_BLOCK_ACCEPT_SCHEMA,
            "binary": binary,
            "reason": payload["reason"],
            "current_cursor": payload["current_cursor"],
            "expected_cursor_after": payload["expected_cursor_after"],
            "block_ids": block_ids,
            "contribution_counts": membership_counts,
            "mapping_statuses": mapping_statuses,
            "classification_counts": classification_counts,
            "authored_gating_identity_count": 0,
            "unresolved_contribution_count": 0,
        },
    )
    accepted = accept_live_authored_non_gating_blocks(
        data,
        block_ids=block_ids,
        evidence_id=evidence_id,
    )
    evidence_after = set(data.get("evidence", {}))
    if evidence_after - evidence_before != {evidence_id}:
        raise ProgressError(
            "authored non-gating block acceptance must create exactly one evidence record"
        )

    unchanged_projection = deepcopy(data)
    unchanged_projection["id_sequences"] = deepcopy(before.get("id_sequences", {}))
    unchanged_projection["evidence"] = deepcopy(before.get("evidence", {}))
    for block_id in block_ids:
        before_authored = before["physical_blocks"][block_id]["order"]["authored"]
        projected_authored = unchanged_projection["physical_blocks"][block_id]["order"][
            "authored"
        ]
        for dimension in AUTHORED_ORDER_DIMENSIONS:
            projected_authored[dimension] = deepcopy(before_authored[dimension])
    if unchanged_projection != before:
        raise ProgressError(
            "authored non-gating block acceptance unexpectedly changed state outside "
            "the evidence record and five authored-order dimensions"
        )

    after_document = ProgressDocument(data)
    after_pipeline = after_document.pipeline(binary, resolve_order_target=False)
    after_phase = after_pipeline.get("phase")
    authored_cursor_matches = (
        after_pipeline.get("authored_order_prefix_end")
        == payload["expected_cursor_after"]
    )
    if after_phase == "authored-function-order":
        phase_cursor_valid = after_pipeline.get("cursor") == payload["expected_cursor_after"]
    elif after_phase == "full-function-order":
        phase_cursor_valid = (
            payload["expected_cursor_after"] == after_pipeline.get("text_end_exclusive")
            and after_pipeline.get("cursor") == before_pipeline.get("full_order_prefix_end")
        )
    else:
        phase_cursor_valid = False
    if not authored_cursor_matches or not phase_cursor_valid:
        raise ProgressError(
            "authored non-gating block acceptance did not derive the exact expected "
            "authored-order phase/cursor-after"
        )
    if (
        after_pipeline.get("full_order_prefix_end")
        != before_pipeline.get("full_order_prefix_end")
        or after_pipeline.get("full_function_order_counts")
        != before_pipeline.get("full_function_order_counts")
    ):
        raise ProgressError(
            "authored non-gating block acceptance unexpectedly changed derived full order"
        )
    accepted_delta = (
        int(after_pipeline["authored_function_order_counts"]["accepted"])
        - int(before_pipeline["authored_function_order_counts"]["accepted"])
    )
    if accepted_delta != len(block_ids):
        raise ProgressError(
            "authored non-gating block acceptance derived an unexpected accepted-block delta"
        )
    return {
        "kind": "authored-non-gating-block-acceptance",
        "validation_mode": "live",
        "binary": binary,
        "reason": payload["reason"],
        "accepted_block_ids": accepted,
        "accepted_block_count": len(accepted),
        "accepted_authored_dimensions": list(AUTHORED_ORDER_DIMENSIONS),
        "contribution_counts": membership_counts,
        "classification_counts": classification_counts,
        "evidence_id": evidence_id,
        "scheduler_before": {
            "phase": before_pipeline.get("phase"),
            "cursor": before_pipeline.get("cursor"),
            "authored_order_prefix_end": before_pipeline.get("authored_order_prefix_end"),
            "full_order_prefix_end": before_pipeline.get("full_order_prefix_end"),
        },
        "scheduler_after": {
            "phase": after_pipeline.get("phase"),
            "cursor": after_pipeline.get("cursor"),
            "authored_order_prefix_end": after_pipeline.get("authored_order_prefix_end"),
            "full_order_prefix_end": after_pipeline.get("full_order_prefix_end"),
        },
        "invariants": {
            "exact_current_block_snapshots": True,
            "exact_contribution_memberships": True,
            "contiguous_live_cursor_prefix": True,
            "zero_authored_gating_identities": True,
            "no_unresolved_contributions": True,
            "no_active_conflicting_work": True,
            "no_configured_order_targets": True,
            "exact_cursor_after": True,
            "full_order_unchanged": True,
            "all_other_state_unchanged": True,
        },
    }


_OWNER_REPLACE_BATCH_SCHEMA_V1 = "recoil-owner-replace-batch-v1"
_OWNER_REPLACE_BATCH_SCHEMA_V2 = "recoil-owner-replace-batch-v2"
_OWNER_DOWNGRADE_SCHEMA_V1 = "recoil-owner-downgrade-v1"
_OWNER_PRIMARY_DATA_TIER_X_REPAIR_SCHEMA_V1 = (
    "recoil-owner-primary-data-tier-x-repair-v1"
)
_OWNER_PRIMARY_DATA_TIER_X_REPAIR_FIELDS = {
    "schema",
    "reviewed",
    "parent_reviewed",
    "reason",
    "binary",
    "owner_id",
    "primary_data",
}
_OWNER_PRIMARY_DATA_TIER_X_REPAIR_ROW_FIELDS = {
    "symbol_id",
    "address",
    "current_ownership_state",
    "current_relationship",
}
_OWNER_DOWNGRADE_FIELDS = {
    "schema",
    "reviewed",
    "parent_reviewed",
    "reason",
    "binary",
    "owner_id",
    "current_gates",
    "new_gates",
    "current_entry_tiers",
    "new_entry_tiers",
}
_OWNER_GATE_NAMES = {
    "boundary",
    "source",
    "data",
    "functional",
    "linkage",
    "owner_linkage",
    "byte",
}
_OWNER_GATE_DOWNGRADE_TARGETS = {
    "accepted": frozenset({"blocked", "pending", "deferred"}),
    "none": frozenset({"blocked", "pending", "deferred"}),
}
_OWNER_TIER_ORDER = {
    "X": 0,
    "C": 1,
    "B": 2,
    "A": 3,
    "S": 4,
}
_OWNER_REPLACE_BATCH_FIELDS = {
    "schema",
    "reviewed",
    "parent_reviewed",
    "reason",
    "binary",
    "current_owners",
    "replacement_owners",
}
_OWNER_REPLACE_BATCH_V2_FIELDS = {
    *_OWNER_REPLACE_BATCH_FIELDS,
    "primary_function_bootstraps",
    "primary_function_detachments",
    "primary_data_reassignments",
    "unknown_data_symbol_bootstraps",
}
_OWNER_REPLACE_ROW_FIELDS = {"id", "record"}
_PRIMARY_FUNCTION_BOOTSTRAP_FIELDS = {
    "reviewed",
    "symbol_id",
    "address",
    "current_ownership_state",
    "new_owner_id",
}
_PRIMARY_FUNCTION_DETACHMENT_FIELDS = {
    "reviewed",
    "symbol_id",
    "address",
    "current_owner_id",
    "current_ownership_state",
    "current_pipeline_class",
    "current_authored_order_role",
}
_PRIMARY_FUNCTION_DETACHMENT_CLASSIFICATIONS = {
    ("non-authored", "compiler-generated-icf-representative"),
    ("non-authored", "non-authored"),
}
_PRIMARY_DATA_REASSIGNMENT_FIELDS = {
    "reviewed",
    "symbol_id",
    "address",
    "current_owner_id",
    "current_ownership_state",
    "new_owner_id",
}
_UNKNOWN_DATA_SYMBOL_BOOTSTRAP_FIELDS = {
    "reviewed",
    "symbol_id",
    "address",
    "navigation_name",
    "disposition",
    "output_section_id",
}


def _parse_owner_primary_data_tier_x_repair_payload(
    payload_json: str,
) -> dict[str, Any]:
    try:
        raw_payload = json.loads(payload_json)
    except json.JSONDecodeError as exc:
        raise ProgressError(f"--payload-json is not valid JSON: {exc.msg}") from exc
    if not isinstance(raw_payload, Mapping):
        raise ProgressError(
            "--payload-json must be one primary-data tier-X repair object"
        )
    payload = _require_exact_payload_fields(
        raw_payload,
        _OWNER_PRIMARY_DATA_TIER_X_REPAIR_FIELDS,
        label="primary-data tier-X repair payload",
    )
    if payload["schema"] != _OWNER_PRIMARY_DATA_TIER_X_REPAIR_SCHEMA_V1:
        raise ProgressError(
            "primary-data tier-X repair schema must be "
            f"{_OWNER_PRIMARY_DATA_TIER_X_REPAIR_SCHEMA_V1!r}"
        )
    if payload["reviewed"] is not True or payload["parent_reviewed"] is not True:
        raise ProgressError(
            "primary-data tier-X repair requires reviewed=true and "
            "parent_reviewed=true"
        )
    payload["reason"] = _require_payload_string(payload["reason"], label="reason")
    payload["binary"] = _require_payload_string(payload["binary"], label="binary")
    if payload["binary"] not in {"recoil", "messages"}:
        raise ProgressError(
            "primary-data tier-X repair binary must be recoil or messages"
        )
    payload["owner_id"] = _require_payload_string(
        payload["owner_id"], label="owner_id"
    )
    raw_rows = payload["primary_data"]
    if not isinstance(raw_rows, list) or not raw_rows:
        raise ProgressError(
            "primary-data tier-X repair primary_data must be a non-empty array"
        )
    rows: list[dict[str, Any]] = []
    seen_symbol_ids: set[str] = set()
    seen_addresses: set[str] = set()
    for index, raw_row in enumerate(raw_rows):
        row = _require_exact_payload_fields(
            raw_row,
            _OWNER_PRIMARY_DATA_TIER_X_REPAIR_ROW_FIELDS,
            label=f"primary_data[{index}]",
        )
        row["symbol_id"] = _require_payload_string(
            row["symbol_id"], label=f"primary_data[{index}].symbol_id"
        )
        if not row["symbol_id"].startswith(f"{payload['binary']}:data:"):
            raise ProgressError(
                f"primary_data[{index}].symbol_id must be a canonical data identity"
            )
        row["address"] = normalize_address(
            _require_payload_string(
                row["address"], label=f"primary_data[{index}].address"
            )
        )
        row["current_ownership_state"] = _require_payload_string(
            row["current_ownership_state"],
            label=f"primary_data[{index}].current_ownership_state",
        )
        if row["current_ownership_state"] != "primary-owned":
            raise ProgressError(
                "primary-data tier-X repair requires current_ownership_state="
                "'primary-owned'"
            )
        relationship = row["current_relationship"]
        if not isinstance(relationship, Mapping):
            raise ProgressError(
                f"primary_data[{index}].current_relationship must be an object"
            )
        row["current_relationship"] = deepcopy(dict(relationship))
        if row["symbol_id"] in seen_symbol_ids:
            raise ProgressError(
                f"primary-data tier-X repair duplicates symbol {row['symbol_id']!r}"
            )
        if row["address"] in seen_addresses:
            raise ProgressError(
                f"primary-data tier-X repair duplicates address {row['address']!r}"
            )
        seen_symbol_ids.add(row["symbol_id"])
        seen_addresses.add(row["address"])
        rows.append(row)
    payload["primary_data"] = rows
    return payload


def _repair_owner_primary_data_tier_x(
    data: dict[str, Any],
    payload: Mapping[str, Any],
) -> dict[str, Any]:
    owners = data.get("owners")
    symbols = data.get("symbols")
    if not isinstance(owners, dict) or not isinstance(symbols, dict):
        raise ProgressError(
            "primary-data tier-X repair requires tracker owner and symbol collections"
        )
    owner_id = str(payload["owner_id"])
    owner = owners.get(owner_id)
    if not isinstance(owner, dict):
        raise ProgressError(
            f"primary-data tier-X repair exact owner {owner_id!r} does not exist"
        )
    if owner.get("binary") != payload["binary"]:
        raise ProgressError(
            f"primary-data tier-X repair owner {owner_id!r} binary is stale"
        )
    if owner.get("kind") == "provider-boundary":
        raise ProgressError(
            "primary-data tier-X repair rejects provider-boundary owners"
        )
    relationships = owner.get("relationships")
    reimplementation = owner.get("reimplementation")
    entries = (
        reimplementation.get("entries")
        if isinstance(reimplementation, dict)
        else None
    )
    if not isinstance(relationships, list) or not isinstance(entries, dict):
        raise ProgressError(
            f"primary-data tier-X repair owner {owner_id!r} must have relationship "
            "and reimplementation-entry collections"
        )

    initialized: list[str] = []
    for row in payload["primary_data"]:
        symbol_id = str(row["symbol_id"])
        address = str(row["address"])
        symbol = symbols.get(symbol_id)
        if not isinstance(symbol, Mapping):
            raise ProgressError(
                f"primary-data tier-X repair references unknown symbol {symbol_id!r}"
            )
        if symbol.get("binary") != payload["binary"]:
            raise ProgressError(
                f"primary-data tier-X repair symbol {symbol_id!r} binary is stale"
            )
        if symbol.get("kind") != "data" or symbol.get("disposition") != "authored":
            raise ProgressError(
                f"primary-data tier-X repair symbol {symbol_id!r} is not authored data"
            )
        if normalize_address(str(symbol.get("address"))) != address:
            raise ProgressError(
                f"primary-data tier-X repair symbol {symbol_id!r} address is stale"
            )
        if symbol.get("ownership_state") != row["current_ownership_state"]:
            raise ProgressError(
                f"primary-data tier-X repair symbol {symbol_id!r} ownership_state is stale"
            )
        exact_relationships = [
            relationship
            for relationship in relationships
            if isinstance(relationship, Mapping)
            and relationship.get("kind") == "primary-data"
            and relationship.get("symbol_id") == symbol_id
        ]
        if len(exact_relationships) != 1:
            raise ProgressError(
                f"primary-data tier-X repair symbol {symbol_id!r} must have exactly "
                f"one current relationship in owner {owner_id!r}"
            )
        if exact_relationships[0] != row["current_relationship"]:
            raise ProgressError(
                f"primary-data tier-X repair symbol {symbol_id!r} relationship snapshot is stale"
            )
        if normalize_address(str(exact_relationships[0].get("address"))) != address:
            raise ProgressError(
                f"primary-data tier-X repair symbol {symbol_id!r} relationship address is stale"
            )
        owning_owner_ids = sorted(
            candidate_owner_id
            for candidate_owner_id, candidate_owner in owners.items()
            if isinstance(candidate_owner, Mapping)
            and any(
                isinstance(candidate_relationship, Mapping)
                and candidate_relationship.get("kind") == "primary-data"
                and candidate_relationship.get("symbol_id") == symbol_id
                for candidate_relationship in candidate_owner.get("relationships", [])
            )
        )
        if owning_owner_ids != [owner_id]:
            raise ProgressError(
                f"primary-data tier-X repair symbol {symbol_id!r} has ambiguous or "
                "changed owner membership"
            )
        if symbol_id in entries:
            raise ProgressError(
                f"primary-data tier-X repair symbol {symbol_id!r} already has a "
                "reimplementation entry"
            )
        initialized.append(symbol_id)

    for symbol_id in initialized:
        entries[symbol_id] = {"kind": "data", "tier": "X", "evidence_ids": []}

    validate_owner_invariants(data)
    return {
        "kind": "owner-primary-data-tier-x-repair",
        "schema": str(payload["schema"]),
        "reviewed": True,
        "parent_reviewed": True,
        "reason": str(payload["reason"]),
        "binary": str(payload["binary"]),
        "owner_id": owner_id,
        "initialized_primary_data_tier_x_ids": initialized,
        "initialized_entry": {"kind": "data", "tier": "X", "evidence_ids": []},
        "owner_invariants_passed": True,
        "membership_unchanged": True,
        "all_other_state_unchanged": True,
    }


def _parse_owner_downgrade_payload(payload_json: str) -> dict[str, Any]:
    try:
        raw_payload = json.loads(payload_json)
    except json.JSONDecodeError as exc:
        raise ProgressError(f"--payload-json is not valid JSON: {exc.msg}") from exc
    if not isinstance(raw_payload, Mapping):
        raise ProgressError("--payload-json must be one owner downgrade object")
    payload = _require_exact_payload_fields(
        raw_payload,
        _OWNER_DOWNGRADE_FIELDS,
        label="owner downgrade payload",
    )
    if payload["schema"] != _OWNER_DOWNGRADE_SCHEMA_V1:
        raise ProgressError(
            f"owner downgrade schema must be {_OWNER_DOWNGRADE_SCHEMA_V1!r}"
        )
    if payload["reviewed"] is not True or payload["parent_reviewed"] is not True:
        raise ProgressError(
            "owner downgrade requires reviewed=true and parent_reviewed=true"
        )
    payload["reason"] = _require_payload_string(payload["reason"], label="reason")
    payload["binary"] = _require_payload_string(
        payload["binary"], label="binary"
    )
    if payload["binary"] not in {"recoil", "messages"}:
        raise ProgressError("owner downgrade binary must be recoil or messages")
    payload["owner_id"] = _require_payload_string(
        payload["owner_id"], label="owner_id"
    )

    def state_map(
        value: Any,
        *,
        label: str,
        allowed_keys: set[str] | None = None,
    ) -> dict[str, str]:
        if not isinstance(value, Mapping):
            raise ProgressError(f"{label} must be an object")
        result: dict[str, str] = {}
        for raw_key, raw_state in value.items():
            key = _require_payload_string(raw_key, label=f"{label} key")
            if allowed_keys is not None and key not in allowed_keys:
                raise ProgressError(
                    f"{label} contains unsupported gate {key!r}"
                )
            result[key] = _require_payload_string(
                raw_state, label=f"{label}.{key}"
            )
        return result

    payload["current_gates"] = state_map(
        payload["current_gates"],
        label="current_gates",
        allowed_keys=_OWNER_GATE_NAMES,
    )
    payload["new_gates"] = state_map(
        payload["new_gates"],
        label="new_gates",
        allowed_keys=_OWNER_GATE_NAMES,
    )
    payload["current_entry_tiers"] = state_map(
        payload["current_entry_tiers"],
        label="current_entry_tiers",
    )
    payload["new_entry_tiers"] = state_map(
        payload["new_entry_tiers"],
        label="new_entry_tiers",
    )
    if set(payload["current_gates"]) != set(payload["new_gates"]):
        raise ProgressError(
            "owner downgrade current_gates and new_gates must select exact same gates"
        )
    if set(payload["current_entry_tiers"]) != set(
        payload["new_entry_tiers"]
    ):
        raise ProgressError(
            "owner downgrade current_entry_tiers and new_entry_tiers must "
            "select exact same entries"
        )
    if not payload["current_gates"] and not payload["current_entry_tiers"]:
        raise ProgressError(
            "owner downgrade must select at least one gate or primary entry"
        )
    for gate, current in payload["current_gates"].items():
        new = payload["new_gates"][gate]
        if new not in _OWNER_GATE_DOWNGRADE_TARGETS.get(current, frozenset()):
            raise ProgressError(
                f"owner downgrade gate {gate!r} transition {current!r} -> "
                f"{new!r} is not a conservative downgrade"
            )
    for symbol_id, current in payload["current_entry_tiers"].items():
        new = payload["new_entry_tiers"][symbol_id]
        if current not in _OWNER_TIER_ORDER or new not in _OWNER_TIER_ORDER:
            raise ProgressError(
                f"owner downgrade entry {symbol_id!r} tiers must be one of "
                + ", ".join(_OWNER_TIER_ORDER)
            )
        if _OWNER_TIER_ORDER[new] >= _OWNER_TIER_ORDER[current]:
            raise ProgressError(
                f"owner downgrade entry {symbol_id!r} transition "
                f"{current!r} -> {new!r} is not a strict tier downgrade"
            )
    return payload


def _downgrade_owner(
    data: dict[str, Any],
    payload: Mapping[str, Any],
) -> dict[str, Any]:
    owners = data.get("owners")
    evidence = data.get("evidence")
    if not isinstance(owners, dict) or not isinstance(evidence, dict):
        raise ProgressError(
            "owner downgrade requires tracker owner and evidence collections"
        )
    owner_id = str(payload["owner_id"])
    owner = owners.get(owner_id)
    if not isinstance(owner, dict):
        raise ProgressError(
            f"owner downgrade exact owner {owner_id!r} does not exist"
        )
    if owner.get("binary") != payload["binary"]:
        raise ProgressError(
            f"owner downgrade owner {owner_id!r} binary is stale"
        )
    if owner.get("kind") == "provider-boundary":
        raise ProgressError("owner downgrade rejects provider-boundary owners")

    gates = owner.get("gates")
    if not isinstance(gates, dict):
        raise ProgressError(
            f"owner downgrade owner {owner_id!r} gates must be an object"
        )
    for gate, expected in payload["current_gates"].items():
        if gate not in gates or gates.get(gate) != expected:
            raise ProgressError(
                f"owner downgrade gate {gate!r} current state is stale: "
                f"expected {expected!r}, found {gates.get(gate)!r}"
            )

    relationships = owner.get("relationships")
    if not isinstance(relationships, list):
        raise ProgressError(
            f"owner downgrade owner {owner_id!r} relationships must be an array"
        )
    primary_symbol_ids = {
        str(row.get("symbol_id"))
        for row in relationships
        if isinstance(row, Mapping)
        and row.get("kind") in {"primary-function", "primary-data"}
        and isinstance(row.get("symbol_id"), str)
    }
    reimplementation = owner.get("reimplementation")
    entries = (
        reimplementation.get("entries")
        if isinstance(reimplementation, dict)
        else None
    )
    if payload["current_entry_tiers"] and not isinstance(entries, dict):
        raise ProgressError(
            f"owner downgrade owner {owner_id!r} reimplementation entries "
            "must be an object"
        )
    if entries is None:
        entries = {}
    affected_entries: list[dict[str, str]] = []
    for symbol_id, expected_tier in payload["current_entry_tiers"].items():
        if symbol_id not in primary_symbol_ids:
            raise ProgressError(
                f"owner downgrade entry {symbol_id!r} is not a primary entry "
                f"of owner {owner_id!r}"
            )
        entry = entries.get(symbol_id)
        if not isinstance(entry, dict):
            raise ProgressError(
                f"owner downgrade primary entry {symbol_id!r} has no exact "
                "reimplementation record"
            )
        if entry.get("tier") != expected_tier:
            raise ProgressError(
                f"owner downgrade entry {symbol_id!r} current tier is stale: "
                f"expected {expected_tier!r}, found {entry.get('tier')!r}"
            )
        affected_entries.append(
            {
                "symbol_id": symbol_id,
                "current_tier": expected_tier,
                "new_tier": str(payload["new_entry_tiers"][symbol_id]),
            }
        )

    evidence_id = add_live_evidence(
        data,
        kind="reviewed-owner-downgrade",
        summary=(
            f"Reviewed conservative owner gate/tier downgrade for {owner_id}"
        ),
        scope_ids=[owner_id, *payload["current_entry_tiers"]],
        provenance={
            "schema": str(payload["schema"]),
            "reason": str(payload["reason"]),
            "binary": str(payload["binary"]),
            "owner_id": owner_id,
            "gate_downgrades": [
                {
                    "gate": gate,
                    "current_state": current,
                    "new_state": str(payload["new_gates"][gate]),
                }
                for gate, current in payload["current_gates"].items()
            ],
            "entry_tier_downgrades": deepcopy(affected_entries),
        },
    )

    owner_evidence_ids = owner.get("evidence_ids", [])
    if not isinstance(owner_evidence_ids, list) or any(
        not isinstance(item, str) for item in owner_evidence_ids
    ):
        raise ProgressError(
            f"owner downgrade owner {owner_id!r} evidence_ids must be a string array"
        )
    owner["evidence_ids"] = sorted(
        set(owner_evidence_ids) | {evidence_id}
    )
    for gate, new_state in payload["new_gates"].items():
        gates[gate] = new_state
    for row in affected_entries:
        entry = entries[row["symbol_id"]]
        entry_evidence_ids = entry.get("evidence_ids", [])
        if not isinstance(entry_evidence_ids, list) or any(
            not isinstance(item, str) for item in entry_evidence_ids
        ):
            raise ProgressError(
                f"owner downgrade entry {row['symbol_id']!r} evidence_ids "
                "must be a string array"
            )
        entry["tier"] = row["new_tier"]
        entry["evidence_ids"] = sorted(
            set(entry_evidence_ids) | {evidence_id}
        )

    validate_owner_invariants(data)
    return {
        "kind": "owner-downgrade",
        "schema": str(payload["schema"]),
        "reviewed": True,
        "parent_reviewed": True,
        "reason": str(payload["reason"]),
        "binary": str(payload["binary"]),
        "owner_id": owner_id,
        "gate_downgrades": [
            {
                "gate": gate,
                "current_state": current,
                "new_state": str(payload["new_gates"][gate]),
            }
            for gate, current in payload["current_gates"].items()
        ],
        "entry_tier_downgrades": affected_entries,
        "evidence_id": evidence_id,
        "owner_invariants_passed": True,
        "unrelated_state_preserved": True,
    }


def _parse_owner_replace_batch_payload(
    payload_json: str,
    *,
    source_label: str = "--payload-json",
) -> dict[str, Any]:
    try:
        raw_payload = json.loads(payload_json)
    except json.JSONDecodeError as exc:
        raise ProgressError(f"{source_label} is not valid JSON: {exc.msg}") from exc
    if not isinstance(raw_payload, Mapping):
        raise ProgressError(
            f"{source_label} must be one owner replacement batch object"
        )
    schema = raw_payload.get("schema")
    if schema == _OWNER_REPLACE_BATCH_SCHEMA_V1:
        expected_fields = _OWNER_REPLACE_BATCH_FIELDS
    elif schema == _OWNER_REPLACE_BATCH_SCHEMA_V2:
        expected_fields = _OWNER_REPLACE_BATCH_V2_FIELDS
    else:
        raise ProgressError(
            "owner replacement batch schema must be "
            f"{_OWNER_REPLACE_BATCH_SCHEMA_V1!r} or {_OWNER_REPLACE_BATCH_SCHEMA_V2!r}"
        )
    normalized_payload = dict(raw_payload)
    if schema == _OWNER_REPLACE_BATCH_SCHEMA_V2:
        normalized_payload.setdefault("primary_function_detachments", [])
    payload = _require_exact_payload_fields(
        normalized_payload,
        expected_fields,
        label="owner replacement batch payload",
    )
    if payload["reviewed"] is not True or payload["parent_reviewed"] is not True:
        raise ProgressError(
            "owner replacement batch requires reviewed=true and parent_reviewed=true"
        )
    payload["reason"] = _require_payload_string(payload["reason"], label="reason")
    payload["binary"] = _require_payload_string(payload["binary"], label="binary")

    def rows(value: Any, *, label: str) -> list[dict[str, Any]]:
        if not isinstance(value, list) or not value:
            raise ProgressError(f"{label} must be a non-empty array")
        result: list[dict[str, Any]] = []
        seen: set[str] = set()
        for index, raw_row in enumerate(value):
            if not isinstance(raw_row, Mapping):
                raise ProgressError(f"{label}[{index}] must be an object")
            row = _require_exact_payload_fields(
                raw_row,
                _OWNER_REPLACE_ROW_FIELDS,
                label=f"{label}[{index}]",
            )
            owner_id = _require_payload_string(row["id"], label=f"{label}[{index}].id")
            if not owner_id.startswith("recoil:owner:"):
                raise ProgressError(f"{label}[{index}].id must be a canonical recoil:owner: id")
            if owner_id in seen:
                raise ProgressError(f"{label} contains duplicate owner id {owner_id!r}")
            if not isinstance(row["record"], Mapping):
                raise ProgressError(f"{label}[{index}].record must be an object")
            record = deepcopy(dict(row["record"]))
            if record.get("binary") != payload["binary"]:
                raise ProgressError(
                    f"{label}[{index}].record binary must be {payload['binary']!r}"
                )
            seen.add(owner_id)
            result.append({"id": owner_id, "record": record})
        return result

    payload["current_owners"] = rows(payload["current_owners"], label="current_owners")
    payload["replacement_owners"] = rows(
        payload["replacement_owners"], label="replacement_owners"
    )
    payload.setdefault("primary_function_bootstraps", [])
    payload.setdefault("primary_function_detachments", [])
    payload.setdefault("primary_data_reassignments", [])
    payload.setdefault("unknown_data_symbol_bootstraps", [])

    def reviewed_rows(
        value: Any,
        *,
        label: str,
        fields: set[str],
        allow_current_owner: bool = False,
    ) -> list[dict[str, Any]]:
        if not isinstance(value, list):
            raise ProgressError(f"{label} must be an array")
        result: list[dict[str, Any]] = []
        seen_symbols: set[str] = set()
        seen_addresses: set[str] = set()
        for index, raw_row in enumerate(value):
            if not isinstance(raw_row, Mapping):
                raise ProgressError(f"{label}[{index}] must be an object")
            row = _require_exact_payload_fields(
                raw_row, fields, label=f"{label}[{index}]"
            )
            if row["reviewed"] is not True:
                raise ProgressError(f"{label}[{index}] must set reviewed=true")
            symbol_id = _require_payload_string(
                row["symbol_id"], label=f"{label}[{index}].symbol_id"
            )
            address = _normalize_payload_address(
                row["address"], label=f"{label}[{index}].address"
            )
            if symbol_id in seen_symbols or address in seen_addresses:
                raise ProgressError(f"{label} contains a duplicate symbol or address")
            if not symbol_id.startswith("recoil:"):
                raise ProgressError(f"{label}[{index}].symbol_id must be a recoil symbol id")
            row["symbol_id"] = symbol_id
            row["address"] = address
            if "new_owner_id" in row:
                row["new_owner_id"] = _require_payload_string(
                    row["new_owner_id"], label=f"{label}[{index}].new_owner_id"
                )
            if allow_current_owner:
                current_owner_id = row["current_owner_id"]
                if current_owner_id is not None:
                    row["current_owner_id"] = _require_payload_string(
                        current_owner_id,
                        label=f"{label}[{index}].current_owner_id",
                    )
            if "current_ownership_state" in row and row["current_ownership_state"] is not None:
                row["current_ownership_state"] = _require_payload_string(
                    row["current_ownership_state"],
                    label=f"{label}[{index}].current_ownership_state",
                )
            seen_symbols.add(symbol_id)
            seen_addresses.add(address)
            result.append(row)
        return result

    payload["primary_function_bootstraps"] = reviewed_rows(
        payload["primary_function_bootstraps"],
        label="primary_function_bootstraps",
        fields=_PRIMARY_FUNCTION_BOOTSTRAP_FIELDS,
    )
    payload["primary_function_detachments"] = reviewed_rows(
        payload["primary_function_detachments"],
        label="primary_function_detachments",
        fields=_PRIMARY_FUNCTION_DETACHMENT_FIELDS,
        allow_current_owner=True,
    )
    for index, row in enumerate(payload["primary_function_detachments"]):
        for field in ("current_pipeline_class", "current_authored_order_role"):
            row[field] = _require_payload_string(
                row[field],
                label=f"primary_function_detachments[{index}].{field}",
            )
    payload["primary_data_reassignments"] = reviewed_rows(
        payload["primary_data_reassignments"],
        label="primary_data_reassignments",
        fields=_PRIMARY_DATA_REASSIGNMENT_FIELDS,
        allow_current_owner=True,
    )
    payload["unknown_data_symbol_bootstraps"] = reviewed_rows(
        payload["unknown_data_symbol_bootstraps"],
        label="unknown_data_symbol_bootstraps",
        fields=_UNKNOWN_DATA_SYMBOL_BOOTSTRAP_FIELDS,
    )
    for index, row in enumerate(payload["unknown_data_symbol_bootstraps"]):
        if not row["symbol_id"].startswith("recoil:data:"):
            raise ProgressError(
                f"unknown_data_symbol_bootstraps[{index}].symbol_id must be recoil:data:..."
            )
        row["navigation_name"] = _require_payload_string(
            row["navigation_name"],
            label=f"unknown_data_symbol_bootstraps[{index}].navigation_name",
        )
        if row["disposition"] not in {"provider", "unresolved"}:
            raise ProgressError(
                f"unknown_data_symbol_bootstraps[{index}].disposition must be provider or unresolved"
            )
        if row["output_section_id"] is not None and not isinstance(
            row["output_section_id"], str
        ):
            raise ProgressError(
                f"unknown_data_symbol_bootstraps[{index}].output_section_id must be a string or null"
            )
    return payload


def _load_owner_replace_batch_payload(args: argparse.Namespace) -> dict[str, Any]:
    if args.payload_file is None:
        return _parse_owner_replace_batch_payload(str(args.payload_json))

    payload_path = args.payload_file
    build_root = (REPO_ROOT / "build").resolve()
    resolved_path = (
        (REPO_ROOT / payload_path).resolve()
        if not payload_path.is_absolute()
        else payload_path.resolve()
    )
    try:
        resolved_path.relative_to(build_root)
    except ValueError as exc:
        raise ProgressError(
            "owner replacement --payload-file must resolve under workspace build/"
        ) from exc
    try:
        file_size = resolved_path.stat().st_size
    except OSError as exc:
        raise ProgressError(
            f"owner replacement payload file is missing or unreadable: {payload_path}"
        ) from exc
    if not resolved_path.is_file():
        raise ProgressError(
            f"owner replacement payload file is not a regular file: {payload_path}"
        )
    if file_size > MAX_PROGRESS_PAYLOAD_FILE_BYTES:
        raise ProgressError(
            "owner replacement payload file exceeds the "
            f"{MAX_PROGRESS_PAYLOAD_FILE_BYTES}-byte limit: {payload_path}"
        )
    try:
        payload_json = resolved_path.read_bytes().decode("utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        if isinstance(exc, UnicodeDecodeError):
            message = "is not valid UTF-8"
        else:
            message = "is unreadable"
        raise ProgressError(
            f"owner replacement payload file {message}: {payload_path}"
        ) from exc
    return _parse_owner_replace_batch_payload(
        payload_json,
        source_label="--payload-file",
    )


def _owner_primary_function_map(
    owners: Mapping[str, Any],
    symbols: Mapping[str, Any],
    *,
    binary: str,
    label: str,
) -> dict[str, str]:
    result: dict[str, str] = {}
    address_owners: dict[str, tuple[str, str]] = {}
    for owner_id, raw_owner in owners.items():
        if not isinstance(raw_owner, Mapping) or raw_owner.get("binary") != binary:
            continue
        relationships = raw_owner.get("relationships")
        if not isinstance(relationships, list):
            raise ProgressError(f"{label} owner {owner_id!r} relationships must be an array")
        local_seen: set[str] = set()
        for index, relationship in enumerate(relationships):
            if not isinstance(relationship, Mapping) or relationship.get("kind") != "primary-function":
                continue
            symbol_id = relationship.get("symbol_id")
            address = relationship.get("address")
            if not isinstance(symbol_id, str) or not symbol_id:
                raise ProgressError(
                    f"{label} owner {owner_id!r} primary-function relationship {index} "
                    "requires symbol_id"
                )
            if symbol_id in local_seen or symbol_id in result:
                raise ProgressError(
                    f"{label} has duplicate primary-function ownership for {symbol_id!r}"
                )
            symbol = symbols.get(symbol_id)
            if not isinstance(symbol, Mapping):
                raise ProgressError(
                    f"{label} owner {owner_id!r} references unknown function symbol {symbol_id!r}"
                )
            if symbol.get("binary") != binary or symbol.get("kind") not in {
                "function",
                "provider-function",
                "compiler-function",
            }:
                raise ProgressError(
                    f"{label} owner {owner_id!r} primary relationship {symbol_id!r} must "
                    f"resolve to an exact {binary} callable function"
                )
            stored_address = symbol.get("address")
            if not isinstance(address, str) or not isinstance(stored_address, str):
                raise ProgressError(
                    f"{label} owner {owner_id!r} primary relationship {symbol_id!r} "
                    "requires an exact address"
                )
            if normalize_address(address) != normalize_address(stored_address):
                raise ProgressError(
                    f"{label} owner {owner_id!r} primary relationship {symbol_id!r} "
                    f"address is stale: expected {stored_address!r}, found {address!r}"
                )
            normalized_address = normalize_address(address)
            previous = address_owners.get(normalized_address)
            if previous is not None:
                raise ProgressError(
                    f"{label} has duplicate primary-function address ownership for "
                    f"{normalized_address!r}: {previous[1]!r} in {previous[0]!r} and "
                    f"{symbol_id!r} in {owner_id!r}"
                )
            local_seen.add(symbol_id)
            result[symbol_id] = str(owner_id)
            address_owners[normalized_address] = (str(owner_id), symbol_id)
    return result


def _non_primary_relationships(owner: Mapping[str, Any]) -> list[Any]:
    relationships = owner.get("relationships", [])
    if not isinstance(relationships, list):
        return []
    return [
        deepcopy(row)
        for row in relationships
        if not (
            isinstance(row, Mapping)
            and row.get("kind") in {"primary-function", "primary-data"}
        )
    ]


def _review_dependency_retargets(
    *,
    owner_id: str,
    old_owner: Mapping[str, Any],
    new_owner: Mapping[str, Any],
    retired_owner_ids: set[str],
    replacement_owner_ids: set[str],
) -> list[dict[str, str]]:
    old_relationships = _non_primary_relationships(old_owner)
    new_relationships = _non_primary_relationships(new_owner)
    if len(old_relationships) != len(new_relationships):
        raise ProgressError(
            f"updated owner {owner_id!r} may not add or remove non-primary relationships"
        )
    retargets: list[dict[str, str]] = []
    for index, (old, new) in enumerate(zip(old_relationships, new_relationships)):
        if old == new:
            continue
        if (
            not isinstance(old, Mapping)
            or not isinstance(new, Mapping)
            or old.get("kind") != "depends-on-owner"
            or new.get("kind") != "depends-on-owner"
        ):
            raise ProgressError(
                f"updated owner {owner_id!r} changes unrelated non-primary relationship {index}"
            )
        old_fixed = {key: value for key, value in old.items() if key not in {"target_owner_id", "reason"}}
        new_fixed = {key: value for key, value in new.items() if key not in {"target_owner_id", "reason"}}
        if old_fixed != new_fixed:
            raise ProgressError(
                f"updated owner {owner_id!r} dependency retarget {index} changes fields "
                "other than target_owner_id and reason"
            )
        old_target = str(old.get("target_owner_id", ""))
        new_target = str(new.get("target_owner_id", ""))
        if old_target not in retired_owner_ids:
            raise ProgressError(
                f"updated owner {owner_id!r} dependency retarget {index} old target "
                f"{old_target!r} is not retired by this batch"
            )
        if new_target not in replacement_owner_ids:
            raise ProgressError(
                f"updated owner {owner_id!r} dependency retarget {index} new target "
                f"{new_target!r} is not an explicit replacement owner"
            )
        old_reason = str(old.get("reason", ""))
        new_reason = str(new.get("reason", ""))
        retargets.append(
            {
                "owner_id": owner_id,
                "old_target_owner_id": old_target,
                "new_target_owner_id": new_target,
                "old_reason": old_reason,
                "new_reason": new_reason,
            }
        )
    return retargets


def _owner_primary_data_map(
    owners: Mapping[str, Any],
    symbols: Mapping[str, Any],
    *,
    binary: str,
    label: str,
) -> dict[str, str]:
    result: dict[str, str] = {}
    addresses: dict[str, tuple[str, str]] = {}
    for owner_id, owner in owners.items():
        if not isinstance(owner, Mapping) or owner.get("binary") != binary:
            continue
        for index, relationship in enumerate(owner.get("relationships", [])):
            if not isinstance(relationship, Mapping) or relationship.get("kind") != "primary-data":
                continue
            symbol_id = relationship.get("symbol_id")
            address = relationship.get("address")
            if not isinstance(symbol_id, str) or not isinstance(address, str):
                raise ProgressError(
                    f"{label} owner {owner_id!r} primary-data relationship {index} "
                    "requires symbol_id and address"
                )
            symbol = symbols.get(symbol_id)
            if (
                not isinstance(symbol, Mapping)
                or symbol.get("binary") != binary
                or symbol.get("kind") != "data"
            ):
                raise ProgressError(
                    f"{label} owner {owner_id!r} primary-data relationship {symbol_id!r} "
                    f"must resolve to an exact {binary} data symbol"
                )
            symbol_address = symbol.get("address")
            if not isinstance(symbol_address, str) or normalize_address(address) != normalize_address(symbol_address):
                raise ProgressError(
                    f"{label} owner {owner_id!r} primary-data relationship {symbol_id!r} "
                    "has a stale address"
                )
            normalized_address = normalize_address(address)
            if symbol_id in result:
                raise ProgressError(f"{label} has duplicate primary-data ownership for {symbol_id!r}")
            previous = addresses.get(normalized_address)
            if previous is not None:
                raise ProgressError(
                    f"{label} has duplicate primary-data address ownership for {normalized_address!r}"
                )
            result[symbol_id] = str(owner_id)
            addresses[normalized_address] = (str(owner_id), symbol_id)
    return result


def _unknown_data_symbol_record(row: Mapping[str, Any], *, binary: str) -> dict[str, Any]:
    return {
        "accepted_byte_facts": None,
        "accepted_order_facts": None,
        "address": str(row["address"]),
        "binary": binary,
        "binary_state": {
            dimension: state_record(
                result="pending",
                disposition="claim",
                freshness="current-unhashed",
                evidence_ids=(),
            )
            for dimension in (
                "object_byte",
                "relocation_identity",
                "linked_presence",
                "linked_address",
                "linked_target_identity",
                "linked_targets",
                "linked_body_byte",
                "linked_byte",
            )
        },
        "binary_state_diagnostics": {
            "legacy_order": state_record(
                result="pending",
                disposition="claim",
                freshness="current-unhashed",
                evidence_ids=(),
            )
        },
        "disposition": str(row["disposition"]),
        "evidence_ids": [],
        "extent_state": "unknown",
        "kind": "data",
        "navigation_name": str(row["navigation_name"]),
        "output_section_id": row["output_section_id"],
        "ownership_state": "primary-owned",
        "physical_block_id": None,
        "semantic_span_ids": [],
        "storage_contribution_ids": [],
        "verification_target_ids": [],
    }


_OWNER_FINDING_PREFIX_RE = re.compile(r"^owners\[(\d+)\]")
_OWNER_FINDING_ADDRESS_RE = re.compile(r"0x[0-9a-fA-F]+")


def _normalized_canonical_owner_findings(
    data: Mapping[str, Any],
) -> list[dict[str, Any]]:
    """Return stable, owner-addressable findings from the canonical validator."""

    from _recoil.lib.source_owners import SourceOwnerDocument, _project_progress_owners

    try:
        projected = _project_progress_owners(deepcopy(dict(data)))
        findings = SourceOwnerDocument(
            Path("<owner-replace-no-new-debt>"), projected
        ).validate()
    except (ValueError, TypeError, KeyError) as exc:
        return [
            {
                "text": f"canonical-owner-projection-error: {type(exc).__name__}: {exc}",
                "owner_id": None,
                "addresses": frozenset(),
                "global": True,
            }
        ]

    raw_owners = data.get("owners")
    canonical_owner_ids = (
        [
            str(owner_id)
            for owner_id, owner in raw_owners.items()
            if isinstance(owner, dict)
        ]
        if isinstance(raw_owners, Mapping)
        else []
    )
    normalized: list[dict[str, Any]] = []
    for raw_finding in findings:
        finding = str(raw_finding)
        match = _OWNER_FINDING_PREFIX_RE.match(finding)
        owner_id: str | None = None
        global_finding = match is None
        if match is not None:
            index = int(match.group(1))
            if index < len(canonical_owner_ids):
                owner_id = canonical_owner_ids[index]
                finding = (
                    f"owners[{owner_id}]" + finding[match.end() :]
                )
            else:
                global_finding = True
        addresses: set[str] = set()
        for raw_address in _OWNER_FINDING_ADDRESS_RE.findall(finding):
            try:
                addresses.add(normalize_address(raw_address))
            except ValueError:
                global_finding = True
        normalized.append(
            {
                "text": finding,
                "owner_id": owner_id,
                "addresses": frozenset(addresses),
                "global": global_finding,
            }
        )
    return normalized


def _owner_replacement_touched_scope(
    *,
    data: Mapping[str, Any],
    payload: Mapping[str, Any],
    current: Mapping[str, Mapping[str, Any]],
    replacement: Mapping[str, Mapping[str, Any]],
) -> dict[str, set[str]]:
    owner_ids = set(current) | set(replacement)
    symbol_ids: set[str] = set()
    addresses: set[str] = set()

    for field in (
        "primary_function_bootstraps",
        "primary_function_detachments",
        "primary_data_reassignments",
        "unknown_data_symbol_bootstraps",
    ):
        for row in payload.get(field, []):
            if not isinstance(row, Mapping):
                continue
            symbol_id = row.get("symbol_id")
            if isinstance(symbol_id, str):
                symbol_ids.add(symbol_id)
            for owner_field in ("current_owner_id", "new_owner_id"):
                owner_id = row.get(owner_field)
                if isinstance(owner_id, str) and owner_id:
                    owner_ids.add(owner_id)
            address = row.get("address")
            if isinstance(address, str):
                addresses.add(normalize_address(address))

    for record in [*current.values(), *replacement.values()]:
        relationships = record.get("relationships")
        if not isinstance(relationships, list):
            continue
        for relationship in relationships:
            if not isinstance(relationship, Mapping):
                continue
            symbol_id = relationship.get("symbol_id")
            if isinstance(symbol_id, str) and symbol_id:
                symbol_ids.add(symbol_id)
            address = relationship.get("address")
            if isinstance(address, str) and address:
                addresses.add(normalize_address(address))

    symbols = data.get("symbols")
    if isinstance(symbols, Mapping):
        for symbol_id in symbol_ids:
            symbol = symbols.get(symbol_id)
            if isinstance(symbol, Mapping):
                address = symbol.get("address", symbol.get("start"))
                if isinstance(address, str):
                    addresses.add(normalize_address(address))

    owner_tokens = set(owner_ids)
    owners = data.get("owners")
    if isinstance(owners, Mapping):
        for owner_id in owner_ids:
            owner = owners.get(owner_id)
            if isinstance(owner, Mapping):
                legacy_id = owner.get("legacy_id")
                if isinstance(legacy_id, str) and legacy_id:
                    owner_tokens.add(legacy_id)
                elif ":owner:" in owner_id:
                    owner_tokens.add(owner_id.split(":owner:", 1)[-1])
    return {
        "owner_ids": owner_ids,
        "owner_tokens": owner_tokens,
        "symbol_ids": symbol_ids,
        "addresses": addresses,
    }


def _validate_owner_replacement_no_new_debt(
    *,
    before_findings: list[dict[str, Any]],
    after_data: Mapping[str, Any],
    touched_scope: Mapping[str, set[str]],
    operation_label: str = "owner replacement",
) -> dict[str, int]:
    after_findings = _normalized_canonical_owner_findings(after_data)

    def intersects(finding: Mapping[str, Any]) -> bool:
        text = str(finding["text"])
        return bool(
            finding.get("global")
            or finding.get("owner_id") in touched_scope["owner_ids"]
            or set(finding.get("addresses", ())) & touched_scope["addresses"]
            or any(token and token in text for token in touched_scope["owner_tokens"])
            or any(symbol_id in text for symbol_id in touched_scope["symbol_ids"])
        )

    touched_before = [row["text"] for row in before_findings if intersects(row)]
    touched_after = [row["text"] for row in after_findings if intersects(row)]
    if touched_before or touched_after:
        raise ProgressError(
            f"{operation_label} canonical findings intersect touched owner/symbol/address "
            f"scope: before={touched_before[:6]}, after={touched_after[:6]}"
        )

    before_counter = Counter(str(row["text"]) for row in before_findings)
    after_counter = Counter(str(row["text"]) for row in after_findings)
    if before_counter != after_counter:
        introduced = sorted((after_counter - before_counter).elements())
        removed_or_changed = sorted((before_counter - after_counter).elements())
        raise ProgressError(
            f"{operation_label} changed unrelated pre-existing canonical findings; "
            f"introduced={introduced[:6]}, removed_or_changed={removed_or_changed[:6]}"
        )
    return {
        "preserved_unrelated_finding_count": sum(before_counter.values()),
        "touched_finding_count": 0,
    }


def _authored_storage_touched_owner_scope(
    *,
    data: Mapping[str, Any],
    payload: Mapping[str, Any],
) -> dict[str, set[str]]:
    owner_id = str(payload["owner_id"])
    symbol_id = str(payload["symbol_id"])
    address = normalize_address(str(payload["expected_symbol"]["address"]))
    owner_tokens = {owner_id}
    owners = data.get("owners")
    owner = owners.get(owner_id) if isinstance(owners, Mapping) else None
    if isinstance(owner, Mapping):
        legacy_id = owner.get("legacy_id")
        if isinstance(legacy_id, str) and legacy_id:
            owner_tokens.add(legacy_id)
        elif ":owner:" in owner_id:
            owner_tokens.add(owner_id.split(":owner:", 1)[-1])
    return {
        "owner_ids": {owner_id},
        "owner_tokens": owner_tokens,
        "symbol_ids": {symbol_id},
        "addresses": {address},
    }


def _mutate_authored_storage_no_new_debt(
    tracker_path: str | Path,
    payload: Mapping[str, Any],
    *,
    expected_revision: int,
    apply: bool,
) -> dict[str, Any]:
    """Run exact storage registration while preserving canonical unrelated debt."""

    store = _authored_storage_progress.tracker_store(tracker_path)
    current = store.load()
    if current.get("revision") != expected_revision:
        raise ConcurrentRevisionUpdate(
            f"revision changed: expected {expected_revision}, "
            f"found {current.get('revision')}"
        )
    before_findings = _normalized_canonical_owner_findings(current)
    touched_scope = _authored_storage_touched_owner_scope(
        data=current,
        payload=payload,
    )
    owner_debt: dict[str, int] = {}

    original_validator = _authored_storage_progress.validate_owner_invariants
    if original_validator is not validate_owner_invariants:
        raise ProgressError(
            "authored storage canonical owner validator binding drifted"
        )

    def validate_scoped_owner_invariants(after_data: Mapping[str, Any]) -> None:
        owner_debt.update(
            _validate_owner_replacement_no_new_debt(
                before_findings=before_findings,
                after_data=after_data,
                touched_scope=touched_scope,
                operation_label="authored storage registration",
            )
        )

    # The storage planner owns every exact target/storage guard. Its sole
    # whole-tracker owner check is injected for this synchronous CLI call and
    # restored even when planning or CAS commit fails.
    _authored_storage_progress.validate_owner_invariants = (
        validate_scoped_owner_invariants
    )
    try:
        result = _authored_storage_progress.mutate_authored_storage(
            tracker_path,
            payload,
            expected_revision=expected_revision,
            apply=apply,
        )
    finally:
        _authored_storage_progress.validate_owner_invariants = original_validator
    if not owner_debt:
        raise ProgressError(
            "authored storage planner did not run the canonical owner invariant"
        )
    return {
        **result,
        "owner_invariants_passed": True,
        "owner_invariant_mode": "no-introduced-debt",
        **owner_debt,
    }


def _replace_owner_batch(data: dict[str, Any], payload: Mapping[str, Any]) -> dict[str, Any]:
    owners = data.get("owners")
    symbols = data.get("symbols")
    if not isinstance(owners, dict) or not isinstance(symbols, dict):
        raise ProgressError("owner replacement batch requires tracker owners and symbols collections")
    binary = str(payload["binary"])
    current = {str(row["id"]): deepcopy(row["record"]) for row in payload["current_owners"]}
    replacement = {
        str(row["id"]): deepcopy(row["record"]) for row in payload["replacement_owners"]
    }
    for owner_id, expected in current.items():
        actual = owners.get(owner_id)
        if not isinstance(actual, Mapping):
            raise ProgressError(f"owner replacement current owner does not exist: {owner_id!r}")
        if dict(actual) != expected:
            raise ProgressError(f"owner replacement current snapshot is stale for {owner_id!r}")
    unguarded_existing = sorted(
        owner_id for owner_id in replacement if owner_id in owners and owner_id not in current
    )
    if unguarded_existing:
        raise ProgressError(
            "owner replacement attempts to update existing owners without exact current snapshots: "
            + ", ".join(unguarded_existing)
        )
    if current == replacement:
        raise ProgressError("owner replacement batch is a no-op")

    before_owner_findings = _normalized_canonical_owner_findings(data)
    touched_owner_scope = _owner_replacement_touched_scope(
        data=data,
        payload=payload,
        current=current,
        replacement=replacement,
    )
    before_owners = deepcopy(owners)
    before_primary = _owner_primary_function_map(
        before_owners, symbols, binary=binary, label="current owner graph"
    )
    before_primary_data = _owner_primary_data_map(
        before_owners, symbols, binary=binary, label="current owner graph"
    )
    function_bootstraps = {
        str(row["symbol_id"]): row for row in payload.get("primary_function_bootstraps", [])
    }
    function_detachments = {
        str(row["symbol_id"]): row
        for row in payload.get("primary_function_detachments", [])
    }
    overlapping_function_changes = sorted(
        set(function_bootstraps) & set(function_detachments)
    )
    if overlapping_function_changes:
        raise ProgressError(
            "owner replacement primary-function rows may not both bootstrap and "
            "detach the same symbol: " + ", ".join(overlapping_function_changes)
        )
    data_reassignments = {
        str(row["symbol_id"]): row for row in payload.get("primary_data_reassignments", [])
    }
    data_symbol_bootstraps = {
        str(row["symbol_id"]): row for row in payload.get("unknown_data_symbol_bootstraps", [])
    }
    before_symbol_ownership_states = {
        symbol_id: row.get("ownership_state")
        for symbol_id, row in symbols.items()
        if isinstance(row, Mapping)
    }
    for symbol_id, row in function_bootstraps.items():
        if not symbol_id.startswith("recoil:function:"):
            raise ProgressError(f"primary function bootstrap id must be recoil:function:...: {symbol_id!r}")
        symbol = symbols.get(symbol_id)
        if not isinstance(symbol, Mapping):
            raise ProgressError(f"primary function bootstrap references unknown symbol {symbol_id!r}")
        if symbol.get("binary") != binary or symbol.get("kind") not in {
            "function", "provider-function", "compiler-function"
        }:
            raise ProgressError(f"primary function bootstrap {symbol_id!r} is not an exact callable")
        if normalize_address(str(symbol.get("address"))) != row["address"]:
            raise ProgressError(f"primary function bootstrap {symbol_id!r} address is stale")
        if symbol.get("ownership_state") != row["current_ownership_state"]:
            raise ProgressError(
                f"primary function bootstrap {symbol_id!r} ownership_state is stale"
            )
        if row["current_ownership_state"] not in {None, "unresolved"}:
            raise ProgressError(
                f"primary function bootstrap {symbol_id!r} is not exact currently-unowned state"
            )
        if symbol_id in before_primary:
            raise ProgressError(f"primary function bootstrap {symbol_id!r} is already owned")
        if row["new_owner_id"] not in replacement:
            raise ProgressError(
                f"primary function bootstrap {symbol_id!r} target is not an explicit replacement owner"
            )
    for symbol_id, row in function_detachments.items():
        if not symbol_id.startswith("recoil:function:"):
            raise ProgressError(
                f"primary function detachment id must be recoil:function:...: {symbol_id!r}"
            )
        symbol = symbols.get(symbol_id)
        if not isinstance(symbol, Mapping):
            raise ProgressError(
                f"primary function detachment references unknown symbol {symbol_id!r}"
            )
        if symbol.get("binary") != binary or symbol.get("kind") not in {
            "function", "compiler-function"
        }:
            raise ProgressError(
                f"primary function detachment {symbol_id!r} is not an exact Recoil physical callable"
            )
        if normalize_address(str(symbol.get("address"))) != row["address"]:
            raise ProgressError(
                f"primary function detachment {symbol_id!r} address is stale"
            )
        for field in (
            "ownership_state",
            "pipeline_class",
            "authored_order_role",
        ):
            expected_field = f"current_{field}"
            if symbol.get(field) != row[expected_field]:
                raise ProgressError(
                    f"primary function detachment {symbol_id!r} {field} is stale"
                )
        if row["current_ownership_state"] != "primary-owned":
            raise ProgressError(
                f"primary function detachment {symbol_id!r} must preserve established "
                "ownership_state='primary-owned'"
            )
        current_classification = (
            row["current_pipeline_class"],
            row["current_authored_order_role"],
        )
        if current_classification not in _PRIMARY_FUNCTION_DETACHMENT_CLASSIFICATIONS:
            raise ProgressError(
                f"primary function detachment {symbol_id!r} requires an already-reviewed "
                "non-authored/non-authored or "
                "non-authored/compiler-generated-icf-representative classification"
            )
        current_owner_id = row["current_owner_id"]
        if before_primary.get(symbol_id) != current_owner_id:
            raise ProgressError(
                f"primary function detachment {symbol_id!r} current owner is stale"
            )
        if current_owner_id not in current or current_owner_id not in replacement:
            raise ProgressError(
                f"primary function detachment {symbol_id!r} requires exact current and "
                "replacement snapshots for its retained owner"
            )
    output_sections = data.get("output_sections")
    for symbol_id, row in data_symbol_bootstraps.items():
        if symbol_id in symbols:
            raise ProgressError(f"unknown data symbol bootstrap already exists: {symbol_id!r}")
        if symbol_id not in data_reassignments:
            raise ProgressError(
                f"unknown data symbol bootstrap {symbol_id!r} lacks an exact primary-data assignment"
            )
        output_section_id = row["output_section_id"]
        if output_section_id is not None:
            section = output_sections.get(output_section_id) if isinstance(output_sections, Mapping) else None
            if not isinstance(section, Mapping) or section.get("binary") != binary:
                raise ProgressError(
                    f"unknown data symbol bootstrap {symbol_id!r} output section is not registered"
                )
        symbols[symbol_id] = _unknown_data_symbol_record(row, binary=binary)
    for owner_id in current:
        del owners[owner_id]
    for owner_id, record in replacement.items():
        owners[owner_id] = deepcopy(record)
    after_primary = _owner_primary_function_map(
        owners, symbols, binary=binary, label="replacement owner graph"
    )
    after_primary_data = _owner_primary_data_map(
        owners, symbols, binary=binary, label="replacement owner graph"
    )
    expected_function_ids = (
        set(before_primary) | set(function_bootstraps)
    ) - set(function_detachments)
    if expected_function_ids != set(after_primary):
        missing = sorted(set(before_primary) - set(after_primary))
        extra = sorted(set(after_primary) - expected_function_ids)
        raise ProgressError(
            "owner replacement has partial primary-function membership; "
            f"missing={missing}, extra={extra}"
        )
    reassigned = sorted(
        symbol_id
        for symbol_id in after_primary
        if before_primary.get(symbol_id) != after_primary[symbol_id]
    )
    detached = sorted(
        symbol_id
        for symbol_id in function_detachments
        if symbol_id in before_primary and symbol_id not in after_primary
    )
    if set(detached) != set(function_detachments):
        arbitrary = sorted(set(function_detachments) - set(detached))
        raise ProgressError(
            "owner replacement primary-function detachments do not match the exact "
            f"reviewed rows: unused={arbitrary}"
        )
    if not reassigned and payload["schema"] == _OWNER_REPLACE_BATCH_SCHEMA_V1:
        raise ProgressError("owner replacement batch does not reassign any primary functions")
    if any(
        symbol_id not in function_bootstraps and before_primary[symbol_id] not in current
        for symbol_id in reassigned
    ):
        raise ProgressError("owner replacement changes a primary function from an unguarded owner")
    if any(after_primary[symbol_id] not in replacement for symbol_id in reassigned):
        raise ProgressError("owner replacement changes a primary function to an undeclared owner")
    for symbol_id, row in function_bootstraps.items():
        if after_primary.get(symbol_id) != row["new_owner_id"]:
            raise ProgressError(
                f"primary function bootstrap {symbol_id!r} does not match its exact replacement owner"
            )
        symbols[symbol_id]["ownership_state"] = "primary-owned"

    missing_data = sorted(set(before_primary_data) - set(after_primary_data))
    if missing_data:
        raise ProgressError("owner replacement loses primary-data relationships: " + ", ".join(missing_data))
    changed_data = sorted(
        symbol_id
        for symbol_id in after_primary_data
        if before_primary_data.get(symbol_id) != after_primary_data[symbol_id]
    )
    if set(changed_data) != set(data_reassignments):
        arbitrary = sorted(set(changed_data) - set(data_reassignments))
        unused = sorted(set(data_reassignments) - set(changed_data))
        raise ProgressError(
            "owner replacement primary-data changes do not match the exact reviewed rows; "
            f"arbitrary={arbitrary}, unused={unused}"
        )
    if not reassigned and not detached and not changed_data:
        raise ProgressError(
            "owner replacement batch does not reassign any primary functions or primary data"
        )
    unowned_authored_data_tier_x_ids: set[str] = set()
    for symbol_id, row in data_reassignments.items():
        symbol = symbols.get(symbol_id)
        if not isinstance(symbol, Mapping) or symbol.get("binary") != binary or symbol.get("kind") != "data":
            raise ProgressError(f"primary-data reassignment references unknown data symbol {symbol_id!r}")
        if normalize_address(str(symbol.get("address"))) != row["address"]:
            raise ProgressError(f"primary-data reassignment {symbol_id!r} address is stale")
        observed_ownership_state = before_symbol_ownership_states.get(symbol_id)
        if observed_ownership_state != row["current_ownership_state"]:
            raise ProgressError(f"primary-data reassignment {symbol_id!r} ownership_state is stale")
        if before_primary_data.get(symbol_id) != row["current_owner_id"]:
            raise ProgressError(f"primary-data reassignment {symbol_id!r} current owner is stale")
        if row["current_owner_id"] is not None and row["current_owner_id"] not in current:
            raise ProgressError(
                f"primary-data reassignment {symbol_id!r} changes an unguarded owner"
            )
        if row["new_owner_id"] not in replacement:
            raise ProgressError(
                f"primary-data reassignment {symbol_id!r} target is not an explicit replacement owner"
            )
        if after_primary_data.get(symbol_id) != row["new_owner_id"]:
            raise ProgressError(
                f"primary-data reassignment {symbol_id!r} does not match its exact replacement owner"
            )
        if (
            row["current_owner_id"] is None
            and symbol_id not in data_symbol_bootstraps
            and symbol.get("disposition") == "authored"
        ):
            if row["current_ownership_state"] not in {None, "unresolved"}:
                raise ProgressError(
                    f"unowned authored primary-data reassignment {symbol_id!r} "
                    "is not exact currently-unowned state"
                )
            destination_owner_id = str(row["new_owner_id"])
            current_destination = current.get(destination_owner_id)
            replacement_destination = replacement.get(destination_owner_id)
            if (
                not isinstance(current_destination, Mapping)
                or not isinstance(replacement_destination, Mapping)
            ):
                raise ProgressError(
                    f"unowned authored primary-data reassignment {symbol_id!r} "
                    "must target an explicit retained owner"
                )
            if (
                current_destination.get("kind") == "provider-boundary"
                or replacement_destination.get("kind") == "provider-boundary"
            ):
                raise ProgressError(
                    f"unowned authored primary-data reassignment {symbol_id!r} "
                    "rejects a provider destination"
                )
            current_reimplementation = current_destination.get(
                "reimplementation"
            )
            current_entries = (
                current_reimplementation.get("entries")
                if isinstance(current_reimplementation, Mapping)
                else None
            )
            if (
                not isinstance(current_entries, Mapping)
                or symbol_id in current_entries
            ):
                raise ProgressError(
                    f"unowned authored primary-data reassignment {symbol_id!r} "
                    "requires a retained destination with no existing entry"
                )
            unowned_authored_data_tier_x_ids.add(symbol_id)
        symbols[symbol_id]["ownership_state"] = "primary-owned"
    if len(unowned_authored_data_tier_x_ids) > 1:
        raise ProgressError(
            "owner replacement may bootstrap exactly one formerly-unowned "
            "authored primary-data tier-X entry"
        )
    if set(data_symbol_bootstraps) - set(changed_data):
        raise ProgressError("unknown data symbol bootstrap is not used by the proposed owner graph")

    storage_reassignments: list[dict[str, str]] = []
    storage_rows = data.get("storage_contributions")
    if not isinstance(storage_rows, dict):
        raise ProgressError("owner replacement requires storage_contributions for primary-data changes")
    for symbol_id in changed_data:
        row = data_reassignments[symbol_id]
        symbol = symbols[symbol_id]
        contribution_ids = symbol.get("storage_contribution_ids", [])
        if not isinstance(contribution_ids, list):
            raise ProgressError(f"primary-data symbol {symbol_id!r} storage_contribution_ids must be an array")
        for contribution_id in contribution_ids:
            storage = storage_rows.get(contribution_id)
            if not isinstance(storage, dict):
                raise ProgressError(
                    f"primary-data symbol {symbol_id!r} references unknown storage {contribution_id!r}"
                )
            owner_ids = storage.get("owner_ids")
            if not isinstance(owner_ids, list) or any(not isinstance(item, str) for item in owner_ids):
                raise ProgressError(f"storage {contribution_id!r} owner_ids must be a string array")
            old_owner_id = row["current_owner_id"]
            new_owner_id = row["new_owner_id"]
            if old_owner_id is None:
                if new_owner_id in owner_ids:
                    raise ProgressError(
                        f"storage {contribution_id!r} already references bootstrap target owner"
                    )
                replacement_owner_ids = [*owner_ids, new_owner_id]
            else:
                if owner_ids.count(old_owner_id) != 1:
                    raise ProgressError(
                        f"storage {contribution_id!r} does not exactly reference current owner {old_owner_id!r}"
                    )
                replacement_owner_ids = [
                    new_owner_id if item == old_owner_id else item for item in owner_ids
                ]
            if len(replacement_owner_ids) != len(set(replacement_owner_ids)):
                raise ProgressError(f"storage {contribution_id!r} owner reassignment would duplicate owners")
            storage["owner_ids"] = replacement_owner_ids
            storage_reassignments.append(
                {
                    "storage_contribution_id": str(contribution_id),
                    "symbol_id": symbol_id,
                    "old_owner_id": str(old_owner_id) if old_owner_id is not None else "",
                    "new_owner_id": str(new_owner_id),
                }
            )

    retired_ids = sorted(set(current) - set(replacement))
    created_ids = sorted(set(replacement) - set(current))
    updated_ids = sorted(set(current) & set(replacement))
    dependency_retargets: list[dict[str, str]] = []
    for owner_id in retired_ids:
        remaining = sorted(
            symbol_id for symbol_id, source_owner in before_primary.items()
            if source_owner == owner_id and symbol_id not in reassigned
        )
        if remaining:
            raise ProgressError(
                f"retired owner {owner_id!r} retains partial primary membership: {remaining}"
            )
        remaining_data = sorted(
            symbol_id
            for symbol_id, source_owner in before_primary_data.items()
            if source_owner == owner_id and symbol_id not in changed_data
        )
        if remaining_data:
            raise ProgressError(
                f"retired owner {owner_id!r} retains partial primary-data membership: {remaining_data}"
            )

    for owner_id in updated_ids:
        old = current[owner_id]
        new = replacement[owner_id]
        for field in (set(old) | set(new)) - {"relationships", "reimplementation", "address_metadata"}:
            if old.get(field) != new.get(field):
                raise ProgressError(
                    f"updated owner {owner_id!r} changes unrelated field {field!r}"
                )
        dependency_retargets.extend(
            _review_dependency_retargets(
                owner_id=owner_id,
                old_owner=old,
                new_owner=new,
                retired_owner_ids=set(retired_ids),
                replacement_owner_ids=set(replacement),
            )
        )
        moved_out = {
            symbol_id
            for symbol_id in reassigned
            if before_primary.get(symbol_id) == owner_id
        } | set(detached) & {
            symbol_id
            for symbol_id, source_owner in before_primary.items()
            if source_owner == owner_id
        } | {
            symbol_id
            for symbol_id in changed_data
            if before_primary_data.get(symbol_id) == owner_id
        }
        moved_in = {
            symbol_id for symbol_id in reassigned if after_primary[symbol_id] == owner_id
        } | {
            symbol_id for symbol_id in changed_data if after_primary_data[symbol_id] == owner_id
        }
        expected_reimplementation = deepcopy(old.get("reimplementation"))
        if isinstance(expected_reimplementation, dict):
            expected_entries = expected_reimplementation.get("entries")
            if isinstance(expected_entries, dict):
                for symbol_id in moved_out:
                    expected_entries.pop(symbol_id, None)
                for symbol_id in moved_in:
                    source_owner_id = before_primary.get(
                        symbol_id, before_primary_data.get(symbol_id)
                    )
                    if symbol_id in unowned_authored_data_tier_x_ids:
                        if symbol_id in expected_entries:
                            raise ProgressError(
                                f"updated owner {owner_id!r} already carries "
                                f"formerly-unowned primary-data entry {symbol_id!r}"
                            )
                        expected_entries[symbol_id] = {
                            "kind": "data",
                            "tier": "X",
                            "evidence_ids": [],
                        }
                        continue
                    if (
                        source_owner_id is None
                        or source_owner_id not in updated_ids
                        or new.get("kind") == "provider-boundary"
                    ):
                        continue
                    source_reimplementation = current[source_owner_id].get(
                        "reimplementation"
                    )
                    source_entries = (
                        source_reimplementation.get("entries")
                        if isinstance(source_reimplementation, Mapping)
                        else None
                    )
                    source_entry = (
                        source_entries.get(symbol_id)
                        if isinstance(source_entries, Mapping)
                        else None
                    )
                    if not isinstance(source_entry, Mapping):
                        raise ProgressError(
                            f"updated owner {owner_id!r} moved-in primary entry "
                            f"{symbol_id!r} has no exact reimplementation record "
                            f"on retained source owner {source_owner_id!r}"
                        )
                    if symbol_id in expected_entries:
                        raise ProgressError(
                            f"updated owner {owner_id!r} already carries moved-in "
                            f"primary entry tier {symbol_id!r}"
                        )
                    expected_entries[symbol_id] = deepcopy(dict(source_entry))
        if new.get("reimplementation") != expected_reimplementation:
            raise ProgressError(
                f"updated owner {owner_id!r} must remove exactly the stale tiers for moved primary entries"
            )
        old_metadata = old.get("address_metadata", {})
        new_metadata = new.get("address_metadata", {})
        if not isinstance(old_metadata, Mapping) or not isinstance(new_metadata, Mapping):
            raise ProgressError(f"updated owner {owner_id!r} address_metadata must be an object")
        moved_out_addresses = {
            normalize_address(str(symbols[symbol_id]["address"])) for symbol_id in moved_out
        }
        moved_in_addresses = {
            normalize_address(str(symbols[symbol_id]["address"])) for symbol_id in moved_in
        }
        preserved_metadata = {
            key: deepcopy(value)
            for key, value in old_metadata.items()
            if normalize_address(str(key)) not in moved_out_addresses
        }
        actual_preserved = {
            key: deepcopy(value)
            for key, value in new_metadata.items()
            if normalize_address(str(key)) not in moved_in_addresses
        }
        if actual_preserved != preserved_metadata:
            raise ProgressError(
                f"updated owner {owner_id!r} must preserve unrelated address metadata"
            )

    for owner_id in created_ids:
        row = replacement[owner_id]
        if row.get("kind") == "provider-boundary":
            reimplementation = row.get("reimplementation")
            if reimplementation not in (None, {"entries": {}}):
                raise ProgressError(
                    f"created provider owner {owner_id!r} may not carry reimplementation tiers"
                )

    owner_debt = _validate_owner_replacement_no_new_debt(
        before_findings=before_owner_findings,
        after_data=data,
        touched_scope=touched_owner_scope,
    )
    return {
        "kind": "owner-replace-batch",
        "schema": str(payload["schema"]),
        "reviewed": True,
        "parent_reviewed": True,
        "reason": str(payload["reason"]),
        "binary": binary,
        "created_owner_ids": created_ids,
        "updated_owner_ids": updated_ids,
        "retired_owner_ids": retired_ids,
        "reassigned_primary_function_ids": reassigned,
        "detached_primary_function_ids": detached,
        "bootstrapped_primary_function_ids": sorted(function_bootstraps),
        "reassigned_primary_data_ids": changed_data,
        "bootstrapped_unowned_primary_data_tier_x_ids": sorted(
            unowned_authored_data_tier_x_ids
        ),
        "bootstrapped_unknown_data_symbol_ids": sorted(data_symbol_bootstraps),
        "storage_owner_reassignments": storage_reassignments,
        "dependency_retargets": dependency_retargets,
        "primary_membership_preserved": not detached,
        "primary_detachments_reviewed": bool(detached),
        "owner_invariants_passed": True,
        "owner_invariant_mode": "no-introduced-debt",
        **owner_debt,
    }


def _closed_open_payload_range(value: Any, *, label: str) -> tuple[int, int]:
    if not isinstance(value, str) or not value.startswith("[") or not value.endswith(")"):
        raise ProgressError(f"{label} must be a closed-open address range")
    parts = value[1:-1].split(",")
    if len(parts) != 2:
        raise ProgressError(f"{label} must be a closed-open address range")
    start = address_value(
        _normalize_payload_address(parts[0].strip(), label=f"{label}.start")
    )
    end = address_value(
        _normalize_payload_address(parts[1].strip(), label=f"{label}.end_exclusive")
    )
    if end <= start:
        raise ProgressError(f"{label} has an empty or reversed interval")
    return start, end


def _parse_physical_block_replace_payload(
    payload_json: str,
    *,
    source_label: str = "--payload-json",
) -> dict[str, Any]:
    try:
        raw = json.loads(payload_json)
    except json.JSONDecodeError as exc:
        raise ProgressError(f"{source_label} is not valid JSON: {exc.msg}") from exc
    if not isinstance(raw, Mapping):
        raise ProgressError(
            f"{source_label} must be one physical-block replacement object"
        )
    payload = _require_exact_payload_fields(
        raw,
        _PHYSICAL_BLOCK_REPLACE_FIELDS,
        label="physical-block replacement payload",
    )
    if payload["schema"] != _PHYSICAL_BLOCK_REPLACE_SCHEMA:
        raise ProgressError(
            f"physical-block replacement schema must be {_PHYSICAL_BLOCK_REPLACE_SCHEMA!r}"
        )
    if payload["reviewed"] is not True or payload["parent_reviewed"] is not True:
        raise ProgressError(
            "physical-block replacement requires reviewed=true and parent_reviewed=true"
        )
    payload["reason"] = _require_payload_string(payload["reason"], label="reason")
    payload["binary"] = _require_payload_string(payload["binary"], label="binary")

    current_raw = payload["current_block"]
    if not isinstance(current_raw, Mapping):
        raise ProgressError("current_block must be an object")
    current_raw = dict(current_raw)
    current_raw.setdefault("source_shape_inputs", [])
    current_raw.setdefault("candidate_header_contributors", [])
    current = _require_exact_payload_fields(
        current_raw,
        _CURRENT_BLOCK_GUARD_FIELDS,
        label="current_block",
    )
    for field in ("id", "source_path", "agent_source_path", "mapping_state", "mapping_status"):
        current[field] = _require_payload_string(
            current[field],
            label=f"current_block.{field}",
        )
    for field in ("original_source_path", "provisional_original_path"):
        if current[field] is not None and not isinstance(current[field], str):
            raise ProgressError(f"current_block.{field} must be a string or null")
    current["start"] = _normalize_payload_address(
        current["start"],
        label="current_block.start",
    )
    current["end_exclusive"] = _normalize_payload_address(
        current["end_exclusive"],
        label="current_block.end_exclusive",
    )
    current["contribution_ids"] = _require_unique_string_list(
        current["contribution_ids"],
        label="current_block.contribution_ids",
    )
    current["semantic_span_ids"] = _require_unique_string_list(
        current["semantic_span_ids"],
        label="current_block.semantic_span_ids",
    )
    current["source_shape_inputs"] = _require_unique_relationship_rows(
        current["source_shape_inputs"],
        label="current_block.source_shape_inputs",
    )
    current["candidate_header_contributors"] = _require_unique_relationship_rows(
        current["candidate_header_contributors"],
        label="current_block.candidate_header_contributors",
    )
    payload["current_block"] = current

    raw_blocks = payload["replacement_blocks"]
    if not isinstance(raw_blocks, list) or not raw_blocks:
        raise ProgressError("replacement_blocks must be a non-empty array")
    replacement_blocks: list[dict[str, Any]] = []
    block_ids: set[str] = set()
    for index, raw_block in enumerate(raw_blocks):
        if not isinstance(raw_block, Mapping):
            raise ProgressError(f"replacement_blocks[{index}] must be an object")
        raw_block = dict(raw_block)
        raw_block.setdefault("source_shape_inputs", [])
        raw_block.setdefault("candidate_header_contributors", [])
        block = _require_exact_payload_fields(
            raw_block,
            _REPLACEMENT_BLOCK_FIELDS,
            label=f"replacement_blocks[{index}]",
        )
        for field in (
            "id",
            "source_path",
            "agent_source_path",
            "mapping_state",
            "mapping_status",
            "mapping_confidence",
        ):
            block[field] = _require_payload_string(
                block[field],
                label=f"replacement_blocks[{index}].{field}",
            )
        if block["original_source_path"] is not None or block["provisional_original_path"] is not None:
            raise ProgressError(
                f"replacement_blocks[{index}] must preserve unresolved original provenance "
                "with null original_source_path and provisional_original_path"
            )
        if block["mapping_state"] != "unresolved":
            raise ProgressError(
                f"replacement_blocks[{index}].mapping_state must remain 'unresolved'"
            )
        block["start"] = _normalize_payload_address(
            block["start"],
            label=f"replacement_blocks[{index}].start",
        )
        block["end_exclusive"] = _normalize_payload_address(
            block["end_exclusive"],
            label=f"replacement_blocks[{index}].end_exclusive",
        )
        block["contribution_ids"] = _require_unique_string_list(
            block["contribution_ids"],
            label=f"replacement_blocks[{index}].contribution_ids",
        )
        block["semantic_span_ids"] = _require_unique_string_list(
            block["semantic_span_ids"],
            label=f"replacement_blocks[{index}].semantic_span_ids",
        )
        block["source_shape_inputs"] = _require_unique_relationship_rows(
            block["source_shape_inputs"],
            label=f"replacement_blocks[{index}].source_shape_inputs",
        )
        block["candidate_header_contributors"] = _require_unique_relationship_rows(
            block["candidate_header_contributors"],
            label=f"replacement_blocks[{index}].candidate_header_contributors",
        )
        if block["id"] in block_ids:
            raise ProgressError(f"duplicate replacement block id {block['id']!r}")
        block_ids.add(block["id"])
        replacement_blocks.append(block)
    payload["replacement_blocks"] = replacement_blocks

    raw_spans = payload["replacement_semantic_spans"]
    if not isinstance(raw_spans, list) or not raw_spans:
        raise ProgressError("replacement_semantic_spans must be a non-empty array")
    replacement_spans: list[dict[str, Any]] = []
    span_ids: set[str] = set()
    for index, raw_span in enumerate(raw_spans):
        if not isinstance(raw_span, Mapping):
            raise ProgressError(f"replacement_semantic_spans[{index}] must be an object")
        span = _require_exact_payload_fields(
            raw_span,
            _REPLACEMENT_SEMANTIC_SPAN_FIELDS,
            label=f"replacement_semantic_spans[{index}]",
        )
        for field in ("id", "physical_block_id", "source_path", "status", "confidence"):
            span[field] = _require_payload_string(
                span[field],
                label=f"replacement_semantic_spans[{index}].{field}",
            )
        span["start"] = _normalize_payload_address(
            span["start"],
            label=f"replacement_semantic_spans[{index}].start",
        )
        span["end_exclusive"] = _normalize_payload_address(
            span["end_exclusive"],
            label=f"replacement_semantic_spans[{index}].end_exclusive",
        )
        span["symbol_ids"] = _require_unique_string_list(
            span["symbol_ids"],
            label=f"replacement_semantic_spans[{index}].symbol_ids",
            allow_empty=True,
        )
        if span["id"] in span_ids:
            raise ProgressError(f"duplicate replacement semantic span id {span['id']!r}")
        span_ids.add(span["id"])
        replacement_spans.append(span)
    payload["replacement_semantic_spans"] = replacement_spans
    return payload


def _load_physical_block_replace_payload(args: argparse.Namespace) -> dict[str, Any]:
    if args.payload_file is None:
        return _parse_physical_block_replace_payload(str(args.payload_json))

    payload_path = args.payload_file
    build_root = (REPO_ROOT / "build").resolve()
    resolved_path = (
        (REPO_ROOT / payload_path).resolve()
        if not payload_path.is_absolute()
        else payload_path.resolve()
    )
    try:
        resolved_path.relative_to(build_root)
    except ValueError as exc:
        raise ProgressError(
            "physical-block replacement --payload-file must resolve under workspace build/"
        ) from exc
    try:
        file_size = resolved_path.stat().st_size
    except OSError as exc:
        raise ProgressError(
            f"physical-block replacement payload file is missing or unreadable: {payload_path}"
        ) from exc
    if not resolved_path.is_file():
        raise ProgressError(
            f"physical-block replacement payload file is not a regular file: {payload_path}"
        )
    if file_size > MAX_PROGRESS_PAYLOAD_FILE_BYTES:
        raise ProgressError(
            "physical-block replacement payload file exceeds the "
            f"{MAX_PROGRESS_PAYLOAD_FILE_BYTES}-byte limit: {payload_path}"
        )
    try:
        payload_json = resolved_path.read_bytes().decode("utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        if isinstance(exc, UnicodeDecodeError):
            message = "is not valid UTF-8"
        else:
            message = "is unreadable"
        raise ProgressError(
            f"physical-block replacement payload file {message}: {payload_path}"
        ) from exc
    return _parse_physical_block_replace_payload(
        payload_json,
        source_label="--payload-file",
    )


def _references_exact_string(value: Any, target: str) -> bool:
    if isinstance(value, str):
        return value == target
    if isinstance(value, Mapping):
        return any(_references_exact_string(item, target) for item in value.values())
    if isinstance(value, list):
        return any(_references_exact_string(item, target) for item in value)
    return False


def _entity_interval(row: Mapping[str, Any], *, label: str) -> tuple[int, int]:
    try:
        start = address_value(row["start"] if "start" in row else row["address"])
        end = address_value(row["end_exclusive"])
    except (KeyError, ProgressError, ValueError, TypeError) as exc:
        raise ProgressError(f"{label} requires exact start/address and end_exclusive") from exc
    if end <= start:
        raise ProgressError(f"{label} has an empty or reversed interval")
    return start, end


def _expected_block_id(binary: str, start: str) -> str:
    return f"{binary}:block:{normalize_address(start)}"


def _expected_semantic_span_id(binary: str, start: str, end: str) -> str:
    return f"{binary}:semantic:{normalize_address(start)}-{normalize_address(end)}"


def _validate_contiguous_rows(
    rows: list[dict[str, Any]],
    *,
    start: int,
    end: int,
    label: str,
) -> list[tuple[int, int]]:
    intervals = [
        _entity_interval(row, label=f"{label}[{index}]")
        for index, row in enumerate(rows)
    ]
    if intervals[0][0] != start:
        raise ProgressError(
            f"{label} starts at {normalize_address(intervals[0][0])}, "
            f"not old block start {normalize_address(start)}"
        )
    cursor = start
    for index, (row_start, row_end) in enumerate(intervals):
        if row_start != cursor:
            relation = "gap" if row_start > cursor else "overlap or out-of-order row"
            raise ProgressError(
                f"{label}[{index}] creates a {relation} at {normalize_address(cursor)}"
            )
        cursor = row_end
    if cursor != end:
        raise ProgressError(
            f"{label} ends at {normalize_address(cursor)}, "
            f"not old block end {normalize_address(end)}"
        )
    return intervals


def _validate_global_block_nonoverlap(
    blocks: Mapping[str, Any],
    *,
    binary: str,
) -> None:
    intervals: list[tuple[int, int, str]] = []
    for block_id, raw_block in blocks.items():
        if not isinstance(raw_block, Mapping) or raw_block.get("binary") != binary:
            continue
        start, end = _entity_interval(raw_block, label=f"physical block {block_id}")
        intervals.append((start, end, str(block_id)))
    intervals.sort()
    for previous, current in zip(intervals, intervals[1:]):
        if previous[1] > current[0]:
            raise ProgressError(
                f"physical blocks overlap after replacement: {previous[2]} and {current[2]}"
            )


def _relationship_row_identity(row: Mapping[str, Any]) -> str:
    return json.dumps(dict(row), sort_keys=True, separators=(",", ":"))


def _validate_block_source_relationship_reassignment(
    current: Mapping[str, Any],
    replacement_blocks: list[dict[str, Any]],
    block_intervals: list[tuple[int, int]],
) -> dict[str, int]:
    counts: dict[str, int] = {}
    for field in ("source_shape_inputs", "candidate_header_contributors"):
        expected_rows = current[field]
        expected = {
            _relationship_row_identity(row): row
            for row in expected_rows
        }
        proposed: dict[str, tuple[str, dict[str, Any], tuple[int, int]]] = {}
        for block, interval in zip(replacement_blocks, block_intervals):
            for row in block[field]:
                identity = _relationship_row_identity(row)
                if identity in proposed:
                    raise ProgressError(
                        f"replacement_blocks duplicate a {field} relationship row"
                    )
                proposed[identity] = (block["id"], row, interval)
        missing = sorted(set(expected) - set(proposed))
        extra = sorted(set(proposed) - set(expected))
        if missing or extra:
            details: list[str] = []
            if missing:
                details.append(f"missing {len(missing)} current row(s)")
            if extra:
                details.append(f"altered or added {len(extra)} row(s)")
            raise ProgressError(
                f"replacement_blocks do not provide the complete exact {field} reassignment: "
                + "; ".join(details)
            )
        if field == "candidate_header_contributors":
            for block_id, row, (block_start, block_end) in proposed.values():
                item_start, item_end = _closed_open_payload_range(
                    row.get("range"),
                    label="candidate_header_contributors[].range",
                )
                if not (block_start <= item_start and item_end <= block_end):
                    raise ProgressError(
                        "candidate_header_contributors row has an invalid replacement block "
                        f"target {block_id!r}: its range is not contained by that block"
                    )
        counts[field] = len(expected)
    return counts


def _validate_physical_block_replace_scheduler_transition(
    before_pipeline: Mapping[str, Any],
    after_pipeline: Mapping[str, Any],
    *,
    old_block_id: str,
    old_start: str,
    replacement_block_ids: list[str],
    invalidated: Mapping[str, Any],
    call_contract_dependency_symbol_ids: list[str],
) -> dict[str, Any]:
    before_phase = str(before_pipeline.get("phase", ""))
    after_phase = str(after_pipeline.get("phase", ""))
    before_cursor = str(before_pipeline.get("cursor", ""))
    after_cursor = str(after_pipeline.get("cursor", ""))
    if before_phase == after_phase:
        if before_cursor and before_cursor != after_cursor:
            raise ProgressError(
                "physical-block replacement unexpectedly changed the scheduler cursor"
            )
        return {
            "transition": "unchanged",
            "from_phase": before_phase,
            "to_phase": after_phase,
            "cursor_rule": "exactly-unchanged",
        }

    allowed_regression = (
        before_phase in {"authored-call-contract", "full-function-order"}
        and after_phase == "authored-function-order"
    )
    if not allowed_regression:
        raise ProgressError(
            "physical-block replacement unexpectedly changed the scheduler phase: "
            "only authored-call-contract or full-function-order to "
            "authored-function-order is permitted"
        )

    expected_cursor = normalize_address(old_start)
    before_counts = before_pipeline.get("authored_function_order_counts", {})
    after_counts = after_pipeline.get("authored_function_order_counts", {})
    actual_invalidated = list(invalidated.get("block_ids", []))
    actual_call_contract_invalidated = list(
        invalidated.get("call_contract_symbol_ids", [])
    )
    exact_regression = (
        isinstance(before_counts, Mapping)
        and isinstance(after_counts, Mapping)
        and before_counts.get("remaining") == 0
        and after_counts.get("remaining") == len(replacement_block_ids)
        and after_counts.get("total")
        == before_counts.get("total", 0) + len(replacement_block_ids) - 1
        and after_counts.get("accepted") == before_counts.get("accepted", 0) - 1
        and actual_invalidated == replacement_block_ids
        and actual_call_contract_invalidated
        == call_contract_dependency_symbol_ids
        and after_cursor == expected_cursor
        and str(after_pipeline.get("authored_order_prefix_end", "")) == expected_cursor
        and str(after_pipeline.get("physical_block_id", "")) == old_block_id
    )
    if not exact_regression:
        raise ProgressError(
            "physical-block replacement authored-order regression is not the exact "
            "replacement invalidation transition"
        )
    return {
        "transition": "replacement-authored-order-regression",
        "from_phase": before_phase,
        "to_phase": after_phase,
        "cursor_rule": "first-replacement-block-start",
        "expected_cursor": expected_cursor,
        "invalidated_block_ids": actual_invalidated,
        "invalidated_call_contract_symbol_ids": actual_call_contract_invalidated,
    }


def _physical_block_replace_call_contract_dependencies(
    document: ProgressDocument,
    pipeline: Mapping[str, Any],
    *,
    binary: str,
    old_block_id: str,
) -> list[str]:
    counts = pipeline.get("authored_call_contract_counts", {})
    if not isinstance(counts, Mapping) or not counts.get("stage_enabled"):
        return []
    slices = document.authored_call_contract_slices(binary)
    return list(
        dict.fromkeys(
            symbol_id
            for slice_row in slices
            if old_block_id in slice_row["physical_block_ids"]
            for symbol_id in slice_row["symbol_ids"]
        )
    )


def _invalidate_physical_block_replace_call_contracts(
    data: dict[str, Any],
    *,
    symbol_ids: Iterable[str],
) -> list[str]:
    symbols = data.get("symbols")
    if not isinstance(symbols, dict):
        raise ProgressError(
            "physical-block replacement requires a symbols collection"
        )
    invalidated: list[str] = []
    for raw_symbol_id in symbol_ids:
        symbol_id = str(raw_symbol_id)
        symbol = symbols.get(symbol_id)
        if not isinstance(symbol, dict):
            raise ProgressError(
                f"physical-block replacement call-contract dependency is unknown: "
                f"{symbol_id}"
            )
        binary_state = symbol.get("binary_state")
        if (
            not isinstance(binary_state, dict)
            or CALL_CONTRACT_DIMENSION not in binary_state
        ):
            raise ProgressError(
                "physical-block replacement found an uninitialized call-contract "
                f"dependency: {symbol_id}"
            )
        binary_state[CALL_CONTRACT_DIMENSION] = state_record(
            "pending", "observed", "changed", []
        )
        symbol.pop("accepted_call_contract_facts", None)
        invalidated.append(symbol_id)
    return invalidated


def _replace_physical_block(
    data: dict[str, Any],
    payload: Mapping[str, Any],
) -> dict[str, Any]:
    binary = str(payload["binary"])
    current = dict(payload["current_block"])
    replacement_blocks = [dict(item) for item in payload["replacement_blocks"]]
    replacement_spans = [dict(item) for item in payload["replacement_semantic_spans"]]
    blocks = data.get("physical_blocks")
    spans = data.get("semantic_spans")
    symbols = data.get("symbols")
    if not isinstance(blocks, dict) or not isinstance(spans, dict) or not isinstance(symbols, dict):
        raise ProgressError(
            "physical-block replacement requires physical_blocks, semantic_spans, and symbols collections"
        )

    old_block_id = str(current["id"])
    old_block = blocks.get(old_block_id)
    if not isinstance(old_block, dict):
        raise ProgressError(f"unknown current physical block {old_block_id!r}")
    if old_block.get("binary") != binary:
        raise ProgressError(
            f"current physical block {old_block_id!r} belongs to {old_block.get('binary')!r}, "
            f"not {binary!r}"
        )
    old_mapping = old_block.get("mapping")
    if not isinstance(old_mapping, Mapping):
        raise ProgressError(f"current physical block {old_block_id!r} lacks mapping metadata")
    old_guard_values = {
        "id": old_block_id,
        "start": normalize_address(old_block.get("start", "")),
        "end_exclusive": normalize_address(old_block.get("end_exclusive", "")),
        "source_path": old_block.get("source_path"),
        "agent_source_path": old_block.get("agent_source_path"),
        "original_source_path": old_block.get("original_source_path"),
        "provisional_original_path": old_block.get("provisional_original_path"),
        "mapping_state": old_mapping.get("state"),
        "mapping_status": old_mapping.get("status"),
        "contribution_ids": list(old_block.get("contribution_ids", [])),
        "semantic_span_ids": list(old_block.get("semantic_span_ids", [])),
        "source_shape_inputs": deepcopy(old_block.get("source_shape_inputs") or []),
        "candidate_header_contributors": deepcopy(
            old_block.get("candidate_header_contributors") or []
        ),
    }
    if old_guard_values != current:
        differences = [
            field
            for field in _CURRENT_BLOCK_GUARD_FIELDS
            if old_guard_values.get(field) != current.get(field)
        ]
        raise ProgressError(
            f"current physical block {old_block_id!r} is stale in fields: "
            + ", ".join(sorted(differences))
        )
    if old_guard_values["original_source_path"] is not None or old_guard_values["mapping_state"] != "unresolved":
        raise ProgressError(
            "physical-block replacement may not erase accepted original-source provenance; "
            "the current block must have original_source_path=null and mapping.state='unresolved'"
        )
    if old_block.get("first_external_callers") not in (None, []):
        raise ProgressError(
            f"current physical block {old_block_id!r} has first_external_callers that require "
            "a separate reviewed relationship reassignment"
        )
    for collection_name in _BLOCK_RELATIONSHIP_COLLECTIONS:
        collection = data.get(collection_name, {})
        if isinstance(collection, Mapping) and _references_exact_string(collection, old_block_id):
            raise ProgressError(
                f"current physical block {old_block_id!r} is referenced by {collection_name}; "
                "replacement cannot infer that relationship reassignment"
            )
    work_items = data.get("work_items", {})
    if isinstance(work_items, Mapping):
        for work_id, work in work_items.items():
            if (
                isinstance(work, Mapping)
                and work.get("state") in {"ready", "current", "active"}
                and _references_exact_string(work, old_block_id)
            ):
                raise ProgressError(
                    f"schedulable work item {work_id!r} still references {old_block_id!r}"
                )

    old_start, old_end = _entity_interval(old_block, label=f"physical block {old_block_id}")
    expected_old_id = _expected_block_id(binary, str(old_block["start"]))
    if old_block_id != expected_old_id:
        raise ProgressError(
            f"current physical block id must be start-derived {expected_old_id!r}, "
            f"found {old_block_id!r}"
        )
    block_intervals = _validate_contiguous_rows(
        replacement_blocks,
        start=old_start,
        end=old_end,
        label="replacement_blocks",
    )
    if replacement_blocks[0]["id"] != old_block_id:
        raise ProgressError(
            "the first replacement block must retain the old start-derived id for "
            "historical-reference continuity"
        )
    for index, (block, (start, _end)) in enumerate(zip(replacement_blocks, block_intervals)):
        expected_id = _expected_block_id(binary, normalize_address(start))
        if block["id"] != expected_id:
            raise ProgressError(
                f"replacement_blocks[{index}].id must be {expected_id!r}, found {block['id']!r}"
            )
        existing = blocks.get(block["id"])
        if existing is not None and block["id"] != old_block_id:
            raise ProgressError(f"replacement block id already exists: {block['id']!r}")
        for other_id, raw_other in blocks.items():
            if other_id == old_block_id or not isinstance(raw_other, Mapping):
                continue
            if raw_other.get("binary") != binary:
                continue
            other_start, other_end = _entity_interval(
                raw_other,
                label=f"physical block {other_id}",
            )
            if start < other_end and block_intervals[index][1] > other_start:
                raise ProgressError(
                    f"replacement block {block['id']!r} overlaps existing block {other_id!r}"
                )

    source_relationship_counts = _validate_block_source_relationship_reassignment(
        current,
        replacement_blocks,
        block_intervals,
    )

    actual_symbol_ids = sorted(
        (
            str(symbol_id)
            for symbol_id, symbol in symbols.items()
            if isinstance(symbol, Mapping) and symbol.get("physical_block_id") == old_block_id
        ),
        key=lambda symbol_id: address_value(symbols[symbol_id].get("address", "")),
    )
    if actual_symbol_ids != current["contribution_ids"]:
        raise ProgressError(
            "current block contribution_ids do not exactly match every symbol that references it"
        )
    symbol_intervals = {
        symbol_id: _entity_interval(symbols[symbol_id], label=f"symbol {symbol_id}")
        for symbol_id in actual_symbol_ids
    }
    derived_block_symbols: dict[str, list[str]] = {block["id"]: [] for block in replacement_blocks}
    symbol_to_block: dict[str, str] = {}
    for symbol_id in actual_symbol_ids:
        symbol_start, symbol_end = symbol_intervals[symbol_id]
        candidates = [
            block["id"]
            for block, (block_start, block_end) in zip(replacement_blocks, block_intervals)
            if block_start <= symbol_start and symbol_end <= block_end
        ]
        if len(candidates) != 1:
            raise ProgressError(
                f"symbol {symbol_id!r} is assigned to {len(candidates)} replacement block intervals"
            )
        block_id = candidates[0]
        symbol_to_block[symbol_id] = block_id
        derived_block_symbols[block_id].append(symbol_id)
    for index, block in enumerate(replacement_blocks):
        if block["contribution_ids"] != derived_block_symbols[block["id"]]:
            raise ProgressError(
                f"replacement_blocks[{index}].contribution_ids is not the complete exact "
                "address-ordered symbol assignment for its interval"
            )

    actual_span_ids = sorted(
        (
            str(span_id)
            for span_id, span in spans.items()
            if isinstance(span, Mapping) and span.get("physical_block_id") == old_block_id
        ),
        key=lambda span_id: address_value(spans[span_id].get("start", "")),
    )
    if actual_span_ids != current["semantic_span_ids"]:
        raise ProgressError(
            "current block semantic_span_ids do not exactly match every semantic span that references it"
        )
    current_span_rows = {
        span_id: spans[span_id]
        for span_id in actual_span_ids
    }
    current_span_intervals = _validate_contiguous_rows(
        [dict(current_span_rows[span_id]) for span_id in actual_span_ids],
        start=old_start,
        end=old_end,
        label="current semantic spans",
    )
    current_span_interval_by_id = dict(zip(actual_span_ids, current_span_intervals))
    current_symbol_to_span: dict[str, str] = {}
    for span_id, (span_start, span_end) in zip(
        actual_span_ids,
        current_span_intervals,
    ):
        current_span = current_span_rows[span_id]
        expected_span_id = _expected_semantic_span_id(
            binary,
            normalize_address(span_start),
            normalize_address(span_end),
        )
        if span_id != expected_span_id:
            raise ProgressError(
                f"current semantic span id must be extent-derived {expected_span_id!r}, "
                f"found {span_id!r}"
            )
        if (
            current_span.get("binary") != binary
            or current_span.get("physical_block_id") != old_block_id
        ):
            raise ProgressError(
                f"current semantic span {span_id!r} has a stale binary/block relationship"
            )
        expected_current_symbols = [
            symbol_id
            for symbol_id in actual_symbol_ids
            if span_start <= symbol_intervals[symbol_id][0]
            and symbol_intervals[symbol_id][1] <= span_end
        ]
        if current_span.get("symbol_ids") != expected_current_symbols:
            raise ProgressError(
                f"current semantic span {span_id!r} does not contain its complete exact "
                "address-ordered symbol population"
            )
        for symbol_id in expected_current_symbols:
            if symbols[symbol_id].get("semantic_span_ids") != [span_id]:
                raise ProgressError(
                    f"symbol {symbol_id!r} does not exactly reference current semantic "
                    f"span {span_id!r}"
                )
            if symbol_id in current_symbol_to_span:
                raise ProgressError(
                    f"symbol {symbol_id!r} appears in multiple current semantic spans"
                )
            current_symbol_to_span[symbol_id] = span_id
    missing_current_span_symbols = set(actual_symbol_ids) - set(current_symbol_to_span)
    if missing_current_span_symbols:
        raise ProgressError(
            "current semantic spans leave symbols unassigned: "
            + ", ".join(sorted(missing_current_span_symbols))
        )

    span_intervals = _validate_contiguous_rows(
        replacement_spans,
        start=old_start,
        end=old_end,
        label="replacement_semantic_spans",
    )
    replacement_span_ids = {span["id"] for span in replacement_spans}
    removed_span_ids = set(actual_span_ids) - replacement_span_ids
    for removed_span_id in removed_span_ids:
        for collection_name in (*_BLOCK_RELATIONSHIP_COLLECTIONS, "work_items", "binaries"):
            collection = data.get(collection_name, {})
            if isinstance(collection, Mapping) and _references_exact_string(
                collection,
                removed_span_id,
            ):
                raise ProgressError(
                    f"removed semantic span {removed_span_id!r} is still referenced by "
                    f"{collection_name}"
                )

    derived_block_spans: dict[str, list[str]] = {block["id"]: [] for block in replacement_blocks}
    derived_span_symbols: dict[str, list[str]] = {span["id"]: [] for span in replacement_spans}
    replacement_span_parent_ids: dict[str, str] = {}
    replacement_spans_by_current: dict[str, list[dict[str, Any]]] = {
        span_id: [] for span_id in actual_span_ids
    }
    replacement_block_seams = {
        block_start
        for block_start, _block_end in block_intervals[1:]
    }
    symbol_to_span: dict[str, str] = {}
    for index, (span, (span_start, span_end)) in enumerate(zip(replacement_spans, span_intervals)):
        expected_id = _expected_semantic_span_id(
            binary,
            normalize_address(span_start),
            normalize_address(span_end),
        )
        if span["id"] != expected_id:
            raise ProgressError(
                f"replacement_semantic_spans[{index}].id must be {expected_id!r}, "
                f"found {span['id']!r}"
            )
        existing = spans.get(span["id"])
        if existing is not None and span["id"] not in actual_span_ids:
            raise ProgressError(f"replacement semantic span id already exists: {span['id']!r}")
        block_candidates = [
            block["id"]
            for block, (block_start, block_end) in zip(replacement_blocks, block_intervals)
            if block_start <= span_start and span_end <= block_end
        ]
        if len(block_candidates) != 1 or span["physical_block_id"] != block_candidates[0]:
            raise ProgressError(
                f"replacement semantic span {span['id']!r} does not fit exactly one declared block"
            )
        current_span_candidates = [
            span_id
            for span_id, (current_start, current_end) in current_span_interval_by_id.items()
            if current_start <= span_start and span_end <= current_end
        ]
        if len(current_span_candidates) != 1:
            raise ProgressError(
                f"replacement semantic span {span['id']!r} merges or crosses current "
                "semantic span boundaries"
            )
        current_span_id = current_span_candidates[0]
        current_span = current_span_rows[current_span_id]
        for field in ("source_path", "status", "confidence"):
            if span[field] != current_span.get(field):
                raise ProgressError(
                    f"replacement semantic span {span['id']!r} must preserve current "
                    f"{field} from {current_span_id!r}"
                )
        replacement_span_parent_ids[span["id"]] = current_span_id
        replacement_spans_by_current[current_span_id].append(span)
        derived_block_spans[span["physical_block_id"]].append(span["id"])
        expected_symbols = [
            symbol_id
            for symbol_id in actual_symbol_ids
            if span_start <= symbol_intervals[symbol_id][0]
            and symbol_intervals[symbol_id][1] <= span_end
        ]
        if span["symbol_ids"] != expected_symbols:
            raise ProgressError(
                f"replacement semantic span {span['id']!r} lacks its complete exact symbol assignment"
            )
        if not expected_symbols:
            current_start, current_end = current_span_interval_by_id[current_span_id]
            preserves_exact_empty_padding = (
                current_span.get("status") == "padding"
                and isinstance(current_span.get("source_path"), str)
                and current_span["source_path"].startswith("padding:")
                and current_span.get("symbol_ids") == []
                and (span_start, span_end) == (current_start, current_end)
                and span["id"] == current_span_id
            )
            if not preserves_exact_empty_padding:
                raise ProgressError(
                    f"replacement semantic span {span['id']!r} may be empty only when "
                    "it exactly preserves a current reviewed zero-symbol padding span"
                )
        derived_span_symbols[span["id"]] = expected_symbols
        for symbol_id in expected_symbols:
            if symbol_id in symbol_to_span:
                raise ProgressError(f"symbol {symbol_id!r} appears in multiple semantic spans")
            symbol_to_span[symbol_id] = span["id"]
    missing_span_symbols = set(actual_symbol_ids) - set(symbol_to_span)
    if missing_span_symbols:
        raise ProgressError(
            "replacement semantic spans leave symbols unassigned: "
            + ", ".join(sorted(missing_span_symbols))
        )
    for current_span_id, child_spans in replacement_spans_by_current.items():
        current_start, current_end = current_span_interval_by_id[current_span_id]
        if not child_spans:
            raise ProgressError(
                f"replacement semantic spans drop current semantic span {current_span_id!r}"
            )
        _validate_contiguous_rows(
            child_spans,
            start=current_start,
            end=current_end,
            label=f"replacement partition for current semantic span {current_span_id}",
        )
        for left, right in zip(child_spans, child_spans[1:]):
            seam = address_value(left["end_exclusive"])
            if seam != address_value(right["start"]) or seam not in replacement_block_seams:
                raise ProgressError(
                    f"replacement semantic span split for {current_span_id!r} is not "
                    "an exact replacement-block seam"
                )
    for index, block in enumerate(replacement_blocks):
        if block["semantic_span_ids"] != derived_block_spans[block["id"]]:
            raise ProgressError(
                f"replacement_blocks[{index}].semantic_span_ids is not the complete exact "
                "address-ordered semantic assignment for its interval"
            )

    before_document = ProgressDocument(data)
    before_pipeline = before_document.pipeline(binary, resolve_order_target=False)
    call_contract_dependency_symbol_ids = (
        _physical_block_replace_call_contract_dependencies(
            before_document,
            before_pipeline,
            binary=binary,
            old_block_id=old_block_id,
        )
    )
    byte_preservation_symbol_ids = list(
        dict.fromkeys(
            [*actual_symbol_ids, *call_contract_dependency_symbol_ids]
        )
    )
    for symbol_id in byte_preservation_symbol_ids:
        if not isinstance(symbols[symbol_id].get("binary_state"), Mapping):
            raise ProgressError(
                f"physical-block replacement symbol {symbol_id!r} has invalid binary_state"
            )
    byte_facts_before = {
        symbol_id: {
            "binary_state": {
                dimension: deepcopy(state)
                for dimension, state in symbols[symbol_id]
                .get("binary_state", {})
                .items()
                if dimension != CALL_CONTRACT_DIMENSION
            },
            "accepted_byte_facts_present": "accepted_byte_facts" in symbols[symbol_id],
            "accepted_byte_facts": deepcopy(symbols[symbol_id].get("accepted_byte_facts")),
        }
        for symbol_id in byte_preservation_symbol_ids
    }
    old_order = deepcopy(old_block.get("order", {}))
    old_order_diagnostic = deepcopy(old_block.get("order_diagnostic", {}))
    old_order_diagnostics = deepcopy(old_block.get("order_diagnostics", {}))
    old_row_kind = old_block.get("row_kind", "physical-source-block")
    old_contribution_kind = old_block.get("contribution_kind", "authored")

    del blocks[old_block_id]
    for block in replacement_blocks:
        blocks[block["id"]] = {
            "accepted_order_facts": None,
            "agent_source_path": block["agent_source_path"],
            "binary": binary,
            "contribution_ids": list(block["contribution_ids"]),
            "contribution_kind": old_contribution_kind,
            "end_exclusive": block["end_exclusive"],
            "first_external_callers": [],
            "mapping": {
                "confidence": block["mapping_confidence"],
                "evidence_ids": [],
                "file_literal": None,
                "literal_xrefs": [],
                "state": block["mapping_state"],
                "status": block["mapping_status"],
            },
            "order": deepcopy(old_order),
            "order_diagnostic": deepcopy(old_order_diagnostic),
            "order_diagnostics": deepcopy(old_order_diagnostics),
            "order_targets": {"linked": "", "object": ""},
            "original_source_path": None,
            "provisional_original_path": None,
            "row_kind": old_row_kind,
            "semantic_span_ids": list(block["semantic_span_ids"]),
            "source_path": block["source_path"],
            "source_shape_inputs": deepcopy(block["source_shape_inputs"]),
            "candidate_header_contributors": deepcopy(
                block["candidate_header_contributors"]
            ),
            "start": block["start"],
        }

    for span_id in actual_span_ids:
        del spans[span_id]
    for span in replacement_spans:
        current_span_id = replacement_span_parent_ids[span["id"]]
        stored_span = deepcopy(current_span_rows[current_span_id])
        stored_span.update(
            {
                "binary": binary,
                "confidence": span["confidence"],
                "end_exclusive": span["end_exclusive"],
                "physical_block_id": span["physical_block_id"],
                "source_path": span["source_path"],
                "start": span["start"],
                "status": span["status"],
                "symbol_ids": list(span["symbol_ids"]),
            }
        )
        if "id" in stored_span:
            stored_span["id"] = span["id"]
        spans[span["id"]] = stored_span
    for symbol_id in actual_symbol_ids:
        symbol = symbols[symbol_id]
        symbol["physical_block_id"] = symbol_to_block[symbol_id]
        symbol["semantic_span_ids"] = [symbol_to_span[symbol_id]]

    invalidated = invalidate_order_dependencies(
        data,
        block_ids=[block["id"] for block in replacement_blocks],
    )
    invalidated["call_contract_symbol_ids"] = (
        _invalidate_physical_block_replace_call_contracts(
            data,
            symbol_ids=call_contract_dependency_symbol_ids,
        )
    )
    byte_facts_after = {
        symbol_id: {
            "binary_state": {
                dimension: deepcopy(state)
                for dimension, state in symbols[symbol_id]
                .get("binary_state", {})
                .items()
                if dimension != CALL_CONTRACT_DIMENSION
            },
            "accepted_byte_facts_present": "accepted_byte_facts" in symbols[symbol_id],
            "accepted_byte_facts": deepcopy(symbols[symbol_id].get("accepted_byte_facts")),
        }
        for symbol_id in byte_preservation_symbol_ids
    }
    if byte_facts_before != byte_facts_after:
        raise ProgressError(
            "physical-block replacement unexpectedly changed independent symbol byte facts"
        )
    _validate_global_block_nonoverlap(blocks, binary=binary)

    for block in replacement_blocks:
        stored_block = blocks.get(block["id"])
        if not isinstance(stored_block, Mapping):
            raise ProgressError(f"replacement block disappeared after mutation: {block['id']!r}")
        for symbol_id in stored_block["contribution_ids"]:
            if symbols[symbol_id].get("physical_block_id") != block["id"]:
                raise ProgressError(
                    f"symbol {symbol_id!r} has a dangling physical block relationship"
                )
        for span_id in stored_block["semantic_span_ids"]:
            if spans[span_id].get("physical_block_id") != block["id"]:
                raise ProgressError(
                    f"semantic span {span_id!r} has a dangling physical block relationship"
                )
    for span in replacement_spans:
        for symbol_id in span["symbol_ids"]:
            if symbols[symbol_id].get("semantic_span_ids") != [span["id"]]:
                raise ProgressError(
                    f"symbol {symbol_id!r} has a dangling semantic span relationship"
                )

    proposed_document = ProgressDocument(data)
    errors = [finding for finding in proposed_document.audit() if finding.severity == "error"]
    if errors:
        raise ProgressError(
            "physical-block replacement failed proposed tracker audit: "
            + "; ".join(finding.message for finding in errors[:8])
        )
    after_pipeline = proposed_document.pipeline(binary, resolve_order_target=False)
    replacement_block_ids = [block["id"] for block in replacement_blocks]
    phase_transition_contract = _validate_physical_block_replace_scheduler_transition(
        before_pipeline,
        after_pipeline,
        old_block_id=old_block_id,
        old_start=current["start"],
        replacement_block_ids=replacement_block_ids,
        invalidated=invalidated,
        call_contract_dependency_symbol_ids=call_contract_dependency_symbol_ids,
    )
    byte_frontier_fields = (
        "authored_byte_cursor",
        "authored_byte_match_frontier",
        "linked_byte_match_prefix_end",
    )
    if any(
        before_pipeline.get(field) != after_pipeline.get(field)
        for field in byte_frontier_fields
    ):
        raise ProgressError(
            "physical-block replacement unexpectedly changed an independent byte frontier"
        )
    after_cursor = str(after_pipeline.get("cursor", ""))
    if after_cursor:
        cursor_value = address_value(after_cursor)
        containing_blocks = [
            block_id
            for block_id, block in blocks.items()
            if isinstance(block, Mapping)
            and block.get("binary") == binary
            and _entity_interval(block, label=f"physical block {block_id}")[0]
            <= cursor_value
            < _entity_interval(block, label=f"physical block {block_id}")[1]
        ]
        if len(containing_blocks) != 1 or after_pipeline.get("physical_block_id") != containing_blocks[0]:
            raise ProgressError("physical-block replacement left the scheduler cursor relationship invalid")

    return {
        "kind": "physical-block-replace",
        "schema": _PHYSICAL_BLOCK_REPLACE_SCHEMA,
        "reviewed": True,
        "parent_reviewed": True,
        "reason": str(payload["reason"]),
        "binary": binary,
        "old_block": {
            "id": old_block_id,
            "start": current["start"],
            "end_exclusive": current["end_exclusive"],
        },
        "replacement_block_ids": replacement_block_ids,
        "replacement_block_intervals": [
            {"start": block["start"], "end_exclusive": block["end_exclusive"]}
            for block in replacement_blocks
        ],
        "removed_semantic_span_ids": sorted(removed_span_ids),
        "replacement_semantic_span_ids": [span["id"] for span in replacement_spans],
        "reassigned_symbol_ids": actual_symbol_ids,
        "reassigned_source_shape_input_count": source_relationship_counts[
            "source_shape_inputs"
        ],
        "reassigned_candidate_header_contributor_count": source_relationship_counts[
            "candidate_header_contributors"
        ],
        "invalidated": invalidated,
        "phase_transition_contract": phase_transition_contract,
        "independent_byte_state": {
            "symbol_facts_preserved": True,
            "authored_byte_cursor_preserved": True,
            "authored_byte_match_frontier_preserved": True,
            "linked_byte_match_prefix_end_preserved": True,
        },
        "relationship_checks": {
            "complete_block_coverage": True,
            "complete_symbol_assignment": True,
            "complete_semantic_span_coverage": True,
            "complete_semantic_symbol_assignment": True,
            "semantic_observations_preserved": True,
            "zero_symbol_spans_limited_to_exact_padding": True,
            "no_physical_block_overlaps": True,
            "no_dangling_current_references": True,
            "unresolved_original_source_provenance_preserved": True,
            "complete_source_shape_input_reassignment": True,
            "complete_candidate_header_contributor_reassignment": True,
        },
        "scheduler_before": {
            "phase": before_pipeline.get("phase"),
            "cursor": before_pipeline.get("cursor"),
            "physical_block_id": before_pipeline.get("physical_block_id"),
        },
        "scheduler_after": {
            "phase": after_pipeline.get("phase"),
            "cursor": after_pipeline.get("cursor"),
            "physical_block_id": after_pipeline.get("physical_block_id"),
        },
    }


def _contains_current_accepted_state(value: Any) -> bool:
    if isinstance(value, Mapping):
        if is_current_accepted_state(value):
            return True
        return any(_contains_current_accepted_state(item) for item in value.values())
    if isinstance(value, list):
        return any(_contains_current_accepted_state(item) for item in value)
    return False


def _function_padding_frontiers(data: Mapping[str, Any], binary: str) -> dict[str, Any]:
    pipeline = ProgressDocument(data).pipeline(binary, resolve_order_target=False)
    fields = (
        "phase",
        "cursor",
        "physical_block_id",
        "authored_order_prefix_end",
        "full_order_prefix_end",
        "authored_byte_cursor",
        "authored_byte_match_frontier",
        "linked_byte_match_prefix_end",
    )
    return {field: deepcopy(pipeline.get(field)) for field in fields}


def _replace_function_with_padding(
    data: dict[str, Any],
    payload: Mapping[str, Any],
) -> dict[str, Any]:
    binary = str(payload["binary"])
    current_function = payload["current_function"]
    current_block = payload["current_block"]
    current_span = payload["current_semantic_span"]
    replacement = payload["replacement_padding"]
    function_id = str(current_function["id"])
    block_id = str(current_block["id"])
    span_id = str(current_span["id"])

    if replacement["remove_function_id"] != function_id:
        raise ProgressError(
            "replacement_padding.remove_function_id must equal current_function.id"
        )
    if replacement["keep_physical_block_id"] != block_id:
        raise ProgressError(
            "replacement_padding.keep_physical_block_id must equal current_block.id"
        )
    if replacement["keep_semantic_span_id"] != span_id:
        raise ProgressError(
            "replacement_padding.keep_semantic_span_id must equal current_semantic_span.id"
        )
    if current_block["expected_contains_function_id"] != function_id:
        raise ProgressError(
            "current_block.expected_contains_function_id must equal current_function.id"
        )
    if current_block["expected_contains_semantic_span_id"] != span_id:
        raise ProgressError(
            "current_block.expected_contains_semantic_span_id must equal "
            "current_semantic_span.id"
        )

    symbols = data.get("symbols")
    blocks = data.get("physical_blocks")
    spans = data.get("semantic_spans")
    if not isinstance(symbols, dict) or not isinstance(blocks, dict) or not isinstance(spans, dict):
        raise ProgressError(
            "function-padding correction requires symbols, physical_blocks, and "
            "semantic_spans collections"
        )
    function = symbols.get(function_id)
    block = blocks.get(block_id)
    span = spans.get(span_id)
    if not isinstance(function, dict):
        raise ProgressError(f"unknown current function {function_id!r}")
    if not isinstance(block, dict):
        raise ProgressError(f"unknown current physical block {block_id!r}")
    if not isinstance(span, dict):
        raise ProgressError(f"unknown current semantic span {span_id!r}")
    if function != current_function["record"]:
        raise ProgressError(f"current function {function_id!r} exact row guard is stale")
    if span != current_span["record"]:
        raise ProgressError(f"current semantic span {span_id!r} exact row guard is stale")

    mapping = block.get("mapping")
    if not isinstance(mapping, Mapping):
        raise ProgressError(f"current physical block {block_id!r} has no mapping object")
    contribution_ids = block.get("contribution_ids")
    semantic_span_ids = block.get("semantic_span_ids")
    source_shape_inputs = block.get("source_shape_inputs") or []
    header_contributors = block.get("candidate_header_contributors") or []
    if not all(
        isinstance(value, list)
        for value in (
            contribution_ids,
            semantic_span_ids,
            source_shape_inputs,
            header_contributors,
        )
    ):
        raise ProgressError(
            f"current physical block {block_id!r} has malformed relationship inventories"
        )
    actual_block_guard = {
        "id": block_id,
        "binary": block.get("binary"),
        "start": block.get("start"),
        "end_exclusive": block.get("end_exclusive"),
        "row_kind": block.get("row_kind"),
        "contribution_kind": block.get("contribution_kind"),
        "source_path": block.get("source_path"),
        "agent_source_path": block.get("agent_source_path"),
        "original_source_path": block.get("original_source_path"),
        "provisional_original_path": block.get("provisional_original_path"),
        "mapping_state": mapping.get("state"),
        "mapping_status": mapping.get("status"),
        "accepted_order_facts": block.get("accepted_order_facts"),
        "expected_contribution_count": len(contribution_ids),
        "expected_contains_function_id": function_id,
        "expected_function_membership_count": contribution_ids.count(function_id),
        "expected_semantic_span_count": len(semantic_span_ids),
        "expected_contains_semantic_span_id": span_id,
        "expected_semantic_span_membership_count": semantic_span_ids.count(span_id),
        "source_shape_input_count": len(source_shape_inputs),
        "candidate_header_contributor_count": len(header_contributors),
    }
    if actual_block_guard != current_block:
        stale_fields = sorted(
            field
            for field in _FUNCTION_PADDING_BLOCK_GUARD_FIELDS
            if actual_block_guard.get(field) != current_block.get(field)
        )
        raise ProgressError(
            f"current physical block {block_id!r} guard is stale in fields: "
            + ", ".join(stale_fields)
        )
    if contribution_ids.count(function_id) != 1:
        raise ProgressError(
            f"current physical block {block_id!r} must contain {function_id!r} exactly once"
        )
    if semantic_span_ids.count(span_id) != 1:
        raise ProgressError(
            f"current physical block {block_id!r} must retain semantic span {span_id!r} "
            "exactly once"
        )

    try:
        function_start, function_end = _entity_interval(
            function,
            label=f"function {function_id}",
        )
        span_start, span_end = _entity_interval(span, label=f"semantic span {span_id}")
        block_start, block_end = _entity_interval(block, label=f"physical block {block_id}")
    except ProgressError:
        raise
    replacement_start = address_value(replacement["start"])
    replacement_end = address_value(replacement["end_exclusive"])
    if (
        (function_start, function_end)
        != (span_start, span_end)
        or (function_start, function_end)
        != (replacement_start, replacement_end)
    ):
        raise ProgressError(
            "function, semantic span, and replacement_padding must have exactly the "
            "same closed-open extent"
        )
    if not (block_start <= function_start < function_end <= block_end):
        raise ProgressError("padding extent must remain wholly inside the retained physical block")
    if function.get("binary") != binary or block.get("binary") != binary or span.get("binary") != binary:
        raise ProgressError("function, physical block, and semantic span binary must match payload")
    if (
        function.get("kind") != "function"
        or function.get("extent_state") != "known"
        or function.get("physical_block_id") != block_id
        or function.get("semantic_span_ids") != [span_id]
        or function.get("pipeline_class") != "unresolved"
        or function.get("ownership_state") != "unresolved"
        or function.get("disposition") != "unresolved"
        or "authored_order_role" in function
    ):
        raise ProgressError(
            "current function must be one exact unresolved known-extent false function "
            "with only the retained block and padding span relationships"
        )
    if function.get("size") != function_end - function_start:
        raise ProgressError("current function size does not match its guarded extent")
    if function.get("accepted_order_facts") is not None:
        raise ProgressError("function-padding correction refuses accepted-order facts")
    if function.get("accepted_byte_facts") is not None:
        raise ProgressError("function-padding correction refuses accepted-byte facts")
    if _contains_current_accepted_state(function.get("binary_state", {})):
        raise ProgressError("function-padding correction refuses accepted binary-state facts")
    if _contains_current_accepted_state(block.get("order", {})):
        raise ProgressError(
            "function-padding correction refuses a physical block with accepted order state"
        )
    if function.get("storage_contribution_ids") not in (None, []):
        raise ProgressError("function-padding correction refuses storage relationships")
    if function.get("verification_target_ids") not in (None, []):
        raise ProgressError("function-padding correction refuses verification-target relationships")
    if (
        span.get("physical_block_id") != block_id
        or span.get("status") != "padding"
        or span.get("symbol_ids") != [function_id]
    ):
        raise ProgressError(
            "current semantic span must be the exact retained padding span with one "
            "false-function membership"
        )

    forbidden_relationships = (
        ("owners", "owner"),
        ("verification_targets", "verification-target"),
        ("storage_contributions", "storage"),
        ("work_items", "work-item"),
    )
    for collection_name, label in forbidden_relationships:
        collection = data.get(collection_name, {})
        if isinstance(collection, Mapping) and _references_exact_string(
            collection,
            function_id,
        ):
            raise ProgressError(
                f"function-padding correction refuses {label} relationships"
            )

    for other_id, other in symbols.items():
        if other_id != function_id and _references_exact_string(other, function_id):
            raise ProgressError(
                f"function-padding correction found unsafe symbol relationship in {other_id!r}"
            )
    for other_id, other in blocks.items():
        if other_id != block_id and _references_exact_string(other, function_id):
            raise ProgressError(
                f"function-padding correction found unsafe physical-block relationship "
                f"in {other_id!r}"
            )
    block_without_membership = deepcopy(block)
    block_without_membership["contribution_ids"] = [
        item for item in contribution_ids if item != function_id
    ]
    if _references_exact_string(block_without_membership, function_id):
        raise ProgressError(
            "retained physical block has an unsafe false-function relationship outside "
            "contribution_ids"
        )
    for other_id, other in spans.items():
        if other_id != span_id and _references_exact_string(other, function_id):
            raise ProgressError(
                f"function-padding correction found unsafe semantic-span relationship "
                f"in {other_id!r}"
            )
    span_without_membership = deepcopy(span)
    span_without_membership["symbol_ids"] = []
    if _references_exact_string(span_without_membership, function_id):
        raise ProgressError(
            "retained semantic span has an unsafe false-function relationship outside symbol_ids"
        )

    migration = data.get("migration")
    if not isinstance(migration, dict):
        raise ProgressError("function-padding correction requires migration to be an object")
    schema_v4 = migration.get("schema_v4")
    if schema_v4 is not None and not isinstance(schema_v4, dict):
        raise ProgressError("optional schema-v4 migration metadata must be an object")
    expected_classification = {
        "authored_body_seeded": False,
        "authored_presence_seeded": False,
        "authored_target_seeded": False,
        "classification_reason": "no unique non-provider primary owner",
        "pipeline_class": "unresolved",
        "symbol_id": function_id,
    }
    classifications = (
        schema_v4.get("symbol_classifications", [])
        if isinstance(schema_v4, Mapping)
        else []
    )
    unclassified = (
        schema_v4.get("unclassified_symbols", [])
        if isinstance(schema_v4, Mapping)
        else []
    )
    if not isinstance(classifications, list) or not isinstance(unclassified, list):
        raise ProgressError(
            "optional schema-v4 symbol classifications and unclassified symbols "
            "must be arrays when present"
        )
    matching_classifications = [
        item
        for item in classifications
        if isinstance(item, Mapping) and item.get("symbol_id") == function_id
    ]
    if matching_classifications not in ([], [expected_classification]):
        raise ProgressError(
            "function-padding correction found ambiguous or drifted related schema-v4 "
            "migration classification"
        )
    unclassified_count = unclassified.count(function_id)
    if unclassified_count not in {0, 1}:
        raise ProgressError(
            "function-padding correction found ambiguous schema-v4 unclassified "
            "symbol memberships"
        )
    removed_schema_v4_classification_count = len(matching_classifications)
    removed_schema_v4_unclassified_count = unclassified_count
    migration_without_allowed = deepcopy(dict(migration))
    migration_v4_without_allowed = migration_without_allowed.get("schema_v4")
    if isinstance(migration_v4_without_allowed, dict):
        if "symbol_classifications" in migration_v4_without_allowed:
            migration_v4_without_allowed["symbol_classifications"] = [
                item
                for item in classifications
                if not (isinstance(item, Mapping) and item.get("symbol_id") == function_id)
            ]
        if "unclassified_symbols" in migration_v4_without_allowed:
            migration_v4_without_allowed["unclassified_symbols"] = [
                item for item in unclassified if item != function_id
            ]
    if _references_exact_string(migration_without_allowed, function_id):
        raise ProgressError(
            "function-padding correction found an unsafe additional migration relationship"
        )
    for collection_name, collection in data.items():
        if collection_name in {
            "symbols",
            "physical_blocks",
            "semantic_spans",
            "migration",
        }:
            continue
        if _references_exact_string(collection, function_id):
            raise ProgressError(
                f"function-padding correction found unsafe relationship in {collection_name}"
            )

    binaries = data.get("binaries")
    binary_row = binaries.get(binary) if isinstance(binaries, Mapping) else None
    reference = binary_row.get("reference") if isinstance(binary_row, Mapping) else None
    inventory = (
        binary_row.get("inventory_snapshot") if isinstance(binary_row, Mapping) else None
    )
    if not isinstance(reference, Mapping) or reference.get("path") != "support/Recoil.exe":
        raise ProgressError(
            "function-padding correction requires the registered immutable "
            "support/Recoil.exe reference"
        )
    if not isinstance(inventory, Mapping):
        raise ProgressError("function-padding correction requires a Recoil inventory snapshot")
    inventory_count = inventory.get("function_count")
    if not isinstance(inventory_count, int) or isinstance(inventory_count, bool):
        raise ProgressError("inventory snapshot function_count must be an integer")
    tracker_function_count_before = sum(
        isinstance(row, Mapping)
        and row.get("binary") == binary
        and row.get("kind") == "function"
        for row in symbols.values()
    )
    if tracker_function_count_before != inventory_count + 1:
        raise ProgressError(
            "function-padding correction requires exactly one excess Recoil function "
            "relative to the unchanged inventory snapshot"
        )

    reference_path = MACHINE_RETAIL_REFERENCE.resolve()
    expected_reference_path = MACHINE_RETAIL_REFERENCE.resolve()
    if reference_path != expected_reference_path:
        raise ProgressError(
            "registered retail reference does not resolve to support/Recoil.exe"
        )
    image = reference_path.read_bytes()
    headers = parse_pe_headers(image, source=str(reference_path))
    extent_size = function_end - function_start
    start_rva = function_start - headers.image_base
    end_rva = function_end - headers.image_base
    file_backing_sections = [
        section
        for section in headers.sections
        if section.virtual_address <= start_rva
        and end_rva <= section.virtual_address + section.raw_size
    ]
    offset = rva_to_offset(start_rva, headers.sections)
    last_offset = rva_to_offset(end_rva - 1, headers.sections)
    if (
        len(file_backing_sections) != 1
        or offset is None
        or last_offset is None
        or offset < 0
        or last_offset != offset + extent_size - 1
        or offset + extent_size > len(image)
    ):
        raise ProgressError("padding extent is not file-backed by the registered retail image")
    retail_bytes = image[offset : offset + extent_size]
    payload_bytes = bytes.fromhex(str(replacement["retail_bytes_hex"]))
    if len(payload_bytes) != extent_size:
        raise ProgressError(
            "replacement_padding.retail_bytes_hex length does not match the exact extent"
        )
    if retail_bytes != payload_bytes:
        raise ProgressError(
            "immutable retail bytes do not exactly match replacement_padding.retail_bytes_hex"
        )
    if not retail_bytes or any(value != 0x90 for value in retail_bytes):
        raise ProgressError(
            "immutable retail extent is not exact 0x90 NOP padding"
        )

    frontiers_before = _function_padding_frontiers(data, binary)
    inventory_before = deepcopy(inventory)
    block_before = deepcopy(block)
    span_before = deepcopy(span)

    del symbols[function_id]
    block["contribution_ids"].remove(function_id)
    span["symbol_ids"] = []
    if isinstance(schema_v4, dict):
        if "symbol_classifications" in schema_v4:
            schema_v4["symbol_classifications"] = [
                item
                for item in classifications
                if not (isinstance(item, Mapping) and item.get("symbol_id") == function_id)
            ]
        if "unclassified_symbols" in schema_v4:
            schema_v4["unclassified_symbols"] = [
                item for item in unclassified if item != function_id
            ]

    expected_block = deepcopy(block_before)
    expected_block["contribution_ids"].remove(function_id)
    if block != expected_block:
        raise ProgressError("function-padding correction changed unrelated physical-block state")
    expected_span = deepcopy(span_before)
    expected_span["symbol_ids"] = []
    if span != expected_span:
        raise ProgressError("function-padding correction changed unrelated semantic-span state")
    if inventory != inventory_before:
        raise ProgressError("function-padding correction changed the inventory snapshot")
    tracker_function_count_after = sum(
        isinstance(row, Mapping)
        and row.get("binary") == binary
        and row.get("kind") == "function"
        for row in symbols.values()
    )
    if tracker_function_count_after != inventory_count:
        raise ProgressError(
            "function-padding correction did not reconcile tracker functions to the "
            "unchanged inventory snapshot"
        )
    frontiers_after = _function_padding_frontiers(data, binary)
    if frontiers_after != frontiers_before:
        raise ProgressError(
            "function-padding correction unexpectedly changed a scheduler/order/byte frontier"
        )
    proposed = ProgressDocument(data)
    errors = [finding for finding in proposed.audit() if finding.severity == "error"]
    if errors:
        raise ProgressError(
            "function-padding correction failed proposed tracker audit: "
            + "; ".join(finding.message for finding in errors[:8])
        )
    return {
        "kind": "function-padding-correction",
        "schema": _FUNCTION_PADDING_CORRECTION_SCHEMA,
        "reviewed": True,
        "parent_reviewed": True,
        "reason": str(payload["reason"]),
        "binary": binary,
        "removed_function_id": function_id,
        "retained_physical_block_id": block_id,
        "retained_semantic_span_id": span_id,
        "padding": {
            "start": replacement["start"],
            "end_exclusive": replacement["end_exclusive"],
            "retail_bytes_hex": retail_bytes.hex(),
            "byte_count": len(retail_bytes),
            "verified_all_nop_0x90": True,
        },
        "removed_relationships": {
            "physical_block_contribution_memberships": 1,
            "semantic_span_symbol_memberships": 1,
            "schema_v4_symbol_classifications": removed_schema_v4_classification_count,
            "schema_v4_unclassified_symbol_memberships": removed_schema_v4_unclassified_count,
        },
        "preserved": {
            "physical_block_except_contribution_membership": True,
            "semantic_span_except_symbol_membership": True,
            "inventory_snapshot": True,
            "scheduler_order_byte_frontiers": True,
            "owners_targets_storage_output_work": True,
        },
        "tracker_function_count_before": tracker_function_count_before,
        "tracker_function_count_after": tracker_function_count_after,
        "inventory_snapshot_function_count": inventory_count,
        "frontiers_before": frontiers_before,
        "frontiers_after": frontiers_after,
    }


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = _parser().parse_args(argv)
    try:
        if args.command == "advance-live-order":
            code, payload = advance_live_order(args)
            _print_json(payload)
            return code
        if args.command == "advance-live-byte":
            code, payload = advance_live_byte(args)
            _print_json(payload)
            return code
        if args.command == "advance-live-call-contract":
            code, payload = advance_live_call_contract(args)
            _print_json(payload)
            return code
        if args.command == "call-contract" and args.call_contract_command == "initialize":
            _print_json(initialize_authored_call_contract(args))
            return 0
        if (
            args.command == "call-contract"
            and args.call_contract_command == "prepare-live-convergence"
        ):
            _print_json(prepare_live_call_contract_convergence(args))
            return 0
        if (
            args.command == "call-contract"
            and args.call_contract_command == "prepare-repair-continuation"
        ):
            _print_json(prepare_call_contract_repair_continuation(args))
            return 0
        document = _load(args.progress)
        if args.command == "compact":
            from _recoil.commands.ledger_compact import (
                prepare_progress_compaction,
                require_compaction_apply_allowed,
            )

            candidate, details = prepare_progress_compaction(document.data)
            if args.apply:
                require_compaction_apply_allowed(details)
            commit = ProgressStore(args.progress).commit(
                candidate,
                expected_revision=args.expected_revision,
                apply=args.apply,
            )
            _print_json(_commit_payload(commit, details))
            return 0
        if args.command == "next":
            payload = _next_work_with_issue_ledger(
                document,
                args.binary,
                issue_ledger=args.issue_ledger,
            )
            payload = _scheduler_domain_guarded_call_contract_commands(
                document, payload
            )
            _print_json(payload) if args.json else _print_pipeline(payload)
            return 0
        if args.command == "status":
            payload = document.show(args.selector) if args.selector else document.pipeline(args.binary)
            if args.selector is None:
                payload = _scheduler_domain_guarded_call_contract_commands(
                    document, payload
                )
            _print_json(payload) if args.json else _print_pipeline(payload)
            return 0
        if args.command == "show":
            payload = document.show(args.selector)
            _print_json(payload) if args.json else _print_json(payload)
            return 0
        if args.command == "find":
            payload = document.find(args.query, args.limit)
            _print_json(payload)
            return 0
        if args.command == "report":
            payload = document.summary()
            _print_json(payload) if args.json else _print_pipeline(payload.get("pipeline", {}))
            return 0
        if args.command == "audit":
            findings = document.audit(args.scope)
            payload = {
                "scope": args.scope,
                "passed": not any(item.severity == "error" for item in findings),
                "findings": [item.to_dict() for item in findings],
                **document.scheduler_identity(),
            }
            _print_json(payload) if args.json else print(
                "progress audit OK" if payload["passed"] else f"progress audit: {len(findings)} finding(s)"
            )
            return 0 if payload["passed"] else 1
        if args.command == "handoff":
            payload = _handoff(document, args)
            _print_json(payload) if args.json else _print_pipeline(payload)
            return 0
        if args.command == "block" and args.block_command == "reclassify-provider":
            payload = _parse_provider_block_reclassify_payload(args.payload_json)
            details: dict[str, Any] = {}

            def transform(data: dict[str, Any]) -> None:
                details.update(_reclassify_provider_block(data, payload))

            commit = ProgressStore(args.progress).mutate(
                transform,
                expected_revision=args.expected_revision,
                apply=args.apply,
            )
            details["mutation_planned"] = True
            _print_json(_commit_payload(commit, details))
            return 0
        if args.command == "block" and args.block_command == "accept-authored-non-gating":
            payload = _load_authored_non_gating_block_accept_payload(args)
            details: dict[str, Any] = {}

            def transform(data: dict[str, Any]) -> None:
                details.update(_accept_authored_non_gating_blocks(data, payload))

            commit = ProgressStore(args.progress).mutate(
                transform,
                expected_revision=args.expected_revision,
                apply=args.apply,
            )
            details["mutation_planned"] = True
            _print_json(_commit_payload(commit, details))
            return 0
        if args.command == "block" and args.block_command == "replace":
            replacement = _load_physical_block_replace_payload(args)
            details: dict[str, Any] = {}

            def transform(data: dict[str, Any]) -> None:
                details.update(_replace_physical_block(data, replacement))

            commit = ProgressStore(args.progress).mutate(
                transform,
                expected_revision=args.expected_revision,
                apply=args.apply,
            )
            details["mutation_planned"] = True
            _print_json(_commit_payload(commit, details))
            return 0
        if (
            args.command == "storage"
            and args.storage_command == "register-authored-data"
        ):
            payload = load_authored_storage_payload(
                payload_json=args.payload_json,
                payload_file=args.payload_file,
            )
            _print_json(
                _mutate_authored_storage_no_new_debt(
                    args.progress,
                    payload,
                    expected_revision=args.expected_revision,
                    apply=bool(args.apply),
                )
            )
            return 0
        if args.command in {"output-section", "storage", "block", "semantic"}:
            collection = {
                "output-section": "output_sections",
                "storage": "storage_contributions",
                "block": "physical_blocks",
                "semantic": "semantic_spans",
            }[args.command]
            _print_json(document.scheduler_output(_resolve_collection_row(document, collection, args.selector)))
            return 0
        if args.command == "owner":
            if args.owner_command == "repair-primary-data-tier-x":
                payload = _parse_owner_primary_data_tier_x_repair_payload(
                    args.payload_json
                )
                details: dict[str, Any] = {}

                def transform(data: dict[str, Any]) -> None:
                    details.update(_repair_owner_primary_data_tier_x(data, payload))

                commit = ProgressStore(args.progress).mutate(
                    transform,
                    expected_revision=args.expected_revision,
                    apply=args.apply,
                )
                details["mutation_planned"] = True
                _print_json(_commit_payload(commit, details))
                return 0
            if args.owner_command == "downgrade":
                payload = _parse_owner_downgrade_payload(args.payload_json)
                details: dict[str, Any] = {}

                def transform(data: dict[str, Any]) -> None:
                    details.update(_downgrade_owner(data, payload))

                commit = ProgressStore(args.progress).mutate(
                    transform,
                    expected_revision=args.expected_revision,
                    apply=args.apply,
                )
                details["mutation_planned"] = True
                _print_json(_commit_payload(commit, details))
                return 0
            if args.owner_command == "replace-batch":
                payload = _load_owner_replace_batch_payload(args)
                details: dict[str, Any] = {}

                def transform(data: dict[str, Any]) -> None:
                    details.update(_replace_owner_batch(data, payload))

                commit = ProgressStore(args.progress).mutate(
                    transform,
                    expected_revision=args.expected_revision,
                    apply=args.apply,
                )
                details["mutation_planned"] = True
                _print_json(_commit_payload(commit, details))
                return 0
            if args.owner_command in {"show", "relationships"}:
                _print_json(_owner_view(document, args.selector))
                return 0
            if args.owner_command == "find":
                rows = document.find(args.query, args.limit)
                rows["matches"] = [item for item in rows["matches"] if item["collection"] == "owners"]
                _print_json(rows)
                return 0
            try:
                validate_owner_invariants(document.data)
            except ProgressError as exc:
                findings = [
                    {
                        "severity": "error",
                        "code": "owner.invariant",
                        "message": str(exc),
                        "entity_id": "owners",
                    }
                ]
            else:
                findings = [
                    {
                        "severity": "error",
                        "code": code,
                        "message": message,
                        "entity_id": "owners",
                    }
                    for code, message in audit_authored_icf_groups(document.data)
                ]
            payload = {
                "passed": not findings,
                "findings": findings,
                **document.scheduler_identity(),
            }
            _print_json(payload)
            return 1 if args.strict and findings else 0
        if args.command == "symbol" and args.symbol_command == "set-pipeline-class-batch":
            batch = _parse_symbol_classification_batch(args.payload_json)
            details: dict[str, Any] = {}

            def transform(data: dict[str, Any]) -> None:
                details.update(_set_symbol_pipeline_class_batch(data, batch))

            commit = ProgressStore(args.progress).mutate(
                transform,
                expected_revision=args.expected_revision,
                apply=args.apply,
            )
            _print_json(_commit_payload(commit, details))
            return 0
        if args.command == "symbol" and args.symbol_command == "set-logical-alias-group":
            payload = _load_logical_alias_group_payload(args)
            details: dict[str, Any] = {}

            def transform(data: dict[str, Any]) -> None:
                details.update(_set_symbol_logical_alias_group(data, payload))

            commit = ProgressStore(args.progress).mutate(
                transform,
                expected_revision=args.expected_revision,
                apply=args.apply,
            )
            _print_json(_commit_payload(commit, details))
            return 0
        if args.command == "symbol" and args.symbol_command == "replace-padding":
            payload = _parse_function_padding_correction_payload(args.payload_json)
            details: dict[str, Any] = {}

            def transform(data: dict[str, Any]) -> None:
                details.update(_replace_function_with_padding(data, payload))

            commit = ProgressStore(args.progress).mutate(
                transform,
                expected_revision=args.expected_revision,
                apply=args.apply,
            )
            details["mutation_planned"] = True
            _print_json(_commit_payload(commit, details))
            return 0
        if args.command == "work" and args.work_command == "show":
            rows = document.collection("work_items")
            row = rows.get(args.work_item_id)
            if not isinstance(row, Mapping):
                raise ProgressError(f"unknown work item: {args.work_item_id}")
            _print_json(
                document.scheduler_output(
                    {"id": args.work_item_id, "record": row}
                )
            )
            return 0
        if args.command == "work" and args.work_command == "leases":
            from _recoil.commands.workspace_issues import combined_lease_view

            _print_json(
                combined_lease_view(
                    args.progress,
                    args.issue_ledger,
                    selector=args.id,
                    document=document,
                )
            )
            return 0
        if args.command == "work" and args.work_command == "claim-current":
            from _recoil.commands.workspace_issues import (
                cross_ledger_reservation_critical_section,
            )

            lock = (
                cross_ledger_reservation_critical_section(
                    Path(args.progress), Path(args.issue_ledger)
                )
                if args.apply
                else nullcontext()
            )
            with lock:
                _print_json(claim_current_work(args))
            return 0
        if args.command == "work" and args.work_command == "create-explicit":
            _print_json(create_explicit_maintenance_work(args))
            return 0
        if args.command == "work" and args.work_command == "reserve":
            from _recoil.commands.workspace_issues import (
                cross_ledger_reservation_critical_section,
                workspace_issue_reservation_conflicts,
            )

            lock = (
                cross_ledger_reservation_critical_section(
                    Path(args.progress), Path(args.issue_ledger)
                )
                if args.apply
                else nullcontext()
            )
            with lock:
                scheduler_document, scheduler_domains = _precheck_scheduler_revision(args)
                details: dict[str, Any] = {}

                def transform(data: dict[str, Any]) -> None:
                    work = ProgressDocument(data).collection("work_items").get(args.id)
                    if not isinstance(work, Mapping):
                        raise ProgressError(f"unknown work item {args.id}")
                    claims, complete, _source = work_resource_claims(work)
                    if not complete:
                        raise ProgressError("work item resource claims are incomplete")
                    conflicts = workspace_issue_reservation_conflicts(
                        Path(args.issue_ledger), args.id, claims
                    )
                    if conflicts:
                        raise ProgressError(
                            "work item conflicts with active workspace issue reservation"
                        )
                    details.update(reserve_work_item(data, args.id))

                commit = (
                    _call_contract_scoped_patch_commit(
                        args=args,
                        document=scheduler_document,
                        transform=transform,
                        expected_domains=scheduler_domains,
                        increment_domains={"scheduler"},
                    )
                    if scheduler_domains is not None
                    else ProgressStore(args.progress).mutate(
                        transform,
                        expected_revision=args.expected_revision,
                        apply=args.apply,
                    )
                )
            _print_json(_commit_payload(commit, details))
            return 0
        if args.command == "work" and args.work_command == "return":
            scheduler_document, scheduler_domains = _precheck_scheduler_revision(args)
            try:
                returned_result = json.loads(args.result_json)
            except json.JSONDecodeError as exc:
                raise ProgressError(f"explicit maintenance result is invalid JSON: {exc}") from exc
            if not isinstance(returned_result, dict):
                raise ProgressError("explicit maintenance result must be an object")
            details: dict[str, Any] = {}

            def transform(data: dict[str, Any]) -> None:
                details.update(
                    return_explicit_maintenance_work_item(
                        data,
                        args.id,
                        returned_result,
                        progress_path=args.progress,
                    )
                )

            commit = (
                _call_contract_scoped_patch_commit(
                    args=args,
                    document=scheduler_document,
                    transform=transform,
                    expected_domains=scheduler_domains,
                    increment_domains={"scheduler"},
                )
                if scheduler_domains is not None
                else ProgressStore(args.progress).mutate(
                    transform,
                    expected_revision=args.expected_revision,
                    apply=args.apply,
                )
            )
            _print_json(_commit_payload(commit, details))
            return 0
        if args.command == "work" and args.work_command == "return-binja":
            _print_json(return_explicit_maintenance_work_with_binja(args))
            return 0
        if args.command == "work" and args.work_command == "recover-expired":
            scheduler_document, scheduler_domains = _precheck_scheduler_revision(args)
            details: dict[str, Any] = {}

            def transform(data: dict[str, Any]) -> None:
                details.update(
                    recover_expired_explicit_maintenance_work_item(data, args.id)
                )

            commit = (
                _call_contract_scoped_patch_commit(
                    args=args,
                    document=scheduler_document,
                    transform=transform,
                    expected_domains=scheduler_domains,
                    increment_domains={"scheduler"},
                )
                if scheduler_domains is not None
                else ProgressStore(args.progress).mutate(
                    transform,
                    expected_revision=args.expected_revision,
                    apply=args.apply,
                )
            )
            _print_json(_commit_payload(commit, details))
            return 0
        if args.command == "work" and args.work_command == "recover-allocation":
            _print_json(recover_explicit_maintenance_allocation(args))
            return 0
        if args.command == "work" and args.work_command == "close":
            scheduler_document, scheduler_domains = _precheck_scheduler_revision(args)
            details: dict[str, Any] = {}

            special_tool_blocked = args.outcome == "returned-tool-blocked"
            if special_tool_blocked != bool(args.linked_tool_issue):
                raise ProgressError(
                    "returned-tool-blocked requires exactly one --linked-tool-issue, "
                    "which is invalid for ordinary close outcomes"
                )
            if special_tool_blocked and args.abandonment_reason:
                raise ProgressError(
                    "returned-tool-blocked cannot carry an abandonment reason"
                )
            close_document = (
                scheduler_document
                if scheduler_document is not None
                else ProgressStore(args.progress).load()
            )
            closing_work = close_document.collection("work_items").get(args.work_id)
            continuation_tool_blocked = bool(
                special_tool_blocked
                and isinstance(closing_work, Mapping)
                and _is_call_contract_continuation_predecessor(closing_work)
            )
            if (
                continuation_tool_blocked
                and args.linked_tool_issue != LINKED_TOOL_ISSUE
            ):
                raise ProgressError(
                    f"call-contract repair continuation is governed only by {LINKED_TOOL_ISSUE}"
                )
            linked_issue_snapshot: dict[str, Any] | None = None
            if special_tool_blocked:
                from _recoil.commands.workspace_issues import issue_store

                linked_issue_ledger = issue_store(Path(args.issue_ledger)).load()
                issue_matches = [
                    row
                    for row in linked_issue_ledger.get("issues", [])
                    if isinstance(row, Mapping)
                    and row.get("id") == args.linked_tool_issue
                ]
                if len(issue_matches) != 1 or issue_matches[0].get("status") not in {
                    "open",
                    "in-progress",
                }:
                    raise ProgressError(
                        "linked tool issue must exist exactly once and remain open"
                    )
                linked_issue_snapshot = deepcopy(linked_issue_ledger)

            def transform(data: dict[str, Any]) -> None:
                current = ProgressDocument(data, path=args.progress)
                migration = data.get("migration", {})
                carry_candidate = (
                    migration.get(CONVERGENCE_MIGRATION_KEY)
                    if isinstance(migration, Mapping)
                    else None
                )
                semantic_projection_before = (
                    _normalized_semantic_projection(current)
                    if scheduler_domains is None
                    and isinstance(carry_candidate, Mapping)
                    and carry_candidate.get("contract_version")
                    == CONVERGENCE_CONTRACT_VERSION
                    else None
                )
                work_items = data.get("work_items", {})
                if not isinstance(work_items, dict):
                    raise ProgressError("progress work_items collection must be an object")
                work = work_items.get(args.work_id)
                if not isinstance(work, dict):
                    raise ProgressError(f"unknown work item {args.work_id}")
                if (
                    work.get("packet_type") == EXPLICIT_MAINTENANCE_PACKET_TYPE
                    and work.get("state") == "returned"
                ):
                    if args.outcome != "closed" or args.abandonment_reason:
                        raise ProgressError(
                            "a returned explicit maintenance packet may only be closed"
                        )
                    work["state"] = "closed"
                    work["closed_outcome"] = "closed"
                    work["nonaccepting"] = True
                    work["acceptance_eligible"] = False
                    details.update(
                        {
                            "work_item_id": args.work_id,
                            "outcome": "closed",
                            "returned_result_bytes_hex": work.get(
                                "returned_result_bytes_hex"
                            ),
                            "acceptance_changed": False,
                            "explicit_packet_closed": True,
                            "terminal_record_retained": True,
                        }
                    )
                    return
                if (
                    work.get("packet_type") == EXPLICIT_MAINTENANCE_PACKET_TYPE
                    and work.get("state") == "active"
                    and args.outcome != "abandoned"
                ):
                    raise ProgressError(
                        "active explicit maintenance packets return only through "
                        "progress work return; progress work close may only abandon them"
                    )
                if (
                    work.get("packet_type") == EXPLICIT_MAINTENANCE_PACKET_TYPE
                    and args.outcome == "abandoned"
                    and not args.abandonment_reason
                ):
                    raise ProgressError(
                        "abandoning an explicit maintenance packet requires a reason"
                    )
                if continuation_tool_blocked:
                    state = continuation_state(current)
                    if _continuation_blocks_primary_scheduler(state):
                        raise ProgressError(
                            "a repair continuation is already active or awaiting full closeout"
                        )
                reservation = work.get("reservation")
                if isinstance(reservation, dict):
                    reservation["state"] = "released"
                    reservation["outcome"] = args.outcome
                if work.get("packet_type") == EXPLICIT_MAINTENANCE_PACKET_TYPE:
                    attempts = work.get("execution_attempts")
                    if isinstance(attempts, list):
                        for attempt in attempts:
                            if (
                                isinstance(attempt, dict)
                                and isinstance(reservation, Mapping)
                                and attempt.get("id") == reservation.get("id")
                            ):
                                attempt["state"] = "released"
                                attempt["outcome"] = args.outcome
                    work["nonaccepting"] = True
                    work["acceptance_eligible"] = False
                work["state"] = args.outcome
                if args.abandonment_reason:
                    work["abandonment_reason"] = args.abandonment_reason
                details.update({"work_item_id": args.work_id, "outcome": args.outcome})
                if continuation_tool_blocked:
                    provenance = returned_tool_blocked_provenance(
                        current,
                        args.work_id,
                        work,
                        str(args.linked_tool_issue),
                    )
                    work[RETURN_PROVENANCE_FIELD] = provenance
                    details.update(
                        {
                            "retained_terminal_predecessor": True,
                            "linked_tool_issue_id": args.linked_tool_issue,
                            "linked_issue_ledger_revision": (
                                linked_issue_snapshot.get("revision")
                                if isinstance(linked_issue_snapshot, Mapping)
                                else None
                            ),
                            "full_convergence_required": True,
                            "noncurrent": True,
                            "nonaccepting": True,
                            "acceptance_eligible": False,
                        }
                    )
                elif special_tool_blocked:
                    work[TOOL_BLOCKED_PROVENANCE_FIELD] = (
                        _generic_tool_blocked_return_provenance(
                            args.work_id,
                            work,
                            str(args.linked_tool_issue),
                        )
                    )
                    details.update(
                        {
                            "retained_terminal_work_item": True,
                            "linked_tool_issue_id": args.linked_tool_issue,
                            "linked_issue_ledger_revision": (
                                linked_issue_snapshot.get("revision")
                                if isinstance(linked_issue_snapshot, Mapping)
                                else None
                            ),
                            "call_contract_continuation": False,
                            "noncurrent": True,
                            "nonaccepting": True,
                            "acceptance_eligible": False,
                        }
                    )
                elif work.get("packet_type") == CONTINUATION_PACKET_TYPE:
                    migration = data.get("migration", {})
                    checkpoint = (
                        migration.get(CONTINUATION_MIGRATION_KEY)
                        if isinstance(migration, dict)
                        else None
                    )
                    if not isinstance(checkpoint, Mapping):
                        raise ProgressError(
                            "continuation child has no exact active checkpoint"
                        )
                    migration[CONTINUATION_MIGRATION_KEY] = (
                        finalize_continuation_child(
                            checkpoint,
                            child_work_item_id=args.work_id,
                            child_work=work,
                        )
                    )
                    journal = migration.get("progress_packet_allocation_journals")
                    journal_rows = journal.get("rows") if isinstance(journal, dict) else None
                    journal_row = journal_rows.get(args.work_id) if isinstance(journal_rows, dict) else None
                    if isinstance(journal_row, dict):
                        journal_row["state"] = "terminal"
                    details.update(
                        {
                            "continuation_finalized": True,
                            "full_convergence_required": True,
                            "noncurrent": True,
                            "nonaccepting": True,
                            "acceptance_eligible": False,
                        }
                    )
                elif work.get("packet_type") == EXPLICIT_MAINTENANCE_PACKET_TYPE:
                    details.update(
                        {
                            "acceptance_changed": False,
                            "terminal_record_retained": True,
                            "nonaccepting": True,
                            "acceptance_eligible": False,
                        }
                    )
                else:
                    work_items.pop(args.work_id)
                details["convergence_generation_carried"] = bool(
                    scheduler_domains is None
                    and semantic_projection_before is not None
                    and not continuation_tool_blocked
                    and work.get("packet_type") != CONTINUATION_PACKET_TYPE
                ) and (
                    carry_current_generation_across_work_ledger_mutation(
                        data,
                        expected_revision=current.revision,
                        semantic_projection_before=semantic_projection_before,
                    )
                )

            commit = (
                _call_contract_scoped_patch_commit(
                    args=args,
                    document=scheduler_document,
                    transform=transform,
                    expected_domains=scheduler_domains,
                    increment_domains={"scheduler"},
                )
                if scheduler_domains is not None
                else ProgressStore(args.progress).mutate(
                    transform,
                    expected_revision=args.expected_revision,
                    apply=args.apply,
                )
            )
            _print_json(_commit_payload(commit, details))
            return 0
        if args.command == "verification-target" and args.verification_target_command == "sync":
            details: dict[str, Any] = {}

            def transform(data: dict[str, Any]) -> None:
                details.update(
                    _sync_verification_targets(
                        data,
                        binary=args.binary,
                        selectors=args.target,
                        source_policy_bootstrap=args.source_policy_bootstrap,
                        revalidate_accepted_order=(
                            args.revalidate_accepted_order
                        ),
                    )
                )

            commit = ProgressStore(args.progress).mutate(
                transform,
                expected_revision=args.expected_revision,
                apply=args.apply,
            )
            _print_json(_commit_payload(commit, details))
            return 0
        if args.command == "verification-target" and args.verification_target_command == "retire":
            details = {}

            def transform(data: dict[str, Any]) -> None:
                details.update(
                    _retire_verification_target(
                        data,
                        selector=args.target,
                    )
                )

            commit = ProgressStore(args.progress).mutate(
                transform,
                expected_revision=args.expected_revision,
                apply=args.apply,
            )
            _print_json(_commit_payload(commit, details))
            return 0
        raise ProgressError(f"unsupported progress command {args.command}")
    except (
        ProgressError,
        BridgeError,
        ConcurrentProgressUpdate,
        ConcurrentRevisionUpdate,
        OSError,
        ValueError,
    ) as exc:
        print(f"progress error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
