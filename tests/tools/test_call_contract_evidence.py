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
from _recoil.commands.asm_verify import Instruction  # noqa: E402
from _recoil.commands.call_contract_verify import (  # noqa: E402
    IdentityIndexes,
    _exact_targetless_vptr_call_proofs,
    _normalize_call_contract_row,
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


def _targetless_branch_receiver_instructions(
    *, member_receiver: bool
) -> tuple[Instruction, ...]:
    def row(address: int, raw_text: str, encoded: str) -> Instruction:
        return Instruction(
            text=raw_text,
            raw_text=raw_text,
            bytes=tuple(encoded.split()),
            source_line=f"{address:08x}:",
        )

    return (
        row(
            0x1000,
            "lea ebp, [ecx+0x20]" if member_receiver else "lea ebp, [esp]",
            "8d 69 20" if member_receiver else "8d 2c 24",
        ),
        row(0x1003, "test eax, eax", "85 c0"),
        row(0x1005, "je 0x1010", "74 09"),
        row(0x1007, "call 0x2000", "e8 f4 0f 00 00"),
        row(0x100C, "mov ebp, eax", "8b e8"),
        row(0x100E, "jmp 0x1018", "eb 08"),
        row(0x1010, "mov edx, [ebp]", "8b 55 00"),
        row(0x1013, "mov ecx, ebp", "8b cd"),
        row(0x1015, "call dword ptr [edx+0x78]", "ff 52 78"),
        row(0x1018, "ret", "c3"),
    )


def _targetless_branch_receiver_proofs(
    *,
    member_receiver: bool,
    source: str = "bn",
    allow_exact_this_member: bool = True,
) -> dict[int, str]:
    return _exact_targetless_vptr_call_proofs(
        _targetless_branch_receiver_instructions(
            member_receiver=member_receiver
        ),
        source=source,
        caller_start="0x1000",
        caller_end_exclusive="0x1019",
        indexes=IdentityIndexes(
            by_address={"0x2000": "symbol:dead-factory"},
            by_candidate_name={},
            provider_ids=frozenset(),
            storage_by_address={},
            storage_by_name={},
        ),
        allow_exact_this_member=allow_exact_this_member,
    )


def test_targetless_vptr_prefers_cfg_unanimous_this_member_over_dead_branch() -> None:
    proofs = _targetless_branch_receiver_proofs(member_receiver=True)

    assert proofs[8] == "load(this+0x20)"


def test_targetless_vptr_rejects_dead_branch_factory_without_live_member() -> None:
    proofs = _targetless_branch_receiver_proofs(member_receiver=False)

    assert 8 not in proofs


def test_targetless_vptr_does_not_replace_reviewed_retail_member_provenance() -> None:
    proofs = _targetless_branch_receiver_proofs(
        member_receiver=True,
        source="bn",
        allow_exact_this_member=False,
    )

    assert 8 not in proofs


def test_virtual_slot_normalization_folds_exact_this_member_lea_spelling() -> None:
    row = {
        "ordinal": 2,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": "load(address(this+0x3c))",
        "slot_displacement": 0x60,
        "cleanup_bytes": None,
    }

    normalized = _normalize_call_contract_row(row, candidate_side=True)

    assert normalized["storage_identity"] == "load(this+0x3c)"
