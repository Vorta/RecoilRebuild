from __future__ import annotations

import argparse
import contextlib
import io
import json
import os
from pathlib import Path
import threading
from types import ModuleType
from typing import Any, Iterable, Mapping

from _recoil.commands.workflow_contract_audit import (
    CommandContract,
    EXPECTED_CALL_CONTRACT_ACCEPTANCE_POLICY,
    REQUIRED_COMMANDS as WORKFLOW_REQUIRED_COMMANDS,
    _named_call_count,
    audit_generated_call_contract_commands,
    registry_view,
)
from _recoil.lib.progress import ProgressDocument, ProgressError
from _recoil.lib.repository_paths import (
    RepositoryPathError,
    validate_repository_relative_path,
)
from _recoil.lib.tooling import REPO_ROOT, configure_stdio
from _recoil.lib.windows_identity import StableReadHandle, physical_identity, require_same_physical_object
from _recoil.lib.worktree_control import (
    WorktreeControlError,
    _reject_reparse,
    reauthenticate_canonical_control_root,
    resolve_canonical_control_root,
)


DEFAULT_MANIFEST_DIR = REPO_ROOT / "tools" / "vc5_verify_targets"
_UNSET = object()
_AUTHENTICATED_INPUTS_TOKEN = object()
ORDER_RECONSTRUCTION_BLOCKER_CODES = frozenset({"order-target-role-gate-blocked"})
_VC5_TRACKER_CACHE_NAMES = (
    "canonical_tracker_artifact_index",
    "canonical_tracker_data",
    "registered_vc5_manifest_paths",
    "canonical_tracker_function_metadata",
)
_VC5_TRACKER_BINDING_LOCK = threading.RLock()
_CALL_CONTRACT_INCLUDE_ROOT_BINDING_LOCK = threading.RLock()


@contextlib.contextmanager
def _bound_vc5_tracker(tracker: Path):
    """Bind verifier-deep tracker reads to one authenticated invocation path."""

    from _recoil.commands import vc5_verify

    tracker_path = tracker.resolve(strict=True)
    cache_functions = tuple(
        getattr(vc5_verify, name) for name in _VC5_TRACKER_CACHE_NAMES
    )
    with _VC5_TRACKER_BINDING_LOCK:
        original_tracker = vc5_verify.DEFAULT_PROGRESS_PATH
        for function in cache_functions:
            function.cache_clear()
        vc5_verify.DEFAULT_PROGRESS_PATH = tracker_path
        try:
            yield
        finally:
            try:
                for function in cache_functions:
                    function.cache_clear()
            finally:
                vc5_verify.DEFAULT_PROGRESS_PATH = original_tracker


def _routed_call_contract_include_roots(
    *,
    execution_root: Path,
    canonical_root: Path,
) -> tuple[tuple[tuple[str, Path], ...], tuple[tuple[Path, Any], ...]]:
    """Keep tracked roots linked while routing ignored SDK roots canonically."""

    from _recoil.commands import call_contract_verify

    execution = execution_root.resolve(strict=True)
    canonical = canonical_root.resolve(strict=True)
    config_path = call_contract_verify.DEFAULT_FINAL_BUILD_MANIFEST.resolve(strict=True)
    try:
        config_path.relative_to(execution)
    except ValueError as exc:
        raise ProgressError(
            "call-contract final-build include-root contract is not from the "
            f"executing worktree: {config_path}"
        ) from exc
    try:
        config = json.loads(config_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ProgressError(
            "call-contract include closure cannot read the executing-worktree "
            f"final-build contract: {exc}"
        ) from exc
    raw_include_dirs = config.get("include_dirs")
    if not isinstance(raw_include_dirs, list):
        raise ProgressError("call-contract final-build include_dirs must be a list")

    roots: list[tuple[str, Path]] = []
    machine_local_identities: list[tuple[Path, Any]] = []
    seen: set[str] = set()
    for index, raw_value in enumerate(raw_include_dirs):
        if not isinstance(raw_value, str) or not raw_value:
            raise ProgressError(
                "call-contract final-build include_dirs entries must be "
                f"non-empty strings: index {index}"
            )
        include_dir = Path(raw_value)
        if include_dir.is_absolute():
            try:
                include_dir.resolve().relative_to(execution)
            except ValueError:
                # External compiler/SDK roots are not repository dependencies.
                continue
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

        execution_candidate = execution.joinpath(*logical_root.split("/"))
        candidate = execution_candidate
        machine_local = False
        if logical_root.startswith("support/sdk/"):
            if execution != canonical and os.path.lexists(execution_candidate):
                raise ProgressError(
                    "call-contract linked execution worktree unexpectedly contains "
                    f"machine-local SDK input: {logical_root}"
                )
            candidate = canonical.joinpath(*logical_root.split("/"))
            machine_local = True
        elif not execution_candidate.is_dir():
            raise ProgressError(
                "call-contract repository include root does not exist or is "
                f"not a directory: {raw_value}"
            )
        try:
            _reject_reparse(
                candidate,
                context=f"call-contract include root {logical_root}",
            )
        except WorktreeControlError as exc:
            raise ProgressError(str(exc)) from exc
        if not candidate.is_dir():
            raise ProgressError(
                "call-contract canonical machine-local include root does not "
                f"exist or is not a directory: {logical_root}"
            )
        physical = candidate.resolve(strict=True)
        expected_parent = canonical if machine_local else execution
        try:
            physical.relative_to(expected_parent)
        except ValueError as exc:
            raise ProgressError(
                f"call-contract include root escapes its authenticated root: {logical_root}"
            ) from exc
        key = str(physical).casefold()
        if key in seen:
            continue
        seen.add(key)
        roots.append((logical_root, physical))
        if machine_local:
            machine_local_identities.append(
                (physical, physical_identity(physical, directory=True))
            )
    return tuple(roots), tuple(machine_local_identities)


@contextlib.contextmanager
def _bound_call_contract_include_roots(
    *,
    execution_root: Path,
    canonical_root: Path,
):
    """Temporarily supply linked tracked and canonical machine-local roots."""

    from _recoil.commands import call_contract_verify

    roots, identities = _routed_call_contract_include_roots(
        execution_root=execution_root,
        canonical_root=canonical_root,
    )
    with _CALL_CONTRACT_INCLUDE_ROOT_BINDING_LOCK:
        original = call_contract_verify._call_contract_repository_include_roots
        call_contract_verify._call_contract_repository_include_roots = lambda: roots
        try:
            yield
        finally:
            call_contract_verify._call_contract_repository_include_roots = original
            for path, expected in identities:
                observed = physical_identity(path, directory=True)
                require_same_physical_object(
                    expected,
                    observed,
                    context=f"call-contract canonical include root {path}",
                )


_PIPELINE_LOCAL_COMMANDS: dict[tuple[str, ...], CommandContract] = {
    ("audit", "pipeline-reachability"): CommandContract(
        "pipeline_reachability_audit", (), False
    ),
    ("audit", "call-contract-readiness"): CommandContract(
        "call_contract_readiness_audit", (), False
    ),
    ("audit", "relocation-expectations"): CommandContract(
        "relocation_expectations", (), False
    ),
    ("audit", "final-image-catalog"): CommandContract(
        "final_image_catalog_audit", (), False
    ),
    ("verify", "final-image"): CommandContract("live_final_verify", (), True),
}

_PIPELINE_REQUIRED_PATHS = (
    ("audit", "pipeline-reachability"),
    ("verify", "vc5-order"),
    ("progress", "advance-live-order"),
    ("verify", "authored-object-byte"),
    ("verify", "authored-byte"),
    ("verify", "linked-byte"),
    ("progress", "advance-live-byte"),
    ("verify", "call-contract"),
    ("audit", "call-contract-readiness"),
    ("progress", "advance-live-call-contract"),
    ("progress", "call-contract", "prepare-live-convergence"),
    ("progress", "call-contract", "prepare-repair-continuation"),
    ("audit", "relocation-expectations"),
    ("progress", "relocation-exception", "set"),
    ("audit", "final-image-catalog"),
    ("verify", "final-image"),
)

# The workflow contract audit owns public command metadata shared across
# operational audits. Pipeline reachability owns only its additional producer
# routes and reuses the exact authoritative objects for every overlap.
REQUIRED_COMMANDS: dict[tuple[str, ...], CommandContract] = {
    path: (
        WORKFLOW_REQUIRED_COMMANDS[path]
        if path in WORKFLOW_REQUIRED_COMMANDS
        else _PIPELINE_LOCAL_COMMANDS[path]
    )
    for path in _PIPELINE_REQUIRED_PATHS
}


def _finding(check: str, message: str) -> dict[str, str]:
    return {"check": check, "message": message}


def _audit_registry(view: Mapping[tuple[str, ...], CommandContract]) -> list[dict[str, str]]:
    failures: list[dict[str, str]] = []
    for path, expected in REQUIRED_COMMANDS.items():
        actual = view.get(path)
        label = " ".join(path)
        if actual is None:
            failures.append(_finding("producer-registry", f"missing public command: {label}"))
        elif actual != expected:
            failures.append(
                _finding(
                    "producer-registry",
                    f"{label} contract is {actual!r}, expected {expected!r}",
                )
            )
    return failures


def _probe_current_order(
    document: ProgressDocument,
    progress_module: ModuleType,
) -> dict[str, Any]:
    pipeline = document.next_work("recoil")
    phase = str(pipeline.get("phase", ""))
    if phase not in progress_module.ORDER_PHASES:
        return {
            "reachable": True,
            "status": "not-applicable",
            "phase": phase,
            "reason": "the current primary scheduler phase is not an order phase",
        }
    cursor_block_id = str(pipeline.get("physical_block_id", ""))
    block = document.collection("physical_blocks").get(cursor_block_id)
    if not isinstance(block, Mapping):
        raise RuntimeError(f"current order cursor lacks physical block {cursor_block_id!r}")
    primary = pipeline.get("cursor_launchability", {}).get("primary", {})
    next_command = str(pipeline.get("next_command") or "")
    launch_plan = pipeline.get("launch_plan")
    if (
        not isinstance(launch_plan, list)
        or not launch_plan
        or not isinstance(launch_plan[0], Mapping)
        or launch_plan[0].get("lane") != "primary"
    ):
        raise RuntimeError("current order producer has a malformed primary launch-plan row")
    actions = launch_plan[0].get("actions")
    if not isinstance(actions, list):
        raise RuntimeError("current order producer primary actions must be a list")
    if not isinstance(primary, Mapping) or primary.get("launchability") != "launchable":
        resolution = pipeline.get("order_target_resolution", {})
        reason_code = str(primary.get("reason_code") or "") if isinstance(primary, Mapping) else ""
        if (
            isinstance(primary, Mapping)
            and primary.get("launchability") == "blocked"
            and reason_code in ORDER_RECONSTRUCTION_BLOCKER_CODES
            and isinstance(resolution, Mapping)
            and resolution.get("status") == "blocked"
            and resolution.get("reason_code") == reason_code
        ):
            if next_command or actions:
                raise RuntimeError(
                    "blocked primary order cursor exposes an executable acceptance command"
                )
            blocker = resolution.get("blocker")
            return {
                "reachable": True,
                "status": "blocked",
                "phase": phase,
                "cursor_block_id": cursor_block_id,
                "reason_code": reason_code,
                "reason": str(primary.get("reason") or resolution.get("reason") or ""),
                "blocker": dict(blocker) if isinstance(blocker, Mapping) else blocker,
                "next_command": next_command,
                "launchable_with_command": False,
            }
        raise RuntimeError(
            str(primary.get("reason") if isinstance(primary, Mapping) else "current order cursor is blocked")
        )
    if not next_command or not isinstance(actions, list) or not actions:
        raise RuntimeError("current primary order cursor is launchable without an executable command")
    resolution = pipeline.get("order_target_resolution", {})
    if not isinstance(resolution, Mapping) or resolution.get("status") != "ready":
        raise RuntimeError("current primary order cursor lacks a ready typed target resolution")
    target_id = str(resolution.get("target_id") or "")
    contract = progress_module._current_order_contract(
        document,
        target_id,
        override_selector=str(resolution.get("override_selector") or "") or None,
    )
    covered = contract.get("covered_block_ids")
    identities_by_block = contract.get("identities_by_block")
    if not isinstance(covered, list) or not covered:
        raise RuntimeError("current order contract has no covered physical blocks")
    if covered[0] != cursor_block_id:
        raise RuntimeError("current order contract does not begin at the scheduler cursor")
    if not isinstance(identities_by_block, Mapping):
        raise RuntimeError("current order contract lacks identities_by_block")
    return {
        "reachable": True,
        "status": "ready",
        "phase": phase,
        "cursor_block_id": cursor_block_id,
        "target_id": target_id,
        "covered_block_ids": list(covered),
        "covered_block_count": len(covered),
        "complete_contiguous_slices": True,
        "expected_identity_count": len(contract.get("identities", [])),
        "next_command": next_command,
        "launchable_with_command": True,
    }


def _probe_byte_lanes(progress_module: ModuleType) -> dict[str, Any]:
    routes = {
        "object": "authored-object-byte",
        "authored": "authored-byte",
        "linked": "linked-byte",
    }
    parsed: dict[str, str] = {}
    for lane in routes:
        args = progress_module._parser().parse_args(
            [
                "advance-live-byte",
                "--lane",
                lane,
                "--build-root",
                "build/audit-fixture",
                "--expected-revision",
                "1",
                "--apply",
            ]
        )
        if args.command != "advance-live-byte" or args.lane != lane:
            raise RuntimeError(f"advance-live-byte parser does not dispatch lane {lane}")
        parsed[lane] = routes[lane]
    return {
        "reachable": True,
        "status": "ready",
        "advance_command": "progress advance-live-byte",
        "verify_commands": parsed,
    }


def _probe_call_contract(
    progress_module: ModuleType,
    *,
    repository_root: Path = REPO_ROOT,
) -> dict[str, Any]:
    from _recoil.commands.call_contract_verify import _call_contract_body_results
    import inspect
    from _recoil.lib.call_contract_generations import (
        current_generations,
        required_call_contract_verifier_component_findings,
    )

    args = progress_module._parser().parse_args([
        "advance-live-call-contract", "--slice",
        "recoil:call-contract-slice:0x401000-0x401100", "--packet-id",
        "recoil:explicit-work:audit-call-contract", "--build-root",
        "build/audit-fixture-call-contract", "--expected-semantic-revision",
        "11", "--expected-evidence-generation-revision", "13", "--apply",
    ])
    with contextlib.redirect_stderr(io.StringIO()):
        continuation = progress_module._parser().parse_args([
            "call-contract", "prepare-repair-continuation", "--producer-packet",
            "recoil:work:call-contract:producer-fixture", "--returned-work-item",
            "recoil:work:call-contract:audit-fixture", "--build-root", "build/audit-repair-continuation",
            "--expected-revision", "1", "--dry-run",
        ])
    main_source = __import__("inspect").getsource(progress_module.main)
    return {
        "reachable": (
            args.command == "advance-live-call-contract"
            and args.slice == "recoil:call-contract-slice:0x401000-0x401100"
        ),
        "status": "ready",
        "advance_command": "progress advance-live-call-contract",
        "verify_command": "verify call-contract",
        "acceptance_enabled": getattr(
            progress_module, "CALL_CONTRACT_VERIFICATION_ACCEPTANCE_ENABLED", None
        ),
        "candidate_expected_truth": False,
        "acceptance_policy": dict(
            progress_module.CALL_CONTRACT_VERIFICATION_ACCEPTANCE_POLICY
        ),
        "body_result_source": inspect.getsource(_call_contract_body_results),
        "generations": current_generations(),
        "required_component_findings": (
            required_call_contract_verifier_component_findings(repository_root)
        ),
        "direct_route": {
            "direct_body_results": True,
            "translation_unit_context": False,
            "parent_fresh_direct_retail": True,
            "worker_results_accepting": False,
            "explicit_invalidation": True,
            "semantic_revision_guard": args.expected_semantic_revision == 11,
            "evidence_generation_revision_guard": (
                args.expected_evidence_generation_revision == 13
            ),
            "transaction_revision_guard_absent": (
                getattr(args, "expected_revision", None) is None
            ),
            "packet_reservation_guard": (
                args.packet_id == "recoil:explicit-work:audit-call-contract"
            ),
        },
        "repair_continuation": {
            "reachable": True,
            "command": "progress call-contract prepare-repair-continuation",
            "parent_only": (
                continuation.returned_work_item
                == "recoil:work:call-contract:audit-fixture"
                and continuation.producer_packet
                == "recoil:work:call-contract:producer-fixture"
            ),
            "producer_bound": (
                "prepare_call_contract_repair_continuation" in main_source
            ),
            "nonaccepting": True,
            "acceptance_eligible": False,
        },
    }


def _probe_call_contract_readiness(
    *,
    tracker: Path,
    execution_root: Path,
    canonical_root: Path | None,
) -> dict[str, Any]:
    from _recoil.commands.call_contract_readiness_audit import (
        audit_call_contract_readiness,
    )

    include_binding = (
        _bound_call_contract_include_roots(
            execution_root=execution_root,
            canonical_root=canonical_root,
        )
        if canonical_root is not None
        else contextlib.nullcontext()
    )
    with include_binding:
        return audit_call_contract_readiness(
            tracker=tracker,
            all_slices=True,
        )


def _authored_cursor(document: ProgressDocument) -> str:
    pipeline = document.pipeline("recoil")
    lane = pipeline.get("authored_byte_lane")
    if isinstance(lane, Mapping) and isinstance(lane.get("cursor"), str) and lane["cursor"]:
        return lane["cursor"]
    cursor = pipeline.get("authored_byte_cursor")
    if isinstance(cursor, str) and cursor:
        return cursor
    groups = document._physical_groups("recoil", gating_only=True)
    if groups and isinstance(groups[0].get("address"), str):
        return str(groups[0]["address"])
    raise RuntimeError("authored relocation producer has no addressable gating group")


def _probe_relocations(
    document: ProgressDocument,
    *,
    reference: Path,
    manifest_dir: Path,
) -> dict[str, Any]:
    from _recoil.commands import relocation_expectations

    report = relocation_expectations.audit_at(
        document=document,
        at=_authored_cursor(document),
        reference=reference,
        manifest_dir=manifest_dir,
    )
    from _recoil.commands import relocation_expectation_mutation

    parsed = relocation_expectation_mutation.build_parser().parse_args(
        [
            "set",
            "--source-symbol-id",
            "recoil:function:0x401000",
            "--source-address",
            "0x401000",
            "--payload-json",
            "{}",
            "--expected-revision",
            "1",
            "--dry-run",
        ]
    )
    report["reviewed_exception_route"] = {
        "reachable": parsed.command == "set",
        "revision_guarded": parsed.expected_revision == 1,
        "dry_run_available": parsed.dry_run is True,
    }
    return report


def _probe_final_catalog(*, tracker: Path, reference: Path) -> dict[str, Any]:
    from _recoil.commands.final_image_catalog_audit import audit_catalog

    return audit_catalog(tracker=tracker, reference=reference)


def _probe_final_verify(*, tracker: Path, reference: Path) -> dict[str, Any]:
    from _recoil.commands import live_final_verify

    try:
        coverage, _document = live_final_verify._load_catalog(
            tracker,
            reference=reference,
        )
    except live_final_verify.LiveFinalError as exc:
        message = str(exc)
        if "live typed final-image coverage is incomplete" not in message:
            raise
        return {
            "reachable": True,
            "status": "blocked-before-build",
            "uses_live_coverage": True,
            "build_started": False,
            "blocker": message,
        }
    if coverage.get("kind") != "live-final-image-coverage":
        raise RuntimeError("final verifier loader did not return live final-image coverage")
    return {
        "reachable": True,
        "status": "ready",
        "uses_live_coverage": True,
        "build_started": False,
        "coverage_kind": coverage.get("kind"),
    }


def _validate_order(report: Any) -> list[dict[str, str]]:
    if not isinstance(report, Mapping):
        return [_finding("order-producer", "current order producer returned no object")]
    if report.get("reachable") is not True:
        return [_finding("order-producer", "current order producer is not reachable")]
    if report.get("status") == "not-applicable":
        return []
    if report.get("status") == "blocked":
        failures: list[dict[str, str]] = []
        reason_code = str(report.get("reason_code") or "")
        if reason_code not in ORDER_RECONSTRUCTION_BLOCKER_CODES:
            failures.append(
                _finding(
                    "order-producer",
                    f"current order producer has unsupported blocked reason {reason_code!r}",
                )
            )
        blocker = report.get("blocker")
        if not isinstance(blocker, Mapping):
            failures.append(
                _finding("order-producer", "blocked order producer lacks a typed blocker")
            )
        else:
            address = blocker.get("address")
            identity = blocker.get("identity")
            problems = blocker.get("problems")
            if (
                blocker.get("kind") != "order-target-role-gate"
                or not isinstance(blocker.get("target_id"), str)
                or not blocker.get("target_id")
                or blocker.get("phase") != report.get("phase")
                or not isinstance(address, str)
                or not address.startswith("0x")
                or not isinstance(identity, str)
                or not identity
                or not isinstance(blocker.get("label"), str)
                or not blocker.get("label")
                or not isinstance(problems, list)
                or not problems
                or any(not isinstance(problem, str) or not problem for problem in problems)
            ):
                failures.append(
                    _finding("order-producer", "blocked order producer has a malformed typed blocker")
                )
            reason = str(report.get("reason") or "")
            if isinstance(address, str) and isinstance(identity, str) and (
                address not in reason or identity not in reason
            ):
                failures.append(
                    _finding(
                        "order-producer",
                        "blocked order producer did not preserve its address/identity reason",
                    )
                )
        if report.get("launchable_with_command") is not False or report.get("next_command"):
            failures.append(
                _finding(
                    "order-producer",
                    "blocked order producer exposes an executable live acceptance command",
                )
            )
        return failures
    if report.get("status") != "ready":
        return [
            _finding(
                "order-producer",
                f"current order producer has unsupported status {report.get('status')!r}",
            )
        ]
    if (
        report.get("complete_contiguous_slices") is not True
        or not isinstance(report.get("covered_block_ids"), list)
        or not report["covered_block_ids"]
    ):
        return [
            _finding(
                "order-producer",
                "current order target does not resolve to complete contiguous covered blocks",
            )
        ]
    if report.get("launchable_with_command") is not True or not report.get("next_command"):
        return [
            _finding(
                "order-producer",
                "current order target is marked ready without an executable live command",
            )
        ]
    return []


def _validate_byte(report: Any) -> list[dict[str, str]]:
    expected = {
        "object": "authored-object-byte",
        "authored": "authored-byte",
        "linked": "linked-byte",
    }
    if not isinstance(report, Mapping) or report.get("reachable") is not True:
        return [_finding("byte-producer", "live byte lane producer is not reachable")]
    if report.get("verify_commands") != expected:
        return [_finding("byte-producer", "object/authored/linked verifier mapping is malformed")]
    return []


def _validate_call_contract(report: Any) -> list[dict[str, str]]:
    if not isinstance(report, Mapping) or report.get("reachable") is not True:
        return [_finding("call-contract-producer", "call-contract producer is not reachable")]
    failures: list[dict[str, str]] = []
    direct_route = report.get("direct_route")
    acceptance_policy = report.get("acceptance_policy")
    body_result_source = report.get("body_result_source")
    generations = report.get("generations")
    component_findings = report.get("required_component_findings")
    if (
        report.get("status") != "ready"
        or report.get("acceptance_enabled") is not True
        or report.get("verify_command") != "verify call-contract"
        or report.get("advance_command") != "progress advance-live-call-contract"
        or report.get("candidate_expected_truth") is not False
    ):
        failures.append(_finding(
            "call-contract-producer",
            "fresh direct call-contract acceptance is not reachable",
        ))
    if not isinstance(component_findings, list) or component_findings:
        failures.append(
            _finding(
                "call-contract-required-component",
                "registered verifier component is missing, unreadable, or unparseable: "
                + json.dumps(component_findings, sort_keys=True),
            )
        )
    if (
        not isinstance(acceptance_policy, Mapping)
        or any(
            acceptance_policy.get(key) != value
            for key, value in EXPECTED_CALL_CONTRACT_ACCEPTANCE_POLICY.items()
        )
        or not isinstance(body_result_source, str)
        or any(
            token not in body_result_source
            for token in (
                '"expected_fact_row"', '"expected_contract"',
                '"candidate_contract"', '"comparison_passed"',
                "current_generations()",
            )
        )
        or "bytes_hex" in body_result_source
        or not isinstance(direct_route, Mapping)
        or direct_route.get("direct_body_results") is not True
        or direct_route.get("translation_unit_context") is not False
        or direct_route.get("parent_fresh_direct_retail") is not True
        or direct_route.get("worker_results_accepting") is not False
        or direct_route.get("explicit_invalidation") is not True
        or direct_route.get("semantic_revision_guard") is not True
        or direct_route.get("evidence_generation_revision_guard") is not True
        or direct_route.get("transaction_revision_guard_absent") is not True
        or direct_route.get("packet_reservation_guard") is not True
    ):
        failures.append(_finding(
            "call-contract-direct-route",
            "direct body comparison, explicit invalidation, or revision guards are incomplete",
        ))
    if (
        not isinstance(generations, Mapping)
        or set(generations) != {
            "call_contract_verifier_generation",
            "normalizer_registry_generation",
            "expected_fact_schema_version",
        }
        or any(not isinstance(value, int) or value < 1 for value in generations.values())
    ):
        failures.append(_finding(
            "call-contract-generations",
            "call-contract generation identities are not explicit positive integers",
        ))
    continuation = report.get("repair_continuation")
    if (
        not isinstance(continuation, Mapping)
        or continuation.get("reachable") is not True
        or continuation.get("parent_only") is not True
        or continuation.get("producer_bound") is not True
        or continuation.get("nonaccepting") is not True
        or continuation.get("acceptance_eligible") is not False
    ):
        failures.append(_finding(
            "call-contract-repair-continuation",
            "repair continuation is not bound to the parent-only producer route",
        ))
    return failures


def _validate_call_contract_readiness(
    report: Any,
) -> list[dict[str, str]]:
    if not isinstance(report, Mapping):
        return [
            _finding(
                "call-contract-readiness",
                "call-contract readiness producer returned no object",
            )
        ]
    failures: list[dict[str, str]] = []
    if report.get("kind") != "call-contract-readiness-audit":
        failures.append(
            _finding(
                "call-contract-readiness",
                "call-contract readiness report has the wrong kind",
            )
        )
    structured_boundary = "producer_operational" in report
    if structured_boundary:
        infrastructure_findings = report.get("infrastructure_findings")
        reconstruction_blockers = report.get("typed_reconstruction_blockers")
        registration_blockers = report.get("stale_registration_blockers")
        if (
            report.get("producer_operational") is not True
            or report.get("passed") is not True
            or not isinstance(infrastructure_findings, list)
            or infrastructure_findings
        ):
            failures.append(
                _finding(
                    "call-contract-readiness",
                    "call-contract readiness producer is not operational or has "
                    "infrastructure findings",
                )
            )
        if not isinstance(report.get("candidate_readiness"), bool):
            failures.append(
                _finding(
                    "call-contract-readiness",
                    "call-contract readiness lacks a typed candidate-readiness decision",
                )
            )
        if not isinstance(reconstruction_blockers, list) or not isinstance(
            registration_blockers, list
        ):
            failures.append(
                _finding(
                    "call-contract-readiness",
                    "call-contract readiness lacks typed reconstruction or stale-registration blockers",
                )
            )
        elif any(
            not isinstance(blocker, Mapping)
            or not isinstance(blocker.get("kind"), str)
            or not blocker.get("kind")
            for blocker in reconstruction_blockers + registration_blockers
        ):
            failures.append(
                _finding(
                    "call-contract-readiness",
                    "call-contract readiness contains an untyped blocker",
                )
            )
        elif (
            report.get("candidate_readiness") is False
            and not reconstruction_blockers
            and not registration_blockers
        ):
            failures.append(
                _finding(
                    "call-contract-readiness",
                    "a not-ready call-contract candidate has no typed blocker",
                )
            )
        elif report.get("candidate_readiness") is True and (
            reconstruction_blockers or registration_blockers
        ):
            failures.append(
                _finding(
                    "call-contract-readiness",
                    "a ready call-contract candidate still has typed blockers",
                )
            )
    if (
        report.get("candidate_independent_slice_membership") is not True
        or report.get("source_authority") != "accepted-target-registration"
        or report.get("all_slices") is not True
        or report.get("phase_closeout_required") is not True
    ):
        failures.append(
            _finding(
                "call-contract-readiness",
                "call-contract readiness does not cover every candidate-independent original slice",
            )
        )
    slice_rows = report.get("slices")
    coverage_valid = isinstance(slice_rows, list)
    accepted_state_covered_body_count = 0
    typed_blocked_body_count = 0
    selected_body_count = 0
    ready_count = 0
    blocked_count = 0
    seen_slice_ids: set[str] = set()

    def exact_nonnegative_int(value: Any) -> bool:
        return isinstance(value, int) and not isinstance(value, bool) and value >= 0

    recognized_reconstruction_kinds = {"call-contract-bodies-not-current"}
    recognized_registration_kinds = {
        "historical-case-alias",
        "manifest-registration-drift",
    }

    typed_blockers_by_slice: dict[str, list[Mapping[str, Any]]] = {}
    if structured_boundary:
        reconstruction_blockers = report.get("typed_reconstruction_blockers")
        registration_blockers = report.get("stale_registration_blockers")
        blocker_groups = (
            (reconstruction_blockers, recognized_reconstruction_kinds),
            (registration_blockers, recognized_registration_kinds),
        )
        for blockers, recognized_kinds in blocker_groups:
            if not isinstance(blockers, list):
                coverage_valid = False
                continue
            for blocker in blockers:
                if not isinstance(blocker, Mapping):
                    coverage_valid = False
                    continue
                blocker_kind = blocker.get("kind")
                blocker_slice_id = blocker.get("slice_id")
                if (
                    not isinstance(blocker_kind, str)
                    or not blocker_kind
                    or blocker_kind not in recognized_kinds
                    or not isinstance(blocker_slice_id, str)
                    or not blocker_slice_id
                ):
                    coverage_valid = False
                    continue
                typed_blockers_by_slice.setdefault(blocker_slice_id, []).append(
                    blocker
                )

        aggregate_blockers = report.get("blockers")
        expected_aggregate_blockers = [
            *(report.get("infrastructure_findings") or []),
            *(
                registration_blockers
                if isinstance(registration_blockers, list)
                else []
            ),
            *(
                reconstruction_blockers
                if isinstance(reconstruction_blockers, list)
                else []
            ),
        ]
        if (
            not isinstance(aggregate_blockers, list)
            or len(aggregate_blockers) != len(expected_aggregate_blockers)
            or any(
                aggregate_blockers.count(blocker)
                != expected_aggregate_blockers.count(blocker)
                for blocker in expected_aggregate_blockers
            )
        ):
            coverage_valid = False

    if isinstance(slice_rows, list):
        for row in slice_rows:
            if not isinstance(row, Mapping):
                coverage_valid = False
                continue
            slice_id = row.get("slice_id")
            status = row.get("status")
            body_count = row.get("body_count")
            if (
                not isinstance(slice_id, str)
                or not slice_id
                or slice_id in seen_slice_ids
                or not exact_nonnegative_int(body_count)
            ):
                coverage_valid = False
                continue
            seen_slice_ids.add(slice_id)
            selected_body_count += body_count

            if status == "ready":
                ready_count += 1
                current_count = row.get("current_accepted_state_count")
                pending_count = row.get("pending_accepted_state_count")
                if (
                    row.get("accepted_state_schema")
                    != "direct-body-acceptance-v1"
                    or not exact_nonnegative_int(current_count)
                    or not exact_nonnegative_int(pending_count)
                    or current_count + pending_count != body_count
                ):
                    coverage_valid = False
                    continue
                accepted_state_covered_body_count += body_count
            elif structured_boundary and status == "blocked":
                blocked_count += 1
                # A typed reconstruction-blocked row is coverage, not accepted
                # evidence.  It must be attributable, nonempty, and mutually
                # exclusive with the direct accepted-state coverage class.
                if body_count <= 0:
                    coverage_valid = False
                if any(
                    key in row
                    for key in (
                        "accepted_state_schema",
                        "current_accepted_state_count",
                        "pending_accepted_state_count",
                        "partial_accepted_state_coverage",
                        "accepted_state_stale_reasons",
                    )
                ):
                    coverage_valid = False
                if (
                    row.get("candidate_status") not in {None, "not-ready"}
                    or row.get("candidate_readiness") is True
                    or row.get("ready") is True
                    or row.get("accepted") is True
                    or row.get("acceptance_eligible") is True
                    or "blocker" in row
                ):
                    coverage_valid = False

                row_blockers: list[Mapping[str, Any]] = []
                for key, recognized_kinds in (
                    ("typed_reconstruction_blockers", recognized_reconstruction_kinds),
                    ("stale_registration_blockers", recognized_registration_kinds),
                ):
                    value = row.get(key, [])
                    if not isinstance(value, list):
                        coverage_valid = False
                        continue
                    for blocker in value:
                        if not isinstance(blocker, Mapping):
                            coverage_valid = False
                            continue
                        blocker_kind = blocker.get("kind")
                        if (
                            not isinstance(blocker_kind, str)
                            or not blocker_kind
                            or blocker_kind not in recognized_kinds
                            or blocker.get("slice_id") != slice_id
                        ):
                            coverage_valid = False
                            continue
                        row_blockers.append(blocker)
                attributable_blockers = [
                    *typed_blockers_by_slice.get(slice_id, []),
                    *row_blockers,
                ]
                if not attributable_blockers:
                    coverage_valid = False
                else:
                    typed_blocked_body_count += body_count
            else:
                coverage_valid = False

    if any(slice_id not in seen_slice_ids for slice_id in typed_blockers_by_slice):
        coverage_valid = False

    if (
        (not structured_boundary and report.get("passed") is not True)
        or not exact_nonnegative_int(report.get("original_slice_count"))
        or report.get("original_slice_count", 0) < 1
        or not exact_nonnegative_int(report.get("selected_slice_count"))
        or not exact_nonnegative_int(report.get("ready_slice_count"))
        or not exact_nonnegative_int(report.get("blocked_slice_count"))
        or report.get("selected_slice_count") != report.get("original_slice_count")
        or not isinstance(slice_rows, list)
        or len(slice_rows) != report.get("original_slice_count")
        or (not structured_boundary and ready_count != report.get("original_slice_count"))
        or (not structured_boundary and blocked_count != 0)
        or (not structured_boundary and report.get("blockers") != [])
        or (structured_boundary and report.get("ready_slice_count") != ready_count)
        or (structured_boundary and report.get("blocked_slice_count") != blocked_count)
        or not exact_nonnegative_int(report.get("accepted_state_body_count"))
        or report.get("accepted_state_body_count")
        != accepted_state_covered_body_count
        or not exact_nonnegative_int(report.get("current_accepted_state_count"))
        or not exact_nonnegative_int(report.get("pending_accepted_state_count"))
        or report.get("current_accepted_state_count", 0)
        + report.get("pending_accepted_state_count", 0)
        != accepted_state_covered_body_count
        or accepted_state_covered_body_count + typed_blocked_body_count
        != selected_body_count
        or not coverage_valid
    ):
        failures.append(
            _finding(
                "call-contract-readiness",
                "call-contract readiness lacks complete typed cursor-slice or "
                "direct per-body accepted-state coverage",
            )
        )
    return failures


def _validate_relocations(report: Any) -> list[dict[str, str]]:
    if not isinstance(report, Mapping):
        return [_finding("relocation-producer", "relocation producer returned no object")]
    failures: list[dict[str, str]] = []
    if report.get("kind") != "relocation-expectations-audit":
        failures.append(_finding("relocation-producer", "relocation report has the wrong kind"))
    if report.get("candidate_independent") is not True:
        failures.append(
            _finding("relocation-producer", "relocation expected facts are candidate-derived")
        )
    if report.get("validation_mode") != "live-retail-derived":
        failures.append(
            _finding("relocation-producer", "relocation report is not live retail-derived")
        )
    rows = report.get("reports")
    if not isinstance(rows, list) or not rows:
        failures.append(_finding("relocation-producer", "relocation report has no typed rows"))
    elif any(
        not isinstance(row, Mapping)
        or row.get("candidate_independent") is not True
        or row.get("validation_mode") != "live-retail-derived"
        or not isinstance(row.get("unresolved"), list)
        for row in rows
    ):
        failures.append(_finding("relocation-producer", "relocation typed rows are malformed"))
    route = report.get("reviewed_exception_route")
    if not isinstance(route, Mapping) or not all(
        route.get(key) is True
        for key in ("reachable", "revision_guarded", "dry_run_available")
    ):
        failures.append(
            _finding(
                "relocation-producer",
                "reviewed relocation ambiguity route is not parser-reachable and revision-guarded",
            )
        )
    return failures


def _validate_final_catalog(report: Any) -> list[dict[str, str]]:
    if not isinstance(report, Mapping):
        return [_finding("final-coverage-producer", "final coverage producer returned no object")]
    failures: list[dict[str, str]] = []
    if report.get("kind") != "final-image-catalog-audit":
        failures.append(_finding("final-coverage-producer", "final coverage report has wrong kind"))
    if report.get("validation_mode") != "live-retail-plus-accepted-tracker":
        failures.append(
            _finding("final-coverage-producer", "final expected facts are not live retail/tracker facts")
        )
    if report.get("legacy_catalog_required") is not False:
        failures.append(
            _finding("final-coverage-producer", "final coverage still requires the legacy catalog blob")
        )
    coverage = report.get("coverage")
    if not isinstance(coverage, Mapping):
        failures.append(_finding("final-coverage-producer", "final report lacks live coverage"))
    else:
        if coverage.get("kind") != "live-final-image-coverage":
            failures.append(
                _finding("final-coverage-producer", "final coverage has the wrong typed producer kind")
            )
        if coverage.get("validation_mode") != "live-retail-plus-accepted-tracker":
            failures.append(
                _finding("final-coverage-producer", "final coverage is candidate-derived")
            )
        if not isinstance(coverage.get("failures"), list):
            failures.append(
                _finding("final-coverage-producer", "final coverage does not expose typed failures")
            )
        if coverage.get("complete") is not True and not coverage.get("failures"):
            failures.append(
                _finding("final-coverage-producer", "incomplete final coverage has no explicit blocker")
            )
    return failures


def _validate_final_verify(report: Any) -> list[dict[str, str]]:
    if not isinstance(report, Mapping):
        return [_finding("final-verifier-consumer", "final verifier probe returned no object")]
    if report.get("reachable") is not True or report.get("uses_live_coverage") is not True:
        return [_finding("final-verifier-consumer", "final verifier does not consume live coverage")]
    if report.get("status") == "blocked-before-build" and report.get("build_started") is not False:
        return [
            _finding(
                "final-verifier-consumer",
                "final verifier does not fail before build on incomplete typed coverage",
            )
        ]
    return []


def audit_pipeline_reachability(
    *,
    specs: Iterable[Any] | None = None,
    tracker: Path | None = None,
    reference: Path | None = None,
    manifest_dir: Path | None = None,
    executing_worktree_root: Path = REPO_ROOT,
    canonical_root: Path | None = None,
    order_report: Any = _UNSET,
    byte_report: Any = _UNSET,
    call_contract_report: Any = _UNSET,
    call_contract_readiness_report: Any = _UNSET,
    relocation_report: Any = _UNSET,
    final_catalog_report: Any = _UNSET,
    final_verify_report: Any = _UNSET,
    progress_module: ModuleType | None = None,
    _authenticated_inputs: tuple[object, Any, dict[str, Any] | None] | None = None,
) -> dict[str, Any]:
    execution_root = executing_worktree_root.resolve(strict=True)
    tracked_manifest_root = (
        manifest_dir.resolve(strict=False)
        if manifest_dir is not None
        else execution_root / "tools" / "vc5_verify_targets"
    )
    needs_tracker = any(
        report is _UNSET
        for report in (
            order_report,
            call_contract_readiness_report,
            relocation_report,
            final_catalog_report,
            final_verify_report,
        )
    )
    needs_reference = any(
        report is _UNSET
        for report in (relocation_report, final_catalog_report, final_verify_report)
    )
    if _authenticated_inputs is None:
        canonical_requested = bool(
            canonical_root is not None or "RECOIL_CANONICAL_ROOT" in os.environ
        )
        expected_manifest_root = execution_root / "tools" / "vc5_verify_targets"
        if (
            canonical_requested
            and manifest_dir is not None
            and tracked_manifest_root != expected_manifest_root
        ):
            raise WorktreeControlError(
                "live linked pipeline manifests must come from the executing "
                f"worktree: {tracked_manifest_root} != {expected_manifest_root}"
            )
        required_machine_local_paths: list[str] = []
        if needs_tracker and (tracker is None or canonical_requested):
            required_machine_local_paths.append(
                ".agent/RECONSTRUCTION_PROGRESS.sqlite3"
            )
        if needs_reference and (reference is None or canonical_requested):
            required_machine_local_paths.append("support/Recoil.exe")
        canonical = None
        if required_machine_local_paths:
            canonical = resolve_canonical_control_root(
                executing_worktree_root=execution_root,
                required_machine_local_paths=required_machine_local_paths,
                explicit_root=canonical_root,
            )
            if needs_tracker:
                expected_tracker = (
                    canonical.canonical_control_root
                    / ".agent"
                    / "RECONSTRUCTION_PROGRESS.sqlite3"
                )
                if tracker is None:
                    tracker = expected_tracker
                elif tracker.resolve(strict=True) != expected_tracker.resolve(strict=True):
                    raise WorktreeControlError(
                        "explicit tracker does not equal the authenticated canonical "
                        f"authority: {tracker.resolve(strict=True)} != "
                        f"{expected_tracker.resolve(strict=True)}"
                    )
                else:
                    tracker = tracker.resolve(strict=True)
            if needs_reference:
                expected_reference = (
                    canonical.canonical_control_root / "support" / "Recoil.exe"
                )
                if reference is None:
                    reference = expected_reference
                elif reference.resolve(strict=True) != expected_reference.resolve(strict=True):
                    raise WorktreeControlError(
                        "explicit reference does not equal the authenticated canonical "
                        f"retail input: {reference.resolve(strict=True)} != "
                        f"{expected_reference.resolve(strict=True)}"
                    )
                else:
                    reference = reference.resolve(strict=True)

        # Explicit fixture paths remain exact when no canonical root was requested.
        retail_handle = (
            StableReadHandle(reference)
            if reference is not None and needs_reference
            else None
        )
        retail_identity = (
            retail_handle.identity.to_dict() if retail_handle is not None else None
        )
        try:
            return audit_pipeline_reachability(
                specs=specs,
                tracker=tracker,
                reference=reference,
                manifest_dir=manifest_dir,
                executing_worktree_root=execution_root,
                canonical_root=canonical_root,
                order_report=order_report,
                byte_report=byte_report,
                call_contract_report=call_contract_report,
                call_contract_readiness_report=call_contract_readiness_report,
                relocation_report=relocation_report,
                final_catalog_report=final_catalog_report,
                final_verify_report=final_verify_report,
                progress_module=progress_module,
                _authenticated_inputs=(
                    _AUTHENTICATED_INPUTS_TOKEN,
                    canonical,
                    retail_identity,
                ),
            )
        finally:
            try:
                if canonical is not None:
                    reauthenticate_canonical_control_root(canonical)
                if retail_handle is not None and reference is not None:
                    observed = physical_identity(reference, directory=False)
                    require_same_physical_object(
                        retail_handle.identity,
                        observed,
                        context="pipeline reachability retail reference",
                    )
            finally:
                if retail_handle is not None:
                    retail_handle.close()

    token, canonical, retail_identity = _authenticated_inputs
    if token is not _AUTHENTICATED_INPUTS_TOKEN:
        raise WorktreeControlError(
            "pipeline input authentication cannot be supplied by a caller"
        )
    tracker_path = tracker
    reference_path = reference
    manifest_path = tracked_manifest_root
    if progress_module is None:
        from _recoil.commands import progress_cli as progress_module

    failures = _audit_registry(registry_view(specs))
    document: ProgressDocument | None = None
    if order_report is _UNSET or relocation_report is _UNSET:
        try:
            if tracker_path is None:
                raise RuntimeError("live tracker path was not resolved")
            document = ProgressDocument.load(tracker_path)
        except Exception as exc:
            failures.append(_finding("tracker-input", f"cannot load live tracker: {exc}"))

    def capture(check: str, operation: Any) -> Any:
        try:
            return operation()
        except Exception as exc:
            failures.append(_finding(check, f"producer invocation failed: {exc}"))
            return None

    if order_report is _UNSET:
        order_report = capture(
            "order-producer",
            lambda: _probe_current_order(document, progress_module),
        ) if document is not None else None
    if byte_report is _UNSET:
        byte_report = capture("byte-producer", lambda: _probe_byte_lanes(progress_module))
    if call_contract_report is _UNSET:
        call_contract_report = capture(
            "call-contract-producer",
            lambda: _probe_call_contract(
                progress_module, repository_root=execution_root
            ),
        )
    needs_vc5_tracker_binding = (
        call_contract_readiness_report is _UNSET
        or relocation_report is _UNSET
    )
    tracker_binding = (
        _bound_vc5_tracker(tracker_path)
        if needs_vc5_tracker_binding and tracker_path is not None
        else contextlib.nullcontext()
    )
    with tracker_binding:
        if call_contract_readiness_report is _UNSET:
            call_contract_readiness_report = capture(
                "call-contract-readiness",
                lambda: _probe_call_contract_readiness(
                    tracker=tracker_path,
                    execution_root=execution_root,
                    canonical_root=(
                        canonical.canonical_control_root
                        if canonical is not None
                        else None
                    ),
                ),
            )
        if relocation_report is _UNSET:
            relocation_report = capture(
                "relocation-producer",
                lambda: _probe_relocations(
                    document,
                    reference=reference_path,
                    manifest_dir=manifest_path,
                ),
            ) if document is not None else None
    if final_catalog_report is _UNSET:
        final_catalog_report = capture(
            "final-coverage-producer",
            lambda: _probe_final_catalog(tracker=tracker_path, reference=reference_path),
        )
    if final_verify_report is _UNSET:
        final_verify_report = capture(
            "final-verifier-consumer",
            lambda: _probe_final_verify(tracker=tracker_path, reference=reference_path),
        )

    failures.extend(_validate_order(order_report))
    failures.extend(_validate_byte(byte_report))
    failures.extend(_validate_call_contract(call_contract_report))
    failures.extend(
        _validate_call_contract_readiness(call_contract_readiness_report)
    )
    failures.extend(_validate_relocations(relocation_report))
    failures.extend(_validate_final_catalog(final_catalog_report))
    failures.extend(_validate_final_verify(final_verify_report))
    if document is not None:
        failures.extend(
            audit_generated_call_contract_commands(
                document, document.pipeline("recoil")
            )
        )

    producer_states = {
        "order": order_report.get("status") if isinstance(order_report, Mapping) else "unavailable",
        "byte_lanes": byte_report.get("status") if isinstance(byte_report, Mapping) else "unavailable",
        "call_contract": (
            call_contract_report.get("status")
            if isinstance(call_contract_report, Mapping)
            else "unavailable"
        ),
        "call_contract_readiness": (
            "ready"
            if isinstance(call_contract_readiness_report, Mapping)
            and (
                call_contract_readiness_report.get("candidate_readiness") is True
                if "producer_operational" in call_contract_readiness_report
                else call_contract_readiness_report.get("passed") is True
            )
            else "operational-not-ready"
            if isinstance(call_contract_readiness_report, Mapping)
            and call_contract_readiness_report.get("producer_operational") is True
            and call_contract_readiness_report.get("infrastructure_findings") == []
            else "blocked"
        ),
        "relocations": (
            "ready" if isinstance(relocation_report, Mapping) and relocation_report.get("passed") else "blocked"
        ),
        "final_coverage": (
            "ready" if isinstance(final_catalog_report, Mapping) and final_catalog_report.get("passed") else "blocked"
        ),
        "final_verify": (
            final_verify_report.get("status")
            if isinstance(final_verify_report, Mapping)
            else "unavailable"
        ),
    }
    return {
        "report_version": 1,
        "kind": "pipeline-reachability-audit",
        "passed": not failures,
        "structural_reachability": not failures,
        "reconstruction_complete": bool(
            isinstance(order_report, Mapping)
            and order_report.get("status") != "blocked"
            and isinstance(call_contract_readiness_report, Mapping)
            and (
                call_contract_readiness_report.get("candidate_readiness") is True
                if "producer_operational" in call_contract_readiness_report
                else call_contract_readiness_report.get("passed") is True
            )
            and call_contract_readiness_report.get("pending_accepted_state_count") == 0
            and isinstance(call_contract_report, Mapping)
            and call_contract_report.get("acceptance_enabled") is True
            and isinstance(relocation_report, Mapping)
            and relocation_report.get("passed") is True
            and isinstance(final_catalog_report, Mapping)
            and final_catalog_report.get("passed") is True
        ),
        "producer_states": producer_states,
        "producers": {
            "current_order": order_report,
            "byte_lanes": byte_report,
            "call_contract": call_contract_report,
            "call_contract_readiness": call_contract_readiness_report,
            "relocations": relocation_report,
            "final_coverage": final_catalog_report,
            "final_verify": final_verify_report,
        },
        "failure_count": len(failures),
        "failures": failures,
        "execution_worktree_root": str(execution_root),
        "canonical_control_root": (
            str(canonical.canonical_control_root) if canonical is not None else None
        ),
        "canonical_resolution_source": (
            canonical.resolution_source if canonical is not None else None
        ),
        "retail_reference_path": str(reference_path) if reference_path is not None else None,
        "retail_reference_from_canonical_control_root": bool(
            canonical is not None
            and reference_path is not None
            and reference_path == canonical.canonical_control_root / "support" / "Recoil.exe"
        ),
        "retail_physical_identity": retail_identity,
        "progress_path": str(tracker_path) if tracker_path is not None else None,
        "issue_ledger_path": None,
        "issue_ledger_used": False,
        "tracked_manifest_root": str(manifest_path),
        "tracked_inputs_from_execution_worktree": (
            manifest_path == execution_root / "tools" / "vc5_verify_targets"
        ),
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Prove that every fail-closed live pipeline consumer has a candidate-independent "
            "expected-fact producer and executable transition."
        )
    )
    parser.add_argument("--root", type=Path, default=REPO_ROOT)
    parser.add_argument("--canonical-root", type=Path)
    parser.add_argument("--tracker", type=Path)
    parser.add_argument("--reference", type=Path)
    parser.add_argument("--manifest-dir", type=Path)
    parser.add_argument("--strict", action="store_true")
    parser.add_argument("--json", action="store_true")
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    report = audit_pipeline_reachability(
        tracker=args.tracker,
        reference=args.reference,
        manifest_dir=args.manifest_dir,
        executing_worktree_root=args.root,
        canonical_root=args.canonical_root,
    )
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print(f"Pipeline reachability: {'PASS' if report['passed'] else 'FAIL'}")
        print(
            "- producer states: "
            + ", ".join(f"{key}={value}" for key, value in report["producer_states"].items())
        )
        for failure in report["failures"]:
            print(f"- {failure['check']}: {failure['message']}")
        if report["passed"] and not report["reconstruction_complete"]:
            print("- live producers are reachable; current reconstruction facts remain incomplete")
    # Structural failures always fail. Current typed reconstruction blockers do not.
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
