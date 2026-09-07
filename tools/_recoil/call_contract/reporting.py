"""Recoil call-contract reporting evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import errors as _cc_errors

if TYPE_CHECKING:
    pass

import ctypes
import json
import os
import sys
import time
from copy import deepcopy
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable, Mapping, Sequence, TextIO

from _recoil.lib.call_contract_evidence import json_evidence_value
from _recoil.lib.call_contract_generations import (
    EXPECTED_FACT_SCHEMA_VERSION,
    current_generations,
)
from _recoil.lib.cpp_definition_closure import (
    DecodedCallableIdentity,
    decode_vc5_zero_argument_callable_identity,
)
from _recoil.lib.progress import ProgressDocument, ProgressError, normalize_address


def _direct_bytes(value: bytes | bytearray | memoryview) -> bytes:
    """Return exact bytes for direct comparison or lossless diagnostics."""

    return bytes(value)


def _build_call_contract_expected_fact_row(
    *,
    symbol_id: str,
    address: str,
    calls: Sequence[Mapping[str, Any]] | None = None,
    expected_contract: Sequence[Mapping[str, Any]] | None = None,
    **facts: Any,
) -> dict[str, Any]:
    """Build one typed retail row without deriving a content identity."""

    return json_evidence_value({
        "schema_version": EXPECTED_FACT_SCHEMA_VERSION,
        **current_generations(),
        "symbol_id": symbol_id,
        "address": address,
        "calls": [
            row
            for row in (calls if calls is not None else expected_contract or ())
        ],
        **facts,
    })


def _r4564_candidate_extra_helper_is_caller_source(
    *,
    caller_address: str,
    caller_end_exclusive: str,
    error: _cc_errors.CandidateCallTargetBridgeError,
) -> bool:
    """Classify only the three reviewed candidate-only inlining helpers."""

    if not error.call_site_unique or error.call_site_address is None:
        return False
    try:
        key = (
            normalize_address(caller_address),
            normalize_address(caller_end_exclusive),
            normalize_address(error.call_site_address),
            error.decorated_identity,
        )
    except (TypeError, ValueError, ProgressError):
        return False
    return key in _cc_catalog._R4564_CALLER_SOURCE_EXTRA_HELPERS


def _decode_candidate_dependent_callable_identity(
    decorated_identity: str,
) -> DecodedCallableIdentity | None:
    """Decode generic zero-argument or exact reviewed r4564 identities."""

    generic = decode_vc5_zero_argument_callable_identity(decorated_identity)
    if generic is not None:
        return generic
    reviewed = _cc_catalog._R4564_DEPENDENT_CALLABLE_IDENTITIES.get(
        str(decorated_identity)
    )
    if reviewed is None:
        return None
    callable_key, calling_convention, parameter_bytes, return_shape = reviewed
    return DecodedCallableIdentity(
        decorated_identity=str(decorated_identity),
        callable_key=callable_key,
        calling_convention=calling_convention,
        parameter_bytes=parameter_bytes,
        return_shape=return_shape,
        identity_format="msvc-exact-reviewed-r4564",
    )


class _ProcessMemoryCountersEx(ctypes.Structure):
    """Windows PROCESS_MEMORY_COUNTERS_EX layout used by opt-in telemetry."""

    _fields_ = [
        ("cb", ctypes.c_ulong),
        ("PageFaultCount", ctypes.c_ulong),
        ("PeakWorkingSetSize", ctypes.c_size_t),
        ("WorkingSetSize", ctypes.c_size_t),
        ("QuotaPeakPagedPoolUsage", ctypes.c_size_t),
        ("QuotaPagedPoolUsage", ctypes.c_size_t),
        ("QuotaPeakNonPagedPoolUsage", ctypes.c_size_t),
        ("QuotaNonPagedPoolUsage", ctypes.c_size_t),
        ("PagefileUsage", ctypes.c_size_t),
        ("PeakPagefileUsage", ctypes.c_size_t),
        ("PrivateUsage", ctypes.c_size_t),
    ]


_CALL_CONTRACT_PROCESS_MEMORY_API: tuple[Any, Any] | None = None


_CALL_CONTRACT_PROCESS_MEMORY_API_UNAVAILABLE = False


def _call_contract_process_private_bytes() -> int | None:
    """Return current Windows private bytes without affecting verification."""

    global _CALL_CONTRACT_PROCESS_MEMORY_API
    global _CALL_CONTRACT_PROCESS_MEMORY_API_UNAVAILABLE
    if os.name != "nt" or _CALL_CONTRACT_PROCESS_MEMORY_API_UNAVAILABLE:
        return None
    if _CALL_CONTRACT_PROCESS_MEMORY_API is None:
        try:
            kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
            psapi = ctypes.WinDLL("psapi", use_last_error=True)
            kernel32.GetCurrentProcess.restype = ctypes.c_void_p
            psapi.GetProcessMemoryInfo.argtypes = [
                ctypes.c_void_p,
                ctypes.POINTER(_ProcessMemoryCountersEx),
                ctypes.c_ulong,
            ]
            psapi.GetProcessMemoryInfo.restype = ctypes.c_int
            _CALL_CONTRACT_PROCESS_MEMORY_API = (kernel32, psapi)
        except (AttributeError, OSError):
            _CALL_CONTRACT_PROCESS_MEMORY_API_UNAVAILABLE = True
            return None
    kernel32, psapi = _CALL_CONTRACT_PROCESS_MEMORY_API
    counters = _ProcessMemoryCountersEx()
    counters.cb = ctypes.sizeof(counters)
    try:
        ok = psapi.GetProcessMemoryInfo(
            kernel32.GetCurrentProcess(),
            ctypes.byref(counters),
            counters.cb,
        )
    except (AttributeError, OSError):
        return None
    return int(counters.PrivateUsage) if ok else None


@dataclass
class _CallContractMemoryTrace:
    """Disabled-by-default flushed JSONL telemetry for one live invocation."""

    enabled: bool
    stream: TextIO | None = None
    owns_stream: bool = False
    peak_private_bytes: int | None = None
    sequence: int = 0

    @classmethod
    def from_environment(cls) -> "_CallContractMemoryTrace":
        return cls(
            enabled=os.environ.get(_cc_catalog.CALL_CONTRACT_MEMORY_TRACE_ENV) == "1"
        )

    @classmethod
    def from_trace_file(
        cls,
        path: Path,
        *,
        build_root: Path,
    ) -> "_CallContractMemoryTrace":
        return cls(
            enabled=True,
            stream=_open_call_contract_memory_trace_file(
                path,
                build_root=build_root,
            ),
            owns_stream=True,
        )

    def emit(
        self,
        stage: str,
        *,
        caller_index: int | None = None,
        caller_total: int | None = None,
        symbol_id: str | None = None,
        address: str | None = None,
        **cardinalities: Any,
    ) -> None:
        if not self.enabled:
            return
        try:
            current = _call_contract_process_private_bytes()
            if current is not None:
                self.peak_private_bytes = max(
                    current,
                    self.peak_private_bytes or 0,
                )
            self.sequence += 1
            row = {
                "kind": _cc_catalog.CALL_CONTRACT_MEMORY_TRACE_KIND,
                "contract_version": (
                    _cc_catalog.CALL_CONTRACT_MEMORY_TRACE_CONTRACT_VERSION
                ),
                "sequence": self.sequence,
                "stage": str(stage),
                "caller_index": caller_index,
                "caller_total": caller_total,
                "symbol_id": symbol_id,
                "address": address,
                "current_private_bytes": current,
                "peak_private_bytes": self.peak_private_bytes,
                **cardinalities,
            }
            destination = self.stream if self.stream is not None else sys.stderr
            print(
                json.dumps(row, sort_keys=True, ensure_ascii=False),
                file=destination,
                flush=True,
            )
        except Exception:
            # Telemetry is diagnostic only and must never change semantic
            # verification, even when stderr or the platform counter fails.
            return

    def close(self) -> None:
        if not self.owns_stream or self.stream is None:
            return
        try:
            self.stream.flush()
            self.stream.close()
        except Exception:
            # Each row was already flushed.  A diagnostic close failure must
            # not change the verifier result or its stdout/stderr contract.
            pass
        finally:
            self.stream = None
            self.owns_stream = False


def _call_contract_path_is_reparse_point(path: Path) -> bool:
    try:
        stat_result = os.lstat(path)
    except OSError:
        return False
    return bool(
        getattr(stat_result, "st_file_attributes", 0)
        & 0x400  # Windows FILE_ATTRIBUTE_REPARSE_POINT
    )


def _call_contract_normalized_absolute_path(path: Path) -> Path:
    return Path(os.path.abspath(os.fspath(path)))


def _call_contract_paths_equal(left: Path, right: Path) -> bool:
    return os.path.normcase(os.fspath(left)) == os.path.normcase(
        os.fspath(right)
    )


def _call_contract_windows_name_is_reserved(name: str) -> bool:
    base = name.rstrip(" .").split(".", 1)[0].upper()
    return base in {"CON", "PRN", "AUX", "NUL"} or (
        len(base) == 4
        and base[:3] in {"COM", "LPT"}
        and base[3] in "123456789"
    )


def _open_call_contract_memory_trace_file(
    path: Path,
    *,
    build_root: Path,
) -> TextIO:
    """Create a stable trace outside the disposable target build root."""

    requested = Path(path)
    if not requested.is_absolute():
        raise ProgressError(
            "--memory-trace-file must be an absolute path"
        )
    target = _call_contract_normalized_absolute_path(requested)
    if not _call_contract_paths_equal(requested, target):
        raise ProgressError(
            "--memory-trace-file must be a normalized absolute path"
        )
    if not target.name or target.name in {".", ".."}:
        raise ProgressError(
            "--memory-trace-file must name one file"
        )
    if (
        _call_contract_windows_name_is_reserved(target.name)
        or ":" in target.name
        or target.name != target.name.rstrip(" .")
    ):
        raise ProgressError(
            "--memory-trace-file has an unsafe Windows file name"
        )
    approved_root = _call_contract_normalized_absolute_path(
        _cc_catalog.CALL_CONTRACT_MEMORY_TRACE_ROOT
    )
    try:
        resolved_approved_root = approved_root.resolve(strict=True)
    except OSError as exc:
        raise ProgressError(
            "--memory-trace-file requires the existing build/live-validation root"
        ) from exc
    if not resolved_approved_root.is_dir():
        raise ProgressError(
            "--memory-trace-file diagnostics root must be a directory"
        )
    if (
        not _call_contract_paths_equal(approved_root, resolved_approved_root)
        or approved_root.is_symlink()
        or _call_contract_path_is_reparse_point(approved_root)
    ):
        raise ProgressError(
            "--memory-trace-file diagnostics root must not be a symlink, "
            "junction, or reparse point"
        )

    parent = target.parent
    try:
        resolved_parent = parent.resolve(strict=True)
    except OSError as exc:
        raise ProgressError(
            "--memory-trace-file parent directory must already exist"
        ) from exc
    if not resolved_parent.is_dir():
        raise ProgressError(
            "--memory-trace-file parent must be a directory"
        )
    if not _call_contract_paths_equal(parent, resolved_parent):
        raise ProgressError(
            "--memory-trace-file must not traverse a symlink, junction, "
            "or reparse point"
        )
    try:
        relative_parent = resolved_parent.relative_to(resolved_approved_root)
    except ValueError as exc:
        raise ProgressError(
            "--memory-trace-file must be contained by build/live-validation"
        ) from exc
    current = resolved_approved_root
    for component in relative_parent.parts:
        current = current / component
        if current.is_symlink() or _call_contract_path_is_reparse_point(current):
            raise ProgressError(
                "--memory-trace-file must not traverse a symlink, junction, "
                "or reparse point"
            )

    root = _call_contract_normalized_absolute_path(Path(build_root))
    try:
        resolved_root = root.resolve(strict=True)
    except OSError as exc:
        raise ProgressError(
            "--memory-trace-file requires an existing build root"
        ) from exc
    if not resolved_root.is_dir():
        raise ProgressError(
            "--memory-trace-file build root must be a directory"
        )
    try:
        target.relative_to(resolved_root)
    except ValueError:
        pass
    else:
        raise ProgressError(
            "--memory-trace-file must be outside the disposable --build-root"
        )
    if target.exists() or target.is_symlink():
        raise ProgressError(
            "--memory-trace-file refuses to overwrite an existing path"
        )

    flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL
    flags |= getattr(os, "O_BINARY", 0)
    flags |= getattr(os, "O_NOINHERIT", 0)
    try:
        descriptor = os.open(target, flags, 0o600)
    except (FileExistsError, IsADirectoryError, NotADirectoryError) as exc:
        raise ProgressError(
            "--memory-trace-file target is not a new regular file"
        ) from exc
    except OSError as exc:
        raise ProgressError(
            f"--memory-trace-file could not be created: {exc}"
        ) from exc
    try:
        return os.fdopen(
            descriptor,
            "w",
            encoding="utf-8",
            newline="\n",
            buffering=1,
        )
    except Exception:
        os.close(descriptor)
        raise


def _empty_call_contract_timings_ms() -> dict[str, float]:
    """Return the stable, diagnostic-only call-contract timing schema."""

    return {key: 0.0 for key in _cc_catalog.CALL_CONTRACT_TIMING_KEYS}


def _add_elapsed_ms(
    timings_ms: dict[str, float],
    key: str,
    started_at: float,
) -> None:
    timings_ms[key] = round(
        timings_ms[key] + (time.perf_counter() - started_at) * 1000.0,
        3,
    )


def _call_contract_body_results(
    *,
    document: ProgressDocument,
    slice_row: Mapping[str, Any],
    candidate_session: Any,
    expected_by_symbol: Mapping[str, Any],
    candidate_by_symbol: Mapping[str, Any],
    caller_divergences: Sequence[Mapping[str, Any]],
    evaluated_symbol_ids: Iterable[str] = (),
) -> list[dict[str, Any]]:
    """Return bounded direct semantic results for this invocation only."""

    del document
    evaluated = {str(value) for value in evaluated_symbol_ids}
    first_divergence_by_symbol: dict[str, Mapping[str, Any]] = {}
    for row in caller_divergences:
        symbol_id = str(row.get("symbol_id", ""))
        if symbol_id and symbol_id not in first_divergence_by_symbol:
            first_divergence_by_symbol[symbol_id] = row
    results: list[dict[str, Any]] = []
    for symbol_id, raw_address in zip(slice_row["symbol_ids"], slice_row["addresses"]):
        symbol_id = str(symbol_id)
        address = normalize_address(str(raw_address))
        expected = expected_by_symbol.get(symbol_id)
        candidate_contract = candidate_by_symbol.get(symbol_id)
        divergence = first_divergence_by_symbol.get(symbol_id)
        if divergence is not None:
            status = "divergent" if divergence.get("kind") == "mismatch" else "blocked"
        elif symbol_id in evaluated and expected is not None and candidate_contract is not None:
            status = "passed"
        else:
            status = "not-evaluated"
        normalized_candidate = (
            json_evidence_value(candidate_contract)
            if candidate_contract is not None
            else None
        )
        normalized_expected = (
            _build_call_contract_expected_fact_row(
                symbol_id=symbol_id,
                address=address,
                expected_contract=expected,
                expected_truth="retail-binary-ninja-plus-reviewed-tracker-identities",
            )
            if expected is not None
            else None
        )
        results.append(
            {
                "symbol_id": symbol_id,
                "address": address,
                "target_id": str(candidate_session.target_id(address) or ""),
                "status": status,
                "comparison_passed": status == "passed",
                "divergence": deepcopy(dict(divergence)) if divergence else None,
                "expected_fact_row": normalized_expected,
                "expected_contract": (
                    json_evidence_value(expected) if expected is not None else None
                ),
                "candidate_contract": normalized_candidate,
                **current_generations(),
            }
        )
    return results
