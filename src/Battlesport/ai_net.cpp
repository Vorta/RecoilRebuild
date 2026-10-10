/*
 * Selects the angle-first Vec3RotateY declaration view of zmth_decls.h (see
 * the reconstruction interface note there) before ai_net.h reaches it.
 * Retail 0x4036bd and 0x4036cc load both pointers before pushing the angle.
 */
#define ZMTH_VEC3ROTATEY_LEGACY_ORDER 1
#include "Battlesport/ai_net.h"

extern "C" {
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-ainet-path-probe-half-width-scale
 * @recoil-artifact defines .rdata recoil:data:0x4cc850: Read-only half-width scale used by the path-probe travel clamp.
 * Retail data evidence: the path-probe travel clamp reads this 0.5 scalar when
 * selecting the minimum half-width travel.
 * Purpose: Stores the read-only half-width scale used for minimum probe-fan travel.
 */
extern const float g_AINetPathProbeHalfWidthScale = 0.5f;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-ainetlisthead
 * @recoil-artifact defines .data recoil:data:0x4e5c58: g_AINetListHead.
 * Purpose: Stores the zero-initialized head pointer for the loaded AINet global list.
 */
AINet* g_AINetListHead = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-ainetlisttail
 * @recoil-artifact defines .data recoil:data:0x4e5c5c: g_AINetListTail.
 * Purpose: Stores the zero-initialized tail pointer maintained with the loaded AINet global list.
 */
AINet* g_AINetListTail = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-player-aimode2-pathfollowpitchinputscale
 * @recoil-artifact defines .data recoil:data:0x4da0c0: g_Player_AiMode2_PathFollowPitchInputScale.
 * BN types this as an initialized .data float used by the Mode2 AI
 * path-follow pitch steering input.
 * Purpose: Scales path-follow vertical steering error into pitch input.
 */
float g_Player_AiMode2_PathFollowPitchInputScale = 0.0174499992f;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-player-aimode2-pathfollowpitchturngain
 * @recoil-artifact defines .data recoil:data:0x4da0c4: g_Player_AiMode2_PathFollowPitchTurnGain.
 * BN types this as an initialized .data float paired with the Mode2 AI
 * path-follow pitch input scale.
 * Purpose: Scales path-follow pitch input into turn correction.
 */
float g_Player_AiMode2_PathFollowPitchTurnGain = 5.69999981f;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-player-aimode2-steeringpitchinputscale
 * @recoil-artifact defines .data recoil:data:0x4da0c8: g_Player_AiMode2_SteeringPitchInputScale.
 * BN types this as an initialized .data float used by the Mode2 AI steering
 * substates.
 * Purpose: Scales steering vertical distance into pitch input.
 */
float g_Player_AiMode2_SteeringPitchInputScale = 0.800000012f;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-player-aimode2-steeringpitchturngain
 * @recoil-artifact defines .data recoil:data:0x4da0cc: g_Player_AiMode2_SteeringPitchTurnGain.
 * BN types this as an initialized .data float paired with the Mode2 AI
 * steering pitch input scale.
 * Purpose: Scales steering pitch input into turn correction.
 */
float g_Player_AiMode2_SteeringPitchTurnGain = 5.69999981f;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-player-aimode2-steeringverticalerrorscale
 * @recoil-artifact defines .data recoil:data:0x4da0d0: g_Player_AiMode2_SteeringVerticalErrorScale.
 * BN types this as an initialized .data float read by the Mode2 AI steering
 * substates.
 * Purpose: Scales steering vertical error before pitch correction.
 */
float g_Player_AiMode2_SteeringVerticalErrorScale = 0.100000001f;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-player-aimode2-tuningscalar55a
 * @recoil-artifact defines .data recoil:data:0x4da0d4: g_Player_AiMode2_TuningScalar55A.
 * BN types this as an initialized .data float in the contiguous Mode2 AI
 * tuning scalar range.
 * Purpose: Stores the first Mode2 AI 55.0 tuning scalar.
 */
float g_Player_AiMode2_TuningScalar55A = 55.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-player-aimode2-tuningscalar55b
 * @recoil-artifact defines .data recoil:data:0x4da0d8: g_Player_AiMode2_TuningScalar55B.
 * BN types this as an initialized .data float in the contiguous Mode2 AI
 * tuning scalar range.
 * Purpose: Stores the second Mode2 AI 55.0 tuning scalar.
 */
float g_Player_AiMode2_TuningScalar55B = 55.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-player-aimode2-tuningscalar5
 * @recoil-artifact defines .data recoil:data:0x4da0dc: g_Player_AiMode2_TuningScalar5.
 * BN types this as an initialized .data float in the contiguous Mode2 AI
 * tuning scalar range.
 * Purpose: Stores the Mode2 AI 5.0 tuning scalar.
 */
float g_Player_AiMode2_TuningScalar5 = 5.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-player-aimode2-tuningscalar10
 * @recoil-artifact defines .data recoil:data:0x4da0e0: g_Player_AiMode2_TuningScalar10.
 * BN types this as an initialized .data float in the contiguous Mode2 AI
 * tuning scalar range.
 * Purpose: Stores the Mode2 AI 10.0 tuning scalar.
 */
float g_Player_AiMode2_TuningScalar10 = 10.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-player-aimode2-offsettargetrotatecos15deg
 * @recoil-artifact defines .data recoil:data:0x4da0e4: g_Player_AiMode2_OffsetTargetRotateCos15Deg.
 * BN types this as an initialized .data float used by the Mode2 AI offset
 * target steering rotation.
 * Inferred declaration qualifier: consistent volatile float declarations
 * reproduce both retail coefficient reads under canonical VC5SP3. The
 * original cv-qualification and its purpose remain unresolved; no external
 * writer or synchronization contract is inferred. Pro review:
 * 2026-09-08T16-25-03-957Z. Qualifier controls affect only this scalar's reads.
 * Purpose: Stores the retail cosine scalar for offset-target rotation.
 */
volatile float g_Player_AiMode2_OffsetTargetRotateCos15Deg = 0.965900004f;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-player-aimode2-offsettargetrotatesin15deg
 * @recoil-artifact defines .data recoil:data:0x4da0e8: g_Player_AiMode2_OffsetTargetRotateSin15Deg.
 * BN types this as an initialized .data float used by the Mode2 AI offset
 * target steering rotation.
 * Inferred declaration qualifier: consistent volatile float declarations
 * reproduce both retail coefficient reads under canonical VC5SP3. The
 * original cv-qualification and its purpose remain unresolved; no external
 * writer or synchronization contract is inferred. Pro review:
 * 2026-09-08T16-25-03-957Z. Qualifier controls affect only this scalar's reads.
 * Purpose: Stores the retail sine scalar for offset-target rotation.
 */
volatile float g_Player_AiMode2_OffsetTargetRotateSin15Deg = 0.25879999995f;
}

#include "Battlesport/game_net.h"
#include "Battlesport/player.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth_decls.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zTime/time.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Battlesport/game_net.h"
#include "Battlesport/player.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth_decls.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zTime/time.h"

#include <math.h>
#include <string.h>

const int kPlayerAiMode2TopSteering = 1;
const int kPlayerAiMode2SteerDirectTarget = 0;
const int kPlayerAiMode2SteerOffsetTarget = 1;
const int kPlayerAiMode2SteerDynamicOffsetTarget = 2;
const int kPlayerAiMode2SteerPathFollow = 3;
const int kPlayerAiMode2SteerTurnInPlace = 5;
const int kPlayerAiMode2SteerAutoTurn = 6;
const float kPlayerAiAltGunAttackForwardMin = 0.75f;
const float kPlayerAiAltGunStatusMinScale = 0.5f;
const int kPlayerAiTopPathFollow = 0;
const int kPlayerAiTopTurnTowardTarget = 2;
const int kPlayerAiTopTurnOnlyTowardTarget = 3;
const int kPlayerAiTopPathSteering = 4;
const int kPlayerAiTopAutoTurn = 5;
const int kPlayerMasterTypeSub = 2;
const int kPlayerLifecycleInactive = 4;
const float kPlayerAiPathFollowMinThrottle = 0.25f;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.path-node-advance-distance
 * @recoil-artifact defines .rdata recoil:data:0x4cc844: Shared five-unit path-node advance distance.
 * Purpose: Supplies the threshold read by forward and reverse node steering.
 * This is the current named native C++ constant; its original spelling and
 * lexical scope remain unresolved. The compiler ordinal is not its identity.
 */
const float kPlayerAiForwardPathAdvanceDistance = 5.0f;
const float kPlayerAiForwardProbeMinLength = 1.0f;
const float kPlayerAiForwardProbeLengthHalfScale = 0.5f;
const float kPlayerAiSyntheticPathRebuildDistanceSq = 400.0f;
const float kPlayerAiSyntheticPathWidth = 10.0f;
const float kPlayerAiSyntheticPathRebuildDelaySec = -1.0f;
const float kPlayerAiAttackLosTargetYOffset = -1.5f;
const unsigned int kOptCatalogFlagLockOnTargetRef = 0x4000;
const unsigned int kOptCatalogFlagCreateTrail = 0x02;

#define AINET_MAX(a, b) (((a) < (b)) ? (b) : (a))

#define AINET_VEC3_SUB_WORLD_DST_V0_WORLD_V1(dst, srcVec, world)                                                       \
    do {                                                                                                               \
        v0 = &(dst);                                                                                                   \
        v1 = &(world);                                                                                                 \
        v0->x = (srcVec)->x - v1->x;                                                                                   \
        v0->y = (srcVec)->y - v1->y;                                                                                   \
        v0->z = (srcVec)->z - v1->z;                                                                                   \
    } while (0)

#define AINET_VEC3_SUB_WORLD_DST_V1_WORLD_V0(dst, srcVec, world)                                                       \
    do {                                                                                                               \
        v1 = &(dst);                                                                                                   \
        v0 = &(world);                                                                                                 \
        v1->x = (srcVec)->x - v0->x;                                                                                   \
        v1->y = (srcVec)->y - v0->y;                                                                                   \
        v1->z = (srcVec)->z - v0->z;                                                                                   \
    } while (0)

#define AINET_VEC3_DOT_XZ(out, steer, delta)                                                                           \
    do {                                                                                                               \
        v1 = &(delta);                                                                                                 \
        v0 = &(steer);                                                                                                 \
        (out) = v0->x * v1->x + v0->z * v1->z;                                                                         \
    } while (0)

#define AINET_VEC3_CROSS_XZ(out, steer, delta)                                                                         \
    do {                                                                                                               \
        v1 = &(delta);                                                                                                 \
        v2 = &(steer);                                                                                                 \
        (out) = v2->z * v1->x - v2->x * v1->z;                                                                         \
    } while (0)

#define AINET_TURN_DIRECTION_SLOT(cross) (*(int*)&(cross))

#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
/**
 * @recoil-raw-asm recoil:raw-asm:battlesport.ai-net.forward-probe-add-world
 * Raw assembly for 0x401420: emits the likely original VC5 x87 vector-add
 * helper body after C++ has bound destination/source pointer temps. ChatGPT Pro
 * source-shape review classified the surrounding normalize, scale, contact,
 * and return logic as C++ compiler output; only this fixed-register add body is
 * treated as the original inline-asm island.
 * Purpose: Add the player's world position into the forward probe endpoint.
 */
#define AINET_FORWARD_PROBE_ADD_WORLD_ASM(dstArg, worldArg, endArg)                                                    \
    do {                                                                                                               \
        zVec3* zaddDst = (dstArg);                                                                                     \
        zVec3* zaddWorld = (worldArg);                                                                                 \
        zVec3* zaddEnd = (endArg);                                                                                     \
        __asm mov ebx, zaddEnd __asm mov ecx, zaddWorld __asm mov edx,                                                 \
            zaddDst __asm fld dword ptr[ebx] zVec3.x __asm fadd dword ptr[ecx] zVec3.x __asm fld dword                 \
                ptr[ebx] zVec3.y __asm fadd dword ptr[ecx] zVec3.y __asm fld dword                                     \
                    ptr[ebx] zVec3.z __asm fadd dword ptr[ecx] zVec3.z __asm fxch ST(2) __asm fstp dword               \
                        ptr[edx] zVec3.x __asm fstp dword ptr[edx] zVec3.y __asm fstp dword ptr[edx] zVec3.z           \
    } while (0)

/**
 * @recoil-raw-asm recoil:raw-asm:battlesport.ai-net.vector-subtract
 * Original-source helper evidence: no standalone retail function exists.
 * The repeated callers at 0x401180, 0x401580, 0x401710, 0x401c60, 0x402090,
 * 0x402170, 0x402be0, 0x402d60, and 0x403620 share the same fixed-register EBX/ECX/EDX
 * grouped-x87 subtraction, `fxch`, and ordered-store sequence. C/C++ forms
 * failed to preserve that retail VC5 shape; the exact historical identifier
 * spelling remains unproven.
 * Raw-assembly evidence: the shared inline-asm region preserves that retail
 * VC5SP3 fixed-register sequence at each declared consumer.
 * Purpose: Provide the recovered shared inlined AINet vector subtraction.
 */
#define AINET_VECTOR_SUBTRACT(destination, source, subtractor)                                                         \
    __asm {                                                     \
        __asm mov ebx, source                                  \
        __asm mov ecx, subtractor                              \
        __asm mov edx, destination                             \
        __asm fld dword ptr [ebx]zVec3.x                       \
        __asm fsub dword ptr [ecx]zVec3.x                      \
        __asm fld dword ptr [ebx]zVec3.y                       \
        __asm fsub dword ptr [ecx]zVec3.y                      \
        __asm fld dword ptr [ebx]zVec3.z                       \
        __asm fsub dword ptr [ecx]zVec3.z                      \
        __asm fxch ST(2)                                       \
        __asm fstp dword ptr [edx]zVec3.x                      \
        __asm fstp dword ptr [edx]zVec3.y                      \
        __asm fstp dword ptr [edx]zVec3.z }

/**
 * Raw assembly wrapper for 0x401180: computes the auto-turn target delta while
 * preserving the observed VC5 local pointer binding for the recovered inlined
 * vector subtract helper.
 * Purpose: Produce the auto-turn target delta for path-follow recovery.
 */
#define AINET_PATH_COMPUTE_AUTO_TURN_DELTA(dst, srcVec, world)                                                         \
    do {                                                                                                               \
        zVec3* v0;                                                                                                     \
        zVec3* v1;                                                                                                     \
        v0 = &(dst);                                                                                                   \
        v1 = &(world);                                                                                                 \
        AINET_VECTOR_SUBTRACT(v0, srcVec, v1)                                                                          \
    } while (0)

/**
 * Raw assembly wrapper for 0x401180: computes the path-follow target delta
 * while preserving the observed VC5 local pointer binding for the recovered
 * inlined vector subtract helper.
 * Purpose: Produce the steering target delta for path-follow movement.
 */
#define AINET_PATH_COMPUTE_PATH_TARGET_DELTA(dst, srcVec, world)                                                       \
    do {                                                                                                               \
        zVec3* v0;                                                                                                     \
        zVec3* v1;                                                                                                     \
        v1 = &(dst);                                                                                                   \
        v0 = &(world);                                                                                                 \
        AINET_VECTOR_SUBTRACT(v1, srcVec, v0)                                                                          \
    } while (0)

/**
 * Raw assembly wrapper for 0x401c60: computes the dynamic offset direction
 * while preserving the observed VC5 local pointer binding for the recovered
 * inlined vector subtract helper.
 * Purpose: Produce the dynamic-offset pursuit direction.
 */
#define AINET_PATH_COMPUTE_DYNAMIC_OFFSET_DIR(dst, srcVec, world)                                                      \
    do {                                                                                                               \
        zVec3* v0;                                                                                                     \
        zVec3* v1;                                                                                                     \
        zVec3* v2;                                                                                                     \
        v1 = &(world);                                                                                                 \
        v2 = &(dst);                                                                                                   \
        v0 = &(srcVec);                                                                                                \
        AINET_VECTOR_SUBTRACT(v2, v0, v1)                                                                              \
    } while (0)

/**
 * Raw assembly wrapper for AINet path-probe setup: computes a segment delta
 * while preserving the observed VC5 local pointer binding for the recovered
 * inlined vector subtract helper.
 * Purpose: Produce the path-probe fan segment delta.
 */
#define AINET_PROBE_FAN_COMPUTE_SEGMENT_DELTA(dst, srcVec, world)                                                      \
    do {                                                                                                               \
        zVec3* v0;                                                                                                     \
        zVec3* v1;                                                                                                     \
        zVec3* v2;                                                                                                     \
        v2 = &(dst);                                                                                                   \
        v1 = &(world);                                                                                                 \
        v0 = &(srcVec);                                                                                                \
        AINET_VECTOR_SUBTRACT(v2, v0, v1)                                                                              \
    } while (0)

/**
 * Raw assembly wrapper for 0x403620: computes a segment delta and leaves the
 * destination pointer available to the following clamp helper.
 * Purpose: Produce the path-probe fan segment delta and retain its pointer.
 */
#define AINET_PROBE_FAN_COMPUTE_SEGMENT_DELTA_KEEP_PTR(dst, srcVec, world, dstPtr)                                     \
    do {                                                                                                               \
        zVec3* v0;                                                                                                     \
        zVec3* v1;                                                                                                     \
        dstPtr = &(dst);                                                                                               \
        v1 = &(world);                                                                                                 \
        v0 = &(srcVec);                                                                                                \
        AINET_VECTOR_SUBTRACT(dstPtr, v0, v1)                                                                          \
    } while (0)

/**
 * @recoil-raw-asm recoil:raw-asm:battlesport.ai-net.path-probe-clamp-travel-vc5
 * Raw assembly for 0x403620: computes the XZ length, clamps travel against
 * path width, and stores the retail `clampedTravel` slot with the observed VC5
 * x87/control-flow shape. C/C++ clamp and assignment variants failed to
 * byte-match the retail register and FPU ordering.
 * Purpose: Preserve the byte-sensitive AINet path-probe travel clamp.
 */
#define AINET_PATH_PROBE_CLAMP_TRAVEL_VC5(deltaPtr, xzLengthLocal, pathWidthValue)                                     \
    do {                                                                                                               \
        __asm mov ecx,                                                                                                 \
            deltaPtr __asm fld dword ptr[ecx] zVec3.x __asm fmul dword ptr[ecx] zVec3.x __asm fld dword                \
                ptr[ecx] zVec3.z __asm fmul dword ptr[ecx] zVec3.z __asm faddp ST(1),                                  \
            ST(0) __asm fsqrt __asm fstp dword ptr xzLengthLocal __asm fld dword ptr pathWidthValue __asm fmul dword   \
                ptr g_AINetPathProbeHalfWidthScale __asm fld dword ptr xzLengthLocal __asm fsub dword ptr              \
                    pathWidthValue __asm fcomp ST(1) __asm fnstsw ax __asm test ah,                                    \
            041h __asm jne ainet_path_probe_clamp_store __asm fstp ST(0) __asm mov ecx,                                \
            deltaPtr __asm fld dword ptr[ecx] zVec3.x __asm fmul dword ptr[ecx] zVec3.x __asm fld dword                \
                ptr[ecx] zVec3.z __asm fmul dword ptr[ecx] zVec3.z __asm faddp ST(1),                                  \
            ST(0) __asm fsqrt __asm fstp dword ptr xzLengthLocal __asm fld dword ptr xzLengthLocal __asm fsub dword    \
                ptr pathWidthValue __asm ainet_path_probe_clamp_store                                                  \
            : __asm fstp dword ptr[esi] AINetPathProbeFan.clampedTravel                                                \
    } while (0)

/**
 * Raw assembly wrapper for 0x402be0 and 0x402d60: computes the forward path
 * node direction while preserving the caller's outer `v2` local required by
 * the recovered VC5 byte shape.
 * Purpose: Produce the direction to the next forward path node.
 */
#define AINET_PATH_COMPUTE_FORWARD_NODE_DIR(dst, srcVec, world)                                                        \
    do {                                                                                                               \
        zVec3* v0;                                                                                                     \
        zVec3* v1;                                                                                                     \
        v1 = &(dst);                                                                                                   \
        v2 = &(world);                                                                                                 \
        v0 = &(srcVec);                                                                                                \
        AINET_VECTOR_SUBTRACT(v1, v0, v2)                                                                              \
    } while (0)

/**
 * Raw assembly wrapper for 0x402090 and 0x402170: computes a delta to the
 * local player's world position while preserving the observed VC5 local
 * pointer binding for the recovered inlined vector subtract helper.
 * Purpose: Produce the local-player target delta for turn-in-place helpers.
 */
#define AINET_PATH_COMPUTE_LOCAL_PLAYER_DELTA(dst, srcVec, world)                                                      \
    do {                                                                                                               \
        zVec3* v0;                                                                                                     \
        zVec3* v1;                                                                                                     \
        zVec3* v2;                                                                                                     \
        v0 = &(dst);                                                                                                   \
        v1 = &(world);                                                                                                 \
        v2 = (srcVec);                                                                                                 \
        AINET_VECTOR_SUBTRACT(v0, v2, v1)                                                                              \
    } while (0)

/**
 * @recoil-raw-asm recoil:raw-asm:battlesport.ai-net.path-dot-xz
 * Raw assembly for 0x401180: computes the XZ dot product using the observed VC5
 * x87 load/multiply/add/store sequence. C/C++ dot-product variants failed to
 * preserve the retail FPU stack and local pointer order recorded by the
 * ainet-vector exception.
 * Purpose: Produce byte-sensitive path-follow forward-dot math.
 */
#define AINET_PATH_DOT_XZ(out, steer, delta)                                                                           \
    do {                                                                                                               \
        zVec3* v0;                                                                                                     \
        zVec3* v1;                                                                                                     \
        v1 = &(delta);                                                                                                 \
        v0 = &(steer);                                                                                                 \
        __asm mov ecx, v0 __asm mov edx,                                                                               \
            v1 __asm fld dword ptr[ecx] zVec3.x __asm fmul dword ptr[edx] zVec3.x __asm fld dword                      \
                ptr[ecx] zVec3.z __asm fmul dword ptr[edx] zVec3.z __asm faddp ST(1),                                  \
            ST(0) __asm fstp dword ptr[out]                                                                            \
    } while (0)

/**
 * @recoil-raw-asm recoil:raw-asm:battlesport.ai-net.path-cross-xz
 * Raw assembly for 0x401180: computes the XZ cross product using the observed
 * VC5 x87 load/multiply/subtract/store sequence. C/C++ cross-product variants
 * failed to preserve the retail register and FPU ordering recorded by the
 * ainet-vector exception.
 * Purpose: Produce byte-sensitive path-follow steering-cross math.
 */
#define AINET_PATH_CROSS_XZ(out, steer, delta)                                                                         \
    do {                                                                                                               \
        zVec3* v1;                                                                                                     \
        v1 = &(delta);                                                                                                 \
        v2 = &(steer);                                                                                                 \
        __asm mov ebx, v2 __asm mov ecx,                                                                               \
            v1 __asm fld dword ptr[ebx] zVec3.z __asm fmul dword ptr[ecx] zVec3.x __asm fld dword                      \
                ptr[ebx] zVec3.x __asm fmul dword ptr[ecx] zVec3.z __asm fsubp ST(1),                                  \
            ST(0) __asm fstp dword ptr[out]                                                                            \
    } while (0)
/**
 * @recoil-raw-asm recoil:raw-asm:battlesport.ai-net.solve-alt-gun-lead.vector-dot-xyz
 * Raw assembly for 0x4024a0: computes the full XYZ dot product with the observed
 * VC5 fixed-register grouped-x87 load/multiply/exchange/add/store sequence. The
 * caller binds both operand pointers first, so only the reload-and-accumulate
 * island is assembly; source-faithful C++ dot shapes emitted the frame-pointer
 * free class instead of the retail grouped island.
 * Purpose: Produce the byte-sensitive alt-gun lead-solve XYZ dot products.
 */
#define AINET_VECTOR_DOT_XYZ(out, source, factor)                                                                      \
    do {                                                                                                               \
        zVec3* v0;                                                                                                     \
        zVec3* v1;                                                                                                     \
        v1 = &(factor);                                                                                                \
        v0 = &(source);                                                                                                \
        __asm mov ecx, v0 __asm mov edx,                                                                               \
            v1 __asm fld dword ptr[ecx] zVec3.x __asm fmul dword ptr[edx] zVec3.x __asm fld dword                      \
                ptr[ecx] zVec3.y __asm fmul dword ptr[edx] zVec3.y __asm fld dword                                     \
                    ptr[ecx] zVec3.z __asm fmul dword ptr[edx] zVec3.z __asm fxch ST(1) __asm faddp ST(2),             \
            ST(0) __asm faddp ST(1), ST(0) __asm fstp dword ptr[out]                                                   \
    } while (0)
/**
 * @recoil-raw-asm recoil:raw-asm:battlesport.ai-net.solve-alt-gun-lead.fast-sqrt-estimate
 * Raw-assembly evidence: retail function offsets [+0x195,+0x1a2)
 * transform the named `discriminant` local through EAX and store the
 * result in the distinct named `fastSqrtEstimate` local, with exact
 * bytes 8B45ECD1F8050000C01F8945F0. Source-faithful VC5SP3 C++
 * variants either coalesced both floats or moved pointer preparation
 * ahead of the x87 completion; even the distinct-[ebp-10] variant kept
 * the wrong EDX/scheduling shape. Parent-brokered Pro review request
 * recoil-hard-byte-4024a0-r4440-20260818T163644Z confirmed that only
 * this four-instruction named-local semantic island is required.
 * Purpose: Preserve the retail fast square-root estimate transform
 * without absorbing the surrounding x87 or pointer-setup code.
 */
#define AINET_FAST_SQRT_ESTIMATE(destination, source)                                                                  \
    __asm {                                           \
        __asm mov eax, source                         \
        __asm sar eax, 1                              \
        __asm add eax, 01fc00000h                     \
        __asm mov destination, eax }
/**
 * @recoil-raw-asm recoil:raw-asm:battlesport.ai-net.solve-alt-gun-lead.vector-add
 * Raw assembly for 0x4024a0: adds two vectors with the observed VC5
 * fixed-register grouped-x87 load/add/exchange/ordered-store sequence. The
 * caller binds the operand pointers first, so only the reload-and-store island
 * is assembly.
 * Purpose: Produce the byte-sensitive alt-gun lead target-point vector add.
 */
#define AINET_VECTOR_ADD_BOUND(destination, sourcePtr, addendPtr)                                                      \
    do {                                                                                                               \
        __asm mov ebx, sourcePtr __asm mov ecx, addendPtr __asm mov edx,                                               \
            destination __asm fld dword ptr[ebx] zVec3.x __asm fadd dword ptr[ecx] zVec3                               \
                .x __asm fld dword ptr[ebx] zVec3.y __asm fadd dword ptr[ecx] zVec3.y __asm fld dword                  \
                    ptr[ebx] zVec3.z __asm fadd dword ptr[ecx] zVec3.z __asm fxch ST(2) __asm fstp dword               \
                        ptr[edx] zVec3.x __asm fstp dword ptr[edx] zVec3.y __asm fstp dword ptr[edx] zVec3.z           \
    } while (0)
#define AINET_VECTOR_ADD(destination, source, addend)                                                                  \
    do {                                                                                                               \
        zVec3* v0;                                                                                                     \
        zVec3* v1;                                                                                                     \
        v1 = &(addend);                                                                                                \
        v0 = &(source);                                                                                                \
        AINET_VECTOR_ADD_BOUND(destination, v0, v1);                                                                   \
    } while (0)
#else
#define AINET_FAST_SQRT_ESTIMATE(destination, source)                                                                  \
    do {                                                                                                               \
        *(int*)&(destination) = (*(int*)&(source) >> 1) + 0x1fc00000;                                                  \
    } while (0)
#define AINET_VECTOR_DOT_XYZ(out, source, factor)                                                                      \
    do {                                                                                                               \
        (out) = (source).x * (factor).x + (source).y * (factor).y + (source).z * (factor).z;                           \
    } while (0)
#define AINET_VECTOR_ADD(destination, source, addend)                                                                  \
    do {                                                                                                               \
        (destination)->x = (source).x + (addend).x;                                                                    \
        (destination)->y = (source).y + (addend).y;                                                                    \
        (destination)->z = (source).z + (addend).z;                                                                    \
    } while (0)
#define AINET_VECTOR_ADD_BOUND(destination, sourcePtr, addendPtr)                                                      \
    AINET_VECTOR_ADD(destination, *(sourcePtr), *(addendPtr))
#define AINET_VECTOR_SUBTRACT(destination, source, subtractor)                                                         \
    do {                                                                                                               \
        (destination)->x = (source)->x - (subtractor)->x;                                                              \
        (destination)->y = (source)->y - (subtractor)->y;                                                              \
        (destination)->z = (source)->z - (subtractor)->z;                                                              \
    } while (0)
#define AINET_FORWARD_PROBE_ADD_WORLD_ASM(dstArg, worldArg, endArg)                                                    \
    do {                                                                                                               \
        (dstArg)->x = (endArg)->x + (worldArg)->x;                                                                     \
        (dstArg)->y = (endArg)->y + (worldArg)->y;                                                                     \
        (dstArg)->z = (endArg)->z + (worldArg)->z;                                                                     \
    } while (0)
#define AINET_PATH_COMPUTE_AUTO_TURN_DELTA(dst, srcVec, world) AINET_VEC3_SUB_WORLD_DST_V0_WORLD_V1(dst, srcVec, world)
#define AINET_PATH_COMPUTE_PATH_TARGET_DELTA(dst, srcVec, world)                                                       \
    AINET_VEC3_SUB_WORLD_DST_V1_WORLD_V0(dst, srcVec, world)
#define AINET_PATH_COMPUTE_LOCAL_PLAYER_DELTA(dst, srcVec, world)                                                      \
    AINET_VEC3_SUB_WORLD_DST_V0_WORLD_V1(dst, srcVec, world)
#define AINET_PATH_COMPUTE_DYNAMIC_OFFSET_DIR(dst, srcVec, world)                                                      \
    do {                                                                                                               \
        (dst).x = (srcVec).x - (world).x;                                                                              \
        (dst).y = (srcVec).y - (world).y;                                                                              \
        (dst).z = (srcVec).z - (world).z;                                                                              \
    } while (0)
#define AINET_PROBE_FAN_COMPUTE_SEGMENT_DELTA(dst, srcVec, world)                                                      \
    AINET_PATH_COMPUTE_DYNAMIC_OFFSET_DIR(dst, srcVec, world)
#define AINET_PROBE_FAN_COMPUTE_SEGMENT_DELTA_KEEP_PTR(dst, srcVec, world, dstPtr)                                     \
    AINET_PATH_COMPUTE_DYNAMIC_OFFSET_DIR(dst, srcVec, world)
#define AINET_PATH_COMPUTE_FORWARD_NODE_DIR(dst, srcVec, world)                                                        \
    AINET_PATH_COMPUTE_DYNAMIC_OFFSET_DIR(dst, srcVec, world)
#define AINET_PATH_DOT_XZ(out, steer, delta) AINET_VEC3_DOT_XZ(out, steer, delta)
#define AINET_PATH_CROSS_XZ(out, steer, delta) AINET_VEC3_CROSS_XZ(out, steer, delta)
#endif
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.tick-ai-mode2-top-level
 * @recoil-artifact defines .text recoil:function:0x401060: AINet::TickAiMode2TopLevel.
 * @recoil-match byte
 *
 * Purpose: Dispatches the active mode-2 top-level state and attack-pursuit transitions. Source model: AINet source-file
 * contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::TickAiMode2TopLevel(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    zUtil_PlayerStateStorage* const localPlayerState = ((zUtil_SaveGameState*)g_GameStateOrMapTable)->playerState;
    playerState->storedTargetPos = localPlayerState->fxOffsetWorld;

    switch (playerState->aiTopLevelState) {
    case kPlayerAiTopPathFollow:
        TickAiMode2PathFollow(saveState);
        if (AiTryEnterMode2AttackPursuitIfLineOfSight(saveState)) {
            AiRebuildSyntheticPathToNodeIfFar(
                saveState,
                playerState->aiCurrentPathNode->neighborNodes[playerState->aiCurrentPathNeighborIndex]
            );
        }
        return;

    case kPlayerAiTopAutoTurn: {
        const int autoTurnActive = playerState->autoTurnActive;
        playerState->steeringInput = 0.0f;
        if (autoTurnActive == 0) {
            playerState->aiTopLevelState = playerState->aiReturnTopLevelState;
        }

        if (AiTryEnterMode2AttackPursuitIfLineOfSight(saveState)) {
            AiRebuildSyntheticPathToNodeIfFar(
                saveState,
                playerState->aiCurrentPathNode->neighborNodes[playerState->aiCurrentPathNeighborIndex]
            );
        }
        return;
    }

    case kPlayerAiMode2TopSteering:
        TickAiMode2SteeringSubstate(saveState);
        return;

    case kPlayerAiTopTurnTowardTarget:
        UpdateAiMode2TurnTowardPlayerNoThrottle(saveState);
        if (AiTryEnterMode2AttackPursuitIfLineOfSight(saveState)) {
            AiRebuildSyntheticPathToNodeIfFar(
                saveState,
                playerState->aiCurrentPathNode->neighborNodes[playerState->aiCurrentPathNeighborIndex]
            );
        }
        return;

    case kPlayerAiTopTurnOnlyTowardTarget:
        UpdateAiMode2TurnInPlaceTowardPlayer(saveState);
        AiTryEnterMode2AttackPursuitIfLineOfSight(saveState);
        return;

    case kPlayerAiTopPathSteering:
        TickAiMode2TimedPathSteering(saveState);
        if (AiTryEnterMode2AttackPursuitIfLineOfSight(saveState)) {
            AiRebuildSyntheticPathToNodeIfFar(
                saveState,
                playerState->aiCurrentPathNode->neighborNodes[playerState->aiCurrentPathNeighborIndex]
            );
        }
        return;

    default:
        return;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.tick-ai-mode2-path-follow
 * @recoil-artifact defines .text recoil:function:0x401180: AINet::TickAiMode2PathFollow.
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.vector-subtract recoil:function:0x401180
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.path-dot-xz recoil:function:0x401180
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.path-cross-xz recoil:function:0x401180
 * @recoil-match byte
 *
 * Original function evidence: retail 0x401180 contains the shared subtraction and the
 * byte-sensitive XZ dot/cross expansions used by this path-follow body.
 * Purpose: Steers toward the current AI path edge, advances the cursor, or arms auto-turn. Source model: AINet
 * source-file contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::TickAiMode2PathFollow(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    AINetNode* currentNode;
    AINetPathProbeFan* edgeProbeFan;
#if !(defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100)
    zVec3* v0;
    zVec3* v1;
#endif
    zVec3* v2;
    zVec3* targetPathNode;
    float steerDotXZ;
    PlayerMasterModalData* masterModalData;
    float targetDistance;
    zVec3 targetDelta;
    zVec3 steerBasis;
    zVec3 autoTurnTargetDelta;

    masterModalData = saveState->primaryModalState->masterModalData;
    currentNode = playerState->aiCurrentPathNode;
    edgeProbeFan = currentNode->probeFans[playerState->aiCurrentPathNeighborIndex];
    targetPathNode = &currentNode->neighborNodes[playerState->aiCurrentPathNeighborIndex]->position;

    if (AiMode2ForwardProbeRequiresAutoTurn(saveState) != 0) {
        AiAdvancePathCursorAndComputeTargetVec(saveState, &currentNode, &edgeProbeFan, &targetDelta);
        targetPathNode = &currentNode->neighborNodes[playerState->aiCurrentPathNeighborIndex]->position;
        playerState->aiReturnTopLevelState = playerState->aiTopLevelState;
        playerState->aiTopLevelState = kPlayerAiTopAutoTurn;
        playerState->autoTurnActive = 1;

        AINET_PATH_COMPUTE_AUTO_TURN_DELTA(autoTurnTargetDelta, targetPathNode, playerState->worldPos);
        autoTurnTargetDelta.y = 0.0f;
        zMath::Vec3NormalizeXZ(&autoTurnTargetDelta, &playerState->autoTurnTargetDir);
        playerState->throttleInput = 0.0f;
        playerState->throttleInputCopy = 0.0f;
        playerState->steeringInput = 0.0f;
        return;
    }

    AINET_PATH_COMPUTE_PATH_TARGET_DELTA(targetDelta, targetPathNode, playerState->worldPos);
    targetDelta.y = 0.0f;
    targetDistance = zMath::Vec3Normalize(&targetDelta);

    steerBasis = playerState->steerBasisNorm;
    AINET_PATH_DOT_XZ(steerDotXZ, steerBasis, targetDelta);
    float steerCrossXZ;
    AINET_PATH_CROSS_XZ(steerCrossXZ, steerBasis, targetDelta);

    if (steerDotXZ < 0.0f) {
        if (playerState->aiPathCursorAdvanceRequested != 0) {
            AiAdvancePathCursorAndComputeTargetVec(saveState, &currentNode, &edgeProbeFan, &targetDelta);
            playerState->aiPathCursorAdvanceRequested = 0;
            TickAiMode2PathFollow(saveState);
            return;
        }

        playerState->throttleInput = 0.0f;
        playerState->steeringInput = (float)(steerCrossXZ < 0.0f ? -1 : 1);
    } else {
        float throttle = 1.0f - (float)(fabs(steerCrossXZ));
        if (throttle <= kPlayerAiPathFollowMinThrottle) {
            throttle = kPlayerAiPathFollowMinThrottle;
        }
        playerState->aiPathCursorAdvanceRequested = 1;
        playerState->throttleInput = throttle;
        playerState->steeringInput = steerCrossXZ;
    }

    playerState->throttleInputCopy = playerState->throttleInput;
    playerState->steeringInputCopy = playerState->steeringInput;

    if (masterModalData->masterType == kPlayerMasterTypeSub) {
        const float pitchInput = ((targetPathNode->y - playerState->worldPos.y + masterModalData->modeAltTransitionTime)
                                         * g_Player_AiMode2_PathFollowPitchInputScale
                                     - playerState->vehiclePitchRad)
            * g_Player_AiMode2_PathFollowPitchTurnGain;
        playerState->subPitchInput = pitchInput;
        playerState->subPitchInputCopy = pitchInput;
    }

    if (targetDistance < 10.0f) {
        AiAdvancePathCursorAndComputeTargetVec(saveState, &currentNode, &edgeProbeFan, &targetDelta);
        playerState->aiPathCursorAdvanceRequested = 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ai-mode2-forward-probe-requires-auto-turn
 * @recoil-artifact defines .text recoil:function:0x401420: AINet::AiMode2ForwardProbeRequiresAutoTurn.
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.forward-probe-add-world recoil:function:0x401420
 * @recoil-match byte
 *
 * Original function evidence: retail 0x401420 contains the fixed-register x87 endpoint
 * addition emitted by the forward-probe macro.
 * Purpose: Checks forward probe queues and requests auto-turn recovery when blocked. Source model: AINet source-file
 * contribution over save-state/playerState, not a Player class.
 */
int __fastcall AINet::AiMode2ForwardProbeRequiresAutoTurn(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* masterModalData = saveState->primaryModalState->masterModalData;
    int segmentTags[2];
    zVec3 forwardDir;
    CZDisplayInstanceSegmentEndpoints segmentPairs[1];

    if (playerState->playerCollisionResolved != 0 || playerState->preferredCollisionResolved != 0) {
        ++playerState->aiMode2SteeringRetryCount;
        return 1;
    }

    segmentPairs[0].start = playerState->worldPos;
    segmentPairs[0].start.y += masterModalData->probePoints[1].y;

    forwardDir = playerState->projectileSpawnVel;

    const float forwardProbeOffset
        = AINET_MAX(zMath::Vec3Normalize(&forwardDir), 1.0f) * kPlayerAiForwardProbeLengthHalfScale
        - masterModalData->probePoints[1].z;
    segmentPairs[0].end.x = forwardProbeOffset * forwardDir.x;
    segmentPairs[0].end.y = forwardProbeOffset * forwardDir.y;
    segmentPairs[0].end.z = forwardProbeOffset * forwardDir.z;
    AINET_FORWARD_PROBE_ADD_WORLD_ASM(&segmentPairs[0].end, &playerState->worldPos, &segmentPairs[0].end);

    segmentTags[0] = -1;
    segmentTags[1] = -1;
    Player::CollectPendingContactsForSegments(saveState, segmentPairs, 2, segmentTags);

    int result;
    if (playerState->preferredCollisionQueue.count != 0 || playerState->playerCollisionQueue.count != 0) {
        result = 1;
    } else {
        result = 0;
    }
    Player::ClearPendingContactQueues(saveState);
    return result;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ai-advance-path-cursor-and-compute-target-vec
 * @recoil-artifact defines .text recoil:function:0x401580: AINet::AiAdvancePathCursorAndComputeTargetVec.
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.vector-subtract recoil:function:0x401580
 * @recoil-match byte
 *
 * Original function evidence: retail 0x401580 contains the shared fixed-register
 * grouped-x87 subtraction expansion.
 * Purpose: Advances the AI path cursor and returns the target vector and probe fan. Source model: AINet source-file
 * contribution over save-state/playerState, not a Player class. Preserve the pre-tested `while (branchOffset < 0x18)`
 * for VC5 byte shape: VC5 folds its initially true entry test and emits the retail direct `jl` latch, while equivalent
 * `do/while` and indefinite-loop/positive-`continue` forms add a two-byte backedge trampoline. The exact original
 * lexical tokens remain unproven.
 */
void __fastcall AINet::AiAdvancePathCursorAndComputeTargetVec(
    zUtil_SaveGameState* saveState,
    AINetNode** currentNodeInOut,
    AINetPathProbeFan** outProbeFan,
    zVec3* outTargetVec
)
{
    zUtil_PlayerStateStorage* playerState = saveState->playerState;
    AINetNode** nodeInOut = currentNodeInOut;
    int chosenBranchIndex;

    int pathNeighborIndex = playerState->aiCurrentPathNeighborIndex;
    AINetNode* nextNode = (*nodeInOut)->neighborNodes[pathNeighborIndex];
    playerState->aiCurrentPathNode = nextNode;

    int previousNodeIndex = (*nodeInOut)->nodeIndex;
    if (previousNodeIndex < 0) {
        (*nodeInOut)->Free();
        *nodeInOut = playerState->aiCurrentPathNode;

        if ((*nodeInOut)->nodeIndex < 0) {
            playerState->aiCurrentPathNeighborIndex = 0;
        } else {
            AINet::AiChooseNextPathBranchIndex(saveState, nodeInOut, &chosenBranchIndex, -1);
            playerState->aiCurrentPathNeighborIndex = chosenBranchIndex;
            if (playerState->aiNet->aiType == AINET_TYPE_HI) {
                playerState->aiTopLevelState = kPlayerAiTopTurnTowardTarget;
            }
        }
    } else {
        *nodeInOut = nextNode;

        int excludedBranchIndex = 4;
        int candidateBranchIndex = 0;
        int branchOffset = 0x0c;
        while (branchOffset < 0x18) {
            AINetNode* reverseNode = *(AINetNode**)((char*)nextNode + branchOffset);
            if (reverseNode != 0 && reverseNode->nodeIndex == previousNodeIndex) {
                excludedBranchIndex = candidateBranchIndex;
                break;
            }

            branchOffset += 4;
            ++candidateBranchIndex;
        }

        AINet::AiChooseNextPathBranchIndex(saveState, nodeInOut, &chosenBranchIndex, excludedBranchIndex);
        playerState->aiCurrentPathNeighborIndex = chosenBranchIndex;
    }

    int index = playerState->aiCurrentPathNeighborIndex;
    *outProbeFan = (*nodeInOut)->probeFans[index];

    zVec3* worldPosition;
    zVec3* selectedPosition;
    worldPosition = &playerState->worldPos;
    selectedPosition = &(*nodeInOut)->position;
    AINET_VECTOR_SUBTRACT(outTargetVec, worldPosition, selectedPosition);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ai-choose-next-path-branch-index
 * @recoil-artifact defines .text recoil:function:0x4016a0: AINet::AiChooseNextPathBranchIndex.
 * @recoil-match byte
 *
 * Purpose: Selects the next non-excluded AI path branch for mode-2 steering.
 */
int __fastcall AINet::AiChooseNextPathBranchIndex(
    zUtil_SaveGameState* saveState,
    AINetNode** currentNodeInOut,
    int* outBranchIndex,
    int excludedBranchIndex
)
{
    (void)saveState;

    AINetNode* currentNode = *currentNodeInOut;
    int branchCount = 0;
    AINetNode** neighborSlot = currentNode->neighborNodes;
    for (int branchIndex = 0; branchIndex < 3; ++branchIndex) {
        if (neighborSlot[branchIndex] != 0) {
            ++branchCount;
        }
    }

    if (branchCount == 1) {
        *outBranchIndex = 0;
        return 1;
    }

    if (branchCount == 2) {
        *outBranchIndex = 0;
    } else {
        *outBranchIndex = rand() % branchCount;
    }

    if (*outBranchIndex == excludedBranchIndex) {
        *outBranchIndex = (*outBranchIndex + 1) % branchCount;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.tick-ai-mode2-steering-substate
 * @recoil-artifact defines .text recoil:function:0x401710: AINet::TickAiMode2SteeringSubstate.
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.vector-subtract recoil:function:0x401710
 * @recoil-match byte
 *
 * Original function evidence: retail 0x401710 contains the shared fixed-register
 * grouped-x87 subtraction expansion.
 * Purpose: Runs pursuit steering, submarine vertical controls, and pursuit exit checks. Source model: AINet source-file
 * contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::TickAiMode2SteeringSubstate(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    float forwardDot;
    float verticalDistanceScale;
    float targetDistance;
    float lateralDot;
    PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;
    const zVec3 targetWorldSnapshot = ((zUtil_PlayerStateStorage*)g_GameStateOrMapTable->playerState)->worldPos;

    if (g_Player_TotalTimeSecScaled >= playerState->aiNextPathRebuildTime
        && playerState->aiCurrentSteeringSubstate != kPlayerAiMode2SteerPathFollow) {
        AiRebuildSyntheticPathToNodeIfFar(saveState, playerState->aiCurrentPathNode);
    }

    zVec3 targetDelta;
    {
        zVec3* v0;
        zVec3* v1;
        const zVec3* v2;
        v0 = &targetDelta;
        v1 = &playerState->worldPos;
        v2 = &targetWorldSnapshot;
        AINET_VECTOR_SUBTRACT(v0, v2, v1);
    }
    verticalDistanceScale = targetDelta.y;
    targetDelta.y = 0.0f;
    targetDistance = zMath::Vec3Normalize(&targetDelta);
    verticalDistanceScale = targetDistance != 0.0f ? verticalDistanceScale / targetDistance : 0.0f;

    const zVec3 steerBasisNorm = playerState->steerBasisNorm;
    lateralDot = steerBasisNorm.z * targetDelta.x - steerBasisNorm.x * targetDelta.z;
    forwardDot = steerBasisNorm.x * targetDelta.x + steerBasisNorm.z * targetDelta.z;

    if (playerState->aiMode2SteeringRetryCount > 6) {
        playerState->aiCurrentSteeringSubstate = kPlayerAiMode2SteerTurnInPlace;
    }

    switch (playerState->aiCurrentSteeringSubstate) {
    case kPlayerAiMode2SteerDirectTarget:
        UpdateAiMode2MoveAndTurnTowardTarget(saveState, forwardDot, lateralDot, targetDistance);
        break;
    case kPlayerAiMode2SteerOffsetTarget:
        TickAiMode2OffsetTargetSteering(saveState, forwardDot, lateralDot, targetDistance);
        forwardDot = 1.0f;
        break;
    case kPlayerAiMode2SteerDynamicOffsetTarget:
        TickAiMode2DynamicOffsetTargetSteering(saveState, forwardDot, lateralDot, targetDistance);
        forwardDot = 1.0f;
        break;
    case kPlayerAiMode2SteerAutoTurn:
        if (playerState->autoTurnActive == 0) {
            playerState->aiCurrentSteeringSubstate = playerState->aiReturnSteeringSubstate;
        }
        forwardDot = 1.0f;
        break;
    case kPlayerAiMode2SteerTurnInPlace:
        UpdateAiMode2TurnInPlaceTowardPlayer(saveState);
        forwardDot = 1.0f;
        break;
    case kPlayerAiMode2SteerPathFollow:
        TickAiMode2PathFollow(saveState);
        forwardDot = 1.0f;
        break;
    default:
        break;
    }

    if (masterModalData->masterType == kPlayerMasterTypeSub) {
        float pitchInput = g_Player_AiMode2_SteeringPitchInputScale;
        pitchInput *= verticalDistanceScale;
        pitchInput = (pitchInput - playerState->vehiclePitchRad) * g_Player_AiMode2_SteeringPitchTurnGain;
        playerState->subPitchInput = pitchInput;
        playerState->subPitchInputCopy = pitchInput;

        const float verticalInput
            = (targetWorldSnapshot.y - playerState->worldPos.y) * g_Player_AiMode2_SteeringVerticalErrorScale;
        playerState->subVerticalInput = verticalInput;
        playerState->subVerticalInputCopy = verticalInput;
    }

    TickAiMode2AltGunAttackWindow(saveState, targetDistance, forwardDot);

    zUtil_PlayerStateStorage* targetPlayerState = (zUtil_PlayerStateStorage*)g_GameStateOrMapTable->playerState;
    if (targetPlayerState->lifecycleState == kPlayerLifecycleInactive
        || zMath::Vec3DeltaLengthSq(&playerState->worldPos, &playerState->aiRestoreTarget)
            > playerState->aiRestoreDistanceSq) {
        AiRestoreSavedTopLevelState(saveState);
        playerState->aiStateUntilTime = g_Player_TotalTimeSecScaled + playerState->aiNotPursuitDwell;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.update-ai-mode2-move-and-turn-toward-target
 * @recoil-artifact defines .text recoil:function:0x401970: AINet::UpdateAiMode2MoveAndTurnTowardTarget.
 * @recoil-match byte
 *
 * Purpose: Converts target alignment and pursuit distance into throttle and steering input. Source model: AINet
 * source-file contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::UpdateAiMode2MoveAndTurnTowardTarget(
    zUtil_SaveGameState* saveState,
    float forwardDot,
    float lateralDot,
    float targetDistance
)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;

    if (forwardDot <= 0.0f) {
        playerState->throttleInput = 0.0f;
        const int steeringDirection = lateralDot < 0.0f ? -1 : 1;
        playerState->steeringInput = (float)steeringDirection;
    } else {
        playerState->steeringInput = lateralDot;
        if (targetDistance > playerState->aiNet->pursuitParam1) {
            playerState->throttleInput = 1.0f;
        } else if (targetDistance < playerState->aiNet->pursuitParam0) {
            playerState->throttleInput = -1.0f;
        } else {
            playerState->throttleInput = 0.0f;
        }
    }

    playerState->throttleInputCopy = playerState->throttleInput;
    playerState->steeringInputCopy = playerState->steeringInput;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.tick-ai-mode2-offset-target-steering
 * @recoil-artifact defines .text recoil:function:0x401a40: AINet::TickAiMode2OffsetTargetSteering.
 * @recoil-match byte
 *
 * Purpose: Runs offset-target pursuit or switches to auto-turn recovery when blocked. Source model: AINet source-file
 * contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::TickAiMode2OffsetTargetSteering(
    zUtil_SaveGameState* saveState,
    float unusedForwardDot,
    float unusedLateralDot,
    float unusedTargetDistance
)
{
    (void)unusedForwardDot;
    (void)unusedLateralDot;
    (void)unusedTargetDistance;

    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    if (AiMode2ForwardProbeRequiresAutoTurn(saveState) == 0) {
        UpdateAiMode2MoveAndTurnTowardOffsetTarget(saveState, (zUtil_SaveGameState*)g_GameStateOrMapTable);
        return;
    }

    Player::SetAutoTurnTargetDirFromWorldPoint(
        saveState,
        &((zUtil_SaveGameState*)g_GameStateOrMapTable)->playerState->worldPos
    );

    const int currentSteeringSubstate = playerState->aiCurrentSteeringSubstate;
    playerState->steeringInputCopy = 0.0f;
    playerState->steeringInput = 0.0f;
    playerState->throttleInputCopy = 0.0f;
    playerState->throttleInput = 0.0f;
    playerState->aiReturnSteeringSubstate = currentSteeringSubstate;
    playerState->aiCurrentSteeringSubstate = kPlayerAiMode2SteerAutoTurn;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.tick-ai-mode2-dynamic-offset-target-steering
 * @recoil-artifact defines .text recoil:function:0x401ab0: AINet::TickAiMode2DynamicOffsetTargetSteering.
 * @recoil-match byte
 *
 * Purpose: Runs dynamic-offset pursuit or switches to auto-turn recovery when blocked. Source model: AINet source-file
 * contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::TickAiMode2DynamicOffsetTargetSteering(
    zUtil_SaveGameState* saveState,
    float unusedForwardDot,
    float unusedLateralDot,
    float targetDistance
)
{
    (void)unusedForwardDot;
    (void)unusedLateralDot;

    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    if (AiMode2ForwardProbeRequiresAutoTurn(saveState) == 0) {
        UpdateAiMode2MoveAndTurnTowardDynamicOffsetTarget(
            saveState,
            (zUtil_SaveGameState*)g_GameStateOrMapTable,
            targetDistance
        );
        return;
    }

    Player::SetAutoTurnTargetDirFromWorldPoint(
        saveState,
        &((zUtil_SaveGameState*)g_GameStateOrMapTable)->playerState->worldPos
    );

    const int currentSteeringSubstate = playerState->aiCurrentSteeringSubstate;
    playerState->steeringInputCopy = 0.0f;
    playerState->steeringInput = 0.0f;
    playerState->throttleInputCopy = 0.0f;
    playerState->throttleInput = 0.0f;
    playerState->aiReturnSteeringSubstate = currentSteeringSubstate;
    playerState->aiCurrentSteeringSubstate = kPlayerAiMode2SteerAutoTurn;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ai-try-enter-mode2-attack-pursuit-if-line-of-sight
 * @recoil-artifact defines .text recoil:function:0x401b20: AINet::AiTryEnterMode2AttackPursuitIfLineOfSight.
 * @recoil-match byte
 *
 * Purpose: Tests attack range and local-player line of sight before steering pursuit. Source model: AINet source-file
 * contribution over save-state/playerState, not a Player class.
 */
int __fastcall AINet::AiTryEnterMode2AttackPursuitIfLineOfSight(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const aiState = saveState->playerState;
    if (g_Player_AiMode2State1Finalized == 0) {
        if (g_Player_TotalTimeSecScaled > aiState->aiStateUntilTime) {
            zUtil_PlayerStateStorage* const localPlayerState
                = ((zUtil_SaveGameState*)g_GameStateOrMapTable)->playerState;
            const float targetDistSq
                = zMath::Vec3DeltaLengthSq(&localPlayerState->fxOffsetWorld, &aiState->fxOffsetWorld);
            if (targetDistSq < aiState->aiAttackRadiusSq) {
                zVec3 lineOfSightPoint = aiState->fxOffsetWorld;
                lineOfSightPoint.y -= kPlayerAiAttackLosTargetYOffset;
                if (HasLineOfSightFromLocalPlayerFxOffset(aiState->rootNode, &lineOfSightPoint, 1) != 0) {
                    AiEnterMode2SteeringPursuit(saveState);
                    aiState->aiTargetLineOfSightClear = 1;
                    if (aiState->aiNet->attackBuddyNetId != 0) {
                        AiAlertAttackBuddies(saveState);
                    }
                    aiState->aiMode2SteeringRetryCount = 0;
                    return 1;
                } else {
                    aiState->aiTargetLineOfSightClear = 0;
                }
            }
        }
    }
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ai-alert-attack-buddies
 * @recoil-artifact defines .text recoil:function:0x401c00: AINet::AiAlertAttackBuddies.
 * @recoil-match byte
 *
 * Purpose: Propagates an attack-pursuit alert around the AI peer ring. Source model: AINet source-file contribution
 * over save-state/playerState, not a Player class.
 */
void __fastcall AINet::AiAlertAttackBuddies(zUtil_SaveGameState* saveState)
{
    zUtil_SaveGameState* buddySaveState = saveState->aiPeerRingNext;
    if (g_Player_AiMode2State1Finalized != 0 || buddySaveState == saveState) {
        return;
    }

    do {
        if (buddySaveState->playerState->aiTopLevelState != kPlayerAiMode2TopSteering) {
            AiEnterMode2SteeringPursuit(buddySaveState);
            buddySaveState->playerState->recentHitFlag = 1;
            buddySaveState->playerState->recentHitExpireTime = g_Time_AccumulatedTimeSec + 10.0f;
        }
        buddySaveState = buddySaveState->aiPeerRingNext;
    } while (buddySaveState != saveState);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ai-enter-mode2-steering-pursuit
 * @recoil-artifact defines .text recoil:function:0x401c60: AINet::AiEnterMode2SteeringPursuit.
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.vector-subtract recoil:function:0x401c60
 * @recoil-match byte
 *
 * Original function evidence: retail 0x401c60 contains the shared fixed-register
 * grouped-x87 subtraction expansion.
 * Purpose: Saves the prior top-level state and enters steering pursuit for the attack window. Source model: AINet
 * source-file contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::AiEnterMode2SteeringPursuit(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const aiState = saveState->playerState;
    zUtil_PlayerStateStorage* const localPlayerState = ((zUtil_SaveGameState*)g_GameStateOrMapTable)->playerState;
    if (g_Player_AiMode2State1Finalized != 0) {
        return;
    }

    const int previousTopLevelState = aiState->aiTopLevelState;
    aiState->aiSavedTopLevelState = previousTopLevelState;
    aiState->aiStateStartTime = g_Player_TotalTimeSecScaled;
    aiState->aiStateEndTime = aiState->aiMode2AttackDwell + g_Player_TotalTimeSecScaled;
    if (previousTopLevelState != kPlayerAiMode2TopSteering) {
        aiState->aiTopLevelState = kPlayerAiMode2TopSteering;
    }

    AINetNode* const restorePathNode = aiState->aiCurrentPathNode->neighborNodes[aiState->aiCurrentPathNeighborIndex];
    aiState->aiRestoreTarget = restorePathNode->position;

    switch (aiState->aiCurrentSteeringSubstate) {
    case kPlayerAiMode2SteerDirectTarget:
        return;
    case kPlayerAiMode2SteerDynamicOffsetTarget:
        break;
    default:
        return;
    }

#if !(defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100)
    zVec3* v0;
    zVec3* v1;
#endif
    AINET_PATH_COMPUTE_DYNAMIC_OFFSET_DIR(aiState->aiDynamicOffsetDir, aiState->worldPos, localPlayerState->worldPos);
    aiState->aiDynamicOffsetDir.y = 0.0f;
    zMath::Vec3Normalize(&aiState->aiDynamicOffsetDir);
}

#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100

/*
 * Stack shape of the hand-authored frame below.  A naked body manages its own
 * frame, so this type lets every slot access name the member it touches rather
 * than compute a displacement.  It describes the frame only; it is never
 * instantiated and emits no code.
 */
struct AiNetLosFxFrame {
    const zVec3* savedEdi;
    zUtil_PlayerStateStorage* savedEsi;
    CZNodePartial* savedEbx;
    PlayerProbeSampleCandidateBuffer rayData;
    void* returnAddress;
    int directionMode;
};

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.has-line-of-sight-from-local-player-fx-offset
 * @recoil-artifact defines .text recoil:function:0x401d50: AINet::HasLineOfSightFromLocalPlayerFxOffset.
 * @recoil-raw-asm recoil:raw-asm:battlesport.ai-net.los-from-local-player-fx-offset
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.los-from-local-player-fx-offset recoil:function:0x401d50
 * @recoil-match byte
 *
 * Original function evidence: retail 0x401d50 is the authored AINet
 * local-player fx-offset line-of-sight probe; this body is its exact
 * reconstruction.
 * Raw-assembly evidence: retail 0x401d50 keeps EDX = &rayData live from before
 * the direction branch through both argument arms into the tail-merged
 * RaycastFindClosest call, and omits the frame pointer.  VC5SP3 C++ cannot
 * express that.  Roughly 140 governed probes covering local pointers, const
 * pointers, C++ references, declaration order, address-taken-early forms,
 * static homing, single-call selected-pointer shapes, non-static member and
 * reference-formal call models, array decay, and twenty optimiser/CPU flag
 * profiles all failed: the compiler either folds the selector test into
 * `cmp dword ptr [esp+0x514],1` and sinks the address to the merged block, or
 * hoists the g_Player_RuntimeDiScene load into that slot instead.  A partial
 * inline-asm region cannot help either, because any __asm in this function
 * forces VC5 to establish an EBP frame that retail does not have, which
 * changes the body from its first byte.  A naked body is therefore the only
 * source form that reproduces the retail bytes, and it is used here under an
 * explicit user authorisation recorded in .agent/RAW_ASSEMBLY_ALLOWLIST.txt.
 * The portable #else arm below carries the equivalent readable C++.
 * Purpose: tests whether the active local player fx-offset position has an
 * unobstructed ray path to the supplied point while temporarily excluding the
 * tested node and local player root from raycast candidates.
 */
__declspec(naked) int __fastcall AINet::HasLineOfSightFromLocalPlayerFxOffset(
    CZNodePartial* node,
    const zVec3* point,
    int directionMode
)
{
    using CZClass::gwNodeSetRaycastable;
    using CZDisplayInstance::RaycastFindClosest;
    using CZDisplayInstance::SetBreakOnFirstCandidate;
    using CZDisplayInstance::SetStopAfterFirstHit;
    /*
     * Frame: [rayData][ebx][esi][edi][return][directionMode].  esi holds the
     * player state, edi the supplied point, ebx the tested node.
     */
    __asm {
        mov     eax, dword ptr [g_GameStateOrMapTable]
        sub     esp, SIZE PlayerProbeSampleCandidateBuffer
        push    ebx
        push    esi
        mov     esi, dword ptr [eax]zInput_GameStateOrMapTablePartial.playerState
        mov     ebx, ecx
        push    edi
        mov     edi, edx
        mov     ecx, dword ptr [esi]zUtil_PlayerStateStorage.variantTag
        xor     edx, edx
        mov     dword ptr [g_Variant_CurrentTag], ecx
        mov     ecx, ebx
        call    gwNodeSetRaycastable
        mov     ecx, dword ptr [esi]zUtil_PlayerStateStorage.rootNode
        xor     edx, edx
        call    gwNodeSetRaycastable
        mov     ecx, 1
        call    SetBreakOnFirstCandidate
        mov     ecx, 40000h
        call    SetStopAfterFirstHit
        mov     eax, dword ptr [esp]AiNetLosFxFrame.directionMode
        lea     edx, [esp]AiNetLosFxFrame.rayData
        cmp     eax, 1
        jne     losfx_reverse
        mov     eax, dword ptr [edi]zVec3.z
        mov     ecx, dword ptr [edi]zVec3.y
        push    eax
        mov     eax, dword ptr [edi]zVec3.x
        push    ecx
        mov     ecx, dword ptr [esi]zUtil_PlayerStateStorage.fxOffsetWorld.z
        push    eax
        mov     eax, dword ptr [esi]zUtil_PlayerStateStorage.fxOffsetWorld.y
        push    ecx
        mov     ecx, dword ptr [esi]zUtil_PlayerStateStorage.fxOffsetWorld.x
        push    eax
        jmp     losfx_merge
    losfx_reverse:
        mov     eax, dword ptr [esi]zUtil_PlayerStateStorage.fxOffsetWorld.z
        mov     ecx, dword ptr [esi]zUtil_PlayerStateStorage.fxOffsetWorld.y
        push    eax
        mov     eax, dword ptr [esi]zUtil_PlayerStateStorage.fxOffsetWorld.x
        push    ecx
        mov     ecx, dword ptr [edi]zVec3.z
        push    eax
        mov     eax, dword ptr [edi]zVec3.y
        push    ecx
        mov     ecx, dword ptr [edi]zVec3.x
        push    eax
    losfx_merge:
        push    ecx
        mov     ecx, dword ptr [g_Player_RuntimeDiScene]
        call    RaycastFindClosest
        xor     ecx, ecx
        mov     edi, eax
        call    SetBreakOnFirstCandidate
        mov     ecx, dword ptr [esi]zUtil_PlayerStateStorage.rootNode
        mov     edx, 1
        call    gwNodeSetRaycastable
        mov     edx, 1
        mov     ecx, ebx
        call    gwNodeSetRaycastable
        test    edi, edi
        jne     losfx_clear
        mov     eax, dword ptr [esp]AiNetLosFxFrame.rayData.candidateCount
        test    eax, eax
        je      losfx_clear
        xor     eax, eax
        pop     edi
        pop     esi
        pop     ebx
        add     esp, SIZE PlayerProbeSampleCandidateBuffer
        ret     4
    losfx_clear:
        pop     edi
        pop     esi
        mov     eax, 1
        pop     ebx
        add     esp, SIZE PlayerProbeSampleCandidateBuffer
        ret     4
    }
}

#else

/**
 * Purpose: tests whether the active local player fx-offset position has an
 * unobstructed ray path to the supplied point while temporarily excluding the
 * tested node and local player root from raycast candidates.  Portable
 * equivalent of the VC5SP3 naked body above; behaviour is identical and only
 * the emitted instruction schedule differs.
 */
int __fastcall AINet::HasLineOfSightFromLocalPlayerFxOffset(CZNodePartial* node, const zVec3* point, int directionMode)
{
    zUtil_PlayerStateStorage* const playerState = (zUtil_PlayerStateStorage*)(g_GameStateOrMapTable->playerState);

    g_Variant_CurrentTag = playerState->variantTag;
    CZClass::gwNodeSetRaycastable(node, 0);
    CZClass::gwNodeSetRaycastable(playerState->rootNode, 0);
    CZDisplayInstance::SetBreakOnFirstCandidate(1);
    CZDisplayInstance::SetStopAfterFirstHit(0x40000);

    PlayerProbeSampleCandidateBuffer rayData;
    if (directionMode == 1) {
        directionMode = CZDisplayInstance::RaycastFindClosest(
            g_Player_RuntimeDiScene,
            playerState->fxOffsetWorld.x,
            playerState->fxOffsetWorld.y,
            playerState->fxOffsetWorld.z,
            point->x,
            point->y,
            point->z,
            &rayData
        );
    } else {
        directionMode = CZDisplayInstance::RaycastFindClosest(
            g_Player_RuntimeDiScene,
            point->x,
            point->y,
            point->z,
            playerState->fxOffsetWorld.x,
            playerState->fxOffsetWorld.y,
            playerState->fxOffsetWorld.z,
            &rayData
        );
    }
    CZDisplayInstance::SetBreakOnFirstCandidate(0);
    CZClass::gwNodeSetRaycastable(playerState->rootNode, 1);
    CZClass::gwNodeSetRaycastable(node, 1);
    if (directionMode == 0 && rayData.candidateCount != 0) {
        return 0;
    }
    return 1;
}

#endif

#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100

/*
 * Stack shape of the hand-authored camera-target frame.  The argument-push
 * sequence walks esp down one slot at a time, so the shifted views below let
 * each access keep naming the member it reads instead of reintroducing a
 * displacement.  These types describe the frame only; none is instantiated and
 * none emits code.
 */
struct AiNetLosCamLocals {
    zVec3 cameraTarget;
    PlayerProbeSampleCandidateBuffer rayData;
};
struct AiNetLosCamFrame {
    zUtil_PlayerStateStorage* savedEdi;
    const zVec3* savedEsi;
    CZNodePartial* savedEbx;
    AiNetLosCamLocals locals;
    void* returnAddress;
    int directionMode;
};
struct AiNetLosCamFrameBeforeEdi {
    const zVec3* savedEsi;
    CZNodePartial* savedEbx;
    AiNetLosCamLocals locals;
};
struct AiNetLosCamFrame1 {
    void* pushed[1];
    AiNetLosCamFrame frame;
};
struct AiNetLosCamFrame2 {
    void* pushed[2];
    AiNetLosCamFrame frame;
};
struct AiNetLosCamFrame3 {
    void* pushed[3];
    AiNetLosCamFrame frame;
};
struct AiNetLosCamFrame4 {
    void* pushed[4];
    AiNetLosCamFrame frame;
};

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.has-line-of-sight-from-camera-target
 * @recoil-artifact defines .text recoil:function:0x401e50: AINet::HasLineOfSightFromCameraTarget.
 * @recoil-raw-asm recoil:raw-asm:battlesport.ai-net.los-from-camera-target
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.los-from-camera-target recoil:function:0x401e50
 * @recoil-match byte
 *
 * Original function evidence: retail 0x401e50 is the authored AINet
 * camera-target line-of-sight probe; this body is its exact reconstruction.
 * Raw-assembly evidence: retail 0x401e50 diverges from every VC5SP3 C++ form at
 * exactly the same construct as its sibling 0x401d50 - it establishes
 * EDX = &rayData before the direction branch, keeps it live through both
 * argument arms into the tail-merged RaycastFindClosest call, and omits the
 * frame pointer.  The same governed probe programme that failed for 0x401d50
 * applies here: VC5 folds the selector test into
 * `cmp dword ptr [esp+0x520],1` and sinks the address past the six argument
 * pushes with a compensated displacement.  A partial inline-asm region is not
 * an option because any __asm forces an EBP frame this body does not have.
 * A naked body is therefore the only source form that reproduces the retail
 * bytes, used under the explicit user authorisation recorded in
 * .agent/RAW_ASSEMBLY_ALLOWLIST.txt.  The portable #else arm below carries the
 * equivalent readable C++.
 * Purpose: tests whether the active camera target has an unobstructed ray path
 * to the supplied point while temporarily excluding the tested node and local
 * player root from raycast candidates.
 */
__declspec(naked) int __fastcall AINet::HasLineOfSightFromCameraTarget(
    CZNodePartial* node,
    const zVec3* point,
    int directionMode
)
{
    using CZCamera::gwCameraGetPosition;
    using CZClass::gwNodeSetRaycastable;
    using CZDisplayInstance::RaycastFindClosest;
    using CZDisplayInstance::SetBreakOnFirstCandidate;
    using CZDisplayInstance::SetStopAfterFirstHit;
    /* edi holds the player state, esi the supplied point, ebx the tested node. */
    __asm {
        sub     esp, SIZE AiNetLosCamLocals
        mov     eax, dword ptr [g_GameStateOrMapTable]
        push    ebx
        push    esi
        mov     ebx, ecx
        mov     esi, edx
        lea     ecx, [esp]AiNetLosCamFrameBeforeEdi.locals.cameraTarget.z
        push    edi
        mov     edi, dword ptr [eax]zInput_GameStateOrMapTablePartial.playerState
        lea     edx, [esp]AiNetLosCamFrame.locals.cameraTarget.y
        push    ecx
        mov     ecx, dword ptr [g_MainCamera]
        push    edx
        lea     edx, [esp]AiNetLosCamFrame2.frame.locals.cameraTarget.x
        call    gwCameraGetPosition
        mov     eax, dword ptr [edi]zUtil_PlayerStateStorage.variantTag
        xor     edx, edx
        mov     ecx, ebx
        mov     dword ptr [g_Variant_CurrentTag], eax
        call    gwNodeSetRaycastable
        mov     ecx, dword ptr [edi]zUtil_PlayerStateStorage.rootNode
        xor     edx, edx
        call    gwNodeSetRaycastable
        mov     ecx, 1
        call    SetBreakOnFirstCandidate
        mov     ecx, 40000h
        call    SetStopAfterFirstHit
        mov     eax, dword ptr [esp]AiNetLosCamFrame.directionMode
        lea     edx, [esp]AiNetLosCamFrame.locals.rayData
        cmp     eax, 1
        jne     loscam_reverse
        mov     ecx, dword ptr [esi]zVec3.z
        mov     eax, dword ptr [esi]zVec3.y
        push    ecx
        mov     ecx, dword ptr [esi]zVec3.x
        push    eax
        mov     eax, dword ptr [esp]AiNetLosCamFrame2.frame.locals.cameraTarget.z
        push    ecx
        mov     ecx, dword ptr [esp]AiNetLosCamFrame3.frame.locals.cameraTarget.y
        push    eax
        mov     eax, dword ptr [esp]AiNetLosCamFrame4.frame.locals.cameraTarget.x
        push    ecx
        jmp     loscam_merge
    loscam_reverse:
        mov     ecx, dword ptr [esp]AiNetLosCamFrame.locals.cameraTarget.z
        mov     eax, dword ptr [esp]AiNetLosCamFrame.locals.cameraTarget.y
        push    ecx
        mov     ecx, dword ptr [esp]AiNetLosCamFrame1.frame.locals.cameraTarget.x
        push    eax
        mov     eax, dword ptr [esi]zVec3.z
        push    ecx
        mov     ecx, dword ptr [esi]zVec3.y
        push    eax
        mov     eax, dword ptr [esi]zVec3.x
        push    ecx
    loscam_merge:
        mov     ecx, dword ptr [g_Player_RuntimeDiScene]
        push    eax
        call    RaycastFindClosest
        xor     ecx, ecx
        mov     esi, eax
        call    SetBreakOnFirstCandidate
        mov     ecx, dword ptr [edi]zUtil_PlayerStateStorage.rootNode
        mov     edx, 1
        call    gwNodeSetRaycastable
        mov     edx, 1
        mov     ecx, ebx
        call    gwNodeSetRaycastable
        test    esi, esi
        jne     loscam_clear
        mov     eax, dword ptr [esp]AiNetLosCamFrame.locals.rayData.candidateCount
        test    eax, eax
        je      loscam_clear
        xor     eax, eax
        pop     edi
        pop     esi
        pop     ebx
        add     esp, SIZE AiNetLosCamLocals
        ret     4
    loscam_clear:
        pop     edi
        pop     esi
        mov     eax, 1
        pop     ebx
        add     esp, SIZE AiNetLosCamLocals
        ret     4
    }
}

#else

/**
 * Purpose: tests whether the active camera target has an unobstructed ray path
 * to the supplied point while temporarily excluding the tested node and local
 * player root from raycast candidates.  Portable equivalent of the VC5SP3
 * naked body above; behaviour is identical and only the emitted instruction
 * schedule differs.
 */
int __fastcall AINet::HasLineOfSightFromCameraTarget(CZNodePartial* node, const zVec3* point, int directionMode)
{
    zUtil_PlayerStateStorage* const playerState = (zUtil_PlayerStateStorage*)(g_GameStateOrMapTable->playerState);

    zVec3 cameraTarget;
    CZCamera::gwCameraGetPosition(g_MainCamera, &cameraTarget.x, &cameraTarget.y, &cameraTarget.z);

    g_Variant_CurrentTag = playerState->variantTag;
    CZClass::gwNodeSetRaycastable(node, 0);
    CZClass::gwNodeSetRaycastable(playerState->rootNode, 0);
    CZDisplayInstance::SetBreakOnFirstCandidate(1);
    CZDisplayInstance::SetStopAfterFirstHit(0x40000);

    PlayerProbeSampleCandidateBuffer rayData;
    int raycastResult;
    if (directionMode == 1) {
        raycastResult = CZDisplayInstance::RaycastFindClosest(
            g_Player_RuntimeDiScene,
            cameraTarget.x,
            cameraTarget.y,
            cameraTarget.z,
            point->x,
            point->y,
            point->z,
            &rayData
        );
    } else {
        raycastResult = CZDisplayInstance::RaycastFindClosest(
            g_Player_RuntimeDiScene,
            point->x,
            point->y,
            point->z,
            cameraTarget.x,
            cameraTarget.y,
            cameraTarget.z,
            &rayData
        );
    }

    CZDisplayInstance::SetBreakOnFirstCandidate(0);
    CZClass::gwNodeSetRaycastable(playerState->rootNode, 1);
    CZClass::gwNodeSetRaycastable(node, 1);

    return raycastResult == 0 && rayData.candidateCount != 0 ? 0 : 1;
}

#endif

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ai-rebuild-synthetic-path-to-node-if-far
 * @recoil-artifact defines .text recoil:function:0x401f60: AINet::AiRebuildSyntheticPathToNodeIfFar.
 * @recoil-match byte
 *
 * Purpose: Builds a temporary synthetic AI path node back to the requested target. Source model: AINet source-file
 * contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::AiRebuildSyntheticPathToNodeIfFar(zUtil_SaveGameState* saveState, AINetNode* targetNode)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;

    zVec3 playerPos = playerState->worldPos;
    zVec3 nodePos = playerState->aiCurrentPathNode->position;
    if (zMath::Vec3DeltaLengthSq(&playerPos, &nodePos) < kPlayerAiSyntheticPathRebuildDistanceSq) {
        return;
    }

    AINetNode* const syntheticNode = (AINetNode*)(malloc(sizeof(AINetNode)));
    memset(syntheticNode, 0, sizeof(*syntheticNode));
    syntheticNode->neighborNodes[0] = targetNode;
    syntheticNode->position = playerState->worldPos;
    syntheticNode->nodeIndex = -1;

    AINetPathProbeFan* const fan = (AINetPathProbeFan*)(malloc(sizeof(AINetPathProbeFan)));
    syntheticNode->probeFans[0] = fan;
    memset(fan, 0, sizeof(*fan));
    syntheticNode->probeFans[0]
        ->InitFromSegment(syntheticNode->position, playerState->aiCurrentPathNode->position, 10.0f);

    playerState->aiCurrentPathNode = syntheticNode;
    playerState->aiCurrentPathNeighborIndex = 0;
    playerState->aiNextPathRebuildTime = g_Player_TotalTimeSecScaled - kPlayerAiSyntheticPathRebuildDelaySec;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ai-restore-saved-top-level-state
 * @recoil-artifact defines .text recoil:function:0x402080: AINet::AiRestoreSavedTopLevelState.
 * @recoil-match byte
 *
 * BN shows a fastcall leaf that copies playerState->aiSavedTopLevelState to
 * playerState->aiTopLevelState through the save-state's playerState pointer.
 * Purpose: Restores a saved AI top-level state for one player save-state node.
 */
void __fastcall AINet::AiRestoreSavedTopLevelState(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    playerState->aiTopLevelState = playerState->aiSavedTopLevelState;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.update-ai-mode2-turn-toward-player-no-throttle
 * @recoil-artifact defines .text recoil:function:0x402090: AINet::UpdateAiMode2TurnTowardPlayerNoThrottle.
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.vector-subtract recoil:function:0x402090
 * @recoil-match byte
 *
 * Original function evidence: retail 0x402090 contains the shared fixed-register
 * grouped-x87 subtraction expansion.
 * Purpose: Turns toward the local player while holding throttle at zero. Source model: AINet source-file contribution
 * over save-state/playerState, not a Player class.
 */
void __fastcall AINet::UpdateAiMode2TurnTowardPlayerNoThrottle(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
#if !(defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100)
    zVec3* v0;
    zVec3* v1;
#endif

    zVec3 targetDelta;

    AINET_PATH_COMPUTE_LOCAL_PLAYER_DELTA(
        targetDelta,
        &((zUtil_SaveGameState*)g_GameStateOrMapTable)->playerState->worldPos,
        playerState->worldPos
    );
    targetDelta.y = 0.0f;
    zMath::Vec3Normalize(&targetDelta);

    const zVec3 steerBasis = playerState->steerBasisNorm;
    const float turnCross = steerBasis.z * targetDelta.x - steerBasis.x * targetDelta.z;
    const float forwardDot = steerBasis.x * targetDelta.x + steerBasis.z * targetDelta.z;
    int turnDirection;
    if (forwardDot <= 0.0f) {
        turnDirection = -1;
        if (turnCross >= 0.0f) {
            turnDirection = 1;
        }
        playerState->steeringInput = (float)turnDirection;
    } else {
        playerState->steeringInput = turnCross;
    }

    playerState->throttleInput = 0.0f;
    playerState->throttleInputCopy = 0.0f;
    playerState->steeringInputCopy = playerState->steeringInput;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.update-ai-mode2-turn-in-place-toward-player
 * @recoil-artifact defines .text recoil:function:0x402170: AINet::UpdateAiMode2TurnInPlaceTowardPlayer.
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.vector-subtract recoil:function:0x402170
 * @recoil-match byte
 *
 * Original function evidence: retail 0x402170 contains the shared fixed-register
 * grouped-x87 subtraction expansion.
 * Purpose: Turns in place toward the local player without changing throttle. Source model: AINet source-file
 * contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::UpdateAiMode2TurnInPlaceTowardPlayer(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
#if !(defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100)
    zVec3* v0;
    zVec3* v1;
#endif

    zVec3 targetDelta;

    AINET_PATH_COMPUTE_LOCAL_PLAYER_DELTA(
        targetDelta,
        &((zUtil_SaveGameState*)g_GameStateOrMapTable)->playerState->worldPos,
        playerState->worldPos
    );
    targetDelta.y = 0.0f;
    zMath::Vec3Normalize(&targetDelta);

    const zVec3 steerBasis = playerState->steerBasisNorm;
    const float turnCross = steerBasis.z * targetDelta.x - steerBasis.x * targetDelta.z;
    const float forwardDot = steerBasis.x * targetDelta.x + steerBasis.z * targetDelta.z;
    int turnDirection;
    if (forwardDot <= 0.0f) {
        turnDirection = -1;
        if (turnCross >= 0.0f) {
            turnDirection = 1;
        }
        playerState->steeringInput = (float)turnDirection;
    } else {
        playerState->steeringInput = turnCross;
    }

    playerState->steeringInputCopy = playerState->steeringInput;
    playerState->throttleInput = 0.0f;
    playerState->throttleInputCopy = 0.0f;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.tick-ai-mode2-alt-gun-attack-window
 * @recoil-artifact defines .text recoil:function:0x402250: AINet::TickAiMode2AltGunAttackWindow.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: Battlesport/ai_net.h.
 * Purpose: reimplement AINet::TickAiMode2AltGunAttackWindow from the recovered
 * Battlesport ai_net.cpp source-file contribution.
 */
void __fastcall AINet::TickAiMode2AltGunAttackWindow(
    zUtil_SaveGameState* saveState,
    float targetDistance,
    float forwardDot
)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerGunFireController* const activeAltGunController = playerState->activeAltGunController;

    if (g_Player_TotalTimeSecScaled > playerState->aiStateEndTime) {
        const float startTime = g_Player_TotalTimeSecScaled + playerState->aiNotPursuitDwell;
        playerState->aiStateStartTime = startTime;
        playerState->aiStateEndTime = startTime + playerState->aiMode2AttackDwell;
    }

    if (playerState->altGunFireHeldFlag == 0) {
        if (g_Player_TotalTimeSecScaled <= activeAltGunController->nextDispatchTime
            || g_Player_TotalTimeSecScaled <= playerState->aiStateStartTime || playerState->damageProtectionActive != 0
            || forwardDot <= kPlayerAiAltGunAttackForwardMin
            || targetDistance >= activeAltGunController->aiAttackRangeMax
            || targetDistance <= activeAltGunController->aiAttackRangeMin
            || HasLineOfSightFromLocalPlayerFxOffset(playerState->rootNode, &playerState->fxOffsetWorld, 1) == 0
            || ((zUtil_SaveGameState*)g_GameStateOrMapTable)->playerState->lifecycleState == kPlayerLifecycleInactive) {
            return;
        }

        playerState->altGunDispatchRequested = 1;

        float statusScale;
        if (playerState->statusMeterScaled > 0.5f) {
            statusScale = playerState->statusMeterScaled;
        } else {
            statusScale = 0.5f;
        }

        activeAltGunController->nextDispatchTime
            = g_Player_TotalTimeSecScaled + activeAltGunController->dispatchRepeatDelay / statusScale;

        OptCatalogEntryDef* const optCatalogEntry = activeAltGunController->optCatalogEntry;
        const unsigned int flags = optCatalogEntry->flags;
        const bool hasTrail = (flags & kOptCatalogFlagCreateTrail) != 0;
        if (hasTrail) {
            playerState->altGunFireHeldFlag = 1;
            OptCatalog::ActivateTrailRuntimeState(
                activeAltGunController->trailRuntimeState,
                playerState->playerOrdinal
            );
            activeAltGunController->nextDispatchTime
                = g_Player_TotalTimeSecScaled + activeAltGunController->dispatchRepeatDelay;
            return;
        }

        if ((flags & kOptCatalogFlagLockOnTargetRef) != 0) {
            playerState->progressTargetCount = 1;
            playerState->progressTargetSlots[0].targetPos
                = &((zUtil_SaveGameState*)g_GameStateOrMapTable)->playerState->fxOffsetWorld;
            playerState->progressTargetSlots[0].targetVelocity
                = &((zUtil_SaveGameState*)g_GameStateOrMapTable)->playerState->projectileSpawnVel;
            HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x908), 5.0f);
            return;
        }

        playerState->progressTargetCount = 0;
        playerState->progressTargetSlots[0].targetPos = 0;
        playerState->progressTargetSlots[0].targetVelocity = 0;
        SolveAltGunLeadTargetPoint(
            saveState,
            (zUtil_SaveGameState*)g_GameStateOrMapTable,
            &playerState->storedTargetPos
        );
        return;
    }

    if (g_Player_TotalTimeSecScaled <= activeAltGunController->nextDispatchTime
        && forwardDot >= kPlayerAiAltGunAttackForwardMin && targetDistance <= activeAltGunController->aiAttackRangeMax
        && ((zUtil_SaveGameState*)g_GameStateOrMapTable)->playerState->lifecycleState != kPlayerLifecycleInactive) {
        playerState->storedTargetPos = ((zUtil_SaveGameState*)g_GameStateOrMapTable)->playerState->fxOffsetWorld;
        return;
    }

    playerState->altGunDispatchRequested = 0;
    activeAltGunController->nextDispatchTime
        = g_Player_TotalTimeSecScaled + activeAltGunController->dispatchRepeatDelay;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.solve-alt-gun-lead-target-point
 * @recoil-artifact defines .text recoil:function:0x4024a0: AINet::SolveAltGunLeadTargetPoint.
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.vector-subtract recoil:function:0x4024a0
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.solve-alt-gun-lead.vector-dot-xyz recoil:function:0x4024a0
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.solve-alt-gun-lead.vector-add recoil:function:0x4024a0
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.solve-alt-gun-lead.fast-sqrt-estimate recoil:function:0x4024a0
 * @recoil-match byte
 *
 * Original function evidence: retail 0x4024a0 contains two shared fixed-register
 * grouped-x87 subtraction islands, three full-XYZ dot islands, and one
 * vector-add island. Native C++ with unused input captures evaluates the
 * discriminant with the retail x87 order at [0x402624,0x402635).
 * The fast square-root bit transform remains the named-local four-instruction
 * island documented at its macro definition. Pointer binds, numerator/division,
 * fallback, rand tail, control flow, and the fastcall shell are compiler-generated.
 * Provisional source-placement hypothesis: Battlesport/ai_net.h.
 * Purpose: reimplement AINet::SolveAltGunLeadTargetPoint from the recovered
 * Battlesport ai_net.cpp source-file contribution.
 */
void __fastcall AINet::SolveAltGunLeadTargetPoint(
    zUtil_SaveGameState* saveState,
    zUtil_SaveGameState* targetSaveState,
    zVec3* outTargetPos
)
{
    zVec3* v0;
    zVec3* v1;
    zVec3* v2;
    zVec3 leadVectors[3];
    union {
        float inverseProjectileVelocity;
        float quadraticB;
    } leadCoefficient;
    float quadraticA;

    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    zUtil_PlayerStateStorage* const targetPlayerState = targetSaveState->playerState;
    const float projectileVelocity = playerState->activeAltGunController->optCatalogEntry->velocity;
    leadCoefficient.inverseProjectileVelocity = 1.0f / projectileVelocity;

    v0 = &leadVectors[2];
    v2 = &targetPlayerState->worldPos;
    v1 = &playerState->worldPos;
    AINET_VECTOR_SUBTRACT(v0, v2, v1);

    leadVectors[2].x = leadCoefficient.inverseProjectileVelocity * leadVectors[2].x;
    leadVectors[2].y = leadCoefficient.inverseProjectileVelocity * leadVectors[2].y;
    leadVectors[2].z = leadCoefficient.inverseProjectileVelocity * leadVectors[2].z;

    v2 = &leadVectors[0];
    v1 = &playerState->projectileSpawnVel;
    v0 = &targetPlayerState->projectileSpawnVel;
    AINET_VECTOR_SUBTRACT(v2, v0, v1);

    leadVectors[1].x = leadCoefficient.inverseProjectileVelocity * leadVectors[0].x;
    leadVectors[1].y = leadCoefficient.inverseProjectileVelocity * leadVectors[0].y;
    leadVectors[1].z = leadCoefficient.inverseProjectileVelocity * leadVectors[0].z;

    float leadScale;
    AINET_VECTOR_DOT_XYZ(quadraticA, leadVectors[1], leadVectors[1]);

    quadraticA = 1.0f - quadraticA;
    if (quadraticA <= 0.0f) {
        *outTargetPos = targetPlayerState->worldPos;
        return;
    }

    AINET_VECTOR_DOT_XYZ(leadCoefficient.quadraticB, leadVectors[1], leadVectors[2]);

    {
        float fastSqrtEstimate;
        float discriminant;
        {
            float dotProduct;
            float unscaledQuadraticA,
                unscaledDistanceSquared; // Unused captures preserve the retail x87 evaluation order.
            AINET_VECTOR_DOT_XYZ(dotProduct, leadVectors[2], leadVectors[2]);
            discriminant = (unscaledQuadraticA = quadraticA) * (unscaledDistanceSquared = dotProduct)
                + leadCoefficient.quadraticB * leadCoefficient.quadraticB;
        }
        AINET_FAST_SQRT_ESTIMATE(fastSqrtEstimate, discriminant);
        const float leadScaleNumerator = fastSqrtEstimate + leadCoefficient.quadraticB;

        leadScale = leadScaleNumerator / quadraticA;

        leadVectors[1].x = leadScale * leadVectors[0].x;
        leadVectors[1].y = leadScale * leadVectors[0].y;
        leadVectors[1].z = leadScale * leadVectors[0].z;

        AINET_VECTOR_ADD(outTargetPos, targetPlayerState->fxOffsetWorld, leadVectors[1]);
    }

    outTargetPos->y -= ((float)(rand()) * 3.05185094e-05f - 0.5f) * -2.0f;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.update-ai-mode2-move-and-turn-toward-offset-target
 * @recoil-artifact defines .text recoil:function:0x4026d0: Primary authored AINet steering body.
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.vector-subtract recoil:function:0x4026d0
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.solve-alt-gun-lead.vector-add recoil:function:0x4026d0
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.path-dot-xz recoil:function:0x4026d0
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.path-cross-xz recoil:function:0x4026d0
 * @recoil-match source
 *
 * Pro review 2026-09-08T14-41-30-384Z: native component, grouped, and pointer
 * variants and mixed native dot/cross failed; use the five exact shared islands.
 * Canonical VC5 confirms all five kernel intervals. Pointer setup,
 * rotation, scaling, control flow, and conversions remain compiler-owned.
 * Purpose: Rotates the target-to-AI vector by accepted tuning globals and steers to the offset point. Source model:
 * AINet source-file contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::UpdateAiMode2MoveAndTurnTowardOffsetTarget(
    zUtil_SaveGameState* saveState,
    zUtil_SaveGameState* targetState
)
{
    zUtil_PlayerStateStorage* savedPlayerState; // Unused snapshot preserves retail VC5 x87 evaluation order.
    zUtil_PlayerStateStorage* const playerState = (savedPlayerState = saveState->playerState);
    zUtil_PlayerStateStorage* savedTargetPlayerState; // Unused snapshot retained for retail VC5 scheduling.
    zUtil_PlayerStateStorage* const targetPlayerState = (savedTargetPlayerState = targetState->playerState);
    zVec3 targetDir;
    zVec3 targetToPlayerDir;
    zVec3 offsetTarget;
    const float offsetDistance = playerState->aiNet->pursuitParam0;
    {
        zVec3* v0 = &targetToPlayerDir;
        zVec3* v1 = &targetPlayerState->worldPos;
        zVec3* v2 = &playerState->worldPos;
        AINET_VECTOR_SUBTRACT(v0, v2, v1);
    }
    targetToPlayerDir.y = 0.0f;
    zMath::Vec3Normalize(&targetToPlayerDir);

    targetDir.y = 0.0f;
    targetDir.x = g_Player_AiMode2_OffsetTargetRotateCos15Deg * targetToPlayerDir.x
        - g_Player_AiMode2_OffsetTargetRotateSin15Deg * targetToPlayerDir.z;
    targetDir.z = g_Player_AiMode2_OffsetTargetRotateCos15Deg * targetToPlayerDir.z
        + g_Player_AiMode2_OffsetTargetRotateSin15Deg * targetToPlayerDir.x;
    targetDir.x = offsetDistance * targetDir.x;
    targetDir.z = offsetDistance * targetDir.z;

    {
        zVec3* destination = &offsetTarget;
        AINET_VECTOR_ADD(destination, targetPlayerState->worldPos, targetDir);
    }
    {
        zVec3* v0 = &targetDir;
        zVec3* v1 = &playerState->worldPos;
        zVec3* v2 = &offsetTarget;
        AINET_VECTOR_SUBTRACT(v0, v2, v1);
    }
    targetDir.y = 0.0f;
    zMath::Vec3Normalize(&targetDir);

    float forwardDot;
    AINET_PATH_DOT_XZ(forwardDot, playerState->steerBasisNorm, targetDir);
    float turnCross;
    {
        zVec3* v2;
        AINET_PATH_CROSS_XZ(turnCross, playerState->steerBasisNorm, targetDir);
    }
    if (forwardDot < 0.0f) {
        playerState->throttleInput = 0.0f;
        playerState->steeringInput = turnCross < 0.0f ? -1 : 1;
    } else {
        float throttle = 1.0f - (float)(fabs(turnCross));
        if (throttle <= kPlayerAiPathFollowMinThrottle) {
            throttle = kPlayerAiPathFollowMinThrottle;
        }
        playerState->throttleInput = throttle;
        playerState->steeringInput = turnCross;
    }
    playerState->throttleInputCopy = playerState->throttleInput;
    playerState->steeringInputCopy = playerState->steeringInput;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.update-ai-mode2-move-and-turn-toward-dynamic-offset-target
 * @recoil-artifact defines .text recoil:function:0x4028c0: Primary authored AINet steering body.
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.vector-subtract recoil:function:0x4028c0
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.solve-alt-gun-lead.vector-add recoil:function:0x4028c0
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.path-dot-xz recoil:function:0x4028c0
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.path-cross-xz recoil:function:0x4028c0
 * @recoil-match byte
 *
 * Pro review 2026-09-08T14-41-30-384Z: native component, grouped, pointer,
 * and mixed native dot/cross variants failed. Canonical VC5 confirms the five
 * exact shared kernel intervals; scalar math and control remain C++.
 * Purpose: Blends dynamic pursuit and side-offset steering based on distance to the local player. Source model: AINet
 * source-file contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::UpdateAiMode2MoveAndTurnTowardDynamicOffsetTarget(
    zUtil_SaveGameState* saveState,
    zUtil_SaveGameState* targetState,
    float targetDistance
)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    zUtil_PlayerStateStorage* const targetPlayerState = targetState->playerState;
    zVec3 steerBasis;
    zVec3 targetDir;
    zVec3 targetPoint;
    int reverseSideOffset;
    {
        AINet* const aiNet = playerState->aiNet;
        const float pursuitDistance = aiNet->pursuitParam0;
        const float sideOffsetScale = aiNet->pursuitParam1;
        const float doublePursuitDistance = pursuitDistance + pursuitDistance;
        steerBasis = playerState->aiDynamicOffsetDir;
        targetPoint.x = pursuitDistance * steerBasis.x;
        targetPoint.y = pursuitDistance * steerBasis.y;
        targetPoint.z = pursuitDistance * steerBasis.z;
        {
            zVec3* destination = &targetPoint;
            AINET_VECTOR_ADD(destination, targetPlayerState->worldPos, targetPoint);
        }

        const float negativeOffsetX = -steerBasis.x;
        float blend = (doublePursuitDistance - targetDistance) / pursuitDistance;
        if (blend > 1.0f) {
            blend = 1.0f;
        } else if (blend < 0.0f) {
            blend = 0.0f;
        }
        // Preserve the observed extended scale calculation after the float clamp.
        // The original local spelling and conversion syntax remain unresolved.
        const double offsetBlend = blend;
        if (playerState->localVel.z > 0.0f && targetDistance < doublePursuitDistance) {
            targetDir.x = steerBasis.z * (-(offsetBlend * sideOffsetScale));
            targetDir.y = targetPoint.y * (-(offsetBlend * sideOffsetScale));
            targetDir.z = negativeOffsetX * (-(offsetBlend * sideOffsetScale));
            reverseSideOffset = 1;
        } else {
            targetDir.x = steerBasis.z * (offsetBlend * sideOffsetScale);
            targetDir.y = targetPoint.y * (offsetBlend * sideOffsetScale);
            targetDir.z = negativeOffsetX * (offsetBlend * sideOffsetScale);
            reverseSideOffset = 0;
        }
    }
    {
        zVec3* destination = &targetPoint;
        AINET_VECTOR_ADD(destination, targetPoint, targetDir);
    }

    {
        zVec3* v0;
        zVec3* v1;
        zVec3* v2;
        v0 = &targetDir;
        v1 = &playerState->worldPos;
        v2 = &targetPoint;
        AINET_VECTOR_SUBTRACT(v0, v2, v1);
    }
    targetDir.y = 0.0f;
    const float targetDirDistance = zMath::Vec3Normalize(&targetDir);

    steerBasis = playerState->steerBasisNorm;
    if (reverseSideOffset != 0) {
        steerBasis.x = -steerBasis.x;
        steerBasis.z = -steerBasis.z;
    }
    float forwardDot;
    AINET_PATH_DOT_XZ(forwardDot, steerBasis, targetDir);
    float turnCross;
    {
        zVec3* v2;
        AINET_PATH_CROSS_XZ(turnCross, steerBasis, targetDir);
    }
    if (forwardDot < 0.0f && targetDirDistance < 10.0f) {
        playerState->throttleInput = -1.0f;
        playerState->steeringInput = 0.0f;
    } else {
        float throttle = 1.0f - (float)(fabs(turnCross));
        if (throttle <= kPlayerAiPathFollowMinThrottle) {
            throttle = kPlayerAiPathFollowMinThrottle;
        }
        playerState->throttleInput = throttle;
        playerState->steeringInput = turnCross;
    }
    if (reverseSideOffset != 0) {
        playerState->throttleInput = -playerState->throttleInput;
        playerState->throttleInputCopy = playerState->throttleInput;
    } else {
        playerState->throttleInputCopy = playerState->throttleInput;
    }
    playerState->steeringInputCopy = playerState->steeringInput;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.tick-ai-mode2-timed-path-steering
 * @recoil-artifact defines .text recoil:function:0x402b70: AINet::TickAiMode2TimedPathSteering.
 * @recoil-match byte
 *
 * Purpose: Alternates timed forward and reverse path-node steering around the AI home path node. Source model: AINet
 * source-file contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::TickAiMode2TimedPathSteering(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;

    if (g_Player_TotalTimeSecScaled > playerState->unknown_0fa4) {
        AINetNode* const currentPathNode = playerState->aiCurrentPathNode;
        AINetNode* const pathAnchorNode = playerState->aiHomePathNode;

        if (currentPathNode == pathAnchorNode) {
            AiSteerTowardPathNodeForward(saveState);
        } else if (currentPathNode->neighborNodes[0] == pathAnchorNode && currentPathNode->nodeIndex != -1) {
            AiSteerTowardPathNodeReverse(saveState);
        } else {
            TickAiMode2PathFollow(saveState);
        }
    }

    playerState->recentHitFlag = 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.steer-toward-path-node-forward
 * @recoil-artifact defines .text recoil:function:0x402be0: Primary authored AINet steering body.
 * @recoil-artifact emits .rdata recoil:data:0x4cc848: VC5 negative operand for the four-second delay addition.
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.vector-subtract recoil:function:0x402be0
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.path-dot-xz recoil:function:0x402be0
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.path-cross-xz recoil:function:0x402be0
 * @recoil-match byte
 *
 * Original function evidence: retail 0x402be0 contains the shared subtraction and the
 * byte-sensitive XZ dot/cross expansions used by forward-node steering.
 * Provisional source-placement hypothesis: Battlesport/ai_net.h.
 * The auxiliary scalar ranges have exact retail readers and widths; original
 * identifiers, literal-versus-named provenance, and containing source extent
 * remain unresolved. These relationships do not accept the owner data gate.
 * Purpose: reimplement AINet::AiSteerTowardPathNodeForward from the recovered
 * Battlesport ai_net.cpp source-file contribution.
 */
void __fastcall AINet::AiSteerTowardPathNodeForward(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
#if !(defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100)
    zVec3* v0;
    zVec3* v1;
#endif
    zVec3* v2;

    zVec3 targetDir;
    zVec3 forwardNodePosition = playerState->aiCurrentPathNode->neighborNodes[0]->position;
    AINET_PATH_COMPUTE_FORWARD_NODE_DIR(targetDir, forwardNodePosition, playerState->worldPos);
    targetDir.y = 0.0f;
    const float targetDistance = zMath::Vec3Normalize(&targetDir);

    if (targetDistance < kPlayerAiForwardPathAdvanceDistance) {
        playerState->aiCurrentPathNode = playerState->aiCurrentPathNode->neighborNodes[0];
        playerState->throttleInputCopy = 0.0f;
        playerState->throttleInput = 0.0f;
        playerState->steeringInputCopy = 0.0f;
        playerState->steeringInput = 0.0f;
        playerState->unknown_0fa4 = g_Player_TotalTimeSecScaled + 4.0f;
        return;
    }

    float forwardDot;
    AINET_PATH_DOT_XZ(forwardDot, playerState->steerBasisNorm, targetDir);
    float turnCross;
    AINET_PATH_CROSS_XZ(turnCross, playerState->steerBasisNorm, targetDir);

    if (forwardDot < 0.0f) {
        const float turnCrossForSign = turnCross;
        AINET_TURN_DIRECTION_SLOT(turnCross) = -1;
        playerState->throttleInputCopy = 0.0f;
        playerState->throttleInput = 0.0f;
        if (turnCrossForSign >= 0.0f) {
            AINET_TURN_DIRECTION_SLOT(turnCross) = 1;
        }
        playerState->steeringInputCopy = (float)AINET_TURN_DIRECTION_SLOT(turnCross);
        playerState->steeringInput = playerState->steeringInputCopy;
        return;
    }

    float throttle = 1.0f - (float)(fabs(turnCross));
    if (throttle <= kPlayerAiPathFollowMinThrottle) {
        throttle = kPlayerAiPathFollowMinThrottle;
    }
    playerState->throttleInputCopy = throttle;
    playerState->throttleInput = throttle;
    playerState->steeringInputCopy = turnCross;
    playerState->steeringInput = turnCross;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.steer-toward-path-node-reverse
 * @recoil-artifact defines .text recoil:function:0x402d60: Primary authored AINet steering body.
 * @recoil-artifact emits .rdata recoil:data:0x4cc84c: VC5 negative operand for the fourteen-second delay addition.
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.vector-subtract recoil:function:0x402d60
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.path-dot-xz recoil:function:0x402d60
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.path-cross-xz recoil:function:0x402d60
 * @recoil-match byte
 *
 * Original function evidence: retail 0x402d60 contains the shared subtraction and the
 * byte-sensitive XZ dot/cross expansions used by reverse-node steering.
 * Provisional source-placement hypothesis: Battlesport/ai_net.h.
 * The delay operand has one exact four-byte retail reader. Its original
 * identifier, literal-versus-named provenance, and containing source extent
 * remain unresolved; this relationship does not accept the owner data gate.
 * Purpose: reimplement AINet::AiSteerTowardPathNodeReverse from the recovered
 * Battlesport ai_net.cpp source-file contribution.
 */
void __fastcall AINet::AiSteerTowardPathNodeReverse(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
#if !(defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100)
    zVec3* v0;
    zVec3* v1;
#endif
    zVec3* v2;

    zVec3 targetDir;
    zVec3 forwardNodePosition = playerState->aiCurrentPathNode->neighborNodes[0]->position;
    AINET_PATH_COMPUTE_FORWARD_NODE_DIR(targetDir, forwardNodePosition, playerState->worldPos);
    targetDir.y = 0.0f;
    const float targetDistance = zMath::Vec3Normalize(&targetDir);

    if (targetDistance < kPlayerAiForwardPathAdvanceDistance) {
        playerState->aiCurrentPathNode = playerState->aiCurrentPathNode->neighborNodes[0];
        playerState->throttleInputCopy = 0.0f;
        playerState->throttleInput = 0.0f;
        playerState->steeringInputCopy = 0.0f;
        playerState->steeringInput = 0.0f;
        playerState->unknown_0fa4 = g_Player_TotalTimeSecScaled + 14.0f;
        return;
    }

    zVec3 reverseSteerBasis = playerState->steerBasisNorm;
    reverseSteerBasis.x = -reverseSteerBasis.x;
    reverseSteerBasis.z = -reverseSteerBasis.z;
    float forwardDot;
    AINET_PATH_DOT_XZ(forwardDot, reverseSteerBasis, targetDir);
    float turnCross;
    AINET_PATH_CROSS_XZ(turnCross, reverseSteerBasis, targetDir);

    if (forwardDot < 0.0f) {
        const float turnCrossForSign = turnCross;
        AINET_TURN_DIRECTION_SLOT(turnCross) = -1;
        playerState->throttleInputCopy = 0.0f;
        playerState->throttleInput = 0.0f;
        if (turnCrossForSign >= 0.0f) {
            AINET_TURN_DIRECTION_SLOT(turnCross) = 1;
        }
        playerState->steeringInputCopy = (float)AINET_TURN_DIRECTION_SLOT(turnCross);
        playerState->steeringInput = playerState->steeringInputCopy;
        return;
    }

    float throttle = 1.0f - (float)(fabs(turnCross));
    if (throttle <= kPlayerAiPathFollowMinThrottle) {
        throttle = kPlayerAiPathFollowMinThrottle;
    }
    throttle = -throttle;
    playerState->throttleInputCopy = throttle;
    playerState->throttleInput = throttle;
    playerState->steeringInputCopy = turnCross;
    playerState->steeringInput = turnCross;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ai-finalize-mode2-state1-for-all-players
 * @recoil-artifact defines .text recoil:function:0x402f10: AINet::AiFinalizeMode2State1ForAllPlayers.
 * @recoil-match byte
 *
 * BN shows traversal from g_PlayerSaveStateList.head, filtering
 * lifecycleState == 2 and aiTopLevelState == 1, restoring matching nodes, and
 * setting g_Player_AiMode2State1Finalized to 1 after the pass.
 * Purpose: Finalizes AI Mode2 State1 by restoring saved top-level state for
 * active AI players and setting the global finalization latch.
 */
void AINet::AiFinalizeMode2State1ForAllPlayers()
{
    zUtil_SaveGameState* saveState = g_PlayerSaveStateList.head;
    while (saveState != 0) {
        zUtil_PlayerStateStorage* const playerState = saveState->playerState;
        if (playerState->lifecycleState == 2 && playerState->aiTopLevelState == 1) {
            AiRestoreSavedTopLevelState(saveState);
        }

        saveState = saveState != 0 ? saveState->next : 0;
    }

    g_Player_AiMode2State1Finalized = 1;
}

#include "GameZRecoil/zMath/zmth.h"
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ainet-load-all-from-zrd
 * @recoil-artifact defines .text recoil:function:0x402fd0: AINet::LoadAllFromZrd.
 * @recoil-match byte
 *
 * Purpose: Loads every numbered AI path network definition from the mission
 * ZRD set. Its retail source relationship remains unresolved.
 */
void AINet::LoadAllFromZrd()
{
    for (int netId = 1; netId < 100; ++netId) {
        AINet::LoadFromZrd(netId);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ainet-alloc
 * @recoil-artifact defines .text recoil:function:0x402ff0: AINet::Alloc (Battlesport/ai_net.cpp).
 * @recoil-match byte
 *
 * Purpose: Allocates a zeroed AI network record and appends it to the global AI network list.
 */
AINet* AINet::Alloc()
{
    AINet* const aiNet = (AINet*)(malloc(sizeof(AINet)));
    memset(aiNet, 0, sizeof(AINet));

    if (g_AINetListHead != 0) {
        g_AINetListTail->next = aiNet;
        g_AINetListTail = aiNet;
        return aiNet;
    }

    g_AINetListHead = aiNet;
    g_AINetListTail = aiNet;
    return aiNet;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ainet-loadfromzrd
 * @recoil-artifact defines .text recoil:function:0x403040: AINet::LoadFromZrd (Battlesport/ai_net.cpp).
 * @recoil-match byte
 *
 * Purpose: Parses one AI path network ZRD, builds its node list, resolves links, and returns the loaded network.
 */
AINet* __fastcall AINet::LoadFromZrd(int netId)
{
    char baseName[8];
    char nodeName[8];
    sprintf(baseName, "net_%02d", netId);

    char path[0x104];
    sprintf(path, "%s.zrd", baseName);

    zReader::Node* const root = zReader::Load(path, 0, 0);
    if (root == 0) {
        return 0;
    }

    zReader::Node* versionNode = zRdrFindTag(root, "version");
    if (versionNode != 0 && versionNode->value.nodes[1].value.i32 != 105) {
        zError::ReportOld(0x200, "D:\\Proj\\Battlesport\\ai_net.cpp", 0x8c, "Wrong ai_paths.zrd version number!");
        return 0;
    }

    AINet* const aiNet = AINet::Alloc();
    aiNet->netId = netId;

    zReader::Node* nameNode = zRdrFindTag(root, "name");
    if (nameNode != 0) {
        strcpy(aiNet->name, nameNode->value.nodes[1].value.str);
    } else {
        strcpy(aiNet->name, baseName);
    }

    char token[0x18];
    zReader::Node* typeNode = zRdrFindTag(root, "type");
    if (typeNode != 0) {
        strcpy(token, typeNode->value.nodes[1].value.str);
        _strupr(token);

        if (strncmp(token, "ST", 2) == 0) {
            aiNet->aiType = AINET_TYPE_ST;
        } else if (strncmp(token, "HI", 2) == 0) {
            aiNet->aiType = AINET_TYPE_HI;
        } else if (strncmp(token, "FI", 2) == 0) {
            aiNet->aiType = AINET_TYPE_FI;
        } else if (strncmp(token, "DE", 2) == 0) {
            aiNet->aiType = AINET_TYPE_DE;
        }
    } else {
        aiNet->aiType = AINET_TYPE_ST;
    }

    zReader::Node* pathWidthNode = zRdrFindTag(root, "path_width");
    if (pathWidthNode != 0) {
        aiNet->pathWidth = pathWidthNode->value.nodes[1].value.f32;
    } else {
        aiNet->pathWidth = 10.0f;
    }

    zReader::Node* activateRadiusNode = zRdrFindTag(root, "activate_rad");
    if (activateRadiusNode != 0) {
        aiNet->activateRadius = activateRadiusNode->value.nodes[1].value.f32;
    }

    zReader::Node* attackRadiusNode = zRdrFindTag(root, "attack_rad");
    if (attackRadiusNode != 0) {
        aiNet->attackRadius = attackRadiusNode->value.nodes[1].value.f32;
    }

    zReader::Node* attackDwellNode = zRdrFindTag(root, "attack_dwell");
    if (attackDwellNode != 0) {
        aiNet->attackDwell = attackDwellNode->value.nodes[1].value.f32;
    }

    zReader::Node* pursuitNode = zRdrFindTag(root, "pursuit_params");
    if (pursuitNode == 0) {
        pursuitNode = zRdrFindTag(root, "pursuit_range");
    }
    if (pursuitNode != 0) {
        aiNet->pursuitParam0 = pursuitNode->value.nodes[1].value.f32;
        aiNet->pursuitParam1 = pursuitNode->value.nodes[2].value.f32;
    }

    zReader::Node* notPursuitDwellNode = zRdrFindTag(root, "not_pursuit_dwell");
    if (notPursuitDwellNode != 0) {
        aiNet->notPursuitDwell = notPursuitDwellNode->value.nodes[1].value.f32;
    }

    zReader::Node* returnRangeNode = zRdrFindTag(root, "return_range");
    if (returnRangeNode != 0) {
        aiNet->returnRange = returnRangeNode->value.nodes[1].value.f32;
    }

    zReader::Node* hideTimesNode = zRdrFindTag(root, "hide_times");
    if (hideTimesNode != 0) {
        aiNet->hideTime0 = hideTimesNode->value.nodes[1].value.f32;
        aiNet->hideTime1 = hideTimesNode->value.nodes[2].value.f32;
    } else {
        aiNet->hideTime0 = 8.0f;
        aiNet->hideTime1 = 4.0f;
    }

    zReader::Node* attackBuddyNode = zRdrFindTag(root, "attack_buddy");
    if (attackBuddyNode != 0) {
        aiNet->attackBuddyNetId = attackBuddyNode->value.nodes[1].value.i32;
    } else {
        aiNet->attackBuddyNetId = 0;
    }

    zReader::Node* activateBuddyNode = zRdrFindTag(root, "activate_buddy");
    if (activateBuddyNode != 0) {
        aiNet->activateBuddyNetId = activateBuddyNode->value.nodes[1].value.i32;
    } else {
        aiNet->attackBuddyNetId = 0;
    }

    zReader::Node* attackStrategyNode = zRdrFindTag(root, "attack_strategy");
    if (attackStrategyNode != 0) {
        strcpy(token, attackStrategyNode->value.nodes[1].value.str);
        _strupr(token);
        if (strncmp(token, "FOL", 3) == 0) {
            aiNet->attackStrategy = AINET_STRAT_FOL;
        } else if (strncmp(token, "CIR", 3) == 0) {
            aiNet->attackStrategy = AINET_STRAT_CIR;
        } else if (strncmp(token, "HEA", 3) == 0) {
            aiNet->attackStrategy = AINET_STRAT_HEA;
        } else if (strncmp(token, "BAC", 3) == 0) {
            aiNet->attackStrategy = AINET_STRAT_BAC;
        } else if (strncmp(token, "ZIG", 3) == 0) {
            aiNet->attackStrategy = AINET_STRAT_ZIG;
        } else if (strncmp(token, "SIT", 3) == 0) {
            aiNet->attackStrategy = AINET_STRAT_SIT;
        }
    } else {
        aiNet->attackStrategy = AINET_STRAT_HEA;
    }

    AINetNode* tail = 0;
    for (int nodeIndex = 0; nodeIndex < 99; ++nodeIndex) {
        sprintf(nodeName, "node_%02d", nodeIndex);

        zReader::Node* node = zRdrFindTag(root, nodeName);
        if (node == 0) {
            continue;
        }

        AINetNode* const aiNode = (AINetNode*)(malloc(sizeof(AINetNode)));
        memset(aiNode, 0, sizeof(AINetNode));

        if (tail == 0) {
            aiNet->nodeListHead = aiNode;
        } else {
            tail->next = aiNode;
        }
        tail = aiNode;

        aiNode->nodeIndex = nodeIndex;
        aiNode->costOrType = node->value.nodes[1].value.i32;
        aiNode->position.x = node->value.nodes[2].value.nodes[1].value.f32;
        aiNode->position.y = node->value.nodes[2].value.nodes[2].value.f32;
        aiNode->position.z = node->value.nodes[2].value.nodes[3].value.f32;
        aiNode->neighborIndices[0] = node->value.nodes[3].value.nodes[1].value.i32;
        aiNode->neighborIndices[1] = node->value.nodes[3].value.nodes[2].value.i32;
        aiNode->neighborIndices[2] = node->value.nodes[3].value.nodes[3].value.i32;
    }

    AINet::ResolveNeighborLinksAndBuildProbeFans(aiNet->nodeListHead, aiNet->pathWidth);
    zReader::Free(root);
    return aiNet;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ainet-findbynetid
 * @recoil-artifact defines .text recoil:function:0x403510: AINet::FindByNetId (Battlesport/ai_net.cpp).
 * @recoil-match byte
 *
 * Purpose: Finds the first loaded AI network with the requested network id.
 */
AINet* __fastcall AINet::FindByNetId(int netId)
{
    AINet* aiNet = g_AINetListHead;
    while (aiNet != 0) {
        if (aiNet->netId == netId) {
            return aiNet;
        }
        aiNet = aiNet->next;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ainet-findnodebyindex
 * @recoil-artifact defines .text recoil:function:0x403530: AINet::FindNodeByIndex (Battlesport/ai_net.cpp).
 * @recoil-match byte
 *
 * Purpose: Finds the first AI path node with the requested parsed node index.
 */
AINetNode* __fastcall AINet::FindNodeByIndex(int nodeIndex, AINetNode* nodeListHead)
{
    AINetNode* node = nodeListHead;
    while (node != 0) {
        if (node->nodeIndex == nodeIndex) {
            return node;
        }
        node = node->next;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ainet-resolveneighborlinksandbuildprobefans
 * @recoil-artifact defines .text recoil:function:0x403550: AINet::ResolveNeighborLinksAndBuildProbeFans (Battlesport/ai_net.cpp).
 * @recoil-match byte
 *
 * Purpose: Resolves neighbor indices into node pointers and allocates probe fans for valid AI path links.
 */
void __fastcall AINet::ResolveNeighborLinksAndBuildProbeFans(AINetNode* nodeListHead, float pathWidth)
{
    AINetNode* node = nodeListHead;
    while (node != 0) {
        const zVec3 fromPosition = node->position;
        int* neighborIndexPtr = node->neighborIndices;
        for (int slotCount = 3; slotCount != 0; --slotCount) {
            const int neighborIndex = *neighborIndexPtr;
            if (neighborIndex >= 0) {
                *(AINetNode**)neighborIndexPtr = AINet::FindNodeByIndex(neighborIndex, nodeListHead);

                *(AINetPathProbeFan**)(neighborIndexPtr + 3) = (AINetPathProbeFan*)(malloc(sizeof(AINetPathProbeFan)));
                memset(*(AINetPathProbeFan**)(neighborIndexPtr + 3), 0, sizeof(AINetPathProbeFan));
                (*(AINetPathProbeFan**)(neighborIndexPtr + 3))
                    ->InitFromSegment(fromPosition, (*(AINetNode**)neighborIndexPtr)->position, pathWidth);
            } else {
                *(AINetNode**)neighborIndexPtr = 0;
                *(AINetPathProbeFan**)(neighborIndexPtr + 3) = 0;
            }
            ++neighborIndexPtr;
        }

        node = node->next;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ainet-path-probe-fan-init-from-segment
 * @recoil-artifact defines .text recoil:function:0x403620: AINetPathProbeFan::InitFromSegment.
 * @recoil-raw-asm recoil:raw-asm:battlesport.ai-net.ainet-path-probe-fan.init-from-segment.path-width-store
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.ainet-path-probe-fan.init-from-segment.path-width-store recoil:function:0x403620
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.vector-subtract recoil:function:0x403620
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.path-probe-clamp-travel-vc5 recoil:function:0x403620
 * @recoil-match byte
 *
 * Original function evidence: retail 0x403620 contains these three approved local and
 * header-expanded raw-assembly regions.
 * Raw-assembly evidence: VC5 C++ variants did not preserve the retail
 * fixed-register path-width store, shared vector subtraction, or x87 clamp
 * ordering; each approved island remains scoped to this exact consumer.
 * Purpose: Builds the normalized path-probe fan basis and travel clamp for one AI navigation segment.
 */
void AINetPathProbeFan::InitFromSegment(zVec3 fromPosition, zVec3 toPosition, float pathWidth)
{
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    zVec3* v2;
#else
    zVec3* v0;
    zVec3* v1;
#endif

#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    AINET_PROBE_FAN_COMPUTE_SEGMENT_DELTA_KEEP_PTR(delta, toPosition, fromPosition, v2);
#else
    AINET_PROBE_FAN_COMPUTE_SEGMENT_DELTA(delta, toPosition, fromPosition);
#endif

#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    float xzLength;
    AINET_PATH_PROBE_CLAMP_TRAVEL_VC5(v2, xzLength, pathWidth);
#else
    float xzLength;
    xzLength = sqrt(delta.x * delta.x + delta.z * delta.z);
    clampedTravel = (xzLength - pathWidth > pathWidth * g_AINetPathProbeHalfWidthScale)
        ? ((xzLength = sqrt(delta.x * delta.x + delta.z * delta.z)) - pathWidth)
        : (pathWidth * g_AINetPathProbeHalfWidthScale);
#endif

    zMath::Vec3NormalizeXZ(&delta, &delta);
    zVec3* const perpendicularPtr = &perpendicular;
    zMath::Vec3ToRightXZ(&delta, perpendicularPtr);
    zMath::Vec3RotateY(45.0f, &probeDirPlus45, perpendicularPtr);
    zMath::Vec3RotateY(-45.0f, &probeDirMinus45, perpendicularPtr);
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    /**
     * AINetPathProbeFan::InitFromSegment path-width store.
     * Raw assembly: preserves the VC5 register/store shape for the final
     * pathWidth assignment after C++ assignment variants kept rotating the
     * remaining byte diff in this function.
     * Purpose: Stores the probe fan path width in the retail register order.
     */
    __asm mov edx, pathWidth __asm mov dword ptr[esi + 34h], edx
#else
    this->pathWidth = pathWidth;
#endif
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ainet-findnearestnode
 * @recoil-artifact defines .text recoil:function:0x4036f0: AINet::FindNearestNode (Battlesport/ai_net.cpp).
 * @recoil-match byte
 *
 * Purpose: Scans an AI node list and returns the node nearest to the query position.
 */
AINetNode* __fastcall AINet::FindNearestNode(const zVec3* position, AINetNode* nodeListHead)
{
    /**
     * @recoil-anchor recoil:anchor:battlesport.ai-net.nearest-node-minimum-distance-sq
     * @recoil-artifact defines .rdata recoil:data:0x4cc858: Nearest-node zero comparison scalar.
     * Purpose: Supplies the separate read-only zero boundary used to recognize
     * the negative distance sentinel. Retail 0x40371e reads the four-byte
     * positive-zero range at 0x4cc858, distinct from the header-body zero.
     * This auxiliary constant represents that storage dependency; the original
     * identifier, lexical scope, and containing-object extent remain unresolved.
     */
    static const float minimumDistanceSq = 0.0f;
    AINetNode* nearest = 0;
    float bestDistanceSq = -1.0f;

    while (nodeListHead != 0) {
        const float distanceSq = zMath::Vec3DeltaLengthSq(position, &nodeListHead->position);
        if (distanceSq < bestDistanceSq || bestDistanceSq < minimumDistanceSq) {
            bestDistanceSq = distanceSq;
            nearest = nodeListHead;
        }
        nodeListHead = nodeListHead->next;
    }

    return nearest;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ainet-buildaipeerringsbyainetid
 * @recoil-artifact defines .text recoil:function:0x403750: AINet::BuildAiPeerRingsByAiNetId.
 * @recoil-match byte
 *
 * Source placement audit: BN file-literal order makes accepted player.cpp
 * ownership invalid; this body remains here only until the AINet remap can
 * pull the required save-state declarations with it.
 * Purpose: link active save states sharing one AI network id into peer rings
 * used by AI bootstrap while leaving inactive/unlinked records untouched.
 * Source owner: pending battlesport_ai.ainet_peer_ring_build audit.
 */
void AINet::BuildAiPeerRingsByAiNetId()
{
    zUtil_SaveGameState* saveState = g_PlayerSaveStateList.head;
    while (saveState != 0) {
        zUtil_PlayerStateStorage* const playerState = saveState->playerState;
        const int aiNetId = playerState->aiNetId;
        zUtil_SaveGameState* candidate = saveState != 0 ? saveState->next : 0;
        while (candidate != 0) {
            zUtil_PlayerStateStorage* const candidatePlayerState = candidate->playerState;
            if (candidatePlayerState->aiNetId == aiNetId
                && candidatePlayerState->lifecycleState != kPlayerLifecycleInactive
                && candidate->aiPeerRingNext == candidate) {
                candidate->aiPeerRingNext = saveState->aiPeerRingNext;
                saveState->aiPeerRingNext = candidate;
            }
            candidate = candidate != 0 ? candidate->next : 0;
        }
        saveState = saveState != 0 ? saveState->next : 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ainetnode-free
 * @recoil-artifact defines .text recoil:function:0x4037c0: AINetNode::Free (Battlesport/ai_net.cpp).
 * @recoil-match byte
 *
 * Purpose: Frees a path node and any probe-fan records allocated for its neighbor links.
 */
void AINetNode::Free()
{
    if (this == 0) {
        return;
    }

    {
        for (int index = 0; index < 3; ++index) {
            AINetPathProbeFan* probeFan = probeFans[index];
            if (probeFan != 0) {
                free(probeFan);
            }
        }
    }

    free(this);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ainet-free
 * @recoil-artifact defines .text recoil:function:0x403800: AINet::Free (Battlesport/ai_net.cpp).
 * @recoil-match byte
 *
 * Purpose: Frees every node owned by this AI network and then releases the network record.
 */
void AINet::Free()
{
    if (this == 0) {
        return;
    }

    AINetNode* node = nodeListHead;
    while (node != 0) {
        AINetNode* const current = node;
        node = node->next;
        current->Free();
    }

    free(this);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ainet-aidiscardnegativebranchpathnodes
 * @recoil-artifact defines .text recoil:function:0x403830: AINet::AiDiscardNegativeBranchPathNodes
 * @recoil-match byte
 *
 * Purpose: discard temporary negative-index AI path nodes before the saved
 * player state releases or resumes its current path cursor.
 */
void __fastcall AINet::AiDiscardNegativeBranchPathNodes(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* playerState = saveState->playerState;
    AINetNode* aiCurrentPathNode = playerState->aiCurrentPathNode;
    if (aiCurrentPathNode == 0 || aiCurrentPathNode->nodeIndex >= 0) {
        return;
    }

    do {
        playerState->aiCurrentPathNode = aiCurrentPathNode->neighborNodes[0];
        aiCurrentPathNode->Free();
        aiCurrentPathNode = playerState->aiCurrentPathNode;
    } while (aiCurrentPathNode->nodeIndex < 0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ainet-freeall
 * @recoil-artifact defines .text recoil:function:0x403870: AINet::FreeAll (Battlesport/ai_net.cpp).
 * @recoil-match byte
 *
 * Purpose: Walks the global AI network list and frees every loaded network.
 */
void AINet::FreeAll()
{
    AINet* aiNet = g_AINetListHead;
    while (aiNet != 0) {
        g_AINetListHead = aiNet->next;
        aiNet->Free();
        aiNet = g_AINetListHead;
    }
}
