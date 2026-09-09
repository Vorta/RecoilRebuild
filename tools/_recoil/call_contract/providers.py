"""Recoil call-contract providers evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import errors as _cc_errors
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import listing as _cc_listing
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        CandidateTuLocalFunctionDefinition,
        IdentityIndexes,
    )

import re
import struct
from pathlib import Path, PurePosixPath, PureWindowsPath
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_DIR32,
    IMAGE_REL_I386_REL32,
    IMAGE_SCN_CNT_CODE,
    IMAGE_SYM_CLASS_EXTERNAL,
    IMAGE_SYM_CLASS_STATIC,
    Instruction,
)
from _recoil.commands.vc5_verify import DEFAULT_VC5_ENV, compiler_env_path
from _recoil.lib.binja import BinaryNinjaBridge
from _recoil.lib.progress import ProgressDocument, address_value, normalize_address


def _prove_same_ordinal_compiler_provider_calls(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    name: str,
    provider_identity: str,
) -> dict[str, str]:
    """Prove an authorized helper spelling using exact live direct-call sites.

    The caller supplies independently governed provider identity. Every helper
    use must have that identity at its immutable retail ordinal, and all COFF
    references must be unique zero-addend E8/REL32 calls to one undefined
    external function. This acquires a spelling only; it never changes,
    removes, reorders, or accepts a call-contract row.
    """
    caller = candidate.caller_definition
    if caller is None:
        return {}
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    invocations = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start="0x0",
        caller_end_exclusive=hex(max(1, len(caller.data))),
    )
    sites = [
        (ordinal, int(offsets[index]), candidate.instructions[index])
        for ordinal, index in enumerate(invocations)
        if offsets[index] is not None
        and _cc_cfg._instruction_operand(candidate.instructions[index]).strip()
        == name
    ]
    if not sites:
        raise _cc_errors.CandidateCallContractEvidenceError(
            "Compiler-provider bridge lacks its exact helper population"
        )
    for ordinal, _offset, instruction in sites:
        if ordinal >= len(expected):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "Player compiler-provider bridge lacks immutable retail ordinal: "
                f"sites={tuple((item[0], hex(item[1])) for item in sites)!r}, "
                f"retail_count={len(expected)}"
            )
        row = expected[ordinal]
        if (
            row.get("ordinal") != ordinal
            or row.get("form") != "call"
            or row.get("dispatch") != "direct"
            or row.get("identity_kind") != "provider"
            or row.get("target_identity") != provider_identity
            or row.get("storage_identity") != ""
            or row.get("slot_displacement") is not None
            or row.get("cleanup_bytes") is not None
            or _cc_cfg._instruction_mnemonic(instruction) != "call"
        ):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "Player compiler-provider bridge rejects retail/provider shape drift: "
                f"sites={tuple((item[0], hex(item[1])) for item in sites)!r}, "
                f"site_ordinal={ordinal}, retail_row={dict(row)!r}"
            )
    exact_symbols = [row for row in caller.coff_symbols if row.name == name]
    references = sorted(
        (row for row in caller.relocations if row.symbol_name == name),
        key=lambda row: row.offset,
    )
    expected_reference_offsets = sorted(offset + 1 for _ordinal, offset, _ in sites)
    if (
        len(exact_symbols) != 1
        or caller.undefined_external_functions.count(name) != 1
        or name in caller.defined_external_functions
        or name in caller.undefined_external_data
        or name in caller.defined_external_data
        or len(caller.relocation_mask) != len(caller.data)
        or [row.offset for row in references] != expected_reference_offsets
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "Player compiler-provider bridge requires one exact undefined COFF population"
        )
    external = exact_symbols[0]
    reference_by_offset = {row.offset: row for row in references}
    if (
        external.section_number != 0
        or external.symbol_type != 0x20
        or external.storage_class != IMAGE_SYM_CLASS_EXTERNAL
        or external.aux_count != 0
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "Player compiler-provider bridge rejects malformed COFF external"
        )
    for _ordinal, offset, instruction in sites:
        reference = reference_by_offset[offset + 1]
        if (
            bytes(int(item, 16) for item in instruction.bytes)
            != b"\xe8\x00\x00\x00\x00"
            or reference.type != IMAGE_REL_I386_REL32
            or reference.symbol_index != external.index
            or reference.offset + 4 > len(caller.data)
            or caller.data[offset] != 0xE8
            or struct.unpack_from("<I", caller.data, reference.offset)[0] != 0
            or not all(caller.relocation_mask[reference.offset : reference.offset + 4])
        ):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "Player compiler-provider bridge rejects E8/REL32 provenance drift"
            )
    return {name: provider_identity}


def _r4578_zrender_static_comparison_callable(value: str) -> str:
    """Decode only the reviewed zrndr_draw STATIC helper spellings.

    VC5 COFF contains the fully decorated symbol, while the earlier bridge
    compared that symbol to a bare source name and therefore never selected
    STATIC handling.  Anonymous-namespace helpers additionally retain the
    exact absolute zrndr_draw.c scope (including VC5's percent marker); the
    three scoped helpers have exact class/global decorations.  No looser
    basename or suffix match is accepted.
    """

    if value in _cc_catalog._R4575_ZRENDER_STATIC_COMPARISON_ONLY_CALLABLES:
        # Retain the already-reviewed unsuffixed STATIC spelling; r4578 adds
        # the actual VC5 decorated spellings without changing that contract.
        return value
    exact_scoped = {
        "?zVideoFxPass3ClampCurrentRadius@zVideo@@YIHHH@Z": (
            "zVideoFxPass3ClampCurrentRadius"
        ),
        "?FxLineOutCode@zVideo_FxSurface@@YIHHHHHHH@Z": "FxLineOutCode",
        "?zRndrSpanDepthAtXByPartsLocal@@YIMHMMH@Z": (
            "zRndrSpanDepthAtXByPartsLocal"
        ),
        "?zVideoFxPass3ApproxRadiusIndex@zVideo@@YIHHH@Z": (
            "zVideoFxPass3ApproxRadiusIndex"
        ),
        "?zVideoFxPass3ScatterDirectSymmetric@zVideo@@YIXHHHHHH@Z": (
            "zVideoFxPass3ScatterDirectSymmetric"
        ),
        "?zVideoFxPass3ScatterClippedSymmetric@zVideo@@YIXHHHH@Z": (
            "zVideoFxPass3ScatterClippedSymmetric"
        ),
        "?zVideoFxPass3CopyDirect@zVideo@@YIXHHHHHH@Z": (
            "zVideoFxPass3CopyDirect"
        ),
        "?zVideoFxPass3CopyScratchToSurface@zVideo@@YIXHHHHH@Z": (
            "zVideoFxPass3CopyScratchToSurface"
        ),
        "?TruncateFloat@zVideo_FxSurface@@YIHM@Z": "TruncateFloat",
    }
    exact = exact_scoped.get(value)
    if exact is not None:
        return exact
    anonymous_suffixes = {
        "BlendLensFlarePixel": "YIGGGH@Z",
        "CommitFogParamsIfChanged": "YIXABUFogParamsPartial@2@@Z",
        "SpanTex16SampleIndex": "YIHHHHH@Z",
        "FogBlendPixel565": "YIGGI@Z",
        "FogBlendPixel555": "YIGGI@Z",
        "BlendPixel565Alpha8": "YIGGGH@Z",
        "BlendPixel555Alpha8": "YIGGGH@Z",
        "BlendPixel555ConstAlphaMap": "YIGGGH@Z",
        "FogBlendPair565": "YIIII@Z",
        "FogBlendPair555": "YIIII@Z",
    }
    for name, suffix in anonymous_suffixes.items():
        match = re.fullmatch(
            rf"\?{re.escape(name)}@\?%"
            rf"(?P<source>[A-Za-z]:\\[^@\r\n]+\\src\\GameZRecoil\\"
            rf"zRender\\zrndr_draw\.c)"
            rf"(?P<discriminator>[0-9]+)@zRndr@@{re.escape(suffix)}",
            value,
        )
        if match is not None:
            source = PureWindowsPath(match.group("source"))
            if (
                source.name == "zrndr_draw.c"
                and tuple(part.casefold() for part in source.parts[-4:])
                == ("src", "gamezrecoil", "zrender", "zrndr_draw.c")
                and match.group("discriminator").isdigit()
            ):
                return name
    return ""


def _candidate_local_coff_callable_bridges(
    candidate: CandidateAssembly,
    *,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    compiler_generated_bridges: Mapping[str, str],
) -> dict[str, str]:
    """Expose exact unreviewed same-object callables as comparison-only rows.

    A freshly compiled object may contain an inline/COMDAT helper which has no
    retail, tracker, import, or provider identity because retail emitted no such
    call at all.  Blocking extraction in that case hides the useful semantic
    divergence.  This bridge therefore creates only a candidate-local identity
    after proving the exact defining COFF symbol and every direct caller
    relocation.  The identity cannot satisfy retail comparison and grants no
    durable or reviewed state.
    """

    caller = candidate.caller_definition
    if caller is None or not caller.coff_symbols:
        return {}
    if (
        caller.section_index <= 0
        or caller.section_start < 0
        or caller.section_end <= caller.section_start
        or caller.section_end - caller.section_start != len(caller.data)
        or len(caller.relocation_mask) != len(caller.data)
    ):
        raise ValueError(
            "candidate-local COFF callable bridge requires one exact caller "
            "section and body extent"
        )

    invocations_by_name: dict[str, list[tuple[int, Instruction, int, int]]] = {}
    # Only COFF function definitions can seed comparison-only callable
    # identities.  VC5 also emits section-defined type-0 LABEL symbols (for
    # example ``$L73374``) for ordinary intra-function branches and switch
    # table entries.  Those labels remain caller-local control flow; treating
    # every positive-section symbol as callable makes an exact short JMP look
    # like a malformed tail call before the contract extractor can prove its
    # encoded, contained destination.
    defined_function_names = {
        row.name
        for row in caller.coff_symbols
        if row.section_number > 0 and row.symbol_type == 0x20
    }
    defined_coff_casefolds = {
        name.casefold() for name in defined_function_names
    }
    normalized_callable_name_by_cod_operand: dict[
        str, tuple[str, str]
    ] = {}
    for object_name in defined_function_names:
        normalized_cod_name = _cc_listing._vc5_compiler_normalized_cod_proc_symbol(
            object_name
        )
        if normalized_cod_name is not None:
            if normalized_cod_name in defined_function_names:
                raise ValueError(
                    "candidate-local COFF callable bridge normalized COD name "
                    f"{normalized_cod_name!r} collides with an exact COFF symbol"
                )
            route = (normalized_cod_name, object_name)
            prior_route = normalized_callable_name_by_cod_operand.get(
                normalized_cod_name.casefold()
            )
            if prior_route not in {None, route}:
                raise ValueError(
                    "candidate-local COFF callable bridge has an ambiguous or "
                    "case-conflicting normalized COD population for "
                    f"{normalized_cod_name!r}"
                )
            normalized_callable_name_by_cod_operand[
                normalized_cod_name.casefold()
            ] = route
    for instruction_index, instruction in enumerate(candidate.instructions):
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        if mnemonic not in {"call", "jmp"}:
            continue
        operand = _cc_cfg._instruction_operand(instruction).strip()
        callable_route = normalized_callable_name_by_cod_operand.get(
            operand.casefold()
        )
        if operand in defined_function_names:
            object_name = operand
        elif callable_route is not None:
            expected_cod_operand, object_name = callable_route
            if operand != expected_cod_operand:
                raise ValueError(
                    "candidate-local COFF callable bridge has a case-conflicting "
                    f"COD operand for {object_name!r}"
                )
        elif operand.casefold() in defined_coff_casefolds:
            # Preserve the established exact-definition population proof below
            # for ordinary case-conflicting symbols.  Only the reviewed VC5
            # percent-marker spelling uses the normalized route above.
            object_name = operand
        else:
            continue
        raw_offset = _cc_cfg._source_instruction_address(instruction)
        opcode = 0xE8 if mnemonic == "call" else 0xE9
        if (
            instruction_index in candidate.local_control_flow_indices
            or not raw_offset
            or len(instruction.bytes) != 5
            or tuple(value.lower() for value in instruction.bytes)
            != (f"{opcode:02x}", "00", "00", "00", "00")
        ):
            raise ValueError(
                "candidate-local COFF callable bridge requires exact nonlocal "
                f"zero-addend E8/E9 COD evidence for {operand!r}"
            )
        invocations_by_name.setdefault(object_name, []).append(
            (
                instruction_index,
                instruction,
                address_value(raw_offset),
                opcode,
            )
        )

    result: dict[str, str] = {}
    for name, invocations in invocations_by_name.items():
        # Existing exact authorities keep their normal resolver.  A differently
        # cased authority is never allowed to be silently shadowed by a local
        # object symbol.
        exact_authorities = (
            name in indexes.by_candidate_name,
            name in bridge_names,
            name in compiler_generated_bridges,
        )
        is_vector_destroy = name.startswith(
            _cc_catalog.MSVC_VECTOR_DESTROY_PREFIX
        )
        is_hud_panel_layout_vector_destroy = (
            name == _cc_catalog.HUD_PANEL_LAYOUT_VECTOR_DESTROY_SYMBOL
        )
        is_hud_composite_panel_vector_destroy = (
            name == _cc_catalog.HUD_COMPOSITE_PANEL_VECTOR_DESTROY_SYMBOL
        )
        is_noop_vector_destroy = (
            is_vector_destroy
            and not is_hud_panel_layout_vector_destroy
            and not is_hud_composite_panel_vector_destroy
        )
        is_hud_cmd_binding_ptr_vector_erase = (
            name == _cc_catalog.HUD_CMD_BINDING_PTR_VECTOR_ERASE_SYMBOL
        )
        is_player_bind_group_scalar_destructor = (
            name == "??_GCZInputBindGroupInfo@@QAEPAXI@Z"
            and caller.symbol == "?BindGroupListClear@zInput@@YAXXZ"
        )
        is_player_bind_group_int_vector_destructor = (
            name == "??1?$vector@HV?$allocator@H@std@@@std@@QAE@XZ"
            and caller.symbol == "??1CZInputBindGroupInfo@@QAE@XZ"
        )
        is_reviewed_candidate_local_provider = (
            is_noop_vector_destroy
            or is_hud_panel_layout_vector_destroy
            or is_hud_composite_panel_vector_destroy
            or is_hud_cmd_binding_ptr_vector_erase
            or is_player_bind_group_scalar_destructor
            or is_player_bind_group_int_vector_destructor
        )
        if any(exact_authorities) and not is_reviewed_candidate_local_provider:
            continue
        folded_name = name.casefold()
        for label, names in (
            ("candidate identity", indexes.by_candidate_name),
            ("Binary Ninja identity", bridge_names),
            ("compiler/provider identity", compiler_generated_bridges),
            ("candidate storage identity", indexes.storage_by_name),
        ):
            collisions = sorted(
                item
                for item in names
                if item.casefold() == folded_name and item != name
            )
            if collisions:
                raise ValueError(
                    "candidate-local COFF callable bridge has a case-folded "
                    f"{label} conflict for {name!r}: "
                    + ", ".join(repr(item) for item in collisions)
                )
        if name in indexes.storage_by_name or "__imp_" in name:
            raise ValueError(
                "candidate-local COFF callable bridge conflicts with import "
                f"or storage identity for {name!r}"
            )

        folded_defined = [
            item
            for item in caller.defined_external_functions
            if item.casefold() == folded_name
        ]
        folded_undefined = [
            item
            for item in caller.undefined_external_functions
            if item.casefold() == folded_name
        ]
        static_comparison_name = _r4578_zrender_static_comparison_callable(
            name
        )
        static_comparison_only = bool(
            static_comparison_name
            and static_comparison_name
            in _cc_catalog._R4575_ZRENDER_STATIC_COMPARISON_ONLY_CALLABLES
        )
        if (
            (folded_defined != [name] or folded_undefined)
            if not static_comparison_only
            else bool(folded_defined or folded_undefined)
        ):
            raise ValueError(
                "candidate-local COFF callable bridge requires one exact "
                "defined external callable population and no undefined, duplicate, or "
                f"case-folded external for {name!r}"
            )

        folded_symbols = [
            row
            for row in caller.coff_symbols
            if row.name.casefold() == folded_name
        ]
        if len(folded_symbols) != 1 or folded_symbols[0].name != name:
            raise ValueError(
                "candidate-local COFF callable bridge requires one exact, "
                f"non-duplicate defining symbol for {name!r}"
            )
        definition = folded_symbols[0]
        weak_aliases = [
            row
            for row in caller.coff_symbols
            if row.storage_class == 105
            and (
                row.name.casefold() == folded_name
                or row.weak_external_tag_index == definition.index
            )
        ]
        coincident_callables = [
            row
            for row in caller.coff_symbols
            if row.index != definition.index
            and row.section_number == definition.section_number
            and row.value == definition.value
            and row.symbol_type == 0x20
        ]
        if weak_aliases or coincident_callables:
            raise ValueError(
                "candidate-local COFF callable bridge rejects weak, alias, or "
                f"coincident callable identity for {name!r}"
            )
        if (
            definition.section_number <= 0
            or definition.storage_class
            != (
                IMAGE_SYM_CLASS_STATIC
                if static_comparison_only
                else IMAGE_SYM_CLASS_EXTERNAL
            )
            or definition.symbol_type != 0x20
            or not definition.section_name
            or definition.section_size <= 0
            or not (definition.section_characteristics & IMAGE_SCN_CNT_CODE)
            or definition.value < 0
            or definition.natural_end <= definition.value
            or definition.natural_end > definition.section_size
        ):
            raise ValueError(
                "candidate-local COFF callable bridge requires an exact "
                "external function in the caller code section with a positive "
                f"well-formed extent for {name!r}"
            )

        matching_relocations = [
            relocation
            for relocation in caller.relocations
            if relocation.symbol_name.casefold() == folded_name
        ]
        expected_offsets = [
            instruction_offset + 1
            for _index, _instruction, instruction_offset, _opcode in invocations
        ]
        if (
            len(matching_relocations) != len(invocations)
            or len(set(expected_offsets)) != len(expected_offsets)
            or sorted(relocation.offset for relocation in matching_relocations)
            != sorted(expected_offsets)
        ):
            raise ValueError(
                "candidate-local COFF callable bridge requires one exact "
                f"ordered caller relocation per invocation of {name!r}"
            )
        relocation_by_offset = {
            relocation.offset: relocation
            for relocation in matching_relocations
        }
        opcode_by_relocation_offset = {
            instruction_offset + 1: opcode
            for _index, _instruction, instruction_offset, opcode in invocations
        }
        for expected_offset in expected_offsets:
            relocation = relocation_by_offset[expected_offset]
            relative = relocation.offset - caller.section_start
            if (
                relocation.type != IMAGE_REL_I386_REL32
                or relocation.symbol_name != name
                or relocation.symbol_index != definition.index
                or relative <= 0
                or relative + 4 > len(caller.data)
                or caller.data[relative - 1]
                != opcode_by_relocation_offset[expected_offset]
                or struct.unpack_from("<I", caller.data, relative)[0] != 0
                or caller.relocation_mask[relative - 1]
                or not all(
                    caller.relocation_mask[index]
                    for index in range(relative, relative + 4)
                )
            ):
                raise ValueError(
                    "candidate-local COFF callable bridge requires exact "
                    "REL32 target, zero addend, opcode mask, and defining "
                    f"symbol provenance for {name!r}"
                )

        if is_noop_vector_destroy:
            # A same-object VC5 vector<T>::_Destroy definition can be the
            # candidate spelling of the unique reviewed retail no-op provider,
            # but its decoration alone is not provider evidence.  Require the
            # independently parsed natural COFF definition to contain only the
            # three-byte return body and exact VC5 NOP padding, canonical VC5
            # header provenance, and no relocations before allowing the
            # reviewed supplier/tracker/BN identity resolver to arbitrate it.
            helper_definition = candidate.tu_local_function_definitions.get(
                name
            )
            # Trivial value elements use this same complete body/ABI proof.
            # No caller address or neighboring template body can substitute
            # for the exact COMDAT and independently registered supplier.
            same_section_external_symbols = tuple(
                row.name
                for row in caller.coff_symbols
                if row.section_number == definition.section_number
                and row.storage_class == IMAGE_SYM_CLASS_EXTERNAL
            )
            expected_destroy_comdat = b"\xc2\x08\x00" + (b"\x90" * 13)
            try:
                provenance = Path(helper_definition.source_provenance).resolve()
                include_root = (
                    compiler_env_path(candidate.target, DEFAULT_VC5_ENV)
                    .resolve()
                    .parent
                    / "VC"
                    / "INCLUDE"
                ).resolve()
                provenance.relative_to(include_root)
            except (AttributeError, OSError, ValueError, TypeError) as exc:
                raise ValueError(
                    "candidate-local no-op vector destroy provider "
                    "arbitration requires exact canonical VC5 include COD "
                    "provenance"
                ) from exc
            if (
                helper_definition is None
                or helper_definition.symbol != name
                or helper_definition.data != expected_destroy_comdat
                or helper_definition.relocations
                or helper_definition.relocation_mask
                != ((False,) * len(expected_destroy_comdat))
                or helper_definition.section_size
                != len(expected_destroy_comdat)
                or helper_definition.section_external_functions != (name,)
                or helper_definition.section_is_comdat is not True
                or helper_definition.comdat_selection != 2
                or same_section_external_symbols != (name,)
                or definition.value != 0
                or definition.natural_end != len(expected_destroy_comdat)
                or definition.section_size != len(expected_destroy_comdat)
                or not (
                    definition.section_characteristics
                    & _cc_catalog.IMAGE_SCN_LNK_COMDAT
                )
            ):
                raise ValueError(
                    "candidate-local no-op vector destroy provider "
                    "arbitration requires one exact external, unaliased VC5 "
                    "COMDAT with a natural C2 08 00 body, exact NOP-padded "
                    "extent, and an empty relocation population for "
                    f"{name!r}"
                )
            result[name] = _cc_targets._pointer_vector_destroy_provider_identity(
                name,
                indexes=indexes,
                bridge_names=bridge_names,
                compiler_generated_bridges=compiler_generated_bridges,
                allow_proven_value_noop=True,
            )
            continue

        if (
            is_hud_panel_layout_vector_destroy
            or is_hud_composite_panel_vector_destroy
        ):
            # This value-vector specialization is not the established
            # pointer/no-op provider.  Preserve its exact same-object COMDAT
            # as a comparison-only sentinel; the later retail-scoped arbiter
            # alone may replace it after proving the independent 0x409b60
            # provider extent, body, relocation, and destructor target.
            identity = f"candidate-local-coff:{name}"
            if (
                identity in indexes.provider_ids
                or identity in indexes.by_address.values()
                or identity in indexes.by_candidate_name.values()
                or identity in compiler_generated_bridges.values()
            ):
                raise ValueError(
                    "HUD panel-layout vector destroy comparison-only identity "
                    "conflicts with reviewed candidate/provider state"
                )
            result[name] = identity
            continue

        if is_hud_cmd_binding_ptr_vector_erase:
            # The candidate emits the exact reviewed erase implementation as a
            # same-object SELECT_ANY COMDAT.  A matching decoration alone is
            # insufficient: prove the complete retail-derived body, exact VC5
            # padding/extent, relocation-free definition, and unaliased COFF
            # population before consulting the reviewed provider supplier.
            helper_definition = candidate.tu_local_function_definitions.get(
                name
            )
            same_section_external_symbols = tuple(
                row.name
                for row in caller.coff_symbols
                if row.section_number == definition.section_number
                and row.storage_class == IMAGE_SYM_CLASS_EXTERNAL
            )
            expected_erase_comdat = (
                _cc_catalog.HUD_CMD_BINDING_PTR_VECTOR_ERASE_RETAIL_BODY
                + (b"\x90" * 14)
            )
            if (
                helper_definition is None
                or helper_definition.symbol != name
                or helper_definition.data != expected_erase_comdat
                or helper_definition.relocations
                or helper_definition.relocation_mask
                != ((False,) * len(expected_erase_comdat))
                or helper_definition.section_size
                != len(expected_erase_comdat)
                or helper_definition.section_external_functions != (name,)
                or helper_definition.section_is_comdat is not True
                or helper_definition.comdat_selection != 2
                or same_section_external_symbols != (name,)
                or definition.value != 0
                or definition.natural_end != len(expected_erase_comdat)
                or definition.section_size != len(expected_erase_comdat)
                or not (
                    definition.section_characteristics
                    & _cc_catalog.IMAGE_SCN_LNK_COMDAT
                )
            ):
                raise ValueError(
                    "candidate-local HudCmdBindingEntry pointer-vector erase "
                    "provider arbitration requires one exact external, "
                    "unaliased VC5 SELECT_ANY COMDAT with the complete "
                    "retail-derived body, exact NOP-padded extent, and an "
                    f"empty relocation population for {name!r}"
                )
            result[name] = (
                _cc_targets._hud_cmd_binding_ptr_vector_erase_provider_identity(
                    name,
                    indexes=indexes,
                    bridge_names=bridge_names,
                    compiler_generated_bridges=compiler_generated_bridges,
                )
            )
            continue

        if _cc_targets._is_zsnd_release_unknown_coff_name(name):
            helper_definition = candidate.tu_local_function_definitions.get(
                name
            )
            same_section_external_symbols = tuple(
                row.name
                for row in caller.coff_symbols
                if row.section_number == definition.section_number
                and row.storage_class == IMAGE_SYM_CLASS_EXTERNAL
            )
            try:
                source_provenance = Path(
                    helper_definition.source_provenance
                ).resolve()
            except (AttributeError, OSError, TypeError, ValueError) as exc:
                raise ValueError(
                    "zSnd ReleaseUnknown candidate-local bridge requires "
                    "exact source provenance"
                ) from exc
            if (
                _cc_listing._vc5_compiler_normalized_cod_proc_symbol(name)
                != name.replace("@?%", "@?", 1)
                or caller.symbol != "?Shutdown@zSndBackend@@YAHXZ"
                or caller.section_index != 27
                or caller.section_start != 0
                or caller.section_end != 0x90
                or len(caller.data) != 0x90
                or len(invocations) != 1
                or invocations[0][2:] != (0x2F, 0xE8)
                or helper_definition is None
                or helper_definition.symbol != name
                or helper_definition.data != _cc_catalog.ZSND_RELEASE_UNKNOWN_BODY
                or helper_definition.relocations
                or helper_definition.relocation_mask
                != ((False,) * len(_cc_catalog.ZSND_RELEASE_UNKNOWN_BODY))
                or helper_definition.section_size != 0x20
                or helper_definition.section_external_functions != (name,)
                or helper_definition.section_is_comdat is not True
                or helper_definition.comdat_selection != 1
                or source_provenance
                != _cc_catalog.ZSND_RELEASE_UNKNOWN_SOURCE_PATH.resolve()
                or same_section_external_symbols != (name,)
                or definition.value != 0
                or definition.natural_end != 0x20
                or definition.section_size != 0x20
                or not (
                    definition.section_characteristics
                    & _cc_catalog.IMAGE_SCN_LNK_COMDAT
                )
            ):
                raise ValueError(
                    "zSnd ReleaseUnknown candidate-local bridge requires the "
                    "exact canonical COD/COFF name, caller callsite, helper "
                    "body, relocation-free COMDAT, source, and TU population"
                )

        identity = f"candidate-local-coff:{name}"
        if (
            identity in indexes.provider_ids
            or identity in indexes.by_address.values()
            or identity in indexes.by_candidate_name.values()
            or identity in compiler_generated_bridges.values()
        ):
            raise ValueError(
                "candidate-local COFF callable bridge identity conflicts with "
                f"reviewed candidate/provider state for {name!r}"
            )
        result[name] = identity
    return result


def _candidate_canonical_header_provider_matches(
    helper: CandidateTuLocalFunctionDefinition,
    candidate: CandidateAssembly,
    provider: Mapping[str, Any],
    retail_body: bytes,
    *, indexes: IdentityIndexes,
    retail_address: str,
) -> bool:
    """Join a fresh canonical header proof with independent retail targets.

    The canonical probe supplies relocation positions, types, names and
    addends. Candidate fields cannot determine which retail bytes are masked
    or which target is expected. Logical aliases carrying relocations remain
    unsupported until each specialization has its own typed target proof.
    """
    from _recoil.commands.provider_function_mutation import (
        ProviderFunctionMutationError,
        _provider_header_comdat_proof,
    )

    names = _canonical_header_provider_catalog_names(provider)
    if helper.symbol not in names:
        return False
    catalog = provider["provider_object_identity"]
    if (
        len(retail_body) != len(helper.data)
        or len(helper.relocation_mask) != len(helper.data)
        or catalog.get("body_size") != len(helper.data)
        or catalog.get("comdat_selection") != helper.comdat_selection
    ):
        return False
    try:
        compiler_root = compiler_env_path(candidate.target, DEFAULT_VC5_ENV).resolve().parent
        header = catalog["canonical_header"]
        if not isinstance(header, str) or "\\" in header:
            return False
        parts = PurePosixPath(header)
        if parts.is_absolute() or ".." in parts.parts or parts.as_posix() != header:
            return False
        expected_header = (compiler_root / Path(*parts.parts)).resolve()
        expected_header.relative_to(compiler_root)
        if Path(helper.source_provenance).resolve() != expected_header:
            return False
        proof = _provider_header_comdat_proof(
            vc5_root=compiler_root,
            request={
                "object_symbol": catalog["object_symbol"],
                "probe_recipe": catalog["probe_recipe"],
                "canonical_header": header,
                "semantic_provider": catalog["semantic_provider"],
                "physical_emitter_state": "winner-unknown",
                "retail_icf_winner_status": "winner-unknown",
                "retail_icf_logical_symbols": list(names),
            },
            body_size=len(retail_body), retail_body=retail_body,
        )
    except (KeyError, OSError, TypeError, ValueError, ProviderFunctionMutationError) as exc:
        raise ValueError(f"canonical-header provider live proof failed: {exc}") from exc
    if (
        proof.comdat_selection != helper.comdat_selection
        or proof.body_size != len(helper.data)
        or len(proof.relocations) != len(helper.relocations)
    ):
        return False
    expected_fields: dict[int, tuple[int, str]] = {}
    masked: set[int] = set()
    for relocation in proof.relocations:
        offset, kind = relocation.get("offset"), relocation.get("type_value")
        if (
            type(offset) is not int or offset < 0 or offset + 4 > len(retail_body)
            or kind not in {IMAGE_REL_I386_DIR32, IMAGE_REL_I386_REL32}
            or relocation.get("width") != 4 or relocation.get("addend") != 0
            or masked.intersection(range(offset, offset + 4))
        ):
            return False
        name = relocation.get("target_symbol")
        target_identity = (indexes.by_candidate_name.get(name, "")
                           or indexes.storage_by_name.get(name, ""))
        if not target_identity:
            return False
        target = struct.unpack_from("<I", retail_body, offset)[0]
        if kind == IMAGE_REL_I386_REL32:
            target = (address_value(retail_address) + offset + 4 + target) & 0xffffffff
        actual_identity = (indexes.by_address.get(normalize_address(target), "")
                           or indexes.storage_by_address.get(normalize_address(target), ""))
        if actual_identity != target_identity:
            return False
        expected_fields[offset] = (kind, target_identity)
        masked.update(range(offset, offset + 4))
    if proof.masked_byte_count != len(masked) or {
        index for index, value in enumerate(helper.relocation_mask) if value
    } != masked:
        return False
    observed_offsets: set[int] = set()
    for relocation in helper.relocations:
        if relocation.offset in observed_offsets or relocation.offset not in expected_fields:
            return False
        observed_offsets.add(relocation.offset)
        expected_kind, expected_identity = expected_fields[relocation.offset]
        actual_identity = (indexes.by_candidate_name.get(relocation.symbol_name, "")
                           or indexes.storage_by_name.get(relocation.symbol_name, ""))
        if (
            relocation.type != expected_kind or actual_identity != expected_identity
            or struct.unpack_from("<I", helper.data, relocation.offset)[0] != 0
        ):
            return False
    return observed_offsets == set(expected_fields) and all(
        index in masked or actual == expected
        for index, (actual, expected) in enumerate(zip(helper.data, retail_body))
    )


def _candidate_only_provider_comdat_catalog_matches(
    callable_symbol: str,
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    indexes: IdentityIndexes,
    bridge: BinaryNinjaBridge,
) -> tuple[str, ...]:
    """Match a native helper through governed canonical-header registrations.

    Caller identity, expected ordinal/target, and expected cleanup are absent.
    The typed registration must explicitly carry the callable key; matching
    bytes alone cannot distinguish template specializations.
    """
    helper = candidate.tu_local_function_definitions.get(callable_symbol)
    caller = candidate.caller_definition
    if helper is None or caller is None:
        return ()
    exact_symbols = tuple(row for row in caller.coff_symbols if row.name == callable_symbol)
    if len(exact_symbols) != 1:
        return ()
    symbol = exact_symbols[0]
    aliases = tuple(
        row for row in caller.coff_symbols
        if row.index != symbol.index and (
            row.weak_external_tag_index == symbol.index
            or (row.section_number == symbol.section_number and row.value == symbol.value
                and row.storage_class == IMAGE_SYM_CLASS_EXTERNAL)
        )
    )
    if (
        aliases or not helper.section_is_comdat or helper.comdat_selection is None
        or helper.section_external_functions != (callable_symbol,)
        or helper.section_size != len(helper.data)
    ):
        return ()
    matches: list[str] = []
    for symbol_id, provider in document.collection("symbols").items():
        if not isinstance(provider, Mapping):
            continue
        identity = _cc_identity._symbol_identity(str(symbol_id), provider)
        raw_address = provider.get("address", provider.get("start"))
        if (
            identity not in indexes.provider_ids
            or provider.get("pipeline_class") != "non-authored"
            or provider.get("authored_order_role") != "non-authored"
            or provider.get("extent_state") != "known"
            or not isinstance(raw_address, str) or provider.get("size") != len(helper.data)
            or callable_symbol not in _canonical_header_provider_catalog_names(provider)
        ):
            continue
        address = normalize_address(raw_address)
        if indexes.by_address.get(address) != identity:
            continue
        retail_body = _cc_cfg._hexdump_bytes(bridge.hexdump(address, len(helper.data)))
        if _candidate_canonical_header_provider_matches(
            helper, candidate, provider, retail_body,
            indexes=indexes, retail_address=address,
        ):
            matches.append(identity)
    return tuple(sorted(matches))


def _require_catalogued_provider_collisions(
    actual: Mapping[str, str], catalog: Mapping[str, str], *, helper_name: str,
) -> None:
    """Check present aliases only, after independent current-helper proof.

    A catalogue is an allowed identity set, not a required population in each
    translation unit. Missing aliases grant nothing; foreign names or identities
    still reject the comparison.
    """
    if any(catalog.get(name) != identity for name, identity in actual.items()):
        raise ValueError(
            "comparison-scoped provider COMDAT arbitration rejects candidate "
            f"identity collisions for {helper_name!r}: actual={dict(actual)!r}, "
            f"catalog={dict(catalog)!r}")


def _canonical_header_provider_catalog_names(symbol: Mapping[str, Any]) -> tuple[str, ...]:
    """Read only explicit accepted provider names, never infer new aliases."""
    provider_object = symbol.get("provider_object_identity")
    if not isinstance(provider_object, Mapping) or (
        provider_object.get("schema") != "recoil-provider-function-object-v2"
        or provider_object.get("proof_mode") != "canonical-header-comdat"
    ):
        return ()
    retail_icf = provider_object.get("retail_icf")
    logical = retail_icf.get("logical_symbols") if isinstance(retail_icf, Mapping) else None
    if (
        not isinstance(logical, list)
        or not logical
        or any(not isinstance(item, str) or not item for item in logical)
        or len(set(logical)) != len(logical)
        or provider_object.get("object_symbol") not in logical
        or symbol.get("object_symbol") != provider_object.get("object_symbol")
    ):
        raise ValueError("provider COMDAT catalog has malformed logical names")
    return tuple(logical)


def candidate_provider_comdat_contract(
    candidate_contract: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *, document: ProgressDocument, indexes: IdentityIndexes, bridge: BinaryNinjaBridge,
) -> list[dict[str, Any]]:
    """Resolve native COMDAT identities independently of caller ordinals.

    The call-site population and physical form are retained. No provider match
    leaves the candidate identity unresolved; multiple matches are a conflict.
    A wrapper or call-free helper cannot disappear through this identity join.
    """

    result, _receipts = _cc_callable_identity._candidate_only_direct_callee_cleanup_contract(candidate_contract, candidate)
    by_name: dict[str, tuple[str, ...]] = {}
    for row in result:
        identity = row.get("target_identity")
        if not isinstance(identity, str) or not identity.startswith("candidate-local-coff:"):
            continue
        name = identity.removeprefix("candidate-local-coff:")
        if name not in by_name:
            by_name[name] = _candidate_only_provider_comdat_catalog_matches(
                name, candidate, document=document, indexes=indexes, bridge=bridge)
        matches = by_name[name]
        if len(matches) > 1:
            raise ValueError(f"native provider identity conflicts for {name!r}: {matches!r}")
        if len(matches) == 1:
            row["identity_kind"] = "provider"
            row["target_identity"] = matches[0]
    return result
