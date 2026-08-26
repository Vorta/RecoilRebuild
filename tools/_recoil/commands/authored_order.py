from __future__ import annotations

from copy import deepcopy
import argparse
import json
import os
from pathlib import Path
import re
import sys
import tempfile
from typing import Any, Mapping

from _recoil.commands.vc5_verify import (
    function_authored_order_gate,
    function_authored_relative_order_gate,
    load_manifest,
    load_manifests,
)
from _recoil.lib.progress import (
    AUTHORED_ORDER_DIMENSIONS,
    AUTHORED_PIPELINE_CLASSES,
    DEFAULT_PROGRESS_PATH,
    PIPELINE_CLASSES,
    ProgressDocument,
    ProgressError,
    address_value,
    is_accepted_state,
    normalize_address,
    logical_alias_authored_order_blocking,
    logical_alias_authored_order_gate,
    logical_alias_authored_order_role,
    symbol_authored_order_gate,
    symbol_has_logical_address_group,
    symbol_authored_order_role,
    symbol_logical_aliases,
)
from _recoil.lib.authored_icf import require_valid_authored_icf_groups
from _recoil.lib.tooling import REPO_ROOT, display_path, repo_path


DEFAULT_MANIFEST_DIR = REPO_ROOT / "tools" / "vc5_verify_targets"
DEFAULT_FINAL_BUILD_MANIFEST = REPO_ROOT / "tools" / "_recoil" / "config" / "vc5_final_build.json"
CANONICAL_MFC_INCLUDE_ROOT = Path("D:/Recoil Project/Compiler/VC5SP3/VC/MFC/INCLUDE")
FUNCTION_KINDS = {"function", "provider-function", "compiler-function"}


class ScaffoldGap(ProgressError):
    def __init__(self, kind: str, message: str) -> None:
        super().__init__(message)
        self.kind = kind


def _logical_alias_rows(symbol: Mapping[str, Any]) -> list[dict[str, Any]]:
    """Expose progress logical aliases as identity-bearing row dictionaries."""
    return [
        {**alias, "identity_key": identity_key}
        for identity_key, alias in symbol_logical_aliases(symbol)
    ]


def _function_rows(document: ProgressDocument, block_id: str) -> list[tuple[str, dict[str, Any]]]:
    block = document.collection("physical_blocks").get(block_id)
    if not isinstance(block, dict):
        raise ProgressError(f"unknown physical block {block_id}")
    rows: list[tuple[str, dict[str, Any]]] = []
    for symbol_id in block.get("contribution_ids", []):
        symbol = document.collection("symbols").get(symbol_id)
        if isinstance(symbol, dict) and symbol.get("kind") in FUNCTION_KINDS:
            rows.append((symbol_id, symbol))
    return sorted(rows, key=lambda item: (address_value(item[1].get("address", "0x0")), item[0]))


def resolve_block(document: ProgressDocument, selector: str | None) -> tuple[str, dict[str, Any]]:
    blocks = document.collection("physical_blocks")
    if selector is None:
        pipeline = document.pipeline("recoil")
        selector = str(pipeline.get("physical_block_id") or pipeline.get("cursor") or "")
    block = blocks.get(selector)
    if isinstance(block, dict):
        return selector, block
    try:
        point = address_value(selector)
    except ProgressError as exc:
        raise ProgressError(f"no physical block matches {selector!r}") from exc
    matches = [
        (block_id, row)
        for block_id, row in blocks.items()
        if isinstance(row, dict)
        and address_value(row.get("start", "0x0")) <= point < address_value(row.get("end_exclusive", "0x0"))
    ]
    if len(matches) != 1:
        raise ProgressError(f"expected one physical block for {selector!r}, found {len(matches)}")
    return matches[0]


def _target_coverage(
    document: ProgressDocument,
    symbol_id: str,
    logical_alias: Mapping[str, Any] | None = None,
) -> list[dict[str, Any]]:
    result: list[dict[str, Any]] = []
    for target_id, target in document.collection("verification_targets").items():
        if not isinstance(target, dict) or symbol_id not in target.get("symbol_ids", []):
            continue
        registration = target.get("registration", {})
        if not isinstance(registration, dict):
            registration = {}
        symbol_address = document.collection("symbols").get(symbol_id, {}).get("address")
        logical_object_symbol = (
            str(logical_alias.get("object_symbol", "")) if logical_alias is not None else ""
        )

        def entry_covers_authored_gate(item: dict[str, Any]) -> bool:
            functions = item.get("functions")
            if isinstance(functions, list) and functions:
                return any(
                    isinstance(function, dict)
                    and function.get("address") == symbol_address
                    and function.get("authored_order_gate", True) is True
                    and (
                        not logical_object_symbol
                        or function.get("symbol") == logical_object_symbol
                    )
                    for function in functions
                )
            if logical_object_symbol:
                return False
            return symbol_address in item.get("function_addresses", [])

        object_scopes = sorted(
            {
                str(item.get("order_scope", "full"))
                for item in registration.get("translation_unit_function_order", [])
                if isinstance(item, dict)
                and entry_covers_authored_gate(item)
            }
        )
        linked_scopes = sorted(
            {
                str(item.get("order_scope", "full"))
                for item in registration.get("linked_function_intervals", [])
                if isinstance(item, dict)
                and entry_covers_authored_gate(item)
            }
        )
        result.append(
            {
                "target_id": target_id,
                "name": target.get("name", ""),
                "object_order_scopes": object_scopes,
                "linked_order_scopes": linked_scopes,
            }
        )
    return result


def census(document: ProgressDocument, selector: str | None = None) -> dict[str, Any]:
    require_valid_authored_icf_groups(document.data)
    block_id, block = resolve_block(document, selector)
    rows = _function_rows(document, block_id)
    counts = {pipeline_class: 0 for pipeline_class in sorted(PIPELINE_CLASSES)}
    logical_counts = {pipeline_class: 0 for pipeline_class in sorted(PIPELINE_CLASSES)}
    symbols: list[dict[str, Any]] = []
    authored_identities: list[dict[str, Any]] = []
    unresolved_logical_identity_keys: list[str] = []
    for symbol_id, symbol in rows:
        pipeline_class = str(symbol.get("pipeline_class", "unresolved"))
        counts[pipeline_class] = counts.get(pipeline_class, 0) + 1
        coverage = _target_coverage(document, symbol_id)
        logical_aliases: list[dict[str, Any]] = []
        for alias in _logical_alias_rows(symbol):
            alias_pipeline_class = str(alias.get("pipeline_class", "unresolved"))
            logical_counts[alias_pipeline_class] = logical_counts.get(alias_pipeline_class, 0) + 1
            alias_coverage = _target_coverage(document, symbol_id, alias)
            alias_gate = logical_alias_authored_order_gate(alias)
            alias_blocking = logical_alias_authored_order_blocking(alias)
            identity = {
                "identity_key": alias["identity_key"],
                "physical_symbol_id": symbol_id,
                "address": symbol.get("address"),
                "object_symbol": alias.get("object_symbol", ""),
                "original_name": alias.get("original_name", ""),
                "original_name_status": alias.get("original_name_status"),
                "source_owner_status": alias.get("source_owner_status"),
                "owner_id": alias.get("owner_id"),
                "fold_status": alias.get("fold_status"),
                "pipeline_class": alias_pipeline_class,
                "authored_order_role": logical_alias_authored_order_role(alias),
                "authored_order_gate": alias_gate,
                "authored_order_blocking": alias_blocking,
                "verification_targets": alias_coverage,
                "authored_object_target_covered": any(
                    "authored" in row["object_order_scopes"] for row in alias_coverage
                ),
                "authored_linked_target_covered": any(
                    "authored" in row["linked_order_scopes"] for row in alias_coverage
                ),
            }
            logical_aliases.append(identity)
            if alias_blocking:
                unresolved_logical_identity_keys.append(alias["identity_key"])
            if alias_gate:
                authored_identities.append(identity)
        physical_gate = symbol_authored_order_gate(symbol)
        physical_row = {
            "id": symbol_id,
            "address": symbol.get("address"),
            "end_exclusive": symbol.get("end_exclusive"),
            "name": symbol.get("navigation_name", ""),
            "pipeline_class": pipeline_class,
            "authored_order_role": symbol_authored_order_role(symbol),
            "authored_order_gate": physical_gate,
            "physical_authored_order_gate": physical_gate,
            "icf_address_group": deepcopy(symbol.get("icf_address_group")),
            "linked_address_group": deepcopy(symbol.get("linked_address_group")),
            "logical_address_group_valid": symbol_has_logical_address_group(symbol),
            "logical_aliases": logical_aliases,
            "semantic_span_ids": list(symbol.get("semantic_span_ids", [])),
            "verification_targets": coverage,
            "authored_object_target_covered": any("authored" in row["object_order_scopes"] for row in coverage),
            "authored_linked_target_covered": any("authored" in row["linked_order_scopes"] for row in coverage),
        }
        symbols.append(physical_row)
        if physical_gate:
            authored_identities.append(
                {
                    "identity_key": symbol_id,
                    "physical_symbol_id": symbol_id,
                    "address": symbol.get("address"),
                    "pipeline_class": pipeline_class,
                    "authored_order_role": symbol_authored_order_role(symbol),
                    "authored_order_gate": True,
                    "authored_order_blocking": False,
                    "authored_object_target_covered": physical_row["authored_object_target_covered"],
                    "authored_linked_target_covered": physical_row["authored_linked_target_covered"],
                }
            )
    authored_state = block.get("order", {}).get("authored", {})
    target_names = block.get("order_targets", {})
    return {
        "schema_version": document.data.get("schema_version"),
        "revision": document.revision,
        "pipeline": document.pipeline("recoil"),
        "block_id": block_id,
        "binary": block.get("binary"),
        "start": block.get("start"),
        "end_exclusive": block.get("end_exclusive"),
        "agent_source_path": block.get("agent_source_path"),
        "semantic_span_ids": list(block.get("semantic_span_ids", [])),
        "order_targets": deepcopy(target_names) if isinstance(target_names, dict) else {},
        "classification_counts": counts,
        "logical_classification_counts": logical_counts,
        "unresolved_symbol_ids": [
            row["id"]
            for row in symbols
            if row["pipeline_class"] == "unresolved"
            and not row["logical_address_group_valid"]
        ],
        "unresolved_logical_identity_keys": unresolved_logical_identity_keys,
        "authored_projection": [row["identity_key"] for row in authored_identities],
        "authored_identity_projection": authored_identities,
        "authored_addresses": [row["address"] for row in authored_identities],
        "authored_object_coverage_complete": all(
            row["authored_object_target_covered"] for row in authored_identities
        ),
        "authored_linked_coverage_complete": all(
            row["authored_linked_target_covered"] for row in authored_identities
        ),
        "authored_order_dimensions": {
            dimension: deepcopy(authored_state.get(dimension)) for dimension in AUTHORED_ORDER_DIMENSIONS
        },
        "authored_order_accepted": all(
            is_accepted_state(authored_state.get(dimension)) for dimension in AUTHORED_ORDER_DIMENSIONS
        ),
        "symbols": symbols,
    }


def _function_index(manifest_dir: Path) -> dict[str, list[dict[str, Any]]]:
    index: dict[str, list[dict[str, Any]]] = {}

    def add(target, function, *, source_from: str, order_scope: str, entry_index: int) -> None:
        index.setdefault(function.address, []).append(
            {
                "target": target.name,
                "manifest": display_path(target.manifest_path),
                "entry_index": entry_index,
                "source_from": source_from,
                "order_scope": order_scope,
                "address": function.address,
                "symbol": function.symbol,
                "symbol_regex": function.symbol_regex,
                "name": function.name,
                "pipeline_class": function.pipeline_class,
                "authored_order_role": function.authored_order_role,
                "authored_order_gate": function_authored_order_gate(function),
                "authored_relative_order_gate": function_authored_relative_order_gate(function),
                "required_presence": function.required_presence,
                "full_order_gate": function.full_order_gate,
                "logical_identity_key": function.logical_identity_key,
                "icf_fold_status": function.icf_fold_status,
            }
        )

    for target in load_manifests(manifest_dir, enforce_source_policy=False):
        for function in target.functions:
            add(target, function, source_from=target.source_from, order_scope="identity", entry_index=-1)
        for entry_index, entry in enumerate(target.translation_unit_function_order):
            for function in entry.functions:
                add(
                    target,
                    function,
                    source_from=entry.source_from,
                    order_scope=entry.order_scope,
                    entry_index=entry_index,
                )
        for interval_index, interval in enumerate(target.linked_function_intervals):
            for function in interval.functions:
                add(
                    target,
                    function,
                    source_from=target.source_from,
                    order_scope=interval.order_scope,
                    entry_index=interval_index,
                )
    return index


def _decorated_identity_index(manifest_dir: Path) -> dict[str, list[dict[str, Any]]]:
    index: dict[str, list[dict[str, Any]]] = {}

    def add(target_name: str, manifest: Path, function, source: str = "") -> None:
        if not function.symbol:
            return
        index.setdefault(function.address, []).append(
            {
                "target": target_name,
                "manifest": display_path(manifest),
                "symbol": function.symbol,
                "name": function.name,
                "source_from": source,
                "identity_provenance": "manifest-exact-symbol",
            }
        )

    for target in load_manifests(manifest_dir, enforce_source_policy=False):
        for function in target.functions:
            add(target.name, target.manifest_path, function, target.source_from)
        for entry in target.translation_unit_function_order:
            for function in entry.functions:
                add(target.name, target.manifest_path, function, entry.source_from)
        for interval in target.linked_function_intervals:
            if interval.predecessor is not None:
                add(target.name, target.manifest_path, interval.predecessor)
            for function in interval.functions:
                add(target.name, target.manifest_path, function)
            add(target.name, target.manifest_path, interval.successor)
    return index


def _merge_identity_indexes(
    *indexes: Mapping[str, list[dict[str, Any]]],
) -> dict[str, list[dict[str, Any]]]:
    merged: dict[str, list[dict[str, Any]]] = {}
    for index in indexes:
        for address, candidates in index.items():
            merged.setdefault(address, []).extend(deepcopy(candidates))
    return merged


def _exact_decorated_identity(
    identity_index: Mapping[str, list[dict[str, Any]]],
    address: str,
    *,
    gap_kind: str,
) -> dict[str, Any]:
    candidates = identity_index.get(address, [])
    by_symbol: dict[str, list[dict[str, Any]]] = {}
    for item in candidates:
        symbol = str(item.get("symbol", ""))
        if symbol:
            by_symbol.setdefault(symbol, []).append(item)
    if len(by_symbol) != 1:
        raise ScaffoldGap(
            gap_kind,
            f"authored-order scaffold requires one exact decorated identity at {address}; found {len(by_symbol)}",
        )
    symbol, matching = next(iter(by_symbol.items()))
    matching = sorted(
        matching,
        key=lambda item: (
            str(item.get("identity_provenance", "")),
            str(item.get("target", "")),
            str(item.get("manifest", "")),
            str(item.get("source_from", "")),
            str(item.get("name", "")),
        ),
    )
    chosen = deepcopy(matching[0])
    chosen["symbol"] = symbol
    chosen["identity_evidence"] = [
        {
            key: item.get(key, "")
            for key in (
                "identity_provenance",
                "target",
                "manifest",
                "source_from",
            )
            if item.get(key)
        }
        for item in matching
    ]
    return chosen


def _compile_context(path: Path = DEFAULT_FINAL_BUILD_MANIFEST) -> dict[str, Any]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise ScaffoldGap("compile_host", f"cannot load final-build compile context {path}: {exc}") from exc
    if not isinstance(data, dict):
        raise ScaffoldGap("compile_host", f"final-build compile context {path} must be an object")
    sources = data.get("sources")
    canonical_mfc = data.get("canonical_mfc")
    if not isinstance(sources, list) or not all(isinstance(item, str) and item for item in sources):
        raise ScaffoldGap("compile_host", f"final-build compile context {path} has invalid sources")
    if not isinstance(canonical_mfc, dict) or not isinstance(canonical_mfc.get("include_root"), str):
        raise ScaffoldGap("compile_host", f"final-build compile context {path} has no canonical MFC include root")
    include_root = Path(canonical_mfc["include_root"])
    if os.path.normcase(str(include_root.resolve())) != os.path.normcase(str(CANONICAL_MFC_INCLUDE_ROOT.resolve())):
        raise ScaffoldGap(
            "compile_host",
            f"final-build compile context must use only canonical MFC include root {CANONICAL_MFC_INCLUDE_ROOT}",
        )
    return {
        "path": display_path(path.resolve()),
        "sources": {display_path(repo_path(item).resolve()) for item in sources},
        "canonical_mfc_include_root": str(include_root).replace("\\", "/"),
    }


def _boundary_function_rows(document: ProgressDocument, binary: str) -> list[dict[str, Any]]:
    return sorted(
        [
            row
            for row in document.collection("symbols").values()
            if isinstance(row, dict) and row.get("binary") == binary and row.get("kind") in FUNCTION_KINDS
        ],
        key=lambda row: address_value(row.get("address", "0x0")),
    )


def _manifest_function(symbol: Mapping[str, Any], identity: Mapping[str, str]) -> dict[str, Any]:
    return {
        "address": str(symbol.get("address")),
        "symbol": identity["symbol"],
        "name": str(symbol.get("navigation_name") or identity.get("name") or identity["symbol"]),
        "pipeline_class": str(symbol.get("pipeline_class", "unresolved")),
        "authored_order_role": symbol_authored_order_role(symbol),
        "required_presence": True,
        "full_order_gate": True,
    }


def scaffold(
    document: ProgressDocument,
    selector: str | None,
    *,
    manifest_dir: Path = DEFAULT_MANIFEST_DIR,
    final_build_manifest: Path = DEFAULT_FINAL_BUILD_MANIFEST,
    function_index: Mapping[str, list[dict[str, Any]]] | None = None,
    identity_index: Mapping[str, list[dict[str, Any]]] | None = None,
    compile_context: Mapping[str, Any] | None = None,
) -> dict[str, Any]:
    require_valid_authored_icf_groups(document.data)
    block_id, block = resolve_block(document, selector)
    manifest_identity_index = (
        deepcopy(identity_index)
        if identity_index is not None
        else _decorated_identity_index(manifest_dir)
    )
    if function_index is not None:
        compatibility_index: dict[str, list[dict[str, Any]]] = {}
        for address, candidates in function_index.items():
            for candidate in candidates:
                if candidate.get("symbol") and not candidate.get("symbol_regex"):
                    item = deepcopy(candidate)
                    item["identity_provenance"] = "manifest-exact-symbol"
                    compatibility_index.setdefault(address, []).append(item)
        manifest_identity_index = _merge_identity_indexes(
            manifest_identity_index,
            compatibility_index,
        )
    identity_index = manifest_identity_index
    compile_context = compile_context or _compile_context(final_build_manifest)
    rows = _function_rows(document, block_id)
    unresolved = [
        symbol_id
        for symbol_id, symbol in rows
        if symbol.get("pipeline_class") == "unresolved"
        and not symbol_has_logical_address_group(symbol)
    ]
    unresolved.extend(
        str(alias["identity_key"])
        for _symbol_id, symbol in rows
        for alias in _logical_alias_rows(symbol)
        if logical_alias_authored_order_blocking(alias)
    )
    if unresolved:
        raise ScaffoldGap(
            "unresolved",
            "authored-order scaffold requires explicit classifications: " + ", ".join(unresolved),
        )
    functions: list[dict[str, Any]] = []
    ambiguities: list[dict[str, Any]] = []
    targets: set[str] = set()
    identity_evidence: list[dict[str, Any]] = []
    source_from = str(block.get("agent_source_path") or block.get("source_path") or "")
    if not source_from:
        raise ScaffoldGap("compile_host", f"physical block {block_id} has no compile host")
    for symbol_id, symbol in rows:
        aliases = _logical_alias_rows(symbol)
        gating_aliases = [
            alias for alias in aliases if logical_alias_authored_order_gate(alias)
        ]
        if gating_aliases:
            for alias in gating_aliases:
                functions.append(
                    {
                        "address": str(symbol.get("address")),
                        "symbol": str(alias.get("object_symbol")),
                        "name": str(
                            alias.get("original_name")
                            or alias.get("identity_key")
                        ),
                        "logical_identity_key": str(alias["identity_key"]),
                        "icf_fold_status": str(alias.get("fold_status")),
                        "pipeline_class": str(alias.get("pipeline_class")),
                        "authored_order_role": logical_alias_authored_order_role(alias),
                        "required_presence": True,
                        "full_order_gate": False,
                    }
                )
                identity_evidence.append(
                    {
                        "address": str(symbol.get("address")),
                        "symbol": str(alias.get("object_symbol")),
                        "logical_identity_key": str(alias["identity_key"]),
                        "identity_evidence": [
                            {
                                "identity_provenance": "tracker-logical-address-group",
                                "physical_symbol_id": symbol_id,
                            }
                        ],
                    }
                )
            continue
        if not symbol_authored_order_gate(symbol):
            continue
        address = str(symbol.get("address"))
        try:
            candidate = _exact_decorated_identity(
                identity_index,
                address,
                gap_kind="decorated_identity",
            )
        except ScaffoldGap:
            candidates = identity_index.get(address, [])
            unique_symbols = sorted(
                {
                    str(row.get("symbol", ""))
                    for row in candidates
                    if row.get("symbol")
                }
            )
            ambiguities.append(
                {
                    "symbol_id": symbol_id,
                    "address": address,
                    "candidate_count": len(unique_symbols),
                    "candidates": deepcopy(candidates),
                }
            )
            continue
        targets.add(str(candidate["target"]))
        functions.append(
            {
                "address": address,
                "symbol": candidate["symbol"],
                "name": candidate["name"],
                "pipeline_class": symbol.get("pipeline_class"),
                "authored_order_role": symbol_authored_order_role(symbol),
                "required_presence": True,
                "full_order_gate": True,
            }
        )
        identity_evidence.append(
            {
                "address": address,
                "symbol": candidate["symbol"],
                "identity_evidence": deepcopy(candidate.get("identity_evidence", [])),
            }
        )
    if ambiguities:
        raise ScaffoldGap(
            "decorated_identity",
            "authored-order scaffold cannot infer decorated identities; resolve ambiguity for "
            + ", ".join(item["address"] for item in ambiguities)
        )
    if not any(
        function.get("authored_order_role") in {"authored-body", "authored-lifecycle-body"}
        for function in functions
    ):
        raise ScaffoldGap(
            "zero_authored",
            "authored-order scaffold has no authored/authored-lifecycle functions; use authored-order-skip",
        )
    compile_sources = compile_context.get("sources")
    if not isinstance(compile_sources, set) or display_path(repo_path(source_from).resolve()) not in compile_sources:
        raise ScaffoldGap(
            "compile_host",
            f"authored-order scaffold compile host is not in the final-build source list: {source_from}",
        )
    binary = str(block.get("binary", ""))
    boundary_rows = _boundary_function_rows(document, binary)
    block_start = normalize_address(block.get("start", ""))
    successor_address = normalize_address(block.get("end_exclusive", ""))
    successor_rows = [
        row for row in boundary_rows if row.get("address") == successor_address
    ]
    if len(successor_rows) != 1:
        raise ScaffoldGap(
            "successor",
            f"authored-order scaffold requires exactly one successor at {successor_address}",
        )
    successor_identity = _exact_decorated_identity(identity_index, successor_address, gap_kind="successor")
    successor = _manifest_function(successor_rows[0], successor_identity)
    identity_evidence.append(
        {
            "address": successor_address,
            "symbol": successor_identity["symbol"],
            "boundary_role": "successor",
            "identity_evidence": deepcopy(successor_identity.get("identity_evidence", [])),
        }
    )

    binary_record = document.collection("binaries").get(binary, {})
    text_start = normalize_address(binary_record.get("text", {}).get("start", ""))
    predecessor: dict[str, Any]
    if block_start == text_start:
        predecessor = {"predecessor_section_boundary": {"section": ".text", "address": block_start}}
    else:
        predecessor_rows = [row for row in boundary_rows if address_value(row.get("address", "0x0")) < address_value(block_start)]
        if not predecessor_rows:
            raise ScaffoldGap("predecessor", f"authored-order scaffold has no mechanical predecessor before {block_start}")
        predecessor_row = predecessor_rows[-1]
        predecessor_address = str(predecessor_row.get("address"))
        predecessor_identity = _exact_decorated_identity(
            identity_index,
            predecessor_address,
            gap_kind="predecessor",
        )
        predecessor = {"predecessor": _manifest_function(predecessor_row, predecessor_identity)}
        identity_evidence.append(
            {
                "address": predecessor_address,
                "symbol": predecessor_identity["symbol"],
                "boundary_role": "predecessor",
                "identity_evidence": deepcopy(predecessor_identity.get("identity_evidence", [])),
            }
        )

    name = f"authored_order_{block_start[2:]}_{successor_address[2:]}_candidate"
    interval_name = f"authored_{block_start[2:]}_{successor_address[2:]}"
    manifest = {
        "name": name,
        "description": (
            f"Generated fail-closed authored-order candidate for {block_id}; reviewed registration is still required."
        ),
        "target_binary": binary,
        "retail_start": block_start,
        "retail_end_exclusive": successor_address,
        "source_filename": Path(source_from).name,
        "source_from": source_from,
        "compile_context_from": str(compile_context.get("path", "")),
        "check_translation_unit_function_order": True,
        "translation_unit_function_order": [
            {
                "source_from": source_from,
                "order_scope": "authored",
                "functions": functions,
            }
        ],
        "linked_function_intervals": [
            {
                "name": interval_name,
                "order_scope": "authored",
                "retail_start": block_start,
                "retail_end_exclusive": successor_address,
                **predecessor,
                "functions": functions,
                "successor": successor,
            }
        ],
        "authored_order_scaffold": {
            "version": 1,
            "block_id": block_id,
            "agent_source_path": str(block.get("agent_source_path") or block.get("source_path") or ""),
            "physical_compile_host": source_from,
            "canonical_mfc_include_root": str(compile_context.get("canonical_mfc_include_root", "")),
            "tracker_revision": document.revision,
        },
    }
    return {
        "kind": "authored-order-manifest-scaffold",
        "read_only": True,
        "block_id": block_id,
        "retail_start": block.get("start"),
        "retail_end_exclusive": block.get("end_exclusive"),
        "agent_source_path": str(block.get("agent_source_path") or block.get("source_path") or ""),
        "source_from": source_from,
        "canonical_mfc_include_root": compile_context.get("canonical_mfc_include_root"),
        "identity_provenance": identity_evidence,
        "existing_targets": sorted(targets),
        "manifest_candidate": manifest,
        "limits": [
            "Rows reuse registered exact decorated identities; logical ICF aliases use their independently evidenced exact object symbols.",
            "Identity provenance does not select the physical compile host; the block agent_source_path selects that host and the final-build source list validates it.",
            "The predecessor and successor reuse exact decorated identities already present in reviewed manifests.",
            "Cofolded logical aliases gate presence independently but carry no invented internal linked order.",
            "No owner, block, provider, tier, or acceptance fact is inferred.",
        ],
    }


def write_manifest_candidate(path: Path, payload: Mapping[str, Any]) -> None:
    manifest = payload.get("manifest_candidate")
    if not isinstance(manifest, dict):
        raise ProgressError("authored-order scaffold payload has no manifest candidate")
    path.parent.mkdir(parents=True, exist_ok=True)
    rendered = json.dumps(manifest, ensure_ascii=False, indent=2, sort_keys=True) + "\n"
    with tempfile.NamedTemporaryFile(
        mode="w",
        encoding="utf-8",
        newline="\n",
        prefix=f".{path.name}.",
        suffix=".tmp",
        dir=path.parent,
        delete=False,
    ) as handle:
        temporary = Path(handle.name)
        handle.write(rendered)
    try:
        load_manifest(temporary, enforce_source_policy=True)
        os.replace(temporary, path)
    finally:
        if temporary.exists():
            temporary.unlink()


def sweep(
    document: ProgressDocument,
    *,
    from_current: bool,
    manifest_dir: Path = DEFAULT_MANIFEST_DIR,
    final_build_manifest: Path = DEFAULT_FINAL_BUILD_MANIFEST,
) -> dict[str, Any]:
    if not from_current:
        raise ProgressError("authored-order sweep requires --from-current")
    pipeline = document.pipeline("recoil")
    cursor = address_value(pipeline.get("authored_order_prefix_end") or pipeline.get("cursor") or "0x0")
    function_index = _function_index(manifest_dir)
    identity_index = _decorated_identity_index(manifest_dir)
    compile_context = _compile_context(final_build_manifest)
    rows: list[dict[str, Any]] = []
    gap_counts: dict[str, int] = {}
    for block_id, block in document._blocks_for_binary("recoil"):
        if address_value(block.get("start", "0x0")) < cursor:
            continue
        try:
            payload = scaffold(
                document,
                block_id,
                manifest_dir=manifest_dir,
                final_build_manifest=final_build_manifest,
                function_index=function_index,
                identity_index=identity_index,
                compile_context=compile_context,
            )
            rows.append(
                {
                    "block_id": block_id,
                    "start": block.get("start"),
                    "end_exclusive": block.get("end_exclusive"),
                    "agent_source_path": block.get("agent_source_path") or block.get("source_path"),
                    "ready": True,
                    "target_name": payload["manifest_candidate"]["name"],
                    "gaps": [],
                }
            )
        except ScaffoldGap as exc:
            gap_counts[exc.kind] = gap_counts.get(exc.kind, 0) + 1
            rows.append(
                {
                    "block_id": block_id,
                    "start": block.get("start"),
                    "end_exclusive": block.get("end_exclusive"),
                    "agent_source_path": block.get("agent_source_path") or block.get("source_path"),
                    "ready": False,
                    "target_name": "",
                    "gaps": [{"kind": exc.kind, "message": str(exc)}],
                }
            )
    return {
        "kind": "authored-order-scaffold-sweep",
        "read_only": True,
        "binary": "recoil",
        "from_current": True,
        "cursor": normalize_address(cursor),
        "tracker_revision": document.revision,
        "block_count": len(rows),
        "ready_count": sum(1 for row in rows if row["ready"]),
        "gap_count": sum(1 for row in rows if not row["ready"]),
        "gap_counts": dict(sorted(gap_counts.items())),
        "blocks": rows,
        "limits": [
            "This sweep performs mechanical scaffold validation only and writes nothing.",
            "It makes no source-owner, physical-block, order, provider, lifecycle, tier, or acceptance recommendation.",
        ],
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Fail-closed authored-order manifest scaffolding and census.")
    subparsers = parser.add_subparsers(dest="command", required=True)
    scaffold_parser = subparsers.add_parser("scaffold")
    scaffold_parser.add_argument("selector", nargs="?")
    scaffold_parser.add_argument("--progress", type=Path, default=DEFAULT_PROGRESS_PATH)
    scaffold_parser.add_argument("--manifest-dir", type=Path, default=DEFAULT_MANIFEST_DIR)
    scaffold_parser.add_argument("--final-build-manifest", type=Path, default=DEFAULT_FINAL_BUILD_MANIFEST)
    scaffold_parser.add_argument("--output", type=Path)
    scaffold_parser.add_argument("--json", action="store_true")
    sweep_parser = subparsers.add_parser("sweep")
    sweep_parser.add_argument("--progress", type=Path, default=DEFAULT_PROGRESS_PATH)
    sweep_parser.add_argument("--manifest-dir", type=Path, default=DEFAULT_MANIFEST_DIR)
    sweep_parser.add_argument("--final-build-manifest", type=Path, default=DEFAULT_FINAL_BUILD_MANIFEST)
    sweep_parser.add_argument("--from-current", action="store_true")
    sweep_parser.add_argument("--json", action="store_true")
    args = parser.parse_args(argv)
    try:
        document = ProgressDocument.load(args.progress)
        if args.command == "scaffold":
            payload = scaffold(
                document,
                args.selector,
                manifest_dir=args.manifest_dir,
                final_build_manifest=args.final_build_manifest,
            )
            if args.output is not None:
                write_manifest_candidate(args.output, payload)
                payload["read_only"] = False
                payload["output"] = str(args.output.resolve())
        else:
            payload = sweep(
                document,
                from_current=args.from_current,
                manifest_dir=args.manifest_dir,
                final_build_manifest=args.final_build_manifest,
            )
        print(json.dumps(payload, ensure_ascii=False, indent=2, sort_keys=True))
        return 0
    except (OSError, ValueError, ProgressError) as exc:
        print(f"authored-order: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
