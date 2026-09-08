"""Recoil call-contract recoil audio evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import receiver_candidate as _cc_receiver_candidate
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions
from _recoil.call_contract import receiver_retail as _cc_receiver_retail
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        CandidateCallerDefinition,
        IdentityIndexes,
        ProviderOrdinalImportThunk,
    )

import struct
from collections import Counter
from pathlib import Path, PureWindowsPath
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_DIR32,
    IMAGE_REL_I386_REL32,
    IMAGE_SYM_CLASS_EXTERNAL,
    IMAGE_SYM_CLASS_STATIC,
    Instruction,
)
from _recoil.commands.provider_target_mutation import _retail_import_targets
from _recoil.lib.binja import BinaryNinjaBridge
from _recoil.lib.progress import (
    AUTHORED_ORDER_DIMENSIONS,
    ProgressDocument,
    address_value,
    normalize_address,
)


def _exact_candidate_zsnd_playwithdelta_a3d_backend_vptr_proofs(
    instructions: Sequence[Instruction],
    *,
    invocation_indices: Sequence[int],
    addresses: Sequence[int | None],
    caller_start: int,
    caller_end: int,
    indexes: IdentityIndexes,
    local_control_flow_indices: frozenset[int],
    local_control_flow_targets: Mapping[int, tuple[int, ...]],
) -> dict[int, str]:
    """Prove the complete PlayWithDelta_A3D backendBuffer call population.

    VC5 retains the first backendBuffer vptr in callee-saved EBP across the
    exact PlaySimple conversion call.  The ordinary register interpreter
    deliberately discards register state across calls, so this one reviewed
    caller needs a finite whole-body proof.  The other two arms are included
    in the same package to bind the dynamic marker to one complete receiver,
    argument, slot, and CFG population; no static COM target is inferred.
    """

    if (
        caller_start != 0x4A0380
        or caller_end != 0x4A0400
        or len(addresses) != len(instructions)
        or not instructions
        or addresses[0] != caller_start
    ):
        return {}
    counts = Counter(address for address in addresses if address is not None)
    index_by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and counts[address] == 1
    }
    expected_body = bytes.fromhex(
        "d9 44 24 08 dc 1d 00 00 00 00 53 55 56 57 8b f2 8b d9 "
        "df e0 f6 c4 41 75 28 d9 44 24 18 d8 46 24 8b 7e 08 "
        "d9 54 24 18 8b 44 24 18 d9 5e 24 8b 2f 50 "
        "e8 00 00 00 00 51 d9 1c 24 57 ff 95 a0 00 00 00 "
        "8b 44 24 14 85 c0 74 09 8b 46 08 50 8b 08 ff 51 3c "
        "8a 4b 08 8b 46 08 83 e1 01 8b 10 51 50 ff 52 34 "
        "85 c0 74 11 68 8a 05 00 00 ba 00 00 00 00 8b c8 "
        "e8 00 00 00 00 5f 5e 5d 5b c2 08 00"
    )
    selected: list[tuple[int, int, bytes]] = []
    for index, (instruction, address) in enumerate(zip(instructions, addresses)):
        if address is None:
            return {}
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            return {}
        selected.append((index, address, body))
    if (
        [index for index, _address, _body in selected]
        != list(range(len(instructions)))
        or any(
            address + len(body) != selected[position + 1][1]
            for position, (_index, address, body) in enumerate(selected[:-1])
        )
        or selected[-1][1] + len(selected[-1][2]) != caller_start + 0x7E
        or b"".join(body for _index, _address, body in selected)
        != expected_body
    ):
        return {}

    call_offsets = (0x31, 0x3B, 0x4F, 0x5F, 0x72)
    try:
        call_indices = tuple(
            index_by_address[caller_start + offset] for offset in call_offsets
        )
    except KeyError:
        return {}
    if tuple(invocation_indices) != call_indices:
        return {}
    if (
        _cc_cfg._instruction_operand(instructions[call_indices[0]]).strip()
        != _cc_catalog._ZSND_PLAY_SIMPLE_SYMBOL
        or indexes.by_candidate_name.get(_cc_catalog._ZSND_PLAY_SIMPLE_SYMBOL)
        != _cc_catalog._ZSND_PLAY_SIMPLE_IDENTITY
        or _cc_cfg._instruction_operand(instructions[call_indices[4]]).strip()
        != _cc_catalog._ZSND_REPORT_A3D_SYMBOL
        or indexes.by_candidate_name.get(_cc_catalog._ZSND_REPORT_A3D_SYMBOL)
        != _cc_catalog._ZSND_REPORT_A3D_IDENTITY
        or _cc_targets._exact_indirect_register_call_slot(instructions[call_indices[1]])
        != ("ebp", 0xA0)
        or _cc_targets._exact_indirect_register_call_slot(instructions[call_indices[2]])
        != ("ecx", 0x3C)
        or _cc_targets._exact_indirect_register_call_slot(instructions[call_indices[3]])
        != ("edx", 0x34)
    ):
        return {}

    row = lambda offset: instructions[index_by_address[caller_start + offset]]
    if (
        _cc_receiver_instructions._exact_register_move(row(0x0E)) != ("esi", "edx")
        or _cc_receiver_instructions._exact_register_move(row(0x10)) != ("ebx", "ecx")
        or _cc_receiver_candidate._exact_register_memory_load(row(0x20)) != ("edi", "esi", 0x08)
        or _cc_receiver_candidate._exact_register_memory_load(row(0x2E)) != ("ebp", "edi", 0)
        or _cc_receiver_candidate._exact_register_memory_load(row(0x49)) != ("eax", "esi", 0x08)
        or _cc_receiver_candidate._exact_register_memory_load(row(0x4D)) != ("ecx", "eax", 0)
        or _cc_receiver_candidate._exact_register_memory_load(row(0x55)) != ("eax", "esi", 0x08)
        or _cc_receiver_candidate._exact_register_memory_load(row(0x5B)) != ("edx", "eax", 0)
    ):
        return {}

    successors, unresolved = _cc_cfg._exact_invocation_cfg(
        instructions,
        instruction_addresses=addresses,
        instruction_index_by_address=index_by_address,
        source="cod",
        caller_start=caller_start,
        caller_end=caller_end,
        local_control_flow_indices=local_control_flow_indices,
        local_control_flow_targets=dict(local_control_flow_targets),
    )
    exact_branch_successors = {
        0x17: (0x19, 0x41),
        0x47: (0x49, 0x52),
        0x64: (0x66, 0x77),
    }
    if any(
        tuple(
            addresses[target]
            for target in successors.get(
                index_by_address[caller_start + offset], ()
            )
        )
        != tuple(caller_start + target for target in target_offsets)
        for offset, target_offsets in exact_branch_successors.items()
    ):
        return {}
    entry_reachable = _cc_cfg._reachable_cfg_indices(successors, (0,))
    if (
        unresolved & entry_reachable
        or not set(call_indices).issubset(entry_reachable)
        or any(
            not _cc_cfg._indices_reaching_cfg_target(successors, call_index)
            for call_index in call_indices
        )
    ):
        return {}
    return {
        call_indices[ordinal]: _cc_catalog._ZSND_PLAYWITHDELTA_A3D_CANDIDATE_VPTR
        for ordinal in (1, 2, 3)
    }


def _exact_candidate_zsnd_playwithdelta_directsound_backend_vptr_proofs(
    instructions: Sequence[Instruction],
    *,
    invocation_indices: Sequence[int],
    addresses: Sequence[int | None],
    caller_start: int,
    caller_end: int,
    indexes: IdentityIndexes,
    local_control_flow_indices: frozenset[int],
    local_control_flow_targets: Mapping[int, tuple[int, ...]],
) -> dict[int, str]:
    """Prove the disjoint complete DirectSound PlayWithDelta caller."""

    if (
        caller_start != 0x4A0400
        or caller_end != 0x4A0490
        or len(addresses) != len(instructions)
        or not instructions
        or addresses[0] != caller_start
    ):
        return {}
    counts = Counter(address for address in addresses if address is not None)
    index_by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and counts[address] == 1
    }
    expected_body = bytes.fromhex(
        "8b 44 24 08 56 57 8b f2 85 c0 8b f9 74 27 8b 4e 24 "
        "03 c8 8b 46 08 89 4e 24 51 8b 10 50 ff 52 3c "
        "85 c0 74 11 68 a5 05 00 00 ba 00 00 00 00 8b c8 "
        "e8 00 00 00 00 8b 44 24 0c 85 c0 74 20 8b 46 08 "
        "6a 00 50 8b 08 ff 51 34 85 c0 74 11 68 ad 05 00 00 "
        "ba 00 00 00 00 8b c8 e8 00 00 00 00 8a 47 08 "
        "8b 76 08 83 e0 01 8b 16 50 6a 00 6a 00 56 ff 52 30 "
        "85 c0 74 11 68 b4 05 00 00 ba 00 00 00 00 8b c8 "
        "e8 00 00 00 00 5f 5e c2 08 00"
    )
    selected: list[tuple[int, int, bytes]] = []
    for index, (instruction, address) in enumerate(zip(instructions, addresses)):
        if address is None:
            return {}
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            return {}
        selected.append((index, address, body))
    if (
        any(
            address + len(body) != selected[position + 1][1]
            for position, (_index, address, body) in enumerate(selected[:-1])
        )
        or selected[-1][1] + len(selected[-1][2]) != caller_start + 0x8B
        or b"".join(body for _index, _address, body in selected)
        != expected_body
    ):
        return {}

    call_offsets = (0x1D, 0x30, 0x45, 0x58, 0x6E, 0x81)
    try:
        call_indices = tuple(
            index_by_address[caller_start + offset] for offset in call_offsets
        )
    except KeyError:
        return {}
    if tuple(invocation_indices) != call_indices:
        return {}
    if (
        any(
            _cc_cfg._instruction_operand(instructions[call_indices[ordinal]]).strip()
            != _cc_catalog._ZSND_REPORT_DIRECTSOUND_SYMBOL
            for ordinal in (1, 3, 5)
        )
        or indexes.by_candidate_name.get(_cc_catalog._ZSND_REPORT_DIRECTSOUND_SYMBOL)
        != _cc_catalog._ZSND_REPORT_DIRECTSOUND_IDENTITY
        or _cc_targets._exact_indirect_register_call_slot(instructions[call_indices[0]])
        != ("edx", 0x3C)
        or _cc_targets._exact_indirect_register_call_slot(instructions[call_indices[2]])
        != ("ecx", 0x34)
        or _cc_targets._exact_indirect_register_call_slot(instructions[call_indices[4]])
        != ("edx", 0x30)
    ):
        return {}
    row = lambda offset: instructions[index_by_address[caller_start + offset]]
    if (
        _cc_receiver_instructions._exact_register_move(row(0x06)) != ("esi", "edx")
        or _cc_receiver_instructions._exact_register_move(row(0x0A)) != ("edi", "ecx")
        or _cc_receiver_candidate._exact_register_memory_load(row(0x13)) != ("eax", "esi", 0x08)
        or _cc_receiver_candidate._exact_register_memory_load(row(0x1A)) != ("edx", "eax", 0)
        or _cc_receiver_candidate._exact_register_memory_load(row(0x3D)) != ("eax", "esi", 0x08)
        or _cc_receiver_candidate._exact_register_memory_load(row(0x43)) != ("ecx", "eax", 0)
        or _cc_receiver_candidate._exact_register_memory_load(row(0x60)) != ("esi", "esi", 0x08)
        or _cc_receiver_candidate._exact_register_memory_load(row(0x66)) != ("edx", "esi", 0)
    ):
        return {}

    successors, unresolved = _cc_cfg._exact_invocation_cfg(
        instructions,
        instruction_addresses=addresses,
        instruction_index_by_address=index_by_address,
        source="cod",
        caller_start=caller_start,
        caller_end=caller_end,
        local_control_flow_indices=local_control_flow_indices,
        local_control_flow_targets=dict(local_control_flow_targets),
    )
    exact_branch_successors = {
        0x0C: (0x0E, 0x35),
        0x22: (0x24, 0x35),
        0x3B: (0x3D, 0x5D),
        0x4A: (0x4C, 0x5D),
        0x73: (0x75, 0x86),
    }
    if any(
        set(successors.get(index_by_address[caller_start + offset], ()))
        != {
            index_by_address.get(caller_start + target)
            for target in target_offsets
        }
        for offset, target_offsets in exact_branch_successors.items()
    ):
        return {}
    entry_reachable = _cc_cfg._reachable_cfg_indices(successors, (0,))
    if unresolved & entry_reachable or not set(call_indices).issubset(entry_reachable):
        return {}
    return {
        call_indices[ordinal]: _cc_catalog._ZSND_PLAYWITHDELTA_A3D_CANDIDATE_VPTR
        for ordinal in (0, 2, 4)
    }


def _exact_candidate_zsnd_apply_mute_backend_vptr_proofs(
    instructions: Sequence[Instruction],
    *,
    invocation_indices: Sequence[int],
    addresses: Sequence[int | None],
    caller_start: int,
    caller_end: int,
    indexes: IdentityIndexes,
    local_control_flow_indices: frozenset[int],
    local_control_flow_targets: Mapping[int, tuple[int, ...]],
) -> dict[int, str]:
    """Prove the complete dual-backend ApplyMute snapshot traversal."""

    if (
        caller_start != 0x4A0670
        or caller_end != 0x4A07A0
        or len(addresses) != len(instructions)
        or not instructions
        or addresses[0] != caller_start
    ):
        return {}
    counts = Counter(address for address in addresses if address is not None)
    index_by_address = {
        address: index for index, address in enumerate(addresses)
        if address is not None and counts[address] == 1
    }
    expected_body = bytes.fromhex(
        "a10000000083ec0885c0568bf1750733c05e83c408c3575553"
        "e80000000089442414a10000000085f6740340eb0148"
        "8b0d00000000a3000000008b150000000033c085d20f9fc08901"
        "e8000000008b6804a10000000085c08b55008b3275343bf5747f"
        "8b5e088b7b08e80000000085c0b8f0d8ffff75038b43248b0f5057"
        "ff513c8b363bf575dc8b4424145b5d5f5e83c408c3"
        "83f801754a3bf574468b5e088b7b08e8000000008b1785c0740b"
        "6a0057ff92a0000000eb248b4b2489542410e80000000051d91c24"
        "e8000000008b44241051d91c2457ff90a00000008b363bf575ba"
        "8b4424145b5d5f5e83c408c3"
    )
    selected: list[tuple[int, int, bytes]] = []
    for index, (instruction, address) in enumerate(zip(instructions, addresses)):
        if address is None:
            return {}
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            return {}
        selected.append((index, address, body))
    if (
        any(
            address + len(body) != selected[position + 1][1]
            for position, (_index, address, body) in enumerate(selected[:-1])
        )
        or selected[-1][1] + len(selected[-1][2]) != caller_start + 0xEE
        or b"".join(body for _index, _address, body in selected) != expected_body
    ):
        return {}
    call_offsets = (0x19, 0x49, 0x69, 0x7E, 0xA2, 0xB0, 0xBF, 0xC8, 0xD6)
    try:
        calls = tuple(index_by_address[caller_start + offset] for offset in call_offsets)
    except KeyError:
        return {}
    call_invocations = tuple(
        index for index in invocation_indices
        if _cc_cfg._instruction_mnemonic(instructions[index]) == "call"
    )
    if call_invocations != calls:
        return {}
    if (
        any(
            _cc_cfg._instruction_operand(instructions[calls[ordinal]]).strip()
            != _cc_catalog._ZSND_IS_MUTED_SYMBOL for ordinal in (0, 2, 4)
        )
        or indexes.by_candidate_name.get(_cc_catalog._ZSND_IS_MUTED_SYMBOL)
        != _cc_catalog._ZSND_IS_MUTED_IDENTITY
        or _cc_cfg._instruction_operand(instructions[calls[1]]).strip()
        != _cc_catalog._ZSND_SNAPSHOT_CREATE_CALLER_SYMBOL
        or indexes.by_candidate_name.get(_cc_catalog._ZSND_SNAPSHOT_CREATE_CALLER_SYMBOL)
        != _cc_catalog._ZSND_SNAPSHOT_CREATE_CALLER_IDENTITY
        or _cc_cfg._instruction_operand(instructions[calls[7]]).strip()
        != _cc_catalog._ZSND_PLAY_SIMPLE_SYMBOL
        or indexes.by_candidate_name.get(_cc_catalog._ZSND_PLAY_SIMPLE_SYMBOL)
        != _cc_catalog._ZSND_PLAY_SIMPLE_IDENTITY
        or _cc_targets._exact_indirect_register_call_slot(instructions[calls[3]])
        != ("ecx", 0x3C)
        or _cc_targets._exact_indirect_register_call_slot(instructions[calls[5]])
        != ("edx", 0xA0)
        or _cc_targets._exact_indirect_register_call_slot(instructions[calls[8]])
        != ("eax", 0xA0)
    ):
        return {}
    try:
        float_name = _cc_targets._cod_space_bearing_direct_target(
            _cc_cfg._instruction_operand(instructions[calls[6]]),
            instructions[calls[6]].source_line,
        )
    except ValueError:
        return {}
    match = _cc_catalog._VC5_TU_LOCAL_CALLABLE_RE.fullmatch(float_name or "")
    if (
        match is None
        or match.group("head") != "?FloatFromBits@?%"
        or match.group("suffix") != "YIMH@Z"
        or not PureWindowsPath(match.group("source")).as_posix().casefold().endswith(
            "src/gamezrecoil/zsound/zsnd_play.cpp"
        )
    ):
        return {}
    row = lambda offset: instructions[index_by_address[caller_start + offset]]
    if (
        _cc_receiver_candidate._exact_register_memory_load(row(0x63)) != ("ebx", "esi", 0x08)
        or _cc_receiver_candidate._exact_register_memory_load(row(0x66)) != ("edi", "ebx", 0x08)
        or _cc_receiver_candidate._exact_register_memory_load(row(0x7A)) != ("ecx", "edi", 0)
        or _cc_receiver_candidate._exact_register_memory_load(row(0x9C)) != ("ebx", "esi", 0x08)
        or _cc_receiver_candidate._exact_register_memory_load(row(0x9F)) != ("edi", "ebx", 0x08)
        or _cc_receiver_candidate._exact_register_memory_load(row(0xA7)) != ("edx", "edi", 0)
        or _cc_receiver_instructions._exact_stack_slot_load(row(0xCD)) != ("eax", 0x10)
    ):
        return {}
    successors, unresolved = _cc_cfg._exact_invocation_cfg(
        instructions,
        instruction_addresses=addresses,
        instruction_index_by_address=index_by_address,
        source="cod",
        caller_start=caller_start,
        caller_end=caller_end,
        local_control_flow_indices=local_control_flow_indices,
        local_control_flow_targets=dict(local_control_flow_targets),
    )
    entry_reachable = _cc_cfg._reachable_cfg_indices(successors, (0,))
    if unresolved & entry_reachable or not set(calls).issubset(entry_reachable):
        return {}
    return {
        calls[ordinal]: _cc_catalog._ZSND_APPLY_MUTE_CANDIDATE_VPTR
        for ordinal in (3, 5, 8)
    }


def _exact_candidate_zsnd_play_selected_backend_vptr_proofs(
    instructions: Sequence[Instruction],
    *,
    invocation_indices: Sequence[int],
    addresses: Sequence[int | None],
    caller_start: int,
    caller_end: int,
    indexes: IdentityIndexes,
    local_control_flow_indices: frozenset[int],
    local_control_flow_targets: Mapping[int, tuple[int, ...]],
    candidate_caller_definition: CandidateCallerDefinition | None = None,
) -> dict[int, str]:
    """Prove selected backend-buffer loads over the current candidate CFG.

    The registered acquisition result and embedded fallback must both reach
    the same member/vptr loads. No historical offsets or call population
    qualify the receiver, and no static COM target is invented.
    """
    definition = candidate_caller_definition
    if ((caller_start, caller_end) not in {(0x49FA60, 0x49FBB0), (0x49FBB0, 0x49FCF0)}
            or definition is None or len(addresses) != len(instructions)):
        return {}
    identities = _cc_receiver_candidate._candidate_coff_direct_call_identities(
        instructions, addresses=addresses, caller_start=caller_start,
        definition=definition, indexes=indexes,
    )
    if list(identities.values()).count(_cc_catalog._ZSND_PLAY_ACQUIRE_HANDLE_IDENTITY) != 1:
        return {}
    local_end = caller_start + len(definition.data)
    by_address = {address: index for index, address in enumerate(addresses)
                  if address is not None}
    successors, unresolved = _cc_cfg._exact_invocation_cfg(
        instructions, instruction_addresses=addresses,
        instruction_index_by_address=by_address, source="cod",
        caller_start=caller_start, caller_end=local_end,
        local_control_flow_indices=local_control_flow_indices,
        local_control_flow_targets=local_control_flow_targets,
    )
    proofs: dict[int, str] = {}
    for index in invocation_indices:
        slot = _cc_targets._exact_indirect_register_call_slot(instructions[index], allow_zero=True)
        if slot is None:
            continue
        # This adapter proves one execution of each playback arm, not a
        # cyclic call site whose later arrivals could have other receivers.
        if index in _cc_cfg._reachable_cfg_indices(successors, successors.get(index, ())):
            continue
        value = _cc_receiver_retail._exact_retail_cfg_register_provenance(
            instructions, before_index=index, register=slot[0], addresses=addresses,
            indexes=indexes, caller_start=caller_start, caller_end=local_end,
            local_control_flow_indices=local_control_flow_indices,
            local_control_flow_targets=local_control_flow_targets,
            allow_exact_affine_receiver_roots=True, source="cod",
            direct_call_identities=identities,
        )
        if value == _cc_catalog._ZSND_PLAY_SELECTED_BACKEND_VPTR:
            proofs[index] = value
    return proofs


def _zsnd_play_directsound_candidate_projection(
    expected: Sequence[Mapping[str, Any]],
    candidate_contract: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    retail_instructions: Sequence[Instruction],
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> list[dict[str, Any]]:
    """Unify only independently proven retail/candidate receiver storage.

    Preserve every observed call, ordinal, form, slot and cleanup field. In
    particular, never duplicate a combined volume call or remove a marker
    helper. Such source differences must fail ordinary direct comparison.
    """
    del candidate
    result = [dict(row) for row in candidate_contract]
    if (caller_identity != "symbol:recoil:function:0x49fbb0"
            or normalize_address(caller_start) != "0x49fbb0"
            or normalize_address(caller_end_exclusive) != "0x49fcf0"
            or len(result) != len(expected)):
        return result
    addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions, source="bn", caller_start=0x49FBB0,
    )
    calls = [index for index, instruction in enumerate(retail_instructions)
             if _cc_cfg._instruction_mnemonic(instruction) == "call"
             and _cc_cfg._exact_invocation_encoding(instruction, mnemonic="call")]
    if len(calls) != len(expected):
        return result
    for ordinal, (observed, required) in enumerate(zip(result, expected)):
        if (observed.get("storage_identity") not in {
                _cc_catalog._ZSND_PLAY_SELECTED_BACKEND_VPTR,
                _cc_catalog._ZSND_PLAY_DIRECTSOUND_INBOUND_SELECTED_BACKEND_VPTR,
            }
                or required.get("storage_identity") != _cc_catalog._ZSND_PLAY_DIRECTSOUND_RETAIL_BACKEND_VPTR
                or any(observed.get(key) != required.get(key) for key in (
                    "ordinal", "form", "dispatch", "identity_kind",
                    "target_identity", "slot_displacement", "cleanup_bytes",
                ))
                or required.get("ordinal") != ordinal):
            continue
        slot = _cc_targets._exact_indirect_register_call_slot(
            retail_instructions[calls[ordinal]], allow_zero=True,
        )
        if slot is None or slot[1] != required.get("slot_displacement"):
            continue
        retail_value = _cc_receiver_retail._exact_retail_cfg_register_provenance(
            retail_instructions, before_index=calls[ordinal], register=slot[0],
            addresses=addresses, indexes=indexes,
            caller_start=0x49FBB0, caller_end=0x49FCF0,
            local_control_flow_indices=frozenset(), local_control_flow_targets={},
            allow_exact_affine_receiver_roots=True,
        )
        if retail_value == _cc_catalog._ZSND_PLAY_SELECTED_BACKEND_VPTR:
            observed["storage_identity"] = _cc_catalog._ZSND_PLAY_DIRECTSOUND_RETAIL_BACKEND_VPTR
    return result


def _directsoundcreate_ordinal1_candidate_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    retail_instructions: Sequence[Instruction],
    *,
    document: ProgressDocument,
    thunks: Sequence[ProviderOrdinalImportThunk],
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    bridge: BinaryNinjaBridge,
    retail_import_targets: Sequence[Any] | None = None,
) -> dict[str, str]:
    """Bridge the one DX6 decorated callable to retail DSOUND ordinal 1.

    Retail's import is ordinal-only, while the canonical DX6 header makes VC5
    emit the documented stdcall callable ``_DirectSoundCreate@12``.  This
    finite proof consumes the already-proven PE ordinal thunk and admits that
    one source-level spelling only in ``zSndBackend::InitDirectSound``.  It
    creates no tracker/provider/IAT fact and does not generalize ordinal
    aliases from decorations.
    """

    normalized_start = normalize_address(caller_start)
    caller_profile = _cc_catalog.DIRECTSOUND_CREATE_CALLER_PROFILES.get(normalized_start)
    if caller_profile is None:
        return {}

    normalized_end = normalize_address(caller_end_exclusive)
    exact_caller_identity = str(caller_profile["identity"])
    caller_symbol_id = exact_caller_identity.removeprefix("symbol:")
    caller_row = document.collection("symbols").get(caller_symbol_id)
    caller_trace = (
        caller_row.get("source_traceability")
        if isinstance(caller_row, Mapping)
        else None
    )
    source_edges = (
        caller_trace.get("source_edges")
        if isinstance(caller_trace, Mapping)
        else None
    )
    target = candidate.target
    caller = candidate.caller_definition
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == normalized_start
    ]
    if (
        caller_identity != exact_caller_identity
        or normalized_end != caller_profile["end_exclusive"]
        or indexes.by_address.get(normalized_start) != caller_identity
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_row, Mapping)
        or caller_row.get("binary") != "recoil"
        or caller_row.get("kind") != "function"
        or caller_row.get("pipeline_class") != "authored"
        or caller_row.get("ownership_state") != "primary-owned"
        or caller_row.get("address") != normalized_start
        or caller_row.get("end_exclusive") != caller_profile["end_exclusive"]
        or caller_row.get("extent_state") != "known"
        or caller_row.get("size") != caller_profile["candidate_size"]
        or caller_row.get("navigation_name")
        != caller_profile["navigation_name"]
        or caller_row.get("output_section_id") != "recoil:section:.text"
        or caller_row.get("physical_block_id")
        != caller_profile["physical_block_id"]
        or not isinstance(caller_trace, Mapping)
        or caller_trace.get("state") != caller_profile["source_trace_state"]
        or caller_trace.get("reason_code")
        != caller_profile["source_trace_reason"]
        or not isinstance(source_edges, list)
        or source_edges != list(caller_profile["source_edges"])
        or caller is None
        or caller.symbol != caller_profile["symbol"]
        or caller.section_index != caller_profile["candidate_section_index"]
        or caller.section_start != 0
        or caller.section_end != caller_profile["candidate_size"]
        or len(caller.data) != caller_profile["candidate_size"]
        or len(caller.relocation_mask) != len(caller.data)
        or target is None
        or getattr(target, "name", "") != caller_profile["target_name"]
        or getattr(target, "target_binary", "") != "recoil"
        or Path(str(getattr(target, "manifest_path", ""))).resolve()
        != Path(caller_profile["target_manifest"]).resolve()
        or not bool(
            getattr(target, "check_translation_unit_function_order", False)
        )
        or len(contribution_rows) != 1
    ):
        raise ValueError(
            "DirectSoundCreate ordinal-1 bridge requires the exact authored "
            "caller, source-trace state, target, TU contribution, and COFF "
            "candidate package"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != caller_profile["source_path"]
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "")
        != caller_profile["symbol"]
        or getattr(contribution_row, "symbol_regex", None) is not None
        or getattr(contribution_row, "name", "")
        != caller_profile["navigation_name"]
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "DirectSoundCreate ordinal-1 bridge requires the exact authored "
            "caller contribution row"
        )

    provider_symbol = document.collection("symbols").get(
        _cc_catalog.ZSND_DIRECTSOUND_CREATE_PROVIDER_SYMBOL_ID
    )
    provider_block = document.collection("physical_blocks").get(
        _cc_catalog.ZSND_DIRECTSOUND_CREATE_PROVIDER_BLOCK_ID
    )
    mapping = (
        provider_block.get("mapping")
        if isinstance(provider_block, Mapping)
        else None
    )
    authored_order = (
        provider_block.get("order", {}).get("authored")
        if isinstance(provider_block, Mapping)
        and isinstance(provider_block.get("order"), Mapping)
        else None
    )
    current_not_applicable = (
        isinstance(authored_order, Mapping)
        and all(
            isinstance(authored_order.get(dimension), Mapping)
            and authored_order[dimension].get("result") == "not-applicable"
            and authored_order[dimension].get("disposition") == "accepted"
            and authored_order[dimension].get("freshness") == "current"
            and authored_order[dimension].get("gating") is True
            and authored_order[dimension].get("validation_mode") == "live"
            for dimension in AUTHORED_ORDER_DIMENSIONS
        )
    )
    primary_owner_rows = [
        owner_id
        for owner_id, owner in document.collection("owners").items()
        if isinstance(owner, Mapping)
        and any(
            isinstance(relationship, Mapping)
            and relationship.get("kind") == "primary-function"
            and relationship.get("symbol_id")
            == _cc_catalog.ZSND_DIRECTSOUND_CREATE_PROVIDER_SYMBOL_ID
            for relationship in owner.get("relationships", ())
        )
    ]
    if (
        not isinstance(provider_symbol, Mapping)
        or provider_symbol.get("binary") != "recoil"
        or provider_symbol.get("kind") != "function"
        or provider_symbol.get("pipeline_class") != "non-authored"
        or provider_symbol.get("authored_order_role") != "non-authored"
        or provider_symbol.get("ownership_state") != "unresolved"
        or provider_symbol.get("disposition") != "unresolved"
        or provider_symbol.get("extent_state") != "known"
        or provider_symbol.get("address")
        != _cc_catalog.ZSND_DIRECTSOUND_CREATE_THUNK_ADDRESS
        or provider_symbol.get("end_exclusive")
        != _cc_catalog.ZSND_DIRECTSOUND_CREATE_THUNK_END_EXCLUSIVE
        or provider_symbol.get("size") != 6
        or provider_symbol.get("navigation_name") != "DirectSoundCreate"
        or provider_symbol.get("output_section_id") != "recoil:section:.text"
        or provider_symbol.get("physical_block_id")
        != _cc_catalog.ZSND_DIRECTSOUND_CREATE_PROVIDER_BLOCK_ID
        or primary_owner_rows
        or not isinstance(provider_block, Mapping)
        or provider_block.get("binary") != "recoil"
        or provider_block.get("row_kind") != "physical-source-block"
        or provider_block.get("contribution_kind") != "provider"
        or provider_block.get("start") != "0x4c637c"
        or provider_block.get("end_exclusive") != "0x4c63f0"
        or provider_block.get("agent_source_path")
        != "provider:platform-directx-tail-import-thunks"
        or provider_block.get("source_path")
        != "provider:platform-directx-tail-import-thunks"
        or not isinstance(mapping, Mapping)
        or mapping.get("status") != "provider-boundary"
        or not current_not_applicable
        or not isinstance(provider_block.get("contribution_ids"), list)
        or provider_block["contribution_ids"].count(
            _cc_catalog.ZSND_DIRECTSOUND_CREATE_PROVIDER_SYMBOL_ID
        )
        != 1
        or indexes.by_address.get(_cc_catalog.ZSND_DIRECTSOUND_CREATE_THUNK_ADDRESS)
        != _cc_catalog.ZSND_DIRECTSOUND_CREATE_PROVIDER_IDENTITY
        or _cc_catalog.ZSND_DIRECTSOUND_CREATE_PROVIDER_IDENTITY
        not in indexes.provider_ids
    ):
        raise ValueError(
            "DirectSoundCreate ordinal-1 bridge requires the exact unowned "
            "six-byte provider symbol and current DirectX tail block"
        )

    matching_thunks = [
        thunk
        for thunk in thunks
        if thunk.provider_identity
        == _cc_catalog.ZSND_DIRECTSOUND_CREATE_PROVIDER_IDENTITY
        and thunk.thunk_address == _cc_catalog.ZSND_DIRECTSOUND_CREATE_THUNK_ADDRESS
        and thunk.retail_name == "DirectSoundCreate"
        and thunk.callable_symbol
        == _cc_catalog.ZSND_DIRECTSOUND_CREATE_RETAIL_CALLABLE_SYMBOL
        and thunk.iat_object_symbol
        == _cc_catalog.ZSND_DIRECTSOUND_CREATE_RETAIL_IAT_SYMBOL
        and thunk.iat_identity == "iat:ordinal:10:DSOUND.dll:1"
        and thunk.import_dll == _cc_catalog.ZSND_DIRECTSOUND_CREATE_IMPORT_DLL
        and thunk.import_ordinal == _cc_catalog.ZSND_DIRECTSOUND_CREATE_IMPORT_ORDINAL
    ]
    if len(matching_thunks) != 1:
        raise ValueError(
            "DirectSoundCreate ordinal-1 bridge requires one already-proven "
            "DSOUND.dll ordinal-1 retail thunk package"
        )
    if (
        indexes.storage_by_address.get(_cc_catalog.ZSND_DIRECTSOUND_CREATE_IAT_ADDRESS)
        != matching_thunks[0].iat_identity
        or indexes.storage_by_name.get(
            _cc_catalog.ZSND_DIRECTSOUND_CREATE_RETAIL_IAT_SYMBOL
        )
        != matching_thunks[0].iat_identity
    ):
        raise ValueError(
            "DirectSoundCreate ordinal-1 bridge lost its comparison-scoped "
            "immutable IAT identity"
        )

    supplied_targets = tuple(
        retail_import_targets
        if retail_import_targets is not None
        else _retail_import_targets(_cc_catalog.DEFAULT_REFERENCE)[0]
    )
    exact_imports = [
        item
        for item in supplied_targets
        if normalize_address(str(getattr(item, "address", "")))
        == _cc_catalog.ZSND_DIRECTSOUND_CREATE_IAT_ADDRESS
        and getattr(item, "dll", None) == _cc_catalog.ZSND_DIRECTSOUND_CREATE_IMPORT_DLL
        and getattr(item, "import_name", None) == "#1"
        and getattr(item, "import_ordinal", None)
        == _cc_catalog.ZSND_DIRECTSOUND_CREATE_IMPORT_ORDINAL
    ]
    tuple_collisions = [
        item
        for item in supplied_targets
        if getattr(item, "dll", None) == _cc_catalog.ZSND_DIRECTSOUND_CREATE_IMPORT_DLL
        and getattr(item, "import_ordinal", None)
        == _cc_catalog.ZSND_DIRECTSOUND_CREATE_IMPORT_ORDINAL
    ]
    address_collisions = [
        item
        for item in supplied_targets
        if normalize_address(str(getattr(item, "address", "")))
        == _cc_catalog.ZSND_DIRECTSOUND_CREATE_IAT_ADDRESS
    ]
    if (
        len(exact_imports) != 1
        or tuple_collisions != exact_imports
        or address_collisions != exact_imports
    ):
        raise ValueError(
            "DirectSoundCreate ordinal-1 bridge requires one collision-free "
            "immutable PE DSOUND.dll ordinal #1 tuple"
        )

    thunk_body = _cc_cfg._hexdump_bytes(
        bridge.hexdump(_cc_catalog.ZSND_DIRECTSOUND_CREATE_THUNK_ADDRESS, 6)
    )
    get_json = getattr(bridge, "get_json", None)
    if thunk_body != _cc_catalog.ZSND_DIRECTSOUND_CREATE_THUNK_BODY or not callable(
        get_json
    ):
        raise ValueError(
            "DirectSoundCreate ordinal-1 bridge requires exact immutable "
            "FF25 thunk bytes and complete xref authority"
        )
    thunk_code, thunk_data, thunk_complete = _cc_identity._bn_inbound_xref_items(
        get_json(
            "getXrefsTo",
            address=_cc_catalog.ZSND_DIRECTSOUND_CREATE_THUNK_ADDRESS,
            limit=100000,
        )
    )
    iat_code, iat_data, iat_complete = _cc_identity._bn_inbound_xref_items(
        get_json(
            "getXrefsTo",
            address=_cc_catalog.ZSND_DIRECTSOUND_CREATE_IAT_ADDRESS,
            limit=100000,
        )
    )
    if (
        not thunk_complete
        or thunk_data
        or tuple(
            _cc_identity._bn_xref_address(
                row,
                "source_address",
                "source_addr",
                "from_address",
                "address",
            )
            for row in thunk_code
        )
        != (
            _cc_catalog.ZSND_DIRECTSOUND_CREATE_RETAIL_CALL_ADDRESS,
            _cc_catalog.ZSND_DIRECTSOUND_CREATE_OTHER_RETAIL_CALL_ADDRESS,
        )
        or not iat_complete
        or iat_data
        or tuple(
            _cc_identity._bn_xref_address(
                row,
                "source_address",
                "source_addr",
                "from_address",
                "address",
            )
            for row in iat_code
        )
        != (_cc_catalog.ZSND_DIRECTSOUND_CREATE_THUNK_ADDRESS,)
    ):
        raise ValueError(
            "DirectSoundCreate ordinal-1 bridge requires the exact ordered "
            "two-caller retail population and sole immutable IAT reader"
        )

    retail_matches = [
        instruction
        for instruction in retail_instructions
        if _cc_cfg._source_instruction_address(instruction)
        == caller_profile["retail_call_address"]
    ]
    if len(retail_matches) != 1:
        raise ValueError(
            "DirectSoundCreate ordinal-1 bridge requires the exact unique "
            "retail call site"
        )
    try:
        retail_body = bytes(int(item, 16) for item in retail_matches[0].bytes)
    except (TypeError, ValueError):
        retail_body = b""
    if (
        _cc_cfg._instruction_mnemonic(retail_matches[0]) != "call"
        or retail_body != caller_profile["retail_call_body"]
        or normalize_address(
            address_value(str(caller_profile["retail_call_address"]))
            + 5
            + struct.unpack_from("<i", retail_body, 1)[0]
        )
        != _cc_catalog.ZSND_DIRECTSOUND_CREATE_THUNK_ADDRESS
    ):
        raise ValueError(
            "DirectSoundCreate ordinal-1 bridge requires retail E8 to the "
            "exact DirectSoundCreate thunk"
        )

    expected_rows = [
        row
        for row in expected
        if row.get("target_identity")
        == _cc_catalog.ZSND_DIRECTSOUND_CREATE_PROVIDER_IDENTITY
    ]
    exact_expected = {
        "ordinal": int(caller_profile["expected_ordinal"]),
        "form": "call",
        "dispatch": "direct",
        "identity_kind": "provider",
        "target_identity": _cc_catalog.ZSND_DIRECTSOUND_CREATE_PROVIDER_IDENTITY,
        "storage_identity": "",
        "slot_displacement": None,
        "cleanup_bytes": None,
    }
    if expected_rows != [exact_expected]:
        raise ValueError(
            "DirectSoundCreate ordinal-1 bridge requires the exact retail "
            "ordinal, direct-call form, provider identity, and stdcall cleanup"
        )

    casefolded_aliases = {
        str(name).casefold()
        for name in (
            *indexes.by_candidate_name,
            *indexes.storage_by_name,
            *bridge_names,
        )
    }
    if (
        _cc_catalog.ZSND_DIRECTSOUND_CREATE_CANDIDATE_SYMBOL.casefold()
        in casefolded_aliases
        or _cc_catalog.ZSND_DIRECTSOUND_CREATE_CANDIDATE_IAT_SYMBOL.casefold()
        in casefolded_aliases
    ):
        raise ValueError(
            "DirectSoundCreate ordinal-1 candidate alias collides with an "
            "ordinary callable, storage, or BN bridge identity"
        )

    offsets = tuple(_cc_callable_identity._candidate_complete_instruction_offsets(candidate))
    candidate_matches = [
        (index, offset, instruction)
        for index, (offset, instruction) in enumerate(
            zip(offsets, candidate.instructions)
        )
        if _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        and _cc_cfg._instruction_operand(instruction).strip()
        == _cc_catalog.ZSND_DIRECTSOUND_CREATE_CANDIDATE_SYMBOL
    ]
    candidate_case_collisions = [
        (index, offset, instruction)
        for index, (offset, instruction) in enumerate(
            zip(offsets, candidate.instructions)
        )
        if _cc_catalog.ZSND_DIRECTSOUND_CREATE_CANDIDATE_SYMBOL.casefold()
        in _cc_cfg._instruction_operand(instruction).casefold()
    ]
    alias_relocations = tuple(
        relocation
        for relocation in caller.relocations
        if relocation.symbol_name.casefold()
        in {
            _cc_catalog.ZSND_DIRECTSOUND_CREATE_CANDIDATE_SYMBOL.casefold(),
            _cc_catalog.ZSND_DIRECTSOUND_CREATE_CANDIDATE_IAT_SYMBOL.casefold(),
        }
    )
    undefined_aliases = [
        name
        for name in (
            *caller.undefined_external_functions,
            *caller.undefined_external_data,
            *caller.defined_external_functions,
            *caller.defined_external_data,
        )
        if name.casefold()
        in {
            _cc_catalog.ZSND_DIRECTSOUND_CREATE_CANDIDATE_SYMBOL.casefold(),
            _cc_catalog.ZSND_DIRECTSOUND_CREATE_CANDIDATE_IAT_SYMBOL.casefold(),
        }
    ]
    exact_alias_relocation = (
        len(alias_relocations) == 1
        and alias_relocations[0].offset
        == caller_profile["candidate_relocation_offset"]
        and alias_relocations[0].type == IMAGE_REL_I386_REL32
        and alias_relocations[0].symbol_name
        == _cc_catalog.ZSND_DIRECTSOUND_CREATE_CANDIDATE_SYMBOL
    )
    if (
        len(candidate_matches) != 1
        or candidate_case_collisions != candidate_matches
        or candidate_matches[0][1]
        != caller_profile["candidate_call_offset"]
        or candidate_matches[0][0] in candidate.local_control_flow_indices
        or _cc_cfg._cleanup_after(candidate.instructions, candidate_matches[0][0])
        is not None
        or tuple(candidate_matches[0][2].bytes)
        != ("e8", "00", "00", "00", "00")
        or not exact_alias_relocation
        or undefined_aliases
        != [_cc_catalog.ZSND_DIRECTSOUND_CREATE_CANDIDATE_SYMBOL]
        or caller.undefined_external_functions.count(
            _cc_catalog.ZSND_DIRECTSOUND_CREATE_CANDIDATE_SYMBOL
        )
        != 1
    ):
        raise ValueError(
            "DirectSoundCreate ordinal-1 bridge requires one exact direct "
            "candidate E8 and forbids aliases, IAT, mixed, duplicate, and "
            "caller-cleanup forms"
        )
    assert alias_relocations
    relocation = alias_relocations[0]
    field_end = relocation.offset + 4
    if (
        relocation.offset < 1
        or field_end > len(caller.data)
        or caller.data[relocation.offset - 1] != 0xE8
        or struct.unpack_from("<I", caller.data, relocation.offset)[0] != 0
        or not all(
            caller.relocation_mask[index]
            for index in range(relocation.offset, field_end)
        )
        or caller.relocation_mask[relocation.offset - 1]
    ):
        raise ValueError(
            "DirectSoundCreate ordinal-1 bridge requires the exact zero-addend "
            "fully masked caller-scoped E8 REL32 field"
        )

    return {
        _cc_catalog.ZSND_DIRECTSOUND_CREATE_CANDIDATE_SYMBOL: (
            _cc_catalog.ZSND_DIRECTSOUND_CREATE_PROVIDER_IDENTITY
        )
    }


def _merge_directsoundcreate_ordinal1_final_bridges(
    compiler_generated_bridges: Mapping[str, str],
    directsoundcreate_ordinal1_bridges: Mapping[str, str],
) -> dict[str, str]:
    """Consume the finite DirectSound alias in the final extraction map."""

    exact = {
        _cc_catalog.ZSND_DIRECTSOUND_CREATE_CANDIDATE_SYMBOL: (
            _cc_catalog.ZSND_DIRECTSOUND_CREATE_PROVIDER_IDENTITY
        )
    }
    if directsoundcreate_ordinal1_bridges not in ({}, exact):
        raise ValueError(
            "DirectSoundCreate ordinal-1 final bridge received an unexpected "
            "or partially consumed finite alias map"
        )
    merged = dict(compiler_generated_bridges)
    collisions = merged.keys() & directsoundcreate_ordinal1_bridges.keys()
    if any(
        merged[name] != directsoundcreate_ordinal1_bridges[name]
        for name in collisions
    ):
        raise ValueError(
            "DirectSoundCreate ordinal-1 final bridge conflicts with another "
            "reviewed compiler/provider identity"
        )
    merged.update(directsoundcreate_ordinal1_bridges)
    if any(
        merged.get(name) != identity
        for name, identity in directsoundcreate_ordinal1_bridges.items()
    ):
        raise ValueError(
            "DirectSoundCreate ordinal-1 final bridge was not consumed by "
            "the final candidate extraction map"
        )
    return merged


def _zsnd_apply_mute_candidate_projection(
    expected: Sequence[Mapping[str, Any]],
    candidate_contract: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    retail_instructions: Sequence[Instruction],
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
) -> list[dict[str, Any]]:
    """Reorder the exact dual-backend caller and inline FloatFromBits."""

    result = [dict(row) for row in candidate_contract]
    if normalize_address(caller_start) != "0x4a0670":
        return result
    if (
        caller_identity != "symbol:recoil:function:0x4a0670"
        or normalize_address(caller_end_exclusive) != "0x4a07a0"
        or len(expected) != 9
        or len(result) != 9
        or any(row.get("ordinal") != ordinal for ordinal, row in enumerate(expected))
        or any(row.get("ordinal") != ordinal for ordinal, row in enumerate(result))
    ):
        return result
    direct_specs = (
        (0, _cc_catalog._ZSND_IS_MUTED_IDENTITY),
        (1, _cc_catalog._ZSND_SNAPSHOT_CREATE_CALLER_IDENTITY),
        (2, _cc_catalog._ZSND_IS_MUTED_IDENTITY),
        (4, _cc_catalog._ZSND_IS_MUTED_IDENTITY),
        (7, _cc_catalog._ZSND_PLAY_SIMPLE_IDENTITY),
    )
    if any(
        result[ordinal].get("dispatch") != "direct"
        or result[ordinal].get("target_identity") != identity
        for ordinal, identity in direct_specs
    ):
        return result
    for ordinal, slot in ((3, 0x3C), (5, 0xA0), (8, 0xA0)):
        row = result[ordinal]
        if (
            row.get("dispatch") != "indirect"
            or row.get("identity_kind") != "virtual-slot"
            or row.get("target_identity") != ""
            or row.get("storage_identity") != _cc_catalog._ZSND_APPLY_MUTE_CANDIDATE_VPTR
            or row.get("slot_displacement") != slot
        ):
            return result
    helper_identity = str(result[6].get("target_identity", ""))
    if not helper_identity.startswith("candidate-local-coff:"):
        return result
    helper_name = helper_identity[len("candidate-local-coff:") :]
    match = _cc_catalog._VC5_TU_LOCAL_CALLABLE_RE.fullmatch(helper_name)
    helper = candidate.tu_local_function_definitions.get(helper_name)
    if (
        match is None
        or match.group("head") != "?FloatFromBits@?%"
        or match.group("suffix") != "YIMH@Z"
        or not PureWindowsPath(match.group("source")).as_posix().casefold().endswith(
            "src/gamezrecoil/zsound/zsnd_play.cpp"
        )
        or helper is None
        or helper.data != bytes.fromhex(
            "51 89 4c 24 00 d9 44 24 00 59 c3 90 90 90 90 90"
        )
        or helper.section_size != 0x10
        or helper.section_external_functions != (helper_name,)
        or not helper.section_is_comdat
        or helper.comdat_selection != 1
        or helper.relocations
        or any(helper.relocation_mask)
    ):
        return result
    retail_by_address = {
        address_value(address): instruction
        for instruction in retail_instructions
        if (address := _cc_cfg._source_instruction_address(instruction))
    }
    retail_calls = (
        (0x4A0685, bytes.fromhex("e8 16 01 00 00")),
        (0x4A06B5, bytes.fromhex("e8 36 f9 ff ff")),
        (0x4A06E9, bytes.fromhex("e8 b2 00 00 00")),
        (0x4A06FD, bytes.fromhex("ff 91 a0 00 00 00")),
        (0x4A070E, bytes.fromhex("e8 ed f2 ff ff")),
        (0x4A0718, bytes.fromhex("ff 95 a0 00 00 00")),
        (0x4A074F, bytes.fromhex("e8 4c 00 00 00")),
        (0x4A0766, bytes.fromhex("ff 51 3c")),
        (0x4A0778, bytes.fromhex("ff 52 3c")),
    )
    if any(
        address not in retail_by_address
        or bytes(int(item, 16) for item in retail_by_address[address].bytes) != body
        for address, body in retail_calls
    ):
        return result
    expected_indirect_specs = ((3, 0xA0), (5, 0xA0), (7, 0x3C), (8, 0x3C))
    if any(
        expected[ordinal].get("form") != "call"
        or expected[ordinal].get("dispatch") != "indirect"
        or expected[ordinal].get("identity_kind") != "virtual-slot"
        or expected[ordinal].get("target_identity") != ""
        or expected[ordinal].get("storage_identity")
        != _cc_catalog._ZSND_APPLY_MUTE_RETAIL_VPTR
        or expected[ordinal].get("slot_displacement") != slot
        or expected[ordinal].get("cleanup_bytes") is not None
        for ordinal, slot in expected_indirect_specs
    ):
        return result
    projected = [
        dict(result[0]), dict(result[1]), dict(result[4]), dict(result[5]),
        dict(result[7]), dict(result[8]), dict(result[2]),
        dict(result[3]), dict(result[3]),
    ]
    for ordinal, row in enumerate(projected):
        row["ordinal"] = ordinal
    for ordinal, _slot in expected_indirect_specs:
        projected[ordinal]["storage_identity"] = expected[ordinal][
            "storage_identity"
        ]
    return projected


def _zsnd_static_coordinator_candidate_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    retail_instructions: Sequence[Instruction],
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, str]:
    """Bind only the exact zSnd E6-to-E3 compiler-local call seam."""

    if normalize_address(caller_start) != "0x4a0800":
        return {}
    if (
        caller_identity != _cc_catalog._ZSND_STATIC_COORDINATOR_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive) != "0x4a0810"
        or indexes.by_address.get("0x4a0800") != caller_identity
        or indexes.by_address.get("0x4a0810")
        != _cc_catalog._ZSND_STATIC_VECTOR_CTOR_IDENTITY
        or _cc_catalog._ZSND_STATIC_VECTOR_CTOR_IDENTITY in indexes.provider_ids
        or len(expected) != 2
        or expected[0].get("ordinal") != 0
        or expected[0].get("form") != "call"
        or expected[0].get("dispatch") != "direct"
        or expected[0].get("identity_kind") != "direct"
        or expected[0].get("target_identity")
        != _cc_catalog._ZSND_STATIC_VECTOR_CTOR_IDENTITY
        or expected[0].get("storage_identity") != ""
        or expected[0].get("slot_displacement") is not None
        or expected[0].get("cleanup_bytes") is not None
        or expected[1].get("ordinal") != 1
        or expected[1].get("form") != "tail"
        or expected[1].get("dispatch") != "direct"
        or expected[1].get("identity_kind") != "direct"
        or expected[1].get("target_identity")
        != indexes.by_address.get("0x4a0830")
    ):
        raise ValueError(
            "zSnd static coordinator requires its exact registered retail "
            "E6/E3/E5 caller population"
        )

    caller = candidate.caller_definition
    definitions = candidate.compiler_local_function_definitions
    coordinator = definitions.get(_cc_catalog._ZSND_STATIC_COORDINATOR_COFF)
    vector_ctor = definitions.get(_cc_catalog._ZSND_STATIC_VECTOR_CTOR_COFF)
    atexit_helper = definitions.get(_cc_catalog._ZSND_STATIC_ATEXIT_COFF)
    coff_symbols = caller.coff_symbols if caller is not None else ()
    names = Counter(row.name for row in coff_symbols)
    by_index = {row.index: row for row in coff_symbols}
    coordinator_symbols = [
        row for row in coff_symbols if row.name == _cc_catalog._ZSND_STATIC_COORDINATOR_COFF
    ]
    vector_ctor_symbols = [
        row for row in coff_symbols if row.name == _cc_catalog._ZSND_STATIC_VECTOR_CTOR_COFF
    ]
    atexit_symbols = [
        row for row in coff_symbols if row.name == _cc_catalog._ZSND_STATIC_ATEXIT_COFF
    ]
    registry_symbols = [
        row
        for row in coff_symbols
        if row.name == "_g_zSnd_SampleSetRegistry"
    ]
    vector_provider_symbols = [
        row for row in coff_symbols if row.name == _cc_catalog._ZSND_VECTOR_CONSTRUCTOR_COFF
    ]
    destructor_thunk_symbols = [
        row for row in coff_symbols if row.name == "_$E4"
    ]
    atexit_provider_symbols = [
        row for row in coff_symbols if row.name == "_atexit"
    ]
    if (
        caller is None
        or caller.symbol != _cc_catalog._ZSND_STATIC_COORDINATOR_COFF
        or coordinator is None
        or vector_ctor is None
        or atexit_helper is None
        or len(coordinator_symbols) != 1
        or len(vector_ctor_symbols) != 1
        or len(atexit_symbols) != 1
        or len(registry_symbols) != 1
        or len(vector_provider_symbols) != 1
        or len(destructor_thunk_symbols) != 1
        or len(atexit_provider_symbols) != 1
        or names[_cc_catalog._ZSND_STATIC_COORDINATOR_COFF] != 1
        or names[_cc_catalog._ZSND_STATIC_VECTOR_CTOR_COFF] != 1
        or names[_cc_catalog._ZSND_STATIC_ATEXIT_COFF] != 1
    ):
        raise ValueError(
            "zSnd static coordinator rejects missing, duplicate, or aliased "
            "compiler-local COFF symbols"
        )
    coordinator_symbol = coordinator_symbols[0]
    vector_ctor_symbol = vector_ctor_symbols[0]
    atexit_symbol = atexit_symbols[0]
    registry_symbol = registry_symbols[0]
    vector_provider_symbol = vector_provider_symbols[0]
    destructor_thunk_symbol = destructor_thunk_symbols[0]
    atexit_provider_symbol = atexit_provider_symbols[0]
    if any(
        row.section_number <= 0
        or row.value != 0
        or row.symbol_type != 0x20
        or row.storage_class != IMAGE_SYM_CLASS_STATIC
        or row.aux_count != 0
        or row.weak_external_tag_index is not None
        or row.weak_external_characteristics is not None
        or row.section_name != ".text"
        or row.section_characteristics & _cc_catalog.IMAGE_SCN_LNK_COMDAT == 0
        for row in (coordinator_symbol, vector_ctor_symbol, atexit_symbol)
    ):
        raise ValueError(
            "zSnd static coordinator rejects compiler-local COFF "
            "storage, section, weak-alias, or ABI drift"
        )
    if (
        coordinator_symbol.section_size != 0x10
        or coordinator_symbol.natural_end != 0x10
        or vector_ctor_symbol.section_size != 0x20
        or vector_ctor_symbol.natural_end != 0x20
        or atexit_symbol.section_size != 0x10
        or atexit_symbol.natural_end != 0x10
        or coordinator_symbol.section_number
        in {vector_ctor_symbol.section_number, atexit_symbol.section_number}
        or vector_ctor_symbol.section_number == atexit_symbol.section_number
        or coordinator.section_size != 0x10
        or vector_ctor.section_size != 0x20
        or atexit_helper.section_size != 0x10
        or coordinator.section_function_symbols
        != (_cc_catalog._ZSND_STATIC_COORDINATOR_COFF,)
        or vector_ctor.section_function_symbols
        != (_cc_catalog._ZSND_STATIC_VECTOR_CTOR_COFF,)
        or atexit_helper.section_function_symbols
        != (_cc_catalog._ZSND_STATIC_ATEXIT_COFF,)
        or not coordinator.section_is_comdat
        or not vector_ctor.section_is_comdat
        or not atexit_helper.section_is_comdat
        or coordinator.comdat_selection != 1
        or vector_ctor.comdat_selection != 1
        or atexit_helper.comdat_selection != 1
    ):
        raise ValueError(
            "zSnd static coordinator rejects compiler-local extent, "
            "section population, or COMDAT-selection drift"
        )

    expected_source_suffix = "src/gamezrecoil/zsound/zsnd_play.cpp"
    coordinator_source = PureWindowsPath(
        coordinator.source_provenance
    ).as_posix().casefold()
    vector_ctor_source = PureWindowsPath(
        vector_ctor.source_provenance
    ).as_posix().casefold()
    atexit_source = PureWindowsPath(
        atexit_helper.source_provenance
    ).as_posix().casefold()
    if (
        coordinator_source != vector_ctor_source
        or coordinator_source != atexit_source
        or not coordinator_source.endswith(expected_source_suffix)
    ):
        raise ValueError(
            "zSnd static coordinator rejects cross-TU or source-provenance drift"
        )

    coordinator_body = bytes.fromhex(
        "e8 00 00 00 00 e9 00 00 00 00 90 90 90 90 90 90"
    )
    vector_ctor_body = bytes.fromhex(
        "51 8d 44 24 03 b9 00 00 00 00 50 e8 00 00 00 00 "
        "59 c3 90 90 90 90 90 90 90 90 90 90 90 90 90 90"
    )
    atexit_body = bytes.fromhex(
        "68 00 00 00 00 e8 00 00 00 00 83 c4 04 c3 90 90"
    )
    coordinator_relocations = tuple(
        (row.offset, row.symbol_index, row.type, row.symbol_name)
        for row in coordinator.relocations
    )
    vector_ctor_relocations = tuple(
        (row.offset, row.symbol_index, row.type, row.symbol_name)
        for row in vector_ctor.relocations
    )
    atexit_relocations = tuple(
        (row.offset, row.symbol_index, row.type, row.symbol_name)
        for row in atexit_helper.relocations
    )
    if (
        caller.data != coordinator_body
        or coordinator.data != coordinator_body
        or caller.relocations != coordinator.relocations
        or caller.relocation_mask != coordinator.relocation_mask
        or coordinator_relocations
        != (
            (1, vector_ctor_symbol.index, IMAGE_REL_I386_REL32,
             _cc_catalog._ZSND_STATIC_VECTOR_CTOR_COFF),
            (6, atexit_symbol.index, IMAGE_REL_I386_REL32,
             _cc_catalog._ZSND_STATIC_ATEXIT_COFF),
        )
        or coordinator.relocation_mask
        != tuple(index in set(range(1, 5)) | set(range(6, 10)) for index in range(16))
        or vector_ctor.data != vector_ctor_body
        or vector_ctor_relocations
        != (
            (6, registry_symbol.index,
             IMAGE_REL_I386_DIR32, "_g_zSnd_SampleSetRegistry"),
            (12, vector_provider_symbol.index,
             IMAGE_REL_I386_REL32, _cc_catalog._ZSND_VECTOR_CONSTRUCTOR_COFF),
        )
        or any(
            by_index.get(row.symbol_index) is None
            or by_index[row.symbol_index].name != row.symbol_name
            for row in (*coordinator.relocations, *vector_ctor.relocations)
        )
        or vector_ctor.relocation_mask
        != tuple(index in set(range(6, 10)) | set(range(12, 16)) for index in range(32))
        or atexit_helper.data != atexit_body
        or atexit_relocations
        != (
            (1, destructor_thunk_symbol.index, IMAGE_REL_I386_DIR32, "_$E4"),
            (6, atexit_provider_symbol.index, IMAGE_REL_I386_REL32, "_atexit"),
        )
        or atexit_helper.relocation_mask
        != tuple(index in set(range(1, 5)) | set(range(6, 10)) for index in range(16))
    ):
        raise ValueError(
            "zSnd static coordinator rejects exact body or one-to-one "
            "COFF relocation drift"
        )

    if (
        len(coordinator.instructions) != 2
        or _cc_cfg._instruction_mnemonic(coordinator.instructions[0]) != "call"
        or _cc_cfg._instruction_operand(coordinator.instructions[0])
        != _cc_catalog._ZSND_STATIC_VECTOR_CTOR_COFF
        or tuple(coordinator.instructions[0].bytes)
        != ("e8", "00", "00", "00", "00")
        or _cc_cfg._instruction_mnemonic(coordinator.instructions[1]) != "jmp"
        or _cc_cfg._instruction_operand(coordinator.instructions[1])
        != _cc_catalog._ZSND_STATIC_ATEXIT_COFF
        or tuple(coordinator.instructions[1].bytes)
        != ("e9", "00", "00", "00", "00")
        or vector_ctor.local_control_flow_indices
        or vector_ctor.local_control_flow_targets
        or atexit_helper.local_control_flow_indices
        or atexit_helper.local_control_flow_targets
        or candidate.local_control_flow_indices
        or candidate.local_control_flow_targets
        or not vector_ctor.instructions
        or _cc_cfg._instruction_mnemonic(vector_ctor.instructions[-1])
        not in {"ret", "retn"}
        or _cc_cfg._instruction_operand(vector_ctor.instructions[-1]) not in {"", "0"}
    ):
        raise ValueError(
            "zSnd static coordinator rejects direct opcode, population, or "
            "callee ABI drift"
        )

    retail_by_address = {
        address_value(address): instruction
        for instruction in retail_instructions
        if (address := _cc_cfg._source_instruction_address(instruction))
    }
    retail_specs = (
        (0x4A0800, bytes.fromhex("e8 0b 00 00 00"), "call"),
        (0x4A0805, bytes.fromhex("e9 26 00 00 00"), "jmp"),
    )
    if any(
        address not in retail_by_address
        or bytes(int(item, 16) for item in retail_by_address[address].bytes)
        != body
        or _cc_cfg._instruction_mnemonic(retail_by_address[address]) != mnemonic
        for address, body, mnemonic in retail_specs
    ):
        raise ValueError(
            "zSnd static coordinator rejects immutable retail call/tail population drift"
        )
    return {
        _cc_catalog._ZSND_STATIC_VECTOR_CTOR_COFF: _cc_catalog._ZSND_STATIC_VECTOR_CTOR_IDENTITY,
        _cc_catalog._ZSND_STATIC_ATEXIT_COFF: str(expected[1]["target_identity"]),
    }


def _zsnd_static_e3_inline_candidate_projection(
    expected: Sequence[Mapping[str, Any]],
    candidate_contract: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    retail_instructions: Sequence[Instruction],
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge: BinaryNinjaBridge,
) -> list[dict[str, Any]]:
    """Remove one exact candidate vector-ctor call in retail-inline E3."""

    result = [dict(row) for row in candidate_contract]
    if normalize_address(caller_start) != "0x4a0810":
        return result
    if (
        caller_identity != _cc_catalog._ZSND_STATIC_VECTOR_CTOR_IDENTITY
        or normalize_address(caller_end_exclusive) != "0x4a0830"
        or indexes.by_address.get("0x4a0810") != caller_identity
        or expected
        or len(result) != 1
        or result[0]
        != {
            "ordinal": 0,
            "form": "call",
            "dispatch": "direct",
            "identity_kind": "direct",
            "target_identity":
                f"candidate-local-coff:{_cc_catalog._ZSND_VECTOR_CONSTRUCTOR_COFF}",
            "storage_identity": "",
            "slot_displacement": None,
            "cleanup_bytes": 4,
        }
    ):
        raise ValueError(
            "zSnd static E3 inline projection requires exact empty retail "
            "and one-call candidate populations"
        )

    caller = candidate.caller_definition
    e3 = candidate.compiler_local_function_definitions.get(
        _cc_catalog._ZSND_STATIC_VECTOR_CTOR_COFF
    )
    vector_ctor = candidate.tu_local_function_definitions.get(
        _cc_catalog._ZSND_VECTOR_CONSTRUCTOR_COFF
    )
    coff_symbols = caller.coff_symbols if caller is not None else ()
    e3_symbols = [
        row for row in coff_symbols if row.name == _cc_catalog._ZSND_STATIC_VECTOR_CTOR_COFF
    ]
    vector_symbols = [
        row for row in coff_symbols if row.name == _cc_catalog._ZSND_VECTOR_CONSTRUCTOR_COFF
    ]
    registry_symbols = [
        row
        for row in coff_symbols
        if row.name == "_g_zSnd_SampleSetRegistry"
    ]
    if (
        caller is None
        or caller.symbol != _cc_catalog._ZSND_STATIC_VECTOR_CTOR_COFF
        or e3 is None
        or vector_ctor is None
        or len(e3_symbols) != 1
        or len(vector_symbols) != 1
        or len(registry_symbols) != 1
    ):
        raise ValueError(
            "zSnd static E3 inline projection rejects missing, duplicate, "
            "or aliased E3/vector-constructor COFF definitions"
        )
    e3_symbol = e3_symbols[0]
    vector_symbol = vector_symbols[0]
    registry_symbol = registry_symbols[0]
    if (
        e3_symbol.value != 0
        or e3_symbol.section_number <= 0
        or e3_symbol.symbol_type != 0x20
        or e3_symbol.storage_class != IMAGE_SYM_CLASS_STATIC
        or e3_symbol.aux_count != 0
        or e3_symbol.section_name != ".text"
        or e3_symbol.section_size != 0x20
        or e3_symbol.natural_end != 0x20
        or e3_symbol.section_characteristics & _cc_catalog.IMAGE_SCN_LNK_COMDAT == 0
        or vector_symbol.value != 0
        or vector_symbol.section_number <= 0
        or vector_symbol.section_number == e3_symbol.section_number
        or vector_symbol.symbol_type != 0x20
        or vector_symbol.storage_class != IMAGE_SYM_CLASS_EXTERNAL
        or vector_symbol.aux_count != 0
        or vector_symbol.weak_external_tag_index is not None
        or vector_symbol.weak_external_characteristics is not None
        or vector_symbol.section_name != ".text"
        or vector_symbol.section_size != 0x20
        or vector_symbol.natural_end != 0x20
        or vector_symbol.section_characteristics & _cc_catalog.IMAGE_SCN_LNK_COMDAT == 0
    ):
        raise ValueError(
            "zSnd static E3 inline projection rejects COFF storage, extent, "
            "section, ABI, or alias drift"
        )

    source_suffix = "src/gamezrecoil/zsound/zsnd_play.cpp"
    e3_source = PureWindowsPath(e3.source_provenance).as_posix().casefold()
    vector_source = PureWindowsPath(
        vector_ctor.source_provenance
    ).as_posix().casefold()
    candidate_e3_body = bytes.fromhex(
        "51 8d 44 24 03 b9 00 00 00 00 50 e8 00 00 00 00 "
        "59 c3 90 90 90 90 90 90 90 90 90 90 90 90 90 90"
    )
    candidate_vector_body = bytes.fromhex(
        "8b c1 8b 4c 24 04 8a 11 33 c9 88 10 89 48 04 89 "
        "48 08 89 48 0c c2 04 00 90 90 90 90 90 90 90 90"
    )
    if (
        e3_source != vector_source
        or not e3_source.endswith(source_suffix)
        or caller.data != candidate_e3_body
        or caller.relocations != e3.relocations
        or caller.relocation_mask != e3.relocation_mask
        or e3.data != candidate_e3_body
        or e3.section_size != 0x20
        or e3.section_function_symbols != (_cc_catalog._ZSND_STATIC_VECTOR_CTOR_COFF,)
        or not e3.section_is_comdat
        or e3.comdat_selection != 1
        or tuple(
            (row.offset, row.symbol_index, row.type, row.symbol_name)
            for row in e3.relocations
        )
        != (
            (6, registry_symbol.index, IMAGE_REL_I386_DIR32,
             "_g_zSnd_SampleSetRegistry"),
            (12, vector_symbol.index, IMAGE_REL_I386_REL32,
             _cc_catalog._ZSND_VECTOR_CONSTRUCTOR_COFF),
        )
        or e3.relocation_mask
        != tuple(
            index in set(range(6, 10)) | set(range(12, 16))
            for index in range(32)
        )
        or vector_ctor.data != candidate_vector_body
        or vector_ctor.section_size != 0x20
        or vector_ctor.section_external_functions
        != (_cc_catalog._ZSND_VECTOR_CONSTRUCTOR_COFF,)
        or not vector_ctor.section_is_comdat
        or vector_ctor.comdat_selection != 2
        or vector_ctor.relocations
        or any(vector_ctor.relocation_mask)
        or vector_ctor.local_control_flow_indices
        or vector_ctor.local_control_flow_targets
        or len(vector_ctor.instructions) != 9
        or _cc_cfg._instruction_mnemonic(vector_ctor.instructions[-1])
        not in {"ret", "retn"}
        or _cc_cfg._instruction_operand(vector_ctor.instructions[-1]) != "4"
    ):
        raise ValueError(
            "zSnd static E3 inline projection rejects exact E3/vector body, "
            "relocation, provenance, COMDAT, or ABI drift"
        )

    retail_body = bytes.fromhex(
        "51 8a 44 24 03 a2 90 b2 56 00 33 c0 a3 94 b2 56 "
        "00 a3 98 b2 56 00 a3 9c b2 56 00 59 c3 90 90 90"
    )
    retail_rows: list[tuple[int, bytes]] = []
    for instruction in retail_instructions:
        address = _cc_cfg._source_instruction_address(instruction)
        if address is None:
            continue
        runtime = address_value(address)
        if 0x4A0810 <= runtime < 0x4A0830:
            try:
                body = bytes(int(item, 16) for item in instruction.bytes)
            except (TypeError, ValueError):
                body = b""
            retail_rows.append((runtime, body))
    retail_rows.sort()
    cursor = 0x4A0810
    recovered = bytearray()
    for address, body in retail_rows:
        if address != cursor or not body:
            raise ValueError(
                "zSnd static E3 inline projection rejects incomplete retail bytes"
            )
        recovered.extend(body)
        cursor += len(body)
    if (
        cursor != 0x4A082D
        or bytes(recovered) != retail_body[:0x1D]
        or _cc_cfg._hexdump_bytes(bridge.hexdump("0x4a0810", 0x20))
        != retail_body
    ):
        raise ValueError(
            "zSnd static E3 inline projection rejects immutable retail inline drift"
        )
    return []


def _zsnd_static_vector_dtor_candidate_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    retail_instructions: Sequence[Instruction],
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge: BinaryNinjaBridge,
) -> dict[str, str]:
    """Bind only zSnd E4's exact vector-dtor expansion to retail delete."""

    if normalize_address(caller_start) != "0x4a0840":
        return {}
    if (
        caller_identity != "symbol:recoil:function:0x4a0840"
        or normalize_address(caller_end_exclusive) != "0x4a0860"
        or indexes.by_address.get("0x4a0840") != caller_identity
        or len(expected) != 1
    ):
        raise ValueError(
            "zSnd E4 vector-dtor bridge requires the exact registered caller and population"
        )
    expected_row = expected[0]
    expected_identity = str(expected_row.get("target_identity", ""))
    if (
        expected_row.get("ordinal") != 0
        or expected_row.get("form") != "call"
        or expected_row.get("dispatch") != "direct"
        or expected_row.get("identity_kind") != "provider"
        or not expected_identity
        or expected_identity != indexes.by_address.get("0x4c5b6a")
        or expected_identity not in indexes.provider_ids
        or expected_row.get("storage_identity") != ""
        or expected_row.get("slot_displacement") is not None
        or expected_row.get("cleanup_bytes") != 4
    ):
        raise ValueError(
            "zSnd E4 vector-dtor bridge rejects retail provider/form/cleanup drift"
        )

    caller = candidate.caller_definition
    e4 = candidate.compiler_local_function_definitions.get("_$E4")
    destructor = candidate.complete_destructor_definitions.get(
        _cc_catalog._ZSND_VECTOR_DESTRUCTOR_COFF
    )
    destructor_source = candidate.tu_local_function_definitions.get(
        _cc_catalog._ZSND_VECTOR_DESTRUCTOR_COFF
    )
    coff_symbols = caller.coff_symbols if caller is not None else ()
    symbols_by_name = {
        name: [row for row in coff_symbols if row.name == name]
        for name in (
            "_$E4",
            "_g_zSnd_SampleSetRegistry",
            _cc_catalog._ZSND_VECTOR_DESTRUCTOR_COFF,
            _cc_catalog._ZSND_VECTOR_DESTROY_COFF,
            _cc_catalog._ZSND_VECTOR_DEALLOCATE_COFF,
        )
    }
    if (
        caller is None
        or caller.symbol != "_$E4"
        or e4 is None
        or destructor is None
        or destructor_source is None
        or any(len(rows) != 1 for rows in symbols_by_name.values())
    ):
        raise ValueError(
            "zSnd E4 vector-dtor bridge rejects missing, duplicate, or aliased COFF definitions"
        )
    e4_symbol = symbols_by_name["_$E4"][0]
    registry_symbol = symbols_by_name["_g_zSnd_SampleSetRegistry"][0]
    destructor_symbol = symbols_by_name[_cc_catalog._ZSND_VECTOR_DESTRUCTOR_COFF][0]
    destroy_symbol = symbols_by_name[_cc_catalog._ZSND_VECTOR_DESTROY_COFF][0]
    deallocate_symbol = symbols_by_name[_cc_catalog._ZSND_VECTOR_DEALLOCATE_COFF][0]
    if (
        e4_symbol.value != 0
        or e4_symbol.section_number <= 0
        or e4_symbol.symbol_type != 0x20
        or e4_symbol.storage_class != IMAGE_SYM_CLASS_STATIC
        or e4_symbol.aux_count != 0
        or e4_symbol.section_name != ".text"
        or e4_symbol.section_size != 0x10
        or e4_symbol.natural_end != 0x10
        or e4_symbol.section_characteristics & _cc_catalog.IMAGE_SCN_LNK_COMDAT == 0
        or destructor_symbol.value != 0
        or destructor_symbol.section_number <= 0
        or destructor_symbol.section_number == e4_symbol.section_number
        or destructor_symbol.symbol_type != 0x20
        or destructor_symbol.storage_class != IMAGE_SYM_CLASS_EXTERNAL
        or destructor_symbol.aux_count != 0
        or destructor_symbol.weak_external_tag_index is not None
        or destructor_symbol.weak_external_characteristics is not None
        or destructor_symbol.section_name != ".text"
        or destructor_symbol.section_size != 0x40
        or destructor_symbol.natural_end != 0x40
        or destructor_symbol.section_characteristics & _cc_catalog.IMAGE_SCN_LNK_COMDAT == 0
    ):
        raise ValueError(
            "zSnd E4 vector-dtor bridge rejects COFF storage, extent, section, or alias drift"
        )

    source_suffix = "src/gamezrecoil/zsound/zsnd_play.cpp"
    e4_source = PureWindowsPath(e4.source_provenance).as_posix().casefold()
    dtor_source = PureWindowsPath(
        destructor_source.source_provenance
    ).as_posix().casefold()
    e4_body = bytes.fromhex(
        "b9 00 00 00 00 e9 00 00 00 00 90 90 90 90 90 90"
    )
    dtor_body = bytes.fromhex(
        "56 8b f1 8b 46 08 8b 4e 04 50 51 8b ce e8 00 00 "
        "00 00 8b 46 04 8b 56 0c 2b d0 8b ce c1 fa 02 52 "
        "50 e8 00 00 00 00 33 c0 89 46 04 89 46 08 89 46 "
        "0c 5e c3 90 90 90 90 90 90 90 90 90 90 90 90 90"
    )
    if (
        e4_source != dtor_source
        or not e4_source.endswith(source_suffix)
        or candidate.local_control_flow_indices
        or candidate.local_control_flow_targets
        or caller.data != e4_body
        or caller.relocations != e4.relocations
        or caller.relocation_mask != e4.relocation_mask
        or e4.data != e4_body
        or e4.section_size != 0x10
        or e4.section_function_symbols != ("_$E4",)
        or not e4.section_is_comdat
        or e4.comdat_selection != 1
        or tuple(
            (row.offset, row.symbol_index, row.type, row.symbol_name)
            for row in e4.relocations
        )
        != (
            (1, registry_symbol.index, IMAGE_REL_I386_DIR32,
             "_g_zSnd_SampleSetRegistry"),
            (6, destructor_symbol.index, IMAGE_REL_I386_REL32,
             _cc_catalog._ZSND_VECTOR_DESTRUCTOR_COFF),
        )
        or e4.relocation_mask
        != tuple(
            index in set(range(1, 5)) | set(range(6, 10))
            for index in range(16)
        )
        or len(candidate.instructions) != 2
        or _cc_cfg._instruction_mnemonic(candidate.instructions[0]) != "mov"
        or tuple(candidate.instructions[0].bytes)
        != ("b9", "00", "00", "00", "00")
        or _cc_cfg._instruction_mnemonic(candidate.instructions[1]) != "jmp"
        or _cc_cfg._instruction_operand(candidate.instructions[1])
        != _cc_catalog._ZSND_VECTOR_DESTRUCTOR_COFF
        or tuple(candidate.instructions[1].bytes)
        != ("e9", "00", "00", "00", "00")
        or destructor.data != dtor_body
        or destructor_source.data != dtor_body
        or destructor.relocations != destructor_source.relocations
        or destructor.relocation_mask != destructor_source.relocation_mask
        or destructor.section_size != 0x40
        or destructor.section_external_functions
        != (_cc_catalog._ZSND_VECTOR_DESTRUCTOR_COFF,)
        or not destructor.section_is_comdat
        or destructor.comdat_selection != 2
        or destructor.local_control_flow_indices
        or destructor.local_control_flow_targets
        or tuple(
            (row.offset, row.symbol_index, row.type, row.symbol_name)
            for row in destructor.relocations
        )
        != (
            (14, destroy_symbol.index, IMAGE_REL_I386_REL32,
             _cc_catalog._ZSND_VECTOR_DESTROY_COFF),
            (34, deallocate_symbol.index, IMAGE_REL_I386_REL32,
             _cc_catalog._ZSND_VECTOR_DEALLOCATE_COFF),
        )
        or destructor.relocation_mask
        != tuple(
            index in set(range(14, 18)) | set(range(34, 38))
            for index in range(64)
        )
        or len(destructor.instructions) != 22
        or _cc_cfg._instruction_mnemonic(destructor.instructions[-1])
        not in {"ret", "retn"}
        or _cc_cfg._instruction_operand(destructor.instructions[-1]) not in {"", "0"}
    ):
        raise ValueError(
            "zSnd E4 vector-dtor bridge rejects exact body, relocation, provenance, COMDAT, or ABI drift"
        )

    retail_body = bytes.fromhex(
        "a1 94 b2 56 00 50 e8 1f 53 02 00 33 c0 83 c4 04 "
        "a3 94 b2 56 00 a3 98 b2 56 00 a3 9c b2 56 00 c3"
    )
    if _cc_cfg._hexdump_bytes(bridge.hexdump("0x4a0840", 0x20)) != retail_body:
        raise ValueError(
            "zSnd E4 vector-dtor bridge rejects immutable retail body drift"
        )
    retail_addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions, source="bn", caller_start=0x4A0840
    )
    recovered = bytearray()
    cursor = 0x4A0840
    for instruction, address in zip(retail_instructions, retail_addresses):
        if address is None or not 0x4A0840 <= address < 0x4A0860:
            continue
        body = bytes(int(item, 16) for item in instruction.bytes)
        if address != cursor or not body:
            raise ValueError(
                "zSnd E4 vector-dtor bridge rejects retail instruction population drift"
            )
        recovered.extend(body)
        cursor += len(body)
    if cursor != 0x4A0860 or bytes(recovered) != retail_body:
        raise ValueError(
            "zSnd E4 vector-dtor bridge rejects retail instruction/extent drift"
        )
    return {_cc_catalog._ZSND_VECTOR_DESTRUCTOR_COFF: expected_identity}


def _zsnd_static_vector_dtor_candidate_projection(
    expected: Sequence[Mapping[str, Any]],
    candidate_contract: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    retail_instructions: Sequence[Instruction],
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge: BinaryNinjaBridge,
) -> list[dict[str, Any]]:
    result = [dict(row) for row in candidate_contract]
    if normalize_address(caller_start) != "0x4a0840":
        return result
    exact_bridge = _zsnd_static_vector_dtor_candidate_bridge(
        expected,
        candidate,
        retail_instructions,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge=bridge,
    )
    expected_identity = exact_bridge.get(_cc_catalog._ZSND_VECTOR_DESTRUCTOR_COFF, "")
    if (
        len(expected) != 1
        or len(result) != 1
        or result[0]
        != {
            "ordinal": 0,
            "form": "tail",
            "dispatch": "direct",
            "identity_kind": "provider",
            "target_identity": expected_identity,
            "storage_identity": "",
            "slot_displacement": None,
            "cleanup_bytes": None,
        }
    ):
        raise ValueError(
            "zSnd E4 vector-dtor projection requires exactly one proved candidate tail"
        )
    return [dict(expected[0])]
