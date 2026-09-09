"""Recoil call-contract recoil input evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import abi as _cc_abi
from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import extraction as _cc_extraction
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        IdentityIndexes,
        ReviewedLoopVptrStorageBridge,
        ReviewedMemberVptrStorageBridge,
        ReviewedRegisterCallStorageBridge,
        ReviewedStaticStorageReferenceBridge,
    )

import re
import struct
from collections import Counter
from dataclasses import replace
from pathlib import Path
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_DIR32,
    IMAGE_REL_I386_REL32,
    Instruction,
)
from _recoil.commands.provider_target_mutation import _retail_import_targets
from _recoil.lib.binja import BinaryNinjaBridge
from _recoil.lib.progress import ProgressDocument, address_value, normalize_address


def _zinput_indexed_callback_identity(base: str) -> str:
    """Render the verifier's existing bounded indexed-storage convention."""

    return f"load(indexed({base},stride=0x4))"


def _zinput_runtime_dispatch_storage_identities(
    indexes: IdentityIndexes,
    *,
    caller_start: str,
) -> dict[str, str]:
    required = {
        "device": "0x561cc8",
        "raw": "0x565bc4",
        "raw-context": "0x565bc8",
        "key-table": "0x561cd4",
    }
    if caller_start == "0x470e80":
        required["bindmap-current"] = "0x565ea0"
    identities = {
        key: indexes.storage_by_address.get(address, "")
        for key, address in required.items()
    }
    if (
        any(
            not identity.startswith("storage:recoil:data:")
            or identity in indexes.provider_ids
            for identity in identities.values()
        )
        or len(set(identities.values())) != len(identities)
        or any(
            indexes.reviewed_logical_aliases_by_address.get(address)
            for address in required.values()
        )
    ):
        raise ValueError(
            "zInput runtime-dispatch bridge requires unique tracker-backed "
            "authored storage identities"
        )
    identities["device-vptr"] = f"load({identities['device']})"
    identities["key-callback"] = _zinput_indexed_callback_identity(
        _cc_receiver_instructions._abstract_with_displacement(identities["key-table"], 4)
    )
    identities["bindmap-this-callback"] = (
        _zinput_indexed_callback_identity("load(this+0xc)")
    )
    if "bindmap-current" in identities:
        identities["bindmap-current-callback"] = (
            _zinput_indexed_callback_identity(
                f"load({identities['bindmap-current']}+0xc)"
            )
        )
    return identities


def _zinput_runtime_aggregate_field_is_bounded(
    *,
    document: ProgressDocument,
    indexes: IdentityIndexes,
    caller_start: str,
    storage_address: str,
    storage_identity: str,
) -> bool:
    """Authenticate one finite keyboard field inside the accepted aggregate."""
    from _recoil.call_contract.records import StorageContainer

    normalized_start = normalize_address(caller_start)
    normalized_address = normalize_address(storage_address)
    leaf_name = _cc_catalog.ZINPUT_KEYBOARD_AGGREGATE_FIELD_NAMES.get(normalized_address)
    if (
        normalized_start not in {"0x46f450", "0x46f690"}
        or leaf_name is None
    ):
        return False

    aggregate_symbol_id = _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_SYMBOL_ID
    aggregate_storage_id = _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_STORAGE_ID
    aggregate_identity = f"storage:{aggregate_symbol_id}"
    leaf_symbol_id = f"recoil:data:{normalized_address}"
    leaf_storage_id = f"recoil:storage:va:{normalized_address}"
    leaf_identity = f"storage:{leaf_symbol_id}"
    if storage_identity != leaf_identity or aggregate_identity == leaf_identity:
        return False

    symbols = document.collection("symbols")
    contributions = document.collection("storage_contributions")
    owners = document.collection("owners")
    aggregate_symbol = symbols.get(aggregate_symbol_id)
    leaf_symbol = symbols.get(leaf_symbol_id)
    aggregate_contribution = contributions.get(aggregate_storage_id)
    leaf_contribution = contributions.get(leaf_storage_id)
    owner = owners.get(_cc_catalog.ZINPUT_JOYSTICK_STORAGE_OWNER_ID)

    def exact_authored_data(
        row: Any,
        *,
        address: str,
        name: str,
        contribution_id: str,
    ) -> bool:
        return (
            isinstance(row, Mapping)
            and row.get("address") == address
            and row.get("binary") == "recoil"
            and row.get("kind") == "data"
            and row.get("disposition") == "authored"
            and row.get("navigation_name") == name
            and row.get("output_section_id") == "recoil:section:.data"
            and row.get("storage_contribution_ids") == [contribution_id]
            and not row.get("logical_aliases")
        )

    def exact_storage_contribution(
        row: Any,
        *,
        symbol_id: str,
        address: str,
    ) -> bool:
        reference = row.get("reference") if isinstance(row, Mapping) else None
        return (
            isinstance(row, Mapping)
            and row.get("binary") == "recoil"
            and row.get("kind") == "data-symbol"
            and row.get("output_section_id") == "recoil:section:.data"
            and row.get("overlap") == "none"
            and row.get("owner_ids") == [_cc_catalog.ZINPUT_JOYSTICK_STORAGE_OWNER_ID]
            and row.get("parent_contribution_id") is None
            and row.get("symbol_ids") == [symbol_id]
            and isinstance(reference, Mapping)
            and reference.get("address") == address
        )

    gates = owner.get("gates") if isinstance(owner, Mapping) else None
    selected_symbol_ids = {aggregate_symbol_id, leaf_symbol_id}
    primary_data_rows = [
        (
            owner_id,
            str(relationship.get("address", "")),
            str(relationship.get("name", "")),
            str(relationship.get("symbol_id", "")),
        )
        for owner_id, owner_row in owners.items()
        if isinstance(owner_row, Mapping)
        for relationship in owner_row.get("relationships", ())
        if isinstance(relationship, Mapping)
        and relationship.get("kind") == "primary-data"
        and relationship.get("symbol_id") in selected_symbol_ids
    ]
    expected_primary_data_rows = [
        (
            _cc_catalog.ZINPUT_JOYSTICK_STORAGE_OWNER_ID,
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS,
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_NAME,
            aggregate_symbol_id,
        ),
        (
            _cc_catalog.ZINPUT_JOYSTICK_STORAGE_OWNER_ID,
            normalized_address,
            leaf_name,
            leaf_symbol_id,
        ),
    ]
    aggregate_start = address_value(_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS)
    field_start = address_value(normalized_address)
    overlapping_containers = [
        container
        for container in indexes.storage_containers
        if container.start < field_start + 4
        and field_start < container.end_exclusive
    ]
    return (
        exact_authored_data(
            aggregate_symbol,
            address=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS,
            name=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_NAME,
            contribution_id=aggregate_storage_id,
        )
        and exact_authored_data(
            leaf_symbol,
            address=normalized_address,
            name=leaf_name,
            contribution_id=leaf_storage_id,
        )
        and exact_storage_contribution(
            aggregate_contribution,
            symbol_id=aggregate_symbol_id,
            address=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS,
        )
        and exact_storage_contribution(
            leaf_contribution,
            symbol_id=leaf_symbol_id,
            address=normalized_address,
        )
        and isinstance(owner, Mapping)
        and owner.get("binary") == "recoil"
        and owner.get("kind") == "subsystem"
        and owner.get("lifecycle_state") == "active"
        and owner.get("provider_state") == "pending"
        and isinstance(gates, Mapping)
        and all(
            gates.get(gate) == "accepted"
            for gate in ("boundary", "source", "data", "owner_linkage")
        )
        and Counter(primary_data_rows) == Counter(expected_primary_data_rows)
        and aggregate_start < field_start
        and field_start + 4 <= _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_END_EXCLUSIVE
        and indexes.storage_by_address.get(_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS)
        == aggregate_identity
        and indexes.storage_by_address.get(normalized_address) == leaf_identity
        and indexes.storage_by_name.get(_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_NAME)
        == aggregate_identity
        and indexes.storage_by_name.get(leaf_name) == leaf_identity
        and [
            address
            for address, identity in indexes.storage_by_address.items()
            if identity == aggregate_identity
        ]
        == [_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS]
        and [
            address
            for address, identity in indexes.storage_by_address.items()
            if identity == leaf_identity
        ]
        == [normalized_address]
        and aggregate_identity not in indexes.provider_ids
        and leaf_identity not in indexes.provider_ids
        and not indexes.reviewed_logical_aliases_by_address.get(
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS
        )
        and not indexes.reviewed_logical_aliases_by_address.get(
            normalized_address
        )
        and overlapping_containers
        == [
            StorageContainer(
                start=aggregate_start,
                end_exclusive=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_END_EXCLUSIVE,
                identity=aggregate_identity,
            )
        ]
    )


def _zinput_runtime_dispatch_non_callback_register_loads(
    retail_instructions: Sequence[Instruction],
    *,
    document: ProgressDocument,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge: BinaryNinjaBridge,
    reference: Path = _cc_catalog.DEFAULT_REFERENCE,
    retail_import_targets: Sequence[Any] | None = None,
) -> dict[str, str]:
    """Publish the complete Reset/Poll zero-data non-callback census."""

    normalized_start = normalize_address(caller_start)
    spec = _cc_catalog.ZINPUT_RUNTIME_DISPATCH_RETAIL_SPECS.get(normalized_start)
    if normalized_start == _cc_catalog.ZINPUT_TRANSLATE_CALLER_START:
        spec = {
            "end": _cc_catalog.ZINPUT_TRANSLATE_CALLER_END,
            "non_callback_loads": ((
                "0x46fba0",
                bytes.fromhex("a1 48 1c 56 00"),
                "0x561c48",
            ),),
        }
    definitions = tuple(spec.get("non_callback_loads", ())) if spec else ()
    callback_definitions = tuple(spec.get("callback_loads", ())) if spec else ()
    if not definitions:
        return {}
    if normalize_address(caller_end_exclusive) != spec["end"]:
        raise ValueError(
            "zInput non-callback load proof requires the exact caller extent"
        )
    if normalized_start == _cc_catalog.ZINPUT_TRANSLATE_CALLER_START:
        exact_rows = {
            "0x46fba0": bytes.fromhex("a1 48 1c 56 00"),
            "0x46fba5": bytes.fromhex("56"),
            "0x46fba6": bytes.fromhex("85 c0"),
            "0x46fbaa": bytes.fromhex("75 0f"),
            "0x46fbac": bytes.fromhex("e8 6f 01 00 00"),
            "0x46fbb1": bytes.fromhex("c7 05 48 1c 56 00 01 00 00 00"),
        }
        by_address = {
            _cc_cfg._source_instruction_address(row): row
            for row in retail_instructions
            if _cc_cfg._source_instruction_address(row) is not None
        }
        for address, expected_body in exact_rows.items():
            row = by_address.get(address)
            try:
                body = bytes(int(item, 16) for item in row.bytes) if row else b""
            except (TypeError, ValueError):
                body = b""
            if body != expected_body:
                raise ValueError(
                    "zInput Translate ready-flag control topology drifted at "
                    f"{address}"
                )
    expected_rows = {
        address: (body, storage)
        for address, body, storage in (*definitions, *callback_definitions)
    }
    if len(expected_rows) != len(definitions) + len(callback_definitions):
        raise ValueError("zInput zero-data definition census collides")
    immutable_import_slots = frozenset(
        normalize_address(str(getattr(target, "address", "")))
        for target in (
            retail_import_targets
            if retail_import_targets is not None
            else _retail_import_targets(reference)[0]
        )
    )
    observed: dict[str, tuple[bytes, str, str]] = {}
    for instruction in retail_instructions:
        definition_address = _cc_cfg._source_instruction_address(instruction)
        if definition_address is None:
            continue
        definition_address = normalize_address(definition_address)
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
        storage_address = normalize_address(
            struct.unpack_from("<I", body, 1 if accumulator else 2)[0]
        )
        storage_identity = indexes.storage_by_address.get(storage_address, "")
        value = address_value(storage_address)
        containers = [
            container
            for container in indexes.storage_containers
            if container.start == value
            and container.end_exclusive == value + 4
            and container.identity == storage_identity
        ]
        aggregate_field_is_bounded = (
            not containers
            and _zinput_runtime_aggregate_field_is_bounded(
                document=document,
                indexes=indexes,
                caller_start=normalized_start,
                storage_address=storage_address,
                storage_identity=storage_identity,
            )
        )
        if (
            not storage_identity
            or (len(containers) != 1 and not aggregate_field_is_bounded)
        ):
            if definition_address in expected_rows:
                raise ValueError(
                    "zInput zero-data definition requires one exact storage "
                    "container or accepted aggregate field at "
                    f"{storage_address}"
                )
            continue
        cell = _cc_cfg._hexdump_bytes(bridge.hexdump(storage_address, 4))
        if len(cell) != 4:
            if definition_address in expected_rows:
                raise ValueError(
                    "zInput zero-data definition lacks exact retail contents "
                    f"at {storage_address}"
                )
            continue
        if cell != b"\x00\x00\x00\x00":
            if definition_address in expected_rows:
                raise ValueError(
                    "zInput zero-data definition storage is not zero at "
                    f"{storage_address}"
                )
            continue
        if (
            storage_identity.startswith("iat:")
            or storage_address in immutable_import_slots
        ):
            if definition_address in expected_rows:
                raise ValueError(
                    "zInput zero-data definition collides with an immutable "
                    f"import at {storage_address}"
                )
            continue
        observed[definition_address] = (body, storage_address, storage_identity)
    expected_addresses = set(expected_rows)
    observed_addresses = set(observed)
    if observed_addresses != expected_addresses:
        missing = sorted(expected_addresses - observed_addresses, key=address_value)
        extra = sorted(observed_addresses - expected_addresses, key=address_value)
        raise ValueError(
            "zInput zero-data definition census cardinality drifted; missing="
            + ",".join(missing)
            + "; extra="
            + ",".join(extra)
        )
    for address, (expected_body, expected_storage) in expected_rows.items():
        body, storage_address, _identity = observed[address]
        if body != expected_body or storage_address != expected_storage:
            raise ValueError(
                "zInput zero-data definition bytes/storage drifted at "
                f"{address}"
            )
    callback_addresses = {row[0] for row in callback_definitions}
    if callback_addresses:
        identities = _zinput_runtime_dispatch_storage_identities(
            indexes,
            caller_start=normalized_start,
        )
        if any(
            observed[address][2] != identities["raw"]
            for address in callback_addresses
        ):
            raise ValueError(
                "zInput zero-data callback partition storage drifted"
            )
    result = {
        address: observed[address][2]
        for address, _body, _storage in definitions
    }
    if len(result) != len(definitions):
        raise ValueError(
            "zInput non-callback definition publication cardinality drifted"
        )
    if any(
        identity in indexes.reviewed_static_callback_target_by_storage
        for identity in result.values()
    ):
        raise ValueError(
            "zInput non-callback definition collides with a static callback"
        )
    return result


def _zinput_runtime_dispatch_retail_bridges(
    retail_instructions: Sequence[Instruction],
    *,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedRegisterCallStorageBridge],
    dict[str, ReviewedLoopVptrStorageBridge],
]:
    """Publish only the five exact r4387 runtime-selected dispatch routes."""
    from _recoil.call_contract.records import (
        ReviewedLoopVptrStorageBridge,
        ReviewedRegisterCallStorageBridge,
    )

    normalized_start = normalize_address(caller_start)
    spec = _cc_catalog.ZINPUT_RUNTIME_DISPATCH_RETAIL_SPECS.get(normalized_start)
    if spec is None:
        return {}, {}
    if normalize_address(caller_end_exclusive) != spec["end"]:
        raise ValueError(
            "zInput runtime-dispatch retail bridge requires the exact caller "
            "extent"
        )
    by_address: dict[str, list[Instruction]] = {}
    for instruction in retail_instructions:
        address = _cc_cfg._source_instruction_address(instruction)
        if address:
            by_address.setdefault(address, []).append(instruction)
    for address, body in spec["rows"]:
        rows = by_address.get(address, ())
        if (
            len(rows) != 1
            or bytes(int(item, 16) for item in rows[0].bytes) != body
        ):
            raise ValueError(
                "zInput runtime-dispatch retail bytes/neighbors drifted at "
                f"{address}"
            )
    expected_calls = tuple(spec["calls"])
    actual_calls = tuple(
        address
        for instruction in retail_instructions
        if (address := _cc_cfg._source_instruction_address(instruction))
        and (
            _cc_cfg._instruction_mnemonic(instruction) == "call"
            or address in expected_calls
            and _cc_cfg._instruction_mnemonic(instruction) == "jmp"
        )
    )
    if actual_calls != expected_calls:
        raise ValueError(
            "zInput runtime-dispatch retail invocation population drifted"
        )
    identities = _zinput_runtime_dispatch_storage_identities(
        indexes,
        caller_start=normalized_start,
    )
    register_bridges: dict[str, ReviewedRegisterCallStorageBridge] = {}
    for address, register, storage_kind, form in spec.get(
        "register_callbacks", ()
    ):
        storage_identity = {
            "key-table": identities["key-callback"],
            "bindmap-this": identities["bindmap-this-callback"],
            "bindmap-current": identities.get(
                "bindmap-current-callback", ""
            ),
        }[storage_kind]
        register_bridges[address] = ReviewedRegisterCallStorageBridge(
            register=register,
            storage_identity=storage_identity,
            identity_kind="callback",
            assembly_source="bn",
            form=form,
        )
    vptr_bridges = {
        address: ReviewedLoopVptrStorageBridge(
            register=register,
            storage_identity=identities["device-vptr"],
            slot_displacement=slot,
            assembly_source="bn",
        )
        for address, register, slot in spec.get("vptr", ())
    }
    return register_bridges, vptr_bridges


def _zinput_runtime_dispatch_normalize_retail_contract(
    expected: Sequence[Mapping[str, Any]],
    call_sites: Sequence[str],
    *,
    caller_start: str,
    indexes: IdentityIndexes,
) -> tuple[list[dict[str, Any]], list[str]]:
    """Resolve the memory callback while preserving physical retail order."""

    normalized_start = normalize_address(caller_start)
    spec = _cc_catalog.ZINPUT_RUNTIME_DISPATCH_RETAIL_SPECS.get(normalized_start)
    if spec is None:
        return [dict(row) for row in expected], list(call_sites)
    if tuple(call_sites) != tuple(spec["calls"]) or len(expected) != len(
        call_sites
    ):
        raise ValueError(
            "zInput runtime-dispatch extracted call population drifted"
        )
    rows_by_site = {
        site: dict(row) for site, row in zip(call_sites, expected)
    }
    raw_site = spec.get("raw_callback_call")
    if raw_site is not None:
        identities = _zinput_runtime_dispatch_storage_identities(
            indexes,
            caller_start=normalized_start,
        )
        raw_row = rows_by_site[raw_site]
        if (
            raw_row.get("form") != "call"
            or raw_row.get("dispatch") != "indirect"
        ):
            raise ValueError(
                "zInput PollState raw callback is not the exact memory call"
            )
        raw_row.update(
            identity_kind="callback",
            target_identity="",
            storage_identity=identities["raw"],
            slot_displacement=None,
            cleanup_bytes=None,
        )
    physical_sites = tuple(spec["calls"])
    physical_rows = [rows_by_site[site] for site in physical_sites]
    for ordinal, row in enumerate(physical_rows):
        row["ordinal"] = ordinal
    return physical_rows, list(physical_sites)


def _zinput_joystick_acquire_device_candidate_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedStaticStorageReferenceBridge],
    dict[str, ReviewedMemberVptrStorageBridge],
]:
    """Prove the exact candidate leaf/vptr chain in DIAcquireJoystickDevice."""
    from _recoil.call_contract.records import (
        ReviewedMemberVptrStorageBridge,
        ReviewedStaticStorageReferenceBridge,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_CALLER_START:
        return {}, {}
    normalized_end = normalize_address(caller_end_exclusive)
    if (
        caller_identity != _cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_CALLER_IDENTITY
        or normalized_end != _cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_CALLER_END_EXCLUSIVE
    ):
        raise ValueError(
            "zInput joystick Acquire member-vptr bridge requires the exact "
            "reviewed caller identity and extent"
        )

    caller_symbol_id = caller_identity.removeprefix("symbol:")
    caller_symbol = document.collection("symbols").get(caller_symbol_id)
    aggregate_symbol = document.collection("symbols").get(
        _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_SYMBOL_ID
    )
    leaf_symbol = document.collection("symbols").get(
        _cc_catalog.ZINPUT_JOYSTICK_DEVICE_SYMBOL_ID
    )
    aggregate_contribution = document.collection(
        "storage_contributions"
    ).get(_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_STORAGE_ID)
    leaf_contribution = document.collection("storage_contributions").get(
        _cc_catalog.ZINPUT_JOYSTICK_DEVICE_STORAGE_ID
    )
    owner = document.collection("owners").get(
        _cc_catalog.ZINPUT_JOYSTICK_STORAGE_OWNER_ID
    )
    caller_trace = (
        caller_symbol.get("source_traceability")
        if isinstance(caller_symbol, Mapping)
        else None
    )
    caller_edges = (
        caller_trace.get("source_edges")
        if isinstance(caller_trace, Mapping)
        else None
    )
    owner_relationships = (
        owner.get("relationships") if isinstance(owner, Mapping) else None
    )

    def exact_authored_data(
        row: Any,
        *,
        address: str,
        name: str,
        contribution_id: str,
    ) -> bool:
        return (
            isinstance(row, Mapping)
            and row.get("address") == address
            and row.get("binary") == "recoil"
            and row.get("kind") == "data"
            and row.get("disposition") == "authored"
            and row.get("navigation_name") == name
            and row.get("output_section_id") == "recoil:section:.data"
            and row.get("storage_contribution_ids") == [contribution_id]
            and not row.get("logical_aliases")
        )

    def exact_storage_contribution(
        row: Any,
        *,
        symbol_id: str,
        address: str,
    ) -> bool:
        reference = row.get("reference") if isinstance(row, Mapping) else None
        return (
            isinstance(row, Mapping)
            and row.get("binary") == "recoil"
            and row.get("kind") == "data-symbol"
            and row.get("output_section_id") == "recoil:section:.data"
            and row.get("overlap") == "none"
            and row.get("owner_ids") == [_cc_catalog.ZINPUT_JOYSTICK_STORAGE_OWNER_ID]
            and row.get("parent_contribution_id") is None
            and row.get("symbol_ids") == [symbol_id]
            and isinstance(reference, Mapping)
            and reference.get("address") == address
        )

    exact_primary_data = {
        (
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS,
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_NAME,
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_SYMBOL_ID,
        ),
        (
            _cc_catalog.ZINPUT_JOYSTICK_DEVICE_ADDRESS,
            _cc_catalog.ZINPUT_JOYSTICK_DEVICE_NAME,
            _cc_catalog.ZINPUT_JOYSTICK_DEVICE_SYMBOL_ID,
        ),
    }
    matching_primary_data = {
        (
            str(row.get("address", "")),
            str(row.get("name", "")),
            str(row.get("symbol_id", "")),
        )
        for row in (owner_relationships or ())
        if isinstance(row, Mapping)
        and row.get("kind") == "primary-data"
        and row.get("symbol_id")
        in {
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_SYMBOL_ID,
            _cc_catalog.ZINPUT_JOYSTICK_DEVICE_SYMBOL_ID,
        }
    }
    aggregate_identity = f"storage:{_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_SYMBOL_ID}"
    leaf_identity = f"storage:{_cc_catalog.ZINPUT_JOYSTICK_DEVICE_SYMBOL_ID}"
    if (
        not isinstance(caller_symbol, Mapping)
        or caller_symbol.get("address")
        != _cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_CALLER_START
        or caller_symbol.get("end_exclusive")
        != _cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_CALLER_END_EXCLUSIVE
        or caller_symbol.get("binary") != "recoil"
        or caller_symbol.get("kind") != "function"
        or caller_symbol.get("pipeline_class") != "authored"
        or caller_symbol.get("authored_order_role") != "authored-body"
        or caller_symbol.get("ownership_state") != "primary-owned"
        or caller_symbol.get("extent_state") != "known"
        or caller_symbol.get("size") != 0x20
        or caller_symbol.get("navigation_name")
        != "zInput::DIAcquireJoystickDevice"
        or caller_symbol.get("output_section_id") != "recoil:section:.text"
        or caller_symbol.get("physical_block_id") != "recoil:block:0x471e40"
        or caller_identity in indexes.provider_ids
        or indexes.by_address.get(_cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_CALLER_START)
        != caller_identity
        or indexes.by_candidate_name.get(
            _cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_CALLER_SYMBOL
        )
        != caller_identity
        or [
            address
            for address, identity in indexes.by_address.items()
            if identity == caller_identity
        ]
        != [_cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_CALLER_START]
        or [
            name
            for name, identity in indexes.by_candidate_name.items()
            if identity == caller_identity
        ]
        != [_cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_CALLER_SYMBOL]
        or not isinstance(caller_trace, Mapping)
        or caller_trace.get("state") != "resolved"
        or caller_trace.get("reason_code") not in {None, ""}
        or not isinstance(caller_edges, list)
        or len(caller_edges) != 1
        or not isinstance(caller_edges[0], Mapping)
        or caller_edges[0].get("relation") != "defines"
        or caller_edges[0].get("anchor_id")
        != _cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_CALLER_ANCHOR_ID
        or caller_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.ZINPUT_JOYSTICK_SOURCE_PATH}
        or not exact_authored_data(
            aggregate_symbol,
            address=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS,
            name=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_NAME,
            contribution_id=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_STORAGE_ID,
        )
        or not exact_authored_data(
            leaf_symbol,
            address=_cc_catalog.ZINPUT_JOYSTICK_DEVICE_ADDRESS,
            name=_cc_catalog.ZINPUT_JOYSTICK_DEVICE_NAME,
            contribution_id=_cc_catalog.ZINPUT_JOYSTICK_DEVICE_STORAGE_ID,
        )
        or not exact_storage_contribution(
            aggregate_contribution,
            symbol_id=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_SYMBOL_ID,
            address=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS,
        )
        or not exact_storage_contribution(
            leaf_contribution,
            symbol_id=_cc_catalog.ZINPUT_JOYSTICK_DEVICE_SYMBOL_ID,
            address=_cc_catalog.ZINPUT_JOYSTICK_DEVICE_ADDRESS,
        )
        or not isinstance(owner, Mapping)
        or owner.get("binary") != "recoil"
        or owner.get("kind") != "subsystem"
        or owner.get("provider_state") != "pending"
        or matching_primary_data != exact_primary_data
        or indexes.storage_by_address.get(
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS
        )
        != aggregate_identity
        or indexes.storage_by_address.get(_cc_catalog.ZINPUT_JOYSTICK_DEVICE_ADDRESS)
        != leaf_identity
        or indexes.storage_by_name.get(_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_NAME)
        != aggregate_identity
        or indexes.storage_by_name.get(_cc_catalog.ZINPUT_JOYSTICK_DEVICE_NAME)
        != leaf_identity
        or [
            address
            for address, identity in indexes.storage_by_address.items()
            if identity == aggregate_identity
        ]
        != [_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS]
        or [
            address
            for address, identity in indexes.storage_by_address.items()
            if identity == leaf_identity
        ]
        != [_cc_catalog.ZINPUT_JOYSTICK_DEVICE_ADDRESS]
        or aggregate_identity in indexes.provider_ids
        or leaf_identity in indexes.provider_ids
        or indexes.reviewed_logical_aliases_by_address.get(
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS
        )
        or indexes.reviewed_logical_aliases_by_address.get(
            _cc_catalog.ZINPUT_JOYSTICK_DEVICE_ADDRESS
        )
    ):
        raise ValueError(
            "zInput joystick Acquire member-vptr bridge requires unique "
            "authored non-provider caller, aggregate, leaf, storage, and "
            "owner provenance"
        )

    target = candidate.target
    definition = candidate.caller_definition
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_CALLER_START
    ]
    if (
        target is None
        or getattr(target, "name", "") != _cc_catalog.ZINPUT_JOYSTICK_TARGET_NAME
        or getattr(target, "target_binary", "") != "recoil"
        or Path(str(getattr(target, "manifest_path", ""))).resolve()
        != _cc_catalog.ZINPUT_JOYSTICK_TARGET_MANIFEST.resolve()
        or not bool(
            getattr(target, "check_translation_unit_function_order", False)
        )
        or tuple(getattr(target, "source_files", ()))
        not in {(), (_cc_catalog.ZINPUT_JOYSTICK_SOURCE_PATH,)}
    ):
        raise ValueError(
            "zInput joystick Acquire member-vptr bridge requires the exact "
            "registered target and source listing"
        )
    if len(contribution_rows) != 1:
        raise ValueError(
            "zInput joystick Acquire member-vptr bridge requires one exact "
            "registered caller contribution"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.ZINPUT_JOYSTICK_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "")
        != _cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_CALLER_SYMBOL
        or getattr(contribution_row, "symbol_regex", None) is not None
        or getattr(contribution_row, "name", "")
        != "zInput::DIAcquireJoystickDevice"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "zInput joystick Acquire member-vptr bridge requires the exact "
            "authored zin_joystick.cpp contribution row"
        )

    exact_body = bytes.fromhex(
        "a1 20 3f 00 00 85 c0 74 10 8b 08 50 ff 51 1c "
        "33 d2 85 c0 0f 94 c2 8b c2 c3 33 c0 c3 90 90 90 90"
    )
    if (
        definition is None
        or definition.symbol != _cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_CALLER_SYMBOL
        or definition.data != exact_body
        or len(definition.data) != _cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_CANDIDATE_SIZE
        or len(definition.relocation_mask) != len(definition.data)
        or definition.section_index
        != _cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_COFF_SECTION_INDEX
        or definition.section_start != 0
        or definition.section_end != _cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_CANDIDATE_SIZE
    ):
        raise ValueError(
            "zInput joystick Acquire member-vptr bridge requires the exact "
            "candidate procedure and complete 0x20-byte COFF body"
        )
    if (
        len(definition.relocations) != 1
        or definition.relocations[0].offset != 0x01
        or definition.relocations[0].type != IMAGE_REL_I386_DIR32
        or definition.relocations[0].symbol_name
        != _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL
        or struct.unpack_from("<I", definition.data, 0x01)[0]
        != _cc_catalog.ZINPUT_JOYSTICK_DEVICE_DISPLACEMENT
        or tuple(definition.relocation_mask)
        != tuple(index in range(0x01, 0x05) for index in range(len(exact_body)))
        or definition.undefined_external_data.count(
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL
        )
        != 1
        or _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL
        in definition.defined_external_data
    ):
        raise ValueError(
            "zInput joystick Acquire member-vptr bridge requires the exact "
            "candidate DIR32 leaf relocation, addend, mask, and symbol role"
        )

    expected_storage = f"load({leaf_identity})"
    expected_rows = [
        row
        for row in expected
        if row.get("identity_kind") == "virtual-slot"
        and row.get("storage_identity") == expected_storage
        and row.get("slot_displacement")
        == _cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_VIRTUAL_SLOT
    ]
    if (
        len(expected) != 1
        or len(expected_rows) != 1
        or expected_rows[0].get("ordinal") != 0
        or any(
            expected_rows[0].get(key) != value
            for key, value in {
                "form": "call",
                "dispatch": "indirect",
                "target_identity": "",
                "cleanup_bytes": None,
            }.items()
        )
    ):
        raise ValueError(
            "zInput joystick Acquire member-vptr bridge requires one exact "
            "immutable-retail slot-0x1c contract"
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    counts: dict[int, int] = {}
    for offset in offsets:
        if offset is not None:
            counts[offset] = counts.get(offset, 0) + 1
    index_by_offset = {
        offset: index
        for index, offset in enumerate(offsets)
        if offset is not None and counts.get(offset) == 1
    }
    instruction_by_offset = {
        offset: candidate.instructions[index]
        for offset, index in index_by_offset.items()
    }
    aggregate_pattern = re.escape(_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL)
    exact_rows = {
        0x00: (
            exact_body[0x00:0x05],
            rf"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            rf"{aggregate_pattern}\+16160",
        ),
        0x05: (exact_body[0x05:0x07], r"test\s+eax\s*,\s*eax"),
        0x07: (
            exact_body[0x07:0x09],
            r"je\s+(?:SHORT\s+)?\$L[0-9A-Za-z_]+",
        ),
        0x09: (
            exact_body[0x09:0x0B],
            r"mov\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[eax\]",
        ),
        0x0B: (exact_body[0x0B:0x0C], r"push\s+eax"),
        0x0C: (
            exact_body[0x0C:0x0F],
            r"call\s+(?:dword\s+(?:ptr\s+)?)?\[ecx\+28\]",
        ),
        0x0F: (exact_body[0x0F:0x11], r"xor\s+edx\s*,\s*edx"),
        0x11: (exact_body[0x11:0x13], r"test\s+eax\s*,\s*eax"),
        0x13: (exact_body[0x13:0x16], r"sete\s+dl"),
        0x16: (exact_body[0x16:0x18], r"mov\s+eax\s*,\s*edx"),
        0x18: (exact_body[0x18:0x19], r"ret(?:n)?(?:\s+0)?"),
        0x19: (exact_body[0x19:0x1B], r"xor\s+eax\s*,\s*eax"),
        0x1B: (exact_body[0x1B:0x1C], r"ret(?:n)?(?:\s+0)?"),
    }
    if (
        len(candidate.instructions) != len(exact_rows)
        or len(offsets) != len(exact_rows)
        or any(offset is None for offset in offsets)
        or set(instruction_by_offset) != set(exact_rows)
    ):
        raise ValueError(
            "zInput joystick Acquire member-vptr bridge requires the exact "
            "complete unique COD instruction extent"
        )
    for offset, (body, pattern) in exact_rows.items():
        instruction = instruction_by_offset[offset]
        if (
            bytes(int(value, 16) for value in instruction.bytes) != body
            or definition.data[offset : offset + len(body)] != body
            or re.fullmatch(
                pattern,
                instruction.raw_text.strip(),
                flags=re.IGNORECASE,
            )
            is None
        ):
            raise ValueError(
                "zInput joystick Acquire member-vptr bridge requires exact "
                f"COD bytes/registers/operands at +{hex(offset)}"
            )
    load_anchors = [
        offset
        for offset, instruction in instruction_by_offset.items()
        if re.fullmatch(
            exact_rows[0x00][1],
            instruction.raw_text.strip(),
            flags=re.IGNORECASE,
        )
        is not None
    ]
    if load_anchors != [0x00]:
        raise ValueError(
            "zInput joystick Acquire member-vptr bridge rejects duplicate "
            "or shifted leaf-load anchors"
        )

    start = address_value(normalized_start)
    end = address_value(normalized_end)
    runtime_addresses = _cc_cfg._instruction_runtime_addresses(
        candidate.instructions,
        source="cod",
        caller_start=start,
    )
    runtime_index_by_address = {
        address: index
        for index, address in enumerate(runtime_addresses)
        if address is not None
    }
    successors, unresolved_cfg = _cc_cfg._exact_invocation_cfg(
        candidate.instructions,
        instruction_addresses=runtime_addresses,
        instruction_index_by_address=runtime_index_by_address,
        source="cod",
        caller_start=start,
        caller_end=end,
        local_control_flow_indices=candidate.local_control_flow_indices,
        local_control_flow_targets=candidate.local_control_flow_targets,
    )
    exact_successors = {
        0: (1,),
        1: (2,),
        2: (3, 11),
        3: (4,),
        4: (5,),
        5: (6,),
        6: (7,),
        7: (8,),
        8: (9,),
        9: (10,),
        10: (),
        11: (12,),
        12: (),
    }
    load_index = index_by_offset[0x00]
    vptr_index = index_by_offset[_cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_VPTR_OFFSET]
    receiver_index = index_by_offset[0x0B]
    call_index = index_by_offset[_cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_CALL_OFFSET]
    if (
        candidate.local_control_flow_indices
        or candidate.local_control_flow_targets
        or unresolved_cfg
        or successors != exact_successors
        or _cc_cfg._reachable_cfg_indices(successors, (0,))
        != frozenset(range(len(exact_rows)))
        or any(
            _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index], "eax"
            )
            for index in range(load_index + 1, vptr_index)
        )
        or any(
            _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index], register
            )
            for index in range(vptr_index + 1, call_index)
            for register in ("eax", "ecx")
        )
        or receiver_index != vptr_index + 1
        or call_index != receiver_index + 1
        or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
        or any(
            definition.relocation_mask[
                _cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_VPTR_OFFSET:
                _cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_CALL_OFFSET + 3
            ]
        )
    ):
        raise ValueError(
            "zInput joystick Acquire member-vptr bridge rejects CFG, "
            "register-chain, vptr, receiver, cleanup, or call-byte drift"
        )

    expression = (
        f"{_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL}"
        f"+{_cc_catalog.ZINPUT_JOYSTICK_DEVICE_DISPLACEMENT}"
    )
    static_bridge = ReviewedStaticStorageReferenceBridge(
        aggregate_symbol=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL,
        displacement=_cc_catalog.ZINPUT_JOYSTICK_DEVICE_DISPLACEMENT,
        access_width=4,
        storage_identity=leaf_identity,
    )
    member_bridge = ReviewedMemberVptrStorageBridge(
        register="ecx",
        source_register="eax",
        source_provenance=leaf_identity,
        receiver_register="eax",
        receiver_provenance=leaf_identity,
        storage_identity=expected_storage,
        slot_displacement=_cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_VIRTUAL_SLOT,
        call_address=normalize_address(
            hex(_cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_CALL_OFFSET)
        ),
    )
    structural_contract = _cc_extraction.extract_invocation_contract(
        candidate.instructions,
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        reviewed_static_storage_reference_bridges={expression: static_bridge},
        reviewed_member_vptr_storage_bridges={
            normalize_address(
                hex(_cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_VPTR_OFFSET)
            ): member_bridge
        },
        local_control_flow_indices=candidate.local_control_flow_indices,
        local_control_flow_targets=candidate.local_control_flow_targets,
    )
    if structural_contract != list(expected):
        raise ValueError(
            "zInput joystick Acquire member-vptr bridge cannot independently "
            "derive the immutable-retail contract"
        )
    return (
        {expression: static_bridge},
        {
            normalize_address(
                hex(_cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_VPTR_OFFSET)
            ): member_bridge
        },
    )


def _zinput_joystick_later_aggregate_leaf_candidate_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedStaticStorageReferenceBridge],
    dict[str, ReviewedMemberVptrStorageBridge],
] | None:
    """Project the finite later joystick-device COM bridge population."""
    from _recoil.call_contract.records import (
        ReviewedMemberVptrStorageBridge,
        ReviewedStaticStorageReferenceBridge,
    )

    normalized_start = normalize_address(caller_start)
    spec = _cc_catalog.ZINPUT_JOYSTICK_LATER_AGGREGATE_LEAF_SPECS.get(normalized_start)
    if spec is None:
        return None
    normalized_end = normalize_address(caller_end_exclusive)
    exact_identity = f"symbol:recoil:function:{normalized_start}"
    if caller_identity != exact_identity or normalized_end != spec["end"]:
        raise ValueError(
            "zInput later aggregate-leaf bridge requires the exact reviewed "
            "caller identity and extent"
        )

    symbols = document.collection("symbols")
    caller_symbol = symbols.get(caller_identity.removeprefix("symbol:"))
    aggregate_symbol = symbols.get(_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_SYMBOL_ID)
    leaf_symbol = symbols.get(_cc_catalog.ZINPUT_JOYSTICK_DEVICE_SYMBOL_ID)
    aggregate_contribution = document.collection(
        "storage_contributions"
    ).get(_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_STORAGE_ID)
    leaf_contribution = document.collection("storage_contributions").get(
        _cc_catalog.ZINPUT_JOYSTICK_DEVICE_STORAGE_ID
    )
    owner = document.collection("owners").get(
        _cc_catalog.ZINPUT_JOYSTICK_STORAGE_OWNER_ID
    )
    trace = (
        caller_symbol.get("source_traceability")
        if isinstance(caller_symbol, Mapping)
        else None
    )
    edges = trace.get("source_edges") if isinstance(trace, Mapping) else None
    aggregate_identity = f"storage:{_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_SYMBOL_ID}"
    leaf_identity = f"storage:{_cc_catalog.ZINPUT_JOYSTICK_DEVICE_SYMBOL_ID}"

    def exact_data(
        row: Any, *, address: str, name: str, contribution_id: str
    ) -> bool:
        return bool(
            isinstance(row, Mapping)
            and row.get("address") == address
            and row.get("binary") == "recoil"
            and row.get("kind") == "data"
            and row.get("disposition") == "authored"
            and row.get("navigation_name") == name
            and row.get("output_section_id") == "recoil:section:.data"
            and row.get("storage_contribution_ids") == [contribution_id]
            and not row.get("logical_aliases")
        )

    def exact_contribution(
        row: Any, *, symbol_id: str, address: str
    ) -> bool:
        reference = row.get("reference") if isinstance(row, Mapping) else None
        return bool(
            isinstance(row, Mapping)
            and row.get("binary") == "recoil"
            and row.get("kind") == "data-symbol"
            and row.get("output_section_id") == "recoil:section:.data"
            and row.get("overlap") == "none"
            and row.get("owner_ids") == [_cc_catalog.ZINPUT_JOYSTICK_STORAGE_OWNER_ID]
            and row.get("parent_contribution_id") is None
            and row.get("symbol_ids") == [symbol_id]
            and isinstance(reference, Mapping)
            and reference.get("address") == address
        )

    owner_rows = (
        owner.get("relationships") if isinstance(owner, Mapping) else None
    )
    exact_owner_rows = {
        (
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS,
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_NAME,
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_SYMBOL_ID,
        ),
        (
            _cc_catalog.ZINPUT_JOYSTICK_DEVICE_ADDRESS,
            _cc_catalog.ZINPUT_JOYSTICK_DEVICE_NAME,
            _cc_catalog.ZINPUT_JOYSTICK_DEVICE_SYMBOL_ID,
        ),
    }
    observed_owner_rows = {
        (
            str(row.get("address", "")),
            str(row.get("name", "")),
            str(row.get("symbol_id", "")),
        )
        for row in (owner_rows or ())
        if isinstance(row, Mapping)
        and row.get("kind") == "primary-data"
        and row.get("symbol_id")
        in {
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_SYMBOL_ID,
            _cc_catalog.ZINPUT_JOYSTICK_DEVICE_SYMBOL_ID,
        }
    }
    if (
        not isinstance(caller_symbol, Mapping)
        or caller_symbol.get("address") != normalized_start
        or caller_symbol.get("end_exclusive") != normalized_end
        or caller_symbol.get("binary") != "recoil"
        or caller_symbol.get("kind") != "function"
        or caller_symbol.get("pipeline_class") != "authored"
        or caller_symbol.get("authored_order_role") != "authored-body"
        or caller_symbol.get("ownership_state") != "primary-owned"
        or caller_symbol.get("extent_state") != "known"
        or caller_symbol.get("size") != address_value(normalized_end) - address_value(normalized_start)
        or caller_symbol.get("navigation_name") != spec["name"]
        or caller_symbol.get("output_section_id") != "recoil:section:.text"
        or caller_symbol.get("physical_block_id") != "recoil:block:0x471e40"
        or caller_symbol.get("logical_aliases")
        or caller_identity in indexes.provider_ids
        or indexes.by_address.get(normalized_start) != caller_identity
        or indexes.by_candidate_name.get(spec["symbol"]) != caller_identity
        or [
            address
            for address, identity in indexes.by_address.items()
            if identity == caller_identity
        ]
        != [normalized_start]
        or [
            name
            for name, identity in indexes.by_candidate_name.items()
            if identity == caller_identity
        ]
        != [spec["symbol"]]
        or not isinstance(trace, Mapping)
        or trace.get("state") != "resolved"
        or trace.get("reason_code") not in {None, ""}
        or not isinstance(edges, list)
        or len(edges) != 1
        or edges[0].get("relation") != "defines"
        or edges[0].get("anchor_id") != spec["anchor"]
        or edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.ZINPUT_JOYSTICK_SOURCE_PATH}
        or not exact_data(
            aggregate_symbol,
            address=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS,
            name=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_NAME,
            contribution_id=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_STORAGE_ID,
        )
        or not exact_data(
            leaf_symbol,
            address=_cc_catalog.ZINPUT_JOYSTICK_DEVICE_ADDRESS,
            name=_cc_catalog.ZINPUT_JOYSTICK_DEVICE_NAME,
            contribution_id=_cc_catalog.ZINPUT_JOYSTICK_DEVICE_STORAGE_ID,
        )
        or not exact_contribution(
            aggregate_contribution,
            symbol_id=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_SYMBOL_ID,
            address=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS,
        )
        or not exact_contribution(
            leaf_contribution,
            symbol_id=_cc_catalog.ZINPUT_JOYSTICK_DEVICE_SYMBOL_ID,
            address=_cc_catalog.ZINPUT_JOYSTICK_DEVICE_ADDRESS,
        )
        or not isinstance(owner, Mapping)
        or owner.get("binary") != "recoil"
        or owner.get("kind") != "subsystem"
        or owner.get("provider_state") != "pending"
        or observed_owner_rows != exact_owner_rows
        or indexes.storage_by_address.get(_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS)
        != aggregate_identity
        or indexes.storage_by_address.get(_cc_catalog.ZINPUT_JOYSTICK_DEVICE_ADDRESS)
        != leaf_identity
        or indexes.storage_by_name.get(_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_NAME)
        != aggregate_identity
        or indexes.storage_by_name.get(_cc_catalog.ZINPUT_JOYSTICK_DEVICE_NAME)
        != leaf_identity
        or aggregate_identity in indexes.provider_ids
        or leaf_identity in indexes.provider_ids
        or indexes.reviewed_logical_aliases_by_address.get(
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS
        )
        or indexes.reviewed_logical_aliases_by_address.get(
            _cc_catalog.ZINPUT_JOYSTICK_DEVICE_ADDRESS
        )
    ):
        raise ValueError(
            "zInput later aggregate-leaf bridge requires unique authored "
            "caller, aggregate, leaf, storage, owner, source, and index authority"
        )

    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == normalized_start
    ]
    if (
        target is None
        or getattr(target, "name", "") != _cc_catalog.ZINPUT_JOYSTICK_TARGET_NAME
        or getattr(target, "target_binary", "") != "recoil"
        or Path(str(getattr(target, "manifest_path", ""))).resolve()
        != _cc_catalog.ZINPUT_JOYSTICK_TARGET_MANIFEST.resolve()
        or not bool(
            getattr(target, "check_translation_unit_function_order", False)
        )
        or tuple(getattr(target, "source_files", ()))
        not in {(), (_cc_catalog.ZINPUT_JOYSTICK_SOURCE_PATH,)}
        or len(contribution_rows) != 1
    ):
        raise ValueError(
            "zInput later aggregate-leaf bridge requires the exact registered target"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.ZINPUT_JOYSTICK_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != spec["symbol"]
        or getattr(contribution_row, "symbol_regex", None) is not None
        or getattr(contribution_row, "name", "") != spec["name"]
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "zInput later aggregate-leaf bridge requires the exact authored contribution"
        )

    definition = candidate.caller_definition
    body = spec["body"]
    expected_relocations = tuple(spec["relocations"])
    observed_relocations = tuple(
        (row.offset, row.type, row.symbol_name)
        for row in getattr(definition, "relocations", ())
    ) if definition is not None else ()
    mask = [False] * len(body)
    for offset, _reloc_type, _symbol_name in expected_relocations:
        mask[offset : offset + 4] = [True] * 4
    if (
        definition is None
        or definition.symbol != spec["symbol"]
        or definition.data != body
        or definition.section_index <= 0
        or definition.section_start != 0
        or definition.section_end != len(body)
        or len(definition.relocation_mask) != len(body)
        or tuple(definition.relocation_mask) != tuple(mask)
        or observed_relocations != expected_relocations
    ):
        raise ValueError(
            "zInput later aggregate-leaf bridge requires exact complete COFF "
            "body, section, relocation partition, and mask"
        )
    aggregate_leaf_relocations = [
        offset
        for offset, reloc_type, symbol_name in expected_relocations
        if reloc_type == IMAGE_REL_I386_DIR32
        and symbol_name == _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL
        and struct.unpack_from("<I", body, offset)[0]
        == _cc_catalog.ZINPUT_JOYSTICK_DEVICE_DISPLACEMENT
    ]
    required_load_relocations = list(spec["load_relocation_offsets"])
    if not set(required_load_relocations).issubset(
        set(aggregate_leaf_relocations)
    ):
        raise ValueError(
            "zInput later aggregate-leaf bridge requires the exact aggregate leaf relocations"
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    if (
        len(candidate.instructions) != spec["instruction_count"]
        or len(offsets) != spec["instruction_count"]
        or any(offset is None for offset in offsets)
        or len(set(offsets)) != len(offsets)
        or tuple(offsets) != tuple(sorted(offsets))
    ):
        raise ValueError(
            "zInput later aggregate-leaf bridge requires the exact complete unique COD extent"
        )
    instruction_by_offset = {
        int(offset): candidate.instructions[index]
        for index, offset in enumerate(offsets)
        if offset is not None
    }
    for offset, instruction in instruction_by_offset.items():
        encoded = bytes(int(value, 16) for value in instruction.bytes)
        if not encoded or body[offset : offset + len(encoded)] != encoded:
            raise ValueError(
                "zInput later aggregate-leaf bridge requires exact COD/body bytes"
            )
    aggregate_pattern = re.compile(
        r"^mov\s+(?:eax|esi)\s*,.*"
        + re.escape(_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL)
        + r"\+16160$",
        flags=re.IGNORECASE,
    )
    observed_load_offsets = [
        offset
        for offset, instruction in instruction_by_offset.items()
        if aggregate_pattern.search(instruction.raw_text.strip()) is not None
    ]
    if observed_load_offsets != list(spec["load_offsets"]):
        raise ValueError(
            "zInput later aggregate-leaf bridge rejects missing, duplicate, or shifted leaf loads"
        )

    start = address_value(normalized_start)
    end = address_value(normalized_end)
    runtime_addresses = _cc_cfg._instruction_runtime_addresses(
        candidate.instructions, source="cod", caller_start=start
    )
    runtime_index = {
        address: index
        for index, address in enumerate(runtime_addresses)
        if address is not None
    }
    successors, unresolved = _cc_cfg._exact_invocation_cfg(
        candidate.instructions,
        instruction_addresses=runtime_addresses,
        instruction_index_by_address=runtime_index,
        source="cod",
        caller_start=start,
        caller_end=end,
        local_control_flow_indices=candidate.local_control_flow_indices,
        local_control_flow_targets=candidate.local_control_flow_targets,
    )
    if (
        candidate.local_control_flow_indices
        or candidate.local_control_flow_targets
        or unresolved
        or _cc_cfg._reachable_cfg_indices(successors, (0,))
        != frozenset(range(len(candidate.instructions)))
    ):
        raise ValueError(
            "zInput later aggregate-leaf bridge rejects CFG or reachability drift"
        )

    expected_storage = f"load({leaf_identity})"
    if len(expected) != spec["expected_count"]:
        raise ValueError(
            "zInput later aggregate-leaf bridge requires the complete immutable-retail contract"
        )
    expression = (
        f"{_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL}"
        f"+{_cc_catalog.ZINPUT_JOYSTICK_DEVICE_DISPLACEMENT}"
    )
    static_bridge = ReviewedStaticStorageReferenceBridge(
        aggregate_symbol=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL,
        displacement=_cc_catalog.ZINPUT_JOYSTICK_DEVICE_DISPLACEMENT,
        access_width=4,
        storage_identity=leaf_identity,
    )
    member_bridges: dict[str, ReviewedMemberVptrStorageBridge] = {}
    for (
        ordinal,
        vptr_offset,
        vptr_register,
        source_register,
        receiver_register,
        call_offset,
        slot,
        cleanup,
    ) in spec["calls"]:
        expected_row = expected[ordinal]
        if any(
            expected_row.get(key) != value
            for key, value in {
                "ordinal": ordinal,
                "form": "call",
                "dispatch": "indirect",
                "identity_kind": "virtual-slot",
                "target_identity": "",
                "storage_identity": expected_storage,
                "slot_displacement": slot,
                "cleanup_bytes": cleanup,
            }.items()
        ):
            raise ValueError(
                "zInput later aggregate-leaf bridge requires exact immutable-retail COM rows"
            )
        if vptr_offset not in instruction_by_offset or call_offset not in instruction_by_offset:
            raise ValueError(
                "zInput later aggregate-leaf bridge requires exact vptr and call offsets"
            )
        vptr_index = offsets.index(vptr_offset)
        call_index = offsets.index(call_offset)
        load_offset = max(
            offset for offset in spec["load_offsets"] if offset <= vptr_offset
        )
        load_index = offsets.index(load_offset)
        if (
            call_index <= vptr_index
            or any(
                _cc_cfg._instruction_may_clobber_register(
                    candidate.instructions[index], source_register
                )
                and not (
                    source_register in {"ebx", "esi", "edi", "ebp"}
                    and _cc_cfg._instruction_mnemonic(
                        candidate.instructions[index]
                    )
                    == "call"
                )
                for index in range(load_index + 1, vptr_index)
            )
            or any(
                _cc_cfg._instruction_may_clobber_register(
                    candidate.instructions[index], register
                )
                for index in range(vptr_index + 1, call_index)
                for register in (source_register, vptr_register)
            )
            or _cc_cfg._cleanup_after(candidate.instructions, call_index) != cleanup
            or any(
                definition.relocation_mask[
                    vptr_offset:
                    vptr_offset
                    + len(instruction_by_offset[vptr_offset].bytes)
                ]
            )
            or any(
                definition.relocation_mask[
                    call_offset:
                    call_offset
                    + len(instruction_by_offset[call_offset].bytes)
                ]
            )
        ):
            raise ValueError(
                "zInput later aggregate-leaf bridge rejects register, vptr, cleanup, or call drift"
            )
        key = normalize_address(hex(vptr_offset))
        member_bridges[key] = ReviewedMemberVptrStorageBridge(
            register=vptr_register,
            source_register=source_register,
            source_provenance=leaf_identity,
            receiver_register=receiver_register,
            receiver_provenance=leaf_identity,
            storage_identity=expected_storage,
            slot_displacement=slot,
            call_address=normalize_address(hex(call_offset)),
        )
    structural = _cc_extraction.extract_invocation_contract(
        candidate.instructions,
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        reviewed_static_storage_reference_bridges={expression: static_bridge},
        reviewed_member_vptr_storage_bridges=member_bridges,
        local_control_flow_indices=candidate.local_control_flow_indices,
        local_control_flow_targets=candidate.local_control_flow_targets,
    )
    if structural != list(expected):
        raise ValueError(
            "zInput later aggregate-leaf bridge cannot independently derive "
            "the complete immutable-retail contract"
        )
    return {expression: static_bridge}, member_bridges


def _zinput_joystick_aggregate_leaf_candidate_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedStaticStorageReferenceBridge],
    dict[str, ReviewedMemberVptrStorageBridge],
]:
    """Project one proven zInput aggregate leaf into candidate provenance."""
    from _recoil.call_contract.records import (
        ReviewedMemberVptrStorageBridge,
        ReviewedStaticStorageReferenceBridge,
    )
    normalized_start = normalize_address(caller_start)
    later = _zinput_joystick_later_aggregate_leaf_candidate_bridges(
        expected,
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    if later is not None:
        return later
    if normalized_start == _cc_catalog.ZINPUT_JOYSTICK_ACQUIRE_CALLER_START:
        return _zinput_joystick_acquire_device_candidate_bridges(
            expected,
            candidate,
            document=document,
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=indexes,
        )
    if normalized_start != _cc_catalog.ZINPUT_JOYSTICK_INIT_CALLER_START:
        return {}, {}
    if (
        caller_identity != _cc_catalog.ZINPUT_JOYSTICK_INIT_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.ZINPUT_JOYSTICK_INIT_CALLER_END_EXCLUSIVE
    ):
        raise ValueError(
            "zInput joystick aggregate-leaf bridge requires the exact "
            "reviewed caller identity and extent"
        )

    caller_symbol_id = caller_identity.removeprefix("symbol:")
    caller_symbol = document.collection("symbols").get(caller_symbol_id)
    aggregate_symbol = document.collection("symbols").get(
        _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_SYMBOL_ID
    )
    leaf_symbol = document.collection("symbols").get(
        _cc_catalog.ZINPUT_JOYSTICK_DEVICE_SYMBOL_ID
    )
    aggregate_contribution = document.collection(
        "storage_contributions"
    ).get(_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_STORAGE_ID)
    leaf_contribution = document.collection("storage_contributions").get(
        _cc_catalog.ZINPUT_JOYSTICK_DEVICE_STORAGE_ID
    )
    owner = document.collection("owners").get(
        _cc_catalog.ZINPUT_JOYSTICK_STORAGE_OWNER_ID
    )
    caller_trace = (
        caller_symbol.get("source_traceability")
        if isinstance(caller_symbol, Mapping)
        else None
    )
    caller_edges = (
        caller_trace.get("source_edges")
        if isinstance(caller_trace, Mapping)
        else None
    )
    owner_relationships = (
        owner.get("relationships") if isinstance(owner, Mapping) else None
    )

    def exact_authored_data(
        row: Any,
        *,
        address: str,
        name: str,
        contribution_id: str,
    ) -> bool:
        return (
            isinstance(row, Mapping)
            and row.get("address") == address
            and row.get("binary") == "recoil"
            and row.get("kind") == "data"
            and row.get("disposition") == "authored"
            and row.get("navigation_name") == name
            and row.get("output_section_id") == "recoil:section:.data"
            and row.get("storage_contribution_ids") == [contribution_id]
            and not row.get("logical_aliases")
        )

    def exact_storage_contribution(
        row: Any,
        *,
        symbol_id: str,
        address: str,
    ) -> bool:
        reference = row.get("reference") if isinstance(row, Mapping) else None
        return (
            isinstance(row, Mapping)
            and row.get("binary") == "recoil"
            and row.get("kind") == "data-symbol"
            and row.get("output_section_id") == "recoil:section:.data"
            and row.get("overlap") == "none"
            and row.get("owner_ids") == [_cc_catalog.ZINPUT_JOYSTICK_STORAGE_OWNER_ID]
            and row.get("parent_contribution_id") is None
            and row.get("symbol_ids") == [symbol_id]
            and isinstance(reference, Mapping)
            and reference.get("address") == address
        )

    exact_primary_data = {
        (
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS,
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_NAME,
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_SYMBOL_ID,
        ),
        (
            _cc_catalog.ZINPUT_JOYSTICK_DEVICE_ADDRESS,
            _cc_catalog.ZINPUT_JOYSTICK_DEVICE_NAME,
            _cc_catalog.ZINPUT_JOYSTICK_DEVICE_SYMBOL_ID,
        ),
    }
    matching_primary_data = {
        (
            str(row.get("address", "")),
            str(row.get("name", "")),
            str(row.get("symbol_id", "")),
        )
        for row in (owner_relationships or ())
        if isinstance(row, Mapping)
        and row.get("kind") == "primary-data"
        and row.get("symbol_id")
        in {
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_SYMBOL_ID,
            _cc_catalog.ZINPUT_JOYSTICK_DEVICE_SYMBOL_ID,
        }
    }
    aggregate_identity = f"storage:{_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_SYMBOL_ID}"
    leaf_identity = f"storage:{_cc_catalog.ZINPUT_JOYSTICK_DEVICE_SYMBOL_ID}"
    if (
        not isinstance(caller_symbol, Mapping)
        or caller_symbol.get("address") != _cc_catalog.ZINPUT_JOYSTICK_INIT_CALLER_START
        or caller_symbol.get("end_exclusive")
        != _cc_catalog.ZINPUT_JOYSTICK_INIT_CALLER_END_EXCLUSIVE
        or caller_symbol.get("binary") != "recoil"
        or caller_symbol.get("kind") != "function"
        or caller_symbol.get("pipeline_class") != "authored"
        or caller_symbol.get("authored_order_role") != "authored-body"
        or caller_symbol.get("ownership_state") != "primary-owned"
        or caller_symbol.get("extent_state") != "known"
        or caller_symbol.get("size") != 0x120
        or caller_symbol.get("navigation_name")
        != "zInput::DIInitJoystickDevice"
        or caller_identity in indexes.provider_ids
        or indexes.by_address.get(_cc_catalog.ZINPUT_JOYSTICK_INIT_CALLER_START)
        != caller_identity
        or indexes.by_candidate_name.get(_cc_catalog.ZINPUT_JOYSTICK_INIT_CALLER_SYMBOL)
        != caller_identity
        or not isinstance(caller_trace, Mapping)
        or caller_trace.get("state") != "resolved"
        or caller_trace.get("reason_code") not in {None, ""}
        or not isinstance(caller_edges, list)
        or len(caller_edges) != 1
        or caller_edges[0].get("relation") != "defines"
        or caller_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.ZINPUT_JOYSTICK_SOURCE_PATH}
        or not exact_authored_data(
            aggregate_symbol,
            address=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS,
            name=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_NAME,
            contribution_id=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_STORAGE_ID,
        )
        or not exact_authored_data(
            leaf_symbol,
            address=_cc_catalog.ZINPUT_JOYSTICK_DEVICE_ADDRESS,
            name=_cc_catalog.ZINPUT_JOYSTICK_DEVICE_NAME,
            contribution_id=_cc_catalog.ZINPUT_JOYSTICK_DEVICE_STORAGE_ID,
        )
        or not exact_storage_contribution(
            aggregate_contribution,
            symbol_id=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_SYMBOL_ID,
            address=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS,
        )
        or not exact_storage_contribution(
            leaf_contribution,
            symbol_id=_cc_catalog.ZINPUT_JOYSTICK_DEVICE_SYMBOL_ID,
            address=_cc_catalog.ZINPUT_JOYSTICK_DEVICE_ADDRESS,
        )
        or not isinstance(owner, Mapping)
        or owner.get("binary") != "recoil"
        or owner.get("kind") != "subsystem"
        or owner.get("provider_state") != "pending"
        or matching_primary_data != exact_primary_data
        or indexes.storage_by_address.get(
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS
        )
        != aggregate_identity
        or indexes.storage_by_address.get(_cc_catalog.ZINPUT_JOYSTICK_DEVICE_ADDRESS)
        != leaf_identity
        or indexes.storage_by_name.get(_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_NAME)
        != aggregate_identity
        or indexes.storage_by_name.get(_cc_catalog.ZINPUT_JOYSTICK_DEVICE_NAME)
        != leaf_identity
        or [
            address
            for address, identity in indexes.storage_by_address.items()
            if identity == aggregate_identity
        ]
        != [_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS]
        or [
            address
            for address, identity in indexes.storage_by_address.items()
            if identity == leaf_identity
        ]
        != [_cc_catalog.ZINPUT_JOYSTICK_DEVICE_ADDRESS]
        or aggregate_identity in indexes.provider_ids
        or leaf_identity in indexes.provider_ids
        or indexes.reviewed_logical_aliases_by_address.get(
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS
        )
        or indexes.reviewed_logical_aliases_by_address.get(
            _cc_catalog.ZINPUT_JOYSTICK_DEVICE_ADDRESS
        )
    ):
        raise ValueError(
            "zInput joystick aggregate-leaf bridge requires unique authored "
            "non-provider caller, aggregate, leaf, storage, and owner provenance"
        )

    target = candidate.target
    definition = candidate.caller_definition
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.ZINPUT_JOYSTICK_INIT_CALLER_START
    ]
    if (
        definition is None
        or definition.symbol != _cc_catalog.ZINPUT_JOYSTICK_INIT_CALLER_SYMBOL
        or len(definition.data) != 0x120
        or len(definition.relocation_mask) != len(definition.data)
    ):
        raise ValueError(
            "zInput joystick aggregate-leaf bridge requires the exact "
            "caller procedure and 0x120-byte definition"
        )
    if (
        target is None
        or getattr(target, "name", "") != _cc_catalog.ZINPUT_JOYSTICK_TARGET_NAME
        or getattr(target, "target_binary", "") != "recoil"
        or Path(str(getattr(target, "manifest_path", ""))).resolve()
        != _cc_catalog.ZINPUT_JOYSTICK_TARGET_MANIFEST.resolve()
        or not bool(getattr(target, "check_translation_unit_function_order", False))
        or tuple(getattr(target, "source_files", ()))
        not in {(), (_cc_catalog.ZINPUT_JOYSTICK_SOURCE_PATH,)}
    ):
        raise ValueError(
            "zInput joystick aggregate-leaf bridge requires the exact "
            "registered target and source listing: "
            f"name={getattr(target, 'name', '')!r}, "
            f"binary={getattr(target, 'target_binary', '')!r}, "
            f"manifest={str(getattr(target, 'manifest_path', ''))!r}, "
            f"tu_order={getattr(target, 'check_translation_unit_function_order', None)!r}, "
            f"source_files={tuple(getattr(target, 'source_files', ()))!r}"
        )
    if len(contribution_rows) != 1:
        raise ValueError(
            "zInput joystick aggregate-leaf bridge requires one exact "
            "registered caller contribution"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.ZINPUT_JOYSTICK_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "")
        != _cc_catalog.ZINPUT_JOYSTICK_INIT_CALLER_SYMBOL
        or getattr(contribution_row, "symbol_regex", None) is not None
        or getattr(contribution_row, "name", "")
        != "zInput::DIInitJoystickDevice"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "zInput joystick aggregate-leaf bridge requires the exact "
            "authored zin_joystick.cpp contribution row"
        )

    expected_storage = f"load({leaf_identity})"
    expected_rows = [
        row
        for row in expected
        if row.get("identity_kind") == "virtual-slot"
        and row.get("storage_identity") == expected_storage
        and row.get("slot_displacement")
        == _cc_catalog.ZINPUT_JOYSTICK_DEVICE_VIRTUAL_SLOT
    ]
    if (
        len(expected_rows) != 1
        or any(
            expected_rows[0].get(key) != value
            for key, value in {
                "form": "call",
                "dispatch": "indirect",
                "target_identity": "",
                "cleanup_bytes": None,
            }.items()
        )
    ):
        raise ValueError(
            "zInput joystick aggregate-leaf bridge requires one exact "
            "immutable-retail virtual-slot contract"
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    counts: dict[int, int] = {}
    for offset in offsets:
        if offset is not None:
            counts[offset] = counts.get(offset, 0) + 1
    index_by_offset = {
        offset: index
        for index, offset in enumerate(offsets)
        if offset is not None and counts.get(offset) == 1
    }
    instruction_by_offset = {
        offset: candidate.instructions[index]
        for offset, index in index_by_offset.items()
    }
    aggregate_pattern = re.escape(
        _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL
    )
    exact_rows = {
        0x31: (
            b"\x8b\x35\x20\x3f\x00\x00",
            rf"mov\s+esi\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            rf"{aggregate_pattern}\+16160",
        ),
        0x37: (b"\x85\xf6", r"test\s+esi\s*,\s*esi"),
        0x39: (b"\x75\x08", r"jne\s+(?:SHORT\s+)?\$L[0-9]+"),
        0x43: (
            b"\xc7\x44\x24\x08\x2c\x00\x00\x00",
            r"mov\s+(?:dword\s+(?:ptr\s+)?)?_caps\$\[esp\+132\]\s*,\s*44",
        ),
        0x4B: (
            b"\x8b\x16",
            r"mov\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[esi\]",
        ),
        0x4D: (
            b"\x68\x00\x00\x00\x00",
            r"push\s+OFFSET\s+FLAT:_c_dfDIJoystick",
        ),
        0x52: (b"\x56", r"push\s+esi"),
        0x53: (
            b"\xff\x52\x2c",
            r"call\s+(?:dword\s+(?:ptr\s+)?)?\[edx\+44\]",
        ),
        0x56: (
            b"\x8b\x06",
            r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[esi\]",
        ),
    }
    if set(exact_rows) - set(instruction_by_offset):
        raise ValueError(
            "zInput joystick aggregate-leaf bridge requires one unique "
            "instruction at every reviewed offset"
        )
    for offset, (body, pattern) in exact_rows.items():
        instruction = instruction_by_offset[offset]
        if (
            bytes(int(value, 16) for value in instruction.bytes) != body
            or definition.data[offset : offset + len(body)] != body
            or re.fullmatch(
                pattern,
                instruction.raw_text.strip(),
                flags=re.IGNORECASE,
            )
            is None
        ):
            raise ValueError(
                "zInput joystick aggregate-leaf bridge requires exact "
                f"instruction bytes/registers/operands at +{hex(offset)}"
            )

    load_anchors = [
        offset
        for offset, instruction in instruction_by_offset.items()
        if re.fullmatch(
            exact_rows[0x31][1],
            instruction.raw_text.strip(),
            flags=re.IGNORECASE,
        )
        is not None
    ]
    if load_anchors != [0x31]:
        raise ValueError(
            "zInput joystick aggregate-leaf bridge rejects duplicate or "
            "shifted aggregate-leaf load anchors"
        )

    for relocation_offset, symbol, addend in (
        (
            0x33,
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL,
            _cc_catalog.ZINPUT_JOYSTICK_DEVICE_DISPLACEMENT,
        ),
        (0x4E, "_c_dfDIJoystick", 0),
    ):
        relocations = [
            row
            for row in definition.relocations
            if row.offset == relocation_offset
        ]
        if (
            len(relocations) != 1
            or relocations[0].type != IMAGE_REL_I386_DIR32
            or relocations[0].symbol_name != symbol
            or struct.unpack_from("<I", definition.data, relocation_offset)[0]
            != addend
            or not all(
                definition.relocation_mask[index]
                for index in range(relocation_offset, relocation_offset + 4)
            )
        ):
            raise ValueError(
                "zInput joystick aggregate-leaf bridge requires exact "
                "candidate DIR32 relocation targets, addends, and masks"
            )
    if (
        definition.undefined_external_data.count(
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL
        )
        != 1
        or _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL
        in definition.defined_external_data
    ):
        raise ValueError(
            "zInput joystick aggregate-leaf bridge requires one exact "
            "undefined aggregate data symbol"
        )

    start = address_value(normalized_start)
    end = address_value(caller_end_exclusive)
    runtime_addresses = _cc_cfg._instruction_runtime_addresses(
        candidate.instructions,
        source="cod",
        caller_start=start,
    )
    runtime_index_by_address = {
        address: index
        for index, address in enumerate(runtime_addresses)
        if address is not None
    }
    branch_index = index_by_offset[0x39]
    branch = _cc_cfg._exact_local_direct_branch(
        candidate.instructions[branch_index],
        instruction_index=branch_index,
        instruction_addresses=runtime_addresses,
        instruction_index_by_address=runtime_index_by_address,
        source="cod",
        caller_start=start,
        caller_end=end,
    )
    load_index = index_by_offset[0x31]
    branch_target_index = index_by_offset[0x43]
    vptr_index = index_by_offset[0x4B]
    call_index = index_by_offset[0x53]
    if (
        branch is None
        or branch[0] != "conditional"
        or offsets[branch[1]] != 0x43
        or any(
            _cc_cfg._instruction_mnemonic(candidate.instructions[index])
            in {"call", "jmp"}
            for index in range(load_index, call_index)
        )
        or any(
            _cc_cfg._instruction_mnemonic(candidate.instructions[index]).startswith("j")
            for index in range(load_index, call_index)
            if index != branch_index
        )
        or any(
            _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index], "esi"
            )
            for index in (
                *range(load_index + 1, branch_index + 1),
                *range(branch_target_index, vptr_index),
            )
        )
        or any(
            _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index], register
            )
            for index in range(vptr_index + 1, call_index)
            for register in ("esi", "edx")
        )
        or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
        or any(definition.relocation_mask[0x53:0x56])
    ):
        raise ValueError(
            "zInput joystick aggregate-leaf bridge rejects alternate CFG, "
            "calls, cleanup, register clobbers, or relocated call bytes"
        )

    expression = (
        f"{_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL}"
        f"+{_cc_catalog.ZINPUT_JOYSTICK_DEVICE_DISPLACEMENT}"
    )
    return (
        {
            expression: ReviewedStaticStorageReferenceBridge(
                aggregate_symbol=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL,
                displacement=_cc_catalog.ZINPUT_JOYSTICK_DEVICE_DISPLACEMENT,
                access_width=4,
                storage_identity=leaf_identity,
            )
        },
        {
            "0x4b": ReviewedMemberVptrStorageBridge(
                register="edx",
                source_register="esi",
                source_provenance=leaf_identity,
                receiver_register="esi",
                receiver_provenance=leaf_identity,
                storage_identity=expected_storage,
                slot_displacement=_cc_catalog.ZINPUT_JOYSTICK_DEVICE_VIRTUAL_SLOT,
                call_address="0x53",
            )
        },
    )


def _zinput_exact_candidate_switch_successors(
    candidate: CandidateAssembly,
    switch: Mapping[str, Any] | None,
    *,
    label: str,
) -> tuple[dict[int, tuple[int, ...]], frozenset[int]]:
    """Prove the two finite keyboard classifier/jump-table CFG rows."""
    if switch is None:
        return {}, frozenset()
    definition = candidate.caller_definition
    if definition is None:
        raise ValueError(f"{label} switch proof requires candidate COFF")
    classifier = bytes(
        switch.get("classifier", _cc_catalog.ZINPUT_KEYBOARD_SWITCH_CLASSIFIER)
    )
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    counts = Counter(offset for offset in offsets if offset is not None)
    index_by_offset = {
        int(offset): index
        for index, offset in enumerate(offsets)
        if offset is not None and counts[offset] == 1
    }
    rows = tuple(switch["rows"])
    if any(offset not in index_by_offset for offset, _body, _pattern in rows):
        raise ValueError(
            f"{label} switch proof requires every exact COD row"
        )
    for offset, body, pattern in rows:
        instruction = candidate.instructions[index_by_offset[offset]]
        actual = bytes(int(item, 16) for item in instruction.bytes)
        row_mask = tuple(
            definition.relocation_mask[offset : offset + len(body)]
        )
        expected_mask = tuple(
            index >= len(body) - 4
            for index in range(len(body))
        ) if offset in {
            switch["classifier_load_relocation"][0] - 2,
            switch["dispatch_offset"],
        } else (False,) * len(body)
        if (
            actual != body
            or definition.data[offset : offset + len(body)] != body
            or row_mask != expected_mask
            or re.fullmatch(
                pattern,
                instruction.raw_text.strip(),
                flags=re.IGNORECASE,
            ) is None
        ):
            raise ValueError(
                f"{label} switch COD/COFF row drifted at +{hex(offset)}"
            )

    classifier_offset = int(switch["classifier_offset"])
    classifier_end = classifier_offset + len(classifier)
    table_offset = int(switch["table_offset"])
    table_targets = tuple(switch["table_targets"])
    table_end = table_offset + len(table_targets) * 4
    if (
        definition.data[classifier_offset:classifier_end]
        != classifier
        or any(definition.relocation_mask[classifier_offset:classifier_end])
        or definition.data[table_offset:table_end]
        != b"\0" * (table_end - table_offset)
        or not all(definition.relocation_mask[table_offset:table_end])
        or any(target not in index_by_offset for _name, target in table_targets)
    ):
        raise ValueError(
            f"{label} switch classifier/table body or target drifted"
        )

    classifier_indices = tuple(
        index_by_offset.get(classifier_offset + offset, -1)
        for offset in range(len(classifier))
    )
    table_indices = tuple(
        index_by_offset.get(table_offset + index * 4, -1)
        for index in range(len(table_targets))
    )
    relational_labels = bool(switch.get("relational_labels"))
    load_relocation = tuple(switch["classifier_load_relocation"])
    dispatch_relocation = tuple(switch["dispatch_relocation"])
    relevant_offsets = {
        int(load_relocation[0]),
        int(dispatch_relocation[0]),
        *(
            table_offset + index * 4
            for index in range(len(table_targets))
        ),
    }
    relevant_relocations = tuple(
        relocation
        for relocation in definition.relocations
        if relocation.offset in relevant_offsets
    )
    relocation_by_offset = {
        relocation.offset: relocation for relocation in relevant_relocations
    }
    if relational_labels:
        expected_value_by_relocation_offset = {
            int(load_relocation[0]): classifier_offset,
            int(dispatch_relocation[0]): table_offset,
            **{
                table_offset + index * 4: int(target_offset)
                for index, (_role, target_offset) in enumerate(table_targets)
            },
        }
        if (
            len(relevant_relocations) != len(relevant_offsets)
            or set(relocation_by_offset) != relevant_offsets
            or any(
                relocation.type != IMAGE_REL_I386_DIR32
                or re.fullmatch(r"\$L[0-9]+", relocation.symbol_name) is None
                or relocation.offset + 4 > len(definition.data)
                or struct.unpack_from(
                    "<I", definition.data, relocation.offset
                )[0]
                != 0
                for relocation in relevant_relocations
            )
        ):
            raise ValueError(
                f"{label} switch relational relocation population drifted"
            )
        referenced_names = {
            relocation.symbol_name for relocation in relevant_relocations
        }
        symbols_by_name = {
            name: tuple(
                symbol
                for symbol in definition.coff_symbols
                if symbol.name == name
            )
            for name in referenced_names
        }
        if any(len(symbols) != 1 for symbols in symbols_by_name.values()):
            raise ValueError(
                f"{label} switch relational label definition population drifted"
            )
        for relocation_offset, expected_value in (
            expected_value_by_relocation_offset.items()
        ):
            relocation = relocation_by_offset[relocation_offset]
            symbol = symbols_by_name[relocation.symbol_name][0]
            if (
                relocation.symbol_index != symbol.index
                or symbol.value != expected_value
                or symbol.section_number != definition.section_index
                or symbol.symbol_type != 0
                or symbol.storage_class != 6
                or symbol.aux_count != 0
            ):
                raise ValueError(
                    f"{label} switch relational label role drifted at "
                    f"+{hex(relocation_offset)}"
                )
        relational_rows = tuple(
            (
                expected_value_by_relocation_offset[offset],
                relocation_by_offset[offset].symbol_name,
            )
            for offset in sorted(relevant_offsets)
        )
        if any(
            (left_value == right_value) != (left_name == right_name)
            for index, (left_value, left_name) in enumerate(relational_rows)
            for right_value, right_name in relational_rows[index + 1 :]
        ):
            raise ValueError(
                f"{label} switch relational label alias partition drifted"
            )
        row_labels = []
        for row_offset, _body, pattern in rows:
            if r"\$L" not in pattern:
                continue
            instruction = candidate.instructions[index_by_offset[row_offset]]
            labels = re.findall(
                r"\$L[0-9]+", instruction.raw_text, flags=re.IGNORECASE
            )
            if len(labels) != 1:
                raise ValueError(
                    f"{label} switch relational COD label role drifted at "
                    f"+{hex(row_offset)}"
                )
            row_labels.append(labels[0])
        expected_row_labels = (
            relocation_by_offset[
                table_offset + (len(table_targets) - 1) * 4
            ].symbol_name,
            relocation_by_offset[int(load_relocation[0])].symbol_name,
            relocation_by_offset[int(dispatch_relocation[0])].symbol_name,
        )
        if tuple(row_labels) != expected_row_labels:
            raise ValueError(
                f"{label} switch relational COD label roles drifted"
            )
        expected_table_names = tuple(
            relocation_by_offset[table_offset + index * 4].symbol_name
            for index in range(len(table_targets))
        )
    else:
        expected_table_names = tuple(
            target_name for target_name, _target_offset in table_targets
        )
    if (
        any(index < 0 for index in (*classifier_indices, *table_indices))
        or any(
            tuple(candidate.instructions[instruction_index].bytes)
            != (f"{classifier[offset]:02x}",)
            or candidate.instructions[instruction_index].raw_text.strip()
            != f"DB {classifier[offset]}"
            for offset, instruction_index in enumerate(classifier_indices)
        )
        or any(
            tuple(candidate.instructions[instruction_index].bytes)
            != ("00", "00", "00", "00", "dd")
            or candidate.instructions[instruction_index].raw_text.strip()
            != expected_name
            for instruction_index, expected_name
            in zip(table_indices, expected_table_names)
        )
    ):
        raise ValueError(
            f"{label} switch parsed DD/DB row population drifted"
        )

    expected_relocations = (
        (load_relocation[0], IMAGE_REL_I386_DIR32, load_relocation[1], 0),
        (
            dispatch_relocation[0],
            IMAGE_REL_I386_DIR32,
            dispatch_relocation[1],
            0,
        ),
        *(
            (
                table_offset + index * 4,
                IMAGE_REL_I386_DIR32,
                target_name,
                0,
            )
            for index, (target_name, _target_offset) in enumerate(
                table_targets
            )
        ),
    )
    actual_relocations = tuple(
        (
            relocation.offset,
            relocation.type,
            relocation.symbol_name,
            struct.unpack_from("<I", definition.data, relocation.offset)[0],
        )
        for relocation in definition.relocations
        if relocation.offset in relevant_offsets
        and relocation.offset + 4 <= len(definition.data)
    )
    if (
        (not relational_labels and actual_relocations != expected_relocations)
        or any(
            classifier_offset <= relocation.offset < classifier_end
            for relocation in definition.relocations
        )
    ):
        raise ValueError(
            f"{label} switch relocation population drifted"
        )

    dispatch_index = index_by_offset[int(switch["dispatch_offset"])]
    # This is the ordered case map shared with the generic COFF proof, not
    # a set of CFG successors. Repeated case destinations remain evidence.
    target_indices = tuple(
        index_by_offset[target_offset]
        for _target_name, target_offset in table_targets
    )
    attached_data_indices = set((*classifier_indices, *table_indices))
    padding = switch.get("padding")
    if padding is not None:
        padding_offset, padding_body, padding_pattern = padding
        padding_index = index_by_offset.get(int(padding_offset), -1)
        if padding_index < 0:
            raise ValueError(f"{label} switch padding row is missing")
        padding_instruction = candidate.instructions[padding_index]
        if (
            bytes(int(item, 16) for item in padding_instruction.bytes)
            != padding_body
            or definition.data[
                padding_offset : padding_offset + len(padding_body)
            ]
            != padding_body
            or any(
                definition.relocation_mask[
                    padding_offset : padding_offset + len(padding_body)
                ]
            )
            or re.fullmatch(
                padding_pattern,
                padding_instruction.raw_text.strip(),
                flags=re.IGNORECASE,
            ) is None
        ):
            raise ValueError(f"{label} switch padding row drifted")
        attached_data_indices.add(padding_index)
    return (
        {dispatch_index: target_indices},
        frozenset(attached_data_indices),
    )


def _zinput_runtime_candidate_switch_extraction_package(
    candidate: CandidateAssembly,
    *,
    caller_start: str,
) -> tuple[CandidateAssembly, frozenset[int]]:
    """Compose only the two proved keyboard switches for every extractor."""
    normalized_start = normalize_address(caller_start)
    spec = _cc_catalog.ZINPUT_RUNTIME_DISPATCH_CANDIDATE_SPECS.get(normalized_start)
    if spec is None:
        return candidate, frozenset()
    switch = spec["switch"]
    if switch is None:
        if (
            normalized_start == "0x46fa10"
            and (
                candidate.local_control_flow_indices
                or candidate.local_control_flow_targets
            )
        ):
            raise ValueError(
                "zInput keyboard Wait rejects an unreviewed candidate switch"
            )
        return candidate, frozenset()
    reviewed_targets, _data_indices = (
        _zinput_exact_candidate_switch_successors(
            candidate,
            switch,
            label=(
                "zInput keyboard runtime dispatch " + normalized_start
            ),
        )
    )
    reviewed_indices = frozenset(reviewed_targets)
    overlap = reviewed_indices & candidate.local_control_flow_indices
    if (
        any(
            candidate.local_control_flow_targets.get(index)
            != reviewed_targets[index]
            for index in overlap
        )
        or any(
            index in candidate.local_control_flow_targets
            and index not in candidate.local_control_flow_indices
            for index in reviewed_indices
        )
    ):
        raise ValueError(
            "zInput keyboard reviewed switch conflicts with candidate CFG"
        )
    merged_targets = {
        **candidate.local_control_flow_targets,
        **reviewed_targets,
    }
    composed = replace(
        candidate,
        local_control_flow_indices=(
            candidate.local_control_flow_indices | reviewed_indices
        ),
        local_control_flow_targets=merged_targets,
    )
    if (
        not reviewed_indices
        or not reviewed_indices.issubset(
            composed.local_control_flow_indices
        )
        or any(
            composed.local_control_flow_targets.get(index)
            != targets
            for index, targets in reviewed_targets.items()
        )
    ):
        raise ValueError(
            "zInput keyboard reviewed switch package was not consumed"
        )
    return composed, reviewed_indices


def _zinput_translate_candidate_switch_extraction_package(
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[CandidateAssembly, frozenset[int]]:
    """Publish only TranslateDikToAscii's exact VC5 computed switch."""

    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.ZINPUT_TRANSLATE_CALLER_START:
        return candidate, frozenset()
    if (
        caller_identity != _cc_catalog.ZINPUT_TRANSLATE_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive) != _cc_catalog.ZINPUT_TRANSLATE_CALLER_END
    ):
        raise ValueError(
            "zInput TranslateDikToAscii switch requires exact caller extent"
        )
    symbol = document.collection("symbols").get(
        _cc_catalog.ZINPUT_TRANSLATE_CALLER_IDENTITY.removeprefix("symbol:")
    )
    trace = symbol.get("source_traceability") if isinstance(symbol, Mapping) else None
    edges = trace.get("source_edges") if isinstance(trace, Mapping) else None
    owner = document.collection("owners").get(_cc_catalog.ZINPUT_TRANSLATE_OWNER_ID)
    owner_relationships = owner.get("relationships") if isinstance(owner, Mapping) else None
    candidate_names = {
        name for name, identity in indexes.by_candidate_name.items()
        if identity == caller_identity
    }
    if (
        not isinstance(symbol, Mapping)
        or symbol.get("address") != _cc_catalog.ZINPUT_TRANSLATE_CALLER_START
        or symbol.get("end_exclusive") != _cc_catalog.ZINPUT_TRANSLATE_CALLER_END
        or symbol.get("binary") != "recoil"
        or symbol.get("kind") != "function"
        or symbol.get("pipeline_class") != "authored"
        or symbol.get("authored_order_role") != "authored-body"
        or symbol.get("ownership_state") != "primary-owned"
        or symbol.get("extent_state") != "known"
        or symbol.get("size") != 0x180
        or symbol.get("navigation_name") != _cc_catalog.ZINPUT_TRANSLATE_CALLER_NAME
        or symbol.get("output_section_id") != "recoil:section:.text"
        or symbol.get("physical_block_id") != "recoil:block:0x46f300"
        or not isinstance(trace, Mapping)
        or trace.get("state") != "resolved"
        or trace.get("reason_code") not in {None, ""}
        or not isinstance(edges, list)
        or len(edges) != 1
        or edges[0].get("relation") != "defines"
        or edges[0].get("anchor_id") != _cc_catalog.ZINPUT_TRANSLATE_CALLER_ANCHOR
        or edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.ZINPUT_KEYBOARD_SOURCE_PATH}
        or not isinstance(owner, Mapping)
        or owner.get("binary") != "recoil"
        or owner.get("kind") != "source-file"
        or owner.get("provider_state") != "pending"
        or [
            row for row in (owner_relationships or ())
            if isinstance(row, Mapping)
            and row.get("kind") == "primary-function"
            and row.get("symbol_id") == "recoil:function:0x46fba0"
        ]
        != [{
            "address": "0x46fba0", "kind": "primary-function",
            "symbol_id": "recoil:function:0x46fba0",
        }]
        or indexes.by_address.get(_cc_catalog.ZINPUT_TRANSLATE_CALLER_START)
        != caller_identity
        or [address for address, identity in indexes.by_address.items()
            if identity == caller_identity] != [_cc_catalog.ZINPUT_TRANSLATE_CALLER_START]
        or candidate_names != {_cc_catalog.ZINPUT_TRANSLATE_CALLER_SYMBOL}
        or caller_identity in indexes.provider_ids
        or indexes.reviewed_logical_aliases_by_address.get(
            _cc_catalog.ZINPUT_TRANSLATE_CALLER_START
        )
    ):
        raise ValueError(
            "zInput TranslateDikToAscii switch requires exact authored "
            "source, owner, alias, and identity provenance"
        )
    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.ZINPUT_TRANSLATE_CALLER_START
    ]
    if (
        target is None
        or getattr(target, "name", "") != _cc_catalog.ZINPUT_KEYBOARD_TARGET_NAME
        or getattr(target, "target_binary", "") != "recoil"
        or Path(str(getattr(target, "manifest_path", ""))).resolve()
        != _cc_catalog.ZINPUT_KEYBOARD_TARGET_MANIFEST.resolve()
        or not bool(getattr(target, "check_translation_unit_function_order", False))
        or tuple(getattr(target, "source_files", ()))
        not in {(), (_cc_catalog.ZINPUT_KEYBOARD_SOURCE_PATH,)}
        or len(contribution_rows) != 1
    ):
        raise ValueError(
            "zInput TranslateDikToAscii switch requires exact target/source"
        )
    contribution, row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "") != _cc_catalog.ZINPUT_KEYBOARD_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(row, "symbol", "") != _cc_catalog.ZINPUT_TRANSLATE_CALLER_SYMBOL
        or getattr(row, "symbol_regex", None) is not None
        or getattr(row, "name", "") != _cc_catalog.ZINPUT_TRANSLATE_CALLER_NAME
        or getattr(row, "pipeline_class", "") != "authored"
        or getattr(row, "authored_order_role", "") != "authored-body"
        or not bool(getattr(row, "required_presence", False))
        or not bool(getattr(row, "full_order_gate", False))
    ):
        raise ValueError(
            "zInput TranslateDikToAscii switch contribution row drifted"
        )
    definition = candidate.caller_definition
    expected_mask = tuple(
        any(offset <= index < offset + 4 for offset, _type, _symbol, _addend
            in _cc_catalog.ZINPUT_TRANSLATE_RELOCATIONS)
        for index in range(0x180)
    )
    actual_relocations = tuple(
        (relocation.offset, relocation.type, relocation.symbol_name,
         struct.unpack_from("<I", definition.data, relocation.offset)[0])
        for relocation in definition.relocations
    ) if definition is not None else ()
    expected_relocation_shape = tuple(
        (offset, relocation_type, addend)
        for offset, relocation_type, _symbol, addend
        in _cc_catalog.ZINPUT_TRANSLATE_RELOCATIONS
    )
    actual_relocation_shape = tuple(
        (offset, relocation_type, addend)
        for offset, relocation_type, _symbol, addend
        in actual_relocations
    )
    if (
        definition is None
        or definition.symbol != _cc_catalog.ZINPUT_TRANSLATE_CALLER_SYMBOL
        or definition.section_index != 23
        or definition.section_start != 0
        or definition.section_end != 0x180
        or definition.data != _cc_catalog.ZINPUT_TRANSLATE_CANDIDATE_BODY
        or tuple(definition.relocation_mask) != expected_mask
        or actual_relocation_shape != expected_relocation_shape
        or actual_relocations[:4] != _cc_catalog.ZINPUT_TRANSLATE_RELOCATIONS[:4]
        or any(
            re.fullmatch(r"\$L[0-9]+", symbol) is None
            for _offset, _type, symbol, _addend
            in actual_relocations[4:]
        )
    ):
        raise ValueError(
            "zInput TranslateDikToAscii switch candidate body/section/"
            "relocation population drifted"
        )
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    code_rows = [
        (offset, instruction) for offset, instruction
        in zip(offsets, candidate.instructions)
        if offset is not None and offset < 0xF0
    ]
    if tuple(offset for offset, _instruction in code_rows) != _cc_catalog.ZINPUT_TRANSLATE_CODE_OFFSETS:
        raise ValueError(
            "zInput TranslateDikToAscii switch instruction population drifted"
        )
    covered: set[int] = set()
    for offset, instruction in code_rows:
        body = bytes(int(item, 16) for item in instruction.bytes)
        if definition.data[offset:offset + len(body)] != body:
            raise ValueError(
                "zInput TranslateDikToAscii switch instruction bytes drifted"
            )
        covered.update(range(offset, offset + len(body)))
    if covered != set(range(0xF0)):
        raise ValueError(
            "zInput TranslateDikToAscii switch instruction extent drifted"
        )
    reviewed_targets, data_indices = _zinput_exact_candidate_switch_successors(
        candidate, _cc_catalog.ZINPUT_TRANSLATE_SWITCH,
        label="zInput TranslateDikToAscii",
    )
    reviewed_indices = frozenset(reviewed_targets)
    overlap = reviewed_indices & candidate.local_control_flow_indices
    if any(candidate.local_control_flow_targets.get(index) != reviewed_targets[index]
           for index in overlap):
        raise ValueError(
            "zInput TranslateDikToAscii switch conflicts with candidate CFG"
        )
    composed = replace(
        candidate,
        local_control_flow_indices=candidate.local_control_flow_indices | reviewed_indices,
        local_control_flow_targets={**candidate.local_control_flow_targets, **reviewed_targets},
    )
    invocation_offsets = tuple(
        offsets[index] for index in _cc_callable_identity._candidate_static_invocation_indices(
            composed,
            caller_start=_cc_catalog.ZINPUT_TRANSLATE_CALLER_START,
            caller_end_exclusive=_cc_catalog.ZINPUT_TRANSLATE_CALLER_END,
        )
    )
    if invocation_offsets != (0x0C,) or not reviewed_indices:
        raise ValueError(
            "zInput TranslateDikToAscii switch invocation population drifted"
        )
    start = address_value(_cc_catalog.ZINPUT_TRANSLATE_CALLER_START)
    addresses = _cc_cfg._instruction_runtime_addresses(
        composed.instructions, source="cod", caller_start=start
    )
    by_address = {address: index for index, address in enumerate(addresses)
                  if address is not None}
    successors, unresolved = _cc_cfg._exact_invocation_cfg(
        composed.instructions,
        instruction_addresses=addresses,
        instruction_index_by_address=by_address,
        source="cod", caller_start=start,
        caller_end=address_value(_cc_catalog.ZINPUT_TRANSLATE_CALLER_END),
        local_control_flow_indices=composed.local_control_flow_indices,
        local_control_flow_targets=composed.local_control_flow_targets,
    )
    if (
        unresolved - data_indices
        or _cc_cfg._reachable_cfg_indices(successors, (0,))
        != frozenset(range(len(composed.instructions))) - data_indices
    ):
        raise ValueError(
            "zInput TranslateDikToAscii switch reachable CFG drifted"
        )
    return composed, reviewed_indices


def _zinput_exact_aggregate_leaf_member_vptr_candidate_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    label: str,
    expected_caller_identity: str,
    expected_caller_start: str,
    expected_caller_end_exclusive: str,
    expected_caller_symbol: str,
    expected_caller_candidate_aliases: tuple[str, ...] = (),
    expected_caller_name: str,
    expected_caller_anchor_id: str,
    expected_caller_size: int,
    expected_caller_section_index: int,
    expected_physical_block_id: str,
    expected_target_name: str,
    expected_target_manifest: Path,
    expected_source_path: str,
    leaf_symbol_id: str,
    leaf_storage_id: str,
    leaf_address: str,
    leaf_name: str,
    leaf_displacement: int,
    aggregate_leaf_relocation_offsets: tuple[int, ...],
    invocation_offsets: tuple[int, ...],
    leaf_chains: tuple[tuple[int, int, int, int, int, str, int], ...],
    expected_retail_size: int | None = None,
    complete_relocation_specs: (
        tuple[tuple[int, int, str, int], ...] | None
    ) = None,
    expected_body: bytes | None = None,
    expected_additional_rows: tuple[
        tuple[int, tuple[tuple[str, Any], ...]], ...
    ] = (),
    expected_branch_successors: tuple[
        tuple[int, tuple[int, ...]], ...
    ] = (),
    expected_switch: Mapping[str, Any] | None = None,
) -> tuple[
    dict[str, ReviewedStaticStorageReferenceBridge],
    dict[str, ReviewedMemberVptrStorageBridge],
]:
    """Prove one finite aggregate member without widening container lookup."""
    from _recoil.call_contract.records import (
        ReviewedMemberVptrStorageBridge,
        ReviewedStaticStorageReferenceBridge,
    )
    normalized_start = normalize_address(caller_start)
    normalized_end = normalize_address(caller_end_exclusive)
    if (
        caller_identity != expected_caller_identity
        or normalized_start != expected_caller_start
        or normalized_end != expected_caller_end_exclusive
    ):
        raise ValueError(
            f"{label} aggregate-leaf bridge requires the exact reviewed "
            "caller identity and extent"
        )

    caller_symbol_id = caller_identity.removeprefix("symbol:")
    symbols = document.collection("symbols")
    contributions = document.collection("storage_contributions")
    owners = document.collection("owners")
    caller_symbol = symbols.get(caller_symbol_id)
    aggregate_symbol = symbols.get(_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_SYMBOL_ID)
    leaf_symbol = symbols.get(leaf_symbol_id)
    aggregate_contribution = contributions.get(
        _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_STORAGE_ID
    )
    leaf_contribution = contributions.get(leaf_storage_id)
    owner = owners.get(_cc_catalog.ZINPUT_JOYSTICK_STORAGE_OWNER_ID)
    caller_trace = (
        caller_symbol.get("source_traceability")
        if isinstance(caller_symbol, Mapping)
        else None
    )
    caller_edges = (
        caller_trace.get("source_edges")
        if isinstance(caller_trace, Mapping)
        else None
    )
    owner_relationships = (
        owner.get("relationships") if isinstance(owner, Mapping) else None
    )
    expected_candidate_names = {
        expected_caller_symbol,
        *expected_caller_candidate_aliases,
    }
    actual_candidate_names = {
        name
        for name, identity in indexes.by_candidate_name.items()
        if identity == caller_identity
    }

    def exact_authored_data(
        row: Any,
        *,
        address: str,
        name: str,
        contribution_id: str,
    ) -> bool:
        return (
            isinstance(row, Mapping)
            and row.get("address") == address
            and row.get("binary") == "recoil"
            and row.get("kind") == "data"
            and row.get("disposition") == "authored"
            and row.get("navigation_name") == name
            and row.get("output_section_id") == "recoil:section:.data"
            and row.get("storage_contribution_ids") == [contribution_id]
            and not row.get("logical_aliases")
        )

    def exact_storage_contribution(
        row: Any,
        *,
        symbol_id: str,
        address: str,
    ) -> bool:
        reference = row.get("reference") if isinstance(row, Mapping) else None
        return (
            isinstance(row, Mapping)
            and row.get("binary") == "recoil"
            and row.get("kind") == "data-symbol"
            and row.get("output_section_id") == "recoil:section:.data"
            and row.get("overlap") == "none"
            and row.get("owner_ids") == [_cc_catalog.ZINPUT_JOYSTICK_STORAGE_OWNER_ID]
            and row.get("parent_contribution_id") is None
            and row.get("symbol_ids") == [symbol_id]
            and isinstance(reference, Mapping)
            and reference.get("address") == address
        )

    aggregate_identity = f"storage:{_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_SYMBOL_ID}"
    leaf_identity = f"storage:{leaf_symbol_id}"
    exact_primary_data = {
        (
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS,
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_NAME,
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_SYMBOL_ID,
        ),
        (leaf_address, leaf_name, leaf_symbol_id),
    }
    matching_primary_data = {
        (
            str(row.get("address", "")),
            str(row.get("name", "")),
            str(row.get("symbol_id", "")),
        )
        for row in (owner_relationships or ())
        if isinstance(row, Mapping)
        and row.get("kind") == "primary-data"
        and row.get("symbol_id")
        in {_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_SYMBOL_ID, leaf_symbol_id}
    }
    if (
        (
            expected_caller_candidate_aliases
            and (
                normalized_start != "0x46f450"
                or expected_caller_candidate_aliases != (
                    "?KeyboardResetTransitionState@zInput@@YIXXZ",
                )
                or not _cc_abi._is_exact_zeroarg_cdecl_fastcall_pair(
                    expected_caller_symbol,
                    expected_caller_candidate_aliases[0],
                )
            )
        )
        or len(expected_candidate_names)
        != 1 + len(expected_caller_candidate_aliases)
        or not isinstance(caller_symbol, Mapping)
        or caller_symbol.get("address") != expected_caller_start
        or caller_symbol.get("end_exclusive") != expected_caller_end_exclusive
        or caller_symbol.get("binary") != "recoil"
        or caller_symbol.get("kind") != "function"
        or caller_symbol.get("pipeline_class") != "authored"
        or caller_symbol.get("authored_order_role") != "authored-body"
        or caller_symbol.get("ownership_state") != "primary-owned"
        or caller_symbol.get("extent_state") != "known"
        or caller_symbol.get("size")
        != (
            expected_retail_size
            if expected_retail_size is not None
            else expected_caller_size
        )
        or caller_symbol.get("navigation_name") != expected_caller_name
        or caller_symbol.get("output_section_id") != "recoil:section:.text"
        or caller_symbol.get("physical_block_id") != expected_physical_block_id
        or caller_identity in indexes.provider_ids
        or indexes.by_address.get(expected_caller_start) != caller_identity
        or indexes.by_candidate_name.get(expected_caller_symbol)
        != caller_identity
        or [
            address
            for address, identity in indexes.by_address.items()
            if identity == caller_identity
        ]
        != [expected_caller_start]
        or actual_candidate_names != expected_candidate_names
        or not isinstance(caller_trace, Mapping)
        or caller_trace.get("state") != "resolved"
        or caller_trace.get("reason_code") not in {None, ""}
        or not isinstance(caller_edges, list)
        or len(caller_edges) != 1
        or not isinstance(caller_edges[0], Mapping)
        or caller_edges[0].get("relation") != "defines"
        or caller_edges[0].get("anchor_id") != expected_caller_anchor_id
        or caller_edges[0].get("emission_context")
        != {"translation_unit": expected_source_path}
        or not exact_authored_data(
            aggregate_symbol,
            address=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS,
            name=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_NAME,
            contribution_id=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_STORAGE_ID,
        )
        or not exact_authored_data(
            leaf_symbol,
            address=leaf_address,
            name=leaf_name,
            contribution_id=leaf_storage_id,
        )
        or not exact_storage_contribution(
            aggregate_contribution,
            symbol_id=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_SYMBOL_ID,
            address=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS,
        )
        or not exact_storage_contribution(
            leaf_contribution,
            symbol_id=leaf_symbol_id,
            address=leaf_address,
        )
        or not isinstance(owner, Mapping)
        or owner.get("binary") != "recoil"
        or owner.get("kind") != "subsystem"
        or owner.get("provider_state") != "pending"
        or matching_primary_data != exact_primary_data
        or address_value(_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS)
        + leaf_displacement
        != address_value(leaf_address)
        or aggregate_identity == leaf_identity
        or indexes.storage_by_address.get(
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS
        )
        != aggregate_identity
        or indexes.storage_by_address.get(leaf_address) != leaf_identity
        or indexes.storage_by_name.get(_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_NAME)
        != aggregate_identity
        or indexes.storage_by_name.get(leaf_name) != leaf_identity
        or [
            address
            for address, identity in indexes.storage_by_address.items()
            if identity == aggregate_identity
        ]
        != [_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS]
        or [
            address
            for address, identity in indexes.storage_by_address.items()
            if identity == leaf_identity
        ]
        != [leaf_address]
        or aggregate_identity in indexes.provider_ids
        or leaf_identity in indexes.provider_ids
        or indexes.reviewed_logical_aliases_by_address.get(
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS
        )
        or indexes.reviewed_logical_aliases_by_address.get(leaf_address)
    ):
        raise ValueError(
            f"{label} aggregate-leaf bridge requires unique authored "
            "non-provider caller, aggregate, leaf, storage, arithmetic, and "
            "same-owner provenance"
        )

    target = candidate.target
    definition = candidate.caller_definition
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == expected_caller_start
    ]
    if (
        target is None
        or getattr(target, "name", "") != expected_target_name
        or getattr(target, "target_binary", "") != "recoil"
        or Path(str(getattr(target, "manifest_path", ""))).resolve()
        != expected_target_manifest.resolve()
        or not bool(
            getattr(target, "check_translation_unit_function_order", False)
        )
        or tuple(getattr(target, "source_files", ()))
        not in {(), (expected_source_path,)}
    ):
        raise ValueError(
            f"{label} aggregate-leaf bridge requires the exact registered "
            "target and source listing"
        )
    if len(contribution_rows) != 1:
        raise ValueError(
            f"{label} aggregate-leaf bridge requires one exact registered "
            "caller contribution"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "") != expected_source_path
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != expected_caller_symbol
        or getattr(contribution_row, "symbol_regex", None) is not None
        or getattr(contribution_row, "name", "") != expected_caller_name
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            f"{label} aggregate-leaf bridge requires the exact authored "
            "translation-unit contribution row"
        )
    if (
        definition is None
        or definition.symbol != expected_caller_symbol
        or len(definition.data) != expected_caller_size
        or len(definition.relocation_mask) != len(definition.data)
        or definition.section_index != expected_caller_section_index
        or definition.section_start != 0
        or definition.section_end != expected_caller_size
        or (expected_body is not None and definition.data != expected_body)
    ):
        raise ValueError(
            f"{label} aggregate-leaf bridge requires the exact candidate "
            "procedure and complete COFF section extent"
        )

    exact_relocation_mask = tuple(
        any(
            relocation.offset <= offset < relocation.offset + 4
            for relocation in definition.relocations
        )
        for offset in range(len(definition.data))
    )
    aggregate_leaf_relocations = [
        relocation
        for relocation in definition.relocations
        if relocation.symbol_name == _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL
        and relocation.offset + 4 <= len(definition.data)
        and struct.unpack_from("<I", definition.data, relocation.offset)[0]
        == leaf_displacement
    ]
    complete_relocations = tuple(
        (
            relocation.offset,
            relocation.type,
            relocation.symbol_name,
            struct.unpack_from("<I", definition.data, relocation.offset)[0],
        )
        for relocation in definition.relocations
        if relocation.offset + 4 <= len(definition.data)
    )
    if (
        tuple(relocation.offset for relocation in aggregate_leaf_relocations)
        != aggregate_leaf_relocation_offsets
        or any(
            relocation.type != IMAGE_REL_I386_DIR32
            for relocation in aggregate_leaf_relocations
        )
        or (
            complete_relocation_specs is not None
            and complete_relocations != complete_relocation_specs
        )
        or tuple(definition.relocation_mask) != exact_relocation_mask
        or definition.undefined_external_data.count(
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL
        )
        != 1
        or _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL
        in definition.defined_external_data
    ):
        raise ValueError(
            f"{label} aggregate-leaf bridge requires the exact candidate "
            "DIR32 aggregate-leaf relocation offsets, target, addend, mask, "
            "and symbol role"
        )

    expected_storage = f"load({leaf_identity})"
    expected_by_ordinal = {
        row.get("ordinal"): row
        for row in expected
        if isinstance(row.get("ordinal"), int)
    }
    if (
        len(expected) != len(invocation_offsets)
        or set(expected_by_ordinal) != set(range(len(invocation_offsets)))
        or any(
            expected_by_ordinal[ordinal].get(key) != value
            for ordinal, _root, _vptr, _receiver, _call, _register, slot
            in leaf_chains
            for key, value in {
                "form": "call",
                "dispatch": "indirect",
                "identity_kind": "virtual-slot",
                "target_identity": "",
                "storage_identity": expected_storage,
                "slot_displacement": slot,
                "cleanup_bytes": None,
            }.items()
        )
        or any(
            expected_by_ordinal[ordinal].get(key) != value
            for ordinal, fields in expected_additional_rows
            for key, value in fields
        )
    ):
        raise ValueError(
            f"{label} aggregate-leaf bridge requires the complete immutable-"
            "retail caller contract and exact leaf ordinals/slots"
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    counts = Counter(offset for offset in offsets if offset is not None)
    index_by_offset = {
        offset: index
        for index, offset in enumerate(offsets)
        if offset is not None and counts[offset] == 1
    }
    instruction_by_offset = {
        offset: candidate.instructions[index]
        for offset, index in index_by_offset.items()
    }
    aggregate_pattern = re.escape(_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL)
    root_pattern = (
        rf"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
        rf"{aggregate_pattern}\+{leaf_displacement}"
    )
    exact_rows: dict[int, tuple[bytes, str]] = {}
    for (
        _ordinal,
        root_offset,
        vptr_offset,
        receiver_offset,
        call_offset,
        vptr_register,
        slot,
    ) in leaf_chains:
        exact_rows[root_offset] = (
            b"\xa1" + struct.pack("<I", leaf_displacement),
            root_pattern,
        )
        vptr_modrm = {"ecx": b"\x08", "edx": b"\x10"}[vptr_register]
        exact_rows[vptr_offset] = (
            b"\x8b" + vptr_modrm,
            rf"mov\s+{vptr_register}\s*,\s*"
            r"(?:dword\s+(?:ptr\s+)?)?\[eax\]",
        )
        exact_rows[receiver_offset] = (b"\x50", r"push\s+eax")
        call_modrm = {"ecx": b"\x51", "edx": b"\x52"}[vptr_register]
        exact_rows[call_offset] = (
            b"\xff" + call_modrm + bytes((slot,)),
            rf"call\s+(?:dword\s+(?:ptr\s+)?)?"
            rf"\[{vptr_register}\+{slot}\]",
        )
    if set(exact_rows) - set(instruction_by_offset):
        raise ValueError(
            f"{label} aggregate-leaf bridge requires one unique instruction "
            "at every reviewed load/vptr/receiver/call offset"
        )
    for offset, (body, pattern) in exact_rows.items():
        instruction = instruction_by_offset[offset]
        if (
            bytes(int(value, 16) for value in instruction.bytes) != body
            or definition.data[offset : offset + len(body)] != body
            or re.fullmatch(
                pattern,
                instruction.raw_text.strip(),
                flags=re.IGNORECASE,
            )
            is None
        ):
            raise ValueError(
                f"{label} aggregate-leaf bridge requires exact COD/COFF "
                f"bytes, registers, and operands at +{hex(offset)}"
            )
    load_anchors = sorted(
        offset
        for offset, instruction in instruction_by_offset.items()
        if re.fullmatch(
            root_pattern,
            instruction.raw_text.strip(),
            flags=re.IGNORECASE,
        )
        is not None
    )
    expected_load_anchors = sorted(chain[1] for chain in leaf_chains)
    if load_anchors != expected_load_anchors:
        raise ValueError(
            f"{label} aggregate-leaf bridge rejects duplicate, missing, or "
            "shifted aggregate-leaf load anchors"
        )

    candidate_invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=expected_caller_start,
        caller_end_exclusive=expected_caller_end_exclusive,
    )
    candidate_invocation_offsets = tuple(
        offsets[index] for index in candidate_invocation_indices
    )
    start = address_value(expected_caller_start)
    end = address_value(expected_caller_end_exclusive)
    runtime_addresses = _cc_cfg._instruction_runtime_addresses(
        candidate.instructions,
        source="cod",
        caller_start=start,
    )
    runtime_index_by_address = {
        address: index
        for index, address in enumerate(runtime_addresses)
        if address is not None
    }
    (
        reviewed_switch_targets,
        reviewed_switch_data_indices,
    ) = _zinput_exact_candidate_switch_successors(
        candidate,
        expected_switch,
        label=label,
    )
    switch_overlap = (
        candidate.local_control_flow_indices
        & reviewed_switch_targets.keys()
    )
    if any(
        candidate.local_control_flow_targets.get(index)
        != reviewed_switch_targets[index]
        for index in switch_overlap
    ) or any(
        index in candidate.local_control_flow_targets
        and index not in candidate.local_control_flow_indices
        for index in reviewed_switch_targets
    ):
        raise ValueError(
            f"{label} switch proof conflicts with generic local CFG metadata"
        )
    local_control_flow_targets = {
        **candidate.local_control_flow_targets,
        **reviewed_switch_targets,
    }
    successors, unresolved_cfg = _cc_cfg._exact_invocation_cfg(
        candidate.instructions,
        instruction_addresses=runtime_addresses,
        instruction_index_by_address=runtime_index_by_address,
        source="cod",
        caller_start=start,
        caller_end=end,
        local_control_flow_indices=(
            candidate.local_control_flow_indices
            | frozenset(reviewed_switch_targets)
        ),
        local_control_flow_targets=local_control_flow_targets,
    )
    expected_branch_map = dict(expected_branch_successors)
    candidate_branch_offsets = {
        offset
        for offset, instruction in instruction_by_offset.items()
        if _cc_cfg._instruction_mnemonic(instruction).startswith("j")
    }
    exact_branch_cfg = all(
        successors.get(index_by_offset[offset])
        == tuple(index_by_offset[target] for target in targets)
        for offset, targets in expected_branch_successors
    )
    if (
        candidate_invocation_offsets != invocation_offsets
        or unresolved_cfg - reviewed_switch_data_indices
        or _cc_cfg._reachable_cfg_indices(successors, (0,))
        != (
            frozenset(range(len(candidate.instructions)))
            - reviewed_switch_data_indices
        )
        or (
            expected_branch_successors
            and candidate_branch_offsets != set(expected_branch_map)
        )
        or not exact_branch_cfg
    ):
        raise ValueError(
            f"{label} aggregate-leaf bridge requires the complete exact "
            "candidate invocation population and reachable CFG"
        )
    for (
        _ordinal,
        root_offset,
        vptr_offset,
        _receiver_offset,
        call_offset,
        vptr_register,
        _slot,
    ) in leaf_chains:
        root_index = index_by_offset[root_offset]
        vptr_index = index_by_offset[vptr_offset]
        call_index = index_by_offset[call_offset]
        if (
            any(
                successors.get(index)
                != tuple(
                    index_by_offset[target]
                    for target in expected_branch_map.get(
                        offsets[index],
                        (offsets[index + 1],),
                    )
                )
                for index in range(root_index, call_index)
            )
            or any(
                _cc_cfg._instruction_may_clobber_register(
                    candidate.instructions[index], "eax"
                )
                for index in range(root_index + 1, vptr_index)
            )
            or any(
                _cc_cfg._instruction_may_clobber_register(
                    candidate.instructions[index], register
                )
                for index in range(vptr_index + 1, call_index)
                for register in ("eax", vptr_register)
            )
            or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
            or any(definition.relocation_mask[call_offset : call_offset + 3])
        ):
            raise ValueError(
                f"{label} aggregate-leaf bridge rejects alternate branch, "
                "cleanup, receiver/vptr clobber, or relocated call bytes"
            )

    expression = (
        f"{_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL}+{leaf_displacement}"
    )
    return (
        {
            expression: ReviewedStaticStorageReferenceBridge(
                aggregate_symbol=_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL,
                displacement=leaf_displacement,
                access_width=4,
                storage_identity=leaf_identity,
            )
        },
        {
            hex(vptr_offset): ReviewedMemberVptrStorageBridge(
                register=vptr_register,
                source_register="eax",
                source_provenance=leaf_identity,
                receiver_register="eax",
                receiver_provenance=leaf_identity,
                storage_identity=expected_storage,
                slot_displacement=slot,
                call_address=hex(call_offset),
            )
            for (
                _ordinal,
                _root_offset,
                vptr_offset,
                _receiver_offset,
                call_offset,
                vptr_register,
                slot,
            ) in leaf_chains
        },
    )


def _zinput_runtime_aggregate_leaf_logical_ordinals(
    caller_start: str,
    *,
    leaf_count: int,
    invocation_count: int,
    occupied_ordinals: tuple[int, ...],
) -> tuple[int, ...]:
    """Return one exact leaf-ordinal partition for the three runtime callers."""

    normalized_start = normalize_address(caller_start)
    logical_ordinals = (
        _cc_catalog.ZINPUT_RUNTIME_AGGREGATE_LEAF_LOGICAL_ORDINALS.get(normalized_start)
    )
    if logical_ordinals is None:
        raise ValueError(
            "zInput runtime aggregate-leaf logical ordinal population is "
            f"missing for {normalized_start}"
        )
    if (
        not isinstance(logical_ordinals, tuple)
        or len(logical_ordinals) != leaf_count
        or any(type(ordinal) is not int for ordinal in logical_ordinals)
    ):
        raise ValueError(
            "zInput runtime aggregate-leaf logical ordinal cardinality "
            "drifted"
        )
    if len(set(logical_ordinals)) != len(logical_ordinals):
        raise ValueError(
            "zInput runtime aggregate-leaf logical ordinals contain a "
            "duplicate"
        )
    valid_ordinals = set(range(invocation_count))
    if set(logical_ordinals) - valid_ordinals:
        raise ValueError(
            "zInput runtime aggregate-leaf logical ordinal is out of range"
        )
    if (
        len(set(occupied_ordinals)) != len(occupied_ordinals)
        or any(type(ordinal) is not int for ordinal in occupied_ordinals)
        or set(occupied_ordinals) - valid_ordinals
    ):
        raise ValueError(
            "zInput runtime direct/callback logical ordinal population "
            "drifted"
        )
    if set(logical_ordinals) & set(occupied_ordinals):
        raise ValueError(
            "zInput runtime aggregate-leaf logical ordinal overlaps a "
            "direct/callback row"
        )
    if set(logical_ordinals) | set(occupied_ordinals) != valid_ordinals:
        raise ValueError(
            "zInput runtime aggregate-leaf logical ordinal population is "
            "missing a caller row"
        )
    return logical_ordinals


def _zinput_keyboard_aggregate_leaf_candidate_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedStaticStorageReferenceBridge],
    dict[str, ReviewedMemberVptrStorageBridge],
]:
    normalized_start = normalize_address(caller_start)
    runtime_spec = _cc_catalog.ZINPUT_RUNTIME_DISPATCH_CANDIDATE_SPECS.get(
        normalized_start
    )
    if runtime_spec is not None:
        identities = _zinput_runtime_dispatch_storage_identities(
            indexes,
            caller_start=normalized_start,
        )
        direct_targets = {
            "report": indexes.by_address.get("0x472490", ""),
            "translate": indexes.by_address.get("0x46fba0", ""),
        }

        def direct_fields(target: str) -> tuple[tuple[str, Any], ...]:
            if not target or target in indexes.provider_ids:
                raise ValueError(
                    "zInput keyboard runtime-dispatch bridge requires exact "
                    "authored direct identities"
                )
            return (
                ("form", "call"), ("dispatch", "direct"),
                ("identity_kind", "direct"),
                ("target_identity", target), ("storage_identity", ""),
                ("slot_displacement", None), ("cleanup_bytes", None),
            )

        additional: tuple[
            tuple[int, tuple[tuple[str, Any], ...]], ...
        ]
        if normalized_start == "0x46f450":
            additional = ((2, direct_fields(direct_targets["report"])),)
            aggregate_relocations = (0x13, 0x46)
        elif normalized_start == "0x46f690":
            callback_fields = lambda storage: (
                ("form", "call"), ("dispatch", "indirect"),
                ("identity_kind", "callback"),
                ("target_identity", ""), ("storage_identity", storage),
                ("slot_displacement", None), ("cleanup_bytes", None),
            )
            additional = (
                (2, direct_fields(direct_targets["translate"])),
                (3, callback_fields(identities["raw"])),
                (4, callback_fields(identities["key-callback"])),
                (5, direct_fields(direct_targets["report"])),
            )
            aggregate_relocations = (0x04, 0x3E)
        else:
            additional = ((2, direct_fields(direct_targets["report"])),)
            aggregate_relocations = (0x22, 0x4B)
        leaf_ordinals = _zinput_runtime_aggregate_leaf_logical_ordinals(
            normalized_start,
            leaf_count=len(runtime_spec["leaf_calls"]),
            invocation_count=len(runtime_spec["calls"]),
            occupied_ordinals=tuple(ordinal for ordinal, _fields in additional),
        )
        return _zinput_exact_aggregate_leaf_member_vptr_candidate_bridges(
            expected,
            candidate,
            document=document,
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=indexes,
            label=f"zInput keyboard runtime dispatch {normalized_start}",
            expected_caller_identity=(
                f"symbol:recoil:function:{normalized_start}"
            ),
            expected_caller_start=normalized_start,
            expected_caller_end_exclusive=runtime_spec["end"],
            expected_caller_symbol=runtime_spec["symbol"],
            expected_caller_candidate_aliases=runtime_spec[
                "candidate_aliases"
            ],
            expected_caller_name=runtime_spec["name"],
            expected_caller_anchor_id=runtime_spec["anchor"],
            expected_caller_size=runtime_spec["candidate_size"],
            expected_caller_section_index=runtime_spec["section"],
            expected_physical_block_id="recoil:block:0x46f300",
            expected_target_name=_cc_catalog.ZINPUT_KEYBOARD_TARGET_NAME,
            expected_target_manifest=_cc_catalog.ZINPUT_KEYBOARD_TARGET_MANIFEST,
            expected_source_path=_cc_catalog.ZINPUT_KEYBOARD_SOURCE_PATH,
            leaf_symbol_id=_cc_catalog.ZINPUT_KEYBOARD_DEVICE_SYMBOL_ID,
            leaf_storage_id=_cc_catalog.ZINPUT_KEYBOARD_DEVICE_STORAGE_ID,
            leaf_address=_cc_catalog.ZINPUT_KEYBOARD_DEVICE_ADDRESS,
            leaf_name=_cc_catalog.ZINPUT_KEYBOARD_DEVICE_NAME,
            leaf_displacement=_cc_catalog.ZINPUT_KEYBOARD_DEVICE_DISPLACEMENT,
            aggregate_leaf_relocation_offsets=aggregate_relocations,
            invocation_offsets=runtime_spec["calls"],
            leaf_chains=tuple(
                (ordinal, *chain)
                for ordinal, chain in zip(
                    leaf_ordinals,
                    runtime_spec["leaf_calls"],
                )
            ),
            expected_retail_size=runtime_spec["retail_size"],
            expected_additional_rows=additional,
            expected_switch=runtime_spec["switch"],
        )
    if normalized_start == _cc_catalog.ZINPUT_KEYBOARD_SHUTDOWN_CALLER_START:
        return _zinput_exact_aggregate_leaf_member_vptr_candidate_bridges(
            expected,
            candidate,
            document=document,
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=indexes,
            label="zInput keyboard shutdown",
            expected_caller_identity=(
                _cc_catalog.ZINPUT_KEYBOARD_SHUTDOWN_CALLER_IDENTITY
            ),
            expected_caller_start=_cc_catalog.ZINPUT_KEYBOARD_SHUTDOWN_CALLER_START,
            expected_caller_end_exclusive=(
                _cc_catalog.ZINPUT_KEYBOARD_SHUTDOWN_CALLER_END_EXCLUSIVE
            ),
            expected_caller_symbol=_cc_catalog.ZINPUT_KEYBOARD_SHUTDOWN_CALLER_SYMBOL,
            expected_caller_name=_cc_catalog.ZINPUT_KEYBOARD_SHUTDOWN_CALLER_NAME,
            expected_caller_anchor_id=(
                _cc_catalog.ZINPUT_KEYBOARD_SHUTDOWN_CALLER_ANCHOR_ID
            ),
            expected_caller_size=_cc_catalog.ZINPUT_KEYBOARD_SHUTDOWN_CANDIDATE_SIZE,
            expected_caller_section_index=(
                _cc_catalog.ZINPUT_KEYBOARD_SHUTDOWN_COFF_SECTION_INDEX
            ),
            expected_physical_block_id="recoil:block:0x46f300",
            expected_target_name=_cc_catalog.ZINPUT_KEYBOARD_TARGET_NAME,
            expected_target_manifest=_cc_catalog.ZINPUT_KEYBOARD_TARGET_MANIFEST,
            expected_source_path=_cc_catalog.ZINPUT_KEYBOARD_SOURCE_PATH,
            leaf_symbol_id=_cc_catalog.ZINPUT_KEYBOARD_DEVICE_SYMBOL_ID,
            leaf_storage_id=_cc_catalog.ZINPUT_KEYBOARD_DEVICE_STORAGE_ID,
            leaf_address=_cc_catalog.ZINPUT_KEYBOARD_DEVICE_ADDRESS,
            leaf_name=_cc_catalog.ZINPUT_KEYBOARD_DEVICE_NAME,
            leaf_displacement=_cc_catalog.ZINPUT_KEYBOARD_DEVICE_DISPLACEMENT,
            aggregate_leaf_relocation_offsets=(0x01, 0x10),
            invocation_offsets=(
                _cc_catalog.ZINPUT_KEYBOARD_SHUTDOWN_INVOCATION_OFFSETS
            ),
            leaf_chains=_cc_catalog.ZINPUT_KEYBOARD_SHUTDOWN_LEAF_CHAINS,
            complete_relocation_specs=(
                _cc_catalog.ZINPUT_KEYBOARD_SHUTDOWN_RELOCATIONS
            ),
            expected_body=_cc_catalog.ZINPUT_KEYBOARD_SHUTDOWN_BODY,
            expected_additional_rows=((
                2,
                (
                    ("form", "call"),
                    ("dispatch", "indirect"),
                    ("identity_kind", "iat"),
                    ("target_identity", "iat:free"),
                    ("storage_identity", "iat:free"),
                    ("slot_displacement", None),
                    ("cleanup_bytes", 4),
                ),
            ),),
            expected_branch_successors=(
                _cc_catalog.ZINPUT_KEYBOARD_SHUTDOWN_BRANCH_SUCCESSORS
            ),
        )
    if normalized_start != _cc_catalog.ZINPUT_KEYBOARD_INIT_CALLER_START:
        return {}, {}
    return _zinput_exact_aggregate_leaf_member_vptr_candidate_bridges(
        expected,
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        label="zInput keyboard",
        expected_caller_identity=_cc_catalog.ZINPUT_KEYBOARD_INIT_CALLER_IDENTITY,
        expected_caller_start=_cc_catalog.ZINPUT_KEYBOARD_INIT_CALLER_START,
        expected_caller_end_exclusive=(
            _cc_catalog.ZINPUT_KEYBOARD_INIT_CALLER_END_EXCLUSIVE
        ),
        expected_caller_symbol=_cc_catalog.ZINPUT_KEYBOARD_INIT_CALLER_SYMBOL,
        expected_caller_name=_cc_catalog.ZINPUT_KEYBOARD_INIT_CALLER_NAME,
        expected_caller_anchor_id=_cc_catalog.ZINPUT_KEYBOARD_INIT_CALLER_ANCHOR_ID,
        expected_caller_size=_cc_catalog.ZINPUT_KEYBOARD_INIT_CANDIDATE_SIZE,
        expected_caller_section_index=_cc_catalog.ZINPUT_KEYBOARD_INIT_COFF_SECTION_INDEX,
        expected_physical_block_id="recoil:block:0x46f300",
        expected_target_name=_cc_catalog.ZINPUT_KEYBOARD_TARGET_NAME,
        expected_target_manifest=_cc_catalog.ZINPUT_KEYBOARD_TARGET_MANIFEST,
        expected_source_path=_cc_catalog.ZINPUT_KEYBOARD_SOURCE_PATH,
        leaf_symbol_id=_cc_catalog.ZINPUT_KEYBOARD_DEVICE_SYMBOL_ID,
        leaf_storage_id=_cc_catalog.ZINPUT_KEYBOARD_DEVICE_STORAGE_ID,
        leaf_address=_cc_catalog.ZINPUT_KEYBOARD_DEVICE_ADDRESS,
        leaf_name=_cc_catalog.ZINPUT_KEYBOARD_DEVICE_NAME,
        leaf_displacement=_cc_catalog.ZINPUT_KEYBOARD_DEVICE_DISPLACEMENT,
        aggregate_leaf_relocation_offsets=(
            _cc_catalog.ZINPUT_KEYBOARD_INIT_AGGREGATE_LEAF_RELOCATIONS
        ),
        invocation_offsets=_cc_catalog.ZINPUT_KEYBOARD_INIT_INVOCATION_OFFSETS,
        leaf_chains=_cc_catalog.ZINPUT_KEYBOARD_INIT_LEAF_CHAINS,
    )


def _zinput_runtime_dispatch_candidate_register_bridges(
    candidate: CandidateAssembly,
    *,
    caller_start: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedRegisterCallStorageBridge]:
    """Consume the exact PollState COD/COFF callback-call census."""
    from _recoil.call_contract.records import ReviewedRegisterCallStorageBridge

    normalized_start = normalize_address(caller_start)
    spec = _cc_catalog.ZINPUT_RUNTIME_DISPATCH_CANDIDATE_SPECS.get(normalized_start)
    callbacks = tuple(spec.get("register_callbacks", ())) if spec else ()
    if not callbacks:
        return {}
    callback_rows = tuple(spec.get("callback_rows", ()))
    callback_branch_targets = tuple(
        spec.get("callback_branch_targets", ())
    )
    callback_relocations = tuple(spec.get("callback_relocations", ()))
    if (
        normalized_start != "0x46f690"
        or callbacks != _cc_catalog.ZINPUT_POLL_REGISTER_CALLBACKS
        or callback_rows != _cc_catalog.ZINPUT_POLL_CALLBACK_PROVENANCE_ROWS
        or callback_branch_targets != _cc_catalog.ZINPUT_POLL_CALLBACK_BRANCH_TARGETS
        or callback_relocations
        != _cc_catalog.ZINPUT_POLL_CALLBACK_PROVENANCE_RELOCATIONS
    ):
        raise ValueError(
            "zInput PollState callback provenance role partition drifted"
        )
    definition = candidate.caller_definition
    if (
        definition is None
        or definition.symbol != spec["symbol"]
        or definition.section_index <= 0
        or definition.section_start != 0
        or definition.section_end != spec["candidate_size"]
        or len(definition.data) != spec["candidate_size"]
    ):
        raise ValueError(
            "zInput PollState callback bridge requires the exact candidate "
            "COMDAT extent"
        )
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    by_offset = {
        offset: instruction
        for offset, instruction in zip(offsets, candidate.instructions)
        if offset is not None
    }
    if len(by_offset) != sum(offset is not None for offset in offsets):
        raise ValueError(
            "zInput PollState callback bridge rejects colliding COD offsets"
        )
    callback_regions = ((0x174, 0x190), (0x1F7, 0x210))
    actual_region_offsets = tuple(
        offset
        for offset in offsets
        if offset is not None
        and any(start <= offset < end for start, end in callback_regions)
    )
    expected_region_offsets = tuple(
        offset for _role, offset, _body, _pattern in callback_rows
    )
    if actual_region_offsets != expected_region_offsets:
        raise ValueError(
            "zInput PollState callback provenance instruction population "
            "is missing, extra, or uses an old coordinate"
        )
    for role, offset, body, pattern in callback_rows:
        instruction = by_offset.get(offset)
        if (
            instruction is None
            or bytes(int(item, 16) for item in instruction.bytes) != body
            or definition.data[offset : offset + len(body)] != body
            or re.fullmatch(
                pattern,
                instruction.raw_text.strip(),
                flags=re.IGNORECASE,
            )
            is None
        ):
            raise ValueError(
                "zInput PollState callback provenance COD/COFF bytes, "
                f"registers, or operands drifted for {role} at +{hex(offset)}"
            )
    branch_rows: list[tuple[int, str]] = []
    for branch_offset, target_offset in callback_branch_targets:
        instruction = by_offset.get(branch_offset)
        if instruction is None:
            raise ValueError(
                "zInput PollState callback branch coordinates drifted"
            )
        body = bytes(int(item, 16) for item in instruction.bytes)
        labels = re.findall(
            r"\$L[0-9]+", instruction.raw_text, flags=re.IGNORECASE
        )
        if (
            len(body) != 2
            or body[0] != 0x74
            or branch_offset + len(body)
            + struct.unpack_from("<b", body, 1)[0]
            != target_offset
            or target_offset not in by_offset
            or len(labels) != 1
        ):
            raise ValueError(
                "zInput PollState callback branch target/label role drifted "
                f"at +{hex(branch_offset)}"
            )
        branch_rows.append((target_offset, labels[0]))
    if any(
        (left_target == right_target) != (left_label == right_label)
        for index, (left_target, left_label) in enumerate(branch_rows)
        for right_target, right_label in branch_rows[index + 1 :]
    ):
        raise ValueError(
            "zInput PollState callback branch target/label alias partition "
            "drifted"
        )
    actual_callback_relocations = tuple(
        (
            relocation.offset,
            relocation.type,
            relocation.symbol_name,
            struct.unpack_from(
                "<I",
                definition.data,
                relocation.offset,
            )[0],
        )
        for relocation in definition.relocations
        if any(
            start <= relocation.offset < end
            for start, end in callback_regions
        )
        and relocation.offset + 4 <= len(definition.data)
    )
    if actual_callback_relocations != callback_relocations:
        raise ValueError(
            "zInput PollState callback provenance storage relocation "
            "population drifted"
        )
    identities = _zinput_runtime_dispatch_storage_identities(
        indexes,
        caller_start=normalized_start,
    )
    aggregate_address = address_value(_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS)
    if (
        aggregate_address + callback_relocations[0][3]
        != address_value("0x565bc4")
        or aggregate_address + callback_relocations[1][3]
        != address_value("0x565bc8")
        or aggregate_address + callback_relocations[3][3]
        != address_value("0x561cd8")
        or aggregate_address + callback_relocations[4][3]
        != address_value("0x561cd4")
        or identities["raw"] == identities["raw-context"]
        or identities["key-callback"]
        != _zinput_indexed_callback_identity(
            _cc_receiver_instructions._abstract_with_displacement(identities["key-table"], 4)
        )
    ):
        raise ValueError(
            "zInput PollState callback provenance storage roles drifted"
        )
    return {
        hex(offset): ReviewedRegisterCallStorageBridge(
            register=register,
            storage_identity=(
                identities["raw"]
                if storage_kind == "raw"
                else identities["key-callback"]
            ),
            identity_kind="callback",
            assembly_source="cod",
            form=form,
        )
        for offset, register, storage_kind, form in callbacks
    }


def _zinput_bindmap_runtime_dispatch_candidate_register_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedRegisterCallStorageBridge]:
    """Publish only the three exact BindMap callback-table call packages."""
    from _recoil.call_contract.records import ReviewedRegisterCallStorageBridge

    normalized_start = normalize_address(caller_start)
    spec = _cc_catalog.ZINPUT_BINDMAP_CANDIDATE_DISPATCH_SPECS.get(normalized_start)
    if spec is None:
        return {}
    normalized_end = normalize_address(caller_end_exclusive)
    expected_identity = f"symbol:recoil:function:{normalized_start}"
    if (
        caller_identity != expected_identity
        or normalized_end != spec["end"]
        or indexes.by_address.get(normalized_start) != caller_identity
        or caller_identity in indexes.provider_ids
    ):
        raise ValueError(
            "zInput BindMap runtime-dispatch bridge requires the exact "
            "reviewed caller identity and extent"
        )

    symbol_id = caller_identity.removeprefix("symbol:")
    symbol = document.collection("symbols").get(symbol_id)
    trace = symbol.get("source_traceability") if isinstance(
        symbol, Mapping
    ) else None
    edges = trace.get("source_edges") if isinstance(trace, Mapping) else None
    candidate_names = {
        name
        for name, identity in indexes.by_candidate_name.items()
        if identity == caller_identity
    }
    if (
        not isinstance(symbol, Mapping)
        or symbol.get("address") != normalized_start
        or symbol.get("end_exclusive") != spec["end"]
        or symbol.get("size") != spec["size"]
        or symbol.get("binary") != "recoil"
        or symbol.get("kind") != "function"
        or symbol.get("pipeline_class") != "authored"
        or symbol.get("authored_order_role") != "authored-body"
        or symbol.get("ownership_state") != "primary-owned"
        or symbol.get("extent_state") != "known"
        or symbol.get("navigation_name") != spec["name"]
        or symbol.get("output_section_id") != "recoil:section:.text"
        or symbol.get("physical_block_id")
        != _cc_catalog.ZINPUT_BINDMAP_PHYSICAL_BLOCK_ID
        or candidate_names != {spec["symbol"]}
        or not isinstance(trace, Mapping)
        or trace.get("state") != "resolved"
        or trace.get("reason_code") not in {None, ""}
        or not isinstance(edges, list)
        or len(edges) != 1
        or not isinstance(edges[0], Mapping)
        or edges[0].get("relation") != "defines"
        or edges[0].get("anchor_id") != spec["anchor"]
        or edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.ZINPUT_BINDMAP_SOURCE_PATH}
    ):
        raise ValueError(
            "zInput BindMap runtime-dispatch bridge requires exact authored "
            "caller, source, owner, TU, and ABI-alias provenance"
        )

    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == normalized_start
    ]
    if (
        target is None
        or getattr(target, "name", "") != _cc_catalog.ZINPUT_BINDMAP_TARGET_NAME
        or getattr(target, "target_binary", "") != "recoil"
        or Path(str(getattr(target, "manifest_path", ""))).resolve()
        != _cc_catalog.ZINPUT_BINDMAP_TARGET_MANIFEST.resolve()
        or not bool(
            getattr(target, "check_translation_unit_function_order", False)
        )
        or tuple(getattr(target, "source_files", ()))
        not in {(), (_cc_catalog.ZINPUT_BINDMAP_SOURCE_PATH,)}
        or len(contribution_rows) != 1
    ):
        raise ValueError(
            "zInput BindMap runtime-dispatch bridge requires the exact "
            "registered target and caller contribution"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.ZINPUT_BINDMAP_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != spec["symbol"]
        or getattr(contribution_row, "symbol_regex", None) is not None
        or getattr(contribution_row, "name", "") != spec["name"]
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "zInput BindMap runtime-dispatch caller contribution drifted"
        )

    definition = candidate.caller_definition
    if (
        definition is None
        or definition.symbol != spec["symbol"]
        or definition.section_index <= 0
        or definition.section_start != 0
        or definition.section_end != spec["size"]
        or len(definition.data) != spec["size"]
        or definition.data != spec["body"]
        or len(definition.relocation_mask) != len(definition.data)
    ):
        raise ValueError(
            "zInput BindMap runtime-dispatch bridge requires the exact "
            "candidate COMDAT extent and body"
        )

    expected_relocations = [
        (offset, IMAGE_REL_I386_REL32, target_symbol, 0)
        for _ordinal, offset, target_symbol in spec["direct_relocations"]
    ]
    expected_relocations.extend(
        (
            offset,
            IMAGE_REL_I386_DIR32,
            target_symbol,
            addend,
        )
        for offset, target_symbol, addend
        in spec.get("current_relocations", ())
    )
    expected_relocations.sort(key=lambda row: row[0])
    actual_relocations = tuple(
        (
            relocation.offset,
            relocation.type,
            relocation.symbol_name,
            struct.unpack_from("<I", definition.data, relocation.offset)[0],
        )
        for relocation in definition.relocations
        if relocation.offset + 4 <= len(definition.data)
    )
    exact_mask = tuple(
        any(
            relocation.offset <= offset < relocation.offset + 4
            for relocation in definition.relocations
        )
        for offset in range(len(definition.data))
    )
    if (
        actual_relocations != tuple(expected_relocations)
        or tuple(definition.relocation_mask) != exact_mask
    ):
        raise ValueError(
            "zInput BindMap runtime-dispatch relocation population or mask "
            "drifted"
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    if (
        tuple(offset for offset in offsets if offset is not None)
        != spec["instruction_offsets"]
        or len(set(offset for offset in offsets if offset is not None))
        != len(spec["instruction_offsets"])
    ):
        raise ValueError(
            "zInput BindMap runtime-dispatch COD instruction population is "
            "missing, extra, shifted, or colliding"
        )
    instruction_by_offset = {
        offset: instruction
        for offset, instruction in zip(offsets, candidate.instructions)
        if offset is not None
    }
    covered: set[int] = set()
    for offset, instruction in instruction_by_offset.items():
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError) as exc:
            raise ValueError(
                "zInput BindMap runtime-dispatch COD bytes are malformed"
            ) from exc
        if not body or definition.data[offset : offset + len(body)] != body:
            raise ValueError(
                "zInput BindMap runtime-dispatch COD/COFF row bytes drifted "
                f"at +{hex(offset)}"
            )
        covered.update(range(offset, offset + len(body)))
    code_end = max(covered, default=-1) + 1
    if covered != set(range(code_end)) or any(
        value != 0x90 for value in definition.data[code_end:]
    ):
        raise ValueError(
            "zInput BindMap runtime-dispatch code coverage or padding "
            "drifted"
        )

    actual_invocations = tuple(
        offset
        for offset, instruction in instruction_by_offset.items()
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
        or (
            _cc_cfg._instruction_mnemonic(instruction) == "jmp"
            and offset in spec["invocations"]
        )
    )
    if actual_invocations != spec["invocations"]:
        raise ValueError(
            "zInput BindMap runtime-dispatch complete invocation population "
            "drifted"
        )

    identities = _zinput_runtime_dispatch_storage_identities(
        indexes,
        caller_start=normalized_start,
    )
    storage_identity = (
        identities["bindmap-current-callback"]
        if normalized_start == "0x470e80"
        else identities["bindmap-this-callback"]
    )
    if normalized_start == "0x470e80" and (
        address_value(_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_ADDRESS) + 0x41F0
        != address_value("0x565ea0")
        or not storage_identity.endswith("+0xc),stride=0x4))")
    ):
        raise ValueError(
            "zInput BindMap current-context storage arithmetic drifted"
        )

    expected_by_ordinal = {
        row.get("ordinal"): row
        for row in expected
        if isinstance(row, Mapping) and type(row.get("ordinal")) is int
    }
    if (
        len(expected) != len(spec["invocations"])
        or set(expected_by_ordinal) != set(range(len(expected)))
    ):
        raise ValueError(
            "zInput BindMap runtime-dispatch retail contract cardinality "
            "drifted"
        )
    callback_ordinals = {row[0] for row in spec["callbacks"]}
    direct_ordinals = {row[0] for row in spec["direct_relocations"]}
    if (
        callback_ordinals & direct_ordinals
        or callback_ordinals | direct_ordinals != set(range(len(expected)))
    ):
        raise ValueError(
            "zInput BindMap runtime-dispatch ordinal partition drifted"
        )
    for ordinal, _offset, target_symbol in spec["direct_relocations"]:
        target_identity = indexes.by_candidate_name.get(target_symbol, "")
        row = expected_by_ordinal[ordinal]
        if (
            not target_identity
            or target_identity in indexes.provider_ids
            or any(
                row.get(key) != value
                for key, value in {
                    "form": "call",
                    "dispatch": "direct",
                    "identity_kind": "direct",
                    "target_identity": target_identity,
                    "storage_identity": "",
                    "slot_displacement": None,
                    "cleanup_bytes": None,
                }.items()
            )
        ):
            raise ValueError(
                "zInput BindMap runtime-dispatch direct-neighbor provenance "
                f"drifted at ordinal {ordinal}"
            )

    bridges: dict[str, ReviewedRegisterCallStorageBridge] = {}
    for (
        ordinal,
        base_offset,
        index_offset,
        test_offset,
        branch_offset,
        argument_offset,
        call_offset,
        register,
        form,
    ) in spec["callbacks"]:
        row = expected_by_ordinal[ordinal]
        if any(
            row.get(key) != value
            for key, value in {
                "form": form,
                "dispatch": "indirect",
                "identity_kind": "callback",
                "target_identity": "",
                "storage_identity": storage_identity,
                "slot_displacement": None,
                "cleanup_bytes": None,
            }.items()
        ):
            raise ValueError(
                "zInput BindMap runtime-dispatch callback contract drifted "
                f"at ordinal {ordinal}"
            )
        exact_bodies = (
            definition.data[base_offset:index_offset],
            definition.data[index_offset:test_offset],
            definition.data[test_offset:branch_offset],
            definition.data[branch_offset:argument_offset],
            definition.data[argument_offset:call_offset],
            definition.data[call_offset:call_offset + 2],
        )
        chain_instructions = tuple(
            instruction_by_offset.get(offset)
            for offset in (
                base_offset,
                index_offset,
                test_offset,
                branch_offset,
                argument_offset,
                call_offset,
            )
        )
        if (
            any(instruction is None for instruction in chain_instructions)
            or tuple(
                bytes(int(item, 16) for item in instruction.bytes)
                for instruction in chain_instructions
                if instruction is not None
            )
            != exact_bodies
            or exact_bodies[2] != b"\x85\xd2"
            or exact_bodies[3][0] != 0x74
            or exact_bodies[4] != b"\x8b\xc8"
            or exact_bodies[5]
            != (b"\xff\xe2" if form == "tail" else b"\xff\xd2")
            or _cc_cfg._cleanup_after(
                candidate.instructions,
                offsets.index(call_offset),
            ) is not None
            or any(definition.relocation_mask[call_offset:call_offset + 2])
        ):
            raise ValueError(
                "zInput BindMap runtime-dispatch table-base/index/null/"
                "argument/register/form/cleanup chain drifted at "
                f"+{hex(call_offset)}"
            )
        bridges[hex(call_offset)] = ReviewedRegisterCallStorageBridge(
            register=register,
            storage_identity=storage_identity,
            identity_kind="callback",
            assembly_source="cod",
            form=form,
        )
    if len(bridges) != len(spec["callbacks"]):
        raise ValueError(
            "zInput BindMap runtime-dispatch callback publication "
            "cardinality drifted"
        )
    return bridges


def _zinput_mouse_aggregate_leaf_candidate_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedStaticStorageReferenceBridge],
    dict[str, ReviewedMemberVptrStorageBridge],
]:
    from _recoil.call_contract.records import ReviewedMemberVptrStorageBridge
    normalized_start = normalize_address(caller_start)
    if normalized_start == _cc_catalog.ZINPUT_MOUSE_UPDATE_ACQUIRE_CALLER_START:
        static, member = (
            _zinput_exact_aggregate_leaf_member_vptr_candidate_bridges(
                expected,
                candidate,
                document=document,
                caller_identity=caller_identity,
                caller_start=caller_start,
                caller_end_exclusive=caller_end_exclusive,
                indexes=indexes,
                label="zInput mouse UpdateAcquireState",
                expected_caller_identity=(
                    _cc_catalog.ZINPUT_MOUSE_UPDATE_ACQUIRE_CALLER_IDENTITY
                ),
                expected_caller_start=(
                    _cc_catalog.ZINPUT_MOUSE_UPDATE_ACQUIRE_CALLER_START
                ),
                expected_caller_end_exclusive=(
                    _cc_catalog.ZINPUT_MOUSE_UPDATE_ACQUIRE_CALLER_END_EXCLUSIVE
                ),
                expected_caller_symbol=(
                    _cc_catalog.ZINPUT_MOUSE_UPDATE_ACQUIRE_CALLER_SYMBOL
                ),
                expected_caller_name=(
                    _cc_catalog.ZINPUT_MOUSE_UPDATE_ACQUIRE_CALLER_NAME
                ),
                expected_caller_anchor_id=(
                    _cc_catalog.ZINPUT_MOUSE_UPDATE_ACQUIRE_CALLER_ANCHOR_ID
                ),
                expected_caller_size=(
                    _cc_catalog.ZINPUT_MOUSE_UPDATE_ACQUIRE_CANDIDATE_SIZE
                ),
                expected_caller_section_index=(
                    _cc_catalog.ZINPUT_MOUSE_UPDATE_ACQUIRE_COFF_SECTION_INDEX
                ),
                expected_physical_block_id="recoil:block:0x470020",
                expected_target_name=_cc_catalog.ZINPUT_MOUSE_TARGET_NAME,
                expected_target_manifest=_cc_catalog.ZINPUT_MOUSE_TARGET_MANIFEST,
                expected_source_path=_cc_catalog.ZINPUT_MOUSE_SOURCE_PATH,
                leaf_symbol_id=_cc_catalog.ZINPUT_MOUSE_DEVICE_SYMBOL_ID,
                leaf_storage_id=_cc_catalog.ZINPUT_MOUSE_DEVICE_STORAGE_ID,
                leaf_address=_cc_catalog.ZINPUT_MOUSE_DEVICE_ADDRESS,
                leaf_name=_cc_catalog.ZINPUT_MOUSE_DEVICE_NAME,
                leaf_displacement=_cc_catalog.ZINPUT_MOUSE_DEVICE_DISPLACEMENT,
                aggregate_leaf_relocation_offsets=(0x08,),
                invocation_offsets=(
                    _cc_catalog.ZINPUT_MOUSE_UPDATE_ACQUIRE_INVOCATION_OFFSETS
                ),
                leaf_chains=_cc_catalog.ZINPUT_MOUSE_UPDATE_ACQUIRE_LEAF_CHAINS,
                expected_retail_size=(
                    _cc_catalog.ZINPUT_MOUSE_UPDATE_ACQUIRE_CANDIDATE_SIZE
                ),
                complete_relocation_specs=(
                    _cc_catalog.ZINPUT_MOUSE_UPDATE_ACQUIRE_RELOCATIONS
                ),
                expected_body=_cc_catalog.ZINPUT_MOUSE_UPDATE_ACQUIRE_BODY,
                expected_additional_rows=((
                    1,
                    (
                        ("form", "call"),
                        ("dispatch", "indirect"),
                        ("identity_kind", "virtual-slot"),
                        ("target_identity", ""),
                        (
                            "storage_identity",
                            "load(storage:recoil:data:0x565e78)",
                        ),
                        ("slot_displacement", 0x20),
                        ("cleanup_bytes", None),
                    ),
                ),),
                expected_branch_successors=(
                    _cc_catalog.ZINPUT_MOUSE_UPDATE_ACQUIRE_BRANCH_SUCCESSORS
                ),
            )
        )
        if set(member) != {"0x12"}:
            raise ValueError(
                "zInput mouse UpdateAcquireState Acquire bridge publication "
                "drifted"
            )
        definition = candidate.caller_definition
        offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
        counts = Counter(offset for offset in offsets if offset is not None)
        instruction_by_offset = {
            offset: candidate.instructions[index]
            for index, offset in enumerate(offsets)
            if offset is not None and counts[offset] == 1
        }
        unacquire_rows = (
            (
                0x30,
                b"\x8b\x10",
                r"mov\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[eax\]",
            ),
            (0x32, b"\x50", r"push\s+eax"),
            (
                0x33,
                b"\xff\x52\x20",
                r"call\s+(?:dword\s+(?:ptr\s+)?)?\[edx\+32\]",
            ),
        )
        if (
            definition is None
            or any(
                instruction_by_offset.get(offset) is None
                or bytes(
                    int(value, 16)
                    for value in instruction_by_offset[offset].bytes
                ) != body
                or re.fullmatch(
                    pattern,
                    instruction_by_offset[offset].raw_text.strip(),
                    flags=re.IGNORECASE,
                ) is None
                for offset, body, pattern in unacquire_rows
            )
            or _cc_cfg._cleanup_after(
                candidate.instructions,
                offsets.index(0x33),
            ) is not None
            or any(definition.relocation_mask[0x33:0x36])
            or "0x30" in member
        ):
            raise ValueError(
                "zInput mouse UpdateAcquireState Unacquire vptr, receiver, "
                "slot, cleanup, or publication drifted"
            )
        member["0x30"] = ReviewedMemberVptrStorageBridge(
            register="edx",
            source_register="eax",
            source_provenance="storage:recoil:data:0x565e78",
            receiver_register="eax",
            receiver_provenance="storage:recoil:data:0x565e78",
            storage_identity="load(storage:recoil:data:0x565e78)",
            slot_displacement=0x20,
            call_address="0x33",
        )
        return static, member
    if normalized_start == _cc_catalog.ZINPUT_MOUSE_SHUTDOWN_CALLER_START:
        update_identity = indexes.by_address.get(
            _cc_catalog.ZINPUT_MOUSE_UPDATE_ACQUIRE_CALLER_START,
            "",
        )
        if (
            update_identity != _cc_catalog.ZINPUT_MOUSE_UPDATE_ACQUIRE_CALLER_IDENTITY
            or indexes.by_candidate_name.get(
                _cc_catalog.ZINPUT_MOUSE_UPDATE_ACQUIRE_CALLER_SYMBOL
            ) != update_identity
            or update_identity in indexes.provider_ids
            or [
                address
                for address, identity in indexes.by_address.items()
                if identity == update_identity
            ] != [_cc_catalog.ZINPUT_MOUSE_UPDATE_ACQUIRE_CALLER_START]
        ):
            raise ValueError(
                "zInput mouse ShutdownDevice requires the exact authored "
                "UpdateAcquireState direct-neighbor identity"
            )
        return _zinput_exact_aggregate_leaf_member_vptr_candidate_bridges(
            expected,
            candidate,
            document=document,
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=indexes,
            label="zInput mouse ShutdownDevice",
            expected_caller_identity=_cc_catalog.ZINPUT_MOUSE_SHUTDOWN_CALLER_IDENTITY,
            expected_caller_start=_cc_catalog.ZINPUT_MOUSE_SHUTDOWN_CALLER_START,
            expected_caller_end_exclusive=(
                _cc_catalog.ZINPUT_MOUSE_SHUTDOWN_CALLER_END_EXCLUSIVE
            ),
            expected_caller_symbol=_cc_catalog.ZINPUT_MOUSE_SHUTDOWN_CALLER_SYMBOL,
            expected_caller_name=_cc_catalog.ZINPUT_MOUSE_SHUTDOWN_CALLER_NAME,
            expected_caller_anchor_id=(
                _cc_catalog.ZINPUT_MOUSE_SHUTDOWN_CALLER_ANCHOR_ID
            ),
            expected_caller_size=_cc_catalog.ZINPUT_MOUSE_SHUTDOWN_CANDIDATE_SIZE,
            expected_caller_section_index=(
                _cc_catalog.ZINPUT_MOUSE_SHUTDOWN_COFF_SECTION_INDEX
            ),
            expected_physical_block_id="recoil:block:0x470020",
            expected_target_name=_cc_catalog.ZINPUT_MOUSE_TARGET_NAME,
            expected_target_manifest=_cc_catalog.ZINPUT_MOUSE_TARGET_MANIFEST,
            expected_source_path=_cc_catalog.ZINPUT_MOUSE_SOURCE_PATH,
            leaf_symbol_id=_cc_catalog.ZINPUT_MOUSE_DEVICE_SYMBOL_ID,
            leaf_storage_id=_cc_catalog.ZINPUT_MOUSE_DEVICE_STORAGE_ID,
            leaf_address=_cc_catalog.ZINPUT_MOUSE_DEVICE_ADDRESS,
            leaf_name=_cc_catalog.ZINPUT_MOUSE_DEVICE_NAME,
            leaf_displacement=_cc_catalog.ZINPUT_MOUSE_DEVICE_DISPLACEMENT,
            aggregate_leaf_relocation_offsets=(0x10, 0x20),
            invocation_offsets=_cc_catalog.ZINPUT_MOUSE_SHUTDOWN_INVOCATION_OFFSETS,
            leaf_chains=_cc_catalog.ZINPUT_MOUSE_SHUTDOWN_LEAF_CHAINS,
            expected_retail_size=_cc_catalog.ZINPUT_MOUSE_SHUTDOWN_CANDIDATE_SIZE,
            complete_relocation_specs=_cc_catalog.ZINPUT_MOUSE_SHUTDOWN_RELOCATIONS,
            expected_body=_cc_catalog.ZINPUT_MOUSE_SHUTDOWN_BODY,
            expected_additional_rows=((
                0,
                (
                    ("form", "call"),
                    ("dispatch", "direct"),
                    ("identity_kind", "direct"),
                    ("target_identity", update_identity),
                    ("storage_identity", ""),
                    ("slot_displacement", None),
                    ("cleanup_bytes", None),
                ),
            ),),
            expected_branch_successors=(
                _cc_catalog.ZINPUT_MOUSE_SHUTDOWN_BRANCH_SUCCESSORS
            ),
        )
    if normalized_start == _cc_catalog.ZINPUT_MOUSE_POLL_CALLER_START:
        direct_specs = (
            (
                _cc_catalog.ZINPUT_MOUSE_UPDATE_ACQUIRE_CALLER_START,
                _cc_catalog.ZINPUT_MOUSE_UPDATE_ACQUIRE_CALLER_SYMBOL,
            ),
            (
                "0x4704f0",
                "?MouseApplyAccumulatedDelta@zInput@@YAXXZ",
            ),
            (
                "0x4702e0",
                "?MouseGetButtonTransitionState@zInput@@YIHH@Z",
            ),
            (
                "0x470d40",
                "?DispatchMouseButtonCallbacks@zInput_BindMapContext@@QAEXXZ",
            ),
        )
        direct_identities: dict[str, str] = {}
        for address, symbol in direct_specs:
            identity = indexes.by_address.get(address, "")
            if (
                identity != f"symbol:recoil:function:{address}"
                or indexes.by_candidate_name.get(symbol) != identity
                or identity in indexes.provider_ids
                or [
                    candidate_address
                    for candidate_address, candidate_identity
                    in indexes.by_address.items()
                    if candidate_identity == identity
                ] != [address]
            ):
                raise ValueError(
                    "zInput mouse PollState requires exact authored direct "
                    f"identity for {symbol}"
                )
            direct_identities[symbol] = identity

        def direct_fields(symbol: str) -> tuple[tuple[str, Any], ...]:
            return (
                ("form", "call"),
                ("dispatch", "direct"),
                ("identity_kind", "direct"),
                ("target_identity", direct_identities[symbol]),
                ("storage_identity", ""),
                ("slot_displacement", None),
                ("cleanup_bytes", None),
            )

        leaf_storage = "load(storage:recoil:data:0x565e78)"
        virtual_fields = lambda slot: (
            ("form", "call"),
            ("dispatch", "indirect"),
            ("identity_kind", "virtual-slot"),
            ("target_identity", ""),
            ("storage_identity", leaf_storage),
            ("slot_displacement", slot),
            ("cleanup_bytes", None),
        )
        update_symbol = _cc_catalog.ZINPUT_MOUSE_UPDATE_ACQUIRE_CALLER_SYMBOL
        get_button_symbol = (
            "?MouseGetButtonTransitionState@zInput@@YIHH@Z"
        )
        static, member = (
            _zinput_exact_aggregate_leaf_member_vptr_candidate_bridges(
                expected,
                candidate,
                document=document,
                caller_identity=caller_identity,
                caller_start=caller_start,
                caller_end_exclusive=caller_end_exclusive,
                indexes=indexes,
                label="zInput mouse PollState",
                expected_caller_identity=_cc_catalog.ZINPUT_MOUSE_POLL_CALLER_IDENTITY,
                expected_caller_start=_cc_catalog.ZINPUT_MOUSE_POLL_CALLER_START,
                expected_caller_end_exclusive=(
                    _cc_catalog.ZINPUT_MOUSE_POLL_CALLER_END_EXCLUSIVE
                ),
                expected_caller_symbol=_cc_catalog.ZINPUT_MOUSE_POLL_CALLER_SYMBOL,
                expected_caller_name=_cc_catalog.ZINPUT_MOUSE_POLL_CALLER_NAME,
                expected_caller_anchor_id=(
                    _cc_catalog.ZINPUT_MOUSE_POLL_CALLER_ANCHOR_ID
                ),
                expected_caller_size=_cc_catalog.ZINPUT_MOUSE_POLL_CANDIDATE_SIZE,
                expected_caller_section_index=(
                    _cc_catalog.ZINPUT_MOUSE_POLL_COFF_SECTION_INDEX
                ),
                expected_physical_block_id="recoil:block:0x470020",
                expected_target_name=_cc_catalog.ZINPUT_MOUSE_TARGET_NAME,
                expected_target_manifest=_cc_catalog.ZINPUT_MOUSE_TARGET_MANIFEST,
                expected_source_path=_cc_catalog.ZINPUT_MOUSE_SOURCE_PATH,
                leaf_symbol_id=_cc_catalog.ZINPUT_MOUSE_DEVICE_SYMBOL_ID,
                leaf_storage_id=_cc_catalog.ZINPUT_MOUSE_DEVICE_STORAGE_ID,
                leaf_address=_cc_catalog.ZINPUT_MOUSE_DEVICE_ADDRESS,
                leaf_name=_cc_catalog.ZINPUT_MOUSE_DEVICE_NAME,
                leaf_displacement=_cc_catalog.ZINPUT_MOUSE_DEVICE_DISPLACEMENT,
                aggregate_leaf_relocation_offsets=(0x45,),
                invocation_offsets=_cc_catalog.ZINPUT_MOUSE_POLL_INVOCATION_OFFSETS,
                leaf_chains=(),
                expected_retail_size=_cc_catalog.ZINPUT_MOUSE_POLL_CANDIDATE_SIZE,
                complete_relocation_specs=_cc_catalog.ZINPUT_MOUSE_POLL_RELOCATIONS,
                expected_body=_cc_catalog.ZINPUT_MOUSE_POLL_BODY,
                expected_additional_rows=(
                    (0, direct_fields(update_symbol)),
                    (1, virtual_fields(0x64)),
                    (2, virtual_fields(0x24)),
                    (3, direct_fields(update_symbol)),
                    (4, direct_fields(
                        "?MouseApplyAccumulatedDelta@zInput@@YAXXZ"
                    )),
                    (5, direct_fields(get_button_symbol)),
                    (6, direct_fields(get_button_symbol)),
                    (7, direct_fields(get_button_symbol)),
                    (8, direct_fields(
                        "?DispatchMouseButtonCallbacks@"
                        "zInput_BindMapContext@@QAEXXZ"
                    )),
                ),
                expected_branch_successors=(
                    _cc_catalog.ZINPUT_MOUSE_POLL_BRANCH_SUCCESSORS
                ),
            )
        )
        definition = candidate.caller_definition
        offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
        counts = Counter(offset for offset in offsets if offset is not None)
        instruction_by_offset = {
            offset: candidate.instructions[index]
            for index, offset in enumerate(offsets)
            if offset is not None and counts[offset] == 1
        }
        aggregate_pattern = re.escape(
            _cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL
        )
        device_rows = (
            (0x43, bytes.fromhex("8b35c8410000"), rf"mov\s+esi\s*,\s*(?:dword\s+(?:ptr\s+)?)?{aggregate_pattern}\+16840"),
            (0x49, b"\x56", r"push\s+esi"),
            (0x4A, b"\x8b\x06", r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[esi\]"),
            (0x4C, b"\xff\x50\x64", r"call\s+(?:dword\s+(?:ptr\s+)?)?\[eax\+100\]"),
            (0x4F, b"\x8b\x0e", r"mov\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[esi\]"),
            (0x58, b"\x56", r"push\s+esi"),
            (0x59, b"\xff\x51\x24", r"call\s+(?:dword\s+(?:ptr\s+)?)?\[ecx\+36\]"),
        )
        if (
            definition is None
            or set(member)
            or set(static) != {
                f"{_cc_catalog.ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL}+16840"
            }
            or any(
                instruction_by_offset.get(offset) is None
                or bytes(
                    int(value, 16)
                    for value in instruction_by_offset[offset].bytes
                ) != body
                or re.fullmatch(
                    pattern,
                    instruction_by_offset[offset].raw_text.strip(),
                    flags=re.IGNORECASE,
                ) is None
                for offset, body, pattern in device_rows
            )
            or any(
                _cc_cfg._cleanup_after(
                    candidate.instructions,
                    offsets.index(call_offset),
                ) is not None
                or any(
                    definition.relocation_mask[
                        call_offset:call_offset + 3
                    ]
                )
                for call_offset in (0x4C, 0x59)
            )
        ):
            raise ValueError(
                "zInput mouse PollState aggregate load, receiver, vptr, "
                "slot, cleanup, collision, or publication drifted"
            )
        for vptr_offset, register, slot, call_offset in (
            (0x4A, "eax", 0x64, 0x4C),
            (0x4F, "ecx", 0x24, 0x59),
        ):
            member[hex(vptr_offset)] = ReviewedMemberVptrStorageBridge(
                register=register,
                source_register="esi",
                source_provenance="storage:recoil:data:0x565e78",
                receiver_register="esi",
                receiver_provenance="storage:recoil:data:0x565e78",
                storage_identity=leaf_storage,
                slot_displacement=slot,
                call_address=hex(call_offset),
            )
        return static, member
    if normalized_start != _cc_catalog.ZINPUT_MOUSE_INIT_CALLER_START:
        return {}, {}
    return _zinput_exact_aggregate_leaf_member_vptr_candidate_bridges(
        expected,
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        label="zInput mouse",
        expected_caller_identity=_cc_catalog.ZINPUT_MOUSE_INIT_CALLER_IDENTITY,
        expected_caller_start=_cc_catalog.ZINPUT_MOUSE_INIT_CALLER_START,
        expected_caller_end_exclusive=_cc_catalog.ZINPUT_MOUSE_INIT_CALLER_END_EXCLUSIVE,
        expected_caller_symbol=_cc_catalog.ZINPUT_MOUSE_INIT_CALLER_SYMBOL,
        expected_caller_name=_cc_catalog.ZINPUT_MOUSE_INIT_CALLER_NAME,
        expected_caller_anchor_id=_cc_catalog.ZINPUT_MOUSE_INIT_CALLER_ANCHOR_ID,
        expected_caller_size=_cc_catalog.ZINPUT_MOUSE_INIT_CANDIDATE_SIZE,
        expected_caller_section_index=_cc_catalog.ZINPUT_MOUSE_INIT_COFF_SECTION_INDEX,
        expected_physical_block_id="recoil:block:0x470020",
        expected_target_name=_cc_catalog.ZINPUT_MOUSE_TARGET_NAME,
        expected_target_manifest=_cc_catalog.ZINPUT_MOUSE_TARGET_MANIFEST,
        expected_source_path=_cc_catalog.ZINPUT_MOUSE_SOURCE_PATH,
        leaf_symbol_id=_cc_catalog.ZINPUT_MOUSE_DEVICE_SYMBOL_ID,
        leaf_storage_id=_cc_catalog.ZINPUT_MOUSE_DEVICE_STORAGE_ID,
        leaf_address=_cc_catalog.ZINPUT_MOUSE_DEVICE_ADDRESS,
        leaf_name=_cc_catalog.ZINPUT_MOUSE_DEVICE_NAME,
        leaf_displacement=_cc_catalog.ZINPUT_MOUSE_DEVICE_DISPLACEMENT,
        aggregate_leaf_relocation_offsets=(
            _cc_catalog.ZINPUT_MOUSE_INIT_AGGREGATE_LEAF_RELOCATIONS
        ),
        invocation_offsets=_cc_catalog.ZINPUT_MOUSE_INIT_INVOCATION_OFFSETS,
        leaf_chains=_cc_catalog.ZINPUT_MOUSE_INIT_LEAF_CHAINS,
    )
