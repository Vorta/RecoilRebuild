#include "GameZRecoil/Time/time.h"

#include "recoil/recoil_types.h"

#if defined(_MSC_VER) && _MSC_VER <= 1100
extern "C" unsigned long __stdcall GetTickCount();
#else
#include <windows.h>
#endif

extern "C" {
/**
 * Reimplements data 0x4e2fa8..0x4e2fb8: g_Time_RuntimeConfig.
 * Purpose: Stores the initialized Time runtime config/current-time record.
 */
TimeRuntimeConfig g_Time_RuntimeConfig = {0.125f, 1, 0.0f, 0.0f, 1.0f};
/**
 * Reimplements data 0x56b424: g_FrameDeltaTimeSec.
 * Purpose: Stores g FrameDeltaTimeSec data used by engine.time_runtime_globals.
 */
float g_FrameDeltaTimeSec = 0.0f;
/**
 * Reimplements data 0x56b428: g_Time_AccumulatedTimeSec.
 * Purpose: Stores g Time AccumulatedTimeSec data used by engine.time_runtime_globals.
 */
float g_Time_AccumulatedTimeSec = 0.0f;
/**
 * Reimplements data 0x56b42c: g_Time_UnscaledDeltaTimeSec.
 * Purpose: Stores g Time UnscaledDeltaTimeSec data used by engine.time_runtime_globals.
 */
float g_Time_UnscaledDeltaTimeSec = 0.0f;
/**
 * Reimplements data 0x56b430: g_Time_UnscaledAccumulatedTimeSec.
 * Purpose: Stores g Time UnscaledAccumulatedTimeSec data used by engine.time_runtime_globals.
 */
float g_Time_UnscaledAccumulatedTimeSec = 0.0f;
}

namespace Time {
/**
 * Reimplements data 0x4d2f50: g_Time_MillisecondsToSecondsScale.
 * Purpose: converts GetTickCount millisecond values to seconds for Time reset
 * and tick accumulation.
 */
static const float g_Time_MillisecondsToSecondsScale = 0.00100000005f;

/**
 * Reimplements 0x4a5670: Time::Reset.
 * Purpose: Clears accumulated frame timing state and seeds the current time from GetTickCount.
 */
void Reset() {
    g_Time_RuntimeConfig.newTimeSec = 0.0f;
    g_Time_UnscaledAccumulatedTimeSec = 0.0f;
    g_Time_UnscaledDeltaTimeSec = 0.0f;
    g_Time_AccumulatedTimeSec = 0.0f;
    g_FrameDeltaTimeSec = 0.0f;

    const __int64 tickCountMillis = GetTickCount();
    g_Time_RuntimeConfig.currentTimeSec =
        (float)(tickCountMillis) * g_Time_MillisecondsToSecondsScale;
}

/**
 * Reimplements 0x4a56d0: Time::Tick (Time.cpp).
 * Purpose: Advances scaled and unscaled frame time, applying the configured maximum-delta clamp.
 */
void Tick() {
    const __int64 tickCountMillis = GetTickCount();
    const float unscaledAccumulatedTimeSec = g_Time_UnscaledAccumulatedTimeSec;
    const float newTimeSec = (float)(tickCountMillis) * g_Time_MillisecondsToSecondsScale;
    const int deltaTimeClampEnabled = g_Time_RuntimeConfig.deltaTimeClampEnabled;

    g_Time_RuntimeConfig.newTimeSec = newTimeSec;
    g_Time_UnscaledDeltaTimeSec = newTimeSec - g_Time_RuntimeConfig.currentTimeSec;
    const float frameDeltaTimeSec =
        g_Time_RuntimeConfig.timeScaleFactor * g_Time_UnscaledDeltaTimeSec;

    g_Time_UnscaledAccumulatedTimeSec =
        unscaledAccumulatedTimeSec + g_Time_UnscaledDeltaTimeSec;
    g_FrameDeltaTimeSec = frameDeltaTimeSec;
    g_Time_RuntimeConfig.timeScaleFactor = 1.0f;

    if (
        deltaTimeClampEnabled != 0 &&
        g_FrameDeltaTimeSec > g_Time_RuntimeConfig.maximumDeltaTimeSec
    ) {
        g_FrameDeltaTimeSec = g_Time_RuntimeConfig.maximumDeltaTimeSec;
    }

    g_Time_RuntimeConfig.currentTimeSec = newTimeSec;
    g_Time_AccumulatedTimeSec += g_FrameDeltaTimeSec;
}
} // namespace Time
