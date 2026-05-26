#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include "GameZRecoil/include/zClass.h"
#include "recoil/recoil_callconv.h"

struct AINetPathProbeFan {
    zVec3 delta;
    float clampedTravel;
    zVec3 perpendicular;
    zVec3 probeDirPlus45;
    zVec3 probeDirMinus45;
    float pathWidth;
    unsigned char unknown_38[0x04];

    RECOIL_NOINLINE void RECOIL_THISCALL InitFromSegment(zVec3 fromPosition, zVec3 toPosition,
                                                         float pathWidth);
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

    RECOIL_NOINLINE void RECOIL_THISCALL Free();
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

    RECOIL_NOINLINE static void RECOIL_CDECL LoadAllFromZrd();
    RECOIL_NOINLINE static AINet *RECOIL_FASTCALL LoadFromZrd(int netId);
    RECOIL_NOINLINE static AINet *RECOIL_CDECL Alloc();
    RECOIL_NOINLINE static AINet *RECOIL_FASTCALL FindByNetId(int netId);
    RECOIL_NOINLINE static AINetNode *RECOIL_FASTCALL FindNearestNode(const zVec3 *position,
                                                                      AINetNode *nodeListHead);
    RECOIL_NOINLINE static AINetNode *RECOIL_FASTCALL FindNodeByIndex(int nodeIndex,
                                                                      AINetNode *nodeListHead);
    RECOIL_NOINLINE static void RECOIL_FASTCALL
    ResolveNeighborLinksAndBuildProbeFans(AINetNode *nodeListHead, float pathWidth);
    RECOIL_NOINLINE void RECOIL_THISCALL Free();
    RECOIL_NOINLINE static void RECOIL_CDECL FreeAll();
};

extern "C" {
extern AINet *g_AINetListHead;
extern AINet *g_AINetListTail;
}

RECOIL_STATIC_ASSERT(offsetof(AINetNode, position) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(AINetNode, neighborNodes) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(AINetNode, probeFans) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(AINetNode, costOrType) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(AINetNode, nodeIndex) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(AINetNode, next) == 0x2c);
RECOIL_STATIC_ASSERT(sizeof(AINetNode) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(AINetPathProbeFan, clampedTravel) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(AINetPathProbeFan, perpendicular) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(AINetPathProbeFan, probeDirPlus45) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(AINetPathProbeFan, probeDirMinus45) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(AINetPathProbeFan, pathWidth) == 0x34);
RECOIL_STATIC_ASSERT(sizeof(AINetPathProbeFan) == 0x3c);
RECOIL_STATIC_ASSERT(offsetof(AINet, netId) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(AINet, name) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(AINet, aiType) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(AINet, pathWidth) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(AINet, activateRadius) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(AINet, attackRadius) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(AINet, attackDwell) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(AINet, notPursuitDwell) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(AINet, pursuitParam0) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(AINet, pursuitParam1) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(AINet, returnRange) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(AINet, hideTime0) == 0x3c);
RECOIL_STATIC_ASSERT(offsetof(AINet, hideTime1) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(AINet, activateBuddyNetId) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(AINet, attackBuddyNetId) == 0x48);
RECOIL_STATIC_ASSERT(offsetof(AINet, attackStrategy) == 0x4c);
RECOIL_STATIC_ASSERT(offsetof(AINet, nodeListHead) == 0x50);
RECOIL_STATIC_ASSERT(offsetof(AINet, next) == 0x54);
RECOIL_STATIC_ASSERT(sizeof(AINet) == 0x58);
