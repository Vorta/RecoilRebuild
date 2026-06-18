#include "Battlesport/GameNet.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/RecoilApp.h"
#include "Battlesport/player.h"
#include "Battlesport/pickup.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zModel/zModel.h"
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
int g_qsandRelayCallbackCount;
int g_qsandRelayCallbackResult;
int g_remoteHudSetVisibleCount;
int g_remoteHudLastVisible;

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

HRESULT __stdcall SendExFake(
    zNetwork_DPlay4 *,
    DWORD,
    DWORD,
    DWORD flags,
    void *packet,
    DWORD packetSizeBytes,
    DWORD,
    DWORD,
    void *,
    DWORD *asyncHandle
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
    if (asyncHandle != 0) {
        *asyncHandle = 0x2468;
    }
    return 0;
}

struct FakeDirectPlay4 {
    void **vtable;
};

struct ScoreboardPacket2 {
    zNetworkPacketHeader header;
    std::int32_t entryCount;
    NetPkt09_PlayerScoreboardEntry entries[2];
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
    vtable[49] = (void *)(&SendExFake);
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

template <typename Method> void *MethodAddress(
    Method method
) {
    union {
        Method method;
        void *address;
    } value = {method};
    return value.address;
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

int __fastcall QSandRelayCallbackFake(
    void *
) {
    ++g_qsandRelayCallbackCount;
    return g_qsandRelayCallbackResult;
}

struct TestRemoteHudPanelOps {
    void SetVisible(int visible) {
        ++g_remoteHudSetVisibleCount;
        g_remoteHudLastVisible = visible;
    }
};
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

extern "C" int gamenet_register_gameplay_handlers_and_callbacks_smoke(void) {
    zNetworkDispatchHandlerListNode *const oldSentinel = g_zNetwork_DispatchHandlerListSentinel;
    const int oldCount = g_zNetwork_DispatchHandlerListCount;
    const int oldRegistered = g_GameNet_HandlersRegistered;
    zDEClient_NetRelayCallback const oldCraterRelay = g_zDEClientCraterNetRelayCallback;
    zDEClient_NetRelayCallback const oldQSandRelay = g_zDEClientQSandNetRelayCallback;
    OptCatalogAllocRuntimeGateCallback const oldAllocGate =
        g_OptCatalog_AllocRuntimeGateCallback;
    OptCatalogAllocRuntimeGateCallback const oldNoOpGate =
        g_OptCatalog_AltGunDispatchNoOpCallback;
    OptCatalogRemoveRuntimeRelayCallback const oldRemoveRelay =
        g_OptCatalog_RemoveRuntimeRelayCallback;
    void(__fastcall *oldEffectDispatch)(
        zEffectAnimActivationRecord *record
    ) = g_zEffectAnim_ActivationDispatchCallback;
    const unsigned int oldEffectDispatchTag = g_zEffectAnim_ActivationDispatchTagHigh;

    zNetworkDispatchHandlerListNode sentinel = {};
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    g_zNetwork_DispatchHandlerListSentinel = &sentinel;
    g_zNetwork_DispatchHandlerListCount = 0;
    g_GameNet_HandlersRegistered = 0;
    g_zDEClientCraterNetRelayCallback = 0;
    g_zDEClientQSandNetRelayCallback = 0;
    g_OptCatalog_AllocRuntimeGateCallback = 0;
    g_OptCatalog_AltGunDispatchNoOpCallback = 0;
    g_OptCatalog_RemoveRuntimeRelayCallback = 0;
    g_zEffectAnim_ActivationDispatchCallback = 0;
    g_zEffectAnim_ActivationDispatchTagHigh = 0;

    GameNet::RegisterGameplayHandlersAndOptCatalogCallbacks();

    unsigned int packetMask = 0;
    bool modesOk = true;
    for (zNetworkDispatchHandlerListNode *node = sentinel.next; node != &sentinel;
         node = node->next) {
        if (node->record == 0 || node->record->mode != 2) {
            modesOk = false;
            break;
        }
        if (node->record->packetType >= 0 && node->record->packetType < 32) {
            packetMask |= 1u << (unsigned int)(node->record->packetType);
        }
    }

    const unsigned int expectedMask =
        (1u << 1) | (1u << 3) | (1u << 6) | (1u << 7) | (1u << 8) |
        (1u << 9) | (1u << 0x0a) | (1u << 0x0b) | (1u << 0x0c) |
        (1u << 0x0d) | (1u << 0x0e) | (1u << 0x0f) | (1u << 0x10) |
        (1u << 0x11) | (1u << 0x12) | (1u << 0x13) | (1u << 0x14);
    const bool registeredOk = g_GameNet_HandlersRegistered == 1 &&
                              g_zNetwork_DispatchHandlerListCount == 17 && modesOk &&
                              packetMask == expectedMask;
    const bool callbacksOk =
        g_zDEClientCraterNetRelayCallback ==
            (zDEClient_NetRelayCallback)&zDEClient_Crater::Execute &&
        g_zDEClientQSandNetRelayCallback ==
            (zDEClient_NetRelayCallback)&GameNet::SendPkt10_QSandEvent &&
        g_OptCatalog_AllocRuntimeGateCallback ==
            &OptCatalog::AltGunDispatchAllocRuntimeGateCallback &&
        g_OptCatalog_AltGunDispatchNoOpCallback == &GameNet::AltGunDispatchNoOpCallback &&
        g_OptCatalog_RemoveRuntimeRelayCallback == &OptCatalog::SendPkt0A_RemoveRuntimeRelay &&
        g_zEffectAnim_ActivationDispatchCallback ==
            &GameNet::SendPkt13_EffectAnimActivationRecord &&
        g_zEffectAnim_ActivationDispatchTagHigh == 0x0c000000u;

    GameNet::RegisterGameplayHandlersAndOptCatalogCallbacks();
    const bool noDuplicateOk = g_zNetwork_DispatchHandlerListCount == 17;

    ClearDispatchHandlerListForTest(sentinel);
    g_zNetwork_DispatchHandlerListSentinel = oldSentinel;
    g_zNetwork_DispatchHandlerListCount = oldCount;
    g_GameNet_HandlersRegistered = oldRegistered;
    g_zDEClientCraterNetRelayCallback = oldCraterRelay;
    g_zDEClientQSandNetRelayCallback = oldQSandRelay;
    g_OptCatalog_AllocRuntimeGateCallback = oldAllocGate;
    g_OptCatalog_AltGunDispatchNoOpCallback = oldNoOpGate;
    g_OptCatalog_RemoveRuntimeRelayCallback = oldRemoveRelay;
    g_zEffectAnim_ActivationDispatchCallback = oldEffectDispatch;
    g_zEffectAnim_ActivationDispatchTagHigh = oldEffectDispatchTag;

    return registeredOk && callbacksOk && noDuplicateOk ? 0 : 1;
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

extern "C" int gamenet_handle_pkt14_hud_timer_and_flags_sync_smoke(void) {
    CWnd *const oldMainWnd = g_RecoilApp.m_pMainWnd;
    const int oldCurrentStateIndex = g_RecoilApp.m_currentStateIndex;
    RecoilApp_StateQueue oldQueue = g_RecoilApp.m_stateQueue;
    RecoilApp_IState *oldStateStack[16];
    for (int index = 0; index < 16; ++index) {
        oldStateStack[index] = g_RecoilApp.m_stateStack[index];
    }

    const int oldMissionFmvMissionId = g_RecoilApp.m_missionFmvState.m_missionId;
    const int oldRuntimeGoalValue = g_HudSensorTracker.runtimeGoalValue;
    const int oldRuntimeTimerSecRaw = g_HudSensorTracker.runtimeTimerSecRaw;
    const int oldMissionId = g_HudSensorTracker.missionId;
    const int oldMissionFlags = g_HudSensorTracker.missionFlags;
    const int oldAllowMaps = g_GameNetStatus_AllowMaps;
    const int oldNameTags = g_GameNetStatus_NameTags;
    const int oldHandlersRegistered = g_GameNet_HandlersRegistered;
    zNetworkDispatchHandlerListNode *const oldSentinel = g_zNetwork_DispatchHandlerListSentinel;
    const int oldHandlerCount = g_zNetwork_DispatchHandlerListCount;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetworkDPlaySessionDescCache *const oldSession = g_zNetwork_CurrentSessionDescCache;
    const int oldIsHost = g_zNetwork_IsHostFlag;

    CZRecoilFrame mainWnd = {};
    mainWnd.m_useArchiveBanks = 77;
    g_RecoilApp.m_pMainWnd = (CWnd *)(&mainWnd);
    g_RecoilApp.m_currentStateIndex = -1;
    std::memset(
        g_RecoilApp.m_stateStack,
        0,
        sizeof(g_RecoilApp.m_stateStack)
    );
    std::memset(
        &g_RecoilApp.m_stateQueue,
        0,
        sizeof(g_RecoilApp.m_stateQueue)
    );
    g_RecoilApp.m_missionFmvState.m_missionId = 99;

    g_HudSensorTracker.runtimeGoalValue = 0;
    g_HudSensorTracker.runtimeTimerSecRaw = 0;
    g_HudSensorTracker.missionId = 0;
    g_HudSensorTracker.missionFlags = 0;
    g_GameNetStatus_AllowMaps = 0;
    g_GameNetStatus_NameTags = 0;
    g_GameNet_HandlersRegistered = 1;

    zNetworkDispatchHandlerListNode sentinel = {};
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    g_zNetwork_DispatchHandlerListSentinel = &sentinel;
    g_zNetwork_DispatchHandlerListCount = 0;

    void *vtable[52];
    InitDirectPlayVtable(vtable);
    FakeDirectPlay4 dplay = {vtable};
    zNetworkDPlaySessionDescCache session = {};
    char sessionName[0x5c] = "pkt14";
    session.desc.lpszSessionNameA = sessionName;
    session.desc.dwMaxPlayers = 8;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_CurrentSessionDescCache = &session;
    g_zNetwork_IsHostFlag = 1;
    g_setSessionDescCalls = 0;
    g_setSessionDescResult = 0;

    NetPkt14_HudTimerAndFlagsSync packet = {};
    packet.header.packetType = 0x14;
    packet.header.packetSizeBytes = sizeof(packet);
    packet.eventCode = 4;
    packet.auxParam = 12;
    packet.valueOrTime = 3;
    packet.statusFlags = 3;

    const int result = GameNet::HandlePkt14_HudTimerAndFlagsSync(
        0x2222,
        &packet
    );

    union TimerSecondsBits {
        float seconds;
        int raw;
    } expectedTimer = {180.0f};

    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue;
    bool queuedIntro = false;
    RecoilApp_StateQueueItem *queuedItem = 0;
    if (queue.m_itemCount == 1 && queue.m_writeBlock.m_cursor != 0) {
        RecoilApp_StateQueueItem **const slot = queue.m_writeBlock.m_cursor - 1;
        queuedItem = *slot;
        queuedIntro =
            queuedItem != 0 &&
            queuedItem->m_kind == RecoilApp_StateQueueKind_SwitchCurrent &&
            queuedItem->m_stateObj == &g_RecoilApp.m_introFmvState &&
            queuedItem->m_param == 0;
    }

    int failure = 0;
    if (result != 1) {
        failure = 1;
    } else if (g_GameNet_HandlersRegistered != 0) {
        failure = 2;
    } else if (g_HudSensorTracker.runtimeTimerSecRaw != expectedTimer.raw ||
               g_HudSensorTracker.runtimeGoalValue != 12) {
        failure = 3;
    } else if (g_HudSensorTracker.missionId != 10 ||
               g_HudSensorTracker.missionFlags != 77) {
        failure = 4;
    } else if (g_GameNetStatus_AllowMaps != 1 ||
               g_GameNetStatus_NameTags != 1) {
        failure = 5;
    } else if (g_RecoilApp.m_missionFmvState.m_missionId != 0 ||
               !queuedIntro) {
        failure = 6;
    } else if (g_setSessionDescCalls != 1 ||
               session.desc.dwUser1 != 4 ||
               session.desc.dwUser4 != 12 ||
               session.desc.dwUser3 != 3 ||
               session.desc.dwUser2 != 3) {
        failure = 7;
    } else if (session.desc.dwMaxPlayers != 8 ||
               std::strcmp(sessionName, "pkt14") != 0) {
        failure = 8;
    }

    if (queuedItem != 0) {
        ::operator delete(queuedItem);
    }
    if (queue.m_readBlock.m_chunkBegin != 0) {
        ::operator delete(queue.m_readBlock.m_chunkBegin);
    }
    if (queue.m_chunkBaseList != 0) {
        ::operator delete(queue.m_chunkBaseList);
    }

    g_RecoilApp.m_pMainWnd = oldMainWnd;
    g_RecoilApp.m_currentStateIndex = oldCurrentStateIndex;
    for (int index = 0; index < 16; ++index) {
        g_RecoilApp.m_stateStack[index] = oldStateStack[index];
    }
    g_RecoilApp.m_stateQueue = oldQueue;
    g_RecoilApp.m_missionFmvState.m_missionId = oldMissionFmvMissionId;
    g_HudSensorTracker.runtimeGoalValue = oldRuntimeGoalValue;
    g_HudSensorTracker.runtimeTimerSecRaw = oldRuntimeTimerSecRaw;
    g_HudSensorTracker.missionId = oldMissionId;
    g_HudSensorTracker.missionFlags = oldMissionFlags;
    g_GameNetStatus_AllowMaps = oldAllowMaps;
    g_GameNetStatus_NameTags = oldNameTags;
    g_GameNet_HandlersRegistered = oldHandlersRegistered;
    g_zNetwork_DispatchHandlerListSentinel = oldSentinel;
    g_zNetwork_DispatchHandlerListCount = oldHandlerCount;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_CurrentSessionDescCache = oldSession;
    g_zNetwork_IsHostFlag = oldIsHost;

    return failure;
}

extern "C" int gamenet_handle_pkt03_remove_remote_player_smoke(void) {
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    const GameNetPlayerRowListState oldRowList = g_GameNetPlayerRowList;
    GameNetPlayerRow *const oldRowHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldRowTail = g_GameNetPlayerRowTail;
    const unsigned int oldRowCount = g_GameNetPlayerRowCount;
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    OptCatalogRuntimeInstanceStorage *const oldFreeRuntimeList =
        g_OptCatalogFreeRuntimeInstanceList;

    HudUiTriplet triplet{};
    triplet.Constructor();
    HudUiStatsListElement statsList{};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    HudUiTopMessageStack topStack{};
    topStack.enabled = 0;
    g_HudUiTopMessageStack = &topStack;

    void *panelVtable[32] = {};
    panelVtable[24] = MethodAddress(&TestRemoteHudPanelOps::SetVisible);
    g_remoteHudSetVisibleCount = 0;
    g_remoteHudLastVisible = 7;

    GameNetPlayerRow first{};
    first.playerKey = 0x3101;
    std::strcpy(
        first.displayName,
        "First"
    );
    first.hudWidget.ConstructorDefault(
        "",
        0,
        0
    );
    *reinterpret_cast<void ***>(&first.hudWidget) = panelVtable;

    GameNetPlayerRow *const removed = new GameNetPlayerRow{};
    removed->playerKey = 0x3102;
    removed->playerColorPackedRgb = 0x00123456;
    std::strcpy(
        removed->displayName,
        "Removed"
    );
    removed->hudWidget.ConstructorDefault(
        "",
        0,
        0
    );
    *reinterpret_cast<void ***>(&removed->hudWidget) = panelVtable;
    first.next = removed;

    zUtil_SaveGameState saveState{};
    zUtil_PlayerStateStorage playerState{};
    saveState.playerState = &playerState;
    removed->saveState = (GameNetPlayerSaveState *)&saveState;
    playerState.lifecycleState = 3;
    playerState.cameraTransitionTimer = 0;
    playerState.activeAltGunController = &playerState.altWeaponBanks[2].controllerA;

    zClass_NodePartial rootNode{};
    playerState.rootNode = &rootNode;
    zClass_NodePartial runtimeWorld{};
    runtimeWorld.classId = 3;
    zClass_NodeFreeListSlot projectile{};
    zClass_Object3DDataPartial projectileData{};
    projectile.node.classId = 5;
    projectile.node.classData = &projectileData;
    zClass_NodePartial *worldChildren[1] = {&projectile.node};
    runtimeWorld.listB = worldChildren;
    runtimeWorld.listCountB = 1;

    OptCatalogEntryDef mineEntry{};
    OptCatalogRuntimeInstanceStorage mineRuntime{};
    mineRuntime.ownerNode = &rootNode;
    mineRuntime.projectileNode = &projectile.node;
    mineRuntime.lifetime = 0.0f;
    mineEntry.activeRuntimeListHead = &mineRuntime;
    playerState.altWeaponBanks[4].controllerA.optCatalogEntry = &mineEntry;

    OptCatalogRuntimeInstanceStorage freeSentinel{};
    g_OptCatalogRuntimeWorld = &runtimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = &freeSentinel;

    g_GameNetPlayerRowList.flags = 1;
    g_GameNetPlayerRowHead = &first;
    g_GameNetPlayerRowTail = removed;
    g_GameNetPlayerRowCount = 2;

    triplet.AddEntry(&first);
    triplet.AddEntry(removed);
    topStack.AddChild(reinterpret_cast<HudUiElement *>(&first.hudWidget));
    topStack.AddChild(reinterpret_cast<HudUiElement *>(&removed->hudWidget));

    const int result = GameNet::HandlePkt03_RemoveRemotePlayer(removed->playerKey, nullptr);

    const bool playerStateOk =
        result == 0 && playerState.cameraTransitionTimer == 1 && playerState.lifecycleState == 4;
    const bool mineOk = mineEntry.activeRuntimeListHead == nullptr &&
                        g_OptCatalogFreeRuntimeInstanceList == &mineRuntime &&
                       mineRuntime.next == &freeSentinel;
    const bool rowListOk = g_GameNetPlayerRowHead == &first && g_GameNetPlayerRowTail == &first &&
                           g_GameNetPlayerRowCount == 1 && first.next == nullptr;
    const bool hudOk = g_remoteHudSetVisibleCount == 1 && g_remoteHudLastVisible == 0 &&
                       topStack.childHead ==
                           reinterpret_cast<HudUiElement *>(&first.hudWidget) &&
                       topStack.childTail ==
                           reinterpret_cast<HudUiElement *>(&first.hudWidget) &&
                       triplet.entries.begin != nullptr &&
                       triplet.entries.end == triplet.entries.begin + 1 &&
                       triplet.entries.begin[0].playerKey == first.playerKey;

    g_HudUiMgrStatsList = oldStatsList;
    g_HudUiTopMessageStack = oldTopStack;
    g_GameNetPlayerRowList = oldRowList;
    g_GameNetPlayerRowHead = oldRowHead;
    g_GameNetPlayerRowTail = oldRowTail;
    g_GameNetPlayerRowCount = oldRowCount;
    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
    triplet.DestructorCore();

    if (!playerStateOk) {
        return 1;
    }
    if (!mineOk) {
        return 2;
    }
    if (!rowListOk) {
        return 3;
    }
    return hudOk ? 0 : 4;
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

extern "C" int gamenet_reassign_player_colors_smoke(void) {
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    zNetworkPlayerRecordList *const oldPlayerRecordList = g_zNetwork_PlayerRecordList;
    zNetworkDPlaySessionDescCache *const oldSession = g_zNetwork_CurrentSessionDescCache;

    zNetwork_PlayerRecord firstRecord = {};
    firstRecord.playerKey = 0x1111;
    firstRecord.colorIndex = 8;
    zNetwork_PlayerRecord secondRecord = {};
    secondRecord.playerKey = 0x2222;
    secondRecord.colorIndex = 2;

    zNetworkPlayerRecordListNode sentinel = {};
    zNetworkPlayerRecordListNode firstNode = {};
    zNetworkPlayerRecordListNode secondNode = {};
    sentinel.next = &firstNode;
    sentinel.prev = &secondNode;
    firstNode.next = &secondNode;
    firstNode.prev = &sentinel;
    firstNode.playerRecord = &firstRecord;
    secondNode.next = &sentinel;
    secondNode.prev = &firstNode;
    secondNode.playerRecord = &secondRecord;
    zNetworkPlayerRecordList playerList = {};
    playerList.sentinelNode = &sentinel;
    playerList.count = 2;
    g_zNetwork_PlayerRecordList = &playerList;

    zNetworkDPlaySessionDescCache session = {};
    session.desc.dwMaxPlayers = 8;
    g_zNetwork_CurrentSessionDescCache = &session;

    HudUiTriplet triplet = {};
    triplet.Constructor();
    HudUiStatsListElement statsList = {};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    zClass_Object3DDataPartial firstObject = {};
    zClass_NodePartial firstObjectNode = {};
    firstObjectNode.classId = 5;
    firstObjectNode.classData = &firstObject;
    PlayerModalState firstModal = {};
    firstModal.modalNode = &firstObjectNode;
    GameNetPlayerSaveState firstSave = {};
    firstSave.primaryModalState = &firstModal;

    zClass_Object3DDataPartial secondObject = {};
    zClass_NodePartial secondObjectNode = {};
    secondObjectNode.classId = 5;
    secondObjectNode.classData = &secondObject;
    PlayerModalState secondModal = {};
    secondModal.modalNode = &secondObjectNode;
    GameNetPlayerSaveState secondSave = {};
    secondSave.primaryModalState = &secondModal;

    GameNetPlayerRow *const firstRow = (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    GameNetPlayerRow *const secondRow = (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
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
    firstRow->playerKey = firstRecord.playerKey;
    firstRow->saveState = &firstSave;
    std::strcpy(firstRow->displayName, "First");
    secondRow->playerKey = secondRecord.playerKey;
    secondRow->saveState = &secondSave;
    std::strcpy(secondRow->displayName, "Second");
    firstRow->next = secondRow;
    g_GameNetPlayerRowHead = firstRow;
    g_GameNetPlayerRowTail = secondRow;
    g_GameNetPlayerRowCount = 2;

    GameNet::RefreshPlayerListMenu(firstRow);
    GameNet::RefreshPlayerListMenu(secondRow);
    const std::int32_t result = GameNet::ReassignPlayerColorsAndRefreshRows(0, 0);

    const HudUiScoreboardEntry *firstEntry = 0;
    const HudUiScoreboardEntry *secondEntry = 0;
    for (HudUiScoreboardEntry *entry = triplet.entries.begin; entry != triplet.entries.end;
         ++entry) {
        if (entry->playerKey == firstRow->playerKey) {
            firstEntry = entry;
        }
        if (entry->playerKey == secondRow->playerKey) {
            secondEntry = entry;
        }
    }

    const bool firstOk =
        firstRow->playerColorIndex == 8 && firstRow->playerColorPackedRgb == 0x000040ff &&
        FieldAt<std::uint32_t>(&firstRow->hudWidget, 0x14c) == 0x000040ff &&
        FieldAt<std::uint32_t>(&firstRow->hudWidget, 0x150) == 0x000040ff &&
        FieldAt<std::int32_t>(&firstRow->hudWidget, 0x270) == 1 && firstEntry != 0 &&
        firstEntry->playerColorPackedRgb == 0x000040ff && firstObject.color.red == 1.0f &&
        firstObject.color.green == 1.0f && firstObject.color.blue == 0.0f &&
        firstObject.colorAlpha == 0.2f;

    const bool secondOk =
        secondRow->playerColorIndex == 2 && secondRow->playerColorPackedRgb == 0x0000ff00 &&
        FieldAt<std::uint32_t>(&secondRow->hudWidget, 0x14c) == 0x0000ff00 &&
        FieldAt<std::uint32_t>(&secondRow->hudWidget, 0x150) == 0x0000ff00 &&
        FieldAt<std::int32_t>(&secondRow->hudWidget, 0x270) == 1 && secondEntry != 0 &&
        secondEntry->playerColorPackedRgb == 0x0000ff00 && secondObject.color.red == 0.0f &&
        secondObject.color.green == 1.0f && secondObject.color.blue == 0.0f &&
        secondObject.colorAlpha == 0.2f;

    g_HudUiMgrStatsList = oldStatsList;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_zNetwork_PlayerRecordList = oldPlayerRecordList;
    g_zNetwork_CurrentSessionDescCache = oldSession;
    triplet.DestructorCore();
    ::operator delete(firstRow);
    ::operator delete(secondRow);

    return result == 1 && firstOk && secondOk ? 0 : 1;
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

extern "C" int gamenet_scoreboard_snapshot_packet_smoke(void) {
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    const std::int32_t oldRaceMode = g_HudSensorTracker.raceCheckpointMode;
    const std::int32_t oldGoalValue = g_HudSensorTracker.runtimeGoalValue;
    const std::int32_t oldOneLapShown = g_GameNetOneLapLeftMessageShown;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const std::int32_t oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const std::int32_t oldIsHost = g_zNetwork_IsHostFlag;
    const std::int32_t oldTcpIpAsync = g_zNetwork_TcpIpAsyncSendEnabled;

    HudUiTriplet triplet;
    triplet.Constructor();
    HudUiStatsListElement statsList = {};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    GameNetPlayerRow *const alpha = (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    GameNetPlayerRow *const bravo = (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    std::memset(
        alpha,
        0,
        sizeof(*alpha)
    );
    std::memset(
        bravo,
        0,
        sizeof(*bravo)
    );

    alpha->playerKey = 0x101;
    alpha->playerColorPackedRgb = 0x00112233;
    std::strcpy(
        alpha->displayName,
        "Alpha"
    );

    bravo->playerKey = 0x202;
    bravo->playerColorPackedRgb = 0x00445566;
    std::strcpy(
        bravo->displayName,
        "Bravo"
    );
    alpha->next = bravo;

    g_GameNetPlayerRowHead = alpha;
    g_GameNetPlayerRowTail = bravo;
    g_GameNetPlayerRowCount = 2;
    g_HudSensorTracker.raceCheckpointMode = 0;
    g_HudSensorTracker.runtimeGoalValue = 999;
    g_GameNetOneLapLeftMessageShown = 0;
    g_zNetwork_IsHostFlag = 0;

    GameNet::RefreshPlayerListMenu(alpha);
    GameNet::RefreshPlayerListMenu(bravo);

    ScoreboardPacket2 packet = {};
    packet.header.packetType = 0x09;
    packet.header.packetSizeBytes = sizeof(packet);
    packet.entryCount = 2;
    packet.entries[0].playerKey = alpha->playerKey;
    packet.entries[0].packedScoreAndLapCount = (std::uint16_t)((3 << 9) | 17);
    packet.entries[1].playerKey = bravo->playerKey;
    packet.entries[1].packedScoreAndLapCount = (std::uint16_t)((4 << 9) | 22);

    const std::int32_t handleResult = GameNet::HandlePkt09_PlayerScoreboardSnapshot(
        0,
        (NetPkt09_PlayerScoreboardSnapshot *)(&packet)
    );
    const bool applied =
        handleResult == 1 && alpha->score == 17 && alpha->lapCount == 3 &&
        bravo->score == 22 && bravo->lapCount == 4 &&
        triplet.entries.begin[0].playerKey == bravo->playerKey &&
        triplet.entries.begin[1].playerKey == alpha->playerKey;

    void *vtable[52];
    InitDirectPlayVtable(vtable);
    FakeDirectPlay4 dplay = {vtable};
    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x4444;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x5678;
    g_zNetwork_IsHostFlag = 1;
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

    alpha->score = 33;
    alpha->lapCount = 7;
    bravo->score = 44;
    bravo->lapCount = 5;
    GameNet::SendPkt09_PlayerScoreboardSnapshot();

    const ScoreboardPacket2 *const sentPacket =
        (const ScoreboardPacket2 *)(g_sendPacketBytes);
    const bool sent =
        g_sendCalls == 1 && g_sendFlags == 1 && g_sendPacketSize == sizeof(ScoreboardPacket2) &&
        g_sendPacketBytesSize == sizeof(ScoreboardPacket2) &&
        sentPacket->header.packetType == 0x09 &&
        sentPacket->header.packetSizeBytes == sizeof(ScoreboardPacket2) &&
        sentPacket->header.payloadDword0 == 0x5678 && sentPacket->entryCount == 2 &&
        sentPacket->entries[0].playerKey == alpha->playerKey &&
        sentPacket->entries[0].packedScoreAndLapCount == (std::uint16_t)((7 << 9) | 33) &&
        sentPacket->entries[1].playerKey == bravo->playerKey &&
        sentPacket->entries[1].packedScoreAndLapCount == (std::uint16_t)((5 << 9) | 44);

    g_HudUiMgrStatsList = oldStatsList;
    g_HudSensorTracker.raceCheckpointMode = oldRaceMode;
    g_HudSensorTracker.runtimeGoalValue = oldGoalValue;
    g_GameNetOneLapLeftMessageShown = oldOneLapShown;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_TcpIpAsyncSendEnabled = oldTcpIpAsync;
    triplet.DestructorCore();
    ::operator delete(alpha);
    ::operator delete(bravo);

    return applied && sent ? 0 : 1;
}

extern "C" int gamenet_lap_progress_packet_smoke(void) {
    struct ScoreboardPacket1 {
        zNetworkPacketHeader header;
        std::int32_t entryCount;
        NetPkt09_PlayerScoreboardEntry entries[1];
    };

    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    const std::int32_t oldRaceMode = g_HudSensorTracker.raceCheckpointMode;
    const std::int32_t oldGoalValue = g_HudSensorTracker.runtimeGoalValue;
    const std::int32_t oldOneLapShown = g_GameNetOneLapLeftMessageShown;
    const std::int32_t oldLapTargetStarted = g_GameNetAllPlayersLapTargetCheckStarted;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const std::int32_t oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const std::int32_t oldIsHost = g_zNetwork_IsHostFlag;
    const std::int32_t oldTcpIpAsync = g_zNetwork_TcpIpAsyncSendEnabled;

    void *vtable[52];
    InitDirectPlayVtable(vtable);
    FakeDirectPlay4 dplay = {vtable};
    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x1111;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x2222;
    g_zNetwork_IsHostFlag = 0;
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

    zUtil_PlayerStateStorage playerState = {};
    playerState.lapCount = 4;
    playerState.lapTimeSec = 65.0f;
    zUtil_SaveGameState saveState = {};
    GameNetPlayerRow *const localRow =
        (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    std::memset(
        localRow,
        0,
        sizeof(*localRow)
    );
    saveState.playerState = &playerState;
    saveState.netPlayerRow = localRow;

    GameNet::SendPkt0E_PlayerLapProgress(&saveState);
    const NetPkt0E_PlayerLapProgress *const sentPacket =
        (const NetPkt0E_PlayerLapProgress *)(g_sendPacketBytes);
    const bool clientSend =
        g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacketSize == sizeof(NetPkt0E_PlayerLapProgress) &&
        g_sendPacketBytesSize == sizeof(NetPkt0E_PlayerLapProgress) &&
        sentPacket->header.packetType == 0x0e &&
        sentPacket->header.packetSizeBytes == sizeof(NetPkt0E_PlayerLapProgress) &&
        sentPacket->header.payloadDword0 == 0x2222 && sentPacket->lapCountPacked == 4 &&
        sentPacket->reserved_0a == 0 && sentPacket->lapTimeSec == 65.0f &&
        localRow->lapCount == 0;

    HudUiTriplet triplet;
    triplet.Constructor();
    HudUiStatsListElement statsList = {};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    GameNetPlayerRow *const remoteRow =
        (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    GameNetPlayerRow *const targetRow =
        (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    std::memset(
        remoteRow,
        0,
        sizeof(*remoteRow)
    );
    std::memset(
        targetRow,
        0,
        sizeof(*targetRow)
    );
    remoteRow->playerKey = 0x3333;
    remoteRow->playerColorPackedRgb = 0x00123456;
    std::strcpy(
        remoteRow->displayName,
        "Remote"
    );
    g_GameNetPlayerRowHead = remoteRow;
    g_GameNetPlayerRowTail = remoteRow;
    g_GameNetPlayerRowCount = 1;
    g_HudSensorTracker.raceCheckpointMode = 0;
    g_HudSensorTracker.runtimeGoalValue = 3;
    g_GameNetOneLapLeftMessageShown = 0;
    GameNet::RefreshPlayerListMenu(remoteRow);

    g_zNetwork_IsHostFlag = 1;
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

    NetPkt0E_PlayerLapProgress packet = {};
    packet.lapCountPacked = 2;
    packet.lapTimeSec = 44.0f;
    const std::int32_t handleResult = GameNet::HandlePkt0E_PlayerLapProgress(
        0x3333,
        &packet
    );
    const ScoreboardPacket1 *const scoreboardPacket =
        (const ScoreboardPacket1 *)(g_sendPacketBytes);
    const bool hostHandle =
        handleResult == 1 && remoteRow->lapCount == 2 && remoteRow->lapTimeSec == 44.0f &&
        g_sendCalls == 1 && g_sendFlags == 1 && g_sendPacketSize == sizeof(ScoreboardPacket1) &&
        g_sendPacketBytesSize == sizeof(ScoreboardPacket1) &&
        scoreboardPacket->header.packetType == 0x09 &&
        scoreboardPacket->header.packetSizeBytes == sizeof(ScoreboardPacket1) &&
        scoreboardPacket->entryCount == 1 &&
        scoreboardPacket->entries[0].playerKey == remoteRow->playerKey &&
        scoreboardPacket->entries[0].packedScoreAndLapCount == (std::uint16_t)(2 << 9);

    targetRow->lapCount = 3;
    remoteRow->next = targetRow;
    remoteRow->lapCount = 2;
    g_GameNetPlayerRowTail = targetRow;
    g_GameNetPlayerRowCount = 2;
    g_GameNetAllPlayersLapTargetCheckStarted = 0;
    const bool lapsBlocked =
        GameNet::AreAllPlayersAtLapTarget() == 0 &&
        g_GameNetAllPlayersLapTargetCheckStarted == 1;

    remoteRow->lapCount = 3;
    g_GameNetAllPlayersLapTargetCheckStarted = 0;
    const bool lapsReached =
        GameNet::AreAllPlayersAtLapTarget() == 1 &&
        g_GameNetAllPlayersLapTargetCheckStarted == 1;

    g_HudUiMgrStatsList = oldStatsList;
    g_HudSensorTracker.raceCheckpointMode = oldRaceMode;
    g_HudSensorTracker.runtimeGoalValue = oldGoalValue;
    g_GameNetOneLapLeftMessageShown = oldOneLapShown;
    g_GameNetAllPlayersLapTargetCheckStarted = oldLapTargetStarted;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_TcpIpAsyncSendEnabled = oldTcpIpAsync;
    triplet.DestructorCore();
    ::operator delete(targetRow);
    ::operator delete(remoteRow);
    ::operator delete(localRow);

    return clientSend && hostHandle && lapsBlocked && lapsReached ? 0 : 1;
}

extern "C" int gamenet_chat_message_packet_smoke(void) {
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const std::int32_t oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const std::int32_t oldTcpIpAsync = g_zNetwork_TcpIpAsyncSendEnabled;
    HudUiTextStack4 *const oldChatStack = g_HudUiChatMessageStack;

    HudUiChatMessageStack *const chatStack =
        (HudUiChatMessageStack *)(::operator new(sizeof(HudUiChatMessageStack)));
    chatStack->Constructor();
    chatStack->enabled = 1;
    g_HudUiChatMessageStack = chatStack;

    NetPkt0B_ChatMessage packet = {};
    packet.messageLength = 5;
    std::memcpy(
        packet.message,
        "hello",
        5
    );
    GameNet::HandlePkt0B_ChatMessage(
        0,
        &packet
    );

    HudUiPanel *const firstLine = &chatStack->lines[0];
    const bool shortMessage =
        std::strcmp(
            firstLine->cachedText,
            "hello"
        ) == 0 &&
        FieldAt<float>(
            firstLine,
            0x10
        ) == 5.0f;

    NetPkt0B_ChatMessage longPacket = {};
    longPacket.messageLength = 0x55;
    for (std::size_t index = 0; index < sizeof(longPacket.message); ++index) {
        longPacket.message[index] = (char)('A' + (index % 26));
    }

    GameNet::HandlePkt0B_ChatMessage(
        0,
        &longPacket
    );
    const bool clamped =
        std::strlen(firstLine->cachedText) == sizeof(longPacket.message) &&
        std::memcmp(
            firstLine->cachedText,
            longPacket.message,
            sizeof(longPacket.message)
        ) == 0 &&
        FieldAt<float>(
            firstLine,
            0x10
        ) == 5.0f;

    void *vtable[52];
    InitDirectPlayVtable(vtable);
    FakeDirectPlay4 dplay = {vtable};
    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x10203040;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x10203040;
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

    GameNet::SendPkt0B_ChatMessage("hello");

    const NetPkt0B_ChatMessage *const sentPacket =
        (const NetPkt0B_ChatMessage *)(g_sendPacketBytes);
    const bool sentShort =
        g_sendCalls == 1 && g_sendFlags == 1 && g_sendPacketSize == 17 &&
        g_sendPacketBytesSize == 17 && sentPacket->header.packetType == 0x0b &&
        sentPacket->header.packetSizeBytes == 17 &&
        sentPacket->header.payloadDword0 == 0x10203040 &&
        sentPacket->messageLength == 5 &&
        std::memcmp(
            sentPacket->message,
            "hello",
            5
        ) == 0 &&
        g_sendPacketBytes[15] == 0 && g_sendPacketBytes[16] == 0;

    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(
        g_sendPacketBytes,
        0xff,
        sizeof(g_sendPacketBytes)
    );

    GameNet::SendPkt0B_ChatMessage("");

    const NetPkt0B_ChatMessage *const emptyPacket =
        (const NetPkt0B_ChatMessage *)(g_sendPacketBytes);
    const bool sentEmpty =
        g_sendCalls == 1 && g_sendFlags == 1 && g_sendPacketSize == 12 &&
        g_sendPacketBytesSize == 12 && emptyPacket->header.packetType == 0x0b &&
        emptyPacket->header.packetSizeBytes == 12 &&
        emptyPacket->header.payloadDword0 == 0x10203040 &&
        emptyPacket->messageLength == 0 && g_sendPacketBytes[10] == 0 &&
        g_sendPacketBytes[11] == 0;

    g_HudUiChatMessageStack = oldChatStack;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = oldTcpIpAsync;
    chatStack->DestructorCore();
    ::operator delete(chatStack);

    return shortMessage && clamped && sentShort && sentEmpty ? 0 : 1;
}

extern "C" int gamenet_show_player_kill_message_smoke(void) {
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;

    HudUiTopMessageStack *const topStack =
        (HudUiTopMessageStack *)(::operator new(sizeof(HudUiTopMessageStack)));
    topStack->Constructor();
    topStack->enabled = 1;
    g_HudUiTopMessageStack = topStack;

    GameNetPlayerRow *const victim =
        (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    GameNetPlayerRow *const killer =
        (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    std::memset(
        victim,
        0,
        sizeof(*victim)
    );
    std::memset(
        killer,
        0,
        sizeof(*killer)
    );
    std::strcpy(
        victim->displayName,
        "Victim"
    );
    std::strcpy(
        killer->displayName,
        "Killer"
    );
    OptCatalogEntryDef killEntry = {};
    killEntry.killVerbString = (char *)("tagged");

    GameNet::ShowPlayerKillMessage(
        victim,
        &killEntry,
        killer
    );

    HudUiPanel *const firstLine = &topStack->lines[0];
    const bool ok =
        std::strcmp(
            firstLine->cachedText,
            "Victim tagged Killer"
        ) == 0 &&
        FieldAt<float>(
            firstLine,
            0x10
        ) == 2.0f;

    g_HudUiTopMessageStack = oldTopStack;
    topStack->DestructorCore();
    ::operator delete(topStack);
    ::operator delete(killer);
    ::operator delete(victim);
    return ok ? 0 : 1;
}

extern "C" int gamenet_player_kill_event_packet_smoke(void) {
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    const std::int32_t oldRaceMode = g_HudSensorTracker.raceCheckpointMode;
    const std::int32_t oldGoalValue = g_HudSensorTracker.runtimeGoalValue;
    const std::int32_t oldOneLapShown = g_GameNetOneLapLeftMessageShown;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    const std::int32_t oldOptCatalogEntryCount = g_OptCatalog_EntryCount;
    OptCatalogEntryDef *const oldOptCatalogEntryTable = g_OptCatalog_EntryTable;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const std::int32_t oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const std::int32_t oldIsHost = g_zNetwork_IsHostFlag;
    const std::int32_t oldTcpIpAsync = g_zNetwork_TcpIpAsyncSendEnabled;

    HudUiTopMessageStack *const topStack =
        (HudUiTopMessageStack *)(::operator new(sizeof(HudUiTopMessageStack)));
    topStack->Constructor();
    topStack->enabled = 1;
    g_HudUiTopMessageStack = topStack;

    HudUiTriplet triplet;
    triplet.Constructor();
    HudUiStatsListElement statsList = {};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    OptCatalogEntryDef killEntry = {};
    killEntry.keyName = (char *)("test_weapon");
    killEntry.ordinalIndex = 3;
    killEntry.killVerbString = (char *)("tagged");
    g_OptCatalog_EntryCount = 1;
    g_OptCatalog_EntryTable = &killEntry;

    GameNetPlayerRow *const killer =
        (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    GameNetPlayerRow *const victim =
        (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    std::memset(
        killer,
        0,
        sizeof(*killer)
    );
    std::memset(
        victim,
        0,
        sizeof(*victim)
    );

    killer->playerKey = 0x11;
    killer->lapCount = 1;
    killer->playerColorPackedRgb = 0x00112233;
    std::strcpy(
        killer->displayName,
        "Killer"
    );

    victim->playerKey = 0x22;
    victim->score = 4;
    victim->lapCount = 2;
    victim->playerColorPackedRgb = 0x00445566;
    std::strcpy(
        victim->displayName,
        "Victim"
    );
    killer->next = victim;

    g_GameNetPlayerRowHead = killer;
    g_GameNetPlayerRowTail = victim;
    g_GameNetPlayerRowCount = 2;
    g_HudSensorTracker.raceCheckpointMode = 0;
    g_HudSensorTracker.runtimeGoalValue = 999;
    g_GameNetOneLapLeftMessageShown = 0;
    GameNet::RefreshPlayerListMenu(killer);
    GameNet::RefreshPlayerListMenu(victim);

    NetPkt08_PlayerKillEvent packet = {};
    packet.killMethodOrOptCatalogEntryId = 3;
    packet.targetPlayerKey = victim->playerKey;
    g_zNetwork_IsHostFlag = 0;
    g_sendCalls = 0;

    const std::int32_t nonHostResult = GameNet::HandlePkt08_PlayerKillEvent(
        killer->playerKey,
        &packet
    );
    HudUiPanel *const firstLine = &topStack->lines[0];
    const bool nonHost =
        nonHostResult == 1 && victim->score == 4 && g_sendCalls == 0 &&
        std::strcmp(
            firstLine->cachedText,
            "Victim tagged Killer"
        ) == 0;

    packet.targetPlayerKey = 0x7777;
    const bool missingRow = GameNet::HandlePkt08_PlayerKillEvent(
        killer->playerKey,
        &packet
    ) == 0;

    void *vtable[52];
    InitDirectPlayVtable(vtable);
    FakeDirectPlay4 dplay = {vtable};
    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x4444;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x5678;
    g_zNetwork_IsHostFlag = 1;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;

    packet.targetPlayerKey = victim->playerKey;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(
        g_sendPacketBytes,
        0,
        sizeof(g_sendPacketBytes)
    );
    const std::int32_t hostResult = GameNet::HandlePkt08_PlayerKillEvent(
        killer->playerKey,
        &packet
    );
    const ScoreboardPacket2 *const hostSentPacket =
        (const ScoreboardPacket2 *)(g_sendPacketBytes);
    const bool host =
        hostResult == 1 && victim->score == 5 && g_sendCalls == 1 &&
        g_sendPacketSize == sizeof(ScoreboardPacket2) &&
        hostSentPacket->entries[1].playerKey == victim->playerKey &&
        hostSentPacket->entries[1].packedScoreAndLapCount ==
            (std::uint16_t)((victim->lapCount << 9) | 5);

    packet.targetPlayerKey = killer->playerKey;
    killer->score = 0;
    g_sendCalls = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(
        g_sendPacketBytes,
        0,
        sizeof(g_sendPacketBytes)
    );
    const std::int32_t suicideResult = GameNet::HandlePkt08_PlayerKillEvent(
        killer->playerKey,
        &packet
    );
    const ScoreboardPacket2 *const suicideSentPacket =
        (const ScoreboardPacket2 *)(g_sendPacketBytes);
    const bool suicide =
        suicideResult == 1 && killer->score == 0 && g_sendCalls == 1 &&
        suicideSentPacket->entries[0].playerKey == killer->playerKey &&
        suicideSentPacket->entries[0].packedScoreAndLapCount ==
            (std::uint16_t)(killer->lapCount << 9);

    zUtil_SaveGameState saveState = {};
    saveState.netPlayerRow = victim;
    g_zNetwork_LocalPlayerKey = killer->playerKey;
    g_zNetwork_IsHostFlag = 0;
    g_sendCalls = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(
        g_sendPacketBytes,
        0,
        sizeof(g_sendPacketBytes)
    );
    GameNet::SendPkt08_PlayerKillEvent(
        &saveState,
        3
    );
    const NetPkt08_PlayerKillEvent *const sentKillPacket =
        (const NetPkt08_PlayerKillEvent *)(g_sendPacketBytes);
    const bool explicitSaveStateSend =
        g_sendCalls == 1 && g_sendPacketSize == sizeof(NetPkt08_PlayerKillEvent) &&
        sentKillPacket->header.packetType == 0x08 &&
        sentKillPacket->header.packetSizeBytes == sizeof(NetPkt08_PlayerKillEvent) &&
        sentKillPacket->header.payloadDword0 == killer->playerKey &&
        sentKillPacket->killMethodOrOptCatalogEntryId == 3 &&
        sentKillPacket->targetPlayerKey == victim->playerKey;

    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)(&saveState);
    g_sendCalls = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(
        g_sendPacketBytes,
        0,
        sizeof(g_sendPacketBytes)
    );
    GameNet::SendPkt08_PlayerKillEvent(
        0,
        3
    );
    const NetPkt08_PlayerKillEvent *const fallbackKillPacket =
        (const NetPkt08_PlayerKillEvent *)(g_sendPacketBytes);
    const bool fallbackSaveStateSend =
        g_sendCalls == 1 && fallbackKillPacket->header.payloadDword0 == killer->playerKey &&
        fallbackKillPacket->targetPlayerKey == victim->playerKey;

    g_HudUiTopMessageStack = oldTopStack;
    g_HudUiMgrStatsList = oldStatsList;
    g_HudSensorTracker.raceCheckpointMode = oldRaceMode;
    g_HudSensorTracker.runtimeGoalValue = oldGoalValue;
    g_GameNetOneLapLeftMessageShown = oldOneLapShown;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_OptCatalog_EntryCount = oldOptCatalogEntryCount;
    g_OptCatalog_EntryTable = oldOptCatalogEntryTable;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_TcpIpAsyncSendEnabled = oldTcpIpAsync;
    topStack->DestructorCore();
    ::operator delete(topStack);
    triplet.DestructorCore();
    ::operator delete(victim);
    ::operator delete(killer);

    return nonHost && missingRow && host && suicide && explicitSaveStateSend &&
                   fallbackSaveStateSend
               ? 0
               : 1;
}

extern "C" int gamenet_send_pkt13_effect_anim_activation_record_smoke(void) {
    void *vtable[52];
    InitDirectPlayVtable(vtable);
    FakeDirectPlay4 dplay = {vtable};
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    const int oldSuppressEcho = g_GameNetSuppressPkt13ActivationEcho;

    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x12345678;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x12345678;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_GameNetSuppressPkt13ActivationEcho = 0;
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

    zEffectAnimActivationRecord record = {};
    std::memset(
        &record,
        0xab,
        sizeof(record)
    );
    record.commandType = 2;
    GameNet::SendPkt13_EffectAnimActivationRecord(&record);

    const zNetworkPacketHeader *const header =
        (const zNetworkPacketHeader *)(g_sendPacketBytes);
    const bool sentOk =
        g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacketSize == sizeof(zNetworkPacketHeader) + 0x48 &&
        header->packetType == 0x13 &&
        header->packetSizeBytes == sizeof(zNetworkPacketHeader) + 0x48 &&
        header->payloadDword0 == 0x12345678 &&
        std::memcmp(
            g_sendPacketBytes + sizeof(zNetworkPacketHeader),
            &record,
            0x48
        ) == 0;

    g_GameNetSuppressPkt13ActivationEcho = 1;
    g_sendCalls = 0;
    GameNet::SendPkt13_EffectAnimActivationRecord(&record);
    const bool suppressOk = g_sendCalls == 0;

    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_GameNetSuppressPkt13ActivationEcho = oldSuppressEcho;

    return sentOk && suppressOk ? 0 : 1;
}

extern "C" int gamenet_handle_pkt13_effect_anim_activation_record_smoke(void) {
    struct Packet13 {
        zNetworkPacketHeader header;
        zEffectAnimActivationRecord record;
    };

    zEffectAnimActivationRecord *const oldRecordTable = g_zEffectAnim_ActivationRecordTable;
    const int oldRecordCount = g_zEffectAnim_ActivationRecordCount;
    const int oldSuppressEcho = g_GameNetSuppressPkt13ActivationEcho;

    Packet13 packet = {};
    packet.header.packetType = 0x13;
    packet.header.packetSizeBytes = sizeof(Packet13);
    packet.record.commandType = 2;
    std::strcpy(
        packet.record.animName,
        "missing_pkt13_activation"
    );
    packet.record.nodeToken = 77;

    g_zEffectAnim_ActivationRecordTable = 0;
    g_zEffectAnim_ActivationRecordCount = 0;
    g_GameNetSuppressPkt13ActivationEcho = 0;
    const int missingResult = GameNet::HandlePkt13_EffectAnimActivationRecord(
        0,
        &packet.header
    );
    const bool missingOk = missingResult == 1 && g_GameNetSuppressPkt13ActivationEcho == 0;

    g_zEffectAnim_ActivationRecordTable = &packet.record;
    g_zEffectAnim_ActivationRecordCount = 1;
    const int duplicateResult = GameNet::HandlePkt13_EffectAnimActivationRecord(
        0,
        &packet.header
    );
    const bool duplicateOk = duplicateResult == 1 && g_GameNetSuppressPkt13ActivationEcho == 0;

    g_zEffectAnim_ActivationRecordTable = oldRecordTable;
    g_zEffectAnim_ActivationRecordCount = oldRecordCount;
    g_GameNetSuppressPkt13ActivationEcho = oldSuppressEcho;

    return missingOk && duplicateOk ? 0 : 1;
}

extern "C" int gamenet_send_all_pkt13_effect_anim_activation_records_smoke(void) {
    void *vtable[52];
    InitDirectPlayVtable(vtable);
    FakeDirectPlay4 dplay = {vtable};
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldIsHost = g_zNetwork_IsHostFlag;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    const int oldSuppressEcho = g_GameNetSuppressPkt13ActivationEcho;
    zEffectAnimActivationRecord *const oldRecordTable = g_zEffectAnim_ActivationRecordTable;
    const int oldRecordCount = g_zEffectAnim_ActivationRecordCount;

    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x13572468;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = localPlayer.playerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_GameNetSuppressPkt13ActivationEcho = 0;

    zEffectAnimActivationRecord records[2] = {};
    std::memset(
        &records[0],
        0x11,
        sizeof(records[0])
    );
    std::memset(
        &records[1],
        0x22,
        sizeof(records[1])
    );
    records[0].commandType = 1;
    records[1].commandType = 3;
    g_zEffectAnim_ActivationRecordTable = records;
    g_zEffectAnim_ActivationRecordCount = 2;

    g_zNetwork_IsHostFlag = 0;
    g_sendCalls = 0;
    GameNet::SendAllPkt13_EffectAnimActivationRecords();
    const bool nonHostOk = g_sendCalls == 0;

    g_zNetwork_IsHostFlag = 1;
    g_sendCalls = 0;
    g_sendPacketSize = 0;
    std::memset(
        g_sendPacketBytes,
        0,
        sizeof(g_sendPacketBytes)
    );
    GameNet::SendAllPkt13_EffectAnimActivationRecords();
    const zNetworkPacketHeader *const header =
        (const zNetworkPacketHeader *)(g_sendPacketBytes);
    const bool hostOk =
        g_sendCalls == 2 &&
        g_sendPacketSize == sizeof(zNetworkPacketHeader) + 0x4c &&
        header->packetType == 0x13 &&
        header->payloadDword0 == localPlayer.playerKey &&
        std::memcmp(
            g_sendPacketBytes + sizeof(zNetworkPacketHeader),
            &records[1],
            0x4c
        ) == 0;

    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_GameNetSuppressPkt13ActivationEcho = oldSuppressEcho;
    g_zEffectAnim_ActivationRecordTable = oldRecordTable;
    g_zEffectAnim_ActivationRecordCount = oldRecordCount;

    return nonHostOk && hostOk ? 0 : 1;
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

extern "C" int gamenet_handle_pkt07_alt_gun_dispatch_smoke(void) {
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    int *const oldPendingSpawnTargetCountPtr = g_OptCatalogPendingSpawnTargetCountPtr;
    PlayerProgressTargetSlotRuntime *const oldPendingSpawnTargetListPtr =
        g_OptCatalogPendingSpawnTargetListPtr;
    const std::int32_t oldEntryCount = g_OptCatalog_EntryCount;
    OptCatalogEntryDef *const oldEntryTable = g_OptCatalog_EntryTable;

    g_GameNetPlayerRowHead = nullptr;
    g_GameNetPlayerRowTail = nullptr;
    g_GameNetPlayerRowCount = 0;

    NetPkt07_AltGunDispatch missingPacket = {};
    missingPacket.header.payloadDword0 = 0x4040;
    const int missingResult = GameNet::HandlePkt07_AltGunDispatch(0x1111, &missingPacket);

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    PlayerGunFireController originalController = {};
    PlayerGunFireController targetController = {};
    playerState.activeAltGunController = &originalController;
    playerState.altGunFireHeldFlag = 1;
    playerState.worldPos = {2.0f, 3.0f, 4.0f};
    playerState.steerBasisRaw = {0.0f, 0.0f, 1.0f};

    OptCatalogEntryDef entry = {};
    entry.keyName = const_cast<char *>("pkt07-alt-gun");
    entry.ordinalIndex = 707;
    g_OptCatalog_EntryCount = 1;
    g_OptCatalog_EntryTable = &entry;
    targetController.optCatalogEntry = &entry;
    playerState.altWeaponBanks[4].controllerA = targetController;

    GameNetPlayerRow *const row = (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    std::memset(row, 0, sizeof(*row));
    row->playerKey = 0x3030;
    row->saveState = (GameNetPlayerSaveState *)(&saveState);
    g_GameNetPlayerRowHead = row;
    g_GameNetPlayerRowTail = row;
    g_GameNetPlayerRowCount = 1;

    NetPkt07_AltGunDispatch packet = {};
    packet.header.payloadDword0 = row->playerKey;
    packet.weaponId = 707;
    packet.dispatchFlags = 0x1234;
    packet.targetPos = {10.0f, 11.0f, 12.0f};
    g_OptCatalogPendingSpawnTargetCountPtr = (int *)(0x11112222);
    g_OptCatalogPendingSpawnTargetListPtr =
        (PlayerProgressTargetSlotRuntime *)(0x33334444);

    const int handledResult = GameNet::HandlePkt07_AltGunDispatch(0x1111, &packet);
    int failure = 0;
    if (missingResult != 0) {
        failure = 1;
    } else if (handledResult != 1) {
        failure = 2;
    } else if (playerState.altGunDispatchFlags != 0) {
        failure = 3;
    } else if (playerState.activeAltGunController != &originalController) {
        failure = 4;
    } else if (!Vec3Equals(playerState.storedTargetPos, packet.targetPos)) {
        failure = 5;
    } else if (!Vec3Equals(playerState.altFireOrigin, {2.0f, 4.0f, 4.0f})) {
        failure = 6;
    } else if (g_OptCatalogPendingSpawnTargetCountPtr != nullptr) {
        failure = 7;
    } else if (g_OptCatalogPendingSpawnTargetListPtr != nullptr) {
        failure = 8;
    }

    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_OptCatalogPendingSpawnTargetCountPtr = oldPendingSpawnTargetCountPtr;
    g_OptCatalogPendingSpawnTargetListPtr = oldPendingSpawnTargetListPtr;
    g_OptCatalog_EntryCount = oldEntryCount;
    g_OptCatalog_EntryTable = oldEntryTable;
    ::operator delete(row);

    return failure;
}

extern "C" int gamenet_send_pkt07_alt_gun_dispatch_smoke(void) {
    const NetPkt07_AltGunDispatch oldPacket = g_NetPkt07_AltGunDispatchBuf;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    zUtil_PlayerStateStorage playerState = {};
    playerState.storedTargetPos = {9.0f, 8.0f, 7.0f};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;

    void *vtable[52];
    InitDirectPlayVtable(vtable);
    FakeDirectPlay4 dplay = {vtable};
    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x1234;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x55667788;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_NetPkt07_AltGunDispatchBuf = {{0x07, sizeof(NetPkt07_AltGunDispatch), 0},
                                    0,
                                    0,
                                    0,
                                    {0.0f, 0.0f, 0.0f}};
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    GameNet::SendPkt07_AltGunDispatch(static_cast<short>(0x8123), 0x01000001u);

    const NetPkt07_AltGunDispatch *const sentPacket =
        reinterpret_cast<const NetPkt07_AltGunDispatch *>(g_sendPacketBytes);
    const bool ok =
        g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacket == &g_NetPkt07_AltGunDispatchBuf.header &&
        g_sendPacketSize == sizeof(NetPkt07_AltGunDispatch) &&
        sentPacket->header.packetType == 7 &&
        sentPacket->header.packetSizeBytes == sizeof(NetPkt07_AltGunDispatch) &&
        sentPacket->header.payloadDword0 == 0x55667788 &&
        sentPacket->weaponId == static_cast<short>(0x8123) &&
        sentPacket->dispatchFlags == 0x01000001u &&
        Vec3Equals(sentPacket->targetPos, playerState.storedTargetPos);

    g_NetPkt07_AltGunDispatchBuf = oldPacket;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;

    return ok ? 0 : 1;
}

extern "C" int gamenet_alt_gun_dispatch_no_op_callback_smoke(void) {
    OptCatalogEntryDef entry = {};
    void *saveStateSlot = nullptr;
    return GameNet::AltGunDispatchNoOpCallback(&entry, &saveStateSlot) == 1 ? 0 : 1;
}

extern "C" int optcatalog_alt_gun_dispatch_alloc_runtime_gate_smoke(void) {
    const NetPkt07_AltGunDispatch oldPacket = g_NetPkt07_AltGunDispatchBuf;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    OptCatalogEntryDef passEntry = {};
    passEntry.ordinalIndex = 0;
    void *passSlot = reinterpret_cast<void *>(0x11223344u);
    const int passZeroResult =
        OptCatalog::AltGunDispatchAllocRuntimeGateCallback(&passEntry, &passSlot);
    passEntry.ordinalIndex = 1;
    const int passOneResult =
        OptCatalog::AltGunDispatchAllocRuntimeGateCallback(&passEntry, &passSlot);
    const bool passOk = passZeroResult == 1 && passOneResult == 1 &&
                        passSlot == reinterpret_cast<void *>(0x11223344u);

    OptCatalogEntryDef entry = {};
    entry.ordinalIndex = 0x8123;
    void *nullSlot = nullptr;
    const bool nullOk =
        OptCatalog::AltGunDispatchAllocRuntimeGateCallback(&entry, &nullSlot) == 0 &&
        nullSlot == nullptr;

    zUtil_PlayerStateStorage localPlayerState = {};
    localPlayerState.storedTargetPos = {1.0f, 2.0f, 3.0f};
    zUtil_SaveGameState localSaveState = {};
    localSaveState.playerState = &localPlayerState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&localSaveState;

    void *vtable[52];
    InitDirectPlayVtable(vtable);
    FakeDirectPlay4 dplay = {vtable};
    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x2468;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x13572468;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_NetPkt07_AltGunDispatchBuf = {{0x07, sizeof(NetPkt07_AltGunDispatch), 0}, 0, 0,
                                    0, {0.0f, 0.0f, 0.0f}};
    g_sendCalls = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    void *localSlot = &localSaveState;
    const int localResult =
        OptCatalog::AltGunDispatchAllocRuntimeGateCallback(&entry, &localSlot);
    const NetPkt07_AltGunDispatch *const sentPacket =
        reinterpret_cast<const NetPkt07_AltGunDispatch *>(g_sendPacketBytes);
    const bool localOk =
        localResult == 1 && localSlot == reinterpret_cast<void *>(0x01000000u) &&
        g_sendCalls == 1 && sentPacket->header.payloadDword0 == 0x13572468 &&
        sentPacket->weaponId == static_cast<short>(0x8123) &&
        sentPacket->dispatchFlags == 0 &&
        Vec3Equals(sentPacket->targetPos, localPlayerState.storedTargetPos);

    zUtil_PlayerStateStorage remotePlayerState = {};
    remotePlayerState.altGunDispatchFlags = 0x01000001;
    zUtil_SaveGameState remoteSaveState = {};
    remoteSaveState.playerState = &remotePlayerState;
    void *remoteSlot = &remoteSaveState;
    const bool remoteRejectedOk =
        OptCatalog::AltGunDispatchAllocRuntimeGateCallback(&entry, &remoteSlot) == 0 &&
        remoteSlot == &remoteSaveState;

    remotePlayerState.altGunDispatchFlags = 0x02000012;
    remoteSlot = &remoteSaveState;
    const bool remoteAcceptedOk =
        OptCatalog::AltGunDispatchAllocRuntimeGateCallback(&entry, &remoteSlot) == 1 &&
        remoteSlot == reinterpret_cast<void *>(0x02000012u);

    g_NetPkt07_AltGunDispatchBuf = oldPacket;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;

    if (!passOk) {
        return 1;
    }
    if (!nullOk) {
        return 2;
    }
    if (!localOk) {
        return 3;
    }
    if (!remoteRejectedOk) {
        return 4;
    }
    return remoteAcceptedOk ? 0 : 5;
}

extern "C" int optcatalog_handle_pkt0a_remove_runtime_relay_smoke(void) {
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    const std::int32_t oldEntryCount = g_OptCatalog_EntryCount;
    OptCatalogEntryDef *const oldEntryTable = g_OptCatalog_EntryTable;
    const int oldRelayEnabled = g_OptCatalogProcessRuntimeRelayEnabled;

    static NetPkt0A_RemoveRuntimeRelay packet;
    std::memset(&packet, 0, sizeof(packet));
    packet.ownerPlayerKey = 0x9090;
    const int missingResult = OptCatalog::HandlePkt0A_RemoveRuntimeRelay(0x1111, &packet);

    static zClass_NodePartial ownerRoot;
    std::memset(&ownerRoot, 0, sizeof(ownerRoot));
    static zUtil_PlayerStateStorage playerState;
    std::memset(&playerState, 0, sizeof(playerState));
    playerState.rootNode = &ownerRoot;
    static zUtil_SaveGameState saveState;
    std::memset(&saveState, 0, sizeof(saveState));
    saveState.playerState = &playerState;

    static GameNetPlayerRow row;
    std::memset(&row, 0, sizeof(row));
    row.playerKey = packet.ownerPlayerKey;
    row.saveState = (GameNetPlayerSaveState *)(&saveState);
    g_GameNetPlayerRowHead = &row;
    g_GameNetPlayerRowTail = &row;
    g_GameNetPlayerRowCount = 1;

    static OptCatalogEntryDef entry;
    std::memset(&entry, 0, sizeof(entry));
    entry.ordinalIndex = 303;
    g_OptCatalog_EntryCount = 1;
    g_OptCatalog_EntryTable = &entry;
    g_OptCatalogProcessRuntimeRelayEnabled = 1;

    packet.optCatalogEntryId = 303;
    packet.pointOrVec3 = {0.0f, 0.0f, 0.0f};
    const int handledZeroResult = OptCatalog::HandlePkt0A_RemoveRuntimeRelay(0x1111, &packet);
    const bool zeroOk = handledZeroResult == 1 && g_OptCatalogProcessRuntimeRelayEnabled == 1;

    packet.pointOrVec3 = {1.0f, 0.0f, 0.0f};
    const int handledPointResult = OptCatalog::HandlePkt0A_RemoveRuntimeRelay(0x1111, &packet);
    const bool pointOk = handledPointResult == 1 && g_OptCatalogProcessRuntimeRelayEnabled == 1;

    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_OptCatalog_EntryCount = oldEntryCount;
    g_OptCatalog_EntryTable = oldEntryTable;
    g_OptCatalogProcessRuntimeRelayEnabled = oldRelayEnabled;

    return missingResult == 0 && zeroOk && pointOk ? 0 : 1;
}

extern "C" int optcatalog_send_pkt0a_remove_runtime_relay_smoke(void) {
    const NetPkt0A_RemoveRuntimeRelay oldPacket = g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf;
    const int oldRelayEnabled = g_OptCatalogProcessRuntimeRelayEnabled;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;

    OptCatalogEntryDef entry = {};
    entry.ordinalIndex = 0x4567;

    g_OptCatalogProcessRuntimeRelayEnabled = 0;
    g_sendCalls = 0;
    OptCatalog::SendPkt0A_RemoveRuntimeRelay(&entry, nullptr, nullptr);
    const bool disabledOk = g_sendCalls == 0;

    static void *vtable[52];
    InitDirectPlayVtable(vtable);
    static FakeDirectPlay4 dplay;
    dplay.vtable = vtable;
    static zNetwork_PlayerRecord localPlayer;
    std::memset(&localPlayer, 0, sizeof(localPlayer));
    localPlayer.playerKey = 0x1111;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x12345678;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_OptCatalogProcessRuntimeRelayEnabled = 1;

    static zClass_NodePartial ownerNode;
    std::memset(&ownerNode, 0, sizeof(ownerNode));
    g_sendCalls = 0;
    OptCatalog::SendPkt0A_RemoveRuntimeRelay(&entry, nullptr, &ownerNode);
    const bool missingContextOk = g_sendCalls == 0;

    static GameNetPlayerRow ownerRow;
    std::memset(&ownerRow, 0, sizeof(ownerRow));
    ownerRow.playerKey = 0x2468;
    static zUtil_SaveGameState ownerSaveState;
    std::memset(&ownerSaveState, 0, sizeof(ownerSaveState));
    ownerSaveState.netPlayerRow = &ownerRow;
    static HudUiMgrSensorTrackNode trackNode;
    std::memset(&trackNode, 0, sizeof(trackNode));
    trackNode.payload = &ownerSaveState;
    ownerNode.callbackContext = (zClass_NodePartial *)&trackNode;

    g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf =
        {{0x0a, sizeof(NetPkt0A_RemoveRuntimeRelay), 0},
         0,
         0,
         {9.0f, 9.0f, 9.0f},
         0};
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    static zVec3 point;
    point = {4.0f, 5.0f, 6.0f};
    OptCatalog::SendPkt0A_RemoveRuntimeRelay(&entry, &point, &ownerNode);
    const NetPkt0A_RemoveRuntimeRelay *const pointPacket =
        reinterpret_cast<const NetPkt0A_RemoveRuntimeRelay *>(g_sendPacketBytes);
    const bool pointOk =
        g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacket == &g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.header &&
        g_sendPacketSize == sizeof(NetPkt0A_RemoveRuntimeRelay) &&
        pointPacket->header.payloadDword0 == 0x12345678 &&
        pointPacket->optCatalogEntryId == static_cast<short>(0x4567) &&
        Vec3Equals(pointPacket->pointOrVec3, point) &&
        pointPacket->ownerPlayerKey == ownerRow.playerKey;

    g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.pointOrVec3 = {9.0f, 9.0f, 9.0f};
    g_sendCalls = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    OptCatalog::SendPkt0A_RemoveRuntimeRelay(&entry, nullptr, &ownerNode);
    const NetPkt0A_RemoveRuntimeRelay *const zeroPacket =
        reinterpret_cast<const NetPkt0A_RemoveRuntimeRelay *>(g_sendPacketBytes);
    const bool zeroOk =
        g_sendCalls == 1 && zeroPacket->pointOrVec3.x == 0.0f &&
        zeroPacket->pointOrVec3.y == 0.0f && zeroPacket->pointOrVec3.z == 0.0f &&
        zeroPacket->ownerPlayerKey == ownerRow.playerKey;

    g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf = oldPacket;
    g_OptCatalogProcessRuntimeRelayEnabled = oldRelayEnabled;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;

    if (!disabledOk) {
        return 1;
    }
    if (!missingContextOk) {
        return 2;
    }
    if (!pointOk) {
        return 3;
    }
    return zeroOk ? 0 : 4;
}

extern "C" int gamenet_host_send_pkt10_qsand_feature_smoke(void) {
    const NetPkt10_QSandEvent oldPacket = g_NetPkt10_QSandEventSendBuf;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldIsHost = g_zNetwork_IsHostFlag;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;

    zDEClient_QSandEventTemplate eventTemplate = {};
    eventTemplate.radius = 12.5f;
    eventTemplate.center = {7.0f, 8.0f, 9.0f};

    g_zNetwork_IsHostFlag = 0;
    g_sendCalls = 0;
    const int nonHostResult = GameNet::HostSendPkt10_QSandFeature(&eventTemplate);
    const bool nonHostOk = nonHostResult == 0 && g_sendCalls == 0;

    static void *vtable[52];
    InitDirectPlayVtable(vtable);
    static FakeDirectPlay4 dplay;
    dplay.vtable = vtable;
    static zNetwork_PlayerRecord localPlayer;
    std::memset(&localPlayer, 0, sizeof(localPlayer));
    localPlayer.playerKey = 0x5555;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x12345678;
    g_zNetwork_IsHostFlag = 1;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_NetPkt10_QSandEventSendBuf =
        {{0x10, sizeof(NetPkt10_QSandEvent), 0}, 0x12u, 0, {0.0f, 0.0f, 0.0f}, 0.0f};
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    const int hostResult = GameNet::HostSendPkt10_QSandFeature(&eventTemplate);
    const NetPkt10_QSandEvent *const sentPacket =
        reinterpret_cast<const NetPkt10_QSandEvent *>(g_sendPacketBytes);
    const bool hostOk =
        hostResult == 1 && g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacket == &g_NetPkt10_QSandEventSendBuf.header &&
        g_sendPacketSize == sizeof(NetPkt10_QSandEvent) &&
        g_sendPacketBytesSize == sizeof(NetPkt10_QSandEvent) &&
        sentPacket->header.packetType == 0x10 &&
        sentPacket->header.packetSizeBytes == sizeof(NetPkt10_QSandEvent) &&
        sentPacket->header.payloadDword0 == 0x12345678 &&
        sentPacket->eventFlags == (0x12u | 0x80u) &&
        Vec3Equals(sentPacket->center, eventTemplate.center) &&
        FloatNear(sentPacket->radius, eventTemplate.radius);

    g_NetPkt10_QSandEventSendBuf = oldPacket;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;

    return nonHostOk && hostOk ? 0 : 1;
}

extern "C" int gamenet_send_pkt10_qsand_event_smoke(void) {
    const NetPkt10_QSandEvent oldPacket = g_NetPkt10_QSandEventSendBuf;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldIsHost = g_zNetwork_IsHostFlag;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zDEClient_NetRelayCallback const oldRelayCallback = g_zDEClientQSandNetRelayCallback;
    const zDEClient_QSandEventTemplate oldDefaults = g_zDEClient_QuickSandEventTemplateDefaults;

    zClass_NodePartial ownerRoot = {};
    zClass_NodePartial otherRoot = {};
    zUtil_PlayerStateStorage playerState = {};
    playerState.rootNode = &ownerRoot;
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)(&saveState);

    static void *vtable[52];
    InitDirectPlayVtable(vtable);
    static FakeDirectPlay4 dplay;
    dplay.vtable = vtable;
    static zNetwork_PlayerRecord localPlayer;
    std::memset(&localPlayer, 0, sizeof(localPlayer));
    localPlayer.playerKey = 0x13572468;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = localPlayer.playerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_zDEClientQSandNetRelayCallback = QSandRelayCallbackFake;
    g_zDEClient_QuickSandEventTemplateDefaults = {};
    g_zDEClient_QuickSandEventTemplateDefaults.pointCount = 4;

    zDEClient_QSandEventTemplate negativeEvent = {};
    negativeEvent.radius = -2.25f;
    const int negativeResult = GameNet::SendPkt10_QSandEvent(&negativeEvent);
    const bool negativeOk =
        negativeResult == 1 && FloatNear(negativeEvent.radius, 2.25f);

    g_NetPkt10_QSandEventSendBuf =
        {{0x10, sizeof(NetPkt10_QSandEvent), 0}, 0x12345678u, 0,
         {0.0f, 0.0f, 0.0f}, 0.0f};
    g_sendCalls = 0;
    zDEClient_QSandEventTemplate otherOwnerEvent = {};
    otherOwnerEvent.radius = 5.0f;
    otherOwnerEvent.center = {1.0f, 2.0f, 3.0f};
    otherOwnerEvent.damageOwnerNode = &otherRoot;
    const int otherOwnerResult = GameNet::SendPkt10_QSandEvent(&otherOwnerEvent);
    const bool otherOwnerOk =
        otherOwnerResult == 0 && g_sendCalls == 0 &&
        g_NetPkt10_QSandEventSendBuf.header.payloadDword0 == 0 &&
        g_NetPkt10_QSandEventSendBuf.eventFlags == 0x12345678u;

    zDEClient_QSandEventTemplate nonHostEvent = {};
    nonHostEvent.radius = 6.5f;
    nonHostEvent.center = {4.0f, 5.0f, 6.0f};
    nonHostEvent.damageOwnerNode = &ownerRoot;
    g_zNetwork_IsHostFlag = 0;
    g_NetPkt10_QSandEventSendBuf =
        {{0x10, sizeof(NetPkt10_QSandEvent), 0}, 0x12345678u, 0,
         {0.0f, 0.0f, 0.0f}, 0.0f};
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    const int nonHostResult = GameNet::SendPkt10_QSandEvent(&nonHostEvent);
    const NetPkt10_QSandEvent *const sentPacket =
        (const NetPkt10_QSandEvent *)(g_sendPacketBytes);
    const bool nonHostOk =
        nonHostResult == 0 && g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacket == &g_NetPkt10_QSandEventSendBuf.header &&
        g_sendPacketSize == sizeof(NetPkt10_QSandEvent) &&
        sentPacket->header.payloadDword0 == localPlayer.playerKey &&
        sentPacket->eventFlags == 0x12340000u &&
        Vec3Equals(sentPacket->center, nonHostEvent.center) &&
        FloatNear(sentPacket->radius, 6.5f);

    g_zNetwork_IsHostFlag = 1;
    g_qsandRelayCallbackCount = 0;
    g_qsandRelayCallbackResult = 0;
    g_sendCalls = 0;
    zDEClient_QSandEventTemplate hostEvent = nonHostEvent;
    hostEvent.radius = 7.75f;
    g_NetPkt10_QSandEventSendBuf =
        {{0x10, sizeof(NetPkt10_QSandEvent), 0}, 0x87654321u, 0,
         {0.0f, 0.0f, 0.0f}, 0.0f};
    const int hostResult = GameNet::SendPkt10_QSandEvent(&hostEvent);
    const bool hostOk =
        hostResult == 0 && g_qsandRelayCallbackCount == 1 && g_sendCalls == 0 &&
        g_NetPkt10_QSandEventSendBuf.header.payloadDword0 == localPlayer.playerKey &&
        g_NetPkt10_QSandEventSendBuf.eventFlags == 0x87650000u &&
        Vec3Equals(g_NetPkt10_QSandEventSendBuf.center, hostEvent.center) &&
        FloatNear(g_NetPkt10_QSandEventSendBuf.radius, 7.75f);

    g_NetPkt10_QSandEventSendBuf = oldPacket;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_zDEClientQSandNetRelayCallback = oldRelayCallback;
    g_zDEClient_QuickSandEventTemplateDefaults = oldDefaults;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;

    if (!negativeOk) {
        return 1;
    }
    if (!otherOwnerOk) {
        return 2;
    }
    if (!nonHostOk) {
        return 3;
    }
    return hostOk ? 0 : 4;
}

extern "C" int gamenet_host_send_pkt0f_crater_feature_smoke(void) {
    const NetPkt0F_CraterEvent oldPacket = g_NetPkt0F_CraterEventSendBuf;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldIsHost = g_zNetwork_IsHostFlag;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    zModel_MaterialSlot *const oldMatlPool = g_zModel_MatlPool;
    const int oldMatlCapacity = g_zModel_MatlPoolCapacity;
    const int oldMatlInUse = g_zModel_MatlPoolInUseCount;

    static zModel_MaterialSlot materialSlots[4];
    std::memset(materialSlots, 0, sizeof(materialSlots));
    g_zModel_MatlPool = materialSlots;
    g_zModel_MatlPoolCapacity = 4;
    g_zModel_MatlPoolInUseCount = 4;

    zDEClient_CraterEventTemplate eventTemplate = {};
    eventTemplate.craterMaterialSlot = &materialSlots[2];
    eventTemplate.radius = 6.25f;
    eventTemplate.center = {3.0f, 4.0f, 5.0f};

    g_zNetwork_IsHostFlag = 0;
    g_sendCalls = 0;
    const int nonHostResult = GameNet::HostSendPkt0F_CraterFeature(&eventTemplate);
    const bool nonHostOk = nonHostResult == 0 && g_sendCalls == 0;

    static void *vtable[52];
    InitDirectPlayVtable(vtable);
    static FakeDirectPlay4 dplay;
    dplay.vtable = vtable;
    static zNetwork_PlayerRecord localPlayer;
    std::memset(&localPlayer, 0, sizeof(localPlayer));
    localPlayer.playerKey = 0x7777;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x23456789;
    g_zNetwork_IsHostFlag = 1;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_NetPkt0F_CraterEventSendBuf =
        {{0x0f, sizeof(NetPkt0F_CraterEvent), 0}, 0x21u, -1, {0.0f, 0.0f, 0.0f}, 0.0f};
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    const int hostResult = GameNet::HostSendPkt0F_CraterFeature(&eventTemplate);
    const NetPkt0F_CraterEvent *const sentPacket =
        reinterpret_cast<const NetPkt0F_CraterEvent *>(g_sendPacketBytes);
    const bool hostOk =
        hostResult == 1 && g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacket == &g_NetPkt0F_CraterEventSendBuf.header &&
        g_sendPacketSize == sizeof(NetPkt0F_CraterEvent) &&
        g_sendPacketBytesSize == sizeof(NetPkt0F_CraterEvent) &&
        sentPacket->header.packetType == 0x0f &&
        sentPacket->header.packetSizeBytes == sizeof(NetPkt0F_CraterEvent) &&
        sentPacket->header.payloadDword0 == 0x23456789 &&
        sentPacket->eventFlags == (0x21u | 0x80u) &&
        sentPacket->craterTypeId == 2 && Vec3Equals(sentPacket->center, eventTemplate.center) &&
        FloatNear(sentPacket->radius, eventTemplate.radius);

    g_NetPkt0F_CraterEventSendBuf = oldPacket;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_zModel_MatlPool = oldMatlPool;
    g_zModel_MatlPoolCapacity = oldMatlCapacity;
    g_zModel_MatlPoolInUseCount = oldMatlInUse;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;

    return nonHostOk && hostOk ? 0 : 1;
}
