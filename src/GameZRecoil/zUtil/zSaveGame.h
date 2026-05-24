#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/include/zDi.h"
#include "GameZRecoil/zMath/zMath.h"
#include "recoil/recoil_callconv.h"

struct AINetNode;
struct GameNetPlayerRow;
struct OptCatalogEntryDef;
struct PlayerMasterCommonData;
struct PlayerModalState;
struct zSndPlayHandle;

struct PlayerGunFireController {
    OptCatalogEntryDef *optCatalogEntry;
    int weaponBankIndex;
    int weaponSideIndex;
    zClass_NodePartial *attachNodePrimary;
    zClass_NodePartial *attachNodeSecondary;
    unsigned char unknown_14[0x14];
    void *attachState;
    int flags;
    unsigned char unknown_30[0x04];
    float ammoOrCharge;
    unsigned char unknown_38[0x1c];
};

struct PlayerAltWeaponBank {
    int selectedSide;
    PlayerGunFireController controllerA;
    PlayerGunFireController controllerB;
};

struct PlayerGunFireSlot {
    float recoilStrength;
    zClass_NodePartial *attachNode;
};

struct PlayerProgressTargetSlotRuntime {
    zVec3 *targetPos;
    zVec3 *targetVelocity;
};

struct zUtil_PlayerStateStorage {
    union {
        unsigned char bytes[0x10c4];
        struct {
            unsigned char unknown_0000[0x04];
            PlayerMasterCommonData *masterCommonData;
            unsigned char unknown_0008[0x18];
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
            int bankInput;
            unsigned char unknown_0044[0x1c];
            union {
                int lifecycleState;
                int masterType;
            };
            unsigned char unknown_0064[0x04];
            float throttleInput;
            float steeringInput;
            unsigned char unknown_0070[0x0c];
            float throttleInputCopy;
            float steeringInputCopy;
            unsigned char unknown_0084[0x10];
            float angVelYaw;
            unsigned char unknown_0098[0x0c];
            zVec3 projectileSpawnVel;
            zVec3 localVel;
            unsigned char unknown_00bc[0x0c];
            float axisClampRuntime;
            float yawVelocityLimit;
            unsigned char unknown_00d0[0x190];
            zMat4x3 motionBasis;
            unsigned char unknown_0290[0x60];
            zMat4x3 previousTransform;
            zMat4x3 gunFireTransform;
            unsigned char unknown_0350[0x30];
            zVec3 steerBasisNorm;
            zVec3 steerBasisRaw;
            zVec3 steerBasisRef;
            zVec3 bankBasis;
            unsigned char unknown_03b0[0x0c];
            union {
                zVec3 vehicleRotationAngles;
                struct {
                    float vehiclePitchRad;
                    float restartYawRad;
                    float vehicleRollRad;
                };
            };
            unsigned char unknown_03c8[0x0c];
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
            unsigned char unknown_03f8[0x0c];
            zVec3 fxOffsetLocal;
            zVec3 fxOffsetWorld;
            unsigned char unknown_041c[0x28];
            zTag4Partial variantTag;
            int spawnStateInitialized;
            zClassDiPickCandidateEntry selectedProbeSample;
            unsigned char unknown_0474[0x118];
            int cameraState;
            unsigned char unknown_0590[0x14];
            int altGunDispatchRequested;
            unsigned char unknown_05a8[0x14];
            int altGunFireHeldFlag;
            unsigned char unknown_05c0[0x04];
            struct {
                unsigned int runtimeFlags;
                OptCatalogEntryDef *hitSource;
                unsigned int unknown_08;
                unsigned int unknown_0c;
                zClass_NodePartial *lightNode;
                float nextUpdateTime;
                unsigned int unknown_18;
            } timedHitStatus;
            unsigned char unknown_05e0[0x04];
            PlayerGunFireController *activeAltGunController;
            PlayerGunFireController *activePrimaryGunController;
            PlayerAltWeaponBank altWeaponBanks[10];
            unsigned char unknown_0ca4[0xa0];
            int altHardpointSelectState;
            unsigned char unknown_0d48[0x18];
            int activeAltBankIndex;
            zVec3 gunFireDir;
            unsigned char unknown_0d70[0x10];
            zVec3 firePointCenter;
            zVec3 firePointRight;
            zVec3 firePointLeft;
            unsigned char unknown_0da4[0x0c];
            zVec3 altFireOrigin;
            unsigned char unknown_0dbc[0x20];
            int progressTargetCount;
            PlayerProgressTargetSlotRuntime progressTargetSlots[26];
            unsigned char unknown_0eb0[0x04];
            zVec3 aimBasisOrigin;
            zVec3 storedTargetPos;
            unsigned char unknown_0ecc[0x04];
            zClass_NodePartial *rootNode;
            unsigned char unknown_0ed4[0x08];
            zClass_NodePartial *turretNode;
            zClass_NodePartial *gunNode;
            unsigned char unknown_0ee4[0x24];
            PlayerGunFireSlot altFireSlotLeft;
            PlayerGunFireSlot altFireSlotRight;
            PlayerGunFireSlot altFireSlotCenter;
            unsigned char unknown_0f20[0x10];
            float statusMeterScaled;
            float statusMeterValue;
            int nanitePanelLevel;
            unsigned char unknown_0f3c[0x34];
            int aiNetId;
            int aiUnknown_0f74;
            AINetNode *aiCurrentPathNode;
            unsigned char unknown_0f7c[0x08];
            int aiTopLevelState;
            int aiSavedTopLevelState;
            int aiReturnTopLevelState;
            unsigned char unknown_0f90[0x20];
            float aiAttackRadiusSq;
            float aiRestoreDistanceSq;
            zVec3 aiRestoreTarget;
            zVec3 aiDynamicOffsetDir;
            unsigned char unknown_0fd0[0x04];
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
    int unknown_0c;
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
    int unknown_20;
    union {
        int unknown_24;
        GameNetPlayerRow *netPlayerRow;
    };
    unsigned char unknown_28[0x9c];

    RECOIL_NOINLINE void RECOIL_THISCALL FreeOwnedResources();
    RECOIL_NOINLINE zSndPlayHandle *RECOIL_THISCALL
    StartMasterTypeLoopSfxHandle(int modeIndex, float sfxVolume);
    RECOIL_NOINLINE void RECOIL_THISCALL StartModalLoopSfxHandle(int modalSfxIndex,
                                                                 float sfxVolume);
    RECOIL_NOINLINE void RECOIL_THISCALL StopModalLoopSfxHandle(int modalSfxIndex);
};

RECOIL_STATIC_ASSERT(sizeof(PlayerGunFireController) == 0x54);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, optCatalogEntry) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, weaponBankIndex) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, weaponSideIndex) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, attachNodePrimary) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, attachNodeSecondary) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, attachState) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, flags) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireController, ammoOrCharge) == 0x34);
RECOIL_STATIC_ASSERT(sizeof(PlayerAltWeaponBank) == 0xac);
RECOIL_STATIC_ASSERT(offsetof(PlayerAltWeaponBank, selectedSide) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(PlayerAltWeaponBank, controllerA) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(PlayerAltWeaponBank, controllerB) == 0x58);
RECOIL_STATIC_ASSERT(sizeof(PlayerGunFireSlot) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(PlayerProgressTargetSlotRuntime) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(PlayerGunFireSlot, attachNode) == 0x04);
RECOIL_STATIC_ASSERT(sizeof(zUtil_PlayerStateStorage) == 0x10c4);
RECOIL_STATIC_ASSERT(sizeof(zUtil_SaveGameState) == 0xc4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, masterCommonData) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, slipSfxActive) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, amphibUnlocked) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, bankInput) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, probeImpactSlot1SeenFlag) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, probeImpactSlot4SeenFlag) == 0x3c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, lifecycleState) == 0x60);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, throttleInput) == 0x68);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, steeringInput) == 0x6c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, throttleInputCopy) == 0x7c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, steeringInputCopy) == 0x80);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, angVelYaw) == 0x94);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, projectileSpawnVel) == 0xa4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, localVel) == 0xb0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, axisClampRuntime) == 0xc8);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, yawVelocityLimit) == 0xcc);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, motionBasis) == 0x260);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, previousTransform) == 0x2f0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, gunFireTransform) == 0x320);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, steerBasisNorm) == 0x380);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, steerBasisRaw) == 0x38c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, steerBasisRef) == 0x398);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, bankBasis) == 0x3a4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, vehicleRotationAngles) == 0x3bc);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, restartYawRad) == 0x3c0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cachedVehicleRotationAngles) == 0x3d4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, worldPos) == 0x3ec);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, fxOffsetLocal) == 0x404);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, fxOffsetWorld) == 0x410);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, variantTag) == 0x444);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, spawnStateInitialized) == 0x448);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, selectedProbeSample) == 0x44c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, cameraState) == 0x58c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, altGunDispatchRequested) == 0x5a4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, altGunFireHeldFlag) == 0x5bc);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, timedHitStatus) == 0x5c4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, activeAltGunController) == 0x5e4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, activePrimaryGunController) == 0x5e8);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, altWeaponBanks) == 0x5ec);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, altHardpointSelectState) == 0xd44);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, activeAltBankIndex) == 0xd60);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, gunFireDir) == 0xd64);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, firePointCenter) == 0xd80);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, firePointRight) == 0xd8c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, firePointLeft) == 0xd98);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, altFireOrigin) == 0xdb0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, progressTargetCount) == 0xddc);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, progressTargetSlots) == 0xde0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aimBasisOrigin) == 0xeb4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, storedTargetPos) == 0xec0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, rootNode) == 0xed0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, turretNode) == 0xedc);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, gunNode) == 0xee0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, altFireSlotLeft) == 0xf08);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, altFireSlotRight) == 0xf10);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, altFireSlotCenter) == 0xf18);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, statusMeterScaled) == 0xf30);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, statusMeterValue) == 0xf34);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, nanitePanelLevel) == 0xf38);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiNetId) == 0xf70);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiCurrentPathNode) == 0xf78);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiTopLevelState) == 0xf84);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiSavedTopLevelState) == 0xf88);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiReturnTopLevelState) == 0xf8c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiAttackRadiusSq) == 0xfb0);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiRestoreDistanceSq) == 0xfb4);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiRestoreTarget) == 0xfb8);
RECOIL_STATIC_ASSERT(offsetof(zUtil_PlayerStateStorage, aiDynamicOffsetDir) == 0xfc4);
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
RECOIL_STATIC_ASSERT(offsetof(zUtil_SaveGameState, modalStateListAux) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zUtil_SaveGameState, saveStateListHead) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zUtil_SaveGameState, saveStateListTail) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zUtil_SaveGameState, saveStateCount) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_SaveGameState, modalStateListHead) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zUtil_SaveGameState, modalStateListTail) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zUtil_SaveGameState, modalStateCount) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zUtil_SaveGameState, netPlayerRow) == 0x24);

RECOIL_NOINLINE zUtil_SaveGameState *RECOIL_FASTCALL
zUtil_SaveGameStateList_Init(zUtil_SaveGameState *self);
RECOIL_NOINLINE zUtil_SaveGameState *RECOIL_FASTCALL
zUtil_SaveGameStateList_AllocAppend(zUtil_SaveGameState *self);
