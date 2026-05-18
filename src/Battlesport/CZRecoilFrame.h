#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include <windows.h>

#include "Battlesport/Mfc42Abi.h"
#include "recoil/recoil_callconv.h"

#if defined(_MSC_VER) && _MSC_VER >= 1300
#define RECOIL_FRAME_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define RECOIL_FRAME_NOINLINE __attribute__((noinline))
#else
#define RECOIL_FRAME_NOINLINE
#endif

struct CZRecoilCmdUI;

struct CZRecoilCmdUIVtable {
    void(RECOIL_THISCALL *Enable)(CZRecoilCmdUI *self, int enable);
    void(RECOIL_THISCALL *SetCheck)(CZRecoilCmdUI *self, int check);
    void *reserved08;
    void(RECOIL_THISCALL *SetText)(CZRecoilCmdUI *self, const char *text);
};

struct CZRecoilMenuProxy {
    void *vftable;
    HMENU m_hMenu;
};

struct CZRecoilCmdUI {
    CZRecoilCmdUIVtable *vftable;
    unsigned int m_nID;
    unsigned int m_nIndex;
    CZRecoilMenuProxy *m_menu_0c;
};

namespace MfcCmdUI {
RECOIL_FRAME_NOINLINE void RECOIL_STDCALL EnableAlways(CZRecoilCmdUI *cmdUi);
}

struct CZRecoilFrame {
    unsigned int vftable;
    unsigned char reserved004[0x1c];
    HWND m_hWnd;
    unsigned char reserved024[0x9c];
    unsigned int m_app;
    unsigned char reserved0c4[0x08];
    char m_openZbdFilePath[0x104];
    unsigned int m_mainMenuVftable;
    HMENU m_mainMenuHandle;
    int m_useArchiveBanks;
    int m_cmdlineFlag;
    int m_videoModeCmdUiState[6];
    int m_acceptedD3DDeviceCount;
    int m_hwApiCmdUiState[4];
    unsigned int m_hwApiMenuCommandIds[4];
    int m_videoModeIndex;
    int m_fullscreenOption;
    int m_vidMemTotalBytes;
    unsigned int m_vidMemFreeBytes;
    int m_campaignsOnlyMode;

    static CRuntimeClass classCZRecoilFrame;
    static const AFX_MSGMAP messageMap;
    static const AFX_MSGMAP_ENTRY messageEntries[];

    RECOIL_FRAME_NOINLINE static unsigned int RECOIL_CDECL GetBaseRuntimeClass();
    RECOIL_FRAME_NOINLINE static CZRecoilFrame *RECOIL_CDECL CreateObject();
    RECOIL_FRAME_NOINLINE static unsigned int RECOIL_CDECL GetRuntimeClass();
    RECOIL_FRAME_NOINLINE static unsigned int RECOIL_CDECL GetBaseMessageMap();
    RECOIL_FRAME_NOINLINE static unsigned int RECOIL_CDECL GetMessageMap();
    RECOIL_FRAME_NOINLINE CZRecoilFrame *RECOIL_THISCALL Constructor();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL Destructor();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL SetMenuBarVisibility(int visible);
    RECOIL_FRAME_NOINLINE CString *RECOIL_THISCALL BuildWindowTitle(CString *outTitle);
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuStartSinglePlayer();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuOpenCampaign();
    RECOIL_FRAME_NOINLINE RECOIL_NO_GS void RECOIL_THISCALL OnOpenFileDialog();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL ConfigureModeFeatureFlags();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuSetVideoMode2();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuSetVideoMode3();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuSetVideoMode4();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuSetVideoMode5();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuSetVideoMode6();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuSetVideoMode7();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuExitGame();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuToggleHud();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnUpdateHudCmdUI(CZRecoilCmdUI *cmdUi);
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuToggleFullscreen();
    RECOIL_FRAME_NOINLINE RECOIL_NO_GS void RECOIL_THISCALL OnMenuOpenHelpDocs();
    RECOIL_FRAME_NOINLINE RECOIL_NO_GS void RECOIL_THISCALL OnMenuAbout();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuStartMultiplayer();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuStartCampaignMode();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuStartCampaignMode2();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuStartCampaignMode3();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuStartCampaignMode4();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuStartCampaignMode5();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuToggleArchiveBanks();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuToggleTexturePacks();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnUpdateVideoMode2CmdUI(CZRecoilCmdUI *cmdUi);
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnUpdateVideoMode3CmdUI(CZRecoilCmdUI *cmdUi);
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnUpdateVideoMode4CmdUI(CZRecoilCmdUI *cmdUi);
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnUpdateVideoMode5CmdUI(CZRecoilCmdUI *cmdUi);
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnUpdateVideoMode6CmdUI(CZRecoilCmdUI *cmdUi);
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnUpdateVideoMode7CmdUI(CZRecoilCmdUI *cmdUi);
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuSelectHwApi0();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuSelectHwApi1();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuSelectHwApi2();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuSelectHwApi3();
    RECOIL_FRAME_NOINLINE RECOIL_NO_GS void RECOIL_THISCALL
    UpdateHwApiMenuItem(CZRecoilCmdUI *cmdUi, int apiIndex);
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnUpdateHwApi0CmdUI(CZRecoilCmdUI *cmdUi);
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnUpdateHwApi1CmdUI(CZRecoilCmdUI *cmdUi);
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnUpdateHwApi2CmdUI(CZRecoilCmdUI *cmdUi);
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnUpdateHwApi3CmdUI(CZRecoilCmdUI *cmdUi);
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnUpdateFullscreenCmdUI(CZRecoilCmdUI *cmdUi);
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuToggleCDAudio();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnUpdateCDAudioCmdUI(CZRecoilCmdUI *cmdUi);
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuToggleJoystick();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnUpdateJoystickCmdUI(CZRecoilCmdUI *cmdUi);
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuSelectDirectSound();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnUpdateDirectSoundCmdUI(CZRecoilCmdUI *cmdUi);
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnMenuSelectA3D();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnUpdateA3DCmdUI(CZRecoilCmdUI *cmdUi);
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL OnSize(unsigned int nType, int cx,
                                                      int cy);
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL SetHwApiAndInitMode(int hwApiIndex);
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL InitFallbackMode();
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL EnsureHwApiInitialized(int hwApiSelector);
    RECOIL_FRAME_NOINLINE void RECOIL_THISCALL InitStartupHwApiFromOptions();
};
RECOIL_STATIC_ASSERT(offsetof(CZRecoilFrame, m_app) == 0x0c0);
RECOIL_STATIC_ASSERT(offsetof(CZRecoilFrame, m_openZbdFilePath) == 0x0cc);

#if defined(_M_IX86) || defined(__i386__)
RECOIL_STATIC_ASSERT(offsetof(CZRecoilCmdUIVtable, SetText) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(CZRecoilMenuProxy, m_hMenu) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(CZRecoilCmdUI, m_menu_0c) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(CZRecoilFrame) == 0x230);
RECOIL_STATIC_ASSERT(offsetof(CZRecoilFrame, m_hWnd) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(CZRecoilFrame, m_openZbdFilePath) == 0xcc);
RECOIL_STATIC_ASSERT(offsetof(CZRecoilFrame, m_mainMenuVftable) == 0x1d0);
RECOIL_STATIC_ASSERT(offsetof(CZRecoilFrame, m_mainMenuHandle) == 0x1d4);
RECOIL_STATIC_ASSERT(offsetof(CZRecoilFrame, m_useArchiveBanks) == 0x1d8);
RECOIL_STATIC_ASSERT(offsetof(CZRecoilFrame, m_cmdlineFlag) == 0x1dc);
RECOIL_STATIC_ASSERT(offsetof(CZRecoilFrame, m_videoModeCmdUiState) == 0x1e0);
RECOIL_STATIC_ASSERT(offsetof(CZRecoilFrame, m_acceptedD3DDeviceCount) == 0x1f8);
RECOIL_STATIC_ASSERT(offsetof(CZRecoilFrame, m_hwApiCmdUiState) == 0x1fc);
RECOIL_STATIC_ASSERT(offsetof(CZRecoilFrame, m_hwApiMenuCommandIds) == 0x20c);
RECOIL_STATIC_ASSERT(offsetof(CZRecoilFrame, m_videoModeIndex) == 0x21c);
RECOIL_STATIC_ASSERT(offsetof(CZRecoilFrame, m_fullscreenOption) == 0x220);
RECOIL_STATIC_ASSERT(offsetof(CZRecoilFrame, m_vidMemTotalBytes) == 0x224);
RECOIL_STATIC_ASSERT(offsetof(CZRecoilFrame, m_vidMemFreeBytes) == 0x228);
RECOIL_STATIC_ASSERT(offsetof(CZRecoilFrame, m_campaignsOnlyMode) == 0x22c);
#endif
