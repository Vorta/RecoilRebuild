#include "Battlesport/cz_recoil_frame.h"

#include "Battlesport/cz_game_frame.h"
#include "Battlesport/game_net.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/net_ui.h"
#include "Battlesport/about.h"
#include "Battlesport/recoil_app.h"
#include "Battlesport/wol_dialog.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <commdlg.h>
#include <objbase.h>
#include <shellapi.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HINSTANCE __stdcall AfxFindResourceHandle(
    LPCSTR resourceName,
    LPCSTR resourceType
);

extern "C" {
extern HWND g_RecoilApp_hWndMain;
extern HINSTANCE g_RecoilApp_hInstance;
/**
 * Reimplements data 0x4dcd00: g_CZRecoilFrame_WolApiRegKey.
 *
 * Purpose: provide the constructor-owned registry path used to detect the
 * Westwood Online API install.
 */
extern const char g_CZRecoilFrame_WolApiRegKey[] = "Software\\Westwood\\WOLAPI\\4352";
/**
 * Reimplements data 0x4dcd20: g_CZRecoilFrame_MainMenuResourceName.
 *
 * Purpose: name the recovered frame menu resource loaded during construction.
 */
extern const char g_CZRecoilFrame_MainMenuResourceName[] = "MYMENU";
/**
 * Reimplements data 0x4dcd28: g_RecoilError_LogFileName.
 *
 * Purpose: name the error log initialized by the frame constructor.
 */
extern const char g_RecoilError_LogFileName[] = "recoil.err";
/**
 * Reimplements data 0x4dcd34: g_CZRecoilFrame_NumericDigits.
 *
 * Purpose: preserve the constructor command-line sentinel tested with strncmp.
 */
extern const char g_CZRecoilFrame_NumericDigits[] = "1234567890";
/**
 * Reimplements data 0x4dcd40: g_CZRecoilFrame_CmdCampaigns.
 *
 * Purpose: preserve the constructor command-line campaign-mode switch prefix.
 */
extern const char g_CZRecoilFrame_CmdCampaigns[] = "/campaigns";
/**
 * Reimplements data 0x4dcd4c: g_CZRecoilFrame_LogBaseName.
 *
 * Purpose: provide the CZGameFrame constructor log/base name passed by the
 * Recoil frame constructor.
 */
extern const char g_CZRecoilFrame_LogBaseName[] = "recoil";
/**
 * Reimplements data 0x4dcd54: g_RecoilApp_WindowTitle.
 *
 * Purpose: provide the default Recoil main-window title used by the frame UI.
 */
extern const char g_RecoilApp_WindowTitle[0x7] = "RECOIL";
/**
 * Reimplements data 0x4dcd5c: g_RecoilApp_WindowTitle3Dfx.
 *
 * Purpose: provide the 3Dfx renderer main-window title used by the frame UI.
 */
extern const char g_RecoilApp_WindowTitle3Dfx[0xe] = "RECOIL (3Dfx)";
/**
 * Reimplements data 0x4dcd6c: g_CZRecoilFrame_DefaultFileExt.
 *
 * Purpose: preserve the common-dialog default extension for campaign files.
 */
extern const char g_CZRecoilFrame_DefaultFileExt[0x3] = "gs";
/**
 * Reimplements data 0x4dcd70: g_CZRecoilFrame_AcceleratorMenuLabelFmt.
 *
 * Purpose: format the hardware accelerator command label shown in the frame UI.
 */
extern "C" char g_CZRecoilFrame_AcceleratorMenuLabelFmt[0x16] = "Accelerator - %s (%s)";
/**
 * Reimplements data 0x4f3efc: Symbol.
 *
 * Purpose: remember whether the Westwood Online registry key was found during
 * frame construction.
 */
int g_CZRecoilFrame_HasWolApi = 0;
/**
 * Reimplements data 0x4f3f04: Symbol.
 *
 * Purpose: gate the one-time Winsock2 prompt before launching the Westwood
 * Online upgrade flow.
 */
int g_CZRecoilFrame_WestwoodOnlineWinsockChecked = 0;
/**
 * Reimplements data 0x4f0cc0: g_HudSensorTracker Symbol.
 * Source model: zero-initialized explicit storage for the CZRecoilFrame-owned
 * global instance. HudSensorTracker::ConstructGlobal and ShutdownGlobal own
 * the typed lifetime for this storage.
 * Purpose: Owns the global HUD sensor tracker state used by mission flow,
 * map/objective rendering, network timer sync, and frame-level HUD updates.
 */
#undef g_HudSensorTracker
HudSensorTrackerStorage g_HudSensorTracker = {0};
#define g_HudSensorTracker \
    (*(HudSensorTracker *)&g_HudSensorTracker)
}

namespace {
typedef CObject *(PASCAL *MfcCreateObjectProc)();

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
const char *kRecoilWndClassName = "RecoilClass";

/**
 * Original helper evidence: no standalone retail function; observed in
 * CZRecoilFrame video-mode command UI callers.
 * Purpose: return the MFC checked-state flag when a video mode is active.
 */
int CommandCheckedIfMode(
    int currentMode,
    int targetMode
) {
    return currentMode == targetMode ? kCmdUiChecked : 0;
}

/**
 * Original helper evidence: no standalone retail function; observed in
 * CZRecoilFrame video-mode update handlers.
 * Purpose: translate cached command state into CCmdUI enable/check calls.
 */
void UpdateCmdUiFromState(
    CCmdUI *cmdUi,
    int state
) {
    if (state == kCmdUiDisabled) {
        cmdUi->Enable(0);
        cmdUi->SetCheck(0);
        return;
    }

    cmdUi->Enable(1);
    cmdUi->SetCheck(state == kCmdUiChecked ? 1 : 0);
}

/**
 * Original helper evidence: no standalone retail function; observed in
 * CZRecoilFrame constructor menu-pruning calls.
 * Purpose: fetch a submenu handle for command removal while matching MFC use.
 */
HMENU SubMenuHandleOrNull(
    HMENU menu,
    int position
) {
    return CMenu::FromHandle(GetSubMenu(
        menu,
        position
    ))->m_hMenu;
}
} // namespace

/**
 * Reimplements data 0x4d0c10: g_CZRecoilFrame_MessageEntries.
 *
 * Purpose: provide the recovered MFC message-map entry array for CZRecoilFrame.
 */
AFX_MSGMAP_ENTRY const CZRecoilFrame::messageEntries[] = {
    {WM_COMMAND, 0, 0x68, 0x68, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuStartSinglePlayer},
    {WM_COMMAND, 0, 0x9c51, 0x9c51, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuOpenCampaign},
    {WM_COMMAND, 0, 0x65, 0x65, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnOpenFileDialog},
    {WM_COMMAND, 0, 0x67, 0x67, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuExitGame},
    {WM_COMMAND, 0, 0x206, 0x206, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuSetVideoMode2},
    {WM_COMMAND, 0, 0x207, 0x207, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuSetVideoMode3},
    {WM_COMMAND, 0, 0x208, 0x208, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuSetVideoMode4},
    {WM_COMMAND, 0, 0x209, 0x209, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuSetVideoMode5},
    {WM_COMMAND, 0, 0x9c4f, 0x9c4f, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuToggleHud},
    {WM_COMMAND, 0, 0x9c4e, 0x9c4e, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuToggleFullscreen},
    {WM_COMMAND, 0, 0x6a, 0x6a, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuOpenHelpDocs},
    {WM_COMMAND, 0, 0x6b, 0x6b, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuAbout},
    {WM_COMMAND,
        0,
        0x9c53,
        0x9c53,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&CZRecoilFrame::OnMenuOpenMultiplayerSessionBrowser},
    {WM_COMMAND, 0, 0x9c55, 0x9c55, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuStartMultiplayer},
    {WM_COMMAND, 0, 0x9c56, 0x9c56, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuStartCampaignMode},
    {WM_COMMAND, 0, 0x9c57, 0x9c57, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuStartCampaignMode2},
    {WM_COMMAND, 0, 0x9c58, 0x9c58, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuStartCampaignMode3},
    {WM_COMMAND, 0, 0x9c59, 0x9c59, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuStartCampaignMode4},
    {WM_COMMAND, 0, 0x9c5a, 0x9c5a, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuStartCampaignMode5},
    {WM_COMMAND, 0, 0x9c6b, 0x9c6b, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuToggleArchiveBanks},
    {WM_COMMAND, 0, 0x9c7b, 0x9c7b, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuToggleTexturePacks},
    {WM_COMMAND, 0, 0x210, 0x210, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuSetVideoMode7},
    {WM_COMMAND, 0, 0x9c71, 0x9c71, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuSetVideoMode6},
    {WM_COMMAND,
        kMfcCommandUpdateCode,
        0x210,
        0x210,
        kMfcMessageMapSigCmdUi,
        (AFX_PMSG)&CZRecoilFrame::OnUpdateVideoMode7CmdUI},
    {WM_COMMAND,
        kMfcCommandUpdateCode,
        0x206,
        0x206,
        kMfcMessageMapSigCmdUi,
        (AFX_PMSG)&CZRecoilFrame::OnUpdateVideoMode2CmdUI},
    {WM_COMMAND,
        kMfcCommandUpdateCode,
        0x207,
        0x207,
        kMfcMessageMapSigCmdUi,
        (AFX_PMSG)&CZRecoilFrame::OnUpdateVideoMode3CmdUI},
    {WM_COMMAND,
        kMfcCommandUpdateCode,
        0x208,
        0x208,
        kMfcMessageMapSigCmdUi,
        (AFX_PMSG)&CZRecoilFrame::OnUpdateVideoMode4CmdUI},
    {WM_COMMAND,
        kMfcCommandUpdateCode,
        0x209,
        0x209,
        kMfcMessageMapSigCmdUi,
        (AFX_PMSG)&CZRecoilFrame::OnUpdateVideoMode5CmdUI},
    {WM_COMMAND,
        kMfcCommandUpdateCode,
        0x9c71,
        0x9c71,
        kMfcMessageMapSigCmdUi,
        (AFX_PMSG)&CZRecoilFrame::OnUpdateVideoMode6CmdUI},
    {WM_COMMAND, 0, 0x9c83, 0x9c83, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuSelectHwApi0},
    {WM_COMMAND, 0, 0x9c72, 0x9c72, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuSelectHwApi1},
    {WM_COMMAND, 0, 0x9c75, 0x9c75, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuSelectHwApi2},
    {WM_COMMAND, 0, 0x9c76, 0x9c76, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuSelectHwApi3},
    {WM_COMMAND,
        kMfcCommandUpdateCode,
        0x9c83,
        0x9c83,
        kMfcMessageMapSigCmdUi,
        (AFX_PMSG)&CZRecoilFrame::OnUpdateHwApi0CmdUI},
    {WM_COMMAND,
        kMfcCommandUpdateCode,
        0x9c72,
        0x9c72,
        kMfcMessageMapSigCmdUi,
        (AFX_PMSG)&CZRecoilFrame::OnUpdateHwApi1CmdUI},
    {WM_COMMAND,
        kMfcCommandUpdateCode,
        0x9c75,
        0x9c75,
        kMfcMessageMapSigCmdUi,
        (AFX_PMSG)&CZRecoilFrame::OnUpdateHwApi2CmdUI},
    {WM_COMMAND,
        kMfcCommandUpdateCode,
        0x9c76,
        0x9c76,
        kMfcMessageMapSigCmdUi,
        (AFX_PMSG)&CZRecoilFrame::OnUpdateHwApi3CmdUI},
    {WM_COMMAND,
        kMfcCommandUpdateCode,
        0x9c4e,
        0x9c4e,
        kMfcMessageMapSigCmdUi,
        (AFX_PMSG)&CZRecoilFrame::OnUpdateFullscreenCmdUI},
    {WM_COMMAND, 0, 0x9c7c, 0x9c7c, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuToggleCDAudio},
    {WM_COMMAND,
        kMfcCommandUpdateCode,
        0x9c7c,
        0x9c7c,
        kMfcMessageMapSigCmdUi,
        (AFX_PMSG)&CZRecoilFrame::OnUpdateCDAudioCmdUI},
    {WM_COMMAND, 0, 0x9c7d, 0x9c7d, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuToggleJoystick},
    {WM_COMMAND,
        kMfcCommandUpdateCode,
        0x9c7d,
        0x9c7d,
        kMfcMessageMapSigCmdUi,
        (AFX_PMSG)&CZRecoilFrame::OnUpdateJoystickCmdUI},
    {WM_COMMAND,
        0,
        0x9c7e,
        0x9c7e,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&CZRecoilFrame::OnMenuWestwoodOnlineUpgrade},
    {WM_COMMAND,
        kMfcCommandUpdateCode,
        0x9c7f,
        0x9c7f,
        kMfcMessageMapSigCmdUi,
        (AFX_PMSG)&CZRecoilFrame::OnUpdateAlwaysEnabledCmdUI},
    {WM_COMMAND,
        kMfcCommandUpdateCode,
        0x9c81,
        0x9c81,
        kMfcMessageMapSigCmdUi,
        (AFX_PMSG)&CZRecoilFrame::OnUpdateAlwaysEnabledCmdUI},
    {WM_COMMAND,
        kMfcCommandUpdateCode,
        0x9c84,
        0x9c84,
        kMfcMessageMapSigCmdUi,
        (AFX_PMSG)&CZRecoilFrame::OnUpdateAlwaysEnabledCmdUI},
    {WM_COMMAND,
        kMfcCommandUpdateCode,
        0x9c7e,
        0x9c7e,
        kMfcMessageMapSigCmdUi,
        (AFX_PMSG)&CZRecoilFrame::OnUpdateNoOpCmdUI},
    {WM_COMMAND,
        kMfcCommandUpdateCode,
        0x9c4f,
        0x9c4f,
        kMfcMessageMapSigCmdUi,
        (AFX_PMSG)&CZRecoilFrame::OnUpdateHudCmdUI},
    {WM_COMMAND, 0, 0x9c80, 0x9c80, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuSelectDirectSound},
    {WM_COMMAND,
        kMfcCommandUpdateCode,
        0x9c80,
        0x9c80,
        kMfcMessageMapSigCmdUi,
        (AFX_PMSG)&CZRecoilFrame::OnUpdateDirectSoundCmdUI},
    {WM_COMMAND, 0, 0x9c82, 0x9c82, kMfcMessageMapSigVoid, (AFX_PMSG)&CZRecoilFrame::OnMenuSelectA3D},
    {WM_COMMAND,
        kMfcCommandUpdateCode,
        0x9c82,
        0x9c82,
        kMfcMessageMapSigCmdUi,
        (AFX_PMSG)&CZRecoilFrame::OnUpdateA3DCmdUI},
    {WM_COMMAND,
        kMfcCommandUpdateCode,
        0x9c53,
        0x9c53,
        kMfcMessageMapSigCmdUi,
        (AFX_PMSG)&CZRecoilFrame::OnUpdateNoOpCmdUI},
    {WM_SIZE,
        0,
        0,
        0,
        kMfcMessageMapSigVoidUIntIntInt,
        (AFX_PMSG)&CZRecoilFrame::OnSize},
    {0, 0, 0, 0, 0, 0},
};

/**
 * Reimplements data 0x4d0c08: g_CZRecoilFrame_MessageMap.
 *
 * Purpose: link CZRecoilFrame's message entries to the recovered CZGameFrame
 * message-map accessor used as the retail base-map callback.
 */
const AFX_MSGMAP CZRecoilFrame::messageMap = {
    &CZGameFrame::GetMessageMapStatic,
    &CZRecoilFrame::messageEntries[0],
};

/**
 * Reimplements data 0x4d0bf0: g_CZRecoilFrame_RuntimeClass.
 *
 * Purpose: expose CZRecoilFrame's MFC runtime-class record with the recovered
 * factory and base-runtime callback pointer identities.
 */
CRuntimeClass CZRecoilFrame::classCZRecoilFrame = {
    "CZRecoilFrame",
    sizeof(CZRecoilFrame),
    0xffff,
    (MfcCreateObjectProc)&CZRecoilFrame::CreateObject,
    &CZGameFrame::GetRuntimeClassStatic,
    0,
};

/**
 * Reimplements 0x430230: CZRecoilFrame::GetBaseRuntimeClass.
 *
 * Purpose: return the recovered CZGameFrame runtime-class record for
 * CZRecoilFrame's MFC hierarchy.
 */
CRuntimeClass *__stdcall CZRecoilFrame::GetBaseRuntimeClass() {
    return &CZGameFrame::classCZGameFrame;
}

/**
 * Reimplements 0x4301e0: CZRecoilFrame::CreateObject.
 *
 * Purpose: allocate and construct the Recoil frame object for the recovered MFC
 * runtime-class factory path.
 */
CZRecoilFrame *CZRecoilFrame::CreateObject() {
    return new CZRecoilFrame();
}

/**
 * Reimplements 0x430240 callback rule: CZRecoilFrame runtime-class access.
 *
 * Purpose: keep a static callback for MFC data records while the vtable slot is
 * modeled by the non-static MFC override.
 */
CRuntimeClass *__stdcall CZRecoilFrame::GetRuntimeClassStatic() {
    return &CZRecoilFrame::classCZRecoilFrame;
}

/**
 * Reimplements 0x430240: CZRecoilFrame::GetRuntimeClass.
 *
 * Purpose: expose CZRecoilFrame's runtime-class record through the inherited
 * MFC virtual slot that owns the compiler-emitted Recoil frame vtable entry.
 */
CRuntimeClass *CZRecoilFrame::GetRuntimeClass() const {
    return &CZRecoilFrame::classCZRecoilFrame;
}

/**
 * Reimplements 0x4306d0: CZRecoilFrame::GetBaseMessageMap.
 *
 * Purpose: return CZGameFrame's recovered message-map record for the frame's
 * MFC hierarchy.
 */
const AFX_MSGMAP *__stdcall CZRecoilFrame::GetBaseMessageMap() {
    return &CZGameFrame::messageMap;
}

/**
 * Reimplements 0x4306e0 callback rule: CZRecoilFrame message-map access.
 *
 * Purpose: keep a static callback for MFC data records while the vtable slot is
 * modeled by the non-static MFC override.
 */
const AFX_MSGMAP *__stdcall CZRecoilFrame::GetMessageMapStatic() {
    return &CZRecoilFrame::messageMap;
}

/**
 * Reimplements 0x4306e0: CZRecoilFrame::GetMessageMap.
 *
 * Purpose: expose CZRecoilFrame's message-map record through the inherited MFC
 * virtual slot used by MFC command and window-message dispatch.
 */
const AFX_MSGMAP * CZRecoilFrame::GetMessageMap() const {
    return &CZRecoilFrame::messageMap;
}

namespace MfcCmdUI {
/**
 * Reimplements 0x431a80: MfcCmdUI::EnableAlways.
 *
 * Purpose: provide the shared MFC command-update target that enables commands.
 */
void __stdcall EnableAlways(
    CCmdUI *cmdUi
) {
    cmdUi->Enable(1);
}
} // namespace MfcCmdUI

/**
 * Reimplements 0x430250: CZRecoilFrame::CZRecoilFrame.
 *
 * Purpose: model the original MFC-derived C++ construction path that builds
 * the CZGameFrame base, menu member, frame window, launch options, renderer
 * menu state, and Westwood Online availability flag.
 */
CZRecoilFrame::CZRecoilFrame() : CZGameFrame(g_CZRecoilFrame_LogBaseName) {
    unsigned long
        titleStorage[(sizeof(CString) + sizeof(unsigned long) - 1) / sizeof(unsigned long)];
    CString *title = (CString *)(titleStorage);
    BuildWindowTitle(title);
    const int windowHeight = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYMENU) +
                             (GetSystemMetrics(SM_CYFRAME) << 1) + 0x1e0;
    const int windowWidth = (GetSystemMetrics(SM_CXFRAME) << 1) + 0x280;
    ((CWnd *)(this))->CreateEx(
        0x20000,
        kRecoilWndClassName,
        (const char *)(*title),
        kMainWindowStyle,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowWidth,
        windowHeight,
        0,
        0,
        0
    );
    title->~CString();

    m_cmdlineFlag = 1;
    m_campaignsOnlyMode = 0;
    char *commandLineCopy = _strdup(GetCommandLineA());
    for (char *token = strtok(
        commandLineCopy,
        " "
    ); token != 0; token = strtok(
        0,
        " "
    )) {
        if (strncmp(
            token,
            g_CZRecoilFrame_CmdCampaigns,
            4
        ) == 0) {
            m_campaignsOnlyMode = 1;
        } else if (strncmp(
            token,
            g_CZRecoilFrame_NumericDigits,
            4
        ) == 0) {
            m_cmdlineFlag = 0;
        }
    }
    free(commandLineCopy);

    zError::InitOutputContext(
        m_hWnd,
        0xe00,
        g_RecoilError_LogFileName
    );
    m_mainMenu.Attach(LoadMenuA(
        AfxFindResourceHandle(
            g_CZRecoilFrame_MainMenuResourceName,
            MAKEINTRESOURCEA(4)
        ),
        g_CZRecoilFrame_MainMenuResourceName
    ));
    ::SetMenu(
        m_hWnd,
        m_mainMenu.m_hMenu
    );

    if (m_campaignsOnlyMode != 0) {
        RemoveMenu(
            SubMenuHandleOrNull(
                m_mainMenu.m_hMenu,
                1
            ),
            0x9c6b,
            MF_BYCOMMAND
        );
        RemoveMenu(
            SubMenuHandleOrNull(
                m_mainMenu.m_hMenu,
                1
            ),
            0x9c7b,
            MF_BYCOMMAND
        );
    } else {
        RemoveMenu(
            m_mainMenu.m_hMenu,
            1,
            MF_BYPOSITION
        );
    }

    RemoveMenu(
        SubMenuHandleOrNull(
            m_mainMenu.m_hMenu,
            2
        ),
        kFullscreenMenuCommandId,
        MF_BYCOMMAND
    );

    g_RecoilApp_hInstance = (HINSTANCE)((unsigned int)(g_RecoilApp.m_hInstance));
    g_RecoilApp_hWndMain = m_hWnd;

    unsigned long formattedTitleStorage
        [(sizeof(CString) + sizeof(unsigned long) - 1) / sizeof(unsigned long)];
    CString *formattedTitle = (CString *)(formattedTitleStorage);
    new (formattedTitle) CString();
    unsigned long
        titleCopyStorage[(sizeof(CString) + sizeof(unsigned long) - 1) / sizeof(unsigned long)];
    CString *titleCopy = (CString *)(titleCopyStorage);
    BuildWindowTitle(titleCopy);
    formattedTitle->Format(
        "%s",
        (const char *)(*titleCopy)
    );
    titleCopy->~CString();
    ((CWnd *)(this))->SetWindowTextA((const char *)(*formattedTitle));
    formattedTitle->~CString();

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

    CheckMenuItem(
        m_mainMenu.m_hMenu,
        0x9c7b,
        zVid::GetTexturePackLoadState() == 0 ? 0 : MF_CHECKED
    );

    g_HudSensorTracker.missionFlags = m_useArchiveBanks;
    zSnd::SetUseArchiveBanksFlag(m_useArchiveBanks);
    m_acceptedD3DDeviceCount = zVid::GetAcceptedHardwareRendererCount_Cached();

    HKEY wolApiRegKey = 0;
    if (RegOpenKeyExA(
            HKEY_LOCAL_MACHINE,
            g_CZRecoilFrame_WolApiRegKey,
            0,
            KEY_READ,
            &wolApiRegKey
        ) == ERROR_SUCCESS) {
        g_CZRecoilFrame_HasWolApi = 1;
        RegCloseKey(wolApiRegKey);
    }

    ((CWnd *)(this))->CenterWindow(0);
    SetCursor(LoadCursorA(
        0,
        IDC_ARROW
    ));
}

/**
 * Reimplements 0x430610: CZRecoilFrame::~CZRecoilFrame.
 *
 * Purpose: let compiler-emitted MFC member and CZGameFrame base teardown
 * destroy the owned menu through the CMenu provider.
 */
CZRecoilFrame::~CZRecoilFrame() {
}

/**
 * Reimplements 0x430680: CZRecoilFrame::SetMenuBarVisibility.
 *
 * Purpose: attach or remove the recovered main menu and frame menu style.
 */
void CZRecoilFrame::SetMenuBarVisibility(
    int visible
) {
    LONG style = GetWindowLongA(
        m_hWnd,
        GWL_STYLE
    );
    HMENU menu = 0;
    if (visible != 0) {
        style |= (LONG)(0x82ca0000);
        menu = m_mainMenu.m_hMenu;
    } else {
        style &= (LONG)(0xfff7ffff);
    }

    SetWindowLongA(
        m_hWnd,
        GWL_STYLE,
        style
    );
    ::SetMenu(
        m_hWnd,
        menu
    );
}

/**
 * Reimplements 0x4306f0: CZRecoilFrame::BuildWindowTitle.
 *
 * Purpose: build the Recoil window title, including the 3Dfx renderer suffix.
 */
CString * CZRecoilFrame::BuildWindowTitle(
    CString *outTitle
) {
    volatile int constructedTitleState = 0;
    if (g_zVideo_ActiveRendererPath == kRendererBackend3dfx) {
        outTitle->CString::CString(g_RecoilApp_WindowTitle3Dfx);
        return outTitle;
    }

    outTitle->CString::CString(g_RecoilApp_WindowTitle);
    return outTitle;
}

/**
 * Reimplements 0x430740: CZRecoilFrame::OnMenuStartSinglePlayer.
 *
 * Purpose: clear intro/mission FMV skips and start the default engine load.
 */
void CZRecoilFrame::OnMenuStartSinglePlayer() {
    g_RecoilApp.m_skipIntroFmv = 0;
    g_RecoilApp.m_missionFmvState.m_skipMissionFmv = 0;
    g_RecoilApp.LoadZbdAndStartEngine();
}

/**
 * Reimplements 0x430760: CZRecoilFrame::OnMenuOpenCampaign.
 *
 * Purpose: enter campaign-open flow with the intro FMV skipped.
 */
void CZRecoilFrame::OnMenuOpenCampaign() {
    g_RecoilApp.m_skipIntroFmv = 1;
    OnOpenFileDialog();
}

/**
 * Reimplements 0x430770: CZRecoilFrame::OnOpenFileDialog.
 *
 * Purpose: open a campaign ZBD file through the retail common dialog path and
 * launch the selected mission data.
 */
RECOIL_NO_GS void CZRecoilFrame::OnOpenFileDialog() {
    char filter[0x100];
    const int filterLength = LoadStringA(
        g_RecoilApp_hInstance,
        0xc8,
        filter,
        sizeof(filter)
    );
    if (filter[0] != '\0') {
        const char separator = filterLength > 0 ? filter[filterLength - 1] : '\0';
        for (char *cursor = filter; *cursor != '\0'; ++cursor) {
            if (*cursor == separator) {
                *cursor = '\0';
            }
        }
    }

    char fileTitle[0x100] = {0};
    OPENFILENAMEA ofn;
    memset(
        &ofn,
        0,
        sizeof(ofn)
    );
    ofn.lStructSize = 0x4c;
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = m_openZbdFilePath;
    ofn.nMaxFile = sizeof(m_openZbdFilePath);
    ofn.lpstrFileTitle = fileTitle;
    ofn.nMaxFileTitle = 0x200;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.lpstrDefExt = g_CZRecoilFrame_DefaultFileExt;

    if (GetOpenFileNameA((LPOPENFILENAMEA)(&ofn)) != 0) {
        strcpy(
            m_openZbdFilePath,
            ofn.lpstrFile
        );
        g_RecoilApp.LoadZbdAndSetupSensorTracker(
            0,
            m_openZbdFilePath,
            1,
            1
        );
    }

    ::InvalidateRect(
        m_hWnd,
        0,
        TRUE
    );
}

/**
 * Reimplements 0x4308c0: CZRecoilFrame::ConfigureModeFeatureFlags.
 *
 * Purpose: cache menu command UI states for video modes based on acceleration
 * state and available video memory.
 */
void CZRecoilFrame::ConfigureModeFeatureFlags() {
    const int mode = zVid::GetVideoModeIndexFromOptions();

    if (zVid::GetAccelerationOption() == 0) {
        m_videoModeCmdUiState[0] = CommandCheckedIfMode(
            mode,
            2
        );
        m_videoModeCmdUiState[1] = CommandCheckedIfMode(
            mode,
            3
        );
        m_videoModeCmdUiState[2] = CommandCheckedIfMode(
            mode,
            4
        );
        m_videoModeCmdUiState[3] = CommandCheckedIfMode(
            mode,
            5
        );
        m_videoModeCmdUiState[4] = CommandCheckedIfMode(
            mode,
            6
        );
        m_videoModeCmdUiState[5] = CommandCheckedIfMode(
            mode,
            7
        );
        return;
    }

    m_videoModeCmdUiState[0] = kCmdUiDisabled;
    m_videoModeCmdUiState[1] = kCmdUiDisabled;
    m_videoModeCmdUiState[2] = CommandCheckedIfMode(
        mode,
        4
    );
    m_videoModeCmdUiState[3] = CommandCheckedIfMode(
        mode,
        5
    );

    if (m_vidMemFreeBytes > kVidMem800x600Threshold) {
        m_videoModeCmdUiState[4] = CommandCheckedIfMode(
            mode,
            6
        );
    } else {
        m_videoModeCmdUiState[4] = kCmdUiDisabled;
    }

    if (m_vidMemFreeBytes > kVidMem1024x768Threshold) {
        m_videoModeCmdUiState[5] = CommandCheckedIfMode(
            mode,
            7
        );
    } else {
        m_videoModeCmdUiState[5] = kCmdUiDisabled;
    }
}

/**
 * Reimplements 0x4309b0: CZRecoilFrame::OnMenuSetVideoMode2.
 *
 * Purpose: set video mode 2 and refresh the recovered mode command state.
 */
void CZRecoilFrame::OnMenuSetVideoMode2() {
    zVid::SetVideoModeIndex(2);
    ConfigureModeFeatureFlags();
}

/**
 * Reimplements 0x4309d0: CZRecoilFrame::OnMenuSetVideoMode3.
 *
 * Purpose: set video mode 3 and refresh the recovered mode command state.
 */
void CZRecoilFrame::OnMenuSetVideoMode3() {
    zVid::SetVideoModeIndex(3);
    ConfigureModeFeatureFlags();
}

/**
 * Reimplements 0x4309f0: CZRecoilFrame::OnMenuSetVideoMode4.
 *
 * Purpose: set video mode 4 and refresh the recovered mode command state.
 */
void CZRecoilFrame::OnMenuSetVideoMode4() {
    zVid::SetVideoModeIndex(4);
    ConfigureModeFeatureFlags();
}

/**
 * Reimplements 0x430a10: CZRecoilFrame::OnMenuSetVideoMode5.
 *
 * Purpose: set video mode 5 and refresh the recovered mode command state.
 */
void CZRecoilFrame::OnMenuSetVideoMode5() {
    zVid::SetVideoModeIndex(5);
    ConfigureModeFeatureFlags();
}

/**
 * Reimplements 0x430a30: CZRecoilFrame::OnMenuSetVideoMode6.
 *
 * Purpose: set video mode 6 and refresh the recovered mode command state.
 */
void CZRecoilFrame::OnMenuSetVideoMode6() {
    zVid::SetVideoModeIndex(6);
    ConfigureModeFeatureFlags();
}

/**
 * Reimplements 0x430a50: CZRecoilFrame::OnMenuSetVideoMode7.
 *
 * Purpose: set video mode 7 and refresh the recovered mode command state.
 */
void CZRecoilFrame::OnMenuSetVideoMode7() {
    zVid::SetVideoModeIndex(7);
    ConfigureModeFeatureFlags();
}

/**
 * Reimplements 0x4308a0: CZRecoilFrame::OnMenuExitGame.
 *
 * Purpose: post WM_CLOSE to the recovered frame window.
 */
void CZRecoilFrame::OnMenuExitGame() {
    ::PostMessageA(
        m_hWnd,
        WM_CLOSE,
        0,
        0
    );
}

/**
 * Reimplements 0x430a70: CZRecoilFrame::OnMenuToggleHud.
 *
 * Purpose: toggle the HUD visibility option from the frame menu.
 */
void CZRecoilFrame::OnMenuToggleHud() {
    zOpt::SetHudVisibilityOption(zOpt::GetHudVisibilityOption() == 0 ? 1 : 0);
}

/**
 * Reimplements 0x430a90: CZRecoilFrame::OnUpdateHudCmdUI.
 *
 * Purpose: enable and check the HUD command from the current option state.
 */
void CZRecoilFrame::OnUpdateHudCmdUI(
    CCmdUI *cmdUi
) {
    cmdUi->Enable(1);
    cmdUi->SetCheck(zOpt::GetHudVisibilityOption());
}

/**
 * Reimplements 0x430ab0: CZRecoilFrame::OnMenuToggleFullscreen.
 *
 * Purpose: toggle the fullscreen option from the frame menu.
 */
void CZRecoilFrame::OnMenuToggleFullscreen() {
    zOpt::SetFullscreenOption(zOpt::GetFullscreenOption() == 0 ? 1 : 0);
}

/**
 * Reimplements 0x430ad0: CZRecoilFrame::OnMenuOpenHelpDocs.
 *
 * Purpose: open the retail help index or report the associated shell error.
 */
RECOIL_NO_GS void CZRecoilFrame::OnMenuOpenHelpDocs() {
    static const unsigned char kFindExecutableErrorMap[0x20] = {0,
        4,
        1,
        1,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        2,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        3};

    char associatedExecutablePath[0x100];
    HINSTANCE findResult = FindExecutableA(
        "Docs\\Index.html",
        0,
        associatedExecutablePath
    );

    char messageBoxTitle[0x80];
    strcpy(
        messageBoxTitle,
        zLoc::GetMessageString(0x19)
    );

    const UINT resultCode = (UINT)((UINT_PTR)(findResult));
    if (resultCode <= 0x1f) {
        switch (kFindExecutableErrorMap[resultCode]) {
        case 0:
            ((CWnd *)(this))->MessageBoxA(
                zLoc::GetMessageString(0x20),
                messageBoxTitle,
                0x30
            );
            return;

        case 1:
            ((CWnd *)(this))->MessageBoxA(
                zLoc::GetMessageString(0x22),
                messageBoxTitle,
                0x30
            );
            return;

        case 2:
            ((CWnd *)(this))->MessageBoxA(
                zLoc::GetMessageString(0x24),
                messageBoxTitle,
                0x30
            );
            return;

        case 3:
            ((CWnd *)(this))->MessageBoxA(
                zLoc::GetMessageString(0x21),
                messageBoxTitle,
                0x30
            );
            return;

        default:
            break;
        }
    }

    ShellExecuteA(
        g_RecoilApp_hWndMain,
        "open",
        "Docs\\Index.html",
        0,
        0,
        SW_HIDE
    );
}

/**
 * Reimplements 0x430c30: CZRecoilFrame::OnMenuAbout (D:\Proj\Battlesport\CZRecoilFrame.cpp).
 *
 * Purpose: display the recovered About dialog through the frame menu.
 */
RECOIL_NO_GS void CZRecoilFrame::OnMenuAbout() {
    CAboutDlg aboutDlg;
    aboutDlg.CDialog::DoModal();
}

/**
 * Reimplements 0x430d80: CZRecoilFrame::OnMenuOpenMultiplayerSessionBrowser.
 * Source file evidence: D:\Proj\Battlesport\CZRecoilFrame.cpp.
 * Purpose: run the DirectPlay session browser/host setup flow and launch the
 * selected multiplayer mission state.
 */
void CZRecoilFrame::OnMenuOpenMultiplayerSessionBrowser() {
    int shouldShutdownNetwork = 1;

    if (CoInitialize(0) >= 0) {
        unsigned long browserStorage
            [(sizeof(NetSessionBrowserDialog) + sizeof(unsigned long) - 1) / sizeof(unsigned long)];
        unsigned long configStorage
            [(sizeof(NetSessionConfigDialog) + sizeof(unsigned long) - 1) / sizeof(unsigned long)];
        NetSessionBrowserDialog *const browserDialog = (NetSessionBrowserDialog *)(browserStorage);
        NetSessionConfigDialog *const configDialog = (NetSessionConfigDialog *)(configStorage);

        browserDialog->Constructor(0);
        configDialog->Constructor(0);

        zNetwork::InitSessionRuntime(&g_zNetwork_RecoilAppGuid);
        zNetwork::SetFatalDisconnectCallback(&RecoilApp::FatalErrorAndExit);
        g_RecoilApp.m_skipIntroFmv = kFmvSkipEnabled;
        g_RecoilApp.m_missionFmvState.m_skipMissionFmv = kFmvSkipEnabled;

        if (((CDialog *)browserDialog)->CDialog::DoModal() == IDOK) {
            zOpt::SetPlayerName((const char *)(browserDialog->m_playerName));

            if (browserDialog->m_shouldEnterHostSetup != 0) {
                zOpt::SetNetworkEnabled(kNetworkOptionEnabled);
                g_RecoilApp.LoadZbdAndStartEngine();
                HudUiNetGameSetupOverlayOwner::QueueEnterWithReconfigureFlag(0);
                shouldShutdownNetwork = 0;
            } else {
                zNetworkSessionDescStatusFields statusFields;
                statusFields.selectedSessionIndex = browserDialog->m_selectedSessionIndex;

                if (zNetworkDPlay::OpenSelectedSessionAndReadStatusFields(&statusFields) != 0) {
                    zOpt::SetNetworkEnabled(kNetworkOptionEnabled);
                    zNetwork_DPlay::CreateLocalPlayerRecordAndRegister(
                        (char *)((const char *)(browserDialog->m_playerName))
                    );
                    zOpt::SetPlayerName((const char *)(browserDialog->m_playerName));

                    if ((unsigned int)(statusFields.eventCode) > kMaxDirectMultiplayerEventCode) {
                        g_RecoilApp.m_pendingState = &g_RecoilApp.m_mpExitDialogState;
                        statusFields.eventCode = kDefaultMultiplayerEventCode;
                        zNetwork::RegisterPacketHandler(
                            kHudTimerAndFlagsSyncPacketType,
                            (zNetworkPacketHandler)&GameNet::HandlePkt14_HudTimerAndFlagsSync,
                            kDispatchModeSession
                        );
                    }

                    GameNet::SetStatusBitsFromFlags(statusFields.statusFlags);

                    union TimerSecondsBits {
                        float seconds;
                        int raw;
                    } timerSeconds = {(float)(statusFields.valueOrTime) * kSecondsPerMinute};
                    g_HudSensorTracker.SetRuntimeTimerSecAndGoalValue(
                        timerSeconds.raw,
                        statusFields.auxParam
                    );

                    g_RecoilApp.LoadZbdAndSetupSensorTracker(
                        statusFields.eventCode + kMultiplayerMissionBase,
                        0,
                        kFmvSkipEnabled,
                        m_useArchiveBanks
                    );
                    shouldShutdownNetwork = 0;
                }
            }
        }

        configDialog->Destructor();
        browserDialog->Destructor();
    }

    if (shouldShutdownNetwork != 0) {
        zNetwork::ShutdownSessionRuntime();
        zOpt::SetNetworkEnabled(kNetworkOptionDisabled);
    }
}

/**
 * Reimplements 0x431270: CZRecoilFrame::OnMenuStartMultiplayer.
 *
 * Purpose: start the default multiplayer mission setup path.
 */
void CZRecoilFrame::OnMenuStartMultiplayer() {
    g_RecoilApp.LoadZbdAndSetupSensorTracker(
        1,
        0,
        1,
        m_useArchiveBanks
    );
}

/**
 * Reimplements 0x431290: CZRecoilFrame::OnMenuStartCampaignMode.
 *
 * Purpose: start campaign mission slot 2 with the current archive-bank flag.
 */
void CZRecoilFrame::OnMenuStartCampaignMode() {
    g_RecoilApp.LoadZbdAndSetupSensorTracker(
        2,
        0,
        1,
        m_useArchiveBanks
    );
}

/**
 * Reimplements 0x4312b0: CZRecoilFrame::OnMenuStartCampaignMode2.
 *
 * Purpose: start campaign mission slot 3 with the current archive-bank flag.
 */
void CZRecoilFrame::OnMenuStartCampaignMode2() {
    g_RecoilApp.LoadZbdAndSetupSensorTracker(
        3,
        0,
        1,
        m_useArchiveBanks
    );
}

/**
 * Reimplements 0x4312d0: CZRecoilFrame::OnMenuStartCampaignMode3.
 *
 * Purpose: start campaign mission slot 4 with the current archive-bank flag.
 */
void CZRecoilFrame::OnMenuStartCampaignMode3() {
    g_RecoilApp.LoadZbdAndSetupSensorTracker(
        4,
        0,
        1,
        m_useArchiveBanks
    );
}

/**
 * Reimplements 0x4312f0: CZRecoilFrame::OnMenuStartCampaignMode4.
 *
 * Purpose: start campaign mission slot 5 with the current archive-bank flag.
 */
void CZRecoilFrame::OnMenuStartCampaignMode4() {
    g_RecoilApp.LoadZbdAndSetupSensorTracker(
        5,
        0,
        1,
        m_useArchiveBanks
    );
}

/**
 * Reimplements 0x431310: CZRecoilFrame::OnMenuStartCampaignMode5.
 *
 * Purpose: start campaign mission slot 6 with the current archive-bank flag.
 */
void CZRecoilFrame::OnMenuStartCampaignMode5() {
    g_RecoilApp.LoadZbdAndSetupSensorTracker(
        6,
        0,
        1,
        m_useArchiveBanks
    );
}

/**
 * Reimplements 0x4319a0: CZRecoilFrame::OnMenuWestwoodOnlineUpgrade.
 * Source file evidence: D:\Proj\Battlesport\CZRecoilFrame.cpp.
 * Purpose: gate the Westwood Online upgrade flow on Winsock2 readiness and
 * launch the selected mission.
 */
RECOIL_NO_GS void CZRecoilFrame::OnMenuWestwoodOnlineUpgrade() {
    int canShowUpgrade = 1;
    if (g_CZRecoilFrame_WestwoodOnlineWinsockChecked == 0) {
        char caption[0x100];
        char messageFormat[0x200];

        g_CZRecoilFrame_WestwoodOnlineWinsockChecked = 1;
        strcpy(
            caption,
            zLoc::GetMessageString(18)
        );
        strcpy(
            messageFormat,
            zLoc::GetMessageString(38)
        );
        if (NetUi::VerifyWinsock2OrPromptContinue(
            caption,
            messageFormat
        ) == 0) {
            canShowUpgrade = 0;
        }
    }

    if (canShowUpgrade == 0) {
        return;
    }

    g_RecoilApp.m_skipIntroFmv = 1;
    g_RecoilApp.m_missionFmvState.m_skipMissionFmv = 1;

    int selectedMissionIndex;
    if (WestwoodOnlineUpgradeDialog::ShowModalAndGetSelectedMissionIndex(&selectedMissionIndex) !=
        0) {
        g_RecoilApp.LoadZbdAndSetupSensorTracker(
            selectedMissionIndex + 6,
            0,
            1,
            g_HudSensorTracker.missionFlags
        );
    }
}

/**
 * Reimplements 0x431330: CZRecoilFrame::OnMenuToggleArchiveBanks.
 *
 * Purpose: toggle archive-bank loading and mirror it into audio/HUD state.
 */
void CZRecoilFrame::OnMenuToggleArchiveBanks() {
    m_useArchiveBanks = m_useArchiveBanks == 0 ? 1 : 0;
    CheckMenuItem(
        m_mainMenu.m_hMenu,
        0x9c6b,
        m_useArchiveBanks == 0 ? MF_UNCHECKED : MF_CHECKED
    );
    g_HudSensorTracker.missionFlags = m_useArchiveBanks;
    zSnd::SetUseArchiveBanksFlag(m_useArchiveBanks);
}

/**
 * Reimplements 0x431380: CZRecoilFrame::OnMenuToggleTexturePacks.
 *
 * Purpose: toggle texture-pack loading and update the menu check state.
 */
void CZRecoilFrame::OnMenuToggleTexturePacks() {
    if (zVid::GetTexturePackLoadState() != 0) {
        zVid::SetTexturePackLoadState(0);
        CheckMenuItem(
            m_mainMenu.m_hMenu,
            0x9c7b,
            MF_UNCHECKED
        );
        return;
    }

    zVid::SetTexturePackLoadState(1);
    CheckMenuItem(
        m_mainMenu.m_hMenu,
        0x9c7b,
        MF_CHECKED
    );
}

/**
 * Reimplements 0x4313d0: CZRecoilFrame::OnUpdateVideoMode2CmdUI.
 *
 * Purpose: apply cached command UI state for video mode 2.
 */
void CZRecoilFrame::OnUpdateVideoMode2CmdUI(
    CCmdUI *cmdUi
) {
    UpdateCmdUiFromState(
        cmdUi,
        m_videoModeCmdUiState[0]
    );
}

/**
 * Reimplements 0x431430: CZRecoilFrame::OnUpdateVideoMode3CmdUI.
 *
 * Purpose: apply cached command UI state for video mode 3.
 */
void CZRecoilFrame::OnUpdateVideoMode3CmdUI(
    CCmdUI *cmdUi
) {
    UpdateCmdUiFromState(
        cmdUi,
        m_videoModeCmdUiState[1]
    );
}

/**
 * Reimplements 0x431490: CZRecoilFrame::OnUpdateVideoMode4CmdUI.
 *
 * Purpose: apply cached command UI state for video mode 4.
 */
void CZRecoilFrame::OnUpdateVideoMode4CmdUI(
    CCmdUI *cmdUi
) {
    UpdateCmdUiFromState(
        cmdUi,
        m_videoModeCmdUiState[2]
    );
}

/**
 * Reimplements 0x4314f0: CZRecoilFrame::OnUpdateVideoMode5CmdUI.
 *
 * Purpose: apply cached command UI state for video mode 5.
 */
void CZRecoilFrame::OnUpdateVideoMode5CmdUI(
    CCmdUI *cmdUi
) {
    UpdateCmdUiFromState(
        cmdUi,
        m_videoModeCmdUiState[3]
    );
}

/**
 * Reimplements 0x431550: CZRecoilFrame::OnUpdateVideoMode6CmdUI.
 *
 * Purpose: apply cached command UI state for video mode 6.
 */
void CZRecoilFrame::OnUpdateVideoMode6CmdUI(
    CCmdUI *cmdUi
) {
    UpdateCmdUiFromState(
        cmdUi,
        m_videoModeCmdUiState[4]
    );
}

/**
 * Reimplements 0x4315b0: CZRecoilFrame::OnUpdateVideoMode7CmdUI.
 *
 * Purpose: apply cached command UI state for video mode 7.
 */
void CZRecoilFrame::OnUpdateVideoMode7CmdUI(
    CCmdUI *cmdUi
) {
    UpdateCmdUiFromState(
        cmdUi,
        m_videoModeCmdUiState[5]
    );
}

/**
 * Reimplements 0x431790: CZRecoilFrame::OnMenuSelectHwApi0.
 *
 * Purpose: select the software/fallback hardware API menu path.
 */
void CZRecoilFrame::OnMenuSelectHwApi0() {
    EnsureHwApiInitialized(0);
}

/**
 * Reimplements 0x4317a0: CZRecoilFrame::OnMenuSelectHwApi1.
 *
 * Purpose: select hardware API menu entry 1.
 */
void CZRecoilFrame::OnMenuSelectHwApi1() {
    EnsureHwApiInitialized(1);
}

/**
 * Reimplements 0x4317b0: CZRecoilFrame::OnMenuSelectHwApi2.
 *
 * Purpose: select hardware API menu entry 2.
 */
void CZRecoilFrame::OnMenuSelectHwApi2() {
    EnsureHwApiInitialized(2);
}

/**
 * Reimplements 0x4317c0: CZRecoilFrame::OnMenuSelectHwApi3.
 *
 * Purpose: select hardware API menu entry 3.
 */
void CZRecoilFrame::OnMenuSelectHwApi3() {
    EnsureHwApiInitialized(3);
}

/**
 * Reimplements 0x4317d0: CZRecoilFrame::UpdateHwApiMenuItem.
 *
 * Purpose: remove unavailable hardware API commands or update their label/check state.
 */
RECOIL_NO_GS void CZRecoilFrame::UpdateHwApiMenuItem(
    CCmdUI *cmdUi,
    int apiIndex
) {
    if (m_acceptedD3DDeviceCount < apiIndex) {
        RemoveMenu(
            cmdUi->m_pMenu->m_hMenu,
            m_hwApiMenuCommandIds[apiIndex],
            MF_BYCOMMAND
        );
        return;
    }

    cmdUi->SetCheck(m_hwApiCmdUiState[apiIndex] == kCmdUiChecked ? 1 : 0);

    char menuLabelText[0x40];
    sprintf(
        menuLabelText,
        g_CZRecoilFrame_AcceleratorMenuLabelFmt,
        zVid::GetHwApiDescription(apiIndex - 1),
        zVid::GetHwApiDriverName(apiIndex - 1)
    );
    cmdUi->SetText(menuLabelText);
}

/**
 * Reimplements 0x431870: CZRecoilFrame::OnUpdateHwApi0CmdUI.
 *
 * Purpose: enable and check the software/fallback hardware API command.
 */
void CZRecoilFrame::OnUpdateHwApi0CmdUI(
    CCmdUI *cmdUi
) {
    cmdUi->Enable(1);
    cmdUi->SetCheck(m_hwApiCmdUiState[0] == kCmdUiChecked ? 1 : 0);
}

/**
 * Reimplements 0x4318b0: CZRecoilFrame::OnUpdateHwApi1CmdUI.
 *
 * Purpose: update hardware API command UI entry 1.
 */
void CZRecoilFrame::OnUpdateHwApi1CmdUI(
    CCmdUI *cmdUi
) {
    UpdateHwApiMenuItem(
        cmdUi,
        1
    );
}

/**
 * Reimplements 0x4318c0: CZRecoilFrame::OnUpdateHwApi2CmdUI.
 *
 * Purpose: update hardware API command UI entry 2.
 */
void CZRecoilFrame::OnUpdateHwApi2CmdUI(
    CCmdUI *cmdUi
) {
    UpdateHwApiMenuItem(
        cmdUi,
        2
    );
}

/**
 * Reimplements 0x4318d0: CZRecoilFrame::OnUpdateHwApi3CmdUI.
 *
 * Purpose: update hardware API command UI entry 3.
 */
void CZRecoilFrame::OnUpdateHwApi3CmdUI(
    CCmdUI *cmdUi
) {
    UpdateHwApiMenuItem(
        cmdUi,
        3
    );
}

/**
 * Reimplements 0x4318e0: CZRecoilFrame::OnUpdateFullscreenCmdUI.
 *
 * Purpose: remove the fullscreen command from the update menu path.
 */
void CZRecoilFrame::OnUpdateFullscreenCmdUI(
    CCmdUI *cmdUi
) {
    RemoveMenu(
        cmdUi->m_pMenu->m_hMenu,
        kFullscreenMenuCommandId,
        MF_BYCOMMAND
    );
}

/**
 * Original helper evidence: CZRecoilFrame message-map entries for command ids
 * 0x9c7f, 0x9c81, and 0x9c84 share the same one-argument enable handler.
 * Purpose: Enable command UI entries that have no authored state gate.
 */
void CZRecoilFrame::OnUpdateAlwaysEnabledCmdUI(
    CCmdUI *cmdUi
) {
    MfcCmdUI::EnableAlways(cmdUi);
}

/**
 * Reimplements 0x431900: CZRecoilFrame::OnMenuToggleCDAudio.
 *
 * Purpose: toggle the CD audio option from the frame menu.
 */
void CZRecoilFrame::OnMenuToggleCDAudio() {
    zSnd::SetCDAudioOption(zSnd::GetCDAudioOption() == 0 ? 1 : 0);
}

/**
 * Reimplements 0x431920: CZRecoilFrame::OnUpdateCDAudioCmdUI.
 *
 * Purpose: enable and check the CD audio command from sound options.
 */
void CZRecoilFrame::OnUpdateCDAudioCmdUI(
    CCmdUI *cmdUi
) {
    cmdUi->Enable(1);
    cmdUi->SetCheck(zSnd::GetCDAudioOption() != 0 ? 1 : 0);
}

/**
 * Reimplements 0x431950: CZRecoilFrame::OnMenuToggleJoystick.
 *
 * Purpose: toggle joystick input from the frame menu.
 */
void CZRecoilFrame::OnMenuToggleJoystick() {
    zInp::SetJoystickOption(zInp::GetJoystickOption() == 0 ? 1 : 0);
}

/**
 * Reimplements 0x431970: CZRecoilFrame::OnUpdateJoystickCmdUI.
 *
 * Purpose: enable and check the joystick command from input options.
 */
void CZRecoilFrame::OnUpdateJoystickCmdUI(
    CCmdUI *cmdUi
) {
    cmdUi->Enable(1);
    cmdUi->SetCheck(zInp::GetJoystickOption() != 0 ? 1 : 0);
}

/**
 * Reimplements 0x431a90: CZRecoilFrame::OnMenuSelectDirectSound.
 *
 * Purpose: select DirectSound as the active audio API option.
 */
void CZRecoilFrame::OnMenuSelectDirectSound() {
    zSnd::SetAudioApiOption(0);
}

/**
 * Reimplements 0x431aa0: CZRecoilFrame::OnUpdateDirectSoundCmdUI.
 *
 * Purpose: enable and check the DirectSound command from audio options.
 */
void CZRecoilFrame::OnUpdateDirectSoundCmdUI(
    CCmdUI *cmdUi
) {
    cmdUi->Enable(1);
    cmdUi->SetCheck(zSnd::GetAudioApiOption() == 0 ? 1 : 0);
}

/**
 * Reimplements 0x431ad0: CZRecoilFrame::OnMenuSelectA3D.
 *
 * Purpose: select A3D as the active audio API option.
 */
void CZRecoilFrame::OnMenuSelectA3D() {
    zSnd::SetAudioApiOption(1);
}

/**
 * Reimplements 0x431ae0: CZRecoilFrame::OnUpdateA3DCmdUI.
 *
 * Purpose: enable and check the A3D command from the active sound backend.
 */
void CZRecoilFrame::OnUpdateA3DCmdUI(
    CCmdUI *cmdUi
) {
    cmdUi->Enable(1);
    cmdUi->SetCheck(zSnd::GetActiveBackend() == 1 ? 1 : 0);
}

/**
 * Reimplements 0x431b10: CZRecoilFrame::OnSize.
 *
 * Purpose: forward sizing to CZGameFrame and deactivate the app on minimized/iconic states.
 */
void CZRecoilFrame::OnSize(
    unsigned int nType,
    int cx,
    int cy
) {
    CZGameFrame::OnSize(
        nType,
        cx,
        cy
    );

    if (nType == 4 || nType == 1) {
        m_app->OnAppDeactivate();
    }
}

/**
 * Reimplements 0x431610: CZRecoilFrame::SetHwApiAndInitMode.
 *
 * Purpose: select a hardware API, query video memory, force accelerated mode, and enter the default hardware video mode.
 */
void CZRecoilFrame::SetHwApiAndInitMode(
    int hwApiIndex
) {
    zVid::SetHwApiOption(zVideo::SelectHwApiDeviceOrFallback(hwApiIndex));
    g_zVideo_pfnQueryDeviceVideoMemoryBytes(
        hwApiIndex,
        &m_vidMemTotalBytes,
        (int *)(&m_vidMemFreeBytes)
    );
    m_fullscreenOption = zOpt::GetFullscreenOption();
    zOpt::SetFullscreenOption(1);
    zVid::SetAccelerationOption(1);
    m_videoModeIndex = zVid::GetVideoModeIndexFromOptions();
    OnMenuSetVideoMode5();
}

/**
 * Reimplements 0x431680: CZRecoilFrame::InitFallbackMode.
 *
 * Purpose: restore software/fallback renderer options and rebuild mode command state.
 */
void CZRecoilFrame::InitFallbackMode() {
    zVid::SetHwApiOption(zVideo::SelectHwApiDeviceOrFallback(-1));
    zVid::SetAccelerationOption(0);
    zOpt::SetFullscreenOption(m_fullscreenOption);
    zVid::SetVideoModeIndex(m_videoModeIndex);
    ConfigureModeFeatureFlags();
}

/**
 * Reimplements 0x4316c0: CZRecoilFrame::EnsureHwApiInitialized.
 *
 * Purpose: initialize the selected hardware API once and clear competing menu checks.
 */
void CZRecoilFrame::EnsureHwApiInitialized(
    int hwApiSelector
) {
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
 * Reimplements 0x431730: CZRecoilFrame::InitStartupHwApiFromOptions.
 *
 * Purpose: select the startup renderer path from saved options or fallback defaults.
 */
void CZRecoilFrame::InitStartupHwApiFromOptions() {
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
