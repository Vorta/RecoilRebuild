#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"

/**
 * Reimplements 0x415670: RecoilStateMainMenuTransition::SetDeferredVideoModeIndex.
 *
 * Purpose: store the requested video-mode index on the global main-menu
 * transition state for deferred application during transition shutdown.
 */
void __fastcall RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(
    zVidModeIndex modeIndex
) {
    g_RecoilState_MainMenuTransition.m_deferredVideoModeIndex = modeIndex;
}
