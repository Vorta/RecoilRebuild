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
// Reimplements 0x4a5670: Time::Reset
RECOIL_NOINLINE void RECOIL_CDECL Reset() {
    g_Time_NewTimeSec = 0.0f;
    g_Time_UnscaledAccumulatedTimeSec = 0.0f;
    g_Time_UnscaledDeltaTimeSec = 0.0f;
    g_Time_AccumulatedTimeSec = 0.0f;
    g_FrameDeltaTimeSec = 0.0f;

    const unsigned __int64 tickCountMillis = GetTickCount();
    g_Time_CurrentTimeSec =
        static_cast<float>(static_cast<long double>(tickCountMillis) * 0.00100000005f);
}

// Reimplements 0x4a56d0: Time::Tick (Time.cpp)
RECOIL_NOINLINE void RECOIL_CDECL Tick() {
    const unsigned __int64 tickCountMillis = GetTickCount();
    const float newTimeSec =
        static_cast<float>(static_cast<long double>(tickCountMillis) * 0.00100000005f);
    const float unscaledDeltaTimeSec = newTimeSec - g_Time_CurrentTimeSec;
    const float timeScaleFactor = g_Time_TimeScaleFactor;

    g_Time_TimeScaleFactor = 1.0f;
    g_Time_NewTimeSec = newTimeSec;
    g_Time_UnscaledDeltaTimeSec = unscaledDeltaTimeSec;
    g_Time_UnscaledAccumulatedTimeSec += unscaledDeltaTimeSec;
    g_FrameDeltaTimeSec = timeScaleFactor * unscaledDeltaTimeSec;

    if (g_Time_DeltaTimeClampEnabled != 0 && g_FrameDeltaTimeSec > g_Time_MaximumDeltaTimeSec) {
        g_FrameDeltaTimeSec = g_Time_MaximumDeltaTimeSec;
    }

    g_Time_CurrentTimeSec = newTimeSec;
    g_Time_AccumulatedTimeSec += g_FrameDeltaTimeSec;
}
} // namespace Time
