#include "Battlesport/GameNet.h"
#include "Battlesport/player.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/include/zClass.h"
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
