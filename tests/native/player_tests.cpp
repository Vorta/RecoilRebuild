#include "Battlesport/GameNet.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zUtil/zZbd.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <cstdlib>
#include <cstring>

namespace {
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
