from __future__ import annotations

import os
import re
from dataclasses import dataclass
from pathlib import Path
from types import MappingProxyType
from typing import Collection, Mapping, Sequence
import unicodedata


class RepositoryPathError(ValueError):
    """A typed failure at the repository logical/physical path boundary."""

    def __init__(
        self,
        kind: str,
        message: str,
        *,
        context: str,
        path_text: str | None = None,
        expected_repository_path: str | None = None,
        candidates: Sequence[str] = (),
    ) -> None:
        super().__init__(message)
        self.kind = kind
        self.context = context
        self.path_text = path_text
        self.expected_repository_path = expected_repository_path
        self.candidates = tuple(candidates)


@dataclass(frozen=True)
class RepositoryPathInventory:
    """One operation's immutable inventory of authorized repository files."""

    repository_root: Path
    exact_paths: frozenset[str]
    casefolded_paths: Mapping[str, tuple[str, ...]]
    allowed_roots: tuple[str, ...]
    allowed_paths: tuple[str, ...]


@dataclass(frozen=True)
class RepositoryFile:
    """An exact repository identity and its separately validated file."""

    repository_path: str
    physical_path: Path
    repository_root: Path


@dataclass(frozen=True)
class HistoricalPathResolution:
    """A diagnostic-only reconciliation of historical path spelling."""

    historical_path: str
    status: str
    current_repository_path: str | None = None
    candidates: tuple[str, ...] = ()
    current: bool = False
    tracker_mutated: bool = False


@dataclass(frozen=True)
class GeneratedRepositoryPath:
    """A lexical generated/output path constrained to one allowed root."""

    logical_path: str
    allowed_root: str


AUTHORED_REPOSITORY_ROOTS = (
    ".codex",
    "docs",
    "src",
    "tests",
    "tools",
)
AUTHORED_REPOSITORY_FILES = (
    ".gitattributes",
    ".gitignore",
    "AGENTS.md",
    "README.md",
)
MACHINE_LOCAL_OR_GENERATED_ROOTS = frozenset(
    {".agent", ".devspace", ".git", ".vs", "build", "out"}
)


_DRIVE_PREFIX_RE = re.compile(r"^[A-Za-z]:")
_WINDOWS_REPARSE_POINT_ATTRIBUTE = 0x400


def _error(
    kind: str,
    message: str,
    *,
    context: str,
    path_text: str | None = None,
    expected_repository_path: str | None = None,
    candidates: Sequence[str] = (),
) -> RepositoryPathError:
    return RepositoryPathError(
        kind,
        message,
        context=context,
        path_text=path_text,
        expected_repository_path=expected_repository_path,
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
        raise _error("nul-path", f"{context} contains NUL", context=context, path_text=path_text)
    if "\\" in path_text:
        raise _error(
            "backslash-path",
            f"{context} must use forward slashes",
            context=context,
            path_text=path_text,
        )
    if path_text.startswith("/") or _DRIVE_PREFIX_RE.match(path_text) or Path(path_text).is_absolute():
        raise _error(
            "absolute-path",
            f"{context} must be repository-relative",
            context=context,
            path_text=path_text,
        )
    components = path_text.split("/")
    if any(component in {"", ".", ".."} for component in components):
        raise _error(
            "nonnormalized-path",
            f"{context} contains an empty, dot, or dot-dot component",
            context=context,
            path_text=path_text,
        )
    if unicodedata.normalize("NFC", path_text) != path_text:
        raise _error(
            "noncanonical-unicode",
            f"{context} must use NFC Unicode spelling",
            context=context,
            path_text=path_text,
        )
    return path_text


def _resolved_existing_directory(path: Path, *, context: str) -> Path:
    supplied = Path(path).absolute()
    reparse = _absolute_path_reparse_component(supplied, context=context)
    if reparse is not None:
        raise _error(
            "reparse-component",
            f"{context} contains a reparse/symlink component: {reparse}",
            context=context,
            path_text=str(path),
        )
    try:
        resolved = supplied.resolve(strict=True)
    except OSError as exc:
        raise _error(
            "repository-root-unavailable",
            f"{context} is unavailable: {path}",
            context=context,
            path_text=str(path),
        ) from exc
    if not resolved.is_dir():
        raise _error(
            "repository-root-not-directory",
            f"{context} is not a directory: {path}",
            context=context,
            path_text=str(path),
        )
    return resolved


def _absolute_path_reparse_component(path: Path, *, context: str) -> Path | None:
    absolute = path.absolute()
    current = Path(absolute.anchor)
    candidates = [current]
    for component in absolute.parts[1:]:
        current /= component
        candidates.append(current)
    for candidate in candidates:
        try:
            stat = candidate.stat(follow_symlinks=False)
        except OSError as exc:
            raise _error(
                "path-inspection-failed",
                f"{context}: cannot inspect {candidate}: {exc}",
                context=context,
                path_text=str(path),
            ) from exc
        if candidate.is_symlink() or (
            os.name == "nt"
            and getattr(stat, "st_file_attributes", 0) & _WINDOWS_REPARSE_POINT_ATTRIBUTE
        ):
            return candidate
    return None


def _is_reparse(entry: os.DirEntry[str]) -> bool:
    try:
        stat = entry.stat(follow_symlinks=False)
    except OSError as exc:
        raise _error(
            "path-inspection-failed",
            f"repository inventory cannot inspect {entry.path}: {exc}",
            context="repository inventory",
            path_text=entry.path,
        ) from exc
    return entry.is_symlink() or (
        os.name == "nt"
        and getattr(stat, "st_file_attributes", 0) & _WINDOWS_REPARSE_POINT_ATTRIBUTE
    )


def _scan_directory(root: Path, logical_root: str) -> list[str]:
    found: list[str] = []
    pending: list[tuple[Path, str]] = [(root.joinpath(*logical_root.split("/")), logical_root)]
    while pending:
        physical, logical = pending.pop()
        try:
            entries = sorted(os.scandir(physical), key=lambda item: item.name)
        except OSError as exc:
            raise _error(
                "path-scan-failed",
                f"repository inventory cannot scan {logical}: {exc}",
                context="repository inventory",
                path_text=logical,
            ) from exc
        for entry in entries:
            child = f"{logical}/{entry.name}"
            validate_repository_relative_path(child, context="repository inventory path")
            if _is_reparse(entry):
                raise _error(
                    "reparse-component",
                    f"repository inventory contains a reparse/symlink entry: {child}",
                    context="repository inventory",
                    path_text=child,
                )
            if entry.is_dir(follow_symlinks=False):
                pending.append((Path(entry.path), child))
            elif entry.is_file(follow_symlinks=False):
                found.append(child)
    return found


def load_repository_path_inventory(
    repository_root: Path,
    *,
    allowed_roots: Collection[str] = AUTHORED_REPOSITORY_ROOTS,
    allowed_paths: Collection[str] = AUTHORED_REPOSITORY_FILES,
    allow_empty: bool = False,
) -> RepositoryPathInventory:
    """Scan only explicitly authorized authored roots for one operation."""

    root = _resolved_existing_directory(repository_root, context="repository root")
    roots = tuple(
        sorted(
            validate_repository_relative_path(value, context="authorized repository root")
            for value in allowed_roots
        )
    )
    paths = tuple(
        sorted(
            validate_repository_relative_path(value, context="authorized repository path")
            for value in allowed_paths
        )
    )
    for value in (*roots, *paths):
        first = value.split("/", 1)[0].casefold()
        if first in MACHINE_LOCAL_OR_GENERATED_ROOTS:
            raise _error(
                "machine-local-root",
                f"repository inventory refuses machine-local/generated path {value!r}",
                context="repository inventory",
                path_text=value,
            )
    exact: set[str] = set()
    for logical_root in roots:
        physical = root.joinpath(*logical_root.split("/"))
        if not physical.exists():
            continue
        if not physical.is_dir():
            raise _error(
                "authorized-root-not-directory",
                f"authorized repository root is not a directory: {logical_root}",
                context="repository inventory",
                path_text=logical_root,
            )
        exact.update(_scan_directory(root, logical_root))
    for logical_path in paths:
        physical = root.joinpath(*logical_path.split("/"))
        if not physical.exists():
            continue
        if not physical.is_file():
            raise _error(
                "authorized-path-not-file",
                f"authorized repository path is not a file: {logical_path}",
                context="repository inventory",
                path_text=logical_path,
            )
        exact.add(logical_path)
    if not exact and not allow_empty:
        raise _error(
            "empty-inventory",
            "repository inventory is empty",
            context="repository inventory",
        )
    folded: dict[str, list[str]] = {}
    for path_text in sorted(exact):
        key = unicodedata.normalize("NFC", path_text).casefold()
        folded.setdefault(key, []).append(path_text)
    collisions = {key: tuple(paths) for key, paths in folded.items() if len(paths) > 1}
    if collisions:
        candidates = collisions[sorted(collisions)[0]]
        raise _error(
            "casefold-collision",
            "repository inventory has a case/Unicode collision: "
            + ", ".join(repr(path) for path in candidates),
            context="repository inventory",
            candidates=candidates,
        )
    return RepositoryPathInventory(
        repository_root=root,
        exact_paths=frozenset(exact),
        casefolded_paths=MappingProxyType({key: tuple(value) for key, value in folded.items()}),
        allowed_roots=roots,
        allowed_paths=paths,
    )


def _same_physical_path(left: Path, right: Path) -> bool:
    return os.path.normcase(str(left)) == os.path.normcase(str(right))


def _path_has_reparse_component(root: Path, candidate: Path) -> Path | None:
    try:
        parts = candidate.relative_to(root).parts
    except ValueError:
        return candidate
    current = root
    for component in parts:
        current /= component
        try:
            stat = current.stat(follow_symlinks=False)
        except OSError as exc:
            raise _error(
                "path-inspection-failed",
                f"repository file cannot inspect {current}: {exc}",
                context="repository file",
                path_text=str(candidate),
            ) from exc
        if current.is_symlink() or (
            os.name == "nt"
            and getattr(stat, "st_file_attributes", 0) & _WINDOWS_REPARSE_POINT_ATTRIBUTE
        ):
            return current
    return None


def resolve_repository_file(
    path_text: str,
    *,
    repository_root: Path,
    inventory: RepositoryPathInventory,
    context: str,
    allowed_suffixes: Collection[str] | None = None,
) -> RepositoryFile:
    """Authenticate exact inventory spelling, then validate the physical file."""

    supplied = validate_repository_relative_path(path_text, context=context)
    root = _resolved_existing_directory(repository_root, context=context)
    if not _same_physical_path(root, inventory.repository_root):
        raise _error(
            "inventory-root-mismatch",
            f"{context}: inventory belongs to another repository root",
            context=context,
            path_text=str(root),
        )
    folded = tuple(inventory.casefolded_paths.get(supplied.casefold(), ()))
    if supplied not in inventory.exact_paths:
        if len(folded) == 1:
            raise _error(
                "wrong-case",
                f"{context} has incorrect repository path case: supplied {supplied!r}; expected {folded[0]!r}",
                context=context,
                path_text=supplied,
                expected_repository_path=folded[0],
                candidates=folded,
            )
        raise _error(
            "unknown-path",
            f"{context} is outside the authorized repository inventory: {supplied}",
            context=context,
            path_text=supplied,
        )
    if allowed_suffixes is not None:
        allowed = {suffix.casefold() for suffix in allowed_suffixes}
        if Path(supplied).suffix.casefold() not in allowed:
            raise _error(
                "disallowed-suffix",
                f"{context} has a disallowed suffix: {supplied}",
                context=context,
                path_text=supplied,
            )
    candidate = root.joinpath(*supplied.split("/"))
    if not os.path.lexists(candidate):
        raise _error(
            "repository-file-missing",
            f"{context} does not exist: {supplied}",
            context=context,
            path_text=supplied,
        )
    reparse = _path_has_reparse_component(root, candidate)
    if reparse is not None:
        raise _error(
            "reparse-component",
            f"{context} contains a reparse/symlink component: {reparse}",
            context=context,
            path_text=supplied,
        )
    try:
        physical = candidate.resolve(strict=True)
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
    return RepositoryFile(
        repository_path=supplied,
        physical_path=physical,
        repository_root=root,
    )


def diagnose_historical_repository_path(
    path_text: str,
    *,
    inventory: RepositoryPathInventory,
    current_allowed_paths: Collection[str] | None = None,
    context: str = "historical repository path",
) -> HistoricalPathResolution:
    """Reconcile old spelling for diagnosis without conferring currentness."""

    supplied = validate_repository_relative_path(path_text, context=context)
    allowed = inventory.exact_paths
    if current_allowed_paths is not None:
        allowed = frozenset(
            validate_repository_relative_path(value, context=f"{context} allowed path")
            for value in current_allowed_paths
        )
        unknown = tuple(sorted(allowed - inventory.exact_paths))
        if unknown:
            raise _error(
                "unknown-current-allowed-path",
                f"{context}: current allowed paths are outside the inventory: "
                + ", ".join(repr(value) for value in unknown),
                context=context,
                candidates=unknown,
            )
    if supplied in allowed:
        return HistoricalPathResolution(
            historical_path=supplied,
            status="exact-historical",
            current_repository_path=supplied,
            candidates=(supplied,),
        )
    folded = tuple(
        candidate
        for candidate in inventory.casefolded_paths.get(supplied.casefold(), ())
        if candidate in allowed
    )
    if len(folded) == 1:
        return HistoricalPathResolution(
            historical_path=supplied,
            status="historical-case-alias",
            current_repository_path=folded[0],
            candidates=folded,
        )
    if len(folded) > 1:
        return HistoricalPathResolution(
            historical_path=supplied,
            status="ambiguous",
            candidates=folded,
        )
    return HistoricalPathResolution(historical_path=supplied, status="missing")


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
        root for root in normalized_roots if logical == root or logical.startswith(root + "/")
    )
    if len(matches) != 1:
        raise _error(
            "generated-root-not-allowed" if not matches else "generated-root-ambiguous",
            f"{context} must be within exactly one allowed generated root",
            context=context,
            path_text=logical,
            candidates=matches,
        )
    return GeneratedRepositoryPath(logical_path=logical, allowed_root=matches[0])
