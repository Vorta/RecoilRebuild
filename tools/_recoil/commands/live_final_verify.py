from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import asdict
from datetime import datetime, timezone
import json
from pathlib import Path
import struct
import subprocess
import sys
from typing import Any, Iterable, Mapping, Sequence

from _recoil.commands.final_image_coverage import (
    derive_final_image_coverage,
    validate_coverage_view,
)
from _recoil.commands.pe_reference import parse_exports, parse_imports
from _recoil.commands.vc5_build import (
    DEFAULT_MANIFEST as DEFAULT_FINAL_CONFIG,
    LinkedMapSymbol,
    build_paths,
    load_config,
    parse_link_map,
    with_explicit_build_dir,
)
from _recoil.lib.pe import PeFormatError, data_directory, parse_pe_headers
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH, SCHEMA_VERSION, ProgressDocument
from _recoil.lib.tooling import REPO_ROOT, configure_stdio, display_path
from _recoil.lib.windows_identity import StableReadHandle


DEFAULT_REFERENCE = REPO_ROOT / "support" / "Recoil.exe"
DEFAULT_TRACKER = DEFAULT_PROGRESS_PATH
CATALOG_VERSION = 1
TIMESTAMP_FIELD_SIZE = 4
TEXT_ENTITY_KINDS = {"address-group", "padding", "data", "provider", "compiler"}
DATA_ENTITY_KINDS = {"initialized-data", "pointer-data", "bss", "padding"}
OTHER_ENTITY_KINDS = {
    "resource",
    "relocations",
    "imports",
    "exports",
    "provider-data",
    "section-payload",
    "padding",
}


class LiveFinalError(RuntimeError):
    pass


def _unique_root() -> Path:
    import os

    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    return Path("build") / "live-validation" / "final" / f"run-{stamp}-{os.getpid()}"


def _section_projection(section: Any) -> dict[str, Any]:
    return {
        "name": section.name,
        "virtual_address": section.virtual_address,
        "virtual_size": section.virtual_size,
        "raw_pointer": section.raw_pointer,
        "raw_size": section.raw_size,
        "characteristics": section.characteristics,
    }


def _directory_projection(directory: Any) -> dict[str, Any]:
    return {
        "name": directory.name,
        "rva": directory.rva,
        "size": directory.size,
        "file_offset": directory.file_offset,
    }


def semantic_projection(data: bytes, *, source: str) -> dict[str, Any]:
    """Project acceptance-relevant PE semantics, intentionally excluding COFF time."""
    headers = parse_pe_headers(data, source=source)
    import_directory = data_directory(headers, 1)
    export_directory = data_directory(headers, 0)
    resource_directory = data_directory(headers, 2)
    return {
        "machine": headers.machine,
        "section_count": headers.section_count,
        "characteristics": headers.characteristics,
        "optional_header_magic": headers.optional_header_magic,
        "entry_point_rva": headers.entry_point_rva,
        "image_base": headers.image_base,
        "section_alignment": headers.section_alignment,
        "file_alignment": headers.file_alignment,
        "subsystem": headers.subsystem,
        "size_of_image": headers.size_of_image,
        "checksum": headers.checksum,
        "number_of_rva_and_sizes": headers.number_of_rva_and_sizes,
        "sections": [_section_projection(section) for section in headers.sections],
        "data_directories": [
            _directory_projection(directory) for directory in headers.data_directories
        ],
        "imports": [
            asdict(row)
            for row in parse_imports(data, import_directory.rva, headers.sections)
        ],
        "exports": [
            asdict(row)
            for row in parse_exports(data, export_directory.rva, headers.sections)
        ],
        "resources": {
            "rva": resource_directory.rva,
            "size": resource_directory.size,
            "file_offset": resource_directory.file_offset,
        },
    }


def _semantic_differences(
    expected: Any,
    candidate: Any,
    *,
    path: str = "",
    limit: int = 64,
) -> list[dict[str, Any]]:
    differences: list[dict[str, Any]] = []

    def visit(left: Any, right: Any, current: str) -> None:
        if len(differences) >= limit:
            return
        if isinstance(left, Mapping) and isinstance(right, Mapping):
            for key in sorted(set(left) | set(right), key=str):
                child = f"{current}.{key}" if current else str(key)
                if key not in left:
                    differences.append({"path": child, "expected": None, "candidate": right[key]})
                elif key not in right:
                    differences.append({"path": child, "expected": left[key], "candidate": None})
                else:
                    visit(left[key], right[key], child)
            return
        if isinstance(left, list) and isinstance(right, list):
            for index in range(max(len(left), len(right))):
                child = f"{current}[{index}]"
                if index >= len(left):
                    differences.append({"path": child, "expected": None, "candidate": right[index]})
                elif index >= len(right):
                    differences.append({"path": child, "expected": left[index], "candidate": None})
                else:
                    visit(left[index], right[index], child)
                if len(differences) >= limit:
                    break
            return
        if left != right:
            differences.append({"path": current, "expected": left, "candidate": right})

    visit(expected, candidate, path)
    return differences


def _byte_difference_ranges(expected: bytes, candidate: bytes, *, limit: int = 64) -> list[dict[str, int]]:
    extent = max(len(expected), len(candidate))
    ranges: list[dict[str, int]] = []
    start: int | None = None
    for offset in range(extent):
        equal = (
            offset < len(expected)
            and offset < len(candidate)
            and expected[offset] == candidate[offset]
        )
        if not equal and start is None:
            start = offset
        elif equal and start is not None:
            ranges.append({"start": start, "end_exclusive": offset})
            start = None
            if len(ranges) >= limit:
                break
    if start is not None and len(ranges) < limit:
        ranges.append({"start": start, "end_exclusive": extent})
    return ranges


def _header_bytes_without_timestamp(data: bytes, headers: Any) -> bytes:
    first_section = min((section.raw_pointer for section in headers.sections), default=len(data))
    header_end = min(first_section, len(data))
    value = bytearray(data[:header_end])
    timestamp_offset = headers.pe_offset + 8
    if timestamp_offset + TIMESTAMP_FIELD_SIZE > len(value):
        raise PeFormatError("COFF timestamp field lies outside PE headers")
    value[timestamp_offset : timestamp_offset + TIMESTAMP_FIELD_SIZE] = b"\0" * TIMESTAMP_FIELD_SIZE
    return bytes(value)


def _complete_image_bytes_without_timestamp(data: bytes, headers: Any) -> bytes:
    """Return the complete file with only COFF TimeDateStamp normalized.

    This is an exact direct byte-comparison input, not a persisted identity or
    summary.  It deliberately includes headers, section slack, inter-section
    gaps, governed padding, overlay, and every other file byte.
    """

    value = bytearray(data)
    timestamp_offset = headers.pe_offset + 8
    if timestamp_offset + TIMESTAMP_FIELD_SIZE > len(value):
        raise PeFormatError("COFF timestamp field lies outside PE image")
    value[timestamp_offset : timestamp_offset + TIMESTAMP_FIELD_SIZE] = b"\0" * TIMESTAMP_FIELD_SIZE
    return bytes(value)


def _directory_payload(data: bytes, headers: Any, index: int) -> bytes | None:
    directory = data_directory(headers, index)
    if directory.rva == 0 and directory.size == 0:
        return b""
    if directory.rva == 0 or directory.size == 0:
        return None
    # The certificate directory stores a file offset rather than an RVA.
    offset = directory.rva if index == 4 else directory.file_offset
    if offset is None or offset < 0 or offset + directory.size > len(data):
        return None
    return data[offset : offset + directory.size]


def _normalize_map_rows(rows: Iterable[Any]) -> list[dict[str, Any]]:
    normalized: list[dict[str, Any]] = []
    for row in rows:
        if isinstance(row, LinkedMapSymbol):
            if not row.is_function:
                continue
            normalized.append(
                {"address": row.address, "symbol": row.symbol, "object": row.object}
            )
        elif isinstance(row, Mapping):
            if row.get("is_function", True):
                normalized.append(
                    {
                        "address": int(row["address"]),
                        "symbol": str(row["symbol"]),
                        "object": str(row["object"]),
                    }
                )
    return normalized


def _validate_coverage_text_population(
    coverage: Mapping[str, Any],
    projection: Mapping[str, Any],
    map_rows: Sequence[Mapping[str, Any]],
) -> list[str]:
    expected_rows = coverage.get("selected_text_identities")
    if not isinstance(expected_rows, list):
        return ["live final coverage lacks selected .text identity rows"]
    text_section = next(
        (section for section in projection["sections"] if section["name"] == ".text"),
        None,
    )
    if not isinstance(text_section, Mapping):
        return ["retail image lacks a .text section for selected population validation"]
    text_start = int(projection["image_base"]) + int(text_section["virtual_address"])
    text_end = text_start + int(text_section["virtual_size"])
    expected: list[tuple[int, str, str]] = []
    for row in expected_rows:
        if not isinstance(row, Mapping):
            return ["live final coverage contains a malformed selected .text identity"]
        address = row.get("address")
        symbol = row.get("symbol")
        object_name = row.get("object")
        if (
            not isinstance(address, int)
            or isinstance(address, bool)
            or not isinstance(symbol, str)
            or not symbol
            or not isinstance(object_name, str)
            or not object_name
        ):
            return ["live final coverage contains an incomplete selected .text identity"]
        expected.append((address, symbol, object_name))
    observed = [
        (int(row["address"]), str(row["symbol"]), str(row["object"]))
        for row in map_rows
        if text_start <= int(row["address"]) < text_end
    ]
    failures: list[str] = []
    expected_counts = Counter(expected)
    observed_counts = Counter(observed)
    missing = sorted((expected_counts - observed_counts).elements())
    unexpected = sorted((observed_counts - expected_counts).elements())
    if missing:
        failures.append(
            f".text selected MAP population is missing {len(missing)} live identity row(s): "
            f"{missing[:8]}"
        )
    if unexpected:
        failures.append(
            f".text selected MAP population has {len(unexpected)} unexpected row(s): "
            f"{unexpected[:8]}"
        )
    if not missing and not unexpected and observed != expected:
        mismatch = next(
            index
            for index, (expected_row, observed_row) in enumerate(zip(expected, observed))
            if expected_row != observed_row
        )
        failures.append(
            ".text selected MAP order differs at row "
            f"{mismatch}: expected={expected[mismatch]!r}, observed={observed[mismatch]!r}"
        )
    return failures


def _reviewed_match_differences(reference_data, candidate_data, reference_headers, candidate_headers, coverage):
    """Derive permitted file offsets from fresh complete function proofs only."""
    from _recoil.lib.function_match import compare_instructions
    from _recoil.lib.commutative_match import compare_commutative, BODY_PROOF_SCOPE
    from _recoil.lib.match_evidence import source_context, review_current, current_match_level
    from _recoil.lib.pe import rva_to_offset
    offsets, reports = set(), []
    reference_text = next((s for s in reference_headers.sections if s.name == ".text"), None)
    candidate_text = next((s for s in candidate_headers.sections if s.name == ".text"), None)
    for section in coverage.get("sections", []):
        if section.get("name") != ".text":
            continue
        for entity in section.get("typed_entities", []):
            identities = entity.get("identities", [])
            if not any(item.get("match_level") in {"instruction", "commutative"} for item in identities):
                continue
            group_offsets = set()
            group_passed = True
            for identity in identities:
                result = {"symbol_id": identity.get("symbol_id"), "passed": False,
                          "reason": "reviewed match lacks a current complete linked proof"}
                try:
                    if (reference_text is None or candidate_text is None or
                            reference_headers.image_base != candidate_headers.image_base or
                            _section_projection(reference_text) != _section_projection(candidate_text)):
                        raise LiveFinalError("reviewed matching does not relax linked placement")
                    start, end = int(entity["start"]), int(entity["end"])
                    if not 0 <= start < end <= reference_text.raw_size:
                        raise LiveFinalError("invalid reviewed-match typed extent")
                    file_start = reference_text.raw_pointer + start
                    retail = reference_data[file_start:reference_text.raw_pointer + end]
                    candidate = candidate_data[file_start:reference_text.raw_pointer + end]
                    level = identity.get("match_level")
                    if level not in {"instruction", "commutative"}:
                        if retail != candidate:
                            raise LiveFinalError("a byte-matched alias does not permit relaxed differences")
                        result.update(passed=True, match_level="byte", reason="exact")
                    else:
                        state = identity.get("function_match", {})
                        review = identity.get(level + "_match_review", {})
                        if current_match_level(identity) != level:
                            raise LiveFinalError("stale " + level + "-match evidence")
                        catalog = state.get("retail_relocations")
                        if not isinstance(catalog, list):
                            raise LiveFinalError("missing typed relocation contract")
                        # Authenticate stored typed operand locations directly
                        # against retail again; these are never candidate facts.
                        body_va = reference_headers.image_base + reference_text.virtual_address + start
                        for relocation in catalog:
                            field = int(relocation["offset"])
                            raw = int.from_bytes(retail[field:field + 4], "little")
                            observed = ((body_va + field + 4 + (raw if raw < 0x80000000 else raw - 0x100000000))
                                        if relocation["type"] == 20 else raw)
                            if observed != int(relocation["retail_target"]):
                                raise LiveFinalError("retail relocation context changed")
                        if level == "commutative":
                            proof = compare_commutative(retail, candidate, function_address=body_va,
                                contract=review.get("contract"))
                            if proof.get("scope") != BODY_PROOF_SCOPE:
                                raise LiveFinalError("commutative proof scope is missing or invalid")
                        else:
                            proof = compare_instructions(retail, candidate, relocations=catalog,
                                                         symbol=str(identity.get("map_symbol", "")))
                        if not proof["passed"]:
                            raise LiveFinalError(proof["reason"])
                        if not proof.get("exact"):
                            context = source_context(review["context"]["source"], load_config(DEFAULT_FINAL_CONFIG))
                            if not review_current(review, context, proof["differences"], level=level):
                                raise LiveFinalError("Pro review does not cover this live " + level + " difference")
                        result.update(passed=True, match_level="byte" if proof.get("exact") else level,
                                      proof=proof, reason="complete live " + level + " comparison")
                        group_offsets.update(file_start + index for index, (a, b) in enumerate(zip(retail, candidate)) if a != b)
                except (KeyError, TypeError, ValueError, OSError, LiveFinalError) as exc:
                    result["reason"] = f"{identity.get('symbol_id')}: {exc}"
                    group_passed = False
                reports.append(result)
            if group_passed:
                offsets.update(group_offsets)
    return offsets, reports


def _compare_image_data(
    candidate: Path,
    reference: Path,
    *,
    candidate_data: bytes,
    reference_data: bytes,
    coverage: Mapping[str, Any],
    candidate_map_rows: Iterable[Any],
) -> dict[str, Any]:
    reference_headers = parse_pe_headers(reference_data, source=str(reference))
    candidate_headers = parse_pe_headers(candidate_data, source=str(candidate))
    reference_projection = semantic_projection(reference_data, source=str(reference))
    candidate_projection = semantic_projection(candidate_data, source=str(candidate))
    failures = validate_coverage_view(
        coverage,
        reference_headers=reference_headers,
        reference_size=len(reference_data),
    )
    semantic_differences = _semantic_differences(reference_projection, candidate_projection)
    if semantic_differences:
        failures.append(f"PE semantic projection differs in {len(semantic_differences)} reported field(s)")

    reference_header = _header_bytes_without_timestamp(reference_data, reference_headers)
    candidate_header = _header_bytes_without_timestamp(candidate_data, candidate_headers)
    if reference_header != candidate_header:
        failures.append("PE headers differ outside the candidate COFF TimeDateStamp field")

    permitted_offsets, reviewed_results = _reviewed_match_differences(
        reference_data, candidate_data, reference_headers, candidate_headers, coverage)
    failures.extend(row["reason"] for row in reviewed_results if not row["passed"])
    normalized_complete_file_equal = (
        _complete_image_bytes_without_timestamp(reference_data, reference_headers)
        == _complete_image_bytes_without_timestamp(candidate_data, candidate_headers)
    )
    expected_complete = _complete_image_bytes_without_timestamp(reference_data, reference_headers)
    observed_complete = bytearray(_complete_image_bytes_without_timestamp(candidate_data, candidate_headers))
    for offset in permitted_offsets:
        observed_complete[offset] = expected_complete[offset]
    permitted_complete_file_equal = expected_complete == bytes(observed_complete)
    if not permitted_complete_file_equal:
        failures.append(
            "complete PE file bytes differ outside the COFF TimeDateStamp field and live-proved reviewed function differences"
        )

    reference_sections = {section.name: section for section in reference_headers.sections}
    candidate_sections = {section.name: section for section in candidate_headers.sections}
    section_results: list[dict[str, Any]] = []
    for name, reference_section in reference_sections.items():
        candidate_section = candidate_sections.get(name)
        if candidate_section is None:
            section_results.append({"name": name, "passed": False, "reason": "missing"})
            failures.append(f"section {name} is missing")
            continue
        reference_payload = reference_data[
            reference_section.raw_pointer : reference_section.raw_pointer + reference_section.raw_size
        ]
        candidate_payload = candidate_data[
            candidate_section.raw_pointer : candidate_section.raw_pointer + candidate_section.raw_size
        ]
        adjusted_payload = bytearray(candidate_payload)
        for offset in permitted_offsets:
            if reference_section.raw_pointer <= offset < reference_section.raw_pointer + reference_section.raw_size:
                relative = offset - reference_section.raw_pointer
                if relative < len(adjusted_payload):
                    adjusted_payload[relative] = reference_payload[relative]
        passed = reference_payload == bytes(adjusted_payload)
        section_results.append({"name": name, "passed": passed, "size": len(reference_payload),
                                "exact_bytes": reference_payload == candidate_payload})
        if not passed:
            failures.append(f"section {name} payload differs from its typed retail entities")

    directory_results: list[dict[str, Any]] = []
    for index, directory in enumerate(reference_headers.data_directories):
        reference_payload = _directory_payload(reference_data, reference_headers, index)
        candidate_payload = _directory_payload(candidate_data, candidate_headers, index)
        passed = reference_payload is not None and candidate_payload is not None and reference_payload == candidate_payload
        directory_results.append(
            {"name": directory.name, "passed": passed, "required_absent": directory.rva == 0 and directory.size == 0}
        )
        if not passed:
            failures.append(f"data directory {directory.name} payload/presence differs")

    reference_overlay_start = max(
        (section.raw_pointer + section.raw_size for section in reference_headers.sections),
        default=len(reference_data),
    )
    candidate_overlay_start = max(
        (section.raw_pointer + section.raw_size for section in candidate_headers.sections),
        default=len(candidate_data),
    )
    overlay_equal = reference_data[reference_overlay_start:] == candidate_data[candidate_overlay_start:]
    if not overlay_equal:
        failures.append("overlay bytes or provider tail selection differ")

    normalized_map_rows = _normalize_map_rows(candidate_map_rows)
    failures.extend(
        _validate_coverage_text_population(coverage, candidate_projection, normalized_map_rows)
    )

    raw_differences = _byte_difference_ranges(reference_data, candidate_data)
    timestamp_offset = reference_headers.pe_offset + 8
    timestamp_only = bool(raw_differences) and all(
        row["start"] >= timestamp_offset
        and row["end_exclusive"] <= timestamp_offset + TIMESTAMP_FIELD_SIZE
        for row in raw_differences
    )
    return {
        "report_version": 2,
        "kind": "live-final-image-semantic",
        "validation_mode": "live",
        "passed": not failures,
        "candidate": display_path(candidate),
        "reference": display_path(reference),
        "coverage_mode": "live-generated",
        "coverage_version": coverage.get("version"),
        "coverage_complete": coverage.get("complete") is True,
        "semantic_failures": failures,
        "semantic_differences": semantic_differences,
        "sections": section_results,
        "directories": directory_results,
        "imports_equal": reference_projection["imports"] == candidate_projection["imports"],
        "exports_equal": reference_projection["exports"] == candidate_projection["exports"],
        "resource_payload_equal": next(
            (row["passed"] for row in directory_results if row["name"] == "resource"),
            False,
        ),
        "overlay_equal": overlay_equal,
        "resolved_operands_equal": not any("operand" in failure for failure in failures),
        "candidate_timestamp": candidate_headers.timestamp,
        "retail_timestamp": reference_headers.timestamp,
        "timestamp_is_diagnostic_only": True,
        "normalized_complete_file_equal": normalized_complete_file_equal,
        "permitted_complete_file_equal": permitted_complete_file_equal,
        "instruction_matches": [row for row in reviewed_results if row.get("match_level") == "instruction"],
        "contains_instruction_matches": any(row.get("match_level") == "instruction" for row in reviewed_results),
        "commutative_matches": [row for row in reviewed_results if row.get("match_level") == "commutative"],
        "contains_commutative_matches": any(row.get("match_level") == "commutative" for row in reviewed_results),
        "reviewed_match_results": reviewed_results,
        "raw_file_equal_diagnostic": candidate_data == reference_data,
        "raw_difference_ranges_diagnostic": raw_differences,
        "timestamp_only_raw_difference": timestamp_only,
    }


def compare_images(
    candidate: Path,
    reference: Path,
    *,
    coverage: Mapping[str, Any],
    candidate_map_rows: Iterable[Any],
) -> dict[str, Any]:
    """Compare both images directly while stable read handles remain open."""

    with StableReadHandle(reference) as retail_handle, StableReadHandle(candidate) as candidate_handle:
        reference_data = retail_handle.read()
        candidate_data = candidate_handle.read()
        report = _compare_image_data(
            candidate,
            reference,
            candidate_data=candidate_data,
            reference_data=reference_data,
            coverage=coverage,
            candidate_map_rows=candidate_map_rows,
        )
        report["retail_physical_identity"] = retail_handle.identity.to_dict()
        report["candidate_physical_identity"] = candidate_handle.identity.to_dict()
        expected_identity = coverage.get("retail_physical_identity")
        if isinstance(expected_identity, Mapping) and dict(expected_identity) != retail_handle.identity.to_dict():
            report["semantic_failures"].append(
                "retail physical file identity changed after typed coverage derivation"
            )
            report["passed"] = False
        return report


def _load_coverage(
    progress_path: Path,
    *,
    reference: Path = DEFAULT_REFERENCE,
) -> tuple[dict[str, Any], dict[str, Any]]:
    document = ProgressDocument.load(progress_path)
    binary = document.collection("binaries").get("recoil")
    if not isinstance(binary, Mapping):
        raise LiveFinalError("tracker v5 lacks the recoil binary row")
    if not reference.is_file():
        raise LiveFinalError(f"retail reference is missing: {display_path(reference)}")
    with StableReadHandle(reference) as retail_handle:
        coverage = derive_final_image_coverage(
            retail_handle.read(),
            document.data,
            source=display_path(reference),
        )
        coverage["retail_physical_identity"] = retail_handle.identity.to_dict()
    if coverage.get("complete") is not True:
        failures = [str(item) for item in coverage.get("failures", [])]
        preview = "; ".join(failures[:4]) or "unknown typed coverage gap"
        raise LiveFinalError(
            "live typed final-image coverage is incomplete; resolve current order/byte/data/provider "
            f"facts before building the final candidate: {preview}"
        )
    return coverage, document.data


def _load_coverage_from_open_retail(
    progress_path: Path,
    *,
    reference: Path,
    retail_handle: StableReadHandle,
) -> tuple[dict[str, Any], dict[str, Any]]:
    """Derive typed retail coverage while the caller keeps one handle open."""

    document = ProgressDocument.load(progress_path)
    binary = document.collection("binaries").get("recoil")
    if not isinstance(binary, Mapping):
        raise LiveFinalError("tracker v5 lacks the recoil binary row")
    retail_handle.seek(0)
    coverage = derive_final_image_coverage(
        retail_handle.read(), document.data, source=display_path(reference)
    )
    coverage["retail_physical_identity"] = retail_handle.identity.to_dict()
    if coverage.get("complete") is not True:
        failures = [str(item) for item in coverage.get("failures", [])]
        preview = "; ".join(failures[:4]) or "unknown typed coverage gap"
        raise LiveFinalError(
            "live typed final-image coverage is incomplete; resolve current order/byte/data/provider "
            f"facts before building the final candidate: {preview}"
        )
    return coverage, document.data


def _fresh_candidate(args: argparse.Namespace) -> tuple[Path, Path, Path]:
    root = args.build_root or _unique_root()
    config = with_explicit_build_dir(load_config(args.final_config), root)
    paths = build_paths(config)
    command = [
        sys.executable,
        str(REPO_ROOT / "tools" / "recoil.py"),
        "verify",
        "final-build",
        "--",
        "--manifest",
        str(args.final_config),
        "--build-dir",
        str(root),
        "--clean",
    ]
    completed = subprocess.run(command, cwd=REPO_ROOT, check=False)
    if completed.returncode != 0:
        raise LiveFinalError(
            f"fresh final build failed with exit code {completed.returncode}; "
            "resolve the reported compile/link/order divergence first"
        )
    if not paths.exe_path.is_file() or not paths.map_path.is_file():
        raise LiveFinalError("fresh final build did not produce both candidate PE and MAP")
    return paths.exe_path, paths.map_path, root


def run(args: argparse.Namespace) -> dict[str, Any]:
    # Keep retail deny-write/deny-delete identity live across coverage
    # extraction, the fresh build, and the complete direct comparison.
    with StableReadHandle(args.reference) as retail_operation_handle:
        coverage, _tracker = _load_coverage_from_open_retail(
            args.progress,
            reference=args.reference,
            retail_handle=retail_operation_handle,
        )
        if args.candidate is None:
            candidate, map_path, build_root = _fresh_candidate(args)
        else:
            candidate = args.candidate
            map_path = args.map
            build_root = None
            if map_path is None:
                raise LiveFinalError("--candidate requires its fresh --map for selected population validation")
        parsed_map = parse_link_map(map_path)
        report = compare_images(
            candidate,
            args.reference,
            coverage=coverage,
            candidate_map_rows=parsed_map.symbols,
        )
    report["build_root"] = build_root.as_posix() if build_root is not None else None
    report["fresh_build"] = build_root is not None
    report["map"] = display_path(map_path)
    semantic_passed = report["passed"]
    report["semantic_comparison_passed"] = semantic_passed
    report["acceptance_eligible"] = bool(build_root is not None and semantic_passed)
    report["candidate_mode"] = "fresh-build" if build_root is not None else "existing-diagnostic"
    if build_root is None:
        report["passed"] = False
        report["diagnostic_only_reason"] = (
            "existing-candidate mode is diagnostic only; final acceptance requires one fresh "
            "unrestricted build and its paired MAP"
        )
    return report


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Freshly build and validate the complete typed final-image semantics against retail; "
            "whole-file differences and the candidate COFF timestamp are diagnostic only."
        )
    )
    parser.add_argument("--candidate", type=Path, help="Validate an existing candidate without building.")
    parser.add_argument("--map", type=Path, help="Fresh MAP paired with --candidate.")
    parser.add_argument("--reference", type=Path, default=DEFAULT_REFERENCE)
    parser.add_argument("--progress", type=Path, default=DEFAULT_TRACKER)
    parser.add_argument("--final-config", type=Path, default=DEFAULT_FINAL_CONFIG)
    parser.add_argument("--build-root", type=Path)
    parser.add_argument("--json", action="store_true")
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    try:
        report = run(args)
    except (LiveFinalError, OSError, ValueError, PeFormatError) as exc:
        print(f"live final validation error: {exc}", file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print(f"Live final semantic image: {'PASS' if report['passed'] else 'FAIL'}")
        print(f"- typed coverage complete: {str(report['coverage_complete']).lower()}")
        print(f"- acceptance eligible: {str(report.get('acceptance_eligible', False)).lower()}")
        print(f"- candidate timestamp (diagnostic): {report['candidate_timestamp']}")
        print(f"- raw file equality (diagnostic): {str(report['raw_file_equal_diagnostic']).lower()}")
        if report.get("diagnostic_only_reason"):
            print(f"- {report['diagnostic_only_reason']}")
        for failure in report["semantic_failures"][:32]:
            print(f"- {failure}")
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
