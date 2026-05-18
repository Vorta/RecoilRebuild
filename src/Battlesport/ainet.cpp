#include "Battlesport/ainet.h"

#include <stdlib.h>

extern "C" {
AINet *g_AINetListHead = 0;
}

// Reimplements 0x4037c0: AINetNode::Free (src/Battlesport/ainet.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL AINetNode::Free() {
    if (this == 0) {
        return;
    }

    {
    for (int index = 0; index < 3; ++index) {
        AINetPathProbeFan *probeFan = probeFans[index];
        if (probeFan != 0) {
            free(probeFan);
        }
    }
    }

    free(this);
}

// Reimplements 0x403800: AINet::Free (src/Battlesport/ainet.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL AINet::Free() {
    if (this == 0) {
        return;
    }

    AINetNode *node = nodeListHead;
    while (node != 0) {
        AINetNode *const next = node->next;
        node->Free();
        node = next;
    }

    free(this);
}

// Reimplements 0x403870: AINet::FreeAll (src/Battlesport/ainet.cpp)
RECOIL_NOINLINE void RECOIL_CDECL AINet::FreeAll() {
    AINet *aiNet = g_AINetListHead;
    while (aiNet != 0) {
        g_AINetListHead = aiNet->next;
        aiNet->Free();
        aiNet = g_AINetListHead;
    }
}
