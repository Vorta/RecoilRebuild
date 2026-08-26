"""Contained-disabled packetless call-contract repair continuation.

No producer exists.  These narrow read helpers keep old callers fail-closed;
they never build, evaluate, allocate an output root, or access Binary Ninja.
"""

from __future__ import annotations

from typing import Any, Mapping

from _recoil.lib.progress import ProgressError


CONTINUATION_MIGRATION_KEY = "authored_call_contract_repair_continuation_disabled_v1"
CONTINUATION_PACKET_TYPE = "call-contract-repair-continuation-disabled-v1"
RETURN_PROVENANCE_FIELD = "repair_continuation_disabled"
DISABLED_REASON = (
    "packetless repair continuation is contained-disabled until a separately "
    "approved active-packet producer exists"
)


def continuation_state(*_args: Any, **_kwargs: Any) -> dict[str, Any]:
    return {
        "state": "none",
        "status": "contained-disabled",
        "active": False,
        "checkpoint": None,
        "reason": DISABLED_REASON,
    }


def continuation_snapshots_equal(*_args: Any, **_kwargs: Any) -> bool:
    return False


def validate_continuation_checkpoint(*_args: Any, **_kwargs: Any) -> bool:
    return False


def returned_tool_blocked_provenance(*_args: Any, **_kwargs: Any) -> dict[str, Any]:
    return {"status": "contained-disabled", "reason": DISABLED_REASON}


def _blocked(*_args: Any, **_kwargs: Any) -> Any:
    raise ProgressError(DISABLED_REASON)


capture_continuation_input_snapshot = _blocked
prepare_repair_continuation = _blocked
activate_continuation_child = _blocked
finalize_continuation_child = _blocked
archive_continuation_checkpoint = _blocked


__all__ = [
    "CONTINUATION_MIGRATION_KEY", "CONTINUATION_PACKET_TYPE",
    "RETURN_PROVENANCE_FIELD", "activate_continuation_child",
    "archive_continuation_checkpoint", "capture_continuation_input_snapshot",
    "continuation_snapshots_equal", "continuation_state",
    "finalize_continuation_child", "prepare_repair_continuation",
    "returned_tool_blocked_provenance", "validate_continuation_checkpoint",
]
