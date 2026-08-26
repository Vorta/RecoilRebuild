from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path, PurePosixPath
import re

from _recoil.lib.source_constructs import parse_source_constructs

ANCHOR_KINDS = {"type-definition", "function-definition", "data-definition"}
QUOTED_INCLUDE_RE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.MULTILINE)
DOXYGEN_BLOCK_RE = re.compile(r"/\*\*.*?\*/", re.DOTALL)
EMITS_LINE_RE = re.compile(r"^Emits\s+(0x[0-9A-Fa-f]+):\s*(\S.*)$")


@dataclass(frozen=True)
class EmissionAnchor:
    path: str
    kind: str
    name: str


@dataclass(frozen=True)
class SourceClosureFile:
    path: Path
    repo_path: str
    text: str


@dataclass(frozen=True)
class SourceEmissionMarker:
    address: str
    description: str
    path: str
    line: int
    anchor: EmissionAnchor


@dataclass(frozen=True)
class _Docblock:
    start: int
    end: int
    text: str


@dataclass(frozen=True)
class _Construct:
    start: int
    end: int


def normalize_emission_address(value: str) -> str:
    try:
        return f"0x{int(value, 16):x}"
    except (TypeError, ValueError) as exc:
        raise ValueError(f"invalid emission address {value!r}") from exc


def normalize_anchor_path(value: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise ValueError("emission_anchor.path must be a non-empty repository-relative path")
    raw = value.strip().replace("\\", "/")
    if Path(raw).is_absolute() or re.match(r"^[A-Za-z]:/", raw):
        raise ValueError("emission_anchor.path must be repository-relative")
    path = PurePosixPath(raw)
    if any(part in {"", ".", ".."} for part in path.parts):
        raise ValueError("emission_anchor.path must be a normalized repository-relative path")
    normalized = path.as_posix()
    if normalized != raw:
        raise ValueError(
            f"emission_anchor.path must be normalized as {normalized!r}, not {value!r}"
        )
    return normalized


def _display_path(path: Path, repo_root: Path) -> str:
    try:
        return path.resolve().relative_to(repo_root.resolve()).as_posix()
    except ValueError:
        return path.resolve().as_posix()


def _resolve_include(include_text: str, including_source: Path, repo_root: Path) -> Path | None:
    include_path = Path(include_text)
    candidates = (
        including_source.parent / include_path,
        repo_root / include_path,
        repo_root / "src" / include_path,
    )
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    return None


def collect_source_closure(
    source_from: str | Path,
    *,
    repo_root: Path,
) -> tuple[SourceClosureFile, ...]:
    source_path = Path(source_from)
    if not source_path.is_absolute():
        source_path = repo_root / source_path
    if not source_path.is_file():
        raise ValueError(f"source_from does not exist: {source_path}")

    visited: set[Path] = set()
    files: list[SourceClosureFile] = []

    def visit(path: Path) -> None:
        resolved = path.resolve()
        if resolved in visited:
            return
        visited.add(resolved)
        text = resolved.read_text(encoding="utf-8", errors="ignore")
        files.append(
            SourceClosureFile(
                path=resolved,
                repo_path=_display_path(resolved, repo_root),
                text=text,
            )
        )
        for match in QUOTED_INCLUDE_RE.finditer(text):
            include_text = match.group(1)
            if not include_text.lower().endswith((".h", ".hpp", ".inl", ".c", ".cpp")):
                continue
            include_path = _resolve_include(include_text, resolved, repo_root)
            if include_path is not None:
                visit(include_path)

    visit(source_path)
    return tuple(files)


def _docblocks(text: str) -> tuple[_Docblock, ...]:
    return tuple(_Docblock(match.start(), match.end(), match.group(0)) for match in DOXYGEN_BLOCK_RE.finditer(text))


def _docblock_markers(block: _Docblock) -> tuple[tuple[str, str, int], ...]:
    rows: list[tuple[str, str, int]] = []
    for offset, raw_line in enumerate(block.text.splitlines()):
        line = raw_line.strip()
        if line.startswith("/**"):
            line = line[3:].strip()
        if line.endswith("*/"):
            line = line[:-2].strip()
        if line.startswith("*"):
            line = line[1:].strip()
        match = EMITS_LINE_RE.fullmatch(line)
        if match:
            rows.append((normalize_emission_address(match.group(1)), match.group(2).strip(), offset))
    return tuple(rows)


def _mask_comments_and_literals(text: str) -> str:
    """Keep source offsets stable while hiding comments and quoted contents."""

    chars = list(text)
    index = 0
    while index < len(chars):
        if text.startswith("//", index):
            end = text.find("\n", index + 2)
            if end < 0:
                end = len(text)
            for pos in range(index, end):
                chars[pos] = " "
            index = end
            continue
        if text.startswith("/*", index):
            end = text.find("*/", index + 2)
            end = len(text) if end < 0 else end + 2
            for pos in range(index, end):
                if chars[pos] not in "\r\n":
                    chars[pos] = " "
            index = end
            continue
        if chars[index] in {'"', "'"}:
            quote = chars[index]
            chars[index] = " "
            index += 1
            while index < len(chars):
                current = chars[index]
                if current == "\\":
                    chars[index] = " "
                    if index + 1 < len(chars):
                        chars[index + 1] = " "
                    index += 2
                    continue
                if current == quote:
                    chars[index] = " "
                    index += 1
                    break
                if current not in "\r\n":
                    chars[index] = " "
                index += 1
            continue
        index += 1
    return "".join(chars)


def _declaration_start(masked: str, position: int) -> int:
    start = position
    while start > 0 and masked[start - 1] not in ";{}":
        start -= 1
    while start < position and masked[start].isspace():
        start += 1
    return start


def _type_definitions(masked: str, name: str) -> tuple[_Construct, ...]:
    if not re.fullmatch(r"[A-Za-z_]\w*", name):
        return ()
    pattern = re.compile(
        rf"\b(?:class|struct)\s+(?:[A-Za-z_]\w*\s+)*{re.escape(name)}\b[^;{{]*{{"
    )
    return tuple(_Construct(match.start(), match.end()) for match in pattern.finditer(masked))


def _matching_paren(masked: str, open_index: int) -> int | None:
    depth = 0
    for index in range(open_index, len(masked)):
        char = masked[index]
        if char == "(":
            depth += 1
        elif char == ")":
            depth -= 1
            if depth == 0:
                return index
    return None


def _function_body_open(masked: str, close_paren: int) -> int | None:
    index = close_paren + 1
    while index < len(masked):
        if masked[index].isspace():
            index += 1
            continue
        char = masked[index]
        if char == "{":
            return index
        if char in ";=,)}]":
            return None
        if char == ":":
            # Constructor initializer list. VC5 source cannot use braced member
            # initializers, so the next top-level opening brace is the body.
            paren_depth = 0
            index += 1
            while index < len(masked):
                char = masked[index]
                if char == "(":
                    paren_depth += 1
                elif char == ")" and paren_depth:
                    paren_depth -= 1
                elif char == "{" and paren_depth == 0:
                    return index
                elif char == ";" and paren_depth == 0:
                    return None
                index += 1
            return None
        token = re.match(r"[A-Za-z_]\w*", masked[index:])
        if token:
            index += len(token.group(0))
            continue
        return None
    return None


def _function_definitions(masked: str, name: str) -> tuple[_Construct, ...]:
    if not name or "(" in name or ")" in name:
        return ()
    pattern = re.compile(rf"(?<![A-Za-z0-9_]){re.escape(name)}\s*\(")
    rows: list[_Construct] = []
    for match in pattern.finditer(masked):
        open_paren = masked.find("(", match.start(), match.end())
        close_paren = _matching_paren(masked, open_paren)
        if close_paren is None:
            continue
        body_open = _function_body_open(masked, close_paren)
        if body_open is None:
            continue
        rows.append(_Construct(_declaration_start(masked, match.start()), body_open + 1))
    return tuple(rows)


def _data_definitions(masked: str, name: str) -> tuple[_Construct, ...]:
    if not name:
        return ()
    pattern = re.compile(rf"(?<![A-Za-z0-9_]){re.escape(name)}(?![A-Za-z0-9_])")
    rows: list[_Construct] = []
    for match in pattern.finditer(masked):
        start = _declaration_start(masked, match.start())
        end = masked.find(";", match.end())
        if end < 0:
            continue
        statement = masked[start : end + 1]
        prefix = masked[start : match.start()]
        if re.search(r"\bextern\b", statement) or "(" in prefix:
            continue
        if "{" in prefix or "}" in prefix:
            continue
        rows.append(_Construct(start, end + 1))
    return tuple(rows)


def _constructs(text: str, *, kind: str, name: str) -> tuple[_Construct, ...]:
    if kind == "type-definition":
        masked = _mask_comments_and_literals(text)
        return _type_definitions(masked, name)
    expected_kind = {
        "function-definition": "function",
        "data-definition": "data",
    }.get(kind)
    if expected_kind is not None:
        return tuple(
            _Construct(item.start, item.end)
            for item in parse_source_constructs(text)
            if item.kind == expected_kind and item.name == name
        )
    raise ValueError(f"unsupported emission anchor kind {kind!r}")


def _attached_docblock(text: str, construct: _Construct) -> _Docblock | None:
    candidates = [block for block in _docblocks(text) if block.end <= construct.start]
    if not candidates:
        return None
    block = candidates[-1]
    if text[block.end : construct.start].strip():
        return None
    return block


def validate_source_emission_marker(
    *,
    source_from: str | Path,
    repo_root: Path,
    anchor: EmissionAnchor,
    address: str,
) -> SourceEmissionMarker:
    normalized_address = normalize_emission_address(address)
    normalized_path = normalize_anchor_path(anchor.path)
    if anchor.kind not in ANCHOR_KINDS:
        raise ValueError(
            f"emission_anchor.kind must be one of {sorted(ANCHOR_KINDS)}, not {anchor.kind!r}"
        )
    if not isinstance(anchor.name, str) or not anchor.name.strip():
        raise ValueError("emission_anchor.name must be a non-empty string")
    if anchor.name != anchor.name.strip():
        raise ValueError("emission_anchor.name must not contain leading or trailing whitespace")

    closure = collect_source_closure(source_from, repo_root=repo_root)
    by_resolved_path = {item.path.resolve(): item for item in closure}
    anchor_path = (repo_root / normalized_path).resolve()
    anchor_file = by_resolved_path.get(anchor_path)
    if anchor_file is None:
        raise ValueError(
            f"emission_anchor.path {normalized_path!r} is not reachable from source_from {str(source_from)!r}"
        )

    occurrences: list[tuple[SourceClosureFile, _Docblock, str, int]] = []
    for source_file in closure:
        for block in _docblocks(source_file.text):
            for marker_address, description, line_offset in _docblock_markers(block):
                if marker_address == normalized_address:
                    occurrences.append((source_file, block, description, line_offset))
    if len(occurrences) > 1:
        locations = ", ".join(
            f"{item.repo_path}:{item.text.count(chr(10), 0, block.start) + line_offset + 1}"
            for item, block, _description, line_offset in occurrences
        )
        raise ValueError(f"duplicate Emits {normalized_address}: markers in source closure: {locations}")

    constructs = _constructs(anchor_file.text, kind=anchor.kind, name=anchor.name)
    if len(constructs) != 1:
        raise ValueError(
            f"emission_anchor {anchor.kind} {anchor.name!r} must resolve exactly once in "
            f"{normalized_path}; found {len(constructs)}"
        )
    attached = _attached_docblock(anchor_file.text, constructs[0])
    if attached is None:
        raise ValueError(
            f"emission_anchor {anchor.kind} {anchor.name!r} in {normalized_path} has no immediately attached /** */ docblock"
        )
    attached_markers = [
        (description, line_offset)
        for marker_address, description, line_offset in _docblock_markers(attached)
        if marker_address == normalized_address
    ]
    if len(attached_markers) != 1:
        raise ValueError(
            f"emission_anchor {anchor.kind} {anchor.name!r} in {normalized_path} requires exactly one "
            f"attached 'Emits {normalized_address}: <description>' marker"
        )
    if len(occurrences) != 1 or occurrences[0][0].path.resolve() != anchor_path:
        raise ValueError(
            f"Emits {normalized_address}: marker must occur exactly once at emission_anchor.path {normalized_path}"
        )

    description, line_offset = attached_markers[0]
    line = anchor_file.text.count("\n", 0, attached.start) + line_offset + 1
    return SourceEmissionMarker(
        address=normalized_address,
        description=description,
        path=normalized_path,
        line=line,
        anchor=EmissionAnchor(path=normalized_path, kind=anchor.kind, name=anchor.name),
    )
