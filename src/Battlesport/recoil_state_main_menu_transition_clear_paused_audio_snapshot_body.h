#include "GameZRecoil/RecoilApp/recoil_state_main_menu_transition.h"

/**
 * Reimplements 0x415630: RecoilStateMainMenuTransition::ClearPausedAudioSnapshot.
 *
 * Purpose: destroy and clear the global main-menu transition paused-audio
 * snapshot when callers need to discard the saved audio state.
 */
void RecoilStateMainMenuTransition::ClearPausedAudioSnapshot() {
    zSndPlayHandleSnapshot *const snapshot =
        (zSndPlayHandleSnapshot *)g_RecoilState_MainMenuTransition.m_pausedAudioSnapshot;
    if (snapshot != 0) {
        snapshot->Destroy();
        g_RecoilState_MainMenuTransition.m_pausedAudioSnapshot = 0;
    }
}
