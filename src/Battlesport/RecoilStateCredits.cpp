#include "Battlesport/RecoilStateCredits.h"
#include "GameZRecoil/zHud/zhud_ui.h"

#include <new>
#include <stdlib.h>

/**
 * Reimplements data 0x4e5de0: g_RecoilStateCredits.
 *
 * Purpose: store the static credits app-state object.
 *
 * Source owner: RecoilStateCredits is the credits-state slice of the
 * RecoilStateBase app-state cluster. BN shows the retail static-lifetime chain
 * as a compiler global-constructor thunk plus CRT atexit callback around this
 * source-file global.
 */
RecoilStateCredits g_RecoilStateCredits;

/**
 * Reimplements 0x409990: RecoilStateCredits::RecoilStateCredits.
 *
 * Purpose: initialize the credits app-state object and clear the active
 * credits-panel pointer.
 */
RecoilStateCredits::RecoilStateCredits() : m_dialog(0) {}

/**
 * Reimplements 0x409950: RecoilStateCredits::StaticInitAndRegisterAtExit.
 *
 * Purpose: construct the static credits state and register its at-exit
 * destructor callback.
 *
 * BN shape: retail calls compiler-generated StaticConstructGlobal at 0x409960
 * before tail-calling StaticInit. The authored source keeps that thunk as
 * compiler glue and expresses the source-level static object construction.
 */
void RecoilStateCredits::StaticInitAndRegisterAtExit() {
    new (&g_RecoilStateCredits) RecoilStateCredits;
    StaticInit();
}

/**
 * Reimplements 0x409970: RecoilStateCredits::StaticInit.
 *
 * Purpose: register the static credits state's destruction callback with the
 * CRT at-exit list.
 *
 * BN shape: retail pushes RegisterAtExit and calls the CRT atexit provider.
 */
void RecoilStateCredits::StaticInit() {
    atexit(RegisterAtExit);
}

/**
 * Reimplements 0x409980: RecoilStateCredits::RegisterAtExit.
 *
 * Purpose: destroy the global credits state from the registered at-exit
 * callback.
 *
 * BN shape: retail sets this to g_RecoilStateCredits and tail-calls the
 * virtual destructor body.
 */
void RecoilStateCredits::RegisterAtExit() {
    g_RecoilStateCredits.~RecoilStateCredits();
}

/**
 * Reimplements 0x4099a0: RecoilStateCredits::OnWndActivate.
 *
 * Purpose: refresh the credits dialog surfaces when the application is
 * reactivated.
 */
void RecoilStateCredits::OnWndActivate(
    int activateCode
) {
    if (activateCode == 0) {
        return;
    }

    HudUiCreditsPanel *const creditsPanel = m_dialog;
    if (creditsPanel != 0) {
        ((HudUiDialogController *)creditsPanel)->BlitOwnedSurfaceToPrimary();
        ((HudUiContainer *)creditsPanel)->InvalidateChildren();
    }
}

/**
 * Reimplements 0x409a60: RecoilStateCredits::OnTryBecomeCurrent.
 *
 * Purpose: allocate, construct, and enable the credits dialog when the credits
 * app state becomes current.
 */
int RecoilStateCredits::OnTryBecomeCurrent() {
    HudUiCreditsPanel *creditsPanel =
        (HudUiCreditsPanel *) ::operator new(sizeof(HudUiCreditsPanel));
    if (creditsPanel != 0) {
        creditsPanel = new (creditsPanel) HudUiCreditsPanel;
    }
    m_dialog = creditsPanel;

    creditsPanel->SetEnabled(1);
    return 1;
}

/**
 * Reimplements 0x409ad0: RecoilStateCredits::OnDeactivate.
 *
 * Purpose: disable, repaint, destroy, and clear the active credits dialog when
 * leaving the credits state.
 */
void RecoilStateCredits::OnDeactivate() {
    HudUiCreditsPanel *creditsPanel = m_dialog;
    if (creditsPanel == 0) {
        return;
    }

    creditsPanel->SetEnabled(0);
    ((HudUiDialogController *)creditsPanel)->BlitOwnedSurfaceToPrimary();

    creditsPanel = m_dialog;
    if (creditsPanel != 0) {
        creditsPanel->ScalarDeletingDestructor(1);
    }

    m_dialog = 0;
}

/**
 * Reimplements 0x4099f0: RecoilStateCredits::~RecoilStateCredits.
 *
 * Purpose: tear down the owned credits dialog during static state destruction.
 */
RecoilStateCredits::~RecoilStateCredits() {
    HudUiCreditsPanel *creditsPanel = m_dialog;
    if (creditsPanel != 0) {
        creditsPanel->SetEnabled(0);

        creditsPanel = m_dialog;
        if (creditsPanel != 0) {
            creditsPanel->ScalarDeletingDestructor(1);
        }

        m_dialog = 0;
    }
}

/**
 * Reimplements 0x409b00: RecoilStateCredits::QueuePush.
 *
 * Purpose: queue the global credits state as the next pushed RecoilApp state.
 */
void RecoilStateCredits::QueuePush() {
    g_RecoilApp.QueuePushState(
        &g_RecoilStateCredits,
        0
    );
}
