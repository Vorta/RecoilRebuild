// Battlesport compilation unit between RecoilForce.cpp and RecoilNet.cpp, inferred
// from the retail object boundary [0x4301e0, 0x431bf0): CZRecoilFrame's runtime
// class, message map and 60.0f constant [0x4d0bf0, 0x4d1140) form their own
// .rdata run (RecoilNet.cpp pools another 60.0f at 0x4d12c4) and its .data
// strings [0x4dccf0, 0x4dcd88) follow RecoilApp.cpp's. Pooled constants do not
// separate it from RecoilForce.cpp; the two are kept apart by module. Original
// filename unresolved; RecoilFrame.cpp is a provisional name (2026-10-02).

#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/about.h"
#include "Battlesport/briefing.h"
#include "Battlesport/game_net.h"
#include "Battlesport/hud.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/hud_ui_net_exit_panel.h"
#include "Battlesport/net_ui.h"
#include "Battlesport/pickup.h"
#include "Battlesport/player.h"
#include "Battlesport/recoil_app.h"
#include "Battlesport/recoil_state_main_menu_transition.h"
#include "Battlesport/recoil_version.h"
#include "Battlesport/turret.h"
#include "Battlesport/wol_dialog.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "zimage.h"

#include <algorithm>
#include <commdlg.h>
#include <math.h>
#include <objbase.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" HINSTANCE g_RecoilApp_hInstance;
HINSTANCE __stdcall AfxFindResourceHandle(LPCSTR resourceName, LPCSTR resourceType);
extern "C" HWND g_RecoilApp_hWndMain;
// Retail reads RecoilApp.cpp's shared class-name pointer (0x4dcac0); this file has no copy.
extern const char* g_RecoilApp_WndClassNamePtr;

extern "C" {
/**
 *
 * Purpose: provide the constructor-owned registry path used to detect the
 * Westwood Online API install.
 */
extern const char g_CZRecoilFrame_WolApiRegKey[] = "Software\\Westwood\\WOLAPI\\4352";
/**
 *
 * Purpose: name the recovered frame menu resource loaded during construction.
 */
extern const char g_CZRecoilFrame_MainMenuResourceName[] = "MYMENU";
/**
 *
 * Purpose: name the error log initialized by the frame constructor.
 */
extern const char g_RecoilError_LogFileName[] = "recoil.err";
/**
 *
 * Purpose: preserve the constructor command-line sentinel tested with strncmp.
 */
extern const char g_CZRecoilFrame_NumericDigits[] = "1234567890";
/**
 *
 * Purpose: preserve the constructor command-line campaign-mode switch prefix.
 */
extern const char g_CZRecoilFrame_CmdCampaigns[] = "/campaigns";
/**
 *
 * Purpose: provide the CZGameFrame constructor log/base name passed by the
 * Recoil frame constructor.
 */
extern const char g_CZRecoilFrame_LogBaseName[] = "recoil";
/**
 *
 * Purpose: provide the default Recoil main-window title used by the frame UI.
 */
extern const char g_RecoilApp_WindowTitle[0x7] = "RECOIL";
/**
 *
 * Purpose: provide the 3Dfx renderer main-window title used by the frame UI.
 */
extern const char g_RecoilApp_WindowTitle3Dfx[0xe] = "RECOIL (3Dfx)";
/**
 *
 * Purpose: preserve the common-dialog default extension for campaign files.
 */
extern const char g_CZRecoilFrame_DefaultFileExt[0x3] = "gs";
/**
 *
 * Purpose: format the hardware accelerator command label shown in the frame UI.
 */
extern "C" char g_CZRecoilFrame_AcceleratorMenuLabelFmt[0x16] = "Accelerator - %s (%s)";
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.symbol-0x4f3efc
 * @recoil-artifact defines .data recoil:data:0x4f3efc: Symbol.
 *
 * Purpose: remember whether the Westwood Online registry key was found during
 * frame construction.
 */
int g_CZRecoilFrame_HasWolApi = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.symbol-0x4f3f04
 * @recoil-artifact defines .data recoil:data:0x4f3f04: Symbol.
 *
 * Purpose: gate the one-time Winsock2 prompt before launching the Westwood
 * Online upgrade flow.
 */
int g_CZRecoilFrame_WestwoodOnlineWinsockChecked = 0;
/**
 * g_HudSensorTracker Symbol.
 * Source model: zero-initialized explicit storage for the CZRecoilFrame-owned
 * global instance. HudSensorTracker::ConstructGlobal and ShutdownGlobal own
 * the typed lifetime for this storage.
 * Purpose: Owns the global HUD sensor tracker state used by mission flow,
 * map/objective rendering, network timer sync, and frame-level HUD updates.
 */
#undef g_HudSensorTracker
HudSensorTrackerStorage g_HudSensorTracker = { 0 };
#define g_HudSensorTracker (*(HudSensorTracker*)&g_HudSensorTracker)
}

namespace {
const UINT kMfcCommandUpdateCode = (UINT)-1;
const UINT kMfcMessageMapSigVoid = 12;
const UINT kMfcMessageMapSigVoidUIntIntInt = 17;
const UINT kMfcMessageMapSigCmdUi = 44;
const int kRendererBackend3dfx = 2;
const int kCmdUiDisabled = 1;
const int kCmdUiChecked = 8;
const int kNetworkOptionDisabled = 0;
const int kNetworkOptionEnabled = 1;
const int kFmvSkipEnabled = 1;
const int kMultiplayerMissionBase = 6;
const int kDefaultMultiplayerEventCode = 1;
const unsigned int kMaxDirectMultiplayerEventCode = 255;
const int kHudTimerAndFlagsSyncPacketType = 20;
const int kDispatchModeSession = 2;
const float kSecondsPerMinute = 60.0f;
const unsigned int kVidMem800x600Threshold = 0x2bf200;
const unsigned int kVidMem1024x768Threshold = 4718592;
const unsigned int kFullscreenMenuCommandId = 0x9c4e;
const DWORD kMainWindowStyle = 0x82ca0000;

/**
 * Original helper evidence: no standalone retail function; observed in
 * CZRecoilFrame video-mode command UI callers.
 * Purpose: return the MFC checked-state flag when a video mode is active.
 */
inline int CommandCheckedIfMode(int currentMode, int targetMode)
{
    return currentMode == targetMode ? kCmdUiChecked : 0;
}

/**
 * Original helper evidence: no standalone retail function; observed inline
 * in the CZRecoilFrame video-mode command UI update handlers.
 * Purpose: translate cached command state into CCmdUI enable/check calls.
 */
inline void UpdateCmdUiFromState(CCmdUI* cmdUi, const int& state)
{
    if (state == kCmdUiDisabled) {
        cmdUi->Enable(0);
        cmdUi->SetCheck(0);
        return;
    }

    cmdUi->Enable(1);
    if (state == kCmdUiChecked) {
        cmdUi->SetCheck(1);
    } else {
        cmdUi->SetCheck(0);
    }
}

} // namespace

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.czrecoilframe-dynamic-create
 * @recoil-artifact emits .text recoil:function:0x4301e0: CZRecoilFrame::CreateObject.
 * @recoil-artifact emits .text recoil:function:0x430240: CZRecoilFrame::GetRuntimeClass.
 * @recoil-artifact emits .rdata recoil:data:0x4d0bf0: g_CZRecoilFrame_RuntimeClass.
 * @recoil-artifact emits .data recoil:data:0x4dccf0: g_CZRecoilFrame_RuntimeClassName.
 *
 * Purpose: use the original VC5SP3 MFC dynamic-creation region to emit the
 * Recoil frame factory, virtual runtime-class accessor, runtime-class record,
 * and class-name string in their natural form.
 */
IMPLEMENT_DYNCREATE(CZRecoilFrame, CZGameFrame)

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-czrecoil-frame
 * @recoil-artifact defines .text recoil:function:0x430250: CZRecoilFrame::CZRecoilFrame.
 * @recoil-match source
 *
 * Purpose: construct the MFC-derived Recoil frame, including the menu, window,
 * launch options, renderer menu state, and Westwood Online availability.
 */
CZRecoilFrame::CZRecoilFrame()
    : CZGameFrame(g_CZRecoilFrame_LogBaseName)
{
    CreateEx(
        0x20000,
        g_RecoilApp_WndClassNamePtr,
        BuildWindowTitle(),
        kMainWindowStyle,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        (GetSystemMetrics(SM_CXFRAME) << 1) + 0x280,
        GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYMENU) + (GetSystemMetrics(SM_CYFRAME) << 1) + 0x1e0,
        0,
        0,
        0
    );

    m_cmdlineFlag = 1;
    m_campaignsOnlyMode = 0;
    char* commandLineCopy = _strdup(GetCommandLineA());
    for (char* token = strtok(commandLineCopy, " "); token != 0; token = strtok(0, " ")) {
        if (strncmp(token, g_CZRecoilFrame_CmdCampaigns, 4) == 0) {
            m_campaignsOnlyMode = 1;
        } else if (strncmp(token, g_CZRecoilFrame_NumericDigits, 4) == 0) {
            m_cmdlineFlag = 0;
        }
    }
    free(commandLineCopy);

    zError::InitOutputContext(m_hWnd, 0xe00, g_RecoilError_LogFileName);
    m_mainMenu.Attach(LoadMenuA(
        AfxFindResourceHandle(g_CZRecoilFrame_MainMenuResourceName, MAKEINTRESOURCEA(4)),
        g_CZRecoilFrame_MainMenuResourceName
    ));
    SetMenu(&m_mainMenu);

    if (m_campaignsOnlyMode == 0) {
        m_mainMenu.RemoveMenu(1, MF_BYPOSITION);
    } else {
        m_mainMenu.GetSubMenu(1)->RemoveMenu(0x9c6b, MF_BYCOMMAND);
        m_mainMenu.GetSubMenu(1)->RemoveMenu(0x9c7b, MF_BYCOMMAND);
    }

    m_mainMenu.GetSubMenu(2)->RemoveMenu(kFullscreenMenuCommandId, MF_BYCOMMAND);

    // Retail reads this->m_hWnd before the global hInstance copy.
    g_RecoilApp_hWndMain = m_hWnd;
    g_RecoilApp_hInstance = (HINSTANCE)((unsigned int)(g_RecoilApp.m_hInstance));

    CString formattedTitle;
    formattedTitle.Format("%s", (const char*)BuildWindowTitle());
    SetWindowTextA(formattedTitle);

    m_openZbdFilePath[0] = 0;
    m_useArchiveBanks = 1;
    m_hwApiCmdUiState[0] = 0;
    m_hwApiCmdUiState[1] = 0;
    m_hwApiCmdUiState[2] = 0;
    m_hwApiCmdUiState[3] = 0;
    m_hwApiMenuCommandIds[0] = 0x9c83;
    m_hwApiMenuCommandIds[1] = 0x9c72;
    m_hwApiMenuCommandIds[2] = 0x9c75;
    m_hwApiMenuCommandIds[3] = 0x9c76;

    if (zVid::GetTexturePackLoadState() != 0) {
        CheckMenuItem(m_mainMenu.m_hMenu, 0x9c7b, MF_CHECKED);
    } else {
        CheckMenuItem(m_mainMenu.m_hMenu, 0x9c7b, MF_UNCHECKED);
    }

    g_HudSensorTracker.missionFlags = m_useArchiveBanks;
    zSnd::SetUseArchiveBanksFlag(m_useArchiveBanks);
    m_acceptedD3DDeviceCount = zVid::GetAcceptedHardwareRendererCount();

    HKEY wolApiRegKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, g_CZRecoilFrame_WolApiRegKey, 0, KEY_READ, &wolApiRegKey) == ERROR_SUCCESS) {
        g_CZRecoilFrame_HasWolApi = 1;
        RegCloseKey(wolApiRegKey);
    }

    ((CWnd*)(this))->CenterWindow(0);
    SetCursor(LoadCursorA(0, IDC_ARROW));
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-destroy-czrecoil-frame
 * @recoil-artifact defines .text recoil:function:0x430610: CZRecoilFrame::~CZRecoilFrame.
 * @recoil-match byte
 *
 * Purpose: let compiler-emitted MFC member and CZGameFrame base teardown
 * destroy the owned menu through the CMenu provider.
 */
CZRecoilFrame::~CZRecoilFrame() { }

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-set-menu-bar-visibility
 * @recoil-artifact defines .text recoil:function:0x430680: CZRecoilFrame::SetMenuBarVisibility.
 * @recoil-match byte
 *
 * Purpose: attach or remove the recovered main menu and frame menu style.
 */
void CZRecoilFrame::SetMenuBarVisibility(int visible)
{
    LONG style = GetWindowLongA(m_hWnd, GWL_STYLE);
    CMenu* menu;
    if (visible != 0) {
        menu = &m_mainMenu;
        style |= (LONG)(0x82ca0000);
    } else {
        menu = 0;
        style &= (LONG)(0xfff7ffff);
    }

    SetWindowLongA(m_hWnd, GWL_STYLE, style);
    ::SetMenu(m_hWnd, menu->GetSafeHmenu());
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.czrecoilframe-message-map
 * @recoil-artifact emits .text recoil:function:0x4306e0: CZRecoilFrame::GetMessageMap.
 * @recoil-artifact emits .rdata recoil:data:0x4d0c08: g_CZRecoilFrame_MessageMap.
 * @recoil-artifact emits .rdata recoil:data:0x4d0c10: g_CZRecoilFrame_MessageEntries.
 *
 * Purpose: use the original VC5SP3 MFC message-map region to emit the Recoil
 * frame's virtual accessor, map record, and command entries in their natural
 * form.
 */
BEGIN_MESSAGE_MAP(CZRecoilFrame, CZGameFrame)
ON_COMMAND(0x68, OnMenuStartSinglePlayer)
ON_COMMAND(0x9c51, OnMenuOpenCampaign)
ON_COMMAND(0x65, OnOpenFileDialog)
ON_COMMAND(0x67, OnMenuExitGame)
ON_COMMAND(0x206, OnMenuSetVideoMode2)
ON_COMMAND(0x207, OnMenuSetVideoMode3)
ON_COMMAND(0x208, OnMenuSetVideoMode4)
ON_COMMAND(0x209, OnMenuSetVideoMode5)
ON_COMMAND(0x9c4f, OnMenuToggleHud)
ON_COMMAND(0x9c4e, OnMenuToggleFullscreen)
ON_COMMAND(0x6a, OnMenuOpenHelpDocs)
ON_COMMAND(0x6b, OnMenuAbout)
ON_COMMAND(0x9c53, OnMenuOpenMultiplayerSessionBrowser)
ON_COMMAND(0x9c55, OnMenuStartMultiplayer)
ON_COMMAND(0x9c56, OnMenuStartCampaignMode)
ON_COMMAND(0x9c57, OnMenuStartCampaignMode2)
ON_COMMAND(0x9c58, OnMenuStartCampaignMode3)
ON_COMMAND(0x9c59, OnMenuStartCampaignMode4)
ON_COMMAND(0x9c5a, OnMenuStartCampaignMode5)
ON_COMMAND(0x9c6b, OnMenuToggleArchiveBanks)
ON_COMMAND(0x9c7b, OnMenuToggleTexturePacks)
ON_COMMAND(0x210, OnMenuSetVideoMode7)
ON_COMMAND(0x9c71, OnMenuSetVideoMode6)
ON_UPDATE_COMMAND_UI(0x210, OnUpdateVideoMode7CmdUI)
ON_UPDATE_COMMAND_UI(0x206, OnUpdateVideoMode2CmdUI)
ON_UPDATE_COMMAND_UI(0x207, OnUpdateVideoMode3CmdUI)
ON_UPDATE_COMMAND_UI(0x208, OnUpdateVideoMode4CmdUI)
ON_UPDATE_COMMAND_UI(0x209, OnUpdateVideoMode5CmdUI)
ON_UPDATE_COMMAND_UI(0x9c71, OnUpdateVideoMode6CmdUI)
ON_COMMAND(0x9c83, OnMenuSelectHwApi0)
ON_COMMAND(0x9c72, OnMenuSelectHwApi1)
ON_COMMAND(0x9c75, OnMenuSelectHwApi2)
ON_COMMAND(0x9c76, OnMenuSelectHwApi3)
ON_UPDATE_COMMAND_UI(0x9c83, OnUpdateHwApi0CmdUI)
ON_UPDATE_COMMAND_UI(0x9c72, OnUpdateHwApi1CmdUI)
ON_UPDATE_COMMAND_UI(0x9c75, OnUpdateHwApi2CmdUI)
ON_UPDATE_COMMAND_UI(0x9c76, OnUpdateHwApi3CmdUI)
ON_UPDATE_COMMAND_UI(0x9c4e, OnUpdateFullscreenCmdUI)
ON_COMMAND(0x9c7c, OnMenuToggleCDAudio)
ON_UPDATE_COMMAND_UI(0x9c7c, OnUpdateCDAudioCmdUI)
ON_COMMAND(0x9c7d, OnMenuToggleJoystick)
ON_UPDATE_COMMAND_UI(0x9c7d, OnUpdateJoystickCmdUI)
ON_COMMAND(0x9c7e, OnMenuWestwoodOnlineUpgrade)
ON_UPDATE_COMMAND_UI(0x9c7f, OnUpdateAlwaysEnabledCmdUI)
ON_UPDATE_COMMAND_UI(0x9c81, OnUpdateAlwaysEnabledCmdUI)
ON_UPDATE_COMMAND_UI(0x9c84, OnUpdateAlwaysEnabledCmdUI)
ON_UPDATE_COMMAND_UI(0x9c7e, OnUpdateNoOpCmdUI)
ON_UPDATE_COMMAND_UI(0x9c4f, OnUpdateHudCmdUI)
ON_COMMAND(0x9c80, OnMenuSelectDirectSound)
ON_UPDATE_COMMAND_UI(0x9c80, OnUpdateDirectSoundCmdUI)
ON_COMMAND(0x9c82, OnMenuSelectA3D)
ON_UPDATE_COMMAND_UI(0x9c82, OnUpdateA3DCmdUI)
ON_UPDATE_COMMAND_UI(0x9c53, OnUpdateNoOpCmdUI)
ON_WM_SIZE()
END_MESSAGE_MAP()

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-build-window-title
 * @recoil-artifact defines .text recoil:function:0x4306f0: CZRecoilFrame::BuildWindowTitle.
 * @recoil-match byte
 *
 * Purpose: build the Recoil window title, including the 3Dfx renderer suffix.
 */
CString CZRecoilFrame::BuildWindowTitle()
{
    if (g_zVideo_ActiveRendererPath == kRendererBackend3dfx) {
        return CString(g_RecoilApp_WindowTitle3Dfx);
    }

    return CString(g_RecoilApp_WindowTitle);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-start-single-player
 * @recoil-artifact defines .text recoil:function:0x430740: CZRecoilFrame::OnMenuStartSinglePlayer.
 * @recoil-match byte
 *
 * Purpose: clear intro/mission FMV skips and start the default engine load.
 */
void CZRecoilFrame::OnMenuStartSinglePlayer()
{
    g_RecoilApp.m_skipIntroFmv = 0;
    g_RecoilApp.m_missionFmvState.m_skipMissionFmv = 0;
    g_RecoilApp.LoadZbdAndStartEngine();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-open-campaign
 * @recoil-artifact defines .text recoil:function:0x430760: CZRecoilFrame::OnMenuOpenCampaign.
 * @recoil-match byte
 *
 * Purpose: enter campaign-open flow with the intro FMV skipped.
 */
void CZRecoilFrame::OnMenuOpenCampaign()
{
    g_RecoilApp.m_skipIntroFmv = 1;
    OnOpenFileDialog();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-open-file-dialog
 * @recoil-artifact defines .text recoil:function:0x430770: CZRecoilFrame::OnOpenFileDialog.
 * @recoil-match byte
 *
 * Purpose: open a campaign ZBD file through the retail common dialog path and
 * launch the selected mission data.
 */
RECOIL_NO_GS void CZRecoilFrame::OnOpenFileDialog()
{
    // Retail copies the title buffer's initializer from a 2-byte all-zero .bss literal: a wide empty string.
    wchar_t fileTitle[0x80] = L"";
    char filter[0x100];
    const int filterLength = LoadStringA(g_RecoilApp_hInstance, 0xc8, filter, sizeof(filter));
    const char separator = filter[filterLength - 1];
    if (filter[0] != '\0') {
        for (char* cursor = filter; *cursor != '\0'; ++cursor) {
            if (*cursor == separator) {
                *cursor = '\0';
            }
        }
    }

    OPENFILENAMEA ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = 0x4c;
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = m_openZbdFilePath;
    ofn.nMaxFile = sizeof(m_openZbdFilePath);
    ofn.lpstrFileTitle = (char*)fileTitle;
    ofn.nMaxFileTitle = 0x200;
    // Retail stores these two already-zero fields again after the memset.
    ofn.lpstrTitle = 0;
    ofn.lpstrInitialDir = 0;
    ofn.lpstrDefExt = g_CZRecoilFrame_DefaultFileExt;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA((LPOPENFILENAMEA)(&ofn)) != 0) {
        strcpy(m_openZbdFilePath, ofn.lpstrFile);
        g_RecoilApp.LoadZbdAndSetupSensorTracker(0, m_openZbdFilePath, 1, 1);
    }

    ::InvalidateRect(m_hWnd, 0, TRUE);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-exit-game
 * @recoil-artifact defines .text recoil:function:0x4308a0: CZRecoilFrame::OnMenuExitGame.
 * @recoil-match byte
 *
 * Purpose: Posts a close request to the main Recoil frame.
 */
void CZRecoilFrame::OnMenuExitGame()
{
    ::PostMessageA(m_hWnd, WM_CLOSE, 0, 0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-configure-mode-feature-flags
 * @recoil-artifact defines .text recoil:function:0x4308c0: CZRecoilFrame::ConfigureModeFeatureFlags.
 * @recoil-match byte
 *
 * Purpose: cache menu command UI states for video modes based on acceleration
 * state and available video memory.
 */
void CZRecoilFrame::ConfigureModeFeatureFlags()
{
    const int mode = zVid::GetVideoModeIndexFromOptions();

    if (zVid::GetAccelerationOption() != 0) {
        m_videoModeCmdUiState[0] = kCmdUiDisabled;
        m_videoModeCmdUiState[1] = kCmdUiDisabled;
        m_videoModeCmdUiState[2] = CommandCheckedIfMode(mode, 4);
        m_videoModeCmdUiState[3] = CommandCheckedIfMode(mode, 5);

        if (m_vidMemFreeBytes > kVidMem800x600Threshold) {
            m_videoModeCmdUiState[4] = CommandCheckedIfMode(mode, 6);
        } else {
            m_videoModeCmdUiState[4] = kCmdUiDisabled;
        }

        if (m_vidMemFreeBytes > kVidMem1024x768Threshold) {
            m_videoModeCmdUiState[5] = CommandCheckedIfMode(mode, 7);
        } else {
            m_videoModeCmdUiState[5] = kCmdUiDisabled;
        }
    } else {
        m_videoModeCmdUiState[0] = CommandCheckedIfMode(mode, 2);
        m_videoModeCmdUiState[1] = CommandCheckedIfMode(mode, 3);
        m_videoModeCmdUiState[2] = CommandCheckedIfMode(mode, 4);
        m_videoModeCmdUiState[3] = CommandCheckedIfMode(mode, 5);
        m_videoModeCmdUiState[4] = CommandCheckedIfMode(mode, 6);
        m_videoModeCmdUiState[5] = CommandCheckedIfMode(mode, 7);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-set-video-mode2
 * @recoil-artifact defines .text recoil:function:0x4309b0: CZRecoilFrame::OnMenuSetVideoMode2.
 * @recoil-match byte
 *
 * Purpose: set video mode 2 and refresh the recovered mode command state.
 */
void CZRecoilFrame::OnMenuSetVideoMode2()
{
    zVid::SetVideoModeIndex(2);
    ConfigureModeFeatureFlags();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-set-video-mode3
 * @recoil-artifact defines .text recoil:function:0x4309d0: CZRecoilFrame::OnMenuSetVideoMode3.
 * @recoil-match byte
 *
 * Purpose: set video mode 3 and refresh the recovered mode command state.
 */
void CZRecoilFrame::OnMenuSetVideoMode3()
{
    zVid::SetVideoModeIndex(3);
    ConfigureModeFeatureFlags();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-set-video-mode4
 * @recoil-artifact defines .text recoil:function:0x4309f0: CZRecoilFrame::OnMenuSetVideoMode4.
 * @recoil-match byte
 *
 * Purpose: set video mode 4 and refresh the recovered mode command state.
 */
void CZRecoilFrame::OnMenuSetVideoMode4()
{
    zVid::SetVideoModeIndex(4);
    ConfigureModeFeatureFlags();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-set-video-mode5
 * @recoil-artifact defines .text recoil:function:0x430a10: CZRecoilFrame::OnMenuSetVideoMode5.
 * @recoil-match byte
 *
 * Purpose: set video mode 5 and refresh the recovered mode command state.
 */
void CZRecoilFrame::OnMenuSetVideoMode5()
{
    zVid::SetVideoModeIndex(5);
    ConfigureModeFeatureFlags();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-set-video-mode6
 * @recoil-artifact defines .text recoil:function:0x430a30: CZRecoilFrame::OnMenuSetVideoMode6.
 * @recoil-match byte
 *
 * Purpose: set video mode 6 and refresh the recovered mode command state.
 */
void CZRecoilFrame::OnMenuSetVideoMode6()
{
    zVid::SetVideoModeIndex(6);
    ConfigureModeFeatureFlags();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-set-video-mode7
 * @recoil-artifact defines .text recoil:function:0x430a50: CZRecoilFrame::OnMenuSetVideoMode7.
 * @recoil-match byte
 *
 * Purpose: set video mode 7 and refresh the recovered mode command state.
 */
void CZRecoilFrame::OnMenuSetVideoMode7()
{
    zVid::SetVideoModeIndex(7);
    ConfigureModeFeatureFlags();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-toggle-hud
 * @recoil-artifact defines .text recoil:function:0x430a70: CZRecoilFrame::OnMenuToggleHud.
 * @recoil-match byte
 *
 * Purpose: toggle the HUD visibility option from the frame menu.
 */
void CZRecoilFrame::OnMenuToggleHud()
{
    zOpt::SetHudVisibilityOption(zOpt::GetHudVisibilityOption() == 0 ? 1 : 0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-update-hud-cmd-ui
 * @recoil-artifact defines .text recoil:function:0x430a90: CZRecoilFrame::OnUpdateHudCmdUI.
 * @recoil-match byte
 *
 * Purpose: enable and check the HUD command from the current option state.
 */
void CZRecoilFrame::OnUpdateHudCmdUI(CCmdUI* cmdUi)
{
    cmdUi->Enable(1);
    cmdUi->SetCheck(zOpt::GetHudVisibilityOption());
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-toggle-fullscreen
 * @recoil-artifact defines .text recoil:function:0x430ab0: CZRecoilFrame::OnMenuToggleFullscreen.
 * @recoil-match byte
 *
 * Purpose: toggle the fullscreen option from the frame menu.
 */
void CZRecoilFrame::OnMenuToggleFullscreen()
{
    if (zOpt::GetFullscreenOption() != 0) {
        zOpt::SetFullscreenOption(0);
        return;
    }
    zOpt::SetFullscreenOption(1);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-open-help-docs
 * @recoil-artifact defines .text recoil:function:0x430ad0: CZRecoilFrame::OnMenuOpenHelpDocs.
 * @recoil-match byte
 *
 * Purpose: open the retail help index or report the associated shell error.
 */
RECOIL_NO_GS void CZRecoilFrame::OnMenuOpenHelpDocs()
{
    char associatedExecutablePath[0x100];
    HINSTANCE findResult = FindExecutableA("Docs\\Index.html", 0, associatedExecutablePath);

    char messageBoxTitle[0x80];
    strcpy(messageBoxTitle, zLoc::GetMessageString(0x19));

    switch ((UINT)((UINT_PTR)(findResult))) {
    case 0:
        ((CWnd*)(this))->MessageBoxA(zLoc::GetMessageString(0x20), messageBoxTitle, 0x30);
        return;

    case SE_ERR_NOASSOC:
        ((CWnd*)(this))->MessageBoxA(zLoc::GetMessageString(0x21), messageBoxTitle, 0x30);
        return;

    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
        ((CWnd*)(this))->MessageBoxA(zLoc::GetMessageString(0x22), messageBoxTitle, 0x30);
        return;

    case ERROR_BAD_FORMAT:
        ((CWnd*)(this))->MessageBoxA(zLoc::GetMessageString(0x24), messageBoxTitle, 0x30);
        return;
    }

    ShellExecuteA(g_RecoilApp_hWndMain, "open", "Docs\\Index.html", 0, 0, SW_HIDE);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-about
 * @recoil-artifact defines .text recoil:function:0x430c30: CZRecoilFrame::OnMenuAbout.
 * @recoil-match byte
 *
 * Purpose: display the recovered About dialog through the frame menu.
 */
RECOIL_NO_GS void CZRecoilFrame::OnMenuAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.CDialog::DoModal();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.recoil-app-fatal-error-and-exit
 * @recoil-artifact defines .text recoil:function:0x430c90: RecoilApp::FatalErrorAndExit.
 * @recoil-match byte
 *
 * Purpose: Presents the fatal startup error and terminates the application.
 */
RECOIL_NO_GS void __fastcall RecoilApp::FatalErrorAndExit(int errorCode)
{
    if (errorCode != -1) {
        return;
    }

    char caption[0x80];
    char text[0x80];
    strcpy(caption, zLoc::GetMessageString(0x12));
    strcpy(text, zLoc::GetMessageString(0x30));

    Briefing::StopAndShutdownThread(0);
    zVideo_dd::FlipToGDIIfAttached();
    zSndSystem::Shutdown();
    zNetwork::ShutdownSessionRuntime();
    zVideo::ShutdownVideoSystem();
    printf("%s: %s\n", caption, text);
    Sleep(1000);
    MessageBeep(MB_ICONHAND);
    MessageBoxA(g_RecoilApp_hWndMain, text, caption, MB_ICONHAND);
    zSys::ExitProcessWithCleanup(0);
}

/**
 * Purpose: run the DirectPlay session browser/host setup flow and launch the
 * selected multiplayer mission state.
 */
void CZRecoilFrame::OnMenuOpenMultiplayerSessionBrowser()
{
    const HRESULT initResult = CoInitialize(0);
    if (SUCCEEDED(initResult)) {
        NetSessionBrowserDialog browserDialog(0);
        NetSessionConfigDialog configDialog(0);

        zNetwork::InitSessionRuntime(&g_zNetwork_RecoilAppGuid);
        zNetwork::SetFatalDisconnectCallback(&RecoilApp::FatalErrorAndExit);
        g_RecoilApp.m_skipIntroFmv = kFmvSkipEnabled;
        g_RecoilApp.m_missionFmvState.m_skipMissionFmv = kFmvSkipEnabled;

        if (browserDialog.CDialog::DoModal() == IDOK) {
            zOpt::SetPlayerName((const char*)(browserDialog.m_playerName));

            if (browserDialog.m_shouldEnterHostSetup == 0) {
                zNetworkSessionDescStatusFields statusFields;
                statusFields.selectedSessionIndex = browserDialog.m_selectedSessionIndex;

                if (zNetworkDPlay::OpenSelectedSessionAndReadStatusFields(&statusFields) != 0) {
                    zOpt::SetNetworkEnabled(kNetworkOptionEnabled);
                    zNetwork_DPlay::CreateLocalPlayerRecordAndRegister(
                        (char*)((const char*)(browserDialog.m_playerName))
                    );
                    zOpt::SetPlayerName((const char*)(browserDialog.m_playerName));

                    if ((unsigned int)(statusFields.eventCode) > kMaxDirectMultiplayerEventCode) {
                        g_RecoilApp.m_pendingState = &g_RecoilApp.m_mpExitDialogState;
                        statusFields.eventCode = kDefaultMultiplayerEventCode;
                        zNetwork::RegisterPacketHandler(
                            kHudTimerAndFlagsSyncPacketType,
                            (zNetworkPacketHandler)&GameNet::HandlePkt14HudTimerAndFlagsSync,
                            kDispatchModeSession
                        );
                    }

                    GameNet::SetStatusBitsFromFlags(statusFields.statusFlags);

                    // Retail converts the session time field as unsigned (fild qword).
                    g_HudSensorTracker.SetRuntimeTimerSecAndGoalValue(
                        (float)((unsigned int)(statusFields.valueOrTime)) * kSecondsPerMinute,
                        statusFields.auxParam
                    );

                    g_RecoilApp.LoadZbdAndSetupSensorTracker(
                        statusFields.eventCode + kMultiplayerMissionBase,
                        0,
                        kFmvSkipEnabled,
                        m_useArchiveBanks
                    );
                    return;
                }
            } else {
                zOpt::SetNetworkEnabled(kNetworkOptionEnabled);
                g_RecoilApp.LoadZbdAndStartEngine();
                HudUiNetGameSetupOverlayOwner::QueueEnterWithReconfigureFlag(0);
                return;
            }
        }
    }

    zNetwork::ShutdownSessionRuntime();
    zOpt::SetNetworkEnabled(kNetworkOptionDisabled);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-start-multiplayer
 * @recoil-artifact defines .text recoil:function:0x431270: CZRecoilFrame::OnMenuStartMultiplayer.
 * @recoil-match byte
 *
 * Purpose: start the default multiplayer mission setup path.
 */
void CZRecoilFrame::OnMenuStartMultiplayer()
{
    g_RecoilApp.LoadZbdAndSetupSensorTracker(1, 0, 1, m_useArchiveBanks);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-start-campaign-mode
 * @recoil-artifact defines .text recoil:function:0x431290: CZRecoilFrame::OnMenuStartCampaignMode.
 * @recoil-match byte
 *
 * Purpose: start campaign mission slot 2 with the current archive-bank flag.
 */
void CZRecoilFrame::OnMenuStartCampaignMode()
{
    g_RecoilApp.LoadZbdAndSetupSensorTracker(2, 0, 1, m_useArchiveBanks);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-start-campaign-mode2
 * @recoil-artifact defines .text recoil:function:0x4312b0: CZRecoilFrame::OnMenuStartCampaignMode2.
 * @recoil-match byte
 *
 * Purpose: start campaign mission slot 3 with the current archive-bank flag.
 */
void CZRecoilFrame::OnMenuStartCampaignMode2()
{
    g_RecoilApp.LoadZbdAndSetupSensorTracker(3, 0, 1, m_useArchiveBanks);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-start-campaign-mode3
 * @recoil-artifact defines .text recoil:function:0x4312d0: CZRecoilFrame::OnMenuStartCampaignMode3.
 * @recoil-match byte
 *
 * Purpose: start campaign mission slot 4 with the current archive-bank flag.
 */
void CZRecoilFrame::OnMenuStartCampaignMode3()
{
    g_RecoilApp.LoadZbdAndSetupSensorTracker(4, 0, 1, m_useArchiveBanks);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-start-campaign-mode4
 * @recoil-artifact defines .text recoil:function:0x4312f0: CZRecoilFrame::OnMenuStartCampaignMode4.
 * @recoil-match byte
 *
 * Purpose: start campaign mission slot 5 with the current archive-bank flag.
 */
void CZRecoilFrame::OnMenuStartCampaignMode4()
{
    g_RecoilApp.LoadZbdAndSetupSensorTracker(5, 0, 1, m_useArchiveBanks);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-start-campaign-mode5
 * @recoil-artifact defines .text recoil:function:0x431310: CZRecoilFrame::OnMenuStartCampaignMode5.
 * @recoil-match byte
 *
 * Purpose: start campaign mission slot 6 with the current archive-bank flag.
 */
void CZRecoilFrame::OnMenuStartCampaignMode5()
{
    g_RecoilApp.LoadZbdAndSetupSensorTracker(6, 0, 1, m_useArchiveBanks);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-toggle-archive-banks
 * @recoil-artifact defines .text recoil:function:0x431330: CZRecoilFrame::OnMenuToggleArchiveBanks.
 * @recoil-match byte
 *
 * Purpose: toggle archive-bank loading and mirror it into audio/HUD state.
 */
void CZRecoilFrame::OnMenuToggleArchiveBanks()
{
    m_useArchiveBanks = m_useArchiveBanks == 0 ? 1 : 0;
    CheckMenuItem(m_mainMenu.m_hMenu, 0x9c6b, m_useArchiveBanks == 0 ? MF_UNCHECKED : MF_CHECKED);
    const int useArchiveBanks = m_useArchiveBanks;
    g_HudSensorTracker.missionFlags = useArchiveBanks;
    zSnd::SetUseArchiveBanksFlag(m_useArchiveBanks);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-toggle-texture-packs
 * @recoil-artifact defines .text recoil:function:0x431380: CZRecoilFrame::OnMenuToggleTexturePacks.
 * @recoil-match byte
 *
 * Purpose: toggle texture-pack loading and update the menu check state.
 */
void CZRecoilFrame::OnMenuToggleTexturePacks()
{
    if (zVid::GetTexturePackLoadState() != 0) {
        zVid::SetTexturePackLoadState(0);
        CheckMenuItem(m_mainMenu.m_hMenu, 0x9c7b, MF_UNCHECKED);
        return;
    }

    zVid::SetTexturePackLoadState(1);
    CheckMenuItem(m_mainMenu.m_hMenu, 0x9c7b, MF_CHECKED);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-update-video-mode2-cmd-ui
 * @recoil-artifact defines .text recoil:function:0x4313d0: CZRecoilFrame::OnUpdateVideoMode2CmdUI.
 * @recoil-match byte
 *
 * Purpose: apply cached command UI state for video mode 2.
 */
void CZRecoilFrame::OnUpdateVideoMode2CmdUI(CCmdUI* cmdUi)
{
    UpdateCmdUiFromState(cmdUi, m_videoModeCmdUiState[0]);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-update-video-mode3-cmd-ui
 * @recoil-artifact defines .text recoil:function:0x431430: CZRecoilFrame::OnUpdateVideoMode3CmdUI.
 * @recoil-match byte
 *
 * Purpose: apply cached command UI state for video mode 3.
 */
void CZRecoilFrame::OnUpdateVideoMode3CmdUI(CCmdUI* cmdUi)
{
    UpdateCmdUiFromState(cmdUi, m_videoModeCmdUiState[1]);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-update-video-mode4-cmd-ui
 * @recoil-artifact defines .text recoil:function:0x431490: CZRecoilFrame::OnUpdateVideoMode4CmdUI.
 * @recoil-match byte
 *
 * Purpose: apply cached command UI state for video mode 4.
 */
void CZRecoilFrame::OnUpdateVideoMode4CmdUI(CCmdUI* cmdUi)
{
    UpdateCmdUiFromState(cmdUi, m_videoModeCmdUiState[2]);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-update-video-mode5-cmd-ui
 * @recoil-artifact defines .text recoil:function:0x4314f0: CZRecoilFrame::OnUpdateVideoMode5CmdUI.
 * @recoil-match byte
 *
 * Purpose: apply cached command UI state for video mode 5.
 */
void CZRecoilFrame::OnUpdateVideoMode5CmdUI(CCmdUI* cmdUi)
{
    UpdateCmdUiFromState(cmdUi, m_videoModeCmdUiState[3]);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-update-video-mode6-cmd-ui
 * @recoil-artifact defines .text recoil:function:0x431550: CZRecoilFrame::OnUpdateVideoMode6CmdUI.
 * @recoil-match byte
 *
 * Purpose: apply cached command UI state for video mode 6.
 */
void CZRecoilFrame::OnUpdateVideoMode6CmdUI(CCmdUI* cmdUi)
{
    UpdateCmdUiFromState(cmdUi, m_videoModeCmdUiState[4]);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-update-video-mode7-cmd-ui
 * @recoil-artifact defines .text recoil:function:0x4315b0: CZRecoilFrame::OnUpdateVideoMode7CmdUI.
 * @recoil-match byte
 *
 * Purpose: apply cached command UI state for video mode 7.
 */
void CZRecoilFrame::OnUpdateVideoMode7CmdUI(CCmdUI* cmdUi)
{
    UpdateCmdUiFromState(cmdUi, m_videoModeCmdUiState[5]);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-set-hw-api-and-init-mode
 * @recoil-artifact defines .text recoil:function:0x431610: CZRecoilFrame::SetHwApiAndInitMode.
 * @recoil-match byte
 *
 * Purpose: select a hardware API, query video memory, force accelerated mode, and enter the default hardware video
 * mode.
 */
void CZRecoilFrame::SetHwApiAndInitMode(int hwApiIndex)
{
    zVid::SetHwApiOption(zVideo::SelectHwApiDeviceOrFallback(hwApiIndex));
    g_zVideo_pfnQueryDeviceVideoMemoryBytes(hwApiIndex, &m_vidMemTotalBytes, (int*)(&m_vidMemFreeBytes));
    m_fullscreenOption = zOpt::GetFullscreenOption();
    zOpt::SetFullscreenOption(1);
    zVid::SetAccelerationOption(1);
    m_videoModeIndex = zVid::GetVideoModeIndexFromOptions();
    OnMenuSetVideoMode5();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-init-fallback-mode
 * @recoil-artifact defines .text recoil:function:0x431680: CZRecoilFrame::InitFallbackMode.
 * @recoil-match byte
 *
 * Purpose: restore software/fallback renderer options and rebuild mode command state.
 */
void CZRecoilFrame::InitFallbackMode()
{
    zVid::SetHwApiOption(zVideo::SelectHwApiDeviceOrFallback(-1));
    zVid::SetAccelerationOption(0);
    zOpt::SetFullscreenOption(m_fullscreenOption);
    zVid::SetVideoModeIndex(m_videoModeIndex);
    ConfigureModeFeatureFlags();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-ensure-hw-api-initialized
 * @recoil-artifact defines .text recoil:function:0x4316c0: CZRecoilFrame::EnsureHwApiInitialized.
 * @recoil-match byte
 *
 * Purpose: initialize the selected hardware API once and clear competing menu checks.
 */
void CZRecoilFrame::EnsureHwApiInitialized(int hwApiSelector)
{
    if (m_hwApiCmdUiState[hwApiSelector] != 0) {
        return;
    }

    if (hwApiSelector != 0) {
        (void)zVid::GetHwApiDescription(hwApiSelector - 1);
    }

    m_hwApiCmdUiState[hwApiSelector] = 8;
    if (hwApiSelector == 0) {
        InitFallbackMode();
    } else {
        SetHwApiAndInitMode(hwApiSelector - 1);
    }

    for (int i = 0; i < 4; ++i) {
        if (i != hwApiSelector) {
            m_hwApiCmdUiState[i] = 0;
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-init-startup-hw-api-from-options
 * @recoil-artifact defines .text recoil:function:0x431730: CZRecoilFrame::InitStartupHwApiFromOptions.
 * @recoil-match byte
 *
 * Purpose: select the startup renderer path from saved options or fallback defaults.
 */
void CZRecoilFrame::InitStartupHwApiFromOptions()
{
    if (zVid::GetHwApiOption() != 0) {
        const int acceptedDirectDrawDeviceCount = zVid::GetAcceptedDirectDrawDeviceCount();
        if (acceptedDirectDrawDeviceCount != 0) {
            m_hwApiCmdUiState[0] = 0;
            m_hwApiCmdUiState[acceptedDirectDrawDeviceCount] = 8;
            SetHwApiAndInitMode(acceptedDirectDrawDeviceCount - 1);
            return;
        }
    }

    m_hwApiCmdUiState[0] = 8;
    m_videoModeIndex = 5;
    m_fullscreenOption = zOpt::GetFullscreenOption();
    InitFallbackMode();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-select-hw-api0
 * @recoil-artifact defines .text recoil:function:0x431790: CZRecoilFrame::OnMenuSelectHwApi0.
 * @recoil-match byte
 *
 * Purpose: select the software/fallback hardware API menu path.
 */
void CZRecoilFrame::OnMenuSelectHwApi0()
{
    EnsureHwApiInitialized(0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-select-hw-api1
 * @recoil-artifact defines .text recoil:function:0x4317a0: CZRecoilFrame::OnMenuSelectHwApi1.
 * @recoil-match byte
 *
 * Purpose: select hardware API menu entry 1.
 */
void CZRecoilFrame::OnMenuSelectHwApi1()
{
    EnsureHwApiInitialized(1);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-select-hw-api2
 * @recoil-artifact defines .text recoil:function:0x4317b0: CZRecoilFrame::OnMenuSelectHwApi2.
 * @recoil-match byte
 *
 * Purpose: select hardware API menu entry 2.
 */
void CZRecoilFrame::OnMenuSelectHwApi2()
{
    EnsureHwApiInitialized(2);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-select-hw-api3
 * @recoil-artifact defines .text recoil:function:0x4317c0: CZRecoilFrame::OnMenuSelectHwApi3.
 * @recoil-match byte
 *
 * Purpose: select hardware API menu entry 3.
 */
void CZRecoilFrame::OnMenuSelectHwApi3()
{
    EnsureHwApiInitialized(3);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-update-hw-api-menu-item
 * @recoil-artifact defines .text recoil:function:0x4317d0: CZRecoilFrame::UpdateHwApiMenuItem.
 * @recoil-match byte
 *
 * Purpose: remove unavailable hardware API commands or update their label/check state.
 */
RECOIL_NO_GS void CZRecoilFrame::UpdateHwApiMenuItem(CCmdUI* cmdUi, int apiIndex)
{
    if (m_acceptedD3DDeviceCount >= apiIndex) {
        const int hwApiIndex = apiIndex - 1;
        if (m_hwApiCmdUiState[apiIndex] == kCmdUiChecked) {
            cmdUi->SetCheck(1);
        } else {
            cmdUi->SetCheck(0);
        }

        char menuLabelText[0x40];
        sprintf(
            menuLabelText,
            g_CZRecoilFrame_AcceleratorMenuLabelFmt,
            zVid::GetHwApiDescription(hwApiIndex),
            zVid::GetHwApiDriverName(hwApiIndex)
        );
        cmdUi->SetText(menuLabelText);
        return;
    }

    RemoveMenu(cmdUi->m_pMenu->m_hMenu, m_hwApiMenuCommandIds[apiIndex], MF_BYCOMMAND);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-update-hw-api0-cmd-ui
 * @recoil-artifact defines .text recoil:function:0x431870: CZRecoilFrame::OnUpdateHwApi0CmdUI.
 * @recoil-match byte
 *
 * Purpose: enable and check the software/fallback hardware API command.
 */
void CZRecoilFrame::OnUpdateHwApi0CmdUI(CCmdUI* cmdUi)
{
    cmdUi->Enable(1);
    if (m_hwApiCmdUiState[0] == kCmdUiChecked) {
        cmdUi->SetCheck(1);
    } else {
        cmdUi->SetCheck(0);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-update-hw-api1-cmd-ui
 * @recoil-artifact defines .text recoil:function:0x4318b0: CZRecoilFrame::OnUpdateHwApi1CmdUI.
 * @recoil-match byte
 *
 * Purpose: update hardware API command UI entry 1.
 */
void CZRecoilFrame::OnUpdateHwApi1CmdUI(CCmdUI* cmdUi)
{
    UpdateHwApiMenuItem(cmdUi, 1);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-update-hw-api2-cmd-ui
 * @recoil-artifact defines .text recoil:function:0x4318c0: CZRecoilFrame::OnUpdateHwApi2CmdUI.
 * @recoil-match byte
 *
 * Purpose: update hardware API command UI entry 2.
 */
void CZRecoilFrame::OnUpdateHwApi2CmdUI(CCmdUI* cmdUi)
{
    UpdateHwApiMenuItem(cmdUi, 2);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-update-hw-api3-cmd-ui
 * @recoil-artifact defines .text recoil:function:0x4318d0: CZRecoilFrame::OnUpdateHwApi3CmdUI.
 * @recoil-match byte
 *
 * Purpose: update hardware API command UI entry 3.
 */
void CZRecoilFrame::OnUpdateHwApi3CmdUI(CCmdUI* cmdUi)
{
    UpdateHwApiMenuItem(cmdUi, 3);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-update-fullscreen-cmd-ui
 * @recoil-artifact defines .text recoil:function:0x4318e0: CZRecoilFrame::OnUpdateFullscreenCmdUI.
 * @recoil-match byte
 *
 * Purpose: remove the fullscreen command from the update menu path.
 */
void CZRecoilFrame::OnUpdateFullscreenCmdUI(CCmdUI* cmdUi)
{
    RemoveMenu(cmdUi->m_pMenu->m_hMenu, kFullscreenMenuCommandId, MF_BYCOMMAND);
}

/**
 * Original helper evidence: CZRecoilFrame message-map entries for command ids
 * 0x9c7f, 0x9c81, and 0x9c84 share the same one-argument enable handler.
 * Purpose: Enable command UI entries that have no authored state gate.
 */
void CZRecoilFrame::OnUpdateAlwaysEnabledCmdUI(CCmdUI* cmdUi)
{
    MfcCmdUI::EnableAlways(cmdUi);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-toggle-cdaudio
 * @recoil-artifact defines .text recoil:function:0x431900: CZRecoilFrame::OnMenuToggleCDAudio.
 * @recoil-match byte
 *
 * Purpose: toggle the CD audio option from the frame menu.
 */
void CZRecoilFrame::OnMenuToggleCDAudio()
{
    zSnd::SetCDAudioOption(zSnd::GetCDAudioOption() == 0 ? 1 : 0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-update-cdaudio-cmd-ui
 * @recoil-artifact defines .text recoil:function:0x431920: CZRecoilFrame::OnUpdateCDAudioCmdUI.
 * @recoil-match byte
 *
 * Purpose: enable and check the CD audio command from sound options.
 */
void CZRecoilFrame::OnUpdateCDAudioCmdUI(CCmdUI* cmdUi)
{
    cmdUi->Enable(1);
    cmdUi->SetCheck(zSnd::GetCDAudioOption() != 0 ? 1 : 0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-toggle-joystick
 * @recoil-artifact defines .text recoil:function:0x431950: CZRecoilFrame::OnMenuToggleJoystick.
 * @recoil-match byte
 *
 * Purpose: toggle joystick input from the frame menu.
 */
void CZRecoilFrame::OnMenuToggleJoystick()
{
    zInp::SetJoystickOption(zInp::GetJoystickOption() == 0 ? 1 : 0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-update-joystick-cmd-ui
 * @recoil-artifact defines .text recoil:function:0x431970: CZRecoilFrame::OnUpdateJoystickCmdUI.
 * @recoil-match byte
 *
 * Purpose: enable and check the joystick command from input options.
 */
void CZRecoilFrame::OnUpdateJoystickCmdUI(CCmdUI* cmdUi)
{
    cmdUi->Enable(1);
    cmdUi->SetCheck(zInp::GetJoystickOption() != 0 ? 1 : 0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-westwood-online-upgrade
 * @recoil-artifact defines .text recoil:function:0x4319a0: CZRecoilFrame::OnMenuWestwoodOnlineUpgrade.
 * @recoil-match byte
 *
 * Purpose: gate the Westwood Online upgrade flow on Winsock2 readiness and
 * launch the selected mission.
 */
RECOIL_NO_GS void CZRecoilFrame::OnMenuWestwoodOnlineUpgrade()
{
    int canShowUpgrade = 1;
    if (g_CZRecoilFrame_WestwoodOnlineWinsockChecked == 0) {
        char caption[0x100];
        char messageFormat[0x200];

        g_CZRecoilFrame_WestwoodOnlineWinsockChecked = 1;
        strcpy(caption, zLoc::GetMessageString(18));
        strcpy(messageFormat, zLoc::GetMessageString(38));
        if (NetUi::VerifyWinsock2OrPromptContinue(caption, messageFormat) == 0) {
            canShowUpgrade = 0;
        }
    }

    if (canShowUpgrade == 0) {
        return;
    }

    g_RecoilApp.m_skipIntroFmv = 1;
    g_RecoilApp.m_missionFmvState.m_skipMissionFmv = 1;

    int selectedMissionIndex;
    if (WestwoodOnlineUpgradeDialog::ShowModalAndGetSelectedMissionIndex(&selectedMissionIndex) != 0) {
        const int missionFlags = g_HudSensorTracker.missionFlags;
        g_RecoilApp.LoadZbdAndSetupSensorTracker(selectedMissionIndex + 6, 0, 1, missionFlags);
    }
}

namespace MfcCmdUI {
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.mfc-cmd-ui-enable-always
 * @recoil-artifact defines .text recoil:function:0x431a80: MfcCmdUI::EnableAlways.
 * @recoil-match byte
 *
 * Purpose: Marks the associated MFC command as enabled.
 */
void __stdcall EnableAlways(CCmdUI* cmdUi)
{
    cmdUi->Enable(1);
}
} // namespace MfcCmdUI

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-select-direct-sound
 * @recoil-artifact defines .text recoil:function:0x431a90: CZRecoilFrame::OnMenuSelectDirectSound.
 * @recoil-match byte
 *
 * Purpose: select DirectSound as the active audio API option.
 */
void CZRecoilFrame::OnMenuSelectDirectSound()
{
    zSnd::SetAudioApiOption(ZSND_AUDIO_API_DIRECTSOUND);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-update-direct-sound-cmd-ui
 * @recoil-artifact defines .text recoil:function:0x431aa0: CZRecoilFrame::OnUpdateDirectSoundCmdUI.
 * @recoil-match byte
 *
 * Purpose: enable and check the DirectSound command from audio options.
 */
void CZRecoilFrame::OnUpdateDirectSoundCmdUI(CCmdUI* cmdUi)
{
    cmdUi->Enable(1);
    cmdUi->SetCheck(zSnd::GetAudioApiOption() == ZSND_AUDIO_API_DIRECTSOUND ? 1 : 0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-menu-select-a3-d
 * @recoil-artifact defines .text recoil:function:0x431ad0: CZRecoilFrame::OnMenuSelectA3D.
 * @recoil-match byte
 *
 * Purpose: select A3D as the active audio API option.
 */
void CZRecoilFrame::OnMenuSelectA3D()
{
    zSnd::SetAudioApiOption(ZSND_AUDIO_API_A3D);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-update-a3-dcmd-ui
 * @recoil-artifact defines .text recoil:function:0x431ae0: CZRecoilFrame::OnUpdateA3DCmdUI.
 * @recoil-match byte
 *
 * Purpose: enable and check the A3D command from the active sound backend.
 */
void CZRecoilFrame::OnUpdateA3DCmdUI(CCmdUI* cmdUi)
{
    cmdUi->Enable(1);
    cmdUi->SetCheck(zSnd::GetActiveBackend() == ZSND_AUDIO_API_A3D ? 1 : 0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilframe.czrecoil-frame-on-size
 * @recoil-artifact defines .text recoil:function:0x431b10: CZRecoilFrame::OnSize.
 * @recoil-match byte
 *
 * Purpose: forward sizing to CZGameFrame and deactivate the app on minimized/iconic states.
 */
void CZRecoilFrame::OnSize(unsigned int nType, int cx, int cy)
{
    CZGameFrame::OnSize(nType, cx, cy);

    if (nType == 4 || nType == 1) {
        m_app->OnAppDeactivate();
    }
}
