from __future__ import annotations

import argparse
from collections import Counter
import json
from pathlib import Path
from typing import Any, Mapping

from _recoil.commands.call_contract_verify import (
    _call_contract_slice_targets,
    _call_contract_target_registrations,
    call_contract_registration_path_reconciliation,
    call_contract_source_closure,
    source_dependency_paths,
)
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH, ProgressDocument
from _recoil.lib.repository_paths import (
    RepositoryPathInventory,
    RepositoryPathError,
    load_repository_path_inventory,
    resolve_repository_file,
)
from _recoil.lib.tooling import REPO_ROOT, configure_stdio


DEFAULT_TRACKER = DEFAULT_PROGRESS_PATH
READINESS_VALIDATION_MODE = "live-tracker-exact-source-closure"


def _finding(
    kind: str,
    message: str,
    *,
    slice_id: str | None = None,
    ordinal: int | None = None,
) -> dict[str, Any]:
    result: dict[str, Any] = {"kind": kind, "message": message}
    if slice_id is not None:
        result["slice_id"] = slice_id
    if ordinal is not None:
        result["ordinal"] = ordinal
    return result


def _canonical_dependency_paths(
    paths: list[str],
    *,
    inventory: RepositoryPathInventory,
) -> list[str]:
    """Authenticate current dependencies without projecting NTFS spelling."""

    result: list[str] = []
    seen: set[str] = set()
    for index, raw_path in enumerate(paths):
        if not isinstance(raw_path, str) or not raw_path:
            raise ValueError(
                f"dependency_paths[{index}] must be a non-empty string"
            )
        try:
            tracked = resolve_repository_file(
                raw_path,
                repository_root=inventory.repository_root,
                inventory=inventory,
                context=f"call-contract dependency_paths[{index}]",
            )
        except RepositoryPathError as exc:
            raise ValueError(str(exc)) from exc
        key = tracked.repository_path.casefold()
        if key in seen:
            raise ValueError(
                f"call-contract dependency is duplicated case-insensitively: {raw_path}"
            )
        seen.add(key)
        result.append(tracked.repository_path)
    return sorted(result, key=lambda path: (path.casefold(), path))


def audit_call_contract_readiness(
    *,
    tracker: Path = DEFAULT_TRACKER,
    all_slices: bool = False,
    slice_id: str | None = None,
    document: ProgressDocument | None = None,
) -> dict[str, Any]:
    infrastructure_findings: list[dict[str, Any]] = []
    typed_reconstruction_blockers: list[dict[str, Any]] = []
    stale_registration_blockers: list[dict[str, Any]] = []
    if all_slices == bool(slice_id):
        infrastructure_findings.append(
            _finding(
                "selection",
                "select exactly one of --all-slices or --slice",
            )
        )
        return {
            "report_version": 2,
            "kind": "call-contract-readiness-audit",
            "validation_mode": READINESS_VALIDATION_MODE,
            "candidate_independent_slice_membership": True,
            "source_authority": "accepted-target-registration",
            "all_slices": all_slices,
            "original_slice_count": 0,
            "selected_slice_count": 0,
            "ready_slice_count": 0,
            "blocked_slice_count": 0,
            "accepted_state_body_count": 0,
            "current_accepted_state_count": 0,
            "pending_accepted_state_count": 0,
            "phase_closeout_required": True,
            "producer_operational": False,
            "infrastructure_findings": infrastructure_findings,
            "candidate_readiness": False,
            "typed_reconstruction_blockers": [],
            "stale_registration_blockers": [],
            "passed": False,
            "slices": [],
            "blockers": infrastructure_findings,
        }

    try:
        repository_inventory = load_repository_path_inventory(REPO_ROOT)
        live_document = document or ProgressDocument.load(tracker)
        original_slices = live_document.authored_call_contract_slices("recoil")
    except Exception as exc:
        infrastructure_findings.append(_finding("tracker-input", str(exc)))
        original_slices = []
        live_document = document
        repository_inventory = None

    selected: list[Mapping[str, Any]] = []
    if original_slices:
        if all_slices:
            selected = list(original_slices)
        else:
            selected = [
                row for row in original_slices if row.get("id") == slice_id
            ]
            if not selected:
                infrastructure_findings.append(
                    _finding(
                        "slice-selection",
                        f"unknown original call-contract slice: {slice_id}",
                    )
                )
    elif not infrastructure_findings:
        infrastructure_findings.append(
            _finding(
                "empty-census",
                "the live reviewed authored-order census produced no call-contract slices",
            )
        )

    reports: list[dict[str, Any]] = []
    if live_document is not None and repository_inventory is not None:
        for slice_row in selected:
            selected_id = str(slice_row.get("id") or "")
            ordinal = int(slice_row.get("ordinal") or 0)
            try:
                targets = _call_contract_slice_targets(
                    live_document,
                    slice_row,
                    repository_path_inventory=repository_inventory,
                )
                reconciliations: list[dict[str, Any]] = []
                slice_stale_blockers: list[dict[str, Any]] = []
                for target_id, registration, _manifest_path in (
                    _call_contract_target_registrations(
                        live_document, slice_row
                    )
                ):
                    reconciliation = (
                        call_contract_registration_path_reconciliation(
                            target_id=target_id,
                            registration=registration,
                            current_target=targets[target_id],
                            inventory=repository_inventory,
                        )
                    )
                    reconciliations.append(reconciliation)
                    if reconciliation["status"] == "historical-case-alias":
                        blocker = {
                            "kind": "historical-case-alias",
                            "message": (
                                f"target {target_id} has historical path spelling "
                                "that is not current evidence"
                            ),
                            "slice_id": selected_id,
                            "ordinal": ordinal,
                            **reconciliation,
                        }
                        slice_stale_blockers.append(blocker)
                        stale_registration_blockers.append(blocker)
                    elif reconciliation["blocker_kind"] is not None:
                        blocker = {
                            "kind": str(reconciliation["blocker_kind"]),
                            "message": (
                                f"target {target_id} registration differs from "
                                "the current tracked manifest"
                            ),
                            "slice_id": selected_id,
                            "ordinal": ordinal,
                            **reconciliation,
                        }
                        slice_stale_blockers.append(blocker)
                        stale_registration_blockers.append(blocker)

                registration_drift = any(
                    row["blocker_kind"] is not None
                    for row in reconciliations
                )
                if registration_drift:
                    reports.append(
                        {
                            "slice_id": selected_id,
                            "ordinal": ordinal,
                            "status": "blocked",
                            "candidate_status": "not-ready",
                            "body_count": int(
                                slice_row.get("body_count") or 0
                            ),
                            "target_ids": list(
                                slice_row.get("target_ids") or []
                            ),
                            "target_manifest_count": len(targets),
                            "registration_reconciliations": reconciliations,
                            "stale_registration_blockers": (
                                slice_stale_blockers
                            ),
                            "producer_operational": True,
                        }
                    )
                    continue
                closure = call_contract_source_closure(
                    live_document,
                    slice_row,
                    repository_path_inventory=repository_inventory,
                )
                if not closure.registered_source_paths:
                    raise ValueError(
                        "exact accepted target registration produced no implementation roots"
                    )
                dependency_paths = _canonical_dependency_paths(
                    list(closure.dependency_paths),
                    inventory=repository_inventory,
                )
                accepted_state_rows: list[dict[str, Any]] = []
                currentness = getattr(
                    live_document, "call_contract_body_currentness", None
                )
                if callable(currentness):
                    accepted_state_rows = [
                        dict(currentness(str(symbol_id)))
                        for symbol_id in slice_row.get("symbol_ids", [])
                    ]
                current_accepted_state_count = sum(
                    row.get("current") is True for row in accepted_state_rows
                )
                accepted_state_reasons = Counter(
                    str(row.get("reason", "unknown"))
                    for row in accepted_state_rows
                    if row.get("current") is not True
                )
                pending_count = (
                    len(accepted_state_rows) - current_accepted_state_count
                )
                slice_reconstruction_blockers: list[dict[str, Any]] = []
                if pending_count:
                    blocker = {
                        "kind": "call-contract-bodies-not-current",
                        "message": (
                            f"{pending_count} selected call-contract bodies are "
                            "not current"
                        ),
                        "slice_id": selected_id,
                        "ordinal": ordinal,
                        "pending_body_count": pending_count,
                        "stale_reasons": dict(
                            sorted(accepted_state_reasons.items())
                        ),
                    }
                    slice_reconstruction_blockers.append(blocker)
                    typed_reconstruction_blockers.append(blocker)
                reports.append(
                    {
                        "slice_id": selected_id,
                        "ordinal": ordinal,
                        "status": "ready",
                        "candidate_status": (
                            "ready"
                            if not pending_count and not slice_stale_blockers
                            else "not-ready"
                        ),
                        "producer_operational": True,
                        "body_count": int(slice_row.get("body_count") or 0),
                        "target_ids": list(slice_row.get("target_ids") or []),
                        "target_manifest_count": len(targets),
                        "registration_reconciliations": reconciliations,
                        "stale_registration_blockers": (
                            slice_stale_blockers
                        ),
                        "typed_reconstruction_blockers": (
                            slice_reconstruction_blockers
                        ),
                        "registered_source_paths": list(
                            closure.registered_source_paths
                        ),
                        "header_path_count": len(closure.header_paths),
                        "definition_source_path_count": len(
                            closure.definition_source_paths
                        ),
                        "source_edit_path_count": len(
                            closure.source_edit_paths
                        ),
                        "write_path_count": len(
                            closure.source_edit_paths
                        ),
                        "dependency_path_count": len(dependency_paths),
                        "closure_contract_version": 2,
                        "definition_resolution": dict(
                            closure.definition_resolution
                        ),
                        "accepted_state_schema": (
                            "direct-body-acceptance-v1"
                            if accepted_state_rows
                            else "legacy-or-unavailable"
                        ),
                        "current_accepted_state_count": current_accepted_state_count,
                        "pending_accepted_state_count": (
                            pending_count
                        ),
                        "partial_accepted_state_coverage": bool(
                            accepted_state_rows
                            and 0 < current_accepted_state_count < len(accepted_state_rows)
                        ),
                        "accepted_state_stale_reasons": dict(
                            sorted(accepted_state_reasons.items())
                        ),
                    }
                )
            except Exception as exc:
                blocker = _finding(
                    "source-dependency-closure",
                    str(exc),
                    slice_id=selected_id,
                    ordinal=ordinal,
                )
                infrastructure_findings.append(blocker)
                reports.append(
                    {
                        "slice_id": selected_id,
                        "ordinal": ordinal,
                        "status": "blocked",
                        "body_count": int(slice_row.get("body_count") or 0),
                        "target_ids": list(slice_row.get("target_ids") or []),
                        "blocker": blocker,
                        "producer_operational": False,
                    }
                )

    ready_count = sum(row.get("status") == "ready" for row in reports)
    blocked_count = sum(row.get("status") == "blocked" for row in reports)
    producer_operational = bool(selected) and not infrastructure_findings
    candidate_readiness = bool(
        producer_operational
        and not typed_reconstruction_blockers
        and not stale_registration_blockers
        and ready_count == len(selected)
    )
    current_accepted_state_count = sum(
        int(row.get("current_accepted_state_count", 0)) for row in reports
    )
    accepted_state_body_count = sum(
        int(row.get("current_accepted_state_count", 0))
        + int(row.get("pending_accepted_state_count", 0))
        for row in reports
    )
    return {
        "report_version": 2,
        "kind": "call-contract-readiness-audit",
        "validation_mode": READINESS_VALIDATION_MODE,
        "candidate_independent_slice_membership": True,
        "source_authority": "accepted-target-registration",
        "legacy_source_metadata_policy": (
            "ignored-only-with-independent-exact-implementation-root"
        ),
        "all_slices": all_slices,
        "original_slice_count": len(original_slices),
        "selected_slice_count": len(selected),
        "ready_slice_count": ready_count,
        "blocked_slice_count": blocked_count,
        "accepted_state_body_count": accepted_state_body_count,
        "current_accepted_state_count": current_accepted_state_count,
        "pending_accepted_state_count": (
            accepted_state_body_count - current_accepted_state_count
        ),
        "phase_closeout_required": True,
        "producer_operational": producer_operational,
        "infrastructure_findings": infrastructure_findings,
        "candidate_readiness": candidate_readiness,
        "typed_reconstruction_blockers": typed_reconstruction_blockers,
        "stale_registration_blockers": stale_registration_blockers,
        "passed": producer_operational,
        "slices": reports,
        "blockers": [
            *infrastructure_findings,
            *stale_registration_blockers,
            *typed_reconstruction_blockers,
        ],
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Preflight exact source/header/definition dependency closure for "
            "the deterministic original authored call-contract slices."
        )
    )
    parser.add_argument("--tracker", type=Path, default=DEFAULT_TRACKER)
    selection = parser.add_mutually_exclusive_group(required=True)
    selection.add_argument("--all-slices", action="store_true")
    selection.add_argument("--slice")
    parser.add_argument("--strict", action="store_true")
    parser.add_argument("--json", action="store_true")
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    report = audit_call_contract_readiness(
        tracker=args.tracker,
        all_slices=args.all_slices,
        slice_id=args.slice,
    )
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print(
            "Call-contract readiness: "
            f"{'PASS' if report['passed'] else 'FAIL'} "
            f"({report['ready_slice_count']}/{report['selected_slice_count']} ready)"
        )
        for blocker in report["blockers"]:
            prefix = (
                f"slice {blocker['ordinal']} ({blocker['slice_id']}): "
                if "slice_id" in blocker
                else ""
            )
            print(f"- {prefix}{blocker['kind']}: {blocker['message']}")
    return 1 if args.strict and not report["passed"] else 0


if __name__ == "__main__":
    raise SystemExit(main())
