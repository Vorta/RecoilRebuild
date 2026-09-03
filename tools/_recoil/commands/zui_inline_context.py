from __future__ import annotations

import argparse
from contextlib import redirect_stdout
import hashlib
import io
import json
from pathlib import Path
import sys
from typing import Any, Mapping, Sequence

from _recoil.commands.call_contract_verify import (
    DEFAULT_PROGRESS,
    _canonical_zui_relocation_names,
    _compile_slice_candidates,
    _resolve_slice,
)
from _recoil.commands.vc5_verify import (
    DEFAULT_VC5_ENV,
    load_manifest,
    with_compiler_profile_override,
)
from _recoil.lib.progress import ProgressDocument, ProgressError, normalize_address
from _recoil.lib.tooling import REPO_ROOT, configure_stdio


ZUI_LOAD_FROM_ZRD_ADDRESS = "0x4b59f0"
CHECK_TOGGLE_LOAD_FROM_ZRD_ADDRESS = "0x4b7340"
HUD_PANEL_INSERT_FRAGMENT = "?insert@?$vector@PAUHudUi"
HUD_PANEL_VECTOR_FRAGMENT = "?$vector@PAUHudUiPanel"
EXPECTED_APPEND_SITE_COUNT = 8
def _hex_offset(value: int) -> str:
    return f"0x{value:x}"


def _matching_insert_symbol(value: str) -> bool:
    return HUD_PANEL_INSERT_FRAGMENT in value


def _artifact_digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _relocation_digest(relocations: Any) -> str:
    payload = json.dumps(
        [
            [row.offset, row.type, row.symbol_name]
            for row in relocations
        ],
        ensure_ascii=True,
        separators=(",", ":"),
    ).encode("utf-8")
    return _artifact_digest(payload)


def _canonical_relocation_digest(relocations: Any) -> str:
    names = _canonical_zui_relocation_names(relocations)
    payload = json.dumps(
        [
            [row.offset, row.type, name]
            for row, name in zip(relocations, names)
        ],
        ensure_ascii=True,
        separators=(",", ":"),
    ).encode("utf-8")
    return _artifact_digest(payload)


def _definition_summary(symbol: str, helper: Any) -> dict[str, Any]:
    direct_calls: list[dict[str, Any]] = []
    for relocation in helper.relocations:
        instruction_offset = relocation.offset - 1
        opcode = (
            helper.data[instruction_offset]
            if 0 <= instruction_offset < len(helper.data)
            else None
        )
        if opcode != 0xE8:
            continue
        direct_calls.append(
            {
                "instruction_offset": _hex_offset(instruction_offset),
                "operand_offset": _hex_offset(relocation.offset),
                "relocation_type": relocation.type,
                "symbol": relocation.symbol_name,
            }
        )
    return {
        "symbol": symbol,
        "extent": _hex_offset(len(helper.data)),
        "body_sha256": _artifact_digest(helper.data),
        "body_hex": helper.data.hex(),
        "section_size": _hex_offset(helper.section_size),
        "section_is_comdat": helper.section_is_comdat,
        "comdat_selection": helper.comdat_selection,
        "section_external_functions": list(
            helper.section_external_functions
        ),
        "source_provenance": helper.source_provenance,
        "relocations": [
            {
                "offset": _hex_offset(row.offset),
                "type": row.type,
                "symbol": row.symbol_name,
            }
            for row in helper.relocations
        ],
        "direct_calls": direct_calls,
    }


def summarize_zui_inline_context(
    candidate: Any,
    *,
    append_site_count: int = EXPECTED_APPEND_SITE_COUNT,
) -> dict[str, Any]:
    caller = candidate.caller_definition
    if caller is None:
        raise ValueError("zUI inline-context probe has no complete caller definition")

    direct_symbol_calls: list[dict[str, Any]] = []
    direct_calls: list[dict[str, Any]] = []
    for relocation in caller.relocations:
        instruction_offset = relocation.offset - 1
        opcode = (
            caller.data[instruction_offset]
            if 0 <= instruction_offset < len(caller.data)
            else None
        )
        row = {
            "instruction_offset": _hex_offset(instruction_offset),
            "operand_offset": _hex_offset(relocation.offset),
            "relocation_type": relocation.type,
            "symbol": relocation.symbol_name,
            "opcode": None if opcode is None else _hex_offset(opcode),
            "is_direct_call": opcode == 0xE8,
        }
        if opcode == 0xE8:
            direct_symbol_calls.append(row)
        if _matching_insert_symbol(relocation.symbol_name):
            direct_calls.append(row)

    providers: list[dict[str, Any]] = []
    direct_local_helpers: list[dict[str, Any]] = []
    local_vector_helpers: list[dict[str, Any]] = []
    direct_symbols = {row["symbol"] for row in direct_symbol_calls}
    for symbol, helper in sorted(candidate.tu_local_function_definitions.items()):
        if _matching_insert_symbol(symbol):
            summary = _definition_summary(symbol, helper)
            providers.append({
                key: value
                for key, value in summary.items()
                if key not in {"body_sha256", "body_hex", "direct_calls"}
            })
        if symbol in direct_symbols:
            direct_local_helpers.append(_definition_summary(symbol, helper))
        if symbol in direct_symbols and HUD_PANEL_VECTOR_FRAGMENT in symbol:
            local_vector_helpers.append(_definition_summary(symbol, helper))

    direct_count = sum(row["is_direct_call"] is True for row in direct_calls)
    return {
        "caller_symbol": caller.symbol,
        "caller_extent": _hex_offset(len(caller.data)),
        "caller_body_sha256": _artifact_digest(caller.data),
        "caller_relocation_count": len(caller.relocations),
        "caller_relocations_sha256": _relocation_digest(caller.relocations),
        "caller_canonical_relocations_sha256": (
            _canonical_relocation_digest(caller.relocations)
        ),
        "direct_symbol_calls": direct_symbol_calls,
        "append_site_count": append_site_count,
        "direct_insert_call_count": direct_count,
        "expanded_insert_site_count": append_site_count - direct_count,
        "direct_insert_calls": direct_calls,
        "insert_provider_definitions": providers,
        "direct_local_helper_definitions": direct_local_helpers,
        "direct_local_vector_helper_definitions": local_vector_helpers,
    }


def compact_zui_inline_summary(summary: Mapping[str, Any]) -> dict[str, Any]:
    """Retain receipt coordinates while bounding routine diagnostic output."""

    result = dict(summary)
    direct_symbol_calls = result.pop("direct_symbol_calls", [])
    result["direct_symbol_call_count"] = len(direct_symbol_calls)
    for key in (
        "direct_local_helper_definitions",
        "direct_local_vector_helper_definitions",
    ):
        definitions = result.get(key, [])
        result[key] = [
            {
                "symbol": row["symbol"],
                "extent": row["extent"],
                "body_sha256": row["body_sha256"],
                "section_size": row["section_size"],
                "section_is_comdat": row["section_is_comdat"],
                "comdat_selection": row["comdat_selection"],
                "section_external_functions": row.get(
                    "section_external_functions", []
                ),
                "source_provenance": row["source_provenance"],
                "relocation_count": len(row.get("relocations", [])),
                "direct_call_count": len(row.get("direct_calls", [])),
            }
            for row in definitions
        ]
    return result


def summarize_zui_local_helper_graph(candidate: Any) -> dict[str, Any]:
    """Report one selected caller and its finite reachable local COMDAT graph.

    This is diagnostic-only candidate evidence.  It deliberately publishes no
    retail identity and performs no normalization; the call-contract verifier
    remains the sole acceptance path.
    """

    caller = candidate.caller_definition
    if caller is None:
        raise ValueError("zUI local-helper probe has no complete caller definition")
    definitions = candidate.tu_local_function_definitions
    direct_relocations = [
        row
        for row in caller.relocations
        if 0 < row.offset - caller.section_start <= len(caller.data)
        and caller.data[row.offset - caller.section_start - 1]
        in {0xE8, 0xE9}
    ]
    pending = [
        row.symbol_name
        for row in direct_relocations
        if row.symbol_name in definitions
    ]
    reached: set[str] = set()
    summaries: list[dict[str, Any]] = []
    while pending:
        symbol = pending.pop(0)
        if symbol in reached:
            continue
        helper = definitions.get(symbol)
        if helper is None:
            continue
        reached.add(symbol)
        summary = _definition_summary(symbol, helper)
        summaries.append(summary)
        pending.extend(
            row["symbol"]
            for row in summary["direct_calls"]
            if row["symbol"] in definitions
            and row["symbol"] not in reached
        )
    return {
        "caller_symbol": caller.symbol,
        "caller_extent": _hex_offset(len(caller.data)),
        "caller_section_start": _hex_offset(caller.section_start),
        "caller_body_sha256": _artifact_digest(caller.data),
        "caller_relocation_count": len(caller.relocations),
        "caller_relocations_sha256": _relocation_digest(caller.relocations),
        "caller_canonical_relocations_sha256": (
            _canonical_relocation_digest(caller.relocations)
        ),
        "caller_relocations": [
            {
                "section_offset": _hex_offset(row.offset),
                "body_offset": _hex_offset(row.offset - caller.section_start),
                "type": row.type,
                "symbol": row.symbol_name,
            }
            for row in caller.relocations
        ],
        "direct_symbol_calls": [
            {
                "instruction_offset": _hex_offset(
                    row.offset - caller.section_start - 1
                ),
                "operand_offset": _hex_offset(
                    row.offset - caller.section_start
                ),
                "relocation_type": row.type,
                "symbol": row.symbol_name,
                "form": (
                    "call"
                    if caller.data[row.offset - caller.section_start - 1]
                    == 0xE8
                    else "tail"
                ),
                "target_is_tu_local_function": row.symbol_name in definitions,
            }
            for row in direct_relocations
        ],
        "reachable_local_helper_count": len(summaries),
        "reachable_local_helper_definitions": summaries,
    }


def compact_zui_local_helper_graph(summary: Mapping[str, Any]) -> dict[str, Any]:
    result = dict(summary)
    result.pop("caller_relocations", None)
    result["direct_symbol_call_count"] = len(
        result.pop("direct_symbol_calls", [])
    )
    definitions = result.get("reachable_local_helper_definitions", [])
    result["reachable_local_helper_definitions"] = [
        {
            "symbol": row["symbol"],
            "extent": row["extent"],
            "body_sha256": row["body_sha256"],
            "section_size": row["section_size"],
            "section_is_comdat": row["section_is_comdat"],
            "comdat_selection": row["comdat_selection"],
            "section_external_functions": row.get(
                "section_external_functions", []
            ),
            "source_provenance": row["source_provenance"],
            "relocation_count": len(row.get("relocations", [])),
            "direct_call_count": len(row.get("direct_calls", [])),
        }
        for row in definitions
    ]
    return result


def _single_address_slice(
    document: ProgressDocument,
    address: str,
) -> dict[str, Any]:
    normalized = normalize_address(address)
    for raw_slice in document.authored_call_contract_slices():
        addresses = raw_slice.get("addresses")
        if not isinstance(addresses, list):
            continue
        normalized_addresses = [normalize_address(str(row)) for row in addresses]
        if normalized not in normalized_addresses:
            continue
        slice_row = _resolve_slice(document, str(raw_slice.get("id", "")))
        index = [normalize_address(str(row)) for row in slice_row["addresses"]].index(
            normalized
        )
        symbol_id = str(slice_row["symbol_ids"][index])
        symbol = document.collection("symbols").get(symbol_id)
        if not isinstance(symbol, Mapping):
            raise ProgressError(
                f"zUI inline-context probe references unknown symbol {symbol_id}"
            )
        block_id = str(symbol.get("physical_block_id", ""))
        block = document.collection("physical_blocks").get(block_id)
        facts = (
            block.get("accepted_order_facts")
            if isinstance(block, Mapping)
            else None
        )
        target_id = facts.get("target_id") if isinstance(facts, Mapping) else None
        if not isinstance(target_id, str) or not target_id:
            raise ProgressError(
                f"zUI inline-context probe has no accepted order target for {symbol_id}"
            )
        return {
            "id": f"diagnostic:zui-inline-context:{normalized}",
            "ordinal": 0,
            "start": normalized,
            "end": normalized,
            "body_count": 1,
            "symbol_ids": [symbol_id],
            "addresses": [normalized],
            "target_ids": [target_id],
            "physical_block_ids": [block_id],
            "source_paths": list(slice_row.get("source_paths", [])),
            "selection_mode": "diagnostic-single-address",
        }
    raise ProgressError(
        f"zUI inline-context address {normalized} is not in an authored call-contract slice"
    )


def _probe_slice(
    document: ProgressDocument,
    addresses: Sequence[str] | None = None,
) -> dict[str, Any]:
    selected_addresses = list(addresses or (
        ZUI_LOAD_FROM_ZRD_ADDRESS,
        CHECK_TOGGLE_LOAD_FROM_ZRD_ADDRESS,
    ))
    rows = [
        _single_address_slice(document, address)
        for address in selected_addresses
    ]
    target_ids = list(
        dict.fromkeys(
            target_id
            for row in rows
            for target_id in row["target_ids"]
        )
    )
    if len(target_ids) != 1:
        raise ProgressError(
            "zUI inline-context callers do not share one exact registered target"
        )
    return {
        "id": "diagnostic:zui-inline-context:callers",
        "ordinal": 0,
        "start": rows[0]["start"],
        "end": rows[-1]["end"],
        "body_count": len(rows),
        "symbol_ids": [row["symbol_ids"][0] for row in rows],
        "addresses": [row["addresses"][0] for row in rows],
        "target_ids": target_ids,
        "physical_block_ids": list(
            dict.fromkeys(
                block_id
                for row in rows
                for block_id in row["physical_block_ids"]
            )
        ),
        "source_paths": list(
            dict.fromkeys(
                path for row in rows for path in row.get("source_paths", [])
            )
        ),
        "selection_mode": "diagnostic-zui-inline-context",
    }


def _require_fresh_live_build_root(build_root: Path) -> Path:
    resolved = build_root.resolve()
    live_root = (REPO_ROOT / "build" / "live-validation").resolve()
    if live_root != resolved and live_root not in resolved.parents:
        raise ProgressError(
            "zUI inline-context --build-root must be below build/live-validation"
        )
    if resolved.exists():
        raise ProgressError(
            f"zUI inline-context --build-root must be absent: {resolved}"
        )
    return resolved


def _profile_override_targets(
    document: ProgressDocument,
    slice_row: Mapping[str, Any],
    *,
    compiler_profile: str,
    allow_disqualified_profile: bool,
) -> dict[str, Any] | None:
    if not compiler_profile:
        return None
    targets: dict[str, Any] = {}
    for target_id in slice_row["target_ids"]:
        target_row = document.collection("verification_targets").get(target_id)
        registration = (
            target_row.get("registration")
            if isinstance(target_row, Mapping)
            else None
        )
        manifest_path = (
            registration.get("manifest_path")
            if isinstance(registration, Mapping)
            else None
        )
        if not isinstance(manifest_path, str) or not manifest_path:
            raise ProgressError(
                f"zUI inline-context target {target_id} has no manifest path"
            )
        target = load_manifest(REPO_ROOT / manifest_path)
        targets[target_id] = with_compiler_profile_override(
            target,
            compiler_profile,
            allow_disqualified_profile=allow_disqualified_profile,
        )
    return targets


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Compile the authentic registered zui_widgets.cpp target and report "
            "HudUiZrdWidget::LoadFromZrd vector-insert call/COMDAT context."
        )
    )
    parser.add_argument("--progress", type=Path, default=DEFAULT_PROGRESS)
    parser.add_argument("--build-root", type=Path, required=True)
    parser.add_argument("--vc5-env", type=Path, default=DEFAULT_VC5_ENV)
    parser.add_argument("--compiler-profile", default="")
    parser.add_argument("--allow-disqualified-profile", action="store_true")
    parser.add_argument(
        "--address",
        action="append",
        default=[],
        help=(
            "select one zUI authored-body address; repeat to inspect several "
            "callers in one authentic target compile"
        ),
    )
    parser.add_argument(
        "--summary",
        action="store_true",
        help="omit full caller call rows and helper body hex from JSON output",
    )
    parser.add_argument("--json", action="store_true")
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    diagnostics = io.StringIO()
    try:
        build_root = _require_fresh_live_build_root(args.build_root)
        document = ProgressDocument.load(args.progress)
        selected_addresses = tuple(
            dict.fromkeys(normalize_address(value) for value in args.address)
        )
        slice_row = _probe_slice(
            document,
            selected_addresses or None,
        )
        if args.allow_disqualified_profile and not args.compiler_profile:
            raise ProgressError(
                "--allow-disqualified-profile requires --compiler-profile"
            )
        preloaded_targets = _profile_override_targets(
            document,
            slice_row,
            compiler_profile=args.compiler_profile,
            allow_disqualified_profile=args.allow_disqualified_profile,
        )
        with redirect_stdout(diagnostics) if args.json else io.StringIO():
            candidates = _compile_slice_candidates(
                document,
                slice_row,
                build_root=build_root,
                vc5_env=args.vc5_env,
                preloaded_targets=preloaded_targets,
            )
        if selected_addresses:
            caller_rows = []
            for address in selected_addresses:
                selected_candidate = candidates.get(address)
                if selected_candidate is None:
                    raise ProgressError(
                        "zUI inline-context compile did not produce selected "
                        f"caller {address}"
                    )
                summary = summarize_zui_local_helper_graph(selected_candidate)
                if args.summary:
                    summary = compact_zui_local_helper_graph(summary)
                caller_rows.append({"address": address, **summary})
            result = {
                "kind": "zui-inline-context-diagnostic",
                "passed": True,
                "nonaccepting": True,
                "acceptance_eligible": False,
                "candidate_expected_truth": False,
                "addresses": list(selected_addresses),
                "target_ids": list(slice_row["target_ids"]),
                "compiler_profile_override": args.compiler_profile or None,
                "build_root": str(build_root),
                "callers": caller_rows,
            }
            if diagnostics.getvalue():
                print(diagnostics.getvalue(), end="", file=sys.stderr)
            print(json.dumps(result, indent=2))
            return 0

        candidate = candidates.get(ZUI_LOAD_FROM_ZRD_ADDRESS)
        if candidate is None:
            raise ProgressError(
                "zUI inline-context compile did not produce the selected caller"
            )
        summary = summarize_zui_inline_context(candidate)
        related_candidate = candidates.get(CHECK_TOGGLE_LOAD_FROM_ZRD_ADDRESS)
        if related_candidate is None:
            raise ProgressError(
                "zUI inline-context compile did not produce the authentic emitter caller"
            )
        related_summary = summarize_zui_inline_context(
            related_candidate,
            append_site_count=2,
        )
        if args.summary:
            summary = compact_zui_inline_summary(summary)
            related_summary = compact_zui_inline_summary(related_summary)
        result = {
            "kind": "zui-inline-context-diagnostic",
            "passed": True,
            "nonaccepting": True,
            "acceptance_eligible": False,
            "candidate_expected_truth": False,
            "address": ZUI_LOAD_FROM_ZRD_ADDRESS,
            "target_ids": list(slice_row["target_ids"]),
            "compiler_profile_override": args.compiler_profile or None,
            "build_root": str(build_root),
            **summary,
            "related_callers": [
                {
                    "address": CHECK_TOGGLE_LOAD_FROM_ZRD_ADDRESS,
                    **related_summary,
                }
            ],
        }
    except (OSError, ProgressError, RuntimeError, ValueError) as exc:
        result = {
            "kind": "zui-inline-context-diagnostic",
            "passed": False,
            "nonaccepting": True,
            "acceptance_eligible": False,
            "candidate_expected_truth": False,
            "address": ZUI_LOAD_FROM_ZRD_ADDRESS,
            "first_divergence": {
                "kind": "diagnostic-blocked",
                "message": str(exc),
            },
        }
    if diagnostics.getvalue():
        print(diagnostics.getvalue(), end="", file=sys.stderr)
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        print(json.dumps(result, indent=2))
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
