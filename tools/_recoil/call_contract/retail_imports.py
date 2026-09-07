"""Recoil call-contract retail imports evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import listing as _cc_listing
from _recoil.call_contract import storage_identity as _cc_storage_identity

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        CurrentIatStoragePackage,
        IdentityIndexes,
        ProviderNamedImportThunk,
        ProviderOrdinalImportThunk,
        ProviderPeNamedImportThunk,
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedLoopVptrStorageBridge,
    )

import re
import struct
from dataclasses import replace
from pathlib import Path
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import IMAGE_REL_I386_REL32, Instruction
from _recoil.commands.provider_target_mutation import (
    _retail_import_targets,
    retail_import_target,
)
from _recoil.commands.vc5_verify import load_manifest
from _recoil.lib.binja import BinaryNinjaBridge
from _recoil.lib.progress import (
    AUTHORED_ORDER_DIMENSIONS,
    ProgressDocument,
    ProgressError,
    address_value,
    normalize_address,
)


def _retail_direct_import_thunk_indexes(
    retail_instructions: Sequence[Instruction],
    *,
    indexes: IdentityIndexes,
    thunks: Sequence[
        ProviderNamedImportThunk
        | ProviderPeNamedImportThunk
        | ProviderOrdinalImportThunk
    ],
    preserved_provider_identities: frozenset[str] = frozenset(),
) -> IdentityIndexes:
    """Bind exact E8/E9 call sites through already-proven FF25 thunks.

    Thunk discovery owns the raw ``FF 25 [IAT]`` bytes and immutable import
    package.  This comparison-scoped join owns only the direct rel32 transfer
    into that thunk.  A named import thunk denotes the imported IAT callable,
    while an ordinal-only thunk denotes its accepted physical provider body;
    the latter is the only exact identity shared with the candidate's natural
    callable-symbol relocation.  Both selections remain scoped to this exact
    retail instruction and retain the governed provider/IAT package proof.
    """
    from _recoil.call_contract.records import (
        ProviderOrdinalImportThunk,
        ProviderPeNamedImportThunk,
    )

    if any(
        not identity.startswith("provider:")
        or identity not in indexes.provider_ids
        for identity in preserved_provider_identities
    ):
        raise ValueError(
            "direct import-thunk preserved identities lack exact governed "
            "provider identities"
        )

    thunks_by_address: dict[
        str,
        list[
            ProviderNamedImportThunk
            | ProviderPeNamedImportThunk
            | ProviderOrdinalImportThunk
        ],
    ] = {}
    for thunk in thunks:
        try:
            thunk_address = normalize_address(thunk.thunk_address)
        except (AttributeError, ProgressError, TypeError, ValueError) as exc:
            raise ValueError("direct import-thunk proof is malformed") from exc
        named_pe = isinstance(thunk, ProviderPeNamedImportThunk)
        if (
            not thunk.provider_identity
            or indexes.by_address.get(thunk_address) != thunk.provider_identity
            or thunk.provider_identity not in indexes.provider_ids
            or not thunk.iat_identity.startswith("iat:")
            or (
                not named_pe
                and indexes.storage_by_name.get(thunk.iat_object_symbol)
                != thunk.iat_identity
            )
            or (
                named_pe
                and (
                    normalize_address(thunk.iat_address) != thunk.iat_address
                    or thunk.iat_identity != f"iat:{thunk.import_name}"
                    or not thunk.import_dll
                    or not thunk.import_name
                )
            )
        ):
            raise ValueError(
                f"direct import thunk {thunk_address} lacks its exact "
                "provider/IAT proof"
            )
        thunks_by_address.setdefault(thunk_address, []).append(thunk)

    reviewed = dict(indexes.reviewed_direct_import_thunk_by_call_site)
    exact_local = dict(
        indexes.reviewed_exact_local_import_thunk_by_call_site
    )
    for instruction in retail_instructions:
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        if mnemonic not in {"call", "jmp"}:
            continue
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            continue
        expected_opcode = 0xE8 if mnemonic == "call" else 0xE9
        source_address = _cc_cfg._source_instruction_address(instruction)
        if (
            len(body) != 5
            or body[0] != expected_opcode
            or not source_address
        ):
            continue
        target_address = normalize_address(
            address_value(source_address)
            + 5
            + struct.unpack_from("<i", body, 1)[0]
        )
        matches = thunks_by_address.get(target_address, [])
        if not matches:
            continue
        if len(matches) != 1:
            raise ValueError(
                f"direct transfer at {source_address} resolves to ambiguous "
                f"import thunks at {target_address}"
            )
        thunk = matches[0]
        call_site = normalize_address(source_address)
        preserve_provider = (
            thunk.provider_identity in preserved_provider_identities
        )
        target_identity = (
            thunk.provider_identity
            if preserve_provider or isinstance(thunk, ProviderOrdinalImportThunk)
            else thunk.iat_identity
        )
        value = (target_address, target_identity)
        prior = reviewed.get(call_site)
        prior_local_value = (target_address, thunk.iat_identity)
        if (
            prior not in {None, value}
            and not (preserve_provider and prior == prior_local_value)
        ):
            raise ValueError(
                f"direct import-thunk call site {call_site} has conflicting "
                "immutable identities"
            )
        exact_local_prior = exact_local.get(call_site)
        if preserve_provider:
            if exact_local_prior not in {None, prior_local_value}:
                raise ValueError(
                    f"preserved provider call site {call_site} conflicts with "
                    "its exact-local import-thunk selector"
                )
            exact_local.pop(call_site, None)
        reviewed[call_site] = value
    return replace(
        indexes,
        reviewed_direct_import_thunk_by_call_site=reviewed,
        reviewed_exact_local_import_thunk_by_call_site=exact_local,
    )


def _retail_local_named_import_thunk_indexes(
    retail_instructions: Sequence[Instruction],
    *,
    document: ProgressDocument,
    indexes: IdentityIndexes,
    bridge: BinaryNinjaBridge,
    caller_start: str | None = None,
    retail_import_targets: Sequence[Any] | None = None,
    precomposed_call_sites: frozenset[str] = frozenset(),
) -> IdentityIndexes:
    """Join exact local non-authored FF25 thunks to immutable named imports.

    Some retail translation-unit blocks contain a local CRT thunk whose
    tracker row is deliberately unresolved rather than provider-accepted.
    That unresolved ownership must not suppress the already-deterministic call
    identity: an exact rel32 transfer, one known non-authored function extent,
    a six-byte ``FF 25 [IAT]`` body followed only by padding, and one immutable
    PE named-import tuple compose the IAT identity without promoting the local
    row to a provider.  Ordinals, authored rows, ambiguous imports, executable
    body tails, and existing identity collisions remain fail-closed.
    """

    symbols = document.collection("symbols")
    if any(
        normalize_address(call_site) != call_site
        or call_site
        not in indexes.reviewed_direct_import_thunk_by_call_site
        for call_site in precomposed_call_sites
    ):
        raise ValueError(
            "precomposed local import-thunk call sites lack exact reviewed "
            "direct-thunk identities"
        )
    supplied_targets = tuple(
        retail_import_targets
        if retail_import_targets is not None
        else _retail_import_targets(_cc_catalog.DEFAULT_REFERENCE)[0]
    )

    def reviewed_dplay_ordinal_route(
        call_site: str,
        target_address: str,
    ) -> bool:
        """Recognize only the reviewed six-byte ordinal thunk boundary."""

        if call_site != "0x48be33" or target_address != "0x4c63ea":
            return False
        provider_identity = indexes.by_address.get(target_address, "")
        expected_iat = "iat:ordinal:10:DPLAYX.dll:4"
        import_rows = [
            row
            for row in supplied_targets
            if getattr(row, "address", "") == "0x4cc054"
        ]
        indexed_iat = indexes.storage_by_address.get("0x4cc054", "")
        if (
            not provider_identity.startswith("provider:")
            or provider_identity not in indexes.provider_ids
            or indexes.reviewed_direct_import_thunk_by_call_site.get(call_site)
            != (target_address, provider_identity)
            or indexed_iat not in {"", expected_iat}
            or len(import_rows) != 1
            or getattr(import_rows[0], "dll", "").casefold()
            != "dplayx.dll"
            or getattr(import_rows[0], "import_name", "") != "#4"
            or getattr(import_rows[0], "import_ordinal", None) != 4
            or _cc_cfg._hexdump_bytes(bridge.hexdump(target_address, 6))
            != bytes.fromhex("ff 25 54 c0 4c 00")
            or _cc_cfg._hexdump_bytes(bridge.hexdump("0x4cc054", 4))
            != bytes.fromhex("0c 8c 7c 00")
        ):
            raise ValueError(
                "reviewed DPLAYX ordinal precomposition conflicts with its "
                "exact six-byte thunk lineage"
            )
        return True

    candidates: list[tuple[str, str, str]] = []
    runtime_addresses = (
        _cc_cfg._instruction_runtime_addresses(
            retail_instructions,
            source="bn",
            caller_start=address_value(caller_start),
        )
        if caller_start is not None
        else tuple(None for _instruction in retail_instructions)
    )
    for instruction, runtime_address in zip(
        retail_instructions,
        runtime_addresses,
    ):
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        if mnemonic not in {"call", "jmp"}:
            continue
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            continue
        source_address = (
            normalize_address(runtime_address)
            if runtime_address is not None
            else _cc_cfg._source_instruction_address(instruction)
        )
        if source_address in precomposed_call_sites:
            continue
        opcode = 0xE8 if mnemonic == "call" else 0xE9
        if len(body) != 5 or body[0] != opcode or not source_address:
            continue
        target_address = normalize_address(
            address_value(source_address)
            + 5
            + struct.unpack_from("<i", body, 1)[0]
        )
        matching_symbols = [
            (str(symbol_id), row)
            for symbol_id, row in symbols.items()
            if isinstance(row, Mapping)
            and row.get("address") == target_address
            and row.get("binary") == "recoil"
            and row.get("kind") == "function"
        ]
        if len(matching_symbols) != 1:
            continue
        symbol_id, symbol = matching_symbols[0]
        if (
            symbol.get("ownership_state") != "unresolved"
            or symbol.get("disposition") != "unresolved"
            or symbol.get("pipeline_class") != "non-authored"
            or symbol.get("authored_order_role") != "non-authored"
        ):
            continue
        if reviewed_dplay_ordinal_route(source_address, target_address):
            # The registered unresolved symbol still has a stale oversized
            # extent.  Immutable ordinal ownership proves only the exact
            # six-byte thunk; adjacent real storage at 0x4c63f0 is not tail
            # padding and is deliberately outside this named-import path.
            continue
        try:
            end_exclusive = normalize_address(
                str(symbol.get("end_exclusive", ""))
            )
        except ProgressError:
            continue
        size = address_value(end_exclusive) - address_value(target_address)
        if (
            symbol.get("extent_state") != "known"
            or symbol.get("output_section_id") != "recoil:section:.text"
            or symbol.get("size") != size
            or size < 6
        ):
            raise ValueError(
                f"local import thunk {target_address} lacks one exact known "
                "registered text extent"
            )
        # Activation belongs to the immutable thunk shape, not merely to an
        # unresolved non-authored rel32 target.  Ordinary compiler/STL
        # provider bodies share that tracker classification and must retain
        # their normal physical provider identity.
        prefix_extent = _cc_cfg._hexdump_bytes(bridge.hexdump(target_address, size))
        if len(prefix_extent) != size:
            raise ValueError(
                f"local import-thunk prefilter at {target_address} has "
                "truncated immutable bytes"
            )
        prefix = prefix_extent[:6]
        if prefix[:2] != b"\xff\x25":
            continue
        candidates.append(
            (normalize_address(source_address), target_address, symbol_id)
        )

    # Some BN assembly renderings retain a friendly operand but omit the raw
    # rel32 bytes from the parsed row.  A friendly name alone is never enough:
    # recover only when it selects one exact unresolved local function, then
    # re-read the five immutable retail bytes and require their decoded target
    # to be that same registered address.
    candidate_sites = {row[0] for row in candidates}
    eligible_by_name: dict[str, list[tuple[str, Mapping[str, Any]]]] = {}
    for symbol_id, symbol in symbols.items():
        if not isinstance(symbol, Mapping):
            continue
        name = symbol.get("navigation_name")
        if (
            not isinstance(name, str)
            or not name
            or symbol.get("binary") != "recoil"
            or symbol.get("kind") != "function"
            or symbol.get("ownership_state") != "unresolved"
            or symbol.get("disposition") != "unresolved"
            or symbol.get("pipeline_class") != "non-authored"
            or symbol.get("authored_order_role") != "non-authored"
            or symbol.get("extent_state") != "known"
            or symbol.get("output_section_id") != "recoil:section:.text"
        ):
            continue
        eligible_by_name.setdefault(name, []).append((str(symbol_id), symbol))
    for instruction, runtime_address in zip(
        retail_instructions,
        runtime_addresses,
    ):
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        source_address = (
            normalize_address(runtime_address)
            if runtime_address is not None
            else _cc_cfg._source_instruction_address(instruction)
        )
        if mnemonic not in {"call", "jmp"} or not source_address:
            continue
        call_site = normalize_address(source_address)
        if call_site in candidate_sites:
            continue
        friendly_name = re.sub(
            r"^(?:near|far)\s+(?:ptr\s+)?",
            "",
            _cc_cfg._instruction_operand(instruction).strip(),
            flags=re.IGNORECASE,
        )
        matches = eligible_by_name.get(friendly_name, [])
        if len(matches) != 1:
            continue
        symbol_id, symbol = matches[0]
        target_address = normalize_address(str(symbol.get("address", "")))
        if reviewed_dplay_ordinal_route(call_site, target_address):
            continue
        end_exclusive = normalize_address(
            str(symbol.get("end_exclusive", ""))
        )
        size = address_value(end_exclusive) - address_value(target_address)
        if symbol.get("size") != size or size < 6:
            raise ValueError(
                f"local import thunk {target_address} lacks one exact known "
                "registered text extent"
            )
        body = _cc_cfg._hexdump_bytes(bridge.hexdump(call_site, 5))
        opcode = 0xE8 if mnemonic == "call" else 0xE9
        if len(body) != 5 or body[0] != opcode:
            raise ValueError(
                f"local import-thunk selector {call_site} lacks one exact "
                f"retail rel32 {mnemonic.upper()}"
            )
        decoded_target = normalize_address(
            address_value(call_site)
            + 5
            + struct.unpack_from("<i", body, 1)[0]
        )
        if decoded_target != target_address:
            raise ValueError(
                f"local import-thunk selector {call_site} decoded "
                f"{decoded_target}, expected {target_address}"
            )
        prefix_extent = _cc_cfg._hexdump_bytes(bridge.hexdump(target_address, size))
        if len(prefix_extent) != size:
            raise ValueError(
                f"local import-thunk prefilter at {target_address} has "
                "truncated immutable bytes"
            )
        prefix = prefix_extent[:6]
        if prefix[:2] != b"\xff\x25":
            continue
        candidates.append((call_site, target_address, symbol_id))
        candidate_sites.add(call_site)
    if not candidates:
        return indexes

    targets_by_address: dict[str, list[Any]] = {}
    routes_by_name: dict[str, set[tuple[str, str, str]]] = {}
    for target in supplied_targets:
        raw_address = getattr(target, "address", "")
        import_name = getattr(target, "import_name", None)
        import_dll = getattr(target, "dll", None)
        import_ordinal = getattr(target, "import_ordinal", None)
        try:
            iat_address = normalize_address(str(raw_address))
        except ProgressError as exc:
            raise ValueError(
                "local import-thunk composition received a malformed "
                "immutable IAT address"
            ) from exc
        if (
            not isinstance(import_name, str)
            or not import_name
            or not isinstance(import_dll, str)
            or not import_dll
        ):
            raise ValueError(
                "local import-thunk composition received a malformed "
                "immutable import tuple"
            )
        targets_by_address.setdefault(iat_address, []).append(target)
        if import_ordinal is None and not import_name.startswith("#"):
            routes_by_name.setdefault(import_name, set()).add(
                (iat_address, import_dll, import_name)
            )

    reviewed = dict(indexes.reviewed_direct_import_thunk_by_call_site)
    exact_local = dict(
        indexes.reviewed_exact_local_import_thunk_by_call_site
    )
    for call_site, target_address, symbol_id in candidates:
        symbol = symbols[symbol_id]
        size = int(symbol["size"])
        extent = _cc_cfg._hexdump_bytes(bridge.hexdump(target_address, size))
        rows = tuple(
            _cc_listing.parse_assembly(bridge.assembly(target_address), source="bn")
        )
        row_addresses = _cc_cfg._instruction_runtime_addresses(
            rows,
            source="bn",
            caller_start=address_value(target_address),
        )
        if len(extent) != size or extent[:2] != b"\xff\x25":
            raise ValueError(
                f"local import thunk {target_address} registered extent "
                f"does not contain an exact FF25 body"
            )
        shape_failures = []
        if any(value not in {0x90, 0xCC} for value in extent[6:]):
            shape_failures.append("non-padding registered tail")
        if len(rows) != 1:
            shape_failures.append(f"assembly row count {len(rows)}")
        if row_addresses != (address_value(target_address),):
            shape_failures.append("assembly coordinate drift")
        if rows and bytes(int(value, 16) for value in rows[0].bytes) != extent[:6]:
            shape_failures.append("assembly/immutable byte drift")
        if rows and _cc_cfg._instruction_mnemonic(rows[0]) != "jmp":
            shape_failures.append("non-JMP FF25 rendering")
        if shape_failures:
            raise ValueError(
                f"local import thunk {target_address} rejects "
                + ", ".join(shape_failures)
            )
        iat_address = normalize_address(
            struct.unpack_from("<I", extent, 2)[0]
        )
        matches = targets_by_address.get(iat_address, [])
        if len(matches) != 1:
            raise ValueError(
                f"local import thunk {target_address} IAT operand does not "
                "join one immutable import"
            )
        match = matches[0]
        import_name = str(getattr(match, "import_name"))
        import_dll = str(getattr(match, "dll"))
        if getattr(match, "import_ordinal", None) is not None:
            # The comparison-scoped ordinal producer owns this exact route.
            # Do not misclassify it as a malformed named import.
            continue
        if import_name.startswith("#") or len(
            routes_by_name.get(import_name, set())
        ) != 1:
            raise ValueError(
                f"local import thunk {target_address} lacks one unique named "
                "immutable import route"
            )
        iat_bytes = _cc_cfg._hexdump_bytes(bridge.hexdump(iat_address, 4))
        if len(iat_bytes) != 4 or iat_bytes == b"\x00\x00\x00\x00":
            raise ValueError(
                f"local import thunk {target_address} lacks one exact "
                "four-byte IAT cell"
            )
        iat_identity = f"iat:{import_name}"
        existing_iat = indexes.storage_by_address.get(iat_address)
        if existing_iat not in {None, iat_identity}:
            raise ValueError(
                f"local import thunk {target_address} conflicts with current "
                "IAT identity"
            )
        value = (target_address, iat_identity)
        prior = reviewed.get(call_site)
        if prior not in {None, value}:
            raise ValueError(
                f"local import-thunk call site {call_site} has conflicting "
                "immutable identities"
            )
        reviewed[call_site] = value
        exact_prior = exact_local.get(call_site)
        if exact_prior not in {None, value}:
            raise ValueError(
                f"local import-thunk call site {call_site} has conflicting "
                "exact-local identities"
            )
        exact_local[call_site] = value
    return replace(
        indexes,
        reviewed_direct_import_thunk_by_call_site=reviewed,
        reviewed_exact_local_import_thunk_by_call_site=exact_local,
    )


def _ftol_import_thunk_retail_bridge(
    *,
    document: ProgressDocument,
    indexes: IdentityIndexes,
    bridge_by_address: Mapping[str, Any],
    bridge_by_name: Mapping[str, Any],
    bridge_data_rows: Sequence[Any],
    bridge: BinaryNinjaBridge,
) -> tuple[str, dict[str, list[Any]]]:
    thunk_symbol = bridge_by_name.get(_cc_catalog.MSVC_FTOL_RETAIL_THUNK_NAME)
    if thunk_symbol is None:
        return "", {}
    import_symbol = bridge_by_name.get(_cc_catalog.MSVC_FTOL_IMPORT_NAME)
    address_symbol = bridge_by_address.get(_cc_catalog.MSVC_FTOL_PROVIDER_ADDRESS)
    if (
        getattr(thunk_symbol, "address", "") != _cc_catalog.MSVC_FTOL_PROVIDER_ADDRESS
        or getattr(thunk_symbol, "name", "") != _cc_catalog.MSVC_FTOL_RETAIL_THUNK_NAME
        or getattr(thunk_symbol, "raw_name", "")
        != _cc_catalog.MSVC_FTOL_RETAIL_THUNK_NAME
        or getattr(thunk_symbol, "kind", "") != "function"
        or import_symbol is None
        or getattr(import_symbol, "address", "")
        != _cc_catalog.MSVC_FTOL_PROVIDER_ADDRESS
        or getattr(import_symbol, "name", "") != _cc_catalog.MSVC_FTOL_IMPORT_NAME
        or getattr(import_symbol, "raw_name", "") != _cc_catalog.MSVC_FTOL_IMPORT_NAME
        or getattr(import_symbol, "full_name", "") != _cc_catalog.MSVC_FTOL_IMPORT_NAME
        or getattr(import_symbol, "kind", "") != "import"
        or address_symbol is None
        or getattr(address_symbol, "address", "")
        != _cc_catalog.MSVC_FTOL_PROVIDER_ADDRESS
        or getattr(address_symbol, "name", "") != _cc_catalog.MSVC_FTOL_IMPORT_NAME
        or getattr(address_symbol, "kind", "") != "import"
        or _cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL in bridge_by_name
    ):
        raise ValueError(
            "MSVC _ftol import-thunk bridge lacks the exact current "
            "collocated retail function/import symbols"
        )

    provider_symbol = document.collection("symbols").get(
        _cc_catalog.MSVC_FTOL_PROVIDER_SYMBOL_ID
    )
    provider_block = document.collection("physical_blocks").get(
        _cc_catalog.MSVC_FTOL_PROVIDER_BLOCK_ID
    )
    if (
        not isinstance(provider_symbol, Mapping)
        or provider_symbol.get("address") != _cc_catalog.MSVC_FTOL_PROVIDER_ADDRESS
        or provider_symbol.get("end_exclusive") != "0x4c60b0"
        or provider_symbol.get("size") != 10
        or provider_symbol.get("extent_state") != "known"
        or provider_symbol.get("binary") != "recoil"
        or provider_symbol.get("kind") != "function"
        or provider_symbol.get("pipeline_class") != "non-authored"
        or provider_symbol.get("authored_order_role") != "non-authored"
        or provider_symbol.get("navigation_name")
        != _cc_catalog.MSVC_FTOL_RETAIL_THUNK_NAME
        or provider_symbol.get("output_section_id") != "recoil:section:.text"
        or provider_symbol.get("physical_block_id")
        != _cc_catalog.MSVC_FTOL_PROVIDER_BLOCK_ID
        or indexes.by_address.get(_cc_catalog.MSVC_FTOL_PROVIDER_ADDRESS)
        != _cc_catalog.MSVC_FTOL_PROVIDER_IDENTITY
        or _cc_catalog.MSVC_FTOL_PROVIDER_IDENTITY not in indexes.provider_ids
    ):
        raise ValueError(
            "MSVC _ftol import-thunk bridge lacks the exact governed "
            "non-authored provider symbol"
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
            isinstance(authored_order.get(key), Mapping)
            and authored_order[key].get("result") == "not-applicable"
            and authored_order[key].get("disposition") == "accepted"
            and authored_order[key].get("freshness") == "current"
            and authored_order[key].get("gating") is True
            and authored_order[key].get("validation_mode") == "live"
            for key in (
                "authored_linked_order",
                "authored_object_order",
                "block_precedence",
                "linked_identity_presence",
                "object_identity_presence",
            )
        )
    )
    if (
        not isinstance(provider_block, Mapping)
        or provider_block.get("binary") != "recoil"
        or provider_block.get("row_kind") != "physical-source-block"
        or provider_block.get("start") != "0x4c60a0"
        or provider_block.get("end_exclusive") != "0x4c60b0"
        or provider_block.get("contribution_kind") != "provider"
        or provider_block.get("agent_source_path")
        != "provider:msvc-crt-import-thunks"
        or provider_block.get("source_path")
        != "provider:msvc-crt-import-thunks"
        or provider_block.get("contribution_ids")
        != [
            "recoil:function:0x4c60a0",
            _cc_catalog.MSVC_FTOL_PROVIDER_SYMBOL_ID,
        ]
        or not isinstance(mapping, Mapping)
        or mapping.get("status") != "provider-boundary"
        or not current_not_applicable
    ):
        raise ValueError(
            "MSVC _ftol import-thunk bridge lacks the exact current "
            "provider-boundary block"
        )

    iat_packages = [
        package
        for package in _cc_storage_identity._current_tracker_iat_storage_packages(document)
        if package.address == _cc_catalog.MSVC_FTOL_IAT_ADDRESS
    ]
    iat_storage = document.collection("storage_contributions").get(
        f"recoil:storage:va:{_cc_catalog.MSVC_FTOL_IAT_ADDRESS}"
    )
    if (
        len(iat_packages) != 1
        or iat_packages[0].import_dll != "MSVCRT.dll"
        or iat_packages[0].import_name != _cc_catalog.MSVC_FTOL_IMPORT_NAME
        or iat_packages[0].object_symbol != _cc_catalog.MSVC_FTOL_IAT_OBJECT_SYMBOL
        or iat_packages[0].identity != _cc_catalog.MSVC_FTOL_IAT_STORAGE_IDENTITY
        or not isinstance(iat_storage, Mapping)
        or iat_storage.get("owner_ids") != [_cc_catalog.MSVC_FTOL_IAT_OWNER_ID]
        or indexes.by_address.get(_cc_catalog.MSVC_FTOL_IAT_ADDRESS)
        != _cc_catalog.MSVC_FTOL_IAT_PROVIDER_IDENTITY
        or _cc_catalog.MSVC_FTOL_IAT_PROVIDER_IDENTITY not in indexes.provider_ids
        or indexes.storage_by_address.get(_cc_catalog.MSVC_FTOL_IAT_ADDRESS)
        != _cc_catalog.MSVC_FTOL_IAT_STORAGE_IDENTITY
        or indexes.storage_by_name.get(_cc_catalog.MSVC_FTOL_IMPORT_NAME)
        != _cc_catalog.MSVC_FTOL_IAT_STORAGE_IDENTITY
        or indexes.storage_by_name.get(_cc_catalog.MSVC_FTOL_IAT_OBJECT_SYMBOL)
        != _cc_catalog.MSVC_FTOL_IAT_STORAGE_IDENTITY
    ):
        raise ValueError(
            "MSVC _ftol import-thunk bridge lacks the exact current "
            "registered MSVCRT _ftol IAT provider package"
        )

    iat_rows = [
        row
        for row in bridge_data_rows
        if getattr(row, "address", "") == _cc_catalog.MSVC_FTOL_IAT_ADDRESS
    ]
    lookup_rows = [
        row
        for row in bridge_data_rows
        if getattr(row, "address", "") == "0x4d8368"
    ]
    import_name_rows = [
        row
        for row in bridge_data_rows
        if getattr(row, "address", "") == "0x4d8580"
    ]
    if (
        len(iat_rows) != 1
        or getattr(iat_rows[0], "name", "") != _cc_catalog.MSVC_FTOL_IMPORT_NAME
        or getattr(iat_rows[0], "raw_name", "") != _cc_catalog.MSVC_FTOL_IMPORT_NAME
        or getattr(iat_rows[0], "size", 0) != 4
        or getattr(iat_rows[0], "type_text", "")
        != "int32_t (* const)(double value @ st0)"
        or len(lookup_rows) != 1
        or getattr(lookup_rows[0], "name", "")
        != "__import_lookup_table_3(MSVCRT:_ftol)"
        or getattr(lookup_rows[0], "size", 0) != 4
        or len(import_name_rows) != 1
        or getattr(import_name_rows[0], "name", "")
        != "__import_by_name_3(MSVCRT:_ftol)"
        or getattr(import_name_rows[0], "size", 0) != 8
    ):
        raise ValueError(
            "MSVC _ftol import-thunk bridge lacks the exact current "
            "MSVCRT import/IAT data"
        )
    thunk_bytes = _cc_cfg._hexdump_bytes(
        bridge.hexdump(_cc_catalog.MSVC_FTOL_PROVIDER_ADDRESS, 10)
    )
    if thunk_bytes != bytes.fromhex("ff 25 ac c5 4c 00 cc cc cc cc"):
        raise ValueError(
            "MSVC _ftol import-thunk bridge retail thunk bytes or IAT "
            "operand drifted"
        )
    return (
        _cc_catalog.MSVC_FTOL_PROVIDER_IDENTITY,
        {_cc_catalog.MSVC_FTOL_RETAIL_THUNK_NAME: [thunk_symbol]},
    )


def _gettickcount_named_import_thunk_retail_bridge(
    *,
    document: ProgressDocument,
    indexes: IdentityIndexes,
    bridge_by_address: Mapping[str, Any],
    bridge_by_name: Mapping[str, Any],
    bridge: BinaryNinjaBridge,
    reference: Path = _cc_catalog.DEFAULT_REFERENCE,
) -> tuple[ProviderNamedImportThunk | None, dict[str, list[Any]]]:
    """Prove the one fixed named KERNEL32 GetTickCount retail thunk."""
    from _recoil.call_contract.records import ProviderNamedImportThunk
    thunk_symbol = bridge_by_name.get(_cc_catalog.GETTICKCOUNT_RETAIL_THUNK_NAME)
    if thunk_symbol is None:
        return None, {}
    import_symbol = bridge_by_name.get(_cc_catalog.GETTICKCOUNT_IMPORT_NAME)
    address_symbol = bridge_by_address.get(_cc_catalog.GETTICKCOUNT_PROVIDER_ADDRESS)
    if (
        getattr(thunk_symbol, "address", "") != _cc_catalog.GETTICKCOUNT_PROVIDER_ADDRESS
        or getattr(thunk_symbol, "name", "") != _cc_catalog.GETTICKCOUNT_RETAIL_THUNK_NAME
        or getattr(thunk_symbol, "raw_name", "")
        != _cc_catalog.GETTICKCOUNT_RETAIL_THUNK_NAME
        or getattr(thunk_symbol, "full_name", "")
        not in {"", _cc_catalog.GETTICKCOUNT_RETAIL_THUNK_NAME}
        or getattr(thunk_symbol, "kind", "") != "function"
        or import_symbol is None
        or getattr(import_symbol, "address", "")
        != _cc_catalog.GETTICKCOUNT_PROVIDER_ADDRESS
        or getattr(import_symbol, "name", "") != _cc_catalog.GETTICKCOUNT_IMPORT_NAME
        or getattr(import_symbol, "raw_name", "")
        != _cc_catalog.GETTICKCOUNT_IMPORT_NAME
        or getattr(import_symbol, "full_name", "")
        != _cc_catalog.GETTICKCOUNT_IMPORT_NAME
        or getattr(import_symbol, "kind", "") != "import"
        or address_symbol is None
        or getattr(address_symbol, "address", "")
        != _cc_catalog.GETTICKCOUNT_PROVIDER_ADDRESS
        or getattr(address_symbol, "name", "")
        != _cc_catalog.GETTICKCOUNT_IMPORT_NAME
        or getattr(address_symbol, "kind", "") != "import"
        or bridge_by_name.get(_cc_catalog.GETTICKCOUNT_CANDIDATE_CALLABLE) is not None
        or bridge_by_name.get(_cc_catalog.GETTICKCOUNT_CANDIDATE_IMPORT_SYMBOL) is not None
    ):
        raise ValueError(
            "GetTickCount named import-thunk bridge lacks its exact distinct "
            "retail BN function name and address"
        )

    symbol = document.collection("symbols").get(
        _cc_catalog.GETTICKCOUNT_PROVIDER_SYMBOL_ID
    )
    owner = document.collection("owners").get(
        _cc_catalog.GETTICKCOUNT_PROVIDER_OWNER_ID
    )
    block = document.collection("physical_blocks").get(
        _cc_catalog.GETTICKCOUNT_PROVIDER_BLOCK_ID
    )
    expected_relationships = [
        {"address": _cc_catalog.GETTICKCOUNT_PROVIDER_ADDRESS, "kind": "anchor-address"},
        {
            "address": _cc_catalog.GETTICKCOUNT_PROVIDER_ADDRESS,
            "kind": "primary-function",
            "symbol_id": _cc_catalog.GETTICKCOUNT_PROVIDER_SYMBOL_ID,
        },
    ]
    if (
        not isinstance(symbol, Mapping)
        or symbol.get("address") != _cc_catalog.GETTICKCOUNT_PROVIDER_ADDRESS
        or symbol.get("end_exclusive")
        != _cc_catalog.GETTICKCOUNT_PROVIDER_END_EXCLUSIVE
        or symbol.get("size") != 16
        or symbol.get("extent_state") != "known"
        or symbol.get("binary") != "recoil"
        or symbol.get("kind") != "function"
        or symbol.get("pipeline_class") != "non-authored"
        or symbol.get("authored_order_role") != "non-authored"
        or symbol.get("navigation_name") != _cc_catalog.GETTICKCOUNT_RETAIL_THUNK_NAME
        or symbol.get("output_section_id") != "recoil:section:.text"
        or symbol.get("ownership_state") != "primary-owned"
        or symbol.get("physical_block_id")
        != _cc_catalog.GETTICKCOUNT_PROVIDER_BLOCK_ID
        or not isinstance(owner, Mapping)
        or owner.get("binary") != "recoil"
        or owner.get("kind") != "provider-boundary"
        or owner.get("blocker") != "none"
        or owner.get("lifecycle_state") != "accepted"
        or owner.get("provider_state") != "accepted"
        or owner.get("relationships") != expected_relationships
        or owner.get("source_paths") != []
        or owner.get("gates")
        != {
            "boundary": "accepted",
            "byte": "deferred",
            "data": "none",
            "owner_linkage": "none",
            "source": "accepted",
        }
        or indexes.by_address.get(_cc_catalog.GETTICKCOUNT_PROVIDER_ADDRESS)
        != _cc_catalog.GETTICKCOUNT_PROVIDER_IDENTITY
        or _cc_catalog.GETTICKCOUNT_PROVIDER_IDENTITY not in indexes.provider_ids
    ):
        raise ValueError(
            "GetTickCount named import-thunk bridge lacks the exact accepted "
            "tracker provider owner/function and gates"
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
    if (
        not isinstance(block, Mapping)
        or block.get("binary") != "recoil"
        or block.get("row_kind") != "physical-source-block"
        or block.get("start") != _cc_catalog.GETTICKCOUNT_PROVIDER_ADDRESS
        or block.get("end_exclusive")
        != _cc_catalog.GETTICKCOUNT_PROVIDER_END_EXCLUSIVE
        or block.get("contribution_kind") != "provider"
        or block.get("agent_source_path") != _cc_catalog.GETTICKCOUNT_PROVIDER_PATH
        or block.get("source_path") != _cc_catalog.GETTICKCOUNT_PROVIDER_PATH
        or block.get("contribution_ids")
        != [_cc_catalog.GETTICKCOUNT_PROVIDER_SYMBOL_ID]
        or not isinstance(mapping, Mapping)
        or mapping.get("status") != "provider-boundary"
        or not current_not_applicable
    ):
        raise ValueError(
            "GetTickCount named import-thunk bridge lacks its exact current "
            "provider block and non-authored order gates"
        )

    target = load_manifest(
        _cc_catalog.GETTICKCOUNT_PROVIDER_ORDER_MANIFEST,
        enforce_source_policy=False,
    )
    rows = tuple(getattr(target, "functions", ()))
    translation_rows = tuple(
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
    )
    linked_intervals = tuple(getattr(target, "linked_function_intervals", ()))
    exact_row = (
        len(rows) == 1
        and getattr(rows[0], "address", "") == _cc_catalog.GETTICKCOUNT_PROVIDER_ADDRESS
        and getattr(rows[0], "symbol", "")
        == _cc_catalog.GETTICKCOUNT_CANDIDATE_CALLABLE
        and getattr(rows[0], "provenance", "") == "provider-boundary"
        and getattr(rows[0], "pipeline_class", "") == "non-authored"
        and getattr(rows[0], "authored_order_role", "") == "non-authored"
        and getattr(rows[0], "required_presence", None) is True
        and getattr(rows[0], "full_order_gate", None) is True
    )
    if (
        getattr(target, "name", "")
        != _cc_catalog.GETTICKCOUNT_PROVIDER_ORDER_TARGET_NAME
        or getattr(target, "target_binary", "") != "recoil"
        or Path(str(getattr(target, "manifest_path", ""))).resolve()
        != _cc_catalog.GETTICKCOUNT_PROVIDER_ORDER_MANIFEST.resolve()
        or getattr(target, "retail_start", "")
        != _cc_catalog.GETTICKCOUNT_PROVIDER_ADDRESS
        or getattr(target, "retail_end_exclusive", "")
        != _cc_catalog.GETTICKCOUNT_PROVIDER_END_EXCLUSIVE
        or getattr(target, "check_translation_unit_function_order", None)
        is not True
        or not exact_row
        or len(translation_rows) != 1
        or translation_rows[0][1] != rows[0]
        or getattr(translation_rows[0][0], "order_scope", "") != "authored"
        or len(linked_intervals) != 1
        or getattr(linked_intervals[0], "order_scope", "") != "full"
        or getattr(linked_intervals[0], "retail_start", "")
        != _cc_catalog.GETTICKCOUNT_PROVIDER_ADDRESS
        or getattr(linked_intervals[0], "retail_end_exclusive", "")
        != _cc_catalog.GETTICKCOUNT_PROVIDER_END_EXCLUSIVE
        or tuple(getattr(linked_intervals[0], "functions", ())) != rows
    ):
        raise ValueError(
            "GetTickCount named import-thunk bridge lacks its exact synchronized "
            "governed provider-order manifest row"
        )

    retail_import, _directory_context = retail_import_target(
        reference=reference,
        address=_cc_catalog.GETTICKCOUNT_IAT_ADDRESS,
        dll=_cc_catalog.GETTICKCOUNT_IMPORT_DLL,
        import_name=_cc_catalog.GETTICKCOUNT_IMPORT_NAME,
    )
    if (
        retail_import.address != _cc_catalog.GETTICKCOUNT_IAT_ADDRESS
        or retail_import.dll != _cc_catalog.GETTICKCOUNT_IMPORT_DLL
        or retail_import.import_name != _cc_catalog.GETTICKCOUNT_IMPORT_NAME
        or retail_import.import_ordinal is not None
    ):
        raise ValueError(
            "GetTickCount named import-thunk bridge immutable retail import "
            "tuple drifted"
        )
    thunk_bytes = _cc_cfg._hexdump_bytes(
        bridge.hexdump(_cc_catalog.GETTICKCOUNT_PROVIDER_ADDRESS, 16)
    )
    if thunk_bytes != bytes.fromhex(
        "ff 25 40 c1 4c 00 90 90 90 90 90 90 90 90 90 90"
    ):
        raise ValueError(
            "GetTickCount named import-thunk bytes, IAT operand, instruction "
            "extent, or NOP padding drifted"
        )
    return (
        ProviderNamedImportThunk(
            provider_identity=_cc_catalog.GETTICKCOUNT_PROVIDER_IDENTITY,
            thunk_address=_cc_catalog.GETTICKCOUNT_PROVIDER_ADDRESS,
            retail_name=_cc_catalog.GETTICKCOUNT_RETAIL_THUNK_NAME,
            callable_symbol=_cc_catalog.GETTICKCOUNT_CANDIDATE_CALLABLE,
            iat_object_symbol=_cc_catalog.GETTICKCOUNT_CANDIDATE_IMPORT_SYMBOL,
            iat_identity=_cc_catalog.GETTICKCOUNT_IAT_IDENTITY,
            iat_address=_cc_catalog.GETTICKCOUNT_IAT_ADDRESS,
            import_dll=_cc_catalog.GETTICKCOUNT_IMPORT_DLL,
            import_name=_cc_catalog.GETTICKCOUNT_IMPORT_NAME,
        ),
        {_cc_catalog.GETTICKCOUNT_RETAIL_THUNK_NAME: [thunk_symbol]},
    )


def _gettickcount_retail_e8_call_population(
    retail_instructions: Sequence[Instruction],
    *,
    thunk: ProviderNamedImportThunk | None,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
) -> tuple[int, ...]:
    """Prove the immutable retail E8 population without consulting candidate state."""

    if (
        caller_identity != _cc_catalog.GETTICKCOUNT_CALLER_IDENTITY
        or normalize_address(caller_start) != _cc_catalog.GETTICKCOUNT_CALLER_START
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.GETTICKCOUNT_CALLER_END_EXCLUSIVE
    ):
        return ()
    if thunk is None:
        raise ValueError(
            "GetTickCount retail E8 proof has no proven immutable retail thunk"
        )
    retail_addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(_cc_catalog.GETTICKCOUNT_CALLER_START),
    )
    retail_calls = [
        (address, instruction)
        for address, instruction in zip(retail_addresses, retail_instructions)
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
        and address in _cc_catalog.GETTICKCOUNT_RETAIL_E8_CALL_BYTES
    ]
    retail_ff_d7_calls = [
        instruction
        for instruction in retail_instructions
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
        and tuple(value.lower() for value in instruction.bytes) == ("ff", "d7")
    ]
    if (
        tuple(address for address, _instruction in retail_calls)
        != tuple(_cc_catalog.GETTICKCOUNT_RETAIL_E8_CALL_BYTES)
        or retail_ff_d7_calls
    ):
        raise ValueError(
            "GetTickCount retail truth requires exactly three direct E8 thunk "
            "calls and no candidate-shaped FF D7 calls"
        )
    for address, instruction in retail_calls:
        try:
            body = bytes(int(value, 16) for value in instruction.bytes)
        except (TypeError, ValueError) as exc:
            raise ValueError(
                "GetTickCount retail caller callsite bytes are malformed"
            ) from exc
        if (
            body != _cc_catalog.GETTICKCOUNT_RETAIL_E8_CALL_BYTES[address]
            or address + 5 + struct.unpack_from("<i", body, 1)[0]
            != address_value(_cc_catalog.GETTICKCOUNT_PROVIDER_ADDRESS)
            or _cc_cfg._instruction_operand(instruction).strip()
            != _cc_catalog.GETTICKCOUNT_RETAIL_THUNK_NAME
        ):
            raise ValueError(
                "GetTickCount retail caller requires exact direct E8 calls to "
                "the named provider thunk"
            )
    return tuple(address for address, _instruction in retail_calls)


def _integrate_preproven_named_import_thunk_packages(
    indexes: IdentityIndexes,
    thunks: Sequence[ProviderNamedImportThunk],
) -> tuple[IdentityIndexes, tuple[CurrentIatStoragePackage, ...]]:
    """Publish complete candidate-independent named packages already proven.

    Some legacy provider boundaries predate the tracker provider-data package
    schema, while their dedicated retail bridge proves the same immutable
    named-import tuple and exact IAT operand.  Join that proof into the generic
    package/index path without weakening ambiguity checks or consulting the
    candidate.
    """
    from _recoil.call_contract.records import (
        CurrentIatStoragePackage,
        ProviderNamedImportThunk,
    )

    packages: set[CurrentIatStoragePackage] = set()
    storage_by_address = dict(indexes.storage_by_address)
    storage_by_name = dict(indexes.storage_by_name)
    for thunk in thunks:
        if (
            not isinstance(thunk, ProviderNamedImportThunk)
            or not thunk.provider_identity.startswith("provider:")
            or not thunk.retail_name
            or not thunk.callable_symbol
            or not _cc_identity._decorated_coff_name(thunk.callable_symbol)
            or not thunk.iat_object_symbol.startswith("__imp_")
            or not _cc_identity._decorated_coff_name(thunk.iat_object_symbol)
            or not thunk.import_dll
            or not thunk.import_name
            or thunk.import_name.startswith("#")
            or thunk.iat_identity != f"iat:{thunk.import_name}"
        ):
            raise ValueError(
                "preproven named import thunk lacks one complete immutable "
                "named package"
            )
        address = normalize_address(thunk.iat_address)
        package = CurrentIatStoragePackage(
            address=address,
            import_dll=thunk.import_dll,
            import_name=thunk.import_name,
            import_ordinal=None,
            object_symbol=thunk.iat_object_symbol,
            identity=thunk.iat_identity,
        )
        packages.add(package)
        for mapping, key in (
            (storage_by_address, address),
            (storage_by_name, thunk.iat_object_symbol),
        ):
            prior = mapping.get(key)
            if prior not in {None, package.identity}:
                raise ValueError(
                    "preproven named import package conflicts with current "
                    f"identity for {key!r}"
                )
            mapping[key] = package.identity
    packages_by_address: dict[str, set[CurrentIatStoragePackage]] = {}
    for package in packages:
        packages_by_address.setdefault(package.address, set()).add(package)
    ambiguous = sorted(
        address
        for address, rows in packages_by_address.items()
        if len(rows) != 1
    )
    if ambiguous:
        raise ValueError(
            "preproven named import packages have ambiguous IAT addresses: "
            + ", ".join(ambiguous)
        )
    return (
        replace(
            indexes,
            storage_by_address=storage_by_address,
            storage_by_name=storage_by_name,
        ),
        tuple(sorted(packages, key=lambda row: address_value(row.address))),
    )


def _provider_named_import_thunk_retail_bridges(
    *,
    document: ProgressDocument,
    indexes: IdentityIndexes,
    bridge_by_address: Mapping[str, Any],
    bridge_by_name: Mapping[str, Any],
    bridge: BinaryNinjaBridge,
    additional_named_packages: Sequence[CurrentIatStoragePackage] = (),
    preproven_named_thunks: Sequence[ProviderNamedImportThunk] = (),
    retail_import_targets: Sequence[Any] | None = None,
) -> tuple[
    tuple[ProviderNamedImportThunk | ProviderPeNamedImportThunk, ...],
    dict[str, list[Any]],
]:
    """Prove unique named-import FF25 thunks from governed live facts.

    The provider function/owner/block and current IAT package are tracker
    authority.  The first six immutable retail bytes prove the thunk and its
    exact IAT operand; any remaining registered function bytes must be only
    linker/compiler padding.  No name, address, DLL, or import allowlist is
    involved.
    """
    from _recoil.call_contract.records import (
        CurrentIatStoragePackage,
        ProviderNamedImportThunk,
        ProviderPeNamedImportThunk,
    )

    owners = document.collection("owners")
    symbols = document.collection("symbols")
    physical_blocks = document.collection("physical_blocks")
    supplied_packages: list[CurrentIatStoragePackage] = []
    for package in additional_named_packages:
        if (
            not isinstance(package, CurrentIatStoragePackage)
            or package.import_ordinal is not None
            or not package.import_name
            or package.import_name.startswith("#")
            or not package.import_dll
            or not package.object_symbol.startswith("__imp_")
            or not _cc_identity._decorated_coff_name(package.object_symbol)
            or package.identity != f"iat:{package.import_name}"
        ):
            raise ValueError(
                "named import thunk discovery received an incomplete "
                "additional named package"
            )
        supplied_packages.append(
            replace(package, address=normalize_address(package.address))
        )
    import_packages = tuple(
        sorted(
            {
                package
                for package in (
                    *_cc_storage_identity._current_tracker_iat_storage_packages(document),
                    *supplied_packages,
                )
            },
            key=lambda package: (
                address_value(package.address),
                package.import_dll,
                package.import_name,
                package.object_symbol,
            ),
        )
    )
    packages_by_address: dict[str, list[CurrentIatStoragePackage]] = {}
    for package in import_packages:
        packages_by_address.setdefault(package.address, []).append(package)
    preproven_by_address: dict[str, ProviderNamedImportThunk] = {}
    for thunk in preproven_named_thunks:
        if not isinstance(thunk, ProviderNamedImportThunk):
            raise ValueError(
                "named import thunk discovery received malformed preproven thunk"
            )
        address = normalize_address(thunk.thunk_address)
        prior = preproven_by_address.get(address)
        if prior is not None and prior != thunk:
            raise ValueError(
                "named import thunk discovery has competing preproven thunks at "
                f"{address}"
            )
        preproven_by_address[address] = thunk

    primary_owners_by_symbol: dict[
        str, list[tuple[str, Mapping[str, Any]]]
    ] = {}
    for owner_id, owner in owners.items():
        if not isinstance(owner, Mapping):
            continue
        relationships = owner.get("relationships")
        if not isinstance(relationships, list):
            continue
        for relationship in relationships:
            if (
                isinstance(relationship, Mapping)
                and relationship.get("kind") == "primary-function"
                and isinstance(relationship.get("symbol_id"), str)
            ):
                primary_owners_by_symbol.setdefault(
                    str(relationship["symbol_id"]), []
                ).append((str(owner_id), owner))

    proven: list[ProviderNamedImportThunk | ProviderPeNamedImportThunk] = []
    retail_names: dict[str, list[Any]] = {}
    for symbol_id, owner_rows in primary_owners_by_symbol.items():
        symbol = symbols.get(symbol_id)
        if not isinstance(symbol, Mapping):
            continue
        retail_name = symbol.get("navigation_name")
        if not isinstance(retail_name, str) or not retail_name:
            continue
        raw_address = symbol.get("address")
        if not isinstance(raw_address, str):
            continue
        try:
            address = normalize_address(raw_address)
            end_exclusive = normalize_address(
                str(symbol.get("end_exclusive", ""))
            )
        except ProgressError:
            continue
        size = address_value(end_exclusive) - address_value(address)
        if (
            symbol.get("binary") != "recoil"
            or symbol.get("kind") != "function"
            or symbol.get("pipeline_class") != "non-authored"
            or symbol.get("authored_order_role") != "non-authored"
            or symbol.get("extent_state") != "known"
            or symbol.get("address") != address
            or size < 6
            or symbol.get("size") != size
            or symbol.get("output_section_id") != "recoil:section:.text"
            or symbol.get("ownership_state") != "primary-owned"
            or not isinstance(symbol.get("physical_block_id"), str)
        ):
            continue

        # Provider owners may legitimately group several ordinary MFC/CRT
        # bodies.  Classify the immutable instruction shape before applying
        # invariants that belong only to a named import thunk; otherwise an
        # unrelated grouped destructor can fail the whole live producer.
        thunk_prefix = _cc_cfg._hexdump_bytes(bridge.hexdump(address, 6))
        if len(thunk_prefix) != 6:
            raise ValueError(
                f"named import thunk prefilter at {symbol_id} has truncated "
                "immutable bytes"
            )
        if thunk_prefix[:2] != b"\xff\x25":
            continue
        accepted_owner_rows = [
            (owner_id, owner)
            for owner_id, owner in owner_rows
            if owner.get("binary") == "recoil"
            and owner.get("kind") == "provider-boundary"
            and owner.get("blocker") == "none"
            and owner.get("lifecycle_state") == "accepted"
            and owner.get("provider_state") == "accepted"
            and owner.get("source_paths") == []
        ]
        if not accepted_owner_rows:
            # A true immutable thunk under a pending/rejected provider owner
            # is unresolved provider debt.  It is not eligible for accepted
            # thunk discovery and remains available to the separate
            # convergence retail-blocker route.
            continue
        iat_address = normalize_address(
            struct.unpack_from("<I", thunk_prefix, 2)[0]
        )
        packages = packages_by_address.get(iat_address, [])
        if len(packages) != 1:
            raise ValueError(
                f"import thunk {symbol_id} IAT operand does not join one "
                "unique complete current named or ordinal package"
            )
        package = packages[0]
        if package.import_ordinal is not None:
            # This exact true thunk belongs to the sibling ordinal producer,
            # which independently proves its owner, block, six-byte extent,
            # package identities, and BN target.  Named discovery must route
            # it onward rather than misreport a missing named package.
            continue
        preproven = preproven_by_address.get(address)
        if preproven is not None:
            if (
                preproven.provider_identity != f"provider:{symbol_id}"
                or normalize_address(preproven.iat_address) != package.address
                or preproven.iat_identity != package.identity
                or preproven.iat_object_symbol != package.object_symbol
                or preproven.import_dll != package.import_dll
                or preproven.import_name != package.import_name
                or preproven.retail_name != retail_name
            ):
                raise ValueError(
                    f"preproven named import thunk {symbol_id} conflicts with "
                    "the unique current package or provider identity"
                )
            # The earlier dedicated producer has already proved the complete
            # provider/package/body/BN chain.  Do not publish a second generic
            # thunk for the same physical provider, which would make candidate
            # provider and IAT identities compete in downstream extraction.
            continue

        expected_relationships = [
            {"kind": "anchor-address", "address": address},
            {
                "kind": "primary-function",
                "address": address,
                "symbol_id": symbol_id,
            },
        ]
        if (
            len(owner_rows) != 1
            or len(accepted_owner_rows) != 1
            or accepted_owner_rows[0][1].get("relationships")
            != expected_relationships
        ):
            raise ValueError(
                f"named import thunk {symbol_id} lacks exactly one accepted "
                "provider-boundary primary owner"
            )

        block_id = str(symbol["physical_block_id"])
        block = physical_blocks.get(block_id)
        mapping = block.get("mapping") if isinstance(block, Mapping) else None
        contribution_ids = (
            block.get("contribution_ids")
            if isinstance(block, Mapping)
            else None
        )
        try:
            block_start = normalize_address(str(block.get("start", "")))
            block_end = normalize_address(
                str(block.get("end_exclusive", ""))
            )
        except (ProgressError, AttributeError):
            block_start = ""
            block_end = ""
        if (
            not isinstance(block, Mapping)
            or block.get("binary") != "recoil"
            or block.get("row_kind") != "physical-source-block"
            or block.get("contribution_kind") != "provider"
            or not isinstance(mapping, Mapping)
            or mapping.get("status") != "provider-boundary"
            or not isinstance(contribution_ids, list)
            or contribution_ids.count(symbol_id) != 1
            or not block_start
            or not block_end
            or address_value(block_start) > address_value(address)
            or address_value(block_end) < address_value(end_exclusive)
            or not isinstance(block.get("agent_source_path"), str)
            or not str(block["agent_source_path"]).startswith("provider:")
            or block.get("source_path") != block.get("agent_source_path")
        ):
            raise ValueError(
                f"named import thunk {symbol_id} lacks one exact provider block"
            )

        thunk_extent = _cc_cfg._hexdump_bytes(bridge.hexdump(address, size))
        if len(thunk_extent) != size:
            raise ValueError(
                f"named import thunk candidate {symbol_id} immutable extent "
                "is truncated"
            )
        if thunk_extent[:6] != thunk_prefix:
            raise ValueError(
                f"named import thunk {symbol_id} immutable prefix changed "
                "between classification and extent proof"
            )
        if any(value not in {0x90, 0xCC} for value in thunk_extent[6:]):
            raise ValueError(
                f"named import thunk {symbol_id} is not one exact immutable "
                "FF25 thunk followed only by registered padding"
            )
        callable_symbol = package.object_symbol.removeprefix("__imp_")
        if (
            callable_symbol == package.object_symbol
            or not callable_symbol
            or not _cc_identity._decorated_coff_name(callable_symbol)
            or indexes.storage_by_address.get(package.address)
            != package.identity
            or indexes.storage_by_name.get(package.object_symbol)
            != package.identity
        ):
            raise ValueError(
                f"named import thunk {symbol_id} package has no exact distinct "
                "callable/IAT COFF identity"
            )

        provider_identity = f"provider:{symbol_id}"
        if (
            indexes.by_address.get(address) != provider_identity
            or provider_identity not in indexes.provider_ids
        ):
            raise ValueError(
                f"named import thunk {symbol_id} lacks one collision-free "
                "governed callable identity"
            )
        friendly_symbol = _exact_bn_pe_import_friendly_symbol(
            bridge_by_address=bridge_by_address,
            bridge_by_name=bridge_by_name,
            address=address,
            retail_name=str(retail_name),
            immutable_import_name=package.import_name,
        )
        if friendly_symbol is None:
            raise ValueError(
                f"named import thunk {symbol_id} lacks one exact BN friendly "
                "target at its governed address"
            )

        thunk = ProviderNamedImportThunk(
            provider_identity=provider_identity,
            thunk_address=address,
            retail_name=str(retail_name),
            callable_symbol=callable_symbol,
            iat_object_symbol=package.object_symbol,
            iat_identity=package.identity,
            iat_address=package.address,
            import_dll=package.import_dll,
            import_name=package.import_name,
        )
        proven.append(thunk)
        retail_names.setdefault(str(retail_name), []).append(friendly_symbol)

    # Some exact retail named-import thunks predate reviewed provider-owner and
    # provider-data package topology.  Expected call identity is nevertheless
    # available without inventing that topology: the registered non-authored
    # symbol/block bounds the thunk bytes and the immutable retail PE import
    # directory owns its named IAT tuple.  This sibling route is expected-side
    # only; ordinal imports and candidate COFF spellings retain their stricter
    # current-package producers above and below.
    supplied_retail_targets = tuple(
        retail_import_targets
        if retail_import_targets is not None
        else _retail_import_targets(_cc_catalog.DEFAULT_REFERENCE)[0]
    )
    import_targets_by_address: dict[str, list[Any]] = {}
    named_targets_by_address: dict[str, list[Any]] = {}
    named_target_routes: dict[str, set[tuple[str, str, str]]] = {}
    for target in supplied_retail_targets:
        raw_address = getattr(target, "address", "")
        import_name = getattr(target, "import_name", None)
        import_dll = getattr(target, "dll", None)
        import_ordinal = getattr(target, "import_ordinal", None)
        if not raw_address:
            continue
        try:
            target_address = normalize_address(str(raw_address))
        except ProgressError as exc:
            raise ValueError(
                "named import thunk discovery received a malformed immutable "
                "PE import address"
            ) from exc
        if (
            not isinstance(import_name, str)
            or not import_name
            or not isinstance(import_dll, str)
            or not import_dll
            or "/" in import_dll
            or "\\" in import_dll
            or (
                import_ordinal is not None
                and (
                    not isinstance(import_ordinal, int)
                    or isinstance(import_ordinal, bool)
                    or not 0 <= import_ordinal <= 0xFFFF
                    or import_name != f"#{import_ordinal}"
                )
            )
            or (import_ordinal is None and import_name.startswith("#"))
        ):
            raise ValueError(
                "named import thunk discovery received a malformed immutable "
                "PE import tuple"
            )
        import_targets_by_address.setdefault(target_address, []).append(target)
        if import_ordinal is not None:
            continue
        named_targets_by_address.setdefault(target_address, []).append(target)
        named_target_routes.setdefault(import_name, set()).add(
            (target_address, import_dll, import_name)
        )

    already_proven = {
        normalize_address(thunk.thunk_address) for thunk in proven
    } | set(preproven_by_address)
    current_packages = _cc_storage_identity._current_tracker_iat_storage_packages(document)
    for symbol_id, symbol in symbols.items():
        if not isinstance(symbol, Mapping):
            continue
        raw_address = symbol.get("address")
        retail_name = symbol.get("navigation_name")
        if not isinstance(raw_address, str) or not isinstance(retail_name, str):
            continue
        try:
            address = normalize_address(raw_address)
        except ProgressError:
            continue
        if address in already_proven:
            continue
        block_id = symbol.get("physical_block_id")
        block = physical_blocks.get(block_id)
        mapping = block.get("mapping") if isinstance(block, Mapping) else None
        contribution_ids = (
            block.get("contribution_ids")
            if isinstance(block, Mapping)
            else None
        )
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
        try:
            block_start = normalize_address(str(block.get("start", "")))
            block_end = normalize_address(str(block.get("end_exclusive", "")))
            symbol_end = normalize_address(str(symbol.get("end_exclusive", "")))
        except (ProgressError, AttributeError):
            continue
        if (
            symbol.get("binary") != "recoil"
            or symbol.get("kind") != "function"
            or symbol.get("pipeline_class") != "non-authored"
            or symbol.get("authored_order_role") != "non-authored"
            or symbol.get("extent_state") != "known"
            or symbol.get("output_section_id") != "recoil:section:.text"
            or symbol.get("ownership_state") not in {"unresolved", "primary-owned"}
            or not isinstance(block, Mapping)
            or block.get("binary") != "recoil"
            or block.get("row_kind") != "physical-source-block"
            or block.get("contribution_kind") != "provider"
            or not isinstance(mapping, Mapping)
            or mapping.get("status") != "provider-boundary"
            or not current_not_applicable
            or not isinstance(contribution_ids, list)
            or contribution_ids.count(str(symbol_id)) != 1
            or not isinstance(block.get("agent_source_path"), str)
            or not str(block["agent_source_path"]).startswith("provider:")
            or block.get("source_path") != block.get("agent_source_path")
            or not (address_value(block_start) <= address_value(address) < address_value(block_end))
        ):
            continue

        # A sole-contribution provider block is the narrower physical extent
        # when a legacy descriptive function row incorrectly runs into later
        # provider data.  Grouped blocks retain each exact symbol boundary.
        if address_value(symbol_end) <= address_value(block_end):
            proof_end = symbol_end
        elif (
            (contribution_ids == [symbol_id] and block_start == address)
            or contribution_ids[-1:] == [symbol_id]
        ):
            # Legacy descriptive function extents may run beyond the exact
            # provider block.  The block end is authoritative for either a
            # sole contribution or the final contribution in a grouped block;
            # an interior crossing row remains ambiguous and is skipped.
            proof_end = block_end
        else:
            continue
        proof_size = address_value(proof_end) - address_value(address)
        if proof_size < 6:
            continue
        thunk_extent = _cc_cfg._hexdump_bytes(bridge.hexdump(address, proof_size))
        if len(thunk_extent) != proof_size:
            raise ValueError(
                f"PE named import thunk {symbol_id} immutable extent is truncated"
            )
        if thunk_extent[:2] != b"\xff\x25":
            continue
        if any(value not in {0x90, 0xCC} for value in thunk_extent[6:]):
            raise ValueError(
                f"PE named import thunk {symbol_id} is not one exact immutable "
                "FF25 thunk followed only by registered padding"
            )
        iat_address = normalize_address(struct.unpack_from("<I", thunk_extent, 2)[0])
        immutable_matches = import_targets_by_address.get(iat_address, [])
        if len(immutable_matches) > 1 and any(
            getattr(target, "import_ordinal", None) is not None
            for target in immutable_matches
        ):
            raise ValueError(
                f"PE import thunk {symbol_id} IAT operand has ambiguous "
                "immutable named/ordinal routes"
            )
        provider_identity = f"provider:{symbol_id}"
        if (
            indexes.by_address.get(address) != provider_identity
            or provider_identity not in indexes.provider_ids
        ):
            raise ValueError(
                f"PE import thunk {symbol_id} lacks one collision-free "
                "governed physical provider identity"
            )
        ordinal_route = (
            len(immutable_matches) == 1
            and getattr(immutable_matches[0], "import_ordinal", None) is not None
        )
        named_matches = named_targets_by_address.get(iat_address, [])
        immutable_import_name = (
            str(getattr(named_matches[0], "import_name"))
            if len(named_matches) == 1
            else None
        )
        friendly_symbol = _exact_bn_pe_import_friendly_symbol(
            bridge_by_address=bridge_by_address,
            bridge_by_name=bridge_by_name,
            address=address,
            retail_name=retail_name,
            immutable_import_name=(
                None if ordinal_route else immutable_import_name
            ),
        )
        if friendly_symbol is None:
            raise ValueError(
                f"PE import thunk {symbol_id} lacks one exact BN friendly "
                "target at its governed address"
            )
        if ordinal_route:
            # The sibling ordinal producer owns its accepted-owner/current-IAT
            # package and candidate COFF identity proof.  Classify the exact
            # immutable ordinal tuple before enforcing named-import authority
            # so a real ordinal thunk cannot be misreported as a missing named
            # package by this expected-side fallback.
            continue
        target_matches = named_matches
        if len(target_matches) != 1:
            raise ValueError(
                f"PE named import thunk {symbol_id} IAT operand does not join "
                "one unique immutable named import"
            )
        target = target_matches[0]
        import_name = str(getattr(target, "import_name"))
        import_dll = str(getattr(target, "dll"))
        if len(named_target_routes.get(import_name, set())) != 1:
            raise ValueError(
                f"PE named import thunk {symbol_id} import name {import_name!r} "
                "has ambiguous DLL/IAT routes"
            )
        package_matches = [
            package
            for package in current_packages
            if package.address == iat_address
        ]
        if len(package_matches) > 1:
            raise ValueError(
                f"PE named import thunk {symbol_id} has ambiguous current "
                "tracker packages"
            )
        if package_matches:
            package = package_matches[0]
            if (
                package.import_ordinal is not None
                or package.import_dll != import_dll
                or package.import_name != import_name
                or package.identity != f"iat:{import_name}"
            ):
                raise ValueError(
                    f"PE named import thunk {symbol_id} conflicts with its "
                    "current tracker package"
                )
        proven.append(
            ProviderPeNamedImportThunk(
                provider_identity=provider_identity,
                thunk_address=address,
                retail_name=retail_name,
                iat_identity=f"iat:{import_name}",
                iat_address=iat_address,
                import_dll=import_dll,
                import_name=import_name,
            )
        )
        retail_names.setdefault(retail_name, []).append(friendly_symbol)
        already_proven.add(address)

    for attribute in (
        "provider_identity",
        "thunk_address",
        "retail_name",
        "iat_identity",
    ):
        values = [getattr(thunk, attribute) for thunk in proven]
        if len(values) != len(set(values)):
            raise ValueError(
                "named import thunk discovery has duplicate or colliding "
                f"{attribute.replace('_', ' ')} values"
            )
    return tuple(proven), retail_names


def _exact_bn_pe_import_friendly_symbol(
    *,
    bridge_by_address: Mapping[str, Any],
    bridge_by_name: Mapping[str, Any],
    address: str,
    retail_name: str,
    immutable_import_name: str | None = None,
) -> Any | None:
    """Join one exact BN function/import view for a PE import thunk.

    Binary Ninja exposes an imported-function symbol through both its function
    and import inventories.  ``BinaryNinjaBridge.symbols`` deliberately gives
    the import inventory the final address slot.  Most live MFC42 rows retain
    the decorated navigation name while ``full_name`` is a demangled alias;
    one reviewed function navigation alias can coexist with a differently
    named import row at that exact address.  Named CRT/platform thunks use the
    same two representations.  Admit them only when every nonempty import alias
    is independently indexed back to the identical import symbol and the
    governed function alias is exact.  When immutable PE supplies a named
    import, also require the canonical BN import name/raw name to equal it.
    Keep the legacy function-only row accepted for stable views and fixtures;
    no other symbol kind or alias approximation participates.
    """

    friendly_symbol = bridge_by_name.get(retail_name)
    address_symbol = bridge_by_address.get(address)
    if friendly_symbol is None or address_symbol is None:
        return None

    def identity(symbol: Any) -> tuple[str, str, str, str, str]:
        return (
            str(getattr(symbol, "address", "")),
            str(getattr(symbol, "name", "")),
            str(getattr(symbol, "raw_name", "")),
            str(getattr(symbol, "full_name", "")),
            str(getattr(symbol, "kind", "")),
        )

    friendly_identity = identity(friendly_symbol)
    address_identity = identity(address_symbol)
    friendly_address, friendly_name, friendly_raw, friendly_full, friendly_kind = (
        friendly_identity
    )
    if friendly_address != address or friendly_name != retail_name:
        return None
    if friendly_identity == address_identity and friendly_kind == "function":
        return friendly_symbol

    import_address, import_name, import_raw, import_full, import_kind = (
        address_identity
    )
    if (
        import_address != address
        or import_kind != "import"
        or not import_name
        or import_raw != import_name
        or not import_full
        or (
            immutable_import_name is not None
            and import_name != immutable_import_name
        )
    ):
        return None
    for alias in {import_name, import_raw, import_full}:
        indexed = bridge_by_name.get(alias)
        if indexed is None or identity(indexed) != address_identity:
            return None

    if friendly_identity == address_identity:
        if friendly_raw != retail_name:
            return None
        return friendly_symbol
    if (
        friendly_kind == "function"
        and friendly_raw == retail_name
        and friendly_full in {"", retail_name}
        and retail_name not in {import_name, import_raw, import_full}
    ):
        return friendly_symbol
    return None


def _provider_ordinal_import_thunk_retail_bridges(
    *,
    document: ProgressDocument,
    indexes: IdentityIndexes,
    bridge_by_address: Mapping[str, Any],
    bridge_by_name: Mapping[str, Any],
    bridge: BinaryNinjaBridge,
    excluded_symbol_ids: frozenset[str] = frozenset(),
) -> tuple[
    tuple[ProviderOrdinalImportThunk, ...],
    dict[str, list[Any]],
]:
    """Prove governed six-byte FF25 ordinal-import thunks from current facts."""
    from _recoil.call_contract.records import ProviderOrdinalImportThunk
    owners = document.collection("owners")
    symbols = document.collection("symbols")
    physical_blocks = document.collection("physical_blocks")
    ordinal_packages = tuple(
        package
        for package in _cc_storage_identity._current_tracker_iat_storage_packages(document)
        if package.import_ordinal is not None
    )
    packages_by_address: dict[str, list[CurrentIatStoragePackage]] = {}
    for package in ordinal_packages:
        packages_by_address.setdefault(package.address, []).append(package)

    primary_owners_by_symbol: dict[str, list[tuple[str, Mapping[str, Any]]]] = {}
    for owner_id, owner in owners.items():
        if not isinstance(owner, Mapping):
            continue
        relationships = owner.get("relationships")
        if not isinstance(relationships, list):
            continue
        for relationship in relationships:
            if (
                isinstance(relationship, Mapping)
                and relationship.get("kind") == "primary-function"
                and isinstance(relationship.get("symbol_id"), str)
            ):
                primary_owners_by_symbol.setdefault(
                    str(relationship["symbol_id"]),
                    [],
                ).append((str(owner_id), owner))

    proven: list[ProviderOrdinalImportThunk] = []
    retail_names: dict[str, list[Any]] = {}
    for symbol_id, owner_rows in primary_owners_by_symbol.items():
        if symbol_id in excluded_symbol_ids:
            continue
        symbol = symbols.get(symbol_id)
        if not isinstance(symbol, Mapping):
            continue
        raw_address = symbol.get("address")
        if not isinstance(raw_address, str):
            continue
        try:
            address = normalize_address(raw_address)
        except ProgressError:
            continue
        if (
            symbol.get("binary") != "recoil"
            or symbol.get("kind") != "function"
            or symbol.get("pipeline_class") != "non-authored"
            or symbol.get("authored_order_role") != "non-authored"
            or symbol.get("extent_state") != "known"
            or symbol.get("address") != address
            or symbol.get("end_exclusive")
            != normalize_address(address_value(address) + 6)
            or symbol.get("size") != 6
            or symbol.get("output_section_id") != "recoil:section:.text"
            or symbol.get("ownership_state") != "primary-owned"
            or not isinstance(symbol.get("physical_block_id"), str)
        ):
            continue

        expected_relationships = [
            {"kind": "anchor-address", "address": address},
            {
                "kind": "primary-function",
                "address": address,
                "symbol_id": symbol_id,
            },
        ]
        accepted_owner_rows = [
            (owner_id, owner)
            for owner_id, owner in owner_rows
            if owner.get("binary") == "recoil"
            and owner.get("kind") == "provider-boundary"
            and owner.get("blocker") == "none"
            and owner.get("lifecycle_state") == "accepted"
            and owner.get("provider_state") == "accepted"
            and owner.get("source_paths") == []
        ]
        if not accepted_owner_rows:
            continue
        if (
            len(owner_rows) != 1
            or len(accepted_owner_rows) != 1
            or accepted_owner_rows[0][1].get("relationships")
            != expected_relationships
        ):
            raise ValueError(
                f"ordinal import thunk {symbol_id} lacks exactly one accepted "
                "provider-boundary primary owner"
            )

        block_id = str(symbol["physical_block_id"])
        block = physical_blocks.get(block_id)
        mapping = block.get("mapping") if isinstance(block, Mapping) else None
        contribution_ids = (
            block.get("contribution_ids")
            if isinstance(block, Mapping)
            else None
        )
        try:
            block_start = normalize_address(str(block.get("start", "")))
            block_end = normalize_address(str(block.get("end_exclusive", "")))
        except (ProgressError, AttributeError):
            block_start = ""
            block_end = ""
        if (
            not isinstance(block, Mapping)
            or block.get("binary") != "recoil"
            or block.get("row_kind") != "physical-source-block"
            or block.get("contribution_kind") != "provider"
            or not isinstance(mapping, Mapping)
            or mapping.get("status") != "provider-boundary"
            or not isinstance(contribution_ids, list)
            or contribution_ids.count(symbol_id) != 1
            or not block_start
            or not block_end
            or address_value(block_start) > address_value(address)
            or address_value(block_end)
            < address_value(symbol["end_exclusive"])
            or not isinstance(block.get("agent_source_path"), str)
            or not str(block["agent_source_path"]).startswith("provider:")
            or block.get("source_path") != block.get("agent_source_path")
        ):
            raise ValueError(
                f"ordinal import thunk {symbol_id} lacks one exact provider block"
            )

        thunk_bytes = _cc_cfg._hexdump_bytes(bridge.hexdump(address, 6))
        if len(thunk_bytes) != 6 or thunk_bytes[:2] != b"\xff\x25":
            raise ValueError(
                f"ordinal import thunk {symbol_id} is not an exact immutable "
                "six-byte FF25 thunk"
            )
        iat_address = normalize_address(struct.unpack_from("<I", thunk_bytes, 2)[0])
        packages = packages_by_address.get(iat_address, [])
        if len(packages) != 1:
            raise ValueError(
                f"ordinal import thunk {symbol_id} IAT operand does not join "
                "one complete current ordinal package"
            )
        package = packages[0]
        callable_symbol = package.object_symbol.removeprefix("__imp_")
        if (
            callable_symbol == package.object_symbol
            or not callable_symbol
            or not _cc_identity._decorated_coff_name(callable_symbol)
            or indexes.storage_by_address.get(package.address)
            != package.identity
            or indexes.storage_by_name.get(package.object_symbol)
            != package.identity
        ):
            raise ValueError(
                f"ordinal import thunk {symbol_id} package has no exact distinct "
                "callable/IAT COFF identity"
            )

        provider_identity = f"provider:{symbol_id}"
        if (
            indexes.by_address.get(address) != provider_identity
            or provider_identity not in indexes.provider_ids
        ):
            raise ValueError(
                f"ordinal import thunk {symbol_id} lacks one collision-free "
                "governed callable identity"
            )
        retail_name = symbol.get("navigation_name")
        friendly_symbol = (
            _exact_bn_pe_import_friendly_symbol(
                bridge_by_address=bridge_by_address,
                bridge_by_name=bridge_by_name,
                address=address,
                retail_name=retail_name,
            )
            if isinstance(retail_name, str) and retail_name
            else None
        )
        if friendly_symbol is None:
            raise ValueError(
                f"ordinal import thunk {symbol_id} lacks one exact BN friendly "
                "target at its governed address"
            )

        thunk = ProviderOrdinalImportThunk(
            provider_identity=provider_identity,
            thunk_address=address,
            retail_name=str(retail_name),
            callable_symbol=callable_symbol,
            iat_object_symbol=package.object_symbol,
            iat_identity=package.identity,
            import_dll=package.import_dll,
            import_ordinal=int(package.import_ordinal),
        )
        proven.append(thunk)
        retail_names.setdefault(str(retail_name), []).append(friendly_symbol)

    for attribute in (
        "provider_identity",
        "thunk_address",
        "retail_name",
        "callable_symbol",
        "iat_object_symbol",
        "iat_identity",
    ):
        values = [getattr(thunk, attribute) for thunk in proven]
        if len(values) != len(set(values)):
            raise ValueError(
                "ordinal import thunk discovery has duplicate or colliding "
                f"{attribute.replace('_', ' ')} values"
            )
    return tuple(proven), retail_names


def _provider_pe_ordinal_import_thunk_retail_bridges(
    *,
    document: ProgressDocument,
    indexes: IdentityIndexes,
    bridge_by_address: Mapping[str, Any],
    bridge_by_name: Mapping[str, Any],
    bridge: BinaryNinjaBridge,
    retail_import_targets: Sequence[Any] | None = None,
) -> tuple[
    IdentityIndexes,
    tuple[ProviderOrdinalImportThunk, ...],
    dict[str, list[Any]],
]:
    """Compose exact unowned ordinal thunks through reviewed provider blocks.

    The immutable PE import directory owns the ordinal IAT tuple, while the
    tracker owns the physical function extent/identity and the already
    reviewed provider block.  This comparison-scoped join deliberately does
    not require or create a primary owner or provider-target registration for
    every six-byte MFC thunk.
    """
    from _recoil.call_contract.records import ProviderOrdinalImportThunk

    symbols = document.collection("symbols")
    blocks = document.collection("physical_blocks")
    owners = document.collection("owners")
    primary_owned_symbols = {
        str(relationship.get("symbol_id"))
        for owner in owners.values()
        if isinstance(owner, Mapping)
        for relationship in owner.get("relationships", ())
        if isinstance(relationship, Mapping)
        and relationship.get("kind") == "primary-function"
        and isinstance(relationship.get("symbol_id"), str)
    }
    supplied_targets = tuple(
        retail_import_targets
        if retail_import_targets is not None
        else _retail_import_targets(_cc_catalog.DEFAULT_REFERENCE)[0]
    )
    all_targets_by_address: dict[str, list[Any]] = {}
    ordinal_targets_by_address: dict[str, list[Any]] = {}
    for target in supplied_targets:
        raw_address = getattr(target, "address", "")
        import_name = getattr(target, "import_name", None)
        import_dll = getattr(target, "dll", None)
        import_ordinal = getattr(target, "import_ordinal", None)
        try:
            iat_address = normalize_address(str(raw_address))
        except ProgressError as exc:
            raise ValueError(
                "PE ordinal thunk composition received a malformed immutable "
                "IAT address"
            ) from exc
        all_targets_by_address.setdefault(iat_address, []).append(target)
        if import_ordinal is None:
            continue
        if (
            not isinstance(import_ordinal, int)
            or isinstance(import_ordinal, bool)
            or not 0 <= import_ordinal <= 0xFFFF
            or import_name != f"#{import_ordinal}"
            or not isinstance(import_dll, str)
            or not import_dll
            or "/" in import_dll
            or "\\" in import_dll
        ):
            raise ValueError(
                "PE ordinal thunk composition received a malformed immutable "
                "ordinal import tuple"
            )
        ordinal_targets_by_address.setdefault(iat_address, []).append(target)

    get_json = getattr(bridge, "get_json", None)
    proven: list[ProviderOrdinalImportThunk] = []
    names: dict[str, list[Any]] = {}
    storage_by_address = dict(indexes.storage_by_address)
    storage_by_name = dict(indexes.storage_by_name)
    for symbol_id, symbol in symbols.items():
        if (
            not isinstance(symbol, Mapping)
            or symbol_id in primary_owned_symbols
            or symbol.get("binary") != "recoil"
            or symbol.get("kind") != "function"
            or symbol.get("pipeline_class") != "non-authored"
            or symbol.get("authored_order_role") != "non-authored"
            or symbol.get("ownership_state") != "unresolved"
            or symbol.get("disposition") != "unresolved"
            or symbol.get("extent_state") != "known"
            or symbol.get("output_section_id") != "recoil:section:.text"
            or symbol.get("size") != 6
            or not isinstance(symbol.get("address"), str)
            or not isinstance(symbol.get("physical_block_id"), str)
            or not isinstance(symbol.get("navigation_name"), str)
            or not symbol.get("navigation_name")
        ):
            continue
        try:
            address = normalize_address(str(symbol["address"]))
            end_exclusive = normalize_address(
                str(symbol.get("end_exclusive", ""))
            )
        except ProgressError:
            continue
        if end_exclusive != normalize_address(address_value(address) + 6):
            continue

        thunk_bytes = _cc_cfg._hexdump_bytes(bridge.hexdump(address, 6))
        if len(thunk_bytes) != 6:
            raise ValueError(
                f"PE ordinal thunk prefilter at {symbol_id} has truncated "
                "immutable bytes"
            )
        if thunk_bytes[:2] != b"\xff\x25":
            continue
        iat_address = normalize_address(
            struct.unpack_from("<I", thunk_bytes, 2)[0]
        )
        packages = ordinal_targets_by_address.get(iat_address, [])
        all_packages = all_targets_by_address.get(iat_address, [])
        if not packages and len(all_packages) == 1:
            # A unique named import belongs to the sibling named producer.
            continue
        if len(packages) != 1:
            raise ValueError(
                f"PE ordinal thunk {symbol_id} IAT operand does not join one "
                "unique immutable ordinal import"
            )
        package = packages[0]

        same_address_symbols = [
            other_id
            for other_id, other in symbols.items()
            if isinstance(other, Mapping)
            and other.get("binary") == "recoil"
            and other.get("kind") == "function"
            and other.get("address") == address
        ]
        if same_address_symbols != [symbol_id]:
            raise ValueError(
                f"PE ordinal thunk {symbol_id} has ambiguous physical aliases"
            )
        block = blocks.get(str(symbol["physical_block_id"]))
        mapping = block.get("mapping") if isinstance(block, Mapping) else None
        contribution_ids = (
            block.get("contribution_ids")
            if isinstance(block, Mapping)
            else None
        )
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
        try:
            block_start = normalize_address(str(block.get("start", "")))
            block_end = normalize_address(str(block.get("end_exclusive", "")))
        except (ProgressError, AttributeError):
            block_start = ""
            block_end = ""
        if block_end == end_exclusive:
            # Preserve the stricter terminal-block producer for its existing
            # current-package/xref population.
            continue
        if (
            not isinstance(block, Mapping)
            or block.get("binary") != "recoil"
            or block.get("row_kind") != "physical-source-block"
            or block.get("contribution_kind") != "provider"
            or not isinstance(mapping, Mapping)
            or mapping.get("status") != "provider-boundary"
            or not current_not_applicable
            or not isinstance(contribution_ids, list)
            or contribution_ids.count(symbol_id) != 1
            or not block_start
            or not block_end
            or not (
                address_value(block_start)
                <= address_value(address)
                < address_value(end_exclusive)
                <= address_value(block_end)
            )
            or not isinstance(block.get("agent_source_path"), str)
            or not str(block["agent_source_path"]).startswith("provider:")
            or block.get("source_path") != block.get("agent_source_path")
        ):
            raise ValueError(
                f"PE ordinal thunk {symbol_id} lacks one exact current "
                "provider block"
            )

        provider_identity = f"provider:{symbol_id}"
        if (
            indexes.by_address.get(address) != provider_identity
            or provider_identity not in indexes.provider_ids
        ):
            raise ValueError(
                f"PE ordinal thunk {symbol_id} lacks one collision-free "
                "physical provider identity"
            )
        retail_name = str(symbol["navigation_name"])
        friendly = _exact_bn_pe_import_friendly_symbol(
            bridge_by_address=bridge_by_address,
            bridge_by_name=bridge_by_name,
            address=address,
            retail_name=retail_name,
        )
        address_symbol = bridge_by_address.get(address)
        callable_symbol = str(getattr(address_symbol, "name", ""))
        if (
            friendly is None
            or getattr(address_symbol, "kind", "") != "import"
            or not callable_symbol
            or (
                not _cc_identity._decorated_coff_name(callable_symbol)
                and re.fullmatch(
                    r"[A-Za-z_][A-Za-z0-9_@$?]*", callable_symbol
                )
                is None
            )
        ):
            raise ValueError(
                f"PE ordinal thunk {symbol_id} lacks one exact BN friendly "
                "callable"
            )

        if not callable(get_json):
            raise ValueError(
                f"PE ordinal thunk {symbol_id} lacks complete xref authority"
            )
        thunk_payload = get_json("getXrefsTo", address=address, limit=100000)
        thunk_code, thunk_data, thunk_complete = _cc_identity._bn_inbound_xref_items(
            thunk_payload
        )
        caller_addresses = tuple(
            _cc_identity._bn_xref_address(
                row,
                "source_address",
                "source_addr",
                "from_address",
                "address",
            )
            for row in thunk_code
        )
        if thunk_complete and not caller_addresses:
            # A physical six-byte ordinal stub with no immutable inbound
            # REL32 transfer is not active for call-contract comparison.
            # Static data references do not turn an address-valued provider
            # entry into a call target.
            continue
        if (
            not thunk_complete
            or any(not caller for caller in caller_addresses)
            or len(caller_addresses) != len(set(caller_addresses))
        ):
            raise ValueError(
                f"PE ordinal thunk {symbol_id} lacks a complete collision-free "
                "REL32 caller population"
            )
        rel32_caller_addresses: list[str] = []
        for caller_address in caller_addresses:
            _caller_name, caller_function = _cc_identity._bn_unique_containing_function(
                bridge,
                instruction_address=caller_address,
            )
            caller_rows = tuple(
                _cc_listing.parse_assembly(bridge.assembly(caller_function), source="bn")
            )
            matches = [
                row
                for row in caller_rows
                if _cc_cfg._source_instruction_address(row) == caller_address
            ]
            if len(matches) != 1:
                raise ValueError(
                    f"PE ordinal thunk {symbol_id} has caller xref drift"
                )
            try:
                caller_bytes = bytes(int(value, 16) for value in matches[0].bytes)
            except (TypeError, ValueError):
                caller_bytes = b""
            if (
                len(caller_bytes) != 5
                or caller_bytes[0] not in {0xE8, 0xE9}
                or normalize_address(
                    address_value(caller_address)
                    + 5
                    + struct.unpack_from("<i", caller_bytes, 1)[0]
                )
                != address
            ):
                # BN code xrefs also include address materializations and EH
                # metadata consumers.  They do not activate an invocation
                # route; only exact rel32 CALL/JMP rows do.
                continue
            rel32_caller_addresses.append(caller_address)
        if not rel32_caller_addresses:
            continue
        iat_payload = get_json(
            "getXrefsTo", address=iat_address, limit=100000
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
            or iat_readers != (address,)
        ):
            raise ValueError(
                f"PE ordinal thunk {symbol_id} has IAT-reader xref drift"
            )

        import_dll = str(getattr(package, "dll"))
        import_ordinal = int(getattr(package, "import_ordinal"))
        iat_identity = (
            f"iat:ordinal:{len(import_dll)}:{import_dll}:{import_ordinal}"
        )
        iat_object_symbol = f"__imp_{callable_symbol}"
        for mapping_index, key in (
            (storage_by_address, iat_address),
            (storage_by_name, iat_object_symbol),
        ):
            prior = mapping_index.get(key)
            if prior not in {None, iat_identity}:
                raise ValueError(
                    f"PE ordinal thunk {symbol_id} conflicts with current "
                    f"comparison identity for {key!r}"
                )
            mapping_index[key] = iat_identity
        proven.append(
            ProviderOrdinalImportThunk(
                provider_identity=provider_identity,
                thunk_address=address,
                retail_name=retail_name,
                callable_symbol=callable_symbol,
                iat_object_symbol=iat_object_symbol,
                iat_identity=iat_identity,
                import_dll=import_dll,
                import_ordinal=import_ordinal,
            )
        )
        names.setdefault(retail_name, []).append(friendly)

    for attribute in (
        "provider_identity",
        "thunk_address",
        "retail_name",
        "callable_symbol",
        "iat_object_symbol",
        "iat_identity",
    ):
        values = [getattr(thunk, attribute) for thunk in proven]
        if len(values) != len(set(values)):
            raise ValueError(
                "PE ordinal thunk composition has duplicate or colliding "
                f"{attribute.replace('_', ' ')} values"
            )
    return (
        replace(
            indexes,
            storage_by_address=storage_by_address,
            storage_by_name=storage_by_name,
        ),
        tuple(proven),
        names,
    )


def _provider_terminal_ordinal_import_thunk_retail_bridges(
    *,
    document: ProgressDocument,
    indexes: IdentityIndexes,
    bridge_by_address: Mapping[str, Any],
    bridge_by_name: Mapping[str, Any],
    bridge: BinaryNinjaBridge,
) -> tuple[tuple[ProviderOrdinalImportThunk, ...], dict[str, list[Any]]]:
    """Prove exact terminal FF25 ordinal thunks in reviewed provider blocks.

    Some retail provider blocks end in a six-byte import thunk while the BN
    navigation function deliberately has a larger descriptive extent and no
    standalone primary owner.  Accept that physical row only when it is the
    exact final six bytes of one reviewed provider block and joins one current
    immutable ordinal-IAT package plus exact BN function/import aliases, one
    complete REL32 caller xref, and one complete FF25 IAT-reader xref.  This is
    not an address or name allowlist and does not infer a target from candidate
    text.
    """
    from _recoil.call_contract.records import ProviderOrdinalImportThunk

    symbols = document.collection("symbols")
    blocks = document.collection("physical_blocks")
    owners = document.collection("owners")
    ordinal_packages = tuple(
        package
        for package in _cc_storage_identity._current_tracker_iat_storage_packages(document)
        if package.import_ordinal is not None
    )
    primary_owned_symbols = {
        str(relationship.get("symbol_id"))
        for owner in owners.values()
        if isinstance(owner, Mapping)
        for relationship in owner.get("relationships", ())
        if isinstance(relationship, Mapping)
        and relationship.get("kind") == "primary-function"
        and isinstance(relationship.get("symbol_id"), str)
    }
    proven: list[ProviderOrdinalImportThunk] = []
    names: dict[str, list[Any]] = {}
    for symbol_id, symbol in symbols.items():
        if not isinstance(symbol, Mapping) or symbol_id in primary_owned_symbols:
            continue
        raw_address = symbol.get("address")
        block_id = symbol.get("physical_block_id")
        if (
            not isinstance(raw_address, str)
            or not isinstance(block_id, str)
            or symbol.get("kind") != "function"
            or symbol.get("pipeline_class") != "non-authored"
            or symbol.get("authored_order_role") != "non-authored"
        ):
            continue
        try:
            address = normalize_address(raw_address)
            terminal = normalize_address(address_value(address) + 6)
        except (ProgressError, TypeError, ValueError):
            continue

        # A terminal provider block is not, by itself, ordinal-package
        # authority.  Activate only from either the exact raw FF25 operand or
        # an exact BN import alias that joins a complete current package.
        # After that activation every physical/provider/byte/identity gate
        # below remains mandatory and fail-closed.
        thunk_bytes = _cc_cfg._hexdump_bytes(bridge.hexdump(address, 6))
        operand_packages: list[CurrentIatStoragePackage] = []
        if len(thunk_bytes) == 6 and thunk_bytes[:2] == b"\xff\x25":
            raw_iat_address = normalize_address(
                struct.unpack_from("<I", thunk_bytes, 2)[0]
            )
            operand_packages = [
                package
                for package in ordinal_packages
                if package.address == raw_iat_address
            ]

        def bridge_identity(row: Any) -> tuple[str, str, str, str, str]:
            return (
                str(getattr(row, "address", "")),
                str(getattr(row, "name", "")),
                str(getattr(row, "raw_name", "")),
                str(getattr(row, "full_name", "")),
                str(getattr(row, "kind", "")),
            )

        address_symbol_identity = bridge_identity(
            bridge_by_address.get(address)
        )
        alias_packages: list[CurrentIatStoragePackage] = []
        for package_candidate in ordinal_packages:
            candidate_callable = (
                package_candidate.object_symbol.removeprefix("__imp_")
            )
            candidate_module = re.sub(
                r"(?i)\.dll$", "", package_candidate.import_dll
            ).upper()
            candidate_ordinal_alias = (
                f"Ordinal_{candidate_module}_{package_candidate.import_ordinal}"
            )
            if (
                not candidate_callable
                or candidate_callable == package_candidate.object_symbol
                or address_symbol_identity[0] != address
                or address_symbol_identity[1]
                not in {candidate_callable, candidate_ordinal_alias}
                or address_symbol_identity[2] != address_symbol_identity[1]
                or not address_symbol_identity[3]
                or address_symbol_identity[4] != "import"
                or any(
                    bridge_identity(bridge_by_name.get(alias))
                    != address_symbol_identity
                    for alias in {
                        address_symbol_identity[1],
                        address_symbol_identity[2],
                        address_symbol_identity[3],
                    }
                )
            ):
                continue
            alias_packages.append(package_candidate)

        activated_packages = {
            package
            for package in (*operand_packages, *alias_packages)
        }
        if not activated_packages:
            continue
        if len(activated_packages) != 1:
            raise ValueError(
                f"terminal ordinal thunk {symbol_id} activates ambiguous "
                "immutable ordinal IAT packages"
            )
        package = next(iter(activated_packages))

        block = blocks.get(block_id)
        mapping = block.get("mapping") if isinstance(block, Mapping) else None
        contribution_ids = (
            block.get("contribution_ids")
            if isinstance(block, Mapping)
            else None
        )
        try:
            block_start = normalize_address(str(block.get("start", "")))
            block_end = normalize_address(str(block.get("end_exclusive", "")))
            symbol_end = normalize_address(
                str(symbol.get("end_exclusive", ""))
            )
        except (ProgressError, AttributeError, TypeError, ValueError):
            raise ValueError(
                f"activated terminal ordinal thunk {symbol_id} has malformed "
                "symbol or provider-block extent"
            )
        if (
            symbol.get("binary") != "recoil"
            or symbol.get("extent_state") != "known"
            or symbol.get("output_section_id") != "recoil:section:.text"
            or not isinstance(block, Mapping)
            or block.get("binary") != "recoil"
            or block.get("row_kind") != "physical-source-block"
            or block.get("contribution_kind") != "provider"
            or not isinstance(mapping, Mapping)
            or mapping.get("status") != "provider-boundary"
            or not isinstance(contribution_ids, list)
            or contribution_ids.count(symbol_id) != 1
            or address_value(block_start) > address_value(address)
            or block_end != terminal
            or address_value(symbol_end) < address_value(terminal)
            or not isinstance(block.get("agent_source_path"), str)
            or not str(block["agent_source_path"]).startswith("provider:")
            or block.get("source_path") != block.get("agent_source_path")
        ):
            raise ValueError(
                f"activated terminal ordinal thunk {symbol_id} lacks one "
                "exact terminal provider block"
            )
        provider_identity = f"provider:{symbol_id}"
        if (
            indexes.by_address.get(address) != provider_identity
            or provider_identity not in indexes.provider_ids
        ):
            raise ValueError(
                f"terminal ordinal thunk {symbol_id} lacks its exact provider "
                "identity"
            )
        if len(thunk_bytes) != 6 or thunk_bytes[:2] != b"\xff\x25":
            raise ValueError(
                f"terminal ordinal thunk {symbol_id} is not exact FF25"
            )
        iat_address = normalize_address(
            struct.unpack_from("<I", thunk_bytes, 2)[0]
        )
        if iat_address != package.address:
            raise ValueError(
                f"terminal ordinal thunk {symbol_id} FF25 operand conflicts "
                "with its activated immutable ordinal IAT package"
            )
        get_json = getattr(bridge, "get_json", None)
        if not callable(get_json):
            raise ValueError(
                f"terminal ordinal thunk {symbol_id} lacks complete inbound "
                "xref authority"
            )

        def one_code_xref(target_address: str, label: str) -> str:
            payload = get_json(
                "getXrefsTo",
                address=target_address,
                limit=100000,
            )
            code_rows, data_rows, complete = _cc_identity._bn_inbound_xref_items(payload)
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
            if (
                not complete
                or data_rows
                or len(source_addresses) != 1
                or not source_addresses[0]
            ):
                raise ValueError(
                    f"terminal ordinal thunk {symbol_id} lacks one complete "
                    f"collision-free {label} xref"
                )
            return source_addresses[0]

        thunk_caller_address = one_code_xref(address, "thunk-caller")
        iat_reader_address = one_code_xref(iat_address, "IAT-reader")
        if iat_reader_address != address:
            raise ValueError(
                f"terminal ordinal thunk {symbol_id} IAT xref does not "
                "rejoin the exact FF25 reader"
            )
        _caller_name, caller_function = _cc_identity._bn_unique_containing_function(
            bridge,
            instruction_address=thunk_caller_address,
        )
        caller_rows = tuple(
            _cc_listing.parse_assembly(bridge.assembly(caller_function), source="bn")
        )
        caller_matches = [
            row
            for row in caller_rows
            if _cc_cfg._source_instruction_address(row) == thunk_caller_address
        ]
        if len(caller_matches) != 1:
            raise ValueError(
                f"terminal ordinal thunk {symbol_id} lacks one exact caller row"
            )
        caller_row = caller_matches[0]
        try:
            caller_body = bytes(int(item, 16) for item in caller_row.bytes)
        except (TypeError, ValueError):
            caller_body = b""
        caller_target = (
            normalize_address(
                address_value(thunk_caller_address)
                + 5
                + struct.unpack_from("<i", caller_body, 1)[0]
            )
            if len(caller_body) == 5 and caller_body[0] == 0xE8
            else ""
        )
        caller_operand = _cc_cfg._instruction_operand(caller_row).strip()
        if (
            _cc_cfg._instruction_mnemonic(caller_row) != "call"
            or caller_target != address
            or (
                caller_operand != str(symbol.get("navigation_name", ""))
                and (
                    not _cc_catalog.ADDRESS_RE.fullmatch(caller_operand)
                    or normalize_address(caller_operand) != address
                )
            )
        ):
            raise ValueError(
                f"terminal ordinal thunk {symbol_id} caller is not one exact "
                "REL32 call to the governed thunk"
            )
        callable_symbol = package.object_symbol.removeprefix("__imp_")
        if (
            not callable_symbol
            or callable_symbol == package.object_symbol
            or (
                not _cc_identity._decorated_coff_name(callable_symbol)
                and re.fullmatch(
                    r"[A-Za-z_][A-Za-z0-9_@$?]*", callable_symbol
                )
                is None
            )
            or indexes.storage_by_address.get(package.address)
            != package.identity
            or indexes.storage_by_name.get(package.object_symbol)
            != package.identity
        ):
            raise ValueError(
                f"terminal ordinal thunk {symbol_id} lacks its exact callable/IAT package"
            )
        retail_name = symbol.get("navigation_name")
        ordinal_module = re.sub(r"(?i)\.dll$", "", package.import_dll).upper()
        ordinal_bn_alias = f"Ordinal_{ordinal_module}_{package.import_ordinal}"
        friendly = (
            _exact_bn_pe_import_friendly_symbol(
                bridge_by_address=bridge_by_address,
                bridge_by_name=bridge_by_name,
                address=address,
                retail_name=retail_name,
                immutable_import_name=ordinal_bn_alias,
            )
            if isinstance(retail_name, str) and retail_name
            else None
        )
        if friendly is None:
            raise ValueError(
                f"terminal ordinal thunk {symbol_id} lacks one exact BN alias"
            )
        thunk = ProviderOrdinalImportThunk(
            provider_identity=provider_identity,
            thunk_address=address,
            retail_name=str(retail_name),
            callable_symbol=callable_symbol,
            iat_object_symbol=package.object_symbol,
            iat_identity=package.identity,
            import_dll=package.import_dll,
            import_ordinal=int(package.import_ordinal),
        )
        proven.append(thunk)
        names.setdefault(str(retail_name), []).append(friendly)
    for attribute in (
        "provider_identity",
        "thunk_address",
        "retail_name",
        "callable_symbol",
        "iat_object_symbol",
        "iat_identity",
    ):
        values = [getattr(thunk, attribute) for thunk in proven]
        if len(values) != len(set(values)):
            raise ValueError(
                "terminal ordinal thunk discovery has duplicate or colliding "
                f"{attribute.replace('_', ' ')} values"
            )
    return tuple(proven), names


def _ftol_provider_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    provider_identity: str,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    reviewed_candidate_direct_bridges: Mapping[str, str],
    reviewed_loop_vptr_storage_bridges: (
        Mapping[str, ReviewedLoopVptrStorageBridge] | None
    ) = None,
    reviewed_absolute_storage_load_bridges: (
        Mapping[str, ReviewedAbsoluteStorageLoadBridge] | None
    ) = None,
) -> dict[str, str]:
    matching_instructions: list[tuple[int, Instruction]] = []
    for instruction_index, instruction in enumerate(candidate.instructions):
        if _cc_cfg._instruction_mnemonic(instruction) not in {"call", "jmp"}:
            continue
        operand = _cc_cfg._instruction_operand(instruction).strip()
        if operand == _cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL:
            matching_instructions.append((instruction_index, instruction))
        elif (
            _cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL in operand
            and _cc_catalog.MSVC_FTOL_IAT_OBJECT_SYMBOL not in operand
        ):
            raise ValueError(
                "MSVC _ftol bridge accepts only the exact candidate operand "
                "__ftol; aliases and indirect helper forms are forbidden"
            )
    if not matching_instructions:
        return {}
    caller = candidate.caller_definition
    if (
        provider_identity != _cc_catalog.MSVC_FTOL_PROVIDER_IDENTITY
        or indexes.by_address.get(_cc_catalog.MSVC_FTOL_PROVIDER_ADDRESS)
        != provider_identity
        or provider_identity not in indexes.provider_ids
        or indexes.by_address.get(_cc_catalog.MSVC_FTOL_IAT_ADDRESS)
        != _cc_catalog.MSVC_FTOL_IAT_PROVIDER_IDENTITY
        or _cc_catalog.MSVC_FTOL_IAT_PROVIDER_IDENTITY not in indexes.provider_ids
        or indexes.storage_by_address.get(_cc_catalog.MSVC_FTOL_IAT_ADDRESS)
        != _cc_catalog.MSVC_FTOL_IAT_STORAGE_IDENTITY
        or indexes.storage_by_name.get(_cc_catalog.MSVC_FTOL_IMPORT_NAME)
        != _cc_catalog.MSVC_FTOL_IAT_STORAGE_IDENTITY
        or indexes.storage_by_name.get(_cc_catalog.MSVC_FTOL_IAT_OBJECT_SYMBOL)
        != _cc_catalog.MSVC_FTOL_IAT_STORAGE_IDENTITY
        or caller is None
        or _cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL in indexes.by_candidate_name
        or _cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL in bridge_names
        or _cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL in indexes.storage_by_name
    ):
        raise ValueError(
            "MSVC _ftol bridge lacks the exact collision-free governed "
            "provider identity"
        )
    if (
        caller.undefined_external_functions.count(
            _cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL
        )
        != 1
    ):
        raise ValueError(
            "MSVC _ftol bridge requires exactly one undefined external "
            "function symbol __ftol in the caller object"
        )
    prior_ftol_identity = reviewed_candidate_direct_bridges.get(
        _cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL
    )
    if (
        prior_ftol_identity is not None
        and prior_ftol_identity != provider_identity
    ):
        raise ValueError(
            "MSVC _ftol bridge conflicts with an already-reviewed candidate "
            "direct bridge"
        )
    expected_rows = [
        row
        for row in expected
        if row.get("target_identity") == provider_identity
    ]
    references = tuple(
        sorted(
            (
                relocation
                for relocation in caller.relocations
                if relocation.symbol_name == _cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL
            ),
            key=lambda relocation: relocation.offset,
        )
    )
    population = len(matching_instructions)
    candidate_call_offsets = tuple(
        address_value(_cc_cfg._source_instruction_address(instruction))
        for _, instruction in matching_instructions
    )
    relocation_offsets = tuple(reference.offset for reference in references)
    zinterp_candidate_only_population = (
        caller_identity == "symbol:recoil:function:0x4c20a0"
        and normalize_address(caller_start) == "0x4c20a0"
        and normalize_address(caller_end_exclusive) == "0x4c5480"
        and population == 2
        and len(set(candidate_call_offsets)) == population
        and relocation_offsets
        == tuple(call_offset + 1 for call_offset in candidate_call_offsets)
        and not expected_rows
        and not any(
            row.get("target_identity") in {
                provider_identity,
                _cc_catalog.MSVC_FTOL_IAT_STORAGE_IDENTITY,
            }
            for row in expected
        )
    )
    recoilapp_branch_join_population = (
        caller_identity == "symbol:recoil:function:0x432d60"
        and normalize_address(caller_start) == "0x432d60"
        and normalize_address(caller_end_exclusive) == "0x432e70"
        and population == 2
        and candidate_call_offsets == (0xB4, 0xBF)
        and relocation_offsets == (0xB5, 0xC0)
        and len(expected_rows) == 3
        and tuple(int(row.get("ordinal", -1)) for row in expected_rows)
        == (5, 6, 7)
        and all(
            row.get("form") == "call"
            and row.get("dispatch") == "direct"
            and row.get("identity_kind") == "provider"
            and row.get("target_identity") == provider_identity
            and row.get("storage_identity") == ""
            and row.get("slot_displacement") is None
            and row.get("cleanup_bytes") is None
            for row in expected_rows
        )
    )
    player_source_divergence_population = (
        (
            caller_identity == "symbol:recoil:function:0x426770"
            and normalize_address(caller_start) == "0x426770"
            and normalize_address(caller_end_exclusive) == "0x427140"
            and (
                (
                    population == 3
                    and candidate_call_offsets == (0x24, 0x311, 0x691)
                    and relocation_offsets == (0x25, 0x312, 0x692)
                )
                or (
                    population == 4
                    and candidate_call_offsets
                    in {
                        (0x24, 0x315, 0x33F, 0x731),
                        (0x24, 0x315, 0x33F, 0x735),
                        (0x24, 0x315, 0x33F, 0x737),
                    }
                    and relocation_offsets
                    == tuple(offset + 1 for offset in candidate_call_offsets)
                )
            )
            and bool(expected_rows)
        )
        or (
            caller_identity == "symbol:recoil:function:0x429870"
            and normalize_address(caller_start) == "0x429870"
            and normalize_address(caller_end_exclusive) == "0x429b40"
            and population == 3
            and candidate_call_offsets == (0x8E, 0x10D, 0x279)
            and relocation_offsets == (0x8F, 0x10E, 0x27A)
            and bool(expected_rows)
        )
    )
    if (
        population < 1
        or len(references) != population
        or (
            len(expected_rows) != population
            and not zinterp_candidate_only_population
            and not recoilapp_branch_join_population
            and not player_source_divergence_population
        )
    ):
        raise ValueError(
            "_ftol bridge requires equal nonempty candidate instruction, "
            "retail row, and COFF relocation populations"
        )
    candidate_sequence = tuple(
        (
            "call"
            if _cc_cfg._instruction_mnemonic(instruction) == "call"
            else "tail",
            (
                _cc_cfg._cleanup_after(candidate.instructions, instruction_index)
                if _cc_cfg._instruction_mnemonic(instruction) == "call"
                else None
            ),
        )
        for instruction_index, instruction in matching_instructions
    )
    expected_sequence = tuple(
        (row.get("form"), row.get("cleanup_bytes"))
        for row in expected_rows
    )
    if zinterp_candidate_only_population:
        if candidate_sequence != (("call", None), ("call", None)):
            raise ValueError(
                "MSVC _ftol bridge reviewed zInterp candidate-only call "
                "sequence drifted"
            )
    elif recoilapp_branch_join_population:
        if candidate_sequence != (("call", None), ("call", None)):
            raise ValueError(
                "MSVC _ftol bridge reviewed RecoilApp branch-join sequence drifted"
            )
    elif player_source_divergence_population:
        if (
            candidate_sequence != tuple(("call", None) for _ in references)
            or expected_sequence
            != tuple(("call", None) for _ in expected_rows)
        ):
            raise ValueError(
                "MSVC _ftol bridge reviewed Player source-divergence call "
                "sequence drifted"
            )
    elif (
        candidate_sequence != expected_sequence
        or any(
            row.get("dispatch") != "direct"
            or row.get("identity_kind") != "provider"
            or row.get("target_identity") != provider_identity
            or row.get("storage_identity") != ""
            or row.get("slot_displacement") is not None
            for row in expected_rows
        )
    ):
        raise ValueError(
            "MSVC _ftol bridge does not preserve the relative retail direct "
            "provider call/tail and cleanup sequence"
        )

    relocation_fields: dict[int, int] = {}
    for _, instruction in matching_instructions:
        raw_offset = _cc_cfg._source_instruction_address(instruction)
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        opcode = 0xE8 if mnemonic == "call" else 0xE9
        if (
            not raw_offset
            or tuple(instruction.bytes[:1]) != (f"{opcode:02x}",)
        ):
            raise ValueError(
                "MSVC _ftol bridge requires exact offset-bearing direct "
                "E8 CALL or E9 tail-JMP instructions"
            )
        relocation_offset = address_value(raw_offset) + 1
        if relocation_offset in relocation_fields:
            raise ValueError(
                "MSVC _ftol bridge candidate instruction offsets are not unique"
            )
        relocation_fields[relocation_offset] = opcode
    if {reference.offset for reference in references} != set(relocation_fields):
        raise ValueError(
            "MSVC _ftol bridge COFF relocations are not one-to-one with the "
            "exact candidate instruction offsets"
        )

    for reference in references:
        opcode = relocation_fields[reference.offset]
        field_end = reference.offset + 4
        if (
            reference.type != IMAGE_REL_I386_REL32
            or reference.offset < 1
            or field_end > len(caller.data)
            or field_end > len(caller.relocation_mask)
            or caller.data[reference.offset - 1 : reference.offset]
            != bytes((opcode,))
            or struct.unpack_from("<I", caller.data, reference.offset)[0]
            != 0
            or not all(
                caller.relocation_mask[index]
                for index in range(reference.offset, field_end)
            )
        ):
            raise ValueError(
                "MSVC _ftol bridge requires exact zero-addend fully masked "
                "E8/E9 REL32 relocations"
            )
    return {_cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL: _cc_catalog.MSVC_FTOL_IAT_STORAGE_IDENTITY}
