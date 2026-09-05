"""Bounded, fail-closed resource-depth proof over authenticated x86 listings.

Instruction boundaries come from the compiler/disassembler, never byte scanning.
All branches must resolve internally and every call needs an explicit effect.
This proves only the selected resource protocol, not general program behavior.
"""
from __future__ import annotations


def path_depths(instructions, data: bytes, call_effects: dict[int, int]) -> dict[str, object]:
    rows = {}
    cursor = 0
    for encoded, mnemonic in instructions:
        if not encoded:
            raise ValueError("empty path instruction")
        rows[cursor] = (encoded, mnemonic.lower())
        cursor += len(encoded)
    listing_bytes = b"".join(row[0] for row in rows.values())
    # VC5 COD omits up to fifteen trailing alignment NOPs. They may not be
    # reachable: a branch into this omitted tail still fails below.
    if (listing_bytes != data[:cursor] or not 0 <= len(data) - cursor < 16
            or any(byte != 0x90 for byte in data[cursor:])):
        raise ValueError("path listing disagrees with body bytes")
    pending = [(0, 0)]
    visited = set()
    returns = set()
    used_calls = set()
    peak = 0
    while pending:
        pc, depth = pending.pop()
        if (pc, depth) in visited:
            continue
        if len(visited) > 100000 or abs(depth) > 64:
            raise ValueError("unbounded resource-depth proof")
        visited.add((pc, depth))
        if pc not in rows:
            raise ValueError("branch/fallthrough leaves exact instruction boundaries")
        raw, mnemonic = rows[pc]
        after = pc + len(raw)
        opcode = raw[0]
        if ((opcode == 0xe8 and len(raw) == 5)
                or (opcode == 0xff and len(raw) > 1 and (raw[1] >> 3 & 7) == 2)):
            if pc not in call_effects:
                raise ValueError("call lacks reviewed resource effect")
            used_calls.add(pc)
            depth += call_effects[pc]
            if depth < 0:
                raise ValueError(f"resource underflow at body offset {pc:#x}")
            peak = max(peak, depth)
        elif opcode in (0xc3, 0xc2) and len(raw) == (1 if opcode == 0xc3 else 3):
            returns.add(depth)
            continue
        elif (opcode in (0xe9, 0xeb) or 0x70 <= opcode <= 0x7f
              or (opcode == 0x0f and len(raw) == 6 and 0x80 <= raw[1] <= 0x8f)):
            width = 4 if opcode in (0xe9, 0x0f) else 1
            if len(raw) != width + (2 if opcode == 0x0f else 1):
                raise ValueError("unsupported branch encoding")
            pending.append((after + int.from_bytes(raw[-width:], "little", signed=True), depth))
            if opcode in (0xe9, 0xeb):
                continue
        elif (mnemonic.startswith(("j", "ret", "call", "loop", "int", "sys"))
              or mnemonic in ("hlt", "ud2", "iret", "iretd")
              or opcode in (0x9a, 0xea, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xf4)
              or (opcode == 0xff and len(raw) > 1 and (raw[1] >> 3 & 7) in (2, 3, 4, 5))):
            raise ValueError("unsupported path control transfer")
        pending.append((after, depth))
    if not returns or used_calls != set(call_effects):
        raise ValueError("missing return or unreachable/unmatched effect call")
    return {"return_depths": sorted(returns), "peak_depth": peak}


MATRIX_CALL_EFFECTS = {
    "?zMath_UnprojectPointBatch@@YIXPBUzProjectedPoint@@PAUzVec3@@H@Z": (0x474bc0, 0),
    "?MatStackPushPtr@zMath@@YIXPAM@Z": (0x472f30, 1),
    "?MatLoadCameraScratchA@zMath@@YAXXZ": (0x472fa0, 0),
    "?MatStackPopPtr@zMath@@YAXXZ": (0x472f60, -1),
}
UNPROJECT_SYMBOL = "?zMath_UnprojectPointBatchZBuf@@YIXPBUzProjectedPoint@@PAUzVec3@@H@Z"
LIGHT_CALL_EFFECTS = {
    "?ReportOld@zError@@YAXHPBDH0ZZ": (0x404e80, 0),
    "?ComputeWorldTransform@zClass_Light@@YIHPAUzClass_NodePartial@@PAUzClass_LightDataPartial@@@Z": (0x453620, 0),
    "?MatStackPushAndCloneParent@zMath@@YIXPAM@Z": (0x472ef0, 1),
    "?MatLoadCameraScratchB@zMath@@YAXXZ": (0x472f90, 0),
    "?zMath_Mat_TransformNormalBatch@@YIXPBUzVec3@@PAU1@H@Z": (0x474710, 0),
    "?MatStackPopPtr@zMath@@YAXXZ": (0x472f60, -1),
}
MATRIX_PATH_BODIES = {
    0x474c20: (UNPROJECT_SYMBOL, "src/GameZRecoil/zMath/zmth_main.c", 0xe7, MATRIX_CALL_EFFECTS),
    0x453880: ("?gwLightUpdate@zClass_Light@@YIHPAUzClass_NodePartial@@@Z",
               "src/GameZRecoil/zClass/Light.c", 0x1b1, LIGHT_CALL_EFFECTS),
    0x476cf0: ("?RenderNodeSoftware@zModel@@YIXPAUzClass_NodePartial@@H@Z",
               "src/GameZRecoil/zModel/gmod_init.c", 0xe3c, None),
    0x477b30: ("?RenderNodeHardware@zModel@@YIXPAUzClass_NodePartial@@H@Z",
               "src/GameZRecoil/zModel/gmod_init.c", 0x1134, None),
}
MATRIX_PRIMITIVES = {
    "?MatStackPushAndCloneParent@zMath@@YIXPAM@Z": (0x472ef0, 1),
    "?MatStackPushPtr@zMath@@YIXPAM@Z": (0x472f30, 1),
    "?MatStackPopPtr@zMath@@YAXXZ": (0x472f60, -1),
}


def local_primitive_effects(rows, start, targets):
    """Effects of direct primitive invocations only; not callee summaries."""
    effects = {}
    offset = 0
    for raw, _ in rows:
        if raw[0] == 0xe8 and len(raw) == 5:
            target = start + offset + 5 + int.from_bytes(raw[1:], "little", signed=True)
            effects[offset] = targets.get(target, 0)
        elif raw[0] == 0xff and len(raw) > 1 and (raw[1] >> 3 & 7) == 2:
            effects[offset] = 0
        offset += len(raw)
    return effects


def local_object_effects(rows, body):
    effects = local_primitive_effects(rows, 0, {})
    for relocation in body.relocations:
        offset = relocation.offset - getattr(body, "start", 0)
        if relocation.symbol_name in MATRIX_PRIMITIVES:
            if (relocation.type != 0x14 or offset - 1 not in effects
                    or body.data[offset - 1] != 0xe8 or body.data[offset:offset + 4] != bytes(4)):
                raise ValueError("local primitive has ambiguous dispatch/addend")
            effects[offset - 1] = MATRIX_PRIMITIVES[relocation.symbol_name][1]
    return effects


def object_matrix_effects(body, reviewed=MATRIX_CALL_EFFECTS) -> dict[int, int]:
    effects = {}
    for relocation in body.relocations:
        offset = relocation.offset - getattr(body, "start", 0)
        if relocation.type != 0x14:
            continue
        if (relocation.symbol_name not in reviewed or offset < 1
                or body.data[offset - 1] != 0xe8
                or body.data[offset:offset + 4] != bytes(4)):
            raise ValueError("matrix call lacks exact reviewed REL32 identity/addend")
        effects[offset - 1] = reviewed[relocation.symbol_name][1]
    return effects


def image_matrix_effects(instructions, start: int, targets: dict[int, int]) -> dict[int, int]:
    effects = {}
    offset = 0
    for raw, _ in instructions:
        if raw[0] == 0xe8 and len(raw) == 5:
            target = start + offset + 5 + int.from_bytes(raw[1:], "little", signed=True)
            if target not in targets:
                raise ValueError("matrix call has unreviewed linked target")
            effects[offset] = targets[target]
        offset += len(raw)
    return effects
