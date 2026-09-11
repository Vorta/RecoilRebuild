"""Register reviewed call-only ICF eligibility without accepting reconstruction."""
from __future__ import annotations

import argparse
from copy import deepcopy
import json
from pathlib import Path
import struct
import sys

from _recoil.lib.call_only_icf import FIELD, require, validate_contract, validate_source, validate_caller_authority
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH, ProgressError, ProgressStore, address_value
from _recoil.lib.tooling import configure_stdio


def stage(data, payload):
    require(isinstance(payload, dict) and set(payload) == {"reviewed", "reason", "expected_alias", "contract"}
            and payload["reviewed"] is True and isinstance(payload["reason"], str)
            and payload["reason"].strip(), "exact reviewed registration payload required")
    contract = payload["contract"]
    alias = validate_contract(data, contract)
    validate_caller_authority(data, contract)
    require(alias == payload["expected_alias"] and FIELD not in alias, "alias changed or already bound")
    validate_source(data, contract)
    # The immutable opcode/operand must agree before any tracker mutation.
    from _recoil.commands.live_byte_verify import _pe_bytes, DEFAULT_REFERENCE
    site = address_value(contract["call_address"])
    code = _pe_bytes(DEFAULT_REFERENCE, site, 5)
    require(code[0] == 0xE8 and site + 5 + struct.unpack_from('<i', code, 1)[0]
            == address_value(contract["physical_address"]), "retail call target differs")
    result = deepcopy(data)
    result["symbols"][contract["physical_symbol_id"]]["logical_aliases"][contract["logical_id"]][FIELD] = deepcopy(contract)
    return result


def main(argv=None):
    configure_stdio()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--payload-file", type=Path, required=True)
    parser.add_argument("--expected-revision", type=int, required=True)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--dry-run", action="store_true")
    mode.add_argument("--apply", action="store_true")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args(argv)
    try:
        payload = json.loads(args.payload_file.read_text(encoding="utf-8-sig"))
        store = ProgressStore(DEFAULT_PROGRESS_PATH)
        document = store.load()
        require(document.revision == args.expected_revision, "tracker revision changed")
        proposed = stage(document.data, payload)
        commit = store.commit(proposed, expected_revision=args.expected_revision, apply=args.apply)
        print(json.dumps(dict(kind="call-only-icf-eligibility", contract=payload["contract"],
            accepts_facts=False, commit=commit.to_dict()), indent=2))
        return 0
    except (OSError, ValueError, KeyError, TypeError, ProgressError) as exc:
        print(f"call-only ICF registration error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
