"""Recoil call-contract abi evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import listing as _cc_listing

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        IdentityIndexes,
        _RetailAuthoredNamespaceAbi,
        _Vc5NamespaceFunctionAbi,
    )

import json
import re
import struct
from pathlib import Path
from typing import Any, Mapping

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_REL32,
    IMAGE_SYM_CLASS_EXTERNAL,
    Instruction,
)
from _recoil.commands.vc5_build import DEFAULT_MANIFEST as DEFAULT_FINAL_BUILD_MANIFEST
from _recoil.lib.binja import BinaryNinjaBridge
from _recoil.lib.progress import (
    AUTHORED_ORDER_DIMENSIONS,
    ProgressDocument,
    ProgressError,
    address_value,
    is_current_accepted_state,
    normalize_address,
)
from _recoil.lib.zeroarg_abi import (
    ZeroArgAbiTarget,
    load_zeroarg_targets,
    manifest_symbol_normalization,
)
from _recoil.lib.zeroarg_abi import eligibility_gates as zeroarg_abi_eligibility_gates


def _decode_vc5_namespace_function_abi(
    decorated_identity: str,
) -> _Vc5NamespaceFunctionAbi | None:
    """Decode one bounded ordinary namespace-function VC5 ABI spelling.

    This intentionally excludes global C-style spellings, members, pointers,
    references, arrays, aggregates, varargs, and templates.  Those forms need
    their own exact proof rather than an optimistic partial demangle.
    """
    from _recoil.call_contract.records import _Vc5NamespaceFunctionAbi

    identity = str(decorated_identity)
    if not identity.startswith("?") or "@@Y" not in identity:
        return None
    head, encoded = identity[1:].split("@@Y", 1)
    components = head.split("@")
    if (
        len(components) < 2
        or any(
            re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", component) is None
            for component in components
        )
        or len(encoded) < 4
        or not encoded.endswith("Z")
    ):
        return None
    calling_convention = {
        "A": "__cdecl",
        "G": "__stdcall",
        "I": "__fastcall",
    }.get(encoded[0])
    return_type = (
        "void"
        if encoded[1] == "X"
        else _cc_catalog._VC5_NAMESPACE_PRIMITIVE_TYPES.get(encoded[1], ("", 0, 0))[0]
    )
    if not calling_convention or not return_type:
        return None
    parameter_encoding = encoded[2:-1]
    if parameter_encoding == "X":
        parameter_codes = ""
    elif parameter_encoding.endswith("@"):
        parameter_codes = parameter_encoding[:-1]
    else:
        return None
    parameters = [
        _cc_catalog._VC5_NAMESPACE_PRIMITIVE_TYPES.get(code)
        for code in parameter_codes
    ]
    if any(parameter is None for parameter in parameters):
        return None
    typed_parameters = [parameter for parameter in parameters if parameter]
    semantic_name = "::".join(
        (*reversed(components[1:]), components[0])
    )
    return _Vc5NamespaceFunctionAbi(
        decorated_identity=identity,
        semantic_name=semantic_name,
        calling_convention=calling_convention,
        return_type=return_type,
        parameter_types=tuple(parameter[0] for parameter in typed_parameters),
        parameter_sizes=tuple(parameter[1] for parameter in typed_parameters),
        parameter_stack_sizes=tuple(
            parameter[2] for parameter in typed_parameters
        ),
    )


def _retail_authored_namespace_abi(
    *,
    address: str,
    end_exclusive: str,
    semantic_name: str,
    bridge: BinaryNinjaBridge,
    cache: dict[tuple[str, str, str], _RetailAuthoredNamespaceAbi],
) -> _RetailAuthoredNamespaceAbi:
    """Read one exact BN name/prototype and unanimous retail RET contract."""
    from _recoil.call_contract.records import _RetailAuthoredNamespaceAbi

    key = (address, end_exclusive, semantic_name)
    cached = cache.get(key)
    if cached is not None:
        return cached

    assembly_payload = bridge.get_json("assembly", name=address)
    function = assembly_payload.get("function")
    function_symbol = (
        function.get("symbol") if isinstance(function, Mapping) else None
    )
    if (
        assembly_payload.get("truncated") is not False
        or not isinstance(function, Mapping)
        or normalize_address(str(function.get("address", ""))) != address
        or function.get("name") != semantic_name
        or function.get("raw_name") != semantic_name
        or not isinstance(function_symbol, Mapping)
        or function_symbol.get("type") != "SymbolType.FunctionSymbol"
        or function_symbol.get("full_name") != semantic_name
        or not isinstance(assembly_payload.get("assembly"), str)
    ):
        raise ValueError(
            "authored decorated ABI bridge requires one exact complete BN "
            f"function/name row for {semantic_name!r} at {address}"
        )
    instructions = tuple(
        _cc_listing.parse_assembly(assembly_payload["assembly"], source="bn")
    )
    instruction_addresses: list[int] = []
    for instruction in instructions:
        raw_instruction_address = _cc_cfg._source_instruction_address(instruction)
        if not raw_instruction_address:
            raise ValueError(
                "authored decorated ABI bridge requires complete address-labelled "
                f"retail assembly at {address}"
            )
        instruction_address = address_value(raw_instruction_address)
        instruction_end = instruction_address + len(instruction.bytes)
        if (
            instruction_address < address_value(address)
            or instruction_end > address_value(end_exclusive)
            or not instruction.bytes
        ):
            raise ValueError(
                "authored decorated ABI bridge retail assembly escapes its "
                f"governed extent at {address}"
            )
        instruction_addresses.append(instruction_address)
    if (
        not instructions
        or instruction_addresses[0] != address_value(address)
        or len(instruction_addresses) != len(set(instruction_addresses))
        or instruction_addresses != sorted(instruction_addresses)
    ):
        raise ValueError(
            "authored decorated ABI bridge requires one ordered complete "
            f"retail function body at {address}"
        )
    cleanup_rows: list[int] = []
    for instruction in instructions:
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        if mnemonic not in {"ret", "retn"}:
            continue
        operand = _cc_cfg._instruction_operand(instruction).strip()
        try:
            cleanup = int(operand, 0) if operand else 0
        except ValueError as exc:
            raise ValueError(
                "authored decorated ABI bridge has malformed retail RET cleanup"
            ) from exc
        if (
            mnemonic not in {"ret", "retn"}
            or len(instruction.bytes) != 3
            or instruction.bytes[0].casefold() != "c2"
            or cleanup <= 0
            or cleanup > 0xFFFF
            or tuple(
                item.casefold() for item in instruction.bytes[1:]
            )
            != (f"{cleanup & 0xff:02x}", f"{cleanup >> 8:02x}")
        ):
            raise ValueError(
                "authored decorated ABI bridge requires exact callee-pop "
                f"retail RET encodings at {address}"
            )
        cleanup_rows.append(cleanup)
    if not cleanup_rows or len(set(cleanup_rows)) != 1:
        raise ValueError(
            "authored decorated ABI bridge requires unanimous nonzero retail "
            f"RET cleanup at {address}"
        )
    cleanup_bytes = cleanup_rows[0]

    stack_payload = bridge.get_json("getStackFrameVars", address=address)
    stack_rows = stack_payload.get("stack_frame_vars")
    exact_stack_rows = [
        row
        for row in stack_rows or []
        if isinstance(row, Mapping)
        and normalize_address(str(row.get("addr", ""))) == address
        and isinstance(row.get("vars"), list)
    ]
    if len(exact_stack_rows) != 1:
        raise ValueError(
            "authored decorated ABI bridge requires one exact BN stack "
            f"prototype at {address}"
        )
    variables = exact_stack_rows[0]["vars"]
    return_address_rows = [
        row
        for row in variables
        if isinstance(row, Mapping)
        and row.get("source_type") == "StackVariableSourceType"
        and row.get("storage") == 0
        and row.get("name") == "__return_addr"
        and str(row.get("size", "")).casefold() == "0x4"
        and row.get("type") == "void* const"
    ]
    parameter_rows: list[tuple[int, int, str]] = []
    for row in variables:
        if (
            not isinstance(row, Mapping)
            or row.get("source_type") != "StackVariableSourceType"
            or not isinstance(row.get("storage"), int)
            or int(row["storage"]) <= 0
        ):
            continue
        try:
            size = int(str(row.get("size", "0")), 0)
        except ValueError as exc:
            raise ValueError(
                "authored decorated ABI bridge has malformed BN parameter size"
            ) from exc
        if size > 0:
            parameter_rows.append(
                (int(row["storage"]), size, str(row.get("type", "")))
            )
    parameter_rows.sort()
    expected_offset = 4
    parameter_stack_sizes: list[int] = []
    for storage, size, _type in parameter_rows:
        if storage != expected_offset:
            raise ValueError(
                "authored decorated ABI bridge requires contiguous BN stack "
                f"parameters at {address}"
            )
        stack_size = max(size, 4)
        parameter_stack_sizes.append(stack_size)
        expected_offset += stack_size
    if (
        len(return_address_rows) != 1
        or len({row[0] for row in parameter_rows}) != len(parameter_rows)
        or expected_offset - 4 != cleanup_bytes
    ):
        raise ValueError(
            "authored decorated ABI bridge BN prototype disagrees with retail "
            f"cleanup at {address}"
        )

    return_payload = bridge.get_json("functionReturnRegs", address=address)
    return_registers = return_payload.get("return_registers")
    if (
        return_payload.get("status") != "ok"
        or return_payload.get("function") != semantic_name
        or normalize_address(str(return_payload.get("address", ""))) != address
        or not isinstance(return_registers, Mapping)
        or return_registers.get("registers") != []
        or return_registers.get("confidence") != 255
    ):
        raise ValueError(
            "authored decorated ABI bridge requires an exact high-confidence "
            f"void BN return prototype at {address}"
        )

    proof = _RetailAuthoredNamespaceAbi(
        semantic_name=semantic_name,
        address=address,
        return_type="void",
        parameter_types=tuple(row[2] for row in parameter_rows),
        parameter_sizes=tuple(row[1] for row in parameter_rows),
        cleanup_bytes=cleanup_bytes,
    )
    cache[key] = proof
    return proof


def _authored_decorated_abi_direct_candidate_bridges(
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    compiler_generated_bridges: Mapping[str, str],
    bridge: BinaryNinjaBridge,
    retail_abi_cache: dict[
        tuple[str, str, str], _RetailAuthoredNamespaceAbi
    ] | None = None,
) -> dict[str, str]:
    """Join an ABI-correct namespace decoration to one authored retail body.

    Candidate COD/COFF supplies only the proposed decorated spelling and exact
    relocation population.  Retail BN name/prototype/RET facts plus the current
    reviewed tracker symbol/source/block topology supply the target identity.
    """

    caller = candidate.caller_definition
    if caller is None:
        return {}
    cache = retail_abi_cache if retail_abi_cache is not None else {}
    invocations_by_name: dict[
        str, list[tuple[int, Instruction, int, int]]
    ] = {}
    for instruction_index, instruction in enumerate(candidate.instructions):
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        if mnemonic not in {"call", "jmp"}:
            continue
        name = _cc_cfg._instruction_operand(instruction).strip()
        decoded = _decode_vc5_namespace_function_abi(name)
        if decoded is None or decoded.calling_convention != "__stdcall":
            continue
        if (
            name in indexes.by_candidate_name
            or name in bridge_names
            or name in compiler_generated_bridges
        ):
            continue
        raw_offset = _cc_cfg._source_instruction_address(instruction)
        opcode = 0xE8 if mnemonic == "call" else 0xE9
        if (
            instruction_index in candidate.local_control_flow_indices
            or not raw_offset
            or tuple(item.casefold() for item in instruction.bytes)
            != (f"{opcode:02x}", "00", "00", "00", "00")
        ):
            raise ValueError(
                "authored decorated ABI bridge requires exact nonlocal "
                f"zero-addend E8/E9 COD evidence for {name!r}"
            )
        invocations_by_name.setdefault(name, []).append(
            (instruction_index, instruction, address_value(raw_offset), opcode)
        )
    if not invocations_by_name:
        return {}

    symbols = document.collection("symbols")
    blocks = document.collection("physical_blocks")
    targets = document.collection("verification_targets")
    resolved: dict[str, str] = {}
    for name, invocations in invocations_by_name.items():
        decoded = _decode_vc5_namespace_function_abi(name)
        assert decoded is not None
        folded_name = name.casefold()
        for label, names in (
            ("candidate identity", indexes.by_candidate_name),
            ("Binary Ninja identity", bridge_names),
            ("compiler/provider identity", compiler_generated_bridges),
            ("candidate storage identity", indexes.storage_by_name),
        ):
            collisions = sorted(
                value for value in names if value.casefold() == folded_name
            )
            if collisions:
                raise ValueError(
                    "authored decorated ABI bridge has a case-folded "
                    f"{label} conflict for {name!r}"
                )
        semantic_collisions = sorted(
            value
            for value in bridge_names
            if value.casefold() == decoded.semantic_name.casefold()
            and value != decoded.semantic_name
        )
        if semantic_collisions:
            raise ValueError(
                "authored decorated ABI bridge has a case-conflicting BN "
                f"semantic name for {decoded.semantic_name!r}"
            )
        raw_matches = bridge_names.get(decoded.semantic_name)
        matches = (
            list(raw_matches)
            if isinstance(raw_matches, (list, tuple, set, frozenset))
            else ([raw_matches] if raw_matches is not None else [])
        )
        exact_matches = [
            match
            for match in matches
            if getattr(match, "kind", "") == "function"
            and getattr(match, "name", "") == decoded.semantic_name
            and getattr(match, "raw_name", "") == decoded.semantic_name
            # The canonical /methods inventory omits top-level full_name for
            # ordinary functions even though the address-scoped function
            # payload below carries the exact nested FunctionSymbol name.
            # Accept only that omission here; a conflicting nonempty spelling
            # still fails, and _retail_authored_namespace_abi independently
            # requires the exact nested full name at the selected address.
            and getattr(match, "full_name", "")
            in ("", decoded.semantic_name)
        ]
        if not matches:
            continue
        if len(exact_matches) != 1 or len(matches) != 1:
            raise ValueError(
                "authored decorated ABI bridge requires one exact BN "
                f"semantic function for {decoded.semantic_name!r}"
            )
        address = normalize_address(str(exact_matches[0].address))
        identity = indexes.by_address.get(address, "")
        physical_symbols: list[tuple[str, Mapping[str, Any]]] = []
        for symbol_id, symbol in symbols.items():
            if not isinstance(symbol, Mapping) or symbol.get("kind") != "function":
                continue
            raw_address = symbol.get("address", symbol.get("start"))
            if not isinstance(raw_address, str):
                continue
            try:
                symbol_address = normalize_address(raw_address)
            except ProgressError:
                continue
            if symbol_address == address:
                physical_symbols.append((str(symbol_id), symbol))
        if len(physical_symbols) != 1:
            raise ValueError(
                "authored decorated ABI bridge requires one unique tracker "
                f"physical authored symbol at {address}"
            )
        symbol_id, symbol = physical_symbols[0]
        end_exclusive = symbol.get("end_exclusive")
        block_id = symbol.get("physical_block_id")
        block = blocks.get(str(block_id))
        order_facts = (
            block.get("accepted_order_facts")
            if isinstance(block, Mapping)
            else None
        )
        accepted_occurrences = 0
        for candidate_block in blocks.values():
            candidate_facts = (
                candidate_block.get("accepted_order_facts")
                if isinstance(candidate_block, Mapping)
                else None
            )
            if (
                not isinstance(candidate_facts, Mapping)
                or candidate_facts.get("phase")
                != "authored-function-order"
                or not isinstance(
                    candidate_facts.get("matched_identities"), list
                )
            ):
                continue
            accepted_occurrences += candidate_facts[
                "matched_identities"
            ].count(symbol_id)
        authored_order = (
            block.get("order", {}).get("authored", {})
            if isinstance(block, Mapping)
            and isinstance(block.get("order"), Mapping)
            else None
        )
        target_id = (
            order_facts.get("target_id")
            if isinstance(order_facts, Mapping)
            else None
        )
        target = targets.get(str(target_id))
        source_traceability = symbol.get("source_traceability")
        source_edges = (
            source_traceability.get("source_edges")
            if isinstance(source_traceability, Mapping)
            else None
        )
        exact_source_edges = [
            edge
            for edge in source_edges or []
            if isinstance(edge, Mapping)
            and edge.get("relation") == "defines"
            and isinstance(edge.get("anchor_id"), str)
            and bool(edge.get("anchor_id"))
            and isinstance(edge.get("emission_context"), Mapping)
            and isinstance(
                edge["emission_context"].get("translation_unit"), str
            )
            and bool(edge["emission_context"].get("translation_unit"))
        ]
        source_path = (
            exact_source_edges[0]["emission_context"]["translation_unit"]
            if len(exact_source_edges) == 1
            else ""
        )
        verification_target_ids = symbol.get("verification_target_ids")
        if (
            symbol_id != f"recoil:function:{address}"
            or identity != f"symbol:{symbol_id}"
            or identity in indexes.provider_ids
            or symbol.get("binary") != "recoil"
            or symbol.get("pipeline_class") != "authored"
            or symbol.get("disposition") != "unresolved"
            or symbol.get("authored_order_role") != "authored-body"
            or symbol.get("ownership_state") != "primary-owned"
            or symbol.get("logical_aliases") not in (None, {})
            or symbol.get("icf_address_group") is not None
            or symbol.get("linked_provider_binding") is not None
            or symbol.get("lifecycle_variant_of") is not None
            or symbol.get("extent_state") != "known"
            or symbol.get("output_section_id") != "recoil:section:.text"
            or symbol.get("navigation_name") != decoded.semantic_name
            or not isinstance(end_exclusive, str)
            or address_value(normalize_address(end_exclusive))
            <= address_value(address)
            or symbol.get("size")
            != address_value(normalize_address(end_exclusive))
            - address_value(address)
            or not isinstance(source_traceability, Mapping)
            or source_traceability.get("state") != "resolved"
            or source_traceability.get("reason_code") not in (None, "")
            or len(source_edges or []) != 1
            or len(exact_source_edges) != 1
            or not isinstance(block_id, str)
            or not isinstance(block, Mapping)
            or block.get("binary") != "recoil"
            or block.get("row_kind") != "physical-source-block"
            or block.get("contribution_kind") != "authored"
            or not isinstance(block.get("contribution_ids"), list)
            or block["contribution_ids"].count(symbol_id) != 1
            or block.get("agent_source_path") != source_path
            or block.get("source_path") != source_path
            or block.get("original_source_path") != source_path
            or not isinstance(order_facts, Mapping)
            or order_facts.get("phase") != "authored-function-order"
            or order_facts.get("validation_mode") != "live"
            or order_facts.get("target_id") != target_id
            or not isinstance(order_facts.get("covered_block_ids"), list)
            or order_facts["covered_block_ids"].count(block_id) != 1
            or not isinstance(order_facts.get("matched_identities"), list)
            or order_facts["matched_identities"].count(symbol_id) != 1
            or accepted_occurrences != 1
            or not isinstance(target_id, str)
            or not isinstance(target, Mapping)
            or target.get("binary") != "recoil"
            or target.get("kind") != "vc5"
            or not isinstance(verification_target_ids, list)
            or verification_target_ids.count(target_id) != 1
            or not isinstance(authored_order, Mapping)
            or any(
                not is_current_accepted_state(
                    authored_order.get(dimension)
                )
                for dimension in AUTHORED_ORDER_DIMENSIONS
            )
        ):
            raise ValueError(
                "authored decorated ABI bridge requires one exact authored "
                f"symbol/source edge/block/target at {address}"
            )

        retail_abi = _retail_authored_namespace_abi(
            address=address,
            end_exclusive=normalize_address(end_exclusive),
            semantic_name=decoded.semantic_name,
            bridge=bridge,
            cache=cache,
        )
        if (
            decoded.return_type != retail_abi.return_type
            or decoded.parameter_types != retail_abi.parameter_types
            or decoded.parameter_sizes != retail_abi.parameter_sizes
            or decoded.parameter_bytes != retail_abi.cleanup_bytes
        ):
            continue

        folded_symbols = [
            row
            for row in caller.coff_symbols
            if row.name.casefold() == folded_name
        ]
        folded_defined = [
            value
            for value in caller.defined_external_functions
            if value.casefold() == folded_name
        ]
        folded_undefined = [
            value
            for value in caller.undefined_external_functions
            if value.casefold() == folded_name
        ]
        if (
            len(folded_symbols) != 1
            or folded_symbols[0].name != name
            or folded_defined
            or folded_undefined != [name]
        ):
            raise ValueError(
                "authored decorated ABI bridge requires one exact undefined "
                f"COFF function symbol for {name!r}"
            )
        external = folded_symbols[0]
        if (
            external.value != 0
            or external.section_number != 0
            or external.symbol_type != 0x20
            or external.storage_class != IMAGE_SYM_CLASS_EXTERNAL
            or external.aux_count != 0
            or external.weak_external_tag_index is not None
            or external.weak_external_characteristics is not None
            or external.section_name
            or external.section_size != 0
            or external.section_characteristics != 0
            or external.natural_end != 0
        ):
            raise ValueError(
                "authored decorated ABI bridge rejects defined, weak, aliased, "
                f"or malformed COFF identity for {name!r}"
            )
        folded_relocations = [
            relocation
            for relocation in caller.relocations
            if relocation.symbol_name.casefold() == folded_name
        ]
        expected_offsets = {
            instruction_offset + 1
            for _index, _instruction, instruction_offset, _opcode in invocations
        }
        if (
            len(folded_relocations) != len(invocations)
            or len(expected_offsets) != len(invocations)
            or {row.offset for row in folded_relocations} != expected_offsets
            or len(caller.data) != len(caller.relocation_mask)
            or caller.section_index <= 0
            or caller.section_end - caller.section_start != len(caller.data)
        ):
            raise ValueError(
                "authored decorated ABI bridge requires one exact candidate "
                f"COFF relocation per invocation of {name!r}"
            )
        relocation_by_offset = {
            relocation.offset: relocation for relocation in folded_relocations
        }
        for _index, instruction, instruction_offset, opcode in invocations:
            relocation = relocation_by_offset[instruction_offset + 1]
            field_end = relocation.offset + 4
            if (
                _cc_cfg._instruction_operand(instruction).strip() != name
                or relocation.type != IMAGE_REL_I386_REL32
                or relocation.symbol_name != name
                or relocation.symbol_index != external.index
                or sum(
                    1
                    for row in caller.relocations
                    if row.offset == relocation.offset
                )
                != 1
                or relocation.offset < 1
                or field_end > len(caller.data)
                or field_end > len(caller.relocation_mask)
                or caller.data[relocation.offset - 1] != opcode
                or struct.unpack_from("<I", caller.data, relocation.offset)[0]
                != 0
                or caller.relocation_mask[relocation.offset - 1]
                or not all(
                    caller.relocation_mask[index]
                    for index in range(relocation.offset, field_end)
                )
            ):
                raise ValueError(
                    "authored decorated ABI bridge requires exact zero-addend "
                    f"fully masked E8/E9 REL32 provenance for {name!r}"
                )
        resolved[name] = identity
    return resolved


def _validate_exact_zeroarg_abi_row_schema(
    raw: object,
    target: ZeroArgAbiTarget,
    *,
    index: int,
    manifest_path: Path,
) -> None:
    label = f"{manifest_path}: zeroarg_abi_equivalence[{index}]"
    if not isinstance(raw, Mapping):
        raise ValueError(f"{label} must be an object")
    expected_fields = set(_cc_catalog.ZEROARG_ABI_ROW_FIELDS)
    if "st0_policy" in raw:
        expected_fields.add("st0_policy")
    if set(raw) != expected_fields:
        raise ValueError(
            f"{label} must contain exactly "
            + ", ".join(sorted(expected_fields))
        )

    evidence = raw.get("retail_evidence")
    if (
        not isinstance(evidence, Mapping)
        or set(evidence) != _cc_catalog.ZEROARG_ABI_RETAIL_EVIDENCE_FIELDS
    ):
        raise ValueError(
            f"{label}.retail_evidence must contain the exact governed "
            "zero-argument evidence schema"
        )
    boolean_fields = _cc_catalog.ZEROARG_ABI_RETAIL_EVIDENCE_FIELDS - {
        "explicit_argument_count",
        "direct_calls",
    }
    if any(type(evidence.get(field)) is not bool for field in boolean_fields):
        raise ValueError(
            f"{label}.retail_evidence boolean fields must contain exact booleans"
        )
    if type(evidence.get("explicit_argument_count")) is not int:
        raise ValueError(
            f"{label}.retail_evidence.explicit_argument_count must be an integer"
        )
    direct_calls = evidence.get("direct_calls")
    if not isinstance(direct_calls, list) or not direct_calls:
        raise ValueError(
            f"{label}.retail_evidence.direct_calls must be a non-empty list"
        )
    direct_call_keys: set[tuple[object, ...]] = set()
    for call_index, call in enumerate(direct_calls):
        if (
            not isinstance(call, Mapping)
            or set(call) != _cc_catalog.ZEROARG_ABI_DIRECT_CALL_FIELDS
        ):
            raise ValueError(
                f"{label}.retail_evidence.direct_calls[{call_index}] must "
                "contain exactly address, dispatch, explicit_argument_count, "
                "and callee_return"
            )
        key = (
            call.get("address"),
            call.get("dispatch"),
            call.get("explicit_argument_count"),
            call.get("callee_return"),
        )
        if key in direct_call_keys:
            raise ValueError(
                f"{label}.retail_evidence.direct_calls contains a duplicate exact row"
            )
        direct_call_keys.add(key)

    eh_policy = raw.get("eh_policy")
    if not isinstance(eh_policy, Mapping):
        raise ValueError(f"{label}.eh_policy must be an object")
    expected_eh_fields = {"kind", "retail_proven"}
    if eh_policy.get("kind") == "paired-sections":
        expected_eh_fields.add("sections")
    if set(eh_policy) != expected_eh_fields:
        raise ValueError(
            f"{label}.eh_policy must contain the exact governed schema for "
            f"kind {eh_policy.get('kind')!r}"
        )
    if type(eh_policy.get("retail_proven")) is not bool:
        raise ValueError(f"{label}.eh_policy.retail_proven must be an exact boolean")

    st0_policy = raw.get("st0_policy")
    if target.return_category == "x87-float":
        if (
            not isinstance(st0_policy, Mapping)
            or set(st0_policy) != {"proven", "evidence"}
            or type(st0_policy.get("proven")) is not bool
            or not isinstance(st0_policy.get("evidence"), str)
            or not st0_policy["evidence"].strip()
        ):
            raise ValueError(
                f"{label}.st0_policy must contain exactly proven and evidence "
                "for an x87-float return"
            )
    elif "st0_policy" in raw:
        raise ValueError(
            f"{label}.st0_policy is only valid for an x87-float return"
        )


def _is_exact_zeroarg_cdecl_fastcall_pair(
    cdecl_symbol: str,
    fastcall_symbol: str,
) -> bool:
    """Recognize only exact VC5 /Gd-/Gr zero-explicit-argument decoration."""
    if cdecl_symbol.startswith("?"):
        match = re.fullmatch(
            r"(?P<prefix>\?.+@@[YS])A(?P<signature>.+XZ)",
            cdecl_symbol,
        )
        return bool(
            match is not None
            and fastcall_symbol
            == f"{match.group('prefix')}I{match.group('signature')}"
        )
    match = re.fullmatch(r"_([A-Za-z][A-Za-z0-9_?$]*)", cdecl_symbol)
    return bool(
        match is not None
        and fastcall_symbol == f"@{match.group(1)}@0"
    )


def resolve_manifest_zeroarg_abi_identity_bridges(
    document: ProgressDocument,
    *,
    indexes: IdentityIndexes,
    bridge_by_address: Mapping[str, Any],
    manifest_path: Path = DEFAULT_FINAL_BUILD_MANIFEST,
) -> dict[str, str]:
    """Resolve exact governed /Gd-/Gr no-argument names to retail identities.

    The manifest and candidate-independent tracker/retail symbol population are
    the only authorities.  The candidate object is deliberately not an input.
    """
    targets = load_zeroarg_targets(manifest_path)
    raw_manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    raw_rows = (
        raw_manifest.get("zeroarg_abi_equivalence")
        if isinstance(raw_manifest, Mapping)
        else None
    )
    if not isinstance(raw_rows, list) or len(raw_rows) != len(targets):
        raise ValueError(
            f"{manifest_path}: zeroarg_abi_equivalence typed/raw row "
            "population is missing or inconsistent"
        )
    normalization = manifest_symbol_normalization(targets)

    retail_rows_by_address: dict[str, list[Any]] = {}
    retail_name_addresses: dict[str, set[str]] = {}
    for raw_address, retail_symbol in bridge_by_address.items():
        value = getattr(retail_symbol, "address", raw_address)
        try:
            address = normalize_address(str(value))
        except ProgressError:
            continue
        retail_rows_by_address.setdefault(address, []).append(retail_symbol)
        for name in {
            str(getattr(retail_symbol, "name", "") or ""),
            str(getattr(retail_symbol, "raw_name", "") or ""),
            str(getattr(retail_symbol, "full_name", "") or ""),
        }:
            if name:
                retail_name_addresses.setdefault(name, set()).add(address)

    symbols = document.collection("symbols")
    symbol_bindings: dict[str, str] = {}
    result: dict[str, str] = {}
    direct_call_bindings: dict[str, str] = {}
    for index, (raw, target) in enumerate(zip(raw_rows, targets)):
        _validate_exact_zeroarg_abi_row_schema(
            raw,
            target,
            index=index,
            manifest_path=manifest_path,
        )
        failed_gates = [
            str(gate.get("gate") or "")
            for gate in zeroarg_abi_eligibility_gates(target)
            if gate.get("passed") is not True
        ]
        if failed_gates:
            raise ValueError(
                f"{manifest_path}: zeroarg ABI row {target.target_id!r} is "
                "ineligible: "
                + ", ".join(failed_gates)
            )
        cdecl_symbol = target.callee.cdecl_symbol
        fastcall_symbol = target.callee.fastcall_symbol
        if not _is_exact_zeroarg_cdecl_fastcall_pair(
            cdecl_symbol,
            fastcall_symbol,
        ):
            raise ValueError(
                f"{manifest_path}: zeroarg ABI row {target.target_id!r} is not "
                "one exact VC5 /Gd-/Gr zero-argument decoration pair"
            )
        if (
            normalization.get(cdecl_symbol) != target.identity
            or normalization.get(fastcall_symbol) != target.identity
        ):
            raise ValueError(
                f"{manifest_path}: zeroarg ABI row {target.target_id!r} has "
                "ambiguous symbol-to-identity normalization"
            )
        for symbol_name in (cdecl_symbol, fastcall_symbol):
            prior_binding = symbol_bindings.get(symbol_name)
            if prior_binding not in {None, target.identity}:
                raise ValueError(
                    f"{manifest_path}: zeroarg ABI symbol {symbol_name!r} maps "
                    "to conflicting manifest identities"
                )
            symbol_bindings[symbol_name] = target.identity

        tracker_symbol = symbols.get(target.identity)
        raw_address = (
            tracker_symbol.get("address", tracker_symbol.get("start"))
            if isinstance(tracker_symbol, Mapping)
            else None
        )
        if (
            not isinstance(tracker_symbol, Mapping)
            or tracker_symbol.get("binary") != "recoil"
            or tracker_symbol.get("kind") != "function"
            or tracker_symbol.get("pipeline_class")
            not in {"authored", "authored-lifecycle"}
            or not isinstance(raw_address, str)
        ):
            raise ValueError(
                f"{manifest_path}: zeroarg ABI identity {target.identity!r} "
                "is not one exact authored tracker function"
            )
        address = normalize_address(raw_address)
        if target.identity != f"recoil:function:{address}":
            raise ValueError(
                f"{manifest_path}: zeroarg ABI identity {target.identity!r} "
                f"does not match tracker address {address}"
            )
        identity = f"symbol:{target.identity}"
        if indexes.by_address.get(address) != identity:
            raise ValueError(
                f"{manifest_path}: zeroarg ABI identity {target.identity!r} "
                "has ambiguous or conflicting tracker identity resolution"
            )

        retail_rows = retail_rows_by_address.get(address, [])
        cdecl_addresses = retail_name_addresses.get(cdecl_symbol, set())
        fastcall_addresses = retail_name_addresses.get(fastcall_symbol, set())
        if (
            len(retail_rows) != 1
            or getattr(retail_rows[0], "kind", "") != "function"
            or cdecl_addresses - {address}
            or fastcall_addresses - {address}
        ):
            raise ValueError(
                f"{manifest_path}: zeroarg ABI identity {target.identity!r} "
                "does not have one exact candidate-independent retail symbol mapping"
            )

        raw_evidence = raw["retail_evidence"]
        for call in raw_evidence["direct_calls"]:
            call_address = normalize_address(str(call["address"]))
            prior_target = direct_call_bindings.get(call_address)
            if prior_target not in {None, target.identity}:
                raise ValueError(
                    f"{manifest_path}: zeroarg ABI retail call evidence "
                    f"{call_address} maps to conflicting target identities"
                )
            direct_call_bindings[call_address] = target.identity

        for symbol_name in (cdecl_symbol, fastcall_symbol):
            prior = indexes.by_candidate_name.get(symbol_name)
            if prior not in {None, identity}:
                raise ValueError(
                    f"{manifest_path}: zeroarg ABI symbol {symbol_name!r} "
                    f"conflicts with current identity {prior!r}"
                )
            result[symbol_name] = identity
    return result
