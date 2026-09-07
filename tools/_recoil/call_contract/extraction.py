"""Recoil call-contract extraction evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import dispatch as _cc_dispatch
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import listing as _cc_listing
from _recoil.call_contract import receiver_candidate as _cc_receiver_candidate
from _recoil.call_contract import receiver_cursor as _cc_receiver_cursor
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions
from _recoil.call_contract import receiver_proofs as _cc_receiver_proofs
from _recoil.call_contract import receiver_storage as _cc_receiver_storage
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateCallerDefinition,
        CandidateExactIatRegisterLoadProof,
        IdentityIndexes,
        RetailCallerScopedProofPackage,
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedCallResultBridge,
        ReviewedExactIndirectStorageBridge,
        ReviewedInboundEntryRegisterTargetBridge,
        ReviewedLoopVptrStorageBridge,
        ReviewedMemberVptrStorageBridge,
        ReviewedRegisterCallStorageBridge,
        ReviewedStaticStorageReferenceBridge,
        ReviewedVptrStorageBridge,
    )

import re
import struct
from dataclasses import replace
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import Instruction
from _recoil.lib.progress import address_value, normalize_address


def extract_invocation_contract(
    instructions: Sequence[Instruction],
    *,
    source: str,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any] | None = None,
    compiler_generated_bridges: Mapping[str, str] | None = None,
    candidate_storage_bridges: Mapping[str, str] | None = None,
    reviewed_register_storage_bridges: Mapping[str, str] | None = None,
    reviewed_register_call_storage_bridges: (
        Mapping[str, ReviewedRegisterCallStorageBridge] | None
    ) = None,
    reviewed_iat_register_definition_offsets: frozenset[str] = frozenset(),
    candidate_exact_iat_register_load_proofs: (
        Mapping[str, CandidateExactIatRegisterLoadProof] | None
    ) = None,
    reviewed_static_storage_reference_bridges: (
        Mapping[str, ReviewedStaticStorageReferenceBridge] | None
    ) = None,
    reviewed_absolute_storage_load_bridges: (
        Mapping[str, ReviewedAbsoluteStorageLoadBridge] | None
    ) = None,
    reviewed_member_vptr_storage_bridges: (
        Mapping[str, ReviewedMemberVptrStorageBridge] | None
    ) = None,
    reviewed_call_result_bridges: (
        Mapping[str, ReviewedCallResultBridge] | None
    ) = None,
    reviewed_vptr_storage_bridges: (
        Mapping[str, ReviewedVptrStorageBridge] | None
    ) = None,
    reviewed_loop_vptr_storage_bridges: (
        Mapping[str, ReviewedLoopVptrStorageBridge] | None
    ) = None,
    reviewed_exact_indirect_storage_bridges: (
        Mapping[str, ReviewedExactIndirectStorageBridge] | None
    ) = None,
    reviewed_inbound_entry_register_target_bridges: (
        Mapping[int, ReviewedInboundEntryRegisterTargetBridge] | None
    ) = None,
    reviewed_inbound_entry_register_roots: frozenset[str] = frozenset(),
    local_control_flow_indices: frozenset[int] = frozenset(),
    local_control_flow_targets: Mapping[int, tuple[int, ...]] | None = None,
    reviewed_direct_call_cleanup_by_instruction_index: (
        Mapping[int, int] | None
    ) = None,
    retail_proof_package: RetailCallerScopedProofPackage | None = None,
    invocation_call_sites_out: list[str] | None = None,
    reviewed_retail_call_sites_by_ordinal: Sequence[str] | None = None,
    candidate_caller_definition: CandidateCallerDefinition | None = None,
    candidate_classification_only_local_control_flow_targets: (
        Mapping[int, tuple[int, ...]] | None
    ) = None,
) -> list[dict[str, Any]]:
    """Extract static ordered calls with exact forward-CFG register provenance."""
    from _recoil.call_contract.records import CandidateExactIatRegisterLoadProof
    start = address_value(caller_start)
    end = address_value(caller_end_exclusive)
    if (
        reviewed_retail_call_sites_by_ordinal is not None
        and source != "cod"
    ):
        raise ValueError(
            "reviewed retail call-site ordinal authority is candidate-only"
        )
    if candidate_caller_definition is not None and (
        source != "cod" or invocation_call_sites_out is None
    ):
        raise ValueError(
            "candidate caller definition is limited to candidate invocation "
            "call-site enumeration"
        )
    indexed_retail_proof_package = (
        indexes.active_retail_caller_proof_package
        if source == "bn"
        else None
    )
    if retail_proof_package is None and indexed_retail_proof_package is not None:
        identical_full_caller = (
            len(indexed_retail_proof_package.instruction_objects)
            == len(instructions)
            and all(
                current is packaged
                for current, packaged in zip(
                    instructions,
                    indexed_retail_proof_package.instruction_objects,
                )
            )
        )
        if identical_full_caller:
            retail_proof_package = indexed_retail_proof_package
        else:
            if len(indexed_retail_proof_package.instruction_objects) == len(
                instructions
            ):
                raise ValueError(
                    "caller-scoped retail proof package does not rebind to the identical caller instructions"
                )
            packaged_positions = {
                id(instruction): index
                for index, instruction in enumerate(
                    indexed_retail_proof_package.instruction_objects
                )
            }
            identity_subset_positions = tuple(
                packaged_positions.get(id(instruction), -1)
                for instruction in instructions
            )
            if all(position >= 0 for position in identity_subset_positions):
                subset_positions = identity_subset_positions
            else:
                # A reviewed bridge may ask Binary Ninja for one bounded
                # generated body nested in the active physical caller.  That
                # render necessarily creates new ``Instruction`` objects, so
                # object identity cannot establish the subset relationship.
                # Rebind only through the immutable BN runtime coordinate and
                # the exact encoding/rendering already present in the caller
                # package.  Every coordinate must occur exactly once; a
                # duplicate package row, foreign render, or byte/text drift is
                # therefore a collision instead of an inferred match.
                package_rows = (
                    indexed_retail_proof_package.instruction_objects
                )
                package_addresses = _cc_cfg._instruction_runtime_addresses(
                    package_rows,
                    source="bn",
                    caller_start=address_value(
                        indexed_retail_proof_package.caller_start
                    ),
                )
                subset_addresses = _cc_cfg._instruction_runtime_addresses(
                    instructions,
                    source="bn",
                    caller_start=start,
                )
                package_positions_by_address: dict[int, list[int]] = {}
                for package_index, package_address in enumerate(
                    package_addresses
                ):
                    if package_address is not None:
                        package_positions_by_address.setdefault(
                            package_address, []
                        ).append(package_index)
                coordinate_positions: list[int] = []
                for instruction, instruction_address in zip(
                    instructions, subset_addresses
                ):
                    matches = (
                        package_positions_by_address.get(
                            instruction_address, []
                        )
                        if instruction_address is not None
                        else []
                    )
                    if len(matches) != 1:
                        coordinate_positions.append(-1)
                        continue
                    package_instruction = package_rows[matches[0]]
                    if (
                        tuple(item.casefold() for item in instruction.bytes)
                        != tuple(
                            item.casefold()
                            for item in package_instruction.bytes
                        )
                        or instruction.text != package_instruction.text
                        or instruction.raw_text
                        != package_instruction.raw_text
                    ):
                        coordinate_positions.append(-1)
                        continue
                    coordinate_positions.append(matches[0])
                subset_positions = tuple(coordinate_positions)
            if (
                any(position < 0 for position in subset_positions)
                or tuple(sorted(set(subset_positions))) != subset_positions
            ):
                raise ValueError(
                    "caller-scoped retail proof package subset does not rebind to exact ordered caller instructions"
                )
            # Some narrow reviewed bridges deliberately extract a prefix or a
            # selected call pair.  A whole-caller proof must never be rebound
            # to that subset.  Remove the caller-global transfer facts so the
            # subset remains owned solely by its explicit bridge evidence.
            indexes = replace(
                indexes,
                reviewed_iat_register_joins=frozenset(),
                reviewed_retail_iat_load_proofs={},
                active_retail_caller_proof_package=None,
            )
    elif (
        indexed_retail_proof_package is not None
        and retail_proof_package is not indexed_retail_proof_package
    ):
        raise ValueError(
            "explicit retail proof package conflicts with the active caller package"
        )
    packaged_iat_objects: dict[
        str, tuple[Instruction, tuple[Instruction, ...]]
    ] = {}
    packaged_targetless_vptr_proofs: dict[int, str] | None = None
    if retail_proof_package is not None:
        if (
            source != "bn"
            or retail_proof_package.caller_identity != caller_identity
            or retail_proof_package.caller_start
            != normalize_address(caller_start)
            or retail_proof_package.caller_end_exclusive
            != normalize_address(caller_end_exclusive)
            or len(retail_proof_package.instruction_objects)
            != len(instructions)
            or any(
                current is not packaged
                for current, packaged in zip(
                    instructions,
                    retail_proof_package.instruction_objects,
                )
            )
        ):
            raise ValueError(
                "caller-scoped retail proof package does not rebind to the identical caller instructions"
            )
        packaged_iat_objects = {
            address: (definition, transfers)
            for address, definition, transfers in (
                retail_proof_package.iat_instruction_objects
            )
        }
        if len(packaged_iat_objects) != len(
            retail_proof_package.iat_instruction_objects
        ):
            raise ValueError(
                "caller-scoped retail proof package has colliding IAT definitions"
            )
        packaged_cleanup = dict(
            retail_proof_package.direct_call_cleanup_by_instruction_index
        )
        if (
            reviewed_direct_call_cleanup_by_instruction_index is not None
            and dict(reviewed_direct_call_cleanup_by_instruction_index)
            != packaged_cleanup
        ):
            raise ValueError(
                "caller-scoped retail proof package conflicts with direct cleanup facts"
            )
        reviewed_direct_call_cleanup_by_instruction_index = packaged_cleanup
        packaged_switch_targets = dict(
            retail_proof_package.local_control_flow_targets
        )
        if (
            local_control_flow_targets is not None
            and dict(local_control_flow_targets) != packaged_switch_targets
        ):
            raise ValueError(
                "caller-scoped retail proof package conflicts with local CFG facts"
            )
        local_control_flow_targets = packaged_switch_targets
        local_control_flow_indices = frozenset(packaged_switch_targets)
        packaged_targetless_vptr_proofs = dict(
            retail_proof_package.targetless_vptr_call_proofs
        )
    names = bridge_names or {}
    compiler_bridges = compiler_generated_bridges or {}
    storage_bridges = candidate_storage_bridges or {}
    register_storage_bridges = reviewed_register_storage_bridges or {}
    iat_register_definition_offsets = frozenset(
        normalize_address(address)
        for address in reviewed_iat_register_definition_offsets
    )
    candidate_iat_load_proofs: dict[
        str, CandidateExactIatRegisterLoadProof
    ] = {}
    retail_iat_load_proofs = (
        dict(indexes.reviewed_retail_iat_load_proofs)
        if source == "bn"
        else {}
    )
    if source != "bn" and indexes.reviewed_retail_iat_load_proofs:
        # Candidate extraction may share the comparison-scoped indexes, but
        # immutable-retail transfer proofs are never candidate authority.
        retail_iat_load_proofs = {}
    for definition_address, proof in retail_iat_load_proofs.items():
        if (
            not isinstance(proof, CandidateExactIatRegisterLoadProof)
            or normalize_address(definition_address) != definition_address
            or proof.definition_offset != definition_address
            or not proof.identity.startswith("iat:")
            or not proof.transfer_offsets
            or type(proof.definition_instruction_index) is not int
            or proof.definition_instruction_index < 0
            or len(proof.transfer_instruction_indices)
            != len(proof.transfer_offsets)
            or any(
                type(instruction_index) is not int
                or instruction_index < 0
                for instruction_index in proof.transfer_instruction_indices
            )
            or proof.definition_instruction_index
            in proof.transfer_instruction_indices
            or type(proof.definition_body_offset) is not int
            or proof.definition_body_offset < 0
            or len(proof.transfer_body_offsets)
            != len(proof.transfer_offsets)
            or any(
                type(body_offset) is not int or body_offset < 0
                for body_offset in proof.transfer_body_offsets
            )
            or proof.definition_body_offset in proof.transfer_body_offsets
        ):
            raise ValueError("retail exact-IAT register-load proof is malformed")
    consumed_retail_iat_transfers: set[tuple[str, str]] = set()
    for raw_address, proof in (
        candidate_exact_iat_register_load_proofs or {}
    ).items():
        address = normalize_address(raw_address)
        if (
            source != "cod"
            or not isinstance(proof, CandidateExactIatRegisterLoadProof)
            or proof.definition_offset != address
            or not proof.transfer_offsets
            or proof.definition_instruction_index is not None
            or proof.transfer_instruction_indices
            or proof.definition_body_offset is not None
            or proof.transfer_body_offsets
            or tuple(
                sorted(
                    {
                        normalize_address(offset)
                        for offset in proof.transfer_offsets
                    },
                    key=address_value,
                )
            )
            != proof.transfer_offsets
            or address in candidate_iat_load_proofs
        ):
            raise ValueError(
                "candidate exact-IAT register-load proof map is malformed"
            )
        candidate_iat_load_proofs[address] = proof
    consumed_candidate_iat_load_proofs: set[str] = set()
    consumed_candidate_iat_transfers: set[tuple[str, str]] = set()
    register_call_storage_bridges = {
        normalize_address(address): bridge
        for address, bridge in (
            reviewed_register_call_storage_bridges or {}
        ).items()
    }
    consumed_register_call_storage_bridges: set[str] = set()
    if register_call_storage_bridges and any(
        bridge.assembly_source != source
        for bridge in register_call_storage_bridges.values()
    ):
        raise ValueError(
            "reviewed register-call storage bridges belong to another "
            "assembly source"
        )
    static_storage_reference_bridges = (
        reviewed_static_storage_reference_bridges or {}
    )
    absolute_storage_load_bridges: dict[
        str, ReviewedAbsoluteStorageLoadBridge
    ] = {}
    for raw_address, absolute_bridge in (
        reviewed_absolute_storage_load_bridges or {}
    ).items():
        address = normalize_address(raw_address)
        if (
            address in absolute_storage_load_bridges
            and absolute_storage_load_bridges[address] != absolute_bridge
        ):
            raise ValueError(
                "conflicting reviewed absolute-storage load bridges at "
                f"{address}"
            )
        absolute_storage_load_bridges[address] = absolute_bridge
    consumed_absolute_storage_load_bridges: set[str] = set()
    natural_nested_absolute_loads = frozenset(
        (
            "exact-load("
            f"{bridge.register},"
            "load("
            f"{_cc_receiver_instructions._abstract_with_displacement(bridge.storage_identity, bridge.displacement)}"
            "))"
        )
        for bridge in absolute_storage_load_bridges.values()
        if bridge.canonicalize_nested_load
    )
    member_vptr_bridge_indexes = (
        _cc_dispatch._index_reviewed_member_vptr_storage_bridges(
            reviewed_member_vptr_storage_bridges or {}
        )
    )
    member_vptr_load_bridges = member_vptr_bridge_indexes.by_load_address
    member_vptr_call_bridges = member_vptr_bridge_indexes.by_call_address
    consumed_member_vptr_load_bridges: set[str] = set()
    consumed_member_vptr_call_bridges: set[str] = set()
    call_result_bridges = {
        normalize_address(address): bridge
        for address, bridge in (reviewed_call_result_bridges or {}).items()
    }
    consumed_call_result_bridges: set[str] = set()
    vptr_storage_bridges = {
        normalize_address(address): bridge
        for address, bridge in (reviewed_vptr_storage_bridges or {}).items()
    }
    consumed_vptr_storage_bridges: set[str] = set()
    loop_vptr_storage_bridges = {
        normalize_address(address): bridge
        for address, bridge in (
            reviewed_loop_vptr_storage_bridges or {}
        ).items()
    }
    consumed_loop_vptr_storage_bridges: set[str] = set()
    exact_indirect_storage_bridges = {
        normalize_address(address): bridge
        for address, bridge in (
            reviewed_exact_indirect_storage_bridges or {}
        ).items()
    }
    consumed_exact_indirect_storage_bridges: set[str] = set()
    inbound_entry_target_bridges = dict(
        reviewed_inbound_entry_register_target_bridges or {}
    )
    if any(
        type(ordinal) is not int
        or ordinal < 0
        or bridge.ordinal != ordinal
        for ordinal, bridge in inbound_entry_target_bridges.items()
    ):
        raise ValueError(
            "reviewed BN inbound entry-register target bridge ordinals are malformed"
        )
    consumed_inbound_entry_target_bridges: set[int] = set()
    inbound_entry_roots = frozenset(reviewed_inbound_entry_register_roots)
    if not inbound_entry_roots.issubset({"ecx", "edx"}):
        raise ValueError("reviewed inbound entry-register roots are malformed")
    switch_targets = dict(local_control_flow_targets or {})
    direct_call_cleanup_by_instruction_index = dict(
        reviewed_direct_call_cleanup_by_instruction_index or {}
    )
    if direct_call_cleanup_by_instruction_index and source != "bn":
        raise ValueError("reviewed direct-callee cleanup is retail-only")
    if not set(switch_targets).issubset(local_control_flow_indices):
        raise ValueError(
            "local control-flow target mappings require matching dispatch indices"
        )
    if compiler_bridges and source != "cod":
        raise ValueError("compiler-generated call bridges are candidate-only")
    if iat_register_definition_offsets and source != "cod":
        raise ValueError(
            "reviewed IAT register-definition offsets are candidate-only"
        )
    if storage_bridges and source != "cod":
        raise ValueError("candidate storage bridges are candidate-only")
    if static_storage_reference_bridges and source != "cod":
        raise ValueError(
            "reviewed static-storage reference bridges are candidate-only"
        )
    if absolute_storage_load_bridges and source != "cod":
        raise ValueError(
            "reviewed absolute-storage load bridges are candidate-only"
        )
    if member_vptr_load_bridges and source != "cod":
        raise ValueError(
            "reviewed member-vptr storage bridges are candidate-only"
        )
    if exact_indirect_storage_bridges and any(
        bridge.assembly_source != source
        for bridge in exact_indirect_storage_bridges.values()
    ):
        raise ValueError(
            "reviewed exact indirect storage bridges require their exact "
            "declared assembly source"
        )
    if vptr_storage_bridges and source != "cod":
        raise ValueError(
            "reviewed exact-callsite vptr storage bridges are candidate-only"
        )
    instruction_addresses = _cc_cfg._instruction_runtime_addresses(
        instructions,
        source=source,
        caller_start=start,
    )
    local_flow_end = end
    if source == "cod":
        local_flow_end = max(
            end,
            max(
                (
                    address + max(1, len(instruction.bytes))
                    for instruction, address in zip(
                        instructions, instruction_addresses
                    )
                    if address is not None
                ),
                default=end,
            ),
        )
    address_counts: dict[int, int] = {}
    for address in instruction_addresses:
        if address is not None and start <= address < local_flow_end:
            address_counts[address] = address_counts.get(address, 0) + 1
    instruction_index_by_address = {
        address: index
        for index, address in enumerate(instruction_addresses)
        if (
            address is not None
            and start <= address < local_flow_end
            and address_counts.get(address) == 1
        )
    }
    retail_iat_proofs_by_instruction_index: dict[
        int, list[CandidateExactIatRegisterLoadProof]
    ] = {}
    if source == "bn":
        for proof in retail_iat_load_proofs.values():
            definition_index = int(proof.definition_instruction_index)
            definition_runtime_address = (
                start + int(proof.definition_body_offset)
            )
            packaged_objects = packaged_iat_objects.get(proof.definition_offset)
            package_rejoined = (
                packaged_objects is not None
                and definition_index < len(instructions)
                and instructions[definition_index] is packaged_objects[0]
                and definition_runtime_address
                == address_value(proof.definition_offset)
            )
            if not package_rejoined and (
                definition_index >= len(instructions)
                or instruction_addresses[definition_index]
                != definition_runtime_address
                or definition_runtime_address
                != address_value(proof.definition_offset)
                or instruction_index_by_address.get(definition_runtime_address)
                != definition_index
            ):
                raise ValueError(
                    "retail exact-IAT definition does not rejoin its distinct "
                    f"exact instruction at {proof.definition_offset}"
                )
            definition_instruction = instructions[definition_index]
            try:
                definition_body = bytes(
                    int(item, 16) for item in definition_instruction.bytes
                )
            except (TypeError, ValueError):
                definition_body = b""
            definition_operands = _cc_cfg._instruction_operand(
                definition_instruction
            ).split(",", 1)
            register_names = (
                "eax", "ecx", "edx", "ebx",
                "esp", "ebp", "esi", "edi",
            )
            if (
                len(definition_body) != 6
                or definition_body[0] != 0x8B
                or definition_body[1] >> 6 != 0
                or definition_body[1] & 7 != 5
                or register_names[(definition_body[1] >> 3) & 7]
                != proof.destination
                or _cc_cfg._instruction_mnemonic(definition_instruction) != "mov"
                or len(definition_operands) != 2
                or definition_operands[0].strip().lower()
                != proof.destination
            ):
                raise ValueError(
                    "retail exact-IAT definition does not rejoin its exact "
                    f"absolute MOV encoding at {proof.definition_offset}"
                )
            for transfer_address, transfer_index, transfer_body_offset in zip(
                proof.transfer_offsets,
                proof.transfer_instruction_indices,
                proof.transfer_body_offsets,
            ):
                transfer_runtime_address = start + transfer_body_offset
                packaged_transfer_rejoined = (
                    packaged_objects is not None
                    and transfer_index < len(instructions)
                    and len(packaged_objects[1])
                    == len(proof.transfer_instruction_indices)
                    and instructions[transfer_index]
                    is packaged_objects[1][
                        proof.transfer_instruction_indices.index(transfer_index)
                    ]
                    and transfer_runtime_address
                    == address_value(transfer_address)
                )
                if not packaged_transfer_rejoined and (
                    transfer_index >= len(instructions)
                    or instruction_addresses[transfer_index]
                    != transfer_runtime_address
                    or transfer_runtime_address
                    != address_value(transfer_address)
                    or transfer_index == definition_index
                ):
                    raise ValueError(
                        "retail exact-IAT transfer is absent from the exact "
                        f"caller instruction population at {transfer_address}"
                    )
                transfer_instruction = instructions[transfer_index]
                transfer_operand = re.sub(
                    r"^(?:near|far)\s+(?:ptr\s+)?",
                    "",
                    _cc_cfg._instruction_operand(transfer_instruction).strip().lower(),
                )
                transfer_mnemonic = _cc_cfg._instruction_mnemonic(
                    transfer_instruction
                )
                if (
                    transfer_mnemonic not in {"call", "jmp"}
                    or transfer_operand != proof.transfer_register(transfer_address)
                    or not _cc_cfg._exact_invocation_encoding(
                        transfer_instruction,
                        mnemonic=transfer_mnemonic,
                    )
                ):
                    raise ValueError(
                        "retail exact-IAT transfer does not rejoin its exact "
                        f"register invocation at {transfer_address}"
                    )
                retail_iat_proofs_by_instruction_index.setdefault(
                    transfer_index, []
                ).append(proof)
    inbound_lineage_by_ordinal: dict[int, tuple[str, str, str]] = {}
    if inbound_entry_target_bridges:
        cleanup_populations = {
            bridge.lineage_cleanup_by_ordinal
            for bridge in inbound_entry_target_bridges.values()
        }
        if len(cleanup_populations) != 1:
            raise ValueError(
                "reviewed BN inbound bridges disagree on direct-call cleanup"
            )
        lineage_rows = _cc_dispatch._entry_register_lineage_calls(
            instructions,
            source=source,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            entry_registers=frozenset(
                bridge.entry_register
                for bridge in inbound_entry_target_bridges.values()
            ),
            cleanup_by_ordinal=dict(next(iter(cleanup_populations))),
        )
        for ordinal, call_address, call_register, entry_register in lineage_rows:
            if ordinal in inbound_lineage_by_ordinal:
                raise ValueError(
                    "entry-register lineage has a duplicate invocation ordinal"
                )
            inbound_lineage_by_ordinal[ordinal] = (
                call_address,
                call_register,
                entry_register,
            )
        if source == "cod":
            candidate_lineages_by_entry: dict[
                str, list[tuple[int, str, str]]
            ] = {}
            for (
                ordinal,
                call_address,
                call_register,
                entry_register,
            ) in lineage_rows:
                candidate_lineages_by_entry.setdefault(
                    entry_register,
                    [],
                ).append((ordinal, call_address, call_register))
            reviewed_bridges_by_entry: dict[
                str, list[ReviewedInboundEntryRegisterTargetBridge]
            ] = {}
            for inbound_bridge in inbound_entry_target_bridges.values():
                reviewed_bridges_by_entry.setdefault(
                    inbound_bridge.entry_register,
                    [],
                ).append(inbound_bridge)
            candidate_inbound_bridges: dict[
                int, ReviewedInboundEntryRegisterTargetBridge
            ] = {}
            for entry_register, reviewed_bridges in (
                reviewed_bridges_by_entry.items()
            ):
                candidate_lineages = candidate_lineages_by_entry.get(
                    entry_register,
                    [],
                )
                reviewed_bridges.sort(key=lambda bridge: bridge.ordinal)
                candidate_lineages.sort(key=lambda row: row[0])
                if (
                    len(candidate_lineages) != len(reviewed_bridges)
                    or len(
                        {
                            bridge.target_identity
                            for bridge in reviewed_bridges
                        }
                    )
                    != 1
                ):
                    raise ValueError(
                        "candidate inbound entry-register target lineage is "
                        "missing or ambiguous"
                    )
                for reviewed_bridge, candidate_lineage in zip(
                    reviewed_bridges,
                    candidate_lineages,
                ):
                    candidate_ordinal, _call_address, call_register = (
                        candidate_lineage
                    )
                    if candidate_ordinal in candidate_inbound_bridges:
                        raise ValueError(
                            "candidate inbound entry-register target lineage "
                            "has a duplicate invocation ordinal"
                        )
                    candidate_inbound_bridges[candidate_ordinal] = replace(
                        reviewed_bridge,
                        ordinal=candidate_ordinal,
                        call_register=call_register,
                    )
            if set(candidate_lineages_by_entry) != set(
                reviewed_bridges_by_entry
            ):
                raise ValueError(
                    "candidate inbound entry-register target lineage is "
                    "missing or ambiguous"
                )
            inbound_entry_target_bridges = candidate_inbound_bridges
        for ordinal, inbound_bridge in inbound_entry_target_bridges.items():
            lineage = inbound_lineage_by_ordinal.get(ordinal)
            if (
                lineage is None
                or lineage[1] != inbound_bridge.call_register
                or lineage[2] != inbound_bridge.entry_register
            ):
                raise ValueError(
                    "reviewed BN inbound target lacks the exact entry-register "
                    "lineage in this body"
                )
    loop_invariant_iat_markers = _cc_cfg._loop_invariant_exact_iat_markers(
        instructions,
        instruction_addresses=instruction_addresses,
        instruction_index_by_address=instruction_index_by_address,
        source=source,
        caller_start=start,
        caller_end=local_flow_end,
        local_control_flow_indices=local_control_flow_indices,
        local_control_flow_targets=switch_targets,
    )
    transparent_cursor_loads = _cc_receiver_cursor._transparent_address_cursor_stack_loads(
        instructions,
        source=source,
        caller_start=start,
        caller_end=local_flow_end,
        indexes=indexes,
        reviewed_register_storage_bridges=register_storage_bridges,
        reviewed_iat_register_definition_offsets=(
            iat_register_definition_offsets
        ),
        candidate_exact_iat_register_load_proofs=(
            candidate_iat_load_proofs
        ),
    )
    bounded_indexed_stack_loads = _cc_receiver_instructions._bounded_indexed_stack_array_loads(
        instructions,
        source=source,
        caller_start=start,
        caller_end=local_flow_end,
    )
    bounded_local_stack_scalar_loads = (
        _cc_receiver_instructions._bounded_initialized_stack_scalar_loads(
            instructions,
            source=source,
            caller_start=start,
            caller_end=local_flow_end,
        )
    )
    bounded_loop_counter_registers = _cc_receiver_proofs._bounded_loop_counter_registers(
        instructions,
        source=source,
        caller_start=start,
        caller_end=local_flow_end,
    )
    bounded_stack_loop_counter_loads = _cc_receiver_proofs._bounded_stack_loop_counter_loads(
        instructions,
        source=source,
        caller_start=start,
        caller_end=local_flow_end,
    )
    bounded_stack_loop_counter_initializers = (
        _cc_receiver_proofs._bounded_stack_loop_counter_initializers(
            instructions,
            bounded_stack_loop_counter_loads,
        )
    )
    bounded_targetless_cursors = _cc_receiver_storage._bounded_targetless_cursor_registers(
        instructions,
        source=source,
        caller_start=start,
        caller_end=local_flow_end,
    )
    first_entry_stack_affine_index_load = (
        _cc_receiver_instructions._first_entry_stack_affine_index_load(instructions)
    )
    exact_targetless_vptr_proofs = (
        packaged_targetless_vptr_proofs
        if packaged_targetless_vptr_proofs is not None
        else _cc_receiver_proofs._exact_targetless_vptr_call_proofs(
            instructions,
            source=source,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=indexes,
            local_control_flow_indices=local_control_flow_indices,
            local_control_flow_targets=switch_targets,
            direct_call_cleanup_by_instruction_index=(
                direct_call_cleanup_by_instruction_index
            ),
            candidate_caller_definition=candidate_caller_definition,
            allow_exact_this_member=(source == "cod"),
        )
    )
    if inbound_entry_roots:
        rebound_targetless_vptr_proofs: dict[int, str] = {}
        for proof_index, storage in exact_targetless_vptr_proofs.items():
            if "this" not in storage:
                rebound_targetless_vptr_proofs[proof_index] = storage
                continue
            # PlayOnDirectSound has one reviewed sole-caller ECX lineage.  Its
            # exact selected-handle proof joins AcquirePlayHandleDispatch's
            # result with the receiver's embedded fallback handle.  Rebase
            # only that finite caller/extent/marker package to the reviewed
            # inbound root; every other ``this``-rooted proof still fails
            # closed under the general inbound-lineage rule.
            if (
                start == 0x49FBB0
                and end == 0x49FCF0
                and inbound_entry_roots == frozenset({"ecx"})
                and storage == _cc_catalog._ZSND_PLAY_SELECTED_BACKEND_VPTR
            ):
                rebound_targetless_vptr_proofs[proof_index] = (
                    _cc_catalog._ZSND_PLAY_DIRECTSOUND_INBOUND_SELECTED_BACKEND_VPTR
                )
        exact_targetless_vptr_proofs = rebound_targetless_vptr_proofs
    registers = _cc_cfg._empty_register_state()
    registers.update(
        {
            "ecx": "this",
            "edx": "entry-register(edx)",
            "esp": "stack",
            "ebp": "frame",
        }
    )
    for entry_register in inbound_entry_roots:
        registers[entry_register] = f"entry-register({entry_register})"
    for inbound_bridge in inbound_entry_target_bridges.values():
        if inbound_bridge.entry_register not in {"ecx", "edx"}:
            raise ValueError(
                "reviewed BN inbound bridge has an unsupported entry register"
            )
        registers[inbound_bridge.entry_register] = (
            f"entry-register({inbound_bridge.entry_register})"
        )
    fallthrough_live = True
    incoming_register_states: dict[int, list[dict[str, str]]] = {}
    calls: list[dict[str, Any]] = []
    for index, instruction in enumerate(instructions):
        incoming = incoming_register_states.pop(index, [])
        if incoming:
            predecessors: list[Mapping[str, str]] = list(incoming)
            if fallthrough_live:
                predecessors.append(_cc_cfg._register_state_snapshot(registers))
            registers = _cc_cfg._merge_register_states(
                predecessors,
                reviewed_iat_register_joins=(
                    indexes.reviewed_iat_register_joins
                ),
            )
            fallthrough_live = True
        elif not fallthrough_live:
            registers = _cc_cfg._empty_register_state()

        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        operand = _cc_cfg._instruction_operand(instruction)
        instruction_address = instruction_addresses[index]
        if source == "bn" and instruction_address is None:
            raise ValueError(
                "retail instruction has no unique runtime address"
            )
        if source == "bn" and not start <= int(instruction_address) < end:
            # A BN assembly request for an overlapping EH continuation may
            # return the containing parent body.  Parent rows are not part of
            # the selected caller and cannot seed its register provenance or
            # invocation census.
            continue
        if (
            source == "bn"
            and address_counts.get(instruction_address, 0) != 1
        ):
            raise ValueError(
                "retail instruction has a duplicate runtime address"
            )
        exact_branch = _cc_cfg._exact_local_direct_branch(
            instruction,
            instruction_index=index,
            instruction_addresses=instruction_addresses,
            instruction_index_by_address=instruction_index_by_address,
            source=source,
            caller_start=start,
            caller_end=local_flow_end,
        )
        if (
            fallthrough_live
            and exact_branch is not None
            and exact_branch[1] > index
        ):
            branch_registers = _cc_cfg._register_state_snapshot(registers)
            if (
                exact_branch[0] == "conditional"
                and mnemonic in {"je", "jz"}
                and index > 0
            ):
                tested_register = _cc_receiver_cursor._exact_test_same_register(
                    instructions[index - 1]
                )
                if tested_register is None:
                    compared_registers = _cc_receiver_cursor._exact_cmp_register_pair(
                        instructions[index - 1]
                    )
                    if compared_registers is not None:
                        left, right = compared_registers
                        left_provenance = branch_registers.get(left, "")
                        right_provenance = branch_registers.get(right, "")
                        if (
                            left_provenance.startswith("call-result(")
                            and left_provenance.endswith(")")
                            and right_provenance == "null"
                        ):
                            tested_register = left
                        elif (
                            right_provenance.startswith("call-result(")
                            and right_provenance.endswith(")")
                            and left_provenance == "null"
                        ):
                            tested_register = right
                tested_provenance = branch_registers.get(
                    tested_register or "",
                    "",
                )
                if (
                    tested_register is not None
                    and tested_provenance.startswith("call-result(")
                    and tested_provenance.endswith(")")
                ):
                    branch_registers[tested_register] = "null"
            incoming_register_states.setdefault(exact_branch[1], []).append(
                branch_registers
            )
        elif (
            fallthrough_live
            and exact_branch is not None
            and exact_branch[1] <= index
        ):
            # Preserve only a uniquely dominating exact preheader IAT load in
            # a nonvolatile register when the complete local CFG proves every
            # loop path retains that same reaching definition.
            for register, abstract in tuple(registers.items()):
                if abstract.startswith(("exact-iat-load(", "exact-iat-join(")) and not any(
                    backedge_index == index
                    and safe_register == register
                    and abstract.startswith(marker_prefix)
                    for backedge_index, safe_register, marker_prefix
                    in loop_invariant_iat_markers
                ):
                    registers[register] = ""
        if mnemonic not in {"call", "jmp"}:
            if fallthrough_live:
                _cc_receiver_storage._update_register_state(
                    instruction,
                    registers,
                    assembly_source=source,
                    indexes=indexes,
                    reviewed_register_storage_bridges=register_storage_bridges,
                    reviewed_static_storage_reference_bridges=(
                        static_storage_reference_bridges
                    ),
                    reviewed_absolute_storage_load_bridges=(
                        absolute_storage_load_bridges
                    ),
                    consumed_absolute_storage_load_bridges=(
                        consumed_absolute_storage_load_bridges
                    ),
                    reviewed_member_vptr_load_bridges=(
                        member_vptr_load_bridges
                    ),
                    consumed_member_vptr_load_bridges=(
                        consumed_member_vptr_load_bridges
                    ),
                    reviewed_instruction_address=(
                        _cc_cfg._reviewed_cod_register_definition_address(
                            instructions,
                            instruction_index=index,
                            instruction_addresses=instruction_addresses,
                            instruction_address_counts=address_counts,
                            caller_start=start,
                            caller_end=local_flow_end,
                            reviewed_absolute_storage_load_bridges=(
                                absolute_storage_load_bridges
                            ),
                            reviewed_member_vptr_load_bridges=(
                                member_vptr_load_bridges
                            ),
                            reviewed_iat_register_definition_offsets=(
                                iat_register_definition_offsets
                            ),
                        )
                        if source == "cod"
                        else None
                    ),
                    entry_stack_root_allowed=(
                        index == 0 and first_entry_stack_affine_index_load
                    ),
                    bounded_local_stack_provenance=(
                        bounded_stack_loop_counter_loads.get(index, "")
                        or bounded_local_stack_scalar_loads.get(index, "")
                    ),
                    bounded_loop_counter_register=(
                        bounded_loop_counter_registers.get(index, "")
                    ),
                    bounded_targetless_cursor=(
                        bounded_targetless_cursors.get(index)
                    ),
                    candidate_exact_iat_register_load_proofs=(
                        candidate_iat_load_proofs
                    ),
                    consumed_candidate_exact_iat_register_load_proofs=(
                        consumed_candidate_iat_load_proofs
                    ),
                )
                cursor_load = transparent_cursor_loads.get(index)
                exact_stack_load = _cc_receiver_instructions._exact_stack_slot_load(instruction)
                if cursor_load and exact_stack_load is not None:
                    registers[exact_stack_load[0]] = cursor_load
                indexed_stack_load = bounded_indexed_stack_loads.get(index)
                exact_indexed_load = _cc_receiver_instructions._exact_indexed_stack_array_load(
                    instruction
                )
                if (
                    indexed_stack_load
                    and exact_indexed_load is not None
                ):
                    registers[exact_indexed_load[0]] = indexed_stack_load
                stack_counter = bounded_stack_loop_counter_loads.get(index)
                symbolic_counter_load = (
                    _cc_receiver_instructions._exact_vc5_symbolic_stack_slot_load(instruction)
                )
                if stack_counter and symbolic_counter_load is not None:
                    registers[symbolic_counter_load[1]] = stack_counter
                stack_counter_initializer = (
                    bounded_stack_loop_counter_initializers.get(index)
                )
                if stack_counter_initializer is not None:
                    register, marker = stack_counter_initializer
                    registers[register] = marker
            if fallthrough_live and _cc_cfg._exact_return_terminates(instruction):
                fallthrough_live = False
                registers = _cc_cfg._empty_register_state()
            continue
        if mnemonic == "jmp" and index in local_control_flow_indices:
            targets = switch_targets.get(index, ())
            if targets and not all(
                index < target < len(instructions) for target in targets
            ):
                raise ValueError(
                    f"local switch at instruction {index} has an invalid "
                    "non-forward target"
                )
            if fallthrough_live:
                snapshot = _cc_cfg._register_state_snapshot(registers)
                for target in targets:
                    incoming_register_states.setdefault(target, []).append(
                        snapshot
                    )
            fallthrough_live = False
            registers = _cc_cfg._empty_register_state()
            continue
        if (
            mnemonic == "jmp"
            and source == "bn"
            and _cc_cfg._exact_local_indexed_jump_table_address(
                instruction,
                caller_start=start,
                caller_end=end,
            )
            is not None
        ):
            # Complete switch-edge recovery is optional for classification.
            # With unknown targets there is no trustworthy fallthrough state,
            # so stop this path without creating a tail-call row.
            fallthrough_live = False
            registers = _cc_cfg._empty_register_state()
            continue
        if (
            mnemonic == "jmp"
            and source == "cod"
            and _cc_listing._cod_exact_indexed_switch(instruction) is not None
        ):
            # VC5 COD listings encode the same FF /4 SIB table with a $L
            # label and relocation zeros.  A byte-map lowering
            # (`xor edx,edx` / `mov dl, map[eax]` / `jmp table[edx*4]`)
            # is still local dispatch.  Unknown successors terminate
            # forward provenance; the JMP is never an external tail.
            fallthrough_live = False
            registers = _cc_cfg._empty_register_state()
            continue
        form = "call" if mnemonic == "call" else "tail"
        indirect = (
            (instruction.bytes and instruction.bytes[0].lower() == "ff")
            or "[" in operand
            or _cc_catalog.REGISTER_RE.fullmatch(
                re.sub(r"^(?:near|far)\s+(?:ptr\s+)?", "", operand).strip()
            )
            is not None
        )
        if form == "tail" and not indirect:
            try:
                direct_body = bytes(
                    int(item, 16) for item in instruction.bytes
                )
            except (TypeError, ValueError):
                direct_body = b""
            if (
                source == "bn"
                and instruction_address is not None
                and len(direct_body) == 2
                and direct_body[0] == 0xEB
            ):
                local_target = (
                    int(instruction_address)
                    + 2
                    + struct.unpack("<b", direct_body[1:2])[0]
                )
                if instruction_addresses.count(local_target) == 1:
                    # BN may return an overlapping EH continuation whose
                    # instruction lies just beyond the selected physical row.
                    # An exact short jump to that instruction is intra-body
                    # control flow, not a tail-call contract.  A real short
                    # tail to an external body is absent from this assembly
                    # population and remains subject to normal target proof.
                    fallthrough_live = False
                    registers = _cc_cfg._empty_register_state()
                    continue
            if exact_branch is not None:
                fallthrough_live = False
                registers = _cc_cfg._empty_register_state()
                continue
            address_matches = _cc_catalog.ADDRESS_RE.findall(operand)
            if address_matches and start <= address_value(address_matches[-1]) < end:
                fallthrough_live = False
                registers = _cc_cfg._empty_register_state()
                continue
            if not address_matches and (
                operand.startswith("$")
                or operand.lower().startswith("short ")
                or (
                    _cc_catalog.DECORATED_RE.search(operand) is None
                    and operand.strip() not in compiler_bridges
                )
            ):
                fallthrough_live = False
                registers = _cc_cfg._empty_register_state()
                continue
        if indirect:
            call_address = (
                normalize_address(instruction_addresses[index])
                if instruction_addresses[index] is not None
                else ""
            )
            register_operand = re.sub(
                r"^(?:near|far)\s+(?:ptr\s+)?",
                "",
                operand.strip().lower(),
            )
            reached_retail_iat_proofs = list(
                retail_iat_proofs_by_instruction_index.get(index, ())
            )
            if any(
                register_operand != proof.transfer_register(call_address)
                for proof in reached_retail_iat_proofs
            ):
                raise ValueError(
                    "retail exact-IAT transfer/register consumer drift at "
                    f"{call_address}"
                )
            if len(reached_retail_iat_proofs) > 1:
                reached_routes = {
                    (proof.transfer_register(call_address), proof.identity)
                    for proof in reached_retail_iat_proofs
                }
                if reached_routes != {
                    (
                        register_operand,
                        reached_retail_iat_proofs[0].identity,
                    )
                }:
                    raise ValueError(
                        "retail exact-IAT register-load proof is ambiguous at "
                        f"{call_address}"
                    )
            if reached_retail_iat_proofs:
                definition_indices = frozenset(
                    int(proof.definition_instruction_index)
                    for proof in reached_retail_iat_proofs
                )
                if (
                    -1 in definition_indices
                    or call_address not in _cc_dispatch._retail_register_definition_transfer_addresses(
                        instructions,
                        load_index=int(reached_retail_iat_proofs[0].definition_instruction_index),
                        destination=reached_retail_iat_proofs[0].destination,
                        caller_start=start, caller_end=end,
                        local_control_flow_indices=local_control_flow_indices,
                        local_control_flow_targets=switch_targets,
                        direct_call_cleanup_by_instruction_index=dict(retail_proof_package.direct_call_cleanup_by_instruction_index) if retail_proof_package else {},
                        equivalent_absolute_load_address=normalize_address(int.from_bytes(bytes.fromhex(" ".join(instructions[int(reached_retail_iat_proofs[0].definition_instruction_index)].bytes))[2:], "little")),
                    )
                ):
                    raise ValueError(
                        "retail exact-IAT register-load proof does not cover "
                        f"every CFG path to {call_address}"
                    )
                registers = dict(registers)
                proof = reached_retail_iat_proofs[0]
                registers[register_operand] = proof.transfer_marker(call_address, joined=len(reached_retail_iat_proofs) > 1)
                consumed_retail_iat_transfers.update(
                    (reached.definition_offset, call_address)
                    for reached in reached_retail_iat_proofs
                )
            candidate_call_offset = (
                normalize_address(int(instruction_addresses[index]) - start)
                if (
                    source == "cod"
                    and instruction_addresses[index] is not None
                )
                else ""
            )
            reached_candidate_iat_proofs = [
                proof
                for proof in candidate_iat_load_proofs.values()
                if candidate_call_offset in proof.transfer_offsets
            ]
            if reached_candidate_iat_proofs:
                # The complete candidate CFG proof owns the exact reaching
                # definition/use population.  Re-materialize that identity at
                # each proved CALL/JMP after conservative loop-state merging,
                # symmetrically with immutable-retail transfer proofs above.
                proof = reached_candidate_iat_proofs[0]
                reached_routes = {
                    (reached.transfer_register(candidate_call_offset), reached.identity, reached.object_symbol)
                    for reached in reached_candidate_iat_proofs
                }
                definition_indices: list[int] = []
                for reached in reached_candidate_iat_proofs:
                    definition_address = start + address_value(
                        reached.definition_offset
                    )
                    definition_index = instruction_index_by_address.get(
                        definition_address
                    )
                    if definition_index is None:
                        definition_indices.append(-1)
                    else:
                        definition_indices.append(definition_index)
                if (
                    reached_routes
                    != {(register_operand, proof.identity, proof.object_symbol)}
                    or -1 in definition_indices
                    or len(set(definition_indices)) != len(definition_indices)
                ):
                    raise ValueError(
                        "candidate exact-IAT register-load proof is ambiguous at "
                        f"{candidate_call_offset}"
                    )
                registers = dict(registers)
                registers[register_operand] = proof.transfer_marker(candidate_call_offset, joined=len(reached_candidate_iat_proofs) > 1)
                consumed_candidate_iat_transfers.update(
                    (reached.definition_offset, candidate_call_offset)
                    for reached in reached_candidate_iat_proofs
                )
            inbound_entry_target_bridge = inbound_entry_target_bridges.get(
                len(calls)
            )
            if inbound_entry_target_bridge is not None:
                registers = dict(registers)
                registers[inbound_entry_target_bridge.call_register] = (
                    f"entry-register({inbound_entry_target_bridge.entry_register})"
                )
            reviewed_call_address = (
                normalize_address(instruction_addresses[index] - start)
                if (
                    source == "cod"
                    and instruction_addresses[index] is not None
                )
                else call_address
            )
            exact_targetless_vptr_storage = (
                exact_targetless_vptr_proofs.get(index, "")
            )
            if reviewed_call_address in {
                *vptr_storage_bridges,
                *loop_vptr_storage_bridges,
                *exact_indirect_storage_bridges,
                *member_vptr_call_bridges,
            }:
                exact_targetless_vptr_storage = ""
            register_call_bridge = register_call_storage_bridges.get(
                reviewed_call_address
            )
            if register_call_bridge is not None:
                canonical_register = re.sub(
                    r"^(?:near|far)\s+(?:ptr\s+)?",
                    "",
                    operand.strip().lower(),
                )
                if (
                    reviewed_call_address
                    in consumed_register_call_storage_bridges
                    or source != register_call_bridge.assembly_source
                    or register_call_bridge.form not in {"call", "tail"}
                    or form != register_call_bridge.form
                    or not _cc_cfg._exact_invocation_encoding(
                        instruction,
                        mnemonic=(
                            "call"
                            if register_call_bridge.form == "call"
                            else "jmp"
                        ),
                    )
                    or canonical_register != register_call_bridge.register
                    or register_call_bridge.identity_kind
                    not in {"callback", "iat"}
                    or not register_call_bridge.storage_identity
                    or (
                        register_call_bridge.identity_kind == "iat"
                        and not register_call_bridge.storage_identity.startswith(
                            "iat:"
                        )
                    )
                    or (
                        register_call_bridge.identity_kind == "callback"
                        and register_call_bridge.storage_identity.startswith(
                            "iat:"
                        )
                    )
                ):
                    raise ValueError(
                        "reviewed register-call storage bridge does not match "
                        f"invocation at {reviewed_call_address}"
                    )
                consumed_register_call_storage_bridges.add(
                    reviewed_call_address
                )
                identity_kind = register_call_bridge.identity_kind
                target_identity = (
                    register_call_bridge.storage_identity
                    if identity_kind == "iat"
                    else ""
                )
                slot = None
                storage = register_call_bridge.storage_identity
            else:
                if (source == "cod" and candidate_caller_definition is not None
                        and _cc_catalog.REGISTER_RE.fullmatch(register_operand)
                        and not registers.get(register_operand)):
                    static_callback = _cc_receiver_candidate._candidate_exact_static_callback_register(
                        instructions, transfer_index=index, register=register_operand,
                        definition=candidate_caller_definition, indexes=indexes,
                        addresses=instruction_addresses, caller_start=start,
                        caller_end=local_flow_end,
                        local_control_flow_indices=local_control_flow_indices,
                        local_control_flow_targets={
                            **switch_targets,
                            **dict(candidate_classification_only_local_control_flow_targets or {}),
                        },
                    )
                    if static_callback:
                        registers[register_operand] = static_callback
                identity_kind, target_identity, slot, storage = _cc_receiver_storage._canonical_indirect_storage(
                    operand,
                    instruction=instruction,
                    reviewed_instruction_address=(
                        reviewed_call_address if source == "cod" else None
                    ),
                    source=source,
                    registers=registers,
                    indexes=indexes,
                    candidate_storage_bridges=storage_bridges,
                    reviewed_call_result_bridges=call_result_bridges,
                    consumed_call_result_bridges=consumed_call_result_bridges,
                    reviewed_vptr_storage_bridges=vptr_storage_bridges,
                    consumed_vptr_storage_bridges=(
                        consumed_vptr_storage_bridges
                    ),
                        reviewed_loop_vptr_storage_bridges=(
                            loop_vptr_storage_bridges
                        ),
                        consumed_loop_vptr_storage_bridges=(
                            consumed_loop_vptr_storage_bridges
                        ),
                        reviewed_exact_indirect_storage_bridges=(
                            exact_indirect_storage_bridges
                        ),
                        consumed_exact_indirect_storage_bridges=(
                            consumed_exact_indirect_storage_bridges
                        ),
                        reviewed_member_vptr_call_bridges=(
                            member_vptr_call_bridges
                    ),
                    consumed_member_vptr_call_bridges=(
                        consumed_member_vptr_call_bridges
                    ),
                    reviewed_inbound_entry_register_target_bridge=(
                        inbound_entry_target_bridge
                    ),
                    natural_nested_absolute_loads=natural_nested_absolute_loads,
                    exact_targetless_vptr_storage=exact_targetless_vptr_storage,
                )
            if source == "cod" and identity_kind == "iat":
                register_operand = re.sub(
                    r"^(?:near|far)\s+(?:ptr\s+)?",
                    "",
                    operand.strip().lower(),
                )
                abstract = registers.get(register_operand, "")
                exact_candidate_load = re.fullmatch(
                    r"exact-iat-load\("
                    r"(?P<register>e(?:ax|bx|cx|dx|si|di|bp)),"
                    r"(?P<definition>0x[0-9a-f]+),"
                    r"(?P<identity>iat:[^)]+)\)",
                    abstract,
                )
                if (
                    exact_candidate_load is not None
                    and candidate_iat_load_proofs
                ):
                    definition_offset = exact_candidate_load.group(
                        "definition"
                    )
                    proof = candidate_iat_load_proofs.get(
                        definition_offset
                    )
                    call_offset = (
                        normalize_address(int(instruction_addresses[index]) - start)
                        if instruction_addresses[index] is not None
                        else ""
                    )
                    if (
                        proof is None
                        or proof.transfer_register(call_offset)
                        != exact_candidate_load.group("register")
                        or proof.identity
                        != exact_candidate_load.group("identity")
                        or call_offset not in proof.transfer_offsets
                    ):
                        raise ValueError(
                            "candidate exact-IAT register-load proof does not "
                            "authorize this reached CALL/JMP"
                        )
            if inbound_entry_target_bridge is not None:
                consumed_inbound_entry_target_bridges.add(len(calls))
            dispatch = "indirect"
        else:
            reviewed_retail_call_site = (
                reviewed_retail_call_sites_by_ordinal[len(calls)]
                if reviewed_retail_call_sites_by_ordinal is not None
                and len(calls) < len(reviewed_retail_call_sites_by_ordinal)
                else None
            )
            identity_kind, target_identity = _cc_targets._canonical_direct_identity(
                operand,
                source=source,
                caller_identity=caller_identity,
                caller_start=start,
                caller_end=end,
                indexes=indexes,
                bridge_names=names,
                compiler_generated_bridges=compiler_bridges,
                call_site_address=(
                    instruction_addresses[index]
                    if source in {"bn", "cod"}
                    else None
                ),
                call_site_unique=(
                    instruction_addresses[index] is not None
                    and address_counts.get(instruction_addresses[index], 0)
                    == 1
                ),
                retail_call_site_address=reviewed_retail_call_site,
                cod_source_line=(
                    instruction.source_line if source == "cod" else ""
                ),
            )
            slot = None
            storage = ""
            dispatch = "direct"
        if not _cc_cfg._exact_invocation_encoding(
            instruction,
            mnemonic=mnemonic,
        ):
            raise ValueError(
                "invocation requires exact x86 CALL/JMP encoding; direct "
                "calls require a direct E8 CALL"
            )
        pre_call_ecx = registers.get("ecx", "")
        calls.append(
            {
                "ordinal": len(calls),
                "form": form,
                "dispatch": dispatch,
                "identity_kind": identity_kind,
                "target_identity": target_identity,
                "storage_identity": storage,
                "slot_displacement": slot,
                "cleanup_bytes": _cc_cfg._cleanup_after(instructions, index),
            }
        )
        if invocation_call_sites_out is not None:
            invocation_address = instruction_addresses[index]
            if (
                invocation_address is None
                and source == "cod"
                and candidate_caller_definition is not None
            ):
                invocation_address = _cc_cfg._closed_candidate_invocation_runtime_address(
                    instructions,
                    instruction_index=index,
                    caller_start=start,
                    caller_end=end,
                    caller_definition=candidate_caller_definition,
                )
            if invocation_address is None:
                raise ValueError(
                    "invocation call-site population has no exact runtime coordinate"
                )
            invocation_call_sites_out.append(
                normalize_address(invocation_address)
            )
        if mnemonic == "call" and fallthrough_live:
            for volatile in ("eax", "ecx", "edx"):
                registers[volatile] = ""
            if target_identity and (
                not indirect or identity_kind == "iat"
            ):
                registers["eax"] = (
                    _cc_targets._exact_thiscall_constructor_result(
                        operand,
                        source=source,
                        target_identity=target_identity,
                        indexes=indexes,
                        receiver_provenance=pre_call_ecx,
                    )
                    or f"call-result({target_identity})"
                )
        if mnemonic == "jmp":
            fallthrough_live = False
            registers = _cc_cfg._empty_register_state()
    unused_call_result_bridges = (
        set(call_result_bridges) - consumed_call_result_bridges
    )
    unused_candidate_iat_load_proofs = (
        set(candidate_iat_load_proofs) - consumed_candidate_iat_load_proofs
    )
    if unused_candidate_iat_load_proofs:
        raise ValueError(
            "unused candidate exact-IAT register-load proof at "
            + ", ".join(sorted(unused_candidate_iat_load_proofs))
        )
    expected_candidate_iat_transfers = {
        (proof.definition_offset, transfer)
        for proof in candidate_iat_load_proofs.values()
        for transfer in proof.transfer_offsets
    }
    unused_candidate_iat_transfers = (
        expected_candidate_iat_transfers
        - consumed_candidate_iat_transfers
    )
    if unused_candidate_iat_transfers:
        raise ValueError(
            "unused candidate exact-IAT register-load transfer proof at "
            + ", ".join(
                f"{definition}->{transfer}"
                for definition, transfer in sorted(
                    unused_candidate_iat_transfers,
                    key=lambda row: (
                        address_value(row[0]),
                        address_value(row[1]),
                    ),
                )
            )
        )
    expected_retail_iat_transfers = {
        (proof.definition_offset, transfer)
        for proof in retail_iat_load_proofs.values()
        for transfer in proof.transfer_offsets
    }
    unused_retail_iat_transfers = (
        expected_retail_iat_transfers - consumed_retail_iat_transfers
    )
    if unused_retail_iat_transfers:
        raise ValueError(
            "unused retail exact-IAT register-load transfer proof at "
            + ", ".join(
                f"{definition}->{transfer}"
                for definition, transfer in sorted(
                    unused_retail_iat_transfers,
                    key=lambda row: (address_value(row[0]), address_value(row[1])),
                )
            )
        )
    if unused_call_result_bridges:
        raise ValueError(
            "unused reviewed call-result bridge at "
            + ", ".join(sorted(unused_call_result_bridges))
        )
    unused_register_call_storage_bridges = (
        set(register_call_storage_bridges)
        - consumed_register_call_storage_bridges
    )
    if unused_register_call_storage_bridges:
        raise ValueError(
            "unused reviewed register-call storage bridge at "
            + ", ".join(sorted(unused_register_call_storage_bridges))
        )
    unused_vptr_storage_bridges = (
        set(vptr_storage_bridges) - consumed_vptr_storage_bridges
    )
    if unused_vptr_storage_bridges:
        raise ValueError(
            "unused reviewed exact-callsite vptr storage bridge at "
            + ", ".join(sorted(unused_vptr_storage_bridges))
        )
    unused_loop_vptr_storage_bridges = (
        set(loop_vptr_storage_bridges)
        - consumed_loop_vptr_storage_bridges
    )
    if unused_loop_vptr_storage_bridges:
        raise ValueError(
            "unused reviewed HUD layout-loop vptr storage bridge at "
            + ", ".join(sorted(unused_loop_vptr_storage_bridges))
        )
    unused_exact_indirect_storage_bridges = (
        set(exact_indirect_storage_bridges)
        - consumed_exact_indirect_storage_bridges
    )
    if unused_exact_indirect_storage_bridges:
        raise ValueError(
            "unused reviewed exact indirect storage bridge at "
            + ", ".join(
                sorted(unused_exact_indirect_storage_bridges)
            )
        )
    unused_inbound_entry_target_bridges = (
        set(inbound_entry_target_bridges)
        - consumed_inbound_entry_target_bridges
    )
    if unused_inbound_entry_target_bridges:
        raise ValueError(
            "unused reviewed BN inbound entry-register target bridge ordinal "
            + ", ".join(
                str(value)
                for value in sorted(unused_inbound_entry_target_bridges)
            )
        )
    unused_absolute_storage_load_bridges = (
        set(absolute_storage_load_bridges)
        - consumed_absolute_storage_load_bridges
    )
    if unused_absolute_storage_load_bridges:
        raise ValueError(
            "unused reviewed absolute-storage load bridge at "
            + ", ".join(sorted(unused_absolute_storage_load_bridges))
        )
    unused_member_vptr_load_bridges = (
        set(member_vptr_load_bridges)
        - consumed_member_vptr_load_bridges
    )
    if unused_member_vptr_load_bridges:
        raise ValueError(
            "unused reviewed member-vptr load bridge at "
            + ", ".join(sorted(unused_member_vptr_load_bridges))
        )
    unused_member_vptr_call_bridges = (
        set(member_vptr_call_bridges)
        - consumed_member_vptr_call_bridges
    )
    if unused_member_vptr_call_bridges:
        raise ValueError(
            "unused reviewed member-vptr call bridge at "
            + ", ".join(sorted(unused_member_vptr_call_bridges))
        )
    for ordinal, lineage in _cc_identity._preserved_entry_stack_argument_lineages(
        instructions,
        calls,
    ).items():
        if 0 <= ordinal < len(calls):
            calls[ordinal]["argument_lineage"] = lineage
    return calls
