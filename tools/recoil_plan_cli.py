from __future__ import annotations

import argparse
from pathlib import Path
import sys

from recoil_plan import (
    FIELD_LABELS,
    FUNCTIONAL_LANE,
    LANE_CHOICES,
    PlanDocument,
    PlanEntry,
    blocker_field,
    normalize_address,
    normalize_field,
    normalize_lane,
    status_summary,
)


def configure_stdio() -> None:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    if hasattr(sys.stderr, "reconfigure"):
        sys.stderr.reconfigure(encoding="utf-8")


def compact_entry(entry: PlanEntry, *, lane: str = FUNCTIONAL_LANE) -> str:
    lane = normalize_lane(lane)
    blocker = blocker_field(entry, lane=lane) or "none"
    name = entry.reimplemented_name
    if not name or name == "pending":
        name = entry.reconstructed_name or "pending"
    file_name = entry.reimplemented_file or "pending"
    provider = f" provider={entry.provider}" if entry.provider else ""
    functional = (
        f" functional_target={entry.functional_target}"
        if entry.functional_equivalent_status == "✅" and entry.functional_target
        else ""
    )
    return (
        f"{entry.address} {entry.milestone} "
        f"{status_summary(entry)} "
        f"blocker={blocker} name={name} file={file_name}{provider}{functional}"
    )


def print_entries(
    entries: list[PlanEntry],
    *,
    limit: int | None = None,
    lane: str = FUNCTIONAL_LANE,
) -> None:
    selected = entries[:limit] if limit is not None else entries
    for entry in selected:
        print(compact_entry(entry, lane=lane))
    if limit is not None and len(entries) > limit:
        print(f"... {len(entries) - limit} more")


def command_show(doc: PlanDocument, args: argparse.Namespace) -> int:
    print(doc.entry_text(args.address))
    return 0


def command_find(doc: PlanDocument, args: argparse.Namespace) -> int:
    matches = doc.find(args.query)
    print_entries(matches, limit=args.limit, lane=args.lane)
    if not matches:
        return 1
    return 0


def command_milestone(doc: PlanDocument, args: argparse.Namespace) -> int:
    matches = doc.milestone_entries(args.milestone)
    print_entries(matches, limit=args.limit, lane=args.lane)
    if not matches:
        return 1
    return 0


def command_next(doc: PlanDocument, args: argparse.Namespace) -> int:
    lane = normalize_lane(args.lane)
    entry = doc.first_unfinished(lane=lane)
    if entry is None:
        print("No unfinished plan entries found.")
        return 0

    field = blocker_field(entry, lane=lane) or "none"
    print(compact_entry(entry, lane=lane))
    print(f"next_field={field} label={FIELD_LABELS.get(field, field)}")
    print(f"show_command=python tools/recoil_plan_cli.py show {entry.address}")
    return 0


def command_set(doc: PlanDocument, args: argparse.Namespace) -> int:
    address = normalize_address(args.address)
    field = normalize_field(args.field)
    marker_file = args.target or args.file
    before, after = doc.update_marker(
        address,
        field,
        args.status,
        name=args.name,
        file=marker_file,
        provider=args.provider,
        evidence=args.evidence,
    )
    if args.dry_run:
        print("Before:")
        print(before)
        print()
        print("After:")
        print(after)
        return 0

    doc.save()
    print(f"updated {address} {field}={args.status}")
    if args.evidence:
        print(f"evidence={args.evidence}")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Navigate and update .agent/RECOIL_PLAN.md without line-number slicing."
    )
    parser.add_argument("--plan", default=".agent/RECOIL_PLAN.md", help="Path to RECOIL_PLAN.md")
    subparsers = parser.add_subparsers(dest="command", required=True)

    show = subparsers.add_parser("show", help="Print one complete plan entry by address.")
    show.add_argument("address")
    show.set_defaults(func=command_show)

    find = subparsers.add_parser("find", help="Search addresses, names, files, providers, and milestones.")
    find.add_argument("query")
    find.add_argument("--limit", type=int, default=50)
    find.add_argument("--lane", choices=LANE_CHOICES, default=FUNCTIONAL_LANE)
    find.set_defaults(func=command_find)

    milestone = subparsers.add_parser("milestone", help="List entries in a milestone.")
    milestone.add_argument("milestone", help="Milestone prefix or substring, e.g. M27")
    milestone.add_argument("--limit", type=int, default=None)
    milestone.add_argument("--lane", choices=LANE_CHOICES, default=FUNCTIONAL_LANE)
    milestone.set_defaults(func=command_milestone)

    next_entry = subparsers.add_parser("next", help="Print the first unfinished entry and blocker.")
    next_entry.add_argument(
        "--lane",
        choices=LANE_CHOICES,
        default=FUNCTIONAL_LANE,
        help="functional stops at Functional-equivalent; binary also requires Binary-safe.",
    )
    next_entry.set_defaults(func=command_next)

    set_marker = subparsers.add_parser("set", help="Update one marker on one plan entry.")
    set_marker.add_argument("address")
    set_marker.add_argument("field", help="recon, deps, impl, verify, functional, or a supported alias")
    set_marker.add_argument("status", help="One of ✅, ☑️, ❌, ❓")
    set_marker.add_argument("--name", default="", help="Name for Reconstructed/Reimplemented marker.")
    set_marker.add_argument("--file", default="", help="File for Reimplemented marker.")
    set_marker.add_argument("--target", default="", help="Functional target for Functional-equivalent marker.")
    set_marker.add_argument("--provider", default="", help="Provider text for external/runtime implementations.")
    set_marker.add_argument(
        "--evidence",
        default="",
        help="Required when setting ✅ or ☑️. Evidence is printed but not stored in the plan.",
    )
    set_marker.add_argument("--dry-run", action="store_true")
    set_marker.set_defaults(func=command_set)

    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = build_parser()
    args = parser.parse_args(argv)
    plan_path = Path(args.plan)
    if not plan_path.exists():
        print(f"Plan file not found: {plan_path}", file=sys.stderr)
        return 2

    try:
        doc = PlanDocument.load(plan_path)
        return args.func(doc, args)
    except ValueError as exc:
        print(str(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
