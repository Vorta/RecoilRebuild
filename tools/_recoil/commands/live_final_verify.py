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
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH, ProgressDocument
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


def _validate_interval_cover(
    rows: Sequence[Mapping[str, Any]],
    *,
    start_key: str,
    end_key: str,
    extent: int,
    label: str,
) -> list[str]:
    failures: list[str] = []
    intervals: list[tuple[int, int, str]] = []
    for row in rows:
        if start_key not in row and end_key not in row:
            continue
        start = row.get(start_key)
        end = row.get(end_key)
        if (
            not isinstance(start, int)
            or isinstance(start, bool)
            or not isinstance(end, int)
            or isinstance(end, bool)
            or start < 0
            or end <= start
            or end > extent
        ):
            failures.append(f"{label}: invalid interval on {row.get('id')!r}: {start!r}..{end!r}")
            continue
        intervals.append((start, end, str(row.get("id", ""))))
    intervals.sort()
    cursor = 0
    for start, end, entity_id in intervals:
        if start > cursor:
            failures.append(f"{label}: unmodeled range {cursor:#x}..{start:#x}")
        elif start < cursor:
            failures.append(f"{label}: overlapping entity {entity_id!r} begins at {start:#x}")
        cursor = max(cursor, end)
    if cursor < extent:
        failures.append(f"{label}: unmodeled range {cursor:#x}..{extent:#x}")
    if extent and not intervals:
        failures.append(f"{label}: no typed entities cover the section")
    return failures


def _tracker_integer(value: Any) -> int | None:
    if isinstance(value, int) and not isinstance(value, bool):
        return value
    if isinstance(value, str):
        try:
            return int(value, 0)
        except ValueError:
            return None
    return None


def _tracker_extent(row: Mapping[str, Any]) -> tuple[int, int] | None:
    if row.get("extent_state") != "known":
        return None
    start = _tracker_integer(row.get("address"))
    end = _tracker_integer(row.get("end_exclusive"))
    if end is None:
        size = _tracker_integer(row.get("size"))
        end = start + size if start is not None and size is not None else None
    if start is None or end is None or end <= start:
        return None
    return start, end


def _catalog_entity_extent(
    entity: Mapping[str, Any],
    section: Mapping[str, Any],
    projection: Mapping[str, Any],
) -> tuple[int, int] | None:
    start = entity.get("virtual_start")
    end = entity.get("virtual_end")
    if (
        not isinstance(start, int)
        or isinstance(start, bool)
        or not isinstance(end, int)
        or isinstance(end, bool)
        or end <= start
    ):
        return None
    section_start = int(projection["image_base"]) + int(section["virtual_address"])
    return section_start + start, section_start + end


def _tracker_section_matches_projection(
    tracker_section: Mapping[str, Any],
    section: Mapping[str, Any],
    projection: Mapping[str, Any],
) -> bool:
    reference = tracker_section.get("reference")
    if not isinstance(reference, Mapping):
        return False
    return (
        tracker_section.get("binary") == "recoil"
        and tracker_section.get("name") == section.get("name")
        and _tracker_integer(reference.get("image_address"))
        == int(projection["image_base"]) + int(section["virtual_address"])
        and _tracker_integer(reference.get("virtual_size")) == int(section["virtual_size"])
        and _tracker_integer(reference.get("raw_size")) == int(section["raw_size"])
    )


def _validate_text_identity_binding(
    *,
    entity: Mapping[str, Any],
    identity: Mapping[str, Any],
    section: Mapping[str, Any],
    projection: Mapping[str, Any],
    symbols: Mapping[str, Any],
    blocks: Mapping[str, Any],
    output_sections: Mapping[str, Any],
) -> list[str]:
    failures: list[str] = []
    entity_id = entity.get("id")
    symbol_id = identity.get("symbol_id")
    if not isinstance(symbol_id, str) or not symbol_id:
        return [f"catalog .text identity in {entity_id!r} lacks symbol_id"]
    symbol = symbols.get(symbol_id)
    if not isinstance(symbol, Mapping) or symbol.get("binary") != "recoil":
        return [f"catalog .text identity {symbol_id!r} does not resolve to a recoil tracker symbol"]
    section_id = "recoil:section:.text"
    tracker_section = output_sections.get(section_id)
    if not isinstance(tracker_section, Mapping) or not _tracker_section_matches_projection(
        tracker_section, section, projection
    ):
        failures.append(f"catalog .text identity {symbol_id!r} lacks tracker output section {section_id}")
    if symbol.get("output_section_id") != section_id:
        failures.append(
            f"catalog .text identity {symbol_id!r} resolves to tracker section "
            f"{symbol.get('output_section_id')!r}, expected {section_id}"
        )
    entity_extent = _catalog_entity_extent(entity, section, projection)
    symbol_extent = _tracker_extent(symbol)
    if entity_extent is None or symbol_extent != entity_extent:
        failures.append(
            f"catalog .text identity {symbol_id!r} extent does not exactly match its tracker symbol: "
            f"catalog={entity_extent}, tracker={symbol_extent}"
        )
    block_id = identity.get("source_block_id")
    block = blocks.get(block_id) if isinstance(block_id, str) else None
    if not isinstance(block, Mapping) or block.get("binary") != "recoil":
        failures.append(
            f"catalog .text identity {symbol_id!r} source_block_id {block_id!r} does not resolve"
        )
    else:
        if symbol.get("physical_block_id") != block_id:
            failures.append(
                f"catalog .text identity {symbol_id!r} source block differs from tracker symbol"
            )
        contributions = block.get("contribution_ids")
        if not isinstance(contributions, list) or symbol_id not in contributions:
            failures.append(
                f"catalog .text identity {symbol_id!r} is not a contribution of {block_id!r}"
            )
    contribution_class = identity.get("contribution_class")
    if contribution_class != symbol.get("pipeline_class"):
        failures.append(
            f"catalog .text identity {symbol_id!r} contribution_class {contribution_class!r} "
            f"does not match tracker pipeline_class {symbol.get('pipeline_class')!r}"
        )
    tracker_comdat = symbol.get("comdat")
    if not isinstance(tracker_comdat, bool):
        failures.append(f"catalog .text tracker symbol {symbol_id!r} lacks COMDAT classification")
    elif identity.get("comdat") is not tracker_comdat:
        failures.append(
            f"catalog .text identity {symbol_id!r} COMDAT classification differs from tracker"
        )
    if "provider" in identity or "provider" in symbol:
        if identity.get("provider") != symbol.get("provider"):
            failures.append(
                f"catalog .text identity {symbol_id!r} provider classification differs from tracker"
            )
    kind = entity.get("kind")
    if kind == "provider" and not (
        symbol.get("kind") == "provider-function" or symbol.get("pipeline_class") == "non-authored"
    ):
        failures.append(f"catalog .text provider identity {symbol_id!r} is not provider-classified")
    if kind == "compiler" and not (
        symbol.get("kind") == "compiler-function"
        or str(symbol.get("authored_order_role", "")).startswith("compiler-generated-")
    ):
        failures.append(f"catalog .text compiler identity {symbol_id!r} is not compiler-classified")
    return failures


def _validate_icf_binding(
    entity: Mapping[str, Any],
    identities: Sequence[Mapping[str, Any]],
    symbols: Mapping[str, Any],
) -> list[str]:
    if len(identities) <= 1:
        return []
    failures: list[str] = []
    entity_id = entity.get("id")
    icf = entity.get("icf")
    if not isinstance(icf, Mapping):
        return [f"catalog .text alias group {entity_id!r} lacks ICF tracker binding"]
    winner_symbol_id = icf.get("winner_symbol_id")
    winner_symbol = icf.get("winner_symbol")
    identity_by_id = {
        identity.get("symbol_id"): identity
        for identity in identities
        if isinstance(identity.get("symbol_id"), str)
    }
    winner_identity = identity_by_id.get(winner_symbol_id)
    if not isinstance(winner_identity, Mapping):
        failures.append(
            f"catalog .text alias group {entity_id!r} winner_symbol_id is not one of its identities"
        )
    elif winner_identity.get("map_symbol") != winner_symbol:
        failures.append(
            f"catalog .text alias group {entity_id!r} winner_symbol does not match winner_symbol_id"
        )
    winner_status = icf.get("winner_status")
    for symbol_id in identity_by_id:
        symbol = symbols.get(symbol_id)
        tracker_group = symbol.get("icf_address_group") if isinstance(symbol, Mapping) else None
        if not isinstance(tracker_group, Mapping):
            failures.append(
                f"catalog .text alias identity {symbol_id!r} lacks tracker ICF classification"
            )
            continue
        tracker_winner = tracker_group.get("winner_identity_key")
        if tracker_winner != winner_symbol_id:
            failures.append(
                f"catalog .text alias identity {symbol_id!r} has tracker ICF winner "
                f"{tracker_winner!r}, expected {winner_symbol_id!r}"
            )
        if winner_status != tracker_group.get("winner_status"):
            failures.append(
                f"catalog .text alias identity {symbol_id!r} ICF winner_status differs from tracker"
            )
    return failures


def _validate_data_source_binding(
    *,
    entity: Mapping[str, Any],
    section: Mapping[str, Any],
    projection: Mapping[str, Any],
    symbols: Mapping[str, Any],
    storage_contributions: Mapping[str, Any],
    output_sections: Mapping[str, Any],
) -> list[str]:
    failures: list[str] = []
    entity_id = entity.get("id")
    source_id = entity.get("source_id")
    if not isinstance(source_id, str) or not source_id:
        return [f"catalog section {section['name']}: entity {entity_id!r} lacks source_id"]
    storage_id: str | None = None
    symbol_id: str | None = None
    storage = storage_contributions.get(source_id)
    if isinstance(storage, Mapping):
        storage_id = source_id
        source_symbols = storage.get("symbol_ids")
        if not isinstance(source_symbols, list) or len(source_symbols) != 1:
            return [
                f"catalog data source {source_id!r} must resolve to exactly one tracker symbol"
            ]
        symbol_id = str(source_symbols[0])
    else:
        symbol = symbols.get(source_id)
        if isinstance(symbol, Mapping):
            symbol_id = source_id
            source_storage = symbol.get("storage_contribution_ids")
            if not isinstance(source_storage, list) or len(source_storage) != 1:
                return [
                    f"catalog data source {source_id!r} must resolve to exactly one storage contribution"
                ]
            storage_id = str(source_storage[0])
            storage = storage_contributions.get(storage_id)
    symbol = symbols.get(symbol_id) if symbol_id is not None else None
    if not isinstance(storage, Mapping) or not isinstance(symbol, Mapping):
        return [f"catalog data source {source_id!r} does not resolve to tracker storage and symbol rows"]
    if storage.get("binary") != "recoil" or symbol.get("binary") != "recoil":
        failures.append(f"catalog data source {source_id!r} does not belong to recoil")
    section_name = str(section["name"])
    section_id = f"recoil:section:{section_name}"
    tracker_section = output_sections.get(section_id)
    if not isinstance(tracker_section, Mapping) or not _tracker_section_matches_projection(
        tracker_section, section, projection
    ):
        failures.append(f"catalog data source {source_id!r} lacks tracker output section {section_id}")
    if storage.get("output_section_id") != section_id or symbol.get("output_section_id") != section_id:
        failures.append(
            f"catalog data source {source_id!r} storage/symbol section does not exactly match {section_id}"
        )
    if symbol_id not in storage.get("symbol_ids", []):
        failures.append(f"catalog data source {source_id!r} storage does not link symbol {symbol_id!r}")
    if storage_id not in symbol.get("storage_contribution_ids", []):
        failures.append(f"catalog data source {source_id!r} symbol does not link storage {storage_id!r}")
    entity_extent = _catalog_entity_extent(entity, section, projection)
    symbol_extent = _tracker_extent(symbol)
    reference = storage.get("reference")
    storage_extent = _tracker_extent(reference) if isinstance(reference, Mapping) else None
    if entity_extent is None or symbol_extent != entity_extent or storage_extent != entity_extent:
        failures.append(
            f"catalog data source {source_id!r} extent does not exactly match tracker storage/symbol: "
            f"catalog={entity_extent}, storage={storage_extent}, symbol={symbol_extent}"
        )
    return failures


def _validate_catalog(
    catalog: Mapping[str, Any],
    reference_projection: Mapping[str, Any],
    tracker: Mapping[str, Any] | None = None,
) -> list[str]:
    failures: list[str] = []
    tracker_ready = isinstance(tracker, Mapping) and tracker.get("schema_version") == 5
    if not tracker_ready:
        failures.append("catalog tracker cross-resolution requires a schema-v5 tracker document")
    tracker_rows = tracker if isinstance(tracker, Mapping) else {}
    symbols = tracker_rows.get("symbols", {})
    blocks = tracker_rows.get("physical_blocks", {})
    storage_contributions = tracker_rows.get("storage_contributions", {})
    output_sections = tracker_rows.get("output_sections", {})
    if not isinstance(symbols, Mapping):
        symbols = {}
        failures.append("catalog tracker symbols collection is missing")
    if not isinstance(blocks, Mapping):
        blocks = {}
        failures.append("catalog tracker physical_blocks collection is missing")
    if not isinstance(storage_contributions, Mapping):
        storage_contributions = {}
        failures.append("catalog tracker storage_contributions collection is missing")
    if not isinstance(output_sections, Mapping):
        output_sections = {}
        failures.append("catalog tracker output_sections collection is missing")
    if catalog.get("version") != CATALOG_VERSION:
        failures.append(f"catalog version must be {CATALOG_VERSION}")
    if catalog.get("binary") != "recoil":
        failures.append("catalog binary must be recoil")
    section_catalog = catalog.get("sections")
    if not isinstance(section_catalog, Mapping):
        return [*failures, "catalog sections must be an object"]
    expected_names = [str(row["name"]) for row in reference_projection["sections"]]
    if set(section_catalog) != set(expected_names):
        failures.append(
            "catalog section population differs from retail: "
            f"expected={expected_names}, catalog={sorted(section_catalog)}"
        )
    seen_entity_ids: set[str] = set()
    for section in reference_projection["sections"]:
        name = str(section["name"])
        row = section_catalog.get(name)
        if not isinstance(row, Mapping):
            continue
        entities = row.get("entities")
        if not isinstance(entities, list) or any(not isinstance(item, Mapping) for item in entities):
            failures.append(f"catalog section {name}: entities must be a list of objects")
            continue
        for entity in entities:
            entity_id = entity.get("id")
            if not isinstance(entity_id, str) or not entity_id:
                failures.append(f"catalog section {name}: every entity requires a stable semantic id")
            elif entity_id in seen_entity_ids:
                failures.append(f"catalog entity id is duplicated: {entity_id}")
            else:
                seen_entity_ids.add(entity_id)
            kind = entity.get("kind")
            allowed = TEXT_ENTITY_KINDS if name == ".text" else DATA_ENTITY_KINDS if name in {".rdata", ".data"} else OTHER_ENTITY_KINDS
            if kind not in allowed:
                failures.append(f"catalog section {name}: entity {entity_id!r} has unsupported kind {kind!r}")
            if name in {".rdata", ".data"} and not isinstance(entity.get("source_id"), str):
                failures.append(f"catalog section {name}: entity {entity_id!r} lacks source/storage identity")
            if name == ".text" and kind in {"address-group", "provider", "compiler"}:
                identities = entity.get("identities")
                if not isinstance(identities, list) or not identities:
                    failures.append(f"catalog .text entity {entity_id!r} lacks selected identities")
                else:
                    for identity in identities:
                        if not isinstance(identity, Mapping):
                            failures.append(f"catalog .text entity {entity_id!r} has malformed identity")
                            continue
                        for field in (
                            "symbol_id",
                            "map_symbol",
                            "object",
                            "source_block_id",
                            "contribution_class",
                        ):
                            if not isinstance(identity.get(field), str) or not identity.get(field):
                                failures.append(
                                    f"catalog .text identity in {entity_id!r} lacks {field}"
                                )
                        if not isinstance(identity.get("comdat"), bool):
                            failures.append(f"catalog .text identity in {entity_id!r} lacks COMDAT classification")
                        failures.extend(
                            _validate_text_identity_binding(
                                entity=entity,
                                identity=identity,
                                section=section,
                                projection=reference_projection,
                                symbols=symbols,
                                blocks=blocks,
                                output_sections=output_sections,
                            )
                        )
                    if len(identities) > 1:
                        failures.extend(_validate_icf_binding(entity, identities, symbols))
                relocations = entity.get("relocations")
                if not isinstance(relocations, list):
                    failures.append(f"catalog .text entity {entity_id!r} lacks relocation inventory")
            if name in {".rdata", ".data"} and kind in DATA_ENTITY_KINDS - {"padding"}:
                failures.extend(
                    _validate_data_source_binding(
                        entity=entity,
                        section=section,
                        projection=reference_projection,
                        symbols=symbols,
                        storage_contributions=storage_contributions,
                        output_sections=output_sections,
                    )
                )
            if kind == "pointer-data" and not isinstance(entity.get("pointers"), list):
                failures.append(f"catalog pointer entity {entity_id!r} lacks pointer inventory")
        failures.extend(
            _validate_interval_cover(
                entities,
                start_key="file_start",
                end_key="file_end",
                extent=int(section["raw_size"]),
                label=f"catalog {name} file coverage",
            )
        )
        failures.extend(
            _validate_interval_cover(
                entities,
                start_key="virtual_start",
                end_key="virtual_end",
                extent=int(section["virtual_size"]),
                label=f"catalog {name} virtual coverage",
            )
        )
    overlay = catalog.get("overlay")
    if not isinstance(overlay, Mapping) or overlay.get("mode") != "exact":
        failures.append("catalog overlay must explicitly require exact semantic coverage")
    if catalog.get("directories") != "exact-including-absence":
        failures.append("catalog must require exact directories including required absence")
    return failures


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


def _validate_text_population(
    catalog: Mapping[str, Any],
    projection: Mapping[str, Any],
    map_rows: Sequence[Mapping[str, Any]],
) -> list[str]:
    failures: list[str] = []
    text = catalog.get("sections", {}).get(".text", {})
    entities = text.get("entities", []) if isinstance(text, Mapping) else []
    expected: list[tuple[int, str, str]] = []
    image_base = int(projection["image_base"])
    for entity in entities:
        if not isinstance(entity, Mapping) or entity.get("kind") not in {"address-group", "provider", "compiler"}:
            continue
        virtual_start = entity.get("virtual_start")
        if not isinstance(virtual_start, int):
            continue
        for identity in entity.get("identities", []):
            if not isinstance(identity, Mapping):
                continue
            expected.append(
                (
                    image_base + next(
                        int(section["virtual_address"])
                        for section in projection["sections"]
                        if section["name"] == ".text"
                    ) + virtual_start,
                    str(identity.get("map_symbol", "")),
                    str(identity.get("object", "")),
                )
            )
    text_section = next(section for section in projection["sections"] if section["name"] == ".text")
    text_start = image_base + int(text_section["virtual_address"])
    text_end = text_start + int(text_section["virtual_size"])
    observed = [
        (int(row["address"]), str(row["symbol"]), str(row["object"]))
        for row in map_rows
        if text_start <= int(row["address"]) < text_end
    ]
    expected_counts = Counter(expected)
    observed_counts = Counter(observed)
    missing = sorted((expected_counts - observed_counts).elements())
    unexpected = sorted((observed_counts - expected_counts).elements())
    if missing:
        failures.append(f".text selected MAP population is missing {len(missing)} identity row(s): {missing[:8]}")
    if unexpected:
        failures.append(f".text selected MAP population has {len(unexpected)} unexpected row(s): {unexpected[:8]}")
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


def _resolved_operand(
    data: bytes,
    *,
    file_offset: int,
    width: int,
    kind: str,
    operand_va: int,
    image_base: int,
) -> int:
    if width not in {1, 2, 4}:
        raise LiveFinalError(f"unsupported catalog operand width {width}")
    raw = int.from_bytes(data[file_offset : file_offset + width], "little", signed=kind == "rel32")
    if kind == "rel32":
        return operand_va + width + raw
    if kind == "rva32":
        return image_base + raw
    if kind == "absolute32":
        return raw
    raise LiveFinalError(f"unsupported catalog relocation kind {kind!r}")


def _validate_operands(
    data: bytes,
    projection: Mapping[str, Any],
    catalog: Mapping[str, Any],
    *,
    label: str,
) -> list[str]:
    failures: list[str] = []
    image_base = int(projection["image_base"])
    sections_by_name = {str(row["name"]): row for row in projection["sections"]}
    for section_name, section_catalog in catalog.get("sections", {}).items():
        if not isinstance(section_catalog, Mapping) or section_name not in sections_by_name:
            continue
        section = sections_by_name[section_name]
        raw_pointer = int(section["raw_pointer"])
        section_va = image_base + int(section["virtual_address"])
        for entity in section_catalog.get("entities", []):
            if not isinstance(entity, Mapping):
                continue
            inventories = []
            if entity.get("kind") in {"address-group", "provider", "compiler"}:
                inventories.append(entity.get("relocations", []))
            if entity.get("kind") == "pointer-data":
                inventories.append(entity.get("pointers", []))
            for inventory in inventories:
                for operand in inventory if isinstance(inventory, list) else []:
                    if not isinstance(operand, Mapping):
                        failures.append(f"{label}: malformed operand in {entity.get('id')!r}")
                        continue
                    offset = operand.get("offset")
                    width = operand.get("width")
                    kind = operand.get("type")
                    target_rva = operand.get("target_rva")
                    addend = operand.get("addend", 0)
                    entity_file_start = entity.get("file_start")
                    entity_virtual_start = entity.get("virtual_start")
                    if not all(isinstance(value, int) and not isinstance(value, bool) for value in (offset, width, target_rva, addend, entity_file_start, entity_virtual_start)) or not isinstance(kind, str):
                        failures.append(f"{label}: incomplete operand catalog in {entity.get('id')!r}")
                        continue
                    file_offset = raw_pointer + entity_file_start + offset
                    operand_va = section_va + entity_virtual_start + offset
                    if file_offset < 0 or file_offset + width > len(data):
                        failures.append(f"{label}: operand lies outside image in {entity.get('id')!r}")
                        continue
                    resolved = _resolved_operand(
                        data,
                        file_offset=file_offset,
                        width=width,
                        kind=kind,
                        operand_va=operand_va,
                        image_base=image_base,
                    )
                    expected = image_base + target_rva + addend
                    if resolved != expected:
                        failures.append(
                            f"{label}: {entity.get('id')} operand +{offset:#x} resolves "
                            f"to {resolved:#x}, expected {expected:#x}"
                        )
    return failures


def _compare_image_data(
    candidate: Path,
    reference: Path,
    *,
    candidate_data: bytes,
    reference_data: bytes,
    catalog: Mapping[str, Any] | None = None,
    coverage: Mapping[str, Any] | None = None,
    candidate_map_rows: Iterable[Any],
    tracker: Mapping[str, Any] | None = None,
) -> dict[str, Any]:
    reference_headers = parse_pe_headers(reference_data, source=str(reference))
    candidate_headers = parse_pe_headers(candidate_data, source=str(candidate))
    reference_projection = semantic_projection(reference_data, source=str(reference))
    candidate_projection = semantic_projection(candidate_data, source=str(candidate))
    if (catalog is None) == (coverage is None):
        raise LiveFinalError("final image comparison requires exactly one live coverage or legacy catalog")
    if coverage is not None:
        failures = validate_coverage_view(
            coverage,
            reference_headers=reference_headers,
            reference_size=len(reference_data),
        )
    else:
        failures = _validate_catalog(catalog or {}, reference_projection, tracker)
    semantic_differences = _semantic_differences(reference_projection, candidate_projection)
    if semantic_differences:
        failures.append(f"PE semantic projection differs in {len(semantic_differences)} reported field(s)")

    reference_header = _header_bytes_without_timestamp(reference_data, reference_headers)
    candidate_header = _header_bytes_without_timestamp(candidate_data, candidate_headers)
    if reference_header != candidate_header:
        failures.append("PE headers differ outside the candidate COFF TimeDateStamp field")

    normalized_complete_file_equal = (
        _complete_image_bytes_without_timestamp(reference_data, reference_headers)
        == _complete_image_bytes_without_timestamp(candidate_data, candidate_headers)
    )
    if not normalized_complete_file_equal:
        failures.append(
            "complete PE file bytes differ outside the COFF TimeDateStamp field"
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
        passed = reference_payload == candidate_payload
        section_results.append({"name": name, "passed": passed, "size": len(reference_payload)})
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
    if coverage is not None:
        failures.extend(
            _validate_coverage_text_population(coverage, candidate_projection, normalized_map_rows)
        )
    else:
        legacy_catalog = catalog or {}
        failures.extend(
            _validate_text_population(legacy_catalog, candidate_projection, normalized_map_rows)
        )
        reference_operand_failures = _validate_operands(
            reference_data, reference_projection, legacy_catalog, label="retail catalog"
        )
        if reference_operand_failures:
            failures.extend(reference_operand_failures)
        failures.extend(
            _validate_operands(
                candidate_data,
                candidate_projection,
                legacy_catalog,
                label="candidate",
            )
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
        "coverage_mode": "live-generated" if coverage is not None else "legacy-catalog",
        "coverage_version": coverage.get("version") if coverage is not None else None,
        "catalog_version": catalog.get("version") if catalog is not None else None,
        "catalog_complete": (
            coverage.get("complete") is True
            if coverage is not None
            else not any("catalog" in failure or "unmodeled" in failure for failure in failures)
        ),
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
        "raw_file_equal_diagnostic": candidate_data == reference_data,
        "raw_difference_ranges_diagnostic": raw_differences,
        "timestamp_only_raw_difference": timestamp_only,
    }


def compare_images(
    candidate: Path,
    reference: Path,
    *,
    catalog: Mapping[str, Any] | None = None,
    coverage: Mapping[str, Any] | None = None,
    candidate_map_rows: Iterable[Any],
    tracker: Mapping[str, Any] | None = None,
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
            catalog=catalog,
            coverage=coverage,
            candidate_map_rows=candidate_map_rows,
            tracker=tracker,
        )
        report["retail_physical_identity"] = retail_handle.identity.to_dict()
        report["candidate_physical_identity"] = candidate_handle.identity.to_dict()
        if coverage is not None:
            expected_identity = coverage.get("retail_physical_identity")
            if isinstance(expected_identity, Mapping) and dict(expected_identity) != retail_handle.identity.to_dict():
                report["semantic_failures"].append(
                    "retail physical file identity changed after typed coverage derivation"
                )
                report["passed"] = False
        return report


def _load_catalog(
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


def _load_catalog_from_open_retail(
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
        validation_model, tracker = _load_catalog_from_open_retail(
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
        if validation_model.get("kind") == "live-final-image-coverage":
            report = compare_images(
                candidate,
                args.reference,
                coverage=validation_model,
                candidate_map_rows=parsed_map.symbols,
                tracker=tracker,
            )
        else:
            report = compare_images(
                candidate,
                args.reference,
                catalog=validation_model,
                candidate_map_rows=parsed_map.symbols,
                tracker=tracker,
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
        print(f"- catalog complete: {str(report['catalog_complete']).lower()}")
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
