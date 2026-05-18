#include "GameZRecoil/zTurret/zTurret.h"

#include <cstdlib>
#include <new>

extern "C" int zturret_reset_iteration_state_smoke(void) {
    g_zTurret_RuntimeCount = 17;
    g_zTurret_CallbackStartIndex = 9;

    return zTurret_System::ResetIterationState() == 0 && g_zTurret_RuntimeCount == 0 &&
                   g_zTurret_CallbackStartIndex == 0
               ? 0
               : 1;
}

extern "C" int zturret_shutdown_leaf_smoke(void) {
    auto *runtime = static_cast<zTurret_Runtime *>(::operator new(sizeof(zTurret_Runtime)));
    *runtime = {};
    runtime->trailRuntimeState = std::malloc(8);
    g_zTurret_RuntimeList[0] = runtime;
    g_zTurret_RuntimeList[1] = nullptr;
    g_zTurret_RuntimeCount = 1;
    g_zTurret_LoadedDefRoot = nullptr;
    g_zTurret_CallbackNode = nullptr;

    const std::int32_t freeResult = zTurret_System::FreeAllRuntimes();
    if (freeResult != 0 || g_zTurret_RuntimeCount != 0 || g_zTurret_RuntimeList[0] != nullptr) {
        return 1;
    }

    runtime = static_cast<zTurret_Runtime *>(::operator new(sizeof(zTurret_Runtime)));
    *runtime = {};
    runtime->trailRuntimeState = std::malloc(8);
    g_zTurret_RuntimeList[0] = runtime;
    g_zTurret_RuntimeCount = 1;

    const std::int32_t shutdownResult = zTurret_System::Shutdown();
    return shutdownResult == 0 && g_zTurret_RuntimeCount == 0 && g_zTurret_RuntimeList[0] == nullptr
               ? 0
               : 2;
}
