#!/usr/bin/env python3
"""Check that the expected Binary Ninja database is open through the bridge."""

from __future__ import annotations

import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from _recoil.lib.binja import DEFAULT_BRIDGE_URL, BinaryNinjaBridge, BridgeError
from _recoil.lib.reference_images import reference_image, reference_image_keys
from _recoil.lib.tooling import configure_stdio


DEFAULT_EXPECTED_FILE = "D:/Recoil Project/Decomp/Recoil.bndb"
DEFAULT_EXPECTED_PLATFORM = "windows-x86"
DEFAULT_EXPECTED_ARCH = "x86"
DEFAULT_PROBE_ADDRESS = "0x401000"


@dataclass(frozen=True)
class PreflightResult:
    ok: bool
    messages: tuple[str, ...]


@dataclass(frozen=True)
class DataItem:
    address: str
    start: int
    size: int
    name: str
    data_type: str
    section: str

    @property
    def end(self) -> int:
        return self.start + self.size


@dataclass(frozen=True)
class DataOverlapFinding:
    inner: DataItem
    outer: DataItem
    offset: int


@dataclass(frozen=True)
class DataOverlapAuditResult:
    ok: bool
    messages: tuple[str, ...]
    item_count: int
    finding_count: int


def normalize_path_text(value: str) -> str:
    return str(Path(value.replace("\\", "/"))).replace("\\", "/").lower()


def normalize_address_text(value: object) -> str:
    text = str(value).strip()
    if text.lower().startswith("0x"):
        return f"0x{int(text, 16):x}"
    return f"0x{int(text, 16):x}"


def parse_bridge_int(value: object, default: int = 0) -> int:
    if value is None or value == "":
        return default
    if isinstance(value, bool):
        return default
    if isinstance(value, int):
        return value
    text = str(value).strip()
    try:
        return int(text, 0)
    except ValueError:
        try:
            return int(text, 16)
        except ValueError:
            return default


def bridge_text(item: dict[str, Any], *keys: str) -> str:
    for key in keys:
        value = item.get(key)
        if value not in {None, ""}:
            return str(value).strip()
    return ""


def bridge_collection(payload: dict[str, Any], *names: str) -> list[dict[str, Any]]:
    for name in names:
        value = payload.get(name)
        if isinstance(value, list):
            return [item for item in value if isinstance(item, dict)]
    for value in payload.values():
        if isinstance(value, list):
            return [item for item in value if isinstance(item, dict)]
    return []


def fetch_json(bridge: BinaryNinjaBridge, endpoint: str, **params: object) -> dict[str, Any]:
    try:
        data = bridge.get_json(endpoint, **params)
    except BridgeError as exc:
        raise RuntimeError(str(exc)) from exc
    if not isinstance(data, dict):
        raise RuntimeError(f"Binary Ninja bridge returned non-object JSON: {endpoint}")
    return data


def validate_status(
    status: dict[str, Any],
    *,
    expected_file: str,
    expected_platform: str,
    expected_arch: str,
) -> list[str]:
    errors: list[str] = []
    if status.get("loaded") is not True:
        errors.append("bridge status reports no loaded Binary Ninja database")

    filename = str(status.get("filename", ""))
    if normalize_path_text(filename) != normalize_path_text(expected_file):
        errors.append(f"loaded database is {filename or '<missing>'}; expected {expected_file}")

    platform = str(status.get("platform", ""))
    if platform != expected_platform:
        errors.append(f"platform is {platform or '<missing>'}; expected {expected_platform}")

    arch = str(status.get("arch", ""))
    if arch != expected_arch:
        errors.append(f"arch is {arch or '<missing>'}; expected {expected_arch}")

    try:
        open_binaries = int(status.get("open_binaries", 0))
    except (TypeError, ValueError):
        open_binaries = 0
    if open_binaries < 1:
        errors.append("bridge reports no open binaries")

    return errors


def validate_binaries(
    binaries: dict[str, Any],
    *,
    expected_file: str,
    require_active: bool = True,
) -> list[str]:
    items = binaries.get("binaries")
    if not isinstance(items, list):
        items = binaries.get("items")
    if not isinstance(items, list) or not items:
        return ["bridge /binaries returned no open binaries"]

    expected = normalize_path_text(expected_file)
    active_matches = []
    matches = []
    for item in items:
        if not isinstance(item, dict):
            continue
        filename = str(item.get("filename", ""))
        if normalize_path_text(filename) == expected:
            matches.append(item)
            if item.get("active") is True:
                active_matches.append(item)

    if not matches:
        return [f"expected database is not open in Binary Ninja: {expected_file}"]
    if require_active and not active_matches:
        return [
            f"expected database is open but not active in Binary Ninja: {expected_file}",
            "Activate that Binary Ninja view before running address-led checks for this binary.",
        ]
    return []


def validate_probe(function_info: dict[str, Any], *, probe_address: str) -> list[str]:
    function = function_info.get("function")
    if not isinstance(function, dict):
        return [f"function probe {probe_address} did not return a function object"]
    address = str(function.get("address", "")).lower()
    if address != probe_address.lower():
        return [f"function probe returned {address or '<missing>'}; expected {probe_address}"]
    if not function.get("name"):
        return [f"function probe {probe_address} returned no name"]
    return []


def normalize_data_item(item: dict[str, Any]) -> DataItem | None:
    raw_address = item.get("address", item.get("start", item.get("addr")))
    if raw_address in {None, ""}:
        return None
    try:
        address = normalize_address_text(raw_address)
    except ValueError:
        return None
    start = int(address, 16)
    size = parse_bridge_int(item.get("size", item.get("length")), default=0)
    if size <= 0:
        return None
    return DataItem(
        address=address,
        start=start,
        size=size,
        name=bridge_text(item, "name", "symbol", "full_name", "raw_name") or "<unnamed>",
        data_type=bridge_text(item, "type", "decl", "data_type"),
        section=bridge_text(item, "section", "section_name"),
    )


def fetch_data_items(bridge: BinaryNinjaBridge, *, limit: int = 100000, max_pages: int = 8) -> list[DataItem]:
    offset = 0
    items: list[DataItem] = []
    seen_offsets: set[int] = set()
    for _page_index in range(max_pages):
        payload = fetch_json(bridge, "data", offset=offset, limit=limit)
        page = bridge_collection(payload, "items", "data", "data_items")
        for raw_item in page:
            item = normalize_data_item(raw_item)
            if item is not None:
                items.append(item)
        total = parse_bridge_int(payload.get("total"), default=offset + len(page))
        if not page or offset + len(page) >= total:
            return items
        seen_offsets.add(offset)
        next_offset = offset + max(len(page), parse_bridge_int(payload.get("limit"), default=limit))
        if next_offset in seen_offsets or next_offset <= offset:
            break
        offset = next_offset
    raise RuntimeError(
        f"Binary Ninja bridge 'data' pagination exceeded {max_pages} page(s); "
        "data-overlap audit would be incomplete"
    )


def address_relevant_to_probe(inner: DataItem, outer: DataItem, probes: tuple[int, ...]) -> bool:
    if not probes:
        return True
    return any(
        probe == inner.start
        or inner.start <= probe < inner.end
        or outer.start <= probe < outer.end
        for probe in probes
    )


def collect_data_overlap_findings(
    items: list[DataItem],
    *,
    probe_addresses: tuple[str, ...] = (),
) -> list[DataOverlapFinding]:
    probes = tuple(int(normalize_address_text(address), 16) for address in probe_addresses)
    findings: list[DataOverlapFinding] = []
    sorted_items = sorted(items, key=lambda item: (item.start, item.size, item.name))
    for inner in sorted_items:
        for outer in sorted_items:
            if inner is outer or outer.size <= 0:
                continue
            if outer.start < inner.start < outer.end and address_relevant_to_probe(inner, outer, probes):
                findings.append(DataOverlapFinding(inner=inner, outer=outer, offset=inner.start - outer.start))
    return findings


def data_item_label(item: DataItem) -> str:
    section = f" section={item.section}" if item.section else ""
    data_type = f" type={item.data_type}" if item.data_type else ""
    return f"{item.address} {item.name} size={item.size}{section}{data_type}"


def audit_data_overlaps(
    items: list[DataItem],
    *,
    probe_addresses: tuple[str, ...] = (),
    max_findings: int = 40,
) -> DataOverlapAuditResult:
    if not items:
        return DataOverlapAuditResult(
            ok=False,
            messages=(
                "Binary Ninja data overlap audit could not inspect data variables: bridge 'data' returned no usable sized items.",
                "This diagnostic is report-only; it does not repair BN data variables or provide data gate acceptance.",
            ),
            item_count=0,
            finding_count=0,
        )

    findings = collect_data_overlap_findings(items, probe_addresses=probe_addresses)
    messages: list[str] = []
    probe_text = " ".join(probe_addresses) if probe_addresses else "all data items"
    if not findings:
        messages.append(
            f"Binary Ninja data overlap audit OK for {probe_text}: "
            f"no interior-root data variables found among {len(items)} sized item(s)."
        )
        messages.append(
            "This diagnostic is report-only; source-owner and data gates still require current BN/source evidence review."
        )
        return DataOverlapAuditResult(True, tuple(messages), len(items), 0)

    messages.append(
        f"Binary Ninja data overlap audit found {len(findings)} interior-root overlap(s) for {probe_text} "
        f"among {len(items)} sized item(s)."
    )
    for finding in findings[:max_findings]:
        messages.append(
            "- "
            f"{data_item_label(finding.inner)} starts inside {data_item_label(finding.outer)} "
            f"at +0x{finding.offset:x}; treat the inner address as ambiguous BN/bridge data-var evidence."
        )
    if len(findings) > max_findings:
        messages.append(f"- ... {len(findings) - max_findings} additional overlap(s) omitted.")
    messages.append(
        "This diagnostic is report-only; do not accept data gates from ambiguous interior-root facts until BN/bridge data variables are repaired or independently resolved."
    )
    return DataOverlapAuditResult(False, tuple(messages), len(items), len(findings))


def run_data_overlap_audit(
    *,
    bridge_url: str,
    probe_addresses: tuple[str, ...],
    max_findings: int,
) -> DataOverlapAuditResult:
    bridge = BinaryNinjaBridge(bridge_url, timeout=10.0)
    items = fetch_data_items(bridge)
    return audit_data_overlaps(items, probe_addresses=probe_addresses, max_findings=max_findings)


def run_preflight(
    *,
    bridge_url: str,
    expected_file: str,
    expected_platform: str,
    expected_arch: str,
    probe_address: str,
    binary_selector: str | None = None,
) -> PreflightResult:
    messages: list[str] = []
    try:
        bridge = BinaryNinjaBridge(bridge_url, timeout=10.0, binary=binary_selector)
        status = fetch_json(bridge, "status")
        binaries = fetch_json(bridge, "binaries")
    except RuntimeError as exc:
        return PreflightResult(False, (str(exc), remediation_message(expected_file),))

    errors: list[str] = []
    errors.extend(
        validate_status(
            status,
            expected_file=expected_file,
            expected_platform=expected_platform,
            expected_arch=expected_arch,
        )
    )
    errors.extend(validate_binaries(binaries, expected_file=expected_file, require_active=binary_selector is None))

    if errors:
        messages.extend(errors)
        messages.append(remediation_message(expected_file))
        return PreflightResult(False, tuple(messages))

    try:
        probe = fetch_json(bridge, "functionInfo", address=probe_address)
    except RuntimeError as exc:
        return PreflightResult(False, (str(exc), remediation_message(expected_file),))
    errors.extend(validate_probe(probe, probe_address=probe_address))

    if errors:
        messages.extend(errors)
        messages.append(remediation_message(expected_file))
        return PreflightResult(False, tuple(messages))

    messages.append(
        "Binary Ninja preflight OK: "
        f"{expected_file} loaded as {expected_platform}/{expected_arch}; "
        f"probe {probe_address} resolved."
    )
    return PreflightResult(True, tuple(messages))


def remediation_message(expected_file: str) -> str:
    return (
        "Open the expected BNDB in Binary Ninja and ensure the updated target-qualified bridge plugin is running: "
        f"{expected_file}"
    )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Check the local Binary Ninja bridge state.")
    parser.add_argument(
        "--binary",
        choices=reference_image_keys(),
        default="recoil",
        help="Reference image whose BNDB should be active.",
    )
    parser.add_argument("--bridge-url", default=DEFAULT_BRIDGE_URL)
    parser.add_argument("--expected-file", default=None)
    parser.add_argument("--expected-platform", default=None)
    parser.add_argument("--expected-arch", default=None)
    parser.add_argument("--probe-address", default=None)
    parser.add_argument("--strict", action="store_true", help="Return nonzero when preflight fails.")
    parser.add_argument(
        "--data-overlap",
        nargs="*",
        metavar="ADDRESS",
        default=None,
        help=(
            "Report BN bridge data variables whose roots begin inside another sized data variable. "
            "Pass optional addresses to focus the report; with no addresses, audit all returned data items."
        ),
    )
    parser.add_argument(
        "--data-overlap-limit",
        type=int,
        default=40,
        help="Maximum data-overlap findings to print.",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    image = reference_image(args.binary)
    expected_file = args.expected_file or image.bndb_path
    expected_platform = args.expected_platform or image.platform
    expected_arch = args.expected_arch or image.arch
    probe_address = args.probe_address or image.probe_address
    result = run_preflight(
        bridge_url=args.bridge_url,
        expected_file=expected_file,
        expected_platform=expected_platform,
        expected_arch=expected_arch,
        probe_address=probe_address,
        binary_selector=Path(expected_file).name,
    )
    for message in result.messages:
        print(message)
    ok = result.ok
    if args.data_overlap is not None:
        try:
            data_result = run_data_overlap_audit(
                bridge_url=args.bridge_url,
                probe_addresses=tuple(args.data_overlap),
                max_findings=max(args.data_overlap_limit, 1),
            )
        except RuntimeError as exc:
            data_result = DataOverlapAuditResult(
                ok=False,
                messages=(
                    f"Binary Ninja data overlap audit failed: {exc}",
                    "This diagnostic is report-only; it does not repair BN data variables or provide data gate acceptance.",
                ),
                item_count=0,
                finding_count=0,
            )
        for message in data_result.messages:
            print(message)
        ok = ok and data_result.ok
    return 1 if args.strict and not ok else 0


if __name__ == "__main__":
    raise SystemExit(main())
