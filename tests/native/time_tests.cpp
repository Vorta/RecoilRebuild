#include "GameZRecoil/Time/Time.h"

#include <windows.h>

static float time_test_abs(float value) {
    return value < 0.0f ? -value : value;
}

static int time_test_nearly_equal(float actual, float expected, float tolerance) {
    return time_test_abs(actual - expected) <= tolerance;
}

static float time_test_now_sec(void) {
    return static_cast<float>(static_cast<unsigned __int64>(GetTickCount())) * 0.00100000005f;
}

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

extern "C" int time_tick_smoke(void) {
    const float baseUnclamped = time_test_now_sec();
    g_Time_MaximumDeltaTimeSec = 0.125f;
    g_Time_DeltaTimeClampEnabled = 0;
    g_Time_CurrentTimeSec = baseUnclamped - 0.05f;
    g_Time_NewTimeSec = 0.0f;
    g_Time_TimeScaleFactor = 2.0f;
    g_FrameDeltaTimeSec = 0.0f;
    g_Time_AccumulatedTimeSec = 10.0f;
    g_Time_UnscaledDeltaTimeSec = 0.0f;
    g_Time_UnscaledAccumulatedTimeSec = 20.0f;

    Time::Tick();

    if (g_Time_TimeScaleFactor != 1.0f) {
        return 1;
    }
    if (!time_test_nearly_equal(g_Time_NewTimeSec, g_Time_CurrentTimeSec, 0.0001f)) {
        return 2;
    }
    if (g_Time_UnscaledDeltaTimeSec < 0.03f || g_Time_UnscaledDeltaTimeSec > 0.5f) {
        return 3;
    }
    if (!time_test_nearly_equal(g_FrameDeltaTimeSec, g_Time_UnscaledDeltaTimeSec * 2.0f,
                                0.0002f)) {
        return 4;
    }
    if (!time_test_nearly_equal(g_Time_UnscaledAccumulatedTimeSec,
                                20.0f + g_Time_UnscaledDeltaTimeSec, 0.0002f)) {
        return 5;
    }
    if (!time_test_nearly_equal(g_Time_AccumulatedTimeSec, 10.0f + g_FrameDeltaTimeSec,
                                0.0002f)) {
        return 6;
    }

    const float baseClamped = time_test_now_sec();
    g_Time_MaximumDeltaTimeSec = 0.125f;
    g_Time_DeltaTimeClampEnabled = 1;
    g_Time_CurrentTimeSec = baseClamped - 1.0f;
    g_Time_TimeScaleFactor = 1.0f;
    g_FrameDeltaTimeSec = 0.0f;
    g_Time_AccumulatedTimeSec = 3.0f;
    g_Time_UnscaledAccumulatedTimeSec = 7.0f;

    Time::Tick();

    if (g_Time_UnscaledDeltaTimeSec < g_Time_MaximumDeltaTimeSec) {
        return 7;
    }
    if (!time_test_nearly_equal(g_FrameDeltaTimeSec, g_Time_MaximumDeltaTimeSec, 0.0001f)) {
        return 8;
    }
    if (!time_test_nearly_equal(g_Time_AccumulatedTimeSec, 3.0f + g_Time_MaximumDeltaTimeSec,
                                0.0002f)) {
        return 9;
    }
    if (!time_test_nearly_equal(g_Time_UnscaledAccumulatedTimeSec,
                                7.0f + g_Time_UnscaledDeltaTimeSec, 0.0002f)) {
        return 10;
    }

    return 0;
}
