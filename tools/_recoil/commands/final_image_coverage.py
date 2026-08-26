from __future__ import annotations

from collections import Counter, defaultdict
from pathlib import Path
from typing import Any, Iterable, Mapping, Sequence

from _recoil.lib.pe import DIRECTORY_NAMES, PeHeaders, PeSection, parse_pe_headers
from _recoil.lib.progress import EXACT_LINK_DIMENSIONS, FULL_ORDER_DIMENSIONS


COVERAGE_VERSION = 1
TRACKER_SCHEMA_VERSION = 5
CORE_TYPED_SECTIONS = {".text", ".rdata", ".data"}
MECHANICAL_SECTION_KINDS = {
    ".rsrc": "resource",
    ".reloc": "relocations",
}


def _integer(value: Any) -> int | None:
    if isinstance(value, int) and not isinstance(value, bool):
        return value
    if isinstance(value, str):
        try:
            return int(value, 0)
        except ValueError:
            return None
    return None


def _known_extent(row: Mapping[str, Any]) -> tuple[int, int] | None:
    if row.get("extent_state") != "known":
        return None
    start = _integer(row.get("address"))
    end = _integer(row.get("end_exclusive"))
    if end is None:
        size = _integer(row.get("size"))
        end = start + size if start is not None and size is not None else None
    if start is None or end is None or end <= start:
        return None
    return start, end


def _accepted_state(value: Any) -> bool:
    return (
        isinstance(value, Mapping)
        and value.get("result") == "passed"
        and value.get("disposition") == "accepted"
        and value.get("freshness") == "current"
        and value.get("gating") is True
        and value.get("validation_mode") == "live"
    )


def _full_order_accepted(block: Mapping[str, Any]) -> bool:
    order = block.get("order")
    full = order.get("full") if isinstance(order, Mapping) else None
    facts = block.get("accepted_order_facts")
    return (
        isinstance(full, Mapping)
        and all(_accepted_state(full.get(dimension)) for dimension in FULL_ORDER_DIMENSIONS)
        and isinstance(facts, Mapping)
        and facts.get("phase") == "full-function-order"
        and facts.get("validation_mode") == "live"
    )


def _linked_byte_accepted(symbol: Mapping[str, Any]) -> bool:
    state = symbol.get("binary_state")
    facts = symbol.get("accepted_byte_facts")
    return (
        isinstance(state, Mapping)
        and all(_accepted_state(state.get(dimension)) for dimension in EXACT_LINK_DIMENSIONS)
        and isinstance(facts, Mapping)
        and facts.get("lane") == "linked"
        and facts.get("validation_mode") == "live"
    )


def _storage_accepted(storage: Mapping[str, Any]) -> bool:
    applicability = storage.get("applicability")
    verification = storage.get("verification")
    if not isinstance(applicability, Mapping) or not isinstance(verification, Mapping):
        return False
    required = [str(name) for name, applies in applicability.items() if applies is True]
    return bool(required) and all(_accepted_state(verification.get(name)) for name in required)


def _section_projection(section: PeSection, image_base: int) -> dict[str, Any]:
    return {
        "name": section.name,
        "image_address": image_base + section.virtual_address,
        "rva": section.virtual_address,
        "virtual_size": section.virtual_size,
        "raw_pointer": section.raw_pointer,
        "raw_size": section.raw_size,
        "characteristics": section.characteristics,
    }


def _tracker_section_failures(
    section: PeSection,
    headers: PeHeaders,
    tracker_row: Any,
) -> list[str]:
    section_id = f"recoil:section:{section.name}"
    if not isinstance(tracker_row, Mapping):
        return [f"tracker output section {section_id} is missing"]
    failures: list[str] = []
    if tracker_row.get("binary") != "recoil" or tracker_row.get("name") != section.name:
        failures.append(f"tracker output section {section_id} has the wrong identity")
    reference = tracker_row.get("reference")
    if not isinstance(reference, Mapping):
        return [*failures, f"tracker output section {section_id} lacks reference facts"]
    expected = {
        "image_address": headers.image_base + section.virtual_address,
        "rva": section.virtual_address,
        "virtual_size": section.virtual_size,
        "raw_pointer": section.raw_pointer,
        "raw_size": section.raw_size,
        "characteristics": section.characteristics,
    }
    for field, expected_value in expected.items():
        actual = _integer(reference.get(field))
        if actual != expected_value:
            failures.append(
                f"tracker output section {section_id} {field} differs from retail: "
                f"tracker={actual!r}, retail={expected_value:#x}"
            )
    return failures


def _interval_analysis(
    rows: Sequence[Mapping[str, Any]],
    *,
    extent: int,
    start_key: str = "start",
    end_key: str = "end",
) -> dict[str, Any]:
    valid: list[tuple[int, int, str]] = []
    invalid: list[dict[str, Any]] = []
    for row in rows:
        start = _integer(row.get(start_key))
        end = _integer(row.get(end_key))
        row_id = str(row.get("id", ""))
        if start is None or end is None or start < 0 or end <= start or end > extent:
            invalid.append({"id": row_id, "start": start, "end": end})
            continue
        valid.append((start, end, row_id))
    valid.sort(key=lambda item: (item[0], item[1], item[2]))
    gaps: list[dict[str, int]] = []
    overlaps: list[dict[str, Any]] = []
    cursor = 0
    owner = ""
    for start, end, row_id in valid:
        if start > cursor:
            gaps.append({"start": cursor, "end": start})
        elif start < cursor:
            overlaps.append(
                {
                    "start": start,
                    "end": min(cursor, end),
                    "left_id": owner,
                    "right_id": row_id,
                }
            )
        if end > cursor:
            cursor = end
            owner = row_id
    if cursor < extent:
        gaps.append({"start": cursor, "end": extent})
    return {
        "extent": extent,
        "interval_count": len(valid),
        "covered_bytes": sum(end - start for start, end, _ in valid),
        "gaps": gaps,
        "overlaps": overlaps,
        "invalid_intervals": invalid,
        "complete": extent == 0 or (not gaps and not overlaps and not invalid),
    }


def _topology_rows(headers: PeHeaders, file_size: int) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    file_rows: list[dict[str, Any]] = []
    first_raw = min((section.raw_pointer for section in headers.sections), default=file_size)
    if first_raw:
        file_rows.append({"id": "retail:file:headers", "kind": "headers", "start": 0, "end": first_raw})
    sorted_sections = sorted(headers.sections, key=lambda section: section.raw_pointer)
    file_cursor = first_raw
    for section in sorted_sections:
        if section.raw_pointer > file_cursor:
            file_rows.append(
                {
                    "id": f"retail:file:padding:{file_cursor:x}",
                    "kind": "file-alignment-padding",
                    "start": file_cursor,
                    "end": section.raw_pointer,
                }
            )
        if section.raw_size:
            file_rows.append(
                {
                    "id": f"retail:section:{section.name}:raw",
                    "kind": "section-raw-container",
                    "section": section.name,
                    "start": section.raw_pointer,
                    "end": section.raw_pointer + section.raw_size,
                }
            )
        file_cursor = max(file_cursor, section.raw_pointer + section.raw_size)
    if file_cursor < file_size:
        file_rows.append(
            {
                "id": "retail:file:overlay",
                "kind": "overlay",
                "start": file_cursor,
                "end": file_size,
            }
        )

    rva_rows: list[dict[str, Any]] = []
    sorted_virtual = sorted(headers.sections, key=lambda section: section.virtual_address)
    rva_cursor = 0
    for section in sorted_virtual:
        if section.virtual_address > rva_cursor:
            rva_rows.append(
                {
                    "id": f"retail:rva:alignment:{rva_cursor:x}",
                    "kind": "headers-or-section-alignment",
                    "start": rva_cursor,
                    "end": section.virtual_address,
                }
            )
        if section.virtual_size:
            rva_rows.append(
                {
                    "id": f"retail:section:{section.name}:virtual",
                    "kind": "section-virtual-container",
                    "section": section.name,
                    "start": section.virtual_address,
                    "end": section.virtual_address + section.virtual_size,
                }
            )
        rva_cursor = max(rva_cursor, section.virtual_address + section.virtual_size)
    if rva_cursor < headers.size_of_image:
        rva_rows.append(
            {
                "id": f"retail:rva:alignment:{rva_cursor:x}",
                "kind": "section-alignment",
                "start": rva_cursor,
                "end": headers.size_of_image,
            }
        )
    return file_rows, rva_rows


def _group_exact_extents(rows: Iterable[tuple[str, Mapping[str, Any], tuple[int, int]]]) -> list[dict[str, Any]]:
    groups: dict[tuple[int, int], list[tuple[str, Mapping[str, Any]]]] = defaultdict(list)
    for row_id, row, extent in rows:
        groups[extent].append((row_id, row))
    result: list[dict[str, Any]] = []
    for (start, end), members in sorted(groups.items()):
        result.append(
            {
                "start_va": start,
                "end_va": end,
                "members": sorted(members, key=lambda item: item[0]),
            }
        )
    return result


def _target_map_binding(
    symbol_id: str,
    symbol: Mapping[str, Any],
    verification_targets: Mapping[str, Any],
) -> tuple[dict[str, Any] | None, str | None]:
    direct_symbol = symbol.get("map_symbol")
    direct_object = symbol.get("object")
    if isinstance(direct_symbol, str) and direct_symbol and isinstance(direct_object, str) and direct_object:
        return {"symbol": direct_symbol, "object": direct_object}, None
    address = _integer(symbol.get("address"))
    bindings: set[tuple[str, str]] = set()
    for target_id in symbol.get("verification_target_ids", []):
        target = verification_targets.get(target_id)
        if not isinstance(target, Mapping) or target.get("kind") != "vc5":
            continue
        registration = target.get("registration")
        if not isinstance(registration, Mapping):
            continue
        source_from = registration.get("source_from")
        object_name = registration.get("object")
        if not isinstance(object_name, str) or not object_name:
            object_name = f"{Path(source_from).stem}.obj" if isinstance(source_from, str) and source_from else ""
        candidates: list[Mapping[str, Any]] = []
        functions = registration.get("functions")
        if isinstance(functions, list):
            candidates.extend(item for item in functions if isinstance(item, Mapping))
        translation_units = registration.get("translation_unit_function_order")
        if isinstance(translation_units, list):
            for unit in translation_units:
                if not isinstance(unit, Mapping):
                    continue
                unit_source = unit.get("source_from")
                unit_object = unit.get("object")
                if not isinstance(unit_object, str) or not unit_object:
                    unit_object = f"{Path(unit_source).stem}.obj" if isinstance(unit_source, str) and unit_source else object_name
                unit_functions = unit.get("functions")
                if not isinstance(unit_functions, list):
                    continue
                for item in unit_functions:
                    if not isinstance(item, Mapping) or _integer(item.get("address")) != address:
                        continue
                    map_symbol = item.get("symbol")
                    if isinstance(map_symbol, str) and map_symbol and unit_object:
                        bindings.add((map_symbol, unit_object))
        for item in candidates:
            if _integer(item.get("address")) != address:
                continue
            map_symbol = item.get("symbol")
            item_object = item.get("object", object_name)
            if isinstance(map_symbol, str) and map_symbol and isinstance(item_object, str) and item_object:
                bindings.add((map_symbol, item_object))
    if len(bindings) == 1:
        map_symbol, object_name = next(iter(bindings))
        return {"symbol": map_symbol, "object": object_name}, None
    if not bindings:
        return None, f"{symbol_id} lacks an exact decorated MAP symbol/object binding"
    return None, f"{symbol_id} has ambiguous MAP bindings: {sorted(bindings)!r}"


def _text_entities(
    section: PeSection,
    headers: PeHeaders,
    tracker: Mapping[str, Any],
) -> tuple[list[dict[str, Any]], list[dict[str, Any]], Counter[str]]:
    symbols = tracker.get("symbols")
    blocks = tracker.get("physical_blocks")
    targets = tracker.get("verification_targets")
    symbols = symbols if isinstance(symbols, Mapping) else {}
    blocks = blocks if isinstance(blocks, Mapping) else {}
    targets = targets if isinstance(targets, Mapping) else {}
    section_id = f"recoil:section:{section.name}"
    section_start = headers.image_base + section.virtual_address
    section_end = section_start + section.virtual_size
    known: list[tuple[str, Mapping[str, Any], tuple[int, int]]] = []
    unresolved = Counter()
    for symbol_id, symbol in symbols.items():
        if not isinstance(symbol, Mapping) or symbol.get("binary") != "recoil":
            continue
        if symbol.get("output_section_id") != section_id:
            continue
        extent = _known_extent(symbol)
        if extent is None:
            unresolved["unknown-symbol-extent"] += 1
            continue
        if extent[0] < section_start or extent[1] > section_end:
            unresolved["symbol-outside-section"] += 1
            continue
        known.append((str(symbol_id), symbol, extent))
    entities: list[dict[str, Any]] = []
    selected: list[dict[str, Any]] = []
    for group in _group_exact_extents(known):
        reasons: list[str] = []
        ordered_identities: list[tuple[tuple[int, int, str], dict[str, Any]]] = []
        for symbol_id, symbol in group["members"]:
            pipeline_class = symbol.get("pipeline_class")
            if pipeline_class not in {"authored", "authored-lifecycle", "non-authored"}:
                reasons.append(f"{symbol_id} has unresolved pipeline classification")
            block_id = symbol.get("physical_block_id")
            block = blocks.get(block_id) if isinstance(block_id, str) else None
            if not isinstance(block, Mapping) or not _full_order_accepted(block):
                reasons.append(f"{symbol_id} lacks accepted full function order")
                order_key = (0, 0, symbol_id)
            elif (
                block.get("binary") != "recoil"
                or not isinstance(block.get("contribution_ids"), list)
                or symbol_id not in block.get("contribution_ids", [])
            ):
                reasons.append(f"{symbol_id} lacks exact source-block contribution binding")
                order_key = (0, 0, symbol_id)
            else:
                order_facts = block.get("accepted_order_facts", {})
                matched = order_facts.get("matched_identities", [])
                covered = order_facts.get("covered_block_ids", [block_id])
                if not isinstance(matched, list) or symbol_id not in matched:
                    reasons.append(f"{symbol_id} lacks exact accepted full-order identity")
                    order_key = (0, 0, symbol_id)
                else:
                    block_index = covered.index(block_id) if isinstance(covered, list) and block_id in covered else 0
                    order_key = (block_index, matched.index(symbol_id), symbol_id)
            if not _linked_byte_accepted(symbol):
                reasons.append(f"{symbol_id} lacks accepted linked-byte facts")
            binding, binding_failure = _target_map_binding(symbol_id, symbol, targets)
            if binding_failure:
                reasons.append(binding_failure)
            identity = {
                "symbol_id": symbol_id,
                "source_block_id": block_id,
                "contribution_class": pipeline_class,
            }
            if "provider" in symbol:
                identity["provider"] = symbol.get("provider")
            if symbol.get("kind") == "provider-function" and not isinstance(
                symbol.get("provider"), str
            ):
                reasons.append(f"{symbol_id} lacks exact provider binding")
            if binding is not None:
                identity.update({"map_symbol": binding["symbol"], "object": binding["object"]})
            ordered_identities.append((order_key, identity))
        identities = [identity for _key, identity in sorted(ordered_identities)]
        relative_start = int(group["start_va"]) - section_start
        relative_end = int(group["end_va"]) - section_start
        entity = {
            "id": f"live:text:{relative_start:x}-{relative_end:x}",
            "kind": "address-group",
            "start": relative_start,
            "end": relative_end,
            "identities": identities,
            "eligible": not reasons,
            "blockers": sorted(set(reasons)),
        }
        if not reasons:
            entities.append(entity)
            for identity in identities:
                selected.append(
                    {
                        "address": group["start_va"],
                        "symbol": identity["map_symbol"],
                        "object": identity["object"],
                        "symbol_id": identity["symbol_id"],
                    }
                )
        else:
            for reason in set(reasons):
                if "full function order" in reason:
                    unresolved["full-order-not-accepted"] += 1
                elif "full-order identity" in reason:
                    unresolved["full-order-identity-unresolved"] += 1
                elif "linked-byte" in reason:
                    unresolved["linked-byte-not-accepted"] += 1
                elif "MAP" in reason:
                    unresolved["map-binding-unresolved"] += 1
                elif "pipeline" in reason:
                    unresolved["pipeline-class-unresolved"] += 1
                elif "source-block" in reason:
                    unresolved["source-block-binding-unresolved"] += 1
                elif "provider binding" in reason:
                    unresolved["provider-binding-unresolved"] += 1
    return entities, selected, unresolved


def _data_entities(
    section: PeSection,
    headers: PeHeaders,
    tracker: Mapping[str, Any],
) -> tuple[list[dict[str, Any]], Counter[str]]:
    storage_rows = tracker.get("storage_contributions")
    symbols = tracker.get("symbols")
    storage_rows = storage_rows if isinstance(storage_rows, Mapping) else {}
    symbols = symbols if isinstance(symbols, Mapping) else {}
    section_id = f"recoil:section:{section.name}"
    section_start = headers.image_base + section.virtual_address
    section_end = section_start + section.virtual_size
    entities: list[dict[str, Any]] = []
    unresolved = Counter()
    known_rows: list[tuple[str, Mapping[str, Any], tuple[int, int]]] = []
    for storage_id, storage in storage_rows.items():
        if not isinstance(storage, Mapping) or storage.get("binary") != "recoil":
            continue
        if storage.get("output_section_id") != section_id:
            continue
        reference = storage.get("reference")
        extent = _known_extent(reference) if isinstance(reference, Mapping) else None
        if extent is None:
            unresolved["unknown-storage-extent"] += 1
            continue
        if extent[0] < section_start or extent[1] > section_end:
            unresolved["storage-outside-section"] += 1
            continue
        known_rows.append((str(storage_id), storage, extent))
    for group in _group_exact_extents(known_rows):
        reasons: list[str] = []
        source_ids: list[str] = []
        symbol_ids: list[str] = []
        kinds: set[str] = set()
        if len(group["members"]) > 1 and any(
            storage.get("overlap") in {None, "none"}
            for _storage_id, storage in group["members"]
        ):
            reasons.append("exactly overlapping storage rows lack an explicit alias/overlap model")
        for storage_id, storage in group["members"]:
            source_ids.append(storage_id)
            kinds.add(str(storage.get("kind", "")))
            if not _storage_accepted(storage):
                reasons.append(f"{storage_id} lacks complete accepted data verification")
            linked_symbols = storage.get("symbol_ids")
            if not isinstance(linked_symbols, list) or not linked_symbols:
                reasons.append(f"{storage_id} has no linked symbol identities")
                continue
            for symbol_id in linked_symbols:
                symbol = symbols.get(symbol_id)
                if not isinstance(symbol, Mapping):
                    reasons.append(f"{storage_id} symbol {symbol_id!r} does not resolve")
                    continue
                if symbol.get("output_section_id") != section_id or _known_extent(symbol) != (
                    group["start_va"],
                    group["end_va"],
                ):
                    reasons.append(f"{storage_id} symbol {symbol_id!r} extent/section differs")
                if storage_id not in symbol.get("storage_contribution_ids", []):
                    reasons.append(f"{storage_id} symbol {symbol_id!r} lacks reciprocal storage link")
                symbol_ids.append(str(symbol_id))
        start = int(group["start_va"]) - section_start
        end = int(group["end_va"]) - section_start
        if section.name == ".data" and start >= section.raw_size:
            kind = "bss"
        elif any("pointer" in value for value in kinds):
            kind = "pointer-data"
        else:
            kind = "initialized-data"
        entity = {
            "id": f"live:{section.name[1:]}:{start:x}-{end:x}",
            "kind": kind,
            "start": start,
            "end": end,
            "source_ids": sorted(source_ids),
            "symbol_ids": sorted(set(symbol_ids)),
            "storage_kinds": sorted(kinds),
            "eligible": not reasons,
            "blockers": sorted(set(reasons)),
        }
        if reasons:
            unresolved["data-verification-incomplete"] += 1
        else:
            entities.append(entity)
    return entities, unresolved


def _coverage_failures(section_rows: Sequence[Mapping[str, Any]]) -> list[str]:
    failures: list[str] = []
    for row in section_rows:
        name = str(row["name"])
        unresolved = row.get("unresolved_annotations")
        if isinstance(unresolved, Mapping) and _integer(unresolved.get("count")):
            failures.append(
                f"section {name} has {unresolved['count']} unresolved typed annotation(s): "
                f"{unresolved.get('by_reason', {})}"
            )
        for dimension in ("file_semantic_coverage", "virtual_semantic_coverage"):
            coverage = row[dimension]
            for gap in coverage["gaps"]:
                failures.append(
                    f"section {name} {dimension}: unmodeled range "
                    f"{gap['start']:#x}..{gap['end']:#x}"
                )
            for overlap in coverage["overlaps"]:
                failures.append(
                    f"section {name} {dimension}: overlapping typed entities at "
                    f"{overlap['start']:#x}..{overlap['end']:#x}"
                )
            for invalid in coverage["invalid_intervals"]:
                failures.append(
                    f"section {name} {dimension}: invalid typed interval {invalid['id']!r}"
                )
    return failures


def derive_final_image_coverage(
    reference_data: bytes,
    tracker: Mapping[str, Any],
    *,
    source: str,
) -> dict[str, Any]:
    """Derive candidate-independent final-image coverage from retail plus accepted tracker facts."""
    headers = parse_pe_headers(reference_data, source=source)
    failures: list[str] = []
    if tracker.get("schema_version") != TRACKER_SCHEMA_VERSION:
        failures.append(
            f"progress tracker must be schema v{TRACKER_SCHEMA_VERSION} before live coverage derivation"
        )
    output_sections = tracker.get("output_sections")
    output_sections = output_sections if isinstance(output_sections, Mapping) else {}
    section_rows: list[dict[str, Any]] = []
    selected_text: list[dict[str, Any]] = []
    for section in headers.sections:
        section_id = f"recoil:section:{section.name}"
        section_failures = _tracker_section_failures(
            section,
            headers,
            output_sections.get(section_id),
        )
        failures.extend(section_failures)
        unresolved = Counter()
        if section.name == ".text":
            entities, selected, unresolved = _text_entities(section, headers, tracker)
            selected_text.extend(selected)
        elif section.name in {".rdata", ".data"}:
            entities, unresolved = _data_entities(section, headers, tracker)
        else:
            kind = MECHANICAL_SECTION_KINDS.get(section.name, "section-payload")
            entities = [
                {
                    "id": f"live:{section.name[1:] or 'section'}:mechanical",
                    "kind": kind,
                    "start": 0,
                    "end": section.virtual_size,
                    "eligible": True,
                    "retail_derived": True,
                }
            ] if section.virtual_size else []
        virtual_entities = [
            {"id": entity["id"], "start": entity["start"], "end": entity["end"]}
            for entity in entities
        ]
        file_entities = [
            {
                "id": entity["id"],
                "start": entity["start"],
                "end": min(entity["end"], section.raw_size),
            }
            for entity in entities
            if entity["start"] < section.raw_size and min(entity["end"], section.raw_size) > entity["start"]
        ]
        if section.raw_size > section.virtual_size:
            file_entities.append(
                {
                    "id": f"live:{section.name}:raw-tail-padding",
                    "start": section.virtual_size,
                    "end": section.raw_size,
                }
            )
        section_rows.append(
            {
                **_section_projection(section, headers.image_base),
                "output_section_id": section_id,
                "tracker_section_matches": not section_failures,
                "typed_entities": entities,
                "unresolved_annotations": {
                    "count": sum(unresolved.values()),
                    "by_reason": dict(sorted(unresolved.items())),
                },
                "file_semantic_coverage": _interval_analysis(
                    file_entities,
                    extent=section.raw_size,
                ),
                "virtual_semantic_coverage": _interval_analysis(
                    virtual_entities,
                    extent=section.virtual_size,
                ),
            }
        )
    file_rows, rva_rows = _topology_rows(headers, len(reference_data))
    file_topology = _interval_analysis(file_rows, extent=len(reference_data))
    rva_topology = _interval_analysis(rva_rows, extent=headers.size_of_image)
    if not file_topology["complete"]:
        failures.append("retail file-backed topology contains a gap, overlap, or invalid interval")
    if not rva_topology["complete"]:
        failures.append("retail RVA topology contains a gap, overlap, or invalid interval")
    failures.extend(_coverage_failures(section_rows))

    directories: list[dict[str, Any]] = []
    for index in range(max(headers.number_of_rva_and_sizes, len(DIRECTORY_NAMES))):
        name = DIRECTORY_NAMES[index] if index < len(DIRECTORY_NAMES) else f"directory_{index}"
        if index < len(headers.data_directories):
            directory = headers.data_directories[index]
            file_offset = directory.rva if index == 4 and directory.rva else directory.file_offset
            present = bool(directory.rva or directory.size)
            valid = (
                not present
                or (
                    directory.rva != 0
                    and directory.size != 0
                    and file_offset is not None
                    and 0 <= file_offset <= len(reference_data)
                    and file_offset + directory.size <= len(reference_data)
                )
            )
            directories.append(
                {
                    "index": index,
                    "name": name,
                    "rva_or_file_offset": directory.rva,
                    "size": directory.size,
                    "file_offset": file_offset,
                    "present": present,
                    "valid": valid,
                }
            )
            if not valid:
                failures.append(f"retail data directory {name} is not a valid typed payload")
        else:
            directories.append(
                {
                    "index": index,
                    "name": name,
                    "rva_or_file_offset": 0,
                    "size": 0,
                    "file_offset": None,
                    "present": False,
                    "valid": True,
                }
            )
    overlay_start = max(
        (section.raw_pointer + section.raw_size for section in headers.sections),
        default=len(reference_data),
    )
    coverage = {
        "version": COVERAGE_VERSION,
        "kind": "live-final-image-coverage",
        "binary": "recoil",
        "validation_mode": "live-retail-plus-accepted-tracker",
        "source": source,
        "tracker_schema": tracker.get("schema_version"),
        "tracker_revision": tracker.get("revision"),
        "reference_layout": {
            "image_base": headers.image_base,
            "size_of_image": headers.size_of_image,
            "file_size": len(reference_data),
            "section_count": headers.section_count,
            "sections": [_section_projection(section, headers.image_base) for section in headers.sections],
        },
        "file_backed_topology": {**file_topology, "entities": file_rows},
        "rva_topology": {**rva_topology, "entities": rva_rows},
        "sections": section_rows,
        "directories": directories,
        "overlay": {
            "mode": "exact",
            "start": overlay_start,
            "end": len(reference_data),
            "size": max(0, len(reference_data) - overlay_start),
        },
        "selected_text_identities": selected_text,
        "timestamp_is_diagnostic_only": True,
        "raw_whole_file_equality_is_diagnostic_only": True,
        "failure_count": len(failures),
        "failures": failures,
        "complete": not failures,
    }
    return coverage


def validate_coverage_view(
    coverage: Mapping[str, Any],
    *,
    reference_headers: PeHeaders,
    reference_size: int,
) -> list[str]:
    failures: list[str] = []
    if coverage.get("version") != COVERAGE_VERSION:
        failures.append(f"live final coverage version must be {COVERAGE_VERSION}")
    if coverage.get("kind") != "live-final-image-coverage" or coverage.get("binary") != "recoil":
        failures.append("live final coverage has the wrong identity")
    if coverage.get("complete") is not True:
        failures.append("live final coverage is incomplete")
        for failure in coverage.get("failures", [])[:32]:
            failures.append(str(failure))
    layout = coverage.get("reference_layout")
    if not isinstance(layout, Mapping):
        failures.append("live final coverage lacks its retail layout binding")
    else:
        expected = {
            "image_base": reference_headers.image_base,
            "size_of_image": reference_headers.size_of_image,
            "file_size": reference_size,
            "section_count": reference_headers.section_count,
        }
        for field, value in expected.items():
            if layout.get(field) != value:
                failures.append(f"live final coverage retail layout {field} differs")
    if coverage.get("timestamp_is_diagnostic_only") is not True:
        failures.append("live final coverage must treat the COFF timestamp as diagnostic only")
    if coverage.get("raw_whole_file_equality_is_diagnostic_only") is not True:
        failures.append("live final coverage must treat whole-file equality as diagnostic only")
    return failures


def coverage_summary(coverage: Mapping[str, Any]) -> dict[str, Any]:
    sections = coverage.get("sections")
    rows = sections if isinstance(sections, list) else []
    return {
        "complete": coverage.get("complete") is True,
        "file_backed_topology_complete": bool(
            isinstance(coverage.get("file_backed_topology"), Mapping)
            and coverage["file_backed_topology"].get("complete") is True
        ),
        "rva_topology_complete": bool(
            isinstance(coverage.get("rva_topology"), Mapping)
            and coverage["rva_topology"].get("complete") is True
        ),
        "sections": [
            {
                "name": row.get("name"),
                "tracker_section_matches": row.get("tracker_section_matches"),
                "typed_entity_count": len(row.get("typed_entities", [])),
                "unresolved_annotations": row.get("unresolved_annotations"),
                "file_gaps": row.get("file_semantic_coverage", {}).get("gaps", []),
                "file_overlaps": row.get("file_semantic_coverage", {}).get("overlaps", []),
                "virtual_gaps": row.get("virtual_semantic_coverage", {}).get("gaps", []),
                "virtual_overlaps": row.get("virtual_semantic_coverage", {}).get("overlaps", []),
            }
            for row in rows
            if isinstance(row, Mapping)
        ],
        "selected_text_identity_count": len(coverage.get("selected_text_identities", [])),
        "failure_count": coverage.get("failure_count"),
        "failures": list(coverage.get("failures", [])),
    }
