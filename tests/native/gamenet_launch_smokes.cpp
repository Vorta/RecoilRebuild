#include "Battlesport/GameNet.h"
#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/pickup.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zNetwork/zNetwork.h"

#include <cstdint>
#include <cstring>

namespace {
int g_setSessionDescCalls;
HRESULT g_setSessionDescResult;
int g_sendCalls;
DWORD g_sendFlags;
void *g_sendPacket;
DWORD g_sendPacketSize;
DWORD g_sendPacketBytesSize;
unsigned char g_sendPacketBytes[0x200];

HRESULT __stdcall SetSessionDescFake(
    zNetwork_DPlay4 *,
    zNetworkDPlaySessionDesc *,
    DWORD
) {
    ++g_setSessionDescCalls;
    return g_setSessionDescResult;
}

HRESULT __stdcall SendFake(
    zNetwork_DPlay4 *,
    DWORD,
    DWORD,
    DWORD flags,
    void *packet,
    DWORD packetSizeBytes
) {
    ++g_sendCalls;
    g_sendFlags = flags;
    g_sendPacket = packet;
    g_sendPacketSize = packetSizeBytes;
    g_sendPacketBytesSize = packetSizeBytes;
    if (packetSizeBytes <= sizeof(g_sendPacketBytes)) {
        std::memcpy(
            g_sendPacketBytes,
            packet,
            packetSizeBytes
        );
    }
    return 0;
}

struct FakeDirectPlay4 {
    void **vtable;
};

void InitDirectPlayVtable(
    void **vtable
) {
    std::memset(
        vtable,
        0,
        sizeof(void *) * 52
    );
    vtable[26] = (void *)(&SendFake);
    vtable[31] = (void *)(&SetSessionDescFake);
}

void ClearDispatchHandlerListForTest(
    zNetworkDispatchHandlerListNode &sentinel
) {
    zNetworkDispatchHandlerListNode *node = sentinel.next;
    while (node != &sentinel) {
        zNetworkDispatchHandlerListNode *const next = node->next;
        ::operator delete(node);
        node = next;
    }
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
}

bool ContainerHasChild(
    const HudUiContainer &container,
    const HudUiElement *child
) {
    const HudUiElement *node = container.childHead;
    while (node != 0) {
        if (node == child) {
            return true;
        }
        node = node->next;
    }
    return false;
}
} // namespace

extern "C" int gamenet_find_player_row_and_status_bits_smoke(void) {
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    const int oldGoalValue = g_HudSensorTracker.runtimeGoalValue;
    const int oldLapTargetStarted = g_GameNetAllPlayersLapTargetCheckStarted;
    const int oldAllowMaps = g_GameNetStatus_AllowMaps;
    const int oldNameTags = g_GameNetStatus_NameTags;

    GameNetPlayerRow *const first = (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    GameNetPlayerRow *const second = (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    std::memset(
        first,
        0,
        sizeof(*first)
    );
    std::memset(
        second,
        0,
        sizeof(*second)
    );
    first->playerKey = 10;
    first->lapCount = 3;
    first->next = second;
    second->playerKey = 20;
    second->lapCount = 4;
    g_GameNetPlayerRowHead = first;
    g_HudSensorTracker.runtimeGoalValue = 3;

    const bool rowLookup =
        GameNet::FindPlayerRowByKey(20) == second && GameNet::FindPlayerRowByKey(30) == 0;
    const bool lapsReached =
        GameNet::AreAllPlayersAtLapTarget() == 1 && g_GameNetAllPlayersLapTargetCheckStarted == 1;
    g_GameNetAllPlayersLapTargetCheckStarted = 0;
    second->lapCount = 2;
    const bool lapsBlocked =
        GameNet::AreAllPlayersAtLapTarget() == 0 && g_GameNetAllPlayersLapTargetCheckStarted == 1;
    g_GameNetAllPlayersLapTargetCheckStarted = 0;
    g_GameNetPlayerRowHead = 0;
    const bool emptyListReached =
        GameNet::AreAllPlayersAtLapTarget() == 1 && g_GameNetAllPlayersLapTargetCheckStarted == 1;

    GameNet::SetStatusBitsFromFlags(3);
    const bool bothSet = g_GameNetStatus_AllowMaps == 1 && g_GameNetStatus_NameTags == 1 &&
                         GameNet::GetStatusBitAllowMaps() == 1 &&
                         GameNet::GetStatusBitNameTags() == 1;

    GameNet::SetStatusBitsFromFlags(0);
    const bool bothClear = g_GameNetStatus_AllowMaps == 0 && g_GameNetStatus_NameTags == 0 &&
                           GameNet::GetStatusBitAllowMaps() == 0 &&
                           GameNet::GetStatusBitNameTags() == 0;

    g_GameNetPlayerRowHead = oldHead;
    g_HudSensorTracker.runtimeGoalValue = oldGoalValue;
    g_GameNetAllPlayersLapTargetCheckStarted = oldLapTargetStarted;
    g_GameNetStatus_AllowMaps = oldAllowMaps;
    g_GameNetStatus_NameTags = oldNameTags;
    ::operator delete(second);
    ::operator delete(first);

    return rowLookup && lapsReached && lapsBlocked && emptyListReached && bothSet && bothClear ? 0
                                                                                               : 1;
}

extern "C" int gamenet_unregister_gameplay_packet_handlers_smoke(void) {
    zNetworkDispatchHandlerListNode *const oldSentinel = g_zNetwork_DispatchHandlerListSentinel;
    const int oldCount = g_zNetwork_DispatchHandlerListCount;
    const int oldRegistered = g_GameNet_HandlersRegistered;

    zNetworkDispatchHandlerListNode sentinel = {};
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    g_zNetwork_DispatchHandlerListSentinel = &sentinel;
    g_zNetwork_DispatchHandlerListCount = 0;
    g_GameNet_HandlersRegistered = 1;

    zNetwork::RegisterPacketHandler(
        6,
        (zNetworkPacketHandler)&GameNet::HandlePkt06_PlayerStateSnapshot,
        0
    );
    zNetwork::RegisterPacketHandler(
        7,
        (zNetworkPacketHandler)&GameNet::HandlePkt07_AltGunDispatch,
        0
    );
    zNetwork::RegisterPacketHandler(
        0x0a,
        (zNetworkPacketHandler)&OptCatalog::HandlePkt0A_RemoveRuntimeRelay,
        0
    );
    zNetwork::RegisterPacketHandler(
        1,
        (zNetworkPacketHandler)&GameNet::ReassignPlayerColorsAndRefreshRows,
        0
    );
    zNetwork::RegisterPacketHandler(
        8,
        (zNetworkPacketHandler)&GameNet::HandlePkt08_PlayerKillEvent,
        0
    );
    zNetwork::RegisterPacketHandler(
        9,
        (zNetworkPacketHandler)&GameNet::HandlePkt09_PlayerScoreboardSnapshot,
        0
    );
    zNetwork::RegisterPacketHandler(
        0x0b,
        (zNetworkPacketHandler)&GameNet::HandlePkt0B_ChatMessage,
        0
    );
    zNetwork::RegisterPacketHandler(
        0x0e,
        (zNetworkPacketHandler)&GameNet::HandlePkt0E_PlayerLapProgress,
        0
    );
    zNetwork::RegisterPacketHandler(
        0x0c,
        (zNetworkPacketHandler)&GameNet::HandlePkt0C_HudTimerStatusBits,
        0
    );
    zNetwork::RegisterPacketHandler(
        0x0d,
        (zNetworkPacketHandler)&GameNet::HandlePkt0D_HudTimerPanelState,
        0
    );
    zNetwork::RegisterPacketHandler(
        0x0f,
        (zNetworkPacketHandler)&zDEClient_Crater::NetRelayCallback,
        0
    );
    zNetwork::RegisterPacketHandler(
        0x10,
        (zNetworkPacketHandler)&zDEClient_QSand::NetRelayCallback,
        0
    );
    zNetwork::RegisterPacketHandler(
        0x11,
        (zNetworkPacketHandler)&Pickup::HandlePkt11_SpawnDelta,
        0
    );
    zNetwork::RegisterPacketHandler(
        0x12,
        (zNetworkPacketHandler)&Pickup::HandlePkt12_AirdropSpawnChuteRelay,
        0
    );
    zNetwork::RegisterPacketHandler(
        0x13,
        (zNetworkPacketHandler)&GameNet::HandlePkt13_EffectAnimActivationRecord,
        0
    );

    GameNet::UnregisterGameplayPacketHandlers();
    const bool ok = g_zNetwork_DispatchHandlerListCount == 0 && sentinel.next == &sentinel &&
                    sentinel.prev == &sentinel && g_GameNet_HandlersRegistered == 0;

    ClearDispatchHandlerListForTest(sentinel);
    g_zNetwork_DispatchHandlerListSentinel = oldSentinel;
    g_zNetwork_DispatchHandlerListCount = oldCount;
    g_GameNet_HandlersRegistered = oldRegistered;

    return ok ? 0 : 1;
}

extern "C" int gamenet_host_update_session_status_fields_smoke(void) {
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetworkDPlaySessionDescCache *const oldSession = g_zNetwork_CurrentSessionDescCache;
    const int oldIsHost = g_zNetwork_IsHostFlag;

    void *vtable[52];
    InitDirectPlayVtable(vtable);
    FakeDirectPlay4 dplay = {vtable};
    zNetworkDPlaySessionDescCache session = {};
    char sessionName[0x5c] = "mission";
    session.desc.lpszSessionNameA = sessionName;
    session.desc.dwMaxPlayers = 8;
    session.desc.dwUser1 = 1;
    session.desc.dwUser2 = 2;
    session.desc.dwUser3 = 3;
    session.desc.dwUser4 = 4;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_CurrentSessionDescCache = &session;
    g_zNetwork_IsHostFlag = 0;
    g_setSessionDescCalls = 0;
    g_setSessionDescResult = 0;

    if (GameNet::HostUpdateSessionDescStatusFields(10, 13, 12, 11) != 0 ||
        g_setSessionDescCalls != 0 || session.desc.dwUser1 != 1) {
        g_zNetwork_IsHostFlag = oldIsHost;
        g_zNetwork_CurrentSessionDescCache = oldSession;
        g_zNetwork_pDirectPlay4 = oldDPlay;
        return 1;
    }

    g_zNetwork_IsHostFlag = 1;
    if (GameNet::HostUpdateSessionDescStatusFields(10, 13, 12, 11) != 1 ||
        g_setSessionDescCalls != 1 || session.desc.dwUser1 != 10 ||
        session.desc.dwUser2 != 11 || session.desc.dwUser3 != 12 ||
        session.desc.dwUser4 != 13 || session.desc.dwMaxPlayers != 8 ||
        std::strcmp(sessionName, "mission") != 0) {
        g_zNetwork_IsHostFlag = oldIsHost;
        g_zNetwork_CurrentSessionDescCache = oldSession;
        g_zNetwork_pDirectPlay4 = oldDPlay;
        return 2;
    }

    g_setSessionDescResult = (HRESULT)(0x88770014);
    if (GameNet::HostUpdateSessionDescStatusFields(20, 23, 22, 21) != 0 ||
        session.desc.dwUser1 != 20) {
        g_zNetwork_IsHostFlag = oldIsHost;
        g_zNetwork_CurrentSessionDescCache = oldSession;
        g_zNetwork_pDirectPlay4 = oldDPlay;
        return 3;
    }

    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_CurrentSessionDescCache = oldSession;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    return 0;
}

extern "C" int gamenet_send_pkt14_hud_timer_and_flags_sync_smoke(void) {
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const std::int32_t oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    const NetPkt14_HudTimerAndFlagsSync oldPacket = g_NetPkt14_HudTimerAndFlagsSyncBuf;

    void *vtable[52];
    InitDirectPlayVtable(vtable);
    FakeDirectPlay4 dplay = {vtable};
    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x11223344;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x55667788;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(
        g_sendPacketBytes,
        0,
        sizeof(g_sendPacketBytes)
    );

    const int result = GameNet::SendPkt14_HudTimerAndFlagsSync(
        0x12345,
        0xaabbccdd,
        77,
        0x23456
    );
    const NetPkt14_HudTimerAndFlagsSync *const sentPacket =
        (const NetPkt14_HudTimerAndFlagsSync *)(g_sendPacketBytes);
    const bool ok =
        result == 0 && g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacket == &g_NetPkt14_HudTimerAndFlagsSyncBuf &&
        g_sendPacketSize == sizeof(NetPkt14_HudTimerAndFlagsSync) &&
        g_NetPkt14_HudTimerAndFlagsSyncBuf.header.packetType == 0x14 &&
        g_NetPkt14_HudTimerAndFlagsSyncBuf.header.packetSizeBytes ==
            sizeof(NetPkt14_HudTimerAndFlagsSync) &&
        g_NetPkt14_HudTimerAndFlagsSyncBuf.header.payloadDword0 == 0x55667788 &&
        g_NetPkt14_HudTimerAndFlagsSyncBuf.eventCode == 0x2345 &&
        g_NetPkt14_HudTimerAndFlagsSyncBuf.auxParam == 0x3456 &&
        g_NetPkt14_HudTimerAndFlagsSyncBuf.valueOrTime == 77 &&
        g_NetPkt14_HudTimerAndFlagsSyncBuf.statusFlags == 0xaabbccdd &&
        sentPacket->header.payloadDword0 == 0x55667788 &&
        sentPacket->eventCode == 0x2345 && sentPacket->auxParam == 0x3456 &&
        sentPacket->valueOrTime == 77 && sentPacket->statusFlags == 0xaabbccdd;

    g_NetPkt14_HudTimerAndFlagsSyncBuf = oldPacket;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    return ok ? 0 : 1;
}

extern "C" int gamenet_reset_remote_players_and_spawn_lists_smoke(void) {
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    const unsigned int oldSpawnList = g_GameNetSpawnPointList;
    GameNetSpawnPoint *const oldSpawnHead = g_GameNetSpawnPointHead;
    GameNetSpawnPoint *const oldSpawnTail = g_GameNetSpawnPointTail;
    const unsigned int oldSpawnCount = g_GameNetSpawnPointCount;
    const unsigned int oldRowList = g_GameNetPlayerRowList;
    GameNetPlayerRow *const oldRowHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldRowTail = g_GameNetPlayerRowTail;
    const unsigned int oldRowCount = g_GameNetPlayerRowCount;

    HudUiTriplet triplet;
    triplet.Constructor();
    HudUiStatsListElement statsList = {};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    HudUiTopMessageStack topStack;
    topStack.Constructor();
    g_HudUiTopMessageStack = &topStack;

    GameNetPlayerRow *const firstRow = (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    GameNetPlayerRow *const secondRow = (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    GameNetSpawnPoint *const firstSpawn = (GameNetSpawnPoint *)(::operator new(sizeof(GameNetSpawnPoint)));
    GameNetSpawnPoint *const secondSpawn = (GameNetSpawnPoint *)(::operator new(sizeof(GameNetSpawnPoint)));
    std::memset(
        firstRow,
        0,
        sizeof(*firstRow)
    );
    std::memset(
        secondRow,
        0,
        sizeof(*secondRow)
    );
    std::memset(
        firstSpawn,
        0,
        sizeof(*firstSpawn)
    );
    std::memset(
        secondSpawn,
        0,
        sizeof(*secondSpawn)
    );
    firstRow->hudWidget.ConstructorDefault(
        "",
        0,
        0
    );
    secondRow->hudWidget.ConstructorDefault(
        "",
        0,
        0
    );

    firstRow->playerKey = 0x1201;
    firstRow->playerColorPackedRgb = 0x00112233;
    std::strcpy(
        firstRow->displayName,
        "First"
    );
    firstRow->next = secondRow;

    secondRow->playerKey = 0x1202;
    secondRow->playerColorPackedRgb = 0x00445566;
    std::strcpy(
        secondRow->displayName,
        "Second"
    );

    firstSpawn->next = secondSpawn;

    g_GameNetPlayerRowList = 1;
    g_GameNetPlayerRowHead = firstRow;
    g_GameNetPlayerRowTail = secondRow;
    g_GameNetPlayerRowCount = 2;
    g_GameNetSpawnPointList = 1;
    g_GameNetSpawnPointHead = firstSpawn;
    g_GameNetSpawnPointTail = secondSpawn;
    g_GameNetSpawnPointCount = 2;

    triplet.AddEntry(firstRow);
    triplet.AddEntry(secondRow);
    topStack.AddChild((HudUiElement *)(&firstRow->hudWidget));
    topStack.AddChild((HudUiElement *)(&secondRow->hudWidget));

    GameNet::ResetRemotePlayersAndSpawnLists();

    const bool listsCleared =
        g_GameNetPlayerRowList == 0 && g_GameNetPlayerRowHead == 0 &&
        g_GameNetPlayerRowTail == 0 && g_GameNetPlayerRowCount == 0 &&
        g_GameNetSpawnPointList == 0 && g_GameNetSpawnPointHead == 0 &&
        g_GameNetSpawnPointTail == 0 && g_GameNetSpawnPointCount == 0;
    const bool hudCleared =
        !ContainerHasChild(topStack, (const HudUiElement *)(&firstRow->hudWidget)) &&
        !ContainerHasChild(topStack, (const HudUiElement *)(&secondRow->hudWidget)) &&
        triplet.entries.begin != 0 && triplet.entries.end == triplet.entries.begin;

    g_HudUiMgrStatsList = oldStatsList;
    g_HudUiTopMessageStack = oldTopStack;
    g_GameNetSpawnPointList = oldSpawnList;
    g_GameNetSpawnPointHead = oldSpawnHead;
    g_GameNetSpawnPointTail = oldSpawnTail;
    g_GameNetSpawnPointCount = oldSpawnCount;
    g_GameNetPlayerRowList = oldRowList;
    g_GameNetPlayerRowHead = oldRowHead;
    g_GameNetPlayerRowTail = oldRowTail;
    g_GameNetPlayerRowCount = oldRowCount;
    topStack.DestructorCore();
    triplet.DestructorCore();

    return listsCleared && hudCleared ? 0 : 1;
}

extern "C" int hud_sensor_tracker_set_runtime_timer_sec_and_goal_value_smoke(void) {
    HudSensorTracker tracker = {};
    tracker.runtimeGoalValue = 1;
    tracker.runtimeTimerSecRaw = 2;

    tracker.SetRuntimeTimerSecAndGoalValue(
        3600,
        7
    );

    return tracker.runtimeTimerSecRaw == 3600 && tracker.runtimeGoalValue == 7 ? 0 : 1;
}
