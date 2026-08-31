#!/usr/bin/env python3
"""Report production functions missing the required reconstruction docblock."""

from __future__ import annotations

import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
import re
import sys

from _recoil.lib.owner_entries import OwnerEntryIndex
from _recoil.lib.comment_hygiene import audit_comment_hygiene
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH
from _recoil.lib.source_constructs import (
    adjacent_comment,
    data_constructs,
    function_constructs,
    has_source_evidence_marker,
    has_source_provenance_evidence,
)
from _recoil.lib.source_traceability import parse_source_trace_text
from _recoil.lib.tooling import REPO_ROOT, SOURCE_SUFFIXES, display_path, iter_source_files, strip_comments_and_strings


FUNCTION_DEF_RE = re.compile(
    r"(?m)^[ \t]*"
    r"(?!if\b|for\b|while\b|switch\b|catch\b|else\b|do\b|return\b|sizeof\b)"
    r"(?P<signature>"
    r"(?:extern[ \t]+\"C\"[ \t]+)?"
    r"(?:static[ \t]+|inline[ \t]+|virtual[ \t]+)?"
    r"(?:[A-Za-z_~][A-Za-z0-9_:<>, \t\*&~]*?[ \t]+)?"
    r"(?:[\*&][ \t]*)*"
    r"(?P<name>(?:[A-Za-z_~][A-Za-z0-9_~]*::)*~?[A-Za-z_][A-Za-z0-9_]*)"
    r"[ \t]*\([^;{}]*?\)"
    r"(?:[ \t\r\n]+const)?"
    r"[ \t\r\n]*\{"
    r")",
    re.DOTALL,
)

BLOCK_COMMENT_PROVENANCE_RE = re.compile(
    r"\b(?:Reimplements\s+0x[0-9A-Fa-f]+|"
    r"[A-Za-z_~][A-Za-z0-9_:~]*\s+(?:--|—)\s*0x[0-9A-Fa-f]+)",
    re.IGNORECASE | re.DOTALL,
)

PURPOSE_RE = re.compile(r"(?mi)^[ \t]*(?:\*[ \t]*)?Purpose:[ \t]*\S")
CANONICAL_TRACE_RE = re.compile(
    r"@recoil-anchor\b[\s\S]*@recoil-artifact\b",
    re.IGNORECASE,
)
DATA_DECL_LINE_RE = re.compile(r"(?m)^[ \t]*(?!extern\b)(?!typedef\b)(?P<line>[^#\n;{}()]*;)")
IDENTIFIER_RE = re.compile(r"\b[A-Za-z_][A-Za-z0-9_]*\b")
TYPE_DECL_RE = re.compile(r"^[ \t]*(?:class|struct|enum)\b")
FUNCTION_DECL_RE = re.compile(
    r"^[ \t]*"
    r"(?!if\b|for\b|while\b|switch\b|catch\b|else\b|do\b|return\b|sizeof\b)"
    r"(?:extern[ \t]+\"C\"[ \t]+)?"
    r"(?:static[ \t]+|inline[ \t]+|virtual[ \t]+|const[ \t]+|unsigned[ \t]+|signed[ \t]+|"
    r"void[ \t]+|int[ \t]+|long[ \t]+|short[ \t]+|char[ \t]+|bool[ \t]+|BOOL[ \t]+|"
    r"DWORD[ \t]+|UINT[ \t]+|HRESULT[ \t]+|LRESULT[ \t]+|AFX_MSGMAP[ \t]+|"
    r"typedef[ \t]+[A-Za-z_][A-Za-z0-9_:<>, \t\*&~]*?[ \t]+|"
    r"[A-Za-z_~][A-Za-z0-9_:<>, \t\*&~]*?[ \t]+)"
    r"(?:[\*&][ \t]*)*"
    r"(?P<name>(?:[A-Za-z_~][A-Za-z0-9_~]*::)*~?[A-Za-z_][A-Za-z0-9_]*)?"
    r"[ \t]*\([^;{}]*\)"
    r"(?:[ \t]*const)?"
    r"[ \t]*(?:;|\{|:)?[ \t]*$",
)
@dataclass(frozen=True)
class FunctionDef:
    rel: str
    name: str
    start: int
    open_brace: int
    end: int
    line_no: int
    line: str
    text: str


@dataclass(frozen=True)
class DataDef:
    rel: str
    addresses: tuple[str, ...]
    name: str
    start: int
    end: int
    line_no: int
    line: str
    text: str


@dataclass(frozen=True)
class Finding:
    rel: str
    line_no: int
    label: str
    detail: str
    line: str


def line_number(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def matching_brace(text: str, open_brace: int) -> int:
    depth = 0
    for index in range(open_brace, len(text)):
        char = text[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return index + 1
    return len(text)


def paren_depth_at(text: str, offset: int) -> int:
    depth = 0
    for char in text[:offset]:
        if char == "(":
            depth += 1
        elif char == ")" and depth > 0:
            depth -= 1
    return depth


def collect_functions(path: Path, repo_root: Path, fallback_root: Path) -> list[FunctionDef]:
    original = path.read_text(encoding="utf-8", errors="ignore")
    rel = display_path(path, repo_root, fallback_root=fallback_root)
    lines = original.splitlines()
    functions: list[FunctionDef] = []

    for construct in function_constructs(original):
        line_no = construct.line
        line = lines[line_no - 1].strip() if 0 < line_no <= len(lines) else construct.name
        functions.append(
            FunctionDef(
                rel=rel,
                name=construct.name,
                start=construct.start,
                open_brace=construct.body_start or construct.start,
                end=construct.end,
                line_no=line_no,
                line=line,
                text=original,
            )
        )

    return filter_nested_function_matches(functions)


def filter_nested_function_matches(functions: list[FunctionDef]) -> list[FunctionDef]:
    filtered: list[FunctionDef] = []
    ordered = sorted(functions, key=lambda item: (item.start, item.end))
    for candidate in ordered:
        if any(other.start < candidate.start < other.end for other in ordered if other is not candidate):
            continue
        filtered.append(candidate)
    return filtered


def preceding_docblock(function: FunctionDef) -> str | None:
    return preceding_docblock_at(function.text, function.start)


def preceding_docblock_at(text: str, start_offset: int) -> str | None:
    prefix = text[:start_offset]
    end = len(prefix)
    while end > 0 and prefix[end - 1].isspace():
        end -= 1
    if not prefix[:end].endswith("*/"):
        return None
    start = prefix.rfind("/**", 0, end)
    if start < 0:
        return None
    return prefix[start:end]


def line_comment_group_before_docblock(text: str, docblock_start: int) -> tuple[int, str] | None:
    prefix = text[:docblock_start]
    end = len(prefix)
    while end > 0 and prefix[end - 1].isspace():
        end -= 1
    if end <= 0:
        return None

    line_start = prefix.rfind("\n", 0, end) + 1
    line = prefix[line_start:end]
    if not line.lstrip().startswith("//"):
        return None

    start = line_start
    cursor = line_start - 1
    while cursor > 0:
        previous_end = cursor
        previous_start = prefix.rfind("\n", 0, previous_end) + 1
        previous_line = prefix[previous_start:previous_end]
        if previous_line.lstrip().startswith("//"):
            start = previous_start
            cursor = previous_start - 1
            continue
        if previous_line.strip() == "":
            cursor = previous_start - 1
            continue
        break

    return line_number(text, start), text[start:end].strip()


def block_comment_before_docblock(text: str, docblock_start: int) -> tuple[int, str] | None:
    prefix = text[:docblock_start]
    end = len(prefix)
    while end > 0 and prefix[end - 1].isspace():
        end -= 1
    if end <= 0 or not prefix[:end].endswith("*/"):
        return None

    start = prefix.rfind("/*", 0, end)
    if start < 0:
        return None
    block = prefix[start:end]
    if block.startswith("/**"):
        return None
    if BLOCK_COMMENT_PROVENANCE_RE.search(block) is None:
        return None
    return line_number(text, start), block.strip()


def adjacent_line_comment_findings(rel: str, text: str) -> list[Finding]:
    findings: list[Finding] = []
    for match in re.finditer(r"(?m)^[ \t]*/\*\*", text):
        group = line_comment_group_before_docblock(text, match.start())
        if group is None:
            continue
        line_no, line = group
        findings.append(
            Finding(
                rel,
                line_no,
                "line comment should be unified with docblock",
                "line comments immediately before /** ... */ reconstruction docblocks must be merged into the docblock",
                line.splitlines()[0].strip(),
            )
        )
    return findings


def adjacent_block_comment_findings(rel: str, text: str) -> list[Finding]:
    findings: list[Finding] = []
    for match in re.finditer(r"(?m)^[ \t]*/\*\*", text):
        block = block_comment_before_docblock(text, match.start())
        if block is None:
            continue
        line_no, line = block
        findings.append(
            Finding(
                rel,
                line_no,
                "block comment should be unified with docblock",
                "non-docblock block comments immediately before /** ... */ reconstruction docblocks must be merged into the docblock",
                line.splitlines()[0].strip(),
            )
        )
    return findings


def line_offsets(text: str) -> list[int]:
    offsets = [0]
    for match in re.finditer(r"\n", text):
        offsets.append(match.end())
    return offsets


def declaration_kind(line: str) -> str | None:
    stripped = line.strip()
    if TYPE_DECL_RE.match(stripped):
        return "type"
    if FUNCTION_DECL_RE.match(stripped):
        return "function"
    return None


def line_comment_declaration_findings(
    rel: str,
    text: str,
    function_ranges: list[tuple[int, int]],
) -> list[Finding]:
    findings: list[Finding] = []
    lines = text.splitlines()
    offsets = line_offsets(text)
    index = 0
    while index < len(lines):
        if not lines[index].lstrip().startswith("//"):
            index += 1
            continue

        group_start = index
        while index < len(lines) and lines[index].lstrip().startswith("//"):
            index += 1
        next_index = index
        while next_index < len(lines) and lines[next_index].strip() == "":
            next_index += 1
        if next_index >= len(lines):
            continue
        if lines[next_index].lstrip().startswith("/**"):
            continue

        kind = declaration_kind(lines[next_index])
        if kind is None:
            continue
        group_offset = offsets[group_start]
        if offset_in_ranges(group_offset, function_ranges):
            continue
        findings.append(
            Finding(
                rel,
                group_start + 1,
                "line comment should be declaration docblock",
                f"line comments immediately before {kind} declarations must use /** ... */ docblocks",
                lines[group_start].strip(),
            )
        )
    return findings


def audit_function(
    function: FunctionDef,
    canonical_definition_starts: set[int] | None = None,
) -> list[Finding]:
    docblock = preceding_docblock(function)
    if docblock is None:
        comment = adjacent_comment(function.text, function.start)
        if function.start not in (canonical_definition_starts or set()) and (
            comment is None or not has_source_evidence_marker(comment)
        ):
            return []
        return [
            Finding(
                function.rel,
                function.line_no,
                "missing function docblock",
                f"{function.name} has a governed source/evidence claim and needs "
                "an immediately preceding /** ... */ docblock",
                function.line,
            )
        ]

    findings: list[Finding] = []
    canonical = (
        function.start in (canonical_definition_starts or set())
        or CANONICAL_TRACE_RE.search(docblock) is not None
    )
    if (
        has_source_evidence_marker(docblock)
        and not has_source_provenance_evidence(docblock)
        and not canonical
    ):
        findings.append(
            Finding(
                function.rel,
                function.line_no,
                "docblock missing provenance",
                f"{function.name} docblock must name original address, recovered inline/static/helper evidence, or provider boundary",
                function.line,
            )
        )
    governed = canonical or has_source_evidence_marker(docblock)
    if governed and PURPOSE_RE.search(docblock) is None:
        findings.append(
            Finding(
                function.rel,
                function.line_no,
                "docblock missing purpose",
                f"{function.name} docblock must include a non-empty Purpose: sentence",
                function.line,
            )
        )
    return findings


def plan_data_entries(owners_path: Path) -> dict[str, tuple[str, ...]]:
    try:
        doc = OwnerEntryIndex.load(owners_path)
    except FileNotFoundError:
        return {}
    entries: dict[str, list[str]] = {}
    for entry in doc.entries.values():
        if not entry.is_data_entry:
            continue
        name = entry.reimplemented_name or entry.reconstructed_name
        if not name:
            continue
        addresses = entries.setdefault(name, [])
        address = normalize_address(entry.address)
        if address not in addresses:
            addresses.append(address)
    return {name: tuple(addresses) for name, addresses in entries.items()}


def normalize_address(address: str) -> str:
    return f"0x{int(address, 16):06x}"


def looks_like_data_definition(line: str, name: str) -> bool:
    match = re.search(r"\b" + re.escape(name) + r"\b", line)
    if match is None:
        return False
    before = line[: match.start()].strip()
    after = line[match.end() :].lstrip()
    if not before:
        return False
    if re.match(r"^return\b", before):
        return False
    assignment = line.find("=")
    if assignment >= 0 and assignment < match.start():
        return False
    if after.startswith((".", "->")):
        return False
    return True


def source_data_name(name: str) -> str:
    if "::" in name:
        return name.rsplit("::", 1)[1]
    return name


def data_name_lookup(data_entries: dict[str, tuple[str, ...]]) -> dict[str, list[str]]:
    lookup: dict[str, list[str]] = {}
    for name in data_entries:
        source_name = source_data_name(name)
        if IDENTIFIER_RE.fullmatch(source_name) is None:
            continue
        lookup.setdefault(source_name, []).append(name)
    return lookup


def offset_in_ranges(offset: int, ranges: list[tuple[int, int]]) -> bool:
    return any(start <= offset < end for start, end in ranges)


def collect_data_defs(
    path: Path,
    repo_root: Path,
    fallback_root: Path,
    data_entries: dict[str, tuple[str, ...]],
    function_ranges: list[tuple[int, int]] | None = None,
) -> list[DataDef]:
    if not data_entries:
        return []
    original = path.read_text(encoding="utf-8", errors="ignore")
    stripped = strip_comments_and_strings(original)
    rel = display_path(path, repo_root, fallback_root=fallback_root)
    lines = original.splitlines()
    if function_ranges is None:
        function_ranges = [
            (function.start, function.end)
            for function in collect_functions(path, repo_root, fallback_root)
        ]

    lookup = data_name_lookup(data_entries)
    definitions_by_name: dict[str, list[DataDef]] = {name: [] for name in data_entries}
    for construct in data_constructs(original):
        start = construct.start
        if offset_in_ranges(start, function_ranges):
            continue
        candidate_names = list(lookup.get(construct.name, ()))
        if not candidate_names:
            continue
        line_no = line_number(stripped, start)
        line = lines[line_no - 1].strip() if 0 < line_no <= len(lines) else construct.name
        for name in candidate_names:
            if not looks_like_data_definition(line, source_data_name(name)):
                continue
            definitions_by_name[name].append(
                DataDef(rel, data_entries[name], name, start, construct.end, line_no, line, original)
            )
    definitions: list[DataDef] = []
    for name in data_entries:
        definitions.extend(sorted(definitions_by_name[name], key=lambda data_def: data_def.start))
    return definitions


def is_whitespace_between(text: str, start: int, end: int) -> bool:
    return text[start:end].strip() == ""


def grouped_data_docblock(data_def: DataDef, source_order: list[DataDef]) -> str | None:
    try:
        index = source_order.index(data_def)
    except ValueError:
        return None

    group_index = index
    while group_index > 0:
        previous = source_order[group_index - 1]
        current = source_order[group_index]
        if not is_whitespace_between(data_def.text, previous.end, current.start):
            return None
        group_index -= 1
        docblock = preceding_docblock_at(data_def.text, source_order[group_index].start)
        if docblock is not None:
            return docblock
    return None


def data_docblock(data_def: DataDef, source_order: list[DataDef] | None = None) -> str | None:
    docblock = preceding_docblock_at(data_def.text, data_def.start)
    if docblock is not None:
        return docblock
    if source_order is None:
        return None
    return grouped_data_docblock(data_def, source_order)


def audit_data_def(
    data_def: DataDef,
    source_order: list[DataDef] | None = None,
    *,
    canonical: bool = False,
) -> list[Finding]:
    docblock = data_docblock(data_def, source_order)
    if docblock is None:
        if not canonical:
            return []
        return [
            Finding(
                data_def.rel,
                data_def.line_no,
                "missing data docblock",
                f"{data_def.name} needs an immediately preceding /** ... */ docblock",
                data_def.line,
            )
        ]

    findings: list[Finding] = []
    canonical = canonical or CANONICAL_TRACE_RE.search(docblock) is not None
    claimed = has_source_evidence_marker(docblock)
    if claimed and not has_source_provenance_evidence(docblock) and not canonical:
        findings.append(
            Finding(
                data_def.rel,
                data_def.line_no,
                "data docblock missing provenance",
                f"{data_def.name} docblock asserts governed source evidence "
                "without structural original/recovered/provider proof",
                data_def.line,
            )
        )
    governed = canonical or claimed
    if governed and PURPOSE_RE.search(docblock) is None:
        findings.append(
            Finding(
                data_def.rel,
                data_def.line_no,
                "data docblock missing purpose",
                f"{data_def.name} docblock must include a non-empty Purpose: sentence",
                data_def.line,
            )
        )
    return findings


def source_files_for_path(path: Path) -> list[Path]:
    if path.is_file():
        return [path]
    if path.is_dir():
        return iter_source_files(path)
    raise FileNotFoundError(f"path does not exist: {path}")


def audit_paths(
    paths: list[Path],
    repo_root: Path,
    data_entries: dict[str, tuple[str, ...]] | None = None,
) -> list[Finding]:
    findings: list[Finding] = []
    data_entries = data_entries or {}
    for root in paths:
        for path in source_files_for_path(root):
            fallback_root = root if root.is_dir() else root.parent
            original = path.read_text(encoding="utf-8", errors="ignore")
            rel = display_path(path, repo_root, fallback_root=fallback_root)
            canonical_function_starts: set[int] = set()
            canonical_data: dict[int, DataDef] = {}
            if "@recoil-" in original:
                try:
                    trace = parse_source_trace_text(original, path=rel)
                except ValueError:
                    trace = None
                if trace is not None:
                    for artifact in trace.artifacts:
                        if (
                            artifact.direct
                            and artifact.relation == "defines"
                            and artifact.construct is not None
                        ):
                            if artifact.entity_kind == "function":
                                canonical_function_starts.add(artifact.construct.start)
                            elif artifact.entity_kind == "data":
                                construct = artifact.construct
                                lines = original.splitlines()
                                line_no = construct.line
                                line = (
                                    lines[line_no - 1].strip()
                                    if 0 < line_no <= len(lines)
                                    else construct.name
                                )
                                canonical_data.setdefault(
                                    construct.start,
                                    DataDef(
                                        rel,
                                        (artifact.artifact_id,),
                                        construct.name,
                                        construct.start,
                                        construct.end,
                                        line_no,
                                        line,
                                        original,
                                    ),
                                )
            functions = collect_functions(path, repo_root, fallback_root)
            function_ranges = [(function.start, function.end) for function in functions]
            findings.extend(adjacent_block_comment_findings(rel, original))
            findings.extend(adjacent_line_comment_findings(rel, original))
            findings.extend(line_comment_declaration_findings(rel, original, function_ranges))
            for function in functions:
                findings.extend(audit_function(function, canonical_function_starts))
            data_defs = collect_data_defs(
                path, repo_root, fallback_root, data_entries, function_ranges=function_ranges
            )
            known_data_starts = {data_def.start for data_def in data_defs}
            data_defs.extend(
                data_def
                for start, data_def in canonical_data.items()
                if start not in known_data_starts
            )
            source_order_data_defs = sorted(data_defs, key=lambda data_def: data_def.start)
            for data_def in data_defs:
                findings.extend(
                    audit_data_def(
                        data_def,
                        source_order_data_defs,
                        canonical=data_def.start in canonical_data,
                    )
                )
            construct_names = {
                function.name
                for function in functions
            }
            construct_names.update(data_def.name for data_def in data_defs)
            for hygiene in audit_comment_hygiene(
                original,
                known_construct_names=construct_names,
            ):
                findings.append(
                    Finding(
                        rel,
                        hygiene.line,
                        hygiene.category,
                        "standalone source-path, symbol-plus-path, routing/lifecycle, "
                        "or duplicate rows are not documentation; keep canonical "
                        "artifact identity and explain purpose or evidence",
                        hygiene.raw_line.strip(),
                    )
                )
    return sorted(
        findings,
        key=lambda finding: (finding.rel.casefold(), finding.line_no, finding.label),
    )


def print_report(findings: list[Finding], *, summary: bool, max_items: int) -> None:
    if summary:
        print("function docblock audit summary:")
        print(f"- findings: {len(findings)}")
        by_label = Counter(finding.label for finding in findings)
        for label, count in by_label.most_common():
            print(f"  {count:4}  {label}")
        if findings:
            print()

    limit = max(max_items, 0)
    shown = findings[:limit] if limit else []
    for finding in shown:
        print(f"{finding.rel}:{finding.line_no}: {finding.label}: {finding.detail}")
        print(f"  {finding.line}")
    if limit and len(findings) > limit:
        print(f"... {len(findings) - limit} more")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Audit reconstruction docblock structure and standalone source-comment "
            "documentation hygiene."
        )
    )
    parser.add_argument(
        "--path",
        dest="path",
        action="append",
        default=[],
        help="source file or directory to audit; defaults to src",
    )
    parser.add_argument("--summary", action="store_true", help="print finding counts by type")
    parser.add_argument("--max", type=int, default=80, help="maximum findings to print")
    parser.add_argument(
        "--progress",
        default=str(DEFAULT_PROGRESS_PATH),
        help="Unified progress tracker used for data-global classification.",
    )
    return parser


def resolve_audit_paths(path_values: list[str], repo_root: Path) -> list[Path]:
    requested = [Path(value) for value in path_values] if path_values else [Path("src")]
    resolved_paths: list[Path] = []
    for path in requested:
        resolved = path if path.is_absolute() else repo_root / path
        resolved_paths.append(resolved)
    return resolved_paths


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    resolved_paths = resolve_audit_paths(args.path, REPO_ROOT)
    owners_path = Path(args.progress)
    if not owners_path.is_absolute():
        owners_path = REPO_ROOT / owners_path

    try:
        findings = audit_paths(resolved_paths, REPO_ROOT, plan_data_entries(owners_path))
    except OSError as exc:
        print(exc, file=sys.stderr)
        return 1

    print_report(findings, summary=args.summary, max_items=args.max)
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
