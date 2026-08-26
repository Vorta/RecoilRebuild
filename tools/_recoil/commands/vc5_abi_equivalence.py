from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
import os
from pathlib import Path
import re
import sys
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import (
    CoffObject,
    IMAGE_REL_I386_REL32,
    IMAGE_SCN_CNT_CODE,
    relocation_size,
)
from _recoil.commands.vc5_build import (
    DEFAULT_MANIFEST,
    FinalBuildConfig,
    effective_compile_flags,
    load_config,
    safe_object_stem,
)
from _recoil.lib.coff_alias import repository_relative_path
from _recoil.lib.tooling import (
    REPO_ROOT,
    quote_cmd_arg,
    response_line,
    run_cmd_script,
)
from _recoil.lib.vc5_compile_topology import (
    IMAGE_SCN_LNK_COMDAT,
    IMAGE_SYM_DTYPE_FUNCTION,
    section_aux_metadata,
)


ELIGIBLE_RETURN_CATEGORIES = {
    "void",
    "pointer",
    "integral8",
    "integral16",
    "integral32",
}
INELIGIBLE_CONTEXT_FLAGS = (
    "hidden_this",
    "hidden_sret",
    "lifecycle",
    "variadic",
    "address_taken",
    "callback",
    "vtable",
    "export",
    "import",
    "function_pointer",
)
SYMBOL_RE = re.compile(r"^[^\s]+$")
ABI_EQUIVALENCE_PROBE_DEFINE = "RECOIL_VC5_ABI_EQUIVALENCE_PROBE"


@dataclass(frozen=True)
class SymbolPair:
    cdecl_symbol: str
    fastcall_symbol: str


@dataclass(frozen=True)
class SourceSymbolPair:
    source: Path
    cdecl_symbol: str
    fastcall_symbol: str

    @property
    def symbols(self) -> SymbolPair:
        return SymbolPair(self.cdecl_symbol, self.fastcall_symbol)


@dataclass(frozen=True)
class ZeroArgAbiTarget:
    target_id: str
    identity: str
    callee_source: Path
    callee: SymbolPair
    callers: tuple[SourceSymbolPair, ...]
    return_category: str
    retail_evidence: dict[str, Any]
    eh_policy: dict[str, Any]
    st0_policy: dict[str, Any] | None


def _required_string(row: dict[str, Any], key: str, *, label: str) -> str:
    value = row.get(key)
    if not isinstance(value, str) or not value or not SYMBOL_RE.fullmatch(value):
        raise ValueError(f"{label}.{key} must be a non-empty whitespace-free string")
    return value


def _symbol_pair(row: object, *, label: str) -> SymbolPair:
    if not isinstance(row, dict) or set(row) != {"cdecl_symbol", "fastcall_symbol"}:
        raise ValueError(f"{label} must contain exactly cdecl_symbol and fastcall_symbol")
    return SymbolPair(
        cdecl_symbol=_required_string(row, "cdecl_symbol", label=label),
        fastcall_symbol=_required_string(row, "fastcall_symbol", label=label),
    )


def _source_symbol_pair(
    row: object,
    *,
    label: str,
    source_for: Any,
) -> SourceSymbolPair:
    if not isinstance(row, dict) or set(row) != {
        "source",
        "cdecl_symbol",
        "fastcall_symbol",
    }:
        raise ValueError(
            f"{label} must contain exactly source, cdecl_symbol, and fastcall_symbol"
        )
    return SourceSymbolPair(
        source=source_for(row["source"], label=f"{label}.source"),
        cdecl_symbol=_required_string(row, "cdecl_symbol", label=label),
        fastcall_symbol=_required_string(row, "fastcall_symbol", label=label),
    )


def load_zeroarg_targets(path: Path) -> tuple[ZeroArgAbiTarget, ...]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise ValueError(f"{path}: manifest root must be an object")
    raw_sources = data.get("sources")
    if not isinstance(raw_sources, list) or not raw_sources:
        raise ValueError(f"{path}: sources must be a non-empty manifest-owned list")
    configured_sources: dict[str, Path] = {}
    for source_index, raw_source in enumerate(raw_sources):
        source = repository_relative_path(
            raw_source,
            label=f"sources[{source_index}]",
            manifest_path=path,
        )
        key = os.path.normcase(str(source))
        if key in configured_sources:
            raise ValueError(
                f"{path}: duplicate or ambiguous configured source path: {raw_source}"
            )
        configured_sources[key] = source

    def source_for(value: object, *, label: str) -> Path:
        source = repository_relative_path(
            value,
            label=label,
            manifest_path=path,
        )
        configured = configured_sources.get(os.path.normcase(str(source)))
        if configured is None:
            raise ValueError(
                f"{path}: {label} must name one exact configured source"
            )
        return configured

    raw_queue = data.get("zeroarg_abi_equivalence")
    if not isinstance(raw_queue, list):
        raise ValueError(f"{path}: zeroarg_abi_equivalence must be a manifest-owned list")
    rows: list[ZeroArgAbiTarget] = []
    seen_ids: set[str] = set()
    seen_identities: set[str] = set()
    for index, raw in enumerate(raw_queue):
        label = f"{path}: zeroarg_abi_equivalence[{index}]"
        if not isinstance(raw, dict):
            raise ValueError(f"{label} must be an object")
        allowed = {
            "id",
            "identity",
            "callee_source",
            "cdecl_symbol",
            "fastcall_symbol",
            "callers",
            "return_category",
            "retail_evidence",
            "eh_policy",
            "st0_policy",
        }
        extra = set(raw) - allowed
        if extra:
            raise ValueError(f"{label} has unsupported fields: {', '.join(sorted(extra))}")
        target_id = _required_string(raw, "id", label=label)
        if target_id in seen_ids:
            raise ValueError(f"{label}: duplicate target id {target_id}")
        seen_ids.add(target_id)
        identity = _required_string(raw, "identity", label=label)
        if identity in seen_identities:
            raise ValueError(f"{label}: duplicate target identity {identity}")
        seen_identities.add(identity)
        callee_source = source_for(
            raw.get("callee_source"),
            label=f"zeroarg_abi_equivalence[{index}].callee_source",
        )
        callers_raw = raw.get("callers")
        if not isinstance(callers_raw, list) or not callers_raw:
            raise ValueError(f"{label}.callers must be a non-empty list")
        retail_evidence = raw.get("retail_evidence")
        eh_policy = raw.get("eh_policy")
        if not isinstance(retail_evidence, dict):
            raise ValueError(f"{label}.retail_evidence must be an object")
        if not isinstance(eh_policy, dict):
            raise ValueError(f"{label}.eh_policy must be an object")
        return_category = raw.get("return_category")
        if not isinstance(return_category, str) or not return_category:
            raise ValueError(f"{label}.return_category must be a string")
        st0_policy = raw.get("st0_policy")
        if st0_policy is not None and not isinstance(st0_policy, dict):
            raise ValueError(f"{label}.st0_policy must be an object when present")
        callers = tuple(
            _source_symbol_pair(
                item,
                label=f"{label}.callers[{caller_index}]",
                source_for=source_for,
            )
            for caller_index, item in enumerate(callers_raw)
        )
        caller_keys = {
            (
                os.path.normcase(str(caller.source)),
                caller.cdecl_symbol,
                caller.fastcall_symbol,
            )
            for caller in callers
        }
        if len(caller_keys) != len(callers):
            raise ValueError(f"{label}.callers contains a duplicate exact caller row")
        rows.append(
            ZeroArgAbiTarget(
                target_id=target_id,
                identity=identity,
                callee_source=callee_source,
                callee=SymbolPair(
                    cdecl_symbol=_required_string(raw, "cdecl_symbol", label=label),
                    fastcall_symbol=_required_string(raw, "fastcall_symbol", label=label),
                ),
                callers=callers,
                return_category=return_category,
                retail_evidence=retail_evidence,
                eh_policy=eh_policy,
                st0_policy=st0_policy,
            )
        )
    result = tuple(rows)
    manifest_symbol_normalization(result)
    return result


def manifest_symbol_normalization(
    targets: Sequence[ZeroArgAbiTarget],
) -> dict[str, str]:
    """Normalize every exact manifest-declared /Gd-/Gr symbol pair.

    Raw definition order is a translation-unit-wide observation.  Selecting
    one identity authorizes only that identity, but unrelated queue identities
    in the same source still change decoration under the paired profiles and
    therefore must participate in mechanical normalization.
    """
    pair_identities: dict[tuple[str, str], str] = {}
    symbol_bindings: dict[str, tuple[str, str]] = {}
    normalization: dict[str, str] = {}
    stable_caller_symbols: set[str] = set()
    pairs: list[tuple[SymbolPair, str]] = []
    for target in targets:
        pairs.append((target.callee, target.identity))
        for caller in target.callers:
            if caller.cdecl_symbol == caller.fastcall_symbol:
                stable_caller_symbols.add(caller.cdecl_symbol)
                continue
            key = (caller.cdecl_symbol, caller.fastcall_symbol)
            caller_identity = pair_identities.setdefault(
                key,
                f"manifest-zeroarg-caller-pair:{len(pair_identities)}",
            )
            pairs.append((caller.symbols, caller_identity))
    for pair, requested_identity in pairs:
        key = (pair.cdecl_symbol, pair.fastcall_symbol)
        if pair.cdecl_symbol == pair.fastcall_symbol:
            raise ValueError(
                f"ABI symbol pair must use distinct /Gd and /Gr symbols: "
                f"{pair.cdecl_symbol}"
            )
        identity = pair_identities.setdefault(key, requested_identity)
        if identity != requested_identity:
            raise ValueError(
                f"ambiguous manifest zero-argument symbol pair "
                f"{pair.cdecl_symbol!r}/{pair.fastcall_symbol!r}"
            )
        for symbol in key:
            existing = symbol_bindings.get(symbol)
            if existing is not None and existing != key:
                raise ValueError(
                    f"ambiguous manifest zero-argument symbol mapping for {symbol!r}"
                )
            symbol_bindings[symbol] = key
            normalization[symbol] = identity
    conflicts = stable_caller_symbols.intersection(normalization)
    if conflicts:
        raise ValueError(
            "ambiguous stable caller symbol also participates in a /Gd-/Gr "
            f"pair: {', '.join(sorted(conflicts))}"
        )
    return normalization


def eligibility_gates(target: ZeroArgAbiTarget) -> list[dict[str, object]]:
    evidence = target.retail_evidence
    gates: list[dict[str, object]] = []

    def gate(name: str, passed: bool, actual: object) -> None:
        gates.append({"gate": name, "passed": bool(passed), "actual": actual})

    gate("free-or-static", evidence.get("free_or_static") is True, evidence.get("free_or_static"))
    count = evidence.get("explicit_argument_count")
    gate("zero-explicit-arguments", type(count) is int and count == 0, count)
    for name in INELIGIBLE_CONTEXT_FLAGS:
        gate(f"not-{name.replace('_', '-')}", evidence.get(name) is False, evidence.get(name))

    direct_calls = evidence.get("direct_calls")
    direct_ok = isinstance(direct_calls, list) and bool(direct_calls)
    if direct_ok:
        for call in direct_calls:
            if not isinstance(call, dict):
                direct_ok = False
                break
            address = call.get("address")
            call_count = call.get("explicit_argument_count")
            direct_ok = direct_ok and (
                isinstance(address, str)
                and re.fullmatch(r"0x[0-9a-fA-F]+", address) is not None
                and call.get("dispatch") == "direct"
                and type(call_count) is int
                and call_count == 0
                and call.get("callee_return") == "plain-ret"
            )
    gate("retail-direct-noarg-plain-ret-calls", direct_ok, direct_calls)

    return_ok = target.return_category in ELIGIBLE_RETURN_CATEGORIES
    if target.return_category == "x87-float":
        policy = target.st0_policy
        return_ok = (
            isinstance(policy, dict)
            and policy.get("proven") is True
            and isinstance(policy.get("evidence"), str)
            and bool(policy["evidence"].strip())
        )
    gate("eligible-return-category", return_ok, target.return_category)

    eh_kind = target.eh_policy.get("kind")
    eh_ok = (
        target.eh_policy.get("retail_proven") is True
        and (
            eh_kind == "none"
            or (
                eh_kind == "paired-sections"
                and isinstance(target.eh_policy.get("sections"), list)
                and bool(target.eh_policy["sections"])
                and all(
                    isinstance(item, str) and item
                    for item in target.eh_policy["sections"]
                )
            )
        )
    )
    gate("explicit-eh-policy", eh_ok, target.eh_policy)
    return gates


def _profile_flags(flags: tuple[str, ...], convention: str) -> tuple[str, ...]:
    if convention not in {"Gd", "Gr"}:
        raise ValueError(f"unknown ABI profile convention: {convention}")
    filtered = [
        flag
        for flag in flags
        if flag.casefold() not in {"/gd", "/gr"}
    ]
    filtered.append("/" + convention)
    return tuple(filtered)


def _safe_build_root(path: Path) -> Path:
    resolved = (REPO_ROOT / path).resolve() if not path.is_absolute() else path.resolve()
    try:
        resolved.relative_to((REPO_ROOT / "build").resolve())
    except ValueError as exc:
        raise ValueError("--build-root must resolve below the repository build directory") from exc
    return resolved


def _compile_one(
    config: FinalBuildConfig,
    source: Path,
    *,
    convention: str,
    build_root: Path,
) -> tuple[int, Path, Path, Path]:
    profile = convention.lower()
    stem = safe_object_stem(source)
    obj = build_root / profile / "obj" / stem
    rsp = build_root / profile / "rsp" / stem.with_suffix(".rsp")
    stdout_log = build_root / profile / "logs" / stem.with_suffix(".out.log")
    stderr_log = build_root / profile / "logs" / stem.with_suffix(".err.log")
    for path in (obj.parent, rsp.parent, stdout_log.parent):
        path.mkdir(parents=True, exist_ok=True)
    flags = _profile_flags(effective_compile_flags(config, source), convention)
    args = [*flags]
    args.append(f"/D{ABI_EQUIVALENCE_PROBE_DEFINE}")
    args.extend(f"/D{define}" for define in config.defines)
    args.extend(f"/I{path}" for path in config.include_dirs)
    args.extend((f"/Fo{obj}", "/c", str(source)))
    rsp.write_text("\n".join(response_line(arg) for arg in args) + "\n", encoding="ascii")
    command = (
        f"call {quote_cmd_arg(config.vc5_env)} && "
        f"cl @{quote_cmd_arg(rsp)}"
    )
    with stdout_log.open("w", encoding="utf-8", errors="replace") as stdout, stderr_log.open(
        "w", encoding="utf-8", errors="replace"
    ) as stderr:
        completed = run_cmd_script(
            command,
            cwd=REPO_ROOT,
            stdout=stdout,
            stderr=stderr,
            script_name=str(build_root / profile / "_compile.cmd"),
        )
    return completed.returncode, obj, stdout_log, stderr_log


def _relocation_rows(coff: CoffObject, symbol_name: str, normalize: dict[str, str]) -> list[dict[str, object]]:
    body = coff.function_bytes(symbol_name)
    rows: list[dict[str, object]] = []
    section = coff.section(body.section_index)
    for relocation in body.relocations:
        size = relocation_size(relocation.type)
        raw = section.raw_data[relocation.offset : relocation.offset + size]
        rows.append(
            {
                "offset": relocation.offset - body.start,
                "type": relocation.type,
                "target": normalize.get(relocation.symbol_name, relocation.symbol_name),
                "addend_hex": raw.hex(),
            }
        )
    return rows


def _function_shape(
    coff: CoffObject,
    symbol_name: str,
    normalize: dict[str, str],
) -> dict[str, object]:
    body = coff.function_bytes(symbol_name)
    section = coff.section(body.section_index)
    return {
        "symbol": symbol_name,
        "bytes_hex": body.data.hex(),
        "plain_ret": _is_plain_ret_with_coff_nop_padding(
            body.data,
            relocation_mask=body.relocation_mask,
        ),
        "relocations": _relocation_rows(coff, symbol_name, normalize),
        "comdat": bool(section.characteristics & IMAGE_SCN_LNK_COMDAT),
        "comdat_selection": section_aux_metadata(coff, body.section_index),
    }


def _is_plain_ret_with_coff_nop_padding(
    data: bytes,
    *,
    relocation_mask: tuple[bool, ...] | None = None,
) -> bool:
    """Recognize an unrelocated near RET plus unrelocated COFF NOP padding."""

    if not data:
        return False
    if relocation_mask is None:
        relocation_mask = (False,) * len(data)
    if len(relocation_mask) != len(data):
        raise ValueError("plain-RET relocation mask length does not match body")
    last_non_padding = len(data) - 1
    while last_non_padding >= 0 and data[last_non_padding] == 0x90:
        if relocation_mask[last_non_padding]:
            return False
        last_non_padding -= 1
    return (
        last_non_padding >= 0
        and data[last_non_padding] == 0xC3
        and not relocation_mask[last_non_padding]
    )


def _definition_order(coff: CoffObject, normalize: dict[str, str]) -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    for symbol in coff.symbols:
        if symbol.section_number <= 0 or (symbol.type & IMAGE_SYM_DTYPE_FUNCTION) == 0:
            continue
        section = coff.section(symbol.section_number)
        if not (section.characteristics & IMAGE_SCN_CNT_CODE):
            continue
        # Preserve strict COFF function-extent validation even though raw-order
        # equality intentionally omits body size and relocation count.
        coff.function_bytes(symbol.name)
        rows.append(
            {
                "identity": normalize.get(symbol.name, symbol.name),
                "section_number": symbol.section_number,
                "value": symbol.value,
                "comdat": bool(section.characteristics & IMAGE_SCN_LNK_COMDAT),
                "selection": section_aux_metadata(coff, symbol.section_number),
            }
        )
    return rows


def _profile_definition_counterpart(
    symbol: str,
    *,
    from_convention: str,
) -> str | None:
    """Return an exact order-only counterpart for a default-convention symbol.

    Nonlocal C++ symbols preserve their complete decorated head and encoded
    return/type/argument tail through terminal ``Z``; only ``A`` versus ``I``
    changes between the paired compiler profiles. Local discriminator symbols
    are deliberately handled by :func:`_local_definition_key`.
    """

    if from_convention not in {"Gd", "Gr"}:
        raise ValueError(f"unknown ABI profile convention: {from_convention}")
    replacements = ()
    if "@?%" not in symbol:
        replacements = (
            (r"^(?P<head>\?.+@@)YA(?P<tail>.+Z)$", r"\g<head>YI\g<tail>"),
            (r"^(?P<head>\?.+@@)SA(?P<tail>.+Z)$", r"\g<head>SI\g<tail>"),
        )
    if from_convention == "Gr":
        replacements = ()
        if "@?%" not in symbol:
            replacements = (
                (r"^(?P<head>\?.+@@)YI(?P<tail>.+Z)$", r"\g<head>YA\g<tail>"),
                (r"^(?P<head>\?.+@@)SI(?P<tail>.+Z)$", r"\g<head>SA\g<tail>"),
            )
    for pattern, replacement in replacements:
        if re.fullmatch(pattern, symbol):
            return re.sub(pattern, replacement, symbol)
    if from_convention == "Gd":
        match = re.fullmatch(r"_([A-Za-z_?$][A-Za-z0-9_?$]*)", symbol)
        return f"@{match.group(1)}@0" if match is not None else None
    match = re.fullmatch(
        r"@([A-Za-z_?$][A-Za-z0-9_?$]*)@[0-9]+",
        symbol,
    )
    return f"_{match.group(1)}" if match is not None else None


def _local_definition_key(
    symbol: str,
    *,
    convention: str,
) -> tuple[str, str, str, str] | None:
    """Return an exact raw-order key for one VC5 local source symbol.

    VC5 places a decimal source discriminator immediately after the source
    filename in local-scope decorated names.  That discriminator can change
    between otherwise equivalent /Gd and /Gr translation-unit compiles.  The
    key deliberately retains the logical name and exact repository source
    path. Default-convention suffixes retain their Y/S family and complete
    encoded tail while omitting only paired A/I. Every other suffix is retained
    byte-for-byte and must therefore be stable across both profiles. Only the
    decimal discriminator is always omitted.
    """
    if convention not in {"Gd", "Gr"}:
        raise ValueError(f"unknown ABI profile convention: {convention}")
    match = re.fullmatch(
        r"(?P<logical>\?.+?)@\?%"
        r"(?P<source>.+\.(?:c|cc|cpp|cxx))"
        r"(?P<discriminator>[0-9]+)@@"
        r"(?P<suffix>.+Z)",
        symbol,
        flags=re.IGNORECASE,
    )
    if match is None:
        return None
    suffix = match.group("suffix")
    default_calling = suffix[:2].upper()
    if default_calling in {"YA", "YI", "SA", "SI"}:
        allowed = {"YA", "SA"} if convention == "Gd" else {"YI", "SI"}
        if default_calling not in allowed:
            raise ValueError(
                f"local default-convention definition is on the wrong "
                f"/{convention} side: {symbol!r}"
            )
        suffix_mode = "default"
        canonical_suffix = default_calling[0] + suffix[2:]
    else:
        suffix_mode = "stable"
        canonical_suffix = suffix
    source_text = match.group("source")
    source_path = Path(source_text)
    resolved = (
        source_path.resolve()
        if source_path.is_absolute()
        else (REPO_ROOT / source_path).resolve()
    )
    try:
        relative = resolved.relative_to(REPO_ROOT.resolve())
    except ValueError:
        return None
    if not resolved.is_file():
        return None
    return (
        match.group("logical"),
        relative.as_posix().casefold(),
        suffix_mode,
        canonical_suffix,
    )


def _paired_raw_definition_normalization(
    cdecl: CoffObject,
    fastcall: CoffObject,
    manifest_normalization: Mapping[str, str],
) -> dict[str, str]:
    """Add exact order-only VC5 definition counterparts."""

    def definition_names(coff: CoffObject) -> list[str]:
        result: list[str] = []
        for symbol in coff.symbols:
            if symbol.section_number <= 0 or (
                symbol.type & IMAGE_SYM_DTYPE_FUNCTION
            ) == 0:
                continue
            if coff.section(symbol.section_number).characteristics & IMAGE_SCN_CNT_CODE:
                result.append(symbol.name)
        return result

    cdecl_names = definition_names(cdecl)
    fastcall_names = definition_names(fastcall)
    cdecl_counts = {name: cdecl_names.count(name) for name in set(cdecl_names)}
    fastcall_counts = {
        name: fastcall_names.count(name) for name in set(fastcall_names)
    }
    cdecl_local_by_key: dict[tuple[str, str, str, str], list[str]] = {}
    fastcall_local_by_key: dict[tuple[str, str, str, str], list[str]] = {}
    for name in cdecl_names:
        key = _local_definition_key(name, convention="Gd")
        if key is not None:
            cdecl_local_by_key.setdefault(key, []).append(name)
    for name in fastcall_names:
        key = _local_definition_key(name, convention="Gr")
        if key is not None:
            fastcall_local_by_key.setdefault(key, []).append(name)
    for side, rows in (
        ("Gd", cdecl_local_by_key),
        ("Gr", fastcall_local_by_key),
    ):
        collisions = [names for names in rows.values() if len(names) != 1]
        if collisions:
            raise ValueError(
                f"ambiguous {side} local definition discriminator "
                f"mapping: {collisions[0]!r}"
            )
    normalization = dict(manifest_normalization)
    manifest_cdecl_by_identity: dict[str, list[str]] = {}
    manifest_fastcall_by_identity: dict[str, list[str]] = {}
    for name in cdecl_counts:
        identity = normalization.get(name)
        if identity is not None:
            manifest_cdecl_by_identity.setdefault(identity, []).append(name)
    for name in fastcall_counts:
        identity = normalization.get(name)
        if identity is not None:
            manifest_fastcall_by_identity.setdefault(identity, []).append(name)
    for identity in set(manifest_cdecl_by_identity) | set(
        manifest_fastcall_by_identity
    ):
        cdecl_rows = manifest_cdecl_by_identity.get(identity, [])
        fastcall_rows = manifest_fastcall_by_identity.get(identity, [])
        if (
            len(cdecl_rows) != 1
            or len(fastcall_rows) != 1
            or cdecl_counts[cdecl_rows[0]] != 1
            or fastcall_counts[fastcall_rows[0]] != 1
        ):
            raise ValueError(
                f"manifest ABI definition pair {identity!r} is unpaired or ambiguous"
            )

    paired_cdecl: set[str] = set(
        name for names in manifest_cdecl_by_identity.values() for name in names
    )
    paired_fastcall: set[str] = set(
        name for names in manifest_fastcall_by_identity.values() for name in names
    )
    for cdecl_name in cdecl_names:
        if cdecl_name in paired_cdecl:
            continue
        fastcall_name = _profile_definition_counterpart(
            cdecl_name,
            from_convention="Gd",
        )
        local_key = _local_definition_key(
            cdecl_name,
            convention="Gd",
        )
        if fastcall_name is None and local_key is None:
            continue
        local_candidates = (
            fastcall_local_by_key.get(local_key, [])
            if local_key is not None
            else []
        )
        extern_c = re.fullmatch(
            r"_([A-Za-z_?$][A-Za-z0-9_?$]*)",
            cdecl_name,
        )
        extern_c_candidates: list[tuple[str, str]] = []
        if extern_c is not None:
            candidate_pattern = re.compile(
                rf"@{re.escape(extern_c.group(1))}@(?P<stack>[0-9]+)"
            )
            extern_c_candidates = [
                (name, match.group("stack"))
                for name in fastcall_counts
                if (match := candidate_pattern.fullmatch(name)) is not None
            ]
            if len(extern_c_candidates) > 1:
                raise ValueError(
                    f"ambiguous /Gd extern-C definition {cdecl_name!r}: "
                    f"multiple /Gr stack-byte candidates "
                    f"{[name for name, _stack in extern_c_candidates]!r}"
                )
        extern_c_stack_bytes: str | None = None
        if extern_c_candidates:
            fastcall_name, extern_c_stack_bytes = extern_c_candidates[0]
            counterpart_count = fastcall_counts[fastcall_name]
        elif local_candidates:
            fastcall_name = local_candidates[0]
            counterpart_count = len(local_candidates)
        else:
            counterpart_count = fastcall_counts.get(fastcall_name, 0)
        stable_count = fastcall_counts.get(cdecl_name, 0)
        if counterpart_count and stable_count:
            raise ValueError(
                f"ambiguous /Gd raw definition {cdecl_name!r}: "
                "both transformed and stable /Gr definitions exist"
            )
        if counterpart_count:
            if cdecl_counts[cdecl_name] != 1 or counterpart_count != 1:
                raise ValueError(
                    f"ambiguous paired raw definition {cdecl_name!r}"
                )
            identity = (
                f"mechanical-extern-c-definition:{cdecl_name}:"
                f"fastcall-stack-bytes:{extern_c_stack_bytes}"
                if extern_c_stack_bytes is not None
                else f"mechanical-abi-definition:{cdecl_name}"
            )
            for name in (cdecl_name, fastcall_name):
                existing = normalization.get(name)
                if existing is not None and existing != identity:
                    raise ValueError(
                        f"mechanical raw definition conflicts with "
                        f"manifest identity for {name!r}"
                    )
                normalization[name] = identity
            paired_cdecl.add(cdecl_name)
            paired_fastcall.add(fastcall_name)
        elif stable_count != 1:
            raise ValueError(
                f"unpaired /Gd raw definition {cdecl_name!r}"
            )

    for fastcall_name in fastcall_names:
        if fastcall_name in paired_fastcall:
            continue
        cdecl_name = _profile_definition_counterpart(
            fastcall_name,
            from_convention="Gr",
        )
        local_key = _local_definition_key(
            fastcall_name,
            convention="Gr",
        )
        if cdecl_name is None and local_key is None:
            continue
        local_candidates = (
            cdecl_local_by_key.get(local_key, [])
            if local_key is not None
            else []
        )
        counterpart_count = (
            len(local_candidates)
            if local_key is not None
            else cdecl_counts.get(cdecl_name, 0)
        )
        if local_candidates:
            cdecl_name = local_candidates[0]
        stable_count = cdecl_counts.get(fastcall_name, 0)
        if counterpart_count and stable_count:
            raise ValueError(
                f"ambiguous /Gr raw definition {fastcall_name!r}: "
                "both transformed and stable /Gd definitions exist"
            )
        if counterpart_count:
            if fastcall_counts[fastcall_name] != 1 or counterpart_count != 1:
                raise ValueError(
                    f"ambiguous paired raw definition {fastcall_name!r}"
                )
        elif stable_count != 1:
            raise ValueError(
                f"unpaired /Gr raw definition {fastcall_name!r}"
            )
    return normalization


def _section_shape(coff: CoffObject, section_names: list[str], normalize: dict[str, str]) -> list[dict[str, object]]:
    requested = {name.casefold() for name in section_names}
    rows: list[dict[str, object]] = []
    for section in coff.sections:
        if section.name.casefold() not in requested:
            continue
        relocations = []
        for relocation in coff.relocations_by_section.get(section.index, ()):
            size = relocation_size(relocation.type)
            relocations.append(
                {
                    "offset": relocation.offset,
                    "type": relocation.type,
                    "target": normalize.get(relocation.symbol_name, relocation.symbol_name),
                    "addend_hex": section.raw_data[
                        relocation.offset : relocation.offset + size
                    ].hex(),
                }
            )
        rows.append(
            {
                "name": section.name,
                "bytes_hex": section.raw_data.hex(),
                "relocations": relocations,
                "characteristics": section.characteristics,
            }
        )
    return rows


def compare_objects(
    target: ZeroArgAbiTarget,
    cdecl_objects: Mapping[Path, CoffObject],
    fastcall_objects: Mapping[Path, CoffObject],
    *,
    normalization: Mapping[str, str],
) -> tuple[list[dict[str, object]], dict[str, object]]:
    expected_sources = {
        target.callee_source,
        *(caller.source for caller in target.callers),
    }
    if set(cdecl_objects) != expected_sources or set(fastcall_objects) != expected_sources:
        raise ValueError(
            "paired ABI object maps must contain exactly the selected callee "
            "and caller sources"
        )
    cdecl_callee_object = cdecl_objects[target.callee_source]
    fastcall_callee_object = fastcall_objects[target.callee_source]

    gates: list[dict[str, object]] = []

    def gate(name: str, passed: bool, actual: object) -> None:
        gates.append({"gate": name, "passed": bool(passed), "actual": actual})

    cdecl_callee = _function_shape(
        cdecl_callee_object,
        target.callee.cdecl_symbol,
        dict(normalization),
    )
    fastcall_callee = _function_shape(
        fastcall_callee_object,
        target.callee.fastcall_symbol,
        dict(normalization),
    )
    gate(
        "candidate-callee-bytes",
        cdecl_callee["bytes_hex"] == fastcall_callee["bytes_hex"],
        {"cdecl": cdecl_callee["bytes_hex"], "fastcall": fastcall_callee["bytes_hex"]},
    )
    gate(
        "candidate-plain-ret",
        cdecl_callee["plain_ret"] is True and fastcall_callee["plain_ret"] is True,
        {"cdecl": cdecl_callee["plain_ret"], "fastcall": fastcall_callee["plain_ret"]},
    )
    gate(
        "callee-relocation-semantics",
        cdecl_callee["relocations"] == fastcall_callee["relocations"],
        {"cdecl": cdecl_callee["relocations"], "fastcall": fastcall_callee["relocations"]},
    )
    gate(
        "callee-comdat-selection",
        (
            cdecl_callee["comdat"] == fastcall_callee["comdat"]
            and cdecl_callee["comdat_selection"] == fastcall_callee["comdat_selection"]
        ),
        {
            "cdecl": {
                "comdat": cdecl_callee["comdat"],
                "selection": cdecl_callee["comdat_selection"],
            },
            "fastcall": {
                "comdat": fastcall_callee["comdat"],
                "selection": fastcall_callee["comdat_selection"],
            },
        },
    )

    caller_rows: list[dict[str, object]] = []
    for index, pair in enumerate(target.callers):
        cdecl_caller = _function_shape(
            cdecl_objects[pair.source],
            pair.cdecl_symbol,
            dict(normalization),
        )
        fastcall_caller = _function_shape(
            fastcall_objects[pair.source],
            pair.fastcall_symbol,
            dict(normalization),
        )
        cdecl_direct = any(
            row["type"] == IMAGE_REL_I386_REL32 and row["target"] == target.identity
            for row in cdecl_caller["relocations"]
        )
        fastcall_direct = any(
            row["type"] == IMAGE_REL_I386_REL32 and row["target"] == target.identity
            for row in fastcall_caller["relocations"]
        )
        passed = (
            cdecl_direct
            and fastcall_direct
            and cdecl_caller["bytes_hex"] == fastcall_caller["bytes_hex"]
            and cdecl_caller["relocations"] == fastcall_caller["relocations"]
            and cdecl_caller["comdat"] == fastcall_caller["comdat"]
            and cdecl_caller["comdat_selection"] == fastcall_caller["comdat_selection"]
        )
        row = {
            "index": index,
            "source": str(pair.source.resolve()),
            "passed": passed,
            "direct_rel32": {"cdecl": cdecl_direct, "fastcall": fastcall_direct},
            "cdecl": cdecl_caller,
            "fastcall": fastcall_caller,
        }
        caller_rows.append(row)
    gate("candidate-direct-callsite-bytes-and-relocations", all(row["passed"] for row in caller_rows), caller_rows)

    source_order_rows: list[dict[str, object]] = []
    for source in cdecl_objects:
        order_normalization = _paired_raw_definition_normalization(
            cdecl_objects[source],
            fastcall_objects[source],
            normalization,
        )
        cdecl_order = _definition_order(
            cdecl_objects[source],
            order_normalization,
        )
        fastcall_order = _definition_order(
            fastcall_objects[source],
            order_normalization,
        )
        source_order_rows.append(
            {
                "source": str(source.resolve()),
                "passed": cdecl_order == fastcall_order,
                "cdecl": cdecl_order,
                "fastcall": fastcall_order,
            }
        )
    gate(
        "raw-definition-order",
        all(bool(row["passed"]) for row in source_order_rows),
        source_order_rows,
    )

    if target.eh_policy.get("kind") == "none":
        eh_actual: object = {"kind": "none", "manifest_proven": True}
        eh_passed = True
    else:
        section_names = list(target.eh_policy["sections"])
        cdecl_eh = _section_shape(
            cdecl_callee_object,
            section_names,
            dict(normalization),
        )
        fastcall_eh = _section_shape(
            fastcall_callee_object,
            section_names,
            dict(normalization),
        )
        eh_actual = {"cdecl": cdecl_eh, "fastcall": fastcall_eh}
        eh_passed = bool(cdecl_eh) and cdecl_eh == fastcall_eh
    gate("candidate-eh-shape", eh_passed, eh_actual)

    return gates, {
        "callee": {"cdecl": cdecl_callee, "fastcall": fastcall_callee},
        "callers": caller_rows,
        "raw_definition_order": source_order_rows,
    }


def run_target(
    target: ZeroArgAbiTarget,
    config: FinalBuildConfig,
    *,
    all_targets: Sequence[ZeroArgAbiTarget],
    build_root: Path,
) -> tuple[int, dict[str, object]]:
    gates = eligibility_gates(target)
    report: dict[str, object] = {
        "id": target.target_id,
        "identity": target.identity,
        "callee_source": str(target.callee_source.resolve()),
        "caller_sources": [str(row.source.resolve()) for row in target.callers],
        "authorization": "__cdecl-normalization-only",
        "grants": [],
        "eligibility_gates": gates,
    }
    if not all(bool(row["passed"]) for row in gates):
        report.update({"status": "ineligible", "success": False})
        return 1, report
    sources: list[Path] = []
    source_keys: set[str] = set()
    for source in (
        target.callee_source,
        *(caller.source for caller in target.callers),
    ):
        key = os.path.normcase(str(source.resolve()))
        if key not in source_keys:
            source_keys.add(key)
            sources.append(source)
    missing_sources = [source for source in sources if not source.is_file()]
    if missing_sources:
        report.update(
            {
                "status": "input-missing",
                "success": False,
                "diagnostic": "source does not exist: "
                + ", ".join(str(source) for source in missing_sources),
            }
        )
        return 2, report

    build_target_root = build_root / re.sub(r"[^A-Za-z0-9_.-]+", "_", target.target_id)
    builds: dict[str, list[dict[str, object]]] = {}
    objects: dict[str, dict[Path, CoffObject]] = {}
    for convention in ("Gd", "Gr"):
        profile_builds: list[dict[str, object]] = []
        profile_objects: dict[Path, CoffObject] = {}
        builds[convention] = profile_builds
        objects[convention] = profile_objects
        for source in sources:
            returncode, obj, stdout_log, stderr_log = _compile_one(
                config,
                source,
                convention=convention,
                build_root=build_target_root,
            )
            profile_builds.append(
                {
                    "source": str(source.resolve()),
                    "returncode": returncode,
                    "object": str(obj.resolve()),
                    "stdout_log": str(stdout_log.resolve()),
                    "stderr_log": str(stderr_log.resolve()),
                }
            )
            if returncode != 0 or not obj.is_file():
                report.update(
                    {
                        "status": "compile-failed",
                        "success": False,
                        "builds": builds,
                    }
                )
                return returncode or 1, report
            profile_objects[source] = CoffObject.from_path(obj)

    comparison_gates, details = compare_objects(
        target,
        objects["Gd"],
        objects["Gr"],
        normalization=manifest_symbol_normalization(all_targets),
    )
    success = all(bool(row["passed"]) for row in comparison_gates)
    report.update(
        {
            "status": "passed" if success else "diverged",
            "success": success,
            "builds": builds,
            "comparison_gates": comparison_gates,
            "comparison": details,
            "conclusion": (
                "Mechanical /Gd-/Gr equivalence proven for this zero-argument identity; "
                "this authorizes only explicit __cdecl normalization and does not establish original ABI."
                if success
                else "Mechanical /Gd-/Gr equivalence was not proven."
            ),
        }
    )
    return (0 if success else 1), report


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Freshly compile one manifest-owned zero-argument identity under paired VC5 /Gd and /Gr "
            "profiles and fail closed unless every mechanical equivalence gate passes. The exact "
            "manifest schema requires callee_source and non-empty caller rows containing source, "
            "cdecl_symbol, and fastcall_symbol."
        )
    )
    parser.add_argument("--target", required=True)
    parser.add_argument("--build-root", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--json", action="store_true")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        targets = load_zeroarg_targets(args.manifest)
        matches = [row for row in targets if row.target_id == args.target]
        if len(matches) != 1:
            raise ValueError(
                f"{args.manifest}: --target {args.target!r} must resolve exactly once in "
                "zeroarg_abi_equivalence"
            )
        config = load_config(args.manifest)
        build_root = _safe_build_root(args.build_root)
        rc, report = run_target(
            matches[0],
            config,
            all_targets=targets,
            build_root=build_root,
        )
        payload = {
            "report_version": 1,
            "kind": "vc5-zeroarg-abi-equivalence",
            "success": rc == 0,
            "manifest": str(args.manifest.resolve()),
            "build_root": str(build_root),
            "identities": [report],
            "acceptance_mutated": False,
            "original_abi_claimed": False,
        }
        if args.json:
            print(json.dumps(payload, indent=2))
        else:
            print("PASS" if rc == 0 else "FAIL")
            print(report.get("conclusion", report.get("status", "")))
        return rc
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        if args.json:
            print(
                json.dumps(
                    {
                        "report_version": 1,
                        "kind": "vc5-zeroarg-abi-equivalence",
                        "success": False,
                        "error": str(exc),
                        "acceptance_mutated": False,
                        "original_abi_claimed": False,
                    },
                    indent=2,
                )
            )
        else:
            print(f"error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
