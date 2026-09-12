"""Guarded current-source TU extraction, without changing retail identities.

This route records implementation topology only. It preserves the retail block
grid and semantic ownership, retracts unresolved broad filename inferences, and
invalidates affected proof state. It does not recover an original filename.
"""
from __future__ import annotations

import argparse
from copy import deepcopy
import json
from pathlib import Path
from typing import Any, Mapping

from _recoil.commands.source_trace_progress import (
    _iter_tracker_artifacts_mutable,
    normalize_source_traceability,
)
from _recoil.lib.progress import (
    DEFAULT_PROGRESS_PATH, ProgressDocument, ProgressError, ProgressStore,
    invalidate_order_dependencies,
)
from _recoil.lib.repository_paths import validate_repository_relative_path
from _recoil.lib.source_traceability import parse_source_trace_text
from _recoil.lib.tooling import REPO_ROOT, configure_stdio

SCHEMA = "recoil-source-path-extraction-v1"
FIELDS = {"schema", "reviewed", "reason", "binary", "old_source", "new_source",
          "expected_artifacts", "expected_owners", "expected_blocks",
          "expected_semantic_spans", "expected_verification_targets"}


def _source(path: str, root: Path) -> tuple[str, Any]:
    if not isinstance(path, str):
        raise ProgressError("TU extraction source path must be a string")
    path = validate_repository_relative_path(path, context="TU extraction source")
    if not path.startswith("src/") or Path(path).suffix.lower() not in {".c", ".cpp"}:
        raise ProgressError("TU extraction requires production .c/.cpp files")
    resolved = (root / path).resolve()
    if not resolved.is_relative_to((root / "src").resolve()) or not resolved.is_file():
        raise ProgressError(f"TU extraction source is absent or escapes src/: {path}")
    document = parse_source_trace_text(resolved.read_text(encoding="utf-8"), path=path)
    if document.findings:
        raise ProgressError(f"TU extraction source has invalid source anchors: {path}")
    return path, document


def _contains_path(value: Any, path: str) -> bool:
    if isinstance(value, str):
        return value == path
    if isinstance(value, Mapping):
        return any(_contains_path(k, path) or _contains_path(v, path) for k, v in value.items())
    if isinstance(value, list):
        return any(_contains_path(v, path) for v in value)
    return False


def extraction_snapshot(data: dict[str, Any], old_source: str, new_source: str,
                        *, root: Path = REPO_ROOT) -> dict[str, Any]:
    old_source, old_doc = _source(old_source, root)
    new_source, new_doc = _source(new_source, root)
    if old_source.casefold() == new_source.casefold():
        raise ProgressError("TU extraction requires two distinct retained source files")
    definitions = [a for a in new_doc.artifacts if a.relation == "defines"]
    if not definitions or any(a.entity_kind != "function" or a.section != ".text"
                              or not a.direct or a.construct is None for a in definitions):
        raise ProgressError("TU extraction supports a complete authored function-only TU")
    ids = [a.artifact_id for a in definitions]
    if len(ids) != len(set(ids)):
        raise ProgressError("TU extraction has duplicate function definitions")
    if set(ids) & {a.artifact_id for a in old_doc.artifacts if a.relation == "defines"}:
        raise ProgressError("extracted definitions still occur in the original source file")
    artifacts = {a.artifact_id: (a, row) for a, row in _iter_tracker_artifacts_mutable(data)}
    selected: dict[str, Any] = {}
    owner_ids: set[str] = set()
    physical_ids: set[str] = set()
    for definition in definitions:
        if definition.artifact_id not in artifacts:
            raise ProgressError(f"unknown extracted artifact: {definition.artifact_id}")
        artifact, row = artifacts[definition.artifact_id]
        if not definition.artifact_id.startswith("recoil:"):
            raise ProgressError("TU extraction currently supports only Recoil authored functions")
        if row.get("pipeline_class") not in {"authored", "authored-lifecycle"}:
            raise ProgressError(f"extracted artifact is not authored: {definition.artifact_id}")
        if not isinstance(row.get("source_traceability"), Mapping):
            raise ProgressError(f"register the current defining source edge first: {definition.artifact_id}")
        trace = normalize_source_traceability(row["source_traceability"])
        edges = trace["source_edges"]
        if (trace["state"] != "resolved" or len(edges) != 1
                or edges[0]["relation"] != "defines"
                or edges[0]["anchor_id"] != definition.anchor_id
                or edges[0]["emission_context"]["translation_unit"] not in {old_source, new_source}):
            raise ProgressError(f"extracted artifact lacks its exact old or synchronized current defining edge: {definition.artifact_id}")
        selected[definition.artifact_id] = deepcopy(row)
        physical_ids.add(artifact.parent_artifact_id or artifact.artifact_id)
        if artifact.parent_artifact_id:
            candidates = [row.get("owner_id")]
        else:
            candidates = [owner_id for owner_id, owner in data["owners"].items()
                          if any(r.get("kind") == "primary-function"
                                 and r.get("symbol_id") == artifact.artifact_id
                                 for r in owner.get("relationships", []) if isinstance(r, Mapping))]
        if len(candidates) != 1 or candidates[0] not in data["owners"]:
            raise ProgressError(f"extracted function has no exclusive current owner: {definition.artifact_id}")
        owner_ids.add(candidates[0])
    block_ids = {data["symbols"][s]["physical_block_id"] for s in physical_ids}
    span_ids = {span for s in physical_ids for span in data["symbols"][s].get("semantic_span_ids", [])}
    if not block_ids or not span_ids:
        raise ProgressError("TU extraction lacks complete block/span relationships")
    targets = {k: deepcopy(v) for k, v in data["verification_targets"].items()
               if v.get("binary") == "recoil" and _contains_path(v, new_source)}
    if not targets:
        raise ProgressError("TU extraction requires synchronized verification targets for the new source")
    return dict(schema=SCHEMA, reviewed=False, reason="", binary="recoil",
                old_source=old_source, new_source=new_source,
                expected_artifacts=selected,
                expected_owners={k: deepcopy(data["owners"][k]) for k in sorted(owner_ids)},
                expected_blocks={k: deepcopy(data["physical_blocks"][k]) for k in sorted(block_ids)},
                expected_semantic_spans={k: deepcopy(data["semantic_spans"][k]) for k in sorted(span_ids)},
                expected_verification_targets=targets)


def _apply_extraction(data: dict[str, Any], payload: Mapping[str, Any],
                      *, root: Path = REPO_ROOT) -> dict[str, Any]:
    if set(payload) != FIELDS or payload.get("schema") != SCHEMA or payload.get("binary") != "recoil":
        raise ProgressError("TU extraction requires the exact v1 payload fields")
    if payload.get("reviewed") is not True or not isinstance(payload.get("reason"), str) or not payload["reason"].strip():
        raise ProgressError("TU extraction requires a reviewed nonempty source-boundary reason")
    expected = extraction_snapshot(data, payload["old_source"], payload["new_source"], root=root)
    for field in FIELDS - {"reviewed", "reason"}:
        if payload[field] != expected[field]:
            raise ProgressError(f"TU extraction exact snapshot is stale or incomplete: {field}")
    before = ProgressDocument(data).pipeline("recoil")
    selected = set(payload["expected_artifacts"])
    old_source, new_source = payload["old_source"], payload["new_source"]
    # All source/order acceptance is re-proved. Historical accepted filename
    # mappings are outside this implementation-only operation. Evidence on an
    # unresolved mapping is an observation to preserve, not mapping acceptance.
    for block in payload["expected_blocks"].values():
        mapping = block.get("mapping", {})
        if mapping.get("state") != "unresolved":
            raise ProgressError("TU extraction cannot retract an accepted source-file mapping")
    artifacts = {a.artifact_id: (a, row) for a, row in _iter_tracker_artifacts_mutable(data)}
    physical_ids: set[str] = set()
    selected_by_block: dict[str, set[str]] = {k: set() for k in payload["expected_blocks"]}
    selected_by_span: dict[str, set[str]] = {k: set() for k in payload["expected_semantic_spans"]}
    changed_addresses: dict[str, set[str]] = {k: set() for k in payload["expected_owners"]}
    for artifact_id in selected:
        artifact, row = artifacts[artifact_id]
        row["source_traceability"]["source_edges"][0]["emission_context"]["translation_unit"] = new_source
        physical_id = artifact.parent_artifact_id or artifact_id
        physical_ids.add(physical_id)
        physical = data["symbols"][physical_id]
        address = physical["address"]
        selected_by_block[physical["physical_block_id"]].add(artifact_id)
        for span_id in physical.get("semantic_span_ids", []):
            selected_by_span[span_id].add(artifact_id)
        for owner_id in changed_addresses:
            owner = data["owners"][owner_id]
            if row.get("owner_id") == owner_id or any(
                    r.get("kind") == "primary-function" and r.get("symbol_id") == artifact_id
                    for r in owner.get("relationships", []) if isinstance(r, Mapping)):
                changed_addresses[owner_id].add(address)
    for owner_id, addresses in changed_addresses.items():
        owner = data["owners"][owner_id]
        paths = owner.get("source_paths")
        if not isinstance(paths, list) or len(paths) != len(set(paths)):
            raise ProgressError(f"TU extraction owner has malformed source_paths: {owner_id}")
        if new_source not in paths:
            paths.append(new_source)
        for address in addresses:
            metadata = owner.get("address_metadata", {}).get(address)
            if isinstance(metadata, dict):
                metadata["source_path"] = new_source
        for gate in ("source", "linkage", "owner_linkage", "byte"):
            if owner.get("gates", {}).get(gate) == "accepted":
                owner["gates"][gate] = "pending"
        for artifact_id, entry in owner.get("reimplementation", {}).get("entries", {}).items():
            if artifact_id in selected and entry.get("tier") in {"B", "A", "S"}:
                entry["tier"] = "C"
                entry["evidence_ids"] = []
    for block_id in payload["expected_blocks"]:
        block = data["physical_blocks"][block_id]
        history = block.setdefault("implementation_extractions", [])
        history.append(dict(old_source=old_source, new_source=new_source,
                            artifact_ids=sorted(selected_by_block[block_id]), reason=payload["reason"],
                            prior_original_source_path=block.get("original_source_path"),
                            prior_mapping=deepcopy(block["mapping"])))
        if not block["mapping"].get("evidence_ids"):
            block["original_source_path"] = None
            block["mapping"]["status"] = "unresolved original TU paths; reviewed current implementation extraction"
    for span_id in payload["expected_semantic_spans"]:
        span = data["semantic_spans"][span_id]
        span.setdefault("implementation_extractions", []).append(dict(
            old_source=old_source, new_source=new_source,
            artifact_ids=sorted(selected_by_span[span_id]), reason=payload["reason"],
            prior_source_path=span.get("source_path"), prior_status=span.get("status")))
        # Repeated extractions can leave one semantic span in several current
        # inputs. Derive those paths from all members, not just this move's old
        # and new input. Keep prior observations if any member is unresolved.
        members = span.get("symbol_ids", [])
        complete = bool(members)
        current_paths = []
        for member_id in members:
            member = artifacts.get(member_id)
            trace = member[1].get("source_traceability", {}) if member else {}
            edges = trace.get("source_edges", [])
            if (trace.get("state") != "resolved" or len(edges) != 1
                    or edges[0].get("relation") != "defines"
                    or not edges[0].get("emission_context", {}).get("translation_unit")):
                complete = False
                continue
            current_paths.append(edges[0]["emission_context"]["translation_unit"])
        prior_paths = [path.strip() for path in str(span.get("source_path") or old_source).split(";")
                       if path.strip()]
        retained_paths = [path for path in prior_paths if not complete or path in current_paths]
        span["source_path"] = "; ".join(dict.fromkeys([*retained_paths, *current_paths, new_source]))
        span["status"] = "current implementation spans reviewed compilation units; historical paths unresolved"
    # Include every body in the changed compile/order groups, including shared
    # physical representatives whose other aliases remain in their current TUs.
    affected = set(physical_ids)
    for block_id in payload["expected_blocks"]:
        affected.update(data["physical_blocks"][block_id].get("contribution_ids", []))
    invalidated = invalidate_order_dependencies(
        data, block_ids=list(payload["expected_blocks"]), symbol_ids=sorted(affected))
    for symbol_id in affected:
        symbol = data["symbols"][symbol_id]
        for field in ("accepted_call_contract_facts", "accepted_order_facts"):
            symbol.pop(field, None)
    after = ProgressDocument(data).pipeline("recoil")
    phases = ["authored-function-order", "authored-call-contract", "authored-byte-match",
              "full-function-order", "linked-byte-match", "final-validation"]
    if phases.index(after["phase"]) > phases.index(before["phase"]):
        raise ProgressError("TU extraction unexpectedly advanced acceptance")
    return dict(kind="source-path-extraction", accepted=False, artifact_ids=sorted(selected),
                owner_ids=sorted(changed_addresses), invalidated_order=invalidated,
                invalidated_symbol_ids=sorted(affected), scheduler_before=before, scheduler_after=after)


def apply_extraction(data: dict[str, Any], payload: Mapping[str, Any],
                     *, root: Path = REPO_ROOT) -> dict[str, Any]:
    """Keep even an in-memory caller unchanged if any guard or postcondition fails."""
    if not isinstance(payload, Mapping):
        raise ProgressError("TU extraction payload must be an object")
    proposed = deepcopy(data)
    result = _apply_extraction(proposed, payload, root=root)
    before_errors = {str(f) for f in ProgressDocument(data).audit() if f.severity == "error"}
    after_errors = {str(f) for f in ProgressDocument(proposed).audit() if f.severity == "error"}
    if after_errors - before_errors:
        raise ProgressError("TU extraction introduced tracker invariant errors: " +
                            "; ".join(sorted(after_errors - before_errors)[:4]))
    data.clear()
    data.update(proposed)
    return result


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--progress", default=str(DEFAULT_PROGRESS_PATH))
    parser.add_argument("--expected-revision", type=int, required=True)
    parser.add_argument("--payload-file", type=Path)
    parser.add_argument("--old-source")
    parser.add_argument("--new-source")
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument("--prepare", action="store_true")
    modes.add_argument("--dry-run", action="store_true")
    modes.add_argument("--apply", action="store_true")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args(argv)
    try:
        store = ProgressStore(args.progress)
        document = store.load()
        if document.revision != args.expected_revision:
            raise ProgressError("TU extraction expected revision is stale")
        if args.prepare:
            if not args.old_source or not args.new_source or args.payload_file:
                raise ProgressError("prepare requires old/new source paths and no input payload")
            result = extraction_snapshot(document.data, args.old_source, args.new_source)
        else:
            if args.payload_file is None or args.old_source or args.new_source:
                raise ProgressError("mutation requires only a reviewed payload file")
            path = args.payload_file.resolve()
            if not path.is_relative_to((REPO_ROOT / "build").resolve()):
                raise ProgressError("reviewed extraction payload must be below workspace build/")
            payload = json.loads(path.read_text(encoding="utf-8"))
            details: dict[str, Any] = {}
            def transform(data: dict[str, Any]) -> None:
                details.update(apply_extraction(data, payload))
            commit = store.mutate(transform, expected_revision=args.expected_revision, apply=args.apply)
            result = dict(details, commit=commit.to_dict())
        print(json.dumps(result, indent=2))
        return 0
    except (ProgressError, ValueError, OSError, KeyError, TypeError) as exc:
        print(f"ERROR: {exc}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
