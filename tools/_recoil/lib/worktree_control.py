"""Orchestrator-owned linked Git worktree control.

This module treats Git commits and checkout paths as workspace provenance only.
It owns no reconstruction truth and deliberately has no progress-packet adapter:
progress packets remain contained-disabled until they carry an immutable Git
baseline of their own.
"""

from __future__ import annotations

from dataclasses import dataclass
from datetime import datetime, timezone
import ast
import json
import os
from pathlib import Path, PurePosixPath
import re
import shlex
import stat
import subprocess
import sys
from typing import Any, Iterable, Mapping, Sequence

from _recoil.lib.git_change_control import (
    GitChangeControlError,
    capture_git_closeout,
    normalize_repository_path,
    validate_git_baseline_descriptor,
)
from _recoil.lib.windows_identity import (
    PhysicalFileIdentity,
    WindowsIdentityError,
    physical_identity,
    require_same_physical_object,
)


class WorktreeControlError(RuntimeError):
    """A worktree topology, association, or ownership contract failed."""


PACKET_WORKTREE_MARKER = "recoil-packet-worktree-v1"
INTEGRATION_WORKTREE_MARKER = "recoil-integration-worktree-v1"
BUILD_ROOT_MARKER_SCHEMA = "recoil-packet-build-root-v1"
BUILD_ROOT_MARKER_NAME = ".recoil-packet-build-root.json"
PROGRESS_ADAPTER_STATE = "contained-disabled"
PROGRESS_ADAPTER_REASON = (
    "progress packets do not yet record a native-Git baseline"
)
_PACKET_BRANCH_PREFIX = "packet/"
_TEMP_INTEGRATION_PREFIX = "integration/recoil-worktree/"
_WINDOWS_SAFE_CHILD_LIMIT = 220
CANONICAL_ROOT_ENV = "RECOIL_CANONICAL_ROOT"
EXECUTION_WORKTREE_ROOT_ENV = "RECOIL_EXECUTION_WORKTREE_ROOT"
EXTERNAL_BUILD_ROOT_ENV = "RECOIL_EXTERNAL_BUILD_ROOT"
FORBIDDEN_VALIDATION_COMMAND_COMPOSITION = (
    "\r",
    "\n",
    "&",
    "|",
    ";",
    ">",
    "<",
    "`",
    "$(",
)


def authenticated_validation_command_tokens(
    command: str,
    *,
    require_public_route: bool,
    resource_claims: Sequence[Mapping[str, Any] | str] = (),
) -> list[str]:
    """Authenticate one exact nonaccepting validation command without a shell.

    Issue-packet construction, handoff validation, and integration consume this
    same registry so a command cannot become invalid only after worker return.
    """

    if not isinstance(command, str) or not command.strip():
        raise WorktreeControlError("validation command must be a non-empty string")
    if any(token in command for token in FORBIDDEN_VALIDATION_COMMAND_COMPOSITION):
        raise WorktreeControlError("validation command forbids shell composition")
    try:
        tokens = shlex.split(command, posix=True)
    except ValueError as exc:
        raise WorktreeControlError(
            f"validation command cannot be parsed: {exc}"
        ) from exc
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
    else:
        lowered = command.casefold()
        if any(
            token in lowered
            for token in (
                "--apply",
                "progress advance-live-",
                "issue work close",
                "issue work reserve",
            )
        ):
            raise WorktreeControlError(
                "validation command exposes a mutating worker operation"
            )
        if not (
            cursor + 2 < len(tokens)
            and tokens[cursor] == "-m"
            and tokens[cursor + 1] == "unittest"
            and any(str(item).strip() for item in tokens[cursor + 2 :])
        ):
            raise WorktreeControlError(
                "stored validation must use tools/recoil.py or python -m unittest"
            )
    return tokens


@dataclass(frozen=True)
class WorktreeAssociation:
    authority: str
    packet_id: str
    external_build_root: str

    def lock_reason(self) -> str:
        if self.authority != "issue" or not self.packet_id:
            raise WorktreeControlError("packet worktree association is not an issue packet")
        build_root = Path(self.external_build_root)
        if not build_root.is_absolute():
            raise WorktreeControlError("packet worktree association build root must be absolute")
        if any("|" in field or "\r" in field or "\n" in field for field in (
            self.authority, self.packet_id, self.external_build_root
        )):
            raise WorktreeControlError("packet worktree association contains a reserved delimiter")
        return "|".join((
            PACKET_WORKTREE_MARKER,
            self.authority,
            self.packet_id,
            self.external_build_root,
        ))

    def to_dict(self) -> dict[str, str]:
        return {
            "authority": self.authority,
            "packet_id": self.packet_id,
            "external_build_root": self.external_build_root,
        }


@dataclass(frozen=True)
class GitWorktree:
    root: Path
    head: str
    branch: str | None
    detached: bool
    locked: bool
    lock_reason: str | None
    prunable: bool
    prunable_reason: str | None
    association: WorktreeAssociation | None

    def to_dict(self) -> dict[str, object]:
        return {
            "root": str(self.root),
            "head": self.head,
            "branch": self.branch,
            "detached": self.detached,
            "locked": self.locked,
            "lock_reason": self.lock_reason,
            "prunable": self.prunable,
            "prunable_reason": self.prunable_reason,
            "association": (
                self.association.to_dict() if self.association is not None else None
            ),
        }


@dataclass(frozen=True)
class WorktreeTopology:
    common_git_directory: Path
    integration_root: Path
    worktree_parent: Path
    build_parent: Path
    worktrees: tuple[GitWorktree, ...]

    def to_dict(self) -> dict[str, object]:
        return {
            "common_git_directory": str(self.common_git_directory),
            "integration_root": str(self.integration_root),
            "worktree_parent": str(self.worktree_parent),
            "build_parent": str(self.build_parent),
            "worktrees": [row.to_dict() for row in self.worktrees],
        }


@dataclass(frozen=True)
class CanonicalControlRoot:
    """Authenticated routing from one executing worktree to shared inputs.

    The absolute roots are bounded diagnostic provenance.  The physical
    identities authenticate the already selected files; neither the path text
    nor an identity value is reconstruction evidence.
    """

    executing_worktree_root: Path
    canonical_control_root: Path
    common_git_directory: Path
    resolution_source: str
    required_machine_local_paths: tuple[str, ...]
    canonical_root_identity: PhysicalFileIdentity
    required_path_identities: tuple[tuple[str, PhysicalFileIdentity], ...]

    def to_dict(self) -> dict[str, object]:
        return {
            "execution_worktree_root": str(self.executing_worktree_root),
            "canonical_control_root": str(self.canonical_control_root),
            "common_git_directory": str(self.common_git_directory),
            "resolution_source": self.resolution_source,
            "required_machine_local_paths": list(self.required_machine_local_paths),
        }


def _run_git(
    root: Path,
    arguments: Sequence[str],
    *,
    check: bool = True,
) -> subprocess.CompletedProcess[str]:
    completed = subprocess.run(
        ["git", *arguments],
        cwd=root,
        check=False,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="surrogateescape",
    )
    if check and completed.returncode != 0:
        detail = (completed.stderr or completed.stdout).strip()
        raise WorktreeControlError(
            f"Git command failed ({' '.join(arguments)}): {detail}"
        )
    return completed


def git_text(root: str | Path, *arguments: str) -> str:
    return _run_git(Path(root).resolve(), arguments).stdout


def _canonical(path: str | Path, *, strict: bool = False) -> Path:
    try:
        return Path(path).resolve(strict=strict)
    except OSError as exc:
        raise WorktreeControlError(f"cannot resolve path {path!s}: {exc}") from exc


def _lexical_absolute(path: str | Path, *, context: str) -> Path:
    """Return an absolute spelling without following a reparse component.

    ``Path.resolve`` is deliberately not used until after the lexical spelling
    has been inspected.  Otherwise a junction/symlink in the supplied path can
    disappear from the spelling before the reparse policy sees it.
    """

    if isinstance(path, str) and not path.strip():
        raise WorktreeControlError(f"{context} is empty")
    try:
        raw = os.fspath(path)
    except TypeError as exc:
        raise WorktreeControlError(f"{context} is not a filesystem path") from exc
    if not raw or "\x00" in raw:
        raise WorktreeControlError(f"{context} is empty or contains NUL")
    lexical = Path(os.path.abspath(raw))
    _reject_reparse(lexical, context=context)
    return lexical


def _is_relative_to(path: Path, parent: Path) -> bool:
    try:
        path.relative_to(parent)
        return True
    except ValueError:
        return False


def _reject_reparse(path: Path, *, context: str) -> None:
    current = path
    while current != current.parent:
        if current.exists() and current.is_symlink():
            raise WorktreeControlError(f"{context} contains a reparse/symlink component: {current}")
        if os.name == "nt" and current.exists():
            attributes = getattr(current.stat(follow_symlinks=False), "st_file_attributes", 0)
            if attributes & 0x400:
                raise WorktreeControlError(f"{context} contains a reparse component: {current}")
        current = current.parent


def _strict_parent(
    raw: str | Path,
    *,
    integration_root: Path,
    label: str,
) -> Path:
    parent = _canonical(raw)
    if parent == integration_root or _is_relative_to(parent, integration_root):
        raise WorktreeControlError(f"{label} must be outside the integration worktree")
    _reject_reparse(parent, context=label)
    return parent


def common_git_directory(root: str | Path) -> Path:
    repo = _canonical(root, strict=True)
    raw = git_text(repo, "rev-parse", "--git-common-dir").strip()
    if not raw:
        raise WorktreeControlError("Git common directory is unavailable")
    candidate = Path(raw)
    if not candidate.is_absolute():
        candidate = repo / candidate
    return _canonical(candidate, strict=True)


def _parse_lock_reason(value: str) -> WorktreeAssociation | None:
    if not value.startswith(PACKET_WORKTREE_MARKER + "|"):
        return None
    fields = value.split("|", 3)
    if len(fields) != 4 or fields[0] != PACKET_WORKTREE_MARKER:
        raise WorktreeControlError("malformed packet worktree lock reason")
    _, authority, packet_id, build_root = fields
    if authority != "issue" or not packet_id or not build_root:
        raise WorktreeControlError("malformed packet worktree association")
    build = Path(build_root)
    if not build.is_absolute():
        raise WorktreeControlError("packet build root in lock reason must be absolute")
    return WorktreeAssociation(authority, packet_id, str(_canonical(build)))


def parse_worktree_list_porcelain(value: str) -> tuple[GitWorktree, ...]:
    records: list[GitWorktree] = []
    current: dict[str, Any] | None = None
    seen_roots: set[str] = set()
    for raw in value.splitlines():
        if raw.startswith("worktree "):
            if current is not None:
                records.append(_finish_worktree_record(current, seen_roots))
            current = {"root": _git_unquote(raw[len("worktree "):]), "flags": set()}
            continue
        if current is None:
            if raw:
                raise WorktreeControlError("worktree porcelain begins without a worktree row")
            continue
        if not raw:
            records.append(_finish_worktree_record(current, seen_roots))
            current = None
            continue
        key, separator, value_part = raw.partition(" ")
        if key in {"detached", "bare"} and not separator:
            current["flags"].add(key)
        elif key in {"HEAD", "branch", "locked", "prunable"}:
            if key in current:
                raise WorktreeControlError(f"duplicate worktree porcelain field {key!r}")
            current[key] = _git_unquote(value_part) if separator else ""
        else:
            raise WorktreeControlError(f"unsupported worktree porcelain row: {raw!r}")
    if current is not None:
        records.append(_finish_worktree_record(current, seen_roots))
    if not records:
        raise WorktreeControlError("Git reports no worktrees")
    return tuple(records)


def _git_unquote(value: str) -> str:
    if not value.startswith('"'):
        return value
    try:
        decoded = ast.literal_eval(value)
    except (SyntaxError, ValueError) as exc:
        raise WorktreeControlError("malformed quoted worktree porcelain field") from exc
    if not isinstance(decoded, str):
        raise WorktreeControlError("quoted worktree porcelain field is not text")
    return decoded


def _finish_worktree_record(
    data: Mapping[str, Any], seen_roots: set[str]
) -> GitWorktree:
    raw_root = data.get("root")
    head = data.get("HEAD")
    if not isinstance(raw_root, str) or not raw_root:
        raise WorktreeControlError("worktree porcelain has no root")
    if not isinstance(head, str) or not head:
        raise WorktreeControlError("worktree porcelain has no HEAD")
    lexical_root = _lexical_absolute(raw_root, context="Git worktree root")
    root = _canonical(lexical_root)
    root_key = os.path.normcase(str(root))
    if root_key in seen_roots:
        raise WorktreeControlError(f"duplicate Git worktree root: {root}")
    seen_roots.add(root_key)
    flags = data.get("flags", set())
    if "bare" in flags:
        raise WorktreeControlError("bare Git repositories are not packet worktrees")
    detached = "detached" in flags
    branch_ref = data.get("branch")
    branch: str | None = None
    if isinstance(branch_ref, str) and branch_ref:
        prefix = "refs/heads/"
        if not branch_ref.startswith(prefix):
            raise WorktreeControlError("worktree branch is not a local named branch")
        branch = branch_ref[len(prefix):]
    if detached and branch is not None:
        raise WorktreeControlError("worktree cannot be both detached and on a branch")
    locked_raw = data.get("locked")
    locked = locked_raw is not None
    lock_reason = str(locked_raw) if locked and locked_raw else None
    association = _parse_lock_reason(lock_reason) if lock_reason else None
    prunable_raw = data.get("prunable")
    return GitWorktree(
        root=root,
        head=head,
        branch=branch,
        detached=detached,
        locked=locked,
        lock_reason=lock_reason,
        prunable=prunable_raw is not None,
        prunable_reason=(str(prunable_raw) if prunable_raw else None),
        association=association,
    )


def list_git_worktrees(root: str | Path) -> tuple[GitWorktree, ...]:
    return parse_worktree_list_porcelain(
        git_text(root, "worktree", "list", "--porcelain")
    )


def _path_key(path: Path) -> str:
    return os.path.normcase(str(path))


def _require_identity_shape(
    identity: PhysicalFileIdentity,
    *,
    path: Path,
    directory: bool,
    context: str,
) -> None:
    if identity.is_directory is not directory:
        expected = "directory" if directory else "file"
        raise WorktreeControlError(f"{context} identity is not a {expected}")
    if _path_key(Path(identity.canonical_path)) != _path_key(path):
        raise WorktreeControlError(f"{context} identity path changed")


def _observe_machine_local_file_identity(
    path: Path, *, root_volume_identity: int
) -> PhysicalFileIdentity:
    """Observe a file ID without taking a share-denying Windows handle.

    SQLite readers and writers legitimately overlap the read-only audits that
    route through this module.  ``windows_identity.physical_identity`` uses a
    deliberately restrictive stable handle, which is appropriate while bytes
    are consumed but can collide with an ordinary SQLite connection merely
    while observing identity.  Windows ``stat`` exposes the same kernel file
    index as ``BY_HANDLE_FILE_INFORMATION``; the already-authenticated,
    reparse-free worktree root supplies the volume identity.  This preserves
    the volume/file-ID replacement check without denying SQLite sharing.
    """

    if os.name != "nt":
        return physical_identity(path, directory=False)
    try:
        info = path.stat(follow_symlinks=False)
    except OSError as exc:
        raise WorktreeControlError(
            f"cannot observe canonical machine-local input identity: {path}: {exc}"
        ) from exc
    if not stat.S_ISREG(info.st_mode):
        raise WorktreeControlError(
            f"canonical machine-local input identity is not a file: {path}"
        )
    file_id = int(info.st_ino)
    if file_id <= 0:
        raise WorktreeControlError(
            f"canonical machine-local input has no stable Windows file ID: {path}"
        )
    return PhysicalFileIdentity(
        volume_identity=int(root_volume_identity),
        file_id=file_id,
        file_size=int(info.st_size),
        is_directory=False,
        canonical_path=str(path),
    )


def _normalize_machine_local_paths(paths: Sequence[str]) -> tuple[str, ...]:
    normalized: list[str] = []
    seen: set[str] = set()
    for raw in paths:
        if not isinstance(raw, str):
            raise WorktreeControlError(
                f"machine-local path must be exact and repository-relative: {raw!r}"
            )
        parts = raw.split("/")
        if (
            not raw
            or "\x00" in raw
            or "\\" in raw
            or raw.startswith("/")
            or raw.startswith("//")
            or (len(raw) >= 2 and raw[1] == ":")
            or any(part in {"", ".", ".."} for part in parts)
        ):
            raise WorktreeControlError(
                f"machine-local path must be exact and repository-relative: {raw!r}"
            )
        value = raw
        key = value.casefold()
        if key in seen:
            raise WorktreeControlError(
                f"duplicate machine-local path requirement: {value}"
            )
        seen.add(key)
        normalized.append(value)
    return tuple(normalized)


def _worktree_by_root(
    worktrees: Sequence[GitWorktree], root: Path
) -> GitWorktree | None:
    key = _path_key(root)
    matches = [row for row in worktrees if _path_key(row.root) == key]
    if len(matches) > 1:
        raise WorktreeControlError(f"multiple Git worktrees resolve to {root}")
    return matches[0] if matches else None


def _is_integration_worktree(row: GitWorktree) -> bool:
    return bool(
        (row.branch and row.branch.startswith("integration/"))
        or (
            row.lock_reason
            and row.lock_reason.startswith(INTEGRATION_WORKTREE_MARKER + "|")
        )
    )


def _authenticate_canonical_candidate(
    *,
    row: GitWorktree,
    common_directory: Path,
    required_paths: tuple[str, ...],
) -> tuple[PhysicalFileIdentity, tuple[tuple[str, PhysicalFileIdentity], ...]]:
    if row.prunable:
        raise WorktreeControlError(
            f"canonical control-root candidate is prunable: {row.root}"
        )
    if row.detached or row.branch is None:
        raise WorktreeControlError(
            f"detached worktree cannot be the canonical control root: {row.root}"
        )
    if row.association is not None or (
        row.branch and row.branch.startswith(_PACKET_BRANCH_PREFIX)
    ):
        raise WorktreeControlError(
            f"packet-associated worktree cannot be the canonical control root: {row.root}"
        )
    if _is_integration_worktree(row):
        raise WorktreeControlError(
            f"temporary integration worktree cannot be the canonical control root: {row.root}"
        )
    lexical_root = _lexical_absolute(
        row.root, context="canonical control-root candidate"
    )
    root = _canonical(lexical_root, strict=True)
    if not root.is_dir() or root.is_symlink():
        raise WorktreeControlError(
            f"canonical control root is not an ordinary directory: {root}"
        )
    _reject_reparse(root, context="canonical control root")
    if common_git_directory(root) != common_directory:
        raise WorktreeControlError(
            f"canonical control root belongs to a different Git common directory: {root}"
        )
    try:
        root_identity = physical_identity(root, directory=True)
    except (OSError, WindowsIdentityError) as exc:
        raise WorktreeControlError(
            f"cannot authenticate canonical control-root identity: {root}: {exc}"
        ) from exc
    _require_identity_shape(
        root_identity,
        path=root,
        directory=True,
        context="canonical control root",
    )
    identities: list[tuple[str, PhysicalFileIdentity]] = []
    for relative in required_paths:
        candidate = root.joinpath(*PurePosixPath(relative).parts)
        if not os.path.lexists(candidate):
            raise WorktreeControlError(
                f"canonical machine-local input is missing: {candidate}"
            )
        _reject_reparse(candidate, context=f"canonical machine-local input {relative}")
        if candidate.is_symlink() or not candidate.is_file():
            raise WorktreeControlError(
                f"canonical machine-local input is not an ordinary file: {candidate}"
            )
        resolved = _canonical(candidate, strict=True)
        if not _is_relative_to(resolved, root):
            raise WorktreeControlError(
                f"canonical machine-local input escapes its worktree: {candidate}"
            )
        try:
            identity = _observe_machine_local_file_identity(
                resolved,
                root_volume_identity=root_identity.volume_identity,
            )
        except (OSError, WindowsIdentityError) as exc:
            raise WorktreeControlError(
                f"cannot authenticate canonical machine-local input identity: "
                f"{candidate}: {exc}"
            ) from exc
        _require_identity_shape(
            identity,
            path=resolved,
            directory=False,
            context=f"canonical machine-local input {relative}",
        )
        identities.append((relative, identity))
    return root_identity, tuple(identities)


def resolve_canonical_control_root(
    *,
    executing_worktree_root: str | Path,
    required_machine_local_paths: Sequence[str],
    explicit_root: str | Path | None = None,
) -> CanonicalControlRoot:
    """Select one authenticated same-repository root for machine-local inputs."""

    executing_lexical = _lexical_absolute(
        executing_worktree_root, context="executing worktree root"
    )
    executing = _canonical(executing_lexical, strict=True)
    worktrees = list_git_worktrees(executing)
    executing_row = _worktree_by_root(worktrees, executing)
    if executing_row is None:
        raise WorktreeControlError(
            f"executing root is not an exact listed Git worktree: {executing}"
        )
    common_directory = common_git_directory(executing)
    required = _normalize_machine_local_paths(required_machine_local_paths)
    supplied = explicit_root
    resolution_source = "explicit"
    if supplied is None:
        supplied = os.environ.get(CANONICAL_ROOT_ENV)

    if supplied is not None:
        candidate_lexical = _lexical_absolute(
            supplied, context="explicit canonical root"
        )
        candidate = _canonical(candidate_lexical, strict=True)
        try:
            candidate_common = common_git_directory(candidate)
        except WorktreeControlError as exc:
            raise WorktreeControlError(
                f"explicit canonical root is not a Git worktree: {candidate}"
            ) from exc
        if candidate_common != common_directory:
            raise WorktreeControlError(
                "explicit canonical root belongs to a different Git common directory: "
                f"{candidate}"
            )
        row = _worktree_by_root(worktrees, candidate)
        if row is None:
            raise WorktreeControlError(
                f"explicit canonical root is not an exact listed Git worktree: {candidate}"
            )
        root_identity, path_identities = _authenticate_canonical_candidate(
            row=row,
            common_directory=common_directory,
            required_paths=required,
        )
        selected = row
    else:
        try:
            root_identity, path_identities = _authenticate_canonical_candidate(
                row=executing_row,
                common_directory=common_directory,
                required_paths=required,
            )
        except WorktreeControlError:
            candidates: list[
                tuple[
                    GitWorktree,
                    PhysicalFileIdentity,
                    tuple[tuple[str, PhysicalFileIdentity], ...],
                ]
            ] = []
            for row in worktrees:
                try:
                    candidate_root_identity, candidate_path_identities = (
                        _authenticate_canonical_candidate(
                            row=row,
                            common_directory=common_directory,
                            required_paths=required,
                        )
                    )
                except WorktreeControlError:
                    continue
                candidates.append(
                    (row, candidate_root_identity, candidate_path_identities)
                )
            if not candidates:
                rendered = ", ".join(required) or "<none>"
                raise WorktreeControlError(
                    "no eligible canonical control root contains every required "
                    f"machine-local input: {rendered}"
                )
            if len(candidates) != 1:
                roots = ", ".join(str(item[0].root) for item in candidates)
                raise WorktreeControlError(
                    f"canonical control root is ambiguous across Git worktrees: {roots}"
                )
            selected, root_identity, path_identities = candidates[0]
            resolution_source = "unique-worktree"
        else:
            selected = executing_row
            resolution_source = "executing-root"

    return CanonicalControlRoot(
        executing_worktree_root=executing,
        canonical_control_root=selected.root,
        common_git_directory=common_directory,
        resolution_source=resolution_source,
        required_machine_local_paths=required,
        canonical_root_identity=root_identity,
        required_path_identities=path_identities,
    )


def reauthenticate_canonical_control_root(
    resolution: CanonicalControlRoot,
) -> CanonicalControlRoot:
    """Reopen the selected root/files and require their physical identities."""

    current = resolve_canonical_control_root(
        executing_worktree_root=resolution.executing_worktree_root,
        required_machine_local_paths=resolution.required_machine_local_paths,
        explicit_root=resolution.canonical_control_root,
    )
    try:
        require_same_physical_object(
            resolution.canonical_root_identity,
            current.canonical_root_identity,
            context="canonical control root",
        )
        expected_paths = dict(resolution.required_path_identities)
        observed_paths = dict(current.required_path_identities)
        if expected_paths.keys() != observed_paths.keys():
            raise WorktreeControlError(
                "canonical machine-local path identity set changed"
            )
        for relative, expected in expected_paths.items():
            require_same_physical_object(
                expected,
                observed_paths[relative],
                context=f"canonical machine-local input {relative}",
            )
    except WindowsIdentityError as exc:
        raise WorktreeControlError(str(exc)) from exc
    return current


def routed_machine_local_path(
    *,
    executing_worktree_root: str | Path,
    relative_path: str,
) -> Path:
    """Route one machine-local input only when linked validation is active.

    Normal primary-worktree and isolated-test use retains the executing-root
    path.  An authenticated linked-validation environment resolves the exact
    input through the canonical control root without copying or linking it into
    the executing checkout.
    """

    executing = Path(executing_worktree_root)
    canonical_text = os.environ.get(CANONICAL_ROOT_ENV)
    if not canonical_text:
        return executing / Path(relative_path)
    resolution = resolve_canonical_control_root(
        executing_worktree_root=executing,
        required_machine_local_paths=(relative_path,),
        explicit_root=canonical_text,
    )
    return resolution.canonical_control_root / Path(relative_path)


def canonical_validation_environment(
    resolution: CanonicalControlRoot,
    *,
    external_build_root: str | Path,
    expected_external_build_root_identity: PhysicalFileIdentity | None = None,
    base_environment: Mapping[str, str] | None = None,
) -> dict[str, str]:
    """Preserve the process environment and add only bounded routing roots."""

    reauthenticate_canonical_control_root(resolution)
    build_lexical = _lexical_absolute(
        external_build_root, context="external validation build root"
    )
    build_root = _canonical(build_lexical)
    if expected_external_build_root_identity is not None:
        authenticate_temporary_build_root(
            build_root,
            expected_identity=expected_external_build_root_identity,
        )
    environment = dict(os.environ if base_environment is None else base_environment)
    environment[CANONICAL_ROOT_ENV] = str(resolution.canonical_control_root)
    environment[EXECUTION_WORKTREE_ROOT_ENV] = str(
        resolution.executing_worktree_root
    )
    environment[EXTERNAL_BUILD_ROOT_ENV] = str(build_root)
    return environment


def create_temporary_build_root(path: str | Path) -> PhysicalFileIdentity:
    """Create one exact transient integration/audit build root."""

    lexical = _lexical_absolute(path, context="temporary build root")
    root = _canonical(lexical)
    if os.path.lexists(root):
        raise WorktreeControlError(f"temporary build root already exists: {root}")
    root.parent.mkdir(parents=True, exist_ok=True)
    _reject_reparse(root.parent, context="temporary build parent")
    root.mkdir()
    return physical_identity(root, directory=True)


def authenticate_temporary_build_root(
    path: str | Path, *, expected_identity: PhysicalFileIdentity
) -> PhysicalFileIdentity:
    """Require the exact allocated transient directory to still exist."""

    lexical = _lexical_absolute(path, context="temporary build root")
    if not os.path.lexists(lexical):
        raise WorktreeControlError(f"temporary build root disappeared: {lexical}")
    if lexical.is_symlink() or not lexical.is_dir():
        raise WorktreeControlError(
            f"temporary build root is not an ordinary directory: {lexical}"
        )
    _reject_reparse(lexical, context="temporary build root")
    root = _canonical(lexical, strict=True)
    _require_identity_shape(
        expected_identity,
        path=root,
        directory=True,
        context="temporary build root expected",
    )
    try:
        observed = physical_identity(root, directory=True)
        _require_identity_shape(
            observed,
            path=root,
            directory=True,
            context="temporary build root observed",
        )
        require_same_physical_object(
            expected_identity,
            observed,
            context="temporary build root",
        )
    except (OSError, WindowsIdentityError) as exc:
        raise WorktreeControlError(str(exc)) from exc
    return observed


def remove_temporary_build_root(
    path: str | Path, *, expected_identity: PhysicalFileIdentity
) -> None:
    """Remove only the exact transient root authenticated at allocation."""

    root = _canonical(path, strict=True)
    authenticate_temporary_build_root(root, expected_identity=expected_identity)
    if root == root.parent or len(root.parts) < 3:
        raise WorktreeControlError("refusing broad temporary build-root removal")
    _reject_reparse(root, context="temporary build root")
    _remove_generated_children(root, preserve=root / ".recoil-preserve-none")
    root.rmdir()


def resolve_topology(
    repository_root: str | Path,
    *,
    worktree_parent: str | Path | None = None,
    build_parent: str | Path | None = None,
) -> WorktreeTopology:
    supplied = _canonical(repository_root, strict=True)
    worktrees = list_git_worktrees(supplied)
    masters = [row for row in worktrees if row.branch == "master"]
    integration_root = masters[0].root if len(masters) == 1 else supplied
    if len(masters) > 1:
        raise WorktreeControlError("multiple worktrees claim master")
    default_worktrees = Path(str(integration_root) + ".worktrees")
    default_builds = Path(str(integration_root) + ".builds")
    worktree_raw = (
        worktree_parent
        or os.environ.get("RECOIL_WORKTREE_PARENT")
        or default_worktrees
    )
    build_raw = (
        build_parent
        or os.environ.get("RECOIL_WORKTREE_BUILD_PARENT")
        or default_builds
    )
    worktree_parent_path = _strict_parent(
        worktree_raw, integration_root=integration_root, label="worktree parent"
    )
    build_parent_path = _strict_parent(
        build_raw, integration_root=integration_root, label="build parent"
    )
    if (
        worktree_parent_path == build_parent_path
        or _is_relative_to(worktree_parent_path, build_parent_path)
        or _is_relative_to(build_parent_path, worktree_parent_path)
    ):
        raise WorktreeControlError("worktree and build parents must be disjoint")
    return WorktreeTopology(
        common_git_directory=common_git_directory(supplied),
        integration_root=integration_root,
        worktree_parent=worktree_parent_path,
        build_parent=build_parent_path,
        worktrees=worktrees,
    )


def _packet_slug(packet_id: str) -> str:
    if not packet_id or any(character in packet_id for character in "\r\n\0"):
        raise WorktreeControlError("packet ID is unavailable or malformed")
    tail = packet_id.split(":")[-1]
    slug = re.sub(r"[^a-z0-9._-]+", "-", tail.casefold()).strip("-.")
    if not slug:
        raise WorktreeControlError("packet ID has no path-safe spelling")
    return slug


def derive_packet_locations(
    topology: WorktreeTopology,
    *,
    authority: str,
    packet_id: str,
    revision: int,
) -> tuple[str, Path, Path]:
    if authority != "issue":
        raise WorktreeControlError(
            "progress worktree adapter is contained-disabled: "
            + PROGRESS_ADAPTER_REASON
        )
    slug = _packet_slug(packet_id)
    suffix = f"{authority}-{slug}-r{int(revision)}"
    branch = f"{_PACKET_BRANCH_PREFIX}{authority}/{slug}-r{int(revision)}"
    worktree = topology.worktree_parent / suffix
    build_root = topology.build_parent / suffix
    for candidate in (worktree, build_root):
        if len(str(candidate)) > _WINDOWS_SAFE_CHILD_LIMIT:
            raise WorktreeControlError(f"packet path exceeds the governed Windows limit: {candidate}")
    return branch, worktree, build_root


def resolve_exact_packet_worktree(
    repository_root: str | Path,
    descriptor: Mapping[str, object],
    *,
    packet_id: str,
    writable_paths: Iterable[str],
) -> tuple[Path, WorktreeAssociation | None]:
    baseline = validate_git_baseline_descriptor(
        descriptor, packet_id=packet_id, writable_paths=writable_paths
    )
    topology = resolve_topology(repository_root)
    branch = str(baseline["branch"])
    matches = [row for row in topology.worktrees if row.branch == branch]
    if len(matches) != 1:
        raise WorktreeControlError(
            f"governed packet Git branch changed or is unavailable: branch "
            f"{branch!r} must have exactly one Git worktree"
        )
    match = matches[0]
    if match.detached or match.prunable:
        raise WorktreeControlError("packet worktree is detached or prunable")
    if common_git_directory(match.root) != topology.common_git_directory:
        raise WorktreeControlError("packet worktree belongs to a different Git common directory")
    if match.association is not None:
        if match.association.authority != "issue" or match.association.packet_id != packet_id:
            raise WorktreeControlError("packet worktree association does not match packet identity")
        if match.root.parent != topology.worktree_parent:
            raise WorktreeControlError("packet worktree is outside the governed worktree parent")
        if _canonical(match.association.external_build_root).parent != topology.build_parent:
            raise WorktreeControlError("packet build root is outside the governed build parent")
    elif match.root != topology.integration_root:
        raise WorktreeControlError("linked packet worktree lacks its strict association lock")
    return match.root, match.association


def capture_packet_git_closeout(
    repository_root: str | Path,
    descriptor: Mapping[str, object],
    *,
    packet_id: str,
    writable_paths: Iterable[str],
) -> dict[str, object]:
    packet_root, association = resolve_exact_packet_worktree(
        repository_root,
        descriptor,
        packet_id=packet_id,
        writable_paths=writable_paths,
    )
    result = capture_git_closeout(
        packet_root,
        descriptor,
        packet_id=packet_id,
        writable_paths=writable_paths,
    )
    return {
        **result,
        "worktree_root": str(packet_root),
        "worktree_association": (
            association.to_dict() if association is not None else None
        ),
    }


def _identity_from_record(record: Mapping[str, Any]) -> PhysicalFileIdentity:
    fields = ("volume_identity", "file_id", "file_size", "is_directory", "canonical_path")
    if set(record) != set(fields):
        raise WorktreeControlError("build-root marker physical identity is incomplete")
    if type(record["is_directory"]) is not bool:
        raise WorktreeControlError("build-root marker directory identity is malformed")
    if any(type(record[field]) is not int for field in ("volume_identity", "file_id", "file_size")):
        raise WorktreeControlError("build-root marker numeric identity is malformed")
    if not isinstance(record["canonical_path"], str) or not record["canonical_path"]:
        raise WorktreeControlError("build-root marker canonical path is malformed")
    try:
        return PhysicalFileIdentity(
            volume_identity=int(record["volume_identity"]),
            file_id=int(record["file_id"]),
            file_size=int(record["file_size"]),
            is_directory=bool(record["is_directory"]),
            canonical_path=str(record["canonical_path"]),
        )
    except (TypeError, ValueError) as exc:
        raise WorktreeControlError("build-root marker physical identity is malformed") from exc


def create_build_root(
    path: str | Path,
    *,
    authority: str,
    packet_id: str,
    branch: str,
    worktree_root: str | Path,
) -> dict[str, object]:
    root = _canonical(path)
    if os.path.lexists(root):
        raise WorktreeControlError(f"packet build root already exists: {root}")
    root.parent.mkdir(parents=True, exist_ok=True)
    _reject_reparse(root.parent, context="build parent")
    root.mkdir()
    identity = physical_identity(root, directory=True)
    marker = {
        "schema": BUILD_ROOT_MARKER_SCHEMA,
        "authority": authority,
        "packet_id": packet_id,
        "branch": branch,
        "worktree_root": str(_canonical(worktree_root)),
        "build_root": str(root),
        "physical_identity": identity.to_dict(),
    }
    marker_path = root / BUILD_ROOT_MARKER_NAME
    with marker_path.open("x", encoding="utf-8", newline="\n") as handle:
        handle.write(json.dumps(marker, indent=2, sort_keys=True) + "\n")
        handle.flush()
        os.fsync(handle.fileno())
    authenticate_build_root(root, authority=authority, packet_id=packet_id)
    return marker


def authenticate_build_root(
    path: str | Path,
    *,
    authority: str,
    packet_id: str,
    branch: str | None = None,
    worktree_root: str | Path | None = None,
) -> dict[str, object]:
    root = _canonical(path, strict=True)
    _reject_reparse(root, context="packet build root")
    marker_path = root / BUILD_ROOT_MARKER_NAME
    if not marker_path.is_file() or marker_path.is_symlink():
        raise WorktreeControlError("packet build-root marker is unavailable")
    try:
        marker = json.loads(marker_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise WorktreeControlError("packet build-root marker is malformed") from exc
    if not isinstance(marker, dict) or marker.get("schema") != BUILD_ROOT_MARKER_SCHEMA:
        raise WorktreeControlError("packet build-root marker schema is unsupported")
    if set(marker) != {
        "schema",
        "authority",
        "packet_id",
        "branch",
        "worktree_root",
        "build_root",
        "physical_identity",
    }:
        raise WorktreeControlError("packet build-root marker fields are incomplete or unexpected")
    if marker.get("authority") != authority or marker.get("packet_id") != packet_id:
        raise WorktreeControlError("packet build-root marker ownership changed")
    if marker.get("build_root") != str(root):
        raise WorktreeControlError("packet build-root marker path changed")
    if branch is not None and marker.get("branch") != branch:
        raise WorktreeControlError("packet build-root marker branch changed")
    if (
        worktree_root is not None
        and marker.get("worktree_root") != str(_canonical(worktree_root, strict=True))
    ):
        raise WorktreeControlError("packet build-root marker worktree changed")
    expected_raw = marker.get("physical_identity")
    if not isinstance(expected_raw, Mapping):
        raise WorktreeControlError("packet build-root physical identity is absent")
    expected = _identity_from_record(expected_raw)
    if expected.canonical_path != str(root) or not expected.is_directory:
        raise WorktreeControlError("packet build-root physical identity path/type changed")
    observed = physical_identity(root, directory=True)
    try:
        require_same_physical_object(
            expected, observed, context="packet build root"
        )
    except WindowsIdentityError as exc:
        raise WorktreeControlError(str(exc)) from exc
    return marker


def remove_authenticated_build_root(
    path: str | Path,
    *,
    authority: str,
    packet_id: str,
) -> None:
    root = _canonical(path, strict=True)
    authenticate_build_root(root, authority=authority, packet_id=packet_id)
    if root == root.parent or len(root.parts) < 3:
        raise WorktreeControlError("refusing broad build-root removal")
    marker = root / BUILD_ROOT_MARKER_NAME
    _remove_generated_children(root, preserve=marker)
    authenticate_build_root(root, authority=authority, packet_id=packet_id)
    marker.unlink()
    root.rmdir()


def _remove_generated_children(root: Path, *, preserve: Path) -> None:
    """Remove an authenticated generated tree without following reparse points."""
    entries = list(os.scandir(root))
    for entry in entries:
        path = Path(entry.path)
        if path == preserve:
            continue
        if entry.is_symlink():
            path.unlink()
            continue
        if os.name == "nt":
            attributes = getattr(entry.stat(follow_symlinks=False), "st_file_attributes", 0)
            if attributes & 0x400:
                raise WorktreeControlError(
                    f"refusing to traverse generated reparse entry: {path}"
                )
        if entry.is_dir(follow_symlinks=False):
            _remove_generated_children(path, preserve=preserve)
            path.rmdir()
        elif entry.is_file(follow_symlinks=False):
            path.unlink()
        else:
            raise WorktreeControlError(f"unsupported generated build-root entry: {path}")


def branch_names(root: str | Path) -> tuple[str, ...]:
    rows = git_text(
        root, "for-each-ref", "--format=%(refname:short)", "refs/heads"
    ).splitlines()
    if any(not row for row in rows):
        raise WorktreeControlError("Git returned an empty branch name")
    return tuple(sorted(rows, key=str.casefold))


def is_ancestor(root: str | Path, ancestor: str, descendant: str) -> bool:
    result = _run_git(
        _canonical(root, strict=True),
        ["merge-base", "--is-ancestor", ancestor, descendant],
        check=False,
    )
    if result.returncode not in {0, 1}:
        raise WorktreeControlError(
            (result.stderr or result.stdout).strip() or "Git ancestry check failed"
        )
    return result.returncode == 0


def require_clean_worktree(root: str | Path) -> None:
    repo = _canonical(root, strict=True)
    status = git_text(repo, "status", "--porcelain=v2", "-z", "--untracked-files=all")
    unmerged = git_text(repo, "ls-files", "-u", "-z")
    if status or unmerged:
        raise WorktreeControlError(f"worktree is not clean: {repo}")


def create_linked_worktree(
    topology: WorktreeTopology,
    *,
    branch: str,
    worktree_root: Path,
    start_point: str,
    association: WorktreeAssociation,
) -> None:
    if branch in branch_names(topology.integration_root):
        raise WorktreeControlError(f"packet branch already exists: {branch}")
    if worktree_root.exists():
        raise WorktreeControlError(f"packet worktree path already exists: {worktree_root}")
    worktree_root.parent.mkdir(parents=True, exist_ok=True)
    _reject_reparse(worktree_root.parent, context="worktree parent")
    _run_git(
        topology.integration_root,
        ["worktree", "add", "-b", branch, str(worktree_root), start_point],
    )
    _run_git(
        topology.integration_root,
        ["worktree", "lock", str(worktree_root), "--reason", association.lock_reason()],
    )


def remove_linked_worktree(
    integration_root: str | Path,
    *,
    worktree_root: str | Path,
    branch: str,
) -> None:
    root = _canonical(integration_root, strict=True)
    child = _canonical(worktree_root, strict=True)
    _run_git(root, ["worktree", "unlock", str(child)])
    _run_git(root, ["worktree", "remove", str(child)])
    _run_git(root, ["branch", "-d", branch])


def hygiene_findings(
    topology: WorktreeTopology,
    *,
    issue_packets: Iterable[Mapping[str, Any]] = (),
    issue_reservations: Iterable[Mapping[str, Any]] = (),
) -> list[dict[str, str]]:
    findings: list[dict[str, str]] = []
    packets = {
        str(row.get("id")): row
        for row in issue_packets
        if isinstance(row, Mapping) and row.get("state") in {"ready", "active"}
    }
    reservations = {
        str(row.get("packet_id")): row
        for row in issue_reservations
        if isinstance(row, Mapping) and row.get("state") == "active"
    }
    association_counts: dict[str, int] = {}
    build_root_owners: dict[str, str] = {}
    branches = set(branch_names(topology.integration_root))
    for row in topology.worktrees:
        if row.prunable:
            findings.append({"kind": "prunable-worktree", "detail": str(row.root)})
        if row.root != topology.integration_root and _is_relative_to(row.root, topology.integration_root):
            findings.append({"kind": "nested-worktree", "detail": str(row.root)})
        association = row.association
        if association is None:
            if row.branch and row.branch.startswith(_PACKET_BRANCH_PREFIX):
                findings.append({"kind": "unassociated-packet-worktree", "detail": row.branch})
            if row.branch and row.branch.startswith(_TEMP_INTEGRATION_PREFIX):
                findings.append({"kind": "stale-integration-worktree", "detail": row.branch})
            continue
        association_counts[association.packet_id] = association_counts.get(association.packet_id, 0) + 1
        if row.root.parent != topology.worktree_parent:
            findings.append({"kind": "packet-worktree-outside-parent", "detail": str(row.root)})
        if row.branch and is_ancestor(topology.integration_root, row.branch, "master"):
            findings.append({"kind": "merged-packet-branch-retained", "detail": row.branch})
        if _canonical(association.external_build_root).parent != topology.build_parent:
            findings.append({
                "kind": "build-root-outside-parent",
                "detail": association.external_build_root,
            })
        build_key = os.path.normcase(str(_canonical(association.external_build_root)))
        prior_build_owner = build_root_owners.get(build_key)
        if prior_build_owner is not None and prior_build_owner != association.packet_id:
            findings.append({
                "kind": "build-root-collision",
                "detail": f"{association.external_build_root}: {prior_build_owner}, {association.packet_id}",
            })
        else:
            build_root_owners[build_key] = association.packet_id
        packet = packets.get(association.packet_id)
        if packet is None:
            findings.append({"kind": "worktree-without-packet", "detail": association.packet_id})
        elif packet.get("state") == "active" and association.packet_id not in reservations:
            findings.append({"kind": "active-packet-without-reservation", "detail": association.packet_id})
        try:
            authenticate_build_root(
                association.external_build_root,
                authority=association.authority,
                packet_id=association.packet_id,
                branch=row.branch,
                worktree_root=row.root,
            )
        except WorktreeControlError as exc:
            findings.append({"kind": "build-root-authentication", "detail": str(exc)})
    for packet_id, count in association_counts.items():
        if count != 1:
            findings.append({"kind": "duplicate-packet-worktree", "detail": f"{packet_id}: {count}"})
    for branch in branches:
        if branch.startswith(_PACKET_BRANCH_PREFIX) and not any(row.branch == branch for row in topology.worktrees):
            findings.append({"kind": "packet-branch-without-worktree", "detail": branch})
        if branch.startswith(_TEMP_INTEGRATION_PREFIX) and not any(row.branch == branch for row in topology.worktrees):
            findings.append({"kind": "stale-integration-branch", "detail": branch})
    if topology.build_parent.exists():
        known_build_roots = {
            os.path.normcase(str(_canonical(row.association.external_build_root)))
            for row in topology.worktrees
            if row.association is not None
        }
        for child in topology.build_parent.iterdir():
            child_key = os.path.normcase(str(_canonical(child)))
            if child_key not in known_build_roots:
                findings.append({"kind": "unassociated-build-root", "detail": str(child)})
    archive_tag = _run_git(
        topology.integration_root,
        ["show-ref", "--verify", "--quiet", "refs/tags/archive/pre-native-git-r3599"],
        check=False,
    )
    if archive_tag.returncode not in {0, 1}:
        raise WorktreeControlError("cannot inspect the native-Git archival tag")
    if archive_tag.returncode == 0 and "pre-git-change-control-migration-r3599" in branches:
        findings.append({
            "kind": "safety-branch-after-archive-tag",
            "detail": "pre-git-change-control-migration-r3599",
        })
    return sorted(findings, key=lambda row: (row["kind"], row["detail"]))


def utc_timestamp() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")


def _capture_normalized_path_projection_child(build_root: str | Path) -> dict[str, object]:
    """Compile and project one normalized registered target in this worktree.

    This helper is invoked only in a clean child whose module search path points
    at the exact probed worktree.  Raw bytes and relocation rows are compared
    directly; no digest, stored receipt, Binary Ninja fact, or acceptance state
    participates.
    """
    from _recoil.commands.asm_verify import (
        CoffObject,
        IMAGE_REL_I386_REL32,
        relocation_size,
    )
    from _recoil.commands.vc5_verify import (
        compiler_env_path,
        effective_source_compile_context,
        load_manifest,
    )
    from _recoil.lib.tooling import REPO_ROOT, quote_cmd_arg, response_line

    worktree = Path.cwd().resolve(strict=True)
    if REPO_ROOT.resolve(strict=True) != worktree:
        raise WorktreeControlError("path probe child did not import its exact worktree tools")
    target_name = "zreader_file_exists"
    manifest = worktree / "tools" / "vc5_verify_targets" / f"{target_name}.json"
    target = load_manifest(manifest, enforce_source_policy=False)
    source = "src/GameZRecoil/zUtil/zutl_zar.cpp"
    symbols = ("?FileExists@zReader@@YIHPBD@Z", "@zReader_FileExists_Wrapper@4")
    if (
        target.name != target_name
        or target.source_from.replace("\\", "/") != source
        or target.source_files != (source,)
        or tuple(row.symbol for row in target.functions) != symbols
        or target.compiler_profile != "vc5_o2_ob1_md_gx_facs"
        or target.compare_mode != "coff_bytes"
        or target.compile_context_from
    ):
        raise WorktreeControlError("registered normalized zReader target shape changed")
    tracked_sources = [
        row.replace("\\", "/")
        for row in git_text(worktree, "ls-files", "--", source).splitlines()
        if row
    ]
    if len(tracked_sources) != 1 or tracked_sources[0] != source:
        raise WorktreeControlError(
            "registered normalized zReader source does not use its exact tracked spelling"
        )
    source = normalize_repository_path(tracked_sources[0])
    output = Path(build_root).resolve()
    if output.exists() and any(output.iterdir()):
        raise WorktreeControlError("path probe build root must start empty")
    output.mkdir(parents=True, exist_ok=True)
    _reject_reparse(output, context="path probe build root")
    _, flags = effective_source_compile_context(target, source)
    normalized_flags: list[str] = []
    flag_index = 0
    while flag_index < len(flags):
        flag = flags[flag_index]
        if flag.upper() == "/I":
            if flag_index + 1 >= len(flags) or not flags[flag_index + 1]:
                raise WorktreeControlError("normalized target has an incomplete /I flag")
            normalized_flags.append("/I" + flags[flag_index + 1])
            flag_index += 2
            continue
        normalized_flags.append(flag)
        flag_index += 1
    includes: list[str] = []
    for raw in target.include_dirs:
        candidate = Path(raw)
        if candidate.is_absolute():
            includes.append(str(candidate))
        else:
            normalized = normalize_repository_path(raw)
            includes.append(normalized)
    obj = output / "zutl_zar.obj"
    listing = output / "zutl_zar.cod"
    rsp = output / "compile-zreader.rsp"
    script = output / "compile-zreader.cmd"
    response_args = [
        *normalized_flags,
        *(f"/I{path}" for path in includes),
        f"/Fo{obj}",
        f"/Fa{listing}",
        "/c",
        source,
    ]
    rsp.write_text(
        "\r\n".join(response_line(value) for value in response_args) + "\r\n",
        encoding="ascii",
    )
    env_script = compiler_env_path(target, Path(target.compiler_env)).resolve(strict=True)
    script.write_text(
        "@echo off\r\ncall " + quote_cmd_arg(env_script)
        + " && cl @" + quote_cmd_arg(rsp) + "\r\n",
        encoding="ascii",
    )
    completed = subprocess.run(
        [os.environ.get("ComSpec", "cmd.exe"), "/d", "/c", str(script)],
        cwd=worktree,
        check=False,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    if completed.returncode != 0 or not obj.is_file():
        raise WorktreeControlError(
            "VC5 normalized zReader path probe compile failed: "
            + (completed.stderr or completed.stdout).strip()
        )
    coff = CoffObject.from_path(obj)
    debug_section_numbers = {
        row.index for row in coff.sections if row.name.casefold().startswith(".debug")
    }
    section_names = {row.index: row.name for row in coff.sections}

    def normalized_section_identity(section_number: int) -> str:
        if section_number == 0:
            return "undefined"
        if section_number == -1:
            return "absolute"
        if section_number > 0:
            try:
                return "section:" + section_names[section_number]
            except KeyError as exc:
                raise WorktreeControlError(
                    "COFF symbol references an unavailable section identity: "
                    f"{section_number}"
                ) from exc
        raise WorktreeControlError(
            f"COFF symbol has unsupported non-debug section identity: {section_number}"
        )

    symbol_population = [
        {
            "name": row.name,
            "section_identity": normalized_section_identity(row.section_number),
            "value": row.value,
            "type": row.type,
            "storage_class": row.storage_class,
        }
        for row in coff.symbols
        if (
            row.storage_class != 103
            and row.section_number != -2
            and row.section_number not in debug_section_numbers
        )
    ]
    definitions = [
        row for row in coff.symbols
        if row.name in symbols and row.section_number > 0
    ]
    if len(definitions) != len(symbols) or len({row.name for row in definitions}) != len(symbols):
        raise WorktreeControlError("zReader path probe selected symbol population changed")
    order = [row.name for row in sorted(definitions, key=lambda row: (row.section_number, row.value))]
    functions: dict[str, object] = {}
    for symbol in symbols:
        body = coff.function_bytes(symbol)
        relocation_rows: list[dict[str, object]] = []
        emitted_calls: list[dict[str, object]] = []
        for relocation in body.relocations:
            relative = relocation.offset - body.start
            size = relocation_size(relocation.type)
            raw = body.data[relative:relative + size]
            row = {
                "offset": relative,
                "type": relocation.type,
                "target": relocation.symbol_name,
                "signed_addend": int.from_bytes(raw, "little", signed=True),
            }
            relocation_rows.append(row)
            if relocation.type == IMAGE_REL_I386_REL32 and relative > 0:
                opcode = body.data[relative - 1]
                if opcode in {0xE8, 0xE9}:
                    emitted_calls.append({
                        "offset": relative,
                        "form": "call" if opcode == 0xE8 else "tail-jump",
                        "dispatch": "direct-rel32",
                        "target": relocation.symbol_name,
                    })
            if size == 4 and relative >= 2 and body.data[relative - 2] == 0xFF:
                modrm_group = (body.data[relative - 1] >> 3) & 0x7
                if modrm_group in {2, 4}:
                    emitted_calls.append({
                        "offset": relative,
                        "form": "call" if modrm_group == 2 else "tail-jump",
                        "dispatch": "indirect-relocation",
                        "target": relocation.symbol_name,
                    })
        functions[symbol] = {
            "extent": len(body.data),
            "unrelocated_bytes": [
                [index, byte]
                for index, byte in enumerate(body.data)
                if not body.relocation_mask[index]
            ],
            "relocation_mask": list(body.relocation_mask),
            "relocations": relocation_rows,
            "emitted_calls": emitted_calls,
            "relocation_set_explicit": True,
        }
    return {
        "compiled": True,
        "target": target_name,
        "source": source,
        "symbols": list(symbols),
        "authored_symbol_order": order,
        "coff_symbol_population": symbol_population,
        "coff_symbol_population_policy": (
            "ordered-non-file-non-debug-symbols-with-normalized-section-identity-"
            "value-type-storage"
        ),
        "functions": functions,
        "compile_contract": {
            "compiler_profile": target.compiler_profile,
            "toolchain_environment": str(env_script),
            "ordered_flags": normalized_flags,
            "ordered_includes": includes,
            "source_spelling": source,
            "whole_link_performed": False,
        },
        "relocation_set_explicit": True,
        "diagnostic_provenance": {
            "worktree_root": str(worktree),
            "build_root": str(output),
            "response_file": str(rsp),
            "command_script": str(script),
            "compiler_stdout": completed.stdout,
            "compiler_stderr": completed.stderr,
        },
        "nonaccepting": True,
    }


def _semantic_probe_projection(value: Mapping[str, object]) -> dict[str, object]:
    return {
        key: value[key]
        for key in (
            "compiled",
            "target",
            "source",
            "symbols",
            "authored_symbol_order",
            "coff_symbol_population",
            "coff_symbol_population_policy",
            "functions",
            "compile_contract",
            "relocation_set_explicit",
        )
    }


def run_absolute_path_independence_probe(
    repository_root: str | Path,
    *,
    commit: str,
    worktree_parent: str | Path | None = None,
    build_parent: str | Path | None = None,
) -> dict[str, object]:
    """Compare direct governed normalized-target COFF facts across checkout paths."""
    topology = resolve_topology(
        repository_root, worktree_parent=worktree_parent, build_parent=build_parent
    )
    topology.worktree_parent.mkdir(parents=True, exist_ok=True)
    topology.build_parent.mkdir(parents=True, exist_ok=True)
    _reject_reparse(topology.worktree_parent, context="probe worktree parent")
    _reject_reparse(topology.build_parent, context="probe build parent")
    roots = (
        topology.worktree_parent / "path-probe-a",
        topology.worktree_parent / "path-probe-second-long-root",
    )
    builds = (
        topology.build_parent / "path-probe-a-output",
        topology.build_parent / "path-probe-second-long-output-root",
    )
    for path in (*roots, *builds):
        if path.exists():
            raise WorktreeControlError(f"path probe collision: {path}")
    created: list[Path] = []
    resolved_commit = git_text(
        topology.integration_root, "rev-parse", "--verify", f"{commit}^{{commit}}"
    ).strip()
    if not resolved_commit:
        raise WorktreeControlError("path probe commit is unavailable")
    try:
        for index, root in enumerate(roots):
            _run_git(
                topology.integration_root,
                ["worktree", "add", "--detach", str(root), resolved_commit],
            )
            created.append(root)
            _run_git(
                topology.integration_root,
                ["worktree", "lock", str(root), "--reason", f"recoil-path-probe-v1|{index}"],
            )
            if git_text(root, "rev-parse", "HEAD").strip() != resolved_commit:
                raise WorktreeControlError(
                    f"path probe worktree HEAD differs from resolved commit: {root}"
                )
        projections: list[dict[str, object]] = []
        code = (
            "import json,sys; from _recoil.lib.worktree_control import "
            "_capture_normalized_path_projection_child as f; print(json.dumps(f(sys.argv[1])))"
        )
        for root, build in zip(roots, builds):
            environment = os.environ.copy()
            environment["PYTHONPATH"] = str(root / "tools")
            environment["PYTHONDONTWRITEBYTECODE"] = "1"
            environment["PYTHONPYCACHEPREFIX"] = str(build.parent / (build.name + "-pycache"))
            completed = subprocess.run(
                [sys.executable, "-B", "-c", code, str(build)],
                cwd=root,
                env=environment,
                check=False,
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
            )
            if completed.returncode != 0:
                raise WorktreeControlError(
                    "path probe child failed: " + (completed.stderr or completed.stdout).strip()
                )
            try:
                projection = json.loads(completed.stdout)
            except json.JSONDecodeError as exc:
                raise WorktreeControlError("path probe child returned malformed JSON") from exc
            if not isinstance(projection, dict):
                raise WorktreeControlError("path probe child projection is not an object")
            projections.append(projection)
        head_receipts = []
        for root in roots:
            observed_head = git_text(root, "rev-parse", "HEAD").strip()
            if observed_head != resolved_commit:
                raise WorktreeControlError(
                    f"path probe worktree HEAD drifted during compilation: {root}"
                )
            head_receipts.append({"worktree_root": str(root), "head": observed_head})
        left = _semantic_probe_projection(projections[0])
        right = _semantic_probe_projection(projections[1])
        compiled = all(row.get("compiled") is True for row in projections)
        relocation_sets = all(
            row.get("relocation_set_explicit") is True for row in projections
        )
        passed = left == right and compiled and relocation_sets
        return {
            "passed": passed,
            "compiled": compiled,
            "target": "zreader_file_exists",
            "resolved_commit": resolved_commit,
            "governed_facts_equal": left == right,
            "relocation_set_explicit": relocation_sets,
            "worktree_head_receipts": head_receipts,
            "first_divergence": (
                None if passed else _first_projection_divergence(left, right)
            ),
            "diagnostic_differences": _diagnostic_projection_differences(
                projections[0], projections[1]
            ),
            "left": projections[0],
            "right": projections[1],
            "absolute_paths_are_diagnostic_only": True,
            "worker_builds_nonaccepting": True,
        }
    finally:
        cleanup_errors: list[str] = []
        active_error = sys.exc_info()[1]
        for root in roots:
            if root.exists():
                _run_git(topology.integration_root, ["worktree", "unlock", str(root)], check=False)
                removed = _run_git(
                    topology.integration_root,
                    ["worktree", "remove", str(root)],
                    check=False,
                )
                if removed.returncode != 0 or root.exists():
                    cleanup_errors.append(
                        f"probe worktree removal failed for {root}: "
                        + (removed.stderr or removed.stdout).strip()
                    )
        for build in builds:
            try:
                if build.exists():
                    _remove_generated_children(build, preserve=build / "<none>")
                    build.rmdir()
                cache = build.parent / (build.name + "-pycache")
                if cache.exists():
                    _remove_generated_children(cache, preserve=cache / "<none>")
                    cache.rmdir()
            except (OSError, WorktreeControlError) as exc:
                cleanup_errors.append(f"probe output cleanup failed for {build}: {exc}")
        remaining_roots = {
            os.path.normcase(str(row.root)) for row in list_git_worktrees(topology.integration_root)
        }
        for root in roots:
            if os.path.normcase(str(root.resolve())) in remaining_roots:
                cleanup_errors.append(f"probe worktree remains registered: {root}")
        if cleanup_errors:
            prefix = f"{active_error}; " if active_error is not None else ""
            raise WorktreeControlError(
                prefix + "absolute-path probe cleanup defect: " + "; ".join(cleanup_errors)
            )


def _diagnostic_projection_differences(
    left: Mapping[str, object], right: Mapping[str, object]
) -> list[dict[str, object]]:
    left_diagnostics = left.get("diagnostic_provenance")
    right_diagnostics = right.get("diagnostic_provenance")
    if not isinstance(left_diagnostics, Mapping) or not isinstance(right_diagnostics, Mapping):
        return [{
            "field": "diagnostic_provenance",
            "left": left_diagnostics,
            "right": right_diagnostics,
        }]
    return [
        {
            "field": key,
            "left": left_diagnostics.get(key),
            "right": right_diagnostics.get(key),
        }
        for key in sorted(set(left_diagnostics) | set(right_diagnostics))
        if left_diagnostics.get(key) != right_diagnostics.get(key)
    ]


def _first_projection_divergence(
    left: Mapping[str, object], right: Mapping[str, object]
) -> dict[str, object]:
    for key in left:
        if left.get(key) != right.get(key):
            return {"field": key, "left": left.get(key), "right": right.get(key)}
    return {"field": "projection", "left": dict(left), "right": dict(right)}


__all__ = [
    "BUILD_ROOT_MARKER_NAME",
    "BUILD_ROOT_MARKER_SCHEMA",
    "CANONICAL_ROOT_ENV",
    "CanonicalControlRoot",
    "EXECUTION_WORKTREE_ROOT_ENV",
    "EXTERNAL_BUILD_ROOT_ENV",
    "GitWorktree",
    "PACKET_WORKTREE_MARKER",
    "PROGRESS_ADAPTER_REASON",
    "PROGRESS_ADAPTER_STATE",
    "WorktreeAssociation",
    "WorktreeControlError",
    "WorktreeTopology",
    "authenticate_build_root",
    "authenticate_temporary_build_root",
    "authenticated_validation_command_tokens",
    "branch_names",
    "canonical_validation_environment",
    "capture_packet_git_closeout",
    "common_git_directory",
    "create_build_root",
    "create_linked_worktree",
    "derive_packet_locations",
    "git_text",
    "hygiene_findings",
    "is_ancestor",
    "list_git_worktrees",
    "parse_worktree_list_porcelain",
    "remove_authenticated_build_root",
    "remove_linked_worktree",
    "remove_temporary_build_root",
    "reauthenticate_canonical_control_root",
    "routed_machine_local_path",
    "require_clean_worktree",
    "resolve_exact_packet_worktree",
    "resolve_canonical_control_root",
    "resolve_topology",
    "run_absolute_path_independence_probe",
    "utc_timestamp",
]
