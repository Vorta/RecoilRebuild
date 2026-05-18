#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include "GameZRecoil/zReader/zReader.h"
#include "recoil/recoil_callconv.h"
#include "zClass.h"

struct zTurret_Runtime {
    int flags;
    int scenePathVisible;
    zClass_NodePartial *turretNode;
    zClass_NodePartial *healthyNode;
    unsigned char unknown_10[0x0c];
    zVec3 firePos;
    unsigned char unknown_28[0xdc];
    void *trailRuntimeState;
    unsigned char unknown_108[0x54];
    float healthCurrent;
    float healthMax;

    RECOIL_NOINLINE int RECOIL_THISCALL Shutdown();
    RECOIL_NOINLINE int RECOIL_THISCALL HasActiveNode();
};

RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, flags) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, scenePathVisible) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, turretNode) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, healthyNode) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, firePos) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, trailRuntimeState) == 0x104);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, healthCurrent) == 0x15c);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, healthMax) == 0x160);

extern "C" {
extern zClass_NodePartial *g_zTurret_CallbackNode;
extern zReader::Node *g_zTurret_LoadedDefRoot;
extern int g_zTurret_RuntimeCount;
extern int g_zTurret_CallbackStartIndex;
extern zTurret_Runtime *g_zTurret_RuntimeList[64];
}

namespace zTurret_System {
RECOIL_NOINLINE int RECOIL_CDECL ResetIterationState();
RECOIL_NOINLINE int RECOIL_CDECL DisableTickCallback();
RECOIL_NOINLINE int RECOIL_CDECL Shutdown();
RECOIL_NOINLINE int RECOIL_CDECL FreeAllRuntimes();
} // namespace zTurret_System
