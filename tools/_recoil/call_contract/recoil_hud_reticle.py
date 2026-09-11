"""Recoil call-contract recoil hud reticle evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract.listing import _matches_compiler_literal
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        CandidateLocalStaticDefinition,
        IdentityIndexes,
        ReviewedExactIndirectStorageBridge,
    )

import re
import struct
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_DIR32,
    IMAGE_REL_I386_REL32,
    IMAGE_SYM_CLASS_STATIC,
)
from _recoil.lib.progress import ProgressDocument, address_value, normalize_address


def _hud_ui_mgr_reticle_widget_candidate_rebaseline_bridges(
    candidate: CandidateAssembly,
    *,
    storage_identity: str,
    normalized_start: str,
    caller_end_expected: str,
) -> dict[str, ReviewedExactIndirectStorageBridge]:
    """Guard the complete current reticle candidate prefix and callsites."""
    from _recoil.call_contract.records import ReviewedExactIndirectStorageBridge
    caller = candidate.caller_definition
    caller_symbol = (
        "?UpdateTargetReticleFromCursor@HudUiMgr@@YIHHPAUzVec3@@MM@Z"
    )
    superseded_wsi024_prefix_end = 0x24F
    superseded_wsi024_tail_start = 0x4D2
    guarded_tail_start = 0x497
    superseded_wsi024_prefix = bytes.fromhex(
        "81 ec 58 05 00 00 d9 84 24 5c 05 00 00 d8 25 00 00 00 00 "
        "d9 84 24 60 05 00 00 d8 25 00 00 00 00 d9 c9 d8 0d "
        "44 03 00 00 d9 c9 d8 0d 48 03 00 00 d9 c9 d8 05 3c "
        "03 00 00 d9 c9 d8 05 40 03 00 00 d9 c9 d9 54 24 04 "
        "55 56 d9 c9 d9 5c 24 08 57 8b ea 8b f9 e8 00 00 00 "
        "00 d9 44 24 0c 8b f0 e8 00 00 00 00 85 ff 89 35 4c "
        "03 00 00 a3 50 03 00 00 75 1d a1 64 03 00 00 6a 00 "
        "b9 64 03 00 00 ff 50 60 33 c0 5f 5e 5d 81 c4 58 05 "
        "00 00 c2 08 00 83 ff 01 75 1d 8b 15 64 03 00 00 57 "
        "b9 64 03 00 00 ff 52 60 33 c0 5f 5e 5d 81 c4 58 05 "
        "00 00 c2 08 00 83 ff 02 0f 85 28 04 00 00 8b 0d "
        "38 03 00 00 8b 15 34 03 00 00 2b c1 2b f2 50 a1 64 "
        "03 00 00 56 b9 64 03 00 00 ff 50 0c a0 48 03 00 00 "
        "a8 01 75 18 8a d0 68 00 00 00 00 80 ca 01 88 15 48 "
        "03 00 00 e8 00 00 00 00 83 c4 04 8b 15 64 03 00 00 "
        "33 c9 89 4c 24 34 c7 44 24 30 00 00 00 00 89 4c 24 "
        "38 89 4c 24 3c b9 64 03 00 00 ff 52 68 89 44 24 34 "
        "a1 a0 03 00 00 85 c0 74 06 0f bf 70 06 eb 02 33 f6 "
        "a1 64 03 00 00 b9 64 03 00 00 ff 50 68 8b 15 64 03 "
        "00 00 03 c6 b9 64 03 00 00 89 44 24 3c ff 52 64 89 "
        "44 24 30 a1 a0 03 00 00 85 c0 74 06 0f bf 70 04 eb "
        "02 33 f6 a1 64 03 00 00 b9 64 03 00 00 ff 50 64 03 "
        "c6 89 44 24 38 e8 00 00 00 00 8d 4c 24 30 50 51 68 "
        "38 03 00 00 ff 15 00 00 00 00 85 c0 0f 84 b5 00 00 "
        "00 8b 15 64 03 00 00 b9 64 03 00 00 ff 52 68 8b 3d "
        "3c 03 00 00 b9 64 03 00 00 2b f8 a1 64 03 00 00 89 "
        "3d 3c 03 00 00 ff 50 68 8b 35 44 03 00 00 8b 15 64 "
        "03 00 00 2b f0 b9 64 03 00 00 89 35 44 03 00 00 ff "
        "52 64 8b 15 38 03 00 00 b9 64 03 00 00 2b d0 a1 64 "
        "03 00 00 89 15 38 03 00 00 ff 50 64 8b 0d 40 03 00 "
        "00 8b 15 64 03 00 00 2b c8 89 0d 40 03 00 00 b9 64 "
        "03 00 00 ff 52 64 8b 3d 38 03 00 00 8b f0 a1 64 03 "
        "00 00 b9 64 03 00 00 03 f7 ff 50 68 8b 15 3c 03 00 "
        "00 b9 64 03 00 00 03 c2 8b 15 64 03 00 00 50 56 ff "
        "52 0c "
        "c7 05 a8 03 00 00 38 03 00 00"
    )
    superseded_wsi024_tail = bytes.fromhex(
        "e8 00 00 00 00 8b d0 8d 4c 24 50 e8 00 00 00 00 "
        "5f 5e 33 c0 5d 81 c4 58 05 00 00 c2 08 00"
    )
    base_relocation_specs = (
        (0xF, IMAGE_REL_I386_DIR32, "$T88358", 0),
        (0x1C, IMAGE_REL_I386_DIR32, "$T88358", 0),
        (0x24, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x344),
        (0x2C, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x348),
        (0x34, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x33C),
        (0x3C, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x340),
        (0x54, IMAGE_REL_I386_REL32, "__ftol", 0),
        (0x5F, IMAGE_REL_I386_REL32, "__ftol", 0),
        (0x67, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x34C),
        (0x6C, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x350),
    )
    shifted_relocation_specs = (
        (0x8A, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x338),
        (0x90, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x334),
        (0x9A, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (0xA0, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (0xA8, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL, 0x348),
        (0xB3, IMAGE_REL_I386_DIR32, "?ReticleStaticAtexitStub@HudUiMgr@@YAXXZ", 0),
        (0xBC, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL, 0x348),
        (0xC1, IMAGE_REL_I386_REL32, "_atexit", 0),
        (0xCA, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (0xE5, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (0xF1, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x3A0),
        (0x102, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (0x107, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (0x110, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (0x117, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (0x127, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x3A0),
        (0x138, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (0x13D, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (0x14B, IMAGE_REL_I386_REL32, "?GetDisplaySection@zOpt@@YIPAUzOpt_ViewRectSection@@XZ", 0),
        (0x156, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL, 0x338),
        (0x15C, IMAGE_REL_I386_DIR32, "__imp__IntersectRect@12", 0),
        (0x16A, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (0x16F, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (0x178, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL, 0x33C),
        (0x17D, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (0x184, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (0x18A, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL, 0x33C),
        (0x193, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL, 0x344),
        (0x199, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (0x1A0, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (0x1A6, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL, 0x344),
        (0x1AF, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL, 0x338),
        (0x1B4, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (0x1BB, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (0x1C1, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL, 0x338),
        (0x1CA, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL, 0x340),
        (0x1D0, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (0x1D8, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL, 0x340),
        (0x1DD, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (0x1E6, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL, 0x338),
        (0x1ED, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (0x1F2, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (0x1FD, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL, 0x33C),
        (0x202, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (0x20A, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (0x215, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x3A8),
        (0x219, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL, 0x338),
        (0x23A, IMAGE_REL_I386_REL32, "?ScreenToWorld@HudUiMgr@@YIXPAM@Z", 0),
        (0x240, IMAGE_REL_I386_DIR32, "_g_GameStateOrMapTable", 0),
        (0x246, IMAGE_REL_I386_DIR32, "_g_MainCamera", 0),
        (
            0x267,
            IMAGE_REL_I386_REL32,
            "?gwCameraGetNearFarClip@zClass_Camera@@"
            "YIHPAUzClass_NodePartial@@PAM1@Z",
            0,
        ),
        (0x26D, IMAGE_REL_I386_DIR32, "$T88344", 0),
        (
            0x296,
            IMAGE_REL_I386_REL32,
            "?zMathUnprojectPointBatchZBuf@@"
            "YIXPBUzProjectedPoint@@PAUzVec3@@H@Z",
            0,
        ),
        (0x2A6, IMAGE_REL_I386_DIR32, "$T88344", 0),
        (
            0x2CC,
            IMAGE_REL_I386_REL32,
            "?zMathUnprojectPointBatchZBuf@@"
            "YIXPBUzProjectedPoint@@PAUzVec3@@H@Z",
            0,
        ),
        (
            0x2D9,
            IMAGE_REL_I386_REL32,
            "?gwNodeSetRaycastable@zClass_Class@@"
            "YIHPAUzClass_NodePartial@@H@Z",
            0,
        ),
        (
            0x2F5,
            IMAGE_REL_I386_REL32,
            "?gwNodeSetRaycastable@zClass_Class@@"
            "YIHPAUzClass_NodePartial@@H@Z",
            0,
        ),
        (
            0x2FF,
            IMAGE_REL_I386_REL32,
            "?SetStopAfterFirstHit@zClass_cls_di@@YIXH@Z",
            0,
        ),
        (0x31A, IMAGE_REL_I386_DIR32, "_g_Player_RuntimeDiScene", 0),
        (
            0x32D,
            IMAGE_REL_I386_REL32,
            "?RaycastSelectClosestHitBetweenPoints@zClass_cls_di@@"
            "YIHPAUzClass_NodePartial@@PBUzVec3@@1"
            "PAUPlayerProbeSampleCandidateBuffer@@@Z",
            0,
        ),
        (
            0x33C,
            IMAGE_REL_I386_REL32,
            "?gwNodeSetRaycastable@zClass_Class@@"
            "YIHPAUzClass_NodePartial@@H@Z",
            0,
        ),
        (
            0x35B,
            IMAGE_REL_I386_REL32,
            "?gwNodeSetRaycastable@zClass_Class@@"
            "YIHPAUzClass_NodePartial@@H@Z",
            0,
        ),
        (0x370, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x330),
        (0x375, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x358),
        (0x37B, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x328),
        (0x381, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x32C),
        (0x398, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x328),
        (0x3A1, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x32C),
        (0x3AA, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x330),
        (0x3B8, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x35C),
        (0x3C1, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x354),
        (0x3C7, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x364),
        (
            0x3CC,
            IMAGE_REL_I386_REL32,
            "?SetImageBorrowedAndInvalidate@HudUiWidget@@"
            "QAEPAUzVidImagePartial@@PAU2@@Z",
            0,
        ),
        (0x3D2, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x328),
        (0x3DB, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x32C),
        (0x3E3, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x330),
        (
            0x3EB,
            IMAGE_REL_I386_REL32,
            "?GetRenderSection@zOpt@@YIPAUzOpt_ViewRectSection@@XZ",
            0,
        ),
        (0x3F9, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0xC10),
        (0x413, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0xC10),
        (0x431, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0xC14),
        (0x44B, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0xC14),
        (0x466, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0xC10),
        (0x46C, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0xC14),
        (0x474, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0xC10),
        (0x47C, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0xC14),
        (
            0x4A1,
            IMAGE_REL_I386_REL32,
            "?GetReplicateMode@zOpt@@YIHXZ",
            0,
        ),
        (
            0x4AC,
            IMAGE_REL_I386_REL32,
            "?SetTargetRect@zClipAlt@@YIXPBUzClipAltFloatRect@@H@Z",
            0,
        ),
    )
    superseded_wsi024_relocation_specs = (
        *base_relocation_specs,
        (
            0x73,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x7A,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x96,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x9C,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        *(
            (offset + 0x32, kind, symbol, addend)
            for offset, kind, symbol, addend
            in shifted_relocation_specs
        ),
    )
    semantic_end = 0x4EF
    exact_body = bytes.fromhex(
        "81 ec 58 05 00 00 d9 84 24 5c 05 00 00 d8 25 00 00 00 00 d9 84 24 60 05 "
        "00 00 d8 25 00 00 00 00 d9 c9 d8 0d 44 03 00 00 d9 c9 d8 0d 48 03 00 00 "
        "d9 c9 d8 05 3c 03 00 00 d9 c9 d8 05 40 03 00 00 d9 c9 d9 54 24 04 55 56 "
        "d9 c9 d9 5c 24 08 57 8b ea 8b f9 e8 00 00 00 00 d9 44 24 0c 8b f0 e8 00 "
        "00 00 00 8b cf 33 ff 2b cf 89 35 4c 03 00 00 a3 50 03 00 00 0f 84 59 04 "
        "00 00 49 0f 84 34 04 00 00 49 0f 85 59 04 00 00 8b 15 38 03 00 00 b9 64 "
        "03 00 00 2b c2 8b 15 34 03 00 00 50 a1 64 03 00 00 2b f2 56 ff 50 0c a0 "
        "48 03 00 00 a8 01 75 18 8a d0 68 00 00 00 00 80 ca 01 88 15 48 03 00 00 "
        "e8 00 00 00 00 83 c4 04 8b 15 64 03 00 00 33 c9 89 4c 24 34 89 7c 24 30 "
        "89 4c 24 38 89 4c 24 3c b9 64 03 00 00 ff 52 68 89 44 24 34 a1 a0 03 00 "
        "00 3b c7 74 06 0f bf 70 06 eb 02 33 f6 a1 64 03 00 00 b9 64 03 00 00 ff "
        "50 68 8b 15 64 03 00 00 03 c6 b9 64 03 00 00 89 44 24 3c ff 52 64 89 44 "
        "24 30 a1 a0 03 00 00 3b c7 74 06 0f bf 70 04 eb 02 33 f6 a1 64 03 00 00 "
        "b9 64 03 00 00 ff 50 64 03 c6 89 44 24 38 e8 00 00 00 00 8d 4c 24 30 50 "
        "51 68 38 03 00 00 ff 15 00 00 00 00 85 c0 0f 84 b4 00 00 00 8b 15 64 03 "
        "00 00 b9 64 03 00 00 ff 52 68 8b 0d 3c 03 00 00 2b c8 a1 64 03 00 00 89 "
        "0d 3c 03 00 00 b9 64 03 00 00 ff 50 68 8b 35 44 03 00 00 8b 15 64 03 00 "
        "00 2b f0 b9 64 03 00 00 89 35 44 03 00 00 ff 52 64 8b 15 38 03 00 00 b9 "
        "64 03 00 00 2b d0 a1 64 03 00 00 89 15 38 03 00 00 ff 50 64 8b 0d 40 03 "
        "00 00 8b 15 64 03 00 00 2b c8 89 0d 40 03 00 00 b9 64 03 00 00 ff 52 64 "
        "8b f0 a1 38 03 00 00 03 f0 a1 64 03 00 00 b9 64 03 00 00 ff 50 68 8b 15 "
        "3c 03 00 00 b9 64 03 00 00 03 c2 8b 15 64 03 00 00 50 56 ff 52 0c c7 05 "
        "a8 03 00 00 38 03 00 00 8b 4c 24 0c 8b 44 24 10 89 4c 24 1c 8d 4c 24 18 "
        "89 44 24 18 c7 44 24 20 00 00 00 00 e8 00 00 00 00 8b 15 00 00 00 00 8b "
        "0d 00 00 00 00 8d 44 24 40 8b 72 04 50 8d 54 24 18 c7 44 24 18 00 00 00 "
        "00 c7 44 24 44 00 00 00 00 e8 00 00 00 00 d9 05 00 00 00 00 d8 74 24 14 "
        "33 c9 6a 01 89 4c 24 4c 8d 54 24 48 89 4c 24 50 8d 4c 24 1c c7 44 24 48 "
        "00 00 00 00 d9 5c 24 24 e8 00 00 00 00 33 d2 c7 44 24 24 00 00 00 00 d9 "
        "05 00 00 00 00 89 54 24 28 6a 01 89 54 24 30 8b 86 e4 05 00 00 8d 54 24 "
        "28 8b 08 d8 71 1c 8d 4c 24 1c d9 5c 24 24 e8 00 00 00 00 8b 8e d0 0e 00 "
        "00 33 d2 e8 00 00 00 00 83 be 8c 05 00 00 07 75 13 8b 86 e4 05 00 00 33 "
        "d2 8b 48 28 8b 49 0c e8 00 00 00 00 b9 00 00 04 00 e8 00 00 00 00 89 7c "
        "24 60 b9 40 01 00 00 33 c0 8d 7c 24 64 f3 ab 8b 0d 00 00 00 00 8d 54 24 "
        "60 8d 44 24 24 52 50 8d 54 24 4c e8 00 00 00 00 8b 8e d0 0e 00 00 33 d2 "
        "8b f8 e8 00 00 00 00 83 be 8c 05 00 00 07 75 16 8b 8e e4 05 00 00 ba 01 "
        "00 00 00 8b 41 28 8b 48 0c e8 00 00 00 00 85 ff 74 24 8b 44 24 2c 8b 4c "
        "24 24 8b 54 24 28 a3 30 03 00 00 a1 58 03 00 00 89 0d 28 03 00 00 89 15 "
        "2c 03 00 00 eb 3e 8b 44 24 60 8d 0c 80 8b 54 cc 70 8d 44 cc 64 89 15 28 "
        "03 00 00 8b 48 10 89 0d 2c 03 00 00 8b 50 14 89 15 30 03 00 00 8b 40 24 "
        "8b 88 bc 00 00 00 a1 5c 03 00 00 85 c9 75 05 a1 54 03 00 00 50 b9 64 03 "
        "00 00 e8 00 00 00 00 8b 0d 28 03 00 00 89 4d 00 8b 15 2c 03 00 00 89 55 "
        "04 a1 30 03 00 00 89 45 08 e8 00 00 00 00 d9 44 24 10 8b c8 db 01 d8 05 "
        "10 0c 00 00 d9 c9 d8 d9 df e0 f6 c4 01 75 20 dd d8 d9 44 24 10 db 41 08 "
        "d8 25 10 0c 00 00 d9 c9 d8 d9 df e0 f6 c4 41 74 06 dd d8 d9 44 24 10 d9 "
        "44 24 0c db 41 04 d8 05 14 0c 00 00 d9 c9 d8 d9 df e0 f6 c4 01 75 20 dd "
        "d8 d9 44 24 0c db 41 0c d8 25 14 0c 00 00 d9 c9 d8 d9 df e0 f6 c4 41 74 "
        "06 dd d8 d9 44 24 0c d9 c1 d9 c1 d9 05 10 0c 00 00 d9 05 14 0c 00 00 d9 "
        "cb d8 25 10 0c 00 00 d9 ca d8 25 14 0c 00 00 d9 c9 d8 c5 d9 cb d8 c4 d9 "
        "ca d9 5c 24 50 d9 5c 24 54 d9 c9 d9 5c 24 58 d9 5c 24 5c dd d8 dd d8 e8 "
        "00 00 00 00 8b d0 8d 4c 24 50 e8 00 00 00 00 33 c0 5f 5e 5d 81 c4 58 05 "
        "00 00 c2 08 00 8b 15 64 03 00 00 6a 01 b9 64 03 00 00 ff 52 60 33 c0 5f "
        "5e 5d 81 c4 58 05 00 00 c2 08 00 a1 64 03 00 00 57 b9 64 03 00 00 ff 50 "
        "60 5f 5e 33 c0 5d 81 c4 58 05 00 00 c2 08 00 90"
    )
    local_static_definitions = (
        caller.local_static_definitions if caller is not None else ()
    )

    def exact_local_literal(
        *,
        data: bytes,
    ) -> CandidateLocalStaticDefinition:
        matches = tuple(
            item
            for item in local_static_definitions
            if _matches_compiler_literal(item, data)
        )
        if (
            len(matches) != 1
            or sum(
                item.name == matches[0].name
                for item in local_static_definitions
            )
            != 1
        ):
            raise ValueError(
                "HUD reticle rebaseline rejects exact current governed "
                "local-static "
                f"$T literal with contents {data.hex()}"
            )
        return matches[0]

    negative_one_literal = exact_local_literal(
        data=bytes.fromhex("00 00 80 bf"),
    )
    positive_one_literal = exact_local_literal(
        data=bytes.fromhex("00 00 80 3f"),
    )
    if negative_one_literal.name == positive_one_literal.name:
        raise ValueError(
            "HUD reticle rebaseline rejects exact current aliased "
            "governed local-static "
            "$T literals"
        )

    relocation_offsets = (
        0xF, 0x1C, 0x24, 0x2C, 0x34, 0x3C, 0x54, 0x5F, 0x6B, 0x70,
        0x8A, 0x8F, 0x97, 0x9D, 0xA8, 0xB3, 0xBC, 0xC1, 0xCA, 0xE1,
        0xED, 0xFE, 0x103, 0x10C, 0x113, 0x123, 0x134, 0x139, 0x147,
        0x152, 0x158, 0x166, 0x16B, 0x174, 0x17B, 0x181, 0x186, 0x18F,
        0x195, 0x19C, 0x1A2, 0x1AB, 0x1B0, 0x1B7, 0x1BD, 0x1C6, 0x1CC,
        0x1D4, 0x1D9, 0x1E3, 0x1EA, 0x1EF, 0x1F8, 0x1FD, 0x205, 0x210,
        0x214, 0x235, 0x23B, 0x241, 0x262, 0x268, 0x291, 0x2A1, 0x2C7,
        0x2D4, 0x2F0, 0x2FA, 0x311, 0x324, 0x333, 0x352, 0x367, 0x36C,
        0x372, 0x378, 0x38F, 0x398, 0x3A1, 0x3AF, 0x3B8, 0x3BE, 0x3C3,
        0x3C9, 0x3D2, 0x3DA, 0x3E2, 0x3F0, 0x40A, 0x428, 0x442, 0x45D,
        0x463, 0x46B, 0x473, 0x498, 0x4A3, 0x4B7, 0x4BE, 0x4D4, 0x4DA,
    )
    relocation_fact_groups = (
        (
            IMAGE_REL_I386_DIR32,
            negative_one_literal.name,
            0,
            (0xF, 0x1C),
        ),
        (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x344,
            (0x24,),
        ),
        (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x348,
            (0x2C,),
        ),
        (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x33C,
            (0x34,),
        ),
        (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x340,
            (0x3C,),
        ),
        (IMAGE_REL_I386_REL32, "__ftol", 0, (0x54, 0x5F)),
        (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x34C,
            (0x6B,),
        ),
        (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x350,
            (0x70,),
        ),
        (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x338,
            (0x8A,),
        ),
        (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
            (
                0x8F, 0x9D, 0xCA, 0xE1, 0xFE, 0x103, 0x10C, 0x113,
                0x134, 0x139, 0x166, 0x16B, 0x17B, 0x186, 0x195,
                0x19C, 0x1B0, 0x1B7, 0x1CC, 0x1D9, 0x1EA, 0x1EF,
                0x1FD, 0x205, 0x3BE, 0x4B7, 0x4BE, 0x4D4, 0x4DA,
            ),
        ),
        (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x334,
            (0x97,),
        ),
        (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL,
            0x348,
            (0xA8, 0xBC),
        ),
        (
            IMAGE_REL_I386_DIR32,
            "?ReticleStaticAtexitStub@HudUiMgr@@YAXXZ",
            0,
            (0xB3,),
        ),
        (IMAGE_REL_I386_REL32, "_atexit", 0, (0xC1,)),
        (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x3A0,
            (0xED, 0x123),
        ),
        (
            IMAGE_REL_I386_REL32,
            "?GetDisplaySection@zOpt@@YIPAUzOpt_ViewRectSection@@XZ",
            0,
            (0x147,),
        ),
        (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL,
            0x338,
            (0x152, 0x1AB, 0x1BD, 0x1E3, 0x214),
        ),
        (IMAGE_REL_I386_DIR32, "__imp__IntersectRect@12", 0, (0x158,)),
        (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL,
            0x33C,
            (0x174, 0x181, 0x1F8),
        ),
        (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL,
            0x344,
            (0x18F, 0x1A2),
        ),
        (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL,
            0x340,
            (0x1C6, 0x1D4),
        ),
        (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x3A8,
            (0x210,),
        ),
        (
            IMAGE_REL_I386_REL32,
            "?ScreenToWorld@HudUiMgr@@YIXPAM@Z",
            0,
            (0x235,),
        ),
        (IMAGE_REL_I386_DIR32, "_g_GameStateOrMapTable", 0, (0x23B,)),
        (IMAGE_REL_I386_DIR32, "_g_MainCamera", 0, (0x241,)),
        (
            IMAGE_REL_I386_REL32,
            "?gwCameraGetNearFarClip@zClass_Camera@@"
            "YIHPAUzClass_NodePartial@@PAM1@Z",
            0,
            (0x262,),
        ),
        (
            IMAGE_REL_I386_DIR32,
            positive_one_literal.name,
            0,
            (0x268, 0x2A1),
        ),
        (
            IMAGE_REL_I386_REL32,
            "?zMathUnprojectPointBatchZBuf@@"
            "YIXPBUzProjectedPoint@@PAUzVec3@@H@Z",
            0,
            (0x291, 0x2C7),
        ),
        (
            IMAGE_REL_I386_REL32,
            "?gwNodeSetRaycastable@zClass_Class@@"
            "YIHPAUzClass_NodePartial@@H@Z",
            0,
            (0x2D4, 0x2F0, 0x333, 0x352),
        ),
        (
            IMAGE_REL_I386_REL32,
            "?SetStopAfterFirstHit@zClass_cls_di@@YIXH@Z",
            0,
            (0x2FA,),
        ),
        (IMAGE_REL_I386_DIR32, "_g_Player_RuntimeDiScene", 0, (0x311,)),
        (
            IMAGE_REL_I386_REL32,
            "?RaycastSelectClosestHitBetweenPoints@zClass_cls_di@@"
            "YIHPAUzClass_NodePartial@@PBUzVec3@@1"
            "PAUPlayerProbeSampleCandidateBuffer@@@Z",
            0,
            (0x324,),
        ),
        (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x330,
            (0x367, 0x3A1, 0x3DA),
        ),
        (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x358,
            (0x36C,),
        ),
        (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x328,
            (0x372, 0x38F, 0x3C9),
        ),
        (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x32C,
            (0x378, 0x398, 0x3D2),
        ),
        (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x35C,
            (0x3AF,),
        ),
        (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x354,
            (0x3B8,),
        ),
        (
            IMAGE_REL_I386_REL32,
            "?SetImageBorrowedAndInvalidate@HudUiWidget@@"
            "QAEPAUzVidImagePartial@@PAU2@@Z",
            0,
            (0x3C3,),
        ),
        (
            IMAGE_REL_I386_REL32,
            "?GetRenderSection@zOpt@@YIPAUzOpt_ViewRectSection@@XZ",
            0,
            (0x3E2,),
        ),
        (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0xC10,
            (0x3F0, 0x40A, 0x45D, 0x46B),
        ),
        (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0xC14,
            (0x428, 0x442, 0x463, 0x473),
        ),
        (
            IMAGE_REL_I386_REL32,
            "?GetReplicateMode@zOpt@@YIHXZ",
            0,
            (0x498,),
        ),
        (
            IMAGE_REL_I386_REL32,
            "?SetTargetRect@zClipAlt@@YIXPBUzClipAltFloatRect@@H@Z",
            0,
            (0x4A3,),
        ),
    )
    relocation_by_offset: dict[int, tuple[int, str, int]] = {}
    for kind, symbol, addend, fact_offsets in relocation_fact_groups:
        for offset in fact_offsets:
            if offset in relocation_by_offset:
                raise ValueError(
                    "HUD reticle rebaseline contains duplicate governed "
                    f"relocation offset +0x{offset:x}"
                )
            relocation_by_offset[offset] = (kind, symbol, addend)
    if tuple(sorted(relocation_by_offset)) != relocation_offsets:
        raise ValueError(
            "HUD reticle rebaseline relocation census is internally incomplete"
        )
    relocation_specs = tuple(
        (offset, *relocation_by_offset[offset])
        for offset in relocation_offsets
    )
    actual_relocations = (
        tuple(
            (
                row.offset,
                row.type,
                row.symbol_name,
                struct.unpack_from("<I", caller.data, row.offset)[0],
            )
            for row in caller.relocations
        )
        if caller is not None
        and all(
            row.offset + 4 <= len(caller.data)
            for row in caller.relocations
        )
        else ()
    )
    superseded_wsi024_shape = (
        caller is not None
        and caller.data[:superseded_wsi024_prefix_end]
        == superseded_wsi024_prefix
        and caller.data[superseded_wsi024_tail_start:]
        == superseded_wsi024_tail
        and actual_relocations == superseded_wsi024_relocation_specs
    )
    if (
        caller is None
        or superseded_wsi024_shape
        or caller.symbol != caller_symbol
        or len(caller.data) != 0x4F0
        or len(caller.relocation_mask) != len(caller.data)
        or caller.data != exact_body
        or actual_relocations != relocation_specs
        or {
            offset
            for offset in range(len(caller.data))
            if caller.relocation_mask[offset]
        }
        != {
            offset
            for relocation_offset, _, _, _ in relocation_specs
            for offset in range(relocation_offset, relocation_offset + 4)
        }
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in caller.defined_external_data
        or _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL in caller.defined_external_data
    ):
        raise ValueError(
            "HUD reticle rebaseline rejects exact current body prefix, "
            "extent, relocation type/target/addend/order/mask, or alias drift"
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    index_by_offset = {
        offset: index
        for index, offset in enumerate(offsets)
        if offset is not None
    }
    call_specs = (
        (0x9C, 0x8E, 0xA4, "eax", 0x0C),
        (0xC8, 0xE0, 0xE5, "edx", 0x68),
        (0xFD, 0x102, 0x107, "eax", 0x68),
        (0x10A, 0x112, 0x11B, "edx", 0x64),
        (0x133, 0x138, 0x13D, "eax", 0x64),
        (0x164, 0x16A, 0x16F, "edx", 0x68),
        (0x17A, 0x185, 0x18A, "eax", 0x68),
        (0x193, 0x19B, 0x1A6, "edx", 0x64),
        (0x1B6, 0x1AF, 0x1C1, "eax", 0x64),
        (0x1CA, 0x1D8, 0x1DD, "edx", 0x64),
        (0x1E9, 0x1EE, 0x1F3, "eax", 0x68),
        (0x203, 0x1FC, 0x20B, "edx", 0x0C),
        (0x4B5, 0x4BD, 0x4C2, "edx", 0x60),
        (0x4D3, 0x4D9, 0x4DE, "eax", 0x60),
    )
    required_offsets = {
        0x53,
        0x5E,
        0x4F,
        0x51,
        0x63,
        0x65,
        0x67,
        0x69,
        0x6F,
        0x74,
        0x7A,
        0x7B,
        0x81,
        0x82,
        0x4BB,
        0x4D8,
        0x4E3,
        0x4EC,
        *(value for spec in call_specs for value in spec[:3]),
    }
    if required_offsets - set(index_by_offset):
        raise ValueError(
            "HUD reticle rebaseline rejects missing current instruction "
            "boundaries or superseded old-layout reuse"
        )
    tail_offsets = (
        0x497,
        0x49C,
        0x49E,
        0x4A2,
        0x4A7,
        0x4A9,
        0x4AA,
        0x4AB,
        0x4AC,
        0x4B2,
        0x4B5,
        0x4BB,
        0x4BD,
        0x4C2,
        0x4C5,
        0x4C7,
        0x4C8,
        0x4C9,
        0x4CA,
        0x4D0,
        0x4D3,
        0x4D8,
        0x4D9,
        0x4DE,
        0x4E1,
        0x4E2,
        0x4E3,
        0x4E5,
        0x4E6,
        0x4EC,
    )
    tail_indices = tuple(index_by_offset[offset] for offset in tail_offsets)
    if (
        tuple(
            offset
            for offset in offsets
            if offset is not None and offset >= guarded_tail_start
        )
        != tail_offsets
        or tail_indices
        != tuple(
            range(tail_indices[0], tail_indices[0] + len(tail_indices))
        )
    ):
        raise ValueError(
            "HUD reticle rebaseline rejects exact switch-tail/epilogue "
            "instruction extent or order"
        )
    complete_offsets = tuple(offset for offset in offsets if offset is not None)
    if (
        not complete_offsets
        or complete_offsets[-1] != 0x4EC
        or any(offset >= semantic_end for offset in complete_offsets)
        or caller.data[semantic_end:] != b"\x90"
    ):
        raise ValueError(
            "HUD reticle rebaseline rejects semantic extent or exact final "
            "COFF pad byte"
        )

    aggregate = _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
    aggregate_regex = re.escape(aggregate)
    for vptr_offset, receiver_offset, call_offset, register, slot in call_specs:
        vptr_operand = _cc_cfg._instruction_operand(
            candidate.instructions[index_by_offset[vptr_offset]]
        )
        receiver_operand = _cc_cfg._instruction_operand(
            candidate.instructions[index_by_offset[receiver_offset]]
        )
        call_operand = _cc_cfg._instruction_operand(
            candidate.instructions[index_by_offset[call_offset]]
        )
        destination, source = vptr_operand.split(",", 1)
        if (
            _cc_cfg._instruction_mnemonic(
                candidate.instructions[index_by_offset[vptr_offset]]
            )
            != "mov"
            or destination.strip().lower() != register
            or _cc_targets._exact_memory_expression(source)
            not in {f"{aggregate}+868", f"{aggregate}+0x364"}
            or re.fullmatch(
                rf"ecx\s*,\s*OFFSET\s+FLAT:{aggregate_regex}"
                r"\+(?:868|0x364)",
                receiver_operand,
                flags=re.IGNORECASE,
            )
            is None
            or _cc_cfg._instruction_mnemonic(
                candidate.instructions[index_by_offset[call_offset]]
            )
            != "call"
            or _cc_targets._exact_memory_expression(call_operand)
            not in {
                f"{register}+{slot}",
                f"{register}+0x{slot:x}",
            }
        ):
            raise ValueError(
                "HUD reticle rebaseline rejects vptr/receiver/register/slot "
                f"provenance at candidate +0x{call_offset:x}"
            )
        if [
            index
            for index in range(
                index_by_offset[vptr_offset],
                index_by_offset[call_offset],
            )
            if _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index], register
            )
        ] != [index_by_offset[vptr_offset]]:
            raise ValueError(
                "HUD reticle rebaseline rejects reaching-definition drift "
                f"at candidate +0x{call_offset:x}"
            )
        if _cc_cfg._cleanup_after(
            candidate.instructions,
            index_by_offset[call_offset],
        ) is not None:
            raise ValueError(
                "HUD reticle rebaseline rejects caller cleanup drift at "
                f"candidate +0x{call_offset:x}"
            )

    def normalized_operand(offset: int) -> str:
        return (
            _cc_cfg._instruction_operand(
                candidate.instructions[index_by_offset[offset]]
            )
            .lower()
            .replace(" ", "")
            .replace("ptr", "")
        )

    normalized_aggregate = aggregate.lower()
    if (
        normalized_operand(0x53) != "__ftol"
        or normalized_operand(0x5E) != "__ftol"
        or normalized_operand(0x4F) != "ebp,edx"
        or normalized_operand(0x51) != "edi,ecx"
        or normalized_operand(0x63) != "ecx,edi"
        or normalized_operand(0x65) != "edi,edi"
        or normalized_operand(0x67) != "ecx,edi"
        or normalized_operand(0x69)
        not in {
            f"dword{normalized_aggregate}+844,esi",
            f"dword{normalized_aggregate}+0x34c,esi",
        }
        or normalized_operand(0x6F)
        not in {
            f"dword{normalized_aggregate}+848,eax",
            f"dword{normalized_aggregate}+0x350,eax",
        }
        or normalized_operand(0x4BB) not in {"1", "0x1"}
        or normalized_operand(0x4D8) != "edi"
        or normalized_operand(0x4E3) != "eax,eax"
        or _cc_cfg._instruction_mnemonic(
            candidate.instructions[index_by_offset[0x4EC]]
        )
        != "ret"
        or normalized_operand(0x4EC) not in {"8", "0x8"}
    ):
        raise ValueError(
            "HUD reticle rebaseline rejects switch selector, projected "
            "coordinate stores, cold visibility arguments, or return shape"
        )
    branch_specs = {
        0x74: ("conditional", 0x4D3),
        0x7B: ("conditional", 0x4B5),
        0x82: ("conditional", 0x4E1),
        0xAE: ("conditional", 0xC8),
        0xF3: ("conditional", 0xFB),
        0xF9: ("unconditional", 0xFD),
        0x129: ("conditional", 0x131),
        0x12F: ("unconditional", 0x133),
        0x15E: ("conditional", 0x218),
        0x2DF: ("conditional", 0x2F4),
        0x33E: ("conditional", 0x356),
        0x358: ("conditional", 0x37E),
        0x37C: ("unconditional", 0x3BC),
        0x3B5: ("conditional", 0x3BC),
        0x3FD: ("conditional", 0x41F),
        0x417: ("conditional", 0x41F),
        0x435: ("conditional", 0x457),
        0x44F: ("conditional", 0x457),
    }
    actual_branch_specs = {
        offset: branch
        for index, instruction in enumerate(candidate.instructions)
        if offsets[index] is not None
        and (
            branch := _cc_cfg._exact_local_direct_branch(
                instruction,
                instruction_index=index,
                instruction_addresses=offsets,
                instruction_index_by_address=index_by_offset,
                source="cod",
                caller_start=0,
                caller_end=len(caller.data),
            )
        )
        is not None
        for offset in (offsets[index],)
    }
    if actual_branch_specs != {
        source: (kind, index_by_offset[target])
        for source, (kind, target) in branch_specs.items()
    }:
        raise ValueError(
            "HUD reticle rebaseline rejects exact complete-body CFG"
        )
    if (
        candidate.local_control_flow_indices
        or candidate.local_control_flow_targets
    ):
        raise ValueError(
            "HUD reticle rebaseline rejects switch dispatch or alternate "
            "entry in the complete guarded caller"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_expected,
    )
    expected_call_offsets = (
        0x53,
        0x5E,
        0xA4,
        0xC0,
        0xE5,
        0x107,
        0x11B,
        0x13D,
        0x146,
        0x156,
        0x16F,
        0x18A,
        0x1A6,
        0x1C1,
        0x1DD,
        0x1F3,
        0x20B,
        0x234,
        0x261,
        0x290,
        0x2C6,
        0x2D3,
        0x2EF,
        0x2F9,
        0x323,
        0x332,
        0x351,
        0x3C2,
        0x3E1,
        0x497,
        0x4A2,
        0x4C2,
        0x4DE,
    )
    if tuple(invocation_indices) != tuple(
        index_by_offset[offset] for offset in expected_call_offsets
    ):
        raise ValueError(
            "HUD reticle rebaseline rejects exact 33-invocation candidate "
            "order, duplicate use, or superseded layout callsites"
        )

    bridges = {
        f"0x{call_offset:x}": ReviewedExactIndirectStorageBridge(
            register=register,
            storage_identity=storage_identity,
            slot_displacement=slot,
            assembly_source="cod",
        )
        for _, _, call_offset, register, slot in call_specs
    }
    if len(bridges) != len(call_specs):
        raise ValueError(
            "HUD reticle rebaseline bridge composition contains a duplicate "
            "callsite"
        )
    return bridges


def _hud_ui_mgr_reticle_widget_candidate_vptr_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedExactIndirectStorageBridge]:
    """Bridge exact relocation-backed reticle widget calls."""
    from _recoil.call_contract.records import (
        ReviewedExactIndirectStorageBridge,
        StorageContainer,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != "0x411270":
        return {}

    caller_symbol_name = (
        "?UpdateTargetReticleFromCursor@HudUiMgr@@YIHHPAUzVec3@@MM@Z"
    )
    caller_symbol_id = "recoil:function:0x411270"
    caller_identity_expected = f"symbol:{caller_symbol_id}"
    caller_end_expected = "0x411710"
    caller_anchor = (
        "recoil:anchor:battlesport.hud."
        "updatetargetreticlefromcursor"
    )
    caller_symbol = document.collection("symbols").get(caller_symbol_id)
    caller_trace = (
        caller_symbol.get("source_traceability")
        if isinstance(caller_symbol, Mapping)
        else None
    )
    caller_name_matches = [
        (name, identity)
        for name, identity in indexes.by_candidate_name.items()
        if name.casefold() == caller_symbol_name.casefold()
    ]
    if (
        caller_identity != caller_identity_expected
        or normalize_address(caller_end_exclusive)
        != caller_end_expected
        or sorted(
            address
            for address, identity in indexes.by_address.items()
            if identity == caller_identity
        )
        != [normalized_start]
        or tuple(caller_name_matches)
        not in {
            (),
            ((caller_symbol_name, caller_identity_expected),),
        }
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_symbol, Mapping)
        or _cc_identity._symbol_identity(caller_symbol_id, caller_symbol)
        != caller_identity_expected
        or caller_symbol.get("binary") != "recoil"
        or caller_symbol.get("kind") != "function"
        or caller_symbol.get("pipeline_class") != "authored"
        or caller_symbol.get("extent_state") != "known"
        or normalize_address(str(caller_symbol.get("address", "")))
        != normalized_start
        or normalize_address(
            str(caller_symbol.get("end_exclusive", ""))
        )
        != caller_end_expected
        or caller_symbol.get("size") != 0x4A0
        or caller_symbol.get("navigation_name")
        != "HudUiMgr::UpdateTargetReticleFromCursor"
        or caller_symbol.get("logical_identity_key") not in {None, ""}
        or caller_symbol.get("icf_fold_status") not in {None, ""}
        or bool(caller_symbol.get("logical_aliases"))
        or not isinstance(caller_trace, Mapping)
        or caller_trace.get("state") != "resolved"
        or caller_trace.get("reason_code") not in {None, ""}
        or caller_trace.get("source_edges")
        != [
            {
                "anchor_id": caller_anchor,
                "emission_context": {
                    "translation_unit": "src/Battlesport/hud.cpp"
                },
                "evidence_ids": [],
                "relation": "defines",
            }
        ]
    ):
        raise ValueError(
            "HUD reticle-widget bridge requires the exact unaliased "
            "authored caller identity, extent, and source edge"
        )

    symbols = document.collection("symbols")
    storage_rows = document.collection("storage_contributions")
    targets = document.collection("verification_targets")
    aggregate_symbol = symbols.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID)
    aggregate_storage = storage_rows.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_STORAGE_ID)
    aggregate_target = targets.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_ID)
    aggregate_registration = (
        aggregate_target.get("registration")
        if isinstance(aggregate_target, Mapping)
        else None
    )
    aggregate_trace = (
        aggregate_symbol.get("source_traceability")
        if isinstance(aggregate_symbol, Mapping)
        else None
    )
    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    if (
        not isinstance(aggregate_symbol, Mapping)
        or aggregate_symbol.get("binary") != "recoil"
        or aggregate_symbol.get("kind") != "data"
        or aggregate_symbol.get("disposition") != "authored"
        or aggregate_symbol.get("extent_state") != "unknown"
        or normalize_address(str(aggregate_symbol.get("address", "")))
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
        or aggregate_symbol.get("navigation_name")
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_NAME
        or aggregate_symbol.get("output_section_id")
        != "recoil:section:.data"
        or aggregate_symbol.get("storage_contribution_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_STORAGE_ID]
        or aggregate_symbol.get("verification_target_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_ID]
        or not isinstance(aggregate_trace, Mapping)
        or aggregate_trace.get("state") != "resolved"
        or aggregate_trace.get("reason_code") not in {None, ""}
        or aggregate_trace.get("source_edges")
        != [
            {
                "anchor_id": (
                    "recoil:anchor:gamezrecoil-zui-zui-widgets-"
                    "g-huduimgr"
                ),
                "emission_context": {
                    "translation_unit": (
                        "src/GameZRecoil/zUI/zui_widgets.cpp"
                    )
                },
                "evidence_ids": [],
                "relation": "defines",
            }
        ]
        or not isinstance(aggregate_storage, Mapping)
        or aggregate_storage.get("binary") != "recoil"
        or aggregate_storage.get("kind") != "data-symbol"
        or aggregate_storage.get("output_section_id")
        != "recoil:section:.data"
        or aggregate_storage.get("overlap") != "none"
        or aggregate_storage.get("parent_contribution_id") is not None
        or aggregate_storage.get("symbol_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID]
        or aggregate_storage.get("owner_ids")
        != ["recoil:owner:hud_ui.hud_ui_mgr_data"]
        or aggregate_storage.get("reference")
        != {
            "address": _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS,
            "evidence_ids": [],
            "extent_state": "unknown",
        }
        or not isinstance(aggregate_target, Mapping)
        or aggregate_target.get("binary") != "recoil"
        or aggregate_target.get("kind") != "vc5"
        or aggregate_target.get("name") != "hud_ui_mgr_data"
        or aggregate_target.get("symbol_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID]
        or aggregate_target.get("unresolved_addresses") != []
        or not isinstance(aggregate_registration, Mapping)
        or aggregate_registration.get("manifest_path")
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_MANIFEST
        or aggregate_registration.get("source_from")
        != "src/GameZRecoil/zUI/zui.cpp"
        or aggregate_registration.get("data_addresses")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS]
        or indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        != aggregate_identity
        or [
            row
            for row in indexes.storage_containers
            if row.identity == aggregate_identity
        ]
        != [
            StorageContainer(
                address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS),
                address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
                + _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE,
                aggregate_identity,
            )
        ]
    ):
        raise ValueError(
            "HUD reticle-widget bridge requires the exact aggregate "
            "symbol, source, storage, target, and container authority"
        )

    storage_identity = (
        "load(storage:recoil:data:0x4e5ed0+0x364)"
    )
    setpos_retail_row = {
        "ordinal": 2,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": storage_identity,
        "slot_displacement": 0x0C,
        "cleanup_bytes": None,
    }
    atexit_retail_row = {
        "ordinal": 3,
        "form": "call",
        "dispatch": "direct",
        "identity_kind": "provider",
        "target_identity": "provider:recoil:function:0x4c60e0",
        "storage_identity": "",
        "slot_displacement": None,
        "cleanup_bytes": 4,
    }
    center_retail_rows = [
        {
            "ordinal": ordinal,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": storage_identity,
            "slot_displacement": slot,
            "cleanup_bytes": None,
        }
        for ordinal, slot in (
            (4, 0x68),
            (5, 0x68),
            (6, 0x64),
            (7, 0x64),
        )
    ]
    clip_predecessor_retail_rows = [
        {
            "ordinal": 8,
            "form": "call",
            "dispatch": "direct",
            "identity_kind": "direct",
            "target_identity": "symbol:recoil:function:0x408650",
            "storage_identity": "",
            "slot_displacement": None,
            "cleanup_bytes": None,
        },
        {
            "ordinal": 9,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "iat",
            "target_identity": "iat:IntersectRect",
            "storage_identity": "iat:IntersectRect",
            "slot_displacement": None,
            "cleanup_bytes": None,
        },
    ]
    clip_retail_rows = [
        {
            "ordinal": ordinal,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": storage_identity,
            "slot_displacement": slot,
            "cleanup_bytes": None,
        }
        for ordinal, slot in (
            (10, 0x68),
            (11, 0x68),
            (12, 0x64),
            (13, 0x64),
            (14, 0x64),
            (15, 0x68),
            (16, 0x0C),
        )
    ]
    visibility_retail_rows = [
        {
            "ordinal": 31,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": storage_identity,
            "slot_displacement": 0x60,
            "cleanup_bytes": None,
        },
        {
            "ordinal": 32,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": storage_identity,
            "slot_displacement": 0x60,
            "cleanup_bytes": None,
        },
    ]
    if (
        len(expected) <= 32
        or expected[2] != setpos_retail_row
        or expected[3] != atexit_retail_row
        or list(expected[4:8]) != center_retail_rows
        or list(expected[8:10]) != clip_predecessor_retail_rows
        or list(expected[10:17]) != clip_retail_rows
        or list(expected[31:33]) != visibility_retail_rows
        or any(
            sum(row == retail_row for row in expected) != 1
            for retail_row in (
                setpos_retail_row,
                atexit_retail_row,
                *center_retail_rows,
                *clip_predecessor_retail_rows,
                *clip_retail_rows,
                *visibility_retail_rows,
            )
        )
    ):
        raise ValueError(
            "HUD reticle-widget bridge requires the exact immutable retail "
            "ordinal-2 SetPos, ordinal-3 atexit, ordinal-4-through-7 "
            "GetCenterY/GetCenterX, ordinal-8 GetDisplaySection, ordinal-9 "
            "IntersectRect, ordinal-10-through-16 clip-block calls, and "
            "ordinal-31/32 SetVisible contracts"
        )

    return _hud_ui_mgr_reticle_widget_candidate_rebaseline_bridges(
        candidate,
        storage_identity=storage_identity,
        normalized_start=normalized_start,
        caller_end_expected=caller_end_expected,
    )

    caller = candidate.caller_definition
    bounded_start = 0x09
    bounded_end = 0x4F
    exact_body = bytes.fromhex(
        "33 ff 3b cf 8b ea 75 1c "
        "a1 64 03 00 00 57 b9 64 03 00 00 ff 50 60 "
        "33 c0 5f 5e 5d 81 c4 58 05 00 00 c2 08 00 "
        "83 f9 01 75 1d "
        "8b 15 64 03 00 00 51 b9 64 03 00 00 ff 52 60 "
        "33 c0 5f 5e 5d 81 c4 58 05 00 00 c2 08 00"
    )
    expected_relocations = (
        (
            0x12,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x18,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x34,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x3A,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
    )
    actual_relocations = (
        tuple(
            (
                row.offset,
                row.type,
                row.symbol_name,
                (
                    struct.unpack_from("<I", caller.data, row.offset)[0]
                    if (
                        caller is not None
                        and 0 <= row.offset <= len(caller.data) - 4
                    )
                    else None
                ),
            )
            for row in caller.relocations
            if bounded_start <= row.offset < bounded_end
        )
        if caller is not None
        else ()
    )
    if (
        caller is None
        or caller.symbol != caller_symbol_name
        or len(caller.data) != 0x4E0
        or len(caller.relocation_mask) != len(caller.data)
        or caller.data[bounded_start:bounded_end] != exact_body
        or actual_relocations != expected_relocations
        or {
            offset
            for offset in range(bounded_start, bounded_end)
            if caller.relocation_mask[offset]
        }
        != {
            offset
            for relocation_offset, _, _, _ in expected_relocations
            for offset in range(relocation_offset, relocation_offset + 4)
        }
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in caller.defined_external_data
    ):
        raise ValueError(
            "HUD reticle-widget bridge rejects caller symbol, bounded body, "
            "aggregate alias, DIR32 relocation/addend, or mask drift"
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    index_by_offset = {
        offset: index
        for index, offset in enumerate(offsets)
        if offset is not None
    }
    sequence_offsets = (
        0x09,
        0x0B,
        0x0D,
        0x0F,
        0x11,
        0x16,
        0x17,
        0x1C,
        0x1F,
        0x21,
        0x22,
        0x23,
        0x24,
        0x2A,
        0x2D,
        0x30,
        0x32,
        0x38,
        0x39,
        0x3E,
        0x41,
        0x43,
        0x44,
        0x45,
        0x46,
        0x4C,
    )
    if (
        0x4F not in index_by_offset
        or set(sequence_offsets) - set(index_by_offset)
        or {
            offset
            for offset in offsets
            if (
                offset is not None
                and bounded_start <= offset < bounded_end
            )
        }
        != set(sequence_offsets)
    ):
        raise ValueError(
            "HUD reticle-widget bridge requires the exact bounded "
            "instruction offsets and mode-2 continuation"
        )
    sequence_indices = tuple(
        index_by_offset[offset] for offset in sequence_offsets
    )
    if sequence_indices != tuple(
        range(sequence_indices[0], sequence_indices[0] + len(sequence_indices))
    ):
        raise ValueError(
            "HUD reticle-widget bridge rejects missing, duplicate, "
            "reordered, or noncontiguous instructions"
        )
    sequence = tuple(
        candidate.instructions[index] for index in sequence_indices
    )
    if (
        tuple(instruction.bytes for instruction in sequence)
        != (
            ("33", "ff"),
            ("3b", "cf"),
            ("8b", "ea"),
            ("75", "1c"),
            ("a1", "64", "03", "00", "00"),
            ("57",),
            ("b9", "64", "03", "00", "00"),
            ("ff", "50", "60"),
            ("33", "c0"),
            ("5f",),
            ("5e",),
            ("5d",),
            ("81", "c4", "58", "05", "00", "00"),
            ("c2", "08", "00"),
            ("83", "f9", "01"),
            ("75", "1d"),
            ("8b", "15", "64", "03", "00", "00"),
            ("51",),
            ("b9", "64", "03", "00", "00"),
            ("ff", "52", "60"),
            ("33", "c0"),
            ("5f",),
            ("5e",),
            ("5d",),
            ("81", "c4", "58", "05", "00", "00"),
            ("c2", "08", "00"),
        )
        or tuple(
            _cc_cfg._instruction_mnemonic(instruction) for instruction in sequence
        )
        != (
            "xor",
            "cmp",
            "mov",
            "jne",
            "mov",
            "push",
            "mov",
            "call",
            "xor",
            "pop",
            "pop",
            "pop",
            "add",
            "ret",
            "cmp",
            "jne",
            "mov",
            "push",
            "mov",
            "call",
            "xor",
            "pop",
            "pop",
            "pop",
            "add",
            "ret",
        )
    ):
        raise ValueError(
            "HUD reticle-widget bridge rejects opcode, body, or order drift"
        )

    def exact_operand(offset: int) -> str:
        return _cc_cfg._instruction_operand(
            candidate.instructions[index_by_offset[offset]]
        ).strip()

    def exact_memory_source(offset: int) -> tuple[str, str]:
        destination, source = exact_operand(offset).split(",", 1)
        return destination.strip().lower(), _cc_targets._exact_memory_expression(source)

    aggregate = _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
    aggregate_regex = re.escape(aggregate)
    if (
        re.fullmatch(
            r"edi\s*,\s*edi",
            exact_operand(0x09),
            flags=re.IGNORECASE,
        )
        is None
        or re.fullmatch(
            r"ecx\s*,\s*edi",
            exact_operand(0x0B),
            flags=re.IGNORECASE,
        )
        is None
        or exact_memory_source(0x11)
        not in {
            ("eax", f"{aggregate}+868"),
            ("eax", f"{aggregate}+0x364"),
        }
        or exact_operand(0x16).lower() != "edi"
        or re.fullmatch(
            rf"ecx\s*,\s*OFFSET\s+FLAT:{aggregate_regex}"
            r"\+(?:868|0x364)",
            exact_operand(0x17),
            flags=re.IGNORECASE,
        )
        is None
        or _cc_targets._exact_memory_expression(exact_operand(0x1C))
        not in {"eax+96", "eax+0x60"}
        or re.fullmatch(
            r"ecx\s*,\s*(?:1|0x1)",
            exact_operand(0x2D),
            flags=re.IGNORECASE,
        )
        is None
        or exact_memory_source(0x32)
        not in {
            ("edx", f"{aggregate}+868"),
            ("edx", f"{aggregate}+0x364"),
        }
        or exact_operand(0x38).lower() != "ecx"
        or re.fullmatch(
            rf"ecx\s*,\s*OFFSET\s+FLAT:{aggregate_regex}"
            r"\+(?:868|0x364)",
            exact_operand(0x39),
            flags=re.IGNORECASE,
        )
        is None
        or _cc_targets._exact_memory_expression(exact_operand(0x3E))
        not in {"edx+96", "edx+0x60"}
    ):
        raise ValueError(
            "HUD reticle-widget bridge rejects zero/one argument, vptr "
            "register, concrete receiver, or slot-0x60 semantic drift"
        )

    cfg_specs = {
        0x0F: ("conditional", 0x2D),
        0x30: ("conditional", 0x4F),
    }
    exact_cfg = {
        source: _cc_cfg._exact_local_direct_branch(
            candidate.instructions[index_by_offset[source]],
            instruction_index=index_by_offset[source],
            instruction_addresses=offsets,
            instruction_index_by_address=index_by_offset,
            source="cod",
            caller_start=0,
            caller_end=0x4E0,
        )
        for source in cfg_specs
    }
    scoped_indices = frozenset(sequence_indices)
    incoming_scoped_targets = {
        (source, target)
        for source, targets_ in candidate.local_control_flow_targets.items()
        for target in targets_
        if target in scoped_indices
    }
    if (
        any(
            exact_cfg[source]
            != (kind, index_by_offset[target])
            for source, (kind, target) in cfg_specs.items()
        )
        or bool(candidate.local_control_flow_indices & scoped_indices)
        or incoming_scoped_targets
    ):
        raise ValueError(
            "HUD reticle-widget bridge requires the exact mode-0/mode-1 "
            "early-return CFG with no alternate entry"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_expected,
    )
    call_indices = (
        index_by_offset[0x1C],
        index_by_offset[0x3E],
    )
    if tuple(invocation_indices[:2]) != call_indices:
        raise ValueError(
            "HUD reticle-widget bridge requires the exact first/second "
            "candidate invocation order"
        )
    if (
        [
            index
            for index in range(
                index_by_offset[0x09],
                index_by_offset[0x1C],
            )
            if _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index], "edi"
            )
        ]
        != [index_by_offset[0x09]]
        or [
            index
            for index in range(
                index_by_offset[0x11],
                index_by_offset[0x1C],
            )
            if _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index], "eax"
            )
        ]
        != [index_by_offset[0x11]]
        or [
            index
            for index in range(
                index_by_offset[0x32],
                index_by_offset[0x3E],
            )
            if _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index], "edx"
            )
        ]
        != [index_by_offset[0x32]]
        or _cc_cfg._cleanup_after(
            candidate.instructions, index_by_offset[0x1C]
        )
        is not None
        or _cc_cfg._cleanup_after(
            candidate.instructions, index_by_offset[0x3E]
        )
        is not None
    ):
        raise ValueError(
            "HUD reticle-widget bridge rejects argument/vptr reaching "
            "definitions or cleanup drift"
        )
    for call_offset, result_offset, return_offset in (
        (0x1C, 0x1F, 0x2A),
        (0x3E, 0x41, 0x4C),
    ):
        call_index = index_by_offset[call_offset]
        result_index = index_by_offset[result_offset]
        return_index = index_by_offset[return_offset]
        if (
            result_index != call_index + 1
            or return_index <= result_index
            or candidate.instructions[result_index].bytes != ("33", "c0")
            or _cc_cfg._instruction_mnemonic(
                candidate.instructions[result_index]
            )
            != "xor"
            or re.fullmatch(
                r"eax\s*,\s*eax",
                _cc_cfg._instruction_operand(
                    candidate.instructions[result_index]
                ),
                flags=re.IGNORECASE,
            )
            is None
            or not _cc_cfg._exact_return_terminates(
                candidate.instructions[return_index]
            )
        ):
            raise ValueError(
                "HUD reticle-widget bridge requires discarded call results "
                "and exact zero-return early exits"
            )

    setpos_start = 0x4F
    setpos_end = 0xD0
    setpos_body = bytes.fromhex(
        "83 f9 02 0f 85 77 04 00 00 "
        "d9 84 24 68 05 00 00 d8 25 00 00 00 00 "
        "d8 0d 44 03 00 00 d8 05 3c 03 00 00 "
        "d9 5c 24 10 "
        "d9 84 24 6c 05 00 00 d8 25 00 00 00 00 "
        "d8 0d 48 03 00 00 d8 05 40 03 00 00 "
        "d9 5c 24 0c d9 44 24 10 e8 00 00 00 00 "
        "d9 44 24 0c 8b f0 e8 00 00 00 00 "
        "8b 15 38 03 00 00 a3 50 03 00 00 2b c2 "
        "8b 15 34 03 00 00 89 35 4c 03 00 00 50 "
        "a1 64 03 00 00 2b f2 56 "
        "b9 64 03 00 00 ff 50 0c"
    )
    setpos_relocations = (
        (0x61, IMAGE_REL_I386_DIR32, "$T88356", 0),
        (
            0x67,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x344,
        ),
        (
            0x6D,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x33C,
        ),
        (0x7E, IMAGE_REL_I386_DIR32, "$T88356", 0),
        (
            0x84,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x348,
        ),
        (
            0x8A,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x340,
        ),
        (0x97, IMAGE_REL_I386_REL32, "__ftol", 0),
        (0xA2, IMAGE_REL_I386_REL32, "__ftol", 0),
        (
            0xA8,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x338,
        ),
        (
            0xAD,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x350,
        ),
        (
            0xB5,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x334,
        ),
        (
            0xBB,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x34C,
        ),
        (
            0xC1,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0xC9,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
    )
    actual_setpos_relocations = tuple(
        (
            row.offset,
            row.type,
            row.symbol_name,
            (
                struct.unpack_from("<I", caller.data, row.offset)[0]
                if 0 <= row.offset <= len(caller.data) - 4
                else None
            ),
        )
        for row in caller.relocations
        if setpos_start <= row.offset < setpos_end
    )
    if (
        caller.data[setpos_start:setpos_end] != setpos_body
        or actual_setpos_relocations != setpos_relocations
        or {
            offset
            for offset in range(setpos_start, setpos_end)
            if caller.relocation_mask[offset]
        }
        != {
            offset
            for relocation_offset, _, _, _ in setpos_relocations
            for offset in range(relocation_offset, relocation_offset + 4)
        }
    ):
        raise ValueError(
            "HUD reticle SetPos bridge rejects coordinate body, DIR32/REL32 "
            "relocation target/addend/order, or mask drift"
        )

    setpos_offsets = (
        0x4F,
        0x52,
        0x58,
        0x5F,
        0x65,
        0x6B,
        0x71,
        0x75,
        0x7C,
        0x82,
        0x88,
        0x8E,
        0x92,
        0x96,
        0x9B,
        0x9F,
        0xA1,
        0xA6,
        0xAC,
        0xB1,
        0xB3,
        0xB9,
        0xBF,
        0xC0,
        0xC5,
        0xC7,
        0xC8,
        0xCD,
    )
    if (
        0xD0 not in index_by_offset
        or 0x4CF not in index_by_offset
        or set(setpos_offsets) - set(index_by_offset)
        or {
            offset
            for offset in offsets
            if (
                offset is not None
                and setpos_start <= offset < setpos_end
            )
        }
        != set(setpos_offsets)
    ):
        raise ValueError(
            "HUD reticle SetPos bridge requires the exact coordinate "
            "instruction offsets, next result clobber, and mode guard target"
        )
    setpos_indices = tuple(
        index_by_offset[offset] for offset in setpos_offsets
    )
    if setpos_indices != tuple(
        range(setpos_indices[0], setpos_indices[0] + len(setpos_indices))
    ):
        raise ValueError(
            "HUD reticle SetPos bridge rejects missing, duplicate, reordered, "
            "or noncontiguous coordinate instructions"
        )
    for position, offset in enumerate(setpos_offsets):
        next_offset = (
            setpos_offsets[position + 1]
            if position + 1 < len(setpos_offsets)
            else setpos_end
        )
        instruction = candidate.instructions[index_by_offset[offset]]
        if bytes(int(item, 16) for item in instruction.bytes) != caller.data[
            offset:next_offset
        ]:
            raise ValueError(
                "HUD reticle SetPos bridge rejects COD/object instruction "
                f"byte disagreement at 0x{offset:x}"
            )
    setpos_mnemonics = (
        "cmp",
        "jne",
        "fld",
        "fsub",
        "fmul",
        "fadd",
        "fstp",
        "fld",
        "fsub",
        "fmul",
        "fadd",
        "fstp",
        "fld",
        "call",
        "fld",
        "mov",
        "call",
        "mov",
        "mov",
        "sub",
        "mov",
        "mov",
        "push",
        "mov",
        "sub",
        "push",
        "mov",
        "call",
    )
    if tuple(
        _cc_cfg._instruction_mnemonic(candidate.instructions[index_by_offset[offset]])
        for offset in setpos_offsets
    ) != setpos_mnemonics:
        raise ValueError(
            "HUD reticle SetPos bridge rejects coordinate opcode/order drift"
        )

    def setpos_operand(offset: int) -> str:
        return _cc_cfg._instruction_operand(
            candidate.instructions[index_by_offset[offset]]
        ).strip()

    def setpos_memory(offset: int) -> str:
        operand = setpos_operand(offset)
        destination = operand.split(",", 1)[0] if "," in operand else operand
        return _cc_targets._exact_memory_expression(destination)

    expected_memory_operands = {
        0x5F: "$T88356",
        0x65: f"{aggregate}+836",
        0x6B: f"{aggregate}+828",
        0x7C: "$T88356",
        0x82: f"{aggregate}+840",
        0x88: f"{aggregate}+832",
    }
    exact_stack_operands = {
        0x58: "dword _normalizedX$[esp+1376]",
        0x71: "dword _screenX$[esp+1380]",
        0x75: "dword _normalizedY$[esp+1376]",
        0x8E: "dword _screenY$[esp+1380]",
        0x92: "dword _screenX$[esp+1380]",
        0x9B: "dword _screenY$[esp+1380]",
    }
    if (
        re.fullmatch(
            r"ecx\s*,\s*(?:2|0x2)",
            setpos_operand(0x4F),
            flags=re.IGNORECASE,
        )
        is None
        or any(
            setpos_operand(offset).casefold()
            != expected.casefold()
            for offset, expected in exact_stack_operands.items()
        )
        or any(
            setpos_memory(offset).casefold()
            != expected.casefold()
            for offset, expected in expected_memory_operands.items()
        )
        or setpos_operand(0x96) != "__ftol"
        or setpos_operand(0x9F).lower().replace(" ", "") != "esi,eax"
        or setpos_operand(0xA1) != "__ftol"
        or exact_memory_source(0xA6)
        not in {
            ("edx", f"{aggregate}+824"),
            ("edx", f"{aggregate}+0x338"),
        }
        or setpos_memory(0xAC)
        not in {f"{aggregate}+848", f"{aggregate}+0x350"}
        or setpos_operand(0xB1).lower().replace(" ", "") != "eax,edx"
        or exact_memory_source(0xB3)
        not in {
            ("edx", f"{aggregate}+820"),
            ("edx", f"{aggregate}+0x334"),
        }
        or setpos_memory(0xB9)
        not in {f"{aggregate}+844", f"{aggregate}+0x34c"}
        or setpos_operand(0xBF).lower() != "eax"
        or exact_memory_source(0xC0)
        not in {
            ("eax", f"{aggregate}+868"),
            ("eax", f"{aggregate}+0x364"),
        }
        or setpos_operand(0xC5).lower().replace(" ", "") != "esi,edx"
        or setpos_operand(0xC7).lower() != "esi"
        or re.fullmatch(
            rf"ecx\s*,\s*OFFSET\s+FLAT:{aggregate_regex}"
            r"\+(?:868|0x364)",
            setpos_operand(0xC8),
            flags=re.IGNORECASE,
        )
        is None
        or _cc_targets._exact_memory_expression(setpos_operand(0xCD))
        not in {"eax+12", "eax+0xc"}
    ):
        raise ValueError(
            "HUD reticle SetPos bridge rejects normalized coordinate, "
            "half-width/half-height argument, EAX-vptr, ECX-receiver, or "
            "slot-0x0c provenance drift"
        )

    setpos_cfg = _cc_cfg._exact_local_direct_branch(
        candidate.instructions[index_by_offset[0x52]],
        instruction_index=index_by_offset[0x52],
        instruction_addresses=offsets,
        instruction_index_by_address=index_by_offset,
        source="cod",
        caller_start=0,
        caller_end=0x4E0,
    )
    setpos_scoped_indices = frozenset(setpos_indices)
    if (
        setpos_cfg != ("conditional", index_by_offset[0x4CF])
        or bool(
            candidate.local_control_flow_indices & setpos_scoped_indices
        )
        or any(
            target in setpos_scoped_indices
            for targets_ in candidate.local_control_flow_targets.values()
            for target in targets_
        )
    ):
        raise ValueError(
            "HUD reticle SetPos bridge requires the exact mode-2 guard CFG "
            "with no alternate coordinate-block entry"
        )
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_expected,
    )
    setpos_call_order = tuple(
        index_by_offset[offset]
        for offset in (0x1C, 0x3E, 0x96, 0xA1, 0xCD)
    )
    if (
        tuple(invocation_indices[:5]) != setpos_call_order
        or _cc_cfg._cleanup_after(
            candidate.instructions,
            index_by_offset[0xCD],
        )
        is not None
    ):
        raise ValueError(
            "HUD reticle SetPos bridge rejects invocation order or caller "
            "cleanup drift"
        )
    next_invocation = (
        invocation_indices[5]
        if len(invocation_indices) > 5
        else len(candidate.instructions)
    )
    result_clobbered = False
    for instruction in candidate.instructions[
        index_by_offset[0xCD] + 1:next_invocation
    ]:
        operand = _cc_cfg._instruction_operand(instruction).lower()
        if re.search(r"\beax\b", operand):
            destination = (
                operand.split(",", 1)[0].strip()
                if "," in operand
                else ""
            )
            if destination != "eax":
                raise ValueError(
                    "HUD reticle SetPos bridge rejects call-result "
                    "consumption"
                )
        if _cc_cfg._instruction_may_clobber_register(instruction, "eax"):
            result_clobbered = True
            break
    if not result_clobbered:
        raise ValueError(
            "HUD reticle SetPos bridge requires a proven discarded call "
            "result before the next invocation"
        )

    get_center_y_start = 0xF1
    get_center_y_end = 0x115
    get_center_y_body = bytes.fromhex(
        "8b 15 64 03 00 00 "
        "33 c9 "
        "89 4c 24 34 "
        "89 7c 24 30 "
        "89 4c 24 38 "
        "89 4c 24 3c "
        "b9 64 03 00 00 "
        "ff 52 68 "
        "89 44 24 34"
    )
    get_center_y_relocations = (
        (
            0xF3,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x10A,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
    )
    actual_get_center_y_relocations = tuple(
        (
            row.offset,
            row.type,
            row.symbol_name,
            (
                struct.unpack_from("<I", caller.data, row.offset)[0]
                if 0 <= row.offset <= len(caller.data) - 4
                else None
            ),
        )
        for row in caller.relocations
        if get_center_y_start <= row.offset < get_center_y_end
    )
    if (
        caller.data[get_center_y_start:get_center_y_end]
        != get_center_y_body
        or actual_get_center_y_relocations
        != get_center_y_relocations
        or {
            offset
            for offset in range(
                get_center_y_start, get_center_y_end
            )
            if caller.relocation_mask[offset]
        }
        != {
            offset
            for relocation_offset, _, _, _ in get_center_y_relocations
            for offset in range(relocation_offset, relocation_offset + 4)
        }
    ):
        raise ValueError(
            "HUD reticle GetCenterY bridge rejects exact body, "
            "g_HudUiMgr+0x364 DIR32 relocation/addend/order, or mask drift"
        )

    get_center_y_offsets = (
        0xF1,
        0xF7,
        0xF9,
        0xFD,
        0x101,
        0x105,
        0x109,
        0x10E,
        0x111,
    )
    neighboring_center_call_offsets = (0x130, 0x144, 0x166)
    required_offsets = {
        0xD7,
        *get_center_y_offsets,
        *neighboring_center_call_offsets,
    }
    if (
        required_offsets - set(index_by_offset)
        or {
            offset
            for offset in offsets
            if (
                offset is not None
                and get_center_y_start
                <= offset
                < get_center_y_end
            )
        }
        != set(get_center_y_offsets)
    ):
        raise ValueError(
            "HUD reticle GetCenterY bridge requires the exact bounded "
            "instruction offsets and neighboring center-call order"
        )
    get_center_y_indices = tuple(
        index_by_offset[offset] for offset in get_center_y_offsets
    )
    if get_center_y_indices != tuple(
        range(
            get_center_y_indices[0],
            get_center_y_indices[0] + len(get_center_y_indices),
        )
    ):
        raise ValueError(
            "HUD reticle GetCenterY bridge rejects missing, duplicate, "
            "reordered, or noncontiguous instructions"
        )
    for position, offset in enumerate(get_center_y_offsets):
        next_offset = (
            get_center_y_offsets[position + 1]
            if position + 1 < len(get_center_y_offsets)
            else get_center_y_end
        )
        instruction = candidate.instructions[index_by_offset[offset]]
        if bytes(int(item, 16) for item in instruction.bytes) != caller.data[
            offset:next_offset
        ]:
            raise ValueError(
                "HUD reticle GetCenterY bridge rejects COD/object byte "
                f"disagreement at 0x{offset:x}"
            )
    if tuple(
        _cc_cfg._instruction_mnemonic(
            candidate.instructions[index_by_offset[offset]]
        )
        for offset in get_center_y_offsets
    ) != (
        "mov",
        "xor",
        "mov",
        "mov",
        "mov",
        "mov",
        "mov",
        "call",
        "mov",
    ):
        raise ValueError(
            "HUD reticle GetCenterY bridge rejects opcode/order drift"
        )

    def get_center_y_operand(offset: int) -> str:
        return _cc_cfg._instruction_operand(
            candidate.instructions[index_by_offset[offset]]
        ).strip()

    def get_center_y_memory_source(
        offset: int,
    ) -> tuple[str, str]:
        destination, source = get_center_y_operand(offset).split(",", 1)
        return (
            destination.strip().lower(),
            _cc_targets._exact_memory_expression(source),
        )

    if (
        get_center_y_memory_source(0xF1)
        not in {
            ("edx", f"{aggregate}+868"),
            ("edx", f"{aggregate}+0x364"),
        }
        or get_center_y_operand(0xF7).lower().replace(" ", "")
        != "ecx,ecx"
        or get_center_y_operand(0xF9).lower().replace(" ", "")
        != "dword_reticlebounds$[esp+1384],ecx"
        or get_center_y_operand(0xFD).lower().replace(" ", "")
        != "dword_reticlebounds$[esp+1380],edi"
        or get_center_y_operand(0x101).lower().replace(" ", "")
        != "dword_reticlebounds$[esp+1388],ecx"
        or get_center_y_operand(0x105).lower().replace(" ", "")
        != "dword_reticlebounds$[esp+1392],ecx"
        or re.fullmatch(
            rf"ecx\s*,\s*OFFSET\s+FLAT:{aggregate_regex}"
            r"\+(?:868|0x364)",
            get_center_y_operand(0x109),
            flags=re.IGNORECASE,
        )
        is None
        or _cc_targets._exact_memory_expression(
            get_center_y_operand(0x10E)
        )
        not in {"edx+104", "edx+0x68"}
        or get_center_y_operand(0x111).lower().replace(" ", "")
        != "dword_reticlebounds$[esp+1384],eax"
    ):
        raise ValueError(
            "HUD reticle GetCenterY bridge rejects EDX-vptr provenance, "
            "RECT initialization/result, concrete ECX receiver, or "
            "slot-0x68 drift"
        )

    get_center_y_cfg = _cc_cfg._exact_local_direct_branch(
        candidate.instructions[index_by_offset[0xD7]],
        instruction_index=index_by_offset[0xD7],
        instruction_addresses=offsets,
        instruction_index_by_address=index_by_offset,
        source="cod",
        caller_start=0,
        caller_end=0x4E0,
    )
    scoped_get_center_y_indices = frozenset(get_center_y_indices)
    incoming_get_center_y_targets = {
        (source, target)
        for source, targets_ in candidate.local_control_flow_targets.items()
        for target in targets_
        if target in scoped_get_center_y_indices
    }
    if (
        get_center_y_cfg
        != ("conditional", index_by_offset[0xF1])
        or bool(
            candidate.local_control_flow_indices
            & scoped_get_center_y_indices
        )
        or bool(
            incoming_get_center_y_targets
            - {(index_by_offset[0xD7], index_by_offset[0xF1])}
        )
    ):
        raise ValueError(
            "HUD reticle GetCenterY bridge requires the exact atexit-guard "
            "fallthrough CFG with no alternate entry"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_expected,
    )
    reviewed_candidate_order = tuple(
        index_by_offset[offset]
        for offset in (
            0xCD,
            0xE9,
            0x10E,
            0x130,
            0x144,
            0x166,
        )
    )
    if (
        tuple(invocation_indices[4:10]) != reviewed_candidate_order
        or [
            index
            for index in range(
                index_by_offset[0xF1],
                index_by_offset[0x10E],
            )
            if _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index], "edx"
            )
        ]
        != [index_by_offset[0xF1]]
        or _cc_cfg._cleanup_after(
            candidate.instructions,
            index_by_offset[0x10E],
        )
        is not None
    ):
        raise ValueError(
            "HUD reticle GetCenterY bridge rejects SetPos/atexit/center-call "
            "order, EDX reaching definition, or cleanup drift"
        )

    center_tail_start = 0x115
    center_tail_end = 0x16F
    center_tail_body = bytes.fromhex(
        "a1 a0 03 00 00 3b c7 74 06 0f bf 70 06 eb 02 33 f6 "
        "a1 64 03 00 00 b9 64 03 00 00 ff 50 68 "
        "8b 15 64 03 00 00 03 c6 b9 64 03 00 00 "
        "89 44 24 3c ff 52 64 89 44 24 30 "
        "a1 a0 03 00 00 3b c7 74 06 0f bf 70 04 eb 02 33 f6 "
        "a1 64 03 00 00 b9 64 03 00 00 ff 50 64 "
        "03 c6 89 44 24 38"
    )
    center_tail_relocations = (
        (
            0x116,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x3A0,
        ),
        (
            0x127,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x12C,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x135,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x13C,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x14C,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x3A0,
        ),
        (
            0x15D,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x162,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
    )
    actual_center_tail_relocations = tuple(
        (
            row.offset,
            row.type,
            row.symbol_name,
            (
                struct.unpack_from("<I", caller.data, row.offset)[0]
                if 0 <= row.offset <= len(caller.data) - 4
                else None
            ),
        )
        for row in caller.relocations
        if center_tail_start <= row.offset < center_tail_end
    )
    if (
        caller.data[center_tail_start:center_tail_end]
        != center_tail_body
        or actual_center_tail_relocations != center_tail_relocations
        or {
            offset
            for offset in range(center_tail_start, center_tail_end)
            if caller.relocation_mask[offset]
        }
        != {
            offset
            for relocation_offset, _, _, _ in center_tail_relocations
            for offset in range(relocation_offset, relocation_offset + 4)
        }
    ):
        raise ValueError(
            "HUD reticle center-tail bridge rejects exact contiguous body, "
            "g_HudUiMgr image/vptr DIR32 relocation/addend/order, or mask "
            "drift"
        )

    center_tail_offsets = (
        0x115,
        0x11A,
        0x11C,
        0x11E,
        0x122,
        0x124,
        0x126,
        0x12B,
        0x130,
        0x133,
        0x139,
        0x13B,
        0x140,
        0x144,
        0x147,
        0x14B,
        0x150,
        0x152,
        0x154,
        0x158,
        0x15A,
        0x15C,
        0x161,
        0x166,
        0x169,
        0x16B,
    )
    if (
        0x16F not in index_by_offset
        or set(center_tail_offsets) - set(index_by_offset)
        or {
            offset
            for offset in offsets
            if (
                offset is not None
                and center_tail_start <= offset < center_tail_end
            )
        }
        != set(center_tail_offsets)
    ):
        raise ValueError(
            "HUD reticle center-tail bridge requires the exact contiguous "
            "instruction offsets and next-call boundary"
        )
    center_tail_indices = tuple(
        index_by_offset[offset] for offset in center_tail_offsets
    )
    if center_tail_indices != tuple(
        range(
            center_tail_indices[0],
            center_tail_indices[0] + len(center_tail_indices),
        )
    ):
        raise ValueError(
            "HUD reticle center-tail bridge rejects missing, duplicate, "
            "reordered, or noncontiguous instructions"
        )
    for position, offset in enumerate(center_tail_offsets):
        next_offset = (
            center_tail_offsets[position + 1]
            if position + 1 < len(center_tail_offsets)
            else center_tail_end
        )
        instruction = candidate.instructions[index_by_offset[offset]]
        if bytes(int(item, 16) for item in instruction.bytes) != caller.data[
            offset:next_offset
        ]:
            raise ValueError(
                "HUD reticle center-tail bridge rejects COD/object byte "
                f"disagreement at 0x{offset:x}"
            )
    if tuple(
        _cc_cfg._instruction_mnemonic(
            candidate.instructions[index_by_offset[offset]]
        )
        for offset in center_tail_offsets
    ) != (
        "mov",
        "cmp",
        "je",
        "movsx",
        "jmp",
        "xor",
        "mov",
        "mov",
        "call",
        "mov",
        "add",
        "mov",
        "mov",
        "call",
        "mov",
        "mov",
        "cmp",
        "je",
        "movsx",
        "jmp",
        "xor",
        "mov",
        "mov",
        "call",
        "add",
        "mov",
    ):
        raise ValueError(
            "HUD reticle center-tail bridge rejects opcode/order drift"
        )

    def center_tail_operand(offset: int) -> str:
        return _cc_cfg._instruction_operand(
            candidate.instructions[index_by_offset[offset]]
        ).strip()

    def center_tail_memory_source(
        offset: int,
    ) -> tuple[str, str]:
        destination, source = center_tail_operand(offset).split(",", 1)
        return (
            destination.strip().lower(),
            _cc_targets._exact_memory_expression(source),
        )

    if (
        center_tail_memory_source(0x115)
        not in {
            ("eax", f"{aggregate}+928"),
            ("eax", f"{aggregate}+0x3a0"),
        }
        or center_tail_operand(0x11A).lower().replace(" ", "")
        != "eax,edi"
        or center_tail_operand(0x11E).lower().replace(" ", "")
        not in {"esi,word[eax+6]", "esi,word[eax+0x6]"}
        or center_tail_operand(0x124).lower().replace(" ", "")
        != "esi,esi"
        or center_tail_memory_source(0x126)
        not in {
            ("eax", f"{aggregate}+868"),
            ("eax", f"{aggregate}+0x364"),
        }
        or re.fullmatch(
            rf"ecx\s*,\s*OFFSET\s+FLAT:{aggregate_regex}"
            r"\+(?:868|0x364)",
            center_tail_operand(0x12B),
            flags=re.IGNORECASE,
        )
        is None
        or _cc_targets._exact_memory_expression(center_tail_operand(0x130))
        not in {"eax+104", "eax+0x68"}
        or center_tail_memory_source(0x133)
        not in {
            ("edx", f"{aggregate}+868"),
            ("edx", f"{aggregate}+0x364"),
        }
        or center_tail_operand(0x139).lower().replace(" ", "")
        != "eax,esi"
        or re.fullmatch(
            rf"ecx\s*,\s*OFFSET\s+FLAT:{aggregate_regex}"
            r"\+(?:868|0x364)",
            center_tail_operand(0x13B),
            flags=re.IGNORECASE,
        )
        is None
        or center_tail_operand(0x140).lower().replace(" ", "")
        != "dword_reticlebounds$[esp+1392],eax"
        or _cc_targets._exact_memory_expression(center_tail_operand(0x144))
        not in {"edx+100", "edx+0x64"}
        or center_tail_operand(0x147).lower().replace(" ", "")
        != "dword_reticlebounds$[esp+1380],eax"
        or center_tail_memory_source(0x14B)
        not in {
            ("eax", f"{aggregate}+928"),
            ("eax", f"{aggregate}+0x3a0"),
        }
        or center_tail_operand(0x150).lower().replace(" ", "")
        != "eax,edi"
        or center_tail_operand(0x154).lower().replace(" ", "")
        not in {"esi,word[eax+4]", "esi,word[eax+0x4]"}
        or center_tail_operand(0x15A).lower().replace(" ", "")
        != "esi,esi"
        or center_tail_memory_source(0x15C)
        not in {
            ("eax", f"{aggregate}+868"),
            ("eax", f"{aggregate}+0x364"),
        }
        or re.fullmatch(
            rf"ecx\s*,\s*OFFSET\s+FLAT:{aggregate_regex}"
            r"\+(?:868|0x364)",
            center_tail_operand(0x161),
            flags=re.IGNORECASE,
        )
        is None
        or _cc_targets._exact_memory_expression(center_tail_operand(0x166))
        not in {"eax+100", "eax+0x64"}
        or center_tail_operand(0x169).lower().replace(" ", "")
        != "eax,esi"
        or center_tail_operand(0x16B).lower().replace(" ", "")
        != "dword_reticlebounds$[esp+1388],eax"
    ):
        raise ValueError(
            "HUD reticle center-tail bridge rejects optional image "
            "height/width flow, EAX/EDX vptr provenance, ECX receiver, "
            "slot, or result composition drift"
        )

    center_tail_cfg_specs = {
        0x11C: ("conditional", 0x124),
        0x122: ("unconditional", 0x126),
        0x152: ("conditional", 0x15A),
        0x158: ("unconditional", 0x15C),
    }
    exact_center_tail_cfg = {
        source: _cc_cfg._exact_local_direct_branch(
            candidate.instructions[index_by_offset[source]],
            instruction_index=index_by_offset[source],
            instruction_addresses=offsets,
            instruction_index_by_address=index_by_offset,
            source="cod",
            caller_start=0,
            caller_end=0x4E0,
        )
        for source in center_tail_cfg_specs
    }
    scoped_center_tail_indices = frozenset(center_tail_indices)
    allowed_center_tail_cfg_edges = {
        (
            index_by_offset[source],
            index_by_offset[target],
        )
        for source, (_, target) in center_tail_cfg_specs.items()
    }
    incoming_center_tail_targets = {
        (source, target)
        for source, targets_ in candidate.local_control_flow_targets.items()
        for target in targets_
        if target in scoped_center_tail_indices
    }
    if (
        any(
            exact_center_tail_cfg[source]
            != (kind, index_by_offset[target])
            for source, (kind, target) in center_tail_cfg_specs.items()
        )
        or bool(
            candidate.local_control_flow_indices
            & scoped_center_tail_indices
        )
        or bool(
            incoming_center_tail_targets
            - allowed_center_tail_cfg_edges
        )
    ):
        raise ValueError(
            "HUD reticle center-tail bridge requires the exact optional "
            "image height/width CFG with no alternate entry"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_expected,
    )
    center_tail_call_order = tuple(
        index_by_offset[offset]
        for offset in (0x10E, 0x130, 0x144, 0x166, 0x16F)
    )
    if (
        tuple(invocation_indices[6:11]) != center_tail_call_order
        or [
            index
            for start_offset, call_offset, register in (
                (0x126, 0x130, "eax"),
                (0x133, 0x144, "edx"),
                (0x15C, 0x166, "eax"),
            )
            for index in range(
                index_by_offset[start_offset],
                index_by_offset[call_offset],
            )
            if _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index], register
            )
        ]
        != [
            index_by_offset[0x126],
            index_by_offset[0x133],
            index_by_offset[0x15C],
        ]
        or any(
            _cc_cfg._cleanup_after(
                candidate.instructions,
                index_by_offset[offset],
            )
            is not None
            for offset in (0x130, 0x144, 0x166)
        )
    ):
        raise ValueError(
            "HUD reticle center-tail bridge rejects post-GetCenterY call "
            "order, vptr reaching definitions, or cleanup drift"
        )

    clip_block_start = 0x16F
    clip_block_end = 0x240
    clip_block_body = bytes.fromhex(
        "e8 00 00 00 00 8d 4c 24 30 50 51 68 38 03 00 00 "
        "ff 15 00 00 00 00 85 c0 0f 84 b3 00 00 00 "
        "8b 15 64 03 00 00 b9 64 03 00 00 ff 52 68 "
        "8b 15 3c 03 00 00 b9 64 03 00 00 2b d0 "
        "a1 64 03 00 00 89 15 3c 03 00 00 ff 50 68 "
        "8b 0d 44 03 00 00 8b 15 64 03 00 00 2b c8 "
        "89 0d 44 03 00 00 b9 64 03 00 00 ff 52 64 "
        "8b 35 38 03 00 00 b9 64 03 00 00 2b f0 "
        "a1 64 03 00 00 89 35 38 03 00 00 ff 50 64 "
        "8b 15 40 03 00 00 b9 64 03 00 00 2b d0 "
        "89 15 40 03 00 00 8b 15 64 03 00 00 ff 52 68 "
        "8b 0d 3c 03 00 00 03 c1 b9 64 03 00 00 50 "
        "a1 64 03 00 00 ff 50 64 8b 15 38 03 00 00 "
        "b9 64 03 00 00 03 c2 8b 15 64 03 00 00 50 "
        "ff 52 0c c7 05 a8 03 00 00 38 03 00 00"
    )
    clip_block_relocations = (
        (
            0x170,
            IMAGE_REL_I386_REL32,
            "?GetDisplaySection@zOpt@@YIPAUzOpt_ViewRectSection@@XZ",
            0,
        ),
        (
            0x17B,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL,
            0x338,
        ),
        (
            0x181,
            IMAGE_REL_I386_DIR32,
            "__imp__IntersectRect@12",
            0,
        ),
        (
            0x18F,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x194,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x19D,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL,
            0x33C,
        ),
        (
            0x1A2,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x1A9,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x1AF,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL,
            0x33C,
        ),
        (
            0x1B8,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL,
            0x344,
        ),
        (
            0x1BE,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x1C6,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL,
            0x344,
        ),
        (
            0x1CB,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x1D4,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL,
            0x338,
        ),
        (
            0x1D9,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x1E0,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x1E6,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL,
            0x338,
        ),
        (
            0x1EF,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL,
            0x340,
        ),
        (
            0x1F4,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x1FC,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL,
            0x340,
        ),
        (
            0x202,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x20B,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL,
            0x33C,
        ),
        (
            0x212,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x218,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x221,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL,
            0x338,
        ),
        (
            0x226,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x22E,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x364,
        ),
        (
            0x238,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x3A8,
        ),
        (
            0x23C,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL,
            0x338,
        ),
    )
    actual_clip_block_relocations = tuple(
        (
            row.offset,
            row.type,
            row.symbol_name,
            (
                struct.unpack_from("<I", caller.data, row.offset)[0]
                if 0 <= row.offset <= len(caller.data) - 4
                else None
            ),
        )
        for row in caller.relocations
        if clip_block_start <= row.offset < clip_block_end
    )
    if (
        caller.data[clip_block_start:clip_block_end]
        != clip_block_body
        or actual_clip_block_relocations != clip_block_relocations
        or {
            offset
            for offset in range(clip_block_start, clip_block_end)
            if caller.relocation_mask[offset]
        }
        != {
            offset
            for relocation_offset, _, _, _ in clip_block_relocations
            for offset in range(relocation_offset, relocation_offset + 4)
        }
        or caller.defined_external_functions.count(
            "?GetDisplaySection@zOpt@@YIPAUzOpt_ViewRectSection@@XZ"
        )
        != 1
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL
        )
        != 1
        or caller.undefined_external_functions.count(
            "__imp__IntersectRect@12"
        )
        != 1
        or _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL
        in caller.defined_external_data
        or "__imp__IntersectRect@12"
        in caller.defined_external_functions
    ):
        raise ValueError(
            "HUD reticle clip-block bridge rejects exact bounded body, "
            "GetDisplaySection/IntersectRect/g_HudUiMgr/g_HudLayoutHW "
            "relocation target/addend/order, mask, or alias drift"
        )

    clip_block_offsets = (
        0x16F,
        0x174,
        0x178,
        0x179,
        0x17A,
        0x17F,
        0x185,
        0x187,
        0x18D,
        0x193,
        0x198,
        0x19B,
        0x1A1,
        0x1A6,
        0x1A8,
        0x1AD,
        0x1B3,
        0x1B6,
        0x1BC,
        0x1C2,
        0x1C4,
        0x1CA,
        0x1CF,
        0x1D2,
        0x1D8,
        0x1DD,
        0x1DF,
        0x1E4,
        0x1EA,
        0x1ED,
        0x1F3,
        0x1F8,
        0x1FA,
        0x200,
        0x206,
        0x209,
        0x20F,
        0x211,
        0x216,
        0x217,
        0x21C,
        0x21F,
        0x225,
        0x22A,
        0x22C,
        0x232,
        0x233,
        0x236,
    )
    if (
        0x16B not in index_by_offset
        or 0x240 not in index_by_offset
        or set(clip_block_offsets) - set(index_by_offset)
        or {
            offset
            for offset in offsets
            if (
                offset is not None
                and clip_block_start <= offset < clip_block_end
            )
        }
        != set(clip_block_offsets)
    ):
        raise ValueError(
            "HUD reticle clip-block bridge requires the exact predecessor, "
            "bounded instruction offsets, and successor boundary"
        )
    clip_block_indices = tuple(
        index_by_offset[offset] for offset in clip_block_offsets
    )
    if clip_block_indices != tuple(
        range(
            clip_block_indices[0],
            clip_block_indices[0] + len(clip_block_indices),
        )
    ):
        raise ValueError(
            "HUD reticle clip-block bridge rejects missing, duplicate, "
            "reordered, or noncontiguous instructions"
        )
    for position, offset in enumerate(clip_block_offsets):
        next_offset = (
            clip_block_offsets[position + 1]
            if position + 1 < len(clip_block_offsets)
            else clip_block_end
        )
        instruction = candidate.instructions[index_by_offset[offset]]
        if bytes(int(item, 16) for item in instruction.bytes) != caller.data[
            offset:next_offset
        ]:
            raise ValueError(
                "HUD reticle clip-block bridge rejects COD/object byte "
                f"disagreement at 0x{offset:x}"
            )
    if tuple(
        _cc_cfg._instruction_mnemonic(
            candidate.instructions[index_by_offset[offset]]
        )
        for offset in clip_block_offsets
    ) != (
        "call",
        "lea",
        "push",
        "push",
        "push",
        "call",
        "test",
        "je",
        "mov",
        "mov",
        "call",
        "mov",
        "mov",
        "sub",
        "mov",
        "mov",
        "call",
        "mov",
        "mov",
        "sub",
        "mov",
        "mov",
        "call",
        "mov",
        "mov",
        "sub",
        "mov",
        "mov",
        "call",
        "mov",
        "mov",
        "sub",
        "mov",
        "mov",
        "call",
        "mov",
        "add",
        "mov",
        "push",
        "mov",
        "call",
        "mov",
        "mov",
        "add",
        "mov",
        "push",
        "call",
        "mov",
    ):
        raise ValueError(
            "HUD reticle clip-block bridge rejects opcode/order drift"
        )

    def clip_block_operand(offset: int) -> str:
        return _cc_cfg._instruction_operand(
            candidate.instructions[index_by_offset[offset]]
        ).strip()

    def clip_block_memory_source(
        offset: int,
    ) -> tuple[str, str]:
        destination, source = clip_block_operand(offset).split(",", 1)
        return (
            destination.strip().lower(),
            _cc_targets._exact_memory_expression(source),
        )

    def exact_clip_receiver(offset: int) -> bool:
        return (
            re.fullmatch(
                rf"ecx\s*,\s*OFFSET\s+FLAT:{aggregate_regex}"
                r"\+(?:868|0x364)",
                clip_block_operand(offset),
                flags=re.IGNORECASE,
            )
            is not None
        )

    layout_regex = re.escape(_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL)
    if (
        clip_block_operand(0x16F).casefold()
        != (
            "?GetDisplaySection@zOpt@@"
            "YIPAUzOpt_ViewRectSection@@XZ"
        ).casefold()
        or clip_block_operand(0x174).lower().replace(" ", "")
        != "ecx,dword_reticlebounds$[esp+1380]"
        or clip_block_operand(0x178).lower() != "eax"
        or clip_block_operand(0x179).lower() != "ecx"
        or re.fullmatch(
            rf"OFFSET\s+FLAT:{layout_regex}\+(?:824|0x338)",
            clip_block_operand(0x17A),
            flags=re.IGNORECASE,
        )
        is None
        or clip_block_operand(0x17F).casefold()
        not in {
            "__imp__intersectrect@12",
            "dword __imp__intersectrect@12",
            "dword ptr __imp__intersectrect@12",
        }
        or clip_block_operand(0x185).lower().replace(" ", "")
        != "eax,eax"
        or clip_block_memory_source(0x18D)
        not in {
            ("edx", f"{aggregate}+868"),
            ("edx", f"{aggregate}+0x364"),
        }
        or not exact_clip_receiver(0x193)
        or _cc_targets._exact_memory_expression(clip_block_operand(0x198))
        not in {"edx+104", "edx+0x68"}
        or clip_block_memory_source(0x19B)
        not in {
            ("edx", f"{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+828"),
            ("edx", f"{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+0x33c"),
        }
        or not exact_clip_receiver(0x1A1)
        or clip_block_operand(0x1A6).lower().replace(" ", "")
        != "edx,eax"
        or clip_block_memory_source(0x1A8)
        not in {
            ("eax", f"{aggregate}+868"),
            ("eax", f"{aggregate}+0x364"),
        }
        or clip_block_operand(0x1AD).lower().replace(" ", "")
        not in {
            (
                f"dword{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+828,edx"
            ).lower(),
            (
                f"dword{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+0x33c,edx"
            ).lower(),
        }
        or _cc_targets._exact_memory_expression(clip_block_operand(0x1B3))
        not in {"eax+104", "eax+0x68"}
        or clip_block_memory_source(0x1B6)
        not in {
            ("ecx", f"{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+836"),
            ("ecx", f"{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+0x344"),
        }
        or clip_block_memory_source(0x1BC)
        not in {
            ("edx", f"{aggregate}+868"),
            ("edx", f"{aggregate}+0x364"),
        }
        or clip_block_operand(0x1C2).lower().replace(" ", "")
        != "ecx,eax"
        or clip_block_operand(0x1C4).lower().replace(" ", "")
        not in {
            (
                f"dword{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+836,ecx"
            ).lower(),
            (
                f"dword{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+0x344,ecx"
            ).lower(),
        }
        or not exact_clip_receiver(0x1CA)
        or _cc_targets._exact_memory_expression(clip_block_operand(0x1CF))
        not in {"edx+100", "edx+0x64"}
        or clip_block_memory_source(0x1D2)
        not in {
            ("esi", f"{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+824"),
            ("esi", f"{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+0x338"),
        }
        or not exact_clip_receiver(0x1D8)
        or clip_block_operand(0x1DD).lower().replace(" ", "")
        != "esi,eax"
        or clip_block_memory_source(0x1DF)
        not in {
            ("eax", f"{aggregate}+868"),
            ("eax", f"{aggregate}+0x364"),
        }
        or clip_block_operand(0x1E4).lower().replace(" ", "")
        not in {
            (
                f"dword{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+824,esi"
            ).lower(),
            (
                f"dword{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+0x338,esi"
            ).lower(),
        }
        or _cc_targets._exact_memory_expression(clip_block_operand(0x1EA))
        not in {"eax+100", "eax+0x64"}
        or clip_block_memory_source(0x1ED)
        not in {
            ("edx", f"{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+832"),
            ("edx", f"{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+0x340"),
        }
        or not exact_clip_receiver(0x1F3)
        or clip_block_operand(0x1F8).lower().replace(" ", "")
        != "edx,eax"
        or clip_block_operand(0x1FA).lower().replace(" ", "")
        not in {
            (
                f"dword{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+832,edx"
            ).lower(),
            (
                f"dword{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+0x340,edx"
            ).lower(),
        }
        or clip_block_memory_source(0x200)
        not in {
            ("edx", f"{aggregate}+868"),
            ("edx", f"{aggregate}+0x364"),
        }
        or _cc_targets._exact_memory_expression(clip_block_operand(0x206))
        not in {"edx+104", "edx+0x68"}
        or clip_block_memory_source(0x209)
        not in {
            ("ecx", f"{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+828"),
            ("ecx", f"{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+0x33c"),
        }
        or clip_block_operand(0x20F).lower().replace(" ", "")
        != "eax,ecx"
        or not exact_clip_receiver(0x211)
        or clip_block_operand(0x216).lower() != "eax"
        or clip_block_memory_source(0x217)
        not in {
            ("eax", f"{aggregate}+868"),
            ("eax", f"{aggregate}+0x364"),
        }
        or _cc_targets._exact_memory_expression(clip_block_operand(0x21C))
        not in {"eax+100", "eax+0x64"}
        or clip_block_memory_source(0x21F)
        not in {
            ("edx", f"{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+824"),
            ("edx", f"{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+0x338"),
        }
        or not exact_clip_receiver(0x225)
        or clip_block_operand(0x22A).lower().replace(" ", "")
        != "eax,edx"
        or clip_block_memory_source(0x22C)
        not in {
            ("edx", f"{aggregate}+868"),
            ("edx", f"{aggregate}+0x364"),
        }
        or clip_block_operand(0x232).lower() != "eax"
        or _cc_targets._exact_memory_expression(clip_block_operand(0x233))
        not in {"edx+12", "edx+0xc"}
        or clip_block_operand(0x236).lower().replace(" ", "")
        not in {
            (
                f"dword{aggregate}+936,"
                f"OFFSETFLAT:{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+824"
            ).lower(),
            (
                f"dword{aggregate}+0x3a8,"
                f"OFFSETFLAT:{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+0x338"
            ).lower(),
        }
    ):
        raise ValueError(
            "HUD reticle clip-block bridge rejects IntersectRect setup, "
            "EAX/EDX vptr provenance, ECX receiver, slot, clip subtract/"
            "store, SetPos add/argument, or result flow drift"
        )

    clip_block_cfg = _cc_cfg._exact_local_direct_branch(
        candidate.instructions[index_by_offset[0x187]],
        instruction_index=index_by_offset[0x187],
        instruction_addresses=offsets,
        instruction_index_by_address=index_by_offset,
        source="cod",
        caller_start=0,
        caller_end=0x4E0,
    )
    scoped_clip_success_indices = frozenset(
        index_by_offset[offset]
        for offset in clip_block_offsets
        if 0x18D <= offset < clip_block_end
    )
    incoming_clip_success_targets = {
        (source, target)
        for source, targets_ in candidate.local_control_flow_targets.items()
        for target in targets_
        if target in scoped_clip_success_indices
    }
    if (
        clip_block_cfg
        != ("conditional", index_by_offset[0x240])
        or bool(
            candidate.local_control_flow_indices
            & frozenset(clip_block_indices)
        )
        or bool(incoming_clip_success_targets)
    ):
        raise ValueError(
            "HUD reticle clip-block bridge requires exact IntersectRect "
            "success fallthrough/false exit CFG with no alternate entry"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_expected,
    )
    clip_block_call_order = tuple(
        index_by_offset[offset]
        for offset in (
            0x16F,
            0x17F,
            0x198,
            0x1B3,
            0x1CF,
            0x1EA,
            0x206,
            0x21C,
            0x233,
        )
    )
    if (
        tuple(invocation_indices[10:19]) != clip_block_call_order
        or [
            index
            for start_offset, call_offset, register in (
                (0x18D, 0x198, "edx"),
                (0x1A8, 0x1B3, "eax"),
                (0x1BC, 0x1CF, "edx"),
                (0x1DF, 0x1EA, "eax"),
                (0x200, 0x206, "edx"),
                (0x217, 0x21C, "eax"),
                (0x22C, 0x233, "edx"),
            )
            for index in range(
                index_by_offset[start_offset],
                index_by_offset[call_offset],
            )
            if _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index], register
            )
        ]
        != [
            index_by_offset[0x18D],
            index_by_offset[0x1A8],
            index_by_offset[0x1BC],
            index_by_offset[0x1DF],
            index_by_offset[0x200],
            index_by_offset[0x217],
            index_by_offset[0x22C],
        ]
        or any(
            _cc_cfg._cleanup_after(
                candidate.instructions,
                index_by_offset[offset],
            )
            is not None
            for offset in (
                0x198,
                0x1B3,
                0x1CF,
                0x1EA,
                0x206,
                0x21C,
                0x233,
            )
        )
    ):
        raise ValueError(
            "HUD reticle clip-block bridge rejects call order, vptr "
            "reaching definitions, or cleanup drift"
        )

    bridges = {
        "0x1c": ReviewedExactIndirectStorageBridge(
            register="eax",
            storage_identity=storage_identity,
            slot_displacement=0x60,
            assembly_source="cod",
        ),
        "0x3e": ReviewedExactIndirectStorageBridge(
            register="edx",
            storage_identity=storage_identity,
            slot_displacement=0x60,
            assembly_source="cod",
        ),
        "0xcd": ReviewedExactIndirectStorageBridge(
            register="eax",
            storage_identity=storage_identity,
            slot_displacement=0x0C,
            assembly_source="cod",
        ),
        "0x10e": ReviewedExactIndirectStorageBridge(
            register="edx",
            storage_identity=storage_identity,
            slot_displacement=0x68,
            assembly_source="cod",
        ),
        "0x130": ReviewedExactIndirectStorageBridge(
            register="eax",
            storage_identity=storage_identity,
            slot_displacement=0x68,
            assembly_source="cod",
        ),
        "0x144": ReviewedExactIndirectStorageBridge(
            register="edx",
            storage_identity=storage_identity,
            slot_displacement=0x64,
            assembly_source="cod",
        ),
        "0x166": ReviewedExactIndirectStorageBridge(
            register="eax",
            storage_identity=storage_identity,
            slot_displacement=0x64,
            assembly_source="cod",
        ),
        "0x198": ReviewedExactIndirectStorageBridge(
            register="edx",
            storage_identity=storage_identity,
            slot_displacement=0x68,
            assembly_source="cod",
        ),
        "0x1b3": ReviewedExactIndirectStorageBridge(
            register="eax",
            storage_identity=storage_identity,
            slot_displacement=0x68,
            assembly_source="cod",
        ),
        "0x1cf": ReviewedExactIndirectStorageBridge(
            register="edx",
            storage_identity=storage_identity,
            slot_displacement=0x64,
            assembly_source="cod",
        ),
        "0x1ea": ReviewedExactIndirectStorageBridge(
            register="eax",
            storage_identity=storage_identity,
            slot_displacement=0x64,
            assembly_source="cod",
        ),
        "0x206": ReviewedExactIndirectStorageBridge(
            register="edx",
            storage_identity=storage_identity,
            slot_displacement=0x68,
            assembly_source="cod",
        ),
        "0x21c": ReviewedExactIndirectStorageBridge(
            register="eax",
            storage_identity=storage_identity,
            slot_displacement=0x64,
            assembly_source="cod",
        ),
        "0x233": ReviewedExactIndirectStorageBridge(
            register="edx",
            storage_identity=storage_identity,
            slot_displacement=0x0C,
            assembly_source="cod",
        ),
    }
    if len(bridges) != len(set(bridges)):
        raise ValueError(
            "HUD reticle-widget bridge composition contains a duplicate "
            "callsite"
        )
    return bridges
