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
int g_enumPlayersCalls;
std::int32_t g_enumPlayersResult;
int g_receiveCalls;
std::int32_t g_receiveResults[4];
std::uint32_t g_receiveFrom[4];
std::uint32_t g_receiveTo[4];
std::uint32_t g_receiveSizes[4];
unsigned char g_receivePayloads[4][128];
int g_dispatchCallsA;
int g_dispatchCallsB;
int g_dispatchSenderA;
int g_dispatchSenderB;
zNetworkPacketHeader *g_dispatchPacketA;
zNetworkPacketHeader *g_dispatchPacketB;
int g_dispatchPacketTypeA;
int g_dispatchPacketTypeB;
int g_dispatchPacketPayloadA;
int g_dispatchPacketPayloadB;
int g_fatalDisconnectCalls;
int g_fatalDisconnectReason;

std::int32_t RECOIL_STDCALL CloseFake(zNetwork_DPlay4 *) {
    ++g_closeCalls;
    return 0;
}

std::int32_t RECOIL_STDCALL ReleaseFake(zNetwork_DPlay4 *) {
    ++g_releaseCalls;
    return 7;
}

int RECOIL_FASTCALL PacketHandlerFake(int, zNetworkPacketHeader *) {
    return 0;
}

int RECOIL_FASTCALL PacketHandlerDispatchA(int senderPlayerId, zNetworkPacketHeader *packet) {
    ++g_dispatchCallsA;
    g_dispatchSenderA = senderPlayerId;
    g_dispatchPacketA = packet;
    g_dispatchPacketTypeA = packet != nullptr ? packet->packetType : 0;
    g_dispatchPacketPayloadA = packet != nullptr ? packet->payloadDword0 : 0;
    return 0;
}

int RECOIL_FASTCALL PacketHandlerDispatchB(int senderPlayerId, zNetworkPacketHeader *packet) {
    ++g_dispatchCallsB;
    g_dispatchSenderB = senderPlayerId;
    g_dispatchPacketB = packet;
    g_dispatchPacketTypeB = packet != nullptr ? packet->packetType : 0;
    g_dispatchPacketPayloadB = packet != nullptr ? packet->payloadDword0 : 0;
    return 0;
}

void RECOIL_FASTCALL FatalDisconnectFake(int reason) {
    ++g_fatalDisconnectCalls;
    g_fatalDisconnectReason = reason;
}

std::int32_t RECOIL_STDCALL DestroyPlayerFake(zNetwork_DPlay4 *, std::uint32_t playerKey) {
    ++g_destroyPlayerCalls;
    g_destroyedPlayerKey = playerKey;
    return g_destroyPlayerResult;
}

std::int32_t RECOIL_STDCALL EnumPlayersFake(zNetwork_DPlay4 *, void *,
                                            zNetworkDPlayEnumPlayersCallback callback, void *,
                                            std::uint32_t) {
    ++g_enumPlayersCalls;
    if (g_enumPlayersResult < 0) {
        return g_enumPlayersResult;
    }

    char aceName[] = "Ace";
    char duplicateName[] = "Duplicate";
    char longName[] = "Long";
    zNetworkDPlayName ace = {sizeof(zNetworkDPlayName), 0, aceName, longName};
    zNetworkDPlayName duplicate = {sizeof(zNetworkDPlayName), 0, duplicateName, longName};
    callback(0x101, 0, &ace, 0, nullptr);
    callback(0x101, 0, &duplicate, 0, nullptr);
    return g_enumPlayersResult;
}

std::int32_t RECOIL_STDCALL ReceiveFake(zNetwork_DPlay4 *, std::uint32_t *fromPlayer,
                                        std::uint32_t *toPlayer, std::uint32_t, void *packet,
                                        std::uint32_t *packetSizeBytes) {
    const int index = g_receiveCalls++;
    if (fromPlayer != nullptr) {
        *fromPlayer = g_receiveFrom[index];
    }
    if (toPlayer != nullptr) {
        *toPlayer = g_receiveTo[index];
    }
    if (packetSizeBytes != nullptr) {
        *packetSizeBytes = g_receiveSizes[index];
    }
    if (packet != nullptr && g_receiveResults[index] >= 0) {
        std::memcpy(packet, g_receivePayloads[index], g_receiveSizes[index]);
    }
    return g_receiveResults[index];
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
    {}, DestroyPlayerFake, {}, EnumPlayersFake, {}, ReceiveFake, SendFake, {}, SetSessionDescFake,
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
    g_enumPlayersCalls = 0;
    g_enumPlayersResult = 0;
    g_receiveCalls = 0;
    std::memset(g_receiveResults, 0, sizeof(g_receiveResults));
    std::memset(g_receiveFrom, 0, sizeof(g_receiveFrom));
    std::memset(g_receiveTo, 0, sizeof(g_receiveTo));
    std::memset(g_receiveSizes, 0, sizeof(g_receiveSizes));
    std::memset(g_receivePayloads, 0, sizeof(g_receivePayloads));
    g_dispatchCallsA = 0;
    g_dispatchCallsB = 0;
    g_dispatchSenderA = 0;
    g_dispatchSenderB = 0;
    g_dispatchPacketA = nullptr;
    g_dispatchPacketB = nullptr;
    g_dispatchPacketTypeA = 0;
    g_dispatchPacketTypeB = 0;
    g_dispatchPacketPayloadA = 0;
    g_dispatchPacketPayloadB = 0;
    g_fatalDisconnectCalls = 0;
    g_fatalDisconnectReason = 0;
    g_zNetwork_pDirectPlay4 = nullptr;
    g_zNetwork_LocalPlayerRecord = nullptr;
    g_zNetwork_IsHostFlag = 0;
    g_zNetwork_LocalPlayerKey = 0;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_zNetwork_LastSendExHandle = 0;
    g_zNetwork_LastSendExCompleted = 0;
    g_zNetwork_SessionRuntimeInitialized = 0;
    g_zNetwork_CurrentSessionDescCache = nullptr;
    g_zNetwork_FatalDisconnectCallback = nullptr;
    g_zNetwork_FatalDisconnectTriggered = 0;
    g_zNetworkCurrentPlayerCountCached = 0;
    g_zNetwork_EnumeratedSessionList = nullptr;
    g_zNetwork_ServiceProviderList = nullptr;
    g_zNetwork_PlayerRecordList = nullptr;
    g_zNetwork_ReceiveBuffer = nullptr;
    g_zNetwork_ReceiveBufferCapacity = 0;
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

extern "C" int znetwork_register_packet_handler_smoke() {
    ResetNetwork();
    zNetwork_InitMessageHandlers();
    zNetworkDispatchHandlerListNode *const sentinel = g_zNetwork_DispatchHandlerListSentinel;

    zNetworkDispatchHandlerRecord *const record =
        zNetwork::RegisterPacketHandler(0x77, &PacketHandlerFake, 3);
    zNetworkDispatchHandlerListNode *const node = sentinel->next;
    const bool firstOk =
        record != nullptr && record->packetType == 0x77 && record->handler == &PacketHandlerFake &&
        record->mode == 3 && g_zNetwork_DispatchHandlerListCount == 1 &&
        sentinel->next == node && sentinel->prev == node && node->next == sentinel &&
        node->prev == sentinel && node->record == record;

    zNetworkDispatchHandlerRecord *const record2 =
        zNetwork::RegisterPacketHandler(0x78, &PacketHandlerFake, 4);
    zNetworkDispatchHandlerListNode *const node2 = sentinel->prev;
    const bool secondOk =
        record2 != nullptr && record2->packetType == 0x78 && record2->mode == 4 &&
        g_zNetwork_DispatchHandlerListCount == 2 && sentinel->next == node &&
        sentinel->prev == node2 && node->next == node2 && node2->prev == node &&
        node2->next == sentinel && node2->record == record2;

    ::operator delete(record);
    ::operator delete(record2);
    zNetwork_DestroyDispatchHandlerList();
    return firstOk && secondOk ? 0 : 1;
}

extern "C" int znetwork_dispatch_packet_to_handlers_smoke() {
    ResetNetwork();
    zNetwork_InitMessageHandlers();

    zNetworkDispatchHandlerRecord *const recordA =
        zNetwork::RegisterPacketHandler(0x77, &PacketHandlerDispatchA, 3);
    zNetworkDispatchHandlerRecord *const recordSkip =
        zNetwork::RegisterPacketHandler(0x78, &PacketHandlerFake, 4);
    zNetworkDispatchHandlerRecord *const recordB =
        zNetwork::RegisterPacketHandler(0x77, &PacketHandlerDispatchB, 5);

    zNetworkPacketHeader packet{};
    packet.packetType = 0x77;
    zNetwork_DPlay::DispatchPacketToHandlers(0x1234, &packet);

    const bool dispatchOk = g_dispatchCallsA == 1 && g_dispatchCallsB == 1 &&
                            g_dispatchSenderA == 0x1234 && g_dispatchSenderB == 0x1234 &&
                            g_dispatchPacketA == &packet && g_dispatchPacketB == &packet;

    packet.packetType = 0x79;
    zNetwork_DPlay::DispatchPacketToHandlers(0x5678, &packet);

    const bool skipOk = g_dispatchCallsA == 1 && g_dispatchCallsB == 1;

    ::operator delete(recordA);
    ::operator delete(recordSkip);
    ::operator delete(recordB);
    zNetwork_DestroyDispatchHandlerList();
    return dispatchOk && skipOk ? 0 : 1;
}

extern "C" int znetwork_player_record_accessors_smoke() {
    ResetNetwork();

    auto *playerList = AllocObject<zNetworkPlayerRecordList>();
    auto *sentinel = AllocObject<zNetworkPlayerRecordListNode>();
    auto *node = AllocObject<zNetworkPlayerRecordListNode>();
    auto *playerRecord = AllocObject<zNetwork_PlayerRecord>();
    playerRecord->playerKey = 0x44556677;
    std::strcpy(playerRecord->playerName, "Wingman");
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
                       zNetwork_FindPlayerRecordByKey(0x44556677) == playerRecord &&
                       zNetwork_FindPlayerRecordByKey(0x11111111) == nullptr &&
                       zNetwork_GetPlayerColorIndexByKey(0x44556677) == 5 &&
                       zNetwork_GetPlayerColorIndexByKey(0x11111111) == 0;

    char nameBuffer[16] = {};
    const bool nameOk = zNetwork::GetPlayerNameByKey(0x44556677, nameBuffer, sizeof(nameBuffer)) ==
                            1 &&
                        std::strcmp(nameBuffer, "Wingman") == 0 &&
                        zNetwork::GetPlayerNameByKey(0x11111111, nameBuffer,
                                                     sizeof(nameBuffer)) == 0;

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
    return valid && nameOk && tooHigh && noSession ? 0 : 1;
}

extern "C" int znetwork_remove_player_record_by_key_smoke() {
    ResetNetwork();

    auto *playerList = AllocObject<zNetworkPlayerRecordList>();
    auto *sentinel = AllocObject<zNetworkPlayerRecordListNode>();
    auto *removeNode = AllocObject<zNetworkPlayerRecordListNode>();
    auto *keepNode = AllocObject<zNetworkPlayerRecordListNode>();
    auto *removeRecord = AllocObject<zNetwork_PlayerRecord>();
    auto *keepRecord = AllocObject<zNetwork_PlayerRecord>();

    removeRecord->playerKey = 0x11112222;
    removeRecord->colorIndex = 5;
    keepRecord->playerKey = 0x33334444;
    keepRecord->colorIndex = 6;

    sentinel->next = removeNode;
    sentinel->prev = keepNode;
    removeNode->next = keepNode;
    removeNode->prev = sentinel;
    removeNode->playerRecord = removeRecord;
    keepNode->next = sentinel;
    keepNode->prev = removeNode;
    keepNode->playerRecord = keepRecord;
    playerList->sentinelNode = sentinel;
    playerList->count = 2;
    g_zNetwork_PlayerRecordList = playerList;
    g_zNetwork_PlayerColorInUseFlags[5] = 1;
    g_zNetwork_PlayerColorInUseFlags[6] = 1;

    zNetwork::RemovePlayerRecordByKey(0x11112222);

    const bool removedOk = playerList->count == 1 && sentinel->next == keepNode &&
                           sentinel->prev == keepNode && keepNode->next == sentinel &&
                           keepNode->prev == sentinel && keepNode->playerRecord == keepRecord &&
                           g_zNetwork_PlayerColorInUseFlags[5] == 0 &&
                           g_zNetwork_PlayerColorInUseFlags[6] == 1 &&
                           removeRecord->playerKey == 0x11112222;

    zNetwork::RemovePlayerRecordByKey(0x99999999);

    const bool missingOk = playerList->count == 1 && sentinel->next == keepNode &&
                           sentinel->prev == keepNode &&
                           g_zNetwork_PlayerColorInUseFlags[6] == 1;

    ::operator delete(removeRecord);
    ::operator delete(keepRecord);
    ::operator delete(keepNode);
    ::operator delete(sentinel);
    ::operator delete(playerList);
    g_zNetwork_PlayerRecordList = nullptr;
    return removedOk && missingOk ? 0 : 1;
}

extern "C" int znetwork_alloc_free_player_color_index_smoke() {
    ResetNetwork();

    zNetworkDPlaySessionDescCache session{};
    session.desc.maxPlayers = 4;
    g_zNetwork_CurrentSessionDescCache = &session;
    g_zNetwork_PlayerColorInUseFlags[1] = 1;
    g_zNetwork_PlayerColorInUseFlags[2] = 1;
    g_zNetwork_PlayerColorInUseFlags[4] = 1;

    const int firstFree = zNetwork::AllocFreePlayerColorIndex();
    const bool firstOk = firstFree == 3 && g_zNetwork_PlayerColorInUseFlags[3] == 1;

    const int noneFree = zNetwork::AllocFreePlayerColorIndex();
    const bool fullOk = noneFree == 0;

    session.desc.maxPlayers = 0;
    g_zNetwork_PlayerColorInUseFlags[1] = 0;
    const bool emptyOk = zNetwork::AllocFreePlayerColorIndex() == 0 &&
                         g_zNetwork_PlayerColorInUseFlags[1] == 0;

    g_zNetwork_CurrentSessionDescCache = nullptr;
    return firstOk && fullOk && emptyOk ? 0 : 1;
}

extern "C" int znetwork_host_send_player_color_assignments_packet_smoke() {
    ResetNetwork();

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0xaaaabbbb;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0xaaaabbbb;
    g_zNetwork_IsHostFlag = 1;

    zNetworkDPlaySessionDescCache session{};
    session.desc.maxPlayers = 4;
    g_zNetwork_CurrentSessionDescCache = &session;
    g_zNetwork_PlayerColorInUseFlags[1] = 1;

    auto *playerList = AllocObject<zNetworkPlayerRecordList>();
    auto *sentinel = AllocObject<zNetworkPlayerRecordListNode>();
    auto *joinNode = AllocObject<zNetworkPlayerRecordListNode>();
    auto *otherNode = AllocObject<zNetworkPlayerRecordListNode>();
    auto *joinRecord = AllocObject<zNetwork_PlayerRecord>();
    auto *otherRecord = AllocObject<zNetwork_PlayerRecord>();

    joinRecord->playerKey = 0x11112222;
    joinRecord->colorIndex = 0;
    otherRecord->playerKey = 0x33334444;
    otherRecord->colorIndex = 4;
    sentinel->next = joinNode;
    sentinel->prev = otherNode;
    joinNode->next = otherNode;
    joinNode->prev = sentinel;
    joinNode->playerRecord = joinRecord;
    otherNode->next = sentinel;
    otherNode->prev = joinNode;
    otherNode->playerRecord = otherRecord;
    playerList->sentinelNode = sentinel;
    playerList->count = 2;
    g_zNetwork_PlayerRecordList = playerList;

    zNetwork::HostSendPlayerColorAssignmentsPacket(0x11112222);

    struct PacketFixture {
        zNetworkPacketHeader header;
        int pairCount;
        zNetworkPlayerColorPair pairs[2];
    } sentPacket{};
    std::memcpy(&sentPacket, g_sendPacketBytes, sizeof(sentPacket));

    const bool packetOk =
        g_sendCalls == 1 && g_sendFromPlayer == 0xaaaabbbb &&
        g_sendPacketSize == sizeof(sentPacket) && sentPacket.header.packetType == 1 &&
        sentPacket.header.packetSizeBytes == sizeof(sentPacket) &&
        sentPacket.header.payloadDword0 == 0xaaaabbbb && sentPacket.pairCount == 2 &&
        sentPacket.pairs[0].playerKey == 0x11112222 && sentPacket.pairs[0].colorIndex == 2 &&
        sentPacket.pairs[1].playerKey == 0x33334444 && sentPacket.pairs[1].colorIndex == 4 &&
        joinRecord->colorIndex == 2 && g_zNetwork_PlayerColorInUseFlags[2] == 1;

    g_sendCalls = 0;
    zNetwork::HostSendPlayerColorAssignmentsPacket(0x99999999);
    const bool missingOk = g_sendCalls == 0;

    g_zNetwork_IsHostFlag = 0;
    zNetwork::HostSendPlayerColorAssignmentsPacket(0x11112222);
    const bool nonHostOk = g_sendCalls == 0;

    ::operator delete(joinRecord);
    ::operator delete(otherRecord);
    ::operator delete(joinNode);
    ::operator delete(otherNode);
    ::operator delete(sentinel);
    ::operator delete(playerList);
    g_zNetwork_PlayerRecordList = nullptr;
    g_zNetwork_CurrentSessionDescCache = nullptr;
    g_zNetwork_LocalPlayerRecord = nullptr;
    g_zNetwork_pDirectPlay4 = nullptr;
    return packetOk && missingOk && nonHostOk ? 0 : 1;
}

extern "C" int znetwork_dplay_pump_incoming_messages_smoke() {
    ResetNetwork();
    zNetwork_InitMessageHandlers();

    zNetworkDispatchHandlerRecord *const joinRecord =
        zNetwork::RegisterPacketHandler(2, &PacketHandlerDispatchA, 0);
    zNetworkDispatchHandlerRecord *const leaveRecord =
        zNetwork::RegisterPacketHandler(3, &PacketHandlerDispatchB, 0);
    zNetworkDispatchHandlerRecord *const sessionJoinRecord =
        zNetwork::RegisterPacketHandler(4, &PacketHandlerDispatchA, 0);
    zNetworkDispatchHandlerRecord *const sessionLeaveRecord =
        zNetwork::RegisterPacketHandler(5, &PacketHandlerDispatchB, 0);

    auto *playerList = AllocObject<zNetworkPlayerRecordList>();
    auto *sentinel = AllocObject<zNetworkPlayerRecordListNode>();
    sentinel->next = sentinel;
    sentinel->prev = sentinel;
    playerList->sentinelNode = sentinel;
    playerList->count = 0;
    g_zNetwork_PlayerRecordList = playerList;

    zNetworkDPlaySessionDescCache session{};
    g_zNetwork_CurrentSessionDescCache = &session;

    char shortName[] = "Ace";
    char longName[] = "Ace Pilot";
    zNetworkDPlaySystemMessage message{};
    message.msgType = 3;
    message.fields.playerId = 0x11112222;
    message.fields.createFlagsOrPlayerType = sizeof(zNetworkDPlayName);
    message.fields.nameShortOrAsyncHandle = 7;
    message.fields.nameLong = shortName;
    message.fields.nameDisplay = longName;

    const int createResult = zNetworkDPlay::PumpIncomingMessages(&message);
    zNetwork_PlayerRecord *const createdRecord = zNetwork_FindPlayerRecordByKey(0x11112222);
    const bool createOk =
        createResult == 0 && createdRecord != nullptr && playerList->count == 1 &&
        g_zNetworkCurrentPlayerCountCached == 1 && createdRecord->playerKey == 0x11112222 &&
        createdRecord->playerNameInfo.size == sizeof(zNetworkDPlayName) &&
        createdRecord->playerNameInfo.flags == 7 &&
        createdRecord->playerNameInfo.shortName == shortName &&
        createdRecord->playerNameInfo.longName == longName &&
        std::strcmp(createdRecord->playerName, "Ace Pilot") == 0 &&
        std::strcmp(createdRecord->altName, "Ace") == 0 && createdRecord->colorIndex == 0 &&
        g_dispatchCallsA == 1 && g_dispatchSenderA == 0x11112222 &&
        g_dispatchPacketTypeA == 2 && g_dispatchPacketPayloadA == 0;

    message.msgType = 5;
    g_dispatchCallsB = 0;
    zNetworkDPlay::PumpIncomingMessages(&message);
    const bool leaveOk = playerList->count == 0 && g_zNetworkCurrentPlayerCountCached == 0 &&
                         g_dispatchCallsB == 1 && g_dispatchSenderB == 0x11112222 &&
                         g_dispatchPacketTypeB == 3 &&
                         zNetwork_FindPlayerRecordByKey(0x11112222) == nullptr;

    message.msgType = 0x101;
    g_zNetwork_IsHostFlag = 0;
    zNetworkDPlay::PumpIncomingMessages(&message);
    const bool hostOk = g_zNetwork_IsHostFlag == 1;

    message.msgType = 0x102;
    g_dispatchCallsA = 0;
    zNetworkDPlay::PumpIncomingMessages(&message);
    const bool sessionJoinOk = g_dispatchCallsA == 1 && g_dispatchPacketTypeA == 4;

    message.msgType = 0x103;
    g_dispatchCallsB = 0;
    zNetworkDPlay::PumpIncomingMessages(&message);
    const bool sessionLeaveOk = g_dispatchCallsB == 1 && g_dispatchPacketTypeB == 5;

    zNetworkDPlaySessionDesc desc{};
    desc.maxPlayers = 6;
    desc.customEventCode = 10;
    desc.customStatusFlags = 11;
    desc.customValueOrTime = 12;
    desc.customAuxParam = 13;
    std::memcpy(message.payload_004, &desc, sizeof(desc));
    message.msgType = 0x104;
    zNetworkDPlay::PumpIncomingMessages(&message);
    const bool descOk = session.desc.maxPlayers == 6 && session.desc.customEventCode == 10 &&
                        session.desc.customStatusFlags == 11 &&
                        session.desc.customValueOrTime == 12 &&
                        session.desc.customAuxParam == 13;

    message.msgType = 0x10d;
    message.fields.nameShortOrAsyncHandle = 0x55aa;
    g_zNetwork_LastSendExHandle = 0x55aa;
    g_zNetwork_LastSendExCompleted = 0;
    zNetworkDPlay::PumpIncomingMessages(&message);
    const bool sendCompleteOk = g_zNetwork_LastSendExCompleted == 1;

    message.msgType = 0x31;
    g_zNetwork_FatalDisconnectCallback = FatalDisconnectFake;
    const int fatalResult = zNetworkDPlay::PumpIncomingMessages(&message);
    const bool fatalOk = fatalResult == -1 && g_zNetwork_FatalDisconnectTriggered == 1 &&
                         g_fatalDisconnectCalls == 1 && g_fatalDisconnectReason == -1;

    ::operator delete(createdRecord);
    ::operator delete(joinRecord);
    ::operator delete(leaveRecord);
    ::operator delete(sessionJoinRecord);
    ::operator delete(sessionLeaveRecord);
    zNetwork_DestroyDispatchHandlerList();
    ::operator delete(playerList);
    g_zNetwork_PlayerRecordList = nullptr;
    g_zNetwork_CurrentSessionDescCache = nullptr;
    return createOk && leaveOk && hostOk && sessionJoinOk && sessionLeaveOk && descOk &&
                   sendCompleteOk && fatalOk
                   ? 0
                   : 1;
}

extern "C" int znetwork_dplay_receive_pending_messages_smoke() {
    ResetNetwork();
    zNetwork_InitMessageHandlers();

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_SessionRuntimeInitialized = 0;
    if (zNetworkDPlay::ReceivePendingMessages(1) != 0 || g_receiveCalls != 0) {
        return 1;
    }

    g_zNetwork_SessionRuntimeInitialized = 1;
    g_zNetwork_FatalDisconnectTriggered = 1;
    if (zNetworkDPlay::ReceivePendingMessages(1) != -1 || g_receiveCalls != 0) {
        return 2;
    }

    g_zNetwork_FatalDisconnectTriggered = 0;
    zNetworkDispatchHandlerRecord *const sessionRecord =
        zNetwork::RegisterPacketHandler(4, &PacketHandlerDispatchA, 0);
    zNetworkDispatchHandlerRecord *const playerRecord =
        zNetwork::RegisterPacketHandler(7, &PacketHandlerDispatchB, 0);

    g_zNetwork_ReceiveBufferCapacity = 8;
    g_zNetwork_ReceiveBuffer = std::malloc(g_zNetwork_ReceiveBufferCapacity);

    g_receiveResults[0] = static_cast<std::int32_t>(0x8877001e);
    g_receiveSizes[0] = sizeof(zNetworkDPlaySystemMessage);

    zNetworkDPlaySystemMessage systemMessage{};
    systemMessage.msgType = 0x102;
    systemMessage.fields.playerId = 0x11112222;
    std::memcpy(g_receivePayloads[1], &systemMessage, sizeof(systemMessage));
    g_receiveResults[1] = 0;
    g_receiveFrom[1] = 0;
    g_receiveSizes[1] = sizeof(systemMessage);

    zNetworkPacketHeader playerPacket{};
    playerPacket.packetType = 7;
    playerPacket.packetSizeBytes = sizeof(playerPacket);
    playerPacket.payloadDword0 = 0x12345678;
    std::memcpy(g_receivePayloads[2], &playerPacket, sizeof(playerPacket));
    g_receiveResults[2] = 0;
    g_receiveFrom[2] = 0x33334444;
    g_receiveSizes[2] = sizeof(playerPacket);

    const int processed = zNetworkDPlay::ReceivePendingMessages(2);
    const bool receiveOk =
        processed == 2 && g_receiveCalls == 3 &&
        g_zNetwork_ReceiveBufferCapacity == sizeof(zNetworkDPlaySystemMessage) &&
        g_zNetwork_ReceiveBuffer != nullptr && g_dispatchCallsA == 1 &&
        g_dispatchPacketTypeA == 4 && g_dispatchSenderA == 0x11112222 &&
        g_dispatchCallsB == 1 && g_dispatchPacketTypeB == 7 &&
        g_dispatchSenderB == 0x33334444 && g_dispatchPacketPayloadB == 0x12345678;

    std::free(g_zNetwork_ReceiveBuffer);
    g_zNetwork_ReceiveBuffer = nullptr;
    g_zNetwork_ReceiveBufferCapacity = 0;
    ::operator delete(sessionRecord);
    ::operator delete(playerRecord);
    zNetwork_DestroyDispatchHandlerList();
    g_zNetwork_pDirectPlay4 = nullptr;
    return receiveOk ? 0 : 3;
}

extern "C" int znetwork_dplay_enum_players_smoke() {
    ResetNetwork();

    auto *playerList = AllocObject<zNetworkPlayerRecordList>();
    auto *sentinel = AllocObject<zNetworkPlayerRecordListNode>();
    sentinel->next = sentinel;
    sentinel->prev = sentinel;
    playerList->sentinelNode = sentinel;
    playerList->count = 0;
    g_zNetwork_PlayerRecordList = playerList;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    g_zNetwork_pDirectPlay4 = &dplay;

    const int count = zNetwork_DPlay::EnumPlayers();
    zNetworkPlayerRecordListNode *const node = sentinel->next;
    const bool ok = count == 1 && g_enumPlayersCalls == 1 && playerList->count == 1 &&
                    node != sentinel && node->next == sentinel && node->prev == sentinel &&
                    sentinel->prev == node && node->playerRecord != nullptr &&
                    node->playerRecord->playerKey == 0x101 &&
                    std::strcmp(node->playerRecord->playerName, "Ace") == 0;

    ::operator delete(node->playerRecord);
    ::operator delete(node);
    ::operator delete(sentinel);
    ::operator delete(playerList);
    g_zNetwork_PlayerRecordList = nullptr;
    g_zNetwork_pDirectPlay4 = nullptr;
    return ok ? 0 : 1;
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

extern "C" int pickup_reconcile_spawn_lists_smoke() {
    struct PickupDeltaPacket {
        zNetworkPacketHeader header;
        unsigned short flags;
        unsigned short reserved;
        int pickupId;
    };

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

    ResetNetwork();
    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x10203040;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = localPlayer.playerKey;

    const PickupSpawnList oldPrimary = g_PickupSpawnList_Primary;
    const PickupSpawnList oldNetworkCopy = g_PickupSpawnList_NetworkCopy;
    const PickupType oldType = g_PickupTypes[4];
    g_PickupTypes[4] = {};
    g_PickupTypes[4].logicalName = "create-type";

    PickupSpawnDef primaryOnly = {};
    primaryOnly.pickupId = 20;
    primaryOnly.pickupType = &g_PickupTypes[4];
    primaryOnly.amount = 7;
    primaryOnly.position.x = 1.0f;
    primaryOnly.position.y = 2.0f;
    primaryOnly.position.z = 3.0f;
    primaryOnly.rotation.x = 4.0f;
    primaryOnly.rotation.y = 5.0f;
    primaryOnly.rotation.z = 6.0f;
    primaryOnly.respawnDelay = 8.0f;
    g_PickupSpawnList_Primary = {};
    g_PickupSpawnList_Primary.head = &primaryOnly;
    g_PickupSpawnList_Primary.tail = &primaryOnly;
    g_PickupSpawnList_Primary.count = 1;
    g_PickupSpawnList_NetworkCopy = {};

    Pickup::ReconcilePrimaryAndNetworkCopySpawnLists();
    const PickupCreatePacket *const createPacket =
        reinterpret_cast<const PickupCreatePacket *>(g_sendPacketBytes);
    const bool createOk = g_sendCalls == 1 &&
                          createPacket->header.packetType == 0x11 &&
                          createPacket->header.payloadDword0 == localPlayer.playerKey &&
                          createPacket->flags == 1 && createPacket->pickupId == 20 &&
                          createPacket->typeKeyIndex == 4 && createPacket->amount == 7;

    ResetNetwork();
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = localPlayer.playerKey;
    PickupSpawnDef networkOnly = {};
    networkOnly.pickupId = 30;
    g_PickupSpawnList_Primary = {};
    g_PickupSpawnList_NetworkCopy = {};
    g_PickupSpawnList_NetworkCopy.head = &networkOnly;
    g_PickupSpawnList_NetworkCopy.tail = &networkOnly;
    g_PickupSpawnList_NetworkCopy.count = 1;

    Pickup::ReconcilePrimaryAndNetworkCopySpawnLists();
    const PickupDeltaPacket *const deletePacket =
        reinterpret_cast<const PickupDeltaPacket *>(g_sendPacketBytes);
    const bool deleteOk = g_sendCalls == 1 &&
                          deletePacket->header.packetType == 0x11 &&
                          deletePacket->header.payloadDword0 == localPlayer.playerKey &&
                          deletePacket->flags == 2 && deletePacket->pickupId == 30;

    g_PickupSpawnList_Primary = oldPrimary;
    g_PickupSpawnList_NetworkCopy = oldNetworkCopy;
    g_PickupTypes[4] = oldType;
    return createOk && deleteOk ? 0 : 1;
}

extern "C" int pickup_send_pkt12_airdrop_spawn_chute_relay_smoke() {
    struct PickupAirdropRelayPacket {
        zNetworkPacketHeader header;
        zVec3 spawnPos;
        unsigned short pickupTypeIndex;
        unsigned short reserved_16;
        int nextPickupId;
    };

    static_assert(sizeof(PickupAirdropRelayPacket) == 0x1c);

    ResetNetwork();
    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x10203040;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x10203040;

    zVec3 spawnPos = {1.0f, 2.0f, 3.0f};
    Pickup::SendPkt12_AirdropSpawnChuteRelay(0x1234, &spawnPos, 77);
    if (g_sendCalls != 1 || g_sendPacketSize != sizeof(PickupAirdropRelayPacket)) {
        return 1;
    }

    const auto *const packet =
        reinterpret_cast<const PickupAirdropRelayPacket *>(g_sendPacketBytes);
    const bool packetOk =
        packet->header.packetType == 0x12 &&
        packet->header.packetSizeBytes == sizeof(PickupAirdropRelayPacket) &&
        packet->header.payloadDword0 == 0x10203040 && packet->spawnPos.x == 1.0f &&
        packet->spawnPos.y == 2.0f && packet->spawnPos.z == 3.0f &&
        packet->pickupTypeIndex == 0x1234 && packet->nextPickupId == 77;

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
