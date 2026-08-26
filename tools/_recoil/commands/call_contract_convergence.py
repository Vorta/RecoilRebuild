"""Fail-closed direct call-contract closeout surface.

The rejected reusable result/currentness/cache architecture is gone.  A future
closeout producer must execute one complete no-reuse direct comparison under
an active packet; no stored result can substitute for that invocation.
"""

from __future__ import annotations

from copy import deepcopy
from typing import Any, Mapping

from _recoil.lib.call_contract_generations import current_generations
from _recoil.lib.progress import ProgressError


CONVERGENCE_CONTRACT_VERSION = 3
CONVERGENCE_EXPECTED_TRUTH = "fresh-complete-direct-retail-comparison"
CONVERGENCE_MIGRATION_KEY = "authored_call_contract_direct_closeout_v3"
CONVERGENCE_VERIFIER_SEMANTIC_PATHS: tuple[str, ...] = ()
RETAIL_FACT_PACKET_TYPE = "call-contract-retail-fact-read-v2"
DISABLED_REASON = (
    "call-contract closeout requires a separately approved active-packet "
    "fresh complete no-reuse direct comparison producer"
)


def derive_convergence_census(document: Any, *_args: Any, **_kwargs: Any) -> dict[str, Any]:
    slices = document.authored_call_contract_slices()
    symbol_ids = [str(value) for row in slices for value in row.get("symbol_ids", [])]
    return {
        "contract_version": CONVERGENCE_CONTRACT_VERSION,
        "symbol_ids": symbol_ids,
        "body_count": len(symbol_ids),
        "slice_ids": [str(row.get("id", "")) for row in slices],
    }


def compact_convergence_census(value: Mapping[str, Any], *_args: Any, **_kwargs: Any) -> dict[str, Any]:
    return {
        "contract_version": CONVERGENCE_CONTRACT_VERSION,
        "body_count": int(value.get("body_count", 0)),
        "slice_ids": list(value.get("slice_ids", [])),
    }


def convergence_generation_state(*_args: Any, **_kwargs: Any) -> dict[str, Any]:
    return {
        "current": False,
        "status": "contained-disabled",
        "generation": None,
        "reason": DISABLED_REASON,
        **current_generations(),
    }


def convergence_scheduler_mode(*_args: Any, **_kwargs: Any) -> str:
    return "contained-disabled"


def convergence_next_action(*_args: Any, **_kwargs: Any) -> str:
    return ""


def current_call_contract_verifier_semantic_identity(*_args: Any, **_kwargs: Any) -> dict[str, Any]:
    return {"kind": "call-contract-reviewed-integer-generations", **current_generations()}


def _normalized_semantic_projection(document: Any, *_args: Any, **_kwargs: Any) -> dict[str, Any]:
    return {
        "revision": int(getattr(document, "revision", -1)),
        **current_generations(),
    }


def carry_current_generation_across_work_ledger_mutation(*_args: Any, **_kwargs: Any) -> bool:
    return False


def prepare_live_convergence(*_args: Any, **_kwargs: Any) -> Any:
    raise ProgressError(DISABLED_REASON)


def prospective_wol_profile_convergence_route(*_args: Any, **_kwargs: Any) -> None:
    return None


def dependent_owner_repair_launchability(*_args: Any, **_kwargs: Any) -> dict[str, Any]:
    return {"launchable": False, "reason": DISABLED_REASON}


def retail_fact_packet_scopes(*_args: Any, **_kwargs: Any) -> list[dict[str, Any]]:
    return []


def _valid_retail_fact_scope(*_args: Any, **_kwargs: Any) -> bool:
    return False


__all__ = [
    "CONVERGENCE_CONTRACT_VERSION", "CONVERGENCE_EXPECTED_TRUTH",
    "CONVERGENCE_MIGRATION_KEY", "CONVERGENCE_VERIFIER_SEMANTIC_PATHS",
    "RETAIL_FACT_PACKET_TYPE", "_normalized_semantic_projection",
    "_valid_retail_fact_scope", "carry_current_generation_across_work_ledger_mutation",
    "compact_convergence_census", "convergence_generation_state",
    "convergence_next_action", "convergence_scheduler_mode",
    "current_call_contract_verifier_semantic_identity",
    "dependent_owner_repair_launchability", "derive_convergence_census",
    "prepare_live_convergence", "prospective_wol_profile_convergence_route",
    "retail_fact_packet_scopes",
]
