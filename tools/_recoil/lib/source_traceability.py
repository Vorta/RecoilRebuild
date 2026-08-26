from __future__ import annotations

"""Canonical source-to-retail artifact trace directives and topology.

The rows produced here are source topology only.  They do not accept progress
state, owner gates, tiers, order, byte identity, or final-image facts.
"""

from dataclasses import dataclass
import json
from pathlib import Path
import re
from typing import Any, Mapping

from _recoil.lib.source_constructs import parse_source_constructs
from _recoil.lib.progress import ProgressStore

RELATIONS = frozenset({"defines", "emits"})
ENTITY_KINDS = frozenset({"function", "data"})
AUTHORED_FUNCTION_CLASSES = frozenset({"authored", "authored-lifecycle"})
SOURCE_TRACE_SUFFIXES = frozenset(
    {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".inl"}
)
ANCHOR_RE = re.compile(r"^@recoil-anchor[ \t]+(\S+)[ \t]*$")
ARTIFACT_RE = re.compile(
    r"^@recoil-artifact[ \t]+(\S+)[ \t]+(\S+)[ \t]+(\S+):[ \t]*(\S.*)$"
)
LEGACY_REIMPLEMENTS_RE = re.compile(
    r"^Reimplements[ \t]+(?:(data)[ \t]+)?(0x[0-9A-Fa-f]+):[ \t]*(\S.*)",
    re.IGNORECASE,
)
LEGACY_EMITS_RE = re.compile(
    r"^Emits[ \t]+(0x[0-9A-Fa-f]+):[ \t]*(\S.*)",
    re.IGNORECASE,
)
LEGACY_ADDRESS_CATCHALL_RE = re.compile(
    r"\b(?P<marker>Reimplements|Emits)\b[^\r\n]*?"
    r"\b(?P<address>0x[0-9A-Fa-f]+)\b",
    re.IGNORECASE,
)
PHYSICAL_FUNCTION_RE = re.compile(r"^([a-z0-9_-]+):function:(0x[0-9a-f]+)$")
PHYSICAL_DATA_RE = re.compile(r"^([a-z0-9_-]+):data:(0x[0-9a-f]+)$")
LOGICAL_FUNCTION_RE = re.compile(
    r"^([a-z0-9_-]+):logical-function:(0x[0-9a-f]+):([a-z0-9][a-z0-9_-]*)$"
)
LOGICAL_DATA_RE = re.compile(
    r"^([a-z0-9_-]+):logical-data:(0x[0-9a-f]+):([a-z0-9][a-z0-9_-]*)$"
)
ANCHOR_ID_RE = re.compile(r"^recoil:anchor:[a-z0-9][a-z0-9._-]*$")


@dataclass(frozen=True)
class SourceTraceConstruct:
    kind: str
    name: str
    start: int
    end: int
    line: int


@dataclass(frozen=True)
class SourceTraceAnchor:
    anchor_id: str
    path: str
    line: int
    comment_start: int
    comment_end: int
    construct: SourceTraceConstruct | None
    comment_style: str = "doxygen"
    attachment_status: str = "attached"

    @property
    def attached(self) -> bool:
        return self.construct is not None


@dataclass(frozen=True)
class SourceTraceArtifact:
    relation: str
    section: str
    artifact_id: str
    description: str
    path: str
    line: int
    anchor_id: str | None
    direct: bool
    construct: SourceTraceConstruct | None
    legacy: bool = False
    comment_style: str = "doxygen"
    attachment_status: str = "attached"

    @property
    def entity_kind(self) -> str | None:
        return artifact_entity_kind(self.artifact_id)

    @property
    def address(self) -> str | None:
        return artifact_address(self.artifact_id)


@dataclass(frozen=True)
class SourceTraceFinding:
    code: str
    path: str
    line: int
    message: str
    anchor_id: str | None = None
    artifact_id: str | None = None


@dataclass(frozen=True)
class SourceTraceLegacyAddress:
    marker: str
    address: str
    text: str
    path: str
    line: int
    comment_style: str


@dataclass(frozen=True)
class SourceTraceDocument:
    path: str
    anchors: tuple[SourceTraceAnchor, ...]
    artifacts: tuple[SourceTraceArtifact, ...]
    legacy_artifacts: tuple[SourceTraceArtifact, ...]
    findings: tuple[SourceTraceFinding, ...]
    encoding: str = "memory"
    newline: str = "none"
    unsupported_legacy_addresses: tuple[SourceTraceLegacyAddress, ...] = ()

    def direct_defining_function_at(self, offset: int) -> SourceTraceArtifact | None:
        """Return the sole direct `defines` function artifact enclosing offset.

        Legacy `Reimplements` inventory and `emits` rows deliberately do not
        qualify for address-sensitive policy such as raw-assembly allowlisting.
        """

        matches = [
            item
            for item in self.artifacts
            if item.direct
            and item.relation == "defines"
            and item.entity_kind == "function"
            and item.construct is not None
            and item.construct.start <= offset < item.construct.end
        ]
        return matches[0] if len(matches) == 1 else None


@dataclass(frozen=True)
class SourceArtifactRow:
    artifact_id: str
    physical_id: str
    kind: str
    output_section_id: str | None
    row: Mapping[str, Any]
    logical: bool = False

    @property
    def output_section(self) -> str | None:
        binary = self.physical_id.split(":", 1)[0]
        prefix = f"{binary}:section:"
        if isinstance(self.output_section_id, str) and self.output_section_id.startswith(prefix):
            return self.output_section_id[len(prefix) :]
        return None


@dataclass(frozen=True)
class SourceArtifactIndex:
    rows: Mapping[str, SourceArtifactRow]
    output_sections: frozenset[str]
    progress_path: str

    def resolve(self, artifact_id: str) -> SourceArtifactRow | None:
        return self.rows.get(normalize_artifact_id(artifact_id))


@dataclass(frozen=True)
class _Comment:
    start: int
    end: int
    text: str
    line: int
    style: str


def _line_number(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def normalize_address(value: str) -> str:
    try:
        return f"0x{int(value, 16):x}"
    except (TypeError, ValueError) as exc:
        raise ValueError(f"invalid retail address {value!r}") from exc


def normalize_artifact_id(value: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise ValueError("artifact id must be a non-empty string")
    raw = value.strip()
    for pattern, build in (
        (PHYSICAL_FUNCTION_RE, lambda match: f"{match.group(1)}:function:{normalize_address(match.group(2))}"),
        (PHYSICAL_DATA_RE, lambda match: f"{match.group(1)}:data:{normalize_address(match.group(2))}"),
        (
            LOGICAL_FUNCTION_RE,
            lambda match: (
                f"{match.group(1)}:logical-function:{normalize_address(match.group(2))}:"
                f"{match.group(3)}"
            ),
        ),
        (
            LOGICAL_DATA_RE,
            lambda match: (
                f"{match.group(1)}:logical-data:{normalize_address(match.group(2))}:"
                f"{match.group(3)}"
            ),
        ),
    ):
        match = pattern.fullmatch(raw.lower())
        if match is not None:
            return build(match)
    return raw


def artifact_entity_kind(artifact_id: str) -> str | None:
    normalized = normalize_artifact_id(artifact_id)
    if PHYSICAL_FUNCTION_RE.fullmatch(normalized) or LOGICAL_FUNCTION_RE.fullmatch(normalized):
        return "function"
    if PHYSICAL_DATA_RE.fullmatch(normalized) or LOGICAL_DATA_RE.fullmatch(normalized):
        return "data"
    return None


def artifact_address(artifact_id: str) -> str | None:
    normalized = normalize_artifact_id(artifact_id)
    for pattern in (
        PHYSICAL_FUNCTION_RE,
        PHYSICAL_DATA_RE,
        LOGICAL_FUNCTION_RE,
        LOGICAL_DATA_RE,
    ):
        match = pattern.fullmatch(normalized)
        if match is not None:
            return normalize_address(match.group(2))
    return None


def _scan_comments(text: str) -> tuple[_Comment, ...]:
    """Lex comments without recognizing marker-looking string contents."""

    comments: list[_Comment] = []
    index = 0
    while index < len(text):
        if text.startswith("//", index):
            start = index
            end = text.find("\n", index + 2)
            end = len(text) if end < 0 else end
            # Treat immediately consecutive // comment lines as one attached
            # directive block.  Whitespace-only lines intentionally detach it.
            while end < len(text):
                next_start = end + 1
                line_end = text.find("\n", next_start)
                line_end = len(text) if line_end < 0 else line_end
                next_line = text[next_start:line_end]
                if not next_line.lstrip(" \t").startswith("//"):
                    break
                end = line_end
            comments.append(_Comment(start, end, text[start:end], _line_number(text, start), "line"))
            index = end
            continue
        if text.startswith("/*", index):
            start = index
            close = text.find("*/", index + 2)
            end = len(text) if close < 0 else close + 2
            style = "doxygen" if text.startswith("/**", start) else "block"
            comments.append(_Comment(start, end, text[start:end], _line_number(text, start), style))
            index = end
            continue
        if text[index] in {'"', "'"}:
            quote = text[index]
            index += 1
            while index < len(text):
                if text[index] == "\\":
                    index += 2
                elif index < len(text) and text[index] == quote:
                    index += 1
                    break
                else:
                    index += 1
            continue
        index += 1
    return tuple(comments)


def _comment_lines(comment: _Comment) -> tuple[tuple[int, str], ...]:
    rows: list[tuple[int, str]] = []
    for offset, raw_line in enumerate(comment.text.splitlines()):
        line = raw_line.strip()
        if line.startswith("//"):
            line = line[2:].strip()
        elif offset == 0 and line.startswith("/*"):
            line = line[2:].strip()
        if line.endswith("*/"):
            line = line[:-2].strip()
        if line.startswith("*"):
            line = line[1:].strip()
        rows.append((comment.line + offset, line))
    return tuple(rows)


def _mask_comments_and_literals(text: str) -> str:
    chars = list(text)
    index = 0
    while index < len(text):
        if text.startswith("//", index):
            end = text.find("\n", index + 2)
            end = len(text) if end < 0 else end
            for position in range(index, end):
                chars[position] = " "
            index = end
            continue
        if text.startswith("/*", index):
            close = text.find("*/", index + 2)
            end = len(text) if close < 0 else close + 2
            for position in range(index, end):
                if chars[position] not in "\r\n":
                    chars[position] = " "
            index = end
            continue
        if text[index] in {'"', "'"}:
            quote = text[index]
            chars[index] = " "
            index += 1
            while index < len(text):
                if text[index] == "\\":
                    chars[index] = " "
                    if index + 1 < len(text):
                        chars[index + 1] = " "
                    index += 2
                elif text[index] == quote:
                    chars[index] = " "
                    index += 1
                    break
                else:
                    if chars[index] not in "\r\n":
                        chars[index] = " "
                    index += 1
            continue
        index += 1
    return "".join(chars)


def _matching_brace(masked: str, open_brace: int) -> int:
    depth = 0
    for index in range(open_brace, len(masked)):
        if masked[index] == "{":
            depth += 1
        elif masked[index] == "}":
            depth -= 1
            if depth == 0:
                return index + 1
    return len(masked)


def _attachment_after(
    text: str,
    masked: str,
    comment: _Comment,
) -> tuple[SourceTraceConstruct | None, str]:
    start = comment.end
    while start < len(masked) and masked[start].isspace():
        start += 1
    if start >= len(masked):
        return None, "eof"
    if text[comment.end:start].strip():
        return None, "stacked-comment"

    # Another comment or preprocessor directive is a real attachment boundary.
    if text.startswith(("//", "/*"), start):
        return None, "stacked-comment"
    if masked[start] == "#":
        return None, "preprocessor"
    macro_match = re.match(r"([A-Z][A-Z0-9_]+)\s*\(", masked[start:])
    if macro_match is not None:
        for parsed in parse_source_constructs(text):
            if parsed.kind != "macro":
                continue
            if parsed.start <= start < parsed.end and not masked[parsed.start:start].strip():
                return SourceTraceConstruct(
                    "macro",
                    parsed.name,
                    start,
                    parsed.end,
                    _line_number(text, start),
                ), "attached"
    if macro_match is not None and (
        macro_match.group(1).startswith(("BEGIN_", "END_", "IMPLEMENT_", "DECLARE_"))
        or macro_match.group(1).isupper()
    ):
        return None, "unsupported-macro-anchor"

    paren_depth = 0
    bracket_depth = 0
    first_brace: int | None = None
    semicolon: int | None = None
    index = start
    while index < len(masked):
        char = masked[index]
        if char == "(":
            paren_depth += 1
        elif char == ")" and paren_depth:
            paren_depth -= 1
        elif char == "[":
            bracket_depth += 1
        elif char == "]" and bracket_depth:
            bracket_depth -= 1
        elif char == "{" and paren_depth == 0 and bracket_depth == 0:
            first_brace = index
            break
        elif char == ";" and paren_depth == 0 and bracket_depth == 0:
            semicolon = index
            break
        index += 1

    prefix_end = first_brace if first_brace is not None else semicolon
    if prefix_end is None:
        return None, "detached"
    prefix = masked[start:prefix_end].strip()
    if not prefix:
        return None, "detached"

    type_match = re.match(
        r"(?:template\s*<[^>]*>\s*)?(?:class|struct|union|enum)\s+([A-Za-z_]\w*)",
        prefix,
        re.DOTALL,
    )
    if first_brace is not None and type_match is not None:
        return SourceTraceConstruct(
            "type",
            type_match.group(1),
            start,
            _matching_brace(masked, first_brace),
            _line_number(text, start),
        ), "attached"

    if first_brace is not None and ")" in prefix:
        opens = [
            match
            for match in re.finditer(r"([~A-Za-z_][\w:~<>]*)\s*\(", prefix)
            if match.group(1) not in {"__attribute__", "__declspec", "alignas", "sizeof"}
        ]
        if not opens:
            return None, "unsupported-macro-or-declaration"
        name = opens[0].group(1)
        return SourceTraceConstruct(
            "function",
            name,
            start,
            _matching_brace(masked, first_brace),
            _line_number(text, start),
        ), "attached"

    if semicolon is not None or first_brace is not None:
        if re.search(r"\bextern\b", prefix) and "=" not in prefix:
            return None, "unsupported-extern-declaration"
        if re.match(r"(?:template\s+)?(?:class|struct)\b", prefix):
            return None, "unsupported-explicit-instantiation"
        if "(" in prefix and first_brace is None:
            return None, "unsupported-macro-or-declaration"
        if re.search(r"\b(?:typedef|using)\b", prefix):
            return None, "unsupported-declaration"
        identifiers = re.findall(r"[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*", prefix)
        if not identifiers:
            return None, "detached"
        end = semicolon + 1 if semicolon is not None else _matching_brace(masked, first_brace)
        if first_brace is not None:
            trailing = end
            while trailing < len(masked) and masked[trailing].isspace():
                trailing += 1
            if trailing < len(masked) and masked[trailing] == ";":
                end = trailing + 1
        return SourceTraceConstruct(
            "data",
            identifiers[-1],
            start,
            end,
            _line_number(text, start),
        ), "attached"
    return None, "detached"


def _construct_after(text: str, masked: str, comment: _Comment) -> SourceTraceConstruct | None:
    return _attachment_after(text, masked, comment)[0]


def _legacy_artifacts(
    comments: tuple[_Comment, ...],
    *,
    path: str,
    text: str,
    masked: str,
    binary: str,
) -> tuple[SourceTraceArtifact, ...]:
    rows: list[SourceTraceArtifact] = []
    for comment in comments:
        construct, attachment_status = _attachment_after(text, masked, comment)
        for line, value in _comment_lines(comment):
            for match in LEGACY_REIMPLEMENTS_RE.finditer(value):
                kind = "data" if match.group(1) else "function"
                address = normalize_address(match.group(2))
                rows.append(
                    SourceTraceArtifact(
                        relation="defines",
                        section="",
                        artifact_id=f"{binary}:{kind}:{address}",
                        description=match.group(3).strip(),
                        path=path,
                        line=line,
                        anchor_id=None,
                        direct=construct is not None,
                        construct=construct,
                        legacy=True,
                        comment_style=comment.style,
                        attachment_status=attachment_status,
                    )
                )
            for match in LEGACY_EMITS_RE.finditer(value):
                address = normalize_address(match.group(1))
                rows.append(
                    SourceTraceArtifact(
                        relation="emits",
                        section="",
                        artifact_id=f"{binary}:function:{address}",
                        description=match.group(2).strip(),
                        path=path,
                        line=line,
                        anchor_id=None,
                        direct=construct is not None,
                        construct=construct,
                        legacy=True,
                        comment_style=comment.style,
                        attachment_status=attachment_status,
                    )
                )
    return tuple(rows)


def _unsupported_legacy_addresses(
    comments: tuple[_Comment, ...],
    *,
    path: str,
) -> tuple[SourceTraceLegacyAddress, ...]:
    if path != "<memory>" and Path(path).suffix.lower() not in SOURCE_TRACE_SUFFIXES:
        return ()
    rows: list[SourceTraceLegacyAddress] = []
    for comment in comments:
        for line, value in _comment_lines(comment):
            if value.startswith("@recoil-artifact"):
                continue
            structured = tuple(
                match
                for pattern in (LEGACY_REIMPLEMENTS_RE, LEGACY_EMITS_RE)
                for match in pattern.finditer(value)
            )
            for match in LEGACY_ADDRESS_CATCHALL_RE.finditer(value):
                if any(
                    parsed.start() == match.start()
                    and normalize_address(
                        parsed.group(2)
                        if parsed.re is LEGACY_REIMPLEMENTS_RE
                        else parsed.group(1)
                    )
                    == normalize_address(match.group("address"))
                    for parsed in structured
                ):
                    continue
                rows.append(
                    SourceTraceLegacyAddress(
                        marker=match.group("marker"),
                        address=normalize_address(match.group("address")),
                        text=value,
                        path=path,
                        line=line,
                        comment_style=comment.style,
                    )
                )
    return tuple(rows)


def _newline_style(text: str, *, path: str) -> str:
    without_crlf = text.replace("\r\n", "")
    styles: list[str] = []
    if "\r\n" in text:
        styles.append("crlf")
    if "\n" in without_crlf:
        styles.append("lf")
    if "\r" in without_crlf:
        styles.append("cr")
    if len(styles) > 1:
        raise ValueError(f"{path}: mixed newline styles are not valid source-trace input: {styles}")
    return styles[0] if styles else "none"


def parse_source_trace_text(
    text: str,
    *,
    path: str = "<memory>",
    encoding: str = "memory",
    legacy_binary: str | None = None,
) -> SourceTraceDocument:
    newline = _newline_style(text, path=path)
    normalized_path = path.replace("\\", "/").lower()
    binary = legacy_binary or (
        "messages"
        if normalized_path.startswith("src/messages/")
        or "/src/messages/" in normalized_path
        else "recoil"
    )
    if binary not in {"recoil", "messages"}:
        raise ValueError(f"{path}: unsupported legacy trace binary {binary!r}")
    comments = _scan_comments(text)
    masked = _mask_comments_and_literals(text)
    anchors: list[SourceTraceAnchor] = []
    artifacts: list[SourceTraceArtifact] = []
    findings: list[SourceTraceFinding] = []

    for comment in comments:
        anchor_rows: list[tuple[int, str]] = []
        artifact_rows: list[tuple[int, re.Match[str]]] = []
        for line, value in _comment_lines(comment):
            anchor_match = ANCHOR_RE.fullmatch(value)
            if anchor_match is not None:
                anchor_rows.append((line, anchor_match.group(1)))
                continue
            artifact_match = ARTIFACT_RE.fullmatch(value)
            if artifact_match is not None:
                artifact_rows.append((line, artifact_match))
                continue
            if value.startswith("@recoil-anchor"):
                findings.append(
                    SourceTraceFinding(
                        "malformed-anchor-directive",
                        path,
                        line,
                        "expected '@recoil-anchor <id>'",
                    )
                )
            elif value.startswith("@recoil-artifact"):
                findings.append(
                    SourceTraceFinding(
                        "malformed-artifact-directive",
                        path,
                        line,
                        "expected '@recoil-artifact <defines|emits> <output-section> "
                        "<artifact-id>: <description>'",
                    )
                )

        if not anchor_rows and not artifact_rows:
            continue
        construct, attachment_status = _attachment_after(text, masked, comment)
        if len(anchor_rows) != 1:
            code = "missing-anchor-directive" if not anchor_rows else "duplicate-anchor-directive"
            findings.append(
                SourceTraceFinding(
                    code,
                    path,
                    (artifact_rows[0][0] if artifact_rows else anchor_rows[0][0]),
                    "a canonical trace comment must contain exactly one @recoil-anchor directive",
                )
            )
        anchor_id = anchor_rows[0][1] if len(anchor_rows) == 1 else None
        if construct is None:
            finding_code = {
                "eof": "eof-anchor",
                "stacked-comment": "stacked-anchor",
                "preprocessor": "preprocessor-anchor",
            }.get(attachment_status, attachment_status)
            findings.append(
                SourceTraceFinding(
                    finding_code,
                    path,
                    anchor_rows[0][0] if anchor_rows else artifact_rows[0][0],
                    "canonical source-trace directives must be immediately attached to a "
                    f"supported function, data, or type definition; status={attachment_status}",
                    anchor_id=anchor_id,
                )
            )
        if comment.style != "doxygen":
            findings.append(
                SourceTraceFinding(
                    "invalid-comment-style",
                    path,
                    anchor_rows[0][0] if anchor_rows else artifact_rows[0][0],
                    "canonical source-trace directives must use an attached Doxygen /** ... */ block",
                    anchor_id=anchor_id,
                )
            )
        if anchor_id is not None:
            if ANCHOR_ID_RE.fullmatch(anchor_id) is None:
                findings.append(
                    SourceTraceFinding(
                        "invalid-anchor-id",
                        path,
                        anchor_rows[0][0],
                        "anchor id must match 'recoil:anchor:<stable-id>' using lowercase "
                        "letters, digits, '.', '_' or '-'",
                        anchor_id=anchor_id,
                    )
                )
            anchors.append(
                SourceTraceAnchor(
                    anchor_id,
                    path,
                    anchor_rows[0][0],
                    comment.start,
                    comment.end,
                    construct,
                    comment.style,
                    attachment_status,
                )
            )
        for line, match in artifact_rows:
            raw_relation, section, raw_artifact_id, description = match.groups()
            relation = raw_relation
            artifact_id = normalize_artifact_id(raw_artifact_id)
            direct = construct is not None and anchor_id is not None
            artifact = SourceTraceArtifact(
                relation,
                section,
                artifact_id,
                description.strip(),
                path,
                line,
                anchor_id,
                direct,
                construct,
                False,
                comment.style,
                attachment_status,
            )
            artifacts.append(artifact)
            if relation not in RELATIONS:
                findings.append(
                    SourceTraceFinding(
                        "invalid-relation",
                        path,
                        line,
                        f"relation must be one of {sorted(RELATIONS)}, not {relation!r}",
                        anchor_id,
                        artifact_id,
                    )
                )
            if not section.startswith(".") or not re.fullmatch(r"\.[A-Za-z0-9_$.-]+", section):
                findings.append(
                    SourceTraceFinding(
                        "invalid-output-section",
                        path,
                        line,
                        f"output section must be an exact PE section token, not {section!r}",
                        anchor_id,
                        artifact_id,
                    )
                )
            if artifact.entity_kind is None:
                findings.append(
                    SourceTraceFinding(
                        "invalid-artifact-id",
                        path,
                        line,
                        "artifact id must be a physical recoil:function/recoil:data id or a "
                        "recoil:logical-function/recoil:logical-data id",
                        anchor_id,
                        artifact_id,
                    )
                )
            if relation == "defines" and construct is not None:
                expected_construct_kind = artifact.entity_kind
                if expected_construct_kind is not None and construct.kind != expected_construct_kind:
                    findings.append(
                        SourceTraceFinding(
                            "wrong-relation",
                            path,
                            line,
                            f"'defines' {expected_construct_kind} artifact is attached to "
                            f"{construct.kind} definition {construct.name!r}",
                            anchor_id,
                            artifact_id,
                        )
                    )

    seen_anchors: dict[str, SourceTraceAnchor] = {}
    for anchor in anchors:
        prior = seen_anchors.get(anchor.anchor_id)
        if prior is not None:
            findings.append(
                SourceTraceFinding(
                    "duplicate-anchor-id",
                    anchor.path,
                    anchor.line,
                    f"anchor id {anchor.anchor_id!r} also occurs at {prior.path}:{prior.line}",
                    anchor.anchor_id,
                )
            )
        else:
            seen_anchors[anchor.anchor_id] = anchor

    seen_artifacts: dict[str, SourceTraceArtifact] = {}
    for artifact in artifacts:
        prior = seen_artifacts.get(artifact.artifact_id)
        if prior is not None:
            findings.append(
                SourceTraceFinding(
                    "duplicate-artifact-id",
                    artifact.path,
                    artifact.line,
                    f"artifact id {artifact.artifact_id!r} also occurs at "
                    f"{prior.path}:{prior.line}",
                    artifact.anchor_id,
                    artifact.artifact_id,
                )
            )
        else:
            seen_artifacts[artifact.artifact_id] = artifact

    return SourceTraceDocument(
        path=path,
        anchors=tuple(anchors),
        artifacts=tuple(artifacts),
        legacy_artifacts=_legacy_artifacts(
            comments,
            path=path,
            text=text,
            masked=masked,
            binary=binary,
        ),
        findings=tuple(findings),
        encoding=encoding,
        newline=newline,
        unsupported_legacy_addresses=_unsupported_legacy_addresses(
            comments,
            path=path,
        ),
    )


def parse_source_trace_path(
    path: Path,
    *,
    repo_root: Path | None = None,
    legacy_binary: str | None = None,
) -> SourceTraceDocument:
    resolved = path.resolve()
    display = resolved.as_posix()
    if repo_root is not None:
        try:
            display = resolved.relative_to(repo_root.resolve()).as_posix()
        except ValueError:
            pass
    raw = resolved.read_bytes()
    if raw.startswith(b"\xef\xbb\xbf"):
        encoding = "utf-8-sig"
        try:
            text = raw.decode(encoding, errors="strict")
        except UnicodeDecodeError as exc:
            raise ValueError(f"{display}: invalid UTF-8 BOM source text: {exc}") from exc
    else:
        try:
            text = raw.decode("utf-8", errors="strict")
            encoding = "utf-8"
        except UnicodeDecodeError:
            try:
                text = raw.decode("cp1252", errors="strict")
                encoding = "cp1252"
            except UnicodeDecodeError as exc:
                raise ValueError(
                    f"{display}: source text is neither strict UTF-8 nor strict cp1252: {exc}"
                ) from exc
    return parse_source_trace_text(
        text,
        path=display,
        encoding=encoding,
        legacy_binary=legacy_binary,
    )


def load_artifact_rows(progress_path: Path) -> SourceArtifactIndex:
    data = ProgressStore(progress_path).load().data
    if not isinstance(data, Mapping):
        raise ValueError(f"{progress_path}: progress root must be an object")
    raw_symbols = data.get("symbols")
    if not isinstance(raw_symbols, Mapping):
        raise ValueError(f"{progress_path}: symbols must be an object")

    rows: dict[str, SourceArtifactRow] = {}
    for raw_id, raw_row in raw_symbols.items():
        if not isinstance(raw_id, str) or not isinstance(raw_row, Mapping):
            continue
        artifact_id = normalize_artifact_id(raw_id)
        kind = artifact_entity_kind(artifact_id)
        if kind not in ENTITY_KINDS:
            continue
        output_section_id = raw_row.get("output_section_id")
        rows[artifact_id] = SourceArtifactRow(
            artifact_id,
            artifact_id,
            kind,
            output_section_id if isinstance(output_section_id, str) else None,
            raw_row,
        )
        aliases = raw_row.get("logical_aliases")
        if not isinstance(aliases, Mapping):
            continue
        for raw_alias_id, alias_row in aliases.items():
            if not isinstance(raw_alias_id, str) or not isinstance(alias_row, Mapping):
                continue
            alias_id = normalize_artifact_id(raw_alias_id)
            expected_pattern = LOGICAL_FUNCTION_RE if kind == "function" else LOGICAL_DATA_RE
            if expected_pattern.fullmatch(alias_id) is None:
                continue
            rows[alias_id] = SourceArtifactRow(
                alias_id,
                artifact_id,
                kind,
                output_section_id if isinstance(output_section_id, str) else None,
                alias_row,
                logical=True,
            )

    output_sections: set[str] = set()
    raw_sections = data.get("output_sections")
    if isinstance(raw_sections, Mapping):
        for section_id, row in raw_sections.items():
            if isinstance(section_id, str) and ":section:" in section_id:
                output_sections.add(section_id.split(":section:", 1)[1])
            if isinstance(row, Mapping):
                name = row.get("name")
                if isinstance(name, str) and name.startswith("."):
                    output_sections.add(name)

    return SourceArtifactIndex(rows, frozenset(output_sections), str(progress_path))


def validate_source_trace(
    document: SourceTraceDocument,
    index: SourceArtifactIndex,
    *,
    strict: bool = True,
) -> tuple[SourceTraceFinding, ...]:
    findings = list(document.findings)
    for artifact in document.artifacts:
        row = index.resolve(artifact.artifact_id)
        if row is None:
            code = (
                "unreviewed-logical-data"
                if LOGICAL_DATA_RE.fullmatch(artifact.artifact_id) is not None
                else "unknown-artifact-id"
            )
            findings.append(
                SourceTraceFinding(
                    code,
                    artifact.path,
                    artifact.line,
                    (
                        f"{artifact.artifact_id!r} does not resolve through an explicitly "
                        "reviewed logical alias nested on its physical tracker data row"
                        if code == "unreviewed-logical-data"
                        else f"{artifact.artifact_id!r} does not resolve through tracker "
                        "function/data rows"
                    ),
                    artifact.anchor_id,
                    artifact.artifact_id,
                )
            )
            continue
        if row.kind != artifact.entity_kind:
            findings.append(
                SourceTraceFinding(
                    "wrong-entity-kind",
                    artifact.path,
                    artifact.line,
                    f"{artifact.artifact_id!r} resolves as {row.kind}, not "
                    f"{artifact.entity_kind or 'a supported entity'}",
                    artifact.anchor_id,
                    artifact.artifact_id,
                )
            )
        if (
            row.kind == "function"
            and not row.logical
            and isinstance(row.row.get("pipeline_class"), str)
            and row.row.get("pipeline_class") not in AUTHORED_FUNCTION_CLASSES
        ):
            findings.append(
                SourceTraceFinding(
                    "non-authored-function-source-edge",
                    artifact.path,
                    artifact.line,
                    f"{artifact.artifact_id!r} has pipeline_class "
                    f"{row.row.get('pipeline_class')!r}; a canonical source edge requires "
                    "reviewed authored or authored-lifecycle identity",
                    artifact.anchor_id,
                    artifact.artifact_id,
                )
            )
        expected_section = row.output_section
        if expected_section is None:
            findings.append(
                SourceTraceFinding(
                    "wrong-or-unknown-section",
                    artifact.path,
                    artifact.line,
                    f"{artifact.artifact_id!r} has no verifiable tracker output_section_id",
                    artifact.anchor_id,
                    artifact.artifact_id,
                )
            )
        elif artifact.section != expected_section:
            findings.append(
                SourceTraceFinding(
                    "wrong-section",
                    artifact.path,
                    artifact.line,
                    f"{artifact.artifact_id!r} declares {artifact.section!r}; tracker row "
                    f"{row.physical_id!r} requires {expected_section!r}",
                    artifact.anchor_id,
                    artifact.artifact_id,
                )
            )

    if strict:
        for legacy in document.legacy_artifacts:
            findings.append(
                SourceTraceFinding(
                    "legacy-marker-nonqualifying",
                    legacy.path,
                    legacy.line,
                    "legacy Reimplements/Emits marker is inventory only and does not qualify "
                    "for canonical source-trace policy",
                    artifact_id=legacy.artifact_id,
                )
            )
        for legacy in document.unsupported_legacy_addresses:
            findings.append(
                SourceTraceFinding(
                    "unsupported-legacy-address",
                    legacy.path,
                    legacy.line,
                    "unsupported address-bearing Reimplements/Emits comment syntax is "
                    "migration cleanup inventory only and does not qualify for canonical "
                    "source-trace policy",
                )
            )
    return tuple(findings)


def merge_source_trace_documents(
    documents: tuple[SourceTraceDocument, ...],
) -> tuple[SourceTraceFinding, ...]:
    """Return cross-file uniqueness findings without changing per-file topology."""

    findings: list[SourceTraceFinding] = []
    seen_anchors: dict[str, SourceTraceAnchor] = {}
    seen_artifacts: dict[str, SourceTraceArtifact] = {}
    for document in documents:
        for anchor in document.anchors:
            prior = seen_anchors.get(anchor.anchor_id)
            if prior is not None:
                findings.append(
                    SourceTraceFinding(
                        "duplicate-anchor-id",
                        anchor.path,
                        anchor.line,
                        f"anchor id {anchor.anchor_id!r} also occurs at "
                        f"{prior.path}:{prior.line}",
                        anchor.anchor_id,
                    )
                )
            else:
                seen_anchors[anchor.anchor_id] = anchor
        for artifact in document.artifacts:
            prior = seen_artifacts.get(artifact.artifact_id)
            if prior is not None:
                findings.append(
                    SourceTraceFinding(
                        "duplicate-artifact-id",
                        artifact.path,
                        artifact.line,
                        f"artifact id {artifact.artifact_id!r} also occurs at "
                        f"{prior.path}:{prior.line}",
                        artifact.anchor_id,
                        artifact.artifact_id,
                    )
                )
            else:
                seen_artifacts[artifact.artifact_id] = artifact
    return tuple(findings)
