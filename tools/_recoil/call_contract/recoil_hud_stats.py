"""Recoil call-contract recoil hud stats evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import CandidateAssembly, IdentityIndexes

import re
import struct
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import IMAGE_REL_I386_REL32, Instruction
from _recoil.lib.progress import normalize_address


def _hud_ui_mgr_ensure_stats_progressive_read_int3_guard(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_start: str,
    caller_end_exclusive: str,
    helper_calls: Sequence[tuple[int, Instruction, re.Match[str]]],
) -> int | None:
    """Prove one leading direct ReadInt3 prefix and the helper suffix."""
    instructions = candidate.instructions
    helper_cod_re = next(
        cod_re
        for label, cod_re, _coff_re, _count in (
            _cc_catalog.HUD_UI_MGR_ENSURE_UNSUPPORTED_HELPER_SPECS
        )
        if label == "HudUiApplyStatsTripletInt3"
    )
    interpolate_rows = [
        index
        for index, instruction in enumerate(instructions)
        if (
            _cc_cfg._instruction_mnemonic(instruction) == "call"
            and _cc_cfg._instruction_operand(instruction).strip()
            == _cc_catalog.HUD_UI_MGR_ENSURE_STATS_INTERPOLATE_SYMBOL
        )
    ]
    if not interpolate_rows and not helper_calls:
        return None
    if len(interpolate_rows) != 1:
        raise ValueError(
            "HUD EnsureHudLoaded progressive stats ReadInt3 bridge requires "
            "one exact HudUiTriplet::InterpolateLayout boundary"
        )

    definition = candidate.caller_definition
    if (
        definition is None
        or definition.symbol != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_SYMBOL
        or not definition.data
        or len(definition.relocation_mask) != len(definition.data)
        or definition.defined_external_functions.count(
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_READ_INT3_SYMBOL
        )
        != 1
    ):
        raise ValueError(
            "HUD EnsureHudLoaded progressive stats ReadInt3 bridge requires "
            "the exact caller COMDAT and one ReadInt3 COFF definition"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalize_address(caller_start),
        caller_end_exclusive=normalize_address(caller_end_exclusive),
    )
    interpolate_index = interpolate_rows[0]
    if (
        interpolate_index not in invocation_indices
        or invocation_indices.index(interpolate_index)
        < _cc_catalog.HUD_UI_MGR_ENSURE_STATS_READ_INT3_COUNT
    ):
        raise ValueError(
            "HUD EnsureHudLoaded progressive stats ReadInt3 bridge rejects "
            "a missing or absorbed six-call statistics unit"
        )
    interpolate_ordinal = invocation_indices.index(interpolate_index)
    stats_indices = invocation_indices[
        interpolate_ordinal - _cc_catalog.HUD_UI_MGR_ENSURE_STATS_READ_INT3_COUNT :
        interpolate_ordinal
    ]
    first_ordinal = _cc_catalog.HUD_UI_MGR_ENSURE_STATS_FIRST_READ_INT3_ORDINAL
    if (
        interpolate_ordinal
        != first_ordinal + _cc_catalog.HUD_UI_MGR_ENSURE_STATS_READ_INT3_COUNT
        or len(expected)
        < first_ordinal + _cc_catalog.HUD_UI_MGR_ENSURE_STATS_READ_INT3_COUNT
    ):
        raise ValueError(
            "HUD EnsureHudLoaded progressive stats ReadInt3 bridge rejects "
            "statistics invocation ordinal drift"
        )
    expected_read_contract = {
        "form": "call",
        "dispatch": "direct",
        "identity_kind": "direct",
        "target_identity": _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_READ_INT3_IDENTITY,
        "storage_identity": "",
        "slot_displacement": None,
        "cleanup_bytes": None,
    }
    if any(
        expected[first_ordinal + position]
        != {
            "ordinal": first_ordinal + position,
            **expected_read_contract,
        }
        for position in range(_cc_catalog.HUD_UI_MGR_ENSURE_STATS_READ_INT3_COUNT)
    ):
        raise ValueError(
            "HUD EnsureHudLoaded progressive stats ReadInt3 bridge requires "
            "six independently retail-derived direct ReadInt3 contracts"
        )

    statuses: list[str] = []
    for index in stats_indices:
        operand = _cc_cfg._instruction_operand(instructions[index]).strip()
        if operand == _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_READ_INT3_SYMBOL:
            statuses.append("direct")
        elif helper_cod_re.fullmatch(operand) is not None:
            statuses.append("helper")
        else:
            statuses.append("other")
    direct_count = 0
    for status in statuses:
        if status != "direct":
            break
        direct_count += 1
    if (
        direct_count not in range(1, _cc_catalog.HUD_UI_MGR_ENSURE_STATS_READ_INT3_COUNT + 1)
        or statuses
        != (
            ["direct"] * direct_count
            + ["helper"]
            * (_cc_catalog.HUD_UI_MGR_ENSURE_STATS_READ_INT3_COUNT - direct_count)
        )
        or [index for index, _instruction, _match in helper_calls]
        != list(stats_indices[direct_count:])
    ):
        raise ValueError(
            "HUD EnsureHudLoaded progressive stats ReadInt3 bridge requires "
            "exactly k leading direct calls and a gapless ordered helper "
            f"suffix for k=1..6: statuses={statuses!r}, "
            f"stats_indices={stats_indices!r}, "
            "helper_indices="
            f"{[index for index, _instruction, _match in helper_calls]!r}"
        )

    addresses = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    if any(address is None for address in addresses):
        raise ValueError(
            "HUD EnsureHudLoaded progressive stats ReadInt3 bridge requires "
            "complete candidate instruction offsets"
        )

    def exact_unmasked_instruction(index: int) -> bytes:
        offset = addresses[index]
        assert offset is not None
        encoded = bytes(int(value, 16) for value in instructions[index].bytes)
        if (
            not encoded
            or definition.data[offset : offset + len(encoded)] != encoded
            or any(
                definition.relocation_mask[item]
                for item in range(offset, offset + len(encoded))
            )
            or any(
                relocation.offset < offset + len(encoded)
                and relocation.offset + 4 > offset
                for relocation in definition.relocations
            )
        ):
            raise ValueError(
                "HUD EnsureHudLoaded progressive stats ReadInt3 bridge "
                "rejects setup byte, relocation, or mask drift"
            )
        return encoded

    def last_definition(register: str, start: int, end: int) -> int:
        return next(
            (
                index
                for index in range(end - 1, start - 1, -1)
                if _cc_cfg._instruction_may_clobber_register(
                    instructions[index],
                    register,
                )
            ),
            -1,
        )

    def require_local_lea(
        *,
        use_index: int,
        register: str,
        local_name: str,
        window_start: int,
    ) -> int:
        definition_index = last_definition(
            register,
            window_start,
            use_index,
        )
        if definition_index < 0:
            raise ValueError(
                "HUD EnsureHudLoaded progressive stats ReadInt3 bridge "
                f"requires a reaching &{local_name} definition"
            )
        operand = _cc_cfg._instruction_operand(instructions[definition_index])
        if "," not in operand:
            raise ValueError(
                "HUD EnsureHudLoaded progressive stats ReadInt3 bridge "
                f"rejects malformed &{local_name} dataflow"
            )
        destination, source = (
            item.strip().lower() for item in operand.split(",", 1)
        )
        expression, _displacement = _cc_targets._memory_slot(source)
        if (
            _cc_cfg._instruction_mnemonic(instructions[definition_index]) != "lea"
            or destination != register
            or f"_{local_name}$" not in source
            or "esp" not in expression
        ):
            raise ValueError(
                "HUD EnsureHudLoaded progressive stats ReadInt3 bridge "
                f"rejects the exact &{local_name} origin"
            )
        exact_unmasked_instruction(definition_index)
        return definition_index

    def push_register(index: int) -> str:
        operand = _cc_cfg._instruction_operand(instructions[index]).strip().lower()
        if (
            _cc_cfg._instruction_mnemonic(instructions[index]) != "push"
            or operand
            not in {"eax", "ebx", "ecx", "edx", "esi", "edi", "ebp"}
            or len(exact_unmasked_instruction(index)) != 1
        ):
            raise ValueError(
                "HUD EnsureHudLoaded progressive stats ReadInt3 bridge "
                "requires exact one-byte register argument pushes"
            )
        return operand

    zero_definitions = [
        index
        for index, instruction in enumerate(instructions)
        if (
            tuple(value.lower() for value in instruction.bytes)
            == ("33", "db")
            and re.fullmatch(
                r"xor\s+ebx\s*,\s*ebx",
                instruction.raw_text.strip(),
                flags=re.IGNORECASE,
            )
            is not None
        )
    ]
    if len(zero_definitions) != 1:
        raise ValueError(
            "HUD EnsureHudLoaded progressive stats ReadInt3 bridge requires "
            "one exact EBX zero definition"
        )
    exact_unmasked_instruction(zero_definitions[0])

    first_stats_index = stats_indices[0]
    payload_definition = last_definition("edi", 0, first_stats_index)
    payload_operand = (
        _cc_cfg._instruction_operand(instructions[payload_definition])
        if payload_definition >= 0
        else ""
    )
    if (
        payload_definition < 0
        or tuple(
            value.lower() for value in instructions[payload_definition].bytes
        )
        != ("8b", "78", "04")
        or re.fullmatch(
            r"mov\s+edi\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[eax\+4\]",
            instructions[payload_definition].raw_text.strip(),
            flags=re.IGNORECASE,
        )
        is None
        or any(
            _cc_cfg._instruction_may_clobber_register(instructions[index], "edi")
            for index in range(payload_definition + 1, interpolate_index)
        )
    ):
        del payload_operand
        raise ValueError(
            "HUD EnsureHudLoaded progressive stats ReadInt3 bridge rejects "
            "the statsPayload base definition or lifetime"
        )
    exact_unmasked_instruction(payload_definition)

    triplet_definition = last_definition("esi", 0, first_stats_index)
    if (
        triplet_definition < 0
        or tuple(
            value.lower() for value in instructions[triplet_definition].bytes
        )
        != ("8b", "72", "34")
        or re.fullmatch(
            r"mov\s+esi\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[edx\+(?:52|0x34)\]",
            instructions[triplet_definition].raw_text.strip(),
            flags=re.IGNORECASE,
        )
        is None
        or any(
            _cc_cfg._instruction_may_clobber_register(instructions[index], "esi")
            for index in range(triplet_definition + 1, interpolate_index)
        )
    ):
        raise ValueError(
            "HUD EnsureHudLoaded progressive stats ReadInt3 bridge rejects "
            "the triplet base definition or lifetime"
        )
    exact_unmasked_instruction(triplet_definition)

    previous_by_index = {
        invocation_index: (
            invocation_indices[position - 1] if position else -1
        )
        for position, invocation_index in enumerate(invocation_indices)
    }
    next_by_index = {
        invocation_index: (
            invocation_indices[position + 1]
            if position + 1 < len(invocation_indices)
            else len(instructions)
        )
        for position, invocation_index in enumerate(invocation_indices)
    }

    def require_zero_push(index: int) -> None:
        if (
            push_register(index) != "ebx"
            or exact_unmasked_instruction(index) != b"\x53"
            or any(
                _cc_cfg._instruction_may_clobber_register(
                    instructions[item],
                    "ebx",
                )
                and not (
                    _cc_cfg._instruction_mnemonic(instructions[item]) == "pop"
                    and item + 2 < len(instructions)
                    and re.fullmatch(
                        r"add\s+esp\s*,\s*(?:132|0x84)",
                        instructions[item + 1].raw_text.strip(),
                        flags=re.IGNORECASE,
                    )
                    is not None
                    and _cc_cfg._exact_return_terminates(instructions[item + 2])
                )
                for item in range(zero_definitions[0] + 1, index)
            )
        ):
            raise ValueError(
                "HUD EnsureHudLoaded progressive stats ReadInt3 bridge "
                "rejects the exact zero argument origin"
            )

    def require_member_pointer(
        *,
        use_index: int,
        register: str,
        displacement: int,
        window_start: int,
    ) -> None:
        definition_index = last_definition(
            register,
            window_start,
            use_index,
        )
        operand = (
            _cc_cfg._instruction_operand(instructions[definition_index])
            if definition_index >= 0
            else ""
        )
        if "," not in operand:
            raise ValueError(
                "HUD EnsureHudLoaded progressive stats ReadInt3 bridge "
                "rejects a helper member-pointer argument"
            )
        destination, source = (
            item.strip().lower() for item in operand.split(",", 1)
        )
        expression, actual_displacement = _cc_targets._memory_slot(source)
        if (
            definition_index < 0
            or _cc_cfg._instruction_mnemonic(instructions[definition_index]) != "lea"
            or destination != register
            or "esi" not in expression
            or actual_displacement != displacement
        ):
            raise ValueError(
                "HUD EnsureHudLoaded progressive stats ReadInt3 bridge "
                "rejects a helper member-pointer origin"
            )
        exact_unmasked_instruction(definition_index)

    for position, call_index in enumerate(stats_indices):
        stats_payload_index = position + 3
        window_start = previous_by_index[call_index] + 1
        push_rows = [
            index
            for index in range(window_start, call_index)
            if _cc_cfg._instruction_mnemonic(instructions[index]) == "push"
        ]
        node_definition = last_definition("ecx", window_start, call_index)
        node_operand = (
            _cc_cfg._instruction_operand(instructions[node_definition])
            if node_definition >= 0
            else ""
        )
        if "," not in node_operand:
            raise ValueError(
                "HUD EnsureHudLoaded progressive stats ReadInt3 bridge "
                "rejects the statsPayload node argument"
            )
        node_destination, node_source = (
            item.strip().lower() for item in node_operand.split(",", 1)
        )
        node_expression, node_displacement = _cc_targets._memory_slot(node_source)

        if position < direct_count:
            if (
                _cc_cfg._instruction_mnemonic(instructions[call_index]) != "call"
                or _cc_cfg._instruction_operand(instructions[call_index]).strip()
                != _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_READ_INT3_SYMBOL
                or len(push_rows) != 2
                or node_definition < 0
                or _cc_cfg._instruction_mnemonic(instructions[node_definition])
                != "lea"
                or node_destination != "ecx"
                or "edi" not in node_expression
                or node_displacement != stats_payload_index * 8
            ):
                raise ValueError(
                    "HUD EnsureHudLoaded progressive stats ReadInt3 bridge "
                    "rejects direct call/node/order/argument population drift"
                )
            exact_unmasked_instruction(node_definition)

            x_definition = last_definition("edx", window_start, call_index)
            if x_definition < 0:
                raise ValueError(
                    "HUD EnsureHudLoaded progressive stats ReadInt3 bridge "
                    "requires the direct &x register argument"
                )
            require_local_lea(
                use_index=call_index,
                register="edx",
                local_name="x",
                window_start=window_start,
            )
            first_register = push_register(push_rows[0])
            second_register = push_register(push_rows[1])
            if stats_payload_index in {3, 4}:
                require_local_lea(
                    use_index=push_rows[0],
                    register=first_register,
                    local_name="z",
                    window_start=window_start,
                )
            else:
                require_zero_push(push_rows[0])
            require_local_lea(
                use_index=push_rows[1],
                register=second_register,
                local_name="y",
                window_start=window_start,
            )

            call_offset = addresses[call_index]
            assert call_offset is not None
            call_end = call_offset + 5
            call_relocations = [
                relocation
                for relocation in definition.relocations
                if (
                    relocation.offset < call_end
                    and relocation.offset + 4 > call_offset
                )
            ]
            if (
                tuple(
                    value.lower() for value in instructions[call_index].bytes
                )
                != ("e8", "00", "00", "00", "00")
                or definition.data[call_offset:call_end] != b"\xe8\0\0\0\0"
                or len(call_relocations) != 1
                or call_relocations[0].offset != call_offset + 1
                or call_relocations[0].type != IMAGE_REL_I386_REL32
                or call_relocations[0].symbol_name
                != _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_READ_INT3_SYMBOL
                or struct.unpack_from(
                    "<I",
                    definition.data,
                    call_offset + 1,
                )[0]
                != 0
                or definition.relocation_mask[call_offset]
                or not all(
                    definition.relocation_mask[item]
                    for item in range(call_offset + 1, call_end)
                )
                or _cc_cfg._cleanup_after(instructions, call_index) is not None
            ):
                raise ValueError(
                    "HUD EnsureHudLoaded progressive stats ReadInt3 bridge "
                    "rejects direct ReadInt3 REL32 target/addend/mask, call "
                    "bytes, or cleanup"
                )

            next_invocation = next_by_index[call_index]
            for index in range(call_index + 1, next_invocation):
                operand = _cc_cfg._instruction_operand(instructions[index]).lower()
                if not re.search(r"\beax\b", operand):
                    continue
                destination = (
                    operand.split(",", 1)[0].strip()
                    if "," in operand
                    else ""
                )
                if (
                    destination == "eax"
                    and _cc_cfg._instruction_may_clobber_register(
                        instructions[index],
                        "eax",
                    )
                ):
                    break
                raise ValueError(
                    "HUD EnsureHudLoaded progressive stats ReadInt3 bridge "
                    "rejects direct-call result consumption"
                )

            if stats_payload_index >= 5:
                expected_pair = _cc_catalog.HUD_UI_MGR_ENSURE_STATS_MEMBER_OFFSETS[
                    position
                ][:2]
                for local_name, displacement in zip(
                    ("x", "y"),
                    expected_pair,
                ):
                    store_rows: list[int] = []
                    for index in range(call_index + 1, next_invocation):
                        instruction = instructions[index]
                        if _cc_cfg._instruction_mnemonic(instruction) != "mov":
                            continue
                        operand = _cc_cfg._instruction_operand(instruction)
                        if "," not in operand:
                            continue
                        destination, source = (
                            item.strip().lower()
                            for item in operand.split(",", 1)
                        )
                        expression, actual_displacement = _cc_targets._memory_slot(
                            destination
                        )
                        if (
                            "esi" in expression
                            and actual_displacement == displacement
                            and source
                            in {
                                "eax",
                                "ebx",
                                "ecx",
                                "edx",
                                "edi",
                                "ebp",
                            }
                        ):
                            store_rows.append(index)
                    if len(store_rows) != 1:
                        raise ValueError(
                            "HUD EnsureHudLoaded progressive stats ReadInt3 "
                            "bridge requires one exact post-call member store"
                        )
                    store_index = store_rows[0]
                    source_register = (
                        _cc_cfg._instruction_operand(instructions[store_index])
                        .split(",", 1)[1]
                        .strip()
                        .lower()
                    )
                    require_local_lea_index = last_definition(
                        source_register,
                        call_index + 1,
                        store_index,
                    )
                    load_operand = (
                        _cc_cfg._instruction_operand(
                            instructions[require_local_lea_index]
                        )
                        if require_local_lea_index >= 0
                        else ""
                    )
                    if "," not in load_operand:
                        raise ValueError(
                            "HUD EnsureHudLoaded progressive stats ReadInt3 "
                            "bridge rejects post-call local/member dataflow"
                        )
                    load_destination, load_source = (
                        item.strip().lower()
                        for item in load_operand.split(",", 1)
                    )
                    if (
                        require_local_lea_index < 0
                        or _cc_cfg._instruction_mnemonic(
                            instructions[require_local_lea_index]
                        )
                        != "mov"
                        or load_destination != source_register
                        or f"_{local_name}$" not in load_source
                    ):
                        raise ValueError(
                            "HUD EnsureHudLoaded progressive stats ReadInt3 "
                            "bridge rejects post-call local/member origins"
                        )
                    exact_unmasked_instruction(require_local_lea_index)
                    exact_unmasked_instruction(store_index)
        else:
            if (
                len(push_rows) != 3
                or node_definition < 0
                or tuple(
                    value.lower()
                    for value in instructions[node_definition].bytes
                )
                != ("8b", "cf")
                or re.fullmatch(
                    r"mov\s+ecx\s*,\s*edi",
                    instructions[node_definition].raw_text.strip(),
                    flags=re.IGNORECASE,
                )
                is None
            ):
                raise ValueError(
                    "HUD EnsureHudLoaded progressive stats ReadInt3 bridge "
                    "rejects helper statsPayload/argument topology"
                )
            exact_unmasked_instruction(node_definition)
            helper_index_definition = last_definition(
                "edx",
                window_start,
                call_index,
            )
            if (
                helper_index_definition < 0
                or bytes(
                    int(value, 16)
                    for value in instructions[
                        helper_index_definition
                    ].bytes
                )
                != bytes((0xBA, stats_payload_index, 0, 0, 0))
                or re.fullmatch(
                    rf"mov\s+edx\s*,\s*{stats_payload_index}",
                    instructions[helper_index_definition].raw_text.strip(),
                    flags=re.IGNORECASE,
                )
                is None
            ):
                raise ValueError(
                    "HUD EnsureHudLoaded progressive stats ReadInt3 bridge "
                    "rejects helper suffix node-index order"
                )
            exact_unmasked_instruction(helper_index_definition)
            registers = [push_register(index) for index in push_rows]
            if stats_payload_index == 4:
                local_names = ("z", "y", "x")
                for push_index, register, local_name in zip(
                    push_rows,
                    registers,
                    local_names,
                ):
                    require_local_lea(
                        use_index=push_index,
                        register=register,
                        local_name=local_name,
                        window_start=window_start,
                    )
            else:
                require_zero_push(push_rows[0])
                x_offset, y_offset, _z_offset = (
                    _cc_catalog.HUD_UI_MGR_ENSURE_STATS_MEMBER_OFFSETS[position]
                )
                require_member_pointer(
                    use_index=push_rows[1],
                    register=registers[1],
                    displacement=y_offset,
                    window_start=window_start,
                )
                require_member_pointer(
                    use_index=push_rows[2],
                    register=registers[2],
                    displacement=x_offset,
                    window_start=window_start,
                )

    return _cc_catalog.HUD_UI_MGR_ENSURE_STATS_READ_INT3_COUNT - direct_count


def _hud_ui_mgr_ensure_unsupported_helpers_candidate_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    compiler_generated_bridges: Mapping[str, str],
) -> dict[str, str]:
    """Expose exact current HUD scaffolds as candidate-only comparison rows.

    These helpers have no asserted retail target identity.  Their exact
    ``00_hud.cpp`` decorations, callsites, COFF definitions, and REL32
    references are bounded only so the complete candidate contract can be
    extracted and ordinary comparison can report the calls as extras or target
    mismatches.  The returned caller identity is a comparison sentinel, not a
    source-owner, provider, or retail-call claim.
    """

    if normalize_address(caller_start) != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START:
        return {}

    matching_by_label: dict[str, list[tuple[int, Instruction, re.Match[str]]]] = {
        label: []
        for label, _, _, _ in _cc_catalog.HUD_UI_MGR_ENSURE_UNSUPPORTED_HELPER_SPECS
    }
    for instruction_index, instruction in enumerate(candidate.instructions):
        if _cc_cfg._instruction_mnemonic(instruction) not in {"call", "jmp"}:
            continue
        operand = _cc_cfg._instruction_operand(instruction).strip()
        for label, cod_re, _, _ in _cc_catalog.HUD_UI_MGR_ENSURE_UNSUPPORTED_HELPER_SPECS:
            match = cod_re.fullmatch(operand)
            if match is not None:
                matching_by_label[label].append(
                    (instruction_index, instruction, match)
                )
                break
            if label in operand:
                raise ValueError(
                    "HUD EnsureHudLoaded unsupported-helper bridge accepts only "
                    f"the exact {label} 00_hud.cpp fastcall decoration"
                )
    if not any(matching_by_label.values()):
        return {}

    definition = candidate.caller_definition
    normalized_start = normalize_address(caller_start)
    normalized_end = normalize_address(caller_end_exclusive)
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_IDENTITY
        or normalized_start != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START
        or normalized_end != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_start) != caller_identity
        or sorted(
            address
            for address, identity in indexes.by_address.items()
            if identity == caller_identity
        )
        != [_cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START]
        or caller_identity in indexes.provider_ids
        or definition is None
        or definition.symbol != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_SYMBOL
        or not definition.data
        or len(definition.relocation_mask) != len(definition.data)
        or any(
            row.get("target_identity") == caller_identity
            for row in expected
        )
    ):
        raise ValueError(
            "HUD EnsureHudLoaded unsupported-helper bridge requires the exact "
            "reviewed caller identity, extent, candidate symbol/body, and no "
            "retail comparison-sentinel target"
        )

    progressive_stats_helper_count = (
        _hud_ui_mgr_ensure_stats_progressive_read_int3_guard(
            expected,
            candidate,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            helper_calls=matching_by_label["HudUiApplyStatsTripletInt3"],
        )
    )
    complete_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    result: dict[str, str] = {}
    for (
        label,
        cod_re,
        coff_re,
        expected_call_count,
    ) in _cc_catalog.HUD_UI_MGR_ENSURE_UNSUPPORTED_HELPER_SPECS:
        calls = matching_by_label[label]
        if not calls:
            continue
        cod_symbols = {
            _cc_cfg._instruction_operand(instruction).strip()
            for _, instruction, _ in calls
        }
        discriminators = {
            match.group("discriminator")
            for _, _, match in calls
        }
        effective_expected_call_count = (
            progressive_stats_helper_count
            if (
                label == "HudUiApplyStatsTripletInt3"
                and progressive_stats_helper_count is not None
            )
            else expected_call_count
        )
        if (
            len(calls) != effective_expected_call_count
            or len(cod_symbols) != 1
            or len(discriminators) != 1
        ):
            raise ValueError(
                "HUD EnsureHudLoaded unsupported-helper bridge requires the "
                f"exact reviewed {label} candidate call population and one "
                "consistent TU discriminator"
            )
        cod_symbol = next(iter(cod_symbols))
        discriminator = next(iter(discriminators))
        coff_symbols = [
            symbol
            for symbol in definition.defined_external_functions
            if (
                (match := coff_re.fullmatch(symbol)) is not None
                and match.group("discriminator") == discriminator
            )
        ]
        if len(coff_symbols) != 1:
            raise ValueError(
                "HUD EnsureHudLoaded unsupported-helper bridge requires exactly "
                f"one same-object {label} COFF definition with the matching "
                "TU discriminator"
            )
        coff_symbol = coff_symbols[0]
        collision_names = {cod_symbol, coff_symbol}
        if (
            any(name in indexes.by_candidate_name for name in collision_names)
            or any(name in bridge_names for name in collision_names)
            or any(name in indexes.storage_by_name for name in collision_names)
            or any(
                name in compiler_generated_bridges
                for name in collision_names
            )
        ):
            raise ValueError(
                "HUD EnsureHudLoaded unsupported-helper bridge has a conflicting "
                f"ordinary candidate, retail, storage, or compiler identity "
                f"for {label}"
            )
        folded_names = {name.casefold() for name in collision_names}
        if (
            [
                name
                for name in definition.defined_external_functions
                if name.casefold() in folded_names
            ]
            != [coff_symbol]
            or any(
                name.casefold() in folded_names
                for name in definition.undefined_external_functions
            )
        ):
            raise ValueError(
                "HUD EnsureHudLoaded unsupported-helper bridge rejects "
                f"duplicate, undefined, or case-folded {label} aliases"
            )
        helper_definition = candidate.tu_local_function_definitions.get(
            coff_symbol
        )
        if (
            helper_definition is None
            or helper_definition.symbol != coff_symbol
            or not helper_definition.data
            or helper_definition.section_size
            != len(helper_definition.data)
            or len(helper_definition.relocation_mask)
            != len(helper_definition.data)
            or helper_definition.section_external_functions != (coff_symbol,)
        ):
            raise ValueError(
                "HUD EnsureHudLoaded unsupported-helper bridge requires one "
                f"exact nonempty unaliased {label} TU-local definition"
            )

        instruction_offsets: list[int] = []
        for instruction_index, instruction, _ in calls:
            instruction_offset = complete_offsets[instruction_index]
            if (
                _cc_cfg._instruction_mnemonic(instruction) != "call"
                or tuple(value.lower() for value in instruction.bytes)
                != ("e8", "00", "00", "00", "00")
                or instruction_offset is None
                or _cc_cfg._cleanup_after(
                    candidate.instructions,
                    instruction_index,
                )
                is not None
            ):
                raise ValueError(
                    "HUD EnsureHudLoaded unsupported-helper bridge requires "
                    "exact structurally located callee-cleanup E8 calls for "
                    f"{label}"
                )
            instruction_offsets.append(instruction_offset)
        if (
            len(set(instruction_offsets)) != len(instruction_offsets)
            or tuple(instruction_offsets) != tuple(sorted(instruction_offsets))
        ):
            raise ValueError(
                "HUD EnsureHudLoaded unsupported-helper bridge requires the "
                f"unique ordered {label} call topology"
            )

        references = tuple(
            sorted(
                (
                    relocation
                    for relocation in definition.relocations
                    if relocation.symbol_name.casefold() in folded_names
                ),
                key=lambda relocation: relocation.offset,
            )
        )
        expected_relocation_offsets = tuple(
            offset + 1 for offset in instruction_offsets
        )
        if (
            tuple(reference.offset for reference in references)
            != expected_relocation_offsets
            or any(
                reference.symbol_name != coff_symbol
                for reference in references
            )
        ):
            raise ValueError(
                "HUD EnsureHudLoaded unsupported-helper bridge requires one "
                f"exact ordered COFF relocation per {label} call"
            )
        for reference in references:
            instruction_offset = reference.offset - 1
            field_end = reference.offset + 4
            if (
                reference.type != IMAGE_REL_I386_REL32
                or field_end > len(definition.data)
                or field_end > len(definition.relocation_mask)
                or definition.data[instruction_offset:reference.offset]
                != b"\xe8"
                or definition.relocation_mask[instruction_offset]
                or struct.unpack_from(
                    "<I",
                    definition.data,
                    reference.offset,
                )[0]
                != 0
                or not all(
                    definition.relocation_mask[index]
                    for index in range(reference.offset, field_end)
                )
            ):
                raise ValueError(
                    "HUD EnsureHudLoaded unsupported-helper bridge requires "
                    f"zero-addend fully masked E8 REL32 evidence for {label}"
                )
        result[cod_symbol] = caller_identity

    return result
