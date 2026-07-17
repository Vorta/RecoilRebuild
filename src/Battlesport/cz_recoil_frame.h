#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "Battlesport/cz_game_frame.h"
#include "recoil/recoil_callconv.h"

namespace MfcCmdUI {
void __stdcall EnableAlways(CCmdUI *cmdUi);
}

class RecoilApp;

/**
 * Authored Recoil frame reconstructed over imported MFC42 frame/window
 * providers; MFC base behavior is not reimplemented here.
 */
struct CZRecoilFrame : CZGameFrame {
    char m_openZbdFilePath[0x104];
    CMenu m_mainMenu;
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

    CZRecoilFrame();
    ~CZRecoilFrame();
    static CRuntimeClass *__stdcall GetBaseRuntimeClass();
    static CZRecoilFrame *CreateObject();
    static CRuntimeClass *__stdcall GetRuntimeClassStatic();
    virtual CRuntimeClass *GetRuntimeClass() const;
    static const AFX_MSGMAP *__stdcall GetBaseMessageMap();
    static const AFX_MSGMAP *__stdcall GetMessageMapStatic();
    virtual const AFX_MSGMAP * GetMessageMap() const;
    void SetMenuBarVisibility(int visible);
    CString * BuildWindowTitle(CString *outTitle);
    void OnMenuStartSinglePlayer();
    void OnMenuOpenCampaign();
    RECOIL_NO_GS void OnOpenFileDialog();
    void ConfigureModeFeatureFlags();
    void OnMenuSetVideoMode2();
    void OnMenuSetVideoMode3();
    void OnMenuSetVideoMode4();
    void OnMenuSetVideoMode5();
    void OnMenuSetVideoMode6();
    void OnMenuSetVideoMode7();
    void OnMenuExitGame();
    void OnMenuToggleHud();
    void OnUpdateHudCmdUI(CCmdUI *cmdUi);
    void OnMenuToggleFullscreen();
    RECOIL_NO_GS void OnMenuOpenHelpDocs();
    RECOIL_NO_GS void OnMenuAbout();
    void OnMenuOpenMultiplayerSessionBrowser();
    void OnMenuStartMultiplayer();
    void OnMenuStartCampaignMode();
    void OnMenuStartCampaignMode2();
    void OnMenuStartCampaignMode3();
    void OnMenuStartCampaignMode4();
    void OnMenuStartCampaignMode5();
    RECOIL_NO_GS void OnMenuWestwoodOnlineUpgrade();
    void OnMenuToggleArchiveBanks();
    void OnMenuToggleTexturePacks();
    void OnUpdateVideoMode2CmdUI(CCmdUI *cmdUi);
    void OnUpdateVideoMode3CmdUI(CCmdUI *cmdUi);
    void OnUpdateVideoMode4CmdUI(CCmdUI *cmdUi);
    void OnUpdateVideoMode5CmdUI(CCmdUI *cmdUi);
    void OnUpdateVideoMode6CmdUI(CCmdUI *cmdUi);
    void OnUpdateVideoMode7CmdUI(CCmdUI *cmdUi);
    void OnMenuSelectHwApi0();
    void OnMenuSelectHwApi1();
    void OnMenuSelectHwApi2();
    void OnMenuSelectHwApi3();
    RECOIL_NO_GS void UpdateHwApiMenuItem(
        CCmdUI *cmdUi,
        int apiIndex
    );
    void OnUpdateHwApi0CmdUI(CCmdUI *cmdUi);
    void OnUpdateHwApi1CmdUI(CCmdUI *cmdUi);
    void OnUpdateHwApi2CmdUI(CCmdUI *cmdUi);
    void OnUpdateHwApi3CmdUI(CCmdUI *cmdUi);
    void OnUpdateFullscreenCmdUI(CCmdUI *cmdUi);
    void OnUpdateAlwaysEnabledCmdUI(CCmdUI *cmdUi);
    /**
     * Reimplements 0x401020: CZRecoilFrame::OnUpdateNoOpCmdUI.
     * The address is a heterogeneous shared physical alias and does not imply
     * sole ownership of the group.
     * Purpose: leave the two mapped command-update states unchanged.
     */
    afx_msg void OnUpdateNoOpCmdUI(CCmdUI *) {}
    void OnMenuToggleCDAudio();
    void OnUpdateCDAudioCmdUI(CCmdUI *cmdUi);
    void OnMenuToggleJoystick();
    void OnUpdateJoystickCmdUI(CCmdUI *cmdUi);
    void OnMenuSelectDirectSound();
    void OnUpdateDirectSoundCmdUI(CCmdUI *cmdUi);
    void OnMenuSelectA3D();
    void OnUpdateA3DCmdUI(CCmdUI *cmdUi);
    void OnSize(
        unsigned int nType,
        int cx,
        int cy
    );
    void SetHwApiAndInitMode(int hwApiIndex);
    void InitFallbackMode();
    void EnsureHwApiInitialized(int hwApiSelector);
    void InitStartupHwApiFromOptions();
};
RECOIL_STATIC_ASSERT(
    offsetof(
        CZRecoilFrame,
        m_app
    ) == 0x0c0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZRecoilFrame,
        m_openZbdFilePath
    ) == 0x0cc
);

#if defined(_M_IX86) || defined(__i386__)
RECOIL_STATIC_ASSERT(
    offsetof(
        CCmdUI,
        m_pMenu
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(sizeof(CZRecoilFrame) == 0x230);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZRecoilFrame,
        m_hWnd
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZRecoilFrame,
        m_openZbdFilePath
    ) == 0xcc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZRecoilFrame,
        m_mainMenu
    ) == 0x1d0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CMenu,
        m_hMenu
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZRecoilFrame,
        m_useArchiveBanks
    ) == 0x1d8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZRecoilFrame,
        m_cmdlineFlag
    ) == 0x1dc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZRecoilFrame,
        m_videoModeCmdUiState
    ) == 0x1e0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZRecoilFrame,
        m_acceptedD3DDeviceCount
    ) == 0x1f8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZRecoilFrame,
        m_hwApiCmdUiState
    ) == 0x1fc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZRecoilFrame,
        m_hwApiMenuCommandIds
    ) == 0x20c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZRecoilFrame,
        m_videoModeIndex
    ) == 0x21c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZRecoilFrame,
        m_fullscreenOption
    ) == 0x220
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZRecoilFrame,
        m_vidMemTotalBytes
    ) == 0x224
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZRecoilFrame,
        m_vidMemFreeBytes
    ) == 0x228
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZRecoilFrame,
        m_campaignsOnlyMode
    ) == 0x22c
);
#endif
