#include "Battlesport/GameNet.h"

#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/RecoilApp.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zUtil/zSaveGame.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
unsigned int g_GameNetPlayerRowList = 0;
GameNetPlayerRow *g_GameNetPlayerRowHead = 0;
GameNetPlayerRow *g_GameNetPlayerRowTail = 0;
unsigned int g_GameNetPlayerRowCount = 0;
unsigned int g_GameNetSpawnPointList = 0;
GameNetSpawnPoint *g_GameNetSpawnPointHead = 0;
GameNetSpawnPoint *g_GameNetSpawnPointTail = 0;
unsigned int g_GameNetSpawnPointCount = 0;
unsigned int g_GameNetPlayerRowStyleColors_00RRGGBB[9] = {
    0x00000000, 0x000000ff, 0x0000ff00, 0x00ff0000, 0x00ff00ff,
    0x00ffff00, 0x0000ffff, 0x00ffffff, 0x000040ff,
};
HudTimerPanelNetState g_HudTimerPanelNetState = {0};
NetPkt0C_HudTimerStatusBits g_NetPkt0C_HudTimerStatusBitsBuf = {
    {0x0c, sizeof(NetPkt0C_HudTimerStatusBits), 0}, 0.0f, 0, 0, 0,
};
NetPkt0D_HudTimerPanelState g_NetPkt0D_HudTimerPanelStateBuf = {
    {0x0d, sizeof(NetPkt0D_HudTimerPanelState), 0},
    0.0f,
    0,
    0,
};
NetPkt0E_PlayerLapProgress g_NetPkt0E_PlayerLapProgressBuf = {
    {0x0e, sizeof(NetPkt0E_PlayerLapProgress), 0},
    0,
    0,
    0.0f,
};
int g_GameNetOneLapLeftMessageShown = 0;
int g_GameNetStatus_AllowMaps = 0;
int g_GameNetStatus_NameTags = 0;
int g_GameNetAllPlayersLapTargetCheckStarted = 0;
int g_GameNetSuppressPkt13ActivationEcho = 0;
}

namespace {
template <typename T> T &EmbeddedHudPanelField(HudUiPanel &panel, size_t offset) {
    return *reinterpret_cast<T *>(reinterpret_cast<unsigned char *>(&panel) + offset);
}

void SetEmbeddedHudPanelColor(GameNetPlayerRow *row, unsigned int color) {
    // The GameNet row embeds a full 0x2a4-byte panel; HudUiPanel is still
    // partial, so keep these BN offsets local to the row-color refresh path.
    EmbeddedHudPanelField<unsigned int>(row->hudWidget, 0x14c) = color;
    EmbeddedHudPanelField<unsigned int>(row->hudWidget, 0x150) = color;
    EmbeddedHudPanelField<int>(row->hudWidget, 0x270) = 1;
}
} // namespace

// Reimplements 0x433a50: GameNetPlayerRow::ApplyPlayerColorTint
void RECOIL_THISCALL GameNetPlayerRow::ApplyPlayerColorTint() {
    PlayerModalState *primaryModalState = saveState->primaryModalState;
    const unsigned int packedColor = g_GameNetPlayerRowStyleColors_00RRGGBB[playerColorIndex];
    zColorRgb color = {
        static_cast<float>(packedColor & 0xff),
        static_cast<float>((packedColor >> 8) & 0xff),
        static_cast<float>((packedColor >> 16) & 0xff),
    };

    zClass_Object3D::gwObject3DSetColorAlpha(primaryModalState->modalNode, &color, 0.2f);
    zClass_Object3D::gwObject3DSetVisibleFlag(primaryModalState->modalNode, 1);
}

// Reimplements 0x434650: GameNetPlayerRow::DestroyEmbeddedPanel
void RECOIL_THISCALL GameNetPlayerRow::DestroyEmbeddedPanel() {
    hudWidget.Destructor();
}

// Reimplements 0x433a40: HudTimerPanelNetState::ClearTailFlagsLocal
void RECOIL_THISCALL HudTimerPanelNetState::ClearTailFlagsLocal() {
    {
    for (int index = 0; index < 8; ++index) {
        tailFlags[index] = 0;
    }
    }
}

namespace GameNet {
// Reimplements 0x432830: GameNet::FindPlayerRowByKey (D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp)
RECOIL_NOINLINE GameNetPlayerRow *RECOIL_FASTCALL FindPlayerRowByKey(int playerKey) {
    GameNetPlayerRow *row = g_GameNetPlayerRowHead;
    while (row != 0) {
        if (row->playerKey == playerKey) {
            return row;
        }

        row = row->next;
    }

    return 0;
}

// Reimplements 0x433200: GameNet::AreAllPlayersAtLapTarget
// (D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp)
RECOIL_NOINLINE int RECOIL_CDECL AreAllPlayersAtLapTarget() {
    if (g_GameNetAllPlayersLapTargetCheckStarted == 0) {
        g_GameNetAllPlayersLapTargetCheckStarted = 1;
    }

    GameNetPlayerRow *row = g_GameNetPlayerRowHead;
    while (row != 0) {
        if (row->lapCount < g_HudSensorTracker.runtimeGoalValue) {
            return 0;
        }

        row = row->next;
    }

    return 1;
}

// Reimplements 0x4320f0: GameNet::ResetRemotePlayersAndSpawnLists
// (D:\Proj\Battlesport\GameNet.cpp)
RECOIL_NOINLINE void RECOIL_CDECL ResetRemotePlayersAndSpawnLists() {
    GameNetPlayerRow *row = g_GameNetPlayerRowHead;
    while (row != 0) {
        HudUi::RemoveScoreboardEntryRow(row);
        g_HudUiTopMessageStack->base.RemoveChild(reinterpret_cast<HudUiElement *>(&row->hudWidget));
        row = row->next;
    }

    GameNetSpawnPoint *spawnPoint = g_GameNetSpawnPointHead;
    while (spawnPoint != 0) {
        GameNetSpawnPoint *const next = spawnPoint->next;
        ::operator delete(spawnPoint);
        spawnPoint = next;
    }

    g_GameNetSpawnPointList = 0;
    g_GameNetSpawnPointTail = 0;
    g_GameNetSpawnPointHead = 0;
    g_GameNetSpawnPointCount = 0;

    row = g_GameNetPlayerRowHead;
    while (row != 0) {
        GameNetPlayerRow *const next = row->next;
        row->DestroyEmbeddedPanel();
        ::operator delete(row);
        row = next;
    }

    g_GameNetPlayerRowList = 0;
    g_GameNetPlayerRowTail = 0;
    g_GameNetPlayerRowHead = 0;
    g_GameNetPlayerRowCount = 0;
}

// Reimplements 0x4322a0: GameNet::ResetHudTimerPanelNetStateLongCountdown
RECOIL_NOINLINE void RECOIL_CDECL ResetHudTimerPanelNetStateLongCountdown() {
    g_HudTimerPanelNetState.timerSeconds = 36000.0f;
    HudUiTimerPanel::SetSeconds(36000.0f, -1.0f);
    g_HudTimerPanelNetState.startCountdownTriggered = 0;
    g_HudTimerPanelNetState.tenSecondWarningsEnabled = 0;
    g_HudTimerPanelNetState.timeWarningThresholdSec = 120.0f;
    g_HudTimerPanelNetState.timerDirectionNeg = 1;
    g_HudTimerPanelNetState.startGateTriggered = 0;
    g_HudTimerPanelNetState.raceFinishCountdownTriggered = 0;
    g_GameNetAllPlayersLapTargetCheckStarted = 0;
    g_HudTimerPanelNetState.ClearTailFlagsLocal();
    g_GameNetOneLapLeftMessageShown = 0;
}

// Reimplements 0x433710: GameNet::SetStatusBitsFromFlags (D:\Proj\Battlesport\GameNet.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SetStatusBitsFromFlags(unsigned int statusFlags) {
    g_GameNetStatus_AllowMaps = statusFlags & 1u;
    g_GameNetStatus_NameTags = (statusFlags >> 1) & 1u;
}

// Reimplements 0x433730: GameNet::GetStatusBitAllowMaps (D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp)
RECOIL_NOINLINE int RECOIL_CDECL GetStatusBitAllowMaps() {
    return g_GameNetStatus_AllowMaps;
}

// Reimplements 0x433740: GameNet::GetStatusBitNameTags (src/Battlesport/gamenet.cpp)
RECOIL_NOINLINE int RECOIL_CDECL GetStatusBitNameTags() {
    return g_GameNetStatus_NameTags;
}

// Reimplements 0x414330: GameNet::ShowPlayerKillMessage (D:\Proj\Battlesport\HudUi.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ShowPlayerKillMessage(GameNetPlayerRow *victimRow,
                                                           OptCatalogEntryDef *killEntry,
                                                           GameNetPlayerRow *killerRow) {
    const char *killVerb = "";
    if (killEntry == 0) {
        killVerb = zLoc::GetMessageString(0x253);
    } else if (killEntry->killVerbString != 0) {
        killVerb = killEntry->killVerbString;
    }

    char message[0x50];
    sprintf(message, "%s %s %s", victimRow->displayName, killVerb, killerRow->displayName);
    HudUi::ShowTopMessageLine(message, 2.0f);
}

// Reimplements 0x432e70: GameNet::ReassignPlayerColorsAndRefreshRows
// (D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp)
RECOIL_NOINLINE int RECOIL_CDECL
ReassignPlayerColorsAndRefreshRows(int, zNetworkPacketHeader *) {
    GameNetPlayerRow *row = g_GameNetPlayerRowHead;
    while (row != 0) {
        const int colorIndex = zNetwork_GetPlayerColorIndexByKey(row->playerKey);
        row->playerColorIndex = colorIndex;

        const unsigned int color = g_GameNetPlayerRowStyleColors_00RRGGBB[colorIndex];
        row->playerColorPackedRgb = color;
        SetEmbeddedHudPanelColor(row, color);
        HudUi::RefreshScoreboardEntryRow(row);
        row->ApplyPlayerColorTint();

        row = row->next;
    }

    return 1;
}

// Reimplements 0x414390: GameNet::RefreshPlayerListMenu (D:\Proj\Battlesport\ai_net.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL RefreshPlayerListMenu(GameNetPlayerRow *playerRow) {
    g_HudUiMgrStatsList->triplet->AddEntry(playerRow);
}

// Reimplements 0x433410: GameNet::HandlePkt0C_HudTimerStatusBits
// (D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
HandlePkt0C_HudTimerStatusBits(int, NetPkt0C_HudTimerStatusBits *packet) {
    const int statusBits = packet->statusBitsPackedHiWord;
    g_HudTimerPanelNetState.timerSeconds = packet->timerSeconds;
    g_HudTimerPanelNetState.timerDirectionNeg = statusBits & 1;

    float secondsStep = -1.0f;
    if (g_HudTimerPanelNetState.timerDirectionNeg == 0) {
        secondsStep = 1.0f;
    }

    HudUiTimerPanel::SetSeconds(g_HudTimerPanelNetState.timerSeconds, secondsStep);

    if (g_HudSensorTracker.raceCheckpointMode == 0 &&
        g_HudTimerPanelNetState.oneMinuteWarningShown == 0 && (statusBits & 4) != 0) {
        g_HudTimerPanelNetState.oneMinuteWarningShown = 1;
        HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x914), 5.0f);
        if (zNetwork::IsHost() != 0) {
            HostUpdateSessionDescStatusFields(0x100, 0, 0, 0);
        }
    }

    if (g_HudTimerPanelNetState.timeWarningShown == 0 && (statusBits & 2) != 0) {
        HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x915), 5.0f);
        g_HudTimerPanelNetState.timeWarningShown = 1;
        g_RecoilApp.QueueSwitchCurrentState(&g_RecoilApp.m_mpExitDialogState_220.base, 0);
    }

    return 1;
}

// Reimplements 0x4337e0: GameNet::HandlePkt0B_ChatMessage
// (D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL HandlePkt0B_ChatMessage(int,
                                                                     NetPkt0B_ChatMessage *packet) {
    char message[0x51] = {0};
    int messageLength = packet->messageLength;
    if (messageLength >= 0x50) {
        messageLength = 0x50;
    }

    if (messageLength > 0) {
        memcpy(message, packet->message, static_cast<size_t>(messageLength));
    }

    message[messageLength] = '\0';
    HudUi::ShowChatLine(message, 5.0f);
    return 1;
}

// Reimplements 0x433250: GameNet::HandlePkt0D_HudTimerPanelState
// (D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
HandlePkt0D_HudTimerPanelState(int, NetPkt0D_HudTimerPanelState *packet) {
    const int statusBits = packet->hudTimerFlagsPacked;
    g_HudTimerPanelNetState.timerSeconds = packet->seconds;
    g_HudTimerPanelNetState.timerDirectionNeg = statusBits & 1;

    float secondsStep = -1.0f;
    if (g_HudTimerPanelNetState.timerDirectionNeg == 0) {
        secondsStep = 1.0f;
    }

    HudUiTimerPanel::SetSeconds(g_HudTimerPanelNetState.timerSeconds, secondsStep);

    if (g_HudTimerPanelNetState.startGateTriggered == 0 && (statusBits & 8) != 0) {
        g_HudTimerPanelNetState.startGateTriggered = 1;
    }

    if ((statusBits & 0x20) != 0) {
        zEffectAnim::SetVelocity_Thunk(zEffectAnim::FindEntryByName("startcountdown"), 0,
                                       0.0f, 0.0f, 0.0f);
        g_HudTimerPanelNetState.startCountdownTriggered = 1;
    }

    if (g_HudTimerPanelNetState.raceFinishCountdownTriggered == 0 && (statusBits & 0x10) != 0) {
        g_HudTimerPanelNetState.raceFinishCountdownTriggered = 1;
    }

    g_HudTimerPanelNetState.timeWarningShown = statusBits & 2;
    if (g_HudTimerPanelNetState.timeWarningShown != 0) {
        g_RecoilApp.QueueSwitchCurrentState(&g_RecoilApp.m_mpExitDialogState_220.base, 0);
    }

    return 1;
}

// Reimplements 0x433060: GameNet::HandlePkt08_PlayerKillEvent (D:\Proj\Battlesport\ai_net.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
HandlePkt08_PlayerKillEvent(int localPlayerKey, NetPkt08_PlayerKillEvent *packet) {
    GameNetPlayerRow *const killerRow = FindPlayerRowByKey(localPlayerKey);
    GameNetPlayerRow *const victimRow = FindPlayerRowByKey(packet->targetPlayerKey);
    if (killerRow == 0 || victimRow == 0) {
        return 0;
    }

    OptCatalogEntryDef *killEntry = 0;
    const short killEntryId = packet->killMethodOrOptCatalogEntryId;
    if (killEntryId != 0) {
        killEntry = OptCatalog::FindEntryById(killEntryId);
    }

    ShowPlayerKillMessage(victimRow, killEntry, killerRow);

    if (zNetwork::IsHost() != 0) {
        const int score = victimRow->score;
        if (victimRow != killerRow) {
            victimRow->score = score + 1;
        } else {
            victimRow->score = score - 1;
            if (victimRow->score < 0) {
                victimRow->score = 0;
            }
        }

        SendPkt09_PlayerScoreboardSnapshot();
    }

    return 1;
}

// Reimplements 0x433000: GameNet::SendPkt08_PlayerKillEvent (D:\Proj\Battlesport\ai_net.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SendPkt08_PlayerKillEvent(
    zUtil_SaveGameState *saveState, short killMethodOrOptCatalogEntryId) {
    zUtil_SaveGameState *saveStateOrLocal = saveState;
    if (saveStateOrLocal == 0) {
        saveStateOrLocal = reinterpret_cast<zUtil_SaveGameState *>(g_GameStateOrMapTable);
    }

    NetPkt08_PlayerKillEvent packet;
    packet.header.packetType = 0x08;
    packet.header.packetSizeBytes = sizeof(NetPkt08_PlayerKillEvent);
    packet.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    packet.killMethodOrOptCatalogEntryId = killMethodOrOptCatalogEntryId;
    packet.targetPlayerKey = saveStateOrLocal->netPlayerRow->playerKey;

    zNetwork_SendPacketReliable(&packet.header);
    HandlePkt08_PlayerKillEvent(zNetwork_GetLocalPlayerKey(), &packet);
}

// Reimplements 0x4330f0: GameNet::SendPkt0E_PlayerLapProgress
// (D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SendPkt0E_PlayerLapProgress(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    NetPkt0E_PlayerLapProgress packet;
    packet.header.packetType = 0x0e;
    packet.header.packetSizeBytes = sizeof(NetPkt0E_PlayerLapProgress);
    packet.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    packet.lapCountPacked = static_cast<short>(playerState->lapCount);
    packet.reserved_0a = 0;
    packet.lapTimeSec = playerState->lapTimeSec;

    if (zNetwork::IsHost() != 0) {
        saveState->netPlayerRow->lapCount = playerState->lapCount;
        saveState->netPlayerRow->lapTimeSec = playerState->lapTimeSec;
        HandlePkt0E_PlayerLapProgress(packet.header.payloadDword0, &packet);
    } else {
        zNetwork_SendPacketReliable(&packet.header);
    }
}

// Reimplements 0x4334f0: GameNet::SendPkt09_PlayerScoreboardSnapshot
// (D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp)
RECOIL_NOINLINE void RECOIL_CDECL SendPkt09_PlayerScoreboardSnapshot() {
    if (zNetwork::IsHost() == 0) {
        return;
    }

    const int entryCount = static_cast<int>(g_GameNetPlayerRowCount);
    const size_t packetSize =
        sizeof(zNetworkPacketHeader) + sizeof(int) +
        static_cast<size_t>(entryCount) * sizeof(NetPkt09_PlayerScoreboardEntry);
    NetPkt09_PlayerScoreboardSnapshot *const packet =
        static_cast<NetPkt09_PlayerScoreboardSnapshot *>(malloc(packetSize));
    memset(packet, 0, packetSize);

    packet->header.packetType = 0x09;
    packet->header.packetSizeBytes = static_cast<unsigned short>(packetSize);
    packet->header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    packet->entryCount = entryCount;

    GameNetPlayerRow *row = g_GameNetPlayerRowHead;
    NetPkt09_PlayerScoreboardEntry *entry = packet->entries;
    while (row != 0) {
        entry->playerKey = row->playerKey;
        entry->packedScoreAndLapCount =
            static_cast<unsigned short>((row->lapCount << 9) + (row->score & 0x1ff));
        row = row->next;
        ++entry;
    }

    zNetwork_SendPacketReliable(&packet->header);
    if (zNetwork::IsHost() != 0) {
        HandlePkt09_PlayerScoreboardSnapshot(zNetwork_GetLocalPlayerKey(), packet);
    }

    free(packet);
}

// Reimplements 0x4335b0: GameNet::HandlePkt09_PlayerScoreboardSnapshot
// (D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
HandlePkt09_PlayerScoreboardSnapshot(int, NetPkt09_PlayerScoreboardSnapshot *packet) {
    GameNetPlayerRow *oneLapLeftRow = 0;
    const int entryCount = packet->entryCount;

    {
    for (int index = 0; index < entryCount; ++index) {
        NetPkt09_PlayerScoreboardEntry *const entry = &packet->entries[index];
        GameNetPlayerRow *const row = FindPlayerRowByKey(entry->playerKey);
        if (row == 0) {
            continue;
        }

        const unsigned short packed = entry->packedScoreAndLapCount;
        row->score = packed & 0x1ff;
        row->lapCount = (static_cast<short>(packed) >> 9) & 0x7f;
        HudUi::RefreshScoreboardEntryRow(row);

        if (g_GameNetOneLapLeftMessageShown == 0 && oneLapLeftRow == 0 &&
            row->lapCount == g_HudSensorTracker.runtimeGoalValue - 1) {
            oneLapLeftRow = row;
        }

        if (zNetwork::IsHost() != 0 && g_HudSensorTracker.raceCheckpointMode == 0 &&
            row->score >= g_HudSensorTracker.runtimeGoalValue) {
            HudTimerPanelNetState timerState = g_HudTimerPanelNetState;
            if (timerState.timeWarningShown == 0) {
                timerState.timeWarningShown = 1;
                SendPkt0C_HudTimerStatusBits(&timerState);
            }
        }
    }
    }

    if (g_HudSensorTracker.raceCheckpointMode != 0 && g_GameNetOneLapLeftMessageShown == 0 &&
        oneLapLeftRow != 0) {
        char message[0x80];
        zLoc::FormatMessage(message, sizeof(message), 0x918, oneLapLeftRow->displayName);
        HudUi::ShowTopMessageLine(message, 5.0f);
        g_GameNetOneLapLeftMessageShown = 1;
        if (zNetwork::IsHost() != 0) {
            HostUpdateSessionDescStatusFields(0x100, 0, 0, 0);
        }
    }

    return 1;
}

// Reimplements 0x433310: GameNet::SendPkt0D_HudTimerPanelState
// (D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
SendPkt0D_HudTimerPanelState(HudTimerPanelNetState *timerState) {
    if (zNetwork::IsHost() == 0) {
        return;
    }

    g_NetPkt0D_HudTimerPanelStateBuf.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    g_NetPkt0D_HudTimerPanelStateBuf.seconds = HudUiTimerPanel::GetSeconds();

    short statusBits = 0;
    if (timerState->timerDirectionNeg != 0) {
        statusBits = 1;
    }
    if (timerState->startGateTriggered != 0) {
        statusBits |= 8;
    }
    if (timerState->timeWarningShown != 0) {
        statusBits |= 2;
    }
    if (timerState->raceFinishCountdownTriggered != 0) {
        statusBits |= 0x10;
    }
    if (timerState->startCountdownTriggered != 0) {
        statusBits |= 0x20;
    }

    g_NetPkt0D_HudTimerPanelStateBuf.hudTimerFlagsPacked = statusBits;
    zNetwork_SendPacketReliable(&g_NetPkt0D_HudTimerPanelStateBuf.header);
    HandlePkt0D_HudTimerPanelState(g_NetPkt0D_HudTimerPanelStateBuf.header.payloadDword0,
                                   &g_NetPkt0D_HudTimerPanelStateBuf);
}

// Reimplements 0x433170: GameNet::HandlePkt0E_PlayerLapProgress
// (D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
HandlePkt0E_PlayerLapProgress(int senderPlayerId, NetPkt0E_PlayerLapProgress *packet) {
    int result = zNetwork::IsHost();
    if (result == 0) {
        return result;
    }

    GameNetPlayerRow *const row = FindPlayerRowByKey(senderPlayerId);
    if (row == 0) {
        return 0;
    }

    row->lapCount = packet->lapCountPacked;
    row->lapTimeSec = packet->lapTimeSec;
    SendPkt09_PlayerScoreboardSnapshot();

    if (row->lapCount >= g_HudSensorTracker.runtimeGoalValue) {
        HudTimerPanelNetState timerState = g_HudTimerPanelNetState;
        if (AreAllPlayersAtLapTarget() != 0) {
            timerState.timeWarningShown = 1;
            timerState.raceFinishCountdownTriggered = 1;
        }

        SendPkt0D_HudTimerPanelState(&timerState);
    }

    return 1;
}

// Reimplements 0x4343f0: GameNet::HandlePkt13_EffectAnimActivationRecord
// (D:\Proj\GameZRecoil\GameNet.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
HandlePkt13_EffectAnimActivationRecord(int, zNetworkPacketHeader *packet) {
    zEffectAnimActivationRecord *const record = reinterpret_cast<zEffectAnimActivationRecord *>(
        reinterpret_cast<unsigned char *>(packet) + sizeof(zNetworkPacketHeader));
    if (zEffect_Anim::HasActivationRecord(record) == 0) {
        g_GameNetSuppressPkt13ActivationEcho = 1;
        zEffect_Anim::ProcessActivationRecord(record);
        g_GameNetSuppressPkt13ActivationEcho = 0;
    }

    return 1;
}

// Reimplements 0x433390: GameNet::SendPkt0C_HudTimerStatusBits
// (D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
SendPkt0C_HudTimerStatusBits(HudTimerPanelNetState *timerState) {
    const int result = zNetwork::IsHost();
    if (result == 0) {
        return result;
    }

    g_NetPkt0C_HudTimerStatusBitsBuf.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    g_NetPkt0C_HudTimerStatusBitsBuf.timerSeconds = HudUiTimerPanel::GetSeconds();

    short statusBits = 0;
    if (timerState->timerDirectionNeg != 0) {
        statusBits = 1;
    }
    if (timerState->timeWarningShown != 0) {
        statusBits |= 2;
    }
    if (timerState->oneMinuteWarningShown != 0) {
        statusBits |= 4;
    }

    g_NetPkt0C_HudTimerStatusBitsBuf.statusBitsPackedHiWord = statusBits;
    g_HudTimerPanelNetState.statusBitsResendDeadline = g_Time_AccumulatedTimeSec + 30.0f;
    zNetwork_SendPacketReliable(&g_NetPkt0C_HudTimerStatusBitsBuf.header);
    return HandlePkt0C_HudTimerStatusBits(g_NetPkt0C_HudTimerStatusBitsBuf.header.payloadDword0,
                                          &g_NetPkt0C_HudTimerStatusBitsBuf);
}

// Reimplements 0x434550: GameNet::HostUpdateSessionDescStatusFields
// (D:\Proj\Battlesport\GameNet.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
HostUpdateSessionDescStatusFields(int eventCode, int auxParam,
                                  int valueOrTime, int statusFlags) {
    if (zNetwork::IsHost() == 0) {
        return 0;
    }

    zNetworkSessionDescStatusFields statusFields;
    if (zNetwork_ExtractStatusFieldsFromSessionDesc(&statusFields) == 0) {
        return 0;
    }

    statusFields.valueOrTime = valueOrTime;
    statusFields.eventCode = eventCode;
    statusFields.statusFlags = statusFlags;
    statusFields.auxParam = auxParam;
    return zNetwork_ApplyStatusFieldsToSessionDesc(&statusFields);
}
} // namespace GameNet

namespace GameNetSpawnPointList {
// Reimplements 0x431bf0: GameNetSpawnPointList::InitGlobals
void RECOIL_CDECL InitGlobals() {
    g_GameNetSpawnPointList = 0;
    g_GameNetSpawnPointTail = 0;
    g_GameNetSpawnPointHead = 0;
    g_GameNetSpawnPointCount = 0;
}
} // namespace GameNetSpawnPointList

namespace GameNetPlayerRowList {
// Reimplements 0x431c20: GameNetPlayerRowList::Reset
void RECOIL_CDECL Reset() {
    g_GameNetPlayerRowList = 0;
    g_GameNetPlayerRowTail = 0;
    g_GameNetPlayerRowHead = 0;
    g_GameNetPlayerRowCount = 0;
}
} // namespace GameNetPlayerRowList
