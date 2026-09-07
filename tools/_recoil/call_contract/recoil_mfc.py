"""Recoil call-contract recoil mfc evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import extraction as _cc_extraction
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import listing as _cc_listing
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        CandidateExactIatRegisterLoadProof,
        IdentityIndexes,
        ProviderOrdinalImportThunk,
    )

import re
import struct
from pathlib import Path
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_DIR32,
    IMAGE_REL_I386_REL32,
    CoffRelocation,
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
from _recoil.lib.tooling import REPO_ROOT


def _wol_cstring_provider_ordinal_import_thunk_retail_bridge(
    *,
    document: ProgressDocument,
    indexes: IdentityIndexes,
    bridge_by_address: Mapping[str, Any],
    bridge_by_name: Mapping[str, Any],
    bridge: BinaryNinjaBridge,
    retail_import_targets: Sequence[Any] | None = None,
) -> tuple[tuple[ProviderOrdinalImportThunk, ...], dict[str, list[Any]]]:
    """Prove WOL's CString thunk without trusting BN's import label.

    Binary Ninja currently exposes the physical ``0x4c5ba0`` function under
    the reviewed CString navigation label while its import inventory assigns
    the same ordinal-only slot an unrelated MFC class name.  The immutable PE
    import directory, the accepted tracker provider, exact FF25 bytes, and
    complete inbound xrefs own the route.  The current BN import row is checked
    only as a same-address diagnostic representation; none of its names supply
    CString identity, the callable COFF spelling, or the IAT object spelling.
    """
    from _recoil.call_contract.records import ProviderOrdinalImportThunk

    symbols = document.collection("symbols")
    owners = document.collection("owners")
    blocks = document.collection("physical_blocks")
    symbol = symbols.get(_cc_catalog.WOL_CSTRING_TARGET_SYMBOL_ID)
    owner = owners.get(_cc_catalog.WOL_CSTRING_OWNER_ID)
    if symbol is None and owner is None:
        return (), {}
    block = (
        blocks.get(str(symbol.get("physical_block_id")))
        if isinstance(symbol, Mapping)
        else None
    )
    mapping = block.get("mapping") if isinstance(block, Mapping) else None
    authored_order = (
        block.get("order", {}).get("authored")
        if isinstance(block, Mapping)
        and isinstance(block.get("order"), Mapping)
        else None
    )
    current_not_applicable = (
        isinstance(authored_order, Mapping)
        and all(
            isinstance(authored_order.get(key), Mapping)
            and authored_order[key].get("result") == "not-applicable"
            and authored_order[key].get("disposition") == "accepted"
            and authored_order[key].get("freshness") == "current"
            and authored_order[key].get("gating") is True
            and authored_order[key].get("validation_mode") == "live"
            for key in AUTHORED_ORDER_DIMENSIONS
        )
    )
    expected_relationships = [
        {"kind": "anchor-address", "address": _cc_catalog.WOL_CSTRING_TARGET_ADDRESS},
        {
            "kind": "primary-function",
            "address": _cc_catalog.WOL_CSTRING_TARGET_ADDRESS,
            "symbol_id": _cc_catalog.WOL_CSTRING_TARGET_SYMBOL_ID,
        },
    ]
    primary_owner_ids = [
        str(owner_id)
        for owner_id, row in owners.items()
        if isinstance(row, Mapping)
        and any(
            isinstance(relationship, Mapping)
            and relationship.get("kind") == "primary-function"
            and relationship.get("symbol_id") == _cc_catalog.WOL_CSTRING_TARGET_SYMBOL_ID
            for relationship in row.get("relationships", ())
        )
    ]
    if (
        not isinstance(symbol, Mapping)
        or symbol.get("binary") != "recoil"
        or symbol.get("kind") != "function"
        or symbol.get("pipeline_class") != "non-authored"
        or symbol.get("authored_order_role") != "non-authored"
        or symbol.get("address") != _cc_catalog.WOL_CSTRING_TARGET_ADDRESS
        or symbol.get("end_exclusive") != _cc_catalog.WOL_CSTRING_TARGET_END_EXCLUSIVE
        or symbol.get("extent_state") != "known"
        or symbol.get("size") != 6
        or symbol.get("navigation_name") != _cc_catalog.WOL_CSTRING_RETAIL_NAME
        or symbol.get("output_section_id") != "recoil:section:.text"
        or symbol.get("ownership_state") != "primary-owned"
        or symbol.get("physical_block_id") != _cc_catalog.WOL_CEDIT_TARGET_BLOCK_ID
        or indexes.by_address.get(_cc_catalog.WOL_CSTRING_TARGET_ADDRESS)
        != _cc_catalog.WOL_CSTRING_TARGET_IDENTITY
        or _cc_catalog.WOL_CSTRING_TARGET_IDENTITY not in indexes.provider_ids
        or primary_owner_ids != [_cc_catalog.WOL_CSTRING_OWNER_ID]
        or not isinstance(owner, Mapping)
        or owner.get("binary") != "recoil"
        or owner.get("kind") != "provider-boundary"
        or owner.get("blocker") != "none"
        or owner.get("lifecycle_state") != "accepted"
        or owner.get("provider_state") != "accepted"
        or owner.get("source_paths") != []
        or owner.get("relationships") != expected_relationships
        or not isinstance(block, Mapping)
        or block.get("binary") != "recoil"
        or block.get("row_kind") != "physical-source-block"
        or block.get("start") != "0x4c5a50"
        or block.get("end_exclusive") != "0x4c5eb8"
        or block.get("contribution_kind") != "provider"
        or block.get("agent_source_path")
        != "provider:mfc42-tail-import-thunks"
        or block.get("source_path") != "provider:mfc42-tail-import-thunks"
        or not isinstance(block.get("contribution_ids"), list)
        or block["contribution_ids"].count(_cc_catalog.WOL_CSTRING_TARGET_SYMBOL_ID) != 1
        or not isinstance(mapping, Mapping)
        or mapping.get("status") != "provider-boundary"
        or not current_not_applicable
    ):
        raise ValueError(
            "WOL CString ordinal bridge lacks the exact accepted tracker "
            "provider and current provider block"
        )

    targets = tuple(
        retail_import_targets
        if retail_import_targets is not None
        else _retail_import_targets(_cc_catalog.DEFAULT_REFERENCE)[0]
    )
    slot_matches = [
        target
        for target in targets
        if normalize_address(str(getattr(target, "address", "")))
        == _cc_catalog.WOL_CSTRING_IAT_ADDRESS
    ]
    target = slot_matches[0] if len(slot_matches) == 1 else None
    if (
        target is None
        or getattr(target, "dll", None) != "MFC42.DLL"
        or getattr(target, "import_name", None)
        != f"#{_cc_catalog.WOL_CSTRING_IMPORT_ORDINAL}"
        or getattr(target, "import_ordinal", None)
        != _cc_catalog.WOL_CSTRING_IMPORT_ORDINAL
        or getattr(target, "descriptor_index", None)
        != _cc_catalog.WOL_CSTRING_IMPORT_DESCRIPTOR_INDEX
        or getattr(target, "thunk_index", None)
        != _cc_catalog.WOL_CSTRING_IMPORT_SLOT_INDEX
        or normalize_address(str(getattr(target, "iat_rva", "")))
        != _cc_catalog.WOL_CSTRING_IAT_RVA
        or normalize_address(str(getattr(target, "iat_end_rva", "")))
        != _cc_catalog.WOL_CSTRING_IAT_END_RVA
        or (
            address_value(_cc_catalog.WOL_CSTRING_IAT_RVA)
            - _cc_catalog.WOL_CSTRING_IMPORT_SLOT_INDEX * 4
        )
        != _cc_catalog.WOL_CSTRING_MFC42_FIRST_IAT_RVA
    ):
        raise ValueError(
            "WOL CString ordinal bridge lacks the exact immutable "
            "MFC42 descriptor-two ordinal-540 IAT/ILT slot"
        )

    friendly = bridge_by_name.get(_cc_catalog.WOL_CSTRING_RETAIL_NAME)
    import_diagnostic = bridge_by_address.get(_cc_catalog.WOL_CSTRING_TARGET_ADDRESS)

    def bn_identity(row: Any) -> tuple[str, str, str, str, str]:
        return (
            str(getattr(row, "address", "")),
            str(getattr(row, "name", "")),
            str(getattr(row, "raw_name", "")),
            str(getattr(row, "full_name", "")),
            str(getattr(row, "kind", "")),
        )

    friendly_identity = bn_identity(friendly)
    import_identity = bn_identity(import_diagnostic)
    diagnostic_aliases = {
        value for value in import_identity[1:4] if value
    }
    if (
        friendly_identity
        != (
            _cc_catalog.WOL_CSTRING_TARGET_ADDRESS,
            _cc_catalog.WOL_CSTRING_RETAIL_NAME,
            _cc_catalog.WOL_CSTRING_RETAIL_NAME,
            "",
            "function",
        )
        or import_identity[0] != _cc_catalog.WOL_CSTRING_TARGET_ADDRESS
        or import_identity[4] != "import"
        or not import_identity[1]
        or import_identity[2] != import_identity[1]
        or any(
            bn_identity(bridge_by_name.get(alias)) != import_identity
            for alias in diagnostic_aliases
        )
    ):
        raise ValueError(
            "WOL CString ordinal bridge lacks the exact BN function view "
            "and complete same-address diagnostic import view"
        )

    thunk_bytes = _cc_cfg._hexdump_bytes(
        bridge.hexdump(_cc_catalog.WOL_CSTRING_TARGET_ADDRESS, 6)
    )
    if (
        thunk_bytes != b"\xff\x25\xc0\xc3\x4c\x00"
        or normalize_address(struct.unpack_from("<I", thunk_bytes, 2)[0])
        != _cc_catalog.WOL_CSTRING_IAT_ADDRESS
    ):
        raise ValueError(
            "WOL CString ordinal bridge requires exact immutable "
            "FF25 [0x4cc3c0] thunk bytes"
        )

    get_json = getattr(bridge, "get_json", None)
    if not callable(get_json):
        raise ValueError(
            "WOL CString ordinal bridge lacks complete BN xref authority"
        )
    thunk_payload = get_json(
        "getXrefsTo", address=_cc_catalog.WOL_CSTRING_TARGET_ADDRESS, limit=100000
    )
    thunk_code, thunk_data, thunk_complete = _cc_identity._bn_inbound_xref_items(
        thunk_payload
    )
    code_addresses = tuple(
        _cc_identity._bn_xref_address(
            row,
            "source_address",
            "source_addr",
            "from_address",
            "address",
        )
        for row in thunk_code
    )
    data_addresses = tuple(
        _cc_identity._bn_xref_address(
            row,
            "source_address",
            "source_addr",
            "from_address",
            "address",
        )
        for row in thunk_data
    )
    if (
        not thunk_complete
        or not code_addresses
        or any(not address for address in (*code_addresses, *data_addresses))
        or len(code_addresses) != len(set(code_addresses))
        or len(data_addresses) != len(set(data_addresses))
    ):
        raise ValueError(
            "WOL CString ordinal bridge lacks a complete collision-free "
            "thunk-xref population"
        )

    assembly_by_function: dict[str, tuple[Instruction, ...]] = {}
    rel32_addresses: list[str] = []
    caller_rel32_addresses: list[str] = []
    for call_address in code_addresses:
        _name, function_address = _cc_identity._bn_unique_containing_function(
            bridge,
            instruction_address=call_address,
        )
        rows = assembly_by_function.setdefault(
            function_address,
            tuple(_cc_listing.parse_assembly(bridge.assembly(function_address), source="bn")),
        )
        matches = [
            row
            for row in rows
            if _cc_cfg._source_instruction_address(row) == call_address
        ]
        if len(matches) != 1:
            raise ValueError(
                "WOL CString ordinal bridge has thunk caller xref drift"
            )
        try:
            encoded = bytes(int(value, 16) for value in matches[0].bytes)
        except (TypeError, ValueError):
            encoded = b""
        is_rel32 = (
            len(encoded) == 5
            and encoded[0] in {0xE8, 0xE9}
            and normalize_address(
                address_value(call_address)
                + 5
                + struct.unpack_from("<i", encoded, 1)[0]
            )
            == _cc_catalog.WOL_CSTRING_TARGET_ADDRESS
        )
        if not is_rel32:
            continue
        rel32_addresses.append(call_address)
        if (
            address_value(_cc_catalog.WOL_CEDIT_CALLER_ADDRESS)
            <= address_value(call_address)
            < address_value(_cc_catalog.WOL_CEDIT_CALLER_END_EXCLUSIVE)
        ):
            if function_address != _cc_catalog.WOL_CEDIT_CALLER_ADDRESS or encoded[0] != 0xE8:
                raise ValueError(
                    "WOL CString ordinal bridge has non-direct WOL caller xref drift"
                )
            caller_rel32_addresses.append(call_address)
    if (
        not rel32_addresses
        or tuple(caller_rel32_addresses) != _cc_catalog.WOL_CSTRING_RETAIL_CALL_SITES
    ):
        raise ValueError(
            "WOL CString ordinal bridge requires exactly the three reviewed "
            "WOL direct E8 caller sites"
        )

    iat_payload = get_json(
        "getXrefsTo", address=_cc_catalog.WOL_CSTRING_IAT_ADDRESS, limit=100000
    )
    iat_code, iat_data, iat_complete = _cc_identity._bn_inbound_xref_items(iat_payload)
    iat_readers = tuple(
        _cc_identity._bn_xref_address(
            row,
            "source_address",
            "source_addr",
            "from_address",
            "address",
        )
        for row in iat_code
    )
    if (
        not iat_complete
        or iat_data
        or iat_readers != (_cc_catalog.WOL_CSTRING_TARGET_ADDRESS,)
    ):
        raise ValueError(
            "WOL CString ordinal bridge requires the exact sole FF25 IAT reader"
        )

    thunk = ProviderOrdinalImportThunk(
        provider_identity=_cc_catalog.WOL_CSTRING_TARGET_IDENTITY,
        thunk_address=_cc_catalog.WOL_CSTRING_TARGET_ADDRESS,
        retail_name=_cc_catalog.WOL_CSTRING_RETAIL_NAME,
        callable_symbol=_cc_catalog.WOL_CSTRING_CANDIDATE_SYMBOL,
        iat_object_symbol=_cc_catalog.WOL_CSTRING_IAT_OBJECT_SYMBOL,
        iat_identity=(
            f"iat:ordinal:9:MFC42.DLL:{_cc_catalog.WOL_CSTRING_IMPORT_ORDINAL}"
        ),
        import_dll="MFC42.DLL",
        import_ordinal=_cc_catalog.WOL_CSTRING_IMPORT_ORDINAL,
    )
    return (thunk,), {_cc_catalog.WOL_CSTRING_RETAIL_NAME: [friendly]}


def _wol_config_reviewed_ordinal_noncall_relocation_offsets(
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    supplemental_candidate_direct_bridges: Mapping[str, str],
) -> dict[str, frozenset[int]]:
    """Prove config-dialog CString callback pointers beside direct calls."""

    if caller_identity != _cc_catalog.WOL_CONFIG_CEDIT_CALLER_TOKEN:
        return {}
    if (
        supplemental_candidate_direct_bridges.get(
            _cc_catalog.WOL_CONFIG_CEDIT_CANDIDATE_SYMBOL
        )
        != _cc_catalog.WOL_CEDIT_TARGET_IDENTITY
        or supplemental_candidate_direct_bridges.get(
            _cc_catalog.WOL_CCOMBOBOX_CANDIDATE_SYMBOL
        )
        != _cc_catalog.WOL_CEDIT_TARGET_IDENTITY
    ):
        return {}
    caller_spec = _cc_catalog.WOL_CATEGORY_A_CALLER_SPECS[_cc_catalog.WOL_CONFIG_CEDIT_CALLER_TOKEN]
    definition = candidate.caller_definition
    if (
        normalize_address(caller_start) != caller_spec["address"]
        or normalize_address(caller_end_exclusive)
        != caller_spec["end_exclusive"]
        or definition is None
        or definition.symbol != caller_spec["symbol"]
        or len(definition.data) != _cc_catalog.WOL_CONFIG_CEDIT_CANDIDATE_SIZE
        or len(definition.relocation_mask) != len(definition.data)
        or definition.section_start != 0
        or definition.section_end != _cc_catalog.WOL_CONFIG_CEDIT_CANDIDATE_SIZE
        or len(definition.relocations)
        != _cc_catalog.WOL_CONFIG_CEDIT_CANDIDATE_RELOCATION_COUNT
    ):
        raise ValueError(
            "WOL config CString callback proof requires the exact complete "
            "caller COFF package"
        )

    folded_symbol = _cc_catalog.WOL_CSTRING_CANDIDATE_SYMBOL.casefold()
    references = tuple(
        sorted(
            (
                relocation
                for relocation in definition.relocations
                if relocation.symbol_name.casefold() == folded_symbol
            ),
            key=lambda relocation: relocation.offset,
        )
    )
    expected_rows = tuple(
        (offset, IMAGE_REL_I386_REL32, _cc_catalog.WOL_CSTRING_CANDIDATE_SYMBOL)
        for offset in _cc_catalog.WOL_CONFIG_CSTRING_CALL_RELOCATION_OFFSETS
    ) + tuple(
        (offset, IMAGE_REL_I386_DIR32, _cc_catalog.WOL_CSTRING_CANDIDATE_SYMBOL)
        for offset in _cc_catalog.WOL_CONFIG_CSTRING_CALLBACK_RELOCATION_OFFSETS
    )
    observed_rows = tuple(
        (row.offset, row.type, row.symbol_name) for row in references
    )
    if observed_rows != tuple(sorted(expected_rows)):
        raise ValueError(
            "WOL config CString callback proof requires two exact REL32 calls "
            "and four exact case-sensitive DIR32 callback relocations"
        )
    for offset in _cc_catalog.WOL_CONFIG_CSTRING_CALLBACK_RELOCATION_OFFSETS:
        if (
            offset < 1
            or offset + 4 > len(definition.data)
            or definition.data[offset - 1] != 0x68
            or definition.relocation_mask[offset - 1]
            or struct.unpack_from("<I", definition.data, offset)[0] != 0
            or not all(
                definition.relocation_mask[index]
                for index in range(offset, offset + 4)
            )
        ):
            raise ValueError(
                "WOL config CString callback proof requires four exact "
                "zero-addend fully masked PUSH/DIR32 rows"
            )
    return {
        _cc_catalog.WOL_CSTRING_CANDIDATE_SYMBOL: frozenset(
            _cc_catalog.WOL_CONFIG_CSTRING_CALLBACK_RELOCATION_OFFSETS
        )
    }


def _wol_config_cedit_provider_candidate_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
) -> dict[str, str]:
    """Bind the exact config-dialog CEdit constructor to reviewed CWnd."""

    if caller_identity != _cc_catalog.WOL_CONFIG_CEDIT_CALLER_TOKEN:
        return {}
    folded_symbol = _cc_catalog.WOL_CONFIG_CEDIT_CANDIDATE_SYMBOL.casefold()
    mentions = [
        instruction
        for instruction in candidate.instructions
        if folded_symbol in _cc_cfg._instruction_operand(instruction).casefold()
    ]
    if not mentions:
        return {}
    provenance = _require_wol_category_a_caller_provenance(
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    if provenance is None:
        return {}
    _caller_row, caller_spec = provenance

    if (
        len(expected) != len(_cc_catalog.WOL_CONFIG_CEDIT_INVOCATION_ORDER)
        or tuple(row.get("ordinal") for row in expected)
        != tuple(range(len(_cc_catalog.WOL_CONFIG_CEDIT_INVOCATION_ORDER)))
    ):
        raise ValueError(
            "WOL config CEdit provider bridge requires the exact complete "
            "11-call retail population"
        )
    expected_row = {
        "ordinal": _cc_catalog.WOL_CONFIG_CEDIT_CALL_ORDINAL,
        "form": "call",
        "dispatch": "direct",
        "identity_kind": "provider",
        "target_identity": _cc_catalog.WOL_CEDIT_TARGET_IDENTITY,
        "storage_identity": "",
        "slot_displacement": None,
        "cleanup_bytes": None,
    }
    if dict(expected[_cc_catalog.WOL_CONFIG_CEDIT_CALL_ORDINAL]) != expected_row:
        raise ValueError(
            "WOL config CEdit provider bridge requires immutable retail "
            "ordinal-two CWnd-provider truth"
        )

    invocations = [
        (index, instruction)
        for index, instruction in enumerate(candidate.instructions)
        if _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        and index not in candidate.local_control_flow_indices
    ]
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    invocation_offsets = tuple(
        offsets[index] if len(offsets) > index else None
        for index, _instruction in invocations
    )
    invocation_operands = tuple(
        _cc_cfg._instruction_operand(instruction).strip()
        for _index, instruction in invocations
    )
    if (
        len(invocations) != len(_cc_catalog.WOL_CONFIG_CEDIT_INVOCATION_ORDER)
        or invocation_offsets != _cc_catalog.WOL_CONFIG_CEDIT_INVOCATION_OFFSETS
        or invocation_operands != _cc_catalog.WOL_CONFIG_CEDIT_INVOCATION_ORDER
        or any(
            _cc_cfg._instruction_mnemonic(instruction) != "call"
            for _index, instruction in invocations
        )
        or mentions != [invocations[_cc_catalog.WOL_CONFIG_CEDIT_CALL_ORDINAL][1]]
    ):
        raise ValueError(
            "WOL config CEdit provider bridge requires the exact complete "
            "ordered 11-call COD population and unique ordinal-two CEdit row"
        )

    target_row = document.collection("symbols").get(
        _cc_catalog.WOL_CEDIT_TARGET_SYMBOL_ID
    )
    target_block = document.collection("physical_blocks").get(
        _cc_catalog.WOL_CEDIT_TARGET_BLOCK_ID
    )
    target_mapping = (
        target_block.get("mapping")
        if isinstance(target_block, Mapping)
        else None
    )
    provider_symbols = bridge_names.get(_cc_catalog.WOL_CEDIT_RETAIL_NAME)
    provider_symbol = (
        provider_symbols[0]
        if isinstance(provider_symbols, Sequence)
        and not isinstance(provider_symbols, (str, bytes))
        and len(provider_symbols) == 1
        else None
    )
    provider_full_symbols = bridge_names.get(_cc_catalog.WOL_CEDIT_RETAIL_FULL_NAME)
    if (
        not isinstance(target_row, Mapping)
        or target_row.get("binary") != "recoil"
        or target_row.get("kind") != "function"
        or target_row.get("pipeline_class") != "non-authored"
        or target_row.get("authored_order_role") != "non-authored"
        or target_row.get("address") != _cc_catalog.WOL_CEDIT_TARGET_ADDRESS
        or target_row.get("end_exclusive")
        != _cc_catalog.WOL_CEDIT_TARGET_END_EXCLUSIVE
        or target_row.get("extent_state") != "known"
        or target_row.get("size") != 6
        or target_row.get("navigation_name") != _cc_catalog.WOL_CEDIT_RETAIL_NAME
        or target_row.get("output_section_id") != "recoil:section:.text"
        or target_row.get("physical_block_id") != _cc_catalog.WOL_CEDIT_TARGET_BLOCK_ID
        or target_row.get("disposition") != "unresolved"
        or target_row.get("ownership_state") != "unresolved"
        or indexes.by_address.get(_cc_catalog.WOL_CEDIT_TARGET_ADDRESS)
        != _cc_catalog.WOL_CEDIT_TARGET_IDENTITY
        or _cc_catalog.WOL_CEDIT_TARGET_IDENTITY not in indexes.provider_ids
        or not isinstance(target_block, Mapping)
        or target_block.get("binary") != "recoil"
        or target_block.get("row_kind") != "physical-source-block"
        or target_block.get("start") != "0x4c5a50"
        or target_block.get("end_exclusive") != "0x4c5eb8"
        or target_block.get("contribution_kind") != "provider"
        or target_block.get("agent_source_path")
        != "provider:mfc42-tail-import-thunks"
        or target_block.get("source_path")
        != "provider:mfc42-tail-import-thunks"
        or not isinstance(target_block.get("contribution_ids"), list)
        or target_block["contribution_ids"].count(
            _cc_catalog.WOL_CEDIT_TARGET_SYMBOL_ID
        )
        != 1
        or not isinstance(target_mapping, Mapping)
        or target_mapping.get("status") != "provider-boundary"
        or provider_symbol is None
        or not isinstance(provider_full_symbols, Sequence)
        or isinstance(provider_full_symbols, (str, bytes))
        or len(provider_full_symbols) != 1
        or provider_full_symbols[0] is not provider_symbol
        or getattr(provider_symbol, "address", "")
        != _cc_catalog.WOL_CEDIT_TARGET_ADDRESS
        or getattr(provider_symbol, "name", "") != _cc_catalog.WOL_CEDIT_RETAIL_NAME
        or getattr(provider_symbol, "raw_name", "")
        != _cc_catalog.WOL_CEDIT_RETAIL_NAME
        or getattr(provider_symbol, "full_name", "")
        != _cc_catalog.WOL_CEDIT_RETAIL_FULL_NAME
        or getattr(provider_symbol, "kind", "") != "import"
    ):
        raise ValueError(
            "WOL config CEdit provider bridge requires the exact reviewed "
            "MFC42 provider row, block, and identical BN import aliases"
        )

    candidate_aliases = {
        _cc_catalog.WOL_CONFIG_CEDIT_CANDIDATE_SYMBOL.casefold(),
        _cc_catalog.WOL_CEDIT_CANDIDATE_FULL_NAME.casefold(),
    }
    collision_rows = [
        (label, name)
        for label, names in (
            ("Binary Ninja", bridge_names),
            ("candidate identity", indexes.by_candidate_name),
            ("candidate storage", indexes.storage_by_name),
        )
        for name in names
        if str(name).casefold() in candidate_aliases
    ]
    if collision_rows:
        raise ValueError(
            "WOL config CEdit provider bridge rejects candidate/provider/"
            "storage identity collision drift"
        )

    definition = candidate.caller_definition
    if (
        definition is None
        or definition.symbol != caller_spec["symbol"]
        or len(definition.data) != _cc_catalog.WOL_CONFIG_CEDIT_CANDIDATE_SIZE
        or len(definition.relocation_mask) != len(definition.data)
        or definition.section_start != 0
        or definition.section_end != _cc_catalog.WOL_CONFIG_CEDIT_CANDIDATE_SIZE
        or len(definition.relocations)
        != _cc_catalog.WOL_CONFIG_CEDIT_CANDIDATE_RELOCATION_COUNT
    ):
        raise ValueError(
            "WOL config CEdit provider bridge requires the exact complete "
            "304-byte, 25-relocation caller COFF definition"
        )
    call_offset, reference = (
        _cc_callable_identity._require_unique_exact_candidate_direct_external_call_row(
            candidate,
            candidate_symbol=_cc_catalog.WOL_CONFIG_CEDIT_CANDIDATE_SYMBOL,
        )
    )
    if (
        call_offset != _cc_catalog.WOL_CONFIG_CEDIT_CALL_OFFSET
        or reference.offset != _cc_catalog.WOL_CONFIG_CEDIT_CALL_OFFSET + 1
    ):
        raise ValueError(
            "WOL config CEdit provider bridge requires the exact offset-0x48 "
            "COD/COFF call row"
        )

    result = {
        _cc_catalog.WOL_CONFIG_CEDIT_CANDIDATE_SYMBOL: _cc_catalog.WOL_CEDIT_TARGET_IDENTITY
    }
    provisional = _cc_extraction.extract_invocation_contract(
        [invocations[_cc_catalog.WOL_CONFIG_CEDIT_CALL_ORDINAL][1]],
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge_names=bridge_names,
        compiler_generated_bridges=result,
        local_control_flow_indices=frozenset(),
        local_control_flow_targets={},
    )
    if len(provisional) != 1:
        raise ValueError(
            "WOL config CEdit provider bridge requires one exact focused "
            "candidate provider invocation"
        )
    observed_row = dict(provisional[0])
    observed_row["ordinal"] = _cc_catalog.WOL_CONFIG_CEDIT_CALL_ORDINAL
    if observed_row != expected_row:
        raise ValueError(
            "WOL config CEdit provider bridge candidate form, cleanup, or "
            "provider identity drifts from retail"
        )
    return result


def _wol_cedit_provider_candidate_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    ordinal_import_thunks: Sequence[ProviderOrdinalImportThunk],
) -> dict[str, str]:
    """Bind the exact external VC5 WOL control constructors to retail CWnd.

    Candidate decorations are not retail identity authority.  Publish the
    four finite control constructor names only when the complete reviewed
    17-call order, current authored caller, selected target, retail provider,
    Binary Ninja import, COD bytes, and COFF externals/relocations all agree.
    """

    if caller_identity == _cc_catalog.WOL_CONFIG_CEDIT_CALLER_TOKEN:
        return _wol_config_cedit_provider_candidate_bridge(
            expected,
            candidate,
            document=document,
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=indexes,
            bridge_names=bridge_names,
        )

    if (
        caller_identity != _cc_catalog.WOL_CEDIT_CALLER_TOKEN
        or normalize_address(caller_start) != _cc_catalog.WOL_CEDIT_CALLER_ADDRESS
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.WOL_CEDIT_CALLER_END_EXCLUSIVE
    ):
        return {}

    control_tokens = ("cedit", "cbutton", "clistbox", "ccombobox")

    def cstring_candidate_alias(name: object) -> bool:
        folded = re.sub(r"\s+", " ", str(name).strip()).casefold()
        if folded == _cc_catalog.WOL_CSTRING_CANDIDATE_SYMBOL.casefold():
            return True
        match = re.fullmatch(r"cstring::cstring\((.*)\)", folded)
        if match is None:
            return False
        arguments = match.group(1).strip()
        if not arguments:
            return True
        hidden_this = re.sub(r"\s+", "", arguments)
        hidden_this = hidden_this.removeprefix("class")
        return hidden_this in {"cstring*", "cstring*this"}

    mentions = [
        (index, instruction)
        for index, instruction in enumerate(candidate.instructions)
        if any(
            token in _cc_cfg._instruction_operand(instruction).casefold()
            for token in control_tokens
        )
    ]
    if not mentions:
        return {}
    invocations = [
        (index, instruction)
        for index, instruction in enumerate(candidate.instructions)
        if _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        and index not in candidate.local_control_flow_indices
    ]
    control_window = invocations[
        _cc_catalog.WOL_CONTROL_FIRST_ORDINAL:_cc_catalog.WOL_CONTROL_END_ORDINAL
    ]
    cstring_window = invocations[
        _cc_catalog.WOL_CSTRING_FIRST_ORDINAL:_cc_catalog.WOL_CSTRING_END_ORDINAL
    ]
    if (
        len(invocations) != _cc_catalog.WOL_CALL_POPULATION
        or len(control_window) != len(_cc_catalog.WOL_CONTROL_CANDIDATE_ORDER)
        or tuple(
            _cc_cfg._instruction_operand(instruction).strip()
            for _index, instruction in control_window
        )
        != _cc_catalog.WOL_CONTROL_CANDIDATE_ORDER
        or mentions != control_window
        or any(
            _cc_cfg._instruction_mnemonic(instruction) != "call"
            for _index, instruction in control_window
        )
        or len(cstring_window) != 3
        or tuple(
            _cc_cfg._instruction_operand(instruction).strip()
            for _index, instruction in cstring_window
        )
        != (_cc_catalog.WOL_CSTRING_CANDIDATE_SYMBOL,) * 3
        or any(
            _cc_cfg._instruction_mnemonic(instruction) != "call"
            for _index, instruction in cstring_window
        )
    ):
        raise ValueError(
            "WOL control provider bridge requires the exact complete ordered "
            "21-call candidate census, 17 direct VC5 control constructors, "
            "and three trailing CString default constructors"
        )

    caller_row = document.collection("symbols").get(
        _cc_catalog.WOL_CEDIT_CALLER_SYMBOL_ID
    )
    caller_trace = (
        caller_row.get("source_traceability")
        if isinstance(caller_row, Mapping)
        else None
    )
    caller_edges = (
        caller_trace.get("source_edges")
        if isinstance(caller_trace, Mapping)
        else None
    )
    caller_block = document.collection("physical_blocks").get(
        _cc_catalog.WOL_CEDIT_CALLER_BLOCK_ID
    )
    if (
        not isinstance(caller_row, Mapping)
        or caller_row.get("binary") != "recoil"
        or caller_row.get("kind") != "function"
        or caller_row.get("pipeline_class") != "authored"
        or caller_row.get("address") != _cc_catalog.WOL_CEDIT_CALLER_ADDRESS
        or caller_row.get("end_exclusive")
        != _cc_catalog.WOL_CEDIT_CALLER_END_EXCLUSIVE
        or caller_row.get("extent_state") != "known"
        or caller_row.get("size") != 0x240
        or caller_row.get("ownership_state") != "primary-owned"
        or caller_row.get("physical_block_id") != _cc_catalog.WOL_CEDIT_CALLER_BLOCK_ID
        or caller_row.get("verification_target_ids", []).count(
            _cc_catalog.WOL_CEDIT_TARGET_ID
        )
        != 1
        or not isinstance(caller_trace, Mapping)
        or caller_trace.get("state") != "resolved"
        or caller_trace.get("reason_code") is not None
        or caller_edges
        != [
            {
                "anchor_id": _cc_catalog.WOL_CEDIT_CALLER_ANCHOR_ID,
                "emission_context": {
                    "translation_unit": _cc_catalog.WOL_CEDIT_CALLER_SOURCE_PATH
                },
                "evidence_ids": [],
                "relation": "defines",
            }
        ]
        or indexes.by_address.get(_cc_catalog.WOL_CEDIT_CALLER_ADDRESS)
        != _cc_catalog.WOL_CEDIT_CALLER_INDEX_IDENTITY
        or _cc_catalog.WOL_CEDIT_CALLER_INDEX_IDENTITY in indexes.provider_ids
        or not isinstance(caller_block, Mapping)
        or caller_block.get("binary") != "recoil"
        or caller_block.get("row_kind") != "physical-source-block"
        or caller_block.get("start") != "0x43cf90"
        or caller_block.get("end_exclusive") != "0x442220"
        or caller_block.get("contribution_kind") != "authored"
        or caller_block.get("agent_source_path")
        != _cc_catalog.WOL_CEDIT_CALLER_SOURCE_PATH
        or caller_block.get("source_path") != _cc_catalog.WOL_CEDIT_CALLER_SOURCE_PATH
        or not isinstance(caller_block.get("contribution_ids"), list)
        or caller_block["contribution_ids"].count(
            _cc_catalog.WOL_CEDIT_CALLER_SYMBOL_ID
        )
        != 1
    ):
        raise ValueError(
            "WOL CEdit provider bridge requires the exact governed authored "
            "caller and resolved source provenance"
        )

    target_row = document.collection("symbols").get(
        _cc_catalog.WOL_CEDIT_TARGET_SYMBOL_ID
    )
    target_block = document.collection("physical_blocks").get(
        _cc_catalog.WOL_CEDIT_TARGET_BLOCK_ID
    )
    target_mapping = (
        target_block.get("mapping")
        if isinstance(target_block, Mapping)
        else None
    )
    if (
        not isinstance(target_row, Mapping)
        or target_row.get("binary") != "recoil"
        or target_row.get("kind") != "function"
        or target_row.get("pipeline_class") != "non-authored"
        or target_row.get("authored_order_role") != "non-authored"
        or target_row.get("address") != _cc_catalog.WOL_CEDIT_TARGET_ADDRESS
        or target_row.get("end_exclusive")
        != _cc_catalog.WOL_CEDIT_TARGET_END_EXCLUSIVE
        or target_row.get("extent_state") != "known"
        or target_row.get("size") != 6
        or target_row.get("navigation_name") != _cc_catalog.WOL_CEDIT_RETAIL_NAME
        or target_row.get("output_section_id") != "recoil:section:.text"
        or target_row.get("physical_block_id") != _cc_catalog.WOL_CEDIT_TARGET_BLOCK_ID
        or target_row.get("disposition") != "unresolved"
        or target_row.get("ownership_state") != "unresolved"
        or indexes.by_address.get(_cc_catalog.WOL_CEDIT_TARGET_ADDRESS)
        != _cc_catalog.WOL_CEDIT_TARGET_IDENTITY
        or _cc_catalog.WOL_CEDIT_TARGET_IDENTITY not in indexes.provider_ids
        or not isinstance(target_block, Mapping)
        or target_block.get("binary") != "recoil"
        or target_block.get("row_kind") != "physical-source-block"
        or target_block.get("start") != "0x4c5a50"
        or target_block.get("end_exclusive") != "0x4c5eb8"
        or target_block.get("contribution_kind") != "provider"
        or target_block.get("agent_source_path")
        != "provider:mfc42-tail-import-thunks"
        or target_block.get("source_path")
        != "provider:mfc42-tail-import-thunks"
        or not isinstance(target_block.get("contribution_ids"), list)
        or target_block["contribution_ids"].count(
            _cc_catalog.WOL_CEDIT_TARGET_SYMBOL_ID
        )
        != 1
        or not isinstance(target_mapping, Mapping)
        or target_mapping.get("status") != "provider-boundary"
    ):
        raise ValueError(
            "WOL CEdit provider bridge lacks the exact current reviewed MFC "
            "provider target"
        )

    retail_symbols = bridge_names.get(_cc_catalog.WOL_CEDIT_RETAIL_NAME)
    if (
        not isinstance(retail_symbols, Sequence)
        or isinstance(retail_symbols, (str, bytes))
        or len(retail_symbols) != 1
    ):
        raise ValueError(
            "WOL CEdit provider bridge requires one exact current retail CWnd "
            "constructor symbol"
        )
    retail_symbol = retail_symbols[0]
    retail_full_symbols = bridge_names.get(_cc_catalog.WOL_CEDIT_RETAIL_FULL_NAME)
    if (
        not isinstance(retail_full_symbols, Sequence)
        or isinstance(retail_full_symbols, (str, bytes))
        or len(retail_full_symbols) != 1
        or retail_full_symbols[0] is not retail_symbol
    ):
        raise ValueError(
            "WOL CEdit provider bridge requires the decorated raw and "
            "demangled full aliases to index one identical current import "
            "symbol"
        )
    candidate_aliases = {
        name.casefold()
        for name in (
            *_cc_catalog.WOL_CONTROL_CANDIDATE_SYMBOLS,
            *_cc_catalog.WOL_CONTROL_CANDIDATE_FULL_NAMES,
        )
    }
    candidate_name_collisions = [
        label
        for label, names in (
            ("Binary Ninja", bridge_names),
            ("candidate identity", indexes.by_candidate_name),
            ("candidate storage", indexes.storage_by_name),
        )
        if any(
            str(name).casefold() in candidate_aliases
            for name in names
        )
    ]
    if (
        getattr(retail_symbol, "address", "") != _cc_catalog.WOL_CEDIT_TARGET_ADDRESS
        or getattr(retail_symbol, "name", "") != _cc_catalog.WOL_CEDIT_RETAIL_NAME
        or getattr(retail_symbol, "raw_name", "") != _cc_catalog.WOL_CEDIT_RETAIL_NAME
        or getattr(retail_symbol, "full_name", "")
        != _cc_catalog.WOL_CEDIT_RETAIL_FULL_NAME
        or getattr(retail_symbol, "kind", "") != "import"
        or candidate_name_collisions
    ):
        raise ValueError(
            "WOL CEdit provider bridge has current-provider, target-address, "
            "or candidate-name collision drift"
        )

    cstring_target_row = document.collection("symbols").get(
        _cc_catalog.WOL_CSTRING_TARGET_SYMBOL_ID
    )
    cstring_bn_candidate_aliases = [
        name
        for name in bridge_names
        if cstring_candidate_alias(name)
    ]
    cstring_candidate_collisions = [
        label
        for label, names in (
            ("candidate identity", indexes.by_candidate_name),
            ("candidate storage", indexes.storage_by_name),
        )
        if any(cstring_candidate_alias(name) for name in names)
    ]
    if cstring_bn_candidate_aliases:
        cstring_candidate_collisions.append("Binary Ninja")
    cstring_thunk_routes = [
        thunk
        for thunk in ordinal_import_thunks
        if (
            thunk.provider_identity == _cc_catalog.WOL_CSTRING_TARGET_IDENTITY
            or thunk.thunk_address == _cc_catalog.WOL_CSTRING_TARGET_ADDRESS
            or thunk.retail_name == _cc_catalog.WOL_CSTRING_RETAIL_NAME
            or thunk.callable_symbol.casefold()
            == _cc_catalog.WOL_CSTRING_CANDIDATE_SYMBOL.casefold()
        )
    ]
    cstring_thunk = (
        cstring_thunk_routes[0]
        if len(cstring_thunk_routes) == 1
        else None
    )
    cstring_function_symbols = bridge_names.get(_cc_catalog.WOL_CSTRING_RETAIL_NAME)
    cstring_function_symbol = (
        cstring_function_symbols[0]
        if isinstance(cstring_function_symbols, Sequence)
        and not isinstance(cstring_function_symbols, (str, bytes))
        and len(cstring_function_symbols) == 1
        else None
    )
    if (
        not isinstance(cstring_target_row, Mapping)
        or cstring_target_row.get("binary") != "recoil"
        or cstring_target_row.get("kind") != "function"
        or cstring_target_row.get("pipeline_class") != "non-authored"
        or cstring_target_row.get("authored_order_role") != "non-authored"
        or cstring_target_row.get("address") != _cc_catalog.WOL_CSTRING_TARGET_ADDRESS
        or cstring_target_row.get("end_exclusive")
        != _cc_catalog.WOL_CSTRING_TARGET_END_EXCLUSIVE
        or cstring_target_row.get("extent_state") != "known"
        or cstring_target_row.get("size") != 6
        or cstring_target_row.get("navigation_name")
        != _cc_catalog.WOL_CSTRING_RETAIL_NAME
        or cstring_target_row.get("output_section_id")
        != "recoil:section:.text"
        or cstring_target_row.get("physical_block_id")
        != _cc_catalog.WOL_CEDIT_TARGET_BLOCK_ID
        or cstring_target_row.get("ownership_state") != "primary-owned"
        or indexes.by_address.get(_cc_catalog.WOL_CSTRING_TARGET_ADDRESS)
        != _cc_catalog.WOL_CSTRING_TARGET_IDENTITY
        or _cc_catalog.WOL_CSTRING_TARGET_IDENTITY not in indexes.provider_ids
        or target_block["contribution_ids"].count(
            _cc_catalog.WOL_CSTRING_TARGET_SYMBOL_ID
        )
        != 1
        or cstring_thunk is None
        or cstring_thunk.provider_identity != _cc_catalog.WOL_CSTRING_TARGET_IDENTITY
        or cstring_thunk.thunk_address != _cc_catalog.WOL_CSTRING_TARGET_ADDRESS
        or cstring_thunk.retail_name != _cc_catalog.WOL_CSTRING_RETAIL_NAME
        or cstring_thunk.callable_symbol != _cc_catalog.WOL_CSTRING_CANDIDATE_SYMBOL
        or cstring_thunk.iat_object_symbol
        != _cc_catalog.WOL_CSTRING_IAT_OBJECT_SYMBOL
        or cstring_thunk.import_dll.casefold() != "mfc42.dll"
        or cstring_thunk.import_ordinal != _cc_catalog.WOL_CSTRING_IMPORT_ORDINAL
        or not cstring_thunk.iat_identity.startswith("iat:ordinal:")
        or not cstring_thunk.iat_identity.endswith(
            f":{_cc_catalog.WOL_CSTRING_IMPORT_ORDINAL}"
        )
        or cstring_function_symbol is None
        or getattr(cstring_function_symbol, "address", "")
        != _cc_catalog.WOL_CSTRING_TARGET_ADDRESS
        or getattr(cstring_function_symbol, "name", "")
        != _cc_catalog.WOL_CSTRING_RETAIL_NAME
        or getattr(cstring_function_symbol, "raw_name", "")
        != _cc_catalog.WOL_CSTRING_RETAIL_NAME
        or getattr(cstring_function_symbol, "full_name", "") not in {
            "",
            _cc_catalog.WOL_CSTRING_RETAIL_NAME,
        }
        or getattr(cstring_function_symbol, "kind", "") != "function"
        or cstring_candidate_collisions
    ):
        raise ValueError(
            "WOL CString provider bridge requires one exact collision-free "
            "tracker provider, immutable ordinal-540 route, and BN function "
            "navigation representation"
        )

    expected_rows = [
        row
        for row in expected
        if row.get("target_identity") == _cc_catalog.WOL_CEDIT_TARGET_IDENTITY
    ]
    if (
        len(expected) != _cc_catalog.WOL_CALL_POPULATION
        or tuple(row.get("ordinal") for row in expected)
        != tuple(range(_cc_catalog.WOL_CALL_POPULATION))
        or expected_rows
        != list(
            expected[
                _cc_catalog.WOL_CONTROL_FIRST_ORDINAL:_cc_catalog.WOL_CONTROL_END_ORDINAL
            ]
        )
        or len(expected_rows) != len(_cc_catalog.WOL_CONTROL_CANDIDATE_ORDER)
        or any(
            row.get("form") != "call"
            or row.get("dispatch") != "direct"
            or row.get("identity_kind") != "provider"
            or row.get("storage_identity") != ""
            or row.get("slot_displacement") is not None
            or row.get("cleanup_bytes") is not None
            for row in expected_rows
        )
    ):
        raise ValueError(
            "WOL control provider bridge lacks the exact candidate-independent "
            "21-call retail census and ordinal-1-through-17 direct provider "
            "population"
        )

    expected_cstring_rows = list(
        expected[_cc_catalog.WOL_CSTRING_FIRST_ORDINAL:_cc_catalog.WOL_CSTRING_END_ORDINAL]
    )
    if (
        len(expected_cstring_rows) != 3
        or [
            row
            for row in expected
            if row.get("target_identity") == _cc_catalog.WOL_CSTRING_TARGET_IDENTITY
        ]
        != expected_cstring_rows
        or any(
            row.get("ordinal") != ordinal
            or row.get("form") != "call"
            or row.get("dispatch") != "direct"
            or row.get("identity_kind") != "provider"
            or row.get("target_identity") != _cc_catalog.WOL_CSTRING_TARGET_IDENTITY
            or row.get("storage_identity") != ""
            or row.get("slot_displacement") is not None
            or row.get("cleanup_bytes") is not None
            for ordinal, row in zip(
                range(_cc_catalog.WOL_CSTRING_FIRST_ORDINAL, _cc_catalog.WOL_CSTRING_END_ORDINAL),
                expected_cstring_rows,
            )
        )
    ):
        raise ValueError(
            "WOL CString provider bridge lacks the exact candidate-independent "
            "ordinal-18-through-20 direct provider population"
        )

    target = candidate.target
    target_functions = tuple(getattr(target, "functions", ()))
    translation_entries = tuple(
        getattr(target, "translation_unit_function_order", ())
    )
    wol_entries = [
        entry
        for entry in translation_entries
        if getattr(entry, "source_from", "")
        == _cc_catalog.WOL_CEDIT_CALLER_SOURCE_PATH
    ]
    progress_entries = [
        entry
        for entry in translation_entries
        if getattr(entry, "source_from", "")
        == _cc_catalog.WOL_PROGRESS_DIALOG_SOURCE_PATH
    ]
    wol_entry = wol_entries[0] if len(wol_entries) == 1 else None
    selected_rows = [
        row
        for row in getattr(wol_entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.WOL_CEDIT_CALLER_ADDRESS
    ]
    target_paths = tuple(getattr(target, "order_edit_paths", ()))
    if (
        target is None
        or getattr(target, "name", "") != _cc_catalog.WOL_CEDIT_TARGET_NAME
        or getattr(target, "target_binary", "") != "recoil"
        or Path(str(getattr(target, "manifest_path", ""))).resolve()
        != _cc_catalog.WOL_CEDIT_TARGET_MANIFEST.resolve()
        or getattr(target, "source_from", "")
        != _cc_catalog.WOL_CEDIT_CALLER_SOURCE_PATH
        or target_paths.count(_cc_catalog.WOL_CEDIT_CALLER_SOURCE_PATH) != 1
        or not bool(
            getattr(target, "check_translation_unit_function_order", False)
        )
        or target_functions
        or len(translation_entries) != 2
        or len(wol_entries) != 1
        or len(progress_entries) != 1
        or getattr(wol_entry, "inventory_only", True)
        or getattr(wol_entry, "order_scope", "") != "authored"
        or not tuple(getattr(wol_entry, "functions", ()))
        or getattr(progress_entries[0], "inventory_only", True)
        or getattr(progress_entries[0], "order_scope", "") != "authored"
        or not tuple(getattr(progress_entries[0], "functions", ()))
        or len(selected_rows) != 1
    ):
        raise ValueError(
            "WOL CEdit provider bridge requires the exact selected two-TU WOL "
            "source target, authored scopes, and manifest"
        )
    selected_row = selected_rows[0]
    if (
        getattr(selected_row, "symbol", "") != _cc_catalog.WOL_CEDIT_CALLER_SYMBOL
        or getattr(selected_row, "symbol_regex", None) is not None
        or getattr(selected_row, "name", "")
        != "WestwoodOnlineUpgradeDialog::Constructor"
        or getattr(selected_row, "pipeline_class", "") != "authored"
        or getattr(selected_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(selected_row, "required_presence", False))
        or not bool(getattr(selected_row, "full_order_gate", False))
    ):
        raise ValueError(
            "WOL CEdit provider bridge requires the exact selected authored "
            "caller row"
        )

    registered_target = document.collection("verification_targets").get(
        _cc_catalog.WOL_CEDIT_TARGET_ID
    )
    registration = (
        registered_target.get("registration")
        if isinstance(registered_target, Mapping)
        else None
    )
    registered_addresses = (
        registered_target.get("registered_addresses")
        if isinstance(registered_target, Mapping)
        else None
    )
    registered_translation_entries = (
        registration.get("translation_unit_function_order")
        if isinstance(registration, Mapping)
        else None
    )
    registered_wol_entries = [
        entry
        for entry in registered_translation_entries or ()
        if isinstance(entry, Mapping)
        and entry.get("source_from") == _cc_catalog.WOL_CEDIT_CALLER_SOURCE_PATH
    ]
    registered_progress_entries = [
        entry
        for entry in registered_translation_entries or ()
        if isinstance(entry, Mapping)
        and entry.get("source_from") == _cc_catalog.WOL_PROGRESS_DIALOG_SOURCE_PATH
    ]
    registered_tu_paths = tuple(
        entry.get("source_from")
        for entry in registered_translation_entries or ()
        if isinstance(entry, Mapping)
    )
    registered_wol_entry = (
        registered_wol_entries[0]
        if len(registered_wol_entries) == 1
        else None
    )
    registered_tu_rows = (
        registered_wol_entry.get("functions")
        if isinstance(registered_wol_entry, Mapping)
        else None
    )
    registered_tu_caller_rows = [
        row
        for row in registered_tu_rows or ()
        if isinstance(row, Mapping)
        and row.get("address") == _cc_catalog.WOL_CEDIT_CALLER_ADDRESS
    ]
    registered_all_tu_caller_rows = [
        row
        for entry in registered_translation_entries or ()
        if isinstance(entry, Mapping)
        for row in entry.get("functions", ())
        if isinstance(row, Mapping)
        and row.get("address") == _cc_catalog.WOL_CEDIT_CALLER_ADDRESS
    ]
    linked_intervals = (
        registration.get("linked_function_intervals")
        if isinstance(registration, Mapping)
        else None
    )
    linked_interval = (
        linked_intervals[0]
        if isinstance(linked_intervals, list)
        and len(linked_intervals) == 1
        and isinstance(linked_intervals[0], Mapping)
        else None
    )
    linked_functions = (
        linked_interval.get("functions")
        if isinstance(linked_interval, Mapping)
        else None
    )
    linked_caller_rows = [
        row
        for row in linked_functions or ()
        if isinstance(row, Mapping)
        and row.get("address") == _cc_catalog.WOL_CEDIT_CALLER_ADDRESS
    ]
    registration_rows = (
        registered_tu_caller_rows[0],
        linked_caller_rows[0],
    ) if (
        len(registered_tu_caller_rows) == 1
        and len(linked_caller_rows) == 1
    ) else ()
    if (
        not isinstance(registered_target, Mapping)
        or registered_target.get("binary") != "recoil"
        or registered_target.get("kind") != "vc5"
        or registered_target.get("name") != _cc_catalog.WOL_CEDIT_TARGET_NAME
        or not isinstance(registered_addresses, list)
        or registered_addresses.count(_cc_catalog.WOL_CEDIT_CALLER_ADDRESS) != 1
        or not isinstance(registration, Mapping)
        or registration.get("binary") != "recoil"
        or registration.get("manifest_path")
        != _cc_catalog.WOL_CEDIT_TARGET_MANIFEST.relative_to(REPO_ROOT).as_posix()
        or registration.get("source_from")
        != _cc_catalog.WOL_CEDIT_CALLER_SOURCE_PATH
        or registration.get("check_translation_unit_function_order") is not True
        or registration.get("function_order_scope") != "authored"
        or not isinstance(registered_translation_entries, list)
        or len(registered_translation_entries) != 2
        or not all(
            isinstance(entry, Mapping)
            for entry in registered_translation_entries
        )
        or registered_tu_paths
        != (
            _cc_catalog.WOL_CEDIT_CALLER_SOURCE_PATH,
            _cc_catalog.WOL_PROGRESS_DIALOG_SOURCE_PATH,
        )
        or len(registered_wol_entries) != 1
        or len(registered_progress_entries) != 1
        or registered_wol_entry.get("inventory_only") is not False
        or registered_wol_entry.get("order_scope") != "authored"
        or not isinstance(registered_tu_rows, list)
        or not registered_tu_rows
        or not all(isinstance(row, Mapping) for row in registered_tu_rows)
        or registered_progress_entries[0].get("inventory_only") is not False
        or registered_progress_entries[0].get("order_scope") != "authored"
        or not isinstance(
            registered_progress_entries[0].get("functions"), list
        )
        or not registered_progress_entries[0].get("functions")
        or not all(
            isinstance(row, Mapping)
            for row in registered_progress_entries[0].get("functions", ())
        )
        or len(registered_tu_caller_rows) != 1
        or len(registered_all_tu_caller_rows) != 1
        or not isinstance(linked_intervals, list)
        or len(linked_intervals) != 1
        or not isinstance(linked_interval, Mapping)
        or "source_from" in linked_interval
        or not isinstance(linked_functions, list)
        or len(linked_functions) != 111
        or not all(isinstance(row, Mapping) for row in linked_functions)
        or len(linked_caller_rows) != 1
        or any(
            row.get("symbol") != _cc_catalog.WOL_CEDIT_CALLER_SYMBOL
            or row.get("symbol_regex") is not None
            or row.get("name")
            != "WestwoodOnlineUpgradeDialog::Constructor"
            or row.get("pipeline_class") != "authored"
            or row.get("authored_order_role") != "authored-body"
            or row.get("required_presence") is not True
            or row.get("full_order_gate") is not True
            for row in registration_rows
        )
    ):
        raise ValueError(
            "WOL CEdit provider bridge requires the synchronized exact WOL "
            "verification registration"
        )

    definition = candidate.caller_definition
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    undefined_external_functions = (
        getattr(definition, "undefined_external_functions", ())
        if definition is not None
        else ()
    )
    required_constructor_symbols = (
        *_cc_catalog.WOL_CONTROL_CANDIDATE_SYMBOLS,
        _cc_catalog.WOL_CSTRING_CANDIDATE_SYMBOL,
    )
    cstring_external_aliases = [
        name
        for name in undefined_external_functions
        if cstring_candidate_alias(name)
    ]
    competing_constructor_definitions = tuple(
        name
        for name in (
            *(
                getattr(definition, "defined_external_functions", ())
                if definition is not None
                else ()
            ),
            *(
                getattr(definition, "undefined_external_data", ())
                if definition is not None
                else ()
            ),
            *(
                getattr(definition, "defined_external_data", ())
                if definition is not None
                else ()
            ),
        )
        if name in required_constructor_symbols
    )
    if (
        definition is None
        or definition.symbol != _cc_catalog.WOL_CEDIT_CALLER_SYMBOL
        or len(definition.data) != _cc_catalog.WOL_CEDIT_CANDIDATE_SIZE
        or len(definition.data) != len(definition.relocation_mask)
        or definition.section_start != 0
        or definition.section_end != _cc_catalog.WOL_CEDIT_CANDIDATE_SIZE
        or len(definition.relocations)
        != _cc_catalog.WOL_CEDIT_CANDIDATE_RELOCATION_COUNT
        or any(
            undefined_external_functions.count(symbol) != 1
            for symbol in required_constructor_symbols
        )
        or cstring_external_aliases != [_cc_catalog.WOL_CSTRING_CANDIDATE_SYMBOL]
        or _cc_catalog.WOL_CSTRING_IAT_OBJECT_SYMBOL in undefined_external_functions
        or competing_constructor_definitions
    ):
        raise ValueError(
            "WOL control provider bridge requires the complete 432-byte, "
            "26-relocation caller COFF definition and each exact control "
            "and CString constructor once in the TU undefined-function "
            "inventory, with no constructor misclassification or alias"
        )

    references = [
        relocation
        for relocation in definition.relocations
        if any(
            token in relocation.symbol_name.casefold()
            for token in control_tokens
        )
    ]
    references_by_offset: dict[int, list[CoffRelocation]] = {}
    for reference in references:
        references_by_offset.setdefault(reference.offset, []).append(reference)
    expected_reference_offsets: set[int] = set()
    malformed_control_calls = False
    for (instruction_index, instruction), expected_symbol in zip(
        control_window,
        _cc_catalog.WOL_CONTROL_CANDIDATE_ORDER,
    ):
        call_offset = (
            offsets[instruction_index]
            if len(offsets) > instruction_index
            else -1
        )
        relocation_offset = call_offset + 1
        call_references = references_by_offset.get(relocation_offset, [])
        expected_reference_offsets.add(relocation_offset)
        if (
            call_offset < 0
            or tuple(instruction.bytes) != ("e8", "00", "00", "00", "00")
            or len(call_references) != 1
            or call_references[0].symbol_name != expected_symbol
            or call_references[0].type != IMAGE_REL_I386_REL32
            or relocation_offset < 1
            or relocation_offset + 4 > len(definition.data)
            or definition.data[call_offset] != 0xE8
            or struct.unpack_from(
                "<I", definition.data, relocation_offset
            )[0]
            != 0
            or definition.relocation_mask[call_offset]
            or not all(
                definition.relocation_mask[index]
                for index in range(relocation_offset, relocation_offset + 4)
            )
        ):
            malformed_control_calls = True
            break
    if (
        malformed_control_calls
        or len(references) != len(_cc_catalog.WOL_CONTROL_CANDIDATE_ORDER)
        or set(references_by_offset) != expected_reference_offsets
    ):
        raise ValueError(
            "WOL control provider bridge requires 17 exact one-to-one "
            "zero-addend fully masked E8 REL32 COFF call relocations"
        )

    cstring_references = [
        relocation
        for relocation in definition.relocations
        if "cstring" in relocation.symbol_name.casefold()
    ]
    cstring_references_by_offset = {
        relocation.offset: relocation
        for relocation in cstring_references
    }
    malformed_cstring_calls = (
        len(cstring_references) != 3
        or len(cstring_references_by_offset) != 3
        or set(cstring_references_by_offset)
        != set(_cc_catalog.WOL_CSTRING_RELOCATION_OFFSETS)
    )
    if not malformed_cstring_calls:
        for (
            (instruction_index, instruction),
            relocation_offset,
        ) in zip(cstring_window, _cc_catalog.WOL_CSTRING_RELOCATION_OFFSETS):
            call_offset = (
                offsets[instruction_index]
                if len(offsets) > instruction_index
                else -1
            )
            reference = cstring_references_by_offset[relocation_offset]
            if (
                call_offset != relocation_offset - 1
                or tuple(instruction.bytes)
                != ("e8", "00", "00", "00", "00")
                or reference.symbol_name != _cc_catalog.WOL_CSTRING_CANDIDATE_SYMBOL
                or reference.type != IMAGE_REL_I386_REL32
                or relocation_offset + 4 > len(definition.data)
                or definition.data[call_offset] != 0xE8
                or struct.unpack_from(
                    "<I", definition.data, relocation_offset
                )[0]
                != 0
                or definition.relocation_mask[call_offset]
                or not all(
                    definition.relocation_mask[index]
                    for index in range(
                        relocation_offset, relocation_offset + 4
                    )
                )
            ):
                malformed_cstring_calls = True
                break
    if malformed_cstring_calls:
        raise ValueError(
            "WOL CString provider bridge requires three exact one-to-one "
            "zero-addend fully masked E8 REL32 caller relocations at "
            "offsets 327, 343, and 359"
        )

    result = {
        symbol: _cc_catalog.WOL_CEDIT_TARGET_IDENTITY
        for symbol in _cc_catalog.WOL_CONTROL_CANDIDATE_SYMBOLS
    }
    result[_cc_catalog.WOL_CSTRING_CANDIDATE_SYMBOL] = _cc_catalog.WOL_CSTRING_TARGET_IDENTITY
    provisional = _cc_extraction.extract_invocation_contract(
        candidate.instructions,
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge_names=bridge_names,
        compiler_generated_bridges=result,
        local_control_flow_indices=candidate.local_control_flow_indices,
        local_control_flow_targets=candidate.local_control_flow_targets,
    )
    candidate_rows = [
        row
        for row in provisional
        if row.get("target_identity") == _cc_catalog.WOL_CEDIT_TARGET_IDENTITY
    ]
    candidate_cstring_rows = [
        row
        for row in provisional
        if row.get("target_identity") == _cc_catalog.WOL_CSTRING_TARGET_IDENTITY
    ]
    if (
        len(provisional) != _cc_catalog.WOL_CALL_POPULATION
        or len(candidate_rows) != len(_cc_catalog.WOL_CONTROL_CANDIDATE_ORDER)
        or candidate_rows
        != provisional[
            _cc_catalog.WOL_CONTROL_FIRST_ORDINAL:_cc_catalog.WOL_CONTROL_END_ORDINAL
        ]
        or candidate_rows != expected_rows
        or candidate_cstring_rows
        != provisional[_cc_catalog.WOL_CSTRING_FIRST_ORDINAL:_cc_catalog.WOL_CSTRING_END_ORDINAL]
        or candidate_cstring_rows != expected_cstring_rows
        or provisional != list(expected)
    ):
        raise ValueError(
            "WOL constructor provider bridge candidate 21-call population, "
            "ordered control/CString ordinals, form, or provider identity "
            "drifts from retail"
        )
    return result


def _require_wol_category_a_caller_provenance(
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[Mapping[str, Any], Mapping[str, Any]] | None:
    """Return the exact governed Category-A WOL caller and its specification."""

    caller_spec = _cc_catalog.WOL_CATEGORY_A_CALLER_SPECS.get(caller_identity)
    if caller_spec is None:
        return None
    if (
        normalize_address(caller_start) != caller_spec["address"]
        or normalize_address(caller_end_exclusive)
        != caller_spec["end_exclusive"]
    ):
        return None
    caller_row = document.collection("symbols").get(
        str(caller_spec["symbol_id"])
    )
    trace = (
        caller_row.get("source_traceability")
        if isinstance(caller_row, Mapping)
        else None
    )
    if (
        not isinstance(caller_row, Mapping)
        or caller_row.get("binary") != "recoil"
        or caller_row.get("kind") != "function"
        or caller_row.get("pipeline_class") != "authored"
        or caller_row.get("address") != caller_spec["address"]
        or caller_row.get("end_exclusive") != caller_spec["end_exclusive"]
        or caller_row.get("extent_state") != "known"
        or caller_row.get("size") != caller_spec["size"]
        or caller_row.get("ownership_state") != "primary-owned"
        or caller_row.get("physical_block_id")
        != caller_spec["physical_block_id"]
        or indexes.by_address.get(str(caller_spec["address"]))
        != caller_identity
        or caller_identity in indexes.provider_ids
        or not isinstance(trace, Mapping)
        or trace.get("state") != "resolved"
        or trace.get("reason_code") is not None
        or trace.get("source_edges")
        != [
            {
                "anchor_id": caller_spec["source_anchor"],
                "emission_context": {
                    "translation_unit": caller_spec["source_path"]
                },
                "evidence_ids": [],
                "relation": "defines",
            }
        ]
    ):
        raise ValueError(
            "WOL Category-A bridge requires the exact canonical authored "
            "caller identity, extent, and resolved source trace"
        )
    _require_wol_chkstk_caller_target_provenance(
        candidate,
        document=document,
        caller_row=caller_row,
        caller_spec=caller_spec,
    )
    return caller_row, caller_spec


def _wol_category_a_candidate_offset_map(
    candidate: CandidateAssembly,
) -> dict[int, Instruction]:
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    counts: dict[int, int] = {}
    for offset in offsets:
        if offset is not None:
            counts[int(offset)] = counts.get(int(offset), 0) + 1
    return {
        int(offset): instruction
        for offset, instruction in zip(offsets, candidate.instructions)
        if offset is not None and counts.get(int(offset)) == 1
    }


def _wol_category_a_direct_candidate_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
) -> dict[str, str]:
    """Bind the two exact Category-A direct-call spellings."""

    if caller_identity not in {
        "symbol:recoil:function:0x43fa90",
        "symbol:recoil:function:0x441750",
    }:
        return {}
    candidate_symbol = (
        _cc_catalog.WOL_TIME_RESET_CANDIDATE_SYMBOL
        if caller_identity == "symbol:recoil:function:0x43fa90"
        else _cc_catalog.WOL_CCOMBOBOX_CANDIDATE_SYMBOL
    )
    mentions = [
        instruction
        for instruction in candidate.instructions
        if candidate_symbol.casefold()
        in _cc_cfg._instruction_operand(instruction).casefold()
    ]
    if not mentions:
        return {}
    provenance = _require_wol_category_a_caller_provenance(
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    if provenance is None:
        return {}
    _caller_row, caller_spec = provenance
    definition = candidate.caller_definition
    if definition is None or definition.symbol != caller_spec["symbol"]:
        raise ValueError(
            "WOL Category-A direct bridge requires the exact complete caller "
            "COFF definition"
        )

    if caller_identity == "symbol:recoil:function:0x43fa90":
        expected_row = {
            "ordinal": 0,
            "form": "call",
            "dispatch": "direct",
            "identity_kind": "direct",
            "target_identity": _cc_catalog.WOL_TIME_RESET_TARGET_IDENTITY,
            "storage_identity": "",
            "slot_displacement": None,
            "cleanup_bytes": None,
        }
        if not expected or dict(expected[0]) != expected_row:
            raise ValueError(
                "WOL Time::Reset bridge requires exact retail ordinal-zero "
                "direct no-cleanup truth"
            )
        target_row = document.collection("symbols").get(
            _cc_catalog.WOL_TIME_RESET_TARGET_SYMBOL_ID
        )
        target_block = document.collection("physical_blocks").get(
            _cc_catalog.WOL_TIME_RESET_TARGET_BLOCK_ID
        )
        target_owner = document.collection("owners").get(
            _cc_catalog.WOL_TIME_RESET_TARGET_OWNER_ID
        )
        trace = (
            target_row.get("source_traceability")
            if isinstance(target_row, Mapping)
            else None
        )
        primary_rows = [
            relationship
            for relationship in (
                target_owner.get("relationships", ())
                if isinstance(target_owner, Mapping)
                else ()
            )
            if isinstance(relationship, Mapping)
            and relationship.get("kind") == "primary-function"
            and relationship.get("symbol_id")
            == _cc_catalog.WOL_TIME_RESET_TARGET_SYMBOL_ID
        ]
        if (
            not isinstance(target_row, Mapping)
            or target_row.get("binary") != "recoil"
            or target_row.get("kind") != "function"
            or target_row.get("pipeline_class") != "authored"
            or target_row.get("address") != _cc_catalog.WOL_TIME_RESET_TARGET_ADDRESS
            or target_row.get("end_exclusive")
            != _cc_catalog.WOL_TIME_RESET_TARGET_END_EXCLUSIVE
            or target_row.get("extent_state") != "known"
            or target_row.get("size") != 0x60
            or target_row.get("navigation_name") != "Time::Reset"
            or target_row.get("ownership_state") != "primary-owned"
            or target_row.get("physical_block_id")
            != _cc_catalog.WOL_TIME_RESET_TARGET_BLOCK_ID
            or indexes.by_address.get(_cc_catalog.WOL_TIME_RESET_TARGET_ADDRESS)
            != _cc_catalog.WOL_TIME_RESET_TARGET_IDENTITY
            or _cc_catalog.WOL_TIME_RESET_TARGET_IDENTITY in indexes.provider_ids
            or not isinstance(trace, Mapping)
            or trace.get("state") != "resolved"
            or trace.get("reason_code") is not None
            or trace.get("source_edges")
            != [
                {
                    "anchor_id": "recoil:anchor:gamezrecoil-time-time-time-reset",
                    "emission_context": {
                        "translation_unit": "src/GameZRecoil/zTime/Time.cpp"
                    },
                    "evidence_ids": [],
                    "relation": "defines",
                }
            ]
            or not isinstance(target_block, Mapping)
            or target_block.get("binary") != "recoil"
            or target_block.get("row_kind") != "physical-source-block"
            or target_block.get("start") != _cc_catalog.WOL_TIME_RESET_TARGET_ADDRESS
            or target_block.get("end_exclusive") != "0x4a5780"
            or target_block.get("contribution_kind") != "authored"
            or target_block.get("agent_source_path")
            != "src/GameZRecoil/zTime/Time.cpp"
            or target_block.get("source_path")
            != "src/GameZRecoil/zTime/Time.cpp"
            or target_block.get("contribution_ids", []).count(
                _cc_catalog.WOL_TIME_RESET_TARGET_SYMBOL_ID
            )
            != 1
            or not isinstance(target_owner, Mapping)
            or target_owner.get("binary") != "recoil"
            or target_owner.get("kind") != "source-file"
            or target_owner.get("provider_state") == "accepted"
            or target_owner.get("source_paths")
            != [
                "src/GameZRecoil/zTime/Time.cpp",
                "src/GameZRecoil/zTime/Time.h",
            ]
            or len(primary_rows) != 1
            or primary_rows[0].get("address")
            != _cc_catalog.WOL_TIME_RESET_TARGET_ADDRESS
        ):
            raise ValueError(
                "WOL Time::Reset bridge requires the exact reviewed authored "
                "target, block, owner, and case-sensitive source paths"
            )
        target_identity = _cc_catalog.WOL_TIME_RESET_TARGET_IDENTITY
    else:
        expected_rows = [
            dict(row)
            for row in expected
            if row.get("ordinal") in {1, 2}
        ]
        required_rows = [
            {
                "ordinal": ordinal,
                "form": "call",
                "dispatch": "direct",
                "identity_kind": "provider",
                "target_identity": _cc_catalog.WOL_CEDIT_TARGET_IDENTITY,
                "storage_identity": "",
                "slot_displacement": None,
                "cleanup_bytes": None,
            }
            for ordinal in (1, 2)
        ]
        if expected_rows != required_rows:
            raise ValueError(
                "WOL CComboBox bridge requires the exact two neighboring "
                "retail CWnd-provider constructor rows"
            )
        target_row = document.collection("symbols").get(
            _cc_catalog.WOL_CEDIT_TARGET_SYMBOL_ID
        )
        target_block = document.collection("physical_blocks").get(
            _cc_catalog.WOL_CEDIT_TARGET_BLOCK_ID
        )
        mapping = (
            target_block.get("mapping")
            if isinstance(target_block, Mapping)
            else None
        )
        provider_symbols = bridge_names.get(_cc_catalog.WOL_CEDIT_RETAIL_NAME)
        provider_symbol = (
            provider_symbols[0]
            if isinstance(provider_symbols, Sequence)
            and not isinstance(provider_symbols, (str, bytes))
            and len(provider_symbols) == 1
            else None
        )
        full_symbols = bridge_names.get(_cc_catalog.WOL_CEDIT_RETAIL_FULL_NAME)
        if (
            not isinstance(target_row, Mapping)
            or target_row.get("binary") != "recoil"
            or target_row.get("kind") != "function"
            or target_row.get("pipeline_class") != "non-authored"
            or target_row.get("authored_order_role") != "non-authored"
            or target_row.get("address") != _cc_catalog.WOL_CEDIT_TARGET_ADDRESS
            or target_row.get("end_exclusive")
            != _cc_catalog.WOL_CEDIT_TARGET_END_EXCLUSIVE
            or target_row.get("extent_state") != "known"
            or target_row.get("size") != 6
            or target_row.get("navigation_name") != _cc_catalog.WOL_CEDIT_RETAIL_NAME
            or target_row.get("physical_block_id")
            != _cc_catalog.WOL_CEDIT_TARGET_BLOCK_ID
            or indexes.by_address.get(_cc_catalog.WOL_CEDIT_TARGET_ADDRESS)
            != _cc_catalog.WOL_CEDIT_TARGET_IDENTITY
            or _cc_catalog.WOL_CEDIT_TARGET_IDENTITY not in indexes.provider_ids
            or not isinstance(target_block, Mapping)
            or target_block.get("binary") != "recoil"
            or target_block.get("row_kind") != "physical-source-block"
            or target_block.get("start") != "0x4c5a50"
            or target_block.get("end_exclusive") != "0x4c5eb8"
            or target_block.get("contribution_kind") != "provider"
            or target_block.get("agent_source_path")
            != "provider:mfc42-tail-import-thunks"
            or target_block.get("source_path")
            != "provider:mfc42-tail-import-thunks"
            or target_block.get("contribution_ids", []).count(
                _cc_catalog.WOL_CEDIT_TARGET_SYMBOL_ID
            )
            != 1
            or not isinstance(mapping, Mapping)
            or mapping.get("status") != "provider-boundary"
            or provider_symbol is None
            or not isinstance(full_symbols, Sequence)
            or isinstance(full_symbols, (str, bytes))
            or len(full_symbols) != 1
            or full_symbols[0] is not provider_symbol
            or getattr(provider_symbol, "address", "")
            != _cc_catalog.WOL_CEDIT_TARGET_ADDRESS
            or getattr(provider_symbol, "name", "")
            != _cc_catalog.WOL_CEDIT_RETAIL_NAME
            or getattr(provider_symbol, "raw_name", "")
            != _cc_catalog.WOL_CEDIT_RETAIL_NAME
            or getattr(provider_symbol, "full_name", "")
            != _cc_catalog.WOL_CEDIT_RETAIL_FULL_NAME
            or getattr(provider_symbol, "kind", "") != "import"
        ):
            raise ValueError(
                "WOL CComboBox bridge requires the exact reviewed CWnd "
                "provider row, block, and identical BN import aliases"
            )
        target_identity = _cc_catalog.WOL_CEDIT_TARGET_IDENTITY

    folded = candidate_symbol.casefold()
    collision_rows = [
        (label, name, identity)
        for label, names in (
            ("candidate identity", indexes.by_candidate_name),
            ("candidate storage", indexes.storage_by_name),
        )
        for name, identity in names.items()
        if str(name).casefold() == folded and identity != target_identity
    ]
    bn_collisions = [
        row
        for name, rows in bridge_names.items()
        if str(name).casefold() == folded
        for row in (
            rows
            if isinstance(rows, Sequence)
            and not isinstance(rows, (str, bytes))
            else (rows,)
        )
        if normalize_address(str(getattr(row, "address", "0x0")))
        != (
            _cc_catalog.WOL_TIME_RESET_TARGET_ADDRESS
            if target_identity == _cc_catalog.WOL_TIME_RESET_TARGET_IDENTITY
            else _cc_catalog.WOL_CEDIT_TARGET_ADDRESS
        )
    ]
    if collision_rows or bn_collisions:
        raise ValueError(
            "WOL Category-A direct bridge rejects candidate/provider/storage "
            "identity collision drift"
        )

    _cc_callable_identity._require_unique_exact_candidate_direct_external_call_row(
        candidate,
        candidate_symbol=candidate_symbol,
    )
    offsets = _wol_category_a_candidate_offset_map(candidate)
    if caller_identity == "symbol:recoil:function:0x441750":
        neighbors = {
            0x28: _cc_catalog.WOL_CONFIG_CDIALOG_CANDIDATE_SYMBOL,
            0x48: _cc_catalog.WOL_CONFIG_CEDIT_CANDIDATE_SYMBOL,
        }
        if any(
            offsets.get(offset) is None
            or _cc_cfg._instruction_mnemonic(offsets[offset]) != "call"
            or _cc_cfg._instruction_operand(offsets[offset]).strip() != symbol
            for offset, symbol in neighbors.items()
        ):
            raise ValueError(
                "WOL CComboBox bridge requires its exact CDialog/CEdit "
                "candidate call neighbors"
            )
    return {candidate_symbol: target_identity}


def _wol_category_a_atoi_register_storage_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, str],
    dict[str, CandidateExactIatRegisterLoadProof],
]:
    """Publish the exact WOL encoded-query ``atoi`` IAT-register spelling."""
    from _recoil.call_contract.records import CandidateExactIatRegisterLoadProof

    if caller_identity not in {
        "symbol:recoil:function:0x4407e0",
        "symbol:recoil:function:0x440a30",
    }:
        return {}, {}
    mentions = [
        instruction
        for instruction in candidate.instructions
        if _cc_catalog.WOL_ATOI_CANDIDATE_SYMBOL.casefold()
        in _cc_cfg._instruction_operand(instruction).casefold()
    ]
    if not mentions:
        return {}, {}
    provenance = _require_wol_category_a_caller_provenance(
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    if provenance is None:
        return {}, {}
    _caller_row, caller_spec = provenance
    required_rows = [
        {
            "ordinal": ordinal,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "iat",
            "target_identity": _cc_catalog.WOL_ATOI_STORAGE_IDENTITY,
            "storage_identity": _cc_catalog.WOL_ATOI_STORAGE_IDENTITY,
            "slot_displacement": None,
            "cleanup_bytes": 4,
        }
        for ordinal in _cc_catalog.WOL_ATOI_CALL_ORDINALS
    ]
    if [dict(expected[ordinal]) for ordinal in _cc_catalog.WOL_ATOI_CALL_ORDINALS] != required_rows:
        raise ValueError(
            "WOL atoi bridge requires the six exact retail indirect-IAT "
            "ordinals and caller cleanup values"
        )
    canonical_names = [
        (name, identity)
        for name, identity in indexes.storage_by_name.items()
        if str(name).casefold() in {
            "atoi",
            _cc_catalog.WOL_ATOI_CANDIDATE_SYMBOL.casefold(),
        }
    ]
    if (
        indexes.storage_by_address.get(_cc_catalog.WOL_ATOI_STORAGE_ADDRESS)
        != _cc_catalog.WOL_ATOI_STORAGE_IDENTITY
        or indexes.storage_by_name.get("atoi") != _cc_catalog.WOL_ATOI_STORAGE_IDENTITY
        or any(identity != _cc_catalog.WOL_ATOI_STORAGE_IDENTITY for _name, identity in canonical_names)
        or any(
            str(name).casefold() == _cc_catalog.WOL_ATOI_CANDIDATE_SYMBOL.casefold()
            and identity != _cc_catalog.WOL_ATOI_STORAGE_IDENTITY
            for name, identity in indexes.by_candidate_name.items()
        )
        or _cc_catalog.WOL_ATOI_STORAGE_IDENTITY in indexes.provider_ids
    ):
        raise ValueError(
            "WOL atoi bridge requires one immutable canonical IAT slot/name "
            "identity without candidate/provider collision"
        )
    definition = candidate.caller_definition
    if (
        definition is None
        or definition.symbol != caller_spec["symbol"]
        or len(definition.data) != len(definition.relocation_mask)
    ):
        raise ValueError(
            "WOL atoi bridge requires the exact complete caller COFF definition"
        )
    offsets = _wol_category_a_candidate_offset_map(candidate)
    load = offsets.get(_cc_catalog.WOL_ATOI_LOAD_OFFSET)
    load_operands = (
        tuple(
            part.strip()
            for part in _cc_cfg._instruction_operand(load).split(",", 1)
        )
        if load is not None
        else ()
    )
    transfers = [
        offset
        for offset, instruction in offsets.items()
        if _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        and _cc_cfg._instruction_operand(instruction).strip().casefold() == "edi"
    ]
    relocation_rows = [
        row
        for row in definition.relocations
        if row.symbol_name.casefold() == _cc_catalog.WOL_ATOI_CANDIDATE_SYMBOL.casefold()
    ]
    external_counts = (
        definition.undefined_external_functions.count(_cc_catalog.WOL_ATOI_CANDIDATE_SYMBOL),
        definition.undefined_external_data.count(_cc_catalog.WOL_ATOI_CANDIDATE_SYMBOL),
        definition.defined_external_functions.count(_cc_catalog.WOL_ATOI_CANDIDATE_SYMBOL),
        definition.defined_external_data.count(_cc_catalog.WOL_ATOI_CANDIDATE_SYMBOL),
    )
    if (
        len(mentions) != 1
        or load is None
        or _cc_cfg._instruction_mnemonic(load) != "mov"
        or len(load_operands) != 2
        or load_operands[0].casefold() != "edi"
        or _cc_targets._exact_memory_expression(load_operands[1])
        != _cc_catalog.WOL_ATOI_CANDIDATE_SYMBOL
        or tuple(value.lower() for value in load.bytes)
        != ("8b", "3d", "00", "00", "00", "00")
        or definition.data[_cc_catalog.WOL_ATOI_LOAD_OFFSET : _cc_catalog.WOL_ATOI_LOAD_OFFSET + 6]
        != b"\x8b\x3d\0\0\0\0"
        or tuple(sorted(transfers)) != _cc_catalog.WOL_ATOI_CALL_OFFSETS
        or any(
            tuple(value.lower() for value in offsets[offset].bytes)
            != ("ff", "d7")
            for offset in _cc_catalog.WOL_ATOI_CALL_OFFSETS
        )
        or len(relocation_rows) != 1
        or relocation_rows[0].offset != _cc_catalog.WOL_ATOI_RELOCATION_OFFSET
        or relocation_rows[0].type != IMAGE_REL_I386_DIR32
        or struct.unpack_from(
            "<I", definition.data, _cc_catalog.WOL_ATOI_RELOCATION_OFFSET
        )[0]
        != 0
        or not all(
            definition.relocation_mask[index]
            for index in range(
                _cc_catalog.WOL_ATOI_RELOCATION_OFFSET,
                _cc_catalog.WOL_ATOI_RELOCATION_OFFSET + 4,
            )
        )
        or sum(external_counts) != 1
        or external_counts[:2] not in {(1, 0), (0, 1)}
    ):
        raise ValueError(
            "WOL atoi bridge requires the exact EDI MOV/DIR32 definition, "
            "six FF D7 transfers, and unique undefined external class"
        )
    return (
        {_cc_catalog.WOL_ATOI_CANDIDATE_SYMBOL: _cc_catalog.WOL_ATOI_STORAGE_IDENTITY},
        {
            normalize_address(_cc_catalog.WOL_ATOI_LOAD_OFFSET): (
                CandidateExactIatRegisterLoadProof(
                    definition_offset=normalize_address(
                        _cc_catalog.WOL_ATOI_LOAD_OFFSET
                    ),
                    destination="edi",
                    object_symbol=_cc_catalog.WOL_ATOI_CANDIDATE_SYMBOL,
                    identity=_cc_catalog.WOL_ATOI_STORAGE_IDENTITY,
                    transfer_offsets=tuple(
                        normalize_address(offset)
                        for offset in _cc_catalog.WOL_ATOI_CALL_OFFSETS
                    ),
                )
            )
        },
    )


def _require_wol_chkstk_caller_target_provenance(
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_row: Mapping[str, Any],
    caller_spec: Mapping[str, Any],
) -> None:
    """Require the exact reviewed two-TU WOL caller/target registration."""

    source_path = str(caller_spec["source_path"])
    progress_source_path = _cc_catalog.WOL_PROGRESS_DIALOG_SOURCE_PATH
    target = candidate.target
    target_functions = tuple(getattr(target, "functions", ()))
    target_entries = tuple(
        getattr(target, "translation_unit_function_order", ())
    )
    target_paths = tuple(getattr(target, "order_edit_paths", ()))
    target_entry_paths = tuple(
        getattr(entry, "source_from", "") for entry in target_entries
    )
    target_wol_entry = (
        target_entries[0]
        if target_entry_paths == (source_path, progress_source_path)
        else None
    )
    target_caller_rows = [
        row
        for entry in target_entries
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == caller_spec["address"]
    ]

    registered_target = document.collection("verification_targets").get(
        str(caller_spec["target_id"])
    )
    registration = (
        registered_target.get("registration")
        if isinstance(registered_target, Mapping)
        else None
    )
    registered_addresses = (
        registered_target.get("registered_addresses")
        if isinstance(registered_target, Mapping)
        else None
    )
    registered_entries = (
        registration.get("translation_unit_function_order")
        if isinstance(registration, Mapping)
        else None
    )
    registered_entry_paths = tuple(
        entry.get("source_from")
        for entry in registered_entries or ()
        if isinstance(entry, Mapping)
    )
    registered_wol_entry = (
        registered_entries[0]
        if isinstance(registered_entries, list)
        and registered_entry_paths == (source_path, progress_source_path)
        else None
    )
    registered_caller_rows = [
        row
        for entry in registered_entries or ()
        if isinstance(entry, Mapping)
        for row in entry.get("functions", ())
        if isinstance(row, Mapping)
        and row.get("address") == caller_spec["address"]
    ]
    linked_intervals = (
        registration.get("linked_function_intervals")
        if isinstance(registration, Mapping)
        else None
    )
    linked_interval = (
        linked_intervals[0]
        if isinstance(linked_intervals, list)
        and len(linked_intervals) == 1
        and isinstance(linked_intervals[0], Mapping)
        else None
    )
    linked_functions = (
        linked_interval.get("functions")
        if isinstance(linked_interval, Mapping)
        else None
    )
    linked_caller_rows = [
        row
        for row in linked_functions or ()
        if isinstance(row, Mapping)
        and row.get("address") == caller_spec["address"]
    ]
    caller_block = document.collection("physical_blocks").get(
        str(caller_spec["physical_block_id"])
    )
    expected_manifest = Path(caller_spec["target_manifest"])
    if (
        target is None
        or getattr(target, "name", "") != caller_spec["target_name"]
        or getattr(target, "target_binary", "") != "recoil"
        or Path(str(getattr(target, "manifest_path", ""))).resolve()
        != expected_manifest.resolve()
        or getattr(target, "source_from", "") != source_path
        or not bool(
            getattr(target, "check_translation_unit_function_order", False)
        )
        or target_functions
        or len(target_entries) != 2
        or target_entry_paths != (source_path, progress_source_path)
        or target_paths.count(source_path) != 1
        or target_paths.count(progress_source_path) != 1
        or target_wol_entry is None
        or any(
            getattr(entry, "inventory_only", True)
            or getattr(entry, "order_scope", "") != "authored"
            or not tuple(getattr(entry, "functions", ()))
            for entry in target_entries
        )
        or len(target_caller_rows) != 1
    ):
        raise ValueError(
            "MSVC __chkstk WOL caller requires the exact selected two-TU "
            "source target and manifest"
        )

    target_caller_row = target_caller_rows[0]
    if (
        getattr(target_caller_row, "symbol", "") != caller_spec["symbol"]
        or getattr(target_caller_row, "symbol_regex", None) is not None
        or getattr(target_caller_row, "name", "")
        != caller_spec["target_row_name"]
        or getattr(target_caller_row, "pipeline_class", "") != "authored"
        or getattr(target_caller_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(target_caller_row, "required_presence", False))
        or not bool(getattr(target_caller_row, "full_order_gate", False))
    ):
        raise ValueError(
            "MSVC __chkstk WOL caller requires its exact selected authored "
            "target row"
        )

    registration_rows = (
        registered_caller_rows[0],
        linked_caller_rows[0],
    ) if (
        len(registered_caller_rows) == 1
        and len(linked_caller_rows) == 1
    ) else ()
    if (
        not isinstance(registered_target, Mapping)
        or registered_target.get("binary") != "recoil"
        or registered_target.get("kind") != "vc5"
        or registered_target.get("name") != caller_spec["target_name"]
        or not isinstance(registered_addresses, list)
        or registered_addresses.count(caller_spec["address"]) != 1
        or not isinstance(registration, Mapping)
        or registration.get("binary") != "recoil"
        or registration.get("manifest_path")
        != expected_manifest.relative_to(REPO_ROOT).as_posix()
        or registration.get("source_from") != source_path
        or registration.get("check_translation_unit_function_order") is not True
        or registration.get("function_order_scope") != "authored"
        or registration.get("function_addresses") != []
        or tuple(registration.get("order_edit_paths", ())) != target_paths
        or not isinstance(registered_entries, list)
        or len(registered_entries) != 2
        or registered_entry_paths != (source_path, progress_source_path)
        or registered_wol_entry is None
        or any(
            not isinstance(entry, Mapping)
            or entry.get("inventory_only") is not False
            or entry.get("order_scope") != "authored"
            or not isinstance(entry.get("functions"), list)
            or not entry.get("functions")
            or not all(
                isinstance(row, Mapping)
                for row in entry.get("functions", ())
            )
            for entry in registered_entries
        )
        or len(registered_caller_rows) != 1
        or not isinstance(linked_interval, Mapping)
        or "source_from" in linked_interval
        or not isinstance(linked_functions, list)
        or len(linked_functions) != 111
        or not all(isinstance(row, Mapping) for row in linked_functions)
        or len(linked_caller_rows) != 1
        or any(
            row.get("symbol") != caller_spec["symbol"]
            or row.get("symbol_regex") is not None
            or row.get("name") != caller_spec["target_row_name"]
            or row.get("pipeline_class") != "authored"
            or row.get("authored_order_role") != "authored-body"
            or row.get("required_presence") is not True
            or row.get("full_order_gate") is not True
            or row.get("authored_order_gate") is not True
            or row.get("authored_relative_order_gate") is not True
            or row.get("icf_fold_status") != ""
            or row.get("logical_identity_key") != ""
            for row in registration_rows
        )
    ):
        raise ValueError(
            "MSVC __chkstk WOL caller requires the synchronized exact two-TU "
            "verification registration"
        )

    if (
        not isinstance(caller_block, Mapping)
        or caller_block.get("binary") != "recoil"
        or caller_block.get("row_kind") != "physical-source-block"
        or caller_block.get("start") != "0x43cf90"
        or caller_block.get("end_exclusive") != "0x442220"
        or caller_block.get("contribution_kind") != "authored"
        or caller_block.get("agent_source_path") != source_path
        or caller_block.get("source_path") != source_path
        or not isinstance(caller_block.get("contribution_ids"), list)
        or caller_block["contribution_ids"].count(caller_spec["symbol_id"])
        != 1
        or tuple(caller_row.get("verification_target_ids", ()))
        != tuple(caller_spec["verification_target_ids"])
    ):
        raise ValueError(
            "MSVC __chkstk WOL caller requires the exact reviewed source "
            "block and target membership"
        )
