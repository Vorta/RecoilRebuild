"""Manifest parsing used by call-contract zero-argument ABI projections."""

from __future__ import annotations

from dataclasses import dataclass
import json
import os
from pathlib import Path
import re
from typing import Any, Callable, Sequence

from _recoil.lib.coff_alias import repository_relative_path


ELIGIBLE_RETURN_CATEGORIES = {
    "void", "pointer", "integral8", "integral16", "integral32",
}
INELIGIBLE_CONTEXT_FLAGS = (
    "hidden_this", "hidden_sret", "lifecycle", "variadic", "address_taken",
    "callback", "vtable", "export", "import", "function_pointer",
)
_SYMBOL_RE = re.compile(r"^[^\s]+$")


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
    if not isinstance(value, str) or not value or not _SYMBOL_RE.fullmatch(value):
        raise ValueError(f"{label}.{key} must be a non-empty whitespace-free string")
    return value


def _source_symbol_pair(
    row: object,
    *,
    label: str,
    source_for: Callable[..., Path],
) -> SourceSymbolPair:
    if not isinstance(row, dict) or set(row) != {
        "source", "cdecl_symbol", "fastcall_symbol",
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
    configured: dict[str, Path] = {}
    for index, raw_source in enumerate(raw_sources):
        source = repository_relative_path(
            raw_source, label=f"sources[{index}]", manifest_path=path
        )
        key = os.path.normcase(str(source))
        if key in configured:
            raise ValueError(f"{path}: duplicate configured source {raw_source}")
        configured[key] = source

    def source_for(value: object, *, label: str) -> Path:
        source = repository_relative_path(value, label=label, manifest_path=path)
        result = configured.get(os.path.normcase(str(source)))
        if result is None:
            raise ValueError(f"{path}: {label} must name one configured source")
        return result

    raw_targets = data.get("zeroarg_abi_equivalence")
    if not isinstance(raw_targets, list):
        raise ValueError(f"{path}: zeroarg_abi_equivalence must be a list")
    rows: list[ZeroArgAbiTarget] = []
    seen_ids: set[str] = set()
    seen_identities: set[str] = set()
    allowed = {
        "id", "identity", "callee_source", "cdecl_symbol", "fastcall_symbol",
        "callers", "return_category", "retail_evidence", "eh_policy", "st0_policy",
    }
    for index, raw in enumerate(raw_targets):
        label = f"{path}: zeroarg_abi_equivalence[{index}]"
        if not isinstance(raw, dict) or set(raw) - allowed:
            raise ValueError(f"{label} is not a supported target object")
        target_id = _required_string(raw, "id", label=label)
        identity = _required_string(raw, "identity", label=label)
        if target_id in seen_ids or identity in seen_identities:
            raise ValueError(f"{label}: duplicate id or identity")
        seen_ids.add(target_id)
        seen_identities.add(identity)
        callers_raw = raw.get("callers")
        retail_evidence = raw.get("retail_evidence")
        eh_policy = raw.get("eh_policy")
        return_category = raw.get("return_category")
        st0_policy = raw.get("st0_policy")
        if not isinstance(callers_raw, list) or not callers_raw:
            raise ValueError(f"{label}.callers must be a non-empty list")
        if not isinstance(retail_evidence, dict) or not isinstance(eh_policy, dict):
            raise ValueError(f"{label}: evidence and EH policy must be objects")
        if not isinstance(return_category, str) or not return_category:
            raise ValueError(f"{label}.return_category must be a string")
        if st0_policy is not None and not isinstance(st0_policy, dict):
            raise ValueError(f"{label}.st0_policy must be an object")
        callers = tuple(
            _source_symbol_pair(
                item, label=f"{label}.callers[{caller_index}]", source_for=source_for
            )
            for caller_index, item in enumerate(callers_raw)
        )
        if len({(os.path.normcase(str(c.source)), c.cdecl_symbol, c.fastcall_symbol) for c in callers}) != len(callers):
            raise ValueError(f"{label}.callers contains a duplicate row")
        rows.append(ZeroArgAbiTarget(
            target_id=target_id,
            identity=identity,
            callee_source=source_for(raw.get("callee_source"), label=f"{label}.callee_source"),
            callee=SymbolPair(
                _required_string(raw, "cdecl_symbol", label=label),
                _required_string(raw, "fastcall_symbol", label=label),
            ),
            callers=callers,
            return_category=return_category,
            retail_evidence=retail_evidence,
            eh_policy=eh_policy,
            st0_policy=st0_policy,
        ))
    result = tuple(rows)
    manifest_symbol_normalization(result)
    return result


def manifest_symbol_normalization(targets: Sequence[ZeroArgAbiTarget]) -> dict[str, str]:
    pair_identities: dict[tuple[str, str], str] = {}
    symbol_bindings: dict[str, tuple[str, str]] = {}
    normalization: dict[str, str] = {}
    stable_callers: set[str] = set()
    pairs: list[tuple[SymbolPair, str]] = []
    for target in targets:
        pairs.append((target.callee, target.identity))
        for caller in target.callers:
            if caller.cdecl_symbol == caller.fastcall_symbol:
                stable_callers.add(caller.cdecl_symbol)
            else:
                key = (caller.cdecl_symbol, caller.fastcall_symbol)
                pairs.append((caller.symbols, pair_identities.setdefault(
                    key, f"manifest-zeroarg-caller-pair:{len(pair_identities)}"
                )))
    for pair, requested_identity in pairs:
        key = (pair.cdecl_symbol, pair.fastcall_symbol)
        if key[0] == key[1]:
            raise ValueError(f"ABI symbol pair must be distinct: {key[0]}")
        identity = pair_identities.setdefault(key, requested_identity)
        if identity != requested_identity:
            raise ValueError(f"ambiguous manifest ABI pair {key!r}")
        for symbol in key:
            previous = symbol_bindings.get(symbol)
            if previous is not None and previous != key:
                raise ValueError(f"ambiguous manifest ABI symbol {symbol!r}")
            symbol_bindings[symbol] = key
            normalization[symbol] = identity
    conflicts = stable_callers.intersection(normalization)
    if conflicts:
        raise ValueError(f"ambiguous stable ABI caller: {', '.join(sorted(conflicts))}")
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
        direct_ok = all(
            isinstance(call, dict)
            and isinstance(call.get("address"), str)
            and re.fullmatch(r"0x[0-9a-fA-F]+", call["address"]) is not None
            and call.get("dispatch") == "direct"
            and type(call.get("explicit_argument_count")) is int
            and call.get("explicit_argument_count") == 0
            and call.get("callee_return") == "plain-ret"
            for call in direct_calls
        )
    gate("retail-direct-noarg-plain-ret-calls", direct_ok, direct_calls)
    return_ok = target.return_category in ELIGIBLE_RETURN_CATEGORIES
    if target.return_category == "x87-float":
        policy = target.st0_policy
        return_ok = (
            isinstance(policy, dict) and policy.get("proven") is True
            and isinstance(policy.get("evidence"), str) and bool(policy["evidence"].strip())
        )
    gate("eligible-return-category", return_ok, target.return_category)
    eh_kind = target.eh_policy.get("kind")
    sections = target.eh_policy.get("sections")
    eh_ok = target.eh_policy.get("retail_proven") is True and (
        eh_kind == "none"
        or (eh_kind == "paired-sections" and isinstance(sections, list)
            and bool(sections) and all(isinstance(item, str) and item for item in sections))
    )
    gate("explicit-eh-policy", eh_ok, target.eh_policy)
    return gates


__all__ = [
    "ZeroArgAbiTarget", "eligibility_gates", "load_zeroarg_targets",
    "manifest_symbol_normalization",
]
