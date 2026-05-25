#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include "GameZRecoil/include/zClass.h"
#include "recoil/recoil_callconv.h"

struct AINetPathProbeFan;

struct AINetNode {
    zVec3 position;
    AINetNode *neighborNodes[3];
    AINetPathProbeFan *probeFans[3];
    unsigned char unknown_24[0x04];
    int nodeIndex;
    AINetNode *next;

    RECOIL_NOINLINE void RECOIL_THISCALL Free();
};

struct AINet {
    unsigned char unknown_00[0x50];
    AINetNode *nodeListHead;
    AINet *next;

    RECOIL_NOINLINE void RECOIL_THISCALL Free();
    RECOIL_NOINLINE static void RECOIL_CDECL FreeAll();
};

extern "C" {
extern AINet *g_AINetListHead;
}

RECOIL_STATIC_ASSERT(offsetof(AINetNode, position) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(AINetNode, neighborNodes) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(AINetNode, probeFans) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(AINetNode, nodeIndex) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(AINetNode, next) == 0x2c);
RECOIL_STATIC_ASSERT(sizeof(AINetNode) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(AINet, nodeListHead) == 0x50);
RECOIL_STATIC_ASSERT(offsetof(AINet, next) == 0x54);
