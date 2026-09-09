"""Review and atomically replace existing symbol and owner display names only."""
from __future__ import annotations

import argparse
from copy import deepcopy
import json
from pathlib import Path
import sys

from _recoil.lib.progress import DEFAULT_PROGRESS_PATH, ProgressError, ProgressStore
from _recoil.lib.tooling import REPO_ROOT, configure_stdio


def plan_renames(document, payload):
    required = {"schema", "reviewed", "binary", "renames"}
    if (not isinstance(payload, dict) or not required <= set(payload)
            or set(payload) - required - {"owner_renames"}
            or payload["schema"] != "recoil-symbol-names-v1" or payload["reviewed"] is not True):
        raise ProgressError("expected reviewed recoil-symbol-names-v1 payload: schema, reviewed, binary, renames, optional owner_renames")
    binary = payload["binary"]
    if binary not in {"recoil", "messages"}:
        raise ProgressError("unknown binary")
    rows = payload["renames"]
    owner_rows = payload.get("owner_renames", [])
    if not isinstance(rows, list) or not isinstance(owner_rows, list) or not (rows or owner_rows):
        raise ProgressError("renames and optional owner_renames must be arrays with at least one rename")
    proposed = deepcopy(document.data)
    seen = set()
    for row in rows:
        if not isinstance(row, dict) or set(row) != {"symbol_id", "expected_name", "name", "reason"}:
            raise ProgressError("each rename requires exactly symbol_id, expected_name, name, reason")
        if any(not isinstance(row[k], str) or not row[k].strip() for k in row):
            raise ProgressError("rename fields must be non-empty strings")
        identity = row["symbol_id"]
        if identity in seen:
            raise ProgressError(f"duplicate symbol identity: {identity}")
        seen.add(identity)
        symbol = proposed.get("symbols", {}).get(identity)
        if not isinstance(symbol, dict) or symbol.get("binary") != binary or not identity.startswith(binary + ":"):
            raise ProgressError(f"unknown or wrong-binary symbol: {identity}")
        if symbol.get("navigation_name") != row["expected_name"]:
            raise ProgressError(f"navigation name changed for {identity}")
        name = row["name"]
        if name != name.strip() or any(ord(c) < 32 or ord(c) == 127 for c in name):
            raise ProgressError("navigation name must be trimmed and contain no control characters")
        if name == row["expected_name"]:
            raise ProgressError(f"unchanged navigation name: {identity}")
        symbol["navigation_name"] = name
    seen_owners = set()
    for row in owner_rows:
        if not isinstance(row, dict) or set(row) != {"owner_id", "address", "expected_name", "name", "reason"}:
            raise ProgressError("owner rename requires exactly owner_id, address, expected_name, name, reason")
        if any(not isinstance(row[k], str) or not row[k].strip()
               for k in ("owner_id", "expected_name", "name", "reason")):
            raise ProgressError("owner rename fields must be non-empty strings")
        address = row["address"]
        if address is not None and (not isinstance(address, str) or not address):
            raise ProgressError("owner address must be null or an existing address metadata key")
        key = (row["owner_id"], address)
        if key in seen_owners:
            raise ProgressError("duplicate owner name location")
        seen_owners.add(key)
        owner = proposed.get("owners", {}).get(row["owner_id"])
        if (not isinstance(owner, dict) or owner.get("binary") != binary
                or not row["owner_id"].startswith(binary + ":owner:")):
            raise ProgressError("unknown or wrong-binary owner")
        location = owner if address is None else owner.get("address_metadata", {}).get(address)
        if not isinstance(location, dict) or location.get("name") != row["expected_name"]:
            raise ProgressError("owner name location is missing or changed")
        name = row["name"]
        if name != name.strip() or any(ord(c) < 32 or ord(c) == 127 for c in name):
            raise ProgressError("owner name must be trimmed and contain no control characters")
        if name == row["expected_name"]:
            raise ProgressError("unchanged owner name")
        location["name"] = name
    return proposed


def main(argv=None):
    configure_stdio()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--payload-file", type=Path, required=True)
    parser.add_argument("--progress", type=Path, default=DEFAULT_PROGRESS_PATH)
    parser.add_argument("--expected-revision", type=int, required=True)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--dry-run", action="store_true")
    mode.add_argument("--apply", action="store_true")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args(argv)
    try:
        path = args.payload_file.resolve()
        if not path.is_relative_to((REPO_ROOT / "build").resolve()):
            raise ProgressError("reviewed payload must be under workspace build/")
        payload = json.loads(path.read_text(encoding="utf-8-sig"))
        store = ProgressStore(args.progress)
        document = store.load()
        if document.revision != args.expected_revision:
            raise ProgressError(f"tracker revision changed: expected {args.expected_revision}, found {document.revision}")
        proposed = plan_renames(document, payload)
        commit = store.commit(proposed, expected_revision=args.expected_revision, apply=args.apply)
        print(json.dumps(dict(kind="symbol-navigation-name-batch", acceptance_effects=[],
            renames=payload["renames"], owner_renames=payload.get("owner_renames", []),
            commit=commit.to_dict()), indent=2))
        return 0
    except (OSError, ValueError, ProgressError) as exc:
        print(f"symbol name error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
