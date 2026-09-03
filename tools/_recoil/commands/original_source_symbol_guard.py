#!/usr/bin/env python3
"""Fail when production source uses unsupported reconstruction helpers."""

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

from _recoil.lib.owner_entries import DONE_STATUS, TIER_COVERAGE, load_owner_entries, tier_at_least
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH
from _recoil.lib.source_constructs import (
    adjacent_comment,
    function_constructs,
    has_source_evidence_marker,
    has_source_provenance_evidence,
)
from _recoil.lib.source_traceability import parse_source_trace_text
from _recoil.lib.tooling import REPO_ROOT, display_path, iter_source_files, strip_comments_and_strings


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

GLOBAL_INIT_RE = re.compile(
    r"(?m)^[ \t]*(?:extern[ \t]+\"C\"[ \t]+)?"
    r"(?P<decl>(?:const[ \t]+|static[ \t]+|volatile[ \t]+)*"
    r"[A-Za-z_][A-Za-z0-9_:<>, \t\*&]*?"
    r"(?P<name>[A-Za-z_][A-Za-z0-9_]*)"
    r"[ \t]*(?:\[[^\]]*\])?[ \t]*=[ \t]*(?P<init>.*?);)",
    re.DOTALL,
)

CALL_RE_TEMPLATE = r"\b{callee}\s*\("
QUALIFIED_CALL_RE_TEMPLATE = r"\b{callee}\s*\("


@dataclass(frozen=True)
class FunctionDef:
    path: Path
    rel: str
    name: str
    base_name: str
    start: int
    open_brace: int
    end: int
    line_no: int
    line: str
    proven: bool
    claimed: bool
    registered: bool
    helper_candidate: bool


@dataclass(frozen=True)
class GlobalInit:
    path: Path
    rel: str
    name: str
    init: str
    start: int
    end: int
    line_no: int
    line: str


@dataclass(frozen=True)
class Violation:
    rel: str
    line_no: int
    label: str
    detail: str
    line: str


def normalize_source_path(path_text: str) -> str:
    return path_text.strip().replace("\\", "/")


def default_owners_path() -> Path:
    return DEFAULT_PROGRESS_PATH


def resolve_owners_argument(owners_text: str) -> Path:
    if not owners_text:
        return default_owners_path()
    owners_path = Path(owners_text)
    if owners_path.is_absolute():
        return owners_path
    return REPO_ROOT / owners_path


def load_allowlist(path: Path) -> set[tuple[str, str]]:
    allowed: set[tuple[str, str]] = set()
    if not path.exists():
        return allowed

    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split()
        if len(parts) != 2:
            raise ValueError(f"invalid original-source symbol allowlist line: {raw_line}")
        rel, symbol = parts
        allowed.add((normalize_source_path(rel), symbol))
    return allowed


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


def looks_like_expression_signature(signature_text: str) -> bool:
    """Reject multiline condition/call expressions misread as definitions."""
    return any(token in signature_text for token in ("&&", "||", "!=", "==", "<=", ">="))


def is_builtin_allocation_operator(signature_text: str, name: str) -> bool:
    if name not in {"new", "delete"}:
        return False
    return re.search(r"\boperator\s+(?:new|delete)\s*\(", signature_text) is not None


def nearby_provenance_comment(original_text: str, stripped_text: str, offset: int) -> bool:
    del stripped_text
    comment = adjacent_comment(original_text, offset)
    return comment is not None and has_source_provenance_evidence(comment)


def function_source_comment(
    original_text: str,
    *,
    start: int,
    open_brace: int,
) -> str | None:
    """Return an attached comment, including VC-style split return types.

    Some legitimate definitions place the docblock between a multiline return
    type and the function name.  The shared construct starts at the return
    type, so a purely preceding lookup cannot see that structurally attached
    block.
    """

    comment = adjacent_comment(original_text, start)
    if comment is not None:
        return comment
    signature = original_text[start:open_brace]
    matches = tuple(re.finditer(r"/\*\*[\s\S]*?\*/", signature))
    return matches[-1].group(0) if matches else None


def file_matches(rel: str, candidates: set[str]) -> bool:
    normalized = normalize_source_path(rel)
    return any(
        normalized == candidate
        or normalized.endswith("/" + candidate)
        or candidate.endswith("/" + normalized)
        for candidate in candidates
    )


def owner_source_symbol_keys(owners_path: Path) -> set[tuple[str, str]]:
    result: set[tuple[str, str]] = set()
    for entry in load_owner_entries(owners_path).values():
        if entry.is_provider_boundary:
            continue
        rel = normalize_source_path(entry.reimplemented_file)
        if not rel or rel in {"pending", "external"}:
            continue
        names = {entry.reimplemented_name, entry.reconstructed_name}
        for name in names:
            if not name:
                continue
            result.add((rel, name))
            result.add((rel, name.split("::")[-1]))
    return result


def tier_c_or_better_files(owners_path: Path) -> set[str]:
    files: set[str] = set()
    for entry in load_owner_entries(owners_path).values():
        if entry.is_provider_boundary:
            continue
        if not tier_at_least(entry.reimplementation_tier, TIER_COVERAGE):
            continue
        rel = normalize_source_path(entry.reimplemented_file)
        if rel and rel not in {"pending", "external"}:
            files.add(rel)
    return files


def tier_c_or_better_source_symbol_keys(
    owners_path: Path,
) -> set[tuple[str, str]]:
    result: set[tuple[str, str]] = set()
    for entry in load_owner_entries(owners_path).values():
        if entry.is_provider_boundary:
            continue
        if not tier_at_least(entry.reimplementation_tier, TIER_COVERAGE):
            continue
        rel = normalize_source_path(entry.reimplemented_file)
        if not rel or rel in {"pending", "external"}:
            continue
        for name in {entry.reimplemented_name, entry.reconstructed_name}:
            if name:
                result.add((rel, name))
                result.add((rel, name.split("::")[-1]))
    return result


def source_equivalent_files(owners_path: Path) -> set[str]:
    files: set[str] = set()
    for entry in load_owner_entries(owners_path).values():
        if entry.is_provider_boundary:
            continue
        if entry.data_status != DONE_STATUS:
            continue
        rel = normalize_source_path(entry.reimplemented_file)
        if rel and rel not in {"pending", "external"}:
            files.add(rel)
    return files


def collect_functions(
    scan_root: Path,
    repo_root: Path,
    *,
    allowlist: set[tuple[str, str]],
    owner_symbols: set[tuple[str, str]],
) -> list[FunctionDef]:
    functions: list[FunctionDef] = []
    for path in iter_source_files(scan_root):
        original = path.read_text(encoding="utf-8", errors="ignore")
        stripped = strip_comments_and_strings(original)
        rel = display_path(path, repo_root, fallback_root=scan_root)
        lines = original.splitlines()
        canonical_definition_keys: set[tuple[int, str]] = set()
        canonical_definition_names: set[str] = set()
        if "@recoil-" in original:
            try:
                trace = parse_source_trace_text(original, path=rel)
            except ValueError:
                trace = None
            if trace is not None:
                invalid_anchors = {
                    finding.anchor_id
                    for finding in trace.findings
                    if finding.anchor_id is not None
                }
                invalid_artifacts = {
                    finding.artifact_id
                    for finding in trace.findings
                    if finding.artifact_id is not None
                }
                canonical_definition_keys = {
                    (artifact.construct.line, artifact.construct.name)
                    for artifact in trace.artifacts
                    if artifact.direct
                    and artifact.relation == "defines"
                    and artifact.entity_kind == "function"
                    and artifact.construct is not None
                    and artifact.comment_style == "doxygen"
                    and artifact.attachment_status == "attached"
                    and artifact.anchor_id not in invalid_anchors
                    and artifact.artifact_id not in invalid_artifacts
                }
                canonical_definition_names = {
                    artifact.construct.name
                    for artifact in trace.artifacts
                    if artifact.direct
                    and artifact.relation == "defines"
                    and artifact.entity_kind == "function"
                    and artifact.construct is not None
                    and artifact.comment_style == "doxygen"
                    and artifact.attachment_status == "attached"
                    and artifact.anchor_id not in invalid_anchors
                    and artifact.artifact_id not in invalid_artifacts
                }

        for construct in function_constructs(original):
            name = construct.name
            base_name = name.split("::")[-1]
            open_brace = construct.body_start or construct.start
            line_no = construct.line
            line = lines[line_no - 1].strip() if 0 < line_no <= len(lines) else name
            comment = function_source_comment(
                original,
                start=construct.start,
                open_brace=open_brace,
            )
            comment_proven = (
                comment is not None
                and has_source_provenance_evidence(comment)
            )
            canonical_comment = (
                comment is not None
                and "@recoil-anchor" in comment
                and "@recoil-artifact" in comment
                and construct.name in canonical_definition_names
            )
            allowlisted = any(
                symbol in {name, base_name}
                and file_matches(rel, {allowed_rel})
                for allowed_rel, symbol in allowlist
            )
            owner_claimed = any(
                symbol in {name, base_name}
                and file_matches(rel, {owner_rel})
                for owner_rel, symbol in owner_symbols
            )
            claimed = (
                comment is not None
                and has_source_evidence_marker(comment)
            )
            signature = original[construct.start:open_brace]
            helper_candidate = (
                claimed
                or re.search(r"\b(?:static|inline)\b", signature) is not None
            )
            functions.append(
                FunctionDef(
                    path=path,
                    rel=rel,
                    name=name,
                    base_name=base_name,
                    start=construct.start,
                    open_brace=open_brace,
                    end=construct.end,
                    line_no=line_no,
                    line=line,
                    proven=(
                        comment_proven
                        or allowlisted
                        or canonical_comment
                        or (construct.line, construct.name) in canonical_definition_keys
                    ),
                    claimed=claimed,
                    registered=owner_claimed,
                    helper_candidate=helper_candidate,
                )
            )
    return filter_nested_function_matches(functions)


def filter_nested_function_matches(functions: list[FunctionDef]) -> list[FunctionDef]:
    by_rel: dict[str, list[FunctionDef]] = {}
    for function in functions:
        by_rel.setdefault(function.rel, []).append(function)

    filtered: list[FunctionDef] = []
    for rel_functions in by_rel.values():
        ordered = sorted(rel_functions, key=lambda item: (item.start, item.end))
        for candidate in ordered:
            if any(
                other.start < candidate.start < other.end
                for other in ordered
                if other is not candidate
            ):
                continue
            filtered.append(candidate)
    return sorted(filtered, key=lambda item: (item.rel, item.start))


def in_ranges(offset: int, ranges: list[tuple[int, int]]) -> bool:
    return any(start <= offset < end for start, end in ranges)


def collect_globals(scan_root: Path, repo_root: Path, functions_by_rel: dict[str, list[FunctionDef]]) -> list[GlobalInit]:
    globals_: list[GlobalInit] = []
    for path in iter_source_files(scan_root):
        original = path.read_text(encoding="utf-8", errors="ignore")
        stripped = strip_comments_and_strings(original)
        rel = display_path(path, repo_root, fallback_root=scan_root)
        body_ranges = [(function.start, function.end) for function in functions_by_rel.get(rel, [])]
        lines = original.splitlines()

        for match in GLOBAL_INIT_RE.finditer(stripped):
            if in_ranges(match.start("decl"), body_ranges):
                continue
            line_no = line_number(stripped, match.start("decl"))
            line = lines[line_no - 1].strip() if 0 < line_no <= len(lines) else match.group("name")
            globals_.append(
                GlobalInit(
                    path=path,
                    rel=rel,
                    name=match.group("name"),
                    init=match.group("init"),
                    start=match.start("decl"),
                    end=match.end("decl"),
                    line_no=line_no,
                    line=line,
                )
            )
    return globals_


def unproven_call_patterns(functions: list[FunctionDef]) -> dict[str, re.Pattern[str]]:
    patterns: dict[str, re.Pattern[str]] = {}
    for function in functions:
        if (
            function.proven
            or function.registered
            or not function.helper_candidate
        ):
            continue
        if "::" in function.name:
            patterns.setdefault(
                function.name,
                re.compile(QUALIFIED_CALL_RE_TEMPLATE.format(callee=re.escape(function.name))),
            )
        else:
            patterns.setdefault(
                function.base_name,
                re.compile(CALL_RE_TEMPLATE.format(callee=re.escape(function.base_name))),
            )
    return patterns


def function_call_key(function: FunctionDef) -> str:
    return function.name if "::" in function.name else function.base_name


def line_text(text: str, offset: int) -> str:
    lines = text.splitlines()
    line_no = line_number(text, offset)
    if 0 < line_no <= len(lines):
        return lines[line_no - 1].strip()
    return ""


def find_violations(
    scan_root: Path,
    repo_root: Path,
    owners_path: Path,
    *,
    allowlist_path: Path,
) -> list[Violation]:
    allowlist = load_allowlist(allowlist_path)
    owner_symbols = owner_source_symbol_keys(owners_path)
    tier_c_symbols = tier_c_or_better_source_symbol_keys(owners_path)
    data_files = source_equivalent_files(owners_path)
    functions = collect_functions(
        scan_root,
        repo_root,
        allowlist=allowlist,
        owner_symbols=owner_symbols,
    )
    functions_by_rel: dict[str, list[FunctionDef]] = {}
    for function in functions:
        functions_by_rel.setdefault(function.rel, []).append(function)

    caller_violations: list[Violation] = []
    called_unproven: set[str] = set()
    call_patterns = unproven_call_patterns(functions)
    if call_patterns:
        for path in iter_source_files(scan_root):
            rel = display_path(path, repo_root, fallback_root=scan_root)
            original = path.read_text(encoding="utf-8", errors="ignore")
            stripped = strip_comments_and_strings(original)
            caller_ranges = [
                (function.open_brace + 1, function.end)
                for function in functions_by_rel.get(rel, [])
                if any(
                    symbol in {function.name, function.base_name}
                    and file_matches(rel, {owner_rel})
                    for owner_rel, symbol in tier_c_symbols
                )
            ]
            if not caller_ranges:
                continue
            for callee, pattern in call_patterns.items():
                for match in pattern.finditer(stripped):
                    if not in_ranges(match.start(), caller_ranges):
                        continue
                    called_unproven.add(callee)
                    caller_violations.append(
                        Violation(
                            rel,
                            line_number(stripped, match.start()),
                            "reimplemented caller uses unsupported helper",
                            f"call to {callee} invalidates affected callers as reimplementations",
                            line_text(original, match.start()),
                        )
                    )

    globals_ = collect_globals(scan_root, repo_root, functions_by_rel)
    if call_patterns:
        for global_init in globals_:
            for callee, pattern in call_patterns.items():
                if pattern.search(global_init.init) is None:
                    continue
                called_unproven.add(callee)
                label = "global initializer calls unsupported helper"
                detail = f"{global_init.name} calls {callee}; Data reimplemented cannot be accepted"
                if file_matches(global_init.rel, data_files):
                    label = "Data reimplemented global uses unsupported helper"
                caller_violations.append(
                    Violation(global_init.rel, global_init.line_no, label, detail, global_init.line)
                )

    definition_violations = [
        Violation(
            function.rel,
            function.line_no,
            "unproven production helper definition",
            f"{function.name} lacks original-source provenance",
            function.line,
        )
        for function in functions
        if not function.proven
        and (
            function.claimed
            or function_call_key(function) in called_unproven
        )
    ]
    return [*definition_violations, *caller_violations]


def print_summary(violations: list[Violation], *, max_items: int) -> None:
    print("original-source symbol provenance summary:")
    print(f"- violations: {len(violations)}")
    by_label = Counter(violation.label for violation in violations)
    for label, count in by_label.most_common():
        print(f"  {count:4}  {label}")
    definition_keys = {
        (item.rel, item.line_no, item.detail)
        for item in violations
        if item.label == "unproven production helper definition"
    }
    caller_keys = {
        (item.rel, item.line_no, item.detail)
        for item in violations
        if item.label in {
            "reimplemented caller uses unsupported helper",
            "global initializer calls unsupported helper",
            "Data reimplemented global uses unsupported helper",
        }
    }
    print(f"- unique unsupported definitions: {len(definition_keys)}")
    print(f"- affected caller/initializer sites: {len(caller_keys)}")
    if violations:
        print()
        print(f"First {min(max_items, len(violations))} violation(s):")
        for violation in violations[:max_items]:
            print(f"{violation.rel}:{violation.line_no}: {violation.label}: {violation.detail}")
            print(f"  {violation.line}")
        if len(violations) > max_items:
            print(f"... {len(violations) - max_items} more")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Reject production source that depends on unsupported reconstruction helpers."
    )
    parser.add_argument("--root", default="src", help="production source root to scan")
    parser.add_argument("--path", dest="root", help="alias for --root")
    parser.add_argument(
        "--progress",
        default="",
        help=(
            "unified progress path; defaults to the canonical tracker "
            "for the scanned source root"
        ),
    )
    parser.add_argument(
        "--allowlist",
        default=".agent/ORIGINAL_SOURCE_SYMBOL_ALLOWLIST.txt",
        help="optional '<repo-relative-path> <symbol>' provenance allowlist",
    )
    parser.add_argument("--max", type=int, default=80, help="maximum violations to print")
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    repo_root = REPO_ROOT
    scan_root = (repo_root / args.root).resolve()
    owners_path = resolve_owners_argument(args.progress).resolve()
    allowlist_path = (repo_root / args.allowlist).resolve()

    try:
        violations = find_violations(
            scan_root,
            repo_root,
            owners_path,
            allowlist_path=allowlist_path,
        )
    except ValueError as exc:
        print(exc, file=sys.stderr)
        return 1

    print_summary(violations, max_items=max(args.max, 0))
    if violations:
        print()
        print("Production source must use only address-backed functions, recovered inlined helpers, or provider boundaries.")
        print("Remove unsupported reconstruction helpers, document original inline/static/member helper evidence,")
        print("or downgrade affected callers to Reimplemented [X]/not done until proper source is recovered.")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
