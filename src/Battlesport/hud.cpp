#include "Battlesport/hud.h"

HudUiNewGamePanelOverlayOwner g_HudUiNewGamePanelOverlayOwner;
HudUiOptionsPanelOverlayOwner g_HudUiOptionsPanelOverlayOwner;
RecoilStateConfirmQuit g_RecoilState_ConfirmQuit;
RecoilStateControls g_RecoilStateControls;

// Reimplements 0x41c6c0: HudUiNewGamePanelOverlayOwner::QueueEnter
// (D:\Proj\Battlesport\hud.cpp)
void RECOIL_CDECL HudUiNewGamePanelOverlayOwner::QueueEnter()
{
    g_RecoilApp.QueuePushState(&g_HudUiNewGamePanelOverlayOwner, 0);
}

// Reimplements 0x40d1c0: HudUiOptionsPanelOverlayOwner::QueueEnter
// (D:\Proj\Battlesport\HudOptionsDialog.cpp)
void RECOIL_CDECL HudUiOptionsPanelOverlayOwner::QueueEnter()
{
    g_RecoilApp.QueuePushState(&g_HudUiOptionsPanelOverlayOwner, 0);
}

// Reimplements 0x4159b0: RecoilStateConfirmQuit::QueueEnter
// (D:\Proj\Battlesport\HudConfirmQuitDialog.cpp)
void RECOIL_CDECL RecoilStateConfirmQuit::QueueEnter()
{
    g_RecoilApp.QueuePushState(&g_RecoilState_ConfirmQuit, 0);
}

// Reimplements 0x408ff0: RecoilStateControls::QueueEnter
// (D:\Proj\Battlesport\recoil_state.cpp)
void RECOIL_CDECL RecoilStateControls::QueueEnter()
{
    g_RecoilApp.QueuePushState(&g_RecoilStateControls, 0);
}
