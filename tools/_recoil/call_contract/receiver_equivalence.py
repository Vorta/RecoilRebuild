"""Recoil call-contract receiver equivalence evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import errors as _cc_errors
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

    def render(term: tuple[str, int]) -> str:
        base, offset = term
        return base + (f"+0x{offset:x}" if offset else "")

    def expression(depth: int = 0) -> tuple[str, int]:
        nonlocal position
        if depth > 8:
            raise ValueError("member expression nesting limit")
        offset = 0
        if value.startswith("this", position):
            consume("this")
            base = "this"
        elif value.startswith("call-result(", position):
            match = re.match(r"call-result\((?:symbol|provider):recoil:function:0x[0-9a-f]+\)", value[position:])
            if match is None:
                raise ValueError("not a typed call-result root")
            base = match.group(0)
            position += len(base)
        elif value.startswith("address(", position):
            consume("address(")
            base, offset = expression(depth + 1)
            consume(")")
        elif value.startswith("load(", position):
            consume("load(")
            base = f"load({render(expression(depth + 1))})"
            consume(")")
        elif value.startswith("exact-receiver-field(", position):
            consume("exact-receiver-field(")
            receiver, receiver_offset = expression(depth + 1)
            consume(",")
            field_offset = receiver_offset + displacement()
            if field_offset > 0x7FFFFFFF:
                raise ValueError("receiver field displacement sum overflow")
            consume(")")
            base = f"load({render((receiver, field_offset))})"
        else:
            raise ValueError("unknown member expression root")
        while value.startswith("+0x", position):
            offset += displacement()
            if offset > 0x7FFFFFFF:
                raise ValueError("member displacement sum overflow")
        return base, offset

    if len(value) <= 512 and value.startswith("load(") and value.endswith(")"):
        try:
            canonical = render(expression())
            if position == len(value):
                return canonical
        except ValueError:
            pass
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
