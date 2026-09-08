"""Recoil call-contract iat evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import dispatch as _cc_dispatch
from _recoil.call_contract import errors as _cc_errors
from _recoil.call_contract import flow as _cc_flow
from _recoil.call_contract import instructions as _cc_instructions
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions
from _recoil.call_contract import receiver_storage as _cc_receiver_storage
from _recoil.call_contract import recoil_lifecycle as _cc_recoil_lifecycle
from _recoil.call_contract import storage_identity as _cc_storage_identity
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        CandidateExactIatRegisterLoadProof,
        IdentityIndexes,
        ProviderNamedImportThunk,
    )

import re
import struct
from dataclasses import replace
from pathlib import Path
from typing import Any, Callable, Mapping, Sequence

from _recoil.commands.asm_verify import IMAGE_REL_I386_DIR32, Instruction
from _recoil.commands.provider_target_mutation import (
    _retail_import_targets,
    retail_import_target,
)
from _recoil.lib.progress import ProgressDocument, address_value, normalize_address


def _comparison_scoped_direct_iat_indexes(
    retail_instructions: Sequence[Instruction],
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    reference: Path = _cc_catalog.DEFAULT_REFERENCE,
    retail_import_targets: Sequence[Any] | None = None,
) -> IdentityIndexes:
    """Publish immutable import identity for exact retail FF15 calls.

    Retail instruction bytes select IAT slots; the immutable PE import
    directory supplies DLL/name-or-ordinal identity.  A complete current
    tracker package supplies the reviewed callable spelling for an ordinal
    import, because the PE deliberately has no name in that case.  BN
    rendering and candidate symbols are checked consumers only.  This
    producer is comparison-scoped and has no caller, address, DLL, import, or
    ordinal allowlist.
    """

    exact_calls: list[tuple[str, Instruction]] = []
    for instruction in retail_instructions:
        decoded_address = _cc_targets._exact_ff15_absolute_address(instruction)
        if decoded_address is None:
            continue
        if _cc_cfg._instruction_mnemonic(instruction) != "call":
            raise ValueError(
                "comparison-scoped direct IAT producer requires CALL for "
                "exact FF15 bytes"
            )
        exact_calls.append((decoded_address, instruction))
    if not exact_calls:
        return indexes

    start = normalize_address(caller_start)
    end = normalize_address(caller_end_exclusive)
    caller_id = caller_identity.removeprefix("symbol:")
    caller = document.collection("symbols").get(caller_id)
    if (
        not caller_identity.startswith("symbol:recoil:function:")
        or not isinstance(caller, Mapping)
        or caller.get("binary") != "recoil"
        or caller.get("kind") != "function"
        or caller.get("pipeline_class")
        not in {"authored", "authored-lifecycle"}
        or caller.get("ownership_state") != "primary-owned"
        or normalize_address(str(caller.get("address", ""))) != start
        or normalize_address(str(caller.get("end_exclusive", ""))) != end
        or caller.get("extent_state") != "known"
        or caller.get("size")
        != address_value(end) - address_value(start)
        or not isinstance(caller.get("physical_block_id"), str)
        or not caller.get("physical_block_id")
        or indexes.by_address.get(start) != caller_identity
        or caller_identity in indexes.provider_ids
    ):
        raise ValueError(
            "comparison-scoped direct IAT producer requires the current "
            "authored caller and exact registered extent"
        )

    pipeline_class = caller.get("pipeline_class")
    authored_role = caller.get("authored_order_role")
    if (
        pipeline_class == "authored"
        and authored_role not in {None, "authored-body"}
    ):
        raise ValueError(
            "comparison-scoped direct IAT producer requires an authored-body "
            "caller"
        )
    if pipeline_class == "authored-lifecycle":
        if authored_role != "authored-lifecycle-body":
            raise ValueError(
                "comparison-scoped direct IAT producer requires an "
                "authored-lifecycle-body caller"
            )
        _cc_recoil_lifecycle._validate_comparison_scoped_lifecycle_extent(
            retail_instructions,
            document=document,
            caller_id=caller_id,
            caller=caller,
            caller_start=start,
            caller_end_exclusive=end,
            reference=reference,
        )

    targets = tuple(
        retail_import_targets
        if retail_import_targets is not None
        else _retail_import_targets(reference)[0]
    )
    tracker_packages = _cc_storage_identity._current_tracker_iat_storage_packages(document)
    storage_by_address = dict(indexes.storage_by_address)
    storage_by_name = dict(indexes.storage_by_name)
    for slot, instruction in exact_calls:
        matches = [
            target
            for target in targets
            if normalize_address(str(getattr(target, "address", ""))) == slot
        ]
        if not matches:
            # FF15 is also the encoding for absolute indirect calls through
            # authored tables and callback storage.  With no immutable PE
            # import route, publish nothing and leave the call to those exact
            # non-import resolvers.
            if storage_by_address.get(slot, "").startswith("iat:"):
                raise ValueError(
                    f"immutable retail direct IAT slot {slot} has no import "
                    "route but conflicts with an existing IAT identity"
                )
            continue
        if len(matches) != 1:
            raise ValueError(
                f"immutable retail direct IAT slot {slot} resolves to "
                f"{len(matches)} imports; expected one"
            )
        target = matches[0]
        import_name = getattr(target, "import_name", None)
        import_dll = getattr(target, "dll", None)
        import_ordinal = getattr(target, "import_ordinal", None)
        if (
            not isinstance(import_name, str)
            or not import_name
            or not isinstance(import_dll, str)
            or not import_dll
            or "/" in import_dll
            or "\\" in import_dll
        ):
            raise ValueError(
                f"immutable retail direct IAT slot {slot} is not one exact "
                "DLL import"
            )
        rendered_names: set[str]
        if import_ordinal is None:
            if import_name.startswith("#"):
                raise ValueError(
                    f"immutable retail named import {import_name!r} has an "
                    "ordinal-shaped name"
                )
            exact_name_routes = {
                (
                    normalize_address(str(getattr(item, "address", ""))),
                    str(getattr(item, "dll", "")),
                    str(getattr(item, "import_name", "")),
                )
                for item in targets
                if getattr(item, "import_ordinal", None) is None
                and getattr(item, "import_name", None) == import_name
            }
            if len(exact_name_routes) != 1:
                raise ValueError(
                    f"immutable retail named import {import_name!r} has "
                    "ambiguous IAT/DLL routes"
                )
            identity = f"iat:{import_name}"
            rendered_names = {import_name}
        else:
            if (
                isinstance(import_ordinal, bool)
                or not isinstance(import_ordinal, int)
                or not 1 <= import_ordinal <= 0xFFFF
                or import_name != f"#{import_ordinal}"
            ):
                raise ValueError(
                    f"immutable retail ordinal import at {slot} has malformed "
                    "ordinal identity"
                )
            package_matches = [
                package
                for package in tracker_packages
                if package.address == slot
                and package.import_dll == import_dll
                and package.import_name == import_name
                and package.import_ordinal == import_ordinal
            ]
            if len(package_matches) != 1:
                raise ValueError(
                    f"immutable retail ordinal import {import_dll}!"
                    f"#{import_ordinal} at {slot} does not join one complete "
                    "current tracker package"
                )
            package = package_matches[0]
            identity = package.identity
            callable_symbol = package.object_symbol.removeprefix("__imp_")
            callable_import_name = _cc_receiver_instructions._candidate_iat_import_name(
                package.object_symbol
            )
            if (
                indexes.storage_by_address.get(slot) != identity
                or indexes.storage_by_name.get(package.object_symbol)
                != identity
                or not callable_symbol
                or callable_symbol == package.object_symbol
                or not callable_import_name
            ):
                raise ValueError(
                    f"immutable retail ordinal import {import_dll}!"
                    f"#{import_ordinal} lacks its exact reviewed tracker "
                    "IAT/callable identity"
                )
            rendered_names = {
                import_name,
                f"{import_dll}!#{import_ordinal}",
                package.object_symbol,
                callable_symbol,
                callable_import_name,
            }
        expression = _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(instruction)
        )
        if _cc_catalog.ADDRESS_RE.fullmatch(expression):
            if normalize_address(expression) != slot:
                raise ValueError(
                    "retail direct-IAT rendered/decoded address disagreement"
                )
        elif expression not in rendered_names:
            raise ValueError(
                f"retail direct-IAT BN name {expression!r} disagrees with "
                f"immutable import {import_dll}!{import_name}"
            )
        publication_keys = ((storage_by_address, slot),) + tuple(
            (storage_by_name, name) for name in sorted(rendered_names)
        )
        for mapping, key in publication_keys:
            prior = mapping.get(key)
            if prior is not None and prior != identity:
                raise ValueError(
                    "comparison-scoped direct IAT producer conflicts with "
                    f"existing identity for {key!r}"
                )
            mapping[key] = identity
    return replace(
        indexes,
        storage_by_address=storage_by_address,
        storage_by_name=storage_by_name,
    )


def _unique_retail_iat_transfer_proofs(
    proofs: Mapping[str, CandidateExactIatRegisterLoadProof],
) -> dict[str, CandidateExactIatRegisterLoadProof]:
    """Keep only route-unique retail transfer observations.

    Multiple exact loads of the same immutable IAT route may feed one transfer
    across branch/loop reload phis.  Preserve all such definitions so the
    consumer can require one unanimous route.  A transfer associated with two
    different register/import routes remains ambiguous and is removed from
    every proof instead of selecting one definition.
    """

    transfer_routes: dict[str, set[tuple[str, str]]] = {}
    for proof in proofs.values():
        for transfer in proof.transfer_offsets:
            transfer_routes.setdefault(transfer, set()).add(
                (proof.transfer_register(transfer), proof.identity)
            )
    result: dict[str, CandidateExactIatRegisterLoadProof] = {}
    for definition, proof in proofs.items():
        indexed = bool(proof.transfer_instruction_indices)
        body_indexed = bool(proof.transfer_body_offsets)
        if (
            indexed
            and len(proof.transfer_instruction_indices)
            != len(proof.transfer_offsets)
            or body_indexed
            and len(proof.transfer_body_offsets) != len(proof.transfer_offsets)
            or indexed != body_indexed
        ):
            raise ValueError(
                "retail exact-IAT proof has mismatched absolute/body/index populations"
            )
        transfer_rows = tuple(
            zip(
                proof.transfer_offsets,
                (
                    proof.transfer_instruction_indices
                    if indexed
                    else (None,) * len(proof.transfer_offsets)
                ),
                (
                    proof.transfer_body_offsets
                    if body_indexed
                    else (None,) * len(proof.transfer_offsets)
                ),
            )
        )
        unique_rows = tuple(
            (transfer, instruction_index, body_offset)
            for transfer, instruction_index, body_offset in transfer_rows
            if transfer_routes.get(transfer)
            == {(proof.transfer_register(transfer), proof.identity)}
        )
        unique_transfers = tuple(row[0] for row in unique_rows)
        if unique_transfers:
            result[definition] = replace(
                proof,
                transfer_offsets=unique_transfers,
                transfer_registers=tuple(proof.transfer_register(transfer) for transfer in unique_transfers),
                transfer_instruction_indices=(
                    tuple(int(row[1]) for row in unique_rows)
                    if indexed
                    else ()
                ),
                transfer_body_offsets=(
                    tuple(int(row[2]) for row in unique_rows)
                    if body_indexed
                    else ()
                ),
            )
    return result


def _comparison_scoped_retail_register_iat_indexes(
    retail_instructions: Sequence[Instruction],
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    reference: Path = _cc_catalog.DEFAULT_REFERENCE,
    retail_import_targets: Sequence[Any] | None = None,
    stored_callback_load_indices: frozenset[int] = frozenset(),
    local_control_flow_indices: frozenset[int] = frozenset(),
    local_control_flow_targets: Mapping[int, Sequence[int]] | None = None,
    precomposed_non_iat_loads: Mapping[str, str] | None = None,
    call_cleanup_by_instruction_index: Mapping[int, int] | None = None,
    _trace_overflow: Callable[[Mapping[str, Any]], None] | None = None,
) -> IdentityIndexes:
    """Publish named IAT identities from immutable retail facts only.

    Every publication is selected by an exact absolute ``MOV r32, [imm32]``
    inside the current authored caller and one unambiguous immutable PE import
    route.  Definition addresses stay embedded in the normal register
    provenance, so two loads of one slot do not become an authorized phi.
    Candidate COD, COFF, names, bytes, and call population are deliberately
    absent from this expected-fact producer.
    """
    from _recoil.call_contract.records import CandidateExactIatRegisterLoadProof

    # Definition/use proofs are caller-local observations.  Identity indexes
    # persist across the target population, but carrying a prior caller's
    # transfer addresses into the next extraction creates a false unused-proof
    # failure (and could carry a join authorization into an unrelated CFG).
    indexes = replace(
        indexes,
        reviewed_iat_register_joins=frozenset(),
        reviewed_retail_iat_load_proofs={},
    )

    register_names = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    start = normalize_address(caller_start)
    end = normalize_address(caller_end_exclusive)
    start_value = address_value(start)
    end_value = address_value(end)
    addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=start_value,
    )
    counts: dict[int, int] = {}
    for address in addresses:
        if address is not None and start_value <= address < end_value:
            counts[address] = counts.get(address, 0) + 1
    instruction_index_by_address = {
        address: instruction_index
        for instruction_index, address in enumerate(addresses)
        if address is not None and counts.get(address) == 1
    }
    precomposed = {
        normalize_address(address): identity
        for address, identity in dict(precomposed_non_iat_loads or {}).items()
    }
    if len(precomposed) != len(precomposed_non_iat_loads or {}):
        raise ValueError("precomposed non-IAT load definitions collide")
    precomposed_modrm: set[str] = set()
    for definition_address, storage_identity in precomposed.items():
        instruction_index = instruction_index_by_address.get(
            address_value(definition_address)
        )
        if instruction_index is None:
            raise ValueError(
                "precomposed non-IAT load publication is unused: "
                f"{definition_address}"
            )
        try:
            body = bytes(
                int(item, 16)
                for item in retail_instructions[instruction_index].bytes
            )
        except (TypeError, ValueError):
            body = b""
        accumulator = len(body) == 5 and body[0] == 0xA1
        modrm_absolute = (
            len(body) == 6
            and body[0] == 0x8B
            and body[1] >> 6 == 0
            and body[1] & 7 == 5
        )
        if not accumulator and not modrm_absolute:
            raise ValueError(
                "precomposed non-IAT definition is not an exact absolute MOV"
            )
        storage_address = normalize_address(
            struct.unpack_from("<I", body, 1 if accumulator else 2)[0]
        )
        if (
            not storage_identity
            or storage_identity.startswith("iat:")
            or indexes.storage_by_address.get(storage_address)
            != storage_identity
        ):
            raise ValueError(
                "precomposed non-IAT definition storage drifted at "
                f"{definition_address}"
            )
        if modrm_absolute:
            precomposed_modrm.add(definition_address)
    if any(
        type(index) is not int or not 0 <= index < len(retail_instructions)
        for index in stored_callback_load_indices
    ):
        raise ValueError(
            "stored-callback proof package has malformed retail load indices"
        )

    has_in_scope_absolute_modrm = any(
        runtime_address is not None
        and start_value <= runtime_address < end_value
        and (
            len(body) == 6
            and body[0] == 0x8B
            and body[1] >> 6 == 0
            and body[1] & 7 == 5
        )
        for runtime_address, instruction in zip(
            addresses, retail_instructions
        )
        for body in (
            (
                bytes(int(item, 16) for item in instruction.bytes)
                if instruction.bytes
                and all(
                    re.fullmatch(r"[0-9a-fA-F]{2}", item) is not None
                    for item in instruction.bytes
                )
                else b""
            ),
        )
    )
    targets = (
        tuple(
            retail_import_targets
            if retail_import_targets is not None
            else _retail_import_targets(reference)[0]
        )
        if has_in_scope_absolute_modrm
        else ()
    )
    immutable_slots = {
        normalize_address(str(getattr(target, "address", "")))
        for target in targets
    }

    def consumes_stored_callback(
        instruction_index: int,
        body: bytes,
    ) -> bool:
        if instruction_index not in stored_callback_load_indices:
            return False
        if (
            len(body) != 6
            or body[0] != 0x8B
            or body[1] >> 6 != 0
            or body[1] & 7 != 5
        ):
            raise ValueError(
                "stored-callback proof package does not rejoin an exact absolute MOV"
            )
        slot = normalize_address(struct.unpack_from("<I", body, 2)[0])
        storage_identity = indexes.storage_by_address.get(slot, "")
        target_identity = indexes.reviewed_static_callback_target_by_storage.get(
            storage_identity,
            "",
        )
        if (
            not storage_identity
            or storage_identity.startswith("iat:")
            or not target_identity
            or target_identity not in set(indexes.by_address.values())
        ):
            raise ValueError(
                "stored-callback proof package conflicts with retail storage/target identity"
            )
        return True

    potential_slots: set[str] = set()
    transfer_addresses_by_load: dict[int, tuple[str, ...]] = {}
    consumed_precomposed: set[str] = set()
    for instruction_index, (runtime_address, instruction) in enumerate(
        zip(addresses, retail_instructions)
    ):
        if runtime_address is None or not start_value <= runtime_address < end_value:
            continue
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            continue
        if (
            len(body) == 6
            and body[0] == 0x8B
            and body[1] >> 6 == 0
            and body[1] & 0x07 == 0x05
        ):
            if consumes_stored_callback(instruction_index, body):
                continue
            definition_address = normalize_address(runtime_address)
            if definition_address in precomposed_modrm:
                consumed_precomposed.add(definition_address)
                continue
            slot = normalize_address(struct.unpack_from("<I", body, 2)[0])
            # Register-IAT proof owns only immutable import cells.  Tracing an
            # ordinary absolute data load before this classification lets an
            # unrelated downstream switch, stack loop, or alternate data
            # definition block the whole caller despite there being no IAT
            # fact to publish.  A stale pre-existing IAT identity remains
            # fail-closed and is traced below so its missing route is reported.
            if (
                slot not in immutable_slots
                and not indexes.storage_by_address.get(
                    slot, ""
                ).startswith("iat:")
            ):
                continue
            destination = register_names[(body[1] >> 3) & 0x07]
            transfers = _cc_dispatch._retail_register_definition_transfer_addresses(
                retail_instructions,
                load_index=instruction_index,
                destination=destination,
                caller_start=start_value,
                caller_end=end_value,
                local_control_flow_indices=local_control_flow_indices,
                local_control_flow_targets=local_control_flow_targets,
                call_cleanup_by_instruction_index=(
                    call_cleanup_by_instruction_index
                ),
                _trace_overflow=_trace_overflow,
                equivalent_absolute_load_address=normalize_address(
                    struct.unpack_from("<I", body, 2)[0]
                ),
            )
            if transfers:
                transfer_addresses_by_load[instruction_index] = transfers
                potential_slots.add(slot)
    unused_precomposed = precomposed_modrm - consumed_precomposed
    if unused_precomposed:
        raise ValueError(
            "precomposed non-IAT load publication is unused: "
            + ", ".join(sorted(unused_precomposed, key=address_value))
        )
    if not potential_slots:
        return indexes

    proven_slots = potential_slots & immutable_slots
    if not proven_slots:
        conflicting = sorted(
            slot
            for slot in potential_slots
            if indexes.storage_by_address.get(slot, "").startswith("iat:")
        )
        if conflicting:
            raise ValueError(
                "immutable retail register IAT slots have no import route but "
                "conflict with existing IAT identities: "
                + ", ".join(conflicting)
            )
        return indexes

    caller_id = caller_identity.removeprefix("symbol:")
    caller = document.collection("symbols").get(caller_id)
    if (
        not caller_identity.startswith("symbol:recoil:function:")
        or not isinstance(caller, Mapping)
        or caller.get("binary") != "recoil"
        or caller.get("kind") != "function"
        or caller.get("pipeline_class")
        not in {"authored", "authored-lifecycle"}
        or caller.get("ownership_state") != "primary-owned"
        or normalize_address(str(caller.get("address", ""))) != start
        or normalize_address(str(caller.get("end_exclusive", ""))) != end
        or caller.get("extent_state") != "known"
        or caller.get("size") != end_value - start_value
        or not isinstance(caller.get("physical_block_id"), str)
        or not caller.get("physical_block_id")
        or indexes.by_address.get(start) != caller_identity
        or caller_identity in indexes.provider_ids
    ):
        raise ValueError(
            "retail register IAT producer requires the current authored "
            "caller and exact registered extent"
        )
    if (
        caller.get("pipeline_class") == "authored"
        and caller.get("authored_order_role")
        not in {None, "authored-body"}
    ):
        raise ValueError(
            "retail register IAT producer requires an authored-body caller"
        )
    if caller.get("pipeline_class") == "authored-lifecycle":
        if caller.get("authored_order_role") != "authored-lifecycle-body":
            raise ValueError(
                "retail register IAT producer requires an "
                "authored-lifecycle-body caller"
            )
        _cc_recoil_lifecycle._validate_comparison_scoped_lifecycle_extent(
            retail_instructions,
            document=document,
            caller_id=caller_id,
            caller=caller,
            caller_start=start,
            caller_end_exclusive=end,
            reference=reference,
        )

    storage_by_address = dict(indexes.storage_by_address)
    storage_by_name = dict(indexes.storage_by_name)
    reviewed_iat_register_joins = set(indexes.reviewed_iat_register_joins)
    retail_iat_proofs: dict[str, CandidateExactIatRegisterLoadProof] = {}
    transfer_sets_by_route: dict[
        tuple[str, str], list[frozenset[str]]
    ] = {}
    for instruction_index, (runtime_address, instruction) in enumerate(
        zip(addresses, retail_instructions)
    ):
        if runtime_address is None or not start_value <= runtime_address < end_value:
            continue
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            body = b""
        if (
            len(body) != 6
            or body[0] != 0x8B
            or body[1] >> 6 != 0
            or body[1] & 0x07 != 0x05
        ):
            continue
        if consumes_stored_callback(instruction_index, body):
            continue
        if counts.get(runtime_address) != 1:
            raise ValueError(
                "retail register IAT producer requires one exact instruction "
                "address for every absolute MOV"
            )
        destination = register_names[(body[1] >> 3) & 0x07]
        operands = _cc_cfg._instruction_operand(instruction).split(",", 1)
        if (
            _cc_cfg._instruction_mnemonic(instruction) != "mov"
            or len(operands) != 2
            or operands[0].strip().lower() != destination
        ):
            raise ValueError(
                "retail register IAT producer rejects malformed absolute MOV "
                "rendering"
            )
        if instruction_index not in transfer_addresses_by_load:
            # Absolute MOVs used for address materialization or ordinary data
            # access are not IAT call-contract facts, even if the displacement
            # happens to have another inbound code/data reference.
            continue
        slot = normalize_address(struct.unpack_from("<I", body, 2)[0])
        expression = _cc_targets._exact_memory_expression(operands[1])
        matches = [
            target
            for target in targets
            if normalize_address(str(getattr(target, "address", ""))) == slot
        ]
        if not matches:
            if storage_by_address.get(slot, "").startswith("iat:"):
                raise ValueError(
                    f"immutable retail register IAT slot {slot} has no import "
                    "route but conflicts with an existing IAT identity"
                )
            continue
        if len(matches) != 1:
            raise ValueError(
                f"immutable retail register IAT slot {slot} resolves to "
                f"{len(matches)} imports; expected one"
            )
        target = matches[0]
        import_name = getattr(target, "import_name", None)
        import_dll = getattr(target, "dll", None)
        if (
            not isinstance(import_name, str)
            or not import_name
            or import_name.startswith("#")
            or not isinstance(import_dll, str)
            or not import_dll
            or "/" in import_dll
            or "\\" in import_dll
            or getattr(target, "import_ordinal", None) is not None
        ):
            raise ValueError(
                f"immutable retail register IAT slot {slot} is not one exact "
                "named DLL import"
            )
        same_name_routes = {
            (
                normalize_address(str(getattr(item, "address", ""))),
                str(getattr(item, "dll", "")),
                str(getattr(item, "import_name", "")),
            )
            for item in targets
            if getattr(item, "import_ordinal", None) is None
            and getattr(item, "import_name", None) == import_name
        }
        if len(same_name_routes) != 1:
            raise ValueError(
                f"immutable retail named import {import_name!r} has ambiguous "
                "IAT/DLL routes"
            )
        if _cc_catalog.ADDRESS_RE.fullmatch(expression):
            if normalize_address(expression) != slot:
                raise ValueError(
                    "retail register-IAT rendered/decoded address disagreement"
                )
        elif expression != import_name:
            raise ValueError(
                f"retail register-IAT BN name {expression!r} disagrees with "
                f"immutable import {import_dll}!{import_name}"
            )
        identity = f"iat:{import_name}"
        transfer_sets_by_route.setdefault(
            (destination, identity), []
        ).append(
            frozenset(transfer_addresses_by_load[instruction_index])
        )
        definition_address = normalize_address(runtime_address)
        transfer_instruction_indices = tuple(
            instruction_index_by_address.get(
                address_value(transfer_address),
                -1,
            )
            for transfer_address in transfer_addresses_by_load[
                instruction_index
            ]
        )
        if (
            -1 in transfer_instruction_indices
            or instruction_index in transfer_instruction_indices
            or len(set(transfer_instruction_indices))
            != len(transfer_instruction_indices)
        ):
            raise ValueError(
                "retail register IAT producer requires distinct exact "
                "definition and transfer instruction rows"
            )
        proof = CandidateExactIatRegisterLoadProof(
            definition_offset=definition_address,
            destination=destination,
            object_symbol=import_name,
            identity=identity,
            transfer_offsets=transfer_addresses_by_load[instruction_index],
            definition_instruction_index=instruction_index,
            transfer_instruction_indices=transfer_instruction_indices,
            transfer_registers=tuple(_cc_instructions.instruction_fact(retail_instructions[index]).operands[0].register
                                     for index in transfer_instruction_indices),
            definition_body_offset=runtime_address - start_value,
            transfer_body_offsets=tuple(
                address_value(transfer_address) - start_value
                for transfer_address in transfer_addresses_by_load[
                    instruction_index
                ]
            ),
        )
        prior_proof = retail_iat_proofs.get(definition_address)
        if prior_proof is not None and prior_proof != proof:
            raise ValueError(
                "retail register IAT producer has conflicting exact load "
                f"proofs at {definition_address}"
            )
        retail_iat_proofs[definition_address] = proof
        for mapping, key in (
            (storage_by_address, slot),
            (storage_by_name, import_name),
        ):
            prior = mapping.get(key)
            if prior is not None and prior != identity:
                raise ValueError(
                    "retail register IAT producer conflicts with existing "
                    f"identity for {key!r}"
                )
            mapping[key] = identity
    for route, transfer_sets in transfer_sets_by_route.items():
        # Authorize only an exact same-route reload lineage.  Overlapping
        # transfer populations mean two immutable definitions can reach the
        # same phi; the abstract CFG merger still requires every incoming path
        # to carry that same identity before it materializes a join marker.
        if any(
            left & right
            for index, left in enumerate(transfer_sets)
            for right in transfer_sets[index + 1 :]
        ):
            reviewed_iat_register_joins.add(route)
    return replace(
        indexes,
        storage_by_address=storage_by_address,
        storage_by_name=storage_by_name,
        reviewed_iat_register_joins=frozenset(
            reviewed_iat_register_joins
        ),
        reviewed_retail_iat_load_proofs=(
            _unique_retail_iat_transfer_proofs(retail_iat_proofs)
        ),
    )




def _candidate_iat_load_reached_transfer_offsets(
    candidate: CandidateAssembly, *, load_index: int, destination: str,
    offsets: Sequence[int | None],
    independent_iat_definitions: Sequence[tuple[int, str, str, str]] = (),
) -> tuple[str, ...]:
    """Join byte/COFF-authenticated definitions through the shared must-flow."""
    if len(offsets) != len(candidate.instructions) or load_index not in range(len(offsets)) or offsets[load_index] is None:
        raise _cc_errors.CandidateCallContractEvidenceError("IAT definition lacks its exact instruction offset")
    counts = {offset: offsets.count(offset) for offset in set(offsets) if offset is not None}
    indices = {offset: index for index, offset in enumerate(offsets) if offset is not None and counts[offset] == 1}
    end = max((offset + len(row.bytes) for offset, row in zip(offsets, candidate.instructions) if offset is not None), default=0)
    successors, unresolved = _cc_cfg._exact_invocation_cfg(
        candidate.instructions, instruction_addresses=offsets,
        instruction_index_by_address=indices, source="cod", caller_start=0, caller_end=end,
        local_control_flow_indices=candidate.local_control_flow_indices,
        local_control_flow_targets=candidate.local_control_flow_targets)
    selected = [row for row in independent_iat_definitions if row[0] == load_index and row[1] == destination]
    if len(selected) > 1:
        raise _cc_errors.CandidateCallContractEvidenceError("IAT definition has conflicting independent producers")
    equivalents = {index: name for index, name, symbol, identity in independent_iat_definitions
                   if selected and (symbol, identity) == selected[0][2:]}
    try:
        uses = _cc_flow.reaching_definition_uses(candidate.instructions,
            _cc_flow.ControlFlow.from_edges(len(offsets), successors, unresolved),
            definition_index=load_index, register=destination, equivalent_definitions=equivalents)
    except ValueError as exc:
        raise _cc_errors.CandidateCallContractEvidenceError(f"candidate exact-IAT unresolved downstream definition proof: {exc}") from exc
    return tuple(normalize_address(offsets[index]) for index in uses)




def _candidate_exact_iat_storage_indexes(
    indexes: IdentityIndexes,
    *reviewed_storage_bridge_sets: Mapping[str, str],
    reviewed_canonical_import_names: Mapping[str, str] | None = None,
) -> IdentityIndexes:
    """Publish reviewed candidate IAT aliases for generic load proofs.

    Dedicated candidate bridges validate exact package/COD/COFF shape before
    returning a ``__imp_`` spelling.  Generic direct and register IAT consumers
    consume the immutable import-name identity, so compose those already-
    reviewed aliases into one comparison-scoped index without allowing a
    candidate spelling to replace or contradict retail-owned truth.
    """

    storage_by_name = dict(indexes.storage_by_name)
    reviewed_bridges: dict[str, str] = {}
    for bridge_set in reviewed_storage_bridge_sets:
        for object_symbol, identity in bridge_set.items():
            prior = reviewed_bridges.get(object_symbol)
            if prior is not None and prior != identity:
                raise _cc_errors.CandidateCallContractEvidenceError(
                    "candidate exact-IAT storage bridges conflict for "
                    f"{object_symbol!r}"
                )
            reviewed_bridges[object_symbol] = identity
    canonical_name_overrides = dict(reviewed_canonical_import_names or {})
    if not set(canonical_name_overrides).issubset(reviewed_bridges):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "candidate exact-IAT canonical-name proofs lack matching storage "
            "bridges"
        )
    for object_symbol, identity in reviewed_bridges.items():
        import_name = _cc_receiver_instructions._candidate_iat_import_name(object_symbol)
        if not import_name:
            continue
        identity_import_name = (
            identity.removeprefix("iat:")
            if isinstance(identity, str) and identity.startswith("iat:")
            else ""
        )
        canonical_import_name = canonical_name_overrides.get(
            object_symbol, identity_import_name
        )
        if (
            not canonical_import_name
            or canonical_import_name != identity_import_name
            or (
                canonical_import_name.casefold() != import_name.casefold()
                and not (
                    object_symbol in canonical_name_overrides
                    and canonical_import_name.startswith("_")
                    and canonical_import_name[1:].casefold()
                    == import_name.casefold()
                )
            )
        ):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "candidate exact-IAT storage bridge conflicts with its "
                f"canonical immutable import identity for {object_symbol!r}"
            )
        for key in (object_symbol, canonical_import_name):
            conflicts = sorted(
                published_key
                for published_key, published_identity in storage_by_name.items()
                if published_key.casefold() == key.casefold()
                and published_identity != identity
            )
            if conflicts:
                raise _cc_errors.CandidateCallContractEvidenceError(
                    "candidate exact-IAT storage bridge conflicts with "
                    "already-published immutable identity for "
                    f"{key!r}: {', '.join(repr(row) for row in conflicts)}"
                )
            storage_by_name[key] = identity
    return replace(indexes, storage_by_name=storage_by_name)


def _candidate_exact_iat_register_load_proofs(
    candidate: CandidateAssembly,
    *,
    indexes: IdentityIndexes,
    reference: Path = _cc_catalog.DEFAULT_REFERENCE,
    retail_import_targets: Sequence[Any] | None = None,
) -> dict[str, CandidateExactIatRegisterLoadProof]:
    """Validate candidate callable MOV/DIR32 loads against immutable IAT truth.

    Every decorated load first proves its exact listing/COFF definition and
    downstream lineage.  A load used only as ordinary imported data emits no
    callable proof and requires no callable IAT identity.  Once an exact
    indirect CALL/JMP is reached, the candidate spelling is only a consumer:
    the immutable import directory must provide one unique named route, and
    the comparison-scoped retail producer must already have published that
    route by both slot and import name before this function can emit a proof.
    """
    from _recoil.call_contract.records import CandidateExactIatRegisterLoadProof

    register_names = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    candidate_rows: list[
        tuple[int, int, Instruction, bytes, str, str]
    ] = []
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    for instruction_index, instruction in enumerate(candidate.instructions):
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            continue
        if (
            len(body) != 6
            or body[0] != 0x8B
            or body[1] >> 6 != 0
            or body[1] & 0x07 != 0x05
        ):
            continue
        operands = _cc_cfg._instruction_operand(instruction).split(",", 1)
        destination = register_names[(body[1] >> 3) & 0x07]
        if (
            _cc_cfg._instruction_mnemonic(instruction) != "mov"
            or len(operands) != 2
            or operands[0].strip().lower() != destination
        ):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "candidate exact-IAT register load has encoding/destination drift"
            )
        object_symbol = _cc_targets._exact_memory_expression(operands[1])
        import_name = _cc_receiver_instructions._candidate_iat_import_name(object_symbol)
        if not import_name:
            if "__imp_" in object_symbol:
                raise _cc_errors.CandidateCallContractEvidenceError(
                    "candidate exact-IAT register load has malformed object symbol"
                )
            continue
        offset = offsets[instruction_index] if instruction_index < len(offsets) else None
        if offset is None:
            raise _cc_errors.CandidateCallContractEvidenceError(
                "candidate exact-IAT register load lacks one exact definition site"
            )
        candidate_rows.append(
            (
                instruction_index,
                int(offset),
                instruction,
                body,
                destination,
                object_symbol,
            )
        )
    if not candidate_rows:
        return {}

    definition = candidate.caller_definition
    if (
        definition is None
        or len(offsets) != len(candidate.instructions)
        or len(definition.data) != len(definition.relocation_mask)
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "candidate exact-IAT register load requires complete COD/COFF definition evidence"
        )
    site_counts: dict[int, int] = {}
    for _instruction_index, offset, *_rest in candidate_rows:
        site_counts[offset] = site_counts.get(offset, 0) + 1

    supplied_targets = (
        tuple(retail_import_targets)
        if retail_import_targets is not None
        else tuple(_retail_import_targets(reference)[0])
    )
    validated_rows: list[
        tuple[int, int, bytes, str, str]
    ] = []
    for (
        instruction_index,
        offset,
        _instruction,
        body,
        destination,
        object_symbol,
    ) in candidate_rows:
        definition_offset = normalize_address(offset)
        import_name = _cc_receiver_instructions._candidate_iat_import_name(object_symbol)
        relocation_offset = offset + 2
        relocations = [
            row for row in definition.relocations if row.offset == relocation_offset
        ]
        external_class_counts = (
            definition.undefined_external_functions.count(object_symbol),
            definition.undefined_external_data.count(object_symbol),
            definition.defined_external_functions.count(object_symbol),
            definition.defined_external_data.count(object_symbol),
        )
        if (
            body[2:6] != b"\x00\x00\x00\x00"
            or len(relocations) != 1
            or relocations[0].type != IMAGE_REL_I386_DIR32
            or relocations[0].symbol_name != object_symbol
            or offset + len(body) > len(definition.data)
            or definition.data[offset : offset + len(body)] != body
            or relocation_offset + 4 > len(definition.data)
            or struct.unpack_from("<I", definition.data, relocation_offset)[0] != 0
            or any(definition.relocation_mask[offset:relocation_offset])
            or not all(
                definition.relocation_mask[index]
                for index in range(relocation_offset, relocation_offset + 4)
            )
            or sum(external_class_counts) != 1
            or external_class_counts[:2] not in {(1, 0), (0, 1)}
            or site_counts.get(offset) != 1
        ):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "candidate exact-IAT register load requires exact MOV/DIR32 symbol, zero addend, four-byte mask, body, and unique undefined external class"
            )
        validated_rows.append(
            (
                instruction_index,
                offset,
                body,
                destination,
                object_symbol,
            )
        )

    independent_iat_definitions: list[tuple[int, str, str, str]] = []
    for (
        instruction_index,
        _offset,
        _body,
        destination,
        object_symbol,
    ) in validated_rows:
        import_name = _cc_receiver_instructions._candidate_iat_import_name(object_symbol)
        route_rows = [
            target
            for target in supplied_targets
            if getattr(target, "import_ordinal", None) is None
            and isinstance(getattr(target, "import_name", None), str)
            and str(getattr(target, "import_name")).casefold()
            == import_name.casefold()
        ]
        route_keys = {
            (
                normalize_address(str(getattr(target, "address", ""))),
                str(getattr(target, "dll", "")),
                str(getattr(target, "import_name", "")),
            )
            for target in route_rows
        }
        if len(route_rows) != 1 or len(route_keys) != 1:
            continue
        _slot, import_dll, route_name = next(iter(route_keys))
        if (
            not import_dll
            or "/" in import_dll
            or "\\" in import_dll
            or not route_name
        ):
            continue
        # This sibling definition is only a negative-lineage guard for the
        # current load: it proves that a differently named register transfer
        # has its own unique immutable import producer.  Its own iteration
        # below must still join the retail-published IAT identity before any
        # call row is emitted, so this cannot authorize a candidate-only
        # import or let one import borrow another's identity.
        published_identity = f"iat:{route_name}"
        independent_iat_definitions.append(
            (
                instruction_index,
                destination,
                object_symbol,
                published_identity,
            )
        )

    proofs: dict[str, CandidateExactIatRegisterLoadProof] = {}
    for (
        instruction_index,
        offset,
        _body,
        destination,
        object_symbol,
    ) in validated_rows:
        definition_offset = normalize_address(offset)
        import_name = _cc_receiver_instructions._candidate_iat_import_name(object_symbol)
        transfer_offsets = _candidate_iat_load_reached_transfer_offsets(
            candidate,
            load_index=instruction_index,
            destination=destination,
            offsets=offsets,
            independent_iat_definitions=tuple(
                independent_iat_definitions
            ),
        )
        if not transfer_offsets:
            continue
        targets = supplied_targets
        canonical_rows = [
            (published_name, published_identity)
            for published_name, published_identity in indexes.storage_by_name.items()
            if not _cc_receiver_instructions._candidate_iat_import_name(published_name)
            and published_name.casefold() == import_name.casefold()
            and published_identity == f"iat:{published_name}"
        ]
        if len(canonical_rows) != 1:
            raise _cc_errors.CandidateCallContractEvidenceError(
                "candidate exact-IAT register load lacks one already-published "
                "immutable retail identity with canonical import spelling"
            )
        canonical_import_name, published_identity = canonical_rows[0]
        route_rows = [
            target
            for target in targets
            if getattr(target, "import_ordinal", None) is None
            and getattr(target, "import_name", None) == canonical_import_name
        ]
        route_keys = {
            (
                normalize_address(str(getattr(target, "address", ""))),
                str(getattr(target, "dll", "")),
                str(getattr(target, "import_name", "")),
            )
            for target in route_rows
        }
        if len(route_rows) != 1 or len(route_keys) != 1:
            raise _cc_errors.CandidateCallContractEvidenceError(
                "candidate exact-IAT register load does not consume one unique immutable import route"
            )
        slot, import_dll, _route_name = next(iter(route_keys))
        expected_identity = f"iat:{canonical_import_name}"
        if (
            not import_dll
            or "/" in import_dll
            or "\\" in import_dll
            or indexes.storage_by_address.get(slot) != expected_identity
            or indexes.storage_by_name.get(canonical_import_name)
            != expected_identity
        ):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "candidate exact-IAT register load lacks one already-published immutable retail identity"
            )
        proofs[definition_offset] = CandidateExactIatRegisterLoadProof(
            definition_offset=definition_offset,
            destination=destination,
            object_symbol=object_symbol,
            identity=expected_identity,
            transfer_offsets=transfer_offsets,
            transfer_registers=tuple(_cc_instructions.instruction_fact(candidate.instructions[offsets.index(address_value(transfer))]).operands[0].register
                                     for transfer in transfer_offsets),
        )
    return proofs


def _comparison_scoped_cached_fread_iat_indexes(
    retail_instructions: Sequence[Instruction],
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    reference: Path,
) -> IdentityIndexes:
    """Publish immutable ``fread`` IAT identity for exact retail MOV loads.

    The publication is comparison-scoped and caller-name independent.  It
    supplies identity to the generic definition-sensitive register CFG; that
    CFG remains responsible for dominance, joins, clobbers, loops, indirect
    form, and cleanup.  No tracker storage row is invented for the IAT slot.
    """
    from _recoil.call_contract.records import CandidateExactIatRegisterLoadProof

    start = normalize_address(caller_start)
    end = normalize_address(caller_end_exclusive)
    addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(start),
    )
    address_counts: dict[int, int] = {}
    for address in addresses:
        if address is not None:
            address_counts[address] = address_counts.get(address, 0) + 1
    instruction_index_by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and address_counts.get(address) == 1
    }
    exact_slot_loads: list[tuple[int, int, Instruction, str]] = []
    for instruction_index, (runtime_address, instruction) in enumerate(
        zip(addresses, retail_instructions)
    ):
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            body = b""
        decoded_slot = (
            len(body) == 6
            and body[0] == 0x8B
            and body[1] >> 6 == 0
            and body[1] & 0x07 == 0x05
            and normalize_address(struct.unpack_from("<I", body, 2)[0])
            == _cc_catalog.CACHED_FREAD_IAT_ADDRESS
        )
        rendered_slot = _cc_catalog.CACHED_FREAD_IAT_ADDRESS in tuple(
            normalize_address(match)
            for match in _cc_catalog.ADDRESS_RE.findall(_cc_cfg._instruction_operand(instruction))
        )
        if not decoded_slot and not rendered_slot:
            continue
        if runtime_address is None:
            raise ValueError(
                "cached fread IAT metadata producer requires an exact retail "
                "load address"
            )
        operands = _cc_cfg._instruction_operand(instruction).split(",", 1)
        destination = operands[0].strip().lower() if len(operands) == 2 else ""
        exact_memory = (
            _cc_targets._exact_memory_expression(operands[1]) if len(operands) == 2 else ""
        )
        definition_address = normalize_address(runtime_address)
        _cc_receiver_storage._exact_iat_register_load_provenance(
            instruction,
            assembly_source="bn",
            destination=destination,
            exact_memory=exact_memory,
            rendered_address=_cc_catalog.CACHED_FREAD_IAT_ADDRESS,
            identity=_cc_catalog.CACHED_FREAD_IAT_IDENTITY,
            indexes=indexes,
            instruction_address=definition_address,
            reviewed_identity=True,
        )
        exact_slot_loads.append(
            (
                instruction_index,
                runtime_address,
                instruction,
                definition_address,
            )
        )
    if not exact_slot_loads:
        return indexes
    definition_addresses = [row[3] for row in exact_slot_loads]
    if len(set(definition_addresses)) != len(definition_addresses):
        raise ValueError(
            "cached fread IAT metadata producer rejects duplicate retail load "
            "addresses"
        )

    caller_id = caller_identity.removeprefix("symbol:")
    caller = document.collection("symbols").get(caller_id)
    if (
        not caller_identity.startswith("symbol:recoil:function:")
        or not isinstance(caller, Mapping)
        or caller.get("binary") != "recoil"
        or caller.get("kind") != "function"
        or caller.get("pipeline_class") != "authored"
        or caller.get("ownership_state") != "primary-owned"
        or normalize_address(str(caller.get("address", ""))) != start
        or normalize_address(str(caller.get("end_exclusive", ""))) != end
        or caller.get("extent_state") != "known"
        or caller.get("size")
        != address_value(end) - address_value(start)
        or not isinstance(caller.get("physical_block_id"), str)
        or not caller.get("physical_block_id")
        or indexes.by_address.get(start) != caller_identity
        or caller_identity in indexes.provider_ids
    ):
        raise ValueError(
            "cached fread IAT metadata producer requires the current authored "
            "caller identity and exact registered extent"
        )

    retail_import, _directory_context = retail_import_target(
        reference=reference,
        address=_cc_catalog.CACHED_FREAD_IAT_ADDRESS,
        dll=_cc_catalog.CACHED_FREAD_IMPORT_DLL,
        import_name=_cc_catalog.CACHED_FREAD_IMPORT_NAME,
    )
    if (
        retail_import.address != _cc_catalog.CACHED_FREAD_IAT_ADDRESS
        or retail_import.dll != _cc_catalog.CACHED_FREAD_IMPORT_DLL
        or retail_import.import_name != _cc_catalog.CACHED_FREAD_IMPORT_NAME
        or retail_import.import_ordinal is not None
    ):
        raise ValueError(
            "cached fread IAT metadata producer immutable retail import route "
            "drifted"
        )

    storage_by_address = dict(indexes.storage_by_address)
    storage_by_name = dict(indexes.storage_by_name)
    retail_iat_proofs = dict(indexes.reviewed_retail_iat_load_proofs)
    for mapping, key in (
        (storage_by_address, _cc_catalog.CACHED_FREAD_IAT_ADDRESS),
        (storage_by_name, _cc_catalog.CACHED_FREAD_IMPORT_NAME),
        (storage_by_name, _cc_catalog.CACHED_FREAD_CANDIDATE_SYMBOL),
    ):
        prior = mapping.get(key)
        if prior is not None and prior != _cc_catalog.CACHED_FREAD_IAT_IDENTITY:
            raise ValueError(
                "cached fread IAT metadata producer conflicts with existing "
                f"storage identity for {key!r}"
            )
        mapping[key] = _cc_catalog.CACHED_FREAD_IAT_IDENTITY
    register_names = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    for instruction_index, _runtime, instruction, definition_address in exact_slot_loads:
        body = bytes(int(item, 16) for item in instruction.bytes)
        destination = register_names[(body[1] >> 3) & 7]
        prior = retail_iat_proofs.get(definition_address)
        if prior is not None:
            expected_body_offset = (
                address_value(definition_address) - address_value(start)
            )
            if (
                prior.definition_offset != definition_address
                or prior.destination != destination
                or prior.object_symbol != _cc_catalog.CACHED_FREAD_IMPORT_NAME
                or prior.identity != _cc_catalog.CACHED_FREAD_IAT_IDENTITY
                or prior.definition_instruction_index != instruction_index
                or prior.definition_body_offset != expected_body_offset
                or not prior.transfer_offsets
                or len(prior.transfer_offsets)
                != len(prior.transfer_instruction_indices)
                or len(prior.transfer_offsets)
                != len(prior.transfer_body_offsets)
            ):
                raise ValueError(
                    "cached fread retail IAT proof conflicts at "
                    f"{definition_address}"
                )
            # The generic caller package already proved this exact immutable
            # definition with the complete local switch/cleanup context.  Do
            # not recompute a second, weaker caller-local transfer population
            # and then reject the two sound projections as a collision.
            continue
        transfers = _cc_dispatch._retail_register_definition_transfer_addresses(
            retail_instructions,
            load_index=instruction_index,
            destination=destination,
            caller_start=address_value(start),
            caller_end=address_value(end),
            equivalent_absolute_load_address=_cc_catalog.CACHED_FREAD_IAT_ADDRESS,
        )
        if not transfers:
            continue
        transfer_instruction_indices = tuple(
            instruction_index_by_address.get(address_value(transfer), -1)
            for transfer in transfers
        )
        if (
            -1 in transfer_instruction_indices
            or instruction_index in transfer_instruction_indices
            or len(set(transfer_instruction_indices))
            != len(transfer_instruction_indices)
        ):
            raise ValueError(
                "cached fread retail IAT proof requires distinct exact "
                "definition and transfer instruction rows"
            )
        proof = CandidateExactIatRegisterLoadProof(
            definition_offset=definition_address,
            destination=destination,
            object_symbol=_cc_catalog.CACHED_FREAD_IMPORT_NAME,
            identity=_cc_catalog.CACHED_FREAD_IAT_IDENTITY,
            transfer_offsets=transfers,
            definition_instruction_index=instruction_index,
            transfer_instruction_indices=transfer_instruction_indices,
            transfer_registers=tuple(_cc_instructions.instruction_fact(retail_instructions[index]).operands[0].register
                                     for index in transfer_instruction_indices),
            definition_body_offset=(
                address_value(definition_address) - address_value(start)
            ),
            transfer_body_offsets=tuple(
                address_value(transfer) - address_value(start)
                for transfer in transfers
            ),
        )
        prior = retail_iat_proofs.get(definition_address)
        if prior not in {None, proof}:
            raise ValueError(
                "cached fread retail IAT proof conflicts at "
                f"{definition_address}"
            )
        retail_iat_proofs[definition_address] = proof
    return replace(
        indexes,
        storage_by_address=storage_by_address,
        storage_by_name=storage_by_name,
        reviewed_retail_iat_load_proofs=(
            _unique_retail_iat_transfer_proofs(retail_iat_proofs)
        ),
    )


def _cached_fread_retail_iat_indexes(
    retail_instructions: Sequence[Instruction],
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    reference: Path = _cc_catalog.DEFAULT_REFERENCE,
) -> IdentityIndexes:
    """Publish the exact cached ``fread`` IAT identity for retail comparison.

    This is comparison-scoped expected truth, not a provider registration.
    The generic producer above handles exact absolute register loads in any
    current authored caller.  The original LoadFromStream scope additionally
    retains its reviewed four-call/load/cleanup census below.  Candidate
    symbols, bytes, and call population never supply expected truth here.
    """
    normalized_start = normalize_address(caller_start)
    indexes = _comparison_scoped_cached_fread_iat_indexes(
        retail_instructions,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        reference=reference,
    )
    if normalized_start != _cc_catalog.CACHED_FREAD_CALLER_START:
        return indexes
    caller = document.collection("symbols").get(
        _cc_catalog.CACHED_FREAD_CALLER_IDENTITY.removeprefix("symbol:")
    )
    trace = (
        caller.get("source_traceability")
        if isinstance(caller, Mapping)
        else None
    )
    source_edges = (
        trace.get("source_edges") if isinstance(trace, Mapping) else None
    )
    if (
        caller_identity != _cc_catalog.CACHED_FREAD_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.CACHED_FREAD_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(_cc_catalog.CACHED_FREAD_CALLER_START)
        != _cc_catalog.CACHED_FREAD_CALLER_IDENTITY
        or _cc_catalog.CACHED_FREAD_CALLER_IDENTITY in indexes.provider_ids
        or not isinstance(caller, Mapping)
        or caller.get("binary") != "recoil"
        or caller.get("kind") != "function"
        or caller.get("pipeline_class") != "authored"
        or caller.get("ownership_state") != "primary-owned"
        or caller.get("address") != _cc_catalog.CACHED_FREAD_CALLER_START
        or caller.get("end_exclusive")
        != _cc_catalog.CACHED_FREAD_CALLER_END_EXCLUSIVE
        or caller.get("extent_state") != "known"
        or caller.get("size") != 0xC0
        or caller.get("navigation_name") != _cc_catalog.CACHED_FREAD_CALLER_NAME
        or caller.get("output_section_id") != "recoil:section:.text"
        or caller.get("physical_block_id")
        != _cc_catalog.CACHED_FREAD_CALLER_PHYSICAL_BLOCK_ID
        or tuple(caller.get("verification_target_ids", ()))
        != _cc_catalog.CACHED_FREAD_CALLER_TARGET_IDS
        or not isinstance(trace, Mapping)
        or trace.get("state") != "resolved"
        or trace.get("reason_code") not in {None, ""}
        or not isinstance(source_edges, list)
        or len(source_edges) != 1
        or not isinstance(source_edges[0], Mapping)
        or source_edges[0].get("relation") != "defines"
        or source_edges[0].get("anchor_id")
        != _cc_catalog.CACHED_FREAD_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.CACHED_FREAD_CALLER_SOURCE_PATH}
    ):
        raise ValueError(
            "cached fread IAT bridge requires the exact current tracker "
            "authored caller identity and source scope"
        )

    retail_import, _directory_context = retail_import_target(
        reference=reference,
        address=_cc_catalog.CACHED_FREAD_IAT_ADDRESS,
        dll=_cc_catalog.CACHED_FREAD_IMPORT_DLL,
        import_name=_cc_catalog.CACHED_FREAD_IMPORT_NAME,
    )
    if (
        retail_import.address != _cc_catalog.CACHED_FREAD_IAT_ADDRESS
        or retail_import.dll != _cc_catalog.CACHED_FREAD_IMPORT_DLL
        or retail_import.import_name != _cc_catalog.CACHED_FREAD_IMPORT_NAME
        or retail_import.import_ordinal is not None
    ):
        raise ValueError(
            "cached fread IAT bridge immutable retail import tuple drifted"
        )

    addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(_cc_catalog.CACHED_FREAD_CALLER_START),
    )
    address_counts: dict[int, int] = {}
    for address in addresses:
        if address is not None:
            address_counts[address] = address_counts.get(address, 0) + 1
    by_address = {
        normalize_address(address): (index, instruction)
        for index, (address, instruction) in enumerate(
            zip(addresses, retail_instructions)
        )
        if address is not None and address_counts.get(address) == 1
    }
    load_row = by_address.get(_cc_catalog.CACHED_FREAD_LOAD_ADDRESS)
    call_rows = tuple(
        by_address.get(address) for address in _cc_catalog.CACHED_FREAD_CALL_ADDRESSES
    )
    if load_row is None or any(row is None for row in call_rows):
        raise ValueError(
            "cached fread IAT bridge requires the exact unique retail load "
            "and four call addresses"
        )
    load_index, load = load_row
    exact_call_rows = tuple(row for row in call_rows if row is not None)
    call_indices = tuple(row[0] for row in exact_call_rows)
    rendered_load = _cc_cfg._instruction_operand(load)
    if (
        _cc_cfg._instruction_mnemonic(load) != "mov"
        or rendered_load.lower()
        != "ebp, dword [0x4cc4e0]"
        or tuple(value.lower() for value in load.bytes)
        != ("8b", "2d", "e0", "c4", "4c", "00")
    ):
        raise ValueError(
            "cached fread IAT bridge rejects retail load rendering/bytes: "
            f"operand={rendered_load!r}, bytes={load.bytes!r}"
        )
    slot_load_addresses = tuple(
        normalize_address(address)
        for address, instruction in zip(addresses, retail_instructions)
        if (
            address is not None
            and _cc_cfg._instruction_mnemonic(instruction) == "mov"
            and _cc_catalog.CACHED_FREAD_IAT_ADDRESS
            in _cc_cfg._instruction_operand(instruction).lower()
        )
    )
    if slot_load_addresses != (_cc_catalog.CACHED_FREAD_LOAD_ADDRESS,):
        raise ValueError(
            "cached fread IAT bridge rejects retail IAT load population: "
            f"{slot_load_addresses!r}"
        )
    register_call_addresses = tuple(
        normalize_address(address)
        for address, instruction in zip(addresses, retail_instructions)
        if (
            address is not None
            and _cc_cfg._instruction_mnemonic(instruction) == "call"
            and _cc_cfg._instruction_operand(instruction).lower() == "ebp"
        )
    )
    if register_call_addresses != _cc_catalog.CACHED_FREAD_CALL_ADDRESSES:
        raise ValueError(
            "cached fread IAT bridge rejects retail call-ebp population: "
            f"{register_call_addresses!r}"
        )
    call_shapes = tuple(
        (
            _cc_cfg._instruction_mnemonic(instruction),
            _cc_cfg._instruction_operand(instruction),
            tuple(value.lower() for value in instruction.bytes),
        )
        for _index, instruction in exact_call_rows
    )
    if any(
        mnemonic != "call"
        or operand.lower() != "ebp"
        or body != ("ff", "d5")
        for mnemonic, operand, body in call_shapes
    ):
        raise ValueError(
            "cached fread IAT bridge rejects retail call-ebp rendering/bytes: "
            f"{call_shapes!r}"
        )
    cleanups = tuple(
        _cc_cfg._cleanup_after(retail_instructions, index)
        for index in call_indices
    )
    if cleanups != (16, 16, 16, 16):
        raise ValueError(
            "cached fread IAT bridge rejects retail caller cleanup: "
            f"{cleanups!r}"
        )
    # Path-local epilogues pop EBP before exact returns between later static
    # call sites.  The forward-CFG extractor below owns reaching-definition
    # proof: it kills those return fallthroughs, merges branch states by the
    # exact load-site marker, and rejects only clobbers that can reach a call.

    storage_by_address = dict(indexes.storage_by_address)
    storage_by_name = dict(indexes.storage_by_name)
    for mapping, key in (
        (storage_by_address, _cc_catalog.CACHED_FREAD_IAT_ADDRESS),
        (storage_by_name, _cc_catalog.CACHED_FREAD_IMPORT_NAME),
        (storage_by_name, _cc_catalog.CACHED_FREAD_CANDIDATE_SYMBOL),
    ):
        prior = mapping.get(key)
        if prior is not None and prior != _cc_catalog.CACHED_FREAD_IAT_IDENTITY:
            raise ValueError(
                "cached fread IAT bridge conflicts with an existing storage "
                f"identity for {key!r}"
            )
        mapping[key] = _cc_catalog.CACHED_FREAD_IAT_IDENTITY
    return replace(
        indexes,
        storage_by_address=storage_by_address,
        storage_by_name=storage_by_name,
    )


def _immediate_entry_register_targets(
    caller_rows: Sequence[Instruction],
    *,
    call_address: str,
    indexes: IdentityIndexes,
    callable_registers: frozenset[str],
) -> dict[str, str]:
    """Resolve only immediates whose callee entry lineage is callable."""

    addresses = _cc_cfg._instruction_runtime_addresses(
        caller_rows,
        source="bn",
        caller_start=0,
    )
    call_indices = [
        index for index, address in enumerate(addresses)
        if address == address_value(call_address)
    ]
    if len(call_indices) != 1:
        raise ValueError("BN inbound caller has an ambiguous direct call address")
    call_index = call_indices[0]
    register_names = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    result: dict[str, str] = {}
    cursor = address_value(call_address)
    index = call_index - 1
    while index >= 0:
        instruction = caller_rows[index]
        address = addresses[index]
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            body = b""
        if address is None or address + len(body) != cursor:
            break
        if len(body) != 5 or not 0xB8 <= body[0] <= 0xBF:
            break
        register = register_names[body[0] - 0xB8]
        operands = _cc_cfg._instruction_operand(instruction).split(",", 1)
        if (
            _cc_cfg._instruction_mnemonic(instruction) != "mov"
            or len(operands) != 2
            or operands[0].strip().lower() != register
        ):
            raise ValueError(
                "BN inbound caller immediate definition rendering drifted"
            )
        target = normalize_address(struct.unpack_from("<I", body, 1)[0])
        rendered = _cc_catalog.ADDRESS_RE.findall(operands[1])
        if rendered and normalize_address(rendered[-1]) != target:
            raise ValueError(
                "BN inbound caller immediate definition address drifted"
            )
        if register not in callable_registers:
            cursor = address
            index -= 1
            continue
        identity = indexes.reviewed_icf_group_by_address.get(target, "")
        if not identity:
            aliases = indexes.reviewed_logical_aliases_by_address.get(target, ())
            alias_identities = {alias.identity for alias in aliases}
            if len(alias_identities) > 1:
                raise ValueError(
                    "BN inbound caller immediate target has ambiguous logical aliases"
                )
            identity = (
                next(iter(alias_identities))
                if alias_identities
                else indexes.by_address.get(target, "")
            )
        if not identity:
            raise ValueError(
                f"BN inbound caller immediate target {target} has no reviewed identity"
            )
        if register in result and result[register] != identity:
            raise ValueError(
                f"BN inbound caller has ambiguous immediate definitions for {register}"
            )
        result[register] = identity
        cursor = address
        index -= 1
    return {
        register: identity
        for register, identity in result.items()
        if register in callable_registers
    }


def _gettickcount_candidate_retail_iat_equivalences(
    thunk: ProviderNamedImportThunk | None,
    direct_bridges: Mapping[str, str],
    register_storage_bridges: Mapping[str, str],
) -> dict[str, str]:
    """Expose exact candidate-to-retail identity after dedicated proof.

    The dedicated bridge above owns caller scope, bytes, call population,
    package, provenance, and collision checks.  This helper returns only the
    final comparison identity for one reviewed candidate form and rejects an
    incomplete or mixed package.
    """

    if not direct_bridges and not register_storage_bridges:
        return {}
    if thunk is None:
        raise ValueError(
            "GetTickCount comparison equivalence has no immutable retail thunk"
        )
    exact_direct = {
        _cc_catalog.GETTICKCOUNT_CANDIDATE_CALLABLE: thunk.provider_identity
    }
    exact_register = {
        _cc_catalog.GETTICKCOUNT_CANDIDATE_IMPORT_SYMBOL: thunk.iat_identity
    }
    if direct_bridges == exact_direct and not register_storage_bridges:
        return {thunk.provider_identity: thunk.iat_identity}
    if register_storage_bridges == exact_register and not direct_bridges:
        return {thunk.iat_identity: thunk.iat_identity}
    raise ValueError(
        "GetTickCount comparison equivalence requires one exact unmixed "
        "direct-provider or register-IAT candidate package"
    )
