#include "Battlesport/recoil_state_credits.h"
#include "Battlesport/hud.h"
#include "GameZRecoil/zHud/zhud_ui.h"

#include <new>

/**
 * Reimplements data 0x4e5de0: g_RecoilStateCredits.
 *
 * Purpose: store the static credits app-state object.
 *
 * Source model note: RecoilStateCredits is a credits app-state singleton.
 */
RecoilStateCredits g_RecoilStateCredits;

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
