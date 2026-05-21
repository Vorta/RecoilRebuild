from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
from pathlib import Path
import sys

from recoil_binja import BinaryNinjaBridge, BridgeError
from recoil_frontier import build_frontier
from recoil_plan import DONE_STATUS, PlanDocument, PlanEntry, normalize_address
from recoil_tooling import configure_stdio
from recoil_vc6_verify import DEFAULT_MANIFEST_DIR, covering_targets, load_manifests


@dataclass(frozen=True)
class BacklogItem:
    address: str
    name: str
    source_file: str
    milestone: str
    direct_callee_count: int | None = None

    @property
    def sort_key(self) -> tuple[int, str, str]:
        leaf_rank = 1 if self.direct_callee_count is None else min(self.direct_callee_count, 999)
        return (leaf_rank, self.source_file, self.address)


def is_authored_verification_gap(entry: PlanEntry, covered_addresses: set[str]) -> bool:
    return (
        entry.reimplemented_status == DONE_STATUS
        and entry.binary_verified_status != DONE_STATUS
        and not entry.is_provider
        and bool(entry.reimplemented_file)
        and entry.reimplemented_file != "external"
        and entry.address not in covered_addresses
    )


def build_backlog(
    doc: PlanDocument,
    covered_addresses: set[str],
    *,
    source_filter: str = "",
) -> list[BacklogItem]:
    items: list[BacklogItem] = []
    for address in doc.order:
        entry = doc.entries[address]
        if not is_authored_verification_gap(entry, covered_addresses):
            continue
        if source_filter and Path(entry.reimplemented_file).as_posix() != Path(source_filter).as_posix():
            continue
        items.append(
            BacklogItem(
                address=entry.address,
                name=entry.reimplemented_name or entry.reconstructed_name or "pending",
                source_file=entry.reimplemented_file,
                milestone=entry.milestone,
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
    for item in items:
        try:
            nodes = build_frontier(item.address, 1, bridge, doc.entries)
            root = nodes[normalize_address(item.address)]
            callee_count = len(root.callees)
        except (BridgeError, ValueError):
            callee_count = None
        ranked.append(
            BacklogItem(
                address=item.address,
                name=item.name,
                source_file=item.source_file,
                milestone=item.milestone,
                direct_callee_count=callee_count,
            )
        )
    return ranked


def coverage_set(manifest_dir: Path) -> set[str]:
    manifests = load_manifests(manifest_dir)
    return {
        function.address
        for manifest in manifests
        for function in manifest.functions
        if covering_targets(manifests, function.address)
    }


def print_text(items: list[BacklogItem], *, limit: int) -> None:
    selected = items[:limit] if limit >= 0 else items
    print(f"verification_backlog: {len(items)} item(s)")
    current_source = ""
    for item in selected:
        if item.source_file != current_source:
            current_source = item.source_file
            print()
            print(current_source)
        leaf = "unknown" if item.direct_callee_count is None else str(item.direct_callee_count)
        print(f"- {item.address} {item.name} callees={leaf} milestone={item.milestone}")
    if limit >= 0 and len(items) > limit:
        print(f"\n... {len(items) - limit} more")


def item_to_dict(item: BacklogItem) -> dict[str, object]:
    return {
        "address": item.address,
        "name": item.name,
        "source_file": item.source_file,
        "milestone": item.milestone,
        "direct_callee_count": item.direct_callee_count,
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Rank implemented authored functions that still need VC verification coverage."
    )
    parser.add_argument("--plan", default=".agent/RECOIL_PLAN.md")
    parser.add_argument("--manifest-dir", default=str(DEFAULT_MANIFEST_DIR))
    parser.add_argument("--source-from", default="", help="Only show entries implemented in this source file.")
    parser.add_argument("--binja", action="store_true", help="Estimate leafness through Binary Ninja direct callees.")
    parser.add_argument("--bridge-url", default="http://localhost:9009")
    parser.add_argument("--limit", type=int, default=40, help="Maximum items to print; use -1 for all.")
    parser.add_argument("--json", action="store_true")
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        doc = PlanDocument.load(Path(args.plan))
        items = build_backlog(doc, coverage_set(Path(args.manifest_dir)), source_filter=args.source_from)
        if args.binja:
            items = add_binja_leaf_estimates(items, doc, bridge_url=args.bridge_url)
        items = sorted(items, key=lambda item: item.sort_key)
        if args.json:
            print(json.dumps([item_to_dict(item) for item in items], indent=2))
        else:
            print_text(items, limit=args.limit)
    except (OSError, ValueError) as exc:
        print(str(exc), file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

