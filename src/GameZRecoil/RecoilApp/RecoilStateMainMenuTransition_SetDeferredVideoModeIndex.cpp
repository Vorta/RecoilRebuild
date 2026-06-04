#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"

// Reimplements 0x415670: RecoilStateMainMenuTransition::SetDeferredVideoModeIndex
void __fastcall RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(
    zVidModeIndex modeIndex
) {
    g_RecoilState_MainMenuTransition.m_deferredVideoModeIndex = modeIndex;
}
