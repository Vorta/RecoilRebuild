#!/usr/bin/env python3
"""Resolve and safely replace Source Traceability v1 tracker state.

This module is deliberately importable without CLI registration.  It does not
schedule reconstruction work and it never creates tracker artifacts: source
artifacts must already exist as physical symbol rows or nested logical aliases.
"""

from __future__ import annotations

import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse
from copy import deepcopy
from dataclasses import dataclass
import json
import re
from typing import Any, Iterable, Mapping

from _recoil.lib.live_progress import (
    TRACKER_SCHEMA_VERSION,
    ConcurrentRevisionUpdate,
    LiveProgressError,
    RevisionStore,
)
from _recoil.lib.progress import (
    DEFAULT_PROGRESS_PATH,
    ConcurrentProgressUpdate,
    ProgressError,
    ProgressStore,
)
from _recoil.lib.tooling import REPO_ROOT, configure_stdio


DEFAULT_TRACKER = DEFAULT_PROGRESS_PATH
SOURCE_TRACEABILITY_STATES = frozenset(
    {"resolved", "unresolved", "not-applicable"}
)
SOURCE_EDGE_RELATIONS = frozenset({"defines", "emits"})
SOURCE_ARTIFACT_ID_RE = re.compile(
    r"^[a-z0-9_-]+:(?:function|data|logical-function|logical-data):"
    r"[^:\s]+(?:[:][^:\s]+)*$"
)
ANCHOR_ID_RE = re.compile(r"^recoil:anchor:[a-z0-9][a-z0-9._-]*$")
EVIDENCE_ID_RE = re.compile(
    r"^[a-z0-9_-]+:evidence:r[0-9]+:[0-9]{6,}$"
)
REPLACE_BATCH_OPERATION = "replace-batch"
MIGRATION_METADATA = {
    "version": 1,
    "policy": "topology-only-no-acceptance",
    "state": "initialized",
}
LEGACY_CLAIM_KIND_HINTS = frozenset({"function", "data"})
LEGACY_CLAIM_RESOLUTION_KINDS = frozenset(
    {"exact-existing-artifact", "interior-of-existing-artifact"}
)


class SourceTraceProgressError(ValueError):
    """A fail-closed Source Traceability progress error."""


@dataclass(frozen=True)
class TrackerArtifact:
    artifact_id: str
    location: str
    collection: str
    parent_artifact_id: str | None
    output_section_id: str | None
    row: Mapping[str, Any]

    @property
    def output_section(self) -> str | None:
        if self.output_section_id is None:
            return None
        binary = self.artifact_id.split(":", 1)[0]
        prefix = f"{binary}:section:"
        return (
            self.output_section_id[len(prefix) :]
            if self.output_section_id.startswith(prefix)
            else self.output_section_id
        )

    def to_dict(self) -> dict[str, Any]:
        return {
            "artifact_id": self.artifact_id,
            "location": self.location,
            "collection": self.collection,
            "parent_artifact_id": self.parent_artifact_id,
            "output_section_id": self.output_section_id,
            "output_section": self.output_section,
            "row": deepcopy(dict(self.row)),
        }


@dataclass(frozen=True)
class SourceTraceabilityBatchPlan:
    expected_revision: int
    proposed_revision: int
    artifact_ids: tuple[str, ...]
    legacy_claim_resolution_count: int
    initializes_migration: bool
    migration_metadata: Mapping[str, Any]
    proposed: Mapping[str, Any]

    def to_dict(self, *, include_document: bool = False) -> dict[str, Any]:
        result = {
            "operation": REPLACE_BATCH_OPERATION,
            "expected_revision": self.expected_revision,
            "proposed_revision": self.proposed_revision,
            "artifact_ids": list(self.artifact_ids),
            "update_count": len(self.artifact_ids),
            "legacy_claim_resolution_count": self.legacy_claim_resolution_count,
            "initializes_migration": self.initializes_migration,
            "migration_metadata": deepcopy(dict(self.migration_metadata)),
        }
        if include_document:
            result["proposed"] = deepcopy(dict(self.proposed))
        return result


def _require_mapping(value: Any, *, label: str) -> Mapping[str, Any]:
    if not isinstance(value, Mapping):
        raise SourceTraceProgressError(f"{label} must be an object")
    return value


def _require_exact_keys(
    value: Mapping[str, Any], *, keys: Iterable[str], label: str
) -> None:
    expected = set(keys)
    actual = set(value)
    if actual != expected:
        missing = sorted(expected - actual)
        extra = sorted(actual - expected)
        raise SourceTraceProgressError(
            f"{label} keys must be exactly {sorted(expected)}; "
            f"missing={missing}, extra={extra}"
        )


def _normalized_translation_unit(value: Any, *, label: str) -> str:
    if not isinstance(value, str) or not value:
        raise SourceTraceProgressError(f"{label} must be a non-empty string")
    normalized = value.replace("\\", "/")
    if (
        normalized != value
        or normalized.startswith("/")
        or re.match(r"^[A-Za-z]:", normalized)
        or any(part in {"", ".", ".."} for part in normalized.split("/"))
    ):
        raise SourceTraceProgressError(
            f"{label} must be a normalized repository-relative path"
        )
    return normalized


def normalize_source_traceability(value: Any) -> dict[str, Any]:
    """Validate and canonicalize the exact nested v1 state object."""

    raw = _require_mapping(value, label="source_traceability")
    _require_exact_keys(
        raw,
        keys=("state", "source_edges", "reason_code"),
        label="source_traceability",
    )
    state = raw["state"]
    if state not in SOURCE_TRACEABILITY_STATES:
        raise SourceTraceProgressError(
            "source_traceability.state must be resolved, unresolved, or "
            "not-applicable"
        )
    reason_code = raw["reason_code"]
    if state == "resolved":
        if reason_code is not None:
            raise SourceTraceProgressError(
                "resolved source_traceability.reason_code must be null"
            )
    elif (
        not isinstance(reason_code, str)
        or not reason_code
        or re.fullmatch(r"[a-z0-9][a-z0-9._-]*", reason_code) is None
    ):
        raise SourceTraceProgressError(
            f"{state} source_traceability.reason_code must be a non-empty "
            "lowercase governed reason"
        )
    raw_edges = raw["source_edges"]
    if not isinstance(raw_edges, list):
        raise SourceTraceProgressError(
            "source_traceability.source_edges must be an array"
        )

    edges: list[dict[str, Any]] = []
    identities: set[tuple[str, str, str]] = set()
    for index, raw_edge in enumerate(raw_edges):
        label = f"source_traceability.source_edges[{index}]"
        edge = _require_mapping(raw_edge, label=label)
        _require_exact_keys(
            edge,
            keys=("relation", "anchor_id", "emission_context", "evidence_ids"),
            label=label,
        )
        relation = edge["relation"]
        if relation not in SOURCE_EDGE_RELATIONS:
            raise SourceTraceProgressError(
                f"{label}.relation must be lowercase defines or emits"
            )
        anchor_id = edge["anchor_id"]
        if not isinstance(anchor_id, str) or ANCHOR_ID_RE.fullmatch(anchor_id) is None:
            raise SourceTraceProgressError(
                f"{label}.anchor_id must match recoil:anchor:<stable-id>"
            )
        context = _require_mapping(
            edge["emission_context"], label=f"{label}.emission_context"
        )
        _require_exact_keys(
            context,
            keys=("translation_unit",),
            label=f"{label}.emission_context",
        )
        translation_unit = _normalized_translation_unit(
            context["translation_unit"],
            label=f"{label}.emission_context.translation_unit",
        )
        evidence_ids = edge["evidence_ids"]
        if not isinstance(evidence_ids, list) or not all(
            isinstance(item, str) and EVIDENCE_ID_RE.fullmatch(item)
            for item in evidence_ids
        ):
            raise SourceTraceProgressError(
                f"{label}.evidence_ids must be an array of exact tracker evidence ids"
            )
        if len(evidence_ids) != len(set(evidence_ids)):
            raise SourceTraceProgressError(f"{label}.evidence_ids contains duplicates")
        identity = (relation, anchor_id, translation_unit)
        if identity in identities:
            raise SourceTraceProgressError(
                f"{label} duplicates relation/anchor/translation-unit edge"
            )
        identities.add(identity)
        edges.append(
            {
                "relation": relation,
                "anchor_id": anchor_id,
                "emission_context": {"translation_unit": translation_unit},
                "evidence_ids": sorted(evidence_ids),
            }
        )

    if state == "resolved" and not edges:
        raise SourceTraceProgressError(
            "resolved source_traceability requires at least one source edge"
        )
    if state != "resolved" and edges:
        raise SourceTraceProgressError(
            f"{state} source_traceability must not contain resolved source edges"
        )
    edges.sort(
        key=lambda edge: (
            edge["anchor_id"],
            edge["relation"],
            edge["emission_context"]["translation_unit"],
            edge["evidence_ids"],
        )
    )
    return {
        "state": state,
        "source_edges": edges,
        "reason_code": reason_code,
    }


def _iter_tracker_artifacts_mutable(
    tracker: Mapping[str, Any],
) -> list[tuple[TrackerArtifact, dict[str, Any]]]:
    symbols = _require_mapping(tracker.get("symbols"), label="tracker.symbols")
    occurrences: dict[str, list[tuple[TrackerArtifact, dict[str, Any]]]] = {}
    for raw_symbol_id, raw_symbol in symbols.items():
        symbol_id = str(raw_symbol_id)
        if SOURCE_ARTIFACT_ID_RE.fullmatch(symbol_id) is None:
            continue
        if not isinstance(raw_symbol, dict):
            raise SourceTraceProgressError(
                f"tracker symbol artifact {symbol_id!r} must be an object"
            )
        output_section_id = raw_symbol.get("output_section_id")
        if not isinstance(output_section_id, str) or not output_section_id:
            output_section_id = None
        physical = TrackerArtifact(
            artifact_id=symbol_id,
            location=f"symbols[{symbol_id!r}]",
            collection="symbols",
            parent_artifact_id=None,
            output_section_id=output_section_id,
            row=deepcopy(raw_symbol),
        )
        occurrences.setdefault(symbol_id, []).append((physical, raw_symbol))

        aliases = raw_symbol.get("logical_aliases", {})
        if aliases is None:
            aliases = {}
        if not isinstance(aliases, Mapping):
            raise SourceTraceProgressError(
                f"tracker artifact {symbol_id!r}.logical_aliases must be an object"
            )
        for raw_alias_id, raw_alias in aliases.items():
            alias_id = str(raw_alias_id)
            if SOURCE_ARTIFACT_ID_RE.fullmatch(alias_id) is None:
                raise SourceTraceProgressError(
                    f"nested logical alias id {alias_id!r} is not a supported source artifact id"
                )
            if ":logical-" not in alias_id:
                raise SourceTraceProgressError(
                    f"nested alias {alias_id!r} must be logical-function or logical-data"
                )
            if not isinstance(raw_alias, dict):
                raise SourceTraceProgressError(
                    f"nested logical alias {alias_id!r} must be an object"
                )
            alias = TrackerArtifact(
                artifact_id=alias_id,
                location=(
                    f"symbols[{symbol_id!r}].logical_aliases[{alias_id!r}]"
                ),
                collection="symbols.logical_aliases",
                parent_artifact_id=symbol_id,
                output_section_id=output_section_id,
                row=deepcopy(raw_alias),
            )
            occurrences.setdefault(alias_id, []).append((alias, raw_alias))

    ambiguous = {
        artifact_id: rows
        for artifact_id, rows in occurrences.items()
        if len(rows) != 1
    }
    if ambiguous:
        artifact_id = sorted(ambiguous)[0]
        locations = [item[0].location for item in ambiguous[artifact_id]]
        raise SourceTraceProgressError(
            f"source artifact id {artifact_id!r} is ambiguous: {locations}"
        )
    return [occurrences[key][0] for key in sorted(occurrences)]


def iter_tracker_artifacts(tracker: Mapping[str, Any]) -> tuple[TrackerArtifact, ...]:
    """Return immutable copies of all supported current tracker artifacts."""

    return tuple(item[0] for item in _iter_tracker_artifacts_mutable(tracker))


def resolve_tracker_artifact(
    tracker: Mapping[str, Any], artifact_id: str
) -> TrackerArtifact:
    """Resolve exactly one existing physical symbol or nested logical alias."""

    if not isinstance(artifact_id, str) or SOURCE_ARTIFACT_ID_RE.fullmatch(
        artifact_id
    ) is None:
        raise SourceTraceProgressError(
            f"unsupported source artifact id {artifact_id!r}"
        )
    matches = [
        item[0]
        for item in _iter_tracker_artifacts_mutable(tracker)
        if item[0].artifact_id == artifact_id
    ]
    if len(matches) != 1:
        raise SourceTraceProgressError(
            f"source artifact id {artifact_id!r} must resolve exactly once; "
            f"found {len(matches)}"
        )
    return matches[0]


def source_traceability_snapshot(
    tracker: Mapping[str, Any], artifact_id: str
) -> dict[str, Any] | None:
    artifact = resolve_tracker_artifact(tracker, artifact_id)
    if "source_traceability" not in artifact.row:
        return None
    return normalize_source_traceability(artifact.row["source_traceability"])


def normalize_artifact_address(value: str | int) -> str:
    if isinstance(value, bool):
        raise SourceTraceProgressError("artifact address must not be boolean")
    try:
        number = value if isinstance(value, int) else int(str(value), 16)
    except (TypeError, ValueError) as exc:
        raise SourceTraceProgressError(
            f"invalid artifact address {value!r}"
        ) from exc
    if number < 0:
        raise SourceTraceProgressError("artifact address must be non-negative")
    return f"0x{number:x}"


def artifact_address(artifact_id: str) -> str:
    parts = artifact_id.split(":")
    if len(parts) < 3:
        raise SourceTraceProgressError(
            f"source artifact id {artifact_id!r} has no address component"
        )
    return normalize_artifact_address(parts[2])


def show_source_traceability(
    tracker_or_path: Mapping[str, Any] | str | Path,
    *,
    artifact_ids: Iterable[str] | None = None,
    addresses: Iterable[str | int] | None = None,
) -> dict[str, Any]:
    """Return current artifact topology without mutating or scheduling work."""

    if isinstance(tracker_or_path, Mapping):
        tracker = tracker_or_path
    else:
        tracker = tracker_store(tracker_or_path).load()
    requested = None if artifact_ids is None else tuple(str(item) for item in artifact_ids)
    requested_addresses = (
        None
        if addresses is None
        else tuple(normalize_artifact_address(item) for item in addresses)
    )
    if requested is not None and requested_addresses is not None:
        raise SourceTraceProgressError(
            "show accepts exact artifact ids or addresses, not both"
        )
    if requested is not None and len(requested) != len(set(requested)):
        raise SourceTraceProgressError("show artifact ids must be unique")
    if requested_addresses is not None and len(requested_addresses) != len(
        set(requested_addresses)
    ):
        raise SourceTraceProgressError("show addresses must be unique")
    if requested is None and requested_addresses is None:
        artifacts = list(iter_tracker_artifacts(tracker))
    elif requested is not None:
        artifacts = [
            resolve_tracker_artifact(tracker, artifact_id)
            for artifact_id in requested
        ]
    else:
        all_artifacts = iter_tracker_artifacts(tracker)
        by_address = {
            address: [
                artifact
                for artifact in all_artifacts
                if artifact_address(artifact.artifact_id) == address
            ]
            for address in requested_addresses or ()
        }
        missing = [
            address for address, matches in by_address.items() if not matches
        ]
        if missing:
            raise SourceTraceProgressError(
                f"show addresses have no current tracker artifacts: {missing}"
            )
        artifacts = [
            artifact
            for address in requested_addresses or ()
            for artifact in by_address[address]
        ]
    rows = []
    for artifact in artifacts:
        rows.append(
            {
                "artifact_id": artifact.artifact_id,
                "location": artifact.location,
                "parent_artifact_id": artifact.parent_artifact_id,
                "output_section_id": artifact.output_section_id,
                "output_section": artifact.output_section,
                "source_traceability": (
                    normalize_source_traceability(
                        artifact.row["source_traceability"]
                    )
                    if "source_traceability" in artifact.row
                    else None
                ),
            }
        )
    return {
        "operation": "show",
        "schema_version": tracker.get("schema_version"),
        "revision": tracker.get("revision"),
        "artifact_count": len(rows),
        "artifacts": rows,
        "read_only": True,
    }


def _validate_migration_metadata(value: Any) -> dict[str, Any]:
    metadata = _require_mapping(
        value, label="migration.source_traceability_v1"
    )
    base_keys = {"version", "policy", "initialized_from_revision", "state"}
    optional_keys = {
        "unresolved_legacy_claims",
        "legacy_claim_resolutions",
    }
    if not base_keys.issubset(metadata) or set(metadata) - base_keys - optional_keys:
        raise SourceTraceProgressError(
            "migration.source_traceability_v1 keys must be the exact base "
            "metadata with optional unresolved_legacy_claims and "
            "legacy_claim_resolutions"
        )
    if metadata["version"] != 1:
        raise SourceTraceProgressError(
            "migration.source_traceability_v1.version must be 1"
        )
    if metadata["policy"] != "topology-only-no-acceptance":
        raise SourceTraceProgressError(
            "migration.source_traceability_v1.policy must be topology-only-no-acceptance"
        )
    initialized = metadata["initialized_from_revision"]
    if (
        not isinstance(initialized, int)
        or isinstance(initialized, bool)
        or initialized < 0
    ):
        raise SourceTraceProgressError(
            "migration.source_traceability_v1.initialized_from_revision "
            "must be a non-negative integer"
        )
    if metadata["state"] != "initialized":
        raise SourceTraceProgressError(
            "migration.source_traceability_v1.state must be initialized"
        )
    normalized = deepcopy(dict(metadata))
    if "unresolved_legacy_claims" in metadata:
        normalized["unresolved_legacy_claims"] = (
            normalize_unresolved_legacy_claims(
                metadata["unresolved_legacy_claims"]
            )
        )
    if "legacy_claim_resolutions" in metadata:
        if "unresolved_legacy_claims" not in metadata:
            raise SourceTraceProgressError(
                "migration.source_traceability_v1 legacy_claim_resolutions "
                "require the immutable unresolved_legacy_claims inventory"
            )
        normalized["legacy_claim_resolutions"] = (
            normalize_legacy_claim_resolutions(
                metadata["legacy_claim_resolutions"]
            )
        )
    return normalized


def normalize_unresolved_legacy_claims(value: Any) -> list[dict[str, str]]:
    if not isinstance(value, list) or not value:
        raise SourceTraceProgressError(
            "unresolved_legacy_claims must be a non-empty array when present"
        )
    claims: list[dict[str, str]] = []
    identities: set[tuple[str, str, str, str]] = set()
    for index, raw_claim in enumerate(value):
        label = f"unresolved_legacy_claims[{index}]"
        claim = _require_mapping(raw_claim, label=label)
        _require_exact_keys(
            claim,
            keys=(
                "binary",
                "kind_hint",
                "address",
                "reason_code",
                "source_path",
            ),
            label=label,
        )
        binary = claim["binary"]
        if (
            not isinstance(binary, str)
            or re.fullmatch(r"[a-z0-9][a-z0-9_-]*", binary) is None
        ):
            raise SourceTraceProgressError(
                f"{label}.binary must be an exact lowercase tracker binary id"
            )
        kind_hint = claim["kind_hint"]
        if kind_hint not in LEGACY_CLAIM_KIND_HINTS:
            raise SourceTraceProgressError(
                f"{label}.kind_hint must be function or data"
            )
        address = normalize_artifact_address(claim["address"])
        if claim["reason_code"] != "missing-artifact-identity":
            raise SourceTraceProgressError(
                f"{label}.reason_code must be missing-artifact-identity"
            )
        source_path = _normalized_translation_unit(
            claim["source_path"], label=f"{label}.source_path"
        )
        identity = (binary, kind_hint, address, source_path)
        if identity in identities:
            raise SourceTraceProgressError(
                f"{label} duplicates an unresolved legacy claim"
            )
        identities.add(identity)
        claims.append(
            {
                "binary": binary,
                "kind_hint": kind_hint,
                "address": address,
                "reason_code": "missing-artifact-identity",
                "source_path": source_path,
            }
        )
    claims.sort(
        key=lambda claim: (
            claim["binary"],
            int(claim["address"], 16),
            claim["kind_hint"],
            claim["source_path"],
        )
    )
    return claims


def _legacy_claim_identity(
    claim: Mapping[str, str],
) -> tuple[str, str, str, str]:
    return (
        claim["binary"],
        claim["kind_hint"],
        claim["address"],
        claim["source_path"],
    )


def normalize_legacy_claim_resolutions(
    value: Any,
) -> list[dict[str, Any]]:
    """Validate append-only resolutions of exact immutable migration claims."""

    if not isinstance(value, list) or not value:
        raise SourceTraceProgressError(
            "legacy_claim_resolutions must be a non-empty array when present"
        )
    records: list[dict[str, Any]] = []
    identities: set[tuple[str, str, str, str]] = set()
    for index, raw_record in enumerate(value):
        label = f"legacy_claim_resolutions[{index}]"
        record = _require_mapping(raw_record, label=label)
        _require_exact_keys(
            record,
            keys=("expected_claim", "replacement_resolution"),
            label=label,
        )
        expected_claim = normalize_unresolved_legacy_claims(
            [record["expected_claim"]]
        )[0]
        identity = _legacy_claim_identity(expected_claim)
        if identity in identities:
            raise SourceTraceProgressError(
                f"{label} duplicates a legacy claim resolution"
            )
        identities.add(identity)

        resolution = _require_mapping(
            record["replacement_resolution"],
            label=f"{label}.replacement_resolution",
        )
        kind = resolution.get("kind")
        if kind not in LEGACY_CLAIM_RESOLUTION_KINDS:
            raise SourceTraceProgressError(
                f"{label}.replacement_resolution.kind must be one of "
                f"{sorted(LEGACY_CLAIM_RESOLUTION_KINDS)}"
            )
        exact_keys = {"kind", "artifact_id"}
        if kind == "interior-of-existing-artifact":
            exact_keys |= {"reason", "evidence_ids"}
        _require_exact_keys(
            resolution,
            keys=exact_keys,
            label=f"{label}.replacement_resolution",
        )
        artifact_id = resolution["artifact_id"]
        if (
            not isinstance(artifact_id, str)
            or SOURCE_ARTIFACT_ID_RE.fullmatch(artifact_id) is None
        ):
            raise SourceTraceProgressError(
                f"{label}.replacement_resolution.artifact_id is not a "
                "supported exact source artifact id"
            )
        normalized_resolution: dict[str, Any] = {
            "kind": kind,
            "artifact_id": artifact_id,
        }
        if kind == "interior-of-existing-artifact":
            reason = resolution["reason"]
            if (
                not isinstance(reason, str)
                or not reason
                or reason.strip() != reason
            ):
                raise SourceTraceProgressError(
                    f"{label}.replacement_resolution.reason must be a "
                    "non-empty trimmed string"
                )
            evidence_ids = resolution["evidence_ids"]
            if (
                not isinstance(evidence_ids, list)
                or not evidence_ids
                or not all(
                    isinstance(item, str)
                    and EVIDENCE_ID_RE.fullmatch(item)
                    for item in evidence_ids
                )
            ):
                raise SourceTraceProgressError(
                    f"{label}.replacement_resolution.evidence_ids must be a "
                    "non-empty array of exact tracker evidence ids"
                )
            if len(evidence_ids) != len(set(evidence_ids)):
                raise SourceTraceProgressError(
                    f"{label}.replacement_resolution.evidence_ids contains "
                    "duplicates"
                )
            normalized_resolution["reason"] = reason
            normalized_resolution["evidence_ids"] = sorted(evidence_ids)
        records.append(
            {
                "expected_claim": expected_claim,
                "replacement_resolution": normalized_resolution,
            }
        )
    return records


def normalize_replace_batch_payload(value: Any) -> dict[str, Any]:
    payload = _require_mapping(value, label="replace-batch payload")
    base_keys = {"operation", "parent_reviewed", "updates"}
    optional_keys = {
        "unresolved_legacy_claims",
        "legacy_claim_resolutions",
    }
    if not base_keys.issubset(payload) or set(payload) - base_keys - optional_keys:
        raise SourceTraceProgressError(
            "replace-batch payload keys must be operation, parent_reviewed, "
            "updates, and optional unresolved_legacy_claims and "
            "legacy_claim_resolutions"
        )
    if payload["operation"] != REPLACE_BATCH_OPERATION:
        raise SourceTraceProgressError(
            f"replace-batch payload.operation must be {REPLACE_BATCH_OPERATION!r}"
        )
    if payload["parent_reviewed"] is not True:
        raise SourceTraceProgressError(
            "replace-batch requires parent_reviewed=true"
        )
    updates = payload["updates"]
    if not isinstance(updates, list):
        raise SourceTraceProgressError(
            "replace-batch payload.updates must be an array"
        )
    normalized_updates: list[dict[str, Any]] = []
    seen: set[str] = set()
    for index, raw_update in enumerate(updates):
        label = f"replace-batch payload.updates[{index}]"
        update = _require_mapping(raw_update, label=label)
        _require_exact_keys(
            update,
            keys=("artifact_id", "expected_current", "source_traceability"),
            label=label,
        )
        artifact_id = update["artifact_id"]
        if not isinstance(artifact_id, str) or SOURCE_ARTIFACT_ID_RE.fullmatch(
            artifact_id
        ) is None:
            raise SourceTraceProgressError(
                f"{label}.artifact_id is not a supported exact source artifact id"
            )
        if artifact_id in seen:
            raise SourceTraceProgressError(
                f"replace-batch contains duplicate artifact id {artifact_id!r}"
            )
        seen.add(artifact_id)
        expected_current = update["expected_current"]
        if expected_current is not None:
            expected_current = normalize_source_traceability(expected_current)
        replacement = normalize_source_traceability(
            update["source_traceability"]
        )
        normalized_updates.append(
            {
                "artifact_id": artifact_id,
                "expected_current": expected_current,
                "source_traceability": replacement,
            }
        )
    normalized = {
        "operation": REPLACE_BATCH_OPERATION,
        "parent_reviewed": True,
        "updates": normalized_updates,
    }
    if "unresolved_legacy_claims" in payload:
        normalized["unresolved_legacy_claims"] = (
            normalize_unresolved_legacy_claims(
                payload["unresolved_legacy_claims"]
            )
        )
    if "legacy_claim_resolutions" in payload:
        normalized["legacy_claim_resolutions"] = (
            normalize_legacy_claim_resolutions(
                payload["legacy_claim_resolutions"]
            )
        )
    if not normalized_updates and "legacy_claim_resolutions" not in normalized:
        raise SourceTraceProgressError(
            "replace-batch requires at least one topology update or legacy "
            "claim resolution"
        )
    return normalized


def _artifact_binary_and_kind(artifact: TrackerArtifact) -> tuple[str, str]:
    binary, encoded_kind, *_ = artifact.artifact_id.split(":")
    kind = encoded_kind.removeprefix("logical-")
    return binary, kind


def _validate_legacy_claim_resolution(
    tracker: Mapping[str, Any],
    record: Mapping[str, Any],
) -> None:
    claim = record["expected_claim"]
    resolution = record["replacement_resolution"]
    artifact = resolve_tracker_artifact(tracker, resolution["artifact_id"])
    binary, kind = _artifact_binary_and_kind(artifact)
    if binary != claim["binary"] or kind != claim["kind_hint"]:
        raise SourceTraceProgressError(
            f"legacy claim {claim!r} cannot resolve to "
            f"{artifact.artifact_id!r}: binary/kind mismatch"
        )

    claim_address = int(claim["address"], 16)
    artifact_start = int(artifact_address(artifact.artifact_id), 16)
    if resolution["kind"] == "exact-existing-artifact":
        if claim_address != artifact_start:
            raise SourceTraceProgressError(
                f"legacy claim {claim!r} exact resolution address does not "
                f"match existing artifact {artifact.artifact_id!r}"
            )
        return

    if artifact.parent_artifact_id is not None:
        raise SourceTraceProgressError(
            f"legacy claim {claim!r} interior resolution requires a physical "
            "artifact with an exact extent, not a logical alias"
        )
    row = artifact.row
    if row.get("extent_state") != "known":
        raise SourceTraceProgressError(
            f"legacy claim {claim!r} interior resolution target "
            f"{artifact.artifact_id!r} does not have a known exact extent"
        )
    try:
        row_start = int(normalize_artifact_address(row["address"]), 16)
        row_end = int(normalize_artifact_address(row["end_exclusive"]), 16)
        row_size = row["size"]
    except KeyError as exc:
        raise SourceTraceProgressError(
            f"legacy claim {claim!r} interior resolution target "
            f"{artifact.artifact_id!r} lacks exact address/end/size fields"
        ) from exc
    if (
        isinstance(row_size, bool)
        or not isinstance(row_size, int)
        or row_size <= 0
        or row_start != artifact_start
        or row_start + row_size != row_end
    ):
        raise SourceTraceProgressError(
            f"legacy claim {claim!r} interior resolution target "
            f"{artifact.artifact_id!r} has an inconsistent exact extent"
        )
    if not row_start < claim_address < row_end:
        raise SourceTraceProgressError(
            f"legacy claim {claim!r} is not strictly interior to existing "
            f"artifact {artifact.artifact_id!r} extent "
            f"[0x{row_start:x},0x{row_end:x})"
        )
    evidence = _require_mapping(tracker.get("evidence"), label="tracker.evidence")
    missing = [
        evidence_id
        for evidence_id in resolution["evidence_ids"]
        if evidence_id not in evidence
    ]
    if missing:
        raise SourceTraceProgressError(
            f"legacy claim {claim!r} interior resolution references unknown "
            f"evidence ids: {missing}"
        )


def plan_source_traceability_batch(
    tracker: Mapping[str, Any],
    payload: Mapping[str, Any],
    *,
    expected_revision: int,
) -> SourceTraceabilityBatchPlan:
    """Plan an exact-current topology-only replacement batch."""

    if (
        not isinstance(expected_revision, int)
        or isinstance(expected_revision, bool)
        or expected_revision < 0
    ):
        raise SourceTraceProgressError(
            "expected_revision must be a non-negative integer"
        )
    if tracker.get("schema_version") != TRACKER_SCHEMA_VERSION:
        raise SourceTraceProgressError(
            f"tracker schema_version must remain {TRACKER_SCHEMA_VERSION}"
        )
    if tracker.get("revision") != expected_revision:
        raise ConcurrentRevisionUpdate(
            f"revision changed: expected {expected_revision}, "
            f"found {tracker.get('revision')}"
        )
    normalized_payload = normalize_replace_batch_payload(payload)
    proposed = deepcopy(dict(tracker))
    mutable = {
        artifact.artifact_id: row
        for artifact, row in _iter_tracker_artifacts_mutable(proposed)
    }
    evidence = _require_mapping(proposed.get("evidence"), label="tracker.evidence")
    changed: list[str] = []
    for update in normalized_payload["updates"]:
        artifact_id = update["artifact_id"]
        row = mutable.get(artifact_id)
        if row is None:
            raise SourceTraceProgressError(
                f"source artifact id {artifact_id!r} does not exist; "
                "replace-batch never creates tracker artifacts"
            )
        current = (
            normalize_source_traceability(row["source_traceability"])
            if "source_traceability" in row
            else None
        )
        if current != update["expected_current"]:
            raise SourceTraceProgressError(
                f"source artifact {artifact_id!r} expected_current is stale: "
                f"expected {update['expected_current']!r}, found {current!r}"
            )
        replacement = update["source_traceability"]
        for edge in replacement["source_edges"]:
            missing = [
                evidence_id
                for evidence_id in edge["evidence_ids"]
                if evidence_id not in evidence
            ]
            if missing:
                raise SourceTraceProgressError(
                    f"source artifact {artifact_id!r} references unknown "
                    f"evidence ids: {missing}"
                )
        if current == replacement:
            raise SourceTraceProgressError(
                f"source artifact {artifact_id!r} already has the requested state"
            )
        row["source_traceability"] = deepcopy(replacement)
        changed.append(artifact_id)

    migration = proposed.get("migration")
    if not isinstance(migration, dict):
        raise SourceTraceProgressError("tracker.migration must be an object")
    existing_metadata = migration.get("source_traceability_v1")
    initializes = existing_metadata is None
    if initializes:
        if "legacy_claim_resolutions" in normalized_payload:
            raise SourceTraceProgressError(
                "legacy_claim_resolutions require an already initialized "
                "immutable unresolved_legacy_claims inventory"
            )
        metadata = {
            **MIGRATION_METADATA,
            "initialized_from_revision": expected_revision,
        }
        if "unresolved_legacy_claims" in normalized_payload:
            metadata["unresolved_legacy_claims"] = deepcopy(
                normalized_payload["unresolved_legacy_claims"]
            )
        migration["source_traceability_v1"] = deepcopy(metadata)
    else:
        metadata = _validate_migration_metadata(existing_metadata)
        requested_claims = normalized_payload.get("unresolved_legacy_claims")
        if (
            requested_claims is not None
            and metadata.get("unresolved_legacy_claims") != requested_claims
        ):
            raise SourceTraceProgressError(
                "unresolved_legacy_claims differ from the initialized exact "
                "migration metadata"
            )
        if migration["source_traceability_v1"] != metadata:
            raise SourceTraceProgressError(
                "migration.source_traceability_v1 must be preserved exactly"
            )
        requested_resolutions = normalized_payload.get(
            "legacy_claim_resolutions", []
        )
        if requested_resolutions:
            immutable_claims = metadata.get("unresolved_legacy_claims")
            if immutable_claims is None:
                raise SourceTraceProgressError(
                    "legacy_claim_resolutions require the immutable "
                    "unresolved_legacy_claims inventory"
                )
            claim_identities = {
                _legacy_claim_identity(claim)
                for claim in immutable_claims
            }
            existing_resolutions = metadata.get(
                "legacy_claim_resolutions", []
            )
            resolved_identities = {
                _legacy_claim_identity(record["expected_claim"])
                for record in existing_resolutions
            }
            for record in requested_resolutions:
                claim = record["expected_claim"]
                identity = _legacy_claim_identity(claim)
                if identity not in claim_identities:
                    raise SourceTraceProgressError(
                        f"legacy claim {claim!r} is stale or absent from the "
                        "immutable unresolved_legacy_claims inventory"
                    )
                if identity in resolved_identities:
                    raise SourceTraceProgressError(
                        f"legacy claim {claim!r} already has an immutable "
                        "resolution record"
                    )
                _validate_legacy_claim_resolution(proposed, record)
                resolved_identities.add(identity)
            metadata["legacy_claim_resolutions"] = [
                *deepcopy(existing_resolutions),
                *deepcopy(requested_resolutions),
            ]
            migration["source_traceability_v1"] = deepcopy(metadata)

    return SourceTraceabilityBatchPlan(
        expected_revision=expected_revision,
        proposed_revision=expected_revision + 1,
        artifact_ids=tuple(changed),
        legacy_claim_resolution_count=len(
            normalized_payload.get("legacy_claim_resolutions", [])
        ),
        initializes_migration=initializes,
        migration_metadata=metadata,
        proposed=proposed,
    )


class SourceTraceTrackerStore:
    """Mapping-shaped adapter over the canonical SQLite-aware progress store.

    Source-trace planning deliberately operates on plain mappings.  Keep that
    semantic surface while routing durable reads and CAS commits through
    ``ProgressStore``, which materializes ``.sqlite3`` authorities instead of
    sending their bytes to the legacy JSON decoder.
    """

    def __init__(self, path: str | Path) -> None:
        self._store = ProgressStore(path)

    def load(self) -> dict[str, Any]:
        try:
            return self._store.load().data
        except ProgressError as exc:
            raise LiveProgressError(str(exc)) from exc

    def commit(
        self,
        proposed: Mapping[str, Any],
        *,
        expected_revision: int,
        apply: bool,
    ) -> Any:
        try:
            return self._store.commit(
                proposed,
                expected_revision=expected_revision,
                apply=apply,
            )
        except ConcurrentProgressUpdate as exc:
            raise ConcurrentRevisionUpdate(str(exc)) from exc
        except ProgressError as exc:
            raise LiveProgressError(str(exc)) from exc


def tracker_store(path: str | Path) -> SourceTraceTrackerStore:
    return SourceTraceTrackerStore(path)


def mutate_source_traceability_batch(
    store_or_path: RevisionStore | SourceTraceTrackerStore | str | Path,
    payload: Mapping[str, Any],
    *,
    expected_revision: int,
    apply: bool,
) -> dict[str, Any]:
    """Dry-run or CAS-apply a parent-reviewed exact replacement batch."""

    store = (
        store_or_path
        if isinstance(store_or_path, (RevisionStore, SourceTraceTrackerStore))
        else tracker_store(store_or_path)
    )
    current = store.load()
    plan = plan_source_traceability_batch(
        current, payload, expected_revision=expected_revision
    )
    commit = store.commit(
        plan.proposed, expected_revision=expected_revision, apply=apply
    )
    return {
        **plan.to_dict(),
        **commit.to_dict(),
        "schema_version": TRACKER_SCHEMA_VERSION,
        "topology_only": True,
        "acceptance_changed": False,
    }


def load_replace_batch_payload(
    *, payload_json: str | None = None, payload_file: str | Path | None = None
) -> dict[str, Any]:
    if (payload_json is None) == (payload_file is None):
        raise SourceTraceProgressError(
            "provide exactly one of --payload-json or --payload-file"
        )
    if payload_file is not None:
        path = Path(payload_file)
        try:
            text = path.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError) as exc:
            raise SourceTraceProgressError(
                f"cannot read replace-batch payload file {path}: {exc}"
            ) from exc
    else:
        text = str(payload_json)
    try:
        value = json.loads(text)
    except json.JSONDecodeError as exc:
        raise SourceTraceProgressError(
            f"replace-batch payload is invalid JSON: {exc}"
        ) from exc
    return normalize_replace_batch_payload(value)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Inspect or replace Source Traceability v1 tracker topology."
    )
    subparsers = parser.add_subparsers(dest="operation", required=True)

    show = subparsers.add_parser(
        "show", help="Read current physical/logical artifact traceability."
    )
    show.add_argument(
        "--progress",
        "--tracker",
        dest="tracker",
        default=str(DEFAULT_TRACKER),
    )
    lookup_group = show.add_mutually_exclusive_group()
    lookup_group.add_argument("--artifact-id", action="append", default=[])
    lookup_group.add_argument("--address", action="append", default=[])
    show.add_argument("--json", action="store_true")

    replace = subparsers.add_parser(
        REPLACE_BATCH_OPERATION,
        help=(
            "Parent-reviewed exact-current dry-run/apply topology replacement "
            "and append-only legacy-claim resolution batch."
        ),
    )
    replace.add_argument(
        "--progress",
        "--tracker",
        dest="tracker",
        default=str(DEFAULT_TRACKER),
    )
    replace.add_argument("--expected-revision", type=int, required=True)
    payload_group = replace.add_mutually_exclusive_group(required=True)
    payload_group.add_argument("--payload-json")
    payload_group.add_argument("--payload-file")
    mode_group = replace.add_mutually_exclusive_group()
    mode_group.add_argument("--dry-run", action="store_true")
    mode_group.add_argument("--apply", action="store_true")
    replace.add_argument("--json", action="store_true")
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        if args.operation == "show":
            result = show_source_traceability(
                args.tracker,
                artifact_ids=args.artifact_id or None,
                addresses=args.address or None,
            )
        else:
            payload = load_replace_batch_payload(
                payload_json=args.payload_json, payload_file=args.payload_file
            )
            result = mutate_source_traceability_batch(
                args.tracker,
                payload,
                expected_revision=args.expected_revision,
                apply=bool(args.apply),
            )
    except (
        SourceTraceProgressError,
        ConcurrentRevisionUpdate,
        LiveProgressError,
    ) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
