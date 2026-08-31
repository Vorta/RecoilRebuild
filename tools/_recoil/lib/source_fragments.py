"""Mechanical inventory and closure checks for production source fragments."""

from __future__ import annotations

from dataclasses import asdict, dataclass
from pathlib import Path
import re
from typing import Iterable


PRODUCTION_SUFFIXES = frozenset({".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".inl"})
SOURCE_SUFFIXES = frozenset({".c", ".cc", ".cpp", ".cxx"})
FRAGMENT_HEADER_RE = re.compile(r"(?:_body|_impl|_impl_body)\.h$", re.IGNORECASE)
QUOTED_INCLUDE_RE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.MULTILINE)


@dataclass(frozen=True)
class IncludeEdge:
    source: str
    line: int
    include: str
    target: str
    edge_scope: str


def is_fragment_header(path: Path) -> bool:
    return bool(FRAGMENT_HEADER_RE.search(path.name))


def _display_path(path: Path, repo_root: Path) -> str:
    resolved = path.resolve()
    try:
        return resolved.relative_to(repo_root.resolve()).as_posix()
    except ValueError:
        return resolved.as_posix()


def _is_within(path: Path, root: Path) -> bool:
    try:
        path.resolve().relative_to(root.resolve())
    except ValueError:
        return False
    return True


def _resolve_include(
    include_text: str,
    *,
    including_source: Path,
    repo_root: Path,
    source_root: Path,
) -> Path | None:
    include_path = Path(include_text)
    candidates = (
        including_source.parent / include_path,
        repo_root / include_path,
        source_root / include_path,
    )
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    return None


def _include_rows(path: Path, *, repo_root: Path, source_root: Path) -> tuple[tuple[int, str, Path], ...]:
    text = path.read_text(encoding="utf-8", errors="ignore")
    rows: list[tuple[int, str, Path]] = []
    for match in QUOTED_INCLUDE_RE.finditer(text):
        include_text = match.group(1)
        target = _resolve_include(
            include_text,
            including_source=path,
            repo_root=repo_root,
            source_root=source_root,
        )
        if target is None or not _is_within(target, source_root):
            continue
        line = text.count("\n", 0, match.start()) + 1
        rows.append((line, include_text, target))
    return tuple(rows)


def _production_files(source_root: Path) -> tuple[Path, ...]:
    return tuple(
        sorted(
            (
                path.resolve()
                for path in source_root.rglob("*")
                if path.is_file() and path.suffix.casefold() in PRODUCTION_SUFFIXES
            ),
            key=lambda path: path.as_posix().casefold(),
        )
    )


def inventory_source_fragments(
    source_root: Path,
    *,
    repo_root: Path,
) -> dict[str, object]:
    if not source_root.is_dir():
        raise ValueError(f"{source_root}: source-fragment root does not exist or is not a directory")
    files = _production_files(source_root)
    fragment_files = tuple(
        _display_path(path, repo_root) for path in files if is_fragment_header(path)
    )
    inl_files = tuple(
        _display_path(path, repo_root) for path in files if path.suffix.casefold() == ".inl"
    )
    fragment_edges: list[IncludeEdge] = []
    included_source_edges: list[IncludeEdge] = []
    for source in files:
        source_key = _display_path(source, repo_root)
        for line, include_text, target in _include_rows(
            source,
            repo_root=repo_root,
            source_root=source_root,
        ):
            target_key = _display_path(target, repo_root)
            if is_fragment_header(target):
                fragment_edges.append(
                    IncludeEdge(
                        source=source_key,
                        line=line,
                        include=include_text,
                        target=target_key,
                        edge_scope="nested" if is_fragment_header(source) else "direct",
                    )
                )
            if target.suffix.casefold() in SOURCE_SUFFIXES:
                included_source_edges.append(
                    IncludeEdge(
                        source=source_key,
                        line=line,
                        include=include_text,
                        target=target_key,
                        edge_scope="direct",
                    )
                )
    fragment_edges.sort(key=lambda item: (item.source.casefold(), item.line, item.target.casefold()))
    included_source_edges.sort(
        key=lambda item: (item.source.casefold(), item.line, item.target.casefold())
    )
    included_source_files = tuple(
        sorted({edge.target for edge in included_source_edges}, key=str.casefold)
    )
    direct_edges = sum(edge.edge_scope == "direct" for edge in fragment_edges)
    nested_edges = len(fragment_edges) - direct_edges
    counts = {
        "fragment_files": len(fragment_files),
        "fragment_include_edges": len(fragment_edges),
        "direct_fragment_include_edges": direct_edges,
        "nested_fragment_include_edges": nested_edges,
        "included_source_edges": len(included_source_edges),
        "included_source_files": len(included_source_files),
        "inl_files": len(inl_files),
    }
    counts["total_findings"] = (
        counts["fragment_files"]
        + counts["fragment_include_edges"]
        + counts["included_source_edges"]
        + counts["inl_files"]
    )
    return {
        "kind": "source-fragment-audit",
        "root": _display_path(source_root, repo_root),
        "ok": counts["total_findings"] == 0,
        "counts": counts,
        "findings": {
            "fragment_files": list(fragment_files),
            "fragment_include_edges": [asdict(edge) for edge in fragment_edges],
            "included_source_edges": [asdict(edge) for edge in included_source_edges],
            "included_source_files": list(included_source_files),
            "inl_files": list(inl_files),
        },
    }


def production_closure_fragment_findings(
    declared_paths: Iterable[str | Path],
    *,
    repo_root: Path,
    source_root: Path | None = None,
) -> tuple[dict[str, object], ...]:
    """Return forbidden forms in declared/transitive repo-local production closure."""

    production_root = (source_root or (repo_root / "src")).resolve()
    queue: list[tuple[Path, int]] = []
    for declared in declared_paths:
        path = Path(declared)
        if not path.is_absolute():
            path = repo_root / path
        if path.is_file() and _is_within(path, production_root):
            queue.append((path.resolve(), 0))
    visited: set[Path] = set()
    findings: list[dict[str, object]] = []
    seen_findings: set[tuple[object, ...]] = set()

    def add(finding: dict[str, object]) -> None:
        key = (
            finding.get("kind"),
            finding.get("path"),
            finding.get("source"),
            finding.get("line"),
            finding.get("target"),
        )
        if key not in seen_findings:
            seen_findings.add(key)
            findings.append(finding)

    while queue:
        path, depth = queue.pop(0)
        if path in visited:
            continue
        visited.add(path)
        path_key = _display_path(path, repo_root)
        if is_fragment_header(path):
            add({"kind": "fragment-file", "path": path_key, "depth": depth})
        if path.suffix.casefold() == ".inl":
            add({"kind": "inl-file", "path": path_key, "depth": depth})
        for line, include_text, target in _include_rows(
            path,
            repo_root=repo_root,
            source_root=production_root,
        ):
            target_key = _display_path(target, repo_root)
            if is_fragment_header(target):
                add(
                    {
                        "kind": "fragment-include-edge",
                        "source": path_key,
                        "line": line,
                        "include": include_text,
                        "target": target_key,
                        "depth": depth,
                        "edge_scope": "direct" if depth == 0 else "nested",
                    }
                )
            if target.suffix.casefold() in SOURCE_SUFFIXES:
                add(
                    {
                        "kind": "included-source-edge",
                        "source": path_key,
                        "line": line,
                        "include": include_text,
                        "target": target_key,
                        "depth": depth,
                        "edge_scope": "direct" if depth == 0 else "nested",
                    }
                )
            if target.suffix.casefold() in PRODUCTION_SUFFIXES and target not in visited:
                queue.append((target, depth + 1))
    return tuple(
        sorted(
            findings,
            key=lambda item: (
                str(item.get("kind", "")),
                str(item.get("path") or item.get("source") or "").casefold(),
                int(item.get("line", 0)),
                str(item.get("target", "")).casefold(),
            ),
        )
    )


def require_clean_production_closure(
    declared_paths: Iterable[str | Path],
    *,
    repo_root: Path,
    context: str,
) -> None:
    findings = production_closure_fragment_findings(declared_paths, repo_root=repo_root)
    if not findings:
        return
    rendered: list[str] = []
    for finding in findings:
        location = finding.get("path") or (
            f"{finding.get('source')}:{finding.get('line')} -> {finding.get('target')}"
        )
        rendered.append(f"{finding['kind']}: {location}")
    raise ValueError(
        f"{context}: forbidden production source-fragment closure:\n- "
        + "\n- ".join(rendered)
    )
