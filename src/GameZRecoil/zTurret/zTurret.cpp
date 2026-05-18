#include "GameZRecoil/zTurret/zTurret.h"

#include "OptCatalog.h"


extern "C" {
zClass_NodePartial *g_zTurret_CallbackNode = 0;
zReader::Node *g_zTurret_LoadedDefRoot = 0;
int g_zTurret_RuntimeCount = 0;
int g_zTurret_CallbackStartIndex = 0;
zTurret_Runtime *g_zTurret_RuntimeList[64] = {0};
}

// Reimplements 0x436e00: zTurret_Runtime::Shutdown
RECOIL_NOINLINE int RECOIL_THISCALL zTurret_Runtime::Shutdown() {
    if (trailRuntimeState != 0) {
        OptCatalog::FreeTrailRuntimeStateStorage(trailRuntimeState);
    }

    return zClass_Node::ClearDamageHandler(healthyNode);
}

// Reimplements 0x436e20: zTurret_Runtime::HasActiveNode
// (D:\Proj\Battlesport\turret.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL zTurret_Runtime::HasActiveNode() {
    if (flags != 0 && (turretNode->flags & 0x04) != 0) {
        return 1;
    }

    return 0;
}

namespace zTurret_System {
// Reimplements 0x437aa0: zTurret_System::ResetIterationState
RECOIL_NOINLINE int RECOIL_CDECL ResetIterationState() {
    g_zTurret_RuntimeCount = 0;
    g_zTurret_CallbackStartIndex = 0;
    return 0;
}

// Reimplements 0x437d40: zTurret_System::DisableTickCallback
// (D:\Proj\Battlesport\turret.cpp)
RECOIL_NOINLINE int RECOIL_CDECL DisableTickCallback() {
    return zClass_Class::gwNodeSetActionCallback(g_zTurret_CallbackNode, 0);
}

// Reimplements 0x437dc0: zTurret_System::FreeAllRuntimes
RECOIL_NOINLINE int RECOIL_CDECL FreeAllRuntimes() {
    for (int i = 0; i < g_zTurret_RuntimeCount; ++i) {
        zTurret_Runtime *const runtime = g_zTurret_RuntimeList[i];
        runtime->Shutdown();
        ::operator delete(runtime);
        g_zTurret_RuntimeList[i] = 0;
    }

    g_zTurret_RuntimeCount = 0;
    if (g_zTurret_LoadedDefRoot != 0) {
        zReader::FreeLoadedTree(g_zTurret_LoadedDefRoot);
        g_zTurret_LoadedDefRoot = 0;
    }

    if (g_zTurret_CallbackNode != 0) {
        zClass_Class::gwNodeSetActionCallback(g_zTurret_CallbackNode, 0);
        zClass_Object3D::DeleteNode(g_zTurret_CallbackNode);
        g_zTurret_CallbackNode = 0;
    }

    return 0;
}

// Reimplements 0x437ab0: zTurret_System::Shutdown
RECOIL_NOINLINE int RECOIL_CDECL Shutdown() {
    FreeAllRuntimes();
    return 0;
}
} // namespace zTurret_System
