#!/usr/bin/env python3
"""Register reviewed authored logical-data occurrences under provider storage."""

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
    _require_exact_keys,
    _require_mapping,
)
from _recoil.commands.source_trace_progress import (
    EVIDENCE_ID_RE,
    SOURCE_ARTIFACT_ID_RE,
    SourceTraceProgressError,
    iter_tracker_artifacts,
    normalize_artifact_address,
    tracker_store,
)
from _recoil.lib.live_progress import (
    TRACKER_SCHEMA_VERSION,
    ConcurrentRevisionUpdate,
    LiveProgressError,
)
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH
from _recoil.lib.tooling import configure_stdio


DEFAULT_TRACKER = DEFAULT_PROGRESS_PATH
REGISTER_OPERATION = "register-logical-data-alias-batch"
POOLING_MODE = "compiler-literal-pooling"


class DataLogicalAliasProgressError(ValueError):
    """A fail-closed reviewed logical-data pooling registration error."""


@dataclass(frozen=True)
class DataLogicalAliasPlan:
    expected_revision: int
    proposed_revision: int
    physical_artifact_id: str
    logical_artifact_ids: tuple[str, ...]
    proposed: Mapping[str, Any]

    def to_dict(self) -> dict[str, Any]:
        return {
            "operation": REGISTER_OPERATION,
            "expected_revision": self.expected_revision,
            "proposed_revision": self.proposed_revision,
            "physical_artifact_id": self.physical_artifact_id,
            "logical_artifact_ids": list(self.logical_artifact_ids),
            "logical_alias_count": len(self.logical_artifact_ids),
        }


def _reason(value: Any, *, label: str) -> str:
    if (
        not isinstance(value, str)
        or not value
        or value.strip() != value
    ):
        raise DataLogicalAliasProgressError(
            f"{label} must be a non-empty trimmed review reason"
        )
    return value


def _evidence_ids(value: Any, *, label: str) -> list[str]:
    if (
        not isinstance(value, list)
        or not value
        or not all(
            isinstance(item, str) and EVIDENCE_ID_RE.fullmatch(item)
            for item in value
        )
    ):
        raise DataLogicalAliasProgressError(
            f"{label} must be a non-empty array of exact tracker evidence ids"
        )
    if len(value) != len(set(value)):
        raise DataLogicalAliasProgressError(f"{label} contains duplicates")
    return sorted(value)


def normalize_register_payload(value: Any) -> dict[str, Any]:
    payload = _require_mapping(value, label="logical-data payload")
    _require_exact_keys(
        payload,
        keys=(
            "operation",
            "reviewed",
            "physical_artifact_id",
            "expected_physical",
            "pooling",
            "aliases",
        ),
        label="logical-data payload",
    )
    if payload["operation"] != REGISTER_OPERATION:
        raise DataLogicalAliasProgressError(
            f"logical-data payload.operation must be {REGISTER_OPERATION!r}"
        )
    if payload["reviewed"] is not True:
        raise DataLogicalAliasProgressError(
            "logical-data alias registration requires reviewed=true"
        )
    physical_id = payload["physical_artifact_id"]
    if (
        not isinstance(physical_id, str)
        or SOURCE_ARTIFACT_ID_RE.fullmatch(physical_id) is None
        or ":data:" not in physical_id
        or ":logical-data:" in physical_id
    ):
        raise DataLogicalAliasProgressError(
            "physical_artifact_id must be one exact physical data artifact id"
        )
    expected = _require_mapping(
        payload["expected_physical"], label="logical-data expected_physical"
    )
    _require_exact_keys(
        expected,
        keys=(
            "disposition",
            "address",
            "extent_state",
            "size",
            "end_exclusive",
            "source_traceability_state",
            "source_traceability_reason_code",
        ),
        label="logical-data expected_physical",
    )
    if expected["disposition"] != "provider":
        raise DataLogicalAliasProgressError(
            "logical-data pooling requires reviewed expected physical "
            "disposition='provider'"
        )
    if expected["extent_state"] != "known":
        raise DataLogicalAliasProgressError(
            "logical-data pooling requires expected extent_state='known'"
        )
    if expected["source_traceability_state"] != "not-applicable":
        raise DataLogicalAliasProgressError(
            "logical-data provider storage requires expected physical "
            "source_traceability_state='not-applicable'"
        )
    reason_code = expected["source_traceability_reason_code"]
    if (
        not isinstance(reason_code, str)
        or re.fullmatch(r"[a-z0-9][a-z0-9._-]*", reason_code) is None
    ):
        raise DataLogicalAliasProgressError(
            "expected physical source_traceability_reason_code must be one "
            "lowercase governed reason"
        )
    address = normalize_artifact_address(expected["address"])
    size = expected["size"]
    if isinstance(size, bool) or not isinstance(size, int) or size <= 0:
        raise DataLogicalAliasProgressError(
            "logical-data expected_physical.size must be positive"
        )
    end = normalize_artifact_address(expected["end_exclusive"])
    if int(address, 16) + size != int(end, 16):
        raise DataLogicalAliasProgressError(
            "logical-data expected physical end must equal address + size"
        )

    pooling = _require_mapping(payload["pooling"], label="logical-data pooling")
    _require_exact_keys(
        pooling,
        keys=("mode", "reason", "evidence_ids"),
        label="logical-data pooling",
    )
    if pooling["mode"] != POOLING_MODE:
        raise DataLogicalAliasProgressError(
            f"logical-data pooling.mode must be {POOLING_MODE!r}"
        )
    pooling_reason = _reason(
        pooling["reason"], label="logical-data pooling.reason"
    )
    pooling_evidence = _evidence_ids(
        pooling["evidence_ids"],
        label="logical-data pooling.evidence_ids",
    )

    aliases = payload["aliases"]
    if not isinstance(aliases, list) or not aliases:
        raise DataLogicalAliasProgressError(
            "logical-data aliases must be a non-empty array"
        )
    normalized_aliases: list[dict[str, Any]] = []
    seen_ids: set[str] = set()
    for index, raw_alias in enumerate(aliases):
        label = f"logical-data aliases[{index}]"
        alias = _require_mapping(raw_alias, label=label)
        _require_exact_keys(
            alias,
            keys=(
                "artifact_id",
                "disposition",
                "navigation_name",
                "object_symbol",
                "evidence_ids",
                "source_traceability_reason_code",
            ),
            label=label,
        )
        alias_id = alias["artifact_id"]
        if (
            not isinstance(alias_id, str)
            or SOURCE_ARTIFACT_ID_RE.fullmatch(alias_id) is None
            or ":logical-data:" not in alias_id
        ):
            raise DataLogicalAliasProgressError(
                f"{label}.artifact_id must be one exact logical-data id"
            )
        if normalize_artifact_address(alias_id.split(":")[2]) != address:
            raise DataLogicalAliasProgressError(
                f"{label}.artifact_id address must equal the reviewed physical "
                "representative address; address equality is necessary but "
                "not sufficient without pooling evidence/object identity"
            )
        if alias_id in seen_ids:
            raise DataLogicalAliasProgressError(
                f"{label}.artifact_id duplicates {alias_id!r}"
            )
        seen_ids.add(alias_id)
        if alias["disposition"] != "authored":
            raise DataLogicalAliasProgressError(
                f"{label}.disposition must be reviewed authored"
            )
        navigation_name = _reason(
            alias["navigation_name"], label=f"{label}.navigation_name"
        )
        object_symbol = alias["object_symbol"]
        if (
            not isinstance(object_symbol, str)
            or not object_symbol
            or object_symbol.strip() != object_symbol
            or re.search(r"\s", object_symbol)
        ):
            raise DataLogicalAliasProgressError(
                f"{label}.object_symbol must be one exact non-whitespace "
                "VC5 object symbol"
            )
        evidence_ids = _evidence_ids(
            alias["evidence_ids"], label=f"{label}.evidence_ids"
        )
        source_reason = alias["source_traceability_reason_code"]
        if (
            not isinstance(source_reason, str)
            or re.fullmatch(r"[a-z0-9][a-z0-9._-]*", source_reason) is None
        ):
            raise DataLogicalAliasProgressError(
                f"{label}.source_traceability_reason_code must be one "
                "lowercase governed reason"
            )
        normalized_aliases.append(
            {
                "artifact_id": alias_id,
                "disposition": "authored",
                "navigation_name": navigation_name,
                "object_symbol": object_symbol,
                "evidence_ids": evidence_ids,
                "source_traceability_reason_code": source_reason,
            }
        )
    normalized_aliases.sort(key=lambda item: item["artifact_id"])
    return {
        "operation": REGISTER_OPERATION,
        "reviewed": True,
        "physical_artifact_id": physical_id,
        "expected_physical": {
            "disposition": "provider",
            "address": address,
            "extent_state": "known",
            "size": size,
            "end_exclusive": end,
            "source_traceability_state": "not-applicable",
            "source_traceability_reason_code": reason_code,
        },
        "pooling": {
            "mode": POOLING_MODE,
            "reason": pooling_reason,
            "evidence_ids": pooling_evidence,
        },
        "aliases": normalized_aliases,
    }


def plan_logical_data_alias_batch(
    tracker: Mapping[str, Any],
    payload: Mapping[str, Any],
    *,
    expected_revision: int,
) -> DataLogicalAliasPlan:
    if (
        not isinstance(expected_revision, int)
        or isinstance(expected_revision, bool)
        or expected_revision < 0
    ):
        raise DataLogicalAliasProgressError(
            "expected_revision must be a non-negative integer"
        )
    if tracker.get("schema_version") != TRACKER_SCHEMA_VERSION:
        raise DataLogicalAliasProgressError(
            f"tracker schema_version must remain {TRACKER_SCHEMA_VERSION}"
        )
    if tracker.get("revision") != expected_revision:
        raise ConcurrentRevisionUpdate(
            f"revision changed: expected {expected_revision}, "
            f"found {tracker.get('revision')}"
        )
    normalized = normalize_register_payload(payload)
    physical_id = normalized["physical_artifact_id"]
    symbols = _require_mapping(tracker.get("symbols"), label="tracker.symbols")
    physical = _require_mapping(
        symbols.get(physical_id),
        label=f"tracker.symbols[{physical_id!r}]",
    )
    expected = normalized["expected_physical"]
    trace = _require_mapping(
        physical.get("source_traceability"),
        label=f"tracker.symbols[{physical_id!r}].source_traceability",
    )
    actual = {
        "disposition": physical.get("disposition"),
        "address": (
            normalize_artifact_address(physical.get("address"))
            if physical.get("address") is not None
            else None
        ),
        "extent_state": physical.get("extent_state"),
        "size": physical.get("size"),
        "end_exclusive": (
            normalize_artifact_address(physical.get("end_exclusive"))
            if physical.get("end_exclusive") is not None
            else None
        ),
        "source_traceability_state": trace.get("state"),
        "source_traceability_reason_code": trace.get("reason_code"),
    }
    if actual != expected:
        raise DataLogicalAliasProgressError(
            f"physical data artifact {physical_id!r} exact reviewed state is "
            f"stale: expected {expected!r}, found {actual!r}"
        )
    if trace.get("source_edges") != []:
        raise DataLogicalAliasProgressError(
            f"physical provider artifact {physical_id!r} must not have "
            "production source edges"
        )
    existing_ids = {
        artifact.artifact_id for artifact in iter_tracker_artifacts(tracker)
    }
    requested_ids = {alias["artifact_id"] for alias in normalized["aliases"]}
    duplicates = sorted(existing_ids & requested_ids)
    if duplicates:
        raise DataLogicalAliasProgressError(
            f"logical-data artifact ids already exist: {duplicates}"
        )
    evidence = _require_mapping(tracker.get("evidence"), label="tracker.evidence")
    all_evidence = {
        *normalized["pooling"]["evidence_ids"],
        *(
            evidence_id
            for alias in normalized["aliases"]
            for evidence_id in alias["evidence_ids"]
        ),
    }
    missing = sorted(all_evidence - set(evidence))
    if missing:
        raise DataLogicalAliasProgressError(
            f"logical-data pooling references unknown evidence ids: {missing}"
        )

    proposed = deepcopy(dict(tracker))
    mutable_physical = proposed["symbols"][physical_id]
    logical_aliases = mutable_physical.setdefault("logical_aliases", {})
    if not isinstance(logical_aliases, dict):
        raise DataLogicalAliasProgressError(
            f"physical data artifact {physical_id!r}.logical_aliases must be "
            "an object"
        )
    for alias in normalized["aliases"]:
        logical_aliases[alias["artifact_id"]] = {
            "kind": "data",
            "disposition": alias["disposition"],
            "navigation_name": alias["navigation_name"],
            "object_symbol": alias["object_symbol"],
            "evidence_ids": alias["evidence_ids"],
            "pooling": {
                "mode": normalized["pooling"]["mode"],
                "reason": normalized["pooling"]["reason"],
                "physical_artifact_id": physical_id,
                "evidence_ids": normalized["pooling"]["evidence_ids"],
            },
            "source_traceability": {
                "state": "unresolved",
                "source_edges": [],
                "reason_code": alias["source_traceability_reason_code"],
            },
        }
    return DataLogicalAliasPlan(
        expected_revision=expected_revision,
        proposed_revision=expected_revision + 1,
        physical_artifact_id=physical_id,
        logical_artifact_ids=tuple(
            alias["artifact_id"] for alias in normalized["aliases"]
        ),
        proposed=proposed,
    )


def mutate_logical_data_alias_batch(
    tracker_path: str | Path,
    payload: Mapping[str, Any],
    *,
    expected_revision: int,
    apply: bool,
) -> dict[str, Any]:
    store = tracker_store(tracker_path)
    current = store.load()
    plan = plan_logical_data_alias_batch(
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
        "logical_only": True,
        "physical_artifacts_created": 0,
        "physical_source_edges_created": 0,
        "owner_links_created": 0,
        "acceptance_changed": False,
    }


def load_payload(
    *, payload_json: str | None = None, payload_file: str | Path | None = None
) -> dict[str, Any]:
    if (payload_json is None) == (payload_file is None):
        raise DataLogicalAliasProgressError(
            "provide exactly one of --payload-json or --payload-file"
        )
    if payload_file is not None:
        path = Path(payload_file)
        try:
            text = path.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError) as exc:
            raise DataLogicalAliasProgressError(
                f"cannot read logical-data payload file {path}: {exc}"
            ) from exc
    else:
        text = str(payload_json)
    try:
        return normalize_register_payload(json.loads(text))
    except json.JSONDecodeError as exc:
        raise DataLogicalAliasProgressError(
            f"logical-data payload is invalid JSON: {exc}"
        ) from exc


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Parent-only revision-guarded logical-data pooling registration "
            "under one reviewed provider physical representative."
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
        result = mutate_logical_data_alias_batch(
            args.tracker,
            payload,
            expected_revision=args.expected_revision,
            apply=bool(args.apply),
        )
    except (
        DataLogicalAliasProgressError,
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
