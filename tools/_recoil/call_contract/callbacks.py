"""Recoil call-contract callbacks evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import dispatch as _cc_dispatch
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import instructions as _cc_instructions
from _recoil.call_contract import listing as _cc_listing
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions
from _recoil.call_contract import recoil_input as _cc_recoil_input
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        IdentityIndexes,
        ReviewedExactIndirectStorageBridge,
    )

import re
import struct
from collections import Counter
from dataclasses import replace
from typing import Any, Callable, Mapping, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_DIR32,
    IMAGE_SYM_CLASS_EXTERNAL,
    Instruction,
)
from _recoil.lib.binja import BinaryNinjaBridge
from _recoil.lib.progress import ProgressDocument, address_value, normalize_address


def _comparison_scoped_retail_static_callback_targets(
    retail_instructions: Sequence[Instruction],
    *,
    indexes: IdentityIndexes,
    bridge: BinaryNinjaBridge,
) -> IdentityIndexes:
    """Bind exact four-byte retail callback cells to reviewed functions.

    The callsite must be an exact unprefixed ``FF 15 [abs32]`` through one
    reviewed non-IAT storage container whose extent is exactly four bytes.
    The immutable cell bytes must contain one already-indexed function target.
    Candidate evidence is absent; candidate extraction may only consume the
    resulting storage-to-target relationship.
    """

    reviewed = dict(indexes.reviewed_static_callback_target_by_storage)
    for instruction in retail_instructions:
        if _cc_cfg._instruction_mnemonic(instruction) != "call":
            continue
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            continue
        if len(body) != 6 or body[:2] != b"\xff\x15":
            continue
        address = normalize_address(struct.unpack_from("<I", body, 2)[0])
        value = address_value(address)
        containers = [
            container
            for container in indexes.storage_containers
            if (
                container.start == value
                and container.end_exclusive == value + 4
            )
        ]
        if len(containers) != 1:
            continue
        storage_identity = containers[0].identity
        if (
            not storage_identity
            or storage_identity.startswith("iat:")
            or indexes.storage_by_address.get(address) != storage_identity
        ):
            continue
        expression = _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(instruction)
        )
        if (
            expression != address
            and indexes.storage_by_name.get(expression) != storage_identity
        ):
            raise ValueError(
                "retail static callback cell rendering/address identity drift"
            )
        cell = _cc_cfg._hexdump_bytes(bridge.hexdump(address, 4))
        if len(cell) != 4:
            raise ValueError(
                "retail static callback cell lacks exact four-byte contents"
            )
        target_address = normalize_address(struct.unpack_from("<I", cell)[0])
        target_identity = indexes.by_address.get(target_address, "")
        if not target_identity:
            continue
        prior = reviewed.get(storage_identity)
        if prior not in {None, target_identity}:
            raise ValueError(
                "retail static callback storage resolves to conflicting "
                "function targets"
            )
        reviewed[storage_identity] = target_identity
    return replace(
        indexes,
        reviewed_static_callback_target_by_storage=reviewed,
    )


def _comparison_scoped_retail_zero_callback_storage_indexes(
    retail_instructions: Sequence[Instruction],
    *,
    document: ProgressDocument,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge: BinaryNinjaBridge,
    local_control_flow_indices: frozenset[int] = frozenset(),
    local_control_flow_targets: Mapping[int, Sequence[int]] | None = None,
    precomposed_non_callback_loads: Mapping[str, str] | None = None,
    call_cleanup_by_instruction_index: Mapping[int, int] | None = None,
    _trace_overflow: Callable[[Mapping[str, Any]], None] | None = None,
) -> IdentityIndexes:
    """Publish exact targetless storage for immutable zero callback cells.

    A zero-initialized callback deliberately has no static target to bind.  It
    is nevertheless exact storage when one unprefixed absolute ``MOV r32,
    [imm32]`` reaches one exact register invocation, the reviewed storage
    container is exactly four bytes, and immutable retail bytes are all zero.
    Candidate evidence and BN spelling cannot supply any part of this proof.
    """

    start = address_value(caller_start)
    end = address_value(caller_end_exclusive)
    register_names = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    precomposed = {
        normalize_address(address): identity
        for address, identity in dict(
            precomposed_non_callback_loads or {}
        ).items()
    }
    if len(precomposed) != len(precomposed_non_callback_loads or {}):
        raise ValueError("precomposed non-callback load definitions collide")
    addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=start,
    )
    counts = Counter(
        address
        for address in addresses
        if address is not None and start <= address < end
    )
    consumed_precomposed: set[str] = set()
    storage_by_address = dict(indexes.storage_by_address)
    for instruction_index, (runtime_address, instruction) in enumerate(
        zip(addresses, retail_instructions)
    ):
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            continue
        accumulator = len(body) == 5 and body[0] == 0xA1
        modrm_absolute = (
            len(body) == 6
            and body[0] == 0x8B
            and body[1] >> 6 == 0
            and body[1] & 7 == 5
        )
        if not accumulator and not modrm_absolute:
            if (
                runtime_address is not None
                and normalize_address(runtime_address) in precomposed
            ):
                raise ValueError(
                    "precomposed non-callback definition is not an exact "
                    "absolute MOV"
                )
            continue
        destination = (
            "eax"
            if accumulator
            else register_names[(body[1] >> 3) & 7]
        )
        cell_address = normalize_address(
            struct.unpack_from("<I", body, 1 if accumulator else 2)[0]
        )
        definition_address = (
            normalize_address(runtime_address)
            if runtime_address is not None
            and counts.get(runtime_address) == 1
            else ""
        )
        if definition_address in precomposed:
            storage_identity = indexes.storage_by_address.get(cell_address, "")
            value = address_value(cell_address)
            containers = [
                container
                for container in indexes.storage_containers
                if container.start == value
                and container.end_exclusive == value + 4
                and container.identity == storage_identity
            ]
            aggregate_field_is_bounded = (
                not containers
                and _cc_recoil_input._zinput_runtime_aggregate_field_is_bounded(
                    document=document,
                    indexes=indexes,
                    caller_start=caller_start,
                    storage_address=cell_address,
                    storage_identity=storage_identity,
                )
            )
            if (
                not storage_identity
                or storage_identity.startswith("iat:")
                or precomposed[definition_address] != storage_identity
                or (
                    len(containers) != 1
                    and not aggregate_field_is_bounded
                )
            ):
                raise ValueError(
                    "precomposed non-callback definition storage drifted at "
                    f"{definition_address}"
                )
            consumed_precomposed.add(definition_address)
            continue
        value = address_value(cell_address)
        containers = [
            container
            for container in indexes.storage_containers
            if container.start == value
            and container.end_exclusive == value + 4
        ]
        if not containers:
            continue
        if len(containers) != 1:
            raise ValueError(
                "retail zero callback cell has ambiguous exact storage extent"
            )
        storage_identity = containers[0].identity
        if not storage_identity or storage_identity.startswith("iat:"):
            continue
        transfers = _cc_dispatch._retail_register_definition_transfer_addresses(
            retail_instructions,
            load_index=instruction_index,
            destination=destination,
            caller_start=start,
            caller_end=end,
            allow_exact_zero_guard=True,
            bridge=bridge,
            local_control_flow_indices=local_control_flow_indices,
            local_control_flow_targets=local_control_flow_targets,
            call_cleanup_by_instruction_index=(
                call_cleanup_by_instruction_index
            ),
            _trace_overflow=_trace_overflow,
        )
        if not transfers:
            continue
        operands = _cc_cfg._instruction_operand(instruction).split(",", 1)
        expression = (
            _cc_targets._exact_memory_expression(operands[1])
            if len(operands) == 2
            else ""
        )
        if (
            _cc_cfg._instruction_mnemonic(instruction) != "mov"
            or len(operands) != 2
            or operands[0].strip().lower() != destination
            or (
                expression != cell_address
                and indexes.storage_by_name.get(expression)
                != storage_identity
            )
        ):
            raise ValueError(
                "retail zero callback load rendering/address identity drift"
            )
        cell = _cc_cfg._hexdump_bytes(bridge.hexdump(cell_address, 4))
        if len(cell) != 4:
            raise ValueError(
                "retail zero callback cell lacks exact four-byte contents"
            )
        if cell != b"\x00\x00\x00\x00":
            continue
        if len(transfers) != 1:
            raise ValueError(
                "retail zero callback cell must reach one exact static "
                "register invocation"
            )
        prior = storage_by_address.get(cell_address)
        if prior not in {None, storage_identity}:
            raise ValueError(
                "retail zero callback cell conflicts with existing storage "
                "identity"
            )
        storage_by_address[cell_address] = storage_identity
    unused = precomposed.keys() - consumed_precomposed
    if unused:
        raise ValueError(
            "precomposed non-callback load publication is unused: "
            + ", ".join(sorted(unused, key=address_value))
        )
    return replace(indexes, storage_by_address=storage_by_address)


def _comparison_scoped_retail_stored_callback_targets(
    retail_instructions: Sequence[Instruction],
    *,
    document: ProgressDocument,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge: BinaryNinjaBridge,
    local_control_flow_indices: frozenset[int] = frozenset(),
    local_control_flow_targets: Mapping[int, Sequence[int]] | None = None,
    precomposed_non_callback_loads: Mapping[str, str] | None = None,
    call_cleanup_by_instruction_index: Mapping[int, int] | None = None,
    _trace_overflow: Callable[[Mapping[str, Any]], None] | None = None,
) -> IdentityIndexes:
    """Validate every reader/writer of one initially-zero callback cell.

    Complete BN xrefs must classify without collision as exact absolute loads
    reaching one path-proved invocation or exact absolute immediate/register
    stores.  A complete reader-only population is an optional callback that
    remains targetless.  A sole ``C7 05 cell,target`` writer may publish a
    static target; register-written and shared cells remain targetless
    callbacks.  Missing readers, extra, malformed, data, mixed-static/dynamic,
    and unresolved CFG evidence fails closed.  Candidate output supplies none
    of this expected truth.
    """

    get_json = getattr(bridge, "get_json", None)
    if not callable(get_json):
        return indexes
    start = address_value(caller_start)
    end = address_value(caller_end_exclusive)
    addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=start,
    )
    register_names = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    precomposed = {
        normalize_address(address): identity
        for address, identity in dict(
            precomposed_non_callback_loads or {}
        ).items()
    }
    if len(precomposed) != len(precomposed_non_callback_loads or {}):
        raise ValueError(
            "stored-callback precomposed non-callback definitions collide"
        )
    consumed_precomposed: set[str] = set()
    reviewed = dict(indexes.reviewed_static_callback_target_by_storage)
    reviewed_dynamic_export_cells: Mapping[str, Mapping[str, Any]] = {
        "0x56b568": {
            "writer_name": "zLoc::LoadMessagesDll",
            "writer_start": 0x4A5AD0,
            "writer_end": 0x4A5AFD,
            "literal_address": "0x4e2ff8",
            "literal_bytes": b"ZLocGetID\x00",
            "getproc_call": "0x4a5aee",
            "getproc_iat": "0x4cc0bc",
            "getproc_identity": "iat:GetProcAddress",
            "store": "0x4a5af4",
            "reader_start": 0x4A5B20,
            "reader_end": 0x4A5B33,
        },
    }
    for load_index, (load_address, instruction) in enumerate(
        zip(addresses, retail_instructions)
    ):
        if load_address is None or not start <= load_address < end:
            continue
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            continue
        accumulator = len(body) == 5 and body[0] == 0xA1
        modrm_absolute = (
            len(body) == 6
            and body[0] == 0x8B
            and body[1] >> 6 == 0
            and body[1] & 7 == 5
        )
        if not accumulator and not modrm_absolute:
            continue
        destination = (
            "eax" if accumulator else register_names[(body[1] >> 3) & 7]
        )
        cell_address = normalize_address(
            struct.unpack_from("<I", body, 1 if accumulator else 2)[0]
        )
        storage_identity = indexes.storage_by_address.get(cell_address, "")
        value = address_value(cell_address)
        containers = [
            container
            for container in indexes.storage_containers
            if container.start == value
            and container.end_exclusive == value + 4
            and container.identity == storage_identity
        ]
        definition_address = normalize_address(load_address)
        if definition_address in precomposed:
            aggregate_field_is_bounded = (
                not containers
                and _cc_recoil_input._zinput_runtime_aggregate_field_is_bounded(
                    document=document,
                    indexes=indexes,
                    caller_start=caller_start,
                    storage_address=cell_address,
                    storage_identity=storage_identity,
                )
            )
            if (
                not storage_identity
                or storage_identity.startswith("iat:")
                or precomposed[definition_address] != storage_identity
                or (
                    len(containers) != 1
                    and not aggregate_field_is_bounded
                )
            ):
                raise ValueError(
                    "stored-callback precomposed non-callback storage drifted "
                    f"at {definition_address}"
                )
            consumed_precomposed.add(definition_address)
            continue
        if len(containers) != 1 or not storage_identity:
            if cell_address in reviewed_dynamic_export_cells:
                raise ValueError(
                    f"retail callback cell {cell_address} dynamic export "
                    "storage identity or exact four-byte extent drifted"
                )
            continue
        initial_cell = _cc_cfg._hexdump_bytes(bridge.hexdump(cell_address, 4))
        if initial_cell != b"\x00\x00\x00\x00":
            # A nonzero or unreadable cell is not an initially-empty mutable
            # callback slot.  In particular, ordinary IAT/provider cells must
            # remain available to the existing import-lineage producers.
            continue
        if storage_identity.startswith("iat:"):
            # Conflict gates are callback-only: they activate after exact
            # initial-zero evidence, never merely because a MOV/register-call
            # shape happens to read a four-byte cell.
            raise ValueError(
                f"retail callback cell {cell_address} conflicts with an import identity"
            )
        transfers = _cc_dispatch._retail_register_definition_transfer_addresses(
            retail_instructions,
            load_index=load_index,
            destination=destination,
            caller_start=start,
            caller_end=end,
            allow_exact_zero_guard=True,
            bridge=bridge,
            local_control_flow_indices=local_control_flow_indices,
            local_control_flow_targets=local_control_flow_targets,
            call_cleanup_by_instruction_index=(
                call_cleanup_by_instruction_index
            ),
            _trace_overflow=_trace_overflow,
        )
        if len(transfers) != 1:
            continue
        payload = get_json(
            "getXrefsTo",
            address=cell_address,
            limit=100000,
        )
        code_rows, data_rows, complete = _cc_identity._bn_inbound_xref_items(payload)
        if not complete:
            raise ValueError(
                f"retail callback cell {cell_address} has incomplete inbound xrefs"
            )
        source_addresses = tuple(
            _cc_identity._bn_xref_address(
                row,
                "source_address",
                "source_addr",
                "from_address",
                "address",
            )
            for row in code_rows
        )
        normalized_load = normalize_address(load_address)
        if normalized_load not in source_addresses:
            raise ValueError(
                f"retail callback cell {cell_address} complete xrefs omit "
                "the activating reader"
            )
        if (
            data_rows
            or not source_addresses
            or "" in source_addresses
            or len(set(source_addresses)) != len(source_addresses)
        ):
            raise ValueError(
                f"retail callback cell {cell_address} has an ambiguous or "
                "colliding complete xref population"
            )

        assembly_cache: dict[str, tuple[Instruction, ...]] = {}

        def containing_rows(
            source_address: str,
        ) -> tuple[tuple[Instruction, ...], int, int, int, str]:
            if source_address == normalized_load:
                rows = tuple(retail_instructions)
                row_start = start
                row_end = end
                function_name = ""
            else:
                try:
                    function_name, function_start = _cc_identity._bn_unique_containing_function(
                        bridge,
                        instruction_address=source_address,
                    )
                except ValueError as exc:
                    raise ValueError(
                        f"retail callback cell {cell_address} lacks a "
                        "complete validated reader/writer xref population"
                    ) from exc
                rows = assembly_cache.get(function_start, ())
                if not rows:
                    rows = tuple(
                        _cc_listing.parse_assembly(
                            bridge.assembly(function_start),
                            source="bn",
                        )
                    )
                    assembly_cache[function_start] = rows
                row_addresses = _cc_cfg._instruction_runtime_addresses(
                    rows,
                    source="bn",
                    caller_start=address_value(function_start),
                )
                exact_ends: list[int] = []
                for address, row in zip(row_addresses, rows):
                    try:
                        row_size = len(bytes(int(item, 16) for item in row.bytes))
                    except (TypeError, ValueError):
                        row_size = 0
                    if address is not None and row_size:
                        exact_ends.append(address + row_size)
                if not exact_ends:
                    raise ValueError(
                        f"retail callback cell {cell_address} has an empty "
                        "containing-function assembly"
                    )
                row_start = address_value(function_start)
                row_end = max(exact_ends)
            matches = [
                index
                for index, row in enumerate(rows)
                if _cc_cfg._source_instruction_address(row) == source_address
            ]
            if len(matches) != 1:
                raise ValueError(
                    f"retail callback cell {cell_address} lacks one exact "
                    "xref instruction row"
                )
            return rows, matches[0], row_start, row_end, function_name

        def exact_dynamic_export_writer(
            source_rows: Sequence[Instruction],
            source_index: int,
            row_start: int,
            row_end: int,
            function_name: str,
            spec: Mapping[str, Any],
        ) -> None:
            """Validate one reviewed GetProcAddress-result callback store."""

            if (
                function_name != spec["writer_name"]
                or row_start != spec["writer_start"]
                or row_end != spec["writer_end"]
                or source_index < 1
            ):
                raise ValueError(
                    f"retail callback cell {cell_address} dynamic writer "
                    "function identity or extent drifted"
                )
            runtime_addresses = _cc_cfg._instruction_runtime_addresses(
                source_rows,
                source="bn",
                caller_start=row_start,
            )
            literal_address = address_value(str(spec["literal_address"]))
            iat_address = address_value(str(spec["getproc_iat"]))
            try:
                bodies = tuple(
                    bytes(int(item, 16) for item in row.bytes)
                    for row in source_rows
                )
            except (TypeError, ValueError) as exc:
                raise ValueError(
                    f"retail callback cell {cell_address} dynamic writer "
                    "bytes drifted"
                ) from exc
            call_rows = [
                index
                for index, (runtime_address, body) in enumerate(
                    zip(runtime_addresses, bodies)
                )
                if (
                    runtime_address == address_value(str(spec["getproc_call"]))
                    and body == b"\xff\x15" + struct.pack("<I", iat_address)
                    and _cc_cfg._instruction_mnemonic(source_rows[index]) == "call"
                )
            ]
            literal_rows = [
                index
                for index, body in enumerate(bodies)
                if (
                    index < (call_rows[0] if len(call_rows) == 1 else 0)
                    and body == b"\x68" + struct.pack("<I", literal_address)
                    and _cc_cfg._instruction_mnemonic(source_rows[index]) == "push"
                )
            ]
            module_rows = [
                index
                for index, body in enumerate(bodies)
                if (
                    index < (call_rows[0] if len(call_rows) == 1 else 0)
                    and body == b"\x50"
                    and _cc_cfg._instruction_mnemonic(source_rows[index]) == "push"
                    and _cc_cfg._instruction_operand(source_rows[index]).strip().lower()
                    == "eax"
                )
            ]
            if (
                len(call_rows) != 1
                or len(literal_rows) != 1
                or len(module_rows) != 1
            ):
                raise ValueError(
                    f"retail callback cell {cell_address} dynamic writer "
                    "requires one exact export push, EAX module push, and "
                    "GetProcAddress call"
                )
            call_index = call_rows[0]
            literal_index = literal_rows[0]
            module_index = module_rows[0]
            rendered_literal = _cc_cfg._instruction_operand(
                source_rows[literal_index]
            ).strip()
            try:
                rendered_literal_value = _cc_cfg._parse_unsigned_assembly_integer(
                    rendered_literal
                )
            except ValueError:
                rendered_literal_value = None
            bounded_rows = tuple(
                source_rows[literal_index : call_index + 1]
            )
            bounded_pushes = tuple(
                literal_index + index
                for index, row in enumerate(bounded_rows)
                if _cc_cfg._instruction_mnemonic(row) == "push"
            )
            drift = tuple(
                label
                for label, matches in (
                    (
                        "order",
                        literal_index < module_index < call_index < source_index,
                    ),
                    (
                        "call-store-adjacency",
                        source_index == call_index + 1
                        and runtime_addresses[source_index]
                        == runtime_addresses[call_index] + len(bodies[call_index]),
                    ),
                    (
                        "store-address-bytes",
                        runtime_addresses[source_index]
                        == address_value(str(spec["store"]))
                        and bodies[source_index]
                        == b"\xa3" + struct.pack("<I", value),
                    ),
                    (
                        "bounded-push-population",
                        bounded_pushes == (literal_index, module_index),
                    ),
                    (
                        "straight-line",
                        not any(
                            _cc_cfg._instruction_mnemonic(row).startswith("j")
                            or _cc_cfg._instruction_mnemonic(row).startswith("ret")
                            or (
                                _cc_cfg._instruction_mnemonic(row) == "call"
                                and literal_index + index != call_index
                            )
                            for index, row in enumerate(bounded_rows)
                        ),
                    ),
                    ("literal-rendering", bool(rendered_literal)),
                    (
                        "literal-value",
                        rendered_literal_value in {None, literal_address},
                    ),
                    (
                        "module-register",
                        _cc_cfg._instruction_operand(source_rows[module_index])
                        .strip()
                        .lower()
                        == "eax",
                    ),
                    (
                        "store-register",
                        _cc_cfg._instruction_operand(source_rows[source_index])
                        .split(",", 1)[-1]
                        .strip()
                        .lower()
                        == "eax",
                    ),
                )
                if not matches
            )
            if drift:
                raise ValueError(
                    f"retail callback cell {cell_address} dynamic writer "
                    "bytes, rendering, or straight-line topology drifted: "
                    + ", ".join(drift)
                )
            getproc_expression = _cc_targets._exact_memory_expression(
                _cc_cfg._instruction_operand(source_rows[call_index])
            )
            expected_iat_identity = str(spec["getproc_identity"])
            if (
                indexes.storage_by_address.get(str(spec["getproc_iat"]))
                != expected_iat_identity
                or (
                    getproc_expression != str(spec["getproc_iat"])
                    and indexes.storage_by_name.get(getproc_expression)
                    != expected_iat_identity
                )
            ):
                raise ValueError(
                    f"retail callback cell {cell_address} dynamic writer "
                    "GetProcAddress import identity drifted"
                )
            literal_bytes = bytes(spec["literal_bytes"])
            if _cc_cfg._hexdump_bytes(
                bridge.hexdump(str(spec["literal_address"]), len(literal_bytes))
            ) != literal_bytes:
                raise ValueError(
                    f"retail callback cell {cell_address} dynamic writer "
                    "export literal bytes drifted"
                )

        def exact_dynamic_export_reader(
            source_rows: Sequence[Instruction],
            row_start: int,
            row_end: int,
            spec: Mapping[str, Any],
        ) -> None:
            """Validate one targetless null-guarded callback call/cleanup4."""

            expected_bodies = (
                b"\xa1" + struct.pack("<I", value),
                b"\x85\xc0",
                b"\x74\x07",
                b"\x51",
                b"\xff\xd0",
                b"\x83\xc4\x04",
                b"\xc3",
                b"\x33\xc0",
                b"\xc3",
            )
            expected_addresses = tuple(
                range(int(spec["reader_start"]), int(spec["reader_end"]))
            )
            runtime_addresses = _cc_cfg._instruction_runtime_addresses(
                source_rows,
                source="bn",
                caller_start=row_start,
            )
            try:
                bodies = tuple(
                    bytes(int(item, 16) for item in row.bytes)
                    for row in source_rows
                )
            except (TypeError, ValueError) as exc:
                raise ValueError(
                    f"retail callback cell {cell_address} reader bytes drifted"
                ) from exc
            covered_addresses = tuple(
                address
                for runtime_address, body in zip(runtime_addresses, bodies)
                if runtime_address is not None
                for address in range(runtime_address, runtime_address + len(body))
            )
            def exact_row(index: int) -> bool:
                return (
                    index < len(bodies)
                    and bodies[index] == expected_bodies[index]
                )

            cleanup_operand = (
                _cc_cfg._instruction_operand(source_rows[5]).split(",", 1)
                if len(source_rows) > 5
                else []
            )
            cleanup_value: int | None = None
            if len(cleanup_operand) == 2:
                try:
                    cleanup_value = _cc_cfg._parse_unsigned_assembly_integer(
                        cleanup_operand[1].strip()
                    )
                except ValueError:
                    cleanup_value = None
            drift = tuple(
                label
                for label, matches in (
                    ("row-population", len(bodies) == len(expected_bodies)),
                    ("load-bytes", exact_row(0)),
                    ("test-bytes", exact_row(1)),
                    ("branch-bytes", exact_row(2)),
                    ("argument-push-bytes", exact_row(3)),
                    ("register-call-bytes", exact_row(4)),
                    ("cleanup4-bytes", exact_row(5)),
                    ("non-null-ret-bytes", exact_row(6)),
                    ("null-zero-bytes", exact_row(7)),
                    ("null-ret-bytes", exact_row(8)),
                    (
                        "extent",
                        row_start == spec["reader_start"]
                        and covered_addresses == expected_addresses,
                    ),
                    (
                        "null-target",
                        len(source_rows) > 2
                        and _cc_cfg._instruction_operand(source_rows[2]).strip().lower()
                        == normalize_address(int(spec["reader_start"]) + 0x10),
                    ),
                    (
                        "argument-register",
                        len(source_rows) > 3
                        and _cc_cfg._instruction_operand(source_rows[3]).strip().lower()
                        == "ecx",
                    ),
                    (
                        "call-register",
                        len(source_rows) > 4
                        and _cc_cfg._instruction_operand(source_rows[4]).strip().lower()
                        == "eax",
                    ),
                    (
                        "cleanup4-rendering",
                        len(cleanup_operand) == 2
                        and cleanup_operand[0].strip().lower() == "esp"
                        and cleanup_value == 4,
                    ),
                )
                if not matches
            )
            if drift:
                raise ValueError(
                    f"retail callback cell {cell_address} reader bytes, "
                    "extent, null CFG, register call, or caller cleanup4 "
                    "drifted: " + ", ".join(drift)
                )

        readers: list[str] = []
        immediate_targets: list[str] = []
        register_writers: list[tuple[str, str]] = []
        dynamic_export_writers: list[str] = []
        for source_address in source_addresses:
            (
                source_rows,
                source_index,
                row_start,
                row_end,
                function_name,
            ) = containing_rows(
                source_address
            )
            source_row = source_rows[source_index]
            try:
                source_body = bytes(
                    int(item, 16) for item in source_row.bytes
                )
            except (TypeError, ValueError):
                source_body = b""
            source_operands = _cc_cfg._instruction_operand(source_row).split(",", 1)
            source_accumulator = (
                len(source_body) == 5 and source_body[0] == 0xA1
            )
            source_absolute_load = (
                len(source_body) == 6
                and source_body[0] == 0x8B
                and source_body[1] >> 6 == 0
                and source_body[1] & 7 == 5
            )
            source_load_address = (
                normalize_address(
                    struct.unpack_from(
                        "<I",
                        source_body,
                        1 if source_accumulator else 2,
                    )[0]
                )
                if source_accumulator or source_absolute_load
                else ""
            )
            source_destination = (
                "eax"
                if source_accumulator
                else (
                    register_names[(source_body[1] >> 3) & 7]
                    if source_absolute_load
                    else ""
                )
            )
            if source_load_address == cell_address:
                if (
                    _cc_cfg._instruction_mnemonic(source_row) != "mov"
                    or len(source_operands) != 2
                    or source_operands[0].strip().lower()
                    != source_destination
                    or (
                        _cc_targets._exact_memory_expression(source_operands[1])
                        != cell_address
                        and indexes.storage_by_name.get(
                            _cc_targets._exact_memory_expression(source_operands[1])
                        )
                        != storage_identity
                    )
                ):
                    raise ValueError(
                        f"retail callback cell {cell_address} has malformed "
                        "load bytes or rendering"
                    )
                if source_address == normalized_load:
                    source_switch_targets = dict(
                        local_control_flow_targets or {}
                    )
                    source_switch_indices = local_control_flow_indices
                else:
                    source_switch_targets = _cc_cfg.retail_local_switch_targets(
                        source_rows,
                        caller_start=normalize_address(row_start),
                        caller_end_exclusive=normalize_address(row_end),
                        bridge=bridge,
                    )
                    source_switch_indices = frozenset(source_switch_targets)
                source_transfers = (
                    _cc_dispatch._retail_register_definition_transfer_addresses(
                        source_rows,
                        load_index=source_index,
                        destination=source_destination,
                        caller_start=row_start,
                        caller_end=row_end,
                        allow_exact_zero_guard=True,
                        bridge=bridge,
                        local_control_flow_indices=source_switch_indices,
                        local_control_flow_targets=source_switch_targets,
                        call_cleanup_by_instruction_index=(
                            call_cleanup_by_instruction_index
                            if source_address == normalized_load
                            else _cc_identity._retail_direct_call_cleanup_by_instruction_index(
                                source_rows,
                                caller_start=normalize_address(row_start),
                                caller_end_exclusive=normalize_address(row_end),
                                bridge=bridge,
                            )
                        ),
                        _trace_overflow=_trace_overflow,
                    )
                )
                if len(source_transfers) != 1:
                    raise ValueError(
                        f"retail callback cell {cell_address} reader does "
                        "not reach exactly one path-proved invocation"
                    )
                readers.append(source_address)
                continue

            exact_immediate_store = (
                len(source_body) == 10
                and source_body[:2] == b"\xc7\x05"
                and normalize_address(
                    struct.unpack_from("<I", source_body, 2)[0]
                )
                == cell_address
            )
            exact_register_store = (
                len(source_body) == 6
                and source_body[0] == 0x89
                and source_body[1] >> 6 == 0
                and source_body[1] & 7 == 5
                and normalize_address(
                    struct.unpack_from("<I", source_body, 2)[0]
                )
                == cell_address
            )
            exact_accumulator_store = (
                len(source_body) == 5
                and source_body[0] == 0xA3
                and normalize_address(struct.unpack_from("<I", source_body, 1)[0])
                == cell_address
            )
            rendered_cell = (
                _cc_targets._exact_memory_expression(source_operands[0])
                if len(source_operands) == 2
                else ""
            )
            if (
                _cc_cfg._instruction_mnemonic(source_row) != "mov"
                or len(source_operands) != 2
                or rendered_cell != cell_address
                or not (
                    exact_immediate_store
                    or exact_register_store
                    or exact_accumulator_store
                )
            ):
                if source_body[:2] == b"\xc7\x05":
                    raise ValueError(
                        f"retail callback cell {cell_address} lacks one exact "
                        "C705 target store"
                    )
                raise ValueError(
                    f"retail callback cell {cell_address} has an unclassified "
                    "reader/writer xref or malformed bytes/rendering"
                )
            if exact_accumulator_store:
                spec = reviewed_dynamic_export_cells.get(cell_address)
                if source_operands[1].strip().lower() != "eax":
                    raise ValueError(
                        f"retail callback cell {cell_address} A3 writer "
                        "rendering disagrees with exact accumulator bytes"
                    )
                if spec is not None:
                    exact_dynamic_export_writer(
                        source_rows,
                        source_index,
                        row_start,
                        row_end,
                        function_name,
                        spec,
                    )
                    dynamic_export_writers.append(source_address)
                register_writers.append((source_address, "eax"))
            elif exact_immediate_store:
                target_address = normalize_address(
                    struct.unpack_from("<I", source_body, 6)[0]
                )
                rendered_target = _cc_cfg._parse_unsigned_assembly_integer(
                    source_operands[1].strip()
                )
                target_identity = indexes.by_address.get(target_address, "")
                if (
                    rendered_target != address_value(target_address)
                    or not target_identity
                ):
                    raise ValueError(
                        f"retail callback cell {cell_address} lacks one exact "
                        "C705 target store"
                    )
                target_rows = tuple(
                    _cc_listing.parse_assembly(
                        bridge.assembly(target_address),
                        source="bn",
                    )
                )
                if (
                    not target_rows
                    or _cc_cfg._source_instruction_address(target_rows[0])
                    != target_address
                ):
                    raise ValueError(
                        f"retail callback cell {cell_address} target lacks "
                        "one exact body start"
                    )
                immediate_targets.append(target_identity)
            else:
                source_register = register_names[(source_body[1] >> 3) & 7]
                if source_operands[1].strip().lower() != source_register:
                    raise ValueError(
                        f"retail callback cell {cell_address} register-store "
                        "rendering disagrees with exact bytes"
                    )
                register_writers.append((source_address, source_register))

        if not readers:
            raise ValueError(
                f"retail callback cell {cell_address} lacks a complete "
                "validated reader population"
            )
        if dynamic_export_writers:
            spec = reviewed_dynamic_export_cells[cell_address]
            if (
                len(source_addresses) != 2
                or readers != [normalized_load]
                or dynamic_export_writers != [str(spec["store"])]
                or register_writers != [(str(spec["store"]), "eax")]
                or immediate_targets
            ):
                raise ValueError(
                    f"retail callback cell {cell_address} dynamic export "
                    "population is not exactly one writer and one reader"
                )
            exact_dynamic_export_reader(
                retail_instructions,
                start,
                end,
                spec,
            )
        if immediate_targets:
            if register_writers or len(immediate_targets) != 1:
                raise ValueError(
                    f"retail callback cell {cell_address} has ambiguous mixed "
                    "or repeated static/dynamic writers"
                )
            target_identity = immediate_targets[0]
            prior = reviewed.get(storage_identity)
            if prior not in {None, target_identity}:
                raise ValueError(
                    f"retail callback cell {cell_address} has conflicting stored targets"
                )
            reviewed[storage_identity] = target_identity
    unused_precomposed = precomposed.keys() - consumed_precomposed
    if unused_precomposed:
        raise ValueError(
            "stored-callback precomposed non-callback publication is unused: "
            + ", ".join(sorted(unused_precomposed, key=address_value))
        )
    return replace(
        indexes,
        reviewed_static_callback_target_by_storage=reviewed,
    )


def _exact_inbound_callback_address_materialization(
    caller_rows: Sequence[Instruction],
    *,
    source_address: str,
    target_address: str,
) -> bool:
    """Classify an inbound code xref that passes, rather than calls, a target.

    Binary Ninja reports immediate function addresses in executable
    instructions as code references.  An exact ``MOV r32, imm32`` followed by
    an exact push of that reaching definition, or an exact ``PUSH imm32``, may
    therefore be callback-registration provenance.  It is not an invocation
    of the callback.  Accept that classification only when the materialized
    argument reaches a subsequent exact CALL/JMP before a branch, return,
    truncation, or clobber.  Near-shapes remain fail-closed.
    """

    addresses = _cc_cfg._instruction_runtime_addresses(
        caller_rows,
        source="bn",
        caller_start=0,
    )
    source_value = address_value(source_address)
    target_value = address_value(target_address)
    source_indices = [
        index for index, address in enumerate(addresses)
        if address == source_value
    ]
    if len(source_indices) != 1:
        raise ValueError(
            "BN callback-address materialization has an ambiguous source instruction"
        )
    source_index = source_indices[0]
    source_instruction = caller_rows[source_index]
    try:
        source_body = bytes(
            int(item, 16) for item in source_instruction.bytes
        )
    except (TypeError, ValueError):
        source_body = b""
    register_names = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    token_registers: set[str] = set()
    argument_materialized = False
    operands = _cc_cfg._instruction_operand(source_instruction).split(",", 1)
    if len(source_body) == 10 and source_body[:2] == b"\xc7\x05":
        encoded_storage = normalize_address(
            struct.unpack_from("<I", source_body, 2)[0]
        )
        encoded_target = normalize_address(
            struct.unpack_from("<I", source_body, 6)[0]
        )
        destination_addresses = (
            _cc_catalog.ADDRESS_RE.findall(operands[0]) if len(operands) == 2 else []
        )
        source_addresses = (
            _cc_catalog.ADDRESS_RE.findall(operands[1]) if len(operands) == 2 else []
        )
        if (
            _cc_cfg._instruction_mnemonic(source_instruction) != "mov"
            or len(operands) != 2
            or len(destination_addresses) != 1
            or len(source_addresses) != 1
            or re.fullmatch(
                r"(?:dword\s+(?:ptr\s+)?)?\[0x[0-9a-f]+\]",
                operands[0].strip(),
                flags=re.IGNORECASE,
            ) is None
            or normalize_address(destination_addresses[0]) != encoded_storage
            or encoded_target != normalize_address(target_address)
            or normalize_address(source_addresses[0]) != encoded_target
        ):
            raise ValueError(
                "BN callback-address absolute-store materialization "
                "rendering/bytes drifted"
            )
        return True
    if len(source_body) == 5 and 0xB8 <= source_body[0] <= 0xBF:
        destination = register_names[source_body[0] - 0xB8]
        if (
            _cc_cfg._instruction_mnemonic(source_instruction) != "mov"
            or len(operands) != 2
            or operands[0].strip().lower() != destination
            or struct.unpack_from("<I", source_body, 1)[0] != target_value
        ):
            raise ValueError(
                "BN callback-address MOV materialization rendering/bytes drifted"
            )
        rendered = _cc_catalog.ADDRESS_RE.findall(operands[1])
        if rendered and normalize_address(rendered[-1]) != normalize_address(
            target_address
        ):
            raise ValueError(
                "BN callback-address MOV materialization target drifted"
            )
        token_registers.add(destination)
    elif len(source_body) == 5 and source_body[0] == 0x68:
        if (
            _cc_cfg._instruction_mnemonic(source_instruction) != "push"
            or struct.unpack_from("<I", source_body, 1)[0] != target_value
        ):
            raise ValueError(
                "BN callback-address PUSH materialization rendering/bytes drifted"
            )
        rendered = _cc_catalog.ADDRESS_RE.findall(_cc_cfg._instruction_operand(source_instruction))
        if rendered and normalize_address(rendered[-1]) != normalize_address(
            target_address
        ):
            raise ValueError(
                "BN callback-address PUSH materialization target drifted"
            )
        argument_materialized = True
    else:
        return False

    previous_end = source_value + len(source_body)
    for instruction, runtime_address in zip(
        caller_rows[source_index + 1 :],
        addresses[source_index + 1 :],
    ):
        if runtime_address is None or runtime_address != previous_end:
            raise ValueError(
                "BN callback-address materialization lineage is truncated"
            )
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            body = b""
        if not body:
            raise ValueError(
                "BN callback-address materialization lineage has missing bytes"
            )
        previous_end = runtime_address + len(body)
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        operand = re.sub(
            r"^(?:near|far)\s+(?:ptr\s+)?",
            "",
            _cc_cfg._instruction_operand(instruction).strip().lower(),
        )
        if mnemonic in {"call", "jmp"}:
            if not _cc_cfg._exact_invocation_encoding(instruction, mnemonic=mnemonic):
                raise ValueError(
                    "BN callback-address materialization reaches a malformed invocation"
                )
            if operand in token_registers:
                # This is an indirect invocation of the callback value, not a
                # registration/provider call that consumes its address.
                return False
            return argument_materialized or bool(
                token_registers & {"ecx", "edx"}
            )
        if mnemonic.startswith("j") or mnemonic.startswith("loop"):
            raise ValueError(
                "BN callback-address materialization has ambiguous branch lineage"
            )
        if _cc_cfg._exact_return_terminates(instruction):
            return False
        move = _cc_receiver_instructions._exact_register_move(instruction)
        if move is not None:
            destination, source_register = move
            if source_register in token_registers:
                token_registers.add(destination)
            else:
                token_registers.discard(destination)
            continue
        if (
            len(body) == 1
            and 0x50 <= body[0] <= 0x57
            and mnemonic == "push"
        ):
            source_register = register_names[body[0] - 0x50]
            if _cc_cfg._instruction_operand(instruction).strip().lower() != source_register:
                raise ValueError(
                    "BN callback-address materialization PUSH rendering drifted"
                )
            if source_register in token_registers:
                argument_materialized = True
            continue
        writes = _cc_instructions.written_registers(instruction)
        if writes & token_registers:
            token_registers.difference_update(writes)
            if not argument_materialized:
                return False
    raise ValueError("BN callback-address materialization lineage is truncated")


def _r4564_prove_candidate_callback_data_reference(
    candidate: CandidateAssembly,
    complete_offsets: Sequence[int | None],
    *,
    instruction_index: int,
    bare_name: str,
    operand_name: str,
    fallback_offset: int | None = None,
) -> int | None:
    """Validate one exact candidate callback-data COFF reference."""

    if operand_name == bare_name:
        return None
    caller = candidate.caller_definition
    if caller is None:
        raise ValueError(
            "r4564 decorated callback projection lacks COFF caller evidence"
        )
    symbols = [row for row in caller.coff_symbols if row.name == operand_name]
    if len(symbols) != 1:
        raise ValueError(
            "r4564 decorated callback projection requires one exact "
            "COFF data symbol"
        )
    symbol = symbols[0]
    instruction_offset = complete_offsets[instruction_index]
    if instruction_offset is None:
        instruction_offset = fallback_offset
    if instruction_offset is None:
        raise ValueError(
            "r4564 decorated callback projection lacks exact COFF "
            "instruction coordinates"
        )
    instruction = candidate.instructions[instruction_index]
    body = bytes(int(item, 16) for item in instruction.bytes)
    field_delta = len(body) - 4
    relocation_offset = int(instruction_offset) + field_delta
    references = [
        row
        for row in caller.relocations
        if row.offset == relocation_offset
        and row.symbol_name == operand_name
    ]
    undefined = (
        symbol.section_number == 0
        and caller.undefined_external_data.count(operand_name) == 1
        and operand_name not in caller.defined_external_data
    )
    same_tu_defined = (
        symbol.section_number > 0
        and caller.defined_external_data.count(operand_name) == 1
        and operand_name not in caller.undefined_external_data
        and symbol.section_name in {".data", ".bss"}
        and symbol.section_size >= 4
        and symbol.value >= 0
        and symbol.natural_end == symbol.value + 4
        and symbol.natural_end <= symbol.section_size
    )
    if (
        len(references) != 1
        or symbol.symbol_type != 0
        or symbol.storage_class != IMAGE_SYM_CLASS_EXTERNAL
        or symbol.aux_count != 0
        or not (undefined or same_tu_defined)
        or references[0].type != IMAGE_REL_I386_DIR32
        or references[0].symbol_index != symbol.index
        or field_delta not in {1, 2}
        or relocation_offset < 0
        or relocation_offset + 4 > len(caller.data)
        or caller.data[relocation_offset : relocation_offset + 4]
        != b"\x00\x00\x00\x00"
        or any(
            caller.relocation_mask[
                int(instruction_offset) : relocation_offset
            ]
        )
        or not all(
            caller.relocation_mask[
                relocation_offset : relocation_offset + 4
            ]
        )
    ):
        raise ValueError(
            "r4564 decorated callback projection rejects malformed "
            "same-TU/undefined COFF data-symbol or call-reaching "
            "relocation provenance"
        )
    return relocation_offset


def _r4564_prove_candidate_callback_function_reference(
    candidate: CandidateAssembly,
    complete_offsets: Sequence[int | None],
    *,
    instruction_index: int,
    operand_name: str,
    immediate_delta: int,
) -> int:
    """Validate one exact same-TU callback-function address reference."""

    caller = candidate.caller_definition
    if caller is None:
        raise ValueError(
            "r4564 dynamic callback-function source lacks COFF caller evidence"
        )
    symbols = [row for row in caller.coff_symbols if row.name == operand_name]
    instruction_offset = complete_offsets[instruction_index]
    if len(symbols) != 1 or instruction_offset is None:
        raise ValueError(
            "r4564 dynamic callback-function source requires one exact COFF symbol"
        )
    symbol = symbols[0]
    relocation_offset = int(instruction_offset) + immediate_delta
    references = [
        row
        for row in caller.relocations
        if row.offset == relocation_offset and row.symbol_name == operand_name
    ]
    same_tu_defined = (
        symbol.section_number > 0
        and caller.defined_external_functions.count(operand_name) == 1
        and operand_name not in caller.undefined_external_functions
        and symbol.section_name == ".text"
        and symbol.value >= 0
        and symbol.natural_end > symbol.value
        and symbol.natural_end <= symbol.section_size
    )
    if (
        len(references) != 1
        or not same_tu_defined
        or symbol.symbol_type != 0x20
        or symbol.storage_class != IMAGE_SYM_CLASS_EXTERNAL
        or symbol.aux_count != 0
        or references[0].type != IMAGE_REL_I386_DIR32
        or references[0].symbol_index != symbol.index
        or relocation_offset < 0
        or relocation_offset + 4 > len(caller.data)
        or caller.data[relocation_offset : relocation_offset + 4]
        != b"\x00\x00\x00\x00"
        or any(caller.relocation_mask[int(instruction_offset) : relocation_offset])
        or not all(caller.relocation_mask[relocation_offset : relocation_offset + 4])
    ):
        raise ValueError(
            "r4564 dynamic callback-function source rejects malformed "
            "same-TU COFF function/DIR32 provenance"
        )
    return relocation_offset


def _r4564_candidate_callback_storage_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    indexes: IdentityIndexes,
    reviewed_exact_indirect_storage_bridges_out: (
        dict[str, ReviewedExactIndirectStorageBridge] | None
    ) = None,
) -> dict[str, str]:
    """Join each exact typed callback invocation to its retail storage.

    This finite projection does not infer a target and does not admit arbitrary
    same-TU data.  The exact authored caller, reviewed name allowlist, candidate
    call/load provenance, and same-ordinal expected callback storage must all
    agree.  Loads and stores which do not reach an indirect call are outside
    this call-contract adapter and remain ordinary candidate data evidence.
    """
    from _recoil.call_contract.records import ReviewedExactIndirectStorageBridge

    start = normalize_address(caller_start)
    allowed = _cc_catalog._R4564_CANDIDATE_CALLBACK_STORAGE_NAMES.get(start)
    if allowed is None:
        return {}
    if (
        indexes.by_address.get(start) != caller_identity
        or caller_identity in indexes.provider_ids
    ):
        raise ValueError(
            "r4564 candidate callback projection requires exact authored caller"
        )
    caller = candidate.caller_definition
    complete_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start="0x0",
        caller_end_exclusive=hex(
            max(1, len(caller.data) if caller is not None else len(candidate.instructions))
        ),
    )

    def callback_names(operand: str) -> tuple[str, ...]:
        return tuple(
            name
            for name in allowed
            if re.search(rf"\b{re.escape(name)}\b", operand)
        )

    # (ordinal, call-index, call-offset, register-or-empty, load-index,
    #  bare-name, exact COFF expression)
    callback_calls: list[tuple[int, int, int, str, int, str, str]] = []
    for ordinal, call_index in enumerate(invocation_indices):
        call = candidate.instructions[call_index]
        call_operand = _cc_cfg._instruction_operand(call).strip()
        direct_names = callback_names(call_operand)
        if len(direct_names) > 1:
            raise ValueError(
                "r4564 candidate callback projection has an ambiguous direct "
                "callback name at one invocation"
            )
        call_offset = complete_offsets[call_index]
        if call_offset is None and direct_names and caller is not None:
            direct_expression = _cc_targets._exact_memory_expression(call_operand)
            direct_references = [
                row
                for row in caller.relocations
                if row.symbol_name == direct_expression
                and row.type == IMAGE_REL_I386_DIR32
                and row.offset >= 2
            ]
            if len(direct_references) == 1:
                call_offset = direct_references[0].offset - 2
        if call_offset is None:
            raise ValueError(
                "r4564 candidate callback projection lacks exact invocation coordinates"
            )
        if direct_names:
            expression = _cc_targets._exact_memory_expression(call_operand)
            callback_calls.append((
                ordinal, call_index, int(call_offset), "", call_index,
                direct_names[0], expression if expression.startswith("?") else direct_names[0],
            ))
            continue
        register_match = re.fullmatch(
            r"(?:near\s+|far\s+)?(?:dword\s+(?:ptr\s+)?)?"
            r"(?P<register>e(?:ax|bx|cx|dx|si|di|bp))",
            call_operand,
            flags=re.IGNORECASE,
        )
        if register_match is None:
            continue
        register = register_match.group("register").lower()
        reaching: tuple[int, str, str] | None = None
        for load_index in range(call_index - 1, -1, -1):
            load = candidate.instructions[load_index]
            load_operand = _cc_cfg._instruction_operand(load)
            names = callback_names(load_operand)
            if (
                names
                and _cc_cfg._instruction_mnemonic(load) == "mov"
                and _cc_instructions.may_clobber_register(load, register)
            ):
                if len(names) != 1:
                    raise ValueError(
                        "r4564 candidate callback projection has an ambiguous "
                        "register-load callback name"
                    )
                expression = _cc_targets._exact_memory_expression(load_operand)
                reaching = (
                    load_index,
                    names[0],
                    expression if expression.startswith("?") else names[0],
                )
                break
            if _cc_cfg._instruction_may_clobber_register(load, register):
                break
        if reaching is not None:
            load_index, bare_name, expression = reaching
            callback_calls.append((
                ordinal, call_index, int(call_offset), register, load_index,
                bare_name, expression,
            ))

    if not callback_calls:
        raise ValueError(
            "r4564 candidate callback projection lacks an exact reviewed COD name"
        )

    resolved: dict[str, str] = {}
    exact_out = reviewed_exact_indirect_storage_bridges_out
    if exact_out is not None and exact_out:
        raise ValueError(
            "r4564 candidate callback projection requires an empty exact-call sink"
        )
    proven_expressions: set[tuple[str, int]] = set()
    for (
        ordinal, _call_index, call_offset, register, load_index,
        bare_name, operand_name,
    ) in callback_calls:
        if ordinal >= len(expected):
            raise ValueError(
                "r4564 candidate callback projection lacks a same-ordinal retail row"
            )
        retail = expected[ordinal]
        identity = str(retail.get("storage_identity", ""))
        if (
            retail.get("ordinal") != ordinal
            or retail.get("form") not in {"call", "tail"}
            or retail.get("dispatch") != "indirect"
            or retail.get("identity_kind") != "callback"
            or retail.get("target_identity") != ""
            or not identity
            or identity.startswith("iat:")
        ):
            raise ValueError(
                "r4564 candidate callback projection rejects same-ordinal "
                "targetless retail callback storage"
            )
        prior = resolved.get(operand_name)
        if not register and prior not in {None, identity}:
            raise ValueError(
                "r4564 direct callback storage has conflicting per-call identities"
            )

        relocation_offset = _r4564_prove_candidate_callback_data_reference(
            candidate,
            complete_offsets,
            instruction_index=load_index,
            bare_name=bare_name,
            operand_name=operand_name,
            fallback_offset=(
                call_offset if load_index == _call_index else None
            ),
        )
        if relocation_offset is not None:
            proven_expressions.add((operand_name, relocation_offset))

        if register:
            if exact_out is None:
                resolved[operand_name] = identity
                resolved[bare_name] = identity
            else:
                key = hex(call_offset)
                if key in exact_out:
                    raise ValueError(
                        "r4564 candidate callback projection collides at one call site"
                    )
                exact_out[key] = ReviewedExactIndirectStorageBridge(
                    register=register,
                    storage_identity=identity,
                    slot_displacement=0,
                    assembly_source="cod",
                    identity_kind="callback",
                )
        else:
            resolved[operand_name] = identity
            resolved[bare_name] = identity

    selection_spec = _cc_catalog._R4564_CANDIDATE_DYNAMIC_SELECTION_SOURCES.get(start)
    selection_source_present = (
        selection_spec is not None
        and any(
            re.search(rf"\b{re.escape(name)}\b", _cc_cfg._instruction_operand(instruction))
            for instruction in candidate.instructions
            for name in selection_spec[2]
        )
    )
    if selection_spec is not None and selection_source_present:
        ordinals, identity, source_names, function_sources = selection_spec
        if any(ordinal >= len(expected) for ordinal in ordinals):
            raise ValueError(
                "r4564 candidate dynamic selection lacks its retail ordinals"
            )
        for ordinal in ordinals:
            retail = expected[ordinal]
            if (
                retail.get("ordinal") != ordinal
                or retail.get("form") not in {"call", "tail"}
                or retail.get("dispatch") != "indirect"
                or retail.get("identity_kind") != "callback"
                or retail.get("target_identity") != ""
                or retail.get("storage_identity") != identity
            ):
                raise ValueError(
                    "r4564 candidate dynamic selection retail identity drifted"
                )
        operands_by_name: dict[str, set[str]] = {
            name: set() for name in source_names
        }
        source_loads_by_name: dict[str, list[tuple[int, str]]] = {
            name: [] for name in source_names
        }
        for instruction_index, instruction in enumerate(candidate.instructions):
            if _cc_cfg._instruction_mnemonic(instruction) != "mov":
                continue
            operand = _cc_cfg._instruction_operand(instruction)
            matches = tuple(
                name
                for name in source_names
                if re.search(rf"\b{re.escape(name)}\b", operand)
            )
            if not matches:
                continue
            if len(matches) != 1:
                raise ValueError(
                    "r4564 candidate dynamic selection has an ambiguous source name"
                )
            bare_name = matches[0]
            operand_names = tuple(
                symbol.name
                for symbol in (caller.coff_symbols if caller is not None else ())
                if symbol.name.startswith("?")
                and re.search(rf"\b{re.escape(bare_name)}\b", symbol.name)
                and symbol.name in operand
            )
            if len(operand_names) != 1:
                raise ValueError(
                    "r4564 candidate dynamic selection lacks a decorated COFF source"
                )
            operand_name = operand_names[0]
            try:
                body = bytes(int(item, 16) for item in instruction.bytes)
            except (TypeError, ValueError):
                body = b""
            operands = _cc_cfg._instruction_operand(instruction).split(",", 1)
            register_names = (
                "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"
            )
            destination = (
                "eax"
                if len(body) == 5 and body[0] == 0xA1
                else register_names[(body[1] >> 3) & 0x07]
                if (
                    len(body) == 6
                    and body[0] == 0x8B
                    and body[1] >> 6 == 0
                    and body[1] & 0x07 == 0x05
                )
                else ""
            )
            if (
                len(operands) != 2
                or operands[0].strip().lower() != destination
                or _cc_targets._exact_memory_expression(operands[1]) != operand_name
            ):
                raise ValueError(
                    "r4564 candidate dynamic selection requires exact "
                    "absolute callback-source loads"
                )
            relocation_offset = (
                _r4564_prove_candidate_callback_data_reference(
                    candidate,
                    complete_offsets,
                    instruction_index=instruction_index,
                    bare_name=bare_name,
                    operand_name=operand_name,
                )
            )
            if relocation_offset is None:
                raise ValueError(
                    "r4564 candidate dynamic selection lacks relocation provenance"
                )
            proof = (operand_name, relocation_offset)
            if proof in proven_expressions:
                raise ValueError(
                    "r4564 candidate dynamic selection reuses a callback proof"
                )
            proven_expressions.add(proof)
            operands_by_name[bare_name].add(operand_name)
            source_loads_by_name[bare_name].append(
                (instruction_index, destination)
            )
        source_counts = _cc_catalog._R4564_CANDIDATE_DYNAMIC_SELECTION_SOURCE_COUNTS.get(
            start,
            {},
        )
        if (
            set(source_counts) - set(source_names)
            or any(
                not isinstance(count, int) or count < 1
                for count in source_counts.values()
            )
            or any(len(operands) != 1 for operands in operands_by_name.values())
            or any(
                len(loads) != source_counts.get(name, 1)
                for name, loads in source_loads_by_name.items()
            )
        ):
            raise ValueError(
                "r4564 candidate dynamic selection source population drifted"
            )
        for bare_name, operands in operands_by_name.items():
            operand_name = next(iter(operands))
            resolved[bare_name] = identity
            resolved[operand_name] = identity

        direct_function_store_indices: set[int] = set()
        for bare_name, address in function_sources:
            normalized_address = normalize_address(address)
            authored_identity = indexes.by_address.get(normalized_address)
            if authored_identity is None or authored_identity in indexes.provider_ids:
                raise ValueError(
                    "r4564 candidate dynamic selection callback-function source "
                    "is not an exact authored identity"
                )
            matches: list[tuple[int, str, int]] = []
            for instruction_index, instruction in enumerate(candidate.instructions):
                if (
                    _cc_cfg._instruction_mnemonic(instruction) != "mov"
                    or not re.search(
                        rf"\b{re.escape(bare_name)}\b",
                        _cc_cfg._instruction_operand(instruction),
                    )
                ):
                    continue
                decoded = _cc_identity._exact_vc5_symbolic_stack_function_store(instruction)
                if decoded is None:
                    raise ValueError(
                        "r4564 candidate dynamic selection requires an exact "
                        "symbolic-local callback-function store"
                    )
                _encoded, _symbol, _rendered, operand_name, immediate_delta = decoded
                if (
                    caller is None
                    or operand_name not in {
                        symbol.name
                        for symbol in caller.coff_symbols
                        if symbol.symbol_type == 0x20
                        and re.search(rf"\b{re.escape(bare_name)}\b", symbol.name)
                    }
                ):
                    raise ValueError(
                        "r4564 candidate dynamic selection lacks its exact "
                        "decorated callback-function symbol"
                    )
                matches.append((instruction_index, operand_name, immediate_delta))
            if len(matches) != 1:
                raise ValueError(
                    "r4564 candidate dynamic selection callback-function source "
                    "population drifted"
                )
            instruction_index, operand_name, immediate_delta = matches[0]
            relocation_offset = _r4564_prove_candidate_callback_function_reference(
                candidate,
                complete_offsets,
                instruction_index=instruction_index,
                operand_name=operand_name,
                immediate_delta=immediate_delta,
            )
            proof = (operand_name, relocation_offset)
            if proof in proven_expressions:
                raise ValueError(
                    "r4564 candidate dynamic selection reuses a callback proof"
                )
            proven_expressions.add(proof)
            direct_function_store_indices.add(instruction_index)
            resolved[bare_name] = identity
            resolved[operand_name] = identity

        if exact_out is not None:
            source_load_registers = {
                instruction_index: register
                for loads in source_loads_by_name.values()
                for instruction_index, register in loads
            }
            proven_calls = _cc_callable_identity._r4564_prove_candidate_dynamic_selection_calls(
                candidate,
                complete_offsets,
                invocation_indices,
                ordinals=ordinals,
                source_load_registers=source_load_registers,
                direct_function_store_indices=frozenset(
                    direct_function_store_indices
                ),
            )
            for call_offset, register, slot_displacement in proven_calls:
                key = hex(call_offset)
                if key in exact_out:
                    raise ValueError(
                        "r4564 candidate dynamic selection collides at one call site"
                    )
                exact_out[key] = ReviewedExactIndirectStorageBridge(
                    register=register,
                    storage_identity=identity,
                    slot_displacement=slot_displacement,
                    assembly_source="cod",
                    identity_kind="callback",
                )
    return resolved
