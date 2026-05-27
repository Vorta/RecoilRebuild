#include "Battlesport/CZGameFrame.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/RecoilApp.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <commdlg.h>
#include <cstring>

extern "C" int g_CZRecoilFrame_HasWolApi;
BOOL RECOIL_STDCALL AfxWinInit(HINSTANCE instance, HINSTANCE previousInstance, LPSTR commandLine,
                               int showCommand);
HINSTANCE RECOIL_STDCALL AfxFindResourceHandle(LPCSTR resourceName, LPCSTR resourceType);

class CWnd {
  public:
    BOOL CreateEx(DWORD dwExStyle, LPCSTR className, LPCSTR windowName, DWORD style, int x, int y,
                  int width, int height, HWND parent, HMENU menu, LPVOID param);
    void SetWindowTextA(LPCSTR text);
    void CenterWindow(CWnd *alternateOwner);

  private:
    unsigned char reserved000[0x20];
    HWND m_hWnd;
};

BOOL CWnd::CreateEx(DWORD dwExStyle, LPCSTR className, LPCSTR windowName, DWORD style, int x,
                    int y, int width, int height, HWND parent, HMENU menu, LPVOID param) {
    m_hWnd = CreateWindowExA(dwExStyle, className, windowName, style, x, y, width, height, parent,
                             menu, GetModuleHandleA(nullptr), param);
    return m_hWnd != nullptr ? 1 : 0;
}

void CWnd::SetWindowTextA(LPCSTR text) {
    ::SetWindowTextA(m_hWnd, text);
}

void CWnd::CenterWindow(CWnd *alternateOwner) {
    (void)alternateOwner;
}

namespace {
int HandleFrameConstructorException(EXCEPTION_POINTERS *exceptionInfo) {
    (void)exceptionInfo;
    return EXCEPTION_EXECUTE_HANDLER;
}
} // namespace

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
    const auto gameBaseRuntimeClass = reinterpret_cast<CRuntimeClass *>(
        static_cast<std::uintptr_t>(CZGameFrame::GetBaseRuntimeClass()));
    const auto gameMessageMap = reinterpret_cast<const AFX_MSGMAP *>(
        static_cast<std::uintptr_t>(CZGameFrame::GetMessageMap()));
    const auto gameBaseMessageMap = reinterpret_cast<const AFX_MSGMAP *>(
        static_cast<std::uintptr_t>(CZGameFrame::GetBaseMessageMap()));
    const auto recoilRuntimeClass = reinterpret_cast<CRuntimeClass *>(
        static_cast<std::uintptr_t>(CZRecoilFrame::GetRuntimeClass()));
    const auto recoilMessageMap = reinterpret_cast<const AFX_MSGMAP *>(
        static_cast<std::uintptr_t>(CZRecoilFrame::GetMessageMap()));

    if (gameRuntimeClass != &CZGameFrame::classCZGameFrame ||
        std::strcmp(gameRuntimeClass->m_lpszClassName, "CZGameFrame") != 0 ||
        gameRuntimeClass->m_pfnGetBaseClass() == nullptr) {
        return 1;
    }

    if (gameBaseRuntimeClass != &CFrameWnd::classCFrameWnd ||
        gameBaseMessageMap != CZGameFrame::GetBaseMessageMapForMfc()) {
        return 5;
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

extern "C" int get_open_file_name_import_provider_smoke(void) {
    OPENFILENAMEA ofn{};
    ofn.lStructSize = 0;
    return GetOpenFileNameA(&ofn) == 0 ? 0 : 1;
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

extern "C" int czrecoil_frame_video_mode_menu_handlers_smoke(void) {
    std::int32_t mode = 0;
    std::int32_t acceleration = 0;
    std::int32_t replicate = -1;
    zOpt_ViewRectSection render{};
    zOpt_ViewRectSection display{};
    zOpt_ViewRectSection window{};
    zOpt_ViewRectSection *renderPtr = &render;
    zOpt_ViewRectSection *displayPtr = &display;
    zOpt_ViewRectSection *windowPtr = &window;
    ZOPT_VIDEO_MODE = &mode;
    ZOPT_VIDEO_ACCELERATION = &acceleration;
    ZOPT_REPLICATE = &replicate;
    g_zOpt_RenderSectionOption = &renderPtr;
    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_WindowSectionOption = &windowPtr;

    CZRecoilFrame frame{};
    frame.m_vidMemFreeBytes = 0x800000;

    typedef void (RECOIL_THISCALL CZRecoilFrame::*Handler)();
    const Handler handlers[] = {
        &CZRecoilFrame::OnMenuSetVideoMode2,
        &CZRecoilFrame::OnMenuSetVideoMode3,
        &CZRecoilFrame::OnMenuSetVideoMode4,
        &CZRecoilFrame::OnMenuSetVideoMode5,
        &CZRecoilFrame::OnMenuSetVideoMode6,
        &CZRecoilFrame::OnMenuSetVideoMode7,
    };

    for (int index = 0; index < 6; ++index) {
        mode = 0;
        for (int stateIndex = 0; stateIndex < 6; ++stateIndex) {
            frame.m_videoModeCmdUiState[stateIndex] = -1;
        }

        (frame.*handlers[index])();
        const int expectedMode = index + 2;
        if (mode != expectedMode) {
            return index + 1;
        }

        for (int stateIndex = 0; stateIndex < 6; ++stateIndex) {
            const int expectedState = stateIndex == index ? 8 : 0;
            if (frame.m_videoModeCmdUiState[stateIndex] != expectedState) {
                return 10 + index;
            }
        }
    }

    return 0;
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

    strcpy_s(g_zVideo_HwApiDeviceTable[0].m_driverDescription, "Api One");
    strcpy_s(g_zVideo_HwApiDeviceTable[0].m_driverName, "drv1");
    frame.m_acceptedD3DDeviceCount = 2;
    frame.m_hwApiCmdUiState[1] = 0;
    g_lastCmdUiCheck = -1;
    g_lastCmdUiText[0] = '\0';
    frame.OnUpdateHwApi1CmdUI(&cmdUi);
    if (g_lastCmdUiCheck != 0 ||
        std::strcmp(g_lastCmdUiText, "Accelerator - Api One (drv1)") != 0) {
        return 2;
    }

    strcpy_s(g_zVideo_HwApiDeviceTable[1].m_driverDescription, "Test Device");
    strcpy_s(g_zVideo_HwApiDeviceTable[1].m_driverName, "testdrv");
    frame.m_hwApiCmdUiState[2] = 8;
    g_lastCmdUiCheck = -1;
    g_lastCmdUiText[0] = '\0';
    frame.OnUpdateHwApi2CmdUI(&cmdUi);
    if (g_lastCmdUiCheck != 1 ||
        std::strcmp(g_lastCmdUiText, "Accelerator - Test Device (testdrv)") != 0) {
        return 3;
    }

    HMENU menu = CreateMenu();
    if (menu == nullptr) {
        return 4;
    }

    CZRecoilMenuProxy menuProxy{nullptr, menu};
    cmdUi.m_menu_0c = &menuProxy;
    frame.m_acceptedD3DDeviceCount = 2;
    frame.m_hwApiMenuCommandIds[3] = 0x7003;
    AppendMenuA(menu, MF_STRING, frame.m_hwApiMenuCommandIds[3], "extra");
    frame.OnUpdateHwApi3CmdUI(&cmdUi);
    if (GetMenuState(menu, frame.m_hwApiMenuCommandIds[3], MF_BYCOMMAND) != 0xffffffff) {
        DestroyMenu(menu);
        return 5;
    }

    AppendMenuA(menu, MF_STRING, 0x9c4e, "fullscreen");
    frame.OnUpdateFullscreenCmdUI(&cmdUi);
    const bool fullscreenRemoved = GetMenuState(menu, 0x9c4e, MF_BYCOMMAND) == 0xffffffff;
    DestroyMenu(menu);
    return fullscreenRemoved ? 0 : 6;
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

extern "C" int czgame_frame_create_object_smoke(void) {
    CZGameFrame *const frame = CZGameFrame::CreateObject();
    if (frame == nullptr) {
        return 1;
    }

    const auto constructedFrameVtable = *reinterpret_cast<RecoilPtr32 *>(frame);
    const auto constructedBitmapVtable = frame->m_gameBitmap.vftable;
    const bool ok = constructedFrameVtable != 0 &&
                    constructedFrameVtable != CZGameFrame::GetRuntimeClass() &&
                    constructedBitmapVtable != nullptr && frame->m_gameBitmap.m_hObject == nullptr;

    frame->Destructor();
    ::operator delete(frame);
    return ok ? 0 : 2;
}

extern "C" int czgame_frame_destructor_smoke(void) {
    CZGameFrame frame{};
    frame.Constructor(nullptr);

    const auto constructedBitmapVtable = frame.m_gameBitmap.vftable;
    frame.Destructor();

    return frame.m_gameBitmap.vftable != nullptr &&
                   frame.m_gameBitmap.vftable != constructedBitmapVtable
               ? 0
               : 1;
}

extern "C" int czrecoil_frame_constructor_smoke(void) {
    g_RecoilApp.Constructor();

    HINSTANCE instance = GetModuleHandleA(nullptr);
    if (AfxWinInit(instance, nullptr, GetCommandLineA(), SW_HIDE) == 0) {
        return 1;
    }

    WNDCLASSA wndClass{};
    wndClass.lpfnWndProc = DefWindowProcA;
    wndClass.hInstance = instance;
    wndClass.lpszClassName = "RecoilClass";
    RegisterClassA(&wndClass);

    CZRecoilFrame frame{};
    g_zVid_AcceptedHardwareRendererCount = 5;
    g_zVid_TexturePackLoadState = 1;
    g_zSnd_UseArchiveBanksFlag = 0;
    g_CZRecoilFrame_HasWolApi = 0;

    CZRecoilFrame *returned = nullptr;
    __try {
        returned = frame.Constructor();
    } __except (HandleFrameConstructorException(GetExceptionInformation())) {
        return 10;
    }

    const auto constructedFrameVtable = *reinterpret_cast<RecoilPtr32 *>(&frame);
    const bool constructed = returned == &frame && constructedFrameVtable != 0 &&
                             constructedFrameVtable != CZRecoilFrame::GetRuntimeClass();
    const bool fieldsOk =
        frame.m_openZbdFilePath[0] == '\0' && frame.m_useArchiveBanks == 1 &&
        frame.m_cmdlineFlag == 1 && frame.m_campaignsOnlyMode == 0 &&
        frame.m_acceptedD3DDeviceCount == g_zVid_AcceptedHardwareRendererCount &&
        frame.m_hwApiCmdUiState[0] == 0 &&
        frame.m_hwApiCmdUiState[1] == 0 && frame.m_hwApiCmdUiState[2] == 0 &&
        frame.m_hwApiCmdUiState[3] == 0 && frame.m_hwApiMenuCommandIds[0] == 0x9c83 &&
        frame.m_hwApiMenuCommandIds[1] == 0x9c72 &&
        frame.m_hwApiMenuCommandIds[2] == 0x9c75 &&
        frame.m_hwApiMenuCommandIds[3] == 0x9c76;
    const bool globalsOk = g_zSnd_UseArchiveBanksFlag == 1;

    return !constructed ? 2 : (!fieldsOk ? 3 : (globalsOk ? 0 : 4));
}

extern "C" int czrecoil_frame_create_object_smoke(void) {
    g_RecoilApp.Constructor();

    HINSTANCE instance = GetModuleHandleA(nullptr);
    if (AfxWinInit(instance, nullptr, GetCommandLineA(), SW_HIDE) == 0) {
        return 1;
    }

    WNDCLASSA wndClass{};
    wndClass.lpfnWndProc = DefWindowProcA;
    wndClass.hInstance = instance;
    wndClass.lpszClassName = "RecoilClass";
    RegisterClassA(&wndClass);

    g_zVid_AcceptedHardwareRendererCount = 6;
    g_zVid_TexturePackLoadState = 1;
    g_zSnd_UseArchiveBanksFlag = 0;
    g_CZRecoilFrame_HasWolApi = 0;

    CZRecoilFrame *const frame = CZRecoilFrame::CreateObject();
    if (frame == nullptr) {
        return 2;
    }

    const auto constructedFrameVtable = *reinterpret_cast<RecoilPtr32 *>(frame);
    const bool ok = constructedFrameVtable != 0 &&
                    constructedFrameVtable != CZRecoilFrame::GetRuntimeClass() &&
                    frame->m_useArchiveBanks == 1 &&
                    frame->m_acceptedD3DDeviceCount == g_zVid_AcceptedHardwareRendererCount &&
                    g_zSnd_UseArchiveBanksFlag == 1;

    frame->Destructor();
    ::operator delete(frame);
    return ok ? 0 : 3;
}

extern "C" int recoil_app_create_main_wnd_smoke(void) {
    g_RecoilApp.Constructor();

    HINSTANCE instance = GetModuleHandleA(nullptr);
    if (AfxWinInit(instance, nullptr, GetCommandLineA(), SW_HIDE) == 0) {
        return 1;
    }

    WNDCLASSA wndClass{};
    wndClass.lpfnWndProc = DefWindowProcA;
    wndClass.hInstance = instance;
    wndClass.lpszClassName = "RecoilClass";
    RegisterClassA(&wndClass);

    g_zVid_AcceptedHardwareRendererCount = 4;
    g_zVid_TexturePackLoadState = 1;
    g_zSnd_UseArchiveBanksFlag = 0;
    g_CZRecoilFrame_HasWolApi = 0;

    CZRecoilFrame *frame = g_RecoilApp.CreateMainWnd();
    if (frame == nullptr) {
        return 2;
    }

    const auto constructedFrameVtable = *reinterpret_cast<RecoilPtr32 *>(frame);
    return constructedFrameVtable != 0 &&
                   constructedFrameVtable != CZRecoilFrame::GetRuntimeClass() &&
                   frame->m_useArchiveBanks == 1 &&
                   frame->m_acceptedD3DDeviceCount == g_zVid_AcceptedHardwareRendererCount &&
                   g_zSnd_UseArchiveBanksFlag == 1
               ? 0
               : 3;
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
std::int32_t g_appActivateCalls;
std::int32_t g_appDeactivateCalls;
std::int32_t g_cFrameWndOnCloseCalls;
std::int32_t g_cFrameWndOnCreateCalls;
std::int32_t g_cFrameWndOnCreateResult;
std::int32_t g_cFrameWndOnDestroyCalls;
std::int32_t g_cFrameWndOnActivateCalls;
bool g_cFrameWndOnActivateArgsOk;
std::int32_t g_stateWndActivateCalls;
std::int32_t g_lastStateWndActivateValue;
std::int32_t g_zInputOnAppActivateCalls;
std::int32_t g_zInputOnAppDeactivateCalls;
std::int32_t g_zGameReturnOnlyStubCalls;
std::int32_t g_zVideoRestoreIconicCalls;
std::int32_t g_gdiDeleteObjectCalls;
std::int32_t g_onDestroyCallOrder[5];
std::int32_t g_onDestroyCallCount;
std::int32_t g_onActivateCallOrder[5];
std::int32_t g_onActivateCallCount;
std::int32_t g_cFrameWndOnSizeCalls;
bool g_cFrameWndOnSizeArgsOk;
std::int32_t g_findResourceHandleCalls;
std::int32_t g_loadBitmapCalls;
HINSTANCE g_fakeResourceHandle = reinterpret_cast<HINSTANCE>(static_cast<std::uintptr_t>(0x1357));
HBITMAP g_fakeBitmapHandle = reinterpret_cast<HBITMAP>(static_cast<std::uintptr_t>(0x2468));
LPCSTR g_lastResourceName;
LPCSTR g_lastResourceType;
LPCSTR g_lastBitmapName;
HINSTANCE g_lastBitmapInstance;
std::int32_t g_paintCreateCompatibleDcCalls;
std::int32_t g_paintSelectObjectCalls;
std::int32_t g_paintBitBltCalls;
std::int32_t g_paintStretchBltCalls;
std::int32_t g_paintDeleteDcCalls;
HDC g_paintCompatibleDc = reinterpret_cast<HDC>(static_cast<std::uintptr_t>(0x12345678));
HDC g_paintLastDestDc;
HDC g_paintLastSourceDc;
HGDIOBJ g_paintLastSelectedObject;
RECT g_paintLastDestRect;
RECT g_paintLastSourceRect;
DWORD g_paintLastRasterOp;

struct CodeFunctionPatch {
    unsigned char *address;
    unsigned char original[5];
};

struct ImportFunctionPatch {
    ULONG_PTR *slot;
    ULONG_PTR original;
};

struct FakeGameFrameApp : CZGameFrameApp {
    void RECOIL_THISCALL OnActivate() {
        ++g_appActivateCalls;
        if (g_onActivateCallCount < 5) {
            g_onActivateCallOrder[g_onActivateCallCount] = 6;
        }
        ++g_onActivateCallCount;
    }

    std::int32_t RECOIL_THISCALL OnIdleOrDispatch(std::uint32_t wParam, std::uint32_t lParam) {
        g_lastIdleWParam = wParam;
        g_lastIdleLParam = lParam;
        return 0x1234;
    }

    void RECOIL_THISCALL OnDeactivate() {
        ++g_appDeactivateCalls;
        if (g_onActivateCallCount < 5) {
            g_onActivateCallOrder[g_onActivateCallCount] = 3;
        }
        ++g_onActivateCallCount;
    }
};

struct FakeActivateState : RecoilApp_IState {
    void RECOIL_THISCALL OnWndActivate(std::uint32_t nState) {
        ++g_stateWndActivateCalls;
        g_lastStateWndActivateValue = static_cast<std::int32_t>(nState);
        if (g_onActivateCallCount < 5) {
            g_onActivateCallOrder[g_onActivateCallCount] = 2;
        }
        ++g_onActivateCallCount;
    }
};

struct CFrameWndOnActivateAccess : CFrameWnd {
    using CFrameWnd::OnActivate;
};

struct CFrameWndOnSizeAccess : CFrameWnd {
    using CFrameWnd::OnSize;
};

struct CFrameWndOnCloseAccess : CFrameWnd {
    using CFrameWnd::OnClose;
};

struct CFrameWndOnCreateAccess : CFrameWnd {
    using CFrameWnd::OnCreate;
};

struct CFrameWndOnDestroyAccess : CFrameWnd {
    using CFrameWnd::OnDestroy;
};

void RecordOnDestroyCall(std::int32_t callId) {
    if (g_onDestroyCallCount < 5) {
        g_onDestroyCallOrder[g_onDestroyCallCount] = callId;
    }
    ++g_onDestroyCallCount;
}

void RecordOnActivateCall(std::int32_t callId) {
    if (g_onActivateCallCount < 5) {
        g_onActivateCallOrder[g_onActivateCallCount] = callId;
    }
    ++g_onActivateCallCount;
}

int RECOIL_CDECL FakeDestroyCachedLocalPlayer() {
    RecordOnDestroyCall(1);
    return 1;
}

void RECOIL_CDECL FakeZInputOnAppActivate() {
    ++g_zInputOnAppActivateCalls;
    RecordOnActivateCall(7);
}

void RECOIL_CDECL FakeZInputOnAppDeactivate() {
    ++g_zInputOnAppDeactivateCalls;
    RecordOnActivateCall(4);
}

void RECOIL_CDECL FakeZGameReturnOnlyStub() {
    ++g_zGameReturnOnlyStubCalls;
    RecordOnActivateCall(5);
}

void RECOIL_CDECL FakeZVideoRestoreIconicFullscreenWindowIfNeeded() {
    ++g_zVideoRestoreIconicCalls;
    RecordOnActivateCall(8);
}

int RECOIL_CDECL FakeShutdownVideoSystem() {
    RecordOnDestroyCall(2);
    return 1;
}

int RECOIL_CDECL FakeZsndCdStop() {
    RecordOnDestroyCall(3);
    return 1;
}

int RECOIL_FASTCALL FakeCFrameWndOnCreate(CFrameWnd *, void *, CREATESTRUCTA *) {
    ++g_cFrameWndOnCreateCalls;
    return g_cFrameWndOnCreateResult;
}

void RECOIL_FASTCALL FakeCFrameWndOnClose(CFrameWnd *, void *) {
    ++g_cFrameWndOnCloseCalls;
}

void RECOIL_FASTCALL FakeCFrameWndOnActivate(CFrameWnd *, void *, unsigned int nState,
                                             CWnd *pWndOther, BOOL bMinimized) {
    ++g_cFrameWndOnActivateCalls;
    g_cFrameWndOnActivateArgsOk =
        (nState == 0 || nState == 1) && pWndOther == 0 &&
        (bMinimized == TRUE || bMinimized == FALSE);
    RecordOnActivateCall(1);
}

void RECOIL_FASTCALL FakeCFrameWndOnDestroy(CFrameWnd *, void *) {
    ++g_cFrameWndOnDestroyCalls;
    RecordOnDestroyCall(4);
}

BOOL RECOIL_FASTCALL FakeCGdiObjectDeleteObject(CGdiObject *, void *) {
    ++g_gdiDeleteObjectCalls;
    RecordOnDestroyCall(5);
    return TRUE;
}

HINSTANCE RECOIL_STDCALL FakeAfxFindResourceHandle(LPCSTR resourceName, LPCSTR resourceType) {
    ++g_findResourceHandleCalls;
    g_lastResourceName = resourceName;
    g_lastResourceType = resourceType;
    return g_fakeResourceHandle;
}

HBITMAP WINAPI FakeLoadBitmapA(HINSTANCE instance, LPCSTR bitmapName) {
    ++g_loadBitmapCalls;
    g_lastBitmapInstance = instance;
    g_lastBitmapName = bitmapName;
    return g_fakeBitmapHandle;
}

HDC WINAPI FakeCreateCompatibleDC(HDC hdc) {
    ++g_paintCreateCompatibleDcCalls;
    g_paintLastDestDc = hdc;
    return g_paintCompatibleDc;
}

HGDIOBJ WINAPI FakeSelectObject(HDC hdc, HGDIOBJ object) {
    if (hdc == g_paintCompatibleDc) {
        ++g_paintSelectObjectCalls;
    }
    g_paintLastSelectedObject = object;
    return object;
}

BOOL WINAPI FakeBitBlt(HDC destDc, int x, int y, int width, int height, HDC sourceDc, int sourceX,
                       int sourceY, DWORD rasterOp) {
    ++g_paintBitBltCalls;
    g_paintLastDestDc = destDc;
    g_paintLastSourceDc = sourceDc;
    g_paintLastDestRect = {x, y, x + width, y + height};
    g_paintLastSourceRect = {sourceX, sourceY, sourceX + width, sourceY + height};
    g_paintLastRasterOp = rasterOp;
    return TRUE;
}

BOOL WINAPI FakeStretchBlt(HDC destDc, int x, int y, int width, int height, HDC sourceDc,
                           int sourceX, int sourceY, int sourceWidth, int sourceHeight,
                           DWORD rasterOp) {
    ++g_paintStretchBltCalls;
    g_paintLastDestDc = destDc;
    g_paintLastSourceDc = sourceDc;
    g_paintLastDestRect = {x, y, x + width, y + height};
    g_paintLastSourceRect = {sourceX, sourceY, sourceX + sourceWidth, sourceY + sourceHeight};
    g_paintLastRasterOp = rasterOp;
    return TRUE;
}

BOOL WINAPI FakeDeleteDC(HDC hdc) {
    if (hdc == g_paintCompatibleDc) {
        ++g_paintDeleteDcCalls;
    }
    return TRUE;
}

void RECOIL_FASTCALL FakeCFrameWndOnSize(CFrameWnd *, void *, unsigned int nType, int cx, int cy) {
    ++g_cFrameWndOnSizeCalls;
    g_cFrameWndOnSizeArgsOk = nType == 0 || nType == 1 || nType == 4;
    g_cFrameWndOnSizeArgsOk = g_cFrameWndOnSizeArgsOk && cx == 640 && cy == 480;
}

void *CFrameWndOnCloseProc() {
    union MemberToFunction {
        void (RECOIL_THISCALL CFrameWndOnCloseAccess::*member)();
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &CFrameWndOnCloseAccess::OnClose;
    return thunk.function;
}

void *CFrameWndOnActivateProc() {
    union MemberToFunction {
        void (RECOIL_THISCALL CFrameWndOnActivateAccess::*member)(unsigned int, CWnd *, BOOL);
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &CFrameWndOnActivateAccess::OnActivate;
    return thunk.function;
}

void *CFrameWndOnCreateProc() {
    union MemberToFunction {
        int (RECOIL_THISCALL CFrameWndOnCreateAccess::*member)(CREATESTRUCTA *);
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &CFrameWndOnCreateAccess::OnCreate;
    return thunk.function;
}

void *CFrameWndOnDestroyProc() {
    union MemberToFunction {
        void (RECOIL_THISCALL CFrameWndOnDestroyAccess::*member)();
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &CFrameWndOnDestroyAccess::OnDestroy;
    return thunk.function;
}

void *CGdiObjectDeleteObjectProc() {
    union MemberToFunction {
        BOOL (RECOIL_THISCALL CGdiObject::*member)();
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &CGdiObject::DeleteObject;
    return thunk.function;
}

void *CFrameWndOnSizeProc() {
    union MemberToFunction {
        void (RECOIL_THISCALL CFrameWndOnSizeAccess::*member)(unsigned int, int, int);
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &CFrameWndOnSizeAccess::OnSize;
    return thunk.function;
}

bool PatchImportByName(const char *dllName, const char *functionName, void *replacement,
                       ImportFunctionPatch &patch) {
    HMODULE module = GetModuleHandleA(nullptr);
    auto *base = reinterpret_cast<unsigned char *>(module);
    auto *dos = reinterpret_cast<IMAGE_DOS_HEADER *>(base);
    auto *nt = reinterpret_cast<IMAGE_NT_HEADERS *>(base + dos->e_lfanew);
    const IMAGE_DATA_DIRECTORY &directory =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    auto *descriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(base + directory.VirtualAddress);

    for (; descriptor->Name != 0; ++descriptor) {
        const char *importDll = reinterpret_cast<const char *>(base + descriptor->Name);
        if (_stricmp(importDll, dllName) != 0) {
            continue;
        }

        auto *names = reinterpret_cast<IMAGE_THUNK_DATA *>(base + descriptor->OriginalFirstThunk);
        auto *thunks = reinterpret_cast<IMAGE_THUNK_DATA *>(base + descriptor->FirstThunk);
        for (; names->u1.AddressOfData != 0; ++names, ++thunks) {
            if ((names->u1.Ordinal & IMAGE_ORDINAL_FLAG) != 0) {
                continue;
            }

            auto *importName = reinterpret_cast<IMAGE_IMPORT_BY_NAME *>(
                base + names->u1.AddressOfData);
            if (std::strcmp(reinterpret_cast<const char *>(importName->Name), functionName) != 0) {
                continue;
            }

            patch.slot = reinterpret_cast<ULONG_PTR *>(&thunks->u1.Function);
            patch.original = *patch.slot;
            DWORD oldProtect = 0;
            if (VirtualProtect(patch.slot, sizeof(*patch.slot), PAGE_EXECUTE_READWRITE,
                               &oldProtect) == 0) {
                patch.slot = nullptr;
                return false;
            }

            *patch.slot = reinterpret_cast<ULONG_PTR>(replacement);
            DWORD ignored = 0;
            VirtualProtect(patch.slot, sizeof(*patch.slot), oldProtect, &ignored);
            FlushInstructionCache(GetCurrentProcess(), patch.slot, sizeof(*patch.slot));
            return true;
        }
    }

    return false;
}

bool PatchFunctionJump(void *target, void *replacement, CodeFunctionPatch &patch) {
    patch.address = static_cast<unsigned char *>(target);
    std::memcpy(patch.original, patch.address, sizeof(patch.original));

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.address, sizeof(patch.original), PAGE_EXECUTE_READWRITE,
                       &oldProtect) == 0) {
        return false;
    }

    patch.address[0] = 0xe9;
    const auto delta = static_cast<std::intptr_t>(reinterpret_cast<unsigned char *>(replacement) -
                                                 (patch.address + sizeof(patch.original)));
    const auto relative = static_cast<std::int32_t>(delta);
    std::memcpy(patch.address + 1, &relative, sizeof(relative));
    FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));

    DWORD ignored = 0;
    VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
    return true;
}

void RestoreImportPatch(ImportFunctionPatch &patch) {
    if (patch.slot == nullptr) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.slot, sizeof(*patch.slot), PAGE_EXECUTE_READWRITE, &oldProtect) !=
        0) {
        *patch.slot = patch.original;
        DWORD ignored = 0;
        VirtualProtect(patch.slot, sizeof(*patch.slot), oldProtect, &ignored);
        FlushInstructionCache(GetCurrentProcess(), patch.slot, sizeof(*patch.slot));
    }

    patch.slot = nullptr;
    patch.original = 0;
}

void RestoreFunctionPatch(CodeFunctionPatch &patch) {
    if (patch.address == nullptr) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.address, sizeof(patch.original), PAGE_EXECUTE_READWRITE,
                       &oldProtect) != 0) {
        std::memcpy(patch.address, patch.original, sizeof(patch.original));
        FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
        DWORD ignored = 0;
        VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
    }
    patch.address = nullptr;
}
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

extern "C" int czgame_frame_on_close_smoke(void) {
    CodeFunctionPatch frameWndOnClosePatch{};
    if (!PatchFunctionJump(CFrameWndOnCloseProc(), reinterpret_cast<void *>(&FakeCFrameWndOnClose),
                           frameWndOnClosePatch)) {
        return 1;
    }

    CZGameFrame frame{};
    g_cFrameWndOnCloseCalls = 0;
    frame.OnClose();
    const bool ok = g_cFrameWndOnCloseCalls == 1;

    RestoreFunctionPatch(frameWndOnClosePatch);
    return ok ? 0 : 2;
}

extern "C" int czgame_frame_on_create_smoke(void) {
    CodeFunctionPatch frameWndOnCreatePatch{};
    CodeFunctionPatch findResourcePatch{};
    ImportFunctionPatch loadBitmapPatch{};
    if (!PatchFunctionJump(CFrameWndOnCreateProc(),
                           reinterpret_cast<void *>(&FakeCFrameWndOnCreate),
                           frameWndOnCreatePatch) ||
        !PatchFunctionJump(reinterpret_cast<void *>(&AfxFindResourceHandle),
                           reinterpret_cast<void *>(&FakeAfxFindResourceHandle),
                           findResourcePatch) ||
        !PatchImportByName("USER32.dll", "LoadBitmapA", reinterpret_cast<void *>(&FakeLoadBitmapA),
                           loadBitmapPatch)) {
        RestoreImportPatch(loadBitmapPatch);
        RestoreFunctionPatch(findResourcePatch);
        RestoreFunctionPatch(frameWndOnCreatePatch);
        return 1;
    }

    CZGameFrame frame{};
    CREATESTRUCTA createStruct{};
    g_cFrameWndOnCreateCalls = 0;
    g_findResourceHandleCalls = 0;
    g_loadBitmapCalls = 0;
    g_zInput_MouseDevice = nullptr;
    g_zInput_MouseInitialized = 1;
    g_zInput_MouseActive = 1;
    g_cFrameWndOnCreateResult = -1;

    if (frame.OnCreate(&createStruct) != -1 || g_cFrameWndOnCreateCalls != 1 ||
        g_findResourceHandleCalls != 0 || g_loadBitmapCalls != 0 ||
        g_zInput_MouseInitialized != 1 || g_zInput_MouseActive != 1) {
        RestoreImportPatch(loadBitmapPatch);
        RestoreFunctionPatch(findResourcePatch);
        RestoreFunctionPatch(frameWndOnCreatePatch);
        return 2;
    }

    g_cFrameWndOnCreateResult = 0;
    g_cFrameWndOnCreateCalls = 0;
    g_findResourceHandleCalls = 0;
    g_loadBitmapCalls = 0;
    g_lastResourceName = nullptr;
    g_lastResourceType = nullptr;
    g_lastBitmapName = nullptr;
    g_lastBitmapInstance = nullptr;
    g_zInput_MouseInitialized = 1;
    g_zInput_MouseActive = 1;
    frame.m_gameBitmap.m_hObject = nullptr;

    const int result = frame.OnCreate(&createStruct);
    const bool ok = result == 0 && g_cFrameWndOnCreateCalls == 1 &&
                    g_findResourceHandleCalls == 1 && g_loadBitmapCalls == 1 &&
                    std::strcmp(g_lastResourceName, "GAMEBMP") == 0 &&
                    g_lastResourceType == MAKEINTRESOURCEA(2) &&
                    g_lastBitmapInstance == g_fakeResourceHandle &&
                    std::strcmp(g_lastBitmapName, "GAMEBMP") == 0 &&
                    frame.m_gameBitmap.m_hObject == g_fakeBitmapHandle &&
                    g_zInput_MouseInitialized == 0 && g_zInput_MouseActive == 0;

    frame.m_gameBitmap.m_hObject = nullptr;
    RestoreImportPatch(loadBitmapPatch);
    RestoreFunctionPatch(findResourcePatch);
    RestoreFunctionPatch(frameWndOnCreatePatch);
    return ok ? 0 : 3;
}

extern "C" int czgame_frame_on_destroy_smoke(void) {
    CodeFunctionPatch networkPatch{};
    CodeFunctionPatch videoPatch{};
    CodeFunctionPatch cdStopPatch{};
    CodeFunctionPatch frameDestroyPatch{};
    CodeFunctionPatch bitmapDeletePatch{};

    if (!PatchFunctionJump(reinterpret_cast<void *>(&zNetwork_DPlay_DestroyCachedLocalPlayer),
                           reinterpret_cast<void *>(&FakeDestroyCachedLocalPlayer),
                           networkPatch) ||
        !PatchFunctionJump(reinterpret_cast<void *>(&zVideo::ShutdownVideoSystem),
                           reinterpret_cast<void *>(&FakeShutdownVideoSystem), videoPatch) ||
        !PatchFunctionJump(reinterpret_cast<void *>(&zSndCd::Stop),
                           reinterpret_cast<void *>(&FakeZsndCdStop), cdStopPatch) ||
        !PatchFunctionJump(CFrameWndOnDestroyProc(),
                           reinterpret_cast<void *>(&FakeCFrameWndOnDestroy),
                           frameDestroyPatch) ||
        !PatchFunctionJump(CGdiObjectDeleteObjectProc(),
                           reinterpret_cast<void *>(&FakeCGdiObjectDeleteObject),
                           bitmapDeletePatch)) {
        RestoreFunctionPatch(bitmapDeletePatch);
        RestoreFunctionPatch(frameDestroyPatch);
        RestoreFunctionPatch(cdStopPatch);
        RestoreFunctionPatch(videoPatch);
        RestoreFunctionPatch(networkPatch);
        return 1;
    }

    CZGameFrame frame{};
    g_cFrameWndOnDestroyCalls = 0;
    g_gdiDeleteObjectCalls = 0;
    g_onDestroyCallCount = 0;
    for (int i = 0; i < 5; ++i) {
        g_onDestroyCallOrder[i] = 0;
    }

    frame.OnDestroy();
    const bool ok = g_onDestroyCallCount == 5 && g_onDestroyCallOrder[0] == 1 &&
                    g_onDestroyCallOrder[1] == 2 && g_onDestroyCallOrder[2] == 3 &&
                    g_onDestroyCallOrder[3] == 4 && g_onDestroyCallOrder[4] == 5 &&
                    g_cFrameWndOnDestroyCalls == 1 && g_gdiDeleteObjectCalls == 1;

    RestoreFunctionPatch(bitmapDeletePatch);
    RestoreFunctionPatch(frameDestroyPatch);
    RestoreFunctionPatch(cdStopPatch);
    RestoreFunctionPatch(videoPatch);
    RestoreFunctionPatch(networkPatch);
    return ok ? 0 : 2;
}

extern "C" int czgame_frame_on_activate_smoke(void) {
    union ActivateMemberToFunction {
        void (RECOIL_THISCALL FakeGameFrameApp::*member)();
        void(RECOIL_THISCALL *function)(CZGameFrameApp *);
    };

    union StateWndActivateMemberToFn {
        void (RECOIL_THISCALL FakeActivateState::*member)(std::uint32_t);
        RecoilFn32 fn;
    };

    ActivateMemberToFunction activateThunk{};
    activateThunk.member = &FakeGameFrameApp::OnActivate;
    ActivateMemberToFunction deactivateThunk{};
    deactivateThunk.member = &FakeGameFrameApp::OnDeactivate;
    StateWndActivateMemberToFn stateThunk{};
    stateThunk.member = &FakeActivateState::OnWndActivate;

    CZGameFrameAppVtable appVtable{};
    appVtable.OnAppActivate = activateThunk.function;
    appVtable.OnAppDeactivate = deactivateThunk.function;

    RecoilApp_IState_Vtbl stateVtable{};
    stateVtable.OnWndActivate = stateThunk.fn;
    FakeActivateState state{};
    state.vftable = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&stateVtable));

    RecoilApp app{};
    app.vftable = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&appVtable));
    app.m_currentStateIndex_0c8 = 0;
    app.m_stateStack_0d8[0] =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&state));

    CZGameFrame frame{};
    frame.m_app = reinterpret_cast<CZGameFrameApp *>(&app);

    CodeFunctionPatch frameWndOnActivatePatch{};
    CodeFunctionPatch zInputActivatePatch{};
    CodeFunctionPatch zInputDeactivatePatch{};
    CodeFunctionPatch zGameReturnPatch{};
    CodeFunctionPatch zVideoRestorePatch{};

    if (!PatchFunctionJump(CFrameWndOnActivateProc(),
                           reinterpret_cast<void *>(&FakeCFrameWndOnActivate),
                           frameWndOnActivatePatch) ||
        !PatchFunctionJump(reinterpret_cast<void *>(&zInput::OnAppActivate),
                           reinterpret_cast<void *>(&FakeZInputOnAppActivate),
                           zInputActivatePatch) ||
        !PatchFunctionJump(reinterpret_cast<void *>(&zInput::OnAppDeactivate),
                           reinterpret_cast<void *>(&FakeZInputOnAppDeactivate),
                           zInputDeactivatePatch) ||
        !PatchFunctionJump(reinterpret_cast<void *>(&zGame::ReturnOnlyStub),
                           reinterpret_cast<void *>(&FakeZGameReturnOnlyStub),
                           zGameReturnPatch) ||
        !PatchFunctionJump(reinterpret_cast<void *>(&zVideo_RestoreIconicFullscreenWindowIfNeeded),
                           reinterpret_cast<void *>(
                               &FakeZVideoRestoreIconicFullscreenWindowIfNeeded),
                           zVideoRestorePatch)) {
        RestoreFunctionPatch(zVideoRestorePatch);
        RestoreFunctionPatch(zGameReturnPatch);
        RestoreFunctionPatch(zInputDeactivatePatch);
        RestoreFunctionPatch(zInputActivatePatch);
        RestoreFunctionPatch(frameWndOnActivatePatch);
        return 1;
    }

    g_appActivateCalls = 0;
    g_appDeactivateCalls = 0;
    g_cFrameWndOnActivateCalls = 0;
    g_cFrameWndOnActivateArgsOk = false;
    g_stateWndActivateCalls = 0;
    g_lastStateWndActivateValue = -1;
    g_zInputOnAppActivateCalls = 0;
    g_zInputOnAppDeactivateCalls = 0;
    g_zGameReturnOnlyStubCalls = 0;
    g_zVideoRestoreIconicCalls = 0;
    g_onActivateCallCount = 0;
    for (int i = 0; i < 5; ++i) {
        g_onActivateCallOrder[i] = 0;
    }

    frame.OnActivate(0, nullptr, FALSE);
    bool ok = g_onActivateCallCount == 5 && g_onActivateCallOrder[0] == 1 &&
              g_onActivateCallOrder[1] == 2 && g_onActivateCallOrder[2] == 3 &&
              g_onActivateCallOrder[3] == 4 && g_onActivateCallOrder[4] == 5 &&
              g_cFrameWndOnActivateCalls == 1 && g_cFrameWndOnActivateArgsOk &&
              g_stateWndActivateCalls == 1 && g_lastStateWndActivateValue == 0 &&
              g_appDeactivateCalls == 1 && g_zInputOnAppDeactivateCalls == 1 &&
              g_zGameReturnOnlyStubCalls == 1 && g_appActivateCalls == 0 &&
              g_zInputOnAppActivateCalls == 0 && g_zVideoRestoreIconicCalls == 0;
    if (!ok) {
        RestoreFunctionPatch(zVideoRestorePatch);
        RestoreFunctionPatch(zGameReturnPatch);
        RestoreFunctionPatch(zInputDeactivatePatch);
        RestoreFunctionPatch(zInputActivatePatch);
        RestoreFunctionPatch(frameWndOnActivatePatch);
        return 2;
    }

    g_appActivateCalls = 0;
    g_appDeactivateCalls = 0;
    g_cFrameWndOnActivateCalls = 0;
    g_cFrameWndOnActivateArgsOk = false;
    g_stateWndActivateCalls = 0;
    g_lastStateWndActivateValue = -1;
    g_zInputOnAppActivateCalls = 0;
    g_zInputOnAppDeactivateCalls = 0;
    g_zGameReturnOnlyStubCalls = 0;
    g_zVideoRestoreIconicCalls = 0;
    g_onActivateCallCount = 0;
    for (int i = 0; i < 5; ++i) {
        g_onActivateCallOrder[i] = 0;
    }

    frame.OnActivate(1, nullptr, TRUE);
    ok = g_onActivateCallCount == 5 && g_onActivateCallOrder[0] == 1 &&
         g_onActivateCallOrder[1] == 2 && g_onActivateCallOrder[2] == 6 &&
         g_onActivateCallOrder[3] == 7 && g_onActivateCallOrder[4] == 8 &&
         g_cFrameWndOnActivateCalls == 1 && g_cFrameWndOnActivateArgsOk &&
         g_stateWndActivateCalls == 1 && g_lastStateWndActivateValue == 1 &&
         g_appActivateCalls == 1 && g_zInputOnAppActivateCalls == 1 &&
         g_zVideoRestoreIconicCalls == 1 && g_appDeactivateCalls == 0 &&
         g_zInputOnAppDeactivateCalls == 0 && g_zGameReturnOnlyStubCalls == 0;

    RestoreFunctionPatch(zVideoRestorePatch);
    RestoreFunctionPatch(zGameReturnPatch);
    RestoreFunctionPatch(zInputDeactivatePatch);
    RestoreFunctionPatch(zInputActivatePatch);
    RestoreFunctionPatch(frameWndOnActivatePatch);
    return ok ? 0 : 3;
}

extern "C" int czgame_frame_on_paint_smoke(void) {
    ImportFunctionPatch createDcPatch{};
    ImportFunctionPatch selectObjectPatch{};
    ImportFunctionPatch bitBltPatch{};
    ImportFunctionPatch stretchBltPatch{};
    ImportFunctionPatch deleteDcPatch{};

    if (!PatchImportByName("GDI32.dll", "CreateCompatibleDC",
                           reinterpret_cast<void *>(&FakeCreateCompatibleDC), createDcPatch) ||
        !PatchImportByName("GDI32.dll", "SelectObject",
                           reinterpret_cast<void *>(&FakeSelectObject), selectObjectPatch) ||
        !PatchImportByName("GDI32.dll", "BitBlt", reinterpret_cast<void *>(&FakeBitBlt),
                           bitBltPatch) ||
        !PatchImportByName("GDI32.dll", "StretchBlt",
                           reinterpret_cast<void *>(&FakeStretchBlt), stretchBltPatch) ||
        !PatchImportByName("GDI32.dll", "DeleteDC", reinterpret_cast<void *>(&FakeDeleteDC),
                           deleteDcPatch)) {
        RestoreImportPatch(deleteDcPatch);
        RestoreImportPatch(stretchBltPatch);
        RestoreImportPatch(bitBltPatch);
        RestoreImportPatch(selectObjectPatch);
        RestoreImportPatch(createDcPatch);
        return 1;
    }

    HWND hwnd = CreateWindowExA(0, "STATIC", "recoil-paint-test", WS_POPUP, 0, 0, 400, 300,
                                nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
    if (hwnd == nullptr) {
        RestoreImportPatch(deleteDcPatch);
        RestoreImportPatch(stretchBltPatch);
        RestoreImportPatch(bitBltPatch);
        RestoreImportPatch(selectObjectPatch);
        RestoreImportPatch(createDcPatch);
        return 2;
    }

    ShowWindow(hwnd, SW_SHOWNOACTIVATE);

    CZRecoilFrame frame{};
    frame.m_hWnd = hwnd;
    auto *gameFrame = reinterpret_cast<CZGameFrame *>(&frame);
    gameFrame->m_gameBitmap.m_hObject =
        reinterpret_cast<HGDIOBJ>(static_cast<std::uintptr_t>(0x2468));

    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const int oldUpdateMask = g_zVid_CachedClientRectUpdateMask;
    g_zVideo_ActiveRendererPath = 0;
    g_zVid_CachedClientRectUpdateMask = 0;

    g_paintCreateCompatibleDcCalls = 0;
    g_paintSelectObjectCalls = 0;
    g_paintBitBltCalls = 0;
    g_paintStretchBltCalls = 0;
    g_paintDeleteDcCalls = 0;
    g_paintLastDestRect = {};
    g_paintLastSourceRect = {};
    g_paintLastRasterOp = 0;

    RECT smallPaint = {0, 0, 400, 300};
    InvalidateRect(hwnd, nullptr, FALSE);
    gameFrame->OnPaint();
    bool ok = g_paintCreateCompatibleDcCalls == 1 && g_paintSelectObjectCalls == 1 &&
              g_paintBitBltCalls == 1 && g_paintStretchBltCalls == 0 &&
              g_paintDeleteDcCalls == 1 && g_paintLastSourceDc == g_paintCompatibleDc &&
              g_paintLastSelectedObject == gameFrame->m_gameBitmap.m_hObject &&
              g_paintLastDestRect.left == smallPaint.left &&
              g_paintLastDestRect.top == smallPaint.top &&
              g_paintLastDestRect.right == smallPaint.right &&
              g_paintLastDestRect.bottom == smallPaint.bottom &&
              g_paintLastSourceRect.left == smallPaint.left &&
              g_paintLastSourceRect.top == smallPaint.top &&
              g_paintLastSourceRect.right == smallPaint.right &&
              g_paintLastSourceRect.bottom == smallPaint.bottom &&
              g_paintLastRasterOp == SRCCOPY;
    if (!ok) {
        int code = 10;
        if (g_paintCreateCompatibleDcCalls != 1) {
            code = 11;
        } else if (g_paintSelectObjectCalls != 1) {
            code = 12;
        } else if (g_paintBitBltCalls != 1) {
            code = 13;
        } else if (g_paintStretchBltCalls != 0) {
            code = 14;
        } else if (g_paintDeleteDcCalls != 1) {
            code = 15;
        } else if (g_paintLastSourceDc != g_paintCompatibleDc) {
            code = 16;
        } else if (g_paintLastSelectedObject != gameFrame->m_gameBitmap.m_hObject) {
            code = 17;
        } else if (g_paintLastDestRect.left != smallPaint.left) {
            code = 18;
        } else if (g_paintLastDestRect.top != smallPaint.top) {
            code = 19;
        } else if (g_paintLastDestRect.right != smallPaint.right) {
            code = 21;
        } else if (g_paintLastDestRect.bottom != smallPaint.bottom) {
            code = 22;
        } else if (g_paintLastSourceRect.left != smallPaint.left) {
            code = 23;
        } else if (g_paintLastSourceRect.top != smallPaint.top) {
            code = 24;
        } else if (g_paintLastSourceRect.right != smallPaint.right) {
            code = 25;
        } else if (g_paintLastSourceRect.bottom != smallPaint.bottom) {
            code = 26;
        } else if (g_paintLastRasterOp != SRCCOPY) {
            code = 27;
        }
        g_zVideo_ActiveRendererPath = oldRendererPath;
        g_zVid_CachedClientRectUpdateMask = oldUpdateMask;
        DestroyWindow(hwnd);
        RestoreImportPatch(deleteDcPatch);
        RestoreImportPatch(stretchBltPatch);
        RestoreImportPatch(bitBltPatch);
        RestoreImportPatch(selectObjectPatch);
        RestoreImportPatch(createDcPatch);
        return code;
    }

    SetWindowPos(hwnd, nullptr, 0, 0, 800, 700, SWP_NOZORDER | SWP_NOACTIVATE);
    RECT tallPaint = {0, 0, 800, 700};
    InvalidateRect(hwnd, nullptr, FALSE);
    gameFrame->OnPaint();
    ok = g_paintCreateCompatibleDcCalls == 2 && g_paintSelectObjectCalls == 2 &&
         g_paintBitBltCalls == 1 && g_paintStretchBltCalls == 1 &&
         g_paintDeleteDcCalls == 2 && g_paintLastSourceDc == g_paintCompatibleDc &&
         g_paintLastDestRect.left == tallPaint.left &&
         g_paintLastDestRect.top == tallPaint.top &&
         g_paintLastDestRect.right == tallPaint.right &&
         g_paintLastDestRect.bottom == tallPaint.bottom &&
         g_paintLastSourceRect.left == tallPaint.left &&
         g_paintLastSourceRect.top == tallPaint.top &&
         g_paintLastSourceRect.right == tallPaint.left + 640 &&
         g_paintLastSourceRect.bottom == tallPaint.top + 480 &&
         g_paintLastRasterOp == SRCCOPY;
    if (!ok) {
        g_zVideo_ActiveRendererPath = oldRendererPath;
        g_zVid_CachedClientRectUpdateMask = oldUpdateMask;
        DestroyWindow(hwnd);
        RestoreImportPatch(deleteDcPatch);
        RestoreImportPatch(stretchBltPatch);
        RestoreImportPatch(bitBltPatch);
        RestoreImportPatch(selectObjectPatch);
        RestoreImportPatch(createDcPatch);
        return 20;
    }

    g_zVideo_ActiveRendererPath = 2;
    g_zVid_CachedClientRectUpdateMask = 1;
    InvalidateRect(hwnd, nullptr, FALSE);
    gameFrame->OnPaint();
    ok = g_paintCreateCompatibleDcCalls == 2 && g_paintSelectObjectCalls == 2 &&
         g_paintBitBltCalls == 1 && g_paintStretchBltCalls == 1 && g_paintDeleteDcCalls == 2;

    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVid_CachedClientRectUpdateMask = oldUpdateMask;
    DestroyWindow(hwnd);
    RestoreImportPatch(deleteDcPatch);
    RestoreImportPatch(stretchBltPatch);
    RestoreImportPatch(bitBltPatch);
    RestoreImportPatch(selectObjectPatch);
    RestoreImportPatch(createDcPatch);
    return ok ? 0 : 3;
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

    CodeFunctionPatch frameWndOnSizePatch{};
    if (!PatchFunctionJump(CFrameWndOnSizeProc(), reinterpret_cast<void *>(&FakeCFrameWndOnSize),
                           frameWndOnSizePatch)) {
        return 6;
    }

    frame.m_hWnd = CreateWindowExA(0, "STATIC", "recoil-size-test", WS_OVERLAPPEDWINDOW, 0, 0, 100,
                                   100, nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
    if (frame.m_hWnd == nullptr) {
        RestoreFunctionPatch(frameWndOnSizePatch);
        return 4;
    }

    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const int oldUpdateMask = g_zVid_CachedClientRectUpdateMask;
    const HWND oldVideoHwnd = g_zVideo_hWnd;
    const RECT oldCachedRect = g_zVideo_CachedClientRectScreen;

    RECT client{};
    GetClientRect(frame.m_hWnd, &client);
    g_zVideo_hWnd = frame.m_hWnd;
    g_zVideo_ActiveRendererPath = 2;
    zVid::SetCachedClientRectUpdateMask(1);
    g_zVideo_CachedClientRectScreen = {7, 8, 9, 10};

    g_appDeactivateCalls = 0;
    g_cFrameWndOnSizeCalls = 0;
    g_cFrameWndOnSizeArgsOk = false;
    frame.OnSize(0, 640, 480);
    if (g_appDeactivateCalls != 0 || g_cFrameWndOnSizeCalls != 1 || !g_cFrameWndOnSizeArgsOk) {
        g_zVideo_ActiveRendererPath = oldRendererPath;
        g_zVid_CachedClientRectUpdateMask = oldUpdateMask;
        g_zVideo_hWnd = oldVideoHwnd;
        g_zVideo_CachedClientRectScreen = oldCachedRect;
        DestroyWindow(frame.m_hWnd);
        RestoreFunctionPatch(frameWndOnSizePatch);
        return 1;
    }

    const LONG cachedWidth =
        g_zVideo_CachedClientRectScreen.right - g_zVideo_CachedClientRectScreen.left;
    const LONG cachedHeight =
        g_zVideo_CachedClientRectScreen.bottom - g_zVideo_CachedClientRectScreen.top;
    if (cachedWidth != client.right - client.left ||
        cachedHeight != client.bottom - client.top) {
        g_zVideo_ActiveRendererPath = oldRendererPath;
        g_zVid_CachedClientRectUpdateMask = oldUpdateMask;
        g_zVideo_hWnd = oldVideoHwnd;
        g_zVideo_CachedClientRectScreen = oldCachedRect;
        DestroyWindow(frame.m_hWnd);
        RestoreFunctionPatch(frameWndOnSizePatch);
        return 5;
    }

    frame.OnSize(1, 640, 480);
    if (g_appDeactivateCalls != 1 || g_cFrameWndOnSizeCalls != 2) {
        g_zVideo_ActiveRendererPath = oldRendererPath;
        g_zVid_CachedClientRectUpdateMask = oldUpdateMask;
        g_zVideo_hWnd = oldVideoHwnd;
        g_zVideo_CachedClientRectScreen = oldCachedRect;
        DestroyWindow(frame.m_hWnd);
        RestoreFunctionPatch(frameWndOnSizePatch);
        return 2;
    }

    frame.OnSize(4, 640, 480);
    const bool ok = g_appDeactivateCalls == 2 && g_cFrameWndOnSizeCalls == 3;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVid_CachedClientRectUpdateMask = oldUpdateMask;
    g_zVideo_hWnd = oldVideoHwnd;
    g_zVideo_CachedClientRectScreen = oldCachedRect;
    DestroyWindow(frame.m_hWnd);
    RestoreFunctionPatch(frameWndOnSizePatch);
    return ok ? 0 : 3;
}
