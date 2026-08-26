"""Shared, offset-stable C/C++ source construct inventory.

This is intentionally a lexical VC5-era parser, not a C++ compiler.  It owns
the common definition inventory used by source-comment and original-symbol
guards so those gates cannot disagree merely because one regex understands an
MFC macro, ``__declspec`` prefix, or data definition that the other does not.
"""

from __future__ import annotations

from dataclasses import dataclass
import re


CONTROL_NAMES = {"if", "for", "while", "switch", "catch", "sizeof"}
MFC_REGION_MACROS = {
    "BEGIN_DISPATCH_MAP",
    "BEGIN_EVENT_MAP",
    "BEGIN_INTERFACE_MAP",
    "BEGIN_MESSAGE_MAP",
    "IMPLEMENT_DYNAMIC",
    "IMPLEMENT_DYNCREATE",
    "IMPLEMENT_OLECREATE",
    "IMPLEMENT_OLECREATE_EX",
    "IMPLEMENT_SERIAL",
}
IDENTIFIER = r"[A-Za-z_][A-Za-z0-9_]*"
QUALIFIED_NAME = rf"(?:~?{IDENTIFIER}::)*~?{IDENTIFIER}"
FUNCTION_RE = re.compile(
    rf"(?m)^[ \t]*"
    rf"(?=[A-Za-z_~])"
    rf"(?P<signature>"
    rf"(?!if\b|for\b|while\b|switch\b|catch\b|else\b|do\b|return\b|sizeof\b)"
    rf"(?:(?![;{{}}]).)*?"
    rf"(?P<name>{QUALIFIED_NAME})[ \t]*"
    rf"\((?:(?![;{{}}]).)*?\)"
    rf"(?:[ \t\r\n]+const)?"
    rf"(?:[ \t\r\n]+(?:throw|__declspec)[ \t]*\([^;{{}}]*\))*"
    rf"[ \t\r\n]*(?::(?:(?![;{{}}]).)*)?"
    rf"\{{)",
    re.DOTALL,
)
MFC_MACRO_RE = re.compile(
    rf"(?m)^[ \t]*(?P<macro>{'|'.join(sorted(MFC_REGION_MACROS))})[ \t]*"
    rf"\([ \t]*(?P<owner>{IDENTIFIER}(?::{IDENTIFIER})*)[^;\n]*\)[ \t]*;?"
)
DATA_STATEMENT_RE = re.compile(
    r"(?m)^[ \t]*(?=[A-Za-z_])(?P<statement>(?!#)(?!typedef\b)(?!using\b)"
    r"[^;{}()]+(?:\[[^\]]*\])?[ \t]*(?:=[^;]*)?;)"
)
ADDRESS_PROVENANCE_RE = re.compile(
    r"\b(?:Reimplements(?:\s+data)?\s+0x[0-9A-Fa-f]+|"
    r"[A-Za-z_~][A-Za-z0-9_:~]*\s+(?:--|—)\s*0x[0-9A-Fa-f]+)",
    re.IGNORECASE,
)
PROVIDER_PROVENANCE_RE = re.compile(
    r"\b(?:provider[- ]boundary|"
    r"imported\b.{0,80}\b(?:provider|boundary|abi|runtime)|"
    r"compiler\b.{0,80}\b(?:provider|boundary|abi|generated))",
    re.IGNORECASE | re.DOTALL,
)
ORIGINAL_HELPER_ROLE_RE = re.compile(
    r"\boriginal(?:-source)?\b.{0,100}\b"
    r"(?:inline|static|helper|constructor|destructor|member)\b",
    re.IGNORECASE | re.DOTALL,
)
ORIGINAL_FUNCTION_RETAIL_RE = re.compile(
    r"\boriginal(?:-source)?\s+function(?:\s+evidence)?\b.{0,160}\b"
    r"(?:retail(?:\s+address)?[ \t:=-]*)?0x[0-9A-Fa-f]+\b",
    re.IGNORECASE | re.DOTALL,
)
NO_STANDALONE_RETAIL_RE = re.compile(
    r"\bno\s+standalone\s+(?:(?:authored\s+)?retail\s+"
    r"(?:function|body|symbol)|plan(?:/source-map)?\s+entry)\b",
    re.IGNORECASE,
)
OBSERVED_CALLER_RE = re.compile(
    r"\bobserved(?:\s+in)?\s+callers?\b.{0,160}\b0x[0-9A-Fa-f]+\b",
    re.IGNORECASE | re.DOTALL,
)
NAMED_CALLER_CLUSTER_RE = re.compile(
    r"(?i:\bobserved\s+in\s+)"
    r"(?=[^.\r\n]{1,120}(?i:\bcallers?\b))"
    r"(?=[^.\r\n]*(?:"
    r"[A-Za-z_][A-Za-z0-9_]*::|"
    r"[A-Za-z_][A-Za-z0-9_]*[-_][A-Za-z0-9_-]*|"
    r"[A-Za-z_]*[a-z][A-Za-z0-9_]*[A-Z0-9][A-Za-z0-9_]*"
    r"))"
    r"[^.\r\n]{1,120}(?i:\bcallers?\b)",
)
SOURCE_PATH_EVIDENCE_RE = re.compile(
    r"\b(?:original\s+source|source-placement|source)\s+"
    r"(?:path|hypothesis)\b.{0,240}\.(?:c|cc|cpp|cxx|h|hpp)\b",
    re.IGNORECASE | re.DOTALL,
)
ORIGINAL_SOURCE_PATH_RE = re.compile(
    r"\boriginal(?:(?:\s+physical)?\s+source"
    r"(?:\s+(?:path|contribution))?[ \t:=-]+|"
    r"\s+file\s+evidence\b.{0,200}?)"
    r"(?:[A-Za-z]:[\\/]|(?:[A-Za-z0-9_.-]+[\\/])+)"
    r".{1,240}\.(?:c|cc|cpp|cxx|h|hpp)\b",
    re.IGNORECASE | re.DOTALL,
)
SOURCE_PATH_LITERAL_RE = re.compile(
    r"(?:\b[A-Za-z]:[\\/]|(?:\b[A-Za-z0-9_.-]+[\\/])+)"
    r"(?:[^*?\"<>|\r\n]+[\\/])*"
    r"[^*?\"<>|\r\n]+\.(?:c|cc|cpp|cxx|h|hpp)\b",
    re.IGNORECASE,
)
PRIMARY_ADDRESS_EVIDENCE_RE = re.compile(
    r"\b(?:retail|BN)\b.{0,160}\b0x[0-9A-Fa-f]+\b",
    re.IGNORECASE | re.DOTALL,
)
ADDRESS_LITERAL_RE = re.compile(r"\b0x[0-9A-Fa-f]+\b")
RETAIL_SOURCE_SHAPE_RE = re.compile(
    r"\bretail\b.{0,180}\b"
    r"(?:inlines?|members?|constructors?|destructors?|vtables?|slots?|STL|owner\s+verification)\b",
    re.IGNORECASE | re.DOTALL,
)
STRUCTURAL_RECOVERY_EVIDENCE_RE = re.compile(
    r"(?:\baddress-backed\s+callers?\b|"
    r"\brecovered\s+from\b.{0,180}\b(?:callers?|source-file\s+cluster)\b|"
    r"\b(?:derived|base|complete)\b.{0,180}\b"
    r"(?:constructors?|destructors?|vtables?|members?)\b|"
    r"\bdefault\s+virtual\b.{0,100}\bhook\b)",
    re.IGNORECASE | re.DOTALL,
)
SOURCE_EVIDENCE_MARKER_RE = re.compile(
    r"(?:@recoil-(?:anchor|artifact|raw-asm|raw-consumer)\b|"
    r"\bReimplements\b|"
    r"^[ \t]*(?:(?:/\*\*?|//|\*)[ \t]*)?"
    r"original(?:-source)?\b.{0,100}\b"
    r"(?:inline|static|helper|function|constructor|destructor|member|"
    r"source|file|evidence|lifetime)\b|"
    r"\bprovider[- ]boundary\b|"
    r"\bimported\b.{0,80}\b(?:provider|boundary|abi|runtime)\b|"
    r"\bcompiler\b.{0,80}\b(?:provider|boundary|abi|generated)\b|"
    r"^[ \t]*(?:(?:/\*\*?|//|\*)[ \t]*)?"
    r"recovered\b.{0,100}\b(?:inline|static|helper|function|member|source)\b)",
    re.IGNORECASE | re.MULTILINE,
)


@dataclass(frozen=True)
class SourceConstruct:
    kind: str
    name: str
    start: int
    end: int
    line: int
    body_start: int | None = None
    macro: str | None = None


def line_number(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def mask_comments_and_literals(text: str) -> str:
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


def matching_brace(masked: str, open_brace: int) -> int:
    depth = 0
    for index in range(open_brace, len(masked)):
        if masked[index] == "{":
            depth += 1
        elif masked[index] == "}":
            depth -= 1
            if depth == 0:
                return index + 1
    return len(masked)


def _nested(offset: int, ranges: list[tuple[int, int]]) -> bool:
    return any(start < offset < end for start, end in ranges)


def parse_source_constructs(text: str) -> tuple[SourceConstruct, ...]:
    masked = mask_comments_and_literals(text)
    functions: list[SourceConstruct] = []
    for match in FUNCTION_RE.finditer(masked):
        name = match.group("name")
        if name.split("::")[-1] in CONTROL_NAMES:
            continue
        if name.split("::")[-1] in MFC_REGION_MACROS:
            continue
        if name in {"new", "delete"} and re.search(
            rf"\boperator\s+{name}\s*\(",
            masked[match.start("signature") : match.end("signature")],
        ):
            continue
        open_brace = masked.rfind("{", match.start("signature"), match.end("signature"))
        if open_brace < 0:
            continue
        functions.append(
            SourceConstruct(
                "function",
                name,
                match.start("signature"),
                matching_brace(masked, open_brace),
                line_number(masked, match.start("signature")),
                open_brace,
            )
        )
    functions.sort(key=lambda item: (item.start, item.end))
    functions = [
        candidate
        for candidate in functions
        if not any(
            other.start < candidate.start < other.end
            for other in functions
            if other is not candidate
        )
    ]
    function_ranges = [(item.start, item.end) for item in functions]

    macros: list[SourceConstruct] = []
    for match in MFC_MACRO_RE.finditer(masked):
        if _nested(match.start(), function_ranges):
            continue
        macros.append(
            SourceConstruct(
                "macro",
                match.group("owner"),
                match.start(),
                match.end(),
                line_number(masked, match.start()),
                macro=match.group("macro"),
            )
        )

    data: list[SourceConstruct] = []
    for match in DATA_STATEMENT_RE.finditer(masked):
        if _nested(match.start("statement"), function_ranges):
            continue
        statement = match.group("statement")
        if re.match(r"\s*extern\b", statement) and "=" not in statement:
            continue
        prefix = statement.split("=", 1)[0]
        identifiers = re.findall(IDENTIFIER, prefix)
        if len(identifiers) < 2:
            continue
        name = identifiers[-1]
        if name in {"const", "static", "volatile"}:
            continue
        data.append(
            SourceConstruct(
                "data",
                name,
                match.start("statement"),
                match.end("statement"),
                line_number(masked, match.start("statement")),
            )
        )

    return tuple(sorted((*functions, *macros, *data), key=lambda item: (item.start, item.end)))


def function_constructs(text: str) -> tuple[SourceConstruct, ...]:
    return tuple(item for item in parse_source_constructs(text) if item.kind == "function")


def data_constructs(text: str) -> tuple[SourceConstruct, ...]:
    return tuple(item for item in parse_source_constructs(text) if item.kind == "data")


def adjacent_comment(text: str, offset: int) -> str | None:
    """Return the complete immediately adjacent comment group, if any."""

    prefix = text[:offset]
    end = len(prefix)
    while end > 0 and prefix[end - 1] in " \t\r\n":
        end -= 1
    if re.search(r"\r?\n[ \t]*\r?\n", prefix[end:]):
        return None
    if end <= 0:
        return None
    if prefix[:end].endswith("*/"):
        start = prefix.rfind("/*", 0, end)
        return prefix[start:end] if start >= 0 else None

    line_end = end
    line_start = prefix.rfind("\n", 0, line_end) + 1
    if not prefix[line_start:line_end].lstrip().startswith("//"):
        return None
    start = line_start
    cursor = line_start - 1
    while cursor >= 0:
        previous_end = cursor
        previous_start = prefix.rfind("\n", 0, previous_end) + 1
        previous = prefix[previous_start:previous_end]
        if not previous.lstrip().startswith("//"):
            break
        start = previous_start
        cursor = previous_start - 1
    return prefix[start:end]


def has_explicit_original_helper_evidence(comment: str) -> bool:
    """Recognize source-model evidence without depending on one phrase."""

    semantic = comment_semantic_text(comment)
    return (
        ORIGINAL_HELPER_ROLE_RE.search(semantic) is not None
        and (
            NO_STANDALONE_RETAIL_RE.search(semantic) is not None
            or OBSERVED_CALLER_RE.search(semantic) is not None
            or NAMED_CALLER_CLUSTER_RE.search(semantic) is not None
            or SOURCE_PATH_EVIDENCE_RE.search(semantic) is not None
            or SOURCE_PATH_LITERAL_RE.search(semantic) is not None
            or PRIMARY_ADDRESS_EVIDENCE_RE.search(semantic) is not None
            or STRUCTURAL_RECOVERY_EVIDENCE_RE.search(semantic) is not None
            or ADDRESS_LITERAL_RE.search(semantic) is not None
            or RETAIL_SOURCE_SHAPE_RE.search(semantic) is not None
        )
    )


def has_source_provenance_evidence(comment: str) -> bool:
    semantic = comment_semantic_text(comment)
    return (
        ADDRESS_PROVENANCE_RE.search(semantic) is not None
        or PROVIDER_PROVENANCE_RE.search(semantic) is not None
        or ORIGINAL_FUNCTION_RETAIL_RE.search(semantic) is not None
        or ORIGINAL_SOURCE_PATH_RE.search(semantic) is not None
        or has_explicit_original_helper_evidence(semantic)
    )


def has_source_evidence_marker(comment: str) -> bool:
    """Whether an adjacent comment asserts a governed source/evidence claim."""

    return SOURCE_EVIDENCE_MARKER_RE.search(comment) is not None


def comment_semantic_text(comment: str) -> str:
    """Remove comment decoration before evaluating evidence semantics."""

    lines: list[str] = []
    for raw_line in comment.splitlines():
        line = raw_line.strip()
        if line.startswith("/**"):
            line = line[3:].strip()
        elif line.startswith("/*"):
            line = line[2:].strip()
        elif line.startswith("//"):
            line = line[2:].strip()
        elif line.startswith("*"):
            line = line[1:].strip()
        if line.endswith("*/"):
            line = line[:-2].strip()
        if line:
            lines.append(line)
    return " ".join(lines)
