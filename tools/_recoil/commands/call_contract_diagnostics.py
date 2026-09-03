from __future__ import annotations

import json
from typing import Any, Callable, Mapping, Sequence


CALL_CONTRACT_DIAGNOSTIC_SCHEMA_VERSION = 1
CALL_CONTRACT_DIAGNOSTIC_WINDOW_MAX = 128
CALL_CONTRACT_DIAGNOSTIC_SEGMENT_MAX = 256
CALL_CONTRACT_DIAGNOSTIC_RELOCATION_MAX = 16
CALL_CONTRACT_DIAGNOSTIC_SERIALIZED_MAX = 256 * 1024


def _diagnostic_status(status: str, reason_code: str | None = None) -> dict[str, Any]:
    result: dict[str, Any] = {
        "schema_version": CALL_CONTRACT_DIAGNOSTIC_SCHEMA_VERSION,
        "status": status,
        "diagnostic_only": True,
        "acceptance_eligible": False,
    }
    if reason_code is not None:
        result["reason_code"] = reason_code
    return result


def _sequence_row(row: Mapping[str, Any]) -> dict[str, Any]:
    return {str(key): value for key, value in row.items() if key != "ordinal"}


def _sequence_token(row: Mapping[str, Any]) -> str:
    return json.dumps(
        _sequence_row(row),
        ensure_ascii=False,
        sort_keys=True,
        separators=(",", ":"),
    )


def _ordinal_rows_are_exact(rows: Sequence[Mapping[str, Any]]) -> bool:
    return all(
        isinstance(row, Mapping)
        and type(row.get("ordinal")) is int
        and row.get("ordinal") == index
        for index, row in enumerate(rows)
    )


def _lcs_operations(
    expected_keys: Sequence[int],
    candidate_keys: Sequence[int],
    equalities: Sequence[Sequence[bool]],
) -> list[tuple[str, int | None, int | None, int]]:
    expected_count = len(expected_keys)
    candidate_count = len(candidate_keys)
    lengths = [
        [0] * (candidate_count + 1)
        for _ in range(expected_count + 1)
    ]
    for expected_index in range(expected_count - 1, -1, -1):
        for candidate_index in range(candidate_count - 1, -1, -1):
            if equalities[expected_index][candidate_index]:
                lengths[expected_index][candidate_index] = (
                    1 + lengths[expected_index + 1][candidate_index + 1]
                )
            else:
                lengths[expected_index][candidate_index] = max(
                    lengths[expected_index + 1][candidate_index],
                    lengths[expected_index][candidate_index + 1],
                )

    operations: list[tuple[str, int | None, int | None, int]] = []
    expected_index = 0
    candidate_index = 0
    while expected_index < expected_count and candidate_index < candidate_count:
        if (
            equalities[expected_index][candidate_index]
            and 1 + lengths[expected_index + 1][candidate_index + 1]
            == lengths[expected_index][candidate_index]
        ):
            operations.append(
                (
                    "match",
                    expected_index,
                    candidate_index,
                    expected_keys[expected_index],
                )
            )
            expected_index += 1
            candidate_index += 1
        elif (
            lengths[expected_index][candidate_index + 1]
            == lengths[expected_index][candidate_index]
        ):
            operations.append(
                (
                    "candidate_only",
                    None,
                    candidate_index,
                    candidate_keys[candidate_index],
                )
            )
            candidate_index += 1
        else:
            operations.append(
                (
                    "expected_only",
                    expected_index,
                    None,
                    expected_keys[expected_index],
                )
            )
            expected_index += 1
    while expected_index < expected_count:
        operations.append(
            (
                "expected_only",
                expected_index,
                None,
                expected_keys[expected_index],
            )
        )
        expected_index += 1
    while candidate_index < candidate_count:
        operations.append(
            (
                "candidate_only",
                None,
                candidate_index,
                candidate_keys[candidate_index],
            )
        )
        candidate_index += 1
    return operations


def _coalesced_segments(
    operations: Sequence[tuple[str, int | None, int | None, int]],
    *,
    first_ordinal: int,
) -> list[dict[str, Any]]:
    segments: list[dict[str, Any]] = []
    for kind, expected_index, candidate_index, key_id in operations:
        expected_ordinal = (
            first_ordinal + expected_index
            if expected_index is not None
            else None
        )
        candidate_ordinal = (
            first_ordinal + candidate_index
            if candidate_index is not None
            else None
        )
        if segments and segments[-1]["kind"] == kind:
            prior = segments[-1]
            expected_contiguous = (
                expected_ordinal is None
                or prior["expected_end_ordinal_exclusive"] == expected_ordinal
            )
            candidate_contiguous = (
                candidate_ordinal is None
                or prior["candidate_end_ordinal_exclusive"] == candidate_ordinal
            )
            if expected_contiguous and candidate_contiguous:
                if expected_ordinal is not None:
                    prior["expected_end_ordinal_exclusive"] = expected_ordinal + 1
                if candidate_ordinal is not None:
                    prior["candidate_end_ordinal_exclusive"] = candidate_ordinal + 1
                prior["key_ids"].append(key_id)
                continue
        segments.append(
            {
                "kind": kind,
                "expected_start_ordinal": expected_ordinal,
                "expected_end_ordinal_exclusive": (
                    expected_ordinal + 1 if expected_ordinal is not None else None
                ),
                "candidate_start_ordinal": candidate_ordinal,
                "candidate_end_ordinal_exclusive": (
                    candidate_ordinal + 1 if candidate_ordinal is not None else None
                ),
                "key_ids": [key_id],
            }
        )
    return segments


def _exact_relocation_candidates(
    segments: Sequence[Mapping[str, Any]],
    *,
    expected_window_end: int,
    candidate_window_end: int,
    expected_truncated_after: bool,
    candidate_truncated_after: bool,
) -> list[dict[str, Any]] | None:
    expected_segments = [
        segment for segment in segments if segment.get("kind") == "expected_only"
    ]
    candidate_segments = [
        segment for segment in segments if segment.get("kind") == "candidate_only"
    ]
    expected_by_keys: dict[tuple[int, ...], list[Mapping[str, Any]]] = {}
    candidate_by_keys: dict[tuple[int, ...], list[Mapping[str, Any]]] = {}
    for segment in expected_segments:
        keys = tuple(int(value) for value in segment.get("key_ids", ()))
        if len(keys) >= 2:
            expected_by_keys.setdefault(keys, []).append(segment)
    for segment in candidate_segments:
        keys = tuple(int(value) for value in segment.get("key_ids", ()))
        if len(keys) >= 2:
            candidate_by_keys.setdefault(keys, []).append(segment)

    candidates: list[dict[str, Any]] = []
    for keys in expected_by_keys.keys() & candidate_by_keys.keys():
        expected_matches = expected_by_keys[keys]
        candidate_matches = candidate_by_keys[keys]
        if len(expected_matches) != 1 or len(candidate_matches) != 1:
            continue
        expected_segment = expected_matches[0]
        candidate_segment = candidate_matches[0]
        if (
            expected_truncated_after
            and expected_segment.get("expected_end_ordinal_exclusive")
            == expected_window_end
        ) or (
            candidate_truncated_after
            and candidate_segment.get("candidate_end_ordinal_exclusive")
            == candidate_window_end
        ):
            continue
        candidates.append(
            {
                "length": len(keys),
                "expected_start_ordinal": expected_segment[
                    "expected_start_ordinal"
                ],
                "expected_end_ordinal_exclusive": expected_segment[
                    "expected_end_ordinal_exclusive"
                ],
                "candidate_start_ordinal": candidate_segment[
                    "candidate_start_ordinal"
                ],
                "candidate_end_ordinal_exclusive": candidate_segment[
                    "candidate_end_ordinal_exclusive"
                ],
                "key_ids": list(keys),
            }
        )
    candidates.sort(
        key=lambda candidate: (
            -int(candidate["length"]),
            int(candidate["expected_start_ordinal"]),
            int(candidate["candidate_start_ordinal"]),
        )
    )
    if len(candidates) > CALL_CONTRACT_DIAGNOSTIC_RELOCATION_MAX:
        return None
    return candidates


def build_call_contract_divergence_diagnostic(
    *,
    base_result: Mapping[str, Any],
    comparison_context: Mapping[str, Any] | None,
    rows_per_side: int,
    comparison_rows_equal: Callable[
        [Mapping[str, Any], Mapping[str, Any], Mapping[str, Any]], bool
    ]
    | None = None,
) -> dict[str, Any]:
    if not 1 <= rows_per_side <= CALL_CONTRACT_DIAGNOSTIC_WINDOW_MAX:
        return _diagnostic_status("blocked", "diagnostic_size_limit")
    if base_result.get("passed") is True:
        return _diagnostic_status("not_applicable", "comparison_passed")
    first_divergence = base_result.get("first_divergence")
    if not isinstance(first_divergence, Mapping):
        return _diagnostic_status("blocked", "comparison_not_reached")
    if first_divergence.get("kind") == "verifier-blocked":
        return _diagnostic_status("blocked", "base_verifier_blocked")
    if not isinstance(comparison_context, Mapping):
        return _diagnostic_status("blocked", "caller_contract_unavailable")

    expected_rows = comparison_context.get("expected_rows")
    candidate_rows = comparison_context.get("candidate_rows")
    expected_normalized = comparison_context.get("expected_normalized_rows")
    candidate_normalized = comparison_context.get("candidate_normalized_rows")
    expected_comparable = comparison_context.get("expected_comparable_rows")
    candidate_comparable = comparison_context.get("candidate_comparable_rows")
    if not all(
        isinstance(value, list)
        for value in (
            expected_rows,
            candidate_rows,
            expected_normalized,
            candidate_normalized,
            expected_comparable,
            candidate_comparable,
        )
    ):
        return _diagnostic_status("blocked", "normalization_unavailable")
    assert isinstance(expected_rows, list)
    assert isinstance(candidate_rows, list)
    assert isinstance(expected_normalized, list)
    assert isinstance(candidate_normalized, list)
    assert isinstance(expected_comparable, list)
    assert isinstance(candidate_comparable, list)
    if (
        len(expected_rows) != len(expected_normalized)
        or len(candidate_rows) != len(candidate_normalized)
        or len(expected_comparable) != max(len(expected_rows), len(candidate_rows))
        or len(candidate_comparable) != max(len(expected_rows), len(candidate_rows))
        or not _ordinal_rows_are_exact(expected_rows)
        or not _ordinal_rows_are_exact(candidate_rows)
        or not _ordinal_rows_are_exact(expected_normalized)
        or not _ordinal_rows_are_exact(candidate_normalized)
    ):
        return _diagnostic_status("blocked", "normalization_inconsistent")

    ordinal = first_divergence.get("ordinal")
    if type(ordinal) is not int or ordinal < 0:
        return _diagnostic_status("blocked", "invalid_first_divergence")
    if (
        ordinal > len(expected_rows)
        or ordinal > len(candidate_rows)
        or ordinal >= max(len(expected_rows), len(candidate_rows))
    ):
        return _diagnostic_status("blocked", "invalid_first_divergence")
    if any(
        expected_comparable[index] != candidate_comparable[index]
        for index in range(ordinal)
    ):
        return _diagnostic_status("blocked", "normalization_inconsistent")
    expected_at_divergence = expected_comparable[ordinal]
    candidate_at_divergence = candidate_comparable[ordinal]
    divergence_kind = first_divergence.get("kind")
    kind_is_consistent = (
        divergence_kind == "missing"
        and expected_at_divergence is not None
        and candidate_at_divergence is None
    ) or (
        divergence_kind == "extra"
        and expected_at_divergence is None
        and candidate_at_divergence is not None
    ) or (
        divergence_kind == "mismatch"
        and expected_at_divergence is not None
        and candidate_at_divergence is not None
        and expected_at_divergence != candidate_at_divergence
    )
    if not kind_is_consistent:
        return _diagnostic_status("blocked", "normalization_inconsistent")
    if (
        comparison_context.get("symbol_id") != first_divergence.get("symbol_id")
        or comparison_context.get("address") != first_divergence.get("address")
    ):
        return _diagnostic_status("blocked", "caller_contract_unavailable")

    expected_window = expected_normalized[
        ordinal : min(len(expected_normalized), ordinal + rows_per_side)
    ]
    candidate_window = candidate_normalized[
        ordinal : min(len(candidate_normalized), ordinal + rows_per_side)
    ]
    key_table: list[dict[str, Any]] = []
    key_ids: dict[str, int] = {}

    def intern(row: Mapping[str, Any]) -> int:
        token = _sequence_token(row)
        existing = key_ids.get(token)
        if existing is not None:
            return existing
        key_id = len(key_table)
        key_ids[token] = key_id
        key_table.append({"id": key_id, "comparison_row": _sequence_row(row)})
        return key_id

    expected_keys = [intern(row) for row in expected_window]
    candidate_keys = [intern(row) for row in candidate_window]
    expected_raw_window = expected_rows[
        ordinal : min(len(expected_rows), ordinal + rows_per_side)
    ]
    candidate_raw_window = candidate_rows[
        ordinal : min(len(candidate_rows), ordinal + rows_per_side)
    ]
    equalities = [
        [
            (
                comparison_rows_equal(
                    expected_raw,
                    candidate_raw,
                    comparison_context,
                )
                if comparison_rows_equal is not None
                else expected_keys[expected_index]
                == candidate_keys[candidate_index]
            )
            for candidate_index, candidate_raw in enumerate(
                candidate_raw_window
            )
        ]
        for expected_index, expected_raw in enumerate(expected_raw_window)
    ]
    operations = _lcs_operations(expected_keys, candidate_keys, equalities)
    segments = _coalesced_segments(operations, first_ordinal=ordinal)
    if len(segments) > CALL_CONTRACT_DIAGNOSTIC_SEGMENT_MAX:
        return _diagnostic_status("blocked", "diagnostic_size_limit")

    expected_end = ordinal + len(expected_window)
    candidate_end = ordinal + len(candidate_window)
    expected_truncated = expected_end < len(expected_normalized)
    candidate_truncated = candidate_end < len(candidate_normalized)
    relocation_candidates = _exact_relocation_candidates(
        segments,
        expected_window_end=expected_end,
        candidate_window_end=candidate_end,
        expected_truncated_after=expected_truncated,
        candidate_truncated_after=candidate_truncated,
    )
    if relocation_candidates is None:
        return _diagnostic_status("blocked", "diagnostic_size_limit")

    diagnostic = {
        **_diagnostic_status("available"),
        "caller": {
            "symbol_id": comparison_context.get("symbol_id"),
            "retail_address": comparison_context.get("address"),
        },
        "first_divergence_ordinal": ordinal,
        "window": {
            "requested_rows_per_side": rows_per_side,
            "hard_max_rows_per_side": CALL_CONTRACT_DIAGNOSTIC_WINDOW_MAX,
            "common_prefix_rows_omitted": ordinal,
            "expected": {
                "start_ordinal": ordinal,
                "end_ordinal_exclusive": expected_end,
                "total_rows": len(expected_normalized),
                "truncated_after": expected_truncated,
            },
            "candidate": {
                "start_ordinal": ordinal,
                "end_ordinal_exclusive": candidate_end,
                "total_rows": len(candidate_normalized),
                "truncated_after": candidate_truncated,
            },
        },
        "normalized_key_table": key_table,
        "alignment_segments": segments,
        "exact_relocation_candidates": relocation_candidates,
    }
    if len(
        json.dumps(
            diagnostic,
            ensure_ascii=False,
            sort_keys=True,
            separators=(",", ":"),
        ).encode("utf-8")
    ) > CALL_CONTRACT_DIAGNOSTIC_SERIALIZED_MAX:
        return _diagnostic_status("blocked", "diagnostic_size_limit")
    return diagnostic
