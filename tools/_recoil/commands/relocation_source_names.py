"""Refresh reviewed relocation source spellings with complete current contexts."""
from __future__ import annotations

import argparse
from copy import deepcopy
import json
from pathlib import Path
import sys

from _recoil.commands import native_eh_relocations as eh
from _recoil.commands import relocation_expectations as expectations
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH, ProgressError, ProgressStore
from _recoil.lib.tooling import REPO_ROOT, configure_stdio


def require(condition, message):
    if not condition:
        raise ProgressError(message)


def plan_refresh(document, payload, *, bindings, reference):
    require(isinstance(payload, dict) and set(payload) == {"schema", "reviewed", "renames"}
            and payload["schema"] == "recoil-relocation-source-names-v1"
            and payload["reviewed"] is True, "expected reviewed source-name refresh payload")
    rows = payload["renames"]
    require(isinstance(rows, list) and rows, "renames must be a non-empty array")
    requests = {}
    for row in rows:
        require(isinstance(row, dict) and set(row) == {
            "source_symbol_id", "expected_object_symbol", "object_symbol", "expected_occurrences", "reason"
        }, "source rename requires the exact identity, names, occurrence count and reason")
        for key in ("source_symbol_id", "expected_object_symbol", "object_symbol", "reason"):
            value = row[key]
            require(isinstance(value, str) and value and value == value.strip()
                    and not any(ord(c) < 32 or ord(c) == 127 for c in value), "invalid source-name field")
        identity = row["source_symbol_id"]
        source = document.collection("symbols").get(identity, {})
        require(identity.startswith("recoil:function:") and source.get("binary") == "recoil"
                and source.get("kind") == "function"
                and source.get("pipeline_class") in {"authored", "authored-lifecycle"},
                "source must be an existing authored Recoil function")
        require(type(row["expected_occurrences"]) is int and row["expected_occurrences"] > 0,
                "expected_occurrences must be a positive integer")
        require(row["expected_object_symbol"] != row["object_symbol"], "source rename is unchanged")
        key = (identity, row["expected_object_symbol"])
        require(key not in requests, "duplicate source rename")
        requests[key] = row

    proposed = deepcopy(document.data)
    counts = {key: 0 for key in requests}
    changes = []

    def refresh_source(snapshot):
        key = (snapshot.get("symbol_id"), snapshot.get("object_symbol"))
        request = requests.get(key)
        if request is None:
            return False
        current = expectations.build_object_binding_snapshot(document, bindings,
            symbol_id=key[0], object_symbol=request["object_symbol"])
        renamed = dict(snapshot, object_symbol=request["object_symbol"])
        require(current == renamed, "source context differs beyond object_symbol: " + key[0])
        snapshot["object_symbol"] = request["object_symbol"]
        counts[key] += 1
        return True

    for identity, symbol in proposed.get("symbols", {}).items():
        raw = symbol.get("relocation_target_binding")
        target_bindings = raw if isinstance(raw, list) else [raw] if isinstance(raw, dict) else []
        for binding in target_bindings:
            if not refresh_source(binding["binding_context"]["source_binding"]):
                continue
            _, stale = expectations.relocation_target_binding_staleness(binding, document=document,
                bindings=bindings, target_symbol_id=identity, reference=reference)
            require(not stale, "relocation target context is stale: " + identity + ": " + json.dumps(stale))
            changes.append(dict(symbol_id=identity, field="relocation_target_binding", binding=binding))

        native = symbol.get(eh.FIELD)
        if native is not None and refresh_source(native["context"]["source"]):
            saved = native["context"]
            current = eh.binding_context(document, bindings, saved["source"]["symbol_id"],
                saved["source"]["object_symbol"], saved["owner_id"], saved["evidence_ids"], reference)
            require(current == saved, "native EH context differs beyond source name: " + identity)
            changes.append(dict(symbol_id=identity, field=eh.FIELD, binding=native))

        for exception in symbol.get("relocation_expectation_exceptions", []):
            snapshot = exception.get("source_binding")
            if not isinstance(snapshot, dict):
                continue
            previous = snapshot.get("object_symbol")
            if not refresh_source(snapshot):
                continue
            require(snapshot["symbol_id"] == identity and exception.get("object_symbol") == previous,
                    "exception source identity or spelling disagrees")
            exception["object_symbol"] = snapshot["object_symbol"]
            _, stale = expectations.reviewed_exception_staleness(exception, document=document,
                bindings=bindings, reference=reference)
            require(not stale, "reviewed exception context is stale: " + identity + ": " + json.dumps(stale))
            changes.append(dict(symbol_id=identity, field="relocation_expectation_exceptions", binding=exception))

    for key, request in requests.items():
        require(counts[key] == request["expected_occurrences"],
                f"source binding population changed for {key}: expected {request['expected_occurrences']}, found {counts[key]}")
    return proposed, changes


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
        from _recoil.commands.live_byte_verify import _bindings, DEFAULT_MANIFEST_DIR, DEFAULT_REFERENCE
        path = args.payload_file.resolve()
        require(path.is_relative_to((REPO_ROOT / "build").resolve()), "review payload must be under build/")
        payload = json.loads(path.read_text(encoding="utf-8-sig"))
        store = ProgressStore(DEFAULT_PROGRESS_PATH)
        document = store.load()
        require(document.revision == args.expected_revision, "tracker revision changed")
        proposed, changes = plan_refresh(document, payload, bindings=_bindings(document, DEFAULT_MANIFEST_DIR),
                                         reference=DEFAULT_REFERENCE)
        commit = store.commit(proposed, expected_revision=args.expected_revision, apply=args.apply)
        if args.apply:
            saved = store.load()
            require(saved.revision == commit.revision, "saved tracker revision differs from commit result")
            for change in changes:
                identity, field = change["symbol_id"], change["field"]
                require(saved.collection("symbols")[identity].get(field) == proposed["symbols"][identity][field],
                        "saved source binding differs from reviewed replacement: " + identity)
        print(json.dumps(dict(kind="relocation-source-name-refresh", acceptance_effects=[],
            renames=payload["renames"], changes=changes, commit=commit.to_dict()), indent=2))
        return 0
    except (OSError, ValueError, KeyError, TypeError, ProgressError,
            expectations.RelocationExpectationError) as exc:
        print(f"relocation source name error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
