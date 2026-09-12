"""Recoil call-contract targets evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import errors as _cc_errors
from _recoil.call_contract import listing as _cc_listing
from _recoil.call_contract import recoil_lifecycle as _cc_recoil_lifecycle
from _recoil.call_contract import reporting as _cc_reporting

if TYPE_CHECKING:
    from _recoil.call_contract.records import IdentityIndexes


import re
import struct
from typing import Any, Mapping

from _recoil.commands.asm_verify import Instruction
from _recoil.lib.progress import ProgressError, address_value, normalize_address


def _registered_function_population_matches(
    target: Mapping[str, Any], symbol_ids: tuple[str, ...],
    symbols: Mapping[str, Any],
) -> bool:
    """Prove a current target's exact ordered physical function population."""
    binary = target.get("binary")
    if not symbol_ids or len(set(symbol_ids)) != len(symbol_ids):
        return False
    addresses = []
    for symbol_id in symbol_ids:
        row = symbols.get(symbol_id)
        if not isinstance(row, Mapping):
            return False
        address = row.get("address")
        if (
            row.get("kind") != "function" or row.get("binary") != binary
            or not isinstance(address, str)
            or re.fullmatch(r"0x[0-9a-f]+", address) is None
            or symbol_id != f"{binary}:function:{address}"
        ):
            return False
        addresses.append(address)
    return target.get("registered_addresses") == addresses


def _target_function_definition_source_matches(
    target: Any, registration: Mapping[str, Any], *,
    address: str, symbol: str, source_path: str,
) -> bool:
    """Join one source definition through matching current TU projections.

    A multi-input target's root source is distinct from the TU that defines
    a particular function. Both the parsed manifest and stored registration
    must select exactly the same definition; duplicate or stale routes reject.
    """
    entries = tuple(getattr(target, "translation_unit_function_order", ()))
    stored_entries = registration.get("translation_unit_function_order") or []
    if not entries:
        return (not stored_entries
            and getattr(target, "source_from", "") == source_path
            and registration.get("source_from") == source_path)
    if (not isinstance(stored_entries, list)
            or len(entries) != len(stored_entries)
            or not getattr(target, "check_translation_unit_function_order", False)
            or any(not isinstance(entry, Mapping) for entry in stored_entries)):
        return False
    paths = tuple(getattr(entry, "source_from", "") for entry in entries)
    if (not all(isinstance(path, str) and path for path in paths)
            or len(paths) != len(set(paths))
            or paths != tuple(entry.get("source_from") for entry in stored_entries)):
        return False
    manifest_rows = [(getattr(entry, "source_from", ""), row)
        for entry in entries for row in getattr(entry, "functions", ())
        if getattr(row, "address", "") == address]
    stored_rows = [(entry.get("source_from"), row)
        for entry in stored_entries for row in entry.get("functions", ())
        if isinstance(row, Mapping) and row.get("address") == address]
    return (len(manifest_rows) == len(stored_rows) == 1
        and manifest_rows[0][0] == stored_rows[0][0] == source_path
        and getattr(manifest_rows[0][1], "symbol", "") == symbol
        and stored_rows[0][1].get("symbol") == symbol)


def _registered_target_artifact_ids(
    target: Mapping[str, Any], symbols: Mapping[str, Any],
) -> list[str] | None:
    """Resolve the current ordered address census to exact typed artifacts.

    Verification targets store addresses, not cached symbol-id or unresolved
    lists. Require one physical function/data identity per address; a missing,
    duplicate, cross-binary, wrong-kind or ambiguous entry supplies no proof.
    This derives membership only and accepts no provider, owner or byte facts.
    """
    binary = target.get("binary")
    addresses = target.get("registered_addresses")
    if (
        not isinstance(binary, str) or not binary
        or not isinstance(addresses, list) or not addresses
        or any(not isinstance(address, str) for address in addresses)
        or len(set(addresses)) != len(addresses)
    ):
        return None
    result = []
    for address in addresses:
        if re.fullmatch(r"0x[0-9a-f]+", address) is None:
            return None
        matches = []
        for family, kinds in (
            ("function", {"function", "provider-function"}),
            ("data", {"data", "provider-data"}),
        ):
            symbol_id = f"{binary}:{family}:{address}"
            row = symbols.get(symbol_id)
            if row is None:
                continue
            if (
                not isinstance(row, Mapping)
                or row.get("binary") != binary
                or row.get("kind") not in kinds
                or row.get("address") != address
            ):
                return None
            matches.append(symbol_id)
        if len(matches) != 1:
            return None
        result.append(matches[0])
    return result


def _registered_artifact_target_memberships_match(
    symbol_id: str, symbols: Mapping[str, Any], targets: Mapping[str, Any],
) -> bool:
    """Validate attached verification references without treating them as owners.

    A pooled literal can acquire more verification consumers without changing
    its physical identity. Every declared target must still select that exact
    typed artifact once; absent, duplicate or conflicting references reject.
    Unrelated target entries stay outside this relationship proof. This check
    accepts neither their identities nor any storage, source or provider facts.
    """
    symbol = symbols.get(symbol_id)
    if not isinstance(symbol, Mapping):
        return False
    address = symbol.get("address")
    if _registered_target_artifact_ids(
        {"binary": symbol.get("binary"), "registered_addresses": [address]}, symbols,
    ) != [symbol_id]:
        return False
    target_ids = symbol.get("verification_target_ids")
    if (not isinstance(target_ids, list)
            or any(not isinstance(item, str) or not item for item in target_ids)
            or len(target_ids) != len(set(target_ids))):
        return False
    for target_id in target_ids:
        target = targets.get(target_id)
        if (not isinstance(target, Mapping) or target.get("kind") != "vc5"
                or target.get("binary") != symbol.get("binary")):
            return False
        addresses = target.get("registered_addresses")
        if (not isinstance(addresses, list)
                or any(not isinstance(item, str)
                       or re.fullmatch(r"0x[0-9a-f]+", item) is None for item in addresses)
                or len(addresses) != len(set(addresses))
                or addresses.count(address) != 1):
            return False
    return True


def _pointer_vector_destroy_provider_identity(
    name: str,
    *,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    compiler_generated_bridges: Mapping[str, str],
    allow_proven_value_noop: bool = False,
) -> str:
    """Resolve one exact no-op vector ``_Destroy`` ABI to reviewed retail.

    Pointer elements retain their established decoration-level route because
    destroying a pointer range is intrinsically a no-op.  A value element is
    eligible only after the caller-local COFF bridge proves the complete VC5
    no-op COMDAT, or when that proof is already present in the reviewed
    compiler/provider bridge map.  Thus a value-vector decoration alone never
    acquires the physical provider identity.
    """

    if not name.startswith(_cc_catalog.MSVC_VECTOR_DESTROY_PREFIX):
        return ""
    if name in {
        _cc_catalog.HUD_PANEL_LAYOUT_VECTOR_DESTROY_SYMBOL,
        _cc_catalog.HUD_COMPOSITE_PANEL_VECTOR_DESTROY_SYMBOL,
    }:
        # This specialization destroys nontrivial values and therefore cannot
        # share the pointer/no-op supplier.  Its exact same-object sentinel is
        # resolved only by the later comparison-scoped retail COMDAT gate.
        return ""
    match = _cc_catalog.MSVC_VECTOR_NOOP_DESTROY_RE.fullmatch(name)
    if match is None:
        raise ValueError(
            f"ABI-incompatible vector destroy decoration {name!r}"
        )
    is_pointer_element = match.group("element").startswith("P")
    compiler_identity = compiler_generated_bridges.get(name)
    if (
        not is_pointer_element
        and not allow_proven_value_noop
        and compiler_identity is None
    ):
        return ""
    suppliers = indexes.pointer_vector_destroy_providers
    if not suppliers:
        raise ValueError(
            f"missing reviewed pointer-vector destroy provider for {name!r}"
        )
    if len(suppliers) != 1:
        raise ValueError(
            f"ambiguous reviewed pointer-vector destroy provider for {name!r}"
        )
    supplier = suppliers[0]
    if (
        supplier.stack_cleanup_bytes != 8
        or _cc_catalog.MSVC_POINTER_VECTOR_DESTROY_PROVIDER_RE.fullmatch(
            supplier.catalog_symbol
        )
        is None
        or indexes.by_address.get(supplier.address) != supplier.identity
        or supplier.identity not in indexes.provider_ids
    ):
        raise ValueError(
            "ABI-incompatible reviewed pointer-vector destroy provider "
            f"for {name!r}"
        )

    indexed = indexes.by_candidate_name.get(name)
    if indexed is not None and indexed != supplier.identity:
        raise ValueError(
            f"conflicting indexed pointer-vector destroy identity for {name!r}"
        )
    if (
        compiler_identity is not None
        and compiler_identity != supplier.identity
    ):
        raise ValueError(
            f"conflicting compiler pointer-vector destroy identity for {name!r}"
        )
    raw_matches = bridge_names.get(name)
    if raw_matches is not None:
        matches = (
            list(raw_matches)
            if isinstance(
                raw_matches,
                (list, tuple, set, frozenset),
            )
            else [raw_matches]
        )
        try:
            addresses = {
                normalize_address(str(getattr(item, "address", "")))
                for item in matches
                if getattr(item, "address", "")
            }
        except ProgressError as exc:
            raise ValueError(
                f"conflicting BN pointer-vector destroy identity for {name!r}"
            ) from exc
        if addresses != {supplier.address}:
            raise ValueError(
                f"conflicting BN pointer-vector destroy identity for {name!r}"
            )
    return supplier.identity


def _hud_cmd_binding_ptr_vector_erase_provider_identity(
    name: str,
    *,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    compiler_generated_bridges: Mapping[str, str],
) -> str:
    """Resolve only the reviewed HudCmdBindingEntry pointer-vector erase ABI."""

    if name != _cc_catalog.HUD_CMD_BINDING_PTR_VECTOR_ERASE_SYMBOL:
        return ""
    supplier = indexes.hud_cmd_binding_ptr_vector_erase_provider
    if supplier is None:
        raise ValueError(
            "missing reviewed physical HudCmdBindingEntry pointer-vector "
            f"erase provider for {name!r}"
        )
    if (
        supplier.candidate_name != _cc_catalog.HUD_CMD_BINDING_PTR_VECTOR_ERASE_SYMBOL
        or supplier.address != _cc_catalog.HUD_CMD_BINDING_PTR_VECTOR_ERASE_ADDRESS
        or supplier.identity != _cc_catalog.HUD_CMD_BINDING_PTR_VECTOR_ERASE_IDENTITY
        or supplier.stack_cleanup_bytes != 8
        or indexes.by_address.get(supplier.address) != supplier.identity
        or supplier.identity not in indexes.provider_ids
    ):
        raise ValueError(
            "ABI-incompatible reviewed physical HudCmdBindingEntry "
            f"pointer-vector erase provider for {name!r}"
        )
    indexed = indexes.by_candidate_name.get(name)
    if indexed is not None and indexed != supplier.identity:
        raise ValueError(
            "conflicting indexed HudCmdBindingEntry pointer-vector erase "
            f"identity for {name!r}"
        )
    compiler_identity = compiler_generated_bridges.get(name)
    if (
        compiler_identity is not None
        and compiler_identity != supplier.identity
    ):
        raise ValueError(
            "conflicting compiler HudCmdBindingEntry pointer-vector erase "
            f"identity for {name!r}"
        )
    raw_matches = bridge_names.get(name)
    if raw_matches is not None:
        matches = (
            list(raw_matches)
            if isinstance(
                raw_matches,
                (list, tuple, set, frozenset),
            )
            else [raw_matches]
        )
        try:
            addresses = [
                normalize_address(str(getattr(item, "address", "")))
                for item in matches
                if getattr(item, "address", "")
            ]
        except ProgressError as exc:
            raise ValueError(
                "conflicting BN HudCmdBindingEntry pointer-vector erase "
                f"identity for {name!r}"
            ) from exc
        if addresses != [supplier.address]:
            raise ValueError(
                "conflicting BN HudCmdBindingEntry pointer-vector erase "
                f"identity for {name!r}"
            )
    return supplier.identity


def _scalar_deleting_destructor_identity(
    name: str,
    *,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    compiler_generated_bridges: Mapping[str, str],
) -> str:
    """Resolve exact natural VC5 scalar deletion to reviewed retail identity."""
    if not name.startswith(_cc_catalog.MSVC_SCALAR_DELETING_DESTRUCTOR_PREFIX):
        return ""
    class_match = _cc_catalog.MSVC_SCALAR_DELETING_DESTRUCTOR_CLASS_RE.match(name)
    if class_match is None:
        return ""
    encoded_class = class_match.group("class_scope")
    class_identity = "::".join(reversed(encoded_class.split("@")))
    if _cc_catalog.MSVC_VIRTUAL_SCALAR_DELETING_DESTRUCTOR_RE.fullmatch(name):
        return ""
    suppliers = [
        supplier
        for supplier in indexes.scalar_deleting_destructors
        if supplier.class_identity == class_identity
    ]
    if len(suppliers) > 1:
        raise ValueError(
            "ambiguous reviewed scalar-deleting-destructor identity "
            f"for {name!r}"
        )
    if not suppliers:
        blocker = indexes.scalar_deleting_destructor_blockers.get(
            class_identity
        )
        if blocker:
            raise ValueError(f"{blocker} for {name!r}")
        return ""
    parsed = _cc_recoil_lifecycle._nonvirtual_scalar_deleting_destructor_class(name)
    if parsed is None:
        raise ValueError(
            f"ABI-incompatible scalar-deleting-destructor decoration {name!r}"
        )
    supplier = suppliers[0]
    if (
        not supplier.identity
        or indexes.by_address.get(supplier.address) != supplier.identity
    ):
        raise ValueError(
            f"stale scalar-deleting-destructor identity for {name!r}"
        )
    indexed = indexes.by_candidate_name.get(name)
    if (
        indexed is not None
        and indexed != supplier.identity
        and not (
            indexed == ""
            and name
            in indexes.scalar_deleting_destructor_legacy_fallback_names
        )
    ):
        raise ValueError(
            f"conflicting indexed scalar-deleting-destructor identity for {name!r}"
        )
    compiler_identity = compiler_generated_bridges.get(name)
    if (
        compiler_identity is not None
        and compiler_identity != supplier.identity
    ):
        raise ValueError(
            f"conflicting compiler scalar-deleting-destructor identity for {name!r}"
        )
    raw_matches = bridge_names.get(name)
    if raw_matches is not None:
        matches = (
            list(raw_matches)
            if isinstance(raw_matches, (list, tuple, set, frozenset))
            else [raw_matches]
        )
        try:
            addresses = {
                normalize_address(str(getattr(item, "address", "")))
                for item in matches
                if getattr(item, "address", "")
            }
        except ProgressError as exc:
            raise ValueError(
                f"conflicting BN scalar-deleting-destructor identity for {name!r}"
            ) from exc
        if addresses != {supplier.address}:
            raise ValueError(
                f"conflicting BN scalar-deleting-destructor identity for {name!r}"
            )
    return supplier.identity


def _is_zsnd_release_unknown_coff_name(value: str) -> bool:
    """Recognize the exact VC5 TU-local ReleaseUnknown identity grammar."""

    return _cc_catalog._ZSND_RELEASE_UNKNOWN_COFF_NAME_RE.fullmatch(value) is not None


def _cod_space_bearing_direct_target(
    operand: str,
    source_line: str,
) -> str | None:
    """Return one exact VC5 COFF identity for a space-bearing COD target."""

    candidate = operand.strip()
    if not candidate.startswith("?"):
        return None
    has_space = any(character.isspace() for character in candidate)
    has_drive_scope = re.search(
        r"@\?[A-Za-z]:[\\/]", candidate, re.IGNORECASE
    ) is not None
    if not has_space:
        if has_drive_scope and not candidate.endswith("Z"):
            raise ValueError(
                "invalid or truncated COD space-bearing decorated call "
                f"target operand {operand!r}"
            )
        return None
    operand_match = _cc_catalog._COD_SPACE_BEARING_DIRECT_TARGET_RE.fullmatch(candidate)
    if operand_match is None:
        raise ValueError(
            "invalid or ambiguous COD space-bearing decorated call target "
            f"operand {operand!r}"
        )
    source_prefix, separator, comment = source_line.rpartition(" ; ")
    if (
        not separator
        or not source_prefix.rstrip().endswith(candidate)
        or source_prefix.rstrip().count(candidate) != 1
    ):
        raise ValueError(
            "COD space-bearing decorated call target lacks one exact "
            f"source-line termination for {operand!r}"
        )
    comment_match = _cc_catalog._COD_SPACE_BEARING_DIRECT_TARGET_COMMENT_RE.fullmatch(
        comment.strip()
    )
    if comment_match is None:
        raise ValueError(
            "COD space-bearing decorated call target lacks one exact "
            f"percent-marked identity comment for {operand!r}"
        )
    canonical_coff_name = comment_match.group(0)
    if (
        canonical_coff_name
        != (
            f"{operand_match.group('head')}%"
            f"{operand_match.group('source')}"
            f"{operand_match.group('scope')}"
            f"{operand_match.group('suffix')}"
        )
        or _cc_listing._vc5_compiler_normalized_cod_proc_symbol(canonical_coff_name)
        != candidate
    ):
        raise ValueError(
            "COD space-bearing decorated call target operand and "
            "percent-marked canonical COFF identity do not agree exactly: "
            f"{operand!r}"
        )
    return canonical_coff_name


def _canonical_direct_identity(
    operand: str,
    *,
    source: str,
    caller_identity: str,
    caller_start: int,
    caller_end: int,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    compiler_generated_bridges: Mapping[str, str],
    call_site_address: str | int | None = None,
    call_site_unique: bool | None = None,
    retail_call_site_address: str | int | None = None,
    cod_source_line: str = "",
    retail_instruction: Instruction | None = None,
    call_only_icf_proved: bool = False,
) -> tuple[str, str]:
    from _recoil.lib.call_only_icf import require
    selected_site = call_site_address if source == "bn" else retail_call_site_address
    selected_site = normalize_address(selected_site) if selected_site is not None else ""
    call_only = indexes.call_only_icf_by_site.get(selected_site)
    candidate_call_only = [contract for contract in indexes.call_only_icf_by_site.values()
                           if contract["object_symbol"] == operand.strip()]
    if call_only is not None or (source == "cod" and candidate_call_only):
        require(call_only is not None and call_only_icf_proved and call_site_unique is True
                and caller_identity == "symbol:" + call_only["caller_id"]
                and caller_start == address_value(call_only["caller_address"])
                and caller_end == address_value(call_only["caller_end_exclusive"]),
                "call target requires its exact proved caller/site")
        if source == "cod":
            require(operand.strip() == call_only["object_symbol"], "candidate call target changed")
        else:
            require(retail_instruction is not None, "retail opcode is missing")
            raw = bytes.fromhex(" ".join(retail_instruction.bytes))
            require(len(raw) == 5 and raw[0] == 0xe8
                    and address_value(selected_site) + 5 + struct.unpack_from('<i', raw, 1)[0]
                    == address_value(call_only["physical_address"]), "retail call target changed")
        return "direct", "logical:" + call_only["logical_id"]
    if "__imp_" in operand:
        match = _cc_catalog.DECORATED_RE.search(operand)
        if match is None:
            raise ValueError(f"unresolved IAT identity in {operand!r}")
        return "iat", f"iat:{match.group(0).removeprefix('__imp_')}"
    address_matches = _cc_catalog.ADDRESS_RE.findall(operand)
    reviewed_import_thunk: tuple[str, str] | None = None
    if source == "bn" and call_site_address is not None:
        call_site = normalize_address(call_site_address)
        reviewed_import_thunk = (
            indexes.reviewed_direct_import_thunk_by_call_site.get(call_site)
        )
        exact_local_import_thunk = (
            indexes.reviewed_exact_local_import_thunk_by_call_site.get(
                call_site
            )
        )
        if exact_local_import_thunk is not None:
            if (
                exact_local_import_thunk != reviewed_import_thunk
                or not exact_local_import_thunk[1].startswith("iat:")
            ):
                raise ValueError(
                    f"exact local import-thunk selector {call_site} conflicts "
                    "with its composed IAT identity"
                )
            return "iat", exact_local_import_thunk[1]
        if reviewed_import_thunk is not None and not address_matches:
            expected_thunk_address, target_identity = reviewed_import_thunk
            friendly_name = re.sub(
                r"^(?:(?:near|far)\s+)?(?:ptr\s+)?",
                "",
                operand.strip(),
                flags=re.IGNORECASE,
            ).strip()
            raw_matches = bridge_names.get(friendly_name)
            matches = (
                list(raw_matches)
                if isinstance(raw_matches, (list, tuple, set, frozenset))
                else ([raw_matches] if raw_matches is not None else [])
            )
            try:
                addresses = {
                    normalize_address(str(getattr(item, "address", "")))
                    for item in matches
                    if getattr(item, "address", "")
                }
            except ProgressError as exc:
                raise ValueError(
                    f"direct import-thunk selector {call_site} has malformed "
                    "friendly-name authority"
                ) from exc
            identity_kind = (
                "iat"
                if target_identity.startswith("iat:")
                else (
                    "provider"
                    if indexes.by_address.get(expected_thunk_address)
                    == target_identity
                    and target_identity in indexes.provider_ids
                    else ""
                )
            )
            if addresses != {expected_thunk_address} or not identity_kind:
                raise ValueError(
                    f"direct import-thunk selector {call_site} does not join "
                    f"friendly target {friendly_name!r} to "
                    f"{expected_thunk_address}"
                )
            return identity_kind, target_identity
    if address_matches:
        address = normalize_address(address_matches[-1])
        value = address_value(address)
        if caller_start <= value < caller_end:
            return "self", caller_identity
        if source == "bn":
            if call_site_address is not None:
                call_site = normalize_address(call_site_address)
                import_thunk = reviewed_import_thunk
                if import_thunk is not None:
                    expected_thunk_address, target_identity = import_thunk
                    identity_kind = (
                        "iat"
                        if target_identity.startswith("iat:")
                        else (
                            "provider"
                            if indexes.by_address.get(expected_thunk_address)
                            == target_identity
                            and target_identity in indexes.provider_ids
                            else ""
                        )
                    )
                    if expected_thunk_address != address or not identity_kind:
                        raise ValueError(
                            f"direct import-thunk selector {call_site} expected "
                            f"physical target {expected_thunk_address}, found "
                            f"{address}"
                        )
                    return identity_kind, target_identity
                selected = indexes.reviewed_authored_icf_by_call_site.get(call_site)
                if selected is not None:
                    expected_physical_address, logical_identity = selected
                    if expected_physical_address != address:
                        raise ValueError(
                            f"authored ICF retail call selector {call_site} expected "
                            f"physical target {expected_physical_address}, found {address}"
                        )
                    return "direct", logical_identity
            group_identity = indexes.reviewed_icf_group_by_address.get(
                address, ""
            )
            if group_identity:
                return "direct", group_identity
            logical_identity = (
                indexes.reviewed_non_gating_logical_target_by_address.get(
                    address, ""
                )
            )
            if logical_identity:
                return "direct", logical_identity
        identity = indexes.by_address.get(address, "")
        if not identity:
            raise ValueError(f"unresolved retail call target identity {address}")
        return ("provider" if identity in indexes.provider_ids else "direct"), identity
    if source == "bn":
        friendly_name = re.sub(
            r"^(?:(?:near|far)\s+)?(?:ptr\s+)?",
            "",
            operand.strip(),
            flags=re.IGNORECASE,
        ).strip()
        exact_name_kind = "friendly" if "::" in friendly_name else "exact-name"
        if (
            _cc_catalog.DECORATED_RE.fullmatch(friendly_name) is None
            and not any(
                token in friendly_name
                for token in ("[", "]", "+", "-", "*", "/")
            )
            and not any(character.isspace() for character in friendly_name)
        ):
            raw_matches = bridge_names.get(friendly_name)
            if raw_matches is None:
                raise ValueError(
                    f"unresolved bn {exact_name_kind} call target bridge identity "
                    f"{friendly_name!r}"
                )
            matches = (
                list(raw_matches)
                if isinstance(raw_matches, (list, tuple, set, frozenset))
                else [raw_matches]
            )
            addresses = {
                normalize_address(str(getattr(item, "address", "")))
                for item in matches
                if getattr(item, "address", "")
            }
            if len(addresses) > 1 and retail_instruction is not None:
                # Overloads may have identical BN display names. Only the
                # exact direct transfer encoding at this unique retail site
                # may select one of those already indexed physical entries.
                try:
                    body = bytes(int(token, 16) for token in retail_instruction.bytes)
                    mnemonic = _cc_cfg._instruction_mnemonic(retail_instruction)
                    site = address_value(call_site_address) if call_site_address is not None else -1
                    width = 4 if len(body) == 5 and body[0] in (0xE8, 0xE9) else (
                        1 if len(body) == 2 and body[0] == 0xEB else 0)
                    valid = (call_site_unique is True and caller_start <= site
                        and site + len(body) <= caller_end and width
                        and _cc_cfg._exact_invocation_encoding(retail_instruction, mnemonic=mnemonic)
                        and mnemonic == ("call" if body[0] == 0xE8 else "jmp"))
                    selected_address = normalize_address(
                        (site + len(body) + int.from_bytes(body[1:], "little", signed=True)) & 0xFFFFFFFF
                    ) if valid else ""
                except (TypeError, ValueError, ProgressError):
                    selected_address = ""
                if selected_address in addresses:
                    matches = [item for item in matches
                        if getattr(item, "address", "")
                        and normalize_address(str(item.address)) == selected_address]
                    addresses = {selected_address}
            if len(addresses) != 1:
                raise ValueError(
                    f"ambiguous bn {exact_name_kind} call target identity "
                    f"{friendly_name!r}"
                )
            address = next(iter(addresses))
            if call_site_address is not None:
                call_site = normalize_address(call_site_address)
                selected = indexes.reviewed_authored_icf_by_call_site.get(call_site)
                if selected is not None:
                    expected_physical_address, logical_identity = selected
                    if expected_physical_address != address:
                        raise ValueError(
                            f"authored ICF retail call selector {call_site} expected "
                            f"physical target {expected_physical_address}, found {address}"
                        )
                    return "direct", logical_identity
            group_identity = indexes.reviewed_icf_group_by_address.get(
                address, ""
            )
            if group_identity:
                return "direct", group_identity
            exact_bridge_names = {friendly_name}
            for item in matches:
                exact_bridge_names.update((
                    str(value)
                    for value in (
                        getattr(item, "name", ""),
                        getattr(item, "raw_name", ""),
                        getattr(item, "full_name", ""),
                    )
                    if value
                ))
            matching_aliases = [
                (alias_address, alias)
                for alias_address, aliases in (
                    indexes.reviewed_logical_aliases_by_address.items()
                )
                for alias in aliases
                if (
                    alias.original_name in exact_bridge_names
                    or alias.object_symbol in exact_bridge_names
                )
            ]
            aliases_at_address = [
                alias
                for alias_address, alias in matching_aliases
                if alias_address == address
            ]
            alias_identities = {
                alias.identity for alias in aliases_at_address
            }
            if len(alias_identities) > 1:
                raise ValueError(
                    f"ambiguous reviewed logical alias for bn {exact_name_kind} "
                    f"call target "
                    f"{friendly_name!r} at {address}"
                )
            non_gating_identity = (
                indexes.reviewed_non_gating_logical_target_by_address.get(
                    address, ""
                )
            )
            if non_gating_identity:
                physical_identity = indexes.by_address.get(address, "")
                if (
                    non_gating_identity in indexes.provider_ids
                    or physical_identity not in {"", non_gating_identity}
                    or (
                        alias_identities
                        and alias_identities != {non_gating_identity}
                    )
                ):
                    raise ValueError(
                        "conflicting reviewed non-gating logical target for "
                        f"bn {exact_name_kind} call target "
                        f"{friendly_name!r} at {address}"
                    )
                if matching_aliases and not aliases_at_address:
                    raise ValueError(
                        "reviewed logical alias for bn "
                        f"{exact_name_kind} call target "
                        f"{friendly_name!r} resolves at a different address "
                        f"than {address}"
                    )
                if (
                    indexes.reviewed_logical_aliases_by_address.get(address)
                    and not aliases_at_address
                ):
                    raise ValueError(
                        "missing exact reviewed logical alias for bn "
                        f"{exact_name_kind} call target "
                        f"{friendly_name!r} at {address}"
                    )
                return "direct", non_gating_identity
            if len(alias_identities) == 1:
                identity = next(iter(alias_identities))
                return "direct", identity
            if matching_aliases:
                raise ValueError(
                    f"reviewed logical alias for bn {exact_name_kind} call target "
                    f"{friendly_name!r} resolves at a different address than {address}"
                )
            if indexes.reviewed_logical_aliases_by_address.get(address):
                raise ValueError(
                    f"missing exact reviewed logical alias for bn {exact_name_kind} "
                    f"call target "
                    f"{friendly_name!r} at {address}"
                )
            identity = indexes.by_address.get(address, "")
            if not identity:
                raise ValueError(
                    f"unresolved bn {exact_name_kind} call target tracker identity "
                    f"{friendly_name!r} at {address}"
                )
            return (
                "provider" if identity in indexes.provider_ids else "direct"
            ), identity
    cod_space_bearing_name = (
        _cod_space_bearing_direct_target(operand, cod_source_line)
        if source == "cod"
        else None
    )
    decorated_match = _cc_catalog.DECORATED_RE.search(operand)
    decorated_name = (
        cod_space_bearing_name
        if cod_space_bearing_name is not None
        else (decorated_match.group(0) if decorated_match is not None else None)
    )
    if source == "cod" and decorated_name is not None:
        physical_provider_identity = (
            _hud_cmd_binding_ptr_vector_erase_provider_identity(
                decorated_name,
                indexes=indexes,
                bridge_names=bridge_names,
                compiler_generated_bridges=compiler_generated_bridges,
            )
        )
        if physical_provider_identity:
            return "provider", physical_provider_identity
        scalar_identity = _scalar_deleting_destructor_identity(
            decorated_name,
            indexes=indexes,
            bridge_names=bridge_names,
            compiler_generated_bridges=compiler_generated_bridges,
        )
        if scalar_identity:
            return "direct", scalar_identity
        provider_identity = _pointer_vector_destroy_provider_identity(
            decorated_name,
            indexes=indexes,
            bridge_names=bridge_names,
            compiler_generated_bridges=compiler_generated_bridges,
        )
        if provider_identity:
            return "provider", provider_identity
    exact_compiler_identities = (
        {
            identity
            for lookup_name in {
                cod_space_bearing_name or operand.strip(),
                decorated_name or "",
            }
            if lookup_name
            for identity in (compiler_generated_bridges.get(lookup_name, ""),)
            if identity
        }
        if source == "cod"
        else set()
    )
    if len(exact_compiler_identities) > 1:
        raise ValueError(
            "compiler-generated candidate bridge has conflicting exact "
            "operand and decorated-symbol identities"
        )
    exact_compiler_identity = (
        next(iter(exact_compiler_identities))
        if exact_compiler_identities else ""
    )
    if exact_compiler_identity:
        return (
            "provider"
            if exact_compiler_identity in indexes.provider_ids
            else "direct"
        ), exact_compiler_identity
    if decorated_name is not None:
        name = decorated_name
        if source == "cod" and name in indexes.candidate_only_names:
            r4564_set_enabled_identity = (
                "logical:recoil:logical-function:0x42ee40:"
                "hud-ui-background-container-set-enabled"
            )
            if (
                name
                == "?SetEnabled@HudUiBackgroundContainer@@UAEXH@Z"
                and caller_identity
                == "symbol:recoil:function:0x4b9900"
                and caller_start == 0x4B9900
                and caller_end == 0x4BA020
                and call_site_unique is True
                and call_site_address is not None
                and normalize_address(call_site_address)
                == "0x4b9e67"
                and indexes.reviewed_authored_icf_physical_by_logical_identity.get(
                    r4564_set_enabled_identity
                )
                == "recoil:function:0x42ee40"
                and r4564_set_enabled_identity not in indexes.provider_ids
            ):
                return "direct", r4564_set_enabled_identity
            provisional_selection = (
                indexes.reviewed_authored_icf_provisional_candidate_by_name.get(
                    name
                )
            )
            if provisional_selection is not None:
                physical_symbol_id, logical_identity = provisional_selection
                if (
                    indexes.reviewed_authored_icf_physical_by_logical_identity.get(
                        logical_identity
                    )
                    == physical_symbol_id
                ):
                    return "direct", logical_identity
                message = (
                    "unresolved cod decorated call target bridge identity "
                    f"{name!r}: reviewed provisional authored ICF candidate "
                    "selection has incomplete physical/logical population"
                )
                raise _cc_errors.CandidateCallTargetBridgeError(
                    decorated_identity=name,
                    decoded_identity=(
                        _cc_reporting._decode_candidate_dependent_callable_identity(name)
                    ),
                    call_site_address=call_site_address,
                    message=message,
                    call_site_unique=call_site_unique,
                )
            call_site = (
                normalize_address(retail_call_site_address)
                if retail_call_site_address is not None
                else ""
            )
            candidate_selection = (
                indexes.reviewed_authored_icf_candidate_by_call_site.get(
                    call_site
                )
                if call_site and call_site_unique is True
                else None
            )
            selected = (
                indexes.reviewed_authored_icf_by_call_site.get(call_site)
                if call_site
                else None
            )
            if candidate_selection is not None:
                (
                    physical_address,
                    physical_symbol_id,
                    selected_object_symbol,
                    logical_identity,
                ) = candidate_selection
                if (
                    selected == (physical_address, logical_identity)
                    and selected_object_symbol == name
                    and indexes.reviewed_authored_icf_physical_by_logical_identity.get(
                        logical_identity
                    )
                    == physical_symbol_id
                ):
                    return "direct", logical_identity
            message = (
                "unresolved cod decorated call target bridge identity "
                f"{name!r}: authored ICF candidate selection requires one "
                "exact target/object-symbol/retail-callsite population"
            )
            raise _cc_errors.CandidateCallTargetBridgeError(
                decorated_identity=name,
                decoded_identity=(
                    _cc_reporting._decode_candidate_dependent_callable_identity(name)
                ),
                call_site_address=call_site_address,
                message=message,
                call_site_unique=call_site_unique,
            )
        identity = (
            ""
            if source == "bn" and name in indexes.candidate_only_names
            else indexes.by_candidate_name.get(name, "")
        )
        if (
            not identity
            and source == "cod"
            and name not in indexes.by_candidate_name
        ):
            identity = compiler_generated_bridges.get(name, "")
        if not identity:
            raw_matches = bridge_names.get(name)
            if raw_matches is None:
                message = (
                    f"unresolved {source} decorated call target bridge identity "
                    f"{name!r}"
                )
                decoded_identity = (
                    _cc_reporting._decode_candidate_dependent_callable_identity(name)
                    if source == "cod"
                    else None
                )
                if source == "cod":
                    raise _cc_errors.CandidateCallTargetBridgeError(
                        decorated_identity=name,
                        decoded_identity=decoded_identity,
                        call_site_address=call_site_address,
                        message=message,
                        call_site_unique=call_site_unique,
                    )
                raise ValueError(message)
            matches = (
                list(raw_matches)
                if isinstance(raw_matches, (list, tuple, set, frozenset))
                else [raw_matches]
            )
            addresses = {
                normalize_address(str(getattr(item, "address", "")))
                for item in matches
                if getattr(item, "address", "")
            }
            if not addresses:
                raise ValueError(
                    f"unresolved {source} decorated call target bridge address "
                    f"{name!r}"
                )
            if len(addresses) != 1:
                raise ValueError(
                    f"ambiguous {source} decorated call target identity {name!r}"
                )
            address = next(iter(addresses))
            if source == "bn":
                if call_site_address is not None:
                    call_site = normalize_address(call_site_address)
                    selected = indexes.reviewed_authored_icf_by_call_site.get(call_site)
                    if selected is not None:
                        expected_physical_address, logical_identity = selected
                        if expected_physical_address != address:
                            raise ValueError(
                                f"authored ICF retail call selector {call_site} expected "
                                f"physical target {expected_physical_address}, found {address}"
                            )
                        return "direct", logical_identity
                group_identity = indexes.reviewed_icf_group_by_address.get(
                    address, ""
                )
                if group_identity:
                    return "direct", group_identity
            exact_bridge_names = {name}
            for item in matches:
                exact_bridge_names.update((
                    str(value)
                    for value in (
                        getattr(item, "name", ""),
                        getattr(item, "raw_name", ""),
                        getattr(item, "full_name", ""),
                    )
                    if value
                ))
            matching_aliases = [
                (alias_address, alias)
                for alias_address, aliases in (
                    indexes.reviewed_logical_aliases_by_address.items()
                )
                for alias in aliases
                if (
                    alias.original_name in exact_bridge_names
                    or alias.object_symbol in exact_bridge_names
                )
            ]
            aliases_at_address = [
                alias
                for alias_address, alias in matching_aliases
                if alias_address == address
            ]
            alias_identities = {
                alias.identity for alias in aliases_at_address
            }
            if len(alias_identities) > 1:
                raise ValueError(
                    f"ambiguous reviewed logical alias for {source} decorated "
                    f"call target {name!r} at {address}"
                )
            if len(alias_identities) == 1:
                identity = next(iter(alias_identities))
            elif matching_aliases:
                raise ValueError(
                    f"reviewed logical alias for {source} decorated call target "
                    f"{name!r} resolves at a different address than {address}"
                )
            elif indexes.reviewed_logical_aliases_by_address.get(address):
                raise ValueError(
                    f"missing exact reviewed logical alias for {source} decorated "
                    f"call target {name!r} at {address}"
                )
            else:
                identity = indexes.by_address.get(address, "")
            if not identity:
                raise ValueError(
                    f"unresolved {source} decorated call target tracker identity "
                    f"{name!r} at {address}"
                )
        if not identity:
            raise ValueError(f"unresolved {source} call target identity {name!r}")
        if source == "cod":
            group_identity = (
                indexes.reviewed_icf_group_by_logical_identity.get(identity, "")
            )
            if group_identity:
                if indexes.by_candidate_name.get(name) != identity:
                    raise ValueError(
                        f"candidate decorated call target {name!r} is not an "
                        "exact reviewed member of its winner-unknown ICF group"
                    )
                identity = group_identity
        return ("provider" if identity in indexes.provider_ids else "direct"), identity
    raise ValueError(f"unresolved {source} direct call operand {operand!r}")


def _exact_thiscall_constructor_result(
    operand: str,
    *,
    source: str,
    target_identity: str,
    indexes: IdentityIndexes,
    receiver_provenance: str,
) -> str:
    """Preserve exact VC5 constructor ``EAX == this`` lineage.

    This records only the ABI relationship needed at a control-flow join.  It
    does not turn unknown receiver state into object provenance: an empty or
    null receiver remains unresolved, and later indirect-call validation still
    applies its ordinary storage/vptr restrictions.
    """
    if not receiver_provenance or receiver_provenance == "null":
        return ""
    constructor_names = {
        name
        for name, identity in indexes.by_candidate_name.items()
        if identity == target_identity and _cc_catalog.MSVC_CONSTRUCTOR_RE.fullmatch(name)
    }
    if len(constructor_names) != 1:
        return ""
    constructor_name = next(iter(constructor_names))
    if source == "cod":
        decorated = _cc_catalog.DECORATED_RE.search(operand)
        if decorated is None or decorated.group(0) != constructor_name:
            return ""
    elif source != "bn":
        return ""
    return receiver_provenance


def _memory_slot(operand: str) -> tuple[str, int | None]:
    match = _cc_catalog.MEMORY_RE.search(operand)
    if match is None:
        expression = re.sub(
            r"^(?:byte|word|dword)\s+(?:ptr\s+)?",
            "",
            operand.strip().lower(),
        )
    else:
        expression = match.group(1).lower()
    expression = re.sub(r"\s+", "", expression)
    displacement: int | None = 0 if match is not None else None
    numbers = list(re.finditer(r"(?P<sign>[+-]?)(?P<number>0x[0-9a-f]+|\d+)", expression))
    if numbers:
        item = numbers[-1]
        value = _cc_cfg._parse_unsigned_assembly_integer(item.group("number"))
        displacement = -value if item.group("sign") == "-" else value
    return expression, displacement


def _exact_indirect_register_call_slot(
    instruction: Instruction,
    *,
    allow_zero: bool = False,
) -> tuple[str, int] | None:
    """Decode one exact ``CALL [r32+disp]`` with render/byte agreement."""

    if _cc_cfg._instruction_mnemonic(instruction) != "call":
        return None
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    if len(body) < 2 or body[0] != 0xFF:
        return None
    modrm = body[1]
    mode = modrm >> 6
    if ((modrm >> 3) & 7) != 2 or mode == 3 or (modrm & 7) == 4:
        return None
    if mode == 0 and len(body) == 2 and (modrm & 7) != 5:
        displacement = 0
    elif mode == 1 and len(body) == 3:
        displacement = struct.unpack("<b", body[2:3])[0]
    elif mode == 2 and len(body) == 6:
        displacement = struct.unpack("<i", body[2:6])[0]
    else:
        return None
    registers = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    base = registers[modrm & 7]
    expression, rendered_displacement = _memory_slot(
        _cc_cfg._instruction_operand(instruction)
    )
    rendered = re.fullmatch(
        r"(?P<base>eax|ecx|edx|ebx|esi|edi|ebp)"
        r"(?:\+(?:0x[0-9a-f]+|\d+))?",
        expression,
    )
    if (
        rendered is None
        or rendered.group("base") != base
        or rendered_displacement != displacement
        or displacement < 0
        or (displacement == 0 and not allow_zero)
    ):
        return None
    return base, displacement


def _exact_indexed_affine_load(
    instruction: Instruction,
) -> tuple[str, str, str, int, int, str] | None:
    """Decode one exact MOV/LEA r32,[base+index*scale+disp] SIB form."""

    mnemonic = _cc_cfg._instruction_mnemonic(instruction)
    expected_opcode = 0x8B if mnemonic == "mov" else 0x8D if mnemonic == "lea" else -1
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    if len(body) < 3 or body[0] != expected_opcode:
        return None
    modrm = body[1]
    mode = modrm >> 6
    if mode == 3 or (modrm & 7) != 4:
        return None
    sib = body[2]
    scale = 1 << (sib >> 6)
    index_code = (sib >> 3) & 7
    base_code = sib & 7
    if index_code == 4 or (mode == 0 and base_code == 5):
        return None
    if mode == 0 and len(body) == 3:
        displacement = 0
    elif mode == 1 and len(body) == 4:
        displacement = struct.unpack("<b", body[3:4])[0]
    elif mode == 2 and len(body) == 7:
        displacement = struct.unpack("<i", body[3:7])[0]
    else:
        return None
    registers = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    destination = registers[(modrm >> 3) & 7]
    base_register = registers[base_code]
    index_register = registers[index_code]
    operands = _cc_cfg._instruction_operand(instruction).split(",", 1)
    if len(operands) != 2 or operands[0].strip().lower() != destination:
        return None
    rendered = re.sub(
        r"\s+",
        "",
        _exact_memory_expression(operands[1]).lower(),
    )
    displacement_texts = (
        ("",)
        if displacement == 0
        else tuple(
            ("+" if displacement > 0 else "-") + value
            for value in (
                f"0x{abs(displacement):x}",
                str(abs(displacement)),
            )
        )
    )
    terms = {
        term
        for displacement_text in displacement_texts
        for term in (
            f"{base_register}+{index_register}*{scale}{displacement_text}",
            f"{index_register}*{scale}+{base_register}{displacement_text}",
        )
    }
    if scale == 1:
        terms.update(f"{left}+{right}{displacement_text}"
                     for left, right in ((base_register, index_register), (index_register, base_register))
                     for displacement_text in displacement_texts)
    if rendered not in terms:
        return None
    return (
        destination,
        base_register,
        index_register,
        scale,
        displacement,
        mnemonic,
    )


def _exact_memory_expression(operand: str) -> str:
    """Return a whitespace-free memory expression without case folding."""
    match = _cc_catalog.MEMORY_RE.search(operand)
    if match is None:
        expression = re.sub(
            r"^(?:byte|word|dword)\s+(?:ptr\s+)?",
            "",
            operand.strip(),
            flags=re.IGNORECASE,
        )
    else:
        expression = match.group(1)
    return re.sub(r"\s+", "", expression)


def _exact_ff15_absolute_address(
    instruction: Instruction,
) -> str | None:
    """Decode exactly one unprefixed ``FF 15 imm32`` instruction."""
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    if len(body) != 6 or body[:2] != b"\xff\x15":
        return None
    return normalize_address(struct.unpack("<I", body[2:6])[0])
