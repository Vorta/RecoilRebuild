#ifndef BATTLESPORT_PLAYER_H
#define BATTLESPORT_PLAYER_H

#pragma once

#include "recoil/recoil_types.h"

#include "Battlesport/ainet.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zUtil/zSaveGame.h"
#include "recoil/recoil_callconv.h"

struct zInput_GameStateOrMapTablePartial;
struct zEffectAnimEntry;
struct PlayerProbeSampleCandidateBuffer;
struct PlayerProbeTypeHistogram;
struct zZbdSectionCallbackCtx;
struct zSndPlayHandle;
struct zSndSample;

extern "C" {
extern int g_Player_HudCounterValue;
extern zUtil_SaveGameState *g_PlayerSaveStateListHead;
extern zUtil_SaveGameState *g_PlayerSaveStateListTail;
extern int g_PlayerSaveStateListAux;
extern int g_PlayerSaveStateCount;
extern zUtil_SaveGameState *g_LocalPlayerSaveState;
extern zInput_GameStateOrMapTablePartial *g_GameStateOrMapTable;
extern int g_Player_NextOrdinal;
extern int g_Player_AiMode2State1Finalized;
extern float g_PlayerStatusMeterRatio;
extern HudUiElement g_Player_UnderwaterFxPass3Ui;
extern HudUiElement g_Player_State7FxPass3Ui;
extern zEffectAnimEntry *g_Player_ActiveDebugScriptAsyncEntry;
extern int g_Player_HorizonNodeFollowCameraEnabled;
extern zClass_NodePartial *g_Player_HorizonNode;
}

struct HudUiMgrSensorTrackNode {
    int trackKind;
    void *payload;
    HudUiMgrSensorTrackNode *next;
};

enum HudUiMgrSensorTrackKind {
    HUD_SENSOR_TRACK_KIND_PLAYER = 2,
    HUD_SENSOR_TRACK_KIND_TURRET = 3
};

struct HudUiMgrSensorTrackList {
    int trackListAux;
    HudUiMgrSensorTrackNode *head;
    HudUiMgrSensorTrackNode *tail;
    int count;
};

struct PlayerMasterWeaponSpec {
    PlayerMasterWeaponSpec *next;
};

struct PlayerMasterCommonData {
    PlayerMasterCommonData *next;
    unsigned char unknown_004[0x2e0];
    zSndSample *sfxWeaponUp[4];
    unsigned char unknown_2f4[0xa4];
    float maxHealth;
    unsigned char unknown_39c[0x0c];
    int weaponSpecListAux;
    PlayerMasterWeaponSpec *weaponSpecHead;
    PlayerMasterWeaponSpec *weaponSpecTail;
    int weaponSpecCount;
};

struct PlayerMasterModalData {
    PlayerMasterModalData *next;
    unsigned char unknown_004[0xa0];
    int masterType;
    float accelRate;
    float maxSpeed;
    float yawAccel;
    float yawRateMax;
    float yawDamping;
    float rateDampingAccel;
    float rateDampingDecel;
    unsigned char unknown_0c4[0x08];
    float frictionStatic;
    float frictionDynamic;
    unsigned char unknown_0d4[0xe0];
    zVec3 probePoints[4];
    unsigned char unknown_1e4[0x44];
    float modeAltTransitionTime;
    unsigned char unknown_22c[0x70];
    zSndSample *sfxEngine[4];
};

struct PlayerProbeTypeHistogram {
    int countByImpactSlot[100];
};

struct PlayerMissionSaveWeaponSide {
    int enabled;
    float ammoOrCharge;
};

struct PlayerMissionSaveWeaponBank {
    int selectedSide;
    PlayerMissionSaveWeaponSide sides[2];
};

struct PlayerMissionSaveTimedHitStatus {
    unsigned int runtimeFlags;
    unsigned int savedHitSourceEntryId;
    unsigned int unknown_08;
    unsigned int unknown_0c;
    zClass_NodePartial *lightNode;
    float nextUpdateTime;
    unsigned int unknown_18;
};

struct PlayerMissionSaveData {
    int size;
    int altWeaponBankIndex;
    int altWeaponSideIndex;
    int primaryWeaponBankIndex;
    int primaryWeaponSideIndex;
    PlayerMissionSaveWeaponBank weaponBank[10];
    float playerStatusMeterRatio;
    int hudCounterValue;
    int amphibUnlocked;
    int hoverUnlocked;
    int subUnlocked;
    int aiMode;
    float nextModeSwitchAllowedTime;
    int motionInput;
    int autoTurnSign;
    int bankInput;
    int playerMasterType;
    zVec3 cameraTarget;
    zVec3 cameraPosition;
    zTag4Partial lastValidCameraVariantTag;
    PlayerMissionSaveTimedHitStatus timedHitStatus;
};

struct PlayerVehicleListSaveEntry {
    int size;
    zVec3 vehicleRotationAngles;
    zVec3 worldPos;
    int aiNetId;
    int aiTopLevelState;
    int aiSavedTopLevelState;
    int aiReturnTopLevelState;
    float aiAttackRadiusSq;
    float aiRestoreDistanceSq;
    zVec3 aiRestoreTarget;
    zVec3 aiDynamicOffsetDir;
    float aiActivationRadiusSq;
    int aiTickSuppressed;
    int aiAlertFlag;
    int aiStateMarkerHandle;
    int aiActive;
    int aiPathCursorAdvanceRequested;
    int aiCurrentSteeringSubstate;
    int aiReturnSteeringSubstate;
    int masterType;
    float statusMeterScaled;
    float statusMeterValue;
    int nanitePanelLevel;
    int localMasterType;
};

struct PlayerNodeFlagRestoreEntry {
    zClass_NodePartial *node;
    int wasCellPickable;
    int wasRaycastable;
    int wasPickable;
};

extern "C" {
extern HudUiMgrSensorTrackList g_HudUiMgrSensor_TrackList;
extern PlayerNodeFlagRestoreEntry *g_PlayerNodeFlagRestoreEntriesBegin;
extern PlayerNodeFlagRestoreEntry *g_PlayerNodeFlagRestoreEntriesEnd;
extern PlayerNodeFlagRestoreEntry *g_PlayerNodeFlagRestoreEntriesCapacityEnd;
extern PlayerMasterCommonData *g_PlayerMasterCommonDataHead;
extern PlayerMasterCommonData *g_PlayerMasterCommonDataTail;
extern int g_PlayerMasterCommonDataListAux;
extern int g_PlayerMasterCommonDataCount;
extern PlayerMasterModalData *g_PlayerMasterModalDataHead;
extern PlayerMasterModalData *g_PlayerMasterModalDataTail;
extern int g_PlayerMasterModalDataListAux;
extern int g_PlayerMasterModalDataCount;
}

namespace Player {
RECOIL_NOINLINE void RECOIL_FASTCALL AddScaledHudCounterValue(float value);
RECOIL_NOINLINE void RECOIL_FASTCALL
AiDiscardNegativeBranchPathNodes(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL AiRestorePreviousTopLevelState(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_CDECL AiFinalizeMode2State1ForAllPlayers();
RECOIL_NOINLINE void RECOIL_FASTCALL SetWorldPoseAndRestartAnchor(
    zUtil_SaveGameState *saveState, const zVec3 *position, float yawRad);
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateBankVelocityFromSteerInput(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateAutoTurnAndSteerFromTarget(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
IntegrateYawAndWrapFromYawVelocity(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
RebuildSteerBasisFromMotionBasis(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL BuildMissionSaveData(PlayerMissionSaveData *outData);
RECOIL_NOINLINE void RECOIL_CDECL RestoreRecordedNodeFlags();
RECOIL_NOINLINE int RECOIL_FASTCALL ZAR_WriteMissionSaveDataSection(
    zZbdSectionCallbackCtx *writer, void *userData);
RECOIL_NOINLINE int RECOIL_FASTCALL ZAR_WriteVehicleListSection(
    zZbdSectionCallbackCtx *writer, void *userData);
RECOIL_NOINLINE void RECOIL_FASTCALL
FreeAltWeaponTrailRuntimeStates(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE int RECOIL_STDCALL FloatSign(float value);
RECOIL_NOINLINE void RECOIL_FASTCALL StartSlipSfx(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL StopSlipSfx(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE float RECOIL_FASTCALL UpdateBankAndTurnDynamics(
    zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL ComputeTurnSlipDelta(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateYawVelocityFromSteerInput(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE PlayerGunFireController *RECOIL_FASTCALL
FindAltGunFireControllerForWeaponId(zUtil_SaveGameState *saveState, int weaponId);
RECOIL_NOINLINE int RECOIL_FASTCALL
TestScenePathBetweenCameraTargetAndPoint(zClass_NodePartial *node, const zVec3 *point,
                                         int directionMode);
RECOIL_NOINLINE float RECOIL_FASTCALL SelectProbeSampleHeightFromCandidates(
    PlayerProbeSampleCandidateBuffer *candidateBuffer, int *outBestCandidateIndex,
    float sampleHeight, float maxRiseWindow, int preferAttachmentSlot1,
    int *outSelectedImpactSlot, float *outTaggedHeight);
RECOIL_NOINLINE void RECOIL_FASTCALL ProbeModalSampleHeights(
    zUtil_SaveGameState *saveState, float *outSampleHeightByPoint, float *outBestHeight,
    int preferAttachmentSlot1, PlayerProbeTypeHistogram *outTypeHistogram,
    int *outAttachmentCandidateCount, zClass_NodePartial **outAttachmentNode);
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateMasterTypeBasicOrTrack_FromModalProbe(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateMasterTypeBasic(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL ComposeAimBasisWorldMatrix(zUtil_SaveGameState *saveState,
                                                                zMat4x3 *outMatrix34);
RECOIL_NOINLINE void RECOIL_FASTCALL SelectAltGunFirePointAndSlot(
    zUtil_SaveGameState *saveState, PlayerGunFireSlot **outActiveFireSlotPtr);
RECOIL_NOINLINE void RECOIL_FASTCALL DestroySaveGameState(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_CDECL ShutdownMissionRuntime();
} // namespace Player

RECOIL_STATIC_ASSERT(offsetof(HudUiMgrSensorTrackNode, next) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(HudUiMgrSensorTrackNode, trackKind) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(HudUiMgrSensorTrackNode, payload) == 0x04);
RECOIL_STATIC_ASSERT(sizeof(HudUiMgrSensorTrackList) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(HudUiMgrSensorTrackList, trackListAux) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(HudUiMgrSensorTrackList, head) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(HudUiMgrSensorTrackList, tail) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(HudUiMgrSensorTrackList, count) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterWeaponSpec, next) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, next) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, maxHealth) == 0x398);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, sfxWeaponUp) == 0x2e4);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, weaponSpecListAux) == 0x3a8);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, weaponSpecHead) == 0x3ac);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, weaponSpecTail) == 0x3b0);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, weaponSpecCount) == 0x3b4);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, next) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, masterType) == 0xa4);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, accelRate) == 0xa8);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, maxSpeed) == 0xac);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, yawAccel) == 0xb0);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, yawRateMax) == 0xb4);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, yawDamping) == 0xb8);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, rateDampingAccel) == 0xbc);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, rateDampingDecel) == 0xc0);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, frictionStatic) == 0xcc);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, frictionDynamic) == 0xd0);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, probePoints) == 0x1b4);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, modeAltTransitionTime) == 0x228);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, sfxEngine) == 0x29c);
RECOIL_STATIC_ASSERT(sizeof(PlayerProbeTypeHistogram) == 0x190);
RECOIL_STATIC_ASSERT(sizeof(PlayerMissionSaveWeaponSide) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(PlayerMissionSaveWeaponBank) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(PlayerMissionSaveData, weaponBank) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(PlayerMissionSaveData, playerStatusMeterRatio) == 0xdc);
RECOIL_STATIC_ASSERT(offsetof(PlayerMissionSaveData, playerMasterType) == 0x104);
RECOIL_STATIC_ASSERT(offsetof(PlayerMissionSaveData, cameraTarget) == 0x108);
RECOIL_STATIC_ASSERT(offsetof(PlayerMissionSaveData, cameraPosition) == 0x114);
RECOIL_STATIC_ASSERT(offsetof(PlayerMissionSaveData, timedHitStatus) == 0x124);
RECOIL_STATIC_ASSERT(sizeof(PlayerMissionSaveData) == 0x140);
RECOIL_STATIC_ASSERT(offsetof(PlayerVehicleListSaveEntry, worldPos) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(PlayerVehicleListSaveEntry, aiNetId) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(PlayerVehicleListSaveEntry, aiAttackRadiusSq) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(PlayerVehicleListSaveEntry, aiRestoreTarget) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(PlayerVehicleListSaveEntry, aiDynamicOffsetDir) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(PlayerVehicleListSaveEntry, aiActivationRadiusSq) == 0x4c);
RECOIL_STATIC_ASSERT(offsetof(PlayerVehicleListSaveEntry, aiActive) == 0x5c);
RECOIL_STATIC_ASSERT(offsetof(PlayerVehicleListSaveEntry, masterType) == 0x6c);
RECOIL_STATIC_ASSERT(offsetof(PlayerVehicleListSaveEntry, localMasterType) == 0x7c);
RECOIL_STATIC_ASSERT(sizeof(PlayerVehicleListSaveEntry) == 0x80);
RECOIL_STATIC_ASSERT(sizeof(PlayerNodeFlagRestoreEntry) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(PlayerNodeFlagRestoreEntry, node) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(PlayerNodeFlagRestoreEntry, wasCellPickable) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(PlayerNodeFlagRestoreEntry, wasRaycastable) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(PlayerNodeFlagRestoreEntry, wasPickable) == 0x0c);

#endif // BATTLESPORT_PLAYER_H
