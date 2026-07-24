// Checked-in focused native smoke translation unit, formerly extracted from zhud_ui_tests.cpp.
// Emits the background video widget draw/rebuild smokes from the non-lifecycle zHud UI test branch.

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

int g_testBlitCount = 0;
zVidImagePartial *g_testBlitImages[8] = {};
std::int32_t g_testBlitX[8] = {};
std::int32_t g_testBlitY[8] = {};
std::int32_t g_testBlitFlags[8] = {};
zVidRect32 g_testBlitRects[8] = {};
std::int32_t g_testBlitHasRect[8] = {};

void __fastcall TestBltSourceToPrimary(zVidImagePartial *self, std::int32_t dstX,
                                            std::int32_t dstY, std::int32_t clipFlags,
                                            zVidRect32 *srcRect) {
    const int index = g_testBlitCount;
    if (index < 8) {
        g_testBlitImages[index] = static_cast<zVidImagePartial *>(self);
        g_testBlitX[index] = dstX;
        g_testBlitY[index] = dstY;
        g_testBlitFlags[index] = clipFlags;
        g_testBlitHasRect[index] = srcRect != nullptr ? 1 : 0;
        if (srcRect != nullptr) {
            g_testBlitRects[index] = *static_cast<zVidRect32 *>(srcRect);
        }
    }
    ++g_testBlitCount;
}


int g_backgroundVideoDrawBaseCount = 0;
void *g_backgroundVideoDrawBaseThis = nullptr;

struct TestBackgroundVideoDrawWidget : HudUiBackgroundVideoWidget {
    void DrawBase();
};

void TestBackgroundVideoDrawWidget::DrawBase() {
    ++g_backgroundVideoDrawBaseCount;
    g_backgroundVideoDrawBaseThis = this;
}

int g_backgroundVideoRebuildGetXCount = 0;
int g_backgroundVideoRebuildGetYCount = 0;
int g_backgroundVideoRebuildSetClipCount = 0;
int g_backgroundVideoRebuildGetXValue = 0;
int g_backgroundVideoRebuildGetYValue = 0;
void *g_backgroundVideoRebuildSetClipThis = nullptr;
HudUiRect g_backgroundVideoRebuildSetClipRect = {};

struct TestBackgroundVideoRebuildWidget : HudUiBackgroundVideoWidget {
    int GetCenterX();
    int GetCenterY();
    void SetClipRect(const HudUiRect *rect);
};

int TestBackgroundVideoRebuildWidget::GetCenterX() {
    ++g_backgroundVideoRebuildGetXCount;
    return g_backgroundVideoRebuildGetXValue;
}

int TestBackgroundVideoRebuildWidget::GetCenterY() {
    ++g_backgroundVideoRebuildGetYCount;
    return g_backgroundVideoRebuildGetYValue;
}

void TestBackgroundVideoRebuildWidget::SetClipRect(const HudUiRect *rect) {
    ++g_backgroundVideoRebuildSetClipCount;
    g_backgroundVideoRebuildSetClipThis = this;
    g_backgroundVideoRebuildSetClipRect = *rect;
}

void ResetBackgroundVideoRebuildProbe(int x, int y) {
    g_backgroundVideoRebuildGetXCount = 0;
    g_backgroundVideoRebuildGetYCount = 0;
    g_backgroundVideoRebuildSetClipCount = 0;
    g_backgroundVideoRebuildGetXValue = x;
    g_backgroundVideoRebuildGetYValue = y;
    g_backgroundVideoRebuildSetClipThis = nullptr;
    g_backgroundVideoRebuildSetClipRect = {};
}

} // namespace


extern "C" int zhud_background_video_widget_draw_smoke(void) {
    zVideo_BltSourceToPrimaryProc oldBlit = g_zVideo_pfnBltSourceToPrimary;

    unsigned char streamStorage[0x100] = {};
    zFMV_Stream *const stream = reinterpret_cast<zFMV_Stream *>(streamStorage);
    TestBackgroundVideoDrawWidget widget{};
    widget.x = 17;
    widget.y = 23;
    widget.colorKey565 = 0x07e0;

    g_testBlitCount = 0;
    g_backgroundVideoDrawBaseCount = 0;
    g_backgroundVideoDrawBaseThis = nullptr;
    g_zVideo_pfnBltSourceToPrimary = TestBltSourceToPrimary;
    widget.stream = nullptr;
    widget.Draw();
    const bool nullStream =
        g_backgroundVideoDrawBaseCount == 1 && g_backgroundVideoDrawBaseThis == &widget &&
        g_testBlitCount == 0;

    g_testBlitCount = 0;
    g_backgroundVideoDrawBaseCount = 0;
    g_backgroundVideoDrawBaseThis = nullptr;
    widget.stream = stream;
    widget.Draw();
    const bool streamBlitted =
        g_backgroundVideoDrawBaseCount == 1 && g_backgroundVideoDrawBaseThis == &widget &&
        g_testBlitCount == 1 &&
        g_testBlitImages[0] == reinterpret_cast<zVidImagePartial *>(stream) &&
        g_testBlitX[0] == 17 && g_testBlitY[0] == 23 &&
        g_testBlitFlags[0] == 0x07e0 && g_testBlitHasRect[0] == 0;

    widget.stream = nullptr;
    g_zVideo_pfnBltSourceToPrimary = oldBlit;
    return nullStream && streamBlitted ? 0 : 1;
}

extern "C" int zhud_background_video_widget_draw_base_smoke(void) {
    zVideo_BltSourceToPrimaryProc oldBlit = g_zVideo_pfnBltSourceToPrimary;

    zVidImagePartial image{};
    HudUiBackgroundVideoWidget widget{};
    widget.x = -4;
    widget.y = -9;
    widget.clipRect = {3, 4, 11, 13};

    g_testBlitCount = 0;
    g_zVideo_pfnBltSourceToPrimary = TestBltSourceToPrimary;
    widget.bltSource = nullptr;
    widget.DrawBase();
    const bool nullSkipped = g_testBlitCount == 0;

    widget.bltSource = &image;
    widget.DrawBase();
    const bool negativeClamped =
        g_testBlitCount == 1 && g_testBlitImages[0] == &image && g_testBlitX[0] == 0 &&
        g_testBlitY[0] == 0 && g_testBlitFlags[0] == 0 && g_testBlitHasRect[0] == 1 &&
        g_testBlitRects[0].left == 3 && g_testBlitRects[0].top == 4 &&
        g_testBlitRects[0].right == 11 && g_testBlitRects[0].bottom == 13;

    widget.x = 17;
    widget.y = 23;
    g_testBlitCount = 0;
    widget.DrawBase();
    const bool positivePassed =
        g_testBlitCount == 1 && g_testBlitImages[0] == &image && g_testBlitX[0] == 17 &&
        g_testBlitY[0] == 23 && g_testBlitFlags[0] == 0 && g_testBlitHasRect[0] == 1;

    g_zVideo_pfnBltSourceToPrimary = oldBlit;
    return nullSkipped && negativeClamped && positivePassed ? 0 : 1;
}

extern "C" int zhud_background_video_widget_rebuild_blt_rect_smoke(void) {
    zVidImagePartial source{};
    source.width = 25;
    source.height = 26;
    unsigned char streamStorage[0x20] = {};
    zFMV_Stream *const stream = reinterpret_cast<zFMV_Stream *>(streamStorage);
    stream->width = 30;
    stream->height = 12;

    TestBackgroundVideoRebuildWidget widget{};

    ResetBackgroundVideoRebuildProbe(5, 6);
    widget.stream = nullptr;
    widget.RebuildBltRect();
    const bool streamNullSkipped =
        g_backgroundVideoRebuildGetXCount == 2 && g_backgroundVideoRebuildGetYCount == 2 &&
        g_backgroundVideoRebuildSetClipCount == 0;

    ResetBackgroundVideoRebuildProbe(-4, 20);
    widget.stream = stream;
    widget.bltSource = nullptr;
    widget.RebuildBltRect();
    const bool noSource =
        g_backgroundVideoRebuildSetClipCount == 1 &&
        g_backgroundVideoRebuildSetClipThis == &widget &&
        g_backgroundVideoRebuildSetClipRect.left == 0 &&
        g_backgroundVideoRebuildSetClipRect.top == 20 &&
        g_backgroundVideoRebuildSetClipRect.right == 30 &&
        g_backgroundVideoRebuildSetClipRect.bottom == 32;

    ResetBackgroundVideoRebuildProbe(10, 20);
    widget.bltSource = &source;
    widget.RebuildBltRect();
    const bool sourceClamped =
        g_backgroundVideoRebuildSetClipCount == 1 &&
        g_backgroundVideoRebuildSetClipThis == &widget &&
        g_backgroundVideoRebuildSetClipRect.left == 10 &&
        g_backgroundVideoRebuildSetClipRect.top == 20 &&
        g_backgroundVideoRebuildSetClipRect.right == 25 &&
        g_backgroundVideoRebuildSetClipRect.bottom == 26;

    widget.stream = nullptr;
    return streamNullSkipped && noSource && sourceClamped ? 0 : 1;
}
