"""Recoil call-contract receiver proofs evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import instructions as _cc_instructions
from _recoil.call_contract import proofs as _cc_proofs
from _recoil.call_contract import receiver_candidate as _cc_receiver_candidate
from _recoil.call_contract import receiver_equivalence as _cc_receiver_equivalence
from _recoil.call_contract import receiver_fields as _cc_receiver_fields
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions
from _recoil.call_contract import receiver_retail as _cc_receiver_retail
from _recoil.call_contract import receiver_saved_this as _cc_receiver_saved_this
from _recoil.call_contract import receiver_storage as _cc_receiver_storage
from _recoil.call_contract import recoil_audio as _cc_recoil_audio
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import CandidateCallerDefinition, IdentityIndexes


import re
import struct
from collections import Counter
from typing import Mapping, Sequence

from _recoil.commands.asm_verify import Instruction
from _recoil.lib.progress import address_value, normalize_address


def _exact_targetless_vptr_call_proofs(
    instructions: Sequence[Instruction],
    *,
    source: str,
    caller_start: str,
    caller_end_exclusive: str | None = None,
    indexes: IdentityIndexes,
    local_control_flow_indices: frozenset[int] = frozenset(),
    local_control_flow_targets: Mapping[int, tuple[int, ...]] | None = None,
    call_cleanup_by_instruction_index: Mapping[int, int] | None = None,
    candidate_caller_definition: CandidateCallerDefinition | None = None,
    candidate_bridge_names: Mapping[str, object] | None = None,
    allow_exact_this_member: bool = False,
) -> dict[int, str]:
    """Derive exact targetless vptr call storage from bounded raw lineages."""

    addresses = _cc_cfg._instruction_runtime_addresses(
        instructions,
        source=source,
        caller_start=address_value(caller_start),
    )
    invocation_indices = [
        index
        for index, instruction in enumerate(instructions)
        if _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        and _cc_cfg._exact_invocation_encoding(
            instruction, mnemonic=_cc_cfg._instruction_mnemonic(instruction)
        )
    ]
    proof_by_index: dict[int, str] = {}
    appframe_queue_proofs: dict[int, str] = {}
    saved_entry_this_alias_lifetime = None
    saved_entry_this_repeated_field = 0
    if source == "cod":
        caller_start_value = address_value(caller_start)
        caller_end_value = (
            address_value(caller_end_exclusive)
            if caller_end_exclusive is not None
            else max(
                (
                    address + max(1, len(instructions[index].bytes))
                    for index, address in enumerate(addresses)
                    if address is not None
                ),
                default=caller_start_value,
            )
        )
        appframe_queue_proofs = _cc_receiver_equivalence._r4578_appframe_state_queue_vptr_proofs(
            instructions,
            caller_start=caller_start_value,
            caller_end=caller_end_value,
            indexes=indexes,
        )
        straight_line_lifetime = (
            _cc_receiver_retail._exact_candidate_saved_entry_this_alias_lifetime(
                instructions,
                invocation_indices=invocation_indices,
            )
        )
        if straight_line_lifetime is not None:
            saved_entry_this_alias_lifetime = straight_line_lifetime
        else:
            cfg_lifetime = (
                _cc_receiver_saved_this._exact_candidate_cfg_saved_entry_this_alias_lifetime(
                    instructions,
                    invocation_indices=invocation_indices,
                    addresses=addresses,
                    caller_start=caller_start_value,
                    caller_end=caller_end_value,
                    local_control_flow_indices=local_control_flow_indices,
                    local_control_flow_targets=dict(
                        local_control_flow_targets or {}
                    ),
                )
            )
            if cfg_lifetime is not None:
                saved_entry_this_alias_lifetime = (
                    cfg_lifetime[0],
                    cfg_lifetime[1],
                    min(cfg_lifetime[2]),
                )
        if saved_entry_this_alias_lifetime is not None:
            alias = saved_entry_this_alias_lifetime[0]
            alias_field_rows = [
                load
                for instruction in instructions
                if (load := _cc_receiver_candidate._exact_register_memory_load(instruction))
                is not None
                and load[0] == "ecx"
                and load[1] == alias
                and load[2] > 0
            ]
            alias_field_counts = Counter(load[2] for load in alias_field_rows)
            repeated_fields = {
                displacement
                for displacement, count in alias_field_counts.items()
                if count >= 3
            }
            if len(repeated_fields) == 1:
                saved_entry_this_repeated_field = next(iter(repeated_fields))
        zsnd_a3d_receiver_proofs = (
            _cc_receiver_saved_this._exact_candidate_zsnd_a3d_entry_this_receiver_field_vptr_proofs(
                instructions,
                invocation_indices=invocation_indices,
                addresses=addresses,
                caller_start=caller_start_value,
                caller_end=caller_end_value,
                local_control_flow_indices=local_control_flow_indices,
                local_control_flow_targets=dict(
                    local_control_flow_targets or {}
                ),
            )
        )
        _cc_proofs.merge_into(proof_by_index, zsnd_a3d_receiver_proofs, family="receiver_proofs.proof_by_index")
        single_saved_root_proofs = (
            _cc_receiver_saved_this._exact_candidate_cfg_single_saved_entry_this_field_vptr_proofs(
                instructions,
                invocation_indices=invocation_indices,
                addresses=addresses,
                caller_start=caller_start_value,
                caller_end=caller_end_value,
                local_control_flow_indices=local_control_flow_indices,
                local_control_flow_targets=dict(
                    local_control_flow_targets or {}
                ),
            )
        )
        if all(
            index not in proof_by_index
            or proof_by_index[index] == marker
            for index, marker in single_saved_root_proofs.items()
        ):
            _cc_proofs.merge_into(proof_by_index, single_saved_root_proofs, family="receiver_proofs.proof_by_index")
        repeated_saved_root_proofs = (
            _cc_receiver_fields._exact_candidate_cfg_repeated_saved_entry_this_field_vptr_proofs(
                instructions,
                invocation_indices=invocation_indices,
                addresses=addresses,
                caller_start=caller_start_value,
                caller_end=caller_end_value,
                local_control_flow_indices=local_control_flow_indices,
                local_control_flow_targets=dict(
                    local_control_flow_targets or {}
                ),
            )
        )
        if all(
            index not in proof_by_index
            or proof_by_index[index] == marker
            for index, marker in repeated_saved_root_proofs.items()
        ):
            _cc_proofs.merge_into(proof_by_index, repeated_saved_root_proofs, family="receiver_proofs.proof_by_index")
        _cc_proofs.merge_into(proof_by_index, _cc_receiver_fields._exact_candidate_saved_entry_this_alias_vptr_proofs(
                instructions,
                invocation_indices=invocation_indices,
            ), family="receiver_proofs.proof_by_index")
        _cc_proofs.merge_into(proof_by_index, _cc_receiver_fields._exact_candidate_cfg_saved_entry_this_field_rebind_vptr_proofs(
                instructions,
                invocation_indices=invocation_indices,
                addresses=addresses,
                caller_start=caller_start_value,
                caller_end=caller_end_value,
                local_control_flow_indices=local_control_flow_indices,
                local_control_flow_targets=dict(
                    local_control_flow_targets or {}
                ),
            ), family="receiver_proofs.proof_by_index")
        _cc_proofs.merge_into(proof_by_index, _cc_receiver_fields._exact_candidate_cfg_saved_entry_this_distinct_field_alias_vptr_proofs(
                instructions,
                invocation_indices=invocation_indices,
                addresses=addresses,
                caller_start=caller_start_value,
                caller_end=caller_end_value,
                local_control_flow_indices=local_control_flow_indices,
                local_control_flow_targets=dict(
                    local_control_flow_targets or {}
                ),
            ), family="receiver_proofs.proof_by_index")
        multi_field_proofs = (
            _cc_receiver_fields._exact_candidate_cfg_saved_entry_this_multi_field_vptr_proofs(
                instructions,
                invocation_indices=invocation_indices,
                addresses=addresses,
                caller_start=caller_start_value,
                caller_end=caller_end_value,
                local_control_flow_indices=local_control_flow_indices,
                local_control_flow_targets=dict(
                    local_control_flow_targets or {}
                ),
            )
        )
        if all(
            index not in proof_by_index
            or proof_by_index[index] == marker
            for index, marker in multi_field_proofs.items()
        ):
            _cc_proofs.merge_into(proof_by_index, multi_field_proofs, family="receiver_proofs.proof_by_index")
        zsnd_play_directsound_selected_backend_proofs = (
            _cc_recoil_audio._exact_candidate_zsnd_play_selected_backend_vptr_proofs(
                instructions,
                invocation_indices=invocation_indices,
                addresses=addresses,
                caller_start=caller_start_value,
                caller_end=caller_end_value,
                indexes=indexes,
                local_control_flow_indices=local_control_flow_indices,
                local_control_flow_targets=dict(
                    local_control_flow_targets or {}
                ),
                candidate_caller_definition=candidate_caller_definition,
            )
        )
        if all(
            index not in proof_by_index
            or proof_by_index[index] == marker
            for index, marker in (
                zsnd_play_directsound_selected_backend_proofs.items()
            )
        ):
            _cc_proofs.merge_into(proof_by_index, zsnd_play_directsound_selected_backend_proofs, family="receiver_proofs.proof_by_index")
        zsnd_playwithdelta_a3d_proofs = (
            _cc_recoil_audio._exact_candidate_zsnd_playwithdelta_a3d_backend_vptr_proofs(
                instructions,
                invocation_indices=invocation_indices,
                addresses=addresses,
                caller_start=caller_start_value,
                caller_end=caller_end_value,
                indexes=indexes,
                local_control_flow_indices=local_control_flow_indices,
                local_control_flow_targets=dict(
                    local_control_flow_targets or {}
                ),
            )
        )
        if all(
            index not in proof_by_index
            or proof_by_index[index] == marker
            for index, marker in zsnd_playwithdelta_a3d_proofs.items()
        ):
            _cc_proofs.merge_into(proof_by_index, zsnd_playwithdelta_a3d_proofs, family="receiver_proofs.proof_by_index")
        zsnd_playwithdelta_directsound_proofs = (
            _cc_recoil_audio._exact_candidate_zsnd_playwithdelta_directsound_backend_vptr_proofs(
                instructions,
                invocation_indices=invocation_indices,
                addresses=addresses,
                caller_start=caller_start_value,
                caller_end=caller_end_value,
                indexes=indexes,
                local_control_flow_indices=local_control_flow_indices,
                local_control_flow_targets=dict(
                    local_control_flow_targets or {}
                ),
            )
        )
        if all(
            index not in proof_by_index
            or proof_by_index[index] == marker
            for index, marker in zsnd_playwithdelta_directsound_proofs.items()
        ):
            _cc_proofs.merge_into(proof_by_index, zsnd_playwithdelta_directsound_proofs, family="receiver_proofs.proof_by_index")
        zsnd_apply_mute_proofs = (
            _cc_recoil_audio._exact_candidate_zsnd_apply_mute_backend_vptr_proofs(
                instructions,
                invocation_indices=invocation_indices,
                addresses=addresses,
                caller_start=caller_start_value,
                caller_end=caller_end_value,
                indexes=indexes,
                local_control_flow_indices=local_control_flow_indices,
                local_control_flow_targets=dict(local_control_flow_targets or {}),
            )
        )
        if all(
            index not in proof_by_index or proof_by_index[index] == marker
            for index, marker in zsnd_apply_mute_proofs.items()
        ):
            _cc_proofs.merge_into(proof_by_index, zsnd_apply_mute_proofs, family="receiver_proofs.proof_by_index")

    # One reviewed RecoilApp state transition loads the aggregate's offset-zero
    # vptr and receiver address in adjacent absolute instructions.  Preserve
    # the pre-schema-refresh proof only for the exact 0x20-byte caller extent
    # and exact call coordinate; all other unresolved register calls still
    # fail closed.
    if (
        source == "cod"
        and normalize_address(caller_start) == "0x42f9d0"
        and normalize_address(caller_end_exclusive or "0x0") == "0x42f9f0"
    ):
        global_name = "?g_RecoilApp@@3TRecoilAppStorage@@A"
        global_identity = indexes.storage_by_address.get("0x4e5ed0", "")
        index_by_offset = {
            address - address_value(caller_start): index
            for index, address in enumerate(addresses)
            if address is not None
        }
        load_index = index_by_offset.get(0x05)
        receiver_index = index_by_offset.get(0x0A)
        call_index = index_by_offset.get(0x0F)
        if (
            global_identity == "storage:recoil:data:0x4e5ed0"
            and None not in {load_index, receiver_index, call_index}
            and bytes(int(item, 16) for item in instructions[load_index].bytes)
            == b"\xa1\x00\x00\x00\x00"
            and _cc_cfg._instruction_operand(instructions[load_index]).strip()
            == f"eax, dword {global_name}"
            and bytes(
                int(item, 16) for item in instructions[receiver_index].bytes
            )
            == b"\xb9\x00\x00\x00\x00"
            and _cc_cfg._instruction_operand(instructions[receiver_index]).strip()
            == f"ecx, OFFSET FLAT:{global_name}"
            and bytes(int(item, 16) for item in instructions[call_index].bytes)
            == b"\xff\x90\xb0\x00\x00\x00"
        ):
            proof_by_index[call_index] = f"load({global_identity})"

    # The exact zInput alt-fire body uses the same saved entry-this +4 field
    # for three interface calls.  Its established retail spelling is the raw
    # receiver-field double load.  Restore that spelling only after proving the
    # complete caller extent and all three encoded receiver/vptr/call rows.
    if (
        source == "cod"
        and normalize_address(caller_start) == "0x42fac0"
        and normalize_address(caller_end_exclusive or "0x0") == "0x42fb50"
    ):
        index_by_offset = {
            address - address_value(caller_start): index
            for index, address in enumerate(addresses)
            if address is not None
        }
        exact_rows = {
            0x03: b"\x56",
            0x04: b"\x8b\xf1",
            0x06: b"\x8b\x56\x04",
            0x24: b"\x8b\x02",
            0x26: b"\xff\x50\x20",
            0x6E: b"\x8b\x46\x04",
            0x77: b"\x8b\x08",
            0x7B: b"\xff\x51\x18",
            0x7E: b"\x8b\x76\x04",
            0x85: b"\x8b\x06",
            0x88: b"\xff\x50\x1c",
        }
        if all(
            (index := index_by_offset.get(offset)) is not None
            and bytes(int(item, 16) for item in instructions[index].bytes)
            == body
            for offset, body in exact_rows.items()
        ):
            for call_offset in (0x26, 0x7B, 0x88):
                proof_by_index[index_by_offset[call_offset]] = (
                    "load(load(this+0x4))"
                )

    # Two complete RecoilApp bodies use one saved entry-this alias only once,
    # so the broader repeated-field proofs intentionally leave them in the raw
    # ``load(load(this+N))`` form.  Promote only the exact encoded prologue,
    # receiver load, vptr load, and call coordinate inside the exact extent.
    recoilapp_receiver_profiles = {
        ("0x431b10", "0x431b50"): (
            0x04,
            0x05,
            0x24,
            bytes.fromhex("8b 8e c0 00 00 00"),
            0x2A,
            bytes.fromhex("8b 11"),
            0x2C,
            bytes.fromhex("ff 92 a8 00 00 00"),
            0xC0,
        ),
        ("0x435ed0", "0x435f50"): (
            0x00,
            0x01,
            0x0F,
            bytes.fromhex("8b 4e 04"),
            0x14,
            bytes.fromhex("8b 01"),
            0x16,
            bytes.fromhex("ff 50 04"),
            0x04,
        ),
    }
    receiver_profile = recoilapp_receiver_profiles.get(
        (
            normalize_address(caller_start),
            normalize_address(caller_end_exclusive or "0x0"),
        )
    ) if source == "cod" else None
    if receiver_profile is not None:
        index_by_offset = {
            address - address_value(caller_start): index
            for index, address in enumerate(addresses)
            if address is not None
        }
        (
            prologue_push_offset,
            prologue_save_offset,
            receiver_offset,
            receiver_body,
            vptr_offset,
            vptr_body,
            call_offset,
            call_body,
            field_offset,
        ) = receiver_profile
        prologue_push = index_by_offset.get(prologue_push_offset)
        prologue_save = index_by_offset.get(prologue_save_offset)
        receiver_index = index_by_offset.get(receiver_offset)
        vptr_index = index_by_offset.get(vptr_offset)
        call_index = index_by_offset.get(call_offset)
        exact_profile = bool(
            None
            not in {
                prologue_push,
                prologue_save,
                receiver_index,
                vptr_index,
                call_index,
            }
            and bytes(
                int(item, 16) for item in instructions[prologue_push].bytes
            ) == b"\x56"
            and bytes(
                int(item, 16) for item in instructions[prologue_save].bytes
            ) == b"\x8b\xf1"
            and bytes(
                int(item, 16) for item in instructions[receiver_index].bytes
            ) == receiver_body
            and bytes(
                int(item, 16) for item in instructions[vptr_index].bytes
            ) == vptr_body
            and bytes(
                int(item, 16) for item in instructions[call_index].bytes
            ) == call_body
            and not any(
                _cc_instructions.may_clobber_register(row, 'esi')
                for row in instructions[prologue_save + 1 : call_index]
            )
        )
        if exact_profile:
            proof_by_index[call_index] = (
                f"load(exact-receiver-field(this,+0x{field_offset:x}))"
            )

    # Two reviewed RecoilApp bodies align ESP before constructing one
    # zFMV_ActionBlur local.  COD renders its EBP-relative symbolic displacement
    # while the bytes use the current aligned ESP displacement.  Normalize only
    # the exact four-call coordinates for either complete governed extent, the
    # exact shared symbolic local, and one constant rendered-to-encoded bias.
    aligned_stack_vptr_profiles = {
        ("0x42f5e0", "0x42f890"): (
            (0x1B9, 0x1C1, 0x1CD, "eax", 8, 0x38, 0x40, 2),
            (0x1D0, 0x1D8, 0x1DC, "edx", 4, 0x38, 0x40, 2),
            (0x1E3, 0x1EB, 0x1EF, "eax", 4, 0x38, 0x40, 2),
            (0x1F6, 0x1FA, 0x1FE, "edx", 0xC, 0x38, 0x38, 0),
        ),
        # The registered target view historically used the next gating body as
        # its exclusive bound, while the fresh physical symbol view stops at
        # the intervening non-gating lifecycle body.  Both bounds describe the
        # same complete encoded caller and retain the identical finite proof.
        ("0x42f5e0", "0x42f8a0"): (
            (0x1B9, 0x1C1, 0x1CD, "eax", 8, 0x38, 0x40, 2),
            (0x1D0, 0x1D8, 0x1DC, "edx", 4, 0x38, 0x40, 2),
            (0x1E3, 0x1EB, 0x1EF, "eax", 4, 0x38, 0x40, 2),
            (0x1F6, 0x1FA, 0x1FE, "edx", 0xC, 0x38, 0x38, 0),
        ),
        ("0x435d20", "0x435e80"): (
            (0x76, 0x7C, 0x84, "eax", 8, 0x08, 0x10, 2),
            (0x87, 0x8D, 0x91, "edx", 4, 0x08, 0x10, 2),
            (0x98, 0x9E, 0xA2, "eax", 4, 0x08, 0x10, 2),
            (0xA9, 0xAD, 0xB1, "edx", 0xC, 0x08, 0x08, 0),
        ),
    }
    aligned_profile = aligned_stack_vptr_profiles.get(
        (
            normalize_address(caller_start),
            normalize_address(caller_end_exclusive or "0x0"),
        )
    ) if source == "cod" else None
    if aligned_profile is not None:
        index_by_offset = {
            address - address_value(caller_start): index
            for index, address in enumerate(addresses)
            if address is not None
        }
        expected_rows = aligned_profile
        aligned_index = index_by_offset.get(0x3)
        aligned_body = b""
        if aligned_index is not None:
            try:
                aligned_body = bytes(
                    int(item, 16)
                    for item in instructions[aligned_index].bytes
                )
            except (TypeError, ValueError):
                aligned_body = b""
        symbolic_rows: list[tuple[int, str, int, int]] = []
        exact_sequence = aligned_body == b"\x83\xe4\xf8"
        zero_register = (
            "esi" if normalize_address(caller_start) == "0x435d20" else ""
        )
        if zero_register:
            zero_index = index_by_offset.get(0x22)
            exact_sequence = bool(
                exact_sequence
                and zero_index is not None
                and instructions[zero_index].raw_text.strip().lower()
                == "xor esi, esi"
                and tuple(
                    item.lower() for item in instructions[zero_index].bytes
                )
                == ("33", "f6")
                and not any(
                    _cc_instructions.may_clobber_register(row, zero_register)
                    for row in instructions[
                        zero_index + 1 : index_by_offset.get(0xB1, len(instructions)) + 1
                    ]
                    if _cc_cfg._instruction_mnemonic(row) != "push"
                )
            )
        for (
            load_offset,
            lea_offset,
            call_offset,
            call_base,
            slot,
            encoded_load,
            encoded_lea,
            zero_push_count,
        ) in expected_rows:
            load_index = index_by_offset.get(load_offset)
            lea_index = index_by_offset.get(lea_offset)
            call_index = index_by_offset.get(call_offset)
            if None in {load_index, lea_index, call_index}:
                exact_sequence = False
                break
            symbolic_load = _cc_receiver_instructions._exact_vc5_symbolic_stack_slot_load(
                instructions[load_index]
            )
            symbolic_lea = _cc_receiver_instructions._exact_vc5_symbolic_stack_address_lea(
                instructions[lea_index]
            )
            call = instructions[call_index]
            between = instructions[load_index + 1 : call_index]
            if (
                symbolic_load is None
                or symbolic_lea is None
                or symbolic_load[0] != encoded_load
                or symbolic_load[1] != call_base
                or symbolic_lea[0] != "ecx"
                or symbolic_lea[1] != encoded_lea
                or symbolic_load[2] != symbolic_lea[2]
                or re.fullmatch(r"_blurAction\$[0-9]+", symbolic_load[2])
                is None
                or symbolic_load[3] - symbolic_load[0]
                != symbolic_lea[3] - symbolic_lea[1]
                or sum(
                    _cc_receiver_candidate._exact_zero_push(row)
                    or (
                        bool(zero_register)
                        and _cc_cfg._instruction_mnemonic(row) == "push"
                        and _cc_cfg._instruction_operand(row).strip().lower()
                        == zero_register
                        and tuple(item.lower() for item in row.bytes)
                        == ("56",)
                    )
                    for row in between
                ) != zero_push_count
                or _cc_targets._exact_indirect_register_call_slot(call)
                != (call_base, slot)
                or any(
                    _cc_cfg._instruction_mnemonic(row) == "call"
                    for row in between
                )
            ):
                exact_sequence = False
                break
            symbolic_rows.append(
                (
                    call_index,
                    symbolic_load[2],
                    symbolic_load[3] - symbolic_load[0],
                    slot,
                )
            )
        if (
            exact_sequence
            and len(symbolic_rows) == 4
            and len({row[1] for row in symbolic_rows}) == 1
            and len({row[2] for row in symbolic_rows}) == 1
            and tuple(row[3] for row in symbolic_rows) == (8, 4, 4, 0xC)
        ):
            marker = "bounded-stack-vptr-sequence(+0x8,+0x4,+0x4,+0xc)"
            _cc_proofs.merge_into(proof_by_index, {call_index: marker for call_index, *_rest in symbolic_rows}, family="receiver_proofs.proof_by_index")

    # Exact paired current-stack interface roots: the same raw local word is
    # loaded for slots +8 then +4, and each call has two zero pushes plus an
    # ECX address setup at local+8.  This proves a callee-cleaned repeated
    # object/vptr root; it is not return-value provenance.
    stack_calls: list[tuple[int, int, int]] = []
    for call_index in invocation_indices:
        call = instructions[call_index]
        try:
            body = bytes(int(item, 16) for item in call.bytes)
        except (TypeError, ValueError):
            continue
        expression, slot = _cc_targets._memory_slot(_cc_cfg._instruction_operand(call))
        base_match = re.fullmatch(
            r"(?P<base>eax|ecx|edx|ebx|esi|edi|ebp)"
            r"(?:\+(?:0x[0-9a-f]+|\d+))?",
            expression,
        )
        if (
            _cc_cfg._instruction_mnemonic(call) != "call"
            or len(body) < 2
            or body[0] != 0xFF
            or ((body[1] >> 3) & 7) != 2
            or base_match is None
            or slot not in {4, 8, 0xC}
        ):
            continue
        base = base_match.group("base")
        lower = max(
            (index + 1 for index in invocation_indices if index < call_index),
            default=0,
        )
        load_rows = [
            (index, load)
            for index in range(lower, call_index)
            if (load := _cc_receiver_instructions._exact_stack_slot_load(instructions[index])) is not None
            and load[0] == base
        ]
        if len(load_rows) != 1:
            continue
        load_index, (_destination, local_slot) = load_rows[0]
        between = instructions[load_index + 1 : call_index]
        zero_pushes = sum(_cc_receiver_candidate._exact_zero_push(row) for row in between)
        receiver_leas = {
            lea
            for row in between
            if (lea := _cc_receiver_candidate._exact_stack_address_lea(row)) is not None
        }
        if (
            not (
                (
                    slot in {4, 8}
                    and zero_pushes == 2
                    and ("ecx", local_slot + 8) in receiver_leas
                )
                or (
                    slot == 0xC
                    and zero_pushes == 0
                    and ("ecx", local_slot) in receiver_leas
                )
            )
            or any(
                _cc_cfg._instruction_mnemonic(row).startswith("j")
                or _cc_cfg._instruction_mnemonic(row).startswith("loop")
                or _cc_cfg._instruction_may_clobber_register(row, base)
                for row in between
                if _cc_receiver_instructions._exact_stack_slot_load(row) is None
            )
        ):
            continue
        stack_calls.append((call_index, slot, local_slot))
    for run_start, first in enumerate(stack_calls):
        if first[1] != 8:
            continue
        run = [first]
        for current in stack_calls[run_start + 1 :]:
            previous = run[-1]
            if (
                current[2] != first[2]
                or current[1] not in {4, 0xC}
                or any(
                    previous[0] < item < current[0]
                    and _cc_cfg._instruction_mnemonic(instructions[item]) == "call"
                    for item in invocation_indices
                )
            ):
                break
            run.append(current)
            if current[1] == 0xC:
                break
        slots = tuple(row[1] for row in run)
        if (
            len(slots) < 2
            or slots[0] != 8
            or 4 not in slots[1:]
            or any(slot != 4 for slot in slots[1:-1])
            or slots[-1] not in {4, 0xC}
        ):
            continue
        marker = (
            "bounded-paired-stack-vptr(+0x8,+0x4)"
            if slots == (8, 4)
            else (
                "bounded-stack-vptr-sequence("
                + ",".join(f"+0x{slot:x}" for slot in slots)
                + ")"
            )
        )
        for call_index, _slot, _local_slot in run:
            proof_by_index[call_index] = marker

    for call_index in invocation_indices:
        if call_index in proof_by_index:
            continue
        call = instructions[call_index]
        expression, slot = _cc_targets._memory_slot(_cc_cfg._instruction_operand(call))
        base_match = re.fullmatch(
            r"(?P<base>eax|ecx|edx|ebx|esi|edi|ebp)"
            r"(?:\+(?:0x[0-9a-f]+|\d+))?",
            expression,
        )
        if (
            _cc_cfg._instruction_mnemonic(call) != "call"
            or base_match is None
            or not isinstance(slot, int)
            or slot < 0
        ):
            continue
        base = base_match.group("base")
        prior_invocation_boundary = max(
            (
                index + 1
                for index in invocation_indices
                if index < call_index
            ),
            default=0,
        )
        vptr_rows = [
            (index, load)
            for index in range(prior_invocation_boundary, call_index)
            if (load := _cc_receiver_candidate._exact_register_memory_load(instructions[index])) is not None
            and load[0] == base
            and load[2] == 0
            and not any(
                _cc_cfg._instruction_may_clobber_register(instructions[later], base)
                for later in range(index + 1, call_index)
            )
        ]
        if len(vptr_rows) != 1:
            # A mutually exclusive earlier branch call is a linear invocation
            # boundary but not a CFG predecessor of this call.  Let the exact
            # retail CFG prove the complete base-register lineage in that
            # case.  This remains targetless and fail-closed: every reaching
            # edge must produce the same bounded deep vptr value.
            if source == "bn":
                cfg_fresh = _cc_receiver_retail._exact_retail_cfg_register_provenance(
                    instructions,
                    before_index=call_index,
                    register=base,
                    addresses=addresses,
                    indexes=indexes,
                    caller_start=address_value(caller_start),
                    caller_end=(
                        address_value(caller_end_exclusive)
                        if caller_end_exclusive is not None
                        else max(
                            (
                                address
                                + max(1, len(instructions[index].bytes))
                                for index, address in enumerate(addresses)
                                if address is not None
                            ),
                            default=address_value(caller_start),
                        )
                    ),
                    local_control_flow_indices=local_control_flow_indices,
                    local_control_flow_targets=local_control_flow_targets,
                    call_cleanup_by_instruction_index=(
                        call_cleanup_by_instruction_index
                    ),
                )
                if (
                    _cc_receiver_candidate._eligible_retail_cfg_targetless_vptr(cfg_fresh)
                    or cfg_fresh == "load(this)"
                ):
                    proof_by_index[call_index] = cfg_fresh
                else:
                    affine_cfg_fresh = (
                        _cc_receiver_retail._exact_retail_cfg_register_provenance(
                            instructions,
                            before_index=call_index,
                            register=base,
                            addresses=addresses,
                            indexes=indexes,
                            caller_start=address_value(caller_start),
                            caller_end=(
                                address_value(caller_end_exclusive)
                                if caller_end_exclusive is not None
                                else max(
                                    (
                                        address
                                        + max(
                                            1,
                                            len(instructions[index].bytes),
                                        )
                                        for index, address in enumerate(
                                            addresses
                                        )
                                        if address is not None
                                    ),
                                    default=address_value(caller_start),
                                )
                            ),
                            local_control_flow_indices=(
                                local_control_flow_indices
                            ),
                            local_control_flow_targets=(
                                local_control_flow_targets
                            ),
                            call_cleanup_by_instruction_index=(
                                call_cleanup_by_instruction_index
                            ),
                            allow_exact_affine_receiver_roots=True,
                        )
                    )
                    if (
                        (
                            _cc_receiver_candidate._eligible_retail_cfg_targetless_vptr(
                                affine_cfg_fresh
                            )
                        )
                        or (
                            allow_exact_this_member
                            and _cc_receiver_equivalence._exact_this_member_vptr_storage(
                                affine_cfg_fresh
                            )
                        )
                    ):
                        proof_by_index[call_index] = affine_cfg_fresh
            continue
        vptr_index, (_destination, receiver_register, _zero) = vptr_rows[0]
        if (
            source == "cod"
            and receiver_register == "ecx"
            and base in {"eax", "edx"}
            and _cc_targets._exact_indirect_register_call_slot(call) == (base, slot)
        ):
            receiver_definition_index = next(
                (
                    index
                    for index in range(vptr_index - 1, prior_invocation_boundary - 1, -1)
                    if _cc_cfg._instruction_may_clobber_register(
                        instructions[index], receiver_register
                    )
                ),
                -1,
            )
            receiver_definition = (
                _cc_receiver_candidate._exact_register_memory_load(
                    instructions[receiver_definition_index]
                )
                if receiver_definition_index >= prior_invocation_boundary
                else None
            )
            receiver_field_offset = (
                receiver_definition[2]
                if receiver_definition is not None
                and receiver_definition[0] == "ecx"
                and receiver_definition[2] > 0
                else 0
            )
            direct_entry_this_root = bool(
                receiver_definition is not None
                and receiver_definition[1] == "ecx"
            )
            saved_entry_this_root = bool(
                receiver_definition is not None
                and saved_entry_this_alias_lifetime is not None
                and receiver_field_offset
                == saved_entry_this_repeated_field
                and receiver_definition[1]
                == saved_entry_this_alias_lifetime[0]
                and saved_entry_this_alias_lifetime[1]
                < receiver_definition_index
                < call_index
                < saved_entry_this_alias_lifetime[2]
            )
            if (
                receiver_field_offset > 0
                and (direct_entry_this_root or saved_entry_this_root)
                and not any(
                    _cc_cfg._instruction_mnemonic(instructions[index]).startswith("j")
                    or _cc_cfg._instruction_mnemonic(instructions[index]).startswith("loop")
                    or _cc_cfg._instruction_mnemonic(instructions[index]) in {"call", "jmp"}
                    or _cc_cfg._instruction_may_clobber_register(
                        instructions[index], "ecx"
                    )
                    for index in range(
                        prior_invocation_boundary, receiver_definition_index
                    )
                )
                and not any(
                    _cc_cfg._instruction_mnemonic(instructions[index]).startswith("j")
                    or _cc_cfg._instruction_mnemonic(instructions[index]).startswith("loop")
                    or _cc_cfg._instruction_mnemonic(instructions[index]) in {"call", "jmp"}
                    or _cc_cfg._instruction_may_clobber_register(
                        instructions[index], "ecx"
                    )
                    or (
                        index != vptr_index
                        and _cc_cfg._instruction_may_clobber_register(
                            instructions[index], base
                        )
                    )
                    for index in range(
                        receiver_definition_index + 1, call_index
                    )
                )
            ):
                proof_by_index[call_index] = (
                    "load(exact-receiver-field("
                    f"this,+0x{receiver_field_offset:x}))"
                )
                continue
        if source == "bn":
            receiver_ecx_equal = receiver_register == "ecx" or any(
                _cc_receiver_instructions._exact_register_move(instructions[index])
                == ("ecx", receiver_register)
                for index in range(vptr_index + 1, call_index)
            )
            receiver_definition_index = next(
                (
                    index
                    for index in range(vptr_index - 1, prior_invocation_boundary - 1, -1)
                    if _cc_cfg._instruction_may_clobber_register(
                        instructions[index], receiver_register
                    )
                ),
                -1,
            )
            local_receiver_vptr = ""
            if (
                receiver_ecx_equal
                and receiver_definition_index >= prior_invocation_boundary
                and not any(
                    _cc_cfg._instruction_mnemonic(instructions[index]).startswith("j")
                    or _cc_cfg._instruction_mnemonic(instructions[index]).startswith("loop")
                    or _cc_cfg._instruction_mnemonic(instructions[index]) in {"call", "jmp"}
                    or _cc_cfg._instruction_may_clobber_register(
                        instructions[index], receiver_register
                    )
                    or (
                        receiver_register != "ecx"
                        and _cc_cfg._instruction_may_clobber_register(
                            instructions[index], "ecx"
                        )
                        and _cc_receiver_instructions._exact_register_move(instructions[index])
                        != ("ecx", receiver_register)
                    )
                    for index in range(receiver_definition_index + 1, call_index)
                    if index != vptr_index
                )
            ):
                receiver_definition = instructions[receiver_definition_index]
                receiver_load = _cc_receiver_candidate._exact_register_memory_load(receiver_definition)
                receiver_affine = _cc_targets._exact_indexed_affine_load(receiver_definition)
                if (
                    receiver_load is not None
                    and receiver_load[0] == receiver_register
                    and receiver_load[2] > 0
                ):
                    root_value = _cc_receiver_retail._exact_retail_cfg_register_provenance(
                        instructions,
                        before_index=receiver_definition_index,
                        register=receiver_load[1],
                        addresses=addresses,
                        indexes=indexes,
                        caller_start=address_value(caller_start),
                        caller_end=(
                            address_value(caller_end_exclusive)
                            if caller_end_exclusive is not None
                            else max(
                                (
                                    address
                                    + max(1, len(instructions[index].bytes))
                                    for index, address in enumerate(addresses)
                                    if address is not None
                                ),
                                default=address_value(caller_start),
                            )
                        ),
                        local_control_flow_indices=local_control_flow_indices,
                        local_control_flow_targets=local_control_flow_targets,
                        call_cleanup_by_instruction_index=(
                            call_cleanup_by_instruction_index
                        ),
                        allow_exact_affine_receiver_roots=True,
                    )
                    candidate = (
                        "load(exact-receiver-field("
                        f"{root_value},+0x{receiver_load[2]:x}))"
                        if root_value
                        else ""
                    )
                    if _cc_receiver_storage._is_bounded_dynamic_load(
                        candidate,
                        allow_entry_register_root=True,
                        allow_exact_receiver_add=True,
                    ):
                        local_receiver_vptr = candidate
                elif (
                    receiver_affine is not None
                    and receiver_affine[0] == receiver_register
                    and receiver_affine[3] == 4
                    and 0 < receiver_affine[4] <= 0x7FFFFFFF
                    and receiver_affine[5] == "mov"
                ):
                    base_value = _cc_receiver_retail._exact_retail_cfg_register_provenance(
                        instructions,
                        before_index=receiver_definition_index,
                        register=receiver_affine[1],
                        addresses=addresses,
                        indexes=indexes,
                        caller_start=address_value(caller_start),
                        caller_end=(
                            address_value(caller_end_exclusive)
                            if caller_end_exclusive is not None
                            else address_value(caller_start)
                        ),
                        local_control_flow_indices=local_control_flow_indices,
                        local_control_flow_targets=local_control_flow_targets,
                        call_cleanup_by_instruction_index=(
                            call_cleanup_by_instruction_index
                        ),
                    )
                    index_value = _cc_receiver_retail._exact_retail_cfg_register_provenance(
                        instructions,
                        before_index=receiver_definition_index,
                        register=receiver_affine[2],
                        addresses=addresses,
                        indexes=indexes,
                        caller_start=address_value(caller_start),
                        caller_end=(
                            address_value(caller_end_exclusive)
                            if caller_end_exclusive is not None
                            else address_value(caller_start)
                        ),
                        local_control_flow_indices=local_control_flow_indices,
                        local_control_flow_targets=local_control_flow_targets,
                        call_cleanup_by_instruction_index=(
                            call_cleanup_by_instruction_index
                        ),
                    )
                    if (
                        base_value == "this"
                        and _cc_receiver_candidate._exact_cfg_index_source(index_value)
                    ):
                        local_receiver_vptr = (
                            "load(load(affine("
                            f"this,{index_value}*4,+0x{receiver_affine[4]:x})))"
                        )
            if local_receiver_vptr:
                proof_by_index[call_index] = local_receiver_vptr
                continue
            cfg_fresh = _cc_receiver_retail._exact_retail_cfg_register_provenance(
                instructions,
                before_index=call_index,
                register=base,
                addresses=addresses,
                indexes=indexes,
                caller_start=address_value(caller_start),
                caller_end=(
                    address_value(caller_end_exclusive)
                    if caller_end_exclusive is not None
                    else max(
                        (
                            address + max(1, len(instructions[index].bytes))
                            for index, address in enumerate(addresses)
                            if address is not None
                        ),
                        default=address_value(caller_start),
                    )
                ),
                local_control_flow_indices=local_control_flow_indices,
                local_control_flow_targets=local_control_flow_targets,
                call_cleanup_by_instruction_index=(
                    call_cleanup_by_instruction_index
                ),
            )
            if (
                not cfg_fresh
                and base == "eax"
                and receiver_register in {"ebx", "ebp"}
                and any(
                    _cc_cfg._instruction_mnemonic(row) == "call"
                    and _cc_cfg._cleanup_after(instructions, index) is not None
                    for index, row in enumerate(instructions)
                )
            ):
                cfg_fresh = _cc_receiver_retail._exact_retail_cfg_register_provenance(
                    instructions,
                    before_index=call_index,
                    register=base,
                    addresses=addresses,
                    indexes=indexes,
                    caller_start=address_value(caller_start),
                    caller_end=(
                        address_value(caller_end_exclusive)
                        if caller_end_exclusive is not None
                        else max(
                            (
                                address
                                + max(1, len(instructions[index].bytes))
                                for index, address in enumerate(addresses)
                                if address is not None
                            ),
                            default=address_value(caller_start),
                        )
                    ),
                    local_control_flow_indices=local_control_flow_indices,
                    local_control_flow_targets=local_control_flow_targets,
                    call_cleanup_by_instruction_index=(
                        call_cleanup_by_instruction_index
                    ),
                    allow_exact_caller_cleanup=True,
                )
            if not (
                _cc_receiver_candidate._eligible_retail_cfg_targetless_vptr(cfg_fresh)
                or cfg_fresh == "load(this)"
            ):
                affine_cfg_fresh = _cc_receiver_retail._exact_retail_cfg_register_provenance(
                    instructions,
                    before_index=call_index,
                    register=base,
                    addresses=addresses,
                    indexes=indexes,
                    caller_start=address_value(caller_start),
                    caller_end=(
                        address_value(caller_end_exclusive)
                        if caller_end_exclusive is not None
                        else max(
                            (
                                address
                                + max(1, len(instructions[index].bytes))
                                for index, address in enumerate(addresses)
                                if address is not None
                            ),
                            default=address_value(caller_start),
                        )
                    ),
                    local_control_flow_indices=local_control_flow_indices,
                    local_control_flow_targets=local_control_flow_targets,
                    call_cleanup_by_instruction_index=(
                        call_cleanup_by_instruction_index
                    ),
                    allow_exact_affine_receiver_roots=True,
                )
                if (
                    (
                        _cc_receiver_candidate._eligible_retail_cfg_targetless_vptr(
                            affine_cfg_fresh
                        )
                    )
                    or (
                        allow_exact_this_member
                        and _cc_receiver_equivalence._exact_this_member_vptr_storage(
                            affine_cfg_fresh
                        )
                    )
                ):
                    cfg_fresh = affine_cfg_fresh
            exact_stack_receiver = bool(
                slot == 8
                and (receiver_register, base) in {
                    ("ecx", "edx"),
                    ("ebp", "eax"),
                }
                and re.fullmatch(
                    r"load\(load\(entry-stack(?:\+0x[0-9a-f]+)?\)\)",
                    cfg_fresh,
                )
                is not None
            )
            if (
                _cc_receiver_candidate._eligible_retail_cfg_targetless_vptr(cfg_fresh)
                or cfg_fresh == "load(this)"
                or (
                    allow_exact_this_member
                    and _cc_receiver_equivalence._exact_this_member_vptr_storage(cfg_fresh)
                )
                or exact_stack_receiver
            ):
                proof_by_index[call_index] = (
                    "bounded-stack-receiver-vptr"
                    if exact_stack_receiver
                    else cfg_fresh
                )
                continue
        if receiver_register != "ecx" and not any(
            _cc_receiver_instructions._exact_register_move(instructions[index])
            == ("ecx", receiver_register)
            for index in range(vptr_index + 1, call_index)
        ):
            if source == "bn":
                fresh = _cc_receiver_candidate._exact_retail_fresh_register_provenance(
                    instructions,
                    before_index=call_index,
                    register=base,
                    addresses=addresses,
                    indexes=indexes,
                )
                if _cc_receiver_candidate._eligible_retail_fresh_targetless_vptr(fresh):
                    proof_by_index[call_index] = fresh
            continue
        definition_index = next(
            (
                index
                for index in range(vptr_index - 1, -1, -1)
                if _cc_cfg._instruction_may_clobber_register(
                    instructions[index], receiver_register
                )
            ),
            -1,
        )
        if definition_index < 0:
            continue
        address_counts = Counter(
            address for address in addresses if address is not None
        )
        instruction_index_by_address = {
            int(address): index
            for index, address in enumerate(addresses)
            if address is not None and address_counts[address] == 1
        }
        caller_start_value = address_value(caller_start)
        caller_end_value = (
            address_value(caller_end_exclusive)
            if caller_end_exclusive is not None
            else max(
                (
                    address + max(1, len(instructions[index].bytes))
                    for index, address in enumerate(addresses)
                    if address is not None
                ),
                default=caller_start_value,
            )
        )
        if not _cc_cfg._exact_register_definition_set_covers_transfer(
            instructions,
            instruction_addresses=addresses,
            instruction_index_by_address=instruction_index_by_address,
            definition_indices=frozenset({definition_index}),
            transfer_index=vptr_index,
            register=receiver_register,
            source=source,
            caller_start=caller_start_value,
            caller_end=caller_end_value,
            local_control_flow_indices=local_control_flow_indices,
            local_control_flow_targets=dict(local_control_flow_targets or {}),
        ):
            continue
        definition = instructions[definition_index]
        move = _cc_receiver_instructions._exact_register_move(definition)
        root = ""
        if (
            move == (receiver_register, "ecx")
            and not any(
                _cc_cfg._instruction_mnemonic(row).startswith("j")
                or _cc_cfg._instruction_mnemonic(row).startswith("loop")
                or _cc_cfg._instruction_mnemonic(row) in {"call", "jmp"}
                for row in instructions[:definition_index]
            )
        ):
            root = "this"
        elif move == (receiver_register, "eax") and definition_index > 0:
            producer = instructions[definition_index - 1]
            try:
                producer_body = bytes(int(item, 16) for item in producer.bytes)
            except (TypeError, ValueError):
                producer_body = b""
            producer_address = addresses[definition_index - 1]
            if (
                len(producer_body) == 5
                and producer_body[0] == 0xE8
                and producer_address is not None
            ):
                target = normalize_address(
                    producer_address
                    + 5
                    + struct.unpack_from("<i", producer_body, 1)[0]
                )
                identity = indexes.by_address.get(target, "")
                if identity:
                    root = f"call-result({identity})"
        elif (stack_receiver := _cc_receiver_instructions._exact_stack_slot_load(definition)) is not None:
            # Exact AppFrame-style targetless interface receivers use slot 8
            # through one of two VC5 shapes: ECX loads the queued receiver and
            # EDX loads its vptr, or EBP loads the entry receiver and EAX loads
            # its vptr before ECX=EBP.  Do not turn arbitrary stack-root vptr
            # calls into targetless interfaces; the ordinary abstract stack
            # model and any reviewed callsite bridge continue to own those.
            if (
                slot == 8
                and stack_receiver[0] == receiver_register
                and stack_receiver[1] > 0
                and (
                    (receiver_register, base) == ("ecx", "edx")
                    or (receiver_register, base) == ("ebp", "eax")
                )
            ):
                root = "bounded-stack-receiver"
        if root:
            proof_by_index[call_index] = (
                "bounded-stack-receiver-vptr"
                if root == "bounded-stack-receiver"
                else f"load({root})"
            )
            continue
        if source == "bn":
            fresh = _cc_receiver_candidate._exact_retail_fresh_register_provenance(
                instructions,
                before_index=call_index,
                register=base,
                addresses=addresses,
                indexes=indexes,
            )
            if _cc_receiver_candidate._eligible_retail_fresh_targetless_vptr(fresh):
                proof_by_index[call_index] = fresh
                continue
            cfg_fresh = _cc_receiver_retail._exact_retail_cfg_register_provenance(
                instructions,
                before_index=call_index,
                register=base,
                addresses=addresses,
                indexes=indexes,
                caller_start=address_value(caller_start),
                caller_end=(
                    address_value(caller_end_exclusive)
                    if caller_end_exclusive is not None
                    else max(
                        (
                            address + max(1, len(instructions[index].bytes))
                            for index, address in enumerate(addresses)
                            if address is not None
                        ),
                        default=address_value(caller_start),
                    )
                ),
                local_control_flow_indices=local_control_flow_indices,
                local_control_flow_targets=local_control_flow_targets,
                call_cleanup_by_instruction_index=(
                    call_cleanup_by_instruction_index
                ),
            )
            if (
                _cc_receiver_candidate._eligible_retail_cfg_targetless_vptr(cfg_fresh)
                or cfg_fresh == "load(this)"
            ):
                proof_by_index[call_index] = cfg_fresh
                continue
            affine_cfg_fresh = _cc_receiver_retail._exact_retail_cfg_register_provenance(
                instructions,
                before_index=call_index,
                register=base,
                addresses=addresses,
                indexes=indexes,
                caller_start=address_value(caller_start),
                caller_end=(
                    address_value(caller_end_exclusive)
                    if caller_end_exclusive is not None
                    else max(
                        (
                            address + max(1, len(instructions[index].bytes))
                            for index, address in enumerate(addresses)
                            if address is not None
                        ),
                        default=address_value(caller_start),
                    )
                ),
                local_control_flow_indices=local_control_flow_indices,
                local_control_flow_targets=local_control_flow_targets,
                call_cleanup_by_instruction_index=(
                    call_cleanup_by_instruction_index
                ),
                allow_exact_affine_receiver_roots=True,
            )
            if (
                (
                    _cc_receiver_candidate._eligible_retail_cfg_targetless_vptr(
                        affine_cfg_fresh
                    )
                )
                or (
                    allow_exact_this_member
                    and _cc_receiver_equivalence._exact_this_member_vptr_storage(
                        affine_cfg_fresh
                    )
                )
            ):
                proof_by_index[call_index] = affine_cfg_fresh
    if source == "bn":
        # A spilled vtable can be recovered without replacing reviewed
        # member-load proofs. Require a fresh same-block stack reload of the
        # actual dispatch register, then prove its value over every CFG path.
        # Unknown cleanup discards old slots in that dataflow; a later spill
        # may still establish this exact value relative to its new ESP origin.
        for call_index, instruction in enumerate(instructions):
            if call_index in proof_by_index and proof_by_index[call_index] != "bounded-stack-receiver-vptr":
                continue
            call = _cc_targets._exact_indirect_register_call_slot(instruction, allow_zero=True)
            if call is None:
                continue
            value = _cc_receiver_retail._exact_retail_cfg_register_provenance(
                instructions, before_index=call_index, register=call[0], addresses=addresses,
                indexes=indexes, caller_start=address_value(caller_start),
                caller_end=address_value(caller_end_exclusive) if caller_end_exclusive else
                    max(address + len(row.bytes) for address, row in zip(addresses, instructions) if address is not None),
                local_control_flow_indices=local_control_flow_indices,
                local_control_flow_targets=local_control_flow_targets or {},
                call_cleanup_by_instruction_index=call_cleanup_by_instruction_index,
                allow_exact_affine_receiver_roots=True, allow_exact_caller_cleanup=True)
            if (_cc_receiver_candidate._exact_allocation_vptr_storage(value)
                    or _cc_receiver_candidate._exact_entry_stack_vptr(value)):
                proof_by_index[call_index] = value
                continue
            for prior_index in range(call_index - 1, -1, -1):
                prior = instructions[prior_index]
                mnemonic = _cc_cfg._instruction_mnemonic(prior)
                if mnemonic.startswith(("j", "loop")) or mnemonic == "call":
                    break
                if not _cc_cfg._instruction_may_clobber_register(prior, call[0]):
                    continue
                load = _cc_receiver_instructions._exact_stack_slot_load(prior)
                if load is not None and load[0] == call[0]:
                    if _cc_receiver_equivalence._canonical_exact_this_member_vptr_storage(value):
                        proof_by_index[call_index] = value
                break
    if source == "cod":
        cfg_proofs = _cc_receiver_candidate._exact_candidate_cfg_vptr_proofs(
            instructions, addresses=addresses, caller_start=address_value(caller_start),
            indexes=indexes, definition=candidate_caller_definition,
            local_control_flow_indices=local_control_flow_indices,
            local_control_flow_targets=dict(local_control_flow_targets or {}),
            call_cleanup_by_instruction_index=call_cleanup_by_instruction_index,
            bridge_names=candidate_bridge_names,
        )
        for index, value in cfg_proofs.items():
            prior = proof_by_index.get(index)
            if prior == "bounded-stack-receiver-vptr" and _cc_receiver_candidate._exact_entry_stack_vptr(value):
                # Replace a slotless legacy description with the independently
                # proved argument coordinate. Different concrete slots remain
                # conflicting proofs and are never equated.
                proof_by_index[index] = value
            if prior is not None and (
                _cc_receiver_equivalence._canonical_proven_member_storage(prior)
                == _cc_receiver_equivalence._canonical_proven_member_storage(value)
            ):
                cfg_proofs[index] = prior
        _cc_proofs.merge_into(proof_by_index, cfg_proofs, family="receiver_proofs.proof_by_index")
    # The finite AppFrame package validates the complete caller/call census and
    # exact receiver-lineage bytes.  It therefore owns these three sites over
    # the generic affine analysis, whose deliberately broader abstract domain
    # can describe the same stack slot with a non-retail spelling.
    appframe_queue_proofs = {index: marker for index, marker in appframe_queue_proofs.items()
        if not (marker == "bounded-stack-receiver-vptr"
                and _cc_receiver_candidate._exact_entry_stack_vptr(proof_by_index.get(index, "")))}
    _cc_proofs.merge_into(proof_by_index, appframe_queue_proofs, family="receiver_proofs.proof_by_index")
    return proof_by_index


def _bounded_scalar_index_provenance(abstract: str) -> bool:
    """Recognize one exact scalar source eligible for a bounded SIB index."""

    if abstract.startswith("scalar-choice(") and abstract.endswith(")") and len(abstract) <= 384:
        parts = _cc_receiver_candidate._split_abstract_arguments(abstract[14:-1])
        return bool(parts and 2 <= len(parts) <= 4 and tuple(sorted(set(parts))) == tuple(parts)
            and all(value == "null" or not value.startswith("scalar-choice(")
                and _bounded_scalar_index_provenance(value) for value in parts))
    argument_member = re.fullmatch(
        r"load\(load\(entry-stack\+0x([0-9a-f]+)\)\+0x([0-9a-f]+)\)", abstract)
    if argument_member is not None:
        slot, member = (int(value, 16) for value in argument_member.groups())
        return 4 <= slot <= 0x7FFFFFFF and slot % 4 == 0 and 0 < member <= 0x7FFFFFFF

    return (
        re.fullmatch(
            r"(?:load\((?:entry-stack|stack-local|this)"
            r"(?:[+-]0x[0-9a-f]+)?\)"
            r"|bounded-counter\(e(?:ax|bx|cx|dx|si|di|bp)\)"
            r"|bounded-stack-counter(?:[+-]0x[0-9a-f]+)?"
            r"|bounded-cursor\(e(?:ax|bx|cx|dx|si|di|bp),"
            r"0x[0-9a-f]+,0x[0-9a-f]+,0x[0-9a-f]+\))",
            abstract,
        )
        is not None
    )


def _bounded_index_provenance(abstract: str) -> bool:
    """Accept one scalar root or one non-aliased exact two-root sum."""

    if _cc_receiver_candidate._bounded_cfg_cursor(abstract) == "index":
        return True
    offset = re.fullmatch(r"index-offset\((.+),0x([0-9a-f]+)\)", abstract)
    if offset is not None:
        return (not offset.group(1).startswith("index-offset(")
                and 0 < int(offset.group(2), 16) <= _cc_catalog._BOUNDED_TARGETLESS_ARITHMETIC_LIMIT
                and _bounded_index_provenance(offset.group(1)))
    if _bounded_scalar_index_provenance(abstract):
        return True
    stride = _cc_receiver_candidate._bounded_stride_components(abstract)
    if (
        stride is not None
        and abstract.startswith("bounded-stride(")
        and stride[1] in _cc_catalog.BOUNDED_TARGETLESS_VPTR_STRIDES
    ):
        return True
    scaled = re.fullmatch(
        r"index-scale\((?P<source>(?:load\((?:entry-stack|stack-local|this)"
        r"(?:[+-]0x[0-9a-f]+)?\)|bounded-counter\(e(?:ax|bx|cx|dx|si|di|bp)\)"
        r"|bounded-stack-counter(?:[+-]0x[0-9a-f]+)?)),3\)",
        abstract,
    )
    if scaled is not None:
        return _bounded_scalar_index_provenance(scaled.group("source"))
    match = re.fullmatch(
        r"index-sum\((?P<left>(?:load\((?:entry-stack|stack-local|this)"
        r"(?:[+-]0x[0-9a-f]+)?\)|bounded-counter\(e(?:ax|bx|cx|dx|si|di|bp)\)"
        r"|bounded-stack-counter(?:[+-]0x[0-9a-f]+)?)),"
        r"(?P<right>(?:load\((?:entry-stack|stack-local|this)"
        r"(?:[+-]0x[0-9a-f]+)?\)|bounded-counter\(e(?:ax|bx|cx|dx|si|di|bp)\)"
        r"|bounded-stack-counter(?:[+-]0x[0-9a-f]+)?))\)",
        abstract,
    )
    return (
        match is not None
        and match.group("left") != match.group("right")
    )


def _exact_register_add_register(
    instruction: Instruction,
) -> tuple[str, str] | None:
    """Decode one exact unprefixed ``ADD r32, r32`` instruction."""

    if _cc_cfg._instruction_mnemonic(instruction) != "add":
        return None
    operands = [
        item.strip().lower()
        for item in _cc_cfg._instruction_operand(instruction).split(",")
    ]
    if (
        len(operands) != 2
        or not _cc_catalog.REGISTER32_RE.fullmatch(operands[0])
        or not _cc_catalog.REGISTER32_RE.fullmatch(operands[1])
    ):
        return None
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    if len(body) != 2 or body[0] not in {0x01, 0x03} or body[1] >> 6 != 3:
        return None
    registers = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    register_field = registers[(body[1] >> 3) & 7]
    rm_field = registers[body[1] & 7]
    decoded = (
        (rm_field, register_field)
        if body[0] == 0x01
        else (register_field, rm_field)
    )
    return decoded if tuple(operands) == decoded else None


def _exact_register_lea_sum(
    instruction: Instruction,
) -> tuple[str, str, str] | None:
    """Decode exact ``LEA dst,[left+right]`` scalar-sum syntax and bytes."""

    if _cc_cfg._instruction_mnemonic(instruction) != "lea":
        return None
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    if (
        len(body) != 3
        or body[0] != 0x8D
        or body[1] >> 6 != 0
        or (body[1] & 7) != 4
        or body[2] >> 6 != 0
        or ((body[2] >> 3) & 7) == 4
        or (body[2] & 7) == 5
    ):
        return None
    registers = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    destination = registers[(body[1] >> 3) & 7]
    index = registers[(body[2] >> 3) & 7]
    base = registers[body[2] & 7]
    operands = _cc_cfg._instruction_operand(instruction).split(",", 1)
    if len(operands) != 2 or operands[0].strip().lower() != destination:
        return None
    rendered = _cc_targets._exact_memory_expression(operands[1]).lower()
    if rendered not in {f"{base}+{index}", f"{index}+{base}"} or base == index:
        return None
    return destination, base, index


def _exact_register_lea_self_scale(
    instruction: Instruction,
) -> tuple[str, str, int] | None:
    """Decode exact ``LEA dst,[src+src*2]`` as a bounded scalar triple."""

    if _cc_cfg._instruction_mnemonic(instruction) != "lea":
        return None
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    if (
        len(body) != 3
        or body[0] != 0x8D
        or body[1] >> 6 != 0
        or (body[1] & 7) != 4
        or body[2] >> 6 != 1
        or (body[2] & 7) == 5
    ):
        return None
    registers = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    destination = registers[(body[1] >> 3) & 7]
    index = registers[(body[2] >> 3) & 7]
    base = registers[body[2] & 7]
    if base != index:
        return None
    operands = _cc_cfg._instruction_operand(instruction).split(",", 1)
    if len(operands) != 2 or operands[0].strip().lower() != destination:
        return None
    rendered = _cc_targets._exact_memory_expression(operands[1]).lower()
    if rendered not in {f"{base}+{base}*2", f"{base}*2+{base}"}:
        return None
    return destination, base, 3


def _exact_zero_register(instruction: Instruction) -> str | None:
    """Decode exact ``XOR r32,r32`` zero initialization."""

    if _cc_cfg._instruction_mnemonic(instruction) != "xor":
        return None
    operands = tuple(
        item.strip().lower()
        for item in _cc_cfg._instruction_operand(instruction).split(",")
    )
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    if len(body) != 2 or body[0] not in {0x31, 0x33} or body[1] >> 6 != 3:
        return None
    registers = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    left = registers[(body[1] >> 3) & 7]
    right = registers[body[1] & 7]
    return left if left == right and operands == (left, right) else None


def _exact_inc_register(instruction: Instruction) -> str | None:
    """Decode exact one-byte ``INC r32``."""

    if _cc_cfg._instruction_mnemonic(instruction) != "inc":
        return None
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    registers = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    if len(body) != 1 or not 0x40 <= body[0] <= 0x47:
        return None
    register = registers[body[0] - 0x40]
    return (
        register
        if _cc_cfg._instruction_operand(instruction).strip().lower() == register
        else None
    )


def _bounded_loop_counter_registers(
    instructions: Sequence[Instruction],
    *,
    source: str,
    caller_start: int,
    caller_end: int,
) -> Mapping[int, str]:
    """Prove exact nonvolatile XOR-zero/INC definitions on one backedge."""

    addresses = _cc_cfg._instruction_runtime_addresses(
        instructions, source=source, caller_start=caller_start
    )
    counts: dict[int, int] = {}
    for address in addresses:
        if address is not None and caller_start <= address < caller_end:
            counts[address] = counts.get(address, 0) + 1
    by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and counts.get(address) == 1
    }
    result: dict[int, str] = {}
    for zero_index, instruction in enumerate(instructions):
        register = _exact_zero_register(instruction)
        if register not in {"ebx", "ebp", "esi", "edi"}:
            continue
        for inc_index in range(zero_index + 1, len(instructions)):
            if _exact_inc_register(instructions[inc_index]) != register:
                continue
            if any(
                _cc_cfg._instruction_may_clobber_register(instructions[index], register)
                for index in range(zero_index + 1, inc_index)
            ):
                break
            has_backedge = any(
                (
                    branch := _cc_cfg._exact_local_direct_branch(
                        instructions[branch_index],
                        instruction_index=branch_index,
                        instruction_addresses=addresses,
                        instruction_index_by_address=by_address,
                        source=source,
                        caller_start=caller_start,
                        caller_end=caller_end,
                    )
                )
                is not None
                and zero_index < branch[1] <= inc_index
                for branch_index in range(inc_index + 1, len(instructions))
            )
            if has_backedge:
                result[zero_index] = register
                result[inc_index] = register
            break
    return result


def _bounded_stack_loop_counter_loads(
    instructions: Sequence[Instruction],
    *,
    source: str,
    caller_start: int,
    caller_end: int,
) -> Mapping[int, str]:
    """Prove a zero-initialized stack scalar updated on an exact backedge."""

    addresses = _cc_cfg._instruction_runtime_addresses(
        instructions, source=source, caller_start=caller_start
    )
    counts: dict[int, int] = {}
    for address in addresses:
        if address is not None and caller_start <= address < caller_end:
            counts[address] = counts.get(address, 0) + 1
    by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and counts.get(address) == 1
    }
    branches = [
        (index, branch)
        for index, instruction in enumerate(instructions)
        if (
            branch := _cc_cfg._exact_local_direct_branch(
                instruction,
                instruction_index=index,
                instruction_addresses=addresses,
                instruction_index_by_address=by_address,
                source=source,
                caller_start=caller_start,
                caller_end=caller_end,
            )
        )
        is not None
        and branch[0] == "conditional"
        and branch[1] <= index
    ]
    result: dict[int, str] = {}

    def decoded_store(index: int) -> tuple[int, str] | None:
        symbolic = _cc_receiver_instructions._exact_vc5_symbolic_stack_slot_store(instructions[index])
        if symbolic is not None:
            encoded, register, _symbol, _rendered = symbolic
            return encoded, register
        numeric = _cc_receiver_instructions._exact_stack_slot_store(instructions[index])
        return numeric

    def decoded_load(index: int) -> tuple[int, str] | None:
        symbolic = _cc_receiver_instructions._exact_vc5_symbolic_stack_slot_load(instructions[index])
        if symbolic is not None:
            encoded, register, _symbol, _rendered = symbolic
            return encoded, register
        numeric = _cc_receiver_instructions._exact_stack_slot_load(instructions[index])
        return (numeric[1], numeric[0]) if numeric else None

    stores = [
        (index, store)
        for index in range(len(instructions))
        if (store := decoded_store(index)) is not None
    ]
    for init_index, (slot_key, init_register) in stores:
        zero_indices = [
            index
            for index in range(init_index)
            if _exact_zero_register(instructions[index]) == init_register
            and not any(
                _cc_cfg._instruction_may_clobber_register(
                    instructions[between], init_register
                )
                for between in range(index + 1, init_index)
            )
        ]
        if len(zero_indices) != 1:
            continue
        for update_index, (update_slot_key, update_register) in stores:
            if update_slot_key != slot_key or update_index <= init_index:
                continue
            matching_loads = [
                (index, register)
                for index in range(init_index + 1, update_index)
                if (load := decoded_load(index)) is not None
                and load[0] == slot_key
                for register in (load[1],)
            ]
            if not matching_loads:
                continue
            load_index, load_register = matching_loads[-1]
            exact_increment = (
                update_register == load_register
                and any(
                    _exact_inc_register(instructions[index]) == load_register
                    for index in range(load_index + 1, update_index)
                )
            )
            exact_lea_increment = any(
                _cc_receiver_instructions._exact_register_lea(instructions[index])
                == (update_register, load_register, 1)
                for index in range(load_index + 1, update_index)
            )
            if not (exact_increment or exact_lea_increment):
                continue
            matching_backedges = [
                (branch_index, target_index)
                for branch_index, (_kind, target_index) in branches
                if update_index < branch_index
                and init_index < target_index <= load_index
            ]
            if len(matching_backedges) != 1:
                continue
            branch_index, target_index = matching_backedges[0]
            permitted_stores = {init_index, update_index}
            for store_index, (store_slot_key, store_register) in stores:
                if store_slot_key != slot_key or not update_index < store_index <= branch_index:
                    continue
                prior_loads = [
                    load_index
                    for load_index in range(update_index + 1, store_index)
                    if decoded_load(load_index) == (slot_key, store_register)
                    and not any(
                        _cc_cfg._instruction_may_clobber_register(
                            instructions[row], store_register
                        )
                        for row in range(load_index + 1, store_index)
                    )
                ]
                if len(prior_loads) == 1:
                    permitted_stores.add(store_index)
            if any(
                store_slot_key == slot_key and index not in permitted_stores
                for index, (store_slot_key, _register) in stores
                if init_index <= index <= branch_index
            ):
                continue
            if any(
                _cc_cfg._instruction_mnemonic(instructions[index]) == "lea"
                and _cc_receiver_instructions._exact_stack_operand_displacement(
                    _cc_cfg._instruction_operand(instructions[index])
                )
                == slot_key
                for index in range(init_index, branch_index + 1)
            ):
                continue
            marker = _cc_receiver_instructions._abstract_with_displacement(
                "bounded-stack-counter", slot_key
            )
            for index in range(target_index, branch_index + 1):
                load = decoded_load(index)
                if load is not None and load[0] == slot_key:
                    result[index] = marker
            # The proven counter remains bounded at exact post-loop loads.
            for index in range(branch_index + 1, len(instructions)):
                load = decoded_load(index)
                if load is not None and load[0] == slot_key:
                    result[index] = marker
                if any(
                    store_index == index and store_slot_key == slot_key
                    for store_index, (store_slot_key, _register) in stores
                ):
                    break
            break
    return result


def _bounded_stack_loop_counter_initializers(
    instructions: Sequence[Instruction],
    proven_loads: Mapping[int, str],
) -> Mapping[int, tuple[str, str]]:
    """Bind the exact zero feeding an already-proven stack recurrence."""

    markers = set(proven_loads.values())
    result: dict[int, tuple[str, str]] = {}
    for store_index, instruction in enumerate(instructions):
        symbolic = _cc_receiver_instructions._exact_vc5_symbolic_stack_slot_store(instruction)
        numeric = _cc_receiver_instructions._exact_stack_slot_store(instruction)
        if symbolic is not None:
            slot, register = symbolic[0], symbolic[1]
        elif numeric is not None:
            slot, register = numeric
        else:
            continue
        marker = _cc_receiver_instructions._abstract_with_displacement("bounded-stack-counter", slot)
        if marker not in markers:
            continue
        zero_indices = [
            index
            for index in range(store_index)
            if _exact_zero_register(instructions[index]) == register
            and not any(
                _cc_cfg._instruction_may_clobber_register(instructions[row], register)
                for row in range(index + 1, store_index)
            )
        ]
        if len(zero_indices) == 1:
            result[zero_indices[0]] = (register, marker)
    return result
