from __future__ import annotations

from contextlib import closing
from dataclasses import dataclass
import json
import os
from pathlib import Path
import re
import sqlite3
from typing import Any, Iterable, Mapping


APPLICATION_ID = 0x52434C50  # "RCLP"
USER_VERSION = 2
LEGACY_USER_VERSION = 1
BUSY_TIMEOUT_MS = 10_000
SEMANTIC_SCHEMA_VERSION = 5

REVISION_DOMAIN_SEMANTIC = "semantic"
REVISION_DOMAIN_EVIDENCE_GENERATION = "evidence_generation"
REVISION_DOMAIN_SCHEDULER = "scheduler"
REVISION_DOMAIN_COLUMNS = {
    REVISION_DOMAIN_SEMANTIC: "semantic_revision",
    REVISION_DOMAIN_EVIDENCE_GENERATION: "evidence_generation_revision",
    REVISION_DOMAIN_SCHEDULER: "scheduler_revision",
}

# These are the entity maps in progress schema v5.  They deliberately remain
# distinct tablespace keys: a physical block is not a semantic span, an owner,
# a work item, or an evidence observation merely because their payloads link.
ENTITY_COLLECTIONS = (
    "binaries",
    "physical_blocks",
    "semantic_spans",
    "symbols",
    "output_sections",
    "storage_contributions",
    "owners",
    "verification_targets",
    "work_items",
    "blockers",
    "evidence",
    "tombstones",
)
SCALAR_TOP_LEVEL_KEYS = ("id_sequences", "migration")

_ADDRESS_COMPONENT_RE = re.compile(
    r"(?:^|_)(?:address|rva|start|end|end_exclusive|cursor|image_base)(?:$|_)"
)
_HEX_RE = re.compile(r"^(?:0x|sub_)([0-9a-fA-F]+)$")


class ProgressSQLiteError(RuntimeError):
    pass


class ConcurrentSQLiteProgressUpdate(ProgressSQLiteError):
    pass


# Compatibility with the backend's revision-specific internal terminology.
ConcurrentSQLiteRevisionUpdate = ConcurrentSQLiteProgressUpdate


class _DeleteFacet:
    def __repr__(self) -> str:
        return "DELETE_FACET"


# Explicit sentinel for a scoped JSON-pointer deletion. JSON null remains a
# normal storable value and therefore cannot safely double as deletion.
DELETE_FACET = _DeleteFacet()


@dataclass(frozen=True)
class SQLiteRevisionVector:
    """Independent concurrency coordinates for one progress snapshot.

    These counters order writes; accepted-state invalidation must remain a
    content/dependency decision rather than an equality check against the
    newest unrelated counter.
    """

    transaction_revision: int
    semantic_revision: int
    evidence_generation_revision: int
    scheduler_revision: int

    def to_dict(self) -> dict[str, int]:
        return {
            "transaction_revision": self.transaction_revision,
            "semantic_revision": self.semantic_revision,
            "evidence_generation_revision": self.evidence_generation_revision,
            "scheduler_revision": self.scheduler_revision,
        }


@dataclass(frozen=True)
class SQLiteMetadata:
    schema_version: int
    revision: int
    cutover_pair_id: str
    application_id: int
    user_version: int
    semantic_revision: int
    evidence_generation_revision: int
    scheduler_revision: int

    @property
    def transaction_revision(self) -> int:
        return self.revision

    @property
    def revision_vector(self) -> SQLiteRevisionVector:
        return SQLiteRevisionVector(
            transaction_revision=self.revision,
            semantic_revision=self.semantic_revision,
            evidence_generation_revision=self.evidence_generation_revision,
            scheduler_revision=self.scheduler_revision,
        )


@dataclass(frozen=True)
class SQLiteCommitResult:
    applied: bool
    path: Path
    previous_revision: int
    revision: int
    upserted_entities: int
    deleted_entities: int
    updated_top_level_values: int
    previous_revision_vector: SQLiteRevisionVector
    revision_vector: SQLiteRevisionVector

    def to_dict(self) -> dict[str, Any]:
        return {
            "applied": self.applied,
            "path": self.path.as_posix(),
            "previous_revision": self.previous_revision,
            "revision": self.revision,
            "upserted_entities": self.upserted_entities,
            "deleted_entities": self.deleted_entities,
            "updated_top_level_values": self.updated_top_level_values,
            "previous_revision_vector": self.previous_revision_vector.to_dict(),
            "revision_vector": self.revision_vector.to_dict(),
        }


@dataclass(frozen=True)
class SQLiteValidation:
    ok: bool
    integrity_check: tuple[str, ...]
    foreign_key_violations: tuple[tuple[Any, ...], ...]
    errors: tuple[str, ...]


def _source_mapping(source: Any) -> Mapping[str, Any]:
    data = getattr(source, "data", source)
    if not isinstance(data, Mapping):
        raise ProgressSQLiteError("progress source must be a mapping or ProgressDocument")
    return data


def _json(value: Any) -> str:
    try:
        return json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"))
    except (TypeError, ValueError) as exc:
        raise ProgressSQLiteError(f"progress value is not JSON serializable: {exc}") from exc


def _json_pointer(parts: tuple[str, ...]) -> str:
    if not parts:
        return ""
    return "/" + "/".join(part.replace("~", "~0").replace("/", "~1") for part in parts)


def _decode_json_pointer(pointer: str) -> tuple[str, ...]:
    if pointer == "":
        return ()
    if not isinstance(pointer, str) or not pointer.startswith("/"):
        raise ProgressSQLiteError(
            f"scoped facet path must be an RFC 6901 JSON pointer, found {pointer!r}"
        )
    parts: list[str] = []
    for encoded in pointer[1:].split("/"):
        decoded: list[str] = []
        index = 0
        while index < len(encoded):
            character = encoded[index]
            if character != "~":
                decoded.append(character)
                index += 1
                continue
            if index + 1 >= len(encoded) or encoded[index + 1] not in {"0", "1"}:
                raise ProgressSQLiteError(f"invalid JSON-pointer escape in {pointer!r}")
            decoded.append("~" if encoded[index + 1] == "0" else "/")
            index += 2
        parts.append("".join(decoded))
    return tuple(parts)


def _validated_pointer_patches(patches: Mapping[str, Any]) -> list[tuple[tuple[str, ...], Any]]:
    if not isinstance(patches, Mapping):
        raise ProgressSQLiteError("scoped facet patches must be an object keyed by JSON pointer")
    decoded = [(_decode_json_pointer(pointer), value) for pointer, value in patches.items()]
    paths = [path for path, _value in decoded]
    for index, path in enumerate(paths):
        for other in paths[index + 1 :]:
            shorter, longer = (path, other) if len(path) <= len(other) else (other, path)
            if longer[: len(shorter)] == shorter:
                raise ProgressSQLiteError(
                    "overlapping scoped facet paths are not allowed: "
                    f"{_json_pointer(path)!r} and {_json_pointer(other)!r}"
                )
    for _path, value in decoded:
        if value is not DELETE_FACET:
            _json(value)
    return decoded


def _apply_pointer_patches(current: Any, patches: Mapping[str, Any], *, context: str) -> Any:
    decoded = _validated_pointer_patches(patches)
    root = json.loads(_json(current))
    for path, value in decoded:
        if not path:
            if value is DELETE_FACET:
                raise ProgressSQLiteError(f"{context}: root facet cannot be deleted")
            root = json.loads(_json(value))
            continue
        if not isinstance(root, dict):
            raise ProgressSQLiteError(f"{context}: cannot address a child facet of a non-object")
        parent = root
        for part in path[:-1]:
            child = parent.get(part)
            if child is None:
                child = {}
                parent[part] = child
            if not isinstance(child, dict):
                raise ProgressSQLiteError(
                    f"{context}: {_json_pointer(path)!r} traverses non-object facet {part!r}"
                )
            parent = child
        leaf = path[-1]
        if value is DELETE_FACET:
            parent.pop(leaf, None)
        else:
            parent[leaf] = json.loads(_json(value))
    return root


def _address_value(value: Any) -> int | None:
    if isinstance(value, bool):
        return None
    if isinstance(value, int):
        return value if 0 <= value <= 0x7FFF_FFFF_FFFF_FFFF else None
    if not isinstance(value, str):
        return None
    match = _HEX_RE.fullmatch(value.strip())
    if match is None:
        return None
    parsed = int(match.group(1), 16)
    return parsed if parsed <= 0x7FFF_FFFF_FFFF_FFFF else None


def _walk(value: Any, parts: tuple[str, ...] = ()) -> Iterable[tuple[tuple[str, ...], Any]]:
    yield parts, value
    if isinstance(value, Mapping):
        for key, child in value.items():
            yield from _walk(child, (*parts, str(key)))
    elif isinstance(value, list):
        for index, child in enumerate(value):
            yield from _walk(child, (*parts, str(index)))


def _address_rows(entity_id: str, payload: Any) -> list[tuple[str, int, str]]:
    rows: list[tuple[str, int, str]] = []
    entity_address = _address_value(entity_id.rsplit(":", 1)[-1])
    if entity_address is not None:
        rows.append(("$entity-id", entity_address, "entity-id"))
    for parts, value in _walk(payload):
        if not parts:
            continue
        # Values in an address array have a numeric final JSON-pointer token;
        # classify them by the nearest named ancestor instead of silently
        # omitting the authoritative integer index rows.
        field = next(
            (
                part.casefold().replace("-", "_")
                for part in reversed(parts)
                if not part.isdecimal()
            ),
            "",
        )
        if _ADDRESS_COMPONENT_RE.search(field) is None:
            continue
        address = _address_value(value)
        if address is not None:
            rows.append((_json_pointer(parts), address, field))
    return rows


def _relationship_rows(
    payload: Any, entity_locations: Mapping[str, tuple[str, str]]
) -> list[tuple[str, str, str]]:
    rows: list[tuple[str, str, str]] = []
    for parts, value in _walk(payload):
        if not parts or not isinstance(value, str):
            continue
        target = entity_locations.get(value)
        if target is not None:
            rows.append((_json_pointer(parts), target[0], target[1]))
    return rows


def _validate_document_shape(data: Mapping[str, Any]) -> tuple[int, int]:
    schema_version = data.get("schema_version")
    revision = data.get("revision")
    if schema_version != SEMANTIC_SCHEMA_VERSION:
        raise ProgressSQLiteError(
            f"schema_version must be {SEMANTIC_SCHEMA_VERSION}, found {schema_version!r}"
        )
    if not isinstance(revision, int) or isinstance(revision, bool) or revision < 0:
        raise ProgressSQLiteError("revision must be a non-negative integer")
    for collection in ENTITY_COLLECTIONS:
        rows = data.get(collection)
        if not isinstance(rows, Mapping):
            raise ProgressSQLiteError(f"{collection} must be an object")
        for entity_id, payload in rows.items():
            if not isinstance(entity_id, str) or not entity_id:
                raise ProgressSQLiteError(f"{collection} contains an invalid entity id")
            _json(payload)
    for key in SCALAR_TOP_LEVEL_KEYS:
        if key not in data:
            raise ProgressSQLiteError(f"missing top-level progress field {key}")
        _json(data[key])
    unexpected = set(data) - {"schema_version", "revision", *ENTITY_COLLECTIONS, *SCALAR_TOP_LEVEL_KEYS}
    if unexpected:
        raise ProgressSQLiteError("unsupported top-level progress fields: " + ", ".join(sorted(unexpected)))
    return schema_version, revision


_SCHEMA = """
CREATE TABLE metadata (
    singleton INTEGER PRIMARY KEY CHECK (singleton = 1),
    schema_version INTEGER NOT NULL CHECK (schema_version > 0),
    revision INTEGER NOT NULL CHECK (revision >= 0),
    cutover_pair_id TEXT NOT NULL CHECK (length(cutover_pair_id) > 0),
    semantic_revision INTEGER NOT NULL CHECK (semantic_revision >= 0),
    evidence_generation_revision INTEGER NOT NULL CHECK (evidence_generation_revision >= 0),
    scheduler_revision INTEGER NOT NULL CHECK (scheduler_revision >= 0)
) STRICT;
CREATE TABLE top_level_values (
    key TEXT PRIMARY KEY,
    payload TEXT NOT NULL CHECK (json_valid(payload))
) STRICT;
CREATE TABLE entities (
    collection TEXT NOT NULL,
    entity_id TEXT NOT NULL,
    payload TEXT NOT NULL CHECK (json_valid(payload)),
    PRIMARY KEY (collection, entity_id)
) STRICT;
CREATE UNIQUE INDEX entities_global_id ON entities(entity_id);
CREATE TABLE address_index (
    collection TEXT NOT NULL,
    entity_id TEXT NOT NULL,
    json_pointer TEXT NOT NULL,
    address INTEGER NOT NULL CHECK (address >= 0),
    address_kind TEXT NOT NULL,
    PRIMARY KEY (collection, entity_id, json_pointer, address, address_kind),
    FOREIGN KEY (collection, entity_id) REFERENCES entities(collection, entity_id)
        ON DELETE CASCADE
) STRICT;
CREATE INDEX address_index_by_value ON address_index(address, collection, entity_id);
CREATE TABLE entity_facets (
    collection TEXT NOT NULL,
    entity_id TEXT NOT NULL,
    binary TEXT,
    kind TEXT,
    primary_address INTEGER,
    start_address INTEGER,
    end_address INTEGER,
    physical_block_id TEXT,
    pipeline_class TEXT,
    authored_order_role TEXT,
    work_state TEXT,
    reservation_state TEXT,
    PRIMARY KEY (collection, entity_id),
    FOREIGN KEY (collection, entity_id) REFERENCES entities(collection, entity_id)
        ON DELETE CASCADE
) STRICT;
CREATE INDEX entity_facets_scheduler ON entity_facets(
    collection, binary, pipeline_class, authored_order_role,
    work_state, reservation_state, primary_address, start_address
);
CREATE INDEX entity_facets_block ON entity_facets(physical_block_id, collection, entity_id);
CREATE TABLE relationship_index (
    source_collection TEXT NOT NULL,
    source_entity_id TEXT NOT NULL,
    json_pointer TEXT NOT NULL,
    target_collection TEXT NOT NULL,
    target_entity_id TEXT NOT NULL,
    PRIMARY KEY (
        source_collection, source_entity_id, json_pointer,
        target_collection, target_entity_id
    ),
    FOREIGN KEY (source_collection, source_entity_id)
        REFERENCES entities(collection, entity_id) ON DELETE CASCADE,
    FOREIGN KEY (target_collection, target_entity_id)
        REFERENCES entities(collection, entity_id) ON DELETE RESTRICT
) STRICT;
CREATE INDEX relationship_index_by_target
    ON relationship_index(target_collection, target_entity_id, source_collection, source_entity_id);
"""


class ProgressSQLiteStore:
    """Normalized durable store for the unified reconstruction tracker.

    Opening is fail-closed and never initializes a missing or malformed file.
    Each operation uses its own connection so callers do not retain an implicit
    write transaction or depend on a concrete session scratch directory.
    """

    def __init__(self, path: str | Path, *, read_only: bool = False) -> None:
        self.path = Path(path)
        self.read_only = read_only
        if not self.path.is_file():
            raise ProgressSQLiteError(f"{self.path}: SQLite progress store does not exist")
        with closing(self._connect()) as connection:
            self._validate_header(connection)

    @classmethod
    def create_from_mapping(
        cls,
        path: str | Path,
        source: Any,
        *,
        cutover_pair_id: str,
        overwrite: bool = False,
    ) -> "ProgressSQLiteStore":
        target = Path(path)
        data = _source_mapping(source)
        schema_version, revision = _validate_document_shape(data)
        if not isinstance(cutover_pair_id, str) or not cutover_pair_id.strip():
            raise ProgressSQLiteError("cutover_pair_id must be a non-empty string")
        if target.exists() and not overwrite:
            raise ProgressSQLiteError(f"{target}: refusing to overwrite existing SQLite store")
        target.parent.mkdir(parents=True, exist_ok=True)
        temporary = target.with_name(f".{target.name}.{os.getpid()}.tmp")
        if temporary.exists():
            temporary.unlink()
        connection: sqlite3.Connection | None = None
        try:
            connection = sqlite3.connect(temporary, timeout=BUSY_TIMEOUT_MS / 1000)
            cls._configure_create_connection(connection)
            connection.executescript(_SCHEMA)
            connection.execute(
                "INSERT INTO metadata("
                "singleton, schema_version, revision, cutover_pair_id, "
                "semantic_revision, evidence_generation_revision, scheduler_revision"
                ") VALUES (1, ?, ?, ?, ?, ?, ?)",
                (
                    schema_version,
                    revision,
                    cutover_pair_id,
                    revision,
                    revision,
                    revision,
                ),
            )
            for key in SCALAR_TOP_LEVEL_KEYS:
                connection.execute(
                    "INSERT INTO top_level_values(key, payload) VALUES (?, ?)",
                    (key, _json(data[key])),
                )
            for collection in ENTITY_COLLECTIONS:
                for entity_id, payload in data[collection].items():
                    connection.execute(
                        "INSERT INTO entities(collection, entity_id, payload) VALUES (?, ?, ?)",
                        (collection, entity_id, _json(payload)),
                    )
            cls._rebuild_all_indexes(connection)
            connection.commit()
            connection.close()
            connection = None
            os.replace(temporary, target)
        except Exception:
            if connection is not None:
                connection.close()
            if temporary.exists():
                temporary.unlink()
            raise
        return cls(target)

    @staticmethod
    def _configure_connection(connection: sqlite3.Connection) -> None:
        connection.execute(f"PRAGMA busy_timeout={BUSY_TIMEOUT_MS}")
        connection.execute("PRAGMA foreign_keys=ON")
        connection.execute("PRAGMA synchronous=FULL")

    @classmethod
    def _configure_create_connection(cls, connection: sqlite3.Connection) -> None:
        cls._configure_connection(connection)
        connection.execute("PRAGMA journal_mode=DELETE")
        connection.execute(f"PRAGMA application_id={APPLICATION_ID}")
        connection.execute(f"PRAGMA user_version={USER_VERSION}")

    def _connect(self) -> sqlite3.Connection:
        try:
            if self.read_only:
                uri = self.path.resolve().as_uri() + "?mode=ro"
                connection = sqlite3.connect(uri, uri=True, timeout=BUSY_TIMEOUT_MS / 1000)
                connection.execute(f"PRAGMA busy_timeout={BUSY_TIMEOUT_MS}")
                connection.execute("PRAGMA foreign_keys=ON")
            else:
                # mode=rw is intentional: ordinary open must never initialize
                # a missing database or stamp an unrelated SQLite file.
                uri = self.path.resolve().as_uri() + "?mode=rw"
                connection = sqlite3.connect(
                    uri, uri=True, timeout=BUSY_TIMEOUT_MS / 1000
                )
                self._configure_connection(connection)
            connection.row_factory = sqlite3.Row
            return connection
        except sqlite3.Error as exc:
            raise ProgressSQLiteError(f"{self.path}: cannot open SQLite progress store: {exc}") from exc

    @staticmethod
    def _read_revision_vector(
        connection: sqlite3.Connection, *, user_version: int | None = None
    ) -> SQLiteRevisionVector:
        version = (
            int(connection.execute("PRAGMA user_version").fetchone()[0])
            if user_version is None
            else user_version
        )
        if version == LEGACY_USER_VERSION:
            row = connection.execute(
                "SELECT revision FROM metadata WHERE singleton=1"
            ).fetchone()
            if row is None:
                raise ProgressSQLiteError("SQLite progress metadata singleton is missing")
            revision = int(row[0])
            return SQLiteRevisionVector(revision, revision, revision, revision)
        if version != USER_VERSION:
            raise ProgressSQLiteError(
                f"SQLite user_version mismatch: expected {LEGACY_USER_VERSION} or "
                f"{USER_VERSION}, found {version}"
            )
        try:
            row = connection.execute(
                "SELECT revision, semantic_revision, evidence_generation_revision, "
                "scheduler_revision FROM metadata WHERE singleton=1"
            ).fetchone()
        except sqlite3.Error as exc:
            raise ProgressSQLiteError(f"invalid revision-domain metadata: {exc}") from exc
        if row is None:
            raise ProgressSQLiteError("SQLite progress metadata singleton is missing")
        return SQLiteRevisionVector(*(int(row[index]) for index in range(4)))

    @classmethod
    def _migrate_revision_domains_in_transaction(
        cls, connection: sqlite3.Connection
    ) -> SQLiteRevisionVector:
        """Upgrade storage v1 to v2 without changing the semantic document revision."""

        user_version = int(connection.execute("PRAGMA user_version").fetchone()[0])
        if user_version == USER_VERSION:
            columns = {
                str(row[1])
                for row in connection.execute("PRAGMA table_info(metadata)")
            }
            if "revision_domains_enforced" not in columns:
                # A database may have been created directly at storage v2
                # before domain enforcement existed.  The first scoped write
                # is the irreversible boundary: after it commits, no caller
                # may fall back to the legacy global revision coordinate.
                connection.execute(
                    "ALTER TABLE metadata ADD COLUMN revision_domains_enforced "
                    "INTEGER NOT NULL DEFAULT 1 CHECK (revision_domains_enforced = 1)"
                )
            return cls._read_revision_vector(connection, user_version=user_version)
        if user_version != LEGACY_USER_VERSION:
            raise ProgressSQLiteError(
                f"cannot migrate unsupported SQLite user_version {user_version}"
            )
        row = connection.execute(
            "SELECT revision FROM metadata WHERE singleton=1"
        ).fetchone()
        if row is None:
            raise ProgressSQLiteError("SQLite progress metadata singleton is missing")
        revision = int(row[0])
        for column in REVISION_DOMAIN_COLUMNS.values():
            connection.execute(
                f"ALTER TABLE metadata ADD COLUMN {column} "
                "INTEGER NOT NULL DEFAULT 0 CHECK ("
                f"{column} >= 0)"
            )
            connection.execute(
                f"UPDATE metadata SET {column}=? WHERE singleton=1", (revision,)
            )
        connection.execute(
            "ALTER TABLE metadata ADD COLUMN revision_domains_enforced "
            "INTEGER NOT NULL DEFAULT 1 CHECK (revision_domains_enforced = 1)"
        )
        connection.execute(f"PRAGMA user_version={USER_VERSION}")
        return SQLiteRevisionVector(revision, revision, revision, revision)

    @staticmethod
    def _validate_header(connection: sqlite3.Connection) -> None:
        try:
            application_id = int(connection.execute("PRAGMA application_id").fetchone()[0])
            user_version = int(connection.execute("PRAGMA user_version").fetchone()[0])
            journal_mode = str(connection.execute("PRAGMA journal_mode").fetchone()[0]).casefold()
            foreign_keys = int(connection.execute("PRAGMA foreign_keys").fetchone()[0])
            synchronous = int(connection.execute("PRAGMA synchronous").fetchone()[0])
            busy_timeout = int(connection.execute("PRAGMA busy_timeout").fetchone()[0])
            row = connection.execute(
                "SELECT schema_version, revision, cutover_pair_id FROM metadata WHERE singleton=1"
            ).fetchone()
        except sqlite3.Error as exc:
            raise ProgressSQLiteError(f"invalid SQLite progress schema: {exc}") from exc
        if application_id != APPLICATION_ID:
            raise ProgressSQLiteError(
                f"SQLite application_id mismatch: expected {APPLICATION_ID}, found {application_id}"
            )
        if user_version not in {LEGACY_USER_VERSION, USER_VERSION}:
            raise ProgressSQLiteError(
                f"SQLite user_version mismatch: expected {LEGACY_USER_VERSION} or "
                f"{USER_VERSION}, found {user_version}"
            )
        if journal_mode != "delete":
            raise ProgressSQLiteError(f"SQLite journal_mode must be DELETE, found {journal_mode}")
        if foreign_keys != 1:
            raise ProgressSQLiteError("SQLite foreign key enforcement is disabled")
        if synchronous != 2:
            raise ProgressSQLiteError(f"SQLite synchronous must be FULL, found {synchronous}")
        if busy_timeout != BUSY_TIMEOUT_MS:
            raise ProgressSQLiteError(
                f"SQLite busy_timeout must be {BUSY_TIMEOUT_MS}, found {busy_timeout}"
            )
        if row is None or not str(row[2]).strip():
            raise ProgressSQLiteError("SQLite progress metadata singleton is missing or invalid")
        if int(row[0]) != SEMANTIC_SCHEMA_VERSION:
            raise ProgressSQLiteError(
                f"semantic schema_version must be {SEMANTIC_SCHEMA_VERSION}, found {row[0]}"
            )
        ProgressSQLiteStore._read_revision_vector(
            connection, user_version=user_version
        )

    @staticmethod
    def _entity_locations(connection: sqlite3.Connection) -> dict[str, tuple[str, str]]:
        locations: dict[str, tuple[str, str]] = {}
        duplicates: set[str] = set()
        for row in connection.execute("SELECT collection, entity_id FROM entities"):
            entity_id = str(row[1])
            if entity_id in locations:
                duplicates.add(entity_id)
            else:
                locations[entity_id] = (str(row[0]), entity_id)
        for entity_id in duplicates:
            locations.pop(entity_id, None)
        return locations

    @classmethod
    def _rebuild_entity_indexes(
        cls,
        connection: sqlite3.Connection,
        collection: str,
        entity_id: str,
        payload: Any,
        locations: Mapping[str, tuple[str, str]],
    ) -> None:
        connection.execute(
            "DELETE FROM address_index WHERE collection=? AND entity_id=?",
            (collection, entity_id),
        )
        connection.execute(
            "DELETE FROM relationship_index WHERE source_collection=? AND source_entity_id=?",
            (collection, entity_id),
        )
        connection.execute(
            "DELETE FROM entity_facets WHERE collection=? AND entity_id=?",
            (collection, entity_id),
        )
        connection.executemany(
            "INSERT INTO address_index VALUES (?, ?, ?, ?, ?)",
            (
                (collection, entity_id, pointer, address, kind)
                for pointer, address, kind in _address_rows(entity_id, payload)
            ),
        )
        def string_field(name: str) -> str | None:
            value = payload.get(name) if isinstance(payload, Mapping) else None
            return value if isinstance(value, str) else None

        reservation = payload.get("reservation") if isinstance(payload, Mapping) else None
        reservation_state = (
            reservation.get("state") if isinstance(reservation, Mapping) else None
        )
        primary_address = _address_value(
            payload.get("address") if isinstance(payload, Mapping) else None
        )
        if primary_address is None:
            primary_address = _address_value(entity_id.rsplit(":", 1)[-1])
        connection.execute(
            "INSERT INTO entity_facets VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            (
                collection,
                entity_id,
                string_field("binary"),
                string_field("kind"),
                primary_address,
                _address_value(payload.get("start")) if isinstance(payload, Mapping) else None,
                _address_value(payload.get("end_exclusive")) if isinstance(payload, Mapping) else None,
                string_field("physical_block_id"),
                string_field("pipeline_class"),
                string_field("authored_order_role"),
                string_field("state"),
                reservation_state if isinstance(reservation_state, str) else None,
            ),
        )
        connection.executemany(
            "INSERT INTO relationship_index VALUES (?, ?, ?, ?, ?)",
            (
                (collection, entity_id, pointer, target_collection, target_id)
                for pointer, target_collection, target_id in _relationship_rows(payload, locations)
            ),
        )

    @classmethod
    def _rebuild_all_indexes(cls, connection: sqlite3.Connection) -> None:
        connection.execute("DELETE FROM relationship_index")
        connection.execute("DELETE FROM address_index")
        connection.execute("DELETE FROM entity_facets")
        locations = cls._entity_locations(connection)
        for row in connection.execute("SELECT collection, entity_id, payload FROM entities"):
            cls._rebuild_entity_indexes(
                connection, str(row[0]), str(row[1]), json.loads(str(row[2])), locations
            )

    def metadata(self) -> SQLiteMetadata:
        with closing(self._connect()) as connection:
            self._validate_header(connection)
            user_version = int(connection.execute("PRAGMA user_version").fetchone()[0])
            row = connection.execute(
                "SELECT schema_version, revision, cutover_pair_id FROM metadata WHERE singleton=1"
            ).fetchone()
            assert row is not None
            revision_vector = self._read_revision_vector(
                connection, user_version=user_version
            )
            return SQLiteMetadata(
                schema_version=int(row[0]),
                revision=int(row[1]),
                cutover_pair_id=str(row[2]),
                application_id=int(connection.execute("PRAGMA application_id").fetchone()[0]),
                user_version=user_version,
                semantic_revision=revision_vector.semantic_revision,
                evidence_generation_revision=revision_vector.evidence_generation_revision,
                scheduler_revision=revision_vector.scheduler_revision,
            )

    def read_revision(self) -> int:
        return self.metadata().revision

    def read_revision_vector(self) -> SQLiteRevisionVector:
        """Read all progress concurrency coordinates without materializing entities."""

        return self.metadata().revision_vector

    def migrate_revision_domains(
        self, *, expected_revision: int, apply: bool
    ) -> SQLiteRevisionVector:
        """Explicitly upgrade legacy storage while preserving the document revision.

        New scoped writes also perform this migration inside their own guarded
        transaction, so read-only diagnostics and legacy reads never mutate a
        v1 database merely by opening it.
        """

        if self.read_only:
            raise ProgressSQLiteError(
                "cannot migrate through a read-only SQLite progress store"
            )
        connection = self._connect()
        try:
            connection.execute("BEGIN IMMEDIATE")
            observed = self._read_revision_vector(connection)
            if observed.transaction_revision != expected_revision:
                raise ConcurrentSQLiteRevisionUpdate(
                    "revision changed: expected "
                    f"{expected_revision}, found {observed.transaction_revision}"
                )
            migrated = self._migrate_revision_domains_in_transaction(connection)
            if apply:
                connection.commit()
            else:
                connection.rollback()
            return migrated
        except (ProgressSQLiteError, ConcurrentSQLiteRevisionUpdate):
            connection.rollback()
            raise
        except sqlite3.Error as exc:
            connection.rollback()
            raise ProgressSQLiteError(
                f"SQLite progress storage migration failed: {exc}"
            ) from exc
        finally:
            connection.close()

    def materialize(self) -> dict[str, Any]:
        with closing(self._connect()) as connection:
            self._validate_header(connection)
            metadata = connection.execute(
                "SELECT schema_version, revision FROM metadata WHERE singleton=1"
            ).fetchone()
            assert metadata is not None
            result: dict[str, Any] = {
                "schema_version": int(metadata[0]),
                "revision": int(metadata[1]),
                **{key: {} for key in SCALAR_TOP_LEVEL_KEYS},
                **{collection: {} for collection in ENTITY_COLLECTIONS},
            }
            for row in connection.execute("SELECT key, payload FROM top_level_values"):
                result[str(row[0])] = json.loads(str(row[1]))
            for row in connection.execute(
                "SELECT collection, entity_id, payload FROM entities ORDER BY collection, entity_id"
            ):
                result[str(row[0])][str(row[1])] = json.loads(str(row[2]))
            return result

    def query_entity_ids(
        self,
        *,
        collection: str | None = None,
        binary: str | None = None,
        pipeline_class: str | None = None,
        authored_order_role: str | None = None,
        work_state: str | None = None,
        reservation_state: str | None = None,
        physical_block_id: str | None = None,
        address_at_or_after: int | None = None,
        limit: int | None = None,
    ) -> list[tuple[str, str]]:
        """Return scheduler candidates from normalized facets without JSON scans."""

        clauses: list[str] = []
        parameters: list[Any] = []
        for column, value in (
            ("collection", collection),
            ("binary", binary),
            ("pipeline_class", pipeline_class),
            ("authored_order_role", authored_order_role),
            ("work_state", work_state),
            ("reservation_state", reservation_state),
            ("physical_block_id", physical_block_id),
        ):
            if value is not None:
                clauses.append(f"{column}=?")
                parameters.append(value)
        if address_at_or_after is not None:
            if isinstance(address_at_or_after, bool) or address_at_or_after < 0:
                raise ProgressSQLiteError("address_at_or_after must be a non-negative integer")
            clauses.append("COALESCE(primary_address, start_address)>=?")
            parameters.append(address_at_or_after)
        if limit is not None:
            if isinstance(limit, bool) or limit < 1:
                raise ProgressSQLiteError("limit must be a positive integer")
        sql = "SELECT collection, entity_id FROM entity_facets"
        if clauses:
            sql += " WHERE " + " AND ".join(clauses)
        sql += " ORDER BY COALESCE(primary_address, start_address, 9223372036854775807), collection, entity_id"
        if limit is not None:
            sql += " LIMIT ?"
            parameters.append(limit)
        with closing(self._connect()) as connection:
            return [(str(row[0]), str(row[1])) for row in connection.execute(sql, parameters)]

    def commit(
        self,
        proposed: Any,
        *,
        expected_revision: int,
        apply: bool,
    ) -> SQLiteCommitResult:
        data = dict(_source_mapping(proposed))
        schema_version, _ = _validate_document_shape(data)
        current_rows: dict[tuple[str, str], str] = {}
        with closing(self._connect()) as connection:
            metadata = connection.execute(
                "SELECT schema_version FROM metadata WHERE singleton=1"
            ).fetchone()
            if metadata is None or int(metadata[0]) != schema_version:
                raise ProgressSQLiteError("proposed semantic schema_version differs from SQLite metadata")
            for row in connection.execute("SELECT collection, entity_id, payload FROM entities"):
                current_rows[(str(row[0]), str(row[1]))] = str(row[2])
            current_top = {
                str(row[0]): str(row[1])
                for row in connection.execute("SELECT key, payload FROM top_level_values")
            }
        wanted_rows = {
            (collection, entity_id): _json(payload)
            for collection in ENTITY_COLLECTIONS
            for entity_id, payload in data[collection].items()
        }
        upserts: dict[str, dict[str, Any]] = {}
        deletes: dict[str, list[str]] = {}
        for (collection, entity_id), encoded in wanted_rows.items():
            if current_rows.get((collection, entity_id)) != encoded:
                upserts.setdefault(collection, {})[entity_id] = data[collection][entity_id]
        for collection, entity_id in current_rows.keys() - wanted_rows.keys():
            deletes.setdefault(collection, []).append(entity_id)
        top_updates = {
            key: data[key]
            for key in SCALAR_TOP_LEVEL_KEYS
            if current_top.get(key) != _json(data[key])
        }
        return self.persist_changes(
            upserts=upserts,
            deletes=deletes,
            top_level_updates=top_updates,
            expected_revision=expected_revision,
            apply=apply,
        )

    def persist_changes(
        self,
        *,
        upserts: Mapping[str, Mapping[str, Any]] | None = None,
        deletes: Mapping[str, Iterable[str]] | None = None,
        top_level_updates: Mapping[str, Any] | None = None,
        expected_revision: int,
        apply: bool,
    ) -> SQLiteCommitResult:
        if self.read_only:
            raise ProgressSQLiteError("cannot persist through a read-only SQLite progress store")
        upserts = upserts or {}
        deletes = deletes or {}
        top_level_updates = top_level_updates or {}
        unknown = (set(upserts) | set(deletes)) - set(ENTITY_COLLECTIONS)
        if unknown:
            raise ProgressSQLiteError("unknown entity collections: " + ", ".join(sorted(unknown)))
        unknown_top = set(top_level_updates) - set(SCALAR_TOP_LEVEL_KEYS)
        if unknown_top:
            raise ProgressSQLiteError("unsupported top-level updates: " + ", ".join(sorted(unknown_top)))
        encoded_upserts: dict[tuple[str, str], str] = {}
        delete_keys: set[tuple[str, str]] = set()
        for collection, rows in upserts.items():
            for entity_id, payload in rows.items():
                if not isinstance(entity_id, str) or not entity_id:
                    raise ProgressSQLiteError("entity ids must be non-empty strings")
                encoded_upserts[(collection, entity_id)] = _json(payload)
        for collection, entity_ids in deletes.items():
            for entity_id in entity_ids:
                delete_keys.add((collection, str(entity_id)))
        overlap = set(encoded_upserts) & delete_keys
        if overlap:
            raise ProgressSQLiteError(f"entities cannot be both upserted and deleted: {sorted(overlap)!r}")

        connection = self._connect()
        try:
            connection.execute("BEGIN IMMEDIATE")
            observed_vector = self._read_revision_vector(connection)
            user_version = int(
                connection.execute("PRAGMA user_version").fetchone()[0]
            )
            enforced_columns = {
                str(row[1])
                for row in connection.execute("PRAGMA table_info(metadata)")
            }
            if (
                user_version == USER_VERSION
                and "revision_domains_enforced" in enforced_columns
            ):
                raise ProgressSQLiteError(
                    "legacy single-revision mutation is disabled after revision-domain "
                    "migration; use persist_scoped_changes with explicit semantic, "
                    "evidence-generation, or scheduler CAS coordinates"
                )
            observed = observed_vector.transaction_revision
            if observed != expected_revision:
                raise ConcurrentSQLiteRevisionUpdate(
                    f"revision changed: expected {expected_revision}, found {observed}"
                )
            # Remove relationship rows first so a deliberately deleted target
            # can be removed if all referring entity payloads are changed in
            # the same transaction.
            for collection, entity_id in set(encoded_upserts) | delete_keys:
                connection.execute(
                    "DELETE FROM relationship_index WHERE source_collection=? AND source_entity_id=?",
                    (collection, entity_id),
                )
            # Evidence observations are immutable history rather than live
            # ownership dependencies.  Their payloads deliberately retain
            # scoped ids after an entity is retired, while a canonical full
            # index rebuild omits those strings once the target is absent.
            # Drop only those stale inbound index rows here; every surviving
            # authoritative relationship remains protected by RESTRICT.
            for collection, entity_id in delete_keys:
                connection.execute(
                    "DELETE FROM relationship_index "
                    "WHERE source_collection='evidence' "
                    "AND target_collection=? AND target_entity_id=?",
                    (collection, entity_id),
                )
            for collection, entity_id in delete_keys:
                connection.execute(
                    "DELETE FROM entities WHERE collection=? AND entity_id=?",
                    (collection, entity_id),
                )
            for (collection, entity_id), payload in encoded_upserts.items():
                connection.execute(
                    "INSERT INTO entities(collection, entity_id, payload) VALUES (?, ?, ?) "
                    "ON CONFLICT(collection, entity_id) DO UPDATE SET payload=excluded.payload",
                    (collection, entity_id, payload),
                )
            for key, value in top_level_updates.items():
                connection.execute(
                    "INSERT INTO top_level_values(key, payload) VALUES (?, ?) "
                    "ON CONFLICT(key) DO UPDATE SET payload=excluded.payload",
                    (key, _json(value)),
                )
            # Tracker entity ids are globally unique.  Consequently both
            # authoritative indexes can remain bounded to the changed rows;
            # RESTRICT catches an attempt to delete a still-referenced target.
            locations = self._entity_locations(connection)
            for (collection, entity_id), encoded in encoded_upserts.items():
                self._rebuild_entity_indexes(
                    connection, collection, entity_id, json.loads(encoded), locations
                )
            user_version = int(connection.execute("PRAGMA user_version").fetchone()[0])
            if user_version == USER_VERSION:
                connection.execute(
                    "UPDATE metadata SET revision=?, semantic_revision=?, "
                    "evidence_generation_revision=?, scheduler_revision=? "
                    "WHERE singleton=1 AND revision=?",
                    (
                        expected_revision + 1,
                        observed_vector.semantic_revision + 1,
                        observed_vector.evidence_generation_revision + 1,
                        observed_vector.scheduler_revision + 1,
                        expected_revision,
                    ),
                )
                result_vector = SQLiteRevisionVector(
                    expected_revision + 1,
                    observed_vector.semantic_revision + 1,
                    observed_vector.evidence_generation_revision + 1,
                    observed_vector.scheduler_revision + 1,
                )
            else:
                connection.execute(
                    "UPDATE metadata SET revision=? WHERE singleton=1 AND revision=?",
                    (expected_revision + 1, expected_revision),
                )
                # A v1 store has one conservative global coordinate. Legacy
                # commits therefore advance every synthetic domain together.
                result_vector = SQLiteRevisionVector(
                    expected_revision + 1,
                    expected_revision + 1,
                    expected_revision + 1,
                    expected_revision + 1,
                )
            violations = tuple(tuple(row) for row in connection.execute("PRAGMA foreign_key_check"))
            if violations:
                raise ProgressSQLiteError(f"foreign-key violations after proposed commit: {violations!r}")
            result = SQLiteCommitResult(
                applied=apply,
                path=self.path,
                previous_revision=observed,
                revision=expected_revision + 1,
                upserted_entities=len(encoded_upserts),
                deleted_entities=len(delete_keys),
                updated_top_level_values=len(top_level_updates),
                previous_revision_vector=observed_vector,
                revision_vector=result_vector,
            )
            if apply:
                connection.commit()
            else:
                connection.rollback()
            return result
        except (ProgressSQLiteError, ConcurrentSQLiteRevisionUpdate):
            connection.rollback()
            raise
        except sqlite3.Error as exc:
            connection.rollback()
            raise ProgressSQLiteError(f"SQLite progress transaction failed: {exc}") from exc
        finally:
            connection.close()

    def persist_scoped_changes(
        self,
        *,
        expected_domain_revisions: Mapping[str, int],
        increment_domains: Iterable[str] | None = None,
        entity_patches: Mapping[
            str, Mapping[str, Mapping[str, Any]]
        ] | None = None,
        top_level_patches: Mapping[str, Mapping[str, Any]] | None = None,
        apply: bool,
    ) -> SQLiteCommitResult:
        """Merge explicitly addressed JSON facets under domain-specific CAS.

        Entity and top-level patch maps use RFC 6901 JSON pointers. The empty
        pointer replaces the complete payload and is the only way to create a
        new entity. Nested patches reload and merge the latest row inside the
        write transaction, preventing a non-overlapping domain write from being
        overwritten by a stale full-document materialization. By default every
        guarded domain is incremented. ``increment_domains`` may name a
        nonempty subset when an operation must guard additional input domains
        without claiming to mutate them.
        """

        if self.read_only:
            raise ProgressSQLiteError(
                "cannot persist through a read-only SQLite progress store"
            )
        if not isinstance(expected_domain_revisions, Mapping) or not expected_domain_revisions:
            raise ProgressSQLiteError(
                "scoped progress changes require at least one expected domain revision"
            )
        unknown_domains = set(expected_domain_revisions) - set(REVISION_DOMAIN_COLUMNS)
        if unknown_domains:
            raise ProgressSQLiteError(
                "unknown progress revision domains: " + ", ".join(sorted(unknown_domains))
            )
        for domain, revision in expected_domain_revisions.items():
            if not isinstance(revision, int) or isinstance(revision, bool) or revision < 0:
                raise ProgressSQLiteError(
                    f"expected {domain} revision must be a non-negative integer"
                )
        guarded_domains = set(expected_domain_revisions)
        if increment_domains is None:
            owning_domains = guarded_domains
        else:
            if isinstance(increment_domains, (str, bytes)):
                raise ProgressSQLiteError(
                    "increment_domains must be a collection of revision-domain names"
                )
            owning_domains = set(increment_domains)
            if not owning_domains:
                raise ProgressSQLiteError("increment_domains must not be empty")
            unknown_increment_domains = owning_domains - set(REVISION_DOMAIN_COLUMNS)
            if unknown_increment_domains:
                raise ProgressSQLiteError(
                    "unknown increment revision domains: "
                    + ", ".join(sorted(unknown_increment_domains))
                )
            unguarded_increment_domains = owning_domains - guarded_domains
            if unguarded_increment_domains:
                raise ProgressSQLiteError(
                    "increment domains must also be guarded by expected revisions: "
                    + ", ".join(sorted(unguarded_increment_domains))
                )

        entity_patches = entity_patches or {}
        top_level_patches = top_level_patches or {}
        unknown_collections = set(entity_patches) - set(ENTITY_COLLECTIONS)
        if unknown_collections:
            raise ProgressSQLiteError(
                "unknown entity collections: " + ", ".join(sorted(unknown_collections))
            )
        unknown_top = set(top_level_patches) - set(SCALAR_TOP_LEVEL_KEYS)
        if unknown_top:
            raise ProgressSQLiteError(
                "unsupported top-level patches: " + ", ".join(sorted(unknown_top))
            )
        for collection, rows in entity_patches.items():
            if not isinstance(rows, Mapping):
                raise ProgressSQLiteError(f"{collection} entity patches must be an object")
            for entity_id, patches in rows.items():
                if not isinstance(entity_id, str) or not entity_id:
                    raise ProgressSQLiteError("entity ids must be non-empty strings")
                _validated_pointer_patches(patches)
        for patches in top_level_patches.values():
            _validated_pointer_patches(patches)

        connection = self._connect()
        try:
            connection.execute("BEGIN IMMEDIATE")
            observed_vector = self._read_revision_vector(connection)
            observed_by_domain = {
                REVISION_DOMAIN_SEMANTIC: observed_vector.semantic_revision,
                REVISION_DOMAIN_EVIDENCE_GENERATION: (
                    observed_vector.evidence_generation_revision
                ),
                REVISION_DOMAIN_SCHEDULER: observed_vector.scheduler_revision,
            }
            for domain, expected in expected_domain_revisions.items():
                observed = observed_by_domain[domain]
                if observed != expected:
                    raise ConcurrentSQLiteRevisionUpdate(
                        f"{domain} revision changed: expected {expected}, found {observed}"
                    )

            # A scoped write is an intentional mutation boundary, so the v1
            # storage upgrade may occur atomically with it. Merely opening or
            # reading a legacy database remains non-mutating.
            self._migrate_revision_domains_in_transaction(connection)

            encoded_upserts: dict[tuple[str, str], str] = {}
            for collection, rows in entity_patches.items():
                for entity_id, patches in rows.items():
                    row = connection.execute(
                        "SELECT payload FROM entities WHERE collection=? AND entity_id=?",
                        (collection, entity_id),
                    ).fetchone()
                    current = None if row is None else json.loads(str(row[0]))
                    if row is None and "" not in patches:
                        raise ProgressSQLiteError(
                            f"{collection}/{entity_id}: a new entity requires root pointer ''"
                        )
                    updated = _apply_pointer_patches(
                        current, patches, context=f"{collection}/{entity_id}"
                    )
                    if not isinstance(updated, Mapping):
                        raise ProgressSQLiteError(
                            f"{collection}/{entity_id}: entity payload must remain an object"
                        )
                    encoded_upserts[(collection, entity_id)] = _json(updated)

            encoded_top_updates: dict[str, str] = {}
            for key, patches in top_level_patches.items():
                row = connection.execute(
                    "SELECT payload FROM top_level_values WHERE key=?", (key,)
                ).fetchone()
                if row is None:
                    raise ProgressSQLiteError(
                        f"missing authoritative top-level progress field {key}"
                    )
                updated = _apply_pointer_patches(
                    json.loads(str(row[0])), patches, context=f"top-level/{key}"
                )
                encoded_top_updates[key] = _json(updated)

            for collection, entity_id in encoded_upserts:
                connection.execute(
                    "DELETE FROM relationship_index "
                    "WHERE source_collection=? AND source_entity_id=?",
                    (collection, entity_id),
                )
            for (collection, entity_id), payload in encoded_upserts.items():
                connection.execute(
                    "INSERT INTO entities(collection, entity_id, payload) VALUES (?, ?, ?) "
                    "ON CONFLICT(collection, entity_id) DO UPDATE SET payload=excluded.payload",
                    (collection, entity_id, payload),
                )
            for key, payload in encoded_top_updates.items():
                connection.execute(
                    "UPDATE top_level_values SET payload=? WHERE key=?", (payload, key)
                )

            locations = self._entity_locations(connection)
            for (collection, entity_id), payload in encoded_upserts.items():
                self._rebuild_entity_indexes(
                    connection, collection, entity_id, json.loads(payload), locations
                )

            next_by_domain = dict(observed_by_domain)
            for domain in owning_domains:
                next_by_domain[domain] += 1
            result_vector = SQLiteRevisionVector(
                transaction_revision=observed_vector.transaction_revision + 1,
                semantic_revision=next_by_domain[REVISION_DOMAIN_SEMANTIC],
                evidence_generation_revision=next_by_domain[
                    REVISION_DOMAIN_EVIDENCE_GENERATION
                ],
                scheduler_revision=next_by_domain[REVISION_DOMAIN_SCHEDULER],
            )
            cursor = connection.execute(
                "UPDATE metadata SET revision=?, semantic_revision=?, "
                "evidence_generation_revision=?, scheduler_revision=? "
                "WHERE singleton=1 AND revision=?",
                (
                    result_vector.transaction_revision,
                    result_vector.semantic_revision,
                    result_vector.evidence_generation_revision,
                    result_vector.scheduler_revision,
                    observed_vector.transaction_revision,
                ),
            )
            if cursor.rowcount != 1:
                raise ConcurrentSQLiteRevisionUpdate(
                    "transaction revision changed during scoped progress update"
                )
            violations = tuple(
                tuple(row) for row in connection.execute("PRAGMA foreign_key_check")
            )
            if violations:
                raise ProgressSQLiteError(
                    f"foreign-key violations after proposed scoped commit: {violations!r}"
                )
            result = SQLiteCommitResult(
                applied=apply,
                path=self.path,
                previous_revision=observed_vector.transaction_revision,
                revision=result_vector.transaction_revision,
                upserted_entities=len(encoded_upserts),
                deleted_entities=0,
                updated_top_level_values=len(encoded_top_updates),
                previous_revision_vector=observed_vector,
                revision_vector=result_vector,
            )
            if apply:
                connection.commit()
            else:
                connection.rollback()
            return result
        except (ProgressSQLiteError, ConcurrentSQLiteRevisionUpdate):
            connection.rollback()
            raise
        except sqlite3.Error as exc:
            connection.rollback()
            raise ProgressSQLiteError(
                f"SQLite scoped progress transaction failed: {exc}"
            ) from exc
        finally:
            connection.close()

    def validate_integrity(self) -> SQLiteValidation:
        errors: list[str] = []
        with closing(self._connect()) as connection:
            try:
                self._validate_header(connection)
            except ProgressSQLiteError as exc:
                errors.append(str(exc))
            integrity = tuple(str(row[0]) for row in connection.execute("PRAGMA integrity_check"))
            violations = tuple(tuple(row) for row in connection.execute("PRAGMA foreign_key_check"))
            if integrity != ("ok",):
                errors.append("integrity_check did not return ok")
            if violations:
                errors.append("foreign_key_check reported violations")
            expected_addresses: set[tuple[Any, ...]] = set()
            expected_relationships: set[tuple[Any, ...]] = set()
            locations = self._entity_locations(connection)
            for row in connection.execute("SELECT collection, entity_id, payload FROM entities"):
                collection, entity_id = str(row[0]), str(row[1])
                payload = json.loads(str(row[2]))
                expected_addresses.update(
                    (collection, entity_id, pointer, address, kind)
                    for pointer, address, kind in _address_rows(entity_id, payload)
                )
                expected_relationships.update(
                    (collection, entity_id, pointer, target_collection, target_id)
                    for pointer, target_collection, target_id in _relationship_rows(payload, locations)
                )
            actual_addresses = {
                tuple(row)
                for row in connection.execute(
                    "SELECT collection, entity_id, json_pointer, address, address_kind FROM address_index"
                )
            }
            actual_relationships = {
                tuple(row)
                for row in connection.execute(
                    "SELECT source_collection, source_entity_id, json_pointer, "
                    "target_collection, target_entity_id FROM relationship_index"
                )
            }
            if actual_addresses != expected_addresses:
                errors.append("address_index does not match authoritative entity payloads")
            if actual_relationships != expected_relationships:
                errors.append("relationship_index does not match authoritative entity payloads")
            facet_count = int(connection.execute("SELECT count(*) FROM entity_facets").fetchone()[0])
            entity_count = int(connection.execute("SELECT count(*) FROM entities").fetchone()[0])
            if facet_count != entity_count:
                errors.append("entity_facets does not contain exactly one row per entity")
        return SQLiteValidation(not errors, integrity, violations, tuple(errors))


def import_legacy_json(
    json_path: str | Path,
    sqlite_path: str | Path,
    *,
    cutover_pair_id: str,
    overwrite: bool = False,
) -> ProgressSQLiteStore:
    source_path = Path(json_path)
    try:
        data = json.loads(source_path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise ProgressSQLiteError(f"{source_path}: unreadable legacy progress JSON: {exc}") from exc
    return ProgressSQLiteStore.create_from_mapping(
        sqlite_path, data, cutover_pair_id=cutover_pair_id, overwrite=overwrite
    )


def read_progress_metadata(path: str | Path) -> SQLiteMetadata:
    """Read the small fail-closed metadata row without materializing entities."""

    return ProgressSQLiteStore(path, read_only=True).metadata()


def read_progress_revision_vector(path: str | Path) -> SQLiteRevisionVector:
    """Read independent progress concurrency coordinates without entity scans."""

    return ProgressSQLiteStore(path, read_only=True).read_revision_vector()
