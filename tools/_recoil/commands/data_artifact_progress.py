#!/usr/bin/env python3
"""Register one reviewed exact physical data identity without acceptance."""

from __future__ import annotations

import argparse
from copy import deepcopy
from dataclasses import dataclass
import json
from pathlib import Path
import re
import sys
from typing import Any, Mapping

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from _recoil.commands.data_extent_progress import (
    DataExtentProgressError,
    _nonnegative_integer,
    _require_exact_keys,
    _require_mapping,
    _section_extent,
)
from _recoil.commands.source_trace_progress import (
    EVIDENCE_ID_RE,
    SOURCE_ARTIFACT_ID_RE,
    SourceTraceProgressError,
    normalize_artifact_address,
    tracker_store,
)
from _recoil.lib.live_progress import (
    TRACKER_SCHEMA_VERSION,
    ConcurrentRevisionUpdate,
    LiveProgressError,
)
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH, ProgressError
from _recoil.commands.progress_v2 import allocate_evidence_id
from _recoil.lib.tooling import REPO_ROOT, configure_stdio


DEFAULT_TRACKER = DEFAULT_PROGRESS_PATH
REGISTER_OPERATION = "register-exact-data-artifact"
REVIEWED_EVIDENCE_KIND = "reviewed-data-artifact-observation"
REVIEWED_EVIDENCE_METHOD = "immutable-retail-bn-plus-vc5sp3-listing"
PENDING_BINARY_DIMENSIONS = (
    "linked_address",
    "linked_body_byte",
    "linked_byte",
    "linked_presence",
    "linked_target_identity",
    "linked_targets",
    "object_byte",
    "relocation_identity",
)


class DataArtifactProgressError(ValueError):
    """A fail-closed reviewed data-artifact registration error."""


@dataclass(frozen=True)
class DataArtifactPlan:
    expected_revision: int
    proposed_revision: int
    artifact_id: str
    row: Mapping[str, Any]
    output_section_start: str
    output_section_end_exclusive: str
    evidence_id: str | None
    evidence_row: Mapping[str, Any] | None
    proposed: Mapping[str, Any]

    def to_dict(self) -> dict[str, Any]:
        result = {
            "operation": REGISTER_OPERATION,
            "expected_revision": self.expected_revision,
            "proposed_revision": self.proposed_revision,
            "artifact_id": self.artifact_id,
            "artifact": deepcopy(dict(self.row)),
            "output_section": {
                "id": self.row["output_section_id"],
                "start": self.output_section_start,
                "end_exclusive": self.output_section_end_exclusive,
            },
        }
        if self.evidence_id is not None and self.evidence_row is not None:
            result["registered_evidence"] = {
                "id": self.evidence_id,
                "row": deepcopy(dict(self.evidence_row)),
            }
        return result


def _normalize_reviewed_evidence(
    value: Any,
    *,
    artifact_id: str,
    artifact: Mapping[str, Any],
) -> dict[str, Any]:
    evidence = _require_mapping(
        value, label="data-artifact new_evidence"
    )
    _require_exact_keys(
        evidence,
        keys=(
            "kind",
            "method",
            "summary",
            "scope_ids",
            "observation",
            "command",
            "target_id",
            "artifacts",
        ),
        label="data-artifact new_evidence",
    )
    if evidence["kind"] != REVIEWED_EVIDENCE_KIND:
        raise DataArtifactProgressError(
            f"data-artifact new_evidence.kind must be {REVIEWED_EVIDENCE_KIND!r}"
        )
    if evidence["method"] != REVIEWED_EVIDENCE_METHOD:
        raise DataArtifactProgressError(
            f"data-artifact new_evidence.method must be {REVIEWED_EVIDENCE_METHOD!r}"
        )
    summary = evidence["summary"]
    if (
        not isinstance(summary, str)
        or len(summary.strip()) < 40
        or summary.strip() != summary
        or artifact["address"].lower() not in summary.lower()
    ):
        raise DataArtifactProgressError(
            "data-artifact new_evidence.summary must be a trimmed substantive "
            "observation naming the exact artifact address"
        )
    if evidence["scope_ids"] != [artifact_id]:
        raise DataArtifactProgressError(
            "data-artifact new_evidence.scope_ids must be the singleton exact "
            "artifact id"
        )
    observation = _require_mapping(
        evidence["observation"],
        label="data-artifact new_evidence.observation",
    )
    _require_exact_keys(
        observation,
        keys=(
            "artifact_id",
            "address",
            "size",
            "end_exclusive",
            "output_section_id",
        ),
        label="data-artifact new_evidence.observation",
    )
    expected_observation = {
        "artifact_id": artifact_id,
        "address": artifact["address"],
        "size": artifact["size"],
        "end_exclusive": artifact["end_exclusive"],
        "output_section_id": artifact["output_section_id"],
    }
    normalized_observation = {
        "artifact_id": observation["artifact_id"],
        "address": normalize_artifact_address(observation["address"]),
        "size": _nonnegative_integer(
            observation["size"],
            label="data-artifact new_evidence.observation.size",
        ),
        "end_exclusive": normalize_artifact_address(
            observation["end_exclusive"]
        ),
        "output_section_id": observation["output_section_id"],
    }
    if normalized_observation != expected_observation:
        raise DataArtifactProgressError(
            "data-artifact new_evidence.observation must exactly repeat the "
            "artifact id/address/extent/output section"
        )
    command = evidence["command"]
    required_tokens = (
        "python tools/recoil.py audit bn-data-evidence ",
        artifact["address"],
        f"--size {artifact['size']}",
        f"--binary {artifact['binary']}",
        "--json",
    )
    if (
        not isinstance(command, str)
        or command.strip() != command
        or not command.startswith(required_tokens[0])
        or any(token not in command for token in required_tokens[1:])
    ):
        raise DataArtifactProgressError(
            "data-artifact new_evidence.command must be the current "
            "bn-data-evidence JSON command for the exact address, size, and binary"
        )
    target_id = evidence["target_id"]
    if (
        not isinstance(target_id, str)
        or not target_id
        or target_id not in artifact["verification_target_ids"]
    ):
        raise DataArtifactProgressError(
            "data-artifact new_evidence.target_id must be one exact artifact "
            "verification_target_id"
        )
    artifacts = evidence["artifacts"]
    if not isinstance(artifacts, list):
        raise DataArtifactProgressError(
            "data-artifact new_evidence.artifacts must be an array"
        )
    normalized_artifacts: list[dict[str, Any]] = []
    seen_paths: set[str] = set()
    for index, raw_artifact in enumerate(artifacts):
        item = _require_mapping(
            raw_artifact,
            label=f"data-artifact new_evidence.artifacts[{index}]",
        )
        _require_exact_keys(
            item,
            keys=("path", "size"),
            label=f"data-artifact new_evidence.artifacts[{index}]",
        )
        path = item["path"]
        if (
            not isinstance(path, str)
            or not path
            or path.strip() != path
            or Path(path).is_absolute()
            or ".." in Path(path).parts
        ):
            raise DataArtifactProgressError(
                "data-artifact new_evidence artifact paths must be trimmed "
                "repository-relative paths without '..'"
            )
        normalized_path = Path(path).as_posix()
        size = _nonnegative_integer(
            item["size"],
            label=f"data-artifact new_evidence.artifacts[{index}].size",
        )
        if size == 0:
            raise DataArtifactProgressError(
                "data-artifact new_evidence artifact sizes must be positive"
            )
        if normalized_path in seen_paths:
            raise DataArtifactProgressError(
                "data-artifact new_evidence.artifacts contains duplicate paths"
            )
        seen_paths.add(normalized_path)
        normalized_artifacts.append({"path": normalized_path, "size": size})
    return {
        "kind": REVIEWED_EVIDENCE_KIND,
        "method": REVIEWED_EVIDENCE_METHOD,
        "summary": summary,
        "scope_ids": [artifact_id],
        "observation": expected_observation,
        "command": command,
        "target_id": target_id,
        "artifacts": normalized_artifacts,
    }


def normalize_register_payload(value: Any) -> dict[str, Any]:
    payload = _require_mapping(value, label="data-artifact payload")
    payload_keys = set(payload)
    expected_keys = {"operation", "reviewed", "artifact_id", "artifact"}
    if payload_keys not in (expected_keys, expected_keys | {"new_evidence"}):
        raise DataArtifactProgressError(
            "data-artifact payload keys must be exactly operation, "
            "reviewed, artifact_id, artifact, with optional new_evidence"
        )
    if payload["operation"] != REGISTER_OPERATION:
        raise DataArtifactProgressError(
            f"data-artifact payload.operation must be {REGISTER_OPERATION!r}"
        )
    if payload["reviewed"] is not True:
        raise DataArtifactProgressError(
            "data-artifact registration requires reviewed=true"
        )
    artifact_id = payload["artifact_id"]
    if (
        not isinstance(artifact_id, str)
        or SOURCE_ARTIFACT_ID_RE.fullmatch(artifact_id) is None
        or ":data:" not in artifact_id
        or ":logical-data:" in artifact_id
    ):
        raise DataArtifactProgressError(
            "data-artifact artifact_id must be one exact physical "
            "<binary>:data:<address> artifact id"
        )
    artifact = _require_mapping(
        payload["artifact"], label="data-artifact artifact"
    )
    _require_exact_keys(
        artifact,
        keys=(
            "address",
            "binary",
            "navigation_name",
            "disposition",
            "source_traceability_state",
            "source_traceability_reason_code",
            "output_section_id",
            "size",
            "end_exclusive",
            "evidence_ids",
            "verification_target_ids",
        ),
        label="data-artifact artifact",
    )
    binary = artifact_id.split(":", 1)[0]
    if artifact["binary"] != binary:
        raise DataArtifactProgressError(
            "data-artifact artifact.binary must match artifact_id"
        )
    address = normalize_artifact_address(artifact["address"])
    if normalize_artifact_address(artifact_id.split(":")[2]) != address:
        raise DataArtifactProgressError(
            "data-artifact artifact.address must match artifact_id"
        )
    navigation_name = artifact["navigation_name"]
    if (
        not isinstance(navigation_name, str)
        or not navigation_name
        or navigation_name.strip() != navigation_name
    ):
        raise DataArtifactProgressError(
            "data-artifact artifact.navigation_name must be a non-empty "
            "trimmed navigation label"
        )
    disposition = artifact["disposition"]
    trace_state = artifact["source_traceability_state"]
    trace_reason = artifact["source_traceability_reason_code"]
    expected_trace_state = {
        "authored": "unresolved",
        "provider": "not-applicable",
    }
    if disposition not in expected_trace_state:
        raise DataArtifactProgressError(
            "data-artifact artifact.disposition must be reviewed authored or provider"
        )
    if trace_state != expected_trace_state[disposition]:
        raise DataArtifactProgressError(
            f"data-artifact {disposition} disposition requires "
            f"source_traceability_state={expected_trace_state[disposition]!r}"
        )
    if (
        not isinstance(trace_reason, str)
        or re.fullmatch(r"[a-z0-9][a-z0-9._-]*", trace_reason) is None
    ):
        raise DataArtifactProgressError(
            "data-artifact artifact.source_traceability_reason_code must be "
            "one non-empty lowercase governed reason"
        )
    output_section_id = artifact["output_section_id"]
    if not isinstance(output_section_id, str) or not output_section_id:
        raise DataArtifactProgressError(
            "data-artifact artifact.output_section_id must be non-empty"
        )
    size = _nonnegative_integer(
        artifact["size"], label="data-artifact artifact.size"
    )
    if size == 0:
        raise DataArtifactProgressError(
            "data-artifact artifact.size must be positive"
        )
    end_exclusive = normalize_artifact_address(artifact["end_exclusive"])
    if int(address, 16) + size != int(end_exclusive, 16):
        raise DataArtifactProgressError(
            "data-artifact artifact.end_exclusive must equal address + size"
        )
    evidence_ids = artifact["evidence_ids"]
    if (
        not isinstance(evidence_ids, list)
        or not all(
            isinstance(item, str) and EVIDENCE_ID_RE.fullmatch(item)
            for item in evidence_ids
        )
    ):
        raise DataArtifactProgressError(
            "data-artifact artifact.evidence_ids must be an array "
            "of exact tracker evidence ids"
        )
    if not evidence_ids and "new_evidence" not in payload:
        raise DataArtifactProgressError(
            "data-artifact artifact.evidence_ids must be non-empty unless one "
            "atomic new_evidence observation is supplied"
        )
    if len(evidence_ids) != len(set(evidence_ids)):
        raise DataArtifactProgressError(
            "data-artifact artifact.evidence_ids contains duplicates"
        )
    target_ids = artifact["verification_target_ids"]
    if (
        not isinstance(target_ids, list)
        or not all(isinstance(item, str) and item for item in target_ids)
    ):
        raise DataArtifactProgressError(
            "data-artifact artifact.verification_target_ids must be a "
            "possibly-empty array of exact existing tracker target ids"
        )
    if len(target_ids) != len(set(target_ids)):
        raise DataArtifactProgressError(
            "data-artifact artifact.verification_target_ids contains duplicates"
        )
    normalized_artifact = {
        "address": address,
        "binary": binary,
        "navigation_name": navigation_name,
        "disposition": disposition,
        "source_traceability_state": trace_state,
        "source_traceability_reason_code": trace_reason,
        "output_section_id": output_section_id,
        "size": size,
        "end_exclusive": end_exclusive,
        "evidence_ids": sorted(evidence_ids),
        "verification_target_ids": sorted(target_ids),
    }
    normalized = {
        "operation": REGISTER_OPERATION,
        "reviewed": True,
        "artifact_id": artifact_id,
        "artifact": normalized_artifact,
    }
    if "new_evidence" in payload:
        normalized["new_evidence"] = _normalize_reviewed_evidence(
            payload["new_evidence"],
            artifact_id=artifact_id,
            artifact=normalized_artifact,
        )
    return normalized


def _pending_claim() -> dict[str, Any]:
    return {
        "disposition": "claim",
        "evidence_ids": [],
        "freshness": "current-unhashed",
        "result": "pending",
    }


def plan_data_artifact_registration(
    tracker: Mapping[str, Any],
    payload: Mapping[str, Any],
    *,
    expected_revision: int,
    repo_root: Path = REPO_ROOT,
) -> DataArtifactPlan:
    if (
        not isinstance(expected_revision, int)
        or isinstance(expected_revision, bool)
        or expected_revision < 0
    ):
        raise DataArtifactProgressError(
            "expected_revision must be a non-negative integer"
        )
    if tracker.get("schema_version") != TRACKER_SCHEMA_VERSION:
        raise DataArtifactProgressError(
            f"tracker schema_version must remain {TRACKER_SCHEMA_VERSION}"
        )
    if tracker.get("revision") != expected_revision:
        raise ConcurrentRevisionUpdate(
            f"revision changed: expected {expected_revision}, "
            f"found {tracker.get('revision')}"
        )
    normalized = normalize_register_payload(payload)
    artifact_id = normalized["artifact_id"]
    artifact = normalized["artifact"]
    symbols = _require_mapping(tracker.get("symbols"), label="tracker.symbols")
    if artifact_id in symbols:
        raise DataArtifactProgressError(
            f"data artifact {artifact_id!r} already exists"
        )
    address = artifact["address"]
    collisions: list[str] = []
    for symbol_id, row in symbols.items():
        if not isinstance(row, Mapping):
            continue
        if (
            str(row.get("binary") or str(symbol_id).split(":", 1)[0])
            == artifact["binary"]
            and row.get("address") is not None
            and normalize_artifact_address(row["address"]) == address
        ):
            collisions.append(str(symbol_id))
    if collisions:
        raise DataArtifactProgressError(
            f"data artifact address {address} already has physical tracker "
            f"identities: {sorted(collisions)}"
        )
    evidence = _require_mapping(tracker.get("evidence"), label="tracker.evidence")
    missing_evidence = [
        evidence_id
        for evidence_id in artifact["evidence_ids"]
        if evidence_id not in evidence
    ]
    if missing_evidence:
        raise DataArtifactProgressError(
            f"data artifact {artifact_id!r} references unknown evidence ids: "
            f"{missing_evidence}"
        )
    new_evidence = normalized.get("new_evidence")
    if new_evidence is not None:
        address_token = artifact["address"].lower()
        already_named = []
        for evidence_id, evidence_row in evidence.items():
            if not isinstance(evidence_row, Mapping):
                continue
            scope_ids = evidence_row.get("scope_ids")
            summary = evidence_row.get("summary")
            if (
                isinstance(scope_ids, list)
                and artifact_id in scope_ids
            ) or (
                isinstance(summary, str)
                and address_token in summary.lower()
            ):
                already_named.append(str(evidence_id))
        if already_named:
            raise DataArtifactProgressError(
                "data-artifact atomic new_evidence is permitted only when no "
                f"existing evidence row names the artifact: {sorted(already_named)}"
            )
    targets = _require_mapping(
        tracker.get("verification_targets"),
        label="tracker.verification_targets",
    )
    missing_targets = [
        target_id
        for target_id in artifact["verification_target_ids"]
        if target_id not in targets
    ]
    if missing_targets:
        raise DataArtifactProgressError(
            f"data artifact {artifact_id!r} references unknown verification "
            f"target ids: {missing_targets}"
        )
    if new_evidence is not None:
        for item in new_evidence["artifacts"]:
            artifact_path = (repo_root / item["path"]).resolve()
            try:
                artifact_path.relative_to(repo_root.resolve())
            except ValueError as exc:
                raise DataArtifactProgressError(
                    f"data-artifact evidence file escapes repository: {item['path']!r}"
                ) from exc
            if not artifact_path.is_file():
                raise DataArtifactProgressError(
                    f"data-artifact evidence file does not exist: {item['path']!r}"
                )
            actual_size = artifact_path.stat().st_size
            if actual_size != item["size"]:
                raise DataArtifactProgressError(
                    f"data-artifact evidence file size changed for {item['path']!r}: "
                    f"expected {item['size']}, found {actual_size}"
                )
    try:
        section_start, section_end = _section_extent(
            tracker,
            artifact_id=artifact_id,
            output_section_id=artifact["output_section_id"],
        )
    except DataExtentProgressError as exc:
        raise DataArtifactProgressError(str(exc)) from exc
    start = int(address, 16)
    end = int(artifact["end_exclusive"], 16)
    if not section_start <= start < end <= section_end:
        raise DataArtifactProgressError(
            f"data artifact {artifact_id!r} extent [0x{start:x},0x{end:x}) "
            f"is outside retail output section {artifact['output_section_id']!r} "
            f"[0x{section_start:x},0x{section_end:x})"
        )

    proposed = deepcopy(dict(tracker))
    new_evidence_id: str | None = None
    new_evidence_row: dict[str, Any] | None = None
    row_evidence_ids = list(artifact["evidence_ids"])
    if new_evidence is not None:
        try:
            new_evidence_id = allocate_evidence_id(proposed)
        except ProgressError as exc:
            raise DataArtifactProgressError(str(exc)) from exc
        proposed_evidence = proposed.get("evidence")
        if not isinstance(proposed_evidence, dict):
            raise DataArtifactProgressError(
                "tracker.evidence must remain an object"
            )
        if new_evidence_id in proposed_evidence:
            raise DataArtifactProgressError(
                f"allocated evidence id collision: {new_evidence_id}"
            )
        new_evidence_row = {
            "kind": new_evidence["kind"],
            "summary": new_evidence["summary"],
            "scope_ids": [artifact_id],
            "result": "observed",
            "disposition": "observed",
            "freshness": "historical",
            "gating": False,
            "validation_mode": "historical-observation",
            "artifacts": deepcopy(new_evidence["artifacts"]),
            "provenance": {
                "method": new_evidence["method"],
                "command": new_evidence["command"],
                "target_id": new_evidence["target_id"],
                "observation": deepcopy(new_evidence["observation"]),
                "acceptance_effect": "none",
            },
        }
        proposed_evidence[new_evidence_id] = deepcopy(new_evidence_row)
        row_evidence_ids.append(new_evidence_id)

    row = {
        "address": address,
        "binary": artifact["binary"],
        "binary_state": {
            dimension: _pending_claim()
            for dimension in PENDING_BINARY_DIMENSIONS
        },
        "binary_state_diagnostics": {
            "legacy_order": _pending_claim(),
        },
        "disposition": artifact["disposition"],
        "evidence_ids": sorted(row_evidence_ids),
        "extent_state": "known",
        "size": artifact["size"],
        "end_exclusive": artifact["end_exclusive"],
        "kind": "data",
        "navigation_name": artifact["navigation_name"],
        "output_section_id": artifact["output_section_id"],
        "physical_block_id": None,
        "semantic_span_ids": [],
        "source_traceability": {
            "state": artifact["source_traceability_state"],
            "source_edges": [],
            "reason_code": artifact["source_traceability_reason_code"],
        },
        "storage_contribution_ids": [],
        "verification_target_ids": list(artifact["verification_target_ids"]),
    }
    proposed["symbols"][artifact_id] = deepcopy(row)
    return DataArtifactPlan(
        expected_revision=expected_revision,
        proposed_revision=expected_revision + 1,
        artifact_id=artifact_id,
        row=row,
        output_section_start=f"0x{section_start:x}",
        output_section_end_exclusive=f"0x{section_end:x}",
        evidence_id=new_evidence_id,
        evidence_row=new_evidence_row,
        proposed=proposed,
    )


def mutate_data_artifact(
    tracker_path: str | Path,
    payload: Mapping[str, Any],
    *,
    expected_revision: int,
    apply: bool,
) -> dict[str, Any]:
    store = tracker_store(tracker_path)
    current = store.load()
    plan = plan_data_artifact_registration(
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
        "catalog_only": True,
        "acceptance_changed": False,
        "source_edges_created": 0,
        "owner_links_created": 0,
        "storage_contributions_created": 0,
    }


def load_payload(
    *, payload_json: str | None = None, payload_file: str | Path | None = None
) -> dict[str, Any]:
    if (payload_json is None) == (payload_file is None):
        raise DataArtifactProgressError(
            "provide exactly one of --payload-json or --payload-file"
        )
    if payload_file is not None:
        path = Path(payload_file)
        try:
            text = path.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError) as exc:
            raise DataArtifactProgressError(
                f"cannot read data-artifact payload file {path}: {exc}"
            ) from exc
    else:
        text = str(payload_json)
    try:
        return normalize_register_payload(json.loads(text))
    except json.JSONDecodeError as exc:
        raise DataArtifactProgressError(
            f"data-artifact payload is invalid JSON: {exc}"
        ) from exc


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Parent-only revision-guarded exact physical data identity and "
            "extent registration without source, owner, storage, or acceptance."
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
        result = mutate_data_artifact(
            args.tracker,
            payload,
            expected_revision=args.expected_revision,
            apply=bool(args.apply),
        )
    except (
        DataArtifactProgressError,
        DataExtentProgressError,
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
