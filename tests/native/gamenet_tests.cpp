#include "Battlesport/GameNet.h"
#include "Battlesport/HudSensorTracker.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zUtil/zSaveGame.h"

#include <cstring>

namespace {
int g_setSessionDescCalls;
std::int32_t g_setSessionDescResult;
int g_sendCalls;
std::uint32_t g_sendFlags;
void *g_sendPacket;
std::uint32_t g_sendPacketSize;
std::uint32_t g_sendPacketBytesSize;
unsigned char g_sendPacketBytes[0x200];

struct ScoreboardPacket2 {
    zNetworkPacketHeader header;
    std::int32_t entryCount;
    NetPkt09_PlayerScoreboardEntry entries[2];
};

template <typename T> T &FieldAt(void *object, std::size_t offset) {
    return *reinterpret_cast<T *>(static_cast<unsigned char *>(object) + offset);
}

void DeleteTopMessageStackFonts(HudUiTopMessageStack &stack) {
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = reinterpret_cast<HudUiPanel *>(&stack.lines[index][0]);
        DeleteObject(panel->hFont);
        panel->hFont = nullptr;
    }
}

std::int32_t RECOIL_STDCALL SetSessionDescFake(zNetwork_DPlay4 *, zNetworkDPlaySessionDesc *,
                                               std::uint32_t) {
    ++g_setSessionDescCalls;
    return g_setSessionDescResult;
}

std::int32_t RECOIL_STDCALL SendFake(zNetwork_DPlay4 *, std::uint32_t, std::uint32_t,
                                     std::uint32_t flags, void *packet,
                                     std::uint32_t packetSizeBytes) {
    ++g_sendCalls;
    g_sendFlags = flags;
    g_sendPacket = packet;
    g_sendPacketSize = packetSizeBytes;
    g_sendPacketBytesSize = packetSizeBytes;
    if (packetSizeBytes <= sizeof(g_sendPacketBytes)) {
        std::memcpy(g_sendPacketBytes, packet, packetSizeBytes);
    }
    return 0;
}

const zNetwork_DPlay4Vtable kDPlayVtable = {
    {}, nullptr, {}, SendFake, {}, SetSessionDescFake, {}, nullptr, {}, nullptr,
};
} // namespace

extern "C" int gamenet_list_reset_smoke(void) {
    g_GameNetSpawnPointList = 1;
    g_GameNetSpawnPointHead = reinterpret_cast<GameNetSpawnPoint *>(2);
    g_GameNetSpawnPointTail = reinterpret_cast<GameNetSpawnPoint *>(3);
    g_GameNetSpawnPointCount = 4;
    GameNetSpawnPointList::InitGlobals();
    if (g_GameNetSpawnPointList != 0 || g_GameNetSpawnPointHead != nullptr ||
        g_GameNetSpawnPointTail != nullptr || g_GameNetSpawnPointCount != 0) {
        return 1;
    }

    g_GameNetPlayerRowList = 5;
    g_GameNetPlayerRowHead = reinterpret_cast<GameNetPlayerRow *>(6);
    g_GameNetPlayerRowTail = reinterpret_cast<GameNetPlayerRow *>(7);
    g_GameNetPlayerRowCount = 8;
    GameNetPlayerRowList::Reset();
    return g_GameNetPlayerRowList == 0 && g_GameNetPlayerRowHead == nullptr &&
                   g_GameNetPlayerRowTail == nullptr && g_GameNetPlayerRowCount == 0
               ? 0
               : 2;
}

extern "C" int hud_timer_panel_net_state_clear_tail_flags_smoke(void) {
    HudTimerPanelNetState state{};
    for (std::uint32_t &flag : state.tailFlags) {
        flag = 0xffffffff;
    }

    state.ClearTailFlagsLocal();
    for (std::uint32_t flag : state.tailFlags) {
        if (flag != 0) {
            return 1;
        }
    }

    return 0;
}

extern "C" int gamenet_find_player_row_and_status_bits_smoke(void) {
    GameNetPlayerRow first{};
    GameNetPlayerRow second{};
    first.playerKey = 10;
    first.lapCount = 3;
    first.next = &second;
    second.playerKey = 20;
    second.lapCount = 4;
    g_GameNetPlayerRowHead = &first;
    g_HudSensorTracker.runtimeGoalValue = 3;

    const bool rowLookup =
        GameNet::FindPlayerRowByKey(20) == &second && GameNet::FindPlayerRowByKey(30) == nullptr;
    const bool lapsReached =
        GameNet::AreAllPlayersAtLapTarget() == 1 && g_GameNetAllPlayersLapTargetCheckStarted == 1;
    g_GameNetAllPlayersLapTargetCheckStarted = 0;
    second.lapCount = 2;
    const bool lapsBlocked =
        GameNet::AreAllPlayersAtLapTarget() == 0 && g_GameNetAllPlayersLapTargetCheckStarted == 1;
    g_GameNetAllPlayersLapTargetCheckStarted = 0;
    g_GameNetPlayerRowHead = nullptr;
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

    g_GameNetPlayerRowHead = nullptr;
    return rowLookup && lapsReached && lapsBlocked && emptyListReached && bothSet && bothClear ? 0
                                                                                               : 1;
}

extern "C" int gamenet_reset_hud_timer_panel_net_state_smoke(void) {
    HudUiTimerPanel timer{};
    HudUiPanel *const panel = reinterpret_cast<HudUiPanel *>(&timer);
    panel->ConstructorDefault("", 0, 0);
    g_HudUiMgrTimerPanel = &timer;

    g_HudTimerPanelNetState.timerSeconds = 1.0f;
    g_HudTimerPanelNetState.timeWarningThresholdSec = 2.0f;
    g_HudTimerPanelNetState.timerDirectionNeg = 0;
    g_HudTimerPanelNetState.startGateTriggered = 1;
    g_HudTimerPanelNetState.raceFinishCountdownTriggered = 1;
    g_HudTimerPanelNetState.startCountdownTriggered = 1;
    g_HudTimerPanelNetState.tenSecondWarningsEnabled = 1;
    for (std::uint32_t &flag : g_HudTimerPanelNetState.tailFlags) {
        flag = 0xffffffff;
    }
    g_GameNetOneLapLeftMessageShown = 1;
    g_GameNetAllPlayersLapTargetCheckStarted = 1;

    GameNet::ResetHudTimerPanelNetStateLongCountdown();

    bool tailsCleared = true;
    for (std::uint32_t flag : g_HudTimerPanelNetState.tailFlags) {
        tailsCleared = tailsCleared && flag == 0;
    }

    const bool ok = g_HudTimerPanelNetState.timerSeconds == 36000.0f &&
                    g_HudTimerPanelNetState.timeWarningThresholdSec == 120.0f &&
                    g_HudTimerPanelNetState.timerDirectionNeg == 1 &&
                    g_HudTimerPanelNetState.startGateTriggered == 0 &&
                    g_HudTimerPanelNetState.raceFinishCountdownTriggered == 0 &&
                    g_HudTimerPanelNetState.startCountdownTriggered == 0 &&
                    g_HudTimerPanelNetState.tenSecondWarningsEnabled == 0 &&
                    g_GameNetOneLapLeftMessageShown == 0 &&
                    g_GameNetAllPlayersLapTargetCheckStarted == 0 && tailsCleared &&
                    FieldAt<float>(&timer, 0x2a4) == 36000.0f &&
                    FieldAt<std::int32_t>(&timer, 0x2ac) == -1 &&
                    std::strcmp(&FieldAt<char>(&timer, 0x34), "10:00:00") == 0;

    DeleteObject(panel->hFont);
    panel->hFont = nullptr;
    g_HudUiMgrTimerPanel = nullptr;
    return ok ? 0 : 1;
}

extern "C" int gamenet_host_update_session_status_fields_smoke(void) {
    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetworkDPlaySessionDescCache session{};
    char sessionName[0x5c] = "mission";
    session.desc.sessionName = sessionName;
    session.desc.maxPlayers = 8;
    session.desc.customEventCode = 1;
    session.desc.customStatusFlags = 2;
    session.desc.customValueOrTime = 3;
    session.desc.customAuxParam = 4;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_CurrentSessionDescCache = &session;
    g_zNetwork_IsHostFlag = 0;
    g_setSessionDescCalls = 0;
    g_setSessionDescResult = 0;

    if (GameNet::HostUpdateSessionDescStatusFields(10, 13, 12, 11) != 0 ||
        g_setSessionDescCalls != 0 || session.desc.customEventCode != 1) {
        return 1;
    }

    g_zNetwork_IsHostFlag = 1;
    if (GameNet::HostUpdateSessionDescStatusFields(10, 13, 12, 11) != 1 ||
        g_setSessionDescCalls != 1 || session.desc.customEventCode != 10 ||
        session.desc.customStatusFlags != 11 || session.desc.customValueOrTime != 12 ||
        session.desc.customAuxParam != 13 || session.desc.maxPlayers != 8 ||
        std::strcmp(sessionName, "mission") != 0) {
        return 2;
    }

    g_setSessionDescResult = static_cast<std::int32_t>(0x88770014);
    if (GameNet::HostUpdateSessionDescStatusFields(20, 23, 22, 21) != 0 ||
        session.desc.customEventCode != 20) {
        return 3;
    }

    g_zNetwork_IsHostFlag = 0;
    g_zNetwork_CurrentSessionDescCache = nullptr;
    g_zNetwork_pDirectPlay4 = nullptr;
    return 0;
}

extern "C" int gamenet_timer_status_packet_smoke(void) {
    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x1234;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x5678;
    g_zNetwork_IsHostFlag = 1;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;

    HudUiTimerPanel timer{};
    HudUiPanel *const panel = reinterpret_cast<HudUiPanel *>(&timer);
    panel->ConstructorDefault("", 0, 0);
    g_HudUiMgrTimerPanel = &timer;
    HudUiTimerPanel::SetSeconds(42.0f, -1.0f);

    g_Time_AccumulatedTimeSec = 12.0f;
    g_HudTimerPanelNetState = {};
    g_HudTimerPanelNetState.timerDirectionNeg = 1;
    GameNet::SendPkt0C_HudTimerStatusBits(&g_HudTimerPanelNetState);

    const bool sent = g_sendCalls == 1 && g_sendFlags == 1 &&
                      g_sendPacket == &g_NetPkt0C_HudTimerStatusBitsBuf &&
                      g_sendPacketSize == sizeof(NetPkt0C_HudTimerStatusBits) &&
                      g_NetPkt0C_HudTimerStatusBitsBuf.header.packetType == 0x0c &&
                      g_NetPkt0C_HudTimerStatusBitsBuf.header.packetSizeBytes ==
                          sizeof(NetPkt0C_HudTimerStatusBits) &&
                      g_NetPkt0C_HudTimerStatusBitsBuf.header.payloadDword0 == 0x5678 &&
                      g_NetPkt0C_HudTimerStatusBitsBuf.timerSeconds == 42.0f &&
                      g_NetPkt0C_HudTimerStatusBitsBuf.statusBitsPackedHiWord == 1;

    const bool applied = g_HudTimerPanelNetState.timerSeconds == 42.0f &&
                         g_HudTimerPanelNetState.timerDirectionNeg == 1 &&
                         g_HudTimerPanelNetState.statusBitsResendDeadline == 42.0f &&
                         FieldAt<float>(&timer, 0x2a4) == 42.0f &&
                         FieldAt<std::int32_t>(&timer, 0x2ac) == -1;

    NetPkt0C_HudTimerStatusBits packet = {};
    packet.timerSeconds = 7.0f;
    packet.statusBitsPackedHiWord = 0;
    GameNet::HandlePkt0C_HudTimerStatusBits(0, &packet);
    const bool directApply = g_HudTimerPanelNetState.timerSeconds == 7.0f &&
                             g_HudTimerPanelNetState.timerDirectionNeg == 0 &&
                             FieldAt<float>(&timer, 0x2a4) == 7.0f &&
                             FieldAt<std::int32_t>(&timer, 0x2ac) == 1;

    DeleteObject(panel->hFont);
    panel->hFont = nullptr;
    g_HudUiMgrTimerPanel = nullptr;
    g_zNetwork_pDirectPlay4 = nullptr;
    g_zNetwork_LocalPlayerRecord = nullptr;
    g_zNetwork_LocalPlayerKey = 0;
    g_zNetwork_IsHostFlag = 0;
    g_Time_AccumulatedTimeSec = 0.0f;
    return sent && applied && directApply ? 0 : 1;
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

    HudUiTriplet triplet{};
    triplet.Constructor();
    HudUiStatsListElement statsList{};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    GameNetPlayerRow alpha{};
    alpha.playerKey = 0x101;
    alpha.playerColorPackedRgb = 0x00112233;
    std::strcpy(alpha.displayName, "Alpha");

    GameNetPlayerRow bravo{};
    bravo.playerKey = 0x202;
    bravo.playerColorPackedRgb = 0x00445566;
    std::strcpy(bravo.displayName, "Bravo");
    alpha.next = &bravo;

    g_GameNetPlayerRowHead = &alpha;
    g_GameNetPlayerRowTail = &bravo;
    g_GameNetPlayerRowCount = 2;
    g_HudSensorTracker.raceCheckpointMode = 0;
    g_HudSensorTracker.runtimeGoalValue = 999;
    g_GameNetOneLapLeftMessageShown = 0;
    g_zNetwork_IsHostFlag = 0;

    GameNet::RefreshPlayerListMenu(&alpha);
    GameNet::RefreshPlayerListMenu(&bravo);

    ScoreboardPacket2 packet{};
    packet.header.packetType = 0x09;
    packet.header.packetSizeBytes = sizeof(packet);
    packet.entryCount = 2;
    packet.entries[0].playerKey = alpha.playerKey;
    packet.entries[0].packedScoreAndLapCount = static_cast<std::uint16_t>((3 << 9) | 17);
    packet.entries[1].playerKey = bravo.playerKey;
    packet.entries[1].packedScoreAndLapCount = static_cast<std::uint16_t>((4 << 9) | 22);

    const std::int32_t handleResult = GameNet::HandlePkt09_PlayerScoreboardSnapshot(
        0, reinterpret_cast<NetPkt09_PlayerScoreboardSnapshot *>(&packet));
    const bool applied = handleResult == 1 && alpha.score == 17 && alpha.lapCount == 3 &&
                         bravo.score == 22 && bravo.lapCount == 4 &&
                         triplet.entries.begin[0].playerKey == bravo.playerKey &&
                         triplet.entries.begin[1].playerKey == alpha.playerKey;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x4444;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x5678;
    g_zNetwork_IsHostFlag = 1;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    alpha.score = 33;
    alpha.lapCount = 7;
    bravo.score = 44;
    bravo.lapCount = 5;
    GameNet::SendPkt09_PlayerScoreboardSnapshot();

    const ScoreboardPacket2 *const sentPacket =
        reinterpret_cast<const ScoreboardPacket2 *>(g_sendPacketBytes);
    const bool sent =
        g_sendCalls == 1 && g_sendFlags == 1 && g_sendPacketSize == sizeof(ScoreboardPacket2) &&
        g_sendPacketBytesSize == sizeof(ScoreboardPacket2) &&
        sentPacket->header.packetType == 0x09 &&
        sentPacket->header.packetSizeBytes == sizeof(ScoreboardPacket2) &&
        sentPacket->header.payloadDword0 == 0x5678 && sentPacket->entryCount == 2 &&
        sentPacket->entries[0].playerKey == alpha.playerKey &&
        sentPacket->entries[0].packedScoreAndLapCount ==
            static_cast<std::uint16_t>((7 << 9) | 33) &&
        sentPacket->entries[1].playerKey == bravo.playerKey &&
        sentPacket->entries[1].packedScoreAndLapCount == static_cast<std::uint16_t>((5 << 9) | 44);

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

    return applied && sent ? 0 : 1;
}

extern "C" int gamenet_chat_message_packet_smoke(void) {
    HudUiChatMessageStack chat{};
    chat.Constructor();
    chat.base.enabled = 1;
    g_HudUiChatMessageStack = &chat;

    NetPkt0B_ChatMessage packet = {};
    packet.messageLength = 5;
    std::memcpy(packet.message, "hello", 5);
    GameNet::HandlePkt0B_ChatMessage(0, &packet);

    HudUiPanel *const firstLine = reinterpret_cast<HudUiPanel *>(&chat.lines[0][0]);
    const bool shortMessage = std::strcmp(firstLine->GetLastTextPtr(), "hello") == 0 &&
                              FieldAt<float>(firstLine, 0x10) == 5.0f;

    NetPkt0B_ChatMessage longPacket = {};
    longPacket.messageLength = 0x55;
    for (std::size_t index = 0; index < sizeof(longPacket.message); ++index) {
        longPacket.message[index] = static_cast<char>('A' + (index % 26));
    }

    GameNet::HandlePkt0B_ChatMessage(0, &longPacket);
    const char *const text = firstLine->GetLastTextPtr();
    const bool clamped = std::strlen(text) == sizeof(longPacket.message) &&
                         std::memcmp(text, longPacket.message, sizeof(longPacket.message)) == 0;

    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = reinterpret_cast<HudUiPanel *>(&chat.lines[index][0]);
        DeleteObject(panel->hFont);
        panel->hFont = nullptr;
    }

    g_HudUiChatMessageStack = nullptr;
    return shortMessage && clamped ? 0 : 1;
}

extern "C" int gamenet_show_player_kill_message_smoke(void) {
    HudUiTopMessageStack top{};
    top.Constructor();
    top.base.enabled = 1;
    g_HudUiTopMessageStack = &top;

    GameNetPlayerRow victim{};
    std::strcpy(victim.displayName, "Victim");
    GameNetPlayerRow killer{};
    std::strcpy(killer.displayName, "Killer");
    OptCatalogEntryDef killEntry{};
    killEntry.killVerbString = const_cast<char *>("tagged");

    GameNet::ShowPlayerKillMessage(&victim, &killEntry, &killer);

    HudUiPanel *const firstLine = reinterpret_cast<HudUiPanel *>(&top.lines[0][0]);
    const bool ok = std::strcmp(firstLine->GetLastTextPtr(), "Victim tagged Killer") == 0 &&
                    FieldAt<float>(firstLine, 0x10) == 2.0f;

    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = reinterpret_cast<HudUiPanel *>(&top.lines[index][0]);
        DeleteObject(panel->hFont);
        panel->hFont = nullptr;
    }

    g_HudUiTopMessageStack = nullptr;
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

    HudUiTopMessageStack top{};
    top.Constructor();
    top.base.enabled = 1;
    g_HudUiTopMessageStack = &top;

    HudUiTriplet triplet{};
    triplet.Constructor();
    HudUiStatsListElement statsList{};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    OptCatalogEntryDef killEntry{};
    killEntry.keyName = const_cast<char *>("test_weapon");
    killEntry.ordinalIndex = 3;
    killEntry.killVerbString = const_cast<char *>("tagged");
    g_OptCatalog_EntryCount = 1;
    g_OptCatalog_EntryTable = &killEntry;

    GameNetPlayerRow killer{};
    killer.playerKey = 0x11;
    killer.lapCount = 1;
    killer.playerColorPackedRgb = 0x00112233;
    std::strcpy(killer.displayName, "Killer");

    GameNetPlayerRow victim{};
    victim.playerKey = 0x22;
    victim.score = 4;
    victim.lapCount = 2;
    victim.playerColorPackedRgb = 0x00445566;
    std::strcpy(victim.displayName, "Victim");
    killer.next = &victim;

    g_GameNetPlayerRowHead = &killer;
    g_GameNetPlayerRowTail = &victim;
    g_GameNetPlayerRowCount = 2;
    g_HudSensorTracker.raceCheckpointMode = 0;
    g_HudSensorTracker.runtimeGoalValue = 999;
    g_GameNetOneLapLeftMessageShown = 0;
    GameNet::RefreshPlayerListMenu(&killer);
    GameNet::RefreshPlayerListMenu(&victim);

    NetPkt08_PlayerKillEvent packet{};
    packet.killMethodOrOptCatalogEntryId = 3;
    packet.targetPlayerKey = victim.playerKey;
    g_zNetwork_IsHostFlag = 0;
    g_sendCalls = 0;

    const std::int32_t nonHostResult =
        GameNet::HandlePkt08_PlayerKillEvent(killer.playerKey, &packet);
    HudUiPanel *const firstLine = reinterpret_cast<HudUiPanel *>(&top.lines[0][0]);
    const bool nonHost = nonHostResult == 1 && victim.score == 4 && g_sendCalls == 0 &&
                         std::strcmp(firstLine->GetLastTextPtr(), "Victim tagged Killer") == 0;

    packet.targetPlayerKey = 0x7777;
    const bool missingRow = GameNet::HandlePkt08_PlayerKillEvent(killer.playerKey, &packet) == 0;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x4444;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x5678;
    g_zNetwork_IsHostFlag = 1;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;

    packet.targetPlayerKey = victim.playerKey;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    const std::int32_t hostResult = GameNet::HandlePkt08_PlayerKillEvent(killer.playerKey, &packet);
    const ScoreboardPacket2 *const hostSentPacket =
        reinterpret_cast<const ScoreboardPacket2 *>(g_sendPacketBytes);
    const bool host = hostResult == 1 && victim.score == 5 && g_sendCalls == 1 &&
                      g_sendPacketSize == sizeof(ScoreboardPacket2) &&
                      hostSentPacket->entries[1].playerKey == victim.playerKey &&
                      hostSentPacket->entries[1].packedScoreAndLapCount ==
                          static_cast<std::uint16_t>((victim.lapCount << 9) | 5);

    packet.targetPlayerKey = killer.playerKey;
    killer.score = 0;
    g_sendCalls = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    const std::int32_t suicideResult =
        GameNet::HandlePkt08_PlayerKillEvent(killer.playerKey, &packet);
    const ScoreboardPacket2 *const suicideSentPacket =
        reinterpret_cast<const ScoreboardPacket2 *>(g_sendPacketBytes);
    const bool suicide = suicideResult == 1 && killer.score == 0 && g_sendCalls == 1 &&
                         suicideSentPacket->entries[0].playerKey == killer.playerKey &&
                         suicideSentPacket->entries[0].packedScoreAndLapCount ==
                             static_cast<std::uint16_t>(killer.lapCount << 9);

    zUtil_SaveGameState saveState{};
    saveState.netPlayerRow = &victim;
    g_zNetwork_LocalPlayerKey = killer.playerKey;
    g_zNetwork_IsHostFlag = 0;
    g_sendCalls = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    GameNet::SendPkt08_PlayerKillEvent(&saveState, 3);
    const NetPkt08_PlayerKillEvent *const sentKillPacket =
        reinterpret_cast<const NetPkt08_PlayerKillEvent *>(g_sendPacketBytes);
    const bool explicitSaveStateSend =
        g_sendCalls == 1 && g_sendPacketSize == sizeof(NetPkt08_PlayerKillEvent) &&
        sentKillPacket->header.packetType == 0x08 &&
        sentKillPacket->header.packetSizeBytes == sizeof(NetPkt08_PlayerKillEvent) &&
        sentKillPacket->header.payloadDword0 == killer.playerKey &&
        sentKillPacket->killMethodOrOptCatalogEntryId == 3 &&
        sentKillPacket->targetPlayerKey == victim.playerKey;

    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);
    g_sendCalls = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    GameNet::SendPkt08_PlayerKillEvent(nullptr, 3);
    const NetPkt08_PlayerKillEvent *const fallbackKillPacket =
        reinterpret_cast<const NetPkt08_PlayerKillEvent *>(g_sendPacketBytes);
    const bool fallbackSaveStateSend =
        g_sendCalls == 1 && fallbackKillPacket->header.payloadDword0 == killer.playerKey &&
        fallbackKillPacket->targetPlayerKey == victim.playerKey;

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
    DeleteTopMessageStackFonts(top);
    triplet.DestructorCore();

    return nonHost && missingRow && host && suicide && explicitSaveStateSend &&
                   fallbackSaveStateSend
               ? 0
               : 1;
}

extern "C" int gamenet_reassign_player_colors_smoke(void) {
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    zNetworkPlayerRecordList *const oldPlayerRecordList = g_zNetwork_PlayerRecordList;
    zNetworkDPlaySessionDescCache *const oldSession = g_zNetwork_CurrentSessionDescCache;

    zNetwork_PlayerRecord firstRecord{};
    firstRecord.playerKey = 0x1111;
    firstRecord.colorIndex = 8;
    zNetwork_PlayerRecord secondRecord{};
    secondRecord.playerKey = 0x2222;
    secondRecord.colorIndex = 2;

    zNetworkPlayerRecordListNode sentinel{};
    zNetworkPlayerRecordListNode firstNode{};
    zNetworkPlayerRecordListNode secondNode{};
    sentinel.next = &firstNode;
    sentinel.prev = &secondNode;
    firstNode.next = &secondNode;
    firstNode.prev = &sentinel;
    firstNode.playerRecord = &firstRecord;
    secondNode.next = &sentinel;
    secondNode.prev = &firstNode;
    secondNode.playerRecord = &secondRecord;
    zNetworkPlayerRecordList playerList{};
    playerList.sentinelNode = &sentinel;
    playerList.count = 2;
    g_zNetwork_PlayerRecordList = &playerList;

    zNetworkDPlaySessionDescCache session{};
    session.desc.maxPlayers = 8;
    g_zNetwork_CurrentSessionDescCache = &session;

    HudUiTriplet triplet{};
    triplet.Constructor();
    HudUiStatsListElement statsList{};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    zClass_Object3DDataPartial firstObject{};
    zClass_NodePartial firstObjectNode{};
    firstObjectNode.classId = 5;
    firstObjectNode.classData = &firstObject;
    PlayerModalState firstModal{};
    firstModal.modalNode = &firstObjectNode;
    GameNetPlayerSaveState firstSave{};
    firstSave.primaryModalState = &firstModal;

    zClass_Object3DDataPartial secondObject{};
    zClass_NodePartial secondObjectNode{};
    secondObjectNode.classId = 5;
    secondObjectNode.classData = &secondObject;
    PlayerModalState secondModal{};
    secondModal.modalNode = &secondObjectNode;
    GameNetPlayerSaveState secondSave{};
    secondSave.primaryModalState = &secondModal;

    GameNetPlayerRow firstRow{};
    firstRow.playerKey = firstRecord.playerKey;
    firstRow.saveState = &firstSave;
    std::strcpy(firstRow.displayName, "First");
    GameNetPlayerRow secondRow{};
    secondRow.playerKey = secondRecord.playerKey;
    secondRow.saveState = &secondSave;
    std::strcpy(secondRow.displayName, "Second");
    firstRow.next = &secondRow;
    g_GameNetPlayerRowHead = &firstRow;
    g_GameNetPlayerRowTail = &secondRow;
    g_GameNetPlayerRowCount = 2;

    GameNet::RefreshPlayerListMenu(&firstRow);
    GameNet::RefreshPlayerListMenu(&secondRow);
    const std::int32_t result = GameNet::ReassignPlayerColorsAndRefreshRows(0, nullptr);

    const HudUiScoreboardEntry *firstEntry = nullptr;
    const HudUiScoreboardEntry *secondEntry = nullptr;
    for (HudUiScoreboardEntry *entry = triplet.entries.begin; entry != triplet.entries.end;
         ++entry) {
        if (entry->playerKey == firstRow.playerKey) {
            firstEntry = entry;
        }
        if (entry->playerKey == secondRow.playerKey) {
            secondEntry = entry;
        }
    }

    const bool firstOk = firstRow.playerColorIndex == 8 &&
                         firstRow.playerColorPackedRgb == 0x000040ff &&
                         FieldAt<std::uint32_t>(&firstRow.hudWidget, 0x14c) == 0x000040ff &&
                         FieldAt<std::uint32_t>(&firstRow.hudWidget, 0x150) == 0x000040ff &&
                         FieldAt<std::int32_t>(&firstRow.hudWidget, 0x270) == 1 &&
                         firstEntry != nullptr && firstEntry->playerColorPackedRgb == 0x000040ff &&
                         firstObject.color.red == 1.0f && firstObject.color.green == 1.0f &&
                         firstObject.color.blue == 0.0f && firstObject.colorAlpha == 0.2f;

    const bool secondOk =
        secondRow.playerColorIndex == 2 && secondRow.playerColorPackedRgb == 0x0000ff00 &&
        FieldAt<std::uint32_t>(&secondRow.hudWidget, 0x14c) == 0x0000ff00 &&
        FieldAt<std::uint32_t>(&secondRow.hudWidget, 0x150) == 0x0000ff00 &&
        FieldAt<std::int32_t>(&secondRow.hudWidget, 0x270) == 1 && secondEntry != nullptr &&
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

    return result == 1 && firstOk && secondOk ? 0 : 1;
}

extern "C" int gamenet_player_row_apply_color_tint_smoke(void) {
    zClass_Object3DDataPartial objectData{};
    zClass_NodePartial objectNode{};
    objectNode.classId = 5;
    objectNode.classData = &objectData;

    PlayerModalState modalState{};
    modalState.modalNode = &objectNode;
    GameNetPlayerSaveState saveState{};
    saveState.primaryModalState = &modalState;
    GameNetPlayerRow row{};
    row.playerColorIndex = 8;
    row.saveState = &saveState;

    row.ApplyPlayerColorTint();
    return objectData.color.red == 1.0f && objectData.color.green == 1.0f &&
                   objectData.color.blue == 0.0f && objectData.colorAlpha == 0.2f &&
                   (objectData.flags & 4) != 0
               ? 0
               : 1;
}

extern "C" int gamenet_player_row_destroy_embedded_panel_smoke(void) {
    GameNetPlayerRow row{};
    row.hudWidget.vtbl = &g_HudUiPanel_FTable;

    row.DestroyEmbeddedPanel();
    return row.hudWidget.vtbl == &g_HudUiCommon_FTable && row.hudWidget.textPick == nullptr ? 0 : 1;
}
