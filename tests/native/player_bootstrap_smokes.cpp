#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/GameNet.h"
#include "Battlesport/ainet.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zUtil/zZbd.h"

#include <windows.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
using TestBackendSimpleFn = std::int32_t(__stdcall *)(void *self);
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
    TestBackendSetIntFn SetPan;
    TestBackendSetIntFn SetFrequency;
    TestBackendSimpleFn Stop;
};

struct TestDirectSoundBuffer {
    TestDirectSoundBufferVTable *vtable;
};

int g_PlayerBootstrapTestPlayCount;
int g_PlayerBootstrapTestStopCount;

std::int32_t __stdcall TestDirectSoundGetStatus(void *, std::int32_t *status) {
    *status = 0;
    return 0;
}

std::int32_t __stdcall TestDirectSoundSetInt(void *, std::int32_t) {
    return 0;
}

std::int32_t __stdcall TestDirectSoundPlay(void *, std::uint32_t, std::uint32_t,
                                           std::uint32_t) {
    ++g_PlayerBootstrapTestPlayCount;
    return 0;
}

std::int32_t __stdcall TestDirectSoundStop(void *) {
    ++g_PlayerBootstrapTestStopCount;
    return 0;
}

bool FloatNear(
    float actual,
    float expected
) {
    return std::fabs(actual - expected) < 0.0001f;
}

bool Vec3Equals(
    const zVec3 &value,
    const zVec3 &expected
) {
    return FloatNear(value.x, expected.x) &&
           FloatNear(value.y, expected.y) &&
           FloatNear(value.z, expected.z);
}

float PlayerFastSqrtEstimateForTest(
    float value
) {
    std::int32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    bits = (bits >> 1) + 0x1fc00000;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

zZbdManager MakePlayerZbdManager(
    zZbdSectionHandlerNode &sentinel
) {
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    sentinel.sectionHandler = {};

    zZbdManager manager = {};
    manager.sectionHandlerListSentinel = &sentinel;
    return manager;
}

void ClearPlayerRegisteredHandlers(
    zZbdSectionHandlerNode &sentinel
) {
    zZbdSectionHandlerNode *node = sentinel.next;
    while (node != &sentinel) {
        zZbdSectionHandlerNode *const next = node->next;
        delete node;
        node = next;
    }
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
}

void SetObjectLocalMatrix(
    zClass_Object3DDataPartial *data,
    const zMat4x3 &matrix
) {
    std::memcpy(data->localMatrix, &matrix, sizeof(matrix));
}
} // namespace

extern "C" int ainet_find_by_net_id_smoke(void) {
    AINet first = {};
    AINet second = {};
    AINet third = {};
    first.netId = 10;
    second.netId = 20;
    third.netId = 30;
    first.next = &second;
    second.next = &third;

    AINet *const oldHead = g_AINetListHead;
    g_AINetListHead = &first;

    const bool ok = AINet::FindByNetId(20) == &second &&
                    AINet::FindByNetId(99) == 0;
    g_AINetListHead = 0;
    const bool emptyOk = AINet::FindByNetId(10) == 0;

    g_AINetListHead = oldHead;
    return ok && emptyOk ? 0 : 1;
}

extern "C" int ainet_find_nearest_node_smoke(void) {
    AINetNode first = {};
    AINetNode second = {};
    AINetNode third = {};
    first.position.x = 10.0f;
    first.position.y = 0.0f;
    first.position.z = 0.0f;
    second.position.x = 1.0f;
    second.position.y = 2.0f;
    second.position.z = 3.0f;
    third.position.x = -2.0f;
    third.position.y = 0.0f;
    third.position.z = 1.0f;
    first.next = &second;
    second.next = &third;

    const zVec3 query = {0.0f, 0.0f, 0.0f};
    return AINet::FindNearestNode(
        &query,
        &first
    ) == &third &&
                   AINet::FindNearestNode(
                       &query,
                       0
                   ) == 0
               ? 0
               : 1;
}

extern "C" int player_get_save_state_list_head_smoke(void) {
    zUtil_SaveGameState *const oldHead = g_PlayerSaveStateListHead;
    zUtil_SaveGameState saveState = {};

    g_PlayerSaveStateListHead = 0;
    const bool nullOk = Player::GetSaveStateListHead() == 0;

    g_PlayerSaveStateListHead = &saveState;
    const bool valueOk = Player::GetSaveStateListHead() == &saveState;

    g_PlayerSaveStateListHead = oldHead;
    return nullOk && valueOk ? 0 : 1;
}

extern "C" int player_init_save_state_list_smoke(void) {
    zUtil_SaveGameState *const oldHead = g_PlayerSaveStateListHead;
    zUtil_SaveGameState *const oldTail = g_PlayerSaveStateListTail;
    const int oldAux = g_PlayerSaveStateListAux;
    const int oldCount = g_PlayerSaveStateCount;

    zUtil_SaveGameState head = {};
    zUtil_SaveGameState tail = {};
    g_PlayerSaveStateListAux = 1;
    g_PlayerSaveStateListHead = &head;
    g_PlayerSaveStateListTail = &tail;
    g_PlayerSaveStateCount = 2;

    Player::InitSaveStateList();

    const bool ok = g_PlayerSaveStateListAux == 0 &&
                    g_PlayerSaveStateListHead == 0 &&
                    g_PlayerSaveStateListTail == 0 &&
                    g_PlayerSaveStateCount == 0;

    g_PlayerSaveStateListHead = oldHead;
    g_PlayerSaveStateListTail = oldTail;
    g_PlayerSaveStateListAux = oldAux;
    g_PlayerSaveStateCount = oldCount;
    return ok ? 0 : 1;
}

extern "C" int player_clone_type6_node_from_template_and_rename_smoke(void) {
    zClass_TypeListLink *const oldType6Head = zClass_TypeList::Head(6);
    zClass_TypeListLink *const oldType6Tail = zClass_TypeList::Tail(6);
    zClass_NodePartial *const oldRuntimeDiScene = g_Player_RuntimeDiScene;
    int *const oldNetworkEnabledPtr = ZOPT_NETWORK_ENABLED;

    int networkEnabled = 1;
    ZOPT_NETWORK_ENABLED = &networkEnabled;

    zClass_NodePartial world = {};
    zClass_NodePartial templateNode = {};
    zClass_TypeListLink templateLink = {&templateNode, 0, 0, 0};

    std::strcpy(templateNode.name, "template_lod");
    templateNode.classId = 6;
    templateNode.flags = 0x04000080;
    templateNode.gridCol = -1;
    templateNode.gridRow = -1;
    world.classId = 2;

    zClass_TypeList::Head(6) = &templateLink;
    zClass_TypeList::Tail(6) = &templateLink;
    g_Player_RuntimeDiScene = &world;

    zClass_NodePartial *const missing =
        Player::CloneType6NodeFromTemplateAndRename("missing_template", "unused");
    zClass_NodePartial *const cloned =
        Player::CloneType6NodeFromTemplateAndRename("template_lod", "runtime_lod");

    const bool ok =
        missing == 0 && cloned == &templateNode &&
        std::strcmp(templateNode.name, "runtime_lod") == 0 &&
        (templateNode.flags & 0x04) != 0 &&
        world.listCountB == 1 &&
        world.listB != 0 &&
        world.listB[0] == &templateNode &&
        templateNode.listCountA == 1 &&
        templateNode.listA != 0 &&
        templateNode.listA[0] == &world &&
        templateNode.gridCol == -1 &&
        templateNode.gridRow == -1;

    std::free(world.listB);
    std::free(templateNode.listA);
    zClass_TypeList::Head(6) = oldType6Head;
    zClass_TypeList::Tail(6) = oldType6Tail;
    g_Player_RuntimeDiScene = oldRuntimeDiScene;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabledPtr;
    return ok ? 0 : 1;
}

extern "C" int player_cache_gun_hardpoints_and_detach_displays_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    zClass_Object3DDataPartial gunData = {};
    zClass_Object3DDataPartial centerData = {};
    zClass_Object3DDataPartial leftData = {};
    zClass_Object3DDataPartial rightData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial gunNode = {};
    zClass_NodePartial centerNode = {};
    zClass_NodePartial leftNode = {};
    zClass_NodePartial rightNode = {};
    zClass_NodePartial *rootChildren[] = {&gunNode};
    zClass_NodePartial *gunChildren[] = {&centerNode, &leftNode, &rightNode};

    std::strcpy(rootNode.name, "root");
    std::strcpy(gunNode.name, "gun");
    std::strcpy(centerNode.name, "fpnt_c");
    std::strcpy(leftNode.name, "fpnt_l");
    std::strcpy(rightNode.name, "fpnt_r");

    rootNode.listCountB = 1;
    rootNode.listB = rootChildren;
    gunNode.listCountB = 3;
    gunNode.listB = gunChildren;

    gunNode.classId = 5;
    centerNode.classId = 5;
    leftNode.classId = 5;
    rightNode.classId = 5;
    gunNode.classData = &gunData;
    centerNode.classData = &centerData;
    leftNode.classData = &leftData;
    rightNode.classData = &rightData;

    const zMat4x3 gunMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                               0.0f, 0.0f, 1.0f, 11.0f, 12.0f, 13.0f};
    const zMat4x3 centerMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                  0.0f, 0.0f, 1.0f, 21.0f, 22.0f, 23.0f};
    const zMat4x3 leftMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                0.0f, 0.0f, 1.0f, 31.0f, 32.0f, 33.0f};
    const zMat4x3 rightMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                 0.0f, 0.0f, 1.0f, 41.0f, 42.0f, 43.0f};
    SetObjectLocalMatrix(&gunData, gunMatrix);
    SetObjectLocalMatrix(&centerData, centerMatrix);
    SetObjectLocalMatrix(&leftData, leftMatrix);
    SetObjectLocalMatrix(&rightData, rightMatrix);

    playerState.rootNode = &rootNode;
    centerNode.userDataOrDiRef = 77;
    leftNode.userDataOrDiRef = 88;
    rightNode.userDataOrDiRef = 99;

    Player::CacheGunHardpointsAndDetachDisplays(&saveState, 0);
    if (playerState.gunNode != &gunNode ||
        !Vec3Equals(playerState.gunNodeMatrixPos, {11.0f, 12.0f, 13.0f}) ||
        !Vec3Equals(playerState.firePointCenter, {21.0f, 22.0f, 23.0f}) ||
        !Vec3Equals(playerState.firePointLeft, {31.0f, 32.0f, 33.0f}) ||
        !Vec3Equals(playerState.firePointRight, {41.0f, 42.0f, 43.0f}) ||
        centerNode.userDataOrDiRef != 77 ||
        leftNode.userDataOrDiRef != 88 ||
        rightNode.userDataOrDiRef != 99) {
        return 1;
    }

    centerNode.userDataOrDiRef = 0;
    leftNode.userDataOrDiRef = 0;
    rightNode.userDataOrDiRef = 0;
    centerNode.flags = 1;
    leftNode.flags = 1;
    rightNode.flags = 1;

    Player::CacheGunHardpointsAndDetachDisplays(&saveState, 1);
    if (centerNode.userDataOrDiRef != 0 ||
        leftNode.userDataOrDiRef != 0 ||
        rightNode.userDataOrDiRef != 0 ||
        (centerNode.flags & 0x200) != 0 ||
        (leftNode.flags & 0x200) != 0 ||
        (rightNode.flags & 0x200) != 0) {
        return 2;
    }

    zClass_NodePartial emptyRoot = {};
    std::strcpy(emptyRoot.name, "root");
    playerState.rootNode = &emptyRoot;
    playerState.gunNode = &gunNode;
    Player::CacheGunHardpointsAndDetachDisplays(&saveState, 0);
    return playerState.gunNode == 0 ? 0 : 3;
}

extern "C" int player_load_weapon_banks_and_select_defaults_smoke(void) {
    const int oldMissionId = g_HudSensorTracker.missionId;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    OptCatalogEntryDef *const oldEntryTable = g_OptCatalog_EntryTable;
    const int oldEntryCount = g_OptCatalog_EntryCount;
    zZbdManager *const oldZbdManager = g_zUtil_ZbdManager;

    int networkEnabled = 0;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_HudSensorTracker.missionId = 8;

    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager = MakePlayerZbdManager(sentinel);
    g_zUtil_ZbdManager = &manager;

    zClass_NodePartial rootNode = {};
    std::strcpy(rootNode.name, "local_vehicle");

    zUtil_PlayerStateStorage playerState = {};
    zUtil_SaveGameState saveState = {};
    PlayerMasterCommonData commonData = {};
    PlayerMasterWeaponSpec specA = {};
    PlayerMasterWeaponSpec specB = {};
    saveState.playerState = &playerState;
    playerState.masterCommonData = &commonData;
    playerState.rootNode = &rootNode;
    playerState.lifecycleState = 1;

    commonData.weaponSpecHead = &specA;
    commonData.weaponSpecTail = &specB;
    commonData.weaponSpecCount = 2;
    commonData.weaponNodeCount = 2;
    specA.next = &specB;

    std::strcpy(specA.optCatalogName, "WEP_2_0");
    specA.missionRequirementOrGateId = 7;
    specA.mountLayoutFlags = 0;
    specA.startAmmoOrCharge = 12.5f;
    specA.dispatchRepeatDelay = 1.25f;
    specA.aiAttackRangeMin = 2.5f;
    specA.aiAttackRangeMax = 9.5f;
    specA.fireSlotRecoilFlags = 1;
    specA.initialHardpointSelectState = 2;

    std::strcpy(specB.optCatalogName, "WEP_1_1");
    specB.missionRequirementOrGateId = 9;
    specB.startAmmoOrCharge = 33.0f;
    specB.dispatchRepeatDelay = 4.0f;
    specB.aiAttackRangeMin = 5.0f;
    specB.aiAttackRangeMax = 6.0f;

    OptCatalogEntryDef entries[2] = {};
    entries[0].keyName = const_cast<char *>("WEP_2_0");
    entries[0].displayName = const_cast<char *>("MountA");
    entries[1].keyName = const_cast<char *>("WEP_1_1");
    entries[1].displayName = const_cast<char *>("MountB");
    g_OptCatalog_EntryTable = entries;
    g_OptCatalog_EntryCount = 2;

    PlayerGunFireController &oldController = playerState.altWeaponBanks[5].controllerB;
    oldController.flags = 4;
    oldController.ammoOrCharge = 99.0f;
    oldController.attachNodePrimary =
        reinterpret_cast<zClass_NodePartial *>(static_cast<std::uintptr_t>(1));
    oldController.trailRuntimeState =
        static_cast<OptCatalogTrailRuntimeState *>(std::malloc(8));
    if (oldController.trailRuntimeState == 0) {
        return 7;
    }
    playerState.pendingAltCameraToggle = 1;
    playerState.timedHitStatus.runtimeFlags = 3;
    playerState.timedHitStatus.currentLevel = 0.5f;
    playerState.timedHitStatus.targetLevel = 1.0f;

    Player::LoadWeaponBanksAndSelectDefaults(&saveState);

    PlayerGunFireController &availableController = playerState.altWeaponBanks[2].controllerA;
    PlayerGunFireController &lockedController = playerState.altWeaponBanks[1].controllerB;
    zZbdSectionHandlerNode *const minesNode = sentinel.next;

    const bool resetOk =
        oldController.weaponBankIndex == 5 &&
        oldController.weaponSideIndex == 1 &&
        (oldController.flags & 4) == 0 &&
        oldController.ammoOrCharge == 0.0f &&
        oldController.attachNodePrimary == 0 &&
        oldController.trailRuntimeState == 0;
    const bool availableOk =
        availableController.optCatalogEntry == &entries[0] &&
        (availableController.flags & 4) != 0 &&
        (availableController.flags & 1) != 0 &&
        availableController.ammoOrCharge == 12.5f &&
        availableController.dispatchRepeatDelay == 1.25f &&
        availableController.aiAttackRangeMin == 2.5f &&
        availableController.aiAttackRangeMax == 9.5f &&
        availableController.initialHardpointSelectState == 2;
    const bool lockedOk =
        lockedController.optCatalogEntry == &entries[1] &&
        (lockedController.flags & 4) == 0 &&
        lockedController.ammoOrCharge == 0.0f;
    const bool selectionOk =
        playerState.activeAltGunController == &availableController &&
        playerState.activePrimaryGunController == &playerState.altWeaponBanks[1].controllerA &&
        playerState.altHardpointSelectState == 2 &&
        playerState.cachedAltSelectionCode == 200 &&
        playerState.cachedPrimarySelectionCode == 100 &&
        playerState.primaryHardpointSelectState == 2;
    const bool finalStateOk =
        playerState.pendingAltCameraToggle == 0 &&
        playerState.timedHitStatus.runtimeFlags == 0 &&
        playerState.timedHitStatus.currentLevel == 0.0f &&
        playerState.timedHitStatus.targetLevel == 0.0f &&
        playerState.timedHitStatus.lightParentNode == &rootNode;
    const bool zbdOk =
        minesNode != &sentinel &&
        minesNode->sectionHandler.sectionName != 0 &&
        std::strcmp(minesNode->sectionHandler.sectionName, "Mines") == 0 &&
        minesNode->sectionHandler.onPreLoad != 0 &&
        minesNode->sectionHandler.onDataReady != 0 &&
        minesNode->sectionHandler.sortOrder == 1000 &&
        minesNode->sectionHandler.userData == 0;

    ClearPlayerRegisteredHandlers(sentinel);
    g_HudSensorTracker.missionId = oldMissionId;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_OptCatalog_EntryTable = oldEntryTable;
    g_OptCatalog_EntryCount = oldEntryCount;
    g_zUtil_ZbdManager = oldZbdManager;

    if (!resetOk) {
        return 1;
    }
    if (!availableOk) {
        return 2;
    }
    if (!lockedOk) {
        return 3;
    }
    if (!selectionOk) {
        return 4;
    }
    if (!finalStateOk) {
        return 5;
    }
    return zbdOk ? 0 : 6;
}

extern "C" int player_free_alt_weapon_trail_runtime_states_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    playerState.altWeaponBanks[0].controllerA.trailRuntimeState =
        reinterpret_cast<OptCatalogTrailRuntimeState *>(static_cast<std::uintptr_t>(1));
    playerState.altWeaponBanks[0].controllerB.trailRuntimeState =
        reinterpret_cast<OptCatalogTrailRuntimeState *>(static_cast<std::uintptr_t>(2));
    playerState.altWeaponBanks[1].controllerA.trailRuntimeState =
        static_cast<OptCatalogTrailRuntimeState *>(std::malloc(8));
    playerState.altWeaponBanks[4].controllerB.trailRuntimeState =
        static_cast<OptCatalogTrailRuntimeState *>(std::malloc(8));
    playerState.altWeaponBanks[9].controllerA.trailRuntimeState =
        static_cast<OptCatalogTrailRuntimeState *>(std::malloc(8));

    if (playerState.altWeaponBanks[1].controllerA.trailRuntimeState == 0 ||
        playerState.altWeaponBanks[4].controllerB.trailRuntimeState == 0 ||
        playerState.altWeaponBanks[9].controllerA.trailRuntimeState == 0) {
        return 1;
    }

    Player::FreeAltWeaponTrailRuntimeStates(&saveState);
    return playerState.altWeaponBanks[0].controllerA.trailRuntimeState ==
                       reinterpret_cast<OptCatalogTrailRuntimeState *>(
                           static_cast<std::uintptr_t>(1)) &&
                   playerState.altWeaponBanks[0].controllerB.trailRuntimeState ==
                       reinterpret_cast<OptCatalogTrailRuntimeState *>(
                           static_cast<std::uintptr_t>(2))
               ? 0
               : 2;
}

extern "C" int player_check_mission_weapon_availability_smoke(void) {
    const int oldMissionId = g_HudSensorTracker.missionId;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    int networkEnabled = 0;
    ZOPT_NETWORK_ENABLED = &networkEnabled;

    int available = -1;
    g_HudSensorTracker.missionId = 8;
    Player::CheckMissionWeaponAvailability(0, 7, 0x61, &available);
    const bool singlePlayerUnlocked = available == 1;

    available = -1;
    Player::CheckMissionWeaponAvailability(0, 9, 0x61, &available);
    const bool singlePlayerLockedByThreshold = available == 0;

    available = -1;
    Player::CheckMissionWeaponAvailability(0, 0, 0x10, &available);
    const bool singlePlayerZeroThresholdLocked = available == 0;

    networkEnabled = 1;
    available = -1;
    Player::CheckMissionWeaponAvailability(0, 0, 0x61, &available);
    const bool networkMission8LaserSabre = available == 1;

    available = -1;
    Player::CheckMissionWeaponAvailability(0, 0, 0x31, &available);
    const bool networkMission8NapalmLocked = available == 0;

    g_HudSensorTracker.missionId = 11;
    available = -1;
    Player::CheckMissionWeaponAvailability(0, 0, 0x80, &available);
    const bool networkMission11Missile = available == 1;

    g_HudSensorTracker.missionId = 5;
    available = -1;
    Player::CheckMissionWeaponAvailability(0, 0, 0x10, &available);
    const bool earlyNetworkMissionLocked = available == 0;

    g_HudSensorTracker.missionId = oldMissionId;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    return singlePlayerUnlocked && singlePlayerLockedByThreshold &&
                   singlePlayerZeroThresholdLocked && networkMission8LaserSabre &&
                   networkMission8NapalmLocked && networkMission11Missile &&
                   earlyNetworkMissionLocked
               ? 0
               : 1;
}

extern "C" int player_apply_primary_weapon_switch_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerGunFireController previousController = {};
    PlayerGunFireController newController = {};
    zClass_NodePartial previousPrimary = {};
    zClass_NodePartial previousSecondary = {};
    zClass_NodePartial newPrimary = {};
    zClass_NodePartial newSecondary = {};

    saveState.playerState = &playerState;
    previousController.attachNodePrimary = &previousPrimary;
    previousController.attachNodeSecondary = &previousSecondary;
    newController.attachNodePrimary = &newPrimary;
    newController.attachNodeSecondary = &newSecondary;
    newController.weaponBankIndex = 6;
    newController.weaponSideIndex = 1;

    previousPrimary.classId = 5;
    previousSecondary.classId = 5;
    newPrimary.classId = 5;
    newSecondary.classId = 5;
    previousPrimary.flags = 0x04;
    previousSecondary.flags = 0x04;

    Player::ApplyPrimaryWeaponSwitch(&saveState, &previousController, &newController);
    const bool firstSwitchOk =
        playerState.activePrimaryGunController == &newController &&
        playerState.primaryHardpointSelectState == 2 &&
        playerState.cachedPrimarySelectionCode == 601 &&
        (previousPrimary.flags & 0x04) == 0 && (previousSecondary.flags & 0x04) == 0 &&
        (newPrimary.flags & 0x04) != 0 && (newSecondary.flags & 0x04) != 0;
    if (!firstSwitchOk) {
        return 1;
    }

    PlayerGunFireController nullSecondaryPrevious = {};
    PlayerGunFireController nextController = {};
    zClass_NodePartial nullPreviousPrimary = {};
    zClass_NodePartial nextPrimary = {};
    nullPreviousPrimary.classId = 5;
    nullPreviousPrimary.flags = 0x04;
    nextPrimary.classId = 5;
    nullSecondaryPrevious.attachNodePrimary = &nullPreviousPrimary;
    nextController.attachNodePrimary = &nextPrimary;
    nextController.weaponBankIndex = 2;
    nextController.weaponSideIndex = 0;

    Player::ApplyPrimaryWeaponSwitch(&saveState, &nullSecondaryPrevious, &nextController);
    return playerState.activePrimaryGunController == &nextController &&
                   playerState.cachedPrimarySelectionCode == 200 &&
                   (nullPreviousPrimary.flags & 0x04) == 0 && (nextPrimary.flags & 0x04) != 0
               ? 0
               : 2;
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

extern "C" int player_cache_disable_copter_snd_nodes_smoke(void) {
    zClass_TypeListLink *const oldHead = zClass_TypeList::Head(6);
    zClass_TypeListLink *const oldTail = zClass_TypeList::Tail(6);
    zClass_NodePartial *const oldHealthy1 = g_Player_CopterHealthyNode1;
    zClass_NodePartial *const oldHealthy2 = g_Player_CopterHealthyNode2;
    zClass_NodePartial *const oldSnd1 = g_Player_CopterSndNode1;
    zClass_NodePartial *const oldSnd2 = g_Player_CopterSndNode2;
    zSndSample *const oldSample = g_Player_CopterSndSample;
    const int oldBackend = g_zSnd_ActiveBackend;

    zClass_NodePartial copter1 = {};
    zClass_NodePartial copter2 = {};
    zClass_NodePartial healthy1 = {};
    zClass_NodePartial healthy2 = {};
    zClass_NodePartial snd1 = {};
    zClass_NodePartial snd2 = {};
    zClass_NodePartial *copter1Children[2] = {&healthy1, &snd1};
    zClass_NodePartial *copter2Children[2] = {&healthy2, &snd2};
    zClass_SoundDataPartial snd1Data = {};
    zClass_SoundDataPartial snd2Data = {};
    zClass_TypeListLink copter1Link = {&copter1, nullptr, nullptr, 0};
    zClass_TypeListLink copter2Link = {&copter2, &copter1Link, nullptr, 0};
    copter1Link.next = &copter2Link;

    std::strcpy(copter1.name, "copter01");
    std::strcpy(copter2.name, "copter02");
    std::strcpy(healthy1.name, "healthy");
    std::strcpy(healthy2.name, "healthy");
    std::strcpy(snd1.name, "snd_chopper");
    std::strcpy(snd2.name, "snd_chopper");
    copter1.listCountB = 2;
    copter1.listB = copter1Children;
    copter2.listCountB = 2;
    copter2.listB = copter2Children;
    snd1.classId = 10;
    snd1.flags = 0x04;
    snd1.classData = &snd1Data;
    snd2.classId = 10;
    snd2.flags = 0x04;
    snd2.classData = &snd2Data;

    TestDirectSoundBufferVTable vtable = {};
    vtable.Stop = &TestDirectSoundStop;
    TestDirectSoundBuffer directSoundBuffer = {&vtable};
    zSndSample sample = {};
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);

    zClass_TypeList::Head(6) = &copter1Link;
    zClass_TypeList::Tail(6) = &copter2Link;
    g_Player_CopterHealthyNode1 = nullptr;
    g_Player_CopterHealthyNode2 = nullptr;
    g_Player_CopterSndNode1 = nullptr;
    g_Player_CopterSndNode2 = nullptr;
    g_Player_CopterSndSample = &sample;
    g_zSnd_ActiveBackend = 0;
    g_PlayerBootstrapTestStopCount = 0;

    Player::CacheDisableCopterSndNodesAndStopSample();
    const bool lazyLookupOk =
        g_Player_CopterHealthyNode1 == &healthy1 && g_Player_CopterHealthyNode2 == &healthy2 &&
        g_Player_CopterSndNode1 == &snd1 && g_Player_CopterSndNode2 == &snd2 &&
        (snd1.flags & 0x04) == 0 && (snd2.flags & 0x04) == 0 &&
        g_PlayerBootstrapTestStopCount == 1;

    snd1.flags = 0x04;
    snd2.flags = 0x04;
    zClass_TypeList::Head(6) = nullptr;
    zClass_TypeList::Tail(6) = nullptr;

    Player::CacheDisableCopterSndNodesAndStopSample();
    const bool cachedPathOk =
        (snd1.flags & 0x04) == 0 && (snd2.flags & 0x04) == 0 &&
        g_PlayerBootstrapTestStopCount == 2;

    zClass_TypeList::Head(6) = oldHead;
    zClass_TypeList::Tail(6) = oldTail;
    g_Player_CopterHealthyNode1 = oldHealthy1;
    g_Player_CopterHealthyNode2 = oldHealthy2;
    g_Player_CopterSndNode1 = oldSnd1;
    g_Player_CopterSndNode2 = oldSnd2;
    g_Player_CopterSndSample = oldSample;
    g_zSnd_ActiveBackend = oldBackend;

    return lazyLookupOk && cachedPathOk ? 0 : 1;
}

extern "C" int player_reactivate_copter_snd_nodes_if_healthy_smoke(void) {
    zClass_NodePartial *const oldHealthy1 = g_Player_CopterHealthyNode1;
    zClass_NodePartial *const oldHealthy2 = g_Player_CopterHealthyNode2;
    zClass_NodePartial *const oldSnd1 = g_Player_CopterSndNode1;
    zClass_NodePartial *const oldSnd2 = g_Player_CopterSndNode2;
    zSndSample *const oldSample = g_Player_CopterSndSample;
    const int oldBackend = g_zSnd_ActiveBackend;

    TestDirectSoundBufferVTable vtable = {};
    vtable.Play = &TestDirectSoundPlay;
    TestDirectSoundBuffer directSoundBuffer = {&vtable};

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    zSndPlayHandle handle1 = {};
    handle1.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);
    zSndPlayHandle handle2 = {};
    handle2.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);
    zClass_SoundDataPartial sndData1 = {};
    sndData1.playHandle = &handle1;
    zClass_SoundDataPartial sndData2 = {};
    sndData2.playHandle = &handle2;

    zClass_NodePartial healthy1 = {};
    zClass_NodePartial healthy2 = {};
    zClass_NodePartial snd1 = {};
    zClass_NodePartial snd2 = {};
    healthy1.flags = 0x04;
    healthy2.flags = 0x04;
    snd1.classId = 10;
    snd1.classData = &sndData1;
    snd2.classId = 10;
    snd2.classData = &sndData2;

    g_Player_CopterHealthyNode1 = &healthy1;
    g_Player_CopterHealthyNode2 = &healthy2;
    g_Player_CopterSndNode1 = &snd1;
    g_Player_CopterSndNode2 = &snd2;
    g_Player_CopterSndSample = &sample;
    g_zSnd_ActiveBackend = 0;
    g_PlayerBootstrapTestPlayCount = 0;

    Player::ReactivateCopterSndNodesIfHealthy();
    const bool activePlaybackOk =
        (snd1.flags & 0x04) != 0 && (snd2.flags & 0x04) != 0 &&
        g_PlayerBootstrapTestPlayCount == 2;

    healthy1.flags = 0;
    snd1.flags = 0;
    snd2.flags = 0;
    sndData2.playHandle = nullptr;
    g_PlayerBootstrapTestPlayCount = 0;

    Player::ReactivateCopterSndNodesIfHealthy();
    const bool inactiveAndMissingHandleOk =
        (snd1.flags & 0x04) == 0 && (snd2.flags & 0x04) != 0 &&
        g_PlayerBootstrapTestPlayCount == 0;

    g_Player_CopterHealthyNode1 = oldHealthy1;
    g_Player_CopterHealthyNode2 = oldHealthy2;
    g_Player_CopterSndNode1 = oldSnd1;
    g_Player_CopterSndNode2 = oldSnd2;
    g_Player_CopterSndSample = oldSample;
    g_zSnd_ActiveBackend = oldBackend;

    return activePlaybackOk && inactiveAndMissingHandleOk ? 0 : 1;
}

extern "C" int player_apply_alt_weapon_switch_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    void *const oldVolumeScalePtr = g_zSnd_GlobalVolumeScalePtr;
    const int oldInitialized = g_zSnd_IsInitialized;
    const int oldPreInitialized = g_zSnd_PreInitialized;
    const int oldBackend = g_zSnd_ActiveBackend;
    const int oldMuteDepth = g_zSnd_MuteDepth;
    const int oldPlaybackEnabled = g_zSnd_Flag10PlaybackEnabled;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    PlayerGunFireController initialController = {};
    initialController.weaponBankIndex = 2;
    initialController.weaponSideIndex = 1;
    playerState.altGunTransitionTimerA = 3.0f;
    playerState.altGunTransitionTimerB = 4.0f;

    Player::ApplyAltWeaponSwitch(&saveState, 0, &initialController);
    const bool initialOk =
        playerState.activeAltGunController == &initialController &&
        playerState.activeAltBankIndex == 2 &&
        playerState.altWeaponBanks[2].selectedSide == 1 &&
        playerState.altHardpointSelectState == 0 &&
        playerState.altGunTransitionTimerA == 0.0f &&
        playerState.altGunTransitionTimerB == 0.0f &&
        playerState.altGunTransitionState == 16 &&
        playerState.altGunTransitionController == &initialController &&
        playerState.cachedAltSelectionCode == 201;
    if (!initialOk) {
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
    g_PlayerBootstrapTestPlayCount = 0;

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    sample.replayFields.gain = 0.5f;
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);

    PlayerMasterCommonData commonData = {};
    commonData.sfxWeaponUp[0] = &sample;
    playerState.masterCommonData = &commonData;

    PlayerGunFireController previousController = {};
    PlayerGunFireController nextController = {};
    previousController.weaponBankIndex = 2;
    previousController.weaponSideIndex = 1;
    nextController.weaponBankIndex = 4;
    nextController.weaponSideIndex = 0;

    OptCatalogEntryDef owner = {};
    OptCatalogTrailRuntimeState runtime = {};
    zClass_NodePartial trailNode = {};
    owner.activeTrailRuntime = &runtime;
    runtime.ownerEntry = &owner;
    runtime.activeNodeSlotCount = 1;
    runtime.activeNodeSlotCursor = 1;
    runtime.activeNodeSlots[0].node = &trailNode;
    trailNode.classId = 5;
    trailNode.flags = 0x04;
    previousController.trailRuntimeState = &runtime;

    playerState.altGunFireHeldFlag = 1;
    playerState.altGunTransitionTimerA = 5.0f;
    playerState.altGunTransitionTimerB = 6.0f;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);

    Player::ApplyAltWeaponSwitch(&saveState, &previousController, &nextController);

    const bool switchOk =
        playerState.activeAltGunController == &nextController &&
        playerState.activeAltBankIndex == 4 &&
        playerState.altWeaponBanks[4].selectedSide == 0 &&
        playerState.altHardpointSelectState == 0 &&
        playerState.altGunTransitionTimerA == 0.0f &&
        playerState.altGunTransitionTimerB == 0.0f &&
        playerState.altGunTransitionState == 4 &&
        playerState.altGunTransitionController == &previousController &&
        playerState.altGunFireHeldFlag == 0 &&
        playerState.cachedAltSelectionCode == 400 &&
        playerState.modeLoopSfxHandle[0] == &sample.primaryVoice &&
        sample.primaryVoice.ownerSample == &sample &&
        g_PlayerBootstrapTestPlayCount == 1 &&
        owner.activeTrailRuntime == 0 &&
        runtime.activeNodeSlotCursor == 0 &&
        (trailNode.flags & 0x04) == 0;

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_zSnd_GlobalVolumeScalePtr = oldVolumeScalePtr;
    g_zSnd_IsInitialized = oldInitialized;
    g_zSnd_PreInitialized = oldPreInitialized;
    g_zSnd_ActiveBackend = oldBackend;
    g_zSnd_MuteDepth = oldMuteDepth;
    g_zSnd_Flag10PlaybackEnabled = oldPlaybackEnabled;

    return switchOk ? 0 : 2;
}

extern "C" int player_apply_aim_pitch_to_direction_smoke(void) {
    zVec3 direction = {3.0f, 9.0f, 4.0f};
    Player::ApplyAimPitchToDirection(&direction, 0.0f);
    const float scale = PlayerFastSqrtEstimateForTest(1.0f / 25.0f);

    int failed = 0;
    failed |= !FloatNear(direction.x, 3.0f * scale);
    failed |= direction.y != 0.0f;
    failed |= !FloatNear(direction.z, 4.0f * scale);

    direction = zVec3_Make(0.0f, 12.0f, 0.0f);
    Player::ApplyAimPitchToDirection(&direction, 0.0f);
    failed |= !Vec3Equals(direction, zVec3_Make(0.0f, 0.0f, -1.0f));

    direction = zVec3_Make(0.0f, 0.0f, 0.0f);
    Player::ApplyAimPitchToDirection(&direction, 0.5f);
    const float diagonal = PlayerFastSqrtEstimateForTest((1.0f - 0.25f) * 0.5f);
    failed |= !FloatNear(direction.x, diagonal);
    failed |= direction.y != 0.5f;
    failed |= !FloatNear(direction.z, diagonal);

    return failed != 0 ? 1 : 0;
}

extern "C" int player_write_mines_zar_section_smoke(void) {
    char tempPath[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    GetTempPathA(sizeof(tempPath), tempPath);
    GetTempFileNameA(tempPath, "pmn", 0, tempFile);

    HANDLE const file =
        CreateFileA(tempFile, GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, 0);
    if (file == INVALID_HANDLE_VALUE) {
        return 1;
    }

    zZbdManager manager = {};
    manager.indexArchive.hFile = file;

    zZbdSectionHandler handler = {};
    handler.sectionName = "Mines";
    zZbdSectionCallbackCtx callbackCtx = {};
    callbackCtx.manager = &manager;
    callbackCtx.sectionHandler = &handler;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;

    OptCatalogEntryDef ignoredEntry = {};
    ignoredEntry.keyName = const_cast<char *>("ignored");
    playerState.altWeaponBanks[3].controllerA.optCatalogEntry = &ignoredEntry;

    OptCatalogEntryDef entry = {};
    entry.keyName = const_cast<char *>("P_HEMINE");
    playerState.altWeaponBanks[4].controllerA.optCatalogEntry = &entry;

    zClass_NodePartial ownerNode = {};
    std::strcpy(ownerNode.name, "mine_node");
    zClass_NodePartial projectileNode = {};
    projectileNode.classId = 5;
    zClass_Object3DDataPartial projectileData = {};
    projectileData.scale = {2.0f, 3.0f, 4.0f};
    projectileNode.classData = &projectileData;

    OptCatalogRuntimeInstanceStorage runtime = {};
    runtime.ownerNode = &ownerNode;
    runtime.projectileNode = &projectileNode;
    runtime.pos = {5.0f, 6.0f, 7.0f};
    entry.activeRuntimeListHead = &runtime;

    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;

    const int result = Player::WriteMinesZarSection(&callbackCtx, 0);

    PlayerMineSaveEntry dummy = {};
    PlayerMineSaveEntry mine = {};
    DWORD readDummy = 0;
    DWORD readMine = 0;
    SetFilePointer(file, 0, 0, FILE_BEGIN);
    ReadFile(file, &dummy, 0x60, &readDummy, 0);
    ReadFile(file, &mine, 0x60, &readMine, 0);

    const bool ok =
        result == 1 && manager.indexArchive.recordCount == 2 &&
        manager.indexArchive.records != 0 &&
        std::strcmp(manager.indexArchive.records[0].name, "Mines/DummyMineData") == 0 &&
        std::strcmp(manager.indexArchive.records[1].name, "Mines/MineData000") == 0 &&
        readDummy == 0x60 && readMine == 0x60 && dummy.resetMarker == 1 &&
        std::strcmp(dummy.ownerNodeName, "Dummy") == 0 && mine.resetMarker == 0 &&
        std::strncmp(mine.optCatalogName, "P_HEMINE", 0x20) == 0 &&
        Vec3Equals(mine.spawnPos, runtime.pos) && Vec3Equals(mine.scale, projectileData.scale) &&
        std::strncmp(mine.ownerNodeName, "mine_node", 0x20) == 0;

    g_GameStateOrMapTable = oldGameState;
    std::free(manager.indexArchive.records);
    manager.indexArchive.records = 0;
    CloseHandle(file);
    return ok ? 0 : 2;
}

extern "C" int player_mines_zar_read_entry_or_reset_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    OptCatalogEntryDef *const oldEntryTable = g_OptCatalog_EntryTable;
    const int oldEntryCount = g_OptCatalog_EntryCount;
    OptCatalogRuntimeInstanceStorage *const oldFreeRuntimeList =
        g_OptCatalogFreeRuntimeInstanceList;
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    const float oldNextSpawnScale = g_OptCatalogNextSpawnScale;
    zClass_TypeListLink *const oldType6Head = zClass_TypeList::Head(6);
    zClass_TypeListLink *const oldType6Tail = zClass_TypeList::Tail(6);

    int result = 0;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;

    OptCatalogEntryDef ignoredEntry = {};
    OptCatalogEntryDef resetEntryA = {};
    OptCatalogEntryDef resetEntryB = {};
    playerState.altWeaponBanks[3].controllerA.optCatalogEntry = &ignoredEntry;
    playerState.altWeaponBanks[4].controllerA.optCatalogEntry = &resetEntryA;
    playerState.altWeaponBanks[5].controllerB.optCatalogEntry = &resetEntryB;

    OptCatalogRuntimeInstanceStorage resetRuntimeA = {};
    OptCatalogRuntimeInstanceStorage resetRuntimeB = {};
    OptCatalogRuntimeInstanceStorage freeSentinel = {};
    zClass_NodeFreeListSlot resetProjectileA = {};
    zClass_NodeFreeListSlot resetProjectileB = {};
    zClass_Object3DDataPartial resetProjectileDataA = {};
    zClass_Object3DDataPartial resetProjectileDataB = {};
    zClass_NodePartial resetRuntimeWorld = {};
    resetRuntimeWorld.classId = 3;
    resetRuntimeWorld.flags = 1;
    resetProjectileA.node.classId = 5;
    resetProjectileA.node.flags = 1;
    resetProjectileA.node.classData = &resetProjectileDataA;
    resetProjectileA.damageHandler = &resetEntryA;
    resetProjectileB.node.classId = 5;
    resetProjectileB.node.flags = 1;
    resetProjectileB.node.classData = &resetProjectileDataB;
    resetProjectileB.damageHandler = &resetEntryB;
    resetRuntimeA.projectileNode = &resetProjectileA.node;
    resetRuntimeA.lifetime = 4.0f;
    resetRuntimeB.projectileNode = &resetProjectileB.node;
    resetRuntimeB.lifetime = 5.0f;
    resetEntryA.activeRuntimeListHead = &resetRuntimeA;
    resetEntryB.activeRuntimeListHead = &resetRuntimeB;
    g_OptCatalogRuntimeWorld = &resetRuntimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = &freeSentinel;
    zClass_Class::AddChild(&resetRuntimeWorld, &resetProjectileA.node);
    zClass_Class::AddChild(&resetRuntimeWorld, &resetProjectileB.node);

    PlayerMineSaveEntry resetData = {};
    resetData.resetMarker = 1;
    Player::Mines_ZAR_ReadEntryOrReset(0, 0, &resetData, sizeof(resetData), 0);

    OptCatalogEntryDef spawnEntry = {};
    OptCatalogRuntimeInstanceStorage spawnRuntime = {};
    zClass_NodeFreeListSlot spawnProjectile = {};
    zClass_Object3DDataPartial spawnProjectileData = {};
    zClass_NodePartial spawnRuntimeWorld = {};
    zClass_NodePartial ownerNode = {};
    zClass_TypeListLink ownerLink = {&ownerNode, 0, 0, 0};
    PlayerMineSaveEntry mineData = {};

    if (ignoredEntry.activeRuntimeListHead != 0 ||
        resetEntryA.activeRuntimeListHead != 0 ||
        resetEntryB.activeRuntimeListHead != 0 || resetRuntimeWorld.listCountB != 0 ||
        resetProjectileA.node.listCountA != 0 || resetProjectileB.node.listCountA != 0 ||
        g_OptCatalogFreeRuntimeInstanceList != &resetRuntimeB ||
        resetRuntimeB.next != &resetRuntimeA || resetRuntimeA.next != &freeSentinel ||
        resetProjectileA.damageHandler != 0 || resetProjectileB.damageHandler != 0 ||
        resetProjectileDataA.scale.x != 1.0f || resetProjectileDataB.scale.x != 1.0f) {
        result = 1;
        goto restore;
    }

    spawnEntry.keyName = const_cast<char *>("P_HEMINE");
    spawnEntry.flyoutHealth = 8.0f;
    g_OptCatalog_EntryTable = &spawnEntry;
    g_OptCatalog_EntryCount = 1;

    spawnRuntimeWorld.classId = 3;
    spawnRuntimeWorld.flags = 1;
    spawnProjectile.node.classId = 5;
    spawnProjectile.node.flags = 1;
    spawnProjectile.node.classData = &spawnProjectileData;
    spawnRuntime.projectileNode = &spawnProjectile.node;
    std::strcpy(ownerNode.name, "mine_owner");
    zClass_TypeList::Head(6) = &ownerLink;
    zClass_TypeList::Tail(6) = &ownerLink;
    g_OptCatalogRuntimeWorld = &spawnRuntimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = &spawnRuntime;
    g_OptCatalogNextSpawnScale = 3.0f;

    std::strcpy(mineData.optCatalogName, "P_HEMINE");
    mineData.spawnPos = {10.0f, 11.0f, 12.0f};
    mineData.scale = {2.0f, 3.0f, 4.0f};
    std::strcpy(mineData.ownerNodeName, "mine_owner");
    Player::Mines_ZAR_ReadEntryOrReset(0, 0, &mineData, sizeof(mineData), 0);

    if (spawnEntry.activeRuntimeListHead != &spawnRuntime ||
        g_OptCatalogFreeRuntimeInstanceList != 0 || spawnRuntime.ownerNode != &ownerNode ||
        !Vec3Equals(spawnRuntime.pos, mineData.spawnPos) || spawnRuntime.spawnScale != 3.0f ||
        g_OptCatalogNextSpawnScale != 1.0f || spawnRuntimeWorld.listCountB != 1 ||
        spawnRuntimeWorld.listB[0] != &spawnProjectile.node ||
        spawnProjectile.node.callbackContext !=
            reinterpret_cast<zClass_NodePartial *>(&spawnRuntime) ||
        spawnProjectileData.scale.x != 2.0f || spawnProjectileData.scale.y != 3.0f ||
        spawnProjectileData.scale.z != 4.0f) {
        result = 2;
        goto restore;
    }

restore:
    g_GameStateOrMapTable = oldGameState;
    g_OptCatalog_EntryTable = oldEntryTable;
    g_OptCatalog_EntryCount = oldEntryCount;
    g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    g_OptCatalogNextSpawnScale = oldNextSpawnScale;
    zClass_TypeList::Head(6) = oldType6Head;
    zClass_TypeList::Tail(6) = oldType6Tail;
    return result;
}

extern "C" int player_remove_all_deployed_mines_smoke(void) {
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    OptCatalogRuntimeInstanceStorage *const oldFreeRuntimeList =
        g_OptCatalogFreeRuntimeInstanceList;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    zClass_NodePartial rootNode = {};
    playerState.rootNode = &rootNode;

    OptCatalogEntryDef ignoredEntry = {};
    OptCatalogEntryDef mineEntries[4] = {};
    OptCatalogRuntimeInstanceStorage ignoredRuntime = {};
    OptCatalogRuntimeInstanceStorage mineRuntimes[4] = {};
    zClass_NodeFreeListSlot ignoredProjectile = {};
    zClass_NodeFreeListSlot mineProjectiles[4] = {};
    zClass_Object3DDataPartial ignoredData = {};
    zClass_Object3DDataPartial mineData[4] = {};
    zClass_NodePartial runtimeWorld = {};
    zClass_NodePartial *worldChildren[5] = {
        &ignoredProjectile.node,
        &mineProjectiles[0].node,
        &mineProjectiles[1].node,
        &mineProjectiles[2].node,
        &mineProjectiles[3].node,
    };

    runtimeWorld.classId = 3;
    runtimeWorld.listB = worldChildren;
    runtimeWorld.listCountB = 5;
    ignoredProjectile.node.classId = 5;
    ignoredProjectile.node.classData = &ignoredData;
    ignoredRuntime.ownerNode = &rootNode;
    ignoredRuntime.projectileNode = &ignoredProjectile.node;
    ignoredRuntime.lifetime = 0.0f;
    ignoredEntry.activeRuntimeListHead = &ignoredRuntime;
    playerState.altWeaponBanks[3].controllerA.optCatalogEntry = &ignoredEntry;

    for (int index = 0; index < 4; ++index) {
        mineProjectiles[index].node.classId = 5;
        mineProjectiles[index].node.classData = &mineData[index];
        mineRuntimes[index].ownerNode = &rootNode;
        mineRuntimes[index].projectileNode = &mineProjectiles[index].node;
        mineRuntimes[index].lifetime = 0.0f;
        mineEntries[index].activeRuntimeListHead = &mineRuntimes[index];
    }

    playerState.altWeaponBanks[4].controllerA.optCatalogEntry = &mineEntries[0];
    playerState.altWeaponBanks[4].controllerB.optCatalogEntry = &mineEntries[1];
    playerState.altWeaponBanks[5].controllerA.optCatalogEntry = &mineEntries[2];
    playerState.altWeaponBanks[5].controllerB.optCatalogEntry = &mineEntries[3];

    OptCatalogRuntimeInstanceStorage freeSentinel = {};
    g_OptCatalogRuntimeWorld = &runtimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = &freeSentinel;

    Player::RemoveAllDeployedMines(&saveState);

    const bool ignoredOk =
        ignoredEntry.activeRuntimeListHead == &ignoredRuntime &&
        ignoredRuntime.next == 0 &&
        ignoredProjectile.node.listCountA == 0;
    const bool minesCleared =
        mineEntries[0].activeRuntimeListHead == 0 &&
        mineEntries[1].activeRuntimeListHead == 0 &&
        mineEntries[2].activeRuntimeListHead == 0 &&
        mineEntries[3].activeRuntimeListHead == 0;
    const bool freeListOk =
        g_OptCatalogFreeRuntimeInstanceList == &mineRuntimes[3] &&
        mineRuntimes[3].next == &mineRuntimes[2] &&
        mineRuntimes[2].next == &mineRuntimes[1] &&
        mineRuntimes[1].next == &mineRuntimes[0] &&
        mineRuntimes[0].next == &freeSentinel;
    const bool worldOk =
        runtimeWorld.listCountB == 1 &&
        runtimeWorld.listB[0] == &ignoredProjectile.node;

    int failure = 0;
    if (!ignoredOk) {
        failure = 1;
    } else if (!minesCleared) {
        failure = 2;
    } else if (!freeListOk) {
        failure = 3;
    } else if (!worldOk) {
        failure = 4;
    }

    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
    return failure;
}

extern "C" int player_init_state_from_name_and_master_common_data_smoke(void) {
    zUtil_SaveGameState *const oldSaveHead = g_PlayerSaveStateListHead;
    zUtil_SaveGameState *const oldSaveTail = g_PlayerSaveStateListTail;
    const int oldSaveCount = g_PlayerSaveStateCount;
    PlayerMasterCommonData *const oldCommonHead = g_PlayerMasterCommonDataHead;
    PlayerMasterCommonData *const oldCommonTail = g_PlayerMasterCommonDataTail;
    const int oldCommonAux = g_PlayerMasterCommonDataListAux;
    const int oldCommonCount = g_PlayerMasterCommonDataCount;
    const int oldNextOrdinal = g_Player_NextOrdinal;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;
    int *const oldGameControlOptions = ZOPT_GAME_CONTROL_OPTIONS;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    zZbdManager *const oldZbdManager = g_zUtil_ZbdManager;
    AINet *const oldAiHead = g_AINetListHead;
    AINet *const oldAiTail = g_AINetListTail;
    zEffectAnimEntry *const oldEffectEntries = g_zEffectAnim_EntryList;
    const short oldEffectCount = g_zEffectAnim_EntryCount;
    int gameControlOptions = 0;
    int networkEnabled = 0;

    PlayerMasterCommonData commonData = {};
    std::strcpy(commonData.vehicleName, "tank_common");
    std::strcpy(commonData.startAnimsName, "startup");
    commonData.cambackSide0 = 1.0f;
    commonData.cambackBase0 = 2.0f;
    commonData.cambackDist0 = 3.0f;
    commonData.cambackSide1 = 4.0f;
    commonData.cambackBase1 = 5.0f;
    commonData.cambackDist1 = 6.0f;
    commonData.cambackSide2 = 7.0f;
    commonData.cambackBase2 = 8.0f;
    commonData.cambackDist2 = 9.0f;
    commonData.aimYawRate = 10.0f;
    commonData.aimYawMax = 11.0f;
    commonData.maxHealth = 250.0f;

    zClass_Object3DDataPartial rootData = {};
    zClass_Object3DDataPartial targetData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial targetNode = {};
    zClass_NodePartial bodyNode = {};
    zClass_NodePartial turretNode = {};
    zClass_NodePartial doorLeftNode = {};
    zClass_NodePartial doorRightNode = {};
    zClass_NodePartial shadowNode = {};
    zClass_NodePartial *rootChildren[] = {&targetNode, &bodyNode, &turretNode,
                                          &doorLeftNode, &doorRightNode, &shadowNode};

    std::strcpy(rootNode.name, "tank");
    std::strcpy(targetNode.name, "target");
    std::strcpy(bodyNode.name, "body");
    std::strcpy(turretNode.name, "turret");
    std::strcpy(doorLeftNode.name, "doorleft");
    std::strcpy(doorRightNode.name, "doorright");
    std::strcpy(shadowNode.name, "shadow");
    rootNode.classId = 5;
    targetNode.classId = 5;
    rootNode.classData = &rootData;
    targetNode.classData = &targetData;
    rootNode.listCountB = 6;
    rootNode.listB = rootChildren;
    targetNode.flags = 0x04;
    const zMat4x3 rootMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                0.0f, 0.0f, 1.0f, 10.0f, 20.0f, 30.0f};
    const zMat4x3 targetMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                  0.0f, 0.0f, 1.0f, 2.0f, 3.0f, 4.0f};
    SetObjectLocalMatrix(&rootData, rootMatrix);
    SetObjectLocalMatrix(&targetData, targetMatrix);

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;
    playerState.rootNode = &rootNode;

    g_PlayerSaveStateListHead = &saveState;
    g_PlayerSaveStateListTail = &saveState;
    g_PlayerSaveStateCount = 1;
    g_PlayerMasterCommonDataHead = &commonData;
    g_PlayerMasterCommonDataTail = &commonData;
    g_PlayerMasterCommonDataListAux = 0;
    g_PlayerMasterCommonDataCount = 1;
    g_Player_NextOrdinal = 1;
    g_GameStateOrMapTable = 0;
    g_Time_AccumulatedTimeSec = 42.0f;
    ZOPT_GAME_CONTROL_OPTIONS = &gameControlOptions;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_zUtil_ZbdManager = 0;
    g_AINetListHead = 0;
    g_AINetListTail = 0;
    g_zEffectAnim_EntryList = 0;
    g_zEffectAnim_EntryCount = 0;

    Player::InitStateFromNameAndMasterCommonData(&saveState, "tank",
                                                 "tank_common");

    int result = 0;
    if (playerState.masterCommonData != &commonData || playerState.playerOrdinal != 1 ||
        g_Player_NextOrdinal != 2 ||
        g_GameStateOrMapTable != (zInput_GameStateOrMapTablePartial *)(&saveState)) {
        result = 1;
    } else if (!Vec3Equals(playerState.worldPos, zVec3_Make(10.0f, 20.0f, 30.0f)) ||
               playerState.pitchPoseCache != 0.0f || playerState.yawPoseCache != 0.0f ||
               playerState.rollPoseCache != 0.0f ||
               !Vec3Equals(playerState.steerBasisRaw, zVec3_Make(0.0f, 0.0f, -1.0f)) ||
               !Vec3Equals(playerState.steerBasisNorm, zVec3_Make(0.0f, 0.0f, -1.0f)) ||
               !Vec3Equals(playerState.cameraDirFlat, zVec3_Make(0.0f, 0.0f, -1.0f))) {
        result = 2;
    } else if (playerState.lifecycleState != 4 ||
               playerState.thirdPersonSideOffset != 1.0f ||
               playerState.thirdPersonBaseYOffset != 2.0f ||
               playerState.cameraDistance != 3.0f ||
               playerState.cameraConfigParam0 != 4.0f ||
               playerState.cameraConfigParam5 != 9.0f ||
               playerState.cameraYOffset != 11.0f ||
               !Vec3Equals(playerState.cameraState2TargetOffset,
                           zVec3_Make(0.0f, 150.0f, 0.0f)) ||
               !Vec3Equals(playerState.altGunAimOrigin,
                           zVec3_Make(0.0f, 0.0f, -1.0f)) ||
               playerState.activeAltBankIndex != 1 || playerState.autoTurnActive != 0 ||
               playerState.cameraTransitionTimer != 0 ||
               playerState.cameraTransitionBlend != 1.0f) {
        result = 3;
    } else if (!Vec3Equals(playerState.fxOffsetLocal, zVec3_Make(2.0f, 3.0f, 4.0f)) ||
               !Vec3Equals(playerState.fxOffsetWorld, zVec3_Make(12.0f, 23.0f, 34.0f)) ||
               (targetNode.flags & 0x04) != 0 || playerState.bodyNode != &bodyNode ||
               playerState.turretNode != &turretNode ||
               playerState.doorLeftNode != &doorLeftNode ||
               playerState.doorRightNode != &doorRightNode ||
               playerState.modeVariantNode != &shadowNode || playerState.gunNode != 0 ||
               playerState.statusMeterValue != 250.0f ||
               playerState.statusMeterScaled != 1.0f ||
               playerState.damageProtectionActive != 0 ||
               playerState.queuedFixedDamageFlag != 0 ||
               playerState.recentHitValid != 0 ||
               playerState.recentHitLightHandle != 0 ||
               playerState.nanitePanelLevel != 0) {
        result = 4;
    }

    AINetNode aiNode = {};
    aiNode.position = zVec3_Make(9.0f, 20.0f, 31.0f);
    AINet aiNet = {};
    aiNet.netId = 77;
    aiNet.aiType = AINET_TYPE_FI;
    aiNet.activateRadius = 5.0f;
    aiNet.attackRadius = 6.0f;
    aiNet.attackDwell = 7.0f;
    aiNet.notPursuitDwell = 8.0f;
    aiNet.returnRange = 9.0f;
    aiNet.hideTime0 = 10.0f;
    aiNet.hideTime1 = 11.0f;
    aiNet.attackStrategy = AINET_STRAT_ZIG;
    aiNet.nodeListHead = &aiNode;

    zClass_NodePartial healthyNode = {};
    std::strcpy(healthyNode.name, "healthy");
    healthyNode.flags = 0x08;
    zClass_NodePartial *aiRootChildren[] = {&healthyNode};
    rootNode.listCountB = 1;
    rootNode.listB = aiRootChildren;
    targetNode.flags = 0x04;
    playerState = {};
    saveState = {};
    saveState.playerState = &playerState;
    playerState.rootNode = &rootNode;
    playerState.aiNetId = 77;
    g_PlayerSaveStateListHead = &saveState;
    g_PlayerSaveStateListTail = &saveState;
    g_Player_NextOrdinal = 2;
    g_GameStateOrMapTable = 0;
    g_Time_AccumulatedTimeSec = 42.0f;
    ZOPT_GAME_CONTROL_OPTIONS = &gameControlOptions;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_zUtil_ZbdManager = 0;
    g_AINetListHead = &aiNet;
    g_AINetListTail = &aiNet;

    if (result == 0) {
        Player::InitStateFromNameAndMasterCommonData(&saveState, "net_tank",
                                                     "tank_common");

        if (playerState.lifecycleState != 2 || playerState.aiNet != &aiNet ||
            playerState.aiTopLevelState != 3 ||
            playerState.aiCurrentSteeringSubstate != AINET_STRAT_ZIG ||
            playerState.aiHideTime0 != 10.0f || playerState.aiHideTime1 != 11.0f ||
            playerState.aiCurrentPathNode != &aiNode ||
            playerState.aiHomePathNode != &aiNode ||
            playerState.aiActivationRadiusSq != 25.0f ||
            playerState.aiAttackRadiusSq != 36.0f ||
            playerState.aiMode2AttackDwell != 7.0f ||
            playerState.aiNotPursuitDwell != 8.0f ||
            playerState.aiRestoreDistanceSq != 81.0f ||
            saveState.aiPeerRingNext != &saveState ||
            playerState.aiStateUntilTime != 52.0f ||
            playerState.aiStateStartTime != 52.0f ||
            (healthyNode.flags & 0x08) != 0) {
            result = 5;
        }
    }

    g_PlayerSaveStateListHead = oldSaveHead;
    g_PlayerSaveStateListTail = oldSaveTail;
    g_PlayerSaveStateCount = oldSaveCount;
    g_PlayerMasterCommonDataHead = oldCommonHead;
    g_PlayerMasterCommonDataTail = oldCommonTail;
    g_PlayerMasterCommonDataListAux = oldCommonAux;
    g_PlayerMasterCommonDataCount = oldCommonCount;
    g_Player_NextOrdinal = oldNextOrdinal;
    g_GameStateOrMapTable = oldGameState;
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    ZOPT_GAME_CONTROL_OPTIONS = oldGameControlOptions;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_zUtil_ZbdManager = oldZbdManager;
    g_AINetListHead = oldAiHead;
    g_AINetListTail = oldAiTail;
    g_zEffectAnim_EntryList = oldEffectEntries;
    g_zEffectAnim_EntryCount = oldEffectCount;
    return result;
}

extern "C" int player_bind_modal_state_from_master_modal_data_smoke(void) {
    zUtil_SaveGameState *const oldSaveHead = g_PlayerSaveStateListHead;
    zUtil_SaveGameState *const oldSaveTail = g_PlayerSaveStateListTail;
    const int oldSaveAux = g_PlayerSaveStateListAux;
    const int oldSaveCount = g_PlayerSaveStateCount;
    PlayerMasterModalData *const oldModalHead = g_PlayerMasterModalDataHead;
    PlayerMasterModalData *const oldModalTail = g_PlayerMasterModalDataTail;
    const int oldModalAux = g_PlayerMasterModalDataListAux;
    const int oldModalCount = g_PlayerMasterModalDataCount;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData nonMatchingModalData = {};
    PlayerMasterModalData modalData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial cacheNodes[12] = {};
    zClass_NodePartial supportNodes[4] = {};
    zClass_Object3DDataPartial supportData[4] = {};
    zClass_NodePartial collisionNodes[12] = {};
    zClass_Object3DDataPartial collisionData[12] = {};
    zClass_NodePartial *children[28] = {};

    std::strcpy(commonData.vehicleName, "tank_common");
    std::strcpy(nonMatchingModalData.modalName, "tank_common");
    std::strcpy(nonMatchingModalData.modeName, "hover");
    nonMatchingModalData.next = &modalData;
    std::strcpy(modalData.modalName, "tank_common");
    std::strcpy(modalData.modeName, "track");

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    playerState.masterCommonData = &commonData;
    playerState.rootNode = &rootNode;
    std::strcpy(rootNode.name, "vehicle");
    rootNode.listCountB = 28;
    rootNode.listB = children;

    const char *const cacheNames[12] = {"right_morphs", "left_morphs", "chassis",
                                        "rtracks",      "ltracks",     "props",
                                        "caustic1",     "wake",        "splash_l",
                                        "splash_r",     "dust_l",      "dust_r"};
    for (int i = 0; i < 12; ++i) {
        std::strcpy(cacheNodes[i].name, cacheNames[i]);
        children[i] = &cacheNodes[i];
    }

    for (int i = 0; i < 4; ++i) {
        children[12 + i] = &supportNodes[i];
        std::sprintf(supportNodes[i].name, "support%02d", i);
        supportNodes[i].classId = 5;
        supportNodes[i].classData = &supportData[i];
        supportNodes[i].flags = 0x04;
        const float base = static_cast<float>(100 + i * 10);
        const zMat4x3 matrix = {1.0f,      0.0f,      0.0f, 0.0f, 1.0f, 0.0f,
                                0.0f,      0.0f,      1.0f, base + 1.0f,
                                base + 2.0f, base + 3.0f};
        SetObjectLocalMatrix(&supportData[i], matrix);
    }

    for (int i = 0; i < 12; ++i) {
        children[16 + i] = &collisionNodes[i];
        std::sprintf(collisionNodes[i].name, "collide%02d", i);
        collisionNodes[i].classId = 5;
        collisionNodes[i].classData = &collisionData[i];
        collisionNodes[i].flags = 0x04;
        const float base = static_cast<float>(i * 10);
        const zMat4x3 matrix = {1.0f,      0.0f,      0.0f, 0.0f, 1.0f, 0.0f,
                                0.0f,      0.0f,      1.0f, base + 1.0f,
                                base + 2.0f, base + 3.0f};
        SetObjectLocalMatrix(&collisionData[i], matrix);
    }

    g_PlayerSaveStateListHead = &saveState;
    g_PlayerSaveStateListTail = &saveState;
    g_PlayerSaveStateListAux = 1;
    g_PlayerSaveStateCount = 1;
    g_PlayerMasterModalDataHead = &nonMatchingModalData;
    g_PlayerMasterModalDataTail = &modalData;
    g_PlayerMasterModalDataListAux = 1;
    g_PlayerMasterModalDataCount = 2;

    Player::BindModalStateFromMasterModalData(&saveState, &modalState, "tank_object",
                                              "track");

    int result = 0;
    if (modalState.masterModalData != &modalData ||
        modalState.nodeRightMorphs != &cacheNodes[0] ||
        modalState.nodeLeftMorphs != &cacheNodes[1] ||
        modalState.modalNode != &cacheNodes[2] ||
        modalState.nodeRTracks != &cacheNodes[3] ||
        modalState.nodeLTracks != &cacheNodes[4] || modalState.nodeProps != &cacheNodes[5] ||
        modalState.nodeCaustic1 != &cacheNodes[6] ||
        modalState.nodeWake != &cacheNodes[7] ||
        modalState.nodeSplashL != &cacheNodes[8] ||
        modalState.nodeSplashR != &cacheNodes[9] ||
        modalState.nodeDustL != &cacheNodes[10] ||
        modalState.nodeDustR != &cacheNodes[11]) {
        result = 1;
    }

    if (result == 0 &&
        (modalState.modalStateCode != 4 || modalState.chassisPitchFilterState != 0.0f ||
         modalState.chassisRollFilterState != 0.0f)) {
        result = 2;
    }

    for (int i = 0; result == 0 && i < 4; ++i) {
        const float base = static_cast<float>(100 + i * 10);
        if (!Vec3Equals(modalData.probePoints[15 + i],
                        zVec3_Make(base + 1.0f, base + 2.0f, base + 3.0f)) ||
            (supportNodes[i].flags & 0x04) != 0) {
            result = 3;
        }
    }

    const int expectedOrder[12] = {0, 1, 2, 6, 7, 8, 3, 4, 5, 9, 10, 11};
    if (result == 0 && modalData.probePointCount != 12) {
        result = 4;
    }
    for (int i = 0; result == 0 && i < 12; ++i) {
        const float base = static_cast<float>(expectedOrder[i] * 10);
        if (!Vec3Equals(modalData.probePoints[i],
                        zVec3_Make(base + 1.0f, base + 2.0f, base + 3.0f)) ||
            (collisionNodes[expectedOrder[i]].flags & 0x04) != 0) {
            result = 5;
        }
    }

    g_PlayerSaveStateListHead = oldSaveHead;
    g_PlayerSaveStateListTail = oldSaveTail;
    g_PlayerSaveStateListAux = oldSaveAux;
    g_PlayerSaveStateCount = oldSaveCount;
    g_PlayerMasterModalDataHead = oldModalHead;
    g_PlayerMasterModalDataTail = oldModalTail;
    g_PlayerMasterModalDataListAux = oldModalAux;
    g_PlayerMasterModalDataCount = oldModalCount;
    return result;
}

extern "C" int player_build_support_points_from_model_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterModalData modalData = {};
    PlayerModalState primaryModalState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &primaryModalState;
    primaryModalState.masterModalData = &modalData;

    zClass_NodePartial modelNode = {};
    zClass_NodePartial supportNodes[4] = {};
    zClass_Object3DDataPartial supportData[4] = {};
    zClass_NodePartial *children[4] = {
        &supportNodes[0], &supportNodes[1], &supportNodes[2], &supportNodes[3]};
    modelNode.listCountB = 4;
    modelNode.listB = children;
    std::strcpy(modelNode.name, "vehicle");

    for (int i = 0; i < 4; ++i) {
        std::sprintf(supportNodes[i].name, "support%02d", i);
        supportNodes[i].classId = 5;
        supportNodes[i].classData = &supportData[i];
        supportNodes[i].flags = 0x04;
        const float base = static_cast<float>(i * 10);
        const zMat4x3 matrix = {1.0f,      0.0f,      0.0f, 0.0f, 1.0f, 0.0f,
                                0.0f,      0.0f,      1.0f, base + 1.0f,
                                base + 2.0f, base + 3.0f};
        SetObjectLocalMatrix(&supportData[i], matrix);
    }

    const int ok = Player::BuildSupportPointsFromModel(&saveState, &modelNode);
    if (ok != 1) {
        return 1;
    }

    for (int i = 0; i < 4; ++i) {
        const float base = static_cast<float>(i * 10);
        if (!Vec3Equals(modalData.probePoints[15 + i],
                        zVec3_Make(base + 1.0f, base + 2.0f, base + 3.0f)) ||
            (supportNodes[i].flags & 0x04) != 0) {
            return 2;
        }
    }

    modalData.probePoints[17] = zVec3_Make(90.0f, 91.0f, 92.0f);
    modelNode.listCountB = 2;
    supportNodes[0].flags = 0x04;
    supportNodes[1].flags = 0x04;
    const int missing = Player::BuildSupportPointsFromModel(&saveState, &modelNode);
    if (missing != 0 || (supportNodes[0].flags & 0x04) != 0 ||
        (supportNodes[1].flags & 0x04) != 0 ||
        !Vec3Equals(modalData.probePoints[17], zVec3_Make(90.0f, 91.0f, 92.0f))) {
        return 3;
    }

    return 0;
}

extern "C" int player_build_collision_points_from_model_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterModalData modalData = {};
    PlayerModalState primaryModalState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &primaryModalState;
    primaryModalState.masterModalData = &modalData;

    zClass_NodePartial modelNode = {};
    zClass_NodePartial collisionNodes[12] = {};
    zClass_Object3DDataPartial collisionData[12] = {};
    zClass_NodePartial *children[12] = {};
    modelNode.listCountB = 12;
    modelNode.listB = children;
    std::strcpy(modelNode.name, "vehicle");

    for (int i = 0; i < 12; ++i) {
        children[i] = &collisionNodes[i];
        std::sprintf(collisionNodes[i].name, "collide%02d", i);
        collisionNodes[i].classId = 5;
        collisionNodes[i].classData = &collisionData[i];
        collisionNodes[i].flags = 0x04;
        const float base = static_cast<float>(i * 10);
        const zMat4x3 matrix = {1.0f,      0.0f,      0.0f, 0.0f, 1.0f, 0.0f,
                                0.0f,      0.0f,      1.0f, base + 1.0f,
                                base + 2.0f, base + 3.0f};
        SetObjectLocalMatrix(&collisionData[i], matrix);
    }

    const int ok = Player::BuildCollisionPointsFromModel(&saveState, &modelNode);
    if (ok != 1 || modalData.probePointCount != 12) {
        return 1;
    }

    const int expectedOrder[12] = {0, 1, 2, 6, 7, 8, 3, 4, 5, 9, 10, 11};
    for (int i = 0; i < 12; ++i) {
        const float base = static_cast<float>(expectedOrder[i] * 10);
        if (!Vec3Equals(modalData.probePoints[i],
                        zVec3_Make(base + 1.0f, base + 2.0f, base + 3.0f)) ||
            (collisionNodes[expectedOrder[i]].flags & 0x04) != 0) {
            return 2;
        }
    }

    modalData.probePoints[0] = zVec3_Make(90.0f, 91.0f, 92.0f);
    modalData.probePointCount = 99;
    modelNode.listCountB = 2;
    collisionNodes[0].flags = 0x04;
    collisionNodes[1].flags = 0x04;
    const int missing = Player::BuildCollisionPointsFromModel(&saveState, &modelNode);
    if (missing != 0 || (collisionNodes[0].flags & 0x04) != 0 ||
        (collisionNodes[1].flags & 0x04) != 0 || modalData.probePointCount != 99 ||
        !Vec3Equals(modalData.probePoints[0], zVec3_Make(90.0f, 91.0f, 92.0f))) {
        return 3;
    }

    return 0;
}

extern "C" int player_sample_ground_and_align_root_to_surface_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;

    static std::int32_t matrixFlags[2];
    static float *matrixSlots[2];
    static zMat4x3 identityMatrix;
    identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixFlags[0] = 1;
    matrixSlots[0] = reinterpret_cast<float *>(&identityMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchA = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    zClass_Object3DDataPartial rootData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial emptyWorld = {};
    zClass_WorldDataPartial emptyWorldData = {};

    saveState.playerState = &playerState;
    playerState.rootNode = &rootNode;
    playerState.worldPos = {0.25f, 10.0f, 0.25f};
    playerState.restartYawRad = 0.25f;
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 7;
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    rootNode.flags = 0x08;
    rootNode.nodeType = 7;
    emptyWorld.classData = &emptyWorldData;
    g_Player_RuntimeDiScene = &emptyWorld;
    g_Variant_CurrentTag.count = 3;
    g_Variant_CurrentTag.tags[0] = 1;
    g_Variant_CurrentTag.tags[1] = 2;
    g_Variant_CurrentTag.tags[2] = 3;

    Player::SampleGroundAndAlignRootToSurface(&saveState, 1);
    if (rootNode.nodeType != 0xff || (rootNode.flags & 0x08) == 0 ||
        playerState.variantTag.count != 0 || playerState.variantTag.tags[0] != 0xff ||
        g_Variant_CurrentTag.count != 0 || g_Variant_CurrentTag.tags[0] != 0xff ||
        rootData.rotation.x != 0.0f || rootData.rotation.y != 0.0f ||
        rootData.rotation.z != 0.0f) {
        g_Player_RuntimeDiScene = oldRuntimeScene;
        g_Variant_CurrentTag = oldVariantCurrent;
        return 1;
    }

    zVec3 vertices[3] = {{0.0f, 12.0f, 0.0f}, {0.0f, 12.0f, 1.0f},
                         {1.0f, 12.0f, 0.0f}};
    std::int32_t indices[3] = {0, 1, 2};
    zModel_MaterialPartial material = {};
    zDiEntryPartial diEntry = {};
    diEntry.flagsAndIndexCount = 3;
    diEntry.vertexIndices = indices;
    diEntry.material = &material;
    diEntry.variantTagInitialized = 0;
    diEntry.variantTag = 0xff;
    zDiPartial di = {};
    di.entryCount = 1;
    di.vertCount = 3;
    di.entries = &diEntry;
    di.verts = vertices;

    zClass_Object3DDataPartial groundData = {};
    groundData.flags = 8;
    zClass_NodePartial groundNode = {};
    groundNode.flags = 0x11c;
    groundNode.nodeType = 0x37;
    groundNode.classId = 5;
    groundNode.classData = &groundData;
    groundNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&di);
    groundNode.cachedBounds[0] = -10.0f;
    groundNode.cachedBounds[1] = 0.0f;
    groundNode.cachedBounds[2] = -10.0f;
    groundNode.cachedBounds[3] = 10.0f;
    groundNode.cachedBounds[4] = 20.0f;
    groundNode.cachedBounds[5] = 10.0f;
    zClass_NodePartial *groundParents[1] = {};
    zClass_NodePartial *worldChildren[1] = {&groundNode};
    zWorldAreaPartial area = {};
    area.childCount = 1;
    area.childList = worldChildren;
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
    world.classId = 2;
    world.classData = &worldData;
    world.listCountB = 1;
    world.listB = worldChildren;
    groundParents[0] = &world;
    groundNode.listCountA = 1;
    groundNode.listA = groundParents;

    g_Player_RuntimeDiScene = &world;
    rootNode.nodeType = 7;
    rootNode.flags = 0x08;
    rootData.rotation = {9.0f, 8.0f, 7.0f};
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 7;
    playerState.steerBasisNorm = {0.0f, 0.0f, 1.0f};
    playerState.steerBasisRaw = {3.0f, 4.0f, 5.0f};
    playerState.restartYawRad = 0.0f;
    playerState.vehiclePitchRad = 6.0f;
    playerState.vehicleRollRad = 7.0f;

    Player::SampleGroundAndAlignRootToSurface(&saveState, 1);

    int hitFailure = 0;
    if (rootNode.nodeType == 0xff) {
        hitFailure = 12;
    } else if (rootNode.nodeType == 0) {
        hitFailure = 13;
    } else if (rootNode.nodeType != 0x37) {
        hitFailure = 2;
    } else if ((rootNode.flags & 0x08) == 0) {
        hitFailure = 3;
    } else if (playerState.variantTag.count != 0) {
        hitFailure = 4;
    } else if (!FloatNear(playerState.steerBasisRef.x, 0.0f)) {
        hitFailure = 5;
    } else if (!FloatNear(playerState.steerBasisRef.z, 0.0f)) {
        hitFailure = 6;
    } else if (!FloatNear(playerState.vehiclePitchRad, 0.0f)) {
        hitFailure = 7;
    } else if (!FloatNear(playerState.vehicleRollRad, 0.0f)) {
        hitFailure = 8;
    } else if (!FloatNear(rootData.rotation.x, 0.0f)) {
        hitFailure = 9;
    } else if (!FloatNear(rootData.rotation.y, 0.0f)) {
        hitFailure = 10;
    } else if (!FloatNear(rootData.rotation.z, 0.0f)) {
        hitFailure = 11;
    }

    rootData.rotation = {3.0f, 4.0f, 5.0f};
    rootNode.nodeType = 7;
    Player::SampleGroundAndAlignRootToSurface(&saveState, 0);
    const bool noRotationOk = rootNode.nodeType == 0x37 &&
                              rootData.rotation.x == 3.0f &&
                              rootData.rotation.y == 4.0f &&
                              rootData.rotation.z == 5.0f;

    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Variant_CurrentTag = oldVariantCurrent;
    if (hitFailure != 0) {
        return hitFailure;
    }
    return noRotationOk ? 0 : 3;
}

extern "C" int player_init_spawn_state_from_primary_modal_data_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const float oldNominalGravity = g_Player_NominalGravity;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    zClass_Object3DDataPartial rootData = {};
    zClass_NodePartial rootNode = {};
    zClass_WorldDataPartial worldData = {};
    zClass_NodePartial worldNode = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    playerState.rootNode = &rootNode;
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    rootNode.flags = 0x08;
    worldNode.classData = &worldData;
    g_Player_RuntimeDiScene = &worldNode;
    g_Player_NominalGravity = 19.5f;

    playerState.spawnStateInitialized = 7;
    playerState.primaryGunGateUntilTime = 8.0f;
    playerState.gravityAccel = 1.0f;
    playerState.primaryFireSlotIndex = 3;
    playerState.altFireSlotIndex = 4;
    playerState.worldPos = {10.0f, 20.0f, 30.0f};
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 9;
    modalData.probePointCount = 3;
    modalData.probePoints[0] = {1.0f, 2.0f, 3.0f};
    modalData.probePoints[1] = {-4.0f, 5.0f, -6.0f};
    modalData.probePoints[2] = {7.0f, -8.0f, 9.0f};

    Player::InitSpawnStateFromPrimaryModalData(&saveState);

    const bool ok = playerState.spawnStateInitialized == 0 &&
                    playerState.primaryGunGateUntilTime == 0.0f &&
                    playerState.gravityAccel == 19.5f &&
                    playerState.primaryFireSlotIndex == 0 &&
                    playerState.altFireSlotIndex == 0 &&
                    Vec3Equals(playerState.rootProbeWorldByIndex[0],
                               zVec3_Make(11.0f, 22.0f, 33.0f)) &&
                    Vec3Equals(playerState.rootProbeWorldByIndex[1],
                               zVec3_Make(6.0f, 25.0f, 24.0f)) &&
                    Vec3Equals(playerState.rootProbeWorldByIndex[2],
                               zVec3_Make(17.0f, 12.0f, 39.0f)) &&
                    rootNode.nodeType == 0xff && (rootNode.flags & 0x08) != 0 &&
                    playerState.variantTag.count == 0 &&
                    playerState.variantTag.tags[0] == 0xff;

    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_Player_NominalGravity = oldNominalGravity;
    return ok ? 0 : 1;
}

extern "C" int player_create_from_names_at_pose_smoke(void) {
    zUtil_SaveGameState *const oldSaveHead = g_PlayerSaveStateListHead;
    zUtil_SaveGameState *const oldSaveTail = g_PlayerSaveStateListTail;
    const int oldSaveAux = g_PlayerSaveStateListAux;
    const int oldSaveCount = g_PlayerSaveStateCount;
    PlayerMasterCommonData *const oldCommonHead = g_PlayerMasterCommonDataHead;
    PlayerMasterCommonData *const oldCommonTail = g_PlayerMasterCommonDataTail;
    const int oldCommonAux = g_PlayerMasterCommonDataListAux;
    const int oldCommonCount = g_PlayerMasterCommonDataCount;
    PlayerMasterModalData *const oldModalHead = g_PlayerMasterModalDataHead;
    PlayerMasterModalData *const oldModalTail = g_PlayerMasterModalDataTail;
    const int oldModalAux = g_PlayerMasterModalDataListAux;
    const int oldModalCount = g_PlayerMasterModalDataCount;
    zClass_TypeListLink *const oldType6Head = zClass_TypeList::Head(6);
    zClass_TypeListLink *const oldType6Tail = zClass_TypeList::Tail(6);
    zClass_NodePartial *const oldRuntimeDiScene = g_Player_RuntimeDiScene;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    const int oldNextOrdinal = g_Player_NextOrdinal;
    const int oldMissionStat1 = g_HudSensorTracker.missionStat1;
    const float oldNominalGravity = g_Player_NominalGravity;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    int *const oldGameControlOptions = ZOPT_GAME_CONTROL_OPTIONS;
    zEffectAnimEntry *const oldEffectEntries = g_zEffectAnim_EntryList;
    const short oldEffectCount = g_zEffectAnim_EntryCount;
    zZbdManager *const oldZbdManager = g_zUtil_ZbdManager;
    AINet *const oldAiHead = g_AINetListHead;
    AINet *const oldAiTail = g_AINetListTail;
    int *const oldMatrixIdentityFlagSlot = zMath::g_currentMatrixIdentityFlagSlot;
    float **const oldMatrixPtrSlot = zMath::g_currentMatrixPtrSlot;
    zVec3 *const oldSharedScratchA = g_zModel_SharedVec3ScratchA;
    zVec3 *const oldSharedScratchB = g_zModel_SharedVec3ScratchB;

    int networkEnabled = 1;
    int gameControlOptions = 0;
    static std::int32_t matrixFlags[2];
    static float *matrixSlots[2];
    static zMat4x3 identityMatrix;
    identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixFlags[0] = 1;
    matrixSlots[0] = (float *)&identityMatrix;
    zClass_NodePartial world = {};
    zClass_WorldDataPartial worldData = {};
    zClass_NodeFreeListSlot rootSlot = {};
    zClass_NodePartial &rootNode = rootSlot.node;
    zClass_Object3DDataPartial rootData = {};
    zClass_TypeListLink rootLink = {&rootNode, 0, 0, 0};
    PlayerMasterCommonData commonData = {};
    PlayerMasterModalData modalData = {};

    world.classId = 2;
    world.classData = &worldData;
    std::strcpy(rootNode.name, "net_tank");
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    rootData.localMatrix[0] = 1.0f;
    rootData.localMatrix[4] = 1.0f;
    rootData.localMatrix[8] = 1.0f;

    std::strcpy(commonData.vehicleName, "tank_common");
    std::strcpy(commonData.modalNames[0], "track");
    std::strcpy(commonData.startAnimsName, "startup");
    commonData.modalCount = 1;
    commonData.maxHealth = 150.0f;
    commonData.cambackDist0 = 12.0f;
    commonData.cambackSide0 = 1.0f;
    commonData.cambackBase0 = 2.0f;
    std::strcpy(modalData.modalName, "tank_common");
    std::strcpy(modalData.modeName, "track");
    modalData.probePointCount = 1;
    modalData.platformPointCount = 1;
    modalData.probePoints[0] = zVec3_Make(0.5f, 1.0f, 1.5f);

    g_PlayerSaveStateListHead = 0;
    g_PlayerSaveStateListTail = 0;
    g_PlayerSaveStateListAux = 0;
    g_PlayerSaveStateCount = 0;
    g_PlayerMasterCommonDataHead = &commonData;
    g_PlayerMasterCommonDataTail = &commonData;
    g_PlayerMasterCommonDataListAux = 1;
    g_PlayerMasterCommonDataCount = 1;
    g_PlayerMasterModalDataHead = &modalData;
    g_PlayerMasterModalDataTail = &modalData;
    g_PlayerMasterModalDataListAux = 1;
    g_PlayerMasterModalDataCount = 1;
    zClass_TypeList::Head(6) = &rootLink;
    zClass_TypeList::Tail(6) = &rootLink;
    g_Player_RuntimeDiScene = &world;
    g_GameStateOrMapTable = 0;
    g_Player_NextOrdinal = 1;
    g_HudSensorTracker.missionStat1 = oldMissionStat1;
    g_Player_NominalGravity = 19.5f;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    ZOPT_GAME_CONTROL_OPTIONS = &gameControlOptions;
    g_zEffectAnim_EntryList = 0;
    g_zEffectAnim_EntryCount = 0;
    g_zUtil_ZbdManager = 0;
    g_AINetListHead = 0;
    g_AINetListTail = 0;
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchA = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    const zVec3 spawnPos = {11.0f, 22.0f, 33.0f};
    const int createResult =
        Player::CreateFromNamesAtPose(&spawnPos, 77, 90.0f, "tank_common", "net_tank");

    zUtil_SaveGameState *const createdSave = g_PlayerSaveStateListHead;
    zUtil_PlayerStateStorage *const playerState =
        createdSave != 0 ? createdSave->playerState : 0;
    PlayerModalState *const modalState =
        createdSave != 0 ? createdSave->primaryModalState : 0;
    zUtil_SaveGameState *wrapperSave = 0;
    PlayerModalState *wrapperModalState = 0;

    int result = 0;
    if (createResult != 1 ||
        createdSave == 0 ||
        playerState == 0 ||
        g_PlayerSaveStateListTail != createdSave ||
        g_PlayerSaveStateCount != 1) {
        result = 1;
    } else if (playerState->rootNode != &rootNode ||
               playerState->aiNetId != 77 ||
               !FloatNear(rootData.localMatrix[9], 11.0f) ||
               !FloatNear(rootData.localMatrix[10], 22.0f) ||
               !FloatNear(rootData.localMatrix[11], 33.0f) ||
               !FloatNear(playerState->restartYawRad, 1.57079637f)) {
        result = 2;
    } else if (world.listCountB != 1 ||
               world.listB == 0 ||
               world.listB[0] != &rootNode ||
               rootNode.listCountA != 1 ||
               rootNode.listA == 0 ||
               rootNode.listA[0] != &world) {
        result = 3;
    } else if (playerState->masterCommonData != &commonData ||
               playerState->statusMeterValue != 150.0f ||
               modalState == 0 ||
               modalState->masterModalData != &modalData ||
               createdSave->primaryModalState != modalState) {
        result = 4;
    } else if (playerState->gravityAccel != 19.5f ||
               !Vec3Equals(
                   playerState->rootProbeWorldByIndex[0],
                   zVec3_Make(11.5f, 23.0f, 34.5f)
               ) ||
               g_GameStateOrMapTable != (zInput_GameStateOrMapTablePartial *)createdSave ||
               g_Player_NextOrdinal != 2 ||
               g_HudSensorTracker.missionStat1 != oldMissionStat1) {
        result = 5;
    } else {
        const zVec3 wrapperSpawnPos = {44.0f, 55.0f, 66.0f};
        wrapperSave = Player::CreateFromNamesAtPoseGetState(
            &wrapperSpawnPos,
            "tank_common",
            180.0f,
            "net_tank"
        );
        zUtil_PlayerStateStorage *const wrapperState =
            wrapperSave != 0 ? wrapperSave->playerState : 0;
        wrapperModalState = wrapperSave != 0 ? wrapperSave->primaryModalState : 0;
        if (wrapperSave == 0 ||
            wrapperSave != g_PlayerSaveStateListTail ||
            createdSave->next != wrapperSave ||
            g_PlayerSaveStateCount != 2 ||
            wrapperState == 0 ||
            wrapperState->aiNetId != 0 ||
            !FloatNear(wrapperState->restartYawRad, 3.14159274f) ||
            !FloatNear(rootData.localMatrix[9], 44.0f) ||
            !FloatNear(rootData.localMatrix[10], 55.0f) ||
            !FloatNear(rootData.localMatrix[11], 66.0f) ||
            g_Player_NextOrdinal != 3) {
            result = 6;
        }
    }

    if (wrapperSave != 0) {
        if (wrapperModalState != 0) {
            std::free(wrapperModalState);
        }
        std::free(wrapperSave->playerState);
        ::operator delete(wrapperSave);
    }
    if (createdSave != 0) {
        if (modalState != 0) {
            std::free(modalState);
        }
        std::free(createdSave->playerState);
        ::operator delete(createdSave);
    }
    std::free(world.listB);
    std::free(rootNode.listA);

    g_PlayerSaveStateListHead = oldSaveHead;
    g_PlayerSaveStateListTail = oldSaveTail;
    g_PlayerSaveStateListAux = oldSaveAux;
    g_PlayerSaveStateCount = oldSaveCount;
    g_PlayerMasterCommonDataHead = oldCommonHead;
    g_PlayerMasterCommonDataTail = oldCommonTail;
    g_PlayerMasterCommonDataListAux = oldCommonAux;
    g_PlayerMasterCommonDataCount = oldCommonCount;
    g_PlayerMasterModalDataHead = oldModalHead;
    g_PlayerMasterModalDataTail = oldModalTail;
    g_PlayerMasterModalDataListAux = oldModalAux;
    g_PlayerMasterModalDataCount = oldModalCount;
    zClass_TypeList::Head(6) = oldType6Head;
    zClass_TypeList::Tail(6) = oldType6Tail;
    g_Player_RuntimeDiScene = oldRuntimeDiScene;
    g_GameStateOrMapTable = oldGameState;
    g_Player_NextOrdinal = oldNextOrdinal;
    g_HudSensorTracker.missionStat1 = oldMissionStat1;
    g_Player_NominalGravity = oldNominalGravity;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    ZOPT_GAME_CONTROL_OPTIONS = oldGameControlOptions;
    g_zEffectAnim_EntryList = oldEffectEntries;
    g_zEffectAnim_EntryCount = oldEffectCount;
    g_zUtil_ZbdManager = oldZbdManager;
    g_AINetListHead = oldAiHead;
    g_AINetListTail = oldAiTail;
    zMath::g_currentMatrixIdentityFlagSlot = oldMatrixIdentityFlagSlot;
    zMath::g_currentMatrixPtrSlot = oldMatrixPtrSlot;
    g_zModel_SharedVec3ScratchA = oldSharedScratchA;
    g_zModel_SharedVec3ScratchB = oldSharedScratchB;
    return result;
}
