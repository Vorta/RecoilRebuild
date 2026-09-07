"""Recoil call-contract recoil ui evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import reporting as _cc_reporting

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        CandidateTuLocalFunctionDefinition,
        IdentityIndexes,
    )

import json
import re
from copy import deepcopy
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_DIR32,
    IMAGE_REL_I386_REL32,
    IMAGE_SYM_CLASS_EXTERNAL,
    CoffRelocation,
)
from _recoil.lib.progress import normalize_address


def _canonical_zui_relocation_names(
    relocations: Sequence[CoffRelocation],
) -> tuple[str, ...]:
    """Remove unstable VC5 serials without weakening relocation topology.

    Function-local ``$L``/``$T`` symbols are numbered from a translation-unit
    compiler counter, so an edit to a later function can renumber otherwise
    byte-identical earlier artifacts. Canonicalize each private label by its
    kind and first-occurrence equivalence class. Offsets, relocation types,
    ordinary symbol identities, TU-local helper kind, and repeated-label
    equality remain exact in the enclosing relocation transcript.
    """

    private_aliases: dict[str, str] = {}
    next_alias_by_kind = {"L": 0, "T": 0}
    result: list[str] = []
    for row in relocations:
        raw_name = row.symbol_name
        name = _cc_catalog._ZUI_TU_RELOCATION_SYMBOL.sub(
            r"\1<tu-discriminator>", raw_name
        )
        private_match = _cc_catalog._VC5_PRIVATE_RELOCATION_LABEL.fullmatch(raw_name)
        if private_match is not None:
            kind = private_match.group("kind")
            alias = private_aliases.get(raw_name)
            if alias is None:
                alias = f"${kind}<local-{next_alias_by_kind[kind]}>"
                private_aliases[raw_name] = alias
                next_alias_by_kind[kind] += 1
            name = alias
        result.append(name)
    return tuple(result)


def _zui_relocation_payload(
    relocations: Sequence[CoffRelocation],
) -> bytes:
    names = _canonical_zui_relocation_names(relocations)
    return json.dumps(
        [
            [row.offset, row.type, name]
            for row, name in zip(relocations, names)
        ],
        ensure_ascii=True,
        separators=(",", ":"),
    ).encode("utf-8")


def _zui_candidate_local_vector_occurrence_projection(
    expected: Sequence[Mapping[str, Any]],
    candidate_contract: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[list[dict[str, Any]], dict[str, Any] | None]:
    """Project one exact VC5 pointer-vector inline occurrence package.

    The retail 0x4b59f0 body inlines six element-copy loops but retains one
    call to the address-backed ``CopySingleDword`` COMDAT in each loop.  The
    governed candidate retains four ``_Ucopy`` and two ``_Ufill`` occurrences
    as call-free local COMDATs, plus one call-free ``size`` bookkeeping
    occurrence.  This finite proof authenticates the complete caller artifact
    and all three local definitions, replaces each
    copy/fill occurrence in place with the independently catalogued retail
    provider leaf, and removes only the call-free bookkeeping occurrences.
    The ordinary downstream provider arbitration and direct retail comparator
    remain solely responsible for accepting the resulting full contract.
    """
    from _recoil.call_contract.records import CandidateAssembly

    profile = (
        caller_identity,
        normalize_address(caller_start),
        normalize_address(caller_end_exclusive),
    )
    if profile != (
        "symbol:recoil:function:0x4b59f0",
        "0x4b59f0",
        "0x4b6fc0",
    ):
        return [dict(row) for row in candidate_contract], None

    prefix = "zUI candidate-only pointer-vector occurrence graph"
    caller = candidate.caller_definition
    caller_symbol = (
        "?LoadFromZrd@HudUiZrdWidget@@UAEHPAUNode@zReader@@"
        "PAUHudUiBackground@@@Z"
    )
    if caller is None:
        raise ValueError(f"{prefix} lacks the complete caller definition")
    if (
        caller.symbol != caller_symbol
        or len(caller.data) != 0x15E0
        or len(caller.relocations) != 150
        or len(caller.relocation_mask) != len(caller.data)
        or any(
            caller.relocation_mask[index]
            != any(
                row.offset <= index < row.offset + 4
                for row in caller.relocations
            )
            for index in range(len(caller.data))
        )
    ):
        raise ValueError(
            f"{prefix} rejects caller symbol, extent, body, relocation, or "
            "boundary drift"
        )

    size = (
        "?size@?$vector@PAUHudUiPanel@@"
        "V?$allocator@PAUHudUiPanel@@@std@@@std@@QBEIXZ"
    )
    ucopy = (
        "?_Ucopy@?$vector@PAUHudUiPanel@@"
        "V?$allocator@PAUHudUiPanel@@@std@@@std@@"
        "IAEPAPAUHudUiPanel@@PBQAU3@0PAPAU3@@Z"
    )
    ufill = (
        "?_Ufill@?$vector@PAUHudUiPanel@@"
        "V?$allocator@PAUHudUiPanel@@@std@@@std@@"
        "IAEXPAPAUHudUiPanel@@IABQAU3@@Z"
    )
    insert_many = (
        "?insert@?$vector@PAUHudUiPanel@@"
        "V?$allocator@PAUHudUiPanel@@@std@@@std@@"
        "QAEXPAPAUHudUiPanel@@IABQAU3@@Z"
    )
    helper_specs = {
        size: bytes.fromhex(
            "8b510485d2750333c0c38b41082bc2c1f802c3" + "90" * 13
        ),
        ucopy: bytes.fromhex(
            "8b4c24048b5424083bca741b8b44240c5685c074048b31893083c104"
            "83c0043bca75ee5ec20c008b44240cc20c00" + "90" * 2
        ),
        ufill: bytes.fromhex(
            "8b4c240885c976188b54240c8b4424045685c074048b32893083c004"
            "4975f25ec20c00" + "90" * 13
        ),
    }
    definitions = candidate.tu_local_function_definitions
    vector_population = {
        name for name in definitions
        if "?$vector@PAUHudUiPanel@@" in name
    }
    if not {*helper_specs, insert_many}.issubset(vector_population):
        raise ValueError(
            f"{prefix} lacks a required local vector definition: "
            f"observed={sorted(vector_population)!r}"
        )
    helper_receipts: list[dict[str, Any]] = []
    for name, body in helper_specs.items():
        definition = definitions.get(name)
        exact_symbols = tuple(
            row for row in caller.coff_symbols if row.name == name
        )
        exact_symbol = exact_symbols[0] if len(exact_symbols) == 1 else None
        if (
            definition is None
            or definition.symbol != name
            or definition.data != body
            or definition.section_size != len(body)
            or definition.section_is_comdat is not True
            or definition.comdat_selection != 2
            or definition.section_external_functions != (name,)
            or definition.relocations
            or any(definition.relocation_mask)
            or definition.local_control_flow_indices
            or definition.local_control_flow_targets
            or exact_symbol is None
            or exact_symbol.value != 0
            or exact_symbol.symbol_type != 0x20
            or exact_symbol.storage_class != IMAGE_SYM_CLASS_EXTERNAL
            or exact_symbol.natural_end != len(body)
            or exact_symbol.section_size != len(body)
            or not (exact_symbol.section_characteristics & _cc_catalog.IMAGE_SCN_LNK_COMDAT)
            or not definition.source_provenance.replace("\\", "/").endswith(
                "/VC/INCLUDE/vector"
            )
        ):
            raise ValueError(
                f"{prefix} rejects body, extent, selection, relocation, "
                f"collision, CFG, or provenance drift for {name!r}"
            )
        helper_assembly = CandidateAssembly(
            instructions=definition.instructions,
            local_control_flow_indices=definition.local_control_flow_indices,
            local_control_flow_targets=definition.local_control_flow_targets,
        )
        if _cc_callable_identity._candidate_static_invocation_indices(
            helper_assembly,
            caller_start="0x0",
            caller_end_exclusive=hex(len(body)),
        ):
            raise ValueError(f"{prefix} rejects a non-call-free local helper")
        helper_receipts.append({
            "symbol": name,
            "body_bytes_hex": _cc_reporting._direct_bytes(body).hex(),
            "relocations": [],
            "call_population": 0,
        })

    local_sites = (
        (0x142B, ucopy),
        (0x143E, ufill),
        (0x1452, ucopy),
        (0x1473, size),
        (0x149B, ucopy),
        (0x14BA, ufill),
        (0x14DD, ucopy),
    )
    cleanup_by_symbol = {name: None for name in helper_specs}
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start="0x0",
        caller_end_exclusive=hex(len(caller.data)),
    )
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    if len(invocation_indices) != len(candidate_contract):
        raise ValueError(f"{prefix} rejects caller invocation population drift")
    selected_ordinals: dict[int, str] = {}
    observed_sites: list[tuple[int, str]] = []
    for ordinal, index in enumerate(invocation_indices):
        if index >= len(offsets) or offsets[index] is None:
            raise ValueError(f"{prefix} rejects unresolved caller offsets")
        offset = int(offsets[index])
        relocation = next(
            (
                row for row in caller.relocations
                if row.offset == offset + 1
                and row.type == IMAGE_REL_I386_REL32
            ),
            None,
        )
        if relocation is None or relocation.symbol_name not in helper_specs:
            continue
        name = relocation.symbol_name
        observed_sites.append((offset, name))
        exact_row = {
            "ordinal": ordinal,
            "form": "call",
            "dispatch": "direct",
            "identity_kind": "direct",
            "target_identity": f"candidate-local-coff:{name}",
            "storage_identity": "",
            "slot_displacement": None,
            "cleanup_bytes": cleanup_by_symbol[name],
        }
        if dict(candidate_contract[ordinal]) != exact_row:
            raise ValueError(
                f"{prefix} rejects local-row identity drift at caller "
                f"offset {hex(offset)} ordinal {ordinal}: "
                f"observed={dict(candidate_contract[ordinal])!r}, "
                f"required={exact_row!r}"
            )
        selected_ordinals[ordinal] = name
    if tuple(observed_sites) != local_sites:
        raise ValueError(
            f"{prefix} rejects local occurrence order/population drift: "
            f"observed={tuple(observed_sites)!r}"
        )

    copy_identity = indexes.by_address.get("0x40c1c0")
    if (
        copy_identity != "provider:recoil:function:0x40c1c0"
        or copy_identity not in indexes.provider_ids
    ):
        raise ValueError(
            f"{prefix} lacks independently catalogued CopySingleDword authority"
        )
    projected: list[dict[str, Any]] = []
    expanded_leaves: list[dict[str, Any]] = []
    for ordinal, raw_row in enumerate(candidate_contract):
        name = selected_ordinals.get(ordinal)
        if name == size:
            continue
        if name in {ucopy, ufill}:
            row = {
                "ordinal": len(projected),
                "form": "call",
                "dispatch": "direct",
                "identity_kind": "provider",
                "target_identity": copy_identity,
                "storage_identity": "",
                "slot_displacement": None,
                "cleanup_bytes": None,
            }
            projected.append(row)
            expanded_leaves.append(deepcopy(row))
            continue
        row = dict(raw_row)
        row["ordinal"] = len(projected)
        projected.append(row)
    return projected, {
        "kind": "candidate-local-helper-graph-expansion-receipt",
        "contract_version": 1,
        "candidate_expected_truth": False,
        "caller_identity": caller_identity,
        "raw_physical_calls": [deepcopy(dict(row)) for row in candidate_contract],
        "excluded_raw_physical_calls": [
            deepcopy(dict(candidate_contract[ordinal]))
            for ordinal in selected_ordinals
        ],
        "expanded_invocation_leaves": expanded_leaves,
        "projected_contract": deepcopy(projected),
        "definition_symbols": sorted(helper_specs),
        "helper_graph": helper_receipts,
    }


def _zui_check_toggle_inline_helper_occurrence_projection(
    candidate_contract: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
) -> tuple[list[dict[str, Any]], dict[str, Any] | None]:
    """Expand the exact compiler-local zUI source-helper occurrences.

    Retail inlines four source-faithful helpers in
    ``HudUiCheckToggleWidget::LoadFromZrd``.  The governed VC5 candidate keeps
    sixteen physical helper calls: thirteen call-free accessor occurrences
    and three ``ApplyHudFontStyleTextOnly`` occurrences whose only invocation
    leaf is the panel's slot-0x80 virtual ``SetFont`` call.  Authenticate the
    complete caller and helper package, remove only those physical wrapper
    calls, and insert each proven virtual leaf at its original occurrence.
    """
    from _recoil.call_contract.records import CandidateAssembly

    result = [dict(row) for row in candidate_contract]
    profile = (
        caller_identity,
        normalize_address(caller_start),
        normalize_address(caller_end_exclusive),
    )
    if profile != (
        "symbol:recoil:function:0x4b7340",
        "0x4b7340",
        "0x4b7d60",
    ):
        return result, None

    prefix = "zUI check-toggle inline-helper occurrence graph"
    caller = candidate.caller_definition
    if caller is None:
        raise ValueError(f"{prefix} lacks the complete caller definition")
    if (
        caller.symbol
        != (
            "?LoadFromZrd@HudUiCheckToggleWidget@@UAEHPAUNode@zReader@@"
            "PAUHudUiBackground@@@Z"
        )
        or len(caller.data) != 0x9A0
        or len(caller.relocations) != 83
        or len(caller.relocation_mask) != len(caller.data)
        or any(
            caller.relocation_mask[index]
            != any(
                row.offset <= index < row.offset + 4
                for row in caller.relocations
            )
            for index in range(len(caller.data))
        )
    ):
        raise ValueError(
            f"{prefix} rejects caller body, relocation, or boundary drift"
        )

    helper_patterns = {
        "string": re.compile(
            r"^\?ZrdArrayString@\?%D:\\Recoil Project\\RecoilRebuild\\"
            r"src\\GameZRecoil\\zUI\\zui_widgets\.cpp(?P<d>[0-9]+)"
            r"@@YIPBDPAUNode@zReader@@H@Z$"
        ),
        "integer": re.compile(
            r"^\?ZrdArrayInt@\?%D:\\Recoil Project\\RecoilRebuild\\"
            r"src\\GameZRecoil\\zUI\\zui_widgets\.cpp(?P<d>[0-9]+)"
            r"@@YIHPAUNode@zReader@@HH@Z$"
        ),
        "owner_style": re.compile(
            r"^\?HudUiZrdOwnerFontStyle@\?%D:\\Recoil Project\\"
            r"RecoilRebuild\\src\\GameZRecoil\\zUI\\zui_widgets\.cpp"
            r"(?P<d>[0-9]+)@@YIPBUHudFontStyle@@"
            r"PBUHudUiBackground@@H@Z$"
        ),
        "apply_style": re.compile(
            r"^\?ApplyHudFontStyleTextOnly@\?%D:\\Recoil Project\\"
            r"RecoilRebuild\\src\\GameZRecoil\\zUI\\zui_widgets\.cpp"
            r"(?P<d>[0-9]+)@@YIXPAUHudUiPanel@@"
            r"PBUHudFontStyle@@@Z$"
        ),
    }
    definitions = candidate.tu_local_function_definitions
    matched: dict[str, tuple[str, CandidateTuLocalFunctionDefinition]] = {}
    discriminators: set[str] = set()
    for logical, pattern in helper_patterns.items():
        rows = [
            (name, definition, match)
            for name, definition in definitions.items()
            if (match := pattern.fullmatch(name)) is not None
        ]
        if len(rows) != 1:
            raise ValueError(
                f"{prefix} requires one exact {logical} definition"
            )
        name, definition, match = rows[0]
        matched[logical] = (name, definition)
        discriminators.add(match.group("d"))
    if len(discriminators) != 1:
        raise ValueError(f"{prefix} rejects mixed TU-local discriminators")

    helper_bodies = {
        "string": bytes.fromhex(
            "85c974058d04d1eb0233c085c0740983380375048b4004c333c0c3"
            "9090909090"
        ),
        "integer": bytes.fromhex(
            "85c974058d04d1eb0233c085c0740b83380175068b4004c20400"
            "8b442404c20400" + "90" * 15
        ),
        "owner_style": bytes.fromhex(
            "8d04d28d8c81ec1c00008b01f7d81bc023c1c3" + "90" * 13
        ),
        "apply_style": bytes.fromhex(
            "56578bfa8bf185ff74508b4f1c8b57088b066a026a006a006a0051"
            "8b4f0452518bceff90800000008b570c89964c0100008b470c898650"
            "010000b8010000008986700200008b4f18898e6402000089869c0200"
            "008986a00200005f5ec3" + "90" * 3
        ),
    }
    helper_receipts: list[dict[str, Any]] = []
    for logical, (name, definition) in matched.items():
        helper = CandidateAssembly(
            instructions=definition.instructions,
            local_control_flow_indices=definition.local_control_flow_indices,
            local_control_flow_targets=definition.local_control_flow_targets,
        )
        invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
            helper,
            caller_start="0x0",
            caller_end_exclusive=hex(len(definition.data)),
        )
        offsets = _cc_callable_identity._candidate_complete_instruction_offsets(helper)
        invocation_offsets = tuple(
            offsets[index] for index in invocation_indices
        )
        required_offsets = (0x22,) if logical == "apply_style" else ()
        if (
            definition.symbol != name
            or definition.data != helper_bodies[logical]
            or definition.section_size != len(helper_bodies[logical])
            or definition.section_is_comdat is not True
            or definition.comdat_selection != 2
            or definition.section_external_functions != (name,)
            or definition.relocations
            or any(definition.relocation_mask)
            or invocation_offsets != required_offsets
            or not definition.source_provenance.replace("\\", "/").endswith(
                "/src/GameZRecoil/zUI/zui_widgets.cpp"
            )
        ):
            raise ValueError(
                f"{prefix} rejects {logical} body, COMDAT, CFG, relocation, "
                "or provenance drift"
            )
        if logical == "apply_style" and (
            len(invocation_indices) != 1
            or bytes(
                int(item, 16)
                for item in definition.instructions[
                    invocation_indices[0]
                ].bytes
            )
            != bytes.fromhex("ff 90 80 00 00 00")
        ):
            raise ValueError(f"{prefix} rejects the exact SetFont leaf")
        helper_receipts.append({
            "logical_helper": logical,
            "symbol": name,
            "body_bytes_hex": _cc_reporting._direct_bytes(definition.data).hex(),
            "relocations": [],
            "invocation_offsets": list(invocation_offsets),
        })

    site_specs = (
        (0x117, "string"),
        (0x14C, "integer"),
        (0x163, "integer"),
        (0x17F, "integer"),
        (0x18C, "owner_style"),
        (0x195, "apply_style"),
        (0x29F, "integer"),
        (0x2B6, "integer"),
        (0x2D2, "integer"),
        (0x2F1, "apply_style"),
        (0x7CC, "string"),
        (0x804, "integer"),
        (0x81B, "integer"),
        (0x83B, "integer"),
        (0x848, "owner_style"),
        (0x851, "apply_style"),
    )
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start="0x0",
        caller_end_exclusive=hex(len(caller.data)),
    )
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    if len(invocation_indices) != len(result):
        raise ValueError(f"{prefix} rejects caller invocation population drift")
    ordinal_by_offset = {
        int(offsets[index]): ordinal
        for ordinal, index in enumerate(invocation_indices)
        if offsets[index] is not None
    }
    cleanup_by_logical = {
        "string": None,
        # The generic cleanup receipt intentionally does not publish
        # free-function ``YIH`` cleanup.  The exact helper body above still
        # authenticates both RET 4 terminals before this wrapper is removed.
        "integer": None,
        "owner_style": None,
        "apply_style": None,
    }
    selected: dict[int, str] = {}
    for offset, logical in site_specs:
        ordinal = ordinal_by_offset.get(offset)
        name = matched[logical][0]
        expected_row = {
            "ordinal": ordinal,
            "form": "call",
            "dispatch": "direct",
            "identity_kind": "direct",
            "target_identity": f"candidate-local-coff:{name}",
            "storage_identity": "",
            "slot_displacement": None,
            "cleanup_bytes": cleanup_by_logical[logical],
        }
        if ordinal is None or result[ordinal] != expected_row:
            raise ValueError(
                f"{prefix} rejects raw {logical} occurrence at +0x{offset:x}: "
                f"observed={result[ordinal] if ordinal is not None else None!r}, "
                f"required={expected_row!r}"
            )
        selected[ordinal] = logical
    observed_local_sites = tuple(
        (offset, logical)
        for offset, logical in site_specs
        if any(
            row.offset == offset + 1
            and row.type == IMAGE_REL_I386_REL32
            and row.symbol_name == matched[logical][0]
            for row in caller.relocations
        )
    )
    if observed_local_sites != site_specs:
        raise ValueError(f"{prefix} rejects helper relocation-site drift")

    apply_receiver_offsets = {
        0x195: 0x1A0,
        0x2F1: 0x2FC,
        0x851: 0x85C,
    }
    leaf_by_ordinal: dict[int, dict[str, Any]] = {}
    for apply_offset, receiver_offset in apply_receiver_offsets.items():
        apply_ordinal = ordinal_by_offset.get(apply_offset)
        receiver_ordinal = ordinal_by_offset.get(receiver_offset)
        receiver = (
            result[receiver_ordinal]
            if receiver_ordinal is not None
            else None
        )
        if (
            apply_ordinal is None
            or selected.get(apply_ordinal) != "apply_style"
            or receiver is None
            or receiver.get("form") != "call"
            or receiver.get("dispatch") != "indirect"
            or receiver.get("identity_kind") != "virtual-slot"
            or receiver.get("target_identity") != ""
            or receiver.get("slot_displacement") != 0x60
            or receiver.get("cleanup_bytes") is not None
            or not receiver.get("storage_identity")
        ):
            raise ValueError(
                f"{prefix} rejects receiver lineage at +0x{apply_offset:x}"
            )
        leaf_by_ordinal[apply_ordinal] = {
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": receiver["storage_identity"],
            "slot_displacement": 0x80,
            "cleanup_bytes": None,
        }

    projected: list[dict[str, Any]] = []
    expanded_leaves: list[dict[str, Any]] = []
    for ordinal, row in enumerate(result):
        if ordinal not in selected:
            projected_row = dict(row)
            projected_row["ordinal"] = len(projected)
            projected.append(projected_row)
            continue
        leaf = leaf_by_ordinal.get(ordinal)
        if leaf is not None:
            projected_row = {"ordinal": len(projected), **leaf}
            projected.append(projected_row)
            expanded_leaves.append(deepcopy(projected_row))
    return projected, {
        "kind": "candidate-local-helper-graph-expansion-receipt",
        "contract_version": 1,
        "candidate_expected_truth": False,
        "caller_identity": caller_identity,
        "raw_physical_calls": deepcopy(result),
        "excluded_raw_physical_calls": [
            deepcopy(result[ordinal]) for ordinal in sorted(selected)
        ],
        "expanded_invocation_leaves": expanded_leaves,
        "projected_contract": deepcopy(projected),
        "helper_graph": helper_receipts,
    }


def _zui_exact_stack_receiver_storage_projection(
    expected: Sequence[Mapping[str, Any]],
    candidate_contract: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
) -> list[dict[str, Any]]:
    """Normalize exact ephemeral panel locals across one VC5 frame delta.

    Retail reserves twelve more local-frame bytes than the governed candidate
    and reuses argument-home slots for the same eight newly allocated panel
    receivers.  The full caller body and relocation package below freezes the
    candidate proof.  Each accepted row must otherwise equal retail exactly;
    only the rendered ephemeral receiver storage may change.
    """

    result = [dict(row) for row in candidate_contract]
    profile = (
        caller_identity,
        normalize_address(caller_start),
        normalize_address(caller_end_exclusive),
    )
    if profile != (
        "symbol:recoil:function:0x4b59f0",
        "0x4b59f0",
        "0x4b6fc0",
    ):
        return result
    prefix = "zUI exact stack-receiver storage projection"
    caller = candidate.caller_definition
    if caller is None:
        raise ValueError(f"{prefix} lacks the complete caller definition")
    if (
        caller.symbol
        != (
            "?LoadFromZrd@HudUiZrdWidget@@UAEHPAUNode@zReader@@"
            "PAUHudUiBackground@@@Z"
        )
        or len(caller.data) != 0x15E0
        or len(caller.relocations) != 150
        or len(caller.relocation_mask) != len(caller.data)
    ):
        raise ValueError(
            f"{prefix} rejects caller body, relocation, boundary, or "
            "contract-population drift: "
            f"symbol={caller.symbol!r}, extent={len(caller.data)}, "
            f"relocation_count={len(caller.relocations)}, "
            f"mask_count={len(caller.relocation_mask)}, "
            f"retail_count={len(expected)}, candidate_count={len(result)}"
        )
    allowed_candidate_storage = {
        "load(load(stack+0x3c))",
        "load(load(stack+0x44))",
        "load(nullable(call-result(provider:recoil:function:0x4c5b76)))",
    }
    allowed_retail_storage = {
        "load(load(stack+0x48))",
        "load(load(stack+0x50))",
    }
    bridged_ordinals: list[int] = []
    for ordinal, (retail_row, candidate_row) in enumerate(
        zip(expected, result)
    ):
        retail = dict(retail_row)
        observed = dict(candidate_row)
        retail_storage = retail.get("storage_identity")
        candidate_storage = observed.get("storage_identity")
        if (
            retail_storage == candidate_storage
            or candidate_storage not in allowed_candidate_storage
            or retail_storage not in allowed_retail_storage
            or retail.get("dispatch") != "indirect"
            or retail.get("identity_kind") != "virtual-slot"
            or observed.get("dispatch") != "indirect"
            or observed.get("identity_kind") != "virtual-slot"
        ):
            continue
        candidate_without_storage = dict(observed)
        retail_without_storage = dict(retail)
        candidate_without_storage["storage_identity"] = ""
        retail_without_storage["storage_identity"] = ""
        if candidate_without_storage != retail_without_storage:
            continue
        observed["storage_identity"] = str(retail_storage)
        result[ordinal] = observed
        bridged_ordinals.append(ordinal)
    required_ordinals = (
        17, 18, 19, 20,
        26, 27, 28, 29,
        45, 46, 47, 48,
        54, 55, 56, 57,
        73, 74, 75, 76,
        82, 83, 84, 85,
        96, 97, 98, 99,
        105, 106, 107, 108,
    )
    if tuple(bridged_ordinals) != required_ordinals:
        raise ValueError(
            f"{prefix} rejects exact bridge population drift: "
            f"observed={tuple(bridged_ordinals)!r}"
        )
    return result


def _zui_r4905_exact_receiver_rendering_projection(
    expected: Sequence[Mapping[str, Any]],
    candidate_contract: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> list[dict[str, Any]]:
    """Normalize the remaining address-valued UI receiver rendering.

    Complete-body lineage proof handles the message-box callbacks directly;
    obsolete spelling profiles must not override that independently proved
    lineage.
    """

    result = [dict(row) for row in candidate_contract]
    start = normalize_address(caller_start)
    cases: Mapping[str, tuple[int, str, str]] = {
        "0x4b8de0": (
            2,
            "load(address(this+0x16c))",
            "load(this+0x16c)",
        ),
    }
    case = cases.get(start)
    if case is None:
        return result
    profile = _cc_catalog._ZUI_RECEIVER_RENDERING_CANDIDATE_PROFILES.get(start)
    if profile is None:
        raise ValueError(
            "zUI exact receiver-rendering projection lacks its caller profile"
        )
    caller = candidate.caller_definition
    relocation_payload = (
        _zui_relocation_payload(caller.relocations)
        if caller is not None
        else b""
    )
    if (
        caller_identity != indexes.by_address.get(start)
        or caller_identity in indexes.provider_ids
        or normalize_address(caller_end_exclusive) != profile["end"]
        or caller is None
        or caller.symbol != profile["symbol"]
        or len(caller.data) != profile["size"]
        or len(caller.relocations) != profile["relocation_count"]
        or len(caller.relocation_mask) != len(caller.data)
        or len(expected) != len(result)
    ):
        raise ValueError(
            "zUI exact receiver-rendering projection rejects caller, "
            "relocation, boundary, identity, or population drift"
        )
    ordinal, candidate_storage, retail_storage = case
    if ordinal >= len(result):
        raise ValueError(
            "zUI exact receiver-rendering projection lacks its reviewed row"
        )
    retail_row = dict(expected[ordinal])
    candidate_row = dict(result[ordinal])
    if (
        candidate_row.get("storage_identity") != candidate_storage
        or retail_row.get("storage_identity") != retail_storage
    ):
        raise ValueError(
            "zUI exact receiver-rendering projection rejects storage drift"
        )
    candidate_without_storage = dict(candidate_row)
    retail_without_storage = dict(retail_row)
    candidate_without_storage["storage_identity"] = ""
    retail_without_storage["storage_identity"] = ""
    if candidate_without_storage != retail_without_storage:
        raise ValueError(
            "zUI exact receiver-rendering projection rejects semantic row drift"
        )
    candidate_row["storage_identity"] = retail_storage
    result[ordinal] = candidate_row
    return result


def _zui_check_toggle_receiver_storage_projection(
    expected: Sequence[Mapping[str, Any]],
    candidate_contract: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
) -> list[dict[str, Any]]:
    """Normalize exact panel receivers in one frozen check-toggle caller.

    The governed candidate configures two conditional transition-text panels
    through an allocation-result register and stores each same nonvolatile
    receiver into ``this+0x160`` after ``AddChild``.  Retail performs the
    accepted owner-field contribution early enough that the configuration
    vcalls are rendered through the member.  Normalize only those six storage
    spellings after authenticating the complete candidate body/relocations and
    both exact allocation, receiver, virtual-call, AddChild, and member-store
    lineages.  The second allocation retains the separately required inline
    constructor as one exact VC5 COMDAT wrapper; expand its one authenticated
    base-constructor leaf before comparing retail.  The same caller has two
    loop-local panels whose candidate frame is four bytes larger than retail;
    normalize their exact eight vcalls only after the same complete caller and
    relocation proof.
    """
    from _recoil.call_contract.records import CandidateAssembly

    result = [dict(row) for row in candidate_contract]
    profile = (
        caller_identity,
        normalize_address(caller_start),
        normalize_address(caller_end_exclusive),
    )
    if profile != (
        "symbol:recoil:function:0x4b7340",
        "0x4b7340",
        "0x4b7d60",
    ):
        return result
    prefix = "zUI check-toggle receiver-storage projection"
    caller = candidate.caller_definition
    if caller is None:
        raise ValueError(f"{prefix} lacks the complete caller definition")
    relocation_payload = _zui_relocation_payload(caller.relocations)
    if (
        caller.symbol
        != (
            "?LoadFromZrd@HudUiCheckToggleWidget@@UAEHPAUNode@zReader@@"
            "PAUHudUiBackground@@@Z"
        )
        or len(caller.data) != 0x9A0
        or len(caller.relocations) != 83
        or len(caller.relocation_mask) != len(caller.data)
        or any(
            caller.relocation_mask[index]
            != any(
                row.offset <= index < row.offset + 4
                for row in caller.relocations
            )
            for index in range(len(caller.data))
        )
    ):
        raise ValueError(f"{prefix} rejects caller artifact drift")

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    index_by_offset = {
        int(offset): index
        for index, offset in enumerate(offsets)
        if offset is not None
    }
    exact_rows = {
        0x025: (bytes.fromhex("8b f9"), "edi, ecx"),
        0x0C4: (bytes.fromhex("e8 00 00 00 00"), "??2@YAPAXI@Z"),
        0x0C9: (bytes.fromhex("8b f0"), "esi, eax"),
        0x0D0: (bytes.fromhex("3b f5"), "esi, ebp"),
        0x0D2: (bytes.fromhex("74 3c"), "$L<private>"),
        0x0D7: (bytes.fromhex("8b ce"), "ecx, esi"),
        0x0D9: (
            bytes.fromhex("e8 00 00 00 00"),
            "??0HudUiPanel@@QAE@PBDHH@Z",
        ),
        0x137: (bytes.fromhex("8b 16"), "edx, dword [esi]"),
        0x13B: (bytes.fromhex("ff 52 74"), "dword [edx+116]"),
        0x13E: (bytes.fromhex("8b 2e"), "ebp, dword [esi]"),
        0x16E: (bytes.fromhex("8b ce"), "ecx, esi"),
        0x173: (bytes.fromhex("ff 55 0c"), "dword [ebp+12]"),
        0x19A: (bytes.fromhex("8b 06"), "eax, dword [esi]"),
        0x19E: (bytes.fromhex("8b ce"), "ecx, esi"),
        0x1A0: (bytes.fromhex("ff 50 60"), "dword [eax+96]"),
        0x1A9: (bytes.fromhex("56"), "esi"),
        0x1AA: (
            bytes.fromhex("e8 00 00 00 00"),
            "?AddChild@HudUiContainer@@QAEHPAUHudUiElement@@@Z",
        ),
        0x1B3: (
            bytes.fromhex("89 b7 60 01 00 00"),
            "dword [edi+352], esi",
        ),
        0x247: (bytes.fromhex("e8 00 00 00 00"), "??2@YAPAXI@Z"),
        0x24C: (bytes.fromhex("8b f0"), "esi, eax"),
        0x251: (bytes.fromhex("85 f6"), "esi, esi"),
        0x253: (bytes.fromhex("74 07"), "$L<private>"),
        0x255: (bytes.fromhex("8b ce"), "ecx, esi"),
        0x257: (
            bytes.fromhex("e8 00 00 00 00"),
            "??0HudUiTransitionTextPanel@@QAE@XZ",
        ),
        0x28A: (bytes.fromhex("8b 0e"), "ecx, dword [esi]"),
        0x28E: (bytes.fromhex("ff 51 74"), "dword [ecx+116]"),
        0x291: (bytes.fromhex("8b 2e"), "ebp, dword [esi]"),
        0x2C1: (bytes.fromhex("8b ce"), "ecx, esi"),
        0x2C6: (bytes.fromhex("ff 55 0c"), "dword [ebp+12]"),
        0x2F6: (bytes.fromhex("8b 16"), "edx, dword [esi]"),
        0x2FA: (bytes.fromhex("8b ce"), "ecx, esi"),
        0x2FC: (bytes.fromhex("ff 52 60"), "dword [edx+96]"),
        0x305: (bytes.fromhex("56"), "esi"),
        0x306: (
            bytes.fromhex("e8 00 00 00 00"),
            "?AddChild@HudUiContainer@@QAEHPAUHudUiElement@@@Z",
        ),
        0x30F: (
            bytes.fromhex("89 b7 60 01 00 00"),
            "dword [edi+352], esi",
        ),
        0x77B: (bytes.fromhex("e8 00 00 00 00"), "??2@YAPAXI@Z"),
        0x780: (bytes.fromhex("8b f0"), "esi, eax"),
        0x785: (bytes.fromhex("3b f5"), "esi, ebp"),
        0x787: (bytes.fromhex("74 3c"), "$L<private>"),
        0x78C: (bytes.fromhex("8b ce"), "ecx, esi"),
        0x78E: (
            bytes.fromhex("e8 00 00 00 00"),
            "??0HudUiPanel@@QAE@PBDHH@Z",
        ),
        0x7EC: (bytes.fromhex("8b 16"), "edx, dword [esi]"),
        0x7F0: (bytes.fromhex("ff 52 74"), "dword [edx+116]"),
        0x7F3: (bytes.fromhex("8b 06"), "eax, dword [esi]"),
        0x800: (bytes.fromhex("89 44 24 38"), "dword 8+[esp+48], eax"),
        0x828: (bytes.fromhex("8b ce"), "ecx, esi"),
        0x82B: (bytes.fromhex("8b 44 24 3c"), "eax, dword 8+[esp+52]"),
        0x82F: (bytes.fromhex("ff 50 0c"), "dword [eax+12]"),
        0x856: (bytes.fromhex("8b 16"), "edx, dword [esi]"),
        0x85A: (bytes.fromhex("8b ce"), "ecx, esi"),
        0x85C: (bytes.fromhex("ff 52 60"), "dword [edx+96]"),
        0x865: (bytes.fromhex("56"), "esi"),
        0x866: (
            bytes.fromhex("e8 00 00 00 00"),
            "?AddChild@HudUiContainer@@QAEHPAUHudUiElement@@@Z",
        ),
        0x86B: (
            bytes.fromhex("89 b7 60 01 00 00"),
            "dword [edi+352], esi",
        ),
    }
    for offset, (body, operand) in exact_rows.items():
        index = index_by_offset.get(offset)
        try:
            observed_body = (
                bytes(int(item, 16) for item in candidate.instructions[index].bytes)
                if index is not None
                else b""
            )
        except (TypeError, ValueError):
            observed_body = b""
        observed_operand = (
            _cc_cfg._instruction_operand(candidate.instructions[index]).strip()
            if index is not None else ""
        )
        operand_matches = (
            observed_operand == operand
            or (
                operand == "$L<private>"
                and re.fullmatch(r"\$L[0-9]+", observed_operand) is not None
            )
        )
        if (
            observed_body != body
            or index is None
            or not operand_matches
        ):
            raise ValueError(
                f"{prefix} rejects receiver lineage at +0x{offset:x}"
            )
    required_relocations = {
        0x0C5: "??2@YAPAXI@Z",
        0x0DA: "??0HudUiPanel@@QAE@PBDHH@Z",
        0x1AB: "?AddChild@HudUiContainer@@QAEHPAUHudUiElement@@@Z",
        0x248: "??2@YAPAXI@Z",
        0x258: "??0HudUiTransitionTextPanel@@QAE@XZ",
        0x307: "?AddChild@HudUiContainer@@QAEHPAUHudUiElement@@@Z",
        0x77C: "??2@YAPAXI@Z",
        0x78F: "??0HudUiPanel@@QAE@PBDHH@Z",
        0x867: "?AddChild@HudUiContainer@@QAEHPAUHudUiElement@@@Z",
    }
    if any(
        len(
            [
                row for row in caller.relocations
                if row.offset == offset
                and row.type == IMAGE_REL_I386_REL32
                and row.symbol_name == name
            ]
        )
        != 1
        for offset, name in required_relocations.items()
    ):
        raise ValueError(f"{prefix} rejects direct-call relocation lineage")

    if (
        len(expected) <= 18
        or len(result) <= 18
        or tuple(row.get("ordinal") for row in expected)
        != tuple(range(len(expected)))
        or tuple(row.get("ordinal") for row in result)
        != tuple(range(len(result)))
    ):
        raise ValueError(f"{prefix} rejects contract population drift")
    transition_constructor = "??0HudUiTransitionTextPanel@@QAE@XZ"
    helper = candidate.tu_local_function_definitions.get(
        transition_constructor
    )
    helper_candidate = (
        CandidateAssembly(
            instructions=helper.instructions,
            local_control_flow_indices=helper.local_control_flow_indices,
            local_control_flow_targets=helper.local_control_flow_targets,
        )
        if helper is not None else None
    )
    helper_invocations = (
        _cc_callable_identity._candidate_static_invocation_indices(
            helper_candidate,
            caller_start="0x0",
            caller_end_exclusive="0x50",
        )
        if helper_candidate is not None else ()
    )
    helper_offsets = (
        _cc_callable_identity._candidate_complete_instruction_offsets(helper_candidate)
        if helper_candidate is not None else ()
    )
    expected_constructor_leaf = {
        "ordinal": 18,
        "form": "call",
        "dispatch": "direct",
        "identity_kind": "direct",
        "target_identity": "symbol:recoil:function:0x4ba740",
        "storage_identity": "",
        "slot_displacement": None,
        "cleanup_bytes": None,
    }
    candidate_constructor_wrapper = {
        **expected_constructor_leaf,
        "target_identity": "symbol:recoil:function:0x4ba020",
    }
    if (
        dict(expected[18]) != expected_constructor_leaf
        or dict(result[18]) != candidate_constructor_wrapper
        or helper is None
        or helper.symbol != transition_constructor
        or len(helper.data) != 0x50
        or helper.section_size != 0x50
        or not helper.section_is_comdat
        or helper.comdat_selection != 2
        or helper.section_external_functions != (transition_constructor,)
        or tuple(
            (row.offset, row.type, row.symbol_name)
            for row in helper.relocations
        ) != (
            (0x0A, IMAGE_REL_I386_REL32, "??0HudUiPanel@@QAE@PBDHH@Z"),
            (0x28, IMAGE_REL_I386_DIR32, "??_7HudUiTransitionTextPanel@@6B@"),
        )
        or len(helper.relocation_mask) != len(helper.data)
        or any(
            helper.relocation_mask[index]
            != (index in range(0x0A, 0x0E) or index in range(0x28, 0x2C))
            for index in range(len(helper.data))
        )
        or len(helper_invocations) != 1
        or helper_offsets[helper_invocations[0]] != 0x09
        or not helper.source_provenance.replace("\\", "/").endswith(
            "/src/GameZRecoil/zHud/zhud_ui_defs.h"
        )
    ):
        raise ValueError(
            f"{prefix} rejects exact transition-panel constructor leaf"
        )
    result[18] = dict(expected_constructor_leaf)
    allocation_storage = (
        "load(nullable(call-result(provider:recoil:function:0x4c5b76)))"
    )
    member_storage = "load(exact-receiver-field(this,+0x160))"
    bridged: list[int] = []
    bridge_specs = (
        (9, 0x0C),
        (10, 0x80),
        (11, 0x60),
        (21, 0x0C),
        (22, 0x80),
        (23, 0x60),
        (52, 0x0C),
        (53, 0x80),
        (54, 0x60),
    )
    for ordinal, slot in bridge_specs:
        retail = dict(expected[ordinal])
        observed = dict(result[ordinal])
        if (
            retail.get("form") != "call"
            or retail.get("dispatch") != "indirect"
            or retail.get("identity_kind") != "virtual-slot"
            or retail.get("target_identity") != ""
            or retail.get("storage_identity") != member_storage
            or retail.get("slot_displacement") != slot
            or retail.get("cleanup_bytes") is not None
            or observed.get("storage_identity")
            != ("load(this)" if ordinal == 52 else allocation_storage)
        ):
            raise ValueError(
                f"{prefix} rejects exact bridge row at ordinal {ordinal}"
            )
        observed["storage_identity"] = member_storage
        if observed != retail:
            raise ValueError(
                f"{prefix} rejects non-storage drift at ordinal {ordinal}"
            )
        result[ordinal] = observed
        bridged.append(ordinal)
    if tuple(bridged) != tuple(ordinal for ordinal, _slot in bridge_specs):
        raise ValueError(f"{prefix} rejects bridge population drift")

    stack_specs = (
        (29, 0x74, "load(load(stack+0x34))", "load(load(stack+0x38))", 8),
        (30, 0x0C, "load(load(stack+0x34))", allocation_storage, None),
        (31, 0x80, "load(load(stack+0x3c))", "load(load(stack+0x40))", None),
        (32, 0x60, "load(load(stack+0x34))", allocation_storage, None),
        (38, 0x74, "load(load(stack+0x34))", "load(load(stack+0x38))", 8),
        (39, 0x0C, "load(load(stack+0x3c))", allocation_storage, None),
        (40, 0x80, "load(load(stack+0x3c))", "load(load(stack+0x40))", None),
        (41, 0x60, "load(load(stack+0x34))", allocation_storage, None),
    )
    stack_bridged: list[int] = []
    for ordinal, slot, retail_storage, candidate_storage, cleanup in stack_specs:
        retail = dict(expected[ordinal])
        observed = dict(result[ordinal])
        if (
            retail.get("form") != "call"
            or retail.get("dispatch") != "indirect"
            or retail.get("identity_kind") != "virtual-slot"
            or retail.get("target_identity") != ""
            or retail.get("storage_identity") != retail_storage
            or retail.get("slot_displacement") != slot
            or retail.get("cleanup_bytes") != cleanup
            or observed.get("storage_identity") != candidate_storage
        ):
            raise ValueError(
                f"{prefix} rejects exact stack row at ordinal {ordinal}: "
                f"retail={retail!r}, candidate={observed!r}, "
                f"required_candidate_storage={candidate_storage!r}"
            )
        observed["storage_identity"] = retail_storage
        if observed != retail:
            raise ValueError(
                f"{prefix} rejects non-storage stack drift at ordinal {ordinal}"
            )
        result[ordinal] = observed
        stack_bridged.append(ordinal)
    if tuple(stack_bridged) != tuple(
        ordinal for ordinal, *_rest in stack_specs
    ):
        raise ValueError(f"{prefix} rejects stack bridge population drift")
    return result
