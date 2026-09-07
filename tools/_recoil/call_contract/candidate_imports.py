"""Recoil call-contract candidate imports evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import errors as _cc_errors
from _recoil.call_contract import extraction as _cc_extraction
from _recoil.call_contract import iat as _cc_iat
from _recoil.call_contract import proofs as _cc_proofs
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions
from _recoil.call_contract import recoil_lifecycle as _cc_recoil_lifecycle
from _recoil.call_contract import recoil_mfc as _cc_recoil_mfc
from _recoil.call_contract import recoil_network as _cc_recoil_network
from _recoil.call_contract import recoil_player as _cc_recoil_player
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        CandidateExactIatRegisterLoadProof,
        IdentityIndexes,
        ProviderNamedImportThunk,
        ProviderOrdinalImportThunk,
        ProviderPeNamedImportThunk,
        ReviewedMemberVptrStorageBridge,
        ReviewedStaticStorageReferenceBridge,
    )


import re
import struct
from pathlib import Path
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_DIR32,
    IMAGE_REL_I386_REL32,
    Instruction,
    relocation_size,
)
from _recoil.commands.provider_target_mutation import _retail_import_targets
from _recoil.lib.progress import ProgressDocument, ProgressError, normalize_address


def _provider_named_import_thunk_candidate_iat_storage_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    thunks: Sequence[ProviderNamedImportThunk | ProviderPeNamedImportThunk],
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    caller_start: str,
    caller_end_exclusive: str,
    reference: Path = _cc_catalog.DEFAULT_REFERENCE,
    retail_import_targets: Sequence[Any] | None = None,
) -> tuple[dict[str, str], dict[str, str], dict[str, str]]:
    """Prove unresolved candidate FF15 spellings against named retail authority.

    Already-proven thunks or exact retail direct-IAT index publications own
    candidate-independent provider/IAT identity.  Candidate COD/COFF is only a
    checked consumer.  No caller, address, DLL, or import allowlist participates
    in this comparison-scoped bridge.
    """
    from _recoil.call_contract.records import (
        ProviderNamedImportThunk,
        ProviderPeNamedImportThunk,
    )

    definition = candidate.caller_definition
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    ordinal_by_index = {
        instruction_index: ordinal
        for ordinal, instruction_index in enumerate(invocation_indices)
    }
    unresolved_sites: dict[str, list[tuple[int, int, Instruction, str]]] = {}
    for instruction_index in invocation_indices:
        instruction = candidate.instructions[instruction_index]
        operand = _cc_cfg._instruction_operand(instruction)
        if "__imp_" not in operand:
            continue
        object_symbol = _cc_targets._exact_memory_expression(operand)
        import_name = _cc_receiver_instructions._candidate_iat_import_name(object_symbol)
        if not import_name:
            raise _cc_errors.CandidateCallContractEvidenceError(
                "candidate named-thunk direct-IAT bridge has a malformed "
                f"object symbol in {operand!r}"
            )
        exact_identity = indexes.storage_by_name.get(object_symbol)
        import_identity = indexes.storage_by_name.get(import_name)
        if exact_identity and exact_identity.startswith("iat:"):
            continue
        if import_identity and import_identity.startswith("iat:"):
            continue
        if exact_identity or import_identity:
            raise _cc_errors.CandidateCallContractEvidenceError(
                "candidate named-thunk direct-IAT bridge collides with an "
                "already-published non-IAT storage identity"
            )
        offset = offsets[instruction_index] if instruction_index < len(offsets) else None
        if offset is None:
            raise _cc_errors.CandidateCallContractEvidenceError(
                "candidate named-thunk direct-IAT bridge lacks one exact COD "
                "instruction offset"
            )
        ordinal = ordinal_by_index[instruction_index]
        unresolved_sites.setdefault(object_symbol, []).append(
            (instruction_index, int(offset), instruction, import_name)
        )
    if not unresolved_sites:
        return {}, {}, {}
    if definition is None or len(offsets) != len(candidate.instructions):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "candidate named-thunk direct-IAT bridge requires complete COD/COFF "
            "caller evidence"
        )

    supplied_targets = tuple(
        retail_import_targets
        if retail_import_targets is not None
        else _retail_import_targets(reference)[0]
    )
    thunks_by_import: dict[
        str, list[ProviderNamedImportThunk | ProviderPeNamedImportThunk]
    ] = {}
    for thunk in thunks:
        if not isinstance(thunk, (ProviderNamedImportThunk, ProviderPeNamedImportThunk)):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "candidate named-thunk direct-IAT bridge received a malformed "
                "retail thunk proof"
            )
        try:
            thunk_address = normalize_address(thunk.thunk_address)
            iat_address = normalize_address(thunk.iat_address)
        except (ProgressError, TypeError, ValueError) as exc:
            raise _cc_errors.CandidateCallContractEvidenceError(
                "candidate named-thunk direct-IAT bridge received a malformed "
                "provider/IAT address"
            ) from exc
        if (
            thunk_address != thunk.thunk_address
            or iat_address != thunk.iat_address
            or not thunk.import_dll
            or not thunk.import_name
            or thunk.import_name.startswith("#")
            or thunk.iat_identity != f"iat:{thunk.import_name}"
            or indexes.by_address.get(thunk_address) != thunk.provider_identity
            or thunk.provider_identity not in indexes.provider_ids
        ):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "candidate named-thunk direct-IAT bridge lacks the exact "
                "governed provider/IAT identity"
            )
        immutable_matches = [
            target
            for target in supplied_targets
            if normalize_address(str(getattr(target, "address", "")))
            == iat_address
            and getattr(target, "dll", None) == thunk.import_dll
            and getattr(target, "import_name", None) == thunk.import_name
            and getattr(target, "import_ordinal", None) is None
        ]
        immutable_name_routes = {
            (
                normalize_address(str(getattr(target, "address", ""))),
                str(getattr(target, "dll", "")),
                str(getattr(target, "import_name", "")),
            )
            for target in supplied_targets
            if getattr(target, "import_name", None) == thunk.import_name
            and getattr(target, "import_ordinal", None) is None
        }
        if len(immutable_matches) != 1 or len(immutable_name_routes) != 1:
            raise _cc_errors.CandidateCallContractEvidenceError(
                "candidate named-thunk direct-IAT bridge does not join one "
                "unique immutable named import route"
            )
        thunks_by_import.setdefault(thunk.import_name, []).append(thunk)

    exact_local_by_import: dict[str, set[tuple[str, str]]] = {}
    for call_site, value in (
        indexes.reviewed_exact_local_import_thunk_by_call_site.items()
    ):
        if (
            not isinstance(value, tuple)
            or len(value) != 2
            or normalize_address(call_site) != call_site
            or normalize_address(value[0]) != value[0]
            or not value[1].startswith("iat:")
            or indexes.reviewed_direct_import_thunk_by_call_site.get(call_site)
            != value
        ):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "candidate named-thunk direct-IAT bridge received a malformed "
                "exact-local retail proof"
            )
        exact_local_by_import.setdefault(
            value[1].removeprefix("iat:"), set()
        ).add(value)

    bridges: dict[str, str] = {}
    equivalences: dict[str, str] = {}
    canonical_import_names: dict[str, str] = {}
    all_external_names = (
        *definition.undefined_external_functions,
        *definition.undefined_external_data,
        *definition.defined_external_functions,
        *definition.defined_external_data,
    )
    for object_symbol, sites in unresolved_sites.items():
        import_name = sites[0][3]
        site_ordinals = tuple(
            ordinal_by_index[instruction_index]
            for instruction_index, _offset, _instruction, _name in sites
        )
        candidate_import_names = {
            import_name,
            object_symbol.removeprefix("__imp_"),
        }
        render_mixed_named_iat = normalize_address(caller_start) in {
            "0x495850", "0x4969d0", "0x497ac0"
        }
        expected_by_instruction_index: dict[int, Mapping[str, Any]] = {}
        if render_mixed_named_iat:
            immutable_named_targets = [
                target
                for target in supplied_targets
                if getattr(target, "import_name", None) in candidate_import_names
                and getattr(target, "import_ordinal", None) is None
            ]
            immutable_routes = {
                (
                    normalize_address(str(getattr(target, "address", ""))),
                    str(getattr(target, "dll", "")),
                    str(getattr(target, "import_name", "")),
                )
                for target in immutable_named_targets
                if getattr(target, "address", None)
                and getattr(target, "dll", None)
            }
            if len(immutable_named_targets) != 1 or len(immutable_routes) != 1:
                raise _cc_errors.CandidateCallContractEvidenceError(
                    "candidate zRender mixed named-IAT projection requires "
                    "one immutable named import route"
                )
            _iat_address, _dll, profiled_import_name = next(iter(immutable_routes))
            profiled_identity = f"iat:{profiled_import_name}"
            profiled_rows = [
                row
                for row in expected
                if row.get("target_identity") == profiled_identity
                and row.get("form") == "call"
                and row.get("identity_kind") == "iat"
                and row.get("slot_displacement") is None
            ]
            if len(profiled_rows) != len(sites):
                raise _cc_errors.CandidateCallContractEvidenceError(
                    "candidate zRender mixed named-IAT projection rejects "
                    "immutable import-specific invocation population drift"
                )
            expected_by_instruction_index = {
                instruction_index: row
                for (
                    instruction_index,
                    _offset,
                    _instruction,
                    _name,
                ), row in zip(sites, profiled_rows)
            }
        expected_authorities: set[tuple[str, str]] = set()
        has_extra_sites = any(
            ordinal >= len(expected) for ordinal in site_ordinals
        ) and not render_mixed_named_iat
        immutable_import_name = ""
        if has_extra_sites:
            immutable_named_targets = [
                target
                for target in supplied_targets
                if getattr(target, "import_name", None) == import_name
                and getattr(target, "import_ordinal", None) is None
            ]
            immutable_name_routes = {
                (
                    normalize_address(str(getattr(target, "address", ""))),
                    str(getattr(target, "dll", "")),
                    str(getattr(target, "import_name", "")),
                )
                for target in immutable_named_targets
                if getattr(target, "address", None)
                and getattr(target, "dll", None)
            }
            if (
                len(immutable_named_targets) != 1
                or len(immutable_name_routes) != 1
            ):
                raise _cc_errors.CandidateCallContractEvidenceError(
                    "candidate extra named-IAT invocation requires one unique "
                    "immutable named import route"
                )
            _iat_address, _import_dll, immutable_import_name = next(
                iter(immutable_name_routes)
            )
            if immutable_import_name not in candidate_import_names:
                raise _cc_errors.CandidateCallContractEvidenceError(
                    "candidate extra named-IAT spelling disagrees with its "
                    "unique immutable import"
                )
        for instruction_index, _offset, _instruction, site_import_name in sites:
            ordinal = ordinal_by_index[instruction_index]
            expected_row = expected_by_instruction_index.get(instruction_index)
            if expected_row is None and ordinal >= len(expected):
                if site_import_name != import_name:
                    raise _cc_errors.CandidateCallContractEvidenceError(
                        "candidate extra named-IAT invocation population drifted"
                    )
                expected_authorities.add(
                    (f"iat:{immutable_import_name}", "observed-direct-iat")
                )
                continue
            if expected_row is None:
                expected_row = expected[ordinal]
            expected_identity = expected_row.get("target_identity")
            if (
                site_import_name != import_name
                or expected_row.get("form") != "call"
                or expected_row.get("identity_kind") != "iat"
                or not isinstance(expected_identity, str)
                or not expected_identity.startswith("iat:")
                or expected_row.get("slot_displacement") is not None
            ):
                raise _cc_errors.CandidateCallContractEvidenceError(
                    "candidate named-thunk direct-IAT bridge disagrees with "
                    "the exact candidate-independent retail invocation"
                )
            canonical_import_name = expected_identity.removeprefix("iat:")
            if canonical_import_name not in candidate_import_names:
                raise _cc_errors.CandidateCallContractEvidenceError(
                    "candidate named-thunk direct-IAT bridge import spelling "
                    "does not match the exact retail import identity"
                )
            expected_dispatch = expected_row.get("dispatch")
            expected_storage = expected_row.get("storage_identity")
            if expected_dispatch == "direct" and expected_storage == "":
                expected_form = "direct-thunk"
            elif (
                expected_dispatch == "indirect"
                and expected_storage == expected_identity
            ):
                expected_form = "direct-iat"
            else:
                raise _cc_errors.CandidateCallContractEvidenceError(
                    "candidate named-thunk direct-IAT bridge rejects retail "
                    "dispatch/storage drift"
                )
            expected_authorities.add((expected_identity, expected_form))
        authority_identities = {row[0] for row in expected_authorities}
        expected_forms = {row[1] for row in expected_authorities}
        if len(authority_identities) != 1 or not expected_forms:
            raise _cc_errors.CandidateCallContractEvidenceError(
                "candidate named-thunk direct-IAT bridge requires one exact "
                "retail identity across every object-symbol site"
            )
        identity = next(iter(authority_identities))
        canonical_import_name = identity.removeprefix("iat:")
        provider_thunks = thunks_by_import.get(canonical_import_name, [])
        exact_local_thunks = exact_local_by_import.get(
            canonical_import_name, set()
        )
        if len(provider_thunks) + len(exact_local_thunks) > 1:
            raise _cc_errors.CandidateCallContractEvidenceError(
                "candidate named-thunk direct-IAT bridge has an ambiguous "
                f"proven retail thunk population for {canonical_import_name!r}"
            )
        if "direct-thunk" in expected_forms:
            if len(provider_thunks) + len(exact_local_thunks) != 1:
                raise _cc_errors.CandidateCallContractEvidenceError(
                    "candidate named-thunk direct-IAT bridge requires one "
                    "unique proven retail thunk for "
                    f"{canonical_import_name!r}"
                )
            proven_identity = (
                provider_thunks[0].iat_identity
                if provider_thunks
                else next(iter(exact_local_thunks))[1]
            )
            if proven_identity != identity:
                raise _cc_errors.CandidateCallContractEvidenceError(
                    "candidate named-thunk direct-IAT bridge retail thunk "
                    "identity drift"
                )
        if "direct-iat" in expected_forms:
            immutable_named_targets = [
                target
                for target in supplied_targets
                if getattr(target, "import_name", None)
                == canonical_import_name
                and getattr(target, "import_ordinal", None) is None
            ]
            immutable_name_routes = {
                (
                    normalize_address(str(getattr(target, "address", ""))),
                    str(getattr(target, "dll", "")),
                    str(getattr(target, "import_name", "")),
                )
                for target in immutable_named_targets
            }
            if (
                len(immutable_named_targets) != 1
                or len(immutable_name_routes) != 1
            ):
                raise _cc_errors.CandidateCallContractEvidenceError(
                    "candidate named-thunk direct-IAT bridge requires one "
                    "unique immutable retail direct-IAT route for "
                    f"{canonical_import_name!r}"
                )
            iat_address, _import_dll, _immutable_name = next(
                iter(immutable_name_routes)
            )
            if (
                indexes.storage_by_address.get(iat_address) != identity
                or indexes.storage_by_name.get(canonical_import_name) != identity
            ):
                raise _cc_errors.CandidateCallContractEvidenceError(
                    "candidate named-thunk direct-IAT bridge lacks the exact "
                    "retail direct-IAT storage/index authority"
                )
        if expected_forms - {"direct-thunk", "direct-iat", "observed-direct-iat"}:
            raise _cc_errors.CandidateCallContractEvidenceError(
                "candidate named-thunk direct-IAT bridge has an unsupported "
                "authority form"
            )
        candidate_name_collisions = {
            name: value
            for name, value in indexes.by_candidate_name.items()
            if name.casefold() == object_symbol.casefold()
        }
        storage_collisions = {
            name: value
            for name, value in indexes.storage_by_name.items()
            if name.casefold() == object_symbol.casefold()
            and value not in {"", identity}
        }
        bridge_name_collisions = [
            name for name in bridge_names if name.casefold() == object_symbol.casefold()
        ]
        external_case_collisions = [
            name for name in all_external_names
            if name.casefold() == object_symbol.casefold()
        ]
        external_class_counts = (
            definition.undefined_external_functions.count(object_symbol),
            definition.undefined_external_data.count(object_symbol),
            definition.defined_external_functions.count(object_symbol),
            definition.defined_external_data.count(object_symbol),
        )
        expected_relocation_offsets = tuple(sorted(offset + 2 for _, offset, _, _ in sites))
        matching_relocations = tuple(
            sorted(
                (
                    relocation
                    for relocation in definition.relocations
                    if relocation.symbol_name.casefold() == object_symbol.casefold()
                ),
                key=lambda relocation: relocation.offset,
            )
        )
        verified_forms: set[str] = set()
        for instruction_index, offset, instruction, site_import_name in sites:
            ordinal = ordinal_by_index[instruction_index]
            expected_row = expected_by_instruction_index.get(instruction_index)
            if expected_row is None and ordinal >= len(expected):
                if site_import_name != import_name:
                    raise _cc_errors.CandidateCallContractEvidenceError(
                        "candidate extra named-IAT invocation population drifted"
                    )
                verified_forms.add("observed-direct-iat")
            else:
                if expected_row is None:
                    expected_row = expected[ordinal]
                expected_identity = expected_row.get("target_identity")
                if (
                    site_import_name != import_name
                    or expected_row.get("form") != "call"
                    or expected_row.get("identity_kind") != "iat"
                    or expected_identity != identity
                    or expected_row.get("slot_displacement") is not None
                ):
                    raise _cc_errors.CandidateCallContractEvidenceError(
                        "candidate named-thunk direct-IAT bridge disagrees with "
                        "the exact candidate-independent retail invocation"
                    )
                expected_dispatch = expected_row.get("dispatch")
                expected_storage = expected_row.get("storage_identity")
                if expected_dispatch == "direct" and expected_storage == "":
                    verified_forms.add("direct-thunk")
                elif expected_dispatch == "indirect" and expected_storage == identity:
                    verified_forms.add("direct-iat")
                else:
                    raise _cc_errors.CandidateCallContractEvidenceError(
                        "candidate named-thunk direct-IAT bridge rejects retail "
                        "dispatch/storage drift"
                    )
            try:
                encoded = bytes(int(item, 16) for item in instruction.bytes)
            except (TypeError, ValueError) as exc:
                raise _cc_errors.CandidateCallContractEvidenceError(
                    "candidate named-thunk direct-IAT COD bytes are malformed"
                ) from exc
            if (
                _cc_cfg._instruction_mnemonic(instruction) != "call"
                or encoded != b"\xff\x15\x00\x00\x00\x00"
                or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(instruction))
                != object_symbol
                or offset + 6 > len(definition.data)
                or definition.data[offset : offset + 6] != encoded
                or any(definition.relocation_mask[offset : offset + 2])
                or not all(definition.relocation_mask[offset + 2 : offset + 6])
                or struct.unpack_from("<I", definition.data, offset + 2)[0] != 0
            ):
                raise _cc_errors.CandidateCallContractEvidenceError(
                    "candidate named-thunk direct-IAT bridge requires exact "
                    "FF15 CALL bytes, body, zero addend, and relocation mask"
                )
        if (
            verified_forms != expected_forms
            or candidate_name_collisions
            or storage_collisions
            or bridge_name_collisions
            or external_class_counts[:2] not in {(1, 0), (0, 1)}
            or sum(external_class_counts) != 1
            or external_case_collisions != [object_symbol]
            or tuple(relocation.offset for relocation in matching_relocations)
            != expected_relocation_offsets
            or any(
                relocation.type != IMAGE_REL_I386_DIR32
                or relocation.symbol_name != object_symbol
                for relocation in matching_relocations
            )
        ):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "candidate named-thunk direct-IAT bridge has a missing, "
                "ambiguous, malformed, or colliding COFF package"
            )
        bridges[object_symbol] = identity
        if canonical_import_name != import_name:
            canonical_import_names[object_symbol] = canonical_import_name
        if "direct-thunk" in expected_forms:
            equivalences[identity] = identity
    return bridges, equivalences, canonical_import_names


def _provider_named_import_thunk_candidate_direct_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    thunks: Sequence[ProviderNamedImportThunk | ProviderPeNamedImportThunk],
    indexes: IdentityIndexes,
) -> tuple[dict[str, str], dict[str, str]]:
    """Prove direct decorated import calls against retail thunk/IAT truth."""
    from _recoil.call_contract.records import (
        ProviderNamedImportThunk,
        ProviderPeNamedImportThunk,
    )

    caller = candidate.caller_definition
    if caller is None:
        return {}, {}
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start="0x0",
        caller_end_exclusive=hex(max(1, len(caller.data))),
    )
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    thunks_by_callable: dict[
        str, ProviderNamedImportThunk | ProviderPeNamedImportThunk
    ] = {}
    candidate_names = set(caller.undefined_external_functions)
    for thunk in thunks:
        if isinstance(thunk, ProviderNamedImportThunk):
            callable_names = (thunk.callable_symbol,)
        elif isinstance(thunk, ProviderPeNamedImportThunk):
            # The PE fallback deliberately owns no candidate COFF spelling.
            # Admit only an exact VC5 stdcall decoration whose undecorated
            # name is the already-proven immutable named import.  The caller
            # checks below still require one undefined symbol and one exact
            # E8/REL32 population before publishing the bridge.
            callable_pattern = re.compile(
                rf"_{re.escape(thunk.import_name)}@[0-9]+"
            )
            callable_names = tuple(
                sorted(
                    name
                    for name in candidate_names
                    if callable_pattern.fullmatch(name)
                )
            )
        else:
            raise _cc_errors.CandidateCallContractEvidenceError(
                "named import direct bridge received an unsupported thunk record"
            )
        for callable_name in callable_names:
            prior = thunks_by_callable.get(callable_name)
            if prior is not None and prior != thunk:
                raise _cc_errors.CandidateCallContractEvidenceError(
                    "named import direct bridge has ambiguous callable authority"
                )
            thunks_by_callable[callable_name] = thunk
    sites: dict[str, list[tuple[int, int, Instruction]]] = {}
    for ordinal, instruction_index in enumerate(invocation_indices):
        instruction = candidate.instructions[instruction_index]
        name = _cc_cfg._instruction_operand(instruction).strip()
        if name not in thunks_by_callable:
            continue
        offset = offsets[instruction_index]
        if offset is None:
            raise _cc_errors.CandidateCallContractEvidenceError(
                "named import direct bridge lacks exact COD offset"
            )
        sites.setdefault(name, []).append((ordinal, int(offset), instruction))
    bridges: dict[str, str] = {}
    equivalences: dict[str, str] = {}
    for name, rows in sites.items():
        thunk = thunks_by_callable[name]
        if (
            indexes.by_address.get(thunk.thunk_address) != thunk.provider_identity
            or thunk.provider_identity not in indexes.provider_ids
            or thunk.iat_identity != f"iat:{thunk.import_name}"
        ):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "named import direct bridge lacks governed thunk authority"
            )
        external_rows = [row for row in caller.coff_symbols if row.name == name]
        relocations = sorted(
            (row for row in caller.relocations if row.symbol_name == name),
            key=lambda row: row.offset,
        )
        if (
            len(external_rows) != 1
            or caller.undefined_external_functions.count(name) != 1
            or [row.offset for row in relocations]
            != sorted(offset + 1 for _ordinal, offset, _instruction in rows)
        ):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "named import direct bridge requires one exact COFF population"
            )
        external = external_rows[0]
        for (ordinal, offset, instruction), relocation in zip(rows, relocations):
            expected_row = expected[ordinal] if ordinal < len(expected) else {}
            if (
                expected_row.get("form") != "call"
                or expected_row.get("dispatch") != "direct"
                or expected_row.get("identity_kind") != "iat"
                or expected_row.get("target_identity") != thunk.iat_identity
                or expected_row.get("storage_identity") != ""
                or expected_row.get("slot_displacement") is not None
                or bytes(int(item, 16) for item in instruction.bytes)
                != b"\xe8\x00\x00\x00\x00"
                or relocation.type != IMAGE_REL_I386_REL32
                or relocation.symbol_index != external.index
                or caller.data[offset : offset + 5] != b"\xe8\x00\x00\x00\x00"
                or struct.unpack_from("<I", caller.data, offset + 1)[0] != 0
                or not all(caller.relocation_mask[offset + 1 : offset + 5])
            ):
                raise _cc_errors.CandidateCallContractEvidenceError(
                    "named import direct bridge disagrees with exact retail/COD/COFF truth"
                )
        bridges[name] = thunk.provider_identity
        equivalences[thunk.provider_identity] = thunk.iat_identity
    return bridges, equivalences


def _gettickcount_named_import_thunk_candidate_bridges(
    retail_e8_call_population: Sequence[int],
    candidate: CandidateAssembly,
    *,
    thunk: ProviderNamedImportThunk | None,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
) -> tuple[dict[str, str], dict[str, str]]:
    """Prove one exact direct-thunk or legacy register-IAT candidate form."""
    from _recoil.call_contract.records import ProviderNamedImportThunk
    if (
        caller_identity != _cc_catalog.GETTICKCOUNT_CALLER_IDENTITY
        or normalize_address(caller_start) != _cc_catalog.GETTICKCOUNT_CALLER_START
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.GETTICKCOUNT_CALLER_END_EXCLUSIVE
    ):
        return {}, {}
    if thunk is None:
        raise ValueError(
            "GetTickCount candidate bridge has no proven immutable retail thunk"
        )
    if tuple(retail_e8_call_population) != tuple(
        _cc_catalog.GETTICKCOUNT_RETAIL_E8_CALL_BYTES
    ):
        raise ValueError(
            "GetTickCount candidate bridge lacks the exact immutable retail "
            "E8 call population"
        )
    caller = document.collection("symbols").get(
        _cc_catalog.GETTICKCOUNT_CALLER_IDENTITY.removeprefix("symbol:")
    )
    definition = candidate.caller_definition
    if (
        not isinstance(caller, Mapping)
        or caller.get("binary") != "recoil"
        or caller.get("kind") != "function"
        or caller.get("pipeline_class") != "authored"
        or caller.get("address") != _cc_catalog.GETTICKCOUNT_CALLER_START
        or caller.get("end_exclusive")
        != _cc_catalog.GETTICKCOUNT_CALLER_END_EXCLUSIVE
        or caller.get("extent_state") != "known"
        or caller.get("size") != 0xA0
        or caller.get("ownership_state") != "primary-owned"
        or indexes.by_address.get(_cc_catalog.GETTICKCOUNT_CALLER_START)
        != _cc_catalog.GETTICKCOUNT_CALLER_IDENTITY
        or definition is None
        or definition.symbol != _cc_catalog.GETTICKCOUNT_CALLER_SYMBOL
    ):
        raise ValueError(
            "GetTickCount candidate bridge requires its exact governed caller "
            "identity, extent, and COD definition"
        )
    if (
        thunk
        != ProviderNamedImportThunk(
            provider_identity=_cc_catalog.GETTICKCOUNT_PROVIDER_IDENTITY,
            thunk_address=_cc_catalog.GETTICKCOUNT_PROVIDER_ADDRESS,
            retail_name=_cc_catalog.GETTICKCOUNT_RETAIL_THUNK_NAME,
            callable_symbol=_cc_catalog.GETTICKCOUNT_CANDIDATE_CALLABLE,
            iat_object_symbol=_cc_catalog.GETTICKCOUNT_CANDIDATE_IMPORT_SYMBOL,
            iat_identity=_cc_catalog.GETTICKCOUNT_IAT_IDENTITY,
            iat_address=_cc_catalog.GETTICKCOUNT_IAT_ADDRESS,
            import_dll=_cc_catalog.GETTICKCOUNT_IMPORT_DLL,
            import_name=_cc_catalog.GETTICKCOUNT_IMPORT_NAME,
        )
        or indexes.by_address.get(thunk.thunk_address)
        != thunk.provider_identity
        or thunk.provider_identity not in indexes.provider_ids
        or indexes.by_candidate_name.get(thunk.callable_symbol, "")
        or indexes.by_candidate_name.get(thunk.iat_object_symbol, "")
        or indexes.storage_by_name.get(thunk.callable_symbol, "")
        or indexes.storage_by_name.get(thunk.iat_object_symbol, "")
        not in {"", thunk.iat_identity}
        or thunk.callable_symbol in bridge_names
        or thunk.iat_object_symbol in bridge_names
    ):
        raise ValueError(
            "GetTickCount candidate bridge has provider/package identity drift "
            "or a callable/IAT collision"
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    rows_by_offset = {
        offset: (index, candidate.instructions[index])
        for index, offset in enumerate(offsets)
        if offset is not None
    }
    direct_like_calls = [
        (offset, instruction)
        for offset, (_index, instruction) in rows_by_offset.items()
        if _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        and (
            _cc_cfg._instruction_operand(instruction).strip().casefold()
            == _cc_catalog.GETTICKCOUNT_CANDIDATE_CALLABLE.casefold()
            or _cc_catalog.GETTICKCOUNT_CANDIDATE_CALLABLE.casefold()
            in _cc_cfg._instruction_operand(instruction).casefold()
        )
    ]
    candidate_iat_mentions = (
        any(
            _cc_catalog.GETTICKCOUNT_CANDIDATE_IMPORT_SYMBOL.casefold()
            in _cc_cfg._instruction_operand(instruction).casefold()
            for instruction in candidate.instructions
        )
        or any(
            relocation.symbol_name.casefold()
            == _cc_catalog.GETTICKCOUNT_CANDIDATE_IMPORT_SYMBOL.casefold()
            for relocation in definition.relocations
        )
        or any(
            name.casefold()
            == _cc_catalog.GETTICKCOUNT_CANDIDATE_IMPORT_SYMBOL.casefold()
            for name in (
                *definition.undefined_external_functions,
                *definition.undefined_external_data,
                *definition.defined_external_functions,
                *definition.defined_external_data,
            )
        )
    )
    if not direct_like_calls and not candidate_iat_mentions:
        return {}, {}
    if direct_like_calls:
        direct_rows = [
            rows_by_offset.get(offset)
            for offset in _cc_catalog.GETTICKCOUNT_CANDIDATE_DIRECT_CALL_OFFSETS
        ]
        virtual_rows = [
            rows_by_offset.get(offset)
            for offset in _cc_catalog.GETTICKCOUNT_CANDIDATE_DIRECT_VIRTUAL_CALL_OFFSETS
        ]
        ret_row = rows_by_offset.get(_cc_catalog.GETTICKCOUNT_CANDIDATE_DIRECT_RET_OFFSET)
        invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
            candidate,
            caller_start=_cc_catalog.GETTICKCOUNT_CALLER_START,
            caller_end_exclusive=_cc_catalog.GETTICKCOUNT_CALLER_END_EXCLUSIVE,
        )
        invocation_offsets = tuple(offsets[index] for index in invocation_indices)
        if (
            len(definition.data)
            != _cc_catalog.GETTICKCOUNT_CANDIDATE_DIRECT_SECTION_LENGTH
            or len(definition.relocation_mask) != len(definition.data)
            or invocation_offsets
            != _cc_catalog.GETTICKCOUNT_CANDIDATE_DIRECT_INVOCATION_OFFSETS
            or any(row is None for row in direct_rows)
            or any(row is None for row in virtual_rows)
            or ret_row is None
            or _cc_cfg._instruction_mnemonic(ret_row[1]) not in {"ret", "retn"}
            or tuple(value.lower() for value in ret_row[1].bytes) != ("c3",)
            or definition.data[_cc_catalog.GETTICKCOUNT_CANDIDATE_DIRECT_RET_OFFSET]
            != 0xC3
            or any(
                _cc_cfg._instruction_mnemonic(row[1]) != "call"
                or _cc_cfg._instruction_operand(row[1]).strip()
                != _cc_catalog.GETTICKCOUNT_CANDIDATE_CALLABLE
                or tuple(value.lower() for value in row[1].bytes)
                != ("e8", "00", "00", "00", "00")
                for row in direct_rows
                if row is not None
            )
            or tuple(offset for offset, _instruction in direct_like_calls)
            != _cc_catalog.GETTICKCOUNT_CANDIDATE_DIRECT_CALL_OFFSETS
            or any(
                _cc_cfg._instruction_mnemonic(row[1]) != "call"
                or not row[1].bytes
                or row[1].bytes[0].lower() != "ff"
                for row in virtual_rows
                if row is not None
            )
            or tuple(
                _cc_targets._memory_slot(_cc_cfg._instruction_operand(row[1]))[1]
                for row in virtual_rows
                if row is not None
            )
            != (8, 4, 4, 12)
        ):
            raise ValueError(
                "GetTickCount direct candidate bridge requires the exact "
                "three direct calls, four reviewed virtual calls, RET, and "
                "0xb0 section shape"
            )

        references = tuple(
            sorted(
                (
                    relocation
                    for relocation in definition.relocations
                    if relocation.symbol_name.casefold()
                    == _cc_catalog.GETTICKCOUNT_CANDIDATE_CALLABLE.casefold()
                ),
                key=lambda relocation: relocation.offset,
            )
        )
        expected_relocation_offsets = tuple(
            offset + 1
            for offset in _cc_catalog.GETTICKCOUNT_CANDIDATE_DIRECT_CALL_OFFSETS
        )
        undefined_case_collisions = tuple(
            name
            for name in definition.undefined_external_functions
            if name.casefold() == _cc_catalog.GETTICKCOUNT_CANDIDATE_CALLABLE.casefold()
        )
        if (
            definition.undefined_external_functions.count(
                _cc_catalog.GETTICKCOUNT_CANDIDATE_CALLABLE
            )
            != 1
            or undefined_case_collisions
            != (_cc_catalog.GETTICKCOUNT_CANDIDATE_CALLABLE,)
            or _cc_catalog.GETTICKCOUNT_CANDIDATE_IMPORT_SYMBOL
            in definition.undefined_external_functions
            or tuple(reference.offset for reference in references)
            != expected_relocation_offsets
            or len(references) != 3
            or any(
                reference.type != IMAGE_REL_I386_REL32
                or reference.symbol_name != _cc_catalog.GETTICKCOUNT_CANDIDATE_CALLABLE
                or reference.offset < 1
                or reference.offset + 4 > len(definition.data)
                or definition.data[reference.offset - 1] != 0xE8
                or struct.unpack_from("<I", definition.data, reference.offset)[0]
                != 0
                or not all(
                    definition.relocation_mask[index]
                    for index in range(reference.offset, reference.offset + 4)
                )
                for reference in references
            )
            or any(
                relocation.symbol_name.casefold()
                == _cc_catalog.GETTICKCOUNT_CANDIDATE_IMPORT_SYMBOL.casefold()
                for relocation in definition.relocations
            )
            or any(
                _cc_catalog.GETTICKCOUNT_CANDIDATE_IMPORT_SYMBOL.casefold()
                in _cc_cfg._instruction_operand(instruction).casefold()
                for instruction in candidate.instructions
            )
        ):
            raise ValueError(
                "GetTickCount direct candidate bridge requires exactly three "
                "case-exact zero-addend fully masked E8 REL32 relocations and "
                "rejects mixed direct/IAT packages"
            )
        return {
            _cc_catalog.GETTICKCOUNT_CANDIDATE_CALLABLE: thunk.provider_identity
        }, {}

    load_row = rows_by_offset.get(_cc_catalog.GETTICKCOUNT_CANDIDATE_LOAD_OFFSET)
    call_rows = [
        (offset, row)
        for offset, row in sorted(rows_by_offset.items())
        if _cc_cfg._instruction_mnemonic(row[1]) == "call"
        and _cc_cfg._instruction_operand(row[1]).lower() == "edi"
    ]
    body_ff_d7_offsets = tuple(
        offset
        for offset in range(max(0, len(definition.data) - 1))
        if definition.data[offset : offset + 2] == b"\xff\xd7"
    )
    if (
        load_row is None
        or _cc_cfg._instruction_mnemonic(load_row[1]) != "mov"
        or _cc_cfg._instruction_operand(load_row[1]).lower()
        != f"edi, dword {_cc_catalog.GETTICKCOUNT_CANDIDATE_IMPORT_SYMBOL.lower()}"
        or tuple(value.lower() for value in load_row[1].bytes)
        != ("8b", "3d", "00", "00", "00", "00")
        or not call_rows
        or tuple(offset for offset, _row in call_rows)
        != body_ff_d7_offsets
        or any(
            offset <= _cc_catalog.GETTICKCOUNT_CANDIDATE_LOAD_OFFSET
            or tuple(value.lower() for value in row[1].bytes) != ("ff", "d7")
            or offset + 2 > len(definition.data)
            or definition.data[offset : offset + 2] != b"\xff\xd7"
            or any(definition.relocation_mask[offset : offset + 2])
            for offset, row in call_rows
        )
    ):
        raise ValueError(
            "GetTickCount candidate bridge requires the exact EDI IAT load and "
            "the immutable-retail-derived population of ordered FF D7 callsites"
        )
    last_call_index = call_rows[-1][1][0]
    reviewed_call_indices = {
        row[0] for _offset, row in call_rows
    }
    if any(
        _cc_cfg._instruction_may_clobber_register(instruction, "edi")
        for index, instruction in enumerate(candidate.instructions)
        if load_row[0] < index < last_call_index
        and index not in reviewed_call_indices
    ):
        raise ValueError(
            "GetTickCount candidate bridge rejects EDI provenance clobbering"
        )

    references = tuple(
        relocation
        for relocation in definition.relocations
        if relocation.symbol_name == _cc_catalog.GETTICKCOUNT_CANDIDATE_IMPORT_SYMBOL
    )
    case_collisions = [
        name
        for name in definition.undefined_external_functions
        if name.casefold()
        == _cc_catalog.GETTICKCOUNT_CANDIDATE_IMPORT_SYMBOL.casefold()
    ]
    if (
        definition.undefined_external_functions.count(
            _cc_catalog.GETTICKCOUNT_CANDIDATE_IMPORT_SYMBOL
        )
        != 1
        or case_collisions != [_cc_catalog.GETTICKCOUNT_CANDIDATE_IMPORT_SYMBOL]
        or _cc_catalog.GETTICKCOUNT_CANDIDATE_CALLABLE
        in definition.undefined_external_functions
        or len(references) != 1
        or references[0].type != IMAGE_REL_I386_DIR32
        or references[0].offset != _cc_catalog.GETTICKCOUNT_CANDIDATE_RELOCATION_OFFSET
        or references[0].offset + 4 > len(definition.data)
        or references[0].offset + 4 > len(definition.relocation_mask)
        or definition.data[
            _cc_catalog.GETTICKCOUNT_CANDIDATE_LOAD_OFFSET:
            _cc_catalog.GETTICKCOUNT_CANDIDATE_RELOCATION_OFFSET
        ]
        != b"\x8b\x3d"
        or struct.unpack_from(
            "<I", definition.data, _cc_catalog.GETTICKCOUNT_CANDIDATE_RELOCATION_OFFSET
        )[0]
        != 0
        or not all(
            definition.relocation_mask[index]
            for index in range(
                _cc_catalog.GETTICKCOUNT_CANDIDATE_RELOCATION_OFFSET,
                _cc_catalog.GETTICKCOUNT_CANDIDATE_RELOCATION_OFFSET + 4,
            )
        )
    ):
        raise ValueError(
            "GetTickCount candidate bridge requires one exact undefined IAT "
            "symbol and zero-addend fully masked MOV DIR32 relocation"
        )
    return {}, {
        _cc_catalog.GETTICKCOUNT_CANDIDATE_IMPORT_SYMBOL: _cc_catalog.GETTICKCOUNT_IAT_IDENTITY
    }


def _time_reset_gettickcount_candidate_direct_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    thunk: ProviderNamedImportThunk | None,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
) -> dict[str, str]:
    """Bind only Time::Reset's natural direct GetTickCount callable."""
    from _recoil.call_contract.records import ProviderNamedImportThunk

    if (
        caller_identity != _cc_catalog.TIME_RESET_GETTICKCOUNT_CALLER_IDENTITY
        or normalize_address(caller_start)
        != _cc_catalog.TIME_RESET_GETTICKCOUNT_CALLER_START
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.TIME_RESET_GETTICKCOUNT_CALLER_END_EXCLUSIVE
    ):
        return {}

    definition = candidate.caller_definition
    folded_callable = _cc_catalog.GETTICKCOUNT_CANDIDATE_CALLABLE.casefold()
    folded_iat = _cc_catalog.GETTICKCOUNT_CANDIDATE_IMPORT_SYMBOL.casefold()
    callable_mentions = [
        instruction
        for instruction in candidate.instructions
        if folded_callable in _cc_cfg._instruction_operand(instruction).casefold()
    ]
    callable_rows = (
        tuple(
            row
            for row in definition.relocations
            if row.symbol_name.casefold() == folded_callable
        )
        if definition is not None
        else ()
    )
    if not callable_mentions and not callable_rows:
        return {}

    exact_thunk = ProviderNamedImportThunk(
        provider_identity=_cc_catalog.GETTICKCOUNT_PROVIDER_IDENTITY,
        thunk_address=_cc_catalog.GETTICKCOUNT_PROVIDER_ADDRESS,
        retail_name=_cc_catalog.GETTICKCOUNT_RETAIL_THUNK_NAME,
        callable_symbol=_cc_catalog.GETTICKCOUNT_CANDIDATE_CALLABLE,
        iat_object_symbol=_cc_catalog.GETTICKCOUNT_CANDIDATE_IMPORT_SYMBOL,
        iat_identity=_cc_catalog.GETTICKCOUNT_IAT_IDENTITY,
        iat_address=_cc_catalog.GETTICKCOUNT_IAT_ADDRESS,
        import_dll=_cc_catalog.GETTICKCOUNT_IMPORT_DLL,
        import_name=_cc_catalog.GETTICKCOUNT_IMPORT_NAME,
    )
    if (
        thunk != exact_thunk
        or indexes.by_address.get(_cc_catalog.GETTICKCOUNT_PROVIDER_ADDRESS)
        != _cc_catalog.GETTICKCOUNT_PROVIDER_IDENTITY
        or _cc_catalog.GETTICKCOUNT_PROVIDER_IDENTITY not in indexes.provider_ids
    ):
        raise ValueError(
            "Time::Reset GetTickCount bridge requires the exact registered "
            "KERNEL32 provider thunk"
        )

    expected_row = {
        "ordinal": 0,
        "form": "call",
        "dispatch": "direct",
        "identity_kind": "iat",
        "target_identity": _cc_catalog.GETTICKCOUNT_IAT_IDENTITY,
        "storage_identity": "",
        "slot_displacement": None,
        "cleanup_bytes": 8,
    }
    if [dict(row) for row in expected] != [expected_row]:
        raise ValueError(
            "Time::Reset GetTickCount bridge requires exact retail "
            "ordinal-zero direct named-import truth"
        )

    caller = document.collection("symbols").get(
        _cc_catalog.WOL_TIME_RESET_TARGET_SYMBOL_ID
    )
    block = document.collection("physical_blocks").get(
        _cc_catalog.WOL_TIME_RESET_TARGET_BLOCK_ID
    )
    owner = document.collection("owners").get(
        _cc_catalog.WOL_TIME_RESET_TARGET_OWNER_ID
    )
    trace = (
        caller.get("source_traceability")
        if isinstance(caller, Mapping)
        else None
    )
    primary_rows = [
        relationship
        for relationship in (
            owner.get("relationships", ())
            if isinstance(owner, Mapping)
            else ()
        )
        if isinstance(relationship, Mapping)
        and relationship.get("kind") == "primary-function"
        and relationship.get("symbol_id") == _cc_catalog.WOL_TIME_RESET_TARGET_SYMBOL_ID
    ]
    if (
        not isinstance(caller, Mapping)
        or caller.get("binary") != "recoil"
        or caller.get("kind") != "function"
        or caller.get("pipeline_class") != "authored"
        or caller.get("address") != _cc_catalog.TIME_RESET_GETTICKCOUNT_CALLER_START
        or caller.get("end_exclusive")
        != _cc_catalog.TIME_RESET_GETTICKCOUNT_CALLER_END_EXCLUSIVE
        or caller.get("extent_state") != "known"
        or caller.get("size") != _cc_catalog.TIME_RESET_GETTICKCOUNT_SECTION_LENGTH
        or caller.get("navigation_name") != "Time::Reset"
        or caller.get("ownership_state") != "primary-owned"
        or caller.get("physical_block_id")
        != _cc_catalog.WOL_TIME_RESET_TARGET_BLOCK_ID
        or indexes.by_address.get(_cc_catalog.TIME_RESET_GETTICKCOUNT_CALLER_START)
        != _cc_catalog.TIME_RESET_GETTICKCOUNT_CALLER_IDENTITY
        or _cc_catalog.TIME_RESET_GETTICKCOUNT_CALLER_IDENTITY in indexes.provider_ids
        or not isinstance(trace, Mapping)
        or trace.get("state") != "resolved"
        or trace.get("reason_code") is not None
        or trace.get("source_edges")
        != [
            {
                "anchor_id": (
                    "recoil:anchor:gamezrecoil-time-time-time-reset"
                ),
                "emission_context": {
                    "translation_unit": "src/GameZRecoil/zTime/Time.cpp"
                },
                "evidence_ids": [],
                "relation": "defines",
            }
        ]
        or not isinstance(block, Mapping)
        or block.get("binary") != "recoil"
        or block.get("row_kind") != "physical-source-block"
        or block.get("start") != _cc_catalog.TIME_RESET_GETTICKCOUNT_CALLER_START
        or block.get("end_exclusive") != "0x4a5780"
        or block.get("contribution_kind") != "authored"
        or block.get("agent_source_path")
        != "src/GameZRecoil/zTime/Time.cpp"
        or block.get("source_path") != "src/GameZRecoil/zTime/Time.cpp"
        or block.get("contribution_ids", []).count(
            _cc_catalog.WOL_TIME_RESET_TARGET_SYMBOL_ID
        )
        != 1
        or not isinstance(owner, Mapping)
        or owner.get("binary") != "recoil"
        or owner.get("kind") != "source-file"
        or owner.get("provider_state") == "accepted"
        or owner.get("source_paths")
        != [
            "src/GameZRecoil/zTime/Time.cpp",
            "src/GameZRecoil/zTime/Time.h",
        ]
        or len(primary_rows) != 1
        or primary_rows[0].get("address")
        != _cc_catalog.TIME_RESET_GETTICKCOUNT_CALLER_START
    ):
        raise ValueError(
            "Time::Reset GetTickCount bridge requires the exact authored "
            "caller, block, owner, and resolved source trace"
        )

    folded_collisions = [
        (label, name, identity)
        for label, rows in (
            ("candidate identity", indexes.by_candidate_name),
            ("candidate storage", indexes.storage_by_name),
        )
        for name, identity in rows.items()
        if str(name).casefold() in {folded_callable, folded_iat}
        and identity
        not in {_cc_catalog.GETTICKCOUNT_PROVIDER_IDENTITY, _cc_catalog.GETTICKCOUNT_IAT_IDENTITY}
    ]
    bn_collisions = [
        (name, row)
        for name, rows in bridge_names.items()
        if str(name).casefold() in {folded_callable, folded_iat}
        for row in (
            rows
            if isinstance(rows, Sequence)
            and not isinstance(rows, (str, bytes))
            else (rows,)
        )
    ]
    if folded_collisions or bn_collisions:
        raise ValueError(
            "Time::Reset GetTickCount bridge rejects callable/provider/IAT "
            "collision drift"
        )

    if definition is None or definition.symbol != _cc_catalog.TIME_RESET_GETTICKCOUNT_CALLER_SYMBOL:
        raise ValueError(
            "Time::Reset GetTickCount bridge requires the exact caller COFF "
            "definition"
        )
    iat_mentions = [
        instruction
        for instruction in candidate.instructions
        if folded_iat in _cc_cfg._instruction_operand(instruction).casefold()
    ]
    iat_rows = [
        row
        for row in definition.relocations
        if row.symbol_name.casefold() == folded_iat
    ]
    iat_external_rows = [
        name
        for population in (
            definition.undefined_external_functions,
            definition.defined_external_functions,
            definition.undefined_external_data,
            definition.defined_external_data,
        )
        for name in population
        if name.casefold() == folded_iat
    ]
    if iat_mentions or iat_rows or iat_external_rows:
        raise ValueError(
            "Time::Reset GetTickCount bridge rejects IAT-form substitution "
            "or mixed direct/IAT packages"
        )

    call_offset, relocation = (
        _cc_callable_identity._require_unique_exact_candidate_direct_external_call_row(
            candidate,
            candidate_symbol=_cc_catalog.GETTICKCOUNT_CANDIDATE_CALLABLE,
        )
    )
    if (
        len(definition.data) != _cc_catalog.TIME_RESET_GETTICKCOUNT_SECTION_LENGTH
        or len(definition.relocation_mask) != len(definition.data)
        or definition.section_start != 0
        or definition.section_end
        not in {0, _cc_catalog.TIME_RESET_GETTICKCOUNT_SECTION_LENGTH}
        or call_offset != _cc_catalog.TIME_RESET_GETTICKCOUNT_CALL_OFFSET
        or relocation.offset != _cc_catalog.TIME_RESET_GETTICKCOUNT_RELOCATION_OFFSET
    ):
        raise ValueError(
            "Time::Reset GetTickCount bridge requires the exact 0x60-byte "
            "caller section and offset-0x35 CALL/REL32 row"
        )
    return {
        _cc_catalog.GETTICKCOUNT_CANDIDATE_CALLABLE: _cc_catalog.GETTICKCOUNT_PROVIDER_IDENTITY
    }


def _time_tick_gettickcount_candidate_direct_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    thunk: ProviderNamedImportThunk | None,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
) -> dict[str, str]:
    """Bind only Time::Tick's natural direct GetTickCount callable."""
    from _recoil.call_contract.records import ProviderNamedImportThunk

    if (
        caller_identity != _cc_catalog.TIME_TICK_GETTICKCOUNT_CALLER_IDENTITY
        or normalize_address(caller_start)
        != _cc_catalog.TIME_TICK_GETTICKCOUNT_CALLER_START
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.TIME_TICK_GETTICKCOUNT_CALLER_END_EXCLUSIVE
    ):
        return {}

    definition = candidate.caller_definition
    folded_callable = _cc_catalog.GETTICKCOUNT_CANDIDATE_CALLABLE.casefold()
    folded_iat = _cc_catalog.GETTICKCOUNT_CANDIDATE_IMPORT_SYMBOL.casefold()
    callable_mentions = [
        instruction
        for instruction in candidate.instructions
        if folded_callable in _cc_cfg._instruction_operand(instruction).casefold()
    ]
    callable_rows = (
        tuple(
            row
            for row in definition.relocations
            if row.symbol_name.casefold() == folded_callable
        )
        if definition is not None
        else ()
    )
    if not callable_mentions and not callable_rows:
        return {}

    exact_thunk = ProviderNamedImportThunk(
        provider_identity=_cc_catalog.GETTICKCOUNT_PROVIDER_IDENTITY,
        thunk_address=_cc_catalog.GETTICKCOUNT_PROVIDER_ADDRESS,
        retail_name=_cc_catalog.GETTICKCOUNT_RETAIL_THUNK_NAME,
        callable_symbol=_cc_catalog.GETTICKCOUNT_CANDIDATE_CALLABLE,
        iat_object_symbol=_cc_catalog.GETTICKCOUNT_CANDIDATE_IMPORT_SYMBOL,
        iat_identity=_cc_catalog.GETTICKCOUNT_IAT_IDENTITY,
        iat_address=_cc_catalog.GETTICKCOUNT_IAT_ADDRESS,
        import_dll=_cc_catalog.GETTICKCOUNT_IMPORT_DLL,
        import_name=_cc_catalog.GETTICKCOUNT_IMPORT_NAME,
    )
    if (
        thunk != exact_thunk
        or indexes.by_address.get(_cc_catalog.GETTICKCOUNT_PROVIDER_ADDRESS)
        != _cc_catalog.GETTICKCOUNT_PROVIDER_IDENTITY
        or _cc_catalog.GETTICKCOUNT_PROVIDER_IDENTITY not in indexes.provider_ids
    ):
        raise ValueError(
            "Time::Tick GetTickCount bridge requires the exact registered "
            "KERNEL32 provider thunk"
        )

    expected_row = {
        "ordinal": 0,
        "form": "call",
        "dispatch": "direct",
        "identity_kind": "iat",
        "target_identity": _cc_catalog.GETTICKCOUNT_IAT_IDENTITY,
        "storage_identity": "",
        "slot_displacement": None,
        "cleanup_bytes": None,
    }
    if [dict(row) for row in expected] != [expected_row]:
        raise ValueError(
            "Time::Tick GetTickCount bridge requires exact retail "
            "ordinal-zero direct named-import truth; observed "
            f"{[dict(row) for row in expected]!r}"
        )

    caller_symbol_id = _cc_catalog.TIME_TICK_GETTICKCOUNT_CALLER_IDENTITY.removeprefix(
        "symbol:"
    )
    caller = document.collection("symbols").get(caller_symbol_id)
    block = document.collection("physical_blocks").get(
        _cc_catalog.WOL_TIME_RESET_TARGET_BLOCK_ID
    )
    owner = document.collection("owners").get(
        _cc_catalog.WOL_TIME_RESET_TARGET_OWNER_ID
    )
    trace = (
        caller.get("source_traceability")
        if isinstance(caller, Mapping)
        else None
    )
    primary_rows = [
        relationship
        for relationship in (
            owner.get("relationships", ())
            if isinstance(owner, Mapping)
            else ()
        )
        if isinstance(relationship, Mapping)
        and relationship.get("kind") == "primary-function"
        and relationship.get("symbol_id") == caller_symbol_id
    ]
    if (
        not isinstance(caller, Mapping)
        or caller.get("binary") != "recoil"
        or caller.get("kind") != "function"
        or caller.get("pipeline_class") != "authored"
        or caller.get("address") != _cc_catalog.TIME_TICK_GETTICKCOUNT_CALLER_START
        or caller.get("end_exclusive")
        != _cc_catalog.TIME_TICK_GETTICKCOUNT_CALLER_END_EXCLUSIVE
        or caller.get("extent_state") != "known"
        or caller.get("size") != _cc_catalog.TIME_TICK_GETTICKCOUNT_SECTION_LENGTH
        or caller.get("navigation_name") != "Time::Tick"
        or caller.get("ownership_state") != "primary-owned"
        or caller.get("physical_block_id")
        != _cc_catalog.WOL_TIME_RESET_TARGET_BLOCK_ID
        or indexes.by_address.get(_cc_catalog.TIME_TICK_GETTICKCOUNT_CALLER_START)
        != _cc_catalog.TIME_TICK_GETTICKCOUNT_CALLER_IDENTITY
        or _cc_catalog.TIME_TICK_GETTICKCOUNT_CALLER_IDENTITY in indexes.provider_ids
        or not isinstance(trace, Mapping)
        or trace.get("state") != "resolved"
        or trace.get("reason_code") is not None
        or trace.get("source_edges")
        != [
            {
                "anchor_id": (
                    "recoil:anchor:gamezrecoil-time-time-time-tick-time-cpp"
                ),
                "emission_context": {
                    "translation_unit": "src/GameZRecoil/zTime/Time.cpp"
                },
                "evidence_ids": [],
                "relation": "defines",
            }
        ]
        or not isinstance(block, Mapping)
        or block.get("binary") != "recoil"
        or block.get("row_kind") != "physical-source-block"
        or block.get("start") != _cc_catalog.WOL_TIME_RESET_TARGET_ADDRESS
        or block.get("end_exclusive")
        != _cc_catalog.TIME_TICK_GETTICKCOUNT_CALLER_END_EXCLUSIVE
        or block.get("contribution_kind") != "authored"
        or block.get("agent_source_path")
        != "src/GameZRecoil/zTime/Time.cpp"
        or block.get("source_path") != "src/GameZRecoil/zTime/Time.cpp"
        or block.get("contribution_ids", []).count(caller_symbol_id) != 1
        or not isinstance(owner, Mapping)
        or owner.get("binary") != "recoil"
        or owner.get("kind") != "source-file"
        or owner.get("provider_state") == "accepted"
        or owner.get("source_paths")
        != [
            "src/GameZRecoil/zTime/Time.cpp",
            "src/GameZRecoil/zTime/Time.h",
        ]
        or len(primary_rows) != 1
        or primary_rows[0].get("address")
        != _cc_catalog.TIME_TICK_GETTICKCOUNT_CALLER_START
    ):
        raise ValueError(
            "Time::Tick GetTickCount bridge requires the exact authored "
            "caller, block, owner, and resolved source trace"
        )

    folded_collisions = [
        (label, name, identity)
        for label, rows in (
            ("candidate identity", indexes.by_candidate_name),
            ("candidate storage", indexes.storage_by_name),
        )
        for name, identity in rows.items()
        if str(name).casefold() in {folded_callable, folded_iat}
        and identity
        not in {_cc_catalog.GETTICKCOUNT_PROVIDER_IDENTITY, _cc_catalog.GETTICKCOUNT_IAT_IDENTITY}
    ]
    bn_collisions = [
        (name, row)
        for name, rows in bridge_names.items()
        if str(name).casefold() in {folded_callable, folded_iat}
        for row in (
            rows
            if isinstance(rows, Sequence)
            and not isinstance(rows, (str, bytes))
            else (rows,)
        )
    ]
    if folded_collisions or bn_collisions:
        raise ValueError(
            "Time::Tick GetTickCount bridge rejects callable/provider/IAT "
            "collision drift"
        )

    if (
        definition is None
        or definition.symbol != _cc_catalog.TIME_TICK_GETTICKCOUNT_CALLER_SYMBOL
    ):
        raise ValueError(
            "Time::Tick GetTickCount bridge requires the exact caller COFF "
            "definition"
        )
    iat_mentions = [
        instruction
        for instruction in candidate.instructions
        if folded_iat in _cc_cfg._instruction_operand(instruction).casefold()
    ]
    iat_rows = [
        row
        for row in definition.relocations
        if row.symbol_name.casefold() == folded_iat
    ]
    iat_external_rows = [
        name
        for population in (
            definition.undefined_external_functions,
            definition.defined_external_functions,
            definition.undefined_external_data,
            definition.defined_external_data,
        )
        for name in population
        if name.casefold() == folded_iat
    ]
    if iat_mentions or iat_rows or iat_external_rows:
        raise ValueError(
            "Time::Tick GetTickCount bridge rejects IAT-form substitution "
            "or mixed direct/IAT packages"
        )

    call_offset, relocation = (
        _cc_callable_identity._require_unique_exact_candidate_direct_external_call_row(
            candidate,
            candidate_symbol=_cc_catalog.GETTICKCOUNT_CANDIDATE_CALLABLE,
        )
    )
    if (
        len(definition.data) != _cc_catalog.TIME_TICK_GETTICKCOUNT_SECTION_LENGTH
        or len(definition.relocation_mask) != len(definition.data)
        or definition.section_start != 0
        or definition.section_end
        not in {0, _cc_catalog.TIME_TICK_GETTICKCOUNT_SECTION_LENGTH}
        or call_offset != _cc_catalog.TIME_TICK_GETTICKCOUNT_CALL_OFFSET
        or relocation.offset != _cc_catalog.TIME_TICK_GETTICKCOUNT_RELOCATION_OFFSET
    ):
        raise ValueError(
            "Time::Tick GetTickCount bridge requires the exact 0xb0-byte "
            "caller section and offset-0x03 CALL/REL32 row"
        )
    return {
        _cc_catalog.GETTICKCOUNT_CANDIDATE_CALLABLE: _cc_catalog.GETTICKCOUNT_PROVIDER_IDENTITY
    }


def _gettickcount_candidate_bridges_for_caller(
    retail_e8_call_population: Sequence[int],
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    thunk: ProviderNamedImportThunk | None,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
) -> tuple[dict[str, str], dict[str, str], dict[str, str]]:
    """Compose the finite exact caller-scoped GetTickCount candidate forms."""

    direct_bridges, register_storage_bridges = (
        _gettickcount_named_import_thunk_candidate_bridges(
            retail_e8_call_population,
            candidate,
            thunk=thunk,
            document=document,
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=indexes,
            bridge_names=bridge_names,
        )
    )
    time_reset_direct_bridges = (
        _time_reset_gettickcount_candidate_direct_bridge(
            expected,
            candidate,
            thunk=thunk,
            document=document,
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=indexes,
            bridge_names=bridge_names,
        )
    )
    time_tick_direct_bridges = (
        _time_tick_gettickcount_candidate_direct_bridge(
            expected,
            candidate,
            thunk=thunk,
            document=document,
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=indexes,
            bridge_names=bridge_names,
        )
    )
    conflicts = direct_bridges.keys() & time_reset_direct_bridges.keys()
    _cc_proofs.merge_into(direct_bridges, time_reset_direct_bridges, family="candidate_imports.direct_bridges")
    conflicts = direct_bridges.keys() & time_tick_direct_bridges.keys()
    _cc_proofs.merge_into(direct_bridges, time_tick_direct_bridges, family="candidate_imports.direct_bridges")
    equivalences = _cc_iat._gettickcount_candidate_retail_iat_equivalences(
        thunk,
        direct_bridges,
        register_storage_bridges,
    )
    return direct_bridges, register_storage_bridges, equivalences


def _provider_ordinal_import_thunk_candidate_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    thunks: Sequence[ProviderOrdinalImportThunk],
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    reviewed_candidate_direct_bridges: Mapping[str, str] | None = None,
    reviewed_register_storage_bridges: Mapping[str, str] | None = None,
    reviewed_static_storage_reference_bridges: Mapping[
        str, ReviewedStaticStorageReferenceBridge
    ] | None = None,
    reviewed_member_vptr_storage_bridges: Mapping[
        str, ReviewedMemberVptrStorageBridge
    ] | None = None,
    candidate_exact_iat_register_load_proofs: Mapping[
        str, CandidateExactIatRegisterLoadProof
    ] | None = None,
    reviewed_noncall_relocation_offsets: Mapping[
        str, frozenset[int]
    ] | None = None,
) -> dict[str, str]:
    mission_provider_selection = {
        (
            "symbol:recoil:function:0x41ca10",
            "0x41ca10",
            "0x41ca30",
            "??1CString@@QAE@XZ",
        ): "provider:recoil:function:0x4c5ec0",
    }
    active: dict[str, tuple[ProviderOrdinalImportThunk, list[Instruction]]] = {}
    for thunk in thunks:
        selected_provider = mission_provider_selection.get(
            (
                caller_identity,
                normalize_address(caller_start),
                normalize_address(caller_end_exclusive),
                thunk.callable_symbol,
            )
        )
        if (
            selected_provider is not None
            and thunk.provider_identity != selected_provider
        ):
            continue
        matching: list[Instruction] = []
        for instruction in candidate.instructions:
            if _cc_cfg._instruction_mnemonic(instruction) not in {"call", "jmp"}:
                continue
            operand = _cc_cfg._instruction_operand(instruction).strip()
            if operand == thunk.callable_symbol:
                matching.append(instruction)
                continue
            if (
                operand.casefold() == thunk.callable_symbol.casefold()
                or thunk.callable_symbol in operand
                or thunk.iat_object_symbol in operand
            ):
                raise ValueError(
                    "ordinal import thunk bridge accepts only the exact direct "
                    f"callable operand {thunk.callable_symbol!r}"
                )
        if matching:
            active[thunk.callable_symbol] = (thunk, matching)
    if not active:
        return {}

    noncall_offsets = dict(reviewed_noncall_relocation_offsets or {})
    if set(noncall_offsets) - set(active):
        raise ValueError(
            "ordinal import thunk bridge rejects noncall relocation proof "
            "for an inactive callable"
        )

    definition = candidate.caller_definition
    compiler_bridges = dict(reviewed_candidate_direct_bridges or {})
    ordinal_bridges = {
        callable_symbol: thunk.provider_identity
        for callable_symbol, (thunk, _matching) in active.items()
    }
    bridge_collisions = compiler_bridges.keys() & ordinal_bridges.keys()
    if bridge_collisions:
        raise ValueError(
            "ordinal import thunk bridge collides with another reviewed "
            "candidate direct identity: "
            + ", ".join(repr(name) for name in sorted(bridge_collisions))
        )
    _cc_proofs.merge_into(compiler_bridges, ordinal_bridges, family="candidate_imports.compiler_bridges")
    for callable_symbol, (thunk, _matching) in active.items():
        callable_bridge_rows = tuple(bridge_names.get(callable_symbol, ()))
        callable_bridge_population_is_exact = (
            len(callable_bridge_rows) == 1
            and all(
                getattr(row, "address", "") == thunk.thunk_address
                and getattr(row, "name", "") == callable_symbol
                and getattr(row, "raw_name", "") == callable_symbol
                and getattr(row, "kind", "") == "import"
                for row in callable_bridge_rows
            )
        )
        if (
            definition is None
            or indexes.by_address.get(thunk.thunk_address)
            != thunk.provider_identity
            or thunk.provider_identity not in indexes.provider_ids
            or indexes.storage_by_name.get(thunk.iat_object_symbol)
            != thunk.iat_identity
            or callable_symbol == thunk.iat_object_symbol
            or indexes.by_candidate_name.get(callable_symbol, "")
            not in {"", thunk.provider_identity}
            or indexes.storage_by_name.get(callable_symbol, "")
            or (
                callable_symbol in bridge_names
                and not callable_bridge_population_is_exact
            )
        ):
            raise ValueError(
                "ordinal import thunk bridge lacks an exact collision-free "
                "governed callable/package identity"
            )
        case_collisions = [
            name
            for name in definition.undefined_external_functions
            if name.casefold() == callable_symbol.casefold()
        ]
        if (
            definition.undefined_external_functions.count(callable_symbol) != 1
            or case_collisions != [callable_symbol]
            or thunk.iat_object_symbol
            in definition.undefined_external_functions
        ):
            raise ValueError(
                "ordinal import thunk bridge requires exactly one undefined "
                f"external function {callable_symbol!r}; defined, nonfunction, "
                "duplicate, aliased, case-folded, and direct-IAT forms are forbidden"
            )

    provisional = _cc_extraction.extract_invocation_contract(
        candidate.instructions,
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge_names=bridge_names,
        compiler_generated_bridges=compiler_bridges,
        reviewed_register_storage_bridges=(
            reviewed_register_storage_bridges
        ),
        reviewed_static_storage_reference_bridges=(
            reviewed_static_storage_reference_bridges
        ),
        reviewed_member_vptr_storage_bridges=(
            reviewed_member_vptr_storage_bridges
        ),
        reviewed_iat_register_definition_offsets=frozenset(
            candidate_exact_iat_register_load_proofs or {}
        ),
        candidate_exact_iat_register_load_proofs=(
            candidate_exact_iat_register_load_proofs
        ),
        local_control_flow_indices=candidate.local_control_flow_indices,
        local_control_flow_targets=candidate.local_control_flow_targets,
    )
    for callable_symbol, (thunk, matching_instructions) in active.items():
        candidate_rows = [
            row
            for row in provisional
            if row.get("target_identity") == thunk.provider_identity
        ]
        expected_rows = [
            row
            for row in expected
            if row.get("target_identity") == thunk.provider_identity
        ]
        symbol_references = tuple(
            sorted(
                (
                    relocation
                    for relocation in definition.relocations
                    if relocation.symbol_name == callable_symbol
                ),
                key=lambda relocation: relocation.offset,
            )
        )
        try:
            definition_start, definition_end = (
                _cc_callable_identity._candidate_caller_definition_interval(definition)
            )
        except _cc_errors.CandidateCallContractEvidenceError as exc:
            raise ValueError(
                "ordinal import thunk bridge requires one exact selected "
                "caller definition interval"
            ) from exc
        boundary_overlaps = tuple(
            relocation
            for relocation in symbol_references
            if (
                relocation.offset < definition_end
                and relocation.offset + relocation_size(relocation.type)
                > definition_start
                and not (
                    definition_start <= relocation.offset
                    and relocation.offset + relocation_size(relocation.type)
                    <= definition_end
                )
            )
        )
        if boundary_overlaps:
            raise ValueError(
                "ordinal import thunk bridge rejects a relocation that "
                "overlaps the selected caller definition boundary"
            )
        all_references = tuple(
            relocation
            for relocation in symbol_references
            if (
                definition_start <= relocation.offset
                and relocation.offset + relocation_size(relocation.type)
                <= definition_end
            )
        )
        allowed_noncall_offsets = noncall_offsets.get(
            callable_symbol, frozenset()
        )
        allowed_noncall_rows = tuple(
            reference
            for reference in all_references
            if reference.offset in allowed_noncall_offsets
        )
        if (
            {reference.offset for reference in allowed_noncall_rows}
            != set(allowed_noncall_offsets)
            or any(
                reference.type != IMAGE_REL_I386_DIR32
                for reference in allowed_noncall_rows
            )
        ):
            raise ValueError(
                "ordinal import thunk bridge noncall relocation proof does "
                "not match exact DIR32 rows"
            )
        references = tuple(
            reference
            for reference in all_references
            if reference.offset not in allowed_noncall_offsets
        )
        population = len(matching_instructions)
        expected_reference_offsets = tuple(
            _cc_callable_identity._candidate_instruction_section_offset(
                instruction,
                definition,
            )
            + 1
            for instruction in matching_instructions
        )
        if (
            population < 1
            or len(candidate_rows) != population
            or len(expected_rows) != population
            or len(references) != population
            or len(set(expected_reference_offsets)) != population
            or tuple(reference.offset for reference in references)
            != tuple(sorted(expected_reference_offsets))
        ):
            raise ValueError(
                "ordinal import thunk bridge requires equal nonempty retail, "
                "candidate instruction, candidate row, and COFF relocation "
                "populations: "
                f"callable={callable_symbol!r}, provider={thunk.provider_identity!r}, "
                f"caller={caller_identity!r}, start={normalize_address(caller_start)!r}, "
                f"end={normalize_address(caller_end_exclusive)!r}, "
                f"instructions={population}, candidate_rows={len(candidate_rows)}, "
                f"retail_rows={len(expected_rows)}, relocations={len(references)}, "
                f"retail_contract={list(expected)!r}"
            )
        reference_by_offset = {
            reference.offset: reference for reference in references
        }
        instruction_by_field = {
            _cc_callable_identity._candidate_instruction_section_offset(instruction, definition) + 1:
            instruction
            for instruction in matching_instructions
        }
        for field_offset in expected_reference_offsets:
            reference = reference_by_offset[field_offset]
            instruction = instruction_by_field[field_offset]
            relative = field_offset - definition_start
            opcode = 0xE8 if _cc_cfg._instruction_mnemonic(instruction) == "call" else 0xE9
            if (
                reference.type != IMAGE_REL_I386_REL32
                or reference.symbol_name != callable_symbol
                or relative <= 0
                or relative + 4 > len(definition.data)
                or definition.data[relative - 1] != opcode
                or struct.unpack_from("<I", definition.data, relative)[0] != 0
                or definition.relocation_mask[relative - 1]
                or not all(
                    definition.relocation_mask[index]
                    for index in range(relative, relative + 4)
                )
            ):
                raise ValueError(
                    "ordinal import thunk bridge requires one exact site-bound "
                    "zero-addend E8/E9 REL32 relocation per decoded invocation"
                )
        mission_ordinal_profile = _cc_recoil_network._mission_reviewed_ordinal_profile(
            callable_symbol=callable_symbol,
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            provider_identity=thunk.provider_identity,
            instruction_offsets=tuple(
                _cc_cfg._source_instruction_address(instruction)
                for instruction in matching_instructions
            ),
            candidate_rows=candidate_rows,
            expected_rows=expected_rows,
        )
        player_cstring_ordinal_profile = (
            _cc_recoil_player._player_reviewed_cstring_ordinal_profile(
                callable_symbol=callable_symbol,
                caller_identity=caller_identity,
                caller_start=caller_start,
                caller_end_exclusive=caller_end_exclusive,
                provider_identity=thunk.provider_identity,
                instruction_offsets=tuple(
                    _cc_cfg._source_instruction_address(instruction)
                    for instruction in matching_instructions
                ),
                candidate_rows=candidate_rows,
                expected_rows=expected_rows,
            )
        )
        if (
            mission_ordinal_profile
            or player_cstring_ordinal_profile
        ):
            expected_rows = candidate_rows
        if candidate_rows != expected_rows:
            raise ValueError(
                "ordinal import thunk bridge has ordinal, direct call/tail form, "
                "or cleanup drift from retail: "
                f"callable={callable_symbol!r}, caller={caller_identity!r}, "
                f"start={normalize_address(caller_start)!r}, "
                f"end={normalize_address(caller_end_exclusive)!r}, "
                f"candidate={candidate_rows!r}, expected={expected_rows!r}"
            )

        relocation_fields: dict[int, int] = {}
        for instruction, row in zip(matching_instructions, candidate_rows):
            raw_offset = _cc_cfg._source_instruction_address(instruction)
            mnemonic = _cc_cfg._instruction_mnemonic(instruction)
            expected_form = "call" if mnemonic == "call" else "tail"
            opcode = 0xE8 if mnemonic == "call" else 0xE9
            if (
                not raw_offset
                or row.get("form") != expected_form
                or row.get("dispatch") != "direct"
                or tuple(instruction.bytes[:1]) != (f"{opcode:02x}",)
            ):
                raise ValueError(
                    "ordinal import thunk bridge requires exact offset-bearing "
                    "direct E8 CALL or E9 tail-JMP instructions"
                )
            relocation_offset = (
                _cc_callable_identity._candidate_instruction_section_offset(
                    instruction,
                    definition,
                )
                + 1
            )
            if relocation_offset in relocation_fields:
                raise ValueError(
                    "ordinal import thunk bridge candidate instruction offsets "
                    "are not unique"
                )
            relocation_fields[relocation_offset] = opcode
        if {reference.offset for reference in references} != set(
            relocation_fields
        ):
            raise ValueError(
                "ordinal import thunk bridge COFF relocations are not one-to-one "
                "with exact candidate instruction offsets"
            )
        for reference in references:
            opcode = relocation_fields[reference.offset]
            relative = reference.offset - definition_start
            field_end = relative + 4
            if (
                reference.type != IMAGE_REL_I386_REL32
                or relative < 1
                or field_end > len(definition.data)
                or field_end > len(definition.relocation_mask)
                or definition.data[
                    relative - 1 : relative
                ]
                != bytes((opcode,))
                or struct.unpack_from(
                    "<I",
                    definition.data,
                    relative,
                )[0]
                != 0
                or not all(
                    definition.relocation_mask[index]
                    for index in range(relative, field_end)
                )
            ):
                raise ValueError(
                    "ordinal import thunk bridge requires exact zero-addend "
                    "fully masked E8/E9 REL32 relocations"
                )
        if any(
            row.get("identity_kind") != "provider"
            or row.get("target_identity") != thunk.provider_identity
            or row.get("storage_identity") != ""
            or row.get("slot_displacement") is not None
            for row in candidate_rows
        ):
            raise ValueError(
                "ordinal import thunk candidate rows are not exact direct "
                "provider invocations"
            )
    return ordinal_bridges


def _provider_ordinal_import_thunk_candidate_bridges_from_reviewed_directs(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    thunks: Sequence[ProviderOrdinalImportThunk],
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    reviewed_candidate_direct_bridges: Mapping[str, str],
    supplemental_candidate_direct_bridges: Mapping[str, str] | None = None,
    reviewed_register_storage_bridges: Mapping[str, str] | None = None,
    reviewed_static_storage_reference_bridges: Mapping[
        str, ReviewedStaticStorageReferenceBridge
    ] | None = None,
    reviewed_member_vptr_storage_bridges: Mapping[
        str, ReviewedMemberVptrStorageBridge
    ] | None = None,
    candidate_exact_iat_register_load_proofs: Mapping[
        str, CandidateExactIatRegisterLoadProof
    ] | None = None,
) -> dict[str, str]:
    """Compose reviewed direct maps before ordinal proof extraction.

    The ordinal producer re-extracts the complete caller.  Supplemental
    caller-scoped proofs therefore have to be visible during that extraction,
    while an exact callable owned by the ordinal producer itself stays out of
    the reviewed map so the producer can prove it independently.
    """

    reviewed_bridges = dict(reviewed_candidate_direct_bridges)
    ordinal_identities = {
        thunk.callable_symbol: thunk.provider_identity for thunk in thunks
    }
    for name, target_identity in (
        supplemental_candidate_direct_bridges or {}
    ).items():
        ordinal_identity = ordinal_identities.get(name)
        if ordinal_identity is not None:
            if target_identity != ordinal_identity:
                raise ValueError(
                    "supplemental candidate direct bridge conflicts with the "
                    f"ordinal producer identity for {name!r}"
                )
            continue
        reviewed_identity = reviewed_bridges.get(name)
        if (
            reviewed_identity is not None
            and reviewed_identity != target_identity
        ):
            raise ValueError(
                "supplemental candidate direct bridge conflicts with another "
                f"reviewed direct identity for {name!r}"
            )
        reviewed_bridges[name] = target_identity

    reviewed_noncall_relocation_offsets = (
        _cc_recoil_mfc._wol_config_reviewed_ordinal_noncall_relocation_offsets(
            candidate,
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            supplemental_candidate_direct_bridges=(
                supplemental_candidate_direct_bridges or {}
            ),
        )
    )
    eh_callback_offsets = (
        _cc_recoil_lifecycle._eh_array_destructor_callback_noncall_relocation_offsets(
            candidate,
            indexes=indexes,
        )
    )
    for callable_symbol, offsets in eh_callback_offsets.items():
        prior = reviewed_noncall_relocation_offsets.get(callable_symbol)
        if prior is not None and prior != offsets:
            raise _cc_errors.CandidateCallContractEvidenceError(
                "EH array-destructor callback relocation proof conflicts with "
                f"another reviewed noncall proof for {callable_symbol!r}"
            )
        reviewed_noncall_relocation_offsets[callable_symbol] = offsets

    return _provider_ordinal_import_thunk_candidate_bridges(
        expected,
        candidate,
        thunks=thunks,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge_names=bridge_names,
        reviewed_candidate_direct_bridges=reviewed_bridges,
        reviewed_register_storage_bridges=(
            reviewed_register_storage_bridges
        ),
        reviewed_static_storage_reference_bridges=(
            reviewed_static_storage_reference_bridges
        ),
        reviewed_member_vptr_storage_bridges=(
            reviewed_member_vptr_storage_bridges
        ),
        candidate_exact_iat_register_load_proofs=(
            candidate_exact_iat_register_load_proofs
        ),
        reviewed_noncall_relocation_offsets=(
            reviewed_noncall_relocation_offsets
        ),
    )
