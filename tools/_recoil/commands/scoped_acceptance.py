"""Fresh, scoped storage/owner acceptance without serial-stage or membership edits."""
from __future__ import annotations

import argparse
from copy import deepcopy
import json
from pathlib import Path
import sys
from dataclasses import asdict

from _recoil.commands import live_byte_verify as byte
from _recoil.commands.progress_cli import _absolute_fresh_build_root
from _recoil.commands.progress_v2 import add_live_evidence
from _recoil.commands.relocation_expectations import build_target_identity_state, derive_relocation_expectations
from _recoil.commands.vc5_build import DEFAULT_MANIFEST, load_config, object_path, parse_link_map
from _recoil.commands.vc5_verify import DEFAULT_MANIFEST_DIR, resolve_function_symbol_for_coff
from _recoil.lib.match_evidence import dependency_states, source_context
from _recoil.lib.progress import (DEFAULT_PROGRESS_PATH, OWNER_GATES, STORAGE_DIMENSIONS, TIERS,
    ProgressError, ProgressStore, state_record, validate_owner_invariants)
from _recoil.lib.storage_proof import compare_storage, extent, require, storage_scope
from _recoil.lib.tooling import REPO_ROOT, DEFAULT_VC5_ROOT, configure_stdio


def owner_scope(document, owner_id):
    owner = document.collection("owners").get(owner_id)
    require(isinstance(owner, dict) and owner.get("binary") == "recoil"
            and owner.get("kind") != "provider-boundary", "select an existing authored Recoil owner")
    members = [edge["symbol_id"] for edge in owner.get("relationships", [])
               if edge.get("kind") in {"primary-function", "primary-data"}]
    require(bool(members) and len(set(members)) == len(members), "owner primary-member census is empty or duplicated")
    for identity in members:
        symbol = document.collection("symbols").get(identity, {})
        authored = (byte.symbol_authored_order_gate(symbol) if ":function:" in identity
                    else symbol.get("disposition") == "authored")
        require(symbol.get("binary") == "recoil" and authored,
                f"{identity}: owner contains a non-authored or unresolved primary member")
        primary_owners = [key for key, value in document.collection("owners").items()
                          if any(e.get("kind") in {"primary-function", "primary-data"} and e.get("symbol_id") == identity
                                 for e in value.get("relationships", []))]
        require(primary_owners == [owner_id], f"{identity}: primary ownership is not unique")
    return owner, sorted(members)


def review_context(document, owner_id, bindings, config):
    owner, members = owner_scope(document, owner_id)
    sources = set(owner.get("source_paths", []))
    sources.update(b.source_from or b.target.source_from for member in members for b in bindings.get(member, []))
    sources = sorted(path for path in sources if Path(path).suffix.lower() in {".c", ".cpp", ".h"})
    require(bool(sources), "owner has no source closure")
    dependencies = {edge["target_owner_id"]: document.collection("owners").get(edge["target_owner_id"])
                    for edge in owner.get("relationships", []) if edge.get("kind") == "depends-on-owner"}
    require(all(dependencies.values()), "owner dependency is unresolved")
    return {"owner_id": owner_id, "owner": deepcopy(owner), "members": {
        member: deepcopy(document.collection("symbols")[member]) for member in members},
        "dependencies": deepcopy(dependencies),
        "source_contexts": [source_context(source, config) for source in sources]}


def validate_review(payload, context, *, gates=(), tier=None):
    require(isinstance(payload, dict) and set(payload) == {"reviewed", "context", "gates", "tier", "rationale", "scrutiny", "entries", "live_comparison"},
            "review payload requires exactly reviewed, context, gates, tier, rationale, scrutiny, entries, live_comparison")
    require(payload["reviewed"] is True and payload["context"] == context, "review context is stale or unreviewed")
    require(payload["gates"] == sorted(gates) and payload["tier"] == tier, "review covers a different gate/tier request")
    require(isinstance(payload["rationale"], str) and payload["rationale"].strip(), "review needs a substantive rationale")
    scrutiny = payload["scrutiny"]
    require(isinstance(scrutiny, dict) and scrutiny.get("decision") == "ALLOW"
            and isinstance(scrutiny.get("rationale"), str) and scrutiny["rationale"].strip(), "positive owner acceptance needs explicit source-owner scrutiny")
    entries = payload["entries"]
    require(isinstance(entries, dict) and set(entries) == set(context["members"]), "review must cover every primary member exactly")
    required = {"source", "behavior"}
    if tier in {"A", "S"}:
        required.add("near_byte")
    if tier == "S":
        required.add("provider_abi")
    for identity, entry in entries.items():
        require(isinstance(entry, dict) and set(entry) == required
                and all(isinstance(value, str) and value.strip() for value in entry.values()),
                f"{identity}: review needs substantive {sorted(required)} observations")


class LiveInputs:
    def __init__(self, document, root):
        self.document, self.root = document, root
        self.before = self.input_inventory()
        self.bindings = byte._bindings(document, DEFAULT_MANIFEST_DIR)
        self.config = load_config(DEFAULT_MANIFEST)
        self.reference = REPO_ROOT / "support/Recoil.exe"
        self.identities = build_target_identity_state(document, self.bindings, reference=self.reference)
        self.outputs = None
        self.unchanged()

    def input_inventory(self):
        config = load_config(DEFAULT_MANIFEST)
        roots = {REPO_ROOT/"src", REPO_ROOT/"tools", Path(DEFAULT_VC5_ROOT),
                 *config.include_dirs, *config.lib_dirs}
        # Inventory directories again at each guard so new headers, aliases or
        # resource inputs cannot change resolution without being detected.
        paths = {str(p.resolve()) for tree in roots for p in tree.rglob("*")
                 if p.is_file() and "__pycache__" not in p.parts and p.suffix != ".pyc"}
        paths.update(str(p) for p in (REPO_ROOT/"support/Recoil.exe", config.vc5_env))
        return dependency_states(list(paths))

    def build(self):
        args = byte.build_parser().parse_args(["authored", "--build-root", self.root.relative_to(REPO_ROOT).as_posix()])
        _, self.config, self.paths = byte._run_fresh_build(args, args.build_root)
        self.outputs = {p: p.read_bytes() for p in self.root.rglob("*")
                        if p.is_file() and p.suffix.lower() in {".obj", ".exe", ".map", ".res", ".lib"}}
        self.map = parse_link_map(self.paths.map_path)

    def unchanged(self):
        require(self.input_inventory() == self.before,
                "source, compiler inputs, retail or proof tools changed during live verification")
        if self.outputs is not None:
            current = {p for p in self.root.rglob("*") if p.is_file()
                       and p.suffix.lower() in {".obj", ".exe", ".map", ".res", ".lib"}}
            require(current == set(self.outputs) and all(p.read_bytes() == data for p, data in self.outputs.items()),
                    "fresh object/MAP/image generation changed during comparison")

    def data_presence(self, identity):
        results = []
        bindings = self.bindings.get(identity, [])
        require(bool(bindings), f"{identity}: data has no registered source selector")
        from _recoil.commands.vc5_verify import resolve_coff_symbol_regex
        for binding in bindings:
            source = binding.source_from or binding.target.source_from
            obj_path = object_path(self.config, self.paths, REPO_ROOT/source)
            obj = byte.CoffObject.from_path(obj_path)
            definition = binding.function
            name = (resolve_coff_symbol_regex(obj, definition.symbol_regex, item_label=identity)
                    if definition.symbol_regex else definition.symbol)
            body = obj.data_symbol_bytes(name)
            results.append({"source": source, "object_symbol": name, "object_bytes": body.data.hex(),
                            "relocations": [asdict(r) for r in body.relocations]})
        return results

    def storage(self, identity):
        return compare_storage(self.document, identity, bindings=self.bindings, identities=self.identities[0],
            config=self.config, paths=self.paths, parsed_map=self.map, reference=self.reference)

    def function(self, identity, *, bytes_required, exact_required=False, linked_required=True):
        symbol = self.document.collection("symbols")[identity]
        bindings = self.bindings.get(identity, [])
        require(bool(bindings), f"{identity}: no current native function selector")
        results = []
        for binding in bindings:
            source = REPO_ROOT / (binding.source_from or binding.target.source_from)
            obj = byte.CoffObject.from_path(object_path(self.config, self.paths, source))
            name = resolve_function_symbol_for_coff(obj, binding.function)
            body = obj.function_bytes(name)
            require(bool(body.data), "owner function has no current emitted definition")
            if not linked_required:
                results.append({"passed": True, "symbol_id": identity, "symbol": name,
                                "source": str(source), "object_bytes": body.data.hex()})
                continue
            placement = [r for r in self.map.symbols if r.symbol == name and r.is_function]
            require(len({r.address for r in placement}) == 1, "owner function linked presence/identity is ambiguous")
            require(all(byte._map_object_name(r.object).casefold() == object_path(self.config, self.paths, source).name.casefold()
                        for r in placement), "linked owner definition comes from a different object; folded identity needs explicit proof")
            if bytes_required:
                row = byte._rows(self.document, "authored", symbol["address"])[0]
                expectation = derive_relocation_expectations(document=self.document, row=row,
                    object_symbol=byte.object_selector(binding.function), bindings=self.bindings,
                    reference=self.reference, target_identity_state=self.identities)
                require(expectation["passed"], f"{identity}: relocation expectations unresolved")
                # Unknown compiler-local reader populations fail closed in the
                # byte engine; this scoped route never guesses static ordinals.
                result = byte._compare_row(mode="authored", row=row, binding=binding,
                    config=self.config, paths=self.paths, reference=self.reference, parsed_map=self.map,
                    relocation_catalog=expectation["expectations"], target_rows=self.document.collection("symbols"),
                    allow_near_byte_review=not exact_required)
                if exact_required:
                    require(result["passed"] and result.get("match_level") == "byte",
                            f"{identity}: exact byte comparison failed at {result.get('stage')}")
                else:
                    require(result.get("structural_comparison_complete") is True,
                            f"{identity}: near-byte review cannot accept incomplete identity/relocation proof at {result.get('stage')}")
                # Tier A reviews the actual differences of this complete fresh
                # comparison. It grants no per-function matching annotation.
                result.pop("object_path", None)
                result["candidate_object_bytes"] = body.data.hex()
                result["retail_bytes"] = byte._pe_bytes(self.reference, int(symbol["address"], 0),
                    int(symbol["end_exclusive"], 0) - int(symbol["address"], 0)).hex()
                result["candidate_linked_bytes"] = byte._pe_bytes(self.paths.exe_path, placement[0].address, len(body.data)).hex()
                results.append(result)
            else:
                results.append({"passed": True, "symbol_id": identity, "symbol": name, "source": str(source),
                                "candidate_address": hex(placement[0].address), "linked_presence": True})
        return results

    def source_graph(self, members):
        from _recoil.commands.source_trace_audit import source_paths, migrated_graph_findings
        from _recoil.lib.source_traceability import (artifact_index_from_data,
            parse_source_trace_path, validate_source_trace, merge_source_trace_documents)
        index = artifact_index_from_data(self.document.data)
        documents = tuple(parse_source_trace_path(path, repo_root=REPO_ROOT) for path in source_paths([], REPO_ROOT))
        paths = {a.path for d in documents for a in d.artifacts if a.artifact_id in members}
        present = {a.artifact_id for d in documents for a in d.artifacts}
        require(set(members) <= present, "owner primary members lack current attached source artifacts")
        findings = list(migrated_graph_findings(documents, index, anchor_scope_paths=paths, emission_scope_paths=paths))
        findings += list(merge_source_trace_documents(documents))
        for document in documents:
            if document.path in paths:
                findings += list(validate_source_trace(document, index, strict=True))
        relevant = [asdict(f) for f in findings if f.artifact_id in members or (f.artifact_id is None and f.path in paths)]
        require(not relevant, f"current owner source graph is invalid: {relevant[:3]}")

    def providers(self, owner):
        results = {}
        for edge in owner.get("relationships", []):
            if edge.get("kind") != "depends-on-owner":
                continue
            dependency = self.document.collection("owners")[edge["target_owner_id"]]
            if dependency.get("kind") != "provider-boundary":
                continue
            members = [e["symbol_id"] for e in dependency.get("relationships", [])
                       if e.get("kind") in {"primary-function", "primary-data", "provider-function"}]
            require(bool(members), "provider ABI proof has no registered member census")
            for identity in members:
                row = self.document.collection("symbols")[identity]
                extent(row)
                result = byte._compare_provider_row(row={**row, "symbol_id": identity}, paths=self.paths,
                    reference=self.reference, parsed_map=self.map, data_body="data" in row.get("kind", ""))
                require(result["passed"], f"{identity}: provider ABI comparison failed: {result.get('stage')}")
                results[identity] = result
        return results


def verify_owner(live, owner_id, *, gates, tier):
    owner, members = owner_scope(live.document, owner_id)
    validate_owner_invariants(live.document.data)
    # Reprove retained accepted claims as well as the selected writes. A weaker
    # request must never carry old stronger evidence onto changed source.
    required_gates = set(gates) | {key for key, value in owner.get("gates", {}).items() if value == "accepted"}
    if tier in {"B", "A", "S"}:
        required_gates.update(("boundary", "source", "data", "owner_linkage"))
        require(all(owner.get("gates", {}).get(gate) == "accepted" or
                    (gate in {"data", "owner_linkage"} and owner.get("gates", {}).get(gate) == "none") for gate in required_gates),
                "higher tier requires separately accepted owner gates")
    if tier == "S":
        require(owner.get("gates", {}).get("byte") == "accepted", "tier S requires the separate owner byte gate")
    exact_required = "byte" in required_gates or tier == "S"
    live.source_graph(members)
    for edge in owner.get("relationships", []):
        if edge.get("kind") == "depends-on-owner" and (tier in {"B", "A", "S"} or required_gates & {"data", "owner_linkage", "byte"}):
            dependency = live.document.collection("owners").get(edge["target_owner_id"], {})
            require(dependency.get("gates", {}).get("boundary") == "accepted", "owner dependency boundary is unresolved")
            if dependency.get("kind") == "provider-boundary":
                require(dependency.get("provider_state") == "accepted", "provider dependency is unresolved")
    results, storages = {}, {}
    for identity in members:
        symbol = live.document.collection("symbols")[identity]
        trace = symbol.get("source_traceability", {})
        require(trace.get("state") == "resolved" and bool(trace.get("source_edges")),
                f"{identity}: source-to-artifact relationships are unresolved")
        retained = owner.get("reimplementation", {}).get("entries", {}).get(identity, {}).get("tier", "X")
        member_tier = max((tier or "X", retained), key=TIERS.index)
        member_exact = exact_required or member_tier == "S"
        if ":function:" in identity:
            values = live.function(identity, bytes_required="byte" in required_gates or member_tier in {"A", "S"}, exact_required=member_exact,
                linked_required=bool(required_gates & {"owner_linkage", "byte"}) or member_tier in {"B", "A", "S"})
            if member_exact:
                require(all(value.get("match_level") == "byte" for value in values), "relaxed matching cannot accept owner bytes or tier S")
            results[identity] = values
        else:
            if not required_gates & {"data", "owner_linkage", "byte"} and member_tier in {"X", "C"}:
                results[identity] = live.data_presence(identity)
                continue
            selected = symbol.get("storage_contribution_ids", [])
            require(bool(selected), f"{identity}: no registered storage contribution")
            for storage_id in selected:
                if storage_id not in storages:
                    storages[storage_id] = live.storage(storage_id)
                require(identity in storages[storage_id]["symbol_ids"], "owner member is absent from its storage proof")
                required = {"extent"}
                if required_gates & {"data", "owner_linkage", "byte"} or member_tier in {"B", "A", "S"}:
                    required.update(("object", "relocation", "link", "zero-fill"))
                if member_exact:
                    required.update(("order", "raw"))
                require(all(storages[storage_id]["dimensions"][key] for key in required), f"{identity}: owner data proof failed")
            results[identity] = {"storage_ids": selected}
    from _recoil.lib.storage_proof import preserve_storage_relationships
    preserve_storage_relationships(list(storages.values()))
    retained_s = any(entry.get("tier") == "S" for entry in owner.get("reimplementation", {}).get("entries", {}).values())
    providers = live.providers(owner) if exact_required or retained_s else {}
    return {"owner_id": owner_id, "members": members, "gates": sorted(gates), "tier": tier, "provider_results": providers,
            "function_results": results, "storage_results": storages, "passed": True}


def record_storage(data, report, dimensions, evidence_id):
    require(bool(dimensions) and set(dimensions) <= set(STORAGE_DIMENSIONS), "invalid storage dimensions")
    require(all(report["dimensions"].get(key) is True for key in dimensions), "requested storage comparison failed")
    row = data["storage_contributions"][report["storage_id"]]
    require(all(row.get("applicability", {}).get(key) is True for key in dimensions), "requested dimension is not applicable")
    for key in dimensions:
        row.setdefault("verification", {})[key] = state_record("passed", "accepted", "current", [evidence_id], validation_mode="live")


def comparison_content(value, root):
    """Exclude only the deliberately different fresh build directory spelling."""
    if isinstance(value, dict):
        return {key: comparison_content(item, root) for key, item in value.items()}
    if isinstance(value, list):
        return [comparison_content(item, root) for item in value]
    if isinstance(value, str):
        for spelling in (str(root), root.as_posix(), root.relative_to(REPO_ROOT).as_posix()):
            value = value.replace(spelling, "<fresh-build>")
    return value


def record_owner(data, report, evidence_id):
    require(report.get("passed") is True, "owner live proof did not pass")
    owner = data["owners"][report["owner_id"]]
    current = sorted(edge["symbol_id"] for edge in owner.get("relationships", [])
                     if edge.get("kind") in {"primary-function", "primary-data"})
    require(current == report["members"], "owner membership changed before commit")
    owner["evidence_ids"] = sorted(set(owner.get("evidence_ids", [])) | {evidence_id})
    for gate in report["gates"]:
        require(gate in OWNER_GATES, "invalid owner gate")
        owner.setdefault("gates", {})[gate] = "accepted"
    if report["tier"]:
        require(report["tier"] in TIERS[1:], "invalid tier")
        entries = owner.setdefault("reimplementation", {}).setdefault("entries", {})
        for identity in report["members"]:
            prior = entries.get(identity, {})
            if TIERS.index(prior.get("tier", "X")) > TIERS.index(report["tier"]):
                continue
            entries[identity] = {**prior, "kind": "function" if ":function:" in identity else "data",
                                 "tier": report["tier"], "evidence_ids": sorted(set(prior.get("evidence_ids", [])) | {evidence_id})}
    validate_owner_invariants(data)


def accept_storage(args):
    root = _absolute_fresh_build_root(args.build_root)
    store = ProgressStore(args.progress)
    document = store.load()
    require(document.revision == args.expected_revision, "tracker revision changed")
    storage_scope(document, args.storage)
    live = LiveInputs(document, root)
    live.build()
    report = live.storage(args.storage)
    live.unchanged()
    dimensions = sorted(set(args.dimension))
    def transform(data):
        live.unchanged()
        evidence_id = add_live_evidence(data, kind="live-storage-validation", scope_ids=[args.storage, *report["symbol_ids"]],
            summary="Scoped live authored storage comparison", provenance={"dimensions": dimensions, "comparison": report})
        record_storage(data, report, dimensions, evidence_id)
    commit = store.mutate(transform, expected_revision=args.expected_revision, apply=args.apply)
    return {"comparison": report, "commit": commit.to_dict()}


def accept_owner(args):
    root = _absolute_fresh_build_root(args.build_root)
    store = ProgressStore(args.progress)
    document = store.load()
    require(document.revision == args.expected_revision, "tracker revision changed")
    live = LiveInputs(document, root)
    context = review_context(document, args.owner, live.bindings, live.config)
    payload = json.loads(args.payload_file.read_text(encoding="utf-8-sig"))
    gates, tier = sorted(set(getattr(args, "gate", []))), getattr(args, "tier", None)
    validate_review(payload, context, gates=gates, tier=tier)
    live.build()
    report = verify_owner(live, args.owner, gates=gates, tier=tier)
    report = comparison_content(report, root)
    require(payload["live_comparison"] == report, "fresh owner comparison differs from the reviewed comparison")
    live.unchanged()
    def transform(data):
        live.unchanged()
        evidence_id = add_live_evidence(data, kind="live-owner-validation", scope_ids=[args.owner, *report["members"]],
            summary="Scoped live authored owner review and comparison", provenance={"review": payload, "comparison": report})
        record_owner(data, report, evidence_id)
    commit = store.mutate(transform, expected_revision=args.expected_revision, apply=args.apply)
    return {"comparison": report, "commit": commit.to_dict()}


def build_parser():
    parser = argparse.ArgumentParser(description=__doc__)
    operations = parser.add_subparsers(dest="operation", required=True)
    for name in ("storage", "owner", "promote", "review-context"):
        child = operations.add_parser(name)
        child.add_argument("--progress", type=Path, default=DEFAULT_PROGRESS_PATH)
        child.add_argument("--json", action="store_true")
        child.add_argument("--storage" if name == "storage" else "--owner", required=True)
        child.add_argument("--build-root", type=Path, required=True)
        if name == "review-context":
            child.add_argument("--gate", choices=OWNER_GATES, action="append", default=[])
            child.add_argument("--tier", choices=TIERS[1:])
            continue
        child.add_argument("--expected-revision", type=int, required=True)
        mode = child.add_mutually_exclusive_group(required=True)
        mode.add_argument("--dry-run", action="store_true")
        mode.add_argument("--apply", action="store_true")
        if name == "storage":
            child.add_argument("--dimension", choices=STORAGE_DIMENSIONS, action="append", required=True)
        else:
            child.add_argument("--payload-file", type=Path, required=True)
            if name == "owner":
                child.add_argument("--gate", choices=OWNER_GATES, action="append", required=True)
            else:
                child.add_argument("--tier", choices=TIERS[1:], required=True)
    return parser


def main(argv=None):
    configure_stdio()
    args = build_parser().parse_args(argv)
    try:
        if args.operation == "storage":
            result = accept_storage(args)
        elif args.operation in {"owner", "promote"}:
            result = accept_owner(args)
        else:
            document = ProgressStore(args.progress).load()
            require(bool(args.gate) != bool(args.tier), "select gates or a tier for review")
            live = LiveInputs(document, _absolute_fresh_build_root(args.build_root))
            context = review_context(document, args.owner, live.bindings, live.config)
            live.build()
            report = comparison_content(verify_owner(live, args.owner, gates=sorted(set(args.gate)), tier=args.tier), live.root)
            live.unchanged()
            fields = {"source": "", "behavior": ""}
            if args.tier in {"A", "S"}: fields["near_byte"] = ""
            if args.tier == "S": fields["provider_abi"] = ""
            result = {"reviewed": False, "context": context,
                "gates": sorted(set(args.gate)), "tier": args.tier, "rationale": "", "scrutiny": {"decision": "BLOCK", "rationale": ""},
                "live_comparison": report,
                "entries": {identity: dict(fields) for identity in owner_scope(document, args.owner)[1]}}
        print(json.dumps(result, indent=2))
        return 0
    except (OSError, ValueError, RuntimeError) as exc:
        print(f"scoped acceptance error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
