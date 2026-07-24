// Checked-in focused native smoke translation unit, formerly extracted from zhud_ui_tests.cpp.
// Emits the message-box smokes from the non-lifecycle zHud UI test branch.

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
} // namespace

extern "C" int zhud_message_box_constructor_fallback_smoke(void) {
    const zVideo_SurfaceStatePartial savedPrimaryState = g_zVideo_PrimarySurfaceState;
    const zVidRect32 savedPrimaryRectScratch = g_zVideo_PrimarySurfaceRectScratch;
    const std::uint32_t savedInvalidateMask = g_HudUi_InvalidateMask;

    g_HudUi_InvalidateMask = 0x80;
    g_zVideo_PrimarySurfaceRectScratch = {11, 22, 33, 44};
    g_zVideo_PrimarySurfaceState.width = 640;
    g_zVideo_PrimarySurfaceState.height = 480;

    alignas(HudUiMessageBoxDialog) std::uint8_t dialogStorage[sizeof(HudUiMessageBoxDialog)]{};
    auto *const dialog = reinterpret_cast<HudUiMessageBoxDialog *>(dialogStorage);
    HudUiMessageBoxDialog *const result = dialog->Constructor(nullptr, nullptr);

    const unsigned short backgroundColor =
        static_cast<unsigned short>(zVid_PackColorRGB(128, 128, 128));
    const unsigned short normalColor =
        static_cast<unsigned short>(zVid_PackColorRGB(192, 192, 192));
    const unsigned short pressedColor =
        static_cast<unsigned short>(zVid_PackColorRGB(160, 192, 160));

    const auto *const dialogFTable =
        TestFieldAt<const void *>(dialog, 0);
    const bool coreFields =
        result == dialog && dialogFTable != nullptr &&
        dialog->blitRect.left == 11 && dialog->blitRect.top == 22 &&
        dialog->blitRect.right == 640 && dialog->blitRect.bottom == 480 &&
        dialog->fallbackWidth == 300 && dialog->fallbackHeight == 200;

    const bool images =
        dialog->backgroundImage != nullptr && dialog->backgroundImage->width == 300 &&
        dialog->backgroundImage->height == 200 &&
        zVid_Image::QueryBytesPerPixel(dialog->backgroundImage) == 2 &&
        static_cast<unsigned short *>(dialog->backgroundImage->pixels)[0] == backgroundColor &&
        dialog->okButtonNormalImage != nullptr && dialog->okButtonNormalImage->width == 75 &&
        dialog->okButtonNormalImage->height == 50 &&
        static_cast<unsigned short *>(dialog->okButtonNormalImage->pixels)[0] == normalColor &&
        dialog->okButtonPressedImage != nullptr && dialog->okButtonPressedImage->width == 75 &&
        dialog->okButtonPressedImage->height == 50 &&
        static_cast<unsigned short *>(dialog->okButtonPressedImage->pixels)[0] == pressedColor;

    const bool bindings =
        dialog->backdropWidget.image == dialog->backgroundImage &&
        dialog->okButton.image == dialog->okButtonNormalImage &&
        dialog->okButton.defaultImage == dialog->okButtonNormalImage &&
        dialog->okButton.rolloverImage == dialog->okButtonPressedImage &&
        dialog->okButton.owner == dialog &&
        dialog->cancelButton.owner == nullptr;

    const bool positions =
        dialog->backdropWidget.x == 170 && dialog->backdropWidget.y == 140 &&
        reinterpret_cast<HudUiElement *>(&dialog->titlePanel)->x == 180 &&
        reinterpret_cast<HudUiElement *>(&dialog->titlePanel)->y == 150 &&
        reinterpret_cast<HudUiElement *>(&dialog->messagePanel)->x == 180 &&
        reinterpret_cast<HudUiElement *>(&dialog->messagePanel)->y == 170 &&
        dialog->okButton.x == 283 && dialog->okButton.y == 280;

    const bool visibleFlags =
        (reinterpret_cast<HudUiElement *>(&dialog->messagePanel)->flags & 0x10u) == 0 &&
        (reinterpret_cast<HudUiElement *>(&dialog->titlePanel)->flags & 0x10u) == 0 &&
        (dialog->okButton.flags & 0x10u) != 0;

    dialog->~HudUiMessageBoxDialog();

    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_PrimarySurfaceRectScratch = savedPrimaryRectScratch;
    g_HudUi_InvalidateMask = savedInvalidateMask;

    return coreFields && images && bindings && positions && visibleFlags ? 0 : 1;
}

int g_messageBoxRunModalUpdateCount = 0;
int g_messageBoxRunModalSetEnabledCount = 0;
int g_messageBoxRunModalLastEnabled = -1;
float g_messageBoxRunModalLastDelta = -1.0f;
zVideo_SurfaceStatePartial *g_messageBoxRunModalUnlockedState = nullptr;

struct TestMessageBoxRunModalElement : HudUiElement {
    void Update(float deltaSeconds) {
        ++g_messageBoxRunModalUpdateCount;
        g_messageBoxRunModalLastDelta = deltaSeconds;
        auto *const dialog = static_cast<HudUiMessageBoxDialog *>(parent);
        dialog->modalResult = 1;
        dialog->modalFrameCountdown = 0;
    }
};

int __fastcall TestMessageBoxRunModalUnlockSurface(zVideo_SurfaceStatePartial *state) {
    g_messageBoxRunModalUnlockedState = state;
    return 0;
}

void __fastcall TestMessageBoxRunModalBltNoOp(zVidRect32 *, zVidRect32 *) {
}

extern "C" int zhud_message_box_run_modal_smoke(void) {
    const int savedRendererPath = g_zVideo_ActiveRendererPath;
    const int savedRendererType = g_zVideo_RendererType;
    const int savedUseHalfRes = g_zVideo_UseHalfResBackbuffer;
    const int savedHalfResMode = g_zVideo_HalfResAdjustMode;
    const int savedAdjustDisableGate = g_zVideo_AdjustSurfacesDisableGate;
    const zVideo_SurfaceStateProc savedUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;
    const zVideo_BltRectDirectProc savedBltPrimaryToSw = g_zVideo_pfnBltPrimaryToSwRectDirect;
    const zVideo_SurfaceStatePartial savedPrimaryState = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;
    const zVidRect32 savedPrimaryRectScratch = g_zVideo_PrimarySurfaceRectScratch;
    const unsigned char savedMouseFlags = g_zInputMouseFlags;
    const short savedMousePollRefCount = g_zInputMousePollRefCount;
    const unsigned char savedJoystickFlags = g_zInputJoystickFlags;
    const short savedJoystickPollRefCount = g_zInputJoystickPollRefCount;
    const unsigned char savedKeyboardFlags = g_zInput_DeviceRegistry;
    const short savedKeyboardPollRefCount = g_zInputKeyboardPollRefCount;
    const void *savedFrameBuffer = zRndr::g_frameBuffer;
    const int savedActiveWidth = zRndr::g_activeRegionWidth;
    const int savedActiveHeight = zRndr::g_activeRegionHeight;
    const int savedPitch = zRndr::g_pitchBytes;
    const int savedBytesPerPixel = zRndr::g_bytesPerPixel;

    g_messageBoxRunModalUpdateCount = 0;
    g_messageBoxRunModalSetEnabledCount = 0;
    g_messageBoxRunModalLastEnabled = -1;
    g_messageBoxRunModalLastDelta = -1.0f;
    g_messageBoxRunModalUnlockedState = nullptr;

    g_zVideo_ActiveRendererPath = 0;
    g_zVideo_RendererType = 0;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_HalfResAdjustMode = 0;
    g_zVideo_AdjustSurfacesDisableGate = 1;
    g_zVideo_pfnUnlockSurfaceState = TestMessageBoxRunModalUnlockSurface;
    g_zVideo_pfnBltPrimaryToSwRectDirect = TestMessageBoxRunModalBltNoOp;
    g_zInputMouseFlags = 0;
    g_zInputMousePollRefCount = 0;
    g_zInputJoystickFlags = 0;
    g_zInputJoystickPollRefCount = 0;
    g_zInput_DeviceRegistry = 0;
    g_zInputKeyboardPollRefCount = 0;

    int swPixels[16] = {};
    int primaryPixels[16] = {};
    g_zVideo_SwSurfaceState.width = 320;
    g_zVideo_SwSurfaceState.height = 240;
    g_zVideo_SwSurfaceState.pitch = 640;
    g_zVideo_SwSurfaceState.pixels = swPixels;
    g_zVideo_PrimarySurfaceState.width = 640;
    g_zVideo_PrimarySurfaceState.height = 480;
    g_zVideo_PrimarySurfaceState.pitch = 1280;
    g_zVideo_PrimarySurfaceState.pixels = primaryPixels;
    g_zVideo_PrimarySurfaceRectScratch = {0, 0, 640, 480};
    g_zVideo_DisplayModeBpp = 16;

    zOpt_ViewRectSection previousRegion = {};
    previousRegion.x = 3;
    previousRegion.y = 4;
    previousRegion.rightExclusive = 103;
    previousRegion.bottomExclusive = 84;
    zRndr::SetFrameBufferRegion(reinterpret_cast<void *>(0x11223344), &previousRegion, 24, 777);

    alignas(HudUiMessageBoxDialog) std::uint8_t dialogStorage[sizeof(HudUiMessageBoxDialog)]{};
    auto *const dialog = reinterpret_cast<HudUiMessageBoxDialog *>(dialogStorage);
    dialog->Constructor(nullptr, nullptr);

    TestMessageBoxRunModalElement updateProbe{};
    dialog->childHead = &updateProbe;
    dialog->childTail = &updateProbe;
    updateProbe.next = nullptr;
    updateProbe.parent = dialog;

    const int result = dialog->RunModal("BODY", "TITLE");

    const bool modal =
        result == 1 && dialog->modalResult == 1 && dialog->modalFrameCountdown == -1 &&
        g_messageBoxRunModalUpdateCount == 1 && dialog->enabled == 0 &&
        g_messageBoxRunModalLastDelta >= 0.0f &&
        g_messageBoxRunModalUnlockedState == &g_zVideo_PrimarySurfaceState;
    const bool textAndButton =
        std::strcmp(&TestFieldAt<char>(&dialog->messagePanel, 0x34), "BODY") == 0 &&
        std::strcmp(&TestFieldAt<char>(&dialog->titlePanel, 0x34), "TITLE") == 0 &&
        (dialog->okButton.flags & 0x10u) == 0;
    const bool restored =
        zRndr::g_frameBuffer == reinterpret_cast<void *>(0x11223344) &&
        zRndr::g_activeRegionWidth == 100 && zRndr::g_activeRegionHeight == 80 &&
        zRndr::g_pitchBytes == 777 && zRndr::g_bytesPerPixel == 3 &&
        g_zVideo_UseHalfResBackbuffer == 0;

    dialog->~HudUiMessageBoxDialog();

    g_zVideo_ActiveRendererPath = savedRendererPath;
    g_zVideo_RendererType = savedRendererType;
    g_zVideo_UseHalfResBackbuffer = savedUseHalfRes;
    g_zVideo_HalfResAdjustMode = savedHalfResMode;
    g_zVideo_AdjustSurfacesDisableGate = savedAdjustDisableGate;
    g_zVideo_pfnUnlockSurfaceState = savedUnlockSurfaceState;
    g_zVideo_pfnBltPrimaryToSwRectDirect = savedBltPrimaryToSw;
    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_SwSurfaceState = savedSwState;
    g_zVideo_PrimarySurfaceRectScratch = savedPrimaryRectScratch;
    g_zInputMouseFlags = savedMouseFlags;
    g_zInputMousePollRefCount = savedMousePollRefCount;
    g_zInputJoystickFlags = savedJoystickFlags;
    g_zInputJoystickPollRefCount = savedJoystickPollRefCount;
    g_zInput_DeviceRegistry = savedKeyboardFlags;
    g_zInputKeyboardPollRefCount = savedKeyboardPollRefCount;
    zRndr::g_frameBuffer = const_cast<void *>(savedFrameBuffer);
    zRndr::g_activeRegionWidth = savedActiveWidth;
    zRndr::g_activeRegionHeight = savedActiveHeight;
    zRndr::g_pitchBytes = savedPitch;
    zRndr::g_bytesPerPixel = savedBytesPerPixel;

    if (!(modal && textAndButton && restored)) {
        std::printf("zhud_message_box_run_modal_smoke modal=%d text=%d restored=%d result=%d "
                    "count=%d set=%d last=%d countdown=%d frame=%p w=%d h=%d pitch=%d bpp=%d\n",
                    modal ? 1 : 0, textAndButton ? 1 : 0, restored ? 1 : 0, result,
                    g_messageBoxRunModalUpdateCount, g_messageBoxRunModalSetEnabledCount,
                    g_messageBoxRunModalLastEnabled, dialog->modalFrameCountdown,
                    zRndr::g_frameBuffer, zRndr::g_activeRegionWidth,
                    zRndr::g_activeRegionHeight, zRndr::g_pitchBytes, zRndr::g_bytesPerPixel);
    }

    return modal && textAndButton && restored ? 0 : 1;
}

extern "C" int zhud_message_box_destructor_smoke(void) {
    const zVideo_SurfaceStatePartial savedPrimaryState = g_zVideo_PrimarySurfaceState;
    const zVidRect32 savedPrimaryRectScratch = g_zVideo_PrimarySurfaceRectScratch;

    g_zVideo_PrimarySurfaceState.width = 640;
    g_zVideo_PrimarySurfaceState.height = 480;
    g_zVideo_PrimarySurfaceRectScratch = {0, 0, 640, 480};

    alignas(HudUiMessageBoxDialog) std::uint8_t dialogStorage[sizeof(HudUiMessageBoxDialog)]{};
    auto *const dialog = reinterpret_cast<HudUiMessageBoxDialog *>(dialogStorage);
    dialog->Constructor(nullptr, nullptr);
    const bool hadImages =
        dialog->backgroundImage != nullptr && dialog->okButtonNormalImage != nullptr &&
        dialog->okButtonPressedImage != nullptr;

    dialog->~HudUiMessageBoxDialog();

    const bool destructed =
        hadImages && dialog->backgroundImage == nullptr &&
        TestFieldAt<const void *>(dialog, 0) != nullptr;

    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_PrimarySurfaceRectScratch = savedPrimaryRectScratch;

    return destructed ? 0 : 1;
}
