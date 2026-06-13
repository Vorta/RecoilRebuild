#include "Time.h"

#include "recoil/recoil_types.h"

#include <windows.h>

extern "C" {
float g_Time_MaximumDeltaTimeSec = 0.125f;
int g_Time_DeltaTimeClampEnabled = 1;
float g_Time_CurrentTimeSec = 0.0f;
float g_Time_NewTimeSec = 0.0f;
float g_Time_TimeScaleFactor = 1.0f;
float g_FrameDeltaTimeSec = 0.0f;
float g_Time_AccumulatedTimeSec = 0.0f;
float g_Time_UnscaledDeltaTimeSec = 0.0f;
float g_Time_UnscaledAccumulatedTimeSec = 0.0f;
}

namespace Time {
// BN .rdata 0x4d2f50 is referenced only by Time::Reset and Time::Tick.
static const float g_Time_MillisecondsToSecondsScale = 0.00100000005f;

/**
 * Reimplements 0x4a5670: Time::Reset.
 * Purpose: Clears accumulated frame timing state and seeds the current time from GetTickCount.
 */
void Reset() {
    g_Time_NewTimeSec = 0.0f;
    g_Time_UnscaledAccumulatedTimeSec = 0.0f;
    g_Time_UnscaledDeltaTimeSec = 0.0f;
    g_Time_AccumulatedTimeSec = 0.0f;
    g_FrameDeltaTimeSec = 0.0f;

    const __int64 tickCountMillis = GetTickCount();
    g_Time_CurrentTimeSec = (float)(tickCountMillis) * g_Time_MillisecondsToSecondsScale;
}

/**
 * Reimplements 0x4a56d0: Time::Tick (Time.cpp).
 * Purpose: Advances scaled and unscaled frame time, applying the configured maximum-delta clamp.
 */
void Tick() {
    const __int64 tickCountMillis = GetTickCount();
    const float unscaledAccumulatedTimeSec = g_Time_UnscaledAccumulatedTimeSec;
    const float newTimeSec = (float)(tickCountMillis) * g_Time_MillisecondsToSecondsScale;
    const int deltaTimeClampEnabled = g_Time_DeltaTimeClampEnabled;

    g_Time_NewTimeSec = newTimeSec;
    g_Time_UnscaledDeltaTimeSec = newTimeSec - g_Time_CurrentTimeSec;
    const float frameDeltaTimeSec = g_Time_TimeScaleFactor * g_Time_UnscaledDeltaTimeSec;

    g_Time_UnscaledAccumulatedTimeSec =
        unscaledAccumulatedTimeSec + g_Time_UnscaledDeltaTimeSec;
    g_FrameDeltaTimeSec = frameDeltaTimeSec;
    g_Time_TimeScaleFactor = 1.0f;

    if (deltaTimeClampEnabled != 0 && g_FrameDeltaTimeSec > g_Time_MaximumDeltaTimeSec) {
        g_FrameDeltaTimeSec = g_Time_MaximumDeltaTimeSec;
    }

    g_Time_CurrentTimeSec = newTimeSec;
    g_Time_AccumulatedTimeSec += g_FrameDeltaTimeSec;
}
} // namespace Time
