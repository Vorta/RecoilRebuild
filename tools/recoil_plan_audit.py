from __future__ import annotations

import argparse
from collections import Counter
from pathlib import Path
import sys

from recoil_plan import DONE_STATUS, LIMITED_STATUS, PlanEntry, load_plan, status_summary


ACCEPTED_DEP_STATUSES = {DONE_STATUS, LIMITED_STATUS}


def entry_has_limited_marker(entry: PlanEntry) -> bool:
    return LIMITED_STATUS in (
        entry.reconstructed_status,
        entry.source_dependencies_status,
        entry.reimplemented_status,
        entry.binary_verified_status,
    )


def entry_has_unknown_marker(entry: PlanEntry) -> bool:
    return "❓" in (
        entry.reconstructed_status,
        entry.source_dependencies_status,
        entry.reimplemented_status,
        entry.binary_verified_status,
    )


def find_impl_green_deps_not_green(entries: list[PlanEntry]) -> list[PlanEntry]:
    return [
        entry
        for entry in entries
        if entry.reimplemented_status == DONE_STATUS
        and entry.source_dependencies_status not in ACCEPTED_DEP_STATUSES
    ]


def find_verify_done_impl_not_green(entries: list[PlanEntry]) -> list[PlanEntry]:
    return [
        entry
        for entry in entries
        if entry.binary_verified_status in ACCEPTED_DEP_STATUSES
        and entry.reimplemented_status != DONE_STATUS
    ]


def find_verify_done_deps_not_green(entries: list[PlanEntry]) -> list[PlanEntry]:
    return [
        entry
        for entry in entries
        if entry.binary_verified_status in ACCEPTED_DEP_STATUSES
        and entry.source_dependencies_status not in ACCEPTED_DEP_STATUSES
    ]


def print_entries(title: str, entries: list[PlanEntry], limit: int) -> None:
    print(title)
    if not entries:
        print("- none")
        return
    for entry in entries[:limit]:
        print(
            f"- {entry.address} {entry.milestone} {entry.reconstructed_name} "
            f"{status_summary(entry)}"
        )
    if len(entries) > limit:
        print(f"- ... {len(entries) - limit} more")


def main(argv: list[str] | None = None) -> int:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    if hasattr(sys.stderr, "reconfigure"):
        sys.stderr.reconfigure(encoding="utf-8")

    parser = argparse.ArgumentParser(description="Audit RECOIL_PLAN marker consistency.")
    parser.add_argument("--plan", default=".agent/RECOIL_PLAN.md")
    parser.add_argument("--summary", action="store_true", help="Print summary counts.")
    parser.add_argument("--strict", action="store_true", help="Return nonzero on inconsistencies.")
    parser.add_argument("--limit", type=int, default=10, help="Maximum entries per finding list.")
    args = parser.parse_args(argv)

    plan = load_plan(Path(args.plan))
    entries = list(plan.values())

    fields = [
        ("recon", "reconstructed_status"),
        ("deps", "source_dependencies_status"),
        ("impl", "reimplemented_status"),
        ("verify", "binary_verified_status"),
    ]
    print(f"entries: {len(entries)}")
    for label, attr in fields:
        counts = Counter(getattr(entry, attr) for entry in entries)
        rendered = " ".join(f"{key}={value}" for key, value in sorted(counts.items()))
        print(f"{label}: {rendered}")

    fully_complete = sum(
        1
        for entry in entries
        if entry.reconstructed_status == DONE_STATUS
        and entry.source_dependencies_status == DONE_STATUS
        and entry.reimplemented_status == DONE_STATUS
        and entry.binary_verified_status == DONE_STATUS
    )
    limited = sum(1 for entry in entries if entry_has_limited_marker(entry))
    unknown = sum(1 for entry in entries if entry_has_unknown_marker(entry))
    print(f"fully_complete_all_green: {fully_complete}")
    print(f"entries_with_limited_marker: {limited}")
    print(f"entries_with_unknown_marker: {unknown}")

    impl_green_deps_not_green = find_impl_green_deps_not_green(entries)
    verify_done_impl_not_green = find_verify_done_impl_not_green(entries)
    verify_done_deps_not_green = find_verify_done_deps_not_green(entries)

    print_entries(
        "reimplemented_done_but_dependencies_not_accepted:",
        impl_green_deps_not_green,
        args.limit,
    )
    print_entries(
        "binary_verified_but_reimplemented_not_done:",
        verify_done_impl_not_green,
        args.limit,
    )
    print_entries(
        "binary_verified_but_dependencies_not_accepted:",
        verify_done_deps_not_green,
        args.limit,
    )

    if args.strict and (
        impl_green_deps_not_green or verify_done_impl_not_green or verify_done_deps_not_green
    ):
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
