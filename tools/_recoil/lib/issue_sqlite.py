from __future__ import annotations

from contextlib import closing
from copy import deepcopy
from dataclasses import dataclass
import json
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
ISSUE_DATABASE_SCHEMA_VERSION = 1
ISSUE_APPLICATION_NAME = "recoil-workspace-issues"


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
);
CREATE TABLE issues (
    issue_order INTEGER PRIMARY KEY,
    issue_id TEXT NOT NULL UNIQUE,
    status TEXT NOT NULL,
    kind TEXT NOT NULL,
    severity TEXT NOT NULL,
    created TEXT NOT NULL,
    updated TEXT NOT NULL,
    history_present INTEGER NOT NULL CHECK (history_present IN (0, 1)),
    scalar_json TEXT NOT NULL
);
CREATE INDEX issues_status_kind_idx ON issues(status, kind, issue_order);
CREATE TABLE issue_history (
    issue_id TEXT NOT NULL REFERENCES issues(issue_id) ON DELETE CASCADE,
    history_order INTEGER NOT NULL,
    event_json TEXT NOT NULL,
    PRIMARY KEY (issue_id, history_order)
);
CREATE TABLE issue_list_fields (
    issue_id TEXT NOT NULL REFERENCES issues(issue_id) ON DELETE CASCADE,
    field_name TEXT NOT NULL,
    PRIMARY KEY (issue_id, field_name)
);
CREATE TABLE issue_list_values (
    issue_id TEXT NOT NULL,
    field_name TEXT NOT NULL,
    value_order INTEGER NOT NULL,
    value_json TEXT NOT NULL,
    PRIMARY KEY (issue_id, field_name, value_order),
    FOREIGN KEY (issue_id, field_name)
        REFERENCES issue_list_fields(issue_id, field_name) ON DELETE CASCADE
);
CREATE TABLE work_packets (
    packet_order INTEGER PRIMARY KEY,
    packet_id TEXT NOT NULL UNIQUE,
    issue_id TEXT NOT NULL REFERENCES issues(issue_id),
    state TEXT NOT NULL,
    handoff_role TEXT NOT NULL,
    resource_claims_present INTEGER NOT NULL CHECK (resource_claims_present IN (0, 1)),
    scalar_json TEXT NOT NULL
);
CREATE INDEX work_packets_issue_state_idx ON work_packets(issue_id, state, packet_order);
CREATE TABLE work_packet_list_fields (
    packet_id TEXT NOT NULL REFERENCES work_packets(packet_id) ON DELETE CASCADE,
    field_name TEXT NOT NULL,
    PRIMARY KEY (packet_id, field_name)
);
CREATE TABLE work_packet_list_values (
    packet_id TEXT NOT NULL,
    field_name TEXT NOT NULL,
    value_order INTEGER NOT NULL,
    value_json TEXT NOT NULL,
    PRIMARY KEY (packet_id, field_name, value_order),
    FOREIGN KEY (packet_id, field_name)
        REFERENCES work_packet_list_fields(packet_id, field_name) ON DELETE CASCADE
);
CREATE TABLE work_packet_resource_claims (
    packet_id TEXT NOT NULL REFERENCES work_packets(packet_id) ON DELETE CASCADE,
    claim_order INTEGER NOT NULL,
    kind TEXT NOT NULL,
    resource_id TEXT NOT NULL,
    access TEXT NOT NULL,
    claim_json TEXT NOT NULL,
    PRIMARY KEY (packet_id, claim_order)
);
CREATE INDEX work_packet_claim_lookup_idx
    ON work_packet_resource_claims(kind, resource_id, access, packet_id);
CREATE TABLE reservations (
    reservation_order INTEGER PRIMARY KEY,
    reservation_id TEXT NOT NULL UNIQUE,
    packet_id TEXT NOT NULL REFERENCES work_packets(packet_id),
    state TEXT NOT NULL,
    evidence_ids_present INTEGER NOT NULL CHECK (evidence_ids_present IN (0, 1)),
    resource_claims_present INTEGER NOT NULL CHECK (resource_claims_present IN (0, 1)),
    scalar_json TEXT NOT NULL
);
CREATE INDEX reservations_packet_state_idx ON reservations(packet_id, state, reservation_order);
CREATE TABLE reservation_evidence (
    reservation_id TEXT NOT NULL REFERENCES reservations(reservation_id) ON DELETE CASCADE,
    evidence_order INTEGER NOT NULL,
    evidence_id TEXT NOT NULL,
    PRIMARY KEY (reservation_id, evidence_order)
);
CREATE TABLE reservation_resource_claims (
    reservation_id TEXT NOT NULL REFERENCES reservations(reservation_id) ON DELETE CASCADE,
    claim_order INTEGER NOT NULL,
    kind TEXT NOT NULL,
    resource_id TEXT NOT NULL,
    access TEXT NOT NULL,
    claim_json TEXT NOT NULL,
    PRIMARY KEY (reservation_id, claim_order)
);
CREATE INDEX reservation_claim_lookup_idx
    ON reservation_resource_claims(kind, resource_id, access, reservation_id);
CREATE TABLE id_sequences (
    sequence_namespace TEXT NOT NULL,
    sequence_key TEXT NOT NULL,
    high_water INTEGER NOT NULL CHECK (high_water >= 0),
    PRIMARY KEY (sequence_namespace, sequence_key)
);
"""


def _json(value: Any) -> str:
    return json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"))


def _decode(value: str) -> Any:
    return json.loads(value)


def _configure(connection: sqlite3.Connection, *, writable: bool) -> None:
    connection.row_factory = sqlite3.Row
    connection.execute("PRAGMA busy_timeout = 10000")
    connection.execute("PRAGMA foreign_keys = ON")
    if writable:
        mode = connection.execute("PRAGMA journal_mode = DELETE").fetchone()[0]
        if str(mode).casefold() != "delete":
            raise LiveProgressError(f"workspace issue database refused DELETE journal mode: {mode}")
        connection.execute("PRAGMA synchronous = FULL")
    else:
        connection.execute("PRAGMA query_only = ON")


def open_issue_database(path: str | Path, *, writable: bool = False) -> sqlite3.Connection:
    """Open an existing issue database without ever initializing one."""
    resolved = Path(path)
    if resolved.suffix.casefold() == ".json":
        raise LiveProgressError(
            f"{resolved}: legacy JSON workspace issue ledgers are not live stores; "
            "run the governed cutover migration and use its .sqlite3 output"
        )
    if not resolved.is_file():
        raise LiveProgressError(
            f"{resolved}: workspace issue SQLite database is missing; run the governed cutover migration first"
        )
    connection: sqlite3.Connection | None = None
    try:
        uri = resolved.resolve().as_uri() + ("?mode=rw" if writable else "?mode=ro")
        connection = sqlite3.connect(uri, uri=True, timeout=10.0, isolation_level=None)
        _configure(connection, writable=writable)
        return connection
    except (sqlite3.Error, LiveProgressError) as exc:
        if connection is not None:
            connection.close()
        if isinstance(exc, LiveProgressError):
            raise
        raise LiveProgressError(f"{resolved}: cannot open workspace issue database: {exc}") from exc


def _metadata_from_connection(connection: sqlite3.Connection, path: Path) -> IssueDatabaseMetadata:
    application_id = int(connection.execute("PRAGMA application_id").fetchone()[0])
    user_version = int(connection.execute("PRAGMA user_version").fetchone()[0])
    if application_id != ISSUE_APPLICATION_ID:
        raise LiveProgressError(
            f"{path}: SQLite application_id must be {ISSUE_APPLICATION_ID}, found {application_id}"
        )
    if user_version != ISSUE_DATABASE_SCHEMA_VERSION:
        raise LiveProgressError(
            f"{path}: SQLite user_version must be {ISSUE_DATABASE_SCHEMA_VERSION}, found {user_version}"
        )
    try:
        row = connection.execute(
            "SELECT application_name, schema_version, ledger_version, revision, cutover_pair_id "
            "FROM metadata WHERE singleton = 1"
        ).fetchone()
    except sqlite3.Error as exc:
        raise LiveProgressError(f"{path}: unreadable workspace issue metadata: {exc}") from exc
    if row is None:
        raise LiveProgressError(f"{path}: workspace issue metadata row is missing")
    if row["application_name"] != ISSUE_APPLICATION_NAME:
        raise LiveProgressError(f"{path}: unexpected workspace issue application name")
    if int(row["schema_version"]) != ISSUE_DATABASE_SCHEMA_VERSION:
        raise LiveProgressError(f"{path}: workspace issue schema version is unsupported")
    if int(row["ledger_version"]) != ISSUE_LEDGER_VERSION:
        raise LiveProgressError(f"{path}: workspace issue ledger version is unsupported")
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


def _split_lists(row: Mapping[str, Any], excluded: set[str]) -> tuple[dict[str, Any], dict[str, list[Any]]]:
    scalar: dict[str, Any] = {}
    lists: dict[str, list[Any]] = {}
    for key, value in row.items():
        if key in excluded:
            continue
        if isinstance(value, list):
            lists[key] = deepcopy(value)
        else:
            scalar[key] = deepcopy(value)
    return scalar, lists


def _replace_document(
    connection: sqlite3.Connection,
    document: Mapping[str, Any],
    *,
    cutover_pair_id: str,
) -> None:
    for table in (
        "reservation_resource_claims", "reservation_evidence", "reservations",
        "work_packet_resource_claims", "work_packet_list_values", "work_packet_list_fields",
        "work_packets", "issue_list_values", "issue_list_fields", "issue_history", "issues",
        "id_sequences",
    ):
        connection.execute(f"DELETE FROM {table}")

    connection.execute(
        "INSERT OR REPLACE INTO metadata "
        "(singleton, application_name, schema_version, ledger_version, revision, cutover_pair_id) "
        "VALUES (1, ?, ?, ?, ?, ?)",
        (
            ISSUE_APPLICATION_NAME,
            ISSUE_DATABASE_SCHEMA_VERSION,
            int(document["version"]),
            int(document["revision"]),
            cutover_pair_id,
        ),
    )
    for issue_order, issue in enumerate(document.get("issues", [])):
        scalar, lists = _split_lists(
            issue, {"history", "id", "status", "kind", "severity", "created", "updated"}
        )
        issue_id = str(issue["id"])
        connection.execute(
            "INSERT INTO issues(issue_order, issue_id, status, kind, severity, created, updated, "
            "history_present, scalar_json) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
            (
                issue_order, issue_id, issue["status"], issue["kind"], issue["severity"],
                issue["created"], issue["updated"], int("history" in issue), _json(scalar),
            ),
        )
        for history_order, event in enumerate(issue.get("history", [])):
            connection.execute(
                "INSERT INTO issue_history(issue_id, history_order, event_json) VALUES (?, ?, ?)",
                (issue_id, history_order, _json(event)),
            )
        for field_name, values in lists.items():
            connection.execute(
                "INSERT INTO issue_list_fields(issue_id, field_name) VALUES (?, ?)",
                (issue_id, field_name),
            )
            connection.executemany(
                "INSERT INTO issue_list_values(issue_id, field_name, value_order, value_json) "
                "VALUES (?, ?, ?, ?)",
                [(issue_id, field_name, order, _json(value)) for order, value in enumerate(values)],
            )

    for packet_order, packet in enumerate(document.get("work_packets", [])):
        scalar, lists = _split_lists(
            packet, {"resource_claims", "id", "issue_id", "state", "handoff_role"}
        )
        packet_id = str(packet["id"])
        connection.execute(
            "INSERT INTO work_packets(packet_order, packet_id, issue_id, state, handoff_role, "
            "resource_claims_present, scalar_json) VALUES (?, ?, ?, ?, ?, ?, ?)",
            (
                packet_order, packet_id, str(packet["issue_id"]), packet["state"],
                packet["handoff_role"], int("resource_claims" in packet), _json(scalar),
            ),
        )
        for field_name, values in lists.items():
            connection.execute(
                "INSERT INTO work_packet_list_fields(packet_id, field_name) VALUES (?, ?)",
                (packet_id, field_name),
            )
            connection.executemany(
                "INSERT INTO work_packet_list_values(packet_id, field_name, value_order, value_json) "
                "VALUES (?, ?, ?, ?)",
                [(packet_id, field_name, order, _json(value)) for order, value in enumerate(values)],
            )
        for claim_order, claim in enumerate(packet.get("resource_claims", [])):
            connection.execute(
                "INSERT INTO work_packet_resource_claims "
                "(packet_id, claim_order, kind, resource_id, access, claim_json) VALUES (?, ?, ?, ?, ?, ?)",
                (
                    packet_id, claim_order, claim["kind"], claim["id"], claim["access"],
                    _json({key: value for key, value in claim.items() if key not in {"kind", "id", "access"}}),
                ),
            )

    for reservation_order, reservation in enumerate(document.get("reservations", [])):
        scalar, _lists = _split_lists(
            reservation, {"resource_claims", "evidence_ids", "id", "packet_id", "state"}
        )
        reservation_id = str(reservation["id"])
        connection.execute(
            "INSERT INTO reservations(reservation_order, reservation_id, packet_id, state, "
            "evidence_ids_present, resource_claims_present, scalar_json) VALUES (?, ?, ?, ?, ?, ?, ?)",
            (
                reservation_order, reservation_id, str(reservation["packet_id"]),
                reservation["state"], int("evidence_ids" in reservation),
                int("resource_claims" in reservation), _json(scalar),
            ),
        )
        connection.executemany(
            "INSERT INTO reservation_evidence(reservation_id, evidence_order, evidence_id) VALUES (?, ?, ?)",
            [
                (reservation_id, order, evidence_id)
                for order, evidence_id in enumerate(reservation.get("evidence_ids", []))
            ],
        )
        for claim_order, claim in enumerate(reservation.get("resource_claims", [])):
            connection.execute(
                "INSERT INTO reservation_resource_claims "
                "(reservation_id, claim_order, kind, resource_id, access, claim_json) VALUES (?, ?, ?, ?, ?, ?)",
                (
                    reservation_id, claim_order, claim["kind"], claim["id"], claim["access"],
                    _json({key: value for key, value in claim.items() if key not in {"kind", "id", "access"}}),
                ),
            )

    sequences = document.get("id_sequences", {})
    for namespace, values in sequences.items():
        if not isinstance(values, Mapping):
            raise LiveProgressError(f"id_sequences.{namespace} must be an object")
        for key, high_water in values.items():
            connection.execute(
                "INSERT INTO id_sequences(sequence_namespace, sequence_key, high_water) VALUES (?, ?, ?)",
                (str(namespace), str(key), int(high_water)),
            )


def _write_issue(connection: sqlite3.Connection, issue: Mapping[str, Any], order: int) -> None:
    scalar, lists = _split_lists(
        issue, {"history", "id", "status", "kind", "severity", "created", "updated"}
    )
    issue_id = str(issue["id"])
    existing = connection.execute(
        "SELECT 1 FROM issues WHERE issue_id = ?", (issue_id,)
    ).fetchone()
    if existing is None:
        connection.execute(
            "INSERT INTO issues(issue_order, issue_id, status, kind, severity, created, updated, "
            "history_present, scalar_json) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
            (
                order, issue_id, issue["status"], issue["kind"], issue["severity"],
                issue["created"], issue["updated"], int("history" in issue), _json(scalar),
            ),
        )
    else:
        connection.execute(
            "UPDATE issues SET issue_order = ?, status = ?, kind = ?, severity = ?, "
            "created = ?, updated = ?, history_present = ?, scalar_json = ? WHERE issue_id = ?",
            (
                order, issue["status"], issue["kind"], issue["severity"], issue["created"],
                issue["updated"], int("history" in issue), _json(scalar), issue_id,
            ),
        )
        connection.execute("DELETE FROM issue_history WHERE issue_id = ?", (issue_id,))
        connection.execute("DELETE FROM issue_list_fields WHERE issue_id = ?", (issue_id,))
    connection.executemany(
        "INSERT INTO issue_history(issue_id, history_order, event_json) VALUES (?, ?, ?)",
        [(issue_id, index, _json(event)) for index, event in enumerate(issue.get("history", []))],
    )
    for field_name, values in lists.items():
        connection.execute(
            "INSERT INTO issue_list_fields(issue_id, field_name) VALUES (?, ?)",
            (issue_id, field_name),
        )
        connection.executemany(
            "INSERT INTO issue_list_values(issue_id, field_name, value_order, value_json) VALUES (?, ?, ?, ?)",
            [(issue_id, field_name, index, _json(value)) for index, value in enumerate(values)],
        )


def _write_packet(connection: sqlite3.Connection, packet: Mapping[str, Any], order: int) -> None:
    scalar, lists = _split_lists(
        packet, {"resource_claims", "id", "issue_id", "state", "handoff_role"}
    )
    packet_id = str(packet["id"])
    existing = connection.execute(
        "SELECT 1 FROM work_packets WHERE packet_id = ?", (packet_id,)
    ).fetchone()
    if existing is None:
        connection.execute(
            "INSERT INTO work_packets(packet_order, packet_id, issue_id, state, handoff_role, "
            "resource_claims_present, scalar_json) VALUES (?, ?, ?, ?, ?, ?, ?)",
            (
                order, packet_id, str(packet["issue_id"]), packet["state"],
                packet["handoff_role"], int("resource_claims" in packet), _json(scalar),
            ),
        )
    else:
        connection.execute(
            "UPDATE work_packets SET packet_order = ?, issue_id = ?, state = ?, "
            "handoff_role = ?, resource_claims_present = ?, scalar_json = ? WHERE packet_id = ?",
            (
                order, str(packet["issue_id"]), packet["state"], packet["handoff_role"],
                int("resource_claims" in packet), _json(scalar), packet_id,
            ),
        )
        connection.execute(
            "DELETE FROM work_packet_list_fields WHERE packet_id = ?", (packet_id,)
        )
        connection.execute(
            "DELETE FROM work_packet_resource_claims WHERE packet_id = ?", (packet_id,)
        )
    for field_name, values in lists.items():
        connection.execute(
            "INSERT INTO work_packet_list_fields(packet_id, field_name) VALUES (?, ?)",
            (packet_id, field_name),
        )
        connection.executemany(
            "INSERT INTO work_packet_list_values(packet_id, field_name, value_order, value_json) VALUES (?, ?, ?, ?)",
            [(packet_id, field_name, index, _json(value)) for index, value in enumerate(values)],
        )
    for index, claim in enumerate(packet.get("resource_claims", [])):
        connection.execute(
            "INSERT INTO work_packet_resource_claims "
            "(packet_id, claim_order, kind, resource_id, access, claim_json) VALUES (?, ?, ?, ?, ?, ?)",
            (
                packet_id, index, claim["kind"], claim["id"], claim["access"],
                _json({key: value for key, value in claim.items() if key not in {"kind", "id", "access"}}),
            ),
        )


def _write_reservation(
    connection: sqlite3.Connection, reservation: Mapping[str, Any], order: int
) -> None:
    scalar, _lists = _split_lists(
        reservation, {"resource_claims", "evidence_ids", "id", "packet_id", "state"}
    )
    reservation_id = str(reservation["id"])
    existing = connection.execute(
        "SELECT 1 FROM reservations WHERE reservation_id = ?", (reservation_id,)
    ).fetchone()
    if existing is None:
        connection.execute(
            "INSERT INTO reservations(reservation_order, reservation_id, packet_id, state, "
            "evidence_ids_present, resource_claims_present, scalar_json) VALUES (?, ?, ?, ?, ?, ?, ?)",
            (
                order, reservation_id, str(reservation["packet_id"]), reservation["state"],
                int("evidence_ids" in reservation), int("resource_claims" in reservation),
                _json(scalar),
            ),
        )
    else:
        connection.execute(
            "UPDATE reservations SET reservation_order = ?, packet_id = ?, state = ?, "
            "evidence_ids_present = ?, resource_claims_present = ?, scalar_json = ? "
            "WHERE reservation_id = ?",
            (
                order, str(reservation["packet_id"]), reservation["state"],
                int("evidence_ids" in reservation), int("resource_claims" in reservation), _json(scalar),
                reservation_id,
            ),
        )
        connection.execute(
            "DELETE FROM reservation_evidence WHERE reservation_id = ?", (reservation_id,)
        )
        connection.execute(
            "DELETE FROM reservation_resource_claims WHERE reservation_id = ?", (reservation_id,)
        )
    connection.executemany(
        "INSERT INTO reservation_evidence(reservation_id, evidence_order, evidence_id) VALUES (?, ?, ?)",
        [
            (reservation_id, index, evidence_id)
            for index, evidence_id in enumerate(reservation.get("evidence_ids", []))
        ],
    )
    for index, claim in enumerate(reservation.get("resource_claims", [])):
        connection.execute(
            "INSERT INTO reservation_resource_claims "
            "(reservation_id, claim_order, kind, resource_id, access, claim_json) VALUES (?, ?, ?, ?, ?, ?)",
            (
                reservation_id, index, claim["kind"], claim["id"], claim["access"],
                _json({key: value for key, value in claim.items() if key not in {"kind", "id", "access"}}),
            ),
        )


def _sync_document(
    connection: sqlite3.Connection,
    current: Mapping[str, Any],
    candidate: Mapping[str, Any],
    *,
    cutover_pair_id: str,
) -> None:
    current_reservations = {row["id"]: row for row in current.get("reservations", [])}
    candidate_reservations = {row["id"]: row for row in candidate.get("reservations", [])}
    for entity_id in current_reservations.keys() - candidate_reservations.keys():
        connection.execute("DELETE FROM reservations WHERE reservation_id = ?", (entity_id,))

    current_packets = {row["id"]: row for row in current.get("work_packets", [])}
    candidate_packets = {row["id"]: row for row in candidate.get("work_packets", [])}
    for entity_id in current_packets.keys() - candidate_packets.keys():
        connection.execute("DELETE FROM work_packets WHERE packet_id = ?", (entity_id,))

    current_issues = {row["id"]: row for row in current.get("issues", [])}
    candidate_issues = {row["id"]: row for row in candidate.get("issues", [])}
    for entity_id in current_issues.keys() - candidate_issues.keys():
        connection.execute("DELETE FROM issues WHERE issue_id = ?", (entity_id,))

    current_issue_order = {
        row["id"]: order for order, row in enumerate(current.get("issues", []))
    }
    current_packet_order = {
        row["id"]: order for order, row in enumerate(current.get("work_packets", []))
    }
    current_reservation_order = {
        row["id"]: order for order, row in enumerate(current.get("reservations", []))
    }
    # Vacate only ordinals that actually move before assigning their final
    # values.  This permits deterministic reorder/compaction without touching
    # unrelated rows or colliding with another row's still-current ordinal.
    for order, issue in enumerate(candidate.get("issues", [])):
        old_order = current_issue_order.get(issue["id"])
        if old_order is not None and old_order != order:
            connection.execute(
                "UPDATE issues SET issue_order = ? WHERE issue_id = ?",
                (-old_order - 1, issue["id"]),
            )
    for order, packet in enumerate(candidate.get("work_packets", [])):
        old_order = current_packet_order.get(packet["id"])
        if old_order is not None and old_order != order:
            connection.execute(
                "UPDATE work_packets SET packet_order = ? WHERE packet_id = ?",
                (-old_order - 1, packet["id"]),
            )
    for order, reservation in enumerate(candidate.get("reservations", [])):
        old_order = current_reservation_order.get(reservation["id"])
        if old_order is not None and old_order != order:
            connection.execute(
                "UPDATE reservations SET reservation_order = ? WHERE reservation_id = ?",
                (-old_order - 1, reservation["id"]),
            )
    for order, issue in enumerate(candidate.get("issues", [])):
        if current_issues.get(issue["id"]) != issue or current_issue_order.get(issue["id"]) != order:
            _write_issue(connection, issue, order)
    for order, packet in enumerate(candidate.get("work_packets", [])):
        if current_packets.get(packet["id"]) != packet or current_packet_order.get(packet["id"]) != order:
            _write_packet(connection, packet, order)
    for order, reservation in enumerate(candidate.get("reservations", [])):
        if (
            current_reservations.get(reservation["id"]) != reservation
            or current_reservation_order.get(reservation["id"]) != order
        ):
            _write_reservation(connection, reservation, order)

    old_sequences = {
        (namespace, key): int(value)
        for namespace, values in current.get("id_sequences", {}).items()
        for key, value in values.items()
    }
    new_sequences = {
        (namespace, key): int(value)
        for namespace, values in candidate.get("id_sequences", {}).items()
        for key, value in values.items()
    }
    for key in old_sequences.keys() - new_sequences.keys():
        connection.execute(
            "DELETE FROM id_sequences WHERE sequence_namespace = ? AND sequence_key = ?", key
        )
    for key, value in new_sequences.items():
        if old_sequences.get(key) != value:
            connection.execute(
                "INSERT INTO id_sequences(sequence_namespace, sequence_key, high_water) VALUES (?, ?, ?) "
                "ON CONFLICT(sequence_namespace, sequence_key) DO UPDATE SET high_water = excluded.high_water",
                (*key, value),
            )
    connection.execute(
        "UPDATE metadata SET revision = ?, cutover_pair_id = ? WHERE singleton = 1",
        (int(candidate["revision"]), cutover_pair_id),
    )


def create_issue_database(
    path: str | Path,
    document: Mapping[str, Any],
    *,
    cutover_pair_id: str,
    overwrite: bool = False,
) -> None:
    """Explicitly create a database; normal stores never call this helper."""
    resolved = Path(path)
    if not cutover_pair_id.strip():
        raise LiveProgressError("cutover_pair_id must be a non-empty string")
    validate_issue_ledger_v2(document)
    if resolved.exists():
        if not overwrite:
            raise LiveProgressError(f"{resolved}: refusing to overwrite an existing issue database")
    resolved.parent.mkdir(parents=True, exist_ok=True)
    working_path = (
        resolved.with_name(f".{resolved.name}.{uuid4().hex}.import.tmp")
        if resolved.exists()
        else resolved
    )
    connection: sqlite3.Connection | None = None
    try:
        connection = sqlite3.connect(working_path, timeout=10.0, isolation_level=None)
        _configure(connection, writable=True)
        connection.execute(f"PRAGMA application_id = {ISSUE_APPLICATION_ID}")
        connection.execute(f"PRAGMA user_version = {ISSUE_DATABASE_SCHEMA_VERSION}")
        connection.executescript(_SCHEMA)
        connection.execute("BEGIN IMMEDIATE")
        _replace_document(connection, document, cutover_pair_id=cutover_pair_id)
        connection.execute("COMMIT")
    except (LiveProgressError, sqlite3.Error, KeyError, TypeError, ValueError) as exc:
        if connection is not None and connection.in_transaction:
            connection.execute("ROLLBACK")
        if working_path.exists():
            working_path.unlink()
        raise LiveProgressError(f"{resolved}: failed to create workspace issue database: {exc}") from exc
    finally:
        if connection is not None:
            connection.close()
    if working_path != resolved:
        working_path.replace(resolved)


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


def _list_fields(connection: sqlite3.Connection, prefix: str, owner_column: str, owner_id: str) -> dict[str, list[Any]]:
    fields = connection.execute(
        f"SELECT field_name FROM {prefix}_list_fields WHERE {owner_column} = ? ORDER BY field_name",
        (owner_id,),
    ).fetchall()
    result: dict[str, list[Any]] = {}
    for field in fields:
        name = str(field["field_name"])
        values = connection.execute(
            f"SELECT value_json FROM {prefix}_list_values "
            f"WHERE {owner_column} = ? AND field_name = ? ORDER BY value_order",
            (owner_id, name),
        ).fetchall()
        result[name] = [_decode(str(row["value_json"])) for row in values]
    return result


def _export_from_connection(connection: sqlite3.Connection, path: Path) -> dict[str, Any]:
    metadata = _metadata_from_connection(connection, path)
    document: dict[str, Any] = {
        "version": metadata.ledger_version,
        "revision": metadata.revision,
        "id_sequences": {},
        "issues": [],
        "work_packets": [],
        "reservations": [],
    }
    for row in connection.execute(
        "SELECT issue_id, status, kind, severity, created, updated, history_present, scalar_json "
        "FROM issues ORDER BY issue_order"
    ):
        issue = _decode(str(row["scalar_json"]))
        issue_id = str(row["issue_id"])
        issue = {
            "id": issue_id,
            "status": str(row["status"]),
            "kind": str(row["kind"]),
            "severity": str(row["severity"]),
            "created": str(row["created"]),
            "updated": str(row["updated"]),
            **issue,
        }
        issue.update(_list_fields(connection, "issue", "issue_id", issue_id))
        if bool(row["history_present"]):
            issue["history"] = [
                _decode(str(event["event_json"]))
                for event in connection.execute(
                    "SELECT event_json FROM issue_history WHERE issue_id = ? ORDER BY history_order",
                    (issue_id,),
                )
            ]
        document["issues"].append(issue)
    for row in connection.execute(
        "SELECT packet_id, issue_id, state, handoff_role, resource_claims_present, scalar_json "
        "FROM work_packets ORDER BY packet_order"
    ):
        packet = _decode(str(row["scalar_json"]))
        packet_id = str(row["packet_id"])
        packet = {
            "id": packet_id,
            "issue_id": str(row["issue_id"]),
            "state": str(row["state"]),
            "handoff_role": str(row["handoff_role"]),
            **packet,
        }
        packet.update(_list_fields(connection, "work_packet", "packet_id", packet_id))
        if bool(row["resource_claims_present"]):
            packet["resource_claims"] = [
                {
                    "kind": str(claim["kind"]),
                    "id": str(claim["resource_id"]),
                    "access": str(claim["access"]),
                    **_decode(str(claim["claim_json"])),
                }
                for claim in connection.execute(
                    "SELECT kind, resource_id, access, claim_json FROM work_packet_resource_claims "
                    "WHERE packet_id = ? ORDER BY claim_order",
                    (packet_id,),
                )
            ]
        document["work_packets"].append(packet)
    for row in connection.execute(
        "SELECT reservation_id, packet_id, state, evidence_ids_present, "
        "resource_claims_present, scalar_json "
        "FROM reservations ORDER BY reservation_order"
    ):
        reservation = _decode(str(row["scalar_json"]))
        reservation_id = str(row["reservation_id"])
        reservation = {
            "id": reservation_id,
            "packet_id": str(row["packet_id"]),
            "state": str(row["state"]),
            **reservation,
        }
        if bool(row["evidence_ids_present"]):
            reservation["evidence_ids"] = [
                str(evidence["evidence_id"])
                for evidence in connection.execute(
                    "SELECT evidence_id FROM reservation_evidence "
                    "WHERE reservation_id = ? ORDER BY evidence_order",
                    (reservation_id,),
                )
            ]
        if bool(row["resource_claims_present"]):
            reservation["resource_claims"] = [
                {
                    "kind": str(claim["kind"]),
                    "id": str(claim["resource_id"]),
                    "access": str(claim["access"]),
                    **_decode(str(claim["claim_json"])),
                }
                for claim in connection.execute(
                    "SELECT kind, resource_id, access, claim_json FROM reservation_resource_claims "
                    "WHERE reservation_id = ? ORDER BY claim_order",
                    (reservation_id,),
                )
            ]
        document["reservations"].append(reservation)
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
            for row in connection.execute("PRAGMA integrity_check"):
                if str(row[0]).casefold() != "ok":
                    findings.append(f"integrity_check: {row[0]}")
            for row in connection.execute("PRAGMA foreign_key_check"):
                findings.append(
                    "foreign_key_check: " + ", ".join(str(value) for value in row)
                )
            document = _export_from_connection(connection, resolved)
            validate_issue_ledger_v2(document)
            if document_validator is not None:
                document_validator(document)
    except (LiveProgressError, sqlite3.Error, ValueError, TypeError, KeyError) as exc:
        findings.append(str(exc))
    return findings


class IssueSQLiteStore:
    """Revision-CAS store for the authoritative workspace-issue database."""

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
                        f"revision changed: expected {expected_revision}, found {metadata.revision}"
                    )
                result = RevisionCommitResult(
                    applied=apply,
                    path=self.path,
                    previous_revision=metadata.revision,
                    revision=expected_revision + 1,
                )
                if apply:
                    current = _export_from_connection(connection, self.path)
                    _sync_document(
                        connection,
                        current,
                        candidate,
                        cutover_pair_id=metadata.cutover_pair_id,
                    )
                    connection.execute("COMMIT")
                else:
                    connection.execute("ROLLBACK")
                return result
            except sqlite3.Error as exc:
                if connection.in_transaction:
                    connection.execute("ROLLBACK")
                raise LiveProgressError(
                    f"{self.path}: workspace issue transaction failed: {exc}"
                ) from exc
            except Exception:
                if connection.in_transaction:
                    connection.execute("ROLLBACK")
                raise
