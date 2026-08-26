from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


DONE_STATUS = "✅"
NO_GLOBALS_STATUS = "❎"
LIMITED_STATUS = "☑️"
NOT_DONE_STATUS = "❌"
UNKNOWN_STATUS = "❓"
VALID_STATUSES = {DONE_STATUS, LIMITED_STATUS, NOT_DONE_STATUS, UNKNOWN_STATUS}
ACCEPTED_STATUSES = {DONE_STATUS, LIMITED_STATUS}
VALID_DATA_STATUSES = {DONE_STATUS, NO_GLOBALS_STATUS, NOT_DONE_STATUS}
ACCEPTED_DATA_STATUSES = {DONE_STATUS, NO_GLOBALS_STATUS}
VALID_OWNER_STATUSES = {DONE_STATUS, NOT_DONE_STATUS}
ACCEPTED_OWNER_STATUSES = {DONE_STATUS}

FUNCTIONAL_LANE = "functional"
BINARY_LANE = "binary"
LANE_ALIASES = {FUNCTIONAL_LANE: FUNCTIONAL_LANE, BINARY_LANE: BINARY_LANE}
LANE_CHOICES = sorted(LANE_ALIASES)

TIER_NONE = "X"
TIER_FUNCTIONAL = "C"
TIER_DATA_EQUIVALENT = "B"
TIER_REGISTER_MATCH = "A"
TIER_BINARY_SAFE = "S"
VALID_REIMPLEMENTATION_TIERS = {
    TIER_NONE,
    TIER_FUNCTIONAL,
    TIER_DATA_EQUIVALENT,
    TIER_REGISTER_MATCH,
    TIER_BINARY_SAFE,
}
TIER_ORDER = {
    TIER_NONE: 0,
    TIER_FUNCTIONAL: 1,
    TIER_DATA_EQUIVALENT: 2,
    TIER_REGISTER_MATCH: 3,
    TIER_BINARY_SAFE: 4,
}

BINARY_BLOCKER_PRIORITY = {
    "recon": 0,
    "provider": 0,
    "deps": 1,
    "owner": 2,
    "impl": 3,
    "functional": 4,
    "data": 5,
    "verify": 6,
}
DEFAULT_BLOCKER_PRIORITY = max(BINARY_BLOCKER_PRIORITY.values()) + 1

FIELD_LABELS = {
    "recon": "Reconstructed",
    "deps": "Source dependencies satisfied",
    "owner": "Source owner",
    "data": "Data reimplemented",
    "impl": "Reimplemented",
    "functional": "Reimplemented [C]",
    "verify": "Reimplemented [S]",
    "provider": "Provider-boundary",
}

SOURCE_OWNER_KINDS = {
    "pending",
    "standalone",
    "class",
    "struct",
    "interface",
    "custom-table",
    "callback-table",
    "namespace",
    "source-file",
    "subsystem",
    "global-data",
    "cluster",
}
SOURCE_OWNER_STATES = {"audit-pending", "parent-pending", "implemented", "standalone"}
SOURCE_MODELS = {"pending", "source-faithful", "data-equivalent-only", "byte-equivalent-only"}
RETIRED_DATA_REASONS: set[str] = set()


@dataclass(frozen=True)
class OwnerEntry:
    address: str
    reconstructed_status: str = DONE_STATUS
    reconstructed_name: str = ""
    source_dependencies_status: str = NOT_DONE_STATUS
    reimplemented_status: str = NOT_DONE_STATUS
    reimplemented_name: str = ""
    reimplemented_file: str = ""
    provider: str = ""
    provider_boundary_status: str = "?"
    provider_kind: str = ""
    provider_name: str = ""
    provider_origin: str = ""
    provider_file: str = ""
    provider_target: str = ""
    binary_verified_status: str = NOT_DONE_STATUS
    functional_equivalent_status: str = NOT_DONE_STATUS
    functional_target: str = ""
    reimplementation_tier: str = TIER_NONE
    entry_group: str = ""
    source_model: str = "pending"
    source_owner: str = ""
    source_owner_status: str = NOT_DONE_STATUS
    source_owner_kind: str = "pending"
    source_owner_parent: str = ""
    source_owner_state: str = "audit-pending"
    data_state: str = NOT_DONE_STATUS
    source_blocker: str = ""
    group_title: str = ""
    group_id: str = ""
    group_kind: str = ""
    group_source: str = ""
    group_validation: str = "SOURCE_OWNERS schema v4"
    group_depends_on: str = ""
    entry_kind: str = "function"
    data_section: str = ""
    data_size: str = ""
    data_type: str = ""
    retired_data_reason: str = ""
    retired_data_evidence: str = ""
    retired_data_duplicate_of: str = ""

    @property
    def is_provider(self) -> bool:
        return self.is_provider_boundary or bool(self.provider) or self.reimplemented_file == "external"

    @property
    def is_data_entry(self) -> bool:
        return self.entry_kind == "data"

    @property
    def is_retired_data_entry(self) -> bool:
        return False

    @property
    def is_provider_boundary(self) -> bool:
        return self.provider_boundary_status in VALID_STATUSES

    @property
    def is_provider_ready(self) -> bool:
        return self.is_provider_boundary and self.provider_boundary_status in ACCEPTED_STATUSES

    @property
    def is_reimplemented(self) -> bool:
        return self.is_provider_ready or self.reimplemented_status == DONE_STATUS

    @property
    def is_source_dependencies_satisfied(self) -> bool:
        if self.is_data_entry:
            return self.has_accepted_source_owner
        return self.is_provider_ready or self.source_dependencies_status in ACCEPTED_STATUSES

    @property
    def is_binary_verified(self) -> bool:
        return self.accepted_reimplementation_tier == TIER_BINARY_SAFE

    @property
    def accepted_reimplementation_tier(self) -> str:
        if self.reimplemented_status not in ACCEPTED_STATUSES:
            return TIER_NONE
        return self.reimplementation_tier

    @property
    def is_functionally_accepted(self) -> bool:
        return tier_at_least(self.accepted_reimplementation_tier, TIER_FUNCTIONAL)

    @property
    def is_functionally_equivalent(self) -> bool:
        return self.is_functionally_accepted

    @property
    def is_data_equivalent(self) -> bool:
        return tier_at_least(self.accepted_reimplementation_tier, TIER_DATA_EQUIVALENT)

    @property
    def has_accepted_data_state(self) -> bool:
        if self.is_data_entry:
            return self.is_data_equivalent
        return self.data_state in ACCEPTED_DATA_STATUSES

    @property
    def has_accepted_source_owner(self) -> bool:
        return self.source_owner_status in ACCEPTED_OWNER_STATUSES

    @property
    def data_status(self) -> str:
        if self.is_data_entry:
            return DONE_STATUS if self.has_accepted_data_state else NOT_DONE_STATUS
        return self.data_state

    @property
    def has_accepted_data_status(self) -> bool:
        return self.has_accepted_data_state


class OwnerEntryIndex:
    def __init__(self, path: Path, entries: dict[str, OwnerEntry], order: list[str], *, binary: str | None) -> None:
        self.path = path
        self.entries = entries
        self.order = order
        self.binary = binary
        self.synthetic = False

    @classmethod
    def load(cls, path: str | Path, *, binary: str | None = None) -> "OwnerEntryIndex":
        from _recoil.lib.source_owners import SourceOwnerDocument

        owner_doc = SourceOwnerDocument.load(Path(path))
        return cls.from_source_owners(owner_doc, binary=binary)

    @classmethod
    def from_source_owners(cls, owner_doc, *, binary: str | None = None) -> "OwnerEntryIndex":
        from _recoil.lib.source_owners import owner_data_address_records, owner_dependency_ids, owner_member_addresses

        if binary is not None and binary not in {"recoil", "messages"}:
            raise ValueError(f"invalid binary {binary!r}; expected recoil or messages")
        entries: dict[str, OwnerEntry] = {}
        order: list[str] = []
        for owner in owner_doc.owners:
            owner_binary = owner.binary or "recoil"
            if binary is not None and owner_binary != binary:
                continue
            metadata_raw = owner.data.get("address_metadata", {})
            metadata = metadata_raw if isinstance(metadata_raw, dict) else {}
            relationships = [
                *(("function", address, "") for address in owner_member_addresses(owner)),
                *(("data", item.address, item.name) for item in owner_data_address_records(owner)),
            ]
            relationships.sort(key=lambda item: int(item[1], 16))
            for entry_kind, address, relationship_name in relationships:
                if address in entries:
                    previous_entry = entries[address]
                    previous = previous_entry.source_owner_parent
                    if previous == owner.id and owner.kind == "provider-boundary":
                        continue
                    # Unified progress may legitimately track a data symbol and
                    # a function at the same image address.  This legacy index
                    # is address-keyed and is used for active function routing,
                    # so retain the function projection while still rejecting
                    # two competing primary owners for the same entry kind.
                    if previous_entry.entry_kind != entry_kind:
                        if previous_entry.entry_kind == "function":
                            continue
                        if entry_kind == "function":
                            raw_meta = metadata.get(address, {})
                            meta = raw_meta if isinstance(raw_meta, dict) else {}
                            entries[address] = _entry_from_owner(
                                owner,
                                entry_kind,
                                address,
                                relationship_name,
                                meta,
                                owner_dependency_ids(owner),
                            )
                            continue
                    raise ValueError(
                        f"duplicate primary address {address}: {previous} and {owner.id}"
                    )
                raw_meta = metadata.get(address, {})
                meta = raw_meta if isinstance(raw_meta, dict) else {}
                entries[address] = _entry_from_owner(owner, entry_kind, address, relationship_name, meta, owner_dependency_ids(owner))
                order.append(address)
        return cls(owner_doc.path, entries, order, binary=binary)

    def entry_text(self, address: str) -> str:
        entry = self.entries[normalize_address(address)]
        return (
            f"{entry.address} kind={entry.entry_kind} name={entry.reimplemented_name or entry.reconstructed_name} "
            f"owner={entry.source_owner_parent} tier={entry.reimplementation_tier} "
            f"section={entry.entry_group or 'unmapped'} blocker={entry.source_blocker or 'none'}"
        )

    def group_ids(self) -> set[str]:
        return {entry.group_id for entry in self.entries.values() if entry.group_id}

    def find(self, query: str) -> list[OwnerEntry]:
        needle = query.lower()
        return [
            self.entries[address]
            for address in self.order
            if needle in address.lower()
            or needle in self.entries[address].reconstructed_name.lower()
            or needle in self.entries[address].reimplemented_name.lower()
            or needle in self.entries[address].source_owner_parent.lower()
            or needle in self.entries[address].group_id.lower()
        ]

    def group_entries(self, group_query: str) -> list[OwnerEntry]:
        needle = group_query.lower()
        return [
            self.entries[address]
            for address in self.order
            if needle in self.entries[address].group_id.lower()
            or needle in self.entries[address].group_title.lower()
        ]

    def first_unfinished(self, *, lane: str = FUNCTIONAL_LANE) -> OwnerEntry | None:
        return first_unfinished((self.entries[address] for address in self.order), lane=lane)


def _entry_from_owner(owner, entry_kind: str, address: str, relationship_name: str, meta: dict, dependencies: list[str]) -> OwnerEntry:
    owner_name = str(owner.data.get("name", "") or owner.id)
    source_paths = [item for item in owner.data.get("source_paths", []) if isinstance(item, str) and item.strip()]
    source_path = str(meta.get("source_path", "") or (source_paths[0] if source_paths else ""))
    name = str(meta.get("name", "") or relationship_name or owner_name)
    target = str(meta.get("target", "") or owner.id)
    section = str(owner.data.get("section", "") or "unmapped")
    owner_ready = owner.owner_scope_ready()
    source_status = DONE_STATUS if owner_ready else NOT_DONE_STATUS
    data_state = NO_GLOBALS_STATUS if owner.no_data() else DONE_STATUS if owner.data_ready() else NOT_DONE_STATUS
    dependencies_ready = owner.linkage_ready()
    tier = owner.entry_reimplementation_tier(address) if owner.kind != "provider-boundary" else TIER_NONE
    provider = owner.kind == "provider-boundary"
    implemented = provider or tier != TIER_NONE
    source_owner_kind = {
        "data-owner": "global-data",
        "record": "struct",
        "provider-boundary": "pending",
    }.get(owner.kind, owner.kind or "pending")
    return OwnerEntry(
        address=address,
        reconstructed_status=DONE_STATUS,
        reconstructed_name=name,
        source_dependencies_status=DONE_STATUS if provider or dependencies_ready or tier != TIER_NONE else NOT_DONE_STATUS,
        reimplemented_status=DONE_STATUS if implemented else NOT_DONE_STATUS,
        reimplemented_name=name,
        reimplemented_file="external" if provider else source_path,
        provider=str(owner.data.get("provider", "") or owner_name) if provider else "",
        provider_boundary_status=DONE_STATUS if provider else "?",
        provider_kind=owner.kind if provider else "",
        provider_name=owner_name if provider else "",
        provider_origin=str(owner.data.get("origin", "") or "SOURCE_OWNERS") if provider else "",
        provider_file="external" if provider else "",
        provider_target=target if provider else "",
        binary_verified_status=DONE_STATUS if tier == TIER_BINARY_SAFE else NOT_DONE_STATUS,
        functional_equivalent_status=DONE_STATUS if tier_at_least(tier, TIER_FUNCTIONAL) else NOT_DONE_STATUS,
        functional_target=str(meta.get("target", "") or ""),
        reimplementation_tier=tier,
        entry_group=section,
        source_model="source-faithful" if owner_ready else "pending",
        source_owner=owner.id,
        source_owner_status=source_status,
        source_owner_kind=source_owner_kind,
        source_owner_parent=owner.id,
        source_owner_state="standalone" if owner.kind == "standalone" else "implemented" if owner_ready else "audit-pending",
        data_state=data_state,
        source_blocker=str(owner.data.get("blocker", "") or ""),
        group_title=owner_name,
        group_id=section,
        group_kind=owner.kind,
        group_source=source_path,
        group_depends_on=", ".join(dependencies),
        entry_kind=entry_kind,
        data_section=str(meta.get("section", "") or ".data") if entry_kind == "data" else "",
        data_size=str(meta.get("size", "") or "pending") if entry_kind == "data" else "",
        data_type=str(meta.get("type", "") or "pending") if entry_kind == "data" else "",
    )


def load_owner_entries(path: str | Path, *, binary: str | None = None) -> dict[str, OwnerEntry]:
    return OwnerEntryIndex.load(path, binary=binary).entries


def normalize_address(value: str) -> str:
    text = value.strip()
    if text.lower().startswith("sub_"):
        text = "0x" + text[4:]
    return f"0x{int(text, 16):x}"


def normalize_tier(value: str) -> str:
    tier = (value or TIER_NONE).strip().upper()
    if tier not in VALID_REIMPLEMENTATION_TIERS:
        valid = ", ".join(sorted(VALID_REIMPLEMENTATION_TIERS, key=TIER_ORDER.get))
        raise ValueError(f"invalid reimplementation tier {value!r}; expected one of: {valid}")
    return tier


def normalize_source_model(value: str) -> str:
    model = value.strip().lower()
    if model not in SOURCE_MODELS:
        raise ValueError(f"invalid source model {value!r}; expected one of: {', '.join(sorted(SOURCE_MODELS))}")
    return model


def normalize_source_owner_kind(value: str) -> str:
    kind = value.strip().lower().replace("_", "-").replace(" ", "-")
    if kind not in SOURCE_OWNER_KINDS:
        raise ValueError(f"invalid source owner kind {value!r}; expected one of: {', '.join(sorted(SOURCE_OWNER_KINDS))}")
    return kind


def normalize_source_owner_state(value: str) -> str:
    state = value.strip().lower().replace("_", "-").replace(" ", "-")
    if state not in SOURCE_OWNER_STATES:
        raise ValueError(f"invalid source owner state {value!r}; expected one of: {', '.join(sorted(SOURCE_OWNER_STATES))}")
    return state


def normalize_retired_data_reason(value: str) -> str:
    raise ValueError("retired noncanonical data entries are no longer supported")


def tier_at_least(value: str, minimum: str) -> bool:
    return TIER_ORDER[normalize_tier(value)] >= TIER_ORDER[normalize_tier(minimum)]


def max_tier(first: str, second: str) -> str:
    first_tier = normalize_tier(first)
    second_tier = normalize_tier(second)
    return first_tier if TIER_ORDER[first_tier] >= TIER_ORDER[second_tier] else second_tier


def normalize_lane(value: str) -> str:
    key = value.strip().lower().replace("_", "-")
    lane = LANE_ALIASES.get(key)
    if lane is None:
        raise ValueError(f"invalid lane {value!r}; expected one of: {', '.join(LANE_CHOICES)}")
    return lane


def status_summary(entry: OwnerEntry, *, include_functional: bool = True) -> str:
    _ = include_functional
    if entry.is_provider_boundary:
        return f"recon={entry.reconstructed_status} provider={entry.provider_boundary_status} kind={entry.provider_kind}"
    if entry.is_data_entry:
        return (
            f"kind=data recon={entry.reconstructed_status} owner_status={entry.source_owner_status} "
            f"impl={entry.reimplemented_status} tier={entry.reimplementation_tier} "
            f"section={entry.data_section or 'pending'} size={entry.data_size or 'pending'}"
        )
    return (
        f"recon={entry.reconstructed_status} deps={entry.source_dependencies_status} "
        f"owner_status={entry.source_owner_status} data_status={entry.data_status} "
        f"impl={entry.reimplemented_status} tier={entry.reimplementation_tier}"
    )


def blocker_field(entry: OwnerEntry, *, lane: str = FUNCTIONAL_LANE) -> str | None:
    lane = normalize_lane(lane)
    if entry.reconstructed_status in {NOT_DONE_STATUS, UNKNOWN_STATUS, "?"}:
        return "recon"
    if entry.is_provider_boundary:
        return None if entry.is_provider_ready else "provider"
    if entry.is_data_entry:
        if lane == BINARY_LANE and not entry.has_accepted_source_owner:
            return "owner"
        if entry.reimplemented_status != DONE_STATUS:
            return "impl"
        if lane == BINARY_LANE and not entry.is_data_equivalent:
            return "data"
        return None if entry.is_binary_verified else "verify"
    if entry.source_dependencies_status in {NOT_DONE_STATUS, UNKNOWN_STATUS, "?"}:
        return "deps"
    if lane == BINARY_LANE and not entry.has_accepted_source_owner:
        return "owner"
    if entry.reimplemented_status != DONE_STATUS:
        return "impl"
    if not entry.is_functionally_accepted:
        return "functional"
    if lane == BINARY_LANE and (not entry.has_accepted_data_state or not entry.is_data_equivalent):
        return "data"
    if lane == BINARY_LANE and not entry.is_binary_verified:
        return "verify"
    if entry.source_dependencies_status == LIMITED_STATUS:
        return "deps"
    return None


def blocker_priority(entry: OwnerEntry, *, lane: str = FUNCTIONAL_LANE) -> int:
    field = blocker_field(entry, lane=lane)
    if field is None:
        return DEFAULT_BLOCKER_PRIORITY
    if normalize_lane(lane) == BINARY_LANE:
        return BINARY_BLOCKER_PRIORITY.get(field, DEFAULT_BLOCKER_PRIORITY)
    return 0


def authored_owner_data_gate_counts(entries: Iterable[OwnerEntry]) -> dict[str, int]:
    owner_count = 0
    data_count = 0
    for entry in entries:
        if entry.is_provider_boundary:
            continue
        if not entry.has_accepted_source_owner:
            owner_count += 1
        if not entry.is_data_entry and not entry.has_accepted_data_state:
            data_count += 1
    return {"owner": owner_count, "data": data_count}


def tier_s_priority_ready(entries: Iterable[OwnerEntry]) -> bool:
    counts = authored_owner_data_gate_counts(entries)
    return counts["owner"] == 0 and counts["data"] == 0


def first_unfinished(entries: Iterable[OwnerEntry], *, lane: str = FUNCTIONAL_LANE) -> OwnerEntry | None:
    lane = normalize_lane(lane)
    pending = [(index, entry) for index, entry in enumerate(entries) if blocker_field(entry, lane=lane)]
    if not pending:
        return None
    if lane != BINARY_LANE:
        return pending[0][1]
    return min(pending, key=lambda item: (blocker_priority(item[1], lane=lane), item[0]))[1]
