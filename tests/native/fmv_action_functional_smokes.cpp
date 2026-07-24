#include "GameZRecoil/zFMV/fmv.h"

#include <new>

namespace {

struct BlockingAction : zFMV_Action {
    int beginCallCount;
    int updateCallCount;
    int endCallCount;
    int nextUpdateResult;
    double lastBeginTimeSec;
    double lastUpdateTimeSec;

    BlockingAction()
        : beginCallCount(0),
          updateCallCount(0),
          endCallCount(0),
          nextUpdateResult(0),
          lastBeginTimeSec(-1.0),
          lastUpdateTimeSec(-1.0) {}

    void Begin(
        double timeSec
    ) {
        ++beginCallCount;
        lastBeginTimeSec = timeSec;
    }

    int Update(
        double timeSec
    ) {
        ++updateCallCount;
        lastUpdateTimeSec = timeSec;
        return nextUpdateResult;
    }

    void End() {
        ++endCallCount;
    }
};

} // namespace

extern "C" int zfmv_action_base_destructor_smoke(void) {
    {
        zFMV_Action action;
        action.next = &action;
        if (action.next != &action) {
            return 1;
        }
    }

    zFMV_Action *const action = new (std::nothrow) zFMV_Action;
    if (action == 0) {
        return 2;
    }

    action->next = action;
    if (action->next != action) {
        delete action;
        return 3;
    }

    delete action;
    return 0;
}

extern "C" int zfmv_action_derived_scalar_deleting_destructor_smoke(void) {
    zFMV_ActionWait *const action = new (std::nothrow) zFMV_ActionWait;
    if (action == 0) {
        return 1;
    }

    action->durationSec = 1.0f;
    action->startSec = 2.0f;
    delete action;
    return 0;
}

extern "C" int zfmv_action_no_op_update_smoke(void) {
    zFMV_Action action;
    action.next = &action;

    const int result = action.Update(456.25);
    return result == 0 && action.next == &action ? 0 : 1;
}

extern "C" int zfmv_action_play_sound_begin_missing_sample_smoke(void) {
    zFMV_ActionPlaySound action("__missing_fmv_sample__");
    action.sample = reinterpret_cast<zSndSample *>(1);

    action.Begin(0.0);

    return action.sample == 0 && action.voice == 0 ? 0 : 1;
}

extern "C" int zfmv_action_run_blocking_immediate_smoke(void) {
    BlockingAction action;
    action.RunBlockingImmediate();

    return action.beginCallCount == 1 && action.lastBeginTimeSec == 0.0 &&
                   action.updateCallCount == 1 && action.lastUpdateTimeSec == 0.0 &&
                   action.endCallCount == 1
               ? 0
               : 1;
}

extern "C" int zfmv_action_run_blocking_timed_smoke(void) {
    BlockingAction action;
    action.RunBlockingTimed();

    return action.beginCallCount == 1 && action.lastBeginTimeSec == 0.0 &&
                   action.updateCallCount == 1 && action.lastUpdateTimeSec >= 0.0 &&
                   action.endCallCount == 1
               ? 0
               : 1;
}
