"""Shared source-comment row scanning and documentation-hygiene classification.

The scanner is deliberately lexical rather than C++-semantic.  It finds
Doxygen comments, ordinary block comments, and line comments while skipping
string and character literals.  Classification is limited to standalone
semantic rows; inline comments and wrapped continuations are retained.
"""

from __future__ import annotations

from dataclasses import dataclass
import re
from typing import Iterable


REDUNDANT_SOURCE_PATH = "redundant source-path prose"
REDUNDANT_MIGRATION_PLACEHOLDER = "redundant migration-placeholder prose"
DUPLICATE_COMMENT_PROSE = "duplicate comment prose"

_CANONICAL_ROW_RE = re.compile(r"^@recoil-[a-z0-9_-]+\b", re.IGNORECASE)
_PURPOSE_ROW_RE = re.compile(r"^Purpose\s*:", re.IGNORECASE)
_EVIDENCE_ROW_RE = re.compile(r"^Evidence\s*:", re.IGNORECASE)
_SOURCE_SUFFIX = r"(?:cxx|cpp|cc|c|hpp|inl|hh|h)(?![A-Za-z0-9_])"
_SOURCE_PATH_RE = re.compile(
    rf"""
    (?:
        [A-Za-z]:[\\/](?:[^\\/\r\n:*?"<>|]+[\\/])*[^\\/\r\n:*?"<>|]+\.{_SOURCE_SUFFIX}
      | (?:\.{{1,2}}[\\/])?(?:[A-Za-z0-9_.-]+[\\/])+[A-Za-z0-9_.-]+\.{_SOURCE_SUFFIX}
      | [A-Za-z0-9_.-]+\.{_SOURCE_SUFFIX}
    )
    """,
    re.IGNORECASE | re.VERBOSE,
)
_SOURCE_PATH_PREFIX_RE = re.compile(
    r"^(?:(?:original|provisional|retail|current)\s+)?source(?:\s+path)?\s*:\s*",
    re.IGNORECASE,
)
_OPERATOR_RE = (
    r"operator(?:\s*(?:\(\)|\[\]|new(?:\[\])?|delete(?:\[\])?|"
    r"[+\-*/%&|^~!=<>]=?|<<=?|>>=?|&&|\|\||,|->\*?|[A-Za-z_][A-Za-z0-9_:<> ]*))"
)
_SYMBOL_SEGMENT = rf"(?:~?[A-Za-z_$][A-Za-z0-9_$]*|{_OPERATOR_RE})"
_SYMBOL_RE = re.compile(
    rf"^(?P<symbol>{_SYMBOL_SEGMENT}(?:::{_SYMBOL_SEGMENT})*)(?:\s*\(\s*\))?$"
)
_MECHANICAL_PATTERNS = (
    re.compile(
        r"^(?:retail\s+)?(?:\S+\s+)?(?:physical[- ]contribution\s+)?"
        r"routing anchors?\.?$",
        re.IGNORECASE,
    ),
    re.compile(r"^routed to\s+\S+\.?$", re.IGNORECASE),
    re.compile(
        r"^(?:(?:the|these)\s+)?(?:(?:address[- ]backed|retail)\s+)?"
        r"(?:body|bodies)\s+(?:now\s+)?compiles?\s+from\s+\S+\.?$",
        re.IGNORECASE,
    ),
    re.compile(
        r"^(?:(?:compiler|vc5)[- ](?:generated|emitted)\s+)?"
        r"lifecycle contribution\.?$",
        re.IGNORECASE,
    ),
)


@dataclass(frozen=True)
class CommentRow:
    """One non-empty physical semantic row inside a source comment."""

    comment_id: int
    style: str
    line: int
    line_start: int
    line_end: int
    content_start: int
    content_end: int
    text: str
    raw_line: str
    standalone_comment: bool
    single_line_comment: bool
    continuation: bool


@dataclass(frozen=True)
class CommentHygieneFinding:
    line: int
    category: str
    text: str
    raw_line: str


def _line_number(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def _line_bounds(text: str, offset: int) -> tuple[int, int]:
    start = text.rfind("\n", 0, offset) + 1
    end = text.find("\n", offset)
    return start, len(text) if end < 0 else end + 1


def _comment_spans(text: str) -> tuple[tuple[int, int, str, bool], ...]:
    spans: list[tuple[int, int, str, bool]] = []
    index = 0
    while index < len(text):
        if text.startswith("//", index):
            line_start, line_end = _line_bounds(text, index)
            standalone = text[line_start:index].strip() == ""
            group_end = line_end
            while group_end < len(text):
                next_start, next_end = _line_bounds(text, group_end)
                next_line = text[next_start:next_end]
                marker = re.match(r"^[ \t]*//", next_line)
                if marker is None:
                    break
                group_end = next_end
            spans.append((index, group_end, "line", standalone))
            index = group_end
            continue
        if text.startswith("/*", index):
            close = text.find("*/", index + 2)
            if close < 0:
                close = len(text) - 2
            end = min(close + 2, len(text))
            line_start, _ = _line_bounds(text, index)
            standalone = text[line_start:index].strip() == ""
            style = "doxygen" if text.startswith("/**", index) else "block"
            spans.append((index, end, style, standalone))
            index = end
            continue
        if text[index] in {'"', "'"}:
            quote = text[index]
            index += 1
            while index < len(text):
                if text[index] == "\\":
                    index += 2
                    continue
                if text[index] == quote:
                    index += 1
                    break
                index += 1
            continue
        index += 1
    return tuple(spans)


def _strip_comment_line(
    raw: str,
    *,
    style: str,
    first: bool,
    last: bool,
) -> tuple[str, int, int]:
    """Return semantic text and its offsets relative to ``raw``."""

    body_end = len(raw.rstrip("\r\n"))
    start = 0
    end = body_end
    if style == "line":
        marker = raw.find("//")
        start = marker + 2
    else:
        if first:
            marker = raw.find("/**" if style == "doxygen" else "/*")
            start = marker + (3 if style == "doxygen" else 2)
        if last:
            marker = raw.rfind("*/", start, body_end)
            if marker >= 0:
                end = marker
        prefix = raw[start:end]
        star = re.match(r"^[ \t]*\*[ \t]?", prefix)
        if star is not None:
            start += star.end()
    while start < end and raw[start] in " \t":
        start += 1
    while end > start and raw[end - 1] in " \t":
        end -= 1
    return raw[start:end], start, end


def _is_wrapped_continuation(previous: str | None, current: str) -> bool:
    if previous is None:
        return False
    if re.sub(r"\s+", " ", previous).strip() == re.sub(r"\s+", " ", current).strip():
        return False
    if (
        classify_semantic_row(previous) is not None
        or is_legacy_migration_construct_title(previous)
    ):
        return False
    if _is_mechanical_placeholder(current):
        return False
    if _CANONICAL_ROW_RE.match(previous):
        return False
    if _PURPOSE_ROW_RE.match(previous):
        return previous[-1:] not in ".?!" and not (
            _CANONICAL_ROW_RE.match(current)
            or _PURPOSE_ROW_RE.match(current)
        )
    if previous.endswith((",", ";", ":", "-", "—")):
        return True
    if previous[-1:] not in ".?!":
        return True
    return bool(re.match(r"^(?:and|or|but|with|without|from|to|for|of|the|a|an)\b", current))


def comment_rows(text: str) -> tuple[CommentRow, ...]:
    """Return every non-empty physical semantic row in source comments."""

    result: list[CommentRow] = []
    for comment_id, (start, end, style, standalone) in enumerate(_comment_spans(text)):
        segment = text[start:end]
        pieces = segment.splitlines(keepends=True) or [segment]
        cursor = start
        previous: str | None = None
        for index, raw in enumerate(pieces):
            semantic, local_start, local_end = _strip_comment_line(
                raw,
                style=style,
                first=index == 0,
                last=index == len(pieces) - 1,
            )
            if semantic:
                line_start, line_end = _line_bounds(text, cursor)
                continuation = _is_wrapped_continuation(previous, semantic)
                result.append(
                    CommentRow(
                        comment_id=comment_id,
                        style=style,
                        line=_line_number(text, cursor),
                        line_start=line_start,
                        line_end=line_end,
                        content_start=cursor + local_start,
                        content_end=cursor + local_end,
                        text=semantic,
                        raw_line=text[line_start:line_end].rstrip("\r\n"),
                        standalone_comment=standalone,
                        single_line_comment=len(pieces) == 1,
                        continuation=continuation,
                    )
                )
                previous = semantic
            elif style != "line":
                previous = None
            cursor += len(raw)
    return tuple(result)


def _strip_title_punctuation(value: str) -> tuple[str, bool]:
    stripped = value.strip()
    terminal_period = stripped.endswith(".")
    if terminal_period:
        stripped = stripped[:-1].rstrip()
    return stripped, terminal_period


def _symbol_signal(symbol: str, *, known_construct_names: frozenset[str]) -> bool:
    if symbol in known_construct_names or symbol.rsplit("::", 1)[-1] in known_construct_names:
        return True
    if "::" in symbol or symbol.startswith("~") or "operator" in symbol:
        return True
    if "_" in symbol or "$" in symbol:
        return True
    return bool(re.search(r"[a-z][A-Z]|[A-Z].*[A-Z]", symbol))


def _is_symbol_title(
    value: str,
    *,
    known_construct_names: frozenset[str],
) -> bool:
    candidate, terminal_period = _strip_title_punctuation(value)
    matched = _SYMBOL_RE.fullmatch(candidate)
    if matched is None:
        return False
    symbol = re.sub(r"\s+", " ", matched.group("symbol")).strip()
    if not _symbol_signal(symbol, known_construct_names=known_construct_names):
        return False
    # A terminal period is normal for historical title rows.  Known attached
    # names and structurally qualified names are also unambiguous without it.
    return terminal_period or symbol in known_construct_names or "::" in symbol or "_" in symbol


def _is_source_path_title(
    value: str,
    *,
    known_construct_names: frozenset[str],
) -> bool:
    candidate, _ = _strip_title_punctuation(value)
    candidate = _SOURCE_PATH_PREFIX_RE.sub("", candidate).strip()
    matches = list(_SOURCE_PATH_RE.finditer(candidate))
    if len(matches) != 1:
        return False
    match = matches[0]
    before = candidate[: match.start()].strip(" \t:—-|()[]")
    after = candidate[match.end() :].strip(" \t:—-|()[]")
    if not before and not after:
        return True
    if after:
        return False
    symbol_candidate, _ = _strip_title_punctuation(before)
    symbol_match = _SYMBOL_RE.fullmatch(symbol_candidate)
    return bool(
        symbol_match is not None
        and _symbol_signal(
            re.sub(r"\s+", " ", symbol_match.group("symbol")).strip(),
            known_construct_names=known_construct_names,
        )
    )


def _is_mechanical_placeholder(value: str) -> bool:
    normalized = re.sub(r"\s+", " ", value).strip()
    return any(pattern.fullmatch(normalized) for pattern in _MECHANICAL_PATTERNS)


def classify_semantic_row(
    value: str,
    *,
    known_construct_names: Iterable[str] = (),
) -> str | None:
    """Classify an audited standalone semantic row, excluding duplicates.

    Standalone symbol titles are deliberately outside normal comment hygiene.
    ``known_construct_names`` remains relevant only for recognizing a
    symbol-plus-source-path row.
    """

    normalized = re.sub(r"\s+", " ", value).strip()
    if (
        not normalized
        or _CANONICAL_ROW_RE.match(normalized)
        or _PURPOSE_ROW_RE.match(normalized)
        or _EVIDENCE_ROW_RE.match(normalized)
    ):
        return None
    known = frozenset(name.strip() for name in known_construct_names if name.strip())
    if _is_mechanical_placeholder(normalized):
        return REDUNDANT_MIGRATION_PLACEHOLDER
    if _is_source_path_title(normalized, known_construct_names=known):
        return REDUNDANT_SOURCE_PATH
    return None


def is_legacy_migration_construct_title(value: str) -> bool:
    """Return whether reviewed legacy migration may delete a construct title.

    This predicate is intentionally migration-only.  Normal comment hygiene
    permits standalone symbol titles; only an explicit reviewed legacy
    ``Reimplements`` rewrite may treat one as removable migration residue.
    """

    normalized = re.sub(r"\s+", " ", value).strip()
    if (
        not normalized
        or _CANONICAL_ROW_RE.match(normalized)
        or _PURPOSE_ROW_RE.match(normalized)
        or _EVIDENCE_ROW_RE.match(normalized)
    ):
        return False
    candidate, _ = _strip_title_punctuation(normalized)
    matched = _SYMBOL_RE.fullmatch(candidate)
    if matched is None:
        return False
    symbol = re.sub(r"\s+", " ", matched.group("symbol")).strip()
    return _symbol_signal(symbol, known_construct_names=frozenset())


def audit_comment_hygiene(
    text: str,
    *,
    known_construct_names: Iterable[str] = (),
) -> tuple[CommentHygieneFinding, ...]:
    """Classify all eligible comment rows with stronger categories first."""

    findings: list[CommentHygieneFinding] = []
    seen_by_comment: dict[int, set[str]] = {}
    for row in comment_rows(text):
        normalized = re.sub(r"\s+", " ", row.text).strip()
        protected = bool(
            _CANONICAL_ROW_RE.match(normalized)
            or _PURPOSE_ROW_RE.match(normalized)
            or _EVIDENCE_ROW_RE.match(normalized)
        )
        seen = seen_by_comment.setdefault(row.comment_id, set())
        category = None
        if row.standalone_comment and not row.continuation and not protected:
            category = classify_semantic_row(
                normalized,
                known_construct_names=known_construct_names,
            )
            if category is None and normalized in seen:
                category = DUPLICATE_COMMENT_PROSE
        if (
            row.standalone_comment
            and not row.continuation
            and not protected
            and normalized
        ):
            seen.add(normalized)
        if category is not None:
            findings.append(
                CommentHygieneFinding(
                    line=row.line,
                    category=category,
                    text=row.text,
                    raw_line=row.raw_line,
                )
            )
    return tuple(findings)
