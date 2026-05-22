from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
from pathlib import Path
import sys

from recoil_binja import BinaryNinjaBridge, BridgeError
from recoil_frontier import build_frontier, is_budget_error_text
from recoil_functional_verify import DEFAULT_MANIFEST_DIR as DEFAULT_FUNCTIONAL_DIR
from recoil_functional_verify import load_manifests as load_functional_manifests
from recoil_plan import (
    ACCEPTED_STATUSES,
    DONE_STATUS,
    FUNCTIONAL_LANE,
    LANE_CHOICES,
    PlanDocument,
    PlanEntry,
    normalize_address,
    normalize_lane,
)
from recoil_tooling import configure_stdio
from recoil_vc6_verify import DEFAULT_MANIFEST_DIR, covering_targets, load_manifests


@dataclass(frozen=True)
class BacklogItem:
    address: str
    name: str
    source_file: str
    milestone: str
    group: str
    functional_status: str
    binary_status: str
    has_functional_target: bool
    has_vc_target: bool
    direct_callee_count: int | None = None

    @property
    def sort_key(self) -> tuple[str, int, str]:
        leaf_rank = 1 if self.direct_callee_count is None else min(self.direct_callee_count, 999)
        return (self.group, leaf_rank, self.address)


def is_authored(entry: PlanEntry) -> bool:
    return (
        entry.reimplemented_status == DONE_STATUS
        and not entry.is_provider
        and bool(entry.reimplemented_file)
        and entry.reimplemented_file != "external"
    )


def is_source_ready(entry: PlanEntry) -> bool:
    return (
        entry.reconstructed_status in ACCEPTED_STATUSES
        and entry.source_dependencies_status in ACCEPTED_STATUSES
        and entry.reimplemented_status == DONE_STATUS
    )


def is_backlog_gap(entry: PlanEntry, *, lane: str) -> bool:
    lane = normalize_lane(lane)
    if not is_authored(entry):
        return False
    if not is_source_ready(entry):
        return False
    if lane == FUNCTIONAL_LANE:
        return entry.functional_equivalent_status != DONE_STATUS
    return (
        entry.functional_equivalent_status == DONE_STATUS
        and entry.binary_verified_status != DONE_STATUS
    )


def source_blocked_gap_count(doc: PlanDocument, *, lane: str, source_filter: str = "") -> int:
    lane = normalize_lane(lane)
    skipped = 0
    for address in doc.order:
        entry = doc.entries[address]
        if not is_authored(entry):
            continue
        if source_filter and Path(entry.reimplemented_file).as_posix() != Path(source_filter).as_posix():
            continue
        if is_source_ready(entry):
            continue
        if lane == FUNCTIONAL_LANE and entry.functional_equivalent_status != DONE_STATUS:
            skipped += 1
        elif (
            lane != FUNCTIONAL_LANE
            and entry.functional_equivalent_status == DONE_STATUS
            and entry.binary_verified_status != DONE_STATUS
        ):
            skipped += 1
    return skipped


def group_key(entry: PlanEntry, *, group_by: str) -> str:
    if group_by == "none":
        return "all"
    if group_by == "milestone":
        return entry.milestone or "(no milestone)"
    if group_by == "source":
        return entry.reimplemented_file or "(no source)"

    name = entry.reimplemented_name or entry.reconstructed_name
    if "::" in name:
        return name.split("::", 1)[0]
    return entry.reimplemented_file or "(no class)"


def build_backlog(
    doc: PlanDocument,
    vc_covered_addresses: set[str],
    functional_covered_addresses: set[str] | None = None,
    *,
    lane: str = FUNCTIONAL_LANE,
    source_filter: str = "",
    group_by: str = "class",
) -> list[BacklogItem]:
    functional_covered_addresses = functional_covered_addresses or set()
    lane = normalize_lane(lane)
    items: list[BacklogItem] = []
    for address in doc.order:
        entry = doc.entries[address]
        if not is_backlog_gap(entry, lane=lane):
            continue
        if source_filter and Path(entry.reimplemented_file).as_posix() != Path(source_filter).as_posix():
            continue
        items.append(
            BacklogItem(
                address=entry.address,
                name=entry.reimplemented_name or entry.reconstructed_name or "pending",
                source_file=entry.reimplemented_file,
                milestone=entry.milestone,
                group=group_key(entry, group_by=group_by),
                functional_status=entry.functional_equivalent_status,
                binary_status=entry.binary_verified_status,
                has_functional_target=entry.address in functional_covered_addresses,
                has_vc_target=entry.address in vc_covered_addresses,
            )
        )
    return items


def add_binja_leaf_estimates(
    items: list[BacklogItem],
    doc: PlanDocument,
    *,
    bridge_url: str,
) -> list[BacklogItem]:
    bridge = BinaryNinjaBridge(bridge_url)
    ranked: list[BacklogItem] = []
    budget_exhausted = False
    for item in items:
        if budget_exhausted:
            ranked.append(item)
            continue
        try:
            nodes = build_frontier(item.address, 1, bridge, doc.entries)
            root = nodes[normalize_address(item.address)]
            if is_budget_error_text(root.bridge_error):
                budget_exhausted = True
                callee_count = None
            else:
                callee_count = len(root.callees)
        except (BridgeError, ValueError) as exc:
            callee_count = None
            if "budget exhausted" in str(exc).lower():
                budget_exhausted = True
        ranked.append(
            BacklogItem(
                address=item.address,
                name=item.name,
                source_file=item.source_file,
                milestone=item.milestone,
                group=item.group,
                functional_status=item.functional_status,
                binary_status=item.binary_status,
                has_functional_target=item.has_functional_target,
                has_vc_target=item.has_vc_target,
                direct_callee_count=callee_count,
            )
        )
    return ranked


def vc_coverage_set(manifest_dir: Path) -> set[str]:
    manifests = load_manifests(manifest_dir)
    return {
        function.address
        for manifest in manifests
        for function in manifest.functions
        if covering_targets(manifests, function.address)
    }


def functional_coverage_set(manifest_dir: Path) -> set[str]:
    return {target.address for target in load_functional_manifests(manifest_dir)}


def print_text(
    items: list[BacklogItem],
    *,
    limit: int,
    lane: str,
    group_by: str,
    source_blocked_skipped: int = 0,
) -> None:
    selected = items[:limit] if limit >= 0 else items
    print(f"verification_backlog lane={lane} group_by={group_by}: {len(items)} item(s)")
    if source_blocked_skipped:
        print(f"source-blocked implemented entries skipped: {source_blocked_skipped}")
    if any(item.direct_callee_count is None for item in items):
        print("leaf estimates may be incomplete; Binary Ninja calls are limited to 10 per tool invocation")
    current_group = ""
    for item in selected:
        if item.group != current_group:
            current_group = item.group
            print()
            print(current_group)
        leaf = "unknown" if item.direct_callee_count is None else str(item.direct_callee_count)
        print(
            f"- {item.address} {item.name} functional={item.functional_status} "
            f"binary={item.binary_status} functional_target={yes_no(item.has_functional_target)} "
            f"vc_target={yes_no(item.has_vc_target)} callees={leaf} milestone={item.milestone}"
        )
    if limit >= 0 and len(items) > limit:
        print(f"\n... {len(items) - limit} more")


def yes_no(value: bool) -> str:
    return "yes" if value else "no"


def item_to_dict(item: BacklogItem) -> dict[str, object]:
    return {
        "address": item.address,
        "name": item.name,
        "source_file": item.source_file,
        "milestone": item.milestone,
        "group": item.group,
        "functional_status": item.functional_status,
        "binary_status": item.binary_status,
        "has_functional_target": item.has_functional_target,
        "has_vc_target": item.has_vc_target,
        "direct_callee_count": item.direct_callee_count,
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Rank authored functions that still need functional or binary-safe verification evidence."
    )
    parser.add_argument("--plan", default=".agent/RECOIL_PLAN.md")
    parser.add_argument("--manifest-dir", default=str(DEFAULT_MANIFEST_DIR))
    parser.add_argument("--functional-dir", default=str(DEFAULT_FUNCTIONAL_DIR))
    parser.add_argument("--source-from", default="", help="Only show entries implemented in this source file.")
    parser.add_argument("--binja", action="store_true", help="Estimate leafness through Binary Ninja direct callees.")
    parser.add_argument("--bridge-url", default="http://localhost:9009")
    parser.add_argument("--limit", type=int, default=40, help="Maximum items to print; use -1 for all.")
    parser.add_argument("--json", action="store_true")
    parser.add_argument(
        "--lane",
        choices=LANE_CHOICES,
        default=FUNCTIONAL_LANE,
        help="functional lists missing Functional-equivalent evidence; binary lists class-pass candidates.",
    )
    parser.add_argument(
        "--group-by",
        choices=["class", "source", "milestone", "none"],
        default="class",
        help="Grouping for the backlog report. Class grouping is best for binary-safe passes.",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        lane = normalize_lane(args.lane)
        doc = PlanDocument.load(Path(args.plan))
        items = build_backlog(
            doc,
            vc_coverage_set(Path(args.manifest_dir)),
            functional_coverage_set(Path(args.functional_dir)),
            lane=lane,
            source_filter=args.source_from,
            group_by=args.group_by,
        )
        source_blocked_skipped = source_blocked_gap_count(
            doc,
            lane=lane,
            source_filter=args.source_from,
        )
        if args.binja:
            items = add_binja_leaf_estimates(items, doc, bridge_url=args.bridge_url)
        items = sorted(items, key=lambda item: item.sort_key)
        if args.json:
            print(
                json.dumps(
                    {
                        "items": [item_to_dict(item) for item in items],
                        "source_blocked_skipped": source_blocked_skipped,
                    },
                    indent=2,
                )
            )
        else:
            print_text(
                items,
                limit=args.limit,
                lane=lane,
                group_by=args.group_by,
                source_blocked_skipped=source_blocked_skipped,
            )
    except (OSError, ValueError) as exc:
        print(str(exc), file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
