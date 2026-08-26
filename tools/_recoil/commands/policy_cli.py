from __future__ import annotations

import argparse
import json
from pathlib import Path

from _recoil.lib.tooling import REPO_ROOT, configure_stdio


POLICY_PATH = REPO_ROOT / "tools" / "_recoil" / "config" / "reconstruction_policies.json"
POLICY_NAMES = ("authored-order", "raw-object-extras", "parallel-arbitration")


def load_policies(path: Path = POLICY_PATH) -> dict[str, object]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if data.get("schema_version") != 1 or not isinstance(data.get("policies"), dict):
        raise ValueError(f"{path}: unsupported reconstruction policy schema")
    policies = data["policies"]
    missing = [name for name in POLICY_NAMES if name not in policies]
    if missing:
        raise ValueError(f"{path}: missing policies: {', '.join(missing)}")
    return data


def main() -> int:
    configure_stdio()
    parser = argparse.ArgumentParser(description="Inspect machine-readable reconstruction policy.")
    subparsers = parser.add_subparsers(dest="command", required=True)
    show = subparsers.add_parser("show")
    show.add_argument("policy", choices=POLICY_NAMES)
    show.add_argument("--json", action="store_true")
    args = parser.parse_args()
    data = load_policies()
    payload = {
        "schema_version": data["schema_version"],
        "name": args.policy,
        "policy": data["policies"][args.policy],
    }
    if args.json:
        print(json.dumps(payload, indent=2, sort_keys=True))
    else:
        print(f"name={args.policy}")
        print(json.dumps(payload["policy"], indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
