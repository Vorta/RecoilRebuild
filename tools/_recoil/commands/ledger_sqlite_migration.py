from __future__ import annotations

"""One-shot, paired cutover of the mutable JSON ledgers to SQLite.

This module is deliberately the only bridge between the legacy JSON authorities
and the SQLite authorities.  It does not provide a general import/export route,
dual-write support, or automatic database initialization.  A cutover is either
validated without installation (``--dry-run``), or installs a matched pair and
removes the legacy sources only after post-install validation (``--apply``).
"""

import argparse
from dataclasses import dataclass
from copy import deepcopy
import json
import os
from pathlib import Path
import sys
from typing import Any, Callable, Mapping, Sequence
import uuid

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from _recoil.commands.workspace_issues import validate_issue_document
from _recoil.lib.live_progress import validate_tracker_v5
from _recoil.lib.progress import ProgressDocument
from _recoil.lib.tooling import configure_stdio


REPORT_VERSION = 1
CUTOVER_PAIR_PREFIX = "recoil-ledger-cutover-v1:"


class LedgerMigrationError(RuntimeError):
    """Base class for a governed cutover failure."""


class LedgerMigrationContractError(LedgerMigrationError):
    """The requested paths, mode, or guarded revisions are invalid."""


class LedgerMigrationValidationError(LedgerMigrationError):
    """A source document or database failed semantic validation."""


@dataclass(frozen=True)
class MigrationRequest:
    progress_json: Path
    issues_json: Path
    progress_db: Path
    issues_db: Path
    expected_progress_revision: int
    expected_issues_revision: int
    apply: bool


@dataclass(frozen=True)
class _Backends:
    create_progress: Callable[..., Any]
    open_progress: Callable[[Path], Any]
    create_issues: Callable[..., Any]
    export_issues: Callable[[Path], dict[str, Any]]
    metadata_issues: Callable[[Path], Any]
    validate_issues: Callable[..., list[str]]


def _load_backends() -> _Backends:
    # Lazy imports keep parser/help usable if one backend is temporarily absent
    # during a coordinated code rollout.  Migration itself always fails closed.
    try:
        from _recoil.lib.progress_sqlite import ProgressSQLiteStore
        from _recoil.lib.issue_sqlite import (
            create_issue_database,
            export_issue_document,
            read_issue_metadata,
            validate_issue_database,
        )
    except ImportError as exc:
        raise LedgerMigrationContractError(
            f"SQLite ledger backends are unavailable: {exc}"
        ) from exc
    return _Backends(
        create_progress=ProgressSQLiteStore.create_from_mapping,
        open_progress=lambda path: ProgressSQLiteStore(path, read_only=True),
        create_issues=create_issue_database,
        export_issues=export_issue_document,
        metadata_issues=read_issue_metadata,
        validate_issues=validate_issue_database,
    )


def _canonical(path: Path) -> Path:
    return path.expanduser().resolve(strict=False)


def _temporary_path(destination: Path) -> Path:
    return Path(f"{destination}.migration-tmp")


def _validate_request(request: MigrationRequest) -> MigrationRequest:
    paths = [
        _canonical(request.progress_json),
        _canonical(request.issues_json),
        _canonical(request.progress_db),
        _canonical(request.issues_db),
    ]
    if len(set(paths)) != len(paths):
        raise LedgerMigrationContractError(
            "progress/issues source and destination paths must all be distinct"
        )
    if paths[0].suffix.casefold() != ".json" or paths[1].suffix.casefold() != ".json":
        raise LedgerMigrationContractError("legacy ledger sources must be .json files")
    if paths[2].suffix.casefold() != ".sqlite3" or paths[3].suffix.casefold() != ".sqlite3":
        raise LedgerMigrationContractError("ledger destinations must be .sqlite3 files")
    for label, value in (
        ("expected progress revision", request.expected_progress_revision),
        ("expected issues revision", request.expected_issues_revision),
    ):
        if isinstance(value, bool) or not isinstance(value, int) or value < 0:
            raise LedgerMigrationContractError(f"{label} must be a non-negative integer")
    for destination in paths[2:]:
        if not destination.parent.is_dir():
            raise LedgerMigrationContractError(
                f"destination parent does not exist: {destination.parent}"
            )
    return MigrationRequest(
        paths[0], paths[1], paths[2], paths[3],
        request.expected_progress_revision, request.expected_issues_revision,
        request.apply,
    )


def _read_json(path: Path, *, label: str) -> dict[str, Any]:
    try:
        with path.open("r", encoding="utf-8") as handle:
            value = json.load(handle)
    except FileNotFoundError as exc:
        raise LedgerMigrationContractError(f"missing {label}: {path}") from exc
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise LedgerMigrationValidationError(f"invalid {label} {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise LedgerMigrationValidationError(f"{label} must contain a JSON object: {path}")
    return value


def _revision(value: Mapping[str, Any], *, label: str) -> int:
    revision = value.get("revision")
    if isinstance(revision, bool) or not isinstance(revision, int) or revision < 0:
        raise LedgerMigrationValidationError(
            f"{label} revision must be a non-negative integer"
        )
    return revision


def _active_progress_reservations(value: Mapping[str, Any]) -> list[str]:
    rows = value.get("work_items", {})
    if not isinstance(rows, Mapping):
        return ["<invalid-work-items>"]
    return sorted(
        str(packet_id)
        for packet_id, packet in rows.items()
        if isinstance(packet, Mapping)
        and isinstance(packet.get("reservation"), Mapping)
        and packet["reservation"].get("state") == "active"
    )


def _active_issue_reservations(value: Mapping[str, Any]) -> list[str]:
    rows = value.get("reservations", [])
    if not isinstance(rows, list):
        return ["<invalid-reservations>"]
    return sorted(
        str(row.get("id") or row.get("packet_id") or "<unnamed>")
        for row in rows
        if isinstance(row, Mapping) and row.get("state") == "active"
    )


def _historical_progress_reservation_activity(
    value: Mapping[str, Any],
) -> list[str]:
    """Classify raw pre-cutover progress reservations before schema audit."""
    rows = value.get("work_items")
    if not isinstance(rows, Mapping):
        raise LedgerMigrationValidationError(
            "legacy progress activity validation failed: work_items must be a mapping"
        )
    blocking: list[str] = []
    for packet_id, packet in rows.items():
        if not isinstance(packet_id, str) or not packet_id:
            raise LedgerMigrationValidationError(
                "legacy progress activity validation failed: "
                "work item identity must be a non-empty string"
            )
        if not isinstance(packet, Mapping):
            raise LedgerMigrationValidationError(
                "legacy progress activity validation failed: "
                f"work item {packet_id!r} must be an object"
            )
        reservation = packet.get("reservation")
        if reservation is None:
            continue
        if not isinstance(reservation, Mapping):
            raise LedgerMigrationValidationError(
                "legacy progress activity validation failed: "
                f"work item {packet_id!r} reservation must be an object"
            )
        state = reservation.get("state")
        if not isinstance(state, str) or not state:
            raise LedgerMigrationValidationError(
                "legacy progress activity validation failed: "
                f"work item {packet_id!r} reservation state must be a "
                "non-empty string"
            )
        if state != "released":
            blocking.append(packet_id)
    return sorted(blocking)


def _historical_issue_activity(
    value: Mapping[str, Any],
) -> tuple[list[str], list[str]]:
    """Classify raw pre-cutover issue work and reservations fail-closed."""
    packets = value.get("work_packets")
    if not isinstance(packets, list):
        raise LedgerMigrationValidationError(
            "legacy issue activity validation failed: work_packets must be a list"
        )
    reservations = value.get("reservations")
    if not isinstance(reservations, list):
        raise LedgerMigrationValidationError(
            "legacy issue activity validation failed: reservations must be a list"
        )

    blocking_packets: list[str] = []
    for index, packet in enumerate(packets):
        if not isinstance(packet, Mapping):
            raise LedgerMigrationValidationError(
                "legacy issue activity validation failed: "
                f"work_packets[{index}] must be an object"
            )
        packet_id = packet.get("id")
        state = packet.get("state")
        if not isinstance(packet_id, str) or not packet_id:
            raise LedgerMigrationValidationError(
                "legacy issue activity validation failed: "
                f"work_packets[{index}].id must be a non-empty string"
            )
        if not isinstance(state, str) or not state:
            raise LedgerMigrationValidationError(
                "legacy issue activity validation failed: "
                f"work packet {packet_id!r} state must be a non-empty string"
            )
        if state != "closed":
            blocking_packets.append(packet_id)

    blocking_reservations: list[str] = []
    for index, reservation in enumerate(reservations):
        if not isinstance(reservation, Mapping):
            raise LedgerMigrationValidationError(
                "legacy issue activity validation failed: "
                f"reservations[{index}] must be an object"
            )
        reservation_id = reservation.get("id")
        state = reservation.get("state")
        if not isinstance(reservation_id, str) or not reservation_id:
            raise LedgerMigrationValidationError(
                "legacy issue activity validation failed: "
                f"reservations[{index}].id must be a non-empty string"
            )
        if not isinstance(state, str) or not state:
            raise LedgerMigrationValidationError(
                "legacy issue activity validation failed: "
                f"reservation {reservation_id!r} state must be a non-empty string"
            )
        if state != "released":
            blocking_reservations.append(reservation_id)

    return sorted(blocking_packets), sorted(blocking_reservations)


def _require_no_historical_activity(
    progress: Mapping[str, Any], issues: Mapping[str, Any],
) -> None:
    progress_reservations = _historical_progress_reservation_activity(progress)
    issue_work, issue_reservations = _historical_issue_activity(issues)
    if progress_reservations or issue_work or issue_reservations:
        raise LedgerMigrationContractError(
            "cutover requires no active reservations or nonterminal issue work; "
            f"progress={progress_reservations}, issue_work={issue_work}, "
            f"issues={issue_reservations}"
        )


def _validate_sources(
    progress: Mapping[str, Any],
    issues: Mapping[str, Any],
    *,
    expected_progress_revision: int,
    expected_issues_revision: int,
) -> dict[str, Any]:
    progress_revision = _revision(progress, label="progress tracker")
    issues_revision = _revision(issues, label="workspace issue ledger")
    if progress_revision != expected_progress_revision:
        raise LedgerMigrationContractError(
            "progress revision changed: expected "
            f"{expected_progress_revision}, found {progress_revision}"
        )
    if issues_revision != expected_issues_revision:
        raise LedgerMigrationContractError(
            "workspace issue revision changed: expected "
            f"{expected_issues_revision}, found {issues_revision}"
        )
    _require_no_historical_activity(progress, issues)
    try:
        validate_tracker_v5(progress)
        progress_findings = ProgressDocument._from_owned_data(
            deepcopy(dict(progress)), path=Path("<legacy-progress-migration>")
        ).audit()
        validate_issue_document(issues)
    except Exception as exc:
        raise LedgerMigrationValidationError(f"legacy ledger audit failed: {exc}") from exc
    progress_errors = [row for row in progress_findings if row.severity == "error"]
    if progress_errors:
        details = "; ".join(f"{row.code}: {row.message}" for row in progress_errors[:12])
        raise LedgerMigrationValidationError(f"legacy progress audit failed: {details}")
    progress_active = _active_progress_reservations(progress)
    issue_active = _active_issue_reservations(issues)
    if progress_active or issue_active:
        raise LedgerMigrationContractError(
            "cutover requires no active reservations; progress="
            f"{progress_active}, issues={issue_active}"
        )
    return {
        "progress_revision": progress_revision,
        "issues_revision": issues_revision,
        "progress_audit_error_count": 0,
        "issue_audit_error_count": 0,
        "active_progress_reservations": progress_active,
        "active_issue_reservations": issue_active,
    }


def _metadata_value(metadata: Any, field: str) -> Any:
    if isinstance(metadata, Mapping):
        return metadata.get(field)
    return getattr(metadata, field, None)


def _progress_snapshot(backends: _Backends, path: Path) -> tuple[dict[str, Any], Any, Any]:
    try:
        store = backends.open_progress(path)
        document = store.materialize()
        metadata = store.metadata()
        validation = store.validate_integrity()
    except Exception as exc:
        raise LedgerMigrationValidationError(
            f"invalid progress SQLite database {path}: {exc}"
        ) from exc
    if not isinstance(document, dict):
        raise LedgerMigrationValidationError("progress SQLite materialization is not an object")
    ok = getattr(validation, "ok", None)
    if ok is not True:
        raise LedgerMigrationValidationError(
            f"progress SQLite integrity/foreign-key validation failed: {validation}"
        )
    return document, metadata, validation


def _issue_snapshot(backends: _Backends, path: Path) -> tuple[dict[str, Any], Any, list[str]]:
    try:
        findings = backends.validate_issues(path, document_validator=validate_issue_document)
        metadata = backends.metadata_issues(path)
        document = backends.export_issues(path)
    except Exception as exc:
        raise LedgerMigrationValidationError(
            f"invalid workspace-issue SQLite database {path}: {exc}"
        ) from exc
    if findings:
        raise LedgerMigrationValidationError(
            "workspace-issue SQLite integrity/foreign-key validation failed: "
            + "; ".join(str(item) for item in findings[:12])
        )
    if not isinstance(document, dict):
        raise LedgerMigrationValidationError("issue SQLite materialization is not an object")
    return document, metadata, findings


def _check_snapshot(
    *,
    label: str,
    document: Mapping[str, Any],
    metadata: Any,
    expected_revision: int,
    expected_pair_id: str | None,
    source: Mapping[str, Any] | None,
) -> str:
    revision = _metadata_value(metadata, "revision")
    pair_id = _metadata_value(metadata, "cutover_pair_id")
    if revision != expected_revision:
        raise LedgerMigrationValidationError(
            f"{label} database revision mismatch: expected {expected_revision}, found {revision}"
        )
    if not isinstance(pair_id, str) or not pair_id:
        raise LedgerMigrationValidationError(f"{label} database has no cutover pair id")
    if expected_pair_id is not None and pair_id != expected_pair_id:
        raise LedgerMigrationValidationError(
            f"{label} database cutover pair mismatch: expected {expected_pair_id!r}, "
            f"found {pair_id!r}"
        )
    if source is not None and dict(document) != dict(source):
        raise LedgerMigrationValidationError(
            f"{label} SQLite materialization differs from the guarded legacy document"
        )
    return pair_id


def _remove_migration_temp(path: Path) -> None:
    # Only exact, caller-derived sibling paths are removed.  No globs or broad
    # directories participate in cleanup.
    for target in (path, Path(f"{path}-journal")):
        try:
            target.unlink()
        except FileNotFoundError:
            pass


def _build_progress_temp(
    backends: _Backends, path: Path, source: Mapping[str, Any], pair_id: str
) -> None:
    _remove_migration_temp(path)
    try:
        backends.create_progress(
            path, deepcopy(dict(source)), cutover_pair_id=pair_id, overwrite=False
        )
    except Exception as exc:
        _remove_migration_temp(path)
        raise LedgerMigrationValidationError(
            f"could not import progress ledger into temporary SQLite database: {exc}"
        ) from exc


def _build_issue_temp(
    backends: _Backends, path: Path, source: Mapping[str, Any], pair_id: str
) -> None:
    _remove_migration_temp(path)
    try:
        backends.create_issues(
            path, deepcopy(dict(source)), cutover_pair_id=pair_id, overwrite=False
        )
    except Exception as exc:
        _remove_migration_temp(path)
        raise LedgerMigrationValidationError(
            f"could not import workspace-issue ledger into temporary SQLite database: {exc}"
        ) from exc


def _parity_validation(
    source_progress: Mapping[str, Any],
    migrated_progress: Mapping[str, Any],
    source_issues: Mapping[str, Any],
    migrated_issues: Mapping[str, Any],
) -> dict[str, Any]:
    if dict(source_progress) != dict(migrated_progress):
        raise LedgerMigrationValidationError("progress semantic parity failed")
    if dict(source_issues) != dict(migrated_issues):
        raise LedgerMigrationValidationError("workspace-issue semantic parity failed")

    # Scheduler results are derived independently on both materializations.  This
    # is intentionally a migration-time cost; normal SQLite reads do not use this
    # compatibility path.
    try:
        source_document = ProgressDocument._from_owned_data(
            deepcopy(dict(source_progress)), path=Path("<legacy-progress-parity>")
        )
        migrated_document = ProgressDocument._from_owned_data(
            deepcopy(dict(migrated_progress)), path=Path("<sqlite-progress-parity>")
        )
        source_next = source_document.next_work("recoil")
        migrated_next = migrated_document.next_work("recoil")
    except Exception as exc:
        raise LedgerMigrationValidationError(
            f"progress scheduler parity could not be evaluated: {exc}"
        ) from exc
    if source_next != migrated_next:
        raise LedgerMigrationValidationError("progress scheduler parity failed")

    # An active lease is forbidden at cutover, but compare the reservation view
    # explicitly so malformed or unexpectedly active rows cannot be hidden.
    source_lease_view = {
        "progress": _active_progress_reservations(source_progress),
        "issues": _active_issue_reservations(source_issues),
    }
    migrated_lease_view = {
        "progress": _active_progress_reservations(migrated_progress),
        "issues": _active_issue_reservations(migrated_issues),
    }
    if source_lease_view != migrated_lease_view:
        raise LedgerMigrationValidationError("combined lease parity failed")
    return {
        "semantic_document_equal": True,
        "scheduler_equal": True,
        "combined_lease_equal": True,
    }


def _recheck_sources(
    request: MigrationRequest,
    original_progress: Mapping[str, Any],
    original_issues: Mapping[str, Any],
) -> None:
    progress = _read_json(request.progress_json, label="progress JSON")
    issues = _read_json(request.issues_json, label="workspace issue JSON")
    _validate_sources(
        progress,
        issues,
        expected_progress_revision=request.expected_progress_revision,
        expected_issues_revision=request.expected_issues_revision,
    )
    if progress != dict(original_progress) or issues != dict(original_issues):
        raise LedgerMigrationContractError(
            "legacy ledger contents changed during migration without a revision change"
        )


def migrate_ledgers_sqlite(
    request: MigrationRequest,
    *,
    backends: _Backends | None = None,
    _after_first_install: Callable[[], None] | None = None,
) -> dict[str, Any]:
    """Validate or apply one guarded paired JSON-to-SQLite cutover.

    ``_after_first_install`` is an internal failure-injection seam used only by
    interruption tests.  Production callers leave it unset.
    """

    request = _validate_request(request)
    backends = backends or _load_backends()
    progress_exists = request.progress_db.exists()
    issues_exists = request.issues_db.exists()
    resumed = progress_exists or issues_exists

    progress_source: dict[str, Any] | None = None
    issues_source: dict[str, Any] | None = None
    if request.progress_json.exists():
        progress_source = _read_json(request.progress_json, label="progress JSON")
    if request.issues_json.exists():
        issues_source = _read_json(request.issues_json, label="workspace issue JSON")

    if progress_source is not None and issues_source is not None:
        source_validation = _validate_sources(
            progress_source,
            issues_source,
            expected_progress_revision=request.expected_progress_revision,
            expected_issues_revision=request.expected_issues_revision,
        )
    elif not (progress_exists and issues_exists):
        raise LedgerMigrationContractError(
            "incomplete cutover can be resumed only while both guarded JSON sources remain"
        )
    else:
        source_validation = {
            "progress_revision": request.expected_progress_revision,
            "issues_revision": request.expected_issues_revision,
            "progress_audit_error_count": 0,
            "issue_audit_error_count": 0,
            "active_progress_reservations": [],
            "active_issue_reservations": [],
        }

    existing_progress: tuple[dict[str, Any], Any, Any] | None = None
    existing_issues: tuple[dict[str, Any], Any, list[str]] | None = None
    pair_id: str | None = None
    if progress_exists:
        existing_progress = _progress_snapshot(backends, request.progress_db)
        pair_id = _check_snapshot(
            label="progress", document=existing_progress[0], metadata=existing_progress[1],
            expected_revision=request.expected_progress_revision,
            expected_pair_id=None, source=progress_source,
        )
    if issues_exists:
        existing_issues = _issue_snapshot(backends, request.issues_db)
        issue_pair = _check_snapshot(
            label="workspace issue", document=existing_issues[0], metadata=existing_issues[1],
            expected_revision=request.expected_issues_revision,
            expected_pair_id=pair_id, source=issues_source,
        )
        pair_id = pair_id or issue_pair
    pair_id = pair_id or f"{CUTOVER_PAIR_PREFIX}{uuid.uuid4()}"

    progress_temp = _temporary_path(request.progress_db)
    issues_temp = _temporary_path(request.issues_db)
    try:
        if existing_progress is None:
            assert progress_source is not None
            _build_progress_temp(backends, progress_temp, progress_source, pair_id)
            candidate_progress = _progress_snapshot(backends, progress_temp)
            _check_snapshot(
                label="progress", document=candidate_progress[0], metadata=candidate_progress[1],
                expected_revision=request.expected_progress_revision,
                expected_pair_id=pair_id, source=progress_source,
            )
        else:
            candidate_progress = existing_progress

        if existing_issues is None:
            assert issues_source is not None
            _build_issue_temp(backends, issues_temp, issues_source, pair_id)
            candidate_issues = _issue_snapshot(backends, issues_temp)
            _check_snapshot(
                label="workspace issue", document=candidate_issues[0], metadata=candidate_issues[1],
                expected_revision=request.expected_issues_revision,
                expected_pair_id=pair_id, source=issues_source,
            )
        else:
            candidate_issues = existing_issues

        candidate_validation = _validate_sources(
            candidate_progress[0],
            candidate_issues[0],
            expected_progress_revision=request.expected_progress_revision,
            expected_issues_revision=request.expected_issues_revision,
        )
        if progress_source is None or issues_source is None:
            source_validation = candidate_validation

        if progress_source is not None and issues_source is not None:
            parity = _parity_validation(
                progress_source, candidate_progress[0], issues_source, candidate_issues[0]
            )
        else:
            # Both installed stores have already been validated and the missing
            # JSON source(s) indicate cleanup was interrupted after installation.
            parity = {
                "semantic_document_equal": True,
                "scheduler_equal": True,
                "combined_lease_equal": True,
            }

        result: dict[str, Any] = {
            "report_version": REPORT_VERSION,
            "kind": "paired-ledger-sqlite-cutover",
            "mode": "apply" if request.apply else "dry-run",
            "applied": False,
            "resumed": resumed,
            "cutover_pair_id": pair_id,
            "expected_progress_revision": request.expected_progress_revision,
            "expected_issues_revision": request.expected_issues_revision,
            "progress_json": str(request.progress_json),
            "issues_json": str(request.issues_json),
            "progress_db": str(request.progress_db),
            "issues_db": str(request.issues_db),
            "source_validation": source_validation,
            "parity": parity,
            "integrity_check": {"progress": "ok", "issues": "ok"},
            "foreign_key_check": {"progress": "ok", "issues": "ok"},
            "installed": {"progress": progress_exists, "issues": issues_exists},
            "legacy_json_removed": {"progress": False, "issues": False},
            "legacy_json_removal_irreversible": bool(request.apply),
        }
        if not request.apply:
            return result

        if progress_source is not None and issues_source is not None:
            _recheck_sources(request, progress_source, issues_source)

        if existing_progress is None:
            if request.progress_db.exists():
                raise LedgerMigrationContractError(
                    f"progress destination appeared during migration: {request.progress_db}"
                )
            os.replace(progress_temp, request.progress_db)
            result["installed"]["progress"] = True
            if _after_first_install is not None:
                _after_first_install()
        if existing_issues is None:
            if request.issues_db.exists():
                raise LedgerMigrationContractError(
                    f"workspace-issue destination appeared during migration: {request.issues_db}"
                )
            os.replace(issues_temp, request.issues_db)
            result["installed"]["issues"] = True

        # Validate only the installed final paths before either JSON source is
        # removed.  This covers interruption between the two replacements.
        installed_progress = _progress_snapshot(backends, request.progress_db)
        installed_issues = _issue_snapshot(backends, request.issues_db)
        _check_snapshot(
            label="progress", document=installed_progress[0], metadata=installed_progress[1],
            expected_revision=request.expected_progress_revision,
            expected_pair_id=pair_id, source=progress_source,
        )
        _check_snapshot(
            label="workspace issue", document=installed_issues[0], metadata=installed_issues[1],
            expected_revision=request.expected_issues_revision,
            expected_pair_id=pair_id, source=issues_source,
        )
        _validate_sources(
            installed_progress[0],
            installed_issues[0],
            expected_progress_revision=request.expected_progress_revision,
            expected_issues_revision=request.expected_issues_revision,
        )
        if progress_source is not None and issues_source is not None:
            _parity_validation(
                progress_source, installed_progress[0], issues_source, installed_issues[0]
            )

        # Removal is intentionally last.  A crash between these two exact-file
        # unlinks is resumable because the complete matched database pair exists.
        if request.progress_json.exists():
            request.progress_json.unlink()
            result["legacy_json_removed"]["progress"] = True
        if request.issues_json.exists():
            request.issues_json.unlink()
            result["legacy_json_removed"]["issues"] = True
        result["applied"] = True
        return result
    finally:
        _remove_migration_temp(progress_temp)
        _remove_migration_temp(issues_temp)


def add_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--progress-json", type=Path, required=True)
    parser.add_argument("--issues-json", type=Path, required=True)
    parser.add_argument("--progress-db", type=Path, required=True)
    parser.add_argument("--issues-db", type=Path, required=True)
    parser.add_argument("--expected-progress-revision", type=int, required=True)
    parser.add_argument("--expected-issues-revision", type=int, required=True)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--dry-run", action="store_true")
    mode.add_argument("--apply", action="store_true")
    parser.add_argument("--json", action="store_true", help="Emit the structured result as JSON.")


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Parent-only paired JSON-to-SQLite ledger cutover."
    )
    add_arguments(parser)
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    configure_stdio()
    args = _parser().parse_args(argv)
    request = MigrationRequest(
        args.progress_json,
        args.issues_json,
        args.progress_db,
        args.issues_db,
        args.expected_progress_revision,
        args.expected_issues_revision,
        args.apply,
    )
    try:
        result = migrate_ledgers_sqlite(request)
    except LedgerMigrationContractError as exc:
        payload = {"ok": False, "error_class": "contract", "error": str(exc)}
        if args.json:
            print(json.dumps(payload, indent=2, sort_keys=True))
        else:
            print(f"ERROR: {exc}", file=sys.stderr)
        return 2
    except LedgerMigrationValidationError as exc:
        payload = {"ok": False, "error_class": "validation", "error": str(exc)}
        if args.json:
            print(json.dumps(payload, indent=2, sort_keys=True))
        else:
            print(f"ERROR: {exc}", file=sys.stderr)
        return 1
    if args.json:
        print(json.dumps(result, indent=2, sort_keys=True))
    else:
        action = "installed" if result["applied"] else "validated"
        print(
            f"PASS: paired SQLite ledgers {action} at progress revision "
            f"{result['expected_progress_revision']} and issue revision "
            f"{result['expected_issues_revision']}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
