"""Repair absent primary-entry bookkeeping at tier X, accepting no evidence."""
from __future__ import annotations

import argparse
from copy import deepcopy
import json
from pathlib import Path
import sys

from _recoil.lib.progress import DEFAULT_PROGRESS_PATH, ProgressStore, validate_owner_invariants
from _recoil.lib.storage_proof import require
from _recoil.lib.tooling import configure_stdio


def repair(data, payload):
    require(isinstance(payload, dict) and set(payload) == {"reviewed", "reason", "current_owners"}
            and payload["reviewed"] is True and isinstance(payload["reason"], str) and payload["reason"].strip(),
            "repair requires an exact reviewed current-owner snapshot and reason")
    snapshots = payload["current_owners"]
    require(isinstance(snapshots, dict) and snapshots, "repair owner census is empty")
    changes = []
    for owner_id, snapshot in snapshots.items():
        owner = data["owners"].get(owner_id)
        require(owner == snapshot and isinstance(owner, dict), "owner snapshot changed")
        require(owner.get("binary") == "recoil" and owner.get("kind") != "provider-boundary",
                "repair selects existing authored owners only")
        entries = owner.setdefault("reimplementation", {}).setdefault("entries", {})
        for edge in owner.get("relationships", []):
            if edge.get("kind") not in {"primary-function", "primary-data"}:
                continue
            identity = edge["symbol_id"]
            symbol = data["symbols"].get(identity, {})
            kind = edge["kind"].removeprefix("primary-")
            require(symbol.get("kind") == kind and symbol.get("address") == edge.get("address"),
                    "primary member identity/address is unresolved")
            if identity not in entries:
                entries[identity] = {"kind": kind, "tier": "X", "evidence_ids": []}
                changes.append({"owner_id": owner_id, "symbol_id": identity, "tier": "X"})
    require(bool(changes), "selected owners have no missing tier entries")
    validate_owner_invariants(data)
    return changes


def build_parser():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--progress", type=Path, default=DEFAULT_PROGRESS_PATH)
    parser.add_argument("--payload-file", type=Path, required=True)
    parser.add_argument("--expected-revision", type=int, required=True)
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument("--dry-run", action="store_true")
    modes.add_argument("--apply", action="store_true")
    parser.add_argument("--json", action="store_true")
    return parser


def main(argv=None):
    configure_stdio()
    args = build_parser().parse_args(argv)
    try:
        payload = json.loads(args.payload_file.read_text(encoding="utf-8-sig"))
        changes = []
        def transform(data):
            changes.extend(repair(data, deepcopy(payload)))
        result = ProgressStore(args.progress).mutate(transform, expected_revision=args.expected_revision, apply=args.apply)
        print(json.dumps({"repairs": changes, "accepts_facts": False, "commit": result.to_dict()}, indent=2))
        return 0
    except (ValueError, RuntimeError, OSError) as exc:
        print(f"owner entry repair error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
