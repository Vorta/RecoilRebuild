from __future__ import annotations

import argparse
from pathlib import Path
import sys

from recoil_binja import BinaryNinjaBridge, BridgeError
from recoil_claim import DEFAULT_CLAIMS_DIR, format_utc, read_claim
from recoil_frontier import build_frontier, recommendation
from recoil_groups_audit import audit_group, parse_groups
from recoil_plan import FIELD_LABELS, PlanDocument, PlanEntry, blocker_field, normalize_address, status_summary
from recoil_tooling import configure_stdio
from recoil_vc6_verify import DEFAULT_MANIFEST_DIR, covering_targets, load_manifests


def select_entry(doc: PlanDocument, address: str | None, *, lane: str = "strict") -> PlanEntry:
    if address:
        normalized = normalize_address(address)
        entry = doc.entries.get(normalized)
        if entry is None:
            raise ValueError(f"Address not found in plan: {normalized}")
        return entry
    entry = doc.first_unfinished(lane=lane)
    if entry is None:
        raise ValueError("No unfinished plan entries found.")
    return entry


def print_plan_status(entry: PlanEntry, *, lane: str = "strict") -> None:
    blocker = blocker_field(entry, lane=lane) or "none"
    print("# Recoil Agent Status")
    print()
    print(f"Address: {entry.address}")
    print(f"Milestone: {entry.milestone}")
    print(f"Name: {entry.reimplemented_name or entry.reconstructed_name or 'pending'}")
    print(f"File: {entry.reimplemented_file or 'pending'}")
    if entry.provider:
        print(f"Provider: {entry.provider}")
    print(f"Plan status: {status_summary(entry, include_functional=lane == 'progress')}")
    print(f"Lane: {lane}")
    if entry.functional_equivalent_status == "✅":
        print(f"Functional target: {entry.functional_target or 'recorded'}")
    print(f"Next blocker: {blocker} ({FIELD_LABELS.get(blocker, blocker)})")
    print(f"Show: python tools/recoil_plan_cli.py show {entry.address}")


def print_group_status(entry: PlanEntry, groups_path: Path, doc: PlanDocument) -> None:
    print()
    print("## Group Context")
    if not groups_path.exists():
        print(f"- groups file not found: {groups_path}")
        return

    groups = parse_groups(groups_path.read_text(encoding="utf-8"))
    matches = [group for group in groups if entry.address in group.addresses]
    if not matches:
        print("- no implementation group contains this address")
        return

    for group in matches:
        audit = audit_group(group, doc.entries)
        print(
            f"- {group.title} section={group.section} "
            f"addresses={len(group.addresses)} recommendation={audit.recommendation}"
        )


def print_claim_status(entry: PlanEntry, claims_dir: Path) -> None:
    print()
    print("## Claim")
    claim = read_claim(claims_dir, entry.address)
    if claim is None:
        print("- unclaimed")
        print(f"- claim: python tools/recoil_claim.py claim {entry.address} --owner <name>")
        return

    state = "expired" if claim.is_expired else "active"
    reason = f" reason={claim.reason}" if claim.reason else ""
    print(
        f"- {state}: owner={claim.owner} expires={format_utc(claim.expires_at_utc)}{reason}"
    )
    print(f"- release: python tools/recoil_claim.py release {entry.address} --token <token>")


def print_vc6_status(entry: PlanEntry, manifest_dir: Path) -> None:
    print()
    print("## VC Coverage")
    manifests = load_manifests(manifest_dir)
    matches = covering_targets(manifests, entry.address)
    if not matches:
        print("- no VC verification target covers this address")
        print(f"- scaffold: python tools/recoil_vc6_verify.py --explain-missing {entry.address}")
        return

    for manifest, function in matches:
        compiler_profile = f" profile={manifest.compiler_profile}" if manifest.compiler_profile else ""
        compiler_env = f" compiler_env={manifest.compiler_env}" if manifest.compiler_env else ""
        print(f"- {manifest.name}: {function.symbol} mode={manifest.compare_mode}{compiler_profile}{compiler_env}")
    print(f"- verify: python tools/recoil_vc6_verify.py {entry.address}")


def print_frontier_status(
    entry: PlanEntry,
    plan: dict[str, PlanEntry],
    *,
    bridge_url: str,
    depth: int,
    lane: str = "strict",
) -> bool:
    print()
    print("## Frontier")
    try:
        bridge = BinaryNinjaBridge(bridge_url)
        nodes = build_frontier(entry.address, depth, bridge, plan)
    except (BridgeError, ValueError) as exc:
        print(f"- unavailable: {exc}")
        return False

    root = nodes[entry.address]
    source_blockers = [
        node for node in nodes.values() if node.address != entry.address and node.blocks_source()
    ]
    verify_blockers = [
        node
        for node in nodes.values()
        if node.address != entry.address
        and not node.blocks_source()
        and node.blocks_binary_verification(lane=lane)
    ]
    print(f"- direct callees: {len(root.callees)}")
    print(f"- unresolved calls: {len(root.unresolved_calls)}")
    print(f"- indirect calls: {len(root.indirect_calls)}")
    print(f"- visible source blockers: {len(source_blockers)}")
    print(f"- visible verification blockers: {len(verify_blockers)}")
    print(f"- recommended next: {recommendation(entry.address, nodes, lane=lane)}")
    lane_arg = "" if lane == "strict" else f" --lane {lane}"
    print(f"- full report: python tools/recoil_frontier.py {entry.address} --depth {depth}{lane_arg}")
    return True


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Print the current reconstruction status, dependency context, and VC verification coverage."
    )
    parser.add_argument("address", nargs="?", help="Plan address. Defaults to first unfinished entry.")
    parser.add_argument("--plan", default=".agent/RECOIL_PLAN.md")
    parser.add_argument("--groups", default=".agent/IMPLEMENTATION_GROUPS.md")
    parser.add_argument("--claims-dir", default=str(DEFAULT_CLAIMS_DIR))
    parser.add_argument("--vc6-manifest-dir", default=str(DEFAULT_MANIFEST_DIR))
    parser.add_argument("--bridge-url", default="http://localhost:9009")
    parser.add_argument("--depth", type=int, default=1)
    parser.add_argument("--no-frontier", action="store_true", help="Skip Binary Ninja frontier queries.")
    parser.add_argument(
        "--lane",
        choices=["strict", "progress"],
        default="strict",
        help="strict keeps Binary-safe verified as the blocker; progress accepts functional markers.",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = build_parser()
    args = parser.parse_args(argv)

    try:
        doc = PlanDocument.load(Path(args.plan))
        entry = select_entry(doc, args.address, lane=args.lane)
        print_plan_status(entry, lane=args.lane)
        print_claim_status(entry, Path(args.claims_dir))
        print_group_status(entry, Path(args.groups), doc)
        print_vc6_status(entry, Path(args.vc6_manifest_dir))
        if not args.no_frontier:
            print_frontier_status(
                entry,
                doc.entries,
                bridge_url=args.bridge_url,
                depth=args.depth,
                lane=args.lane,
            )
    except (OSError, ValueError) as exc:
        print(str(exc), file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
