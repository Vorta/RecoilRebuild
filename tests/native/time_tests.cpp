#include "GameZRecoil/Time/Time.h"

#include <windows.h>

extern "C" int time_reset_smoke(void) {
    g_Time_NewTimeSec = 1.0f;
    g_FrameDeltaTimeSec = 2.0f;
    g_Time_AccumulatedTimeSec = 3.0f;
    g_Time_UnscaledDeltaTimeSec = 4.0f;
    g_Time_UnscaledAccumulatedTimeSec = 5.0f;
    g_Time_CurrentTimeSec = 6.0f;

    const float before = static_cast<float>(GetTickCount() * 0.001);
    Time::Reset();
    const float after = static_cast<float>(GetTickCount() * 0.001);

    if (g_Time_NewTimeSec != 0.0f || g_FrameDeltaTimeSec != 0.0f ||
        g_Time_AccumulatedTimeSec != 0.0f || g_Time_UnscaledDeltaTimeSec != 0.0f ||
        g_Time_UnscaledAccumulatedTimeSec != 0.0f) {
        return 1;
    }

    return g_Time_CurrentTimeSec >= before - 0.01f && g_Time_CurrentTimeSec <= after + 0.01f ? 0
                                                                                             : 2;
}
