from __future__ import annotations

from dataclasses import dataclass
import gc
import json
import os
import re
from pathlib import Path
import time
from typing import Any

from _recoil.lib.tooling import REPO_ROOT
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH, ProgressStore


SCHEMA_VERSION = 4
DEFAULT_OWNER_LEDGER = DEFAULT_PROGRESS_PATH
OWNER_LEDGER_REPLACE_RETRY_DELAYS = (0.025, 0.05, 0.1, 0.2, 0.4, 0.8, 1.0, 1.0, 1.0)

VALID_OWNER_KINDS = {
    "class",
    "interface",
    "record",
    "source-file",
    "subsystem",
    "callback-table",
    "data-owner",
    "provider-boundary",
    "standalone",
}
VALID_OWNER_STATES = {"discovered", "mapped", "active", "accepted", "blocked", "deferred"}
VALID_OWNER_BINARIES = {"recoil", "messages"}
OWNER_GATES = ("boundary", "source", "data", "functional", "linkage", "byte")
VALID_GATE_STATES = {"pending", "accepted", "blocked", "none", "deferred"}
OWNER_REIMPLEMENTATION_TIERS = ("X", "C", "B", "A", "S")
VALID_OWNER_REIMPLEMENTATION_TIERS = set(OWNER_REIMPLEMENTATION_TIERS)
RELATIONSHIP_KINDS = (
    "anchor-address",
    "primary-function",
    "primary-data",
    "depends-on-owner",
)
VALID_RELATIONSHIP_KINDS = set(RELATIONSHIP_KINDS)
RELATIONSHIP_REASONS = (
    "source-dependency",
    "data-dependency",
    "provider-boundary",
    "layout-contract",
    "table-dispatch",
    "owner-split",
    "legacy-owner-dependency",
    "manual",
)
VALID_RELATIONSHIP_REASONS = set(RELATIONSHIP_REASONS)
ADDRESS_RE = re.compile(r"^0x[0-9a-f]+$")


def normalize_owner_address(value: str) -> str:
    text = value.strip().lower()
    if not ADDRESS_RE.match(text):
        raise ValueError(f"invalid address {value!r}; expected 0x-prefixed hex")
    return text


def normalize_owner_id(value: str) -> str:
    text = value.strip()
    if not text or any(char.isspace() for char in text):
        raise ValueError(f"invalid owner id {value!r}; expected non-empty id without whitespace")
    return text


def default_gates() -> dict[str, str]:
    return {gate: "pending" for gate in OWNER_GATES}


def default_entry_reimplementation(kind: str) -> dict[str, str]:
    if kind not in {"function", "data"}:
        raise ValueError(f"invalid entry reimplementation kind {kind!r}")
    return {"kind": kind, "tier": "X", "evidence": "pending: newly linked primary entry"}


def normalize_owner_binary(value: str) -> str:
    text = value.strip().lower()
    if text not in VALID_OWNER_BINARIES:
        raise ValueError(
            f"invalid owner binary {value!r}; expected one of {', '.join(sorted(VALID_OWNER_BINARIES))}"
        )
    return text


def normalize_owner_reimplementation_tier(value: str) -> str:
    text = value.strip().upper()
    if text not in VALID_OWNER_REIMPLEMENTATION_TIERS:
        raise ValueError(
            f"invalid owner reimplementation tier {value!r}; "
            f"expected one of {', '.join(OWNER_REIMPLEMENTATION_TIERS)}"
        )
    return text


@dataclass(frozen=True)
class SourceOwnerRelationship:
    owner_id: str
    kind: str
    address: str = ""
    name: str = ""
    target_owner_id: str = ""
    reason: str = ""

    def as_payload(self) -> dict[str, Any]:
        payload: dict[str, Any] = {"kind": self.kind}
        if self.address:
            payload["address"] = self.address
        if self.name:
            payload["name"] = self.name
        if self.target_owner_id:
            payload["target_owner_id"] = self.target_owner_id
        if self.reason:
            payload["reason"] = self.reason
        return payload


def _normalize_relationship_payload(
    owner_id: str,
    raw: Any,
    *,
    source: str,
) -> SourceOwnerRelationship:
    if not isinstance(raw, dict):
        raise ValueError(f"{source}: relationship entries must be objects")
    kind = str(raw.get("kind", "") or "").strip()
    if kind not in VALID_RELATIONSHIP_KINDS:
        raise ValueError(
            f"{source}: invalid relationship kind {kind!r}; "
            f"expected one of {', '.join(RELATIONSHIP_KINDS)}"
        )
    if kind in {"anchor-address", "primary-function", "primary-data"}:
        address = normalize_owner_address(str(raw.get("address", "") or ""))
        name = str(raw.get("name", "") or "").strip()
        if kind == "primary-data" and not name:
            raise ValueError(f"{source}: primary-data relationships need name")
        return SourceOwnerRelationship(owner_id=owner_id, kind=kind, address=address, name=name)
    target_owner_id = normalize_owner_id(str(raw.get("target_owner_id", "") or ""))
    reason = str(raw.get("reason", "") or "manual").strip() or "manual"
    if reason not in VALID_RELATIONSHIP_REASONS:
        raise ValueError(
            f"{source}: invalid relationship reason {reason!r}; "
            f"expected one of {', '.join(RELATIONSHIP_REASONS)}"
        )
    return SourceOwnerRelationship(
        owner_id=owner_id,
        kind=kind,
        target_owner_id=target_owner_id,
        reason=reason,
    )


def relationship_payloads_for_owner(owner: "SourceOwner") -> list[dict[str, Any]]:
    return [relationship.as_payload() for relationship in owner_relationships(owner)]


def owner_relationships(owner: "SourceOwner") -> list[SourceOwnerRelationship]:
    raw_relationships = owner.data.get("relationships")
    if not isinstance(raw_relationships, list):
        raise ValueError(f"{owner.id}: schema_version 4 requires relationships to be a list")
    return [
        _normalize_relationship_payload(
            owner.id,
            raw,
            source=f"{owner.id}: relationships[{index}]",
        )
        for index, raw in enumerate(raw_relationships)
    ]


def relationships_by_kind(owner: "SourceOwner", kind: str) -> list[SourceOwnerRelationship]:
    return [relationship for relationship in owner_relationships(owner) if relationship.kind == kind]


def owner_anchor_addresses(owner: "SourceOwner") -> set[str]:
    return {relationship.address for relationship in relationships_by_kind(owner, "anchor-address")}


def owner_member_addresses(owner: "SourceOwner") -> set[str]:
    return {relationship.address for relationship in relationships_by_kind(owner, "primary-function")}


def owner_data_address_records(owner: "SourceOwner") -> list[SourceOwnerRelationship]:
    return relationships_by_kind(owner, "primary-data")


def owner_data_addresses(owner: "SourceOwner") -> set[str]:
    return {relationship.address for relationship in owner_data_address_records(owner)}


def owner_dependency_relationships(owner: "SourceOwner") -> list[SourceOwnerRelationship]:
    return relationships_by_kind(owner, "depends-on-owner")


def owner_dependency_ids(owner: "SourceOwner") -> list[str]:
    return [relationship.target_owner_id for relationship in owner_dependency_relationships(owner)]


def primary_owners_for_entry(doc: "SourceOwnerDocument", entry) -> list["SourceOwner"]:
    """Return ledger owners that carry the canonical primary link for an entry."""
    wanted = entry.address.lower()
    if entry.is_data_entry:
        return [owner for owner in doc.owners if wanted in owner_data_addresses(owner)]
    owners: list[SourceOwner] = []
    for owner in doc.owners:
        if wanted in owner_member_addresses(owner):
            owners.append(owner)
            continue
        if _owner_has_function_shaped_static_data_primary(owner, entry):
            owners.append(owner)
    return owners


def _owner_has_function_shaped_static_data_primary(owner: "SourceOwner", entry) -> bool:
    """Match the membership audit's narrow static-data-as-function-row exception."""
    if entry.is_data_entry or entry.is_retired_data_entry or entry.is_provider_boundary:
        return False
    expected_name = entry.reconstructed_name.strip()
    if not expected_name:
        return False
    wanted = entry.address.lower()
    return any(
        relationship.address == wanted and relationship.name.strip() == expected_name
        for relationship in owner_data_address_records(owner)
    )


def _is_retryable_replace_permission_error(exc: PermissionError) -> bool:
    winerror = getattr(exc, "winerror", None)
    return winerror == 5 or (os.name == "nt" and winerror is None)


@dataclass(frozen=True)
class SourceOwner:
    data: dict[str, Any]

    @property
    def id(self) -> str:
        return str(self.data.get("id", ""))

    @property
    def kind(self) -> str:
        return str(self.data.get("kind", ""))

    @property
    def state(self) -> str:
        return str(self.data.get("state", ""))

    @property
    def gates(self) -> dict[str, str]:
        gates = self.data.get("gates", {})
        return gates if isinstance(gates, dict) else {}

    def addresses(self) -> set[str]:
        result: set[str] = set()
        for relationship in owner_relationships(self):
            if relationship.address:
                result.add(relationship.address)
        return result

    def gate(self, name: str) -> str:
        return self.gates.get(name, "pending")

    def owner_scope_ready(self) -> bool:
        return self.kind == "standalone" or (
            self.gate("boundary") == "accepted" and self.gate("source") == "accepted"
        )

    def data_ready(self) -> bool:
        return self.gate("data") == "accepted"

    def linkage_ready(self) -> bool:
        return self.gate("linkage") in {"accepted", "none"}

    def byte_ready(self) -> bool:
        return self.gate("byte") == "accepted"

    def no_data(self) -> bool:
        return self.kind == "standalone" or self.gate("data") == "none"

    @property
    def binary(self) -> str:
        return str(self.data.get("binary", ""))

    @property
    def reimplementation(self) -> dict[str, Any]:
        raw = self.data.get("reimplementation", {})
        return raw if isinstance(raw, dict) else {}

    @property
    def entry_reimplementations(self) -> dict[str, dict[str, Any]]:
        raw = self.reimplementation.get("entries", {})
        if not isinstance(raw, dict):
            return {}
        return {str(address): value for address, value in raw.items() if isinstance(value, dict)}

    def entry_reimplementation(self, address: str) -> dict[str, Any] | None:
        return self.entry_reimplementations.get(normalize_owner_address(address))

    def entry_reimplementation_tier(self, address: str) -> str:
        record = self.entry_reimplementation(address)
        if record is None:
            return "X"
        return normalize_owner_reimplementation_tier(str(record.get("tier", "X") or "X"))

    @property
    def reimplementation_tier(self) -> str:
        if self.kind == "provider-boundary":
            return "provider-boundary"
        return derive_owner_reimplementation_tier(self)


def derive_owner_reimplementation_tier(owner: SourceOwner) -> str:
    """Derive an authored owner tier from primary-entry tiers and owner gates."""
    if owner.kind == "provider-boundary":
        return "provider-boundary"
    primary_addresses = sorted(owner_member_addresses(owner) | owner_data_addresses(owner))
    if not primary_addresses:
        return "X"
    entries = owner.entry_reimplementations
    if any(address not in entries for address in primary_addresses):
        return "X"
    tiers = [normalize_owner_reimplementation_tier(str(entries[address].get("tier", "X") or "X")) for address in primary_addresses]
    order = {tier: index for index, tier in enumerate(OWNER_REIMPLEMENTATION_TIERS)}
    floor = min(tiers, key=order.__getitem__)
    if floor == "X":
        return "X"
    b_gates_ready = (
        owner.gate("boundary") == "accepted"
        and owner.gate("source") == "accepted"
        and owner.gate("data") in {"accepted", "none"}
        and owner.gate("linkage") in {"accepted", "none"}
    )
    # B/A/S are independent higher-order gate decisions. A source-faithful
    # owner can legitimately reach B before a dedicated behavior smoke.
    if floor == "S" and b_gates_ready and owner.gate("byte") == "accepted":
        return "S"
    if floor in {"A", "S"} and b_gates_ready:
        return "A"
    if floor in {"B", "A", "S"} and b_gates_ready:
        return "B"
    # C deliberately precedes the structural/data/linkage source-owner gates.
    # Data owners may carry one accessor/initializer function while behavior is
    # certified through their data gate rather than a standalone functional gate.
    has_functions = bool(owner_member_addresses(owner)) and owner.kind != "data-owner"
    if has_functions and owner.gate("functional") != "accepted":
        return "X"
    return "C"


def _project_progress_owners(progress: dict[str, Any]) -> dict[str, Any]:
    """Project unified tracker owners into the legacy read-only object model."""
    symbols = progress.get("symbols", {})
    evidence = progress.get("evidence", {})
    raw_owners = progress.get("owners", {})
    if not isinstance(symbols, dict) or not isinstance(evidence, dict) or not isinstance(raw_owners, dict):
        raise ValueError("unified progress owner projection requires object collections")

    legacy_ids = {
        owner_id: str(raw.get("legacy_id") or owner_id.split(":owner:", 1)[-1])
        for owner_id, raw in raw_owners.items()
        if isinstance(raw, dict)
    }

    def summaries(ids: Any) -> list[str]:
        result: list[str] = []
        for evidence_id in ids if isinstance(ids, list) else []:
            record = evidence.get(evidence_id, {})
            summary = record.get("summary") if isinstance(record, dict) else None
            if isinstance(summary, str) and summary.strip():
                result.append(summary)
        return result

    owners: list[dict[str, Any]] = []
    for owner_id, raw in raw_owners.items():
        if not isinstance(raw, dict):
            continue
        legacy_id = legacy_ids[owner_id]
        relationships: list[dict[str, Any]] = []
        anchors: list[str] = []
        members: list[str] = []
        data_addresses: list[dict[str, str]] = []
        dependencies: list[str] = []
        for relationship in raw.get("relationships", []):
            if not isinstance(relationship, dict):
                continue
            kind = str(relationship.get("kind", ""))
            converted: dict[str, Any] = {"kind": kind}
            if kind in {"anchor-address", "primary-function", "primary-data"}:
                address = str(relationship.get("address", ""))
                converted["address"] = address
                symbol = symbols.get(str(relationship.get("symbol_id", "")), {})
                symbol_name = (
                    str(symbol.get("navigation_name", ""))
                    if isinstance(symbol, dict)
                    else ""
                )
                relationship_name = str(
                    relationship.get("name", "")
                    or symbol_name
                    or ("pending" if kind == "primary-data" else "")
                )
                if relationship_name:
                    converted["name"] = relationship_name
                if kind == "anchor-address":
                    anchors.append(address)
                elif kind == "primary-function":
                    members.append(address)
                else:
                    data_addresses.append({"address": address, "name": relationship_name})
            elif kind == "depends-on-owner":
                target_id = str(relationship.get("target_owner_id", ""))
                target = legacy_ids.get(target_id, target_id)
                converted["target_owner_id"] = target
                converted["reason"] = str(relationship.get("reason", "manual") or "manual")
                dependencies.append(target)
            relationships.append(converted)

        gates = dict(raw.get("gates", {})) if isinstance(raw.get("gates"), dict) else {}
        gates["linkage"] = gates.pop("owner_linkage", gates.get("linkage", "pending"))
        owner: dict[str, Any] = {
            "id": legacy_id,
            "binary": str(raw.get("binary", "recoil")),
            "kind": str(raw.get("kind", "")),
            "name": str(raw.get("name", "")),
            "state": str(raw.get("lifecycle_state", "discovered")),
            "section": str(raw.get("section", "")),
            "source_paths": list(raw.get("source_paths", [])),
            "anchors": sorted(set(anchors)),
            "member_addresses": sorted(set(members)),
            "data_addresses": sorted(data_addresses, key=lambda item: (item["address"], item["name"])),
            "dependencies": sorted(set(dependencies)),
            "relationships": relationships,
            "gates": gates,
            "blocker": str(raw.get("blocker", "")),
            "address_metadata": dict(raw.get("address_metadata", {})),
            "evidence": summaries(raw.get("evidence_ids", [])),
        }
        if raw.get("kind") != "provider-boundary":
            entries: dict[str, dict[str, Any]] = {}
            reimplementation = raw.get("reimplementation", {})
            raw_entries = reimplementation.get("entries", {}) if isinstance(reimplementation, dict) else {}
            if isinstance(raw_entries, dict):
                for symbol_id, entry in raw_entries.items():
                    symbol = symbols.get(symbol_id, {})
                    if not isinstance(symbol, dict) or not isinstance(entry, dict):
                        continue
                    address = str(symbol.get("address", ""))
                    entry_summaries = summaries(entry.get("evidence_ids", []))
                    entries[address] = {
                        "kind": str(entry.get("kind", "function")),
                        "tier": str(entry.get("tier", "X")),
                        "evidence": " | ".join(entry_summaries) or "migrated unified progress entry",
                    }
            owner["reimplementation"] = {"entries": entries}
        owners.append(owner)

    return {"schema_version": SCHEMA_VERSION, "owners": owners, "removed_owners": []}


class SourceOwnerDocument:
    def __init__(self, path: Path, payload: dict[str, Any], original_bytes: bytes | None = None) -> None:
        self.path = path
        self.payload = payload
        self._original_bytes = original_bytes

    @classmethod
    def from_progress_document(
        cls, document: object, *, path: Path = DEFAULT_OWNER_LEDGER
    ) -> "SourceOwnerDocument":
        data = getattr(document, "data", document)
        if not isinstance(data, dict):
            raise ValueError("progress owner projection requires a progress mapping")
        document_path = getattr(document, "path", None)
        return cls(document_path or path, _project_progress_owners(data))

    @classmethod
    def load(cls, path: Path = DEFAULT_OWNER_LEDGER) -> "SourceOwnerDocument":
        if path.suffix.lower() == ".sqlite3":
            document = ProgressStore(path).load()
            return cls.from_progress_document(document, path=path)
        if not path.exists():
            return cls(path, {"schema_version": SCHEMA_VERSION, "owners": []})
        original_bytes = path.read_bytes()
        payload = json.loads(original_bytes.decode("utf-8"))
        if not isinstance(payload, dict):
            raise ValueError(f"{path}: expected JSON object")
        if payload.get("schema_version") in {1, 2, 3, 4, 5} and isinstance(payload.get("owners"), dict):
            payload = _project_progress_owners(payload)
            # This is a read-only compatibility projection for validation
            # consumers.  Unified progress mutations go through progress CLI.
            original_bytes = None
        schema_version = payload.get("schema_version")
        if schema_version != SCHEMA_VERSION:
            raise ValueError(
                f"{path}: unsupported SOURCE_OWNERS schema_version {schema_version!r}; "
                f"schema_version {SCHEMA_VERSION} is required"
            )
        if not isinstance(payload.get("owners"), list):
            raise ValueError(f"{path}: owners must be a list")
        return cls(path, payload, original_bytes)

    def _lock_path(self) -> Path:
        return self.path.with_name(f"{self.path.name}.lock")

    def _acquire_save_lock(self) -> int:
        lock_path = self._lock_path()
        lock_path.parent.mkdir(parents=True, exist_ok=True)
        flags = os.O_CREAT | os.O_EXCL | os.O_WRONLY
        try:
            fd = os.open(lock_path, flags)
        except FileExistsError as exc:
            raise ValueError(
                f"{self.path}: owner ledger lock exists at {lock_path}; "
                "another write may be in progress, rerun the command"
            ) from exc
        os.write(fd, f"pid={os.getpid()} path={self.path}\n".encode("ascii", errors="replace"))
        return fd

    def _release_save_lock(self, fd: int) -> None:
        os.close(fd)
        try:
            self._lock_path().unlink()
        except FileNotFoundError:
            pass

    def _atomic_write_bytes(self, content: bytes) -> None:
        temp_path = self.path.with_name(
            f".{self.path.name}.{os.getpid()}.{time.monotonic_ns()}.tmp"
        )
        replace_error: PermissionError | None = None
        try:
            with temp_path.open("xb") as stream:
                stream.write(content)
                stream.flush()
                os.fsync(stream.fileno())
            self._replace_temp_with_retry(temp_path)
        except PermissionError as exc:
            replace_error = exc
            raise
        finally:
            try:
                temp_path.unlink()
            except FileNotFoundError:
                pass
            except PermissionError as cleanup_exc:
                if replace_error is None:
                    raise
                replace_error.add_note(
                    f"also failed to remove temporary owner ledger {temp_path}: {cleanup_exc}"
                )

    def _replace_temp_with_retry(self, temp_path: Path) -> None:
        last_exc: PermissionError | None = None
        for delay in OWNER_LEDGER_REPLACE_RETRY_DELAYS:
            try:
                os.replace(temp_path, self.path)
                return
            except PermissionError as exc:
                if not _is_retryable_replace_permission_error(exc):
                    raise
                last_exc = exc
                gc.collect()
                time.sleep(delay)
        try:
            os.replace(temp_path, self.path)
        except PermissionError as exc:
            if not _is_retryable_replace_permission_error(exc):
                raise
            last_exc = exc
        if last_exc is None:
            return
        winerror = getattr(last_exc, "winerror", None)
        raise PermissionError(
            f"{self.path}: failed to atomically replace owner ledger from temporary file "
            f"{temp_path} after {len(OWNER_LEDGER_REPLACE_RETRY_DELAYS) + 1} attempts; "
            f"last error: {last_exc}; winerror={winerror!r}. "
            "A transient Windows file lock may still be active; rerun the command."
        ) from last_exc

    @property
    def owners(self) -> list[SourceOwner]:
        owners = self.payload.get("owners", [])
        if not isinstance(owners, list):
            raise ValueError(f"{self.path}: expected owners list")
        return [SourceOwner(item) for item in owners if isinstance(item, dict)]

    def save(self) -> bool:
        self.payload["owners"] = sorted(
            self.payload.get("owners", []),
            key=lambda item: item.get("id", "") if isinstance(item, dict) else "",
        )
        content = (json.dumps(self.payload, indent=2, ensure_ascii=False) + "\n").encode("utf-8")
        fd = self._acquire_save_lock()
        try:
            current_bytes = self.path.read_bytes() if self.path.exists() else None
            if self._original_bytes is None:
                if current_bytes is not None and current_bytes != content:
                    raise ValueError(
                        f"{self.path}: owner ledger appeared on disk after load; "
                        "rerun the command to avoid overwriting concurrent updates"
                    )
            elif current_bytes != self._original_bytes:
                raise ValueError(
                    f"{self.path}: owner ledger changed on disk after load; "
                    "rerun the command to avoid overwriting concurrent updates"
                )
            if current_bytes == content:
                return False
            self._atomic_write_bytes(content)
            self._original_bytes = content
            return True
        finally:
            self._release_save_lock(fd)

    def owner(self, owner_id: str) -> SourceOwner:
        wanted = normalize_owner_id(owner_id)
        for owner in self.owners:
            if owner.id == wanted:
                return owner
        raise ValueError(f"source owner not found: {owner_id}")

    def owners_for_address(self, address: str) -> list[SourceOwner]:
        wanted = normalize_owner_address(address)
        return [owner for owner in self.owners if wanted in owner.addresses()]

    def uses_relationships(self, owner_id: str) -> bool:
        owner = self._mutable_owner(owner_id)
        return isinstance(owner.get("relationships"), list)

    def _relationship_payloads(self, owner_id: str) -> list[dict[str, Any]]:
        owner = self._mutable_owner(owner_id)
        relationships = owner.get("relationships")
        if isinstance(relationships, list):
            return relationships
        relationships = relationship_payloads_for_owner(SourceOwner(owner))
        owner["relationships"] = relationships
        return relationships

    def _upsert_relationship(self, owner_id: str, relationship: SourceOwnerRelationship) -> None:
        relationships = self._relationship_payloads(owner_id)
        for item in relationships:
            if not isinstance(item, dict) or item.get("kind") != relationship.kind:
                continue
            if relationship.address and item.get("address") == relationship.address:
                item.update(relationship.as_payload())
                return
            if relationship.target_owner_id and item.get("target_owner_id") == relationship.target_owner_id:
                item.update(relationship.as_payload())
                return
        relationships.append(relationship.as_payload())

    def _remove_relationship(self, owner_id: str, *, kind: str, address: str = "", target_owner_id: str = "") -> None:
        if not self.uses_relationships(owner_id):
            return
        owner = self._mutable_owner(owner_id)
        relationships = owner.get("relationships", [])
        if not isinstance(relationships, list):
            return
        owner["relationships"] = [
            item
            for item in relationships
            if not (
                isinstance(item, dict)
                and item.get("kind") == kind
                and (not address or item.get("address") == address)
                and (not target_owner_id or item.get("target_owner_id") == target_owner_id)
            )
        ]



    def set_owner_binary(self, owner_id: str, binary: str) -> None:
        owner = self._mutable_owner(owner_id)
        owner["binary"] = normalize_owner_binary(binary)


    def set_entry_reimplementation(
        self,
        owner_id: str,
        address: str,
        tier: str,
        evidence: str,
    ) -> tuple[str, str]:
        if self.payload.get("schema_version") != SCHEMA_VERSION:
            raise ValueError("owner set-entry-tier requires SOURCE_OWNERS schema_version 4")
        addr = normalize_owner_address(address)
        normalized_tier = normalize_owner_reimplementation_tier(tier)
        evidence_text = evidence.strip()
        if not evidence_text:
            raise ValueError("owner set-entry-tier requires --evidence")
        owner_data = self._mutable_owner(owner_id)
        owner = SourceOwner(owner_data)
        if owner.kind == "provider-boundary":
            raise ValueError(f"{owner.id}: provider-boundary owners do not carry authored entry tiers")
        function_addresses = owner_member_addresses(owner)
        data_addresses = owner_data_addresses(owner)
        if addr in function_addresses:
            kind = "function"
        elif addr in data_addresses:
            kind = "data"
        else:
            raise ValueError(f"{addr}: not a primary function/data entry of {owner.id}")
        if normalized_tier == "S":
            current = owner.entry_reimplementation_tier(addr)
            if current not in {"B", "A", "S"}:
                raise ValueError(
                    f"tier S entry acceptance for {addr} requires an accepted tier-B/A source evidence baseline first"
                )
        old_owner_tier = owner.reimplementation_tier
        reimplementation = owner_data.setdefault("reimplementation", {"entries": {}})
        entries = reimplementation.setdefault("entries", {})
        record: dict[str, Any] = {"kind": kind, "tier": normalized_tier, "evidence": evidence_text}
        entries[addr] = record
        return old_owner_tier, SourceOwner(owner_data).reimplementation_tier


    def find(self, query: str) -> list[SourceOwner]:
        needle = query.strip().lower()
        return [
            owner
            for owner in self.owners
            if needle in json.dumps(owner.data, ensure_ascii=False).lower()
        ]

    def add_owner(self, owner_data: dict[str, Any]) -> None:
        owner_data = dict(owner_data)
        owner_data["id"] = normalize_owner_id(str(owner_data.get("id", "")))
        if any(owner.id == owner_data["id"] for owner in self.owners):
            raise ValueError(f"source owner already exists: {owner_data['id']}")
        relationships: list[dict[str, Any]] = []
        relationships.extend(
            {"kind": "anchor-address", "address": normalize_owner_address(address)}
            for address in owner_data.get("anchors", [])
        )
        relationships.extend(
            {"kind": "primary-function", "address": normalize_owner_address(address)}
            for address in owner_data.get("member_addresses", [])
        )
        relationships.extend(
            {
                "kind": "primary-data",
                "address": normalize_owner_address(str(item.get("address", ""))),
                "name": str(item.get("name", "") or "pending"),
            }
            for item in owner_data.get("data_addresses", [])
            if isinstance(item, dict)
        )
        relationships.extend(
            {"kind": "depends-on-owner", "target_owner_id": dependency, "reason": "manual"}
            for dependency in owner_data.get("dependencies", [])
        )
        owner_data["relationships"] = relationships
        self._check_new_owner_primary_children(owner_data)
        owner_data["gates"] = {**default_gates(), **owner_data.get("gates", {})}
        owner_data["binary"] = normalize_owner_binary(str(owner_data.get("binary", "recoil") or "recoil"))
        if owner_data.get("kind") != "provider-boundary":
            entries: dict[str, Any] = {}
            source_owner = SourceOwner(owner_data)
            for address in owner_member_addresses(source_owner):
                entries[address] = default_entry_reimplementation("function")
            for address in owner_data_addresses(source_owner):
                entries[address] = default_entry_reimplementation("data")
            owner_data["reimplementation"] = {"entries": entries}
        self.payload.setdefault("owners", []).append(owner_data)
        findings = self.validate()
        if findings:
            raise ValueError("; ".join(findings))

    def remove_owner(self, owner_id: str, evidence: str) -> dict[str, Any]:
        wanted = normalize_owner_id(owner_id)
        evidence_text = evidence.strip()
        if not evidence_text:
            raise ValueError("owner remove requires --evidence")

        owner = self.owner(wanted)
        function_addresses = sorted(owner_member_addresses(owner))
        data_addresses = sorted(relationship.address for relationship in owner_data_address_records(owner))
        if function_addresses or data_addresses:
            details = []
            if function_addresses:
                details.append(f"functions={','.join(function_addresses)}")
            if data_addresses:
                details.append(f"data={','.join(data_addresses)}")
            raise ValueError(
                f"{wanted}: owner remove refuses owners with primary children; "
                f"{' '.join(details)}; unlink or move primary children first"
            )

        dependents = sorted(
            other.id
            for other in self.owners
            if other.id != wanted and wanted in owner_dependency_ids(other)
        )
        if dependents:
            raise ValueError(
                f"{wanted}: owner remove refuses owners referenced by dependencies: "
                f"{', '.join(dependents)}; update dependent owner links first"
            )

        owners = self.payload.setdefault("owners", [])
        self.payload["owners"] = [
            item
            for item in owners
            if not (isinstance(item, dict) and item.get("id") == wanted)
        ]
        removed_record = {
            "id": owner.id,
            "kind": owner.kind,
            "name": str(owner.data.get("name", "") or owner.id),
            "section": owner.data.get("section", "pending"),
            "anchors": sorted(owner_anchor_addresses(owner)),
            "source_paths": [
                item
                for item in owner.data.get("source_paths", [])
                if isinstance(item, str)
            ],
            "evidence": evidence_text,
        }
        removed_owners = self.payload.setdefault("removed_owners", [])
        if not isinstance(removed_owners, list):
            raise ValueError(f"{self.path}: removed_owners must be a list")
        removed_owners.append(removed_record)

        findings = self.validate()
        if findings:
            raise ValueError("; ".join(findings))
        return removed_record

    def _mutable_owner(self, owner_id: str) -> dict[str, Any]:
        wanted = normalize_owner_id(owner_id)
        owners = self.payload.setdefault("owners", [])
        for item in owners:
            if isinstance(item, dict) and item.get("id") == wanted:
                return item
        raise ValueError(f"source owner not found: {owner_id}")

    def _primary_function_owner(self, address: str) -> SourceOwner | None:
        addr = normalize_owner_address(address)
        for owner in self.owners:
            if addr in owner_member_addresses(owner):
                return owner
        return None

    def _primary_data_owner(self, address: str) -> SourceOwner | None:
        addr = normalize_owner_address(address)
        for owner in self.owners:
            if any(relationship.address == addr for relationship in owner_data_address_records(owner)):
                return owner
        return None

    def _reject_primary_function_conflict(self, owner_id: str, address: str) -> None:
        wanted_owner_id = normalize_owner_id(owner_id)
        addr = normalize_owner_address(address)
        existing_owner = self._primary_function_owner(addr)
        if existing_owner is not None and existing_owner.id != wanted_owner_id:
            raise ValueError(
                f"{addr}: primary function address is already owned by {existing_owner.id}; "
                "unlink/move first"
            )
        data_owner = self._primary_data_owner(addr)
        if data_owner is not None:
            raise ValueError(
                f"{addr}: address is already primary data of {data_owner.id}; "
                "an address cannot be primary function and primary data"
            )

    def _reject_primary_data_conflict(self, owner_id: str, address: str) -> None:
        wanted_owner_id = normalize_owner_id(owner_id)
        addr = normalize_owner_address(address)
        existing_owner = self._primary_data_owner(addr)
        if existing_owner is not None and existing_owner.id != wanted_owner_id:
            raise ValueError(
                f"{addr}: primary data address is already owned by {existing_owner.id}; "
                "unlink/move first"
            )
        function_owner = self._primary_function_owner(addr)
        if function_owner is not None:
            raise ValueError(
                f"{addr}: address is already primary function of {function_owner.id}; "
                "an address cannot be primary function and primary data"
            )

    def _check_new_owner_primary_children(self, owner_data: dict[str, Any]) -> None:
        owner_id = normalize_owner_id(str(owner_data.get("id", "")))
        for raw_address in owner_data.get("member_addresses", []) or []:
            self._reject_primary_function_conflict(owner_id, str(raw_address))
        for raw_item in owner_data.get("data_addresses", []) or []:
            if isinstance(raw_item, dict):
                raw_address = raw_item.get("address", "")
            else:
                raw_address = raw_item
            self._reject_primary_data_conflict(owner_id, str(raw_address))

    def _prune_address_metadata_if_unlinked(self, owner: dict[str, Any], address: str) -> None:
        addr = normalize_owner_address(address)
        if addr in {normalize_owner_address(str(item)) for item in owner.get("anchors", []) or []}:
            return
        if addr in {normalize_owner_address(str(item)) for item in owner.get("member_addresses", []) or []}:
            return
        for item in owner.get("data_addresses", []) or []:
            if isinstance(item, dict) and normalize_owner_address(str(item.get("address", ""))) == addr:
                return
        for raw in owner.get("relationships", []) or []:
            if not isinstance(raw, dict):
                continue
            if raw.get("kind") in {"anchor-address", "primary-function", "primary-data"}:
                if normalize_owner_address(str(raw.get("address", ""))) == addr:
                    return
        metadata = owner.get("address_metadata")
        if isinstance(metadata, dict):
            metadata.pop(addr, None)
            if not metadata:
                owner.pop("address_metadata", None)

    def link_address(self, owner_id: str, address: str) -> None:
        owner = self._mutable_owner(owner_id)
        addr = normalize_owner_address(address)
        self._reject_primary_function_conflict(owner_id, addr)
        addresses = owner.setdefault("member_addresses", [])
        if addr not in addresses:
            addresses.append(addr)
        if self.uses_relationships(owner_id):
            self._upsert_relationship(
                owner_id,
                SourceOwnerRelationship(owner_id=owner_id, kind="primary-function", address=addr),
            )
        if self.payload.get("schema_version") == SCHEMA_VERSION:
            if owner.get("kind") == "provider-boundary":
                raise ValueError(f"{owner_id}: provider-boundary owner cannot link authored primary function")
            owner.setdefault("reimplementation", {}).setdefault("entries", {}).setdefault(
                addr, default_entry_reimplementation("function")
            )

    def unlink_address(self, owner_id: str, address: str) -> None:
        owner = self._mutable_owner(owner_id)
        addr = normalize_owner_address(address)
        addresses = owner.setdefault("member_addresses", [])
        if addr not in addresses:
            raise ValueError(f"{addr} is not linked as a member address to {owner_id}")
        self._prepare_entry_unlink(owner_id, addr)
        owner["member_addresses"] = [item for item in addresses if item != addr]
        self._remove_relationship(owner_id, kind="primary-function", address=addr)
        self._prune_address_metadata_if_unlinked(owner, addr)

    def link_anchor(self, owner_id: str, address: str) -> bool:
        owner = self._mutable_owner(owner_id)
        addr = normalize_owner_address(address)
        anchors = owner.setdefault("anchors", [])
        if addr in anchors:
            return False
        anchors.append(addr)
        if self.uses_relationships(owner_id):
            self._upsert_relationship(
                owner_id,
                SourceOwnerRelationship(owner_id=owner_id, kind="anchor-address", address=addr),
            )
        return True

    def unlink_anchor(self, owner_id: str, address: str) -> None:
        owner = self._mutable_owner(owner_id)
        addr = normalize_owner_address(address)
        anchors = owner.setdefault("anchors", [])
        if addr not in anchors:
            raise ValueError(f"{addr} is not linked as an anchor to {owner_id}")
        owner["anchors"] = [item for item in anchors if item != addr]
        self._remove_relationship(owner_id, kind="anchor-address", address=addr)
        self._prune_address_metadata_if_unlinked(owner, addr)

    def move_anchor(self, from_owner_id: str, to_owner_id: str, address: str) -> bool:
        from_owner = normalize_owner_id(from_owner_id)
        to_owner = normalize_owner_id(to_owner_id)
        if from_owner == to_owner:
            raise ValueError("owner move-anchor requires distinct source and target owners")
        self.owner(to_owner)
        self.unlink_anchor(from_owner, address)
        return self.link_anchor(to_owner, address)

    def link_data(self, owner_id: str, address: str, name: str = "") -> None:
        owner = self._mutable_owner(owner_id)
        addr = normalize_owner_address(address)
        self._reject_primary_data_conflict(owner_id, addr)
        data_addresses = owner.setdefault("data_addresses", [])
        for item in data_addresses:
            if isinstance(item, dict) and item.get("address") == addr:
                if name:
                    item["name"] = name
                    if self.uses_relationships(owner_id):
                        self._upsert_relationship(
                            owner_id,
                            SourceOwnerRelationship(
                                owner_id=owner_id,
                                kind="primary-data",
                                address=addr,
                                name=name,
                            ),
                        )
                return
        link_name = name or "pending"
        data_addresses.append({"address": addr, "name": link_name})
        if self.uses_relationships(owner_id):
            self._upsert_relationship(
                owner_id,
                SourceOwnerRelationship(
                    owner_id=owner_id,
                    kind="primary-data",
                    address=addr,
                    name=link_name,
                ),
            )
        if self.payload.get("schema_version") == SCHEMA_VERSION:
            if owner.get("kind") == "provider-boundary":
                raise ValueError(f"{owner_id}: provider-boundary owner cannot link authored primary data")
            owner.setdefault("reimplementation", {}).setdefault("entries", {}).setdefault(
                addr, default_entry_reimplementation("data")
            )

    def unlink_data(self, owner_id: str, address: str) -> None:
        owner = self._mutable_owner(owner_id)
        addr = normalize_owner_address(address)
        data_addresses = owner.setdefault("data_addresses", [])
        if not any(isinstance(item, dict) and item.get("address") == addr for item in data_addresses):
            raise ValueError(f"{addr} is not linked as a data address to {owner_id}")
        self._prepare_entry_unlink(owner_id, addr)
        owner["data_addresses"] = [
            item
            for item in data_addresses
            if not (isinstance(item, dict) and item.get("address") == addr)
        ]
        self._remove_relationship(owner_id, kind="primary-data", address=addr)
        self._prune_address_metadata_if_unlinked(owner, addr)

    def _prepare_entry_unlink(self, owner_id: str, address: str) -> None:
        if self.payload.get("schema_version") != SCHEMA_VERSION:
            return
        owner = self._mutable_owner(owner_id)
        entries = owner.setdefault("reimplementation", {}).setdefault("entries", {})
        record = entries.get(address)
        if isinstance(record, dict) and record.get("tier", "X") != "X":
            raise ValueError(
                f"{address}: refuses to unlink accepted tier {record.get('tier')} entry from {owner_id}; "
                "use owner move-primary to preserve its evidence"
            )
        entries.pop(address, None)

    def owner_ids(self) -> set[str]:
        return {owner.id for owner in self.owners}

    def owner_dependencies(self, owner_id: str) -> list[str]:
        owner = self.owner(owner_id)
        dependencies = owner.data.get("dependencies", [])
        if not isinstance(dependencies, list) or not all(isinstance(item, str) for item in dependencies):
            raise ValueError(f"{self.path}: {owner.id}: dependencies must be a string list")
        return owner_dependency_ids(owner)

    def set_dependencies(
        self,
        owner_id: str,
        dependencies: list[str],
        evidence: str,
        *,
        reason: str = "manual",
        reason_targets: set[str] | None = None,
    ) -> None:
        if reason not in VALID_RELATIONSHIP_REASONS:
            raise ValueError(
                f"invalid relationship reason {reason!r}; expected one of {', '.join(RELATIONSHIP_REASONS)}"
            )
        owner = self._mutable_owner(owner_id)
        wanted = normalize_owner_id(owner_id)
        normalized: list[str] = []
        seen: set[str] = set()
        existing_owner_ids = self.owner_ids()
        existing_relationships = {
            relationship.target_owner_id: relationship
            for relationship in owner_dependency_relationships(SourceOwner(owner))
        }
        for raw_dependency in dependencies:
            dependency = normalize_owner_id(raw_dependency)
            if dependency == wanted:
                raise ValueError(f"{wanted}: owner dependency cannot reference itself")
            if dependency in seen:
                raise ValueError(f"{wanted}: duplicate owner dependency: {dependency}")
            if dependency not in existing_owner_ids:
                raise ValueError(f"{wanted}: dependency owner not found: {dependency}")
            seen.add(dependency)
            normalized.append(dependency)
        if reason_targets is None:
            reason_target_ids = set(normalized)
        else:
            reason_target_ids = {normalize_owner_id(item) for item in reason_targets}
        owner["dependencies"] = normalized
        if self.uses_relationships(owner_id):
            existing = owner.get("relationships", [])
            owner["relationships"] = [
                item
                for item in existing
                if not (isinstance(item, dict) and item.get("kind") == "depends-on-owner")
            ]
            for dependency in normalized:
                existing_relationship = existing_relationships.get(dependency)
                next_reason = (
                    reason
                    if dependency in reason_target_ids or existing_relationship is None
                    else existing_relationship.reason
                )
                self._upsert_relationship(
                    owner_id,
                    SourceOwnerRelationship(
                        owner_id=owner_id,
                        kind="depends-on-owner",
                        target_owner_id=dependency,
                        reason=next_reason,
                    ),
                )
        if evidence:
            owner.setdefault("evidence", []).append(evidence)

    def add_evidence(self, owner_id: str, evidence: str, *, dedupe: bool = False) -> None:
        text = evidence.strip()
        if not text:
            return
        owner = self._mutable_owner(owner_id)
        evidence_items = owner.setdefault("evidence", [])
        if not dedupe or text not in evidence_items:
            evidence_items.append(text)

    def replace_evidence(
        self,
        owner_id: str,
        match: str,
        replacement: str,
        evidence: str,
    ) -> tuple[str, str]:
        normalized_owner_id = normalize_owner_id(owner_id)
        match_text = str(match)
        replacement_text = str(replacement)
        evidence_text = str(evidence).strip()
        if not match_text.strip():
            raise ValueError("owner replace-evidence requires a non-empty --match")
        if not replacement_text.strip():
            raise ValueError("owner replace-evidence requires a non-empty --replacement")
        if not evidence_text:
            raise ValueError("owner replace-evidence requires non-empty --evidence")

        owner = self._mutable_owner(normalized_owner_id)
        evidence_items = owner.get("evidence")
        if not isinstance(evidence_items, list):
            raise ValueError(f"{normalized_owner_id}: evidence must be a list")
        if not all(isinstance(item, str) for item in evidence_items):
            raise ValueError(f"{normalized_owner_id}: evidence must contain only strings")

        matches = [
            (index, item, item.count(match_text))
            for index, item in enumerate(evidence_items)
            if match_text in item
        ]
        if len(matches) != 1:
            raise ValueError(
                f"{normalized_owner_id}: owner replace-evidence expected exactly one evidence "
                f"item containing --match; found {len(matches)}"
            )
        index, superseded_item, occurrence_count = matches[0]
        if occurrence_count != 1:
            raise ValueError(
                f"{normalized_owner_id}: owner replace-evidence expected exactly one --match "
                f"occurrence in the matched evidence item; found {occurrence_count}"
            )
        if replacement_text == superseded_item:
            raise ValueError("owner replace-evidence rejects an unchanged --replacement")

        evidence_items[index] = replacement_text
        evidence_items.append(
            "evidence item replaced; "
            f"superseded={superseded_item!r}; replacement={replacement_text!r}; "
            f"rationale={evidence_text}"
        )
        return superseded_item, replacement_text

    def set_source_paths(self, owner_id: str, source_paths: list[str], evidence: str) -> tuple[list[str], list[str]]:
        normalized_owner_id = normalize_owner_id(owner_id)
        paths: list[str] = []
        seen: set[str] = set()
        for raw_path in source_paths:
            path = str(raw_path).strip()
            if not path:
                raise ValueError("owner set-source-paths requires non-empty --source-path values")
            if path in seen:
                raise ValueError(f"{normalized_owner_id}: duplicate source path: {path}")
            seen.add(path)
            paths.append(path)
        if not paths:
            raise ValueError("owner set-source-paths requires at least one --source-path")
        evidence_text = evidence.strip()
        if not evidence_text:
            raise ValueError("owner set-source-paths requires --evidence")

        owner = self._mutable_owner(normalized_owner_id)
        old_paths = [
            item.strip()
            for item in owner.get("source_paths", [])
            if isinstance(item, str) and item.strip()
        ]
        owner["source_paths"] = paths
        superseding_evidence = (
            "source_paths updated "
            f"from {old_paths!r} to {paths!r}; "
            "supersedes prior source_paths/evidence where contradicted by current owner/source evidence: "
            f"{evidence_text}"
        )
        owner.setdefault("evidence", []).append(superseding_evidence)
        return old_paths, paths

    def set_section(self, owner_id: str, section_id: str, evidence: str) -> tuple[str, str]:
        normalized_owner_id = normalize_owner_id(owner_id)
        normalized_section = str(section_id).strip()
        if not normalized_section:
            raise ValueError("owner set-section requires a non-empty section id")
        evidence_text = evidence.strip()
        if not evidence_text:
            raise ValueError("owner set-section requires --evidence")

        owner = self._mutable_owner(normalized_owner_id)
        old_section = str(owner.get("section", "") or "").strip()
        owner["section"] = normalized_section
        owner.setdefault("evidence", []).append(
            "section updated "
            f"from {old_section or 'pending'} to {normalized_section}; "
            f"{evidence_text}"
        )
        return old_section, normalized_section

    def set_gates(self, owner_id: str, updates: dict[str, str], evidence: str) -> None:
        if not updates:
            raise ValueError("setting gates requires at least one gate update")
        for gate, state in updates.items():
            if gate not in OWNER_GATES:
                raise ValueError(f"invalid gate {gate!r}; expected one of {', '.join(OWNER_GATES)}")
            if state not in VALID_GATE_STATES:
                raise ValueError(
                    f"invalid gate state {state!r}; expected one of {', '.join(sorted(VALID_GATE_STATES))}"
                )
            if state in {"accepted", "none"} and not evidence:
                raise ValueError(f"setting {gate}={state} requires --evidence")
        owner = self._mutable_owner(owner_id)
        gates = owner.setdefault("gates", default_gates())
        for gate, state in updates.items():
            gates[gate] = state
        if evidence:
            owner.setdefault("evidence", []).append(evidence)

    def set_gate(self, owner_id: str, gate: str, state: str, evidence: str) -> None:
        self.set_gates(owner_id, {gate: state}, evidence)

    def set_blocker(self, owner_id: str, blocker: str, evidence: str) -> None:
        text = blocker.strip()
        if not text:
            raise ValueError("setting blocker requires non-empty text; use 'none' when clear")
        owner = self._mutable_owner(owner_id)
        owner["blocker"] = text
        if evidence:
            owner.setdefault("evidence", []).append(evidence)

    def validate(self) -> list[str]:
        findings: list[str] = []
        schema_version = self.payload.get("schema_version")
        if schema_version != SCHEMA_VERSION:
            findings.append(f"schema_version must be {SCHEMA_VERSION}")
        raw_owners = self.payload.get("owners")
        if not isinstance(raw_owners, list):
            return [*findings, "owners must be a list"]

        all_owner_ids = {
            str(raw.get("id", ""))
            for raw in raw_owners
            if isinstance(raw, dict) and isinstance(raw.get("id"), str)
        }
        seen_ids: set[str] = set()
        global_primary: dict[str, tuple[str, str]] = {}
        for index, raw in enumerate(raw_owners):
            label = f"owners[{index}]"
            if not isinstance(raw, dict):
                findings.append(f"{label}: expected object")
                continue
            owner_id = raw.get("id")
            if not isinstance(owner_id, str) or not owner_id.strip():
                findings.append(f"{label}: missing id")
            elif owner_id in seen_ids:
                findings.append(f"{label}: duplicate id {owner_id}")
            else:
                seen_ids.add(owner_id)

            for key in ("kind", "name", "section", "state"):
                if not isinstance(raw.get(key), str) or not raw.get(key):
                    findings.append(f"{label}: missing {key}")
            if raw.get("kind") not in VALID_OWNER_KINDS:
                findings.append(f"{label}: invalid kind {raw.get('kind')!r}")
            if raw.get("state") not in VALID_OWNER_STATES:
                findings.append(f"{label}: invalid state {raw.get('state')!r}")

            for key in ("anchors", "member_addresses", "source_paths", "dependencies", "evidence"):
                if not isinstance(raw.get(key, []), list) or not all(
                    isinstance(item, str) for item in raw.get(key, [])
                ):
                    findings.append(f"{label}: {key} must be a string list")
            for key in ("anchors", "member_addresses"):
                for value in raw.get(key, []):
                    try:
                        normalize_owner_address(value)
                    except ValueError as exc:
                        findings.append(f"{label}: {key}: {exc}")

            data_addresses = raw.get("data_addresses", [])
            if not isinstance(data_addresses, list):
                findings.append(f"{label}: data_addresses must be a list")
            else:
                for item in data_addresses:
                    if not isinstance(item, dict):
                        findings.append(f"{label}: data_addresses entries must be objects")
                        continue
                    try:
                        normalize_owner_address(str(item.get("address", "")))
                    except ValueError as exc:
                        findings.append(f"{label}: data_addresses: {exc}")
                    if not isinstance(item.get("name"), str) or not item.get("name"):
                        findings.append(f"{label}: data_addresses entries need name")

            gates = raw.get("gates")
            if not isinstance(gates, dict):
                findings.append(f"{label}: gates must be an object")
                continue
            for gate in OWNER_GATES:
                state = gates.get(gate)
                if state not in VALID_GATE_STATES:
                    findings.append(f"{label}: gate {gate} has invalid state {state!r}")
            for gate in gates:
                if gate not in OWNER_GATES:
                    findings.append(f"{label}: unknown gate {gate}")

            if schema_version == SCHEMA_VERSION:
                binary = raw.get("binary")
                if not isinstance(binary, str) or binary not in VALID_OWNER_BINARIES:
                    findings.append(f"{label}: binary must be one of {', '.join(sorted(VALID_OWNER_BINARIES))}")
                if raw.get("kind") == "provider-boundary":
                    if "reimplementation" in raw:
                        findings.append(f"{label}: provider-boundary owners must not carry reimplementation")
                else:
                    reimplementation = raw.get("reimplementation")
                    if not isinstance(reimplementation, dict):
                        findings.append(f"{label}: reimplementation must be an object for schema_version {schema_version}")
                    else:
                        if "tier" in reimplementation:
                            findings.append(f"{label}: schema_version 4 must not persist owner reimplementation tier")
                        entries = reimplementation.get("entries")
                        if not isinstance(entries, dict):
                            findings.append(f"{label}: schema_version 4 reimplementation.entries must be an object")
                        else:
                            try:
                                source_owner = SourceOwner(raw)
                                expected = {
                                    **{address: "function" for address in owner_member_addresses(source_owner)},
                                    **{address: "data" for address in owner_data_addresses(source_owner)},
                                }
                            except ValueError as exc:
                                findings.append(f"{label}: cannot validate entry tiers: {exc}")
                                expected = {}
                            actual: set[str] = set()
                            for entry_address, record in entries.items():
                                try:
                                    address = normalize_owner_address(str(entry_address))
                                except ValueError as exc:
                                    findings.append(f"{label}: reimplementation entry address: {exc}")
                                    continue
                                if address != entry_address:
                                    findings.append(f"{label}: reimplementation entry key must be normalized: {entry_address}")
                                actual.add(address)
                                if not isinstance(record, dict):
                                    findings.append(f"{label}: entry {address} must be an object")
                                    continue
                                kind = record.get("kind")
                                if kind not in {"function", "data"}:
                                    findings.append(f"{label}: entry {address} kind must be function or data")
                                elif expected.get(address) != kind:
                                    findings.append(f"{label}: entry {address} kind {kind!r} does not match primary relationship")
                                if record.get("tier") not in VALID_OWNER_REIMPLEMENTATION_TIERS:
                                    findings.append(f"{label}: entry {address} tier must be one of {', '.join(OWNER_REIMPLEMENTATION_TIERS)}")
                                if not isinstance(record.get("evidence"), str) or not record["evidence"].strip():
                                    findings.append(f"{label}: entry {address} evidence must be non-empty")
                            missing = sorted(set(expected) - actual)
                            extra = sorted(actual - set(expected))
                            if missing:
                                findings.append(f"{label}: missing primary entry tier records: {', '.join(missing)}")
                            if extra:
                                findings.append(f"{label}: non-primary entry tier records: {', '.join(extra)}")

            relationships = raw.get("relationships")
            if schema_version == SCHEMA_VERSION and not isinstance(relationships, list):
                findings.append(f"{label}: relationships must be a list for schema_version {schema_version}")
            if relationships is not None:
                if not isinstance(relationships, list):
                    findings.append(f"{label}: relationships must be a list")
                else:
                    seen_relationships: set[tuple[str, str, str]] = set()
                    for rel_index, relationship in enumerate(relationships):
                        try:
                            normalized = _normalize_relationship_payload(
                                str(raw.get("id", "")),
                                relationship,
                                source=f"{label}: relationships[{rel_index}]",
                            )
                        except ValueError as exc:
                            findings.append(str(exc))
                            continue
                        key = (
                            normalized.kind,
                            normalized.address,
                            normalized.target_owner_id,
                        )
                        if key in seen_relationships:
                            findings.append(f"{label}: duplicate relationship {normalized.as_payload()}")
                        seen_relationships.add(key)
                        if normalized.target_owner_id == raw.get("id"):
                            findings.append(f"{label}: relationship dependency cannot reference itself")
                        if normalized.target_owner_id and normalized.target_owner_id not in all_owner_ids:
                            findings.append(
                                f"{label}: dependency owner not found: {normalized.target_owner_id}"
                            )

                    if schema_version == SCHEMA_VERSION:
                        try:
                            source_owner = SourceOwner(raw)
                            legacy_anchors = {
                                normalize_owner_address(item)
                                for item in raw.get("anchors", [])
                                if isinstance(item, str)
                            }
                            legacy_members = {
                                normalize_owner_address(item)
                                for item in raw.get("member_addresses", [])
                                if isinstance(item, str)
                            }
                            legacy_data = {
                                (
                                    normalize_owner_address(str(item.get("address", ""))),
                                    str(item.get("name", "") or "pending"),
                                )
                                for item in raw.get("data_addresses", [])
                                if isinstance(item, dict)
                            }
                            legacy_dependencies = {
                                normalize_owner_id(item)
                                for item in raw.get("dependencies", [])
                                if isinstance(item, str)
                            }
                            relationship_data = {
                                (relationship.address, relationship.name)
                                for relationship in owner_data_address_records(source_owner)
                            }
                            if owner_anchor_addresses(source_owner) != legacy_anchors:
                                findings.append(f"{label}: anchor-address relationships do not match anchors")
                            if owner_member_addresses(source_owner) != legacy_members:
                                findings.append(
                                    f"{label}: primary-function relationships do not match member_addresses"
                                )
                            if relationship_data != legacy_data:
                                findings.append(
                                    f"{label}: primary-data relationships do not match data_addresses"
                                )
                            if set(owner_dependency_ids(source_owner)) != legacy_dependencies:
                                findings.append(
                                    f"{label}: depends-on-owner relationships do not match dependencies"
                                )
                        except ValueError as exc:
                            findings.append(f"{label}: relationships mirror check failed: {exc}")

            try:
                source_owner = SourceOwner(raw)
                if source_owner.kind == "provider-boundary":
                    continue
                for address in owner_member_addresses(source_owner):
                    previous = global_primary.get(address)
                    if previous is not None:
                        findings.append(
                            f"{label}: primary function {address} conflicts with {previous[1]} of {previous[0]}"
                        )
                    else:
                        global_primary[address] = (str(owner_id), "primary-function")
                for address in owner_data_addresses(source_owner):
                    previous = global_primary.get(address)
                    if previous is not None:
                        findings.append(
                            f"{label}: primary data {address} conflicts with {previous[1]} of {previous[0]}"
                        )
                    else:
                        global_primary[address] = (str(owner_id), "primary-data")
            except ValueError:
                pass
        return findings


def owner_gate_summary(owners: list[SourceOwner]) -> str:
    if not owners:
        return "no linked source owner"
    parts = []
    for owner in owners:
        gate_text = " ".join(f"{gate}={owner.gate(gate)}" for gate in OWNER_GATES)
        parts.append(f"{owner.id}({owner.kind}; {gate_text})")
    return "; ".join(parts)
