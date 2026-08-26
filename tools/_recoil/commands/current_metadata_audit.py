from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
from typing import Any, Mapping

from _recoil.lib.progress import DEFAULT_PROGRESS_PATH, ProgressDocument, ProgressStore
from _recoil.lib.tooling import REPO_ROOT, configure_stdio, display_path


GENERATED_KIND = "generated-current"
GENERATOR_COMMAND = "python tools/recoil.py progress next --json"
REFRESH_COMMAND = "python tools/recoil.py progress current-metadata refresh"
GENERATED_CURRENT_PATH = (
    "binaries.recoil.source_layout_context.provenance_status_summary.generated_current"
)
STATIC_CURRENT_PATTERN = re.compile(
    r"(?:authoritative current|current primary cursor|current divergence|"
    r"current cursor\s+(?:is|at)|current (?:authored|full|linked)[^.!?]{0,40} cursor)",
    re.IGNORECASE,
)


def current_scope_identity(next_work: Mapping[str, Any]) -> dict[str, Any]:
    """Expose the scheduler fields that identify the live primary scope."""

    return {
        "lane": "primary",
        "phase": next_work.get("phase"),
        "cursor": next_work.get("cursor"),
        "physical_block_id": next_work.get("physical_block_id"),
        "primary_lane": next_work.get("primary_lane"),
    }


def generated_current_record(
    document: ProgressDocument,
    *,
    committed_revision: int | None = None,
) -> dict[str, Any]:
    """Derive the one non-authoritative display record from scheduler output."""
    next_work = document.next_work("recoil")
    primary = next_work.get("cursor_launchability", {}).get("primary", {})
    if not isinstance(primary, Mapping):
        primary = {}
    phase = str(next_work.get("phase") or "")
    cursor = next_work.get("cursor")
    reason_code = str(primary.get("typed_reason_code") or primary.get("reason_code") or "none")
    action = str(primary.get("required_parent_action") or "none")
    return {
        "metadata_kind": GENERATED_KIND,
        "tracker_revision": document.revision if committed_revision is None else committed_revision,
        "command": GENERATOR_COMMAND,
        "lane": "primary",
        "phase": phase,
        "cursor": cursor,
        "scope_identity": current_scope_identity(next_work),
        "reason_code": reason_code,
        "required_parent_action": action,
        "narrative": (
            f"Generated primary {phase} at {cursor}: {reason_code}; action={action}."
        ),
    }


def refresh_remaining_blocker_metadata(data: dict[str, Any]) -> dict[str, Any]:
    """Replace only the governed source-layout live-summary surface."""
    # The commit path owns this mutable graph already.  Avoid a second complete
    # defensive copy of the full tracker solely to derive generated metadata.
    document = ProgressDocument._from_owned_data(data)
    binaries = data.get("binaries")
    recoil = binaries.get("recoil") if isinstance(binaries, dict) else None
    layout = recoil.get("source_layout_context") if isinstance(recoil, dict) else None
    summary = layout.get("provenance_status_summary") if isinstance(layout, dict) else None
    if not isinstance(summary, dict) or "remaining_blocker" not in summary:
        raise ValueError(
            "binaries.recoil.source_layout_context.provenance_status_summary.remaining_blocker "
            "is missing; this governed command cannot patch another field"
        )
    old = summary.get("remaining_blocker")
    history = summary.setdefault("remaining_blocker_history", [])
    if not isinstance(history, list):
        raise ValueError("remaining_blocker_history must be a list")
    if isinstance(old, str) and old and old != "Dynamic scheduler state: consult generated_current.":
        historical = {
            "historicalized_at_revision": document.revision + 1,
            "narrative": old,
        }
        if historical not in history:
            history.append(historical)
    summary["remaining_blocker"] = "Dynamic scheduler state: consult generated_current."
    summary["generated_current"] = generated_current_record(
        document,
        committed_revision=document.revision + 1,
    )
    return {
        "path": (
            "binaries.recoil.source_layout_context.provenance_status_summary."
            "remaining_blocker"
        ),
        "historicalized": isinstance(old, str) and bool(old),
        "generated_current": summary["generated_current"],
    }


def audit_current_metadata(document: ProgressDocument) -> list[dict[str, str]]:
    next_work = document.next_work("recoil")
    expected = {
        "tracker_revision": document.revision,
        "command": GENERATOR_COMMAND,
        "lane": "primary",
        "phase": next_work.get("phase"),
        "cursor": next_work.get("cursor"),
        "scope_identity": current_scope_identity(next_work),
    }
    findings: list[dict[str, str]] = []

    remediation = (
        f"{REFRESH_COMMAND} --expected-revision {document.revision} --dry-run --json"
    )

    def add(code: str, path: str, message: str) -> None:
        findings.append({
            "severity": "error",
            "code": code,
            "path": path,
            "message": message,
            "remediation": remediation,
        })

    def walk(value: Any, path: str) -> None:
        if isinstance(value, Mapping):
            if value.get("metadata_kind") == GENERATED_KIND:
                missing = [key for key in expected if key not in value]
                if missing:
                    add(
                        "generated-current-fields",
                        path,
                        "generated current metadata lacks " + ", ".join(missing),
                    )
                for key, expected_value in expected.items():
                    if key in value and value.get(key) != expected_value:
                        add(
                            "generated-current-stale",
                            f"{path}.{key}",
                            f"expected {expected_value!r}, found {value.get(key)!r}",
                        )
                if "reservation" in value or "resource_claims" in value or "leases" in value:
                    add(
                        "generated-current-lease-mixing",
                        path,
                        "generated current metadata must remain separate from reservation and lease state",
                    )
                return
            for key, item in value.items():
                # Immutable evidence may quote the then-current frontier. It is historical
                # evidence, not a live scheduling surface.
                if not path and key == "evidence":
                    continue
                if key in {"remaining_blocker_history", "first_unresolved_item_history"}:
                    continue
                walk(item, f"{path}.{key}" if path else str(key))
            return
        if isinstance(value, list):
            for index, item in enumerate(value):
                walk(item, f"{path}[{index}]")
            return
        if isinstance(value, str) and STATIC_CURRENT_PATTERN.search(value):
            add(
                "static-current-narrative",
                path,
                "static narrative claims a current cursor/divergence; replace it with generated-current metadata or historical wording",
            )

    binaries = document.data.get("binaries")
    recoil = binaries.get("recoil") if isinstance(binaries, Mapping) else None
    layout = recoil.get("source_layout_context") if isinstance(recoil, Mapping) else None
    summary = layout.get("provenance_status_summary") if isinstance(layout, Mapping) else None
    governed = summary.get("generated_current") if isinstance(summary, Mapping) else None
    if not isinstance(governed, Mapping) or governed.get("metadata_kind") != GENERATED_KIND:
        add(
            "generated-current-missing",
            GENERATED_CURRENT_PATH,
            "governed generated current metadata is missing",
        )

    walk(document.data, "")
    return findings


def main() -> int:
    configure_stdio()
    parser = argparse.ArgumentParser(description="Audit dynamic current-cursor metadata.")
    parser.add_argument("--progress", type=Path, default=DEFAULT_PROGRESS_PATH)
    parser.add_argument("--readme", type=Path)
    parser.add_argument("--strict", action="store_true")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    document = ProgressStore(args.progress).load()
    findings = audit_current_metadata(document)
    if args.readme is not None or args.progress.resolve() == DEFAULT_PROGRESS_PATH.resolve():
        from _recoil.commands.readme_progress import (
            DEFAULT_README_PATH,
            readme_freshness_findings,
        )

        findings.extend(
            readme_freshness_findings(
                progress_path=args.progress,
                readme_path=args.readme or DEFAULT_README_PATH,
            )
        )
    payload = {
        "progress": display_path(args.progress),
        "tracker_revision": document.revision,
        "current": {
            "lane": "primary",
            "cursor": document.next_work("recoil").get("cursor"),
            "scope_identity": current_scope_identity(document.next_work("recoil")),
        },
        "findings": findings,
        "errors": len(findings),
    }
    if args.json:
        print(json.dumps(payload, indent=2, sort_keys=True))
    else:
        print(f"progress={payload['progress']}")
        print(f"tracker_revision={document.revision}")
        for finding in findings:
            print(f"ERROR {finding['code']} {finding['path']}: {finding['message']}")
        print(f"errors={len(findings)}")
    return 1 if args.strict and findings else 0


if __name__ == "__main__":
    raise SystemExit(main())
