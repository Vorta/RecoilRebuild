#ifndef BATTLESPORT_AI_NET_H
#define BATTLESPORT_AI_NET_H


#include "recoil/recoil_types.h"
#include <stddef.h>

#include "GameZRecoil/include/zclass.h"
#include "recoil/recoil_callconv.h"

struct zClass_NodePartial;
struct zUtil_SaveGameState;

struct AINetPathProbeFan {
    zVec3 delta;
    float clampedTravel;
    zVec3 perpendicular;
    zVec3 probeDirPlus45;
    zVec3 probeDirMinus45;
    float pathWidth;
    unsigned char unknown_38[0x04];

    void InitFromSegment(
        zVec3 fromPosition,
        zVec3 toPosition,
        float pathWidth
    );
};

struct AINetNode {
    zVec3 position;
    union {
        AINetNode *neighborNodes[3];
        int neighborIndices[3];
    };
    AINetPathProbeFan *probeFans[3];
    int costOrType;
    int nodeIndex;
    AINetNode *next;

    void Free();
};

enum AINetType {
    AINET_TYPE_ST = 0,
    AINET_TYPE_HI = 1,
    AINET_TYPE_FI = 2,
    AINET_TYPE_DE = 3,
};

enum AINetAttackStrategy {
    AINET_STRAT_HEA = 0,
    AINET_STRAT_CIR = 1,
    AINET_STRAT_BAC = 2,
    AINET_STRAT_FOL = 3,
    AINET_STRAT_ZIG = 4,
    AINET_STRAT_SIT = 5,
};

struct AINet {
    int netId;
    char name[0x14];
    AINetType aiType;
    float pathWidth;
    float activateRadius;
    float attackRadius;
    float attackDwell;
    float notPursuitDwell;
    float pursuitParam0;
    float pursuitParam1;
    float returnRange;
    float hideTime0;
    float hideTime1;
    int activateBuddyNetId;
    int attackBuddyNetId;
    AINetAttackStrategy attackStrategy;
    AINetNode *nodeListHead;
    AINet *next;

    static void __cdecl LoadAllFromZrd();
    static AINet *__fastcall LoadFromZrd(int netId);
    static AINet *Alloc();
    static AINet *__fastcall FindByNetId(int netId);
    static AINetNode *__fastcall FindNearestNode(
        const zVec3 *position,
        AINetNode *nodeListHead
    );
    static AINetNode *__fastcall FindNodeByIndex(
        int nodeIndex,
        AINetNode *nodeListHead
    );
    static void __fastcall ResolveNeighborLinksAndBuildProbeFans(
        AINetNode *nodeListHead,
        float pathWidth
    );
    static void __fastcall TickAiMode2TopLevel(zUtil_SaveGameState *saveState);
    static void __fastcall TickAiMode2PathFollow(zUtil_SaveGameState *saveState);
    static int __fastcall AiMode2ForwardProbeRequiresAutoTurn(
        zUtil_SaveGameState *saveState
    );
    static void __fastcall AiAdvancePathCursorAndComputeTargetVec(
        zUtil_SaveGameState *saveState,
        AINetNode **currentNodeInOut,
        AINetPathProbeFan **outProbeFan,
        zVec3 *outTargetVec
    );
    static int __fastcall AiChooseNextPathBranchIndex(
        zUtil_SaveGameState *saveState,
        AINetNode **currentNodeInOut,
        int *outBranchIndex,
        int excludedBranchIndex
    );
    static void __fastcall TickAiMode2SteeringSubstate(
        zUtil_SaveGameState *saveState
    );
    static void __fastcall UpdateAiMode2MoveAndTurnTowardTarget(
        zUtil_SaveGameState *saveState,
        float forwardDot,
        float lateralDot,
        float targetDistance
    );
    static void __fastcall TickAiMode2OffsetTargetSteering(
        zUtil_SaveGameState *saveState,
        float unusedForwardDot,
        float unusedLateralDot,
        float unusedTargetDistance
    );
    static void __fastcall TickAiMode2DynamicOffsetTargetSteering(
        zUtil_SaveGameState *saveState,
        float unusedForwardDot,
        float unusedLateralDot,
        float targetDistance
    );
    static int __fastcall AiTryEnterMode2AttackPursuitIfLineOfSight(
        zUtil_SaveGameState *saveState
    );
    static void __fastcall AiAlertAttackBuddies(zUtil_SaveGameState *saveState);
    static void __fastcall AiEnterMode2SteeringPursuit(
        zUtil_SaveGameState *saveState
    );
    static int __fastcall HasLineOfSightFromLocalPlayerFxOffset(
        zClass_NodePartial *node,
        const zVec3 *point,
        int directionMode
    );
    static int __fastcall HasLineOfSightFromCameraTarget(
        zClass_NodePartial *node,
        const zVec3 *point,
        int directionMode
    );
    static void __fastcall AiRebuildSyntheticPathToNodeIfFar(
        zUtil_SaveGameState *saveState,
        AINetNode *targetNode
    );
    static void __fastcall AiRestoreSavedTopLevelState(
        zUtil_SaveGameState *saveState
    );
    static void __fastcall UpdateAiMode2TurnTowardPlayerNoThrottle(
        zUtil_SaveGameState *saveState
    );
    static void __fastcall UpdateAiMode2TurnInPlaceTowardPlayer(
        zUtil_SaveGameState *saveState
    );
    static void __fastcall TickAiMode2AltGunAttackWindow(
        zUtil_SaveGameState *saveState,
        float targetDistance,
        float forwardDot
    );
    static void __fastcall SolveAltGunLeadTargetPoint(
        zUtil_SaveGameState *saveState,
        zUtil_SaveGameState *targetSaveState,
        zVec3 *outTargetPos
    );
    static void __fastcall UpdateAiMode2MoveAndTurnTowardOffsetTarget(
        zUtil_SaveGameState *saveState,
        zUtil_SaveGameState *targetState
    );
    static void __fastcall UpdateAiMode2MoveAndTurnTowardDynamicOffsetTarget(
        zUtil_SaveGameState *saveState,
        zUtil_SaveGameState *targetState,
        float targetDistance
    );
    static void __fastcall TickAiMode2TimedPathSteering(
        zUtil_SaveGameState *saveState
    );
    static void __fastcall AiSteerTowardPathNodeForward(
        zUtil_SaveGameState *saveState
    );
    static void __fastcall AiSteerTowardPathNodeReverse(
        zUtil_SaveGameState *saveState
    );
    static void __cdecl AiFinalizeMode2State1ForAllPlayers();
    static void __cdecl BuildAiPeerRingsByAiNetId();
    static void __fastcall AiDiscardNegativeBranchPathNodes(
        zUtil_SaveGameState *saveState
    );
    void Free();
    static void __cdecl FreeAll();
};

extern "C" {
extern AINet *g_AINetListHead;
extern AINet *g_AINetListTail;
}

RECOIL_STATIC_ASSERT(
    offsetof(
        AINetNode,
        position
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINetNode,
        neighborNodes
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINetNode,
        probeFans
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINetNode,
        costOrType
    ) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINetNode,
        nodeIndex
    ) == 0x28
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINetNode,
        next
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(sizeof(AINetNode) == 0x30);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINetPathProbeFan,
        clampedTravel
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINetPathProbeFan,
        perpendicular
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINetPathProbeFan,
        probeDirPlus45
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINetPathProbeFan,
        probeDirMinus45
    ) == 0x28
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINetPathProbeFan,
        pathWidth
    ) == 0x34
);
RECOIL_STATIC_ASSERT(sizeof(AINetPathProbeFan) == 0x3c);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINet,
        netId
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINet,
        name
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINet,
        aiType
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINet,
        pathWidth
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINet,
        activateRadius
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINet,
        attackRadius
    ) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINet,
        attackDwell
    ) == 0x28
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINet,
        notPursuitDwell
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINet,
        pursuitParam0
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINet,
        pursuitParam1
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINet,
        returnRange
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINet,
        hideTime0
    ) == 0x3c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINet,
        hideTime1
    ) == 0x40
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINet,
        activateBuddyNetId
    ) == 0x44
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINet,
        attackBuddyNetId
    ) == 0x48
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINet,
        attackStrategy
    ) == 0x4c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINet,
        nodeListHead
    ) == 0x50
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AINet,
        next
    ) == 0x54
);
RECOIL_STATIC_ASSERT(sizeof(AINet) == 0x58);

#endif
#ifdef BATTLESPORT_AI_NET_EMIT_HEADER_BODIES
#ifndef BATTLESPORT_AI_NET_HEADER_BODIES
#define BATTLESPORT_AI_NET_HEADER_BODIES

#include "Battlesport/game_net.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth_decls.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"

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
const float kPlayerAiPathFollowAdvanceDistance = 10.0f;
const float kPlayerAiForwardPathAdvanceDistance = 5.0f;
const float kPlayerAiForwardProbeMinLength = 1.0f;
const float kPlayerAiForwardProbeLengthHalfScale = 0.5f;
const float kPlayerAiSyntheticPathRebuildDistanceSq = 400.0f;
const float kPlayerAiSyntheticPathWidth = 10.0f;
const float kPlayerAiSyntheticPathRebuildDelaySec = 1.0f;
const float kPlayerAiAttackLosTargetYOffset = -1.5f;
const float kPlayerAiDynamicOffsetBackUpDistance = 10.0f;
const unsigned int kOptCatalogFlagLockOnTargetRef = 0x4000;
const unsigned int kOptCatalogFlagCreateTrail = 0x02;

#define AINET_MAX(a, b) (((a) < (b)) ? (b) : (a))

#define AINET_VEC3_SUB_WORLD_DST_V0_WORLD_V1(dst, srcVec, world) \
    do {                                                 \
        v0 = &(dst);                                     \
        v1 = &(world);                                   \
        v0->x = (srcVec)->x - v1->x;                     \
        v0->y = (srcVec)->y - v1->y;                     \
        v0->z = (srcVec)->z - v1->z;                     \
    } while (0)

#define AINET_VEC3_SUB_WORLD_DST_V1_WORLD_V0(dst, srcVec, world) \
    do {                                                 \
        v1 = &(dst);                                     \
        v0 = &(world);                                   \
        v1->x = (srcVec)->x - v0->x;                     \
        v1->y = (srcVec)->y - v0->y;                     \
        v1->z = (srcVec)->z - v0->z;                     \
    } while (0)

#define AINET_VEC3_DOT_XZ(out, steer, delta) \
    do {                                     \
        v1 = &(delta);                       \
        v0 = &(steer);                       \
        (out) = v0->x * v1->x +              \
                v0->z * v1->z;               \
    } while (0)

#define AINET_VEC3_CROSS_XZ(out, steer, delta) \
    do {                                       \
        v1 = &(delta);                         \
        v2 = &(steer);                         \
        (out) = v2->z * v1->x -                \
                v2->x * v1->z;                 \
    } while (0)

#define AINET_TURN_DIRECTION_SLOT(cross) (*(int *)&(cross))

#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
/**
 * Raw assembly for 0x401420: emits the likely original VC5 x87 vector-add
 * helper body after C++ has bound destination/source pointer temps. ChatGPT Pro
 * source-shape review classified the surrounding normalize, scale, contact,
 * and return logic as C++ compiler output; only this fixed-register add body is
 * treated as the original inline-asm island.
 * Purpose: Add the player's world position into the forward probe endpoint.
 */
#define AINET_FORWARD_PROBE_ADD_WORLD_ASM(dstArg, worldArg, endArg) \
    do {                                                           \
        zVec3 *zaddDst = (dstArg);                                 \
        zVec3 *zaddWorld = (worldArg);                             \
        zVec3 *zaddEnd = (endArg);                                 \
        __asm mov ebx, zaddEnd                                     \
        __asm mov ecx, zaddWorld                                   \
        __asm mov edx, zaddDst                                     \
        __asm fld dword ptr [ebx+0]                                \
        __asm fadd dword ptr [ecx+0]                               \
        __asm fld dword ptr [ebx+4]                                \
        __asm fadd dword ptr [ecx+4]                               \
        __asm fld dword ptr [ebx+8]                                \
        __asm fadd dword ptr [ecx+8]                               \
        __asm fxch ST(2)                                           \
        __asm fstp dword ptr [edx+0]                               \
        __asm fstp dword ptr [edx+4]                               \
        __asm fstp dword ptr [edx+8]                               \
    } while (0)

/**
 * Original-source helper evidence: no standalone retail function exists.
 * The repeated callers at 0x401180, 0x401580, 0x401710, 0x401c60, 0x402090,
 * 0x402170, 0x402be0, 0x402d60, and 0x403620 share the same fixed-register EBX/ECX/EDX
 * grouped-x87 subtraction, `fxch`, and ordered-store sequence. C/C++ forms
 * failed to preserve that retail VC5 shape; the exact historical identifier
 * spelling remains unproven.
 * Purpose: Provide the recovered shared inlined AINet vector subtraction.
 */
#define AINET_VECTOR_SUBTRACT(destination, source, subtractor) \
    __asm {                                                     \
        __asm mov ebx, source                                  \
        __asm mov ecx, subtractor                              \
        __asm mov edx, destination                             \
        __asm fld dword ptr [ebx+0]                            \
        __asm fsub dword ptr [ecx+0]                           \
        __asm fld dword ptr [ebx+4]                            \
        __asm fsub dword ptr [ecx+4]                           \
        __asm fld dword ptr [ebx+8]                            \
        __asm fsub dword ptr [ecx+8]                           \
        __asm fxch ST(2)                                       \
        __asm fstp dword ptr [edx+0]                           \
        __asm fstp dword ptr [edx+4]                           \
        __asm fstp dword ptr [edx+8]                           \
    }

/**
 * Raw assembly wrapper for 0x401180: computes the auto-turn target delta while
 * preserving the observed VC5 local pointer binding for the recovered inlined
 * vector subtract helper.
 * Purpose: Produce the auto-turn target delta for path-follow recovery.
 */
#define AINET_PATH_COMPUTE_AUTO_TURN_DELTA(dst, srcVec, world) \
    do {                                                 \
        zVec3 *v0;                                       \
        zVec3 *v1;                                       \
        v0 = &(dst);                                     \
        v1 = &(world);                                   \
        AINET_VECTOR_SUBTRACT(v0, srcVec, v1)            \
    } while (0)

/**
 * Raw assembly wrapper for 0x401180: computes the path-follow target delta
 * while preserving the observed VC5 local pointer binding for the recovered
 * inlined vector subtract helper.
 * Purpose: Produce the steering target delta for path-follow movement.
 */
#define AINET_PATH_COMPUTE_PATH_TARGET_DELTA(dst, srcVec, world) \
    do {                                                 \
        zVec3 *v0;                                       \
        zVec3 *v1;                                       \
        v1 = &(dst);                                     \
        v0 = &(world);                                   \
        AINET_VECTOR_SUBTRACT(v1, srcVec, v0)            \
    } while (0)

/**
 * Raw assembly wrapper for 0x401c60: computes the dynamic offset direction
 * while preserving the observed VC5 local pointer binding for the recovered
 * inlined vector subtract helper.
 * Purpose: Produce the dynamic-offset pursuit direction.
 */
#define AINET_PATH_COMPUTE_DYNAMIC_OFFSET_DIR(dst, srcVec, world) \
    do {                                                    \
        zVec3 *v0;                                          \
        zVec3 *v1;                                          \
        zVec3 *v2;                                          \
        v1 = &(world);                                      \
        v2 = &(dst);                                        \
        v0 = &(srcVec);                                     \
        AINET_VECTOR_SUBTRACT(v2, v0, v1)                   \
    } while (0)

/**
 * Raw assembly wrapper for AINet path-probe setup: computes a segment delta
 * while preserving the observed VC5 local pointer binding for the recovered
 * inlined vector subtract helper.
 * Purpose: Produce the path-probe fan segment delta.
 */
#define AINET_PROBE_FAN_COMPUTE_SEGMENT_DELTA(dst, srcVec, world) \
    do {                                                    \
        zVec3 *v0;                                          \
        zVec3 *v1;                                          \
        zVec3 *v2;                                          \
        v2 = &(dst);                                        \
        v1 = &(world);                                      \
        v0 = &(srcVec);                                     \
        AINET_VECTOR_SUBTRACT(v2, v0, v1)                   \
    } while (0)

/**
 * Raw assembly wrapper for 0x403620: computes a segment delta and leaves the
 * destination pointer available to the following clamp helper.
 * Purpose: Produce the path-probe fan segment delta and retain its pointer.
 */
#define AINET_PROBE_FAN_COMPUTE_SEGMENT_DELTA_KEEP_PTR(dst, srcVec, world, dstPtr) \
    do {                                                                     \
        zVec3 *v0;                                                           \
        zVec3 *v1;                                                           \
        dstPtr = &(dst);                                                     \
        v1 = &(world);                                                       \
        v0 = &(srcVec);                                                      \
        AINET_VECTOR_SUBTRACT(dstPtr, v0, v1)                                \
    } while (0)

/**
 * Raw assembly for 0x403620: computes the XZ length, clamps travel against
 * path width, and stores the retail `clampedTravel` slot with the observed VC5
 * x87/control-flow shape. C/C++ clamp and assignment variants failed to
 * byte-match the retail register and FPU ordering.
 * Purpose: Preserve the byte-sensitive AINet path-probe travel clamp.
 */
#define AINET_PATH_PROBE_CLAMP_TRAVEL_VC5(deltaPtr, xzLengthLocal, pathWidthValue) \
    do {                                                                           \
        __asm mov ecx, deltaPtr                                                    \
        __asm fld dword ptr [ecx+0]                                                \
        __asm fmul dword ptr [ecx+0]                                               \
        __asm fld dword ptr [ecx+8]                                                \
        __asm fmul dword ptr [ecx+8]                                               \
        __asm faddp ST(1), ST(0)                                                   \
        __asm fsqrt                                                               \
        __asm fstp dword ptr xzLengthLocal                                         \
        __asm fld dword ptr pathWidthValue                                         \
        __asm fmul dword ptr g_AINetPathProbeHalfWidthScale                        \
        __asm fld dword ptr xzLengthLocal                                          \
        __asm fsub dword ptr pathWidthValue                                        \
        __asm fcomp ST(1)                                                          \
        __asm fnstsw ax                                                            \
        __asm test ah, 041h                                                        \
        __asm jne ainet_path_probe_clamp_store                                     \
        __asm fstp ST(0)                                                           \
        __asm mov ecx, deltaPtr                                                    \
        __asm fld dword ptr [ecx+0]                                                \
        __asm fmul dword ptr [ecx+0]                                               \
        __asm fld dword ptr [ecx+8]                                                \
        __asm fmul dword ptr [ecx+8]                                               \
        __asm faddp ST(1), ST(0)                                                   \
        __asm fsqrt                                                               \
        __asm fstp dword ptr xzLengthLocal                                         \
        __asm fld dword ptr xzLengthLocal                                          \
        __asm fsub dword ptr pathWidthValue                                        \
        __asm ainet_path_probe_clamp_store:                                        \
        __asm fstp dword ptr [esi+0Ch]                                             \
    } while (0)

/**
 * Raw assembly wrapper for 0x402be0 and 0x402d60: computes the forward path
 * node direction while preserving the caller's outer `v2` local required by
 * the recovered VC5 byte shape.
 * Purpose: Produce the direction to the next forward path node.
 */
#define AINET_PATH_COMPUTE_FORWARD_NODE_DIR(dst, srcVec, world) \
    do {                                                         \
        zVec3 *v0;                                               \
        zVec3 *v1;                                               \
        v1 = &(dst);                                             \
        v2 = &(world);                                           \
        v0 = &(srcVec);                                          \
        AINET_VECTOR_SUBTRACT(v1, v0, v2)                        \
    } while (0)

/**
 * Raw assembly wrapper for 0x402090 and 0x402170: computes a delta to the
 * local player's world position while preserving the observed VC5 local
 * pointer binding for the recovered inlined vector subtract helper.
 * Purpose: Produce the local-player target delta for turn-in-place helpers.
 */
#define AINET_PATH_COMPUTE_LOCAL_PLAYER_DELTA(dst, srcVec, world) \
    do {                                                          \
        zVec3 *v0;                                                \
        zVec3 *v1;                                                \
        v0 = &(dst);                                              \
        v1 = &(world);                                            \
        AINET_VECTOR_SUBTRACT(v0, srcVec, v1)                     \
    } while (0)

/**
 * Raw assembly for 0x401180: computes the XZ dot product using the observed VC5
 * x87 load/multiply/add/store sequence. C/C++ dot-product variants failed to
 * preserve the retail FPU stack and local pointer order recorded by the
 * ainet-vector exception.
 * Purpose: Produce byte-sensitive path-follow forward-dot math.
 */
#define AINET_PATH_DOT_XZ(out, steer, delta) \
    do {                                     \
        zVec3 *v0;                           \
        zVec3 *v1;                           \
        v1 = &(delta);                       \
        v0 = &(steer);                       \
        __asm mov ecx, v0                    \
        __asm mov edx, v1                    \
        __asm fld dword ptr [ecx+0]          \
        __asm fmul dword ptr [edx+0]         \
        __asm fld dword ptr [ecx+8]          \
        __asm fmul dword ptr [edx+8]         \
        __asm faddp ST(1), ST(0)             \
        __asm fstp dword ptr [out]           \
    } while (0)

/**
 * Raw assembly for 0x401180: computes the XZ cross product using the observed
 * VC5 x87 load/multiply/subtract/store sequence. C/C++ cross-product variants
 * failed to preserve the retail register and FPU ordering recorded by the
 * ainet-vector exception.
 * Purpose: Produce byte-sensitive path-follow steering-cross math.
 */
#define AINET_PATH_CROSS_XZ(out, steer, delta) \
    do {                                       \
        zVec3 *v1;                             \
        v1 = &(delta);                         \
        v2 = &(steer);                         \
        __asm mov ebx, v2                      \
        __asm mov ecx, v1                      \
        __asm fld dword ptr [ebx+8]            \
        __asm fmul dword ptr [ecx+0]           \
        __asm fld dword ptr [ebx+0]            \
        __asm fmul dword ptr [ecx+8]           \
        __asm fsubp ST(1), ST(0)               \
        __asm fstp dword ptr [out]             \
    } while (0)
#else
#define AINET_VECTOR_SUBTRACT(destination, source, subtractor) \
    do {                                                        \
        (destination)->x = (source)->x - (subtractor)->x;       \
        (destination)->y = (source)->y - (subtractor)->y;       \
        (destination)->z = (source)->z - (subtractor)->z;       \
    } while (0)
#define AINET_FORWARD_PROBE_ADD_WORLD_ASM(dstArg, worldArg, endArg) \
    do {                                                           \
        (dstArg)->x = (endArg)->x + (worldArg)->x;                  \
        (dstArg)->y = (endArg)->y + (worldArg)->y;                  \
        (dstArg)->z = (endArg)->z + (worldArg)->z;                  \
    } while (0)
#define AINET_PATH_COMPUTE_AUTO_TURN_DELTA(dst, srcVec, world) AINET_VEC3_SUB_WORLD_DST_V0_WORLD_V1(dst, srcVec, world)
#define AINET_PATH_COMPUTE_PATH_TARGET_DELTA(dst, srcVec, world) AINET_VEC3_SUB_WORLD_DST_V1_WORLD_V0(dst, srcVec, world)
#define AINET_PATH_COMPUTE_LOCAL_PLAYER_DELTA(dst, srcVec, world) AINET_VEC3_SUB_WORLD_DST_V0_WORLD_V1(dst, srcVec, world)
#define AINET_PATH_COMPUTE_DYNAMIC_OFFSET_DIR(dst, srcVec, world) \
    do {                                                    \
        (dst).x = (srcVec).x - (world).x;                   \
        (dst).y = (srcVec).y - (world).y;                   \
        (dst).z = (srcVec).z - (world).z;                   \
    } while (0)
#define AINET_PROBE_FAN_COMPUTE_SEGMENT_DELTA(dst, srcVec, world) AINET_PATH_COMPUTE_DYNAMIC_OFFSET_DIR(dst, srcVec, world)
#define AINET_PROBE_FAN_COMPUTE_SEGMENT_DELTA_KEEP_PTR(dst, srcVec, world, dstPtr) AINET_PATH_COMPUTE_DYNAMIC_OFFSET_DIR(dst, srcVec, world)
#define AINET_PATH_COMPUTE_FORWARD_NODE_DIR(dst, srcVec, world) AINET_PATH_COMPUTE_DYNAMIC_OFFSET_DIR(dst, srcVec, world)
#define AINET_PATH_DOT_XZ(out, steer, delta) AINET_VEC3_DOT_XZ(out, steer, delta)
#define AINET_PATH_CROSS_XZ(out, steer, delta) AINET_VEC3_CROSS_XZ(out, steer, delta)
#endif

/**
 * Reimplements 0x401060: AINet::TickAiMode2TopLevel (Battlesport/ai_net.h).
 * Purpose: Dispatches the active mode-2 top-level state and attack-pursuit transitions. Source model: AINet source-file contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::TickAiMode2TopLevel(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zUtil_PlayerStateStorage *const localPlayerState =
        ((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState;
    playerState->storedTargetPos = localPlayerState->fxOffsetWorld;

    switch (playerState->aiTopLevelState) {
    case kPlayerAiTopPathFollow:
        TickAiMode2PathFollow(saveState);
        if (AiTryEnterMode2AttackPursuitIfLineOfSight(saveState)) {
            AiRebuildSyntheticPathToNodeIfFar(
                saveState,
                playerState->aiCurrentPathNode
                    ->neighborNodes[playerState->aiCurrentPathNeighborIndex]
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
                playerState->aiCurrentPathNode
                    ->neighborNodes[playerState->aiCurrentPathNeighborIndex]
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
                playerState->aiCurrentPathNode
                    ->neighborNodes[playerState->aiCurrentPathNeighborIndex]
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
                playerState->aiCurrentPathNode
                    ->neighborNodes[playerState->aiCurrentPathNeighborIndex]
            );
        }
        return;

    default:
        return;
    }
}

#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
#pragma optimize("y", off)
#endif

/**
 * Reimplements 0x401180: AINet::TickAiMode2PathFollow (Battlesport/ai_net.h).
 * Purpose: Steers toward the current AI path edge, advances the cursor, or arms auto-turn. Source model: AINet source-file contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::TickAiMode2PathFollow(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    AINetNode *currentNode;
    AINetPathProbeFan *edgeProbeFan;
#if !(defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100)
    zVec3 *v0;
    zVec3 *v1;
#endif
    zVec3 *v2;
    zVec3 *targetPathNode;
    float steerDotXZ;
    PlayerMasterModalData *masterModalData;
    float targetDistance;
    zVec3 targetDelta;
    zVec3 steerBasis;
    zVec3 autoTurnTargetDelta;

    masterModalData = saveState->primaryModalState->masterModalData;
    currentNode = playerState->aiCurrentPathNode;
    edgeProbeFan =
        currentNode->probeFans[playerState->aiCurrentPathNeighborIndex];
    targetPathNode =
        &currentNode->neighborNodes[playerState->aiCurrentPathNeighborIndex]->position;

    if (AiMode2ForwardProbeRequiresAutoTurn(saveState) != 0) {
        AiAdvancePathCursorAndComputeTargetVec(
            saveState,
            &currentNode,
            &edgeProbeFan,
            &targetDelta
        );
        targetPathNode =
            &currentNode->neighborNodes[playerState->aiCurrentPathNeighborIndex]->position;
        playerState->aiReturnTopLevelState = playerState->aiTopLevelState;
        playerState->aiTopLevelState = kPlayerAiTopAutoTurn;
        playerState->autoTurnActive = 1;

        AINET_PATH_COMPUTE_AUTO_TURN_DELTA(
            autoTurnTargetDelta,
            targetPathNode,
            playerState->worldPos
        );
        autoTurnTargetDelta.y = 0.0f;
        zMath::Vec3NormalizeXZ(
            &autoTurnTargetDelta,
            &playerState->autoTurnTargetDir
        );
        playerState->throttleInput = 0.0f;
        playerState->throttleInputCopy = 0.0f;
        playerState->steeringInput = 0.0f;
        return;
    }

    AINET_PATH_COMPUTE_PATH_TARGET_DELTA(
        targetDelta,
        targetPathNode,
        playerState->worldPos
    );
    targetDelta.y = 0.0f;
    targetDistance = zMath::Vec3Normalize(&targetDelta);

    steerBasis = playerState->steerBasisNorm;
    AINET_PATH_DOT_XZ(
        steerDotXZ,
        steerBasis,
        targetDelta
    );
    float steerCrossXZ;
    AINET_PATH_CROSS_XZ(
        steerCrossXZ,
        steerBasis,
        targetDelta
    );

    if (steerDotXZ < 0.0f) {
        if (playerState->aiPathCursorAdvanceRequested != 0) {
            AiAdvancePathCursorAndComputeTargetVec(
                saveState,
                &currentNode,
                &edgeProbeFan,
                &targetDelta
            );
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
        const float pitchInput = ((targetPathNode->y - playerState->worldPos.y +
                                      masterModalData->modeAltTransitionTime) *
                                         g_Player_AiMode2_PathFollowPitchInputScale -
                                     playerState->vehiclePitchRad) *
                                 g_Player_AiMode2_PathFollowPitchTurnGain;
        playerState->subPitchInput = pitchInput;
        playerState->subPitchInputCopy = pitchInput;
    }

    if (targetDistance < kPlayerAiPathFollowAdvanceDistance) {
        AiAdvancePathCursorAndComputeTargetVec(
            saveState,
            &currentNode,
            &edgeProbeFan,
            &targetDelta
        );
        playerState->aiPathCursorAdvanceRequested = 0;
    }
}

/**
 * Reimplements 0x401420: AINet::AiMode2ForwardProbeRequiresAutoTurn (Battlesport/ai_net.h).
 * Purpose: Checks forward probe queues and requests auto-turn recovery when blocked. Source model: AINet source-file contribution over save-state/playerState, not a Player class.
 */
int __fastcall AINet::AiMode2ForwardProbeRequiresAutoTurn(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *masterModalData =
        saveState->primaryModalState->masterModalData;
    int segmentTags[2];
    zVec3 forwardDir;
    zClass_DiSegmentEndpoints segmentPairs[1];

    if (playerState->playerCollisionResolved != 0 || playerState->preferredCollisionResolved != 0) {
        ++playerState->aiMode2SteeringRetryCount;
        return 1;
    }

    segmentPairs[0].start = playerState->worldPos;
    segmentPairs[0].start.y += masterModalData->probePoints[1].y;

    forwardDir = playerState->projectileSpawnVel;

    const float forwardProbeOffset =
        AINET_MAX(
            zMath::Vec3Normalize(&forwardDir),
            kPlayerAiForwardProbeMinLength
        ) * kPlayerAiForwardProbeLengthHalfScale -
        masterModalData->probePoints[1].z;
    segmentPairs[0].end.x = forwardProbeOffset * forwardDir.x;
    segmentPairs[0].end.y = forwardProbeOffset * forwardDir.y;
    segmentPairs[0].end.z = forwardProbeOffset * forwardDir.z;
    AINET_FORWARD_PROBE_ADD_WORLD_ASM(
        &segmentPairs[0].end,
        &playerState->worldPos,
        &segmentPairs[0].end
    );

    segmentTags[0] = -1;
    segmentTags[1] = -1;
    Player::CollectPendingContactsForSegments(
        saveState,
        segmentPairs,
        2,
        segmentTags
    );

    int result;
    if (playerState->preferredCollisionQueue.count != 0 ||
        playerState->playerCollisionQueue.count != 0) {
        result = 1;
    } else {
        result = 0;
    }
    Player::ClearPendingContactQueues(saveState);
    return result;
}

/**
 * Reimplements 0x401580: AINet::AiAdvancePathCursorAndComputeTargetVec (Battlesport/ai_net.h).
 * Purpose: Advances the AI path cursor and returns the target vector and probe fan. Source model: AINet source-file contribution over save-state/playerState, not a Player class.
 * Preserve the pre-tested `while (branchOffset < 0x18)` for VC5 byte shape:
 * VC5 folds its initially true entry test and emits the retail direct `jl`
 * latch, while equivalent `do/while` and indefinite-loop/positive-`continue`
 * forms add a two-byte backedge trampoline. The exact original lexical tokens
 * remain unproven.
 */
void __fastcall AINet::AiAdvancePathCursorAndComputeTargetVec(
    zUtil_SaveGameState *saveState,
    AINetNode **currentNodeInOut,
    AINetPathProbeFan **outProbeFan,
    zVec3 *outTargetVec
) {
    zUtil_PlayerStateStorage *playerState = saveState->playerState;
    AINetNode **nodeInOut = currentNodeInOut;
    int chosenBranchIndex;

    int pathNeighborIndex = playerState->aiCurrentPathNeighborIndex;
    AINetNode *nextNode = (*nodeInOut)->neighborNodes[pathNeighborIndex];
    playerState->aiCurrentPathNode = nextNode;

    int previousNodeIndex = (*nodeInOut)->nodeIndex;
    if (previousNodeIndex < 0) {
        (*nodeInOut)->Free();
        *nodeInOut = playerState->aiCurrentPathNode;

        if ((*nodeInOut)->nodeIndex < 0) {
            playerState->aiCurrentPathNeighborIndex = 0;
        } else {
            AINet::AiChooseNextPathBranchIndex(
                saveState,
                nodeInOut,
                &chosenBranchIndex,
                -1
            );
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
            AINetNode *reverseNode =
                *(AINetNode **)((char *)nextNode + branchOffset);
            if (reverseNode != 0 &&
                reverseNode->nodeIndex == previousNodeIndex) {
                excludedBranchIndex = candidateBranchIndex;
                break;
            }

            branchOffset += 4;
            ++candidateBranchIndex;
        }

        AINet::AiChooseNextPathBranchIndex(
            saveState,
            nodeInOut,
            &chosenBranchIndex,
            excludedBranchIndex
        );
        playerState->aiCurrentPathNeighborIndex = chosenBranchIndex;
    }

    int index = playerState->aiCurrentPathNeighborIndex;
    *outProbeFan = (*nodeInOut)->probeFans[index];

    zVec3 *worldPosition;
    zVec3 *selectedPosition;
    worldPosition = &playerState->worldPos;
    selectedPosition = &(*nodeInOut)->position;
    AINET_VECTOR_SUBTRACT(outTargetVec, worldPosition, selectedPosition);
}

#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
#pragma optimize("y", on)
#endif

/**
 * Reimplements 0x4016a0: AINet::AiChooseNextPathBranchIndex (Battlesport/ai_net.h).
 * Purpose: Selects the next non-excluded AI path branch for mode-2 steering.
 */
int __fastcall AINet::AiChooseNextPathBranchIndex(
    zUtil_SaveGameState *saveState,
    AINetNode **currentNodeInOut,
    int *outBranchIndex,
    int excludedBranchIndex
) {
    (void)saveState;

    AINetNode *currentNode = *currentNodeInOut;
    int branchCount = 0;
    AINetNode **neighborSlot = currentNode->neighborNodes;
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
 * Reimplements 0x401710: AINet::TickAiMode2SteeringSubstate (Battlesport/ai_net.h).
 * Purpose: Runs pursuit steering, submarine vertical controls, and pursuit exit checks. Source model: AINet source-file contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::TickAiMode2SteeringSubstate(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    float forwardDot;
    float verticalDistanceScale;
    float targetDistance;
    float lateralDot;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    const zVec3 targetWorldSnapshot =
        ((zUtil_PlayerStateStorage *)g_GameStateOrMapTable->playerState)->worldPos;

    if (g_Player_TotalTimeSecScaled >= playerState->aiNextPathRebuildTime &&
        playerState->aiCurrentSteeringSubstate != kPlayerAiMode2SteerPathFollow) {
        AiRebuildSyntheticPathToNodeIfFar(
            saveState,
            playerState->aiCurrentPathNode
        );
    }

    zVec3 targetDelta;
    {
        zVec3 *v0;
        zVec3 *v1;
        const zVec3 *v2;
        v0 = &targetDelta;
        v1 = &playerState->worldPos;
        v2 = &targetWorldSnapshot;
        AINET_VECTOR_SUBTRACT(v0, v2, v1);
    }
    verticalDistanceScale = targetDelta.y;
    targetDelta.y = 0.0f;
    targetDistance = zMath::Vec3Normalize(&targetDelta);
    verticalDistanceScale =
        targetDistance != 0.0f ? verticalDistanceScale / targetDistance : 0.0f;

    const zVec3 steerBasisNorm = playerState->steerBasisNorm;
    lateralDot = steerBasisNorm.z * targetDelta.x -
                 steerBasisNorm.x * targetDelta.z;
    forwardDot = steerBasisNorm.x * targetDelta.x +
                 steerBasisNorm.z * targetDelta.z;

    if (playerState->aiMode2SteeringRetryCount > 6) {
        playerState->aiCurrentSteeringSubstate = kPlayerAiMode2SteerTurnInPlace;
    }

    switch (playerState->aiCurrentSteeringSubstate) {
    case kPlayerAiMode2SteerDirectTarget:
        UpdateAiMode2MoveAndTurnTowardTarget(
            saveState,
            forwardDot,
            lateralDot,
            targetDistance
        );
        break;
    case kPlayerAiMode2SteerOffsetTarget:
        TickAiMode2OffsetTargetSteering(
            saveState,
            forwardDot,
            lateralDot,
            targetDistance
        );
        forwardDot = 1.0f;
        break;
    case kPlayerAiMode2SteerDynamicOffsetTarget:
        TickAiMode2DynamicOffsetTargetSteering(
            saveState,
            forwardDot,
            lateralDot,
            targetDistance
        );
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
        const float pitchInput = (g_Player_AiMode2_SteeringPitchInputScale * verticalDistanceScale -
                                     playerState->vehiclePitchRad) *
                                 g_Player_AiMode2_SteeringPitchTurnGain;
        playerState->subPitchInput = pitchInput;
        playerState->subPitchInputCopy = pitchInput;

        const float verticalInput = (targetWorldSnapshot.y - playerState->worldPos.y) *
                                    g_Player_AiMode2_SteeringVerticalErrorScale;
        playerState->subVerticalInput = verticalInput;
        playerState->subVerticalInputCopy = verticalInput;
    }

    TickAiMode2AltGunAttackWindow(
        saveState,
        targetDistance,
        forwardDot
    );

    zUtil_PlayerStateStorage *targetPlayerState =
        (zUtil_PlayerStateStorage *)g_GameStateOrMapTable->playerState;
    if (targetPlayerState->lifecycleState == kPlayerLifecycleInactive ||
        zMath::Vec3DeltaLengthSq(
            &playerState->worldPos,
            &playerState->aiRestoreTarget
        ) >
            playerState->aiRestoreDistanceSq) {
        AiRestoreSavedTopLevelState(saveState);
        playerState->aiStateUntilTime =
            g_Player_TotalTimeSecScaled + playerState->aiNotPursuitDwell;
    }
}

/**
 * Reimplements 0x401970: AINet::UpdateAiMode2MoveAndTurnTowardTarget (Battlesport/ai_net.h).
 * Purpose: Converts target alignment and pursuit distance into throttle and steering input. Source model: AINet source-file contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::UpdateAiMode2MoveAndTurnTowardTarget(
    zUtil_SaveGameState *saveState,
    float forwardDot,
    float lateralDot,
    float targetDistance
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    float *const throttleInput = &playerState->throttleInput;
    float *const steeringInput = &playerState->steeringInput;

    if (forwardDot <= 0.0f) {
        *throttleInput = 0.0f;
        *steeringInput = (float)(lateralDot < 0.0f ? -1 : 1);
        playerState->throttleInputCopy = *throttleInput;
        playerState->steeringInputCopy = *steeringInput;
        return;
    }

    AINet *const aiNet = playerState->aiNet;
    playerState->steeringInput = lateralDot;
    if (targetDistance > aiNet->pursuitParam1) {
        *throttleInput = 1.0f;
        playerState->steeringInputCopy = *steeringInput;
        playerState->throttleInputCopy = *throttleInput;
        return;
    }

    if (targetDistance < aiNet->pursuitParam0) {
        *throttleInput = -1.0f;
        playerState->steeringInputCopy = *steeringInput;
        playerState->throttleInputCopy = *throttleInput;
        return;
    }

    *throttleInput = 0.0f;
    playerState->steeringInputCopy = *steeringInput;
    playerState->throttleInputCopy = *throttleInput;
}

/**
 * Reimplements 0x401a40: AINet::TickAiMode2OffsetTargetSteering (Battlesport/ai_net.h).
 * Purpose: Runs offset-target pursuit or switches to auto-turn recovery when blocked. Source model: AINet source-file contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::TickAiMode2OffsetTargetSteering(
    zUtil_SaveGameState *saveState,
    float unusedForwardDot,
    float unusedLateralDot,
    float unusedTargetDistance
) {
    (void)unusedForwardDot;
    (void)unusedLateralDot;
    (void)unusedTargetDistance;

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (AiMode2ForwardProbeRequiresAutoTurn(saveState) == 0) {
        UpdateAiMode2MoveAndTurnTowardOffsetTarget(
            saveState,
            (zUtil_SaveGameState *)g_GameStateOrMapTable
        );
        return;
    }

    Player::SetAutoTurnTargetDirFromWorldPoint(
        saveState,
        &((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState->worldPos
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
 * Reimplements 0x401ab0: AINet::TickAiMode2DynamicOffsetTargetSteering (Battlesport/ai_net.h).
 * Purpose: Runs dynamic-offset pursuit or switches to auto-turn recovery when blocked. Source model: AINet source-file contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::TickAiMode2DynamicOffsetTargetSteering(
    zUtil_SaveGameState *saveState,
    float unusedForwardDot,
    float unusedLateralDot,
    float targetDistance
) {
    (void)unusedForwardDot;
    (void)unusedLateralDot;

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (AiMode2ForwardProbeRequiresAutoTurn(saveState) == 0) {
        UpdateAiMode2MoveAndTurnTowardDynamicOffsetTarget(
            saveState,
            (zUtil_SaveGameState *)g_GameStateOrMapTable,
            targetDistance
        );
        return;
    }

    Player::SetAutoTurnTargetDirFromWorldPoint(
        saveState,
        &((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState->worldPos
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
 * Reimplements 0x401b20: AINet::AiTryEnterMode2AttackPursuitIfLineOfSight (Battlesport/ai_net.h).
 * Purpose: Tests attack range and local-player line of sight before steering pursuit. Source model: AINet source-file contribution over save-state/playerState, not a Player class.
 */
int __fastcall AINet::AiTryEnterMode2AttackPursuitIfLineOfSight(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const aiState = saveState->playerState;
    if (g_Player_AiMode2State1Finalized == 0) {
        if (g_Player_TotalTimeSecScaled > aiState->aiStateUntilTime) {
            zUtil_PlayerStateStorage *const localPlayerState =
                ((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState;
            const float targetDistSq =
                zMath::Vec3DeltaLengthSq(
                    &localPlayerState->fxOffsetWorld,
                    &aiState->fxOffsetWorld
                );
            if (targetDistSq < aiState->aiAttackRadiusSq) {
                zVec3 lineOfSightPoint = aiState->fxOffsetWorld;
                lineOfSightPoint.y -= kPlayerAiAttackLosTargetYOffset;
                if (HasLineOfSightFromLocalPlayerFxOffset(
                    aiState->rootNode,
                    &lineOfSightPoint,
                    1
                ) != 0) {
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
 * Reimplements 0x401c00: AINet::AiAlertAttackBuddies (Battlesport/ai_net.h).
 * Purpose: Propagates an attack-pursuit alert around the AI peer ring. Source model: AINet source-file contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::AiAlertAttackBuddies(
    zUtil_SaveGameState *saveState
) {
    zUtil_SaveGameState *buddySaveState = saveState->aiPeerRingNext;
    if (g_Player_AiMode2State1Finalized != 0 || buddySaveState == saveState) {
        return;
    }

    do {
        if (buddySaveState->playerState->aiTopLevelState != kPlayerAiMode2TopSteering) {
            AiEnterMode2SteeringPursuit(buddySaveState);
            buddySaveState->playerState->recentHitFlag = 1;
            buddySaveState->playerState->recentHitExpireTime =
                g_Time_AccumulatedTimeSec + 10.0f;
        }
        buddySaveState = buddySaveState->aiPeerRingNext;
    } while (buddySaveState != saveState);
}

/**
 * Reimplements 0x401c60: AINet::AiEnterMode2SteeringPursuit (Battlesport/ai_net.h).
 * Purpose: Saves the prior top-level state and enters steering pursuit for the attack window. Source model: AINet source-file contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::AiEnterMode2SteeringPursuit(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const aiState = saveState->playerState;
    zUtil_PlayerStateStorage *const localPlayerState =
        ((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState;
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

    AINetNode *const restorePathNode =
        aiState->aiCurrentPathNode->neighborNodes[aiState->aiCurrentPathNeighborIndex];
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
    zVec3 *v0;
    zVec3 *v1;
#endif
    AINET_PATH_COMPUTE_DYNAMIC_OFFSET_DIR(
        aiState->aiDynamicOffsetDir,
        aiState->worldPos,
        localPlayerState->worldPos
    );
    aiState->aiDynamicOffsetDir.y = 0.0f;
    zMath::Vec3Normalize(&aiState->aiDynamicOffsetDir);
}

/**
 * Reimplements 0x401d50: AINet::HasLineOfSightFromLocalPlayerFxOffset
 * (Battlesport/ai_net.h).
 * Purpose: tests whether the active local player fx-offset position has an
 * unobstructed ray path to the supplied point while temporarily excluding the
 * tested node and local player root from raycast candidates.
 */
int __fastcall AINet::HasLineOfSightFromLocalPlayerFxOffset(
    zClass_NodePartial *node,
    const zVec3 *point,
    int directionMode
) {
    zUtil_PlayerStateStorage *const playerState =
        (zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState);

    g_Variant_CurrentTag = playerState->variantTag;
    zClass_Class::gwNodeSetRaycastable(
        node,
        0
    );
    zClass_Class::gwNodeSetRaycastable(
        playerState->rootNode,
        0
    );
    zClass_cls_di::SetBreakOnFirstCandidate(1);
    zClass_cls_di::SetStopAfterFirstHit(0x40000);

    PlayerProbeSampleCandidateBuffer rayData;
    int raycastResult;
    if (directionMode == 1) {
        raycastResult = zClass_cls_di::RaycastFindClosest(
            g_Player_RuntimeDiScene,
            &rayData,
            playerState->fxOffsetWorld.x,
            playerState->fxOffsetWorld.y,
            playerState->fxOffsetWorld.z,
            point->x,
            point->y,
            point->z
        );
    } else {
        raycastResult = zClass_cls_di::RaycastFindClosest(
            g_Player_RuntimeDiScene,
            &rayData,
            point->x,
            point->y,
            point->z,
            playerState->fxOffsetWorld.x,
            playerState->fxOffsetWorld.y,
            playerState->fxOffsetWorld.z
        );
    }

    zClass_cls_di::SetBreakOnFirstCandidate(0);
    zClass_Class::gwNodeSetRaycastable(
        playerState->rootNode,
        1
    );
    zClass_Class::gwNodeSetRaycastable(
        node,
        1
    );

    return raycastResult == 0 && rayData.candidateCount != 0 ? 0 : 1;
}

/**
 * Reimplements 0x401e50: AINet::HasLineOfSightFromCameraTarget
 * (Battlesport/ai_net.h).
 * Purpose: tests whether the active camera target has an unobstructed ray path
 * to the supplied point while temporarily excluding the tested node and local
 * player root from raycast candidates.
 */
int __fastcall AINet::HasLineOfSightFromCameraTarget(
    zClass_NodePartial *node,
    const zVec3 *point,
    int directionMode
) {
    zUtil_PlayerStateStorage *const playerState =
        (zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState);

    zVec3 cameraTarget;
    zClass_Camera::gwCameraGetTarget(
        g_MainCamera,
        &cameraTarget.x,
        &cameraTarget.y,
        &cameraTarget.z
    );

    g_Variant_CurrentTag = playerState->variantTag;
    zClass_Class::gwNodeSetRaycastable(
        node,
        0
    );
    zClass_Class::gwNodeSetRaycastable(
        playerState->rootNode,
        0
    );
    zClass_cls_di::SetBreakOnFirstCandidate(1);
    zClass_cls_di::SetStopAfterFirstHit(0x40000);

    PlayerProbeSampleCandidateBuffer rayData;
    int raycastResult;
    if (directionMode == 1) {
        raycastResult = zClass_cls_di::RaycastFindClosest(
            g_Player_RuntimeDiScene,
            &rayData,
            cameraTarget.x,
            cameraTarget.y,
            cameraTarget.z,
            point->x,
            point->y,
            point->z
        );
    } else {
        raycastResult = zClass_cls_di::RaycastFindClosest(
            g_Player_RuntimeDiScene,
            &rayData,
            point->x,
            point->y,
            point->z,
            cameraTarget.x,
            cameraTarget.y,
            cameraTarget.z
        );
    }

    zClass_cls_di::SetBreakOnFirstCandidate(0);
    zClass_Class::gwNodeSetRaycastable(
        playerState->rootNode,
        1
    );
    zClass_Class::gwNodeSetRaycastable(
        node,
        1
    );

    return raycastResult == 0 && rayData.candidateCount != 0 ? 0 : 1;
}

/**
 * Reimplements 0x401f60: AINet::AiRebuildSyntheticPathToNodeIfFar (Battlesport/ai_net.h).
 * Purpose: Builds a temporary synthetic AI path node back to the requested target. Source model: AINet source-file contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::AiRebuildSyntheticPathToNodeIfFar(
    zUtil_SaveGameState *saveState,
    AINetNode *targetNode
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    AINetNode *const currentPathNode = playerState->aiCurrentPathNode;

    if (zMath::Vec3DeltaLengthSq(&playerState->worldPos, &currentPathNode->position) <
        kPlayerAiSyntheticPathRebuildDistanceSq) {
        return;
    }

    AINetNode *const syntheticNode = (AINetNode *)(malloc(sizeof(AINetNode)));
    memset(
        syntheticNode,
        0,
        sizeof(*syntheticNode)
    );
    syntheticNode->neighborNodes[0] = targetNode;
    syntheticNode->position = playerState->worldPos;
    syntheticNode->nodeIndex = -1;

    AINetPathProbeFan *const fan = (AINetPathProbeFan *)(malloc(sizeof(AINetPathProbeFan)));
    memset(
        fan,
        0,
        sizeof(*fan)
    );
    syntheticNode->probeFans[0] = fan;
    fan->InitFromSegment(
        syntheticNode->position,
        currentPathNode->position,
        kPlayerAiSyntheticPathWidth
    );

    playerState->aiCurrentPathNode = syntheticNode;
    playerState->aiCurrentPathNeighborIndex = 0;
    playerState->aiNextPathRebuildTime =
        g_Player_TotalTimeSecScaled + kPlayerAiSyntheticPathRebuildDelaySec;
}

/**
 * Reimplements 0x402080: AINet::AiRestoreSavedTopLevelState.
 * BN shows a fastcall leaf that copies playerState->aiSavedTopLevelState to
 * playerState->aiTopLevelState through the save-state's playerState pointer.
 * Purpose: Restores a saved AI top-level state for one player save-state node.
 */
void __fastcall AINet::AiRestoreSavedTopLevelState(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->aiTopLevelState = playerState->aiSavedTopLevelState;
}

/**
 * Reimplements 0x402090: AINet::UpdateAiMode2TurnTowardPlayerNoThrottle (Battlesport/ai_net.h).
 * Purpose: Turns toward the local player while holding throttle at zero. Source model: AINet source-file contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::UpdateAiMode2TurnTowardPlayerNoThrottle(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
#if !(defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100)
    zVec3 *v0;
    zVec3 *v1;
#endif

    zVec3 targetDelta;
    zVec3 *const targetWorldPos =
        &((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState->worldPos;

    AINET_PATH_COMPUTE_LOCAL_PLAYER_DELTA(
        targetDelta,
        targetWorldPos,
        playerState->worldPos
    );
    targetDelta.y = 0.0f;
    zMath::Vec3Normalize(&targetDelta);

    const zVec3 steerBasis = playerState->steerBasisNorm;
    const float turnCross = steerBasis.z * targetDelta.x -
                            steerBasis.x * targetDelta.z;
    const float forwardDot = steerBasis.x * targetDelta.x +
                             steerBasis.z * targetDelta.z;
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
 * Reimplements 0x402170: AINet::UpdateAiMode2TurnInPlaceTowardPlayer (Battlesport/ai_net.h).
 * Purpose: Turns in place toward the local player without changing throttle. Source model: AINet source-file contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::UpdateAiMode2TurnInPlaceTowardPlayer(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
#if !(defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100)
    zVec3 *v0;
    zVec3 *v1;
#endif

    zVec3 targetDelta;
    zVec3 *const targetWorldPos =
        &((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState->worldPos;

    AINET_PATH_COMPUTE_LOCAL_PLAYER_DELTA(
        targetDelta,
        targetWorldPos,
        playerState->worldPos
    );
    targetDelta.y = 0.0f;
    zMath::Vec3Normalize(&targetDelta);

    const zVec3 steerBasis = playerState->steerBasisNorm;
    const float turnCross = steerBasis.z * targetDelta.x -
                            steerBasis.x * targetDelta.z;
    const float forwardDot = steerBasis.x * targetDelta.x +
                             steerBasis.z * targetDelta.z;
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
 * Reimplements 0x402250: AINet::TickAiMode2AltGunAttackWindow.
 * Original source path: Battlesport/ai_net.h.
 * Purpose: reimplement AINet::TickAiMode2AltGunAttackWindow from the recovered
 * Battlesport ai_net.cpp source-file contribution.
 */
void __fastcall AINet::TickAiMode2AltGunAttackWindow(
    zUtil_SaveGameState *saveState,
    float targetDistance,
    float forwardDot
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    if (g_Player_TotalTimeSecScaled > playerState->aiStateEndTime) {
        const float startTime = g_Player_TotalTimeSecScaled + playerState->aiNotPursuitDwell;
        playerState->aiStateStartTime = startTime;
        playerState->aiStateEndTime = startTime + playerState->aiMode2AttackDwell;
    }

    PlayerGunFireController *const activeAltGunController = playerState->activeAltGunController;

    if (g_Player_TotalTimeSecScaled <= activeAltGunController->nextDispatchTime) {
        if (playerState->altGunFireHeldFlag == 0) {
            return;
        }

        if (forwardDot >= kPlayerAiAltGunAttackForwardMin &&
            targetDistance <= activeAltGunController->aiAttackRangeMax &&
            ((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState->lifecycleState !=
                kPlayerLifecycleInactive) {
            playerState->storedTargetPos =
                ((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState->fxOffsetWorld;
            return;
        }

        playerState->altGunDispatchRequested = 0;
        activeAltGunController->nextDispatchTime =
            g_Player_TotalTimeSecScaled + activeAltGunController->dispatchRepeatDelay;
        return;
    }

    if (playerState->altGunFireHeldFlag != 0) {
        playerState->altGunDispatchRequested = 0;
        activeAltGunController->nextDispatchTime =
            g_Player_TotalTimeSecScaled + activeAltGunController->dispatchRepeatDelay;
        return;
    }

    if (g_Player_TotalTimeSecScaled <= playerState->aiStateStartTime ||
        playerState->damageProtectionActive != 0 || forwardDot <= kPlayerAiAltGunAttackForwardMin ||
        targetDistance >= activeAltGunController->aiAttackRangeMax ||
        targetDistance <= activeAltGunController->aiAttackRangeMin ||
        HasLineOfSightFromLocalPlayerFxOffset(
            playerState->rootNode,
            &playerState->fxOffsetWorld,
            1
        ) == 0 ||
        ((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState->lifecycleState ==
            kPlayerLifecycleInactive) {
        return;
    }

    playerState->altGunDispatchRequested = 1;

    float statusScale = playerState->statusMeterScaled;
    if (statusScale <= kPlayerAiAltGunStatusMinScale) {
        statusScale = kPlayerAiAltGunStatusMinScale;
    }

    activeAltGunController->nextDispatchTime =
        g_Player_TotalTimeSecScaled + activeAltGunController->dispatchRepeatDelay / statusScale;

    OptCatalogEntryDef *const optCatalogEntry = activeAltGunController->optCatalogEntry;
    const unsigned int flags = optCatalogEntry->flags;
    if ((flags & kOptCatalogFlagCreateTrail) != 0) {
        playerState->altGunFireHeldFlag = 1;
        OptCatalog::ActivateTrailRuntimeState(
            activeAltGunController->trailRuntimeState,
            playerState->playerOrdinal
        );
        activeAltGunController->nextDispatchTime =
            g_Player_TotalTimeSecScaled + activeAltGunController->dispatchRepeatDelay;
        return;
    }

    if ((flags & kOptCatalogFlagLockOnTargetRef) != 0) {
        playerState->progressTargetCount = 1;
        playerState->progressTargetSlots[0].targetPos =
            &((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState->fxOffsetWorld;
        playerState->progressTargetSlots[0].targetVelocity =
            &((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState->projectileSpawnVel;
        HudUi::ShowTopMessageLine(
            zLoc::GetMessageString(0x908),
            5.0f
        );
        return;
    }

    playerState->progressTargetCount = 0;
    playerState->progressTargetSlots[0].targetPos = 0;
    playerState->progressTargetSlots[0].targetVelocity = 0;
    SolveAltGunLeadTargetPoint(
        saveState,
        (zUtil_SaveGameState *)g_GameStateOrMapTable,
        &playerState->storedTargetPos
    );
}

/**
 * Reimplements 0x4024a0: AINet::SolveAltGunLeadTargetPoint.
 * Original source path: Battlesport/ai_net.h.
 * Purpose: reimplement AINet::SolveAltGunLeadTargetPoint from the recovered
 * Battlesport ai_net.cpp source-file contribution.
 */
void __fastcall AINet::SolveAltGunLeadTargetPoint(
    zUtil_SaveGameState *saveState,
    zUtil_SaveGameState *targetSaveState,
    zVec3 *outTargetPos
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zUtil_PlayerStateStorage *const targetPlayerState = targetSaveState->playerState;
    const float inverseProjectileVelocity =
        1.0f / playerState->activeAltGunController->optCatalogEntry->velocity;

    zVec3 scaledTargetDelta = {
        (targetPlayerState->worldPos.x - playerState->worldPos.x) * inverseProjectileVelocity,
        (targetPlayerState->worldPos.y - playerState->worldPos.y) * inverseProjectileVelocity,
        (targetPlayerState->worldPos.z - playerState->worldPos.z) * inverseProjectileVelocity,
    };
    zVec3 relativeVelocity = {
        targetPlayerState->projectileSpawnVel.x - playerState->projectileSpawnVel.x,
        targetPlayerState->projectileSpawnVel.y - playerState->projectileSpawnVel.y,
        targetPlayerState->projectileSpawnVel.z - playerState->projectileSpawnVel.z,
    };
    zVec3 scaledRelativeVelocity = {
        relativeVelocity.x * inverseProjectileVelocity,
        relativeVelocity.y * inverseProjectileVelocity,
        relativeVelocity.z * inverseProjectileVelocity,
    };

    const float relativeSpeedSq = scaledRelativeVelocity.x * scaledRelativeVelocity.x +
                                  scaledRelativeVelocity.y * scaledRelativeVelocity.y +
                                  scaledRelativeVelocity.z * scaledRelativeVelocity.z;
    const float quadraticA = 1.0f - relativeSpeedSq;
    if (quadraticA <= 0.0f) {
        *outTargetPos = targetPlayerState->worldPos;
        return;
    }

    const float quadraticB = scaledRelativeVelocity.x * scaledTargetDelta.x +
                             scaledRelativeVelocity.y * scaledTargetDelta.y +
                             scaledRelativeVelocity.z * scaledTargetDelta.z;
    const float targetDistanceSq = scaledTargetDelta.x * scaledTargetDelta.x +
                                   scaledTargetDelta.y * scaledTargetDelta.y +
                                   scaledTargetDelta.z * scaledTargetDelta.z;
    const float discriminant = quadraticA * targetDistanceSq + quadraticB * quadraticB;
    union {
        float value;
        int bits;
    } fastSqrtEstimate;
    fastSqrtEstimate.value = discriminant;
    fastSqrtEstimate.bits = (fastSqrtEstimate.bits >> 1) + 0x1fc00000;
    const float leadScale = (fastSqrtEstimate.value + quadraticB) / quadraticA;

    scaledRelativeVelocity.x = relativeVelocity.x * leadScale;
    scaledRelativeVelocity.y = relativeVelocity.y * leadScale;
    scaledRelativeVelocity.z = relativeVelocity.z * leadScale;
    outTargetPos->x = targetPlayerState->fxOffsetWorld.x + scaledRelativeVelocity.x;
    outTargetPos->y = targetPlayerState->fxOffsetWorld.y + scaledRelativeVelocity.y;
    outTargetPos->z = targetPlayerState->fxOffsetWorld.z + scaledRelativeVelocity.z;
    outTargetPos->y -= ((float)(rand()) * 3.05185094e-05f - 0.5f) * -2.0f;
}

/**
 * Reimplements 0x4026d0: AINet::UpdateAiMode2MoveAndTurnTowardOffsetTarget (Battlesport/ai_net.h).
 * Purpose: Rotates the target-to-AI vector by accepted tuning globals and steers to the offset point. Source model: AINet source-file contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::UpdateAiMode2MoveAndTurnTowardOffsetTarget(
    zUtil_SaveGameState *saveState,
    zUtil_SaveGameState *targetState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zUtil_PlayerStateStorage *const targetPlayerState = targetState->playerState;
    const float offsetDistance = playerState->aiNet->pursuitParam0;

    zVec3 targetToPlayerDir = {
        playerState->worldPos.x - targetPlayerState->worldPos.x,
        playerState->worldPos.y - targetPlayerState->worldPos.y,
        playerState->worldPos.z - targetPlayerState->worldPos.z,
    };
    targetToPlayerDir.y = 0.0f;
    zMath::Vec3Normalize(&targetToPlayerDir);

    zVec3 steerOffsetDir = {0};
    steerOffsetDir.y = 0.0f;
    steerOffsetDir.x =
        offsetDistance * (g_Player_AiMode2_OffsetTargetRotateCos15Deg * targetToPlayerDir.x -
                             g_Player_AiMode2_OffsetTargetRotateSin15Deg * targetToPlayerDir.z);
    steerOffsetDir.z =
        offsetDistance * (g_Player_AiMode2_OffsetTargetRotateCos15Deg * targetToPlayerDir.z +
                             g_Player_AiMode2_OffsetTargetRotateSin15Deg * targetToPlayerDir.x);

    zVec3 offsetTarget = {
        targetPlayerState->worldPos.x + steerOffsetDir.x,
        targetPlayerState->worldPos.y + steerOffsetDir.y,
        targetPlayerState->worldPos.z + steerOffsetDir.z,
    };

    zVec3 targetDir = {
        offsetTarget.x - playerState->worldPos.x,
        offsetTarget.y - playerState->worldPos.y,
        offsetTarget.z - playerState->worldPos.z,
    };
    targetDir.y = 0.0f;
    zMath::Vec3Normalize(&targetDir);

    const float forwardDot =
        playerState->steerBasisNorm.x * targetDir.x + playerState->steerBasisNorm.z * targetDir.z;
    const float turnCross =
        playerState->steerBasisNorm.z * targetDir.x - playerState->steerBasisNorm.x * targetDir.z;

    if (forwardDot < 0.0f) {
        playerState->throttleInput = 0.0f;
        playerState->steeringInput = turnCross < 0.0f ? -1.0f : 1.0f;
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
 * Reimplements 0x4028c0: AINet::UpdateAiMode2MoveAndTurnTowardDynamicOffsetTarget (Battlesport/ai_net.h).
 * Purpose: Blends dynamic pursuit and side-offset steering based on distance to the local player. Source model: AINet source-file contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::UpdateAiMode2MoveAndTurnTowardDynamicOffsetTarget(
    zUtil_SaveGameState *saveState,
    zUtil_SaveGameState *targetState,
    float targetDistance
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zUtil_PlayerStateStorage *const targetPlayerState = targetState->playerState;
    AINet *const aiNet = playerState->aiNet;
    const float pursuitDistance = aiNet->pursuitParam0;
    const float sideOffsetScale = aiNet->pursuitParam1;
    const float doublePursuitDistance = pursuitDistance + pursuitDistance;

    zVec3 dynamicOffsetDir = playerState->aiDynamicOffsetDir;
    zVec3 targetPoint = {
        targetPlayerState->worldPos.x + pursuitDistance * dynamicOffsetDir.x,
        targetPlayerState->worldPos.y + pursuitDistance * dynamicOffsetDir.y,
        targetPlayerState->worldPos.z + pursuitDistance * dynamicOffsetDir.z,
    };

    float blend = (doublePursuitDistance - targetDistance) / pursuitDistance;
    if (blend > 1.0f) {
        blend = 1.0f;
    } else if (blend < 0.0f) {
        blend = 0.0f;
    }

    int reverseSideOffset = 0;
    float signedSideScale = blend * sideOffsetScale;
    if (playerState->localVel.z > 0.0f && targetDistance < doublePursuitDistance) {
        reverseSideOffset = 1;
        signedSideScale = -signedSideScale;
        zVec3 sideOffset = {
            dynamicOffsetDir.z * signedSideScale,
            targetPoint.y * signedSideScale,
            -dynamicOffsetDir.x * signedSideScale,
        };
        targetPoint.x += sideOffset.x;
        targetPoint.y += sideOffset.y;
        targetPoint.z += sideOffset.z;
    } else {
        zVec3 sideOffset = {
            dynamicOffsetDir.z * signedSideScale,
            targetPoint.y * signedSideScale,
            -dynamicOffsetDir.x * signedSideScale,
        };
        targetPoint.x += sideOffset.x;
        targetPoint.y += sideOffset.y;
        targetPoint.z += sideOffset.z;
    }

    zVec3 targetDir = {
        targetPoint.x - playerState->worldPos.x,
        targetPoint.y - playerState->worldPos.y,
        targetPoint.z - playerState->worldPos.z,
    };
    targetDir.y = 0.0f;
    float targetDirDistance;
    targetDirDistance = zMath::Vec3Normalize(&targetDir);

    zVec3 steerBasis = playerState->steerBasisNorm;
    if (reverseSideOffset != 0) {
        steerBasis.x = -steerBasis.x;
        steerBasis.z = -steerBasis.z;
    }

    const float forwardDot = steerBasis.x * targetDir.x + steerBasis.z * targetDir.z;
    const float turnCross = steerBasis.z * targetDir.x - steerBasis.x * targetDir.z;

    if (forwardDot < 0.0f && targetDirDistance < kPlayerAiDynamicOffsetBackUpDistance) {
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
    }
    playerState->throttleInputCopy = playerState->throttleInput;
    playerState->steeringInputCopy = playerState->steeringInput;
}

/**
 * Reimplements 0x402b70: AINet::TickAiMode2TimedPathSteering (Battlesport/ai_net.h).
 * Purpose: Alternates timed forward and reverse path-node steering around the AI home path node. Source model: AINet source-file contribution over save-state/playerState, not a Player class.
 */
void __fastcall AINet::TickAiMode2TimedPathSteering(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    if (g_Player_TotalTimeSecScaled > playerState->unknown_0fa4) {
        AINetNode *const currentPathNode = playerState->aiCurrentPathNode;
        AINetNode *const pathAnchorNode = playerState->aiHomePathNode;

        if (currentPathNode == pathAnchorNode) {
            AiSteerTowardPathNodeForward(saveState);
        } else if (currentPathNode->neighborNodes[0] == pathAnchorNode &&
                   currentPathNode->nodeIndex != -1) {
            AiSteerTowardPathNodeReverse(saveState);
        } else {
            TickAiMode2PathFollow(saveState);
        }
    }

    playerState->recentHitFlag = 1;
}

/**
 * Reimplements 0x402be0: AINet::AiSteerTowardPathNodeForward.
 * Original source path: Battlesport/ai_net.h.
 * Purpose: reimplement AINet::AiSteerTowardPathNodeForward from the recovered
 * Battlesport ai_net.cpp source-file contribution.
 */
void __fastcall AINet::AiSteerTowardPathNodeForward(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
#if !(defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100)
    zVec3 *v0;
    zVec3 *v1;
#endif
    zVec3 *v2;

    zVec3 targetDir;
    zVec3 forwardNodePosition = playerState->aiCurrentPathNode->neighborNodes[0]->position;
    AINET_PATH_COMPUTE_FORWARD_NODE_DIR(
        targetDir,
        forwardNodePosition,
        playerState->worldPos
    );
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
    AINET_PATH_DOT_XZ(
        forwardDot,
        playerState->steerBasisNorm,
        targetDir
    );
    float turnCross;
    AINET_PATH_CROSS_XZ(
        turnCross,
        playerState->steerBasisNorm,
        targetDir
    );

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
 * Reimplements 0x402d60: AINet::AiSteerTowardPathNodeReverse.
 * Original source path: Battlesport/ai_net.h.
 * Purpose: reimplement AINet::AiSteerTowardPathNodeReverse from the recovered
 * Battlesport ai_net.cpp source-file contribution.
 */
void __fastcall AINet::AiSteerTowardPathNodeReverse(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
#if !(defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100)
    zVec3 *v0;
    zVec3 *v1;
#endif
    zVec3 *v2;

    zVec3 targetDir;
    zVec3 forwardNodePosition = playerState->aiCurrentPathNode->neighborNodes[0]->position;
    AINET_PATH_COMPUTE_FORWARD_NODE_DIR(
        targetDir,
        forwardNodePosition,
        playerState->worldPos
    );
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
    AINET_PATH_DOT_XZ(
        forwardDot,
        reverseSteerBasis,
        targetDir
    );
    float turnCross;
    AINET_PATH_CROSS_XZ(
        turnCross,
        reverseSteerBasis,
        targetDir
    );

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
 * Reimplements 0x402f10: AINet::AiFinalizeMode2State1ForAllPlayers.
 * BN shows traversal from g_PlayerSaveStateListHead, filtering
 * lifecycleState == 2 and aiTopLevelState == 1, restoring matching nodes, and
 * setting g_Player_AiMode2State1Finalized to 1 after the pass.
 * Purpose: Finalizes AI Mode2 State1 by restoring saved top-level state for
 * active AI players and setting the global finalization latch.
 */
void AINet::AiFinalizeMode2State1ForAllPlayers() {
    zUtil_SaveGameState *saveState = g_PlayerSaveStateListHead;
    while (saveState != 0) {
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        if (playerState->lifecycleState == 2 && playerState->aiTopLevelState == 1) {
            AiRestoreSavedTopLevelState(saveState);
        }

        saveState = saveState != 0 ? saveState->next : 0;
    }

    g_Player_AiMode2State1Finalized = 1;
}

#endif
#endif
