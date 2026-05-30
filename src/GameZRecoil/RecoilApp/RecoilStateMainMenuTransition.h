#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include "Battlesport/RecoilApp.h"
#include "GameZRecoil/RecoilApp/RecoilStateBase.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "recoil/recoil_callconv.h"

const RecoilPtr32 kRecoilStateMainMenuTransition_VtblAddress = 0x004cee28;

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
    RECOIL_NOINLINE RecoilStateMainMenuTransition *RECOIL_THISCALL
    ScalarDeletingDestructor(unsigned int flags);
    RECOIL_NOINLINE RECOIL_NO_GS ~RecoilStateMainMenuTransition();
    RECOIL_NO_GS int RECOIL_THISCALL OnTryBecomeCurrent();
    void RECOIL_THISCALL OnResume(int param);
    void RECOIL_THISCALL OnDeactivate();

    static void RECOIL_CDECL ClearPausedAudioSnapshot();
    static void RECOIL_FASTCALL QueueEnter(RecoilMainMenuEntryRoute entryRoute);
    static void RECOIL_FASTCALL SetDeferredVideoModeIndex(zVidModeIndex modeIndex);
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateMainMenuTransition) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(RecoilStateMainMenuTransition, m_mainMenuDialog) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(RecoilStateMainMenuTransition, m_savedHalfResAdjustMode) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(RecoilStateMainMenuTransition, m_entryRoute) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(RecoilStateMainMenuTransition, m_deferredVideoModeIndex) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(RecoilStateMainMenuTransition, m_pausedAudioSnapshot) == 0x14);

extern RecoilStateMainMenuTransition g_RecoilState_MainMenuTransition;

struct HudUiMainMenuDialog;

struct HudUiMainMenuDialogVirtual {
    virtual void RECOIL_THISCALL Update(float deltaSeconds);
    virtual void RECOIL_THISCALL SetEnabled(int enabled);
    virtual HudUiMainMenuDialog *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
};
RECOIL_STATIC_ASSERT(sizeof(HudUiMainMenuDialogVirtual) == 0x04);

struct HudUiMainMenuDialog_Vtbl {
    void(RECOIL_THISCALL *UpdateAll)(HudUiMainMenuDialog *self, float deltaSeconds);
    void(RECOIL_THISCALL *SetEnabled)(HudUiMainMenuDialog *self, int enabled);
    HudUiMainMenuDialog *(RECOIL_THISCALL *ScalarDeletingDtor)(HudUiMainMenuDialog *self,
                                                               unsigned int flags);
};
RECOIL_STATIC_ASSERT(sizeof(HudUiMainMenuDialog_Vtbl) == 0x0c);

struct HudUiMainMenuDialog {
    union {
        HudUiBackground base;
        RecoilPtr32 vftable; // HudUiMainMenuDialog_Vtbl*, alias for base.base.base.vptr
    };
    HudUiZrdWidget creditsButton;
    HudUiZrdWidget backButton;
    HudUiZrdWidget saveGameButton;
    HudUiZrdWidget loadGameButton;
    HudUiZrdWidget newGameButton;
    HudUiZrdWidget optionsButton;
    HudUiZrdWidget quitButton;
    HudUiZrdWidget controlsButton;

    HudUiMainMenuDialog *RECOIL_THISCALL Constructor(RecoilMainMenuEntryRoute route);
    void RECOIL_THISCALL Destructor();
    void RECOIL_THISCALL Update(float deltaSeconds);
    void RECOIL_THISCALL SetEnabled(int enabled);
    HudUiMainMenuDialog *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);

    RECOIL_NOINLINE static int RECOIL_CDECL CanLoadGame();
    RECOIL_NOINLINE static int RECOIL_CDECL CanSaveGame();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiMainMenuDialog) == 0xb3ac);
RECOIL_STATIC_ASSERT(offsetof(HudUiMainMenuDialog, creditsButton) == 0xa94c);
RECOIL_STATIC_ASSERT(offsetof(HudUiMainMenuDialog, backButton) == 0xaa98);
RECOIL_STATIC_ASSERT(offsetof(HudUiMainMenuDialog, saveGameButton) == 0xabe4);
RECOIL_STATIC_ASSERT(offsetof(HudUiMainMenuDialog, loadGameButton) == 0xad30);
RECOIL_STATIC_ASSERT(offsetof(HudUiMainMenuDialog, newGameButton) == 0xae7c);
RECOIL_STATIC_ASSERT(offsetof(HudUiMainMenuDialog, optionsButton) == 0xafc8);
RECOIL_STATIC_ASSERT(offsetof(HudUiMainMenuDialog, quitButton) == 0xb114);
RECOIL_STATIC_ASSERT(offsetof(HudUiMainMenuDialog, controlsButton) == 0xb260);
