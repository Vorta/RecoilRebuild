from __future__ import annotations

import argparse
from pathlib import Path
import subprocess
import sys

from recoil_binja import create_shared_budget_file, env_with_shared_budget
from recoil_claim import (
    DEFAULT_CLAIMS_DIR,
    claim_addresses,
    first_unclaimed_unfinished,
    print_claim,
    read_claim,
)
from recoil_plan import (
    FIELD_LABELS,
    FUNCTIONAL_LANE,
    LANE_CHOICES,
    PlanDocument,
    PlanEntry,
    blocker_field,
    normalize_address,
    normalize_lane,
    status_summary,
)
from recoil_tooling import REPO_ROOT, configure_stdio, hidden_creation_flags


def run_step(
    label: str,
    command: list[str],
    *,
    enabled: bool = True,
    env: dict[str, str] | None = None,
) -> int:
    print()
    print(f"## {label}")
    print("command: " + " ".join(command))
    if not enabled:
        print("- skipped")
        return 0
    completed = subprocess.run(
        command,
        cwd=REPO_ROOT,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        encoding="utf-8",
        errors="replace",
        creationflags=hidden_creation_flags(),
    )
    if completed.stdout:
        print(completed.stdout.rstrip())
    return completed.returncode


def select_entry(args: argparse.Namespace, doc: PlanDocument) -> tuple[PlanEntry, bool]:
    if args.address:
        normalized = normalize_address(args.address)
        entry = doc.entries.get(normalized)
        if entry is None:
            raise ValueError(f"Address not found in plan: {normalized}")
        return entry, False
    if args.claim_next:
        entry = first_unclaimed_unfinished(doc, Path(args.claims_dir), lane=args.lane)
        if entry is None:
            raise ValueError("No unclaimed unfinished plan entries found.")
        return entry, True
    entry = doc.first_unfinished(lane=args.lane)
    if entry is None:
        raise ValueError("No unfinished plan entries found.")
    return entry, False


def print_header(entry: PlanEntry, *, lane: str) -> None:
    lane = normalize_lane(lane)
    blocker = blocker_field(entry, lane=lane) or "none"
    print("# Recoil Task Packet")
    print()
    print(f"Address: {entry.address}")
    print(f"Milestone: {entry.milestone}")
    print(f"Name: {entry.reimplemented_name or entry.reconstructed_name or 'pending'}")
    print(f"File: {entry.reimplemented_file or 'pending'}")
    print(f"Plan status: {status_summary(entry)}")
    print(f"Lane: {lane}")
    print(f"Next blocker: {blocker} ({FIELD_LABELS.get(blocker, blocker)})")


def print_claim_state(entry: PlanEntry, args: argparse.Namespace, *, should_claim: bool) -> None:
    print()
    print("## Claim")
    claims_dir = Path(args.claims_dir)
    if should_claim:
        claims = claim_addresses(
            [entry.address],
            owner=args.owner,
            ttl_hours=args.ttl_hours,
            reason=args.reason,
            claims_dir=claims_dir,
        )
        for claim in claims:
            print_claim(claim)
        return

    claim = read_claim(claims_dir, entry.address)
    if claim is None:
        print("- unclaimed")
        print(f"- claim: python tools/recoil_claim.py claim {entry.address} --owner <name>")
    else:
        print_claim(claim)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Print a launch packet for one Recoil reconstruction task."
    )
    parser.add_argument("--address", help="Use a specific address instead of the first unfinished entry.")
    parser.add_argument("--plan", default=".agent/RECOIL_PLAN.md")
    parser.add_argument("--claims-dir", default=str(DEFAULT_CLAIMS_DIR))
    parser.add_argument("--claim-next", action="store_true", help="Select and claim the first unclaimed unfinished entry.")
    parser.add_argument("--owner", default="", help="Claim owner for --claim-next.")
    parser.add_argument("--ttl-hours", type=float, default=12.0)
    parser.add_argument("--reason", default="")
    parser.add_argument(
        "--lane",
        choices=LANE_CHOICES,
        default=FUNCTIONAL_LANE,
        help="functional stops at Functional-equivalent; binary also requires Binary-safe.",
    )
    parser.add_argument("--depth", type=int, default=1)
    parser.add_argument("--no-binja", action="store_true", help="Skip Binary Ninja-dependent checks.")
    parser.add_argument("--skip-doctor", action="store_true", help="Skip the quick doctor preflight.")
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        if args.claim_next and not args.owner:
            raise ValueError("--claim-next requires --owner")
        args.lane = normalize_lane(args.lane)
        doc = PlanDocument.load(Path(args.plan))
        entry, should_claim = select_entry(args, doc)
        print_header(entry, lane=args.lane)
        print_claim_state(entry, args, should_claim=should_claim)
        budget_file = create_shared_budget_file() if not args.no_binja else None
        binja_env = env_with_shared_budget(budget_file) if budget_file is not None else None

        try:
            doctor_cmd = [sys.executable, "tools/recoil_doctor.py", "--quick"]
            if not args.no_binja:
                doctor_cmd.append("--binja")
            run_step("Doctor", doctor_cmd, enabled=not args.skip_doctor, env=binja_env)

            status_cmd = [
                sys.executable,
                "tools/recoil_status.py",
                entry.address,
                "--lane",
                args.lane,
                "--depth",
                str(args.depth),
            ]
            if args.no_binja:
                status_cmd.append("--no-frontier")
            run_step("Status", status_cmd, env=binja_env)

            run_step(
                "Source Map",
                [
                    sys.executable,
                    "tools/recoil_source_file_map.py",
                    "--check",
                    "docs/reconstruction/source_file_map.md",
                ],
            )
            run_step("Functional Coverage", [sys.executable, "tools/recoil_functional_verify.py", "--list"])
            run_step("VC Coverage", [sys.executable, "tools/recoil_vc6_verify.py", "--explain-missing", entry.address])
        finally:
            if budget_file is not None:
                try:
                    budget_file.unlink()
                except FileNotFoundError:
                    pass

        print()
        print("## Next Commands")
        print(f"- python tools/recoil_plan_cli.py show {entry.address}")
        print(f"- python tools/recoil_frontier.py {entry.address} --depth {args.depth}")
        print(f"- python tools/recoil_vc6_verify.py {entry.address}")
    except (OSError, ValueError) as exc:
        print(str(exc), file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
