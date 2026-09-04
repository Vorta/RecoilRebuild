#include "zsnd.h"

#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd_a3d_provider.h"

#include <list>
#include <stdlib.h>
#include <string.h>

/**
 * Data owner: namespace:zSound system configuration state.
 * Purpose: hold the loaded sound configuration tree until sound shutdown.
 */
extern "C" zReader::Node *g_zSnd_ConfigRootNode = 0;
/**
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
 * compiler-generated static initialization coordinator.
 * compiler-generated constructors for both fade lists.
 * compiler-generated atexit registration helper.
 * compiler-generated destructors for both fade lists.
 * These four contributions arise naturally from the two namespace-scope
 * std::list objects above; they are not authored wrapper functions.
 */

namespace zSndFadeDispatchList {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-fade.pushback
 * @recoil-artifact defines .text recoil:function:0x4a3a80: zSndFadeDispatchList::PushBack.
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
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-fade.zsndfadeentry-tickandmaybedispatch
 * @recoil-artifact defines .text recoil:function:0x4a3ad0: zSndFadeEntry::UpdateAndQueueCompletion.
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

        g_zSndFadeDispatchList.push_back(this);
        return 1;
    }
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-fade.zsndfadeactivelist-tickall
 * @recoil-artifact defines .text recoil:function:0x4a3c20: zSndFadeActiveList::TickAll.
 * Purpose: tick active fades, compact unfinished entries, and delete completed
 * fade-list nodes.
 */
extern "C" void __stdcall zSndFadeActiveList_TickAll(
    float deltaTime
) {
    std::list<zSndFadeEntry *>::iterator compactIt =
        g_zSndFadeActiveList.begin();
    while (compactIt != g_zSndFadeActiveList.end()) {
        if ((*compactIt)->TickAndMaybeDispatch(deltaTime) != 0) {
            break;
        }
        ++compactIt;
    }

    if (compactIt == g_zSndFadeActiveList.end()) {
        return;
    }

    std::list<zSndFadeEntry *>::iterator fadeIt = compactIt;
    ++fadeIt;
    while (fadeIt != g_zSndFadeActiveList.end()) {
        if ((*fadeIt)->TickAndMaybeDispatch(deltaTime) == 0) {
            *compactIt = *fadeIt;
            ++compactIt;
        }
        ++fadeIt;
    }

    g_zSndFadeActiveList.erase(compactIt, g_zSndFadeActiveList.end());
}

/*
 * These definitions remain beside the sound-system initialization sequence.
 * This translation unit retains the fade-list implementation.
 */

namespace zSndFadeLists {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-fade.stopallandshutdown
 * @recoil-artifact defines .text recoil:function:0x4a3d20: zSndFadeLists::StopAllAndShutdown.
 * Purpose: stop active fade handles and drain both recovered fade lists during
 * sound-system shutdown.
 */
void __cdecl StopAllAndShutdown() {
    std::list<zSndFadeEntry *>::iterator fadeIt =
        g_zSndFadeActiveList.begin();
    while (fadeIt != g_zSndFadeActiveList.end()) {
        zSndFadeEntry *const fadeEntry = *fadeIt;
        fadeEntry->handle->StopIfActive();
        zSndFadeDispatchList::PushBack(fadeEntry);
        ++fadeIt;
    }
    zSndFadeList *const activeList =
        (zSndFadeList *)(&g_zSndFadeActiveList);
    zSndFadeListCursor activeCursor;
    activeCursor.node = activeList->sentinel->next;
    while (activeCursor.node != activeList->sentinel) {
        zSndFadeListNode *node;
        activeCursor.PopFrontCursor(&node, 0);
        activeList->DeleteNodeAndAdvanceCursor(&activeCursor.node, node);
    }

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
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-fade.zsndfadelist-deletenodeandadvancecursor
 * @recoil-artifact defines .text recoil:function:0x4a3e50: zSndFadeList::DeleteNodeAndAdvanceCursor.
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
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-fade.zsndfadelistcursor-popfrontcursor
 * @recoil-artifact defines .text recoil:function:0x4a3e90: zSndFadeListCursor::PopFrontCursor.
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
