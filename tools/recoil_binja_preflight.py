#!/usr/bin/env python3
"""Check that the expected Binary Ninja database is open through the bridge."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import sys
from typing import Any

from recoil_binja import BinaryNinjaBridge, BridgeError
from recoil_tooling import configure_stdio


DEFAULT_BRIDGE_URL = "http://localhost:9009"
DEFAULT_EXPECTED_FILE = "D:/Recoil Project/Decomp/Recoil.bndb"
DEFAULT_EXPECTED_PLATFORM = "windows-x86"
DEFAULT_EXPECTED_ARCH = "x86"
DEFAULT_PROBE_ADDRESS = "0x401000"


@dataclass(frozen=True)
class PreflightResult:
    ok: bool
    messages: tuple[str, ...]


def normalize_path_text(value: str) -> str:
    return str(Path(value.replace("\\", "/"))).replace("\\", "/").lower()


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


def validate_binaries(binaries: dict[str, Any], *, expected_file: str) -> list[str]:
    items = binaries.get("binaries")
    if not isinstance(items, list) or not items:
        return ["bridge /binaries returned no open binaries"]

    expected = normalize_path_text(expected_file)
    active_matches = []
    for item in items:
        if not isinstance(item, dict):
            continue
        filename = str(item.get("filename", ""))
        if normalize_path_text(filename) == expected and item.get("active") is True:
            active_matches.append(item)

    if not active_matches:
        return [f"expected database is not the active Binary Ninja binary: {expected_file}"]
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


def run_preflight(
    *,
    bridge_url: str,
    expected_file: str,
    expected_platform: str,
    expected_arch: str,
    probe_address: str,
) -> PreflightResult:
    messages: list[str] = []
    try:
        bridge = BinaryNinjaBridge(bridge_url, timeout=10.0)
        status = fetch_json(bridge, "status")
        binaries = fetch_json(bridge, "binaries")
        probe = fetch_json(bridge, "functionInfo", address=probe_address)
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
    errors.extend(validate_binaries(binaries, expected_file=expected_file))
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
        "Open the expected BNDB in Binary Ninja and ensure the local bridge plugin is running: "
        f"{expected_file}"
    )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Check the local Binary Ninja bridge state.")
    parser.add_argument("--bridge-url", default=DEFAULT_BRIDGE_URL)
    parser.add_argument("--expected-file", default=DEFAULT_EXPECTED_FILE)
    parser.add_argument("--expected-platform", default=DEFAULT_EXPECTED_PLATFORM)
    parser.add_argument("--expected-arch", default=DEFAULT_EXPECTED_ARCH)
    parser.add_argument("--probe-address", default=DEFAULT_PROBE_ADDRESS)
    parser.add_argument("--strict", action="store_true", help="Return nonzero when preflight fails.")
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    result = run_preflight(
        bridge_url=args.bridge_url,
        expected_file=args.expected_file,
        expected_platform=args.expected_platform,
        expected_arch=args.expected_arch,
        probe_address=args.probe_address,
    )
    for message in result.messages:
        print(message)
    return 1 if args.strict and not result.ok else 0


if __name__ == "__main__":
    raise SystemExit(main())
