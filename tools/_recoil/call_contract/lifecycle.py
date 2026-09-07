"""Recoil call-contract lifecycle evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import instructions as _cc_instructions
from _recoil.call_contract import listing as _cc_listing
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        IdentityIndexes,
        ReviewedLoopVptrStorageBridge,
    )

import re
import struct
from dataclasses import replace
from pathlib import Path
from typing import Sequence

from _recoil.commands.asm_verify import Instruction
from _recoil.lib.binja import BinaryNinjaBridge, BridgeError
from _recoil.lib.pe import parse_pe_headers, rva_to_offset
from _recoil.lib.progress import address_value, normalize_address
from _recoil.lib.windows_identity import StableReadHandle


def _retail_constructor_absolute_table_storage_indexes(
    retail_instructions: Sequence[Instruction],
    *,
    indexes: IdentityIndexes,
    bridge: BinaryNinjaBridge,
    reference: Path = _cc_catalog.DEFAULT_REFERENCE,
) -> IdentityIndexes:
    """Compose constructor-installed immutable table fields as storage.

    This covers absolute ``FF 15 [cell]`` dispatch through a field of one
    exact BN data container when the same caller installs that container at
    receiver offset zero.  The pointed function is used only as a collision
    guard; publication stays targetless because a table cell is callback
    storage, not accepted static callee identity.
    """
    from _recoil.call_contract.records import StorageContainer

    addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=0,
    )
    unresolved_calls: list[tuple[int, str, str]] = []
    for index, (instruction, runtime_address) in enumerate(
        zip(retail_instructions, addresses)
    ):
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            continue
        if (
            runtime_address is None
            or _cc_cfg._instruction_mnemonic(instruction) != "call"
            or len(body) != 6
            or body[:2] != b"\xff\x15"
        ):
            continue
        cell_address = normalize_address(struct.unpack_from("<I", body, 2)[0])
        value = address_value(cell_address)
        if cell_address in indexes.storage_by_address or any(
            row.start <= value < row.end_exclusive
            for row in indexes.storage_containers
        ):
            continue
        unresolved_calls.append(
            (index, normalize_address(runtime_address), cell_address)
        )
    if not unresolved_calls:
        return indexes

    try:
        with StableReadHandle(reference) as stable_reference:
            reference_data = stable_reference.read()
        headers = parse_pe_headers(reference_data, source=str(reference))
    except (OSError, ValueError) as exc:
        raise ValueError(
            "constructor-table storage composition cannot read the immutable "
            "retail PE"
        ) from exc
    data_rows = tuple(bridge.data_variables())
    additions: list[StorageContainer] = []
    for call_index, call_address, cell_address in unresolved_calls:
        cell_value = address_value(cell_address)
        containing = [
            row
            for row in data_rows
            if int(getattr(row, "size", 0)) > 0
            and address_value(str(getattr(row, "address", "")))
            <= cell_value
            < address_value(str(getattr(row, "address", "")))
            + int(getattr(row, "size", 0))
        ]
        if len(containing) != 1:
            continue
        data_row = containing[0]
        table_address = normalize_address(str(data_row.address))
        table_start = address_value(table_address)
        table_size = int(data_row.size)
        table_end = table_start + table_size
        table_name = str(getattr(data_row, "name", ""))
        table_raw_name = str(getattr(data_row, "raw_name", ""))
        if (
            not table_name
            or table_size < 4
            or cell_value + 4 > table_end
        ):
            continue
        table_rva = table_start - headers.image_base
        table_end_rva = table_end - headers.image_base
        rdata_sections = [
            section
            for section in headers.sections
            if section.name == ".rdata"
            and section.virtual_address <= table_rva
            and table_end_rva
            <= section.virtual_address + section.raw_size
        ]
        table_offset = rva_to_offset(table_rva, headers.sections)
        if len(rdata_sections) != 1 or table_offset is None:
            continue
        immutable = reference_data[table_offset : table_offset + table_size]
        bridged = _cc_cfg._hexdump_bytes(
            bridge.hexdump(table_address, table_size)
        )
        cell_offset = cell_value - table_start
        if (
            len(immutable) != table_size
            or bridged != immutable
            or len(bridged) != table_size
        ):
            raise ValueError(
                f"constructor table {table_address} immutable extent changed"
            )
        target_address = normalize_address(
            struct.unpack_from("<I", immutable, cell_offset)[0]
        )
        target_identity = indexes.by_address.get(target_address, "")
        if (
            not target_identity
            or [
                address
                for address, identity in indexes.by_address.items()
                if identity == target_identity
            ]
            != [target_address]
        ):
            continue

        get_json = getattr(bridge, "get_json", None)
        if not callable(get_json):
            continue
        payload = get_json("getXrefsTo", address=cell_address, limit=100000)
        code_rows, data_xrefs, complete = _cc_identity._bn_inbound_xref_items(payload)
        xref_addresses = [
            _cc_identity._bn_xref_address(
                row,
                "source_address",
                "source_addr",
                "from_address",
                "address",
            )
            for row in code_rows
        ]
        if (
            not complete
            or data_xrefs
            or xref_addresses != [call_address]
        ):
            continue

        aliases = {"ecx"}
        stores: list[int] = []
        for index, instruction in enumerate(retail_instructions[:call_index]):
            mnemonic = _cc_cfg._instruction_mnemonic(instruction)
            if mnemonic.startswith("j") or mnemonic in {
                "call", "loop", "loope", "loopne", "jecxz",
            }:
                aliases.clear()
                continue
            move = _cc_receiver_instructions._exact_register_move(instruction)
            if move is not None:
                destination, source_register = move
                if source_register in aliases:
                    aliases.add(destination)
                else:
                    aliases.discard(destination)
                continue
            try:
                body = bytes(int(value, 16) for value in instruction.bytes)
            except (TypeError, ValueError):
                body = b""
            operands = _cc_cfg._instruction_operand(instruction).split(",", 1)
            if (
                mnemonic == "mov"
                and len(body) >= 6
                and body[0] == 0xC7
                and ((body[1] >> 3) & 7) == 0
                and body[1] >> 6 != 3
                and len(operands) == 2
                and struct.unpack_from("<I", body, len(body) - 4)[0]
                == table_start
            ):
                expression, displacement = _cc_targets._memory_slot(operands[0])
                if expression in aliases and displacement in {None, 0}:
                    stores.append(index)
                continue
            aliases.difference_update(_cc_instructions.written_registers(instruction))
        if len(stores) != 1:
            continue

        identity_candidates = {
            identity
            for identity in (
                indexes.storage_by_address.get(table_address, ""),
                indexes.storage_by_name.get(table_name, ""),
                indexes.storage_by_name.get(table_raw_name, ""),
            )
            if identity
        }
        if len(identity_candidates) > 1:
            raise ValueError(
                f"constructor table {table_address} has conflicting storage "
                "identities"
            )
        storage_identity = (
            next(iter(identity_candidates))
            if identity_candidates
            else f"constructor-table:{table_address}"
        )
        if storage_identity.startswith("iat:"):
            raise ValueError(
                f"constructor table {table_address} collides with an IAT"
            )
        candidate = StorageContainer(
            start=table_start,
            end_exclusive=table_end,
            identity=storage_identity,
        )
        overlaps = [
            row
            for row in (*indexes.storage_containers, *additions)
            if row.start < candidate.end_exclusive
            and candidate.start < row.end_exclusive
        ]
        if overlaps and any(row != candidate for row in overlaps):
            raise ValueError(
                f"constructor table {table_address} overlaps a different "
                "reviewed storage container"
            )
        additions.append(candidate)
    if not additions:
        return indexes
    return replace(
        indexes,
        storage_containers=tuple(
            sorted(
                {*indexes.storage_containers, *additions},
                key=lambda row: (
                    row.start,
                    row.end_exclusive,
                    row.identity,
                ),
            )
        ),
    )


def _retail_constructor_table_dispatch_bridges(
    retail_instructions: Sequence[Instruction],
    *,
    indexes: IdentityIndexes,
    bridge: BinaryNinjaBridge,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Prove constructor-written local-table dispatches from immutable retail.

    A tracker selector is only a candidate relationship.  Publication requires
    the raw table cell to hold the reviewed physical target, one exact raw
    constructor store of the table address, the constructor's governed
    identity/name, a direct E8 call to that constructor in this caller, and an
    exact subsequent receiver-vptr slot transfer.  Any competing chain is left
    unresolved.
    """
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge

    selector_rows: list[tuple[str, int, str, str, str]] = []
    call_slots: set[int] = set()
    instruction_addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=0,
    )
    for instruction in retail_instructions:
        if _cc_cfg._instruction_mnemonic(instruction) not in {"call", "jmp"}:
            continue
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            continue
        if len(body) >= 2 and body[0] == 0xFF and ((body[1] >> 3) & 7) in {2, 4}:
            _expression, displacement = _cc_targets._memory_slot(
                _cc_cfg._instruction_operand(instruction)
            )
            if displacement is not None:
                call_slots.add(displacement)
    if not call_slots:
        return {}

    direct_constructor_targets: set[str] = set()
    for instruction, runtime_address in zip(
        retail_instructions,
        instruction_addresses,
    ):
        if runtime_address is None or _cc_cfg._instruction_mnemonic(instruction) != "call":
            continue
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            continue
        if len(body) == 5 and body[0] == 0xE8:
            direct_constructor_targets.add(
                normalize_address(
                    runtime_address + 5 + struct.unpack_from("<i", body, 1)[0]
                )
            )
    if not direct_constructor_targets:
        return {}

    # Discover exact constructor/factory chains from the current caller.  The
    # older reviewed-selector route below remains authoritative when present,
    # but live retail also contains friendly-named constructors and one-hop
    # row factories that have no synthetic ``constructor-table:*`` selector.
    # A publication therefore still requires an exact C7 table write, one
    # immutable table cell, and one governed physical target identity.
    assembly_cache: dict[str, tuple[Instruction, ...]] = {}

    def assembly_rows(function_address: str) -> tuple[Instruction, ...]:
        rows = assembly_cache.get(function_address)
        if rows is None:
            rows = tuple(
                _cc_listing.parse_assembly(
                    bridge.assembly(function_address),
                    source="bn",
                )
            )
            assembly_cache[function_address] = rows
        return rows

    def constructor_receiver_table_stores(
        rows: Sequence[Instruction],
    ) -> tuple[str, ...]:
        """Decode exact offset-zero C7 stores through entry-ECX aliases."""

        aliases = {"ecx"}
        tables: list[str] = []
        registers = (
            "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
        )
        for instruction in rows:
            mnemonic = _cc_cfg._instruction_mnemonic(instruction)
            if mnemonic.startswith("j") or mnemonic in {
                "call", "loop", "loope", "loopne", "jecxz",
            }:
                aliases.clear()
                continue
            move = _cc_receiver_instructions._exact_register_move(instruction)
            if move is not None:
                destination, source_register = move
                if source_register in aliases:
                    aliases.add(destination)
                else:
                    aliases.discard(destination)
                continue
            try:
                body = bytes(int(item, 16) for item in instruction.bytes)
            except (TypeError, ValueError):
                body = b""
            operands = _cc_cfg._instruction_operand(instruction).split(",", 1)
            if (
                mnemonic == "mov"
                and len(body) == 6
                and body[0] == 0xC7
                and body[1] >> 6 == 0
                and ((body[1] >> 3) & 7) == 0
                and (body[1] & 7) not in {4, 5}
                and len(operands) == 2
            ):
                base = registers[body[1] & 7]
                rendered_base = _cc_targets._exact_memory_expression(
                    operands[0]
                ).lower()
                try:
                    rendered_immediate = _cc_cfg._parse_unsigned_assembly_integer(
                        operands[1].strip()
                    )
                except ValueError:
                    rendered_immediate = -1
                encoded_immediate = struct.unpack_from("<I", body, 2)[0]
                if (
                    rendered_base == base
                    and rendered_immediate == encoded_immediate
                    and base in aliases
                ):
                    tables.append(normalize_address(encoded_immediate))
                continue
            aliases.difference_update(_cc_instructions.written_registers(instruction))
        return tuple(tables)

    reachable_constructor_targets = set(direct_constructor_targets)
    for target in tuple(direct_constructor_targets):
        try:
            rows = assembly_rows(target)
        except (BridgeError, OSError, RuntimeError, ValueError):
            continue
        target_addresses = _cc_cfg._instruction_runtime_addresses(
            rows,
            source="bn",
            caller_start=address_value(target),
        )
        for instruction, runtime_address in zip(rows, target_addresses):
            if runtime_address is None or _cc_cfg._instruction_mnemonic(instruction) != "call":
                continue
            try:
                body = bytes(int(item, 16) for item in instruction.bytes)
            except (TypeError, ValueError):
                continue
            if len(body) == 5 and body[0] == 0xE8:
                reachable_constructor_targets.add(
                    normalize_address(
                        runtime_address
                        + 5
                        + struct.unpack_from("<i", body, 1)[0]
                    )
                )

    discovered_rows: set[tuple[str, int, str, str, str]] = set()
    for constructor_target in sorted(
        reachable_constructor_targets,
        key=address_value,
    ):
        constructor_identity = indexes.by_address.get(constructor_target, "")
        if not constructor_identity:
            continue
        constructor_names = {
            name
            for name, identity in indexes.by_candidate_name.items()
            if identity == constructor_identity
            and _cc_catalog.MSVC_CONSTRUCTOR_RE.fullmatch(name)
        }
        if len(constructor_names) != 1:
            continue
        try:
            rows = assembly_rows(constructor_target)
        except (BridgeError, OSError, RuntimeError, ValueError):
            continue
        for table_address in constructor_receiver_table_stores(rows):
            for slot in call_slots:
                try:
                    cell = _cc_cfg._hexdump_bytes(
                        bridge.hexdump(
                            normalize_address(
                                address_value(table_address) + slot
                            ),
                            4,
                        )
                    )
                except (BridgeError, OSError, RuntimeError, ValueError):
                    continue
                if len(cell) != 4:
                    continue
                physical_target = normalize_address(
                    struct.unpack("<I", cell)[0]
                )
                logical_identity = indexes.by_address.get(
                    physical_target,
                    "",
                )
                if not logical_identity:
                    continue
                canonical_storage = indexes.storage_by_address.get(
                    table_address,
                    f"constructor-table:{table_address}",
                )
                reviewed_claims = {
                    identity
                    for storage in {
                        canonical_storage,
                        f"constructor-table:{table_address}",
                    }
                    for identity in [
                        indexes.reviewed_authored_icf_by_vtable_selector.get(
                            (storage, slot),
                            "",
                        )
                    ]
                    if identity
                }
                if reviewed_claims and (
                    len(reviewed_claims) != 1
                    or {
                        indexes.reviewed_authored_icf_physical_by_logical_identity.get(
                            identity,
                            "",
                        )
                        for identity in reviewed_claims
                    }
                    != {physical_target}
                ):
                    continue
                if reviewed_claims:
                    logical_identity = next(iter(reviewed_claims))
                discovered_rows.add(
                    (
                        canonical_storage,
                        slot,
                        logical_identity,
                        table_address,
                        constructor_target,
                    )
                )

    selector_rows.extend(sorted(discovered_rows))

    for (storage_identity, slot), logical_identity in (
        indexes.reviewed_authored_icf_by_vtable_selector.items()
    ):
        match = re.fullmatch(r"constructor-table:(0x[0-9a-fA-F]+)", storage_identity)
        if match is None or slot not in call_slots:
            continue
        table_address = normalize_address(match.group(1))
        physical_target = (
            indexes.reviewed_authored_icf_physical_by_logical_identity.get(
                logical_identity,
                "",
            )
        )
        if not physical_target:
            physical_matches = [
                address
                for address, identity in indexes.by_address.items()
                if identity == logical_identity
            ]
            if len(physical_matches) == 1:
                physical_target = physical_matches[0]
        if not physical_target:
            continue
        cell = _cc_cfg._hexdump_bytes(
            bridge.hexdump(
                normalize_address(address_value(table_address) + slot),
                4,
            )
        )
        if (
            len(cell) != 4
            or normalize_address(struct.unpack("<I", cell)[0])
            != normalize_address(physical_target)
        ):
            continue

        get_json = getattr(bridge, "get_json", None)
        if not callable(get_json):
            continue
        payload = get_json(
            "getXrefsTo",
            address=table_address,
            limit=100000,
        )
        code_rows, _data_rows, complete = _cc_identity._bn_inbound_xref_items(payload)
        if not complete:
            raise ValueError(
                f"constructor table {table_address} has incomplete inbound xrefs"
            )
        stores: list[tuple[str, str]] = []
        assembly_cache: dict[str, tuple[Instruction, ...]] = {}
        for row in code_rows:
            source_address = _cc_identity._bn_xref_address(
                row,
                "source_address",
                "source_addr",
                "from_address",
                "address",
            )
            if not source_address:
                raise ValueError(
                    f"constructor table {table_address} has an ambiguous xref"
                )
            _name, function_start = _cc_identity._bn_unique_containing_function(
                bridge,
                instruction_address=source_address,
            )
            rows = assembly_cache.get(function_start)
            if rows is None:
                rows = tuple(
                    _cc_listing.parse_assembly(bridge.assembly(function_start), source="bn")
                )
                assembly_cache[function_start] = rows
            matching = [
                instruction
                for instruction in rows
                if _cc_cfg._source_instruction_address(instruction) == source_address
            ]
            if len(matching) != 1:
                continue
            instruction = matching[0]
            try:
                body = bytes(int(item, 16) for item in instruction.bytes)
            except (TypeError, ValueError):
                continue
            if (
                len(body) < 6
                or body[0] != 0xC7
                or ((body[1] >> 3) & 7) != 0
                or body[1] >> 6 == 3
                or struct.unpack_from("<I", body, len(body) - 4)[0]
                != address_value(table_address)
                or _cc_cfg._instruction_mnemonic(instruction) != "mov"
            ):
                continue
            constructor_identity = indexes.by_address.get(function_start, "")
            constructor_names = {
                name
                for name, identity in indexes.by_candidate_name.items()
                if identity == constructor_identity
                and _cc_catalog.MSVC_CONSTRUCTOR_RE.fullmatch(name)
            }
            if len(constructor_names) != 1:
                continue
            stores.append((source_address, function_start))
        if len(stores) == 1 and stores[0][1] in direct_constructor_targets:
            selector_rows.append(
                (
                    storage_identity,
                    slot,
                    logical_identity,
                    table_address,
                    stores[0][1],
                )
            )

    # Publication is keyed by the constructor that produced the receiver, not
    # merely by a slot number observed elsewhere in the caller.  This prevents
    # an unrelated self-vptr store or another same-slot table from lending a
    # static target to a runtime-selected nested member object.
    selector_rows = sorted(set(selector_rows))
    selector_by_constructor_slot: dict[
        tuple[str, int], list[tuple[str, int, str, str, str]]
    ] = {}
    for row in selector_rows:
        selector_by_constructor_slot.setdefault((row[4], row[1]), []).append(row)
    constructor_targets = {row[4] for row in selector_rows}

    # Values are (kind, exact constructor address).  The state is deliberately
    # straight-line and is cleared at control-flow joins: missing or ambiguous
    # reachability must leave extraction unresolved rather than manufacture a
    # constructor/table relationship.
    value_by_register: dict[str, tuple[str, str]] = {
        register: ("", "")
        for register in ("eax", "ecx", "edx", "ebx", "esi", "edi", "ebp")
    }
    result: dict[str, ReviewedLoopVptrStorageBridge] = {}
    for instruction, runtime_address in zip(
        retail_instructions,
        instruction_addresses,
    ):
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            body = b""

        if mnemonic in {"call", "jmp"} and len(body) >= 2 and body[0] == 0xFF:
            if ((body[1] >> 3) & 7) in {2, 4} and runtime_address is not None:
                expression, slot = _cc_targets._memory_slot(_cc_cfg._instruction_operand(instruction))
                base_match = re.fullmatch(
                    r"(?P<base>eax|ecx|edx|ebx|esi|edi)"
                    r"(?:\+(?:0x[0-9a-f]+|\d+))?",
                    expression,
                )
                if slot is not None and base_match is not None:
                    base_register = base_match.group("base")
                    vptr_kind, vptr_constructor = value_by_register.get(
                        base_register,
                        ("", ""),
                    )
                    receiver_kind, receiver_constructor = value_by_register.get(
                        "ecx",
                        ("", ""),
                    )
                    matches = selector_by_constructor_slot.get(
                        (vptr_constructor, slot),
                        [],
                    )
                    if (
                        vptr_kind == "vptr"
                        and receiver_kind == "object"
                        and receiver_constructor == vptr_constructor
                        and len(matches) == 1
                    ):
                        storage_identity, _slot, target, _table, _constructor = (
                            matches[0]
                        )
                        result[normalize_address(runtime_address)] = (
                            ReviewedLoopVptrStorageBridge(
                                register=base_register,
                                storage_identity=storage_identity,
                                slot_displacement=slot,
                                assembly_source="bn",
                                target_identity=target,
                            )
                        )
            if mnemonic == "call":
                for volatile in ("eax", "ecx", "edx"):
                    value_by_register[volatile] = ("", "")
            else:
                value_by_register = {
                    register: ("", "") for register in value_by_register
                }
            continue

        if mnemonic == "call" and len(body) == 5 and body[0] == 0xE8:
            if runtime_address is None:
                value_by_register = {
                    register: ("", "") for register in value_by_register
                }
                continue
            target = normalize_address(
                runtime_address + 5 + struct.unpack_from("<i", body, 1)[0]
            )
            for volatile in ("eax", "ecx", "edx"):
                value_by_register[volatile] = ("", "")
            if target in constructor_targets:
                value_by_register["eax"] = ("object", target)
            continue

        move = _cc_receiver_instructions._exact_register_move(instruction)
        if move is not None:
            destination, source_register = move
            if destination in value_by_register:
                value_by_register[destination] = value_by_register.get(
                    source_register,
                    ("", ""),
                )
            continue

        memory_move = re.fullmatch(
            r"mov\s+(?P<destination>eax|ecx|edx|ebx|esi|edi|ebp)\s*,\s*"
            r"(?:dword\s+(?:ptr\s+)?)?\[(?P<source>[^\]]+)\]",
            instruction.raw_text.strip(),
            flags=re.IGNORECASE,
        )
        if memory_move is not None:
            destination = memory_move.group("destination").lower()
            expression, displacement = _cc_targets._memory_slot(
                f"[{memory_move.group('source')}]"
            )
            source_match = re.fullmatch(
                r"eax|ecx|edx|ebx|esi|edi|ebp",
                expression,
            )
            source_value = (
                value_by_register.get(source_match.group(0), ("", ""))
                if source_match is not None and displacement in {None, 0}
                else ("", "")
            )
            value_by_register[destination] = (
                ("vptr", source_value[1])
                if source_value[0] == "object"
                else ("", "")
            )
            continue

        if mnemonic.startswith("j") or mnemonic in {
            "loop",
            "loope",
            "loopne",
            "jecxz",
        }:
            value_by_register = {
                register: ("", "") for register in value_by_register
            }
            continue
        for written in _cc_instructions.written_registers(instruction):
            if written in value_by_register:
                value_by_register[written] = ("", "")
    return result


def _exact_zeroarg_constructor_row_name(
    candidate_name: str,
) -> str | None:
    """Map one exact VC5 constructor decoration to ``Class::Class``."""
    match = _cc_catalog.MSVC_EXACT_ZEROARG_CONSTRUCTOR_RE.fullmatch(candidate_name)
    if match is None:
        return None
    encoded_parts = match.group("class_scope").split("@")
    class_identity = "::".join(reversed(encoded_parts))
    return f"{class_identity}::{encoded_parts[0]}"


def _exact_parameterized_constructor_row_name(
    candidate_name: str,
) -> str | None:
    """Map one exact ordinary one-enum VC5 constructor to ``Class::Constructor``."""
    match = _cc_catalog.MSVC_EXACT_SINGLE_ENUM_PARAMETER_CONSTRUCTOR_RE.fullmatch(
        candidate_name
    )
    if match is None:
        return None
    encoded_parts = match.group("class_scope").split("@")
    class_identity = "::".join(reversed(encoded_parts))
    return f"{class_identity}::Constructor"


def _exact_named_zeroarg_constructor_row_name(
    candidate_name: str,
) -> str | None:
    """Map one exact self-returning ``Constructor()`` method to its row.

    This is disjoint from ordinary ``??0`` constructors.  The decoration must
    literally name ``Constructor``, use the VC5 public thiscall ``QAE``
    package, return ``PAU1@`` (a pointer to the same encoded class scope), and
    end in ``XZ`` with zero explicit arguments.
    """
    match = _cc_catalog.MSVC_EXACT_NAMED_ZEROARG_CONSTRUCTOR_RE.fullmatch(
        candidate_name
    )
    if match is None:
        return None
    encoded_parts = match.group("class_scope").split("@")
    class_identity = "::".join(reversed(encoded_parts))
    return f"{class_identity}::Constructor"
