from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
import re
import sys

from recoil_plan import (
    DONE_STATUS,
    LIMITED_STATUS,
    NOT_DONE_STATUS,
    UNKNOWN_STATUS,
    PlanDocument,
    PlanEntry,
    normalize_address,
    status_summary,
)


ADDRESS_RE = re.compile(r"0x[0-9A-Fa-f]{6,8}")
GROUP_HEADING_RE = re.compile(r"^### Group:\s*(.+?)\s*$")

ACCEPTED_SOURCE_DEP_STATUSES = {DONE_STATUS, LIMITED_STATUS}
ACCEPTED_VERIFY_STATUSES = {DONE_STATUS, LIMITED_STATUS}


@dataclass(frozen=True)
class ImplementationGroup:
    title: str
    section: str
    start_line: int
    end_line: int
    lines: tuple[str, ...]
    standard_heading: bool = True

    @property
    def addresses(self) -> tuple[str, ...]:
        seen: set[str] = set()
        result: list[str] = []
        in_notes = False
        for line in self.lines:
            if line == "- Notes:":
                in_notes = True
                continue
            if in_notes:
                continue
            for match in ADDRESS_RE.findall(line):
                address = normalize_address(match)
                if address not in seen:
                    seen.add(address)
                    result.append(address)
        return tuple(result)


@dataclass(frozen=True)
class GroupAudit:
    group: ImplementationGroup
    recommendation: str
    reasons: tuple[str, ...]
    missing_addresses: tuple[str, ...]
    source_pending_addresses: tuple[str, ...]
    verification_pending_addresses: tuple[str, ...]

    @property
    def address_count(self) -> int:
        return len(self.group.addresses)


def parse_groups(text: str) -> list[ImplementationGroup]:
    lines = text.splitlines()
    groups: list[ImplementationGroup] = []
    section = ""
    current_title = ""
    current_start = -1
    current_standard = True

    def close_group(end_line: int) -> None:
        nonlocal current_title, current_start, current_standard
        if current_start < 0:
            return
        groups.append(
            ImplementationGroup(
                title=current_title,
                section=section,
                start_line=current_start + 1,
                end_line=end_line,
                lines=tuple(lines[current_start:end_line]),
                standard_heading=current_standard,
            )
        )
        current_title = ""
        current_start = -1
        current_standard = True

    for index, line in enumerate(lines):
        if line == "## Active Groups":
            close_group(index)
            section = "active"
            continue
        if line == "## Completed Groups":
            close_group(index)
            section = "completed"
            continue
        if not section:
            continue

        heading = GROUP_HEADING_RE.match(line)
        if heading:
            close_group(index)
            current_title = heading.group(1)
            current_start = index
            current_standard = True
            continue

        if section == "active" and line.startswith("## "):
            close_group(index)
            current_title = line[3:].strip()
            current_start = index
            current_standard = False

    close_group(len(lines))
    return groups


def source_pending(entry: PlanEntry) -> bool:
    return (
        entry.reconstructed_status in {NOT_DONE_STATUS, UNKNOWN_STATUS, "?"}
        or entry.source_dependencies_status not in ACCEPTED_SOURCE_DEP_STATUSES
        or entry.reimplemented_status != DONE_STATUS
    )


def verification_pending(entry: PlanEntry) -> bool:
    return entry.binary_verified_status not in ACCEPTED_VERIFY_STATUSES


def audit_group(group: ImplementationGroup, plan: dict[str, PlanEntry]) -> GroupAudit:
    missing: list[str] = []
    source_pending_items: list[str] = []
    verification_pending_items: list[str] = []
    reasons: list[str] = []

    if not group.standard_heading:
        reasons.append("nonstandard heading")

    addresses = group.addresses
    if not addresses:
        reasons.append("no addresses found")

    for address in addresses:
        entry = plan.get(address)
        if entry is None:
            missing.append(address)
            continue
        if source_pending(entry):
            source_pending_items.append(address)
        if verification_pending(entry):
            verification_pending_items.append(address)

    if missing:
        reasons.append("addresses missing from plan")

    if group.section == "completed":
        if source_pending_items or verification_pending_items:
            reasons.append("completed group still has pending plan markers")
        recommendation = "needs-review" if reasons else "completed"
    elif reasons:
        recommendation = "needs-review"
    elif source_pending_items:
        recommendation = "keep-active"
    elif verification_pending_items:
        recommendation = "condense"
    else:
        recommendation = "move-completed"

    return GroupAudit(
        group=group,
        recommendation=recommendation,
        reasons=tuple(reasons),
        missing_addresses=tuple(missing),
        source_pending_addresses=tuple(source_pending_items),
        verification_pending_addresses=tuple(verification_pending_items),
    )


def audit_groups(groups: list[ImplementationGroup], plan: dict[str, PlanEntry]) -> list[GroupAudit]:
    return [audit_group(group, plan) for group in groups]


def print_summary(audits: list[GroupAudit]) -> None:
    by_section = Counter(audit.group.section for audit in audits)
    by_recommendation = Counter(audit.recommendation for audit in audits)
    groups_with_missing = sum(1 for audit in audits if audit.missing_addresses)
    groups_with_source_pending = sum(1 for audit in audits if audit.source_pending_addresses)
    groups_with_verify_pending = sum(1 for audit in audits if audit.verification_pending_addresses)
    nonstandard_groups = sum(1 for audit in audits if not audit.group.standard_heading)
    no_address_groups = sum(1 for audit in audits if not audit.group.addresses)

    print(f"groups: {len(audits)}")
    print("sections: " + " ".join(f"{key}={value}" for key, value in sorted(by_section.items())))
    print(
        "recommendations: "
        + " ".join(f"{key}={value}" for key, value in sorted(by_recommendation.items()))
    )
    print(f"groups_with_missing_plan_addresses: {groups_with_missing}")
    print(f"groups_with_source_pending: {groups_with_source_pending}")
    print(f"groups_with_verification_pending: {groups_with_verify_pending}")
    print(f"nonstandard_heading_groups: {nonstandard_groups}")
    print(f"no_address_groups: {no_address_groups}")


def format_address_list(addresses: tuple[str, ...], limit: int) -> str:
    if not addresses:
        return "-"
    shown = list(addresses[:limit])
    suffix = f" (+{len(addresses) - limit} more)" if len(addresses) > limit else ""
    return ", ".join(shown) + suffix


def print_group_report(audits: list[GroupAudit], *, limit: int, address_limit: int) -> None:
    selected = audits[:limit] if limit >= 0 else audits
    for audit in selected:
        group = audit.group
        reason_text = ", ".join(audit.reasons) if audit.reasons else "-"
        print(
            f"- {audit.recommendation}: {group.title} "
            f"({group.section}, lines {group.start_line}-{group.end_line}, "
            f"addresses={audit.address_count})"
        )
        print(f"  reasons: {reason_text}")
        if audit.missing_addresses:
            print(f"  missing: {format_address_list(audit.missing_addresses, address_limit)}")
        if audit.source_pending_addresses:
            print(
                "  source_pending: "
                f"{format_address_list(audit.source_pending_addresses, address_limit)}"
            )
        if audit.verification_pending_addresses:
            print(
                "  verification_pending: "
                f"{format_address_list(audit.verification_pending_addresses, address_limit)}"
            )
    if limit >= 0 and len(audits) > limit:
        print(f"- ... {len(audits) - limit} more")


def describe_address(plan: dict[str, PlanEntry], address: str) -> str:
    entry = plan[address]
    name = entry.reimplemented_name or entry.reconstructed_name or "pending"
    return f"{address} {name} {status_summary(entry)}"


def print_details(audits: list[GroupAudit], plan: dict[str, PlanEntry], address_limit: int) -> None:
    for audit in audits:
        print(f"{audit.group.title}: {audit.recommendation}")
        for label, addresses in (
            ("missing", audit.missing_addresses),
            ("source_pending", audit.source_pending_addresses),
            ("verification_pending", audit.verification_pending_addresses),
        ):
            if not addresses:
                continue
            print(f"  {label}:")
            for address in addresses[:address_limit]:
                if address in plan:
                    print(f"    - {describe_address(plan, address)}")
                else:
                    print(f"    - {address}")
            if len(addresses) > address_limit:
                print(f"    - ... {len(addresses) - address_limit} more")


def configure_stdio() -> None:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    if hasattr(sys.stderr, "reconfigure"):
        sys.stderr.reconfigure(encoding="utf-8")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Audit .agent/IMPLEMENTATION_GROUPS.md against RECOIL_PLAN markers."
    )
    parser.add_argument("--groups", default=".agent/IMPLEMENTATION_GROUPS.md")
    parser.add_argument("--plan", default=".agent/RECOIL_PLAN.md")
    parser.add_argument("--summary", action="store_true", help="Print summary counts first.")
    parser.add_argument(
        "--recommendation",
        choices=["keep-active", "condense", "move-completed", "needs-review", "completed"],
        help="Show only groups with one recommendation.",
    )
    parser.add_argument(
        "--section",
        choices=["active", "completed"],
        help="Show only groups from one section.",
    )
    parser.add_argument(
        "--details",
        action="store_true",
        help="Print plan names/statuses for pending and missing addresses.",
    )
    parser.add_argument("--limit", type=int, default=40, help="Maximum groups to print; use -1 for all.")
    parser.add_argument(
        "--address-limit",
        type=int,
        default=8,
        help="Maximum addresses to print per group field.",
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Return nonzero when any group needs review.",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = build_parser()
    args = parser.parse_args(argv)

    groups_path = Path(args.groups)
    plan_path = Path(args.plan)
    if not groups_path.exists():
        print(f"Implementation groups file not found: {groups_path}", file=sys.stderr)
        return 2
    if not plan_path.exists():
        print(f"Plan file not found: {plan_path}", file=sys.stderr)
        return 2

    plan = PlanDocument.load(plan_path).entries
    groups = parse_groups(groups_path.read_text(encoding="utf-8"))
    audits = audit_groups(groups, plan)

    if args.section:
        audits = [audit for audit in audits if audit.group.section == args.section]
    if args.recommendation:
        audits = [audit for audit in audits if audit.recommendation == args.recommendation]

    if args.summary:
        print_summary(audits)
    print_group_report(audits, limit=args.limit, address_limit=args.address_limit)
    if args.details:
        print_details(audits, plan, args.address_limit)

    if args.strict and any(audit.recommendation == "needs-review" for audit in audits):
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
