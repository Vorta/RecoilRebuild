#!/usr/bin/env python3
"""Register an exact extent on one existing physical data artifact."""

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

from _recoil.lib.live_progress import (
    TRACKER_SCHEMA_VERSION,
    ConcurrentRevisionUpdate,
    LiveProgressError,
)
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH
from _recoil.lib.tooling import configure_stdio
from _recoil.commands.source_trace_progress import (
    EVIDENCE_ID_RE,
    SOURCE_ARTIFACT_ID_RE,
    SourceTraceProgressError,
    normalize_artifact_address,
    resolve_tracker_artifact,
    tracker_store,
)


DEFAULT_TRACKER = DEFAULT_PROGRESS_PATH
REGISTER_OPERATION = "register-existing-data-extent"


class DataExtentProgressError(ValueError):
    """A fail-closed reviewed data-extent registration error."""


@dataclass(frozen=True)
class DataExtentPlan:
    expected_revision: int
    proposed_revision: int
    artifact_id: str
    address: str
    size: int
    end_exclusive: str
    evidence_ids: tuple[str, ...]
    output_section_id: str
    output_section_start: str
    output_section_end_exclusive: str
    proposed: Mapping[str, Any]

    def to_dict(self) -> dict[str, Any]:
        return {
            "operation": REGISTER_OPERATION,
            "expected_revision": self.expected_revision,
            "proposed_revision": self.proposed_revision,
            "artifact_id": self.artifact_id,
            "extent": {
                "state": "known",
                "address": self.address,
                "size": self.size,
                "end_exclusive": self.end_exclusive,
                "evidence_ids": list(self.evidence_ids),
            },
            "output_section": {
                "id": self.output_section_id,
                "start": self.output_section_start,
                "end_exclusive": self.output_section_end_exclusive,
            },
        }


def _require_mapping(value: Any, *, label: str) -> Mapping[str, Any]:
    if not isinstance(value, Mapping):
        raise DataExtentProgressError(f"{label} must be an object")
    return value


def _require_exact_keys(
    value: Mapping[str, Any], *, keys: tuple[str, ...], label: str
) -> None:
    expected = set(keys)
    actual = set(value)
    if actual != expected:
        raise DataExtentProgressError(
            f"{label} keys must be exactly {sorted(expected)}; "
            f"missing={sorted(expected - actual)}, extra={sorted(actual - expected)}"
        )


def _nonnegative_integer(value: Any, *, label: str) -> int:
    if isinstance(value, bool):
        raise DataExtentProgressError(f"{label} must not be boolean")
    if isinstance(value, int):
        result = value
    elif isinstance(value, str):
        try:
            result = int(value, 0)
        except ValueError as exc:
            raise DataExtentProgressError(
                f"{label} must be a non-negative integer"
            ) from exc
    else:
        raise DataExtentProgressError(f"{label} must be a non-negative integer")
    if result < 0:
        raise DataExtentProgressError(f"{label} must be a non-negative integer")
    return result


def normalize_register_payload(value: Any) -> dict[str, Any]:
    payload = _require_mapping(value, label="data-extent payload")
    _require_exact_keys(
        payload,
        keys=(
            "operation",
            "parent_reviewed",
            "artifact_id",
            "expected_current",
            "replacement",
        ),
        label="data-extent payload",
    )
    if payload["operation"] != REGISTER_OPERATION:
        raise DataExtentProgressError(
            f"data-extent payload.operation must be {REGISTER_OPERATION!r}"
        )
    if payload["parent_reviewed"] is not True:
        raise DataExtentProgressError(
            "data-extent registration requires parent_reviewed=true"
        )
    artifact_id = payload["artifact_id"]
    if (
        not isinstance(artifact_id, str)
        or SOURCE_ARTIFACT_ID_RE.fullmatch(artifact_id) is None
        or ":data:" not in artifact_id
        or ":logical-data:" in artifact_id
    ):
        raise DataExtentProgressError(
            "data-extent artifact_id must be one exact physical "
            "<binary>:data:<address> artifact id"
        )

    expected = _require_mapping(
        payload["expected_current"], label="data-extent expected_current"
    )
    _require_exact_keys(
        expected,
        keys=("extent_state", "address"),
        label="data-extent expected_current",
    )
    if expected["extent_state"] != "unknown":
        raise DataExtentProgressError(
            "data-extent expected_current.extent_state must be 'unknown'"
        )
    expected_address = normalize_artifact_address(expected["address"])

    replacement = _require_mapping(
        payload["replacement"], label="data-extent replacement"
    )
    _require_exact_keys(
        replacement,
        keys=("extent_state", "size", "end_exclusive", "evidence_ids"),
        label="data-extent replacement",
    )
    if replacement["extent_state"] != "known":
        raise DataExtentProgressError(
            "data-extent replacement.extent_state must be 'known'"
        )
    size = _nonnegative_integer(
        replacement["size"], label="data-extent replacement.size"
    )
    if size == 0:
        raise DataExtentProgressError(
            "data-extent replacement.size must be positive"
        )
    end_exclusive = normalize_artifact_address(replacement["end_exclusive"])
    evidence_ids = replacement["evidence_ids"]
    if (
        not isinstance(evidence_ids, list)
        or not evidence_ids
        or not all(
            isinstance(item, str) and EVIDENCE_ID_RE.fullmatch(item)
            for item in evidence_ids
        )
    ):
        raise DataExtentProgressError(
            "data-extent replacement.evidence_ids must be a non-empty array "
            "of exact existing tracker evidence ids"
        )
    if len(evidence_ids) != len(set(evidence_ids)):
        raise DataExtentProgressError(
            "data-extent replacement.evidence_ids contains duplicates"
        )
    return {
        "operation": REGISTER_OPERATION,
        "parent_reviewed": True,
        "artifact_id": artifact_id,
        "expected_current": {
            "extent_state": "unknown",
            "address": expected_address,
        },
        "replacement": {
            "extent_state": "known",
            "size": size,
            "end_exclusive": end_exclusive,
            "evidence_ids": sorted(evidence_ids),
        },
    }


def _section_extent(
    tracker: Mapping[str, Any],
    *,
    artifact_id: str,
    output_section_id: str,
) -> tuple[int, int]:
    sections = _require_mapping(
        tracker.get("output_sections"), label="tracker.output_sections"
    )
    section = _require_mapping(
        sections.get(output_section_id),
        label=f"tracker.output_sections[{output_section_id!r}]",
    )
    binary = artifact_id.split(":", 1)[0]
    if section.get("binary") != binary:
        raise DataExtentProgressError(
            f"output section {output_section_id!r} does not belong to "
            f"artifact binary {binary!r}"
        )
    reference = _require_mapping(
        section.get("reference"),
        label=f"tracker.output_sections[{output_section_id!r}].reference",
    )
    if "image_address" not in reference or "virtual_size" not in reference:
        raise DataExtentProgressError(
            f"output section {output_section_id!r} lacks exact retail "
            "image_address/virtual_size containment facts"
        )
    start = int(normalize_artifact_address(reference["image_address"]), 16)
    size = _nonnegative_integer(
        reference["virtual_size"],
        label=f"tracker.output_sections[{output_section_id!r}].reference.virtual_size",
    )
    if size == 0:
        raise DataExtentProgressError(
            f"output section {output_section_id!r} has an empty retail extent"
        )
    return start, start + size


def plan_data_extent_registration(
    tracker: Mapping[str, Any],
    payload: Mapping[str, Any],
    *,
    expected_revision: int,
) -> DataExtentPlan:
    if (
        not isinstance(expected_revision, int)
        or isinstance(expected_revision, bool)
        or expected_revision < 0
    ):
        raise DataExtentProgressError(
            "expected_revision must be a non-negative integer"
        )
    if tracker.get("schema_version") != TRACKER_SCHEMA_VERSION:
        raise DataExtentProgressError(
            f"tracker schema_version must remain {TRACKER_SCHEMA_VERSION}"
        )
    if tracker.get("revision") != expected_revision:
        raise ConcurrentRevisionUpdate(
            f"revision changed: expected {expected_revision}, "
            f"found {tracker.get('revision')}"
        )
    normalized = normalize_register_payload(payload)
    artifact_id = normalized["artifact_id"]
    try:
        artifact = resolve_tracker_artifact(tracker, artifact_id)
    except SourceTraceProgressError as exc:
        raise DataExtentProgressError(str(exc)) from exc
    if artifact.parent_artifact_id is not None or artifact.collection != "symbols":
        raise DataExtentProgressError(
            "data-extent registration requires one existing physical symbol row"
        )
    row = artifact.row
    if row.get("kind") != "data":
        raise DataExtentProgressError(
            f"artifact {artifact_id!r} is not an existing data row"
        )
    current_address = normalize_artifact_address(row.get("address"))
    expected_current = normalized["expected_current"]
    if (
        row.get("extent_state") != expected_current["extent_state"]
        or current_address != expected_current["address"]
        or "size" in row
        or "end_exclusive" in row
    ):
        raise DataExtentProgressError(
            f"artifact {artifact_id!r} exact current extent is stale or is "
            "not an unknown extent with omitted size/end"
        )
    if normalize_artifact_address(artifact_id.split(":")[2]) != current_address:
        raise DataExtentProgressError(
            f"artifact {artifact_id!r} id/address identity is inconsistent"
        )

    replacement = normalized["replacement"]
    start = int(current_address, 16)
    end = int(replacement["end_exclusive"], 16)
    if start + replacement["size"] != end:
        raise DataExtentProgressError(
            f"artifact {artifact_id!r} replacement end_exclusive does not "
            "equal address + size"
        )
    evidence = _require_mapping(tracker.get("evidence"), label="tracker.evidence")
    missing_evidence = [
        evidence_id
        for evidence_id in replacement["evidence_ids"]
        if evidence_id not in evidence
    ]
    if missing_evidence:
        raise DataExtentProgressError(
            f"artifact {artifact_id!r} replacement references unknown evidence "
            f"ids: {missing_evidence}"
        )

    output_section_id = row.get("output_section_id")
    if not isinstance(output_section_id, str) or not output_section_id:
        raise DataExtentProgressError(
            f"artifact {artifact_id!r} lacks one exact output_section_id"
        )
    section_start, section_end = _section_extent(
        tracker,
        artifact_id=artifact_id,
        output_section_id=output_section_id,
    )
    if not section_start <= start < end <= section_end:
        raise DataExtentProgressError(
            f"artifact {artifact_id!r} replacement extent "
            f"[0x{start:x},0x{end:x}) is outside retail output section "
            f"{output_section_id!r} [0x{section_start:x},0x{section_end:x})"
        )

    proposed = deepcopy(dict(tracker))
    mutable_row = proposed["symbols"][artifact_id]
    mutable_row["extent_state"] = "known"
    mutable_row["size"] = replacement["size"]
    mutable_row["end_exclusive"] = replacement["end_exclusive"]
    current_evidence = mutable_row.get("evidence_ids", [])
    if not isinstance(current_evidence, list) or not all(
        isinstance(item, str) for item in current_evidence
    ):
        raise DataExtentProgressError(
            f"artifact {artifact_id!r}.evidence_ids must be an array"
        )
    mutable_row["evidence_ids"] = list(
        dict.fromkeys([*current_evidence, *replacement["evidence_ids"]])
    )

    return DataExtentPlan(
        expected_revision=expected_revision,
        proposed_revision=expected_revision + 1,
        artifact_id=artifact_id,
        address=current_address,
        size=replacement["size"],
        end_exclusive=replacement["end_exclusive"],
        evidence_ids=tuple(replacement["evidence_ids"]),
        output_section_id=output_section_id,
        output_section_start=f"0x{section_start:x}",
        output_section_end_exclusive=f"0x{section_end:x}",
        proposed=proposed,
    )


def mutate_data_extent(
    tracker_path: str | Path,
    payload: Mapping[str, Any],
    *,
    expected_revision: int,
    apply: bool,
) -> dict[str, Any]:
    store = tracker_store(tracker_path)
    current = store.load()
    plan = plan_data_extent_registration(
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
        "artifact_created": False,
        "source_edges_changed": False,
    }


def load_payload(
    *, payload_json: str | None = None, payload_file: str | Path | None = None
) -> dict[str, Any]:
    if (payload_json is None) == (payload_file is None):
        raise DataExtentProgressError(
            "provide exactly one of --payload-json or --payload-file"
        )
    if payload_file is not None:
        path = Path(payload_file)
        try:
            text = path.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError) as exc:
            raise DataExtentProgressError(
                f"cannot read data-extent payload file {path}: {exc}"
            ) from exc
    else:
        text = str(payload_json)
    try:
        return normalize_register_payload(json.loads(text))
    except json.JSONDecodeError as exc:
        raise DataExtentProgressError(
            f"data-extent payload is invalid JSON: {exc}"
        ) from exc


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Parent-only revision-guarded exact extent registration for one "
            "existing physical data artifact."
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
        result = mutate_data_extent(
            args.tracker,
            payload,
            expected_revision=args.expected_revision,
            apply=bool(args.apply),
        )
    except (
        DataExtentProgressError,
        ConcurrentRevisionUpdate,
        LiveProgressError,
        SourceTraceProgressError,
    ) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
