"""Recoil call-contract recoil callbacks evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import receiver_candidate as _cc_receiver_candidate
from _recoil.call_contract import receiver_cursor as _cc_receiver_cursor
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions
from _recoil.call_contract import reviewed_dispatch as _cc_reviewed_dispatch

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        IdentityIndexes,
        ReviewedExactIndirectStorageBridge,
        ReviewedLoopVptrStorageBridge,
        ReviewedRegisterCallStorageBridge,
    )

from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import Instruction
from _recoil.lib.progress import address_value, normalize_address


def _reviewed_r4008_zinterp_ctx_callback_bridges(
    retail_instructions: Sequence[Instruction],
    *,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedExactIndirectStorageBridge]:
    """Publish only the two exact targetless ctx+0x70 callback calls."""
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge

    start = normalize_address(caller_start)
    specs = {
        "0x4c1b30": ("0x4c1b50", "0x4c1b45"),
        "0x4c5520": ("0x4c5550", "0x4c553c"),
    }
    selected = specs.get(start)
    if selected is None:
        return {}
    # Reuse the finite raw-byte/guard/cleanup proof; it returns no storage
    # binding because the source field is runtime-selected, not tracker data.
    _cc_reviewed_dispatch._reviewed_r3994_retail_register_storage_bridges(
        retail_instructions,
        caller_start=start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    if normalize_address(caller_end_exclusive) != selected[0]:
        raise ValueError("reviewed zInterp ctx callback extent drifted")
    return {
        selected[1]: ReviewedLoopVptrStorageBridge(
            register="eax",
            storage_identity="dynamic:zInterp_Context+0x70-callback",
            slot_displacement=0,
            assembly_source="bn",
        )
    }


def _exact_zinterp_logf_parameter_load(
    instruction: Instruction,
    *,
    destination: str,
    encoded_displacement: int,
    vc5_symbol: str,
) -> bool:
    """Accept one exact numeric or VC5 symbolic Logf parameter load.

    VC5 COD renders these incoming parameters as ``_ctx$[esp-4]`` and
    ``_fmt$[esp-4]`` even though the encoded SIB displacements are +4 and +8.
    Keep that presentation quirk local to this fully byte-gated finite body.
    """

    if _cc_receiver_instructions._exact_stack_slot_load(instruction) == (
        destination,
        encoded_displacement,
    ):
        return True
    return _cc_receiver_instructions._exact_vc5_symbolic_stack_slot_load(instruction) == (
        encoded_displacement,
        destination,
        vc5_symbol,
        -4,
    )


def _zinterp_logf_candidate_ctx_callback_bridge(
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    existing_bridges: Mapping[str, ReviewedLoopVptrStorageBridge] | None = None,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Bind only Logf's exact candidate ctx+0x70 callback provenance.

    This bridge deliberately does not change the candidate invocation form.
    The current VC5 body tail-jumps through EAX, while retail calls EAX; normal
    contract extraction retains that distinction for the live comparison.
    """
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge

    start = normalize_address(caller_start)
    if start != _cc_catalog._ZINTERP_LOGF_CALLER_START:
        return {}
    definition = candidate.caller_definition
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    rows = candidate.instructions
    physical_bodies = {
        "fastcall-tail": (
            _cc_catalog._ZINTERP_LOGF_FASTCALL_TAIL_BODY,
            _cc_catalog._ZINTERP_LOGF_FASTCALL_TAIL_BODY
            + b"\x90" * (0x20 - len(_cc_catalog._ZINTERP_LOGF_FASTCALL_TAIL_BODY)),
        ),
        "cdecl-call": (
            _cc_catalog._ZINTERP_LOGF_CDECL_CALL_BODY,
            _cc_catalog._ZINTERP_LOGF_CDECL_CALL_BODY
            + b"\x90" * (0x20 - len(_cc_catalog._ZINTERP_LOGF_CDECL_CALL_BODY)),
        ),
    }
    shape = next(
        (
            name
            for name, representations in physical_bodies.items()
            if definition is not None
            and definition.data in representations
        ),
        "",
    )
    semantic_body = (
        physical_bodies[shape][0]
        if shape in physical_bodies
        else b""
    )
    callback_offset = normalize_address(
        "0x13" if shape == "fastcall-tail" else "0x15"
    )
    candidate_names = [
        name
        for name, identity in indexes.by_candidate_name.items()
        if identity == caller_identity
    ]
    caller_addresses = [
        address
        for address, identity in indexes.by_address.items()
        if identity == caller_identity
    ]
    try:
        listed_body: bytes | None = b"".join(
            bytes(int(item, 16) for item in row.bytes) for row in rows
        )
    except (TypeError, ValueError):
        listed_body = None
    expected_row_count = 8 if shape == "fastcall-tail" else 11
    expected_offsets = (
        (0, 4, 7, 9, 0x0B, 0x0F, 0x13, 0x15)
        if shape == "fastcall-tail"
        else (0, 4, 7, 9, 0x0B, 0x0F, 0x13, 0x14, 0x15, 0x17, 0x1A)
    )
    # CandidateCallerDefinition carries the object/TU-wide undefined-symbol
    # census, not a per-function dependency list.  This exact body has no
    # relocation-bearing operand, so the empty per-body relocation tuple and
    # all-clear mask below are the authoritative exclusion.  TU neighbors may
    # add or remove unrelated undefined symbols without changing this caller.
    aggregate_checks: dict[str, tuple[bool, Any]] = {
        "caller_identity": (
            caller_identity == _cc_catalog._ZINTERP_LOGF_CALLER_IDENTITY,
            caller_identity,
        ),
        "caller_end_exclusive": (
            normalize_address(caller_end_exclusive)
            == _cc_catalog._ZINTERP_LOGF_CALLER_END_EXCLUSIVE,
            normalize_address(caller_end_exclusive),
        ),
        "address_identity": (
            indexes.by_address.get(start) == caller_identity,
            indexes.by_address.get(start),
        ),
        "caller_addresses": (
            caller_addresses == [_cc_catalog._ZINTERP_LOGF_CALLER_START],
            tuple(caller_addresses),
        ),
        "candidate_symbol_identity": (
            indexes.by_candidate_name.get(_cc_catalog._ZINTERP_LOGF_CANDIDATE_SYMBOL)
            == caller_identity,
            indexes.by_candidate_name.get(_cc_catalog._ZINTERP_LOGF_CANDIDATE_SYMBOL),
        ),
        "candidate_names": (
            candidate_names == [_cc_catalog._ZINTERP_LOGF_CANDIDATE_SYMBOL],
            tuple(candidate_names),
        ),
        "provider": (
            caller_identity not in indexes.provider_ids,
            caller_identity in indexes.provider_ids,
        ),
        "definition": (definition is not None, definition is not None),
        "definition_symbol": (
            definition is not None
            and definition.symbol == _cc_catalog._ZINTERP_LOGF_CANDIDATE_SYMBOL,
            definition.symbol if definition is not None else None,
        ),
        "body_shape": (
            shape in {"fastcall-tail", "cdecl-call"},
            (
                definition.data.hex()
                if definition is not None
                else None
            ),
        ),
        "relocations": (
            definition is not None and not definition.relocations,
            definition.relocations if definition is not None else None,
        ),
        "relocation_mask": (
            definition is not None
            and len(definition.relocation_mask) == len(definition.data)
            and not any(definition.relocation_mask),
            (
                (
                    len(definition.relocation_mask),
                    sum(bool(item) for item in definition.relocation_mask),
                )
                if definition is not None
                else None
            ),
        ),
        "instruction_count": (
            len(rows) == expected_row_count,
            len(rows),
        ),
        "instruction_offsets": (offsets == expected_offsets, offsets),
        "listing_body": (
            definition is not None and listed_body == semantic_body,
            listed_body.hex() if listed_body is not None else None,
        ),
    }
    aggregate_drift = {
        name: observed
        for name, (passed, observed) in aggregate_checks.items()
        if not passed
    }
    if aggregate_drift:
        raise ValueError(
            "zInterp Logf candidate ctx callback requires the exact authored "
            "caller identity, symbol, extent, body, and empty relocation set; "
            f"drift={aggregate_drift!r}"
        )
    common_lineage_checks: dict[str, tuple[bool, Any]] = {
        "context_root": (
            _exact_zinterp_logf_parameter_load(
                rows[0],
                destination="eax",
                encoded_displacement=4,
                vc5_symbol="_ctx$",
            ),
            (
                _cc_receiver_instructions._exact_stack_slot_load(rows[0]),
                _cc_receiver_instructions._exact_vc5_symbolic_stack_slot_load(rows[0]),
                rows[0].raw_text,
                tuple(rows[0].bytes),
            ),
        ),
        "callback_field": (
            _cc_receiver_candidate._exact_register_memory_load(rows[1]) == ("eax", "eax", 0x70),
            (
                _cc_receiver_candidate._exact_register_memory_load(rows[1]),
                rows[1].raw_text,
                tuple(rows[1].bytes),
            ),
        ),
        "callback_test": (
            _cc_receiver_cursor._exact_test_same_register(rows[2]) == "eax",
            (_cc_receiver_cursor._exact_test_same_register(rows[2]), rows[2].raw_text),
        ),
        "null_branch": (
            _cc_cfg._instruction_mnemonic(rows[3]) in {"je", "jz"},
            rows[3].raw_text,
        ),
    }
    if shape == "fastcall-tail":
        shape_lineage_checks: dict[str, tuple[bool, Any]] = {
            "null_branch_bytes": (
                tuple(rows[3].bytes) == ("74", "0a"),
                tuple(rows[3].bytes),
            ),
            "format_argument": (
                _exact_zinterp_logf_parameter_load(
                    rows[4],
                    destination="ecx",
                    encoded_displacement=8,
                    vc5_symbol="_fmt$",
                ),
                (
                    _cc_receiver_instructions._exact_stack_slot_load(rows[4]),
                    _cc_receiver_instructions._exact_vc5_symbolic_stack_slot_load(rows[4]),
                    rows[4].raw_text,
                ),
            ),
            "varargs_argument": (
                _cc_cfg._instruction_mnemonic(rows[5]) == "lea"
                and tuple(rows[5].bytes) == ("8d", "54", "24", "0c"),
                (rows[5].raw_text, tuple(rows[5].bytes)),
            ),
            "eax_clobber": (
                not any(
                    _cc_cfg._instruction_may_clobber_register(row, "eax")
                    for row in rows[2:6]
                ),
                tuple(
                    row.raw_text
                    for row in rows[2:6]
                    if _cc_cfg._instruction_may_clobber_register(row, "eax")
                ),
            ),
            "invocation": (
                _cc_cfg._instruction_mnemonic(rows[6]) == "jmp"
                and _cc_cfg._instruction_operand(rows[6]).strip().lower() == "eax"
                and _cc_cfg._exact_invocation_encoding(rows[6], mnemonic="jmp"),
                (rows[6].raw_text, tuple(rows[6].bytes)),
            ),
            "return": (
                _cc_cfg._instruction_mnemonic(rows[7]) in {"ret", "retn"},
                rows[7].raw_text,
            ),
        }
    else:
        shape_lineage_checks = {
            "null_branch_bytes": (
                tuple(rows[3].bytes) == ("74", "0f"),
                tuple(rows[3].bytes),
            ),
            "format_argument": (
                _exact_zinterp_logf_parameter_load(
                    rows[4],
                    destination="edx",
                    encoded_displacement=8,
                    vc5_symbol="_fmt$",
                ),
                (
                    _cc_receiver_instructions._exact_stack_slot_load(rows[4]),
                    _cc_receiver_instructions._exact_vc5_symbolic_stack_slot_load(rows[4]),
                    rows[4].raw_text,
                ),
            ),
            "varargs_argument": (
                _cc_cfg._instruction_mnemonic(rows[5]) == "lea"
                and tuple(rows[5].bytes) == ("8d", "4c", "24", "0c"),
                (rows[5].raw_text, tuple(rows[5].bytes)),
            ),
            "argument_pushes": (
                tuple(rows[6].bytes) == ("51",)
                and tuple(rows[7].bytes) == ("52",),
                (rows[6].raw_text, rows[7].raw_text),
            ),
            "eax_clobber": (
                not any(
                    _cc_cfg._instruction_may_clobber_register(row, "eax")
                    for row in rows[2:8]
                ),
                tuple(
                    row.raw_text
                    for row in rows[2:8]
                    if _cc_cfg._instruction_may_clobber_register(row, "eax")
                ),
            ),
            "invocation": (
                _cc_cfg._instruction_mnemonic(rows[8]) == "call"
                and _cc_cfg._instruction_operand(rows[8]).strip().lower() == "eax"
                and _cc_cfg._exact_invocation_encoding(rows[8], mnemonic="call"),
                (rows[8].raw_text, tuple(rows[8].bytes)),
            ),
            "cleanup": (
                tuple(rows[9].bytes) == ("83", "c4", "08"),
                (rows[9].raw_text, tuple(rows[9].bytes)),
            ),
            "return": (
                _cc_cfg._instruction_mnemonic(rows[10]) in {"ret", "retn"},
                rows[10].raw_text,
            ),
        }
    lineage_drift = {
        name: observed
        for name, (passed, observed) in {
            **common_lineage_checks,
            **shape_lineage_checks,
        }.items()
        if not passed
    }
    if lineage_drift:
        raise ValueError(
            "zInterp Logf candidate ctx callback field/root/register/use "
            f"lineage drifted; drift={lineage_drift!r}"
        )
    prior = (existing_bridges or {}).get(callback_offset)
    if prior is not None:
        raise ValueError(
            "zInterp Logf candidate ctx callback has duplicate or conflicting "
            f"existing provenance at {callback_offset}: {prior!r}"
        )
    return {
        callback_offset: ReviewedLoopVptrStorageBridge(
            register="eax",
            storage_identity=_cc_catalog._ZINTERP_CONTEXT_CALLBACK_IDENTITY,
            slot_displacement=0,
            assembly_source="cod",
        )
    }


def _zinterp_report_errorf_candidate_ctx_callback_bridge(
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    existing_bridges: Mapping[str, ReviewedLoopVptrStorageBridge] | None = None,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Bind only ReportErrorf's exact candidate ctx+0x70 callback call.

    Retail and the current VC5 candidate both load the callback from the
    incoming context's +0x70 field and call it through EAX.  The callback is
    runtime-selected, so this finite caller proof publishes targetless storage
    provenance; it does not grant generic EAX provenance to any other caller.
    """
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge

    start = normalize_address(caller_start)
    if start != _cc_catalog._ZINTERP_REPORT_ERRORF_CALLER_START:
        return {}
    definition = candidate.caller_definition
    rows = candidate.instructions
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    candidate_names = [
        name
        for name, identity in indexes.by_candidate_name.items()
        if identity == caller_identity
    ]
    caller_addresses = [
        address
        for address, identity in indexes.by_address.items()
        if identity == caller_identity
    ]
    try:
        listed_body: bytes | None = b"".join(
            bytes(int(item, 16) for item in row.bytes) for row in rows
        )
    except (TypeError, ValueError):
        listed_body = None
    aggregate_checks: dict[str, tuple[bool, Any]] = {
        "caller_identity": (
            caller_identity == _cc_catalog._ZINTERP_REPORT_ERRORF_CALLER_IDENTITY,
            caller_identity,
        ),
        "caller_end_exclusive": (
            normalize_address(caller_end_exclusive)
            == _cc_catalog._ZINTERP_REPORT_ERRORF_CALLER_END_EXCLUSIVE,
            normalize_address(caller_end_exclusive),
        ),
        "address_identity": (
            indexes.by_address.get(start) == caller_identity,
            indexes.by_address.get(start),
        ),
        "caller_addresses": (
            caller_addresses == [_cc_catalog._ZINTERP_REPORT_ERRORF_CALLER_START],
            tuple(caller_addresses),
        ),
        "candidate_symbol_identity": (
            indexes.by_candidate_name.get(
                _cc_catalog._ZINTERP_REPORT_ERRORF_CANDIDATE_SYMBOL
            )
            == caller_identity,
            indexes.by_candidate_name.get(
                _cc_catalog._ZINTERP_REPORT_ERRORF_CANDIDATE_SYMBOL
            ),
        ),
        "candidate_names": (
            candidate_names == [_cc_catalog._ZINTERP_REPORT_ERRORF_CANDIDATE_SYMBOL],
            tuple(candidate_names),
        ),
        "provider": (
            caller_identity not in indexes.provider_ids,
            caller_identity in indexes.provider_ids,
        ),
        "definition": (definition is not None, definition is not None),
        "definition_symbol": (
            definition is not None
            and definition.symbol == _cc_catalog._ZINTERP_REPORT_ERRORF_CANDIDATE_SYMBOL,
            definition.symbol if definition is not None else None,
        ),
        "body": (
            definition is not None
            and definition.data == _cc_catalog._ZINTERP_REPORT_ERRORF_PHYSICAL_BODY,
            definition.data.hex() if definition is not None else None,
        ),
        "relocations": (
            definition is not None and not definition.relocations,
            definition.relocations if definition is not None else None,
        ),
        "relocation_mask": (
            definition is not None
            and len(definition.relocation_mask) == len(definition.data)
            and not any(definition.relocation_mask),
            (
                (
                    len(definition.relocation_mask),
                    sum(bool(item) for item in definition.relocation_mask),
                )
                if definition is not None
                else None
            ),
        ),
        "section_extent": (
            definition is not None
            and definition.section_start == 0
            and definition.section_end == len(definition.data),
            (
                (definition.section_start, definition.section_end)
                if definition is not None
                else None
            ),
        ),
        "instruction_count": (len(rows) == 12, len(rows)),
        "instruction_offsets": (
            offsets
            == (0, 4, 0x0B, 0x0E, 0x10, 0x12, 0x16, 0x1A, 0x1B, 0x1C, 0x1E, 0x21),
            offsets,
        ),
        "listing_body": (
            listed_body == _cc_catalog._ZINTERP_REPORT_ERRORF_SEMANTIC_BODY,
            listed_body.hex() if listed_body is not None else None,
        ),
    }
    aggregate_drift = {
        name: observed
        for name, (passed, observed) in aggregate_checks.items()
        if not passed
    }
    if aggregate_drift:
        raise ValueError(
            "zInterp ReportErrorf candidate ctx callback requires the exact "
            "authored caller identity, symbol, extent, body, padding, and "
            f"empty relocation set; drift={aggregate_drift!r}"
        )

    lineage_checks: dict[str, tuple[bool, Any]] = {
        "context_root": (
            _exact_zinterp_logf_parameter_load(
                rows[0],
                destination="eax",
                encoded_displacement=4,
                vc5_symbol="_ctx$",
            ),
            rows[0].raw_text,
        ),
        "error_flag_write": (
            tuple(rows[1].bytes)
            == ("c7", "40", "10", "01", "00", "00", "00"),
            (rows[1].raw_text, tuple(rows[1].bytes)),
        ),
        "callback_field": (
            _cc_receiver_candidate._exact_register_memory_load(rows[2]) == ("eax", "eax", 0x70),
            (_cc_receiver_candidate._exact_register_memory_load(rows[2]), rows[2].raw_text),
        ),
        "callback_test": (
            _cc_receiver_cursor._exact_test_same_register(rows[3]) == "eax",
            (_cc_receiver_cursor._exact_test_same_register(rows[3]), rows[3].raw_text),
        ),
        "null_branch": (
            _cc_cfg._instruction_mnemonic(rows[4]) in {"je", "jz"}
            and tuple(rows[4].bytes) == ("74", "0f"),
            (rows[4].raw_text, tuple(rows[4].bytes)),
        ),
        "format_argument": (
            _exact_zinterp_logf_parameter_load(
                rows[5],
                destination="edx",
                encoded_displacement=8,
                vc5_symbol="_fmt$",
            ),
            rows[5].raw_text,
        ),
        "varargs_argument": (
            _cc_cfg._instruction_mnemonic(rows[6]) == "lea"
            and tuple(rows[6].bytes) == ("8d", "4c", "24", "0c"),
            (rows[6].raw_text, tuple(rows[6].bytes)),
        ),
        "argument_pushes": (
            tuple(rows[7].bytes) == ("51",)
            and tuple(rows[8].bytes) == ("52",),
            (rows[7].raw_text, rows[8].raw_text),
        ),
        "eax_lineage": (
            not any(
                _cc_cfg._instruction_may_clobber_register(row, "eax")
                for row in rows[3:9]
            ),
            tuple(
                row.raw_text
                for row in rows[3:9]
                if _cc_cfg._instruction_may_clobber_register(row, "eax")
            ),
        ),
        "invocation": (
            _cc_cfg._instruction_mnemonic(rows[9]) == "call"
            and _cc_cfg._instruction_operand(rows[9]).strip().lower() == "eax"
            and _cc_cfg._exact_invocation_encoding(rows[9], mnemonic="call"),
            (rows[9].raw_text, tuple(rows[9].bytes)),
        ),
        "cleanup": (
            tuple(rows[10].bytes) == ("83", "c4", "08"),
            (rows[10].raw_text, tuple(rows[10].bytes)),
        ),
        "return": (
            _cc_cfg._instruction_mnemonic(rows[11]) in {"ret", "retn"},
            rows[11].raw_text,
        ),
    }
    lineage_drift = {
        name: observed
        for name, (passed, observed) in lineage_checks.items()
        if not passed
    }
    if lineage_drift:
        raise ValueError(
            "zInterp ReportErrorf candidate ctx callback field/root/register/"
            f"argument/use lineage drifted; drift={lineage_drift!r}"
        )
    callback_offset = "0x1c"
    prior = (existing_bridges or {}).get(callback_offset)
    if prior is not None:
        raise ValueError(
            "zInterp ReportErrorf candidate ctx callback has duplicate or "
            f"conflicting existing provenance at {callback_offset}: {prior!r}"
        )
    return {
        callback_offset: ReviewedLoopVptrStorageBridge(
            register="eax",
            storage_identity=_cc_catalog._ZINTERP_CONTEXT_CALLBACK_IDENTITY,
            slot_displacement=0,
            assembly_source="cod",
        )
    }


def _require_zcom_interface_map_resolver_instruction_body(
    instructions: Sequence[Instruction],
    *,
    source: str,
) -> None:
    """Require the complete exact resolver loop and invocation population."""

    start = address_value(_cc_catalog._ZCOM_INTERFACE_MAP_CALLER_START)
    addresses = _cc_cfg._instruction_runtime_addresses(
        instructions,
        source=source,
        caller_start=start,
    )
    try:
        bodies = tuple(bytes(int(item, 16) for item in row.bytes) for row in instructions)
    except (TypeError, ValueError) as exc:
        raise ValueError(
            "zCom interface-map resolver callback requires exact instruction bytes"
        ) from exc
    expected_addresses: list[int] = []
    cursor = start
    for body in bodies:
        expected_addresses.append(cursor)
        cursor += len(body)
    if (
        tuple(addresses) != tuple(expected_addresses)
        or b"".join(bodies) != _cc_catalog._ZCOM_INTERFACE_MAP_INSTRUCTION_BODY
        or cursor != start + len(_cc_catalog._ZCOM_INTERFACE_MAP_INSTRUCTION_BODY)
    ):
        raise ValueError(
            "zCom interface-map resolver callback body, extent, or instruction "
            "coordinates drifted"
        )
    invocation_offsets = tuple(
        address - start
        for address, row in zip(addresses, instructions)
        if _cc_cfg._instruction_mnemonic(row) in {"call", "jmp"}
        and not (
            _cc_cfg._instruction_mnemonic(row) == "jmp"
            and _cc_cfg._exact_local_direct_branch(
                row,
                instruction_index=addresses.index(address),
                instruction_addresses=addresses,
                instruction_index_by_address={
                    item: index for index, item in enumerate(addresses)
                },
                source=source,
                caller_start=start,
                caller_end=address_value(
                    _cc_catalog._ZCOM_INTERFACE_MAP_CALLER_END_EXCLUSIVE
                ),
            )
            is not None
        )
    )
    if invocation_offsets != (0x92, 0xC0):
        raise ValueError(
            "zCom interface-map resolver callback invocation population drifted"
        )


def _zcom_interface_map_resolver_retail_register_bridge(
    retail_instructions: Sequence[Instruction],
    *,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedRegisterCallStorageBridge]:
    """Publish the exact targetless per-entry resolver callback in retail."""
    from _recoil.call_contract.records import ReviewedRegisterCallStorageBridge

    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog._ZCOM_INTERFACE_MAP_CALLER_START:
        return {}
    if (
        normalize_address(caller_end_exclusive)
        != _cc_catalog._ZCOM_INTERFACE_MAP_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_start)
        != _cc_catalog._ZCOM_INTERFACE_MAP_CALLER_IDENTITY
        or _cc_catalog._ZCOM_INTERFACE_MAP_CALLER_IDENTITY in indexes.provider_ids
    ):
        raise ValueError(
            "zCom interface-map resolver callback requires the exact authored "
            "caller identity and extent"
        )
    _require_zcom_interface_map_resolver_instruction_body(
        retail_instructions,
        source="bn",
    )
    return {
        _cc_catalog._ZCOM_INTERFACE_MAP_RESOLVER_RETAIL_CALL:
        ReviewedRegisterCallStorageBridge(
            register="ecx",
            storage_identity=_cc_catalog._ZCOM_INTERFACE_MAP_RESOLVER_STORAGE,
            identity_kind="callback",
            assembly_source="bn",
        )
    }


def _zcom_interface_map_resolver_candidate_register_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedRegisterCallStorageBridge]:
    """Rejoin the same targetless resolver field in exact current COD/COFF."""
    from _recoil.call_contract.records import ReviewedRegisterCallStorageBridge

    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog._ZCOM_INTERFACE_MAP_CALLER_START:
        return {}
    expected_callback = {
        "ordinal": 0,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "callback",
        "target_identity": "",
        "storage_identity": _cc_catalog._ZCOM_INTERFACE_MAP_RESOLVER_STORAGE,
        "slot_displacement": None,
        "cleanup_bytes": None,
    }
    expected_addref = {
        "ordinal": 1,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": _cc_catalog._ZCOM_INTERFACE_MAP_ADDREF_STORAGE,
        "slot_displacement": 4,
        "cleanup_bytes": None,
    }
    definition = candidate.caller_definition
    if (
        caller_identity != _cc_catalog._ZCOM_INTERFACE_MAP_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog._ZCOM_INTERFACE_MAP_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_start) != caller_identity
        or caller_identity in indexes.provider_ids
        or len(expected) != 2
        or dict(expected[0]) != expected_callback
        or dict(expected[1]) != expected_addref
        or definition is None
        or definition.symbol != _cc_catalog._ZCOM_INTERFACE_MAP_CALLER_SYMBOL
        or definition.data != _cc_catalog._ZCOM_INTERFACE_MAP_BODY
        or definition.relocations
        or len(definition.relocation_mask) != len(definition.data)
        or any(definition.relocation_mask)
        or definition.section_start != 0
        or definition.section_end != len(_cc_catalog._ZCOM_INTERFACE_MAP_BODY)
    ):
        raise ValueError(
            "zCom interface-map resolver candidate bridge rejects expected "
            "targetless truth, caller identity, body, section, or relocation population"
        )
    _require_zcom_interface_map_resolver_instruction_body(
        candidate.instructions,
        source="cod",
    )
    return {
        _cc_catalog._ZCOM_INTERFACE_MAP_RESOLVER_CANDIDATE_CALL:
        ReviewedRegisterCallStorageBridge(
            register="ecx",
            storage_identity=_cc_catalog._ZCOM_INTERFACE_MAP_RESOLVER_STORAGE,
            identity_kind="callback",
            assembly_source="cod",
        )
    }


def _zcom_interface_map_addref_candidate_indirect_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedExactIndirectStorageBridge]:
    """Rejoin only the exact targetless AddRef-style vptr call."""
    from _recoil.call_contract.records import ReviewedExactIndirectStorageBridge

    if normalize_address(caller_start) != _cc_catalog._ZCOM_INTERFACE_MAP_CALLER_START:
        return {}
    # The paired register producer owns the whole-body, extent, relocation,
    # and complete two-row expected-contract validation.
    _zcom_interface_map_resolver_candidate_register_bridge(
        expected,
        candidate,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    return {
        _cc_catalog._ZCOM_INTERFACE_MAP_ADDREF_CANDIDATE_CALL:
        ReviewedExactIndirectStorageBridge(
            register="ecx",
            storage_identity=_cc_catalog._ZCOM_INTERFACE_MAP_ADDREF_STORAGE,
            slot_displacement=4,
            assembly_source="cod",
        )
    }
