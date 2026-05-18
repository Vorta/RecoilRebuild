#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"

// Reimplements 0x415650: RecoilStateMainMenuTransition::QueueEnter
void RECOIL_FASTCALL
RecoilStateMainMenuTransition::QueueEnter(RecoilMainMenuEntryRoute entryRoute) {
    g_RecoilState_MainMenuTransition.m_entryRoute = entryRoute;
    g_RecoilApp.QueuePushState((RecoilApp_IState *)&g_RecoilState_MainMenuTransition, 0);
}
