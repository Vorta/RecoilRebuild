#include "player.h"

#include "Battlesport/GameNet.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/zDi.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zUtil/zZbd.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "HudSensorTracker.h"
#include "OptCatalog.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
int g_Player_HudCounterValue = 0;
HudUiMgrSensorTrackList g_HudUiMgrSensor_TrackList = {0};
PlayerNodeFlagRestoreEntry *g_PlayerNodeFlagRestoreEntriesBegin = 0;
PlayerNodeFlagRestoreEntry *g_PlayerNodeFlagRestoreEntriesEnd = 0;
PlayerNodeFlagRestoreEntry *g_PlayerNodeFlagRestoreEntriesCapacityEnd = 0;
PlayerMasterCommonData *g_PlayerMasterCommonDataHead = 0;
PlayerMasterCommonData *g_PlayerMasterCommonDataTail = 0;
int g_PlayerMasterCommonDataListAux = 0;
int g_PlayerMasterCommonDataCount = 0;
PlayerMasterModalData *g_PlayerMasterModalDataHead = 0;
PlayerMasterModalData *g_PlayerMasterModalDataTail = 0;
int g_PlayerMasterModalDataListAux = 0;
int g_PlayerMasterModalDataCount = 0;
zUtil_SaveGameState *g_PlayerSaveStateListHead = 0;
zUtil_SaveGameState *g_PlayerSaveStateListTail = 0;
int g_PlayerSaveStateListAux = 0;
int g_PlayerSaveStateCount = 0;
zUtil_SaveGameState *g_LocalPlayerSaveState = 0;
int g_Player_NextOrdinal = 0;
int g_Player_AiMode2State1Finalized = 0;
float g_PlayerStatusMeterRatio = 0.0f;
HudUiElement g_Player_UnderwaterFxPass3Ui = {0};
HudUiElement g_Player_State7FxPass3Ui = {0};
zEffectAnimEntry *g_Player_ActiveDebugScriptAsyncEntry = 0;
int g_Player_HorizonNodeFollowCameraEnabled = 0;
zClass_NodePartial *g_Player_HorizonNode = 0;
}

// Reimplements 0x4385a0: Player::StartMasterTypeLoopSfxHandle
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE zSndPlayHandle *RECOIL_THISCALL
zUtil_SaveGameState::StartMasterTypeLoopSfxHandle(int modeIndex, float sfxVolume) {
    zUtil_SaveGameState *const saveState = this;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zVec3 *const worldPos = modeIndex != 3 ? &playerState->worldPos : 0;
    zSndSample *const sample = playerState->masterCommonData->sfxWeaponUp[modeIndex];
    zSndPlayHandle *const handle = sample->PlayA3D(worldPos, sfxVolume, 0);
    playerState->modeLoopSfxHandle[modeIndex] = handle;
    return handle;
}

// Reimplements 0x4385f0: Player::StartModalLoopSfxHandle
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL
zUtil_SaveGameState::StartModalLoopSfxHandle(int modalSfxIndex, float sfxVolume) {
    zUtil_SaveGameState *const saveState = this;
    PlayerModalState *const modalState = saveState->primaryModalState;
    zSndSample *const sample = modalState->masterModalData->sfxEngine[modalSfxIndex];
    zSndPlayHandle *const handle =
        sample->PlayA3D(&saveState->playerState->worldPos, sfxVolume, 0);
    saveState->primaryModalState->modalSfxHandle[modalSfxIndex] = handle;
}

// Reimplements 0x438690: Player::StopModalLoopSfxHandle
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL
zUtil_SaveGameState::StopModalLoopSfxHandle(int modalSfxIndex) {
    zUtil_SaveGameState *const saveState = this;
    zSndPlayHandle *const handle =
        saveState->primaryModalState->modalSfxHandle[modalSfxIndex];
    if (handle != 0) {
        handle->StopIfActive();
        saveState->primaryModalState->modalSfxHandle[modalSfxIndex] = 0;
    }
}

namespace Player {
namespace {
zVec3 TransformPointByMatrix(const zVec3 &point, const zMat4x3 &matrix) {
    zVec3 result = {0};
    result.x = point.x * matrix.xx + point.y * matrix.yx + point.z * matrix.zx + matrix.posX;
    result.y = point.x * matrix.xy + point.y * matrix.yy + point.z * matrix.zy + matrix.posY;
    result.z = point.x * matrix.xz + point.y * matrix.yz + point.z * matrix.zz + matrix.posZ;
    return result;
}

enum {
    kPlayerMasterTypeSub = 2,
    kPlayerMaxModalProbePoints = 4
};

} // namespace

// Reimplements 0x42a9f0: Player::AddScaledHudCounterValue
RECOIL_NOINLINE void RECOIL_FASTCALL AddScaledHudCounterValue(float value) {
    float scale = 1.0f;
    if (g_HudSensorTracker.primaryGunDispatchCount > 0) {
        scale = static_cast<float>(g_OptCatalog_DamageFeedbackHitCount) /
                static_cast<float>(g_HudSensorTracker.primaryGunDispatchCount);
    }

    g_Player_HudCounterValue += static_cast<int>(value * scale * 1000.0f);
}

// Reimplements 0x403830: Player::AiDiscardNegativeBranchPathNodes (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
AiDiscardNegativeBranchPathNodes(zUtil_SaveGameState *saveState) {
    AINetNode *aiCurrentPathNode = saveState->playerState->aiCurrentPathNode;
    if (aiCurrentPathNode == 0 || aiCurrentPathNode->nodeIndex >= 0) {
        return;
    }

    do {
        saveState->playerState->aiCurrentPathNode = aiCurrentPathNode->neighborNodes[0];
        aiCurrentPathNode->Free();
        aiCurrentPathNode = saveState->playerState->aiCurrentPathNode;
    } while (aiCurrentPathNode->nodeIndex < 0);
}

// Reimplements 0x402080: Player::AiRestorePreviousTopLevelState
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
AiRestorePreviousTopLevelState(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->aiTopLevelState = playerState->aiSavedTopLevelState;
}

// Reimplements 0x402f10: Player::AiFinalizeMode2State1ForAllPlayers
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_CDECL AiFinalizeMode2State1ForAllPlayers() {
    zUtil_SaveGameState *saveState = g_PlayerSaveStateListHead;
    while (saveState != 0) {
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        if (playerState->lifecycleState == 2 && playerState->aiTopLevelState == 1) {
            AiRestorePreviousTopLevelState(saveState);
        }

        saveState = saveState != 0 ? saveState->next : 0;
    }

    g_Player_AiMode2State1Finalized = 1;
}

// Reimplements 0x42be00: Player::SetWorldPoseAndRestartAnchor
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
SetWorldPoseAndRestartAnchor(zUtil_SaveGameState *saveState, const zVec3 *position,
                             float yawRad) {
    if (saveState == 0) {
        return;
    }

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->worldPos.x = position->x;
    playerState->worldPos.y = position->y;
    playerState->worldPos.z = position->z;
    playerState->restartYawRad = yawRad;
    playerState->previousTransform.posX = position->x;
    playerState->previousTransform.posY = position->y;
    playerState->previousTransform.posZ = position->z;
    zTag4::Clear(&g_VariantTag_Current);
    g_Variant_CurrentTag = g_VariantTag_Current;
    playerState->variantTag = g_VariantTag_Current;
}

// Reimplements 0x4283f0: Player::UpdateBankVelocityFromSteerInput
// (D:\Proj\GameZRecoil\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateBankVelocityFromSteerInput(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;

    playerState->restartYawRad = 0.0f;
    if (playerState->steeringInput == 0.0f) {
        playerState->localVel.x = 0.0f;
        return;
    }

    if ((playerState->steeringInputCopy > 0.0f && playerState->localVel.x > 0.0f) ||
        (playerState->steeringInputCopy < 0.0f && playerState->localVel.x < 0.0f)) {
        playerState->localVel.x = 0.0f;
    }

    playerState->localVel.x -=
        masterModalData->accelRate * g_Player_DeltaTime * playerState->steeringInputCopy;
}

// Reimplements 0x429750: Player::UpdateAutoTurnAndSteerFromTarget
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateAutoTurnAndSteerFromTarget(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;

    if (playerState->steeringInput == 0.0f) {
        float dampingScale = masterModalData->yawDamping * g_Player_DeltaTime;
        dampingScale = -dampingScale;
        int dampingBits = static_cast<int>(dampingScale * 12102200.0f);
        const int dampingFloatBits = dampingBits + 0x3f800000;

        float dampingFactor = 0.0f;
        memcpy(&dampingFactor, &dampingFloatBits, sizeof(dampingFactor));
        playerState->angVelYaw *= dampingFactor;
        return;
    }

    if ((playerState->steeringInputCopy > 0.0f && playerState->angVelYaw < 0.0f) ||
        (playerState->steeringInputCopy < 0.0f && playerState->angVelYaw > 0.0f)) {
        playerState->angVelYaw = 0.0f;
    }

    const float newYawVelocity =
        masterModalData->yawAccel * g_Player_DeltaTime * playerState->steeringInputCopy +
        playerState->angVelYaw;
    playerState->angVelYaw = newYawVelocity;

    if (newYawVelocity > playerState->yawVelocityLimit) {
        playerState->angVelYaw = playerState->yawVelocityLimit;
    } else if (newYawVelocity < -playerState->yawVelocityLimit) {
        playerState->angVelYaw = -playerState->yawVelocityLimit;
    }
}

// Reimplements 0x428490: Player::IntegrateYawAndWrapFromYawVelocity
// (D:\Proj\GameZRecoil\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
IntegrateYawAndWrapFromYawVelocity(zUtil_SaveGameState *saveState) {
    const float kTwoPi = 6.28318548f;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    if (playerState->autoTurnActive != 0) {
        playerState->restartYawRad = static_cast<float>(
            atan2(-playerState->autoTurnTargetDir.z, -playerState->autoTurnTargetDir.x));
        playerState->steeringInput = 0.0f;
        playerState->angVelYaw = 0.0f;
        playerState->autoTurnActive = 0;
    }

    UpdateAutoTurnAndSteerFromTarget(saveState);

    float yaw = playerState->restartYawRad + playerState->angVelYaw * g_Player_DeltaTime;
    playerState->restartYawRad = yaw;
    if (yaw < -kTwoPi) {
        playerState->restartYawRad = yaw + kTwoPi;
    } else if (yaw > kTwoPi) {
        playerState->restartYawRad = yaw - kTwoPi;
    }
}

// Reimplements 0x4294d0: Player::RebuildSteerBasisFromMotionBasis
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
RebuildSteerBasisFromMotionBasis(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    playerState->steerBasisRaw.x = -playerState->motionBasis.zx;
    playerState->steerBasisRaw.y = -playerState->motionBasis.zy;
    playerState->steerBasisRaw.z = -playerState->motionBasis.zz;

    playerState->steerBasisRef.x = playerState->motionBasis.yx;
    playerState->steerBasisRef.y = playerState->motionBasis.yy;
    playerState->steerBasisRef.z = playerState->motionBasis.yz;

    playerState->steerBasisNorm = playerState->steerBasisRaw;
    playerState->steerBasisNorm.y = 0.0f;
    zMath::Vec3NormalizeXZ(&playerState->steerBasisNorm, &playerState->steerBasisNorm);
}

// Reimplements 0x41f010: Player::BuildMissionSaveData
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL BuildMissionSaveData(PlayerMissionSaveData *outData) {
    zUtil_SaveGameState *const localSaveState = g_LocalPlayerSaveState;
    zUtil_PlayerStateStorage *const playerState = localSaveState->playerState;
    PlayerMasterModalData *const masterModalData =
        localSaveState->primaryModalState->masterModalData;

    outData->size = sizeof(PlayerMissionSaveData);
    {
    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        const PlayerAltWeaponBank &srcBank = playerState->altWeaponBanks[bankIndex];
        PlayerMissionSaveWeaponBank &dstBank = outData->weaponBank[bankIndex];

        dstBank.selectedSide = srcBank.selectedSide;
        const PlayerGunFireController *controllers[2] = {&srcBank.controllerA,
                                                         &srcBank.controllerB};

        {
        for (int sideIndex = 0; sideIndex < 2; ++sideIndex) {
            dstBank.sides[sideIndex].enabled = (controllers[sideIndex]->flags >> 2) & 1;
            dstBank.sides[sideIndex].ammoOrCharge = controllers[sideIndex]->ammoOrCharge;
        }
        }
    }
    }

    outData->altWeaponBankIndex = playerState->activeAltGunController->weaponBankIndex;
    outData->altWeaponSideIndex = playerState->activeAltGunController->weaponSideIndex;
    outData->primaryWeaponBankIndex = playerState->activePrimaryGunController->weaponBankIndex;
    outData->primaryWeaponSideIndex = playerState->activePrimaryGunController->weaponSideIndex;
    outData->playerStatusMeterRatio = g_PlayerStatusMeterRatio;
    outData->hudCounterValue = g_Player_HudCounterValue;
    outData->amphibUnlocked = playerState->amphibUnlocked;
    outData->hoverUnlocked = playerState->hoverUnlocked;
    outData->subUnlocked = playerState->subUnlocked;
    outData->aiMode = playerState->aiMode;
    outData->nextModeSwitchAllowedTime = playerState->nextModeSwitchAllowedTime;
    outData->motionInput = playerState->motionInput;
    outData->autoTurnSign = playerState->autoTurnSign;
    outData->bankInput = playerState->bankInput;
    outData->playerMasterType = masterModalData->masterType;

    zClass_Camera::gwCameraGetTarget(g_MainCamera, &outData->cameraTarget.x,
                                     &outData->cameraTarget.y, &outData->cameraTarget.z);
    zClass_Camera::gwCameraGetPosition(g_MainCamera, &outData->cameraPosition.x,
                                       &outData->cameraPosition.y, &outData->cameraPosition.z);

    memcpy(&outData->timedHitStatus, &playerState->timedHitStatus,
                sizeof(outData->timedHitStatus));

    if ((playerState->timedHitStatus.runtimeFlags & 1) != 0) {
        outData->timedHitStatus.lightNode = 0;
        outData->timedHitStatus.nextUpdateTime -= g_Time_AccumulatedTimeSec;
        outData->timedHitStatus.savedHitSourceEntryId =
            playerState->timedHitStatus.hitSource->ordinalIndex;
    }
}

// Reimplements 0x41efa0: Player::RestoreRecordedNodeFlags
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_CDECL RestoreRecordedNodeFlags() {
    PlayerNodeFlagRestoreEntry *entry = g_PlayerNodeFlagRestoreEntriesBegin;
    while (entry != g_PlayerNodeFlagRestoreEntriesEnd) {
        zClass_NodePartial *const node = entry->node;
        if (entry->wasCellPickable != 0) {
            zClass_Class::gwNodeSetCellPickable(node, 1);
        }
        if (entry->wasRaycastable != 0) {
            zClass_Class::gwNodeSetRaycastable(node, 1);
        }
        if (entry->wasPickable != 0) {
            zClass_Class::gwNodeSetPickable(node, 1);
        }
        ++entry;
    }
}

// Reimplements 0x41f5f0: Player::ZAR_WriteMissionSaveDataSection
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
ZAR_WriteMissionSaveDataSection(zZbdSectionCallbackCtx *writer, void *) {
    PlayerMissionSaveData missionData;
    zUtil_PlayerStateStorage *const playerState = g_LocalPlayerSaveState->playerState;

    BuildMissionSaveData(&missionData);
    missionData.lastValidCameraVariantTag = g_Variant_CurrentTag;
    return zUtil_ZAR::WriteSectionBlob(writer, playerState->rootNode->name, &missionData,
                                       sizeof(missionData));
}

// Reimplements 0x41f6a0: Player::ZAR_WriteVehicleListSection
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
ZAR_WriteVehicleListSection(zZbdSectionCallbackCtx *writer, void *) {
    int writeOk = 1;
    zUtil_SaveGameState *saveState = g_PlayerSaveStateListHead;
    while (saveState != 0 && writeOk != 0) {
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        PlayerVehicleListSaveEntry vehicleRecord;
        vehicleRecord.size = 128;
        vehicleRecord.worldPos = playerState->worldPos;
        vehicleRecord.vehicleRotationAngles = playerState->vehicleRotationAngles;
        vehicleRecord.aiNetId = playerState->aiNetId;
        vehicleRecord.aiTopLevelState = playerState->aiTopLevelState;
        vehicleRecord.aiSavedTopLevelState = playerState->aiSavedTopLevelState;
        vehicleRecord.aiReturnTopLevelState = playerState->aiReturnTopLevelState;
        vehicleRecord.aiAttackRadiusSq = playerState->aiAttackRadiusSq;
        vehicleRecord.aiRestoreDistanceSq = playerState->aiRestoreDistanceSq;
        vehicleRecord.aiRestoreTarget = playerState->aiRestoreTarget;
        vehicleRecord.aiDynamicOffsetDir = playerState->aiDynamicOffsetDir;
        vehicleRecord.aiActivationRadiusSq = playerState->aiActivationRadiusSq;
        vehicleRecord.aiTickSuppressed = playerState->aiTickSuppressed;
        vehicleRecord.aiAlertFlag = playerState->recentHitFlag;
        vehicleRecord.aiStateMarkerHandle = playerState->recentHitMarkerHandle;
        vehicleRecord.aiActive = playerState->aiActive;
        vehicleRecord.aiPathCursorAdvanceRequested = playerState->aiPathCursorAdvanceRequested;
        vehicleRecord.aiCurrentSteeringSubstate = playerState->aiCurrentSteeringSubstate;
        vehicleRecord.aiReturnSteeringSubstate = playerState->aiReturnSteeringSubstate;
        vehicleRecord.masterType = playerState->masterType;
        vehicleRecord.statusMeterScaled = playerState->statusMeterScaled;
        vehicleRecord.statusMeterValue = playerState->statusMeterValue;
        vehicleRecord.nanitePanelLevel = playerState->nanitePanelLevel;
        if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
            vehicleRecord.localMasterType = saveState->primaryModalState->masterModalData->masterType;
        }

        writeOk = zUtil_ZAR::WriteSectionBlob(writer, playerState->rootNode->name, &vehicleRecord,
                                              sizeof(vehicleRecord));
        saveState = saveState != 0 ? saveState->next : 0;
    }

    return writeOk;
}

// Reimplements 0x438b60: Player::FreeAltWeaponTrailRuntimeStates (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
FreeAltWeaponTrailRuntimeStates(zUtil_SaveGameState *saveState) {
    unsigned char *cursor = saveState->playerState->bytes + 0x740;
    for (int i = 0; i < 9; ++i) {
        void *const controllerATrail = *(void **)(cursor - 0x54);
        if (controllerATrail != 0) {
            OptCatalog::FreeTrailRuntimeStateStorage(controllerATrail);
        }

        void *const controllerBTrail = *(void **)(cursor);
        if (controllerBTrail != 0) {
            OptCatalog::FreeTrailRuntimeStateStorage(controllerBTrail);
        }

        cursor += 0xac;
    }
}

// Reimplements 0x426350: Player::FloatSign
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE int RECOIL_STDCALL FloatSign(float value) {
    if (value == 0.0f) {
        return 0;
    }

    if (value < 0.0f) {
        return -1;
    }

    return 1;
}

// Reimplements 0x429ed0: Player::StartSlipSfx
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL StartSlipSfx(zUtil_SaveGameState *saveState) {
    saveState->playerState->slipSfxActive = 1;
    saveState->StartModalLoopSfxHandle(3, 1.0f);
}

// Reimplements 0x429ef0: Player::StopSlipSfx
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL StopSlipSfx(zUtil_SaveGameState *saveState) {
    saveState->playerState->slipSfxActive = 0;
    saveState->StopModalLoopSfxHandle(3);
}

// Reimplements 0x429b40: Player::UpdateBankAndTurnDynamics
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE float RECOIL_FASTCALL
UpdateBankAndTurnDynamics(zUtil_SaveGameState *saveState) {
    if (g_Player_DeltaTime < 0.0000001) {
        return 0.0f;
    }

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;

    const float crossYaw = playerState->steerBasisNorm.x * playerState->bankBasis.z -
                           playerState->steerBasisNorm.z * playerState->bankBasis.x;
    const float slipDelta = crossYaw * -playerState->localVel.z * g_Player_InvDeltaTime +
                            playerState->motionBasis.xy * -28.0f;

    float residual = 0.0f;
    if (playerState->localVel.x == 0.0f) {
        if (fabs(slipDelta) <= masterModalData->frictionStatic) {
            return residual;
        }

        const int sign = slipDelta < 0.0f ? -1 : 1;
        residual = slipDelta - static_cast<float>(sign) * masterModalData->frictionStatic;
        StartSlipSfx(saveState);
        return residual;
    }

    residual = slipDelta -
               static_cast<float>(FloatSign(playerState->localVel.x)) *
                   masterModalData->frictionDynamic;

    if (playerState->throttleInputCopy != 0.0f &&
        FloatSign(playerState->steeringInputCopy) == FloatSign(playerState->restartYawRad)) {
        const int residualSign = residual < 0.0f ? -1 : 1;
        const int velocitySign = playerState->localVel.x < 0.0f ? -1 : 1;
        if (residualSign != velocitySign) {
            residual = 0.0f;
        }
    }

    if (playerState->slipSfxActive == 0 &&
        fabs(slipDelta) > masterModalData->frictionStatic) {
        StartSlipSfx(saveState);
    }

    return residual;
}

// Reimplements 0x429d30: Player::ComputeTurnSlipDelta
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ComputeTurnSlipDelta(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;

    playerState->localVel = playerState->projectileSpawnVel;

    const zVec3 localVel = playerState->localVel;
    const zMat4x3 &motionBasis = playerState->motionBasis;
    playerState->localVel.x =
        localVel.x * motionBasis.xx + localVel.y * motionBasis.xy +
        localVel.z * motionBasis.xz;
    playerState->localVel.y =
        localVel.x * motionBasis.yx + localVel.y * motionBasis.yy +
        localVel.z * motionBasis.yz;
    playerState->localVel.z =
        localVel.x * motionBasis.zx + localVel.y * motionBasis.zy +
        localVel.z * motionBasis.zz;

    const float axisClampRuntime = playerState->axisClampRuntime;
    playerState->localVel.z -=
        masterModalData->accelRate * playerState->throttleInputCopy * g_Player_DeltaTime;
    if (playerState->localVel.z > axisClampRuntime) {
        playerState->localVel.z = axisClampRuntime;
    } else if (playerState->localVel.z < -axisClampRuntime) {
        playerState->localVel.z = -axisClampRuntime;
    }

    float localX = playerState->localVel.x +
                   UpdateBankAndTurnDynamics(saveState) * g_Player_DeltaTime;
    if (playerState->localVel.x != 0.0f) {
        const int oldSign = playerState->localVel.x < 0.0f ? -1 : 1;
        const int newSign = localX < 0.0f ? -1 : 1;
        if (oldSign != newSign) {
            localX = 0.0f;
            StopSlipSfx(saveState);
        }
    }

    playerState->localVel.x = localX;
    if (localX > axisClampRuntime) {
        playerState->localVel.x = axisClampRuntime;
    } else if (localX < -axisClampRuntime) {
        playerState->localVel.x = -axisClampRuntime;
    }
}

// Reimplements 0x429870: Player::UpdateYawVelocityFromSteerInput
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateYawVelocityFromSteerInput(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;

    if (fabs(playerState->localVel.x) < g_Player_DeltaTimeScaled001) {
        playerState->localVel.x = 0.0f;
    }

    if (fabs(playerState->localVel.z) < g_Player_DeltaTimeScaled001) {
        playerState->localVel.z = 0.0f;
    }

    if (playerState->slipSfxActive != 0) {
        ComputeTurnSlipDelta(saveState);
        return;
    }

    if (playerState->throttleInput != 0.0f) {
        float dampingScale = masterModalData->rateDampingDecel * g_Player_DeltaTime;
        dampingScale = -dampingScale;
        int dampingBits = static_cast<int>(dampingScale * 12102200.0f);
        const int dampingFloatBits = dampingBits + 0x3f800000;

        float dampingFactor = 0.0f;
        memcpy(&dampingFactor, &dampingFloatBits, sizeof(dampingFactor));
        playerState->localVel.z *= dampingFactor;
    } else {
        if ((playerState->throttleInputCopy > 0.0f && playerState->localVel.z > 0.0f) ||
            (playerState->throttleInputCopy < 0.0f && playerState->localVel.z < 0.0f)) {
            float dampingScale = masterModalData->rateDampingDecel * g_Player_DeltaTime;
            dampingScale = -dampingScale;
            int dampingBits = static_cast<int>(dampingScale * 12102200.0f);
            const int dampingFloatBits = dampingBits + 0x3f800000;

            float dampingFactor = 0.0f;
            memcpy(&dampingFactor, &dampingFloatBits, sizeof(dampingFactor));
            playerState->localVel.z *= dampingFactor;
        }

        playerState->localVel.z -=
            masterModalData->accelRate * g_Player_DeltaTime * playerState->throttleInputCopy;
        const float velocityLimit = static_cast<float>(fabs(playerState->throttleInputCopy)) *
                                    playerState->axisClampRuntime;
        if (playerState->localVel.z > velocityLimit) {
            playerState->localVel.z = velocityLimit;
        } else if (playerState->localVel.z < -velocityLimit) {
            playerState->localVel.z = -velocityLimit;
        }
    }

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        const float residual = UpdateBankAndTurnDynamics(saveState);
        if (residual != 0.0f) {
            const float oldLocalX = playerState->localVel.x;
            float localX = oldLocalX + residual * g_Player_DeltaTime;

            if (localX > playerState->axisClampRuntime) {
                localX = playerState->axisClampRuntime;
            } else if (localX < -playerState->axisClampRuntime) {
                localX = -playerState->axisClampRuntime;
            }

            if (oldLocalX != 0.0f && FloatSign(localX) != FloatSign(oldLocalX)) {
                playerState->localVel.x = 0.0f;
                return;
            }

            playerState->localVel.x = localX;
            return;
        }
    }

    if (playerState->localVel.x != 0.0f) {
        float dampingScale = masterModalData->rateDampingAccel * g_Player_DeltaTime;
        dampingScale = -dampingScale;
        int dampingBits = static_cast<int>(dampingScale * 12102200.0f);
        const int dampingFloatBits = dampingBits + 0x3f800000;

        float dampingFactor = 0.0f;
        memcpy(&dampingFactor, &dampingFloatBits, sizeof(dampingFactor));
        playerState->localVel.x *= dampingFactor;
    }
}

// Reimplements 0x43c9c0: Player::FindAltGunFireControllerForWeaponId
// (D:\Proj\GameZRecoil\Player\player_weapon.c)
RECOIL_NOINLINE PlayerGunFireController *RECOIL_FASTCALL
FindAltGunFireControllerForWeaponId(zUtil_SaveGameState *saveState, int weaponId) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    OptCatalogEntryDef *const entry = OptCatalog::FindEntryById(weaponId);

    for (int i = 2; i < 10; ++i) {
        PlayerAltWeaponBank &bank = playerState->altWeaponBanks[i];
        if (bank.controllerA.optCatalogEntry == entry) {
            return &bank.controllerA;
        }

        if (bank.controllerB.optCatalogEntry == entry) {
            return &bank.controllerB;
        }
    }

    return &playerState->altWeaponBanks[1].controllerA;
}

// Reimplements 0x401e50: Player::TestScenePathBetweenCameraTargetAndPoint
// (GameZRecoil/Player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
TestScenePathBetweenCameraTargetAndPoint(zClass_NodePartial *node, const zVec3 *point,
                                         int directionMode) {
    zUtil_PlayerStateStorage *const playerState =
        (zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState);

    zVec3 cameraTarget = {0};
    zClass_Camera::gwCameraGetTarget(g_MainCamera, &cameraTarget.x, &cameraTarget.y,
                                     &cameraTarget.z);

    g_Variant_CurrentTag = playerState->variantTag;
    zClass_Class::gwNodeSetRaycastable(node, 0);
    zClass_Class::gwNodeSetRaycastable(playerState->rootNode, 0);
    zClass_cls_di::SetBreakOnFirstCandidate(1);
    zClass_cls_di::SetStopAfterFirstHit(0x40000);

    zVec3 startPoint = {0};
    zVec3 endPoint = {0};
    if (directionMode == 1) {
        startPoint = cameraTarget;
        endPoint = *point;
    } else {
        startPoint = *point;
        endPoint = cameraTarget;
    }

    PlayerProbeSampleCandidateBuffer rayData = {0};
    const int raycastResult = zClass_cls_di::RaycastFindClosest(
        g_Player_RuntimeDiScene, &rayData, startPoint.x, startPoint.y, startPoint.z, endPoint.x,
        endPoint.y, endPoint.z);

    zClass_cls_di::SetBreakOnFirstCandidate(0);
    zClass_Class::gwNodeSetRaycastable(playerState->rootNode, 1);
    zClass_Class::gwNodeSetRaycastable(node, 1);

    return raycastResult == 0 && rayData.candidateCount != 0 ? 0 : 1;
}

// Reimplements 0x4290f0: Player::SelectProbeSampleHeightFromCandidates
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE float RECOIL_FASTCALL SelectProbeSampleHeightFromCandidates(
    PlayerProbeSampleCandidateBuffer *candidateBuffer, int *outBestCandidateIndex,
    float sampleHeight, float maxRiseWindow, int preferAttachmentSlot1,
    int *outSelectedImpactSlot, float *outTaggedHeight) {
    float selectedHeight = -250.0f;
    float nearestFallbackHeight = -300.0f;
    int selectedImpactSlot = 0;

    *outBestCandidateIndex = 0;
    *outSelectedImpactSlot = 0;
    *outTaggedHeight = -300.0f;

    const int candidateCount = candidateBuffer->candidateCount;
    if (candidateCount <= 0) {
        return sampleHeight;
    }

    float bestAbsDelta = 10000.9f;
    for (int i = 0; i < candidateCount; ++i) {
        zClassDiPickCandidateEntry *const candidate = &candidateBuffer->entries[i];
        const float candidateHeight = candidate->hitPos.y;
        int impactSlot = 0;
        if (candidate->scenePayload != 0) {
            impactSlot = ((zModel_MaterialPartial *)candidate->scenePayload)->userTag;
        }

        if (impactSlot != 0) {
            *outTaggedHeight = candidateHeight;
            selectedImpactSlot = impactSlot;
            if (preferAttachmentSlot1 != 0 && impactSlot == 1) {
                continue;
            }
        }

        const float absDelta = static_cast<float>(fabs(candidateHeight - sampleHeight));
        if (absDelta < bestAbsDelta) {
            bestAbsDelta = absDelta;
            nearestFallbackHeight = candidateHeight;
        }

        if (candidateHeight > selectedHeight && candidateHeight - maxRiseWindow <= sampleHeight) {
            selectedHeight = candidateHeight;
            *outBestCandidateIndex = i;
        }
    }

    if (*outTaggedHeight + maxRiseWindow >= sampleHeight) {
        *outSelectedImpactSlot = selectedImpactSlot;
    }

    if (selectedHeight == -250.0f && nearestFallbackHeight != -300.0f) {
        return nearestFallbackHeight;
    }
    if (selectedHeight <= -250.0f) {
        return -250.0f;
    }
    return selectedHeight;
}

// Reimplements 0x428d60: Player::ProbeModalSampleHeights
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ProbeModalSampleHeights(
    zUtil_SaveGameState *saveState, float *outSampleHeightByPoint, float *outBestHeight,
    int preferAttachmentSlot1, PlayerProbeTypeHistogram *outTypeHistogram,
    int *outAttachmentCandidateCount, zClass_NodePartial **outAttachmentNode) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zUtil_PlayerStateStorage *const globalPlayerState =
        static_cast<zUtil_PlayerStateStorage *>(static_cast<void *>(g_GameStateOrMapTable->playerState));
    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    PlayerMasterModalData *const masterModalData = primaryModalState->masterModalData;

    memset(outTypeHistogram, 0, sizeof(*outTypeHistogram));
    zClass_Class::gwNodeSetCellPickable(playerState->rootNode, 0);
    zClass_Class::gwNodeSetCellPickable(globalPlayerState->rootNode, 0);

    const float probeYAdvance = playerState->projectileSpawnVel.y * g_Player_DeltaTime;
    const int probePointCount = primaryModalState->modalStateCode;
    for (int i = 0; i < probePointCount; ++i) {
        zVec3 transformed =
            TransformPointByMatrix(masterModalData->probePoints[i], playerState->motionBasis);
        if (masterModalData->masterType != kPlayerMasterTypeSub) {
            transformed.y += probeYAdvance;
        }
        primaryModalState->transformedProbePointWorldByIndex[i] = transformed;
    }

    float maxRiseWindow = 1.0f - probeYAdvance;
    if (maxRiseWindow > 4.0f) {
        maxRiseWindow = 4.0f;
    }

    zClass_Class::gwNodeSetCellPickable(playerState->rootNode, 0);
    zClass_Class::gwNodeSetCellPickable(globalPlayerState->rootNode, 0);
    g_Variant_CurrentTag = playerState->variantTag;

    PlayerProbeSampleCandidateBuffer candidateBuffers[kPlayerMaxModalProbePoints] = {};
    zClass_cls_di::BuildPickCandidatesForPointBatch(
        g_Player_RuntimeDiScene, primaryModalState->transformedProbePointWorldByIndex,
        probePointCount, 500.0f, candidateBuffers);

    g_Variant_CurrentTag = g_VariantTag_Current;
    zClass_Class::gwNodeSetCellPickable(playerState->rootNode, 1);
    zClass_Class::gwNodeSetCellPickable(globalPlayerState->rootNode, 1);

    *outBestHeight = -300.0f;
    *outAttachmentCandidateCount = 0;

    for (int i = 0; i < probePointCount; ++i) {
        int bestCandidateIndex = 0;
        int selectedImpactSlot = 0;
        float taggedHeight = -300.0f;
        PlayerProbeSampleCandidateBuffer *const candidateBuffer = &candidateBuffers[i];
        const float sampleHeight = primaryModalState->transformedProbePointWorldByIndex[i].y;

        outSampleHeightByPoint[i] = SelectProbeSampleHeightFromCandidates(
            candidateBuffer, &bestCandidateIndex, sampleHeight, maxRiseWindow,
            preferAttachmentSlot1, &selectedImpactSlot, &taggedHeight);

        if (*outBestHeight < taggedHeight) {
            *outBestHeight = taggedHeight;
        }

        if (i == 0) {
            if (candidateBuffers[0].candidateCount <= 0) {
                zClass_Class::gwNodeSetNodeType(playerState->rootNode, 0xff);
            } else {
                const zClassDiPickCandidateEntry *const selectedCandidate =
                    &candidateBuffers[0].entries[bestCandidateIndex];
                playerState->selectedProbeSample = *selectedCandidate;
                playerState->selectedProbeSample.hitPos.x =
                    primaryModalState->transformedProbePointWorldByIndex[0].x;
                playerState->selectedProbeSample.hitPos.z =
                    primaryModalState->transformedProbePointWorldByIndex[0].z;
                playerState->variantTag = selectedCandidate->variantTag;

                zClass_NodePartial *const worldChild =
                    zClass_Class::gwNodeGetWorldChild(selectedCandidate->node);
                if (worldChild != 0) {
                    zClass_Class::gwNodeSetNodeType(playerState->rootNode, worldChild->nodeType);
                } else {
                    zClass_Class::gwNodeSetNodeType(playerState->rootNode,
                                                    selectedCandidate->variantTag.tags[0]);
                }
            }
        }

        outTypeHistogram->countByImpactSlot[selectedImpactSlot] += 1;

        if (candidateBuffer->candidateCount != 0) {
            zClass_NodePartial *const candidateNode =
                candidateBuffer->entries[bestCandidateIndex].node;
            if (candidateNode != 0 && candidateNode->auxFlags != 0) {
                *outAttachmentCandidateCount += 1;
                *outAttachmentNode =
                    static_cast<zClass_NodePartial *>(candidateNode->callbackContext);
            }
        }
    }

    playerState->probeImpactSlot1SeenFlag = outTypeHistogram->countByImpactSlot[1] > 0 ? 1 : 0;
    playerState->probeImpactSlot4SeenFlag = outTypeHistogram->countByImpactSlot[4] > 0 ? 1 : 0;
}

// Reimplements 0x428350: Player::UpdateMasterTypeBasicOrTrack_FromModalProbe
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateMasterTypeBasicOrTrack_FromModalProbe(zUtil_SaveGameState *saveState) {
    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = primaryModalState->masterModalData;

    float sampleHeights[kPlayerMaxModalProbePoints] = {};
    float unusedBestHeight = 0.0f;
    PlayerProbeTypeHistogram unusedHistogram = {};
    int unusedAttachmentCandidateCount = 0;
    zClass_NodePartial *unusedAttachmentNode = 0;
    ProbeModalSampleHeights(saveState, sampleHeights, &unusedBestHeight, 0, &unusedHistogram,
                            &unusedAttachmentCandidateCount, &unusedAttachmentNode);

    playerState->yawVelocityLimit = masterModalData->yawRateMax;

    float maxSampleHeight = 0.0f;
    const int probePointCount = primaryModalState->modalStateCode;
    if (probePointCount > 0) {
        maxSampleHeight = sampleHeights[0];
        for (int i = 1; i < probePointCount; ++i) {
            if (maxSampleHeight < sampleHeights[i]) {
                maxSampleHeight = sampleHeights[i];
            }
        }
    }

    playerState->vehiclePitchRad = 0.0f;
    playerState->vehicleRollRad = 0.0f;
    playerState->worldPos.y = masterModalData->modeAltTransitionTime + maxSampleHeight;
}

// Reimplements 0x428120: Player::UpdateMasterTypeBasic
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateMasterTypeBasic(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;

    float savedLocalVelX = 0.0f;
    if (playerState->cameraState == 2) {
        UpdateBankVelocityFromSteerInput(saveState);
        savedLocalVelX = playerState->localVel.x;
    } else {
        IntegrateYawAndWrapFromYawVelocity(saveState);
    }

    zMath::MatBuildEulerRotation3x3(&playerState->motionBasis, playerState->vehiclePitchRad,
                                    playerState->restartYawRad, playerState->vehicleRollRad);
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;
    RebuildSteerBasisFromMotionBasis(saveState);

    playerState->axisClampRuntime = masterModalData->maxSpeed;
    UpdateYawVelocityFromSteerInput(saveState);
    if (playerState->cameraState == 2) {
        playerState->localVel.x = savedLocalVelX;
    }

    const float negSteerBasisX = -playerState->steerBasisNorm.x;
    const float negSteerBasisZ = -playerState->steerBasisNorm.z;
    const float worldVelX =
        negSteerBasisX * playerState->localVel.z + negSteerBasisZ * playerState->localVel.x;
    const float worldVelZ =
        playerState->steerBasisNorm.x * playerState->localVel.x +
        negSteerBasisZ * playerState->localVel.z;

    playerState->projectileSpawnVel.y = playerState->localVel.y;
    playerState->projectileSpawnVel.x = worldVelX;
    playerState->projectileSpawnVel.z = worldVelZ;

    playerState->worldPos.x += worldVelX * g_Player_DeltaTime;
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->worldPos.z += worldVelZ * g_Player_DeltaTime;
    playerState->motionBasis.posZ = playerState->worldPos.z;

    UpdateMasterTypeBasicOrTrack_FromModalProbe(saveState);

    zClass_Object3D::gwObject3DSetRotation(playerState->rootNode, playerState->vehiclePitchRad,
                                           playerState->restartYawRad,
                                           playerState->vehicleRollRad);
    zClass_Object3D::gwObject3DSetPosition(playerState->rootNode, playerState->worldPos.x,
                                           playerState->worldPos.y, playerState->worldPos.z);

    playerState->fxOffsetWorld.x = playerState->fxOffsetLocal.x + playerState->worldPos.x;
    playerState->fxOffsetWorld.y = playerState->fxOffsetLocal.y + playerState->worldPos.y;
    playerState->fxOffsetWorld.z = playerState->fxOffsetLocal.z + playerState->worldPos.z;

    zClass_Class::gwNodeUpdate(playerState->rootNode);
    memcpy(&playerState->previousTransform,
           zClass_Object3D::gwObject3DGetMatrixPtr(playerState->rootNode),
           sizeof(playerState->previousTransform));

    playerState->bankBasis = playerState->steerBasisNorm;
    playerState->cachedPitchRad = playerState->vehiclePitchRad;
    playerState->cachedYawRad = playerState->restartYawRad;
    playerState->cachedRollRad = playerState->vehicleRollRad;
}

// Reimplements 0x43afd0: Player::ComposeAimBasisWorldMatrix (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ComposeAimBasisWorldMatrix(zUtil_SaveGameState *saveState,
                                                                zMat4x3 *outMatrix34) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    zMat4x3 gunMatrix = {0};
    memcpy(&gunMatrix, zClass_Object3D::gwObject3DGetMatrixPtr(playerState->gunNode),
                sizeof(gunMatrix));

    zMat4x3 turretMatrix = {0};
    memcpy(&turretMatrix, zClass_Object3D::gwObject3DGetMatrixPtr(playerState->turretNode),
                sizeof(turretMatrix));

    zMat4x3 gunFireTransform = {0};
    memcpy(&gunFireTransform, &playerState->gunFireTransform, sizeof(gunFireTransform));

    outMatrix34->xx = turretMatrix.xx * gunFireTransform.xx + turretMatrix.xz * gunFireTransform.zx;
    outMatrix34->xy = turretMatrix.xx * gunFireTransform.xy + turretMatrix.xz * gunFireTransform.zy;
    outMatrix34->xz = turretMatrix.xx * gunFireTransform.xz + turretMatrix.xz * gunFireTransform.zz;

    const float yawX =
        turretMatrix.zx * gunFireTransform.xx + turretMatrix.zz * gunFireTransform.zx;
    const float yawY =
        turretMatrix.zx * gunFireTransform.xy + turretMatrix.zz * gunFireTransform.zy;
    const float yawZ =
        turretMatrix.zx * gunFireTransform.xz + turretMatrix.zz * gunFireTransform.zz;

    outMatrix34->yx = gunFireTransform.yx * gunMatrix.yy + gunMatrix.yz * yawX;
    outMatrix34->yy = gunFireTransform.yy * gunMatrix.yy + gunMatrix.yz * yawY;
    outMatrix34->yz = gunFireTransform.yz * gunMatrix.yy + gunMatrix.yz * yawZ;

    outMatrix34->zx = gunFireTransform.yx * gunMatrix.zy + gunMatrix.zz * yawX;
    outMatrix34->zy = gunFireTransform.yy * gunMatrix.zy + gunMatrix.zz * yawY;
    outMatrix34->zz = gunFireTransform.yz * gunMatrix.zy + gunMatrix.zz * yawZ;

    outMatrix34->posX = playerState->aimBasisOrigin.x;
    outMatrix34->posY = playerState->aimBasisOrigin.y;
    outMatrix34->posZ = playerState->aimBasisOrigin.z;
}

// Reimplements 0x43aa30: Player::SelectAltGunFirePointAndSlot (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SelectAltGunFirePointAndSlot(
    zUtil_SaveGameState *saveState, PlayerGunFireSlot **outActiveFireSlotPtr) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const activeAltGunController = playerState->activeAltGunController;

    if (playerState->gunNode == 0 || playerState->turretNode == 0) {
        const float y = playerState->worldPos.y + 1.0f;
        playerState->altFireOrigin.x = playerState->worldPos.x;
        playerState->altFireOrigin.y = y;
        playerState->altFireOrigin.z = playerState->worldPos.z;
        playerState->aimBasisOrigin.x = playerState->worldPos.x;
        playerState->aimBasisOrigin.y = y;
        playerState->aimBasisOrigin.z = playerState->worldPos.z;
        playerState->gunFireDir = playerState->steerBasisRaw;
        return;
    }

    zMat4x3 aimBasisWorldMatrix = {0};
    ComposeAimBasisWorldMatrix(saveState, &aimBasisWorldMatrix);

    switch (playerState->altHardpointSelectState) {
    case 0:
        if (activeAltGunController->attachState != 0) {
            playerState->altFireOrigin.x = aimBasisWorldMatrix.posX;
            playerState->altFireOrigin.y = aimBasisWorldMatrix.posY;
            playerState->altFireOrigin.z = aimBasisWorldMatrix.posZ;
        } else {
            playerState->altFireOrigin =
                TransformPointByMatrix(playerState->firePointCenter, aimBasisWorldMatrix);
        }
        playerState->altFireSlotCenter.attachNode = activeAltGunController->attachNodePrimary;
        *outActiveFireSlotPtr = &playerState->altFireSlotCenter;
        return;

    case 1:
        playerState->altFireOrigin =
            TransformPointByMatrix(playerState->firePointRight, aimBasisWorldMatrix);
        playerState->altFireSlotRight.attachNode = activeAltGunController->attachNodeSecondary;
        *outActiveFireSlotPtr = &playerState->altFireSlotRight;
        playerState->altHardpointSelectState = 2;
        return;

    case 2:
        playerState->altFireOrigin =
            TransformPointByMatrix(playerState->firePointLeft, aimBasisWorldMatrix);
        playerState->altFireSlotLeft.attachNode = activeAltGunController->attachNodePrimary;
        *outActiveFireSlotPtr = &playerState->altFireSlotLeft;
        playerState->altHardpointSelectState = 1;
        return;
    }
}

static void RemoveTrackNode(HudUiMgrSensorTrackNode *trackNode) {
    if (g_HudUiMgrSensor_TrackList.count != 0) {
        HudUiMgrSensorTrackNode *cursor = g_HudUiMgrSensor_TrackList.head;
        if (trackNode == cursor) {
            --g_HudUiMgrSensor_TrackList.count;
            g_HudUiMgrSensor_TrackList.head = trackNode->next;
            if (g_HudUiMgrSensor_TrackList.head == 0) {
                g_HudUiMgrSensor_TrackList.trackListAux = 0;
                g_HudUiMgrSensor_TrackList.tail = 0;
            }
        } else {
            while (cursor != 0) {
                HudUiMgrSensorTrackNode *const next = cursor->next;
                if (next == trackNode) {
                    --g_HudUiMgrSensor_TrackList.count;
                    cursor->next = trackNode->next;
                    if (g_HudUiMgrSensor_TrackList.tail == trackNode) {
                        g_HudUiMgrSensor_TrackList.tail = cursor;
                    }
                    break;
                }

                cursor = next;
            }
        }
    }
}

static void UnlinkSaveState(zUtil_SaveGameState *saveState) {
    if (g_PlayerSaveStateCount == 0) {
        return;
    }

    zUtil_SaveGameState *cursor = g_PlayerSaveStateListHead;
    if (saveState == cursor) {
        --g_PlayerSaveStateCount;
        g_PlayerSaveStateListHead = saveState->next;
        if (g_PlayerSaveStateListHead == 0) {
            g_PlayerSaveStateListAux = 0;
            g_PlayerSaveStateListTail = 0;
        }
        return;
    }

    while (cursor != 0) {
        zUtil_SaveGameState *const next = cursor->next;
        if (next == saveState) {
            --g_PlayerSaveStateCount;
            cursor->next = saveState->next;
            if (g_PlayerSaveStateListTail == saveState) {
                g_PlayerSaveStateListTail = cursor;
            }
            break;
        }

        cursor = next;
    }
}

// Reimplements 0x41fd20: Player::DestroySaveGameState (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL DestroySaveGameState(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    FreeAltWeaponTrailRuntimeStates(saveState);
    zClass_Node::ClearDamageHandler(playerState->rootNode);

    HudUiMgrSensorTrackNode *const trackNode =
        (HudUiMgrSensorTrackNode *)(playerState->rootNode->callbackContext);
    if (trackNode != 0) {
        RemoveTrackNode(trackNode);
        free(trackNode);
    }

    if (saveState != 0) {
        UnlinkSaveState(saveState);
        saveState->FreeOwnedResources();
        ::operator delete(saveState);
    }
}

static void DeleteRemainingTrackNodes() {
    HudUiMgrSensorTrackNode *node = g_HudUiMgrSensor_TrackList.head;
    while (node != 0) {
        HudUiMgrSensorTrackNode *const next = node->next;
        ::operator delete(node);
        node = next;
    }

    memset(&g_HudUiMgrSensor_TrackList, 0, sizeof(g_HudUiMgrSensor_TrackList));
}

static void DeleteWeaponSpecs(PlayerMasterCommonData *commonData) {
    PlayerMasterWeaponSpec *weaponSpec = commonData->weaponSpecHead;
    while (weaponSpec != 0) {
        PlayerMasterWeaponSpec *const next = weaponSpec->next;
        ::operator delete(weaponSpec);
        weaponSpec = next;
    }

    commonData->weaponSpecListAux = 0;
    commonData->weaponSpecTail = 0;
    commonData->weaponSpecHead = 0;
    commonData->weaponSpecCount = 0;
}

// Reimplements 0x41fb80: Player::ShutdownMissionRuntime (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_CDECL ShutdownMissionRuntime() {
    while (g_PlayerSaveStateListHead != 0) {
        DestroySaveGameState(g_PlayerSaveStateListHead);
    }

    DeleteRemainingTrackNodes();

    zUtil_SaveGameState *saveState = g_PlayerSaveStateListHead;
    g_PlayerSaveStateListAux = 0;
    g_PlayerSaveStateListTail = 0;
    g_PlayerSaveStateListHead = 0;
    g_PlayerSaveStateCount = 0;
    while (saveState != 0) {
        zUtil_SaveGameState *const next = saveState->next;
        saveState->FreeOwnedResources();
        ::operator delete(saveState);
        saveState = next;
    }

    PlayerMasterCommonData *commonData = g_PlayerMasterCommonDataHead;
    while (commonData != 0) {
        DeleteWeaponSpecs(commonData);
        commonData = commonData->next;
    }

    commonData = g_PlayerMasterCommonDataHead;
    while (commonData != 0) {
        PlayerMasterCommonData *const next = commonData->next;
        ::operator delete(commonData);
        commonData = next;
    }

    g_PlayerMasterCommonDataListAux = 0;
    g_PlayerMasterCommonDataTail = 0;
    g_PlayerMasterCommonDataHead = 0;
    g_PlayerMasterCommonDataCount = 0;

    PlayerMasterModalData *modalData = g_PlayerMasterModalDataHead;
    while (modalData != 0) {
        PlayerMasterModalData *const next = modalData->next;
        ::operator delete(modalData);
        modalData = next;
    }

    g_PlayerMasterModalDataListAux = 0;
    g_PlayerMasterModalDataTail = 0;
    g_PlayerMasterModalDataHead = 0;
    g_PlayerMasterModalDataCount = 0;

    AINet::FreeAll();
    g_Player_NextOrdinal = 0;
    g_GameStateOrMapTable = 0;
    ((HudUiContainer *)(&g_zVideo_FxPass3ConfigLocal))->RemoveChild(&g_Player_UnderwaterFxPass3Ui);
    ((HudUiContainer *)(&g_zVideo_FxPass3ConfigLocal))->RemoveChild(&g_Player_State7FxPass3Ui);
}
} // namespace Player
