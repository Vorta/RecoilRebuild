"""Recoil call-contract cli evidence and checks."""

from __future__ import annotations

import argparse
import io
import json
import sys
import time
from contextlib import redirect_stdout
from copy import deepcopy
from pathlib import Path
from typing import Any, Mapping, Sequence

from _recoil.call_contract import candidate_session as _cc_candidate_session
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import comparison as _cc_comparison
from _recoil.call_contract import errors as _cc_errors
from _recoil.call_contract import reporting as _cc_reporting
from _recoil.call_contract import session as _cc_session
from _recoil.call_contract import source as _cc_source
from _recoil.commands.call_contract_diagnostics import (
    CALL_CONTRACT_DIAGNOSTIC_WINDOW_MAX,
    build_call_contract_divergence_diagnostic,
)
from _recoil.commands.vc5_verify import DEFAULT_VC5_ENV
from _recoil.lib.binja import DEFAULT_BRIDGE_URL, BridgeError
from _recoil.lib.progress import (
    CALL_CONTRACT_CONTRACT_VERSION,
    ProgressDocument,
    ProgressError,
)
from _recoil.lib.tooling import configure_stdio


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Freshly compile one deterministic authored-body selection and compare its "
            "static invocation contracts directly with retail Binary Ninja evidence."
        )
    )
    selection = parser.add_mutually_exclusive_group(required=True)
    selection.add_argument("--slice")
    selection.add_argument("--target")
    parser.add_argument("--all-authored-bodies", action="store_true")
    parser.add_argument(
        "--collect-all-divergences",
        dest="collect_all_divergences",
        action="store_true",
        help=(
            "evaluate every selected caller and emit exhaustive per-body "
            "outcomes; required by incremental verification acceptance"
        ),
    )
    parser.add_argument(
        "--diagnostic-window",
        type=_diagnostic_window_size,
        metavar="N",
        help=(
            "emit a diagnostic-only exact alignment of at most N expected "
            "and N candidate calls beginning at the unchanged first "
            "divergence (1..128); does not change traversal, pass/fail, "
            "exit status, or acceptance eligibility"
        ),
    )
    parser.add_argument("--progress", type=Path, default=_cc_catalog.DEFAULT_PROGRESS)
    parser.add_argument("--build-root", type=Path, required=True)
    parser.add_argument("--vc5-env", type=Path, default=DEFAULT_VC5_ENV)
    parser.add_argument("--bridge-url", default=DEFAULT_BRIDGE_URL)
    parser.add_argument(
        "--memory-trace-file",
        type=Path,
        metavar="ABSOLUTE_PATH",
        help=(
            "diagnostic-only: exclusively create a flushed JSONL private-memory "
            "trace under build/live-validation and outside the disposable "
            "--build-root; disabled when omitted"
        ),
    )
    parser.add_argument("--json", action="store_true")
    parser.add_argument(
        "--summary",
        action="store_true",
        help=(
            "with --json, emit only status, selection, timing, and the first "
            "typed divergence; validation and exit status remain unchanged"
        ),
    )
    return parser


def _diagnostic_window_size(value: str) -> int:
    try:
        parsed = int(value, 10)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(
            "diagnostic window must be an integer from 1 through 128"
        ) from exc
    if not 1 <= parsed <= CALL_CONTRACT_DIAGNOSTIC_WINDOW_MAX:
        raise argparse.ArgumentTypeError(
            "diagnostic window must be from 1 through 128"
        )
    return parsed


def _blocked_call_contract_result(
    document: ProgressDocument | None,
    *,
    slice_id: str | None,
    target_id: str | None = None,
    all_authored_bodies: bool = False,
    error: Exception,
) -> dict[str, Any]:
    target_mode = target_id is not None
    result: dict[str, Any] = {
        "kind": (
            "authored-call-contract-target-convergence-result"
            if target_mode
            else "authored-call-contract-live-result"
        ),
        "contract_version": CALL_CONTRACT_CONTRACT_VERSION,
        "closure_contract_version": 2,
        "slice_id": slice_id,
        "passed": False,
        "first_divergence": {"kind": "verifier-blocked", "message": str(error)},
        "candidate_expected_truth": False,
        "all_caller_divergences_collected": False,
        "timings_ms": _cc_reporting._empty_call_contract_timings_ms(),
    }
    if target_mode:
        result.update(
            {
                "target_id": target_id,
                "all_authored_bodies": all_authored_bodies,
                "acceptance_eligible": False,
                "nonaccepting": True,
                "acceptance_route": None,
                "selected_target_compile_count": 0,
                "compiled_target_ids": [],
            }
        )
    if isinstance(error, _cc_errors.CallContractBodyError):
        result["first_divergence"].update(
            {
                "symbol_id": error.symbol_id,
                "address": error.address,
                "side": error.side,
            }
        )
    if document is None:
        return result
    try:
        slice_row = (
            _cc_candidate_session._resolve_target_all_authored_bodies(document, target_id)
            if target_mode and target_id is not None and all_authored_bodies
            else _cc_candidate_session._resolve_slice(document, str(slice_id))
        )
        dependency_paths = _cc_source.source_dependency_paths(document, slice_row)
        signatures = _cc_source.file_dependency_states(dependency_paths)
    except (OSError, ProgressError, RuntimeError, ValueError):
        return result
    result.update(
        {
            "body_count": slice_row["body_count"],
            "symbol_ids": slice_row["symbol_ids"],
            "target_ids": slice_row["target_ids"],
            "physical_block_ids": slice_row["physical_block_ids"],
            "source_paths": slice_row["source_paths"],
            "dependency_paths": dependency_paths,
            "dependency_states_before": signatures,
            "dependency_states_after": signatures,
            "source_changed_during_validation": False,
            "expected_contracts": {},
            "candidate_contracts": {},
            "candidate_inline_absence_proofs": {},
            "expected_truth": "retail-binary-ninja-plus-reviewed-tracker-identities",
        }
    )
    return result


def _compact_call_contract_result(
    result: Mapping[str, Any],
) -> dict[str, Any]:
    """Project one full live result to a stable terminal-sized report."""

    retained = (
        "kind",
        "contract_version",
        "closure_contract_version",
        "slice_id",
        "target_id",
        "all_authored_bodies",
        "body_count",
        "passed",
        "acceptance_eligible",
        "nonaccepting",
        "candidate_expected_truth",
        "source_changed_during_validation",
        "first_divergence",
        "timings_ms",
    )
    compact = {
        key: deepcopy(result[key])
        for key in retained
        if key in result
    }
    caller_divergences = result.get("caller_divergences")
    if isinstance(caller_divergences, Sequence) and not isinstance(
        caller_divergences, (str, bytes)
    ):
        compact["caller_divergence_count"] = len(caller_divergences)
        divergence_keys = (
            "symbol_id",
            "address",
            "kind",
            "side",
            "message",
            "ordinal",
            "expected",
            "candidate",
        )
        compact["caller_divergences"] = [
            {
                key: deepcopy(row[key])
                for key in divergence_keys
                if isinstance(row, Mapping) and key in row
            }
            for row in caller_divergences
        ]
    return compact


def _direct_cli_target_census_options(
    *,
    target_id: str | None,
    all_authored_bodies: bool,
    collect_all_divergences: bool = False,
) -> dict[str, bool]:
    """Enable exhaustive diagnostics only for the nonaccepting target route."""

    if target_id is not None and all_authored_bodies:
        return {
            "collect_all_divergences": True,
            "compile_definition_closure": False,
        }
    return (
        {"collect_all_divergences": True}
        if collect_all_divergences
        else {}
    )


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    memory_trace = _cc_reporting._CallContractMemoryTrace.from_environment()
    total_started = time.perf_counter()
    diagnostics = io.StringIO()
    document: ProgressDocument | None = None
    diagnostic_comparison_context: dict[str, Any] | None = (
        {} if args.diagnostic_window is not None else None
    )
    try:
        if args.summary and not args.json:
            raise ProgressError("--summary requires --json")
        if args.memory_trace_file is not None:
            memory_trace = _cc_reporting._CallContractMemoryTrace.from_trace_file(
                args.memory_trace_file,
                build_root=args.build_root,
            )
        memory_trace.emit("cli-start")
        if args.json:
            # VC5 helpers intentionally print useful compile transcripts. Keep
            # JSON stdout machine-readable while preserving that transcript on
            # stderr for direct live invocations.
            with redirect_stdout(diagnostics):
                document = ProgressDocument.load(args.progress)
                result = _cc_session.live_call_contract_result(
                    document=document,
                    slice_id=args.slice,
                    target_id=args.target,
                    all_authored_bodies=args.all_authored_bodies,
                    build_root=args.build_root,
                    vc5_env=args.vc5_env,
                    bridge_url=args.bridge_url,
                    _memory_trace=memory_trace,
                    **_direct_cli_target_census_options(
                        target_id=args.target,
                        all_authored_bodies=args.all_authored_bodies,
                        collect_all_divergences=args.collect_all_divergences,
                    ),
                    **(
                        {
                            "_diagnostic_comparison_context":
                            diagnostic_comparison_context
                        }
                        if diagnostic_comparison_context is not None
                        else {}
                    ),
                )
        else:
            document = ProgressDocument.load(args.progress)
            result = _cc_session.live_call_contract_result(
                document=document,
                slice_id=args.slice,
                target_id=args.target,
                all_authored_bodies=args.all_authored_bodies,
                build_root=args.build_root,
                vc5_env=args.vc5_env,
                bridge_url=args.bridge_url,
                _memory_trace=memory_trace,
                **_direct_cli_target_census_options(
                    target_id=args.target,
                    all_authored_bodies=args.all_authored_bodies,
                    collect_all_divergences=args.collect_all_divergences,
                ),
                **(
                    {
                        "_diagnostic_comparison_context":
                        diagnostic_comparison_context
                    }
                    if diagnostic_comparison_context is not None
                    else {}
                ),
            )
    except (BridgeError, OSError, ProgressError, RuntimeError, ValueError) as exc:
        memory_trace.emit(
            "verification-error",
            error_type=type(exc).__name__,
        )
        result = _blocked_call_contract_result(
            document,
            slice_id=args.slice,
            target_id=args.target,
            all_authored_bodies=args.all_authored_bodies,
            error=exc,
        )
    timings_ms = result.get("timings_ms")
    if not isinstance(timings_ms, dict):
        timings_ms = _cc_reporting._empty_call_contract_timings_ms()
        result["timings_ms"] = timings_ms
    timings_ms["total"] = max(
        float(timings_ms.get("total", 0.0)),
        round((time.perf_counter() - total_started) * 1000.0, 3),
    )
    base_passed = bool(result["passed"])
    output_result = result
    if args.diagnostic_window is not None:
        output_result = dict(result)
        output_result["diagnostics"] = {
            "call_contract_divergence": (
                build_call_contract_divergence_diagnostic(
                    base_result=result,
                    comparison_context=diagnostic_comparison_context,
                    rows_per_side=args.diagnostic_window,
                    comparison_rows_equal=(
                        _cc_comparison._diagnostic_call_contract_rows_equal
                    ),
                )
            )
        }
    if args.summary:
        output_result = _compact_call_contract_result(output_result)
    if args.json:
        compile_diagnostics = diagnostics.getvalue()
        if compile_diagnostics:
            print(compile_diagnostics, file=sys.stderr, end="")
        memory_trace.emit(
            "serialization-start",
            result_key_count=len(output_result),
            expected_result_count=len(
                output_result.get("expected_contracts", {})
            ),
            candidate_result_count=len(
                output_result.get("candidate_contracts", {})
            ),
            divergence_count=len(
                output_result.get("caller_divergences", [])
            ),
        )
        serialized_result = json.dumps(
            output_result,
            indent=2,
            ensure_ascii=False,
        )
        memory_trace.emit(
            "serialization-complete",
            serialized_character_count=len(serialized_result),
        )
        print(serialized_result)
    else:
        memory_trace.emit(
            "serialization-start",
            result_key_count=len(output_result),
            output_format="text",
        )
        print(
            "authored call-contract PASS"
            if result["passed"]
            else f"authored call-contract FAIL: {result.get('first_divergence')}"
        )
        if args.diagnostic_window is not None:
            print(
                "call-contract diagnostic: "
                + json.dumps(
                    output_result["diagnostics"][
                        "call_contract_divergence"
                    ],
                    ensure_ascii=False,
                    sort_keys=True,
                )
            )
        memory_trace.emit(
            "serialization-complete",
            output_format="text",
        )
    memory_trace.close()
    return 0 if base_passed else 1
