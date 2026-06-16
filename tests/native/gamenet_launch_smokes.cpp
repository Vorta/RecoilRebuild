#include "Battlesport/GameNet.h"
#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/player.h"
#include "Battlesport/pickup.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zUtil/zSaveGame.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <cstddef>
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

template <typename T> T &FieldAt(
    void *object,
    std::size_t offset
) {
    return *(T *)((unsigned char *)object + offset);
}

bool FloatNear(
    float actual,
    float expected
) {
    return actual >= expected - 0.0001f && actual <= expected + 0.0001f;
}

bool Vec3Equals(
    const zVec3 &value,
    const zVec3 &expected
) {
    return FloatNear(
               value.x,
               expected.x
           ) &&
           FloatNear(
               value.y,
               expected.y
           ) &&
           FloatNear(
               value.z,
               expected.z
           );
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
    const unsigned int oldSpawnListFlags = g_GameNetSpawnPointList.flags;
    GameNetSpawnPoint *const oldSpawnHead = g_GameNetSpawnPointHead;
    GameNetSpawnPoint *const oldSpawnTail = g_GameNetSpawnPointTail;
    const unsigned int oldSpawnCount = g_GameNetSpawnPointCount;
    const unsigned int oldRowListFlags = g_GameNetPlayerRowList.flags;
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

    g_GameNetPlayerRowList.flags = 1;
    g_GameNetPlayerRowHead = firstRow;
    g_GameNetPlayerRowTail = secondRow;
    g_GameNetPlayerRowCount = 2;
    g_GameNetSpawnPointList.flags = 1;
    g_GameNetSpawnPointHead = firstSpawn;
    g_GameNetSpawnPointTail = secondSpawn;
    g_GameNetSpawnPointCount = 2;

    triplet.AddEntry(firstRow);
    triplet.AddEntry(secondRow);
    topStack.AddChild((HudUiElement *)(&firstRow->hudWidget));
    topStack.AddChild((HudUiElement *)(&secondRow->hudWidget));

    GameNet::ResetRemotePlayersAndSpawnLists();

    const bool listsCleared =
        g_GameNetPlayerRowList.flags == 0 && g_GameNetPlayerRowHead == 0 &&
        g_GameNetPlayerRowTail == 0 && g_GameNetPlayerRowCount == 0 &&
        g_GameNetSpawnPointList.flags == 0 && g_GameNetSpawnPointHead == 0 &&
        g_GameNetSpawnPointTail == 0 && g_GameNetSpawnPointCount == 0;
    const bool hudCleared =
        !ContainerHasChild(topStack, (const HudUiElement *)(&firstRow->hudWidget)) &&
        !ContainerHasChild(topStack, (const HudUiElement *)(&secondRow->hudWidget)) &&
        triplet.entries.begin != 0 && triplet.entries.end == triplet.entries.begin;

    g_HudUiMgrStatsList = oldStatsList;
    g_HudUiTopMessageStack = oldTopStack;
    g_GameNetSpawnPointList.flags = oldSpawnListFlags;
    g_GameNetSpawnPointHead = oldSpawnHead;
    g_GameNetSpawnPointTail = oldSpawnTail;
    g_GameNetSpawnPointCount = oldSpawnCount;
    g_GameNetPlayerRowList.flags = oldRowListFlags;
    g_GameNetPlayerRowHead = oldRowHead;
    g_GameNetPlayerRowTail = oldRowTail;
    g_GameNetPlayerRowCount = oldRowCount;
    topStack.DestructorCore();
    triplet.DestructorCore();

    return listsCleared && hudCleared ? 0 : 1;
}

extern "C" int gamenet_player_row_append_smoke(void) {
    const unsigned int oldFlags = g_GameNetPlayerRowList.flags;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const unsigned int oldCount = g_GameNetPlayerRowCount;

    g_GameNetPlayerRowList.flags = 0;
    g_GameNetPlayerRowHead = 0;
    g_GameNetPlayerRowTail = 0;
    g_GameNetPlayerRowCount = 0;

    GameNetPlayerRow *const first =
        GameNetPlayerRowList::AppendNewRow(
            &g_GameNetPlayerRowList,
            0
        );
    GameNetPlayerRow *const second =
        GameNetPlayerRowList::AppendNewRow(
            &g_GameNetPlayerRowList,
            1
        );
    const bool ok =
        first != 0 && second != 0 && first != second &&
        g_GameNetPlayerRowHead == first &&
        g_GameNetPlayerRowTail == second &&
        g_GameNetPlayerRowCount == 2 &&
        first->next == second &&
        second->next == 0 &&
        second->playerKey == 0 &&
        second->displayName[0] == 0;

    ::operator delete(second);
    ::operator delete(first);
    g_GameNetPlayerRowList.flags = oldFlags;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    return ok ? 0 : 1;
}

extern "C" int gamenet_player_row_apply_color_tint_smoke(void) {
    zClass_Object3DDataPartial objectData = {};
    zClass_NodePartial objectNode = {};
    objectNode.classId = 5;
    objectNode.classData = &objectData;

    PlayerMasterModalData modalData = {};
    modalData.masterType = 5;
    PlayerModalState modalState = {};
    modalState.masterModalData = &modalData;
    modalState.modalNode = &objectNode;
    GameNetPlayerSaveState saveState = {};
    saveState.primaryModalState = &modalState;
    GameNetPlayerRow *const row =
        (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    std::memset(
        row,
        0,
        sizeof(*row)
    );
    row->playerColorIndex = 8;
    row->saveState = &saveState;

    row->ApplyPlayerColorTint();
    const bool ok = objectData.color.red == 1.0f &&
                    objectData.color.green == 1.0f &&
                    objectData.color.blue == 0.0f &&
                    objectData.colorAlpha == 0.2f &&
                    (objectData.flags & 4) != 0;
    ::operator delete(row);
    return ok ? 0 : 1;
}

extern "C" int gamenet_player_row_destroy_embedded_panel_smoke(void) {
    GameNetPlayerRow row = {};
    row.hudWidget.textPick = 0;
    row.hudWidget.textDirty = 123;

    row.DestroyEmbeddedPanel();
    return row.hudWidget.textPick == 0 &&
                   row.hudWidget.textDirty == 123
               ? 0
               : 1;
}

extern "C" int gamenet_apply_pkt06_player_state_snapshot_smoke(void) {
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const unsigned int oldCount = g_GameNetPlayerRowCount;
    const int oldFrameTick = g_zVideo_FrameTick;

    HudUiTriplet triplet;
    triplet.Constructor();
    HudUiStatsListElement statsList = {};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    zClass_Object3DDataPartial objectData = {};
    zClass_NodePartial objectNode = {};
    objectNode.classId = 5;
    objectNode.classData = &objectData;

    PlayerMasterModalData modalData = {};
    modalData.masterType = 5;
    PlayerModalState modalState = {};
    modalState.masterModalData = &modalData;
    modalState.modalNode = &objectNode;

    zUtil_PlayerStateStorage playerState = {};
    GameNetPlayerSaveState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    GameNetPlayerRow *const row = (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    std::memset(
        row,
        0,
        sizeof(*row)
    );
    row->playerKey = 0x2468;
    row->playerColorIndex = 1;
    row->saveState = &saveState;
    std::strcpy(
        row->displayName,
        "Pkt06"
    );
    g_GameNetPlayerRowHead = row;
    g_GameNetPlayerRowTail = row;
    g_GameNetPlayerRowCount = 1;
    GameNet::RefreshPlayerListMenu(row);

    zVec3 staleTargets[10] = {};
    for (int index = 0; index < 10; ++index) {
        playerState.progressTargetRuntimeSlots[index].targetPos = &staleTargets[index];
    }
    playerState.cachedAltSelectionCode = 301;
    playerState.cachedPrimarySelectionCode = 201;
    playerState.netLastUpdateFrameTick = oldFrameTick - 1;
    g_zVideo_FrameTick = oldFrameTick + 17;

    NetPkt06_PlayerStateSnapshot packet = {};
    packet.cachedAltSelectionCode = 301;
    packet.cachedPrimarySelectionCode = 201;
    packet.packedMasterTypeColorFlags = 5u | (8u << 8) | 0x10000u;
    packet.storedTargetPos = zVec3_Make(
        1.0f,
        2.0f,
        3.0f
    );
    packet.worldPos = zVec3_Make(
        4.0f,
        5.0f,
        6.0f
    );
    packet.vehicleRotationAngles = zVec3_Make(
        0.25f,
        0.5f,
        0.75f
    );
    packet.statusMeterValue = 77.0f;

    const int clearResult = GameNet::ApplyPkt06_PlayerStateSnapshotToRow(
        row,
        &packet
    );
    bool clearedSlots = true;
    for (int index = 0; index < 10; ++index) {
        clearedSlots =
            clearedSlots && playerState.progressTargetRuntimeSlots[index].targetPos == 0;
    }
    const bool firstOk =
        clearResult == 1 && playerState.netUpdateReceived == 1 &&
        row->playerColorIndex == 8 && row->playerColorPackedRgb == 0x000040ff &&
        FieldAt<unsigned int>(
            &row->hudWidget,
            0x14c
        ) == 0x000040ff &&
        FieldAt<unsigned int>(
            &row->hudWidget,
            0x150
        ) == 0x000040ff &&
        FieldAt<int>(
            &row->hudWidget,
            0x270
        ) == 1 &&
        objectData.color.red == 1.0f && objectData.color.green == 1.0f &&
        objectData.color.blue == 0.0f && objectData.colorAlpha == 0.2f &&
        Vec3Equals(
            playerState.netReceivedPos,
            packet.worldPos
        ) &&
        Vec3Equals(
            playerState.netReceivedAngles,
            packet.vehicleRotationAngles
        ) &&
        Vec3Equals(
            playerState.storedTargetPos,
            packet.storedTargetPos
        ) &&
        playerState.netInputBit16Latch == 1 && playerState.netInputBit17Latch == 0 &&
        playerState.netLastUpdateFrameTick == g_zVideo_FrameTick &&
        FloatNear(
            playerState.statusMeterValue,
            77.0f
        ) &&
        playerState.progressTargetCount == 0 && clearedSlots;

    packet.packedMasterTypeColorFlags = 5u | (8u << 8) | 0x20000u | 0x40000u;
    packet.statusMeterValue = 88.0f;
    packet.progressTargetCount = 2;
    packet.progressTargetPoints[0] = zVec3_Make(
        10.0f,
        11.0f,
        12.0f
    );
    packet.progressTargetPoints[1] = zVec3_Make(
        20.0f,
        21.0f,
        22.0f
    );

    const int targetResult = GameNet::ApplyPkt06_PlayerStateSnapshotToRow(
        row,
        &packet
    );
    const bool secondOk =
        targetResult == 1 && playerState.netInputBit16Latch == 1 &&
        playerState.netInputBit17Latch == 1 &&
        FloatNear(
            playerState.statusMeterValue,
            88.0f
        ) &&
        playerState.progressTargetCount == 2 &&
        playerState.progressTargetRuntimeSlots[0].targetPos ==
            &playerState.progressTargetPointStorage[0] &&
        playerState.progressTargetRuntimeSlots[1].targetPos ==
            &playerState.progressTargetPointStorage[1] &&
        Vec3Equals(
            playerState.progressTargetPointStorage[0],
            packet.progressTargetPoints[0]
        ) &&
        Vec3Equals(
            playerState.progressTargetPointStorage[1],
            packet.progressTargetPoints[1]
        );

    g_HudUiMgrStatsList = oldStatsList;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_zVideo_FrameTick = oldFrameTick;
    ::operator delete(row);
    triplet.DestructorCore();

    return firstOk && secondOk ? 0 : 1;
}

extern "C" int gamenet_handle_pkt06_player_state_snapshot_smoke(void) {
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const unsigned int oldCount = g_GameNetPlayerRowCount;
    const int oldInitialSyncGate = g_GameNetPkt06InitialSyncGate;
    const int oldFrameTick = g_zVideo_FrameTick;

    const int nullResult = GameNet::HandlePkt06_PlayerStateSnapshot(
        0x1111,
        0
    );

    zClass_Object3DDataPartial objectData = {};
    zClass_NodePartial objectNode = {};
    objectNode.classId = 5;
    objectNode.classData = &objectData;

    PlayerMasterModalData modalData = {};
    modalData.masterType = 5;
    PlayerModalState modalState = {};
    modalState.masterModalData = &modalData;
    modalState.modalNode = &objectNode;

    zUtil_PlayerStateStorage playerState = {};
    GameNetPlayerSaveState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    GameNetPlayerRow *const row = (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    std::memset(
        row,
        0,
        sizeof(*row)
    );
    row->playerKey = 0x2468;
    row->playerColorIndex = 1;
    row->saveState = &saveState;
    g_GameNetPlayerRowHead = row;
    g_GameNetPlayerRowTail = row;
    g_GameNetPlayerRowCount = 1;

    NetPkt06_PlayerStateSnapshot ignoredPacket = {};
    ignoredPacket.header.packetType = 5;
    ignoredPacket.header.payloadDword0 = row->playerKey;
    g_GameNetPkt06InitialSyncGate = 1;
    const int ignoredResult = GameNet::HandlePkt06_PlayerStateSnapshot(
        0x1111,
        &ignoredPacket
    );
    const bool ignoredOk =
        ignoredResult == 0 && g_GameNetPkt06InitialSyncGate == 0 &&
        playerState.netUpdateReceived == 0;

    NetPkt06_PlayerStateSnapshot packet = {};
    packet.header.packetType = 6;
    packet.header.payloadDword0 = row->playerKey;
    packet.packedMasterTypeColorFlags = 5u | (1u << 8);
    packet.worldPos = zVec3_Make(
        8.0f,
        9.0f,
        10.0f
    );
    packet.vehicleRotationAngles = zVec3_Make(
        0.125f,
        0.25f,
        0.5f
    );
    g_zVideo_FrameTick = oldFrameTick + 3;

    const int handledResult = GameNet::HandlePkt06_PlayerStateSnapshot(
        0x1111,
        &packet
    );
    const bool handledOk =
        handledResult == 0 && playerState.netUpdateReceived == 1 &&
        row->playerColorIndex == 1 &&
        Vec3Equals(
            playerState.netReceivedPos,
            packet.worldPos
        ) &&
        Vec3Equals(
            playerState.netReceivedAngles,
            packet.vehicleRotationAngles
        );

    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_GameNetPkt06InitialSyncGate = oldInitialSyncGate;
    g_zVideo_FrameTick = oldFrameTick;
    ::operator delete(row);

    return nullResult == -1 && ignoredOk && handledOk ? 0 : 1;
}

extern "C" int gamenet_spawn_remote_player_missing_template_smoke(void) {
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    zNetworkPlayerRecordList *const oldPlayerRecordList = g_zNetwork_PlayerRecordList;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const unsigned int oldCount = g_GameNetPlayerRowCount;

    zNetwork_PlayerRecord playerRecord = {};
    playerRecord.playerKey = 0x1111;
    std::strcpy(
        playerRecord.playerName,
        "Remote"
    );
    zNetworkPlayerRecordListNode sentinel = {};
    zNetworkPlayerRecordListNode playerNode = {};
    sentinel.next = &playerNode;
    sentinel.prev = &playerNode;
    playerNode.next = &sentinel;
    playerNode.prev = &sentinel;
    playerNode.playerRecord = &playerRecord;
    zNetworkPlayerRecordList playerList = {};
    playerList.sentinelNode = &sentinel;
    playerList.count = 1;
    g_zNetwork_PlayerRecordList = &playerList;

    HudUiTopMessageStack topStack = {};
    topStack.enabled = 0;
    g_HudUiTopMessageStack = &topStack;

    g_GameNetPlayerRowHead = 0;
    g_GameNetPlayerRowTail = 0;
    g_GameNetPlayerRowCount = 0;

    NetPkt06_PlayerStateSnapshot packet = {};
    packet.header.packetType = 6;
    packet.header.payloadDword0 = 0x2222;

    const int result = GameNet::SpawnRemotePlayerFromPkt06_PlayerStateSnapshot(
        (int)playerRecord.playerKey,
        &packet
    );
    const bool ok = result == 0 && g_GameNetPlayerRowHead == 0 &&
                    g_GameNetPlayerRowTail == 0 && g_GameNetPlayerRowCount == 0;

    g_HudUiTopMessageStack = oldTopStack;
    g_zNetwork_PlayerRecordList = oldPlayerRecordList;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;

    return ok ? 0 : 1;
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
