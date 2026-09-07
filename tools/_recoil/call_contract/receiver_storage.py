"""Recoil call-contract receiver storage evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import instructions as _cc_instructions
from _recoil.call_contract import receiver_candidate as _cc_receiver_candidate
from _recoil.call_contract import receiver_cursor as _cc_receiver_cursor
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions
from _recoil.call_contract import receiver_proofs as _cc_receiver_proofs
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateExactIatRegisterLoadProof,
        IdentityIndexes,
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedCallResultBridge,
        ReviewedExactIndirectStorageBridge,
        ReviewedInboundEntryRegisterTargetBridge,
        ReviewedLoopVptrStorageBridge,
        ReviewedMemberVptrStorageBridge,
        ReviewedStaticStorageReferenceBridge,
        ReviewedVptrStorageBridge,
    )

import re
import struct
from collections import Counter
from typing import Mapping, Sequence

from _recoil.commands.asm_verify import Instruction
from _recoil.lib.progress import address_value, normalize_address


def _bounded_targetless_cursor_registers(
    instructions: Sequence[Instruction],
    *,
    source: str,
    caller_start: int,
    caller_end: int,
) -> Mapping[int, tuple[str, str]]:
    """Prove the exact 0x150/+4 bounded cursor recurrence.

    The proof requires one nonvolatile register definition, one exact ADD,
    one immediately adjacent encoded CMP, and one local conditional backedge.
    Any intervening register definition, missing bound, overflow, or alternate
    recurrence leaves the cursor unclassified.
    """

    addresses = _cc_cfg._instruction_runtime_addresses(
        instructions,
        source=source,
        caller_start=caller_start,
    )
    counts = Counter(
        address
        for address in addresses
        if address is not None and caller_start <= address < caller_end
    )
    by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and counts[address] == 1
    }
    result: dict[int, tuple[str, str]] = {}
    for init_index, instruction in enumerate(instructions):
        initial = _cc_receiver_instructions._exact_register_move_immediate(instruction)
        if initial is None:
            continue
        register, start_value = initial
        if register not in {"ebx", "ebp", "esi", "edi"} or start_value != 0x150:
            continue
        matching_adds = [
            index
            for index in range(init_index + 1, len(instructions))
            if _cc_receiver_instructions._exact_register_add_immediate(instructions[index])
            == (register, 4)
        ]
        if len(matching_adds) != 1:
            continue
        add_index = matching_adds[0]
        branch_rows: list[tuple[int, int, int]] = []
        for branch_index in range(add_index + 2, len(instructions)):
            if _cc_cfg._instruction_mnemonic(instructions[branch_index]) not in {
                "jb",
                "jbe",
                "jl",
                "jle",
                "jna",
                "jnae",
                "jng",
                "jnge",
            }:
                continue
            exact_branch = _cc_cfg._exact_local_direct_branch(
                instructions[branch_index],
                instruction_index=branch_index,
                instruction_addresses=addresses,
                instruction_index_by_address=by_address,
                source=source,
                caller_start=caller_start,
                caller_end=caller_end,
            )
            comparison = _cc_receiver_instructions._exact_register_compare_immediate(
                instructions[branch_index - 1]
            )
            if (
                exact_branch is not None
                and exact_branch[0] == "conditional"
                and init_index < exact_branch[1] <= add_index
                and comparison is not None
                and comparison[0] == register
            ):
                branch_rows.append(
                    (branch_index, exact_branch[1], comparison[1])
                )
        if len(branch_rows) != 1:
            continue
        branch_index, _target_index, end_value = branch_rows[0]
        if (
            end_value <= start_value
            or end_value - start_value
            > _cc_catalog._BOUNDED_TARGETLESS_ARITHMETIC_LIMIT
            or (end_value - start_value) % 4
            or any(
                index != add_index
                and _cc_cfg._instruction_may_clobber_register(
                    instructions[index], register
                )
                for index in range(init_index + 1, branch_index)
            )
            or any(
                _cc_receiver_instructions._exact_register_move_immediate(row) is not None
                and _cc_receiver_instructions._exact_register_move_immediate(row)[0] == register
                for row in instructions[:init_index]
            )
        ):
            continue
        marker = (
            f"bounded-cursor({register},0x{start_value:x},0x4,"
            f"0x{end_value:x})"
        )
        result[init_index] = (register, marker)
        result[add_index] = (register, marker)
    return result


def _is_bounded_address_vptr(abstract: str) -> bool:
    """Accept one or two loads over one exact authored/storage address."""
    depth = 0
    value = abstract
    while value.startswith("load(") and value.endswith(")"):
        depth += 1
        value = value[5:-1]
    address_match = re.fullmatch(
        r"(?P<address>address\(.+\))"
        r"(?P<affine>(?:[+-]0x[0-9a-f]+)*)",
        value,
    )
    return (
        depth in {1, 2}
        and address_match is not None
        and _cc_receiver_cursor._address_cursor_seed(address_match.group("address")) is not None
    )


def _is_bounded_dynamic_load(
    abstract: str,
    *,
    allow_entry_register_root: bool = False,
    allow_exact_receiver_add: bool = False,
) -> bool:
    """Accept a small exact load chain rooted in known object storage.

    This deliberately preserves targetlessness.  It recognizes the bounded
    field/callback and receiver-vptr shapes emitted by the ordinary abstract
    interpreter, but excludes IAT, entry-register, null, and unknown roots.
    """

    if (
        not abstract
        or len(abstract) > 512
        or "iat:" in abstract
        or (
            "entry-register(" in abstract
            and not allow_entry_register_root
        )
        or (
            "exact-receiver-add(" in abstract
            and not allow_exact_receiver_add
        )
        or "null" in abstract
        or abstract.count("load(") not in {1, 2, 3}
    ):
        return False
    if "entry-register(" in abstract:
        sanitized, count = re.subn(
            r"entry-register\((?:ecx|edx)\)",
            "this",
            abstract,
        )
        if count != 1 or "entry-register(" in sanitized:
            return False
    else:
        sanitized = abstract
    if "exact-receiver-add(" in sanitized:
        sanitized = _canonical_exact_receiver_add_provenance(sanitized)
        if "exact-receiver-add(" in sanitized:
            return False
    return (
        "this" in sanitized
        or "storage:" in sanitized
        or "stack" in sanitized
        or "entry-stack" in sanitized
        or "call-result(" in sanitized
    )


def _bounded_loaded_receiver_pointer(abstract: str) -> bool:
    """Recognize one exact loaded object pointer eligible for receiver ADD.

    The marker emitted for this shape is intentionally unusable by ordinary
    dynamic callbacks.  Only the subsequent exact receiver/vptr equality path
    may consume it.
    """

    if (
        not abstract
        or len(abstract) > 256
        or abstract.count("load(") != 1
        or any(
            token in abstract
            for token in (
                "address(",
                "call-result(",
                "exact-receiver-add(",
                "frame",
                "iat:",
                "null",
                "stack",
            )
        )
    ):
        return False
    return (
        re.fullmatch(
            r"load\((?:"
            r"this"
            r"|storage:[A-Za-z0-9_:@?.\\/]+"
            r"|entry-register\((?:ecx|edx)\)"
            r")(?:[+-]0x[0-9a-f]+)?\)",
            abstract,
        )
        is not None
    )


def _canonical_exact_receiver_add_provenance(abstract: str) -> str:
    """Remove internal exact-ADD markers while retaining affine provenance."""

    marker = "exact-receiver-add("
    result = abstract
    while marker in result:
        start = result.rfind(marker)
        depth = 1
        end = start + len(marker)
        while end < len(result) and depth:
            if result[end] == "(":
                depth += 1
            elif result[end] == ")":
                depth -= 1
            end += 1
        if depth:
            return abstract
        payload = result[start + len(marker) : end - 1]
        base, separator, displacement = payload.rpartition(",+")
        if (
            not separator
            or not base
            or re.fullmatch(r"0x[0-9a-f]+", displacement) is None
        ):
            return abstract
        result = (
            result[:start]
            + f"{base}+{displacement}"
            + result[end:]
        )
    return result


def _canonical_address_root_affine_storage(abstract: str) -> str:
    """Fold exact signed-hex affine terms into one bounded address root.

    Extraction deliberately preserves the instruction-by-instruction pointer
    lineage.  Comparison may nevertheless see the same address represented as
    ``address(this+0x20)+0x4`` or ``address(this+0x24)``.  Canonicalize only the
    already accepted one/two-load address-vptr grammar, with a non-aliased
    ``this`` or simple ``storage:`` root and a signed 32-bit total.
    """
    if not _is_bounded_address_vptr(abstract):
        return abstract
    depth = 0
    value = abstract
    while value.startswith("load(") and value.endswith(")"):
        depth += 1
        value = value[5:-1]
    match = re.fullmatch(
        r"address\("
        r"(?P<root>this|storage:[A-Za-z0-9_:@?.\\/]+)"
        r"(?P<inner>(?:[+-]0x[0-9a-f]+)*)"
        r"\)"
        r"(?P<outer>(?:[+-]0x[0-9a-f]+)*)",
        value,
    )
    if match is None:
        return abstract
    displacement = _additive_provenance_displacement(
        match.group("inner") + match.group("outer")
    )
    if not -0x80000000 <= displacement <= 0x7FFFFFFF:
        return abstract
    canonical = (
        f"address("
        f"{_cc_receiver_instructions._abstract_with_displacement(match.group('root'), displacement)}"
        f")"
    )
    for _index in range(depth):
        canonical = f"load({canonical})"
    return canonical


def _is_exact_safe_stack_root_adjustment(
    instruction: Instruction,
    *,
    operation: str,
    parsed_immediate: int,
) -> bool:
    """Require an unprefixed ADD/SUB ESP encoding with one safe immediate."""
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return False
    expected_modrm = 0xC4 if operation == "add" else 0xEC
    encoded_immediate: int
    if len(body) == 3 and body[:2] == bytes((0x83, expected_modrm)):
        encoded_immediate = struct.unpack("<b", body[2:3])[0]
    elif len(body) == 6 and body[:2] == bytes((0x81, expected_modrm)):
        encoded_immediate = struct.unpack("<i", body[2:6])[0]
    else:
        return False
    return (
        0 <= encoded_immediate <= 0x7FFFFFFF
        and parsed_immediate == encoded_immediate
    )


def _eligible_add_pointer_provenance(abstract: str) -> bool:
    """Return whether exact positive pointer arithmetic may retain ``abstract``.

    Register arithmetic is useful only when the input already has a proven
    object/storage address lineage. Stack, frame, IAT, and arbitrary register
    values remain fail-closed. Wrapped ``address(...)`` forms retain their
    proven root; ``load(...)`` values remain ineligible because this layer has
    no type proof that an arbitrary loaded value is a pointer.
    """
    if (
        not abstract
        or abstract in {"stack", "frame"}
        or "iat:" in abstract
        or "load(" in abstract
        or "stack" in abstract
        or "frame" in abstract
    ):
        return False
    base = abstract
    while True:
        without_displacement = re.sub(
            r"(?:[+-]0x[0-9a-f]+)+$",
            "",
            base,
        )
        if without_displacement != base:
            base = without_displacement
            continue
        wrapper = re.fullmatch(r"(?:address|load)\((.+)\)", base)
        if wrapper is not None:
            base = wrapper.group(1)
            continue
        break
    return base == "this" or base.startswith("storage:")


def _candidate_c_external_storage_identity(
    name: str,
    *,
    indexes: IdentityIndexes,
) -> str:
    """Resolve one exact x86 C external decoration without guessing identity.

    VC5 prefixes C-linkage external names with one underscore in COFF/COD.
    The tracker may carry the authored identifier while BN navigation may
    already carry the decorated spelling.  Accept either spelling only when
    their tracker-backed identities do not conflict.
    """
    exact_identity = indexes.storage_by_name.get(name, "")
    if (
        not re.fullmatch(r"_[A-Za-z_][A-Za-z0-9_]*", name)
        or name.startswith("__imp_")
    ):
        return exact_identity
    authored_identity = indexes.storage_by_name.get(name[1:], "")
    if (
        exact_identity
        and authored_identity
        and exact_identity != authored_identity
    ):
        raise ValueError(
            f"conflicting exact and x86 C external storage identities for "
            f"{name!r}"
        )
    return exact_identity or authored_identity


def _exact_iat_register_load_provenance(
    instruction: Instruction,
    *,
    assembly_source: str,
    destination: str,
    exact_memory: str,
    rendered_address: str | None,
    identity: str,
    indexes: IdentityIndexes,
    instruction_address: str,
    reviewed_identity: bool,
    candidate_proof: CandidateExactIatRegisterLoadProof | None = None,
) -> str:
    """Prove one unique unprefixed ``MOV r32, [absolute IAT]`` definition.

    The load site remains part of the abstract value so CFG joins cannot merge
    two distinct reaching definitions merely because both happen to read the
    same IAT slot.  Candidate COD listings must retain the case-exact external
    symbol and the four relocation placeholders; retail must encode and render
    the same absolute address indexed by the current provider package.
    """
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError) as exc:
        raise ValueError(
            "IAT register provenance requires exact MOV encoding"
        ) from exc
    registers = (
        "eax",
        "ecx",
        "edx",
        "ebx",
        "esp",
        "ebp",
        "esi",
        "edi",
    )
    definition_address = instruction_address
    if (
        not identity.startswith("iat:")
        or len(body) != 6
        or body[0] != 0x8B
        or body[1] >> 6 != 0
        or body[1] & 0x07 != 0x05
        or registers[(body[1] >> 3) & 0x07] != destination
        or not definition_address
    ):
        raise ValueError(
            "IAT register provenance requires one exact absolute MOV load"
        )
    decoded_address = normalize_address(struct.unpack("<I", body[2:6])[0])
    if assembly_source == "bn":
        retail_address = rendered_address or decoded_address
        if (
            decoded_address != retail_address
            or (
                indexes.storage_by_address.get(retail_address, "") != identity
                and not reviewed_identity
            )
        ):
            raise ValueError(
                "retail IAT register load encoding/address identity drift"
            )
        if rendered_address is None:
            named_identity = indexes.storage_by_name.get(exact_memory, "")
            if named_identity != identity and not reviewed_identity:
                raise ValueError(
                    "retail IAT register load name/address identity drift"
                )
    elif assembly_source == "cod":
        candidate_proof_matches = (
            candidate_proof is not None
            and candidate_proof.definition_offset == definition_address
            and candidate_proof.destination == destination
            and candidate_proof.object_symbol == exact_memory
            and candidate_proof.identity == identity
            and bool(candidate_proof.transfer_offsets)
        )
        if (
            rendered_address is not None
            or body[2:6] != b"\x00\x00\x00\x00"
            or re.fullmatch(r"__imp_[A-Za-z0-9_@$?]+", exact_memory)
            is None
            or (
                indexes.storage_by_name.get(exact_memory, "") != identity
                and not reviewed_identity
                and not candidate_proof_matches
            )
        ):
            raise ValueError(
                "candidate IAT register load symbol/placeholder identity "
                "drift: "
                f"memory={exact_memory!r}, identity={identity!r}, "
                f"definition={instruction_address!r}, "
                f"indexed={indexes.storage_by_name.get(exact_memory, '')!r}, "
                f"reviewed={reviewed_identity!r}, "
                f"proof={candidate_proof!r}"
            )
    else:
        raise ValueError(
            f"unsupported IAT register provenance source {assembly_source!r}"
        )
    return (
        f"exact-iat-load({destination},{definition_address},{identity})"
    )


def _additive_provenance_displacement(abstract: str) -> int:
    return sum(
        (-1 if match.group("sign") == "-" else 1)
        * int(match.group("number"), 16)
        for match in re.finditer(
            r"(?P<sign>[+-])(?P<number>0x[0-9a-f]+)",
            abstract,
        )
    )


def _canonical_indirect_storage(
    operand: str,
    *,
    instruction: Instruction,
    reviewed_instruction_address: str | None = None,
    source: str,
    registers: Mapping[str, str],
    indexes: IdentityIndexes,
    candidate_storage_bridges: Mapping[str, str],
    reviewed_call_result_bridges: Mapping[str, ReviewedCallResultBridge],
    consumed_call_result_bridges: set[str],
    reviewed_vptr_storage_bridges: Mapping[
        str, ReviewedVptrStorageBridge
    ],
    consumed_vptr_storage_bridges: set[str],
    reviewed_loop_vptr_storage_bridges: Mapping[
        str, ReviewedLoopVptrStorageBridge
    ],
    consumed_loop_vptr_storage_bridges: set[str],
    reviewed_exact_indirect_storage_bridges: Mapping[
        str, ReviewedExactIndirectStorageBridge
    ],
    consumed_exact_indirect_storage_bridges: set[str],
    reviewed_member_vptr_call_bridges: Mapping[
        str, ReviewedMemberVptrStorageBridge
    ],
    consumed_member_vptr_call_bridges: set[str],
    reviewed_inbound_entry_register_target_bridge: (
        ReviewedInboundEntryRegisterTargetBridge | None
    ) = None,
    natural_nested_absolute_loads: frozenset[str] = frozenset(),
    exact_targetless_vptr_storage: str = "",
) -> tuple[str, str, int | None, str]:
    exact_direct_iat = _cc_receiver_instructions._exact_direct_iat_storage(
        operand,
        instruction=instruction,
        source=source,
        indexes=indexes,
    )
    if exact_direct_iat is not None:
        return exact_direct_iat
    exact_expression = _cc_targets._exact_memory_expression(operand)
    exact_identity = indexes.storage_by_name.get(exact_expression, "")
    if exact_identity.startswith("iat:"):
        raise ValueError(
            f"retail IAT name {exact_expression!r} requires an exact "
            "unprefixed FF15 operand"
        )
    expression, displacement = _cc_targets._memory_slot(operand)
    base_match = re.search(r"\b(e?[abcd]x|e?[sd]i|e?[sb]p)\b", expression)
    if base_match is not None:
        base = base_match.group(1)
        abstract = registers.get(base, "")
        source_instruction_address = _cc_cfg._source_instruction_address(instruction)
        if (
            reviewed_instruction_address
            and source_instruction_address
            and normalize_address(reviewed_instruction_address)
            != normalize_address(source_instruction_address)
        ):
            raise ValueError(
                "reviewed indirect-call runtime/source address disagreement"
            )
        raw_instruction_address = (
            reviewed_instruction_address or source_instruction_address
        )
        instruction_address = (
            normalize_address(raw_instruction_address)
            if raw_instruction_address
            else ""
        )
        reviewed_exact_indirect = (
            reviewed_exact_indirect_storage_bridges.get(instruction_address)
        )
        if exact_targetless_vptr_storage:
            exact_zsnd_a3d_duplicate = (
                exact_targetless_vptr_storage
                == _cc_catalog._ZSND_A3D_DUPLICATE_CANDIDATE_VPTR
                and base == "ecx"
                and displacement == 0x08
            )
            exact_zsnd_directsound_duplicate = (
                exact_targetless_vptr_storage
                == _cc_catalog._ZSND_DIRECTSOUND_DUPLICATE_CANDIDATE_VPTR
                and base == "ecx"
                and displacement == 0x08
            )
            if (
                displacement is None
                or displacement <= 0
                or exact_expression
                not in {
                    f"{base}+0x{displacement:x}",
                    f"{base}+{displacement}",
                }
                or not _cc_receiver_candidate._is_bounded_stack_vptr(
                    exact_targetless_vptr_storage
                )
                and not _is_bounded_dynamic_load(
                    exact_targetless_vptr_storage,
                    allow_entry_register_root=True,
                    allow_exact_receiver_add=True,
                )
                and not exact_zsnd_a3d_duplicate
                and not exact_zsnd_directsound_duplicate
            ):
                raise ValueError(
                    "exact targetless vptr proof does not match invocation"
                )
            return (
                "virtual-slot",
                "",
                displacement,
                exact_targetless_vptr_storage,
            )
        if reviewed_exact_indirect is not None:
            expected_expressions = {
                (
                    f"{reviewed_exact_indirect.register}"
                    f"+0x{reviewed_exact_indirect.slot_displacement:x}"
                ),
                (
                    f"{reviewed_exact_indirect.register}"
                    f"+{reviewed_exact_indirect.slot_displacement}"
                ),
            }
            expected_displacements: set[int | None] = {
                reviewed_exact_indirect.slot_displacement
            }
            if reviewed_exact_indirect.slot_displacement == 0:
                expected_expressions.add(reviewed_exact_indirect.register)
                expected_displacements.add(None)
            if (
                instruction_address
                in consumed_exact_indirect_storage_bridges
            ):
                raise ValueError(
                    "duplicate use of reviewed exact indirect storage bridge "
                    f"at {instruction_address}"
                )
            if (
                reviewed_exact_indirect.assembly_source != source
                or exact_expression not in expected_expressions
                or base != reviewed_exact_indirect.register
                or displacement not in expected_displacements
                or not reviewed_exact_indirect.storage_identity
                or reviewed_exact_indirect.storage_identity.startswith(
                    "iat:"
                )
            ):
                raise ValueError(
                    "reviewed exact indirect storage bridge does not match "
                    f"invocation at {instruction_address}"
                )
            consumed_exact_indirect_storage_bridges.add(
                instruction_address
            )
            if reviewed_exact_indirect.identity_kind not in {
                "virtual-slot", "callback"
            }:
                raise ValueError(
                    "reviewed exact indirect storage bridge has unsupported "
                    "identity kind"
                )
            return (
                reviewed_exact_indirect.identity_kind,
                "",
                (
                    reviewed_exact_indirect.slot_displacement
                    if reviewed_exact_indirect.identity_kind == "virtual-slot"
                    else None
                ),
                reviewed_exact_indirect.storage_identity,
            )
        reviewed_loop_vptr = reviewed_loop_vptr_storage_bridges.get(
            instruction_address
        )
        if reviewed_loop_vptr is not None:
            if instruction_address in consumed_loop_vptr_storage_bridges:
                raise ValueError(
                    "duplicate use of reviewed HUD layout-loop vptr storage "
                    f"bridge at {instruction_address}"
                )
            expected_loop_expressions = {
                f"{reviewed_loop_vptr.register}"
                f"+0x{reviewed_loop_vptr.slot_displacement:x}",
                f"{reviewed_loop_vptr.register}"
                f"+{reviewed_loop_vptr.slot_displacement}",
            }
            expected_loop_displacements: set[int | None] = {
                reviewed_loop_vptr.slot_displacement
            }
            if reviewed_loop_vptr.slot_displacement == 0:
                expected_loop_expressions.add(reviewed_loop_vptr.register)
                expected_loop_displacements.add(None)
            if (
                reviewed_loop_vptr.assembly_source != source
                or exact_expression not in expected_loop_expressions
                or base != reviewed_loop_vptr.register
                or displacement not in expected_loop_displacements
            ):
                raise ValueError(
                    "reviewed HUD layout-loop vptr storage bridge does not "
                    f"match invocation at {instruction_address}"
                )
            consumed_loop_vptr_storage_bridges.add(instruction_address)
            return (
                (
                    "callback"
                    if reviewed_loop_vptr.slot_displacement == 0
                    and reviewed_loop_vptr.storage_identity
                    == "dynamic:zInterp_Context+0x70-callback"
                    else "virtual-slot"
                ),
                reviewed_loop_vptr.target_identity,
                reviewed_loop_vptr.slot_displacement,
                reviewed_loop_vptr.storage_identity,
            )
        reviewed_member_vptr = reviewed_member_vptr_call_bridges.get(
            instruction_address
        )
        if reviewed_member_vptr is not None:
            expected_provenance = (
                f"exact-member-vptr({reviewed_member_vptr.register},"
                f"{reviewed_member_vptr.storage_identity})"
            )
            expected_expressions = {
                (
                    f"{reviewed_member_vptr.register}"
                    f"+0x{reviewed_member_vptr.slot_displacement:x}"
                ),
                (
                    f"{reviewed_member_vptr.register}"
                    f"+{reviewed_member_vptr.slot_displacement}"
                ),
            }
            expected_displacements: set[int | None] = {
                reviewed_member_vptr.slot_displacement
            }
            if reviewed_member_vptr.slot_displacement == 0:
                expected_expressions.add(reviewed_member_vptr.register)
                expected_displacements.add(None)
            if instruction_address in consumed_member_vptr_call_bridges:
                raise ValueError(
                    "duplicate use of reviewed member-vptr storage bridge at "
                    f"{instruction_address}"
                )
            if (
                exact_expression not in expected_expressions
                or base != reviewed_member_vptr.register
                or abstract != expected_provenance
                or displacement not in expected_displacements
                or registers.get(
                    reviewed_member_vptr.receiver_register,
                    "",
                )
                != reviewed_member_vptr.receiver_provenance
            ):
                raise ValueError(
                    "reviewed member-vptr storage bridge does not match "
                    f"invocation and receiver at {instruction_address}"
                )
            if (
                normalize_address(reviewed_member_vptr.call_address)
                != instruction_address
            ):
                raise ValueError(
                    "reviewed member-vptr call bridge is not indexed by its "
                    f"exact call address at {instruction_address}"
                )
            consumed_member_vptr_call_bridges.add(instruction_address)
            return (
                "virtual-slot",
                "",
                reviewed_member_vptr.slot_displacement,
                reviewed_member_vptr.storage_identity,
            )
        inbound_entry = reviewed_inbound_entry_register_target_bridge
        if inbound_entry is not None:
            expected_provenance = (
                f"entry-register({inbound_entry.entry_register})"
            )
            if (
                exact_expression != inbound_entry.call_register
                or base != inbound_entry.call_register
                or displacement is not None
                or abstract != expected_provenance
                or not inbound_entry.target_identity
                or inbound_entry.target_identity
                not in {
                    *indexes.by_address.values(),
                    *indexes.reviewed_icf_group_by_address.values(),
                    *(
                        alias.identity
                        for aliases in indexes.reviewed_logical_aliases_by_address.values()
                        for alias in aliases
                    ),
                }
                or (
                    source == "bn"
                    and instruction_address
                    != normalize_address(inbound_entry.retail_call_address)
                )
            ):
                raise ValueError(
                    "reviewed BN inbound entry-register target does not match "
                    "the exact indirect invocation lineage"
                )
            return (
                "provider"
                if inbound_entry.target_identity in indexes.provider_ids
                else "direct",
                inbound_entry.target_identity,
                None,
                expected_provenance,
            )
        if not abstract:
            raise ValueError(
                f"unresolved indirect register storage {base} "
                f"at {instruction_address}"
            )
        storage = abstract
        if (
            abstract == "this"
            and isinstance(displacement, int)
            and displacement > 0
            and exact_expression
            in {
                f"{base}+0x{displacement:x}",
                f"{base}+{displacement}",
            }
        ):
            return (
                "callback",
                "",
                displacement,
                _cc_receiver_instructions._abstract_with_displacement(abstract, displacement),
            )
        exact_iat_load = re.fullmatch(
            r"exact-iat-load\("
            r"(?P<register>e(?:ax|bx|cx|dx|si|di|bp)),"
            r"(?P<definition>0x[0-9a-f]+),"
            r"(?P<identity>iat:[^)]+)\)",
            abstract,
        )
        exact_iat_join = re.fullmatch(
            r"exact-iat-join\("
            r"(?P<register>e(?:ax|bx|cx|dx|si|di|bp)),"
            r"(?P<identity>iat:[^)]+)\)",
            abstract,
        )
        exact_iat_provenance = exact_iat_load or exact_iat_join
        if exact_iat_provenance is not None:
            identity = exact_iat_provenance.group("identity")
            if (
                base != exact_iat_provenance.group("register")
                or exact_expression != base
                or displacement is not None
            ):
                raise ValueError(
                    f"unresolved indirect register storage {base}"
                )
            return "iat", identity, None, identity
        if abstract.startswith("iat:"):
            raise ValueError(
                f"unproven bare IAT register provenance {base}: {abstract!r}"
            )
        if abstract.startswith("storage:"):
            static_target = (
                indexes.reviewed_static_callback_target_by_storage.get(
                    abstract,
                    "",
                )
                if exact_expression == base and displacement is None
                else ""
            )
            if static_target:
                return (
                    "provider"
                    if static_target in indexes.provider_ids
                    else "direct",
                    static_target,
                    None,
                    abstract,
                )
            return "callback", "", displacement, abstract
        entry_register = re.fullmatch(
            r"entry-register\((?P<register>e(?:ax|bx|cx|dx|si|di|bp))\)",
            abstract,
        )
        if entry_register is not None:
            if (
                exact_expression
                not in {
                    base,
                    f"{base}+0x{displacement:x}"
                    if isinstance(displacement, int) and displacement > 0
                    else "",
                    f"{base}+{displacement}"
                    if isinstance(displacement, int) and displacement > 0
                    else "",
                }
                or base == "esp"
            ):
                raise ValueError(
                    f"unresolved indirect entry-register storage {base}"
                )
            # Runtime storage is exact, but the entry value does not prove a
            # unique static target.  Preserve the structured provenance while
            # intentionally leaving target_identity blank.
            return (
                "callback",
                "",
                displacement,
                _cc_receiver_instructions._abstract_with_displacement(abstract, displacement),
            )
        exact_absolute_load = re.fullmatch(
            r"exact-load\((?P<register>e(?:ax|bx|cx|dx|si|di|bp)),"
            r"(?P<provenance>load\(storage:[^)]+\))\)",
            abstract,
        )
        if exact_absolute_load is not None:
            if base != exact_absolute_load.group("register"):
                raise ValueError(
                    f"unresolved indirect register storage {base}"
                )
            abstract = exact_absolute_load.group("provenance")
            storage = abstract
        nested_exact_absolute_load = re.fullmatch(
            r"load\(exact-load\((?P<register>e(?:ax|bx|cx|dx|si|di|bp)),"
            r"(?P<provenance>load\(storage:[^)]+\))\)\)",
            abstract,
        )
        if nested_exact_absolute_load is not None:
            exact_marker = (
                "exact-load("
                f"{nested_exact_absolute_load.group('register')},"
                f"{nested_exact_absolute_load.group('provenance')})"
            )
            if exact_marker in natural_nested_absolute_loads:
                # The marker is internal evidence that the inner pointer came
                # from one uniquely consumed, relocation-backed absolute load.
                # Only an explicitly reviewed bridge may expose the equivalent
                # natural two-load identity used by retail.
                abstract = (
                    f"load({nested_exact_absolute_load.group('provenance')})"
                )
                storage = abstract
        if abstract.startswith("load("):
            if abstract == "load(null)":
                raise ValueError(
                    f"unresolved indirect register storage {base}"
                )
            if (
                exact_expression == base
                and displacement is None
                and re.fullmatch(
                    r"load\((?:"
                    r"(?:stack|entry-stack)(?:[+-]0x[0-9a-f]+)?"
                    r"|this(?:[+-]0x[0-9a-f]+)?"
                    r"|storage:[A-Za-z0-9_:@?.\\/]+"
                    r"(?:[+-]0x[0-9a-f]+)?"
                    r")\)",
                    abstract,
                )
                is not None
            ):
                # One exact load followed by CALL r32 is a callback value,
                # not a vtable slot.  Keep the stack/object-field/static-field
                # runtime storage and leave the static callee unresolved.
                return "callback", "", None, abstract
            if (
                exact_expression == base
                and displacement is None
                and "stack" not in abstract
                and _is_bounded_dynamic_load(abstract)
            ):
                # A bounded loaded value invoked directly is a dynamic field
                # callback.  Its storage is exact, but no static callee is.
                return "callback", "", None, abstract
            source_instruction_address = _cc_cfg._source_instruction_address(
                instruction
            )
            if (
                reviewed_instruction_address
                and source_instruction_address
                and normalize_address(reviewed_instruction_address)
                != normalize_address(source_instruction_address)
            ):
                raise ValueError(
                    "reviewed indirect-call runtime/source address "
                    "disagreement"
                )
            raw_instruction_address = (
                reviewed_instruction_address or source_instruction_address
            )
            instruction_address = (
                normalize_address(raw_instruction_address)
                if raw_instruction_address
                else ""
            )
            reviewed_vptr = reviewed_vptr_storage_bridges.get(
                instruction_address
            )
            if reviewed_vptr is not None:
                if instruction_address in consumed_vptr_storage_bridges:
                    raise ValueError(
                        "duplicate use of reviewed exact-callsite vptr "
                        f"storage bridge at {instruction_address}"
                    )
                if (
                    exact_expression not in {
                        (
                            f"{reviewed_vptr.register}"
                            f"+0x{reviewed_vptr.slot_displacement:x}"
                        ),
                        (
                            f"{reviewed_vptr.register}"
                            f"+{reviewed_vptr.slot_displacement}"
                        ),
                    }
                    or base != reviewed_vptr.register
                    or abstract != reviewed_vptr.provenance
                    or displacement
                    != reviewed_vptr.slot_displacement
                    or reviewed_vptr.identity_kind
                    not in {"callback", "virtual-slot"}
                ):
                    raise ValueError(
                        "reviewed exact-callsite vptr storage bridge "
                        f"does not match invocation at {instruction_address}"
                    )
                consumed_vptr_storage_bridges.add(instruction_address)
                return (
                    reviewed_vptr.identity_kind,
                    "",
                    reviewed_vptr.slot_displacement,
                    reviewed_vptr.storage_identity,
                )
            exact_memory_operands = _cc_catalog.MEMORY_RE.findall(operand)
            if (
                len(exact_memory_operands) != 1
                or re.fullmatch(
                    rf"{re.escape(base)}"
                    r"(?:[+-](?:0x[0-9a-f]+|\d+))?",
                    expression,
                )
                is None
            ):
                raise ValueError(
                    f"unresolved indirect register storage {base}"
                )
            if "stack" in abstract:
                if not _cc_receiver_candidate._is_bounded_stack_vptr(abstract):
                    raise ValueError(
                        f"unresolved indirect register storage {base}: unbounded stack lineage {abstract!r}"
                    )
            receiver = registers.get("ecx", "")
            bounded_targetless_form = (
                "bounded-stride(" in abstract
                or "bounded-cursor(" in abstract
            )
            if (
                bounded_targetless_form
                and displacement not in _cc_catalog.BOUNDED_TARGETLESS_VPTR_SLOTS
            ):
                raise ValueError(
                    "bounded targetless vptr call uses an unreviewed slot "
                    f"displacement {displacement!r}"
                )
            canonical_abstract = (
                _canonical_exact_receiver_add_provenance(abstract)
            )
            canonical_receiver = (
                _canonical_exact_receiver_add_provenance(receiver)
            )
            canonical_receiver_load_base = canonical_receiver
            if (
                canonical_receiver.startswith("address(affine(")
                and canonical_receiver.endswith(")")
            ):
                canonical_receiver_load_base = canonical_receiver[
                    len("address(") : -1
                ]
            exact_receiver_vptr = (
                canonical_abstract
                == f"load({canonical_receiver_load_base})"
                and canonical_receiver_load_base
                and _is_bounded_dynamic_load(
                    canonical_abstract,
                    allow_entry_register_root=True,
                    allow_exact_receiver_add=True,
                )
            )
            if bounded_targetless_form and not exact_receiver_vptr:
                raise ValueError(
                    "bounded targetless vptr call "
                    f"{instruction_address or '<unknown>'} lost exact "
                    "receiver/vptr lineage: "
                    f"expression={exact_expression!r}, base={base!r}, "
                    f"displacement={displacement!r}, abstract={abstract!r}, "
                    f"receiver={receiver!r}, "
                    f"canonical_abstract={canonical_abstract!r}, "
                    f"canonical_receiver={canonical_receiver!r}"
                )
            if exact_receiver_vptr:
                # Exact receiver equality is stronger than guessing through
                # an unbounded address expression: ECX is the same bounded
                # object address whose first word supplied the call base.
                return (
                    "virtual-slot",
                    "",
                    displacement,
                    _canonical_exact_receiver_add_provenance(storage),
                )
            if (
                "address(" in abstract
                and not _is_bounded_address_vptr(abstract)
            ):
                raise ValueError(
                    f"unresolved indirect register storage {base}"
                )
            if (
                "entry-register(" in abstract
                or "exact-receiver-add(" in abstract
                or "frame" in abstract
            ):
                raise ValueError(
                    f"unresolved indirect register storage {base}: {abstract!r}"
                )
            return "virtual-slot", "", displacement, storage
        if (
            abstract.startswith("call-result(")
            or abstract.startswith("nullable(call-result(")
        ):
            source_instruction_address = _cc_cfg._source_instruction_address(
                instruction
            )
            if (
                reviewed_instruction_address
                and source_instruction_address
                and normalize_address(reviewed_instruction_address)
                != normalize_address(source_instruction_address)
            ):
                raise ValueError(
                    "reviewed indirect-call runtime/source address "
                    "disagreement"
                )
            raw_instruction_address = (
                reviewed_instruction_address or source_instruction_address
            )
            instruction_address = (
                normalize_address(raw_instruction_address)
                if raw_instruction_address
                else ""
            )
            reviewed = reviewed_call_result_bridges.get(instruction_address)
            if (
                reviewed is not None
                and exact_expression == base
                and displacement is None
                and base == reviewed.register
                and abstract == reviewed.provenance
                and reviewed.target_identity in indexes.provider_ids
                and instruction_address not in consumed_call_result_bridges
            ):
                consumed_call_result_bridges.add(instruction_address)
                return (
                    "provider",
                    reviewed.target_identity,
                    None,
                    reviewed.provenance,
                )
            raise ValueError(f"unresolved indirect register storage {base}")
        raise ValueError(f"unresolved indirect register provenance {base}: {abstract!r}")
    address_matches = _cc_catalog.ADDRESS_RE.findall(expression)
    if address_matches:
        address = normalize_address(address_matches[-1])
        storage = indexes.storage_by_address.get(address, "")
        if storage.startswith("iat:"):
            raise ValueError(
                f"retail IAT address {address} requires an exact unprefixed "
                "FF15 operand"
            )
        value = address_value(address)
        containers = [
            row
            for row in indexes.storage_containers
            if row.start <= value < row.end_exclusive
        ]
        if len(containers) > 1:
            raise ValueError(f"ambiguous indirect storage containers at {address}")
        if containers:
            container = containers[0]
            if storage and storage != container.identity:
                raise ValueError(
                    f"conflicting exact and container storage identities at {address}"
                )
            container_offset = value - container.start
            static_target = (
                indexes.reviewed_static_callback_target_by_storage.get(
                    container.identity,
                    "",
                )
                if container_offset == 0
                else ""
            )
            return (
                (
                    "provider"
                    if static_target in indexes.provider_ids
                    else "direct"
                )
                if static_target
                else "callback",
                static_target,
                container_offset,
                container.identity,
            )
        if not storage:
            raise ValueError(f"unresolved indirect storage identity {address}")
        static_target = (
            indexes.reviewed_static_callback_target_by_storage.get(
                storage,
                "",
            )
            if displacement in {None, 0}
            else ""
        )
        return (
            (
                "provider"
                if static_target in indexes.provider_ids
                else "direct"
            )
            if static_target
            else "callback",
            static_target,
            displacement,
            storage,
        )
    folded_name_expression = re.sub(
        r"(?:[+-](?:0x[0-9a-f]+|\d+))$",
        "",
        expression,
    )
    name_expression = re.sub(
        r"(?:[+-](?:0x[0-9a-fA-F]+|\d+))$",
        "",
        exact_expression,
    )
    bridge_identities = {
        identity
        for identity in (
            candidate_storage_bridges.get(name_expression, ""),
            candidate_storage_bridges.get(folded_name_expression, ""),
        )
        if identity
    }
    if len(bridge_identities) > 1:
        raise ValueError(
            f"conflicting candidate storage bridges for {name_expression!r}"
        )
    bridged_storage = (
        next(iter(bridge_identities)) if bridge_identities else ""
    )
    if bridged_storage:
        return (
            "callback",
            "",
            0 if name_expression == exact_expression else displacement,
            bridged_storage,
        )
    exact_storage = (
        _candidate_c_external_storage_identity(
            name_expression,
            indexes=indexes,
        )
        if source == "cod"
        else indexes.storage_by_name.get(name_expression, "")
    )
    if exact_storage:
        containers = [
            row
            for row in indexes.storage_containers
            if row.identity == exact_storage
        ]
        if len(containers) > 1:
            raise ValueError(
                f"ambiguous indirect storage containers for {name_expression!r}"
            )
        named_displacement = (
            0
            if name_expression == exact_expression
            else displacement
        )
        if containers and (
            named_displacement is None
            or named_displacement < 0
            or containers[0].start + named_displacement
            >= containers[0].end_exclusive
        ):
            raise ValueError(
                f"indirect storage displacement outside reviewed BN extent "
                f"for {name_expression!r}"
            )
        if exact_storage.startswith("iat:"):
            raise ValueError(
                f"retail IAT name {name_expression!r} requires an exact "
                "unprefixed FF15 operand"
            )
        static_target = (
            indexes.reviewed_static_callback_target_by_storage.get(
                exact_storage,
                "",
            )
            if named_displacement in {None, 0}
            else ""
        )
        return (
            (
                "provider"
                if static_target in indexes.provider_ids
                else "direct"
            )
            if static_target
            else "callback",
            static_target,
            named_displacement,
            exact_storage,
        )
    decorated = _cc_catalog.DECORATED_RE.search(exact_expression)
    if decorated is not None:
        name = decorated.group(0)
        storage = indexes.storage_by_name.get(name, "")
        if not storage:
            raise ValueError(f"unresolved indirect storage identity {name!r}")
        containers = [
            row
            for row in indexes.storage_containers
            if row.identity == storage
        ]
        if len(containers) > 1:
            raise ValueError(
                f"ambiguous indirect storage containers for {name!r}"
            )
        named_displacement = (
            0
            if containers and name == exact_expression
            else displacement
        )
        if containers and (
            named_displacement is None
            or named_displacement < 0
            or containers[0].start + named_displacement
            >= containers[0].end_exclusive
        ):
            raise ValueError(
                f"indirect storage displacement outside reviewed BN extent "
                f"for {name!r}"
            )
        if storage.startswith("iat:"):
            return "iat", storage, None, storage
        static_target = (
            indexes.reviewed_static_callback_target_by_storage.get(
                storage,
                "",
            )
            if named_displacement in {None, 0}
            else ""
        )
        return (
            (
                "provider"
                if static_target in indexes.provider_ids
                else "direct"
            )
            if static_target
            else "callback",
            static_target,
            named_displacement,
            storage,
        )
    raise ValueError(f"unresolved indirect operand storage {operand!r}")


def _update_register_state(
    instruction: Instruction,
    registers: dict[str, str],
    *,
    assembly_source: str,
    indexes: IdentityIndexes,
    reviewed_register_storage_bridges: Mapping[str, str],
    reviewed_static_storage_reference_bridges: Mapping[
        str, ReviewedStaticStorageReferenceBridge
    ] | None = None,
    reviewed_absolute_storage_load_bridges: Mapping[
        str, ReviewedAbsoluteStorageLoadBridge
    ] | None = None,
    consumed_absolute_storage_load_bridges: set[str] | None = None,
    reviewed_member_vptr_load_bridges: Mapping[
        str, ReviewedMemberVptrStorageBridge
    ] | None = None,
    consumed_member_vptr_load_bridges: set[str] | None = None,
    reviewed_instruction_address: str | None = None,
    candidate_exact_iat_register_load_proofs: Mapping[
        str, CandidateExactIatRegisterLoadProof
    ] | None = None,
    consumed_candidate_exact_iat_register_load_proofs: set[str] | None = None,
    entry_stack_root_allowed: bool = False,
    bounded_local_stack_provenance: str = "",
    bounded_loop_counter_register: str = "",
    bounded_targetless_cursor: tuple[str, str] | None = None,
) -> None:
    text = instruction.raw_text.strip()
    candidate_iat_proofs = candidate_exact_iat_register_load_proofs or {}
    consumed_candidate_iat_proofs = (
        consumed_candidate_exact_iat_register_load_proofs
        if consumed_candidate_exact_iat_register_load_proofs is not None
        else set()
    )
    if bounded_targetless_cursor is not None:
        cursor_register, cursor_marker = bounded_targetless_cursor
        cursor_initial = _cc_receiver_instructions._exact_register_move_immediate(instruction)
        cursor_increment = _cc_receiver_instructions._exact_register_add_immediate(instruction)
        if (
            cursor_initial == (cursor_register, 0x150)
            or (
                cursor_increment == (cursor_register, 4)
                and registers.get(cursor_register) == cursor_marker
            )
        ):
            registers[cursor_register] = cursor_marker
            return
        registers[cursor_register] = ""
        return
    imul_immediate = _cc_receiver_instructions._exact_register_imul_immediate(instruction)
    if imul_immediate is not None:
        destination, source_register, coefficient = imul_immediate
        registers[destination] = _cc_receiver_candidate._bounded_stride_provenance(
            registers.get(source_register, ""),
            coefficient,
        )
        return
    shift_left = _cc_receiver_instructions._exact_register_shift_left_immediate(instruction)
    if shift_left is not None:
        destination, shift = shift_left
        registers[destination] = _cc_receiver_candidate._bounded_stride_provenance(
            registers.get(destination, ""),
            1 << shift,
        )
        return
    indexed_lea = _cc_targets._exact_indexed_affine_load(instruction)
    if (
        indexed_lea is not None
        and indexed_lea[5] == "lea"
        and indexed_lea[4] == 0
        and not (
            indexed_lea[1] == indexed_lea[2]
            and indexed_lea[3] == 2
        )
    ):
        destination, base_register, index_register, scale, _disp, _op = (
            indexed_lea
        )
        combined = _cc_receiver_candidate._combine_bounded_stride_provenance(
            registers.get(base_register, ""),
            registers.get(index_register, ""),
            right_scale=scale,
        )
        if combined:
            registers[destination] = combined
            return
    zero_register = _cc_receiver_proofs._exact_zero_register(instruction)
    if zero_register is not None and zero_register == bounded_loop_counter_register:
        registers[zero_register] = f"bounded-counter({zero_register})"
        return
    inc_register = _cc_receiver_proofs._exact_inc_register(instruction)
    if inc_register is not None and inc_register == bounded_loop_counter_register and registers.get(inc_register) == (
        f"bounded-counter({inc_register})"
    ):
        return
    lea_scale = _cc_receiver_proofs._exact_register_lea_self_scale(instruction)
    if lea_scale is not None:
        destination, source_register, scale = lea_scale
        source_provenance = registers.get(source_register, "")
        registers[destination] = (
            f"index-scale({source_provenance},{scale})"
            if _cc_receiver_proofs._bounded_scalar_index_provenance(source_provenance)
            else ""
        )
        return
    lea_sum = _cc_receiver_proofs._exact_register_lea_sum(instruction)
    if lea_sum is not None:
        destination, left_register, right_register = lea_sum
        left = registers.get(left_register, "")
        right = registers.get(right_register, "")
        registers[destination] = (
            f"index-sum({left},{right})"
            if (
                _cc_receiver_proofs._bounded_scalar_index_provenance(left)
                and _cc_receiver_proofs._bounded_scalar_index_provenance(right)
                and left != right
            )
            else ""
        )
        return
    arithmetic_match = re.fullmatch(
        r"(add|sub)\s+([a-z]{2,3})\s*,\s*(0x[0-9a-f]+|\d+)",
        text,
        flags=re.IGNORECASE,
    )
    if arithmetic_match is not None:
        operation, destination, raw_immediate = arithmetic_match.groups()
        operation = operation.lower()
        destination = destination.lower()
        if _cc_catalog.REGISTER32_RE.fullmatch(destination):
            abstract = registers.get(destination, "")
            immediate = _cc_cfg._parse_unsigned_assembly_integer(raw_immediate)
            if destination == "esp":
                registers[destination] = (
                    "stack"
                    if (
                        abstract == "stack"
                        and _is_exact_safe_stack_root_adjustment(
                            instruction,
                            operation=operation,
                            parsed_immediate=immediate,
                        )
                    )
                    else ""
                )
                return
            displacement = _additive_provenance_displacement(abstract)
            if (
                operation == "add"
                and immediate <= 0x7FFFFFFF
                and -0x80000000
                <= displacement + immediate
                <= 0x7FFFFFFF
                and _eligible_add_pointer_provenance(abstract)
            ):
                registers[destination] = _cc_receiver_instructions._abstract_with_displacement(
                    abstract,
                    immediate,
                )
            elif (
                operation == "add"
                and _bounded_loaded_receiver_pointer(abstract)
                and _cc_receiver_instructions._exact_register_add_immediate(instruction)
                == (destination, immediate)
            ):
                registers[destination] = (
                    f"exact-receiver-add({abstract},+0x{immediate:x})"
                )
            else:
                registers[destination] = ""
            return
        full_register = _cc_catalog.PARTIAL_REGISTER32.get(destination)
        if full_register is not None:
            registers[full_register] = ""
            return
    partial_arithmetic_match = re.match(
        r"(?:add|sub)\s+([a-z]{2,3})\s*,",
        text,
        flags=re.IGNORECASE,
    )
    if partial_arithmetic_match is not None:
        full_register = _cc_catalog.PARTIAL_REGISTER32.get(
            partial_arithmetic_match.group(1).lower()
        )
        if full_register is not None:
            registers[full_register] = ""
            return
    register_add = _cc_receiver_proofs._exact_register_add_register(instruction)
    if register_add is not None:
        destination, source_register = register_add
        destination_value = registers.get(destination, "")
        source_value = registers.get(source_register, "")
        combined_stride = _cc_receiver_candidate._combine_bounded_stride_provenance(
            destination_value,
            source_value,
        )
        if combined_stride and destination != source_register:
            registers[destination] = combined_stride
        elif (
            _cc_receiver_candidate._bounded_targetless_pointer_base(destination_value)
            and _cc_receiver_proofs._bounded_index_provenance(source_value)
        ):
            registers[destination] = (
                f"address(affine({destination_value},{source_value}*1))"
            )
        elif (
            _cc_receiver_proofs._bounded_index_provenance(destination_value)
            and _cc_receiver_candidate._bounded_targetless_pointer_base(source_value)
        ):
            registers[destination] = (
                f"address(affine({source_value},{destination_value}*1))"
            )
        else:
            registers[destination] = (
                f"index-sum({destination_value},{source_value})"
                if (
                    destination != source_register
                    and destination_value != source_value
                    and _cc_receiver_proofs._bounded_scalar_index_provenance(destination_value)
                    and _cc_receiver_proofs._bounded_scalar_index_provenance(source_value)
                )
                else ""
            )
        return
    operands = [
        item.strip().lower()
        for item in _cc_cfg._instruction_operand(instruction).split(",")
    ]
    mnemonic = _cc_cfg._instruction_mnemonic(instruction)
    if mnemonic in {"enter", "leave"}:
        registers["esp"] = ""
        return
    if mnemonic == "xchg":
        for operand in operands:
            full_register = _cc_catalog.PARTIAL_REGISTER32.get(operand, operand)
            if _cc_catalog.REGISTER32_RE.fullmatch(full_register):
                registers[full_register] = ""
        return
    if (
        mnemonic == "xor"
        and len(operands) == 2
        and operands[0] == operands[1]
        and _cc_catalog.REGISTER32_RE.fullmatch(operands[0])
        and _cc_receiver_cursor._is_exact_self_xor_zero(
            instruction,
            destination=operands[0],
        )
    ):
        registers[operands[0]] = "null"
        return
    match = re.match(
        r"^(mov|lea)\s+([a-z]{2,3})\s*,\s*(.+)$",
        text,
        flags=re.IGNORECASE,
    )
    if match is None or not _cc_catalog.REGISTER_RE.fullmatch(match.group(2)):
        # Unknown transfer semantics cannot preserve a pointer through a
        # write. Include partial MOV destinations (DL/AH/etc.) and implicit
        # outputs such as CDQ, which have no full-register first operand.
        try:
            effect = _cc_instructions.instruction_fact(instruction)
            stack_operation = (
                effect.mnemonic in {"push", "pop"}
                and len(effect.operands) == 1
                and effect.operands[0].size == 4
                and not (effect.mnemonic == "pop" and effect.operands[0].register == "esp")
            )
        except _cc_instructions.InstructionProofError:
            stack_operation = False
        for destination in _cc_catalog.REGISTER_STATE_NAMES:
            # This interpreter's stack marker denotes the current stack root,
            # not a fixed entry-ESP value. Exact 32-bit pushes/pops preserve
            # that root; POP ESP and other unmodelled writes still kill it.
            # Entry-slot equality is established by the separate CFG proof.
            if destination == "esp" and registers.get("esp") == "stack" and stack_operation:
                continue
            if _cc_cfg._instruction_may_clobber_register(instruction, destination):
                registers[destination] = ""
        return
    operation, destination, source = match.groups()
    operation = operation.lower()
    destination = destination.lower()
    full_destination = _cc_catalog.PARTIAL_REGISTER32.get(destination, destination)
    if full_destination == "esp" or full_destination != destination:
        registers[full_destination] = ""
        return
    source = re.sub(
        r"^(?:byte|word|dword)\s+(?:ptr\s+)?",
        "",
        source,
        flags=re.IGNORECASE,
    ).strip()
    if _cc_catalog.REGISTER_RE.fullmatch(source):
        source_register = source.lower()
        registers[destination] = (
            registers.get(source_register, "") if operation == "mov" else ""
        )
        return
    memory, displacement = _cc_targets._memory_slot(source)
    exact_memory = _cc_targets._exact_memory_expression(source)
    raw_instruction_address = (
        reviewed_instruction_address
        or _cc_cfg._source_instruction_address(instruction)
    )
    instruction_address = (
        normalize_address(raw_instruction_address)
        if raw_instruction_address
        else ""
    )
    absolute_storage_load = (
        reviewed_absolute_storage_load_bridges or {}
    ).get(instruction_address)
    if absolute_storage_load is not None:
        expected_expressions = {
            (
                f"{absolute_storage_load.aggregate_symbol}"
                f"+{absolute_storage_load.displacement}"
            ),
            (
                f"{absolute_storage_load.aggregate_symbol}"
                f"+0x{absolute_storage_load.displacement:x}"
            ),
        }
        if absolute_storage_load.displacement == 0:
            expected_expressions.add(
                absolute_storage_load.aggregate_symbol
            )
        consumed = consumed_absolute_storage_load_bridges
        if (
            assembly_source != "cod"
            or operation != "mov"
            or destination != absolute_storage_load.register
            or exact_memory not in expected_expressions
            or absolute_storage_load.access_width != 4
            or not absolute_storage_load.storage_identity.startswith(
                "storage:recoil:data:"
            )
            or consumed is None
            or instruction_address in consumed
        ):
            raise ValueError(
                "reviewed absolute-storage load bridge does not match unique "
                f"candidate load at {instruction_address or '<no-offset>'}"
            )
        provenance = (
            "load("
            f"{_cc_receiver_instructions._abstract_with_displacement(absolute_storage_load.storage_identity, absolute_storage_load.displacement)}"
            ")"
        )
        registers[destination] = (
            f"exact-load({absolute_storage_load.register},{provenance})"
        )
        consumed.add(instruction_address)
        return
    member_vptr_load = (
        reviewed_member_vptr_load_bridges or {}
    ).get(instruction_address)
    if member_vptr_load is not None:
        consumed = consumed_member_vptr_load_bridges
        if (
            assembly_source != "cod"
            or operation != "mov"
            or destination != member_vptr_load.register
            or exact_memory != member_vptr_load.source_register
            or registers.get(member_vptr_load.source_register, "")
            != member_vptr_load.source_provenance
            or consumed is None
            or instruction_address in consumed
        ):
            raise ValueError(
                "reviewed member-vptr load bridge does not match unique "
                f"candidate load at {instruction_address or '<no-offset>'}"
            )
        registers[destination] = (
            f"exact-member-vptr({member_vptr_load.register},"
            f"{member_vptr_load.storage_identity})"
        )
        consumed.add(instruction_address)
        return
    static_storage_reference = (
        reviewed_static_storage_reference_bridges or {}
    ).get(exact_memory)
    if static_storage_reference is not None:
        expected_expressions = {
            (
                f"{static_storage_reference.aggregate_symbol}"
                f"+{static_storage_reference.displacement}"
            ),
            (
                f"{static_storage_reference.aggregate_symbol}"
                f"+0x{static_storage_reference.displacement:x}"
            ),
        }
        if (
            assembly_source != "cod"
            or operation != "mov"
            or exact_memory not in expected_expressions
            or static_storage_reference.access_width != 4
            or not static_storage_reference.storage_identity.startswith(
                "storage:recoil:data:"
            )
        ):
            raise ValueError(
                "reviewed static-storage reference bridge does not match "
                f"candidate load {exact_memory!r}"
            )
        registers[destination] = (
            static_storage_reference.storage_identity
        )
        return
    encoded_stack_displacement = (
        _cc_receiver_instructions._encoded_stack_slot_load_displacement(
            instruction,
            parsed_destination=destination,
            parsed_source=source,
        )
        if operation == "mov"
        else None
    )
    if encoded_stack_displacement is not None:
        stack = registers.get("esp", "")
        stack_root = (
            "entry-stack"
            if entry_stack_root_allowed and stack == "stack"
            else stack
        )
        registers[destination] = (
            bounded_local_stack_provenance
            or (
                f"load({_cc_receiver_instructions._abstract_with_displacement(stack_root, encoded_stack_displacement)})"
                if stack_root in {"stack", "entry-stack"}
                else ""
            )
        )
        return
    indexed_affine = _cc_targets._exact_indexed_affine_load(instruction)
    if indexed_affine is not None:
        (
            indexed_destination,
            base_register,
            index_register,
            scale,
            indexed_displacement,
            indexed_operation,
        ) = indexed_affine
        base = registers.get(base_register, "")
        index_provenance = registers.get(index_register, "")
        if (
            indexed_destination != destination
            or indexed_operation != operation
            or not base
            or not _cc_receiver_proofs._bounded_index_provenance(index_provenance)
            or base in {"frame", "stack"}
            or index_provenance in {"frame", "stack"}
        ):
            registers[destination] = ""
            return
        affine = (
            f"affine({base},{index_provenance}*{scale}"
            + (
                ""
                if indexed_displacement == 0
                else (
                    ",+" if indexed_displacement > 0 else ",-"
                )
                + f"0x{abs(indexed_displacement):x}"
            )
            + ")"
        )
        registers[destination] = (
            f"address({affine})"
            if operation == "lea"
            else f"load({affine})"
        )
        return
    memory_registers = re.findall(
        r"\b(e?[abcd]x|e?[sd]i|e?[sb]p)\b",
        memory,
    )
    if "*" in memory or len(set(memory_registers)) > 1:
        # An indexed/SIB load that did not pass the exact decoder must not be
        # simplified to its first register and last numeric term.
        registers[destination] = ""
        return
    base_match = re.search(r"\b(e?[abcd]x|e?[sd]i|e?[sb]p)\b", memory)
    if base_match is not None:
        base_register = base_match.group(1).lower()
        if base_register == "esp":
            registers[destination] = ""
            return
        base = registers.get(base_register, "")
        if not base:
            registers[destination] = ""
            return
        # A positive EBP entry argument is distinct from an arbitrary frame
        # local.  This bounded root lets exact branch reloads compose with the
        # ordinary receiver/vptr equality rule while negative/local offsets
        # and unknown frame arithmetic remain unresolved.
        if (
            base == "frame"
            and operation == "mov"
            and isinstance(displacement, int)
            and displacement >= 8
        ):
            expression = _cc_receiver_instructions._abstract_with_displacement(
                "entry-stack",
                displacement,
            )
        else:
            expression = _cc_receiver_instructions._abstract_with_displacement(base, displacement)
        if (
            operation == "mov"
            and displacement == 0
            and base.startswith("address(affine(")
            and base.endswith(")")
        ):
            # Dereferencing a fully proven affine address is the exact loaded
            # receiver, not a second opaque address layer.
            expression = base[len("address(") : -1]
        elif (
            operation == "mov"
            and displacement == 0
            and base.startswith(
                f"address({_cc_catalog.HUD_SHIELD_LAYOUT_POINTER_IDENTITY}"
            )
            and base.endswith(")")
        ):
            # The exact 0x40eb00 bridge proves that this COD LEA is merely an
            # address-form spelling of the same aggregate subobject that
            # retail reaches with ADD.  Normalize only that reviewed affine
            # root; arbitrary address/load provenance remains unchanged.
            expression = base[len("address(") : -1]
        registers[destination] = (
            f"address({expression})"
            if operation == "lea"
            else f"load({expression})"
        )
        return
    addresses = _cc_catalog.ADDRESS_RE.findall(memory)
    if addresses:
        address = normalize_address(addresses[-1])
        indexed_identity = indexes.storage_by_address.get(address, "")
        identity = indexed_identity
        reviewed_identity = reviewed_register_storage_bridges.get(address, "")
        if identity and reviewed_identity and identity != reviewed_identity:
            raise ValueError(
                f"conflicting reviewed register storage identity at {address}"
            )
        identity = identity or reviewed_identity
        if identity:
            registers[destination] = (
                _exact_iat_register_load_provenance(
                    instruction,
                    assembly_source=assembly_source,
                    destination=destination,
                    exact_memory=exact_memory,
                    rendered_address=address,
                    identity=identity,
                    indexes=indexes,
                    instruction_address=instruction_address,
                    reviewed_identity=(
                        not indexed_identity and bool(reviewed_identity)
                    ),
                )
                if operation == "mov" and identity.startswith("iat:")
                else identity if operation == "mov" else ""
            )
            return
        value = address_value(address)
        containers = [
            row
            for row in indexes.storage_containers
            if row.start <= value and value + 4 <= row.end_exclusive
        ]
        if len(containers) > 1:
            raise ValueError(
                f"ambiguous register storage containers at {address}"
            )
        registers[destination] = (
            "load("
            f"{_cc_receiver_instructions._abstract_with_displacement(containers[0].identity, value - containers[0].start)}"
            ")"
            if operation == "mov" and len(containers) == 1
            else ""
        )
        return
    name_displacement_match = re.search(
        r"(?P<sign>[+-])(?P<value>0x[0-9a-fA-F]+|\d+)$",
        exact_memory,
    )
    name_expression = (
        exact_memory[: name_displacement_match.start()]
        if name_displacement_match is not None
        else exact_memory
    )
    name_displacement = (
        (
            -1 if name_displacement_match.group("sign") == "-" else 1
        )
        * _cc_cfg._parse_unsigned_assembly_integer(name_displacement_match.group("value"))
        if name_displacement_match is not None
        else None
    )
    indexed_identity = (
        _candidate_c_external_storage_identity(
            name_expression,
            indexes=indexes,
        )
        if assembly_source == "cod"
        else indexes.storage_by_name.get(name_expression, "")
    )
    if assembly_source == "cod" and not indexed_identity:
        # Expected identity is retail/import-owned.  A candidate ``__imp_``
        # spelling is only a checked consumer of that route and therefore may
        # project to the already-published import name without publishing any
        # new expected fact.
        candidate_import_name = _cc_receiver_instructions._candidate_iat_import_name(name_expression)
        if candidate_import_name:
            indexed_identity = indexes.storage_by_name.get(
                candidate_import_name,
                "",
            )
    identity = indexed_identity
    reviewed_identity = reviewed_register_storage_bridges.get(
        name_expression,
        "",
    )
    if identity and reviewed_identity and identity != reviewed_identity:
        raise ValueError(
            f"conflicting reviewed register storage identity for {name_expression!r}"
        )
    identity = identity or reviewed_identity
    if not identity:
        decorated = _cc_catalog.DECORATED_RE.search(exact_memory)
        if decorated is not None:
            name = decorated.group(0)
            identity = indexes.storage_by_name.get(name, "")
            reviewed_identity = reviewed_register_storage_bridges.get(name, "")
            if identity and reviewed_identity and identity != reviewed_identity:
                raise ValueError(
                    f"conflicting reviewed register storage identity for {name!r}"
                )
            identity = identity or reviewed_identity
    if identity:
        if name_displacement is None:
            if operation == "mov" and identity.startswith("iat:"):
                candidate_proof = candidate_iat_proofs.get(instruction_address)
                registers[destination] = _exact_iat_register_load_provenance(
                    instruction,
                    assembly_source=assembly_source,
                    destination=destination,
                    exact_memory=exact_memory,
                    rendered_address=None,
                    identity=identity,
                    indexes=indexes,
                    instruction_address=instruction_address,
                    reviewed_identity=(
                        not indexed_identity and bool(reviewed_identity)
                    ),
                    candidate_proof=candidate_proof,
                )
                if candidate_proof is not None:
                    if instruction_address in consumed_candidate_iat_proofs:
                        raise ValueError(
                            "candidate exact-IAT register-load proof was consumed twice"
                        )
                    consumed_candidate_iat_proofs.add(instruction_address)
            else:
                registers[destination] = identity if operation == "mov" else ""
            return
        containers = [
            row for row in indexes.storage_containers if row.identity == identity
        ]
        if len(containers) > 1:
            raise ValueError(
                f"ambiguous register storage containers for {name_expression!r}"
            )
        registers[destination] = (
            f"load({_cc_receiver_instructions._abstract_with_displacement(identity, name_displacement)})"
            if (
                operation == "mov"
                and len(containers) == 1
                and name_displacement >= 0
                and containers[0].start + name_displacement + 4
                <= containers[0].end_exclusive
            )
            else ""
        )
        return
    registers[destination] = ""
