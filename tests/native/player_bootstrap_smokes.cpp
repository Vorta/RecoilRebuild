#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/GameNet.h"
#include "Battlesport/ainet.h"
#include "Battlesport/player.h"
#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zUtil/zZbd.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace {
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
} // namespace

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
