#!/usr/bin/env python3
"""Report and optionally delete generated artifacts or governed session scratch."""

from __future__ import annotations

import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse
from collections import Counter
from dataclasses import dataclass
import os
import re
import stat
import time
from typing import Any

from _recoil.lib.progress import DEFAULT_PROGRESS_PATH, ProgressDocument, ProgressError
from _recoil.lib.tooling import REPO_ROOT, configure_stdio, display_path


DEFAULT_OLDER_THAN_DAYS = 14

SESSION_SCRATCH_NAME = ".devspace"

DURABLE_REFERENCE_ROOTS = (
    "src",
    "tests/native",
    "docs/reconstruction",
    "tools/functional_verify_targets",
    "tools/vc5_verify_targets",
)

DURABLE_REFERENCE_FILES = (
    ".agent/RAW_ASSEMBLY_ALLOWLIST.txt",
    ".agent/RAW_ADDRESS_ALLOWLIST.txt",
)

DEVSPACE_REFERENCE_RE = re.compile(
    r"\.devspace[\\/](?:runs|context-bundles?|chatgpt-history|state|chrome-profiles?)"
    r"(?:[\\/][^\s`\"'|]*|\.[A-Za-z0-9_-]+)?",
    re.IGNORECASE,
)
PLACEHOLDER_RE = re.compile(r"<[^>]+>|\{[^}]+\}")

BUILD_ROOT_NAME = "build"
LOCAL_ARTIFACT_ROOT_NAMES = {".vs", "playground"}

IO_REPARSE_TAG_MOUNT_POINT = 0xA0000003
IO_REPARSE_TAG_SYMLINK = 0xA000000C


@dataclass(frozen=True)
class ArtifactRoot:
    path: Path
    size_bytes: int
    age_days: float
    selected: bool
    reason: str
    direct_entry_count: int = 0
    retain_root: bool = False


@dataclass(frozen=True)
class DurableReference:
    path: Path
    line: int
    reference: str


def directory_size(path: Path) -> int:
    """Return local file bytes without following any reparse point."""

    total = 0

    def visit(directory: Path) -> None:
        nonlocal total
        try:
            entries = list(os.scandir(directory))
        except OSError:
            return
        for entry in entries:
            item = Path(entry.path)
            try:
                stat_result = entry.stat(follow_symlinks=False)
            except OSError:
                continue
            if _reparse_kind(item, stat_result) is not None:
                continue
            if stat.S_ISDIR(stat_result.st_mode):
                visit(item)
            elif stat.S_ISREG(stat_result.st_mode):
                total += stat_result.st_size

    visit(path)
    return total


def artifact_age_days(path: Path, now: float) -> float:
    try:
        mtime = path.stat().st_mtime
    except OSError:
        mtime = now
    return max((now - mtime) / 86400.0, 0.0)


def selected_by_age(age_days: float, *, older_than_days: int, all_ages: bool) -> bool:
    return all_ages or age_days >= older_than_days


def _has_reparse_point(path: Path) -> bool:
    """Return true for symlinks and Windows junction/reparse points."""

    try:
        stat_result = path.lstat()
    except OSError:
        return False
    attributes = getattr(stat_result, "st_file_attributes", 0)
    reparse_flag = getattr(stat_result, "FILE_ATTRIBUTE_REPARSE_POINT", 0x400)
    return path.is_symlink() or bool(attributes & reparse_flag)


def _reparse_kind(path: Path, stat_result: os.stat_result | None = None) -> str | None:
    """Classify a reparse point without following it.

    Symlinks and mount-point junctions are the only known removable kinds.
    Any other reparse tag is intentionally reported as unknown and rejected by
    preflight before cleanup mutates any target.
    """

    try:
        result = path.lstat() if stat_result is None else stat_result
    except OSError:
        return None
    attributes = getattr(result, "st_file_attributes", 0)
    reparse_flag = getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0x400)
    if not path.is_symlink() and not bool(attributes & reparse_flag):
        return None
    tag = getattr(result, "st_reparse_tag", None)
    if path.is_symlink() or tag == IO_REPARSE_TAG_SYMLINK:
        return "symlink"
    if tag == IO_REPARSE_TAG_MOUNT_POINT:
        return "junction"
    is_junction = getattr(path, "is_junction", None)
    if callable(is_junction):
        try:
            if is_junction():
                return "junction"
        except OSError:
            pass
    return "unknown-reparse"


def _direct_entry_count(path: Path) -> int:
    try:
        with os.scandir(path) as entries:
            return sum(1 for _entry in entries)
    except OSError:
        return 0


def validate_repository_root(root: Path) -> tuple[Path | None, list[str]]:
    """Validate the lexical selected root before resolving it.

    This intentionally checks the exact caller-selected path with ``lstat`` so
    a root symlink or junction cannot disappear through ``resolve()`` before
    the reparse-point policy is enforced.
    """

    lexical_root = root.absolute()
    if not os.path.lexists(lexical_root):
        return None, [f"repository root does not exist: {lexical_root}"]
    try:
        root_stat = lexical_root.lstat()
    except OSError as exc:
        return None, [f"cannot inspect repository root {lexical_root}: {exc}"]
    kind = _reparse_kind(lexical_root, root_stat)
    if kind is not None:
        return None, [
            f"repository root must be a real directory, not a symlink, junction, "
            f"or reparse point ({kind}): {lexical_root}"
        ]
    if not stat.S_ISDIR(root_stat.st_mode):
        return None, [f"repository root must be a real directory: {lexical_root}"]
    try:
        resolved_root = lexical_root.resolve(strict=True)
    except OSError as exc:
        return None, [f"repository root cannot be resolved: {lexical_root}: {exc}"]
    return resolved_root, []


def validate_session_scratch(root: Path, target: Path) -> list[str]:
    errors: list[str] = []
    absolute_root = root.absolute()
    absolute_target = target.absolute()
    if absolute_target.name != SESSION_SCRATCH_NAME or absolute_target.parent != absolute_root:
        errors.append(f"session scratch must be the repository's direct {SESSION_SCRATCH_NAME} child: {target}")
        return errors
    if target.exists() or target.is_symlink():
        if _has_reparse_point(target):
            errors.append(f"refusing session scratch symlink, junction, or reparse point: {target}")
            return errors
        if not target.is_dir():
            errors.append(f"refusing non-directory session scratch path: {target}")
            return errors
        try:
            target.resolve(strict=True).relative_to(root.resolve(strict=True))
        except (OSError, ValueError):
            errors.append(f"refusing session scratch path outside repository root: {target}")
    return errors


def _iter_durable_reference_files(root: Path):
    seen: set[Path] = set()
    for relative in DURABLE_REFERENCE_ROOTS:
        base = root / relative
        if not base.is_dir():
            continue
        for path in sorted(item for item in base.rglob("*") if item.is_file()):
            resolved = path.resolve()
            if resolved not in seen:
                seen.add(resolved)
                yield path
    for relative in DURABLE_REFERENCE_FILES:
        path = root / relative
        if path.is_file() and path.resolve() not in seen:
            seen.add(path.resolve())
            yield path
def progress_tracker_path(root: Path) -> Path:
    """Resolve the canonical SQLite tracker."""

    relative = DEFAULT_PROGRESS_PATH.relative_to(REPO_ROOT)
    return root / relative


def load_progress_tracker_data(root: Path) -> dict[str, Any] | None:
    """Load tracker semantics without treating the SQLite authority as text."""

    tracker_path = progress_tracker_path(root)
    if not tracker_path.is_file():
        return None
    try:
        data = ProgressDocument.load(tracker_path).data
    except (OSError, ValueError, ProgressError):
        return None
    return data if isinstance(data, dict) else None


def _is_concrete_devspace_reference(reference: str) -> bool:
    if PLACEHOLDER_RE.search(reference):
        return False
    normalized = reference.replace("\\", "/").rstrip(".,;:)]}")
    marker = ".devspace/"
    suffix = normalized.lower().split(marker, 1)[-1]
    components = [part for part in suffix.split("/") if part]
    if len(components) >= 2:
        return True
    return bool(components and "." in components[0])


def _find_all_durable_devspace_references(root: Path) -> list[DurableReference]:
    findings: list[DurableReference] = []
    for path in _iter_durable_reference_files(root):
        try:
            text = path.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError):
            continue
        for line_number, line in enumerate(text.splitlines(), start=1):
            for match in DEVSPACE_REFERENCE_RE.finditer(line):
                reference = match.group(0).rstrip(".,;:)]}")
                if _is_concrete_devspace_reference(reference):
                    findings.append(DurableReference(path, line_number, reference))
    tracker_path = progress_tracker_path(root)
    if tracker_path.suffix.lower() != ".json":
        tracker = load_progress_tracker_data(root)
        if tracker is not None:
            findings.extend(
                DurableReference(tracker_path, 0, reference)
                for reference in _concrete_devspace_references(tracker)
            )
    return findings


def _concrete_devspace_references(value: Any) -> list[str]:
    references: list[str] = []
    if isinstance(value, str):
        for match in DEVSPACE_REFERENCE_RE.finditer(value):
            reference = match.group(0).rstrip(".,;:)]}")
            if _is_concrete_devspace_reference(reference):
                references.append(reference)
    elif isinstance(value, dict):
        for child in value.values():
            references.extend(_concrete_devspace_references(child))
    elif isinstance(value, list):
        for child in value:
            references.extend(_concrete_devspace_references(child))
    return references


def _contains_exact_string(value: Any, expected: str) -> bool:
    if isinstance(value, str):
        return value == expected
    if isinstance(value, dict):
        return any(_contains_exact_string(child, expected) for child in value.values())
    if isinstance(value, list):
        return any(_contains_exact_string(child, expected) for child in value)
    return False


def _artifact_is_durable(artifact: Any) -> bool:
    if not isinstance(artifact, dict):
        return False
    path = artifact.get("path")
    size = artifact.get("size")
    return (
        isinstance(path, str)
        and bool(path.strip())
        and not _concrete_devspace_references(path)
        and isinstance(size, int)
        and not isinstance(size, bool)
        and size >= 0
    )


def _replacement_historical_metadata(
    replacement: dict[str, Any],
    *,
    old_id: str,
    old_record: dict[str, Any],
) -> tuple[bool, list[str]]:
    """Separate narrowly recognized supersession history from live dependencies."""

    provenance = replacement.get("provenance")
    active_replacement = dict(replacement)
    historical_scratch: list[str] = []
    if isinstance(provenance, dict):
        active_provenance = dict(provenance)
        if active_provenance.get("supersedes_evidence_id") == old_id:
            del active_provenance["supersedes_evidence_id"]

        removed_reference = active_provenance.get("temporary_reference_removed")
        if removed_reference is not None:
            if isinstance(removed_reference, str):
                removed_values = [removed_reference]
            elif isinstance(removed_reference, list):
                removed_values = removed_reference
            else:
                return False, []
            if not all(isinstance(item, str) and bool(item.strip()) for item in removed_values):
                return False, []
            candidate_scratch = _concrete_devspace_references(removed_values)
            old_scratch = Counter(_concrete_devspace_references(old_record))
            candidate_counts = Counter(candidate_scratch)
            if candidate_scratch and all(
                count <= old_scratch[reference] for reference, count in candidate_counts.items()
            ):
                historical_scratch = candidate_scratch
                del active_provenance["temporary_reference_removed"]

        active_replacement["provenance"] = active_provenance

    if _contains_exact_string(active_replacement, old_id):
        return False, []
    if _concrete_devspace_references(active_replacement):
        return False, []
    return True, historical_scratch


def _historical_superseded_devspace_allowance(root: Path) -> Counter[str]:
    """Count scratch paths retained only in valid immutable evidence tombstones."""

    data = load_progress_tracker_data(root)
    if data is None:
        return Counter()
    evidence = data.get("evidence")
    tombstones = data.get("tombstones")
    if not isinstance(evidence, dict) or not isinstance(tombstones, dict):
        return Counter()

    active_state = {
        key: value for key, value in data.items() if key not in {"evidence", "tombstones"}
    }
    allowed: Counter[str] = Counter()
    for tombstone_id, tombstone in tombstones.items():
        if not isinstance(tombstone, dict) or tombstone.get("entity_kind") != "evidence":
            continue
        old_id = tombstone.get("removed_id")
        replacement_id = tombstone.get("replacement_evidence_id")
        old_record = tombstone.get("record")
        reference_paths = tombstone.get("reference_paths")
        binary = tombstone.get("binary")
        if not all(isinstance(item, str) and item for item in (old_id, replacement_id, binary)):
            continue
        if not isinstance(tombstone_id, str) or not tombstone_id:
            continue
        if (
            not isinstance(reference_paths, list)
            or not all(isinstance(item, str) and bool(item.strip()) for item in reference_paths)
            or not isinstance(old_record, dict)
        ):
            continue
        replacement = evidence.get(replacement_id)
        if not isinstance(replacement, dict):
            continue
        other_evidence = {key: value for key, value in evidence.items() if key != replacement_id}
        if (
            old_id in evidence
            or _contains_exact_string(active_state, old_id)
            or _contains_exact_string(other_evidence, old_id)
        ):
            continue
        if old_record.get("kind") != replacement.get("kind"):
            continue
        old_scopes = old_record.get("scope_ids", [])
        replacement_scopes = replacement.get("scope_ids", [])
        if not isinstance(old_scopes, list) or not isinstance(replacement_scopes, list):
            continue
        if not set(str(item) for item in old_scopes).issubset(str(item) for item in replacement_scopes):
            continue
        if any(old_record.get(field) != replacement.get(field) for field in ("result", "disposition")):
            continue
        artifacts = replacement.get("artifacts")
        if artifacts is not None and (
            not isinstance(artifacts, list)
            or not all(_artifact_is_durable(item) for item in artifacts)
        ):
            continue
        history_valid, replacement_history = _replacement_historical_metadata(
            replacement,
            old_id=old_id,
            old_record=old_record,
        )
        if not history_valid:
            continue
        allowed.update(_concrete_devspace_references(old_record))
        allowed.update(replacement_history)
    return allowed


def find_durable_devspace_references(root: Path) -> list[DurableReference]:
    """Find active durable scratch dependencies, preserving valid history.

    A concrete path is ignored only when that exact occurrence lives in a
    historical evidence tombstone whose replacement has the same semantic
    kind/scope/outcome and contains no active scratch dependency.
    """

    findings = _find_all_durable_devspace_references(root)
    tracker_path = progress_tracker_path(root).resolve()
    allowed = _historical_superseded_devspace_allowance(root)
    actionable: list[DurableReference] = []
    for finding in findings:
        if finding.path.resolve() == tracker_path and allowed[finding.reference] > 0:
            allowed[finding.reference] -= 1
            continue
        actionable.append(finding)
    return actionable


def collect_session_artifacts(root: Path, *, now: float | None = None) -> tuple[list[ArtifactRoot], list[str]]:
    target = root / SESSION_SCRATCH_NAME
    errors = validate_session_scratch(root, target)
    if errors or not target.exists():
        return [], errors
    current_time = time.time() if now is None else now
    return [
        ArtifactRoot(
            target,
            directory_size(target),
            artifact_age_days(target, current_time),
            True,
            "per-session scratch",
            _direct_entry_count(target),
        )
    ], []


def collect_artifacts(
    root: Path,
    *,
    older_than_days: int = DEFAULT_OLDER_THAN_DAYS,
    all_ages: bool = False,
    include_vs: bool = False,
    include_playground: bool = False,
    now: float | None = None,
) -> list[ArtifactRoot]:
    current_time = time.time() if now is None else now
    artifacts: list[ArtifactRoot] = []
    build_root = root / BUILD_ROOT_NAME
    if build_root.is_dir() and not _has_reparse_point(build_root):
        artifacts.append(
            ArtifactRoot(
                build_root,
                directory_size(build_root),
                artifact_age_days(build_root, current_time),
                True,
                "all build contents; name and age filters do not apply",
                _direct_entry_count(build_root),
                True,
            )
        )

    for name, include_flag in ((".vs", include_vs), ("playground", include_playground)):
        path = root / name
        if not path.is_dir() or _has_reparse_point(path):
            continue
        age_days = artifact_age_days(path, current_time)
        selected = include_flag and selected_by_age(age_days, older_than_days=older_than_days, all_ages=all_ages)
        if selected:
            reason = "explicitly included local artifact root"
        elif include_flag:
            reason = "newer than threshold"
        else:
            reason = f"requires --include-{name.lstrip('.')}"
        artifacts.append(
            ArtifactRoot(
                path,
                directory_size(path),
                age_days,
                selected,
                reason,
                _direct_entry_count(path),
            )
        )

    return artifacts


def format_size(size_bytes: int) -> str:
    value = float(size_bytes)
    for suffix in ("B", "KB", "MB", "GB"):
        if value < 1024.0 or suffix == "GB":
            return f"{value:.1f} {suffix}"
        value /= 1024.0
    return f"{value:.1f} GB"


def _validate_real_directory(root: Path, path: Path, *, expected_name: str) -> list[str]:
    errors: list[str] = []
    absolute_root = root.absolute()
    absolute_path = path.absolute()
    if absolute_path.name != expected_name or absolute_path.parent != absolute_root:
        return [f"cleanup target must be the repository's direct {expected_name} child: {path}"]
    if not os.path.lexists(path):
        return []
    try:
        result = path.lstat()
    except OSError as exc:
        return [f"cannot inspect cleanup target {path}: {exc}"]
    kind = _reparse_kind(path, result)
    if kind is not None:
        errors.append(f"refusing cleanup target symlink, junction, or reparse point ({kind}): {path}")
        return errors
    if not stat.S_ISDIR(result.st_mode):
        errors.append(f"refusing non-directory cleanup target: {path}")
        return errors
    try:
        resolved_root = root.resolve(strict=True)
        resolved_path = path.resolve(strict=True)
    except OSError as exc:
        errors.append(f"cannot resolve cleanup target {path}: {exc}")
        return errors
    if resolved_path.parent != resolved_root:
        errors.append(f"refusing cleanup target outside repository root: {path} -> {resolved_path}")
    return errors


def validate_artifact_target(root: Path, artifact: ArtifactRoot) -> list[str]:
    """Validate one selected target's exact governed root before traversal."""

    if not artifact.selected:
        return []
    if artifact.retain_root:
        if artifact.path.absolute() != (root / BUILD_ROOT_NAME).absolute():
            return [f"refusing retained-root cleanup outside direct build directory: {artifact.path}"]
        return _validate_real_directory(root, artifact.path, expected_name=BUILD_ROOT_NAME)
    expected_names = LOCAL_ARTIFACT_ROOT_NAMES | {SESSION_SCRATCH_NAME}
    if artifact.path.name not in expected_names:
        return [f"refusing ungoverned artifact cleanup target: {artifact.path}"]
    return _validate_real_directory(root, artifact.path, expected_name=artifact.path.name)


def _preflight_tree(path: Path, *, root: Path) -> list[str]:
    """Reject unreadable entries and unknown reparse types without following links."""

    errors: list[str] = []
    try:
        with os.scandir(path) as iterator:
            entries = list(iterator)
    except OSError as exc:
        return [f"{display_path(path, root)}: cannot enumerate before cleanup: {exc}"]
    for entry in entries:
        child = Path(entry.path)
        try:
            result = entry.stat(follow_symlinks=False)
        except OSError as exc:
            errors.append(f"{display_path(child, root)}: cannot inspect before cleanup: {exc}")
            continue
        kind = _reparse_kind(child, result)
        if kind == "unknown-reparse":
            tag = getattr(result, "st_reparse_tag", None)
            suffix = f" tag=0x{tag:08x}" if isinstance(tag, int) else ""
            errors.append(
                f"{display_path(child, root)}: refusing unknown nested reparse point{suffix}"
            )
        elif kind is None and stat.S_ISDIR(result.st_mode):
            errors.extend(_preflight_tree(child, root=root))
    return errors


def preflight_selected(root: Path, artifacts: list[ArtifactRoot]) -> list[str]:
    """Validate every selected target and tree before any cleanup mutation."""

    errors: list[str] = []
    selected = [artifact for artifact in artifacts if artifact.selected]
    for artifact in selected:
        errors.extend(validate_artifact_target(root, artifact))
    if errors:
        return errors
    for artifact in selected:
        if os.path.lexists(artifact.path):
            errors.extend(_preflight_tree(artifact.path, root=root))
    return errors


def _retry_readonly(
    path: Path,
    operation,
    *,
    root: Path,
    action: str,
    expected_directory: bool,
) -> str | None:
    try:
        operation()
        return None
    except OSError as first:
        try:
            retry_stat = path.lstat()
            retry_kind = _reparse_kind(path, retry_stat)
            retry_is_directory = stat.S_ISDIR(retry_stat.st_mode)
            if retry_kind is not None or retry_is_directory != expected_directory:
                actual = retry_kind or ("directory" if retry_is_directory else "non-directory")
                return (
                    f"{display_path(path, root)}: {action} failed safely after target changed "
                    f"to {actual}: {first}"
                )
            os.chmod(path, stat.S_IREAD | stat.S_IWRITE)
            operation()
            return None
        except OSError as second:
            return f"{display_path(path, root)}: {action} failed: {second} (initial error: {first})"


def _unlink_known_reparse(path: Path, kind: str, *, root: Path) -> str | None:
    action = "junction unlink" if kind == "junction" else "symlink unlink"
    operation = (lambda: os.rmdir(path)) if kind == "junction" else path.unlink
    try:
        operation()
    except OSError as exc:
        return f"{display_path(path, root)}: {action} failed: {exc}"
    return None


def _remove_tree_contents(path: Path, *, root: Path) -> list[str]:
    errors: list[str] = []
    try:
        root_stat = path.lstat()
    except OSError as exc:
        return [f"{display_path(path, root)}: cannot inspect cleanup directory: {exc}"]
    root_kind = _reparse_kind(path, root_stat)
    if root_kind is not None or not stat.S_ISDIR(root_stat.st_mode):
        actual = root_kind or "non-directory"
        return [f"{display_path(path, root)}: refusing changed cleanup directory ({actual})"]
    try:
        with os.scandir(path) as iterator:
            entries = list(iterator)
    except OSError as exc:
        return [f"{display_path(path, root)}: cannot enumerate during cleanup: {exc}"]
    for entry in entries:
        child = Path(entry.path)
        try:
            result = entry.stat(follow_symlinks=False)
        except OSError as exc:
            errors.append(f"{display_path(child, root)}: cannot inspect during cleanup: {exc}")
            continue
        kind = _reparse_kind(child, result)
        if kind in {"symlink", "junction"}:
            error = _unlink_known_reparse(child, kind, root=root)
            if error:
                errors.append(error)
        elif kind == "unknown-reparse":
            errors.append(f"{display_path(child, root)}: unknown reparse point appeared after preflight")
        elif stat.S_ISDIR(result.st_mode):
            errors.extend(_remove_tree_contents(child, root=root))
            if os.path.lexists(child):
                try:
                    after_stat = child.lstat()
                except OSError as exc:
                    errors.append(f"{display_path(child, root)}: cannot recheck directory: {exc}")
                    continue
                after_kind = _reparse_kind(child, after_stat)
                if after_kind in {"symlink", "junction"}:
                    error = _unlink_known_reparse(child, after_kind, root=root)
                elif after_kind == "unknown-reparse":
                    error = f"{display_path(child, root)}: unknown reparse point appeared during cleanup"
                elif not stat.S_ISDIR(after_stat.st_mode):
                    error = f"{display_path(child, root)}: directory changed to non-directory during cleanup"
                else:
                    error = _retry_readonly(
                        child,
                        lambda: os.rmdir(child),
                        root=root,
                        action="directory removal",
                        expected_directory=True,
                    )
                if error:
                    errors.append(error)
        else:
            error = _retry_readonly(
                child,
                child.unlink,
                root=root,
                action="file removal",
                expected_directory=False,
            )
            if error:
                errors.append(error)
    return errors


def _verify_real_empty_directory(root: Path, path: Path) -> list[str]:
    errors = _validate_real_directory(root, path, expected_name=BUILD_ROOT_NAME)
    if errors:
        return errors
    if not path.exists():
        return [f"{display_path(path, root)}: cleanup verification failed; retained build directory is absent"]
    try:
        with os.scandir(path) as entries:
            remaining = [entry.name for entry in entries]
    except OSError as exc:
        return [f"{display_path(path, root)}: cleanup verification failed: {exc}"]
    if remaining:
        preview = ", ".join(sorted(remaining)[:5])
        errors.append(
            f"{display_path(path, root)}: cleanup verification failed; directory is not empty: {preview}"
        )
    return errors


def delete_selected(root: Path, artifacts: list[ArtifactRoot]) -> list[str]:
    """Preflight every selected root, then safely remove the selected content."""

    errors = preflight_selected(root, artifacts)
    if errors:
        return errors
    for artifact in (item for item in artifacts if item.selected):
        if not os.path.lexists(artifact.path):
            continue
        target_errors = validate_artifact_target(root, artifact)
        if target_errors:
            errors.extend(target_errors)
            continue
        errors.extend(_remove_tree_contents(artifact.path, root=root))
        if artifact.retain_root:
            errors.extend(_verify_real_empty_directory(root, artifact.path))
        else:
            error = _retry_readonly(
                artifact.path,
                lambda: os.rmdir(artifact.path),
                root=root,
                action="artifact-root removal",
                expected_directory=True,
            )
            if error:
                errors.append(error)
            elif os.path.lexists(artifact.path):
                errors.append(
                    f"{display_path(artifact.path, root)}: deletion verification failed; path remains"
                )
    return errors


def print_report(artifacts: list[ArtifactRoot], *, root: Path, delete: bool) -> None:
    selected = [artifact for artifact in artifacts if artifact.selected]
    total_selected_size = sum(artifact.size_bytes for artifact in selected)
    total_direct_entries = sum(artifact.direct_entry_count for artifact in selected)
    print("artifact audit summary:")
    print(f"- targets scanned: {len(artifacts)}")
    print(f"- selected for cleanup: {len(selected)}")
    print(f"- selected size: {format_size(total_selected_size)}")
    print(f"- selected direct entries: {total_direct_entries}")
    if not artifacts:
        print("No generated artifact roots found.")
        return
    print()
    for artifact in artifacts:
        marker = "cleaned" if delete and artifact.selected else "cleanup" if artifact.selected else "keep"
        if not artifact.selected:
            disposition = "root unchanged"
        elif artifact.retain_root:
            disposition = "root retained"
        else:
            disposition = "cleanup removes root"
        print(
            f"- [{marker}] {display_path(artifact.path, root)} "
            f"({format_size(artifact.size_bytes)}, {artifact.direct_entry_count} direct entries, "
            f"{artifact.age_days:.1f} days; {disposition}): {artifact.reason}"
        )
    if selected:
        print()
        if delete:
            if any(artifact.retain_root for artifact in selected):
                print("Cleanup completed; the direct build directory is retained and empty.")
            else:
                print("Cleanup completed.")
        else:
            print("Would clean these targets. Re-run with --delete to apply cleanup.")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Dry-run or empty the repository's direct build directory, optionally clean local "
            "artifact roots, or clean governed session scratch."
        )
    )
    parser.add_argument("--root", default=str(REPO_ROOT), help="Repository root to scan.")
    parser.add_argument(
        "--older-than-days",
        type=int,
        default=DEFAULT_OLDER_THAN_DAYS,
        help="Age threshold for opted-in .vs/playground only; build contents are always selected.",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="Ignore age for opted-in .vs/playground only; build contents are always selected.",
    )
    parser.add_argument("--include-vs", action="store_true", help="Opt .vs into age-filtered cleanup.")
    parser.add_argument(
        "--include-playground", action="store_true", help="Opt playground into age-filtered cleanup."
    )
    parser.add_argument(
        "--session-only",
        action="store_true",
        help="Audit only repository .devspace session scratch, regardless of age.",
    )
    parser.add_argument(
        "--delete",
        action="store_true",
        help="Apply cleanup. Normal mode retains build/ but removes every entry beneath it.",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = build_parser()
    args = parser.parse_args(argv)
    root, root_errors = validate_repository_root(Path(args.root))
    for error in root_errors:
        print(f"ERROR: {error}", file=sys.stderr)
    if root_errors or root is None:
        return 1
    if args.session_only:
        conflicts: list[str] = []
        if args.all:
            conflicts.append("--all")
        if args.include_vs:
            conflicts.append("--include-vs")
        if args.include_playground:
            conflicts.append("--include-playground")
        if args.older_than_days != DEFAULT_OLDER_THAN_DAYS:
            conflicts.append("--older-than-days")
        if conflicts:
            print(
                "ERROR: --session-only cannot be combined with " + ", ".join(conflicts),
                file=sys.stderr,
            )
            return 2
        references = find_durable_devspace_references(root)
        if references:
            print("ERROR: durable workspace surfaces depend on concrete .devspace artifacts:", file=sys.stderr)
            for finding in references:
                print(
                    f"- {display_path(finding.path, root)}:{finding.line}: {finding.reference}",
                    file=sys.stderr,
                )
            print("Promote the material evidence and remove these dependencies before cleanup.", file=sys.stderr)
            return 1
        artifacts, safety_errors = collect_session_artifacts(root)
        for error in safety_errors:
            print(f"ERROR: {error}", file=sys.stderr)
        if safety_errors:
            return 1
        if not args.delete:
            print_report(artifacts, root=root, delete=False)
            return 0
        safety_errors = validate_session_scratch(root, root / SESSION_SCRATCH_NAME)
        for error in safety_errors:
            print(f"ERROR: {error}", file=sys.stderr)
        if safety_errors:
            return 1
        errors = delete_selected(root, artifacts)
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        if errors:
            return 1
        print_report(artifacts, root=root, delete=True)
        return 0

    build_root = root / BUILD_ROOT_NAME
    build_errors = _validate_real_directory(root, build_root, expected_name=BUILD_ROOT_NAME)
    for error in build_errors:
        print(f"ERROR: {error}", file=sys.stderr)
    if build_errors:
        return 1
    local_root_errors: list[str] = []
    for name, included in ((".vs", args.include_vs), ("playground", args.include_playground)):
        path = root / name
        if included and os.path.lexists(path):
            local_root_errors.extend(_validate_real_directory(root, path, expected_name=name))
    for error in local_root_errors:
        print(f"ERROR: {error}", file=sys.stderr)
    if local_root_errors:
        return 1
    artifacts = collect_artifacts(
        root,
        older_than_days=args.older_than_days,
        all_ages=args.all,
        include_vs=args.include_vs,
        include_playground=args.include_playground,
    )
    safety_errors = preflight_selected(root, artifacts)
    for error in safety_errors:
        print(f"ERROR: {error}", file=sys.stderr)
    if safety_errors:
        return 1
    if not args.delete:
        print_report(artifacts, root=root, delete=False)
        return 0
    errors = delete_selected(root, artifacts)
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    if errors:
        return 1
    print_report(artifacts, root=root, delete=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
