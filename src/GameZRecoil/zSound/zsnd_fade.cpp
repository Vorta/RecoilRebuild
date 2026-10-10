#include "zsnd.h"

#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd_a3d_provider.h"
#include "GameZRecoil/zTime/time.h"

#include <algorithm>
#include <list>
#include <stdlib.h>
#include <string.h>

/**
 * Data owner: namespace:zSound system configuration state.
 * Purpose: hold the loaded sound configuration tree until sound shutdown.
 */
extern "C" zReader::Node* g_zSnd_ConfigRootNode = 0;
/**
 * Purpose: hold the sound resource search path list built from SOUND_PATH.
 */
extern "C" zArchiveList* g_zSnd_SearchPathList = 0;
/**
 * Data owner: namespace:zSound backend runtime state.
 * Purpose: reference the active A3D or DirectSound backend device.
 */
extern "C" void* g_zSnd_BackendDevice;

namespace {
std::list<zSndFadeEntry*> g_zSndFadeActiveList;
std::list<zSndFadeEntry*> g_zSndFadeDispatchList;
} // namespace

/*
 * compiler-generated static initialization coordinator.
 * compiler-generated constructors for both fade lists.
 * compiler-generated atexit registration helper.
 * compiler-generated destructors for both fade lists.
 * These four contributions arise naturally from the two namespace-scope
 * std::list objects above; they are not authored wrapper functions.
 * The std::list<zSndFadeEntry *>::erase(iterator) and
 * iterator::operator++(int) COMDATs that close this object are likewise
 * <list> template instances, not authored helpers.
 */

namespace zSndFadeDispatchList {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-fade.pushback
 * @recoil-artifact defines .text recoil:function:0x4a3a80: zSndFadeDispatchList::PushBack.
 * @recoil-match byte
 *
 * Purpose: append a completed fade entry to the dispatch list for completion
 * handling.
 */
void __fastcall PushBack(zSndFadeEntry* fadeEntry)
{
    g_zSndFadeDispatchList.push_back(fadeEntry);
}
} // namespace zSndFadeDispatchList

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-fade.zsndfadeentry-tickandmaybedispatch
 * @recoil-artifact defines .text recoil:function:0x4a3ad0: zSndFadeEntry::UpdateAndQueueCompletion.
 * @recoil-match byte
 *
 * Purpose: advance one fade entry toward its target, apply the backend
 * volume/gain value, and queue completed entries for dispatch.
 */
int zSndFadeEntry::TickAndMaybeDispatch(float deltaTime)
{
    const float direction = (targetValue - currentValue) < 0.0 ? -1.0f : 1.0f;
    const float step = direction * deltaTime * 2500.0f;
    currentValue = currentValue + step;

    switch (g_zSnd_ActiveBackend) {
    case ZSND_AUDIO_API_DIRECTSOUND: {
        if (currentValue > 0.0f) {
            currentValue = 0.0f;
        } else if (currentValue < -10000.0f) {
            currentValue = -10000.0f;
        }

        LPDIRECTSOUNDBUFFER const buffer = (LPDIRECTSOUNDBUFFER)(handle->backendBuffer);
        buffer->SetVolume((int)(currentValue));
        break;
    }
    case ZSND_AUDIO_API_A3D: {
        if (currentValue > 1.0) {
            currentValue = 1.0f;
        } else if (currentValue < 0.0) {
            currentValue = 0.0f;
        }

        ((zA3dProviderSource*)(handle->backendBuffer))->SetGain(zSndGainScaleIdentity(currentValue));
        break;
    }
    }

    if (currentValue == targetValue) {
        if (stopOnComplete != 0) {
            zSndPlayHandleStopIfActive(handle);
        }

        g_zSndFadeDispatchList.push_back(this);
        return 1;
    }
    return 0;
}

namespace {
/**
 * Original-source helper: std::remove_if predicate that ticks one active fade
 * and reports completion; expanded inline into zSndFadeActiveListTickAll.
 */
struct zSndFadeTickPredicate {
    float deltaTime;

    zSndFadeTickPredicate(float elapsed)
        : deltaTime(elapsed)
    {
    }

    int operator()(zSndFadeEntry* fadeEntry) const
    {
        return fadeEntry->TickAndMaybeDispatch(deltaTime);
    }
};
} // namespace

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-fade.zsndfadeactivelist-tickall
 * @recoil-artifact defines .text recoil:function:0x4a3c20: zSndFadeActiveList::TickAll.
 * @recoil-match byte
 *
 * Purpose: tick active fades, compact unfinished entries, and delete completed
 * fade-list nodes.
 */
extern "C" void __stdcall zSndFadeActiveListTickAll(float deltaTime)
{
    if (g_zSndFadeActiveList.empty()) {
        return;
    }

    const std::list<zSndFadeEntry*>::iterator fadeEnd = g_zSndFadeActiveList.end();
    g_zSndFadeActiveList.erase(
        std::remove_if(g_zSndFadeActiveList.begin(), fadeEnd, zSndFadeTickPredicate(deltaTime)),
        fadeEnd
    );
}

/*
 * These definitions remain beside the sound-system initialization sequence.
 * This translation unit retains the fade-list implementation.
 */

namespace {
/**
 * Original-source helper: std::for_each functor that stops one active fade's
 * playback handle and queues the entry for dispatch; expanded inline into
 * zSndFadeLists::StopAllAndShutdown.
 */
struct zSndFadeStopAndQueue {
    void operator()(zSndFadeEntry* fadeEntry) const
    {
        zSndPlayHandleStopIfActive(fadeEntry->handle);
        zSndFadeDispatchList::PushBack(fadeEntry);
    }
};

/**
 * Original-source helper: std::transform functor that deletes one fade entry
 * and returns null; expanded inline into zSndFadeLists::StopAllAndShutdown.
 */
struct zSndFadeEntryDelete {
    zSndFadeEntry* operator()(zSndFadeEntry* fadeEntry) const
    {
        ::operator delete(fadeEntry);
        return 0;
    }
};
} // namespace

namespace zSndFadeLists {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-fade.stopallandshutdown
 * @recoil-artifact defines .text recoil:function:0x4a3d20: zSndFadeLists::StopAllAndShutdown.
 * @recoil-match byte
 *
 * Purpose: stop active fade handles and drain both recovered fade lists during
 * sound-system shutdown.
 */
void __cdecl StopAllAndShutdown()
{
    std::for_each(g_zSndFadeActiveList.begin(), g_zSndFadeActiveList.end(), zSndFadeStopAndQueue());
    g_zSndFadeActiveList.clear();

    std::transform(
        g_zSndFadeDispatchList.begin(),
        g_zSndFadeDispatchList.end(),
        g_zSndFadeDispatchList.begin(),
        zSndFadeEntryDelete()
    );
    g_zSndFadeDispatchList.clear();
}
} // namespace zSndFadeLists
