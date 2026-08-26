from __future__ import annotations

from dataclasses import dataclass
import os
from pathlib import Path
import re
import shutil
import struct
from typing import Any

from _recoil.commands.asm_verify import CoffObject
from _recoil.lib.tooling import REPO_ROOT


IMAGE_FILE_MACHINE_I386 = 0x014C
IMAGE_SYM_CLASS_EXTERNAL = 2
IMAGE_SYM_CLASS_WEAK_EXTERNAL = 105
IMAGE_WEAK_EXTERN_SEARCH_ALIAS = 3

_SAFE_DIRECTIVE_RE = re.compile(
    r"(?i)^\s*(?:\.(?:386|486|586|686)(?:p)?|\.model\s+flat(?:\s*,\s*c)?|"
    r"option\s+casemap\s*:\s*none)\s*$"
)
_EXTERN_RE = re.compile(
    r"(?i)^\s*extern(?:def)?\s+([?$@A-Za-z_][?$@A-Za-z0-9_]*)\s*:\s*"
    r"(?:proc|byte|word|dword|qword)\s*$"
)
_ALIAS_RE = re.compile(
    r"(?i)^\s*alias\s*<\s*([^<>\s]+)\s*>\s*=\s*<\s*([^<>\s]+)\s*>\s*$"
)
_END_RE = re.compile(r"(?i)^\s*end\s*$")
_FORBIDDEN_SOURCE_RE = re.compile(
    r"(?i)\b(?:segment|ends|proc|endp|macro|endm|equ|textequ|include|"
    r"public|comm|org|db|dw|dd|dq|dt|dup|invoke)\b|^\s*\.(?:code|data|const|stack|startup)\b"
)


@dataclass(frozen=True)
class CoffAlias:
    alias: str
    target: str


@dataclass(frozen=True)
class CoffAliasSource:
    source: Path
    aliases: tuple[CoffAlias, ...]
    link_after_source: Path


def repository_relative_path(value: object, *, label: str, manifest_path: Path) -> Path:
    if not isinstance(value, str) or not value.strip():
        raise ValueError(f"{manifest_path}: {label} must be a non-empty repository-relative path")
    raw = Path(value)
    if raw.is_absolute():
        raise ValueError(f"{manifest_path}: {label} must be repository-relative")
    resolved = (REPO_ROOT / raw).resolve()
    try:
        resolved.relative_to(REPO_ROOT.resolve())
    except ValueError as exc:
        raise ValueError(f"{manifest_path}: {label} must remain inside the repository") from exc
    return resolved


def parse_coff_alias_sources(
    data: dict[str, Any],
    *,
    configured_sources: tuple[Path, ...],
    manifest_path: Path,
) -> tuple[CoffAliasSource, ...]:
    raw_rows = data.get("coff_alias_sources")
    if raw_rows is None:
        return ()
    if not isinstance(raw_rows, list):
        raise ValueError(f"{manifest_path}: coff_alias_sources must be a list")

    source_keys = {os.path.normcase(str(path.resolve())): path for path in configured_sources}
    seen_alias_sources: set[str] = set()
    seen_aliases: set[str] = set()
    rows: list[CoffAliasSource] = []
    for row_index, raw_row in enumerate(raw_rows):
        label = f"coff_alias_sources[{row_index}]"
        if not isinstance(raw_row, dict) or set(raw_row) != {
            "source",
            "aliases",
            "link_after_source",
        }:
            raise ValueError(
                f"{manifest_path}: {label} must contain exactly source, aliases, and link_after_source"
            )
        source = repository_relative_path(
            raw_row["source"], label=f"{label}.source", manifest_path=manifest_path
        )
        if source.suffix.lower() not in {".asm", ".s"}:
            raise ValueError(f"{manifest_path}: {label}.source must be an assembly source path")
        source_key = os.path.normcase(str(source))
        if source_key in seen_alias_sources:
            raise ValueError(f"{manifest_path}: duplicate coff_alias_sources source: {source}")
        seen_alias_sources.add(source_key)

        link_after_source = repository_relative_path(
            raw_row["link_after_source"],
            label=f"{label}.link_after_source",
            manifest_path=manifest_path,
        )
        if os.path.normcase(str(link_after_source)) not in source_keys:
            raise ValueError(
                f"{manifest_path}: {label}.link_after_source must name one configured C/C++ source"
            )

        raw_aliases = raw_row["aliases"]
        if not isinstance(raw_aliases, list) or not raw_aliases:
            raise ValueError(f"{manifest_path}: {label}.aliases must be a non-empty list")
        aliases: list[CoffAlias] = []
        for alias_index, raw_alias in enumerate(raw_aliases):
            alias_label = f"{label}.aliases[{alias_index}]"
            if not isinstance(raw_alias, dict) or set(raw_alias) != {"alias", "target"}:
                raise ValueError(
                    f"{manifest_path}: {alias_label} must contain exactly alias and target"
                )
            alias = raw_alias["alias"]
            target = raw_alias["target"]
            if (
                not isinstance(alias, str)
                or not alias
                or not isinstance(target, str)
                or not target
                or alias == target
            ):
                raise ValueError(
                    f"{manifest_path}: {alias_label} requires distinct non-empty alias/target strings"
                )
            if alias in seen_aliases:
                raise ValueError(f"{manifest_path}: duplicate weak alias identity: {alias}")
            seen_aliases.add(alias)
            aliases.append(CoffAlias(alias=alias, target=target))
        row = CoffAliasSource(
            source=source,
            aliases=tuple(aliases),
            link_after_source=source_keys[os.path.normcase(str(link_after_source))],
        )
        validate_alias_source(row)
        rows.append(row)
    return tuple(rows)


def parse_alias_source_text(text: str, *, path: Path) -> tuple[set[str], tuple[CoffAlias, ...]]:
    externs: set[str] = set()
    aliases: list[CoffAlias] = []
    ended = False
    for line_number, raw_line in enumerate(text.splitlines(), 1):
        line = raw_line.split(";", 1)[0].strip()
        if not line:
            continue
        if ended:
            raise ValueError(f"{path}:{line_number}: content after END is not allowed")
        if _SAFE_DIRECTIVE_RE.fullmatch(line):
            continue
        match = _EXTERN_RE.fullmatch(line)
        if match:
            symbol = match.group(1)
            if symbol in externs:
                raise ValueError(f"{path}:{line_number}: duplicate EXTERN {symbol}")
            externs.add(symbol)
            continue
        match = _ALIAS_RE.fullmatch(line)
        if match:
            aliases.append(CoffAlias(alias=match.group(1), target=match.group(2)))
            continue
        if _END_RE.fullmatch(line):
            ended = True
            continue
        if _FORBIDDEN_SOURCE_RE.search(line):
            raise ValueError(
                f"{path}:{line_number}: segments, procedures, instructions, data, and macros are forbidden"
            )
        raise ValueError(
            f"{path}:{line_number}: only safe model/CPU directives, EXTERN, ALIAS, and END are allowed"
        )
    if not ended:
        raise ValueError(f"{path}: alias-only assembly source must end with END")
    return externs, tuple(aliases)


def validate_alias_source(spec: CoffAliasSource) -> dict[str, object]:
    if not spec.source.is_file():
        raise ValueError(f"COFF alias source is missing: {spec.source}")
    externs, aliases = parse_alias_source_text(
        spec.source.read_text(encoding="utf-8", errors="strict"),
        path=spec.source,
    )
    expected = tuple(spec.aliases)
    if aliases != expected:
        raise ValueError(
            f"{spec.source}: ALIAS rows must exactly match manifest order and identities"
        )
    targets = {item.target for item in expected}
    if externs != targets:
        raise ValueError(
            f"{spec.source}: EXTERN identities must exactly equal declared ALIAS targets"
        )
    return {
        "source": str(spec.source.resolve()),
        "externs": sorted(externs),
        "aliases": [
            {"alias": item.alias, "target": item.target}
            for item in expected
        ],
        "policy_class": "coff-weak-alias-only",
    }


def resolve_llvm_ml(environ: dict[str, str] | None = None) -> Path:
    environment = os.environ if environ is None else environ
    configured = environment.get("RECOIL_LLVM_ML", "").strip()
    if configured:
        path = Path(configured).expanduser()
        if not path.is_file():
            raise ValueError(f"RECOIL_LLVM_ML does not name an existing file: {path}")
        return path.resolve()
    on_path = shutil.which("llvm-ml.exe") or shutil.which("llvm-ml")
    if on_path:
        return Path(on_path).resolve()
    fallback = Path(r"C:\Program Files\LLVM\bin\llvm-ml.exe")
    if fallback.is_file():
        return fallback.resolve()
    raise ValueError(
        "llvm-ml was not found; set RECOIL_LLVM_ML, add llvm-ml to PATH, "
        r"or install it at C:\Program Files\LLVM\bin\llvm-ml.exe"
    )


def validate_alias_object(path: Path, spec: CoffAliasSource) -> dict[str, object]:
    data = path.read_bytes()
    if len(data) < 20:
        raise ValueError(f"{path}: COFF alias object is truncated")
    machine, section_count, _timestamp, _symbol_offset, _symbol_count, optional_size, _flags = (
        struct.unpack_from("<HHIIIHH", data, 0)
    )
    if machine != IMAGE_FILE_MACHINE_I386:
        raise ValueError(f"{path}: COFF alias object machine must be i386 (0x014c)")
    if optional_size != 0:
        raise ValueError(f"{path}: COFF alias object must not have an optional header")
    if section_count != 0:
        raise ValueError(f"{path}: COFF alias object must have zero sections")

    coff = CoffObject.from_bytes(data)
    if coff.sections or any(coff.relocations_by_section.values()):
        raise ValueError(f"{path}: COFF alias object must have no sections or relocations")
    symbols = list(coff.symbols)
    expected_aliases = {item.alias: item.target for item in spec.aliases}
    expected_targets = set(expected_aliases.values())
    targets = {
        symbol.name: symbol
        for symbol in symbols
        if symbol.storage_class == IMAGE_SYM_CLASS_EXTERNAL
        and symbol.section_number == 0
        and symbol.value == 0
        and symbol.aux_count == 0
    }
    weak = {
        symbol.name: symbol
        for symbol in symbols
        if symbol.storage_class == IMAGE_SYM_CLASS_WEAK_EXTERNAL
    }
    feature_symbols = [
        symbol
        for symbol in symbols
        if symbol.name == "@feat.00"
        and symbol.storage_class == IMAGE_SYM_CLASS_EXTERNAL
        and symbol.section_number == -1
        and symbol.type == 0
        and symbol.aux_count == 0
    ]
    if set(targets) != expected_targets or set(weak) != set(expected_aliases):
        raise ValueError(
            f"{path}: COFF symbols must contain only exact declared externs and WeakExternal aliases"
        )
    if len(feature_symbols) > 1 or len(symbols) != len(targets) + len(weak) + len(feature_symbols):
        raise ValueError(f"{path}: COFF alias object contains unexpected symbols")

    rows: list[dict[str, object]] = []
    for alias, expected_target in expected_aliases.items():
        symbol = weak[alias]
        if (
            symbol.section_number != 0
            or symbol.value != 0
            or symbol.type != 0
            or symbol.aux_count != 1
            or symbol.weak_external_characteristics != IMAGE_WEAK_EXTERN_SEARCH_ALIAS
        ):
            raise ValueError(
                f"{path}: {alias} must be one undefined WeakExternal Alias row"
            )
        target_index = symbol.weak_external_tag_index
        target = coff.symbols_by_index.get(target_index if target_index is not None else -1)
        if target is None or target.name != expected_target or target.name not in targets:
            raise ValueError(
                f"{path}: WeakExternal {alias} must target exact declared extern {expected_target}"
            )
        rows.append(
            {
                "alias": alias,
                "target": target.name,
                "characteristics": symbol.weak_external_characteristics,
            }
        )
    return {
        "path": str(path.resolve()),
        "machine": "i386",
        "section_count": 0,
        "relocation_count": 0,
        "contribution_bytes": 0,
        "weak_aliases": rows,
        "assembler_metadata_symbols": [symbol.name for symbol in feature_symbols],
        "validated": True,
        "policy_class": "coff-weak-alias-only",
        "grants": [],
    }
