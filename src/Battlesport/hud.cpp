#include "Battlesport/hud.h"

#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zSound/zSound.h"

HudUiNewGamePanelOverlayOwner g_HudUiNewGamePanelOverlayOwner;
HudUiOptionsPanelOverlayOwner g_HudUiOptionsPanelOverlayOwner;
RecoilStateConfirmQuit g_RecoilState_ConfirmQuit;
RecoilStateControls g_RecoilStateControls;
RecoilStateCheatCode g_RecoilStateCheatCode;
zSndSample *g_Hud_LowMeterBeepSample = 0;
zSndSample *g_Hud_LowMeterLoopSample = 0;
int g_Hud_LowMeterLoopActive = 0;
float g_Hud_LowMeterBeepInterval = 0.0f;
float g_Hud_LowMeterNextBeepTime = 0.0f;

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

// Reimplements 0x407110: HudUiCallback::QueueCheatCodeState
// (D:\Proj\Battlesport\hud.cpp)
int RECOIL_CDECL HudUiCallback::QueueCheatCodeState()
{
    g_RecoilApp.QueuePushState(&g_RecoilStateCheatCode, 0);
    return 1;
}

namespace zOpt {

// Reimplements 0x413600: zOpt::ToggleHudTypeForCurrentHwMode
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE int RECOIL_CDECL ToggleHudTypeForCurrentHwMode()
{
    const int currentHudType = GetHudTypeForCurrentHwMode();
    if (currentHudType == ZOPT_HUD_TYPE_STANDARD)
    {
        return SetHudTypeForCurrentHwMode(ZOPT_HUD_TYPE_PERSPECTIVE);
    }
    if (currentHudType == ZOPT_HUD_TYPE_PERSPECTIVE)
    {
        return SetHudTypeForCurrentHwMode(ZOPT_HUD_TYPE_STANDARD);
    }
    return GetHudTypeForCurrentHwMode();
}

} // namespace zOpt

namespace HudLowMeterLoopSound {

// Reimplements 0x439b20: HudLowMeterLoopSound::SetLoopActive
// (D:\Proj\Battlesport\Hud.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SetLoopActive(int enabled)
{
    const int wasActive = g_Hud_LowMeterLoopActive;
    if (enabled == 0)
    {
        if (wasActive != 0)
        {
            g_Hud_LowMeterLoopSample->StopActiveVoicesIfPlaying();
            g_Hud_LowMeterLoopActive = 0;
        }
        return;
    }

    if (wasActive == 0)
    {
        g_Hud_LowMeterLoopSample->PlayA3DSimple(1.0f);
        g_Hud_LowMeterLoopActive = 1;
    }
}

// Reimplements 0x439b70: HudLowMeterLoopSound::Disable
// (D:\Proj\Battlesport\Hud.cpp)
RECOIL_NOINLINE void RECOIL_CDECL Disable()
{
    g_Hud_LowMeterBeepSample->StopActiveVoicesIfPlaying();
    g_Hud_LowMeterLoopSample->StopActiveVoicesIfPlaying();
    g_Hud_LowMeterLoopActive = 0;
}

} // namespace HudLowMeterLoopSound
