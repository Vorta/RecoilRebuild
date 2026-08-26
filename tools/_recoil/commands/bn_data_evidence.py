#!/usr/bin/env python3
"""Read-only Binary Ninja evidence probe for data-owner recovery."""

from __future__ import annotations

import argparse
from dataclasses import asdict, dataclass
import json
import math
from pathlib import Path
import struct
import sys
from typing import Any

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from _recoil.commands.binja_preflight import (
    DataItem,
    bridge_collection,
    data_item_label,
    fetch_data_items,
    normalize_address_text,
    parse_bridge_int,
)
from _recoil.lib.binja import DEFAULT_BRIDGE_URL, BinaryNinjaBridge, BridgeError, Symbol
from _recoil.lib.reference_images import reference_image, reference_image_keys
from _recoil.lib.tooling import configure_stdio


XREF_ENDPOINTS = ("getXrefsTo", "get_xrefs_to", "xrefs", "references", "dataReferences", "codeReferences")


@dataclass(frozen=True)
class XrefHit:
    query_address: str
    query_offset: int
    source_address: str
    source_name: str
    kind: str
    text: str

    def as_dict(self) -> dict[str, object]:
        return asdict(self)


@dataclass(frozen=True)
class AssemblyHit:
    function_address: str
    function_name: str
    matched: tuple[dict[str, object], ...]

    def as_dict(self) -> dict[str, object]:
        return {
            "function_address": self.function_address,
            "function_name": self.function_name,
            "matched": list(self.matched),
        }


@dataclass(frozen=True)
class DerivedConstant:
    offset: int
    address: str
    kind: str
    value: str
    raw_dword: str

    def as_dict(self) -> dict[str, object]:
        return asdict(self)


@dataclass(frozen=True)
class BnDataEvidenceResult:
    binary: str
    address: str
    size: int
    range_end: str
    data: dict[str, object]
    adjacent_data: tuple[dict[str, object], ...]
    hexdump: str
    direct_xrefs: dict[str, object]
    assembly_address_scan: dict[str, object]
    derived_constants: tuple[dict[str, object], ...]
    global_constant_search: dict[str, object]
    relocations: dict[str, object]
    limitations: tuple[str, ...]

    def as_dict(self) -> dict[str, object]:
        return {
            "binary": self.binary,
            "address": self.address,
            "size": self.size,
            "range_end": self.range_end,
            "data": self.data,
            "adjacent_data": list(self.adjacent_data),
            "hexdump": self.hexdump,
            "direct_xrefs": self.direct_xrefs,
            "assembly_address_scan": self.assembly_address_scan,
            "derived_constants": list(self.derived_constants),
            "global_constant_search": self.global_constant_search,
            "relocations": self.relocations,
            "limitations": list(self.limitations),
        }


def data_item_dict(item: DataItem | None) -> dict[str, object]:
    if item is None:
        return {"present": False}
    return {
        "present": True,
        "address": item.address,
        "name": item.name,
        "size": item.size,
        "section": item.section,
        "type": item.data_type,
        "end": f"0x{item.end:x}",
    }


def find_exact_and_enclosing_data(
    items: list[DataItem],
    address: str,
) -> tuple[DataItem | None, DataItem | None]:
    value = int(normalize_address_text(address), 16)
    exact = next((item for item in items if item.start == value), None)
    enclosing = [item for item in items if item.start <= value < item.end]
    if not enclosing:
        return exact, None
    return exact, min(enclosing, key=lambda item: item.size)


def adjacent_data_items(
    items: list[DataItem],
    *,
    start: int,
    end: int,
    nearby: int,
) -> tuple[dict[str, object], ...]:
    low = start - nearby
    high = end + nearby
    adjacent = [
        item
        for item in items
        if item.end >= low and item.start <= high and not (item.start == start and item.end == end)
    ]
    return tuple(data_item_dict(item) for item in sorted(adjacent, key=lambda item: (item.start, item.size, item.name)))


def aligned_range_addresses(start: int, size: int, *, stride: int = 4) -> tuple[str, ...]:
    if size <= 0:
        return ()
    values = [start]
    aligned = start if start % stride == 0 else start + (stride - (start % stride))
    current = aligned
    end = start + size
    while current < end:
        if current != start:
            values.append(current)
        current += stride
    return tuple(f"0x{value:x}" for value in values)


def _first_text(item: dict[str, Any], *keys: str) -> str:
    for key in keys:
        value = item.get(key)
        if value not in {None, ""}:
            return str(value)
    return ""


def _first_address_text(item: dict[str, Any], *keys: str) -> object:
    for key in keys:
        value = item.get(key)
        if value is None or value == "":
            continue
        if isinstance(value, dict):
            nested = _first_address_text(
                value,
                "address",
                "addr",
                "start",
                "source",
                "source_address",
                "source_addr",
                "from",
                "from_address",
                "from_addr",
            )
            if nested is not None and nested != "":
                return nested
            continue
        return value
    return ""


def normalize_xref_item(query_address: str, base: int, item: dict[str, Any]) -> XrefHit:
    raw_source = _first_address_text(
        item,
        "source",
        "source_address",
        "source_addr",
        "from",
        "from_address",
        "from_addr",
        "address",
        "addr",
        "ref_addr",
        "reference",
        "function",
        "function_address",
        "function_start",
        "source_function",
    )
    try:
        source_address = normalize_address_text(raw_source)
    except (TypeError, ValueError):
        source_address = str(raw_source)
    query_value = int(normalize_address_text(query_address), 16)
    return XrefHit(
        query_address=normalize_address_text(query_address),
        query_offset=query_value - base,
        source_address=source_address,
        source_name=_first_text(item, "source_name", "source_function_name", "function_name", "name", "symbol"),
        kind=_first_text(item, "kind", "type", "xref_type"),
        text=_first_text(item, "text", "line", "disassembly", "instruction", "instruction_text"),
    )


def _xref_items_from_payload(payload: Any) -> list[dict[str, Any]]:
    if isinstance(payload, list):
        return [item for item in payload if isinstance(item, dict)]
    if not isinstance(payload, dict):
        return []
    combined: list[dict[str, Any]] = []
    for key, kind in (("code_references", "code"), ("data_references", "data")):
        value = payload.get(key)
        if not isinstance(value, list):
            continue
        for item in value:
            if not isinstance(item, dict):
                continue
            cloned = dict(item)
            cloned.setdefault("kind", kind)
            combined.append(cloned)
    if combined:
        return combined
    return bridge_collection(payload, "xrefs", "references", "refs", "items", "data", "results")


def _payload_has_xref_collection(payload: Any) -> bool:
    if isinstance(payload, list):
        return True
    if not isinstance(payload, dict):
        return False
    return any(
        name in payload
        for name in ("xrefs", "references", "refs", "items", "data", "results", "code_references", "data_references")
    )


def fetch_direct_xrefs(
    bridge: BinaryNinjaBridge,
    targets: tuple[str, ...],
    *,
    base: int,
    limit: int,
) -> dict[str, object]:
    if not targets:
        return {"status": "not_requested", "hits": []}

    endpoint_errors: list[str] = []
    selected_endpoint = ""
    first_items: list[dict[str, Any]] = []
    for endpoint in XREF_ENDPOINTS:
        try:
            payload = bridge.get_json(endpoint, address=targets[0], limit=limit)
        except BridgeError as exc:
            endpoint_errors.append(f"{endpoint}: {exc}")
            continue
        items = _xref_items_from_payload(payload)
        if items or _payload_has_xref_collection(payload):
            selected_endpoint = endpoint
            first_items = items
            break
        endpoint_errors.append(f"{endpoint}: response contained no recognized xref collection")

    if not selected_endpoint:
        return {
            "status": "unsupported",
            "reason": "no recognized read-only xref endpoint exposed by the local Binary Ninja bridge",
            "attempted_endpoints": list(XREF_ENDPOINTS),
            "errors": endpoint_errors,
            "hits": [],
        }

    hits = [normalize_xref_item(targets[0], base, item).as_dict() for item in first_items]
    for target in targets[1:]:
        if len(hits) >= limit:
            break
        try:
            payload = bridge.get_json(selected_endpoint, address=target, limit=limit)
        except BridgeError as exc:
            return {
                "status": "partial",
                "endpoint": selected_endpoint,
                "reason": str(exc),
                "hits": hits,
            }
        for item in _xref_items_from_payload(payload):
            if len(hits) >= limit:
                break
            hits.append(normalize_xref_item(target, base, item).as_dict())

    return {
        "status": "supported",
        "endpoint": selected_endpoint,
        "queried_addresses": list(targets),
        "hits": hits,
        "truncated": len(hits) >= limit,
    }


def derive_constants(
    data: bytes,
    *,
    start: int,
    mode: str,
    limit: int,
) -> tuple[DerivedConstant, ...]:
    if mode == "none":
        return ()
    constants: list[DerivedConstant] = []
    for offset in range(0, max(0, len(data) - 3), 4):
        raw = data[offset : offset + 4]
        raw_dword = struct.unpack("<I", raw)[0]
        if mode in {"dword", "all"}:
            constants.append(
                DerivedConstant(
                    offset=offset,
                    address=f"0x{start + offset:x}",
                    kind="dword",
                    value=f"0x{raw_dword:08x}",
                    raw_dword=f"0x{raw_dword:08x}",
                )
            )
        if mode in {"float", "all"}:
            value = struct.unpack("<f", raw)[0]
            if math.isfinite(value):
                constants.append(
                    DerivedConstant(
                        offset=offset,
                        address=f"0x{start + offset:x}",
                        kind="float",
                        value=f"{value:.9g}",
                        raw_dword=f"0x{raw_dword:08x}",
                    )
                )
        if len(constants) >= limit:
            return tuple(constants)
    return tuple(constants)


def _symbol_name(symbol: Symbol) -> str:
    return symbol.name or symbol.raw_name or symbol.full_name


def _assembly_targets(
    address_targets: tuple[str, ...],
    constants: tuple[DerivedConstant, ...],
) -> dict[str, dict[str, object]]:
    targets: dict[str, dict[str, object]] = {}
    for address in address_targets:
        normalized = normalize_address_text(address)
        value = int(normalized, 16)
        targets[f"0x{value:x}"] = {"kind": "address", "value": normalized}
    for constant in constants:
        if constant.raw_dword.lower() == "0x00000000":
            continue
        raw_value = int(constant.raw_dword, 16)
        targets[f"0x{raw_value:x}"] = {
            "kind": f"constant-{constant.kind}",
            "value": constant.value,
            "source_address": constant.address,
            "raw_dword": constant.raw_dword,
        }
    return targets


def scan_assembly_text(
    bridge: BinaryNinjaBridge,
    *,
    address_targets: tuple[str, ...],
    constants: tuple[DerivedConstant, ...],
    max_functions: int,
    hit_limit: int,
) -> dict[str, object]:
    try:
        symbols_by_address, _symbols_by_name = bridge.symbols()
    except BridgeError as exc:
        return {
            "status": "unsupported",
            "reason": f"function inventory unavailable: {exc}",
            "hits": [],
        }

    targets = _assembly_targets(address_targets, constants)
    if not targets:
        return {"status": "not_requested", "hits": []}

    hits: list[AssemblyHit] = []
    checked = 0
    functions = sorted(
        (symbol for symbol in symbols_by_address.values() if symbol.kind == "function"),
        key=lambda symbol: int(normalize_address_text(symbol.address), 16),
    )
    for symbol in functions[:max_functions]:
        try:
            assembly = bridge.assembly(symbol.address)
        except BridgeError:
            continue
        checked += 1
        lowered = assembly.lower()
        matched = []
        for needle, detail in targets.items():
            if needle.lower() in lowered:
                matched.append(detail)
        if matched:
            hits.append(
                AssemblyHit(
                    function_address=normalize_address_text(symbol.address),
                    function_name=_symbol_name(symbol),
                    matched=tuple(matched),
                )
            )
        if len(hits) >= hit_limit:
            break

    status = "partial" if len(functions) > max_functions or len(hits) >= hit_limit else "supported"
    return {
        "status": status,
        "method": "assembly_text_scan",
        "functions_checked": checked,
        "functions_available": len(functions),
        "max_functions": max_functions,
        "hits": [hit.as_dict() for hit in hits],
        "limitations": [
            "text scanning is a fallback diagnostic; absence of a hit is not proof of no reference",
            "only rendered assembly text exposed by the bridge is searched",
        ],
    }


def split_constant_hits(scan: dict[str, object]) -> tuple[dict[str, object], dict[str, object]]:
    hits = scan.get("hits")
    if not isinstance(hits, list):
        return scan, {"status": "unsupported", "reason": "assembly scan did not return structured hits", "hits": []}

    address_hits = []
    constant_hits = []
    for hit in hits:
        if not isinstance(hit, dict):
            continue
        matched = hit.get("matched")
        if not isinstance(matched, list):
            continue
        address_matched = [item for item in matched if isinstance(item, dict) and item.get("kind") == "address"]
        constant_matched = [item for item in matched if isinstance(item, dict) and str(item.get("kind", "")).startswith("constant-")]
        if address_matched:
            cloned = dict(hit)
            cloned["matched"] = address_matched
            address_hits.append(cloned)
        if constant_matched:
            cloned = dict(hit)
            cloned["matched"] = constant_matched
            constant_hits.append(cloned)

    address_scan = dict(scan)
    address_scan["hits"] = address_hits
    constant_scan = dict(scan)
    constant_scan["hits"] = constant_hits
    if not constant_hits and scan.get("status") == "not_requested":
        constant_scan = {"status": "not_requested", "hits": []}
    return address_scan, constant_scan


def collect_bn_data_evidence(
    *,
    bridge: BinaryNinjaBridge,
    binary: str,
    address: str,
    size: int,
    nearby: int,
    constants: str,
    xref_limit: int,
    constant_limit: int,
    max_assembly_functions: int,
    assembly_hit_limit: int,
) -> BnDataEvidenceResult:
    normalized = normalize_address_text(address)
    start = int(normalized, 16)
    end = start + size
    items = fetch_data_items(bridge)
    exact, enclosing = find_exact_and_enclosing_data(items, normalized)
    hexdump = bridge.hexdump(normalized, size)
    try:
        hexdump_bytes = bytes_from_hexdump(hexdump, expected_length=size)
    except ValueError:
        hexdump_bytes = b""

    address_targets = aligned_range_addresses(start, size)
    direct_xrefs = fetch_direct_xrefs(bridge, address_targets, base=start, limit=xref_limit)
    derived = tuple(item.as_dict() for item in derive_constants(hexdump_bytes, start=start, mode=constants, limit=constant_limit))
    derived_objects = tuple(
        DerivedConstant(
            offset=int(item["offset"]),
            address=str(item["address"]),
            kind=str(item["kind"]),
            value=str(item["value"]),
            raw_dword=str(item["raw_dword"]),
        )
        for item in derived
    )
    scan = scan_assembly_text(
        bridge,
        address_targets=address_targets,
        constants=derived_objects,
        max_functions=max_assembly_functions,
        hit_limit=assembly_hit_limit,
    )
    address_scan, constant_scan = split_constant_hits(scan)
    if constants == "none":
        constant_search = {"status": "not_requested", "hits": []}
    elif constant_scan.get("hits") or constant_scan.get("status") in {"supported", "partial"}:
        constant_search = constant_scan
    else:
        constant_search = {
            "status": "unsupported",
            "reason": "no bridge memory/constant-search endpoint is available; assembly text fallback did not run",
            "hits": [],
        }

    limitations = [
        "This command is read-only diagnostic evidence only; it does not satisfy source-owner or data gates by itself.",
        "Assembly text fallback can miss references hidden by analysis, formatting, register math, or non-immediate addressing.",
    ]
    if direct_xrefs.get("status") == "unsupported":
        limitations.append("Direct xref enumeration is unavailable through the current local BN bridge.")
    if hexdump_bytes == b"":
        limitations.append("BN hexdump could not be parsed to derive constants.")

    return BnDataEvidenceResult(
        binary=binary,
        address=normalized,
        size=size,
        range_end=f"0x{end:x}",
        data={"exact": data_item_dict(exact), "enclosing": data_item_dict(enclosing)},
        adjacent_data=adjacent_data_items(items, start=start, end=end, nearby=nearby),
        hexdump=hexdump,
        direct_xrefs=direct_xrefs,
        assembly_address_scan=address_scan,
        derived_constants=derived,
        global_constant_search=constant_search,
        relocations={
            "status": "unsupported",
            "reason": "the local BN bridge exposes no read-only relocation enumeration endpoint",
            "hits": [],
        },
        limitations=tuple(limitations),
    )


def bytes_from_hexdump(hexdump_text: str, *, expected_length: int | None = None) -> bytes:
    values: list[int] = []
    for line in hexdump_text.splitlines():
        parts = line.strip().split()
        if not parts:
            continue
        for part in parts[1:]:
            if len(part) != 2:
                break
            try:
                values.append(int(part, 16))
            except ValueError:
                break
    data = bytes(values)
    if expected_length is not None and len(data) != expected_length:
        raise ValueError(f"BN hexdump yielded {len(data)} byte(s), expected {expected_length}")
    return data


def print_human_result(result: BnDataEvidenceResult) -> None:
    print(f"bn-data-evidence binary={result.binary} address={result.address} size=0x{result.size:x} end={result.range_end}")
    exact = result.data["exact"]
    enclosing = result.data["enclosing"]
    print(f"- data exact: {exact if exact.get('present') else 'not exposed'}")
    print(f"- data enclosing: {enclosing if enclosing.get('present') else 'not exposed'}")
    print(f"- adjacent data items: {len(result.adjacent_data)}")
    for item in result.adjacent_data[:12]:
        print(f"  - {item}")
    print("- hexdump:")
    for line in result.hexdump.splitlines():
        print(f"  {line}")
    print(f"- direct xrefs: {result.direct_xrefs.get('status')}")
    for hit in result.direct_xrefs.get("hits", [])[:12]:
        print(f"  - {hit}")
    print(f"- assembly address scan: {result.assembly_address_scan.get('status')}")
    for hit in result.assembly_address_scan.get("hits", [])[:12]:
        print(f"  - {hit}")
    print(f"- derived constants: {len(result.derived_constants)}")
    for item in result.derived_constants[:12]:
        print(f"  - {item}")
    print(f"- global constant search: {result.global_constant_search.get('status')}")
    for hit in result.global_constant_search.get("hits", [])[:12]:
        print(f"  - {hit}")
    print(f"- relocations: {result.relocations.get('status')} ({result.relocations.get('reason')})")
    print("- limitations:")
    for limitation in result.limitations:
        print(f"  - {limitation}")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Read-only Binary Ninja data evidence probe for source-owner recovery. "
            "Reports BN data declarations, hexdump bytes, nearby data variables, xref support where exposed, "
            "and explicit unsupported fields for unavailable relocation/global-search capabilities."
        )
    )
    parser.add_argument("address", help="Data address to inspect, e.g. 0x4e5954.")
    parser.add_argument("--size", type=lambda text: int(text, 0), required=True, help="Byte size of the data range.")
    parser.add_argument("--nearby", type=lambda text: int(text, 0), default=0x40, help="Adjacent data window.")
    parser.add_argument("--constants", choices=("none", "dword", "float", "all"), default="none")
    parser.add_argument("--binary", choices=reference_image_keys(), default="recoil")
    parser.add_argument("--bridge-url", default=DEFAULT_BRIDGE_URL)
    parser.add_argument("--xrefs-limit", type=int, default=120)
    parser.add_argument("--constant-limit", type=int, default=80)
    parser.add_argument("--max-assembly-functions", type=int, default=120)
    parser.add_argument("--assembly-hit-limit", type=int, default=80)
    parser.add_argument("--json", action="store_true", help="Emit structured JSON.")
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    image = reference_image(args.binary)
    bridge = BinaryNinjaBridge(args.bridge_url, timeout=10.0, binary=Path(image.bndb_path).name)
    try:
        result = collect_bn_data_evidence(
            bridge=bridge,
            binary=args.binary,
            address=args.address,
            size=args.size,
            nearby=args.nearby,
            constants=args.constants,
            xref_limit=max(1, args.xrefs_limit),
            constant_limit=max(1, args.constant_limit),
            max_assembly_functions=max(1, args.max_assembly_functions),
            assembly_hit_limit=max(1, args.assembly_hit_limit),
        )
    except (BridgeError, RuntimeError, ValueError) as exc:
        print(f"bn-data-evidence failed: {exc}", file=sys.stderr)
        return 2

    if args.json:
        print(json.dumps(result.as_dict(), indent=2, sort_keys=True))
    else:
        print_human_result(result)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
