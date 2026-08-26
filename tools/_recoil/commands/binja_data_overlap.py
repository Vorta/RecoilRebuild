#!/usr/bin/env python3
"""Report Binary Ninja data variables that overlap as interior roots."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from _recoil.commands.binja_preflight import (
    DEFAULT_BRIDGE_URL,
    DEFAULT_EXPECTED_ARCH,
    DEFAULT_EXPECTED_FILE,
    DEFAULT_EXPECTED_PLATFORM,
    DEFAULT_PROBE_ADDRESS,
    DataOverlapAuditResult,
    run_data_overlap_audit,
    run_preflight,
)
from _recoil.lib.tooling import configure_stdio


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Report BN bridge data variables whose roots begin inside another sized data variable. "
            "This is a diagnostic only; it does not repair BN state or satisfy data gates."
        )
    )
    parser.add_argument("addresses", nargs="*", metavar="ADDRESS", help="Optional addresses to focus the report.")
    parser.add_argument("--bridge-url", default=DEFAULT_BRIDGE_URL)
    parser.add_argument("--expected-file", default=DEFAULT_EXPECTED_FILE)
    parser.add_argument("--expected-platform", default=DEFAULT_EXPECTED_PLATFORM)
    parser.add_argument("--expected-arch", default=DEFAULT_EXPECTED_ARCH)
    parser.add_argument("--probe-address", default=DEFAULT_PROBE_ADDRESS)
    parser.add_argument(
        "--skip-preflight",
        action="store_true",
        help="Skip the standard Binary Ninja database preflight before inspecting data variables.",
    )
    parser.add_argument("--limit", type=int, default=40, help="Maximum overlap findings to print.")
    parser.add_argument("--strict", action="store_true", help="Return nonzero when preflight or overlap audit fails.")
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    ok = True

    if not args.skip_preflight:
        preflight = run_preflight(
            bridge_url=args.bridge_url,
            expected_file=args.expected_file,
            expected_platform=args.expected_platform,
            expected_arch=args.expected_arch,
            probe_address=args.probe_address,
        )
        for message in preflight.messages:
            print(message)
        ok = preflight.ok
        if not preflight.ok:
            return 1 if args.strict else 0

    try:
        result = run_data_overlap_audit(
            bridge_url=args.bridge_url,
            probe_addresses=tuple(args.addresses),
            max_findings=max(args.limit, 1),
        )
    except (RuntimeError, ValueError) as exc:
        result = DataOverlapAuditResult(
            ok=False,
            messages=(
                f"Binary Ninja data overlap audit failed: {exc}",
                "This diagnostic is report-only; it does not repair BN data variables or provide data gate acceptance.",
            ),
            item_count=0,
            finding_count=0,
        )

    for message in result.messages:
        print(message)
    ok = ok and result.ok
    return 1 if args.strict and not ok else 0


if __name__ == "__main__":
    raise SystemExit(main())
