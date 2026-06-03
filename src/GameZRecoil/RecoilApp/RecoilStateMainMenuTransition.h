#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "Battlesport/RecoilApp.h"
#include "GameZRecoil/RecoilApp/RecoilStateBase.h"
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

struct RecoilStateMainMenuTransition {
    RecoilPtr32 vftable;                   // RecoilStateMainMenuTransition_Vtbl*
    volatile RecoilPtr32 m_mainMenuDialog; // HudUiMainMenuDialog*
    int m_savedHalfResAdjustMode;
    RecoilMainMenuEntryRoute m_entryRoute;
    zVidModeIndex m_deferredVideoModeIndex;
    RecoilPtr32 m_pausedAudioSnapshot; // zSndPlayHandleSnapshot*

    RecoilStateMainMenuTransition *RECOIL_THISCALL Constructor();
    RECOIL_NOINLINE RecoilStateMainMenuTransition *RECOIL_THISCALL ScalarDeletingDestructor(
        unsigned int flags
    );
    RECOIL_NOINLINE RECOIL_NO_GS ~RecoilStateMainMenuTransition();
    RECOIL_NO_GS int RECOIL_THISCALL OnTryBecomeCurrent();
    void RECOIL_THISCALL OnResume(int param);
    void RECOIL_THISCALL OnDeactivate();

    static void RECOIL_CDECL StaticInitAndRegisterAtExit();
    static RecoilStateMainMenuTransition *RECOIL_CDECL StaticInit();
    static void RECOIL_CDECL RegisterAtExit();
    static void RECOIL_CDECL AtExitDestructor();
    static void RECOIL_CDECL ClearPausedAudioSnapshot();
    static void RECOIL_FASTCALL QueueEnter(RecoilMainMenuEntryRoute entryRoute);
    static void RECOIL_FASTCALL SetDeferredVideoModeIndex(zVidModeIndex modeIndex);
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
extern RecoilApp_IState_Vtbl g_RecoilStateMainMenuTransition_Vtbl;

class HudUiMainMenuDialog;

struct HudUiMainMenuDialogBackground : HudUiBackground {
    HudUiMainMenuDialogBackground();
    ~HudUiMainMenuDialogBackground();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiMainMenuDialogBackground) == sizeof(HudUiBackground));

struct HudUiMainMenuDialog_CreditsButton : HudUiZrdWidget {
    HudUiMainMenuDialog_CreditsButton();
    ~HudUiMainMenuDialog_CreditsButton();
    void RECOIL_THISCALL OnActivate();
};

struct HudUiMenuBackButton : HudUiZrdWidget {
    HudUiMenuBackButton();
    ~HudUiMenuBackButton();
    void RECOIL_THISCALL OnActivate();
};

struct HudUiMainMenuDialog_SaveButton : HudUiZrdWidget {
    HudUiMainMenuDialog_SaveButton();
    ~HudUiMainMenuDialog_SaveButton();
    void RECOIL_THISCALL OnActivate();
};

struct HudUiMainMenuDialog_LoadButton : HudUiZrdWidget {
    HudUiMainMenuDialog_LoadButton();
    ~HudUiMainMenuDialog_LoadButton();
    void RECOIL_THISCALL OnActivate();
};

struct HudUiMainMenuDialog_NewGameButton : HudUiZrdWidget {
    HudUiMainMenuDialog_NewGameButton();
    ~HudUiMainMenuDialog_NewGameButton();
    void RECOIL_THISCALL OnActivate();
};

struct HudUiMainMenuDialog_OptionsButton : HudUiZrdWidget {
    HudUiMainMenuDialog_OptionsButton();
    ~HudUiMainMenuDialog_OptionsButton();
    void RECOIL_THISCALL OnActivate();
};

struct HudUiMainMenuDialog_QuitButton : HudUiZrdWidget {
    HudUiMainMenuDialog_QuitButton();
    ~HudUiMainMenuDialog_QuitButton();
    void RECOIL_THISCALL OnActivate();
};

struct HudUiMainMenuDialog_ControlsButton : HudUiZrdWidget {
    HudUiMainMenuDialog_ControlsButton();
    ~HudUiMainMenuDialog_ControlsButton();
    void RECOIL_THISCALL OnActivate();
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

    RECOIL_NOINLINE static int RECOIL_CDECL CanLoadGame();
    RECOIL_NOINLINE static int RECOIL_CDECL CanSaveGame();
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
