from __future__ import annotations

from contextlib import contextmanager
from copy import deepcopy
from dataclasses import dataclass
import json
import os
from pathlib import Path
import re
import time
from typing import Any, Callable, Iterator, Mapping


TRACKER_SCHEMA_VERSION = 5
ISSUE_LEDGER_VERSION = 2

REVISION_ENTITY_RE = re.compile(
    r"^recoil:(?P<kind>[a-z][a-z0-9-]*):r(?P<revision>[0-9]+):(?P<ordinal>[0-9]+)$"
)


class LiveProgressError(RuntimeError):
    pass


class ConcurrentRevisionUpdate(LiveProgressError):
    pass


def canonical_json_bytes(value: Any) -> bytes:
    return (
        json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"))
        + "\n"
    ).encode("utf-8")


def revision_entity_id(kind: str, revision: int, ordinal: int) -> str:
    normalized_kind = str(kind).strip().lower().replace("_", "-")
    if not re.fullmatch(r"[a-z][a-z0-9-]*", normalized_kind):
        raise LiveProgressError(f"invalid revision entity kind {kind!r}")
    if revision < 0 or ordinal < 1:
        raise LiveProgressError("revision entity ids require revision >= 0 and ordinal >= 1")
    return f"recoil:{normalized_kind}:r{revision}:{ordinal:06d}"


def allocate_revision_entity_id(
    data: Mapping[str, Any],
    *,
    kind: str,
    revision: int,
    collection: str,
) -> str:
    rows = data.get(collection, {})
    if not isinstance(rows, Mapping):
        raise LiveProgressError(f"{collection} must be an object")
    used: set[int] = set()
    for entity_id in rows:
        match = REVISION_ENTITY_RE.fullmatch(str(entity_id))
        if (
            match is not None
            and match.group("kind") == kind.replace("_", "-")
            and int(match.group("revision")) == revision
        ):
            used.add(int(match.group("ordinal")))
    ordinal = 1
    while ordinal in used:
        ordinal += 1
    return revision_entity_id(kind, revision, ordinal)


@dataclass(frozen=True)
class RevisionCommitResult:
    applied: bool
    path: Path
    previous_revision: int
    revision: int

    def to_dict(self) -> dict[str, Any]:
        return {
            "applied": self.applied,
            "path": self.path.as_posix(),
            "previous_revision": self.previous_revision,
            "revision": self.revision,
        }


@contextmanager
def revision_lock(path: Path, timeout: float = 10.0) -> Iterator[None]:
    lock_path = path.with_name(path.name + ".revision.lock")
    deadline = time.monotonic() + timeout
    descriptor: int | None = None
    while descriptor is None:
        try:
            descriptor = os.open(lock_path, os.O_CREAT | os.O_EXCL | os.O_WRONLY)
        except FileExistsError:
            if time.monotonic() >= deadline:
                raise ConcurrentRevisionUpdate(f"timed out waiting for revision lock {lock_path}")
            time.sleep(0.05)
    try:
        os.write(descriptor, f"pid={os.getpid()}\n".encode("ascii"))
        os.close(descriptor)
        descriptor = None
        yield
    finally:
        if descriptor is not None:
            os.close(descriptor)
        try:
            lock_path.unlink()
        except FileNotFoundError:
            pass


def atomic_replace(path: Path, payload: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(f".{path.name}.{os.getpid()}.tmp")
    try:
        with temporary.open("xb") as stream:
            stream.write(payload)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
    finally:
        if temporary.exists():
            temporary.unlink()


class RevisionStore:
    """A revision-only JSON store.

    The lock and expected revision prevent concurrent writes.  No content
    content summary is produced or accepted, and dry-run/apply return the same
    proposed revision when the expected revision remains current.
    """

    def __init__(
        self,
        path: str | Path,
        *,
        schema_field: str,
        schema_version: int,
        validator: Callable[[Mapping[str, Any]], None] | None = None,
        initializer: Callable[[], Mapping[str, Any]] | None = None,
    ) -> None:
        self.path = Path(path)
        self.schema_field = schema_field
        self.schema_version = schema_version
        self.validator = validator
        self.initializer = initializer

    def load(self) -> dict[str, Any]:
        if not self.path.exists() and self.initializer is not None:
            value = deepcopy(dict(self.initializer()))
            if self.validator is not None:
                self.validator(value)
            return value
        try:
            value = json.loads(self.path.read_text(encoding="utf-8"))
        except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
            raise LiveProgressError(f"{self.path}: unreadable JSON store: {exc}") from exc
        if not isinstance(value, dict):
            raise LiveProgressError(f"{self.path}: JSON store must be an object")
        if value.get(self.schema_field) != self.schema_version:
            raise LiveProgressError(
                f"{self.path}: {self.schema_field} must be {self.schema_version}; "
                "run the live-validation migration first"
            )
        revision = value.get("revision")
        if not isinstance(revision, int) or isinstance(revision, bool) or revision < 0:
            raise LiveProgressError(f"{self.path}: revision must be a non-negative integer")
        if self.validator is not None:
            self.validator(value)
        return value

    def commit(
        self,
        proposed: Mapping[str, Any],
        *,
        expected_revision: int,
        apply: bool,
    ) -> RevisionCommitResult:
        candidate = deepcopy(dict(proposed))
        candidate[self.schema_field] = self.schema_version
        candidate["revision"] = expected_revision + 1
        if self.validator is not None:
            self.validator(candidate)
        with revision_lock(self.path):
            current = self.load()
            observed_revision = int(current["revision"])
            if observed_revision != expected_revision:
                raise ConcurrentRevisionUpdate(
                    f"revision changed: expected {expected_revision}, found {observed_revision}"
                )
            result = RevisionCommitResult(
                applied=apply,
                path=self.path,
                previous_revision=observed_revision,
                revision=expected_revision + 1,
            )
            if apply:
                atomic_replace(self.path, canonical_json_bytes(candidate))
            return result

    def mutate(
        self,
        transform: Callable[[dict[str, Any]], None],
        *,
        expected_revision: int,
        apply: bool,
    ) -> RevisionCommitResult:
        current = self.load()
        if current["revision"] != expected_revision:
            raise ConcurrentRevisionUpdate(
                f"revision changed: expected {expected_revision}, found {current['revision']}"
            )
        proposed = deepcopy(current)
        transform(proposed)
        return self.commit(proposed, expected_revision=expected_revision, apply=apply)


def validate_tracker_v5(data: Mapping[str, Any]) -> None:
    if data.get("schema_version") != TRACKER_SCHEMA_VERSION:
        raise LiveProgressError("tracker schema_version must be 5")
    evidence = data.get("evidence")
    if not isinstance(evidence, Mapping):
        raise LiveProgressError("tracker evidence must be an object")
    for evidence_id, record in evidence.items():
        match = REVISION_ENTITY_RE.fullmatch(str(evidence_id))
        if match is None or match.group("kind") != "evidence":
            raise LiveProgressError(f"invalid revision-scoped evidence id {evidence_id!r}")
        if not isinstance(record, Mapping):
            raise LiveProgressError(f"evidence {evidence_id} must be an object")
        if record.get("freshness") == "current" and record.get("validation_mode") != "live":
            raise LiveProgressError(
                f"evidence {evidence_id}: current evidence requires validation_mode=live"
            )


def validate_issue_ledger_v2(data: Mapping[str, Any]) -> None:
    if data.get("version") != ISSUE_LEDGER_VERSION:
        raise LiveProgressError("workspace issue ledger version must be 2")
    for field in ("issues", "work_packets", "reservations"):
        if not isinstance(data.get(field), list):
            raise LiveProgressError(f"workspace issue ledger {field} must be a list")
    for packet in data.get("work_packets", []):
        if not isinstance(packet, Mapping):
            raise LiveProgressError("workspace issue work packet must be an object")
        if packet.get("semantic_contract_version") != 1:
            raise LiveProgressError(
                f"workspace issue packet {packet.get('id')!r} lacks semantic contract version 1"
            )
