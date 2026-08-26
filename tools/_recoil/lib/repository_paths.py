from __future__ import annotations

import os
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path
from types import MappingProxyType
from typing import Collection, Mapping, Sequence


class RepositoryPathError(ValueError):
    """A typed failure at the repository logical/physical path boundary."""

    def __init__(
        self,
        kind: str,
        message: str,
        *,
        context: str,
        path_text: str | None = None,
        expected_git_path: str | None = None,
        candidates: Sequence[str] = (),
    ) -> None:
        super().__init__(message)
        self.kind = kind
        self.context = context
        self.path_text = path_text
        self.expected_git_path = expected_git_path
        self.candidates = tuple(candidates)


@dataclass(frozen=True)
class GitTrackedPathInventory:
    """One operation's immutable exact Git-index path inventory."""

    repository_root: Path
    exact_paths: frozenset[str]
    casefolded_paths: Mapping[str, tuple[str, ...]]


@dataclass(frozen=True)
class TrackedRepositoryPath:
    """An exact Git logical identity and its separately validated file."""

    git_path: str
    physical_path: Path
    repository_root: Path


@dataclass(frozen=True)
class HistoricalPathResolution:
    """A diagnostic-only reconciliation of historical path spelling."""

    historical_path: str
    status: str
    current_git_path: str | None = None
    candidates: tuple[str, ...] = ()
    current: bool = False
    tracker_mutated: bool = False


@dataclass(frozen=True)
class GeneratedRepositoryPath:
    """A lexical generated/output path constrained to one allowed root."""

    logical_path: str
    allowed_root: str


_DRIVE_PREFIX_RE = re.compile(r"^[A-Za-z]:")
_WINDOWS_REPARSE_POINT_ATTRIBUTE = 0x400


def _error(
    kind: str,
    message: str,
    *,
    context: str,
    path_text: str | None = None,
    expected_git_path: str | None = None,
    candidates: Sequence[str] = (),
) -> RepositoryPathError:
    return RepositoryPathError(
        kind,
        message,
        context=context,
        path_text=path_text,
        expected_git_path=expected_git_path,
        candidates=candidates,
    )


def validate_repository_relative_path(path_text: str, *, context: str) -> str:
    """Validate lexical repository-relative spelling without consulting disk."""

    if not isinstance(path_text, str) or not path_text:
        raise _error(
            "empty-path",
            f"{context} must be a non-empty string",
            context=context,
            path_text=path_text if isinstance(path_text, str) else None,
        )
    if "\0" in path_text:
        raise _error(
            "nul-path",
            f"{context} contains NUL",
            context=context,
            path_text=path_text,
        )
    if "\\" in path_text:
        raise _error(
            "backslash-path",
            f"{context} must be a normalized repo-local path using forward slashes",
            context=context,
            path_text=path_text,
        )
    if (
        path_text.startswith("/")
        or path_text.startswith("//")
        or _DRIVE_PREFIX_RE.match(path_text)
        or Path(path_text).is_absolute()
    ):
        raise _error(
            "absolute-path",
            f"{context} must be a normalized repo-local path",
            context=context,
            path_text=path_text,
        )
    components = path_text.split("/")
    if any(component in {"", ".", ".."} for component in components):
        raise _error(
            "nonnormalized-path",
            f"{context} is not normalized: empty, dot, or dot-dot component",
            context=context,
            path_text=path_text,
        )
    return path_text


def _git_path_command(
    repository_root: Path,
    *arguments: str,
) -> subprocess.CompletedProcess[bytes]:
    """Narrow byte-oriented Git subprocess seam for tests and callers."""

    return subprocess.run(
        ["git", "-C", str(repository_root), *arguments],
        check=False,
        capture_output=True,
    )


def _git_command_bytes(
    repository_root: Path,
    *arguments: str,
    context: str,
) -> bytes:
    completed = _git_path_command(repository_root, *arguments)
    stdout = completed.stdout
    stderr = completed.stderr
    if isinstance(stdout, str):
        stdout = stdout.encode("utf-8")
    if isinstance(stderr, str):
        stderr = stderr.encode("utf-8")
    if not isinstance(stdout, bytes) or not isinstance(stderr, bytes):
        raise _error(
            "git-output-type",
            f"{context}: Git command returned malformed output types",
            context=context,
        )
    if completed.returncode != 0:
        detail = (stderr or stdout).decode("utf-8", errors="replace").strip()
        raise _error(
            "git-command-failed",
            f"{context}: Git command failed: {detail or '<no output>'}",
            context=context,
        )
    return stdout


def _resolved_existing_directory(path: Path, *, context: str) -> Path:
    supplied = Path(path).absolute()
    reparse = _absolute_path_reparse_component(supplied, context=context)
    if reparse is not None:
        raise _error(
            "reparse-component",
            f"{context}: repository root contains a reparse/symlink component: {reparse}",
            context=context,
            path_text=str(path),
        )
    try:
        resolved = supplied.resolve(strict=True)
    except OSError as exc:
        raise _error(
            "repository-root-unavailable",
            f"{context}: repository root is unavailable: {path}",
            context=context,
            path_text=str(path),
        ) from exc
    if not resolved.is_dir():
        raise _error(
            "repository-root-not-directory",
            f"{context}: repository root is not a directory: {path}",
            context=context,
            path_text=str(path),
        )
    return resolved


def _same_physical_path(left: Path, right: Path) -> bool:
    return os.path.normcase(str(left)) == os.path.normcase(str(right))


def _absolute_path_reparse_component(path: Path, *, context: str) -> Path | None:
    absolute = path.absolute()
    anchor = Path(absolute.anchor)
    current = anchor
    candidates = [anchor]
    for component in absolute.parts[1:]:
        current /= component
        candidates.append(current)
    for candidate in candidates:
        try:
            stat = candidate.stat(follow_symlinks=False)
        except OSError as exc:
            raise _error(
                "path-inspection-failed",
                f"{context}: cannot inspect path component {candidate}: {exc}",
                context=context,
                path_text=str(path),
            ) from exc
        if candidate.is_symlink() or (
            os.name == "nt"
            and getattr(stat, "st_file_attributes", 0)
            & _WINDOWS_REPARSE_POINT_ATTRIBUTE
        ):
            return candidate
    return None


def load_git_tracked_path_inventory(
    repository_root: Path,
    *,
    allow_empty: bool = False,
) -> GitTrackedPathInventory:
    """Load a fresh exact tracked-path inventory for one executing worktree."""

    context = "tracked repository root"
    requested_root = _resolved_existing_directory(repository_root, context=context)
    root_output = _git_command_bytes(
        requested_root,
        "rev-parse",
        "--show-toplevel",
        context=context,
    )
    if b"\0" in root_output:
        raise _error(
            "malformed-root-output",
            f"{context}: malformed Git output",
            context=context,
        )
    try:
        root_text = root_output.decode("utf-8", errors="strict")
    except UnicodeDecodeError as exc:
        raise _error(
            "invalid-root-encoding",
            f"{context}: invalid UTF-8 Git output",
            context=context,
        ) from exc
    if root_text.endswith("\r\n"):
        root_text = root_text[:-2]
    elif root_text.endswith("\n"):
        root_text = root_text[:-1]
    else:
        raise _error(
            "malformed-root-output",
            f"{context}: malformed Git output",
            context=context,
        )
    if not root_text or "\n" in root_text or "\r" in root_text:
        raise _error(
            "malformed-root-output",
            f"{context}: malformed Git output",
            context=context,
        )
    root = _resolved_existing_directory(Path(root_text), context=context)
    if not _same_physical_path(requested_root, root):
        raise _error(
            "repository-root-not-worktree-root",
            f"{context}: supplied path is not the exact executing Git worktree root; "
            f"supplied {requested_root}; expected {root}",
            context=context,
            path_text=str(requested_root),
            expected_git_path=str(root),
        )

    raw = _git_command_bytes(
        root,
        "ls-files",
        "-z",
        context="tracked repository inventory",
    )
    inventory_context = "tracked repository inventory"
    if not raw:
        if not allow_empty:
            raise _error(
                "empty-inventory",
                f"{inventory_context}: empty inventory is not permitted",
                context=inventory_context,
            )
        encoded_paths: list[bytes] = []
    else:
        if not raw.endswith(b"\0"):
            raise _error(
                "truncated-inventory",
                f"{inventory_context}: truncated NUL-delimited output",
                context=inventory_context,
            )
        encoded_paths = raw[:-1].split(b"\0")

    exact: set[str] = set()
    folded: dict[str, list[str]] = {}
    for encoded in encoded_paths:
        if not encoded:
            raise _error(
                "empty-inventory-row",
                f"{inventory_context}: malformed empty path row",
                context=inventory_context,
            )
        try:
            path_text = encoded.decode("utf-8", errors="strict")
        except UnicodeDecodeError as exc:
            raise _error(
                "invalid-inventory-encoding",
                f"{inventory_context}: path is not valid UTF-8",
                context=inventory_context,
            ) from exc
        validate_repository_relative_path(
            path_text,
            context="tracked repository inventory path",
        )
        if path_text in exact:
            raise _error(
                "duplicate-inventory-path",
                f"{inventory_context}: duplicate path {path_text!r}",
                context=inventory_context,
                path_text=path_text,
            )
        exact.add(path_text)
        folded.setdefault(path_text.casefold(), []).append(path_text)

    collisions = {
        key: tuple(sorted(paths))
        for key, paths in folded.items()
        if len(paths) > 1
    }
    if collisions:
        first_key = sorted(collisions)[0]
        candidates = collisions[first_key]
        raise _error(
            "casefold-collision",
            f"{inventory_context}: ambiguous case-fold collision: "
            + ", ".join(repr(path) for path in candidates),
            context=inventory_context,
            candidates=candidates,
        )

    immutable_folded = MappingProxyType(
        {key: tuple(paths) for key, paths in folded.items()}
    )
    return GitTrackedPathInventory(
        repository_root=root,
        exact_paths=frozenset(exact),
        casefolded_paths=immutable_folded,
    )


def _path_has_reparse_component(root: Path, candidate: Path) -> Path | None:
    try:
        relative_parts = candidate.relative_to(root).parts
    except ValueError:
        return candidate
    current = root
    paths = [root]
    for component in relative_parts:
        current /= component
        paths.append(current)
    for path in paths:
        try:
            stat = path.stat(follow_symlinks=False)
        except OSError as exc:
            raise _error(
                "path-inspection-failed",
                f"tracked repository file: cannot inspect path component {path}: {exc}",
                context="tracked repository file",
                path_text=str(candidate),
            ) from exc
        if path.is_symlink() or (
            os.name == "nt"
            and getattr(stat, "st_file_attributes", 0)
            & _WINDOWS_REPARSE_POINT_ATTRIBUTE
        ):
            return path
    return None


def _authenticate_inventory_root(
    repository_root: Path,
    inventory: GitTrackedPathInventory,
    *,
    context: str,
) -> Path:
    requested_root = _resolved_existing_directory(repository_root, context=context)
    if not _same_physical_path(requested_root, inventory.repository_root):
        raise _error(
            "inventory-worktree-mismatch",
            f"{context}: Git inventory belongs to another worktree; supplied "
            f"{requested_root}; inventory root {inventory.repository_root}",
            context=context,
            path_text=str(requested_root),
        )
    return requested_root


def _folded_matches(
    supplied: str,
    inventory: GitTrackedPathInventory,
) -> tuple[str, ...]:
    return tuple(inventory.casefolded_paths.get(supplied.casefold(), ()))


def resolve_tracked_repository_file(
    path_text: str,
    *,
    repository_root: Path,
    inventory: GitTrackedPathInventory,
    context: str,
    allowed_suffixes: Collection[str] | None = None,
) -> TrackedRepositoryPath:
    """Authenticate exact Git spelling, then validate the physical file."""

    supplied = validate_repository_relative_path(path_text, context=context)
    root = _authenticate_inventory_root(
        repository_root,
        inventory,
        context=context,
    )
    folded_matches = _folded_matches(supplied, inventory)
    if len(folded_matches) > 1:
        raise _error(
            "casefold-collision",
            f"{context} is ambiguous under case folding: supplied {supplied!r}; "
            f"tracked matches {', '.join(repr(value) for value in folded_matches)}",
            context=context,
            path_text=supplied,
            candidates=folded_matches,
        )
    if supplied not in inventory.exact_paths:
        if len(folded_matches) == 1:
            expected = folded_matches[0]
            raise _error(
                "wrong-case",
                f"{context} has incorrect Git path case: supplied {supplied!r}; "
                f"expected {expected!r}",
                context=context,
                path_text=supplied,
                expected_git_path=expected,
                candidates=folded_matches,
            )
        raise _error(
            "untracked-path",
            f"{context} does not exist as a Git-tracked path: {supplied}",
            context=context,
            path_text=supplied,
        )
    if allowed_suffixes is not None:
        allowed = {suffix.casefold() for suffix in allowed_suffixes}
        if Path(supplied).suffix.casefold() not in allowed:
            raise _error(
                "disallowed-suffix",
                f"{context} is not a C/C++ source or header: {supplied}",
                context=context,
                path_text=supplied,
            )

    logical_candidate = root.joinpath(*supplied.split("/"))
    if not os.path.lexists(logical_candidate):
        raise _error(
            "tracked-file-missing",
            f"{context} does not exist: {supplied}",
            context=context,
            path_text=supplied,
        )
    reparse = _path_has_reparse_component(root, logical_candidate)
    if reparse is not None:
        raise _error(
            "reparse-component",
            f"{context} contains a reparse/symlink component: {reparse}",
            context=context,
            path_text=supplied,
        )
    try:
        physical = logical_candidate.resolve(strict=True)
        physical.relative_to(root)
    except (OSError, ValueError) as exc:
        raise _error(
            "repository-escape",
            f"{context} resolves outside the repository: {supplied}",
            context=context,
            path_text=supplied,
        ) from exc
    if not physical.is_file():
        raise _error(
            "not-ordinary-file",
            f"{context} is not a regular file: {supplied}",
            context=context,
            path_text=supplied,
        )
    return TrackedRepositoryPath(
        git_path=supplied,
        physical_path=physical,
        repository_root=root,
    )


def diagnose_historical_repository_path(
    path_text: str,
    *,
    inventory: GitTrackedPathInventory,
    current_allowed_paths: Collection[str] | None = None,
    context: str = "historical repository path",
) -> HistoricalPathResolution:
    """Reconcile old spelling for diagnosis without conferring currentness."""

    supplied = validate_repository_relative_path(
        path_text,
        context=context,
    )
    if current_allowed_paths is None:
        allowed = inventory.exact_paths
    else:
        normalized_allowed = frozenset(
            validate_repository_relative_path(value, context=f"{context} allowed path")
            for value in current_allowed_paths
        )
        invalid_allowed = tuple(sorted(normalized_allowed - inventory.exact_paths))
        if invalid_allowed:
            raise _error(
                "untracked-current-allowed-path",
                f"{context}: allowed current paths are not exact Git entries: "
                + ", ".join(repr(value) for value in invalid_allowed),
                context=context,
                candidates=invalid_allowed,
            )
        allowed = normalized_allowed
    exact_allowed = supplied in inventory.exact_paths and supplied in allowed
    if exact_allowed:
        return HistoricalPathResolution(
            historical_path=supplied,
            status="exact-historical",
            current_git_path=supplied,
            candidates=(supplied,),
        )
    folded = tuple(
        candidate
        for candidate in _folded_matches(supplied, inventory)
        if candidate in allowed
    )
    if len(folded) == 1:
        return HistoricalPathResolution(
            historical_path=supplied,
            status="historical-case-alias",
            current_git_path=folded[0],
            candidates=folded,
        )
    if len(folded) > 1:
        return HistoricalPathResolution(
            historical_path=supplied,
            status="ambiguous",
            candidates=folded,
        )
    return HistoricalPathResolution(
        historical_path=supplied,
        status="missing",
    )


def normalize_generated_repository_path(
    path_text: str,
    *,
    allowed_roots: Collection[str],
    context: str,
) -> GeneratedRepositoryPath:
    """Normalize a generated path only through explicit lexical roots."""

    logical = validate_repository_relative_path(path_text, context=context)
    normalized_roots = tuple(
        validate_repository_relative_path(root, context=f"{context} allowed root")
        for root in allowed_roots
    )
    matches = tuple(
        root
        for root in normalized_roots
        if logical == root or logical.startswith(root + "/")
    )
    if len(matches) != 1:
        raise _error(
            "generated-root-not-allowed" if not matches else "generated-root-ambiguous",
            f"{context} must be within exactly one allowed generated root; "
            f"supplied {logical!r}; allowed {', '.join(repr(root) for root in normalized_roots)}",
            context=context,
            path_text=logical,
            candidates=matches,
        )
    return GeneratedRepositoryPath(logical_path=logical, allowed_root=matches[0])
