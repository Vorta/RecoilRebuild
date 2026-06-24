#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"

#include <new>
#include <stdlib.h>

/**
 * Reimplements data 0x4edc58: g_RecoilState_MainMenuTransition.
 *
 * Data owner: legacy.app_shell.class_recoilstatemainmenutransition. BN exposes
 * a 0x18-byte zero-initialized .data object of type
 * RecoilStateMainMenuTransition at 0x4edc58. StaticInit constructs it in place;
 * AtExitDestructor destroys that same object. BN base-object xrefs are
 * StaticInit, AtExitDestructor, and QueueEnter.
 *
 * Purpose: own the global app-state singleton used while transitioning into
 * the main menu.
 */
RecoilStateMainMenuTransition g_RecoilState_MainMenuTransition;

/**
 * Reimplements 0x415170: RecoilStateMainMenuTransition::RecoilStateMainMenuTransition.
 *
 * Purpose: initialize the static main-menu transition app state and clear its
 * dialog/audio ownership fields.
 */
RecoilStateMainMenuTransition::RecoilStateMainMenuTransition()
    : m_mainMenuDialog(0),
      m_savedHalfResAdjustMode(0),
      m_entryRoute(RECOIL_MAINMENU_ROUTE_FRONTEND),
      m_deferredVideoModeIndex(ZVID_MODE_INVALID_COMPLEMENT),
      m_pausedAudioSnapshot(0) {}

/**
 * Reimplements 0x415100: RecoilStateMainMenuTransition::StaticInitAndRegisterAtExit.
 *
 * Purpose: construct the static transition state and register its at-exit
 * destructor callback.
 */
void RecoilStateMainMenuTransition::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

/**
 * Reimplements 0x415110: RecoilStateMainMenuTransition::StaticInit.
 *
 * Purpose: construct the global main-menu transition state in place and return
 * it to the static-initialization wrapper.
 */
RecoilStateMainMenuTransition *RecoilStateMainMenuTransition::StaticInit() {
    return new (&g_RecoilState_MainMenuTransition) RecoilStateMainMenuTransition;
}

/**
 * Reimplements 0x415120: RecoilStateMainMenuTransition::RegisterAtExit.
 *
 * Purpose: register the static transition state's destruction callback with
 * the CRT at-exit list.
 */
void RecoilStateMainMenuTransition::RegisterAtExit() {
    atexit(RecoilStateMainMenuTransition::AtExitDestructor);
}

/**
 * Reimplements 0x415130: RecoilStateMainMenuTransition::AtExitDestructor.
 *
 * Purpose: destroy the global main-menu transition state from the registered
 * at-exit callback.
 */
void RecoilStateMainMenuTransition::AtExitDestructor() {
    g_RecoilState_MainMenuTransition.~RecoilStateMainMenuTransition();
}

/**
 * Reimplements 0x4151b0: RecoilStateMainMenuTransition::~RecoilStateMainMenuTransition.
 *
 * Purpose: disable and destroy the owned main-menu dialog during transition
 * state teardown.
 */
RECOIL_NO_GS RecoilStateMainMenuTransition::~RecoilStateMainMenuTransition() {
    HudUiMainMenuDialog *dialog = m_mainMenuDialog;
    if (dialog != 0) {
        dialog->SetEnabled(0);

        dialog = m_mainMenuDialog;
        if (dialog != 0) {
            delete dialog;
        }

        m_mainMenuDialog = 0;
    }
}
