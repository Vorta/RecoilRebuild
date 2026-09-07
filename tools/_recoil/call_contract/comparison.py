"""Recoil call-contract comparison evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import listing as _cc_listing
from _recoil.call_contract import proofs as _cc_proofs
from _recoil.call_contract import receiver_candidate as _cc_receiver_candidate
from _recoil.call_contract import receiver_equivalence as _cc_receiver_equivalence
from _recoil.call_contract import receiver_storage as _cc_receiver_storage

if TYPE_CHECKING:
    from _recoil.call_contract.records import CandidateAssembly, IdentityIndexes

import re
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_DIR32,
    IMAGE_SYM_CLASS_EXTERNAL,
    Instruction,
)
from _recoil.lib.binja import BinaryNinjaBridge
from _recoil.lib.call_contract_evidence import json_evidence_value
from _recoil.lib.progress import address_value, normalize_address


def _normalize_call_contract_row(
    row: Mapping[str, Any],
    *,
    candidate_side: bool,
    reviewed_direct_target_equivalences: Mapping[str, str] | None = None,
    reviewed_provider_iat_equivalences: Mapping[str, str] | None = None,
) -> dict[str, Any]:
    """Return the existing side-local comparison form without consulting peers."""

    comparable = dict(row)
    if candidate_side:
        candidate_identity = comparable.get("target_identity")
        expected_identity = dict(
            reviewed_direct_target_equivalences or {}
        ).get(str(candidate_identity), "")
        if (
            expected_identity
            and comparable.get("dispatch") == "direct"
            and comparable.get("identity_kind") == "direct"
            and comparable.get("storage_identity") == ""
            and comparable.get("slot_displacement") is None
        ):
            comparable["identity_kind"] = "provider"
            comparable["target_identity"] = expected_identity
        expected_iat_identity = dict(
            reviewed_provider_iat_equivalences or {}
        ).get(str(candidate_identity), "")
        direct_provider_form = (
            comparable.get("dispatch") == "direct"
            and comparable.get("identity_kind") == "provider"
            and comparable.get("storage_identity") == ""
        )
        register_iat_form = (
            comparable.get("dispatch") == "indirect"
            and comparable.get("identity_kind") == "iat"
            and comparable.get("storage_identity") == candidate_identity
        )
        reviewed_direct_iat_form = (
            comparable.get("dispatch") == "direct"
            and comparable.get("identity_kind") == "direct"
            and comparable.get("storage_identity") == ""
            and isinstance(candidate_identity, str)
            and candidate_identity.startswith("iat:")
            and expected_iat_identity == candidate_identity
        )
        if (
            expected_iat_identity
            and str(expected_iat_identity).startswith("iat:")
            and comparable.get("form") == "call"
            and comparable.get("slot_displacement") is None
            and (
                direct_provider_form
                or register_iat_form
                or reviewed_direct_iat_form
            )
        ):
            comparable["identity_kind"] = "iat"
            comparable["target_identity"] = expected_iat_identity
    storage = comparable.get("storage_identity")
    if (
        comparable.get("identity_kind") == "virtual-slot"
        and isinstance(storage, str)
    ):
        canonical_storage = _cc_receiver_storage._canonical_address_root_affine_storage(storage)
        exact_member_storage = _cc_receiver_equivalence._canonical_exact_this_member_vptr_storage(
            canonical_storage
        )
        if exact_member_storage:
            canonical_storage = exact_member_storage
        canonical_storage = _cc_receiver_equivalence._canonical_proven_member_storage(canonical_storage)
        if _cc_receiver_candidate._is_bounded_stack_vptr(canonical_storage):
            if (
                "index-scale(" in canonical_storage
                or "bounded-stride(" in canonical_storage
            ):
                canonical_storage = re.sub(
                    r"(?:load\(stack-local(?P<load>[+-]0x[0-9a-f]+)?\)"
                    r"|bounded-stack-counter(?P<counter>[+-]0x[0-9a-f]+)?"
                    r"|bounded-counter\(e(?:ax|bx|cx|dx|si|di|bp)\))",
                    lambda match: (
                        "bounded-stack-scalar"
                        + (match.group("load") or match.group("counter") or "")
                    ),
                    canonical_storage,
                )
            else:
                canonical_storage = re.sub(
                    r"(?:bounded-counter\(e(?:ax|bx|cx|dx|si|di|bp)\)"
                    r"|bounded-stack-counter(?:[+-]0x[0-9a-f]+)?)",
                    "bounded-loop-counter",
                    canonical_storage,
                )
            canonical_storage = re.sub(
                r"bounded-cursor\(e(?:ax|bx|cx|dx|si|di|bp),"
                r"(?P<bounds>0x[0-9a-f]+,0x[0-9a-f]+,0x[0-9a-f]+)\)",
                r"bounded-cursor(\g<bounds>)",
                canonical_storage,
            )
        comparable["storage_identity"] = canonical_storage
    return comparable


def _compose_reviewed_provider_iat_equivalences(
    *equivalence_sets: Mapping[str, str],
) -> dict[str, str]:
    """Compose only explicit comparison-scoped provider/IAT proofs."""

    result: dict[str, str] = {}
    for equivalences in equivalence_sets:
        for candidate_identity, expected_iat_identity in equivalences.items():
            if (
                not isinstance(candidate_identity, str)
                or not candidate_identity
                or not isinstance(expected_iat_identity, str)
                or not expected_iat_identity.startswith("iat:")
            ):
                raise ValueError(
                    "reviewed provider/IAT equivalence is malformed"
                )
            prior = result.get(candidate_identity)
            if prior is not None and prior != expected_iat_identity:
                raise ValueError(
                    "reviewed provider/IAT equivalences conflict for "
                    f"{candidate_identity!r}"
                )
            result[candidate_identity] = expected_iat_identity
    return result


def _normalize_call_contract_pair(
    expected_row: Mapping[str, Any] | None,
    candidate_row: Mapping[str, Any] | None,
    *,
    reviewed_direct_target_equivalences: Mapping[str, str] | None = None,
    reviewed_provider_iat_equivalences: Mapping[str, str] | None = None,
) -> tuple[dict[str, Any] | None, dict[str, Any] | None]:
    comparable_expected = (
        _normalize_call_contract_row(
            expected_row,
            candidate_side=False,
            reviewed_direct_target_equivalences=(
                reviewed_direct_target_equivalences
            ),
            reviewed_provider_iat_equivalences=(
                reviewed_provider_iat_equivalences
            ),
        )
        if expected_row is not None
        else None
    )
    comparable_candidate = (
        _normalize_call_contract_row(
            candidate_row,
            candidate_side=True,
            reviewed_direct_target_equivalences=(
                reviewed_direct_target_equivalences
            ),
            reviewed_provider_iat_equivalences=(
                reviewed_provider_iat_equivalences
            ),
        )
        if candidate_row is not None
        else None
    )
    if comparable_expected is not None and comparable_candidate is not None:
        # ``None`` records that retail does not constrain caller cleanup.  It
        # must not turn a candidate-observed ADD ESP into a semantic mismatch;
        # positive retail cleanup remains exact and unchanged.
        if comparable_expected.get("cleanup_bytes") is None:
            comparable_candidate["cleanup_bytes"] = None

    return comparable_expected, comparable_candidate


def _capture_call_contract_diagnostic_context(
    context: dict[str, Any] | None,
    *,
    symbol_id: str,
    address: str,
    expected: Sequence[Mapping[str, Any]],
    candidate: Sequence[Mapping[str, Any]],
    reviewed_direct_target_equivalences: Mapping[str, str] | None = None,
    reviewed_provider_iat_equivalences: Mapping[str, str] | None = None,
) -> None:
    if context is None or context:
        return
    expected_rows = [dict(row) for row in expected]
    candidate_rows = [dict(row) for row in candidate]
    expected_normalized_rows = [
        _normalize_call_contract_row(
            row,
            candidate_side=False,
            reviewed_direct_target_equivalences=(
                reviewed_direct_target_equivalences
            ),
            reviewed_provider_iat_equivalences=(
                reviewed_provider_iat_equivalences
            ),
        )
        for row in expected_rows
    ]
    candidate_normalized_rows = [
        _normalize_call_contract_row(
            row,
            candidate_side=True,
            reviewed_direct_target_equivalences=(
                reviewed_direct_target_equivalences
            ),
            reviewed_provider_iat_equivalences=(
                reviewed_provider_iat_equivalences
            ),
        )
        for row in candidate_rows
    ]
    expected_comparable_rows: list[dict[str, Any] | None] = []
    candidate_comparable_rows: list[dict[str, Any] | None] = []
    for index in range(max(len(expected_rows), len(candidate_rows))):
        expected_comparable, candidate_comparable = (
            _normalize_call_contract_pair(
                expected_rows[index] if index < len(expected_rows) else None,
                candidate_rows[index] if index < len(candidate_rows) else None,
                reviewed_direct_target_equivalences=(
                    reviewed_direct_target_equivalences
                ),
                reviewed_provider_iat_equivalences=(
                    reviewed_provider_iat_equivalences
                ),
            )
        )
        expected_comparable_rows.append(expected_comparable)
        candidate_comparable_rows.append(candidate_comparable)
    context.update(
        {
            "symbol_id": symbol_id,
            "address": address,
            "expected_rows": expected_rows,
            "candidate_rows": candidate_rows,
            "expected_normalized_rows": expected_normalized_rows,
            "candidate_normalized_rows": candidate_normalized_rows,
            "expected_comparable_rows": expected_comparable_rows,
            "candidate_comparable_rows": candidate_comparable_rows,
            "reviewed_direct_target_equivalences": dict(
                reviewed_direct_target_equivalences or {}
            ),
            "reviewed_provider_iat_equivalences": dict(
                reviewed_provider_iat_equivalences or {}
            ),
        }
    )


def _recoilapp_reviewed_retail_cleanup_contract(
    contract: Sequence[Mapping[str, Any]],
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
) -> list[dict[str, Any]]:
    """Remove one BN path-local frame reversal from retail cleanup truth.

    ``CZRecoilFrame::UpdateHwApiMenuItem`` has two terminal epilogues.  The
    RemoveMenu path performs ``add esp, 0x40; retn 8`` after its final call,
    while the sibling path restores ESI/EDI before the same frame reversal.
    BN's block-local ordering makes the former look like caller cleanup.  The
    exact caller, ordinal, IAT identity, and 0x40 observation below are the
    immutable reviewed package; every other caller retains the generic parser.
    """

    result = [dict(row) for row in contract]
    normalized_start = normalize_address(caller_start)
    if normalized_start == "0x42f8e0":
        if (
            caller_identity != "symbol:recoil:function:0x42f8e0"
            or normalize_address(caller_end_exclusive) != "0x42f9d0"
            or len(result) != 17
            or result[16]
            != {
                "ordinal": 16,
                "form": "call",
                "dispatch": "direct",
                "identity_kind": "direct",
                "target_identity": "symbol:recoil:function:0x462630",
                "storage_identity": "",
                "slot_displacement": None,
                "cleanup_bytes": 0x30,
            }
        ):
            raise ValueError(
                "RecoilApp PlayState terminal-frame cleanup normalization "
                "rejects retail caller, extent, population, identity, or epilogue drift"
            )
        result[16]["cleanup_bytes"] = None
        return result
    if normalized_start == "0x430070":
        if (
            caller_identity != "symbol:recoil:function:0x430070"
            or normalize_address(caller_end_exclusive) != "0x430100"
            or result
            != [
                {
                    "ordinal": 0,
                    "form": "call",
                    "dispatch": "direct",
                    "identity_kind": "direct",
                    "target_identity": "symbol:recoil:function:0x472450",
                    "storage_identity": "",
                    "slot_displacement": None,
                    "cleanup_bytes": 0x48,
                }
            ]
        ):
            raise ValueError(
                "zInput constant-force terminal-frame cleanup normalization "
                "rejects retail caller, extent, population, identity, or epilogue drift"
            )
        result[0]["cleanup_bytes"] = None
        return result
    if normalized_start == "0x425920":
        if (
            caller_identity != "symbol:recoil:function:0x425920"
            or normalize_address(caller_end_exclusive) != "0x425a20"
            or len(result) != 13
            or tuple(row.get("ordinal") for row in result)
            != tuple(range(13))
            or result[12]
            != {
                "ordinal": 12,
                "form": "call",
                "dispatch": "direct",
                "identity_kind": "direct",
                "target_identity": "symbol:recoil:function:0x42f9f0",
                "storage_identity": "",
                "slot_displacement": None,
                "cleanup_bytes": 0x10,
            }
        ):
            raise ValueError(
                "Player init terminal-frame cleanup normalization rejects "
                "retail caller, extent, population, identity, or epilogue drift"
            )
        result[12]["cleanup_bytes"] = None
        return result
    if normalized_start != "0x4317d0":
        return result
    if (
        caller_identity != "symbol:recoil:function:0x4317d0"
        or normalize_address(caller_end_exclusive) != "0x431870"
        or len(result) != 7
        or result[6]
        != {
            "ordinal": 6,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "iat",
            "target_identity": "iat:RemoveMenu",
            "storage_identity": "iat:RemoveMenu",
            "slot_displacement": None,
            "cleanup_bytes": 0x40,
        }
    ):
        raise ValueError(
            "RecoilApp RemoveMenu cleanup normalization rejects retail caller, "
            "extent, population, identity, or epilogue drift"
        )
    result[6]["cleanup_bytes"] = None
    return result


def _diagnostic_call_contract_rows_equal(
    expected_row: Mapping[str, Any],
    candidate_row: Mapping[str, Any],
    context: Mapping[str, Any],
) -> bool:
    comparable_expected, comparable_candidate = _normalize_call_contract_pair(
        expected_row,
        candidate_row,
        reviewed_direct_target_equivalences=context.get(
            "reviewed_direct_target_equivalences"
        ),
        reviewed_provider_iat_equivalences=context.get(
            "reviewed_provider_iat_equivalences"
        ),
    )
    assert comparable_expected is not None
    assert comparable_candidate is not None
    comparable_expected.pop("ordinal", None)
    comparable_candidate.pop("ordinal", None)
    return comparable_expected == comparable_candidate


def _briefing_dispatch_contract_facts(
    expected_rows: list[dict[str, Any]], candidate_rows: list[dict[str, Any]],
    *, caller_start: str, candidate: CandidateAssembly,
    bridge: BinaryNinjaBridge, indexes: IdentityIndexes,
) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    from _recoil.lib.constructor_dispatch import (
        constructor_vptr_store,
        exact_table_slots,
        global_vptr_call_window,
        resolve_table_weak_target,
    )

    if caller_start not in {"0x403930", "0x404280"}:
        return expected_rows, candidate_rows
    if not expected_rows or not candidate_rows:
        raise ValueError("briefing dispatch dependency lacks invocation rows")
    retail = _cc_listing.parse_assembly(bridge.assembly("0x403930"), source="bn")
    chunks = tuple(bytes.fromhex(" ".join(row.bytes)) for row in retail)
    _, table_address = constructor_vptr_store(chunks)
    body = b"".join(chunks)
    if body != _cc_cfg._hexdump_bytes(bridge.hexdump("0x403930", len(body))):
        raise ValueError("briefing constructor assembly disagrees with live bytes")
    definition = candidate.constructor_definitions.get(_cc_catalog.HUD_BRIEFING_RUNTIME_CONSTRUCTOR_SYMBOL)
    if definition is None:
        raise ValueError("briefing dispatch lacks fresh constructor dependency")
    chunks = tuple(bytes.fromhex(" ".join(row.bytes)) for row in definition.instructions)
    write_offset, addend = constructor_vptr_store(chunks)
    body = b"".join(chunks)
    tail = definition.data[len(body):]
    if (not definition.data.startswith(body) or len(tail) >= 16
            or (tail and (len(definition.data) % 16 or any(byte != 0x90 for byte in tail)))
            or definition.section_external_functions != (_cc_catalog.HUD_BRIEFING_RUNTIME_CONSTRUCTOR_SYMBOL,)
            or definition.section_size != len(definition.data)):
        raise ValueError("briefing constructor lacks exact complete listing/COFF extent")
    refs = [row for row in definition.relocations
            if row.offset < write_offset + 4 and row.offset + 4 > write_offset]
    if (len(refs) != 1 or refs[0].offset != write_offset
            or refs[0].type != IMAGE_REL_I386_DIR32 or addend):
        raise ValueError("briefing this-vptr lacks unique zero-addend DIR32 relocation")
    table = candidate.vftable_definitions.get(refs[0].symbol_name)
    if (table is None or not refs[0].symbol_name.startswith("??_7")
            or table.section_size != len(table.data)
            or table.section_external_symbols != (table.symbol,)):
        raise ValueError("briefing this-vptr lacks exclusive emitted C++ table")
    exact_table_slots(table.data, table.relocations, 3)
    reference = _cc_cfg._hexdump_bytes(bridge.hexdump(normalize_address(table_address), 12))
    if len(reference) != 12:
        raise ValueError("briefing retail table read is incomplete")
    address = normalize_address(int.from_bytes(reference[:4], "little"))
    target = resolve_table_weak_target(
        next(row for row in table.relocations if row.offset == 0), definition.coff_symbols,
    )
    required, observed = indexes.by_address.get(address), indexes.by_candidate_name.get(target)
    if not required or not observed:
        raise ValueError(f"unresolved briefing update slot identity: {address}, {target}")
    common = {"constructor": indexes.by_address.get("0x403930"),
              "receiver_offset": 0, "slot_count": 3, "slot_displacement": 0}
    expected_ordinal = candidate_ordinal = 0
    required_storage = observed_storage = None
    if caller_start == "0x404280":
        worker = _cc_listing.parse_assembly(bridge.assembly(caller_start), source="bn")
        worker_chunks = tuple(bytes.fromhex(" ".join(row.bytes)) for row in worker)
        call_index, _, receiver = global_vptr_call_window(worker_chunks)
        worker_body = b"".join(worker_chunks)
        if worker_body != _cc_cfg._hexdump_bytes(bridge.hexdump(caller_start, len(worker_body))):
            raise ValueError("briefing worker assembly disagrees with live bytes")
        expected_ordinal = sum(_cc_cfg._instruction_mnemonic(row) == "call" for row in worker[:call_index])
        required_storage = indexes.storage_by_address.get(normalize_address(receiver))
        worker_chunks = tuple(bytes.fromhex(" ".join(row.bytes)) for row in candidate.instructions)
        call_index, operand, addend = global_vptr_call_window(worker_chunks)
        worker_definition = candidate.caller_definition
        if worker_definition is None or not worker_definition.data.startswith(b"".join(worker_chunks)):
            raise ValueError("briefing worker lacks matching fresh COFF body")
        refs = [row for row in worker_definition.relocations
                if row.offset < operand + 4 and row.offset + 4 > operand]
        if len(refs) != 1 or refs[0].offset != operand or refs[0].type != IMAGE_REL_I386_DIR32 or addend:
            raise ValueError("briefing worker receiver lacks unique DIR32 storage relocation")
        observed_storage = indexes.storage_by_name.get(refs[0].symbol_name)
        if not required_storage or not observed_storage:
            raise ValueError("briefing worker receiver storage identity is unresolved")
        candidate_ordinal = sum(_cc_cfg._instruction_mnemonic(row) == "call" for row in candidate.instructions[:call_index])
        for rows, ordinal in ((expected_rows, expected_ordinal), (candidate_rows, candidate_ordinal)):
            if ordinal >= len(rows) or rows[ordinal].get("slot_displacement") != 0:
                raise ValueError("briefing concrete dispatch does not align with its slot-zero invocation")
    expected_rows[expected_ordinal]["concrete_update_dispatch"] = {
        **common, "target": required, "receiver_storage": required_storage,
    }
    candidate_rows[candidate_ordinal]["concrete_update_dispatch"] = {
        **common, "target": observed, "receiver_storage": observed_storage,
    }
    return expected_rows, candidate_rows


def _member_dispatch_return_contract_facts(
    expected_rows: list[dict[str, Any]], candidate_rows: list[dict[str, Any]],
    *, caller_start: str, candidate: CandidateAssembly,
    bridge: BinaryNinjaBridge, indexes: IdentityIndexes,
) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    """Check the embedded exit state's concrete quit-return dependency.

    This is a behavioral obligation on an existing constructor call row, not
    acceptance of a new physical/logical ICF identity or a synthesized call.
    The expected value is decoded from the live retail table's actual target.
    """
    from _recoil.lib.constructor_dispatch import (
        constant_return,
        exact_table_slots,
        resolve_table_weak_target,
        straight_constructor_member_store,
    )

    if caller_start != "0x42dfa0":
        return _briefing_dispatch_contract_facts(
            expected_rows, candidate_rows, caller_start=caller_start,
            candidate=candidate, bridge=bridge, indexes=indexes,
        )
    member, slot, count = 0x1D0, 4, 10
    if not expected_rows or not candidate_rows:
        raise ValueError("member dispatch dependency lacks its constructor invocation row")
    retail = _cc_listing.parse_assembly(bridge.assembly(caller_start), source="bn")
    retail_chunks = tuple(bytes.fromhex(" ".join(row.bytes)) for row in retail)
    _, table_address = straight_constructor_member_store(retail_chunks, member)
    retail_body = b"".join(retail_chunks)
    if retail_body != _cc_cfg._hexdump_bytes(bridge.hexdump(caller_start, len(retail_body))):
        raise ValueError("member constructor assembly disagrees with live retail bytes")
    definition = candidate.caller_definition
    if definition is None:
        raise ValueError("member constructor lacks its fresh COFF definition")
    chunks = tuple(bytes.fromhex(" ".join(row.bytes)) for row in candidate.instructions)
    write_offset, addend = straight_constructor_member_store(chunks, member)
    body = b"".join(chunks)
    tail = definition.data[len(body):]
    if (not definition.data.startswith(body) or len(tail) >= 16
            or (tail and (len(definition.data) % 16 or any(byte != 0x90 for byte in tail)))):
        raise ValueError("member constructor listing/COFF extent mismatch")
    refs = [row for row in definition.relocations if row.offset == write_offset]
    if len(refs) != 1 or refs[0].type != IMAGE_REL_I386_DIR32 or addend:
        raise ValueError("member vptr lacks its exact table relocation")
    table = candidate.vftable_definitions.get(refs[0].symbol_name)
    if table is None:
        raise ValueError("member vptr lacks its emitted C++ table")
    exact_table_slots(table.data, table.relocations, count)
    target_name = resolve_table_weak_target(
        next(row for row in table.relocations if row.offset == slot * 4), definition.coff_symbols,
    )
    if target_name != "?OnUpdateShouldQuit@RecoilApp_LeaveNetworkState@@UAEHXZ":
        raise ValueError("exit state table does not dispatch its concrete quit override")
    definitions = [row for row in definition.coff_symbols
                   if row.name == target_name and row.section_number > 0]
    if (len(definitions) != 1 or definitions[0].value != 0
            or definitions[0].storage_class != IMAGE_SYM_CLASS_EXTERNAL
            or definitions[0].symbol_type != 0x20 or definitions[0].section_name != ".text"):
        raise ValueError("quit callback lacks its unique complete function definition")
    callback = definitions[0]
    if any(row.section_number == callback.section_number and row.name != target_name
           and row.storage_class == IMAGE_SYM_CLASS_EXTERNAL for row in definition.coff_symbols):
        raise ValueError("quit callback shares an ambiguous COFF extent")
    observed = constant_return(callback.section_data, callback.section_relocations)
    raw_cell = _cc_cfg._hexdump_bytes(bridge.hexdump(normalize_address(table_address + slot * 4), 4))
    if len(raw_cell) != 4:
        raise ValueError("quit callback retail table cell is incomplete")
    target_address = normalize_address(int.from_bytes(raw_cell, "little"))
    callback_rows = _cc_listing.parse_assembly(bridge.assembly(target_address), source="bn")
    callback_body = b"".join(bytes.fromhex(" ".join(row.bytes)) for row in callback_rows)
    if callback_body != _cc_cfg._hexdump_bytes(bridge.hexdump(target_address, len(callback_body))):
        raise ValueError("quit callback retail assembly disagrees with live bytes")
    required = constant_return(callback_body)
    common = {"receiver_offset": member, "slot_displacement": slot * 4, "cleanup_bytes": 0}
    expected_rows[0]["member_dispatch_return"] = {**common, "return_value": required}
    candidate_rows[0]["member_dispatch_return"] = {**common, "return_value": observed}
    return expected_rows, candidate_rows


def _constructor_dispatch_contract_facts(
    expected: Sequence[Mapping[str, Any]],
    candidate_contract: Sequence[Mapping[str, Any]],
    *,
    caller_start: str,
    candidate: CandidateAssembly,
    bridge: BinaryNinjaBridge,
    indexes: IdentityIndexes,
) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    from _recoil.lib.constructor_dispatch import (
        exact_table_slots,
        leaf_constructor_vptr_write,
        resolve_table_weak_target,
        validate_leaf_listing_body,
    )

    expected_rows = [dict(row) for row in expected]
    candidate_rows = [dict(row) for row in candidate_contract]
    obligation = _cc_catalog._CONSTRUCTOR_DISPATCH_OBLIGATIONS.get(caller_start)
    if obligation is None:
        return _member_dispatch_return_contract_facts(
            expected_rows, candidate_rows, caller_start=caller_start,
            candidate=candidate, bridge=bridge, indexes=indexes,
        )
    if len(expected_rows) != 1 or len(candidate_rows) != 1:
        raise ValueError("constructor dispatch dependency requires its one initializer invocation")
    constructor_address, constructor_symbol, slot_count, selected_slots = obligation
    retail = _cc_listing.parse_assembly(bridge.assembly(constructor_address), source="bn")
    retail_chunks = tuple(bytes.fromhex(" ".join(row.bytes)) for row in retail)
    _, table_address = leaf_constructor_vptr_write(retail_chunks)
    if b"".join(retail_chunks) != _cc_cfg._hexdump_bytes(bridge.hexdump(constructor_address, sum(map(len, retail_chunks)))):
        raise ValueError("constructor dispatch retail assembly disagrees with live bytes")
    definition = candidate.constructor_definitions.get(constructor_symbol)
    if definition is None:
        raise ValueError(f"missing concrete constructor dependency: {constructor_symbol}")
    chunks = tuple(bytes.fromhex(" ".join(row.bytes)) for row in definition.instructions)
    write_offset, addend = leaf_constructor_vptr_write(chunks)
    validate_leaf_listing_body(chunks, definition.data, definition.relocations)
    if (definition.section_size != len(definition.data)
            or definition.section_external_functions != (constructor_symbol,)):
        raise ValueError("constructor dispatch requires an exclusive complete COFF definition")
    references = [relocation for relocation in definition.relocations if relocation.offset == write_offset]
    if len(references) != 1 or references[0].type != IMAGE_REL_I386_DIR32 or addend != 0:
        raise ValueError("constructor vptr lacks its exact DIR32 table relocation")
    if len(definition.relocations) != 1:
        raise ValueError("leaf constructor has unexplained additional relocations")
    table_symbol = references[0].symbol_name
    table = candidate.vftable_definitions.get(table_symbol)
    if table is None or not table_symbol.startswith("??_7"):
        raise ValueError("constructor vptr does not reference an emitted C++ dispatch table")
    target_names = exact_table_slots(table.data, table.relocations, slot_count)
    raw_table = _cc_cfg._hexdump_bytes(bridge.hexdump(normalize_address(table_address), slot_count * 4))
    if len(raw_table) != slot_count * 4:
        raise ValueError("constructor dispatch retail table read is incomplete")
    expected_slots = {}
    candidate_slots = {}
    unresolved_slots = []
    storage = f"constructor-table:{normalize_address(table_address)}"
    for slot, name in enumerate(target_names):
        if slot not in selected_slots:
            continue
        address = normalize_address(int.from_bytes(raw_table[slot * 4:slot * 4 + 4], "little"))
        expected_identity = indexes.reviewed_authored_icf_by_vtable_selector.get(
            (storage, slot * 4),
            indexes.reviewed_icf_group_by_address.get(address, indexes.by_address.get(address)),
        )
        relocation = next(row for row in table.relocations if row.offset == slot * 4)
        resolved_name = resolve_table_weak_target(relocation, definition.coff_symbols)
        candidate_identity = indexes.by_candidate_name.get(resolved_name)
        candidate_identity = indexes.reviewed_icf_group_by_logical_identity.get(candidate_identity, candidate_identity)
        if not expected_identity or not candidate_identity:
            unresolved_slots.append({"slot": slot, "retail": address,
                                     "expected_identity": expected_identity,
                                     "candidate_name": name, "resolved_name": resolved_name,
                                     "candidate_identity": candidate_identity})
        expected_slots[str(slot)] = expected_identity
        candidate_slots[str(slot)] = candidate_identity
    if unresolved_slots:
        raise ValueError(f"unresolved constructor dispatch slots: {unresolved_slots!r}")
    expected_rows[0]["constructor_dispatch"] = {
        "constructor": indexes.by_address.get(constructor_address), "receiver_offset": 0,
        "slot_count": slot_count, "selected_slot_targets": expected_slots,
    }
    candidate_rows[0]["constructor_dispatch"] = {
        "constructor": indexes.by_candidate_name.get(constructor_symbol), "receiver_offset": 0,
        "slot_count": slot_count, "selected_slot_targets": candidate_slots,
    }
    return expected_rows, candidate_rows


def _provider_argument_contract_facts(
    contract: Sequence[Mapping[str, Any]],
    instructions: Sequence[Instruction],
    *,
    caller_start: str,
    caller_end_exclusive: str,
    candidate: CandidateAssembly | None = None,
) -> list[dict[str, Any]]:
    from _recoil.lib.call_argument_bits import ArgumentBits

    obligations = _cc_catalog._PROVIDER_ARGUMENT_OBLIGATIONS.get(caller_start)
    result = [dict(row) for row in contract]
    if not obligations:
        return result
    source = "cod" if candidate is not None else "bn"
    addresses = (
        _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
        if candidate is not None
        else _cc_cfg._instruction_runtime_addresses(instructions, source="bn", caller_start=address_value(caller_start))
    )
    if any(value is None for value in addresses) or len(set(addresses)) != len(addresses):
        raise ValueError("provider argument proof requires exact instruction addresses")
    successors, unresolved = _cc_cfg._exact_invocation_cfg(
        instructions,
        instruction_addresses=addresses,
        instruction_index_by_address={value: index for index, value in enumerate(addresses)},
        source=source,
        caller_start=0 if candidate is not None else address_value(caller_start),
        caller_end=(max(addresses) + len(instructions[-1].bytes)) if candidate is not None else address_value(caller_end_exclusive),
        local_control_flow_indices=candidate.local_control_flow_indices if candidate is not None else frozenset(),
        local_control_flow_targets=candidate.local_control_flow_targets if candidate is not None else {},
    )
    calls = [index for index, instruction in enumerate(instructions) if _cc_cfg._instruction_mnemonic(instruction) == "call"]
    if len(calls) != len(contract):
        raise ValueError("provider argument proof cannot align raw calls with invocation census")
    proof = ArgumentBits.from_machine(instructions, successors, unresolved)
    for ordinal, arguments in obligations.items():
        if ordinal >= len(calls):
            raise ValueError("provider argument obligation has no call occurrence")
        result[ordinal]["argument_bits"] = {
            str(argument): json_evidence_value(proof.stack_argument(calls[ordinal], argument))
            for argument in arguments
        }
    return result


def compare_call_contracts(
    expected: Sequence[Mapping[str, Any]],
    candidate: Sequence[Mapping[str, Any]],
    *,
    reviewed_direct_target_equivalences: Mapping[str, str] | None = None,
    reviewed_provider_iat_equivalences: Mapping[str, str] | None = None,
) -> dict[str, Any]:
    """Discharge retail obligations against independently canonicalized facts.

    Every field remains a constraint, including selected argument and returned
    value dependencies. Only explicitly unknown retail cleanup is unconstrained.
    Targetless dispatch still requires its exact receiver/storage and slot.
    """

    expected_rows = [dict(item) for item in expected]
    candidate_rows = [dict(item) for item in candidate]
    count = max(len(expected_rows), len(candidate_rows))
    results = [_cc_proofs.prove(_cc_proofs.Obligation("invocation-population",
        _cc_proofs.Fact("retail", "call-count", len(expected_rows), "body")),
        _cc_proofs.Fact("candidate", "call-count", len(candidate_rows), "body"))]
    first_divergence = None
    families = {
        "ordinal": "invocation-order", "form": "physical-invocation",
        "dispatch": "physical-invocation", "identity_kind": "target-provider-abi",
        "target_identity": "target-provider-abi", "storage_identity": "receiver-storage",
        "slot_displacement": "receiver-vptr-slot", "cleanup_bytes": "cleanup",
        "argument_bits": "selected-argument-bits", "constructor_dispatch": "constructor-lineage",
        "concrete_update_dispatch": "constructor-lineage", "member_dispatch_return": "return-dependency",
    }
    for index in range(count):
        expected_row = expected_rows[index] if index < len(expected_rows) else None
        candidate_row = candidate_rows[index] if index < len(candidate_rows) else None
        retail, observed = _normalize_call_contract_pair(expected_row, candidate_row,
            reviewed_direct_target_equivalences=reviewed_direct_target_equivalences,
            reviewed_provider_iat_equivalences=reviewed_provider_iat_equivalences)
        if retail is not None:
            for field in sorted(retail.keys() | (observed.keys() if observed is not None else set())):
                required = not (field == "cleanup_bytes" and retail.get(field) is None)
                expected_fact = _cc_proofs.Fact("retail", field, retail.get(field), f"call:{index}")
                candidate_fact = (_cc_proofs.Fact("candidate", field, observed.get(field), f"call:{index}")
                                  if observed is not None and field in observed else None)
                results.append(_cc_proofs.prove(_cc_proofs.Obligation(families.get(field, "selected-dependency"),
                                               expected_fact, required), candidate_fact))
        if retail != observed and first_divergence is None:
            first_divergence = {"ordinal": index, "expected": expected_row, "candidate": candidate_row,
                                "kind": "missing" if candidate_row is None else "extra" if expected_row is None else "mismatch"}
    present_families = {result.obligation.family for result in results}
    for family in sorted(set(families.values()) - present_families):
        results.append(_cc_proofs.prove(_cc_proofs.Obligation(family,
            _cc_proofs.Fact("retail", "selected-obligation", False, "body"), required=False), None))
    passed = first_divergence is None and all(result.status in {
        _cc_proofs.ProofStatus.PROVEN, _cc_proofs.ProofStatus.NOT_APPLICABLE} for result in results)
    return {"passed": passed, "first_divergence": first_divergence,
            "proof_results": [result.as_json() for result in results]}
