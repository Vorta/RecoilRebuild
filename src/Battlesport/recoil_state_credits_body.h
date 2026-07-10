#include "Battlesport/recoil_state_credits.h"
#include "Battlesport/hud.h"
#include "GameZRecoil/zHud/zhud_ui.h"

#include <new>
#include <stdlib.h>

/**
 * Reimplements data 0x4e5de0: g_RecoilStateCredits.
 *
 * Source owner: legacy.app_shell.cluster_recoilstatebase.
 * Purpose: own the zero-initialized credits app-state singleton storage.
 *
 * Source model note: StaticInit constructs the typed object in this storage and
 * AtExitDestructor tears it down through the CRT at-exit list.
 */
#undef g_RecoilStateCredits
RecoilStateCreditsStorage g_RecoilStateCredits = {0};
#define g_RecoilStateCredits \
    (*(RecoilStateCredits *)&g_RecoilStateCredits)

/**
 * Reimplements 0x409950: RecoilStateCredits::StaticInitAndRegisterAtExit.
 *
 * Purpose: construct the global credits app state and register its CRT
 * shutdown destructor.
 */
void RecoilStateCredits::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

#if defined(_MSC_VER) && defined(_M_IX86)
typedef void (__cdecl *RecoilStateCreditsCrtInitializerFn)();
/* VC5 emits this credits-state startup callback as a direct .CRT$XCU row. */
#pragma data_seg(".CRT$XCU")
RecoilStateCreditsCrtInitializerFn s_RecoilStateCreditsCrtInit =
    RecoilStateCredits::StaticInitAndRegisterAtExit;
#pragma data_seg()
#endif

/**
 * Reimplements 0x409960: RecoilStateCredits::StaticInit.
 *
 * Purpose: placement-construct the zero-initialized global credits app-state
 * singleton.
 */
RecoilStateCredits *RecoilStateCredits::StaticInit() {
    return new (&g_RecoilStateCredits) RecoilStateCredits;
}

/**
 * Reimplements 0x409970: RecoilStateCredits::RegisterAtExit.
 *
 * Purpose: register the global credits app-state destructor with the CRT
 * at-exit list.
 */
void RecoilStateCredits::RegisterAtExit() {
    atexit(AtExitDestructor);
}

/**
 * Reimplements 0x409980: RecoilStateCredits::AtExitDestructor.
 *
 * Purpose: destroy the global credits app state during CRT shutdown.
 */
void RecoilStateCredits::AtExitDestructor() {
    g_RecoilStateCredits.~RecoilStateCredits();
}

/**
 * Reimplements 0x409990: RecoilStateCredits::RecoilStateCredits.
 *
 * Purpose: initialize the credits app-state object and clear the active
 * credits-panel pointer.
 */
RecoilStateCredits::RecoilStateCredits() {
    m_dialog = 0;
}

/**
 * Reimplements 0x4099a0: RecoilStateDialogHost::OnWndActivate.
 *
 * Purpose: refresh the hosted HUD dialog surfaces when the application is
 * reactivated.
 */
void RecoilStateDialogHost::OnWndActivate(
    int activateCode
) {
    if (activateCode == 0) {
        return;
    }

    if (m_dialog == 0) {
        return;
    }

    ((HudUiDialogController *)m_dialog)->BlitOwnedSurfaceToPrimary();
    m_dialog->InvalidateChildren();
}

/**
 * Reimplements 0x4099f0: RecoilStateCredits::~RecoilStateCredits.
 *
 * Purpose: tear down the owned credits dialog during static state destruction.
 */
RecoilStateCredits::~RecoilStateCredits() {
    HudUiCreditsPanel *creditsPanel = (HudUiCreditsPanel *)m_dialog;
    if (creditsPanel != 0) {
        creditsPanel->SetEnabled(0);

        creditsPanel = (HudUiCreditsPanel *)m_dialog;
        if (creditsPanel != 0) {
            creditsPanel->ScalarDeletingDestructor(1);
        }

        m_dialog = 0;
    }

    /* Late ABI reset keeps RecoilStateBase materialization after the zStub block. */
    ((RecoilStateBase *)this)->RecoilStateBase::~RecoilStateBase();
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
 * Reimplements 0x409ad0: RecoilStateDialogHost::OnDeactivate.
 *
 * Purpose: disable, repaint, destroy, and clear the active hosted HUD dialog.
 */
void RecoilStateDialogHost::OnDeactivate() {
    if (m_dialog == 0) {
        return;
    }

    m_dialog->SetEnabled(0);
    ((HudUiDialogController *)m_dialog)->BlitOwnedSurfaceToPrimary();

    if (m_dialog != 0) {
        ((HudUiBackground *)m_dialog)->ScalarDeletingDestructor(1);
    }

    m_dialog = 0;
}

/**
 * Reimplements 0x409b00: RecoilStateCredits::QueuePush.
 *
 * Purpose: queue the global credits state as the next pushed RecoilApp state.
 */
void RecoilStateCredits::QueuePush() {
    g_RecoilApp.QueuePushState(
        (RecoilApp_IState *)&g_RecoilStateCredits,
        0
    );
}
