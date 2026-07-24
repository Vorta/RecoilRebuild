// Checked-in focused native smoke translation unit, formerly extracted from zhud_ui_tests.cpp.
// Emits the background video widget update smoke from the non-lifecycle zHud UI test branch.

#include "Battlesport/game_net.h"
#include "Battlesport/briefing.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/hud_ui_mp_exit_dialog.h"
#include "Battlesport/hud_ui_net_game_setup.h"
#include "Battlesport/hud_ui_net_exit_panel.h"
#include "Battlesport/hud.h"
#include "Battlesport/pickup.h"
#include "GameZRecoil/Time/time.h"
#include "Battlesport/mission.h"
#include "Battlesport/recoil_state_main_menu_transition.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zClass/cls_stubs.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/include/zclip_alt.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "Battlesport/turret.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mmsystem.h>
#include <new>

extern "C" std::uint32_t g_HudUi_InvalidateMask;
extern "C" int g_Hud_MapOverlayRefCount;
extern "C" int g_HudSensorTracker_ObjectiveCommandLocked;
extern "C" float g_HudLineClip_CurrentLeft;
extern "C" float g_HudLineClip_CurrentTop;
extern "C" float g_HudLineClip_CurrentRight;
extern "C" float g_HudLineClip_CurrentBottom;
extern "C" zVec3 g_HudSensor_ClipSegmentStart;
extern "C" zVec3 g_HudSensor_ClipSegmentEnd;
extern "C" HWND g_RecoilApp_hWndMain;
extern float g_zMath_ClipZLowerBound;
extern float g_zMath_ClipZUpperBound;
extern zFMV_Playback *g_HudUiSensorWindowPlayback;

namespace {
template <typename T> T &TestFieldAt(void *base, std::size_t offset) {
    return *reinterpret_cast<T *>(static_cast<std::uint8_t *>(base) + offset);
}

static std::uintptr_t *TestVTable(void *object) {
    return *reinterpret_cast<std::uintptr_t **>(object);
}

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
} // namespace

int g_backgroundVideoReadCount = 0;
zFMV_Stream *g_backgroundVideoLastStream = nullptr;
unsigned int g_backgroundVideoLastFrame = 0;

int __fastcall TestBackgroundVideoReadAndDecodeFrame(zFMV_Stream *stream, void *,
                                                          unsigned int frame) {
    ++g_backgroundVideoReadCount;
    g_backgroundVideoLastStream = stream;
    g_backgroundVideoLastFrame = frame;
    return 1;
}

void ResetBackgroundVideoUpdateProbe() {
    g_backgroundVideoReadCount = 0;
    g_backgroundVideoLastStream = nullptr;
    g_backgroundVideoLastFrame = 0;
}

extern "C" int zhud_background_video_widget_update_smoke(void) {
    CodeFunctionPatch readPatch{};
    const std::uintptr_t readAddress = MethodAddress(&zFMV_Stream::ReadAndDecodeFrame);
    if (!PatchFunctionJump(reinterpret_cast<void *>(readAddress),
                           reinterpret_cast<void *>(&TestBackgroundVideoReadAndDecodeFrame),
                           readPatch)) {
        return 1;
    }

    unsigned char streamStorage[0x100] = {};
    zFMV_Stream *const stream = reinterpret_cast<zFMV_Stream *>(streamStorage);
    TestFieldAt<int>(stream, 0x4c) = 10;
    TestFieldAt<int>(stream, 0xec) = 12;

    HudUiBackgroundVideoWidget widget{};
    widget.stream = stream;
    widget.flags = 0x10;
    widget.elapsedTimeSec = 3.0f;
    ResetBackgroundVideoUpdateProbe();
    widget.Update(0.25f);
    const bool disabledSkipped =
        g_backgroundVideoReadCount == 0 && widget.elapsedTimeSec == 3.0f;

    widget.stream = nullptr;
    widget.flags = 0x02;
    widget.elapsedTimeSec = 1.0f;
    ResetBackgroundVideoUpdateProbe();
    widget.Update(0.25f);
    const bool noStreamUpdatesBase =
        g_backgroundVideoReadCount == 0 && widget.elapsedTimeSec == 1.25f;

    widget.stream = stream;
    widget.flags = 0x02;
    widget.elapsedTimeSec = 1.25f;
    ResetBackgroundVideoUpdateProbe();
    widget.Update(0.5f);
    const bool streamDecodesFrame =
        g_backgroundVideoReadCount == 1 && g_backgroundVideoLastStream == stream &&
        g_backgroundVideoLastFrame == 5 && widget.elapsedTimeSec == 1.75f;

    widget.stream = nullptr;
    RestoreFunctionPatch(readPatch);
    return disabledSkipped && noStreamUpdatesBase && streamDecodesFrame ? 0 : 2;
}

int g_backgroundVideoDrawBaseCount = 0;
void *g_backgroundVideoDrawBaseThis = nullptr;

struct TestBackgroundVideoDrawDispatch {
    void DrawBase() {
        ++g_backgroundVideoDrawBaseCount;
        g_backgroundVideoDrawBaseThis = this;
    }
};
