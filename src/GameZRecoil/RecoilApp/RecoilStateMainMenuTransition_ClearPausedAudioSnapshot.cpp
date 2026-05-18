#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"

// Reimplements 0x415630: RecoilStateMainMenuTransition::ClearPausedAudioSnapshot
void RECOIL_CDECL RecoilStateMainMenuTransition::ClearPausedAudioSnapshot() {
    zSndPlayHandleSnapshot *const snapshot =
        (zSndPlayHandleSnapshot *)g_RecoilState_MainMenuTransition.m_pausedAudioSnapshot;
    if (snapshot != 0) {
        snapshot->Destroy();
        g_RecoilState_MainMenuTransition.m_pausedAudioSnapshot = 0;
    }
}
