#!/usr/bin/env python3
"""Join reconstruction_priority_list.md with RECOIL_PLAN.md markers."""

from __future__ import annotations

import argparse
from pathlib import Path
import re

from recoil_plan import (
    ACCEPTED_STATUSES,
    BINARY_LANE,
    DONE_STATUS,
    PlanDocument,
    PlanEntry,
    blocker_field,
    normalize_address,
    status_summary,
)
from recoil_tooling import configure_stdio


ROOT = Path(__file__).resolve().parents[1]
PRIORITY_PATH = ROOT / "temp" / "reconstruction_priority_list.md"
PLAN_PATH = ROOT / ".agent" / "RECOIL_PLAN.md"

ROW_RE = re.compile(r"\| (\d+) \| `(0x[0-9a-fA-F]+)` \| `([^`]+)` \| (\d+) \|")


def bucket_for_rank(rank: int) -> str:
    if rank <= 660:
        return "P0"
    if rank <= 1231:
        return "P1"
    return "P2+"


def parse_priority(path: Path) -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        match = ROW_RE.match(line)
        if not match:
            continue
        rank = int(match.group(1))
        rows.append(
            {
                "rank": rank,
                "bucket": bucket_for_rank(rank),
                "address": normalize_address(match.group(2)),
                "name": match.group(3),
                "authored_callees": int(match.group(4)),
            }
        )
    return rows


def source_ready(entry: PlanEntry | None) -> bool:
    return bool(
        entry
        and entry.reconstructed_status in ACCEPTED_STATUSES
        and entry.source_dependencies_status in ACCEPTED_STATUSES
    )


def mark(status: str | None) -> str:
    if status == DONE_STATUS:
        return "OK"
    if status == "☑️":
        return "LIM"
    if status in {"❓", "?"}:
        return "??"
    return "--"


def entry_blocker(entry: PlanEntry | None, *, lane: str | None = None) -> str:
    if entry is None:
        return "missing"
    if lane is None:
        return blocker_field(entry) or "none"
    return blocker_field(entry, lane=lane) or "none"


def row_matches_filters(row: dict[str, object], entry: PlanEntry | None, args: argparse.Namespace) -> bool:
    if row["bucket"] != args.bucket:
        return False
    if args.pending_impl and entry_blocker(entry) != "impl":
        return False
    if args.pending_functional and entry_blocker(entry) != "functional":
        return False
    if (args.pending_binary or args.pending_verify) and entry_blocker(entry, lane=BINARY_LANE) != "verify":
        return False
    if args.file_hint:
        hint = f"{row['name']} {entry.reimplemented_file if entry else ''}"
        if args.file_hint.lower() not in hint.lower():
            return False
    return True


def print_summary(rows: list[dict[str, object]], plan: dict[str, PlanEntry]) -> None:
    for bucket in ("P0", "P1", "P2+"):
        sub = [row for row in rows if row["bucket"] == bucket]
        entries = [plan.get(row["address"]) for row in sub]
        source_ready_count = sum(1 for entry in entries if source_ready(entry))
        impl_done = sum(1 for entry in entries if entry and entry.reimplemented_status == DONE_STATUS)
        functional_done = sum(
            1 for entry in entries if entry and entry.functional_equivalent_status == DONE_STATUS
        )
        binary_done = sum(1 for entry in entries if entry and entry.binary_verified_status == DONE_STATUS)
        pending_impl = sum(1 for entry in entries if entry_blocker(entry) == "impl")
        pending_functional = sum(1 for entry in entries if entry_blocker(entry) == "functional")
        pending_binary = sum(1 for entry in entries if entry_blocker(entry, lane=BINARY_LANE) == "verify")
        print(
            f"{bucket}: total={len(sub)} source_ready={source_ready_count} "
            f"impl_done={impl_done} functional_done={functional_done} binary_done={binary_done} "
            f"pending_impl={pending_impl} pending_functional={pending_functional} "
            f"pending_binary={pending_binary}"
        )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bucket", choices=["P0", "P1", "P2+"], default="P0")
    parser.add_argument("--pending-impl", action="store_true")
    parser.add_argument("--pending-functional", action="store_true")
    parser.add_argument("--pending-binary", action="store_true")
    parser.add_argument(
        "--pending-verify",
        action="store_true",
        help="Compatibility alias for --pending-binary.",
    )
    parser.add_argument("--file-hint", default="", help="Substring filter on priority name or source file.")
    parser.add_argument("--limit", type=int, default=30)
    parser.add_argument("--summary", action="store_true")
    parser.add_argument("--plan", default=str(PLAN_PATH))
    parser.add_argument("--priority", default=str(PRIORITY_PATH))
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    plan_doc = PlanDocument.load(Path(args.plan))
    plan = plan_doc.entries
    rows = parse_priority(Path(args.priority))

    if args.summary:
        print_summary(rows, plan)
        return 0

    shown = 0
    for row in rows:
        entry = plan.get(row["address"])
        if not row_matches_filters(row, entry, args):
            continue
        blocker = entry_blocker(entry, lane=BINARY_LANE)
        if entry is None:
            status = "recon=-- deps=-- impl=-- functional=-- binary=--"
            file_text = "pending"
        else:
            status = (
                f"recon={mark(entry.reconstructed_status)} "
                f"deps={mark(entry.source_dependencies_status)} "
                f"impl={mark(entry.reimplemented_status)} "
                f"functional={mark(entry.functional_equivalent_status)} "
                f"binary={mark(entry.binary_verified_status)}"
            )
            file_text = entry.reimplemented_file or "pending"
        print(
            f"{int(row['rank']):4d} {row['address']} {row['name']} "
            f"{status} blocker={blocker} file={file_text}"
        )
        shown += 1
        if shown >= args.limit:
            break
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
