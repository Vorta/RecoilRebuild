#include "Battlesport/Briefing.h"

#include <cstdint>

namespace {
int g_deleteCount;
std::uint32_t g_deleteFlags;
HudUiBriefingRuntime *g_deletedRuntime;

struct TestBriefingRuntime : HudUiBriefingRuntime {
    HudUiBriefingRuntime *RECOIL_THISCALL ScalarDeletingDtor(std::uint32_t flags) {
        ++g_deleteCount;
        g_deleteFlags = flags;
        g_deletedRuntime = this;
        return this;
    }
};

HudUiBriefingRuntimeScalarDeletingDestructor MakeScalarDeletingDtorThunk() {
    union MemberToFunction {
        HudUiBriefingRuntime *(RECOIL_THISCALL TestBriefingRuntime::*member)(std::uint32_t);
        HudUiBriefingRuntimeScalarDeletingDestructor fn;
    };

    MemberToFunction thunk{};
    thunk.member = &TestBriefingRuntime::ScalarDeletingDtor;
    return thunk.fn;
}
} // namespace

extern "C" int briefing_stop_and_shutdown_thread_smoke(void) {
    static TestBriefingRuntime runtime;
    static HudUiBriefingRuntimeVtable vtable;

    g_deleteCount = 0;
    g_deleteFlags = 0;
    g_deletedRuntime = nullptr;
    vtable.ScalarDeletingDtor = MakeScalarDeletingDtorThunk();
    runtime.vptr = &vtable;

    g_Briefing_Runtime = &runtime;
    g_Briefing_ThreadRunFlag = 1;
    g_Briefing_ThreadExitedFlag = 1;
    g_Briefing_SequenceActiveFlag = 1;
    g_Briefing_AllowAdvanceFlag = 1;
    g_Briefing_SystemActiveFlag = 1;

    Briefing::StopAndShutdownThread(0);
    if (g_Briefing_ThreadRunFlag != 0 || g_Briefing_SystemActiveFlag != 0 ||
        g_Briefing_Runtime != nullptr || g_deleteCount != 1 || g_deleteFlags != 1 ||
        g_deletedRuntime != &runtime) {
        return 1;
    }

    g_Briefing_ThreadRunFlag = 1;
    g_Briefing_ThreadExitedFlag = 1;
    g_Briefing_SequenceActiveFlag = 0;
    g_Briefing_AllowAdvanceFlag = 0;
    g_Briefing_SystemActiveFlag = 1;
    Briefing::StopAndShutdownThread(1);

    return g_Briefing_ThreadRunFlag == 0 && g_Briefing_SystemActiveFlag == 0 &&
                   g_Briefing_Runtime == nullptr && g_deleteCount == 1
               ? 0
               : 2;
}
