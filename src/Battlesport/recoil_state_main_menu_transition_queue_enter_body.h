#include "GameZRecoil/RecoilApp/recoil_state_main_menu_transition.h"

/**
 * Reimplements 0x415650: RecoilStateMainMenuTransition::QueueEnter.
 *
 * Purpose: record the requested main-menu entry route and queue the global
 * transition state on RecoilApp's app-state stack.
 */
void __fastcall RecoilStateMainMenuTransition::QueueEnter(
    RecoilMainMenuEntryRoute entryRoute
) {
    g_RecoilState_MainMenuTransition.m_entryRoute = entryRoute;
    g_RecoilApp.QueuePushState(
        (RecoilApp_IState *)&g_RecoilState_MainMenuTransition,
        0
    );
}
