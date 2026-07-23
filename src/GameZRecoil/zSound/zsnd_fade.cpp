#include "zsnd.h"

#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd_a3d_provider.h"

#include <list>
#include <stdlib.h>
#include <string.h>

/*
 * Retail zsnd_play.cpp physical-contribution routing anchor. This
 * address-backed body now compiles from zsnd_play.cpp; the anchor preserves
 * narrow legacy target provenance without compiling a duplicate definition
 * in this TU.
 * Reimplements 0x49f620: zSnd::Tick.
 */

/**
 * Reimplements data 0x56b368: g_zSnd_ConfigRootNode.
 * Data owner: namespace:zSound system configuration state.
 * Purpose: hold the loaded sound configuration tree until sound shutdown.
 */
extern "C" zReader::Node *g_zSnd_ConfigRootNode = 0;
/**
 * Reimplements data 0x56b364: g_zSnd_SearchPathList.
 * Data owner: namespace:zSound system configuration state.
 * Purpose: hold the sound resource search path list built from SOUND_PATH.
 */
extern "C" zArchiveList *g_zSnd_SearchPathList = 0;
/**
 * Data owner: namespace:zSound backend runtime state.
 * Purpose: reference the active A3D or DirectSound backend device.
 */
extern "C" void *g_zSnd_BackendDevice;

namespace {
std::list<zSndFadeEntry *> g_zSndFadeActiveList;
std::list<zSndFadeEntry *> g_zSndFadeDispatchList;
} // namespace

/*
 * Reimplements 0x4a3930: compiler-generated static initialization coordinator.
 * Reimplements 0x4a3940: compiler-generated constructors for both fade lists.
 * Reimplements 0x4a39a0: compiler-generated atexit registration helper.
 * Reimplements 0x4a39b0: compiler-generated destructors for both fade lists.
 * These four contributions arise naturally from the two namespace-scope
 * std::list objects above; they are not authored wrapper functions.
 */

namespace zSndFadeDispatchList {
/**
 * Reimplements 0x4a3a80: zSndFadeDispatchList::PushBack.
 * Purpose: append a completed fade entry to the dispatch list for completion
 * handling.
 */
void __fastcall PushBack(
    zSndFadeEntry *fadeEntry
) {
    g_zSndFadeDispatchList.push_back(fadeEntry);
}
} // namespace zSndFadeDispatchList

/**
 * Reimplements 0x4a3ad0: zSndFadeEntry::UpdateAndQueueCompletion.
 * Purpose: advance one fade entry toward its target, apply the backend
 * volume/gain value, and queue completed entries for dispatch.
 */
int zSndFadeEntry::TickAndMaybeDispatch(
    float deltaTime
) {
    const float direction = (targetValue - currentValue) < 0.0 ? -1.0f : 1.0f;
    const float step = direction * deltaTime * 2500.0f;
    currentValue = currentValue + step;

    switch (g_zSnd_ActiveBackend) {
    case 0: {
        if (currentValue > 0.0f) {
            currentValue = 0.0f;
        } else if (currentValue < -10000.0f) {
            currentValue = -10000.0f;
        }

        LPDIRECTSOUNDBUFFER const buffer = (LPDIRECTSOUNDBUFFER)(handle->backendBuffer);
        buffer->SetVolume((int)(currentValue));
        break;
    }
    case 1: {
        if (currentValue > 1.0) {
            currentValue = 1.0f;
        } else if (currentValue < 0.0) {
            currentValue = 0.0f;
        }

        ((zA3dProviderSource *)(handle->backendBuffer))->SetGain(
            zSndSample_PlaySimple(currentValue)
        );
        break;
    }
    }

    if (currentValue == targetValue) {
        if (stopOnComplete != 0) {
            handle->StopIfActive();
        }

        zSndFadeDispatchList::PushBack(this);
        return 1;
    }
    return 0;
}

/**
 * Reimplements 0x4a3c20: zSndFadeActiveList::TickAll.
 * Purpose: tick active fades, compact unfinished entries, and delete completed
 * fade-list nodes.
 */
extern "C" void __stdcall zSndFadeActiveList_TickAll(
    float deltaTime
) {
    std::list<zSndFadeEntry *>::iterator fadeIt =
        g_zSndFadeActiveList.begin();
    while (fadeIt != g_zSndFadeActiveList.end()) {
        if ((*fadeIt)->TickAndMaybeDispatch(deltaTime) != 0) {
            fadeIt = g_zSndFadeActiveList.erase(fadeIt);
        } else {
            ++fadeIt;
        }
    }
}

/*
 * Reimplements 0x4a13d0: the body compiles from zsnd_init.cpp.
 * Reimplements 0x4a1510: the body compiles from zsnd_init.cpp.
 * Reimplements 0x4a1870: the body compiles from zsnd_init.cpp.
 * These definitions remain beside the sound-system initialization sequence.
 * This translation unit retains the fade-list implementation.
 */

namespace zSndFadeLists {
/**
 * Reimplements 0x4a3d20: zSndFadeLists::StopAllAndShutdown.
 * Purpose: stop active fade handles and drain both recovered fade lists during
 * sound-system shutdown.
 */
void StopAllAndShutdown() {
    std::list<zSndFadeEntry *>::iterator fadeIt =
        g_zSndFadeActiveList.begin();
    while (fadeIt != g_zSndFadeActiveList.end()) {
        zSndFadeEntry *const fadeEntry = *fadeIt;
        fadeEntry->handle->StopIfActive();
        zSndFadeDispatchList::PushBack(fadeEntry);
        ++fadeIt;
    }
    g_zSndFadeActiveList.clear();

    fadeIt = g_zSndFadeDispatchList.begin();
    while (fadeIt != g_zSndFadeDispatchList.end()) {
        ::operator delete(*fadeIt);
        *fadeIt = 0;
        ++fadeIt;
    }
    g_zSndFadeDispatchList.clear();
}
} // namespace zSndFadeLists

/**
 * Reimplements 0x4a3e50: zSndFadeList::DeleteNodeAndAdvanceCursor.
 * Purpose: remove the current fade-list node, release its storage, and advance
 * the caller's cursor to the next node.
 */
void zSndFadeList::DeleteNodeAndAdvanceCursor(
    zSndFadeListNode **outCursor,
    zSndFadeListNode *node
) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    zSndFadeListNode *const outNext = node->next;
    ::operator delete(node);
    --count;
    *outCursor = outNext;
}

/**
 * Reimplements 0x4a3e90: zSndFadeListCursor::PopFrontCursor.
 * Purpose: return the current cursor node and advance the cursor to the next
 * intrusive-list node.
 */
zSndFadeListNode ** zSndFadeListCursor::PopFrontCursor(
    zSndFadeListNode **outNode,
    int unused
) {
    (void)unused;

    zSndFadeListNode *const current = node;
    node = current->next;
    *outNode = current;
    return outNode;
}
