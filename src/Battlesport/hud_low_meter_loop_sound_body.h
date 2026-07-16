#include "Battlesport/Mfc42Abi.h"
#include "Battlesport/hud.h"
#include "GameZRecoil/zSound/zsnd.h"

/*
 * Complete HudLowMeterLoopSound state and implementation. This semantic
 * fragment is compiled once by a provisional later-shelf carrier while the
 * original mixed Player/combat translation-unit filename remains unresolved.
 */

/**
 * Reimplements data 0x4f3748: g_Hud_LowMeterBeepSample.
 * Source owner: hud_ui.hud_low_meter_loop_sound_globals.
 * Purpose: Holds the one-shot low-meter warning sample loaded from player.zrd.
 */
zSndSample *g_Hud_LowMeterBeepSample = 0;
/**
 * Reimplements data 0x4f374c: g_Hud_LowMeterLoopSample.
 * Source owner: hud_ui.hud_low_meter_loop_sound_globals.
 * Purpose: Holds the looped low-meter warning sample loaded from player.zrd.
 */
zSndSample *g_Hud_LowMeterLoopSample = 0;
/**
 * Reimplements data 0x4f3750: g_Hud_LowMeterLoopActive.
 * Source owner: hud_ui.hud_low_meter_loop_sound_globals.
 * Purpose: Tracks whether the low-meter loop sample has been started.
 */
int g_Hud_LowMeterLoopActive = 0;
/**
 * Reimplements data 0x4f3758: g_Hud_LowMeterBeepInterval.
 * Source owner: hud_ui.hud_low_meter_loop_sound_globals.
 * Purpose: Stores the low-meter one-shot beep interval from player.zrd.
 */
float g_Hud_LowMeterBeepInterval = 0.0f;
/**
 * Reimplements data 0x4f375c: g_Hud_LowMeterNextBeepTime.
 * Source owner: hud_ui.hud_low_meter_loop_sound_globals.
 * Purpose: Stores the next absolute mission time for a low-meter one-shot beep.
 */
float g_Hud_LowMeterNextBeepTime = 0.0f;

namespace HudLowMeterLoopSound {

/**
 * Reimplements 0x439b20: HudLowMeterLoopSound::SetLoopActive.
 * Original source filename remains unresolved in the mixed later Player/combat shelf.
 * Purpose: Starts or stops the low-meter loop sample on active-state changes.
 */
void __fastcall SetLoopActive(
    int enabled
) {
    const int wasActive = g_Hud_LowMeterLoopActive;
    if (enabled == 0) {
        if (wasActive != 0) {
            g_Hud_LowMeterLoopSample->StopActiveVoicesIfPlaying();
            g_Hud_LowMeterLoopActive = 0;
        }
        return;
    }

    if (wasActive == 0) {
        g_Hud_LowMeterLoopSample->PlayA3DSimple(1.0f);
        g_Hud_LowMeterLoopActive = 1;
    }
}

/**
 * Reimplements 0x439b70: HudLowMeterLoopSound::Disable.
 * Original source filename remains unresolved in the mixed later Player/combat shelf.
 * Purpose: Stops both low-meter warning samples and clears the loop-active flag.
 */
void Disable() {
    g_Hud_LowMeterBeepSample->StopActiveVoicesIfPlaying();
    g_Hud_LowMeterLoopSample->StopActiveVoicesIfPlaying();
    g_Hud_LowMeterLoopActive = 0;
}

} // namespace HudLowMeterLoopSound
