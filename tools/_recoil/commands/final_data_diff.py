#!/usr/bin/env python3
"""Diagnose final-build .data section size and variable drift."""

from __future__ import annotations

import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse
from collections import Counter
from dataclasses import asdict, dataclass, field
import json
import re
from typing import Any

from _recoil.commands.asm_verify import CoffObject, IMAGE_SCN_CNT_UNINITIALIZED_DATA
from _recoil.commands.binja_preflight import DataItem, fetch_data_items
from _recoil.commands.vc5_verify import DEFAULT_MANIFEST_DIR, load_manifests
from _recoil.lib.binja import DEFAULT_BRIDGE_URL, BinaryNinjaBridge, BridgeError
from _recoil.lib.pe import PeSection, hex32, parse_pe_headers
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH
from _recoil.lib.owner_entries import (
    DONE_STATUS,
    OwnerEntryIndex,
    TIER_BINARY_SAFE,
    normalize_address,
)
from _recoil.lib.reference_images import reference_image_keys
from _recoil.lib.source_owners import (
    SourceOwnerDocument,
    owner_data_addresses,
    primary_owners_for_entry,
)
from _recoil.lib.tooling import REPO_ROOT, configure_stdio, display_path, response_line


DEFAULT_REFERENCE = REPO_ROOT / "support" / "Recoil.exe"
DEFAULT_CANDIDATE = REPO_ROOT / "build" / "vc5-final" / "Recoil.exe"
DEFAULT_MAP = REPO_ROOT / "build" / "vc5-final" / "Recoil.map"
DEFAULT_LINK_RSP = REPO_ROOT / "build" / "vc5-final" / "rsp" / "link.rsp"
DEFAULT_PROGRESS = DEFAULT_PROGRESS_PATH
FINAL_DATA_ISSUE_KINDS = {"candidate-address-drift", "missing-candidate-map-symbol"}
FINAL_DATA_OWNER_CORRELATION_DIRECT_ISSUE_LIMIT = 250
RETIRED_ACTION_OPTIONS = (
    "--owner-actions-json",
    "--owner-action-direct-issue-limit",
    "--action-chunk-size",
    "--plan-actions-json",
    "--plan-action-json",
)
IMAGE_SCN_CNT_INITIALIZED_DATA = 0x00000040
FINAL_DATA_SUBSECTION_FOCUS_OBJECTS = (
    "zinterp_parse.obj",
    "zVideo.obj",
    "MSVCRT:ti_inst.obj",
    "ainet.obj",
    "Briefing.obj",
    "HudUiMessageBoxDialog.obj",
    "RecoilApp_Late.obj",
    "GameNet.obj",
    "hud.obj",
    "RecoilStateCredits.obj",
    "HudUiNetExitPanel.obj",
    "HudUiMpExitDialog.obj",
    "WestwoodOnlineUpgradeApi.obj",
)
FINAL_DATA_SUBSECTION_THRESHOLDS = (0x121, 0x321, 0x521)
TAIL_BYTE_SUMMARY_LIMITATION = (
    "byte composition is diagnostic only and does not prove source ownership or marker eligibility"
)
TAIL_PRINTABLE_RUN_MIN_LENGTH = 4
TAIL_PRINTABLE_SNIPPET_LIMIT = 80

MAP_SECTION_RE = re.compile(
    r"^\s*(?P<segment>[0-9A-Fa-f]{4}):(?P<offset>[0-9A-Fa-f]{8})\s+"
    r"(?P<length>[0-9A-Fa-f]+)H\s+(?P<name>\S+)\s+(?P<section_class>\S+)\s*$"
)
MAP_SYMBOL_RE = re.compile(
    r"^\s*(?P<segment>[0-9A-Fa-f]{4}):(?P<offset>[0-9A-Fa-f]{8})\s+"
    r"(?P<symbol>.+?)\s+(?P<address>[0-9A-Fa-f]{8,16})\s+(?P<tail>.+?)\s*$"
)


@dataclass(frozen=True)
class SectionFacts:
    name: str
    rva: int
    virtual_size: int
    raw_size: int
    raw_pointer: int
    zero_fill_tail: int
    image_base: int = 0
    image_address: int = 0
    characteristics: int = 0


@dataclass(frozen=True)
class SectionByteComparison:
    available: bool
    equal: bool
    reference_size: int
    candidate_size: int
    mismatch_count: int
    first_mismatch_offset: int | None = None
    first_reference_byte: int | None = None
    first_candidate_byte: int | None = None


@dataclass(frozen=True)
class SectionDelta:
    field: str
    reference: int
    candidate: int
    delta: int


@dataclass(frozen=True)
class MapSection:
    segment: int
    offset: int
    length: int
    name: str
    section_class: str

    @property
    def end(self) -> int:
        return self.offset + self.length


@dataclass(frozen=True)
class MapSymbol:
    segment: int
    offset: int
    symbol: str
    address: int
    object: str
    source: str


@dataclass(frozen=True)
class ObjectContribution:
    object: str
    data_size: int
    bss_size: int
    sections: tuple[dict[str, int | str], ...]
    object_path: str = ""


@dataclass(frozen=True)
class ManifestIssue:
    manifest: str
    target: str
    name: str
    symbol: str
    address: str
    byte_length: int
    kind: str
    detail: str
    candidate_address: str = ""


@dataclass(frozen=True)
class ManifestCoverage:
    manifest_count: int
    data_symbol_count: int
    in_reference_section: int
    symbol_name_matches: int
    exact_address_matches: int
    issues: tuple[ManifestIssue, ...]


@dataclass(frozen=True)
class ManifestDataSymbolRange:
    manifest: str
    target: str
    name: str
    symbol: str
    address: str
    start: int
    end: int
    byte_length: int
    source_from: str
    source_filename: str


@dataclass(frozen=True)
class BnCoverage:
    available: bool
    item_count: int = 0
    section_item_count: int = 0
    manifest_covered_count: int = 0
    uncovered_items: tuple[dict[str, int | str], ...] = ()
    raw_tail_items: tuple[dict[str, Any], ...] = ()
    virtual_tail_items: tuple[dict[str, Any], ...] = ()
    error: str = ""


@dataclass(frozen=True)
class FinalDataReport:
    reference: str
    candidate: str
    map: str
    link_rsp: str
    section: str
    reference_section: SectionFacts
    candidate_section: SectionFacts
    deltas: tuple[SectionDelta, ...]
    rankings: dict[str, Any]
    map_sections: tuple[dict[str, int | str], ...]
    manifest_coverage: ManifestCoverage
    bn_coverage: BnCoverage
    classifications: tuple[str, ...]
    raw_byte_comparison: SectionByteComparison = field(
        default_factory=lambda: SectionByteComparison(False, False, 0, 0, 0)
    )
    candidate_threshold_attribution: dict[str, Any] = field(default_factory=dict)
    candidate_initialized_data_thresholds: dict[str, Any] = field(default_factory=dict)
    candidate_object_subsection_attribution: dict[str, Any] = field(default_factory=dict)
    candidate_object_traces: dict[str, Any] = field(default_factory=dict)
    candidate_boundary_contribution_summary: dict[str, Any] = field(default_factory=dict)
    candidate_boundary_packing: dict[str, Any] = field(default_factory=dict)
    raw_tail_attribution: dict[str, Any] = field(default_factory=dict)
    virtual_tail_attribution: dict[str, Any] = field(default_factory=dict)
    binary: str = "recoil"
    output_section_id: str = "recoil:section:.data"
    storage_contributions: tuple[dict[str, Any], ...] = ()


@dataclass(frozen=True)
class DirectOwnerIssue:
    address: str
    range: str
    name: str
    symbol: str
    target: str
    manifest: str
    byte_length: int
    issue_kind: str
    reason: str
    status: str
    owner_address: str
    owner_name: str
    owner_target: str
    owner_group: str
    owner_tier: str
    reference_address: str
    reference_range: str
    candidate_address: str
    candidate_range: str
    owner_ids: tuple[str, ...]


@dataclass(frozen=True)
class FinalDataOwnerCorrelation:
    progress: str
    direct_issue_count: int
    direct_issue_detail_limit: int
    direct_issue_detail_count: int
    direct_issue_truncated_count: int
    direct_issues_truncated: bool
    direct_issues_scope: str
    diagnostic_owner_issue_count: int
    direct_s_tier_issue_count: int
    diagnostic_owner_ids: tuple[str, ...]
    diagnostic_owner_addresses: tuple[str, ...]
    affected_owner_ids: tuple[str, ...]
    affected_owner_addresses: tuple[str, ...]
    direct_issues: tuple[DirectOwnerIssue, ...]
    counts: dict[str, int]
    limitations: tuple[str, ...]


def signed_hex(value: int) -> str:
    sign = "+" if value >= 0 else "-"
    return f"{sign}0x{abs(value):x}"


def require_file(path: Path, label: str) -> None:
    if not path.is_file():
        raise ValueError(f"{label} does not exist: {path}")


def pe_section_facts(path: Path, section_name: str) -> SectionFacts:
    image = path.read_bytes()
    headers = parse_pe_headers(image, source=str(path))
    for section in headers.sections:
        if section.name == section_name:
            return section_facts(section, image_base=headers.image_base, image=image)
    names = ", ".join(section.name for section in headers.sections)
    raise ValueError(f"{path}: PE section {section_name!r} not found; sections: {names}")


def pe_file_alignment(path: Path) -> int:
    return parse_pe_headers(path.read_bytes(), source=str(path)).file_alignment


def section_facts(section: PeSection, *, image_base: int = 0, image: bytes = b"") -> SectionFacts:
    return SectionFacts(
        name=section.name,
        rva=section.virtual_address,
        virtual_size=section.virtual_size,
        raw_size=section.raw_size,
        raw_pointer=section.raw_pointer,
        zero_fill_tail=max(section.virtual_size - section.raw_size, 0),
        image_base=image_base,
        image_address=image_base + section.virtual_address if image_base else 0,
        characteristics=section.characteristics,
    )


def compare_section_byte_slices(
    reference_path: Path,
    candidate_path: Path,
    reference: SectionFacts,
    candidate: SectionFacts,
) -> SectionByteComparison:
    """Compare the live PE section payloads directly and expose the drift."""

    reference_image = reference_path.read_bytes()
    candidate_image = candidate_path.read_bytes()
    reference_bytes = reference_image[
        reference.raw_pointer : reference.raw_pointer + reference.raw_size
    ]
    candidate_bytes = candidate_image[
        candidate.raw_pointer : candidate.raw_pointer + candidate.raw_size
    ]
    shared = min(len(reference_bytes), len(candidate_bytes))
    differing = [
        index for index in range(shared) if reference_bytes[index] != candidate_bytes[index]
    ]
    mismatch_count = len(differing) + abs(len(reference_bytes) - len(candidate_bytes))
    if differing:
        first = differing[0]
    elif len(reference_bytes) != len(candidate_bytes):
        first = shared
    else:
        first = None
    return SectionByteComparison(
        available=True,
        equal=mismatch_count == 0,
        reference_size=len(reference_bytes),
        candidate_size=len(candidate_bytes),
        mismatch_count=mismatch_count,
        first_mismatch_offset=first,
        first_reference_byte=(
            reference_bytes[first] if first is not None and first < len(reference_bytes) else None
        ),
        first_candidate_byte=(
            candidate_bytes[first] if first is not None and first < len(candidate_bytes) else None
        ),
    )


def section_deltas(reference: SectionFacts, candidate: SectionFacts) -> tuple[SectionDelta, ...]:
    return tuple(
        SectionDelta(field=field, reference=getattr(reference, field), candidate=getattr(candidate, field), delta=getattr(candidate, field) - getattr(reference, field))
        for field in (
            "image_base",
            "rva",
            "virtual_size",
            "raw_pointer",
            "raw_size",
            "characteristics",
            "zero_fill_tail",
        )
    )


def parse_map(path: Path) -> tuple[tuple[MapSection, ...], tuple[MapSymbol, ...]]:
    sections: list[MapSection] = []
    symbols: list[MapSymbol] = []
    current_source = ""
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        section_match = MAP_SECTION_RE.match(line)
        if section_match:
            sections.append(
                MapSection(
                    segment=int(section_match.group("segment"), 16),
                    offset=int(section_match.group("offset"), 16),
                    length=int(section_match.group("length"), 16),
                    name=section_match.group("name"),
                    section_class=section_match.group("section_class"),
                )
            )
            continue
        stripped = line.strip()
        if stripped in {"Publics by Value", "Static symbols"} or stripped.startswith("Address"):
            current_source = stripped
            continue
        symbol_match = MAP_SYMBOL_RE.match(line)
        if not symbol_match:
            continue
        tail = symbol_match.group("tail").strip()
        tail_parts = tail.split()
        while tail_parts and tail_parts[0] in {"f", "i"}:
            tail_parts.pop(0)
        object_name = " ".join(tail_parts) if tail_parts else ""
        symbols.append(
            MapSymbol(
                segment=int(symbol_match.group("segment"), 16),
                offset=int(symbol_match.group("offset"), 16),
                symbol=symbol_match.group("symbol").strip(),
                address=int(symbol_match.group("address"), 16),
                object=object_name,
                source=current_source,
            )
        )
    return tuple(sections), tuple(symbols)


def data_map_section(sections: tuple[MapSection, ...], section_name: str) -> MapSection | None:
    exact = [section for section in sections if section.name == section_name]
    if exact:
        return exact[0]
    return None


def symbols_in_segment(symbols: tuple[MapSymbol, ...], segment: int) -> tuple[MapSymbol, ...]:
    return tuple(sorted((symbol for symbol in symbols if symbol.segment == segment), key=lambda item: (item.offset, item.symbol)))


def object_origin_record(object_name: str) -> dict[str, Any]:
    if ":" not in object_name or re.match(r"^[A-Za-z]:[\\/]", object_name):
        return {
            "object": object_name,
            "kind": "object",
            "library": "",
            "member": object_name,
            "provider_candidate": False,
        }
    library, member = object_name.split(":", 1)
    return {
        "object": object_name,
        "kind": "library-member",
        "library": library,
        "member": member,
        "provider_candidate": True,
    }


def map_section_record(section: MapSection) -> dict[str, int | str]:
    return {
        "segment": section.segment,
        "offset": section.offset,
        "length": section.length,
        "end": section.end,
        "name": section.name,
        "section_class": section.section_class,
    }


def symbol_records(symbols: list[MapSymbol] | tuple[MapSymbol, ...], limit: int) -> tuple[dict[str, Any], ...]:
    return tuple(
        {
            "segment": f"{symbol.segment:04x}",
            "offset": symbol.offset,
            "address": hex32(symbol.address),
            "symbol": symbol.symbol,
            "object": symbol.object,
            "object_origin": object_origin_record(symbol.object),
        }
        for symbol in symbols[:limit]
    )


def symbols_at_or_beyond(symbols: tuple[MapSymbol, ...], offset: int, limit: int) -> tuple[dict[str, Any], ...]:
    return symbol_records([symbol for symbol in symbols if symbol.offset >= offset], limit)


def boundary_symbols(symbols: tuple[MapSymbol, ...], offset: int, limit: int) -> tuple[dict[str, Any], ...]:
    before_count = limit // 2
    after_count = limit - before_count
    before = [symbol for symbol in symbols if symbol.offset < offset]
    if before_count:
        before = before[-before_count:]
    else:
        before = []
    after = [symbol for symbol in symbols if symbol.offset >= offset][:after_count]
    return symbol_records([*before, *after], limit)


def object_context_records(
    symbols: list[MapSymbol],
    contributions: dict[str, ObjectContribution],
    *,
    limit: int,
) -> tuple[dict[str, Any], ...]:
    records: list[dict[str, Any]] = []
    seen: set[str] = set()
    for symbol in symbols:
        if symbol.object in seen:
            continue
        seen.add(symbol.object)
        contribution = contributions.get(symbol.object)
        record: dict[str, Any] = {
            "object": symbol.object,
            "first_symbol": symbol.symbol,
            "first_symbol_offset": symbol.offset,
            "first_symbol_address": hex32(symbol.address),
            "object_origin": object_origin_record(symbol.object),
        }
        if contribution is not None:
            record.update(
                {
                    "data_size": contribution.data_size,
                    "bss_size": contribution.bss_size,
                    "sections": contribution.sections,
                }
            )
        records.append(record)
        if len(records) >= limit:
            break
    return tuple(records)


def map_section_at_offset(
    map_sections: tuple[MapSection, ...],
    *,
    segment: int,
    offset: int,
) -> MapSection | None:
    containing = [
        section
        for section in map_sections
        if section.segment == segment and section.offset <= offset < section.end
    ]
    if containing:
        return min(containing, key=lambda item: (item.end - item.offset, item.offset))
    return None


def boundary_section_context(
    *,
    segment: int,
    boundary_offset: int,
    map_sections: tuple[MapSection, ...],
) -> dict[str, Any]:
    preceding = [
        section
        for section in map_sections
        if section.segment == segment and section.offset <= boundary_offset and section.end <= boundary_offset
    ]
    following = [
        section
        for section in map_sections
        if section.segment == segment and section.offset >= boundary_offset
    ]
    preceding_section = max(preceding, key=lambda item: item.end) if preceding else None
    following_section = min(following, key=lambda item: item.offset) if following else None
    containing_section = map_section_at_offset(
        map_sections,
        segment=segment,
        offset=boundary_offset,
    )
    context: dict[str, Any] = {}
    if preceding_section is not None:
        context["preceding_section"] = map_section_record(preceding_section)
        context["gap_from_preceding_end"] = boundary_offset - preceding_section.end
    if following_section is not None:
        context["following_section"] = map_section_record(following_section)
        context["gap_to_following_start"] = following_section.offset - boundary_offset
    if containing_section is not None:
        context["containing_section"] = map_section_record(containing_section)
        context["slack_before_within_containing"] = boundary_offset - containing_section.offset
        context["slack_after_within_containing"] = containing_section.end - boundary_offset
    return context


def map_section_transition_record(
    *,
    name: str,
    segment: int,
    boundary_offset: int,
    candidate_section: SectionFacts,
    map_sections: tuple[MapSection, ...],
    segment_symbols: tuple[MapSymbol, ...],
    contributions: dict[str, ObjectContribution],
    limit: int,
) -> dict[str, Any]:
    before_count = limit // 2
    after_count = limit - before_count
    before_symbols = [symbol for symbol in segment_symbols if symbol.offset < boundary_offset]
    if before_count:
        before_symbols = before_symbols[-before_count:]
    else:
        before_symbols = []
    after_symbols = [symbol for symbol in segment_symbols if symbol.offset >= boundary_offset][:after_count]
    record: dict[str, Any] = {
        "name": name,
        "segment": f"{segment:04x}",
        "boundary_offset": boundary_offset,
        "boundary_address": hex32(0x400000 + candidate_section.rva + boundary_offset),
        "symbols_before": symbol_records(before_symbols, limit),
        "symbols_after": symbol_records(after_symbols, limit),
        "objects_before": object_context_records(before_symbols, contributions, limit=before_count),
        "objects_after": object_context_records(after_symbols, contributions, limit=after_count),
    }
    record.update(
        boundary_section_context(
            segment=segment,
            boundary_offset=boundary_offset,
            map_sections=map_sections,
        )
    )
    return record


def map_section_transitions(
    *,
    map_sections: tuple[MapSection, ...],
    segment_symbols: tuple[MapSymbol, ...],
    candidate_section: SectionFacts,
    data_section: MapSection | None,
    bss_boundary: int | None,
    object_rows: tuple[ObjectContribution, ...],
    limit: int,
) -> tuple[dict[str, Any], ...]:
    if data_section is None:
        return ()
    contributions = {row.object: row for row in object_rows}
    transitions = [
        map_section_transition_record(
            name=f"{data_section.name}_end",
            segment=data_section.segment,
            boundary_offset=data_section.end,
            candidate_section=candidate_section,
            map_sections=map_sections,
            segment_symbols=segment_symbols,
            contributions=contributions,
            limit=limit,
        )
    ]
    if bss_boundary is not None:
        transitions.append(
            map_section_transition_record(
                name=f"{data_section.name}_to_bss",
                segment=data_section.segment,
                boundary_offset=bss_boundary,
                candidate_section=candidate_section,
                map_sections=map_sections,
                segment_symbols=segment_symbols,
                contributions=contributions,
                limit=limit,
            )
        )
    return tuple(transitions)


def candidate_boundary_packing_record(
    *,
    name: str,
    segment: int,
    boundary_offset: int,
    candidate_section: SectionFacts,
    map_sections: tuple[MapSection, ...],
    segment_symbols: tuple[MapSymbol, ...],
    contributions: dict[str, ObjectContribution],
    limit: int,
) -> dict[str, Any]:
    before_count = limit // 2
    after_count = limit - before_count
    before_symbols = [symbol for symbol in segment_symbols if symbol.offset < boundary_offset]
    if before_count:
        before_symbols = before_symbols[-before_count:]
    else:
        before_symbols = []
    after_symbols = [symbol for symbol in segment_symbols if symbol.offset >= boundary_offset][:after_count]
    record: dict[str, Any] = {
        "name": name,
        "segment": f"{segment:04x}",
        "offset": boundary_offset,
        "address": hex32(0x400000 + candidate_section.rva + boundary_offset),
        "nearby_symbols_before": symbol_records(before_symbols, limit),
        "nearby_symbols_after": symbol_records(after_symbols, limit),
        "nearby_objects_before": object_context_records(before_symbols, contributions, limit=before_count),
        "nearby_objects_after": object_context_records(after_symbols, contributions, limit=after_count),
    }
    record.update(
        boundary_section_context(
            segment=segment,
            boundary_offset=boundary_offset,
            map_sections=map_sections,
        )
    )
    return record


def candidate_boundary_packing(
    *,
    map_sections: tuple[MapSection, ...],
    segment_symbols: tuple[MapSymbol, ...],
    candidate_section: SectionFacts,
    data_section: MapSection | None,
    bss_boundary: int | None,
    object_rows: tuple[ObjectContribution, ...],
    limit: int,
) -> dict[str, Any]:
    if data_section is None:
        return {
            "available": False,
            "reason": "candidate map section not found",
            "boundaries": (),
        }
    contributions = {row.object: row for row in object_rows}
    boundary_offsets: list[tuple[str, int]] = [
        ("map_data_end", data_section.end),
        ("candidate_raw_end", candidate_section.raw_size),
    ]
    if bss_boundary is not None:
        boundary_offsets.append(("bss_start", bss_boundary))
    boundary_offsets.append(("candidate_virtual_end", candidate_section.virtual_size))

    seen_offsets: set[tuple[str, int]] = set()
    boundaries: list[dict[str, Any]] = []
    for name, offset in boundary_offsets:
        key = (name, offset)
        if key in seen_offsets:
            continue
        seen_offsets.add(key)
        boundaries.append(
            candidate_boundary_packing_record(
                name=name,
                segment=data_section.segment,
                boundary_offset=offset,
                candidate_section=candidate_section,
                map_sections=map_sections,
                segment_symbols=segment_symbols,
                contributions=contributions,
                limit=limit,
            )
        )

    summary: dict[str, Any] = {
        "segment": f"{data_section.segment:04x}",
        "data_section_end_offset": data_section.end,
        "candidate_raw_end_offset": candidate_section.raw_size,
        "candidate_virtual_end_offset": candidate_section.virtual_size,
        "data_end_to_raw_end_slack": candidate_section.raw_size - data_section.end,
        "raw_end_to_virtual_end_slack": candidate_section.virtual_size - candidate_section.raw_size,
    }
    if bss_boundary is not None:
        summary["bss_start_offset"] = bss_boundary
        summary["data_end_to_bss_start_slack"] = bss_boundary - data_section.end
        summary["raw_end_to_bss_start_slack"] = bss_boundary - candidate_section.raw_size
        summary["bss_start_to_virtual_end_slack"] = candidate_section.virtual_size - bss_boundary
    return {
        "available": True,
        "summary": summary,
        "boundaries": tuple(boundaries),
    }


def compact_boundary_symbol_record(symbol: MapSymbol) -> dict[str, Any]:
    return {
        "segment": f"{symbol.segment:04x}",
        "offset": symbol.offset,
        "address": hex32(symbol.address),
        "symbol": symbol.symbol,
        "source": symbol.source,
    }


def boundary_object_symbol_groups(
    symbols: list[MapSymbol],
    contributions: dict[str, ObjectContribution],
) -> tuple[dict[str, Any], ...]:
    groups: list[dict[str, Any]] = []
    by_object: dict[str, dict[str, Any]] = {}
    for symbol in symbols:
        group = by_object.get(symbol.object)
        if group is None:
            contribution = contributions.get(symbol.object)
            group = {
                "object": symbol.object,
                "object_origin": object_origin_record(symbol.object),
                "symbols": [],
                "symbol_count": 0,
            }
            if contribution is not None:
                group.update(
                    {
                        "data_size": contribution.data_size,
                        "bss_size": contribution.bss_size,
                        "sections": contribution.sections,
                    }
                )
            by_object[symbol.object] = group
            groups.append(group)
        group["symbols"].append(compact_boundary_symbol_record(symbol))
        group["symbol_count"] += 1
    for group in groups:
        group["symbols"] = tuple(group["symbols"])
    return tuple(groups)


def candidate_boundary_contribution_record(
    *,
    name: str,
    segment: int,
    boundary_offset: int,
    candidate_section: SectionFacts,
    map_sections: tuple[MapSection, ...],
    segment_symbols: tuple[MapSymbol, ...],
    contributions: dict[str, ObjectContribution],
    limit: int,
) -> dict[str, Any]:
    before_count = limit // 2
    after_count = limit - before_count
    before_symbols = [symbol for symbol in segment_symbols if symbol.offset < boundary_offset]
    if before_count:
        before_symbols = before_symbols[-before_count:]
    else:
        before_symbols = []
    after_symbols = [symbol for symbol in segment_symbols if symbol.offset >= boundary_offset][:after_count]
    record: dict[str, Any] = {
        "name": name,
        "segment": f"{segment:04x}",
        "offset": boundary_offset,
        "address": hex32(0x400000 + candidate_section.rva + boundary_offset),
        "objects_before": boundary_object_symbol_groups(before_symbols, contributions),
        "objects_after": boundary_object_symbol_groups(after_symbols, contributions),
        "symbol_window": {
            "before_count": len(before_symbols),
            "after_count": len(after_symbols),
            "limit": limit,
        },
    }
    record.update(
        boundary_section_context(
            segment=segment,
            boundary_offset=boundary_offset,
            map_sections=map_sections,
        )
    )
    return record


def candidate_boundary_contribution_summary(
    *,
    map_sections: tuple[MapSection, ...],
    segment_symbols: tuple[MapSymbol, ...],
    candidate_section: SectionFacts,
    data_section: MapSection | None,
    bss_boundary: int | None,
    object_rows: tuple[ObjectContribution, ...],
    limit: int,
) -> dict[str, Any]:
    if data_section is None:
        return {
            "available": False,
            "reason": "candidate map section not found",
            "boundaries": (),
        }
    contributions = {row.object: row for row in object_rows}
    summary: dict[str, Any] = {
        "segment": f"{data_section.segment:04x}",
        "data_end_offset": data_section.end,
        "candidate_raw_end_offset": candidate_section.raw_size,
        "candidate_virtual_end_offset": candidate_section.virtual_size,
        "data_end_to_raw_end_slack": candidate_section.raw_size - data_section.end,
        "raw_end_to_virtual_end_slack": candidate_section.virtual_size - candidate_section.raw_size,
    }
    boundary_offsets: list[tuple[str, int]] = [
        ("data_end", data_section.end),
        ("candidate_raw_end", candidate_section.raw_size),
    ]
    if bss_boundary is not None:
        summary["bss_start_offset"] = bss_boundary
        summary["data_end_to_bss_start_slack"] = bss_boundary - data_section.end
        summary["raw_end_to_bss_start_slack"] = bss_boundary - candidate_section.raw_size
        summary["bss_start_to_virtual_end_slack"] = candidate_section.virtual_size - bss_boundary
        boundary_offsets.insert(1, ("bss_start", bss_boundary))
    boundary_offsets.append(("candidate_virtual_end", candidate_section.virtual_size))

    seen: set[tuple[str, int]] = set()
    boundaries: list[dict[str, Any]] = []
    for name, offset in boundary_offsets:
        key = (name, offset)
        if key in seen:
            continue
        seen.add(key)
        boundaries.append(
            candidate_boundary_contribution_record(
                name=name,
                segment=data_section.segment,
                boundary_offset=offset,
                candidate_section=candidate_section,
                map_sections=map_sections,
                segment_symbols=segment_symbols,
                contributions=contributions,
                limit=limit,
            )
        )
    return {
        "available": True,
        "summary": summary,
        "boundaries": tuple(boundaries),
    }


def aligned_raw_threshold_offsets(
    *,
    data_end: int,
    candidate_raw_end: int,
    reference_raw_end: int,
    file_alignment: int,
) -> tuple[int, ...]:
    if file_alignment <= 0:
        return ()
    thresholds: list[int] = []
    current_threshold = ((candidate_raw_end + file_alignment - 1) // file_alignment) * file_alignment
    if current_threshold <= candidate_raw_end:
        current_threshold += file_alignment
    final_threshold = max(reference_raw_end, current_threshold)
    while current_threshold <= final_threshold:
        thresholds.append(current_threshold)
        current_threshold += file_alignment
    if reference_raw_end not in thresholds and reference_raw_end > candidate_raw_end:
        thresholds.append(reference_raw_end)
    return tuple(dict.fromkeys(thresholds))


def candidate_initialized_data_threshold_record(
    *,
    target_raw_end_offset: int,
    segment: int,
    data_end: int,
    candidate_section: SectionFacts,
    reference_section: SectionFacts,
    candidate_file_alignment: int,
    map_sections: tuple[MapSection, ...],
    segment_symbols: tuple[MapSymbol, ...],
    contributions: dict[str, ObjectContribution],
    limit: int,
) -> dict[str, Any]:
    before_count = limit // 2
    after_count = limit - before_count
    before_symbols = [symbol for symbol in segment_symbols if symbol.offset < target_raw_end_offset]
    if before_count:
        before_symbols = before_symbols[-before_count:]
    else:
        before_symbols = []
    after_symbols = [symbol for symbol in segment_symbols if symbol.offset >= target_raw_end_offset][:after_count]
    previous_raw_end_offset = target_raw_end_offset - candidate_file_alignment
    record: dict[str, Any] = {
        "target_raw_end_offset": target_raw_end_offset,
        "target_raw_end_address": hex32(0x400000 + candidate_section.rva + target_raw_end_offset),
        "previous_raw_end_offset": previous_raw_end_offset,
        "bytes_needed_from_data_end": max(previous_raw_end_offset + 1 - data_end, 0),
        "bytes_to_fill_target_raw_end": max(target_raw_end_offset - data_end, 0),
        "bytes_needed_from_current_raw_end": target_raw_end_offset - candidate_section.raw_size,
        "matches_reference_raw_end": target_raw_end_offset == reference_section.raw_size,
        "reaches_reference_raw_end": target_raw_end_offset >= reference_section.raw_size,
        "nearby_symbols_before": symbol_records(before_symbols, limit),
        "nearby_symbols_after": symbol_records(after_symbols, limit),
        "nearby_objects_before": object_context_records(before_symbols, contributions, limit=before_count),
        "nearby_objects_after": object_context_records(after_symbols, contributions, limit=after_count),
        "object_groups_before": boundary_object_symbol_groups(before_symbols, contributions),
        "object_groups_after": boundary_object_symbol_groups(after_symbols, contributions),
    }
    record.update(
        boundary_section_context(
            segment=segment,
            boundary_offset=target_raw_end_offset,
            map_sections=map_sections,
        )
    )
    return record


def candidate_initialized_data_thresholds(
    *,
    map_sections: tuple[MapSection, ...],
    segment_symbols: tuple[MapSymbol, ...],
    reference_section: SectionFacts,
    candidate_section: SectionFacts,
    data_section: MapSection | None,
    bss_boundary: int | None,
    object_rows: tuple[ObjectContribution, ...],
    candidate_file_alignment: int,
    limit: int,
) -> dict[str, Any]:
    if data_section is None:
        return {
            "available": False,
            "reason": "candidate map .data section not found",
            "thresholds": (),
        }
    if candidate_file_alignment <= 0:
        return {
            "available": False,
            "reason": "candidate PE file alignment is not positive",
            "thresholds": (),
        }
    data_end = data_section.end
    thresholds = aligned_raw_threshold_offsets(
        data_end=data_end,
        candidate_raw_end=candidate_section.raw_size,
        reference_raw_end=reference_section.raw_size,
        file_alignment=candidate_file_alignment,
    )
    if not thresholds:
        return {
            "available": False,
            "reason": "no raw-aligned threshold offsets could be computed",
            "thresholds": (),
        }
    contributions = {row.object: row for row in object_rows}
    summary: dict[str, Any] = {
        "segment": f"{data_section.segment:04x}",
        "data_end_offset": data_end,
        "candidate_initialized_data_end_offset": data_end,
        "candidate_raw_end_offset": candidate_section.raw_size,
        "candidate_virtual_end_offset": candidate_section.virtual_size,
        "reference_raw_end_offset": reference_section.raw_size,
        "candidate_file_alignment": candidate_file_alignment,
        "data_end_to_candidate_raw_end_slack": candidate_section.raw_size - data_end,
        "data_end_to_reference_raw_end_bytes": reference_section.raw_size - data_end,
        "current_raw_end_to_reference_raw_end_bytes": reference_section.raw_size - candidate_section.raw_size,
    }
    if bss_boundary is not None:
        summary["bss_start_offset"] = bss_boundary
        summary["data_end_to_bss_start_slack"] = bss_boundary - data_end
        summary["raw_end_to_bss_start_slack"] = bss_boundary - candidate_section.raw_size
    return {
        "available": True,
        "summary": summary,
        "thresholds": tuple(
            candidate_initialized_data_threshold_record(
                target_raw_end_offset=offset,
                segment=data_section.segment,
                data_end=data_end,
                candidate_section=candidate_section,
                reference_section=reference_section,
                candidate_file_alignment=candidate_file_alignment,
                map_sections=map_sections,
                segment_symbols=segment_symbols,
                contributions=contributions,
                limit=limit,
            )
            for offset in thresholds
        ),
    }


def map_evidence_kind_for_section(section: MapSection | None) -> str:
    if section is None:
        return "unknown"
    name = section.name.lower()
    if name == ".data" or name.startswith(".data$"):
        return "initialized-data"
    if name == ".bss" or name.startswith(".bss$"):
        return "bss"
    return "unknown"


def coff_subsection_kind(name: str, characteristics: int = 0) -> str:
    lowered = name.lower()
    if lowered == ".data" or lowered.startswith(".data$"):
        return "initialized-data"
    if lowered == ".bss" or lowered.startswith(".bss$") or characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA:
        return "bss"
    if lowered == ".rdata" or lowered.startswith(".rdata$"):
        return "rdata"
    if characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA:
        return "initialized-data"
    return "unknown"


def object_provenance_kind(object_name: str) -> str:
    origin = object_origin_record(object_name)
    if origin["provider_candidate"]:
        return "provider/library"
    return "authored-object"


def normalized_object_sections(contribution: ObjectContribution | None) -> tuple[dict[str, Any], ...]:
    if contribution is None:
        return ()
    rows: list[dict[str, Any]] = []
    for section in contribution.sections:
        name = str(section.get("name", ""))
        characteristics = section.get("characteristics", 0)
        try:
            characteristics_int = int(characteristics)
        except (TypeError, ValueError):
            characteristics_int = 0
        row = {
            "order": int(section.get("order", 0)),
            "name": name,
            "size": int(section.get("size", 0)),
            "kind": str(section.get("kind") or coff_subsection_kind(name, characteristics_int)),
        }
        if "section_class" in section:
            row["section_class"] = section["section_class"]
        if "characteristics" in section:
            row["characteristics"] = section["characteristics"]
        if "error" in section:
            row["error"] = section["error"]
        rows.append(row)
    return tuple(sorted(rows, key=lambda item: (int(item.get("order", 0)), str(item.get("name", "")))))


def map_evidence_kind_for_symbol(
    symbol: MapSymbol,
    *,
    map_sections: tuple[MapSection, ...],
    contributions: dict[str, ObjectContribution],
) -> str:
    origin = object_origin_record(symbol.object)
    if origin["provider_candidate"]:
        return "provider/library"
    containing = map_section_at_offset(
        map_sections,
        segment=symbol.segment,
        offset=symbol.offset,
    )
    kind = map_evidence_kind_for_section(containing)
    if kind != "unknown":
        return kind
    contribution = contributions.get(symbol.object)
    if contribution is None:
        return "unknown"
    section_kinds = {
        map_evidence_kind_for_section(
            MapSection(symbol.segment, 0, 0, str(section.get("name", "")), "DATA")
        )
        for section in contribution.sections
    }
    section_kinds.discard("unknown")
    if len(section_kinds) == 1:
        return next(iter(section_kinds))
    return "unknown"


def attributed_symbol_record(
    symbol: MapSymbol,
    *,
    map_sections: tuple[MapSection, ...],
    contributions: dict[str, ObjectContribution],
) -> dict[str, Any]:
    containing = map_section_at_offset(
        map_sections,
        segment=symbol.segment,
        offset=symbol.offset,
    )
    record = {
        "segment": f"{symbol.segment:04x}",
        "offset": symbol.offset,
        "address": hex32(symbol.address),
        "symbol": symbol.symbol,
        "object": symbol.object,
        "object_origin": object_origin_record(symbol.object),
        "map_evidence_kind": map_evidence_kind_for_symbol(
            symbol,
            map_sections=map_sections,
            contributions=contributions,
        ),
        "source": symbol.source,
    }
    if containing is not None:
        record["map_section"] = map_section_record(containing)
    return record


def attributed_object_groups(
    symbols: list[MapSymbol],
    *,
    map_sections: tuple[MapSection, ...],
    contributions: dict[str, ObjectContribution],
    limit: int,
) -> tuple[dict[str, Any], ...]:
    groups: list[dict[str, Any]] = []
    by_object: dict[str, dict[str, Any]] = {}
    for symbol in symbols:
        group = by_object.get(symbol.object)
        if group is None:
            contribution = contributions.get(symbol.object)
            group = {
                "object": symbol.object,
                "object_origin": object_origin_record(symbol.object),
                "map_evidence_kind": "unknown",
                "map_evidence_kinds": [],
                "symbols": [],
                "symbol_count": 0,
            }
            if contribution is not None:
                group.update(
                    {
                        "data_size": contribution.data_size,
                        "bss_size": contribution.bss_size,
                        "sections": contribution.sections,
                    }
                )
            by_object[symbol.object] = group
            groups.append(group)
        symbol_record = attributed_symbol_record(
            symbol,
            map_sections=map_sections,
            contributions=contributions,
        )
        kind = str(symbol_record["map_evidence_kind"])
        if kind not in group["map_evidence_kinds"]:
            group["map_evidence_kinds"].append(kind)
        group["symbols"].append(symbol_record)
        group["symbol_count"] += 1

    for group in groups:
        kinds = tuple(group["map_evidence_kinds"])
        if "provider/library" in kinds:
            primary = "provider/library"
        elif len(kinds) == 1:
            primary = kinds[0]
        else:
            primary = "unknown"
        group["map_evidence_kind"] = primary
        group["map_evidence_kinds"] = kinds
        group["symbols"] = tuple(group["symbols"][:limit])
    return tuple(groups[:limit])


def candidate_threshold_attribution_point(
    *,
    name: str,
    offset: int,
    segment: int,
    candidate_section: SectionFacts,
    map_sections: tuple[MapSection, ...],
    segment_symbols: tuple[MapSymbol, ...],
    contributions: dict[str, ObjectContribution],
    limit: int,
) -> dict[str, Any]:
    before_count = limit // 2
    after_count = limit - before_count
    before_symbols = [symbol for symbol in segment_symbols if symbol.offset < offset]
    if before_count:
        before_symbols = before_symbols[-before_count:]
    else:
        before_symbols = []
    after_symbols = [symbol for symbol in segment_symbols if symbol.offset >= offset][:after_count]
    containing = map_section_at_offset(
        map_sections,
        segment=segment,
        offset=offset,
    )
    record: dict[str, Any] = {
        "name": name,
        "segment": f"{segment:04x}",
        "offset": offset,
        "address": hex32(0x400000 + candidate_section.rva + offset),
        "map_evidence_kind": map_evidence_kind_for_section(containing),
        "symbols_before": tuple(
            attributed_symbol_record(symbol, map_sections=map_sections, contributions=contributions)
            for symbol in before_symbols
        ),
        "symbols_after": tuple(
            attributed_symbol_record(symbol, map_sections=map_sections, contributions=contributions)
            for symbol in after_symbols
        ),
        "objects_before": attributed_object_groups(
            before_symbols,
            map_sections=map_sections,
            contributions=contributions,
            limit=before_count,
        ),
        "objects_after": attributed_object_groups(
            after_symbols,
            map_sections=map_sections,
            contributions=contributions,
            limit=after_count,
        ),
    }
    if containing is not None:
        record["map_section"] = map_section_record(containing)
    record.update(
        boundary_section_context(
            segment=segment,
            boundary_offset=offset,
            map_sections=map_sections,
        )
    )
    return record


def compact_threshold_attribution_record(row: dict[str, Any]) -> dict[str, Any]:
    keys = (
        "target_raw_end_offset",
        "target_raw_end_address",
        "previous_raw_end_offset",
        "bytes_needed_from_data_end",
        "bytes_to_fill_target_raw_end",
        "bytes_needed_from_current_raw_end",
        "matches_reference_raw_end",
        "reaches_reference_raw_end",
        "containing_section",
        "following_section",
        "nearby_symbols_before",
        "nearby_symbols_after",
        "nearby_objects_before",
        "nearby_objects_after",
        "object_groups_before",
        "object_groups_after",
    )
    return {key: row[key] for key in keys if key in row}


def candidate_threshold_attribution(
    *,
    thresholds: dict[str, Any],
    map_sections: tuple[MapSection, ...],
    segment_symbols: tuple[MapSymbol, ...],
    reference_section: SectionFacts,
    candidate_section: SectionFacts,
    data_section: MapSection | None,
    bss_boundary: int | None,
    object_rows: tuple[ObjectContribution, ...],
    candidate_file_alignment: int,
    limit: int,
) -> dict[str, Any]:
    if data_section is None:
        return {
            "available": False,
            "reason": "candidate map .data section not found",
            "limitations": (
                "Attribution unavailable without candidate map .data section evidence.",
                "This diagnostic is read-only and does not build, relink, generate probes, or prove source ownership.",
            ),
        }
    summary = thresholds.get("summary", {})
    data_end = int(summary.get("candidate_initialized_data_end_offset", data_section.end))
    candidate_base = 0x400000 + candidate_section.rva
    reference_base = 0x400000 + reference_section.rva
    attribution_summary: dict[str, Any] = {
        "segment": f"{data_section.segment:04x}",
        "candidate_initialized_data_end_offset": data_end,
        "candidate_initialized_data_end_address": hex32(candidate_base + data_end),
        "candidate_raw_end_offset": candidate_section.raw_size,
        "candidate_raw_end_address": hex32(candidate_base + candidate_section.raw_size),
        "candidate_virtual_end_offset": candidate_section.virtual_size,
        "candidate_virtual_end_address": hex32(candidate_base + candidate_section.virtual_size),
        "reference_raw_end_offset": reference_section.raw_size,
        "reference_raw_end_address": hex32(reference_base + reference_section.raw_size),
        "candidate_file_alignment": candidate_file_alignment,
        "data_end_to_candidate_raw_end_slack": candidate_section.raw_size - data_end,
        "data_end_to_reference_raw_end_bytes": reference_section.raw_size - data_end,
        "current_raw_end_to_reference_raw_end_bytes": reference_section.raw_size - candidate_section.raw_size,
    }
    if bss_boundary is not None:
        attribution_summary["bss_start_offset"] = bss_boundary
        attribution_summary["bss_start_address"] = hex32(candidate_base + bss_boundary)
        attribution_summary["data_end_to_bss_start_slack"] = bss_boundary - data_end
        attribution_summary["raw_end_to_bss_start_slack"] = bss_boundary - candidate_section.raw_size

    contributions = {row.object: row for row in object_rows}
    point_offsets: list[tuple[str, int]] = [
        ("candidate_initialized_data_end", data_end),
    ]
    if bss_boundary is not None:
        point_offsets.append(("bss_start", bss_boundary))
    point_offsets.append(("candidate_raw_end", candidate_section.raw_size))
    point_offsets.extend(
        (
            f"threshold_{hex32(int(row.get('target_raw_end_offset', 0)))}",
            int(row.get("target_raw_end_offset", 0)),
        )
        for row in tuple(thresholds.get("thresholds", ()))
    )

    seen_offsets: set[tuple[str, int]] = set()
    points: list[dict[str, Any]] = []
    for name, offset in point_offsets:
        key = (name, offset)
        if key in seen_offsets:
            continue
        seen_offsets.add(key)
        points.append(
            candidate_threshold_attribution_point(
                name=name,
                offset=offset,
                segment=data_section.segment,
                candidate_section=candidate_section,
                map_sections=map_sections,
                segment_symbols=segment_symbols,
                contributions=contributions,
                limit=limit,
            )
        )

    return {
        "available": True,
        "summary": attribution_summary,
        "raw_alignment_thresholds": tuple(
            compact_threshold_attribution_record(row)
            for row in tuple(thresholds.get("thresholds", ()))
        ),
        "attribution_points": tuple(points),
        "limitations": (
            "Attribution-only: map/COFF/link-response placement does not prove source ownership, source model, or data gate acceptance.",
            "Provider/library object labels are map-origin hints and require provider-boundary review before marker use.",
            "Missing or unreadable COFF objects are reported as unknown; the audit is read-only and does not build, relink, or generate probes.",
        ),
    }


def add_object_selection_reason(reasons: dict[str, list[str]], object_name: str, reason: str) -> None:
    if not object_name:
        return
    values = reasons.setdefault(object_name, [])
    if reason not in values:
        values.append(reason)


def add_selected_map_symbol(
    selected_symbols: dict[str, dict[str, MapSymbol]],
    selected_reasons: dict[tuple[str, str], list[str]],
    symbol: MapSymbol,
    reason: str,
) -> None:
    if not symbol.object or not symbol.symbol:
        return
    selected_symbols.setdefault(symbol.object, {}).setdefault(symbol.symbol, symbol)
    reason_key = (symbol.object, symbol.symbol)
    reasons = selected_reasons.setdefault(reason_key, [])
    if reason not in reasons:
        reasons.append(reason)


def select_symbols_by_manifest_name(
    *,
    symbol_name: str,
    by_symbol: dict[str, list[MapSymbol]],
    segment_symbols: tuple[MapSymbol, ...],
) -> tuple[MapSymbol, ...]:
    if not symbol_name:
        return ()
    if symbol_name.startswith("symbol_regex="):
        pattern_text = symbol_name.removeprefix("symbol_regex=")
        try:
            pattern = re.compile(pattern_text)
        except re.error:
            return ()
        return tuple(symbol for symbol in segment_symbols if pattern.fullmatch(symbol.symbol))
    return tuple(by_symbol.get(symbol_name, ()))


def collect_tail_manifest_match_symbols(
    attribution: dict[str, Any],
    *,
    by_symbol: dict[str, list[MapSymbol]],
    segment_symbols: tuple[MapSymbol, ...],
    reason: str,
    selected_symbols: dict[str, dict[str, MapSymbol]],
    selected_reasons: dict[tuple[str, str], list[str]],
    selection_reasons: dict[str, list[str]],
) -> None:
    if not attribution.get("available"):
        return
    for item in tuple(attribution.get("reference_tail_bn_items", ())):
        for match in tuple(item.get("manifest_matches", ())):
            symbol_name = str(match.get("symbol", ""))
            for symbol in select_symbols_by_manifest_name(
                symbol_name=symbol_name,
                by_symbol=by_symbol,
                segment_symbols=segment_symbols,
            ):
                add_object_selection_reason(selection_reasons, symbol.object, reason)
                add_selected_map_symbol(selected_symbols, selected_reasons, symbol, reason)


def coff_storage_class_name(storage_class: int) -> str:
    if storage_class == 2:
        return "external"
    if storage_class == 3:
        return "static"
    return str(storage_class)


def offset_relation(offset: int, boundary: int | None) -> str:
    if boundary is None:
        return "unknown"
    if offset < boundary:
        return "before"
    if offset == boundary:
        return "at"
    return "after"


def range_text(start: int, size: int) -> str:
    end = start + max(size, 0)
    return f"{hex32(start)}..{hex32(end)}"


def selected_manifest_issue_correlation_record(
    issue: ManifestIssue,
    symbol: MapSymbol,
    *,
    reference_base: int,
    reference_section: SectionFacts,
    candidate_section: SectionFacts,
    data_end: int,
    bss_boundary: int | None,
) -> dict[str, Any]:
    try:
        reference_address = int(issue.address, 16)
    except ValueError:
        reference_address = 0
    reference_offset = reference_address - reference_base if reference_address else None
    candidate_address = symbol.address
    candidate_offset = symbol.offset
    byte_length = max(issue.byte_length, 0)
    candidate_issue_address = issue.candidate_address or hex32(candidate_address)
    record: dict[str, Any] = {
        "manifest": issue.manifest,
        "target": issue.target,
        "name": issue.name,
        "issue_symbol": issue.symbol,
        "issue_kind": issue.kind,
        "issue_detail": issue.detail,
        "byte_length": byte_length,
        "reference_address": hex32(reference_address) if reference_address else issue.address,
        "candidate_issue_address": candidate_issue_address,
        "candidate_map_address": hex32(candidate_address),
        "candidate_map_offset": candidate_offset,
        "candidate_map_range": range_text(candidate_address, byte_length),
        "candidate_map_offset_range": range_text(candidate_offset, byte_length),
        "candidate_relation_to_initialized_data_end": offset_relation(candidate_offset, data_end),
        "candidate_relation_to_bss_start": offset_relation(candidate_offset, bss_boundary),
        "candidate_relation_to_raw_end": offset_relation(candidate_offset, candidate_section.raw_size),
        "candidate_relation_to_virtual_end": offset_relation(candidate_offset, candidate_section.virtual_size),
        "candidate_map_matches_issue_address": candidate_issue_address == hex32(candidate_address),
    }
    if reference_offset is not None:
        record.update(
            {
                "reference_offset": reference_offset,
                "reference_range": range_text(reference_address, byte_length),
                "reference_offset_range": range_text(reference_offset, byte_length),
                "reference_relation_to_candidate_raw_end": offset_relation(reference_offset, candidate_section.raw_size),
                "reference_relation_to_reference_raw_end": offset_relation(reference_offset, reference_section.raw_size),
                "reference_relation_to_reference_virtual_end": offset_relation(reference_offset, reference_section.virtual_size),
                "reference_in_raw_tail_gap": candidate_section.raw_size <= reference_offset < reference_section.raw_size,
                "reference_bytes_past_candidate_raw_end": reference_offset - candidate_section.raw_size,
                "candidate_offset_delta_from_reference": candidate_offset - reference_offset,
            }
        )
    return record


def resolve_contribution_object_path(contribution: ObjectContribution | None) -> Path | None:
    if contribution is None:
        return None
    path_text = contribution.object_path or contribution.object
    if not path_text or ":" in path_text and not re.match(r"^[A-Za-z]:[\\/]", path_text):
        return None
    path = Path(path_text)
    if path.is_absolute():
        return path
    return REPO_ROOT / path


def selected_coff_symbol_miss_record(
    symbol: MapSymbol,
    *,
    lookup_status: str,
    reasons: tuple[str, ...],
    manifest_issue_correlations: tuple[dict[str, Any], ...] = (),
) -> dict[str, Any]:
    return {
        "symbol": symbol.symbol,
        "object": symbol.object,
        "map_address": hex32(symbol.address),
        "map_offset": symbol.offset,
        "coff_symbol_index": None,
        "coff_section_number": None,
        "coff_section_order": None,
        "coff_section_name": "",
        "coff_section_kind": "unknown",
        "coff_value": None,
        "coff_section_size": None,
        "storage_class": None,
        "lookup_status": lookup_status,
        "selection_reasons": reasons,
        "manifest_issue_correlations": manifest_issue_correlations,
    }


def selected_coff_symbol_record(
    symbol: MapSymbol,
    *,
    coff: CoffObject | None,
    lookup_status: str,
    reasons: tuple[str, ...],
    manifest_issue_correlations: tuple[dict[str, Any], ...] = (),
) -> dict[str, Any]:
    if coff is None:
        return selected_coff_symbol_miss_record(
            symbol,
            lookup_status=lookup_status,
            reasons=reasons,
            manifest_issue_correlations=manifest_issue_correlations,
        )
    coff_symbol = coff.symbols_by_name.get(symbol.symbol)
    if coff_symbol is None:
        return selected_coff_symbol_miss_record(
            symbol,
            lookup_status="coff-symbol-missing",
            reasons=reasons,
            manifest_issue_correlations=manifest_issue_correlations,
        )
    try:
        section = coff.section(coff_symbol.section_number)
    except ValueError:
        return {
            **selected_coff_symbol_miss_record(
                symbol,
                lookup_status="coff-section-missing",
                reasons=reasons,
                manifest_issue_correlations=manifest_issue_correlations,
            ),
            "coff_symbol_index": coff_symbol.index,
            "coff_section_number": coff_symbol.section_number,
            "coff_value": coff_symbol.value,
            "storage_class": coff_symbol.storage_class,
            "storage_class_name": coff_storage_class_name(coff_symbol.storage_class),
        }
    return {
        "symbol": symbol.symbol,
        "object": symbol.object,
        "map_address": hex32(symbol.address),
        "map_offset": symbol.offset,
        "coff_symbol_index": coff_symbol.index,
        "coff_section_number": coff_symbol.section_number,
        "coff_section_order": section.index - 1,
        "coff_section_name": section.name,
        "coff_section_kind": coff_subsection_kind(section.name, section.characteristics),
        "coff_value": coff_symbol.value,
        "coff_section_size": len(section.raw_data),
        "storage_class": coff_symbol.storage_class,
        "storage_class_name": coff_storage_class_name(coff_symbol.storage_class),
        "lookup_status": "matched",
        "selection_reasons": reasons,
        "manifest_issue_correlations": manifest_issue_correlations,
    }


def selected_coff_symbol_records(
    *,
    object_name: str,
    contribution: ObjectContribution | None,
    selected_symbols: dict[str, dict[str, MapSymbol]],
    selected_reasons: dict[tuple[str, str], list[str]],
    manifest_issue_correlations: dict[tuple[str, str], tuple[dict[str, Any], ...]] | None = None,
) -> tuple[dict[str, Any], ...]:
    object_symbols = tuple(
        sorted(
            selected_symbols.get(object_name, {}).values(),
            key=lambda item: (item.offset, item.symbol),
        )
    )
    if not object_symbols:
        return ()
    object_path = resolve_contribution_object_path(contribution)
    if object_path is None:
        return tuple(
            selected_coff_symbol_miss_record(
                symbol,
                lookup_status="coff-object-path-unavailable",
                reasons=tuple(selected_reasons.get((symbol.object, symbol.symbol), ())),
                manifest_issue_correlations=tuple((manifest_issue_correlations or {}).get((symbol.object, symbol.symbol), ())),
            )
            for symbol in object_symbols
        )
    if not object_path.is_file():
        return tuple(
            selected_coff_symbol_miss_record(
                symbol,
                lookup_status="coff-object-missing",
                reasons=tuple(selected_reasons.get((symbol.object, symbol.symbol), ())),
                manifest_issue_correlations=tuple((manifest_issue_correlations or {}).get((symbol.object, symbol.symbol), ())),
            )
            for symbol in object_symbols
        )
    try:
        coff = CoffObject.from_path(object_path)
    except ValueError:
        return tuple(
            selected_coff_symbol_miss_record(
                symbol,
                lookup_status="coff-object-unreadable",
                reasons=tuple(selected_reasons.get((symbol.object, symbol.symbol), ())),
                manifest_issue_correlations=tuple((manifest_issue_correlations or {}).get((symbol.object, symbol.symbol), ())),
            )
            for symbol in object_symbols
        )
    return tuple(
        selected_coff_symbol_record(
            symbol,
            coff=coff,
            lookup_status="matched",
            reasons=tuple(selected_reasons.get((symbol.object, symbol.symbol), ())),
            manifest_issue_correlations=tuple((manifest_issue_correlations or {}).get((symbol.object, symbol.symbol), ())),
        )
        for symbol in object_symbols
    )


def object_primary_map_evidence_kind(kinds: tuple[str, ...], contribution: ObjectContribution | None) -> str:
    if "provider/library" in kinds:
        return "provider/library"
    concrete = tuple(kind for kind in kinds if kind != "unknown")
    if len(set(concrete)) == 1:
        return concrete[0]
    if concrete:
        return "mixed"
    section_kinds = {str(section.get("kind", "unknown")) for section in normalized_object_sections(contribution)}
    section_kinds.discard("unknown")
    if len(section_kinds) == 1:
        return next(iter(section_kinds))
    if section_kinds:
        return "mixed"
    return "unknown"


def object_threshold_size_records(contribution: ObjectContribution | None) -> tuple[dict[str, Any], ...]:
    data_size = contribution.data_size if contribution is not None else 0
    bss_size = contribution.bss_size if contribution is not None else 0
    return tuple(
        {
            "threshold_bytes": threshold,
            "initialized_data_size_satisfies": data_size >= threshold,
            "bss_size_satisfies": bss_size >= threshold,
            "initialized_or_bss_size_satisfies": max(data_size, bss_size) >= threshold,
        }
        for threshold in FINAL_DATA_SUBSECTION_THRESHOLDS
    )


def parse_trace_object_names(values: list[str] | tuple[str, ...] | None) -> tuple[str, ...]:
    names: list[str] = []
    seen: set[str] = set()
    for value in values or ():
        for item in str(value).split(","):
            name = item.strip().strip('"')
            if not name:
                continue
            key = name.lower()
            if key in seen:
                continue
            seen.add(key)
            names.append(name)
    return tuple(names)


def object_match_keys(value: str) -> set[str]:
    normalized = value.replace("\\", "/").strip().lower()
    keys = {normalized}
    is_library_member = ":" in normalized and not re.match(r"^[a-z]:/", normalized)
    if not is_library_member:
        keys.add(Path(normalized).name)
    else:
        _library, member = normalized.split(":", 1)
        keys.add(member)
    return {key for key in keys if key}


def object_name_matches(request: str, object_name: str) -> bool:
    request_keys = object_match_keys(request)
    object_keys = object_match_keys(object_name)
    return bool(request_keys & object_keys)


def offset_boundary_relationships(
    offset: int,
    *,
    data_end: int,
    bss_boundary: int | None,
    candidate_section: SectionFacts,
    reference_section: SectionFacts,
) -> dict[str, Any]:
    boundaries: tuple[tuple[str, int | None], ...] = (
        ("candidate_data_end", data_end),
        ("bss_start", bss_boundary),
        ("candidate_raw_end", candidate_section.raw_size),
        ("reference_raw_end", reference_section.raw_size),
        ("candidate_virtual_end", candidate_section.virtual_size),
    )
    return {
        name: {
            "boundary_offset": boundary,
            "relation": offset_relation(offset, boundary),
            "delta": None if boundary is None else offset - boundary,
        }
        for name, boundary in boundaries
    }


def coff_section_trace_records(coff: CoffObject | None) -> tuple[dict[str, Any], ...]:
    if coff is None:
        return ()
    return tuple(
        {
            "index": section.index,
            "order": section.index - 1,
            "name": section.name,
            "kind": coff_subsection_kind(section.name, section.characteristics),
            "size": len(section.raw_data),
            "characteristics": section.characteristics,
            "characteristics_hex": hex32(section.characteristics),
        }
        for section in coff.sections
    )


def object_trace_symbol_record(
    symbol: MapSymbol,
    *,
    coff: CoffObject | None,
    coff_status: str,
    map_sections: tuple[MapSection, ...],
    contributions: dict[str, ObjectContribution],
    data_end: int,
    bss_boundary: int | None,
    candidate_section: SectionFacts,
    reference_section: SectionFacts,
) -> dict[str, Any]:
    base = selected_coff_symbol_record(
        symbol,
        coff=coff,
        lookup_status="matched" if coff is not None else coff_status,
        reasons=("trace_object",),
    )
    containing = map_section_at_offset(
        map_sections,
        segment=symbol.segment,
        offset=symbol.offset,
    )
    base.update(
        {
            "map_segment": f"{symbol.segment:04x}",
            "map_rva": symbol.address - 0x400000,
            "map_evidence_kind": map_evidence_kind_for_symbol(
                symbol,
                map_sections=map_sections,
                contributions=contributions,
            ),
            "relationships": offset_boundary_relationships(
                symbol.offset,
                data_end=data_end,
                bss_boundary=bss_boundary,
                candidate_section=candidate_section,
                reference_section=reference_section,
            ),
        }
    )
    if containing is not None:
        base["map_section"] = map_section_record(containing)
    return base


def link_rsp_object_records(object_paths: tuple[Path, ...]) -> tuple[dict[str, Any], ...]:
    return tuple(
        {
            "order": index,
            "object": path.name,
            "object_path": display_path(path, REPO_ROOT),
            "path": path,
        }
        for index, path in enumerate(object_paths)
    )


def resolve_trace_link_record(
    object_name: str,
    *,
    request: str,
    link_records: tuple[dict[str, Any], ...],
) -> dict[str, Any] | None:
    for record in link_records:
        if object_name_matches(object_name, str(record["object"])):
            return record
    for record in link_records:
        if object_name_matches(request, str(record["object"])):
            return record
    return None


def trace_object_names(
    requested_objects: tuple[str, ...],
    *,
    object_rows: tuple[ObjectContribution, ...],
    segment_symbols: tuple[MapSymbol, ...],
    link_records: tuple[dict[str, Any], ...],
) -> tuple[tuple[str, str], ...]:
    known_names: list[str] = []
    for symbol in segment_symbols:
        if symbol.object and symbol.object not in known_names:
            known_names.append(symbol.object)
    for row in object_rows:
        if row.object and row.object not in known_names:
            known_names.append(row.object)
    for record in link_records:
        object_name = str(record["object"])
        if object_name and object_name not in known_names:
            known_names.append(object_name)

    selected: list[tuple[str, str]] = []
    seen: set[tuple[str, str]] = set()
    for request in requested_objects:
        matches = [name for name in known_names if object_name_matches(request, name)]
        if not matches:
            key = (request, request)
            if key not in seen:
                seen.add(key)
                selected.append(key)
            continue
        for name in matches:
            key = (request, name)
            if key in seen:
                continue
            seen.add(key)
            selected.append(key)
    return tuple(selected)


def candidate_object_traces(
    *,
    requested_objects: tuple[str, ...],
    map_sections: tuple[MapSection, ...],
    segment_symbols: tuple[MapSymbol, ...],
    reference_section: SectionFacts,
    candidate_section: SectionFacts,
    data_section: MapSection | None,
    bss_boundary: int | None,
    object_rows: tuple[ObjectContribution, ...],
    object_paths: tuple[Path, ...],
    limit: int,
) -> dict[str, Any]:
    if not requested_objects:
        return {
            "available": False,
            "reason": "no --trace-object values requested",
            "objects": (),
        }
    if data_section is None:
        return {
            "available": False,
            "reason": "candidate map .data section not found",
            "objects_requested": requested_objects,
            "objects": (),
            "limitations": (
                "Trace unavailable without candidate map .data section evidence.",
                "Trace-only: this diagnostic does not prove source ownership, data-owner acceptance, provider classification, or marker eligibility.",
            ),
        }

    data_end = data_section.end
    candidate_base = 0x400000 + candidate_section.rva
    reference_base = 0x400000 + reference_section.rva
    contributions = {row.object: row for row in object_rows}
    link_records = link_rsp_object_records(object_paths)
    symbols_by_object: dict[str, list[MapSymbol]] = {}
    for symbol in segment_symbols:
        symbols_by_object.setdefault(symbol.object, []).append(symbol)

    object_records: list[dict[str, Any]] = []
    for request, object_name in trace_object_names(
        requested_objects,
        object_rows=object_rows,
        segment_symbols=segment_symbols,
        link_records=link_records,
    ):
        contribution = contributions.get(object_name)
        link_record = resolve_trace_link_record(object_name, request=request, link_records=link_records)
        object_path_text = contribution.object_path if contribution is not None else ""
        object_path: Path | None = resolve_contribution_object_path(contribution)
        if object_path is None and link_record is not None:
            object_path_text = str(link_record["object_path"])
            record_path = link_record.get("path")
            if isinstance(record_path, Path):
                object_path = record_path
        coff: CoffObject | None = None
        coff_status = "coff-object-path-unavailable"
        coff_error = ""
        if object_path is not None:
            if object_path.is_file():
                try:
                    coff = CoffObject.from_path(object_path)
                    coff_status = "read"
                except ValueError as exc:
                    coff_status = "coff-object-unreadable"
                    coff_error = str(exc)
            else:
                coff_status = "coff-object-missing"

        object_symbols = tuple(sorted(symbols_by_object.get(object_name, ()), key=lambda item: (item.offset, item.symbol)))
        symbol_kinds = tuple(
            dict.fromkeys(
                map_evidence_kind_for_symbol(symbol, map_sections=map_sections, contributions=contributions)
                for symbol in object_symbols
            )
        )
        offsets = [symbol.offset for symbol in object_symbols]
        min_offset = min(offsets) if offsets else None
        max_offset = max(offsets) if offsets else None
        object_record: dict[str, Any] = {
            "requested_object": request,
            "object": object_name,
            "object_path": object_path_text,
            "object_origin": object_origin_record(object_name),
            "provenance_kind": object_provenance_kind(object_name),
            "found_in_link_rsp": link_record is not None,
            "link_rsp_order": int(link_record["order"]) if link_record is not None else None,
            "link_rsp_order_one_based": int(link_record["order"]) + 1 if link_record is not None else None,
            "coff_status": coff_status,
            "coff_error": coff_error,
            "coff_sections": coff_section_trace_records(coff),
            "map_contribution": {
                "map_symbol_count": len(object_symbols),
                "map_offset_min": min_offset,
                "map_offset_max": max_offset,
                "map_offset_range": range_text(min_offset, max_offset - min_offset + 1) if min_offset is not None and max_offset is not None else "",
                "map_evidence_kind": object_primary_map_evidence_kind(symbol_kinds, contribution),
                "map_evidence_kinds": symbol_kinds,
                "initialized_data_contribution_size": contribution.data_size if contribution is not None else 0,
                "bss_contribution_size": contribution.bss_size if contribution is not None else 0,
                "sections": normalized_object_sections(contribution),
            },
            "threshold_relationships": {
                "candidate_data_end_offset": data_end,
                "candidate_data_end_address": hex32(candidate_base + data_end),
                "bss_start_offset": bss_boundary,
                "bss_start_address": hex32(candidate_base + bss_boundary) if bss_boundary is not None else "",
                "candidate_raw_end_offset": candidate_section.raw_size,
                "candidate_raw_end_address": hex32(candidate_base + candidate_section.raw_size),
                "reference_raw_end_offset": reference_section.raw_size,
                "reference_raw_end_address": hex32(reference_base + reference_section.raw_size),
                "candidate_virtual_end_offset": candidate_section.virtual_size,
                "candidate_virtual_end_address": hex32(candidate_base + candidate_section.virtual_size),
                "object_min_map_offset_relationships": offset_boundary_relationships(
                    min_offset,
                    data_end=data_end,
                    bss_boundary=bss_boundary,
                    candidate_section=candidate_section,
                    reference_section=reference_section,
                ) if min_offset is not None else {},
                "object_max_map_offset_relationships": offset_boundary_relationships(
                    max_offset,
                    data_end=data_end,
                    bss_boundary=bss_boundary,
                    candidate_section=candidate_section,
                    reference_section=reference_section,
                ) if max_offset is not None else {},
                "threshold_bytes_past_candidate_data_end": FINAL_DATA_SUBSECTION_THRESHOLDS,
                "can_satisfy_thresholds_by_size": object_threshold_size_records(contribution),
            },
            "selected_symbols": tuple(
                object_trace_symbol_record(
                    symbol,
                    coff=coff,
                    coff_status=coff_status,
                    map_sections=map_sections,
                    contributions=contributions,
                    data_end=data_end,
                    bss_boundary=bss_boundary,
                    candidate_section=candidate_section,
                    reference_section=reference_section,
                )
                for symbol in object_symbols[:limit]
            ),
            "selected_symbol_count": min(len(object_symbols), limit),
            "selected_symbols_truncated_count": max(len(object_symbols) - limit, 0),
        }
        object_records.append(object_record)

    return {
        "available": True,
        "objects_requested": requested_objects,
        "summary": {
            "segment": f"{data_section.segment:04x}",
            "candidate_data_end_offset": data_end,
            "candidate_data_end_address": hex32(candidate_base + data_end),
            "bss_start_offset": bss_boundary,
            "bss_start_address": hex32(candidate_base + bss_boundary) if bss_boundary is not None else "",
            "candidate_raw_end_offset": candidate_section.raw_size,
            "candidate_raw_end_address": hex32(candidate_base + candidate_section.raw_size),
            "reference_raw_end_offset": reference_section.raw_size,
            "reference_raw_end_address": hex32(reference_base + reference_section.raw_size),
            "candidate_virtual_end_offset": candidate_section.virtual_size,
            "candidate_virtual_end_address": hex32(candidate_base + candidate_section.virtual_size),
            "object_count": len(object_records),
        },
        "objects": tuple(object_records),
        "limitations": (
            "Trace-only: map/COFF/link-response placement does not prove source ownership, data-owner acceptance, provider classification, or marker eligibility.",
            "COFF object paths come from the existing link response; missing paths are reported without building or probing.",
        ),
    }


def candidate_object_subsection_attribution(
    *,
    thresholds: dict[str, Any],
    map_sections: tuple[MapSection, ...],
    segment_symbols: tuple[MapSymbol, ...],
    reference_section: SectionFacts,
    candidate_section: SectionFacts,
    data_section: MapSection | None,
    bss_boundary: int | None,
    object_rows: tuple[ObjectContribution, ...],
    candidate_file_alignment: int,
    limit: int,
    manifest_coverage: ManifestCoverage | None = None,
    raw_tail_attribution: dict[str, Any] | None = None,
    virtual_tail_attribution: dict[str, Any] | None = None,
) -> dict[str, Any]:
    if data_section is None:
        return {
            "available": False,
            "reason": "candidate map .data section not found",
            "objects": (),
            "limitations": (
                "Attribution unavailable without candidate map .data section evidence.",
                "Attribution-only: this field does not prove source ownership, data-owner acceptance, provider classification, or marker eligibility.",
            ),
        }

    summary = thresholds.get("summary", {})
    data_end = int(summary.get("candidate_initialized_data_end_offset", data_section.end))
    candidate_base = 0x400000 + candidate_section.rva
    reference_base = 0x400000 + reference_section.rva
    contributions = {row.object: row for row in object_rows}
    symbols_by_object: dict[str, list[MapSymbol]] = {}
    for symbol in segment_symbols:
        symbols_by_object.setdefault(symbol.object, []).append(symbol)
    by_symbol: dict[str, list[MapSymbol]] = {}
    for symbol in segment_symbols:
        by_symbol.setdefault(symbol.symbol, []).append(symbol)

    selection_reasons: dict[str, list[str]] = {}
    selected_symbols: dict[str, dict[str, MapSymbol]] = {}
    selected_reasons: dict[tuple[str, str], list[str]] = {}
    manifest_issue_correlations: dict[tuple[str, str], list[dict[str, Any]]] = {}
    raw_tail_symbols = [
        symbol
        for symbol in segment_symbols
        if data_end <= symbol.offset < reference_section.raw_size
    ]
    for symbol in raw_tail_symbols:
        add_object_selection_reason(selection_reasons, symbol.object, "raw_tail_symbol")
        add_selected_map_symbol(selected_symbols, selected_reasons, symbol, "raw_tail_symbol")

    if manifest_coverage is not None:
        for issue in manifest_coverage.issues:
            for symbol in select_symbols_by_manifest_name(
                symbol_name=issue.symbol,
                by_symbol=by_symbol,
                segment_symbols=segment_symbols,
            ):
                reason = f"manifest_issue:{issue.kind}"
                add_object_selection_reason(selection_reasons, symbol.object, reason)
                add_selected_map_symbol(selected_symbols, selected_reasons, symbol, reason)
                manifest_issue_correlations.setdefault((symbol.object, symbol.symbol), []).append(
                    selected_manifest_issue_correlation_record(
                        issue,
                        symbol,
                        reference_base=reference_base,
                        reference_section=reference_section,
                        candidate_section=candidate_section,
                        data_end=data_end,
                        bss_boundary=bss_boundary,
                    )
                )

    collect_tail_manifest_match_symbols(
        raw_tail_attribution or {},
        by_symbol=by_symbol,
        segment_symbols=segment_symbols,
        reason="raw_tail_manifest_match",
        selected_symbols=selected_symbols,
        selected_reasons=selected_reasons,
        selection_reasons=selection_reasons,
    )
    collect_tail_manifest_match_symbols(
        virtual_tail_attribution or {},
        by_symbol=by_symbol,
        segment_symbols=segment_symbols,
        reason="virtual_tail_manifest_match",
        selected_symbols=selected_symbols,
        selected_reasons=selected_reasons,
        selection_reasons=selection_reasons,
    )

    point_offsets: list[tuple[str, int]] = [("candidate_initialized_data_end", data_end)]
    if bss_boundary is not None:
        point_offsets.append(("bss_start", bss_boundary))
    point_offsets.append(("candidate_raw_end", candidate_section.raw_size))
    point_offsets.extend(
        (
            f"threshold_{hex32(int(row.get('target_raw_end_offset', 0)))}",
            int(row.get("target_raw_end_offset", 0)),
        )
        for row in tuple(thresholds.get("thresholds", ()))
    )
    point_offsets.append(("reference_raw_end", reference_section.raw_size))

    before_count = max(limit // 2, 1)
    after_count = max(limit - before_count, 1)
    for name, offset in point_offsets:
        before_symbols = [symbol for symbol in segment_symbols if symbol.offset < offset][-before_count:]
        after_symbols = [symbol for symbol in segment_symbols if symbol.offset >= offset][:after_count]
        for symbol in before_symbols:
            add_object_selection_reason(selection_reasons, symbol.object, f"{name}:before")
            add_selected_map_symbol(selected_symbols, selected_reasons, symbol, f"{name}:before")
        for symbol in after_symbols:
            add_object_selection_reason(selection_reasons, symbol.object, f"{name}:after")
            add_selected_map_symbol(selected_symbols, selected_reasons, symbol, f"{name}:after")

    focus_present: list[str] = []
    for object_name in FINAL_DATA_SUBSECTION_FOCUS_OBJECTS:
        if object_name in contributions or object_name in symbols_by_object:
            focus_present.append(object_name)
            add_object_selection_reason(selection_reasons, object_name, "focus_object")

    ordered_objects: list[str] = []
    for object_name in FINAL_DATA_SUBSECTION_FOCUS_OBJECTS:
        if object_name in selection_reasons and object_name not in ordered_objects:
            ordered_objects.append(object_name)
    for symbol in segment_symbols:
        if symbol.object in selection_reasons and symbol.object not in ordered_objects:
            ordered_objects.append(symbol.object)
    for row in object_rows:
        if row.object in selection_reasons and row.object not in ordered_objects:
            ordered_objects.append(row.object)

    object_records: list[dict[str, Any]] = []
    for object_name in ordered_objects:
        contribution = contributions.get(object_name)
        object_symbols = tuple(symbols_by_object.get(object_name, ()))
        symbol_kinds = tuple(
            dict.fromkeys(
                map_evidence_kind_for_symbol(symbol, map_sections=map_sections, contributions=contributions)
                for symbol in object_symbols
            )
        )
        map_symbols = tuple(
            attributed_symbol_record(symbol, map_sections=map_sections, contributions=contributions)
            for symbol in object_symbols[:limit]
        )
        selected_coff_symbols = selected_coff_symbol_records(
            object_name=object_name,
            contribution=contribution,
            selected_symbols=selected_symbols,
            selected_reasons=selected_reasons,
            manifest_issue_correlations={
                key: tuple(records) for key, records in manifest_issue_correlations.items()
            },
        )
        object_records.append(
            {
                "object": object_name,
                "object_path": contribution.object_path if contribution is not None else "",
                "object_origin": object_origin_record(object_name),
                "provenance_kind": object_provenance_kind(object_name),
                "selection_reasons": tuple(selection_reasons.get(object_name, ())),
                "map_evidence_kind": object_primary_map_evidence_kind(symbol_kinds, contribution),
                "map_evidence_kinds": symbol_kinds,
                "map_symbol_count": len(object_symbols),
                "map_symbols": map_symbols,
                "selected_coff_symbols": selected_coff_symbols,
                "selected_coff_symbol_count": len(selected_coff_symbols),
                "initialized_data_contribution_size": contribution.data_size if contribution is not None else 0,
                "bss_contribution_size": contribution.bss_size if contribution is not None else 0,
                "can_satisfy_thresholds_by_size": object_threshold_size_records(contribution),
                "sections": normalized_object_sections(contribution),
            }
        )

    attribution_summary: dict[str, Any] = {
        "segment": f"{data_section.segment:04x}",
        "candidate_initialized_data_end_offset": data_end,
        "candidate_initialized_data_end_address": hex32(candidate_base + data_end),
        "candidate_raw_end_offset": candidate_section.raw_size,
        "candidate_raw_end_address": hex32(candidate_base + candidate_section.raw_size),
        "reference_raw_end_offset": reference_section.raw_size,
        "reference_raw_end_address": hex32(reference_base + reference_section.raw_size),
        "candidate_file_alignment": candidate_file_alignment,
        "threshold_bytes_past_candidate_initialized_data_end": FINAL_DATA_SUBSECTION_THRESHOLDS,
        "focus_objects_requested": FINAL_DATA_SUBSECTION_FOCUS_OBJECTS,
        "focus_objects_present": tuple(focus_present),
        "selected_object_count": len(object_records),
    }
    if bss_boundary is not None:
        attribution_summary["bss_start_offset"] = bss_boundary
        attribution_summary["bss_start_address"] = hex32(candidate_base + bss_boundary)
        attribution_summary["data_end_to_bss_start_slack"] = bss_boundary - data_end
    return {
        "available": True,
        "summary": attribution_summary,
        "objects": tuple(object_records),
        "limitations": (
            "Attribution-only: this field does not prove source ownership, data-owner acceptance, provider classification, or marker eligibility.",
            "Size-satisfaction booleans are arithmetic filters only; they do not prove that an object can source-faithfully account for the retail raw-end gap.",
            "Provider/library provenance is inferred from existing map object labels and still requires provider-boundary review before marker use.",
            "Missing or unreadable COFF objects report empty section rows; the audit is read-only and does not build, relink, or generate probes.",
        ),
    }


def parse_link_rsp_objects(path: Path) -> tuple[Path, ...]:
    objects: list[Path] = []
    for raw_line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        line = raw_line.strip()
        if not line:
            continue
        if line.startswith('"') and line.endswith('"'):
            line = line[1:-1]
        if line.lower().endswith(".obj"):
            objects.append(Path(line))
    return tuple(objects)


def object_contributions(object_paths: tuple[Path, ...]) -> tuple[ObjectContribution, ...]:
    rows: list[ObjectContribution] = []
    for path in object_paths:
        display_object_path = display_path(path, REPO_ROOT)
        if not path.is_file():
            rows.append(ObjectContribution(object=str(path), data_size=0, bss_size=0, sections=({"name": "<missing>", "size": 0, "kind": "unknown"},), object_path=display_object_path))
            continue
        data_size = 0
        bss_size = 0
        section_rows: list[dict[str, int | str]] = []
        try:
            coff = CoffObject.from_path(path)
        except ValueError as exc:
            rows.append(ObjectContribution(object=str(path), data_size=0, bss_size=0, sections=({"name": "<unreadable>", "size": 0, "kind": "unknown", "error": str(exc)},), object_path=display_object_path))
            continue
        for section_index, section in enumerate(coff.sections):
            name = section.name.lower()
            size = len(section.raw_data)
            kind = coff_subsection_kind(section.name, section.characteristics)
            if name == ".data" or name.startswith(".data$"):
                data_size += size
                section_rows.append({"name": section.name, "size": size, "order": section_index, "kind": kind, "section_class": "DATA", "characteristics": section.characteristics})
            elif name == ".bss" or name.startswith(".bss$") or section.characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA:
                bss_size += size
                section_rows.append({"name": section.name, "size": size, "order": section_index, "kind": kind, "section_class": "DATA", "characteristics": section.characteristics})
            elif name == ".rdata" or name.startswith(".rdata$"):
                section_rows.append({"name": section.name, "size": size, "order": section_index, "kind": kind, "section_class": "DATA", "characteristics": section.characteristics})
        if data_size or bss_size:
            rows.append(ObjectContribution(object=path.name, data_size=data_size, bss_size=bss_size, sections=tuple(section_rows), object_path=display_object_path))
    return tuple(rows)


def top_contributions(rows: tuple[ObjectContribution, ...], field: str, limit: int) -> tuple[dict[str, Any], ...]:
    selected = sorted(rows, key=lambda item: getattr(item, field), reverse=True)
    return tuple(asdict(item) for item in selected if getattr(item, field) > 0)[:limit]


def map_symbol_index(symbols: tuple[MapSymbol, ...]) -> tuple[dict[str, list[MapSymbol]], dict[int, list[MapSymbol]]]:
    by_name: dict[str, list[MapSymbol]] = {}
    by_address: dict[int, list[MapSymbol]] = {}
    for symbol in symbols:
        by_name.setdefault(symbol.symbol, []).append(symbol)
        by_address.setdefault(symbol.address, []).append(symbol)
    return by_name, by_address


def candidate_symbol_matches(data_symbol: Any, symbols: tuple[MapSymbol, ...], by_symbol: dict[str, list[MapSymbol]]) -> list[MapSymbol]:
    if data_symbol.symbol:
        return by_symbol.get(data_symbol.symbol, [])
    if data_symbol.symbol_regex is None:
        return []
    pattern = re.compile(data_symbol.symbol_regex)
    return [symbol for symbol in symbols if pattern.fullmatch(symbol.symbol)]


def manifest_coverage(manifest_dir: Path, symbols: tuple[MapSymbol, ...], reference_section: SectionFacts) -> ManifestCoverage:
    manifests = load_manifests(manifest_dir, enforce_source_policy=False)
    by_symbol, by_address = map_symbol_index(symbols)
    section_start = 0x400000 + reference_section.rva
    section_end = section_start + reference_section.virtual_size
    data_symbol_count = 0
    in_reference_section = 0
    symbol_name_matches = 0
    exact_address_matches = 0
    issues: list[ManifestIssue] = []
    for manifest in manifests:
        for data_symbol in manifest.data_symbols:
            data_symbol_count += 1
            address_value = int(data_symbol.address, 16)
            in_section = section_start <= address_value < section_end
            if in_section:
                in_reference_section += 1
            name_matches = candidate_symbol_matches(data_symbol, symbols, by_symbol)
            exact_matches = by_address.get(address_value, [])
            if name_matches:
                symbol_name_matches += 1
            if exact_matches:
                exact_address_matches += 1
            if not in_section:
                continue
            if not name_matches and not exact_matches:
                issues.append(
                    ManifestIssue(
                        manifest=display_path(manifest.manifest_path, REPO_ROOT),
                        target=manifest.name,
                        name=data_symbol.name,
                        symbol=data_symbol.symbol or f"symbol_regex={data_symbol.symbol_regex}",
                        address=data_symbol.address,
                        byte_length=data_symbol.byte_length,
                        kind="missing-candidate-map-symbol",
                        detail="no candidate map symbol matched by decorated name, symbol_regex, or original address",
                    )
                )
            elif name_matches and not any(match.address == address_value for match in name_matches):
                first = name_matches[0]
                candidate_address = hex32(first.address)
                issues.append(
                    ManifestIssue(
                        manifest=display_path(manifest.manifest_path, REPO_ROOT),
                        target=manifest.name,
                        name=data_symbol.name,
                        symbol=data_symbol.symbol or first.symbol,
                        address=data_symbol.address,
                        byte_length=data_symbol.byte_length,
                        kind="candidate-address-drift",
                        detail=f"candidate map places symbol at {candidate_address}",
                        candidate_address=candidate_address,
                    )
                )
    return ManifestCoverage(
        manifest_count=len(manifests),
        data_symbol_count=data_symbol_count,
        in_reference_section=in_reference_section,
        symbol_name_matches=symbol_name_matches,
        exact_address_matches=exact_address_matches,
        issues=tuple(issues),
    )


def manifest_data_symbol_ranges(manifests: list[Any]) -> tuple[ManifestDataSymbolRange, ...]:
    ranges: list[ManifestDataSymbolRange] = []
    for manifest in manifests:
        for data_symbol in manifest.data_symbols:
            start = int(data_symbol.address, 16)
            end = start + data_symbol.byte_length
            if data_symbol.symbol:
                symbol = data_symbol.symbol
            elif data_symbol.symbol_regex is not None:
                symbol = f"symbol_regex={data_symbol.symbol_regex}"
            else:
                symbol = ""
            ranges.append(
                ManifestDataSymbolRange(
                    manifest=display_path(manifest.manifest_path, REPO_ROOT),
                    target=manifest.name,
                    name=data_symbol.name,
                    symbol=symbol,
                    address=hex32(start),
                    start=start,
                    end=end,
                    byte_length=data_symbol.byte_length,
                    source_from=manifest.source_from,
                    source_filename=manifest.source_filename,
                )
            )
    return tuple(sorted(ranges, key=lambda item: (item.start, item.end, item.target, item.name)))


def manifest_tail_match_record(
    data_symbol: ManifestDataSymbolRange,
    *,
    overlap_start: int,
    overlap_end: int,
) -> dict[str, Any]:
    overlap_size = overlap_end - overlap_start
    return {
        "target": data_symbol.target,
        "manifest": data_symbol.manifest,
        "name": data_symbol.name,
        "symbol": data_symbol.symbol,
        "address": data_symbol.address,
        "byte_length": data_symbol.byte_length,
        "source_from": data_symbol.source_from,
        "source_filename": data_symbol.source_filename,
        "overlap_start": hex32(overlap_start),
        "overlap_end": hex32(overlap_end),
        "overlap_range": f"{hex32(overlap_start)}..{hex32(overlap_end)}",
        "overlap_size": overlap_size,
    }


def tail_item_bounds(item: dict[str, Any]) -> tuple[int, int]:
    overlap_start = item.get("tail_overlap_start")
    overlap_end = item.get("tail_overlap_end")
    if overlap_start is not None and overlap_end is not None:
        return int(str(overlap_start), 16), int(str(overlap_end), 16)
    start = int(str(item.get("address", "0")), 16)
    end_value = item.get("end_address")
    if end_value is not None:
        return start, int(str(end_value), 16)
    return start, start + int(item.get("size", 0))


def merged_intervals(spans: list[tuple[int, int]]) -> tuple[tuple[int, int], ...]:
    ordered = sorted((start, end) for start, end in spans if start < end)
    if not ordered:
        return ()
    merged: list[tuple[int, int]] = [ordered[0]]
    for start, end in ordered[1:]:
        last_start, last_end = merged[-1]
        if start <= last_end:
            merged[-1] = (last_start, max(last_end, end))
        else:
            merged.append((start, end))
    return tuple(merged)


def uncovered_intervals(
    start: int,
    end: int,
    covered: tuple[tuple[int, int], ...],
) -> tuple[tuple[int, int], ...]:
    if start >= end:
        return ()
    gaps: list[tuple[int, int]] = []
    cursor = start
    for covered_start, covered_end in covered:
        if covered_end <= cursor:
            continue
        if covered_start > cursor:
            gaps.append((cursor, min(covered_start, end)))
        cursor = max(cursor, covered_end)
        if cursor >= end:
            break
    if cursor < end:
        gaps.append((cursor, end))
    return tuple(gap for gap in gaps if gap[0] < gap[1])


def range_record(start: int, end: int) -> dict[str, Any]:
    return {
        "start": hex32(start),
        "end": hex32(end),
        "range": f"{hex32(start)}..{hex32(end)}",
        "size": end - start,
    }


def annotate_tail_manifest_matches(
    items: tuple[dict[str, Any], ...],
    data_symbols: tuple[ManifestDataSymbolRange, ...],
    *,
    limit: int,
) -> tuple[dict[str, Any], ...]:
    annotated: list[dict[str, Any]] = []
    for item in items:
        start, end = tail_item_bounds(item)
        matches = []
        match_spans: list[tuple[int, int]] = []
        for data_symbol in data_symbols:
            overlap_start = max(start, data_symbol.start)
            overlap_end = min(end, data_symbol.end)
            if overlap_start < overlap_end:
                match_spans.append((overlap_start, overlap_end))
                matches.append(
                    manifest_tail_match_record(
                        data_symbol,
                        overlap_start=overlap_start,
                        overlap_end=overlap_end,
                    )
                )
        covered = merged_intervals(match_spans)
        uncovered = uncovered_intervals(start, end, covered)
        covered_bytes = sum(interval_end - interval_start for interval_start, interval_end in covered)
        uncovered_bytes = sum(interval_end - interval_start for interval_start, interval_end in uncovered)
        enriched = dict(item)
        enriched["manifest_match_count"] = len(matches)
        enriched["manifest_unmatched"] = not matches
        enriched["manifest_covered_bytes"] = covered_bytes
        enriched["manifest_uncovered_bytes"] = uncovered_bytes
        enriched["manifest_fully_covered"] = uncovered_bytes == 0
        enriched["manifest_uncovered_range_count"] = len(uncovered)
        enriched["manifest_uncovered_ranges"] = tuple(range_record(start, end) for start, end in uncovered[:limit])
        if len(uncovered) > limit:
            enriched["manifest_uncovered_ranges_truncated"] = len(uncovered) - limit
        enriched["manifest_matches"] = tuple(matches[:limit])
        if len(matches) > limit:
            enriched["manifest_matches_truncated"] = len(matches) - limit
        annotated.append(enriched)
    return tuple(annotated)


def _append_capped_unique(values: list[str], value: str, *, limit: int) -> None:
    if value and value not in values and len(values) < limit:
        values.append(value)


def reference_tail_source_summary(
    items: tuple[dict[str, Any], ...],
    *,
    limit: int,
) -> dict[str, Any]:
    groups: dict[tuple[str, str, str, str], dict[str, Any]] = {}
    unmatched_items: list[dict[str, Any]] = []
    uncovered_spans: list[dict[str, Any]] = []
    manifest_item_ranges: set[str] = set()
    manifest_item_bytes = 0
    unmatched_bytes = 0
    uncovered_bytes = 0
    uncovered_span_count = 0
    fully_covered_item_count = 0
    partially_covered_item_count = 0

    for item in items:
        item_start, item_end = tail_item_bounds(item)
        item_size = item_end - item_start
        item_range = f"{hex32(item_start)}..{hex32(item_end)}"
        display_range = str(item.get("range") or item.get("address") or "")
        matches = tuple(item.get("manifest_matches", ()))
        item_uncovered_bytes = int(item.get("manifest_uncovered_bytes", item_size if not matches else 0))
        item_uncovered_ranges = tuple(item.get("manifest_uncovered_ranges", ()))
        item_uncovered_count = int(item.get("manifest_uncovered_range_count", len(item_uncovered_ranges)))
        uncovered_bytes += item_uncovered_bytes
        uncovered_span_count += item_uncovered_count
        if matches and item_uncovered_bytes:
            partially_covered_item_count += 1
        elif matches:
            fully_covered_item_count += 1
        if item_uncovered_bytes and len(uncovered_spans) < limit:
            for span in item_uncovered_ranges:
                if len(uncovered_spans) >= limit:
                    break
                uncovered_spans.append(
                    {
                        "range": span.get("range", ""),
                        "start": span.get("start", ""),
                        "end": span.get("end", ""),
                        "size": int(span.get("size", 0)),
                        "item_range": display_range,
                        "tail_range": item_range,
                        "address": item.get("address", ""),
                        "end_address": item.get("end_address", ""),
                        "name": item.get("name", ""),
                        "type": item.get("type", ""),
                        "section": item.get("section", ""),
                        "backing": item.get("backing", ""),
                        "classification": item.get("classification", ""),
                        "manifest_match_count": int(item.get("manifest_match_count", 0)),
                    }
                )
        if not matches:
            unmatched_bytes += item_size
            if len(unmatched_items) < limit:
                unmatched_items.append(
                    {
                        "range": item_range,
                        "address": item.get("address", ""),
                        "end_address": item.get("end_address", ""),
                        "size": item_size,
                        "name": item.get("name", ""),
                        "type": item.get("type", ""),
                        "section": item.get("section", ""),
                        "backing": item.get("backing", ""),
                        "classification": item.get("classification", ""),
                    }
                )
            continue

        if item_range not in manifest_item_ranges:
            manifest_item_ranges.add(item_range)
            manifest_item_bytes += item_size

        item_groups_seen: set[tuple[str, str, str, str]] = set()
        for match in matches:
            key = (
                str(match.get("source_from") or ""),
                str(match.get("source_filename") or ""),
                str(match.get("target") or ""),
                str(match.get("manifest") or ""),
            )
            group = groups.get(key)
            if group is None:
                group = {
                    "source_from": key[0],
                    "source_filename": key[1],
                    "target": key[2],
                    "manifest": key[3],
                    "item_count": 0,
                    "item_bytes": 0,
                    "matched_overlap_bytes": 0,
                    "ranges": [],
                    "names": [],
                    "item_names": [],
                }
                groups[key] = group

            if key not in item_groups_seen:
                group["item_count"] += 1
                group["item_bytes"] += item_size
                _append_capped_unique(group["ranges"], item_range, limit=limit)
                _append_capped_unique(group["item_names"], str(item.get("name") or ""), limit=limit)
                item_groups_seen.add(key)

            group["matched_overlap_bytes"] += int(match.get("overlap_size", 0))
            _append_capped_unique(group["names"], str(match.get("name") or ""), limit=limit)

    sorted_groups = sorted(
        groups.values(),
        key=lambda group: (
            str(group.get("source_from") or ""),
            str(group.get("source_filename") or ""),
            str(group.get("target") or ""),
            str(group.get("manifest") or ""),
        ),
    )
    for group in sorted_groups:
        group["ranges_truncated"] = max(int(group["item_count"]) - len(group["ranges"]), 0)
        group["names_truncated"] = max(int(group["item_count"]) - len(group["names"]), 0)
        group["item_names_truncated"] = max(int(group["item_count"]) - len(group["item_names"]), 0)
        group["ranges"] = tuple(group["ranges"])
        group["names"] = tuple(group["names"])
        group["item_names"] = tuple(group["item_names"])

    return {
        "available": True,
        "source_group_count": len(sorted_groups),
        "manifest_backed_item_count": len(manifest_item_ranges),
        "manifest_backed_item_bytes": manifest_item_bytes,
        "fully_manifest_covered_item_count": fully_covered_item_count,
        "partially_manifest_covered_item_count": partially_covered_item_count,
        "matched_overlap_bytes": sum(int(group["matched_overlap_bytes"]) for group in sorted_groups),
        "source_groups": tuple(sorted_groups),
        "unmatched_item_count": sum(1 for item in items if not tuple(item.get("manifest_matches", ()))),
        "unmatched_bytes": unmatched_bytes,
        "unmatched_items": tuple(unmatched_items),
        "unmatched_items_truncated": max(
            sum(1 for item in items if not tuple(item.get("manifest_matches", ()))) - len(unmatched_items),
            0,
        ),
        "manifest_uncovered_span_count": uncovered_span_count,
        "manifest_uncovered_bytes": uncovered_bytes,
        "manifest_uncovered_spans": tuple(uncovered_spans),
        "manifest_uncovered_spans_truncated": max(uncovered_span_count - len(uncovered_spans), 0),
    }


def bn_coverage(
    *,
    bridge_url: str,
    section_name: str,
    reference_section: SectionFacts,
    manifest_covered_addresses: set[int],
    raw_tail_start: int | None = None,
    raw_tail_end: int | None = None,
    virtual_tail_start: int | None = None,
    virtual_tail_end: int | None = None,
    limit: int,
) -> BnCoverage:
    try:
        bridge = BinaryNinjaBridge(bridge_url, timeout=8.0, binary="Recoil.bndb")
        items = fetch_data_items(bridge, limit=100000, max_pages=4)
    except (BridgeError, RuntimeError, ValueError) as exc:
        return BnCoverage(available=False, error=str(exc))
    section_start = 0x400000 + reference_section.rva
    section_end = section_start + reference_section.virtual_size
    section_items = [
        item
        for item in items
        if item.section == section_name or section_start <= item.start < section_end
    ]
    uncovered = [
        item
        for item in section_items
        if not any(item.start <= address < item.end for address in manifest_covered_addresses)
    ]
    raw_tail_items: list[DataItem] = []
    if raw_tail_start is not None and raw_tail_end is not None:
        raw_tail_items = [
            item
            for item in section_items
            if item.start < raw_tail_end and item.end > raw_tail_start
        ]
    virtual_tail_items: list[DataItem] = []
    if virtual_tail_start is not None and virtual_tail_end is not None:
        virtual_tail_items = [
            item
            for item in section_items
            if item.start < virtual_tail_end and item.end > virtual_tail_start
        ]
    raw_tail_items.sort(key=lambda item: (item.start, item.end, item.name))
    virtual_tail_items.sort(key=lambda item: (item.start, item.end, item.name))
    return BnCoverage(
        available=True,
        item_count=len(items),
        section_item_count=len(section_items),
        manifest_covered_count=len(section_items) - len(uncovered),
        uncovered_items=tuple(data_item_record(item) for item in uncovered[:limit]),
        raw_tail_items=tuple(
            data_item_tail_record(item, reference_section, tail_start=raw_tail_start, tail_end=raw_tail_end)
            for item in raw_tail_items
        ),
        virtual_tail_items=tuple(
            data_item_tail_record(item, reference_section, tail_start=virtual_tail_start, tail_end=virtual_tail_end)
            for item in virtual_tail_items
        ),
    )


def data_item_record(item: DataItem) -> dict[str, int | str]:
    return {
        "address": item.address,
        "size": item.size,
        "name": item.name,
        "type": item.data_type,
        "section": item.section,
    }


def provider_name_classification(name: str, data_type: str = "") -> str:
    text = f"{name} {data_type}".lower()
    if "rtti" in text or "type_info" in text or "typeinfo" in text:
        return "provider-candidate:rtti/type_info"
    if "??_r" in name.lower() or "??_7type_info" in name.lower():
        return "provider-candidate:rtti/type_info"
    return ""


def data_item_tail_record(
    item: DataItem,
    reference_section: SectionFacts,
    *,
    tail_start: int | None = None,
    tail_end: int | None = None,
) -> dict[str, Any]:
    section_base = 0x400000 + reference_section.rva
    offset_start = max(item.start - section_base, 0)
    offset_end = max(item.end - section_base, offset_start)
    raw_end = reference_section.raw_size
    if offset_end <= raw_end:
        backing = "raw-backed"
    elif offset_start >= raw_end:
        backing = "zero-fill"
    else:
        backing = "mixed-raw-and-zero-fill"
    record: dict[str, Any] = {
        "address": item.address,
        "end_address": hex32(item.end),
        "size": item.size,
        "range": f"{item.address}..{hex32(item.end)}",
        "name": item.name,
        "type": item.data_type,
        "section": item.section,
        "section_offset_start": offset_start,
        "section_offset_end": offset_end,
        "backing": backing,
        "classification": provider_name_classification(item.name, item.data_type),
    }
    if tail_start is not None and tail_end is not None:
        overlap_start = max(item.start, tail_start)
        overlap_end = min(item.end, tail_end)
        if overlap_start < overlap_end:
            record.update(
                {
                    "tail_overlap_start": hex32(overlap_start),
                    "tail_overlap_end": hex32(overlap_end),
                    "tail_overlap_range": f"{hex32(overlap_start)}..{hex32(overlap_end)}",
                    "tail_overlap_size": overlap_end - overlap_start,
                }
            )
    return record


def candidate_window_backing(candidate_section: SectionFacts, start_offset: int, end_offset: int, map_data: MapSection | None, bss_boundary: int | None) -> str:
    if start_offset >= candidate_section.virtual_size:
        return "outside-candidate-section"
    if start_offset >= candidate_section.raw_size:
        if bss_boundary is not None and start_offset >= bss_boundary:
            return "zero-fill-bss"
        if map_data is not None and start_offset >= map_data.end:
            return "zero-fill-or-padding-before-bss"
        return "zero-fill-tail"
    if end_offset <= candidate_section.raw_size:
        return "raw-backed"
    return "mixed-raw-and-zero-fill"


def tail_window_record(*, start_address: int, start_offset: int, size: int, backing: str) -> dict[str, Any]:
    return {
        "start": hex32(start_address),
        "end": hex32(start_address + size),
        "size": size,
        "section_offset_start": start_offset,
        "section_offset_end": start_offset + size,
        "backing": backing,
    }


def _printable_ascii(byte: int) -> bool:
    return 0x20 <= byte <= 0x7E


def _printable_text_snippet(data: bytes) -> str:
    text = data[:TAIL_PRINTABLE_SNIPPET_LIMIT].decode("ascii", errors="replace")
    if len(data) > TAIL_PRINTABLE_SNIPPET_LIMIT:
        return text + "..."
    return text


def printable_byte_runs(
    data: bytes,
    *,
    section_offset_start: int,
    address_start: int,
    file_offset_start: int,
    limit: int,
) -> tuple[tuple[dict[str, Any], ...], int]:
    runs: list[dict[str, Any]] = []
    run_start: int | None = None
    run_count = 0

    def finish_run(end_index: int) -> None:
        nonlocal run_start, run_count
        if run_start is None:
            return
        length = end_index - run_start
        if length >= TAIL_PRINTABLE_RUN_MIN_LENGTH:
            run_count += 1
            if len(runs) < limit:
                section_start = section_offset_start + run_start
                section_end = section_offset_start + end_index
                address = address_start + run_start
                runs.append(
                    {
                        "section_offset_start": section_start,
                        "section_offset_end": section_end,
                        "file_offset_start": file_offset_start + run_start,
                        "file_offset_end": file_offset_start + end_index,
                        "start_address": hex32(address),
                        "end_address": hex32(address + length),
                        "size": length,
                        "text": _printable_text_snippet(data[run_start:end_index]),
                        "text_truncated": length > TAIL_PRINTABLE_SNIPPET_LIMIT,
                    }
                )
        run_start = None

    for index, byte in enumerate(data):
        if _printable_ascii(byte):
            if run_start is None:
                run_start = index
        else:
            finish_run(index)
    finish_run(len(data))
    return tuple(runs), max(run_count - len(runs), 0)


def tail_byte_summary(
    image_path: Path,
    section: SectionFacts,
    window: dict[str, Any],
    *,
    limit: int,
) -> dict[str, Any]:
    backing = str(window.get("backing") or "")
    start_offset = int(window.get("section_offset_start", 0))
    end_offset = int(window.get("section_offset_end", start_offset))
    start_address_text = str(window.get("start") or "0x0")
    try:
        start_address = int(start_address_text, 16)
    except ValueError:
        start_address = 0x400000 + section.rva + start_offset
    limitations = [TAIL_BYTE_SUMMARY_LIMITATION]

    file_backed_start = max(start_offset, 0)
    file_backed_end = min(end_offset, section.raw_size)
    if end_offset <= 0 or start_offset >= section.raw_size or file_backed_start >= file_backed_end:
        if backing:
            limitations.append(f"window backing is {backing}; no file-backed bytes were read")
        else:
            limitations.append("window has no file-backed bytes to read")
        return {
            "available": False,
            "window": window,
            "backing": backing,
            "file_backed_byte_count": 0,
            "nonzero_byte_count": 0,
            "zero_byte_count": 0,
            "first_nonzero_offset": None,
            "last_nonzero_offset": None,
            "first_nonzero_address": "",
            "last_nonzero_address": "",
            "printable_runs": (),
            "printable_runs_truncated": 0,
            "limitations": tuple(limitations),
        }

    if file_backed_start != start_offset or file_backed_end != end_offset:
        limitations.append(
            "only the file-backed portion of this mixed or clipped window was read"
        )

    image = image_path.read_bytes()
    file_start = section.raw_pointer + file_backed_start
    file_end = section.raw_pointer + file_backed_end
    data = image[file_start:min(file_end, len(image))]
    if len(data) < file_backed_end - file_backed_start:
        limitations.append("image ended before the full file-backed window could be read")
        file_backed_end = file_backed_start + len(data)

    nonzero_offsets = [index for index, byte in enumerate(data) if byte]
    first_nonzero_offset = None
    last_nonzero_offset = None
    first_nonzero_address = ""
    last_nonzero_address = ""
    if nonzero_offsets:
        first_nonzero_offset = file_backed_start + nonzero_offsets[0]
        last_nonzero_offset = file_backed_start + nonzero_offsets[-1]
        first_nonzero_address = hex32(start_address + (first_nonzero_offset - start_offset))
        last_nonzero_address = hex32(start_address + (last_nonzero_offset - start_offset))

    printable_runs, printable_runs_truncated = printable_byte_runs(
        data,
        section_offset_start=file_backed_start,
        address_start=start_address + (file_backed_start - start_offset),
        file_offset_start=file_start,
        limit=limit,
    )
    return {
        "available": True,
        "window": window,
        "backing": backing,
        "file_backed_byte_count": len(data),
        "nonzero_byte_count": len(nonzero_offsets),
        "zero_byte_count": len(data) - len(nonzero_offsets),
        "first_nonzero_offset": first_nonzero_offset,
        "last_nonzero_offset": last_nonzero_offset,
        "first_nonzero_address": first_nonzero_address,
        "last_nonzero_address": last_nonzero_address,
        "printable_runs": printable_runs,
        "printable_runs_truncated": printable_runs_truncated,
        "limitations": tuple(limitations),
    }


def symbol_relation_to_boundary(offset: int, boundary: int | None) -> str:
    if boundary is None:
        return "unknown"
    if offset < boundary:
        return "before-bss-start"
    if offset == boundary:
        return "at-bss-start"
    return "after-bss-start"


def tail_boundary_symbol_records(
    symbols: tuple[MapSymbol, ...],
    *,
    boundary_offset: int,
    bss_boundary: int | None,
    limit: int,
) -> tuple[dict[str, Any], ...]:
    records = []
    for record in boundary_symbols(symbols, boundary_offset, limit):
        enriched = dict(record)
        enriched["relation_to_bss_start"] = symbol_relation_to_boundary(int(enriched["offset"]), bss_boundary)
        records.append(enriched)
    return tuple(records)


def raw_tail_attribution(
    *,
    reference_path: Path,
    candidate_path: Path,
    reference_section: SectionFacts,
    candidate_section: SectionFacts,
    deltas: tuple[SectionDelta, ...],
    map_data: MapSection | None,
    bss_boundary: int | None,
    segment_symbols: tuple[MapSymbol, ...],
    manifest_data_symbols: tuple[ManifestDataSymbolRange, ...],
    bn: BnCoverage,
    limit: int,
) -> dict[str, Any]:
    raw_delta = next((delta.delta for delta in deltas if delta.field == "raw_size"), 0)
    if raw_delta == 0:
        return {
            "available": False,
            "reason": "raw_size matches",
            "raw_size_delta": 0,
        }

    reference_base = 0x400000 + reference_section.rva
    candidate_base = 0x400000 + candidate_section.rva
    size = abs(raw_delta)
    if raw_delta < 0:
        reference_offset_start = reference_section.raw_size - size
        candidate_offset_start = candidate_section.raw_size
        mode = "reference-raw-tail-missing-from-candidate"
        reference_window = tail_window_record(
            start_address=reference_base + reference_offset_start,
            start_offset=reference_offset_start,
            size=size,
            backing="raw-backed",
        )
        candidate_window = tail_window_record(
            start_address=candidate_base + candidate_offset_start,
            start_offset=candidate_offset_start,
            size=size,
            backing=candidate_window_backing(candidate_section, candidate_offset_start, candidate_offset_start + size, map_data, bss_boundary),
        )
        raw_boundary_offset = candidate_section.raw_size
    else:
        reference_offset_start = reference_section.raw_size
        candidate_offset_start = candidate_section.raw_size - size
        mode = "candidate-extra-raw-tail"
        reference_window = tail_window_record(
            start_address=reference_base + reference_offset_start,
            start_offset=reference_offset_start,
            size=size,
            backing=candidate_window_backing(reference_section, reference_offset_start, reference_offset_start + size, None, None),
        )
        candidate_window = tail_window_record(
            start_address=candidate_base + candidate_offset_start,
            start_offset=candidate_offset_start,
            size=size,
            backing="raw-backed",
        )
        raw_boundary_offset = candidate_offset_start
    transition_boundary_offset = (
        bss_boundary if bss_boundary is not None else map_data.end if map_data is not None else raw_boundary_offset
    )

    candidate_map_boundaries: dict[str, Any] = {
        "candidate_raw_end_offset": candidate_section.raw_size,
    }
    if map_data is not None:
        candidate_map_boundaries["data_section_end_offset"] = map_data.end
        candidate_map_boundaries["data_section_end_delta_from_candidate_raw_end"] = map_data.end - candidate_section.raw_size
    if bss_boundary is not None:
        candidate_map_boundaries["bss_start_offset"] = bss_boundary
        candidate_map_boundaries["bss_start_delta_from_candidate_raw_end"] = bss_boundary - candidate_section.raw_size

    reference_tail_items = annotate_tail_manifest_matches(
        bn.raw_tail_items,
        manifest_data_symbols,
        limit=limit,
    )
    return {
        "available": True,
        "mode": mode,
        "raw_size_delta": raw_delta,
        "raw_size_delta_text": signed_hex(raw_delta),
        "reference_tail_window": reference_window,
        "candidate_corresponding_window": candidate_window,
        "reference_tail_byte_summary": tail_byte_summary(
            reference_path,
            reference_section,
            reference_window,
            limit=limit,
        ),
        "candidate_corresponding_window_byte_summary": tail_byte_summary(
            candidate_path,
            candidate_section,
            candidate_window,
            limit=limit,
        ),
        "candidate_map_boundaries": candidate_map_boundaries,
        "bn_available": bn.available,
        "bn_error": "" if bn.available else bn.error,
        "reference_tail_bn_items": reference_tail_items,
        "reference_tail_source_summary": reference_tail_source_summary(
            reference_tail_items,
            limit=limit,
        ),
        "candidate_boundary_map_symbols": tail_boundary_symbol_records(
            segment_symbols,
            boundary_offset=transition_boundary_offset,
            bss_boundary=bss_boundary,
            limit=limit,
        ),
    }


def virtual_tail_attribution(
    *,
    reference_path: Path,
    candidate_path: Path,
    reference_section: SectionFacts,
    candidate_section: SectionFacts,
    deltas: tuple[SectionDelta, ...],
    map_data: MapSection | None,
    bss_boundary: int | None,
    segment_symbols: tuple[MapSymbol, ...],
    manifest_data_symbols: tuple[ManifestDataSymbolRange, ...],
    bn: BnCoverage,
    limit: int,
) -> dict[str, Any]:
    virtual_delta = next((delta.delta for delta in deltas if delta.field == "virtual_size"), 0)
    if virtual_delta == 0:
        return {
            "available": False,
            "reason": "virtual_size matches",
            "virtual_size_delta": 0,
        }

    reference_base = 0x400000 + reference_section.rva
    candidate_base = 0x400000 + candidate_section.rva
    size = abs(virtual_delta)
    if virtual_delta < 0:
        reference_offset_start = reference_section.virtual_size - size
        candidate_offset_start = candidate_section.virtual_size
        mode = "reference-virtual-tail-missing-from-candidate"
        reference_window = tail_window_record(
            start_address=reference_base + reference_offset_start,
            start_offset=reference_offset_start,
            size=size,
            backing=candidate_window_backing(reference_section, reference_offset_start, reference_offset_start + size, None, None),
        )
        candidate_window = tail_window_record(
            start_address=candidate_base + candidate_offset_start,
            start_offset=candidate_offset_start,
            size=size,
            backing=candidate_window_backing(candidate_section, candidate_offset_start, candidate_offset_start + size, map_data, bss_boundary),
        )
        boundary_offset = candidate_section.virtual_size
    else:
        reference_offset_start = reference_section.virtual_size
        candidate_offset_start = candidate_section.virtual_size - size
        mode = "candidate-extra-virtual-tail"
        reference_window = tail_window_record(
            start_address=reference_base + reference_offset_start,
            start_offset=reference_offset_start,
            size=size,
            backing=candidate_window_backing(reference_section, reference_offset_start, reference_offset_start + size, None, None),
        )
        candidate_window = tail_window_record(
            start_address=candidate_base + candidate_offset_start,
            start_offset=candidate_offset_start,
            size=size,
            backing=candidate_window_backing(candidate_section, candidate_offset_start, candidate_offset_start + size, map_data, bss_boundary),
        )
        boundary_offset = candidate_offset_start

    candidate_map_boundaries: dict[str, Any] = {
        "candidate_raw_end_offset": candidate_section.raw_size,
        "candidate_virtual_end_offset": candidate_section.virtual_size,
    }
    if map_data is not None:
        candidate_map_boundaries["data_section_end_offset"] = map_data.end
        candidate_map_boundaries["data_section_end_delta_from_candidate_virtual_end"] = map_data.end - candidate_section.virtual_size
    if bss_boundary is not None:
        candidate_map_boundaries["bss_start_offset"] = bss_boundary
        candidate_map_boundaries["bss_start_delta_from_candidate_virtual_end"] = bss_boundary - candidate_section.virtual_size

    reference_tail_items = annotate_tail_manifest_matches(
        bn.virtual_tail_items,
        manifest_data_symbols,
        limit=limit,
    )
    return {
        "available": True,
        "mode": mode,
        "virtual_size_delta": virtual_delta,
        "virtual_size_delta_text": signed_hex(virtual_delta),
        "reference_tail_window": reference_window,
        "candidate_corresponding_window": candidate_window,
        "reference_tail_byte_summary": tail_byte_summary(
            reference_path,
            reference_section,
            reference_window,
            limit=limit,
        ),
        "candidate_corresponding_window_byte_summary": tail_byte_summary(
            candidate_path,
            candidate_section,
            candidate_window,
            limit=limit,
        ),
        "candidate_map_boundaries": candidate_map_boundaries,
        "bn_available": bn.available,
        "bn_error": "" if bn.available else bn.error,
        "reference_tail_bn_items": reference_tail_items,
        "reference_tail_source_summary": reference_tail_source_summary(
            reference_tail_items,
            limit=limit,
        ),
        "candidate_boundary_map_symbols": tail_boundary_symbol_records(
            segment_symbols,
            boundary_offset=boundary_offset,
            bss_boundary=bss_boundary,
            limit=limit,
        ),
    }


def classification_lines(deltas: tuple[SectionDelta, ...], map_data_section: MapSection | None) -> tuple[str, ...]:
    by_field = {delta.field: delta.delta for delta in deltas}
    lines: list[str] = []
    raw_delta = by_field.get("raw_size", 0)
    virtual_delta = by_field.get("virtual_size", 0)
    zero_delta = by_field.get("zero_fill_tail", 0)
    if raw_delta > 0:
        lines.append(f"candidate initialized .data grew by {signed_hex(raw_delta)}; inspect symbols at/beyond the reference raw boundary and top .data object contributors")
    elif raw_delta < 0:
        lines.append(f"candidate initialized .data shrank by {signed_hex(raw_delta)}; inspect missing initialized globals or section packing")
    else:
        lines.append("candidate initialized .data raw size matches the reference")
    if virtual_delta < 0:
        lines.append(f"candidate total .data virtual size shrank by {signed_hex(virtual_delta)}; likely zero-fill/BSS extent is still short")
    elif virtual_delta > 0:
        lines.append(f"candidate total .data virtual size grew by {signed_hex(virtual_delta)}; likely extra BSS or initialized globals remain")
    if zero_delta:
        lines.append(f"candidate zero-fill tail delta is {signed_hex(zero_delta)}")
    if map_data_section is not None:
        lines.append(f"map .data subsection ends at segment offset {hex32(map_data_section.end)} before linker alignment")
    return tuple(lines)


def artifact_record(path: Path) -> dict[str, Any]:
    resolved = path.resolve()
    if not resolved.is_file():
        return {
            "path": display_path(resolved, REPO_ROOT),
            "present": False,
            "size": None,
        }
    return {
        "path": display_path(resolved, REPO_ROOT),
        "present": True,
        "size": resolved.stat().st_size,
    }


def stable_storage_component(value: str) -> str:
    normalized = value.replace("\\", "/").strip().lower()
    component = re.sub(r"[^a-z0-9._-]+", "-", normalized).strip("-")
    return component or "anonymous"


def normalized_storage_contributions(
    *,
    binary: str,
    section: str,
    object_rows: tuple[ObjectContribution, ...],
    object_paths: tuple[Path, ...],
    map_sections: tuple[MapSection, ...],
    segment_symbols: tuple[MapSymbol, ...],
) -> tuple[dict[str, Any], ...]:
    output_section_id = f"{binary}:section:{section}"
    rows_by_name = {row.object: row for row in object_rows}
    symbol_names = {symbol.object for symbol in segment_symbols if symbol.object}
    names = set(rows_by_name) | symbol_names
    link_order: dict[str, int] = {}
    object_path_by_name: dict[str, Path] = {}
    for index, path in enumerate(object_paths):
        for name in names:
            if name not in link_order and object_name_matches(str(path), name):
                link_order[name] = index
                object_path_by_name[name] = path
    symbols_by_name: dict[str, list[MapSymbol]] = {}
    for symbol in segment_symbols:
        if symbol.object:
            symbols_by_name.setdefault(symbol.object, []).append(symbol)

    def sort_key(name: str) -> tuple[int, int, str]:
        symbols = symbols_by_name.get(name, [])
        first_offset = min((symbol.offset for symbol in symbols), default=0x7FFFFFFF)
        return (link_order.get(name, 0x7FFFFFFF), first_offset, name.lower())

    records: list[dict[str, Any]] = []
    for contribution_index, name in enumerate(sorted(names, key=sort_key)):
        row = rows_by_name.get(name)
        symbols = tuple(sorted(symbols_by_name.get(name, ()), key=lambda item: (item.offset, item.symbol)))
        symbol_kinds = tuple(
            dict.fromkeys(
                map_evidence_kind_for_symbol(
                    symbol,
                    map_sections=map_sections,
                    contributions=rows_by_name,
                )
                for symbol in symbols
            )
        )
        origin = object_origin_record(name)
        provider = origin.get("kind") == "library-member"
        identity_name = str(origin.get("member") or Path(name).name or name)
        records.append(
            {
                "id": (
                    f"{binary}:storage:{'provider-data' if provider else 'object-section'}:"
                    f"{stable_storage_component(identity_name)}:{contribution_index}"
                ),
                "binary": binary,
                "kind": "provider-data" if provider else "object-section",
                "output_section_id": output_section_id,
                "contribution_index": contribution_index,
                "link_order": link_order.get(name),
                "object": name,
                "object_path": display_path(object_path_by_name[name], REPO_ROOT) if name in object_path_by_name else "",
                "object_origin": origin,
                "provenance_kind": object_provenance_kind(name),
                "initialized_size": row.data_size if row is not None else 0,
                "zero_fill_size": row.bss_size if row is not None else 0,
                "coff_sections": normalized_object_sections(row),
                "map_evidence_kind": object_primary_map_evidence_kind(symbol_kinds, row),
                "map_evidence_kinds": symbol_kinds,
                "candidate": {
                    "extent_state": "unknown",
                    "map_offset_start": min((symbol.offset for symbol in symbols), default=None),
                    "map_offset_end": max((symbol.offset for symbol in symbols), default=None),
                    "symbol_count": len(symbols),
                },
                "symbols": tuple(
                    {
                        "address": hex32(symbol.address),
                        "offset": symbol.offset,
                        "symbol": symbol.symbol,
                        "source": symbol.source,
                    }
                    for symbol in symbols
                ),
            }
        )
    return tuple(records)


def build_report(args: argparse.Namespace, *, include_bn: bool = True) -> FinalDataReport:
    reference = Path(args.reference)
    candidate = Path(args.candidate)
    map_path = Path(args.map)
    link_rsp = Path(args.link_rsp)
    manifest_dir = Path(args.manifest_dir)
    require_file(reference, "reference PE")
    require_file(candidate, "candidate PE")
    require_file(map_path, "final map")
    require_file(link_rsp, "link response file")
    if not manifest_dir.is_dir():
        raise ValueError(f"manifest dir does not exist: {manifest_dir}")

    reference_section = pe_section_facts(reference, args.section)
    candidate_section = pe_section_facts(candidate, args.section)
    candidate_file_alignment = pe_file_alignment(candidate)
    deltas = section_deltas(reference_section, candidate_section)
    map_sections, map_symbols = parse_map(map_path)
    map_data = data_map_section(map_sections, args.section)
    segment_symbols = symbols_in_segment(map_symbols, map_data.segment) if map_data else ()
    object_paths = parse_link_rsp_objects(link_rsp)
    object_rows = object_contributions(object_paths)
    manifest = manifest_coverage(manifest_dir, map_symbols, reference_section)
    loaded_manifests = load_manifests(manifest_dir, enforce_source_policy=False)
    manifest_data_symbols = manifest_data_symbol_ranges(loaded_manifests)
    manifest_addresses = {
        int(issue.address, 16)
        for issue in manifest.issues
        if issue.kind in {"candidate-address-drift", "missing-candidate-map-symbol"}
    }
    manifest_addresses.update(int(data_symbol.address, 16) for item in loaded_manifests for data_symbol in item.data_symbols)
    bss_boundary = None
    if map_data is not None:
        bss_candidates = [
            section for section in map_sections if section.segment == map_data.segment and section.name.lower().startswith(".bss")
        ]
        if bss_candidates:
            bss_boundary = min(section.offset for section in bss_candidates)
    raw_delta = next((delta.delta for delta in deltas if delta.field == "raw_size"), 0)
    raw_tail_start = None
    raw_tail_end = None
    if raw_delta:
        reference_base = 0x400000 + reference_section.rva
        raw_tail_size = abs(raw_delta)
        if raw_delta < 0:
            raw_tail_start = reference_base + reference_section.raw_size - raw_tail_size
            raw_tail_end = reference_base + reference_section.raw_size
        else:
            raw_tail_start = reference_base + reference_section.raw_size
            raw_tail_end = reference_base + reference_section.raw_size + raw_tail_size
    virtual_delta = next((delta.delta for delta in deltas if delta.field == "virtual_size"), 0)
    virtual_tail_start = None
    virtual_tail_end = None
    if virtual_delta:
        reference_base = 0x400000 + reference_section.rva
        virtual_tail_size = abs(virtual_delta)
        if virtual_delta < 0:
            virtual_tail_start = reference_base + reference_section.virtual_size - virtual_tail_size
            virtual_tail_end = reference_base + reference_section.virtual_size
        else:
            virtual_tail_start = reference_base + reference_section.virtual_size
            virtual_tail_end = reference_base + reference_section.virtual_size + virtual_tail_size
    if include_bn:
        bn = bn_coverage(
            bridge_url=args.bridge_url,
            section_name=args.section,
            reference_section=reference_section,
            manifest_covered_addresses=manifest_addresses,
            raw_tail_start=raw_tail_start,
            raw_tail_end=raw_tail_end,
            virtual_tail_start=virtual_tail_start,
            virtual_tail_end=virtual_tail_end,
            limit=args.limit,
        )
    else:
        bn = BnCoverage(available=False, error="not requested")
    raw_tail = raw_tail_attribution(
        reference_path=reference,
        candidate_path=candidate,
        reference_section=reference_section,
        candidate_section=candidate_section,
        deltas=deltas,
        map_data=map_data,
        bss_boundary=bss_boundary,
        segment_symbols=segment_symbols,
        manifest_data_symbols=manifest_data_symbols,
        bn=bn,
        limit=args.limit,
    )
    virtual_tail = virtual_tail_attribution(
        reference_path=reference,
        candidate_path=candidate,
        reference_section=reference_section,
        candidate_section=candidate_section,
        deltas=deltas,
        map_data=map_data,
        bss_boundary=bss_boundary,
        segment_symbols=segment_symbols,
        manifest_data_symbols=manifest_data_symbols,
        bn=bn,
        limit=args.limit,
    )
    boundary_packing = candidate_boundary_packing(
        map_sections=map_sections,
        segment_symbols=segment_symbols,
        candidate_section=candidate_section,
        data_section=map_data,
        bss_boundary=bss_boundary,
        object_rows=object_rows,
        limit=args.limit,
    )
    boundary_contribution_summary = candidate_boundary_contribution_summary(
        map_sections=map_sections,
        segment_symbols=segment_symbols,
        candidate_section=candidate_section,
        data_section=map_data,
        bss_boundary=bss_boundary,
        object_rows=object_rows,
        limit=args.limit,
    )
    initialized_thresholds = candidate_initialized_data_thresholds(
        map_sections=map_sections,
        segment_symbols=segment_symbols,
        reference_section=reference_section,
        candidate_section=candidate_section,
        data_section=map_data,
        bss_boundary=bss_boundary,
        object_rows=object_rows,
        candidate_file_alignment=candidate_file_alignment,
        limit=args.limit,
    )
    threshold_attribution = candidate_threshold_attribution(
        thresholds=initialized_thresholds,
        map_sections=map_sections,
        segment_symbols=segment_symbols,
        reference_section=reference_section,
        candidate_section=candidate_section,
        data_section=map_data,
        bss_boundary=bss_boundary,
        object_rows=object_rows,
        candidate_file_alignment=candidate_file_alignment,
        limit=args.limit,
    )
    subsection_attribution = candidate_object_subsection_attribution(
        thresholds=initialized_thresholds,
        map_sections=map_sections,
        segment_symbols=segment_symbols,
        reference_section=reference_section,
        candidate_section=candidate_section,
        data_section=map_data,
        bss_boundary=bss_boundary,
        object_rows=object_rows,
        candidate_file_alignment=candidate_file_alignment,
        limit=args.limit,
        manifest_coverage=manifest,
        raw_tail_attribution=raw_tail,
        virtual_tail_attribution=virtual_tail,
    )
    object_traces = candidate_object_traces(
        requested_objects=parse_trace_object_names(getattr(args, "trace_object", ())),
        map_sections=map_sections,
        segment_symbols=segment_symbols,
        reference_section=reference_section,
        candidate_section=candidate_section,
        data_section=map_data,
        bss_boundary=bss_boundary,
        object_rows=object_rows,
        object_paths=object_paths,
        limit=args.limit,
    )
    rankings = {
        "symbols_at_or_beyond_reference_raw_boundary": symbols_at_or_beyond(segment_symbols, reference_section.raw_size, args.limit),
        "data_boundary_symbols": boundary_symbols(segment_symbols, reference_section.raw_size, args.limit),
        "bss_boundary_symbols": boundary_symbols(segment_symbols, bss_boundary, args.limit) if bss_boundary is not None else (),
        "candidate_map_section_transitions": map_section_transitions(
            map_sections=map_sections,
            segment_symbols=segment_symbols,
            candidate_section=candidate_section,
            data_section=map_data,
            bss_boundary=bss_boundary,
            object_rows=object_rows,
            limit=args.limit,
        ),
        "top_object_data_contributors": top_contributions(object_rows, "data_size", args.limit),
        "top_object_bss_contributors": top_contributions(object_rows, "bss_size", args.limit),
        "object_count": len(object_rows),
    }
    map_section_records = tuple(map_section_record(section) for section in map_sections if section.section_class == "DATA")
    binary = str(getattr(args, "binary", "recoil"))
    storage = normalized_storage_contributions(
        binary=binary,
        section=args.section,
        object_rows=object_rows,
        object_paths=object_paths,
        map_sections=map_sections,
        segment_symbols=segment_symbols,
    )
    return FinalDataReport(
        reference=display_path(reference, REPO_ROOT),
        candidate=display_path(candidate, REPO_ROOT),
        map=display_path(map_path, REPO_ROOT),
        link_rsp=display_path(link_rsp, REPO_ROOT),
        section=args.section,
        reference_section=reference_section,
        candidate_section=candidate_section,
        deltas=deltas,
        rankings=rankings,
        map_sections=map_section_records,
        manifest_coverage=manifest,
        bn_coverage=bn,
        classifications=classification_lines(deltas, map_data),
        raw_byte_comparison=compare_section_byte_slices(
            reference, candidate, reference_section, candidate_section
        ),
        candidate_threshold_attribution=threshold_attribution,
        candidate_initialized_data_thresholds=initialized_thresholds,
        candidate_object_subsection_attribution=subsection_attribution,
        candidate_object_traces=object_traces,
        candidate_boundary_contribution_summary=boundary_contribution_summary,
        candidate_boundary_packing=boundary_packing,
        raw_tail_attribution=raw_tail,
        virtual_tail_attribution=virtual_tail,
        binary=binary,
        output_section_id=f"{binary}:section:{args.section}",
        storage_contributions=storage,
    )


def print_section_report(report: FinalDataReport) -> None:
    print(f"final-data audit section={report.section}")
    print(f"reference={report.reference}")
    print(f"candidate={report.candidate}")
    print("section_deltas:")
    for delta in report.deltas:
        print(
            f"- {delta.field}: reference={hex32(delta.reference)} "
            f"candidate={hex32(delta.candidate)} delta={signed_hex(delta.delta)}"
        )
    print("likely_causes:")
    for line in report.classifications:
        print(f"- {line}")


def delta_record(delta: SectionDelta) -> dict[str, int | str]:
    return {
        "field": delta.field,
        "reference": delta.reference,
        "candidate": delta.candidate,
        "delta": delta.delta,
        "delta_text": signed_hex(delta.delta),
    }


def command_path(path: Path) -> str:
    return response_line(display_path(path.resolve(), REPO_ROOT))


def final_data_command_suffix(
    *,
    reference_path: Path | None = None,
    candidate_path: Path | None = None,
    map_path: Path | None = None,
    link_rsp_path: Path | None = None,
) -> str:
    options: list[str] = []
    if reference_path is not None:
        options.extend(["--reference", command_path(reference_path)])
    if candidate_path is not None:
        options.extend(["--candidate", command_path(candidate_path)])
    if map_path is not None:
        options.extend(["--map", command_path(map_path)])
    if link_rsp_path is not None:
        options.extend(["--link-rsp", command_path(link_rsp_path)])
    return "" if not options else " " + " ".join(options)


def print_ranked_records(title: str, records: tuple[dict[str, Any], ...]) -> None:
    print(f"{title}:")
    if not records:
        print("- none")
        return
    for record in records:
        if "symbol" in record:
            print(
                f"- {record['address']} offset={hex32(int(record['offset']))} "
                f"symbol={record['symbol']} object={record['object'] or '-'}"
            )
        elif "object" in record:
            print(f"- {record['object']}: data={hex32(record['data_size'])} bss={hex32(record['bss_size'])}")
        elif "boundary_offset" in record:
            preceding = record.get("preceding_section", {})
            following = record.get("following_section", {})
            preceding_name = preceding.get("name", "-") if isinstance(preceding, dict) else "-"
            following_name = following.get("name", "-") if isinstance(following, dict) else "-"
            print(
                f"- {record['name']}: address={record['boundary_address']} "
                f"offset={hex32(int(record['boundary_offset']))} "
                f"{preceding_name}-> {following_name} "
                f"gap_after_prev={signed_hex(int(record.get('gap_from_preceding_end', 0)))} "
                f"gap_to_next={signed_hex(int(record.get('gap_to_following_start', 0)))}"
            )
        else:
            print(f"- {record}")


def print_report(report: FinalDataReport, *, limit: int) -> None:
    print_section_report(report)
    print_raw_tail_attribution(report.raw_tail_attribution, limit=limit)
    print_virtual_tail_attribution(report.virtual_tail_attribution, limit=limit)
    print_candidate_threshold_attribution(report.candidate_threshold_attribution, limit=limit)
    print_candidate_initialized_data_thresholds(report.candidate_initialized_data_thresholds, limit=limit)
    print_candidate_object_subsection_attribution(report.candidate_object_subsection_attribution, limit=limit)
    print_candidate_object_traces(report.candidate_object_traces, limit=limit)
    print_candidate_boundary_contribution_summary(report.candidate_boundary_contribution_summary, limit=limit)
    print_candidate_boundary_packing(report.candidate_boundary_packing, limit=limit)
    print_ranked_records("symbols_at_or_beyond_reference_raw_boundary", report.rankings["symbols_at_or_beyond_reference_raw_boundary"])
    print_ranked_records("top_object_data_contributors", report.rankings["top_object_data_contributors"])
    print_ranked_records("top_object_bss_contributors", report.rankings["top_object_bss_contributors"])
    print_ranked_records("data_boundary_symbols", report.rankings["data_boundary_symbols"])
    print_ranked_records("bss_boundary_symbols", report.rankings["bss_boundary_symbols"])
    print_ranked_records("candidate_map_section_transitions", report.rankings["candidate_map_section_transitions"])
    coverage = report.manifest_coverage
    print("manifest_coverage:")
    print(
        f"- manifests={coverage.manifest_count} data_symbols={coverage.data_symbol_count} "
        f"in_reference_section={coverage.in_reference_section} "
        f"symbol_name_matches={coverage.symbol_name_matches} exact_address_matches={coverage.exact_address_matches}"
    )
    for issue in coverage.issues[:limit]:
        print(f"- {issue.kind}: {issue.address} {issue.name} target={issue.target} {issue.detail}")
    bn = report.bn_coverage
    print("bn_coverage:")
    if not bn.available:
        print(f"- unavailable: {bn.error}")
    else:
        print(
            f"- items={bn.item_count} section_items={bn.section_item_count} "
            f"manifest_covered={bn.manifest_covered_count} uncovered={len(bn.uncovered_items)}"
        )
        for item in bn.uncovered_items[:limit]:
            print(f"- uncovered_bn_item: {item['address']} size={item['size']} name={item['name']}")


def print_candidate_threshold_attribution(attribution: dict[str, Any], *, limit: int) -> None:
    if not attribution.get("available"):
        return
    summary = attribution.get("summary", {})
    print("candidate_threshold_attribution:")
    print(
        f"- segment={summary.get('segment', '-')} "
        f"data_end={hex32(int(summary.get('candidate_initialized_data_end_offset', 0)))} "
        f"data_end_address={summary.get('candidate_initialized_data_end_address', '-')} "
        f"bss_start={hex32(int(summary.get('bss_start_offset', 0))) if 'bss_start_offset' in summary else '-'} "
        f"bss_start_address={summary.get('bss_start_address', '-')} "
        f"candidate_raw_end={hex32(int(summary.get('candidate_raw_end_offset', 0)))} "
        f"candidate_raw_end_address={summary.get('candidate_raw_end_address', '-')} "
        f"reference_raw_end={hex32(int(summary.get('reference_raw_end_offset', 0)))} "
        f"reference_raw_end_address={summary.get('reference_raw_end_address', '-')}"
    )
    for key in (
        "data_end_to_candidate_raw_end_slack",
        "data_end_to_bss_start_slack",
        "raw_end_to_bss_start_slack",
        "data_end_to_reference_raw_end_bytes",
        "current_raw_end_to_reference_raw_end_bytes",
    ):
        if key in summary:
            print(f"- {key}={signed_hex(int(summary[key]))}")
    thresholds = tuple(attribution.get("raw_alignment_thresholds", ()))
    print("raw_alignment_thresholds:")
    if not thresholds:
        print("- none")
    for row in thresholds[:limit]:
        print(
            f"- target={hex32(int(row.get('target_raw_end_offset', 0)))} "
            f"address={row.get('target_raw_end_address', '-')} "
            f"bytes_needed_from_data_end={hex32(int(row.get('bytes_needed_from_data_end', 0)))} "
            f"bytes_to_fill={hex32(int(row.get('bytes_to_fill_target_raw_end', 0)))} "
            f"bytes_from_current_raw_end={signed_hex(int(row.get('bytes_needed_from_current_raw_end', 0)))} "
            f"matches_reference={bool(row.get('matches_reference_raw_end'))}"
        )
    print("attribution_points:")
    for point in tuple(attribution.get("attribution_points", ()))[:limit]:
        containing = point.get("containing_section", {})
        containing_name = containing.get("name", "-") if isinstance(containing, dict) else "-"
        print(
            f"- {point.get('name', '-')} offset={hex32(int(point.get('offset', 0)))} "
            f"address={point.get('address', '-')} kind={point.get('map_evidence_kind', 'unknown')} "
            f"containing={containing_name}"
        )
        for label in ("objects_before", "objects_after"):
            objects = tuple(point.get(label, ()))
            if not objects:
                continue
            print(f"-   {label}:")
            for item in objects[:limit]:
                sections = tuple(item.get("sections", ()))
                rendered_sections = ", ".join(
                    f"{section.get('name', '?')}@{section.get('order', '?')}={hex32(int(section.get('size', 0)))}"
                    for section in sections
                ) or "-"
                print(
                    f"-     object={item.get('object', '-')} "
                    f"kind={item.get('map_evidence_kind', 'unknown')} "
                    f"data={hex32(int(item.get('data_size', 0)))} "
                    f"bss={hex32(int(item.get('bss_size', 0)))} "
                    f"origin={item.get('object_origin', {}).get('kind', '-')} "
                    f"sections={rendered_sections}"
                )
    limitations = tuple(attribution.get("limitations", ()))
    if limitations:
        print("limitations:")
        for item in limitations[:limit]:
            print(f"- {item}")


def print_candidate_object_subsection_attribution(attribution: dict[str, Any], *, limit: int) -> None:
    if not attribution.get("available"):
        return
    summary = attribution.get("summary", {})
    print("candidate_object_subsection_attribution:")
    print(
        f"- segment={summary.get('segment', '-')} "
        f"data_end={hex32(int(summary.get('candidate_initialized_data_end_offset', 0)))} "
        f"bss_start={hex32(int(summary.get('bss_start_offset', 0))) if 'bss_start_offset' in summary else '-'} "
        f"candidate_raw_end={hex32(int(summary.get('candidate_raw_end_offset', 0)))} "
        f"reference_raw_end={hex32(int(summary.get('reference_raw_end_offset', 0)))} "
        f"objects={int(summary.get('selected_object_count', 0))}"
    )
    objects = tuple(attribution.get("objects", ()))
    if not objects:
        print("- objects: none")
    for item in objects[:limit]:
        threshold_rows = tuple(item.get("can_satisfy_thresholds_by_size", ()))
        threshold_text = ", ".join(
            f"{hex32(int(row.get('threshold_bytes', 0)))}:"
            f"data={bool(row.get('initialized_data_size_satisfies'))}/"
            f"bss={bool(row.get('bss_size_satisfies'))}"
            for row in threshold_rows
        ) or "-"
        sections = tuple(item.get("sections", ()))
        section_text = ", ".join(
            f"{section.get('name', '?')}@{section.get('order', '?')}="
            f"{hex32(int(section.get('size', 0)))}:{section.get('kind', 'unknown')}"
            for section in sections
        ) or "-"
        reasons = ",".join(str(reason) for reason in tuple(item.get("selection_reasons", ()))[:limit]) or "-"
        selected = tuple(item.get("selected_coff_symbols", ()))
        selected_hits = sum(1 for row in selected if row.get("lookup_status") == "matched")
        print(
            f"- object={item.get('object', '-')} "
            f"origin={item.get('provenance_kind', '-')} "
            f"kind={item.get('map_evidence_kind', 'unknown')} "
            f"data={hex32(int(item.get('initialized_data_contribution_size', 0)))} "
            f"bss={hex32(int(item.get('bss_contribution_size', 0)))} "
            f"symbols={int(item.get('map_symbol_count', 0))} "
            f"selected_coff={selected_hits}/{len(selected)} "
            f"thresholds={threshold_text} "
            f"reasons={reasons} "
            f"sections={section_text}"
        )
        if selected:
            entries = []
            for row in selected[:limit]:
                if row.get("lookup_status") == "matched":
                    correlations = tuple(row.get("manifest_issue_correlations", ()))
                    correlation_text = ""
                    if correlations:
                        first = correlations[0]
                        reference_offset = first.get("reference_offset")
                        delta = first.get("candidate_offset_delta_from_reference")
                        if reference_offset is not None and delta is not None:
                            correlation_text = (
                                f"[ref={hex32(int(reference_offset))} "
                                f"delta={signed_hex(int(delta))} "
                                f"ref_tail={bool(first.get('reference_in_raw_tail_gap'))}]"
                            )
                    entries.append(
                        f"{row.get('symbol')}:{row.get('coff_section_name')}@"
                        f"{row.get('coff_section_order')}+{hex32(int(row.get('coff_value', 0)))}"
                        f"{correlation_text}"
                    )
                else:
                    entries.append(f"{row.get('symbol')}:miss({row.get('lookup_status')})")
            suffix = ""
            if len(selected) > limit:
                suffix = f", ... {len(selected) - limit} more"
            print(f"  selected_coff_symbols={', '.join(entries)}{suffix}")
    limitations = tuple(attribution.get("limitations", ()))
    if limitations:
        print("candidate_object_subsection_attribution_limitations:")
        for item in limitations[:limit]:
            print(f"- {item}")


def print_candidate_object_traces(trace: dict[str, Any], *, limit: int) -> None:
    if not trace.get("available"):
        return
    summary = trace.get("summary", {})
    print("candidate_object_traces:")
    print(
        f"- segment={summary.get('segment', '-')} "
        f"data_end={hex32(int(summary.get('candidate_data_end_offset', 0)))} "
        f"bss_start={hex32(int(summary.get('bss_start_offset', 0))) if summary.get('bss_start_offset') is not None else '-'} "
        f"candidate_raw_end={hex32(int(summary.get('candidate_raw_end_offset', 0)))} "
        f"reference_raw_end={hex32(int(summary.get('reference_raw_end_offset', 0)))} "
        f"candidate_virtual_end={hex32(int(summary.get('candidate_virtual_end_offset', 0)))} "
        f"objects={int(summary.get('object_count', 0))}"
    )
    for item in tuple(trace.get("objects", ()))[:limit]:
        map_contribution = item.get("map_contribution", {})
        relationships = item.get("threshold_relationships", {})
        coff_sections = tuple(item.get("coff_sections", ()))
        section_text = ", ".join(
            f"#{int(section.get('index', 0))}:{section.get('name', '?')}@"
            f"{section.get('order', '?')}={hex32(int(section.get('size', 0)))}:"
            f"{section.get('kind', 'unknown')}:chars={section.get('characteristics_hex', '-')}"
            for section in coff_sections[:limit]
        ) or "-"
        print(
            f"- object={item.get('object', '-')} "
            f"requested={item.get('requested_object', '-')} "
            f"path={item.get('object_path') or '-'} "
            f"link_rsp_order={item.get('link_rsp_order') if item.get('link_rsp_order') is not None else '-'} "
            f"coff_status={item.get('coff_status', '-')} "
            f"map_kind={map_contribution.get('map_evidence_kind', 'unknown')} "
            f"data={hex32(int(map_contribution.get('initialized_data_contribution_size', 0)))} "
            f"bss={hex32(int(map_contribution.get('bss_contribution_size', 0)))} "
            f"map_symbols={int(map_contribution.get('map_symbol_count', 0))}"
        )
        print(f"  coff_sections={section_text}")
        print(
            "  thresholds="
            f"data_end={hex32(int(relationships.get('candidate_data_end_offset', 0)))} "
            f"raw_end={hex32(int(relationships.get('candidate_raw_end_offset', 0)))} "
            f"reference_raw_end={hex32(int(relationships.get('reference_raw_end_offset', 0)))} "
            f"virtual_end={hex32(int(relationships.get('candidate_virtual_end_offset', 0)))}"
        )
        selected = tuple(item.get("selected_symbols", ()))
        if not selected:
            print("  selected_symbols=none")
            continue
        print("  selected_symbols:")
        for symbol in selected[:limit]:
            rel = symbol.get("relationships", {})
            raw_rel = rel.get("candidate_raw_end", {}) if isinstance(rel, dict) else {}
            ref_rel = rel.get("reference_raw_end", {}) if isinstance(rel, dict) else {}
            print(
                f"  - {symbol.get('symbol', '-')} "
                f"map_offset={hex32(int(symbol.get('map_offset', 0)))} "
                f"map_rva={hex32(int(symbol.get('map_rva', 0)))} "
                f"map_address={symbol.get('map_address', '-')} "
                f"coff={symbol.get('coff_section_name') or '-'}@"
                f"{symbol.get('coff_section_order') if symbol.get('coff_section_order') is not None else '-'}+"
                f"{hex32(int(symbol.get('coff_value', 0))) if symbol.get('coff_value') is not None else '-'} "
                f"status={symbol.get('lookup_status', '-')} "
                f"raw_end={raw_rel.get('relation', 'unknown')}({signed_hex(int(raw_rel.get('delta', 0))) if raw_rel.get('delta') is not None else '-'}) "
                f"reference_raw_end={ref_rel.get('relation', 'unknown')}({signed_hex(int(ref_rel.get('delta', 0))) if ref_rel.get('delta') is not None else '-'})"
            )
        truncated = int(item.get("selected_symbols_truncated_count", 0))
        if truncated:
            print(f"  - ... {truncated} more selected symbols")
    limitations = tuple(trace.get("limitations", ()))
    if limitations:
        print("candidate_object_trace_limitations:")
        for item in limitations[:limit]:
            print(f"- {item}")


def print_candidate_initialized_data_thresholds(thresholds: dict[str, Any], *, limit: int) -> None:
    if not thresholds.get("available"):
        return
    summary = thresholds.get("summary", {})
    print("candidate_initialized_data_thresholds:")
    data_end = summary.get("candidate_initialized_data_end_offset", summary.get("data_end_offset"))
    bss_start = summary.get("bss_start_offset")
    raw_end = summary.get("candidate_raw_end_offset")
    virtual_end = summary.get("candidate_virtual_end_offset")
    reference_raw_end = summary.get("reference_raw_end_offset")
    file_alignment = summary.get("candidate_file_alignment")
    print(
        f"- segment={summary.get('segment', '-')} "
        f"data_end={hex32(int(data_end)) if data_end is not None else '-'} "
        f"bss_start={hex32(int(bss_start)) if bss_start is not None else '-'} "
        f"candidate_raw_end={hex32(int(raw_end)) if raw_end is not None else '-'} "
        f"candidate_virtual_end={hex32(int(virtual_end)) if virtual_end is not None else '-'} "
        f"reference_raw_end={hex32(int(reference_raw_end)) if reference_raw_end is not None else '-'} "
        f"file_alignment={hex32(int(file_alignment)) if file_alignment is not None else '-'}"
    )
    for key in (
        "data_end_to_candidate_raw_end_slack",
        "data_end_to_bss_start_slack",
        "raw_end_to_bss_start_slack",
        "data_end_to_reference_raw_end_bytes",
        "current_raw_end_to_reference_raw_end_bytes",
    ):
        if key in summary:
            print(f"- {key}={signed_hex(int(summary[key]))}")
    for row in tuple(thresholds.get("thresholds", ()))[:limit]:
        containing = row.get("containing_section", {})
        following = row.get("following_section", {})
        containing_name = containing.get("name", "-") if isinstance(containing, dict) else "-"
        following_name = following.get("name", "-") if isinstance(following, dict) else "-"
        print(
            f"- threshold={hex32(int(row.get('target_raw_end_offset', 0)))} "
            f"address={row.get('target_raw_end_address', '-')} "
            f"bytes_from_data_end={hex32(int(row.get('bytes_needed_from_data_end', 0)))} "
            f"bytes_to_fill={hex32(int(row.get('bytes_to_fill_target_raw_end', 0)))} "
            f"bytes_from_current_raw_end={signed_hex(int(row.get('bytes_needed_from_current_raw_end', 0)))} "
            f"matches_reference={bool(row.get('matches_reference_raw_end'))} "
            f"reaches_reference={bool(row.get('reaches_reference_raw_end'))} "
            f"containing={containing_name} following={following_name}"
        )
        before_objects = tuple(row.get("nearby_objects_before", ()))
        after_objects = tuple(row.get("nearby_objects_after", ()))
        if before_objects:
            print("-   nearby_objects_before:")
            for item in before_objects[:limit]:
                print_candidate_threshold_object(item, relation="before")
        if after_objects:
            print("-   nearby_objects_after:")
            for item in after_objects[:limit]:
                print_candidate_threshold_object(item, relation="after")


def print_candidate_threshold_object(item: dict[str, Any], *, relation: str) -> None:
    sections = tuple(item.get("sections", ()))
    if sections:
        rendered_sections = ", ".join(
            f"{section.get('name', '?')}@{section.get('order', '?')}={hex32(int(section.get('size', 0)))}"
            for section in sections
        )
    else:
        rendered_sections = "-"
    print(
        f"-     {relation} object={item.get('object', '-')} "
        f"first_symbol={item.get('first_symbol', '-')} "
        f"first_offset={hex32(int(item.get('first_symbol_offset', 0)))} "
        f"data={hex32(int(item.get('data_size', 0)))} "
        f"bss={hex32(int(item.get('bss_size', 0)))} "
        f"origin={item.get('object_origin', {}).get('kind', '-')} "
        f"sections={rendered_sections}"
    )


def print_candidate_boundary_contribution_summary(summary: dict[str, Any], *, limit: int) -> None:
    if not summary.get("available"):
        return
    header = summary.get("summary", {})
    print("candidate_boundary_contribution_summary:")
    data_end = header.get("data_end_offset")
    raw_end = header.get("candidate_raw_end_offset")
    bss_start = header.get("bss_start_offset")
    virtual_end = header.get("candidate_virtual_end_offset")
    print(
        f"- segment={header.get('segment', '-')} "
        f"data_end={hex32(int(data_end)) if data_end is not None else '-'} "
        f"bss_start={hex32(int(bss_start)) if bss_start is not None else '-'} "
        f"raw_end={hex32(int(raw_end)) if raw_end is not None else '-'} "
        f"virtual_end={hex32(int(virtual_end)) if virtual_end is not None else '-'}"
    )
    for key in (
        "data_end_to_bss_start_slack",
        "raw_end_to_bss_start_slack",
        "bss_start_to_virtual_end_slack",
        "data_end_to_raw_end_slack",
        "raw_end_to_virtual_end_slack",
    ):
        if key in header:
            print(f"- {key}={signed_hex(int(header[key]))}")
    for boundary in tuple(summary.get("boundaries", ()))[:limit]:
        preceding = boundary.get("preceding_section", {})
        following = boundary.get("following_section", {})
        containing = boundary.get("containing_section", {})
        preceding_name = preceding.get("name", "-") if isinstance(preceding, dict) else "-"
        following_name = following.get("name", "-") if isinstance(following, dict) else "-"
        containing_name = containing.get("name", "-") if isinstance(containing, dict) else "-"
        print(
            f"- boundary={boundary.get('name', '-')} address={boundary.get('address', '-')} "
            f"offset={hex32(int(boundary.get('offset', 0)))} "
            f"prev={preceding_name} next={following_name} containing={containing_name}"
        )
        before_objects = tuple(boundary.get("objects_before", ()))
        after_objects = tuple(boundary.get("objects_after", ()))
        if before_objects:
            print("-   objects_before:")
            for item in before_objects[:limit]:
                print_boundary_contribution_object(item, relation="before", limit=limit)
        if after_objects:
            print("-   objects_after:")
            for item in after_objects[:limit]:
                print_boundary_contribution_object(item, relation="after", limit=limit)


def print_boundary_contribution_object(item: dict[str, Any], *, relation: str, limit: int) -> None:
    symbols = tuple(item.get("symbols", ()))
    rendered_symbols = ", ".join(
        f"{symbol.get('symbol', '-')}@{hex32(int(symbol.get('offset', 0)))}"
        for symbol in symbols[:limit]
    ) or "-"
    print(
        f"-     {relation} object={item.get('object', '-')} "
        f"data={hex32(int(item.get('data_size', 0)))} "
        f"bss={hex32(int(item.get('bss_size', 0)))} "
        f"origin={item.get('object_origin', {}).get('kind', '-')} "
        f"symbols={rendered_symbols}"
    )


def print_candidate_boundary_packing(packing: dict[str, Any], *, limit: int) -> None:
    if not packing.get("available"):
        return
    summary = packing.get("summary", {})
    print("candidate_boundary_packing:")
    data_end = summary.get("data_section_end_offset")
    raw_end = summary.get("candidate_raw_end_offset")
    bss_start = summary.get("bss_start_offset")
    virtual_end = summary.get("candidate_virtual_end_offset")
    print(
        f"- segment={summary.get('segment', '-')} "
        f"map_data_end={hex32(int(data_end)) if data_end is not None else '-'} "
        f"raw_end={hex32(int(raw_end)) if raw_end is not None else '-'} "
        f"bss_start={hex32(int(bss_start)) if bss_start is not None else '-'} "
        f"virtual_end={hex32(int(virtual_end)) if virtual_end is not None else '-'}"
    )
    for key in (
        "data_end_to_raw_end_slack",
        "data_end_to_bss_start_slack",
        "raw_end_to_bss_start_slack",
        "bss_start_to_virtual_end_slack",
        "raw_end_to_virtual_end_slack",
    ):
        if key in summary:
            print(f"- {key}={signed_hex(int(summary[key]))}")
    for boundary in tuple(packing.get("boundaries", ()))[:limit]:
        preceding = boundary.get("preceding_section", {})
        following = boundary.get("following_section", {})
        containing = boundary.get("containing_section", {})
        preceding_name = preceding.get("name", "-") if isinstance(preceding, dict) else "-"
        following_name = following.get("name", "-") if isinstance(following, dict) else "-"
        containing_name = containing.get("name", "-") if isinstance(containing, dict) else "-"
        print(
            f"- boundary={boundary.get('name', '-')} address={boundary.get('address', '-')} "
            f"offset={hex32(int(boundary.get('offset', 0)))} "
            f"prev={preceding_name} next={following_name} containing={containing_name} "
            f"gap_after_prev={signed_hex(int(boundary.get('gap_from_preceding_end', 0)))} "
            f"gap_to_next={signed_hex(int(boundary.get('gap_to_following_start', 0)))} "
            f"slack_before={signed_hex(int(boundary.get('slack_before_within_containing', 0)))} "
            f"slack_after={signed_hex(int(boundary.get('slack_after_within_containing', 0)))}"
        )
        before_objects = tuple(boundary.get("nearby_objects_before", ()))
        after_objects = tuple(boundary.get("nearby_objects_after", ()))
        if before_objects:
            print("-   nearby_objects_before:")
            for item in before_objects[:limit]:
                print_candidate_packing_object(item)
        if after_objects:
            print("-   nearby_objects_after:")
            for item in after_objects[:limit]:
                print_candidate_packing_object(item)


def print_candidate_packing_object(item: dict[str, Any]) -> None:
    sections = tuple(item.get("sections", ()))
    if sections:
        rendered_sections = ", ".join(
            f"{section.get('name', '?')}@{section.get('order', '?')}={hex32(int(section.get('size', 0)))}"
            for section in sections
        )
    else:
        rendered_sections = "-"
    print(
        f"-     object={item.get('object', '-')} "
        f"first_symbol={item.get('first_symbol', '-')} "
        f"first_offset={hex32(int(item.get('first_symbol_offset', 0)))} "
        f"data={hex32(int(item.get('data_size', 0)))} "
        f"bss={hex32(int(item.get('bss_size', 0)))} "
        f"sections={rendered_sections}"
    )


def print_raw_tail_attribution(attribution: dict[str, Any], *, limit: int) -> None:
    if not attribution.get("available"):
        return
    reference_window = attribution.get("reference_tail_window", {})
    candidate_window = attribution.get("candidate_corresponding_window", {})
    print("raw_tail_attribution:")
    print(
        f"- mode={attribution.get('mode', '')} "
        f"raw_size_delta={attribution.get('raw_size_delta_text', signed_hex(int(attribution.get('raw_size_delta', 0))))} "
        f"reference_tail={reference_window.get('start', '?')}..{reference_window.get('end', '?')} "
        f"reference_backing={reference_window.get('backing', '?')} "
        f"candidate_window={candidate_window.get('start', '?')}..{candidate_window.get('end', '?')} "
        f"candidate_backing={candidate_window.get('backing', '?')}"
    )
    print_tail_byte_summary(
        "reference_tail_byte_summary",
        attribution.get("reference_tail_byte_summary", {}),
        limit=limit,
    )
    print_tail_byte_summary(
        "candidate_corresponding_window_byte_summary",
        attribution.get("candidate_corresponding_window_byte_summary", {}),
        limit=limit,
    )
    boundaries = attribution.get("candidate_map_boundaries", {})
    if boundaries:
        data_end = boundaries.get("data_section_end_offset")
        bss_start = boundaries.get("bss_start_offset")
        print(
            f"- candidate_offsets raw_end={hex32(int(boundaries.get('candidate_raw_end_offset', 0)))} "
            f"map_data_end={hex32(int(data_end)) if data_end is not None else '-'} "
            f"bss_start={hex32(int(bss_start)) if bss_start is not None else '-'}"
        )
    print("reference_tail_bn_items:")
    if not attribution.get("bn_available"):
        print(f"- unavailable: {attribution.get('bn_error', '')}")
    else:
        items = tuple(attribution.get("reference_tail_bn_items", ()))
        print_reference_tail_source_summary(attribution.get("reference_tail_source_summary", {}), limit=limit)
        if not items:
            print("- none")
        for item in items[:limit]:
            classification = item.get("classification") or "-"
            print(
                f"- {item.get('range', item.get('address', '?'))} size={item.get('size', 0)} "
                f"backing={item.get('backing', '?')} name={item.get('name', '')} "
                f"type={item.get('type', '') or '-'} classification={classification}"
            )
            print_tail_manifest_matches(item, limit=limit)
    print("candidate_boundary_map_symbols:")
    symbols = tuple(attribution.get("candidate_boundary_map_symbols", ()))
    if not symbols:
        print("- none")
    for symbol in symbols[:limit]:
        print(
            f"- {symbol['address']} offset={hex32(int(symbol['offset']))} "
            f"symbol={symbol['symbol']} object={symbol['object'] or '-'} "
            f"relation_to_bss_start={symbol.get('relation_to_bss_start', 'unknown')}"
        )


def print_tail_manifest_matches(item: dict[str, Any], *, limit: int) -> None:
    matches = tuple(item.get("manifest_matches", ()))
    match_count = int(item.get("manifest_match_count", len(matches)))
    if not matches:
        print("-   manifest_matches=unmatched")
        return
    rendered = []
    for match in matches[:limit]:
        rendered.append(
            f"{match.get('target', '?')}:{match.get('name', '?')} "
            f"{match.get('address', '?')} len={match.get('byte_length', '?')} "
            f"overlap={match.get('overlap_range', '?')} source={match.get('source_from') or match.get('source_filename', '-')}"
        )
    suffix = ""
    truncated = int(item.get("manifest_matches_truncated", 0))
    if truncated:
        suffix = f"; +{truncated} more"
    elif match_count > len(rendered):
        suffix = f"; +{match_count - len(rendered)} more"
    print(f"-   manifest_matches={'; '.join(rendered)}{suffix}")


def print_reference_tail_source_summary(summary: dict[str, Any], *, limit: int) -> None:
    print("reference_tail_source_summary:")
    groups = tuple(summary.get("source_groups", ()))
    if (
        not groups
        and not int(summary.get("unmatched_item_count", 0))
        and not int(summary.get("manifest_uncovered_span_count", 0))
    ):
        print("- none")
        return
    uncovered_count = int(summary.get("manifest_uncovered_span_count", 0))
    if uncovered_count:
        print(
            f"- manifest-uncovered spans={uncovered_count} "
            f"bytes={summary.get('manifest_uncovered_bytes', 0)}"
        )
        for item in tuple(summary.get("manifest_uncovered_spans", ()))[:limit]:
            classification = item.get("classification") or "-"
            print(
                f"- uncovered {item.get('range', '?')} "
                f"size={item.get('size', 0)} backing={item.get('backing', '?')} "
                f"name={item.get('name', '')} classification={classification} "
                f"manifest_matches={item.get('manifest_match_count', 0)}"
            )
        truncated = int(summary.get("manifest_uncovered_spans_truncated", 0))
        if truncated:
            print(f"- ... {truncated} more manifest-uncovered spans")
    for group in groups[:limit]:
        source = group.get("source_from") or group.get("source_filename") or "-"
        names = ", ".join(str(name) for name in tuple(group.get("names", ()))[:limit]) or "-"
        print(
            f"- source={source} target={group.get('target') or '-'} "
            f"items={group.get('item_count', 0)} item_bytes={group.get('item_bytes', 0)} "
            f"matched_overlap_bytes={group.get('matched_overlap_bytes', 0)} names={names}"
        )
    if len(groups) > limit:
        print(f"- ... {len(groups) - limit} more source groups")
    unmatched_count = int(summary.get("unmatched_item_count", 0))
    if unmatched_count:
        print(f"- unmatched items={unmatched_count} bytes={summary.get('unmatched_bytes', 0)}")
        for item in tuple(summary.get("unmatched_items", ()))[:limit]:
            classification = item.get("classification") or "-"
            print(
                f"- unmatched {item.get('range', item.get('address', '?'))} "
                f"size={item.get('size', 0)} backing={item.get('backing', '?')} "
                f"name={item.get('name', '')} classification={classification}"
            )
        truncated = int(summary.get("unmatched_items_truncated", 0))
        if truncated:
            print(f"- ... {truncated} more unmatched items")


def print_tail_byte_summary(label: str, summary: dict[str, Any], *, limit: int) -> None:
    if not summary:
        return
    print(f"{label}:")
    if not summary.get("available"):
        limitations = tuple(summary.get("limitations", ()))
        reason = limitations[-1] if limitations else "unavailable"
        print(
            f"- unavailable backing={summary.get('backing', '-')} "
            f"file_backed_bytes={summary.get('file_backed_byte_count', 0)} reason={reason}"
        )
        return
    print(
        f"- file_backed_bytes={summary.get('file_backed_byte_count', 0)} "
        f"nonzero={summary.get('nonzero_byte_count', 0)} "
        f"zero={summary.get('zero_byte_count', 0)} "
        f"first_nonzero={summary.get('first_nonzero_address') or '-'} "
        f"last_nonzero={summary.get('last_nonzero_address') or '-'}"
    )
    runs = tuple(summary.get("printable_runs", ()))
    for run in runs[:limit]:
        print(
            f"- printable {run.get('start_address', '?')}..{run.get('end_address', '?')} "
            f"size={run.get('size', 0)} text={run.get('text', '')!r}"
        )
    truncated = int(summary.get("printable_runs_truncated", 0))
    if truncated:
        print(f"- ... {truncated} more printable runs")


def print_virtual_tail_attribution(attribution: dict[str, Any], *, limit: int) -> None:
    if not attribution.get("available"):
        return
    reference_window = attribution.get("reference_tail_window", {})
    candidate_window = attribution.get("candidate_corresponding_window", {})
    print("virtual_tail_attribution:")
    print(
        f"- mode={attribution.get('mode', '')} "
        f"virtual_size_delta={attribution.get('virtual_size_delta_text', signed_hex(int(attribution.get('virtual_size_delta', 0))))} "
        f"reference_tail={reference_window.get('start', '?')}..{reference_window.get('end', '?')} "
        f"reference_backing={reference_window.get('backing', '?')} "
        f"candidate_window={candidate_window.get('start', '?')}..{candidate_window.get('end', '?')} "
        f"candidate_backing={candidate_window.get('backing', '?')}"
    )
    print_tail_byte_summary(
        "reference_tail_byte_summary",
        attribution.get("reference_tail_byte_summary", {}),
        limit=limit,
    )
    print_tail_byte_summary(
        "candidate_corresponding_window_byte_summary",
        attribution.get("candidate_corresponding_window_byte_summary", {}),
        limit=limit,
    )
    boundaries = attribution.get("candidate_map_boundaries", {})
    if boundaries:
        data_end = boundaries.get("data_section_end_offset")
        bss_start = boundaries.get("bss_start_offset")
        print(
            f"- candidate_offsets raw_end={hex32(int(boundaries.get('candidate_raw_end_offset', 0)))} "
            f"virtual_end={hex32(int(boundaries.get('candidate_virtual_end_offset', 0)))} "
            f"map_data_end={hex32(int(data_end)) if data_end is not None else '-'} "
            f"bss_start={hex32(int(bss_start)) if bss_start is not None else '-'}"
        )
    print("reference_virtual_tail_bn_items:")
    if not attribution.get("bn_available"):
        print(f"- unavailable: {attribution.get('bn_error', '')}")
    else:
        items = tuple(attribution.get("reference_tail_bn_items", ()))
        print_reference_tail_source_summary(attribution.get("reference_tail_source_summary", {}), limit=limit)
        if not items:
            print("- none")
        for item in items[:limit]:
            classification = item.get("classification") or "-"
            print(
                f"- {item.get('range', item.get('address', '?'))} size={item.get('size', 0)} "
                f"backing={item.get('backing', '?')} name={item.get('name', '')} "
                f"type={item.get('type', '') or '-'} classification={classification}"
            )
            print_tail_manifest_matches(item, limit=limit)
    print("candidate_virtual_boundary_map_symbols:")
    symbols = tuple(attribution.get("candidate_boundary_map_symbols", ()))
    if not symbols:
        print("- none")
    for symbol in symbols[:limit]:
        print(
            f"- {symbol['address']} offset={hex32(int(symbol['offset']))} "
            f"symbol={symbol['symbol']} object={symbol['object'] or '-'} "
            f"relation_to_bss_start={symbol.get('relation_to_bss_start', 'unknown')}"
        )


def write_json(path: Path, report: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def address_range_text(address: str, byte_length: int) -> str:
    if not address:
        return ""
    try:
        start = int(address, 16)
    except ValueError:
        return ""
    end = start + max(byte_length, 0)
    return f"{hex32(start)}..{hex32(end)}"


def direct_owner_issue_record(
    *,
    address: str,
    issue: ManifestIssue,
    entry: Any,
    owner_ids: tuple[str, ...],
    status: str,
) -> DirectOwnerIssue:
    owner_address = entry.address if entry is not None else ""
    owner_target = entry.functional_target if entry is not None else ""
    owner_name = entry.reconstructed_name if entry is not None else ""
    owner_group = entry.entry_group if entry is not None else ""
    owner_tier = entry.reimplementation_tier if entry is not None else ""
    candidate_address = normalize_address(issue.candidate_address) if issue.candidate_address else ""
    return DirectOwnerIssue(
        address=address,
        range=address_range_text(address, issue.byte_length),
        name=owner_name or issue.name,
        symbol=issue.symbol,
        target=owner_target or issue.target,
        manifest=issue.manifest,
        byte_length=issue.byte_length,
        issue_kind=issue.kind,
        reason=issue.detail,
        status=status,
        owner_address=owner_address,
        owner_name=owner_name,
        owner_target=owner_target,
        owner_group=owner_group,
        owner_tier=owner_tier,
        reference_address=address,
        reference_range=address_range_text(address, issue.byte_length),
        candidate_address=candidate_address,
        candidate_range=address_range_text(candidate_address, issue.byte_length),
        owner_ids=owner_ids,
    )


def correlate_owners(
    report: FinalDataReport,
    *,
    progress_path: Path,
    direct_issue_detail_limit: int = FINAL_DATA_OWNER_CORRELATION_DIRECT_ISSUE_LIMIT,
) -> FinalDataOwnerCorrelation:
    if direct_issue_detail_limit < 0:
        raise ValueError("direct issue detail limit must be non-negative")
    entry_index = OwnerEntryIndex.load(progress_path)
    owner_doc = SourceOwnerDocument.load(progress_path)
    issue_by_address: dict[str, ManifestIssue] = {}
    for issue in report.manifest_coverage.issues:
        if issue.kind not in FINAL_DATA_ISSUE_KINDS:
            continue
        address = normalize_address(issue.address)
        issue_by_address.setdefault(address, issue)

    direct_issues: list[DirectOwnerIssue] = []
    direct_s_tier_issue_count = 0
    diagnostic_owner_ids: set[str] = set()
    diagnostic_owner_addresses: set[str] = set()
    affected_owner_ids: set[str] = set()
    for address, issue in sorted(issue_by_address.items(), key=lambda item: int(item[0], 16)):
        entry = entry_index.entries.get(address)
        owner_ids: tuple[str, ...] = ()
        if entry is None:
            status = "untracked-direct-issue"
        elif not entry.is_data_entry:
            status = "non-data-owner-entry-entry"
        else:
            owners = primary_owners_for_entry(owner_doc, entry)
            owner_ids = tuple(sorted(owner.id for owner in owners))
            diagnostic_owner_addresses.add(address)
            diagnostic_owner_ids.update(owner_ids)
            if (
                entry.reimplemented_status == DONE_STATUS
                and entry.reimplementation_tier == TIER_BINARY_SAFE
            ):
                status = "s-tier-byte-gate-affected"
                direct_s_tier_issue_count += 1
                affected_owner_ids.update(owner_ids)
            else:
                status = "diagnostic-data-owner-entry-entry"
        if direct_issue_detail_limit == 0 or len(direct_issues) < direct_issue_detail_limit:
            direct_issues.append(
                direct_owner_issue_record(
                    address=address,
                    issue=issue,
                    entry=entry,
                    owner_ids=owner_ids,
                    status=status,
                )
            )
        if entry is None or not entry.is_data_entry:
            continue

    affected_owner_addresses: set[str] = set()
    for owner_id in affected_owner_ids:
        owner = owner_doc.owner(owner_id)
        for address in owner_data_addresses(owner):
            entry = entry_index.entries.get(address)
            if (
                entry is not None
                and entry.is_data_entry
                and entry.reimplemented_status == DONE_STATUS
                and entry.reimplementation_tier == TIER_BINARY_SAFE
            ):
                affected_owner_addresses.add(address)

    diagnostic_owner_list = tuple(sorted(diagnostic_owner_ids))
    diagnostic_address_list = tuple(sorted(diagnostic_owner_addresses, key=lambda item: int(item, 16)))
    owner_list = tuple(sorted(affected_owner_ids))
    address_list = tuple(sorted(affected_owner_addresses, key=lambda item: int(item, 16)))
    direct_issue_count = len(issue_by_address)
    direct_issue_detail_count = len(direct_issues)
    direct_issues_truncated = direct_issue_detail_count < direct_issue_count
    direct_issue_truncated_count = max(direct_issue_count - direct_issue_detail_count, 0)
    direct_issue_scope_prefix = (
        "all manifest direct issues"
        if direct_issue_detail_limit == 0
        else "bounded sample of manifest direct issues"
    )
    return FinalDataOwnerCorrelation(
        progress=display_path(progress_path, REPO_ROOT),
        direct_issue_count=direct_issue_count,
        direct_issue_detail_limit=direct_issue_detail_limit,
        direct_issue_detail_count=direct_issue_detail_count,
        direct_issue_truncated_count=direct_issue_truncated_count,
        direct_issues_truncated=direct_issues_truncated,
        direct_issues_scope=(
            f"{direct_issue_scope_prefix} sorted by reference address; "
            "correlations are diagnostic observations and produce no gate, tier, work, or scheduler actions"
        ),
        diagnostic_owner_issue_count=len(diagnostic_address_list),
        direct_s_tier_issue_count=direct_s_tier_issue_count,
        diagnostic_owner_ids=diagnostic_owner_list,
        diagnostic_owner_addresses=diagnostic_address_list,
        affected_owner_ids=owner_list,
        affected_owner_addresses=address_list,
        direct_issues=tuple(direct_issues),
        counts={
            "diagnostic_owners": len(diagnostic_owner_list),
            "diagnostic_owner_addresses": len(diagnostic_address_list),
            "affected_owners": len(owner_list),
            "affected_owner_addresses": len(address_list),
        },
        limitations=(
            "Owner correlations are non-authoritative navigation evidence only.",
            "This report never mutates owner gates, entry tiers, work items, blockers, or pipeline state.",
        ),
    )


def print_owner_correlation(correlation: FinalDataOwnerCorrelation, *, limit: int) -> None:
    print("owner_correlation:")
    print(
        f"- direct_issues={correlation.direct_issue_count} "
        f"diagnostic_owner_issues={correlation.diagnostic_owner_issue_count} "
        f"direct_s_tier_issues={correlation.direct_s_tier_issue_count} "
        f"diagnostic_owners={len(correlation.diagnostic_owner_ids)} "
        f"diagnostic_owner_addresses={len(correlation.diagnostic_owner_addresses)} "
        f"affected_owners={len(correlation.affected_owner_ids)} "
        f"affected_owner_addresses={len(correlation.affected_owner_addresses)}"
    )
    print("diagnostic_owners:")
    for owner_id in correlation.diagnostic_owner_ids[:limit]:
        print(f"- {owner_id}")
    if len(correlation.diagnostic_owner_ids) > limit:
        print(f"- ... {len(correlation.diagnostic_owner_ids) - limit} more")
    print("diagnostic_owner_addresses:")
    for address in correlation.diagnostic_owner_addresses[:limit]:
        print(f"- {address}")
    if len(correlation.diagnostic_owner_addresses) > limit:
        print(f"- ... {len(correlation.diagnostic_owner_addresses) - limit} more")
    print("affected_owners:")
    for owner_id in correlation.affected_owner_ids[:limit]:
        print(f"- {owner_id}")
    if len(correlation.affected_owner_ids) > limit:
        print(f"- ... {len(correlation.affected_owner_ids) - limit} more")
    print("affected_owner_addresses:")
    for address in correlation.affected_owner_addresses[:limit]:
        print(f"- {address}")
    if len(correlation.affected_owner_addresses) > limit:
        print(f"- ... {len(correlation.affected_owner_addresses) - limit} more")
    print("- diagnostic only; no owner or progress mutation commands are generated")


def final_data_result(report: FinalDataReport) -> str:
    byte_comparison = report.raw_byte_comparison
    if (
        any(delta.delta != 0 for delta in report.deltas)
        or report.manifest_coverage.issues
        or (byte_comparison.available and not byte_comparison.equal)
    ):
        return "failed"
    if (
        not byte_comparison.available
        or not report.storage_contributions
        or report.manifest_coverage.data_symbol_count == 0
    ):
        return "blocked"
    return "passed"


def build_report_payload(
    report: FinalDataReport,
    args: argparse.Namespace,
    *,
    owner_correlation: FinalDataOwnerCorrelation | None = None,
) -> dict[str, Any]:
    object_paths = parse_link_rsp_objects(Path(args.link_rsp))
    seen_objects: set[str] = set()
    object_artifacts: list[dict[str, Any]] = []
    for path in object_paths:
        key = str(path.resolve()).lower()
        if key in seen_objects:
            continue
        seen_objects.add(key)
        object_artifacts.append(artifact_record(path))
    manifest_artifacts = tuple(
        artifact_record(path)
        for path in sorted(Path(args.manifest_dir).glob("*.json"), key=lambda item: item.as_posix().lower())
    )
    artifacts = {
        "reference": artifact_record(Path(args.reference)),
        "candidate": artifact_record(Path(args.candidate)),
        "map": artifact_record(Path(args.map)),
        "link_response": artifact_record(Path(args.link_rsp)),
        "summary": artifact_record(Path(args.candidate).parent / "summary.json"),
        "manifests": manifest_artifacts,
        "objects": tuple(object_artifacts),
    }
    result = final_data_result(report)
    section_shape_equal = not any(delta.delta != 0 for delta in report.deltas)
    payload = asdict(report)
    payload.update(
        {
            "report_version": 3,
            "kind": "final-data-report",
            "result": result,
            "binary": report.binary,
            "output_section_id": report.output_section_id,
            "artifacts": artifacts,
            "output_section": {
                "id": report.output_section_id,
                "name": report.section,
                "reference": asdict(report.reference_section),
                "candidate": asdict(report.candidate_section),
            },
            "checks": {
                "section_shape_equal": section_shape_equal,
                "raw_bytes_equal": report.raw_byte_comparison.equal,
                "manifest_coverage_complete": bool(
                    report.manifest_coverage.data_symbol_count
                    and report.manifest_coverage.exact_address_matches == report.manifest_coverage.data_symbol_count
                    and not report.manifest_coverage.issues
                ),
                "storage_contributions_present": bool(report.storage_contributions),
            },
            "deltas": tuple(delta_record(delta) for delta in report.deltas),
            "coverage": {
                "manifest": asdict(report.manifest_coverage),
                "binary_ninja": asdict(report.bn_coverage),
            },
            "owner_correlations": asdict(owner_correlation) if owner_correlation is not None else None,
            "limitations": (
                "This report is read-only diagnostic evidence and never mutates unified progress state.",
                "Observed storage contributions and owner correlations do not prove source ownership, data gates, section acceptance, or final acceptance.",
            ),
        }
    )
    return payload


def retired_action_option(argv: list[str]) -> str:
    for token in argv:
        option = token.split("=", 1)[0]
        if option in RETIRED_ACTION_OPTIONS:
            return option
    return ""


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Read-only final-build .data section drift diagnostic."
    )
    parser.add_argument("--binary", choices=reference_image_keys(), default="recoil")
    parser.add_argument("--reference", default=str(DEFAULT_REFERENCE))
    parser.add_argument("--candidate", default=str(DEFAULT_CANDIDATE))
    parser.add_argument("--map", default=str(DEFAULT_MAP))
    parser.add_argument("--link-rsp", default=str(DEFAULT_LINK_RSP))
    parser.add_argument("--manifest-dir", default=str(DEFAULT_MANIFEST_DIR))
    parser.add_argument("--section", default=".data")
    parser.add_argument("--limit", type=int, default=20)
    parser.add_argument("--json-out", default="")
    parser.add_argument(
        "--trace-object",
        action="append",
        default=[],
        help=(
            "Emit a focused read-only COFF/map/link-order trace for a named object; "
            "may be repeated or comma-separated, e.g. --trace-object player.obj,ainet.obj."
        ),
    )
    parser.add_argument("--include-owners", action="store_true", help="Include non-authoritative correlations with current unified-progress data rows.")
    parser.add_argument("--progress", type=Path, default=DEFAULT_PROGRESS, help="Unified progress tracker used only for optional diagnostic owner correlations.")
    parser.add_argument("--strict", action="store_true", help="Return nonzero when section deltas are present.")
    parser.add_argument("--bridge-url", default=DEFAULT_BRIDGE_URL)
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    effective_argv = list(sys.argv[1:] if argv is None else argv)
    retired = retired_action_option(effective_argv)
    if retired:
        print(
            f"final-data audit failed: {retired} is retired; use the read-only live "
            "--json-out final-data report instead",
            file=sys.stderr,
        )
        return 2
    parser = build_parser()
    args = parser.parse_args(effective_argv)
    if args.limit <= 0:
        print("final-data audit failed: --limit must be positive", file=sys.stderr)
        return 2
    try:
        report = build_report(args)
        correlation = (
            correlate_owners(report, progress_path=args.progress)
            if args.include_owners
            else None
        )
        payload = build_report_payload(report, args, owner_correlation=correlation)
    except (OSError, ValueError) as exc:
        print(f"final-data audit failed: {exc}", file=sys.stderr)
        return 2
    print_report(report, limit=args.limit)
    if args.json_out:
        write_json(Path(args.json_out), payload)
        print(f"json_out={args.json_out}")
    if correlation is not None:
        print_owner_correlation(correlation, limit=args.limit)
    return 1 if args.strict and payload["result"] != "passed" else 0


if __name__ == "__main__":
    raise SystemExit(main())
