from __future__ import annotations

"""Governed Source Traceability v1 inventory, review, and migration helpers.

The inventory, proposal, and tracker-payload operations are non-mutating.
``batch --apply`` is the only source-writing operation; it validates the whole
batch first, uses atomic replacement, and requires every rewritten file to
preserve its complete non-comment C/C++ token stream.  Progress state is never
written here: the generated payload must go through the revision-guarded
``progress source-trace replace-batch`` command.
"""

import argparse
from copy import deepcopy
from dataclasses import asdict, dataclass
import json
from pathlib import Path, PurePosixPath
import re
import sys
from typing import Any, Mapping, Sequence

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from _recoil.lib.source_traceability import (
    ANCHOR_ID_RE,
    SourceArtifactIndex,
    load_artifact_rows,
    normalize_artifact_id,
    merge_source_trace_documents,
    parse_source_trace_text,
    validate_source_trace,
)
from _recoil.lib.comment_hygiene import (
    classify_semantic_row,
    comment_rows,
    is_legacy_migration_construct_title,
)
from _recoil.lib.live_progress import atomic_replace
from _recoil.commands.source_trace_progress import (
    normalize_source_traceability,
    normalize_unresolved_legacy_claims,
)
from _recoil.lib.tooling import configure_stdio


INVENTORY_SCHEMA = "recoil-source-trace-migration-v1"
RELATIONS = frozenset({"defines", "emits"})
LEGACY_REIMPLEMENTS_RE = re.compile(
    r"Reimplements[ \t]+(?:(data)[ \t]+)?(0x[0-9A-Fa-f]+):[ \t]*"
    r"(\S(?:[^\r\n]*?\S)?)(?=[ \t]*(?:\*/)?[ \t]*(?:\r\n|\n|\r)?$)",
    re.IGNORECASE,
)
LEGACY_EMITS_RE = re.compile(
    r"Emits[ \t]+(0x[0-9A-Fa-f]+):[ \t]*"
    r"(\S(?:[^\r\n]*?\S)?)(?=[ \t]*(?:\*/)?[ \t]*(?:\r\n|\n|\r)?$)",
    re.IGNORECASE,
)
ARTIFACT_ID_RE = re.compile(
    r"^[a-z0-9_-]+:(?:function|data|logical-function|logical-data):\S+$"
)


class SourceTraceMigrationError(RuntimeError):
    pass


@dataclass(frozen=True)
class LegacyOccurrence:
    path: str
    line: int
    relation: str
    legacy_artifact_id: str
    description: str

    def key(self) -> tuple[str, int]:
        return (self.path, self.line)


@dataclass(frozen=True)
class MigrationDebt:
    reason_code: str
    path: str
    line: int
    legacy_artifact_id: str
    message: str


@dataclass(frozen=True)
class MigrationInventoryRow:
    path: str
    line: int
    expected_relation: str
    relation: str
    expected_legacy_artifact_id: str
    expected_description: str
    state: str
    artifact_id: str | None
    anchor_id: str | None
    output_section: str | None
    translation_unit: str | None
    reason_code: str | None
    reviewed: bool
    record_tracker_state: bool

    def key(self) -> tuple[str, int]:
        return (self.path, self.line)


@dataclass(frozen=True)
class MigrationInventory:
    schema: str
    rows: tuple[MigrationInventoryRow, ...]


@dataclass(frozen=True)
class SourceBytes:
    text: str
    encoding: str
    newline: str
    bom: bool

    def encode(self, text: str) -> bytes:
        payload = text.encode("utf-8" if self.encoding == "utf-8" else self.encoding)
        return b"\xef\xbb\xbf" + payload if self.bom else payload


@dataclass(frozen=True)
class MigrationProposal:
    path: str
    encoding: str
    newline: str
    bom: bool
    occurrences: tuple[LegacyOccurrence, ...]
    debts: tuple[MigrationDebt, ...]
    artifact_ids: tuple[str, ...]
    tracker_states: tuple[Mapping[str, Any], ...]
    proposed_text: str | None
    proposed_bytes: bytes | None
    token_equivalent: bool

    @property
    def ready(self) -> bool:
        return self.proposed_bytes is not None and self.token_equivalent

    def to_dict(self, *, include_text: bool = False) -> dict[str, Any]:
        result: dict[str, Any] = {
            "report_version": 1,
            "kind": "source-trace-migration-proposal",
            "topology_only": True,
            "acceptance_effect": "none",
            "path": self.path,
            "encoding": self.encoding,
            "newline": self.newline,
            "bom": self.bom,
            "ready": self.ready,
            "token_equivalent": self.token_equivalent,
            "occurrences": [asdict(item) for item in self.occurrences],
            "debts": [asdict(item) for item in self.debts],
            "artifact_ids": list(self.artifact_ids),
            "tracker_states": [deepcopy(dict(item)) for item in self.tracker_states],
        }
        if include_text:
            result["proposed_text"] = self.proposed_text
        return result


def _normalized_path(value: Any, *, label: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise SourceTraceMigrationError(f"{label} must be a non-empty path")
    raw = value.strip().replace("\\", "/")
    path = PurePosixPath(raw)
    if Path(raw).is_absolute() or re.match(r"^[A-Za-z]:/", raw):
        raise SourceTraceMigrationError(f"{label} must be repository-relative")
    if any(part in {"", ".", ".."} for part in path.parts) or path.as_posix() != raw:
        raise SourceTraceMigrationError(f"{label} must be normalized as {path.as_posix()!r}")
    return raw


def _nonempty(value: Any, *, label: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise SourceTraceMigrationError(f"{label} must be a non-empty string")
    if value != value.strip():
        raise SourceTraceMigrationError(f"{label} must not have outer whitespace")
    return value


def _optional_nonempty(value: Any, *, label: str) -> str | None:
    if value is None:
        return None
    return _nonempty(value, label=label)


def parse_migration_inventory(value: Mapping[str, Any] | str) -> MigrationInventory:
    if isinstance(value, str):
        try:
            parsed = json.loads(value)
        except json.JSONDecodeError as exc:
            raise SourceTraceMigrationError(f"invalid migration inventory JSON: {exc}") from exc
    else:
        parsed = dict(value)
    if not isinstance(parsed, Mapping):
        raise SourceTraceMigrationError("migration inventory must be an object")
    if set(parsed) != {"schema", "rows"}:
        raise SourceTraceMigrationError("migration inventory requires exactly schema and rows")
    if parsed.get("schema") != INVENTORY_SCHEMA:
        raise SourceTraceMigrationError(
            f"migration inventory schema must be {INVENTORY_SCHEMA!r}"
        )
    raw_rows = parsed.get("rows")
    if not isinstance(raw_rows, list):
        raise SourceTraceMigrationError("migration inventory rows must be a list")
    allowed = {
        "path",
        "line",
        "expected_relation",
        "relation",
        "expected_legacy_artifact_id",
        "expected_description",
        "state",
        "artifact_id",
        "anchor_id",
        "output_section",
        "translation_unit",
        "reason_code",
        "reviewed",
        "record_tracker_state",
    }
    rows: list[MigrationInventoryRow] = []
    seen: set[tuple[str, int]] = set()
    for ordinal, raw in enumerate(raw_rows):
        label = f"migration inventory row {ordinal}"
        if not isinstance(raw, Mapping):
            raise SourceTraceMigrationError(f"{label} must be an object")
        unknown = sorted(set(raw) - allowed)
        missing = sorted(allowed - set(raw))
        if unknown or missing:
            raise SourceTraceMigrationError(
                f"{label} fields differ: missing={missing}, unknown={unknown}"
            )
        path = _normalized_path(raw["path"], label=f"{label}.path")
        line = raw["line"]
        if isinstance(line, bool) or not isinstance(line, int) or line < 1:
            raise SourceTraceMigrationError(f"{label}.line must be a positive integer")
        relation = _nonempty(raw["expected_relation"], label=f"{label}.expected_relation")
        if relation not in RELATIONS:
            raise SourceTraceMigrationError(
                f"{label}.expected_relation must be one of {sorted(RELATIONS)}"
            )
        canonical_relation = _nonempty(raw["relation"], label=f"{label}.relation")
        if canonical_relation not in RELATIONS:
            raise SourceTraceMigrationError(
                f"{label}.relation must be one of {sorted(RELATIONS)}"
            )
        legacy_id = normalize_artifact_id(
            _nonempty(
                raw["expected_legacy_artifact_id"],
                label=f"{label}.expected_legacy_artifact_id",
            )
        )
        description = _nonempty(
            raw["expected_description"], label=f"{label}.expected_description"
        )
        state = _nonempty(raw["state"], label=f"{label}.state")
        if state not in {"resolved", "unresolved", "not-applicable"}:
            raise SourceTraceMigrationError(
                f"{label}.state must be resolved, unresolved, or not-applicable"
            )
        artifact_id = _optional_nonempty(raw["artifact_id"], label=f"{label}.artifact_id")
        anchor_id = _optional_nonempty(raw["anchor_id"], label=f"{label}.anchor_id")
        section = _optional_nonempty(raw["output_section"], label=f"{label}.output_section")
        translation_unit = (
            _normalized_path(raw["translation_unit"], label=f"{label}.translation_unit")
            if raw["translation_unit"] is not None
            else None
        )
        reason = _optional_nonempty(raw["reason_code"], label=f"{label}.reason_code")
        if raw["reviewed"] is not True:
            raise SourceTraceMigrationError(
                f"{label}.reviewed must be true before a rewrite can be proposed"
            )
        if not isinstance(raw["record_tracker_state"], bool):
            raise SourceTraceMigrationError(
                f"{label}.record_tracker_state must be a boolean"
            )
        record_tracker_state = raw["record_tracker_state"]
        if state == "resolved" and not record_tracker_state:
            raise SourceTraceMigrationError(
                f"{label}: resolved rows must record the tracker state"
            )
        if record_tracker_state and artifact_id is None:
            raise SourceTraceMigrationError(
                f"{label}: tracker-state decisions require an existing artifact id"
            )
        if artifact_id is None:
            if any(item is not None for item in (anchor_id, section, translation_unit)):
                raise SourceTraceMigrationError(
                    f"{label}: unresolved row must not supply anchor/section/translation unit"
                )
            if state != "unresolved" or reason != "missing-artifact-identity":
                raise SourceTraceMigrationError(
                    f"{label}: an absent artifact requires state='unresolved' and "
                    "reason_code='missing-artifact-identity'"
                )
        else:
            artifact_id = normalize_artifact_id(artifact_id)
            if ARTIFACT_ID_RE.fullmatch(artifact_id) is None:
                raise SourceTraceMigrationError(
                    f"{label}.artifact_id is not a supported exact artifact id"
                )
            if state == "resolved":
                if anchor_id is None or ANCHOR_ID_RE.fullmatch(anchor_id) is None:
                    raise SourceTraceMigrationError(
                        f"{label}.anchor_id must match 'recoil:anchor:<stable-id>'"
                    )
                if section is None or not section.startswith("."):
                    raise SourceTraceMigrationError(
                        f"{label}.output_section must be an exact dotted section name"
                    )
                if translation_unit is None:
                    raise SourceTraceMigrationError(
                        f"{label}.translation_unit is required for resolved rows"
                    )
            elif any(item is not None for item in (anchor_id, section, translation_unit)):
                raise SourceTraceMigrationError(
                    f"{label}: {state} row must not supply an edge anchor/section/"
                    "translation unit"
                )
        if state == "resolved":
            if reason is not None:
                raise SourceTraceMigrationError(
                    f"{label}.reason_code must be null for resolved state"
                )
        else:
            if reason is None:
                raise SourceTraceMigrationError(f"{label}.reason_code is required")
            if re.fullmatch(r"[a-z0-9][a-z0-9._-]*", reason) is None:
                raise SourceTraceMigrationError(
                    f"{label}.reason_code must be a governed lowercase code"
                )
        row = MigrationInventoryRow(
            path,
            line,
            relation,
            canonical_relation,
            legacy_id,
            description,
            state,
            artifact_id,
            anchor_id,
            section,
            translation_unit,
            reason,
            True,
            record_tracker_state,
        )
        if row.key() in seen:
            raise SourceTraceMigrationError(f"duplicate migration inventory key {row.key()!r}")
        seen.add(row.key())
        rows.append(row)
    return MigrationInventory(INVENTORY_SCHEMA, tuple(rows))


def load_migration_inventory(
    *,
    payload_json: str | None = None,
    payload_file: Path | None = None,
) -> MigrationInventory:
    if (payload_json is None) == (payload_file is None):
        raise SourceTraceMigrationError(
            "provide exactly one of payload_json or payload_file"
        )
    if payload_file is not None:
        try:
            payload_json = payload_file.read_text(encoding="utf-8")
        except (OSError, UnicodeError) as exc:
            raise SourceTraceMigrationError(
                f"{payload_file}: unreadable UTF-8 migration inventory: {exc}"
            ) from exc
    assert payload_json is not None
    try:
        container = json.loads(payload_json)
    except json.JSONDecodeError:
        container = None
    if (
        isinstance(container, Mapping)
        and container.get("kind") == "source-trace-conservative-review"
        and isinstance(container.get("inventory"), Mapping)
    ):
        return parse_migration_inventory(container["inventory"])
    return parse_migration_inventory(payload_json)


def decode_source_bytes(raw: bytes, *, path: str) -> SourceBytes:
    bom = raw.startswith(b"\xef\xbb\xbf")
    payload = raw[3:] if bom else raw
    if bom:
        encoding = "utf-8"
        try:
            text = payload.decode("utf-8", errors="strict")
        except UnicodeDecodeError as exc:
            raise SourceTraceMigrationError(f"{path}: invalid UTF-8 BOM source: {exc}") from exc
    else:
        try:
            text = payload.decode("utf-8", errors="strict")
            encoding = "utf-8"
        except UnicodeDecodeError:
            try:
                text = payload.decode("cp1252", errors="strict")
                encoding = "cp1252"
            except UnicodeDecodeError as exc:
                raise SourceTraceMigrationError(
                    f"{path}: source is neither strict UTF-8 nor strict cp1252: {exc}"
                ) from exc
    document = parse_source_trace_text(text, path=path, encoding=encoding)
    newline = {"crlf": "\r\n", "lf": "\n", "cr": "\r", "none": ""}[document.newline]
    return SourceBytes(text, encoding, newline, bom)


def scan_legacy_source(text: str, *, path: str) -> tuple[LegacyOccurrence, ...]:
    normalized_path = _normalized_path(path, label="source path")
    document = parse_source_trace_text(text, path=normalized_path)
    rows = [
        LegacyOccurrence(
            normalized_path,
            item.line,
            item.relation,
            normalize_artifact_id(item.artifact_id),
            item.description,
        )
        for item in document.legacy_artifacts
    ]
    existing_lines = {item.line for item in rows}
    binary = (
        "messages"
        if normalized_path.casefold().startswith("src/messages/")
        else "recoil"
    )
    source_lines = text.splitlines(keepends=True)
    for item in document.unsupported_legacy_addresses:
        if item.line in existing_lines or item.line > len(source_lines):
            continue
        matched = _legacy_match(source_lines[item.line - 1], binary=binary)
        if matched is None:
            continue
        _, relation, artifact_id, description = matched
        rows.append(
            LegacyOccurrence(
                normalized_path,
                item.line,
                relation,
                normalize_artifact_id(artifact_id),
                description,
            )
        )
        existing_lines.add(item.line)
    return tuple(sorted(rows, key=lambda item: item.line))


def inventory_legacy_source(
    text: str,
    *,
    path: str,
    artifact_index: SourceArtifactIndex,
) -> dict[str, Any]:
    occurrences = scan_legacy_source(text, path=path)
    debts = []
    for item in occurrences:
        if artifact_index.resolve(item.legacy_artifact_id) is None:
            debts.append(
                MigrationDebt(
                    "missing-artifact-identity",
                    item.path,
                    item.line,
                    item.legacy_artifact_id,
                    "legacy address has no exact current tracker artifact; no row or extent "
                    "may be fabricated",
                )
            )
    return {
        "report_version": 1,
        "kind": "source-trace-legacy-inventory",
        "topology_only": True,
        "acceptance_effect": "none",
        "path": path,
        "occurrences": [asdict(item) for item in occurrences],
        "debts": [asdict(item) for item in debts],
    }


def _comment_spans(text: str) -> tuple[tuple[int, int], ...]:
    spans: list[tuple[int, int]] = []
    index = 0
    while index < len(text):
        if text.startswith("//", index):
            start = index
            end = text.find("\n", index + 2)
            end = len(text) if end < 0 else end
            spans.append((start, end))
            index = end
            continue
        if text.startswith("/*", index):
            start = index
            close = text.find("*/", index + 2)
            if close < 0:
                raise SourceTraceMigrationError("unterminated block comment")
            end = close + 2
            spans.append((start, end))
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
    return tuple(spans)


def non_comment_token_stream(text: str) -> tuple[str, ...]:
    """Return exact C/C++ preprocessing tokens, excluding comments/whitespace."""

    tokens: list[str] = []
    index = 0
    length = len(text)
    punctuators = (
        ">>=", "<<=", "->*", "...", "##", "::", ".*", "->", "++", "--", "<<",
        ">>", "<=", ">=", "==", "!=", "&&", "||", "*=", "/=", "%=", "+=", "-=",
        "&=", "^=", "|=",
    )
    while index < length:
        if text[index].isspace():
            index += 1
            continue
        if text.startswith("//", index):
            end = text.find("\n", index + 2)
            index = length if end < 0 else end
            continue
        if text.startswith("/*", index):
            close = text.find("*/", index + 2)
            if close < 0:
                raise SourceTraceMigrationError("unterminated block comment")
            index = close + 2
            continue
        if text[index] in {'"', "'"}:
            quote = text[index]
            start = index
            index += 1
            while index < length:
                if text[index] == "\\":
                    index += 2
                elif index < length and text[index] == quote:
                    index += 1
                    break
                else:
                    index += 1
            else:
                raise SourceTraceMigrationError("unterminated string/character literal")
            tokens.append(text[start:index])
            continue
        identifier = re.match(r"[A-Za-z_$][A-Za-z0-9_$]*", text[index:])
        if identifier is not None:
            token = identifier.group(0)
            tokens.append(token)
            index += len(token)
            continue
        number = re.match(
            r"(?:0[xX][0-9A-Fa-f]+|0[bB][01]+|(?:\d+(?:\.\d*)?|\.\d+)"
            r"(?:[eEpP][+-]?\d+)?)(?:[A-Za-z_][A-Za-z0-9_]*)?",
            text[index:],
        )
        if number is not None:
            token = number.group(0)
            tokens.append(token)
            index += len(token)
            continue
        punctuator = next((item for item in punctuators if text.startswith(item, index)), None)
        if punctuator is not None:
            tokens.append(punctuator)
            index += len(punctuator)
            continue
        tokens.append(text[index])
        index += 1
    return tuple(tokens)


def verify_comment_only_equivalence(before: str, after: str) -> None:
    before_tokens = non_comment_token_stream(before)
    after_tokens = non_comment_token_stream(after)
    if before_tokens != after_tokens:
        mismatch = next(
            (
                index
                for index, pair in enumerate(zip(before_tokens, after_tokens))
                if pair[0] != pair[1]
            ),
            min(len(before_tokens), len(after_tokens)),
        )
        raise SourceTraceMigrationError(
            "comment-only equivalence failed at non-comment token "
            f"{mismatch}: before={before_tokens[mismatch:mismatch + 3]!r}, "
            f"after={after_tokens[mismatch:mismatch + 3]!r}"
        )


def _line_starts(text: str) -> list[int]:
    starts = [0]
    starts.extend(match.end() for match in re.finditer(r"\r\n|\n|\r", text))
    return starts


def _legacy_match(
    raw_line: str, *, binary: str = "recoil"
) -> tuple[re.Match[str], str, str, str] | None:
    match = LEGACY_REIMPLEMENTS_RE.search(raw_line)
    if match is not None:
        kind = "data" if match.group(1) else "function"
        artifact_id = f"{binary}:{kind}:0x{int(match.group(2), 16):x}"
        return match, "defines", artifact_id, match.group(3).strip()
    match = LEGACY_EMITS_RE.search(raw_line)
    if match is not None:
        artifact_id = f"{binary}:function:0x{int(match.group(1), 16):x}"
        return match, "emits", artifact_id, match.group(2).strip()
    return None


def _canonical_directive_prefix(prefix: str) -> str:
    """Retain only indentation/comment syntax before a canonical directive."""

    match = re.match(r"^(\s*(?:/\*\*?|\*|//)\s*)", prefix)
    if match is None:
        raise SourceTraceMigrationError(
            "legacy marker line has no supported comment directive prefix"
        )
    return match.group(1)


def _legacy_marker_is_standalone(raw_line: str) -> bool:
    """Return whether the legacy marker is the line's sole semantic comment text."""

    matched = _legacy_match(raw_line)
    if matched is None:
        return False
    match = matched[0]
    prefix = raw_line[: match.start()]
    try:
        return _canonical_directive_prefix(prefix) == prefix
    except SourceTraceMigrationError:
        return False


_REDUNDANT_OCCURRENCE_REASONS = frozenset(
    {
        "redundant-legacy-after-canonical-source",
        "redundant-legacy-occurrence",
        "redundant-detached-registry",
    }
)


def _delete_standalone_legacy_row(
    raw_line: str,
    *,
    description: str,
    row: MigrationInventoryRow,
) -> bool:
    """Return whether review requires deleting the complete semantic row."""

    if not _legacy_marker_is_standalone(raw_line):
        return False
    if (
        not row.record_tracker_state
        and row.reason_code in _REDUNDANT_OCCURRENCE_REASONS
    ):
        return True
    if row.state in {"unresolved", "not-applicable"}:
        return (
            classify_semantic_row(description) is not None
            or is_legacy_migration_construct_title(description)
        )
    return False


def _line_terminator(raw_line: str) -> str:
    matched = re.search(r"(?:\r\n|\n|\r)$", raw_line)
    return matched.group(0) if matched is not None else ""


def _remove_standalone_comment_row(
    raw_line: str,
    *,
    match: re.Match[str],
) -> tuple[str, bool]:
    """Remove a semantic row while retaining any required block delimiters.

    The boolean reports whether the complete physical line disappeared.
    """

    prefix = raw_line[: match.start()]
    suffix = raw_line[match.end() :]
    newline = _line_terminator(raw_line)
    if "//" in prefix:
        return "", True
    opener = "/**" if "/**" in prefix else ("/*" if "/*" in prefix else "")
    closer = "*/" if "*/" in suffix else ""
    if opener and closer:
        indent = prefix[: prefix.find(opener)]
        return f"{indent}{opener} {closer}{newline}", False
    if opener:
        indent = prefix[: prefix.find(opener)]
        return f"{indent}{opener}{newline}", False
    if closer:
        indent = re.match(r"^[ \t]*", prefix).group(0)
        return f"{indent} */{newline}", False
    return "", True


def _comment_line_range(text: str, span: tuple[int, int]) -> tuple[int, int]:
    start_line = text.count("\n", 0, span[0])
    end_line = text.count("\n", 0, span[1])
    return start_line, end_line


def _normalize_touched_blank_separators(
    lines: list[str],
    *,
    deleted_indexes: set[int],
) -> str:
    """Remove deleted rows and keep at most one nearby blank separator."""

    result: list[str] = []
    index = 0
    while index < len(lines):
        if lines[index] and lines[index].strip():
            result.append(lines[index])
            index += 1
            continue
        start = index
        while index < len(lines) and (not lines[index] or not lines[index].strip()):
            index += 1
        indexes = range(start, index)
        touched = any(item in deleted_indexes for item in indexes)
        surviving = [lines[item] for item in indexes if lines[item]]
        if not touched:
            result.extend(surviving)
        elif surviving:
            result.append(surviving[0])
    return "".join(result)


def propose_source_trace_rewrite(
    source: bytes | str,
    *,
    path: str,
    inventory: MigrationInventory,
    artifact_index: SourceArtifactIndex,
) -> MigrationProposal:
    normalized_path = _normalized_path(path, label="source path")
    source_bytes = (
        decode_source_bytes(source, path=normalized_path)
        if isinstance(source, bytes)
        else SourceBytes(
            source,
            "utf-8",
            {"crlf": "\r\n", "lf": "\n", "cr": "\r", "none": ""}[
                parse_source_trace_text(source, path=normalized_path).newline
            ],
            False,
        )
    )
    text = source_bytes.text
    occurrences = scan_legacy_source(text, path=normalized_path)
    by_key = {row.key(): row for row in inventory.rows if row.path == normalized_path}
    occurrence_keys = {item.key() for item in occurrences}
    extra = sorted(set(by_key) - occurrence_keys)
    if extra:
        raise SourceTraceMigrationError(
            f"migration inventory contains stale source locations for {normalized_path}: {extra}"
        )
    debts: list[MigrationDebt] = []
    replacements: dict[int, MigrationInventoryRow] = {}
    artifact_ids: list[str] = []
    seen_artifacts: set[str] = set()
    resolved_artifacts: set[str] = set()
    tracker_states: list[dict[str, Any]] = []
    for occurrence in occurrences:
        row = by_key.get(occurrence.key())
        if row is None:
            raise SourceTraceMigrationError(
                f"{occurrence.path}:{occurrence.line}: legacy occurrence has no "
                "exact reviewed migration inventory row"
            )
        expected = (
            row.expected_relation,
            row.expected_legacy_artifact_id,
            row.expected_description,
        )
        actual = (
            occurrence.relation,
            occurrence.legacy_artifact_id,
            occurrence.description,
        )
        if expected != actual:
            raise SourceTraceMigrationError(
                f"{normalized_path}:{occurrence.line}: legacy source snapshot changed: "
                f"expected={expected!r}, actual={actual!r}"
            )
        if row.artifact_id is None:
            debts.append(
                MigrationDebt(
                    row.reason_code or "missing-artifact-identity",
                    row.path,
                    row.line,
                    row.expected_legacy_artifact_id,
                    "reviewed inventory records no safe current artifact identity",
                )
            )
        else:
            target = artifact_index.resolve(row.artifact_id)
            if target is None:
                raise SourceTraceMigrationError(
                    f"{normalized_path}:{row.line}: reviewed artifact "
                    f"{row.artifact_id!r} no longer resolves in the tracker"
                )
            if not row.record_tracker_state:
                debts.append(
                    MigrationDebt(
                        row.reason_code or "occurrence-only-removal",
                        row.path,
                        row.line,
                        row.artifact_id,
                        "reviewed occurrence-only cleanup removes the legacy address "
                        "claim without creating a duplicate tracker-state decision",
                    )
                )
                replacements[row.line] = row
                continue
            if row.state == "resolved" and target.output_section != row.output_section:
                raise SourceTraceMigrationError(
                    f"{normalized_path}:{row.line}: output section snapshot changed: "
                    f"inventory={row.output_section!r}, tracker={target.output_section!r}"
                )
            if row.artifact_id in seen_artifacts:
                raise SourceTraceMigrationError(
                    f"artifact {row.artifact_id!r} appears more than once in migration proposal"
                )
            seen_artifacts.add(row.artifact_id)
            artifact_ids.append(row.artifact_id)
            source_edges: list[dict[str, Any]] = []
            if row.state == "resolved":
                resolved_artifacts.add(row.artifact_id)
                source_edges.append(
                    {
                        "relation": row.relation,
                        "anchor_id": row.anchor_id,
                        "emission_context": {
                            "translation_unit": row.translation_unit,
                        },
                        "evidence_ids": [],
                    }
                )
            else:
                debts.append(
                    MigrationDebt(
                        row.reason_code or row.state,
                        row.path,
                        row.line,
                        row.artifact_id,
                        "reviewed topology decision leaves this artifact without a "
                        "resolved source edge",
                    )
                )
            tracker_states.append(
                {
                    "artifact_id": row.artifact_id,
                    "source_traceability": {
                        "state": row.state,
                        "source_edges": source_edges,
                        "reason_code": (
                            None if row.state == "resolved" else row.reason_code
                        ),
                    },
                }
            )
        replacements[row.line] = row
    if not replacements:
        raise SourceTraceMigrationError(f"{normalized_path}: no legacy occurrences to rewrite")

    lines = text.splitlines(keepends=True)
    starts = _line_starts(text)
    spans = _comment_spans(text)
    comment_for_line: dict[int, tuple[int, int]] = {}
    for line_number in replacements:
        if line_number > len(starts):
            raise SourceTraceMigrationError(f"{normalized_path}:{line_number}: line disappeared")
        offset = starts[line_number - 1]
        span = next((item for item in spans if item[0] <= offset < item[1]), None)
        if span is None:
            # The line can start before an indented comment.
            line_end = starts[line_number] if line_number < len(starts) else len(text)
            span = next(
                (item for item in spans if offset <= item[0] < line_end),
                None,
            )
        if span is None:
            raise SourceTraceMigrationError(
                f"{normalized_path}:{line_number}: legacy marker is not inside a comment"
            )
        comment_for_line[line_number] = span

    anchors_inserted: set[tuple[int, int]] = set()
    touched_comment_spans: set[tuple[int, int]] = set()
    deleted_line_indexes: set[int] = set()
    for line_number in sorted(replacements):
        row = replacements[line_number]
        raw_line = lines[line_number - 1]
        legacy_binary = row.expected_legacy_artifact_id.split(":", 1)[0]
        matched = _legacy_match(raw_line, binary=legacy_binary)
        if matched is None:
            raise SourceTraceMigrationError(
                f"{normalized_path}:{line_number}: legacy marker cannot be rewritten exactly"
            )
        match, relation, legacy_id, description = matched
        if (
            relation != row.expected_relation
            or normalize_artifact_id(legacy_id) != row.expected_legacy_artifact_id
            or description != row.expected_description
        ):
            raise SourceTraceMigrationError(
                f"{normalized_path}:{line_number}: line-level legacy snapshot changed"
            )
        delete_row = _delete_standalone_legacy_row(
            raw_line,
            description=description,
            row=row,
        )
        if delete_row:
            rewritten, line_deleted = _remove_standalone_comment_row(
                raw_line,
                match=match,
            )
            touched_comment_spans.add(comment_for_line[line_number])
            if line_deleted:
                deleted_line_indexes.add(line_number - 1)
        else:
            replacement_text = description
            output_prefix = raw_line[: match.start()]
            if row.state == "resolved":
                replacement_text = (
                    f"@recoil-artifact {row.relation} {row.output_section} "
                    f"{row.artifact_id}: {description}"
                )
                output_prefix = _canonical_directive_prefix(output_prefix)
            rewritten = (
                output_prefix
                + replacement_text
                + raw_line[match.end() :]
            )
        span = comment_for_line[line_number]
        if row.state == "resolved":
            replacement_text = (
                f"@recoil-artifact {row.relation} {row.output_section} "
                f"{row.artifact_id}: {description}"
            )
            output_prefix = _canonical_directive_prefix(raw_line[: match.start()])
        single_line_block = (
            row.state == "resolved"
            and span not in anchors_inserted
            and "/*" in raw_line[: match.start()]
            and "*/" in raw_line[match.end() :]
        )
        if single_line_block:
            newline = source_bytes.newline
            if not newline:
                raise SourceTraceMigrationError(
                    f"{normalized_path}: cannot expand a newline-free legacy comment"
                )
            indent = re.match(r"^[ \t]*", raw_line).group(0)
            rewritten = (
                f"{indent}/**{newline}"
                f"{indent} * @recoil-anchor {row.anchor_id}{newline}"
                f"{indent} * {replacement_text}{newline}"
                f"{indent} */{newline}"
            )
            anchors_inserted.add(span)
        elif row.state == "resolved" and span not in anchors_inserted:
            prefix = output_prefix
            anchor_line = f"{prefix}@recoil-anchor {row.anchor_id}"
            newline = source_bytes.newline
            if not newline:
                raise SourceTraceMigrationError(
                    f"{normalized_path}: cannot add an anchor to newline-free source"
                )
            # Preserve a line's terminator on the canonical artifact row.
            anchor_line += newline
            rewritten = anchor_line + rewritten
            anchors_inserted.add(span)
        lines[line_number - 1] = rewritten

    for span in touched_comment_spans:
        start_index, end_index = _comment_line_range(text, span)
        segment = "".join(lines[start_index : end_index + 1])
        if comment_rows(segment):
            continue
        for index in range(start_index, min(end_index + 1, len(lines))):
            lines[index] = ""
            deleted_line_indexes.add(index)

    proposed = _normalize_touched_blank_separators(
        lines,
        deleted_indexes=deleted_line_indexes,
    )
    verify_comment_only_equivalence(text, proposed)
    parsed = parse_source_trace_text(
        proposed, path=normalized_path, encoding=source_bytes.encoding
    )
    findings = validate_source_trace(parsed, artifact_index, strict=False)
    resolved_anchors = {
        row.anchor_id for row in replacements.values() if row.state == "resolved"
    }
    new_artifact_findings = [
        item
        for item in findings
        if item.artifact_id in resolved_artifacts
        or item.anchor_id in resolved_anchors
    ]
    if new_artifact_findings:
        first = new_artifact_findings[0]
        raise SourceTraceMigrationError(
            f"{first.path}:{first.line}: proposed canonical trace failed "
            f"{first.code}: {first.message}"
        )
    parsed_ids = [
        item.artifact_id
        for item in parsed.artifacts
        if item.artifact_id in resolved_artifacts
    ]
    if sorted(parsed_ids) != sorted(resolved_artifacts):
        raise SourceTraceMigrationError(
            "proposed canonical trace does not resolve every target artifact exactly once"
        )
    encoded = source_bytes.encode(proposed)
    if decode_source_bytes(encoded, path=normalized_path).text != proposed:
        raise SourceTraceMigrationError("proposed source does not round-trip its original encoding")
    return MigrationProposal(
        normalized_path,
        source_bytes.encoding,
        source_bytes.newline,
        source_bytes.bom,
        occurrences,
        tuple(debts),
        tuple(artifact_ids),
        tuple(tracker_states),
        proposed,
        encoded,
        True,
    )


SOURCE_SUFFIXES = frozenset({".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp"})


def _migration_legacy_document(
    text: str,
    *,
    path: str,
    encoding: str,
) -> Any:
    """Expose exactly parseable embedded legacy prose to governed review.

    The canonical parser deliberately inventories a prefixed
    ``Provenance: Reimplements ...`` row as unsupported syntax.  Migration
    review still needs its exact identity and attachment facts so it can remove
    the address without turning the row into a resolved claim.  Normalize only
    the temporary parse view; the source rewrite continues to use the original
    text and preserves its prose prefix.
    """

    document = parse_source_trace_text(text, path=path, encoding=encoding)
    if not document.unsupported_legacy_addresses:
        return document
    lines = text.splitlines(keepends=True)
    binary = "messages" if path.casefold().startswith("src/messages/") else "recoil"
    changed = False
    normalized_lines: set[int] = set()
    for item in document.unsupported_legacy_addresses:
        if item.line in normalized_lines or item.line > len(lines):
            continue
        raw_line = lines[item.line - 1]
        matched = _legacy_match(raw_line, binary=binary)
        if matched is None:
            continue
        match = matched[0]
        prefix = raw_line[: match.start()]
        try:
            directive_prefix = _canonical_directive_prefix(prefix)
        except SourceTraceMigrationError:
            continue
        lines[item.line - 1] = directive_prefix + raw_line[match.start() :]
        normalized_lines.add(item.line)
        changed = True
    if not changed:
        return document
    return parse_source_trace_text(
        "".join(lines),
        path=path,
        encoding=encoding,
    )


def _source_files(paths: Sequence[Path], *, repo_root: Path) -> tuple[Path, ...]:
    root = repo_root.resolve()
    files: set[Path] = set()
    for raw in paths:
        candidate = raw if raw.is_absolute() else root / raw
        resolved = candidate.resolve()
        try:
            resolved.relative_to(root)
        except ValueError as exc:
            raise SourceTraceMigrationError(
                f"migration scan path escapes repository root: {raw}"
            ) from exc
        if resolved.is_dir():
            files.update(
                item.resolve()
                for item in resolved.rglob("*")
                if item.is_file() and item.suffix.lower() in SOURCE_SUFFIXES
            )
        elif resolved.is_file() and resolved.suffix.lower() in SOURCE_SUFFIXES:
            files.add(resolved)
        else:
            raise SourceTraceMigrationError(
                f"migration scan path is not a supported source file/directory: {raw}"
            )
    return tuple(sorted(files, key=lambda item: item.as_posix().casefold()))


def build_migration_template(
    paths: Sequence[Path],
    *,
    repo_root: Path,
    artifact_index: SourceArtifactIndex,
) -> dict[str, Any]:
    """Build a deterministic, deliberately unapproved full-corpus review template."""

    occurrences: list[LegacyOccurrence] = []
    occurrence_details: dict[tuple[str, int], Any] = {}
    occurrence_standalone: dict[tuple[str, int], bool] = {}
    occurrence_comment_keys: dict[tuple[str, int], tuple[str, int, int]] = {}
    encodings: dict[str, dict[str, Any]] = {}
    for source_path in _source_files(paths, repo_root=repo_root):
        relative = source_path.relative_to(repo_root.resolve()).as_posix()
        decoded = decode_source_bytes(source_path.read_bytes(), path=relative)
        document = _migration_legacy_document(
            decoded.text,
            path=relative,
            encoding=decoded.encoding,
        )
        rows = tuple(
            LegacyOccurrence(
                relative,
                item.line,
                item.relation,
                normalize_artifact_id(item.artifact_id),
                item.description,
            )
            for item in document.legacy_artifacts
        )
        for item in document.legacy_artifacts:
            occurrence_details[(relative, item.line)] = item
            source_lines = decoded.text.splitlines(keepends=True)
            occurrence_standalone[(relative, item.line)] = (
                item.line <= len(source_lines)
                and _legacy_marker_is_standalone(source_lines[item.line - 1])
            )
        starts = _line_starts(decoded.text)
        spans = _comment_spans(decoded.text)
        for item in document.legacy_artifacts:
            line_start = starts[item.line - 1]
            line_end = (
                starts[item.line] if item.line < len(starts) else len(decoded.text)
            )
            span = next(
                (
                    candidate
                    for candidate in spans
                    if candidate[0] <= line_start < candidate[1]
                    or line_start <= candidate[0] < line_end
                ),
                None,
            )
            if span is None:
                raise SourceTraceMigrationError(
                    f"{relative}:{item.line}: legacy occurrence has no exact comment span"
                )
            occurrence_comment_keys[(relative, item.line)] = (
                relative,
                span[0],
                span[1],
            )
        occurrences.extend(rows)
        if rows:
            encodings[relative] = {
                "encoding": decoded.encoding,
                "newline": {
                    "\r\n": "crlf",
                    "\n": "lf",
                    "\r": "cr",
                    "": "none",
                }[decoded.newline],
                "bom": decoded.bom,
            }

    identity_counts: dict[str, int] = {}
    for item in occurrences:
        identity_counts[item.legacy_artifact_id] = (
            identity_counts.get(item.legacy_artifact_id, 0) + 1
        )
    comment_counts: dict[tuple[str, int, int], int] = {}
    for key in occurrence_comment_keys.values():
        comment_counts[key] = comment_counts.get(key, 0) + 1
    rows: list[dict[str, Any]] = []
    debts: list[dict[str, Any]] = []
    facts: list[dict[str, Any]] = []
    unresolved_legacy_claims: list[dict[str, str]] = []
    used_anchor_bases: set[str] = set()
    for item in sorted(
        occurrences,
        key=lambda row: (row.path.casefold(), row.line, row.legacy_artifact_id),
    ):
        target = artifact_index.resolve(item.legacy_artifact_id)
        detail = occurrence_details[item.key()]
        construct_kind = detail.construct.kind if detail.construct is not None else None
        construct_name = detail.construct.name if detail.construct is not None else None
        target_class = None
        target_trace_state = None
        if target is not None:
            if target.row.get("kind") == "provider-function":
                target_class = "provider"
            elif target.kind == "function":
                target_class = str(
                    target.row.get("pipeline_class", "unresolved")
                )
            else:
                target_class = "data"
            trace = target.row.get("source_traceability")
            target_trace_state = (
                trace.get("state") if isinstance(trace, Mapping) else None
            )
        direct_kind = (
            (target is not None and target.kind == "function" and construct_kind == "function")
            or (target is not None and target.kind == "data" and construct_kind == "data")
        )
        source_suffix = PurePosixPath(item.path).suffix.lower()
        direct_translation_unit = source_suffix in {".c", ".cc", ".cpp", ".cxx"}
        safe_authored = (
            target is not None
            and (
                (target.kind == "function" and target_class == "authored")
                or target.kind == "data"
            )
        )
        auto_resolved = (
            identity_counts[item.legacy_artifact_id] == 1
            and comment_counts[occurrence_comment_keys[item.key()]] == 1
            and item.relation == "defines"
            and detail.comment_style == "doxygen"
            and detail.attachment_status == "attached"
            and occurrence_standalone[item.key()]
            and direct_kind
            and direct_translation_unit
            and safe_authored
            and target.output_section is not None
        )
        if target is None:
            reason = "missing-artifact-identity"
            artifact_id = None
            binary, kind_hint, address = item.legacy_artifact_id.split(":", 2)
            if construct_kind in {"function", "data"}:
                kind_hint = construct_kind
            unresolved_legacy_claims.append(
                {
                    "binary": binary,
                    "kind_hint": kind_hint,
                    "address": address,
                    "reason_code": "missing-artifact-identity",
                    "source_path": item.path,
                }
            )
        elif auto_resolved:
            reason = None
            artifact_id = item.legacy_artifact_id
        elif identity_counts[item.legacy_artifact_id] > 1:
            reason = "duplicate-legacy-occurrence-review-required"
            artifact_id = item.legacy_artifact_id
        elif detail.attachment_status != "attached":
            reason = f"{detail.attachment_status}-review-required"
            artifact_id = item.legacy_artifact_id
        elif target_class in {"provider", "non-authored"}:
            reason = "provider-or-non-authored-review-required"
            artifact_id = item.legacy_artifact_id
        elif target_class == "authored-lifecycle":
            reason = "lifecycle-relation-review-required"
            artifact_id = item.legacy_artifact_id
        elif source_suffix in {".h", ".hh", ".hpp"}:
            reason = "header-translation-unit-review-required"
            artifact_id = item.legacy_artifact_id
        else:
            reason = "parent-review-required"
            artifact_id = item.legacy_artifact_id
        anchor_id = None
        if auto_resolved:
            source_base = (
                PurePosixPath(item.path).with_suffix("").as_posix()
                + "-"
                + str(construct_kind)
                + "-"
                + str(construct_name)
            )
            anchor_slug = re.sub(
                r"[^a-z0-9._-]+",
                "-",
                source_base.lower(),
            ).strip("-")
            if anchor_slug in used_anchor_bases:
                address = item.legacy_artifact_id.rsplit(":", 1)[-1]
                anchor_slug = f"{anchor_slug}-{address}"
            used_anchor_bases.add(anchor_slug)
            anchor_id = f"recoil:anchor:{anchor_slug}"
        row = {
            "path": item.path,
            "line": item.line,
            "expected_relation": item.relation,
            "relation": item.relation,
            "expected_legacy_artifact_id": item.legacy_artifact_id,
            "expected_description": item.description,
            "state": "resolved" if auto_resolved else "unresolved",
            "artifact_id": artifact_id,
            "anchor_id": anchor_id,
            "output_section": target.output_section if auto_resolved else None,
            "translation_unit": item.path if auto_resolved else None,
            "reason_code": reason,
            "reviewed": auto_resolved,
            "record_tracker_state": auto_resolved,
        }
        rows.append(row)
        facts.append(
            {
                "path": item.path,
                "line": item.line,
                "legacy_artifact_id": item.legacy_artifact_id,
                "attachment_status": detail.attachment_status,
                "comment_style": detail.comment_style,
                "standalone_semantic_line": occurrence_standalone[item.key()],
                "construct_kind": construct_kind,
                "construct_name": construct_name,
                "construct_line": (
                    detail.construct.line if detail.construct is not None else None
                ),
                "global_occurrence_count": identity_counts[item.legacy_artifact_id],
                "comment_legacy_row_count": comment_counts[
                    occurrence_comment_keys[item.key()]
                ],
                "tracker_resolution": "resolved" if target is not None else "missing",
                "tracker_physical_id": target.physical_id if target is not None else None,
                "tracker_kind": target.kind if target is not None else None,
                "tracker_class": target_class,
                "tracker_source_traceability_state": target_trace_state,
                "output_section": target.output_section if target is not None else None,
                "classification": (
                    "auto-resolved-unique-attached-direct"
                    if auto_resolved
                    else "parent-review-required"
                ),
            }
        )
        if not auto_resolved:
            debts.append(
                {
                    "reason_code": reason,
                    "path": item.path,
                    "legacy_artifact_id": item.legacy_artifact_id,
                    "review_required": True,
                }
            )
    return {
        "report_version": 1,
        "kind": "source-trace-migration-template",
        "schema": INVENTORY_SCHEMA,
        "topology_only": True,
        "acceptance_effect": "none",
        "reviewed": all(row["reviewed"] for row in rows),
        "rows": rows,
        "inventory": {
            "schema": INVENTORY_SCHEMA,
            "rows": deepcopy(rows),
        },
        "source_encodings": encodings,
        "occurrence_facts": facts,
        "unresolved_legacy_claims": sorted(
            {
                (
                    row["binary"],
                    row["kind_hint"],
                    row["address"],
                    row["reason_code"],
                    row["source_path"],
                ): row
                for row in unresolved_legacy_claims
            }.values(),
            key=lambda row: (
                row["binary"],
                row["kind_hint"],
                row["address"],
                row["source_path"],
            ),
        ),
        "debts": debts,
        "instructions": (
            "Every row requires parent review. Set reviewed=true; choose resolved, "
            "unresolved, or not-applicable; select canonical relation independently "
            "from expected_relation; and select exactly one record_tracker_state=true "
            "row per existing artifact. Occurrence-only duplicates remove legacy "
            "address prose without a second tracker update."
        ),
    }


def propose_source_trace_batch(
    inventory: MigrationInventory,
    *,
    repo_root: Path,
    artifact_index: SourceArtifactIndex,
    apply: bool = False,
) -> dict[str, Any]:
    """Validate all files before optionally applying comment-only source rewrites."""

    root = repo_root.resolve()
    source_paths = sorted({row.path for row in inventory.rows}, key=str.casefold)
    proposals: list[tuple[Path, MigrationProposal]] = []
    tracker_decisions: dict[str, Mapping[str, Any]] = {}
    for relative in source_paths:
        source_path = (root / relative).resolve()
        try:
            source_path.relative_to(root)
        except ValueError as exc:
            raise SourceTraceMigrationError(
                f"migration inventory path escapes repository root: {relative}"
            ) from exc
        raw = source_path.read_bytes()
        proposal = propose_source_trace_rewrite(
            raw,
            path=relative,
            inventory=inventory,
            artifact_index=artifact_index,
        )
        if not proposal.ready:
            raise SourceTraceMigrationError(
                f"{relative}: migration proposal is not ready"
            )
        for state in proposal.tracker_states:
            artifact_id = str(state["artifact_id"])
            if artifact_id in tracker_decisions:
                raise SourceTraceMigrationError(
                    f"artifact {artifact_id!r} has more than one tracker-state "
                    "decision across the migration batch"
                )
            tracker_decisions[artifact_id] = deepcopy(dict(state))
        proposals.append((source_path, proposal))

    if apply:
        # All source snapshots, canonical parsing, tracker joins, encodings, and
        # token streams have passed before the first write.
        for source_path, proposal in proposals:
            assert proposal.proposed_bytes is not None
            atomic_replace(source_path, proposal.proposed_bytes)

    return {
        "report_version": 1,
        "kind": "source-trace-migration-batch",
        "topology_only": True,
        "acceptance_effect": "none",
        "applied": apply,
        "file_count": len(proposals),
        "files": [
            proposal.to_dict(include_text=False) for _path, proposal in proposals
        ],
        "tracker_states": [
            tracker_decisions[key] for key in sorted(tracker_decisions)
        ],
        "debts": [
            asdict(debt)
            for _path, proposal in proposals
            for debt in proposal.debts
        ],
    }


def write_review_report(path: Path, report: Mapping[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = (
        json.dumps(report, ensure_ascii=False, indent=2, sort_keys=True) + "\n"
    ).encode("utf-8")
    atomic_replace(path, payload)


def build_tracker_replace_payload(
    paths: Sequence[Path],
    *,
    repo_root: Path,
    artifact_index: SourceArtifactIndex,
    migration_tracker_states: Sequence[Mapping[str, Any]] = (),
    header_translation_units: Mapping[str, Any] | None = None,
    unresolved_legacy_claims: Sequence[Mapping[str, Any]] = (),
) -> dict[str, Any]:
    """Join canonical source edges and reviewed migration states into one CAS payload."""

    root = repo_root.resolve()
    overrides = dict(header_translation_units or {})
    normalized_header_defaults: dict[str, str] = {}
    normalized_artifact_overrides: dict[tuple[str, str], str] = {}

    def normalize_translation_unit(raw_tu: Any, *, label: str) -> str:
        if not isinstance(raw_tu, str):
            raise SourceTraceMigrationError(f"{label} must be a translation-unit string")
        translation_unit = _normalized_path(raw_tu, label=label)
        if PurePosixPath(translation_unit).suffix.lower() not in {
            ".c",
            ".cc",
            ".cpp",
            ".cxx",
        }:
            raise SourceTraceMigrationError(
                f"header override translation unit is not a source file: "
                f"{translation_unit!r}"
            )
        if not (root / translation_unit).is_file():
            raise SourceTraceMigrationError(
                f"header override translation unit does not exist: {translation_unit!r}"
            )
        return translation_unit

    for raw_header, raw_value in overrides.items():
        header = _normalized_path(raw_header, label="header override path")
        if PurePosixPath(header).suffix.lower() not in {".h", ".hh", ".hpp"}:
            raise SourceTraceMigrationError(
                f"header override key is not a header path: {header!r}"
            )
        if not (root / header).is_file():
            raise SourceTraceMigrationError(
                f"header override path does not exist: {header!r}"
            )
        if isinstance(raw_value, str):
            normalized_header_defaults[header] = normalize_translation_unit(
                raw_value, label=f"header override {header!r}"
            )
            continue
        if not isinstance(raw_value, Mapping) or not raw_value:
            raise SourceTraceMigrationError(
                f"header override {header!r} must be a translation-unit string or "
                "a nonempty artifact-id-to-translation-unit object"
            )
        for raw_artifact_id, raw_tu in raw_value.items():
            if not isinstance(raw_artifact_id, str):
                raise SourceTraceMigrationError(
                    f"header override {header!r} artifact ids must be strings"
                )
            artifact_id = normalize_artifact_id(raw_artifact_id)
            if artifact_index.resolve(artifact_id) is None:
                raise SourceTraceMigrationError(
                    f"header override {header!r} has unknown artifact id "
                    f"{artifact_id!r}"
                )
            key = (header, artifact_id)
            if key in normalized_artifact_overrides:
                raise SourceTraceMigrationError(
                    f"duplicate header artifact override for {header!r} "
                    f"{artifact_id!r}"
                )
            normalized_artifact_overrides[key] = normalize_translation_unit(
                raw_tu,
                label=f"header override {header!r} artifact {artifact_id!r}",
            )

    documents = []
    for source_path in _source_files(paths, repo_root=root):
        relative = source_path.relative_to(root).as_posix()
        decoded = decode_source_bytes(source_path.read_bytes(), path=relative)
        document = parse_source_trace_text(
            decoded.text,
            path=relative,
            encoding=decoded.encoding,
        )
        findings = validate_source_trace(document, artifact_index, strict=False)
        if findings:
            first = findings[0]
            raise SourceTraceMigrationError(
                f"{first.path}:{first.line}: canonical source trace failed "
                f"{first.code}: {first.message}"
            )
        documents.append(document)
    cross_findings = merge_source_trace_documents(tuple(documents))
    if cross_findings:
        first = cross_findings[0]
        raise SourceTraceMigrationError(
            f"{first.path}:{first.line}: canonical source trace failed "
            f"{first.code}: {first.message}"
        )

    decisions: dict[str, dict[str, Any]] = {}
    used_artifact_overrides: set[tuple[str, str]] = set()
    for document in documents:
        suffix = PurePosixPath(document.path).suffix.lower()
        for artifact in document.artifacts:
            if suffix in {".h", ".hh", ".hpp"}:
                override_key = (document.path, artifact.artifact_id)
                translation_unit = normalized_artifact_overrides.get(override_key)
                if translation_unit is not None:
                    used_artifact_overrides.add(override_key)
                else:
                    translation_unit = normalized_header_defaults.get(document.path)
                if translation_unit is None:
                    raise SourceTraceMigrationError(
                        f"canonical header artifact {artifact.artifact_id!r} in "
                        f"{document.path!r} requires exactly one explicit "
                        "translation-unit override"
                    )
            else:
                translation_unit = document.path
            target = artifact_index.resolve(artifact.artifact_id)
            if target is None:
                raise SourceTraceMigrationError(
                    f"canonical artifact {artifact.artifact_id!r} disappeared from tracker"
                )
            if target.row.get("source_traceability") is not None:
                raise SourceTraceMigrationError(
                    f"canonical artifact {artifact.artifact_id!r} no longer has "
                    "expected_current=null"
                )
            if artifact.artifact_id in decisions:
                raise SourceTraceMigrationError(
                    f"duplicate tracker-state decision for {artifact.artifact_id!r}"
                )
            state = normalize_source_traceability(
                {
                    "state": "resolved",
                    "source_edges": [
                        {
                            "relation": artifact.relation,
                            "anchor_id": artifact.anchor_id,
                            "emission_context": {
                                "translation_unit": translation_unit,
                            },
                            "evidence_ids": [],
                        }
                    ],
                    "reason_code": None,
                }
            )
            decisions[artifact.artifact_id] = {
                "artifact_id": artifact.artifact_id,
                "expected_current": None,
                "source_traceability": state,
            }

    unused_artifact_overrides = sorted(
        set(normalized_artifact_overrides) - used_artifact_overrides
    )
    if unused_artifact_overrides:
        header, artifact_id = unused_artifact_overrides[0]
        raise SourceTraceMigrationError(
            f"stale header artifact override for {header!r} {artifact_id!r}: "
            "no matching canonical artifact was found"
        )

    for raw_state in migration_tracker_states:
        if not isinstance(raw_state, Mapping) or set(raw_state) != {
            "artifact_id",
            "source_traceability",
        }:
            raise SourceTraceMigrationError(
                "migration tracker_states rows require exactly artifact_id and "
                "source_traceability"
            )
        artifact_id = normalize_artifact_id(str(raw_state["artifact_id"]))
        target = artifact_index.resolve(artifact_id)
        if target is None:
            raise SourceTraceMigrationError(
                f"migration tracker-state artifact {artifact_id!r} is unknown"
            )
        if target.row.get("source_traceability") is not None:
            raise SourceTraceMigrationError(
                f"migration artifact {artifact_id!r} no longer has expected_current=null"
            )
        if artifact_id in decisions:
            raise SourceTraceMigrationError(
                f"duplicate tracker-state decision for {artifact_id!r}"
            )
        decisions[artifact_id] = {
            "artifact_id": artifact_id,
            "expected_current": None,
            "source_traceability": normalize_source_traceability(
                raw_state["source_traceability"]
            ),
        }

    claims = (
        normalize_unresolved_legacy_claims(list(unresolved_legacy_claims))
        if unresolved_legacy_claims
        else []
    )
    payload: dict[str, Any] = {
        "operation": "replace-batch",
        "parent_reviewed": True,
        "updates": [decisions[key] for key in sorted(decisions)],
    }
    if claims:
        payload["unresolved_legacy_claims"] = claims
    return payload


def _load_json_mapping(path: Path, *, label: str) -> Mapping[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise SourceTraceMigrationError(f"{path}: invalid {label} JSON: {exc}") from exc
    if not isinstance(value, Mapping):
        raise SourceTraceMigrationError(f"{path}: {label} must be an object")
    return value


def review_conservative_template(
    template: Mapping[str, Any],
    *,
    current_template: Mapping[str, Any],
    canonical_artifact_ids: set[str],
    parent_reviewed: bool,
) -> dict[str, Any]:
    """Apply the parent-approved conservative decision policy to an exact template."""

    if parent_reviewed is not True:
        raise SourceTraceMigrationError(
            "conservative review output requires explicit parent_reviewed=true"
        )
    if template.get("kind") != "source-trace-migration-template":
        raise SourceTraceMigrationError("input is not a source-trace migration template")
    raw_rows = template.get("rows")
    raw_facts = template.get("occurrence_facts")
    if not isinstance(raw_rows, list) or not isinstance(raw_facts, list):
        raise SourceTraceMigrationError(
            "migration template requires rows and occurrence_facts arrays"
        )
    current_rows = current_template.get("rows")
    current_facts = current_template.get("occurrence_facts")
    if not isinstance(current_rows, list) or not isinstance(current_facts, list):
        raise SourceTraceMigrationError("current migration template is malformed")
    snapshot_fields = (
        "path",
        "line",
        "expected_relation",
        "expected_legacy_artifact_id",
        "expected_description",
    )
    expected_snapshots = [
        tuple(row.get(field) for field in snapshot_fields)
        for row in raw_rows
        if isinstance(row, Mapping)
    ]
    observed_snapshots = [
        tuple(row.get(field) for field in snapshot_fields)
        for row in current_rows
        if isinstance(row, Mapping)
    ]
    if expected_snapshots != observed_snapshots:
        raise SourceTraceMigrationError(
            "migration template source snapshots are stale; regenerate before review"
        )
    fact_keys = (
        "path",
        "line",
        "legacy_artifact_id",
        "attachment_status",
        "comment_style",
        "standalone_semantic_line",
        "construct_kind",
        "construct_name",
        "global_occurrence_count",
        "comment_legacy_row_count",
        "tracker_resolution",
        "tracker_physical_id",
        "tracker_kind",
        "tracker_class",
        "tracker_source_traceability_state",
        "output_section",
    )
    expected_facts = [
        tuple(row.get(field) for field in fact_keys)
        for row in raw_facts
        if isinstance(row, Mapping)
    ]
    observed_facts = [
        tuple(row.get(field) for field in fact_keys)
        for row in current_facts
        if isinstance(row, Mapping)
    ]
    if expected_facts != observed_facts:
        raise SourceTraceMigrationError(
            "migration template attachment/tracker facts are stale; regenerate before review"
        )

    facts_by_key = {
        (str(row["path"]), int(row["line"])): row
        for row in raw_facts
        if isinstance(row, Mapping)
    }
    groups: dict[str, list[int]] = {}
    for index, row in enumerate(raw_rows):
        if not isinstance(row, Mapping):
            raise SourceTraceMigrationError(f"template row {index} must be an object")
        artifact_id = row.get("artifact_id")
        if isinstance(artifact_id, str):
            groups.setdefault(artifact_id, []).append(index)

    reviewed_rows = [deepcopy(dict(row)) for row in raw_rows]
    decisions_by_reason: dict[str, int] = {}
    decision_counts = {
        "resolved": 0,
        "unresolved": 0,
        "not-applicable": 0,
        "occurrence-only": 0,
        "missing-artifact": 0,
    }

    def set_occurrence_only(row: dict[str, Any], reason: str) -> None:
        row.update(
            {
                "state": "unresolved",
                "anchor_id": None,
                "output_section": None,
                "translation_unit": None,
                "reason_code": reason,
                "reviewed": True,
                "record_tracker_state": False,
            }
        )
        decisions_by_reason[reason] = decisions_by_reason.get(reason, 0) + 1
        decision_counts["occurrence-only"] += 1

    # Missing artifacts never acquire a tracker decision or fabricated identity.
    for row in reviewed_rows:
        if row.get("artifact_id") is None:
            set_occurrence_only(row, "missing-artifact-identity")
            decision_counts["missing-artifact"] += 1

    for artifact_id in sorted(groups):
        indexes = groups[artifact_id]
        if artifact_id in canonical_artifact_ids:
            for index in indexes:
                set_occurrence_only(
                    reviewed_rows[index],
                    "redundant-legacy-after-canonical-source",
                )
            continue
        auto_safe = [
            index
            for index in indexes
            if raw_rows[index].get("state") == "resolved"
            and raw_rows[index].get("reviewed") is True
            and raw_rows[index].get("record_tracker_state") is True
        ]
        if len(auto_safe) == 1 and len(indexes) == 1:
            row = reviewed_rows[auto_safe[0]]
            row["reviewed"] = True
            row["record_tracker_state"] = True
            decision_counts["resolved"] += 1
            continue

        primary = indexes[0]
        fact = facts_by_key[
            (
                str(reviewed_rows[primary]["path"]),
                int(reviewed_rows[primary]["line"]),
            )
        ]
        tracker_class = str(fact.get("tracker_class", "unresolved"))
        if tracker_class in {"provider", "non-authored"}:
            state = "not-applicable"
            reason = "provider-boundary"
        elif str(fact.get("attachment_status")) != "attached":
            state = "unresolved"
            reason = "detached-or-unsupported-source-topology"
        elif fact.get("standalone_semantic_line") is not True:
            state = "unresolved"
            reason = "embedded-legacy-prose"
        elif tracker_class == "authored-lifecycle":
            state = "unresolved"
            reason = "lifecycle-relation-unresolved"
        elif PurePosixPath(str(fact.get("path"))).suffix.lower() in {
            ".h",
            ".hh",
            ".hpp",
        }:
            state = "unresolved"
            reason = "header-translation-unit-unresolved"
        elif len(indexes) > 1 or int(fact.get("comment_legacy_row_count", 0)) > 1:
            state = "unresolved"
            reason = "multiple-legacy-occurrences"
        else:
            state = "unresolved"
            reason = "source-topology-unresolved"
        reviewed_rows[primary].update(
            {
                "state": state,
                "anchor_id": None,
                "output_section": None,
                "translation_unit": None,
                "reason_code": reason,
                "reviewed": True,
                "record_tracker_state": True,
            }
        )
        decisions_by_reason[reason] = decisions_by_reason.get(reason, 0) + 1
        decision_counts[state] += 1
        for index in indexes[1:]:
            set_occurrence_only(
                reviewed_rows[index],
                "redundant-legacy-occurrence",
            )

    reviewed_inventory = {
        "schema": INVENTORY_SCHEMA,
        "rows": reviewed_rows,
    }
    # The normal parser is the final exact-shape/state policy gate.
    parse_migration_inventory(reviewed_inventory)
    return {
        "report_version": 1,
        "kind": "source-trace-conservative-review",
        "topology_only": True,
        "acceptance_effect": "none",
        "parent_reviewed": True,
        "inventory": reviewed_inventory,
        "unresolved_legacy_claims": deepcopy(
            list(template.get("unresolved_legacy_claims", []))
        ),
        "decision_counts": decision_counts,
        "reason_counts": {
            key: decisions_by_reason[key] for key in sorted(decisions_by_reason)
        },
    }


def canonical_artifact_ids_for_paths(
    paths: Sequence[Path],
    *,
    repo_root: Path,
    artifact_index: SourceArtifactIndex,
) -> set[str]:
    result: set[str] = set()
    documents = []
    for source_path in _source_files(paths, repo_root=repo_root):
        relative = source_path.relative_to(repo_root.resolve()).as_posix()
        decoded = decode_source_bytes(source_path.read_bytes(), path=relative)
        document = parse_source_trace_text(
            decoded.text, path=relative, encoding=decoded.encoding
        )
        findings = validate_source_trace(document, artifact_index, strict=False)
        if findings:
            first = findings[0]
            raise SourceTraceMigrationError(
                f"{first.path}:{first.line}: canonical source trace failed "
                f"{first.code}: {first.message}"
            )
        documents.append(document)
        result.update(item.artifact_id for item in document.artifacts)
    cross = merge_source_trace_documents(tuple(documents))
    if cross:
        first = cross[0]
        raise SourceTraceMigrationError(
            f"{first.path}:{first.line}: canonical source trace failed "
            f"{first.code}: {first.message}"
        )
    return result


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Inventory or propose comment-only Source Traceability v1 migration."
    )
    subparsers = parser.add_subparsers(dest="operation", required=True)
    for name in ("inventory", "propose"):
        child = subparsers.add_parser(name)
        child.add_argument("--path", type=Path, required=True)
        child.add_argument("--progress", type=Path, required=True)
        if name == "propose":
            payload = child.add_mutually_exclusive_group(required=True)
            payload.add_argument("--payload-json")
            payload.add_argument("--payload-file", type=Path)
            child.add_argument("--include-text", action="store_true")
        child.add_argument("--json", action="store_true")
    template = subparsers.add_parser("template")
    template.add_argument("--path", type=Path, action="append", required=True)
    template.add_argument("--repo-root", type=Path, default=Path("."))
    template.add_argument("--progress", type=Path, required=True)
    template.add_argument("--output", type=Path)
    template.add_argument("--json", action="store_true")
    batch = subparsers.add_parser("batch-propose")
    batch.add_argument("--repo-root", type=Path, default=Path("."))
    batch.add_argument("--progress", type=Path, required=True)
    payload = batch.add_mutually_exclusive_group(required=True)
    payload.add_argument("--payload-json")
    payload.add_argument("--payload-file", type=Path)
    batch.add_argument("--output", type=Path)
    batch.add_argument("--apply", action="store_true")
    batch.add_argument("--json", action="store_true")
    tracker_payload = subparsers.add_parser("tracker-payload")
    tracker_payload.add_argument("--path", type=Path, action="append", required=True)
    tracker_payload.add_argument("--repo-root", type=Path, default=Path("."))
    tracker_payload.add_argument("--progress", type=Path, required=True)
    tracker_payload.add_argument("--migration-report", type=Path)
    tracker_payload.add_argument("--migration-template", type=Path)
    tracker_payload.add_argument("--header-overrides", type=Path)
    tracker_payload.add_argument("--output", type=Path, required=True)
    tracker_payload.add_argument("--json", action="store_true")
    review = subparsers.add_parser("review-conservative")
    review.add_argument("--template", type=Path, required=True)
    review.add_argument("--repo-root", type=Path, default=Path("."))
    review.add_argument("--progress", type=Path, required=True)
    review.add_argument("--output", type=Path, required=True)
    review.add_argument("--parent-reviewed", action="store_true", required=True)
    review.add_argument("--json", action="store_true")
    return parser


def run(args: argparse.Namespace) -> dict[str, Any]:
    index = load_artifact_rows(args.progress)
    if args.operation == "template":
        report = build_migration_template(
            args.path,
            repo_root=args.repo_root,
            artifact_index=index,
        )
        if args.output is not None:
            write_review_report(args.output, report)
            report = {**report, "output": args.output.as_posix()}
        return report
    if args.operation == "batch-propose":
        migration_inventory = load_migration_inventory(
            payload_json=args.payload_json,
            payload_file=args.payload_file,
        )
        report = propose_source_trace_batch(
            migration_inventory,
            repo_root=args.repo_root,
            artifact_index=index,
            apply=bool(args.apply),
        )
        if args.output is not None:
            write_review_report(args.output, report)
            report = {**report, "output": args.output.as_posix()}
        return report
    if args.operation == "tracker-payload":
        tracker_states: Sequence[Mapping[str, Any]] = ()
        claims: Sequence[Mapping[str, Any]] = ()
        overrides: Mapping[str, Any] = {}
        if args.migration_report is not None:
            report = _load_json_mapping(
                args.migration_report, label="migration report"
            )
            raw_states = report.get("tracker_states", [])
            if not isinstance(raw_states, list):
                raise SourceTraceMigrationError(
                    "migration report tracker_states must be an array"
                )
            tracker_states = raw_states
        if args.migration_template is not None:
            template = _load_json_mapping(
                args.migration_template, label="migration template"
            )
            raw_claims = template.get("unresolved_legacy_claims", [])
            if not isinstance(raw_claims, list):
                raise SourceTraceMigrationError(
                    "migration template unresolved_legacy_claims must be an array"
                )
            claims = raw_claims
        if args.header_overrides is not None:
            raw_overrides = _load_json_mapping(
                args.header_overrides, label="header overrides"
            )
            if not all(isinstance(key, str) for key in raw_overrides):
                raise SourceTraceMigrationError(
                    "header override keys must be source header strings"
                )
            overrides = dict(raw_overrides)
        payload = build_tracker_replace_payload(
            args.path,
            repo_root=args.repo_root,
            artifact_index=index,
            migration_tracker_states=tracker_states,
            header_translation_units=overrides,
            unresolved_legacy_claims=claims,
        )
        write_review_report(args.output, payload)
        return {
            "report_version": 1,
            "kind": "source-trace-tracker-payload",
            "topology_only": True,
            "acceptance_effect": "none",
            "output": args.output.as_posix(),
            "update_count": len(payload["updates"]),
            "unresolved_legacy_claim_count": len(
                payload.get("unresolved_legacy_claims", [])
            ),
            "payload": payload,
        }
    if args.operation == "review-conservative":
        template = _load_json_mapping(
            args.template, label="migration template"
        )
        raw_rows = template.get("rows")
        if not isinstance(raw_rows, list):
            raise SourceTraceMigrationError(
                "migration template rows must be an array"
            )
        source_paths = sorted(
            {
                str(row.get("path"))
                for row in raw_rows
                if isinstance(row, Mapping) and isinstance(row.get("path"), str)
            },
            key=str.casefold,
        )
        current = build_migration_template(
            [Path(path) for path in source_paths],
            repo_root=args.repo_root,
            artifact_index=index,
        )
        canonical_scan_roots = sorted(
            {
                Path(path).parts[0]
                for path in source_paths
                if Path(path).parts
            },
            key=str.casefold,
        )
        canonical_ids = canonical_artifact_ids_for_paths(
            [Path(path) for path in canonical_scan_roots],
            repo_root=args.repo_root,
            artifact_index=index,
        )
        report = review_conservative_template(
            template,
            current_template=current,
            canonical_artifact_ids=canonical_ids,
            parent_reviewed=bool(args.parent_reviewed),
        )
        write_review_report(args.output, report)
        return {**report, "output": args.output.as_posix()}
    raw = args.path.read_bytes()
    decoded = decode_source_bytes(raw, path=args.path.as_posix())
    if args.operation == "inventory":
        return inventory_legacy_source(
            decoded.text,
            path=args.path.as_posix(),
            artifact_index=index,
        )
    inventory = load_migration_inventory(
        payload_json=args.payload_json,
        payload_file=args.payload_file,
    )
    return propose_source_trace_rewrite(
        raw,
        path=args.path.as_posix(),
        inventory=inventory,
        artifact_index=index,
    ).to_dict(include_text=args.include_text)


def main(argv: Sequence[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    try:
        report = run(args)
    except (OSError, ValueError, SourceTraceMigrationError) as exc:
        print(f"source trace migration error: {exc}", file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps(report, indent=2, sort_keys=True))
    else:
        print(json.dumps(report, indent=2, sort_keys=True))
    if report.get("kind") == "source-trace-migration-proposal":
        return 0 if report.get("ready") else 1
    if report.get("kind") in {
        "source-trace-migration-template",
        "source-trace-migration-batch",
    }:
        return 0
    return 0 if not report.get("debts") else 1


if __name__ == "__main__":
    raise SystemExit(main())
