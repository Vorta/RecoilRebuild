#include "player.h"

#include "Battlesport/GameNet.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/zDi.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "HudSensorTracker.h"
#include "OptCatalog.h"

#include <stdlib.h>
#include <string.h>

extern "C" {
int g_Player_HudCounterValue = 0;
HudUiMgrSensorTrackList g_HudUiMgrSensor_TrackList = {0};
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

namespace Player {
namespace {
zVec3 TransformPointByMatrix(const zVec3 &point, const zMat4x3 &matrix) {
    zVec3 result = {0};
    result.x = point.x * matrix.xx + point.y * matrix.yx + point.z * matrix.zx + matrix.posX;
    result.y = point.x * matrix.xy + point.y * matrix.yy + point.z * matrix.zy + matrix.posY;
    result.z = point.x * matrix.xz + point.y * matrix.yz + point.z * matrix.zz + matrix.posZ;
    return result;
}
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

// Reimplements 0x403830: Player::AiDiscardNegativeBranchPathNodes (src/Battlesport/ainet.cpp)
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

// Reimplements 0x438b60: Player::FreeAltWeaponTrailRuntimeStates (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
FreeAltWeaponTrailRuntimeStates(zUtil_SaveGameState *saveState) {
    unsigned char *cursor = saveState->playerState->bytes + 0x740;
    for (int i = 0; i < 9; ++i) {
        void *const controllerATrail = *reinterpret_cast<void **>(cursor - 0x54);
        if (controllerATrail != 0) {
            OptCatalog::FreeTrailRuntimeStateStorage(controllerATrail);
        }

        void *const controllerBTrail = *reinterpret_cast<void **>(cursor);
        if (controllerBTrail != 0) {
            OptCatalog::FreeTrailRuntimeStateStorage(controllerBTrail);
        }

        cursor += 0xac;
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
        reinterpret_cast<zUtil_PlayerStateStorage *>(g_GameStateOrMapTable->playerState);

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
        reinterpret_cast<HudUiMgrSensorTrackNode *>(playerState->rootNode->callbackContext);
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
    reinterpret_cast<HudUiContainer *>(&g_zVideo_FxPass3ConfigLocal)
        ->RemoveChild(&g_Player_UnderwaterFxPass3Ui);
    reinterpret_cast<HudUiContainer *>(&g_zVideo_FxPass3ConfigLocal)
        ->RemoveChild(&g_Player_State7FxPass3Ui);
}
} // namespace Player
