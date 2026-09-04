from __future__ import annotations

import argparse
from collections import Counter
from contextvars import ContextVar
from copy import deepcopy
from functools import wraps
import json
from pathlib import Path
import re
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
from _recoil.commands.call_contract_verify import (
    CallContractSourceClosure,
    _decorated_coff_name,
    _finite_literal_symbol_regex_alternatives,
    _resolve_phase_all_authored_bodies,
    call_contract_source_closure,
    file_dependency_states,
    live_call_contract_result,
)
from _recoil.commands import storage_contribution_progress as _authored_storage_progress
from _recoil.commands.storage_contribution_progress import (
    load_payload as load_authored_storage_payload,
)
from _recoil.lib.progress import (
    AUTHORED_BYTE_DIMENSIONS,
    AUTHORED_ORDER_DIMENSIONS,
    CALL_CONTRACT_CLOSEOUT_SCHEMA,
    CALL_CONTRACT_DIMENSION,
    CALL_CONTRACT_CONTRACT_VERSION,
    CALL_CONTRACT_EXPECTED_TRUTH,
    CALL_CONTRACT_LINKABILITY_SCHEMA,
    EXACT_LINK_DIMENSIONS,
    FULL_ORDER_DIMENSIONS,
    ConcurrentProgressUpdate,
    ProgressDocument,
    ProgressError,
    ProgressStore,
    accept_live_authored_non_gating_blocks,
    address_value,
    is_current_accepted_state,
    invalidate_order_dependencies,
    normalize_address,
    state_record,
    symbol_authored_order_gate,
    validate_authored_order_role,
    validate_owner_invariants,
)
from _recoil.lib.repository_paths import (
    RepositoryPathInventory,
    RepositoryPathError,
    RepositoryFile,
    load_repository_path_inventory,
    resolve_repository_file,
)
from _recoil.lib.live_progress import ConcurrentRevisionUpdate
from _recoil.lib.pe import parse_pe_headers, rva_to_offset
from _recoil.lib.source_traceability import parse_source_trace_text
from _recoil.lib.progress_sqlite import (
    DELETE_FACET,
    ProgressSQLiteStore,
)
from _recoil.lib.call_contract_generations import (
    current_generations,
    evidence_generations_current,
    required_call_contract_verifier_component_findings,
    required_call_contract_verifier_component_graph,
)
from _recoil.commands.source_trace_progress import normalize_source_traceability
from _recoil.lib.authored_icf import (
    AUTHORED_ICF_GROUP_MODEL,
    AUTHORED_ICF_MEMBER_GATE_MODE,
    audit_authored_icf_groups,
    validate_authored_icf_proof,
    validate_authored_icf_source_mirrors,
)
from _recoil.lib.binja import BridgeError
from _recoil.lib.tooling import REPO_ROOT, configure_stdio, display_path
PROGRESS_AUTHORITY_RELATIVE_PATH = ".agent/RECONSTRUCTION_PROGRESS.sqlite3"
DEFAULT_PROGRESS = REPO_ROOT / PROGRESS_AUTHORITY_RELATIVE_PATH
MACHINE_RETAIL_REFERENCE = REPO_ROOT / "support/Recoil.exe"
MAX_PROGRESS_PAYLOAD_FILE_BYTES = 16 * 1024 * 1024
ORDER_PHASES = {"authored-function-order", "full-function-order"}
SOURCE_POLICY_BOOTSTRAP_STATE = "pending-source-placement"
BYTE_VERIFY_COMMANDS = {
    "authored": "authored-byte",
    "linked": "linked-byte",
}
DIVERGENCE_KINDS = {"missing", "extra", "duplicate", "reordered"}
PIPELINE_CLASSES = {"authored", "authored-lifecycle", "non-authored", "unresolved"}
CALL_CONTRACT_FINAL_BUILD_MANIFEST = (
    REPO_ROOT / "tools" / "_recoil" / "config" / "vc5_final_build.json"
)


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


def _call_contract_scoped_patch_plan(
    document: ProgressDocument,
    transform: Any,
) -> tuple[
    dict[str, Any],
    dict[str, dict[str, dict[str, Any]]],
    dict[str, dict[str, Any]],
]:
    """Build the exact narrow patch set for one call-contract transform."""

    proposed = deepcopy(document.data)
    transform(proposed)
    entity_patches: dict[str, dict[str, dict[str, Any]]] = {}
    for collection in (
        "symbols",
        "physical_blocks",
        "semantic_spans",
        "source_owners",
        "verification_targets",
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
    return proposed, entity_patches, top_level_patches


def _call_contract_scoped_patch_commit(
    *,
    args: argparse.Namespace,
    document: ProgressDocument,
    transform: Any,
    expected_domains: Mapping[str, int],
    increment_domains: Iterable[str] | None = None,
) -> Any:
    """Apply one transform as narrow entity/facet patches under domain CAS."""

    _proposed, entity_patches, top_level_patches = (
        _call_contract_scoped_patch_plan(document, transform)
    )
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


def _require_current_call_contract_action(
    document: ProgressDocument,
    *,
    requested_slice: str | None,
) -> None:
    """Reject any call-contract mutation outside the sole current action."""

    state = document.pipeline("recoil", resolve_order_target=False)
    if state.get("phase") != "authored-call-contract":
        raise ProgressError(
            "call-contract mutation requires authored-call-contract to be the "
            f"current serial stage, found {state.get('phase')!r}"
        )
    current_slice = str(state.get("authored_call_contract_slice_id") or "")
    if requested_slice is not None:
        if current_slice != requested_slice:
            raise ProgressError(
                "call-contract mutation requires the sole current slice "
                f"{current_slice or '<closeout>'!r}, found {requested_slice!r}"
            )
        return
    if current_slice:
        raise ProgressError(
            "call-contract closeout is not current; the sole current slice is "
            f"{current_slice!r}"
        )
    closeout = state.get("authored_call_contract_closeout")
    if isinstance(closeout, Mapping) and closeout.get("current") is True:
        raise ProgressError("call-contract closeout is already current")






def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Inspect unified progress and atomically advance it from live validators."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    for name, help_text in (
        ("next", "Select the authoritative next Recoil.exe task."),
        ("status", "Show derived pipeline status."),
    ):
        child = subparsers.add_parser(name, help=help_text)
        _add_progress_path(child)
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

    audit = subparsers.add_parser("audit", help="Audit schema-v6 progress invariants.")
    _add_progress_path(audit)
    audit.add_argument("--scope", default="all")
    audit.add_argument("--strict", action="store_true")
    audit.add_argument("--json", action="store_true")

    for name in ("output-section", "storage", "block", "semantic"):
        record_parser = subparsers.add_parser(name, help=f"Inspect {name} records.")
        children = record_parser.add_subparsers(dest=f"{name.replace('-', '_')}_command", required=True)
        child = children.add_parser("show")
        _add_progress_path(child)
        child.add_argument("selector")
        child.add_argument("--json", action="store_true")
        if name == "storage":
            register_authored_data = children.add_parser(
                "register-authored-data",
                help=(
                    "Register one exact non-overlapping "
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
                    "Reviewed reclassification of one exact stale authored "
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
                    "Reviewed acceptance of one exact contiguous live-cursor "
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
                    "Reviewed replacement of one exact physical block, "
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
                    "for reviewed replacement payloads that exceed the Windows command-line limit."
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
            "Reviewed ICF logical-alias group mutation with exact "
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
            "object; use this for reviewed logical-alias payloads that exceed the "
            "Windows command-line limit."
        ),
    )
    _add_mutation_controls(symbol_alias_group)
    symbol_replace_padding = symbol_children.add_parser(
        "replace-padding",
        help=(
            "Reviewed removal of one false function identity whose exact "
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
    owner_replace = owner_children.add_parser(
        "replace-batch",
        help=(
            "Atomic replacement of exact reviewed current owner records, "
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
            "Atomic conservative downgrade of selected gates and "
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
            "Registration bootstrap for one reviewed complete order target "
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
            "order, and required object-target identity paired with linked acceptance during "
            "full order."
        ),
    )
    advance_order.add_argument(
        "--linked-target",
        help="Exact registered full-order target override for an empty or stale block binding.",
    )
    advance_order.add_argument("--build-root", type=Path, required=True)
    _add_mutation_controls(advance_order)

    for command, mode in (
        ("advance-live-authored-byte", "authored"),
        ("advance-live-linked-byte", "linked"),
    ):
        advance_byte = subparsers.add_parser(
            command,
            help=f"Run the direct live {mode} byte validator and commit matched groups.",
        )
        advance_byte.set_defaults(mode=mode)
        _add_progress_path(advance_byte)
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
    advance_call_contract.add_argument("--build-root", type=Path, required=True)
    _add_mutation_controls(
        advance_call_contract,
        expected_revision_required=False,
        expected_revision_mode="none",
    )
    _add_call_contract_domain_guards(advance_call_contract)

    call_contract = subparsers.add_parser(
        "call-contract",
        help="Close the completed authored call-contract census from a fresh live scan.",
    )
    call_contract_children = call_contract.add_subparsers(
        dest="call_contract_command",
        required=True,
    )
    call_contract_close = call_contract_children.add_parser(
        "close-live",
        help="Run a fresh no-reuse complete-census scan and record the closeout.",
    )
    _add_progress_path(call_contract_close)
    call_contract_close.add_argument("--build-root", type=Path, required=True)
    _add_mutation_controls(
        call_contract_close,
        expected_revision_required=False,
        expected_revision_mode="none",
    )
    _add_call_contract_domain_guards(call_contract_close)

    return parser


def _load(path: Path) -> ProgressDocument:
    return ProgressStore(path).load()


def _print_json(value: Any) -> None:
    print(json.dumps(value, indent=2, ensure_ascii=False))


def _print_pipeline(value: Mapping[str, Any]) -> None:
    for key in (
        "stage",
        "task_id",
        "state",
        "cursor",
        "objective",
        "check_command",
        "stage_runner_command",
        "acceptance_command",
        "blocker",
    ):
        if key in value and value[key] is not None:
            print(f"{key}={value[key]}")




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
    return document.with_revision_vector(
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










def _command_arg(value: str) -> str:
    return f'"{value.replace(chr(34), chr(34) * 2)}"' if any(ch.isspace() for ch in value) else value


def _progress_command_path(path: Path) -> str:
    """Render a machine-local SQLite path without assigning repository identity."""

    resolved = path.resolve()
    try:
        return resolved.relative_to(REPO_ROOT.resolve()).as_posix()
    except ValueError:
        return resolved.as_posix()










_PROGRESS_REPOSITORY_DOCUMENT: ContextVar[ProgressDocument | None] = ContextVar(
    "progress-repository-path-operation-document",
    default=None,
)


def _load_progress_repository_inventory(
    document: ProgressDocument | None = None,
) -> RepositoryPathInventory:
    if document is None:
        document = _PROGRESS_REPOSITORY_DOCUMENT.get()
    cache_key = ("repository-path-inventory", str(REPO_ROOT))
    request_cache = getattr(document, "_request_cache", None)
    if isinstance(request_cache, dict):
        cached = request_cache.get(cache_key)
        if isinstance(cached, RepositoryPathInventory):
            return cached
    try:
        inventory = load_repository_path_inventory(REPO_ROOT)
    except RepositoryPathError as exc:
        raise ProgressError(str(exc)) from exc
    if isinstance(request_cache, dict):
        request_cache[cache_key] = inventory
    return inventory


def _with_progress_repository_inventory(function):
    """Scope one immutable canonical-checkout inventory to one operation."""

    @wraps(function)
    def wrapped(document: ProgressDocument, *args, **kwargs):
        if _PROGRESS_REPOSITORY_DOCUMENT.get() is document:
            return function(document, *args, **kwargs)
        token = _PROGRESS_REPOSITORY_DOCUMENT.set(document)
        try:
            return function(document, *args, **kwargs)
        finally:
            _PROGRESS_REPOSITORY_DOCUMENT.reset(token)

    return wrapped


def _resolve_progress_repository_file(
    path_text: str,
    *,
    context: str,
    inventory: RepositoryPathInventory | None = None,
) -> RepositoryFile:
    current_inventory = inventory or _load_progress_repository_inventory()
    try:
        return resolve_repository_file(
            path_text,
            repository_root=REPO_ROOT,
            inventory=current_inventory,
            context=context,
        )
    except RepositoryPathError as exc:
        raise ProgressError(str(exc)) from exc
























































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
    repository_manifest = _resolve_progress_repository_file(
        manifest_value,
        context="registered order manifest",
    )
    manifest_path = repository_manifest.physical_path
    try:
        payload = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ProgressError(
            f"cannot read registered order manifest {repository_manifest.repository_path}: {exc}"
        ) from exc
    if not isinstance(payload, Mapping):
        raise ProgressError(
            f"registered order manifest {repository_manifest.repository_path} must contain an object"
        )
    start = payload.get("retail_start")
    end = payload.get("retail_end_exclusive")
    if not isinstance(start, str) or not isinstance(end, str) or not start or not end:
        raise ProgressError(
            f"registered order manifest {repository_manifest.repository_path} lacks an exact retail interval"
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
    repository_manifest = _resolve_progress_repository_file(
        manifest_value,
        context=f"order target {target_id!r} registered manifest",
    )
    if not repository_manifest.repository_path.startswith("tools/vc5_verify_targets/"):
        raise ProgressError(
            f"order target {target_id!r} manifest is outside tools/vc5_verify_targets"
        )
    manifest_path = repository_manifest.physical_path

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
    authorize acceptance for the complete covered block.
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


@_with_progress_repository_inventory
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


@_with_progress_repository_inventory
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
            "the direct vc5-order compile loop"
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
                "object_target_id": object_target_id,
                "object_target": object_contract["target"],
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
            "validation_target_id": verifier_target_id,
            "validation_target": verifier["target"],
            "verifier_target": verifier["target"],
            "verifier_target_id": verifier_target_id,
            "override_selector": override_selector or "",
            "configured_selectors": selectors if not override_selector else [],
        }
    )
    return result


@_with_progress_repository_inventory
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
            "object_target_name": str(contract["object_target"].get("name") or ""),
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
    inventory = _load_progress_repository_inventory()
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
        manifest_path = _resolve_progress_repository_file(
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
    current = resolve_current_order_target(document)
    if current.get("status") != "ready":
        raise ProgressError(
            "order mutation requires one ready current serial target: "
            + str(current.get("reason") or current.get("status"))
        )
    if str(current.get("target_id") or "") != str(args.target):
        raise ProgressError(
            "order mutation requires the sole current target "
            f"{current.get('target_id')!r}, found {args.target!r}"
        )
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


def _byte_groups(document: ProgressDocument, mode: str) -> list[dict[str, Any]]:
    if mode == "authored":
        groups = document._physical_groups("recoil", gating_only=True)
    else:
        groups = document._physical_groups("recoil", gating_only=False)
    return groups


def _validate_byte_mode_eligibility(document: ProgressDocument, mode: str) -> None:
    pipeline = document.pipeline("recoil")
    expected_stage = {
        "authored": "authored-byte-match",
        "linked": "linked-byte-match",
    }.get(mode)
    if expected_stage is None:
        raise ProgressError(f"unsupported serial byte stage {mode!r}")
    if pipeline.get("phase") != expected_stage:
        raise ProgressError(
            f"{mode} byte advancement requires {expected_stage}, "
            f"found {pipeline.get('phase')!r}"
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
    mode: str,
    expected_groups: list[dict[str, Any]],
) -> dict[str, Any]:
    if result.get("kind") != "live-byte-mode" or result.get("mode") != mode:
        raise ProgressError("live byte validator returned the wrong mode result")
    matched = _normalize_matched_groups(result.get("matched_groups"))
    if len(matched) > len(expected_groups):
        raise ProgressError("live byte result matched more groups than the tracker mode contains")
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
            raise ProgressError("passing live byte result must match the complete selected mode")
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
    _validate_byte_mode_eligibility(document, args.mode)
    groups = _byte_groups(document, args.mode)
    if not groups:
        raise ProgressError(f"the {args.mode} byte mode has no selected physical groups")
    build_root = _absolute_fresh_build_root(args.build_root)
    command = [
        sys.executable,
        str(REPO_ROOT / "tools" / "recoil.py"),
        "verify",
        BYTE_VERIFY_COMMANDS[args.mode],
        "--progress",
        display_path(args.progress),
        "--build-root",
        display_path(build_root),
    ]
    command.append("--json")
    returncode, raw, stderr = _run_json_process(command)
    try:
        result = _validate_byte_result(raw, mode=args.mode, expected_groups=groups)
    except ProgressError as exc:
        if "lacks matched_groups" not in str(exc):
            raise
        return 2, {
            "kind": "live-byte-advance",
            "status": "contract-blocked",
            "mode": args.mode,
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
    if args.mode == "authored":
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
        "mode": args.mode,
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
            summary=f"Live {args.mode} byte comparison accepted {len(groups_to_commit)} physical groups",
            scope_ids=flat_scope,
            provenance={
                "mode": args.mode,
                "matched_groups": groups_to_commit,
                "first_divergence": result["first_divergence"],
            },
        )
        accept_live_byte_groups(
            data,
            mode=args.mode,
            groups=groups_to_commit,
            evidence_id=evidence_id,
            facts={"validation_mode": "live", "mode": args.mode},
        )
        details["evidence_id"] = evidence_id

    commit = store.mutate(
        transform,
        expected_revision=args.expected_revision,
        apply=args.apply,
    )
    return (0 if result["passed"] else 1), _commit_payload(commit, details)


def _call_contract_separate_definition_compile_sources(
    closure: CallContractSourceClosure,
) -> list[str]:
    """Return definition TUs not already compiled by registered targets."""

    registered = {
        path.casefold() for path in closure.registered_source_paths
    }
    return [
        path
        for path in closure.definition_source_paths
        if path.casefold() not in registered
    ]


def _validate_call_contract_result(
    result: Mapping[str, Any],
    *,
    expected_slice: Mapping[str, Any],
    expected_source_write_paths: list[str],
    expected_definition_source_paths: list[str],
    expected_compiled_definition_sources: list[str],
    expected_dependency_paths: list[str],
) -> dict[str, Any]:
    if (
        result.get("kind") != "authored-call-contract-live-result"
        or result.get("contract_version") != CALL_CONTRACT_CONTRACT_VERSION
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
    result_passed = result.get("passed") is True
    if (
        result_passed
        and compiled_sources != expected_compiled_definition_sources
    ) or (
        not result_passed
        and compiled_sources
        and compiled_sources != expected_compiled_definition_sources
    ):
        raise ProgressError("call-contract direct result did not compile the exact definition closure")
    raw_results = result.get("body_results")
    symbol_ids = list(expected_slice.get("symbol_ids", []))
    addresses = list(expected_slice.get("addresses", []))
    if not isinstance(raw_results, list) or len(raw_results) != len(symbol_ids):
        raise ProgressError("call-contract direct result lacks one result per selected body")
    raw_transcript = result.get("exact_fact_transcript")
    provider_transcript = result.get("provider_fact_transcript")
    if (
        not isinstance(raw_transcript, list)
        or not isinstance(provider_transcript, list)
        or (result_passed and len(raw_transcript) != len(symbol_ids))
        or (not result_passed and len(raw_transcript) > len(symbol_ids))
    ):
        raise ProgressError(
            "call-contract direct result lacks one exact expected-fact transcript row per selected body"
        )
    coordinate_indexes = {
        (str(symbol_id), normalize_address(str(address))): index
        for index, (symbol_id, address) in enumerate(zip(symbol_ids, addresses))
    }
    transcript: list[dict[str, Any]] = []
    transcript_by_symbol: dict[str, dict[str, Any]] = {}
    prior_index = -1
    for raw_row in raw_transcript:
        symbol_id = raw_row.get("symbol_id") if isinstance(raw_row, Mapping) else None
        raw_address = raw_row.get("address") if isinstance(raw_row, Mapping) else None
        try:
            normalized_address = normalize_address(str(raw_address))
        except (ProgressError, ValueError):
            normalized_address = ""
        coordinate = (str(symbol_id), normalized_address)
        coordinate_index = coordinate_indexes.get(coordinate)
        if (
            not isinstance(raw_row, Mapping)
            or coordinate_index is None
            or coordinate_index <= prior_index
            or str(symbol_id) in transcript_by_symbol
            or not isinstance(raw_row.get("expected_fact_row"), Mapping)
        ):
            raise ProgressError(
                "call-contract expected-fact transcript disagrees with the current slice"
            )
        copied_row = deepcopy(dict(raw_row))
        transcript.append(copied_row)
        transcript_by_symbol[str(symbol_id)] = copied_row
        prior_index = coordinate_index
    body_results: list[dict[str, Any]] = []
    passing: list[str] = []
    for symbol_id, address, raw in zip(symbol_ids, addresses, raw_results):
        normalized_address = normalize_address(str(address))
        if (
            not isinstance(raw, Mapping)
            or raw.get("symbol_id") != symbol_id
            or raw.get("address") != normalized_address
        ):
            raise ProgressError(f"call-contract direct body result is incomplete for {symbol_id}")
        status = raw.get("status")
        if status not in {"passed", "divergent", "blocked", "not-evaluated"}:
            raise ProgressError(f"call-contract body result has invalid status {status!r}")
        transcript_row = transcript_by_symbol.get(str(symbol_id))
        if status == "not-evaluated":
            if raw.get("comparison_passed") is True:
                raise ProgressError(
                    f"call-contract not-evaluated body claims equality: {symbol_id}"
                )
            body_results.append(deepcopy(dict(raw)))
            continue
        if transcript_row is None:
            if status == "blocked":
                if (
                    raw.get("comparison_passed") is True
                    or not isinstance(raw.get("divergence"), Mapping)
                ):
                    raise ProgressError(
                        "call-contract blocked body without expected truth is "
                        f"malformed: {symbol_id}"
                    )
                body_results.append(deepcopy(dict(raw)))
                continue
            raise ProgressError(
                f"call-contract evaluated body lacks an exact fact transcript: {symbol_id}"
            )
        expected_fact_row = transcript_row["expected_fact_row"]
        if (
            raw.get("expected_fact_row") != expected_fact_row
            or raw.get("expected_contract") is None
            or raw.get("candidate_contract") is None
            or not evidence_generations_current(raw)
            or not evidence_generations_current(expected_fact_row)
            or expected_fact_row.get("symbol_id") != symbol_id
            or expected_fact_row.get("address") != normalized_address
            or expected_fact_row.get("calls") != raw.get("expected_contract")
        ):
            raise ProgressError(f"call-contract direct body result is incomplete for {symbol_id}")
        if status == "passed":
            if (
                raw.get("comparison_passed") is not True
                or raw.get("divergence") is not None
            ):
                raise ProgressError(
                    "call-contract passing body lacks a passing direct "
                    f"comparison: {symbol_id}"
                )
            passing.append(str(symbol_id))
        elif raw.get("comparison_passed") is True:
            raise ProgressError(f"call-contract nonpassing body claims equality: {symbol_id}")
        body_results.append(deepcopy(dict(raw)))
    if result_passed and passing != symbol_ids:
        raise ProgressError(
            "call-contract passing result does not pass every selected body"
        )
    return {
        "passed": result_passed,
        "first_divergence": deepcopy(result.get("first_divergence")),
        "body_results": body_results,
        "passing_symbol_ids": passing,
        "exact_fact_transcript": transcript,
        "provider_fact_transcript": deepcopy(provider_transcript),
    }


_CALL_CONTRACT_REPLAY_SCHEMA = "recoil-authored-call-contract-stage-replay-v2"
_CALL_CONTRACT_REPLAY_KIND = "authored-call-contract-stage-replay"
_CALL_CONTRACT_SLICE_ACCEPTANCE_RE = re.compile(
    r"^python tools/recoil\.py progress advance-live-call-contract "
    r"--slice (?P<slice>\S+) --build-root (?P<root>\S+) "
    r"--expected-semantic-revision (?P<semantic>\d+) "
    r"--expected-evidence-generation-revision (?P<evidence>\d+) "
    r"--apply --json$"
)
_CALL_CONTRACT_CLOSEOUT_ACCEPTANCE_RE = re.compile(
    r"^python tools/recoil\.py progress call-contract close-live "
    r"--build-root (?P<root>\S+) "
    r"--expected-semantic-revision (?P<semantic>\d+) "
    r"--expected-evidence-generation-revision (?P<evidence>\d+) "
    r"--apply --json$"
)


def _call_contract_replay_current_action(
    document: ProgressDocument,
    revision_vector: Mapping[str, int],
) -> dict[str, Any]:
    task = document.current_task("recoil")
    if (
        task.get("schema") != "recoil-current-task-v2"
        or task.get("stage") != "authored-call-contract"
        or task.get("state") != "ready"
    ):
        raise ProgressError(
            "call-contract replay requires one ready authored-call-contract task"
        )
    command = task.get("acceptance_command")
    if not isinstance(command, str):
        raise ProgressError("call-contract replay current task lacks an acceptance command")
    if task.get("revision_vector") != dict(revision_vector):
        raise ConcurrentProgressUpdate(
            "call-contract replay tracker changed after the atomic task snapshot"
        )
    slice_match = _CALL_CONTRACT_SLICE_ACCEPTANCE_RE.fullmatch(command)
    closeout_match = _CALL_CONTRACT_CLOSEOUT_ACCEPTANCE_RE.fullmatch(command)
    if slice_match is None and closeout_match is None:
        raise ProgressError(
            "call-contract replay current acceptance is neither a direct slice nor close-live"
        )
    match = slice_match or closeout_match
    assert match is not None
    semantic = int(match.group("semantic"))
    evidence = int(match.group("evidence"))
    if (
        semantic != int(revision_vector["semantic_revision"])
        or evidence != int(revision_vector["evidence_generation_revision"])
    ):
        raise ProgressError(
            "call-contract replay scheduler guards disagree with the atomic tracker snapshot"
        )
    if closeout_match is not None:
        return {
            "kind": "closeout",
            "acceptance_command": command,
            "revision_vector": dict(revision_vector),
        }
    slice_id = str(match.group("slice"))
    if task.get("task_id") != slice_id:
        raise ProgressError(
            "call-contract replay slice acceptance disagrees with the current task identity"
        )
    direct_root = _absolute_fresh_build_root(Path(str(match.group("root"))))
    return {
        "kind": "slice",
        "slice_id": slice_id,
        "direct_build_root": direct_root,
        "acceptance_command": command,
        "revision_vector": dict(revision_vector),
    }


def _call_contract_replay_root(direct_root: Path, *, create: bool) -> Path:
    base = (REPO_ROOT / "build" / "live-validation" / "call-contract").resolve()
    try:
        direct_root.resolve().relative_to(base)
    except ValueError as exc:
        raise ProgressError(
            "call-contract replay scheduler root escaped the governed stage root"
        ) from exc
    for ordinal in range(1, 1000):
        candidate = direct_root.with_name(
            f"{direct_root.name}-replay-{ordinal:03d}"
        )
        if candidate.exists():
            continue
        if create:
            try:
                candidate.mkdir(parents=True, exist_ok=False)
            except FileExistsError:
                continue
        return candidate
    raise ProgressError("call-contract replay exhausted fresh sibling build roots")


def _call_contract_replay_component_state(
    repository_path_inventory: RepositoryPathInventory | None = None,
) -> dict[str, Any]:
    findings = required_call_contract_verifier_component_findings(REPO_ROOT)
    if findings:
        first = findings[0]
        raise ProgressError(
            "required call-contract verifier component is not operational: "
            f"{first.get('path')}: {first.get('detail')}"
        )
    graph = required_call_contract_verifier_component_graph()
    paths = [str(row["path"]) for row in graph]
    inventory = (
        repository_path_inventory
        or load_repository_path_inventory(REPO_ROOT)
    )
    return {
        "generations": current_generations(),
        "paths": paths,
        "states": file_dependency_states(
            paths,
            repository_path_inventory=inventory,
        ),
    }


def _call_contract_replay_require_unchanged(
    expected: Mapping[str, Any],
    *,
    dependency_paths: Iterable[str],
    expected_dependency_states: list[dict[str, Any]],
    repository_path_inventory: RepositoryPathInventory | None = None,
) -> None:
    if (
        _call_contract_replay_component_state(repository_path_inventory)
        != expected
    ):
        raise ProgressError(
            "call-contract verifier components or generation coordinates changed during replay"
        )
    current_dependencies = file_dependency_states(
        list(dependency_paths),
        repository_path_inventory=repository_path_inventory,
    )
    if current_dependencies != expected_dependency_states:
        raise ProgressError(
            "call-contract source dependency state changed after the full-census proof"
        )


def _call_contract_phase_compile_rows_by_source(
    result: Mapping[str, Any],
) -> dict[str, dict[str, Any]]:
    raw_rows = result.get("definition_compile_results")
    if not isinstance(raw_rows, list):
        raise ProgressError("call-contract phase result lacks definition compile rows")
    rows: dict[str, dict[str, Any]] = {}
    for raw in raw_rows:
        if not isinstance(raw, Mapping) or raw.get("returncode") != 0:
            raise ProgressError(
                "call-contract phase result contains an unsuccessful definition compile"
            )
        source = str(raw.get("source", ""))
        key = source.casefold()
        if not source or key in rows:
            raise ProgressError(
                "call-contract phase result repeats or omits a definition source"
            )
        rows[key] = deepcopy(dict(raw))
    return rows


def _project_call_contract_phase_result(
    document: ProgressDocument,
    result: Mapping[str, Any],
    *,
    repository_path_inventory: RepositoryPathInventory | None = None,
) -> tuple[list[dict[str, Any]], CallContractSourceClosure]:
    """Validate one complete proof and project it to the existing slice contract."""

    phase_scope = _resolve_phase_all_authored_bodies(document)
    if (
        result.get("kind") != "authored-call-contract-phase-replay-result"
        or result.get("contract_version") != CALL_CONTRACT_CONTRACT_VERSION
        or result.get("phase_all_authored_bodies") is not True
        or result.get("nonaccepting") is not True
        or result.get("acceptance_eligible") is not False
        or result.get("acceptance_route") != "project-to-original-slices"
        or result.get("all_caller_divergences_collected") is not True
        or result.get("candidate_expected_truth") is not False
    ):
        raise ProgressError("call-contract phase verifier returned the wrong governed result")
    exact_fields = (
        "body_count",
        "symbol_ids",
        "target_ids",
        "physical_block_ids",
        "original_slice_ids",
        "slice_boundaries",
    )
    for field in exact_fields:
        if result.get(field) != phase_scope.get(field):
            raise ProgressError(
                f"call-contract phase result {field} disagrees with the tracker census"
            )
    attempted_target_ids = result.get("attempted_target_ids")
    compiled_target_ids = result.get("compiled_target_ids")
    if (
        not isinstance(attempted_target_ids, list)
        or len(attempted_target_ids) != len(set(attempted_target_ids))
        or set(attempted_target_ids) != set(phase_scope["target_ids"])
        or not isinstance(compiled_target_ids, list)
        or len(compiled_target_ids) != len(set(compiled_target_ids))
        or not set(compiled_target_ids).issubset(set(attempted_target_ids))
    ):
        raise ProgressError(
            "call-contract phase did not attempt every unique target exactly once"
        )
    if (
        result.get("source_changed_during_validation") is not False
        or result.get("dependency_states_before")
        != result.get("dependency_states_after")
    ):
        raise ProgressError("call-contract phase lacks one stable exact source closure")

    inventory = (
        repository_path_inventory
        or load_repository_path_inventory(REPO_ROOT)
    )
    phase_closure = call_contract_source_closure(
        document,
        phase_scope,
        repository_path_inventory=inventory,
    )
    if (
        result.get("source_edit_paths") != list(phase_closure.source_edit_paths)
        or result.get("definition_source_paths")
        != list(phase_closure.definition_source_paths)
        or result.get("dependency_paths") != list(phase_closure.dependency_paths)
    ):
        raise ProgressError("call-contract phase source closure disagrees with live discovery")
    compile_rows = _call_contract_phase_compile_rows_by_source(result)
    expected_phase_compiles = _call_contract_separate_definition_compile_sources(
        phase_closure
    )
    if list(compile_rows) != [path.casefold() for path in expected_phase_compiles]:
        raise ProgressError(
            "call-contract phase did not compile each separate definition TU exactly once"
        )
    phase_registered_source_keys = {
        path.casefold() for path in phase_closure.registered_source_paths
    }
    phase_definition_source_keys = {
        path.casefold() for path in phase_closure.definition_source_paths
    }

    raw_bodies = result.get("body_results")
    raw_transcript = result.get("exact_fact_transcript")
    provider_transcript = result.get("provider_fact_transcript")
    raw_caller_divergences = result.get("caller_divergences")
    if (
        not isinstance(raw_bodies, list)
        or len(raw_bodies) != int(phase_scope["body_count"])
        or not isinstance(raw_transcript, list)
        or not isinstance(provider_transcript, list)
        or not isinstance(raw_caller_divergences, list)
    ):
        raise ProgressError("call-contract phase result lacks its complete body census")
    bodies_by_symbol = {
        str(row.get("symbol_id")): deepcopy(dict(row))
        for row in raw_bodies
        if isinstance(row, Mapping)
    }
    transcript_by_symbol = {
        str(row.get("symbol_id")): deepcopy(dict(row))
        for row in raw_transcript
        if isinstance(row, Mapping)
    }
    divergence_by_symbol = {
        str(row.get("symbol_id")): deepcopy(dict(row))
        for row in raw_caller_divergences
        if isinstance(row, Mapping) and row.get("symbol_id")
    }
    if len(bodies_by_symbol) != len(raw_bodies):
        raise ProgressError("call-contract phase body census repeats an identity")
    state_before_by_path = {
        str(row.get("path", "")).casefold(): deepcopy(dict(row))
        for row in result["dependency_states_before"]
        if isinstance(row, Mapping)
    }
    state_after_by_path = {
        str(row.get("path", "")).casefold(): deepcopy(dict(row))
        for row in result["dependency_states_after"]
        if isinstance(row, Mapping)
    }

    projections: list[dict[str, Any]] = []
    first_projected_divergence: Any = None
    for slice_row in document.authored_call_contract_slices():
        closure = call_contract_source_closure(
            document,
            slice_row,
            repository_path_inventory=inventory,
        )
        symbol_ids = [str(value) for value in slice_row["symbol_ids"]]
        slice_bodies = [bodies_by_symbol.get(symbol_id) for symbol_id in symbol_ids]
        if any(row is None for row in slice_bodies):
            raise ProgressError(
                f"call-contract phase omitted bodies from {slice_row['id']}"
            )
        slice_transcript = [
            transcript_by_symbol[symbol_id]
            for symbol_id in symbol_ids
            if symbol_id in transcript_by_symbol
        ]
        first_nonpassing = next(
            (
                row
                for row in slice_bodies
                if isinstance(row, Mapping) and row.get("status") != "passed"
            ),
            None,
        )
        slice_first_divergence = (
            deepcopy(
                divergence_by_symbol.get(
                    str(first_nonpassing.get("symbol_id", "")),
                    first_nonpassing.get("divergence"),
                )
            )
            if isinstance(first_nonpassing, Mapping)
            else None
        )
        if first_projected_divergence is None and slice_first_divergence is not None:
            first_projected_divergence = deepcopy(slice_first_divergence)
        expected_compiles: list[str] = []
        for path in closure.definition_source_paths:
            key = path.casefold()
            if key not in phase_definition_source_keys:
                raise ProgressError(
                    "call-contract slice definition closure escapes the complete phase"
                )
            compile_routes = int(key in phase_registered_source_keys) + int(
                key in compile_rows
            )
            if compile_routes != 1:
                raise ProgressError(
                    "call-contract slice definition source lacks one exact complete-phase "
                    "compile route"
                )
            if key in compile_rows:
                expected_compiles.append(path)
        dependency_keys = [path.casefold() for path in closure.dependency_paths]
        projected_raw = {
            "kind": "authored-call-contract-live-result",
            "contract_version": CALL_CONTRACT_CONTRACT_VERSION,
            "slice_id": str(slice_row["id"]),
            "body_count": int(slice_row["body_count"]),
            "symbol_ids": symbol_ids,
            "target_ids": list(slice_row["target_ids"]),
            "physical_block_ids": list(slice_row["physical_block_ids"]),
            "source_edit_paths": list(closure.source_edit_paths),
            "definition_source_paths": list(closure.definition_source_paths),
            "definition_compile_results": [
                deepcopy(compile_rows[path.casefold()])
                for path in expected_compiles
            ],
            "dependency_paths": list(closure.dependency_paths),
            "dependency_states_before": [
                deepcopy(state_before_by_path[key]) for key in dependency_keys
            ],
            "dependency_states_after": [
                deepcopy(state_after_by_path[key]) for key in dependency_keys
            ],
            "source_changed_during_validation": False,
            "passed": first_nonpassing is None,
            "body_results": slice_bodies,
            "exact_fact_transcript": slice_transcript,
            "provider_fact_transcript": deepcopy(provider_transcript),
            "first_divergence": slice_first_divergence,
            "candidate_expected_truth": False,
            "all_caller_divergences_collected": True,
        }
        validated = _validate_call_contract_result(
            projected_raw,
            expected_slice=slice_row,
            expected_source_write_paths=list(closure.source_edit_paths),
            expected_definition_source_paths=list(closure.definition_source_paths),
            expected_compiled_definition_sources=expected_compiles,
            expected_dependency_paths=list(closure.dependency_paths),
        )
        projections.append(
            {
                "slice_row": deepcopy(dict(slice_row)),
                "closure": closure,
                "result": validated,
            }
        )
    if (result.get("passed") is True) != all(
        row["result"]["passed"] for row in projections
    ):
        raise ProgressError("call-contract phase pass state disagrees with slice projections")
    phase_first = result.get("first_divergence")
    if phase_first != first_projected_divergence:
        raise ProgressError(
            "call-contract phase first divergence disagrees with retail slice order"
        )
    return projections, phase_closure







def _commit_validated_call_contract_slice(
    *,
    args: argparse.Namespace,
    document: ProgressDocument,
    slice_row: Mapping[str, Any],
    result: Mapping[str, Any],
    build_root: Path,
    expected_domains: Mapping[str, int],
) -> tuple[int, dict[str, Any], ProgressDocument]:
    """Commit one already-governed slice result with the direct evidence shape."""

    slice_id = str(slice_row["id"])
    _require_current_call_contract_action(document, requested_slice=slice_id)
    passing = [
        symbol_id
        for symbol_id in result["passing_symbol_ids"]
        if not document.call_contract_body_currentness(symbol_id).get("current")
    ]
    details = {
        "kind": "live-call-contract-advance",
        "status": "passed" if result["passed"] else "diverged",
        "slice_id": slice_id,
        "build_root": display_path(build_root),
        "fresh_build": True,
        "reuse": False,
        "passing_symbol_ids": passing,
        "first_divergence": result["first_divergence"],
        "mutation_planned": bool(passing),
    }
    if not passing:
        details["commit"] = {
            "applied": False,
            "path": args.progress.as_posix(),
            "previous_revision": document.revision,
            "revision": document.revision,
        }
        return (0 if result["passed"] else 1), details, document

    results_by_symbol = {
        str(row["symbol_id"]): deepcopy(dict(row))
        for row in result["body_results"]
        if isinstance(row, Mapping) and str(row.get("symbol_id", "")) in passing
    }

    def transform(data: dict[str, Any]) -> None:
        evidence_ids: dict[str, str] = {}
        for symbol_id in passing:
            body = results_by_symbol[symbol_id]
            symbol = data["symbols"].get(symbol_id, {})
            transcript = [
                deepcopy(row)
                for row in result["exact_fact_transcript"]
                if isinstance(row, Mapping) and row.get("symbol_id") == symbol_id
            ]
            provenance = {
                "symbol_id": symbol_id,
                "address": body["address"],
                "target_id": body["target_id"],
                "physical_block_id": str(symbol.get("physical_block_id", "")),
                "slice_id": slice_id,
                "expected_truth": CALL_CONTRACT_EXPECTED_TRUTH,
                "fresh_build": True,
                "reuse": False,
                "comparison_passed": True,
                "expected_contract": deepcopy(body["expected_contract"]),
                "candidate_contract": deepcopy(body["candidate_contract"]),
                "exact_fact_transcript": transcript,
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
                    "slice_id": slice_id,
                    **current_generations(),
                },
            )
            evidence_ids[symbol_id] = evidence_id
        details["evidence_ids"] = evidence_ids
    proposed, entity_patches, top_level_patches = _call_contract_scoped_patch_plan(
        document,
        transform,
    )
    commit = ProgressSQLiteStore(Path(args.progress)).persist_scoped_changes(
        expected_domain_revisions=dict(expected_domains),
        entity_patches=entity_patches,
        top_level_patches=top_level_patches,
        increment_domains={"semantic", "evidence_generation"},
        apply=bool(args.apply),
    )
    next_document = document
    if commit.applied:
        proposed["revision"] = commit.revision
        next_document = ProgressDocument._from_owned_data(
            proposed,
            path=Path(args.progress),
        )
    return (
        0 if result["passed"] else 1,
        _commit_payload(commit, details),
        next_document,
    )


def _call_contract_replay_payload(
    *,
    mode: str,
    status: str,
    initial_vector: Mapping[str, int],
    final_vector: Mapping[str, int],
    proof_session: Mapping[str, Any],
    slice_results: list[dict[str, Any]],
    first_divergence: Any = None,
    closeout_command: str | None = None,
) -> dict[str, Any]:
    return {
        "schema": _CALL_CONTRACT_REPLAY_SCHEMA,
        "kind": _CALL_CONTRACT_REPLAY_KIND,
        "mode": mode,
        "status": status,
        "slice_count": len(slice_results),
        "passing_body_count": sum(
            int(row.get("passing_body_count", 0)) for row in slice_results
        ),
        "slice_results": slice_results,
        "first_divergence": deepcopy(first_divergence),
        "initial_revision_vector": dict(initial_vector),
        "final_revision_vector": dict(final_vector),
        "proof_session": deepcopy(dict(proof_session)),
        "closeout_command": closeout_command,
    }


def replay_live_call_contract(
    args: argparse.Namespace,
    *,
    status_sink: Any | None = None,
) -> tuple[int, dict[str, Any]]:
    """Run one fresh full-census proof and serially commit slice projections."""

    emit = status_sink or (lambda _message: None)
    progress = Path(args.progress).resolve(strict=True)
    store = ProgressSQLiteStore(progress)
    raw_document, vector = store.materialize_with_revision_vector()
    document = ProgressDocument._from_owned_data(raw_document, path=progress)
    initial_vector = vector.to_dict()
    action = _call_contract_replay_current_action(document, initial_vector)
    mode = "apply" if bool(args.apply) else "dry-run"
    slices = document.authored_call_contract_slices()
    ordered_symbol_ids = [
        str(symbol_id)
        for slice_row in slices
        for symbol_id in slice_row["symbol_ids"]
    ]
    pending_symbol_ids = [
        symbol_id
        for symbol_id in ordered_symbol_ids
        if not document.call_contract_body_currentness(symbol_id).get("current")
    ]
    pending_slice_ids = [
        str(slice_row["id"])
        for slice_row in slices
        if any(
            not document.call_contract_body_currentness(str(symbol_id)).get("current")
            for symbol_id in slice_row["symbol_ids"]
        )
    ]
    base_proof_session = {
        "scope": "full-authored-call-contract-census",
        "original_slice_count": len(slices),
        "body_count": len(ordered_symbol_ids),
        "unique_target_count": len(
            {
                str(target_id)
                for slice_row in slices
                for target_id in slice_row["target_ids"]
            }
        ),
        "pending_slice_count": len(pending_slice_ids),
        "pending_body_count": len(pending_symbol_ids),
        "fresh_build": mode == "apply",
        "reuse": False,
        "candidate_expected_truth": False,
        **current_generations(),
    }
    if action["kind"] == "closeout":
        return 0, _call_contract_replay_payload(
            mode=mode,
            status="closeout-ready",
            initial_vector=initial_vector,
            final_vector=initial_vector,
            proof_session={**base_proof_session, "executed": False},
            slice_results=[],
            closeout_command=str(action["acceptance_command"]),
        )
    current_slice_id = str(action["slice_id"])
    current_index = next(
        (
            index
            for index, slice_row in enumerate(slices)
            if slice_row.get("id") == current_slice_id
        ),
        None,
    )
    if current_index is None:
        raise ProgressError("call-contract replay current slice is absent from the census")
    replay_root = _call_contract_replay_root(
        Path(action["direct_build_root"]),
        create=mode == "apply",
    )
    if mode == "dry-run":
        planned_rows = [
            {
                "slice_id": str(slice_row["id"]),
                "status": (
                    "revalidate-current"
                    if index < current_index
                    else "planned"
                ),
                "body_count": int(slice_row["body_count"]),
                "passing_body_count": 0,
            }
            for index, slice_row in enumerate(slices)
            if index <= current_index or str(slice_row["id"]) in pending_slice_ids
        ]
        return 0, _call_contract_replay_payload(
            mode=mode,
            status="planned",
            initial_vector=initial_vector,
            final_vector=initial_vector,
            proof_session={
                **base_proof_session,
                "executed": False,
                "build_root": display_path(replay_root),
                "direct_scheduler_root_consumed": False,
            },
            slice_results=planned_rows,
        )

    repository_path_inventory = load_repository_path_inventory(REPO_ROOT)
    component_state = _call_contract_replay_component_state(
        repository_path_inventory
    )
    emit(
        "call-contract replay START full-census "
        f"bodies={len(ordered_symbol_ids)} root={display_path(replay_root)}"
    )
    phase_result = live_call_contract_result(
        document=document,
        phase_all_authored_bodies=True,
        build_root=replay_root,
        collect_all_divergences=True,
        compile_definition_closure=True,
        compile_definition_closure_on_divergence=True,
        _repository_path_inventory=repository_path_inventory,
    )
    projections, phase_closure = _project_call_contract_phase_result(
        document,
        phase_result,
        repository_path_inventory=repository_path_inventory,
    )
    _call_contract_replay_require_unchanged(
        component_state,
        dependency_paths=phase_closure.dependency_paths,
        expected_dependency_states=phase_result["dependency_states_after"],
        repository_path_inventory=repository_path_inventory,
    )
    emit(
        "call-contract replay PROOF "
        f"{'PASS' if phase_result.get('passed') else 'DIVERGED'} "
        f"targets={len(phase_result.get('attempted_target_ids', []))}"
    )

    proof_session = {
        **base_proof_session,
        "executed": True,
        "build_root": display_path(replay_root),
        "direct_scheduler_root_consumed": False,
        "passed": phase_result.get("passed") is True,
        "attempted_target_count": len(phase_result["attempted_target_ids"]),
        "compiled_target_count": len(phase_result["compiled_target_ids"]),
        "compiled_definition_count": len(
            phase_result["definition_compile_results"]
        ),
        "binary_ninja_fact_cache": deepcopy(
            phase_result.get("binary_ninja_fact_cache", {})
        ),
        "candidate_cod_index": deepcopy(
            phase_result.get("candidate_cod_index", {})
        ),
        "timings_ms": deepcopy(phase_result.get("timings_ms", {})),
    }

    slice_results: list[dict[str, Any]] = []
    for index in range(current_index):
        projected = projections[index]
        slice_row = projected["slice_row"]
        result = projected["result"]
        current_symbols = all(
            document.call_contract_body_currentness(str(symbol_id)).get("current")
            for symbol_id in slice_row["symbol_ids"]
        )
        summary = {
            "slice_id": str(slice_row["id"]),
            "status": "revalidated" if result["passed"] and current_symbols else "diverged",
            "body_count": int(slice_row["body_count"]),
            "passing_body_count": len(result["passing_symbol_ids"]),
            "first_divergence": result["first_divergence"],
            "committed": False,
        }
        slice_results.append(summary)
        if summary["status"] == "diverged":
            return 1, _call_contract_replay_payload(
                mode=mode,
                status="diverged-before-current-slice",
                initial_vector=initial_vector,
                final_vector=initial_vector,
                proof_session=proof_session,
                slice_results=slice_results,
                first_divergence=result["first_divergence"],
            )

    current_vector = dict(initial_vector)
    for index in range(current_index, len(projections)):
        projected = projections[index]
        slice_row = projected["slice_row"]
        result = projected["result"]
        _call_contract_replay_require_unchanged(
            component_state,
            dependency_paths=phase_closure.dependency_paths,
            expected_dependency_states=phase_result["dependency_states_after"],
            repository_path_inventory=repository_path_inventory,
        )
        expected_current_slice = str(
            document.pipeline("recoil", resolve_order_target=False).get(
                "authored_call_contract_slice_id", ""
            )
        )
        if expected_current_slice != str(slice_row["id"]):
            raise ProgressError(
                "call-contract replay in-memory scheduler did not advance to the next projection"
            )
        commit_args = argparse.Namespace(
            progress=progress,
            apply=True,
        )
        returncode, details, document = _commit_validated_call_contract_slice(
            args=commit_args,
            document=document,
            slice_row=slice_row,
            result=result,
            build_root=replay_root,
            expected_domains={
                "semantic": current_vector["semantic_revision"],
                "evidence_generation": current_vector[
                    "evidence_generation_revision"
                ],
            },
        )
        commit = details.get("commit")
        if not isinstance(commit, Mapping):
            raise ProgressError("call-contract replay slice lacks a commit result")
        if commit.get("applied") is True:
            next_vector = commit.get("revision_vector")
            previous_vector = commit.get("previous_revision_vector")
            if previous_vector != current_vector or not isinstance(next_vector, Mapping):
                raise ProgressError(
                    "call-contract replay slice commit broke the serial revision chain"
                )
            current_vector = {
                key: int(next_vector[key])
                for key in (
                    "transaction_revision",
                    "semantic_revision",
                    "evidence_generation_revision",
                )
            }
        summary = {
            "slice_id": str(slice_row["id"]),
            "status": str(details["status"]),
            "body_count": int(slice_row["body_count"]),
            "passing_body_count": len(details["passing_symbol_ids"]),
            "first_divergence": deepcopy(details["first_divergence"]),
            "committed": commit.get("applied") is True,
            "previous_revision_vector": deepcopy(
                commit.get("previous_revision_vector")
            ),
            "revision_vector": deepcopy(commit.get("revision_vector")),
        }
        slice_results.append(summary)
        emit(
            "call-contract replay "
            f"{'PASS' if returncode == 0 else 'DIVERGED'} "
            f"slice={slice_row['id']} passing={summary['passing_body_count']}"
        )
        if returncode != 0:
            return 1, _call_contract_replay_payload(
                mode=mode,
                status="diverged",
                initial_vector=initial_vector,
                final_vector=current_vector,
                proof_session=proof_session,
                slice_results=slice_results,
                first_divergence=details["first_divergence"],
            )

    final_raw, final_sql_vector = store.materialize_with_revision_vector()
    final_vector = final_sql_vector.to_dict()
    if final_vector != current_vector:
        raise ConcurrentProgressUpdate(
            "call-contract replay tracker changed after the final serial commit"
        )
    final_document = ProgressDocument._from_owned_data(final_raw, path=progress)
    final_action = _call_contract_replay_current_action(final_document, final_vector)
    if final_action["kind"] != "closeout":
        raise ProgressError(
            "call-contract replay completed every projection without reaching close-live"
        )
    return 0, _call_contract_replay_payload(
        mode=mode,
        status="closeout-ready",
        initial_vector=initial_vector,
        final_vector=final_vector,
        proof_session=proof_session,
        slice_results=slice_results,
        closeout_command=str(final_action["acceptance_command"]),
    )


def advance_live_call_contract(args: argparse.Namespace) -> tuple[int, dict[str, Any]]:
    build_root = _absolute_fresh_build_root(args.build_root)
    store = ProgressStore(args.progress)
    document = store.load()
    expected_domains = _precheck_call_contract_revisions(args, document)
    _require_current_call_contract_action(
        document,
        requested_slice=str(args.slice),
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
        "--collect-all-divergences",
        "--json",
    ]
    returncode, raw, stderr = _run_json_process(command)
    result = _validate_call_contract_result(
        raw,
        expected_slice=slice_row,
        expected_source_write_paths=list(closure.source_edit_paths),
        expected_definition_source_paths=list(closure.definition_source_paths),
        expected_compiled_definition_sources=(
            _call_contract_separate_definition_compile_sources(closure)
        ),
        expected_dependency_paths=list(closure.dependency_paths),
    )
    if returncode not in ({0} if result["passed"] else {1}):
        raise ProgressError(f"live call-contract validator exited {returncode}: {stderr}")
    returncode, details, _next_document = _commit_validated_call_contract_slice(
        args=args,
        document=document,
        slice_row=slice_row,
        result=result,
        build_root=build_root,
        expected_domains=expected_domains,
    )
    return returncode, details


def _validate_call_contract_linkability_summary(
    summary: Mapping[str, Any],
    *,
    build_root: Path,
) -> dict[str, Any]:
    """Validate the narrow no-deploy whole-program linkability diagnostic."""

    expected_paths = {
        "build_root": build_root,
        "candidate_path": build_root / "Recoil.exe",
        "map_path": build_root / "Recoil.map",
        "resource_path": build_root / "Recoil.res",
        "config_path": CALL_CONTRACT_FINAL_BUILD_MANIFEST,
    }
    if (
        summary.get("kind") != "final-build-diagnostic"
        or summary.get("diagnostic_kind") != "whole-program-linkability"
        or summary.get("binary") != "recoil"
        or summary.get("success") is not True
        or summary.get("validation_mode") != "live"
        or summary.get("fresh_build") is not True
        or summary.get("reuse") is not False
        or summary.get("dry_run") is not False
        or summary.get("candidate_expected_truth") is not False
        or summary.get("linked_order_evaluation_suppressed") is not True
        or summary.get("playtest_deployment_suppressed") is not True
        or summary.get("required_order_targets") != []
        or summary.get("effective_order_targets") != []
        or summary.get("diagnostic_only") is not True
        or summary.get("final_image_validation") != "not-run"
        or summary.get("compiler_profile") != "VC5SP3"
        or not isinstance(summary.get("canonical_mfc_include_trace"), Mapping)
        or summary.get("canonical_mfc_include_trace", {}).get("ok") is not True
        or any(
            summary.get(field) is not True
            for field in (
                "compile_succeeded",
                "coff_alias_sources_succeeded",
                "resource_succeeded",
                "link_succeeded",
                "candidate_available",
            )
        )
        or any(
            summary.get(field) is not False
            for field in (
                "authored_byte_eligible",
                "linked_order_passed",
                "accepts_linked_order",
                "accepts_bytes",
                "accepts_final_image",
            )
        )
    ):
        raise ProgressError(
            "call-contract whole-program linkability summary has an invalid "
            "diagnostic or acceptance boundary"
        )

    for field, expected_path in expected_paths.items():
        value = summary.get(field)
        if (
            not isinstance(value, str)
            or not value
            or Path(value).resolve() != expected_path.resolve()
        ):
            raise ProgressError(
                "call-contract whole-program linkability summary has an invalid "
                f"{field}"
            )
    for field in ("candidate_path", "map_path", "resource_path"):
        if not expected_paths[field].is_file():
            raise ProgressError(
                "call-contract whole-program linkability output is missing: "
                f"{display_path(expected_paths[field])}"
            )

    deployment = summary.get("playtest_deploy")
    expected_playtest = REPO_ROOT / "playground" / "Recoil-rebuild.exe"
    if (
        not isinstance(deployment, Mapping)
        or deployment.get("attempted") is not False
        or deployment.get("updated") is not False
        or deployment.get("error") is not None
        or deployment.get("suppression_reason")
        != "whole-program-linkability"
        or not isinstance(deployment.get("destination"), str)
        or Path(str(deployment.get("destination"))).resolve()
        != expected_playtest.resolve()
    ):
        raise ProgressError(
            "call-contract whole-program linkability did not prove play-test "
            "deployment suppression"
        )

    return {
        "schema": CALL_CONTRACT_LINKABILITY_SCHEMA,
        "validation_mode": "live",
        "fresh_build": True,
        "reuse": False,
        "whole_program_linked": True,
        "playtest_deployment_suppressed": True,
        "candidate_expected_truth": False,
        "accepts_linked_order": False,
        "accepts_bytes": False,
        "accepts_final_image": False,
        "build_root": display_path(build_root),
        "summary_path": display_path(build_root / "summary.json"),
    }


def _run_call_contract_linkability_gate(
    *,
    build_root: Path,
    progress_path: Path,
) -> dict[str, Any]:
    """Run one fresh canonical full link without order or play-test effects."""

    linkability_root = build_root / "whole-program-linkability"
    if linkability_root.exists():
        raise ProgressError(
            "call-contract whole-program linkability root must be fresh: "
            f"{display_path(linkability_root)}"
        )
    command = [
        sys.executable,
        str(REPO_ROOT / "tools" / "recoil.py"),
        "verify",
        "final-build",
        "--manifest",
        display_path(CALL_CONTRACT_FINAL_BUILD_MANIFEST),
        "--progress",
        str(progress_path.resolve(strict=True)),
        "--build-dir",
        display_path(linkability_root),
        "--clean",
        "--order-scope",
        "authored",
        "--linkability-only",
    ]
    completed = subprocess.run(
        command,
        cwd=REPO_ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    summary_path = linkability_root / "summary.json"
    if not summary_path.is_file():
        detail = completed.stderr.strip() or completed.stdout.strip()[-1600:]
        raise ProgressError(
            "call-contract whole-program linkability produced no summary; "
            f"exit_code={completed.returncode}"
            + (f": {detail}" if detail else "")
        )
    try:
        summary = json.loads(summary_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ProgressError(
            f"cannot read call-contract whole-program linkability summary: {exc}"
        ) from exc
    if not isinstance(summary, Mapping):
        raise ProgressError(
            "call-contract whole-program linkability summary must be an object"
        )
    if completed.returncode != 0:
        delegated = _delegated_vc5_build_failure(summary)
        detail = (
            delegated
            or completed.stderr.strip()
            or completed.stdout.strip()[-1600:]
        )
        raise ProgressError(
            "call-contract whole-program linkability failed; "
            f"exit_code={completed.returncode}"
            + (f": {detail}" if detail else "")
        )
    return _validate_call_contract_linkability_summary(
        summary,
        build_root=linkability_root,
    )


def close_live_call_contract(args: argparse.Namespace) -> dict[str, Any]:
    """Run the fresh census and one no-deploy full-link closeout gate."""

    build_root = _absolute_fresh_build_root(args.build_root)
    store = ProgressStore(args.progress)
    document = store.load()
    expected_domains = _precheck_call_contract_revisions(args, document)
    _require_current_call_contract_action(document, requested_slice=None)
    slices = document.authored_call_contract_slices()
    ordered_symbol_ids = [
        str(symbol_id)
        for slice_row in slices
        for symbol_id in slice_row["symbol_ids"]
    ]
    incomplete = [
        symbol_id
        for symbol_id in ordered_symbol_ids
        if not document.call_contract_body_currentness(symbol_id).get("current")
    ]
    if incomplete:
        raise ProgressError(
            "call-contract closeout requires every body to be current; first pending: "
            + ", ".join(incomplete[:8])
        )

    scan_rows: list[dict[str, Any]] = []
    for index, slice_row in enumerate(slices, start=1):
        slice_id = str(slice_row["id"])
        slice_root = build_root / f"slice-{index:02d}"
        closure = call_contract_source_closure(document, slice_row)
        command = [
            sys.executable,
            str(REPO_ROOT / "tools" / "recoil.py"),
            "verify",
            "call-contract",
            "--slice",
            slice_id,
            "--progress",
            str(Path(args.progress).resolve(strict=True)),
            "--build-root",
            display_path(slice_root),
            "--collect-all-divergences",
            "--json",
        ]
        returncode, raw, stderr = _run_json_process(command)
        result = _validate_call_contract_result(
            raw,
            expected_slice=slice_row,
            expected_source_write_paths=list(closure.source_edit_paths),
            expected_definition_source_paths=list(closure.definition_source_paths),
            expected_compiled_definition_sources=(
                _call_contract_separate_definition_compile_sources(closure)
            ),
            expected_dependency_paths=list(closure.dependency_paths),
        )
        if returncode != 0 or not result["passed"]:
            raise ProgressError(
                f"call-contract closeout diverged in {slice_id}: "
                f"{result['first_divergence'] or stderr}"
            )
        if result["passing_symbol_ids"] != list(slice_row["symbol_ids"]):
            raise ProgressError(
                f"call-contract closeout did not pass every body in {slice_id}"
            )
        scan_rows.append(
            {
                "slice_id": slice_id,
                "body_count": int(slice_row["body_count"]),
                "build_root": display_path(slice_root),
                "exact_fact_transcript": deepcopy(
                    result["exact_fact_transcript"]
                ),
            }
        )

    linkability = _run_call_contract_linkability_gate(
        build_root=build_root,
        progress_path=Path(args.progress),
    )
    closeout = {
        "schema": CALL_CONTRACT_CLOSEOUT_SCHEMA,
        "ordered_symbol_ids": ordered_symbol_ids,
        "slice_ids": [str(row["id"]) for row in slices],
        "complete_no_reuse_zero_divergence": True,
        "fresh_build": True,
        "reuse": False,
        "candidate_expected_truth": False,
        "scan_rows": scan_rows,
        "whole_program_linkability": deepcopy(linkability),
        **current_generations(),
    }
    details = {
        "kind": "live-call-contract-closeout",
        "status": "passed",
        "body_count": len(ordered_symbol_ids),
        "slice_count": len(slices),
        "build_root": display_path(build_root),
        "fresh_build": True,
        "reuse": False,
        "whole_program_linkability": deepcopy(linkability),
        "mutation_planned": True,
    }

    def transform(data: dict[str, Any]) -> None:
        current = ProgressDocument(data, path=args.progress)
        current_ordered = [
            str(symbol_id)
            for row in current.authored_call_contract_slices()
            for symbol_id in row["symbol_ids"]
        ]
        if current_ordered != ordered_symbol_ids:
            raise ProgressError(
                "call-contract census changed before closeout CAS"
            )
        if any(
            not current.call_contract_body_currentness(symbol_id).get("current")
            for symbol_id in current_ordered
        ):
            raise ProgressError(
                "call-contract body currentness changed before closeout CAS"
            )
        migration = data.setdefault("migration", {})
        if not isinstance(migration, dict):
            raise ProgressError("tracker migration metadata must be an object")
        migration["authored_call_contract_fresh_closeout_v4"] = deepcopy(closeout)

    commit = _call_contract_scoped_patch_commit(
        args=args,
        document=document,
        transform=transform,
        expected_domains=expected_domains,
        increment_domains={"semantic", "evidence_generation"},
    )
    return _commit_payload(commit, details)


def _direct_compile_host_policy_text(source_from: str) -> str:
    source_path = _resolve_progress_repository_file(
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
        vc5_manifest_dir=REPO_ROOT / "tools" / "vc5_verify_targets",
    )
    repository_inventory = _load_progress_repository_inventory()
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
        repository_manifest = _resolve_progress_repository_file(
            manifest_value,
            context=f"verification target {target_id!r} registered manifest",
            inventory=repository_inventory,
        )
        if not repository_manifest.repository_path.startswith("tools/vc5_verify_targets/"):
            raise ProgressError(
                f"target {target_id!r} manifest is outside tools/vc5_verify_targets"
            )
        manifest_path = repository_manifest.physical_path
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
                    # ``order_edit_paths`` is the reviewed direct-edit
                    # closure. It controls editable scope, not target
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
    if payload["reviewed"] is not True:
        raise ProgressError(
            "logical alias group payload requires reviewed=true"
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
    if payload["reviewed"] is not True:
        raise ProgressError(
            "function-padding correction requires reviewed=true"
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
    if payload.get("reviewed") is not True:
        raise ProgressError(
            "authored non-gating block acceptance requires reviewed=true"
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
    if payload.get("reviewed") is not True:
        raise ProgressError(
            "provider block reclassification requires reviewed=true"
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
    del document, block_id
    return []


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
    del document, block_ids
    return []


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
_OWNER_DOWNGRADE_FIELDS = {
    "schema",
    "reviewed",
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
    if payload["reviewed"] is not True:
        raise ProgressError(
            "owner downgrade requires reviewed=true"
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
    if payload["reviewed"] is not True:
        raise ProgressError(
            "owner replacement batch requires reviewed=true"
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
    if payload["reviewed"] is not True:
        raise ProgressError(
            "physical-block replacement requires reviewed=true"
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
        for collection_name in (*_BLOCK_RELATIONSHIP_COLLECTIONS, "binaries"):
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
        if args.command in {
            "advance-live-authored-byte",
            "advance-live-linked-byte",
        }:
            code, payload = advance_live_byte(args)
            _print_json(payload)
            return code
        if args.command == "advance-live-call-contract":
            code, payload = advance_live_call_contract(args)
            _print_json(payload)
            return code
        if args.command == "call-contract" and args.call_contract_command == "close-live":
            _print_json(close_live_call_contract(args))
            return 0
        document = _load(args.progress)
        if args.command == "next":
            payload = document.current_task(args.binary)
            _print_json(payload) if args.json else _print_pipeline(payload)
            return 0
        if args.command == "status":
            payload = document.show(args.selector) if args.selector else document.current_task(args.binary)
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
        if args.command == "audit":
            findings = document.audit(args.scope)
            payload = {
                "scope": args.scope,
                "passed": not any(item.severity == "error" for item in findings),
                "findings": [item.to_dict() for item in findings],
                **document.revision_identity(),
            }
            _print_json(payload) if args.json else print(
                "progress audit OK" if payload["passed"] else f"progress audit: {len(findings)} finding(s)"
            )
            return 0 if payload["passed"] else 1
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
            _print_json(document.with_revision_vector(_resolve_collection_row(document, collection, args.selector)))
            return 0
        if args.command == "owner":
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
                **document.revision_identity(),
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
