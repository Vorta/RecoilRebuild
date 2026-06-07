#include "Battlesport/CZRecoilFrame.h"

#include "Battlesport/CZGameFrame.h"
#include "Battlesport/GameNet.h"
#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/NetUi.h"
#include "Battlesport/Recoil.h"
#include "Battlesport/RecoilApp.h"
#include "Battlesport/WestwoodOnlineUpgradeDialog.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zVideo/zVideo.h"

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
HINSTANCE g_RecoilApp_hInstance = 0;
int g_CZRecoilFrame_HasWolApi = 0;
int g_CZRecoilFrame_WestwoodOnlineWinsockChecked = 0;
HudSensorTracker g_HudSensorTracker;
}

namespace {
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
const char *kMainMenuResourceName = "MYMENU";

// Source-faithful helper recovered from address-backed callers in this source file.
unsigned int Ptr32FromSymbol(
    const void *symbol
) {
    return (unsigned int)((unsigned int)(symbol));
}

// Source-faithful helper recovered from address-backed callers in this source file.
CRuntimeClass *__stdcall GetCZRecoilFrameBaseRuntimeClass() {
    return &CZGameFrame::classCZGameFrame;
}

// Source-faithful helper recovered from address-backed callers in this source file.
const AFX_MSGMAP *__stdcall GetCZRecoilFrameBaseMessageMap() {
    return &CZGameFrame::messageMap;
}

// Source-faithful helper recovered from address-backed callers in this source file.
int CommandCheckedIfMode(
    int currentMode,
    int targetMode
) {
    return currentMode == targetMode ? kCmdUiChecked : 0;
}

// Source-faithful helper recovered from address-backed callers in this source file.
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

// Source-faithful helper recovered from address-backed callers in this source file.
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

AFX_MSGMAP_ENTRY const CZRecoilFrame::messageEntries[] = {
    {0, 0, 0, 0, 0, 0},
};

const AFX_MSGMAP CZRecoilFrame::messageMap = {
    &GetCZRecoilFrameBaseMessageMap,
    &CZRecoilFrame::messageEntries[0],
};

CRuntimeClass CZRecoilFrame::classCZRecoilFrame = {
    "CZRecoilFrame",
    sizeof(CZRecoilFrame),
    0xffff,
    0,
    &GetCZRecoilFrameBaseRuntimeClass,
    0,
};

// Source-faithful helper recovered from address-backed callers in this source file.
unsigned int CZRecoilFrame::GetBaseRuntimeClass() {
    return Ptr32FromSymbol(&CZGameFrame::classCZGameFrame);
}

// Reimplements 0x4301e0: CZRecoilFrame::CreateObject
CZRecoilFrame *CZRecoilFrame::CreateObject() {
    CZRecoilFrame *const frame = (CZRecoilFrame *)(::operator new(sizeof(CZRecoilFrame)));
    if (frame == 0) {
        return 0;
    }

    try {
        return frame->Constructor();
    } catch (...) {
        ::operator delete(frame);
        throw;
    }
}

// Reimplements 0x430240: CZRecoilFrame::GetRuntimeClass
unsigned int CZRecoilFrame::GetRuntimeClass() {
    return Ptr32FromSymbol(&CZRecoilFrame::classCZRecoilFrame);
}

// Source-faithful helper recovered from address-backed callers in this source file.
unsigned int CZRecoilFrame::GetBaseMessageMap() {
    return Ptr32FromSymbol(&CZGameFrame::messageMap);
}

// Reimplements 0x4306e0: CZRecoilFrame::GetMessageMap
unsigned int CZRecoilFrame::GetMessageMap() {
    return Ptr32FromSymbol(&CZRecoilFrame::messageMap);
}

namespace MfcCmdUI {
// Reimplements 0x431a80: MfcCmdUI::EnableAlways
void __stdcall EnableAlways(
    CCmdUI *cmdUi
) {
    cmdUi->Enable(1);
}
} // namespace MfcCmdUI

// Reimplements 0x430250: CZRecoilFrame::Constructor
CZRecoilFrame * CZRecoilFrame::Constructor() {
    ((CZGameFrame *)(this))->Constructor("recoil");
    new (&m_mainMenu) CMenu();

    unsigned long
        titleStorage[(sizeof(CString) + sizeof(unsigned long) - 1) / sizeof(unsigned long)];
    CString *title = (CString *)(titleStorage);
    BuildWindowTitle(title);
    const int windowHeight = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYMENU) +
                             (GetSystemMetrics(SM_CYFRAME) << 1) + 0x1e0;
    const int windowWidth = (GetSystemMetrics(SM_CXFRAME) << 1) + 0x280;
    ((CWnd *)(this))
        ->CreateEx(
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
            "/campaigns",
            4
        ) == 0) {
            m_campaignsOnlyMode = 1;
        } else if (strncmp(
            token,
            "1234567890",
            4
        ) == 0) {
            m_cmdlineFlag = 0;
        }
    }
    free(commandLineCopy);

    zError::InitOutputContext(
        m_hWnd,
        0xe00,
        "recoil.err"
    );
    m_mainMenu.Attach(LoadMenuA(
        AfxFindResourceHandle(
            kMainMenuResourceName,
            MAKEINTRESOURCEA(4)
        ),
        kMainMenuResourceName
    ));
    SetMenu(
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
            "Software\\Westwood\\WOLAPI\\4352",
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
    return this;
}

// Reimplements 0x430610: CZRecoilFrame::Destructor
void CZRecoilFrame::Destructor() {
    m_mainMenu.DestroyMenu();
    m_mainMenu.CMenu::~CMenu();
    ((CZGameFrame *)(this))->Destructor();
}

// Reimplements 0x430680: CZRecoilFrame::SetMenuBarVisibility
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
    SetMenu(
        m_hWnd,
        menu
    );
}

// Reimplements 0x4306f0: CZRecoilFrame::BuildWindowTitle
CString * CZRecoilFrame::BuildWindowTitle(
    CString *outTitle
) {
    volatile int constructedTitleState = 0;
    if (g_zVideo_ActiveRendererPath == kRendererBackend3dfx) {
        outTitle->CString::CString("RECOIL (3Dfx)");
        return outTitle;
    }

    outTitle->CString::CString("RECOIL");
    return outTitle;
}

// Reimplements 0x430740: CZRecoilFrame::OnMenuStartSinglePlayer
void CZRecoilFrame::OnMenuStartSinglePlayer() {
    g_RecoilApp.m_skipIntroFmv = 0;
    g_RecoilApp.m_missionFmvState.m_skipMissionFmv = 0;
    g_RecoilApp.LoadZbdAndStartEngine();
}

// Reimplements 0x430760: CZRecoilFrame::OnMenuOpenCampaign
void CZRecoilFrame::OnMenuOpenCampaign() {
    g_RecoilApp.m_skipIntroFmv = 1;
    OnOpenFileDialog();
}

// Reimplements 0x430770: CZRecoilFrame::OnOpenFileDialog
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
    ofn.lpstrDefExt = "gs";

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

    InvalidateRect(
        m_hWnd,
        0,
        TRUE
    );
}

// Reimplements 0x4308c0: CZRecoilFrame::ConfigureModeFeatureFlags
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

// Reimplements 0x4309b0: CZRecoilFrame::OnMenuSetVideoMode2
void CZRecoilFrame::OnMenuSetVideoMode2() {
    zVid::SetVideoModeIndex(2);
    ConfigureModeFeatureFlags();
}

// Reimplements 0x4309d0: CZRecoilFrame::OnMenuSetVideoMode3
void CZRecoilFrame::OnMenuSetVideoMode3() {
    zVid::SetVideoModeIndex(3);
    ConfigureModeFeatureFlags();
}

// Reimplements 0x4309f0: CZRecoilFrame::OnMenuSetVideoMode4
void CZRecoilFrame::OnMenuSetVideoMode4() {
    zVid::SetVideoModeIndex(4);
    ConfigureModeFeatureFlags();
}

// Reimplements 0x430a10: CZRecoilFrame::OnMenuSetVideoMode5
void CZRecoilFrame::OnMenuSetVideoMode5() {
    zVid::SetVideoModeIndex(5);
    ConfigureModeFeatureFlags();
}

// Reimplements 0x430a30: CZRecoilFrame::OnMenuSetVideoMode6
void CZRecoilFrame::OnMenuSetVideoMode6() {
    zVid::SetVideoModeIndex(6);
    ConfigureModeFeatureFlags();
}

// Reimplements 0x430a50: CZRecoilFrame::OnMenuSetVideoMode7
void CZRecoilFrame::OnMenuSetVideoMode7() {
    zVid::SetVideoModeIndex(7);
    ConfigureModeFeatureFlags();
}

// Reimplements 0x4308a0: CZRecoilFrame::OnMenuExitGame
void CZRecoilFrame::OnMenuExitGame() {
    PostMessageA(
        m_hWnd,
        WM_CLOSE,
        0,
        0
    );
}

// Reimplements 0x430a70: CZRecoilFrame::OnMenuToggleHud
void CZRecoilFrame::OnMenuToggleHud() {
    zOpt::SetHudVisibilityOption(zOpt::GetHudVisibilityOption() == 0 ? 1 : 0);
}

// Reimplements 0x430a90: CZRecoilFrame::OnUpdateHudCmdUI
void CZRecoilFrame::OnUpdateHudCmdUI(
    CCmdUI *cmdUi
) {
    cmdUi->Enable(1);
    cmdUi->SetCheck(zOpt::GetHudVisibilityOption());
}

// Reimplements 0x430ab0: CZRecoilFrame::OnMenuToggleFullscreen
void CZRecoilFrame::OnMenuToggleFullscreen() {
    zOpt::SetFullscreenOption(zOpt::GetFullscreenOption() == 0 ? 1 : 0);
}

// Reimplements 0x430ad0: CZRecoilFrame::OnMenuOpenHelpDocs
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

// Reimplements 0x430c30: CZRecoilFrame::OnMenuAbout (D:\Proj\Battlesport\CZRecoilFrame.cpp)
RECOIL_NO_GS void CZRecoilFrame::OnMenuAbout() {
    CAboutDlg aboutDlg;
    aboutDlg.CDialog::DoModal();
}

// Reimplements 0x430d80: CZRecoilFrame::OnMenuOpenMultiplayerSessionBrowser
// (D:\Proj\Battlesport\CZRecoilFrame.cpp)
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

        zNetwork::InitSessionRuntime(g_zNetwork_RecoilAppGuid);
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

// Reimplements 0x431270: CZRecoilFrame::OnMenuStartMultiplayer
void CZRecoilFrame::OnMenuStartMultiplayer() {
    g_RecoilApp.LoadZbdAndSetupSensorTracker(
        1,
        0,
        1,
        m_useArchiveBanks
    );
}

// Reimplements 0x431290: CZRecoilFrame::OnMenuStartCampaignMode
void CZRecoilFrame::OnMenuStartCampaignMode() {
    g_RecoilApp.LoadZbdAndSetupSensorTracker(
        2,
        0,
        1,
        m_useArchiveBanks
    );
}

// Reimplements 0x4312b0: CZRecoilFrame::OnMenuStartCampaignMode2
void CZRecoilFrame::OnMenuStartCampaignMode2() {
    g_RecoilApp.LoadZbdAndSetupSensorTracker(
        3,
        0,
        1,
        m_useArchiveBanks
    );
}

// Reimplements 0x4312d0: CZRecoilFrame::OnMenuStartCampaignMode3
void CZRecoilFrame::OnMenuStartCampaignMode3() {
    g_RecoilApp.LoadZbdAndSetupSensorTracker(
        4,
        0,
        1,
        m_useArchiveBanks
    );
}

// Reimplements 0x4312f0: CZRecoilFrame::OnMenuStartCampaignMode4
void CZRecoilFrame::OnMenuStartCampaignMode4() {
    g_RecoilApp.LoadZbdAndSetupSensorTracker(
        5,
        0,
        1,
        m_useArchiveBanks
    );
}

// Reimplements 0x431310: CZRecoilFrame::OnMenuStartCampaignMode5
void CZRecoilFrame::OnMenuStartCampaignMode5() {
    g_RecoilApp.LoadZbdAndSetupSensorTracker(
        6,
        0,
        1,
        m_useArchiveBanks
    );
}

// Reimplements 0x4319a0: CZRecoilFrame::OnMenuWestwoodOnlineUpgrade
// (D:\Proj\Battlesport\CZRecoilFrame.cpp)
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

// Reimplements 0x431330: CZRecoilFrame::OnMenuToggleArchiveBanks
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

// Reimplements 0x431380: CZRecoilFrame::OnMenuToggleTexturePacks
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

// Reimplements 0x4313d0: CZRecoilFrame::OnUpdateVideoMode2CmdUI
void CZRecoilFrame::OnUpdateVideoMode2CmdUI(
    CCmdUI *cmdUi
) {
    UpdateCmdUiFromState(
        cmdUi,
        m_videoModeCmdUiState[0]
    );
}

// Reimplements 0x431430: CZRecoilFrame::OnUpdateVideoMode3CmdUI
void CZRecoilFrame::OnUpdateVideoMode3CmdUI(
    CCmdUI *cmdUi
) {
    UpdateCmdUiFromState(
        cmdUi,
        m_videoModeCmdUiState[1]
    );
}

// Reimplements 0x431490: CZRecoilFrame::OnUpdateVideoMode4CmdUI
void CZRecoilFrame::OnUpdateVideoMode4CmdUI(
    CCmdUI *cmdUi
) {
    UpdateCmdUiFromState(
        cmdUi,
        m_videoModeCmdUiState[2]
    );
}

// Reimplements 0x4314f0: CZRecoilFrame::OnUpdateVideoMode5CmdUI
void CZRecoilFrame::OnUpdateVideoMode5CmdUI(
    CCmdUI *cmdUi
) {
    UpdateCmdUiFromState(
        cmdUi,
        m_videoModeCmdUiState[3]
    );
}

// Reimplements 0x431550: CZRecoilFrame::OnUpdateVideoMode6CmdUI
void CZRecoilFrame::OnUpdateVideoMode6CmdUI(
    CCmdUI *cmdUi
) {
    UpdateCmdUiFromState(
        cmdUi,
        m_videoModeCmdUiState[4]
    );
}

// Reimplements 0x4315b0: CZRecoilFrame::OnUpdateVideoMode7CmdUI
void CZRecoilFrame::OnUpdateVideoMode7CmdUI(
    CCmdUI *cmdUi
) {
    UpdateCmdUiFromState(
        cmdUi,
        m_videoModeCmdUiState[5]
    );
}

// Reimplements 0x431790: CZRecoilFrame::OnMenuSelectHwApi0
void CZRecoilFrame::OnMenuSelectHwApi0() {
    EnsureHwApiInitialized(0);
}

// Reimplements 0x4317a0: CZRecoilFrame::OnMenuSelectHwApi1
void CZRecoilFrame::OnMenuSelectHwApi1() {
    EnsureHwApiInitialized(1);
}

// Reimplements 0x4317b0: CZRecoilFrame::OnMenuSelectHwApi2
void CZRecoilFrame::OnMenuSelectHwApi2() {
    EnsureHwApiInitialized(2);
}

// Reimplements 0x4317c0: CZRecoilFrame::OnMenuSelectHwApi3
void CZRecoilFrame::OnMenuSelectHwApi3() {
    EnsureHwApiInitialized(3);
}

// Reimplements 0x4317d0: CZRecoilFrame::UpdateHwApiMenuItem
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
        "Accelerator - %s (%s)",
        zVid::GetHwApiDescription(apiIndex - 1),
        zVid::GetHwApiDriverName(apiIndex - 1)
    );
    cmdUi->SetText(menuLabelText);
}

// Reimplements 0x431870: CZRecoilFrame::OnUpdateHwApi0CmdUI
void CZRecoilFrame::OnUpdateHwApi0CmdUI(
    CCmdUI *cmdUi
) {
    cmdUi->Enable(1);
    cmdUi->SetCheck(m_hwApiCmdUiState[0] == kCmdUiChecked ? 1 : 0);
}

// Reimplements 0x4318b0: CZRecoilFrame::OnUpdateHwApi1CmdUI
void CZRecoilFrame::OnUpdateHwApi1CmdUI(
    CCmdUI *cmdUi
) {
    UpdateHwApiMenuItem(
        cmdUi,
        1
    );
}

// Reimplements 0x4318c0: CZRecoilFrame::OnUpdateHwApi2CmdUI
void CZRecoilFrame::OnUpdateHwApi2CmdUI(
    CCmdUI *cmdUi
) {
    UpdateHwApiMenuItem(
        cmdUi,
        2
    );
}

// Reimplements 0x4318d0: CZRecoilFrame::OnUpdateHwApi3CmdUI
void CZRecoilFrame::OnUpdateHwApi3CmdUI(
    CCmdUI *cmdUi
) {
    UpdateHwApiMenuItem(
        cmdUi,
        3
    );
}

// Reimplements 0x4318e0: CZRecoilFrame::OnUpdateFullscreenCmdUI
void CZRecoilFrame::OnUpdateFullscreenCmdUI(
    CCmdUI *cmdUi
) {
    RemoveMenu(
        cmdUi->m_pMenu->m_hMenu,
        kFullscreenMenuCommandId,
        MF_BYCOMMAND
    );
}

// Reimplements 0x431900: CZRecoilFrame::OnMenuToggleCDAudio
void CZRecoilFrame::OnMenuToggleCDAudio() {
    zSnd::SetCDAudioOption(zSnd::GetCDAudioOption() == 0 ? 1 : 0);
}

// Reimplements 0x431920: CZRecoilFrame::OnUpdateCDAudioCmdUI
void CZRecoilFrame::OnUpdateCDAudioCmdUI(
    CCmdUI *cmdUi
) {
    cmdUi->Enable(1);
    cmdUi->SetCheck(zSnd::GetCDAudioOption() != 0 ? 1 : 0);
}

// Reimplements 0x431950: CZRecoilFrame::OnMenuToggleJoystick
void CZRecoilFrame::OnMenuToggleJoystick() {
    zInp::SetJoystickOption(zInp::GetJoystickOption() == 0 ? 1 : 0);
}

// Reimplements 0x431970: CZRecoilFrame::OnUpdateJoystickCmdUI
void CZRecoilFrame::OnUpdateJoystickCmdUI(
    CCmdUI *cmdUi
) {
    cmdUi->Enable(1);
    cmdUi->SetCheck(zInp::GetJoystickOption() != 0 ? 1 : 0);
}

// Reimplements 0x431a90: CZRecoilFrame::OnMenuSelectDirectSound
void CZRecoilFrame::OnMenuSelectDirectSound() {
    zSnd::SetAudioApiOption(0);
}

// Reimplements 0x431aa0: CZRecoilFrame::OnUpdateDirectSoundCmdUI
void CZRecoilFrame::OnUpdateDirectSoundCmdUI(
    CCmdUI *cmdUi
) {
    cmdUi->Enable(1);
    cmdUi->SetCheck(zSnd::GetAudioApiOption() == 0 ? 1 : 0);
}

// Reimplements 0x431ad0: CZRecoilFrame::OnMenuSelectA3D
void CZRecoilFrame::OnMenuSelectA3D() {
    zSnd::SetAudioApiOption(1);
}

// Reimplements 0x431ae0: CZRecoilFrame::OnUpdateA3DCmdUI
void CZRecoilFrame::OnUpdateA3DCmdUI(
    CCmdUI *cmdUi
) {
    cmdUi->Enable(1);
    cmdUi->SetCheck(zSnd::GetActiveBackend() == 1 ? 1 : 0);
}

// Reimplements 0x431b10: CZRecoilFrame::OnSize
void CZRecoilFrame::OnSize(
    unsigned int nType,
    int cx,
    int cy
) {
    ((CZGameFrame *)(this))->OnSize(
        nType,
        cx,
        cy
    );

    if (nType == 4 || nType == 1) {
        ((CZGameFrame *)(this))->m_app->OnAppDeactivate();
    }
}

// Reimplements 0x431610: CZRecoilFrame::SetHwApiAndInitMode
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

// Reimplements 0x431680: CZRecoilFrame::InitFallbackMode
void CZRecoilFrame::InitFallbackMode() {
    zVid::SetHwApiOption(zVideo::SelectHwApiDeviceOrFallback(-1));
    zVid::SetAccelerationOption(0);
    zOpt::SetFullscreenOption(m_fullscreenOption);
    zVid::SetVideoModeIndex(m_videoModeIndex);
    ConfigureModeFeatureFlags();
}

// Reimplements 0x4316c0: CZRecoilFrame::EnsureHwApiInitialized
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

// Reimplements 0x431730: CZRecoilFrame::InitStartupHwApiFromOptions
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
