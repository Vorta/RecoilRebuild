"""Recoil call-contract receiver equivalence evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import errors as _cc_errors
from _recoil.call_contract import recoil_ui as _cc_recoil_ui
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        CandidateCallerDefinition,
        IdentityIndexes,
        ReviewedLoopVptrStorageBridge,
    )

import re
from collections import Counter
from pathlib import Path
from typing import Any, Mapping, NoReturn, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_DIR32,
    IMAGE_REL_I386_REL32,
    Instruction,
)
from _recoil.lib.pe import parse_pe_headers, rva_to_offset
from _recoil.lib.progress import address_value, normalize_address
from _recoil.lib.windows_identity import StableReadHandle


def _zvid_dd_candidate_com_vptr_storage_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Prove the finite VC5 candidate counterpart of zVid's DD caller."""
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge

    expected_identity = "symbol:recoil:function:0x4a8220"
    candidate_symbol = (
        "?ClearScreenAndZBufferRect@zVideo_dd@@YIHPAUzVidRect32@@"
        "PAUzVideo_SurfaceStatePartial@@@Z"
    )
    start = address_value(caller_start)
    if caller_identity != expected_identity and start != 0x4A8220:
        return {}

    label = "zVid DD candidate COM-vtable lineage"

    def reject(detail: str) -> NoReturn:
        raise ValueError(f"{label} rejects {detail}")

    if (
        caller_identity != expected_identity
        or start != 0x4A8220
        or address_value(caller_end_exclusive) != 0x4A82F0
        or indexes.by_address.get("0x4a8220") != expected_identity
        or expected_identity in indexes.provider_ids
    ):
        reject("the exact authored caller identity/address/extent")
    global_identity = indexes.storage_by_address.get("0x6333f4", "")
    if (
        not global_identity.startswith("storage:recoil:data:")
        or global_identity.startswith("iat:")
        or global_identity in indexes.provider_ids
    ):
        reject("one exact non-provider, non-IAT global 0x6333f4 identity")
    member_identity = "load(load(entry-register(edx)+0x1c))"
    direct_identity = indexes.by_address.get("0x4ad6a0", "")
    exact_expected = [
        {
            "ordinal": 0, "form": "call", "dispatch": "indirect",
            "identity_kind": "virtual-slot", "target_identity": "",
            "storage_identity": member_identity,
            "slot_displacement": 0x14, "cleanup_bytes": None,
        },
        {
            "ordinal": 1, "form": "call", "dispatch": "indirect",
            "identity_kind": "virtual-slot", "target_identity": "",
            "storage_identity": member_identity,
            "slot_displacement": 0x6C, "cleanup_bytes": None,
        },
        {
            "ordinal": 2, "form": "call", "dispatch": "direct",
            "identity_kind": "direct", "target_identity": direct_identity,
            "storage_identity": "", "slot_displacement": None,
            "cleanup_bytes": None,
        },
        {
            "ordinal": 3, "form": "call", "dispatch": "indirect",
            "identity_kind": "virtual-slot", "target_identity": "",
            "storage_identity": f"load({global_identity})",
            "slot_displacement": 0x14, "cleanup_bytes": None,
        },
        {
            "ordinal": 4, "form": "call", "dispatch": "indirect",
            "identity_kind": "virtual-slot", "target_identity": "",
            "storage_identity": f"load({global_identity})",
            "slot_displacement": 0x6C, "cleanup_bytes": None,
        },
        {
            "ordinal": 5, "form": "call", "dispatch": "direct",
            "identity_kind": "direct", "target_identity": direct_identity,
            "storage_identity": "", "slot_displacement": None,
            "cleanup_bytes": None,
        },
    ]
    if not direct_identity or list(expected) != exact_expected:
        reject("the complete independently retail-derived six-call contract")

    definition = candidate.caller_definition
    expected_relocations = (
        (0x04, IMAGE_REL_I386_DIR32, "_g_zVideo_ClearScreenBufferEnabled"),
        (0x1B, IMAGE_REL_I386_DIR32, "_g_zVideo_ClearColorPacked16"),
        (0x59, IMAGE_REL_I386_DIR32, "_g_zVideo_SourceFile_ZvidDdC"),
        (0x60, IMAGE_REL_I386_REL32, "?ReportError@zVideo_dd@@YIHHPBDH@Z"),
        (0x6B, IMAGE_REL_I386_DIR32, "_g_zVideo_pZBufferSurface"),
        (0x84, IMAGE_REL_I386_DIR32, "_g_zVideo_pZBufferSurface"),
        (0xA9, IMAGE_REL_I386_DIR32, "_g_zVideo_pZBufferSurface"),
        (0xBD, IMAGE_REL_I386_DIR32, "_g_zVideo_SourceFile_ZvidDdC"),
        (0xC4, IMAGE_REL_I386_REL32, "?ReportError@zVideo_dd@@YIHHPBDH@Z"),
    )
    if (
        definition is None
        or definition.symbol != candidate_symbol
        or definition.section_index != 34
        or definition.section_start != 0
        or definition.section_end != 0xE0
        or len(definition.data) != 0xE0
        or len(definition.relocation_mask) != 0xE0
        or tuple(
            (row.offset, row.type, row.symbol_name)
            for row in definition.relocations
        ) != expected_relocations
        or any(
            definition.data[offset:offset + 4] != b"\0" * 4
            or not all(definition.relocation_mask[offset:offset + 4])
            for offset, _kind, _symbol in expected_relocations
        )
        or any(
            definition.relocation_mask[index]
            and not any(offset <= index < offset + 4
                        for offset, _kind, _symbol in expected_relocations)
            for index in range(0xE0)
        )
    ):
        reject("the exact current section-34/0xe0 COFF relocation package")

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    by_offset: dict[int, tuple[int, Instruction]] = {}
    duplicates: set[int] = set()
    for index, (offset, instruction) in enumerate(
        zip(offsets, candidate.instructions)
    ):
        if offset is None:
            continue
        if offset in by_offset:
            duplicates.add(offset)
        by_offset[offset] = (index, instruction)
    exact_rows = {
        0x0A: (bytes.fromhex("8b f2"), r"mov\s+esi\s*,\s*edx"),
        0x23: (bytes.fromhex("8b 46 1c"), r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[esi(?:\+28|\+0x1c)\]"),
        0x2A: (bytes.fromhex("52"), r"push\s+edx"),
        0x2B: (bytes.fromhex("68 00 04 00 01"), r"push\s+(?:16778240|0x1000400)"),
        0x30: (bytes.fromhex("8b 08"), r"mov\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[eax\]"),
        0x32: (bytes.fromhex("6a 00"), r"push\s+(?:0|0x0)"),
        0x34: (bytes.fromhex("6a 00"), r"push\s+(?:0|0x0)"),
        0x36: (bytes.fromhex("57"), r"push\s+edi"),
        0x37: (bytes.fromhex("50"), r"push\s+eax"),
        0x38: (bytes.fromhex("ff 51 14"), r"call\s+(?:dword\s+(?:ptr\s+)?)?\[ecx(?:\+20|\+0x14)\]"),
        0x46: (bytes.fromhex("8b 46 1c"), r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[esi(?:\+28|\+0x1c)\]"),
        0x49: (bytes.fromhex("50"), r"push\s+eax"),
        0x4A: (bytes.fromhex("8b 08"), r"mov\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[eax\]"),
        0x4C: (bytes.fromhex("ff 51 6c"), r"call\s+(?:dword\s+(?:ptr\s+)?)?\[ecx(?:\+108|\+0x6c)\]"),
        0x53: (bytes.fromhex("68 67 02 00 00"), r"push\s+(?:615|0x267)"),
        0x5D: (bytes.fromhex("8b c8"), r"mov\s+ecx\s*,\s*eax"),
        0x5F: (bytes.fromhex("e8 00 00 00 00"), r"call\s+\?ReportError@zVideo_dd@@YIHHPBDH@Z(?:\s*;.*)?"),
        0x6A: (bytes.fromhex("a1 00 00 00 00"), r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?_g_zVideo_pZBufferSurface"),
        0x81: (bytes.fromhex("eb 05"), r"jmp\s+(?:short\s+)?\$L[0-9]+"),
        0x83: (bytes.fromhex("a1 00 00 00 00"), r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?_g_zVideo_pZBufferSurface"),
        0x88: (bytes.fromhex("8b 10"), r"mov\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[eax\]"),
        0x8E: (bytes.fromhex("51"), r"push\s+ecx"),
        0x8F: (bytes.fromhex("68 00 00 00 02"), r"push\s+(?:33554432|0x2000000)"),
        0x94: (bytes.fromhex("6a 00"), r"push\s+(?:0|0x0)"),
        0x96: (bytes.fromhex("6a 00"), r"push\s+(?:0|0x0)"),
        0x98: (bytes.fromhex("57"), r"push\s+edi"),
        0x99: (bytes.fromhex("50"), r"push\s+eax"),
        0x9A: (bytes.fromhex("ff 52 14"), r"call\s+(?:dword\s+(?:ptr\s+)?)?\[edx(?:\+20|\+0x14)\]"),
        0xA8: (bytes.fromhex("a1 00 00 00 00"), r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?_g_zVideo_pZBufferSurface"),
        0xAD: (bytes.fromhex("50"), r"push\s+eax"),
        0xAE: (bytes.fromhex("8b 10"), r"mov\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[eax\]"),
        0xB0: (bytes.fromhex("ff 52 6c"), r"call\s+(?:dword\s+(?:ptr\s+)?)?\[edx(?:\+108|\+0x6c)\]"),
        0xB7: (bytes.fromhex("68 7f 02 00 00"), r"push\s+(?:639|0x27f)"),
        0xC1: (bytes.fromhex("8b c8"), r"mov\s+ecx\s*,\s*eax"),
        0xC3: (bytes.fromhex("e8 00 00 00 00"), r"call\s+\?ReportError@zVideo_dd@@YIHHPBDH@Z(?:\s*;.*)?"),
    }
    if duplicates & exact_rows.keys() or not exact_rows.keys() <= by_offset.keys():
        reject("unique exact candidate instruction coordinates")
    for offset, (body, pattern) in exact_rows.items():
        instruction = by_offset[offset][1]
        if (
            bytes(int(item, 16) for item in instruction.bytes) != body
            or definition.data[offset:offset + len(body)] != body
            or re.fullmatch(pattern, instruction.raw_text.strip(), re.IGNORECASE)
            is None
        ):
            reject(f"exact candidate bytes/operands at +0x{offset:x}")

    invocation_offsets = tuple(
        offsets[index]
        for index in _cc_callable_identity._candidate_static_invocation_indices(
            candidate,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
        )
    )
    if invocation_offsets != (0x38, 0x4C, 0x5F, 0x9A, 0xB0, 0xC3):
        reject("the complete exact six-call candidate population")

    instruction_addresses = tuple(
        offset if offset is not None else None for offset in offsets
    )
    index_by_address = {
        offset: index for index, offset in enumerate(offsets)
        if offset is not None
    }
    coverage_specs = (
        ((0x6A, 0x83), 0x88, "eax", "the two-definition depth-fill A1 join"),
        ((0xA8,), 0xAE, "eax", "the singleton Restore A1 lineage"),
        ((0x88,), 0x9A, "edx", "the depth-fill vptr lineage"),
        ((0xAE,), 0xB0, "edx", "the Restore vptr lineage"),
    )
    for definitions, transfer, register, detail in coverage_specs:
        if not _cc_cfg._exact_register_definition_set_covers_transfer(
            candidate.instructions,
            instruction_addresses=instruction_addresses,
            instruction_index_by_address=index_by_address,
            definition_indices=frozenset(index_by_address[item] for item in definitions),
            transfer_index=index_by_address[transfer],
            register=register,
            source="cod",
            caller_start=0,
            caller_end=0xE0,
            local_control_flow_indices=candidate.local_control_flow_indices,
            local_control_flow_targets=dict(candidate.local_control_flow_targets),
        ):
            reject(detail)

    return {
        "0x38": ReviewedLoopVptrStorageBridge(
            register="ecx", storage_identity=member_identity,
            slot_displacement=0x14, assembly_source="cod",
        ),
        "0x4c": ReviewedLoopVptrStorageBridge(
            register="ecx", storage_identity=member_identity,
            slot_displacement=0x6C, assembly_source="cod",
        ),
        "0x9a": ReviewedLoopVptrStorageBridge(
            register="edx", storage_identity=f"load({global_identity})",
            slot_displacement=0x14, assembly_source="cod",
        ),
        "0xb0": ReviewedLoopVptrStorageBridge(
            register="edx", storage_identity=f"load({global_identity})",
            slot_displacement=0x6C, assembly_source="cod",
        ),
    }


def _r4578_appframe_state_queue_vptr_proofs(
    instructions: Sequence[Instruction],
    *,
    caller_start: int,
    caller_end: int,
    indexes: IdentityIndexes,
) -> dict[int, str]:
    """Prove the three exact RecoilApp queued-state receiver lineages."""
    from _recoil.call_contract.records import CandidateAssembly

    profiles: Mapping[int, Mapping[str, Any]] = {
        0x443160: {
            "end": 0x443310,
            "calls": (0x09, 0x16, 0x75, 0x96, 0x11D, 0x141, 0x185, 0x18E),
            "rows": {
                0x00: bytes.fromhex("83 ec 14"),
                0x03: bytes.fromhex("53"),
                0x04: bytes.fromhex("55"),
                0x05: bytes.fromhex("56"),
                0x06: bytes.fromhex("57"),
                0x07: bytes.fromhex("8b f1"),
                0x09: bytes.fromhex("e8 00 00 00 00"),
                0x0E: bytes.fromhex("8b d8"),
                0x17A: bytes.fromhex("85 db"),
                0x17F: bytes.fromhex("74 07"),
                0x181: bytes.fromhex("8b 03"),
                0x183: bytes.fromhex("8b cb"),
                0x185: bytes.fromhex("ff 50 14"),
                0x188: bytes.fromhex("8b 4c 24 28"),
                0x18C: bytes.fromhex("8b 11"),
                0x18E: bytes.fromhex("ff 52 08"),
                0x191: bytes.fromhex("5f"),
                0x192: bytes.fromhex("5e"),
                0x193: bytes.fromhex("8b c3"),
                0x195: bytes.fromhex("5d"),
                0x196: bytes.fromhex("5b"),
                0x197: bytes.fromhex("83 c4 14"),
                0x19A: bytes.fromhex("c2 08 00"),
            },
            "sites": {
                0x185: "load(call-result(symbol:recoil:function:0x443140))",
                0x18E: "bounded-stack-receiver-vptr",
            },
        },
        0x443310: {
            "end": 0x4434B0,
            "calls": (0x09, 0x14, 0x74, 0x91, 0x118, 0x13C, 0x17B),
            "rows": {
                0x09: bytes.fromhex("e8 00 00 00 00"),
                0x10: bytes.fromhex("89 44 24 14"),
                0x171: bytes.fromhex("8b 4c 24 28"),
                0x179: bytes.fromhex("8b 01"),
                0x17B: bytes.fromhex("ff 50 08"),
            },
            "sites": {0x17B: "bounded-stack-receiver-vptr"},
        },
        0x4434B0: {
            "end": 0x443650,
            "calls": (0x09, 0x16, 0x6B, 0x8C, 0x110, 0x134, 0x178),
            "rows": {
                0x09: bytes.fromhex("e8 00 00 00 00"),
                0x0E: bytes.fromhex("8b d8"),
                0x16D: bytes.fromhex("85 db"),
                0x172: bytes.fromhex("74 07"),
                0x174: bytes.fromhex("8b 03"),
                0x176: bytes.fromhex("8b cb"),
                0x178: bytes.fromhex("ff 50 14"),
            },
            "sites": {
                0x178: "load(call-result(symbol:recoil:function:0x443140))",
            },
        },
    }
    spec = profiles.get(caller_start)
    if spec is None:
        return {}
    caller_identity = f"symbol:recoil:function:0x{caller_start:x}"
    current_state_identity = "symbol:recoil:function:0x443140"
    if (
        caller_end != spec["end"]
        or indexes.by_address.get(normalize_address(caller_start))
        != caller_identity
        or caller_identity in indexes.provider_ids
        or indexes.by_address.get("0x443140") != current_state_identity
        or current_state_identity in indexes.provider_ids
    ):
        raise ValueError(
            "AppFrame queued-state vptr projection rejects authored identity "
            "or registered extent drift"
        )
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(
        CandidateAssembly(tuple(instructions), frozenset())
    )
    if len(offsets) != len(instructions) or any(offset is None for offset in offsets):
        raise ValueError(
            "AppFrame queued-state vptr projection requires complete COD offsets"
        )
    index_by_offset = {
        int(offset): index for index, offset in enumerate(offsets)
        if offset is not None
    }
    if len(index_by_offset) != len(instructions):
        raise ValueError(
            "AppFrame queued-state vptr projection rejects duplicate COD offsets"
        )
    call_offsets = tuple(
        int(offsets[index])
        for index, instruction in enumerate(instructions)
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
        and _cc_cfg._exact_invocation_encoding(instruction, mnemonic="call")
    )
    if call_offsets != spec["calls"]:
        raise ValueError(
            "AppFrame queued-state vptr projection rejects complete call population drift"
        )
    for offset, body in spec["rows"].items():
        index = index_by_offset.get(offset)
        try:
            observed = (
                bytes(int(item, 16) for item in instructions[index].bytes)
                if index is not None
                else b""
            )
        except (TypeError, ValueError):
            observed = b""
        if observed != body:
            raise ValueError(
                "AppFrame queued-state vptr projection rejects exact lineage "
                f"bytes at +0x{offset:x}"
            )
    get_current_index = index_by_offset[0x09]
    if (
        _cc_cfg._instruction_operand(instructions[get_current_index]).strip()
        != "?GetCurrentState@RecoilApp@@QBEPAURecoilApp_IState@@XZ"
    ):
        raise ValueError(
            "AppFrame queued-state vptr projection rejects GetCurrentState identity drift"
        )
    return {
        index_by_offset[int(site)]: str(marker)
        for site, marker in spec["sites"].items()
    }




def _exact_candidate_zui_label_panels_vptr_proofs(
    instructions: Sequence[Instruction],
    *,
    source: str,
    caller_start: str,
    caller_end_exclusive: str | None,
    indexes: IdentityIndexes,
) -> dict[int, str]:
    """Prove the two guarded ``labelPanels[0]`` candidate vcalls.

    VC5 keeps the nullable first-vector-element receiver in EDI across the
    active/inactive branch.  The generic register join intentionally discards
    a null-versus-loaded-object merge, so retain the targetless vptr only for
    this complete, exact caller shape.  Retail independently supplies the
    reviewed ``dynamic:labelPanels-element-vptr`` family for comparison.
    """
    from _recoil.call_contract.records import CandidateAssembly

    start = normalize_address(caller_start)
    if source != "cod" or start != "0x4b4ba0":
        return {}

    def reject(detail: str) -> NoReturn:
        raise _cc_errors.CandidateCallContractEvidenceError(
            "zUI SetInputActive labelPanels vptr projection requires "
            + detail
        )

    caller_identity = "symbol:recoil:function:0x4b4ba0"
    if (
        normalize_address(caller_end_exclusive or "0x0") != "0x4b4c50"
        or indexes.by_address.get(start) != caller_identity
        or caller_identity in indexes.provider_ids
    ):
        reject("the exact authored caller identity and registered extent")

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(
        CandidateAssembly(tuple(instructions), frozenset())
    )
    if len(offsets) != len(instructions) or any(
        offset is None for offset in offsets
    ):
        reject("complete unique COD instruction coordinates")
    index_by_offset = {
        int(offset): index
        for index, offset in enumerate(offsets)
        if offset is not None
    }
    if len(index_by_offset) != len(instructions):
        reject("complete unique COD instruction coordinates")

    expected_call_offsets = (0x4D, 0x5E, 0x6B, 0x7C, 0x89, 0x9A)
    call_offsets = tuple(
        int(offsets[index])
        for index, instruction in enumerate(instructions)
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
        and _cc_cfg._exact_invocation_encoding(instruction, mnemonic="call")
    )
    if call_offsets != expected_call_offsets:
        reject("the complete exact current candidate call population")

    exact_rows = {
        0x00: bytes.fromhex("8b 54 24 04"),
        0x06: bytes.fromhex("8b f1"),
        0x09: bytes.fromhex("33 ff"),
        0x17: bytes.fromhex("8b 8e 10 01 00 00"),
        0x1D: bytes.fromhex("85 c9"),
        0x1F: bytes.fromhex("75 04"),
        0x21: bytes.fromhex("33 c0"),
        0x23: bytes.fromhex("eb 0b"),
        0x25: bytes.fromhex("8b 86 14 01 00 00"),
        0x2B: bytes.fromhex("2b c1"),
        0x2D: bytes.fromhex("c1 f8 02"),
        0x30: bytes.fromhex("33 c9"),
        0x32: bytes.fromhex("85 c0"),
        0x34: bytes.fromhex("0f 94 c1"),
        0x37: bytes.fromhex("84 c9"),
        0x39: bytes.fromhex("75 08"),
        0x3B: bytes.fromhex("8b 86 10 01 00 00"),
        0x41: bytes.fromhex("8b 38"),
        0x43: bytes.fromhex("85 d2"),
        0x45: bytes.fromhex("74 2f"),
        0x61: bytes.fromhex("85 ff"),
        0x63: bytes.fromhex("74 38"),
        0x65: bytes.fromhex("8b 17"),
        0x67: bytes.fromhex("6a 01"),
        0x69: bytes.fromhex("8b cf"),
        0x6B: bytes.fromhex("ff 52 60"),
        0x73: bytes.fromhex("c2 04 00"),
        0x76: bytes.fromhex("8b 06"),
        0x7C: bytes.fromhex("ff 50 60"),
        0x7F: bytes.fromhex("85 ff"),
        0x81: bytes.fromhex("74 09"),
        0x83: bytes.fromhex("8b 17"),
        0x85: bytes.fromhex("6a 00"),
        0x87: bytes.fromhex("8b cf"),
        0x89: bytes.fromhex("ff 52 60"),
        0x9A: bytes.fromhex("ff 50 60"),
        0xA2: bytes.fromhex("c2 04 00"),
    }
    for offset, expected_body in exact_rows.items():
        index = index_by_offset.get(offset)
        try:
            body = (
                bytes(int(item, 16) for item in instructions[index].bytes)
                if index is not None
                else b""
            )
        except (TypeError, ValueError):
            body = b""
        if body != expected_body:
            reject(f"exact guarded receiver lineage at +0x{offset:x}")

    storage = "load(load(load(this+0x110)))"
    return {
        index_by_offset[0x6B]: storage,
        index_by_offset[0x89]: storage,
    }


def _exact_candidate_zui_spilled_receiver_vptr_proof(
    instructions: Sequence[Instruction],
    *,
    source: str,
    caller_start: str,
    caller_end_exclusive: str | None,
    indexes: IdentityIndexes,
    candidate_caller_definition: CandidateCallerDefinition | None,
) -> dict[int, str]:
    """Prove one VC5 stack-spilled receiver-vptr call in zUI.

    ``HudUiCheckToggleWidget::LoadFromZrd`` loads the vptr of its newly
    allocated panel receiver, spills that value across one ``ZrdArrayInt``
    call, and reloads the same absolute stack slot after the argument pushes.
    The ordinary register interpreter deliberately does not retain arbitrary
    stack values across calls.  Preserve this one targetless vptr only when
    the complete caller body, relocation package, and exact spill/reload
    window match the governed VC5 artifact.
    """
    from _recoil.call_contract.records import CandidateAssembly

    start = normalize_address(caller_start)
    if source != "cod" or start != "0x4b7340":
        return {}

    def reject(detail: str) -> NoReturn:
        raise _cc_errors.CandidateCallContractEvidenceError(
            "zUI spilled receiver-vptr projection requires " + detail
        )

    caller_identity = "symbol:recoil:function:0x4b7340"
    caller = candidate_caller_definition
    if (
        normalize_address(caller_end_exclusive or "0x0") != "0x4b7d60"
        or indexes.by_address.get(start) != caller_identity
        or caller_identity in indexes.provider_ids
        or caller is None
    ):
        reject("the exact authored caller identity, extent, and definition")

    canonical_relocation_names = _cc_recoil_ui._canonical_zui_relocation_names(
        caller.relocations
    )
    relocation_payload = _cc_recoil_ui._zui_relocation_payload(caller.relocations)
    if (
        caller.symbol
        != (
            "?LoadFromZrd@HudUiCheckToggleWidget@@UAEHPAUNode@zReader@@"
            "PAUHudUiBackground@@@Z"
        )
        or len(caller.data) != 0x9A0
        or len(caller.relocations) != 83
        or any(
            name != row.symbol_name
            and _cc_catalog._ZUI_TU_RELOCATION_SYMBOL.search(row.symbol_name) is None
            and _cc_catalog._VC5_PRIVATE_RELOCATION_LABEL.fullmatch(row.symbol_name) is None
            for row, name in zip(
                caller.relocations, canonical_relocation_names
            )
        )
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
        reject("the complete caller body, relocation package, and mask")

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(
        CandidateAssembly(tuple(instructions), frozenset())
    )
    unresolved_offsets = tuple(
        index for index, offset in enumerate(offsets) if offset is None
    )
    if (
        len(offsets) != len(instructions)
        or unresolved_offsets != (0,)
        or not instructions
        or bytes(int(item, 16) for item in instructions[0].bytes)
        != bytes.fromhex("64 a1 00 00 00 00")
    ):
        # VC5's COD listing omits only the coordinate of the leading FS
        # exception-chain load.  The complete COFF body above authenticates
        # that row; every later row, including the proof window, must retain
        # one unique coordinate.
        reject("the exact leading FS row and all later COD coordinates")
    index_by_offset = {
        int(offset): index
        for index, offset in enumerate(offsets)
        if offset is not None
    }
    if len(index_by_offset) != len(instructions) - 1:
        reject("complete unique COD instruction coordinates")

    exact_rows = {
        0x7F3: bytes.fromhex("8b 06"),
        0x7F5: bytes.fromhex("83 c4 08"),
        0x7F8: bytes.fromhex("ba 03 00 00 00"),
        0x7FD: bytes.fromhex("8b cb"),
        0x7FF: bytes.fromhex("55"),
        0x800: bytes.fromhex("89 44 24 38"),
        0x804: bytes.fromhex("e8 00 00 00 00"),
        0x809: bytes.fromhex("8b 8f c0 00 00 00"),
        0x80F: bytes.fromhex("ba 02 00 00 00"),
        0x814: bytes.fromhex("03 c1"),
        0x816: bytes.fromhex("8b cb"),
        0x818: bytes.fromhex("50"),
        0x819: bytes.fromhex("6a 00"),
        0x81B: bytes.fromhex("e8 00 00 00 00"),
        0x820: bytes.fromhex("8b 8f bc 00 00 00"),
        0x826: bytes.fromhex("03 c1"),
        0x828: bytes.fromhex("8b ce"),
        0x82A: bytes.fromhex("50"),
        0x82B: bytes.fromhex("8b 44 24 3c"),
        0x82F: bytes.fromhex("ff 50 0c"),
    }
    for offset, expected_body in exact_rows.items():
        index = index_by_offset.get(offset)
        try:
            body = (
                bytes(int(item, 16) for item in instructions[index].bytes)
                if index is not None
                else b""
            )
        except (TypeError, ValueError):
            body = b""
        if body != expected_body:
            reject(f"the exact spill/reload lineage at +0x{offset:x}")

    helper_relocations = tuple(
        row
        for row in caller.relocations
        if row.offset in {0x805, 0x81C}
    )
    if (
        len(helper_relocations) != 2
        or tuple(row.offset for row in helper_relocations) != (0x805, 0x81C)
        or any(row.type != IMAGE_REL_I386_REL32 for row in helper_relocations)
        or any(
            not row.symbol_name.startswith("?ZrdArrayInt@?")
            for row in helper_relocations
        )
    ):
        reject("the exact intervening helper relocations")

    return {index_by_offset[0x82F]: "load(this)"}


def _exact_candidate_zui_cycle_entry_vptr_proofs(
    instructions: Sequence[Instruction],
    *,
    source: str,
    caller_start: str,
    caller_end_exclusive: str | None,
    indexes: IdentityIndexes,
    candidate_caller_definition: CandidateCallerDefinition | None,
) -> dict[int, str]:
    """Prove two indexed ``entriesA[index]`` vptr calls in AddTextEntry.

    VC5's SEH prologue loads the first entry argument into EBX before the
    constructor path, then uses ``[this+ebx*4+0x168]`` for SetPos and
    SetVisible.  The generic candidate walk deliberately does not carry that
    entry-stack/index lineage through the constructor join.  Retain it only
    for the complete frozen caller and the two exact reload/vptr/call windows.
    """
    from _recoil.call_contract.records import CandidateAssembly

    start = normalize_address(caller_start)
    if source != "cod" or start != "0x4b7fd0":
        return {}

    def reject(detail: str) -> NoReturn:
        raise _cc_errors.CandidateCallContractEvidenceError(
            "zUI cycle-entry indexed-vptr projection requires " + detail
        )

    caller_identity = "symbol:recoil:function:0x4b7fd0"
    caller = candidate_caller_definition
    if (
        normalize_address(caller_end_exclusive or "0x0") != "0x4b8100"
        or indexes.by_address.get(start) != caller_identity
        or caller_identity in indexes.provider_ids
        or caller is None
    ):
        reject("the exact authored caller identity, extent, and definition")

    relocation_payload = _cc_recoil_ui._zui_relocation_payload(caller.relocations)
    if (
        caller.symbol
        != "?AddTextEntry@HudUiCycleSelectorWidget@@QAEXHPBDHH@Z"
        or len(caller.data) != 0x130
        or len(caller.relocations) != 8
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
        reject("the complete caller body, relocation package, and mask")

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(
        CandidateAssembly(tuple(instructions), frozenset())
    )
    unresolved_offsets = tuple(
        index for index, offset in enumerate(offsets) if offset is None
    )
    if (
        len(offsets) != len(instructions)
        or unresolved_offsets != (0,)
        or not instructions
        or bytes(int(item, 16) for item in instructions[0].bytes)
        != bytes.fromhex("64 a1 00 00 00 00")
    ):
        reject("the exact leading FS row and all later COD coordinates")
    index_by_offset = {
        int(offset): index
        for index, offset in enumerate(offsets)
        if offset is not None
    }
    if len(index_by_offset) + 1 != len(instructions):
        reject("complete unique COD instruction coordinates")

    exact_rows = {
        0x016: (bytes.fromhex("8b 5c 24 14"), "ebx, dword _index$[esp+12]"),
        0x01C: (bytes.fromhex("8b f1"), "esi, ecx"),
        0x0D7: (
            bytes.fromhex("8b 8c 9e 68 01 00 00"),
            "ecx, dword [esi+ebx*4+360]",
        ),
        0x0E7: (bytes.fromhex("8b 11"), "edx, dword [ecx]"),
        0x0F3: (bytes.fromhex("ff 52 0c"), "dword [edx+12]"),
        0x0F6: (
            bytes.fromhex("8b 8c 9e 68 01 00 00"),
            "ecx, dword [esi+ebx*4+360]",
        ),
        0x0FE: (bytes.fromhex("8b 11"), "edx, dword [ecx]"),
        0x100: (bytes.fromhex("ff 52 60"), "dword [edx+96]"),
        0x103: (
            bytes.fromhex("8b 84 9e 68 01 00 00"),
            "eax, dword [esi+ebx*4+360]",
        ),
    }
    for offset, (body, operand) in exact_rows.items():
        index = index_by_offset.get(offset)
        try:
            observed_body = (
                bytes(int(item, 16) for item in instructions[index].bytes)
                if index is not None else b""
            )
        except (TypeError, ValueError):
            observed_body = b""
        if (
            observed_body != body
            or index is None
            or _cc_cfg._instruction_operand(instructions[index]).strip() != operand
            or caller.data[offset:offset + len(body)] != body
        ):
            reject(f"the exact indexed receiver lineage at +0x{offset:x}")

    storage = (
        "load(load(affine(this,load(entry-stack+0x4)*4,+0x168)))"
    )
    return {
        index_by_offset[0x0F3]: storage,
        index_by_offset[0x100]: storage,
    }


def _identical_relocated_call_body(
    retail: bytes,
    definition: CandidateCallerDefinition,
    *,
    retail_start: int,
    image_start: int,
    image_end: int,
    indexes: IdentityIndexes,
) -> bool:
    """Compare complete code and independently bound address operands.

    This is an invocation-local equivalence proof, not a stored candidate
    profile or byte-stage acceptance. Unknown decoding, aliases, nonzero COFF
    addends, absent/extra relocations, and differing padding fail closed.
    """
    from _recoil.commands.relocation_expectations import decode_x86_operand_sites

    if not retail or len(retail) != len(definition.data):
        return False
    sites, unresolved = decode_x86_operand_sites(retail, function_address=retail_start)
    if unresolved:
        return False
    required = {}
    for site in sites:
        operand = retail[site.offset:site.offset + 4]
        if len(operand) != 4:
            return False
        if site.relocation_type == IMAGE_REL_I386_REL32:
            target = retail_start + site.offset + 4 + int.from_bytes(operand, "little", signed=True)
            if retail_start <= target < retail_start + len(retail):
                continue
            identity = indexes.by_address.get(hex(target), "")
        elif site.relocation_type == IMAGE_REL_I386_DIR32:
            target = int.from_bytes(operand, "little")
            if site.kind == "potential-absolute32" and not image_start <= target < image_end:
                continue
            identity = indexes.storage_by_address.get(hex(target), "") or indexes.by_address.get(hex(target), "")
        else:
            return False
        if not identity or site.offset in required:
            return False
        # Do not use a logical alias to equate different physical targets.
        physical = {address_value(address) for address, value in
                    (*indexes.by_address.items(), *indexes.storage_by_address.items())
                    if value == identity}
        if physical != {target}:
            return False
        required[site.offset] = (site.relocation_type, identity)
    actual = {}
    mask = [False] * len(retail)
    for relocation in definition.relocations:
        offset = relocation.offset
        if offset < 0 or offset + 4 > len(retail) or offset in actual:
            return False
        if any(mask[offset:offset + 4]) or any(definition.data[offset:offset + 4]):
            return False
        identity = indexes.by_candidate_name.get(relocation.symbol_name, "")
        if relocation.type == IMAGE_REL_I386_DIR32:
            identity = indexes.storage_by_name.get(relocation.symbol_name, "") or identity
        if not identity:
            return False
        actual[offset] = (relocation.type, identity)
        mask[offset:offset + 4] = [True] * 4
    return (
        actual == required
        and tuple(mask) == tuple(definition.relocation_mask)
        and all(mask[offset] or value == definition.data[offset]
                for offset, value in enumerate(retail))
    )


def _exact_body_vptr_candidate_bridges(
    candidate: CandidateAssembly,
    *,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    expected: Sequence[Mapping[str, Any]],
    retail_call_sites: Sequence[str],
    reference: Path = _cc_catalog.DEFAULT_REFERENCE,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Transfer a retail call lineage only across fully equal relocated code.

    No call is added, removed, retargeted, or changed from direct to indirect.
    Every instruction, branch, receiver computation and typed relocation in
    the complete current COFF body must agree with the live immutable PE.
    The independently decoded candidate CALL still supplies its own slot.
    """
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge
    definition = candidate.caller_definition
    if definition is None or len(expected) != len(retail_call_sites):
        return {}
    retail_virtual_calls = {
        normalize_address(site): row
        for site, row in zip(retail_call_sites, expected)
        if row.get("form") == "call" and row.get("dispatch") == "indirect"
        and row.get("identity_kind") == "virtual-slot" and row.get("storage_identity")
    }
    if not retail_virtual_calls or len(set(retail_call_sites)) != len(retail_call_sites):
        return {}
    start, end = address_value(caller_start), address_value(caller_end_exclusive)
    if end - start != len(definition.data):
        return {}
    with StableReadHandle(reference) as handle:
        image = handle.read()
    headers = parse_pe_headers(image, source=str(reference))
    file_offset = rva_to_offset(start - headers.image_base, headers.sections)
    if file_offset is None:
        return {}
    retail = image[file_offset:file_offset + end - start]
    if not _identical_relocated_call_body(
        retail, definition, retail_start=start, image_start=headers.image_base,
        image_end=headers.image_base + headers.size_of_image, indexes=indexes,
    ):
        return {}
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    counts = Counter(offsets)
    result = {}
    for offset, instruction in zip(offsets, candidate.instructions):
        if offset is None or counts[offset] != 1:
            return {}
        encoded = bytes(int(value, 16) for value in instruction.bytes)
        if definition.data[offset:offset + len(encoded)] != encoded:
            return {}
        row = retail_virtual_calls.get(hex(start + offset))
        if row is None:
            continue
        expression, slot = _cc_targets._memory_slot(_cc_cfg._instruction_operand(instruction))
        register = expression.split("+", 1)[0].strip()
        if (_cc_cfg._instruction_mnemonic(instruction) != "call"
                or not _cc_cfg._exact_invocation_encoding(instruction, mnemonic="call")
                or not re.fullmatch(r"e(?:ax|bx|cx|dx|si|di|bp|sp)", register)
                or (slot or 0) != row.get("slot_displacement")):
            return {}
        result[hex(offset)] = ReviewedLoopVptrStorageBridge(
            register=register, storage_identity=str(row["storage_identity"]),
            slot_displacement=int(row["slot_displacement"]), assembly_source="cod",
            target_identity=str(row.get("target_identity", "")),
        )
    return result


def _canonical_proven_member_storage(value: str) -> str:
    """Normalize rendering only, never establish a receiver's eligibility.

    An exact receiver-field annotation already denotes a field pointer load.
    An address wrapper on a proven embedded member adds no dereference. Keep
    both loads and both offsets where present; accept no arbitrary expression
    grammar, register root, or out-of-range displacement.
    """
    # Parse only the already-proved this/load/field grammar. Fold additions
    # at their own dereference level, never through a load. Unknown roots,
    # malformed expressions, excessive nesting and overflowing offsets remain
    # unchanged, so rendering cannot confer extraction eligibility.
    position = 0

    def consume(token: str) -> None:
        nonlocal position
        if not value.startswith(token, position):
            raise ValueError("not a proved member expression")
        position += len(token)

    def displacement() -> int:
        nonlocal position
        match = re.match(r"\+0x([0-9a-f]+)", value[position:])
        if match is None:
            raise ValueError("not a positive field displacement")
        position += len(match.group(0))
        number = int(match.group(1), 16)
        if number > 0x7FFFFFFF:
            raise ValueError("field displacement overflow")
        return number

    def expression(depth: int = 0) -> str:
        nonlocal position
        if depth > 8:
            raise ValueError("member expression nesting limit")
        if value.startswith("this", position):
            consume("this")
            base = "this"
        elif value.startswith("load(", position):
            consume("load(")
            base = f"load({expression(depth + 1)})"
            consume(")")
        elif value.startswith("exact-receiver-field(", position):
            consume("exact-receiver-field(")
            receiver = expression(depth + 1)
            consume(",")
            offset = displacement()
            consume(")")
            base = f"load({receiver}+0x{offset:x})"
        else:
            raise ValueError("unknown member expression root")
        offset = 0
        while value.startswith("+0x", position):
            offset += displacement()
            if offset > 0x7FFFFFFF:
                raise ValueError("member displacement sum overflow")
        return base + (f"+0x{offset:x}" if offset else "")

    if len(value) <= 512 and value.startswith("load(") and value.endswith(")"):
        try:
            canonical = expression()
            if position == len(value):
                return canonical
        except ValueError:
            pass
    embedded = re.fullmatch(
        r"load\(address\(load\(this\+0x([0-9a-f]+)\)\+0x([0-9a-f]+)\)\)",
        value,
    )
    if embedded is not None:
        root_offset, member_offset = (int(item, 16) for item in embedded.groups())
        if max(root_offset, member_offset) <= 0x7FFFFFFF:
            return f"load(load(this+0x{root_offset:x})+0x{member_offset:x})"
    return value


def _canonical_exact_this_member_vptr_storage(value: str) -> str:
    """Canonicalize one exact vptr load from a positive embedded-this offset."""

    match = re.fullmatch(
        r"(?:load\((?P<direct>this\+0x0*[1-9a-f][0-9a-f]*)\)"
        r"|load\(address\((?P<address>this\+0x0*[1-9a-f][0-9a-f]*)\)\))",
        value,
    )
    if match is None:
        return ""
    return f"load({match.group('direct') or match.group('address')})"


def _exact_this_member_vptr_storage(value: str) -> bool:
    return bool(_canonical_exact_this_member_vptr_storage(value))
