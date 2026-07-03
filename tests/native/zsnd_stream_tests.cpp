#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd.h"

#include <cstdint>
#include <cstdlib>

namespace {
void FreeListStorage(
    zArchiveList *list
) {
    if (list == 0) {
        return;
    }

    while (list->count > 0) {
        zArchiveListNode *node = list->head;
        if (list->count == 1) {
            list->head = 0;
        } else {
            node->prev->next = node->next;
            node->next->prev = node->prev;
            list->head = node->next;
        }
        --list->count;
        std::free(node);
    }

    std::free(list);
}
} // namespace

extern "C" int zsnd_stream_request_stop_if_active_smoke(void) {
    zArchiveList *oldFreePool = g_zUtil_ZRDR_FreePool;
    zArchiveList *oldActiveList = g_zSndStream_ActiveList;
    const int oldFreeCount = g_zUtil_ZRDR_FreeCount;
    const int oldGrowCount = g_zUtil_ZRDR_GrowCount;
    const int oldTotalAllocated = g_zUtil_ZRDR_TotalAllocated;

    g_zUtil_ZRDR_FreePool = zArchiveList_CreateEmpty();
    g_zSndStream_ActiveList = zArchiveList_CreateEmpty();
    if (g_zUtil_ZRDR_FreePool == 0 || g_zSndStream_ActiveList == 0) {
        FreeListStorage(g_zSndStream_ActiveList);
        FreeListStorage(g_zUtil_ZRDR_FreePool);
        g_zSndStream_ActiveList = oldActiveList;
        g_zUtil_ZRDR_FreePool = oldFreePool;
        return 1;
    }

    zSndStreamRequest request = {};
    request.streamState = 1;
    zSndStreamRequest otherRequest = {};

    int result = 0;
    if (zArchiveList_PushFrontPayload(g_zSndStream_ActiveList, &request) != 1) {
        result = 2;
    } else if (zSndStreamRequest_StopIfActive(reinterpret_cast<zSndPlayHandle *>(&request)) != 1 ||
               request.streamState != 4) {
        result = 3;
    } else if (zSndStreamRequest_StopIfActive(reinterpret_cast<zSndPlayHandle *>(&otherRequest)) != 0 ||
               otherRequest.streamState != 0) {
        result = 4;
    }

    zArchiveList_Destroy(g_zSndStream_ActiveList);
    FreeListStorage(g_zUtil_ZRDR_FreePool);
    g_zSndStream_ActiveList = oldActiveList;
    g_zUtil_ZRDR_FreePool = oldFreePool;
    g_zUtil_ZRDR_FreeCount = oldFreeCount;
    g_zUtil_ZRDR_GrowCount = oldGrowCount;
    g_zUtil_ZRDR_TotalAllocated = oldTotalAllocated;

    return result;
}

extern "C" int zsnd_report_error_helpers_smoke(void) {
    const char *sourceFile = "zsnd_report_error_helpers_smoke";
    if (zSnd::ReportDirectSoundError(0, sourceFile, 1) != 1 ||
        zSnd::ReportA3DError(0, sourceFile, 2) != 1) {
        return 1;
    }

    if (zSnd::ReportDirectSoundError(static_cast<std::int32_t>(0x8878000a), sourceFile, 3) != 0 ||
        zSnd::ReportDirectSoundError(static_cast<std::int32_t>(0x12345678), sourceFile, 4) != 0) {
        return 2;
    }

    if (zSnd::ReportA3DError(static_cast<std::int32_t>(0x80040001), sourceFile, 5) != 0 ||
        zSnd::ReportA3DError(7, sourceFile, 6) != 0) {
        return 3;
    }

    return 0;
}
