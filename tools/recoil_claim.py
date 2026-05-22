from __future__ import annotations

import argparse
from dataclasses import dataclass
from datetime import UTC, datetime, timedelta
import json
import os
from pathlib import Path
import secrets
import socket
import sys
from typing import Any

from recoil_plan import (
    FUNCTIONAL_LANE,
    LANE_CHOICES,
    PlanDocument,
    PlanEntry,
    blocker_field,
    normalize_address,
    normalize_lane,
)
from recoil_tooling import REPO_ROOT, configure_stdio


DEFAULT_CLAIMS_DIR = REPO_ROOT / ".agent" / "claims" / "functions"
DEFAULT_TTL_HOURS = 12.0
SCHEMA_VERSION = 1


@dataclass(frozen=True)
class Claim:
    address: str
    owner: str
    token: str
    pid: int
    host: str
    cwd: str
    created_at_utc: datetime
    expires_at_utc: datetime
    reason: str = ""

    @property
    def is_expired(self) -> bool:
        return datetime.now(UTC) >= self.expires_at_utc


class ClaimError(ValueError):
    pass


def utc_now() -> datetime:
    return datetime.now(UTC)


def format_utc(value: datetime) -> str:
    normalized = value.astimezone(UTC).replace(microsecond=0)
    return normalized.isoformat().replace("+00:00", "Z")


def parse_utc(value: str) -> datetime:
    text = value.strip()
    if text.endswith("Z"):
        text = text[:-1] + "+00:00"
    parsed = datetime.fromisoformat(text)
    if parsed.tzinfo is None:
        return parsed.replace(tzinfo=UTC)
    return parsed.astimezone(UTC)


def claim_path(claims_dir: Path, address: str) -> Path:
    return claims_dir / f"{normalize_address(address)}.json"


def claim_to_json(claim: Claim) -> str:
    data = {
        "schema_version": SCHEMA_VERSION,
        "address": claim.address,
        "owner": claim.owner,
        "token": claim.token,
        "pid": claim.pid,
        "host": claim.host,
        "cwd": claim.cwd,
        "created_at_utc": format_utc(claim.created_at_utc),
        "expires_at_utc": format_utc(claim.expires_at_utc),
        "reason": claim.reason,
    }
    return json.dumps(data, indent=2, sort_keys=True) + "\n"


def _require_str(data: dict[str, Any], key: str, *, path: Path) -> str:
    value = data.get(key)
    if not isinstance(value, str) or not value:
        raise ClaimError(f"{path}: expected non-empty string field {key!r}")
    return value


def load_claim(path: Path) -> Claim:
    try:
        with path.open("r", encoding="utf-8") as handle:
            data = json.load(handle)
    except OSError as exc:
        raise ClaimError(f"cannot read claim {path}: {exc}") from exc
    except json.JSONDecodeError as exc:
        raise ClaimError(f"cannot parse claim {path}: {exc}") from exc

    if not isinstance(data, dict):
        raise ClaimError(f"{path}: expected JSON object")
    schema = data.get("schema_version")
    if schema != SCHEMA_VERSION:
        raise ClaimError(f"{path}: unsupported schema_version {schema!r}")

    pid = data.get("pid")
    if not isinstance(pid, int):
        raise ClaimError(f"{path}: expected integer field 'pid'")

    return Claim(
        address=normalize_address(_require_str(data, "address", path=path)),
        owner=_require_str(data, "owner", path=path),
        token=_require_str(data, "token", path=path),
        pid=pid,
        host=_require_str(data, "host", path=path),
        cwd=_require_str(data, "cwd", path=path),
        created_at_utc=parse_utc(_require_str(data, "created_at_utc", path=path)),
        expires_at_utc=parse_utc(_require_str(data, "expires_at_utc", path=path)),
        reason=str(data.get("reason", "")),
    )


def read_claim(claims_dir: Path, address: str) -> Claim | None:
    path = claim_path(claims_dir, address)
    if not path.exists():
        return None
    return load_claim(path)


def list_claims(claims_dir: Path) -> list[Claim]:
    if not claims_dir.exists():
        return []
    claims: list[Claim] = []
    for path in sorted(claims_dir.glob("0x*.json")):
        claims.append(load_claim(path))
    return claims


def create_claim(
    address: str,
    *,
    owner: str,
    token: str,
    reason: str,
    expires_at: datetime,
    claims_dir: Path,
) -> Claim:
    normalized = normalize_address(address)
    now = utc_now()
    return Claim(
        address=normalized,
        owner=owner,
        token=token,
        pid=os.getpid(),
        host=socket.gethostname(),
        cwd=str(Path.cwd()),
        created_at_utc=now,
        expires_at_utc=expires_at,
        reason=reason,
    )


def write_claim_exclusive(claim: Claim, claims_dir: Path) -> Path:
    claims_dir.mkdir(parents=True, exist_ok=True)
    path = claim_path(claims_dir, claim.address)
    try:
        with path.open("x", encoding="utf-8") as handle:
            handle.write(claim_to_json(claim))
    except FileExistsError as exc:
        existing = load_claim(path)
        raise ClaimError(
            f"{claim.address} is already claimed by {existing.owner} "
            f"until {format_utc(existing.expires_at_utc)}"
        ) from exc
    return path


def prune_stale_claims(claims_dir: Path) -> list[Claim]:
    pruned: list[Claim] = []
    for claim in list_claims(claims_dir):
        if not claim.is_expired:
            continue
        path = claim_path(claims_dir, claim.address)
        try:
            path.unlink()
        except FileNotFoundError:
            pass
        pruned.append(claim)
    return pruned


def claim_addresses(
    addresses: list[str],
    *,
    owner: str,
    ttl_hours: float,
    reason: str,
    claims_dir: Path,
) -> list[Claim]:
    if not owner.strip():
        raise ClaimError("owner is required")
    if ttl_hours <= 0:
        raise ClaimError("ttl-hours must be greater than zero")

    normalized = []
    for address in addresses:
        item = normalize_address(address)
        if item not in normalized:
            normalized.append(item)
    if not normalized:
        raise ClaimError("at least one address is required")

    prune_stale_claims(claims_dir)
    token = secrets.token_urlsafe(18)
    expires_at = utc_now() + timedelta(hours=ttl_hours)
    created: list[Claim] = []
    try:
        for address in normalized:
            claim = create_claim(
                address,
                owner=owner,
                token=token,
                reason=reason,
                expires_at=expires_at,
                claims_dir=claims_dir,
            )
            write_claim_exclusive(claim, claims_dir)
            created.append(claim)
    except ClaimError:
        for claim in created:
            try:
                claim_path(claims_dir, claim.address).unlink()
            except FileNotFoundError:
                pass
        raise
    return created


def release_addresses(addresses: list[str], *, token: str, claims_dir: Path) -> list[Claim]:
    if not token:
        raise ClaimError("release requires --token")
    claims: list[Claim] = []
    for address in addresses:
        normalized = normalize_address(address)
        path = claim_path(claims_dir, normalized)
        if not path.exists():
            raise ClaimError(f"{normalized} is not claimed")
        claim = load_claim(path)
        if claim.token != token:
            raise ClaimError(f"{normalized} token mismatch")
        claims.append(claim)
    for claim in claims:
        claim_path(claims_dir, claim.address).unlink()
    return claims


def first_unclaimed_unfinished(
    doc: PlanDocument,
    claims_dir: Path,
    *,
    lane: str = FUNCTIONAL_LANE,
) -> PlanEntry | None:
    lane = normalize_lane(lane)
    prune_stale_claims(claims_dir)
    for address in doc.order:
        entry = doc.entries[address]
        if blocker_field(entry, lane=lane) and read_claim(claims_dir, address) is None:
            return entry
    return None


def print_claim(claim: Claim) -> None:
    state = "expired" if claim.is_expired else "active"
    reason = f" reason={claim.reason}" if claim.reason else ""
    print(
        f"{claim.address} owner={claim.owner} state={state} "
        f"expires={format_utc(claim.expires_at_utc)} token={claim.token}{reason}"
    )


def command_claim(args: argparse.Namespace) -> int:
    claims = claim_addresses(
        args.address,
        owner=args.owner,
        ttl_hours=args.ttl_hours,
        reason=args.reason,
        claims_dir=Path(args.claims_dir),
    )
    for claim in claims:
        print_claim(claim)
    return 0


def command_release(args: argparse.Namespace) -> int:
    released = release_addresses(args.address, token=args.token, claims_dir=Path(args.claims_dir))
    for claim in released:
        print(f"released {claim.address} owner={claim.owner}")
    return 0


def command_status(args: argparse.Namespace) -> int:
    claims_dir = Path(args.claims_dir)
    if args.address and args.owner:
        raise ClaimError("status --owner cannot be combined with an address")
    if args.address:
        claim = read_claim(claims_dir, args.address)
        if claim is None:
            print(f"{normalize_address(args.address)} unclaimed")
            return 0
        print_claim(claim)
        return 0

    claims = list_claims(claims_dir)
    if args.owner:
        claims = [claim for claim in claims if claim.owner == args.owner]
    if not claims:
        print("no claims")
        return 0
    for claim in claims:
        print_claim(claim)
    return 0


def command_prune_stale(args: argparse.Namespace) -> int:
    pruned = prune_stale_claims(Path(args.claims_dir))
    for claim in pruned:
        print(f"pruned {claim.address} owner={claim.owner}")
    if not pruned:
        print("no stale claims")
    return 0


def command_next(args: argparse.Namespace) -> int:
    doc = PlanDocument.load(Path(args.plan))
    claims_dir = Path(args.claims_dir)
    lane = normalize_lane(args.lane)
    entry = first_unclaimed_unfinished(doc, claims_dir, lane=lane)
    if entry is None:
        print("No unclaimed unfinished plan entries found.")
        return 0
    print(
        f"{entry.address} {entry.milestone} blocker={blocker_field(entry, lane=lane)} "
        f"name={entry.reimplemented_name or entry.reconstructed_name or 'pending'}"
    )
    print(f"show_command=python tools/recoil_plan_cli.py show {entry.address}")
    if args.claim:
        claims = claim_addresses(
            [entry.address],
            owner=args.owner,
            ttl_hours=args.ttl_hours,
            reason=args.reason,
            claims_dir=claims_dir,
        )
        print("claim:")
        for claim in claims:
            print_claim(claim)
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Claim Recoil reconstruction functions by original address.")
    parser.add_argument("--claims-dir", default=str(DEFAULT_CLAIMS_DIR))
    subparsers = parser.add_subparsers(dest="command", required=True)

    claim = subparsers.add_parser("claim", help="Atomically claim one or more function addresses.")
    claim.add_argument("address", nargs="+")
    claim.add_argument("--owner", required=True)
    claim.add_argument("--ttl-hours", type=float, default=DEFAULT_TTL_HOURS)
    claim.add_argument("--reason", default="")
    claim.set_defaults(func=command_claim)

    release = subparsers.add_parser("release", help="Release one or more addresses with the claim token.")
    release.add_argument("address", nargs="+")
    release.add_argument("--token", required=True)
    release.set_defaults(func=command_release)

    status = subparsers.add_parser("status", aliases=["list"], help="Show claim state.")
    status.add_argument("address", nargs="?")
    status.add_argument("--owner", default="", help="Only show claims owned by this owner.")
    status.set_defaults(func=command_status)

    prune = subparsers.add_parser("prune-stale", help="Remove expired claims.")
    prune.set_defaults(func=command_prune_stale)

    next_entry = subparsers.add_parser("next", help="Print the first unfinished unclaimed plan entry.")
    next_entry.add_argument("--plan", default=".agent/RECOIL_PLAN.md")
    next_entry.add_argument("--claim", action="store_true")
    next_entry.add_argument("--owner", default="")
    next_entry.add_argument("--ttl-hours", type=float, default=DEFAULT_TTL_HOURS)
    next_entry.add_argument("--reason", default="")
    next_entry.add_argument(
        "--lane",
        choices=LANE_CHOICES,
        default=FUNCTIONAL_LANE,
        help="functional stops at Functional-equivalent; binary also requires Binary-safe.",
    )
    next_entry.set_defaults(func=command_next)

    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        if args.command == "next" and args.claim and not args.owner:
            raise ClaimError("next --claim requires --owner")
        return args.func(args)
    except (OSError, ValueError, ClaimError) as exc:
        print(str(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
