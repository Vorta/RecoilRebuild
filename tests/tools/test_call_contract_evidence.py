from __future__ import annotations


def test_scalar_initializer_proves_stores_not_allocator_defaults():
    import pytest
    from _recoil.lib.initializer_contract import constant_member_bytes

    # mov edx,ecx; xor eax,eax; mov [edx+4],eax; mov eax,edx; ret
    body = bytes.fromhex("8b d1 33 c0 89 42 04 8b c2 c3")
    expected = {i: 0 for i in range(4, 8)}
    assert constant_member_bytes(body, object_size=16) == expected
    assert constant_member_bytes(body[:4] + body[7:], object_size=16) != expected
    assert constant_member_bytes(bytes.fromhex("8b d1 b8 01 00 00 00 89 42 04 8b c2 c3"), object_size=16) != expected
    # A subsequent partial overwrite must not be hidden by an earlier zero.
    overwrite = body[:-3] + bytes.fromhex("c6 42 05 01") + body[-3:]
    assert constant_member_bytes(overwrite, object_size=16)[5] == 1
    for invalid in (b"\xeb\x02" + body, body[:-1], body + b"\xc3",
                    bytes.fromhex("33 c0 89 42 04 8b c2 c3"),
                    bytes.fromhex("8b d1 33 c0 89 42 0f 8b c2 c3")):
        with pytest.raises(ValueError):
            constant_member_bytes(invalid, object_size=16)


def test_scalar_initializer_equates_byte_stores_and_bounded_rep():
    from _recoil.lib.initializer_contract import constant_member_bytes
    direct = bytes.fromhex("8b d1 33 c0 89 42 04 8b c2 c3")
    bytewise = bytes.fromhex("8b d1 33 c0 88 42 04 88 42 05 88 42 06 88 42 07 8b c2 c3")
    repeat = bytes.fromhex("57 8b d1 8d 7a 04 33 c0 b9 01 00 00 00 f3 ab 8b c2 5f c3")
    assert constant_member_bytes(direct, object_size=16) == constant_member_bytes(bytewise, object_size=16)
    assert constant_member_bytes(direct, object_size=16) == constant_member_bytes(repeat, object_size=16)


def test_reviewed_target_authority_is_not_duplicated_by_diagnostics():
    import pytest
    from _recoil.lib.authored_icf import reviewed_authority_targets, exact_selected_target_membership
    contract = "existing-winner-unknown-physical-group-refresh-v1"
    explicit = {"evidence_contract": contract, "governed_target_id": "reviewed"}
    assert reviewed_authority_targets([explicit], "older") == {"reviewed"}
    assert reviewed_authority_targets([{}], "accepted") == {"accepted"}
    assert exact_selected_target_membership(["reviewed", "diagnostic"], "reviewed")
    assert not exact_selected_target_membership(["diagnostic"], "reviewed")
    assert not exact_selected_target_membership(["reviewed", "reviewed"], "reviewed")
    for rows in ([explicit, {**explicit, "governed_target_id": "other"}],
                 [{"evidence_contract": contract}]):
        with pytest.raises(ValueError):
            reviewed_authority_targets(rows, "accepted")

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


def test_argument_bits_compare_equivalent_operations_and_reject_wrong_flags() -> None:
    from _recoil.lib.call_argument_bits import ArgumentBits, ArgumentProofError

    def prove(rows: list[str]) -> tuple[object, ...]:
        instructions = ["mov ebx, ecx", *rows, "push edx", "call eax"]
        cfg = {i: (i + 1,) for i in range(len(instructions) - 1)}
        return ArgumentBits(instructions, cfg).stack_argument(len(instructions) - 1, 0)

    first = prove(["mov dl, byte [ebx+9]", "and edx, 2", "shl edx, 6", "or edx, 64"])
    second = prove(["mov dl, byte [ebx+9]", "and edx, 2", "or edx, 1", "shl edx, 6"])
    assert first == second
    assert first != prove(["mov dl, byte [ebx+9]", "and edx, 2", "shl edx, 6", "or edx, 32"])
    assert first != prove(["mov dl, byte [ebx+8]", "and edx, 2", "shl edx, 6", "or edx, 64"])
    with pytest.raises(ArgumentProofError, match="volatile"):
        prove(["mov edx, 1", "call eax"])


def test_argument_bits_require_unanimous_paths_and_reject_unknown_stack_effects() -> None:
    from _recoil.lib.call_argument_bits import ArgumentBits, ArgumentProofError

    instructions = ["test eax, eax", "je somewhere", "mov edx, 1", "jmp join", "mov edx, 2", "push edx", "call eax"]
    cfg = {0: (1,), 1: (2, 4), 2: (3,), 3: (5,), 4: (5,), 5: (6,)}
    with pytest.raises(ArgumentProofError, match="conflicting"):
        ArgumentBits(instructions, cfg).stack_argument(6, 0)
    instructions[4] = "mov edx, 1"
    assert ArgumentBits(instructions, cfg).stack_argument(6, 0)[0] == 1
    with pytest.raises(ArgumentProofError, match="unresolved"):
        ArgumentBits(instructions, cfg, frozenset({1})).stack_argument(6, 0)
    bad = ["push 1", "mov dword [esp], 2", "call eax"]
    with pytest.raises(ArgumentProofError, match="overwritten"):
        ArgumentBits(bad, {0: (1,), 1: (2,)}).stack_argument(2, 0)


def test_constructor_dispatch_requires_a_this_store_and_complete_relocated_table() -> None:
    from types import SimpleNamespace
    from _recoil.lib.constructor_dispatch import exact_table_slots, leaf_constructor_vptr_write

    body = [bytes.fromhex("8b c1"), bytes.fromhex("c7 00 00 10 40 00"), bytes.fromhex("c7 40 04 00 00 00 00"), b"\xc3"]
    assert leaf_constructor_vptr_write(body) == (4, 0x401000)
    for invalid in (body[:1] + body[2:], [bytes.fromhex("8b c3"), *body[1:]], body[:-1], [*body, b"\xc3"]):
        with pytest.raises(ValueError):
            leaf_constructor_vptr_write(invalid)
    relocations = [SimpleNamespace(offset=0, type=6, symbol_name="_first"), SimpleNamespace(offset=4, type=6, symbol_name="_second")]
    assert exact_table_slots(bytes(8), relocations, 2) == ("_first", "_second")
    for invalid in (relocations[:1], [relocations[0], relocations[0]], [SimpleNamespace(offset=0, type=20, symbol_name="_first"), relocations[1]]):
        with pytest.raises(ValueError):
            exact_table_slots(bytes(8), invalid, 2)
    with pytest.raises(ValueError, match="addends"):
        exact_table_slots(b"\x01" + bytes(7), relocations, 2)


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


def test_constructor_body_padding_cannot_hide_code_or_relocations() -> None:
    from types import SimpleNamespace
    from _recoil.lib.constructor_dispatch import validate_leaf_listing_body

    instructions = [bytes.fromhex("c7 01 00 00 00 00"), b"\xc3"]
    body = b"".join(instructions)
    refs = [SimpleNamespace(offset=2)]
    validate_leaf_listing_body(instructions, body, refs)
    validate_leaf_listing_body(instructions, body + b"\x90" * 9, refs)
    for data, relocations in (
        (body + b"\xc3" + b"\x90" * 8, refs),
        (body + b"\x90" * 25, refs),
        (body + b"\x90", refs),
        (body + b"\x90" * 9, [*refs, SimpleNamespace(offset=8)]),
        (b"\x90" + body[1:], refs),
    ):
        with pytest.raises(ValueError):
            validate_leaf_listing_body(instructions, data, relocations)


def test_dispatch_weak_default_requires_exact_coff_chain() -> None:
    from copy import deepcopy
    from types import SimpleNamespace as S
    from _recoil.lib.constructor_dispatch import resolve_table_weak_target

    relocation = S(symbol_name="_weak", symbol_index=1)
    symbols = [
        S(index=1, name="_weak", storage_class=105, section_number=0, value=0,
          symbol_type=0x20, aux_count=1, weak_external_tag_index=3,
          weak_external_characteristics=2),
        S(index=3, name="_actual", storage_class=2, section_number=0, value=0,
          symbol_type=0x20, aux_count=0),
        S(index=4, name="_actual", storage_class=2, section_number=5, value=0,
          symbol_type=0x20, aux_count=0, section_name=".text", section_characteristics=0x20),
    ]
    assert resolve_table_weak_target(relocation, symbols) == "_actual"
    for index, field, value in (
        (0, "name", "_other"), (0, "weak_external_tag_index", 4),
        (0, "weak_external_characteristics", 3), (0, "aux_count", 0),
        (1, "storage_class", 105), (2, "section_name", ".data"),
    ):
        changed = deepcopy(symbols)
        setattr(changed[index], field, value)
        with pytest.raises(ValueError):
            resolve_table_weak_target(relocation, changed)
    with pytest.raises(ValueError):
        resolve_table_weak_target(relocation, symbols[:2])
    with pytest.raises(ValueError):
        resolve_table_weak_target(relocation, [*symbols, symbols[2]])


def test_constructor_member_store_preserves_only_proven_saved_this() -> None:
    from _recoil.lib.constructor_dispatch import straight_constructor_member_store

    rows = [bytes.fromhex(value) for value in (
        "56", "8b f1", "e8 00 00 00 00", "c7 46 20 00 10 40 00", "5e", "c3",
    )]
    assert straight_constructor_member_store(rows, 0x20) == (11, 0x401000)
    for altered in (
        [rows[0], bytes.fromhex("8b f3"), *rows[2:]],
        [rows[0], bytes.fromhex("8b c1"), rows[2], bytes.fromhex("c7 40 20 00 10 40 00"), *rows[4:]],
        [*rows[:3], bytes.fromhex("74 07"), *rows[3:]],
        [*rows[:4], rows[3], *rows[4:]],
        [*rows[:3], bytes.fromhex("c7 46 24 00 10 40 00"), *rows[4:]],
        rows[:-1], [*rows, b"\xc3"],
    ):
        with pytest.raises(ValueError):
            straight_constructor_member_store(altered, 0x20)


def test_constant_callback_return_cannot_hide_calls_or_wrong_value() -> None:
    from types import SimpleNamespace
    from _recoil.lib.constructor_dispatch import constant_return

    one = bytes.fromhex("b8 01 00 00 00 c3")
    assert constant_return(one) == constant_return(one + b"\x90" * 10) == 1
    assert constant_return(bytes.fromhex("33 c0 c3")) == 0
    for body in (one + b"\xc3", one + b"\x90", b"\xe8\x00\x00\x00\x00" + one, one[:-1]):
        with pytest.raises(ValueError):
            constant_return(body)
    with pytest.raises(ValueError):
        constant_return(one, [SimpleNamespace(offset=1)])


def test_constructor_cfg_requires_the_same_this_stamp_on_every_return() -> None:
    from _recoil.lib.constructor_dispatch import constructor_vptr_store

    rows = [bytes.fromhex(value) for value in (
        "56", "8b f1", "e8 00 00 00 00", "c7 06 00 10 40 00",
        "85 c0", "74 02", "8b ce", "5e", "c3",
    )]
    assert constructor_vptr_store(rows) == (10, 0x401000)
    for altered in (
        [rows[0], bytes.fromhex("8b f3"), *rows[2:]],  # not entry this
        [*rows[:3], bytes.fromhex("74 06"), *rows[3:]],  # stamp bypass
        [*rows[:4], rows[3], *rows[4:]],  # overwritten vptr
        [*rows[:4], bytes.fromhex("c7 46 fe 00 20 40 00"), *rows[4:]],
        [*rows[:4], bytes.fromhex("c7 00 00 20 40 00"), *rows[4:]],
        [*rows[:4], bytes.fromhex("eb fe"), *rows[4:]],  # unbounded loop
        rows[:-1],
    ):
        with pytest.raises(ValueError):
            constructor_vptr_store(altered)


def test_constructor_cfg_kills_volatile_and_partial_register_aliases() -> None:
    from _recoil.lib.constructor_dispatch import constructor_vptr_store

    for prefix in (
        ("8b c1", "e8 00 00 00 00"),
        ("8b c1", "8a 44 24 04"),
    ):
        with pytest.raises(ValueError):
            constructor_vptr_store([bytes.fromhex(row) for row in
                                    (*prefix, "c7 00 00 10 40 00", "c3")])


def test_global_virtual_receiver_window_rejects_bypass_and_wrong_provenance() -> None:
    from _recoil.lib.constructor_dispatch import global_vptr_call_window

    rows = [bytes.fromhex(row) for row in (
        "8b 0d 00 20 40 00", "8b 15 00 30 40 00", "52", "8b 01", "ff 10",
    )]
    assert global_vptr_call_window(rows) == (4, 2, 0x402000)
    for altered in (
        [bytes.fromhex("8b 15 00 20 40 00"), *rows[1:]],
        [rows[0], bytes.fromhex("8b 0d 00 30 40 00"), b"\x51", *rows[3:]],
        [*rows[:3], bytes.fromhex("8b 03"), rows[4]],
        [*rows[:4], bytes.fromhex("ff 50 04")],
        [bytes.fromhex("74 06"), *rows],
        [*rows, *rows],
    ):
        with pytest.raises(ValueError):
            global_vptr_call_window(altered)


def test_invocation_comparison_retains_dependency_and_argument_obligations() -> None:
    from copy import deepcopy
    from _recoil.commands.call_contract_verify import compare_call_contracts

    base = {"ordinal": 0, "form": "call", "dispatch": "direct", "identity_kind": "direct",
            "target_identity": "symbol:unit", "storage_identity": "",
            "slot_displacement": None, "cleanup_bytes": 0}
    for field, facts, changed in (
        ("constructor_dispatch", {"slots": ["symbol:first", "symbol:second"]}, {"slots": ["symbol:second", "symbol:first"]}),
        ("member_dispatch_return", {"return_value": 1}, {"return_value": 0}),
        ("concrete_update_dispatch", {"target": "symbol:override"}, {"target": "symbol:base"}),
        ("argument_bits", {"4": [0, 1]}, {"4": [1, 0]}),
    ):
        expected = {**base, field: facts}
        assert compare_call_contracts([expected], [deepcopy(expected)])["passed"]
        assert not compare_call_contracts([expected], [base])["passed"]
        assert not compare_call_contracts([expected], [{**base, field: changed}])["passed"]


def test_explicit_lifecycle_target_cannot_be_replaced_by_ambiguous_fallback() -> None:
    from _recoil.commands.call_contract_verify import _explicit_lifecycle_target_identity

    assert _explicit_lifecycle_target_identity("symbol:unit", {"symbol:unit"}) == "symbol:unit"
    for prior, candidates in (
        (None, {"symbol:unit"}), ("", {"symbol:unit"}),
        ("symbol:other", {"symbol:unit"}),
        ("symbol:unit", {"symbol:unit", "symbol:other"}),
        ("symbol:unit", set()),
    ):
        assert _explicit_lifecycle_target_identity(prior, candidates) == ""


def test_dynamic_probe_coordinates_allow_only_uniform_translation() -> None:
    from _recoil.commands.call_contract_verify import _translated_dynamic_probe_setup

    setup = ((8, b"\x24\xfc"), (18, b"\x24\xfc"))
    for delta in (-1, 0, 3):
        assert _translated_dynamic_probe_setup((10, 20), (10 + delta, 20 + delta), setup) == tuple(
            (offset + delta, body) for offset, body in setup
        )
    for reviewed, observed, rows in (
        ((), (), setup), ((10, 20), (10,), setup),
        ((10, 20), (10, None), setup), ((10, 20), (10, 10), setup),
        ((20, 10), (20, 10), setup), ((10, 20), (20, 10), setup),
        ((10, 20), (9, 20), setup), ((10, 20), (0, 10), setup),
        ((10, 20), (10, 20), ()), ((10, 20), (10, 20), ((8, b""),)),
        ((10, 20), (True, 20), setup), ((-1, 9), (0, 10), setup),
    ):
        with pytest.raises(ValueError):
            _translated_dynamic_probe_setup(reviewed, observed, rows)


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


def test_path_resource_cleanup_distinguishes_exclusive_and_sequential_calls() -> None:
    from _recoil.lib.path_contract import path_depths
    call = (bytes.fromhex("e8 00 00 00 00"), "call")
    ret = (b"\xc3", "ret")
    # Same static call population: one acquire and two release sites.
    exclusive = [call, (b"\x74\x06", "je"), call, ret, call, ret]
    assert path_depths(exclusive, b"".join(x[0] for x in exclusive), {0: 1, 7: -1, 13: -1}) == {
        "return_depths": [0], "peak_depth": 1,
    }
    sequential = [call, call, call, ret]
    with pytest.raises(ValueError, match="underflow"):
        path_depths(sequential, b"".join(x[0] for x in sequential), {0: 1, 5: -1, 10: -1})
    leak = [call, ret]
    assert path_depths(leak, b"".join(x[0] for x in leak), {0: 1})["return_depths"] == [1]
    for rows, effects in (
        ([call, ret], {}),
        ([(b"\xeb\x01", "jmp"), ret], {}),
        ([(b"\xff\xe0", "jmp")], {}),
        ([call, (b"\xeb\xf9", "jmp")], {0: 1}),
        ([ret, call], {1: 0}),
    ):
        with pytest.raises(ValueError):
            path_depths(rows, b"".join(x[0] for x in rows), effects)
    with pytest.raises(ValueError, match="listing"):
        path_depths([ret], b"\x90", {})
    # A zero-effect loop converges; its conditional exit is still checked.
    loop = [(b"\x75\xfe", "jne"), ret]
    assert path_depths(loop, b"\x75\xfe\xc3", {})["return_depths"] == [0]
    assert path_depths([ret], b"\xc3\x90", {})["return_depths"] == [0]
    with pytest.raises(ValueError, match="listing"):
        path_depths([ret], b"\xc3\xcc", {})
    with pytest.raises(ValueError, match="boundaries"):
        path_depths([(b"\xeb\x00", "jmp")], b"\xeb\x00\x90", {})
    indirect = [(b"\xff\x10", "call"), ret]
    with pytest.raises(ValueError, match="effect"):
        path_depths(indirect, b"\xff\x10\xc3", {})
    assert path_depths(indirect, b"\xff\x10\xc3", {0: 0})["return_depths"] == [0]


def test_image_path_listing_preserves_wrapped_instruction_bytes() -> None:
    from _recoil.commands.startup_contract import parse_image_listing
    listing = "  00001000: C7 44 24 18 00 00  mov dword ptr [esp+18h],0\n            00 00\n  00001008: C3                 ret\n"
    rows = parse_image_listing(listing, 0x1000, 9)
    assert rows == [(bytes.fromhex("c7 44 24 18 00 00 00 00"), "mov"), (b"\xc3", "ret")]
    assert parse_image_listing("  401000: c3   ret\n", 0x401000, 1) == [(b"\xc3", "ret")]
    with pytest.raises(ValueError, match="noncontiguous"):
        parse_image_listing(listing.replace("00001008", "00001007"), 0x1000, 9)


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
