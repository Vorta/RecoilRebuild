#!/usr/bin/env python3
"""Read-only audit of canonical source-to-retail artifact trace topology."""

from __future__ import annotations

import argparse
from dataclasses import asdict
import json
from pathlib import Path
import sys

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from _recoil.lib.source_traceability import (
    SourceArtifactIndex,
    SourceTraceArtifact,
    SourceTraceDocument,
    SourceTraceFinding,
    load_artifact_rows,
    merge_source_trace_documents,
    parse_source_trace_path,
    validate_source_trace,
)
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH
from _recoil.lib.tooling import REPO_ROOT, SOURCE_SUFFIXES, iter_source_files


def _resolve_path(value: str, repo_root: Path) -> Path:
    path = Path(value)
    return path if path.is_absolute() else repo_root / path


def source_paths(values: list[str], repo_root: Path) -> tuple[Path, ...]:
    requested = values or ["src"]
    paths: list[Path] = []
    seen: set[Path] = set()
    for value in requested:
        root = _resolve_path(value, repo_root)
        if root.is_file():
            candidates = [root]
        elif root.is_dir():
            candidates = iter_source_files(root, suffixes=SOURCE_SUFFIXES)
        else:
            raise ValueError(f"{root}: source-trace path does not exist")
        for candidate in candidates:
            resolved = candidate.resolve()
            if resolved not in seen:
                seen.add(resolved)
                paths.append(candidate)
    return tuple(sorted(paths, key=lambda item: item.as_posix().lower()))


def source_graph_paths(
    values: list[str],
    scoped_paths: tuple[Path, ...],
    repo_root: Path,
) -> tuple[Path, ...]:
    """Return the source universe needed to resolve focused migrated edges.

    A focused translation-unit audit still needs repository-wide comment
    context because the one canonical mirror may attach to a complete type or
    source-generation region in a header.  Paths outside the repository use
    their requested directory as the source universe so command fixtures and
    external focused probes retain the same semantics.
    """

    if not values:
        return scoped_paths

    repo_source_root = (repo_root / "src").resolve()
    roots: set[Path] = set()
    for path in scoped_paths:
        resolved = path.resolve()
        try:
            resolved.relative_to(repo_source_root)
        except ValueError:
            roots.add(resolved.parent if resolved.is_file() else resolved)
        else:
            roots.add(repo_source_root)

    paths: dict[Path, Path] = {path.resolve(): path for path in scoped_paths}
    for root in roots:
        for candidate in iter_source_files(root, suffixes=SOURCE_SUFFIXES):
            paths.setdefault(candidate.resolve(), candidate)
    return tuple(sorted(paths.values(), key=lambda item: item.as_posix().lower()))


def _dedupe_findings(
    findings: list[SourceTraceFinding],
) -> tuple[SourceTraceFinding, ...]:
    unique: dict[tuple[str, str, int, str | None, str | None], SourceTraceFinding] = {}
    for finding in findings:
        key = (
            finding.code,
            finding.path,
            finding.line,
            finding.anchor_id,
            finding.artifact_id,
        )
        unique.setdefault(key, finding)
    return tuple(unique.values())


def audit_documents(
    documents: tuple[SourceTraceDocument, ...],
    *,
    progress_path: Path,
    strict: bool,
) -> tuple[SourceTraceFinding, ...]:
    index = load_artifact_rows(progress_path)
    findings: list[SourceTraceFinding] = []
    for document in documents:
        findings.extend(validate_source_trace(document, index, strict=strict))
    findings.extend(merge_source_trace_documents(documents))
    return _dedupe_findings(findings)


def _translation_unit(artifact: SourceTraceArtifact) -> str | None:
    return artifact.path if Path(artifact.path).suffix.lower() in {".c", ".cc", ".cpp", ".cxx"} else None


def _inline_edge(
    artifact: SourceTraceArtifact,
    *,
    translation_unit: str | None = None,
) -> tuple[str, str, str, str, str | None]:
    return (
        str(artifact.anchor_id or ""),
        artifact.relation,
        artifact.section,
        artifact.artifact_id,
        _translation_unit(artifact) if translation_unit is None else translation_unit,
    )


def _tracker_edges(
    artifact_id: str,
    index: SourceArtifactIndex,
) -> tuple[tuple[str, str, str, str, str | None], ...]:
    row = index.resolve(artifact_id)
    if row is None:
        return ()
    trace = row.row.get("source_traceability")
    if not isinstance(trace, dict):
        return ()
    raw_edges = trace.get("source_edges")
    if not isinstance(raw_edges, list):
        return ()
    edges: list[tuple[str, str, str, str, str | None]] = []
    for raw in raw_edges:
        if not isinstance(raw, dict):
            continue
        context = raw.get("emission_context")
        translation_unit = (
            context.get("translation_unit")
            if isinstance(context, dict) and isinstance(context.get("translation_unit"), str)
            else None
        )
        edges.append(
            (
                str(raw.get("anchor_id") or ""),
                str(raw.get("relation") or ""),
                str(row.output_section or ""),
                row.artifact_id,
                translation_unit,
            )
        )
    return tuple(edges)


def migrated_graph_findings(
    documents: tuple[SourceTraceDocument, ...],
    index: SourceArtifactIndex,
    *,
    anchor_scope_paths: set[str] | None = None,
    emission_scope_paths: set[str] | None = None,
) -> tuple[SourceTraceFinding, ...]:
    findings: list[SourceTraceFinding] = []
    all_canonical = [
        artifact for document in documents for artifact in document.artifacts
    ]
    if anchor_scope_paths is None and emission_scope_paths is None:
        canonical = all_canonical
        selected_artifact_ids = {
            row.artifact_id for artifact in all_canonical
            if (row := index.resolve(artifact.artifact_id)) is not None
        }
    else:
        anchor_scope = anchor_scope_paths or set()
        emission_scope = emission_scope_paths or set()
        selected_artifact_ids = {
            row.artifact_id
            for artifact in all_canonical
            if artifact.path in anchor_scope
            and (row := index.resolve(artifact.artifact_id)) is not None
        }
        selected_artifact_ids.update(
            row.artifact_id
            for row in index.rows.values()
            if any(edge[-1] in emission_scope for edge in _tracker_edges(row.artifact_id, index))
        )
        canonical = [
            artifact
            for artifact in all_canonical
            if (row := index.resolve(artifact.artifact_id)) is not None
            and row.artifact_id in selected_artifact_ids
        ]
    source_edges: dict[str, set[tuple[str, str, str, str, str | None]]] = {}
    edge_artifacts: dict[tuple[str, str, str, str, str | None], SourceTraceArtifact] = {}
    physical_occurrences: dict[str, list[SourceTraceArtifact]] = {}
    for artifact in canonical:
        row = index.resolve(artifact.artifact_id)
        if row is None:
            continue
        translation_unit = _translation_unit(artifact)
        if translation_unit is None:
            matching_tus = {
                edge[-1]
                for edge in _tracker_edges(row.artifact_id, index)
                if edge[:4]
                == (
                    str(artifact.anchor_id or ""),
                    artifact.relation,
                    artifact.section,
                    row.artifact_id,
                )
                and edge[-1]
            }
            if len(matching_tus) == 1:
                translation_unit = next(iter(matching_tus))
            else:
                code = (
                    "ambiguous-translation-unit"
                    if len(matching_tus) > 1
                    else "unknown-translation-unit"
                )
                findings.append(
                    SourceTraceFinding(
                        code,
                        artifact.path,
                        artifact.line,
                        "header attachment path is distinct from emission_context.translation_unit; "
                        f"matching governed tracker translation units={sorted(matching_tus)}",
                        artifact.anchor_id,
                        artifact.artifact_id,
                    )
                )
        edge = _inline_edge(artifact, translation_unit=translation_unit)
        source_edges.setdefault(row.artifact_id, set()).add(edge)
        edge_artifacts[edge] = artifact
        if not row.logical:
            physical_occurrences.setdefault(row.artifact_id, []).append(artifact)
        trace = row.row.get("source_traceability")
        state = trace.get("state") if isinstance(trace, dict) else None
        if state != "resolved":
            findings.append(
                SourceTraceFinding(
                    "source-trace-state-not-resolved",
                    artifact.path,
                    artifact.line,
                    f"tracker row source_traceability.state is {state!r}, not 'resolved'",
                    artifact.anchor_id,
                    artifact.artifact_id,
                )
            )

    for _artifact_id, occurrences in physical_occurrences.items():
        if len(occurrences) <= 1:
            continue
        first = occurrences[0]
        for artifact in occurrences[1:]:
            findings.append(
                SourceTraceFinding(
                    "duplicate-physical-source-edge",
                    artifact.path,
                    artifact.line,
                    f"physical row is also sourced at {first.path}:{first.line}",
                    artifact.anchor_id,
                    artifact.artifact_id,
                )
            )

    artifact_ids = set(source_edges) | selected_artifact_ids

    for artifact_id in sorted(artifact_ids):
        inline = source_edges.get(artifact_id, set())
        tracker = set(_tracker_edges(artifact_id, index))
        row = index.resolve(artifact_id)
        if row is None:
            continue
        source_by_key = {(edge[0], edge[1], edge[3]) for edge in inline}
        tracker_by_key = {(edge[0], edge[1], edge[3]) for edge in tracker}
        if inline and tracker and inline != tracker:
            artifact = next(
                (
                    item
                    for item in canonical
                    if index.resolve(item.artifact_id) is not None
                    and index.resolve(item.artifact_id).artifact_id == artifact_id
                ),
                None,
            )
            findings.append(
                SourceTraceFinding(
                    "source-tracker-edge-mismatch",
                    artifact.path if artifact is not None else "<tracker>",
                    artifact.line if artifact is not None else 0,
                    f"inline and tracker edge tuples differ for {artifact_id!r}",
                    artifact.anchor_id if artifact is not None else None,
                    artifact_id,
                )
            )
        for edge in sorted(inline - tracker, key=str):
            artifact = edge_artifacts[edge]
            findings.append(
                SourceTraceFinding(
                    "source-edge-missing-tracker",
                    artifact.path,
                    artifact.line,
                    f"inline edge is absent from tracker source_edges: {edge!r}",
                    artifact.anchor_id,
                    artifact.artifact_id,
                )
            )
        for edge in sorted(tracker - inline, key=str):
            findings.append(
                SourceTraceFinding(
                    "tracker-edge-missing-source",
                    str(edge[-1] or "<tracker>"),
                    0,
                    f"tracker source edge has no canonical inline edge: {edge!r}",
                    edge[0] or None,
                    edge[3],
                )
            )
        if source_by_key != tracker_by_key and inline and tracker:
            # The broad mismatch above is machine-useful even if the only
            # divergence is output section or translation-unit context.
            pass
    return _dedupe_findings(findings)


def report_payload(
    documents: tuple[SourceTraceDocument, ...],
    findings: tuple[SourceTraceFinding, ...],
    *,
    progress_path: Path,
) -> dict[str, object]:
    parsed_legacy_count = sum(len(item.legacy_artifacts) for item in documents)
    unsupported_legacy_count = sum(
        len(item.unsupported_legacy_addresses) for item in documents
    )
    return {
        "schema": "source-trace-audit-v2",
        "result": "passed" if not findings else "failed",
        "progress": str(progress_path),
        "acceptance_effect": "none",
        "topology_only": True,
        "counts": {
            "files": len(documents),
            "anchors": sum(len(item.anchors) for item in documents),
            "canonical_artifacts": sum(len(item.artifacts) for item in documents),
            "legacy_syntax": parsed_legacy_count + unsupported_legacy_count,
            "findings": len(findings),
        },
        "files": [
            {
                "path": item.path,
                "encoding": item.encoding,
                "newline": item.newline,
                "anchors": len(item.anchors),
                "canonical_artifacts": len(item.artifacts),
                "legacy_syntax": len(item.legacy_artifacts)
                + len(item.unsupported_legacy_addresses),
            }
            for item in documents
        ],
        "findings": [asdict(item) for item in findings],
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Read-only canonical @recoil-anchor/@recoil-artifact topology audit. "
            "Rows are source observations and have no acceptance effect."
        )
    )
    parser.add_argument(
        "--progress",
        default=str(DEFAULT_PROGRESS_PATH),
        help="unified tracker used only for function/data/logical-id and output-section lookup",
    )
    parser.add_argument("--json", action="store_true", help="emit the complete JSON report")
    parser.add_argument("--max", type=int, default=100, help="maximum text findings to print")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    progress_path = _resolve_path(args.progress, REPO_ROOT)
    try:
        paths = source_paths([], REPO_ROOT)
        documents = tuple(
            parse_source_trace_path(
                path,
                repo_root=REPO_ROOT,
            )
            for path in paths
        )
        index = load_artifact_rows(progress_path)
        findings = list(audit_documents(
            documents,
            progress_path=progress_path,
            strict=True,
        ))
        graph_paths = source_graph_paths([], paths, REPO_ROOT)
        graph_documents = tuple(
            parse_source_trace_path(path, repo_root=REPO_ROOT)
            for path in graph_paths
        )
        anchor_scope_paths = {document.path for document in documents}
        emission_scope_paths = {
            document.path
            for document in documents
            if Path(document.path).suffix.lower() in {".c", ".cc", ".cpp", ".cxx"}
        }
        findings.extend(
            migrated_graph_findings(
                graph_documents,
                index,
                anchor_scope_paths=anchor_scope_paths,
                emission_scope_paths=emission_scope_paths,
            )
        )
        findings = _dedupe_findings(findings)
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        if args.json:
            print(
                json.dumps(
                    {
                        "schema": "source-trace-audit-v2",
                        "result": "error",
                        "acceptance_effect": "none",
                        "error": str(exc),
                    },
                    indent=2,
                    sort_keys=True,
                )
            )
        else:
            print(f"source-trace audit error: {exc}", file=sys.stderr)
        return 1

    payload = report_payload(
        documents,
        findings,
        progress_path=progress_path,
    )
    if args.json:
        print(json.dumps(payload, indent=2, sort_keys=True))
    else:
        counts = payload["counts"]
        print(
            "source-trace audit "
            f"{str(payload['result']).upper()}: files={counts['files']} "
            f"anchors={counts['anchors']} artifacts={counts['canonical_artifacts']} "
            f"legacy_syntax={counts['legacy_syntax']} findings={counts['findings']}"
        )
        print("Source-trace rows are read-only topology observations; acceptance effect: none.")
        limit = max(int(args.max), 0)
        for finding in findings[:limit]:
            print(f"{finding.path}:{finding.line}: {finding.code}: {finding.message}")
        if limit and len(findings) > limit:
            print(f"... {len(findings) - limit} more")
    return 0 if not findings else 1


if __name__ == "__main__":
    sys.exit(main())
