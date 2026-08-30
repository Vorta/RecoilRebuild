from __future__ import annotations

from contextlib import closing
from copy import deepcopy
from dataclasses import dataclass
import json
import os
from pathlib import Path
import sqlite3
from typing import Any, Callable, Mapping
from uuid import uuid4

from _recoil.lib.live_progress import (
    ISSUE_LEDGER_VERSION,
    ConcurrentRevisionUpdate,
    LiveProgressError,
    RevisionCommitResult,
    validate_issue_ledger_v2,
)


ISSUE_APPLICATION_ID = 0x52434953  # "RCIS"
ISSUE_DATABASE_SCHEMA_VERSION = 2
ISSUE_APPLICATION_NAME = "recoil-workspace-issues"
BUSY_TIMEOUT_MS = 10_000


@dataclass(frozen=True)
class IssueDatabaseMetadata:
    path: Path
    application_id: int
    user_version: int
    schema_version: int
    ledger_version: int
    revision: int
    cutover_pair_id: str


_SCHEMA = """
CREATE TABLE metadata (
    singleton INTEGER PRIMARY KEY CHECK (singleton = 1),
    application_name TEXT NOT NULL,
    schema_version INTEGER NOT NULL,
    ledger_version INTEGER NOT NULL,
    revision INTEGER NOT NULL CHECK (revision >= 0),
    cutover_pair_id TEXT NOT NULL CHECK (length(cutover_pair_id) > 0)
) STRICT;
CREATE TABLE issues (
    issue_order INTEGER PRIMARY KEY,
    issue_id TEXT NOT NULL UNIQUE,
    status TEXT NOT NULL,
    kind TEXT NOT NULL,
    severity TEXT NOT NULL,
    created TEXT NOT NULL,
    updated TEXT NOT NULL,
    payload TEXT NOT NULL CHECK (json_valid(payload))
) STRICT;
CREATE INDEX issues_status_kind_idx ON issues(status, kind, issue_order);
CREATE TABLE id_sequences (
    sequence_namespace TEXT NOT NULL,
    sequence_key TEXT NOT NULL,
    high_water INTEGER NOT NULL CHECK (high_water >= 0),
    PRIMARY KEY (sequence_namespace, sequence_key)
) STRICT;
"""


def _json(value: Any) -> str:
    return json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"))


def _configure(connection: sqlite3.Connection, *, writable: bool) -> None:
    connection.row_factory = sqlite3.Row
    connection.execute(f"PRAGMA busy_timeout={BUSY_TIMEOUT_MS}")
    connection.execute("PRAGMA foreign_keys=ON")
    if writable:
        mode = str(connection.execute("PRAGMA journal_mode=DELETE").fetchone()[0])
        if mode.casefold() != "delete":
            raise LiveProgressError(
                f"workspace issue database refused DELETE journal mode: {mode}"
            )
        connection.execute("PRAGMA synchronous=FULL")
    else:
        connection.execute("PRAGMA query_only=ON")


def open_issue_database(
    path: str | Path, *, writable: bool = False
) -> sqlite3.Connection:
    resolved = Path(path)
    if not resolved.is_file():
        raise LiveProgressError(
            f"{resolved}: workspace issue SQLite database is missing"
        )
    connection: sqlite3.Connection | None = None
    try:
        uri = resolved.resolve().as_uri() + ("?mode=rw" if writable else "?mode=ro")
        connection = sqlite3.connect(
            uri, uri=True, timeout=BUSY_TIMEOUT_MS / 1000, isolation_level=None
        )
        _configure(connection, writable=writable)
        return connection
    except (sqlite3.Error, LiveProgressError) as exc:
        if connection is not None:
            connection.close()
        if isinstance(exc, LiveProgressError):
            raise
        raise LiveProgressError(
            f"{resolved}: cannot open workspace issue database: {exc}"
        ) from exc


def _metadata_from_connection(
    connection: sqlite3.Connection, path: Path
) -> IssueDatabaseMetadata:
    application_id = int(connection.execute("PRAGMA application_id").fetchone()[0])
    user_version = int(connection.execute("PRAGMA user_version").fetchone()[0])
    if application_id != ISSUE_APPLICATION_ID:
        raise LiveProgressError(
            f"{path}: SQLite application_id must be {ISSUE_APPLICATION_ID}, "
            f"found {application_id}"
        )
    if user_version != ISSUE_DATABASE_SCHEMA_VERSION:
        raise LiveProgressError(
            f"{path}: SQLite user_version must be {ISSUE_DATABASE_SCHEMA_VERSION}, "
            f"found {user_version}; run the governed single-agent cutover"
        )
    try:
        row = connection.execute(
            "SELECT application_name, schema_version, ledger_version, revision, "
            "cutover_pair_id FROM metadata WHERE singleton=1"
        ).fetchone()
    except sqlite3.Error as exc:
        raise LiveProgressError(
            f"{path}: unreadable workspace issue metadata: {exc}"
        ) from exc
    if row is None:
        raise LiveProgressError(f"{path}: workspace issue metadata row is missing")
    if (
        row["application_name"] != ISSUE_APPLICATION_NAME
        or int(row["schema_version"]) != ISSUE_DATABASE_SCHEMA_VERSION
        or int(row["ledger_version"]) != ISSUE_LEDGER_VERSION
    ):
        raise LiveProgressError(f"{path}: unsupported workspace issue schema")
    return IssueDatabaseMetadata(
        path=path,
        application_id=application_id,
        user_version=user_version,
        schema_version=int(row["schema_version"]),
        ledger_version=int(row["ledger_version"]),
        revision=int(row["revision"]),
        cutover_pair_id=str(row["cutover_pair_id"]),
    )


def read_issue_metadata(path: str | Path) -> IssueDatabaseMetadata:
    resolved = Path(path)
    with closing(open_issue_database(resolved)) as connection:
        return _metadata_from_connection(connection, resolved)


def _replace_document(
    connection: sqlite3.Connection,
    document: Mapping[str, Any],
    *,
    cutover_pair_id: str,
) -> None:
    connection.execute("DELETE FROM issues")
    connection.execute("DELETE FROM id_sequences")
    connection.execute(
        "INSERT OR REPLACE INTO metadata "
        "(singleton, application_name, schema_version, ledger_version, revision, "
        "cutover_pair_id) VALUES (1, ?, ?, ?, ?, ?)",
        (
            ISSUE_APPLICATION_NAME,
            ISSUE_DATABASE_SCHEMA_VERSION,
            ISSUE_LEDGER_VERSION,
            int(document["revision"]),
            cutover_pair_id,
        ),
    )
    for issue_order, issue in enumerate(document.get("issues", [])):
        connection.execute(
            "INSERT INTO issues(issue_order, issue_id, status, kind, severity, "
            "created, updated, payload) VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
            (
                issue_order,
                str(issue["id"]),
                str(issue["status"]),
                str(issue["kind"]),
                str(issue["severity"]),
                str(issue["created"]),
                str(issue["updated"]),
                _json(issue),
            ),
        )
    for namespace, values in document.get("id_sequences", {}).items():
        if not isinstance(values, Mapping):
            raise LiveProgressError(f"id_sequences.{namespace} must be an object")
        for key, high_water in values.items():
            connection.execute(
                "INSERT INTO id_sequences(sequence_namespace, sequence_key, high_water) "
                "VALUES (?, ?, ?)",
                (str(namespace), str(key), int(high_water)),
            )


def create_issue_database(
    path: str | Path,
    document: Mapping[str, Any],
    *,
    cutover_pair_id: str,
    overwrite: bool = False,
) -> None:
    resolved = Path(path)
    if not isinstance(cutover_pair_id, str) or not cutover_pair_id.strip():
        raise LiveProgressError("cutover_pair_id must be a non-empty string")
    validate_issue_ledger_v2(document)
    if resolved.exists() and not overwrite:
        raise LiveProgressError(
            f"{resolved}: refusing to overwrite an existing issue database"
        )
    resolved.parent.mkdir(parents=True, exist_ok=True)
    temporary = resolved.with_name(f".{resolved.name}.{uuid4().hex}.tmp")
    connection: sqlite3.Connection | None = None
    try:
        connection = sqlite3.connect(temporary, timeout=10.0, isolation_level=None)
        _configure(connection, writable=True)
        connection.execute(f"PRAGMA application_id={ISSUE_APPLICATION_ID}")
        connection.execute(f"PRAGMA user_version={ISSUE_DATABASE_SCHEMA_VERSION}")
        connection.executescript(_SCHEMA)
        connection.execute("BEGIN IMMEDIATE")
        _replace_document(connection, document, cutover_pair_id=cutover_pair_id)
        connection.execute("COMMIT")
        connection.close()
        connection = None
        os.replace(temporary, resolved)
    except Exception:
        if connection is not None:
            if connection.in_transaction:
                connection.execute("ROLLBACK")
            connection.close()
        if temporary.exists():
            temporary.unlink()
        raise


def import_issue_document(
    path: str | Path,
    document: Mapping[str, Any],
    *,
    cutover_pair_id: str,
    overwrite: bool = False,
) -> None:
    create_issue_database(
        path, document, cutover_pair_id=cutover_pair_id, overwrite=overwrite
    )


def _export_from_connection(
    connection: sqlite3.Connection, path: Path
) -> dict[str, Any]:
    metadata = _metadata_from_connection(connection, path)
    document: dict[str, Any] = {
        "version": metadata.ledger_version,
        "revision": metadata.revision,
        "id_sequences": {},
        "issues": [],
    }
    for row in connection.execute("SELECT payload FROM issues ORDER BY issue_order"):
        issue = json.loads(str(row["payload"]))
        if not isinstance(issue, dict):
            raise LiveProgressError(f"{path}: issue payload must be an object")
        document["issues"].append(issue)
    for row in connection.execute(
        "SELECT sequence_namespace, sequence_key, high_water FROM id_sequences "
        "ORDER BY sequence_namespace, sequence_key"
    ):
        document["id_sequences"].setdefault(str(row["sequence_namespace"]), {})[
            str(row["sequence_key"])
        ] = int(row["high_water"])
    return document


def export_issue_document(path: str | Path) -> dict[str, Any]:
    resolved = Path(path)
    with closing(open_issue_database(resolved)) as connection:
        return _export_from_connection(connection, resolved)


def validate_issue_database(
    path: str | Path,
    *,
    document_validator: Callable[[Mapping[str, Any]], None] | None = None,
) -> list[str]:
    resolved = Path(path)
    findings: list[str] = []
    try:
        with closing(open_issue_database(resolved)) as connection:
            metadata = _metadata_from_connection(connection, resolved)
            if metadata.revision < 0:
                findings.append("metadata.revision: expected non-negative integer")
            if tuple(str(row[0]) for row in connection.execute("PRAGMA integrity_check")) != (
                "ok",
            ):
                findings.append("integrity_check did not return ok")
            findings.extend(
                "foreign_key_check: " + ", ".join(str(value) for value in row)
                for row in connection.execute("PRAGMA foreign_key_check")
            )
            document = _export_from_connection(connection, resolved)
            validate_issue_ledger_v2(document)
            if document_validator is not None:
                document_validator(document)
    except Exception as exc:
        findings.append(str(exc))
    return findings


class IssueSQLiteStore:
    """Small revision-CAS store for issue reports; it has no work allocator."""

    def __init__(
        self,
        path: str | Path,
        *,
        validator: Callable[[Mapping[str, Any]], None] | None = None,
    ) -> None:
        self.path = Path(path)
        self.validator = validator

    def load(self) -> dict[str, Any]:
        document = export_issue_document(self.path)
        if self.validator is not None:
            self.validator(document)
        return document

    def commit(
        self,
        proposed: Mapping[str, Any],
        *,
        expected_revision: int,
        apply: bool,
    ) -> RevisionCommitResult:
        candidate = deepcopy(dict(proposed))
        candidate["version"] = ISSUE_LEDGER_VERSION
        candidate["revision"] = expected_revision + 1
        validate_issue_ledger_v2(candidate)
        if self.validator is not None:
            self.validator(candidate)
        with closing(open_issue_database(self.path, writable=True)) as connection:
            try:
                connection.execute("BEGIN IMMEDIATE")
                metadata = _metadata_from_connection(connection, self.path)
                if metadata.revision != expected_revision:
                    raise ConcurrentRevisionUpdate(
                        f"revision changed: expected {expected_revision}, "
                        f"found {metadata.revision}"
                    )
                result = RevisionCommitResult(
                    applied=apply,
                    path=self.path,
                    previous_revision=metadata.revision,
                    revision=expected_revision + 1,
                )
                if apply:
                    _replace_document(
                        connection,
                        candidate,
                        cutover_pair_id=metadata.cutover_pair_id,
                    )
                    connection.execute("COMMIT")
                else:
                    connection.execute("ROLLBACK")
                return result
            except Exception:
                if connection.in_transaction:
                    connection.execute("ROLLBACK")
                raise
