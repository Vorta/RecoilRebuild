#include "Battlesport/briefing.h"

extern "C" int briefing_stop_and_shutdown_thread_smoke(void) {
    HudUiBriefingRuntime *const savedRuntime = g_Briefing_Runtime;
    const int savedThreadRunFlag = g_Briefing_ThreadRunFlag;
    const int savedThreadExitedFlag = g_Briefing_ThreadExitedFlag;
    const int savedSequenceActiveFlag = g_Briefing_SequenceActiveFlag;
    const int savedAllowAdvanceFlag = g_Briefing_AllowAdvanceFlag;
    const int savedSystemActiveFlag = g_Briefing_SystemActiveFlag;

    g_Briefing_Runtime = 0;
    g_Briefing_ThreadRunFlag = 1;
    g_Briefing_ThreadExitedFlag = 1;
    g_Briefing_SequenceActiveFlag = 1;
    g_Briefing_AllowAdvanceFlag = 1;
    g_Briefing_SystemActiveFlag = 1;

    Briefing::StopAndShutdownThread(0);
    const bool immediateStopOk =
        g_Briefing_ThreadRunFlag == 0 &&
        g_Briefing_SystemActiveFlag == 0 &&
        g_Briefing_Runtime == 0;

    /*
     * Keep the wait-for-input branch deterministic: an inactive sequence
     * skips the input provider while still exercising the shared shutdown.
     */
    g_Briefing_ThreadRunFlag = 1;
    g_Briefing_ThreadExitedFlag = 1;
    g_Briefing_SequenceActiveFlag = 0;
    g_Briefing_AllowAdvanceFlag = 0;
    g_Briefing_SystemActiveFlag = 1;
    Briefing::StopAndShutdownThread(1);
    const bool waitBranchOk =
        g_Briefing_ThreadRunFlag == 0 &&
        g_Briefing_SystemActiveFlag == 0 &&
        g_Briefing_Runtime == 0;

    g_Briefing_Runtime = savedRuntime;
    g_Briefing_ThreadRunFlag = savedThreadRunFlag;
    g_Briefing_ThreadExitedFlag = savedThreadExitedFlag;
    g_Briefing_SequenceActiveFlag = savedSequenceActiveFlag;
    g_Briefing_AllowAdvanceFlag = savedAllowAdvanceFlag;
    g_Briefing_SystemActiveFlag = savedSystemActiveFlag;

    return immediateStopOk && waitBranchOk ? 0 : 1;
}
