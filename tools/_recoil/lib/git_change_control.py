"""Native Git change control for governed workspace packets.

Git commit identifiers are opaque repository state. They identify the reviewed
workspace starting point; they never establish retail or candidate semantics.
Packet closeout uses Git's exact status and commit-relative name-status output.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path, PurePosixPath
import subprocess
from typing import Iterable, Mapping, Sequence


class GitChangeControlError(RuntimeError):
    pass


GIT_WORKSPACE_BASELINE_SCHEMA = "recoil-git-workspace-baseline-v2"
GIT_PACKET_POSTFLIGHT_SCHEMA = "recoil-git-packet-postflight-v3"
_GIT_WORKSPACE_BASELINE_FIELDS = frozenset({
    "schema",
    "packet_id",
    "baseline_commit",
    "branch",
    "writable_paths",
    "status_porcelain_v2",
    "ignored_paths",
    "git_object_ids_are_opaque",
})


def _run_git_text(root: Path, arguments: Sequence[str]) -> str:
    completed = subprocess.run(
        ["git", *arguments], cwd=root, check=False, capture_output=True,
        text=True, encoding="utf-8", errors="surrogateescape",
    )
    if completed.returncode != 0:
        detail = (completed.stderr or completed.stdout).strip()
        raise GitChangeControlError(
            f"Git command failed ({' '.join(arguments)}): {detail}"
        )
    return completed.stdout


def _nul_fields(value: str, *, context: str) -> list[str]:
    if not value:
        return []
    if not value.endswith("\0"):
        raise GitChangeControlError(f"{context} output has a truncated NUL record")
    return value[:-1].split("\0")


def _nul_rows(value: str, *, context: str = "Git path inventory") -> tuple[str, ...]:
    return tuple(_nul_fields(value, context=context))


@dataclass(frozen=True)
class GitPathOperation:
    record_kind: str
    status: str
    similarity: str | None
    source_path: str | None
    destination_path: str

    @property
    def paths(self) -> tuple[str, ...]:
        if self.source_path is None:
            return (self.destination_path,)
        return (self.source_path, self.destination_path)

    def to_dict(self) -> dict[str, object]:
        return {
            "record_kind": self.record_kind,
            "status": self.status,
            "similarity": self.similarity,
            "source_path": self.source_path,
            "destination_path": self.destination_path,
        }


@dataclass(frozen=True)
class GitUnmergedIndexEntry:
    mode: str
    object_id: str
    stage: int
    path: str

    def to_dict(self) -> dict[str, object]:
        return {
            "mode": self.mode,
            "object_id": self.object_id,
            "stage": self.stage,
            "path": self.path,
        }


_UNMERGED_XY_STATES = frozenset({"DD", "AU", "UD", "UA", "DU", "AA", "UU"})


def _is_unmerged_operation(operation: GitPathOperation) -> bool:
    return (
        operation.record_kind == "u"
        or operation.status in _UNMERGED_XY_STATES
        or operation.status[:1] == "U"
    )


def _status_primary_operation(primary: str, original: str | None) -> GitPathOperation:
    if primary.startswith("? ") or primary.startswith("! "):
        path = primary[2:]
        if not path or original is not None:
            raise GitChangeControlError("malformed porcelain-v2 untracked/ignored record")
        return GitPathOperation(primary[:1], primary[:1], None, None, path)
    kind = primary[:1]
    limit = {"1": 8, "2": 9, "u": 10}.get(kind)
    if limit is None:
        raise GitChangeControlError(f"unsupported porcelain-v2 record: {primary!r}")
    fields = primary.split(" ", limit)
    if len(fields) != limit + 1 or not fields[-1]:
        raise GitChangeControlError(f"malformed porcelain-v2 type-{kind} record")
    status = fields[1]
    if kind == "2":
        score = fields[8]
        if score[:1] not in {"R", "C"} or not score[1:].isdigit():
            raise GitChangeControlError("malformed porcelain-v2 rename/copy score")
        if not original:
            raise GitChangeControlError(
                "porcelain-v2 rename/copy record is missing its NUL continuation"
            )
        return GitPathOperation(kind, status, score, original, fields[-1])
    if original is not None:
        raise GitChangeControlError(
            f"porcelain-v2 type-{kind} record has an unexpected continuation"
        )
    return GitPathOperation(kind, status, None, None, fields[-1])


def parse_porcelain_v2_z(value: str) -> tuple[tuple[str, ...], tuple[GitPathOperation, ...]]:
    fields = _nul_fields(value, context="git status --porcelain=v2 -z")
    rows: list[str] = []
    operations: list[GitPathOperation] = []
    index = 0
    while index < len(fields):
        primary = fields[index]
        if not primary:
            raise GitChangeControlError("porcelain-v2 contains an empty record")
        index += 1
        original: str | None = None
        if primary.startswith("2 "):
            if index >= len(fields):
                raise GitChangeControlError(
                    "porcelain-v2 rename/copy record is missing its NUL continuation"
                )
            original = fields[index]
            index += 1
        operation = _status_primary_operation(primary, original)
        rows.append(primary if original is None else primary + "\0" + original)
        operations.append(operation)
    return tuple(rows), tuple(operations)


def parse_name_status_z(value: str) -> tuple[tuple[str, ...], tuple[GitPathOperation, ...]]:
    fields = _nul_fields(value, context="git diff --name-status -z")
    rows: list[str] = []
    operations: list[GitPathOperation] = []
    index = 0
    while index < len(fields):
        status = fields[index]
        index += 1
        if not status:
            raise GitChangeControlError("name-status contains an empty status")
        code = status[:1]
        if code not in {"A", "C", "D", "M", "R", "T", "U", "X", "B"}:
            raise GitChangeControlError(f"unsupported name-status operation: {status!r}")
        endpoint_count = 2 if code in {"R", "C"} else 1
        if index + endpoint_count > len(fields):
            raise GitChangeControlError(
                f"name-status {code} record is missing a path endpoint"
            )
        endpoints = fields[index:index + endpoint_count]
        index += endpoint_count
        if any(not path for path in endpoints):
            raise GitChangeControlError("name-status contains an empty path endpoint")
        if code in {"R", "C"}:
            if len(status) == 1 or not status[1:].isdigit():
                raise GitChangeControlError("malformed name-status rename/copy score")
            source, destination = endpoints
            rows.append("\0".join((status, source, destination)))
            operations.append(
                GitPathOperation("name-status", code, status, source, destination)
            )
        else:
            destination = endpoints[0]
            rows.append("\0".join((status, destination)))
            operations.append(
                GitPathOperation("name-status", code, None, None, destination)
            )
    return tuple(rows), tuple(operations)


def parse_unmerged_index_z(
    value: str,
) -> tuple[tuple[str, ...], tuple[GitUnmergedIndexEntry, ...]]:
    """Parse ``git ls-files -u -z`` without interpreting object identities."""
    rows = _nul_rows(value, context="git ls-files -u -z")
    entries: list[GitUnmergedIndexEntry] = []
    seen: dict[tuple[str, int], GitUnmergedIndexEntry] = {}
    for row in rows:
        if "\t" not in row:
            raise GitChangeControlError("malformed unmerged-index stage row")
        metadata, raw_path = row.split("\t", 1)
        fields = metadata.split(" ")
        if len(fields) != 3 or any(not field for field in fields):
            raise GitChangeControlError("malformed unmerged-index stage metadata")
        mode, object_id, stage_text = fields
        if len(mode) != 6 or any(character not in "01234567" for character in mode):
            raise GitChangeControlError("malformed unmerged-index mode")
        if any(character.isspace() for character in object_id):
            raise GitChangeControlError("malformed opaque unmerged-index object ID")
        if stage_text not in {"1", "2", "3"}:
            raise GitChangeControlError("malformed unmerged-index stage number")
        path = normalize_repository_path(raw_path)
        entry = GitUnmergedIndexEntry(mode, object_id, int(stage_text), path)
        key = (path, entry.stage)
        previous = seen.get(key)
        if previous is not None:
            qualifier = "inconsistent " if previous != entry else ""
            raise GitChangeControlError(
                f"duplicate-{qualifier}unmerged-index stage row for {path!r}"
            )
        seen[key] = entry
        entries.append(entry)
    return rows, tuple(sorted(entries, key=lambda entry: (
        entry.path.casefold(), entry.stage, entry.mode, entry.object_id,
    )))


def normalize_repository_path(path: str) -> str:
    value = str(path).replace("\\", "/")
    candidate = PurePosixPath(value)
    if (
        not value or value in {".", ".."} or value.startswith("/")
        or (len(value) >= 2 and value[1] == ":")
        or any(part in {"", ".", ".."} for part in candidate.parts)
    ):
        raise GitChangeControlError(
            f"repository path must be exact and relative: {path!r}"
        )
    return candidate.as_posix()


def _path_key(path: str) -> str:
    return normalize_repository_path(path).casefold()


def check_ignored_paths(root: str | Path, paths: Iterable[str]) -> tuple[str, ...]:
    repo = Path(root).resolve()
    normalized = tuple(dict.fromkeys(normalize_repository_path(path) for path in paths))
    if not normalized:
        return ()
    completed = subprocess.run(
        ["git", "check-ignore", "-z", "--stdin"], cwd=repo,
        input="\0".join(normalized) + "\0", capture_output=True, text=True,
        encoding="utf-8", errors="surrogateescape", check=False,
    )
    if completed.returncode not in {0, 1}:
        raise GitChangeControlError(
            "Git check-ignore failed: "
            + (completed.stderr or completed.stdout).strip()
        )
    return _nul_rows(completed.stdout)


def _current_branch(repo: Path) -> str:
    branch = _run_git_text(repo, ["rev-parse", "--abbrev-ref", "HEAD"]).strip()
    if not branch or branch == "HEAD":
        raise GitChangeControlError("governed packets require a named Git branch")
    return branch


def _current_head(repo: Path) -> str:
    head = _run_git_text(repo, ["rev-parse", "HEAD"]).strip()
    if not head:
        raise GitChangeControlError("Git HEAD is unavailable")
    return head


def _validate_ignored_inventory(value: object) -> list[str]:
    if not isinstance(value, list):
        raise GitChangeControlError(
            "Git workspace baseline ignored_paths must be a list"
        )
    normalized: list[str] = []
    for path in value:
        if not isinstance(path, str):
            raise GitChangeControlError(
                "Git workspace baseline ignored_paths entries must be strings"
            )
        canonical = normalize_repository_path(path)
        if path != canonical:
            raise GitChangeControlError(
                "Git workspace baseline ignored_paths must be forward-slash "
                "normalized repository-relative paths"
            )
        normalized.append(canonical)
    expected = sorted(set(normalized), key=str.casefold)
    if value != expected:
        raise GitChangeControlError(
            "Git workspace baseline ignored_paths must be sorted and unique"
        )
    return expected


def capture_clean_git_baseline(
    root: str | Path, *, packet_id: str, writable_paths: Iterable[str],
) -> dict[str, object]:
    """Require a clean authored worktree and return its minimal Git descriptor."""
    repo = Path(root).resolve()
    status_text = _run_git_text(
        repo, ["status", "--porcelain=v2", "-z", "--untracked-files=all"]
    )
    rows, operations = parse_porcelain_v2_z(status_text)
    if operations:
        dirty = sorted(
            {path for operation in operations for path in operation.paths},
            key=str.casefold,
        )
        raise GitChangeControlError(
            "governed packet requires a clean Git worktree; dirty paths: "
            + ", ".join(dirty)
        )
    return {
        "schema": GIT_WORKSPACE_BASELINE_SCHEMA,
        "packet_id": packet_id,
        "baseline_commit": _current_head(repo),
        "branch": _current_branch(repo),
        "writable_paths": sorted(
            {normalize_repository_path(path) for path in writable_paths},
            key=str.casefold,
        ),
        "status_porcelain_v2": list(rows),
        # Deprecated v2 compatibility field. Maintained authored inputs are
        # Git-visible; ignored generated or machine-local state is neither
        # packet change authority nor reconstruction evidence.
        "ignored_paths": [],
        "git_object_ids_are_opaque": True,
    }


def validate_git_baseline_descriptor(
    descriptor: Mapping[str, object], *, packet_id: str,
    writable_paths: Iterable[str],
) -> dict[str, object]:
    if descriptor.get("schema") != GIT_WORKSPACE_BASELINE_SCHEMA:
        raise GitChangeControlError("unsupported Git workspace baseline schema")
    if set(descriptor) != _GIT_WORKSPACE_BASELINE_FIELDS:
        raise GitChangeControlError(
            "Git workspace baseline descriptor fields are incomplete or unexpected"
        )
    if descriptor.get("packet_id") != packet_id:
        raise GitChangeControlError("Git workspace baseline packet identity changed")
    commit = descriptor.get("baseline_commit")
    branch = descriptor.get("branch")
    if not isinstance(commit, str) or not commit:
        raise GitChangeControlError("Git workspace baseline commit is unavailable")
    if not isinstance(branch, str) or not branch:
        raise GitChangeControlError("Git workspace baseline branch is unavailable")
    expected = sorted(
        {normalize_repository_path(path) for path in writable_paths}, key=str.casefold
    )
    if descriptor.get("writable_paths") != expected:
        raise GitChangeControlError("Git workspace baseline writable closure changed")
    status_rows = descriptor.get("status_porcelain_v2")
    if status_rows != []:
        raise GitChangeControlError(
            "Git workspace baseline must record a clean porcelain-v2 status"
        )
    ignored_paths = _validate_ignored_inventory(descriptor.get("ignored_paths"))
    if descriptor.get("git_object_ids_are_opaque") is not True:
        raise GitChangeControlError("Git object IDs must remain explicitly opaque")
    return {
        "schema": GIT_WORKSPACE_BASELINE_SCHEMA,
        "packet_id": packet_id,
        "baseline_commit": commit,
        "branch": branch,
        "writable_paths": expected,
        "status_porcelain_v2": [],
        "ignored_paths": ignored_paths,
        "git_object_ids_are_opaque": True,
    }


def reauthenticate_clean_git_baseline(
    root: str | Path,
    descriptor: Mapping[str, object],
    *,
    packet_id: str,
    writable_paths: Iterable[str],
) -> dict[str, object]:
    """Reauthenticate an immutable packet baseline before worker handoff.

    The caller supplies only the packet identity and immutable writable closure;
    branch, commit, and cleanliness are read back from Git.  Commit identifiers
    remain opaque repository state.
    """
    repo = Path(root).resolve()
    top_level = Path(
        _run_git_text(repo, ["rev-parse", "--show-toplevel"]).strip()
    ).resolve()
    if top_level != repo:
        raise GitChangeControlError(
            "workspace handoff repository root does not match the Git top level"
        )
    baseline = validate_git_baseline_descriptor(
        descriptor, packet_id=packet_id, writable_paths=writable_paths
    )
    commit = str(baseline["baseline_commit"])
    resolved = _run_git_text(
        repo,
        ["rev-parse", "--verify", "--end-of-options", f"{commit}^{{commit}}"],
    ).strip()
    if not resolved:
        raise GitChangeControlError(
            "Git workspace baseline commit is not resolvable"
        )
    branch = _current_branch(repo)
    if branch != baseline["branch"]:
        raise GitChangeControlError("governed packet Git branch changed")
    head = _current_head(repo)
    if head != commit:
        raise GitChangeControlError(
            "governed packet Git HEAD changed from its immutable baseline"
        )

    status_text = _run_git_text(
        repo, ["status", "--porcelain=v2", "-z", "--untracked-files=all"]
    )
    status_rows, status_operations = parse_porcelain_v2_z(status_text)
    unmerged_rows, unmerged_entries = parse_unmerged_index_z(
        _run_git_text(repo, ["ls-files", "-u", "-z"])
    )
    if status_operations or unmerged_entries:
        dirty = sorted(
            {
                *(path for operation in status_operations for path in operation.paths),
                *(entry.path for entry in unmerged_entries),
            },
            key=str.casefold,
        )
        raise GitChangeControlError(
            "governed packet handoff requires its exact clean Git baseline; "
            "dirty or unmerged paths: " + ", ".join(dirty)
        )
    return {
        **baseline,
        "repository_root": str(repo),
        "worktree_root": str(repo),
        "current_head": head,
        "current_branch": branch,
        "status_porcelain_v2": list(status_rows),
        "unmerged_index_rows": list(unmerged_rows),
        "unmerged_index_entries": [entry.to_dict() for entry in unmerged_entries],
    }


def _deduplicate_operations(
    operations: Iterable[GitPathOperation],
) -> tuple[GitPathOperation, ...]:
    by_key: dict[tuple[object, ...], GitPathOperation] = {}
    for operation in operations:
        key = (operation.status[:1], operation.source_path, operation.destination_path)
        if key not in by_key or operation.record_kind == "name-status":
            by_key[key] = operation
    return tuple(sorted(by_key.values(), key=lambda row: (
        row.status[:1], (row.source_path or "").casefold(),
        row.destination_path.casefold(),
    )))


def capture_git_closeout(
    root: str | Path, descriptor: Mapping[str, object], *, packet_id: str,
    writable_paths: Iterable[str],
) -> dict[str, object]:
    """Inventory all commit-relative packet changes and enforce write closure."""
    repo = Path(root).resolve()
    baseline = validate_git_baseline_descriptor(
        descriptor, packet_id=packet_id, writable_paths=writable_paths
    )
    if _current_branch(repo) != baseline["branch"]:
        raise GitChangeControlError("governed packet Git branch changed")
    status_text = _run_git_text(
        repo, ["status", "--porcelain=v2", "-z", "--untracked-files=all"]
    )
    status_rows, status_operations = parse_porcelain_v2_z(status_text)
    diff_text = _run_git_text(
        repo,
        ["diff", "--no-ext-diff", "--name-status", "-z", "--find-renames",
         str(baseline["baseline_commit"]), "--"],
    )
    diff_rows, diff_operations = parse_name_status_z(diff_text)
    unmerged_index_rows, unmerged_index_entries = parse_unmerged_index_z(
        _run_git_text(repo, ["ls-files", "-u", "-z"])
    )
    untracked = _nul_rows(_run_git_text(
        repo, ["ls-files", "-z", "--others", "--exclude-standard"]
    ))
    operations = _deduplicate_operations((*diff_operations, *status_operations))
    writable = {_path_key(path) for path in writable_paths}
    violations: list[dict[str, object]] = []
    changed_paths: set[str] = set(untracked)
    copy_sources: set[str] = set()
    independently_changed_path_keys: set[str] = set()
    unmerged_paths: set[str] = {
        entry.path for entry in unmerged_index_entries
    }
    for operation in operations:
        code = operation.status[:1]
        changed_paths.update(operation.paths)
        if code != "C":
            independently_changed_path_keys.update(
                _path_key(path) for path in operation.paths
            )
        if _is_unmerged_operation(operation):
            unmerged_paths.update(
                normalize_repository_path(path) for path in operation.paths
            )
        destination_allowed = _path_key(operation.destination_path) in writable
        source_allowed = True
        if code == "R" and operation.source_path is not None:
            source_allowed = _path_key(operation.source_path) in writable
        if code == "C" and operation.source_path is not None:
            copy_sources.add(operation.source_path)
        if not source_allowed or not destination_allowed:
            violations.append({
                "operation": code,
                "status": operation.status,
                "similarity": operation.similarity,
                "source_path": operation.source_path,
                "destination_path": operation.destination_path,
                "source_authorized": source_allowed,
                "destination_authorized": destination_allowed,
            })
    independently_changed_path_keys.update(_path_key(path) for path in unmerged_paths)
    unchanged_copy_source_keys = {
        _path_key(path)
        for path in copy_sources
        if _path_key(path) not in independently_changed_path_keys
    }
    changed_paths = {
        path for path in changed_paths
        if _path_key(path) not in unchanged_copy_source_keys
    }
    changed_paths.update(unmerged_paths)
    unexpected = sorted(
        {path for path in changed_paths if _path_key(path) not in writable},
        key=str.casefold,
    )
    unexpected = sorted(
        set(unexpected)
        | unmerged_paths,
        key=str.casefold,
    )
    return {
        "schema": GIT_PACKET_POSTFLIGHT_SCHEMA,
        "passed": not unexpected and not violations and not unmerged_paths,
        "packet_id": packet_id,
        "baseline_schema": baseline["schema"],
        "baseline_commit": baseline["baseline_commit"],
        "branch": baseline["branch"],
        "operations": [operation.to_dict() for operation in operations],
        "changed_paths": sorted(changed_paths, key=str.casefold),
        "unexpected_paths": unexpected,
        "endpoint_violations": violations,
        "status_porcelain_v2": list(status_rows),
        "diff_name_status": list(diff_rows),
        "unmerged_paths": sorted(unmerged_paths, key=str.casefold),
        "unmerged_index_entries": [
            entry.to_dict() for entry in unmerged_index_entries
        ],
        "unmerged_index_rows": list(unmerged_index_rows),
        "ignored_added_paths": [],
        "ignored_removed_paths": [],
        "ignored_delta_fields_deprecated": True,
        "ignored_generated_paths_packet_gated": False,
        "ignored_generated_paths_inspected": False,
        "ignored_generated_policy": "nonauthoritative-generated-state",
        "ignored_content_comparison_performed": False,
        "untracked_paths": list(untracked),
        "diff_stat": _run_git_text(
            repo, ["diff", "--no-ext-diff", "--stat",
                   str(baseline["baseline_commit"]), "--"]
        ),
        "ordinary_copy_rule": "unchanged-source-destination-only",
        "read_dependencies_are_write_authority": False,
        "git_object_ids_are_opaque": True,
    }


__all__ = [
    "GIT_PACKET_POSTFLIGHT_SCHEMA", "GIT_WORKSPACE_BASELINE_SCHEMA",
    "GitChangeControlError", "GitPathOperation", "GitUnmergedIndexEntry",
    "capture_clean_git_baseline",
    "capture_git_closeout", "check_ignored_paths", "normalize_repository_path",
    "parse_name_status_z", "parse_porcelain_v2_z", "parse_unmerged_index_z",
    "reauthenticate_clean_git_baseline", "validate_git_baseline_descriptor",
]
