#include "Battlesport/CZGameFrame.h"
#include "Battlesport/CZRecoilFrame.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <cstring>

extern "C" int czrecoil_frame_build_window_title_smoke(void) {
    CZRecoilFrame frame{};
    alignas(CString) unsigned char storage[sizeof(CString)];
    auto *title = reinterpret_cast<CString *>(storage);

    g_zVideo_ActiveRendererPath = 0;
    CString *returned = frame.BuildWindowTitle(title);
    bool ok = returned == title && title->m_pchData != nullptr &&
              std::strcmp(title->m_pchData, "RECOIL") == 0;
    title->~CString();

    g_zVideo_ActiveRendererPath = 2;
    returned = frame.BuildWindowTitle(title);
    ok = ok && returned == title && title->m_pchData != nullptr &&
         std::strcmp(title->m_pchData, "RECOIL (3Dfx)") == 0;
    title->~CString();

    return ok ? 0 : 1;
}

extern "C" int czframe_metadata_accessors_smoke(void) {
    const auto gameRuntimeClass = reinterpret_cast<CRuntimeClass *>(
        static_cast<std::uintptr_t>(CZGameFrame::GetRuntimeClass()));
    const auto gameMessageMap = reinterpret_cast<const AFX_MSGMAP *>(
        static_cast<std::uintptr_t>(CZGameFrame::GetMessageMap()));
    const auto recoilRuntimeClass = reinterpret_cast<CRuntimeClass *>(
        static_cast<std::uintptr_t>(CZRecoilFrame::GetRuntimeClass()));
    const auto recoilMessageMap = reinterpret_cast<const AFX_MSGMAP *>(
        static_cast<std::uintptr_t>(CZRecoilFrame::GetMessageMap()));

    if (gameRuntimeClass != &CZGameFrame::classCZGameFrame ||
        std::strcmp(gameRuntimeClass->m_lpszClassName, "CZGameFrame") != 0 ||
        gameRuntimeClass->m_pfnGetBaseClass() == nullptr) {
        return 1;
    }

    if (gameMessageMap != &CZGameFrame::messageMap || gameMessageMap->pfnGetBaseMap() == nullptr ||
        gameMessageMap->lpEntries != &CZGameFrame::messageEntries[0]) {
        return 2;
    }

    if (recoilRuntimeClass != &CZRecoilFrame::classCZRecoilFrame ||
        std::strcmp(recoilRuntimeClass->m_lpszClassName, "CZRecoilFrame") != 0 ||
        recoilRuntimeClass->m_pfnGetBaseClass() != &CZGameFrame::classCZGameFrame) {
        return 3;
    }

    return recoilMessageMap == &CZRecoilFrame::messageMap &&
                   recoilMessageMap->pfnGetBaseMap() == &CZGameFrame::messageMap &&
                   recoilMessageMap->lpEntries == &CZRecoilFrame::messageEntries[0]
               ? 0
               : 4;
}

extern "C" int czrecoil_frame_set_menu_bar_visibility_smoke(void) {
    HWND hwnd = CreateWindowExA(0, "STATIC", "recoil-test", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100,
                                nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
    if (hwnd == nullptr) {
        return 1;
    }

    HMENU menu = CreateMenu();
    if (menu == nullptr) {
        DestroyWindow(hwnd);
        return 2;
    }

    CZRecoilFrame frame{};
    frame.m_hWnd = hwnd;
    frame.m_mainMenuHandle = menu;

    SetWindowLongA(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
    frame.SetMenuBarVisibility(0);
    const LONG hiddenStyle = GetWindowLongA(hwnd, GWL_STYLE);
    const bool hiddenOk = (hiddenStyle & WS_SYSMENU) == 0 && GetMenu(hwnd) == nullptr;

    frame.SetMenuBarVisibility(1);
    const LONG shownStyle = GetWindowLongA(hwnd, GWL_STYLE);
    const bool shownOk = (shownStyle & WS_SYSMENU) != 0 && GetMenu(hwnd) == menu;

    DestroyMenu(menu);
    DestroyWindow(hwnd);
    return hiddenOk && shownOk ? 0 : 3;
}

extern "C" int czrecoil_frame_configure_mode_feature_flags_smoke(void) {
    std::int32_t mode = 6;
    std::int32_t acceleration = 0;
    ZOPT_VIDEO_MODE = &mode;
    ZOPT_VIDEO_ACCELERATION = &acceleration;

    CZRecoilFrame frame{};
    frame.ConfigureModeFeatureFlags();
    if (frame.m_videoModeCmdUiState[0] != 0 || frame.m_videoModeCmdUiState[1] != 0 ||
        frame.m_videoModeCmdUiState[2] != 0 || frame.m_videoModeCmdUiState[3] != 0 ||
        frame.m_videoModeCmdUiState[4] != 8 || frame.m_videoModeCmdUiState[5] != 0) {
        return 1;
    }

    acceleration = 1;
    mode = 7;
    frame.m_vidMemFreeBytes = 0x480000;
    frame.ConfigureModeFeatureFlags();
    if (frame.m_videoModeCmdUiState[0] != 1 || frame.m_videoModeCmdUiState[1] != 1 ||
        frame.m_videoModeCmdUiState[2] != 0 || frame.m_videoModeCmdUiState[3] != 0 ||
        frame.m_videoModeCmdUiState[4] != 0 || frame.m_videoModeCmdUiState[5] != 1) {
        return 2;
    }

    frame.m_vidMemFreeBytes = 0x480001;
    frame.ConfigureModeFeatureFlags();
    return frame.m_videoModeCmdUiState[0] == 1 && frame.m_videoModeCmdUiState[1] == 1 &&
                   frame.m_videoModeCmdUiState[2] == 0 && frame.m_videoModeCmdUiState[3] == 0 &&
                   frame.m_videoModeCmdUiState[4] == 0 && frame.m_videoModeCmdUiState[5] == 8
               ? 0
               : 3;
}

namespace {
int g_gameFrameValidityReturn;

struct FakeGameFrameWindow : CZGameFrameMfcWindow {
    std::int32_t RECOIL_THISCALL QueryValidity() {
        return g_gameFrameValidityReturn;
    }
};
} // namespace

extern "C" int czrecoil_frame_set_hw_api_and_init_mode_smoke(void) {
    std::int32_t mode = 4;
    std::int32_t acceleration = 0;
    std::int32_t hwApi = -1;
    std::int32_t fullscreen = 0;
    std::int32_t replicate = -1;
    zOpt_ViewRectSection render{};
    zOpt_ViewRectSection display{};
    zOpt_ViewRectSection window{};
    zOpt_ViewRectSection *renderPtr = &render;
    zOpt_ViewRectSection *displayPtr = &display;
    zOpt_ViewRectSection *windowPtr = &window;
    ZOPT_VIDEO_MODE = &mode;
    ZOPT_VIDEO_ACCELERATION = &acceleration;
    ZOPT_HW_API = &hwApi;
    ZOPT_VIDEO_FULLSCREEN = &fullscreen;
    ZOPT_REPLICATE = &replicate;
    g_zOpt_RenderSectionOption = &renderPtr;
    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_WindowSectionOption = &windowPtr;
    g_zVideo_HwApiDeviceTable[2].m_videoMemTotalBytes = 0x900000;
    g_zVideo_HwApiDeviceTable[2].m_videoMemFreeBytes = 0x700000;
    g_zVideo_HwApiDeviceTable[2].m_textureMemTotalBytes = 0x200000;

    CZRecoilFrame frame{};
    frame.SetHwApiAndInitMode(2);

    if (frame.m_vidMemTotalBytes != 0x900000 || frame.m_vidMemFreeBytes != 0x500000 ||
        frame.m_fullscreenOption != 0 || frame.m_videoModeIndex != 4) {
        return 1;
    }

    if (hwApi != 1 || fullscreen != 1 || acceleration != 1 || g_zOpt_HwMode != 1 || mode != 5 ||
        replicate != 0 || display.width != 0x280 || display.height != 0x1e0) {
        return 2;
    }

    return frame.m_videoModeCmdUiState[0] == 1 && frame.m_videoModeCmdUiState[1] == 1 &&
                   frame.m_videoModeCmdUiState[2] == 0 && frame.m_videoModeCmdUiState[3] == 8 &&
                   frame.m_videoModeCmdUiState[4] == 0 && frame.m_videoModeCmdUiState[5] == 0
               ? 0
               : 3;
}

extern "C" int czrecoil_frame_init_fallback_mode_smoke(void) {
    std::int32_t mode = 5;
    std::int32_t acceleration = 1;
    std::int32_t hwApi = 1;
    std::int32_t fullscreen = 1;
    std::int32_t replicate = -1;
    zOpt_ViewRectSection render{};
    zOpt_ViewRectSection display{};
    zOpt_ViewRectSection window{};
    zOpt_ViewRectSection *renderPtr = &render;
    zOpt_ViewRectSection *displayPtr = &display;
    zOpt_ViewRectSection *windowPtr = &window;
    ZOPT_VIDEO_MODE = &mode;
    ZOPT_VIDEO_ACCELERATION = &acceleration;
    ZOPT_HW_API = &hwApi;
    ZOPT_VIDEO_FULLSCREEN = &fullscreen;
    ZOPT_REPLICATE = &replicate;
    g_zOpt_RenderSectionOption = &renderPtr;
    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_WindowSectionOption = &windowPtr;

    CZRecoilFrame frame{};
    frame.m_fullscreenOption = 0;
    frame.m_videoModeIndex = 3;
    frame.InitFallbackMode();

    return hwApi == 0 && acceleration == 0 && g_zOpt_HwMode == 0 && fullscreen == 0 && mode == 3 &&
                   replicate == 1 && render.width == 0x140 && render.height == 0xf0 &&
                   frame.m_videoModeCmdUiState[1] == 8
               ? 0
               : 1;
}

extern "C" int czrecoil_frame_ensure_hw_api_initialized_smoke(void) {
    std::int32_t mode = 4;
    std::int32_t acceleration = 0;
    std::int32_t hwApi = -1;
    std::int32_t fullscreen = 0;
    std::int32_t replicate = -1;
    zOpt_ViewRectSection render{};
    zOpt_ViewRectSection display{};
    zOpt_ViewRectSection window{};
    zOpt_ViewRectSection *renderPtr = &render;
    zOpt_ViewRectSection *displayPtr = &display;
    zOpt_ViewRectSection *windowPtr = &window;
    ZOPT_VIDEO_MODE = &mode;
    ZOPT_VIDEO_ACCELERATION = &acceleration;
    ZOPT_HW_API = &hwApi;
    ZOPT_VIDEO_FULLSCREEN = &fullscreen;
    ZOPT_REPLICATE = &replicate;
    g_zOpt_RenderSectionOption = &renderPtr;
    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_WindowSectionOption = &windowPtr;
    g_zVideo_HwApiDeviceTable[1].m_videoMemTotalBytes = 0x900000;
    g_zVideo_HwApiDeviceTable[1].m_videoMemFreeBytes = 0x700000;
    g_zVideo_HwApiDeviceTable[1].m_textureMemTotalBytes = 0x200000;

    CZRecoilFrame frame{};
    frame.m_hwApiCmdUiState[0] = 8;
    frame.m_fullscreenOption = 0;
    frame.m_videoModeIndex = 3;
    frame.EnsureHwApiInitialized(0);
    if (hwApi != -1 || acceleration != 0 || frame.m_hwApiCmdUiState[0] != 8) {
        return 1;
    }

    frame.m_hwApiCmdUiState[0] = 8;
    frame.EnsureHwApiInitialized(2);
    return frame.m_hwApiCmdUiState[0] == 0 && frame.m_hwApiCmdUiState[1] == 0 &&
                   frame.m_hwApiCmdUiState[2] == 8 && frame.m_hwApiCmdUiState[3] == 0 &&
                   hwApi == 1 && acceleration == 1 && mode == 5
               ? 0
               : 2;
}

extern "C" int czrecoil_frame_init_startup_hw_api_from_options_smoke(void) {
    std::int32_t mode = 4;
    std::int32_t acceleration = 0;
    std::int32_t hwApi = 1;
    std::int32_t fullscreen = 0;
    std::int32_t replicate = -1;
    zOpt_ViewRectSection render{};
    zOpt_ViewRectSection display{};
    zOpt_ViewRectSection window{};
    zOpt_ViewRectSection *renderPtr = &render;
    zOpt_ViewRectSection *displayPtr = &display;
    zOpt_ViewRectSection *windowPtr = &window;
    ZOPT_VIDEO_MODE = &mode;
    ZOPT_VIDEO_ACCELERATION = &acceleration;
    ZOPT_HW_API = &hwApi;
    ZOPT_VIDEO_FULLSCREEN = &fullscreen;
    ZOPT_REPLICATE = &replicate;
    g_zOpt_RenderSectionOption = &renderPtr;
    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_WindowSectionOption = &windowPtr;
    g_zVideo_NumAcceptedDirectDrawDevices = 2;
    g_zVideo_HwApiDeviceTable[1].m_videoMemTotalBytes = 0x900000;
    g_zVideo_HwApiDeviceTable[1].m_videoMemFreeBytes = 0x700000;
    g_zVideo_HwApiDeviceTable[1].m_textureMemTotalBytes = 0x200000;

    CZRecoilFrame frame{};
    frame.InitStartupHwApiFromOptions();
    if (frame.m_hwApiCmdUiState[0] != 0 || frame.m_hwApiCmdUiState[2] != 8 ||
        frame.m_videoModeIndex != 4 || hwApi != 1 || acceleration != 1 || mode != 5) {
        return 1;
    }

    hwApi = 0;
    acceleration = 1;
    fullscreen = 1;
    mode = 4;
    frame = {};
    frame.InitStartupHwApiFromOptions();
    return frame.m_hwApiCmdUiState[0] == 8 && frame.m_videoModeIndex == 5 &&
                   frame.m_fullscreenOption == 1 && hwApi == 0 && acceleration == 0 &&
                   fullscreen == 1 && mode == 5
               ? 0
               : 2;
}

namespace {
std::int32_t g_lastCmdUiEnable;
std::int32_t g_lastCmdUiCheck;
char g_lastCmdUiText[0x80];

struct FakeRecoilCmdUI : CZRecoilCmdUI {
    void RECOIL_THISCALL Enable(std::int32_t enable) {
        g_lastCmdUiEnable = enable;
    }

    void RECOIL_THISCALL SetCheck(std::int32_t check) {
        g_lastCmdUiCheck = check;
    }

    void RECOIL_THISCALL SetText(const char *text) {
        std::strncpy(g_lastCmdUiText, text, sizeof(g_lastCmdUiText) - 1);
        g_lastCmdUiText[sizeof(g_lastCmdUiText) - 1] = '\0';
    }
};

void InitFakeRecoilCmdUI(FakeRecoilCmdUI &cmdUi) {
    union MemberToFunction {
        void (RECOIL_THISCALL FakeRecoilCmdUI::*member)(std::int32_t);
        void(RECOIL_THISCALL *function)(CZRecoilCmdUI *, std::int32_t);
    };

    union TextMemberToFunction {
        void (RECOIL_THISCALL FakeRecoilCmdUI::*member)(const char *);
        void(RECOIL_THISCALL *function)(CZRecoilCmdUI *, const char *);
    };

    static CZRecoilCmdUIVtable vtable{};
    MemberToFunction enableThunk{};
    enableThunk.member = &FakeRecoilCmdUI::Enable;
    MemberToFunction checkThunk{};
    checkThunk.member = &FakeRecoilCmdUI::SetCheck;
    TextMemberToFunction textThunk{};
    textThunk.member = &FakeRecoilCmdUI::SetText;
    vtable.Enable = enableThunk.function;
    vtable.SetCheck = checkThunk.function;
    vtable.SetText = textThunk.function;
    cmdUi.vftable = &vtable;
}
} // namespace

extern "C" int czrecoil_frame_menu_toggle_smoke(void) {
    std::int32_t fullscreen = 0;
    std::int32_t hudSw = 1;
    std::int32_t hudHw = 0;
    ZOPT_VIDEO_FULLSCREEN = &fullscreen;
    ZOPT_HUD_SW = &hudSw;
    ZOPT_HUD_HW = &hudHw;

    CZRecoilFrame frame{};
    frame.OnMenuToggleFullscreen();
    if (fullscreen != 1) {
        return 1;
    }

    frame.OnMenuToggleFullscreen();
    if (fullscreen != 0) {
        return 2;
    }

    g_zOpt_HwMode = 0;
    frame.OnMenuToggleHud();
    if (hudSw != 0) {
        return 3;
    }

    g_lastCmdUiEnable = 0;
    g_lastCmdUiCheck = -1;
    FakeRecoilCmdUI cmdUi{};
    InitFakeRecoilCmdUI(cmdUi);
    frame.OnUpdateHudCmdUI(&cmdUi);
    return g_lastCmdUiEnable == 1 && g_lastCmdUiCheck == 0 ? 0 : 4;
}

extern "C" int czrecoil_frame_update_video_mode_cmd_ui_smoke(void) {
    CZRecoilFrame frame{};
    FakeRecoilCmdUI cmdUi{};
    InitFakeRecoilCmdUI(cmdUi);

    frame.m_videoModeCmdUiState[0] = 1;
    g_lastCmdUiEnable = -1;
    g_lastCmdUiCheck = -1;
    frame.OnUpdateVideoMode2CmdUI(&cmdUi);
    if (g_lastCmdUiEnable != 0 || g_lastCmdUiCheck != 0) {
        return 1;
    }

    frame.m_videoModeCmdUiState[1] = 8;
    g_lastCmdUiEnable = -1;
    g_lastCmdUiCheck = -1;
    frame.OnUpdateVideoMode3CmdUI(&cmdUi);
    if (g_lastCmdUiEnable != 1 || g_lastCmdUiCheck != 1) {
        return 2;
    }

    frame.m_videoModeCmdUiState[2] = 0;
    g_lastCmdUiEnable = -1;
    g_lastCmdUiCheck = -1;
    frame.OnUpdateVideoMode4CmdUI(&cmdUi);
    if (g_lastCmdUiEnable != 1 || g_lastCmdUiCheck != 0) {
        return 3;
    }

    frame.m_videoModeCmdUiState[3] = 8;
    frame.m_videoModeCmdUiState[4] = 1;
    frame.m_videoModeCmdUiState[5] = 0;
    frame.OnUpdateVideoMode5CmdUI(&cmdUi);
    if (g_lastCmdUiEnable != 1 || g_lastCmdUiCheck != 1) {
        return 4;
    }

    frame.OnUpdateVideoMode6CmdUI(&cmdUi);
    if (g_lastCmdUiEnable != 0 || g_lastCmdUiCheck != 0) {
        return 5;
    }

    frame.OnUpdateVideoMode7CmdUI(&cmdUi);
    return g_lastCmdUiEnable == 1 && g_lastCmdUiCheck == 0 ? 0 : 6;
}

extern "C" int czrecoil_frame_hw_api_menu_cmd_ui_smoke(void) {
    CZRecoilFrame frame{};
    FakeRecoilCmdUI cmdUi{};
    InitFakeRecoilCmdUI(cmdUi);

    frame.m_hwApiCmdUiState[0] = 8;
    g_lastCmdUiEnable = -1;
    g_lastCmdUiCheck = -1;
    frame.OnUpdateHwApi0CmdUI(&cmdUi);
    if (g_lastCmdUiEnable != 1 || g_lastCmdUiCheck != 1) {
        return 1;
    }

    strcpy_s(g_zVideo_HwApiDeviceTable[1].m_driverDescription, "Test Device");
    strcpy_s(g_zVideo_HwApiDeviceTable[1].m_driverName, "testdrv");
    frame.m_acceptedD3DDeviceCount = 2;
    frame.m_hwApiCmdUiState[2] = 8;
    g_lastCmdUiCheck = -1;
    g_lastCmdUiText[0] = '\0';
    frame.OnUpdateHwApi2CmdUI(&cmdUi);
    if (g_lastCmdUiCheck != 1 ||
        std::strcmp(g_lastCmdUiText, "Accelerator - Test Device (testdrv)") != 0) {
        return 2;
    }

    HMENU menu = CreateMenu();
    if (menu == nullptr) {
        return 3;
    }

    CZRecoilMenuProxy menuProxy{nullptr, menu};
    cmdUi.m_menu_0c = &menuProxy;
    frame.m_acceptedD3DDeviceCount = 2;
    frame.m_hwApiMenuCommandIds[3] = 0x7003;
    AppendMenuA(menu, MF_STRING, frame.m_hwApiMenuCommandIds[3], "extra");
    frame.OnUpdateHwApi3CmdUI(&cmdUi);
    if (GetMenuState(menu, frame.m_hwApiMenuCommandIds[3], MF_BYCOMMAND) != 0xffffffff) {
        DestroyMenu(menu);
        return 4;
    }

    AppendMenuA(menu, MF_STRING, 0x9c4e, "fullscreen");
    frame.OnUpdateFullscreenCmdUI(&cmdUi);
    const bool fullscreenRemoved = GetMenuState(menu, 0x9c4e, MF_BYCOMMAND) == 0xffffffff;
    DestroyMenu(menu);
    return fullscreenRemoved ? 0 : 5;
}

extern "C" int czrecoil_frame_audio_input_menu_smoke(void) {
    std::int32_t cdAudio = 0;
    std::int32_t joystick = 0;
    std::int32_t audioApi = 0;
    ZOPT_SOUND_CDAUDIO = &cdAudio;
    ZOPT_INPUT_JOYSTICK = &joystick;
    ZOPT_AUDIO_API = &audioApi;
    g_zSnd_IsInitialized = 0;
    g_zSnd_ActiveBackend = 0;

    CZRecoilFrame frame{};
    FakeRecoilCmdUI cmdUi{};
    InitFakeRecoilCmdUI(cmdUi);

    g_lastCmdUiEnable = -1;
    MfcCmdUI::EnableAlways(&cmdUi);
    if (g_lastCmdUiEnable != 1) {
        return 9;
    }

    frame.OnMenuToggleCDAudio();
    if (cdAudio != 1) {
        return 1;
    }

    g_lastCmdUiEnable = -1;
    g_lastCmdUiCheck = -1;
    frame.OnUpdateCDAudioCmdUI(&cmdUi);
    if (g_lastCmdUiEnable != 1 || g_lastCmdUiCheck != 1) {
        return 2;
    }

    frame.OnMenuToggleJoystick();
    if (joystick != 1) {
        return 3;
    }

    g_lastCmdUiEnable = -1;
    g_lastCmdUiCheck = -1;
    frame.OnUpdateJoystickCmdUI(&cmdUi);
    if (g_lastCmdUiEnable != 1 || g_lastCmdUiCheck != 1) {
        return 4;
    }

    frame.OnMenuSelectA3D();
    if (audioApi != 1 || g_zSnd_ActiveBackend != 1) {
        return 5;
    }

    g_lastCmdUiEnable = -1;
    g_lastCmdUiCheck = -1;
    frame.OnUpdateA3DCmdUI(&cmdUi);
    if (g_lastCmdUiEnable != 1 || g_lastCmdUiCheck != 1) {
        return 6;
    }

    frame.OnMenuSelectDirectSound();
    if (audioApi != 0 || g_zSnd_ActiveBackend != 0) {
        return 7;
    }

    g_lastCmdUiEnable = -1;
    g_lastCmdUiCheck = -1;
    frame.OnUpdateDirectSoundCmdUI(&cmdUi);
    return g_lastCmdUiEnable == 1 && g_lastCmdUiCheck == 1 ? 0 : 8;
}

extern "C" int czrecoil_frame_menu_exit_game_smoke(void) {
    HWND hwnd = CreateWindowExA(0, "STATIC", "recoil-exit-test", WS_OVERLAPPEDWINDOW, 0, 0, 100,
                                100, nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
    if (hwnd == nullptr) {
        return 1;
    }

    CZRecoilFrame frame{};
    frame.m_hWnd = hwnd;
    frame.OnMenuExitGame();

    MSG msg{};
    const BOOL found = PeekMessageA(&msg, hwnd, WM_CLOSE, WM_CLOSE, PM_REMOVE);
    DestroyWindow(hwnd);
    return found != 0 && msg.message == WM_CLOSE ? 0 : 2;
}

extern "C" int czgame_frame_is_window_valid_smoke(void) {
    union MemberToFunction {
        std::int32_t (RECOIL_THISCALL FakeGameFrameWindow::*member)();
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &FakeGameFrameWindow::QueryValidity;

    void *vtable[5] = {};
    vtable[4] = thunk.function;
    FakeGameFrameWindow wnd{{vtable}};

    if (CZGameFrame::IsWindowValid(nullptr) != 0) {
        return 1;
    }

    g_gameFrameValidityReturn = 0;
    if (CZGameFrame::IsWindowValid(&wnd) != 1) {
        return 2;
    }

    g_gameFrameValidityReturn = 7;
    return CZGameFrame::IsWindowValid(&wnd) == 0 ? 0 : 3;
}

extern "C" int czgame_frame_build_window_title_smoke(void) {
    CZGameFrame frame{};
    alignas(CString) unsigned char storage[sizeof(CString)];
    auto *title = reinterpret_cast<CString *>(storage);

    CString *returned = frame.BuildWindowTitle(title);
    const bool ok = returned == title && title->m_pchData != nullptr &&
                    std::strcmp(title->m_pchData, "Zipper Interactive") == 0;
    title->~CString();

    return ok ? 0 : 1;
}

extern "C" int czgame_frame_constructor_smoke(void) {
    CZGameFrame frame{};
    CZGameFrame *returned = frame.Constructor(nullptr);
    const auto constructedFrameVtable = *reinterpret_cast<RecoilPtr32 *>(&frame);
    const auto constructedBitmapVtable = frame.m_gameBitmap.vftable;
    if (returned == &frame && constructedFrameVtable != 0 &&
        constructedFrameVtable != CZGameFrame::GetRuntimeClass() &&
        constructedBitmapVtable != nullptr && frame.m_gameBitmap.m_hObject == nullptr) {
        frame.Destructor();
        return frame.m_gameBitmap.vftable != nullptr &&
                       frame.m_gameBitmap.vftable != constructedBitmapVtable
                   ? 0
                   : 2;
    }

    return 1;
}

extern "C" int czrecoil_frame_destructor_smoke(void) {
    CZRecoilFrame frame{};
    reinterpret_cast<CZGameFrame *>(&frame)->Constructor(nullptr);
    frame.m_mainMenuVftable = 0;
    frame.m_mainMenuHandle = CreateMenu();
    if (frame.m_mainMenuHandle == nullptr) {
        return 1;
    }

    frame.Destructor();
    return frame.m_mainMenuVftable != 0 &&
                   frame.m_mainMenuVftable != CZRecoilFrame::GetRuntimeClass()
               ? 0
               : 2;
}

namespace {
std::uint32_t g_lastIdleWParam;
std::uint32_t g_lastIdleLParam;
std::int32_t g_appDeactivateCalls;

struct FakeGameFrameApp : CZGameFrameApp {
    std::int32_t RECOIL_THISCALL OnIdleOrDispatch(std::uint32_t wParam, std::uint32_t lParam) {
        g_lastIdleWParam = wParam;
        g_lastIdleLParam = lParam;
        return 0x1234;
    }

    void RECOIL_THISCALL OnDeactivate() {
        ++g_appDeactivateCalls;
    }
};
} // namespace

extern "C" int czgame_frame_on_app_idle_dispatch_message_smoke(void) {
    union MemberToFunction {
        std::int32_t (RECOIL_THISCALL FakeGameFrameApp::*member)(std::uint32_t, std::uint32_t);
        std::int32_t(RECOIL_THISCALL *function)(CZGameFrameApp *, std::uint32_t, std::uint32_t);
    };

    MemberToFunction thunk{};
    thunk.member = &FakeGameFrameApp::OnIdleOrDispatch;

    CZGameFrameAppVtable vtable{};
    vtable.OnIdleOrDispatch = thunk.function;
    FakeGameFrameApp app{{&vtable}};
    CZGameFrame frame{};
    frame.m_app = &app;

    const std::int32_t result = frame.OnAppIdleDispatchMessage(0xabcdef01, 0x23456789);

    return result == 0x1234 && g_lastIdleWParam == 0xabcdef01 && g_lastIdleLParam == 0x23456789 ? 0
                                                                                                : 1;
}

extern "C" int czrecoil_frame_on_size_smoke(void) {
    union DeactivateMemberToFunction {
        void (RECOIL_THISCALL FakeGameFrameApp::*member)();
        void(RECOIL_THISCALL *function)(CZGameFrameApp *);
    };

    DeactivateMemberToFunction deactivateThunk{};
    deactivateThunk.member = &FakeGameFrameApp::OnDeactivate;

    CZGameFrameAppVtable vtable{};
    vtable.OnAppDeactivate = deactivateThunk.function;
    FakeGameFrameApp app{{&vtable}};
    CZRecoilFrame frame{};
    reinterpret_cast<CZGameFrame *>(&frame)->m_app = &app;
    frame.m_hWnd = CreateWindowExA(0, "STATIC", "recoil-size-test", WS_OVERLAPPEDWINDOW, 0, 0, 100,
                                   100, nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
    if (frame.m_hWnd == nullptr) {
        return 4;
    }

    g_appDeactivateCalls = 0;
    frame.OnSize(0, 640, 480);
    if (g_appDeactivateCalls != 0) {
        DestroyWindow(frame.m_hWnd);
        return 1;
    }

    frame.OnSize(1, 640, 480);
    if (g_appDeactivateCalls != 1) {
        DestroyWindow(frame.m_hWnd);
        return 2;
    }

    frame.OnSize(4, 640, 480);
    const bool ok = g_appDeactivateCalls == 2;
    DestroyWindow(frame.m_hWnd);
    return ok ? 0 : 3;
}
