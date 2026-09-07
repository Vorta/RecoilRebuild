"""Physical invocation obligations survive identity and lifecycle projections."""
from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Mapping, Sequence

from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import instructions as _cc_instructions
from _recoil.call_contract import proofs as _cc_proofs


@dataclass(frozen=True)
class InvocationContribution:
    side: str
    location: str
    form: str
    dispatch: str


def physical_contributions(instructions: Sequence[Any], sites: Sequence[str], *,
                           side: str, caller_start: str, candidate: Any = None,
                           caller_end_exclusive: str = "") -> tuple[InvocationContribution, ...]:
    addresses = list(_cc_cfg._instruction_runtime_addresses(instructions,
        source="bn" if side == "retail" else "cod", caller_start=int(caller_start, 16)))
    if candidate is not None:
        if side != "candidate" or tuple(instructions) != tuple(candidate.instructions):
            raise ValueError("physical contribution belongs to a different candidate body")
        for index, address in enumerate(addresses):
            if address is None and _cc_cfg._instruction_mnemonic(instructions[index]) in {"call", "jmp"}:
                addresses[index] = _cc_cfg._closed_candidate_invocation_runtime_address(
                    instructions, instruction_index=index, caller_start=int(caller_start, 16),
                    caller_end=int(caller_end_exclusive, 16), caller_definition=candidate.caller_definition)
    if len(set(sites)) != len(sites):
        raise ValueError("physical invocation contribution is counted more than once")
    result = []
    for site in sites:
        matches = [index for index, address in enumerate(addresses) if address == int(site, 16)]
        if len(matches) != 1:
            raise ValueError("physical invocation contribution lacks its unique machine location")
        fact = _cc_instructions.instruction_fact(instructions[matches[0]])
        if not (fact.is_call or (fact.is_jump and fact.mnemonic == "jmp")) or len(fact.operands) != 1:
            raise ValueError("physical invocation contribution is not an exact CALL or tail JMP")
        result.append(InvocationContribution(side, site, "call" if fact.is_call else "tail",
            "direct" if fact.operands[0].kind == "immediate" else "indirect"))
    return tuple(result)


@dataclass(frozen=True)
class OwnedInvocationContribution:
    owner: str
    contribution: InvocationContribution


def partition_native_funclets(physical: Sequence[InvocationContribution], proof: Any, *, caller_start: str
                              ) -> tuple[tuple[InvocationContribution, ...], tuple[OwnedInvocationContribution, ...]]:
    """Retain each invocation exactly once across the parent and native bodies.

    The caller supplies the live COD/COFF catch-funclet proof. This operation
    checks its complete census and locations before separating contributions;
    each resulting lifecycle owner is compared in its own authored census row.
    """
    if proof is None:
        return tuple(physical), ()
    if len(physical) != proof.generic_invocation_count or len(proof.ranges) != len(proof.lifecycle_addresses):
        raise ValueError("native funclet partition has a different physical census")
    offsets = tuple(int(row.location, 16) - int(caller_start, 16) for row in physical)
    selected = tuple(index for index, offset in enumerate(offsets)
                     if any(start <= offset < end for start, end in proof.ranges))
    if (selected != proof.excluded_invocation_ordinals
            or tuple(offsets[index] for index in selected) != proof.excluded_invocation_offsets):
        raise ValueError("native funclet partition disagrees with exact invocation locations")
    owned = []
    for index in selected:
        owners = [owner for owner, (start, end) in zip(proof.lifecycle_addresses, proof.ranges)
                  if start <= offsets[index] < end]
        if len(owners) != 1:
            raise ValueError("native funclet contribution has ambiguous ownership")
        owned.append(OwnedInvocationContribution(owners[0], physical[index]))
    return tuple(row for index, row in enumerate(physical) if index not in selected), tuple(owned)


def contribution_results(retail: Sequence[InvocationContribution],
                         candidate: Sequence[InvocationContribution]) -> tuple[_cc_proofs.ProofResult, ...]:
    results = [_cc_proofs.prove(_cc_proofs.Obligation("physical-contribution", _cc_proofs.Fact("retail", "count", len(retail), "body")),
                     _cc_proofs.Fact("candidate", "count", len(candidate), "body"))]
    for index, expected in enumerate(retail):
        observed = candidate[index] if index < len(candidate) else None
        for dimension in ("form", "dispatch"):
            results.append(_cc_proofs.prove(_cc_proofs.Obligation("physical-contribution",
                _cc_proofs.Fact("retail", dimension, getattr(expected, dimension), expected.location)),
                _cc_proofs.Fact("candidate", dimension, getattr(observed, dimension), observed.location) if observed else None))
    return tuple(results)


def require_projection_accounting(physical: Sequence[InvocationContribution],
                                 projected: Sequence[Mapping[str, Any]], *, side: str) -> None:
    # Identity equivalence never authorizes removing a call, converting a tail,
    # or erasing indirect dispatch. An expanded helper graph must retain these
    # primary contributions; its separate receipt does not grant deletion.
    if len(physical) != len(projected):
        raise _cc_proofs.ProofConflict(family="physical-contribution-population", location=side,
                            existing=len(physical), incoming=len(projected))
    for index, (original, row) in enumerate(zip(physical, projected)):
        if row.get("form") != original.form or row.get("dispatch") != original.dispatch:
            raise _cc_proofs.ProofConflict(family="physical-contribution-form/dispatch", location=original.location,
                                existing=(original.form, original.dispatch), incoming=(row.get("form"), row.get("dispatch")))
