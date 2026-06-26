#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "Battlesport/RecoilApp.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zSound/zSound.h"
#include "recoil/recoil_callconv.h"

enum RecoilMainMenuEntryRoute {
    RECOIL_MAINMENU_ROUTE_FRONTEND = 0,
    RECOIL_MAINMENU_ROUTE_INGAME = 1,
};

enum zVidModeIndex {
    ZVID_MODE_INVALID_COMPLEMENT = -1,
    ZVID_MODE_320X200_TO_640X400 = 2,
    ZVID_MODE_320X240_TO_640X480 = 3,
    ZVID_MODE_640X400 = 4,
    ZVID_MODE_640X480 = 5,
    ZVID_MODE_800X600 = 6,
    ZVID_MODE_1024X768 = 7,
};

class HudUiMainMenuDialog;

struct RecoilStateMainMenuTransition : RecoilApp_IState {
    HudUiMainMenuDialog *m_mainMenuDialog;
    int m_savedHalfResAdjustMode;
    RecoilMainMenuEntryRoute m_entryRoute;
    zVidModeIndex m_deferredVideoModeIndex;
    RecoilPtr32 m_pausedAudioSnapshot; // zSndPlayHandleSnapshot*

    RecoilStateMainMenuTransition();
    RECOIL_NO_GS ~RecoilStateMainMenuTransition();
    RECOIL_NO_GS int OnTryBecomeCurrent();
    int OnUpdateShouldQuit();
    void OnResume(int param);
    void OnDeactivate();

    static void StaticInitAndRegisterAtExit();
    static RecoilStateMainMenuTransition *StaticInit();
    static void RegisterAtExit();
    static void AtExitDestructor();
    static void ClearPausedAudioSnapshot();
    static void __fastcall QueueEnter(RecoilMainMenuEntryRoute entryRoute);
    static void __fastcall SetDeferredVideoModeIndex(zVidModeIndex modeIndex);
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateMainMenuTransition) == 0x18);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilStateMainMenuTransition,
        m_mainMenuDialog
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilStateMainMenuTransition,
        m_savedHalfResAdjustMode
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilStateMainMenuTransition,
        m_entryRoute
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilStateMainMenuTransition,
        m_deferredVideoModeIndex
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilStateMainMenuTransition,
        m_pausedAudioSnapshot
    ) == 0x14
);

extern RecoilStateMainMenuTransition g_RecoilState_MainMenuTransition;

struct HudUiMainMenuDialogBackground : HudUiBackground {
    HudUiMainMenuDialogBackground();
    ~HudUiMainMenuDialogBackground();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiMainMenuDialogBackground) == sizeof(HudUiBackground));

struct HudUiMainMenuDialog_CreditsButton : HudUiZrdWidget {
    HudUiMainMenuDialog_CreditsButton();
    ~HudUiMainMenuDialog_CreditsButton();
    void OnActivate();
};

struct HudUiMenuBackButton : HudUiZrdWidget {
    /**
     * Restores the original-source inline menu back-button constructor. No
     * standalone retail function exists; observed in callers 0x414c12,
     * 0x41c2ca, 0x434680, and 0x434b90.
     * Purpose: keep shared back-button dispatch-table installation owned by
     * the typed button member.
     */
    HudUiMenuBackButton() : HudUiZrdWidget() {
    }

    /**
     * Restores the original-source inline menu back-button destructor. No
     * standalone retail function exists; observed through owner destructor
     * paths that tear down the shared HudUiZrdWidget base.
     * Purpose: keep shared back-button cleanup on the typed button member.
     */
    ~HudUiMenuBackButton() {
        HudUiZrdWidget::DestructorCore();
    }

    void OnActivate();
};

struct HudUiMainMenuDialog_SaveButton : HudUiZrdWidget {
    HudUiMainMenuDialog_SaveButton();
    ~HudUiMainMenuDialog_SaveButton();
    void OnActivate();
};

struct HudUiMainMenuDialog_LoadButton : HudUiZrdWidget {
    HudUiMainMenuDialog_LoadButton();
    ~HudUiMainMenuDialog_LoadButton();
    void OnActivate();
};

struct HudUiMainMenuDialog_NewGameButton : HudUiZrdWidget {
    HudUiMainMenuDialog_NewGameButton();
    ~HudUiMainMenuDialog_NewGameButton();
    void OnActivate();
};

struct HudUiMainMenuDialog_OptionsButton : HudUiZrdWidget {
    HudUiMainMenuDialog_OptionsButton();
    ~HudUiMainMenuDialog_OptionsButton();
    void OnActivate();
};

struct HudUiMainMenuDialog_QuitButton : HudUiZrdWidget {
    HudUiMainMenuDialog_QuitButton();
    ~HudUiMainMenuDialog_QuitButton();
    void OnActivate();
};

struct HudUiMainMenuDialog_ControlsButton : HudUiZrdWidget {
    HudUiMainMenuDialog_ControlsButton();
    ~HudUiMainMenuDialog_ControlsButton();
    void OnActivate();
};

class HudUiMainMenuDialog : public HudUiMainMenuDialogBackground {
  public:
    HudUiMainMenuDialog_CreditsButton creditsButton;
    HudUiMenuBackButton backButton;
    HudUiMainMenuDialog_SaveButton saveGameButton;
    HudUiMainMenuDialog_LoadButton loadGameButton;
    HudUiMainMenuDialog_NewGameButton newGameButton;
    HudUiMainMenuDialog_OptionsButton optionsButton;
    HudUiMainMenuDialog_QuitButton quitButton;
    HudUiMainMenuDialog_ControlsButton controlsButton;

    HudUiMainMenuDialog(RecoilMainMenuEntryRoute route);
    ~HudUiMainMenuDialog();

    static int CanLoadGame();
    static int CanSaveGame();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiMainMenuDialog) == 0xb3ac);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMainMenuDialog,
        creditsButton
    ) == 0xa94c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMainMenuDialog,
        backButton
    ) == 0xaa98
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMainMenuDialog,
        saveGameButton
    ) == 0xabe4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMainMenuDialog,
        loadGameButton
    ) == 0xad30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMainMenuDialog,
        newGameButton
    ) == 0xae7c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMainMenuDialog,
        optionsButton
    ) == 0xafc8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMainMenuDialog,
        quitButton
    ) == 0xb114
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMainMenuDialog,
        controlsButton
    ) == 0xb260
);
