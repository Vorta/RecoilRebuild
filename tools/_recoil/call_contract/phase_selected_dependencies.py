"""Serial compare selected dependencies proof phase."""
from __future__ import annotations

from _recoil.call_contract import comparison as _cc_comparison
from _recoil.call_contract import contributions as _cc_contributions
from _recoil.call_contract import work as _cc_work


def compare_selected_dependencies(work: _cc_work.CallerWork) -> None:
    from _recoil.commands.startup_contract import image_bytes
    from _recoil.lib.initializer_contract import constant_member_bytes
    from _recoil.lib.tooling import REPO_ROOT

    if work.address == "0x436630":
        # Zero-call initializers still establish call receiver state.
        # Compare all decoded scalar stores; an empty call list cannot
        # prove that a later trail callback starts inactive.
        from _recoil.commands.startup_contract import image_bytes
        from _recoil.lib.initializer_contract import constant_member_bytes
        work.retail_body = b"".join(bytes.fromhex(" ".join(row.bytes)) for row in work.retail_instructions)
        if work.retail_body != image_bytes((REPO_ROOT / "support/Recoil.exe").read_bytes(), 0x436630, len(work.retail_body)):
            raise ValueError("initializer BN listing disagrees with immutable retail")
        work.definition = work.candidate_assembly.caller_definition
        if work.definition is None:
            raise ValueError("initializer lacks fresh object definition")
        if constant_member_bytes(work.retail_body, object_size=0x180) != constant_member_bytes(work.definition.data, object_size=0x180):
            raise ValueError("initializer constant member stores differ from retail")
    work.expected = _cc_comparison._provider_argument_contract_facts(
        work.expected, work.retail_instructions,
        caller_start=work.address, caller_end_exclusive=work.end_exclusive,
    )
    work.candidate = _cc_comparison._provider_argument_contract_facts(
        work.candidate, work.candidate_assembly.instructions,
        caller_start=work.address, caller_end_exclusive=work.end_exclusive,
        candidate=work.candidate_assembly,
    )
    _cc_contributions.require_projection_accounting(work.retail_physical_contributions, work.expected, side="retail")
    _cc_contributions.require_projection_accounting(work.candidate_physical_contributions, work.candidate, side="candidate")
    work.trace_caller(
        "candidate-projections-complete",
        caller_index=work.caller_index,
        symbol_id=work.symbol_id,
        address=work.address,
        candidate_invocation_count=len(work.candidate),
    )
