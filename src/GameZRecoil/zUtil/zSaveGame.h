#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/include/zDi.h"
#include "GameZRecoil/zMath/zMath.h"
#include "recoil/recoil_callconv.h"

struct AINetNode;
struct AINet;
struct GameNetPlayerRow;
struct OptCatalogEntryDef;
struct OptCatalogTrailRuntimeState;
struct PlayerAiRuntimePartial;
struct PlayerMasterCommonData;
struct PlayerModalState;
struct zEffectAnimEntry;
struct zSndPlayHandle;

struct PlayerGunFireController {
    OptCatalogEntryDef *optCatalogEntry;
    int weaponBankIndex;
    int weaponSideIndex;
    zClass_NodePartial *attachNodePrimary;
    zClass_NodePartial *attachNodeSecondary;
    zDiPartial *scrollTextureModelA;
    zDiPartial *scrollTextureModelB;
    float attachPosX;
    float attachPosY;
    float attachPosZ;
    void *attachState;
    int flags;
    int initialHardpointSelectState;
    float ammoOrCharge;
    int unknown_38;
    int unknown_3c;
    float nextDispatchTime;
    float dispatchRepeatDelay;
    float aiAttackRangeMin;
    float aiAttackRangeMax;
    OptCatalogTrailRuntimeState *trailRuntimeState;
};

struct PlayerAltWeaponBank {
    int selectedSide;
    PlayerGunFireController controllerA;
    PlayerGunFireController controllerB;
};

struct PlayerGunFireSlot {
    float offset;
    zClass_NodePartial *attachNode;
};

struct PlayerProgressTargetSlotRuntime {
    zVec3 *targetPos;
    zVec3 *targetVelocity;
};

struct PlayerPendingContact {
    zClassDiPickCandidateEntry hit;
    zVec3 sweepStart;
    zVec3 sweepEnd;
    int segmentTag;
    PlayerPendingContact *next;

    RECOIL_NOINLINE PlayerPendingContact *RECOIL_FASTCALL
    SelectPreferred(PlayerPendingContact *rhs);
};

struct PlayerPendingContactQueue {
    int listAux;
    PlayerPendingContact *head;
    PlayerPendingContact *tail;
    int count;
};

struct PlayerTimedHitStatus {
    unsigned int runtimeFlags;
    OptCatalogEntryDef *hitSource;
    float currentLevel;
    float targetLevel;
    zClass_NodePartial *lightNode;
    float nextUpdateTime;
    zClass_NodePartial *lightParentNode;

    RECOIL_NOINLINE void RECOIL_THISCALL ResetFields();
    RECOIL_NOINLINE void RECOIL_THISCALL ClearLightAndReset();
    RECOIL_NOINLINE int RECOIL_THISCALL TickAndUpdateLight(float hitStatus);
};

struct zUtil_PlayerStateStorage {
    union {
        unsigned char bytes[0x10c4];
        struct {
            int playerOrdinal;
            PlayerMasterCommonData *masterCommonData;
            int currentMasterType;
            float gravityAccel;
            int airborneFlag;
            int airborneFlagPrev;
            int damageProtectionActive;
            int queuedFixedDamageFlag;
            int slipSfxActive;
            int amphibUnlocked;
            int hoverUnlocked;
            int subUnlocked;
            int aiMode;
            union {
                float nextModeSwitchAllowedTime;
                int probeImpactSlot1SeenFlag;
            };
            int motionInput;
            union {
                int autoTurnSign;
                int probeImpactSlot4SeenFlag;
            };
            union {
                int bankInput;
                int amphibProbeCoverageFailed;
            };
            int netUpdateReceived;
            int recentHitValid;
            float recentHitDamage;
            void *lastHitOwnerOrCtx;
            OptCatalogEntryDef *recentHitSource;
            float recentHitFxExpireTime;
            zEffectAnimEntry *recentHitLightHandle;
            union {
                int lifecycleState;
                int masterType;
            };
            union {
                float primaryGunGateUntilTime;
                float masterTypeTransitionCooldownUntilTime;
            };
            float throttleInput;
            float steeringInput;
            float subVerticalInput;
            float subPitchInput;
            float joyCameraYawInput;
            float throttleInputCopy;
            float steeringInputCopy;
            float subVerticalInputCopy;
            float subPitchInputCopy;
            int underwaterStatusActive;
            float angVelPitch;
            float angVelYaw;
            float angVelRoll;
            unsigned char unknown_009c[0x08];
            zVec3 projectileSpawnVel;
            zVec3 localVel;
            zVec3 yawRotatedLocalVel;
            float axisClampRuntime;
            float yawVelocityLimit;
            int unknown_00d0;
            int unknown_00d4;
            int unknown_00d8;
            int unknown_00dc;
            int unknown_00e0;
            union {
                float autoTurnCursorNormX;
                float cursorNormX;
            };
            union {
                float autoTurnCursorNormY;
                float cursorNormY;
            };
            float cursorDeltaX;
            float cursorDeltaY;
            zVec3 modalProbeWorldByIndex[15];
            zVec3 rootProbeWorldByIndex[15];
            int environmentAttachmentActive;
            zMat4x3 motionBasis;
            zMat4x3 environmentAttachmentMatrix;
            unsigned char unknown_02c0[0x30];
            zMat4x3 previousTransform;
            zMat4x3 gunFireTransform;
            unsigned char unknown_0350[0x30];
            zVec3 steerBasisNorm;
            zVec3 steerBasisRaw;
            zVec3 steerBasisRef;
            zVec3 bankBasis;
            float pitchPoseCache;
            float yawPoseCache;
            float rollPoseCache;
            union {
                zVec3 vehicleRotationAngles;
                zVec3 cameraState6BasePos;
                struct {
                    float vehiclePitchRad;
                    float restartYawRad;
                    float vehicleRollRad;
                };
            };
            zVec3 netReceivedAngles;
            union {
                zVec3 cachedVehicleRotationAngles;
                struct {
                    float cachedPitchRad;
                    float cachedYawRad;
                    float cachedRollRad;
                };
            };
            unsigned char unknown_03e0[0x0c];
            zVec3 worldPos;
            zVec3 netReceivedPos;
            zVec3 fxOffsetLocal;
            zVec3 fxOffsetWorld;
            unsigned char unknown_041c[0x0c];
            zVec3 cameraTarget;
            zVec3 cameraDir;
            float subModeProbeBestHeight;
            zTag4Partial variantTag;
            int spawnStateInitialized;
            zClassDiPickCandidateEntry selectedProbeSample;
            PlayerPendingContactQueue playerCollisionQueue;
            PlayerPendingContactQueue worldCollisionQueue;
            PlayerPendingContactQueue preferredCollisionQueue;
            PlayerPendingContactQueue pickupQueue;
            PlayerPendingContactQueue checkpointQueue;
            PlayerPendingContactQueue transferQueue;
            int noPendingContactsQueued;
            int pickupQueueProcessed;
            int playerCollisionResolved;
            int worldCollisionResolved;
            int preferredCollisionResolved;
            int checkpointLapProgressNotified;
            int collisionProbeResolved;
            unsigned char unknown_04f0[0x04];
            int cameraLerpActive;
            float cameraTargetDistance;
            float thirdPersonYawOffset;
            float cameraYOffset;
            float cameraState6YOffset;
            float cameraElevationOffset;
            float thirdPersonPositionYOffset;
            union {
                int underwaterFxEnabled;
                int cameraTickEnabled;
            };
            unsigned char unknown_0514[0x0c];
            zVec3 cameraDirNext;
            float thirdPersonSideOffset;
            float thirdPersonBaseYOffset;
            float cameraDistance;
            zVec3 cameraState2TargetOffset;
            union {
                struct {
                    float cameraConfigParam0;
                    float cameraConfigParam1;
                    float cameraConfigParam2;
                    float cameraConfigParam3;
                    float cameraConfigParam4;
                    float cameraConfigParam5;
                };
                struct {
                    float unknownCameraConfigParam0;
                    float unknownCameraConfigParam1;
                    float unknownCameraConfigParam2;
                    zVec3 cameraState6LocalOffset;
                };
            };
            zVec3 cameraLerpStart;
            zVec3 cameraLerpEnd;
            zVec3 cameraDirFlat;
            zVec3 cameraBasisCache;
            int cameraState;
            int previousCameraState;
            int cameraTransitionTimer;
            float cameraTransitionBlend;
            int unknown_059c;
            int transitionDamageSuppressed;
            int altGunDispatchRequested;
            int primaryGunDispatchRequested;
            int pendingAltCameraToggle;
            int netInputBit16Latch;
            int netInputBit17Latch;
            int altGunTriggerProcessFlag;
            int altGunFireHeldFlag;
            int damageVisualFlag;
            PlayerTimedHitStatus timedHitStatus;
            int altGunTransitionState;
            PlayerGunFireController *activeAltGunController;
            PlayerGunFireController *activePrimaryGunController;
            PlayerAltWeaponBank altWeaponBanks[10];
            unsigned char unknown_0ca4[0xa0];
            int altHardpointSelectState;
            int primaryHardpointSelectState;
            int cachedAltSelectionCode;
            int cachedPrimarySelectionCode;
            int altGunDispatchFlags;
            int primaryFireSlotIndex;
            int altFireSlotIndex;
            int activeAltBankIndex;
            union {
                zVec3 gunFireDir;
                zVec3 cameraObstructionDir;
            };
            zVec3 altGunAimOrigin;
            int usePresetGunFireDir;
            zVec3 firePointCenter;
            zVec3 firePointRight;
            zVec3 firePointLeft;
            unsigned char unknown_0da4[0x0c];
            zVec3 altFireOrigin;
            unsigned char unknown_0dbc[0x0c];
            zVec3 primaryFireOrigin;
            float aimPitchResult;
            float aimTargetDistanceApprox;
            int progressTargetCount;
            union {
                PlayerProgressTargetSlotRuntime progressTargetSlots[26];
                struct {
                    PlayerProgressTargetSlotRuntime progressTargetSlotsBeforeGunMatrixPos[25];
                    zVec3 gunNodeMatrixPos;
                };
            };
            zVec3 aimBasisOrigin;
            zVec3 storedTargetPos;
            unsigned char unknown_0ecc[0x04];
            zClass_NodePartial *rootNode;
            zClass_NodePartial *environmentAttachmentNode;
            zClass_NodePartial *bodyNode;
            zClass_NodePartial *turretNode;
            zClass_NodePartial *gunNode;
            zClass_NodePartial *doorLeftNode;
            zClass_NodePartial *doorRightNode;
            zClass_NodePartial *modeVariantNode;
            unsigned char unknown_0ef0[0x18];
            PlayerGunFireSlot altFireSlotLeft;
            PlayerGunFireSlot altFireSlotRight;
            PlayerGunFireSlot altFireSlotCenter;
            PlayerGunFireController *altGunTransitionController;
            unsigned char unknown_0f24[0x04];
            float altGunTransitionTimerA;
            float altGunTransitionTimerB;
            float statusMeterScaled;
            float statusMeterValue;
            int nanitePanelLevel;
            zEffectAnimEntry *regenSkinFxEntry;
            zEffectAnimEntry *masterTypeTransitionToAmphibNodeAction;
            zEffectAnimEntry *masterTypeTransitionToAmphibLightHandle;
            zEffectAnimEntry *masterTypeTransitionToSubLightHandle;
            zEffectAnimEntry *masterTypeTransitionToSubNodeAction;
            zEffectAnimEntry *masterTypeTransitionToTrackNodeAction;
            zEffectAnimEntry *masterTypeTransitionToTrackLightHandle;
            zEffectAnimEntry *napalmVehicleFxEntry;
            zEffectAnimEntry *shatterVehicleFxEntry;
            zEffectAnimEntry *shockVehicleFxEntry;
            zEffectAnimEntry *subTransitionFxEntry;
            zEffectAnimEntry *destroyedRespawnFxEntry;
            zEffectAnimEntry *destroyedRespawnAsyncHandle;
            int aiNetId;
            union {
                PlayerAiRuntimePartial *aiRuntime;
                AINet *aiNet;
            };
            AINetNode *aiCurrentPathNode;
            union {
                int aiUnknown_0f7c;
                AINetNode *aiHomePathNode;
            };
            int aiCurrentPathNeighborIndex;
            int aiTopLevelState;
            int aiSavedTopLevelState;
            int aiReturnTopLevelState;
            float aiStateUntilTime;
            float aiMode2AttackDwell;
            float aiNotPursuitDwell;
            float aiHideTime0;
            float aiHideTime1;
            float unknown_0fa4;
            float aiStateStartTime;
            float aiStateEndTime;
            float aiAttackRadiusSq;
            float aiRestoreDistanceSq;
            zVec3 aiRestoreTarget;
            zVec3 aiDynamicOffsetDir;
            float unknown_0fd0;
            float aiActivationRadiusSq;
            int aiTickSuppressed;
            int recentHitFlag;
            int recentHitMarkerHandle;
            float recentHitExpireTime;
            int aiActive;
            int aiPathCursorAdvanceRequested;
            int aiCurrentSteeringSubstate;
            int aiReturnSteeringSubstate;
            unsigned char unknown_0ff8[0x08];
            zVec3 autoTurnTargetDir;
            zVec3 autoTurnTargetWorldPos;
            int autoTurnActive;
            unsigned char unknown_101c[0x84];
            float lapTimeSec;
            unsigned char unknown_10a4[0x08];
            int lapCount;
            unsigned char unknown_10b0[0x04];
            zSndPlayHandle *modeLoopSfxHandle[4];
        };
    };
};

struct zUtil_SaveGameState {
    zUtil_SaveGameState *next;
    zUtil_PlayerStateStorage *playerState;
    union {
        zUtil_SaveGameState *firstSaveState;
        PlayerModalState *primaryModalState;
    };
    union {
        int unknown_0c;
        zUtil_SaveGameState *aiPeerRingNext;
    };
    union {
        int unknown_10;
        int modalStateListAux;
    };
    union {
        zUtil_SaveGameState *saveStateListHead;
        PlayerModalState *modalStateListHead;
    };
    union {
        zUtil_SaveGameState *saveStateListTail;
        PlayerModalState *modalStateListTail;
    };
    union {
        int saveStateCount;
        int modalStateCount;
    };
    float modeLoopBlend;
    union {
        int unknown_24;
        GameNetPlayerRow *netPlayerRow;
    };
    unsigned char unknown_28[0x9c];

    RECOIL_NOINLINE void RECOIL_THISCALL FreeOwnedResources();
    RECOIL_NOINLINE zSndPlayHandle *RECOIL_THISCALL
    StartMasterTypeLoopSfxHandle(int modeIndex, float sfxVolume);
    RECOIL_NOINLINE void RECOIL_THISCALL EnsureMasterTypeLoopSfxHandle(int modeIndex,
                                                                        float sfxVolume);
    RECOIL_NOINLINE void RECOIL_THISCALL StartModalLoopSfxHandle(int modalSfxIndex,
                                                                 float sfxVolume);
    RECOIL_NOINLINE void RECOIL_THISCALL StopMasterTypeLoopSfxHandle(int modeIndex);
    RECOIL_NOINLINE void RECOIL_THISCALL StopModalLoopSfxHandle(int modalSfxIndex);
    RECOIL_NOINLINE void RECOIL_THISCALL UpdateModalLoopSfx(int enabled);
    RECOIL_NOINLINE int RECOIL_THISCALL SelectModalStateByMasterType(int masterType);
};

RECOIL_STATIC_ASSERT(sizeof(PlayerGunFireController) == 0x54);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, optCatalogEntry) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, weaponBankIndex) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, weaponSideIndex) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, attachNodePrimary) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, attachNodeSecondary) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, scrollTextureModelA) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, scrollTextureModelB) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, attachPosX) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, attachPosY) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, attachPosZ) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, attachState) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, flags) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, initialHardpointSelectState) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, ammoOrCharge) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, nextDispatchTime) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, dispatchRepeatDelay) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, aiAttackRangeMin) == 0x48);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, aiAttackRangeMax) == 0x4c);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, trailRuntimeState) == 0x50);
RECOIL_STATIC_ASSERT(sizeof(PlayerAltWeaponBank) == 0xac);
RECOIL_STATIC_ASSERT(offsetof(PlayerAltWeaponBank, selectedSide) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(PlayerAltWeaponBank, controllerA) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(PlayerAltWeaponBank, controllerB) == 0x58);
RECOIL_STATIC_ASSERT(sizeof(PlayerGunFireSlot) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(PlayerProgressTargetSlotRuntime) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireSlot, offset) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireSlot, attachNode) == 0x04);
RECOIL_STATIC_ASSERT(sizeof(PlayerPendingContact) == 0x48);
RECOIL_STATIC_ASSERT(offsetof(PlayerPendingContact, next) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(PlayerPendingContact, sweepStart) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(PlayerPendingContact, sweepEnd) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(PlayerPendingContact, segmentTag) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(PlayerPendingContactQueue) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(PlayerPendingContactQueue, listAux) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(PlayerPendingContactQueue, head) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(PlayerPendingContactQueue, tail) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(PlayerPendingContactQueue, count) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zUtil_PlayerStateStorage) == 0x10c4);
RECOIL_STATIC_ASSERT(sizeof(zUtil_SaveGameState) == 0xc4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, playerOrdinal) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, masterCommonData) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, currentMasterType) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, gravityAccel) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, airborneFlag) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, airborneFlagPrev) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, damageProtectionActive) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, queuedFixedDamageFlag) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, slipSfxActive) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, amphibUnlocked) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, bankInput) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, amphibProbeCoverageFailed) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, recentHitValid) == 0x48);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, recentHitDamage) == 0x4c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, lastHitOwnerOrCtx) == 0x50);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, recentHitSource) == 0x54);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, recentHitFxExpireTime) == 0x58);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, recentHitLightHandle) == 0x5c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, netUpdateReceived) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, probeImpactSlot1SeenFlag) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, probeImpactSlot4SeenFlag) == 0x3c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, lifecycleState) == 0x60);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, primaryGunGateUntilTime) == 0x64);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, masterTypeTransitionCooldownUntilTime) ==
                     0x64);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, throttleInput) == 0x68);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, steeringInput) == 0x6c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, subVerticalInput) == 0x70);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, throttleInputCopy) == 0x7c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, steeringInputCopy) == 0x80);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, subVerticalInputCopy) == 0x84);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, underwaterStatusActive) == 0x8c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, angVelPitch) == 0x90);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, angVelYaw) == 0x94);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, angVelRoll) == 0x98);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, projectileSpawnVel) == 0xa4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, localVel) == 0xb0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, yawRotatedLocalVel) == 0xbc);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, axisClampRuntime) == 0xc8);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, yawVelocityLimit) == 0xcc);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, unknown_00d4) == 0xd4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, autoTurnCursorNormX) == 0xe4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, autoTurnCursorNormY) == 0xe8);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cursorNormX) == 0xe4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cursorNormY) == 0xe8);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cursorDeltaX) == 0xec);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cursorDeltaY) == 0xf0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, modalProbeWorldByIndex) == 0xf4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, rootProbeWorldByIndex) == 0x1a8);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, environmentAttachmentActive) == 0x25c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, motionBasis) == 0x260);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, environmentAttachmentMatrix) == 0x290);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, previousTransform) == 0x2f0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, gunFireTransform) == 0x320);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, steerBasisNorm) == 0x380);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, steerBasisRaw) == 0x38c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, steerBasisRef) == 0x398);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, bankBasis) == 0x3a4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, pitchPoseCache) == 0x3b0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, yawPoseCache) == 0x3b4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, rollPoseCache) == 0x3b8);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, vehicleRotationAngles) == 0x3bc);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cameraState6BasePos) == 0x3bc);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, restartYawRad) == 0x3c0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, netReceivedAngles) == 0x3c8);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cachedVehicleRotationAngles) == 0x3d4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, worldPos) == 0x3ec);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, netReceivedPos) == 0x3f8);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, fxOffsetLocal) == 0x404);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, fxOffsetWorld) == 0x410);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cameraTarget) == 0x428);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cameraDir) == 0x434);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, subModeProbeBestHeight) == 0x440);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, variantTag) == 0x444);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, spawnStateInitialized) == 0x448);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, selectedProbeSample) == 0x44c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, playerCollisionQueue) == 0x474);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, worldCollisionQueue) == 0x484);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, preferredCollisionQueue) == 0x494);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, pickupQueue) == 0x4a4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, checkpointQueue) == 0x4b4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, transferQueue) == 0x4c4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, noPendingContactsQueued) == 0x4d4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, pickupQueueProcessed) == 0x4d8);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, playerCollisionResolved) == 0x4dc);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, worldCollisionResolved) == 0x4e0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, preferredCollisionResolved) == 0x4e4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, checkpointLapProgressNotified) == 0x4e8);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, collisionProbeResolved) == 0x4ec);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cameraLerpActive) == 0x4f4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cameraTargetDistance) == 0x4f8);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, thirdPersonYawOffset) == 0x4fc);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cameraYOffset) == 0x500);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cameraState6YOffset) == 0x504);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cameraElevationOffset) == 0x508);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, thirdPersonPositionYOffset) == 0x50c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, underwaterFxEnabled) == 0x510);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cameraTickEnabled) == 0x510);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cameraDirNext) == 0x520);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, thirdPersonSideOffset) == 0x52c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, thirdPersonBaseYOffset) == 0x530);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cameraDistance) == 0x534);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cameraState2TargetOffset) == 0x538);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cameraConfigParam0) == 0x544);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cameraConfigParam5) == 0x558);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cameraState6LocalOffset) == 0x550);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cameraLerpStart) == 0x55c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cameraLerpEnd) == 0x568);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cameraDirFlat) == 0x574);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cameraBasisCache) == 0x580);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cameraState) == 0x58c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, previousCameraState) == 0x590);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cameraTransitionTimer) == 0x594);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, altGunDispatchRequested) == 0x5a4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, altGunFireHeldFlag) == 0x5bc);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, damageVisualFlag) == 0x5c0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, timedHitStatus) == 0x5c4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, altGunTransitionState) == 0x5e0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, activeAltGunController) == 0x5e4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, activePrimaryGunController) == 0x5e8);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, pendingAltCameraToggle) == 0x5ac);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, altWeaponBanks) == 0x5ec);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, altHardpointSelectState) == 0xd44);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, primaryHardpointSelectState) == 0xd48);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cachedAltSelectionCode) == 0xd4c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cachedPrimarySelectionCode) == 0xd50);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, activeAltBankIndex) == 0xd60);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, gunFireDir) == 0xd64);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cameraObstructionDir) == 0xd64);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, altGunAimOrigin) == 0xd70);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, usePresetGunFireDir) == 0xd7c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, firePointCenter) == 0xd80);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, firePointRight) == 0xd8c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, firePointLeft) == 0xd98);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, altFireOrigin) == 0xdb0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, primaryFireOrigin) == 0xdc8);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aimPitchResult) == 0xdd4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aimTargetDistanceApprox) == 0xdd8);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, progressTargetCount) == 0xddc);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, progressTargetSlots) == 0xde0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, gunNodeMatrixPos) == 0xea8);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aimBasisOrigin) == 0xeb4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, storedTargetPos) == 0xec0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, rootNode) == 0xed0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, environmentAttachmentNode) == 0xed4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, bodyNode) == 0xed8);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, turretNode) == 0xedc);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, gunNode) == 0xee0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, doorLeftNode) == 0xee4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, doorRightNode) == 0xee8);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, modeVariantNode) == 0xeec);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, altFireSlotLeft) == 0xf08);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, altFireSlotRight) == 0xf10);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, altFireSlotCenter) == 0xf18);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, altGunTransitionController) == 0xf20);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, altGunTransitionTimerA) == 0xf28);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, altGunTransitionTimerB) == 0xf2c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, statusMeterScaled) == 0xf30);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, statusMeterValue) == 0xf34);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, nanitePanelLevel) == 0xf38);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, regenSkinFxEntry) == 0xf3c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage,
                             masterTypeTransitionToAmphibNodeAction) == 0xf40);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage,
                             masterTypeTransitionToAmphibLightHandle) == 0xf44);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, masterTypeTransitionToSubLightHandle) ==
                     0xf48);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage,
                             masterTypeTransitionToSubNodeAction) == 0xf4c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage,
                             masterTypeTransitionToTrackNodeAction) == 0xf50);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, masterTypeTransitionToTrackLightHandle) ==
                     0xf54);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, napalmVehicleFxEntry) == 0xf58);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, shatterVehicleFxEntry) == 0xf5c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, shockVehicleFxEntry) == 0xf60);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, subTransitionFxEntry) == 0xf64);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, destroyedRespawnFxEntry) == 0xf68);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, destroyedRespawnAsyncHandle) == 0xf6c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiNetId) == 0xf70);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiRuntime) == 0xf74);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiCurrentPathNode) == 0xf78);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiHomePathNode) == 0xf7c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiCurrentPathNeighborIndex) == 0xf80);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiTopLevelState) == 0xf84);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiSavedTopLevelState) == 0xf88);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiReturnTopLevelState) == 0xf8c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiStateUntilTime) == 0xf90);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiMode2AttackDwell) == 0xf94);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiNotPursuitDwell) == 0xf98);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiHideTime0) == 0xf9c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiHideTime1) == 0xfa0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, unknown_0fa4) == 0xfa4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiStateStartTime) == 0xfa8);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiStateEndTime) == 0xfac);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiAttackRadiusSq) == 0xfb0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiRestoreDistanceSq) == 0xfb4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiRestoreTarget) == 0xfb8);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiDynamicOffsetDir) == 0xfc4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, unknown_0fd0) == 0xfd0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiActivationRadiusSq) == 0xfd4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiTickSuppressed) == 0xfd8);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, recentHitFlag) == 0xfdc);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, recentHitMarkerHandle) == 0xfe0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, recentHitExpireTime) == 0xfe4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiActive) == 0xfe8);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiPathCursorAdvanceRequested) == 0xfec);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiCurrentSteeringSubstate) == 0xff0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiReturnSteeringSubstate) == 0xff4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, autoTurnTargetDir) == 0x1000);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, autoTurnTargetWorldPos) == 0x100c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, autoTurnActive) == 0x1018);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, lapTimeSec) == 0x10a0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, modeLoopSfxHandle) == 0x10b4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, lapCount) == 0x10ac);
RECOIL_STATIC_ASSERT(offsetof(zUtil_SaveGameState, playerState) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zUtil_SaveGameState, firstSaveState) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zUtil_SaveGameState, primaryModalState) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zUtil_SaveGameState, aiPeerRingNext) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_SaveGameState, modalStateListAux) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zUtil_SaveGameState, saveStateListHead) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zUtil_SaveGameState, saveStateListTail) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zUtil_SaveGameState, saveStateCount) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_SaveGameState, modeLoopBlend) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zUtil_SaveGameState, modalStateListHead) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zUtil_SaveGameState, modalStateListTail) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zUtil_SaveGameState, modalStateCount) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_SaveGameState, netPlayerRow) == 0x24);

RECOIL_NOINLINE zUtil_SaveGameState *RECOIL_FASTCALL
zUtil_SaveGameStateList_Init(zUtil_SaveGameState *self);
RECOIL_NOINLINE zUtil_SaveGameState *RECOIL_FASTCALL
zUtil_SaveGameStateList_AllocAppend(zUtil_SaveGameState *self);
