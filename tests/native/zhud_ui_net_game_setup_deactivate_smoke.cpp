// Checked-in focused native smoke translation unit, formerly extracted from zhud_ui_tests.cpp.
// Emits the net-game setup overlay queue-enter, lifecycle, and deactivate smokes.

#include "Battlesport/recoil_app.h"
#include "Battlesport/game_net.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/hud.h"
#include "Battlesport/player.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/include/zclip_alt.h"
#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "Battlesport/turret.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>

extern "C" std::uint32_t g_HudUi_InvalidateMask;
extern "C" float g_HudLineClip_CurrentLeft;
extern "C" float g_HudLineClip_CurrentTop;
extern "C" float g_HudLineClip_CurrentRight;
extern "C" float g_HudLineClip_CurrentBottom;
extern "C" zVec3 g_HudSensor_ClipSegmentStart;
extern zFMV_Playback *g_HudUiSensorWindowPlayback;

namespace {
template <typename Method> std::uintptr_t MethodAddress(Method method) {
    static_assert(sizeof(method) <= sizeof(std::uintptr_t));
    std::uintptr_t address = 0;
    std::memcpy(&address, &method, sizeof(method));
    return address;
}

template <typename Slot, typename Method> void AssignMethodSlot(Slot &slot, Method method) {
    static_assert(sizeof(slot) == sizeof(method));
    std::memcpy(&slot, &method, sizeof(slot));
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

    FlushInstructionCache(GetCurrentProcess(), target, sizeof(patch.original));
    DWORD ignored = 0;
    VirtualProtect(target, sizeof(patch.original), oldProtect, &ignored);
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
        FlushInstructionCache(GetCurrentProcess(), patch.target, sizeof(patch.original));
        DWORD ignored = 0;
        VirtualProtect(patch.target, sizeof(patch.original), oldProtect, &ignored);
    }
    patch.active = false;
}

RecoilApp_IState *g_netGameSetupQueuedState;
int g_netGameSetupQueuedParam;
int g_netGameSetupQueueCallCount;
int g_netGameSetupOverlaySetEnabledCalls;
int g_netGameSetupOverlaySetEnabledValue;
int g_netGameSetupOverlayScalarCalls;
unsigned int g_netGameSetupOverlayScalarFlags;
DWORD g_netGameSetupDeactivateSleepMs;
int g_netGameSetupDeactivateSampleCalls;
const char *g_netGameSetupDeactivateSampleName;
int g_netGameSetupDeactivatePostprocessCalls;
HudUiDialogController *g_netGameSetupDeactivateBlitThis;
int g_netGameSetupDeactivateBlitCalls;
int g_netGameSetupDeactivateUnlockCalls;

struct NetGameSetupQueuePatchOps {
    RecoilPtr32 QueuePushState(RecoilApp_IState *state, int suspendParam) {
        ++g_netGameSetupQueueCallCount;
        g_netGameSetupQueuedState = state;
        g_netGameSetupQueuedParam = suspendParam;
        return 0x12345678;
    }
};

struct TestNetGameSetupOverlayPanel {
    virtual void Update(float) {}

    virtual void SetEnabled(int enabled) {
        ++g_netGameSetupOverlaySetEnabledCalls;
        g_netGameSetupOverlaySetEnabledValue = enabled;
    }

    virtual TestNetGameSetupOverlayPanel * ScalarDeletingDestructor(unsigned int flags) {
        ++g_netGameSetupOverlayScalarCalls;
        g_netGameSetupOverlayScalarFlags = flags;
        return this;
    }
};

void ResetNetGameSetupOverlayOwnerProbe() {
    g_netGameSetupOverlaySetEnabledCalls = 0;
    g_netGameSetupOverlaySetEnabledValue = -1;
    g_netGameSetupOverlayScalarCalls = 0;
    g_netGameSetupOverlayScalarFlags = 0;
}

template <typename T> unsigned int SmokeVPtr32(const T &object) {
    return static_cast<unsigned int>(
        reinterpret_cast<std::uintptr_t>(*reinterpret_cast<void *const *>(&object)));
}

template <typename T> void SmokeSetVPtr32(T &object, unsigned int value) {
    *reinterpret_cast<unsigned int *>(&object) = value;
}

unsigned int SmokeExpectedOverlayOwnerVPtr32() {
    HudUiNetGameSetupOverlayOwner expected;
    return SmokeVPtr32(expected);
}

unsigned int SmokeExpectedBaseStateVPtr32() {
    RecoilApp_IState expected;
    return SmokeVPtr32(expected);
}

template <typename T> T *SmokeScalarDeletingDestructor(T *object, unsigned int flags) {
    object->~T();
    if ((flags & 1) != 0) {
        ::operator delete(object);
    }
    return object;
}

int g_netGameSetupAtexitCalls;
void(__cdecl *g_netGameSetupAtexitCallback)(void);

void ResetNetGameSetupAtexitProbe() {
    g_netGameSetupAtexitCalls = 0;
    g_netGameSetupAtexitCallback = nullptr;
}

int __cdecl FakeNetGameSetupAtexit(void(__cdecl *callback)(void)) {
    ++g_netGameSetupAtexitCalls;
    g_netGameSetupAtexitCallback = callback;
    return 0;
}

void ResetNetGameSetupDeactivateProbe() {
    g_netGameSetupDeactivateSleepMs = 0;
    g_netGameSetupDeactivateSampleCalls = 0;
    g_netGameSetupDeactivateSampleName = nullptr;
    g_netGameSetupDeactivatePostprocessCalls = 0;
    g_netGameSetupDeactivateBlitThis = nullptr;
    g_netGameSetupDeactivateBlitCalls = 0;
    g_netGameSetupDeactivateUnlockCalls = 0;
}

void WINAPI FakeNetGameSetupDeactivateSleep(DWORD milliseconds) {
    g_netGameSetupDeactivateSleepMs = milliseconds;
}

int __fastcall FakeNetGameSetupDeactivateSampleSetDestroyByName(const char *setName) {
    ++g_netGameSetupDeactivateSampleCalls;
    g_netGameSetupDeactivateSampleName = setName;
    return 1;
}

int FakeNetGameSetupDeactivateRunPostprocessOnPrimaryBuffer() {
    ++g_netGameSetupDeactivatePostprocessCalls;
    return 1;
}

int FakeNetGameSetupDeactivateUnlockPrimarySurfaceState() {
    ++g_netGameSetupDeactivateUnlockCalls;
    return 1;
}

struct NetGameSetupDeactivatePatchOps {
    void BlitOwnedSurfaceToPrimary() {
        ++g_netGameSetupDeactivateBlitCalls;
        g_netGameSetupDeactivateBlitThis = (HudUiDialogController *)this;
    }
};
} // namespace

extern "C" int hud_ui_net_game_setup_queue_enter_reconfigure_smoke(void) {
    CodeFunctionPatch queuePatch{};
    const bool installed =
        PatchFunctionJump(reinterpret_cast<void *>(MethodAddress(&RecoilApp::QueuePushState)),
                          reinterpret_cast<void *>(MethodAddress(&NetGameSetupQueuePatchOps::QueuePushState)),
                          queuePatch);

    g_HudUiNetGameSetupOverlayOwner.m_reconfigureExistingSession = 0;
    g_netGameSetupQueuedState = nullptr;
    g_netGameSetupQueuedParam = -1;
    g_netGameSetupQueueCallCount = 0;

    if (installed) {
        HudUiNetGameSetupOverlayOwner::QueueEnterWithReconfigureFlag(1);
    }

    const bool ok =
        installed && g_HudUiNetGameSetupOverlayOwner.m_reconfigureExistingSession == 1 &&
        g_netGameSetupQueueCallCount == 1 &&
        g_netGameSetupQueuedState ==
            reinterpret_cast<RecoilApp_IState *>(&g_HudUiNetGameSetupOverlayOwner) &&
        g_netGameSetupQueuedParam == 0;

    RestoreFunctionPatch(queuePatch);
    return ok ? 0 : 1;
}

extern "C" int hud_ui_net_game_setup_overlay_owner_lifecycle_smoke(void) {
    HudUiNetGameSetupOverlayOwner state{};
    (void)0;
    state.m_dialog = reinterpret_cast<HudUiContainer *>(0x22222222);
    state.m_reconfigureExistingSession = 7;
    HudUiNetGameSetupOverlayOwner *const constructed = new (&state) HudUiNetGameSetupOverlayOwner;
    const bool constructorOk =
        constructed == &state &&
        SmokeVPtr32(state) == SmokeExpectedOverlayOwnerVPtr32() &&
        state.m_dialog == 0 && state.m_reconfigureExistingSession == 0;

    (void)0;
    g_HudUiNetGameSetupOverlayOwner.m_dialog = reinterpret_cast<HudUiContainer *>(0x44444444);
    g_HudUiNetGameSetupOverlayOwner.m_reconfigureExistingSession = 9;
    HudUiNetGameSetupOverlayOwner *const staticReturned =
        HudUiNetGameSetupOverlayOwner::StaticInit();
    const bool staticInitOk =
        staticReturned == &g_HudUiNetGameSetupOverlayOwner &&
        SmokeVPtr32(g_HudUiNetGameSetupOverlayOwner) == SmokeExpectedOverlayOwnerVPtr32() &&
        g_HudUiNetGameSetupOverlayOwner.m_dialog == 0 &&
        g_HudUiNetGameSetupOverlayOwner.m_reconfigureExistingSession == 0;

    TestNetGameSetupOverlayPanel panel;
    (void)0;
    state.m_dialog = reinterpret_cast<HudUiContainer *>(&panel);
    state.m_reconfigureExistingSession = 4;
    ResetNetGameSetupOverlayOwnerProbe();
    state.~HudUiNetGameSetupOverlayOwner();
    const bool destructorOk =
        SmokeVPtr32(state) == SmokeExpectedBaseStateVPtr32() && state.m_dialog == 0 &&
        state.m_reconfigureExistingSession == 4 &&
        g_netGameSetupOverlaySetEnabledCalls == 1 &&
        g_netGameSetupOverlaySetEnabledValue == 0 &&
        g_netGameSetupOverlayScalarCalls == 1 &&
        g_netGameSetupOverlayScalarFlags == 1;

    (void)0;
    g_HudUiNetGameSetupOverlayOwner.m_dialog = reinterpret_cast<HudUiContainer *>(&panel);
    ResetNetGameSetupOverlayOwnerProbe();
    HudUiNetGameSetupOverlayOwner::AtExitDestructor();
    const bool atExitOk =
        SmokeVPtr32(g_HudUiNetGameSetupOverlayOwner) == SmokeExpectedBaseStateVPtr32() &&
        g_HudUiNetGameSetupOverlayOwner.m_dialog == 0 &&
        g_netGameSetupOverlaySetEnabledCalls == 1 &&
        g_netGameSetupOverlayScalarCalls == 1;

    HudUiNetGameSetupOverlayOwner scalarState{};
    (void)0;
    scalarState.m_dialog = 0;
    HudUiNetGameSetupOverlayOwner *const scalarReturned =
        SmokeScalarDeletingDestructor(&scalarState, 0);
    const bool scalarOk =
        scalarReturned == &scalarState &&
        SmokeVPtr32(scalarState) == SmokeExpectedBaseStateVPtr32() &&
        scalarState.m_dialog == 0;

    HudUiNetGameSetupOverlayOwner *const deletingState =
        new HudUiNetGameSetupOverlayOwner;
    HudUiNetGameSetupOverlayOwner *const deletingReturned =
        SmokeScalarDeletingDestructor(deletingState, 1);
    const bool deletingOk = deletingReturned == deletingState;

    CodeFunctionPatch atexitPatch{};
    const bool atexitInstalled =
        PatchFunctionJump(reinterpret_cast<void *>(&atexit),
                          reinterpret_cast<void *>(&FakeNetGameSetupAtexit),
                          atexitPatch);
    ResetNetGameSetupAtexitProbe();
    if (atexitInstalled) {
        HudUiNetGameSetupOverlayOwner::RegisterAtExit();
    }
    const bool registerAtExitOk =
        atexitInstalled && g_netGameSetupAtexitCalls == 1 &&
        g_netGameSetupAtexitCallback == &HudUiNetGameSetupOverlayOwner::AtExitDestructor;

    (void)0;
    g_HudUiNetGameSetupOverlayOwner.m_dialog = 0;
    g_HudUiNetGameSetupOverlayOwner.m_reconfigureExistingSession = 3;
    ResetNetGameSetupAtexitProbe();
    if (atexitInstalled) {
        HudUiNetGameSetupOverlayOwner::StaticInitAndRegisterAtExit();
    }
    const bool staticInitRegisterOk =
        SmokeVPtr32(g_HudUiNetGameSetupOverlayOwner) == SmokeExpectedOverlayOwnerVPtr32() &&
        g_HudUiNetGameSetupOverlayOwner.m_dialog == 0 &&
        g_HudUiNetGameSetupOverlayOwner.m_reconfigureExistingSession == 0 &&
        g_netGameSetupAtexitCalls == 1 &&
        g_netGameSetupAtexitCallback == &HudUiNetGameSetupOverlayOwner::AtExitDestructor;

    RestoreFunctionPatch(atexitPatch);

    return constructorOk && staticInitOk && destructorOk && atExitOk &&
                   scalarOk && deletingOk && registerAtExitOk &&
                   staticInitRegisterOk
               ? 0
               : 1;
}

extern "C" int hud_ui_net_game_setup_overlay_owner_on_deactivate_smoke(void) {
    CodeFunctionPatch patches[5] = {};
    bool installed =
        PatchFunctionJump(reinterpret_cast<void *>(&Sleep),
                          reinterpret_cast<void *>(&FakeNetGameSetupDeactivateSleep),
                          patches[0]) &&
        PatchFunctionJump(reinterpret_cast<void *>(&zSndSampleSet_DestroyByName),
                          reinterpret_cast<void *>(&FakeNetGameSetupDeactivateSampleSetDestroyByName),
                          patches[1]) &&
        PatchFunctionJump(reinterpret_cast<void *>(&zVideo::RunPostprocessOnPrimaryBuffer),
                          reinterpret_cast<void *>(&FakeNetGameSetupDeactivateRunPostprocessOnPrimaryBuffer),
                          patches[2]) &&
        PatchFunctionJump(reinterpret_cast<void *>(MethodAddress(&HudUiDialogController::BlitOwnedSurfaceToPrimary)),
                          reinterpret_cast<void *>(MethodAddress(&NetGameSetupDeactivatePatchOps::BlitOwnedSurfaceToPrimary)),
                          patches[3]) &&
        PatchFunctionJump(reinterpret_cast<void *>(&zVideo::Dispatch_UnlockPrimarySurfaceState),
                          reinterpret_cast<void *>(&FakeNetGameSetupDeactivateUnlockPrimarySurfaceState),
                          patches[4]);

    HudUiNetGameSetupOverlayOwner nullState{};
    nullState.m_dialog = 0;
    ResetNetGameSetupOverlayOwnerProbe();
    ResetNetGameSetupDeactivateProbe();
    if (installed) {
        nullState.OnDeactivate();
    }
    const bool nullPanelOk =
        installed && nullState.m_dialog == 0 &&
        g_netGameSetupDeactivateSleepMs == 1000 &&
        g_netGameSetupDeactivateSampleCalls == 1 &&
        std::strcmp(g_netGameSetupDeactivateSampleName, "DIALOG") == 0 &&
        g_netGameSetupDeactivatePostprocessCalls == 0 &&
        g_netGameSetupOverlaySetEnabledCalls == 0 &&
        g_netGameSetupDeactivateBlitCalls == 0 &&
        g_netGameSetupDeactivateUnlockCalls == 0 &&
        g_netGameSetupOverlayScalarCalls == 0;

    HudUiNetGameSetupOverlayOwner state{};
    TestNetGameSetupOverlayPanel panel;
    state.m_dialog = reinterpret_cast<HudUiContainer *>(&panel);
    ResetNetGameSetupOverlayOwnerProbe();
    ResetNetGameSetupDeactivateProbe();
    if (installed) {
        state.OnDeactivate();
    }
    const bool ownedPanelOk =
        installed && state.m_dialog == 0 &&
        g_netGameSetupDeactivateSleepMs == 1000 &&
        g_netGameSetupDeactivateSampleCalls == 1 &&
        std::strcmp(g_netGameSetupDeactivateSampleName, "DIALOG") == 0 &&
        g_netGameSetupDeactivatePostprocessCalls == 1 &&
        g_netGameSetupOverlaySetEnabledCalls == 1 &&
        g_netGameSetupOverlaySetEnabledValue == 0 &&
        g_netGameSetupDeactivateBlitCalls == 1 &&
        g_netGameSetupDeactivateBlitThis == (HudUiDialogController *)&panel &&
        g_netGameSetupDeactivateUnlockCalls == 1 &&
        g_netGameSetupOverlayScalarCalls == 1 &&
        g_netGameSetupOverlayScalarFlags == 1;

    for (int i = 4; i >= 0; --i) {
        RestoreFunctionPatch(patches[i]);
    }

    if (!nullPanelOk) {
        return 2;
    }
    if (!ownedPanelOk) {
        return 3;
    }

    return 0;
}
