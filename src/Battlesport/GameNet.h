#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "recoil/recoil_callconv.h"
#include "zClass.h"

struct OptCatalogEntryDef;
struct PlayerMasterModalData;
struct zSndPlayHandle;
struct zUtil_PlayerStateStorage;
struct zUtil_SaveGameState;

struct PlayerModalState {
    PlayerModalState *next;
    PlayerMasterModalData *masterModalData;
    int modalStateCode;
    zVec3 transformedProbePointWorldByIndex[4];
    unsigned char reserved03c[0x30];
    float chassisPitchAngleRad;
    float chassisPitchFilterState;
    float chassisRollAngleRad;
    float chassisRollFilterState;
    unsigned char reserved07c[0x08];
    zClass_NodePartial *modalNode;
    zClass_NodePartial *nodeRTracks;
    zClass_NodePartial *nodeLTracks;
    zClass_NodePartial *nodeProps;
    zClass_NodePartial *nodeCaustic1;
    zClass_NodePartial *nodeWake;
    zClass_NodePartial *nodeSplashL;
    zClass_NodePartial *nodeSplashR;
    zClass_NodePartial *nodeDustL;
    zClass_NodePartial *nodeDustR;
    zSndPlayHandle *modalSfxHandle[4];
};

RECOIL_STATIC_ASSERT(offsetof(PlayerModalState, chassisPitchAngleRad) == 0x6c);
RECOIL_STATIC_ASSERT(offsetof(PlayerModalState, chassisPitchFilterState) == 0x70);
RECOIL_STATIC_ASSERT(offsetof(PlayerModalState, chassisRollAngleRad) == 0x74);
RECOIL_STATIC_ASSERT(offsetof(PlayerModalState, chassisRollFilterState) == 0x78);
RECOIL_STATIC_ASSERT(offsetof(PlayerModalState, modalNode) == 0x84);
RECOIL_STATIC_ASSERT(offsetof(PlayerModalState, nodeRTracks) == 0x88);
RECOIL_STATIC_ASSERT(offsetof(PlayerModalState, nodeLTracks) == 0x8c);
RECOIL_STATIC_ASSERT(offsetof(PlayerModalState, nodeProps) == 0x90);
RECOIL_STATIC_ASSERT(offsetof(PlayerModalState, nodeCaustic1) == 0x94);
RECOIL_STATIC_ASSERT(offsetof(PlayerModalState, nodeWake) == 0x98);
RECOIL_STATIC_ASSERT(offsetof(PlayerModalState, nodeSplashL) == 0x9c);
RECOIL_STATIC_ASSERT(offsetof(PlayerModalState, nodeSplashR) == 0xa0);
RECOIL_STATIC_ASSERT(offsetof(PlayerModalState, nodeDustL) == 0xa4);
RECOIL_STATIC_ASSERT(offsetof(PlayerModalState, nodeDustR) == 0xa8);
RECOIL_STATIC_ASSERT(offsetof(PlayerModalState, modalSfxHandle) == 0xac);

struct GameNetPlayerSaveState {
    unsigned char reserved000[0x04];
    zUtil_PlayerStateStorage *playerState;
    PlayerModalState *primaryModalState;
};

struct GameNetPlayerRow {
    int playerKey;
    int playerColorIndex;
    unsigned int playerColorPackedRgb;
    int score;
    int lapCount;
    float lapTimeSec;
    unsigned char reserved018[0x0c];
    char displayName[0x40];
    HudUiPanel hudWidget;
    unsigned char reserved1bc[0x14c];
    GameNetPlayerSaveState *saveState;
    GameNetPlayerRow *next;

    void RECOIL_THISCALL ApplyPlayerColorTint();
    void RECOIL_THISCALL DestroyEmbeddedPanel();
};

struct GameNetSpawnPoint {
    zVec3 position;
    float yawDegrees;
    GameNetSpawnPoint *next;
};

struct HudTimerPanelNetState {
    float timerSeconds;
    float timeWarningThresholdSec;
    float statusBitsResendDeadline;
    unsigned char reserved00c[0x04];
    int timerDirectionNeg;
    int startGateTriggered;
    int timeWarningShown;
    int raceFinishCountdownTriggered;
    int oneMinuteWarningShown;
    int startCountdownTriggered;
    int tenSecondWarningsEnabled;
    unsigned int tailFlags[8];

    void RECOIL_THISCALL ClearTailFlagsLocal();
};

struct NetPkt0C_HudTimerStatusBits {
    zNetworkPacketHeader header;
    float timerSeconds;
    int reserved_0c;
    short statusBitsLowWord;
    short statusBitsPackedHiWord;
};

struct NetPkt0D_HudTimerPanelState {
    zNetworkPacketHeader header;
    float seconds;
    short hudTimerFlagsPacked;
    short reserved_0e;
};

struct NetPkt0E_PlayerLapProgress {
    zNetworkPacketHeader header;
    short lapCountPacked;
    short reserved_0a;
    float lapTimeSec;
};

struct NetPkt0B_ChatMessage {
    zNetworkPacketHeader header;
    short messageLength;
    char message[0x50];
};

struct NetPkt08_PlayerKillEvent {
    zNetworkPacketHeader header;
    short killMethodOrOptCatalogEntryId;
    short reserved_0a;
    int targetPlayerKey;
};

struct NetPkt09_PlayerScoreboardEntry {
    int playerKey;
    unsigned short packedScoreAndLapCount;
    unsigned short reserved_06;
};

struct NetPkt09_PlayerScoreboardSnapshot {
    zNetworkPacketHeader header;
    int entryCount;
    NetPkt09_PlayerScoreboardEntry entries[1];
};

namespace GameNet {
RECOIL_NOINLINE GameNetPlayerRow *RECOIL_FASTCALL FindPlayerRowByKey(int playerKey);
RECOIL_NOINLINE int RECOIL_CDECL GetLocalPlayerColorIndexOrZero();
RECOIL_NOINLINE float RECOIL_FASTCALL
GetNearestOtherPlayerDistanceToSpawnPoint(GameNetSpawnPoint *spawnPoint,
                                          GameNetPlayerSaveState **outSaveState);
RECOIL_NOINLINE int RECOIL_CDECL AreAllPlayersAtLapTarget();
RECOIL_NOINLINE void RECOIL_CDECL ResetRemotePlayersAndSpawnLists();
RECOIL_NOINLINE void RECOIL_CDECL ResetHudTimerPanelNetStateLongCountdown();
RECOIL_NOINLINE void RECOIL_FASTCALL SetStatusBitsFromFlags(unsigned int statusFlags);
RECOIL_NOINLINE int RECOIL_CDECL GetStatusBitAllowMaps();
RECOIL_NOINLINE int RECOIL_CDECL GetStatusBitNameTags();
RECOIL_NOINLINE void RECOIL_FASTCALL ShowPlayerKillMessage(GameNetPlayerRow *victimRow,
                                                           OptCatalogEntryDef *killEntry,
                                                           GameNetPlayerRow *killerRow);
RECOIL_NOINLINE int RECOIL_CDECL
ReassignPlayerColorsAndRefreshRows(int senderPlayerId, zNetworkPacketHeader *packet);
RECOIL_NOINLINE void RECOIL_FASTCALL RefreshPlayerListMenu(GameNetPlayerRow *playerRow);
RECOIL_NOINLINE int RECOIL_FASTCALL
HandlePkt0C_HudTimerStatusBits(int senderPlayerId, NetPkt0C_HudTimerStatusBits *packet);
RECOIL_NOINLINE int RECOIL_FASTCALL HandlePkt0B_ChatMessage(int senderPlayerId,
                                                                     NetPkt0B_ChatMessage *packet);
RECOIL_NOINLINE int RECOIL_FASTCALL
HandlePkt0D_HudTimerPanelState(int senderPlayerId, NetPkt0D_HudTimerPanelState *packet);
RECOIL_NOINLINE int RECOIL_FASTCALL
HandlePkt08_PlayerKillEvent(int localPlayerKey, NetPkt08_PlayerKillEvent *packet);
RECOIL_NOINLINE void RECOIL_FASTCALL SendPkt08_PlayerKillEvent(
    zUtil_SaveGameState *saveState, short killMethodOrOptCatalogEntryId);
RECOIL_NOINLINE void RECOIL_CDECL SendPkt09_PlayerScoreboardSnapshot();
RECOIL_NOINLINE void RECOIL_FASTCALL SendPkt0E_PlayerLapProgress(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE int RECOIL_FASTCALL HandlePkt09_PlayerScoreboardSnapshot(
    int senderPlayerId, NetPkt09_PlayerScoreboardSnapshot *packet);
RECOIL_NOINLINE void RECOIL_FASTCALL
RespawnPlayerAndDropWeaponPickupIfAllowed(zUtil_SaveGameState *saveState,
                                          int useColorIndexedSpawn);
RECOIL_NOINLINE void RECOIL_FASTCALL
SendPkt0D_HudTimerPanelState(HudTimerPanelNetState *timerState);
RECOIL_NOINLINE int RECOIL_FASTCALL
SendPkt0C_HudTimerStatusBits(HudTimerPanelNetState *timerState);
RECOIL_NOINLINE int RECOIL_FASTCALL
HandlePkt0E_PlayerLapProgress(int senderPlayerId, NetPkt0E_PlayerLapProgress *packet);
RECOIL_NOINLINE int RECOIL_FASTCALL
HandlePkt13_EffectAnimActivationRecord(int senderPlayerId, zNetworkPacketHeader *packet);
RECOIL_NOINLINE int RECOIL_FASTCALL
HostUpdateSessionDescStatusFields(int eventCode, int auxParam,
                                  int valueOrTime, int statusFlags);
} // namespace GameNet

namespace GameNetSpawnPointList {
void RECOIL_CDECL InitGlobals();
}

namespace GameNetPlayerRowList {
void RECOIL_CDECL Reset();
}

extern "C" {
extern unsigned int g_GameNetPlayerRowList;
extern GameNetPlayerRow *g_GameNetPlayerRowHead;
extern GameNetPlayerRow *g_GameNetPlayerRowTail;
extern unsigned int g_GameNetPlayerRowCount;
extern unsigned int g_GameNetSpawnPointList;
extern GameNetSpawnPoint *g_GameNetSpawnPointHead;
extern GameNetSpawnPoint *g_GameNetSpawnPointTail;
extern unsigned int g_GameNetSpawnPointCount;
extern unsigned int g_GameNetPlayerRowStyleColors_00RRGGBB[9];
extern HudTimerPanelNetState g_HudTimerPanelNetState;
extern NetPkt0C_HudTimerStatusBits g_NetPkt0C_HudTimerStatusBitsBuf;
extern NetPkt0D_HudTimerPanelState g_NetPkt0D_HudTimerPanelStateBuf;
extern int g_GameNetOneLapLeftMessageShown;
extern int g_GameNetStatus_AllowMaps;
extern int g_GameNetStatus_NameTags;
extern int g_GameNetAllPlayersLapTargetCheckStarted;
extern int g_GameNetSuppressPkt13ActivationEcho;
}

#if defined(_M_IX86) || defined(__i386__)
RECOIL_STATIC_ASSERT(offsetof(PlayerModalState, modalNode) == 0x84);
RECOIL_STATIC_ASSERT(offsetof(PlayerModalState, nodeCaustic1) == 0x94);
RECOIL_STATIC_ASSERT(offsetof(PlayerModalState, modalSfxHandle) == 0xac);
RECOIL_STATIC_ASSERT(offsetof(PlayerModalState, next) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(GameNetPlayerSaveState, primaryModalState) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(GameNetPlayerRow, playerColorIndex) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(GameNetPlayerRow, playerColorPackedRgb) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(GameNetPlayerRow, score) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(GameNetPlayerRow, lapCount) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(GameNetPlayerRow, lapTimeSec) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(GameNetPlayerRow, displayName) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(GameNetPlayerRow, hudWidget) == 0x64);
RECOIL_STATIC_ASSERT(offsetof(GameNetPlayerRow, saveState) == 0x308);
RECOIL_STATIC_ASSERT(offsetof(GameNetPlayerRow, next) == 0x30c);
RECOIL_STATIC_ASSERT(offsetof(GameNetSpawnPoint, yawDegrees) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(GameNetSpawnPoint, next) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(HudTimerPanelNetState, timerDirectionNeg) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(HudTimerPanelNetState, timeWarningShown) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(HudTimerPanelNetState, oneMinuteWarningShown) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(HudTimerPanelNetState, startCountdownTriggered) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(HudTimerPanelNetState, tailFlags) == 0x2c);
RECOIL_STATIC_ASSERT(sizeof(HudTimerPanelNetState) == 0x4c);
RECOIL_STATIC_ASSERT(offsetof(NetPkt0C_HudTimerStatusBits, timerSeconds) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(NetPkt0C_HudTimerStatusBits, statusBitsPackedHiWord) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(NetPkt0C_HudTimerStatusBits) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(NetPkt0D_HudTimerPanelState, hudTimerFlagsPacked) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(NetPkt0D_HudTimerPanelState) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(NetPkt0E_PlayerLapProgress, lapCountPacked) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(NetPkt0E_PlayerLapProgress, lapTimeSec) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(NetPkt0E_PlayerLapProgress) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(NetPkt0B_ChatMessage, messageLength) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(NetPkt0B_ChatMessage, message) == 0x0a);
RECOIL_STATIC_ASSERT(offsetof(NetPkt08_PlayerKillEvent, killMethodOrOptCatalogEntryId) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(NetPkt08_PlayerKillEvent, targetPlayerKey) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(NetPkt08_PlayerKillEvent) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(NetPkt09_PlayerScoreboardEntry, packedScoreAndLapCount) == 0x04);
RECOIL_STATIC_ASSERT(sizeof(NetPkt09_PlayerScoreboardEntry) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(NetPkt09_PlayerScoreboardSnapshot, entryCount) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(NetPkt09_PlayerScoreboardSnapshot, entries) == 0x0c);
#endif
