"""Recoil call-contract listing evidence and checks."""

from __future__ import annotations

import io
import os
import re
import time
from contextvars import ContextVar
from dataclasses import dataclass, replace
from pathlib import Path
from typing import Any, Mapping

from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import targets as _cc_targets
from _recoil.commands.asm_verify import Instruction, parse_instruction_record
from _recoil.commands.asm_verify import parse_assembly as _parse_assembly


def _matches_compiler_literal(item, data: bytes) -> bool:
    """Recognize exact local literal contents without freezing COFF placement.

    Callers must still require unique selection and prove their complete
    relocation reference to the selected symbol. Section ordinal and pool offset
    are candidate layout, not a call-contract identity or linked-layout proof.
    """
    from _recoil.commands.asm_verify import IMAGE_SYM_CLASS_STATIC
    return (
        re.fullmatch(r"\$T[0-9]+", item.name) is not None
        and item.section_name == ".rdata"
        and item.section_number > 0
        and item.value >= 0
        and item.symbol_type == 0
        and item.storage_class == IMAGE_SYM_CLASS_STATIC
        and item.aux_count == 0
        and item.data == data
    )


def _exact_cod_wrapped_mov_source_line(
    first_line: str,
    continuation_line: str,
    instruction: Instruction,
) -> str:
    """Carry one exact VC5 split disp32-MOV offset onto its instruction.

    VC5 ``/FAcs`` prints the first five bytes of a six-byte ``8B /r disp32``
    instruction on an offset-bearing row, then the final byte and mnemonic on
    the immediately adjacent continuation.  The shared assembly parser joins
    the bytes but normally retains only the continuation as ``source_line``.
    Preserve the first row's offset for an absolute relocation placeholder or
    a register-relative disp32 load without SIB. Every malformed, multi-row,
    non-adjacent, or non-MOV
    continuation remains addressless and therefore fails closed downstream.
    """
    first_code = first_line.split(";", 1)[0].strip()
    continuation_code = continuation_line.split(";", 1)[0].strip()
    first_parts = first_code.split()
    continuation_parts = continuation_code.split()
    if (
        len(first_parts) != 6
        or re.fullmatch(r"[0-9A-Fa-f]{5}", first_parts[0]) is None
        or any(
            re.fullmatch(r"[0-9A-Fa-f]{2}", item) is None
            for item in first_parts[1:]
        )
        or len(continuation_parts) < 2
        or re.fullmatch(r"[0-9A-Fa-f]{2}", continuation_parts[0]) is None
        or continuation_parts[1].casefold() != "mov"
    ):
        return ""
    body = tuple(
        item.casefold()
        for item in (*first_parts[1:], continuation_parts[0])
    )
    try:
        modrm = int(body[1], 16)
    except (IndexError, ValueError):
        return ""
    seh_head = (
        body == ("64", "a1", "00", "00", "00", "00")
        and re.fullmatch(r"mov\s+eax\s*,\s*dword\s+(?:ptr\s+)?fs:__except_list",
                         instruction.raw_text, re.IGNORECASE) is not None
    )
    if (
        tuple(item.casefold() for item in instruction.bytes) != body
        or len(body) != 6
        or not (seh_head or body[0] == "8b" and (
            (modrm >> 6 == 0 and modrm & 0x07 == 0x05
                and body[2:] == ("00", "00", "00", "00"))
            or (modrm >> 6 == 2 and modrm & 0x07 != 4)
        ))
        or not instruction.raw_text
        or instruction.raw_text.split(None, 1)[0].casefold() != "mov"
    ):
        return ""
    return f"{first_parts[0]} {continuation_line.lstrip()}"


def _exact_cod_wrapped_terminal_invocation_source_line(
    first_line: str,
    continuation_line: str,
    following_line: str,
    instruction: Instruction,
) -> str:
    """Retain one exact terminal VC5 five-plus-one ``FF 25`` split.

    Unlike the wrapped-MOV adapter above, this deliberately does not publish
    an instruction coordinate.  It only carries the immediately preceding
    coordinate row into the addressless continuation so the candidate
    call-site closer can require the complete COFF body, section, relocation,
    symbol population, and caller extent before accepting the coordinate.
    """
    first_code = first_line.split(";", 1)[0].strip()
    continuation_code = continuation_line.split(";", 1)[0].strip()
    following_code = following_line.split(";", 1)[0].strip()
    first_parts = first_code.split()
    continuation_parts = continuation_code.split()
    if (
        len(first_parts) != 6
        or re.fullmatch(r"[0-9A-Fa-f]{5}", first_parts[0]) is None
        or tuple(item.casefold() for item in first_parts[1:])
        != ("ff", "25", "00", "00", "00")
        or len(continuation_parts) != 5
        or continuation_parts[0].casefold() != "00"
        or continuation_parts[1].casefold() != "jmp"
        or continuation_parts[2].casefold() != "dword"
        or continuation_parts[3].casefold() != "ptr"
        or re.fullmatch(
            r"[A-Za-z_?$@][A-Za-z0-9_?$@]*",
            continuation_parts[4],
        )
        is None
        or re.fullmatch(r"\S+\s+ENDP", following_code, re.IGNORECASE)
        is None
    ):
        return ""
    body = tuple(
        item.casefold()
        for item in (*first_parts[1:], continuation_parts[0])
    )
    if (
        tuple(item.casefold() for item in instruction.bytes) != body
        or body != ("ff", "25", "00", "00", "00", "00")
        or _cc_cfg._instruction_mnemonic(instruction) != "jmp"
        or _cc_cfg._instruction_operand(instruction).strip().casefold()
        != f"dword {continuation_parts[4]}".casefold()
    ):
        return ""
    return (
        continuation_line.rstrip("\r\n")
        + _cc_catalog._COD_TERMINAL_WRAP_MARKER
        + first_code
    )


def _cod_cpu_rdtsc_lines(lines: list[str]) -> list[str]:
    """Decode the CPU probe's two literal DB rows without swallowing DB as hex.

    Only the exact contiguous 0F 31 encoding becomes an RDTSC instruction.
    Other directives remain data and cannot establish register-flow facts.
    Source policy independently owns the address-scoped assembly exception.
    """
    result = list(lines)
    code_lines = [(index, line.split(";", 1)[0].strip()) for index, line in enumerate(lines)
                  if line.split(";", 1)[0].strip()]
    pattern = re.compile(r"([0-9a-f]{5})\s+([0-9a-f]{2})\s+DB\s+(-?[0-9]+)", re.I)
    for (first, a), (second, b), (_third, c) in zip(code_lines, code_lines[1:], code_lines[2:]):
        left, right = pattern.fullmatch(a), pattern.fullmatch(b)
        following = re.match(r"([0-9a-f]{5})\s", c, re.I)
        if (left and right and following and left[2].lower() == "0f" and right[2] == "31"
                and int(left[3]) == 15 and int(right[3]) == 49
                and int(right[1], 16) == int(left[1], 16) + 1
                and int(following[1], 16) == int(left[1], 16) + 2):
            result[first] = f"{left[1]} 0f 31 rdtsc"
            result[second] = ""
    return result


def parse_assembly(text: str, *, source: str) -> list[Instruction]:
    """Parse assembly while retaining exact VC5 wrapped-MOV start offsets."""
    if source != "cod":
        return _parse_assembly(text, source=source)

    result: list[Instruction] = []
    restored_source_lines: dict[int, str] = {}
    pending_cod_bytes: list[str] = []
    pending_first_line = ""
    pending_line_index: int | None = None
    source_lines = _cod_cpu_rdtsc_lines(text.splitlines())
    for line_index, line in enumerate(source_lines):
        directive = re.fullmatch(r"\s*([0-9a-f]{5})\s+([0-9a-f]{2})\s+DB\s+(-?[0-9]+)\s*", line.split(";", 1)[0], re.I)
        if directive:
            if pending_cod_bytes or int(directive[2], 16) != int(directive[3]) & 0xff:
                raise ValueError("COD byte directive conflicts with its exact byte population")
            result.append(Instruction(text=f"db {directive[3]}", raw_text=f"DB {directive[3]}",
                bytes=(directive[2].lower(),), source_line=line))
            pending_first_line = ""
            pending_line_index = None
            continue
        had_pending = bool(pending_cod_bytes)
        parsed = parse_instruction_record(
            line,
            source=source,
            pending_cod_bytes=pending_cod_bytes,
        )
        if parsed is not None:
            if (
                had_pending
                and pending_first_line
                and pending_line_index == line_index - 1
            ):
                source_line = _exact_cod_wrapped_mov_source_line(
                    pending_first_line,
                    line,
                    parsed,
                )
                if not source_line and line_index + 1 < len(source_lines):
                    source_line = (
                        _exact_cod_wrapped_terminal_invocation_source_line(
                            pending_first_line,
                            line,
                            source_lines[line_index + 1],
                            parsed,
                        )
                    )
                if source_line:
                    source_parts = source_line.strip().split()
                    if (
                        source_parts
                        and re.fullmatch(
                            r"[0-9A-Fa-f]{5}", source_parts[0]
                        )
                    ):
                        restored_source_lines[len(result)] = (
                            parsed.source_line
                        )
                    parsed = replace(parsed, source_line=source_line)
            result.append(parsed)
            pending_first_line = ""
            pending_line_index = None
            continue
        if not had_pending and pending_cod_bytes:
            pending_first_line = line.rstrip("\r\n")
            pending_line_index = line_index
        elif had_pending:
            # The shared parser may accumulate across additional listing rows,
            # but only one immediately adjacent five-plus-one split is exact
            # enough to publish source-offset provenance here.
            pending_first_line = ""
            pending_line_index = None
        elif not pending_cod_bytes:
            pending_first_line = ""
            pending_line_index = None
    offset_counts: dict[str, int] = {}
    for instruction in result:
        parts = instruction.source_line.strip().split()
        if parts and re.fullmatch(r"[0-9A-Fa-f]{5}", parts[0]):
            offset = parts[0].casefold()
            offset_counts[offset] = offset_counts.get(offset, 0) + 1
    for instruction_index, original_source_line in restored_source_lines.items():
        parts = result[instruction_index].source_line.strip().split()
        if parts and offset_counts.get(parts[0].casefold(), 0) != 1:
            result[instruction_index] = replace(
                result[instruction_index],
                source_line=original_source_line,
            )
    return result


def _vc5_compiler_normalized_cod_proc_symbol(value: str) -> str | None:
    """Return the one reviewed VC5 anonymous-namespace PROC spelling.

    VC5 preserves the anonymous/TU-local percent marker in the COFF symbol,
    PUBLIC directive, and listing comments, but can drop only that marker from
    the PROC/ENDP label.  The absolute-path form is a reviewed VC5 spelling
    difference.  Retain the already-reviewed synthetic TU-order spelling, but
    do not normalize any other decorated name or more than one marker.
    """

    if value.count("?%") != 1:
        return None
    absolute_scope = re.search(
        r"\?%(?P<source>[A-Za-z]:[\\/][^@\r\n]*"
        r"\.(?:c|cc|cpp|cxx)[0-9]+)(?=@)",
        value,
        re.IGNORECASE,
    )
    if absolute_scope is not None:
        marker = absolute_scope.start()
        return value[: marker + 1] + value[marker + 2 :]
    if "?%_tu_order\\" in value:
        return value.replace("?%_tu_order\\", "?_tu_order\\", 1)
    return None


@dataclass(frozen=True)
class _CodProcedureRecord:
    """One exact PROC body and the control record that terminated it."""

    start_line_index: int
    ordinary_symbol: str
    path_symbol: str
    body_lines: tuple[str, ...]
    terminator_kind: str
    terminator_ordinary_symbol: str
    terminator_path_symbol: str
    terminator_line_index: int


@dataclass(frozen=True)
class _CodListingIndex:
    """One race-checked immutable COD read with exact procedure indexes."""

    path: Path
    text_ignore: str
    text_replace: str
    lines: tuple[str, ...]
    records_by_ordinary_symbol: Mapping[str, tuple[_CodProcedureRecord, ...]]
    records_by_path_symbol: Mapping[str, tuple[_CodProcedureRecord, ...]]
    endp_path_line_indexes: Mapping[str, tuple[int, ...]]


class _CodListingInvocationIndex:
    """Invocation-local COD reads and exact procedure lookups.

    This cache is owned by one lazy candidate session, is never serialized,
    and is discarded when the live verifier returns.  Each physical path is
    read and indexed once while every lookup keeps the established ordinary or
    reviewed anonymous-symbol pairing rules.
    """

    def __init__(self) -> None:
        self._files: dict[Path, _CodListingIndex] = {}
        self._procedure_results: dict[
            tuple[Path, str, bool], tuple[bool, tuple[str, ...] | str]
        ] = {}
        self.file_read_count = 0
        self.file_index_count = 0
        self.procedure_lookup_count = 0
        self.procedure_cache_hit_count = 0
        self.file_index_ms = 0.0
        self.procedure_lookup_ms = 0.0

    @staticmethod
    def _stat_identity(stat_result: os.stat_result) -> tuple[int, int, int, int, int]:
        return (
            int(stat_result.st_dev),
            int(stat_result.st_ino),
            int(stat_result.st_size),
            int(stat_result.st_mtime_ns),
            int(stat_result.st_ctime_ns),
        )

    @staticmethod
    def _decode(raw: bytes, *, errors: str) -> str:
        # Match Path.read_text/open's universal-newline behavior while retaining
        # one raw file read for both established decode error policies.
        with io.TextIOWrapper(
            io.BytesIO(raw),
            encoding="utf-8",
            errors=errors,
            newline=None,
        ) as stream:
            return stream.read()

    @staticmethod
    def _build_index(path: Path, raw: bytes) -> _CodListingIndex:
        text_ignore = _CodListingInvocationIndex._decode(raw, errors="ignore")
        text_replace = _CodListingInvocationIndex._decode(raw, errors="replace")
        lines = tuple(text_ignore.splitlines())
        path_proc_re = re.compile(
            r"^\s*(\S(?:.*?\S)?)\s+PROC\b", re.IGNORECASE
        )
        path_endp_re = re.compile(
            r"^\s*(\S(?:.*?\S)?)\s+ENDP\b", re.IGNORECASE
        )
        segment_re = re.compile(r"^\s*(\S+)\s+SEGMENT\b", re.IGNORECASE)
        segment_end_re = re.compile(r"^\s*(\S+)\s+ENDS\b", re.IGNORECASE)
        def scan(
            proc_re: re.Pattern[str],
            endp_re: re.Pattern[str],
        ) -> tuple[list[_CodProcedureRecord], dict[str, list[int]]]:
            records: list[_CodProcedureRecord] = []
            endp_line_indexes: dict[str, list[int]] = {}
            current_segment: str | None = None
            active_start = -1
            active_symbol = ""
            active_segment: str | None = None
            active_segment_closed = False
            active_body: list[str] = []

            def finish(
                *,
                kind: str,
                symbol: str = "",
                line_index: int,
            ) -> None:
                nonlocal active_start
                nonlocal active_symbol
                nonlocal active_segment
                nonlocal active_segment_closed
                nonlocal active_body
                if active_start < 0:
                    return
                records.append(
                    _CodProcedureRecord(
                        start_line_index=active_start,
                        ordinary_symbol=active_symbol,
                        path_symbol=active_symbol,
                        body_lines=tuple(active_body),
                        terminator_kind=kind,
                        terminator_ordinary_symbol=symbol,
                        terminator_path_symbol=symbol,
                        terminator_line_index=line_index,
                    )
                )
                active_start = -1
                active_symbol = ""
                active_segment = None
                active_segment_closed = False
                active_body = []

            for line_index, line in enumerate(lines):
                segment = segment_re.match(line)
                if segment:
                    current_segment = segment.group(1)
                    continue
                proc = proc_re.match(line)
                if proc:
                    symbol = proc.group(1)
                    finish(kind="proc", symbol=symbol, line_index=line_index)
                    active_start = line_index
                    active_symbol = symbol
                    active_segment = current_segment
                    active_segment_closed = False
                    active_body = []
                    continue
                endp = endp_re.match(line)
                if endp:
                    symbol = endp.group(1)
                    endp_line_indexes.setdefault(symbol, []).append(line_index)
                    finish(kind="endp", symbol=symbol, line_index=line_index)
                    continue
                segment_end = segment_end_re.match(line)
                if segment_end:
                    if (
                        active_start >= 0
                        and active_segment is not None
                        and segment_end.group(1) == active_segment
                    ):
                        active_segment_closed = True
                    if current_segment == segment_end.group(1):
                        current_segment = None
                    continue
                if active_start >= 0 and not active_segment_closed:
                    active_body.append(line)
            finish(kind="eof", line_index=len(lines))
            return records, endp_line_indexes

        ordinary_records, _ordinary_endps = scan(_cc_catalog.PROC_RE, _cc_catalog.ENDP_RE)
        path_records, endp_path_line_indexes = scan(path_proc_re, path_endp_re)
        ordinary: dict[str, list[_CodProcedureRecord]] = {}
        path_symbols: dict[str, list[_CodProcedureRecord]] = {}
        for record in ordinary_records:
            ordinary.setdefault(record.ordinary_symbol, []).append(record)
        for record in path_records:
            path_symbols.setdefault(record.path_symbol, []).append(record)
        return _CodListingIndex(
            path=path,
            text_ignore=text_ignore,
            text_replace=text_replace,
            lines=lines,
            records_by_ordinary_symbol={
                key: tuple(value) for key, value in ordinary.items()
            },
            records_by_path_symbol={
                key: tuple(value) for key, value in path_symbols.items()
            },
            endp_path_line_indexes={
                key: tuple(value) for key, value in endp_path_line_indexes.items()
            },
        )

    def _file(self, cod_path: Path) -> _CodListingIndex:
        path = cod_path.resolve()
        cached = self._files.get(path)
        if cached is not None:
            return cached
        started = time.perf_counter()
        before = path.stat()
        raw = path.read_bytes()
        after = path.stat()
        if self._stat_identity(before) != self._stat_identity(after):
            raise ValueError(f"{cod_path}: COD listing changed while indexing")
        indexed = self._build_index(path, raw)
        self._files[path] = indexed
        self.file_read_count += 1
        self.file_index_count += 1
        self.file_index_ms = round(
            self.file_index_ms + (time.perf_counter() - started) * 1000.0,
            3,
        )
        return indexed

    def text(self, cod_path: Path, *, errors: str) -> str:
        indexed = self._file(cod_path)
        if errors == "ignore":
            return indexed.text_ignore
        if errors == "replace":
            return indexed.text_replace
        raise ValueError(f"unsupported indexed COD decode policy: {errors}")

    @staticmethod
    def _ordinary_lines(
        cod_path: Path,
        indexed: _CodListingIndex,
        symbol_name: str,
        *,
        include_terminator: bool = False,
    ) -> tuple[str, ...]:
        selected: list[str] = []
        for record in indexed.records_by_ordinary_symbol.get(symbol_name, ()):
            selected.extend(record.body_lines)
            if (
                record.terminator_kind == "endp"
                and record.terminator_ordinary_symbol == symbol_name
            ):
                if include_terminator:
                    selected.append(
                        indexed.lines[record.terminator_line_index]
                    )
                break
        if not selected:
            raise ValueError(f"{cod_path}: no COD PROC body for {symbol_name}")
        return tuple(selected)

    @staticmethod
    def _reviewed_anonymous_lines(
        cod_path: Path,
        indexed: _CodListingIndex,
        symbol_name: str,
        normalized_symbol: str,
        *,
        include_terminator: bool = False,
    ) -> tuple[str, ...]:
        expected = {symbol_name, normalized_symbol}
        matching_records = sorted(
            (
                record
                for value in expected
                for record in indexed.records_by_path_symbol.get(value, ())
            ),
            key=lambda record: record.start_line_index,
        )
        if len(matching_records) > 1:
            raise ValueError(
                f"{cod_path}: COD identity {symbol_name!r} must resolve "
                "to exactly one PROC/ENDP pair"
            )
        if not matching_records:
            raise ValueError(f"{cod_path}: no COD PROC body for {symbol_name}")
        record = matching_records[0]
        if record.terminator_kind == "proc":
            raise ValueError(
                f"{cod_path}: COD PROC {record.path_symbol!r} has no exact "
                "matching ENDP before the next PROC"
            )
        if (
            record.terminator_kind == "endp"
            and record.terminator_path_symbol != record.path_symbol
        ):
            raise ValueError(
                f"{cod_path}: COD PROC {record.path_symbol!r} and ENDP "
                f"{record.terminator_path_symbol!r} do not agree exactly"
            )
        expected_endp_lines = sorted(
            line_index
            for value in expected
            for line_index in indexed.endp_path_line_indexes.get(value, ())
        )
        if (
            record.terminator_kind != "endp"
            or record.terminator_path_symbol != record.path_symbol
            or expected_endp_lines != [record.terminator_line_index]
        ):
            if expected_endp_lines and record.terminator_kind != "endp":
                raise ValueError(
                    f"{cod_path}: COD identity {symbol_name!r} has an ENDP "
                    "without its unique matching PROC"
                )
            raise ValueError(
                f"{cod_path}: COD identity {symbol_name!r} must have exactly "
                "one matching PROC and ENDP"
            )
        if not record.body_lines:
            raise ValueError(f"{cod_path}: no COD PROC body for {symbol_name}")
        if _cc_targets._is_zsnd_release_unknown_coff_name(symbol_name):
            escaped_coff = re.escape(symbol_name)
            escaped_cod = re.escape(normalized_symbol)
            public_rows = [
                line_index
                for line_index, line in enumerate(indexed.lines)
                if re.fullmatch(
                    rf"\s*PUBLIC\s+{escaped_coff}\s*;\s*"
                    rf"{escaped_coff}\s*",
                    line,
                )
            ]
            public_case_rows = [
                line_index
                for line_index, line in enumerate(indexed.lines)
                if "releaseunknown@?" in line.casefold()
                and line.lstrip().casefold().startswith("public")
            ]
            comdat_rows = [
                line_index
                for line_index, line in enumerate(indexed.lines)
                if re.fullmatch(
                    rf"\s*;\s*COMDAT\s+{escaped_coff}\s*",
                    line,
                )
            ]
            proc_line = indexed.lines[record.start_line_index]
            endp_line = indexed.lines[record.terminator_line_index]
            if (
                public_rows != public_case_rows
                or len(public_rows) != 1
                # VC5 emits one declaration-census COMDAT row and repeats the
                # same exact identity at the procedure body.  Neither row is
                # optional, and no third spelling may join this finite helper.
                or len(comdat_rows) != 2
                or re.fullmatch(
                    rf"\s*{escaped_cod}\s+PROC\s+NEAR\s*;\s*"
                    rf"{escaped_coff}\s*,\s*COMDAT\s*",
                    proc_line,
                )
                is None
                or re.fullmatch(
                    rf"\s*{escaped_cod}\s+ENDP\s*;\s*"
                    rf"{escaped_coff}\s*",
                    endp_line,
                )
                is None
                or _vc5_compiler_normalized_cod_proc_symbol(symbol_name)
                != normalized_symbol
            ):
                raise ValueError(
                    f"{cod_path}: zSnd ReleaseUnknown requires one exact "
                    "PUBLIC/two-COMDAT/PROC/ENDP canonical identity population"
                )
        return (
            (*record.body_lines, indexed.lines[record.terminator_line_index])
            if include_terminator
            else record.body_lines
        )

    def procedure_lines(
        self,
        cod_path: Path,
        symbol_name: str,
        *,
        include_terminator: bool = False,
    ) -> list[str]:
        started = time.perf_counter()
        self.procedure_lookup_count += 1
        path = cod_path.resolve()
        cache_key = (path, symbol_name, include_terminator)
        cached = self._procedure_results.get(cache_key)
        if cached is not None:
            self.procedure_cache_hit_count += 1
            ok, value = cached
            self.procedure_lookup_ms = round(
                self.procedure_lookup_ms
                + (time.perf_counter() - started) * 1000.0,
                3,
            )
            if ok:
                return list(value)
            raise ValueError(str(value))
        try:
            indexed = self._file(cod_path)
            normalized = _vc5_compiler_normalized_cod_proc_symbol(symbol_name)
            value = (
                self._ordinary_lines(
                    cod_path,
                    indexed,
                    symbol_name,
                    include_terminator=include_terminator,
                )
                if normalized is None
                else self._reviewed_anonymous_lines(
                    cod_path,
                    indexed,
                    symbol_name,
                    normalized,
                    include_terminator=include_terminator,
                )
            )
        except ValueError as exc:
            self._procedure_results[cache_key] = (False, str(exc))
            raise
        else:
            self._procedure_results[cache_key] = (True, value)
            return list(value)
        finally:
            self.procedure_lookup_ms = round(
                self.procedure_lookup_ms
                + (time.perf_counter() - started) * 1000.0,
                3,
            )

    def metrics(self) -> dict[str, Any]:
        return {
            "contract_version": 1,
            "scope": "live-call-contract-candidate-session",
            "file_read_count": self.file_read_count,
            "file_index_count": self.file_index_count,
            "procedure_lookup_count": self.procedure_lookup_count,
            "procedure_cache_hit_count": self.procedure_cache_hit_count,
            "file_index_ms": self.file_index_ms,
            "procedure_lookup_ms": self.procedure_lookup_ms,
        }


_ACTIVE_COD_LISTING_INDEX: ContextVar[
    _CodListingInvocationIndex | None
] = ContextVar("call_contract_cod_listing_index", default=None)


def _read_cod_text(cod_path: Path, *, errors: str) -> str:
    active = _ACTIVE_COD_LISTING_INDEX.get()
    if active is not None:
        return active.text(cod_path, errors=errors)
    return cod_path.read_text(encoding="utf-8", errors=errors)


def _extract_cod_proc_lines(
    cod_path: Path,
    symbol_name: str,
    *,
    include_terminator: bool = False,
) -> list[str]:
    active_index = _ACTIVE_COD_LISTING_INDEX.get()
    if active_index is not None:
        return active_index.procedure_lines(
            cod_path,
            symbol_name,
            include_terminator=include_terminator,
        )
    if _cc_targets._is_zsnd_release_unknown_coff_name(symbol_name):
        # Keep the reviewed ReleaseUnknown grammar identical in focused callers
        # and in the live candidate session.  The older standalone scanner below
        # recognizes the percent-normalized PROC body but has no PUBLIC/COMDAT
        # population, case, or comment authority to validate.
        return _CodListingInvocationIndex().procedure_lines(
            cod_path,
            symbol_name,
            include_terminator=include_terminator,
        )
    normalized_symbol = _vc5_compiler_normalized_cod_proc_symbol(symbol_name)
    lines = cod_path.read_text(encoding="utf-8", errors="ignore").splitlines()
    if normalized_symbol is None:
        # Ordinary symbols retain the established exact-name extraction path.
        # The stricter uniqueness/pairing rules below are confined to the
        # reviewed percent-marker bridge.
        selected: list[str] = []
        active = False
        current_segment: str | None = None
        proc_segment: str | None = None
        proc_segment_closed = False
        for line in lines:
            segment = re.match(r"^\s*(\S+)\s+SEGMENT\b", line, re.IGNORECASE)
            if segment:
                current_segment = segment.group(1)
                continue
            proc = _cc_catalog.PROC_RE.match(line)
            if proc:
                active = proc.group(1) == symbol_name
                if active:
                    proc_segment = current_segment
                    proc_segment_closed = False
                continue
            endp = _cc_catalog.ENDP_RE.match(line)
            if endp:
                if active and endp.group(1) == symbol_name:
                    if include_terminator:
                        selected.append(line)
                    break
                active = False
                continue
            segment_end = re.match(
                r"^\s*(\S+)\s+ENDS\b", line, re.IGNORECASE
            )
            if segment_end:
                if (
                    active
                    and proc_segment is not None
                    and segment_end.group(1) == proc_segment
                ):
                    proc_segment_closed = True
                if current_segment == segment_end.group(1):
                    current_segment = None
                continue
            if active and not proc_segment_closed:
                selected.append(line)
        if not selected:
            raise ValueError(f"{cod_path}: no COD PROC body for {symbol_name}")
        return selected

    expected_proc_symbols = {symbol_name}
    expected_proc_symbols.add(normalized_symbol)
    path_proc_re = re.compile(
        r"^\s*(\S(?:.*?\S)?)\s+PROC\b", re.IGNORECASE
    )
    path_endp_re = re.compile(
        r"^\s*(\S(?:.*?\S)?)\s+ENDP\b", re.IGNORECASE
    )
    selected: list[str] = []
    active = False
    active_proc_symbol: str | None = None
    current_segment: str | None = None
    proc_segment: str | None = None
    proc_segment_closed = False
    matched_proc_count = 0
    matched_endp_count = 0
    for line in lines:
        segment = re.match(r"^\s*(\S+)\s+SEGMENT\b", line, re.IGNORECASE)
        if segment:
            current_segment = segment.group(1)
            continue
        proc = path_proc_re.match(line)
        if proc:
            proc_symbol = proc.group(1)
            if proc_symbol in expected_proc_symbols:
                if active or matched_proc_count:
                    raise ValueError(
                        f"{cod_path}: COD identity {symbol_name!r} must resolve "
                        "to exactly one PROC/ENDP pair"
                    )
                active = True
                active_proc_symbol = proc_symbol
                matched_proc_count += 1
                proc_segment = current_segment
                proc_segment_closed = False
            elif active:
                raise ValueError(
                    f"{cod_path}: COD PROC {active_proc_symbol!r} has no exact "
                    "matching ENDP before the next PROC"
                )
            continue
        endp = path_endp_re.match(line)
        if endp:
            endp_symbol = endp.group(1)
            if active:
                if endp_symbol != active_proc_symbol:
                    raise ValueError(
                        f"{cod_path}: COD PROC {active_proc_symbol!r} and ENDP "
                        f"{endp_symbol!r} do not agree exactly"
                    )
                active = False
                active_proc_symbol = None
                matched_endp_count += 1
                if include_terminator:
                    selected.append(line)
            elif endp_symbol in expected_proc_symbols:
                raise ValueError(
                    f"{cod_path}: COD identity {symbol_name!r} has an ENDP "
                    "without its unique matching PROC"
                )
            continue
        segment_end = re.match(r"^\s*(\S+)\s+ENDS\b", line, re.IGNORECASE)
        if segment_end:
            if active and proc_segment is not None and segment_end.group(1) == proc_segment:
                proc_segment_closed = True
            if current_segment == segment_end.group(1):
                current_segment = None
            continue
        if active and not proc_segment_closed:
            selected.append(line)
    if matched_proc_count != 1 or matched_endp_count != 1 or active:
        if matched_proc_count:
            raise ValueError(
                f"{cod_path}: COD identity {symbol_name!r} must have exactly "
                "one matching PROC and ENDP"
            )
    if not selected:
        raise ValueError(f"{cod_path}: no COD PROC body for {symbol_name}")
    return selected


def _cod_exact_indexed_switch(
    instruction: Instruction,
) -> tuple[str, str] | None:
    """Return the exact VC5 COD table label and index register for FF /4."""
    if _cc_cfg._instruction_mnemonic(instruction) != "jmp":
        return None
    match = re.fullmatch(
        r"(?:dword\s+)?(?P<table>\$L[0-9A-Za-z_]+)"
        r"\[\s*(?P<index>e(?:ax|cx|dx|bx|bp|si|di))\s*\*\s*4\s*\]",
        _cc_cfg._instruction_operand(instruction).strip(),
        flags=re.IGNORECASE,
    )
    if match is None:
        return None
    index_register = match.group("index").lower()
    index_code = _cc_catalog._COD_SWITCH_INDEX_CODES.get(index_register)
    raw = tuple(item.lower() for item in instruction.bytes)
    expected_sib = (2 << 6) | (index_code << 3) | 5 if index_code is not None else -1
    if (
        len(raw) != 7
        or raw[0:2] != ("ff", "24")
        or int(raw[2], 16) != expected_sib
        or raw[3:] != ("00", "00", "00", "00")
    ):
        return None
    return match.group("table"), index_register


def _cod_direct_label_target(
    instruction: Instruction,
    *,
    label_instruction_indices: Mapping[str, int],
) -> int | None:
    labels = re.findall(
        r"\$[A-Za-z_][0-9A-Za-z_$]*",
        _cc_cfg._instruction_operand(instruction),
    )
    if len(labels) != 1:
        return None
    return label_instruction_indices.get(labels[0])
