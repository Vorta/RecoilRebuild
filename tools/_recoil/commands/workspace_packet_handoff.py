"""Read-only workspace-issue packet handoff authentication and projection.

This module is deliberately outside the call-contract verifier component
closure.  It owns workspace-issue ledger/reservation checks and native-Git
baseline reauthentication; it never performs reconstruction acceptance.
"""

from __future__ import annotations

from pathlib import Path
from typing import Any, Mapping

from _recoil.commands.workspace_issues import (
    ISSUE_WORK_ROLES,
    find_work_packet,
    load_ledger,
    validate_ledger,
)
from _recoil.lib.git_change_control import (
    GitChangeControlError,
    reauthenticate_clean_git_baseline,
)
from _recoil.lib.worktree_control import (
    WorktreeControlError,
    authenticated_validation_command_tokens,
    authenticate_build_root,
    resolve_exact_packet_worktree,
)
from _recoil.lib.progress import ProgressError, normalize_resource_claims


class WorkspacePacketHandoffError(RuntimeError):
    """A workspace-issue packet cannot be rendered as a runnable handoff."""


def _compact_reserved_packet(
    packet_id: str,
    packet: Mapping[str, Any],
    reservation: Mapping[str, Any],
    *,
    repository_root: Path,
) -> dict[str, Any]:
    if packet.get("state") != "active":
        raise WorkspacePacketHandoffError(
            f"issue work packet {packet_id} is not active"
        )
    reservation_id = packet.get("reservation_id")
    if not isinstance(reservation_id, str) or not reservation_id:
        raise WorkspacePacketHandoffError(
            f"issue work packet {packet_id} has no reservation id"
        )
    if (
        reservation.get("id") != reservation_id
        or reservation.get("packet_id") != packet_id
        or reservation.get("state") != "active"
    ):
        raise WorkspacePacketHandoffError(
            f"issue work packet {packet_id} has no exact active reservation"
        )
    handoff_role = packet.get("handoff_role")
    if handoff_role not in ISSUE_WORK_ROLES:
        raise WorkspacePacketHandoffError(
            f"issue work packet {packet_id} has unsupported handoff role"
        )
    reservation_role = reservation.get("handoff_role")
    if reservation_role is not None and reservation_role != handoff_role:
        raise WorkspacePacketHandoffError(
            f"issue work packet {packet_id} reservation role does not match packet role"
        )

    raw_claims = packet.get("resource_claims")
    if not isinstance(raw_claims, list):
        raise WorkspacePacketHandoffError(
            f"issue work packet {packet_id} has malformed resource claims"
        )
    try:
        claims = normalize_resource_claims(
            row for row in raw_claims if isinstance(row, Mapping)
        )
    except ProgressError as exc:
        raise WorkspacePacketHandoffError(str(exc)) from exc
    if claims != raw_claims or reservation.get("resource_claims") != claims:
        raise WorkspacePacketHandoffError(
            f"issue work packet {packet_id} reservation claims do not match packet claims"
        )
    write_claims = [row for row in claims if row["access"] == "write"]
    if not write_claims:
        raise WorkspacePacketHandoffError(
            f"issue work packet {packet_id} has no write claims"
        )
    write_paths = sorted(
        row["id"]
        for row in write_claims
        if row["kind"] in {"path", "source-path", "header-path"}
    )
    if not write_paths:
        raise WorkspacePacketHandoffError(
            f"issue work packet {packet_id} has no writable governed paths"
        )
    read_claims = [dict(row) for row in claims if row["access"] == "read"]
    read_paths = sorted(
        row["id"]
        for row in read_claims
        if row["kind"] in {"path", "source-path", "header-path"}
    )

    allowed_paths = packet.get("allowed_paths")
    forbidden_paths = packet.get("forbidden_paths")
    if (
        not isinstance(allowed_paths, list)
        or not allowed_paths
        or any(not isinstance(path, str) or not path.strip() for path in allowed_paths)
    ):
        raise WorkspacePacketHandoffError(
            f"issue work packet {packet_id} has incomplete allowed paths"
        )
    if (
        not isinstance(forbidden_paths, list)
        or not forbidden_paths
        or any(not isinstance(path, str) or not path.strip() for path in forbidden_paths)
    ):
        raise WorkspacePacketHandoffError(
            f"issue work packet {packet_id} has incomplete forbidden paths"
        )
    normalized_allowed = sorted({path.strip() for path in allowed_paths})
    normalized_forbidden = sorted({path.strip() for path in forbidden_paths})
    if normalized_allowed != write_paths:
        raise WorkspacePacketHandoffError(
            f"issue work packet {packet_id} allowed paths do not match write claims"
        )
    if set(normalized_allowed) & set(normalized_forbidden):
        raise WorkspacePacketHandoffError(
            f"issue work packet {packet_id} allowed and forbidden paths overlap"
        )

    if packet.get("validation_command_contract_version") != 1:
        raise WorkspacePacketHandoffError(
            f"issue work packet {packet_id} requires validation command contract v1"
        )
    validation_commands = packet.get("validation_commands")
    if (
        not isinstance(validation_commands, list)
        or len(validation_commands) != 1
        or not isinstance(validation_commands[0], str)
        or not validation_commands[0].strip()
    ):
        raise WorkspacePacketHandoffError(
            f"issue work packet {packet_id} requires exactly one validation command"
        )
    commands = [validation_commands[0]]
    worker_command = packet.get("next_command")
    if (
        not isinstance(worker_command, str)
        or worker_command != commands[0]
    ):
        raise WorkspacePacketHandoffError(
            f"issue work packet {packet_id} next command must byte-identically equal "
            "the single validation command"
        )
    try:
        authenticated_validation_command_tokens(
            commands[0],
            require_public_route=False,
            resource_claims=claims,
        )
    except WorktreeControlError as exc:
        raise WorkspacePacketHandoffError(
            f"issue work packet {packet_id} validation command is not authenticated: {exc}"
        ) from exc
    required_return_fields = packet.get("required_return_fields")
    if (
        not isinstance(required_return_fields, list)
        or not required_return_fields
        or any(
            not isinstance(field, str) or not field.strip()
            for field in required_return_fields
        )
    ):
        raise WorkspacePacketHandoffError(
            f"issue work packet {packet_id} has incomplete required return fields"
        )
    scope = packet.get("scope")
    issue_id = packet.get("issue_id")
    if not isinstance(scope, str) or not scope.strip():
        raise WorkspacePacketHandoffError(
            f"issue work packet {packet_id} has no scope"
        )
    if not isinstance(issue_id, str) or not issue_id:
        raise WorkspacePacketHandoffError(
            f"issue work packet {packet_id} has no issue id"
        )

    descriptor = reservation.get("git_workspace_baseline")
    if not isinstance(descriptor, Mapping):
        raise WorkspacePacketHandoffError(
            f"issue work packet {packet_id} has no complete Git workspace baseline"
        )
    try:
        packet_worktree, association = resolve_exact_packet_worktree(
            repository_root,
            descriptor,
            packet_id=packet_id,
            writable_paths=write_paths,
        )
        baseline = reauthenticate_clean_git_baseline(
            packet_worktree,
            descriptor,
            packet_id=packet_id,
            writable_paths=write_paths,
        )
        if association is not None:
            authenticate_build_root(
                association.external_build_root,
                authority="issue",
                packet_id=packet_id,
                branch=str(baseline["branch"]),
                worktree_root=packet_worktree,
            )
    except (GitChangeControlError, WorktreeControlError) as exc:
        raise WorkspacePacketHandoffError(
            f"issue work packet {packet_id} Git handoff preflight failed: {exc}"
        ) from exc

    return {
        "packet_type": "issue-maintenance-v1",
        "packet_source": "workspace-issues",
        "packet_id": packet_id,
        "reservation_id": reservation_id,
        "handoff_role": handoff_role,
        "issue_id": issue_id,
        "phase": "workspace-issue",
        "lane": "issue",
        "byte_lane": "",
        "cursor": "",
        "slice_id": "",
        "target": issue_id,
        "linked_target_id": "",
        "object_target_id": "",
        "worker_target_id": issue_id,
        "target_ids": [issue_id],
        "scope_ids": [issue_id],
        "covered_block_ids": [],
        "write_paths": write_paths,
        "read_paths": read_paths,
        "read_dependencies": read_claims,
        "allowed_paths": normalized_allowed,
        "forbidden_paths": normalized_forbidden,
        "worker_command": worker_command,
        "validation_commands": commands,
        "objective": scope.strip(),
        "stop_condition": (
            "Return after all listed validation commands pass or at the first "
            "concrete blocker."
        ),
        "required_return_fields": [field.strip() for field in required_return_fields],
        "baseline_schema": baseline["schema"],
        "baseline_commit": baseline["baseline_commit"],
        "branch": baseline["branch"],
        "repository_root": str(repository_root),
        "worktree_root": baseline["worktree_root"],
        "external_build_root": (
            association.external_build_root if association is not None else None
        ),
        "git_object_ids_are_opaque": True,
        "nonaccepting": True,
        "acceptance_eligible": False,
        "worker_acceptance_allowed": False,
        "git_restrictions": {
            "worker_git_mutation_allowed": True,
            "worker_may_stage_exact_writable_paths": True,
            "worker_may_create_one_packet_commit": True,
            "packet_commit_message_must_contain_packet_id": packet_id,
            "worker_branch_worktree_integration_allowed": False,
            "branch_worktree_merge_integration_parent_owned": True,
            "writable_closure_only": True,
        },
    }


def render_workspace_issue_handoff(
    *,
    repository_root: str | Path,
    issue_ledger_path: str | Path,
    packet_id: str,
) -> dict[str, Any] | None:
    """Return one authenticated, nonaccepting workspace-issue handoff."""
    ledger_path = Path(issue_ledger_path)
    try:
        issue_data = load_ledger(ledger_path)
    except (OSError, ValueError) as exc:
        raise WorkspacePacketHandoffError(
            f"workspace issue ledger is unavailable or malformed: {exc}"
        ) from exc
    findings = validate_ledger(issue_data)
    if findings:
        raise WorkspacePacketHandoffError(
            "invalid workspace issue ledger: "
            + "; ".join(f"{row.path}: {row.message}" for row in findings)
        )
    packet = find_work_packet(issue_data, packet_id)
    if packet is None:
        return None
    reservations = issue_data.get("reservations", [])
    reservation_id = packet.get("reservation_id")
    matches = (
        [
            row
            for row in reservations
            if isinstance(row, Mapping)
            and row.get("id") == reservation_id
            and row.get("packet_id") == packet_id
            and row.get("state") == "active"
        ]
        if isinstance(reservations, list)
        else []
    )
    if len(matches) != 1:
        raise WorkspacePacketHandoffError(
            f"issue work packet {packet_id} requires exactly one active reservation"
        )
    compact = _compact_reserved_packet(
        packet_id,
        packet,
        matches[0],
        repository_root=Path(repository_root).resolve(),
    )
    return {
        "issue_ledger_path": str(ledger_path.resolve()),
        "issue_ledger_revision": issue_data.get("revision"),
        "issue_id": compact["issue_id"],
        "work_item": compact,
    }


__all__ = [
    "WorkspacePacketHandoffError",
    "render_workspace_issue_handoff",
]
