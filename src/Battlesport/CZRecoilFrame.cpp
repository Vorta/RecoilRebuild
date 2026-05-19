#include "Battlesport/CZRecoilFrame.h"

#include "Battlesport/CZGameFrame.h"
#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/Recoil.h"
#include "Battlesport/RecoilApp.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <commdlg.h>
#include <shellapi.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HINSTANCE RECOIL_STDCALL AfxFindResourceHandle(LPCSTR resourceName, LPCSTR resourceType);

class CWnd {
  public:
    BOOL CreateEx(DWORD dwExStyle, LPCSTR className, LPCSTR windowName, DWORD style, int x, int y,
                  int width, int height, HWND parent, HMENU menu, LPVOID param);
    void SetWindowTextA(LPCSTR text);
    void CenterWindow(CWnd *alternateOwner);
    int MessageBoxA(LPCSTR text, LPCSTR caption, UINT type);
};

class CMenu {
  public:
    BOOL Attach(HMENU menu);
    static CMenu *RECOIL_STDCALL FromHandle(HMENU menu);

    void *vftable;
    HMENU m_hMenu;
};

extern "C" {
extern HWND g_RecoilApp_hWndMain;
HINSTANCE g_RecoilApp_hInstance = 0;
int g_CZRecoilFrame_HasWolApi = 0;
HudSensorTracker g_HudSensorTracker;
}

namespace {
const int kRendererBackend3dfx = 2;
const int kCmdUiDisabled = 1;
const int kCmdUiChecked = 8;
const unsigned int kVidMem800x600Threshold = 0x2bf200;
const unsigned int kVidMem1024x768Threshold = 0x480000;
const unsigned int kFullscreenMenuCommandId = 0x9c4e;
const DWORD kMainWindowStyle = 0x82ca0000;
const char *kRecoilWndClassName = "RecoilClass";
const char *kMainMenuResourceName = "MYMENU";
const RecoilNamedVtable kCZRecoilFrame_Vtable = {"CZRecoilFrame vtable"};
const RecoilNamedVtable kCMenu_Vtable = {"CMenu vtable"};
const RecoilNamedVtable kCObject_Vtable = {"CObject vtable"};

typedef int (RECOIL_THISCALL *CMenuDestroyMenuProc)(void *);

unsigned int Ptr32FromSymbol(const void *symbol) {
    return static_cast<unsigned int>((unsigned int)(symbol));
}

CRuntimeClass *RECOIL_STDCALL GetCZRecoilFrameBaseRuntimeClass() {
    return &CZGameFrame::classCZGameFrame;
}

const AFX_MSGMAP *RECOIL_STDCALL GetCZRecoilFrameBaseMessageMap() {
    return &CZGameFrame::messageMap;
}

void CallMfcCMenuDestroyMenu(void *menu) {
    HMODULE mfc42 = GetModuleHandleA("MFC42.DLL");
    if (mfc42 == 0) {
        mfc42 = LoadLibraryA("MFC42.DLL");
    }

    if (mfc42 != 0) {
        CMenuDestroyMenuProc const destroyMenu = (CMenuDestroyMenuProc)(
            GetProcAddress(mfc42, "?DestroyMenu@CMenu@@QAEHXZ"));
        if (destroyMenu != 0) {
            destroyMenu(menu);
        }
    }
}

int CommandCheckedIfMode(int currentMode, int targetMode) {
    return currentMode == targetMode ? kCmdUiChecked : 0;
}

void UpdateCmdUiFromState(CZRecoilCmdUI *cmdUi, int state) {
    if (state == kCmdUiDisabled) {
        cmdUi->vftable->Enable(cmdUi, 0);
        cmdUi->vftable->SetCheck(cmdUi, 0);
        return;
    }

    cmdUi->vftable->Enable(cmdUi, 1);
    cmdUi->vftable->SetCheck(cmdUi, state == kCmdUiChecked ? 1 : 0);
}

HMENU SubMenuHandleOrNull(HMENU menu, int position) {
    return CMenu::FromHandle(GetSubMenu(menu, position))->m_hMenu;
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
    "CZRecoilFrame", sizeof(CZRecoilFrame), 0xffff, 0, &GetCZRecoilFrameBaseRuntimeClass,
    0,
};

RECOIL_FRAME_NOINLINE unsigned int RECOIL_CDECL CZRecoilFrame::GetBaseRuntimeClass() {
    return Ptr32FromSymbol(&CZGameFrame::classCZGameFrame);
}

// Reimplements 0x4301e0: CZRecoilFrame::CreateObject
RECOIL_FRAME_NOINLINE CZRecoilFrame *RECOIL_CDECL CZRecoilFrame::CreateObject() {
    CZRecoilFrame *const frame = static_cast<CZRecoilFrame *>(::operator new(sizeof(CZRecoilFrame)));
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

RECOIL_FRAME_NOINLINE unsigned int RECOIL_CDECL CZRecoilFrame::GetRuntimeClass() {
    return Ptr32FromSymbol(&CZRecoilFrame::classCZRecoilFrame);
}

RECOIL_FRAME_NOINLINE unsigned int RECOIL_CDECL CZRecoilFrame::GetBaseMessageMap() {
    return Ptr32FromSymbol(&CZGameFrame::messageMap);
}

// Reimplements 0x4306e0: CZRecoilFrame::GetMessageMap
RECOIL_FRAME_NOINLINE unsigned int RECOIL_CDECL CZRecoilFrame::GetMessageMap() {
    return Ptr32FromSymbol(&CZRecoilFrame::messageMap);
}

namespace MfcCmdUI {
// Reimplements 0x431a80: MfcCmdUI::EnableAlways
RECOIL_FRAME_NOINLINE void RECOIL_STDCALL EnableAlways(CZRecoilCmdUI *cmdUi) {
    cmdUi->vftable->Enable(cmdUi, 1);
}
} // namespace MfcCmdUI

// Reimplements 0x430250: CZRecoilFrame::Constructor
RECOIL_FRAME_NOINLINE CZRecoilFrame *RECOIL_THISCALL CZRecoilFrame::Constructor() {
    ((CZGameFrame *)(this))->Constructor("recoil");
    m_mainMenuVftable = Ptr32FromSymbol(&kCMenu_Vtable);
    m_mainMenuHandle = 0;
    vftable = Ptr32FromSymbol(&kCZRecoilFrame_Vtable);

    unsigned long titleStorage[(sizeof(CString) + sizeof(unsigned long) - 1) /
                               sizeof(unsigned long)];
    CString *title = (CString *)(titleStorage);
    BuildWindowTitle(title);
    const int windowHeight = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYMENU) +
                             (GetSystemMetrics(SM_CYFRAME) << 1) + 0x1e0;
    const int windowWidth = (GetSystemMetrics(SM_CXFRAME) << 1) + 0x280;
    ((CWnd *)(this))->CreateEx(0x20000, kRecoilWndClassName, title->m_pchData,
                                             kMainWindowStyle, CW_USEDEFAULT, CW_USEDEFAULT,
                                             windowWidth, windowHeight, 0, 0, 0);
    title->~CString();

    m_cmdlineFlag = 1;
    m_campaignsOnlyMode = 0;
    char *commandLineCopy = _strdup(GetCommandLineA());
    for (char *token = strtok(commandLineCopy, " "); token != 0;
         token = strtok(0, " ")) {
        if (strncmp(token, "/campaigns", 4) == 0) {
            m_campaignsOnlyMode = 1;
        } else if (strncmp(token, "1234567890", 4) == 0) {
            m_cmdlineFlag = 0;
        }
    }
    free(commandLineCopy);

    zError::InitOutputContext(m_hWnd, 0xe00, "recoil.err");
    CMenu *const mainMenu = (CMenu *)(&m_mainMenuVftable);
    mainMenu->Attach(LoadMenuA(AfxFindResourceHandle(kMainMenuResourceName, MAKEINTRESOURCEA(4)),
                               kMainMenuResourceName));
    m_mainMenuHandle = mainMenu->m_hMenu;
    SetMenu(m_hWnd, m_mainMenuHandle);

    if (m_campaignsOnlyMode != 0) {
        RemoveMenu(SubMenuHandleOrNull(m_mainMenuHandle, 1), 0x9c6b, MF_BYCOMMAND);
        RemoveMenu(SubMenuHandleOrNull(m_mainMenuHandle, 1), 0x9c7b, MF_BYCOMMAND);
    } else {
        RemoveMenu(m_mainMenuHandle, 1, MF_BYPOSITION);
    }

    RemoveMenu(SubMenuHandleOrNull(m_mainMenuHandle, 2), kFullscreenMenuCommandId, MF_BYCOMMAND);

    g_RecoilApp_hInstance =
        (HINSTANCE)(static_cast<unsigned int>(g_RecoilApp.m_hInstance_6c));
    g_RecoilApp_hWndMain = m_hWnd;

    unsigned long formattedTitleStorage[(sizeof(CString) + sizeof(unsigned long) - 1) /
                                        sizeof(unsigned long)];
    CString *formattedTitle = (CString *)(formattedTitleStorage);
    new (formattedTitle) CString();
    unsigned long titleCopyStorage[(sizeof(CString) + sizeof(unsigned long) - 1) /
                                   sizeof(unsigned long)];
    CString *titleCopy = (CString *)(titleCopyStorage);
    BuildWindowTitle(titleCopy);
    formattedTitle->Format("%s", titleCopy->m_pchData);
    titleCopy->~CString();
    ((CWnd *)(this))->SetWindowTextA(formattedTitle->m_pchData);
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

    CheckMenuItem(m_mainMenuHandle, 0x9c7b, zVid::GetTexturePackLoadState() == 0 ? 0 : MF_CHECKED);

    g_HudSensorTracker.missionFlags = m_useArchiveBanks;
    zSnd::SetUseArchiveBanksFlag(m_useArchiveBanks);
    m_acceptedD3DDeviceCount = zVid::GetAcceptedHardwareRendererCount_Cached();

    HKEY wolApiRegKey = 0;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Westwood\\WOLAPI\\4352", 0, KEY_READ,
                      &wolApiRegKey) == ERROR_SUCCESS) {
        g_CZRecoilFrame_HasWolApi = 1;
        RegCloseKey(wolApiRegKey);
    }

    ((CWnd *)(this))->CenterWindow(0);
    SetCursor(LoadCursorA(0, IDC_ARROW));
    return this;
}

RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::Destructor() {
    vftable = Ptr32FromSymbol(&kCZRecoilFrame_Vtable);
    m_mainMenuVftable = Ptr32FromSymbol(&kCMenu_Vtable);
    CallMfcCMenuDestroyMenu(&m_mainMenuVftable);
    m_mainMenuVftable = Ptr32FromSymbol(&kCObject_Vtable);
    ((CZGameFrame *)(this))->Destructor();
}

// Reimplements 0x430680: CZRecoilFrame::SetMenuBarVisibility
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL
CZRecoilFrame::SetMenuBarVisibility(int visible) {
    LONG style = GetWindowLongA(m_hWnd, GWL_STYLE);
    HMENU menu = 0;
    if (visible != 0) {
        style |= static_cast<LONG>(0x82ca0000);
        menu = m_mainMenuHandle;
    } else {
        style &= static_cast<LONG>(0xfff7ffff);
    }

    SetWindowLongA(m_hWnd, GWL_STYLE, style);
    SetMenu(m_hWnd, menu);
}

// Reimplements 0x4306f0: CZRecoilFrame::BuildWindowTitle
RECOIL_FRAME_NOINLINE CString *RECOIL_THISCALL CZRecoilFrame::BuildWindowTitle(CString *outTitle) {
    if (g_zVideo_ActiveRendererPath == kRendererBackend3dfx) {
        new (outTitle) CString("RECOIL (3Dfx)");
        return outTitle;
    }

    new (outTitle) CString("RECOIL");
    return outTitle;
}

// Reimplements 0x430740: CZRecoilFrame::OnMenuStartSinglePlayer
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuStartSinglePlayer() {
    g_RecoilApp.m_skipIntroFmv = 0;
    g_RecoilApp.m_missionFmvState_1d8.m_skipMissionFmv = 0;
    g_RecoilApp.LoadZbdAndStartEngine();
}

// Reimplements 0x430760: CZRecoilFrame::OnMenuOpenCampaign
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuOpenCampaign() {
    g_RecoilApp.m_skipIntroFmv = 1;
    OnOpenFileDialog();
}

// Reimplements 0x430770: CZRecoilFrame::OnOpenFileDialog
RECOIL_FRAME_NOINLINE RECOIL_NO_GS void RECOIL_THISCALL CZRecoilFrame::OnOpenFileDialog() {
    char filter[0x100];
    const int filterLength = LoadStringA(g_RecoilApp_hInstance, 0xc8, filter, sizeof(filter));
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
    memset(&ofn, 0, sizeof(ofn));
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
        strcpy(m_openZbdFilePath, ofn.lpstrFile);
        g_RecoilApp.LoadZbdAndSetupSensorTracker(0, m_openZbdFilePath, 1, 1);
    }

    InvalidateRect(m_hWnd, 0, TRUE);
}

// Reimplements 0x4308c0: CZRecoilFrame::ConfigureModeFeatureFlags
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::ConfigureModeFeatureFlags() {
    const int mode = zVid::GetVideoModeIndexFromOptions();

    if (zVid::GetAccelerationOption() == 0) {
        m_videoModeCmdUiState[0] = CommandCheckedIfMode(mode, 2);
        m_videoModeCmdUiState[1] = CommandCheckedIfMode(mode, 3);
        m_videoModeCmdUiState[2] = CommandCheckedIfMode(mode, 4);
        m_videoModeCmdUiState[3] = CommandCheckedIfMode(mode, 5);
        m_videoModeCmdUiState[4] = CommandCheckedIfMode(mode, 6);
        m_videoModeCmdUiState[5] = CommandCheckedIfMode(mode, 7);
        return;
    }

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
}

// Reimplements 0x4309b0: CZRecoilFrame::OnMenuSetVideoMode2
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuSetVideoMode2() {
    zVid::SetVideoModeIndex(2);
    ConfigureModeFeatureFlags();
}

// Reimplements 0x4309d0: CZRecoilFrame::OnMenuSetVideoMode3
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuSetVideoMode3() {
    zVid::SetVideoModeIndex(3);
    ConfigureModeFeatureFlags();
}

// Reimplements 0x4309f0: CZRecoilFrame::OnMenuSetVideoMode4
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuSetVideoMode4() {
    zVid::SetVideoModeIndex(4);
    ConfigureModeFeatureFlags();
}

// Reimplements 0x430a10: CZRecoilFrame::OnMenuSetVideoMode5
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuSetVideoMode5() {
    zVid::SetVideoModeIndex(5);
    ConfigureModeFeatureFlags();
}

// Reimplements 0x430a30: CZRecoilFrame::OnMenuSetVideoMode6
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuSetVideoMode6() {
    zVid::SetVideoModeIndex(6);
    ConfigureModeFeatureFlags();
}

// Reimplements 0x430a50: CZRecoilFrame::OnMenuSetVideoMode7
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuSetVideoMode7() {
    zVid::SetVideoModeIndex(7);
    ConfigureModeFeatureFlags();
}

// Reimplements 0x4308a0: CZRecoilFrame::OnMenuExitGame
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuExitGame() {
    PostMessageA(m_hWnd, WM_CLOSE, 0, 0);
}

// Reimplements 0x430a70: CZRecoilFrame::OnMenuToggleHud
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuToggleHud() {
    zOpt::SetHudVisibilityOption(zOpt::GetHudVisibilityOption() == 0 ? 1 : 0);
}

// Reimplements 0x430a90: CZRecoilFrame::OnUpdateHudCmdUI
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnUpdateHudCmdUI(CZRecoilCmdUI *cmdUi) {
    cmdUi->vftable->Enable(cmdUi, 1);
    cmdUi->vftable->SetCheck(cmdUi, zOpt::GetHudVisibilityOption());
}

// Reimplements 0x430ab0: CZRecoilFrame::OnMenuToggleFullscreen
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuToggleFullscreen() {
    zOpt::SetFullscreenOption(zOpt::GetFullscreenOption() == 0 ? 1 : 0);
}

// Reimplements 0x430ad0: CZRecoilFrame::OnMenuOpenHelpDocs
RECOIL_FRAME_NOINLINE RECOIL_NO_GS void RECOIL_THISCALL CZRecoilFrame::OnMenuOpenHelpDocs() {
    static const unsigned char kFindExecutableErrorMap[0x20] = {0, 4, 1, 1, 4, 4, 4, 4, 4, 4, 4,
                                                                2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
                                                                4, 4, 4, 4, 4, 4, 4, 4, 4, 3};

    char associatedExecutablePath[0x100];
    HINSTANCE findResult = FindExecutableA("Docs\\Index.html", 0, associatedExecutablePath);

    char messageBoxTitle[0x80];
    strcpy(messageBoxTitle, zLoc::GetMessageString(0x19));

    const UINT resultCode = static_cast<UINT>((UINT_PTR)(findResult));
    if (resultCode <= 0x1f) {
        switch (kFindExecutableErrorMap[resultCode]) {
        case 0:
            ((CWnd *)(this))->MessageBoxA(zLoc::GetMessageString(0x20),
                                                        messageBoxTitle, 0x30);
            return;

        case 1:
            ((CWnd *)(this))->MessageBoxA(zLoc::GetMessageString(0x22),
                                                        messageBoxTitle, 0x30);
            return;

        case 2:
            ((CWnd *)(this))->MessageBoxA(zLoc::GetMessageString(0x24),
                                                        messageBoxTitle, 0x30);
            return;

        case 3:
            ((CWnd *)(this))->MessageBoxA(zLoc::GetMessageString(0x21),
                                                        messageBoxTitle, 0x30);
            return;

        default:
            break;
        }
    }

    ShellExecuteA(g_RecoilApp_hWndMain, "open", "Docs\\Index.html", 0, 0, SW_HIDE);
}

// Reimplements 0x430c30: CZRecoilFrame::OnMenuAbout (D:\Proj\Battlesport\CZRecoilFrame.cpp)
RECOIL_FRAME_NOINLINE RECOIL_NO_GS void RECOIL_THISCALL CZRecoilFrame::OnMenuAbout() {
    CAboutDlg aboutDlg;
    aboutDlg.CDialog::DoModal();
}

// Reimplements 0x431270: CZRecoilFrame::OnMenuStartMultiplayer
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuStartMultiplayer() {
    g_RecoilApp.LoadZbdAndSetupSensorTracker(1, 0, 1, m_useArchiveBanks);
}

// Reimplements 0x431290: CZRecoilFrame::OnMenuStartCampaignMode
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuStartCampaignMode() {
    g_RecoilApp.LoadZbdAndSetupSensorTracker(2, 0, 1, m_useArchiveBanks);
}

// Reimplements 0x4312b0: CZRecoilFrame::OnMenuStartCampaignMode2
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuStartCampaignMode2() {
    g_RecoilApp.LoadZbdAndSetupSensorTracker(3, 0, 1, m_useArchiveBanks);
}

// Reimplements 0x4312d0: CZRecoilFrame::OnMenuStartCampaignMode3
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuStartCampaignMode3() {
    g_RecoilApp.LoadZbdAndSetupSensorTracker(4, 0, 1, m_useArchiveBanks);
}

// Reimplements 0x4312f0: CZRecoilFrame::OnMenuStartCampaignMode4
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuStartCampaignMode4() {
    g_RecoilApp.LoadZbdAndSetupSensorTracker(5, 0, 1, m_useArchiveBanks);
}

// Reimplements 0x431310: CZRecoilFrame::OnMenuStartCampaignMode5
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuStartCampaignMode5() {
    g_RecoilApp.LoadZbdAndSetupSensorTracker(6, 0, 1, m_useArchiveBanks);
}

// Reimplements 0x431330: CZRecoilFrame::OnMenuToggleArchiveBanks
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuToggleArchiveBanks() {
    m_useArchiveBanks = m_useArchiveBanks == 0 ? 1 : 0;
    CheckMenuItem(m_mainMenuHandle, 0x9c6b, m_useArchiveBanks == 0 ? MF_UNCHECKED : MF_CHECKED);
    g_HudSensorTracker.missionFlags = m_useArchiveBanks;
    zSnd::SetUseArchiveBanksFlag(m_useArchiveBanks);
}

// Reimplements 0x431380: CZRecoilFrame::OnMenuToggleTexturePacks
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuToggleTexturePacks() {
    if (zVid::GetTexturePackLoadState() != 0) {
        zVid::SetTexturePackLoadState(0);
        CheckMenuItem(m_mainMenuHandle, 0x9c7b, MF_UNCHECKED);
        return;
    }

    zVid::SetTexturePackLoadState(1);
    CheckMenuItem(m_mainMenuHandle, 0x9c7b, MF_CHECKED);
}

// Reimplements 0x4313d0: CZRecoilFrame::OnUpdateVideoMode2CmdUI
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL
CZRecoilFrame::OnUpdateVideoMode2CmdUI(CZRecoilCmdUI *cmdUi) {
    UpdateCmdUiFromState(cmdUi, m_videoModeCmdUiState[0]);
}

// Reimplements 0x431430: CZRecoilFrame::OnUpdateVideoMode3CmdUI
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL
CZRecoilFrame::OnUpdateVideoMode3CmdUI(CZRecoilCmdUI *cmdUi) {
    UpdateCmdUiFromState(cmdUi, m_videoModeCmdUiState[1]);
}

// Reimplements 0x431490: CZRecoilFrame::OnUpdateVideoMode4CmdUI
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL
CZRecoilFrame::OnUpdateVideoMode4CmdUI(CZRecoilCmdUI *cmdUi) {
    UpdateCmdUiFromState(cmdUi, m_videoModeCmdUiState[2]);
}

// Reimplements 0x4314f0: CZRecoilFrame::OnUpdateVideoMode5CmdUI
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL
CZRecoilFrame::OnUpdateVideoMode5CmdUI(CZRecoilCmdUI *cmdUi) {
    UpdateCmdUiFromState(cmdUi, m_videoModeCmdUiState[3]);
}

// Reimplements 0x431550: CZRecoilFrame::OnUpdateVideoMode6CmdUI
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL
CZRecoilFrame::OnUpdateVideoMode6CmdUI(CZRecoilCmdUI *cmdUi) {
    UpdateCmdUiFromState(cmdUi, m_videoModeCmdUiState[4]);
}

// Reimplements 0x4315b0: CZRecoilFrame::OnUpdateVideoMode7CmdUI
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL
CZRecoilFrame::OnUpdateVideoMode7CmdUI(CZRecoilCmdUI *cmdUi) {
    UpdateCmdUiFromState(cmdUi, m_videoModeCmdUiState[5]);
}

// Reimplements 0x431790: CZRecoilFrame::OnMenuSelectHwApi0
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuSelectHwApi0() {
    EnsureHwApiInitialized(0);
}

// Reimplements 0x4317a0: CZRecoilFrame::OnMenuSelectHwApi1
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuSelectHwApi1() {
    EnsureHwApiInitialized(1);
}

// Reimplements 0x4317b0: CZRecoilFrame::OnMenuSelectHwApi2
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuSelectHwApi2() {
    EnsureHwApiInitialized(2);
}

// Reimplements 0x4317c0: CZRecoilFrame::OnMenuSelectHwApi3
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuSelectHwApi3() {
    EnsureHwApiInitialized(3);
}

// Reimplements 0x4317d0: CZRecoilFrame::UpdateHwApiMenuItem
RECOIL_FRAME_NOINLINE RECOIL_NO_GS void RECOIL_THISCALL
CZRecoilFrame::UpdateHwApiMenuItem(CZRecoilCmdUI *cmdUi, int apiIndex) {
    if (m_acceptedD3DDeviceCount < apiIndex) {
        RemoveMenu(cmdUi->m_menu_0c->m_hMenu, m_hwApiMenuCommandIds[apiIndex], MF_BYCOMMAND);
        return;
    }

    cmdUi->vftable->SetCheck(cmdUi, m_hwApiCmdUiState[apiIndex] == kCmdUiChecked ? 1 : 0);

    char menuLabelText[0x40];
    sprintf(menuLabelText, "Accelerator - %s (%s)", zVid::GetHwApiDescription(apiIndex - 1),
                 zVid::GetHwApiDriverName(apiIndex - 1));
    cmdUi->vftable->SetText(cmdUi, menuLabelText);
}

// Reimplements 0x431870: CZRecoilFrame::OnUpdateHwApi0CmdUI
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL
CZRecoilFrame::OnUpdateHwApi0CmdUI(CZRecoilCmdUI *cmdUi) {
    cmdUi->vftable->Enable(cmdUi, 1);
    cmdUi->vftable->SetCheck(cmdUi, m_hwApiCmdUiState[0] == kCmdUiChecked ? 1 : 0);
}

// Reimplements 0x4318b0: CZRecoilFrame::OnUpdateHwApi1CmdUI
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL
CZRecoilFrame::OnUpdateHwApi1CmdUI(CZRecoilCmdUI *cmdUi) {
    UpdateHwApiMenuItem(cmdUi, 1);
}

// Reimplements 0x4318c0: CZRecoilFrame::OnUpdateHwApi2CmdUI
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL
CZRecoilFrame::OnUpdateHwApi2CmdUI(CZRecoilCmdUI *cmdUi) {
    UpdateHwApiMenuItem(cmdUi, 2);
}

// Reimplements 0x4318d0: CZRecoilFrame::OnUpdateHwApi3CmdUI
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL
CZRecoilFrame::OnUpdateHwApi3CmdUI(CZRecoilCmdUI *cmdUi) {
    UpdateHwApiMenuItem(cmdUi, 3);
}

// Reimplements 0x4318e0: CZRecoilFrame::OnUpdateFullscreenCmdUI
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL
CZRecoilFrame::OnUpdateFullscreenCmdUI(CZRecoilCmdUI *cmdUi) {
    RemoveMenu(cmdUi->m_menu_0c->m_hMenu, kFullscreenMenuCommandId, MF_BYCOMMAND);
}

// Reimplements 0x431900: CZRecoilFrame::OnMenuToggleCDAudio
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuToggleCDAudio() {
    zSnd::SetCDAudioOption(zSnd::GetCDAudioOption() == 0 ? 1 : 0);
}

// Reimplements 0x431920: CZRecoilFrame::OnUpdateCDAudioCmdUI
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL
CZRecoilFrame::OnUpdateCDAudioCmdUI(CZRecoilCmdUI *cmdUi) {
    cmdUi->vftable->Enable(cmdUi, 1);
    cmdUi->vftable->SetCheck(cmdUi, zSnd::GetCDAudioOption() != 0 ? 1 : 0);
}

// Reimplements 0x431950: CZRecoilFrame::OnMenuToggleJoystick
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuToggleJoystick() {
    zInp::SetJoystickOption(zInp::GetJoystickOption() == 0 ? 1 : 0);
}

// Reimplements 0x431970: CZRecoilFrame::OnUpdateJoystickCmdUI
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL
CZRecoilFrame::OnUpdateJoystickCmdUI(CZRecoilCmdUI *cmdUi) {
    cmdUi->vftable->Enable(cmdUi, 1);
    cmdUi->vftable->SetCheck(cmdUi, zInp::GetJoystickOption() != 0 ? 1 : 0);
}

// Reimplements 0x431a90: CZRecoilFrame::OnMenuSelectDirectSound
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuSelectDirectSound() {
    zSnd::SetAudioApiOption(0);
}

// Reimplements 0x431aa0: CZRecoilFrame::OnUpdateDirectSoundCmdUI
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL
CZRecoilFrame::OnUpdateDirectSoundCmdUI(CZRecoilCmdUI *cmdUi) {
    cmdUi->vftable->Enable(cmdUi, 1);
    cmdUi->vftable->SetCheck(cmdUi, zSnd::GetAudioApiOption() == 0 ? 1 : 0);
}

// Reimplements 0x431ad0: CZRecoilFrame::OnMenuSelectA3D
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnMenuSelectA3D() {
    zSnd::SetAudioApiOption(1);
}

// Reimplements 0x431ae0: CZRecoilFrame::OnUpdateA3DCmdUI
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnUpdateA3DCmdUI(CZRecoilCmdUI *cmdUi) {
    cmdUi->vftable->Enable(cmdUi, 1);
    cmdUi->vftable->SetCheck(cmdUi, zSnd::GetActiveBackend() == 1 ? 1 : 0);
}

// Reimplements 0x431b10: CZRecoilFrame::OnSize
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::OnSize(unsigned int nType,
                                                                 int cx, int cy) {
    ((CZGameFrame *)(this))->OnSize(nType, cx, cy);

    if (nType == 4 || nType == 1) {
        ((CZGameFrame *)(this))->m_app->vftable->OnAppDeactivate(
            ((CZGameFrame *)(this))->m_app);
    }
}

// Reimplements 0x431610: CZRecoilFrame::SetHwApiAndInitMode
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL
CZRecoilFrame::SetHwApiAndInitMode(int hwApiIndex) {
    zVid::SetHwApiOption(zVideo::SelectHwApiDeviceOrFallback(hwApiIndex));
    g_zVideo_pfnQueryDeviceVideoMemoryBytes(hwApiIndex, &m_vidMemTotalBytes,
                                            (int *)(&m_vidMemFreeBytes));
    m_fullscreenOption = zOpt::GetFullscreenOption();
    zOpt::SetFullscreenOption(1);
    zVid::SetAccelerationOption(1);
    m_videoModeIndex = zVid::GetVideoModeIndexFromOptions();
    OnMenuSetVideoMode5();
}

// Reimplements 0x431680: CZRecoilFrame::InitFallbackMode
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::InitFallbackMode() {
    zVid::SetHwApiOption(zVideo::SelectHwApiDeviceOrFallback(-1));
    zVid::SetAccelerationOption(0);
    zOpt::SetFullscreenOption(m_fullscreenOption);
    zVid::SetVideoModeIndex(m_videoModeIndex);
    ConfigureModeFeatureFlags();
}

// Reimplements 0x4316c0: CZRecoilFrame::EnsureHwApiInitialized
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL
CZRecoilFrame::EnsureHwApiInitialized(int hwApiSelector) {
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
RECOIL_FRAME_NOINLINE void RECOIL_THISCALL CZRecoilFrame::InitStartupHwApiFromOptions() {
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
