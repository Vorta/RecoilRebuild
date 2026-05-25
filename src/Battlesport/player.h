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
struct OptCatalogEntryDef;
struct OptCatalogHitEventPartial;

extern "C" {
extern int g_Player_HudCounterValue;
extern zUtil_SaveGameState *g_PlayerSaveStateListHead;
extern zUtil_SaveGameState *g_PlayerSaveStateListTail;
extern int g_PlayerSaveStateListAux;
extern int g_PlayerSaveStateCount;
extern zUtil_SaveGameState *g_LocalPlayerSaveState;
extern zVec3 g_Player_AmphibBasisUpRef;
extern float g_Player_AmphibSteerBasisLerpRate;
extern zInput_GameStateOrMapTablePartial *g_GameStateOrMapTable;
extern int g_Player_NextOrdinal;
extern int g_Player_AiMode2State1Finalized;
extern float g_Player_TotalTimeSecScaled;
extern int g_PlayerPendingCheckpointNumber;
extern float g_PlayerStatusMeterRatio;
extern float g_Player_NominalGravity;
extern float g_Player_WaterGravity;
extern float g_Player_QuicksandGravity;
extern float g_Player_QuicksandSinkRate;
extern float g_Player_LavaSinkRate;
extern float g_Player_MaxSlope;
extern float g_Player_CollisionContactResolveScale;
extern HudUiElement g_Player_UnderwaterFxPass3Ui;
extern HudUiElement g_Player_State7FxPass3Ui;
extern OptCatalogEntryDef *g_Player_MakeHotOptEntry;
extern zEffectAnimEntry *g_Player_BftSplashAnimEntry;
extern zEffectAnimEntry *g_Player_ActiveDebugScriptAsyncEntry;
extern int g_Player_HorizonNodeFollowCameraEnabled;
extern zClass_NodePartial *g_Player_HorizonNode;
extern int g_PlayerPrevCameraState;
extern int g_PlayerPrevSteeringMode;
extern int g_Player_SavedSteeringMode;
extern zClass_NodePartial *g_Player_CopterHealthyNode1;
extern zClass_NodePartial *g_Player_CopterHealthyNode2;
extern zClass_NodePartial *g_Player_CopterSndNode1;
extern zClass_NodePartial *g_Player_CopterSndNode2;
extern zSndSample *g_Player_CopterSndSample;
extern int g_PlayerEnvProbeSampleCount;
extern zVec3 g_PlayerEnvProbeWorldPoints[7];
extern int g_PlayerEnvProbe_AboveGroundFlags[10];
extern int g_PlayerEnvProbe_AboveGroundIndices[10];
extern int g_PlayerEnvProbe_AboveGroundCount;
extern zEffectAnimEntry *g_PlayerRecentHitFxAnimEntry;
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
    char optCatalogName[0x50];
    int missionRequirementOrGateId;
    int mountLayoutFlags;
    float startAmmoOrCharge;
    float dispatchRepeatDelay;
    float aiAttackRangeMin;
    float aiAttackRangeMax;
    int fireSlotRecoilFlags;
    int initialHardpointSelectState;
};

struct PlayerAiRuntimePartial {
    unsigned char unknown_00[0x48];
    int attackBuddyNetId;
};

struct PlayerMasterCommonData {
    PlayerMasterCommonData *next;
    unsigned char unknown_004[0x2d4];
    int naniteBuildRate;
    int naniteSpawnCounter;
    int naniteMaxLevel;
    zSndSample *sfxWeaponUp[4];
    unsigned char unknown_2f4[0x98];
    float trackSwitchDist0;
    float trackSwitchDist1;
    float trackSwitchDist2;
    float maxHealth;
    float invMaxHealth;
    int pickupType;
    int pickupCapacity;
    int weaponSpecListAux;
    PlayerMasterWeaponSpec *weaponSpecHead;
    PlayerMasterWeaponSpec *weaponSpecTail;
    int weaponSpecCount;
    int weaponNodeCount;
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
    float gunPitchRate;
    float gunPitchMin;
    float frictionStatic;
    float frictionDynamic;
    float frictionSlide;
    float stoppingForce;
    float chassisSmoothFactor;
    float chassisPitchRate;
    float chassisPitchMax;
    float chassisPitchDamping;
    float chassisRollRate;
    float chassisRollMax;
    float chassisRollDamping;
    float quicksandSlowdown;
    float lavaSlowdown;
    zVec3 probePoints[23];
    int probePointCount;
    int platformPointCount;
    float mass;
    float invMass;
    float aDamping;
    float modeAltTransitionTime;
    float hoverLiftDampingRate;
    float hoverLiftScale;
    float hoverNormalLerpRate;
    float hoverPitchWaveBaseRate;
    float hoverPitchWaveSpeedRate;
    float hoverPitchWaveAmplitude;
    float hoverRollWaveBaseRate;
    float hoverRollWaveSpeedRate;
    float hoverRollWaveAmplitude;
    float hoverRollYawCoupleScale;
    float collisionDampingA;
    float collisionDampingB;
    zEffectAnimEntry *fxList_fromTrackToAmphib[2];
    zEffectAnimEntry *fxList_fromAmphibToTrack[2];
    zEffectAnimEntry *fxList_fromTrackToHover[2];
    zEffectAnimEntry *fxList_fromHoverToTrack[2];
    zEffectAnimEntry *fxList_fromSubToAmphib[2];
    zEffectAnimEntry *fxList_fromAmphibToSub[2];
    zEffectAnimEntry *fxList_fromHoverToAmphib[2];
    zEffectAnimEntry *fxList_fromAmphibToHover[2];
    zSndSample *sfxEngine[4];
    unsigned char unknown_2ac[0x08];
    float sfxPitchScale;
    float sfxVolumeScale;
};

struct PlayerProbeTypeHistogram {
    int countByImpactSlot[100];
};

struct PlayerEnvProbeResult {
    int bestIndexBySample[9];
    float candidateScoreBySample[9];
    float highestSelectedHitY;
    float minProbeDepth;
    int preferAttachmentSlot1;
    int attachmentCandidateCount;
    zClass_NodePartial *attachmentNode;
    int impactSlotBySample[9];
    PlayerProbeTypeHistogram hitHistogram;
    PlayerProbeSampleCandidateBuffer candidateBuffers[7];
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
    float currentLevel;
    float targetLevel;
    zClass_NodePartial *lightNode;
    float nextUpdateTime;
    zClass_NodePartial *lightParentNode;
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

struct PlayerMineSaveEntry {
    int resetMarker;
    char optCatalogName[0x20];
    zVec3 spawnPos;
    zVec3 scale;
    char ownerNodeName[0x24];
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
extern int g_Player_RuntimeInputFlags;
}

namespace Checkpoint {
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdatePlayerLapProgressAndNotifyNet(zUtil_SaveGameState *saveState, int checkpointIndex);
} // namespace Checkpoint

namespace PlayerPickupContact {
RECOIL_NOINLINE int RECOIL_FASTCALL
PassesCollectionTest(zUtil_SaveGameState *saveState, PlayerPendingContact *contact);
} // namespace PlayerPickupContact

namespace Player {
RECOIL_NOINLINE void RECOIL_FASTCALL AddScaledHudCounterValue(float value);
RECOIL_NOINLINE void RECOIL_FASTCALL
AiDiscardNegativeBranchPathNodes(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
AiEnterMode2SteeringPursuit(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL AiAlertAttackBuddies(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL AiRestorePreviousTopLevelState(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_CDECL AiFinalizeMode2State1ForAllPlayers();
RECOIL_NOINLINE void RECOIL_FASTCALL SetWorldPoseAndRestartAnchor(
    zUtil_SaveGameState *saveState, const zVec3 *position, float yawRad);
RECOIL_NOINLINE void RECOIL_FASTCALL
ResetMouseControlStateAndRecenterCursor(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL ResetMotionTransientState(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateBankVelocityFromSteerInput(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateAutoTurnAndSteerFromTarget(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
IntegrateYawAndWrapFromYawVelocity(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
RebuildSteerBasisFromMotionBasis(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
RebuildSteerBasisRawFromRef(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
RebuildMotionBasisFromSteerBasis(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
RebuildSteerBasisFromMotionAxes(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL ClearPendingContactQueues(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL ClassifyPendingContactsForSegment(
    zUtil_SaveGameState *saveState, PlayerProbeSampleCandidateBuffer *sceneResults,
    const zVec3 *segmentStart, const zVec3 *segmentEnd, int segmentTag);
RECOIL_NOINLINE int RECOIL_FASTCALL CollectPendingContactsForSegments(
    zUtil_SaveGameState *saveState, zClass_DiSegmentEndpoints *segmentPairs, int endpointCount,
    int *segmentTags);
RECOIL_NOINLINE int RECOIL_FASTCALL
CollectPendingCollisionContactsForQuadProbe(zUtil_SaveGameState *saveState, float expandRadius);
RECOIL_NOINLINE void RECOIL_FASTCALL BuildPendingContactQueues(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL ProcessPendingPickupContacts(
    zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
ApplyPendingCollisionProbeVelocity(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE int RECOIL_FASTCALL
TryResolvePendingCollisionProbeSweep(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
PreparePendingWorldCollisionResponse(zUtil_SaveGameState *saveState,
                                     PlayerPendingContact *worldContacts);
RECOIL_NOINLINE void RECOIL_FASTCALL
ResolvePendingWorldCollisionContact(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
ResolvePendingCollisionContact(zUtil_SaveGameState *saveState, PlayerPendingContact *contact);
RECOIL_NOINLINE void RECOIL_FASTCALL
ResolvePendingPlayerCollisionContact(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
ProcessTransferContactQueue(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL ProcessPendingContactQueues(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
SelectAndResolvePreferredPendingCollisionContact(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL ApplyPitchRollVelocityImpulseFromDirection(
    zUtil_SaveGameState *saveState, const zVec3 *direction, float angleScale,
    float velocityScale);
RECOIL_NOINLINE void RECOIL_FASTCALL
RecordRecentHitFeedback(zUtil_SaveGameState *saveState, OptCatalogEntryDef *hitSource,
                        float damage);
RECOIL_NOINLINE float RECOIL_FASTCALL
UpdateTimedHitStatusFromHitSource(zUtil_SaveGameState *saveState, OptCatalogEntryDef *hitSource,
                                  float damage);
RECOIL_NOINLINE void RECOIL_FASTCALL ClearDestroyedRespawnEffectHandleCallback(
    zEffectAnimEntry *entry, zUtil_SaveGameState *saveState, int value);
RECOIL_NOINLINE void RECOIL_CDECL DestroyedStateResetLocalFinalize();
RECOIL_NOINLINE void RECOIL_FASTCALL
DestroyedStateResetFinalizeCallback(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
DestroyedStateResetCallback(zEffectAnimEntry *entry, zUtil_SaveGameState *saveState, int value);
RECOIL_NOINLINE void RECOIL_FASTCALL
EnterLocalInactiveDestroyedLifecycle(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE int RECOIL_FASTCALL EnterDestroyedState(
    zUtil_SaveGameState *saveState, OptCatalogEntryDef *hitSource,
    OptCatalogHitEventPartial *hitRenderPoint, float damage);
RECOIL_NOINLINE int RECOIL_FASTCALL HitCallback_RecordContextAndTimedStatus(
    zUtil_SaveGameState *saveState, OptCatalogEntryDef *hitSource, void *hitRenderPointEntry,
    float damage);
RECOIL_NOINLINE void RECOIL_FASTCALL RecordNodeFlagsForRestore(zClass_NodePartial *node);
RECOIL_NOINLINE void RECOIL_FASTCALL BuildMissionSaveData(PlayerMissionSaveData *outData);
RECOIL_NOINLINE void RECOIL_FASTCALL ApplyMissionSaveData(PlayerMissionSaveData *saveData);
RECOIL_NOINLINE void RECOIL_CDECL RestoreRecordedNodeFlags();
RECOIL_NOINLINE void RECOIL_FASTCALL ZAR_ReadMissionSaveDataSection(
    zZbdSectionCallbackCtx *reader, const char *sectionToken, PlayerMissionSaveData *saveData,
    unsigned int byteCount, void *userData);
RECOIL_NOINLINE void RECOIL_CDECL ZAR_RegisterSections();
RECOIL_NOINLINE int RECOIL_FASTCALL ZAR_WriteMissionSaveDataSection(
    zZbdSectionCallbackCtx *writer, void *userData);
RECOIL_NOINLINE void RECOIL_FASTCALL ZAR_ReadVehicleListSection(
    zZbdSectionCallbackCtx *reader, const char *sectionToken,
    PlayerVehicleListSaveEntry *saveData, unsigned int byteCount, void *userData);
RECOIL_NOINLINE int RECOIL_FASTCALL ZAR_WriteVehicleListSection(
    zZbdSectionCallbackCtx *writer, void *userData);
RECOIL_NOINLINE void RECOIL_FASTCALL Mines_ZAR_ReadEntryOrReset(
    zZbdSectionCallbackCtx *reader, const char *sectionToken, PlayerMineSaveEntry *mineData,
    unsigned int byteCount, void *userData);
RECOIL_NOINLINE int RECOIL_FASTCALL WriteMinesZarSection(
    zZbdSectionCallbackCtx *writer, void *userData);
RECOIL_NOINLINE void RECOIL_FASTCALL RefreshHudFromState(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
ApplyStatusMeterChange(zUtil_SaveGameState *saveState, int mode, float delta);
RECOIL_NOINLINE int RECOIL_FASTCALL
UpdateStatusMeter(zUtil_SaveGameState *saveState, int mode, float delta);
RECOIL_NOINLINE int RECOIL_FASTCALL IsMissionProbeType1EnabledById(int missionId);
RECOIL_NOINLINE void RECOIL_FASTCALL
FreeAltWeaponTrailRuntimeStates(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
LoadWeaponBanksAndSelectDefaults(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
CheckMissionWeaponAvailability(zUtil_SaveGameState *saveState, int missionThreshold,
                               int packedWeaponSlotId, int *availableOut);
RECOIL_NOINLINE int RECOIL_STDCALL FloatSign(float value);
RECOIL_NOINLINE void RECOIL_FASTCALL StartSlipSfx(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL StopSlipSfx(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_CDECL CacheDisableCopterSndNodesAndStopSample();
RECOIL_NOINLINE void RECOIL_CDECL ReactivateCopterSndNodesIfHealthy();
RECOIL_NOINLINE void RECOIL_FASTCALL StopBftBubbleFxHandle(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE int RECOIL_FASTCALL TransitionToMasterTypeFly(zUtil_SaveGameState *saveState,
                                                              int flags);
RECOIL_NOINLINE int RECOIL_FASTCALL TransitionToMasterTypeTrack(zUtil_SaveGameState *saveState,
                                                                int flags);
RECOIL_NOINLINE int RECOIL_FASTCALL TransitionToMasterTypeAmphib(
    zUtil_SaveGameState *saveState, int transitionFlags, int extraFlags);
RECOIL_NOINLINE int RECOIL_FASTCALL TransitionToMasterTypeSub(zUtil_SaveGameState *saveState,
                                                              int flags);
RECOIL_NOINLINE int RECOIL_FASTCALL TransitionToMasterTypeHover(zUtil_SaveGameState *saveState,
                                                                int flags);
RECOIL_NOINLINE int RECOIL_FASTCALL
ApplyMasterTypeTransition(zUtil_SaveGameState *saveState, int masterType, int flags);
RECOIL_NOINLINE float RECOIL_FASTCALL UpdateBankAndTurnDynamics(
    zUtil_SaveGameState *saveState);
RECOIL_NOINLINE int RECOIL_FASTCALL Vec3_FastNormalize(zVec3 *vec);
RECOIL_NOINLINE void RECOIL_FASTCALL ConstrainToUnitDistanceFrom(zVec3 *pos,
                                                                 const zVec3 *center);
RECOIL_NOINLINE void RECOIL_FASTCALL ComputeTurnSlipDelta(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateSubModeWaterProbeState(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateSubVerticalDamping(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateYawVelocityFromSteerInput(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
ApplyAmphibSpeedOscillation(zUtil_SaveGameState *saveState, zVec3 *inOutUpVector,
                            int includeYawCoupling);
RECOIL_NOINLINE void RECOIL_FASTCALL
TickMasterTypeAndForceFeedback(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateMasterTypeSub(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateMasterTypeTrack(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE PlayerGunFireController *RECOIL_FASTCALL
FindAltGunFireControllerForWeaponId(zUtil_SaveGameState *saveState, int weaponId);
RECOIL_NOINLINE int RECOIL_FASTCALL
IsAltWeaponAllowedInCurrentMasterMode(zUtil_SaveGameState *saveState, OptCatalogEntryDef *entry);
RECOIL_NOINLINE void RECOIL_FASTCALL
AutoSwitchToNextUsableAltWeapon(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE int RECOIL_FASTCALL
TestScenePathBetweenCameraTargetAndPoint(zClass_NodePartial *node, const zVec3 *point,
                                         int directionMode);
RECOIL_NOINLINE void RECOIL_FASTCALL ApplyPrimaryWeaponSwitch(
    zUtil_SaveGameState *saveState, PlayerGunFireController *previousController,
    PlayerGunFireController *newController);
RECOIL_NOINLINE void RECOIL_FASTCALL ApplyAltWeaponSwitch(
    zUtil_SaveGameState *saveState, PlayerGunFireController *previousController,
    PlayerGunFireController *newController);
RECOIL_NOINLINE void RECOIL_FASTCALL HandleAltWeaponBankSelectInput(int inputCode);
RECOIL_NOINLINE void RECOIL_FASTCALL HandlePrimaryWeaponVariantToggleInput(int keyCode);
RECOIL_NOINLINE void RECOIL_FASTCALL
ResetDamageStateAndTimedHitStatus(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
ResetAltGunDoorAnimationState(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL ResetAltGunRuntimeState(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
StartDestroyedStateVehicleEffect(zUtil_SaveGameState *saveState, void *respawnCallback);
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateThirdPersonCamera(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL ApplyCameraState(int newState);
RECOIL_NOINLINE float RECOIL_FASTCALL SelectProbeSampleHeightFromCandidates(
    PlayerProbeSampleCandidateBuffer *candidateBuffer, int *outBestCandidateIndex,
    float sampleHeight, float maxRiseWindow, int preferAttachmentSlot1,
    int *outSelectedImpactSlot, float *outTaggedHeight);
RECOIL_NOINLINE void RECOIL_FASTCALL ProbeModalSampleHeights(
    zUtil_SaveGameState *saveState, float *outSampleHeightByPoint, float *outBestHeight,
    int preferAttachmentSlot1, PlayerProbeTypeHistogram *outTypeHistogram,
    int *outAttachmentCandidateCount, zClass_NodePartial **outAttachmentNode);
RECOIL_NOINLINE void RECOIL_FASTCALL BuildEnvironmentProbeResult(
    zUtil_SaveGameState *saveState, PlayerEnvProbeResult *outProbe);
RECOIL_NOINLINE int RECOIL_FASTCALL
ApplyEnvironmentProbeResult(zUtil_SaveGameState *saveState, PlayerEnvProbeResult *envProbe);
RECOIL_NOINLINE float RECOIL_FASTCALL
SolveHeightOnSurface(zUtil_SaveGameState *saveState, float supportPlaneDot);
RECOIL_NOINLINE void RECOIL_FASTCALL
ResetTerrainContactImpulsesAndPlayImpactSfx(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
ApplyTerrainTilt(zUtil_SaveGameState *saveState, const zVec3 *tiltVector, float tiltScale);
RECOIL_NOINLINE void RECOIL_FASTCALL
ComputeTriangleNormal(zUtil_SaveGameState *saveState, const zVec3 *pointA, const zVec3 *pointB,
                      const zVec3 *pointC);
RECOIL_NOINLINE void RECOIL_FASTCALL
ComputeSurfaceFrom1Probe(zUtil_SaveGameState *saveState, PlayerEnvProbeResult *probeResult);
RECOIL_NOINLINE void RECOIL_FASTCALL
ComputeSurfaceFrom2Probes(zUtil_SaveGameState *saveState, PlayerEnvProbeResult *probeResult);
RECOIL_NOINLINE int RECOIL_FASTCALL
CheckProbeSampleMaskOverlap(int sampleIndexA, int sampleIndexB, int sampleIndexC);
RECOIL_NOINLINE void RECOIL_FASTCALL RebuildAboveGroundIndices();
RECOIL_NOINLINE void RECOIL_FASTCALL
SelectBestProbesByDotProduct(const zVec3 *referenceNormal, PlayerEnvProbeResult *probeResult);
RECOIL_NOINLINE void RECOIL_FASTCALL
ComputeSurfaceFrom3Probes(zUtil_SaveGameState *saveState, PlayerEnvProbeResult *probeResult);
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdatePostMoveEnvironment(zUtil_SaveGameState *saveState, int probeSampleCount);
RECOIL_NOINLINE void RECOIL_FASTCALL
ProcessEnvProbeResults(zUtil_SaveGameState *saveState, PlayerEnvProbeResult *probeResult);
RECOIL_NOINLINE void RECOIL_FASTCALL
RebuildOrientationFromNormal(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
FindThirdProbeAndComputeNormal(zUtil_SaveGameState *saveState,
                               PlayerEnvProbeResult *probeResult);
RECOIL_NOINLINE void RECOIL_FASTCALL
AccumulateSlopeForces(zUtil_SaveGameState *saveState, PlayerEnvProbeResult *probeResult);
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateVerticalVelocityAndTransform(zUtil_SaveGameState *saveState,
                                   PlayerEnvProbeResult *probeResult);
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateMasterTypeBasicOrTrack_FromModalProbe(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateMasterTypeHover_FromModalProbe(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateMasterTypeHover(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateMasterTypeAmphib_FromModalProbe(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateMasterTypeAmphib(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateMasterTypeBasic(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL ComposeAimBasisWorldMatrix(zUtil_SaveGameState *saveState,
                                                                zMat4x3 *outMatrix34);
RECOIL_NOINLINE void RECOIL_FASTCALL DecayAndApplyAltFireSlotOffsetToNode(
    PlayerGunFireSlot *slot, zClass_NodePartial *slotNode, float slotAimY, int applyMatrix);
RECOIL_NOINLINE void RECOIL_FASTCALL
ApplyGunFireSlotOffsetToNode(zUtil_SaveGameState *saveState);
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
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterWeaponSpec, optCatalogName) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterWeaponSpec, missionRequirementOrGateId) == 0x54);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterWeaponSpec, mountLayoutFlags) == 0x58);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterWeaponSpec, startAmmoOrCharge) == 0x5c);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterWeaponSpec, dispatchRepeatDelay) == 0x60);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterWeaponSpec, aiAttackRangeMin) == 0x64);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterWeaponSpec, aiAttackRangeMax) == 0x68);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterWeaponSpec, fireSlotRecoilFlags) == 0x6c);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterWeaponSpec, initialHardpointSelectState) == 0x70);
RECOIL_STATIC_ASSERT(sizeof(PlayerMasterWeaponSpec) == 0x74);
RECOIL_STATIC_ASSERT(offsetof(PlayerAiRuntimePartial, attackBuddyNetId) == 0x48);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, next) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, naniteBuildRate) == 0x2d8);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, naniteSpawnCounter) == 0x2dc);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, naniteMaxLevel) == 0x2e0);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, maxHealth) == 0x398);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, invMaxHealth) == 0x39c);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, sfxWeaponUp) == 0x2e4);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, pickupType) == 0x3a0);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, trackSwitchDist0) == 0x38c);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, trackSwitchDist1) == 0x390);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, trackSwitchDist2) == 0x394);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, pickupCapacity) == 0x3a4);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, weaponSpecListAux) == 0x3a8);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, weaponSpecHead) == 0x3ac);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, weaponSpecTail) == 0x3b0);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, weaponSpecCount) == 0x3b4);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, weaponNodeCount) == 0x3b8);
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
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, probePoints) == 0x100);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, probePointCount) == 0x214);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, platformPointCount) == 0x218);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, modeAltTransitionTime) == 0x228);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, hoverLiftDampingRate) == 0x22c);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, hoverLiftScale) == 0x230);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, hoverNormalLerpRate) == 0x234);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, hoverPitchWaveBaseRate) == 0x238);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, hoverPitchWaveSpeedRate) == 0x23c);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, hoverPitchWaveAmplitude) == 0x240);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, hoverRollWaveBaseRate) == 0x244);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, hoverRollWaveSpeedRate) == 0x248);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, hoverRollWaveAmplitude) == 0x24c);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, hoverRollYawCoupleScale) == 0x250);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, collisionDampingA) == 0x254);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, collisionDampingB) == 0x258);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, fxList_fromTrackToAmphib) == 0x25c);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, fxList_fromAmphibToTrack) == 0x264);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, fxList_fromTrackToHover) == 0x26c);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, fxList_fromHoverToTrack) == 0x274);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, fxList_fromSubToAmphib) == 0x27c);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, fxList_fromAmphibToSub) == 0x284);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, fxList_fromHoverToAmphib) == 0x28c);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, fxList_fromAmphibToHover) == 0x294);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, sfxEngine) == 0x29c);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, sfxPitchScale) == 0x2b4);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, sfxVolumeScale) == 0x2b8);
RECOIL_STATIC_ASSERT(sizeof(PlayerProbeTypeHistogram) == 0x190);
RECOIL_STATIC_ASSERT(offsetof(PlayerEnvProbeResult, candidateScoreBySample) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(PlayerEnvProbeResult, highestSelectedHitY) == 0x48);
RECOIL_STATIC_ASSERT(offsetof(PlayerEnvProbeResult, minProbeDepth) == 0x4c);
RECOIL_STATIC_ASSERT(offsetof(PlayerEnvProbeResult, preferAttachmentSlot1) == 0x50);
RECOIL_STATIC_ASSERT(offsetof(PlayerEnvProbeResult, attachmentCandidateCount) == 0x54);
RECOIL_STATIC_ASSERT(offsetof(PlayerEnvProbeResult, attachmentNode) == 0x58);
RECOIL_STATIC_ASSERT(offsetof(PlayerEnvProbeResult, impactSlotBySample) == 0x5c);
RECOIL_STATIC_ASSERT(offsetof(PlayerEnvProbeResult, hitHistogram) == 0x80);
RECOIL_STATIC_ASSERT(offsetof(PlayerEnvProbeResult, candidateBuffers) == 0x210);
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
RECOIL_STATIC_ASSERT(offsetof(PlayerMineSaveEntry, optCatalogName) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(PlayerMineSaveEntry, spawnPos) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(PlayerMineSaveEntry, scale) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(PlayerMineSaveEntry, ownerNodeName) == 0x3c);
RECOIL_STATIC_ASSERT(sizeof(PlayerMineSaveEntry) == 0x60);
RECOIL_STATIC_ASSERT(sizeof(PlayerNodeFlagRestoreEntry) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(PlayerNodeFlagRestoreEntry, node) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(PlayerNodeFlagRestoreEntry, wasCellPickable) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(PlayerNodeFlagRestoreEntry, wasRaycastable) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(PlayerNodeFlagRestoreEntry, wasPickable) == 0x0c);

#endif // BATTLESPORT_PLAYER_H
