"""Governed Pro review, live function classification and source mirrors."""
from __future__ import annotations

import argparse
from copy import deepcopy
import json
from pathlib import Path
import sys
from typing import Any, Mapping

from _recoil.lib.function_match import MATCH_VERSION, MATCH_LEVELS
from _recoil.lib.commutative_match import COMMUTATIVE_VERSION, valid_contract
from _recoil.lib.match_evidence import (annotation_edits, dependency_states, source_context)
from _recoil.lib.progress import (DEFAULT_PROGRESS_PATH, ProgressStore, ProgressError, ConcurrentProgressUpdate)
from _recoil.lib.source_traceability import parse_source_trace_path
from _recoil.lib.tooling import REPO_ROOT, configure_stdio
from _recoil.commands.progress_v2 import add_live_evidence

PROOF_DEPENDENCIES = (
    "tools/_recoil/lib/function_match.py", "tools/_recoil/lib/match_evidence.py",
    "tools/_recoil/lib/commutative_match.py",
    "tools/_recoil/commands/live_byte_verify.py", "tools/_recoil/commands/relocation_expectations.py",
    "tools/_recoil/call_contract/instructions.py", "tools/_recoil/config/vc5_final_build.json",
    "tools/requirements.txt", "tools/_recoil/lib/call_contract_generations.py",
    "tools/_recoil/lib/source_traceability.py", "tools/_recoil/commands/progress_v2.py",
    "tools/_recoil/commands/match_progress.py", "tools/_recoil/lib/progress.py",
)


def prepare_updates(document, report, config):
    """Project only complete invocation-local proofs, with no stage effects."""
    contexts = {}
    updates = {}
    for group in report.get("matched_groups", []):
        level = group.get("match_level", "byte")
        if level not in MATCH_LEVELS:
            raise ProgressError("live match result has an invalid match level")
        for identity in group["scope_ids"]:
            sources = sorted({item["source_from"] for item in group.get("target_bindings", [])
                              if item.get("scope_id") == identity and item.get("source_from")})
            if not sources:
                continue  # provider binaries and generated artifacts have no source tag
            dependencies = set(PROOF_DEPENDENCIES)
            for source in sources:
                if source not in contexts:
                    contexts[source] = source_context(source, config)
                dependencies.update(contexts[source]["files"])
            review = document.collection("symbols")[identity].get(level + "_match_review", {})
            bodies = [item for item in group.get("identity_results", []) if identity in item.get("scope_ids", [])]
            updates[identity] = {"version": MATCH_VERSION, "level": level,
                "validation_mode": "live", "freshness": "current", "evidence_ids": ["pending-live-evidence"],
                "review_evidence_id": review.get("evidence_id") if level != "byte" else None,
                "dependencies": dependency_states(list(dependencies)), "source_paths": sources,
                "retail_relocations": bodies[0].get("retail_relocations", []) if bodies else []}
            if level == "commutative":
                updates[identity]["commutative_version"] = COMMUTATIVE_VERSION
    return updates


def record_updates(data, updates, evidence_id):
    for identity, record in updates.items():
        value = deepcopy(record)
        value["evidence_ids"] = [evidence_id]
        value["dependencies"] = dependency_states([item["path"] for item in value["dependencies"]])
        data["symbols"][identity]["function_match"] = value


def prepare_annotations(document, updates, *, all_sources=False):
    from _recoil.commands.source_trace_audit import source_paths
    symbols = deepcopy(document.collection("symbols"))
    for identity, record in updates.items():
        symbols[identity]["function_match"] = record
    selected = [] if all_sources else sorted({path for record in updates.values() for path in record["source_paths"]})
    if not selected and not all_sources:
        return [], []
    documents = [parse_source_trace_path(path, repo_root=REPO_ROOT) for path in source_paths(selected, REPO_ROOT)]
    return annotation_edits(documents, symbols)


def with_annotation_writes(edits, apply, operation):
    """CAS guard source mirrors, rolling back our writes if ledger CAS fails."""
    written = []
    try:
        for edit in edits:
            path = REPO_ROOT / edit["path"]
            if path.read_bytes() != edit["before"]:
                raise ConcurrentProgressUpdate(f"annotation source changed: {edit['path']}")
            if edit["before"].count(b"\n") != edit["after"].count(b"\n"):
                raise ProgressError("match annotation edits must preserve source line counts")
            if apply:
                path.write_bytes(edit["after"])
                written.append(edit)
        return operation()
    except BaseException:
        for edit in reversed(written):
            path = REPO_ROOT / edit["path"]
            if path.read_bytes() == edit["after"]:
                path.write_bytes(edit["before"])
        raise


def _review(args, document, store):
    from _recoil.commands.live_byte_verify import _bindings, _select_bindings, _rows
    from _recoil.commands.vc5_build import DEFAULT_MANIFEST, load_config
    from _recoil.commands.vc5_verify import DEFAULT_MANIFEST_DIR
    payload = json.loads(args.payload_file.read_text(encoding="utf-8-sig"))
    level = args.operation.removeprefix("review-")
    decision = ("compiler-register-allocation-only" if level == "instruction"
                else "compiler-commutative-operand-selection-only")
    required = {"symbol_id", "decision", "no_remaining_credible_source_options", "reason", "attempts",
                "differences", "source_context", "prompt", "answer", "transcript", "receipt", "reviewed"}
    if level == "commutative":
        required.update(("contract", "contract_justification"))
    if not isinstance(payload, dict) or set(payload) != required or payload["reviewed"] is not True:
        raise ProgressError(f"review payload must contain exactly {sorted(required)} and reviewed:true")
    if payload["decision"] != decision or payload["no_remaining_credible_source_options"] is not True:
        raise ProgressError("Pro must explicitly confirm compiler attribution and exhaustion of credible source options")
    if not payload["attempts"] or not payload["differences"] or not str(payload["reason"]).strip():
        raise ProgressError("review needs concrete attempts, differences and rationale")
    if level == "commutative" and (not valid_contract(payload["contract"])
                                  or not str(payload["contract_justification"]).strip()):
        raise ProgressError("commutative review needs the supported explicit FP contract and its caller justification")
    identity = payload["symbol_id"]
    symbol = document.collection("symbols").get(identity)
    if not symbol or symbol.get("pipeline_class") not in {"authored", "authored-lifecycle"}:
        raise ProgressError("match review requires an existing authored function")
    artifacts = {}
    for name in ("prompt", "answer", "transcript", "receipt"):
        path = (REPO_ROOT / payload[name]).resolve()
        path.relative_to(REPO_ROOT)
        artifacts[name] = path.read_text(encoding="utf-8-sig")
    receipt = json.loads(artifacts["receipt"])
    if receipt.get("submission", {}).get("status") != "confirmed":
        raise ProgressError("review requires a confirmed Pro submission receipt")
    # Require an unambiguous, captured decision in the actual answer. This is
    # advisory eligibility only; the independent live machine proof is mandatory.
    import re
    sentinel = level.upper() + "_MATCH_APPROVED"
    if not re.search(r"(?m)^" + sentinel + r"[ \t]*$", artifacts["answer"]) or level.upper() + "_MATCH_NOT_APPROVED" in artifacts["answer"]:
        raise ProgressError("captured Pro answer lacks the explicit " + sentinel + " decision")
    if artifacts["answer"].strip() not in artifacts["transcript"]:
        raise ProgressError("captured answer is not present in the submitted exchange transcript")
    rows = _rows(document, "authored", symbol["address"])
    binding = _select_bindings(_bindings(document, DEFAULT_MANIFEST_DIR), rows[0])[0]
    context = source_context(binding.source_from or binding.target.source_from, load_config(DEFAULT_MANIFEST))
    if payload["source_context"] != context:
        raise ProgressError("reviewed source/compiler context is stale")
    review = {"version": MATCH_VERSION, "decision": payload["decision"],
              "no_remaining_credible_source_options": True, "context": context,
              "differences": payload["differences"], "reason": payload["reason"], "attempts": payload["attempts"],
              "artifacts": {key: payload[key] for key in artifacts}}
    if level == "commutative":
        review.update(commutative_version=COMMUTATIVE_VERSION, contract=payload["contract"],
                      contract_justification=payload["contract_justification"])
    def transform(data):
        evidence_id = add_live_evidence(data, kind=level + "-match-pro-review",
            summary=f"Pro reviewed {level} fallback eligibility for {identity}", scope_ids=[identity],
            provenance={"advisory_only": True, "artifacts": review["artifacts"], "answer": artifacts["answer"],
                        "reason": payload["reason"], "attempts": payload["attempts"]})
        review["evidence_id"] = evidence_id
        data["symbols"][identity][level + "_match_review"] = review
        prior = data["symbols"][identity].get("function_match")
        if isinstance(prior, dict) and prior.get("level") == level:
            prior["freshness"] = "changed"
    commit = store.mutate(transform, expected_revision=args.expected_revision, apply=args.apply)
    return {"kind": level + "-match-review", "symbol_id": identity, "advisory_only": True,
            "decision": payload["decision"], "commit": commit.to_dict()}


def _refresh(args, document, store):
    from _recoil.commands import live_byte_verify as verifier
    from _recoil.commands.progress_cli import _absolute_fresh_build_root
    from _recoil.commands.vc5_build import load_config
    root = _absolute_fresh_build_root(args.build_root)
    verify_args = verifier.build_parser().parse_args(["authored", "--progress", str(args.progress), "--build-root", root.relative_to(REPO_ROOT).as_posix()])
    verify_args.at = args.at
    verify_args.classify_all = True
    report = verifier.run(verify_args)
    updates = prepare_updates(document, report, load_config(verify_args.final_config))
    # Selected failures revoke only their old classification, never fabricate
    # byte-stage acceptance or advance the scheduler.
    failed = {identity for row in report.get("classifications", []) if not row["passed"] for identity in row["scope_ids"]}
    preview = deepcopy(document)
    for identity in failed:
        if isinstance(preview.collection("symbols")[identity].get("function_match"), dict):
            preview.collection("symbols")[identity]["function_match"]["freshness"] = "changed"
    edits, excluded = prepare_annotations(preview, updates, all_sources=args.all)
    def transform(data):
        evidence_id = add_live_evidence(data, kind="live-function-match-validation",
            summary=f"Fresh complete function comparison: {len(updates)} matched, {len(failed)} excluded",
            scope_ids=sorted(set(updates) | failed),
            provenance={"stage_acceptance": False, "match_version": MATCH_VERSION,
                        "build_root": str(root), "match_counts": report.get("match_counts", {})})
        record_updates(data, updates, evidence_id)
        for identity in failed:
            prior = data["symbols"][identity].get("function_match")
            if isinstance(prior, dict):
                prior["freshness"] = "changed"
    commit = with_annotation_writes(edits, args.apply,
        lambda: store.mutate(transform, expected_revision=args.expected_revision, apply=args.apply))
    report_path = root / "function-match-report.json"
    report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")
    return {"kind": "live-function-match-refresh", "stage_acceptance": False,
            "complete_census": report["checked_rows"] == report["selected_rows"], "matched_functions": len(updates),
            "unmatched_functions": len(failed), "match_counts": report.get("match_counts", {}),
            "annotation_edits": [{"path": edit["path"], "annotations_changed": edit["annotations_changed"]} for edit in edits],
            "annotation_exclusions": excluded, "report": str(report_path), "commit": commit.to_dict()}


def build_parser():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("operation", choices=("review-instruction", "review-commutative", "refresh"))
    parser.add_argument("--progress", type=Path, default=DEFAULT_PROGRESS_PATH)
    parser.add_argument("--expected-revision", type=int, required=True)
    parser.add_argument("--payload-file", type=Path)
    selector = parser.add_mutually_exclusive_group()
    selector.add_argument("--all", action="store_true")
    selector.add_argument("--at")
    parser.add_argument("--build-root", type=Path)
    mutation = parser.add_mutually_exclusive_group(required=True)
    mutation.add_argument("--apply", action="store_true")
    mutation.add_argument("--dry-run", action="store_true")
    parser.add_argument("--json", action="store_true")
    return parser


def main(argv=None):
    configure_stdio()
    args = build_parser().parse_args(argv)
    try:
        store = ProgressStore(args.progress)
        document = store.load()
        if document.revision != args.expected_revision:
            raise ConcurrentProgressUpdate(f"revision changed: expected {args.expected_revision}, found {document.revision}")
        if args.operation.startswith("review-"):
            if args.payload_file is None:
                raise ProgressError(args.operation + " requires --payload-file")
            result = _review(args, document, store)
        else:
            if not (args.all or args.at) or args.build_root is None:
                raise ProgressError("refresh requires --all or --at and a fresh --build-root")
            result = _refresh(args, document, store)
        print(json.dumps(result, indent=2))
        return 0
    except (ValueError, OSError, ProgressError) as exc:
        print(f"function-match error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
