from __future__ import annotations

from pathlib import Path
import sys

import pytest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from _recoil.lib.call_contract_evidence import (  # noqa: E402
    CallContractEvidenceError,
    json_evidence_value,
)
from _recoil.lib.call_contract_generations import (  # noqa: E402
    current_generations,
    evidence_generations_current,
    generation_increment_findings,
    required_call_contract_verifier_component_findings,
    required_call_contract_verifier_component_graph,
)
from _recoil.lib.zeroarg_abi import (  # noqa: E402
    SourceSymbolPair,
    SymbolPair,
    ZeroArgAbiTarget,
    eligibility_gates,
    manifest_symbol_normalization,
)


def test_json_evidence_boundary_returns_an_alias_free_native_copy() -> None:
    source = {"calls": ({"ordinal": 0, "cleanup": None},), "passed": True}
    copied = json_evidence_value(source)
    assert copied == {"calls": [{"ordinal": 0, "cleanup": None}], "passed": True}
    assert copied is not source
    assert copied["calls"] is not source["calls"]


@pytest.mark.parametrize("value", [b"bytes", object(), {1: "bad"}, float("nan"), float("inf")])
def test_json_evidence_boundary_rejects_implicit_or_nonfinite_values(value: object) -> None:
    with pytest.raises(CallContractEvidenceError):
        json_evidence_value(value)


def test_generation_contract_has_only_reviewed_integer_coordinates() -> None:
    generations = current_generations()
    assert set(generations) == {
        "call_contract_verifier_generation",
        "expected_fact_schema_version",
    }
    assert all(type(value) is int and value > 0 for value in generations.values())
    assert evidence_generations_current(generations)
    assert not evidence_generations_current({**generations, "expected_fact_schema_version": 0})


def test_generation_component_graph_is_complete_and_operational() -> None:
    paths = {row["path"] for row in required_call_contract_verifier_component_graph()}
    assert "tools/_recoil/lib/call_contract_evidence.py" in paths
    assert "tools/_recoil/lib/zeroarg_abi.py" in paths
    assert required_call_contract_verifier_component_findings(ROOT) == []
    assert generation_increment_findings(
        ["tools/_recoil/lib/call_contract_evidence.py"], current_generations()
    ) == [
        "call_contract_verifier_generation: component changes require an increment; touched tools/_recoil/lib/call_contract_evidence.py",
        "expected_fact_schema_version: component changes require an increment; touched tools/_recoil/lib/call_contract_evidence.py",
    ]


def test_zeroarg_abi_policy_requires_every_semantic_gate() -> None:
    evidence = {
        "free_or_static": True,
        "explicit_argument_count": 0,
        "direct_calls": [{
            "address": "0x1000",
            "dispatch": "direct",
            "explicit_argument_count": 0,
            "callee_return": "plain-ret",
        }],
        **{name: False for name in (
            "hidden_this", "hidden_sret", "lifecycle", "variadic", "address_taken",
            "callback", "vtable", "export", "import", "function_pointer",
        )},
    }
    target = ZeroArgAbiTarget(
        target_id="unit", identity="unit-identity", callee_source=Path("callee.cpp"),
        callee=SymbolPair("_callee", "@callee@0"),
        callers=(SourceSymbolPair(Path("caller.cpp"), "_caller", "@caller@0"),),
        return_category="integral32", retail_evidence=evidence,
        eh_policy={"kind": "none", "retail_proven": True}, st0_policy=None,
    )
    assert all(row["passed"] for row in eligibility_gates(target))
    assert manifest_symbol_normalization((target,))["@callee@0"] == "unit-identity"
