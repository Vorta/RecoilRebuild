// Checked-in focused native smoke translation unit, formerly extracted from recoil_state_main_menu_transition.cpp.
// Emits only the main-menu transition OnTryBecomeCurrent smoke.

#include "Battlesport/recoil_state_main_menu_transition.h"

#include <cstdint>
#include <cstring>
#include <new>

#include "Battlesport/hud.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zVideo/zvid.h"

extern "C" int g_RecoilState_MainMenuSkipExitDelay;

extern "C" int(__fastcall *g_zVideo_pfnLockSurfaceState)(
    zVideo_SurfaceStatePartial *surfaceState);
extern "C" int(__fastcall *g_zVideo_pfnUnlockSurfaceState)(
    zVideo_SurfaceStatePartial *surfaceState);

namespace zVideo {
int __fastcall SetHalfResAdjustMode(int mode);
}

namespace HudUi {
void __fastcall SetInvalidateMode(int mode);
}

namespace zSnd {
int GetCDAudioOption();
}

namespace {
int __fastcall TestVideoSurfaceStateNoOp(zVideo_SurfaceStatePartial *surfaceState) {
    (void)surfaceState;
    return 0;
}

zSndPlayHandleSnapshot *NewEmptySnapshot() {
    auto *const snapshot =
        static_cast<zSndPlayHandleSnapshot *>(::operator new(sizeof(zSndPlayHandleSnapshot)));
    auto *const listHead = static_cast<zSndPlayHandleSnapshotItem *>(
        ::operator new(sizeof(zSndPlayHandleSnapshotItem)));
    listHead->next = listHead;
    listHead->prev = listHead;
    snapshot->backendTag = 0;
    snapshot->listHead = listHead;
    snapshot->itemCount = 0;
    return snapshot;
}

struct CodeFunctionPatch {
    void *target;
    unsigned char original[5];
    bool active;
};

bool PatchFunctionJump(void *target, void *replacement, CodeFunctionPatch &patch) {
    DWORD oldProtect = 0;
    if (!VirtualProtect(target, sizeof(patch.original), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }

    patch.target = target;
    std::memcpy(patch.original, target, sizeof(patch.original));

    unsigned char *const bytes = static_cast<unsigned char *>(target);
    bytes[0] = 0xe9;
    const std::intptr_t rel = reinterpret_cast<unsigned char *>(replacement) -
                              (reinterpret_cast<unsigned char *>(target) + 5);
    *reinterpret_cast<std::int32_t *>(bytes + 1) = static_cast<std::int32_t>(rel);

    DWORD ignored = 0;
    VirtualProtect(target, sizeof(patch.original), oldProtect, &ignored);
    FlushInstructionCache(GetCurrentProcess(), target, sizeof(patch.original));
    patch.active = true;
    return true;
}

void RestoreFunctionPatch(CodeFunctionPatch &patch) {
    if (!patch.active) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.target, sizeof(patch.original), PAGE_EXECUTE_READWRITE,
                       &oldProtect)) {
        std::memcpy(patch.target, patch.original, sizeof(patch.original));
        DWORD ignored = 0;
        VirtualProtect(patch.target, sizeof(patch.original), oldProtect, &ignored);
        FlushInstructionCache(GetCurrentProcess(), patch.target, sizeof(patch.original));
    }
    patch.active = false;
}

zSndPlayHandleSnapshot *FakeMainMenuCreateSnapshot() {
    return NewEmptySnapshot();
}

int __fastcall FakeMainMenuSetHalfResAdjustMode(int) {
    return 0;
}

void __fastcall FakeMainMenuSetInvalidateMode(int) {
}

int __fastcall FakeMainMenuSampleSetInitByName(const char *) {
    return 1;
}

int FakeMainMenuGetCDAudioOption() {
    return 0;
}

} // namespace

extern "C" int recoil_state_main_menu_transition_on_try_become_current_smoke(void) {
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const zVideo_BltRectDirectProc oldBltDirect = g_zVideo_pfnBltSwToPrimaryRectDirect;
    const zVideo_SurfaceStatePartial oldSwSurface = g_zVideo_SwSurfaceState;
    const zVideo_SurfaceStatePartial oldPrimarySurface = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial oldDisplaySurface = g_zVideo_DisplayModeSurfaceState;
    auto *const oldLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    auto *const oldUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;
    int *const oldNetworkEnabled = g_zGame_Options_PointerCache.networkEnabled;
    int *const oldCdAudio = g_zGame_Options_PointerCache.cdAudio;
    void *const oldGlobalVolumeScale = g_zSnd_GlobalVolumeScalePtr;
    const zSndSampleSetRegistry oldSampleSetRegistry = g_zSnd_SampleSetRegistry;

    int networkEnabled = 0;
    int cdAudio = 0;
    float globalVolumeScale = 1.0f;
    char dialogSetName[] = "DIALOG";
    zSndSampleSet dialogSet = {};
    dialogSet.setName = dialogSetName;
    g_zVideo_ActiveRendererPath = 0;
    g_zVideo_pfnBltSwToPrimaryRectDirect = nullptr;
    g_zVideo_pfnLockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_pfnUnlockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_DisplayModeSurfaceState = {};
    g_zGame_Options_PointerCache.networkEnabled = &networkEnabled;
    g_zGame_Options_PointerCache.cdAudio = &cdAudio;
    g_zSnd_GlobalVolumeScalePtr = &globalVolumeScale;
    g_zSnd_SampleSetRegistry.clear();
    g_zSnd_SampleSetRegistry.push_back(&dialogSet);

    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;

    CodeFunctionPatch patches[5] = {};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::SetHalfResAdjustMode),
            reinterpret_cast<void *>(&FakeMainMenuSetHalfResAdjustMode),
            patches[0]) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&HudUi::SetInvalidateMode),
            reinterpret_cast<void *>(&FakeMainMenuSetInvalidateMode),
            patches[1]) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zSndPlayHandleSnapshot::CreateFromActiveSamples),
            reinterpret_cast<void *>(&FakeMainMenuCreateSnapshot),
            patches[2]) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zSndSampleSet_InitByName),
            reinterpret_cast<void *>(&FakeMainMenuSampleSetInitByName),
            patches[3]) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zSnd::GetCDAudioOption),
            reinterpret_cast<void *>(&FakeMainMenuGetCDAudioOption),
            patches[4])) {
        for (int i = 4; i >= 0; --i) {
            RestoreFunctionPatch(patches[i]);
        }
        return 5;
    }

    RecoilStateMainMenuTransition state{};
    state.m_entryRoute = RECOIL_MAINMENU_ROUTE_FRONTEND;

    const int result = state.OnTryBecomeCurrent();
    int failure = 0;
    if (result != 1) {
        failure = 1;
    }
    if (failure == 0 && g_RecoilState_MainMenuSkipExitDelay != 0) {
        failure = 2;
    }
    if (failure == 0 && state.m_pausedAudioSnapshot == 0) {
        failure = 3;
    }
    if (failure == 0 && state.m_mainMenuDialog == 0) {
        failure = 4;
    }

    state.m_pausedAudioSnapshot = 0;
    state.m_mainMenuDialog = 0;
    for (int i = 4; i >= 0; --i) {
        RestoreFunctionPatch(patches[i]);
    }
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVideo_pfnBltSwToPrimaryRectDirect = oldBltDirect;
    g_zVideo_pfnLockSurfaceState = oldLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = oldUnlockSurfaceState;
    g_zVideo_SwSurfaceState = oldSwSurface;
    g_zVideo_PrimarySurfaceState = oldPrimarySurface;
    g_zVideo_DisplayModeSurfaceState = oldDisplaySurface;
    g_zGame_Options_PointerCache.networkEnabled = oldNetworkEnabled;
    g_zGame_Options_PointerCache.cdAudio = oldCdAudio;
    g_zSnd_GlobalVolumeScalePtr = oldGlobalVolumeScale;
    g_zSnd_SampleSetRegistry = oldSampleSetRegistry;
    return failure;
}
