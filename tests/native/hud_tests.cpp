#include "Battlesport/hud.h"

extern "C" int hud_low_meter_loop_sound_set_loop_active_smoke(void)
{
    zSndSample *const oldBeepSample = g_Hud_LowMeterBeepSample;
    zSndSample *const oldSample = g_Hud_LowMeterLoopSample;
    const int oldActive = g_Hud_LowMeterLoopActive;

    g_Hud_LowMeterBeepSample = 0;
    g_Hud_LowMeterLoopSample = 0;
    g_Hud_LowMeterLoopActive = 0;

    HudLowMeterLoopSound::SetLoopActive(1);
    const bool enabledOk = g_Hud_LowMeterLoopActive == 1;

    HudLowMeterLoopSound::SetLoopActive(1);
    const bool enableIdempotentOk = g_Hud_LowMeterLoopActive == 1;

    HudLowMeterLoopSound::SetLoopActive(0);
    const bool disabledOk = g_Hud_LowMeterLoopActive == 0;

    HudLowMeterLoopSound::SetLoopActive(0);
    const bool disableIdempotentOk = g_Hud_LowMeterLoopActive == 0;

    g_Hud_LowMeterLoopActive = 1;
    HudLowMeterLoopSound::Disable();
    const bool fullDisableOk = g_Hud_LowMeterLoopActive == 0;

    g_Hud_LowMeterBeepSample = oldBeepSample;
    g_Hud_LowMeterLoopSample = oldSample;
    g_Hud_LowMeterLoopActive = oldActive;

    return enabledOk && enableIdempotentOk && disabledOk && disableIdempotentOk &&
                   fullDisableOk
               ? 0
               : 1;
}
