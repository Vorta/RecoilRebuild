#!/usr/bin/env python3
"""Repair one invalid reviewed data-artifact observation schema pair."""

from __future__ import annotations

import argparse
from copy import deepcopy
from dataclasses import dataclass
import json
from pathlib import Path
import sys
from typing import Any, Mapping

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from _recoil.commands.data_artifact_progress import (
    DEFAULT_TRACKER,
    REVIEWED_EVIDENCE_KIND,
)
from _recoil.commands.data_extent_progress import (
    _require_exact_keys,
    _require_mapping,
)
from _recoil.commands.source_trace_progress import (
    EVIDENCE_ID_RE,
    SOURCE_ARTIFACT_ID_RE,
    tracker_store,
)
from _recoil.lib.live_progress import (
    TRACKER_SCHEMA_VERSION,
    ConcurrentRevisionUpdate,
    LiveProgressError,
)
from _recoil.lib.tooling import configure_stdio


REPAIR_OPERATION = "repair-reviewed-data-artifact-observation-schema"
INVALID_FRESHNESS = "current-unhashed"
INVALID_VALIDATION_MODE = "reviewed-non-gating-observation"
REPAIRED_FRESHNESS = "historical"
REPAIRED_VALIDATION_MODE = "historical-observation"


class DataArtifactEvidenceRepairError(ValueError):
    """A fail-closed reviewed observation schema-repair error."""


@dataclass(frozen=True)
class DataArtifactEvidenceRepairPlan:
    expected_revision: int
    proposed_revision: int
    artifact_id: str
    evidence_id: str
    before: Mapping[str, str]
    after: Mapping[str, str]
    proposed: Mapping[str, Any]

    def to_dict(self) -> dict[str, Any]:
        return {
            "operation": REPAIR_OPERATION,
            "expected_revision": self.expected_revision,
            "proposed_revision": self.proposed_revision,
            "artifact_id": self.artifact_id,
            "evidence_id": self.evidence_id,
            "before": dict(self.before),
            "after": dict(self.after),
        }


def normalize_repair_payload(value: Any) -> dict[str, Any]:
    payload = _require_mapping(
        value, label="data-artifact evidence repair payload"
    )
    _require_exact_keys(
        payload,
        keys=(
            "operation",
            "reviewed",
            "artifact_id",
            "evidence_id",
            "expected_invalid",
        ),
        label="data-artifact evidence repair payload",
    )
    if payload["operation"] != REPAIR_OPERATION:
        raise DataArtifactEvidenceRepairError(
            f"repair payload.operation must be {REPAIR_OPERATION!r}"
        )
    if payload["reviewed"] is not True:
        raise DataArtifactEvidenceRepairError(
            "data-artifact evidence repair requires reviewed=true"
        )
    artifact_id = payload["artifact_id"]
    if (
        not isinstance(artifact_id, str)
        or SOURCE_ARTIFACT_ID_RE.fullmatch(artifact_id) is None
        or ":data:" not in artifact_id
        or ":logical-data:" in artifact_id
    ):
        raise DataArtifactEvidenceRepairError(
            "artifact_id must be one exact physical data artifact id"
        )
    evidence_id = payload["evidence_id"]
    if (
        not isinstance(evidence_id, str)
        or EVIDENCE_ID_RE.fullmatch(evidence_id) is None
    ):
        raise DataArtifactEvidenceRepairError(
            "evidence_id must be one exact revision-scoped evidence id"
        )
    expected_invalid = _require_mapping(
        payload["expected_invalid"],
        label="data-artifact evidence repair expected_invalid",
    )
    _require_exact_keys(
        expected_invalid,
        keys=("freshness", "validation_mode"),
        label="data-artifact evidence repair expected_invalid",
    )
    expected = {
        "freshness": INVALID_FRESHNESS,
        "validation_mode": INVALID_VALIDATION_MODE,
    }
    if dict(expected_invalid) != expected:
        raise DataArtifactEvidenceRepairError(
            "expected_invalid must exactly name the known invalid "
            "current-unhashed/reviewed-non-gating-observation pair"
        )
    return {
        "operation": REPAIR_OPERATION,
        "reviewed": True,
        "artifact_id": artifact_id,
        "evidence_id": evidence_id,
        "expected_invalid": expected,
    }


def plan_data_artifact_evidence_repair(
    tracker: Mapping[str, Any],
    payload: Mapping[str, Any],
    *,
    expected_revision: int,
) -> DataArtifactEvidenceRepairPlan:
    if (
        not isinstance(expected_revision, int)
        or isinstance(expected_revision, bool)
        or expected_revision < 0
    ):
        raise DataArtifactEvidenceRepairError(
            "expected_revision must be a non-negative integer"
        )
    if tracker.get("schema_version") != TRACKER_SCHEMA_VERSION:
        raise DataArtifactEvidenceRepairError(
            f"tracker schema_version must remain {TRACKER_SCHEMA_VERSION}"
        )
    if tracker.get("revision") != expected_revision:
        raise ConcurrentRevisionUpdate(
            f"revision changed: expected {expected_revision}, "
            f"found {tracker.get('revision')}"
        )
    normalized = normalize_repair_payload(payload)
    artifact_id = normalized["artifact_id"]
    evidence_id = normalized["evidence_id"]
    symbols = _require_mapping(tracker.get("symbols"), label="tracker.symbols")
    artifact = symbols.get(artifact_id)
    if not isinstance(artifact, Mapping) or artifact.get("kind") != "data":
        raise DataArtifactEvidenceRepairError(
            f"physical data artifact {artifact_id!r} does not exist"
        )
    artifact_evidence_ids = artifact.get("evidence_ids")
    if (
        not isinstance(artifact_evidence_ids, list)
        or evidence_id not in artifact_evidence_ids
    ):
        raise DataArtifactEvidenceRepairError(
            f"artifact {artifact_id!r} does not reference evidence "
            f"{evidence_id!r}"
        )
    evidence_rows = _require_mapping(
        tracker.get("evidence"), label="tracker.evidence"
    )
    evidence = evidence_rows.get(evidence_id)
    if not isinstance(evidence, Mapping):
        raise DataArtifactEvidenceRepairError(
            f"evidence {evidence_id!r} does not exist"
        )
    required_semantics = {
        "kind": REVIEWED_EVIDENCE_KIND,
        "scope_ids": [artifact_id],
        "result": "observed",
        "disposition": "observed",
        "gating": False,
    }
    drift = {
        key: {"expected": expected, "found": evidence.get(key)}
        for key, expected in required_semantics.items()
        if evidence.get(key) != expected
    }
    provenance = evidence.get("provenance")
    if not isinstance(provenance, Mapping):
        drift["provenance"] = {
            "expected": "mapping with acceptance_effect='none'",
            "found": provenance,
        }
    elif provenance.get("acceptance_effect") != "none":
        drift["provenance.acceptance_effect"] = {
            "expected": "none",
            "found": provenance.get("acceptance_effect"),
        }
    if drift:
        raise DataArtifactEvidenceRepairError(
            "reviewed observation semantic identity drifted; refusing schema "
            f"repair: {drift}"
        )
    before = {
        "freshness": evidence.get("freshness"),
        "validation_mode": evidence.get("validation_mode"),
    }
    if before != normalized["expected_invalid"]:
        raise DataArtifactEvidenceRepairError(
            "reviewed observation no longer has the exact expected invalid "
            f"schema pair: expected {normalized['expected_invalid']}, found {before}"
        )

    proposed = deepcopy(dict(tracker))
    mutable_evidence = proposed["evidence"][evidence_id]
    mutable_evidence["freshness"] = REPAIRED_FRESHNESS
    mutable_evidence["validation_mode"] = REPAIRED_VALIDATION_MODE
    after = {
        "freshness": REPAIRED_FRESHNESS,
        "validation_mode": REPAIRED_VALIDATION_MODE,
    }
    return DataArtifactEvidenceRepairPlan(
        expected_revision=expected_revision,
        proposed_revision=expected_revision + 1,
        artifact_id=artifact_id,
        evidence_id=evidence_id,
        before=before,
        after=after,
        proposed=proposed,
    )


def mutate_data_artifact_evidence_repair(
    tracker_path: str | Path,
    payload: Mapping[str, Any],
    *,
    expected_revision: int,
    apply: bool,
) -> dict[str, Any]:
    store = tracker_store(tracker_path)
    current = store.load()
    plan = plan_data_artifact_evidence_repair(
        current,
        payload,
        expected_revision=expected_revision,
    )
    commit = store.commit(
        plan.proposed,
        expected_revision=expected_revision,
        apply=apply,
    )
    return {
        **plan.to_dict(),
        **commit.to_dict(),
        "schema_version": TRACKER_SCHEMA_VERSION,
        "metadata_only": True,
        "acceptance_changed": False,
        "source_edges_changed": False,
        "owner_gates_changed": False,
        "owner_tiers_changed": False,
    }


def load_payload(
    *, payload_json: str | None = None, payload_file: str | Path | None = None
) -> dict[str, Any]:
    if (payload_json is None) == (payload_file is None):
        raise DataArtifactEvidenceRepairError(
            "provide exactly one of --payload-json or --payload-file"
        )
    if payload_file is not None:
        path = Path(payload_file)
        try:
            text = path.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError) as exc:
            raise DataArtifactEvidenceRepairError(
                f"cannot read evidence repair payload file {path}: {exc}"
            ) from exc
    else:
        text = str(payload_json)
    try:
        return normalize_repair_payload(json.loads(text))
    except json.JSONDecodeError as exc:
        raise DataArtifactEvidenceRepairError(
            f"evidence repair payload is invalid JSON: {exc}"
        ) from exc


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Parent-only revision-guarded repair of the known invalid schema "
            "pair on one reviewed non-gating data-artifact observation."
        )
    )
    parser.add_argument("--tracker", default=str(DEFAULT_TRACKER))
    parser.add_argument("--expected-revision", type=int, required=True)
    payload_group = parser.add_mutually_exclusive_group(required=True)
    payload_group.add_argument("--payload-json")
    payload_group.add_argument("--payload-file")
    mode_group = parser.add_mutually_exclusive_group()
    mode_group.add_argument("--dry-run", action="store_true")
    mode_group.add_argument("--apply", action="store_true")
    parser.add_argument("--json", action="store_true")
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    try:
        payload = load_payload(
            payload_json=args.payload_json,
            payload_file=args.payload_file,
        )
        result = mutate_data_artifact_evidence_repair(
            args.tracker,
            payload,
            expected_revision=args.expected_revision,
            apply=bool(args.apply),
        )
    except (
        DataArtifactEvidenceRepairError,
        ConcurrentRevisionUpdate,
        LiveProgressError,
    ) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
