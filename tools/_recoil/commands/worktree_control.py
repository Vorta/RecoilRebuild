"""Parent/orchestrator commands for workspace-issue linked worktrees."""

from __future__ import annotations

import argparse
from contextlib import redirect_stderr, redirect_stdout
import io
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
from typing import Any, Mapping, Sequence

from _recoil.commands.workspace_issues import (
    DEFAULT_LEDGER,
    find_work_packet,
    load_ledger,
    reserve_issue_work_item,
)
from _recoil.commands.workspace_packet_handoff import (
    WorkspacePacketHandoffError,
    render_workspace_issue_handoff,
)
from _recoil.lib.git_change_control import capture_git_closeout
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH
from _recoil.lib.tooling import REPO_ROOT, configure_stdio
from _recoil.lib.worktree_control import (
    PROGRESS_ADAPTER_REASON,
    PROGRESS_ADAPTER_STATE,
    WorktreeAssociation,
    WorktreeControlError,
    authenticate_temporary_build_root,
    authenticate_build_root,
    branch_names,
    canonical_validation_environment,
    capture_packet_git_closeout,
    common_git_directory,
    create_build_root,
    create_linked_worktree,
    create_temporary_build_root,
    derive_packet_locations,
    git_text,
    hygiene_findings,
    is_ancestor,
    remove_authenticated_build_root,
    remove_linked_worktree,
    remove_temporary_build_root,
    reauthenticate_canonical_control_root,
    resolve_canonical_control_root,
    require_clean_worktree,
    resolve_exact_packet_worktree,
    resolve_topology,
    run_absolute_path_independence_probe,
)


def _active_issue_rows(data: Mapping[str, Any]) -> tuple[list[Mapping[str, Any]], list[Mapping[str, Any]]]:
    packets = [
        row for row in data.get("work_packets", [])
        if isinstance(row, Mapping) and row.get("state") in {"ready", "active"}
    ]
    reservations = [
        row for row in data.get("reservations", [])
        if isinstance(row, Mapping) and row.get("state") == "active"
    ]
    return packets, reservations


def _status_projection(
    *, repository_root: Path, ledger_path: Path,
) -> dict[str, object]:
    topology = resolve_topology(repository_root)
    issue_data = load_ledger(ledger_path)
    packets, reservations = _active_issue_rows(issue_data)
    findings = hygiene_findings(
        topology, issue_packets=packets, issue_reservations=reservations
    )
    rows: list[dict[str, object]] = []
    for worktree in topology.worktrees:
        row = worktree.to_dict()
        association = worktree.association
        packet = next(
            (item for item in packets if association and item.get("id") == association.packet_id),
            None,
        )
        reservation = next(
            (item for item in reservations if association and item.get("packet_id") == association.packet_id),
            None,
        )
        row.update({
            "associated_packet_state": packet.get("state") if packet else None,
            "associated_active_reservation": reservation.get("id") if reservation else None,
            "external_build_root": (
                association.external_build_root if association else None
            ),
        })
        rows.append(row)
    return {
        "integration_root": str(topology.integration_root),
        "common_git_directory": str(topology.common_git_directory),
        "master_worktree": next(
            (str(row.root) for row in topology.worktrees if row.branch == "master"),
            None,
        ),
        "worktree_parent": str(topology.worktree_parent),
        "build_parent": str(topology.build_parent),
        "worktrees": rows,
        "branch_hygiene_findings": findings,
        "progress_adapter": {
            "state": PROGRESS_ADAPTER_STATE,
            "reason": PROGRESS_ADAPTER_REASON,
        },
        "nonaccepting": True,
    }


def command_status(args: argparse.Namespace) -> int:
    try:
        result = _status_projection(
            repository_root=REPO_ROOT, ledger_path=Path(args.ledger)
        )
        print(json.dumps(result, indent=2) if args.json else _human_status(result))
        return 0
    except (OSError, ValueError, WorktreeControlError) as exc:
        print(f"recoil workspace worktree status: {exc}", file=sys.stderr)
        return 2


def _human_status(result: Mapping[str, object]) -> str:
    lines = [
        f"Integration root: {result['integration_root']}",
        f"Common Git directory: {result['common_git_directory']}",
        f"Worktree parent: {result['worktree_parent']}",
        f"Build parent: {result['build_parent']}",
        f"Progress adapter: {PROGRESS_ADAPTER_STATE} ({PROGRESS_ADAPTER_REASON})",
        f"Hygiene findings: {len(result['branch_hygiene_findings'])}",
    ]
    return "\n".join(lines)


def _safe_create_compensation(
    *,
    topology,
    branch: str,
    worktree_root: Path,
    build_root: Path,
    association: WorktreeAssociation,
    build_created: bool,
    worktree_created: bool,
) -> list[str]:
    errors: list[str] = []
    if build_created and build_root.exists():
        try:
            marker = authenticate_build_root(
                build_root, authority=association.authority, packet_id=association.packet_id
            )
            if set(path.name for path in build_root.iterdir()) != {".recoil-packet-build-root.json"}:
                raise WorktreeControlError("reservation compensation found unexpected build-root content")
            remove_authenticated_build_root(
                build_root, authority=association.authority, packet_id=association.packet_id
            )
        except (OSError, WorktreeControlError) as exc:
            errors.append(f"build-root compensation: {exc}")
    if worktree_created and worktree_root.exists():
        try:
            remove_linked_worktree(
                topology.integration_root,
                worktree_root=worktree_root,
                branch=branch,
            )
        except (OSError, WorktreeControlError) as exc:
            errors.append(f"worktree compensation: {exc}")
    return errors


def create_issue_packet_worktree(
    *,
    repository_root: Path,
    ledger_path: Path,
    progress_path: Path,
    packet_id: str,
    expected_revision: int,
    apply: bool,
) -> dict[str, object]:
    if not apply:
        raise WorktreeControlError("worktree creation requires explicit --apply")
    topology = resolve_topology(repository_root)
    masters = [row for row in topology.worktrees if row.branch == "master"]
    if len(masters) != 1 or masters[0].root != topology.integration_root:
        raise WorktreeControlError("canonical integration root must check out master")
    require_clean_worktree(topology.integration_root)
    issue_data = load_ledger(ledger_path)
    if issue_data.get("revision") != expected_revision:
        raise WorktreeControlError(
            f"issue revision changed: expected {expected_revision}, found {issue_data.get('revision')}"
        )
    packet = find_work_packet(issue_data, packet_id)
    if packet is None or packet.get("state") != "ready" or packet.get("reservation_id") is not None:
        raise WorktreeControlError("worktree create requires one exact unreserved ready issue packet")
    branch, worktree_root, build_root = derive_packet_locations(
        topology, authority="issue", packet_id=packet_id, revision=expected_revision
    )
    if build_root.exists():
        raise WorktreeControlError(f"packet build root already exists: {build_root}")
    association = WorktreeAssociation("issue", packet_id, str(build_root))
    worktree_created = False
    build_created = False
    reservation_applied = False
    try:
        create_linked_worktree(
            topology,
            branch=branch,
            worktree_root=worktree_root,
            start_point="master",
            association=association,
        )
        worktree_created = True
        create_build_root(
            build_root,
            authority="issue",
            packet_id=packet_id,
            branch=branch,
            worktree_root=worktree_root,
        )
        build_created = True
        reservation = reserve_issue_work_item(
            ledger_path=ledger_path,
            progress_path=progress_path,
            packet_id=packet_id,
            expected_revision=expected_revision,
            apply=True,
            repository_root=worktree_root,
        )
        reservation_applied = True
        handoff = render_workspace_issue_handoff(
            repository_root=topology.integration_root,
            issue_ledger_path=ledger_path,
            packet_id=packet_id,
        )
        if handoff is None:
            raise WorktreeControlError("active packet handoff disappeared after reservation")
        return {
            "applied": True,
            "authority": "issue",
            "packet_id": packet_id,
            "branch": branch,
            "worktree_root": str(worktree_root),
            "external_build_root": str(build_root),
            "association": association.to_dict(),
            "reservation": reservation,
            "handoff": handoff,
            "progress_adapter": {
                "state": PROGRESS_ADAPTER_STATE,
                "reason": PROGRESS_ADAPTER_REASON,
            },
            "nonaccepting": True,
        }
    except Exception as exc:
        if reservation_applied:
            raise WorktreeControlError(
                "worktree creation failed after the issue reservation committed; "
                "the active reservation, associated worktree, branch, and build root "
                f"were retained for governed recovery: {exc}"
            ) from exc
        errors = _safe_create_compensation(
            topology=topology,
            branch=branch,
            worktree_root=worktree_root,
            build_root=build_root,
            association=association,
            build_created=build_created,
            worktree_created=worktree_created,
        )
        suffix = "" if not errors else "; compensation defects: " + "; ".join(errors)
        raise WorktreeControlError(f"worktree creation/reservation failed: {exc}{suffix}") from exc


def command_create(args: argparse.Namespace) -> int:
    try:
        if args.authority != "issue":
            raise WorktreeControlError(
                f"progress worktree adapter is {PROGRESS_ADAPTER_STATE}: {PROGRESS_ADAPTER_REASON}"
            )
        result = create_issue_packet_worktree(
            repository_root=REPO_ROOT,
            ledger_path=Path(args.ledger).resolve(),
            progress_path=Path(args.progress).resolve(),
            packet_id=args.id,
            expected_revision=args.expected_revision,
            apply=bool(args.apply),
        )
        print(json.dumps(result, indent=2))
        return 0
    except (OSError, ValueError, WorktreeControlError, WorkspacePacketHandoffError) as exc:
        print(f"recoil workspace worktree create: {exc}", file=sys.stderr)
        return 2


def validate_issue_packet_worktree(
    *, repository_root: Path, ledger_path: Path, packet_id: str,
) -> dict[str, object]:
    issue_data = load_ledger(ledger_path)
    packet = find_work_packet(issue_data, packet_id)
    if packet is None or packet.get("state") != "active":
        raise WorktreeControlError("worktree validate requires an active issue packet")
    reservations = [
        row for row in issue_data.get("reservations", [])
        if isinstance(row, Mapping)
        and row.get("id") == packet.get("reservation_id")
        and row.get("packet_id") == packet_id
        and row.get("state") == "active"
    ]
    if len(reservations) != 1:
        raise WorktreeControlError("worktree validate requires one exact active reservation")
    descriptor = reservations[0].get("git_workspace_baseline")
    if not isinstance(descriptor, Mapping):
        raise WorktreeControlError("active reservation has no Git baseline")
    writable = packet.get("allowed_paths", [])
    packet_root, association = resolve_exact_packet_worktree(
        repository_root,
        descriptor,
        packet_id=packet_id,
        writable_paths=writable,
    )
    if association is None:
        raise WorktreeControlError("linked-worktree validation requires an associated packet worktree")
    marker = authenticate_build_root(
        association.external_build_root,
        authority="issue",
        packet_id=packet_id,
        branch=str(descriptor.get("branch")),
        worktree_root=packet_root,
    )
    require_clean_worktree(packet_root)
    baseline = str(descriptor.get("baseline_commit"))
    head = git_text(packet_root, "rev-parse", "HEAD").strip()
    if head == baseline or not is_ancestor(packet_root, baseline, head):
        raise WorktreeControlError("packet branch lacks one descendant worker commit")
    commits = [row for row in git_text(packet_root, "rev-list", "--reverse", f"{baseline}..{head}").splitlines() if row]
    if len(commits) != 1:
        raise WorktreeControlError("version 1 requires exactly one worker packet commit")
    subject_body = git_text(packet_root, "show", "-s", "--format=%B", head)
    if packet_id not in subject_body:
        raise WorktreeControlError("worker packet commit message lacks the exact packet ID")
    postflight = capture_git_closeout(
        packet_root,
        descriptor,
        packet_id=packet_id,
        writable_paths=writable,
    )
    if postflight.get("passed") is not True:
        raise WorktreeControlError(
            "packet commit violates writable closure: "
            + ", ".join(postflight.get("unexpected_paths", []))
        )
    return {
        "passed": True,
        "packet_id": packet_id,
        "reservation_id": reservations[0].get("id"),
        "baseline_commit": baseline,
        "head": head,
        "commit_range": commits,
        "branch": descriptor.get("branch"),
        "worktree_root": str(packet_root),
        "external_build_root": association.external_build_root,
        "build_root_marker": marker,
        "changed_paths": postflight.get("changed_paths", []),
        "git_path_postflight": postflight,
        "worker_result_nonaccepting": True,
        "acceptance_eligible": False,
    }


def command_validate(args: argparse.Namespace) -> int:
    try:
        result = validate_issue_packet_worktree(
            repository_root=REPO_ROOT,
            ledger_path=Path(args.ledger),
            packet_id=args.id,
        )
        if args.absolute_path_probe:
            result["absolute_path_probe"] = run_absolute_path_independence_probe(
                REPO_ROOT, commit=str(result["head"])
            )
            if result["absolute_path_probe"].get("passed") is not True:
                raise WorktreeControlError("absolute-path governed VC5 facts diverged")
        print(json.dumps(result, indent=2))
        return 0
    except (OSError, ValueError, WorktreeControlError) as exc:
        print(f"recoil workspace worktree validate: {exc}", file=sys.stderr)
        return 2


_FORBIDDEN_COMMAND_COMPOSITION = ("\r", "\n", "&", "|", ";", ">", "<", "`", "$(")


def _validation_command_tokens(
    command: str,
    *,
    require_public_route: bool,
    resource_claims: Sequence[Mapping[str, Any] | str] = (),
) -> list[str]:
    """Authenticate one exact nonaccepting validation command without a shell."""

    if any(token in command for token in _FORBIDDEN_COMMAND_COMPOSITION):
        raise WorktreeControlError("validation command forbids shell composition")
    try:
        tokens = shlex.split(command, posix=True)
    except ValueError as exc:
        raise WorktreeControlError(f"validation command cannot be parsed: {exc}") from exc
    if not tokens or tokens[0].casefold() not in {"python", "python.exe"}:
        raise WorktreeControlError("validation command must use repository Python")
    cursor = 1
    if cursor < len(tokens) and tokens[cursor] == "-B":
        cursor += 1
    public_route = (
        cursor < len(tokens)
        and tokens[cursor].replace("\\", "/").casefold() == "tools/recoil.py"
    )
    if public_route:
        public = tokens[cursor + 1 :]
        try:
            import recoil as public_registry

            public_registry.validate_nonmutating_public_command(
                public, resource_claims=resource_claims
            )
        except Exception as exc:
            raise WorktreeControlError(
                f"validation command is not an authenticated public route: {exc}"
            ) from exc
    elif require_public_route:
        raise WorktreeControlError(
            "additional integration validation must invoke tools/recoil.py"
        )
    elif not (
        cursor + 1 < len(tokens)
        and tokens[cursor] == "-m"
        and tokens[cursor + 1] == "unittest"
    ):
        raise WorktreeControlError(
            "stored validation must use tools/recoil.py or python -m unittest"
        )
    return tokens


def _run_validation(
    command: str,
    root: Path,
    *,
    require_public_route: bool = False,
    resource_claims: Sequence[Mapping[str, Any] | str] = (),
    environment: Mapping[str, str] | None = None,
) -> dict[str, object]:
    tokens = _validation_command_tokens(
        command,
        require_public_route=require_public_route,
        resource_claims=resource_claims,
    )
    completed = subprocess.run(
        tokens,
        cwd=root,
        shell=False,
        check=False,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        env=(dict(environment) if environment is not None else None),
    )
    result: dict[str, object] = {
        "command": command,
        "cwd": str(root),
        "returncode": completed.returncode,
        "stdout": completed.stdout,
        "stderr": completed.stderr,
        "passed": completed.returncode == 0,
    }
    if environment is not None:
        result["root_routing"] = {
            "canonical_control_root": environment.get("RECOIL_CANONICAL_ROOT"),
            "execution_worktree_root": environment.get(
                "RECOIL_EXECUTION_WORKTREE_ROOT"
            ),
            "external_build_root": environment.get("RECOIL_EXTERNAL_BUILD_ROOT"),
        }
    return result


def _run_authenticated_validation(
    command: str,
    root: Path,
    *,
    canonical_resolution,
    external_build_root: Path,
    external_build_identity,
    require_public_route: bool,
    resource_claims: Sequence[Mapping[str, Any] | str],
    environment: Mapping[str, str],
) -> dict[str, object]:
    """Reauthenticate every routed input immediately around one child."""

    reauthenticate_canonical_control_root(canonical_resolution)
    authenticate_temporary_build_root(
        external_build_root,
        expected_identity=external_build_identity,
    )
    result = _run_validation(
        command,
        root,
        require_public_route=require_public_route,
        resource_claims=resource_claims,
        environment=environment,
    )
    reauthenticate_canonical_control_root(canonical_resolution)
    authenticate_temporary_build_root(
        external_build_root,
        expected_identity=external_build_identity,
    )
    return result


def _require_named_clean_tip(root: Path, *, branch: str, tip: str, label: str) -> dict[str, str]:
    observed_branch = git_text(root, "symbolic-ref", "--quiet", "--short", "HEAD").strip()
    if observed_branch != branch:
        raise WorktreeControlError(
            f"{label} branch changed: expected {branch!r}, found {observed_branch!r}"
        )
    observed_head = git_text(root, "rev-parse", "HEAD").strip()
    branch_head = git_text(root, "rev-parse", branch).strip()
    if observed_head != tip or branch_head != tip:
        raise WorktreeControlError(
            f"{label} tip changed: expected {tip}, found HEAD={observed_head}, branch={branch_head}"
        )
    require_clean_worktree(root)
    return {
        "label": label,
        "root": str(root.resolve(strict=True)),
        "branch": observed_branch,
        "head": observed_head,
        "authored_status": "clean",
        "unmerged_index": "empty",
    }


def _reauthenticate_integration_before_fast_forward(
    *,
    expected_common_git_directory: Path,
    master_root: Path,
    master_tip: str,
    packet_root: Path,
    packet_branch: str,
    packet_tip: str,
    integration_root: Path,
    integration_branch: str,
    integration_tip: str,
) -> dict[str, object]:
    """Authenticate every participating worktree immediately before master moves."""
    expected_common = expected_common_git_directory.resolve(strict=True)
    for label, root in (
        ("master", master_root),
        ("packet", packet_root),
        ("integration", integration_root),
    ):
        observed_common = common_git_directory(root).resolve(strict=True)
        if observed_common != expected_common:
            raise WorktreeControlError(
                f"{label} worktree changed Git common-directory identity: "
                f"expected {expected_common}, found {observed_common}"
            )
    return {
        "common_git_directory": str(expected_common),
        "master": _require_named_clean_tip(
            master_root, branch="master", tip=master_tip, label="master"
        ),
        "packet": _require_named_clean_tip(
            packet_root,
            branch=packet_branch,
            tip=packet_tip,
            label="packet",
        ),
        "integration": _require_named_clean_tip(
            integration_root,
            branch=integration_branch,
            tip=integration_tip,
            label="integration",
        ),
    }


def _assert_git_state_after_fast_forward(
    *,
    expected_common_git_directory: Path,
    master_root: Path,
    packet_root: Path,
    packet_branch: str,
    packet_tip: str,
    integration_root: Path,
    integration_branch: str,
    integration_tip: str,
) -> dict[str, object]:
    """Run deterministic Git assertions only after master has advanced."""
    expected_common = expected_common_git_directory.resolve(strict=True)
    common_receipt: dict[str, str] = {}
    for label, root in (
        ("master", master_root),
        ("packet", packet_root),
        ("integration", integration_root),
    ):
        observed_common = common_git_directory(root).resolve(strict=True)
        if observed_common != expected_common:
            raise WorktreeControlError(
                f"{label} worktree changed Git common-directory identity after "
                f"master fast-forward: expected {expected_common}, found {observed_common}"
            )
        common_receipt[label] = str(observed_common)
    return {
        "expected_common_git_directory": str(expected_common),
        "common_git_directories": common_receipt,
        "master": _require_named_clean_tip(
            master_root,
            branch="master",
            tip=integration_tip,
            label="master-post-fast-forward",
        ),
        "packet": _require_named_clean_tip(
            packet_root,
            branch=packet_branch,
            tip=packet_tip,
            label="packet-post-fast-forward",
        ),
        "integration": _require_named_clean_tip(
            integration_root,
            branch=integration_branch,
            tip=integration_tip,
            label="integration-post-fast-forward",
        ),
        "validation_subprocesses_after_fast_forward": 0,
    }


def integrate_issue_packet_worktree(
    *, repository_root: Path, ledger_path: Path, packet_id: str,
    additional_commands: Sequence[str] = (), apply: bool,
) -> dict[str, object]:
    if not apply:
        raise WorktreeControlError("worktree integration requires explicit --apply")
    validated = validate_issue_packet_worktree(
        repository_root=repository_root, ledger_path=ledger_path, packet_id=packet_id
    )
    topology = resolve_topology(repository_root)
    masters = [row for row in topology.worktrees if row.branch == "master"]
    if len(masters) != 1:
        raise WorktreeControlError("integration requires one canonical master worktree")
    master_root = masters[0].root
    require_clean_worktree(master_root)
    issue_data = load_ledger(ledger_path)
    packet = find_work_packet(issue_data, packet_id)
    if packet is None:
        raise WorktreeControlError("active packet disappeared before integration")
    packet_commands = list(packet.get("validation_commands", []))
    parent_commands = list(additional_commands)
    if not packet_commands and not parent_commands:
        raise WorktreeControlError("integration requires at least one registered validation command")
    resource_claims = list(packet.get("resource_claims", []))
    # Authenticate every command before creating a temporary branch or worktree.
    for command in packet_commands:
        _validation_command_tokens(
            command,
            require_public_route=False,
            resource_claims=resource_claims,
        )
    for command in parent_commands:
        _validation_command_tokens(
            command,
            require_public_route=True,
            resource_claims=resource_claims,
        )
    issue_revision = int(issue_data.get("revision", 0))
    slug = packet_id.split(":")[-1].replace("_", "-")
    integration_branch = f"integration/recoil-worktree/{slug}-r{issue_revision}"
    integration_root = topology.worktree_parent / f"integration-{slug}-r{issue_revision}"
    integration_build_root = (
        topology.build_parent / f"integration-{slug}-r{issue_revision}"
    )
    if (
        integration_branch in branch_names(master_root)
        or integration_root.exists()
        or integration_build_root.exists()
    ):
        raise WorktreeControlError(
            "temporary integration branch/worktree/build root already exists"
        )
    master_before = git_text(master_root, "rev-parse", "master").strip()
    common_before = common_git_directory(master_root).resolve(strict=True)
    packet_root = Path(str(validated["worktree_root"])).resolve(strict=True)
    packet_branch = str(validated["branch"])
    packet_tip = str(validated["head"])
    created = False
    integration_build_identity = None
    canonical_resolution = None
    try:
        subprocess.run(
            [
                "git", "worktree", "add", "-b", integration_branch,
                str(integration_root), master_before,
            ],
            cwd=master_root, check=True, capture_output=True, text=True,
        )
        created = True
        subprocess.run(
            ["git", "worktree", "lock", str(integration_root), "--reason", f"recoil-integration-worktree-v1|{packet_id}"],
            cwd=master_root, check=True, capture_output=True, text=True,
        )
        integration_build_identity = create_temporary_build_root(
            integration_build_root
        )
        canonical_resolution = resolve_canonical_control_root(
            executing_worktree_root=integration_root,
            required_machine_local_paths=(),
            explicit_root=master_root,
        )
        validation_environment = canonical_validation_environment(
            canonical_resolution,
            external_build_root=integration_build_root,
            expected_external_build_root_identity=integration_build_identity,
        )
        merge = subprocess.run(
            ["git", "merge", "--no-ff", "--no-commit", "--no-edit", packet_tip],
            cwd=integration_root, check=False, capture_output=True, text=True,
        )
        if merge.returncode != 0:
            subprocess.run(["git", "merge", "--abort"], cwd=integration_root, check=False, capture_output=True)
            raise WorktreeControlError(
                "temporary integration merge failed: " + (merge.stderr or merge.stdout).strip()
            )
        integration_results = [
            _run_authenticated_validation(
                command,
                integration_root,
                canonical_resolution=canonical_resolution,
                external_build_root=integration_build_root,
                external_build_identity=integration_build_identity,
                require_public_route=False,
                resource_claims=resource_claims,
                environment=validation_environment,
            )
            for command in packet_commands
        ] + [
            _run_authenticated_validation(
                command,
                integration_root,
                canonical_resolution=canonical_resolution,
                external_build_root=integration_build_root,
                external_build_identity=integration_build_identity,
                require_public_route=True,
                resource_claims=resource_claims,
                environment=validation_environment,
            )
            for command in parent_commands
        ]
        if not all(row["passed"] for row in integration_results):
            subprocess.run(
                ["git", "merge", "--abort"], cwd=integration_root,
                check=False, capture_output=True,
            )
            raise WorktreeControlError("fresh integration-worktree validation failed")
        committed = subprocess.run(
            ["git", "commit", "--no-edit"], cwd=integration_root,
            check=False, capture_output=True, text=True,
        )
        if committed.returncode != 0:
            subprocess.run(
                ["git", "merge", "--abort"], cwd=integration_root,
                check=False, capture_output=True,
            )
            raise WorktreeControlError(
                "temporary integration commit failed: "
                + (committed.stderr or committed.stdout).strip()
            )
        integration_head = git_text(integration_root, "rev-parse", "HEAD").strip()
        post_results = [
            _run_authenticated_validation(
                command,
                integration_root,
                canonical_resolution=canonical_resolution,
                external_build_root=integration_build_root,
                external_build_identity=integration_build_identity,
                require_public_route=False,
                resource_claims=resource_claims,
                environment=validation_environment,
            )
            for command in packet_commands
        ] + [
            _run_authenticated_validation(
                command,
                integration_root,
                canonical_resolution=canonical_resolution,
                external_build_root=integration_build_root,
                external_build_identity=integration_build_identity,
                require_public_route=True,
                resource_claims=resource_claims,
                environment=validation_environment,
            )
            for command in parent_commands
        ]
        if not all(row["passed"] for row in post_results):
            raise WorktreeControlError(
                "post-integration validation failed before master fast-forward"
            )
        reauthenticate_canonical_control_root(canonical_resolution)
        authenticate_temporary_build_root(
            integration_build_root,
            expected_identity=integration_build_identity,
        )
        pre_fast_forward = _reauthenticate_integration_before_fast_forward(
            expected_common_git_directory=common_before,
            master_root=master_root,
            master_tip=master_before,
            packet_root=packet_root,
            packet_branch=packet_branch,
            packet_tip=packet_tip,
            integration_root=integration_root,
            integration_branch=integration_branch,
            integration_tip=integration_head,
        )
        ff = subprocess.run(
            ["git", "merge", "--ff-only", integration_head],
            cwd=master_root, check=False, capture_output=True, text=True,
        )
        if ff.returncode != 0:
            _require_named_clean_tip(
                master_root,
                branch="master",
                tip=master_before,
                label="master-after-failed-fast-forward",
            )
            raise WorktreeControlError(
                "master fast-forward failed: " + (ff.stderr or ff.stdout).strip()
            )
        post_fast_forward = _assert_git_state_after_fast_forward(
            expected_common_git_directory=common_before,
            master_root=master_root,
            packet_root=packet_root,
            packet_branch=packet_branch,
            packet_tip=packet_tip,
            integration_root=integration_root,
            integration_branch=integration_branch,
            integration_tip=integration_head,
        )
        reauthenticate_canonical_control_root(canonical_resolution)
        authenticate_temporary_build_root(
            integration_build_root,
            expected_identity=integration_build_identity,
        )
        post_fast_forward["canonical_control_root_identity"] = "unchanged"
        post_fast_forward["external_build_root_identity"] = "unchanged"
        return {
            "passed": True,
            "applied": True,
            "packet_id": packet_id,
            "master_before": master_before,
            "master_after": git_text(master_root, "rev-parse", "master").strip(),
            "integration_branch": integration_branch,
            "integration_head": integration_head,
            "captured_opaque_tips": {
                "master_input": master_before,
                "packet_input": packet_tip,
                "integration_validated": integration_head,
                "master_fast_forward_operand": integration_head,
            },
            "integration_validation": integration_results,
            "post_integration_validation": post_results,
            "pre_fast_forward_reauthentication": pre_fast_forward,
            "post_fast_forward_git_assertions": post_fast_forward,
            "validation_execution_phase": "pre-fast-forward-integration-worktree",
            "validation_completed_before_master_advance": True,
            "root_routing": {
                **canonical_resolution.to_dict(),
                "external_build_root": str(integration_build_root.resolve(strict=True)),
            },
            "worker_result_nonaccepting": True,
            "master_integration_is_reconstruction_acceptance": False,
        }
    finally:
        cleanup_errors: list[str] = []
        active_error = sys.exc_info()[1]
        if created and integration_root.exists():
            subprocess.run(["git", "merge", "--abort"], cwd=integration_root, check=False, capture_output=True)
            detached = subprocess.run(
                ["git", "switch", "--detach"],
                cwd=integration_root,
                check=False,
                capture_output=True,
                text=True,
            )
            if detached.returncode != 0:
                cleanup_errors.append(
                    "temporary integration detach failed: "
                    + (detached.stderr or detached.stdout).strip()
                )
            elif integration_branch in branch_names(master_root):
                # The detached worktree HEAD retains the unique integration tip,
                # so ordinary -d is safe even when validation failed before master
                # advanced. No force deletion or history rewrite is needed.
                deleted = subprocess.run(
                    ["git", "branch", "-d", integration_branch],
                    cwd=integration_root,
                    check=False,
                    capture_output=True,
                    text=True,
                )
                if deleted.returncode != 0:
                    cleanup_errors.append(
                        "temporary integration branch deletion failed: "
                        + (deleted.stderr or deleted.stdout).strip()
                    )
            subprocess.run(["git", "worktree", "unlock", str(integration_root)], cwd=master_root, check=False, capture_output=True)
            removed = subprocess.run(
                ["git", "worktree", "remove", str(integration_root)],
                cwd=master_root,
                check=False,
                capture_output=True,
                text=True,
            )
            if removed.returncode != 0 or integration_root.exists():
                cleanup_errors.append(
                    "temporary integration worktree removal failed: "
                    + (removed.stderr or removed.stdout).strip()
                )
        if integration_branch in branch_names(master_root):
            cleanup_errors.append(
                "temporary integration branch remains after detached safe deletion: "
                + integration_branch
            )
        if integration_build_identity is not None:
            try:
                if not os.path.lexists(integration_build_root):
                    raise WorktreeControlError(
                        "temporary integration build root disappeared before cleanup: "
                        + str(integration_build_root)
                    )
                remove_temporary_build_root(
                    integration_build_root,
                    expected_identity=integration_build_identity,
                )
            except WorktreeControlError as exc:
                cleanup_errors.append(
                    "temporary integration build-root removal failed: " + str(exc)
                )
        if cleanup_errors:
            prefix = f"{active_error}; " if active_error is not None else ""
            raise WorktreeControlError(
                prefix + "temporary integration cleanup defect: " + "; ".join(cleanup_errors)
            )


def command_integrate(args: argparse.Namespace) -> int:
    try:
        result = integrate_issue_packet_worktree(
            repository_root=REPO_ROOT,
            ledger_path=Path(args.ledger),
            packet_id=args.id,
            additional_commands=args.validation_command,
            apply=bool(args.apply),
        )
        print(json.dumps(result, indent=2))
        return 0
    except (OSError, ValueError, WorktreeControlError, subprocess.SubprocessError) as exc:
        print(f"recoil workspace worktree integrate: {exc}", file=sys.stderr)
        return 2


def _retirement_status_receipt(
    *, repository_root: Path, ledger_path: Path,
) -> dict[str, object]:
    try:
        projection = _status_projection(
            repository_root=repository_root, ledger_path=ledger_path
        )
        return {
            "remaining_topology": list(projection["worktrees"]),
            "remaining_status": {
                "integration_root": projection["integration_root"],
                "common_git_directory": projection["common_git_directory"],
                "branches": sorted(branch_names(Path(str(projection["integration_root"])))),
            },
            "hygiene_findings": list(projection["branch_hygiene_findings"]),
        }
    except (OSError, ValueError, WorktreeControlError) as exc:
        finding = {"kind": "retirement-hygiene-unavailable", "detail": str(exc)}
        return {
            "remaining_topology": [],
            "remaining_status": {"unavailable": str(exc)},
            "hygiene_findings": [finding],
        }


def _retirement_failure(
    *,
    repository_root: Path,
    ledger_path: Path,
    packet_id: str,
    outcome: str,
    completed_steps: Sequence[str],
    failed_step: str,
    detail: str,
    archive_tag: str | None,
    expected_branch_tip: str,
) -> dict[str, object]:
    completed = set(completed_steps)
    receipt = _retirement_status_receipt(
        repository_root=repository_root, ledger_path=ledger_path
    )
    partial = bool(completed)
    return {
        "passed": False,
        "applied": True,
        "partial": partial,
        "partial_retirement": partial,
        "packet_id": packet_id,
        "outcome": outcome,
        "expected_branch_tip": expected_branch_tip,
        "completed_steps": list(completed_steps),
        "failed_step": failed_step,
        "first_failure": detail,
        "archive_tag": archive_tag,
        "archive_tag_created": any(
            step.startswith("annotated-archive-tag-created") for step in completed
        ),
        "archive_tag_verified": "annotated-archive-tag-created" in completed,
        "worktree_removed": "worktree-removed" in completed,
        "branch_removed": "branch-deleted" in completed,
        "build_root_removed": "build-root-removed" in completed,
        "remaining_topology": receipt["remaining_topology"],
        "remaining_status": receipt["remaining_status"],
        "hygiene_findings": receipt["hygiene_findings"],
        "hygiene_expected_to_detect_partial_state": partial,
        "nonaccepting": True,
    }


def _verify_commit_tip(root: Path, expected_tip: str) -> str:
    if not expected_tip or any(character in expected_tip for character in "\r\n\0"):
        raise WorktreeControlError("retirement expected tip is unavailable or malformed")
    resolved = git_text(root, "rev-parse", "--verify", f"{expected_tip}^{{commit}}").strip()
    if resolved != expected_tip:
        raise WorktreeControlError(
            f"retirement expected tip must be the exact opaque commit ID: {resolved}"
        )
    return resolved


def _validate_abandonment_tag_name(*, root: Path, tag: str) -> None:
    if not tag.startswith("archive/packet/") or tag.endswith("/"):
        raise WorktreeControlError(
            "abandoned packet tag must use the archive/packet/ namespace"
        )
    valid = subprocess.run(
        ["git", "check-ref-format", f"refs/tags/{tag}"],
        cwd=root,
        check=False,
        capture_output=True,
        text=True,
    )
    if valid.returncode != 0:
        raise WorktreeControlError("abandoned packet archive tag is malformed")
    existing = subprocess.run(
        ["git", "show-ref", "--verify", "--quiet", f"refs/tags/{tag}"],
        cwd=root,
        check=False,
        capture_output=True,
        text=True,
    )
    if existing.returncode not in {0, 1}:
        raise WorktreeControlError("cannot inspect abandoned packet archive tag")
    if existing.returncode == 0:
        raise WorktreeControlError(f"abandoned packet archive tag already exists: {tag}")


def _create_abandonment_tag(
    *, root: Path, tag: str, packet_id: str, tip: str, reason: str,
) -> None:
    _validate_abandonment_tag_name(root=root, tag=tag)
    message = f"Archived abandoned workspace packet {packet_id}.\n\nReason: {reason}"
    created = subprocess.run(
        ["git", "tag", "-a", tag, tip, "-m", message],
        cwd=root,
        check=False,
        capture_output=True,
        text=True,
    )
    if created.returncode != 0:
        raise WorktreeControlError(
            "abandoned packet archive tag creation failed: "
            + (created.stderr or created.stdout).strip()
        )
    if git_text(root, "cat-file", "-t", tag).strip() != "tag":
        raise WorktreeControlError("abandoned packet archive reference is not annotated")
    if git_text(root, "rev-parse", f"{tag}^{{commit}}").strip() != tip:
        raise WorktreeControlError("abandoned packet archive tag target changed")


def retire_issue_packet_worktree(
    *,
    repository_root: Path,
    ledger_path: Path,
    packet_id: str,
    apply: bool,
    outcome: str = "integrated",
    reason: str | None = None,
    expected_tip: str | None = None,
    archive_tag: str | None = None,
    parent_reviewed_abandonment: bool = False,
    discard_unmerged_without_tag: bool = False,
    discard_confirmation: str | None = None,
) -> dict[str, object]:
    if not apply:
        raise WorktreeControlError("worktree retirement requires explicit --apply")
    topology = resolve_topology(repository_root)
    issue_data = load_ledger(ledger_path)
    packets, reservations = _active_issue_rows(issue_data)
    if any(row.get("id") == packet_id for row in packets) or any(row.get("packet_id") == packet_id for row in reservations):
        raise WorktreeControlError("packet must be terminal with no active reservation before retirement")
    matches = [
        row for row in topology.worktrees
        if row.association is not None and row.association.packet_id == packet_id
    ]
    if len(matches) != 1:
        raise WorktreeControlError("retirement requires one exact associated packet worktree")
    worktree = matches[0]
    association = worktree.association
    assert association is not None
    if association.authority != "issue" or worktree.branch is None:
        raise WorktreeControlError("retirement requires an exact issue packet branch")
    if not worktree.branch.startswith("packet/"):
        raise WorktreeControlError("retirement refuses a non-packet branch")
    if not worktree.locked:
        raise WorktreeControlError("retirement requires the associated packet worktree lock")
    branch_checkouts = [
        row for row in topology.worktrees if row.branch == worktree.branch
    ]
    if len(branch_checkouts) != 1 or branch_checkouts[0].root != worktree.root:
        raise WorktreeControlError(
            "retirement requires exactly one worktree checkout for the packet branch"
        )
    require_clean_worktree(worktree.root)
    branch = worktree.branch
    observed_tip = git_text(worktree.root, "rev-parse", "HEAD").strip()
    branch_tip = git_text(topology.integration_root, "rev-parse", branch).strip()
    if observed_tip != branch_tip:
        raise WorktreeControlError("packet worktree HEAD and branch tip differ")
    if expected_tip is not None:
        _verify_commit_tip(topology.integration_root, expected_tip)
        if expected_tip != observed_tip:
            raise WorktreeControlError(
                f"packet branch tip changed: expected {expected_tip}, found {observed_tip}"
            )
    merged = is_ancestor(topology.integration_root, branch, "master")
    if outcome == "integrated":
        if not merged:
            raise WorktreeControlError("packet branch is not proven merged into master")
        if any((
            reason is not None,
            expected_tip is not None,
            archive_tag is not None,
            parent_reviewed_abandonment,
            discard_unmerged_without_tag,
            discard_confirmation is not None,
        )):
            raise WorktreeControlError(
                "normal merged retirement does not accept abandonment controls"
            )
        force_abandoned_delete = False
    elif outcome == "abandoned-unmerged":
        if merged:
            raise WorktreeControlError(
                "abandoned-unmerged retirement refuses a branch already merged into master"
            )
        if expected_tip is None:
            raise WorktreeControlError(
                "abandoned-unmerged retirement requires the exact expected branch tip"
            )
        reviewed_reason = (reason or "").strip()
        if not reviewed_reason:
            raise WorktreeControlError(
                "abandoned-unmerged retirement requires an explicit parent reason"
            )
        if not parent_reviewed_abandonment:
            raise WorktreeControlError(
                "abandoned-unmerged retirement requires explicit parent-reviewed authorization"
            )
        if (archive_tag is None) == (not discard_unmerged_without_tag):
            raise WorktreeControlError(
                "abandoned-unmerged retirement requires exactly one archive tag or "
                "explicit discard-without-tag selection"
            )
        expected_confirmation = f"discard-unmerged:{packet_id}:{expected_tip}"
        if archive_tag is not None and discard_confirmation is not None:
            raise WorktreeControlError(
                "archive-tag abandonment does not accept discard confirmation"
            )
        if discard_unmerged_without_tag and discard_confirmation != expected_confirmation:
            raise WorktreeControlError(
                "discard-without-tag confirmation does not match packet and expected tip"
            )
        if archive_tag is not None:
            _validate_abandonment_tag_name(
                root=topology.integration_root, tag=archive_tag
            )
        force_abandoned_delete = True
    else:
        raise WorktreeControlError(f"unsupported retirement outcome: {outcome!r}")
    build_root = association.external_build_root
    authenticate_build_root(
        build_root,
        authority="issue",
        packet_id=packet_id,
        branch=branch,
        worktree_root=worktree.root,
    )
    completed_steps: list[str] = []
    if outcome == "abandoned-unmerged" and archive_tag is not None:
        try:
            _create_abandonment_tag(
                root=topology.integration_root,
                tag=archive_tag,
                packet_id=packet_id,
                tip=observed_tip,
                reason=(reason or "").strip(),
            )
            completed_steps.append("annotated-archive-tag-created")
        except (OSError, WorktreeControlError) as exc:
            observed_tag = subprocess.run(
                [
                    "git", "show-ref", "--verify", "--quiet",
                    f"refs/tags/{archive_tag}",
                ],
                cwd=topology.integration_root,
                check=False,
                capture_output=True,
                text=True,
            )
            if observed_tag.returncode == 0:
                completed_steps.append("annotated-archive-tag-created-unverified")
            return _retirement_failure(
                repository_root=repository_root,
                ledger_path=ledger_path,
                packet_id=packet_id,
                outcome=outcome,
                completed_steps=completed_steps,
                failed_step="archive-tag",
                detail=str(exc),
                archive_tag=archive_tag,
                expected_branch_tip=observed_tip,
            )
    unlocked = subprocess.run(
        ["git", "worktree", "unlock", str(worktree.root)],
        cwd=topology.integration_root,
        check=False,
        capture_output=True,
        text=True,
    )
    if unlocked.returncode != 0:
        return _retirement_failure(
            repository_root=repository_root,
            ledger_path=ledger_path,
            packet_id=packet_id,
            outcome=outcome,
            completed_steps=completed_steps,
            failed_step="worktree-unlock",
            detail=(unlocked.stderr or unlocked.stdout).strip(),
            archive_tag=archive_tag,
            expected_branch_tip=observed_tip,
        )
    completed_steps.append("worktree-unlocked")
    removed_worktree = subprocess.run(
        ["git", "worktree", "remove", str(worktree.root)],
        cwd=topology.integration_root,
        check=False,
        capture_output=True,
        text=True,
    )
    if removed_worktree.returncode != 0 or worktree.root.exists():
        return _retirement_failure(
            repository_root=repository_root,
            ledger_path=ledger_path,
            packet_id=packet_id,
            outcome=outcome,
            completed_steps=completed_steps,
            failed_step="worktree-remove",
            detail=(removed_worktree.stderr or removed_worktree.stdout).strip(),
            archive_tag=archive_tag,
            expected_branch_tip=observed_tip,
        )
    completed_steps.append("worktree-removed")
    if force_abandoned_delete:
        # This destructive deletion is reachable only through the fully guarded,
        # exact-tip abandoned-unmerged preflight above.
        assert outcome == "abandoned-unmerged" and expected_tip == observed_tip
        assert parent_reviewed_abandonment
        assert archive_tag is not None or (
            discard_unmerged_without_tag
            and discard_confirmation == f"discard-unmerged:{packet_id}:{expected_tip}"
        )
    deleted_branch = subprocess.run(
        ["git", "branch", "-D" if force_abandoned_delete else "-d", branch],
        cwd=topology.integration_root,
        check=False,
        capture_output=True,
        text=True,
    )
    if deleted_branch.returncode != 0 or branch in branch_names(topology.integration_root):
        return _retirement_failure(
            repository_root=repository_root,
            ledger_path=ledger_path,
            packet_id=packet_id,
            outcome=outcome,
            completed_steps=completed_steps,
            failed_step="branch-delete",
            detail=(deleted_branch.stderr or deleted_branch.stdout).strip(),
            archive_tag=archive_tag,
            expected_branch_tip=observed_tip,
        )
    completed_steps.append("branch-deleted")
    try:
        remove_authenticated_build_root(
            build_root, authority="issue", packet_id=packet_id
        )
    except (OSError, WorktreeControlError) as exc:
        return _retirement_failure(
            repository_root=repository_root,
            ledger_path=ledger_path,
            packet_id=packet_id,
            outcome=outcome,
            completed_steps=completed_steps,
            failed_step="build-root-remove",
            detail=str(exc),
            archive_tag=archive_tag,
            expected_branch_tip=observed_tip,
        )
    completed_steps.append("build-root-removed")
    pruned = subprocess.run(
        ["git", "worktree", "prune"],
        cwd=topology.integration_root,
        check=False,
        capture_output=True,
        text=True,
    )
    if pruned.returncode != 0:
        return _retirement_failure(
            repository_root=repository_root,
            ledger_path=ledger_path,
            packet_id=packet_id,
            outcome=outcome,
            completed_steps=completed_steps,
            failed_step="worktree-prune",
            detail=(pruned.stderr or pruned.stdout).strip(),
            archive_tag=archive_tag,
            expected_branch_tip=observed_tip,
        )
    completed_steps.append("worktree-pruned")
    receipt = _retirement_status_receipt(
        repository_root=repository_root, ledger_path=ledger_path
    )
    return {
        "passed": True,
        "applied": True,
        "partial": False,
        "partial_retirement": False,
        "packet_id": packet_id,
        "outcome": outcome,
        "reason": reason,
        "expected_tip": expected_tip,
        "expected_branch_tip": observed_tip,
        "archive_tag": archive_tag,
        "archive_tag_created": archive_tag is not None,
        "archive_tag_verified": archive_tag is not None,
        "completed_steps": completed_steps,
        "branch_removed": True,
        "removed_branch": branch,
        "worktree_removed": True,
        "removed_worktree": str(worktree.root),
        "build_root_removed": True,
        "removed_build_root": build_root,
        "worktree_prune": "completed",
        "remaining_topology": receipt["remaining_topology"],
        "remaining_status": receipt["remaining_status"],
        "hygiene_findings": receipt["hygiene_findings"],
        "hygiene_expected_to_detect_partial_state": False,
        "nonaccepting": True,
    }


def command_retire(args: argparse.Namespace) -> int:
    try:
        result = retire_issue_packet_worktree(
            repository_root=REPO_ROOT,
            ledger_path=Path(args.ledger),
            packet_id=args.id,
            apply=bool(args.apply),
            outcome=args.outcome,
            reason=args.reason,
            expected_tip=args.expected_tip,
            archive_tag=args.archive_tag,
            parent_reviewed_abandonment=bool(args.parent_reviewed_abandonment),
            discard_unmerged_without_tag=bool(args.discard_unmerged_without_tag),
            discard_confirmation=args.confirm_discard_without_tag,
        )
        print(json.dumps(result, indent=2))
        return 0 if result.get("passed") is True else 2
    except (OSError, ValueError, WorktreeControlError) as exc:
        print(f"recoil workspace worktree retire: {exc}", file=sys.stderr)
        return 2


def command_hygiene(args: argparse.Namespace) -> int:
    try:
        result = _status_projection(repository_root=REPO_ROOT, ledger_path=Path(args.ledger))
        result = {**result, "passed": not result["branch_hygiene_findings"]}
        print(json.dumps(result, indent=2) if args.json else _human_status(result))
        if args.strict and not result["passed"]:
            return 1
        return 0
    except (OSError, ValueError, WorktreeControlError) as exc:
        print(f"recoil workspace worktree hygiene: {exc}", file=sys.stderr)
        return 2


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Parent-owned linked worktree lifecycle for workspace-issue packets."
    )
    sub = parser.add_subparsers(dest="command", required=True)
    status = sub.add_parser("status")
    status.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    status.add_argument("--json", action="store_true")
    status.set_defaults(func=command_status)

    create = sub.add_parser("create")
    create.add_argument("--authority", default="issue", choices=("issue", "progress"))
    create.add_argument("--id", required=True)
    create.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    create.add_argument("--progress", type=Path, default=DEFAULT_PROGRESS_PATH)
    create.add_argument("--expected-revision", required=True, type=int)
    create.add_argument("--apply", action="store_true", required=True)
    create.set_defaults(func=command_create)

    validate = sub.add_parser("validate")
    validate.add_argument("--id", required=True)
    validate.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    validate.add_argument("--json", action="store_true")
    validate.add_argument("--absolute-path-probe", action="store_true")
    validate.set_defaults(func=command_validate)

    integrate = sub.add_parser("integrate")
    integrate.add_argument("--id", required=True)
    integrate.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    integrate.add_argument("--validation-command", action="append", default=[])
    integrate.add_argument("--apply", action="store_true", required=True)
    integrate.set_defaults(func=command_integrate)

    retire = sub.add_parser("retire")
    retire.add_argument("--id", required=True)
    retire.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    retire.add_argument(
        "--outcome",
        choices=("integrated", "abandoned-unmerged"),
        default="integrated",
    )
    retire.add_argument("--reason")
    retire.add_argument("--expected-tip")
    retire.add_argument("--archive-tag")
    retire.add_argument("--parent-reviewed-abandonment", action="store_true")
    retire.add_argument("--discard-unmerged-without-tag", action="store_true")
    retire.add_argument("--confirm-discard-without-tag")
    retire.add_argument("--apply", action="store_true", required=True)
    retire.set_defaults(func=command_retire)

    hygiene = sub.add_parser("hygiene")
    hygiene.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    hygiene.add_argument("--strict", action="store_true")
    hygiene.add_argument("--json", action="store_true")
    hygiene.set_defaults(func=command_hygiene)
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
