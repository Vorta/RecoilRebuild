#include "Battlesport/GameNet.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/include/zDi.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zUtil/zZbd.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace {
using TestBackendGetStatusFn = std::int32_t(__stdcall *)(void *self, std::int32_t *status);
using TestBackendPlayDirectSoundFn = std::int32_t(__stdcall *)(void *self,
                                                               std::uint32_t reserved1,
                                                               std::uint32_t reserved2,
                                                               std::uint32_t flags);
using TestBackendSetIntFn = std::int32_t(__stdcall *)(void *self, std::int32_t value);

struct TestDirectSoundBufferVTable {
    void *slots00_1c[8];
    void *GetFrequency;
    TestBackendGetStatusFn GetStatus;
    void *slot28;
    void *slot2c;
    TestBackendPlayDirectSoundFn Play;
    TestBackendSetIntFn SetCurrentPosition;
    void *slot38;
    TestBackendSetIntFn SetVolume;
};

struct TestDirectSoundBuffer {
    TestDirectSoundBufferVTable *vtable;
};

template <typename T> T *AllocZeroedMalloc() {
    void *const mem = std::calloc(1, sizeof(T));
    return static_cast<T *>(mem);
}

template <typename T> T *AllocZeroedNew() {
    T *const value = static_cast<T *>(::operator new(sizeof(T)));
    std::memset(value, 0, sizeof(T));
    return value;
}

void SetObjectLocalMatrix(zClass_Object3DDataPartial *data, const zMat4x3 &matrix) {
    std::memcpy(data->localMatrix, &matrix, sizeof(matrix));
}

bool Vec3Equals(const zVec3 &value, const zVec3 &expected) {
    return value.x == expected.x && value.y == expected.y && value.z == expected.z;
}

bool MatrixEquals(const zMat4x3 &value, const zMat4x3 &expected) {
    return value.xx == expected.xx && value.xy == expected.xy && value.xz == expected.xz &&
           value.yx == expected.yx && value.yy == expected.yy && value.yz == expected.yz &&
           value.zx == expected.zx && value.zy == expected.zy && value.zz == expected.zz &&
           value.posX == expected.posX && value.posY == expected.posY &&
           value.posZ == expected.posZ;
}

bool FloatNear(float actual, float expected) {
    return std::fabs(actual - expected) < 0.0001f;
}

float PlayerDampingFactor(float rate, float deltaTime) {
    const int bits = static_cast<int>(-rate * deltaTime * 12102200.0f) + 0x3f800000;
    float factor = 0.0f;
    std::memcpy(&factor, &bits, sizeof(factor));
    return factor;
}

std::int32_t __stdcall TestDirectSoundGetStatus(void *, std::int32_t *status) {
    *status = 0;
    return 0;
}

std::int32_t __stdcall TestDirectSoundSetInt(void *, std::int32_t) {
    return 0;
}

std::int32_t __stdcall TestDirectSoundPlay(void *, std::uint32_t, std::uint32_t,
                                           std::uint32_t) {
    return 0;
}
} // namespace

extern "C" int ainet_free_all_smoke(void) {
    AINet *const first = AllocZeroedMalloc<AINet>();
    AINet *const second = AllocZeroedMalloc<AINet>();
    AINetNode *const nodeA = AllocZeroedMalloc<AINetNode>();
    AINetNode *const nodeB = AllocZeroedMalloc<AINetNode>();

    nodeA->next = nodeB;
    first->nodeListHead = nodeA;
    first->next = second;
    g_AINetListHead = first;

    AINet::FreeAll();
    return g_AINetListHead == nullptr ? 0 : 1;
}

extern "C" int player_ai_discard_negative_branch_nodes_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    AINetNode *const negativeA = AllocZeroedMalloc<AINetNode>();
    AINetNode *const negativeB = AllocZeroedMalloc<AINetNode>();
    AINetNode positive = {};

    negativeA->nodeIndex = -2;
    negativeB->nodeIndex = -1;
    positive.nodeIndex = 3;
    negativeA->neighborNodes[0] = negativeB;
    negativeB->neighborNodes[0] = &positive;
    playerState.aiCurrentPathNode = negativeA;
    saveState.playerState = &playerState;

    Player::AiDiscardNegativeBranchPathNodes(&saveState);
    return playerState.aiCurrentPathNode == &positive ? 0 : 1;
}

extern "C" int player_find_alt_gun_controller_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    OptCatalogEntryDef entries[3] = {};
    entries[0].keyName = const_cast<char *>("weapon-a");
    entries[0].ordinalIndex = 101;
    entries[1].keyName = const_cast<char *>("weapon-b");
    entries[1].ordinalIndex = 202;
    entries[2].keyName = const_cast<char *>("other");
    entries[2].ordinalIndex = 303;

    const std::int32_t oldEntryCount = g_OptCatalog_EntryCount;
    OptCatalogEntryDef *const oldEntryTable = g_OptCatalog_EntryTable;
    g_OptCatalog_EntryCount = 3;
    g_OptCatalog_EntryTable = entries;

    playerState.altWeaponBanks[4].controllerA.optCatalogEntry = &entries[0];
    playerState.altWeaponBanks[5].controllerB.optCatalogEntry = &entries[1];
    playerState.altWeaponBanks[6].controllerA.optCatalogEntry = &entries[2];
    playerState.altWeaponBanks[6].controllerB.optCatalogEntry = &entries[2];

    const bool ctrlAFound = Player::FindAltGunFireControllerForWeaponId(&saveState, 101) ==
                            &playerState.altWeaponBanks[4].controllerA;
    const bool ctrlBFound = Player::FindAltGunFireControllerForWeaponId(&saveState, 202) ==
                            &playerState.altWeaponBanks[5].controllerB;
    const bool ctrlAPriority = Player::FindAltGunFireControllerForWeaponId(&saveState, 303) ==
                               &playerState.altWeaponBanks[6].controllerA;

    for (std::int32_t i = 2; i < 10; ++i) {
        playerState.altWeaponBanks[i].controllerA.optCatalogEntry = &entries[2];
        playerState.altWeaponBanks[i].controllerB.optCatalogEntry = &entries[2];
    }
    const bool fallback = Player::FindAltGunFireControllerForWeaponId(&saveState, 404) ==
                          &playerState.altWeaponBanks[1].controllerA;

    g_OptCatalog_EntryCount = oldEntryCount;
    g_OptCatalog_EntryTable = oldEntryTable;

    return ctrlAFound && ctrlBFound && ctrlAPriority && fallback ? 0 : 1;
}

extern "C" int player_alt_gun_fire_point_selection_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    playerState.worldPos = {10.0f, 20.0f, 30.0f};
    playerState.steerBasisRaw = {1.0f, 2.0f, 3.0f};
    PlayerGunFireSlot sentinelSlot = {};
    PlayerGunFireSlot *outSlot = &sentinelSlot;
    Player::SelectAltGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.altFireOrigin, {10.0f, 21.0f, 30.0f}) ||
        !Vec3Equals(playerState.aimBasisOrigin, {10.0f, 21.0f, 30.0f}) ||
        !Vec3Equals(playerState.gunFireDir, {1.0f, 2.0f, 3.0f}) || outSlot != &sentinelSlot) {
        return 1;
    }

    zClass_Object3DDataPartial gunData = {};
    zClass_NodePartial gunNode = {};
    gunNode.classId = 5;
    gunNode.classData = &gunData;

    zClass_Object3DDataPartial turretData = {};
    zClass_NodePartial turretNode = {};
    turretNode.classId = 5;
    turretNode.classData = &turretData;

    zMat4x3 gunMatrix = {0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 3.0f, 0.0f, 5.0f, 7.0f, 0.0f, 0.0f, 0.0f};
    zMat4x3 turretMatrix = {11.0f, 0.0f, 13.0f, 0.0f, 0.0f, 0.0f,
                            17.0f, 0.0f, 19.0f, 0.0f, 0.0f, 0.0f};
    playerState.gunFireTransform = {23.0f, 29.0f, 31.0f, 37.0f, 41.0f, 43.0f,
                                    47.0f, 53.0f, 59.0f, 61.0f, 67.0f, 71.0f};
    playerState.aimBasisOrigin = {101.0f, 102.0f, 103.0f};
    playerState.gunNode = &gunNode;
    playerState.turretNode = &turretNode;
    SetObjectLocalMatrix(&gunData, gunMatrix);
    SetObjectLocalMatrix(&turretData, turretMatrix);

    const float yawX = turretMatrix.zx * playerState.gunFireTransform.xx +
                       turretMatrix.zz * playerState.gunFireTransform.zx;
    const float yawY = turretMatrix.zx * playerState.gunFireTransform.xy +
                       turretMatrix.zz * playerState.gunFireTransform.zy;
    const float yawZ = turretMatrix.zx * playerState.gunFireTransform.xz +
                       turretMatrix.zz * playerState.gunFireTransform.zz;
    const zMat4x3 expectedComposed = {
        turretMatrix.xx * playerState.gunFireTransform.xx +
            turretMatrix.xz * playerState.gunFireTransform.zx,
        turretMatrix.xx * playerState.gunFireTransform.xy +
            turretMatrix.xz * playerState.gunFireTransform.zy,
        turretMatrix.xx * playerState.gunFireTransform.xz +
            turretMatrix.xz * playerState.gunFireTransform.zz,
        playerState.gunFireTransform.yx * gunMatrix.yy + gunMatrix.yz * yawX,
        playerState.gunFireTransform.yy * gunMatrix.yy + gunMatrix.yz * yawY,
        playerState.gunFireTransform.yz * gunMatrix.yy + gunMatrix.yz * yawZ,
        playerState.gunFireTransform.yx * gunMatrix.zy + gunMatrix.zz * yawX,
        playerState.gunFireTransform.yy * gunMatrix.zy + gunMatrix.zz * yawY,
        playerState.gunFireTransform.yz * gunMatrix.zy + gunMatrix.zz * yawZ,
        101.0f,
        102.0f,
        103.0f};
    zMat4x3 composed = {};
    Player::ComposeAimBasisWorldMatrix(&saveState, &composed);
    if (!MatrixEquals(composed, expectedComposed)) {
        return 2;
    }

    const zMat4x3 identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    SetObjectLocalMatrix(&gunData, identityMatrix);
    SetObjectLocalMatrix(&turretData, identityMatrix);
    playerState.gunFireTransform = identityMatrix;
    playerState.aimBasisOrigin = {10.0f, 20.0f, 30.0f};

    PlayerGunFireController controller = {};
    zClass_NodePartial primaryAttachNode = {};
    zClass_NodePartial secondaryAttachNode = {};
    zClass_NodePartial activeAttachState = {};
    controller.attachNodePrimary = &primaryAttachNode;
    controller.attachNodeSecondary = &secondaryAttachNode;
    playerState.activeAltGunController = &controller;
    playerState.firePointCenter = {1.0f, 2.0f, 3.0f};
    playerState.firePointRight = {4.0f, 5.0f, 6.0f};
    playerState.firePointLeft = {7.0f, 8.0f, 9.0f};

    outSlot = nullptr;
    playerState.altHardpointSelectState = 0;
    controller.attachState = nullptr;
    Player::SelectAltGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.altFireOrigin, {11.0f, 22.0f, 33.0f}) ||
        outSlot != &playerState.altFireSlotCenter ||
        playerState.altFireSlotCenter.attachNode != &primaryAttachNode ||
        playerState.altHardpointSelectState != 0) {
        return 3;
    }

    outSlot = nullptr;
    controller.attachState = &activeAttachState;
    Player::SelectAltGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.altFireOrigin, {10.0f, 20.0f, 30.0f}) ||
        outSlot != &playerState.altFireSlotCenter ||
        playerState.altFireSlotCenter.attachNode != &primaryAttachNode ||
        playerState.altHardpointSelectState != 0) {
        return 4;
    }

    outSlot = nullptr;
    controller.attachState = nullptr;
    playerState.altHardpointSelectState = 1;
    Player::SelectAltGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.altFireOrigin, {14.0f, 25.0f, 36.0f}) ||
        outSlot != &playerState.altFireSlotRight ||
        playerState.altFireSlotRight.attachNode != &secondaryAttachNode ||
        playerState.altHardpointSelectState != 2) {
        return 5;
    }

    outSlot = nullptr;
    playerState.altHardpointSelectState = 2;
    Player::SelectAltGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.altFireOrigin, {17.0f, 28.0f, 39.0f}) ||
        outSlot != &playerState.altFireSlotLeft ||
        playerState.altFireSlotLeft.attachNode != &primaryAttachNode ||
        playerState.altHardpointSelectState != 1) {
        return 6;
    }

    return 0;
}

extern "C" int player_build_mission_save_data_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    zClass_NodePartial cameraNode = {};
    zClass_CameraDataPartial cameraData = {};
    OptCatalogEntryDef hitSource = {};

    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        PlayerAltWeaponBank &bank = playerState.altWeaponBanks[bankIndex];
        bank.selectedSide = bankIndex & 1;
        bank.controllerA.flags = bankIndex & 1 ? 4 : 0;
        bank.controllerA.ammoOrCharge = static_cast<float>(10 + bankIndex);
        bank.controllerA.weaponBankIndex = bankIndex;
        bank.controllerA.weaponSideIndex = 0;
        bank.controllerB.flags = bankIndex & 1 ? 0 : 4;
        bank.controllerB.ammoOrCharge = static_cast<float>(20 + bankIndex);
        bank.controllerB.weaponBankIndex = bankIndex;
        bank.controllerB.weaponSideIndex = 1;
    }

    playerState.activeAltGunController = &playerState.altWeaponBanks[3].controllerB;
    playerState.activePrimaryGunController = &playerState.altWeaponBanks[4].controllerA;
    playerState.amphibUnlocked = 1;
    playerState.hoverUnlocked = 2;
    playerState.subUnlocked = 3;
    playerState.aiMode = 4;
    playerState.nextModeSwitchAllowedTime = 12.5f;
    playerState.motionInput = 5;
    playerState.autoTurnSign = -1;
    playerState.bankInput = 6;
    playerState.timedHitStatus.runtimeFlags = 1;
    playerState.timedHitStatus.hitSource = &hitSource;
    playerState.timedHitStatus.unknown_08 = 8;
    playerState.timedHitStatus.unknown_0c = 9;
    playerState.timedHitStatus.lightNode = &cameraNode;
    playerState.timedHitStatus.nextUpdateTime = 50.0f;
    playerState.timedHitStatus.unknown_18 = 18;
    hitSource.ordinalIndex = 77;

    modalData.masterType = 11;
    modalState.masterModalData = &modalData;
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    cameraData.cameraFlags = 2;
    cameraData.worldTarget = {31.0f, 32.0f, 33.0f};
    cameraData.targetOrEuler = {21.0f, 22.0f, 23.0f};
    cameraData.posOffset = {41.0f, 42.0f, 43.0f};

    zUtil_SaveGameState *const oldLocalSaveState = g_LocalPlayerSaveState;
    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    const float oldStatusMeterRatio = g_PlayerStatusMeterRatio;
    const int oldHudCounterValue = g_Player_HudCounterValue;
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;

    g_LocalPlayerSaveState = &saveState;
    g_MainCamera = &cameraNode;
    g_PlayerStatusMeterRatio = 0.625f;
    g_Player_HudCounterValue = 1234;
    g_Time_AccumulatedTimeSec = 45.0f;

    PlayerMissionSaveData outData = {};
    Player::BuildMissionSaveData(&outData);

    bool ok = outData.size == sizeof(PlayerMissionSaveData) &&
              outData.altWeaponBankIndex == 3 && outData.altWeaponSideIndex == 1 &&
              outData.primaryWeaponBankIndex == 4 && outData.primaryWeaponSideIndex == 0 &&
              outData.playerStatusMeterRatio == 0.625f && outData.hudCounterValue == 1234 &&
              outData.amphibUnlocked == 1 && outData.hoverUnlocked == 2 &&
              outData.subUnlocked == 3 && outData.aiMode == 4 &&
              outData.nextModeSwitchAllowedTime == 12.5f && outData.motionInput == 5 &&
              outData.autoTurnSign == -1 && outData.bankInput == 6 &&
              outData.playerMasterType == 11 &&
              Vec3Equals(outData.cameraTarget, cameraData.worldTarget) &&
              Vec3Equals(outData.cameraPosition, cameraData.posOffset) &&
              outData.timedHitStatus.runtimeFlags == 1 &&
              outData.timedHitStatus.savedHitSourceEntryId == 77 &&
              outData.timedHitStatus.unknown_08 == 8 &&
              outData.timedHitStatus.unknown_0c == 9 &&
              outData.timedHitStatus.lightNode == nullptr &&
              outData.timedHitStatus.nextUpdateTime == 5.0f &&
              outData.timedHitStatus.unknown_18 == 18;

    for (int bankIndex = 0; ok && bankIndex < 10; ++bankIndex) {
        const PlayerMissionSaveWeaponBank &savedBank = outData.weaponBank[bankIndex];
        const PlayerAltWeaponBank &sourceBank = playerState.altWeaponBanks[bankIndex];
        ok = savedBank.selectedSide == sourceBank.selectedSide &&
             savedBank.sides[0].enabled == ((sourceBank.controllerA.flags >> 2) & 1) &&
             savedBank.sides[0].ammoOrCharge == sourceBank.controllerA.ammoOrCharge &&
             savedBank.sides[1].enabled == ((sourceBank.controllerB.flags >> 2) & 1) &&
             savedBank.sides[1].ammoOrCharge == sourceBank.controllerB.ammoOrCharge;
    }

    g_LocalPlayerSaveState = oldLocalSaveState;
    g_MainCamera = oldMainCamera;
    g_PlayerStatusMeterRatio = oldStatusMeterRatio;
    g_Player_HudCounterValue = oldHudCounterValue;
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    return ok ? 0 : 1;
}

extern "C" int player_set_world_pose_and_restart_anchor_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    playerState.worldPos = {1.0f, 2.0f, 3.0f};
    playerState.previousTransform.posX = 4.0f;
    playerState.previousTransform.posY = 5.0f;
    playerState.previousTransform.posZ = 6.0f;
    playerState.restartYawRad = 7.0f;
    playerState.variantTag.count = 3;
    playerState.variantTag.tags[0] = 10;
    playerState.variantTag.tags[1] = 11;
    playerState.variantTag.tags[2] = 12;

    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    g_VariantTag_Current.count = 2;
    g_VariantTag_Current.tags[0] = 20;
    g_VariantTag_Current.tags[1] = 21;
    g_VariantTag_Current.tags[2] = 22;
    g_Variant_CurrentTag.count = 1;
    g_Variant_CurrentTag.tags[0] = 30;
    g_Variant_CurrentTag.tags[1] = 31;
    g_Variant_CurrentTag.tags[2] = 32;

    Player::SetWorldPoseAndRestartAnchor(nullptr, nullptr, 9.0f);
    if (!Vec3Equals(playerState.worldPos, {1.0f, 2.0f, 3.0f}) ||
        playerState.restartYawRad != 7.0f || g_VariantTag_Current.count != 2 ||
        g_Variant_CurrentTag.count != 1) {
        g_VariantTag_Current = oldVariantTagCurrent;
        g_Variant_CurrentTag = oldVariantCurrent;
        return 1;
    }

    const zVec3 newPosition = {40.0f, 41.0f, 42.0f};
    Player::SetWorldPoseAndRestartAnchor(&saveState, &newPosition, 1.25f);

    const bool ok = Vec3Equals(playerState.worldPos, newPosition) &&
                    playerState.previousTransform.posX == newPosition.x &&
                    playerState.previousTransform.posY == newPosition.y &&
                    playerState.previousTransform.posZ == newPosition.z &&
                    playerState.restartYawRad == 1.25f &&
                    g_VariantTag_Current.count == 0 &&
                    g_VariantTag_Current.tags[0] == 0xff &&
                    g_VariantTag_Current.tags[1] == 0xff &&
                    g_VariantTag_Current.tags[2] == 0xff &&
                    g_Variant_CurrentTag.count == 0 &&
                    g_Variant_CurrentTag.tags[0] == 0xff &&
                    g_Variant_CurrentTag.tags[1] == 0xff &&
                    g_Variant_CurrentTag.tags[2] == 0xff &&
                    playerState.variantTag.count == 0 &&
                    playerState.variantTag.tags[0] == 0xff &&
                    playerState.variantTag.tags[1] == 0xff &&
                    playerState.variantTag.tags[2] == 0xff;

    g_VariantTag_Current = oldVariantTagCurrent;
    g_Variant_CurrentTag = oldVariantCurrent;
    return ok ? 0 : 2;
}

extern "C" int player_update_bank_velocity_from_steer_input_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    modalData.accelRate = 6.0f;
    g_Player_DeltaTime = 0.25f;

    playerState.restartYawRad = 2.0f;
    playerState.steeringInput = 1.0f;
    playerState.steeringInputCopy = 0.5f;
    playerState.localVel.x = -3.0f;
    Player::UpdateBankVelocityFromSteerInput(&saveState);
    if (playerState.restartYawRad != 0.0f || playerState.localVel.x != -3.75f) {
        return 1;
    }

    playerState.steeringInputCopy = 0.5f;
    playerState.localVel.x = 2.0f;
    Player::UpdateBankVelocityFromSteerInput(&saveState);
    if (playerState.localVel.x != -0.75f) {
        return 2;
    }

    playerState.steeringInputCopy = -0.5f;
    playerState.localVel.x = -2.0f;
    Player::UpdateBankVelocityFromSteerInput(&saveState);
    if (playerState.localVel.x != 0.75f) {
        return 3;
    }

    playerState.steeringInput = 0.0f;
    playerState.steeringInputCopy = -0.5f;
    playerState.localVel.x = 4.0f;
    playerState.restartYawRad = 8.0f;
    Player::UpdateBankVelocityFromSteerInput(&saveState);
    return playerState.restartYawRad == 0.0f && playerState.localVel.x == 0.0f ? 0 : 4;
}

extern "C" int player_update_auto_turn_and_steer_from_target_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    modalData.yawAccel = 4.0f;
    modalData.yawDamping = 3.0f;
    playerState.yawVelocityLimit = 1.25f;
    g_Player_DeltaTime = 0.5f;

    playerState.steeringInput = 1.0f;
    playerState.steeringInputCopy = 0.25f;
    playerState.angVelYaw = -2.0f;
    Player::UpdateAutoTurnAndSteerFromTarget(&saveState);
    if (playerState.angVelYaw != 0.5f) {
        return 1;
    }

    playerState.steeringInputCopy = 0.25f;
    playerState.angVelYaw = 1.0f;
    Player::UpdateAutoTurnAndSteerFromTarget(&saveState);
    if (playerState.angVelYaw != 1.25f) {
        return 2;
    }

    playerState.steeringInputCopy = -0.25f;
    playerState.angVelYaw = -1.0f;
    Player::UpdateAutoTurnAndSteerFromTarget(&saveState);
    if (playerState.angVelYaw != -1.25f) {
        return 3;
    }

    playerState.steeringInput = 0.0f;
    playerState.angVelYaw = 2.0f;
    g_Player_DeltaTime = 0.0f;
    Player::UpdateAutoTurnAndSteerFromTarget(&saveState);
    return playerState.angVelYaw == 2.0f ? 0 : 4;
}

extern "C" int player_integrate_yaw_and_wrap_from_yaw_velocity_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    g_Player_DeltaTime = 1.0f;

    playerState.autoTurnActive = 1;
    playerState.autoTurnTargetDir.x = -1.0f;
    playerState.autoTurnTargetDir.z = 0.0f;
    playerState.steeringInput = 1.0f;
    playerState.angVelYaw = 3.0f;
    Player::IntegrateYawAndWrapFromYawVelocity(&saveState);
    if (playerState.autoTurnActive != 0 || playerState.steeringInput != 0.0f ||
        playerState.angVelYaw != 0.0f || !FloatNear(playerState.restartYawRad, 0.0f)) {
        return 1;
    }

    playerState.restartYawRad = 6.2f;
    playerState.angVelYaw = 0.2f;
    playerState.steeringInput = 0.0f;
    modalData.yawDamping = 0.0f;
    Player::IntegrateYawAndWrapFromYawVelocity(&saveState);
    if (!FloatNear(playerState.restartYawRad, 0.1168146f)) {
        return 2;
    }

    playerState.restartYawRad = -6.2f;
    playerState.angVelYaw = -0.2f;
    Player::IntegrateYawAndWrapFromYawVelocity(&saveState);
    if (!FloatNear(playerState.restartYawRad, -0.1168146f)) {
        return 3;
    }

    playerState.restartYawRad = 1.0f;
    playerState.angVelYaw = 0.5f;
    g_Player_DeltaTime = 0.5f;
    Player::IntegrateYawAndWrapFromYawVelocity(&saveState);
    return FloatNear(playerState.restartYawRad, 1.25f) ? 0 : 4;
}

extern "C" int player_rebuild_steer_basis_from_motion_basis_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    playerState.motionBasis.yx = 7.0f;
    playerState.motionBasis.yy = 8.0f;
    playerState.motionBasis.yz = 9.0f;
    playerState.motionBasis.zx = 3.0f;
    playerState.motionBasis.zy = 4.0f;
    playerState.motionBasis.zz = 0.0f;
    playerState.steerBasisNorm.y = 99.0f;

    Player::RebuildSteerBasisFromMotionBasis(&saveState);
    if (!FloatNear(playerState.steerBasisRaw.x, -3.0f) ||
        !FloatNear(playerState.steerBasisRaw.y, -4.0f) ||
        !FloatNear(playerState.steerBasisRaw.z, 0.0f)) {
        return 1;
    }
    if (!FloatNear(playerState.steerBasisRef.x, 7.0f) ||
        !FloatNear(playerState.steerBasisRef.y, 8.0f) ||
        !FloatNear(playerState.steerBasisRef.z, 9.0f)) {
        return 2;
    }

    return FloatNear(playerState.steerBasisNorm.x, -1.0f) &&
                   FloatNear(playerState.steerBasisNorm.y, 0.0f) &&
                   FloatNear(playerState.steerBasisNorm.z, 0.0f)
               ? 0
               : 3;
}

extern "C" int player_start_modal_loop_sfx_handle_smoke(void) {
    TestDirectSoundBufferVTable vtable = {};
    vtable.GetStatus = &TestDirectSoundGetStatus;
    vtable.Play = &TestDirectSoundPlay;
    vtable.SetCurrentPosition = &TestDirectSoundSetInt;
    vtable.SetVolume = &TestDirectSoundSetInt;
    TestDirectSoundBuffer directSoundBuffer = {&vtable};

    float globalVolume = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_MuteDepth = 0;
    g_zSnd_Flag10PlaybackEnabled = 1;

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    sample.replayFields.gain = 0.5f;
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);

    PlayerMasterModalData modalData = {};
    modalData.sfxEngine[1] = &sample;
    PlayerModalState modalState = {};
    modalState.masterModalData = &modalData;
    zUtil_PlayerStateStorage playerState = {};
    playerState.worldPos = {2.0f, 3.0f, 4.0f};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    saveState.StartModalLoopSfxHandle(1, 0.25f);

    g_zSnd_GlobalVolumeScalePtr = nullptr;
    g_zSnd_IsInitialized = 0;
    g_zSnd_PreInitialized = 0;

    if (modalState.modalSfxHandle[1] == nullptr) {
        return 1;
    }
    if (modalState.modalSfxHandle[1] != &sample.primaryVoice) {
        return 2;
    }

    return sample.primaryVoice.ownerSample == &sample ? 0 : 3;
}

extern "C" int player_start_master_type_loop_sfx_handle_smoke(void) {
    TestDirectSoundBufferVTable vtable = {};
    vtable.GetStatus = &TestDirectSoundGetStatus;
    vtable.Play = &TestDirectSoundPlay;
    vtable.SetCurrentPosition = &TestDirectSoundSetInt;
    vtable.SetVolume = &TestDirectSoundSetInt;
    TestDirectSoundBuffer directSoundBuffer = {&vtable};

    float globalVolume = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_MuteDepth = 0;
    g_zSnd_Flag10PlaybackEnabled = 1;

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    sample.replayFields.gain = 0.5f;
    sample.markerBaseTime = 12.0f;
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);

    PlayerMasterCommonData commonData = {};
    commonData.sfxWeaponUp[3] = &sample;
    zUtil_PlayerStateStorage playerState = {};
    playerState.masterCommonData = &commonData;
    playerState.worldPos = {2.0f, 3.0f, 4.0f};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;

    zSndPlayHandle *const handle = saveState.StartMasterTypeLoopSfxHandle(3, 0.25f);

    g_zSnd_GlobalVolumeScalePtr = nullptr;
    g_zSnd_IsInitialized = 0;
    g_zSnd_PreInitialized = 0;

    if (handle == nullptr || handle != &sample.primaryVoice) {
        return 1;
    }
    if (playerState.modeLoopSfxHandle[3] != handle || sample.primaryVoice.ownerSample != &sample) {
        return 2;
    }

    return sample.markerBaseTime == 0.0f ? 0 : 3;
}

extern "C" int player_stop_modal_loop_sfx_handle_smoke(void) {
    zUtil_SaveGameState saveState = {};
    PlayerModalState modalState = {};
    zSndPlayHandle handle = {};
    saveState.primaryModalState = &modalState;
    modalState.modalSfxHandle[2] = &handle;

    saveState.StopModalLoopSfxHandle(2);
    if (modalState.modalSfxHandle[2] != nullptr) {
        return 1;
    }

    saveState.StopModalLoopSfxHandle(1);
    return modalState.modalSfxHandle[1] == nullptr ? 0 : 2;
}

extern "C" int player_start_slip_sfx_smoke(void) {
    TestDirectSoundBufferVTable vtable = {};
    vtable.GetStatus = &TestDirectSoundGetStatus;
    vtable.Play = &TestDirectSoundPlay;
    vtable.SetCurrentPosition = &TestDirectSoundSetInt;
    vtable.SetVolume = &TestDirectSoundSetInt;
    TestDirectSoundBuffer directSoundBuffer = {&vtable};

    float globalVolume = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_MuteDepth = 0;
    g_zSnd_Flag10PlaybackEnabled = 1;

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    sample.replayFields.gain = 1.0f;
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);

    PlayerMasterModalData modalData = {};
    modalData.sfxEngine[3] = &sample;
    PlayerModalState modalState = {};
    modalState.masterModalData = &modalData;
    zUtil_PlayerStateStorage playerState = {};
    playerState.worldPos = {10.0f, 20.0f, 30.0f};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    Player::StartSlipSfx(&saveState);

    g_zSnd_GlobalVolumeScalePtr = nullptr;
    g_zSnd_IsInitialized = 0;
    g_zSnd_PreInitialized = 0;

    return playerState.slipSfxActive == 1 &&
                   modalState.modalSfxHandle[3] == &sample.primaryVoice
               ? 0
               : 1;
}

extern "C" int player_stop_slip_sfx_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    zSndPlayHandle handle = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    playerState.slipSfxActive = 1;
    modalState.modalSfxHandle[3] = &handle;

    Player::StopSlipSfx(&saveState);
    return playerState.slipSfxActive == 0 && modalState.modalSfxHandle[3] == nullptr ? 0 : 1;
}

extern "C" int player_float_sign_smoke(void) {
    if (Player::FloatSign(0.0f) != 0) {
        return 1;
    }
    if (Player::FloatSign(-0.25f) != -1) {
        return 2;
    }

    return Player::FloatSign(4.0f) == 1 ? 0 : 3;
}

extern "C" int player_update_bank_and_turn_dynamics_smoke(void) {
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldPlayerInvDeltaTime = g_Player_InvDeltaTime;
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;

    g_Player_DeltaTime = 0.0f;
    if (Player::UpdateBankAndTurnDynamics(&saveState) != 0.0f) {
        g_Player_DeltaTime = oldPlayerDeltaTime;
        g_Player_InvDeltaTime = oldPlayerInvDeltaTime;
        return 1;
    }

    TestDirectSoundBufferVTable vtable = {};
    vtable.GetStatus = &TestDirectSoundGetStatus;
    vtable.Play = &TestDirectSoundPlay;
    vtable.SetCurrentPosition = &TestDirectSoundSetInt;
    vtable.SetVolume = &TestDirectSoundSetInt;
    TestDirectSoundBuffer directSoundBuffer = {&vtable};

    float globalVolume = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_MuteDepth = 0;
    g_zSnd_Flag10PlaybackEnabled = 1;

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    sample.replayFields.gain = 1.0f;
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);
    modalData.sfxEngine[3] = &sample;

    g_Player_DeltaTime = 0.5f;
    g_Player_InvDeltaTime = 2.0f;
    modalData.frictionStatic = 5.0f;
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.bankBasis = {0.0f, 0.0f, 2.0f};
    playerState.localVel = {0.0f, 0.0f, -3.0f};
    playerState.motionBasis.xy = 0.0f;

    const float staticResidual = Player::UpdateBankAndTurnDynamics(&saveState);
    if (!FloatNear(staticResidual, 7.0f) || playerState.slipSfxActive != 1 ||
        modalState.modalSfxHandle[3] != &sample.primaryVoice) {
        g_zSnd_GlobalVolumeScalePtr = nullptr;
        g_zSnd_IsInitialized = 0;
        g_zSnd_PreInitialized = 0;
        g_Player_DeltaTime = oldPlayerDeltaTime;
        g_Player_InvDeltaTime = oldPlayerInvDeltaTime;
        return 2;
    }

    std::memset(&playerState, 0, sizeof(playerState));
    std::memset(&modalState, 0, sizeof(modalState));
    modalState.masterModalData = &modalData;
    saveState.primaryModalState = &modalState;
    modalData.frictionStatic = 20.0f;
    modalData.frictionDynamic = 2.0f;
    playerState.slipSfxActive = 1;
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.bankBasis = {0.0f, 0.0f, 2.0f};
    playerState.localVel = {1.0f, 0.0f, -1.0f};
    playerState.motionBasis.xy = 0.0f;

    const float dynamicResidual = Player::UpdateBankAndTurnDynamics(&saveState);

    g_zSnd_GlobalVolumeScalePtr = nullptr;
    g_zSnd_IsInitialized = 0;
    g_zSnd_PreInitialized = 0;
    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_Player_InvDeltaTime = oldPlayerInvDeltaTime;

    return FloatNear(dynamicResidual, 2.0f) ? 0 : 3;
}

extern "C" int player_compute_turn_slip_delta_smoke(void) {
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldPlayerInvDeltaTime = g_Player_InvDeltaTime;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;

    g_Player_DeltaTime = 0.5f;
    g_Player_InvDeltaTime = 2.0f;
    playerState.motionBasis.xx = 1.0f;
    playerState.motionBasis.yy = 1.0f;
    playerState.motionBasis.zz = 1.0f;
    playerState.projectileSpawnVel = {2.0f, 0.0f, 6.0f};
    playerState.axisClampRuntime = 3.0f;
    playerState.throttleInputCopy = 1.0f;
    modalData.accelRate = 4.0f;
    modalData.frictionDynamic = 0.0f;
    modalData.frictionStatic = 100.0f;

    Player::ComputeTurnSlipDelta(&saveState);
    if (!FloatNear(playerState.localVel.x, 2.0f) ||
        !FloatNear(playerState.localVel.z, 3.0f)) {
        g_Player_DeltaTime = oldPlayerDeltaTime;
        g_Player_InvDeltaTime = oldPlayerInvDeltaTime;
        return 1;
    }

    zSndPlayHandle handle = {};
    std::memset(&playerState, 0, sizeof(playerState));
    playerState.motionBasis.xx = 1.0f;
    playerState.motionBasis.yy = 1.0f;
    playerState.motionBasis.zz = 1.0f;
    playerState.projectileSpawnVel = {1.0f, 0.0f, 1.0f};
    playerState.axisClampRuntime = 10.0f;
    playerState.slipSfxActive = 1;
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.bankBasis = {0.0f, 0.0f, 4.0f};
    modalState.modalSfxHandle[3] = &handle;
    modalData.accelRate = 0.0f;
    modalData.frictionDynamic = 0.0f;
    modalData.frictionStatic = 100.0f;
    g_Player_DeltaTime = 0.5f;
    g_Player_InvDeltaTime = 1.0f;

    Player::ComputeTurnSlipDelta(&saveState);

    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_Player_InvDeltaTime = oldPlayerInvDeltaTime;
    return FloatNear(playerState.localVel.x, 0.0f) && playerState.slipSfxActive == 0 &&
                   modalState.modalSfxHandle[3] == nullptr
               ? 0
               : 2;
}

extern "C" int player_update_yaw_velocity_from_steer_input_smoke(void) {
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldPlayerInvDeltaTime = g_Player_InvDeltaTime;
    const float oldPlayerDeltaTimeScaled001 = g_Player_DeltaTimeScaled001;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;

    g_Player_DeltaTime = 0.5f;
    g_Player_InvDeltaTime = 2.0f;
    g_Player_DeltaTimeScaled001 = 0.01f;
    g_GameStateOrMapTable = nullptr;

    playerState.localVel = {0.001f, 0.0f, -0.002f};
    playerState.axisClampRuntime = 5.0f;
    Player::UpdateYawVelocityFromSteerInput(&saveState);
    if (playerState.localVel.x != 0.0f || playerState.localVel.z != 0.0f) {
        g_Player_DeltaTime = oldPlayerDeltaTime;
        g_Player_InvDeltaTime = oldPlayerInvDeltaTime;
        g_Player_DeltaTimeScaled001 = oldPlayerDeltaTimeScaled001;
        g_GameStateOrMapTable = oldGameStateOrMapTable;
        return 1;
    }

    std::memset(&playerState, 0, sizeof(playerState));
    playerState.localVel = {2.0f, 0.0f, 4.0f};
    playerState.throttleInput = 1.0f;
    playerState.axisClampRuntime = 6.0f;
    modalData.rateDampingAccel = 0.125f;
    modalData.rateDampingDecel = 0.25f;
    g_Player_DeltaTimeScaled001 = 0.0f;

    Player::UpdateYawVelocityFromSteerInput(&saveState);
    if (!FloatNear(playerState.localVel.x, 2.0f * PlayerDampingFactor(0.125f, 0.5f)) ||
        !FloatNear(playerState.localVel.z, 4.0f * PlayerDampingFactor(0.25f, 0.5f))) {
        g_Player_DeltaTime = oldPlayerDeltaTime;
        g_Player_InvDeltaTime = oldPlayerInvDeltaTime;
        g_Player_DeltaTimeScaled001 = oldPlayerDeltaTimeScaled001;
        g_GameStateOrMapTable = oldGameStateOrMapTable;
        return 2;
    }

    std::memset(&playerState, 0, sizeof(playerState));
    playerState.localVel = {1.0f, 0.0f, -3.0f};
    playerState.throttleInput = 1.0f;
    playerState.axisClampRuntime = 10.0f;
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.bankBasis = {0.0f, 0.0f, 2.0f};
    modalData.rateDampingDecel = 0.0f;
    modalData.frictionDynamic = 1.0f;
    modalData.frictionStatic = 100.0f;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;

    Player::UpdateYawVelocityFromSteerInput(&saveState);

    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_Player_InvDeltaTime = oldPlayerInvDeltaTime;
    g_Player_DeltaTimeScaled001 = oldPlayerDeltaTimeScaled001;
    g_GameStateOrMapTable = oldGameStateOrMapTable;

    return FloatNear(playerState.localVel.x, 6.5f) &&
                   FloatNear(playerState.localVel.z, -3.0f)
               ? 0
               : 3;
}

extern "C" int player_select_probe_sample_height_smoke(void) {
    PlayerProbeSampleCandidateBuffer emptyBuffer = {};
    int bestIndex = 99;
    int selectedImpactSlot = 88;
    float taggedHeight = 77.0f;
    float selected = Player::SelectProbeSampleHeightFromCandidates(
        &emptyBuffer, &bestIndex, 10.0f, 4.0f, 0, &selectedImpactSlot, &taggedHeight);
    if (selected != 10.0f || bestIndex != 0 || selectedImpactSlot != 0 ||
        taggedHeight != -300.0f) {
        return 1;
    }

    PlayerProbeSampleCandidateBuffer buffer = {};
    buffer.candidateCount = 3;
    buffer.entries[0].hitPos.y = 8.0f;
    buffer.entries[1].hitPos.y = 12.0f;
    buffer.entries[2].hitPos.y = 15.0f;
    selected = Player::SelectProbeSampleHeightFromCandidates(
        &buffer, &bestIndex, 10.0f, 4.0f, 0, &selectedImpactSlot, &taggedHeight);
    if (selected != 12.0f || bestIndex != 1 || selectedImpactSlot != 0 ||
        taggedHeight != -300.0f) {
        return 2;
    }

    buffer = {};
    buffer.candidateCount = 1;
    buffer.entries[0].hitPos.y = 20.0f;
    selected = Player::SelectProbeSampleHeightFromCandidates(
        &buffer, &bestIndex, 10.0f, 4.0f, 0, &selectedImpactSlot, &taggedHeight);
    if (selected != 20.0f || bestIndex != 0 || selectedImpactSlot != 0) {
        return 3;
    }

    zModel_MaterialPartial material = {};
    material.userTag = 1;
    buffer = {};
    buffer.candidateCount = 1;
    buffer.entries[0].hitPos.y = 12.0f;
    buffer.entries[0].scenePayload = &material;
    selected = Player::SelectProbeSampleHeightFromCandidates(
        &buffer, &bestIndex, 10.0f, 4.0f, 1, &selectedImpactSlot, &taggedHeight);
    if (selected != -250.0f || taggedHeight != 12.0f || selectedImpactSlot != 1) {
        return 4;
    }

    material.userTag = 2;
    buffer.entries[0].hitPos.y = 4.0f;
    selected = Player::SelectProbeSampleHeightFromCandidates(
        &buffer, &bestIndex, 10.0f, 4.0f, 0, &selectedImpactSlot, &taggedHeight);
    return selected == 4.0f && taggedHeight == 4.0f && selectedImpactSlot == 0 ? 0 : 5;
}

extern "C" int player_probe_modal_sample_heights_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;

    static std::int32_t matrixFlags[2];
    static float *matrixSlots[2];
    static zMat4x3 identityMatrix;
    identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixFlags[0] = 1;
    matrixSlots[0] = reinterpret_cast<float *>(&identityMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage globalPlayerState = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial globalRootNode = {};
    zClass_NodePartial attachmentNode = {};
    playerState.rootNode = &rootNode;
    globalPlayerState.rootNode = &globalRootNode;
    playerState.motionBasis.xx = 1.0f;
    playerState.motionBasis.yy = 1.0f;
    playerState.motionBasis.zz = 1.0f;
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 0x42;
    playerState.variantTag.tags[1] = 0xff;
    playerState.variantTag.tags[2] = 0xff;

    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    gameStateOrMap.playerState =
        static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&globalPlayerState));
    g_GameStateOrMapTable = &gameStateOrMap;
    g_Player_DeltaTime = 0.5f;
    g_VariantTag_Current.count = 0;
    g_VariantTag_Current.tags[0] = 0xff;
    g_VariantTag_Current.tags[1] = 0xff;
    g_VariantTag_Current.tags[2] = 0xff;

    PlayerMasterModalData masterModalData = {};
    masterModalData.masterType = 1;
    masterModalData.probePoints[0] = {0.25f, 0.5f, 0.25f};
    masterModalData.probePoints[1] = {0.6f, 0.75f, 0.2f};
    PlayerModalState modalState = {};
    modalState.masterModalData = &masterModalData;
    modalState.modalStateCode = 2;
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    zVec3 probeFaceVertices[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
                                  {1.0f, 0.0f, 0.0f}};
    std::int32_t faceIndices[3] = {0, 1, 2};
    zModel_MaterialPartial materialPayload = {};
    materialPayload.userTag = 4;
    zModel_PickFaceUvData faceUvData = {};
    zModel_PickFaceEntry faceEntry = {};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.vertexIndices = faceIndices;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload =
        static_cast<zModel_PickFaceScenePayload *>(static_cast<void *>(&materialPayload));
    faceEntry.variantTag.count = 1;
    faceEntry.variantTag.tags[0] = 0x42;
    zModel_PickFaceData faceData = {};
    faceData.faceCount = 1;
    faceData.vertexCount = 3;
    faceData.faces = &faceEntry;
    faceData.baseVertices = probeFaceVertices;

    zClass_Object3DDataPartial objectData = {};
    objectData.flags = 8;
    zClass_NodePartial objectNode = {};
    objectNode.flags = 0x11c;
    objectNode.auxFlags = 1;
    objectNode.nodeType = 0xff;
    objectNode.classId = 5;
    objectNode.classData = &objectData;
    objectNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    objectNode.callbackContext = &attachmentNode;
    objectNode.cachedBounds[0] = 0.0f;
    objectNode.cachedBounds[1] = 0.0f;
    objectNode.cachedBounds[2] = 0.0f;
    objectNode.cachedBounds[3] = 1.0f;
    objectNode.cachedBounds[4] = 1.0f;
    objectNode.cachedBounds[5] = 1.0f;
    zClass_NodePartial *areaChildren[1] = {&objectNode};
    zWorldAreaPartial area = {};
    area.childCount = 1;
    area.childList = areaChildren;
    zWorldAreaPartial *areaRows[1] = {&area};
    zClass_WorldDataPartial worldData = {};
    worldData.clampQueriesToBounds = 1;
    worldData.areaCellSizeX = 1.0f;
    worldData.areaCellSizeZ = 1.0f;
    worldData.areaInvSizeX = 1.0f;
    worldData.areaInvSizeZ = 1.0f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = areaRows;
    zClass_NodePartial world = {};
    world.classData = &worldData;
    g_Player_RuntimeDiScene = &world;

    float sampleHeights[2] = {};
    float bestHeight = -9.0f;
    PlayerProbeTypeHistogram histogram = {};
    int attachmentCandidateCount = 0;
    zClass_NodePartial *attachmentOut = nullptr;
    Player::ProbeModalSampleHeights(&saveState, sampleHeights, &bestHeight, 0, &histogram,
                                    &attachmentCandidateCount, &attachmentOut);

    playerState.yawVelocityLimit = -1.0f;
    playerState.vehiclePitchRad = 3.0f;
    playerState.vehicleRollRad = 4.0f;
    playerState.worldPos.y = -5.0f;
    masterModalData.yawRateMax = 12.5f;
    masterModalData.modeAltTransitionTime = 10.0f;
    Player::UpdateMasterTypeBasicOrTrack_FromModalProbe(&saveState);
    const bool updateModalProbeOk =
        playerState.yawVelocityLimit == 12.5f && playerState.vehiclePitchRad == 0.0f &&
        playerState.vehicleRollRad == 0.0f && playerState.worldPos.y == 10.0f;

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_VariantTag_Current = oldVariantTagCurrent;

    if (sampleHeights[0] != 0.0f || sampleHeights[1] != 0.0f || bestHeight != 0.0f ||
        histogram.countByImpactSlot[4] != 2 || playerState.probeImpactSlot4SeenFlag != 1 ||
        playerState.probeImpactSlot1SeenFlag != 0 || attachmentCandidateCount != 2 ||
        attachmentOut != &attachmentNode || playerState.selectedProbeSample.node != &objectNode ||
        playerState.selectedProbeSample.hitPos.x != 0.25f ||
        playerState.selectedProbeSample.hitPos.y != 0.0f ||
        playerState.selectedProbeSample.hitPos.z != 0.25f ||
        playerState.variantTag.tags[0] != 0x42 || g_Variant_CurrentTag.count != 0 ||
        !updateModalProbeOk) {
        return 1;
    }

    return 0;
}

extern "C" int player_update_master_type_basic_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldPlayerDeltaTimeScaled001 = g_Player_DeltaTimeScaled001;

    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage globalPlayerState = {};
    PlayerMasterModalData modalData = {};
    PlayerModalState modalState = {};
    zUtil_SaveGameState saveState = {};
    zClass_Object3DDataPartial objectData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial globalRootNode = {};
    zClass_WorldDataPartial worldData = {};
    zClass_NodePartial worldNode = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    modalState.modalStateCode = 0;

    rootNode.classId = 5;
    rootNode.classData = &objectData;
    playerState.rootNode = &rootNode;
    globalPlayerState.rootNode = &globalRootNode;
    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    gameStateOrMap.playerState =
        static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&globalPlayerState));
    g_GameStateOrMapTable = &gameStateOrMap;
    worldNode.classData = &worldData;
    g_Player_RuntimeDiScene = &worldNode;

    objectData.scale = {1.0f, 1.0f, 1.0f};
    objectData.localMatrix[0] = 1.0f;
    objectData.localMatrix[4] = 1.0f;
    objectData.localMatrix[8] = 1.0f;

    modalData.maxSpeed = 100.0f;
    modalData.yawRateMax = 12.0f;
    modalData.accelRate = 0.0f;
    modalData.yawAccel = 0.0f;
    modalData.yawDamping = 0.0f;
    modalData.rateDampingAccel = 0.0f;
    modalData.rateDampingDecel = 0.0f;
    modalData.modeAltTransitionTime = 7.0f;

    playerState.worldPos = {10.0f, 5.0f, 20.0f};
    playerState.localVel = {2.0f, 3.0f, 4.0f};
    playerState.throttleInputCopy = 1.0f;
    playerState.fxOffsetLocal = {1.0f, 2.0f, 3.0f};
    playerState.cameraState = 0;
    g_Player_DeltaTime = 0.5f;
    g_Player_DeltaTimeScaled001 = 0.0f;

    Player::UpdateMasterTypeBasic(&saveState);

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_Player_DeltaTimeScaled001 = oldPlayerDeltaTimeScaled001;

    if (!FloatNear(playerState.worldPos.x, 11.0f) ||
        !FloatNear(playerState.worldPos.y, 7.0f) ||
        !FloatNear(playerState.worldPos.z, 22.0f) ||
        !FloatNear(playerState.projectileSpawnVel.x, 2.0f) ||
        !FloatNear(playerState.projectileSpawnVel.y, 3.0f) ||
        !FloatNear(playerState.projectileSpawnVel.z, 4.0f) ||
        playerState.axisClampRuntime != 100.0f || playerState.yawVelocityLimit != 12.0f ||
        playerState.vehiclePitchRad != 0.0f || playerState.vehicleRollRad != 0.0f) {
        return 1;
    }

    if (!FloatNear(playerState.fxOffsetWorld.x, 12.0f) ||
        !FloatNear(playerState.fxOffsetWorld.y, 9.0f) ||
        !FloatNear(playerState.fxOffsetWorld.z, 25.0f)) {
        return 2;
    }
    if (!FloatNear(playerState.previousTransform.posX, objectData.localMatrix[9]) ||
        !FloatNear(playerState.previousTransform.posY, objectData.localMatrix[10]) ||
        !FloatNear(playerState.previousTransform.posZ, objectData.localMatrix[11])) {
        return 4;
    }

    return FloatNear(playerState.bankBasis.x, 0.0f) &&
                   FloatNear(playerState.bankBasis.y, 0.0f) &&
                   FloatNear(playerState.bankBasis.z, -1.0f) &&
                   playerState.cachedPitchRad == playerState.vehiclePitchRad &&
                   playerState.cachedYawRad == playerState.restartYawRad &&
                   playerState.cachedRollRad == playerState.vehicleRollRad
               ? 0
               : 3;
}

extern "C" int player_zar_write_mission_save_data_section_smoke(void) {
    char tempPath[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    GetTempPathA(sizeof(tempPath), tempPath);
    GetTempFileNameA(tempPath, "pms", 0, tempFile);

    HANDLE const file =
        CreateFileA(tempFile, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return 1;
    }

    zZbdManager manager = {};
    manager.indexArchive.hFile = file;

    zZbdSectionHandler handler = {};
    handler.sectionName = "Player";
    zZbdSectionCallbackCtx callbackCtx = {};
    callbackCtx.manager = &manager;
    callbackCtx.sectionHandler = &handler;

    zClass_NodePartial rootNode = {};
    std::strcpy(rootNode.name, "local_player");
    zClass_NodePartial cameraNode = {};
    zClass_CameraDataPartial cameraData = {};
    PlayerMasterModalData modalData = {};
    PlayerModalState modalState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_SaveGameState saveState = {};
    OptCatalogEntryDef hitSource = {};

    playerState.rootNode = &rootNode;
    playerState.activeAltGunController = &playerState.altWeaponBanks[1].controllerA;
    playerState.activePrimaryGunController = &playerState.altWeaponBanks[2].controllerB;
    playerState.altWeaponBanks[1].controllerA.weaponBankIndex = 1;
    playerState.altWeaponBanks[1].controllerA.weaponSideIndex = 0;
    playerState.altWeaponBanks[2].controllerB.weaponBankIndex = 2;
    playerState.altWeaponBanks[2].controllerB.weaponSideIndex = 1;
    playerState.timedHitStatus.runtimeFlags = 1;
    playerState.timedHitStatus.hitSource = &hitSource;
    playerState.timedHitStatus.nextUpdateTime = 100.0f;
    hitSource.ordinalIndex = 88;

    modalData.masterType = 66;
    modalState.masterModalData = &modalData;
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    cameraData.targetOrEuler = {1.0f, 2.0f, 3.0f};
    cameraData.posOffset = {4.0f, 5.0f, 6.0f};

    zUtil_SaveGameState *const oldLocalSaveState = g_LocalPlayerSaveState;
    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    const zTag4Partial oldVariantTag = g_Variant_CurrentTag;
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;
    g_LocalPlayerSaveState = &saveState;
    g_MainCamera = &cameraNode;
    g_Variant_CurrentTag.count = 2;
    g_Variant_CurrentTag.tags[0] = 7;
    g_Variant_CurrentTag.tags[1] = 8;
    g_Variant_CurrentTag.tags[2] = 0xff;
    g_Time_AccumulatedTimeSec = 90.0f;

    const int result = Player::ZAR_WriteMissionSaveDataSection(&callbackCtx, nullptr);

    PlayerMissionSaveData readBack = {};
    DWORD read = 0;
    SetFilePointer(file, 0, nullptr, FILE_BEGIN);
    ReadFile(file, &readBack, sizeof(readBack), &read, nullptr);

    const bool ok = result == 1 && read == sizeof(readBack) && manager.indexArchive.recordCount == 1 &&
                    manager.indexArchive.records != nullptr &&
                    std::strcmp(manager.indexArchive.records[0].name, "Player/local_player") == 0 &&
                    readBack.size == sizeof(PlayerMissionSaveData) &&
                    readBack.primaryWeaponBankIndex == 2 &&
                    readBack.primaryWeaponSideIndex == 1 &&
                    readBack.playerMasterType == 66 &&
                    Vec3Equals(readBack.cameraTarget, cameraData.targetOrEuler) &&
                    Vec3Equals(readBack.cameraPosition, cameraData.posOffset) &&
                    readBack.lastValidCameraVariantTag.count == 2 &&
                    readBack.lastValidCameraVariantTag.tags[0] == 7 &&
                    readBack.lastValidCameraVariantTag.tags[1] == 8 &&
                    readBack.lastValidCameraVariantTag.tags[2] == 0xff &&
                    readBack.timedHitStatus.savedHitSourceEntryId == 88 &&
                    readBack.timedHitStatus.nextUpdateTime == 10.0f;

    g_LocalPlayerSaveState = oldLocalSaveState;
    g_MainCamera = oldMainCamera;
    g_Variant_CurrentTag = oldVariantTag;
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    std::free(manager.indexArchive.records);
    manager.indexArchive.records = nullptr;
    CloseHandle(file);
    return ok ? 0 : 2;
}

extern "C" int player_restore_recorded_node_flags_smoke(void) {
    zClass_NodePartial untouched = {};
    zClass_NodePartial allFlags = {};
    zClass_NodePartial rayOnly = {};
    untouched.flags = 0;
    allFlags.flags = 0;
    rayOnly.flags = 0x20;

    PlayerNodeFlagRestoreEntry entries[3] = {};
    entries[0].node = &untouched;
    entries[1].node = &allFlags;
    entries[1].wasCellPickable = 1;
    entries[1].wasRaycastable = 1;
    entries[1].wasPickable = 1;
    entries[2].node = &rayOnly;
    entries[2].wasRaycastable = 1;

    PlayerNodeFlagRestoreEntry *const oldBegin = g_PlayerNodeFlagRestoreEntriesBegin;
    PlayerNodeFlagRestoreEntry *const oldEnd = g_PlayerNodeFlagRestoreEntriesEnd;
    PlayerNodeFlagRestoreEntry *const oldCapacity = g_PlayerNodeFlagRestoreEntriesCapacityEnd;
    g_PlayerNodeFlagRestoreEntriesBegin = entries;
    g_PlayerNodeFlagRestoreEntriesEnd = entries + 3;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = entries + 3;

    Player::RestoreRecordedNodeFlags();

    const bool ok = untouched.flags == 0 && (allFlags.flags & 0x38) == 0x38 &&
                    (rayOnly.flags & 0x10) != 0 && (rayOnly.flags & 0x20) != 0 &&
                    (rayOnly.flags & 0x08) == 0;

    g_PlayerNodeFlagRestoreEntriesBegin = oldBegin;
    g_PlayerNodeFlagRestoreEntriesEnd = oldEnd;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = oldCapacity;
    return ok ? 0 : 1;
}

extern "C" int zutil_save_game_state_free_owned_resources_smoke(void) {
    zUtil_SaveGameState saveState = {};
    saveState.playerState = AllocZeroedMalloc<zUtil_PlayerStateStorage>();

    GameNetPlayerRow row = {};
    row.saveState = reinterpret_cast<GameNetPlayerSaveState *>(&saveState);
    saveState.netPlayerRow = &row;

    PlayerModalState *const firstModal = AllocZeroedMalloc<PlayerModalState>();
    PlayerModalState *const secondModal = AllocZeroedMalloc<PlayerModalState>();
    firstModal->next = secondModal;
    saveState.primaryModalState = firstModal;
    saveState.modalStateListHead = firstModal;
    saveState.modalStateListTail = secondModal;
    saveState.modalStateCount = 2;
    saveState.modalStateListAux = 1;

    saveState.FreeOwnedResources();

    return row.saveState == nullptr && saveState.primaryModalState == nullptr &&
                   saveState.modalStateListHead == nullptr &&
                   saveState.modalStateListTail == nullptr && saveState.modalStateCount == 0 &&
                   saveState.modalStateListAux == 0
               ? 0
               : 1;
}

extern "C" int player_zar_write_vehicle_list_section_smoke(void) {
    char tempPath[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    GetTempPathA(sizeof(tempPath), tempPath);
    GetTempFileNameA(tempPath, "pvl", 0, tempFile);

    HANDLE const file =
        CreateFileA(tempFile, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return 1;
    }

    zZbdManager manager = {};
    manager.indexArchive.hFile = file;

    zZbdSectionHandler handler = {};
    handler.sectionName = "VehicleList";
    zZbdSectionCallbackCtx callbackCtx = {};
    callbackCtx.manager = &manager;
    callbackCtx.sectionHandler = &handler;

    zClass_NodePartial rootNode = {};
    std::strcpy(rootNode.name, "local_vehicle");

    PlayerMasterModalData modalData = {};
    modalData.masterType = 77;
    PlayerModalState modalState = {};
    modalState.masterModalData = &modalData;

    zUtil_PlayerStateStorage playerState = {};
    playerState.rootNode = &rootNode;
    playerState.vehicleRotationAngles = {1.0f, 2.0f, 3.0f};
    playerState.worldPos = {4.0f, 5.0f, 6.0f};
    playerState.aiNetId = 1001;
    playerState.aiTopLevelState = 11;
    playerState.aiSavedTopLevelState = 12;
    playerState.aiReturnTopLevelState = 13;
    playerState.aiAttackRadiusSq = 14.0f;
    playerState.aiRestoreDistanceSq = 15.0f;
    playerState.aiRestoreTarget = {16.0f, 17.0f, 18.0f};
    playerState.aiDynamicOffsetDir = {19.0f, 20.0f, 21.0f};
    playerState.aiActivationRadiusSq = 22.0f;
    playerState.aiTickSuppressed = 23;
    playerState.recentHitFlag = 24;
    playerState.recentHitMarkerHandle = 25;
    playerState.aiActive = 26;
    playerState.aiPathCursorAdvanceRequested = 27;
    playerState.aiCurrentSteeringSubstate = 28;
    playerState.aiReturnSteeringSubstate = 29;
    playerState.masterType = 30;
    playerState.statusMeterScaled = 31.0f;
    playerState.statusMeterValue = 32.0f;
    playerState.nanitePanelLevel = 33;

    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    zUtil_SaveGameState *const oldHead = g_PlayerSaveStateListHead;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    g_PlayerSaveStateListHead = &saveState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;

    const int result = Player::ZAR_WriteVehicleListSection(&callbackCtx, nullptr);

    PlayerVehicleListSaveEntry readBack = {};
    DWORD read = 0;
    SetFilePointer(file, 0, nullptr, FILE_BEGIN);
    ReadFile(file, &readBack, sizeof(readBack), &read, nullptr);

    const bool ok = result == 1 && read == sizeof(readBack) && manager.indexArchive.recordCount == 1 &&
                    manager.indexArchive.records != nullptr &&
                    std::strcmp(manager.indexArchive.records[0].name,
                                "VehicleList/local_vehicle") == 0 &&
                    readBack.size == 128 &&
                    Vec3Equals(readBack.vehicleRotationAngles, playerState.vehicleRotationAngles) &&
                    Vec3Equals(readBack.worldPos, playerState.worldPos) &&
                    readBack.aiNetId == playerState.aiNetId &&
                    readBack.aiTopLevelState == playerState.aiTopLevelState &&
                    readBack.aiSavedTopLevelState == playerState.aiSavedTopLevelState &&
                    readBack.aiReturnTopLevelState == playerState.aiReturnTopLevelState &&
                    readBack.aiAttackRadiusSq == playerState.aiAttackRadiusSq &&
                    readBack.aiRestoreDistanceSq == playerState.aiRestoreDistanceSq &&
                    Vec3Equals(readBack.aiRestoreTarget, playerState.aiRestoreTarget) &&
                    Vec3Equals(readBack.aiDynamicOffsetDir, playerState.aiDynamicOffsetDir) &&
                    readBack.aiActivationRadiusSq == playerState.aiActivationRadiusSq &&
                    readBack.aiTickSuppressed == playerState.aiTickSuppressed &&
                    readBack.aiAlertFlag == playerState.recentHitFlag &&
                    readBack.aiStateMarkerHandle == playerState.recentHitMarkerHandle &&
                    readBack.aiActive == playerState.aiActive &&
                    readBack.aiPathCursorAdvanceRequested == playerState.aiPathCursorAdvanceRequested &&
                    readBack.aiCurrentSteeringSubstate == playerState.aiCurrentSteeringSubstate &&
                    readBack.aiReturnSteeringSubstate == playerState.aiReturnSteeringSubstate &&
                    readBack.masterType == playerState.masterType &&
                    readBack.statusMeterScaled == playerState.statusMeterScaled &&
                    readBack.statusMeterValue == playerState.statusMeterValue &&
                    readBack.nanitePanelLevel == playerState.nanitePanelLevel &&
                    readBack.localMasterType == modalData.masterType;

    g_PlayerSaveStateListHead = oldHead;
    g_GameStateOrMapTable = oldGameState;
    std::free(manager.indexArchive.records);
    manager.indexArchive.records = nullptr;
    CloseHandle(file);
    return ok ? 0 : 2;
}

extern "C" int player_destroy_save_game_state_smoke(void) {
    zUtil_SaveGameState *const saveState = AllocZeroedNew<zUtil_SaveGameState>();
    saveState->playerState = AllocZeroedMalloc<zUtil_PlayerStateStorage>();

    zClass_NodeFreeListSlot rootSlot = {};
    saveState->playerState->rootNode = &rootSlot.node;

    HudUiMgrSensorTrackNode *const trackNode = AllocZeroedMalloc<HudUiMgrSensorTrackNode>();
    rootSlot.node.callbackContext = reinterpret_cast<zClass_NodePartial *>(trackNode);

    g_HudUiMgrSensor_TrackList.trackListAux = 1;
    g_HudUiMgrSensor_TrackList.head = trackNode;
    g_HudUiMgrSensor_TrackList.tail = trackNode;
    g_HudUiMgrSensor_TrackList.count = 1;

    g_PlayerSaveStateListAux = 1;
    g_PlayerSaveStateListHead = saveState;
    g_PlayerSaveStateListTail = saveState;
    g_PlayerSaveStateCount = 1;

    Player::DestroySaveGameState(saveState);

    return g_HudUiMgrSensor_TrackList.head == nullptr &&
                   g_HudUiMgrSensor_TrackList.tail == nullptr &&
                   g_HudUiMgrSensor_TrackList.count == 0 &&
                   g_HudUiMgrSensor_TrackList.trackListAux == 0 &&
                   g_PlayerSaveStateListHead == nullptr && g_PlayerSaveStateListTail == nullptr &&
                   g_PlayerSaveStateCount == 0 && g_PlayerSaveStateListAux == 0
               ? 0
               : 1;
}

extern "C" int player_shutdown_mission_runtime_smoke(void) {
    auto *const trackNode = AllocZeroedNew<HudUiMgrSensorTrackNode>();
    g_HudUiMgrSensor_TrackList.head = trackNode;
    g_HudUiMgrSensor_TrackList.tail = trackNode;
    g_HudUiMgrSensor_TrackList.count = 1;
    g_HudUiMgrSensor_TrackList.trackListAux = 1;

    PlayerMasterCommonData *const commonData = AllocZeroedNew<PlayerMasterCommonData>();
    PlayerMasterWeaponSpec *const weaponSpec = AllocZeroedNew<PlayerMasterWeaponSpec>();
    commonData->weaponSpecHead = weaponSpec;
    commonData->weaponSpecTail = weaponSpec;
    commonData->weaponSpecCount = 1;
    commonData->weaponSpecListAux = 1;
    g_PlayerMasterCommonDataHead = commonData;
    g_PlayerMasterCommonDataTail = commonData;
    g_PlayerMasterCommonDataCount = 1;
    g_PlayerMasterCommonDataListAux = 1;

    PlayerMasterModalData *const modalData = AllocZeroedNew<PlayerMasterModalData>();
    g_PlayerMasterModalDataHead = modalData;
    g_PlayerMasterModalDataTail = modalData;
    g_PlayerMasterModalDataCount = 1;
    g_PlayerMasterModalDataListAux = 1;

    g_Player_NextOrdinal = 7;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(0x1234);

    HudUiContainer *const fxContainer =
        reinterpret_cast<HudUiContainer *>(&g_zVideo_FxPass3ConfigLocal);
    fxContainer->childHead = &g_Player_UnderwaterFxPass3Ui;
    fxContainer->childTail = &g_Player_State7FxPass3Ui;
    g_Player_UnderwaterFxPass3Ui.next = &g_Player_State7FxPass3Ui;
    g_Player_UnderwaterFxPass3Ui.parent = fxContainer;
    g_Player_State7FxPass3Ui.next = nullptr;
    g_Player_State7FxPass3Ui.parent = fxContainer;

    Player::ShutdownMissionRuntime();

    return g_HudUiMgrSensor_TrackList.head == nullptr &&
                   g_HudUiMgrSensor_TrackList.tail == nullptr &&
                   g_HudUiMgrSensor_TrackList.count == 0 &&
                   g_PlayerMasterCommonDataHead == nullptr &&
                   g_PlayerMasterCommonDataTail == nullptr && g_PlayerMasterCommonDataCount == 0 &&
                   g_PlayerMasterModalDataHead == nullptr &&
                   g_PlayerMasterModalDataTail == nullptr && g_PlayerMasterModalDataCount == 0 &&
                   g_Player_NextOrdinal == 0 && g_GameStateOrMapTable == nullptr &&
                   fxContainer->childHead == nullptr && fxContainer->childTail == nullptr
               ? 0
               : 1;
}
