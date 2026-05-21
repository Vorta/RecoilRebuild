from __future__ import annotations

import argparse
from pathlib import Path
import subprocess
import sys

from recoil_claim import DEFAULT_CLAIMS_DIR, read_claim, release_addresses
from recoil_functional_verify import DEFAULT_MANIFEST_DIR as FUNCTIONAL_MANIFEST_DIR
from recoil_functional_verify import find_target as find_functional_target
from recoil_functional_verify import load_manifests as load_functional_manifests
from recoil_groups_audit import audit_group, parse_groups
from recoil_plan import PlanDocument, normalize_address, status_summary
from recoil_tooling import REPO_ROOT, configure_stdio, hidden_creation_flags
from recoil_vc6_verify import DEFAULT_MANIFEST_DIR as VC_MANIFEST_DIR
from recoil_vc6_verify import covering_targets, load_manifests


def print_doctor() -> None:
    command = [sys.executable, "tools/recoil_doctor.py", "--quick", "--binja"]
    print()
    print("## Doctor")
    print("command: " + " ".join(command))
    completed = subprocess.run(
        command,
        cwd=REPO_ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        encoding="utf-8",
        errors="replace",
        creationflags=hidden_creation_flags(),
    )
    if completed.stdout:
        print(completed.stdout.rstrip())


def print_address_report(address: str, args: argparse.Namespace, doc: PlanDocument) -> None:
    normalized = normalize_address(address)
    entry = doc.entries.get(normalized)
    print()
    print(f"## {normalized}")
    if entry is None:
        print("- not found in plan")
        return

    print(f"- milestone: {entry.milestone}")
    print(f"- name: {entry.reimplemented_name or entry.reconstructed_name or 'pending'}")
    print(f"- file: {entry.reimplemented_file or 'pending'}")
    print(f"- status: {status_summary(entry, include_functional=True)}")

    claim = read_claim(Path(args.claims_dir), normalized)
    if claim is None:
        print("- claim: unclaimed")
    else:
        print(f"- claim: owner={claim.owner} expires={claim.expires_at_utc.isoformat()}")

    groups_path = Path(args.groups)
    if groups_path.exists():
        groups = parse_groups(groups_path.read_text(encoding="utf-8"))
        matches = [group for group in groups if normalized in group.addresses]
        if matches:
            group_text = []
            for group in matches:
                audit = audit_group(group, doc.entries)
                group_text.append(f"{group.title} ({audit.recommendation})")
            print("- groups: " + "; ".join(group_text))
        else:
            print("- groups: none")

    vc_matches = covering_targets(load_manifests(Path(args.vc_manifest_dir)), normalized)
    if vc_matches:
        for manifest, function in vc_matches:
            print(f"- vc: {manifest.name} symbol={function.symbol}")
            if args.include_artifacts:
                artifact_dir = REPO_ROOT / "build" / "vc6-verify" / manifest.name / "verify"
                print(f"  artifacts: {artifact_dir if artifact_dir.exists() else 'not present'}")
    else:
        print("- vc: none")

    functional_targets = load_functional_manifests(Path(args.functional_manifest_dir))
    try:
        target = find_functional_target(functional_targets, normalized)
        print(f"- functional: {target.name} smokes={len(target.smoke_tests)}")
    except ValueError:
        print("- functional: none")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Print a reconstruction handoff report for one or more addresses.")
    parser.add_argument("address", nargs="+")
    parser.add_argument("--plan", default=".agent/RECOIL_PLAN.md")
    parser.add_argument("--groups", default=".agent/IMPLEMENTATION_GROUPS.md")
    parser.add_argument("--claims-dir", default=str(DEFAULT_CLAIMS_DIR))
    parser.add_argument("--vc-manifest-dir", default=str(VC_MANIFEST_DIR))
    parser.add_argument("--functional-manifest-dir", default=str(FUNCTIONAL_MANIFEST_DIR))
    parser.add_argument("--run-doctor", action="store_true")
    parser.add_argument("--include-artifacts", action="store_true")
    parser.add_argument("--release", action="store_true", help="Release these addresses after printing the report.")
    parser.add_argument("--token", default="", help="Claim token required by --release.")
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        doc = PlanDocument.load(Path(args.plan))
        print("# Recoil Handoff")
        if args.run_doctor:
            print_doctor()
        for address in args.address:
            print_address_report(address, args, doc)
        if args.release:
            released = release_addresses(args.address, token=args.token, claims_dir=Path(args.claims_dir))
            print()
            print("## Released")
            for claim in released:
                print(f"- {claim.address} owner={claim.owner}")
    except (OSError, ValueError) as exc:
        print(str(exc), file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

