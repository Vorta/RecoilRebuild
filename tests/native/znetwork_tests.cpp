#include "Battlesport/pickup.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zReader/zReader.h"

#include <cstdlib>
#include <cstring>
#include <new>

namespace {
int g_destroyPlayerCalls;
std::uint32_t g_destroyedPlayerKey;
std::int32_t g_destroyPlayerResult;
int g_sendCalls;
std::uint32_t g_sendFromPlayer;
std::uint32_t g_sendFlags;
void *g_sendPacket;
std::uint32_t g_sendPacketSize;
unsigned char g_sendPacketBytes[512];
std::int32_t g_sendResult;
int g_setSessionDescCalls;
zNetworkDPlaySessionDesc *g_setSessionDescPtr;
std::uint32_t g_setSessionDescFlags;
std::int32_t g_setSessionDescResult;
int g_sendExCalls;
std::uint32_t g_sendExFlags;
std::uint32_t g_sendExAsyncValue;
int g_cancelCalls;
std::uint32_t g_cancelHandle;
std::uint32_t g_cancelFlags;
int g_closeCalls;
int g_releaseCalls;

std::int32_t RECOIL_STDCALL CloseFake(zNetwork_DPlay4 *) {
    ++g_closeCalls;
    return 0;
}

std::int32_t RECOIL_STDCALL ReleaseFake(zNetwork_DPlay4 *) {
    ++g_releaseCalls;
    return 7;
}

std::int32_t RECOIL_STDCALL DestroyPlayerFake(zNetwork_DPlay4 *, std::uint32_t playerKey) {
    ++g_destroyPlayerCalls;
    g_destroyedPlayerKey = playerKey;
    return g_destroyPlayerResult;
}

std::int32_t RECOIL_STDCALL SendFake(zNetwork_DPlay4 *, std::uint32_t fromPlayer, std::uint32_t,
                                     std::uint32_t flags, void *packet,
                                     std::uint32_t packetSizeBytes) {
    ++g_sendCalls;
    g_sendFromPlayer = fromPlayer;
    g_sendFlags = flags;
    g_sendPacket = packet;
    g_sendPacketSize = packetSizeBytes;
    if (packet != nullptr && packetSizeBytes <= sizeof(g_sendPacketBytes)) {
        std::memcpy(g_sendPacketBytes, packet, packetSizeBytes);
    }
    return g_sendResult;
}

std::int32_t RECOIL_STDCALL SetSessionDescFake(zNetwork_DPlay4 *,
                                               zNetworkDPlaySessionDesc *sessionDesc,
                                               std::uint32_t flags) {
    ++g_setSessionDescCalls;
    g_setSessionDescPtr = sessionDesc;
    g_setSessionDescFlags = flags;
    return g_setSessionDescResult;
}

std::int32_t RECOIL_STDCALL SendExFake(zNetwork_DPlay4 *, std::uint32_t fromPlayer, std::uint32_t,
                                       std::uint32_t flags, void *packet,
                                       std::uint32_t packetSizeBytes, std::uint32_t, std::uint32_t,
                                       void *, std::uint32_t *asyncHandle) {
    ++g_sendExCalls;
    g_sendFromPlayer = fromPlayer;
    g_sendExFlags = flags;
    g_sendPacket = packet;
    g_sendPacketSize = packetSizeBytes;
    if (packet != nullptr && packetSizeBytes <= sizeof(g_sendPacketBytes)) {
        std::memcpy(g_sendPacketBytes, packet, packetSizeBytes);
    }
    if (asyncHandle != nullptr) {
        *asyncHandle = g_sendExAsyncValue;
    }

    return g_sendResult;
}

std::int32_t RECOIL_STDCALL CancelMessageFake(zNetwork_DPlay4 *, std::uint32_t asyncHandle,
                                              std::uint32_t flags) {
    ++g_cancelCalls;
    g_cancelHandle = asyncHandle;
    g_cancelFlags = flags;
    return 0;
}

const zNetwork_DPlay4Vtable kDPlayVtable = {
    {}, DestroyPlayerFake, {}, SendFake,          {}, SetSessionDescFake,
    {}, SendExFake,        {}, CancelMessageFake,
};

void ResetNetwork() {
    g_destroyPlayerCalls = 0;
    g_destroyedPlayerKey = 0;
    g_destroyPlayerResult = 0;
    g_sendCalls = 0;
    g_sendFromPlayer = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    g_sendResult = 0;
    g_setSessionDescCalls = 0;
    g_setSessionDescPtr = nullptr;
    g_setSessionDescFlags = 0;
    g_setSessionDescResult = 0;
    g_sendExCalls = 0;
    g_sendExFlags = 0;
    g_sendExAsyncValue = 0xabcdef01;
    g_cancelCalls = 0;
    g_cancelHandle = 0;
    g_cancelFlags = 0;
    g_closeCalls = 0;
    g_releaseCalls = 0;
    g_zNetwork_pDirectPlay4 = nullptr;
    g_zNetwork_LocalPlayerRecord = nullptr;
    g_zNetwork_IsHostFlag = 0;
    g_zNetwork_LocalPlayerKey = 0;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_zNetwork_LastSendExHandle = 0;
    g_zNetwork_LastSendExCompleted = 0;
    g_zNetwork_SessionRuntimeInitialized = 0;
    g_zNetwork_CurrentSessionDescCache = nullptr;
    g_zNetwork_EnumeratedSessionList = nullptr;
    g_zNetwork_ServiceProviderList = nullptr;
    g_zNetwork_PlayerRecordList = nullptr;
    g_zNetwork_ReceiveBuffer = nullptr;
    g_zNetwork_DispatchHandlerListSentinel = nullptr;
    g_zNetwork_DispatchHandlerListCount = 0;
    std::memset(g_zNetwork_PlayerColorInUseFlags, 0, sizeof(g_zNetwork_PlayerColorInUseFlags));
    std::memset(g_zNetwork_SessionNameCache, 0, sizeof(g_zNetwork_SessionNameCache));
}

template <typename T> T *AllocObject() {
    auto *object = static_cast<T *>(::operator new(sizeof(T)));
    std::memset(object, 0, sizeof(T));
    return object;
}
} // namespace

extern "C" int znetwork_local_identity_smoke() {
    ResetNetwork();
    g_zNetwork_LocalPlayerKey = 0x12345678;
    g_zNetwork_IsHostFlag = 1;

    if (zNetwork_GetLocalPlayerKey() != 0x12345678 || zNetwork::IsHost() != 1 ||
        zNetwork_GetLocalPlayerColorIndex() != 0) {
        return 1;
    }

    zNetwork_PlayerRecord localPlayer{};
    localPlayer.colorIndex = 7;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    if (zNetwork_GetLocalPlayerColorIndex() != 7) {
        return 2;
    }

    g_zNetwork_IsHostFlag = 0;
    g_zNetwork_LocalPlayerRecord = nullptr;
    return zNetwork::IsHost() == 0 ? 0 : 3;
}

extern "C" int znetwork_destroy_cached_local_player_smoke() {
    ResetNetwork();
    if (zNetwork_DPlay_DestroyCachedLocalPlayer() != 0) {
        return 1;
    }

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord player{0x12345678};
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &player;

    if (zNetwork_DPlay_DestroyCachedLocalPlayer() != 1 || g_destroyPlayerCalls != 1 ||
        g_destroyedPlayerKey != 0x12345678) {
        return 2;
    }

    g_destroyPlayerResult = static_cast<std::int32_t>(0x88770014);
    return zNetwork_DPlay_DestroyCachedLocalPlayer() == 0 ? 0 : 3;
}

extern "C" int znetwork_dplay_report_error_smoke() {
    if (zNetwork_DPlay_ReportError(0, __FILE__, __LINE__) != 1) {
        return 1;
    }

    return zNetwork_DPlay_ReportError(static_cast<std::int32_t>(0x88770014), __FILE__, __LINE__) ==
                   0
               ? 0
               : 2;
}

extern "C" int znetwork_dplay_close_release_smoke() {
    ResetNetwork();

    zNetwork_DPlay4Vtable vtable{};
    vtable.reserved_00[2] = reinterpret_cast<void *>(&ReleaseFake);
    vtable.reserved_00[4] = reinterpret_cast<void *>(&CloseFake);
    zNetwork_DPlay4 directPlay{};
    directPlay.vtbl_00 = &vtable;

    if (zNetwork_DPlay::CloseReleaseAndCoUninitialize(&directPlay) != 7) {
        return 1;
    }

    if (g_closeCalls != 1 || g_releaseCalls != 1) {
        return 2;
    }

    return zNetwork_DPlay::CloseReleaseAndCoUninitialize(nullptr) == 0 ? 0 : 3;
}

extern "C" int znetwork_dispatch_handler_list_smoke() {
    ResetNetwork();
    zNetwork_InitMessageHandlers();
    zNetworkDispatchHandlerListNode *const sentinel = g_zNetwork_DispatchHandlerListSentinel;
    if (sentinel == nullptr || sentinel->next != sentinel || sentinel->prev != sentinel ||
        g_zNetwork_DispatchHandlerListCount != 0) {
        return 1;
    }

    auto *node = AllocObject<zNetworkDispatchHandlerListNode>();
    sentinel->next = node;
    sentinel->prev = node;
    node->next = sentinel;
    node->prev = sentinel;
    g_zNetwork_DispatchHandlerListCount = 1;

    zNetwork_DestroyDispatchHandlerList();
    if (g_zNetwork_DispatchHandlerListSentinel != nullptr ||
        g_zNetwork_DispatchHandlerListCount != 0) {
        return 2;
    }

    zNetwork_DestroyDispatchHandlerList();
    return g_zNetwork_DispatchHandlerListCount == 0 ? 0 : 3;
}

extern "C" int znetwork_player_record_accessors_smoke() {
    ResetNetwork();

    auto *playerList = AllocObject<zNetworkPlayerRecordList>();
    auto *sentinel = AllocObject<zNetworkPlayerRecordListNode>();
    auto *node = AllocObject<zNetworkPlayerRecordListNode>();
    auto *playerRecord = AllocObject<zNetwork_PlayerRecord>();
    playerRecord->playerKey = 0x44556677;
    playerRecord->colorIndex = 5;
    sentinel->next = node;
    sentinel->prev = node;
    node->next = sentinel;
    node->prev = sentinel;
    node->playerRecord = playerRecord;
    playerList->sentinelNode = sentinel;
    playerList->count = 1;
    g_zNetwork_PlayerRecordList = playerList;

    zNetworkDPlaySessionDescCache session{};
    session.desc.maxPlayers = 8;
    g_zNetwork_CurrentSessionDescCache = &session;

    const bool valid = zNetwork_GetPlayerRecordCount() == 1 &&
                       zNetwork_GetPlayerColorIndexByKey(0x44556677) == 5 &&
                       zNetwork_GetPlayerColorIndexByKey(0x11111111) == 0;

    playerRecord->colorIndex = 9;
    const bool tooHigh = zNetwork_GetPlayerColorIndexByKey(0x44556677) == 0;

    playerRecord->colorIndex = 4;
    g_zNetwork_CurrentSessionDescCache = nullptr;
    const bool noSession = zNetwork_GetPlayerColorIndexByKey(0x44556677) == 0;

    ::operator delete(playerRecord);
    ::operator delete(node);
    ::operator delete(sentinel);
    ::operator delete(playerList);
    g_zNetwork_PlayerRecordList = nullptr;
    return valid && tooHigh && noSession ? 0 : 1;
}

extern "C" int znetwork_packet_send_wrappers_smoke() {
    ResetNetwork();

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x10203040;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x10203040;

    zNetworkPacketHeader packet{};
    packet.packetType = 6;
    packet.packetSizeBytes = 12;

    if (zNetwork_SendPacketReliable(&packet) != 0 || g_sendCalls != 1 ||
        g_sendFromPlayer != 0x10203040 || g_sendFlags != 1 || g_sendPacket != &packet ||
        g_sendPacketSize != 12) {
        return 1;
    }

    g_sendCalls = 0;
    if (zNetwork_SendPacketUnreliable(&packet) != 0 || g_sendCalls != 1 || g_sendFlags != 0) {
        return 2;
    }

    g_zNetwork_TcpIpAsyncSendEnabled = 1;
    g_zNetwork_LastSendExHandle = 0x55aa;
    g_zNetwork_LastSendExCompleted = 0;
    if (zNetwork_SendPacketUnreliable(&packet) != 0 || g_cancelCalls != 1 ||
        g_cancelHandle != 0x55aa || g_cancelFlags != 0 || g_sendExCalls != 1 ||
        g_sendExFlags != 0x200 || g_zNetwork_LastSendExHandle != g_sendExAsyncValue) {
        return 3;
    }

    packet.packetType = 7;
    g_sendExCalls = 0;
    g_cancelCalls = 0;
    g_zNetwork_LastSendExCompleted = 1;
    if (zNetwork_SendPacketUnreliable(&packet) != 0 || g_cancelCalls != 0 ||
        g_zNetwork_LastSendExCompleted != 1 || g_sendExCalls != 1 || g_sendExFlags != 0x600) {
        return 4;
    }

    g_sendExCalls = 0;
    if (zNetwork_SendPacketReliable(&packet) != 0 || g_sendExCalls != 1 || g_sendExFlags != 0x601) {
        return 5;
    }

    g_sendResult = static_cast<std::int32_t>(0x8000000a);
    return zNetwork_SendPacketReliable(&packet) == 0 ? 0 : 6;
}

extern "C" int pickup_send_pkt11_delta_smoke() {
    struct PickupDeltaPacket {
        zNetworkPacketHeader header;
        unsigned short flags;
        unsigned short reserved;
        int pickupId;
    };

    ResetNetwork();
    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x10203040;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x10203040;

    PickupSpawnDef spawn = {};
    spawn.pickupId = 77;
    if (Pickup::SendPkt11_Flag2Delta(&spawn) != 0 || g_sendCalls != 1 ||
        g_sendPacketSize != sizeof(PickupDeltaPacket)) {
        return 1;
    }

    auto *packet = static_cast<PickupDeltaPacket *>(g_sendPacket);
    const bool flag2Ok = packet->header.packetType == 0x11 &&
                         packet->header.packetSizeBytes == sizeof(PickupDeltaPacket) &&
                         packet->header.payloadDword0 == 0x10203040 &&
                         packet->flags == 2 && packet->pickupId == 77;
    if (!flag2Ok) {
        return 2;
    }

    g_sendCalls = 0;
    spawn.pickupId = 91;
    if (Pickup::SendPkt11_Flag8Delta(&spawn) != 0 || g_sendCalls != 1 ||
        g_sendPacketSize != sizeof(PickupDeltaPacket)) {
        return 3;
    }

    packet = static_cast<PickupDeltaPacket *>(g_sendPacket);
    const bool flag8Ok = packet->header.packetType == 0x11 &&
                         packet->header.packetSizeBytes == sizeof(PickupDeltaPacket) &&
                         packet->header.payloadDword0 == 0x10203040 &&
                         packet->flags == 8 && packet->pickupId == 91;
    return flag8Ok ? 0 : 4;
}

extern "C" int pickup_send_pkt11_create_delta_smoke() {
    struct PickupCreatePacket {
        zNetworkPacketHeader header;
        unsigned short flags;
        unsigned short reserved_0a;
        int pickupId;
        unsigned short typeKeyIndex;
        unsigned short reserved_12;
        int amount;
        zVec3 position;
        zVec3 rotation;
        float respawnDelay;
    };

    static_assert(sizeof(PickupCreatePacket) == 0x34);

    ResetNetwork();
    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x10203040;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x10203040;

    const PickupType oldType = g_PickupTypes[4];
    g_PickupTypes[4] = {};
    g_PickupTypes[4].logicalName = "create-type";

    PickupSpawnDef spawn = {};
    spawn.pickupId = 123;
    spawn.pickupType = &g_PickupTypes[4];
    spawn.amount = 17;
    spawn.position.x = 1.0f;
    spawn.position.y = 2.0f;
    spawn.position.z = 3.0f;
    spawn.rotation.x = 4.0f;
    spawn.rotation.y = 5.0f;
    spawn.rotation.z = 6.0f;
    spawn.respawnDelay = 7.5f;

    Pickup::SendPkt11_CreateDelta(&spawn);
    if (g_sendCalls != 1 || g_sendPacketSize != sizeof(PickupCreatePacket)) {
        g_PickupTypes[4] = oldType;
        return 1;
    }

    const auto *const packet = reinterpret_cast<const PickupCreatePacket *>(g_sendPacketBytes);
    const bool packetOk =
        packet->header.packetType == 0x11 &&
        packet->header.packetSizeBytes == sizeof(PickupCreatePacket) &&
        packet->header.payloadDword0 == 0x10203040 && packet->flags == 1 &&
        packet->reserved_0a == 0 && packet->pickupId == 123 && packet->typeKeyIndex == 4 &&
        packet->reserved_12 == 0 && packet->amount == 17 && packet->position.x == 1.0f &&
        packet->position.y == 2.0f && packet->position.z == 3.0f &&
        packet->rotation.x == 4.0f && packet->rotation.y == 5.0f &&
        packet->rotation.z == 6.0f && packet->respawnDelay == 7.5f;

    g_PickupTypes[4] = oldType;
    return packetOk ? 0 : 2;
}

extern "C" int znetwork_session_status_fields_smoke() {
    ResetNetwork();

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetworkDPlaySessionDescCache session{};
    char sessionName[0x5c] = "original";
    session.desc.maxPlayers = 8;
    session.desc.sessionName = sessionName;
    session.desc.customEventCode = 1;
    session.desc.customStatusFlags = 2;
    session.desc.customValueOrTime = 3;
    session.desc.customAuxParam = 4;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_CurrentSessionDescCache = &session;

    zNetworkSessionDescStatusFields fields{};
    if (zNetwork_ExtractStatusFieldsFromSessionDesc(&fields) != 1 || fields.eventCode != 1 ||
        fields.statusFlags != 2 || fields.valueOrTime != 3 || fields.auxParam != 4 ||
        fields.maxPlayers != 8 || fields.selectedSessionIndex != -1 ||
        std::strcmp(fields.sessionNameBuf, "original") != 0) {
        return 1;
    }

    fields.eventCode = 10;
    fields.statusFlags = 11;
    fields.valueOrTime = 12;
    fields.auxParam = 13;
    fields.maxPlayers = 6;
    std::strcpy(fields.sessionNameBuf, "updated");
    if (zNetwork_ApplyStatusFieldsToSessionDesc(&fields) != 1 || g_setSessionDescCalls != 1 ||
        g_setSessionDescPtr != &session.desc || g_setSessionDescFlags != 0 ||
        session.desc.customEventCode != 10 || session.desc.customStatusFlags != 11 ||
        session.desc.customValueOrTime != 12 || session.desc.customAuxParam != 13 ||
        session.desc.maxPlayers != 6 || std::strcmp(sessionName, "updated") != 0 ||
        std::strcmp(g_zNetwork_SessionNameCache, "updated") != 0) {
        return 2;
    }

    g_setSessionDescResult = static_cast<std::int32_t>(0x88770014);
    std::strcpy(fields.sessionNameBuf, "failed");
    std::strcpy(g_zNetwork_SessionNameCache, "updated");
    if (zNetwork_ApplyStatusFieldsToSessionDesc(&fields) != 0 ||
        std::strcmp(g_zNetwork_SessionNameCache, "updated") != 0) {
        return 3;
    }

    g_zNetwork_CurrentSessionDescCache = nullptr;
    g_zNetwork_pDirectPlay4 = nullptr;
    return 0;
}

extern "C" int znetwork_shutdown_session_runtime_smoke() {
    ResetNetwork();

    zNetworkDispatchHandlerRecord handlerRecord{};
    handlerRecord.packetType = 1;
    handlerRecord.handler = zNetwork_ApplyPkt01_PlayerColorAssignments;
    auto *dispatchSentinel = AllocObject<zNetworkDispatchHandlerListNode>();
    auto *dispatchNode = AllocObject<zNetworkDispatchHandlerListNode>();
    dispatchSentinel->next = dispatchNode;
    dispatchSentinel->prev = dispatchNode;
    dispatchNode->next = dispatchSentinel;
    dispatchNode->prev = dispatchSentinel;
    dispatchNode->record = &handlerRecord;
    g_zNetwork_DispatchHandlerListSentinel = dispatchSentinel;
    g_zNetwork_DispatchHandlerListCount = 1;

    auto *desc =
        static_cast<zNetworkDPlaySessionDesc *>(std::malloc(sizeof(zNetworkDPlaySessionDesc)));
    std::memset(desc, 0, sizeof(zNetworkDPlaySessionDesc));
    desc->reservedData = std::malloc(4);
    auto *sessionNode = static_cast<zArchiveListNode *>(std::malloc(sizeof(zArchiveListNode)));
    sessionNode->payload = desc;
    sessionNode->next = sessionNode;
    sessionNode->prev = sessionNode;
    g_zNetwork_EnumeratedSessionList = zArchiveList_CreateEmpty();
    g_zNetwork_EnumeratedSessionList->head = sessionNode;
    g_zNetwork_EnumeratedSessionList->count = 1;

    auto *providerList = AllocObject<zNetworkServiceProviderListVec>();
    providerList->begin = static_cast<zNetworkDPlayServiceProviderInfo **>(
        ::operator new(sizeof(zNetworkDPlayServiceProviderInfo *)));
    providerList->end = providerList->begin + 1;
    providerList->cap = providerList->end;
    auto *provider = AllocObject<zNetworkDPlayServiceProviderInfo>();
    provider->displayName = static_cast<char *>(std::malloc(8));
    provider->connectionData = std::malloc(8);
    providerList->begin[0] = provider;
    g_zNetwork_ServiceProviderList = providerList;

    auto *playerList = AllocObject<zNetworkPlayerRecordList>();
    auto *playerSentinel = AllocObject<zNetworkPlayerRecordListNode>();
    auto *playerNode = AllocObject<zNetworkPlayerRecordListNode>();
    auto *playerRecord = AllocObject<zNetwork_PlayerRecord>();
    playerRecord->playerKey = 0x1234;
    playerSentinel->next = playerNode;
    playerSentinel->prev = playerNode;
    playerNode->next = playerSentinel;
    playerNode->prev = playerSentinel;
    playerNode->playerRecord = playerRecord;
    playerList->sentinelNode = playerSentinel;
    playerList->count = 1;
    g_zNetwork_PlayerRecordList = playerList;

    NetPkt01_PlayerColorAssignments packet{};
    packet.pairCount = 1;
    packet.pairs[0].playerKey = 0x1234;
    packet.pairs[0].colorIndex = 5;
    if (zNetwork_ApplyPkt01_PlayerColorAssignments(0, &packet.header) != 1 ||
        playerRecord->colorIndex != 5 || g_zNetwork_PlayerColorInUseFlags[5] != 1) {
        return 1;
    }

    g_zNetwork_SessionRuntimeInitialized = 1;
    g_zNetwork_ReceiveBuffer = std::malloc(16);

    if (zNetwork::ShutdownSessionRuntime() != 0) {
        return 2;
    }

    const bool ok =
        g_zNetwork_SessionRuntimeInitialized == 0 && g_zNetwork_EnumeratedSessionList == nullptr &&
        g_zNetwork_ServiceProviderList == nullptr && g_zNetwork_PlayerRecordList == nullptr &&
        g_zNetwork_ReceiveBuffer == nullptr && g_zNetwork_DispatchHandlerListCount == 0 &&
        dispatchSentinel->next == dispatchSentinel && dispatchSentinel->prev == dispatchSentinel;

    ::operator delete(dispatchSentinel);
    g_zNetwork_DispatchHandlerListSentinel = nullptr;

    return ok ? 0 : 3;
}
