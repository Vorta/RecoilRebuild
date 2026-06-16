#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zReader/zReader.h"

#include <cstdlib>
#include <cstring>

namespace {
int g_closeCalls;
int g_releaseCalls;
int g_getCapsCalls;
DWORD g_getCapsFlags;
HRESULT g_getCapsResult;
DWORD g_getCapsReturnedFlags;
int g_openCalls;
zNetworkDPlaySessionDesc *g_openDescPtr;
DWORD g_openFlags;
HRESULT g_openResult;

HRESULT __stdcall FakeDirectPlayClose(
    zNetwork_DPlay4 *
) {
    ++g_closeCalls;
    return 0;
}

ULONG __stdcall FakeDirectPlayRelease(
    zNetwork_DPlay4 *
) {
    ++g_releaseCalls;
    return 17;
}

HRESULT __stdcall FakeDirectPlayGetCaps(
    zNetwork_DPlay4 *,
    zNetworkDPlayCaps *caps,
    DWORD flags
) {
    ++g_getCapsCalls;
    g_getCapsFlags = flags;
    if (caps != 0) {
        caps->dwFlags = g_getCapsReturnedFlags;
    }
    return g_getCapsResult;
}

HRESULT __stdcall FakeDirectPlayOpen(
    zNetwork_DPlay4 *,
    zNetworkDPlaySessionDesc *sessionDesc,
    DWORD flags
) {
    ++g_openCalls;
    g_openDescPtr = sessionDesc;
    g_openFlags = flags;
    return g_openResult;
}

int __fastcall TestPacketHandlerA(
    int,
    zNetworkPacketHeader *
) {
    return 0;
}

int __fastcall TestPacketHandlerB(
    int,
    zNetworkPacketHeader *
) {
    return 0;
}

void ResetDirectPlayScenarioState() {
    g_closeCalls = 0;
    g_releaseCalls = 0;
    g_getCapsCalls = 0;
    g_getCapsFlags = 0;
    g_getCapsResult = 0;
    g_getCapsReturnedFlags = 0;
    g_openCalls = 0;
    g_openDescPtr = 0;
    g_openFlags = 0;
    g_openResult = 0;
    g_zNetwork_pDirectPlay4 = 0;
    g_zNetwork_ActiveProviderIsTcpIp = 0;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_zNetwork_DPlayCaps = zNetworkDPlayCaps();
    g_zNetwork_AppGuid = 0;
    g_zNetwork_CurrentSessionDescCache = 0;
    memset(
        g_zNetwork_SessionNameCache,
        0,
        sizeof(g_zNetwork_SessionNameCache)
    );
}

void BuildMinimalDirectPlayVtable(
    void **vtable
) {
    memset(
        vtable,
        0,
        sizeof(void *) * 25
    );
    vtable[2] = (void *)(&FakeDirectPlayRelease);
    vtable[4] = (void *)(&FakeDirectPlayClose);
    vtable[14] = (void *)(&FakeDirectPlayGetCaps);
    vtable[24] = (void *)(&FakeDirectPlayOpen);
}
} // namespace

extern "C" int znetwork_local_identity_smoke(void) {
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldIsHostFlag = g_zNetwork_IsHostFlag;
    zNetwork_PlayerRecord *const oldLocalPlayerRecord = g_zNetwork_LocalPlayerRecord;

    g_zNetwork_LocalPlayerKey = 0x12345678;
    g_zNetwork_IsHostFlag = 1;
    g_zNetwork_LocalPlayerRecord = 0;

    int result = 0;
    if (zNetwork_GetLocalPlayerKey() != 0x12345678 ||
        zNetwork::IsHost() != 1 ||
        zNetwork_GetLocalPlayerColorIndex() != 0) {
        result = 1;
    }

    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.colorIndex = 7;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    if (result == 0 && zNetwork_GetLocalPlayerColorIndex() != 7) {
        result = 2;
    }

    g_zNetwork_IsHostFlag = 0;
    g_zNetwork_LocalPlayerRecord = 0;
    if (result == 0 && zNetwork::IsHost() != 0) {
        result = 3;
    }

    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHostFlag;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayerRecord;
    return result;
}

extern "C" int znetwork_dplay_close_release_smoke(void) {
    g_closeCalls = 0;
    g_releaseCalls = 0;

    if (zNetwork_DPlay::CloseReleaseAndCoUninitialize(0) != 0) {
        return 1;
    }

    void *vtable[5] = {};
    vtable[2] = (void *)(&FakeDirectPlayRelease);
    vtable[4] = (void *)(&FakeDirectPlayClose);

    struct FakeDirectPlay {
        void **vtable;
    } fake = {vtable};

    const int result = zNetwork_DPlay::CloseReleaseAndCoUninitialize(
        (zNetwork_DPlay4 *)(&fake)
    );

    if (result != 17) {
        return 2;
    }

    if (g_closeCalls != 1 || g_releaseCalls != 1) {
        return 3;
    }

    return 0;
}

extern "C" int znetwork_dplay_report_error_smoke(void) {
    if (zNetwork_DPlay_ReportError(0, __FILE__, __LINE__) != 1) {
        return 1;
    }

    return zNetwork_DPlay_ReportError(
               (int)(0x88770014),
               __FILE__,
               __LINE__
           ) == 0
               ? 0
               : 2;
}

extern "C" int znetwork_dplay_query_caps_configure_send_mode_smoke(void) {
    void *vtable[25];
    BuildMinimalDirectPlayVtable(vtable);
    struct FakeDirectPlay {
        void **vtable;
    } fake = {vtable};

    ResetDirectPlayScenarioState();
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&fake);
    if (zNetworkDPlay::QueryCapsAndConfigureSendMode() != 1 ||
        g_getCapsCalls != 1 ||
        g_getCapsFlags != 1 ||
        g_zNetwork_DPlayCaps.dwSize != sizeof(zNetworkDPlayCaps)) {
        return 1;
    }

    ResetDirectPlayScenarioState();
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&fake);
    g_zNetwork_ActiveProviderIsTcpIp = 1;
    g_zNetwork_TcpIpAsyncSendEnabled = 1;
    g_getCapsReturnedFlags = 0x40;
    if (zNetworkDPlay::QueryCapsAndConfigureSendMode() != 1 ||
        g_zNetwork_TcpIpAsyncSendEnabled != 0) {
        return 2;
    }

    ResetDirectPlayScenarioState();
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&fake);
    g_zNetwork_ActiveProviderIsTcpIp = 1;
    g_zNetwork_TcpIpAsyncSendEnabled = 1;
    g_getCapsReturnedFlags = 0x10040;
    if (zNetworkDPlay::QueryCapsAndConfigureSendMode() != 1 ||
        g_zNetwork_TcpIpAsyncSendEnabled != 1) {
        return 3;
    }

    ResetDirectPlayScenarioState();
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&fake);
    g_zNetwork_ActiveProviderIsTcpIp = 1;
    g_getCapsReturnedFlags = 0;
    if (zNetworkDPlay::QueryCapsAndConfigureSendMode() != 0) {
        return 4;
    }

    ResetDirectPlayScenarioState();
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&fake);
    g_getCapsResult = -1;
    return zNetworkDPlay::QueryCapsAndConfigureSendMode() == 0 ? 0 : 5;
}

extern "C" int znetwork_dplay_create_session_from_status_fields_smoke(void) {
    void *vtable[25];
    BuildMinimalDirectPlayVtable(vtable);
    struct FakeDirectPlay {
        void **vtable;
    } fake = {vtable};

    GUID appGuid = {
        0x76543210,
        0xba98,
        0xfedc,
        {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef}
    };

    ResetDirectPlayScenarioState();
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&fake);
    g_zNetwork_AppGuid = &appGuid;

    zNetworkDPlaySessionDescCache *const oldCache =
        (zNetworkDPlaySessionDescCache *)(malloc(sizeof(zNetworkDPlaySessionDescCache)));
    memset(
        oldCache,
        0,
        sizeof(zNetworkDPlaySessionDescCache)
    );
    g_zNetwork_CurrentSessionDescCache = oldCache;

    zNetworkSessionDescStatusFields fields = {};
    fields.eventCode = 3;
    fields.statusFlags = 5;
    fields.valueOrTime = 7;
    fields.auxParam = 11;
    fields.maxPlayers = 8;
    strcpy(
        fields.sessionNameBuf,
        "Arena"
    );

    g_getCapsReturnedFlags = 0x10040;
    const int success = zNetwork_DPlay::CreateSessionFromStatusFields(&fields);
    zNetworkDPlaySessionDescCache *const cache = g_zNetwork_CurrentSessionDescCache;
    const int successOk =
        success == 1 &&
        cache != 0 &&
        cache != oldCache &&
        g_openCalls == 1 &&
        g_openDescPtr == &cache->desc &&
        g_openFlags == 2 &&
        strcmp(g_zNetwork_SessionNameCache, "Arena") == 0 &&
        cache->desc.dwSize == sizeof(zNetworkDPlaySessionDesc) &&
        cache->desc.dwFlags == 0x44 &&
        memcmp(
            &cache->desc.guidApplication,
            &appGuid,
            sizeof(cache->desc.guidApplication)
        ) == 0 &&
        cache->desc.dwMaxPlayers == 8 &&
        cache->desc.dwUser1 == 3 &&
        cache->desc.dwUser2 == 5 &&
        cache->desc.dwUser3 == 7 &&
        cache->desc.dwUser4 == 11 &&
        strcmp(cache->desc.lpszSessionNameA, "Arena") == 0;

    free(cache->desc.lpszSessionNameA);
    free(cache);
    g_zNetwork_CurrentSessionDescCache = 0;

    ResetDirectPlayScenarioState();
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&fake);
    g_zNetwork_AppGuid = &appGuid;
    g_openResult = (HRESULT)(0x88770118);
    strcpy(
        fields.sessionNameBuf,
        "Cancel"
    );
    const int cancelResult = zNetwork_DPlay::CreateSessionFromStatusFields(&fields);
    zNetworkDPlaySessionDescCache *leakedCache =
        (zNetworkDPlaySessionDescCache *)(
            (unsigned char *)(g_openDescPtr) -
            offsetof(zNetworkDPlaySessionDescCache, desc)
        );
    free(g_openDescPtr->lpszSessionNameA);
    free(leakedCache);

    ResetDirectPlayScenarioState();
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&fake);
    g_zNetwork_AppGuid = &appGuid;
    g_zNetwork_ActiveProviderIsTcpIp = 1;
    g_getCapsReturnedFlags = 0;
    g_openResult = 0;
    const int capsRejected = zNetwork_DPlay::CreateSessionFromStatusFields(&fields);
    leakedCache =
        (zNetworkDPlaySessionDescCache *)(
            (unsigned char *)(g_openDescPtr) -
            offsetof(zNetworkDPlaySessionDescCache, desc)
        );
    free(g_openDescPtr->lpszSessionNameA);
    free(leakedCache);

    if (!successOk) {
        return 2;
    }
    if (cancelResult != 0) {
        return 3;
    }
    if (capsRejected != 0) {
        return 4;
    }
    if (g_closeCalls != 1) {
        return 5;
    }
    return 0;
}

extern "C" int znetwork_unregister_packet_handler_smoke(void) {
    zNetworkDispatchHandlerListNode sentinel = {};
    zNetworkDispatchHandlerListNode first = {};
    zNetworkDispatchHandlerListNode *second =
        (zNetworkDispatchHandlerListNode *)(::operator new(sizeof(zNetworkDispatchHandlerListNode)));
    zNetworkDispatchHandlerListNode *third =
        (zNetworkDispatchHandlerListNode *)(::operator new(sizeof(zNetworkDispatchHandlerListNode)));

    zNetworkDispatchHandlerRecord firstRecord = {};
    zNetworkDispatchHandlerRecord secondRecord = {};
    zNetworkDispatchHandlerRecord thirdRecord = {};
    firstRecord.packetType = 7;
    firstRecord.handler = TestPacketHandlerA;
    secondRecord.packetType = 8;
    secondRecord.handler = TestPacketHandlerB;
    thirdRecord.packetType = 7;
    thirdRecord.handler = TestPacketHandlerA;

    sentinel.next = &first;
    sentinel.prev = third;
    first.next = second;
    first.prev = &sentinel;
    first.record = &firstRecord;
    second->next = third;
    second->prev = &first;
    second->record = &secondRecord;
    third->next = &sentinel;
    third->prev = second;
    third->record = &thirdRecord;

    zNetworkDispatchHandlerListNode *const oldSentinel =
        g_zNetwork_DispatchHandlerListSentinel;
    const int oldCount = g_zNetwork_DispatchHandlerListCount;
    g_zNetwork_DispatchHandlerListSentinel = &sentinel;
    g_zNetwork_DispatchHandlerListCount = 3;

    const int result = zNetwork::UnregisterPacketHandler(
        7,
        TestPacketHandlerA
    );

    const int ok = result == 1 &&
                   g_zNetwork_DispatchHandlerListCount == 1 &&
                   sentinel.next == &first &&
                   sentinel.prev == &first &&
                   first.next == &sentinel &&
                   first.prev == &sentinel &&
                   first.record == &secondRecord;

    g_zNetwork_DispatchHandlerListSentinel = oldSentinel;
    g_zNetwork_DispatchHandlerListCount = oldCount;
    return ok ? 0 : 1;
}

extern "C" int znetwork_clear_enumerated_session_list_smoke(void) {
    zArchiveList list = {};
    zArchiveListNode *const first =
        (zArchiveListNode *)(std::malloc(sizeof(zArchiveListNode)));
    zArchiveListNode *const second =
        (zArchiveListNode *)(std::malloc(sizeof(zArchiveListNode)));
    zNetworkDPlaySessionDesc *const firstDesc =
        (zNetworkDPlaySessionDesc *)(std::malloc(sizeof(zNetworkDPlaySessionDesc)));
    zNetworkDPlaySessionDesc *const secondDesc =
        (zNetworkDPlaySessionDesc *)(std::malloc(sizeof(zNetworkDPlaySessionDesc)));
    std::memset(first, 0, sizeof(zArchiveListNode));
    std::memset(second, 0, sizeof(zArchiveListNode));
    std::memset(firstDesc, 0, sizeof(zNetworkDPlaySessionDesc));
    std::memset(secondDesc, 0, sizeof(zNetworkDPlaySessionDesc));
    firstDesc->dwReserved1 = (DWORD)(std::malloc(4));

    first->payload = firstDesc;
    first->next = second;
    first->prev = second;
    second->payload = secondDesc;
    second->next = first;
    second->prev = first;
    list.count = 2;
    list.head = first;

    zArchiveList *const oldEnumeratedList = g_zNetwork_EnumeratedSessionList;
    zArchiveList *const oldFreePool = g_zUtil_ZRDR_FreePool;
    const int oldFreeCount = g_zUtil_ZRDR_FreeCount;
    const int oldGrowCount = g_zUtil_ZRDR_GrowCount;
    const int oldTotalAllocated = g_zUtil_ZRDR_TotalAllocated;
    g_zNetwork_EnumeratedSessionList = &list;
    g_zUtil_ZRDR_FreePool = 0;
    g_zUtil_ZRDR_FreeCount = 0;
    g_zUtil_ZRDR_GrowCount = 0;
    g_zUtil_ZRDR_TotalAllocated = 0;

    zNetwork::ClearEnumeratedSessionList();
    const int ok = list.count == 0 && list.head == 0;

    zUtil_ZRDR_FreeNodePool();
    g_zNetwork_EnumeratedSessionList = oldEnumeratedList;
    g_zUtil_ZRDR_FreePool = oldFreePool;
    g_zUtil_ZRDR_FreeCount = oldFreeCount;
    g_zUtil_ZRDR_GrowCount = oldGrowCount;
    g_zUtil_ZRDR_TotalAllocated = oldTotalAllocated;
    return ok ? 0 : 1;
}

extern "C" int znetwork_clear_service_provider_list_smoke(void) {
    zNetworkDPlayServiceProviderInfo **const slots =
        (zNetworkDPlayServiceProviderInfo **)(::operator new(
            sizeof(zNetworkDPlayServiceProviderInfo *) * 2
        ));
    zNetworkDPlayServiceProviderInfo *const info =
        (zNetworkDPlayServiceProviderInfo *)(::operator new(
            sizeof(zNetworkDPlayServiceProviderInfo)
        ));
    std::memset(info, 0, sizeof(zNetworkDPlayServiceProviderInfo));
    info->displayName = (char *)(std::malloc(8));
    info->connectionData = std::malloc(8);
    slots[0] = info;
    slots[1] = 0;

    zNetworkServiceProviderListVec list = {};
    list.begin = slots;
    list.end = slots + 2;
    list.cap = slots + 2;

    zNetworkServiceProviderListVec *const oldList = g_zNetwork_ServiceProviderList;
    g_zNetwork_ServiceProviderList = &list;

    zNetwork::ClearServiceProviderList();
    const int ok = list.end == list.begin && slots[0] == 0 && slots[1] == 0;

    ::operator delete(slots);
    g_zNetwork_ServiceProviderList = oldList;
    return ok ? 0 : 1;
}

extern "C" int znetwork_clear_player_record_list_smoke(void) {
    zNetworkPlayerRecordListNode sentinel = {};
    zNetworkPlayerRecordListNode *const first =
        (zNetworkPlayerRecordListNode *)(::operator new(sizeof(zNetworkPlayerRecordListNode)));
    zNetworkPlayerRecordListNode *const second =
        (zNetworkPlayerRecordListNode *)(::operator new(sizeof(zNetworkPlayerRecordListNode)));
    zNetwork_PlayerRecord *const firstRecord =
        (zNetwork_PlayerRecord *)(::operator new(sizeof(zNetwork_PlayerRecord)));
    zNetwork_PlayerRecord *const secondRecord =
        (zNetwork_PlayerRecord *)(::operator new(sizeof(zNetwork_PlayerRecord)));
    std::memset(first, 0, sizeof(zNetworkPlayerRecordListNode));
    std::memset(second, 0, sizeof(zNetworkPlayerRecordListNode));
    std::memset(firstRecord, 0, sizeof(zNetwork_PlayerRecord));
    std::memset(secondRecord, 0, sizeof(zNetwork_PlayerRecord));

    sentinel.next = first;
    sentinel.prev = second;
    first->next = second;
    first->prev = &sentinel;
    first->playerRecord = firstRecord;
    second->next = &sentinel;
    second->prev = first;
    second->playerRecord = secondRecord;

    zNetworkPlayerRecordList list = {};
    list.sentinelNode = &sentinel;
    list.count = 2;

    zNetworkPlayerRecordList *const oldList = g_zNetwork_PlayerRecordList;
    g_zNetwork_PlayerRecordList = &list;

    zNetwork::ClearPlayerRecordList();
    const int ok = list.count == 0 &&
                   sentinel.next == &sentinel &&
                   sentinel.prev == &sentinel;

    g_zNetwork_PlayerRecordList = oldList;
    return ok ? 0 : 1;
}

extern "C" int znetwork_player_record_accessors_smoke(void) {
    zNetworkPlayerRecordListNode sentinel = {};
    zNetworkPlayerRecordListNode node = {};
    zNetwork_PlayerRecord record = {};
    record.playerKey = 0x12345678;
    record.colorIndex = 5;

    sentinel.next = &node;
    sentinel.prev = &node;
    node.next = &sentinel;
    node.prev = &sentinel;
    node.playerRecord = &record;

    zNetworkPlayerRecordList list = {};
    list.sentinelNode = &sentinel;
    list.count = 1;

    zNetworkPlayerRecordList *const oldList = g_zNetwork_PlayerRecordList;
    g_zNetwork_PlayerRecordList = &list;

    const int ok = zNetwork_FindPlayerRecordByKey(0x12345678) == &record &&
                   zNetwork_FindPlayerRecordByKey(0x87654321) == 0;

    g_zNetwork_PlayerRecordList = oldList;
    return ok ? 0 : 1;
}

extern "C" int znetwork_apply_pkt01_player_color_assignments_smoke(void) {
    zNetworkPlayerRecordListNode sentinel = {};
    zNetworkPlayerRecordListNode node = {};
    zNetwork_PlayerRecord record = {};
    record.playerKey = 0x12345678;
    record.colorIndex = 2;

    sentinel.next = &node;
    sentinel.prev = &node;
    node.next = &sentinel;
    node.prev = &sentinel;
    node.playerRecord = &record;

    zNetworkPlayerRecordList list = {};
    list.sentinelNode = &sentinel;
    list.count = 1;

    NetPkt01_PlayerColorAssignments packet = {};
    packet.header.packetType = 1;
    packet.header.packetSizeBytes = sizeof(packet);
    packet.pairCount = 1;
    packet.pairs[0].playerKey = 0x12345678;
    packet.pairs[0].colorIndex = 6;

    zNetworkPlayerRecordList *const oldList = g_zNetwork_PlayerRecordList;
    const int oldColorFlag = g_zNetwork_PlayerColorInUseFlags[6];
    g_zNetwork_PlayerRecordList = &list;
    g_zNetwork_PlayerColorInUseFlags[6] = 0;

    const int result = zNetwork_ApplyPkt01_PlayerColorAssignments(
        0,
        &packet.header
    );
    const int ok = result == 1 &&
                   record.colorIndex == 6 &&
                   g_zNetwork_PlayerColorInUseFlags[6] == 1;

    g_zNetwork_PlayerRecordList = oldList;
    g_zNetwork_PlayerColorInUseFlags[6] = oldColorFlag;
    return ok ? 0 : 1;
}

extern "C" int znetwork_shutdown_session_runtime_smoke(void) {
    void *vtable[5] = {};
    vtable[2] = (void *)(&FakeDirectPlayRelease);
    vtable[4] = (void *)(&FakeDirectPlayClose);
    struct FakeDirectPlay {
        void **vtable;
    } fakeDirectPlay = {vtable};

    zNetworkDispatchHandlerRecord handlerRecord = {};
    handlerRecord.packetType = 1;
    handlerRecord.handler = zNetwork_ApplyPkt01_PlayerColorAssignments;
    zNetworkDispatchHandlerListNode dispatchSentinel = {};
    zNetworkDispatchHandlerListNode *const dispatchNode =
        (zNetworkDispatchHandlerListNode *)(::operator new(
            sizeof(zNetworkDispatchHandlerListNode)
        ));
    dispatchSentinel.next = dispatchNode;
    dispatchSentinel.prev = dispatchNode;
    dispatchNode->next = &dispatchSentinel;
    dispatchNode->prev = &dispatchSentinel;
    dispatchNode->record = &handlerRecord;

    zArchiveList *const enumeratedList =
        (zArchiveList *)(std::malloc(sizeof(zArchiveList)));
    zArchiveListNode *const sessionNode =
        (zArchiveListNode *)(std::malloc(sizeof(zArchiveListNode)));
    zNetworkDPlaySessionDesc *const sessionDesc =
        (zNetworkDPlaySessionDesc *)(std::malloc(sizeof(zNetworkDPlaySessionDesc)));
    std::memset(enumeratedList, 0, sizeof(zArchiveList));
    std::memset(sessionNode, 0, sizeof(zArchiveListNode));
    std::memset(sessionDesc, 0, sizeof(zNetworkDPlaySessionDesc));
    sessionDesc->dwReserved1 = (DWORD)(std::malloc(4));
    sessionNode->payload = sessionDesc;
    sessionNode->next = sessionNode;
    sessionNode->prev = sessionNode;
    enumeratedList->count = 1;
    enumeratedList->head = sessionNode;

    zNetworkServiceProviderListVec *const providerList =
        (zNetworkServiceProviderListVec *)(::operator new(
            sizeof(zNetworkServiceProviderListVec)
        ));
    zNetworkDPlayServiceProviderInfo **const providerSlots =
        (zNetworkDPlayServiceProviderInfo **)(::operator new(
            sizeof(zNetworkDPlayServiceProviderInfo *)
        ));
    zNetworkDPlayServiceProviderInfo *const provider =
        (zNetworkDPlayServiceProviderInfo *)(::operator new(
            sizeof(zNetworkDPlayServiceProviderInfo)
        ));
    std::memset(provider, 0, sizeof(zNetworkDPlayServiceProviderInfo));
    provider->displayName = (char *)(std::malloc(8));
    provider->connectionData = std::malloc(8);
    providerSlots[0] = provider;
    providerList->begin = providerSlots;
    providerList->end = providerSlots + 1;
    providerList->cap = providerSlots + 1;

    zNetworkPlayerRecordList *const playerList =
        (zNetworkPlayerRecordList *)(::operator new(sizeof(zNetworkPlayerRecordList)));
    zNetworkPlayerRecordListNode *const playerSentinel =
        (zNetworkPlayerRecordListNode *)(::operator new(
            sizeof(zNetworkPlayerRecordListNode)
        ));
    zNetworkPlayerRecordListNode *const playerNode =
        (zNetworkPlayerRecordListNode *)(::operator new(
            sizeof(zNetworkPlayerRecordListNode)
        ));
    zNetwork_PlayerRecord *const playerRecord =
        (zNetwork_PlayerRecord *)(::operator new(sizeof(zNetwork_PlayerRecord)));
    playerSentinel->next = playerNode;
    playerSentinel->prev = playerNode;
    playerNode->next = playerSentinel;
    playerNode->prev = playerSentinel;
    playerNode->playerRecord = playerRecord;
    playerList->sentinelNode = playerSentinel;
    playerList->count = 1;

    zNetwork_DPlay4 *const oldDirectPlay = g_zNetwork_pDirectPlay4;
    const int oldSessionRuntime = g_zNetwork_SessionRuntimeInitialized;
    zNetworkDispatchHandlerListNode *const oldDispatchSentinel =
        g_zNetwork_DispatchHandlerListSentinel;
    const int oldDispatchCount = g_zNetwork_DispatchHandlerListCount;
    zArchiveList *const oldEnumeratedList = g_zNetwork_EnumeratedSessionList;
    zNetworkDPlaySessionDescCache *const oldCurrentCache =
        g_zNetwork_CurrentSessionDescCache;
    zNetworkServiceProviderListVec *const oldProviderList =
        g_zNetwork_ServiceProviderList;
    zNetworkPlayerRecordList *const oldPlayerList = g_zNetwork_PlayerRecordList;
    void *const oldReceiveBuffer = g_zNetwork_ReceiveBuffer;
    zArchiveList *const oldFreePool = g_zUtil_ZRDR_FreePool;
    const int oldFreeCount = g_zUtil_ZRDR_FreeCount;
    const int oldGrowCount = g_zUtil_ZRDR_GrowCount;
    const int oldTotalAllocated = g_zUtil_ZRDR_TotalAllocated;

    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&fakeDirectPlay);
    g_zNetwork_SessionRuntimeInitialized = 1;
    g_zNetwork_DispatchHandlerListSentinel = &dispatchSentinel;
    g_zNetwork_DispatchHandlerListCount = 1;
    g_zNetwork_EnumeratedSessionList = enumeratedList;
    g_zNetwork_CurrentSessionDescCache = (zNetworkDPlaySessionDescCache *)(&fakeDirectPlay);
    g_zNetwork_ServiceProviderList = providerList;
    g_zNetwork_PlayerRecordList = playerList;
    g_zNetwork_ReceiveBuffer = std::malloc(8);
    g_zUtil_ZRDR_FreePool = 0;
    g_zUtil_ZRDR_FreeCount = 0;
    g_zUtil_ZRDR_GrowCount = 0;
    g_zUtil_ZRDR_TotalAllocated = 0;
    g_closeCalls = 0;
    g_releaseCalls = 0;

    const int result = zNetwork::ShutdownSessionRuntime();
    const int ok = result == 0 &&
                   g_closeCalls == 1 &&
                   g_releaseCalls == 1 &&
                   g_zNetwork_SessionRuntimeInitialized == 0 &&
                   g_zNetwork_DispatchHandlerListCount == 0 &&
                   g_zNetwork_EnumeratedSessionList == 0 &&
                   g_zNetwork_CurrentSessionDescCache == 0 &&
                   g_zNetwork_ServiceProviderList == 0 &&
                   g_zNetwork_PlayerRecordList == 0 &&
                   g_zNetwork_ReceiveBuffer == 0;

    zUtil_ZRDR_FreeNodePool();
    g_zNetwork_pDirectPlay4 = oldDirectPlay;
    g_zNetwork_SessionRuntimeInitialized = oldSessionRuntime;
    g_zNetwork_DispatchHandlerListSentinel = oldDispatchSentinel;
    g_zNetwork_DispatchHandlerListCount = oldDispatchCount;
    g_zNetwork_EnumeratedSessionList = oldEnumeratedList;
    g_zNetwork_CurrentSessionDescCache = oldCurrentCache;
    g_zNetwork_ServiceProviderList = oldProviderList;
    g_zNetwork_PlayerRecordList = oldPlayerList;
    g_zNetwork_ReceiveBuffer = oldReceiveBuffer;
    g_zUtil_ZRDR_FreePool = oldFreePool;
    g_zUtil_ZRDR_FreeCount = oldFreeCount;
    g_zUtil_ZRDR_GrowCount = oldGrowCount;
    g_zUtil_ZRDR_TotalAllocated = oldTotalAllocated;
    return ok ? 0 : 1;
}
