#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "recoil/recoil_callconv.h"
#include "zClass.h"

struct zDEClient_QSandEventTemplate;
struct zDEClient_CraterEventTemplate;
struct OptCatalogEntryDef;
struct PlayerMasterModalData;
struct zSndPlayHandle;
struct zEffectAnimActivationRecord;
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
    zClass_NodePartial *nodeRightMorphs;
    zClass_NodePartial *nodeLeftMorphs;
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
RECOIL_STATIC_ASSERT(offsetof(PlayerModalState, nodeRightMorphs) == 0x7c);
RECOIL_STATIC_ASSERT(offsetof(PlayerModalState, nodeLeftMorphs) == 0x80);
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
    zClass_NodePartial *playerNode;
    zClass_NodePartial *turretNode;
    zClass_NodePartial *gunNode;
    char displayName[0x40];
    HudUiPanel hudWidget;
    GameNetPlayerSaveState *saveState;
    GameNetPlayerRow *next;

    void RECOIL_THISCALL ApplyPlayerColorTint();
    void RECOIL_THISCALL DestroyEmbeddedPanel();
};

struct GameNetPlayerRowListState {
    unsigned int flags;
    GameNetPlayerRow *head;
    GameNetPlayerRow *tail;
    unsigned int count;
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

struct NetPkt06_PlayerStateSnapshot {
    zNetworkPacketHeader header;
    short cachedAltSelectionCode;
    short cachedPrimarySelectionCode;
    unsigned int packedMasterTypeColorFlags;
    zVec3 altGunAimOrigin;
    zVec3 storedTargetPos;
    zVec3 worldPos;
    zVec3 vehicleRotationAngles;
    float statusMeterValue;
    int progressTargetCount;
    zVec3 progressTargetPoints[26];
};

struct NetPkt0A_RemoveRuntimeRelay {
    zNetworkPacketHeader header;
    short optCatalogEntryId;
    short reserved_0a;
    zVec3 pointOrVec3;
    int ownerPlayerKey;
};

struct NetPkt07_AltGunDispatch {
    zNetworkPacketHeader header;
    short weaponId;
    short reserved_0a;
    unsigned int dispatchFlags;
    zVec3 targetPos;
};

struct NetPkt0E_PlayerLapProgress {
    zNetworkPacketHeader header;
    short lapCountPacked;
    short reserved_0a;
    float lapTimeSec;
};

struct NetPkt10_QSandEvent {
    zNetworkPacketHeader header;
    unsigned int eventFlags;
    int reserved_0c;
    zVec3 center;
    float radius;
};

struct NetPkt0F_CraterEvent {
    zNetworkPacketHeader header;
    unsigned int eventFlags;
    int craterTypeId;
    zVec3 center;
    float radius;
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

struct NetPkt14_HudTimerAndFlagsSync {
    zNetworkPacketHeader header;
    short eventCode;
    short auxParam;
    int valueOrTime;
    unsigned int statusFlags;
};

namespace GameNet {
RECOIL_NOINLINE GameNetPlayerRow *RECOIL_FASTCALL FindPlayerRowByKey(int playerKey);
RECOIL_NOINLINE int RECOIL_CDECL GetLocalPlayerColorIndexOrZero();
RECOIL_NOINLINE float RECOIL_FASTCALL
GetNearestOtherPlayerDistanceToSpawnPoint(GameNetSpawnPoint *spawnPoint,
                                          GameNetPlayerSaveState **outSaveState);
RECOIL_NOINLINE int RECOIL_CDECL AreAllPlayersAtLapTarget();
RECOIL_NOINLINE void RECOIL_CDECL RegisterGameplayHandlersAndOptCatalogCallbacks();
RECOIL_NOINLINE void RECOIL_CDECL UnregisterGameplayPacketHandlers();
RECOIL_NOINLINE void RECOIL_CDECL ResetRemotePlayersAndSpawnLists();
RECOIL_NOINLINE int RECOIL_FASTCALL WaitForLocalPlayerColorIndex(int maxWaitSeconds);
RECOIL_NOINLINE void RECOIL_CDECL ResetHudTimerPanelNetStateLongCountdown();
RECOIL_NOINLINE void RECOIL_FASTCALL SetStatusBitsFromFlags(unsigned int statusFlags);
RECOIL_NOINLINE int RECOIL_CDECL GetStatusBitAllowMaps();
RECOIL_NOINLINE int RECOIL_CDECL GetStatusBitNameTags();
RECOIL_NOINLINE void RECOIL_FASTCALL ShowPlayerKillMessage(GameNetPlayerRow *victimRow,
                                                           OptCatalogEntryDef *killEntry,
                                                           GameNetPlayerRow *killerRow);
RECOIL_NOINLINE int RECOIL_CDECL
ReassignPlayerColorsAndRefreshRows(int senderPlayerId, zNetworkPacketHeader *packet);
RECOIL_NOINLINE int RECOIL_FASTCALL
HandlePkt03_RemoveRemotePlayer(int senderPlayerId, zNetworkPacketHeader *packet);
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
RECOIL_NOINLINE int RECOIL_FASTCALL
TickLocalPlayerPkt06ReplicationAndHudTimer(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE int RECOIL_FASTCALL
ApplyPkt06_PlayerStateSnapshotToRow(GameNetPlayerRow *row,
                                    NetPkt06_PlayerStateSnapshot *packet);
RECOIL_NOINLINE int RECOIL_FASTCALL
SpawnRemotePlayerFromPkt06_PlayerStateSnapshot(int senderPlayerId,
                                               NetPkt06_PlayerStateSnapshot *packet);
RECOIL_NOINLINE int RECOIL_FASTCALL
HandlePkt06_PlayerStateSnapshot(int senderPlayerId, NetPkt06_PlayerStateSnapshot *packet);
RECOIL_NOINLINE int RECOIL_FASTCALL HandlePkt07_AltGunDispatch(int senderPlayerId,
                                                               NetPkt07_AltGunDispatch *packet);
RECOIL_NOINLINE void RECOIL_FASTCALL SendPkt07_AltGunDispatch(short weaponId,
                                                              unsigned int dispatchFlags);
RECOIL_NOINLINE int RECOIL_FASTCALL AltGunDispatchNoOpCallback(OptCatalogEntryDef *entry,
                                                               void **saveStateSlot);
RECOIL_NOINLINE int RECOIL_FASTCALL
HostSendPkt10_QSandFeature(zDEClient_QSandEventTemplate *eventTemplate);
RECOIL_NOINLINE int RECOIL_FASTCALL
SendPkt10_QSandEvent(zDEClient_QSandEventTemplate *eventTemplate);
RECOIL_NOINLINE int RECOIL_FASTCALL
HostSendPkt0F_CraterFeature(zDEClient_CraterEventTemplate *eventTemplate);
RECOIL_NOINLINE void RECOIL_FASTCALL
SendPkt0D_HudTimerPanelState(HudTimerPanelNetState *timerState);
RECOIL_NOINLINE int RECOIL_FASTCALL
SendPkt0C_HudTimerStatusBits(HudTimerPanelNetState *timerState);
RECOIL_NOINLINE int RECOIL_FASTCALL
HandlePkt0E_PlayerLapProgress(int senderPlayerId, NetPkt0E_PlayerLapProgress *packet);
RECOIL_NOINLINE int RECOIL_FASTCALL
HandlePkt13_EffectAnimActivationRecord(int senderPlayerId, zNetworkPacketHeader *packet);
RECOIL_NOINLINE void RECOIL_FASTCALL
SendPkt13_EffectAnimActivationRecord(zEffectAnimActivationRecord *record);
RECOIL_NOINLINE void RECOIL_CDECL SendAllPkt13_EffectAnimActivationRecords();
RECOIL_NOINLINE int RECOIL_FASTCALL
HandlePkt14_HudTimerAndFlagsSync(int senderPlayerId,
                                 NetPkt14_HudTimerAndFlagsSync *packet);
RECOIL_NOINLINE int RECOIL_FASTCALL
HostUpdateSessionDescStatusFields(int eventCode, int auxParam,
                                  int valueOrTime, int statusFlags);
RECOIL_NOINLINE int RECOIL_FASTCALL
UpdateRemotePlayerHudWidgetScreenPos(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL ChatComposeKeyCallback(int dikCodeWithMods);
RECOIL_NOINLINE void RECOIL_CDECL BeginChatCompose();
} // namespace GameNet

namespace Net {
RECOIL_NOINLINE void RECOIL_CDECL InitFromZrd();
}

namespace GameNetSpawnPointList {
void RECOIL_CDECL InitGlobals();
}

namespace GameNetPlayerRowList {
void RECOIL_CDECL Reset();
RECOIL_NOINLINE GameNetPlayerRow *RECOIL_FASTCALL
AppendNewRow(GameNetPlayerRowListState *self, int zeroInitializeRow);
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
extern NetPkt0A_RemoveRuntimeRelay g_NetPkt0A_RemoveRuntimeRelayBuf;
extern NetPkt07_AltGunDispatch g_NetPkt07_AltGunDispatchBuf;
extern int g_GameNetOneLapLeftMessageShown;
extern int g_GameNetStatus_AllowMaps;
extern int g_GameNetStatus_NameTags;
extern int g_GameNetAllPlayersLapTargetCheckStarted;
extern int g_GameNetSuppressPkt13ActivationEcho;
extern int g_GameNetPkt06InitialSyncGate;
extern int g_GameNetPkt06InputBit17Latch;
extern int g_GameNetPkt06InputBit16Latch;
extern float g_GameNetPkt06NextSendTimeSec;
extern int g_GameNetHostHudTimerInitFlag;
extern int g_GameNetHudTimerTenSecondWarningArmed;
extern int g_GameNetHudTimerPendingSaveReminderArmed;
extern int g_GameNet_HandlersRegistered;
extern NetPkt06_PlayerStateSnapshot g_NetPkt06_PlayerStateSnapshotBuf;
extern NetPkt0F_CraterEvent g_NetPkt0F_CraterEventSendBuf;
extern NetPkt10_QSandEvent g_NetPkt10_QSandEventSendBuf;
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
RECOIL_STATIC_ASSERT(offsetof(GameNetPlayerRow, playerNode) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(GameNetPlayerRow, turretNode) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(GameNetPlayerRow, gunNode) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(GameNetPlayerRow, displayName) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(GameNetPlayerRow, hudWidget) == 0x64);
RECOIL_STATIC_ASSERT(offsetof(GameNetPlayerRow, saveState) == 0x308);
RECOIL_STATIC_ASSERT(offsetof(GameNetPlayerRow, next) == 0x30c);
RECOIL_STATIC_ASSERT(offsetof(GameNetPlayerRowListState, head) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(GameNetPlayerRowListState, tail) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(GameNetPlayerRowListState, count) == 0x0c);
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
RECOIL_STATIC_ASSERT(offsetof(NetPkt06_PlayerStateSnapshot, cachedAltSelectionCode) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(NetPkt06_PlayerStateSnapshot, packedMasterTypeColorFlags) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(NetPkt06_PlayerStateSnapshot, altGunAimOrigin) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(NetPkt06_PlayerStateSnapshot, storedTargetPos) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(NetPkt06_PlayerStateSnapshot, worldPos) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(NetPkt06_PlayerStateSnapshot, vehicleRotationAngles) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(NetPkt06_PlayerStateSnapshot, statusMeterValue) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(NetPkt06_PlayerStateSnapshot, progressTargetCount) == 0x44);
RECOIL_STATIC_ASSERT(sizeof(NetPkt0D_HudTimerPanelState) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(NetPkt0A_RemoveRuntimeRelay, optCatalogEntryId) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(NetPkt0A_RemoveRuntimeRelay, pointOrVec3) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(NetPkt0A_RemoveRuntimeRelay, ownerPlayerKey) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(NetPkt0A_RemoveRuntimeRelay) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(NetPkt07_AltGunDispatch, weaponId) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(NetPkt07_AltGunDispatch, dispatchFlags) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(NetPkt07_AltGunDispatch, targetPos) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(NetPkt07_AltGunDispatch) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(NetPkt0E_PlayerLapProgress, lapCountPacked) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(NetPkt0E_PlayerLapProgress, lapTimeSec) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(NetPkt0E_PlayerLapProgress) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(NetPkt0F_CraterEvent, eventFlags) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(NetPkt0F_CraterEvent, craterTypeId) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(NetPkt0F_CraterEvent, center) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(NetPkt0F_CraterEvent, radius) == 0x1c);
RECOIL_STATIC_ASSERT(sizeof(NetPkt0F_CraterEvent) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(NetPkt10_QSandEvent, eventFlags) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(NetPkt10_QSandEvent, center) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(NetPkt10_QSandEvent, radius) == 0x1c);
RECOIL_STATIC_ASSERT(sizeof(NetPkt10_QSandEvent) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(NetPkt0B_ChatMessage, messageLength) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(NetPkt0B_ChatMessage, message) == 0x0a);
RECOIL_STATIC_ASSERT(offsetof(NetPkt08_PlayerKillEvent, killMethodOrOptCatalogEntryId) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(NetPkt08_PlayerKillEvent, targetPlayerKey) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(NetPkt08_PlayerKillEvent) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(NetPkt09_PlayerScoreboardEntry, packedScoreAndLapCount) == 0x04);
RECOIL_STATIC_ASSERT(sizeof(NetPkt09_PlayerScoreboardEntry) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(NetPkt09_PlayerScoreboardSnapshot, entryCount) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(NetPkt09_PlayerScoreboardSnapshot, entries) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(NetPkt14_HudTimerAndFlagsSync, eventCode) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(NetPkt14_HudTimerAndFlagsSync, auxParam) == 0x0a);
RECOIL_STATIC_ASSERT(offsetof(NetPkt14_HudTimerAndFlagsSync, valueOrTime) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(NetPkt14_HudTimerAndFlagsSync, statusFlags) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(NetPkt14_HudTimerAndFlagsSync) == 0x14);
#endif
