#include "player.h"

#include "Battlesport/GameNet.h"
#include "Battlesport/ainet.h"
#include "Battlesport/pickup.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/include/zDi.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zTurret/zTurret.h"
#include "GameZRecoil/zUtil/zZbd.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "hud.h"
#include "HudSensorTracker.h"
#include "OptCatalog.h"

#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
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
int g_Player_LocalControlEnabled = 1;
int g_Player_RuntimeInputFlags = 0;
float g_Player_CameraZone = 0.5f;
float g_Player_CameraZoneInvRange = 2.0f;
float g_Player_MaxCamYawRate = 0.0f;
float g_Player_MousePushX = 0.0f;
float g_Player_MousePushY = 0.0f;
float g_Player_CameraElastic = 0.0f;
float g_Player_MaxCamTetherAngleRad = 0.0f;
float g_Player_FpCamElevationRate = 0.0f;
float g_Player_FpCamElevationMin = -1.0f;
float g_Player_FpCamElevationMax = 1.0f;
float g_Player_UnderwaterCamDistance = 0.0f;
float g_Player_UnderwaterCamHeight = 0.0f;
int g_Player_UnderwaterCamStepCount = 0;
float g_Player_UnderwaterCamFar = 0.0f;
int g_Player_UnderwaterCamPackedColor = 0;
float g_Player_UnderwaterCamAlpha = 0.0f;
float g_Player_GameplayInputStepScale = 0.03f;
float g_Player_CameraHeadingDotAbs = 1.0f;
float g_Player_CameraHeadingLerpBaseWhenFlagClear = 3.0f;
float g_Player_CameraHeadingLerpBaseWhenFlagSet = 2.0f;
zUtil_SaveGameState *g_PlayerSaveStateListHead = 0;
zUtil_SaveGameState *g_PlayerSaveStateListTail = 0;
int g_PlayerSaveStateListAux = 0;
int g_PlayerSaveStateCount = 0;
zUtil_SaveGameState *g_LocalPlayerSaveState = 0;
zUtil_SaveGameState *g_Player2SaveState = 0;
zUtil_SaveGameState *g_CurrentPlayerSaveState = 0;
zTag4Partial g_Player_LastValidCameraVariantTag = {0};
float g_Player_ThirdPersonCameraSideProbeOffsetScale = 1.0f;
int g_Player_CameraVariantUpdatedThisTick = 0;
int g_Player_RebuildCameraDirFlatFromCurrentTarget = 0;
zVec3 g_Player_AmphibBasisUpRef = {0.0f, 1.0f, 0.0f};
float g_Player_AmphibSteerBasisLerpRate = 3.0f;
int g_Player_NextOrdinal = 0;
int g_Player_AiMode2State1Finalized = 0;
float g_Player_AiMode2_PathFollowPitchInputScale = 0.0174499992f;
float g_Player_AiMode2_PathFollowPitchTurnGain = 5.69999981f;
float g_Player_AiMode2_SteeringPitchInputScale = 0.800000012f;
float g_Player_AiMode2_SteeringPitchTurnGain = 5.69999981f;
float g_Player_AiMode2_SteeringVerticalErrorScale = 0.100000001f;
float g_Player_AiMode2_OffsetTargetRotateCos15Deg = 0.965925813f;
float g_Player_AiMode2_OffsetTargetRotateSin15Deg = 0.258819044f;
float g_Player_TotalTimeSecScaled = 0.0f;
int g_PlayerPendingCheckpointNumber = 0;
float g_PlayerStatusMeterRatio = 0.0f;
float g_Player_NominalGravity = 0.0f;
float g_Player_WaterGravity = 0.0f;
float g_Player_QuicksandGravity = 0.0f;
float g_Player_QuicksandSinkRate = 0.0f;
float g_Player_LavaSinkRate = 0.0f;
float g_Player_MaxSlope = 0.0f;
float g_Player_CollisionContactResolveScale = 0.2f;
HudUiElement g_Player_UnderwaterFxPass3Ui = {0};
HudUiElement g_Player_State7FxPass3Ui = {0};
OptCatalogEntryDef *g_Player_MakeHotOptEntry = 0;
OptCatalogEntryDef *g_Player_MakeColdOptEntry = 0;
zEffectAnimEntry *g_Player_BftSplashAnimEntry = 0;
zEffectAnimEntry *g_Player_ActiveDebugScriptAsyncEntry = 0;
int g_Player_HorizonNodeFollowCameraEnabled = 0;
zClass_NodePartial *g_Player_HorizonNode = 0;
int g_PlayerPrevCameraState = 0;
int g_PlayerPrevSteeringMode = 0;
int g_Player_SavedSteeringMode = 0;
zClass_NodePartial *g_Player_CopterHealthyNode1 = 0;
zClass_NodePartial *g_Player_CopterHealthyNode2 = 0;
zClass_NodePartial *g_Player_CopterSndNode1 = 0;
zClass_NodePartial *g_Player_CopterSndNode2 = 0;
zSndSample *g_Player_CopterSndSample = 0;
int g_PlayerEnvProbeSampleCount = 0;
zVec3 g_PlayerEnvProbeWorldPoints[7] = {0};
int g_PlayerEnvProbe_AboveGroundFlags[10] = {0};
int g_PlayerEnvProbe_AboveGroundIndices[10] = {0};
int g_PlayerEnvProbe_AboveGroundCount = 0;
zEffectAnimEntry *g_PlayerRecentHitFxAnimEntry = 0;
zVec3 *g_Player_LocalFxOffsetWorldPtr = 0;
int g_Player_MissionInitFirstRunFlag = 1;
HudUiPanel g_Player_TopMsgPanel1 = {0};
HudUiPanel g_Player_TopMsgPanel2 = {0};
char g_Player_AivParentDir[0x100] = {0};
}

namespace {
float PlayerClamp01(float value) {
    if (value > 1.0f) {
        return 1.0f;
    }
    if (value < 0.0f) {
        return 0.0f;
    }
    return value;
}

float PlayerFloatFromBits(int bits) {
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

float PlayerFastSqrtEstimate(float value) {
    int bits = 0;
    memcpy(&bits, &value, sizeof(bits));
    bits = (bits >> 1) + 0x1fc00000;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

float PlayerDampingFromRate(float rate) {
    return PlayerFloatFromBits(static_cast<int>(-rate * g_Player_DeltaTime * 12102200.0f) +
                               0x3f800000);
}

float PlayerWrapSignedTwoPi(float angle) {
    const float twoPi = 6.28318548f;
    if (angle < -twoPi) {
        angle += twoPi;
    } else if (angle > twoPi) {
        angle -= twoPi;
    }
    return angle;
}

float PlayerClampSigned(float value, float limit) {
    if (value > limit) {
        return limit;
    }
    if (value < -limit) {
        return -limit;
    }
    return value;
}

zVec3 TransformWorldVectorToLocal(const zVec3 &vec, const zMat4x3 &matrix) {
    zVec3 out = {};
    out.x = vec.x * matrix.xx + vec.y * matrix.xy + vec.z * matrix.xz;
    out.y = vec.x * matrix.yx + vec.y * matrix.yy + vec.z * matrix.yz;
    out.z = vec.x * matrix.zx + vec.y * matrix.zy + vec.z * matrix.zz;
    return out;
}

zVec3 TransformLocalVectorToWorld(const zVec3 &vec, const zMat4x3 &matrix) {
    zVec3 out = {};
    out.x = vec.x * matrix.xx + vec.y * matrix.yx + vec.z * matrix.zx;
    out.y = vec.x * matrix.xy + vec.y * matrix.yy + vec.z * matrix.zy;
    out.z = vec.x * matrix.xz + vec.y * matrix.yz + vec.z * matrix.zz;
    return out;
}

enum PlayerMasterTypeId {
    kPlayerMasterTypeFly = 1,
    kPlayerMasterTypeSub = 2,
    kPlayerMasterTypeTrack = 3,
    kPlayerMasterTypeHover = 4,
    kPlayerMasterTypeAmphib = 5
};

const float kPlayerMasterTypeTrackCooldownSec = 1.0f;
const float kPlayerMasterTypeFlyCooldownSec = 5.0f;
const int kPlayerAiMode2TopSteering = 1;
const int kPlayerAiMode2SteerDirectTarget = 0;
const int kPlayerAiMode2SteerOffsetTarget = 1;
const int kPlayerAiMode2SteerDynamicOffsetTarget = 2;
const int kPlayerAiMode2SteerPathFollow = 3;
const int kPlayerAiMode2SteerTurnInPlace = 5;
const int kPlayerAiMode2SteerAutoTurn = 6;
const float kPlayerAiAltGunAttackForwardMin = 0.75f;
const float kPlayerAiAltGunStatusMinScale = 0.5f;
const int kPlayerAiTopPathFollow = 0;
const int kPlayerAiTopTurnTowardTarget = 2;
const int kPlayerAiTopTurnOnlyTowardTarget = 3;
const int kPlayerAiTopPathSteering = 4;
const int kPlayerAiTopAutoTurn = 5;
const int kPlayerLifecycleLocal = 1;
const int kPlayerLifecycleAi = 2;
const int kPlayerLifecycleRemote = 3;
const int kPlayerLifecycleInactive = 4;
const int kPlayerLifecycleDestroyed = 5;
const int kPlayerLifecycleState6Inactive = 6;
const int kPlayerNodeFlagNetworkBftCloneSource = 1 << 22;
const int kPlayerPerFrameGeneralFlag = 2;
const float kPlayerMinFrameDeltaSec = 0.00499999989f;
const float kPlayerDeltaTimeScaled001Factor = 0.00999999978f;
const float kPlayerWorldCollisionStackDrop = 0.200000003f;
const float kPlayerWorldCollisionSubRestoreYOffset = -1.0f;
const float kPlayerWorldCollisionUpwardBounceDamping = -0.800000012f;
const float kPlayerTransferDamageScale = 5.0f;
const float kPlayerTransferVelocityDamping = 0.666700006f;
const int kPlayerNanitePanelDisabledSentinel = 123456789;
const float kPlayerAltAmmoDisabledSentinel = 123456792.0f;
const float kPlayerRecentHitAlertSec = 5.0f;
const int kPlayerMissionSaveLegacySize = 0x124;
const unsigned int kPlayerGunControllerAvailableFlag = 0x04;
const unsigned int kPlayerGunControllerDualMountFlag = 0x02;
const unsigned int kPlayerGunControllerRecoilFlag = 0x01;
const unsigned int kOptCatalogFlagAltDispatchLatch = 0x02;
const unsigned int kOptCatalogFlagLockOnTargetRef = 0x4000;
const unsigned int kPlayerOptCatalogFlagTetherGuided = 1u << 20;
const unsigned int kOptCatalogFlagReload = 1u << 18;
const unsigned int kOptCatalogFlagCreateTrail = 0x02;
const int kCheckpointNodeAuxFlagTracked = 0x02;
const int kCheckpointNodePickableFlag = 0x40000;
const int kCheckpointNodeContextFlag = 0x200000;
const unsigned int kPlayerTimedHitStatusActiveFlag = 0x01;
const unsigned int kOptCatalogFlagBypassDamageProtection = 0x200;
const unsigned int kOptCatalogFlagRecordsRecentHit = 0x1000;
const unsigned int kOptCatalogFlagAppliesTimedHitStatus = 0x200000;
const unsigned int kOptCatalogFlagBlockedInSub = 0x1000;
const unsigned int kOptCatalogFlagNoSubUse = 0x02;
const int kPlayerTickCameraStateProjectileAttached = 7;
const int kPlayerTickCameraStateRestorePrevious = 8;
const char kVehicleEasyZrd[] = "vehicle_easy.zrd";
const char kVehicleHardZrd[] = "vehicle_hard.zrd";
const char kVehicleDefaultZrd[] = "vehicle.zrd";
const char kPlayerAivZrd[] = "aiv.zrd";
const float kPlayerDefaultActivationRange = 100.0f;
const float kPlayerDefaultReturnRange = 250.0f;
const float kPlayerDefaultNotPursuitDwellTime = 3.0f;
const float kPlayerDefaultMaxHealth = 100.0f;
const float kPlayerDefaultAiAttackRadiusSq = 1500.0f;
const float kPlayerDefaultAiAttackDwellTime = 10.0f;
const float kPlayerAiInitialStateDelaySec = 10.0f;
const float kPlayerAiPathFollowMinThrottle = 0.25f;
const float kPlayerAiPathFollowAdvanceDistance = 10.0f;
const float kPlayerAiForwardPathAdvanceDistance = 5.0f;
const float kPlayerAiSyntheticPathRebuildDistanceSq = 400.0f;
const float kPlayerAiSyntheticPathWidth = 10.0f;
const float kPlayerAiSyntheticPathRebuildDelaySec = 1.0f;
const float kPlayerAiAttackLosTargetYOffset = 1.5f;
const float kPlayerAiDynamicOffsetBackUpDistance = 10.0f;
const float kPlayerCameraState2TargetYOffset = 150.0f;
const float kPlayerDefaultAltGunAimOriginZ = -1.0f;
const double kPlayerRadiansToDegrees = 57.29577951308;

struct HitOwnerSaveStateLinkPartial {
    unsigned char unknown_00[0x04];
    zUtil_SaveGameState *ownerSaveState;
};

struct HitOwnerOrContextPartial {
    unsigned char unknown_00[0x40];
    HitOwnerSaveStateLinkPartial *ownerLink;
};

struct PlayerCollisionContactContextPartial {
    unsigned char unknown_00[0x04];
    zUtil_SaveGameState *saveState;
};
RECOIL_STATIC_ASSERT(offsetof(PlayerCollisionContactContextPartial, saveState) == 0x04);

zReader::Node *PlayerZrdArrayBase(zReader::Node *node) {
    return node->value.nodes;
}

int PlayerZrdArrayCount(zReader::Node *node) {
    return PlayerZrdArrayBase(node)[0].value.i32;
}

const char *PlayerZrdArrayString(zReader::Node *node, int index) {
    return PlayerZrdArrayBase(node)[index].value.str;
}

void PlayerDetachDisplayInstanceIfRequested(zClass_NodePartial *node) {
    unsigned int displayInstanceValue = 0;
    zClass_Class::gwNodeGetUserData(node, &displayInstanceValue);
    zClass_Class::gwNodeSetDisplayInstance(node, 0);

    zDiPartial *const displayInstance = (zDiPartial *)displayInstanceValue;
    if (displayInstance != 0 && displayInstance->refCount != 0) {
        zDi::Release(displayInstance);
        zModel_DiPool::FreeIfUnreferenced(displayInstance);
    }
}

void PlayerCacheGunHardpoint(zUtil_PlayerStateStorage *playerState, const char *nodeName,
                             zVec3 *outPosition, int detachDisplays) {
    zClass_NodePartial *const hardpointNode =
        zClass_Class::FindNodeRecursiveByName(playerState->gunNode, nodeName);
    if (hardpointNode == 0) {
        return;
    }

    zClass_Object3D::gwObject3DGetPosition(hardpointNode, &outPosition->x, &outPosition->y,
                                           &outPosition->z);
    if (detachDisplays != 0) {
        PlayerDetachDisplayInstanceIfRequested(hardpointNode);
    }
}

int PlayerZrdArrayInt(zReader::Node *node, int index) {
    return PlayerZrdArrayBase(node)[index].value.i32;
}

float PlayerZrdArrayFloat(zReader::Node *node, int index) {
    return PlayerZrdArrayBase(node)[index].value.f32;
}

zReader::Node *PlayerZrdArrayNode(zReader::Node *node, int index) {
    return &PlayerZrdArrayBase(node)[index];
}

void PlayerCopyZrdArrayString(char *dest, zReader::Node *node, int index) {
    strcpy(dest, PlayerZrdArrayString(node, index));
}

void PlayerLoadSoundSample(zReader::Node *parentNode, const char *name, zSndSample **outSample) {
    zReader::Node *const node = zReader_GetNamedNode(parentNode, name);
    if (node != 0) {
        *outSample = zSnd::FindSampleByName(PlayerZrdArrayString(node, 1));
    }
}

void PlayerLoadModalPointList(zReader::Node *node, zVec3 *points, int *outCount) {
    if (node == 0) {
        *outCount = 0;
        return;
    }

    const int count = PlayerZrdArrayCount(node) - 1;
    *outCount = count;
    for (int index = 0; index < count; ++index) {
        zReader::Node *const coords = PlayerZrdArrayBase(node)[index + 1].value.nodes;
        points[index].x = coords[1].value.f32;
        points[index].y = coords[2].value.f32;
        points[index].z = coords[3].value.f32;
    }
}

void PlayerLoadModalFxList(zReader::Node *modalNode, const char *name,
                           zEffectAnimEntry **entries) {
    zReader::Node *const node = zReader_GetNamedNode(modalNode, name);
    if (node == 0) {
        return;
    }

    int count = PlayerZrdArrayCount(node) - 1;
    if (count > 2) {
        count = 2;
    }

    for (int index = 0; index < count; ++index) {
        entries[index] = zEffectAnim::FindEntryByName(PlayerZrdArrayString(node, index + 1));
    }
}

void PlayerLoadModalWaveParams(PlayerMasterModalData *modalData, zReader::Node *modalNode,
                               const char *name) {
    zReader::Node *const node = zReader_GetNamedNode(modalNode, name);
    if (node == 0) {
        return;
    }

    modalData->hoverPitchWaveBaseRate = PlayerZrdArrayFloat(node, 1);
    modalData->hoverPitchWaveSpeedRate = PlayerZrdArrayFloat(node, 2);
    modalData->hoverPitchWaveAmplitude = PlayerZrdArrayFloat(node, 3);
    modalData->hoverRollWaveBaseRate = PlayerZrdArrayFloat(node, 4);
    modalData->hoverRollWaveSpeedRate = PlayerZrdArrayFloat(node, 5);
    modalData->hoverRollWaveAmplitude = PlayerZrdArrayFloat(node, 6);
    modalData->hoverRollYawCoupleScale = PlayerZrdArrayFloat(node, 7);
}

void SetHudUiElementVisible(HudUiElement *element, int visible) {
    typedef void (RECOIL_THISCALL *SetVisibleFn)(HudUiElement * self, int visible);
    SetVisibleFn const setVisible = (SetVisibleFn)(element->ftable->slots[24]);
    setVisible(element, visible);
}

void SetHudPanelVisible(HudUiPanel *panel, int visible) {
    SetHudUiElementVisible((HudUiElement *)panel, visible);
}

void PlayerInitActionCallbackNode(void *callback) {
    zClass_NodePartial *const node = zClass_Object3D::gwObject3DInit();
    zClass_Class::gwNodeSetPriority(node, 2);
    zClass_Class::gwNodeSetActionCallback(node, callback);
}

PlayerMasterCommonData *PlayerAllocMasterCommonData() {
    PlayerMasterCommonData *const commonData =
        static_cast<PlayerMasterCommonData *>(::operator new(sizeof(PlayerMasterCommonData)));
    memset(commonData, 0, sizeof(PlayerMasterCommonData));
    commonData->next = 0;
    if (g_PlayerMasterCommonDataCount == 0) {
        g_PlayerMasterCommonDataHead = commonData;
    } else {
        g_PlayerMasterCommonDataTail->next = commonData;
    }
    g_PlayerMasterCommonDataTail = commonData;
    ++g_PlayerMasterCommonDataCount;
    return commonData;
}

PlayerMasterModalData *PlayerAllocMasterModalData() {
    PlayerMasterModalData *const modalData =
        static_cast<PlayerMasterModalData *>(::operator new(sizeof(PlayerMasterModalData)));
    memset(modalData, 0, sizeof(PlayerMasterModalData));
    modalData->next = 0;
    if (g_PlayerMasterModalDataCount == 0) {
        g_PlayerMasterModalDataHead = modalData;
    } else {
        g_PlayerMasterModalDataTail->next = modalData;
    }
    g_PlayerMasterModalDataTail = modalData;
    ++g_PlayerMasterModalDataCount;
    return modalData;
}

zUtil_SaveGameState *PlayerAllocLinkedSaveState() {
    zUtil_SaveGameState *saveState =
        static_cast<zUtil_SaveGameState *>(::operator new(sizeof(zUtil_SaveGameState)));
    saveState = zUtil_SaveGameStateList_Init(saveState);
    saveState->next = 0;
    if (g_PlayerSaveStateCount == 0) {
        g_PlayerSaveStateListHead = saveState;
    } else {
        g_PlayerSaveStateListTail->next = saveState;
    }
    g_PlayerSaveStateListTail = saveState;
    ++g_PlayerSaveStateCount;
    return saveState;
}

void PlayerLoadPlayerZrdTuning(zReader::Node *root) {
    zReader::Node *node = zReader_GetNamedNode(root, "camera_zone");
    if (node != 0) {
        const float cameraZone = PlayerZrdArrayFloat(node, 1);
        if (cameraZone > 0.0f && cameraZone < 1.0f) {
            g_Player_CameraZone = cameraZone;
            g_Player_CameraZoneInvRange = 1.0f / (1.0f - cameraZone);
        }
    }

    node = zReader_GetNamedNode(root, "max_cam_yaw_rate");
    g_Player_MaxCamYawRate = node != 0 ? PlayerZrdArrayFloat(node, 1) : 2.0f;

    node = zReader_GetNamedNode(root, "mouse_push");
    if (node != 0) {
        g_Player_MousePushX = PlayerZrdArrayFloat(node, 1);
        g_Player_MousePushY = PlayerZrdArrayFloat(node, 2);
    } else {
        g_Player_MousePushX = 0.00200000009f;
        g_Player_MousePushY = 0.00999999978f;
    }

    node = zReader_GetNamedNode(root, "fp_cam_el_rate");
    g_Player_FpCamElevationRate = node != 0 ? PlayerZrdArrayFloat(node, 1) : 5.0f;

    node = zReader_GetNamedNode(root, "fp_cam_el_lim");
    if (node != 0) {
        g_Player_FpCamElevationMin = PlayerZrdArrayFloat(node, 1);
        g_Player_FpCamElevationMax = PlayerZrdArrayFloat(node, 2);
    } else {
        g_Player_FpCamElevationMin = -0.75f;
        g_Player_FpCamElevationMax = 1.0f;
    }

    node = zReader_GetNamedNode(root, "underwater_cam");
    if (node != 0) {
        int rBits = 0;
        int gBits = 0;
        int bBits = 0;
        g_Player_UnderwaterCamDistance = PlayerZrdArrayFloat(node, 1);
        g_Player_UnderwaterCamHeight = PlayerZrdArrayFloat(node, 2);
        g_Player_UnderwaterCamStepCount = PlayerZrdArrayInt(node, 3);
        g_Player_UnderwaterCamFar = PlayerZrdArrayFloat(node, 4);
        zVideo::PixelPack_GetRgbBits(&rBits, &gBits, &bBits);
        g_Player_UnderwaterCamPackedColor =
            (PlayerZrdArrayInt(node, 5) >> (8 - rBits) << (bBits + gBits)) +
            (PlayerZrdArrayInt(node, 6) >> (8 - gBits) << bBits) +
            (PlayerZrdArrayInt(node, 7) >> (8 - bBits));
        g_Player_UnderwaterCamAlpha = PlayerZrdArrayFloat(node, 8);
    } else {
        g_Player_UnderwaterCamDistance = 5.0f;
        g_Player_UnderwaterCamHeight = 4.0f;
        g_Player_UnderwaterCamStepCount = 12;
        g_Player_UnderwaterCamFar = 100.0f;
        g_Player_UnderwaterCamPackedColor = 0x1f5;
        g_Player_UnderwaterCamAlpha = 0.5f;
    }

    node = zReader_GetNamedNode(root, "camera_elastic");
    if (node != 0) {
        g_Player_CameraElastic = PlayerZrdArrayFloat(node, 1);
    }

    node = zReader_GetNamedNode(root, "max_cam_tether_angle");
    if (node != 0) {
        g_Player_MaxCamTetherAngleRad = PlayerZrdArrayFloat(node, 1) * 0.01745329251994f;
    }

    node = zReader_GetNamedNode(root, "nom_gravity");
    g_Player_NominalGravity = node != 0 ? PlayerZrdArrayFloat(node, 1) : 28.0f;

    node = zReader_GetNamedNode(root, "wat_gravity");
    g_Player_WaterGravity =
        node != 0 ? PlayerZrdArrayFloat(node, 1) : g_Player_NominalGravity * 0.333333343f;

    node = zReader_GetNamedNode(root, "qsd_gravity");
    g_Player_QuicksandGravity =
        node != 0 ? PlayerZrdArrayFloat(node, 1) : g_Player_NominalGravity * 0.166666672f;

    node = zReader_GetNamedNode(root, "qsand_sink");
    g_Player_QuicksandSinkRate = node != 0 ? PlayerZrdArrayFloat(node, 1) : 0.899999976f;

    node = zReader_GetNamedNode(root, "lava_sink");
    g_Player_LavaSinkRate = node != 0 ? PlayerZrdArrayFloat(node, 1) : 0.600000024f;

    node = zReader_GetNamedNode(root, "max_slope");
    g_Player_MaxSlope = node != 0 ? PlayerZrdArrayFloat(node, 1) : 0.707000017f;

    node = zReader_GetNamedNode(root, "make_hot");
    if (node != 0) {
        g_Player_MakeHotOptEntry = OptCatalog::FindEntryByName(PlayerZrdArrayString(node, 1));
    }

    node = zReader_GetNamedNode(root, "make_cold");
    if (node != 0) {
        g_Player_MakeColdOptEntry = OptCatalog::FindEntryByName(PlayerZrdArrayString(node, 1));
    }

    node = zReader_GetNamedNode(root, "burning_anim");
    if (node != 0) {
        g_PlayerRecentHitFxAnimEntry = zEffectAnim::FindEntryByName(PlayerZrdArrayString(node, 1));
    }

    node = zReader_GetNamedNode(root, "low_shield_snd");
    if (node != 0) {
        g_Hud_LowMeterBeepSample = zSnd::FindSampleByName(PlayerZrdArrayString(node, 1));
        g_Hud_LowMeterBeepInterval = PlayerZrdArrayFloat(node, 2);
        g_Hud_LowMeterLoopSample = zSnd::FindSampleByName(PlayerZrdArrayString(node, 3));
    }

    g_PlayerStatusMeterRatio = 1.0f;
    g_Hud_LowMeterNextBeepTime = 0.0f;
    g_Player_CopterSndSample = zSnd::FindSampleByName("snd_chopper");
}

struct PlayerWeatherFxEmitterOverlay {
    HudUiElement ui;
    unsigned char unknown_34[0x1c];
    zClass_NodePartial *cameraNode;
    int particleAgeTick;
};
RECOIL_STATIC_ASSERT(offsetof(PlayerWeatherFxEmitterOverlay, cameraNode) == 0x50);
RECOIL_STATIC_ASSERT(offsetof(PlayerWeatherFxEmitterOverlay, particleAgeTick) == 0x54);

void TriggerZeroVelocityFxList(zEffectAnimEntry **entries, zClass_NodePartial *rootNode,
                               int flags) {
    for (int i = 0; i < 2; ++i) {
        zEffectAnimEntry *const entry = entries[i];
        if (entry != 0 && flags == 0) {
            zEffectAnim::SetVelocity_Thunk(entry, rootNode, 0.0f, 0.0f, 0.0f);
        }
    }
}

void CopyNodeCachedWorldMatrix(zMat4x3 *outMatrix, zClass_NodePartial *node) {
    zClass_Object3DDataPartial *const objectData =
        static_cast<zClass_Object3DDataPartial *>(node->classData);
    memcpy(outMatrix, objectData->cachedWorldMatrix, sizeof(*outMatrix));
}

float ExtractYawFromMatrix(const zMat4x3 *matrix) {
    return static_cast<float>(atan2(matrix->zx, matrix->zz));
}

const char *PlayerDebugMasterTypeName(int masterType) {
    switch (masterType) {
    case 0:
        return "BASIC";
    case kPlayerMasterTypeFly:
        return "FLY";
    case kPlayerMasterTypeSub:
        return "SUB";
    case kPlayerMasterTypeTrack:
        return "TRACK";
    case kPlayerMasterTypeHover:
        return "HOVER";
    case kPlayerMasterTypeAmphib:
        return "AMPHIB";
    default:
        return "UNKNOWN";
    }
}

void CacheAttachmentLocalOffset(zUtil_PlayerStateStorage *playerState) {
    const float dx = playerState->worldPos.x - playerState->environmentAttachmentMatrix.posX;
    const float dy = playerState->worldPos.y - playerState->environmentAttachmentMatrix.posY;
    const float dz = playerState->worldPos.z - playerState->environmentAttachmentMatrix.posZ;
    const zMat4x3 *const matrix = &playerState->environmentAttachmentMatrix;

    playerState->fxOffsetLocal.x = dx * matrix->xx + dy * matrix->xy + dz * matrix->xz;
    playerState->fxOffsetLocal.y = dx * matrix->yx + dy * matrix->yy + dz * matrix->yz;
    playerState->fxOffsetLocal.z = dx * matrix->zx + dy * matrix->zy + dz * matrix->zz;
}

PlayerGunFireController *PlayerSavedWeaponController(PlayerAltWeaponBank *bank, int sideIndex) {
    return sideIndex == 0 ? &bank->controllerA : &bank->controllerB;
}

void PlayerRestoreSavedWeaponSide(PlayerGunFireController *controller,
                                  const PlayerMissionSaveWeaponSide *savedSide) {
    controller->flags &= ~kPlayerGunControllerAvailableFlag;
    if ((savedSide->enabled & 1) != 0) {
        controller->flags |= kPlayerGunControllerAvailableFlag;
    }
    controller->ammoOrCharge = savedSide->ammoOrCharge;
}

void PlayerRefreshSavedWeaponBankHud(int bankIndex, PlayerAltWeaponBank *bank) {
    PlayerGunFireController *const selectedController =
        PlayerSavedWeaponController(bank, bank->selectedSide);

    HudUiMessage::SetValueIfOwnerMatches(bankIndex, bank->selectedSide,
                                         selectedController->ammoOrCharge);
    if ((selectedController->flags & kPlayerGunControllerAvailableFlag) != 0) {
        HudUiMessage::SelectVariantDisplay(bankIndex, bank->selectedSide);
    } else {
        HudUiMessage::ClearDisplay(bankIndex);
    }
}

void PlayerRefreshPreviousWeaponControllerHud(PlayerGunFireController *controller) {
    if ((controller->flags & kPlayerGunControllerAvailableFlag) != 0) {
        HudUiMessage::SelectVariantDisplay(controller->weaponBankIndex,
                                           controller->weaponSideIndex);
    } else {
        HudUiMessage::ClearDisplay(controller->weaponBankIndex);
    }
}

void PlayerResetWeaponController(PlayerGunFireController *controller, int bankIndex,
                                 int sideIndex, float ammoOrCharge) {
    controller->weaponBankIndex = bankIndex;
    controller->weaponSideIndex = sideIndex;
    controller->flags &= ~kPlayerGunControllerAvailableFlag;
    controller->ammoOrCharge = ammoOrCharge;
    controller->attachNodePrimary = 0;
    controller->trailRuntimeState = 0;
}

zDiPartial *PlayerFindScrollTextureModel(zClass_NodePartial *root, const char *mountName) {
    char scrollName[0x50];
    sprintf(scrollName, "%sSCROLL", mountName);

    zClass_NodePartial *const scrollNode =
        root != 0 ? zClass_Class::FindNodeRecursiveByName(root, scrollName) : 0;
    if (scrollNode == 0) {
        return 0;
    }

    unsigned int userData = 0;
    zClass_Class::gwNodeGetUserData(scrollNode, &userData);
    zDiPartial *const textureModel = (zDiPartial *)userData;
    zModel::SetDiTextureWorldPerMeter(textureModel, 1, 0.0f, 2);
    return textureModel;
}

void PlayerBindDualWeaponMount(zUtil_PlayerStateStorage *playerState,
                               PlayerGunFireController *controller) {
    char mountName[0x50];
    sprintf(mountName, "%s_L", controller->optCatalogEntry->displayName);
    controller->attachNodePrimary =
        zClass_Class::FindNodeRecursiveByName(playerState->gunNode, mountName);
    if (controller->attachNodePrimary != 0) {
        zClass_Class::gwNodeSetActive(controller->attachNodePrimary, 0);
    }
    controller->scrollTextureModelA =
        PlayerFindScrollTextureModel(controller->attachNodePrimary, mountName);

    sprintf(mountName, "%s_R", controller->optCatalogEntry->displayName);
    controller->attachNodeSecondary =
        zClass_Class::FindNodeRecursiveByName(playerState->gunNode, mountName);
    if (controller->attachNodeSecondary != 0) {
        zClass_Class::gwNodeSetActive(controller->attachNodeSecondary, 0);
    }
    controller->scrollTextureModelB =
        PlayerFindScrollTextureModel(controller->attachNodeSecondary, mountName);
}

void PlayerBindSingleWeaponMount(zUtil_PlayerStateStorage *playerState,
                                 PlayerGunFireController *controller) {
    controller->attachNodePrimary =
        zClass_Class::FindNodeRecursiveByName(playerState->gunNode,
                                              controller->optCatalogEntry->displayName);
    if (controller->attachNodePrimary != 0) {
        zClass_Class::gwNodeSetActive(controller->attachNodePrimary, 0);
    }
    zClass_Object3D::gwObject3DGetPosition(controller->attachNodePrimary,
                                           &controller->attachPosX,
                                           &controller->attachPosY,
                                           &controller->attachPosZ);
}

float &PlayerAltGunTransitionAnimScale(zUtil_PlayerStateStorage *playerState) {
    return *((float *)playerState->unknown_0f24);
}

void TickAltGunRetractDelay(zUtil_SaveGameState *saveState, zUtil_PlayerStateStorage *playerState) {
    playerState->altGunTransitionTimerA += g_FrameDeltaTimeSec;
    if (playerState->altGunTransitionTimerA > 0.300000012f) {
        playerState->altGunTransitionTimerA = 0.0f;
        playerState->altGunTransitionState = 4;
        saveState->StartMasterTypeLoopSfxHandle(0, 1.0f);
    }
}

void TickAltGunLowerTransition(zUtil_PlayerStateStorage *playerState,
                               PlayerGunFireController *controller) {
    playerState->altGunTransitionTimerA += g_FrameDeltaTimeSec;
    const float progress = playerState->altGunTransitionTimerA * 4.0f;
    const float targetY = controller->attachPosY - 0.400000006f;
    const float animScale = progress * 0.400000006f;
    PlayerAltGunTransitionAnimScale(playerState) = animScale;

    const float y = controller->attachPosY - animScale;
    if (y <= targetY) {
        zClass_Object3D::gwObject3DSetPosition(controller->attachNodePrimary,
                                               controller->attachPosX, targetY,
                                               controller->attachPosZ);
        zClass_Object3D::gwObject3DSetScale(controller->attachNodePrimary, 1.0f,
                                            0.600000024f, 0.600000024f);
        playerState->altGunTransitionState = 8;
        playerState->altGunTransitionTimerA = 0.0f;
        return;
    }

    zClass_Object3D::gwObject3DSetPosition(controller->attachNodePrimary, controller->attachPosX,
                                           y, controller->attachPosZ);
    const float scale = 1.0f - progress * 0.399999976f;
    zClass_Object3D::gwObject3DSetScale(controller->attachNodePrimary, 1.0f, scale, scale);
    zClass_Object3D::gwObject3DSetRotation(controller->attachNodePrimary, 0.0f, 0.0f, 0.0f);
}

void TickAltGunDoorOpen(zUtil_PlayerStateStorage *playerState) {
    playerState->altGunTransitionTimerB += g_FrameDeltaTimeSec;
    float xScale = playerState->altGunTransitionTimerB * 4.0f;
    if (xScale >= 1.0f) {
        xScale = 1.0f;
        playerState->altGunTransitionState = 16;
        playerState->altGunTransitionTimerB = 0.0f;
    }

    zClass_Object3D::gwObject3DSetScale(playerState->doorLeftNode, xScale, 1.0f, 1.0f);
    zClass_Object3D::gwObject3DSetScale(playerState->doorRightNode, xScale, 1.0f, 1.0f);
}

void DeactivateOppositeAltGunControllerNodes(zUtil_PlayerStateStorage *playerState,
                                             PlayerGunFireController *activeController) {
    PlayerAltWeaponBank *const bank = &playerState->altWeaponBanks[activeController->weaponBankIndex];
    PlayerGunFireController *const oppositeController =
        activeController->weaponSideIndex == 0 ? &bank->controllerB : &bank->controllerA;

    if (oppositeController->attachNodePrimary != 0) {
        zClass_Class::gwNodeSetActive(oppositeController->attachNodePrimary, 0);
    }
    if (oppositeController->attachNodeSecondary != 0) {
        zClass_Class::gwNodeSetActive(oppositeController->attachNodeSecondary, 0);
    }
}

void TickAltGunActivateTransition(zUtil_SaveGameState *saveState,
                                  zUtil_PlayerStateStorage *playerState) {
    PlayerGunFireController *const transitionController =
        playerState->altGunTransitionController;
    if (transitionController != 0 && transitionController->attachNodePrimary != 0 &&
        (transitionController->flags & kPlayerGunControllerDualMountFlag) == 0) {
        OptCatalogRuntimeInstanceStorage *const attachState =
            (OptCatalogRuntimeInstanceStorage *)transitionController->attachState;
        if (attachState != 0) {
            zClass_Class::RemoveChild(transitionController->attachNodePrimary,
                                      attachState->projectileNode);
            OptCatalog::RecycleRuntimeInstanceStorage(transitionController->optCatalogEntry,
                                                      attachState);
            transitionController->attachState = 0;
        }

        zClass_Class::gwNodeSetActive(transitionController->attachNodePrimary, 0);
        zClass_Object3D::gwObject3DSetPosition(
            transitionController->attachNodePrimary, transitionController->attachPosX,
            transitionController->attachPosY, transitionController->attachPosZ);
    }

    PlayerGunFireController *const activeController = playerState->activeAltGunController;
    if (activeController == 0 || activeController->attachNodePrimary == 0) {
        playerState->altGunTransitionState = 1;
        return;
    }

    if ((activeController->flags & kPlayerGunControllerDualMountFlag) != 0) {
        saveState->StartMasterTypeLoopSfxHandle(2, 1.0f);
        DeactivateOppositeAltGunControllerNodes(playerState, activeController);
        if (activeController->attachNodePrimary != 0) {
            zClass_Class::gwNodeSetActive(activeController->attachNodePrimary, 1);
        }
        if (activeController->attachNodeSecondary != 0) {
            zClass_Class::gwNodeSetActive(activeController->attachNodeSecondary, 1);
        }
        playerState->altGunTransitionState = 1;
        return;
    }

    OptCatalogEntryDef *const entry = activeController->optCatalogEntry;
    if ((entry->flags & kOptCatalogFlagReload) != 0 && activeController->ammoOrCharge > 0.0f) {
        activeController->attachState = OptCatalog::AllocOrReuseAttachNodeClone(entry);
        OptCatalogRuntimeInstanceStorage *const attachState =
            (OptCatalogRuntimeInstanceStorage *)activeController->attachState;
        zClass_Class::AddChild(activeController->attachNodePrimary, attachState->projectileNode);
        attachState->ownerNode = playerState->rootNode;
    }

    zClass_Class::gwNodeSetActive(activeController->attachNodePrimary, 1);
    zClass_Object3D::gwObject3DSetPosition(activeController->attachNodePrimary,
                                           activeController->attachPosX,
                                           activeController->attachPosY - 0.400000006f,
                                           activeController->attachPosZ);
    zClass_Object3D::gwObject3DSetScale(activeController->attachNodePrimary, 1.0f,
                                        0.600000024f, 0.600000024f);
    playerState->altGunTransitionState = 32;
    saveState->StartMasterTypeLoopSfxHandle(0, 1.0f);
}

void TickAltGunDoorClose(zUtil_PlayerStateStorage *playerState) {
    playerState->altGunTransitionTimerB += g_FrameDeltaTimeSec;
    float xScale = 1.0f - playerState->altGunTransitionTimerB * 4.0f;
    if (xScale <= 0.00100000005f) {
        xScale = 0.00100000005f;
        playerState->altGunTransitionState = 64;
        playerState->altGunTransitionTimerB = 0.0f;
    }

    zClass_Object3D::gwObject3DSetScale(playerState->doorLeftNode, xScale, 1.0f, 1.0f);
    zClass_Object3D::gwObject3DSetScale(playerState->doorRightNode, xScale, 1.0f, 1.0f);
}

void TickAltGunRaiseTransition(zUtil_PlayerStateStorage *playerState,
                               PlayerGunFireController *controller) {
    playerState->altGunTransitionTimerA += g_FrameDeltaTimeSec;
    const float progress = playerState->altGunTransitionTimerA * 4.0f;
    const float animScale = progress * 0.400000006f;
    PlayerAltGunTransitionAnimScale(playerState) = animScale;
    const float y = animScale + controller->attachPosY - 0.400000006f;
    if (y >= controller->attachPosY) {
        PlayerAltGunTransitionAnimScale(playerState) = 0.400000006f;
        playerState->altGunTransitionState = 1;
        playerState->altGunTransitionTimerA = 0.0f;
        zClass_Object3D::gwObject3DSetPosition(controller->attachNodePrimary,
                                               controller->attachPosX, controller->attachPosY,
                                               controller->attachPosZ);
        zClass_Object3D::gwObject3DSetScale(controller->attachNodePrimary, 1.0f, 1.0f,
                                            1.0f);
        return;
    }

    zClass_Object3D::gwObject3DSetPosition(controller->attachNodePrimary, controller->attachPosX,
                                           y, controller->attachPosZ);
    const float scale = progress * 0.399999976f + 0.600000024f;
    zClass_Object3D::gwObject3DSetScale(controller->attachNodePrimary, 1.0f, scale, scale);
}

void TickAltGunTransitionAnimation(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const transitionController =
        playerState->altGunTransitionController;

    switch (playerState->altGunTransitionState) {
    case 2:
        TickAltGunRetractDelay(saveState, playerState);
        break;
    case 4:
        TickAltGunLowerTransition(playerState, transitionController);
        break;
    case 8:
        TickAltGunDoorOpen(playerState);
        break;
    case 16:
        TickAltGunActivateTransition(saveState, playerState);
        break;
    case 32:
        TickAltGunDoorClose(playerState);
        break;
    case 64:
        TickAltGunRaiseTransition(playerState, playerState->activeAltGunController);
        break;
    }
}

void TickAltGunTetherCleanup(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const activeController = playerState->activeAltGunController;
    OptCatalogRuntimeInstanceStorage *const attachState =
        (OptCatalogRuntimeInstanceStorage *)activeController->attachState;

    if (attachState->ownerNode == 0) {
        if (playerState->cameraState == kPlayerTickCameraStateProjectileAttached) {
            HudUiMgr::EnableHud();
            Player::ApplyCameraState(kPlayerTickCameraStateRestorePrevious);
        }
        playerState->pendingAltCameraToggle = 0;
        OptCatalog::RecycleRuntimeInstanceStorage(activeController->optCatalogEntry,
                                                  attachState);
        activeController->attachState = 0;
        playerState->altGunTransitionState =
            activeController->ammoOrCharge > 0.0f ? 4 : 1;
        return;
    }

    if (playerState->pendingAltCameraToggle != 0) {
        if (playerState->cameraState == kPlayerTickCameraStateProjectileAttached) {
            HudUiMgr::EnableHud();
            Player::ApplyCameraState(kPlayerTickCameraStateRestorePrevious);
            playerState->altGunTransitionState = 256;
        } else {
            HudUiMgr::DisableHud();
            Player::ApplyCameraState(kPlayerTickCameraStateProjectileAttached);
            playerState->altGunTransitionState = 128;
        }
        playerState->pendingAltCameraToggle = 0;
    }
}

void TickAltGunTriggerProcessCleanup(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (playerState->altGunTriggerProcessFlag == 0) {
        return;
    }

    char message[0x50] = {0};
    int removedA = 0;
    int removedB = 0;
    OptCatalogEntryDef *const entryA = playerState->altWeaponBanks[5].controllerA.optCatalogEntry;
    if (entryA != 0) {
        removedA = OptCatalog::RemoveRuntimeInstance(entryA, 0, playerState->rootNode);
        if (removedA != 0) {
            zLoc::FormatMessage(message, sizeof(message), 0x248, removedA);
            HudUi::ShowTopMessageLine(message, 5.0f);
        }
    }

    OptCatalogEntryDef *const entryB = playerState->altWeaponBanks[5].controllerB.optCatalogEntry;
    if (entryB != 0) {
        removedB = OptCatalog::RemoveRuntimeInstance(entryB, 0, playerState->rootNode);
        if (removedB != 0) {
            zLoc::FormatMessage(message, sizeof(message), 0x249, removedB);
            HudUi::ShowTopMessageLine(message, 5.0f);
        }
    }

    if (removedA == 0 && removedB == 0) {
        OptCatalog::PlayWeaponInactiveWarning();
    }
    playerState->altGunTriggerProcessFlag = 0;
}

int TickAltGunLocalAmmoState(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const activeController = playerState->activeAltGunController;
    OptCatalogEntryDef *const entry = activeController->optCatalogEntry;

    if (playerState->altGunFireHeldFlag != 0 &&
        activeController->ammoOrCharge != kPlayerAltAmmoDisabledSentinel) {
        activeController->ammoOrCharge -= g_FrameDeltaTimeSec / entry->fireRateInterval;
        if (activeController->ammoOrCharge < 0.0f) {
            activeController->ammoOrCharge = 0.0f;
        }
        activeController->trailRuntimeState->ammoOrChargeMirror =
            activeController->ammoOrCharge;
    }

    if (activeController->ammoOrCharge <= 0.0f) {
        if ((entry->flags & kOptCatalogFlagReload) != 0 &&
            playerState->altGunTransitionState != 1) {
            return 0;
        }

        activeController->ammoOrCharge = 0.0f;
        if (playerState->altGunFireHeldFlag != 0) {
            activeController->trailRuntimeState->ammoOrChargeMirror = 0.0f;
            playerState->altGunFireHeldFlag = 0;
            playerState->altGunDispatchRequested = 0;
            OptCatalog::DeactivateTrailRuntimeState(activeController->trailRuntimeState);
        }

        HudUiMessage::SetValueIfOwnerMatches(activeController->weaponBankIndex,
                                             activeController->weaponSideIndex, 0.0f);
        Player::AutoSwitchToNextUsableAltWeapon(saveState);
        return 1;
    }

    if ((entry->flags & kOptCatalogFlagReload) != 0 &&
        playerState->altGunTransitionState == 1 && activeController->attachState == 0) {
        playerState->altGunTransitionState = 2;
    }
    return 1;
}

void TickAltGunLocalSlotAndPrimaryState(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (playerState->gunNode != 0) {
        if (playerState->altFireSlotLeft.offset != 0.0f) {
            Player::DecayAndApplyAltFireSlotOffsetToNode(
                &playerState->altFireSlotLeft, playerState->altFireSlotLeft.attachNode,
                playerState->gunFireDir.y, 1);
        }
        if (playerState->altFireSlotRight.offset != 0.0f) {
            Player::DecayAndApplyAltFireSlotOffsetToNode(
                &playerState->altFireSlotRight, playerState->altFireSlotRight.attachNode,
                playerState->gunFireDir.y, 1);
        }
        if (playerState->altFireSlotCenter.offset != 0.0f) {
            Player::DecayAndApplyAltFireSlotOffsetToNode(
                &playerState->altFireSlotCenter, playerState->altFireSlotCenter.attachNode,
                playerState->gunFireDir.y, 0);
        }
    }

    if (playerState->activePrimaryGunController != 0) {
        Player::ProcessPrimaryGunDispatchRequest(saveState);
    }
}

template <typename T> zZbdSectionCallback ZbdCallbackPtr(T callback) {
    RECOIL_STATIC_ASSERT(sizeof(T) == sizeof(zZbdSectionCallback));
    union {
        T callback;
        zZbdSectionCallback raw;
    } value = {0};
    value.callback = callback;
    return value.raw;
}
} // namespace

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

// Reimplements 0x438630: Player::EnsureMasterTypeLoopSfxHandle
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL
zUtil_SaveGameState::EnsureMasterTypeLoopSfxHandle(int modeIndex, float sfxVolume) {
    zUtil_SaveGameState *const saveState = this;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (playerState->modeLoopSfxHandle[modeIndex] == 0 &&
        playerState->masterCommonData->sfxWeaponUp[modeIndex] != 0) {
        saveState->StartMasterTypeLoopSfxHandle(modeIndex, sfxVolume);
    }
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

// Reimplements 0x438540: Player::SelectModalStateByMasterType
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL
zUtil_SaveGameState::SelectModalStateByMasterType(int masterType) {
    zUtil_SaveGameState *const saveState = this;
    PlayerModalState *modalState = saveState->modalStateListHead;
    if (modalState == 0) {
        return 0;
    }

    do {
        if (modalState->masterModalData->masterType == masterType) {
            saveState->StopModalLoopSfxHandle(2);
            saveState->StopModalLoopSfxHandle(0);
            saveState->StopModalLoopSfxHandle(1);
            saveState->primaryModalState = modalState;
            return 1;
        }
        modalState = modalState != 0 ? modalState->next : 0;
    } while (modalState != 0);

    return 0;
}

// Reimplements 0x438660: Player::StopMasterTypeLoopSfxHandle
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL
zUtil_SaveGameState::StopMasterTypeLoopSfxHandle(int modeIndex) {
    zUtil_SaveGameState *const saveState = this;
    zSndPlayHandle *const handle = saveState->playerState->modeLoopSfxHandle[modeIndex];
    if (handle != 0) {
        handle->StopIfActive();
        saveState->playerState->modeLoopSfxHandle[modeIndex] = 0;
    }
}

// Reimplements 0x4386c0: Player::UpdateModalLoopSfx
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL zUtil_SaveGameState::UpdateModalLoopSfx(int enabled) {
    zUtil_SaveGameState *const saveState = this;
    if (enabled == 0) {
        saveState->StopModalLoopSfxHandle(2);
        saveState->StopModalLoopSfxHandle(0);
        saveState->StopModalLoopSfxHandle(1);
        saveState->StopMasterTypeLoopSfxHandle(3);
        return;
    }

    PlayerModalState *primaryModalState = saveState->primaryModalState;
    if (primaryModalState->modalSfxHandle[0] == 0) {
        if (primaryModalState->masterModalData->sfxEngine[0] != 0) {
            saveState->StartModalLoopSfxHandle(0, 0.0f);
        }
        if (saveState->primaryModalState->masterModalData->sfxEngine[1] != 0) {
            saveState->StartModalLoopSfxHandle(1, 0.0f);
        }
        if (saveState->primaryModalState->masterModalData->sfxEngine[2] != 0) {
            saveState->StartModalLoopSfxHandle(2, 0.0f);
        }
        return;
    }

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (playerState->slipSfxActive != 0 || playerState->airborneFlag != 0 ||
        playerState->damageProtectionActive != 0) {
        const int smoothingBits =
            static_cast<int>(g_FrameDeltaTimeSec * -2.5f * 12102200.0f) + 0x3f800000;
        const float smoothingFactor = PlayerFloatFromBits(smoothingBits);
        saveState->modeLoopBlend =
            fabsf(playerState->throttleInputCopy) * (1.0f - smoothingFactor) +
            smoothingFactor * saveState->modeLoopBlend;
    } else {
        saveState->modeLoopBlend =
            fabsf(playerState->localVel.z) / primaryModalState->masterModalData->maxSpeed;
    }

    saveState->modeLoopBlend = PlayerClamp01(saveState->modeLoopBlend);

    primaryModalState = saveState->primaryModalState;
    zSndPlayHandle *handle = primaryModalState->modalSfxHandle[2];
    if (handle != 0) {
        handle->SetFreqScaled(primaryModalState->masterModalData->sfxPitchScale *
                              saveState->modeLoopBlend);
        saveState->primaryModalState->modalSfxHandle[2]->SetEnableScale(
            1.0f - saveState->modeLoopBlend);
        saveState->primaryModalState->modalSfxHandle[2]->Update3DDispatch(
            &saveState->playerState->worldPos, 0, 0);
    }

    primaryModalState = saveState->primaryModalState;
    PlayerMasterModalData *const masterModalData = primaryModalState->masterModalData;
    const float engineEnableScale =
        PlayerClamp01(masterModalData->sfxVolumeScale * saveState->modeLoopBlend + 0.699999988f);

    primaryModalState->modalSfxHandle[0]->SetFreqScaled(masterModalData->sfxPitchScale *
                                                        saveState->modeLoopBlend);
    saveState->primaryModalState->modalSfxHandle[0]->SetEnableScale(engineEnableScale);
    saveState->primaryModalState->modalSfxHandle[0]->Update3DDispatch(
        &saveState->playerState->worldPos, 0, 0);

    handle = saveState->primaryModalState->modalSfxHandle[1];
    if (handle != 0) {
        handle->SetEnableScale(saveState->modeLoopBlend);
        saveState->primaryModalState->modalSfxHandle[1]->Update3DDispatch(
            &saveState->playerState->worldPos, 0, 0);
    }
}

namespace zVehicle {

// Reimplements 0x41fe50: zVehicle::SelectZrdByDifficulty
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE const char *RECOIL_FASTCALL SelectZrdByDifficulty(const char *extraSearchPath) {
    const char *filename = kVehicleDefaultZrd;
    const int difficultyMode = zOpt::GetGameDifficultyMode();
    if (difficultyMode == 0) {
        filename = kVehicleEasyZrd;
    } else if (difficultyMode == 2) {
        filename = kVehicleHardZrd;
    }

    if (zReader::TryResolvePath(filename, extraSearchPath) == 0) {
        filename = kVehicleDefaultZrd;
    }

    return filename;
}

} // namespace zVehicle

namespace Player {
// Reimplements 0x41fe40: Player::GetAivZrdPath
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE const char *RECOIL_CDECL GetAivZrdPath() {
    return kPlayerAivZrd;
}

// Reimplements 0x423150: Player::ExtractVehicleNameFromAivName
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ExtractVehicleNameFromAivName(const char *aivName,
                                                                   char *outVehicleName) {
    int outLen = 0;
    outVehicleName[0] = '\0';
    if (aivName[0] == '\0') {
        return;
    }

    const char *cursor = aivName;
    do {
        if (*cursor == '_' && isdigit(aivName[outLen + 1]) != 0) {
            break;
        }

        outVehicleName[outLen] = *cursor;
        ++outLen;
        outVehicleName[outLen] = '\0';
        ++cursor;
    } while (*cursor != '\0');
}

// Reimplements 0x421a40: Player::CloneType6NodeFromTemplateAndRename
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL
CloneType6NodeFromTemplateAndRename(const char *templateName, const char *newName) {
    zClass_NodePartial *const source = zClass::FindByTypeAndName(6, templateName);
    if (source == 0) {
        return 0;
    }

    const int cloneDiMode = zOpt::GetNetworkEnabled() != 0 ? 1 : 0;
    zClass_NodePartial *const child =
        zClass_cls_util::CopyNodeWithCloneOptions(source, cloneDiMode, cloneDiMode);
    if (child == 0) {
        return 0;
    }

    if (zClass_World::AddChildAtGrid(g_Player_RuntimeDiScene, child) != 0) {
        return 0;
    }

    if (zClass_Class::gwNodeSetName(child, newName) != 0) {
        return 0;
    }

    zClass_Class::gwNodeSetActive(child, 1);
    return child;
}

// Reimplements 0x421ab0: Player::CreateFromNamesAtPose
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL CreateFromNamesAtPose(
    const zVec3 *spawnPos, int aiNetId, float yawDeg, const char *templateName,
    const char *objectName) {
    const int objectIsBft00 = strcmp(objectName, "bft_00") == 0;
    zClass_NodePartial *rootNode = 0;

    if (zOpt::GetNetworkEnabled() != 0 && objectIsBft00 != 0) {
        rootNode = zClass::FindByTypeAndName(6, "bft_00");
        if (rootNode == 0) {
            return 0;
        }

        zClass_NodePartial *const networkClone =
            zClass_cls_util::CopyNodeWithCloneOptions(rootNode, 1, 1);
        if (networkClone != 0) {
            zClass_Class::gwNodeSetName(networkClone, "bft_99");
        }

        rootNode->flags |= kPlayerNodeFlagNetworkBftCloneSource;
    } else {
        rootNode = zClass::FindByTypeAndName(6, objectName);
        if (rootNode == 0) {
            rootNode = CloneType6NodeFromTemplateAndRename(templateName, objectName);
        }
        if (rootNode == 0) {
            return 0;
        }
    }

    zUtil_SaveGameState *saveState =
        static_cast<zUtil_SaveGameState *>(::operator new(sizeof(zUtil_SaveGameState)));
    saveState = zUtil_SaveGameStateList_Init(saveState);
    saveState->next = 0;
    if (g_PlayerSaveStateCount == 0) {
        g_PlayerSaveStateListHead = saveState;
    } else {
        g_PlayerSaveStateListTail->next = saveState;
    }
    g_PlayerSaveStateListTail = saveState;
    saveState->next = 0;
    ++g_PlayerSaveStateCount;

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (spawnPos != 0) {
        zClass_Object3D::gwObject3DSetPosition(rootNode, spawnPos->x, spawnPos->y, spawnPos->z);
        zClass_Object3D::gwObject3DSetRotation(rootNode, 0.0f,
                                               yawDeg * 0.01745329251994f, 0.0f);
        playerState->aiNetId = aiNetId;
    }

    if (rootNode->listCountA == 0) {
        zClass_Class::AddChild(g_Player_RuntimeDiScene, rootNode);
    }

    playerState->rootNode = rootNode;
    InitStateFromNameAndMasterCommonData(saveState, objectName, templateName);

    if (objectIsBft00 != 0) {
        zClass_Node::SetDamageHitCallback((void *)(&EnterDestroyedState),
                                          playerState->rootNode, saveState);
        g_OptCatalogDamageFeedbackTrackedNode = playerState->rootNode;
        g_Player_LocalFxOffsetWorldPtr = &playerState->fxOffsetWorld;
        zClass_Camera::SetTargetNode(playerState->rootNode);
        g_HudSensorTracker.SetTrackedSaveState(saveState);
        if (zOpt::GetNetworkEnabled() == 0 && OptCatalog_IsDamageMaskEnabled() != 0) {
            zClass_Node::SetMaterialFlagBit9ForFlagBit0EntriesRecursive(playerState->rootNode,
                                                                        1);
        }
    } else {
        void *callback = (void *)(&HitCallback_RecordContextAndTimedStatus);
        if (strstr(objectName, "net") != 0) {
            callback = (void *)(&HitCallback_RecordNetContextAndTimedStatus);
        }
        zClass_Node::SetDamageHitCallback(callback, playerState->rootNode, saveState);
    }

    PlayerMasterCommonData *const commonData = playerState->masterCommonData;
    for (int i = 0; i < commonData->modalCount; ++i) {
        PlayerModalState *const modalState =
            (PlayerModalState *)zUtil_SaveGameStateList_AllocAppend(saveState);
        BindModalStateFromMasterModalData(saveState, modalState, commonData->modalNames[i],
                                          objectName);
    }

    if (playerState->destroyedRespawnFxEntry == 0) {
        playerState->destroyedRespawnFxEntry = zEffectAnim::FindEntryByName(templateName);
    }

    InitSpawnStateFromPrimaryModalData(saveState);
    if (saveState != (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        ++g_HudSensorTracker.missionStat1;
    }

    return 1;
}

// Reimplements 0x421ea0: Player::CreateFromNamesAtPoseGetState
RECOIL_NOINLINE zUtil_SaveGameState *RECOIL_FASTCALL CreateFromNamesAtPoseGetState(
    const zVec3 *spawnPos, const char *templateName, float yawDeg, const char *objectName) {
    if (CreateFromNamesAtPose(spawnPos, 0, yawDeg, templateName, objectName) == 0) {
        return 0;
    }

    return g_PlayerSaveStateListTail;
}

// Reimplements 0x41fe90: Player::InitMissionRuntimeFromWorldAndCamera
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
InitMissionRuntimeFromWorldAndCamera(zClass_NodePartial *worldNode,
                                     zClass_NodePartial *cameraNode) {
    if (g_Player_MissionInitFirstRunFlag != 0) {
        g_HudUiTopMessageStack->base.AddChild((HudUiElement *)(&g_Player_TopMsgPanel1));
        g_HudUiTopMessageStack->base.AddChild((HudUiElement *)(&g_Player_TopMsgPanel2));
        g_Player_MissionInitFirstRunFlag = 0;
    }

    if (zOpt::GetNetworkEnabled() == 0) {
        AINet::LoadAllFromZrd();
    }

    g_Player_TopMsgPanel1.SetTextFmt(zLoc::GetMessageString(0x909));
    ((HudUiElement *)(&g_Player_TopMsgPanel1))->x = 55;
    ((HudUiElement *)(&g_Player_TopMsgPanel1))->y = 66;
    g_Player_TopMsgPanel1.Invalidate();
    SetHudPanelVisible(&g_Player_TopMsgPanel1, 0);

    g_Player_TopMsgPanel2.SetTextFmt(zLoc::GetMessageString(0x910));
    ((HudUiElement *)(&g_Player_TopMsgPanel2))->x = 55;
    ((HudUiElement *)(&g_Player_TopMsgPanel2))->y = 66;
    g_Player_TopMsgPanel2.Invalidate();
    SetHudPanelVisible(&g_Player_TopMsgPanel2, 0);

    ((HudUiContainer *)(&g_zVideo_FxPass3ConfigLocal))->AddChild(&g_Player_UnderwaterFxPass3Ui);
    SetHudUiElementVisible(&g_Player_UnderwaterFxPass3Ui, 0);
    ((HudUiContainer *)(&g_zVideo_FxPass3ConfigLocal))->AddChild(&g_Player_State7FxPass3Ui);
    SetHudUiElementVisible(&g_Player_State7FxPass3Ui, 0);

    g_Player_RuntimeDiScene = worldNode;
    g_MainCamera = cameraNode;
    g_Player_HorizonNode =
        zClass_Class::FindSubNodeByName(g_HudSensorTracker.worldNode, "horizon");

    float fovX = 0.0f;
    float fovY = 0.0f;
    zClass_Camera::gwCameraGetFOV(g_MainCamera, &fovX, &fovY);
    zClass_Camera::gwCameraSetPosition(g_MainCamera, 0.0f, 0.0f, 0.0f);
    zClass_Camera::gwCameraSetTarget(g_MainCamera, 0.0f, 0.0f, 0.0f);
    if (g_Player_HorizonNode != 0) {
        g_Player_HorizonNodeFollowCameraEnabled = 1;
        zClass_Object3D::gwObject3DSetPosition(g_Player_HorizonNode, 0.0f, 0.0f, 0.0f);
    }

    memset(&g_VariantTag_Current, 0, sizeof(g_VariantTag_Current));
    zTag4::Clear(&g_VariantTag_Current);
    g_Variant_CurrentTag = g_VariantTag_Current;

    PlayerInitActionCallbackNode((void *)(&TickAllPlayers));
    PlayerInitActionCallbackNode((void *)(&PickupRespawnQueue::Update));
    PlayerInitActionCallbackNode((void *)(&zClass_Object3D_ModelRefLerpQueue::Update));

    g_Player_LocalControlEnabled = zOpt::GetNetworkEnabled();
    g_Player_TotalTimeSecScaled = g_Time_AccumulatedTimeSec;
    g_Player_CameraZone = 0.899999976f;
    g_Player_CameraZoneInvRange = 10.0f;
    g_Player_CopterSndNode1 = 0;
    g_Player_CopterSndNode2 = 0;
    g_Player_BftSplashAnimEntry = zEffectAnim::FindEntryByName("bftsplash");

    zReader::Node *playerRoot = zReader::LoadNodeFromPath("player.zrd", 0, 0);
    PlayerLoadPlayerZrdTuning(playerRoot);
    zReader::FreeLoadedTree(playerRoot);

    g_Player_RuntimeInputFlags = 3;
    zEffectAnimEntry *asyncEntry = zEffectAnim::FindNextAsyncEntry(0);
    while (asyncEntry != 0) {
        zEffectAnimEntry::SetOnStateDoneCallback(
            asyncEntry, (void *)(&AsyncCommandCallback), 0);
        asyncEntry = zEffectAnim::FindNextAsyncEntry(asyncEntry);
    }

    zReader::Node *vehicleRoot =
        zReader::LoadNodeFromPath(zVehicle::SelectZrdByDifficulty(0), 0, 0);
    const int vehicleCount = (PlayerZrdArrayCount(vehicleRoot) - 1) / 2;
    for (int vehicleIndex = 0; vehicleIndex < vehicleCount; ++vehicleIndex) {
        char vehicleName[0x14];
        strcpy(vehicleName, PlayerZrdArrayString(vehicleRoot, vehicleIndex * 2 + 1));

        PlayerMasterCommonData *const commonData = PlayerAllocMasterCommonData();
        zReader::Node *const vehicleNode = zReader_GetNamedNode(vehicleRoot, vehicleName);
        LoadMasterCommonDataFromNode(commonData, vehicleNode, vehicleName);

        for (int modalIndex = 0; modalIndex < commonData->modalCount; ++modalIndex) {
            PlayerMasterModalData *const modalData = PlayerAllocMasterModalData();
            zReader::Node *const modalNode = PlayerZrdArrayNode(vehicleNode, modalIndex * 2 + 4);
            LoadMasterModalDataFromNode(modalData, modalNode, vehicleName);
            strcpy(commonData->modalNames[modalIndex], modalData->modeName);
        }
    }

    zUtil_SaveGameState *const stealthSaveState = PlayerAllocLinkedSaveState();
    zUtil_PlayerStateStorage *const stealthPlayerState = stealthSaveState->playerState;
    memset(stealthPlayerState, 0, sizeof(*stealthPlayerState));
    PlayerModalState *const stealthModalState =
        (PlayerModalState *)zUtil_SaveGameStateList_AllocAppend(stealthSaveState);
    g_Player2SaveState = stealthSaveState;
    stealthPlayerState->rootNode = zClass_Object3D::gwObject3DInit();
    zClass_Class::gwNodeSetName(stealthPlayerState->rootNode, "Stealth");
    zClass_Object3D::gwObject3DSetPosition(stealthPlayerState->rootNode, 500.0f, 50.0f,
                                           500.0f);
    zClass_Object3D::gwObject3DSetRotation(stealthPlayerState->rootNode, 0.0f, 0.0f,
                                           0.0f);
    zClass_Class::gwNodeSetPriority(stealthPlayerState->rootNode, 1);
    zClass_Class::gwNodeSetRaycastable(stealthPlayerState->rootNode, 0);
    zClass_Class::gwNodeSetCellPickable(stealthPlayerState->rootNode, 0);
    zReader_GetNamedNode(zReader_GetNamedNode(vehicleRoot, "stealth"), "common_mode");
    InitStateFromNameAndMasterCommonData(stealthSaveState, "stealth", "stealth");
    BindModalStateFromMasterModalData(stealthSaveState, stealthModalState, "stealth", "basic");
    InitSpawnStateFromPrimaryModalData(stealthSaveState);
    stealthSaveState->firstSaveState->playerState->projectileSpawnVel.x = 0.0f;
    stealthPlayerState->cameraState = zOpt::GetCameraModePlayerState();

    zReader::Node *aivRoot = zReader::LoadNodeFromPath(GetAivZrdPath(), 0, 0);
    if (aivRoot == 0) {
        zError::ReportOld(0x800, "D:\\Proj\\Battlesport\\player.cpp", 0x399,
                          "Cannot find aiv.zrd!");
        return;
    }

    zReader::BuildResolvedParentDir(GetAivZrdPath(), g_Player_AivParentDir);
    int aivCount = (PlayerZrdArrayCount(aivRoot) - 1) / 2;
    if (zOpt::GetNetworkEnabled() != 0) {
        aivCount = 1;
    }

    for (int aivIndex = 0; aivIndex < aivCount; ++aivIndex) {
        char aivName[0x1c];
        char vehicleName[0x14];
        strcpy(aivName, PlayerZrdArrayString(aivRoot, aivIndex * 2 + 1));
        ExtractVehicleNameFromAivName(aivName, vehicleName);

        if (zReader_GetNamedNode(vehicleRoot, vehicleName) != 0) {
            zReader::Node *const aivNode = zReader_GetNamedNode(aivRoot, aivName);
            if (aivNode != 0) {
                zReader::Node *const spawnNode = PlayerZrdArrayNode(aivNode, 2);
                zVec3 spawnPos;
                spawnPos.x = PlayerZrdArrayFloat(spawnNode, 1);
                spawnPos.y = PlayerZrdArrayFloat(spawnNode, 2);
                spawnPos.z = PlayerZrdArrayFloat(spawnNode, 3);
                CreateFromNamesAtPose(&spawnPos, PlayerZrdArrayInt(aivNode, 1),
                                      PlayerZrdArrayFloat(aivNode, 3), vehicleName, aivName);
            }
        }
    }

    zReader::FreeLoadedTree(vehicleRoot);
    zReader::FreeLoadedTree(aivRoot);

    zUtil_SaveGameState *const headSaveState = g_PlayerSaveStateListHead;
    headSaveState->playerState->lifecycleState = kPlayerLifecycleInactive;
    zUtil_SaveGameState *const localSaveState = headSaveState != 0 ? headSaveState->next : 0;
    g_LocalPlayerSaveState = localSaveState;
    g_CurrentPlayerSaveState = localSaveState;
    localSaveState->playerState->cameraTickEnabled = 1;
    localSaveState->playerState->transitionDamageSuppressed = 0;
    g_VariantTag_Current = localSaveState->playerState->variantTag;
    g_Player_LastValidCameraVariantTag = localSaveState->playerState->variantTag;
    g_Variant_CurrentTag = localSaveState->playerState->variantTag;
    zEffect::SetConditionalRefPos(&localSaveState->playerState->worldPos);
    localSaveState->playerState->lifecycleState = kPlayerLifecycleLocal;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)localSaveState;

    if (zOpt::GetNetworkEnabled() != 0) {
        localSaveState->playerState->amphibUnlocked =
            IsMissionProbeType1EnabledById(g_HudSensorTracker.GetMissionId());
    } else {
        const int missionId = g_HudSensorTracker.GetMissionId();
        if (missionId == 6) {
            localSaveState->playerState->subUnlocked = 1;
        }
        if (missionId >= 4) {
            localSaveState->playerState->hoverUnlocked = 1;
        }
        if (missionId >= 3) {
            localSaveState->playerState->amphibUnlocked = 1;
        }
    }

    stealthPlayerState->worldPos = localSaveState->playerState->worldPos;
    zClass_Object3D::gwObject3DSetPosition(stealthPlayerState->rootNode,
                                           stealthPlayerState->worldPos.x,
                                           stealthPlayerState->worldPos.y,
                                           stealthPlayerState->worldPos.z);
    stealthPlayerState->vehicleRotationAngles =
        localSaveState->playerState->vehicleRotationAngles;
    zClass_Object3D::gwObject3DSetRotation(stealthPlayerState->rootNode,
                                           stealthPlayerState->vehiclePitchRad,
                                           stealthPlayerState->restartYawRad,
                                           stealthPlayerState->vehicleRollRad);

    BuildAiPeerRingsByAiNetId();
    zInput::BindMap_Current_SetCommandCallback(
        15, (zInputCommandCallbackFn)(HandlePrimaryWeaponVariantToggleInput));
    for (int commandId = 16; commandId <= 23; ++commandId) {
        zInput::BindMap_Current_SetCommandCallback(
            commandId, (zInputCommandCallbackFn)(HandleAltWeaponBankSelectInput));
    }
    RegisterGameplayCommandCallbacksAndCreateFfEffects();
    zClass_Node::MaskExtraFlagsRecursive(g_Player_RuntimeDiScene, 0);
    zReader::LoadMoversFromZrd();
    if (g_HudSensorTracker.raceCheckpointMode != 0) {
        Checkpoint::InstantiateNamedObjects();
    }
}

// Reimplements 0x42aa40: Player::GetSaveStateListHead
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE zUtil_SaveGameState *RECOIL_CDECL GetSaveStateListHead() {
    return g_PlayerSaveStateListHead;
}

// Reimplements 0x406430: Player::UnbindCurrentSaveStateIfSinglePlayer
// (D:\Proj\GameZRecoil\Player\player_camera.c)
RECOIL_NOINLINE void RECOIL_CDECL UnbindCurrentSaveStateIfSinglePlayer() {
    if (zOpt::GetNetworkEnabled() == 0) {
        g_CurrentPlayerSaveState->playerState->currentSaveStateBound = 0;
        g_CurrentPlayerSaveState = 0;
    }
}

// Reimplements 0x406450: Player::BindActiveGameStateAsCurrentSaveState
// (D:\Proj\GameZRecoil\Player\player_camera.c)
RECOIL_NOINLINE void RECOIL_CDECL BindActiveGameStateAsCurrentSaveState() {
    zUtil_SaveGameState *const activeSaveState = (zUtil_SaveGameState *)g_GameStateOrMapTable;
    activeSaveState->playerState->currentSaveStateBound = 1;
    g_CurrentPlayerSaveState = activeSaveState;
}

// Reimplements 0x42b810: Player::SyncLocalPoseFromRootNode
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_CDECL SyncLocalPoseFromRootNode() {
    zUtil_PlayerStateStorage *const playerState =
        ((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState;

    zClass_Object3D::gwObject3DGetPosition(playerState->rootNode, &playerState->worldPos.x,
                                           &playerState->worldPos.y,
                                           &playerState->worldPos.z);
    zClass_Object3D::gwObject3DGetRotation(playerState->rootNode,
                                           &playerState->vehiclePitchRad,
                                           &playerState->restartYawRad,
                                           &playerState->vehicleRollRad);
    zMath::MatBuildEulerRotation3x3(&playerState->motionBasis, playerState->vehiclePitchRad,
                                    playerState->restartYawRad,
                                    playerState->vehicleRollRad);
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;
    playerState->lifecycleState = 1;
    playerState->previousTransform = playerState->motionBasis;
}

// Reimplements 0x4390d0: Player::CacheGunHardpointsAndDetachDisplays
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
CacheGunHardpointsAndDetachDisplays(zUtil_SaveGameState *saveState, int detachDisplays) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    playerState->gunNode = zClass_Class::FindSubNodeByName(playerState->rootNode, "gun");
    if (playerState->gunNode != 0) {
        float *const gunMatrix = zClass_Object3D::gwObject3DGetMatrixPtr(playerState->gunNode);
        playerState->gunNodeMatrixPos.x = gunMatrix[9];
        playerState->gunNodeMatrixPos.y = gunMatrix[10];
        playerState->gunNodeMatrixPos.z = gunMatrix[11];
    }

    if (playerState->gunNode == 0) {
        return;
    }

    PlayerCacheGunHardpoint(playerState, "fpnt_c", &playerState->firePointCenter,
                            detachDisplays);
    PlayerCacheGunHardpoint(playerState, "fpnt_l", &playerState->firePointLeft,
                            detachDisplays);
    PlayerCacheGunHardpoint(playerState, "fpnt_r", &playerState->firePointRight,
                            detachDisplays);
}

// Reimplements 0x420d10: Player::InitStateFromNameAndMasterCommonData
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL InitStateFromNameAndMasterCommonData(
    zUtil_SaveGameState *saveState, const char *objectName,
    const char *masterCommonDataName) {
    zUtil_SaveGameState *const localSaveState = GetSaveStateListHead();
    zUtil_PlayerStateStorage *const localPlayerState =
        localSaveState != 0 ? localSaveState->playerState : 0;
    GetSaveStateListHead();

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterCommonData *commonData = g_PlayerMasterCommonDataHead;
    while (commonData != 0) {
        if (strcmp(commonData->vehicleName, masterCommonDataName) == 0) {
            playerState->masterCommonData = commonData;
            break;
        }
        commonData = commonData->next;
    }

    if (playerState->masterCommonData == 0) {
        char errorText[0x80];
        sprintf(errorText, "Cannot find Master Common Data for %s!", objectName);
        zError::ReportOld(0x800, "D:\\Proj\\Battlesport\\player.cpp", 0x46d,
                          errorText);
    }

    playerState->playerOrdinal = g_Player_NextOrdinal;
    ++g_Player_NextOrdinal;
    if (playerState->playerOrdinal == 1) {
        g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)saveState;
    }

    zClass_Object3D::gwObject3DGetPosition(playerState->rootNode, &playerState->worldPos.x,
                                           &playerState->worldPos.y,
                                           &playerState->worldPos.z);
    zClass_Object3D::gwObject3DGetRotation(playerState->rootNode,
                                           &playerState->vehiclePitchRad,
                                           &playerState->restartYawRad,
                                           &playerState->vehicleRollRad);
    playerState->pitchPoseCache = playerState->vehiclePitchRad;
    playerState->yawPoseCache = playerState->restartYawRad;
    playerState->rollPoseCache = playerState->vehicleRollRad;
    playerState->angVelPitch = 0.0f;
    playerState->angVelYaw = 0.0f;
    playerState->angVelRoll = 0.0f;

    zMath::MatBuildEulerRotation3x3(&playerState->motionBasis, playerState->vehiclePitchRad,
                                    playerState->restartYawRad,
                                    playerState->vehicleRollRad);
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;
    playerState->previousTransform = playerState->motionBasis;
    RebuildSteerBasisFromMotionBasis(saveState);
    playerState->cameraDirFlat = playerState->steerBasisNorm;

    AINet *aiNet = 0;
    if (playerState->aiNetId != 0) {
        aiNet = AINet::FindByNetId(playerState->aiNetId);
    }
    playerState->lifecycleState = aiNet != 0 ? kPlayerLifecycleAi : kPlayerLifecycleInactive;
    if (playerState->lifecycleState == kPlayerLifecycleAi) {
        playerState->aiNet = aiNet;
        switch (aiNet->aiType) {
        case AINET_TYPE_ST:
            playerState->aiTopLevelState = kPlayerAiTopPathFollow;
            break;
        case AINET_TYPE_HI:
            playerState->aiTopLevelState = kPlayerAiTopTurnTowardTarget;
            break;
        case AINET_TYPE_FI:
            playerState->aiTopLevelState = kPlayerAiTopTurnOnlyTowardTarget;
            break;
        case AINET_TYPE_DE:
            playerState->aiTopLevelState = kPlayerAiTopPathSteering;
            break;
        }

        playerState->aiCurrentSteeringSubstate = aiNet->attackStrategy;
        playerState->aiHideTime0 = aiNet->hideTime0;
        playerState->aiHideTime1 = aiNet->hideTime1;
        playerState->aiCurrentPathNode =
            AINet::FindNearestNode(&playerState->worldPos, aiNet->nodeListHead);
        playerState->aiHomePathNode = playerState->aiCurrentPathNode;

        zClass_NodePartial *const healthyNode =
            zClass_Class::FindSubNodeByName(playerState->rootNode, "healthy");
        if (healthyNode != 0) {
            zClass_Class::gwNodeSetCellPickable(healthyNode, 0);
        }

        if (aiNet->activateRadius != 0.0f) {
            playerState->aiActivationRadiusSq =
                aiNet->activateRadius * aiNet->activateRadius;
        }
        if (aiNet->attackRadius != 0.0f) {
            playerState->aiAttackRadiusSq = aiNet->attackRadius * aiNet->attackRadius;
        } else {
            playerState->aiAttackRadiusSq = kPlayerDefaultAiAttackRadiusSq;
        }
        if (aiNet->attackDwell != 0.0f) {
            playerState->aiMode2AttackDwell = aiNet->attackDwell;
        } else {
            playerState->aiMode2AttackDwell = kPlayerDefaultAiAttackDwellTime;
        }
        if (aiNet->notPursuitDwell != 0.0f) {
            playerState->aiNotPursuitDwell = aiNet->notPursuitDwell;
        }
        if (aiNet->returnRange != 0.0f) {
            playerState->aiRestoreDistanceSq = aiNet->returnRange * aiNet->returnRange;
        }

        saveState->aiPeerRingNext = saveState;
        playerState->aiStateUntilTime =
            g_Time_AccumulatedTimeSec + kPlayerAiInitialStateDelaySec;
        playerState->aiStateStartTime = playerState->aiStateUntilTime;
    }

    playerState->regenSkinFxEntry = zEffectAnim::FindEntryByName("regen_skin");
    playerState->masterTypeTransitionToAmphibNodeAction =
        zEffectAnim::FindEntryByName("boat_wake_trail");
    playerState->masterTypeTransitionToTrackNodeAction =
        zEffectAnim::FindEntryByName("bft_exhaust_trail");
    playerState->shatterVehicleFxEntry = zEffectAnim::FindEntryByName("shatter_vehicle");
    playerState->shockVehicleFxEntry = zEffectAnim::FindEntryByName("shock_vehicle");
    playerState->napalmVehicleFxEntry = zEffectAnim::FindEntryByName("napalm_vehicle");
    playerState->masterTypeTransitionToSubNodeAction =
        zEffectAnim::FindEntryByName("bft_bubble1");
    playerState->subTransitionFxEntry = zEffectAnim::FindEntryByName("subt");

    const int objectIsNetwork = strstr(objectName, "net") != 0;
    playerState->destroyedRespawnFxEntry =
        zEffectAnim::FindEntryByName(objectIsNetwork != 0 ? "bft_00" : objectName);
    if (objectIsNetwork != 0 || strstr(objectName, "bft") != 0) {
        playerState->masterTypeTransitionToTrackLightHandle =
            zEffectAnim::SetVelocity_Thunk(
                playerState->masterTypeTransitionToTrackNodeAction, playerState->rootNode,
                0.0f, 0.0f, 0.0f);
    }
    zEffectAnim::SetVelocity_Thunk(
        zEffectAnim::FindEntryByName(commonData->startAnimsName), playerState->rootNode,
        0.0f, 0.0f, 0.0f);

    playerState->cameraState = zOpt::GetCameraModePlayerState();
    playerState->cameraLerpActive = 0;
    playerState->thirdPersonYawOffset = 0.0f;
    playerState->thirdPersonSideOffset = commonData->cambackSide0;
    playerState->thirdPersonBaseYOffset = commonData->cambackBase0;
    playerState->cameraDistance = commonData->cambackDist0;
    playerState->cameraConfigParam0 = commonData->cambackSide1;
    playerState->cameraConfigParam1 = commonData->cambackBase1;
    playerState->cameraConfigParam2 = commonData->cambackDist1;
    playerState->cameraConfigParam3 = commonData->cambackSide2;
    playerState->cameraConfigParam4 = commonData->cambackBase2;
    playerState->cameraConfigParam5 = commonData->cambackDist2;
    playerState->cameraYOffset = commonData->aimYawRate;
    playerState->cameraYOffset = commonData->aimYawMax;
    playerState->cameraState2TargetOffset = zVec3_Make(0.0f, kPlayerCameraState2TargetYOffset,
                                                       0.0f);
    playerState->unknown_00d4 = 0;
    playerState->unknown_00d8 = 0;
    playerState->unknown_00dc = 0;
    playerState->unknown_00e0 = 0;
    playerState->altGunAimOrigin = zVec3_Make(0.0f, 0.0f, kPlayerDefaultAltGunAimOriginZ);
    playerState->activeAltBankIndex = 1;
    playerState->autoTurnActive = 0;
    playerState->cameraTransitionTimer = 0;
    playerState->cameraTransitionBlend = 1.0f;

    zClass_NodePartial *const targetNode =
        zClass_Class::FindSubNodeByName(playerState->rootNode, "target");
    if (targetNode != 0) {
        zClass_Object3D::gwObject3DGetPosition(targetNode, &playerState->fxOffsetLocal.x,
                                               &playerState->fxOffsetLocal.y,
                                               &playerState->fxOffsetLocal.z);
        zClass_Class::gwNodeSetActive(targetNode, 0);
    } else {
        playerState->fxOffsetLocal = zVec3_Make(0.0f, 0.0f, 0.0f);
    }
    playerState->fxOffsetWorld.x = playerState->worldPos.x + playerState->fxOffsetLocal.x;
    playerState->fxOffsetWorld.y = playerState->worldPos.y + playerState->fxOffsetLocal.y;
    playerState->fxOffsetWorld.z = playerState->worldPos.z + playerState->fxOffsetLocal.z;

    playerState->bodyNode = zClass_Class::FindSubNodeByName(playerState->rootNode, "body");
    playerState->turretNode = zClass_Class::FindSubNodeByName(playerState->rootNode, "turret");
    playerState->doorLeftNode = zClass_Class::FindSubNodeByName(playerState->rootNode, "doorleft");
    playerState->doorRightNode =
        zClass_Class::FindSubNodeByName(playerState->rootNode, "doorright");
    playerState->modeVariantNode =
        zClass_Class::FindSubNodeByName(playerState->rootNode, "shadow");

    CacheGunHardpointsAndDetachDisplays(saveState, 1);

    playerState->statusMeterValue = commonData->maxHealth;
    playerState->statusMeterScaled = 1.0f;
    playerState->damageProtectionActive = 0;
    playerState->queuedFixedDamageFlag = 0;
    playerState->recentHitValid = 0;
    playerState->recentHitLightHandle = 0;
    playerState->nanitePanelLevel = 0;

    if (playerState != localPlayerState) {
        HudUiMgrSensorTrackNode *const context =
            HudUiMgrSensor::TrackList_Add(HUD_SENSOR_TRACK_KIND_PLAYER, saveState);
        zClass_Node::SetContextRecursive(playerState->rootNode,
                                         (zClass_NodePartial *)context, 0x100000);
    }

    LoadWeaponBanksAndSelectDefaults(saveState);
}

// Reimplements 0x403750: Player::BuildAiPeerRingsByAiNetId
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_CDECL BuildAiPeerRingsByAiNetId() {
    zUtil_SaveGameState *saveState = g_PlayerSaveStateListHead;
    while (saveState != 0) {
        const int aiNetId = saveState->playerState->aiNetId;
        zUtil_SaveGameState *candidate = saveState->next;
        while (candidate != 0) {
            zUtil_PlayerStateStorage *const candidatePlayerState = candidate->playerState;
            if (candidatePlayerState->aiNetId == aiNetId &&
                candidatePlayerState->lifecycleState != kPlayerLifecycleInactive &&
                candidate->aiPeerRingNext == candidate) {
                candidate->aiPeerRingNext = saveState->aiPeerRingNext;
                saveState->aiPeerRingNext = candidate;
            }
            candidate = candidate->next;
        }
        saveState = saveState->next;
    }
}

// Reimplements 0x421470: Player::BindModalStateFromMasterModalData
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
BindModalStateFromMasterModalData(zUtil_SaveGameState *saveState, PlayerModalState *modalState,
                                  const char *modalName, const char *objectName) {
    GetSaveStateListHead();
    GetSaveStateListHead();

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *masterModalData = g_PlayerMasterModalDataHead;
    while (masterModalData != 0) {
        if (strcmp(masterModalData->modalName, playerState->masterCommonData->vehicleName) == 0 &&
            strcmp(modalName, masterModalData->modeName) == 0) {
            modalState->masterModalData = masterModalData;
            break;
        }
        masterModalData = masterModalData->next;
    }

    if (modalState->masterModalData == 0) {
        char errorText[0x100];
        sprintf(errorText, "Cannot find Master Modal Data for %s!", objectName);
        zError::ReportOld(0x800, "D:\\Proj\\Battlesport\\player.cpp", 0x5c3,
                          errorText);
    }

    zClass_NodePartial *const rootNode = playerState->rootNode;
    modalState->nodeRightMorphs = zClass_Class::FindSubNodeByName(rootNode, "right_morphs");
    modalState->nodeLeftMorphs = zClass_Class::FindSubNodeByName(rootNode, "left_morphs");
    modalState->modalNode = zClass_Class::FindSubNodeByName(rootNode, "chassis");
    modalState->nodeRTracks = zClass_Class::FindSubNodeByName(rootNode, "rtracks");
    modalState->nodeLTracks = zClass_Class::FindSubNodeByName(rootNode, "ltracks");
    modalState->nodeProps = zClass_Class::FindSubNodeByName(rootNode, "props");
    modalState->nodeCaustic1 = zClass_Class::FindSubNodeByName(rootNode, "caustic1");
    modalState->nodeWake = zClass_Class::FindSubNodeByName(rootNode, "wake");
    modalState->nodeSplashL = zClass_Class::FindSubNodeByName(rootNode, "splash_l");
    modalState->nodeSplashR = zClass_Class::FindSubNodeByName(rootNode, "splash_r");
    modalState->nodeDustL = zClass_Class::FindSubNodeByName(rootNode, "dust_l");
    modalState->nodeDustR = zClass_Class::FindSubNodeByName(rootNode, "dust_r");

    modalState->chassisRollFilterState = 0.0f;
    modalState->chassisPitchFilterState = 0.0f;
    modalState->modalStateCode = 4;

    if (BuildSupportPointsFromModel(saveState, rootNode) == 0 &&
        masterModalData->platformPointCount == 0) {
        char errorText[0x100];
        sprintf(errorText, "Cannot find support points for %s!", objectName);
        zError::ReportOld(0x800, "D:\\Proj\\Battlesport\\player.cpp", 0x5df,
                          errorText);
    }

    if (masterModalData->probePointCount == 0 &&
        BuildCollisionPointsFromModel(saveState, rootNode) == 0) {
        char errorText[0x100];
        sprintf(errorText, "Cannot find collision points for %s!", objectName);
        zError::ReportOld(0x800, "D:\\Proj\\Battlesport\\player.cpp", 0x5e6,
                          errorText);
    }
}

// Reimplements 0x421790: Player::InitSpawnStateFromPrimaryModalData
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
InitSpawnStateFromPrimaryModalData(zUtil_SaveGameState *saveState) {
    GetSaveStateListHead();
    GetSaveStateListHead();

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;

    playerState->spawnStateInitialized = 0;
    playerState->primaryGunGateUntilTime = 0.0f;
    playerState->gravityAccel = g_Player_NominalGravity;
    playerState->primaryFireSlotIndex = 0;
    playerState->altFireSlotIndex = 0;

    for (int i = 0; i < masterModalData->probePointCount; ++i) {
        playerState->rootProbeWorldByIndex[i].x =
            masterModalData->probePoints[i].x + playerState->worldPos.x;
        playerState->rootProbeWorldByIndex[i].y =
            masterModalData->probePoints[i].y + playerState->worldPos.y;
        playerState->rootProbeWorldByIndex[i].z =
            masterModalData->probePoints[i].z + playerState->worldPos.z;
    }

    SampleGroundAndAlignRootToSurface(saveState, 1);
}

// Reimplements 0x421830: Player::SampleGroundAndAlignRootToSurface
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
SampleGroundAndAlignRootToSurface(zUtil_SaveGameState *saveState, int updateRotation) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    zTag4::Clear(&playerState->variantTag);
    g_Variant_CurrentTag = playerState->variantTag;
    zClass_Class::gwNodeSetNodeType(playerState->rootNode, playerState->variantTag.tags[0]);
    zClass_Class::gwNodeSetCellPickable(playerState->rootNode, 0);

    PlayerProbeSampleCandidateBuffer candidateBuffer = {};
    zClass_cls_di::BuildPickCandidateListBelowPoint(
        g_Player_RuntimeDiScene, &candidateBuffer, playerState->worldPos.x, 500.0f,
        playerState->worldPos.z);

    int bestCandidateIndex = 0;
    int selectedImpactSlot = 0;
    float taggedHeight = -300.0f;
    SelectProbeSampleHeightFromCandidates(
        &candidateBuffer, &bestCandidateIndex, playerState->worldPos.y, 4.0f,
        playerState->amphibUnlocked == 0, &selectedImpactSlot, &taggedHeight);

    zClass_Class::gwNodeSetCellPickable(playerState->rootNode, 1);

    if (candidateBuffer.candidateCount <= 0) {
        zClass_Class::gwNodeSetNodeType(playerState->rootNode, 0xff);
        return;
    }

    zClassDiPickCandidateEntry *const selectedCandidate =
        &candidateBuffer.entries[bestCandidateIndex];
    playerState->variantTag = selectedCandidate->variantTag;

    zClass_NodePartial *const worldChild =
        zClass_Class::gwNodeGetWorldChild(selectedCandidate->node);
    const int nodeType =
        worldChild != 0 ? worldChild->nodeType : selectedCandidate->variantTag.tags[0];
    zClass_Class::gwNodeSetNodeType(playerState->rootNode, nodeType);

    if (updateRotation == 0) {
        return;
    }

    playerState->steerBasisRef = selectedCandidate->surfaceNormal;
    zVec3 yawRelativeNormal = selectedCandidate->surfaceNormal;
    RebuildSteerBasisRawFromRef(saveState);
    zMath::Vec3RotateY(&yawRelativeNormal, &playerState->steerBasisRef,
                       -playerState->restartYawRad);

    const float pitchAngleRad = static_cast<float>(asin(yawRelativeNormal.z));
    float clampedPitchAngleRad = pitchAngleRad;
    if (clampedPitchAngleRad > 0.523599982f) {
        clampedPitchAngleRad = 0.523599982f;
    } else if (clampedPitchAngleRad < -0.523599982f) {
        clampedPitchAngleRad = -0.523599982f;
    }

    const float rollAngleRad = static_cast<float>(asin(-yawRelativeNormal.x));
    playerState->vehiclePitchRad = clampedPitchAngleRad;
    playerState->vehicleRollRad = rollAngleRad;
    zClass_Object3D::gwObject3DSetRotation(playerState->rootNode,
                                           playerState->vehiclePitchRad,
                                           playerState->restartYawRad, rollAngleRad);
}

// Reimplements 0x421ed0: Player::BuildCollisionPointsFromModel
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
BuildCollisionPointsFromModel(zUtil_SaveGameState *saveState, zClass_NodePartial *modelNode) {
    PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;
    zVec3 collisionPoints[12];

    for (int i = 0; i < 12; ++i) {
        char nodeName[0x50];
        sprintf(nodeName, "collide%02d", i);
        zClass_NodePartial *const collisionNode =
            zClass_Class::FindSubNodeByName(modelNode, nodeName);
        if (collisionNode == 0) {
            return 0;
        }

        zClass_Object3D::gwObject3DGetPosition(collisionNode, &collisionPoints[i].x,
                                               &collisionPoints[i].y,
                                               &collisionPoints[i].z);
        zClass_Class::gwNodeSetActive(collisionNode, 0);
    }

    masterModalData->probePoints[0] = collisionPoints[0];
    masterModalData->probePoints[1] = collisionPoints[1];
    masterModalData->probePoints[2] = collisionPoints[2];
    masterModalData->probePoints[3] = collisionPoints[6];
    masterModalData->probePoints[4] = collisionPoints[7];
    masterModalData->probePoints[5] = collisionPoints[8];
    masterModalData->probePoints[6] = collisionPoints[3];
    masterModalData->probePoints[7] = collisionPoints[4];
    masterModalData->probePoints[8] = collisionPoints[5];
    masterModalData->probePoints[9] = collisionPoints[9];
    masterModalData->probePoints[10] = collisionPoints[10];
    masterModalData->probePoints[11] = collisionPoints[11];
    masterModalData->probePointCount = 12;
    return 1;
}

// Reimplements 0x4220f0: Player::BuildSupportPointsFromModel
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
BuildSupportPointsFromModel(zUtil_SaveGameState *saveState, zClass_NodePartial *modelNode) {
    PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;

    for (int i = 0; i < 4; ++i) {
        char nodeName[0x50];
        sprintf(nodeName, "support%02d", i);
        zClass_NodePartial *const supportNode =
            zClass_Class::FindSubNodeByName(modelNode, nodeName);
        if (supportNode == 0) {
            return 0;
        }

        zVec3 *const supportPoint = &masterModalData->probePoints[15 + i];
        zClass_Object3D::gwObject3DGetPosition(supportNode, &supportPoint->x,
                                               &supportPoint->y, &supportPoint->z);
        zClass_Class::gwNodeSetActive(supportNode, 0);
    }

    return 1;
}

// Reimplements 0x422170: Player::LoadMasterCommonDataFromNode
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
LoadMasterCommonDataFromNode(PlayerMasterCommonData *commonData, zReader::Node *vehicleNode,
                             const char *vehicleName) {
    strcpy(commonData->vehicleName, vehicleName);

    commonData->modalCount = ((PlayerZrdArrayCount(vehicleNode) - 1) / 2) - 1;

    zReader::Node *const commonModeNode = zReader_GetNamedNode(vehicleNode, "common_mode");
    zReader::Node *node = zReader_GetNamedNode(commonModeNode, "nanite");
    if (node != 0) {
        commonData->naniteBuildRate = PlayerZrdArrayInt(node, 1);
        commonData->naniteMaxLevel = PlayerZrdArrayInt(node, 2);
    } else {
        commonData->naniteBuildRate = 0;
        commonData->naniteMaxLevel = 0;
    }

    zReader::Node *const soundsNode = zReader_GetNamedNode(commonModeNode, "sounds");
    if (soundsNode != 0) {
        PlayerLoadSoundSample(soundsNode, "weapon_up", &commonData->sfxWeaponUp[0]);
        PlayerLoadSoundSample(soundsNode, "weapon_select", &commonData->sfxWeaponUp[2]);
        PlayerLoadSoundSample(soundsNode, "pinging", &commonData->sfxWeaponUp[3]);
    }

    node = zReader_GetNamedNode(commonModeNode, "activation");
    if (node != 0) {
        const float activationRange = PlayerZrdArrayFloat(node, 1);
        commonData->activationRangeSq = activationRange * activationRange;
    } else {
        commonData->activationRangeSq =
            kPlayerDefaultActivationRange * kPlayerDefaultActivationRange;
    }

    node = zReader_GetNamedNode(commonModeNode, "not_pursuit_dwell");
    commonData->notPursuitDwellTime =
        node != 0 ? PlayerZrdArrayFloat(node, 1) : kPlayerDefaultNotPursuitDwellTime;

    node = zReader_GetNamedNode(commonModeNode, "return_range");
    if (node != 0) {
        const float returnRange = PlayerZrdArrayFloat(node, 1);
        commonData->returnRangeSq = returnRange * returnRange;
    } else {
        commonData->returnRangeSq = kPlayerDefaultReturnRange * kPlayerDefaultReturnRange;
    }

    node = zReader_GetNamedNode(commonModeNode, "start_anims");
    if (node != 0) {
        PlayerCopyZrdArrayString(commonData->startAnimsName, node, 1);
    }

    node = zReader_GetNamedNode(commonModeNode, "camback");
    if (node != 0) {
        zReader::Node *const first = PlayerZrdArrayBase(node)[1].value.nodes;
        zReader::Node *const second = PlayerZrdArrayBase(node)[2].value.nodes;
        zReader::Node *const third = PlayerZrdArrayBase(node)[3].value.nodes;
        commonData->cambackSide0 = first[1].value.f32;
        commonData->cambackBase0 = first[2].value.f32;
        commonData->cambackDist0 = first[3].value.f32;
        commonData->cambackSide1 = second[1].value.f32;
        commonData->cambackBase1 = second[2].value.f32;
        commonData->cambackDist1 = second[3].value.f32;
        commonData->cambackSide2 = third[1].value.f32;
        commonData->cambackBase2 = third[2].value.f32;
        commonData->cambackDist2 = third[3].value.f32;
    } else {
        commonData->cambackSide0 = 0.0f;
        commonData->cambackBase0 = 4.0f;
        commonData->cambackDist0 = 9.0f;
        commonData->cambackSide1 = 0.0f;
        commonData->cambackBase1 = 3.5f;
        commonData->cambackDist1 = 2.25f;
        commonData->cambackSide2 = 0.0f;
        commonData->cambackBase2 = 2.25f;
        commonData->cambackDist2 = 2.25f;
    }

    node = zReader_GetNamedNode(commonModeNode, "aimy");
    if (node != 0) {
        commonData->aimYawRate = PlayerZrdArrayFloat(node, 1);
        commonData->aimYawMax = PlayerZrdArrayFloat(node, 2);
    } else {
        commonData->aimYawRate = 3.0f;
        commonData->aimYawMax = 2.0f;
    }

    node = zReader_GetNamedNode(commonModeNode, "camera_ud_swing");
    if (node != 0) {
        commonData->cameraUdSwing[0] = PlayerZrdArrayFloat(node, 1);
        commonData->cameraUdSwing[1] = PlayerZrdArrayFloat(node, 2);
        commonData->cameraUdSwing[2] = PlayerZrdArrayFloat(node, 3);
        commonData->cameraUdSwing[3] = PlayerZrdArrayFloat(node, 4);
    } else {
        commonData->cameraUdSwing[0] = 5.5f;
        commonData->cameraUdSwing[1] = 2.5f;
        commonData->cameraUdSwing[2] = 0.0f;
        commonData->cameraUdSwing[3] = 0.0f;
    }

    node = zReader_GetNamedNode(commonModeNode, "track_switch");
    if (node != 0) {
        commonData->trackSwitchDist0 = PlayerZrdArrayFloat(node, 1);
        commonData->trackSwitchDist1 = PlayerZrdArrayFloat(node, 2);
        commonData->trackSwitchDist2 = PlayerZrdArrayFloat(node, 3);
    } else {
        commonData->trackSwitchDist0 = 10000.0f;
        commonData->trackSwitchDist1 = 10000.0f;
        commonData->trackSwitchDist2 = 10000.0f;
    }

    node = zReader_GetNamedNode(commonModeNode, "health");
    if (node != 0) {
        commonData->maxHealth =
            zOpt::GetNetworkEnabled() != 0 ? PlayerZrdArrayFloat(node, 2)
                                           : PlayerZrdArrayFloat(node, 1);
    } else {
        commonData->maxHealth = kPlayerDefaultMaxHealth;
    }
    commonData->invMaxHealth = 1.0f / commonData->maxHealth;

    node = zReader_GetNamedNode(commonModeNode, "pickups");
    if (node != 0) {
        PickupType::FindByLogicalName(PlayerZrdArrayString(node, 1), &commonData->pickupType);
        commonData->pickupCapacity = PlayerZrdArrayInt(node, 2);
    } else {
        commonData->pickupType = 0;
        commonData->pickupCapacity = 0;
    }

    node = zReader_GetNamedNode(commonModeNode, "weapons");
    if (node == 0) {
        return;
    }

    commonData->weaponNodeCount = PlayerZrdArrayCount(node) - 1;
    if (commonData->weaponNodeCount <= 0) {
        return;
    }

    for (int index = 0; index < commonData->weaponNodeCount; ++index) {
        PlayerMasterWeaponSpec *const weaponSpec =
            static_cast<PlayerMasterWeaponSpec *>(::operator new(sizeof(PlayerMasterWeaponSpec)));
        memset(weaponSpec, 0, sizeof(PlayerMasterWeaponSpec));
        if (commonData->weaponSpecCount == 0) {
            commonData->weaponSpecHead = weaponSpec;
        } else {
            commonData->weaponSpecTail->next = weaponSpec;
        }
        commonData->weaponSpecTail = weaponSpec;
        weaponSpec->next = 0;
        ++commonData->weaponSpecCount;

        zReader::Node *const weaponFields = PlayerZrdArrayBase(node)[index + 1].value.nodes;
        strcpy(weaponSpec->optCatalogName, weaponFields[1].value.str);
        weaponSpec->missionRequirementOrGateId = weaponFields[2].value.i32;
        weaponSpec->mountLayoutFlags = weaponFields[3].value.i32;
        weaponSpec->startAmmoOrCharge = static_cast<float>(weaponFields[4].value.i32);
        weaponSpec->dispatchRepeatDelay = weaponFields[5].value.f32;
        weaponSpec->aiAttackRangeMin = weaponFields[6].value.f32;
        weaponSpec->aiAttackRangeMax = weaponFields[7].value.f32;
        weaponSpec->fireSlotRecoilFlags = weaponFields[8].value.i32;
        weaponSpec->initialHardpointSelectState = weaponFields[9].value.i32;
    }
}

// Reimplements 0x4226d0: Player::LoadMasterModalDataFromNode
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
LoadMasterModalDataFromNode(PlayerMasterModalData *modalData, zReader::Node *modalNode,
                            const char *modalName) {
    strcpy(modalData->modalName, modalName);

    zReader::Node *node = zReader_GetNamedNode(modalNode, "mode");
    if (node != 0) {
        PlayerCopyZrdArrayString(modalData->modeName, node, 1);
    } else {
        strcpy(modalData->modeName, "unknown");
    }

    modalData->masterType = 0;
    if (strncmp(modalData->modeName, "basic", 2) == 0) {
        modalData->masterType = 0;
    } else if (strncmp(modalData->modeName, "track", 2) == 0) {
        modalData->masterType = 3;
    } else if (strncmp(modalData->modeName, "hover", 2) == 0) {
        modalData->masterType = 4;
    } else if (strncmp(modalData->modeName, "amphib", 2) == 0) {
        modalData->masterType = 5;
    } else if (strncmp(modalData->modeName, "sub", 2) == 0) {
        modalData->masterType = 2;
    } else if (strncmp(modalData->modeName, "fly", 2) == 0) {
        modalData->masterType = 1;
    }

    PlayerLoadModalPointList(zReader_GetNamedNode(modalNode, "platform"),
                             &modalData->probePoints[15], &modalData->platformPointCount);
    PlayerLoadModalPointList(zReader_GetNamedNode(modalNode, "collision"), modalData->probePoints,
                             &modalData->probePointCount);

    node = zReader_GetNamedNode(modalNode, "rates");
    if (node != 0) {
        modalData->accelRate = PlayerZrdArrayFloat(node, 1);
        modalData->maxSpeed = PlayerZrdArrayFloat(node, 2);
    } else {
        modalData->accelRate = 10.0f;
        modalData->maxSpeed = 30.0f;
    }

    node = zReader_GetNamedNode(modalNode, "friction");
    if (node != 0) {
        modalData->frictionStatic = PlayerZrdArrayFloat(node, 1);
        modalData->frictionDynamic = PlayerZrdArrayFloat(node, 2);
        modalData->frictionSlide = PlayerZrdArrayFloat(node, 3);
    } else {
        modalData->frictionStatic = 10000.0f;
        modalData->frictionDynamic = 10.0f;
        modalData->frictionSlide = 0.0f;
    }
    if (modalData->frictionDynamic >= modalData->frictionStatic) {
        modalData->frictionDynamic = modalData->frictionStatic * 0.899999976f;
    }

    node = zReader_GetNamedNode(modalNode, "stopping");
    modalData->stoppingForce = node != 0 ? PlayerZrdArrayFloat(node, 1) : 8.0f;

    node = zReader_GetNamedNode(modalNode, "quicksand_slowdown");
    modalData->quicksandSlowdown = node != 0 ? PlayerZrdArrayFloat(node, 1) : 0.899999976f;

    node = zReader_GetNamedNode(modalNode, "lava_slowdown");
    modalData->lavaSlowdown = node != 0 ? PlayerZrdArrayFloat(node, 1) : 0.800000012f;

    node = zReader_GetNamedNode(modalNode, "turns");
    if (node != 0) {
        modalData->yawAccel = PlayerZrdArrayFloat(node, 1);
        modalData->yawRateMax = PlayerZrdArrayFloat(node, 2);
    } else {
        modalData->yawAccel = 0.600000024f;
        modalData->yawRateMax = 2.0f;
    }

    node = zReader_GetNamedNode(modalNode, "turn_damping");
    modalData->yawDamping = node != 0 ? PlayerZrdArrayFloat(node, 1) : 30.0f;

    node = zReader_GetNamedNode(modalNode, "rate_damping");
    if (node != 0) {
        modalData->rateDampingAccel = PlayerZrdArrayFloat(node, 1);
        modalData->rateDampingDecel = PlayerZrdArrayFloat(node, 2);
    } else {
        modalData->rateDampingAccel = 30.0f;
        modalData->rateDampingDecel = 30.0f;
    }

    node = zReader_GetNamedNode(modalNode, "a_damping");
    modalData->aDamping = node != 0 ? PlayerZrdArrayFloat(node, 1) : 8.0f;

    node = zReader_GetNamedNode(modalNode, "alt_control");
    if (node != 0) {
        modalData->hoverLiftDampingRate = PlayerZrdArrayFloat(node, 1);
        modalData->hoverLiftScale = PlayerZrdArrayFloat(node, 2);
        modalData->hoverNormalLerpRate = PlayerZrdArrayFloat(node, 3);
    } else {
        modalData->hoverLiftDampingRate = -10.0f;
        modalData->hoverLiftScale = 0.800000012f;
        modalData->hoverNormalLerpRate = -3.0f;
    }

    node = zReader_GetNamedNode(modalNode, "mass");
    modalData->mass = node != 0 ? PlayerZrdArrayFloat(node, 1) : 1.0f;
    modalData->invMass = 1.0f / modalData->mass;

    node = zReader_GetNamedNode(modalNode, "gun_pitch");
    if (node != 0) {
        modalData->gunPitchMin = PlayerZrdArrayFloat(node, 1);
        modalData->gunPitchRate = PlayerZrdArrayFloat(node, 2);
    } else {
        modalData->gunPitchMin = -0.2588f;
        modalData->gunPitchRate = 0.5f;
    }

    node = zReader_GetNamedNode(modalNode, "mode_alt");
    modalData->modeAltTransitionTime = node != 0 ? PlayerZrdArrayFloat(node, 1) : 2.0f;

    node = zReader_GetNamedNode(modalNode, "chas_smooth");
    modalData->chassisSmoothFactor =
        node != 0 ? static_cast<float>(fabs(PlayerZrdArrayFloat(node, 1))) : 0.0f;

    node = zReader_GetNamedNode(modalNode, "chas_pitch");
    if (node != 0) {
        modalData->chassisPitchRate = PlayerZrdArrayFloat(node, 1);
        modalData->chassisPitchMax = PlayerZrdArrayFloat(node, 2);
        modalData->chassisPitchDamping =
            static_cast<float>(fabs(PlayerZrdArrayFloat(node, 3)));
    } else {
        modalData->chassisPitchRate = 0.0f;
        modalData->chassisPitchMax = 0.0f;
        modalData->chassisPitchDamping = 0.0f;
    }

    node = zReader_GetNamedNode(modalNode, "chas_roll");
    if (node != 0) {
        modalData->chassisRollRate = PlayerZrdArrayFloat(node, 1);
        modalData->chassisRollMax = PlayerZrdArrayFloat(node, 2);
        modalData->chassisRollDamping = static_cast<float>(fabs(PlayerZrdArrayFloat(node, 3)));
    } else {
        // Retail code clears the pitch slots here when chas_roll is absent.
        modalData->chassisPitchRate = 0.0f;
        modalData->chassisPitchMax = 0.0f;
        modalData->chassisPitchDamping = 0.0f;
    }

    PlayerLoadModalWaveParams(modalData, modalNode, "amphib_wave");
    PlayerLoadModalWaveParams(modalData, modalNode, "hover_wave");
    PlayerLoadModalWaveParams(modalData, modalNode, "sub_wave");

    node = zReader_GetNamedNode(modalNode, "collision_d");
    if (node != 0) {
        modalData->collisionDampingA = PlayerZrdArrayFloat(node, 1);
        modalData->collisionDampingB = PlayerZrdArrayFloat(node, 2);
    } else {
        modalData->collisionDampingA = 0.5f;
        modalData->collisionDampingB = 0.150000006f;
    }

    PlayerLoadModalFxList(modalNode, "t2a_anims", modalData->fxList_fromTrackToAmphib);
    PlayerLoadModalFxList(modalNode, "a2t_anims", modalData->fxList_fromAmphibToTrack);
    PlayerLoadModalFxList(modalNode, "t2h_anims", modalData->fxList_fromTrackToHover);
    PlayerLoadModalFxList(modalNode, "h2t_anims", modalData->fxList_fromHoverToTrack);
    PlayerLoadModalFxList(modalNode, "s2a_anims", modalData->fxList_fromSubToAmphib);
    PlayerLoadModalFxList(modalNode, "a2s_anims", modalData->fxList_fromAmphibToSub);
    PlayerLoadModalFxList(modalNode, "h2a_anims", modalData->fxList_fromHoverToAmphib);
    PlayerLoadModalFxList(modalNode, "a2h_anims", modalData->fxList_fromAmphibToHover);

    zReader::Node *const soundsNode = zReader_GetNamedNode(modalNode, "sounds");
    if (soundsNode == 0) {
        return;
    }

    PlayerLoadSoundSample(soundsNode, "engine", &modalData->sfxEngine[0]);
    PlayerLoadSoundSample(soundsNode, "external", &modalData->sfxEngine[1]);
    PlayerLoadSoundSample(soundsNode, "idle", &modalData->sfxEngine[2]);
    PlayerLoadSoundSample(soundsNode, "skid", &modalData->sfxEngine[3]);
    PlayerLoadSoundSample(soundsNode, "collide", &modalData->sfxCollide);
    PlayerLoadSoundSample(soundsNode, "land", &modalData->sfxLand);

    node = zReader_GetNamedNode(soundsNode, "pitch_scale");
    if (node != 0) {
        modalData->sfxPitchScale = PlayerZrdArrayFloat(node, 1);
    }

    node = zReader_GetNamedNode(soundsNode, "volume_scale");
    if (node != 0) {
        modalData->sfxVolumeScale = PlayerZrdArrayFloat(node, 1);
    }
}

// Reimplements 0x42b630: Player::CacheDisableCopterSndNodesAndStopSample
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_CDECL CacheDisableCopterSndNodesAndStopSample() {
    if (g_Player_CopterSndNode1 == 0) {
        zClass_NodePartial *const copterRoot = zClass::FindByTypeAndName(6, "copter01");
        if (copterRoot != 0) {
            g_Player_CopterHealthyNode1 =
                zClass_Class::FindSubNodeByName(copterRoot, "healthy");
            g_Player_CopterSndNode1 =
                zClass_Class::FindSubNodeByName(copterRoot, "snd_chopper");
        }
    }

    if (g_Player_CopterSndNode2 == 0) {
        zClass_NodePartial *const copterRoot = zClass::FindByTypeAndName(6, "copter02");
        if (copterRoot != 0) {
            g_Player_CopterHealthyNode2 =
                zClass_Class::FindSubNodeByName(copterRoot, "healthy");
            g_Player_CopterSndNode2 =
                zClass_Class::FindSubNodeByName(copterRoot, "snd_chopper");
        }
    }

    if (g_Player_CopterSndNode1 != 0) {
        zClass_Class::gwNodeSetActive(g_Player_CopterSndNode1, 0);
    }
    if (g_Player_CopterSndNode2 != 0) {
        zClass_Class::gwNodeSetActive(g_Player_CopterSndNode2, 0);
    }

    g_Player_CopterSndSample->StopActiveVoicesIfPlaying();
}

// Reimplements 0x42b5a0: Player::ReactivateCopterSndNodesIfHealthy
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_CDECL ReactivateCopterSndNodesIfHealthy() {
    zClass_NodePartial *const healthyNode1 = g_Player_CopterHealthyNode1;
    if (healthyNode1 != 0 && (healthyNode1->flags & 0x04) != 0) {
        zClass_NodePartial *const sndNode1 = g_Player_CopterSndNode1;
        if (sndNode1 != 0) {
            zClass_Class::gwNodeSetActive(sndNode1, 1);

            zClass_SoundDataPartial *const soundData =
                static_cast<zClass_SoundDataPartial *>(sndNode1->classData);
            if (soundData != 0) {
                zSndPlayHandle *const playHandle = soundData->playHandle;
                if (playHandle != 0) {
                    zSndPlayHandle::PlayWithDelta_BackendDispatch(
                        g_Player_CopterSndSample, playHandle, 0, 0.0f);
                }
            }
        }
    }

    zClass_NodePartial *const healthyNode2 = g_Player_CopterHealthyNode2;
    if (healthyNode2 != 0 && (healthyNode2->flags & 0x04) != 0) {
        zClass_NodePartial *const sndNode2 = g_Player_CopterSndNode2;
        if (sndNode2 != 0) {
            zClass_Class::gwNodeSetActive(sndNode2, 1);

            zClass_SoundDataPartial *const soundData =
                static_cast<zClass_SoundDataPartial *>(sndNode2->classData);
            if (soundData != 0) {
                zSndPlayHandle *const playHandle = soundData->playHandle;
                if (playHandle != 0) {
                    zSndPlayHandle::PlayWithDelta_BackendDispatch(
                        g_Player_CopterSndSample, playHandle, 0, 0.0f);
                }
            }
        }
    }
}

// Reimplements 0x42b4a0: Player::StopBftBubbleFxHandle
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL StopBftBubbleFxHandle(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zEffectAnimEntry *const handle = playerState->masterTypeTransitionToSubLightHandle;
    if (handle != 0) {
        zEffectAnim::Stop(handle);
        playerState->masterTypeTransitionToSubLightHandle = 0;
    }
}

// Reimplements 0x42b4c0: Player::TransitionToMasterTypeFly
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL TransitionToMasterTypeFly(zUtil_SaveGameState *saveState,
                                                              int flags) {
    (void)flags;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;

    if (g_Time_AccumulatedTimeSec < playerState->masterTypeTransitionCooldownUntilTime) {
        return 0;
    }

    if (masterModalData->masterType == kPlayerMasterTypeSub) {
        playerState->damageVisualFlag = 1;
    }

    playerState->currentMasterType = masterModalData->masterType;
    saveState->SelectModalStateByMasterType(kPlayerMasterTypeFly);
    playerState->masterTypeTransitionCooldownUntilTime =
        g_Time_AccumulatedTimeSec + kPlayerMasterTypeFlyCooldownSec;
    return 1;
}

// Reimplements 0x42ac90: Player::TransitionToMasterTypeTrack
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL TransitionToMasterTypeTrack(zUtil_SaveGameState *saveState,
                                                                int flags) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    PlayerMasterModalData *const masterModalData = primaryModalState->masterModalData;

    if (g_Time_AccumulatedTimeSec < playerState->masterTypeTransitionCooldownUntilTime) {
        return 0;
    }

    const int sourceMasterType = masterModalData->masterType;
    if (sourceMasterType == kPlayerMasterTypeSub) {
        if (flags == 0) {
            return 0;
        }

        zClass_Object3D::gwObject3DSetPosition(
            playerState->altWeaponBanks[1].controllerA.attachNodePrimary, 0.0f, 0.0f, 0.0f);
        zClass_Object3D::gwObject3DSetPosition(
            playerState->altWeaponBanks[1].controllerA.attachNodeSecondary, 0.0f, 0.0f, 0.0f);
        zClass_Object3D::gwObject3DSetPosition(
            playerState->altWeaponBanks[1].controllerB.attachNodePrimary, 0.0f, 0.0f, 0.0f);
        zClass_Object3D::gwObject3DSetPosition(
            playerState->altWeaponBanks[1].controllerB.attachNodeSecondary, 0.0f, 0.0f, 0.0f);
        SetHudUiElementVisible(&g_Player_UnderwaterFxPass3Ui, 0);
        g_Player_HorizonNodeFollowCameraEnabled = 1;
        saveState->StopMasterTypeLoopSfxHandle(kPlayerMasterTypeTrack);
        ReactivateCopterSndNodesIfHealthy();

        zClass_NodePartial *const nodeCaustic1 = primaryModalState->nodeCaustic1;
        if (nodeCaustic1 != 0) {
            unsigned int displayInstanceValue = 0;
            zClass_Class::gwNodeGetUserData(nodeCaustic1, &displayInstanceValue);
            zDi::SetCurrentVariantCycleTextureSpeed((zDiPartial *)displayInstanceValue, 0.0f);
        }

        playerState->damageVisualFlag = 1;
    } else if (sourceMasterType == kPlayerMasterTypeHover) {
        if (playerState->autoTurnSign != 0) {
            return 0;
        }

        TriggerZeroVelocityFxList(masterModalData->fxList_fromHoverToTrack,
                                  playerState->rootNode, flags);
    } else if (sourceMasterType == kPlayerMasterTypeAmphib) {
        TriggerZeroVelocityFxList(masterModalData->fxList_fromAmphibToTrack,
                                  playerState->rootNode, flags);
    }

    playerState->currentMasterType = masterModalData->masterType;
    saveState->SelectModalStateByMasterType(kPlayerMasterTypeTrack);
    playerState->masterTypeTransitionCooldownUntilTime =
        g_Time_AccumulatedTimeSec + kPlayerMasterTypeTrackCooldownSec;
    zClass_Class::gwNodeSetActive(playerState->modeVariantNode, 1);

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x238), 5.0f);
        HudUiMgr::SetModeCounterState(0, 2);
    }

    zEffectAnimEntry *const toAmphibLightHandle =
        playerState->masterTypeTransitionToAmphibLightHandle;
    if (toAmphibLightHandle != 0) {
        zEffectAnim::Stop(toAmphibLightHandle);
        playerState->masterTypeTransitionToAmphibLightHandle = 0;
    }

    zEffect_Anim::NodeActionCallback(playerState->masterTypeTransitionToTrackNodeAction,
                                     playerState->rootNode);
    playerState->masterTypeTransitionToTrackLightHandle = zEffectAnim::SetVelocity_Thunk(
        playerState->masterTypeTransitionToTrackNodeAction, playerState->rootNode, 0.0f, 0.0f,
        0.0f);
    return 1;
}

// Reimplements 0x42aeb0: Player::TransitionToMasterTypeAmphib
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL TransitionToMasterTypeAmphib(
    zUtil_SaveGameState *saveState, int transitionFlags, int extraFlags) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    PlayerMasterModalData *const masterModalData = primaryModalState->masterModalData;

    if (g_Time_AccumulatedTimeSec < playerState->masterTypeTransitionCooldownUntilTime) {
        return 0;
    }
    if (playerState->amphibUnlocked == 0) {
        return 0;
    }

    const int sourceMasterType = masterModalData->masterType;
    if (sourceMasterType == kPlayerMasterTypeSub) {
        if (transitionFlags != 0) {
            return 0;
        }

        SetHudUiElementVisible(&g_Player_UnderwaterFxPass3Ui, 0);
        g_Player_HorizonNodeFollowCameraEnabled = 1;
        saveState->StopMasterTypeLoopSfxHandle(kPlayerMasterTypeTrack);
        ReactivateCopterSndNodesIfHealthy();

        zClass_NodePartial *const nodeCaustic1 = primaryModalState->nodeCaustic1;
        if (nodeCaustic1 != 0) {
            unsigned int displayInstanceValue = 0;
            zClass_Class::gwNodeGetUserData(nodeCaustic1, &displayInstanceValue);
            zDi::SetCurrentVariantCycleTextureSpeed((zDiPartial *)displayInstanceValue, 0.0f);
        }

        StopBftBubbleFxHandle(saveState);
        TriggerZeroVelocityFxList(masterModalData->fxList_fromSubToAmphib,
                                  playerState->rootNode, extraFlags);
        playerState->damageVisualFlag = 1;
    } else if (sourceMasterType == kPlayerMasterTypeTrack) {
        playerState->airborneFlag = 0;
        zClass_NodePartial *const modalNode = primaryModalState->modalNode;
        if (modalNode != 0) {
            zClass_Object3D::gwObject3DSetRotation(modalNode, 0.0f, 0.0f, 0.0f);
        }
        zClass_Class::gwNodeSetActive(playerState->modeVariantNode, 1);
        TriggerZeroVelocityFxList(masterModalData->fxList_fromTrackToAmphib,
                                  playerState->rootNode, extraFlags);
    } else if (sourceMasterType == kPlayerMasterTypeHover) {
        TriggerZeroVelocityFxList(masterModalData->fxList_fromHoverToAmphib,
                                  playerState->rootNode, extraFlags);
    }

    playerState->currentMasterType = masterModalData->masterType;
    saveState->SelectModalStateByMasterType(kPlayerMasterTypeAmphib);
    playerState->masterTypeTransitionCooldownUntilTime =
        g_Time_AccumulatedTimeSec + kPlayerMasterTypeTrackCooldownSec;

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x239), 5.0f);
        HudUiMgr::SetModeCounterState(1, 2);
    }

    playerState->vehicleRollRad = 0.0f;
    playerState->vehiclePitchRad = 0.0f;

    zEffectAnimEntry *const toTrackLightHandle =
        playerState->masterTypeTransitionToTrackLightHandle;
    if (toTrackLightHandle != 0) {
        zEffectAnim::Stop(toTrackLightHandle);
        playerState->masterTypeTransitionToTrackLightHandle = 0;
    }

    zEffect_Anim::NodeActionCallback(playerState->masterTypeTransitionToAmphibNodeAction,
                                     playerState->rootNode);
    playerState->masterTypeTransitionToAmphibLightHandle = zEffectAnim::SetVelocity_Thunk(
        playerState->masterTypeTransitionToAmphibNodeAction, playerState->rootNode, 0.0f,
        0.0f, 0.0f);
    return 1;
}

// Reimplements 0x42b2a0: Player::TransitionToMasterTypeSub
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL TransitionToMasterTypeSub(zUtil_SaveGameState *saveState,
                                                              int flags) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    PlayerMasterModalData *const masterModalData = primaryModalState->masterModalData;

    playerState->damageVisualFlag = 1;
    ApplyGunFireSlotOffsetToNode(saveState);

    if (g_Time_AccumulatedTimeSec < playerState->masterTypeTransitionCooldownUntilTime) {
        return 0;
    }
    if (playerState->subUnlocked == 0) {
        return 0;
    }

    const int sourceMasterType = masterModalData->masterType;
    if (sourceMasterType == kPlayerMasterTypeSub) {
        if (flags == 0) {
            return 1;
        }

        saveState->StopMasterTypeLoopSfxHandle(kPlayerMasterTypeTrack);
    } else if (sourceMasterType == kPlayerMasterTypeTrack) {
        if (flags == 0) {
            return 0;
        }

        playerState->airborneFlag = 0;
        zClass_NodePartial *const modalNode = primaryModalState->modalNode;
        if (modalNode != 0) {
            zClass_Object3D::gwObject3DSetRotation(modalNode, 0.0f, 0.0f, 0.0f);
        }

        playerState->localVel.y = -3.0f;
        playerState->worldPos.y -= 4.0999999f;
    } else if (sourceMasterType == kPlayerMasterTypeAmphib) {
        if (playerState->bankInput != 0 && flags == 0) {
            return 0;
        }

        TriggerZeroVelocityFxList(masterModalData->fxList_fromAmphibToSub,
                                  playerState->rootNode, flags);
        playerState->localVel.y = -3.0f;
        playerState->worldPos.y -= 4.0999999f;
    }

    playerState->currentMasterType = masterModalData->masterType;
    saveState->SelectModalStateByMasterType(kPlayerMasterTypeSub);

    if (Player::IsAltWeaponAllowedInCurrentMasterMode(
            saveState, playerState->activeAltGunController->optCatalogEntry) == 0) {
        Player::AutoSwitchToNextUsableAltWeapon(saveState);
    }

    playerState->masterTypeTransitionCooldownUntilTime =
        g_Time_AccumulatedTimeSec + kPlayerMasterTypeTrackCooldownSec;

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x23b), 5.0f);
        HudUiMgr::SetModeCounterState(3, 2);
        saveState->EnsureMasterTypeLoopSfxHandle(kPlayerMasterTypeTrack, 0.5f);
        CacheDisableCopterSndNodesAndStopSample();
    }

    zEffectAnimEntry *const toTrackLightHandle =
        playerState->masterTypeTransitionToTrackLightHandle;
    if (toTrackLightHandle != 0) {
        zEffectAnim::Stop(toTrackLightHandle);
        playerState->masterTypeTransitionToTrackLightHandle = 0;
    }

    zEffectAnimEntry *const toAmphibLightHandle =
        playerState->masterTypeTransitionToAmphibLightHandle;
    if (toAmphibLightHandle != 0) {
        zEffectAnim::Stop(toAmphibLightHandle);
        playerState->masterTypeTransitionToAmphibLightHandle = 0;
    }

    zEffect_Anim::NodeActionCallback(playerState->masterTypeTransitionToSubNodeAction,
                                     playerState->rootNode);
    playerState->masterTypeTransitionToSubLightHandle = zEffectAnim::SetVelocity_Thunk(
        playerState->masterTypeTransitionToSubNodeAction, playerState->rootNode, 0.0f, 0.0f,
        0.0f);
    return 1;
}

// Reimplements 0x42b0f0: Player::TransitionToMasterTypeHover
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL TransitionToMasterTypeHover(zUtil_SaveGameState *saveState,
                                                                int flags) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    PlayerMasterModalData *const masterModalData = primaryModalState->masterModalData;

    if (g_Time_AccumulatedTimeSec < playerState->masterTypeTransitionCooldownUntilTime) {
        return 0;
    }
    if (playerState->hoverUnlocked == 0) {
        return 0;
    }

    const int sourceMasterType = masterModalData->masterType;
    if (sourceMasterType == kPlayerMasterTypeSub) {
        if (flags == 0) {
            return 0;
        }

        SetHudUiElementVisible(&g_Player_UnderwaterFxPass3Ui, 0);
        g_Player_HorizonNodeFollowCameraEnabled = 1;
        saveState->StopMasterTypeLoopSfxHandle(kPlayerMasterTypeTrack);
        ReactivateCopterSndNodesIfHealthy();

        zClass_NodePartial *const nodeCaustic1 = primaryModalState->nodeCaustic1;
        if (nodeCaustic1 != 0) {
            unsigned int displayInstanceValue = 0;
            zClass_Class::gwNodeGetUserData(nodeCaustic1, &displayInstanceValue);
            zDi::SetCurrentVariantCycleTextureSpeed((zDiPartial *)displayInstanceValue, 0.0f);
        }

        playerState->damageVisualFlag = 1;
    } else if (sourceMasterType == kPlayerMasterTypeTrack) {
        playerState->airborneFlag = 0;
        zClass_NodePartial *const modalNode = primaryModalState->modalNode;
        if (modalNode != 0) {
            zClass_Object3D::gwObject3DSetRotation(modalNode, 0.0f, 0.0f, 0.0f);
        }
        zClass_Class::gwNodeSetActive(playerState->modeVariantNode, 1);
        TriggerZeroVelocityFxList(masterModalData->fxList_fromTrackToHover,
                                  playerState->rootNode, flags);
    } else if (sourceMasterType == kPlayerMasterTypeAmphib) {
        TriggerZeroVelocityFxList(masterModalData->fxList_fromAmphibToHover,
                                  playerState->rootNode, flags);
    }

    playerState->currentMasterType = masterModalData->masterType;
    saveState->SelectModalStateByMasterType(kPlayerMasterTypeHover);
    playerState->masterTypeTransitionCooldownUntilTime =
        g_Time_AccumulatedTimeSec + kPlayerMasterTypeTrackCooldownSec;

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x23a), 5.0f);
        HudUiMgr::SetModeCounterState(2, 2);
    }

    return 1;
}

// Reimplements 0x42b520: Player::ApplyMasterTypeTransition
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
ApplyMasterTypeTransition(zUtil_SaveGameState *saveState, int masterType, int flags) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->primaryGunGateUntilTime = g_Time_AccumulatedTimeSec;

    switch (masterType) {
    case kPlayerMasterTypeFly:
        return TransitionToMasterTypeFly(saveState, flags);
    case kPlayerMasterTypeSub:
        return TransitionToMasterTypeSub(saveState, flags);
    case kPlayerMasterTypeTrack:
        return TransitionToMasterTypeTrack(saveState, flags);
    case kPlayerMasterTypeHover:
        return TransitionToMasterTypeHover(saveState, flags);
    case kPlayerMasterTypeAmphib:
        return TransitionToMasterTypeAmphib(saveState, 0, flags);
    default:
        return masterType - 1;
    }
}
} // namespace Player

// Reimplements 0x425060: HudSensorTracker::ParseCheckpointNumberFromNode
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
HudSensorTracker::ParseCheckpointNumberFromNode(zClass_NodePartial *node) {
    if ((node->flags & 0x200000) == 0) {
        return 0;
    }

    zClass_NodePartial *const contextNode = node->callbackContext;
    if ((contextNode->auxFlags & 2) == 0) {
        return 0;
    }

    const char *const name = contextNode->name;
    const size_t nameLen = strlen(name);
    if (nameLen <= 10) {
        return 0;
    }

    const long parsedNumber = atol(name + 10);
    return parsedNumber < 0 ? 0 : static_cast<int>(parsedNumber);
}

namespace {
struct PlayerCheckpointLapProgressView {
    unsigned char unknown_0000[0x1018];
    int checkpointVisitedFlags[33];
    float lapTimeDelta;
    float lapTimeSec;
    float lapTimestampSec;
    float checkpointTimestampSec;
    int lapCompletionCount;
};

RECOIL_STATIC_ASSERT(offsetof(PlayerCheckpointLapProgressView, checkpointVisitedFlags) == 0x1018);
RECOIL_STATIC_ASSERT(offsetof(PlayerCheckpointLapProgressView, lapTimeDelta) == 0x109c);
RECOIL_STATIC_ASSERT(offsetof(PlayerCheckpointLapProgressView, lapTimeSec) == 0x10a0);
RECOIL_STATIC_ASSERT(offsetof(PlayerCheckpointLapProgressView, lapTimestampSec) == 0x10a4);
RECOIL_STATIC_ASSERT(offsetof(PlayerCheckpointLapProgressView, checkpointTimestampSec) == 0x10a8);
RECOIL_STATIC_ASSERT(offsetof(PlayerCheckpointLapProgressView, lapCompletionCount) == 0x10ac);
} // namespace

namespace Checkpoint {
// Reimplements 0x420c60: Checkpoint::InstantiateNamedObjects
// (D:\Proj\GameZRecoil\checkpoint.cpp)
RECOIL_NOINLINE void RECOIL_CDECL InstantiateNamedObjects() {
    CString searchName;
    const int checkpointCount = g_HudSensorTracker.checkpointCount;

    for (int checkpointNumber = 1; checkpointNumber <= checkpointCount; ++checkpointNumber) {
        searchName.Format("checkpoint%d", checkpointNumber);
        zClass_NodePartial *const checkpointNode =
            zClass::FindByTypeAndName(6, searchName.m_pchData);
        if (checkpointNode != 0) {
            zClass_Node::PropagateExtraFlagsRecursive(checkpointNode,
                                                      kCheckpointNodeAuxFlagTracked);
            zClass_Node::PropagateFlagsRecursive(checkpointNode, kCheckpointNodePickableFlag);
            zClass_Node::SetContextRecursive(checkpointNode, checkpointNode,
                                             kCheckpointNodeContextFlag);
        }
    }
}

// Reimplements 0x425150: Checkpoint::UpdatePlayerLapProgressAndNotifyNet
// (D:\Proj\GameZRecoil\checkpoint.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdatePlayerLapProgressAndNotifyNet(zUtil_SaveGameState *saveState, int checkpointIndex) {
    const int checkpointCount = g_HudSensorTracker.checkpointCount;
    PlayerCheckpointLapProgressView *const playerProgress =
        (PlayerCheckpointLapProgressView *)(saveState->playerState);

    if (playerProgress->checkpointVisitedFlags[checkpointIndex] != 0) {
        return;
    }

    playerProgress->checkpointVisitedFlags[checkpointIndex] = 1;
    if (checkpointCount != checkpointIndex) {
        return;
    }

    int allPriorCheckpointsVisited = 1;
    for (int index = 1; index <= checkpointCount; ++index) {
        allPriorCheckpointsVisited =
            allPriorCheckpointsVisited != 0 &&
                    playerProgress->checkpointVisitedFlags[index] != 0
                ? 1
                : 0;
        playerProgress->checkpointVisitedFlags[index] = 0;
    }

    if (allPriorCheckpointsVisited == 0) {
        return;
    }

    playerProgress->lapTimeDelta =
        g_Time_AccumulatedTimeSec - playerProgress->lapTimestampSec;
    playerProgress->lapTimestampSec = g_Time_AccumulatedTimeSec;
    playerProgress->lapCompletionCount += 1;
    playerProgress->lapTimeSec =
        g_Time_AccumulatedTimeSec - playerProgress->checkpointTimestampSec;
    GameNet::SendPkt0E_PlayerLapProgress(saveState);
}
} // namespace Checkpoint

// Reimplements 0x424010: PlayerPendingContact::SelectPreferred
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE PlayerPendingContact *RECOIL_FASTCALL
PlayerPendingContact::SelectPreferred(PlayerPendingContact *rhs) {
    const float selfApproachDot =
        (sweepEnd.x - hit.hitPos.x) * hit.surfaceNormal.x +
        (sweepEnd.z - hit.hitPos.z) * hit.surfaceNormal.z;
    const float rhsApproachDot =
        (rhs->sweepEnd.x - rhs->hit.hitPos.x) * rhs->hit.surfaceNormal.x +
        (rhs->sweepEnd.z - rhs->hit.hitPos.z) * rhs->hit.surfaceNormal.z;

    if (-rhsApproachDot < -selfApproachDot) {
        return this;
    }
    return rhs;
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
    kPlayerMaxModalProbePoints = 4,
    kPlayerEnvProbeBasePointOffset = 15
};

const int g_PlayerEnvProbeSampleMaskTable[8] = {0x89, 0x43, 0x86, 0x4c,
                                                0x28, 0x22, 0xf0, 0x00};

enum PlayerCameraState {
    kPlayerCameraStateToggleRequest = 0,
    kPlayerCameraStateThirdPerson = 1,
    kPlayerCameraStateClearScreen = 2,
    kPlayerCameraStateFirstPerson = 3,
    kPlayerCameraStateTargeting = 4,
    kPlayerCameraStateProjectileAttached = 7,
    kPlayerCameraStateRestorePrevious = 8
};

struct PlayerContactSurfacePayload {
    unsigned char unknown_00[0x20];
    int impactSlot;
};

void SetState7FxPass3Visible(int visible) {
    typedef void (RECOIL_THISCALL *SetVisibleFn)(HudUiElement * self, int visible);
    SetVisibleFn const setVisible =
        (SetVisibleFn)(g_Player_State7FxPass3Ui.ftable->slots[0x60 / 4]);
    setVisible(&g_Player_State7FxPass3Ui, visible);
}

PlayerPendingContact *AppendPendingContact(PlayerPendingContactQueue *queue) {
    PlayerPendingContact *const contact = new PlayerPendingContact;
    memset(contact, 0, sizeof(*contact));
    contact->next = 0;

    if (queue->count == 0) {
        queue->head = contact;
    } else {
        queue->tail->next = contact;
    }

    queue->tail = contact;
    contact->next = 0;
    ++queue->count;
    return contact;
}

void CopyPendingContactPayload(PlayerPendingContact *contact,
                               const zClassDiPickCandidateEntry *candidate,
                               const zVec3 *segmentStart, const zVec3 *segmentEnd,
                               int segmentTag) {
    contact->hit = *candidate;
    contact->sweepStart = *segmentStart;
    contact->sweepEnd = *segmentEnd;
    contact->segmentTag = segmentTag;
}

OptCatalogDamageHandlerPartial *GetNodeDamageHandler(zClass_NodePartial *node) {
    return (OptCatalogDamageHandlerPartial *)(((zClass_NodeFreeListSlot *)(node))->damageHandler);
}

void FreePendingContactQueue(PlayerPendingContactQueue *queue) {
    PlayerPendingContact *contact = queue->head;
    while (contact != 0) {
        PlayerPendingContact *const next = contact->next;
        delete contact;
        contact = next;
    }

    queue->listAux = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
}

void AppendExistingPendingContact(PlayerPendingContactQueue *queue, PlayerPendingContact *contact) {
    contact->next = 0;
    if (queue->count == 0) {
        queue->head = contact;
    } else {
        queue->tail->next = contact;
    }

    queue->tail = contact;
    contact->next = 0;
    ++queue->count;
}

void RemoveExistingPendingContact(PlayerPendingContactQueue *queue, PlayerPendingContact *contact) {
    if (queue->count == 0 || contact == 0) {
        return;
    }

    if (queue->head == contact) {
        queue->head = contact->next;
        --queue->count;
        if (queue->head == 0) {
            queue->listAux = 0;
            queue->tail = 0;
        }
        return;
    }

    PlayerPendingContact *previous = queue->head;
    while (previous != 0 && previous->next != contact) {
        previous = previous->next;
    }

    if (previous != 0) {
        previous->next = contact->next;
        --queue->count;
        if (queue->tail == contact) {
            queue->tail = previous;
        }
    }
}

void MoveTransferContactsToPreferredCollision(zUtil_PlayerStateStorage *playerState) {
    PlayerPendingContact *contact = playerState->transferQueue.head;
    while (contact != 0) {
        PlayerPendingContact *const next = contact->next;
        playerState->transferQueue.head = next;
        --playerState->transferQueue.count;
        if (next == 0) {
            playerState->transferQueue.listAux = 0;
            playerState->transferQueue.tail = 0;
        }

        AppendExistingPendingContact(&playerState->preferredCollisionQueue, contact);
        contact = next;
    }
}

void EnableContactSegment(int *enabledSegmentFlags, int index) {
    enabledSegmentFlags[index] = 1;
}

void BuildModalAndRootProbeWorldCaches(zUtil_PlayerStateStorage *playerState,
                                       const PlayerMasterModalData *masterModalData) {
    for (int i = 0; i < masterModalData->probePointCount; ++i) {
        playerState->modalProbeWorldByIndex[i] =
            TransformPointByMatrix(masterModalData->probePoints[i], playerState->motionBasis);
        playerState->rootProbeWorldByIndex[i] =
            TransformPointByMatrix(masterModalData->probePoints[i], playerState->previousTransform);
    }
}

float Vec3Length(const zVec3 &vec) {
    return static_cast<float>(sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z));
}

float Vec3Dot(const zVec3 &a, const zVec3 &b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float Vec3DotXZ(const zVec3 &a, const zVec3 &b) {
    return a.x * b.x + a.z * b.z;
}

zVec3 Vec3Cross(const zVec3 &a, const zVec3 &b) {
    return {a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}

} // namespace

// Reimplements 0x42a9f0: Player::AddScaledHudCounterValue
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL AddScaledHudCounterValue(float value) {
    float scale = 1.0f;
    if (g_HudSensorTracker.primaryGunDispatchCount > 0) {
        scale = static_cast<float>(g_OptCatalog_DamageFeedbackHitCount) /
                static_cast<float>(g_HudSensorTracker.primaryGunDispatchCount);
    }

    g_Player_HudCounterValue += static_cast<int>(value * scale * 1000.0f);
}

// Reimplements 0x41bab0: Player::UpdateGunDispatchRequestsFromTriggerLatches
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateGunDispatchRequestsFromTriggerLatches(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    if (playerState->netInputBit16Latch == 0) {
        playerState->altGunDispatchRequested = 0;
    } else if ((playerState->activeAltGunController->optCatalogEntry->flags &
                kOptCatalogFlagAltDispatchLatch) != 0) {
        playerState->altGunDispatchRequested = 1;
    }

    if (playerState->netInputBit17Latch == 0) {
        playerState->primaryGunDispatchRequested = 0;
        return;
    }

    PlayerGunFireController *const activePrimaryGunController =
        playerState->activePrimaryGunController;
    if (g_Player_TotalTimeSecScaled >= activePrimaryGunController->nextDispatchTime &&
        (playerState->altGunTransitionState & 0x180) == 0) {
        playerState->primaryGunDispatchRequested = 1;
        activePrimaryGunController->nextDispatchTime =
            activePrimaryGunController->dispatchRepeatDelay + g_Player_TotalTimeSecScaled;
    }
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

// Reimplements 0x401420: Player::AiMode2ForwardProbeRequiresAutoTurn
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
AiMode2ForwardProbeRequiresAutoTurn(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (playerState->playerCollisionResolved != 0 ||
        playerState->preferredCollisionResolved != 0) {
        ++playerState->aiMode2SteeringRetryCount;
        return 1;
    }

    const PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;

    zClass_DiSegmentEndpoints segmentPairs[1];
    segmentPairs[0].start = playerState->worldPos;
    segmentPairs[0].start.y += masterModalData->probePoints[1].y;

    zVec3 forwardDir = playerState->projectileSpawnVel;
    float forwardProbeLength = zMath::Vec3Normalize(&forwardDir);
    if (forwardProbeLength < 1.0f) {
        forwardProbeLength = 1.0f;
    } else {
        forwardProbeLength = zMath::Vec3Normalize(&forwardDir);
    }

    const float forwardProbeOffset =
        forwardProbeLength * 0.5f - masterModalData->probePoints[1].z;
    segmentPairs[0].end.x = playerState->worldPos.x + forwardProbeOffset * forwardDir.x;
    segmentPairs[0].end.y = playerState->worldPos.y + forwardProbeOffset * forwardDir.y;
    segmentPairs[0].end.z = playerState->worldPos.z + forwardProbeOffset * forwardDir.z;

    int segmentTags[2] = {-1, -1};
    CollectPendingContactsForSegments(saveState, segmentPairs, 2, segmentTags);

    const int hasBlockingContacts =
        playerState->preferredCollisionQueue.count != 0 ||
        playerState->playerCollisionQueue.count != 0;
    ClearPendingContactQueues(saveState);
    return hasBlockingContacts != 0 ? 1 : 0;
}

// Reimplements 0x4016a0: Player::AiChooseNextPathBranchIndex
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL AiChooseNextPathBranchIndex(
    zUtil_SaveGameState *saveState, AINetNode **currentNodeInOut, int *outBranchIndex,
    int excludedBranchIndex) {
    (void)saveState;

    AINetNode *const currentNode = *currentNodeInOut;
    int branchCount = 0;
    for (int branchIndex = 0; branchIndex < 3; ++branchIndex) {
        if (currentNode->neighborNodes[branchIndex] != 0) {
            ++branchCount;
        }
    }

    if (branchCount == 1) {
        *outBranchIndex = 0;
        return 1;
    }

    if (branchCount == 2) {
        *outBranchIndex = 0;
    } else {
        *outBranchIndex = rand() % branchCount;
    }

    if (*outBranchIndex == excludedBranchIndex) {
        *outBranchIndex = (*outBranchIndex + 1) % branchCount;
    }

    return 1;
}

// Reimplements 0x401580: Player::AiAdvancePathCursorAndComputeTargetVec
// (GameZRecoil/Player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL AiAdvancePathCursorAndComputeTargetVec(
    zUtil_SaveGameState *saveState, AINetNode **currentNodeInOut,
    AINetPathProbeFan **outProbeFan, zVec3 *outTargetVec) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    AINetNode *const previousNode = *currentNodeInOut;
    AINetNode *const nextNode =
        previousNode->neighborNodes[playerState->aiCurrentPathNeighborIndex];
    playerState->aiCurrentPathNode = nextNode;

    const int previousNodeIndex = previousNode->nodeIndex;
    if (previousNodeIndex < 0) {
        previousNode->Free();
        *currentNodeInOut = playerState->aiCurrentPathNode;

        if ((*currentNodeInOut)->nodeIndex < 0) {
            playerState->aiCurrentPathNeighborIndex = 0;
        } else {
            int nextBranchIndex = 0;
            AiChooseNextPathBranchIndex(saveState, currentNodeInOut, &nextBranchIndex, -1);
            playerState->aiCurrentPathNeighborIndex = nextBranchIndex;
            if (playerState->aiNet->aiType == AINET_TYPE_HI) {
                playerState->aiTopLevelState = kPlayerAiTopTurnTowardTarget;
            }
        }
    } else {
        *currentNodeInOut = nextNode;

        int excludedBranchIndex = 4;
        for (int branchIndex = 0; branchIndex < 3; ++branchIndex) {
            AINetNode *const reverseNode = nextNode->neighborNodes[branchIndex];
            if (reverseNode != 0 && reverseNode->nodeIndex == previousNodeIndex) {
                excludedBranchIndex = branchIndex;
                break;
            }
        }

        int nextBranchIndex = 0;
        AiChooseNextPathBranchIndex(saveState, currentNodeInOut, &nextBranchIndex,
                                    excludedBranchIndex);
        playerState->aiCurrentPathNeighborIndex = nextBranchIndex;
    }

    AINetNode *const selectedNode = *currentNodeInOut;
    *outProbeFan = selectedNode->probeFans[playerState->aiCurrentPathNeighborIndex];
    outTargetVec->x = playerState->worldPos.x - selectedNode->position.x;
    outTargetVec->y = playerState->worldPos.y - selectedNode->position.y;
    outTargetVec->z = playerState->worldPos.z - selectedNode->position.z;
}

// Reimplements 0x401060: Player::TickAiMode2TopLevel
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL TickAiMode2TopLevel(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zUtil_PlayerStateStorage *const localPlayerState =
        ((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState;
    playerState->storedTargetPos = localPlayerState->fxOffsetWorld;

    switch (playerState->aiTopLevelState) {
    case kPlayerAiTopPathFollow:
        TickAiMode2PathFollow(saveState);
        if (AiTryEnterMode2AttackPursuitIfLineOfSight(saveState) != 0) {
            AiRebuildSyntheticPathToNodeIfFar(
                saveState,
                playerState->aiCurrentPathNode
                    ->neighborNodes[playerState->aiCurrentPathNeighborIndex]);
        }
        return;

    case kPlayerAiMode2TopSteering:
        TickAiMode2SteeringSubstate(saveState);
        return;

    case kPlayerAiTopTurnTowardTarget:
        UpdateAiMode2TurnTowardPlayerNoThrottle(saveState);
        if (AiTryEnterMode2AttackPursuitIfLineOfSight(saveState) != 0) {
            AiRebuildSyntheticPathToNodeIfFar(
                saveState,
                playerState->aiCurrentPathNode
                    ->neighborNodes[playerState->aiCurrentPathNeighborIndex]);
        }
        return;

    case kPlayerAiTopTurnOnlyTowardTarget:
        UpdateAiMode2TurnInPlaceTowardPlayer(saveState);
        AiTryEnterMode2AttackPursuitIfLineOfSight(saveState);
        return;

    case kPlayerAiTopPathSteering:
        TickAiMode2TimedPathSteering(saveState);
        if (AiTryEnterMode2AttackPursuitIfLineOfSight(saveState) != 0) {
            AiRebuildSyntheticPathToNodeIfFar(
                saveState,
                playerState->aiCurrentPathNode
                    ->neighborNodes[playerState->aiCurrentPathNeighborIndex]);
        }
        return;

    case kPlayerAiTopAutoTurn: {
        const int autoTurnActive = playerState->autoTurnActive;
        playerState->autoTurnSign = 0;
        if (autoTurnActive == 0) {
            playerState->aiTopLevelState = playerState->aiReturnTopLevelState;
        }
        if (AiTryEnterMode2AttackPursuitIfLineOfSight(saveState) != 0) {
            AiRebuildSyntheticPathToNodeIfFar(
                saveState,
                playerState->aiCurrentPathNode
                    ->neighborNodes[playerState->aiCurrentPathNeighborIndex]);
        }
        return;
    }

    default:
        return;
    }
}

// Reimplements 0x401180: Player::TickAiMode2PathFollow
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL TickAiMode2PathFollow(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;
    AINetNode *currentNode = playerState->aiCurrentPathNode;
    AINetPathProbeFan *edgeProbeFan =
        currentNode->probeFans[playerState->aiCurrentPathNeighborIndex];
    AINetNode *targetPathNode =
        currentNode->neighborNodes[playerState->aiCurrentPathNeighborIndex];

    zVec3 targetDelta = {};
    if (AiMode2ForwardProbeRequiresAutoTurn(saveState) != 0) {
        AiAdvancePathCursorAndComputeTargetVec(saveState, &currentNode, &edgeProbeFan,
                                               &targetDelta);
        targetPathNode = currentNode->neighborNodes[playerState->aiCurrentPathNeighborIndex];
        playerState->aiReturnTopLevelState = playerState->aiTopLevelState;
        playerState->aiTopLevelState = kPlayerAiTopAutoTurn;
        playerState->autoTurnActive = 1;

        zVec3 autoTurnTargetDelta = {
            targetPathNode->position.x - playerState->worldPos.x,
            targetPathNode->position.y - playerState->worldPos.y,
            targetPathNode->position.z - playerState->worldPos.z,
        };
        autoTurnTargetDelta.y = 0.0f;
        zMath::Vec3NormalizeXZ(&autoTurnTargetDelta, &playerState->autoTurnTargetDir);
        playerState->throttleInput = 0.0f;
        playerState->throttleInputCopy = 0.0f;
        playerState->steeringInput = 0.0f;
        return;
    }

    targetDelta.x = targetPathNode->position.x - playerState->worldPos.x;
    targetDelta.y = targetPathNode->position.y - playerState->worldPos.y;
    targetDelta.z = targetPathNode->position.z - playerState->worldPos.z;
    targetDelta.y = 0.0f;
    const float targetDistance = zMath::Vec3Normalize(&targetDelta);

    zVec3 steerBasis = playerState->steerBasisNorm;
    const float steerDotXZ =
        steerBasis.x * targetDelta.x + steerBasis.z * targetDelta.z;
    const float steerCrossXZ =
        steerBasis.z * targetDelta.x - steerBasis.x * targetDelta.z;

    if (steerDotXZ < 0.0f) {
        if (playerState->aiPathCursorAdvanceRequested != 0) {
            AiAdvancePathCursorAndComputeTargetVec(saveState, &currentNode, &edgeProbeFan,
                                                   &targetDelta);
            playerState->aiPathCursorAdvanceRequested = 0;
            TickAiMode2PathFollow(saveState);
            return;
        }

        playerState->throttleInput = 0.0f;
        playerState->steeringInput = steerCrossXZ < 0.0f ? -1.0f : 1.0f;
    } else {
        float throttle = 1.0f - static_cast<float>(fabs(steerCrossXZ));
        if (throttle <= kPlayerAiPathFollowMinThrottle) {
            throttle = kPlayerAiPathFollowMinThrottle;
        }
        playerState->aiPathCursorAdvanceRequested = 1;
        playerState->throttleInput = throttle;
        playerState->steeringInput = steerCrossXZ;
    }

    playerState->throttleInputCopy = playerState->throttleInput;
    playerState->steeringInputCopy = playerState->steeringInput;

    if (masterModalData->masterType == kPlayerMasterTypeSub) {
        const float pitchInput =
            ((targetPathNode->position.y - playerState->worldPos.y +
              masterModalData->modeAltTransitionTime) *
                 g_Player_AiMode2_PathFollowPitchInputScale -
             playerState->vehiclePitchRad) *
            g_Player_AiMode2_PathFollowPitchTurnGain;
        playerState->subPitchInput = pitchInput;
        playerState->subPitchInputCopy = pitchInput;
    }

    if (targetDistance < kPlayerAiPathFollowAdvanceDistance) {
        AiAdvancePathCursorAndComputeTargetVec(saveState, &currentNode, &edgeProbeFan,
                                               &targetDelta);
        playerState->aiPathCursorAdvanceRequested = 0;
    }
}

// Reimplements 0x401c60: Player::AiEnterMode2SteeringPursuit
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
AiEnterMode2SteeringPursuit(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const aiState = saveState->playerState;
    zUtil_PlayerStateStorage *const localPlayerState =
        ((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState;
    if (g_Player_AiMode2State1Finalized != 0) {
        return;
    }

    const int previousTopLevelState = aiState->aiTopLevelState;
    aiState->aiSavedTopLevelState = previousTopLevelState;
    aiState->aiStateStartTime = g_Player_TotalTimeSecScaled;
    aiState->aiStateEndTime = aiState->aiMode2AttackDwell + g_Player_TotalTimeSecScaled;
    if (previousTopLevelState != kPlayerAiMode2TopSteering) {
        aiState->aiTopLevelState = kPlayerAiMode2TopSteering;
    }

    AINetNode *const restorePathNode =
        aiState->aiCurrentPathNode->neighborNodes[aiState->aiCurrentPathNeighborIndex];
    aiState->aiRestoreTarget = restorePathNode->position;

    if (aiState->aiCurrentSteeringSubstate != kPlayerAiMode2SteerDynamicOffsetTarget) {
        return;
    }

    aiState->aiDynamicOffsetDir.x = aiState->worldPos.x - localPlayerState->worldPos.x;
    aiState->aiDynamicOffsetDir.y = 0.0f;
    aiState->aiDynamicOffsetDir.z = aiState->worldPos.z - localPlayerState->worldPos.z;
    zMath::Vec3Normalize(&aiState->aiDynamicOffsetDir);
}

// Reimplements 0x401c00: Player::AiAlertAttackBuddies
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL AiAlertAttackBuddies(zUtil_SaveGameState *saveState) {
    zUtil_SaveGameState *buddySaveState = saveState->aiPeerRingNext;
    if (g_Player_AiMode2State1Finalized != 0 || buddySaveState == saveState) {
        return;
    }

    do {
        zUtil_PlayerStateStorage *const buddyState = buddySaveState->playerState;
        if (buddyState->aiTopLevelState != kPlayerAiMode2TopSteering) {
            AiEnterMode2SteeringPursuit(buddySaveState);
            buddyState->recentHitFlag = 1;
            buddyState->recentHitExpireTime = g_Time_AccumulatedTimeSec + 10.0f;
        }
        buddySaveState = buddySaveState->aiPeerRingNext;
    } while (buddySaveState != saveState);
}

// Reimplements 0x401b20: Player::AiTryEnterMode2AttackPursuitIfLineOfSight
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
AiTryEnterMode2AttackPursuitIfLineOfSight(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const aiState = saveState->playerState;
    if (g_Player_AiMode2State1Finalized != 0) {
        return 0;
    }

    if (g_Player_TotalTimeSecScaled <= aiState->aiStateUntilTime) {
        return 0;
    }

    zUtil_PlayerStateStorage *const localPlayerState =
        ((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState;
    const float targetDistSq =
        zMath::Vec3DeltaLengthSq(&localPlayerState->fxOffsetWorld, &aiState->fxOffsetWorld);
    if (targetDistSq >= aiState->aiAttackRadiusSq) {
        return 0;
    }

    zVec3 lineOfSightPoint = aiState->fxOffsetWorld;
    lineOfSightPoint.y += kPlayerAiAttackLosTargetYOffset;
    if (HasLineOfSightFromLocalPlayerFxOffset(aiState->rootNode, &lineOfSightPoint, 1) == 0) {
        aiState->aiTargetLineOfSightClear = 0;
        return 0;
    }

    AiEnterMode2SteeringPursuit(saveState);
    aiState->aiTargetLineOfSightClear = 1;
    if (aiState->aiNet->attackBuddyNetId != 0) {
        AiAlertAttackBuddies(saveState);
    }
    aiState->aiMode2SteeringRetryCount = 0;
    return 1;
}

// Reimplements 0x401f60: Player::AiRebuildSyntheticPathToNodeIfFar
// (GameZRecoil/Player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
AiRebuildSyntheticPathToNodeIfFar(zUtil_SaveGameState *saveState, AINetNode *targetNode) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    AINetNode *const currentPathNode = playerState->aiCurrentPathNode;

    if (zMath::Vec3DeltaLengthSq(&playerState->worldPos, &currentPathNode->position) <
        kPlayerAiSyntheticPathRebuildDistanceSq) {
        return;
    }

    AINetNode *const syntheticNode = static_cast<AINetNode *>(malloc(sizeof(AINetNode)));
    memset(syntheticNode, 0, sizeof(*syntheticNode));
    syntheticNode->neighborNodes[0] = targetNode;
    syntheticNode->position = playerState->worldPos;
    syntheticNode->nodeIndex = -1;

    AINetPathProbeFan *const fan =
        static_cast<AINetPathProbeFan *>(malloc(sizeof(AINetPathProbeFan)));
    memset(fan, 0, sizeof(*fan));
    syntheticNode->probeFans[0] = fan;
    fan->InitFromSegment(syntheticNode->position, currentPathNode->position,
                         kPlayerAiSyntheticPathWidth);

    playerState->aiCurrentPathNode = syntheticNode;
    playerState->aiCurrentPathNeighborIndex = 0;
    playerState->aiNextPathRebuildTime =
        g_Player_TotalTimeSecScaled + kPlayerAiSyntheticPathRebuildDelaySec;
}

// Reimplements 0x401710: Player::TickAiMode2SteeringSubstate
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
TickAiMode2SteeringSubstate(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;
    zUtil_SaveGameState *const targetSaveState = (zUtil_SaveGameState *)g_GameStateOrMapTable;
    zUtil_PlayerStateStorage *const targetPlayerState = targetSaveState->playerState;
    const zVec3 targetWorldSnapshot = targetPlayerState->worldPos;

    if (g_Player_TotalTimeSecScaled >= playerState->aiNextPathRebuildTime &&
        playerState->aiCurrentSteeringSubstate != kPlayerAiMode2SteerPathFollow) {
        AiRebuildSyntheticPathToNodeIfFar(saveState, playerState->aiCurrentPathNode);
    }

    zVec3 targetDelta = {
        targetWorldSnapshot.x - playerState->worldPos.x,
        targetWorldSnapshot.y - playerState->worldPos.y,
        targetWorldSnapshot.z - playerState->worldPos.z,
    };
    const float targetVerticalDelta = targetDelta.y;
    targetDelta.y = 0.0f;
    const float targetDistance = zMath::Vec3Normalize(&targetDelta);
    const float verticalDistanceScale =
        targetDistance != 0.0f ? targetVerticalDelta / targetDistance : 0.0f;

    const float lateralDot =
        playerState->steerBasisNorm.z * targetDelta.x -
        playerState->steerBasisNorm.x * targetDelta.z;
    float forwardDot =
        playerState->steerBasisNorm.x * targetDelta.x +
        playerState->steerBasisNorm.z * targetDelta.z;

    if (playerState->aiMode2SteeringRetryCount > 6) {
        playerState->aiCurrentSteeringSubstate = kPlayerAiMode2SteerTurnInPlace;
    }

    switch (playerState->aiCurrentSteeringSubstate) {
    case kPlayerAiMode2SteerDirectTarget:
        UpdateAiMode2MoveAndTurnTowardTarget(saveState, forwardDot, lateralDot,
                                             targetDistance);
        break;
    case kPlayerAiMode2SteerOffsetTarget:
        TickAiMode2OffsetTargetSteering(saveState, forwardDot, lateralDot, targetDistance);
        forwardDot = 1.0f;
        break;
    case kPlayerAiMode2SteerDynamicOffsetTarget:
        TickAiMode2DynamicOffsetTargetSteering(saveState, forwardDot, lateralDot,
                                               targetDistance);
        forwardDot = 1.0f;
        break;
    case kPlayerAiMode2SteerPathFollow:
        TickAiMode2PathFollow(saveState);
        forwardDot = 1.0f;
        break;
    case kPlayerAiMode2SteerTurnInPlace:
        UpdateAiMode2TurnInPlaceTowardPlayer(saveState);
        forwardDot = 1.0f;
        break;
    case kPlayerAiMode2SteerAutoTurn:
        if (playerState->autoTurnActive == 0) {
            playerState->aiCurrentSteeringSubstate = playerState->aiReturnSteeringSubstate;
        }
        forwardDot = 1.0f;
        break;
    default:
        break;
    }

    if (masterModalData->masterType == kPlayerMasterTypeSub) {
        const float pitchInput =
            (g_Player_AiMode2_SteeringPitchInputScale * verticalDistanceScale -
             playerState->vehiclePitchRad) *
            g_Player_AiMode2_SteeringPitchTurnGain;
        playerState->subPitchInput = pitchInput;
        playerState->subPitchInputCopy = pitchInput;

        const float verticalInput =
            (targetWorldSnapshot.y - playerState->worldPos.y) *
            g_Player_AiMode2_SteeringVerticalErrorScale;
        playerState->subVerticalInput = verticalInput;
        playerState->subVerticalInputCopy = verticalInput;
    }

    TickAiMode2AltGunAttackWindow(saveState, targetDistance, forwardDot);

    if (targetPlayerState->lifecycleState == kPlayerLifecycleInactive ||
        zMath::Vec3DeltaLengthSq(&playerState->worldPos, &playerState->aiRestoreTarget) >
            playerState->aiRestoreDistanceSq) {
        AiRestoreSavedTopLevelState(saveState);
        playerState->aiStateUntilTime =
            g_Player_TotalTimeSecScaled + playerState->aiNotPursuitDwell;
    }
}

// Reimplements 0x401970: Player::UpdateAiMode2MoveAndTurnTowardTarget
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateAiMode2MoveAndTurnTowardTarget(
    zUtil_SaveGameState *saveState, float forwardDot, float lateralDot,
    float targetDistance) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    if (forwardDot <= 0.0f) {
        playerState->throttleInput = 0.0f;
        playerState->steeringInput = lateralDot < 0.0f ? -1.0f : 1.0f;
    } else {
        AINet *const aiNet = playerState->aiNet;
        playerState->steeringInput = lateralDot;
        if (targetDistance > aiNet->pursuitParam1) {
            playerState->throttleInput = 1.0f;
        } else if (targetDistance < aiNet->pursuitParam0) {
            playerState->throttleInput = -1.0f;
        } else {
            playerState->throttleInput = 0.0f;
        }
    }

    playerState->throttleInputCopy = playerState->throttleInput;
    playerState->steeringInputCopy = playerState->steeringInput;
}

// Reimplements 0x402090: Player::UpdateAiMode2TurnTowardPlayerNoThrottle
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateAiMode2TurnTowardPlayerNoThrottle(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zUtil_PlayerStateStorage *const targetPlayerState =
        ((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState;

    zVec3 targetDelta = {
        targetPlayerState->worldPos.x - playerState->worldPos.x,
        targetPlayerState->worldPos.y - playerState->worldPos.y,
        targetPlayerState->worldPos.z - playerState->worldPos.z,
    };
    targetDelta.y = 0.0f;
    zMath::Vec3Normalize(&targetDelta);

    const float turnCross =
        playerState->steerBasisNorm.z * targetDelta.x -
        playerState->steerBasisNorm.x * targetDelta.z;
    const float forwardDot =
        playerState->steerBasisNorm.x * targetDelta.x +
        playerState->steerBasisNorm.z * targetDelta.z;
    if (forwardDot <= 0.0f) {
        playerState->steeringInput = turnCross < 0.0f ? -1.0f : 1.0f;
    } else {
        playerState->steeringInput = turnCross;
    }

    playerState->throttleInput = 0.0f;
    playerState->throttleInputCopy = 0.0f;
    playerState->steeringInputCopy = playerState->steeringInput;
}

// Reimplements 0x402170: Player::UpdateAiMode2TurnInPlaceTowardPlayer
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateAiMode2TurnInPlaceTowardPlayer(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zUtil_PlayerStateStorage *const targetPlayerState =
        ((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState;

    zVec3 targetDelta = {
        targetPlayerState->worldPos.x - playerState->worldPos.x,
        targetPlayerState->worldPos.y - playerState->worldPos.y,
        targetPlayerState->worldPos.z - playerState->worldPos.z,
    };
    targetDelta.y = 0.0f;
    zMath::Vec3Normalize(&targetDelta);

    const float turnCross =
        playerState->steerBasisNorm.z * targetDelta.x -
        playerState->steerBasisNorm.x * targetDelta.z;
    const float forwardDot =
        playerState->steerBasisNorm.x * targetDelta.x +
        playerState->steerBasisNorm.z * targetDelta.z;
    if (forwardDot <= 0.0f) {
        playerState->steeringInput = turnCross < 0.0f ? -1.0f : 1.0f;
    } else {
        playerState->steeringInput = turnCross;
    }

    playerState->steeringInputCopy = playerState->steeringInput;
    playerState->throttleInput = 0.0f;
    playerState->throttleInputCopy = 0.0f;
}

// Reimplements 0x402250: Player::TickAiMode2AltGunAttackWindow
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL TickAiMode2AltGunAttackWindow(
    zUtil_SaveGameState *saveState, float targetDistance, float forwardDot) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    if (g_Player_TotalTimeSecScaled > playerState->aiStateEndTime) {
        const float startTime = g_Player_TotalTimeSecScaled + playerState->aiNotPursuitDwell;
        playerState->aiStateStartTime = startTime;
        playerState->aiStateEndTime = startTime + playerState->aiMode2AttackDwell;
    }

    PlayerGunFireController *const activeAltGunController =
        playerState->activeAltGunController;
    zUtil_SaveGameState *const targetSaveState = (zUtil_SaveGameState *)g_GameStateOrMapTable;
    zUtil_PlayerStateStorage *const targetPlayerState = targetSaveState->playerState;

    if (playerState->altGunFireHeldFlag != 0) {
        if (g_Player_TotalTimeSecScaled <= activeAltGunController->nextDispatchTime &&
            forwardDot >= kPlayerAiAltGunAttackForwardMin &&
            targetDistance <= activeAltGunController->aiAttackRangeMax &&
            targetPlayerState->lifecycleState != kPlayerLifecycleInactive) {
            playerState->storedTargetPos = targetPlayerState->fxOffsetWorld;
            return;
        }

        playerState->altGunDispatchRequested = 0;
        activeAltGunController->nextDispatchTime =
            g_Player_TotalTimeSecScaled + activeAltGunController->dispatchRepeatDelay;
        return;
    }

    if (g_Player_TotalTimeSecScaled <= activeAltGunController->nextDispatchTime ||
        g_Player_TotalTimeSecScaled <= playerState->aiStateStartTime ||
        playerState->damageProtectionActive != 0 ||
        forwardDot <= kPlayerAiAltGunAttackForwardMin ||
        targetDistance >= activeAltGunController->aiAttackRangeMax ||
        targetDistance <= activeAltGunController->aiAttackRangeMin ||
        HasLineOfSightFromLocalPlayerFxOffset(playerState->rootNode, &playerState->fxOffsetWorld,
                                              1) == 0 ||
        targetPlayerState->lifecycleState == kPlayerLifecycleInactive) {
        return;
    }

    playerState->altGunDispatchRequested = 1;

    float statusScale = playerState->statusMeterScaled;
    if (statusScale <= kPlayerAiAltGunStatusMinScale) {
        statusScale = kPlayerAiAltGunStatusMinScale;
    }

    activeAltGunController->nextDispatchTime =
        g_Player_TotalTimeSecScaled + activeAltGunController->dispatchRepeatDelay / statusScale;

    OptCatalogEntryDef *const optCatalogEntry = activeAltGunController->optCatalogEntry;
    const unsigned int flags = optCatalogEntry->flags;
    if ((flags & kOptCatalogFlagCreateTrail) != 0) {
        playerState->altGunFireHeldFlag = 1;
        OptCatalog::ActivateTrailRuntimeState(activeAltGunController->trailRuntimeState,
                                              playerState->playerOrdinal);
        activeAltGunController->nextDispatchTime =
            g_Player_TotalTimeSecScaled + activeAltGunController->dispatchRepeatDelay;
        return;
    }

    if ((flags & kOptCatalogFlagLockOnTargetRef) != 0) {
        playerState->progressTargetCount = 1;
        playerState->progressTargetSlots[0].targetPos = &targetPlayerState->fxOffsetWorld;
        playerState->progressTargetSlots[0].targetVelocity =
            &targetPlayerState->projectileSpawnVel;
        HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x908), 5.0f);
        return;
    }

    playerState->progressTargetCount = 0;
    playerState->progressTargetSlots[0].targetPos = 0;
    playerState->progressTargetSlots[0].targetVelocity = 0;
    SolveAltGunLeadTargetPoint(saveState, targetSaveState, &playerState->storedTargetPos);
}

// Reimplements 0x4024a0: Player::SolveAltGunLeadTargetPoint
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SolveAltGunLeadTargetPoint(
    zUtil_SaveGameState *saveState, zUtil_SaveGameState *targetSaveState,
    zVec3 *outTargetPos) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zUtil_PlayerStateStorage *const targetPlayerState = targetSaveState->playerState;
    const float inverseProjectileVelocity =
        1.0f / playerState->activeAltGunController->optCatalogEntry->velocity;

    zVec3 scaledTargetDelta = {
        (targetPlayerState->worldPos.x - playerState->worldPos.x) * inverseProjectileVelocity,
        (targetPlayerState->worldPos.y - playerState->worldPos.y) * inverseProjectileVelocity,
        (targetPlayerState->worldPos.z - playerState->worldPos.z) * inverseProjectileVelocity,
    };
    zVec3 relativeVelocity = {
        targetPlayerState->projectileSpawnVel.x - playerState->projectileSpawnVel.x,
        targetPlayerState->projectileSpawnVel.y - playerState->projectileSpawnVel.y,
        targetPlayerState->projectileSpawnVel.z - playerState->projectileSpawnVel.z,
    };
    zVec3 scaledRelativeVelocity = {
        relativeVelocity.x * inverseProjectileVelocity,
        relativeVelocity.y * inverseProjectileVelocity,
        relativeVelocity.z * inverseProjectileVelocity,
    };

    const float relativeSpeedSq =
        scaledRelativeVelocity.x * scaledRelativeVelocity.x +
        scaledRelativeVelocity.y * scaledRelativeVelocity.y +
        scaledRelativeVelocity.z * scaledRelativeVelocity.z;
    const float quadraticA = 1.0f - relativeSpeedSq;
    if (quadraticA <= 0.0f) {
        *outTargetPos = targetPlayerState->worldPos;
        return;
    }

    const float quadraticB =
        scaledRelativeVelocity.x * scaledTargetDelta.x +
        scaledRelativeVelocity.y * scaledTargetDelta.y +
        scaledRelativeVelocity.z * scaledTargetDelta.z;
    const float targetDistanceSq =
        scaledTargetDelta.x * scaledTargetDelta.x +
        scaledTargetDelta.y * scaledTargetDelta.y +
        scaledTargetDelta.z * scaledTargetDelta.z;
    const float discriminant = quadraticA * targetDistanceSq + quadraticB * quadraticB;
    const float leadScale =
        (PlayerFastSqrtEstimate(discriminant) + quadraticB) / quadraticA;

    scaledRelativeVelocity.x = relativeVelocity.x * leadScale;
    scaledRelativeVelocity.y = relativeVelocity.y * leadScale;
    scaledRelativeVelocity.z = relativeVelocity.z * leadScale;
    outTargetPos->x = targetPlayerState->fxOffsetWorld.x + scaledRelativeVelocity.x;
    outTargetPos->y = targetPlayerState->fxOffsetWorld.y + scaledRelativeVelocity.y;
    outTargetPos->z = targetPlayerState->fxOffsetWorld.z + scaledRelativeVelocity.z;
    outTargetPos->y -=
        (static_cast<float>(rand()) * 3.05185094e-05f - 0.5f) * -2.0f;
}

// Reimplements 0x4026d0: Player::UpdateAiMode2MoveAndTurnTowardOffsetTarget
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateAiMode2MoveAndTurnTowardOffsetTarget(
    zUtil_SaveGameState *saveState, zUtil_SaveGameState *targetState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zUtil_PlayerStateStorage *const targetPlayerState = targetState->playerState;
    const float offsetDistance = playerState->aiNet->pursuitParam0;

    zVec3 targetToPlayerDir = {
        playerState->worldPos.x - targetPlayerState->worldPos.x,
        playerState->worldPos.y - targetPlayerState->worldPos.y,
        playerState->worldPos.z - targetPlayerState->worldPos.z,
    };
    targetToPlayerDir.y = 0.0f;
    zMath::Vec3Normalize(&targetToPlayerDir);

    zVec3 steerOffsetDir = {};
    steerOffsetDir.y = 0.0f;
    steerOffsetDir.x =
        offsetDistance *
        (g_Player_AiMode2_OffsetTargetRotateCos15Deg * targetToPlayerDir.x -
         g_Player_AiMode2_OffsetTargetRotateSin15Deg * targetToPlayerDir.z);
    steerOffsetDir.z =
        offsetDistance *
        (g_Player_AiMode2_OffsetTargetRotateCos15Deg * targetToPlayerDir.z +
         g_Player_AiMode2_OffsetTargetRotateSin15Deg * targetToPlayerDir.x);

    zVec3 offsetTarget = {
        targetPlayerState->worldPos.x + steerOffsetDir.x,
        targetPlayerState->worldPos.y + steerOffsetDir.y,
        targetPlayerState->worldPos.z + steerOffsetDir.z,
    };

    zVec3 targetDir = {
        offsetTarget.x - playerState->worldPos.x,
        offsetTarget.y - playerState->worldPos.y,
        offsetTarget.z - playerState->worldPos.z,
    };
    targetDir.y = 0.0f;
    zMath::Vec3Normalize(&targetDir);

    const float forwardDot =
        playerState->steerBasisNorm.x * targetDir.x +
        playerState->steerBasisNorm.z * targetDir.z;
    const float turnCross =
        playerState->steerBasisNorm.z * targetDir.x -
        playerState->steerBasisNorm.x * targetDir.z;

    if (forwardDot < 0.0f) {
        playerState->throttleInput = 0.0f;
        playerState->steeringInput = turnCross < 0.0f ? -1.0f : 1.0f;
    } else {
        float throttle = 1.0f - static_cast<float>(fabs(turnCross));
        if (throttle <= kPlayerAiPathFollowMinThrottle) {
            throttle = kPlayerAiPathFollowMinThrottle;
        }
        playerState->throttleInput = throttle;
        playerState->steeringInput = turnCross;
    }

    playerState->throttleInputCopy = playerState->throttleInput;
    playerState->steeringInputCopy = playerState->steeringInput;
}

// Reimplements 0x4028c0: Player::UpdateAiMode2MoveAndTurnTowardDynamicOffsetTarget
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateAiMode2MoveAndTurnTowardDynamicOffsetTarget(
    zUtil_SaveGameState *saveState, zUtil_SaveGameState *targetState, float targetDistance) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zUtil_PlayerStateStorage *const targetPlayerState = targetState->playerState;
    AINet *const aiNet = playerState->aiNet;
    const float pursuitDistance = aiNet->pursuitParam0;
    const float sideOffsetScale = aiNet->pursuitParam1;
    const float doublePursuitDistance = pursuitDistance + pursuitDistance;

    zVec3 dynamicOffsetDir = playerState->aiDynamicOffsetDir;
    zVec3 targetPoint = {
        targetPlayerState->worldPos.x + pursuitDistance * dynamicOffsetDir.x,
        targetPlayerState->worldPos.y + pursuitDistance * dynamicOffsetDir.y,
        targetPlayerState->worldPos.z + pursuitDistance * dynamicOffsetDir.z,
    };

    float blend = (doublePursuitDistance - targetDistance) / pursuitDistance;
    if (blend > 1.0f) {
        blend = 1.0f;
    } else if (blend < 0.0f) {
        blend = 0.0f;
    }

    int reverseSideOffset = 0;
    float signedSideScale = blend * sideOffsetScale;
    if (playerState->localVel.z > 0.0f && targetDistance < doublePursuitDistance) {
        reverseSideOffset = 1;
        signedSideScale = -signedSideScale;
    }

    zVec3 sideOffset = {
        dynamicOffsetDir.z * signedSideScale,
        targetPoint.y * signedSideScale,
        -dynamicOffsetDir.x * signedSideScale,
    };
    targetPoint.x += sideOffset.x;
    targetPoint.y += sideOffset.y;
    targetPoint.z += sideOffset.z;

    zVec3 targetDir = {
        targetPoint.x - playerState->worldPos.x,
        targetPoint.y - playerState->worldPos.y,
        targetPoint.z - playerState->worldPos.z,
    };
    targetDir.y = 0.0f;
    const float targetDirDistance = zMath::Vec3Normalize(&targetDir);

    zVec3 steerBasis = playerState->steerBasisNorm;
    if (reverseSideOffset != 0) {
        steerBasis.x = -steerBasis.x;
        steerBasis.z = -steerBasis.z;
    }

    const float forwardDot =
        steerBasis.x * targetDir.x + steerBasis.z * targetDir.z;
    const float turnCross =
        steerBasis.z * targetDir.x - steerBasis.x * targetDir.z;

    if (forwardDot < 0.0f &&
        targetDirDistance < kPlayerAiDynamicOffsetBackUpDistance) {
        playerState->throttleInput = -1.0f;
        playerState->steeringInput = 0.0f;
    } else {
        float throttle = 1.0f - static_cast<float>(fabs(turnCross));
        if (throttle <= kPlayerAiPathFollowMinThrottle) {
            throttle = kPlayerAiPathFollowMinThrottle;
        }
        playerState->throttleInput = throttle;
        playerState->steeringInput = turnCross;
    }

    if (reverseSideOffset != 0) {
        playerState->throttleInput = -playerState->throttleInput;
    }
    playerState->throttleInputCopy = playerState->throttleInput;
    playerState->steeringInputCopy = playerState->steeringInput;
}

// Reimplements 0x401a40: Player::TickAiMode2OffsetTargetSteering
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL TickAiMode2OffsetTargetSteering(
    zUtil_SaveGameState *saveState, float unusedForwardDot, float unusedLateralDot,
    float unusedTargetDistance) {
    (void)unusedForwardDot;
    (void)unusedLateralDot;
    (void)unusedTargetDistance;

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (AiMode2ForwardProbeRequiresAutoTurn(saveState) == 0) {
        UpdateAiMode2MoveAndTurnTowardOffsetTarget(
            saveState, (zUtil_SaveGameState *)g_GameStateOrMapTable);
        return;
    }

    zUtil_SaveGameState *const targetSaveState =
        (zUtil_SaveGameState *)g_GameStateOrMapTable;
    SetAutoTurnTargetDirFromWorldPoint(saveState, &targetSaveState->playerState->worldPos);

    const int currentSteeringSubstate = playerState->aiCurrentSteeringSubstate;
    playerState->steeringInputCopy = 0.0f;
    playerState->steeringInput = 0.0f;
    playerState->throttleInputCopy = 0.0f;
    playerState->throttleInput = 0.0f;
    playerState->aiReturnSteeringSubstate = currentSteeringSubstate;
    playerState->aiCurrentSteeringSubstate = kPlayerAiMode2SteerAutoTurn;
}

// Reimplements 0x401ab0: Player::TickAiMode2DynamicOffsetTargetSteering
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL TickAiMode2DynamicOffsetTargetSteering(
    zUtil_SaveGameState *saveState, float unusedForwardDot, float unusedLateralDot,
    float targetDistance) {
    (void)unusedForwardDot;
    (void)unusedLateralDot;

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (AiMode2ForwardProbeRequiresAutoTurn(saveState) == 0) {
        UpdateAiMode2MoveAndTurnTowardDynamicOffsetTarget(
            saveState, (zUtil_SaveGameState *)g_GameStateOrMapTable, targetDistance);
        return;
    }

    zUtil_SaveGameState *const targetSaveState =
        (zUtil_SaveGameState *)g_GameStateOrMapTable;
    SetAutoTurnTargetDirFromWorldPoint(saveState, &targetSaveState->playerState->worldPos);

    const int currentSteeringSubstate = playerState->aiCurrentSteeringSubstate;
    playerState->steeringInputCopy = 0.0f;
    playerState->steeringInput = 0.0f;
    playerState->throttleInputCopy = 0.0f;
    playerState->throttleInput = 0.0f;
    playerState->aiReturnSteeringSubstate = currentSteeringSubstate;
    playerState->aiCurrentSteeringSubstate = kPlayerAiMode2SteerAutoTurn;
}

// Reimplements 0x402080: Player::AiRestoreSavedTopLevelState
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
AiRestoreSavedTopLevelState(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->aiTopLevelState = playerState->aiSavedTopLevelState;
}

// Reimplements 0x402be0: Player::AiSteerTowardPathNodeForward
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
AiSteerTowardPathNodeForward(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    AINetNode *const forwardNode = playerState->aiCurrentPathNode->neighborNodes[0];

    zVec3 targetDir = {
        forwardNode->position.x - playerState->worldPos.x,
        forwardNode->position.y - playerState->worldPos.y,
        forwardNode->position.z - playerState->worldPos.z,
    };
    targetDir.y = 0.0f;
    const float targetDistance = zMath::Vec3Normalize(&targetDir);

    if (targetDistance < kPlayerAiForwardPathAdvanceDistance) {
        playerState->aiCurrentPathNode = forwardNode;
        playerState->throttleInputCopy = 0.0f;
        playerState->throttleInput = 0.0f;
        playerState->steeringInputCopy = 0.0f;
        playerState->steeringInput = 0.0f;
        playerState->unknown_0fa4 = g_Player_TotalTimeSecScaled + 4.0f;
        return;
    }

    const float forwardDot =
        playerState->steerBasisNorm.x * targetDir.x +
        playerState->steerBasisNorm.z * targetDir.z;
    const float turnCross =
        playerState->steerBasisNorm.z * targetDir.x -
        playerState->steerBasisNorm.x * targetDir.z;

    if (forwardDot < 0.0f) {
        playerState->throttleInputCopy = 0.0f;
        playerState->throttleInput = 0.0f;
        playerState->steeringInputCopy = turnCross < 0.0f ? -1.0f : 1.0f;
        playerState->steeringInput = playerState->steeringInputCopy;
        return;
    }

    float throttle = 1.0f - static_cast<float>(fabs(turnCross));
    if (throttle <= kPlayerAiPathFollowMinThrottle) {
        throttle = kPlayerAiPathFollowMinThrottle;
    }
    playerState->throttleInputCopy = throttle;
    playerState->throttleInput = throttle;
    playerState->steeringInputCopy = turnCross;
    playerState->steeringInput = turnCross;
}

// Reimplements 0x402d60: Player::AiSteerTowardPathNodeReverse
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
AiSteerTowardPathNodeReverse(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    AINetNode *const forwardNode = playerState->aiCurrentPathNode->neighborNodes[0];

    zVec3 targetDir = {
        forwardNode->position.x - playerState->worldPos.x,
        forwardNode->position.y - playerState->worldPos.y,
        forwardNode->position.z - playerState->worldPos.z,
    };
    targetDir.y = 0.0f;
    const float targetDistance = zMath::Vec3Normalize(&targetDir);

    if (targetDistance < kPlayerAiForwardPathAdvanceDistance) {
        playerState->aiCurrentPathNode = forwardNode;
        playerState->throttleInputCopy = 0.0f;
        playerState->throttleInput = 0.0f;
        playerState->steeringInputCopy = 0.0f;
        playerState->steeringInput = 0.0f;
        playerState->unknown_0fa4 = g_Player_TotalTimeSecScaled + 14.0f;
        return;
    }

    const zVec3 reverseSteerBasis = {
        -playerState->steerBasisNorm.x,
        playerState->steerBasisNorm.y,
        -playerState->steerBasisNorm.z,
    };
    const float forwardDot =
        reverseSteerBasis.x * targetDir.x + reverseSteerBasis.z * targetDir.z;
    const float turnCross =
        reverseSteerBasis.z * targetDir.x - reverseSteerBasis.x * targetDir.z;

    if (forwardDot < 0.0f) {
        playerState->throttleInputCopy = 0.0f;
        playerState->throttleInput = 0.0f;
        playerState->steeringInputCopy = turnCross < 0.0f ? -1.0f : 1.0f;
        playerState->steeringInput = playerState->steeringInputCopy;
        return;
    }

    float throttle = 1.0f - static_cast<float>(fabs(turnCross));
    if (throttle <= kPlayerAiPathFollowMinThrottle) {
        throttle = kPlayerAiPathFollowMinThrottle;
    }
    throttle = -throttle;
    playerState->throttleInputCopy = throttle;
    playerState->throttleInput = throttle;
    playerState->steeringInputCopy = turnCross;
    playerState->steeringInput = turnCross;
}

// Reimplements 0x402b70: Player::TickAiMode2TimedPathSteering
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
TickAiMode2TimedPathSteering(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    if (g_Player_TotalTimeSecScaled > playerState->unknown_0fa4) {
        AINetNode *const currentPathNode = playerState->aiCurrentPathNode;
        AINetNode *const pathAnchorNode = playerState->aiHomePathNode;

        if (currentPathNode == pathAnchorNode) {
            AiSteerTowardPathNodeForward(saveState);
        } else if (currentPathNode->neighborNodes[0] == pathAnchorNode &&
                   currentPathNode->nodeIndex != -1) {
            AiSteerTowardPathNodeReverse(saveState);
        } else {
            TickAiMode2PathFollow(saveState);
        }
    }

    playerState->recentHitFlag = 1;
}

// Reimplements 0x423530: Player::ClearPendingContactQueues
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
ClearPendingContactQueues(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    FreePendingContactQueue(&playerState->preferredCollisionQueue);
    FreePendingContactQueue(&playerState->playerCollisionQueue);
    FreePendingContactQueue(&playerState->worldCollisionQueue);
    FreePendingContactQueue(&playerState->pickupQueue);
    FreePendingContactQueue(&playerState->checkpointQueue);
    FreePendingContactQueue(&playerState->transferQueue);
}

// Reimplements 0x406730: Player::FilterCameraProbeBlockingHits
// (D:\Proj\GameZRecoil\Player\player_camera.c)
RECOIL_NOINLINE void RECOIL_FASTCALL
FilterCameraProbeBlockingHits(PlayerProbeSampleCandidateBuffer *batches, int batchCount) {
    const int kIgnoredCameraProbeNodeFlag = 0x8000000;
    const int kCallbackContextPresentFlag = 0x100000;
    const int kPlayerCollisionContextKind = 2;

    if (batchCount <= 0) {
        return;
    }

    PlayerProbeSampleCandidateBuffer *batch = batches;
    for (int batchIndex = 0; batchIndex < batchCount; ++batchIndex) {
        for (int hitIndex = 0; hitIndex < batch->candidateCount; ++hitIndex) {
            zClassDiPickCandidateEntry *const candidate = &batch->entries[hitIndex];
            zClass_NodePartial *const node = candidate->node;
            const int flags = node->flags;

            if ((flags & kIgnoredCameraProbeNodeFlag) != 0) {
                candidate->node = 0;
                continue;
            }

            if ((flags & kCallbackContextPresentFlag) != 0 && node->callbackContext != 0) {
                int *const contextKind = (int *)(node->callbackContext);
                if (*contextKind == kPlayerCollisionContextKind) {
                    candidate->node = 0;
                }
            } else if (g_HudSensorTracker.raceCheckpointMode != 0 &&
                       HudSensorTracker::ParseCheckpointNumberFromNode(node) != 0) {
                candidate->node = 0;
            }
        }

        ++batch;
    }
}

// Reimplements 0x42b6e0: Player::FindNearestThirdPersonCameraProbePoint
// (D:\Proj\GameZRecoil\Player\player_camera.c)
RECOIL_NOINLINE int RECOIL_FASTCALL FindNearestThirdPersonCameraProbePoint(
    PlayerProbeSampleCandidateBuffer *batches, int batchCount, const zVec3 *referencePos,
    zVec3 *outHitPos) {
    int found = 0;
    int bestBatchIndex = 0;
    int bestEntryIndex = 0;

    for (int batchIndex = 0; batchIndex < batchCount; ++batchIndex) {
        PlayerProbeSampleCandidateBuffer *const batch = &batches[batchIndex];
        for (int hitIndex = 0; hitIndex < batch->candidateCount; ++hitIndex) {
            if (batch->entries[hitIndex].node != 0) {
                bestBatchIndex = batchIndex;
                bestEntryIndex = hitIndex;
                found = 1;
                batchIndex = batchCount;
                break;
            }
        }
    }

    if (found == 0) {
        return 0;
    }

    float bestDistSq =
        zMath::Vec3DeltaLengthSq(&batches[bestBatchIndex].entries[bestEntryIndex].hitPos,
                                 referencePos);

    for (int batchIndex = 0; batchIndex < batchCount; ++batchIndex) {
        PlayerProbeSampleCandidateBuffer *const batch = &batches[batchIndex];
        for (int hitIndex = 0; hitIndex < batch->candidateCount; ++hitIndex) {
            zClassDiPickCandidateEntry *const candidate = &batch->entries[hitIndex];
            if (candidate->node != 0) {
                const float distSq = zMath::Vec3DeltaLengthSq(&candidate->hitPos, referencePos);
                if (distSq < bestDistSq) {
                    bestDistSq = distSq;
                    bestEntryIndex = hitIndex;
                    bestBatchIndex = batchIndex;
                }
            }
        }
    }

    *outHitPos = batches[bestBatchIndex].entries[bestEntryIndex].hitPos;
    return 1;
}

// Reimplements 0x4067a0: Player::AdjustSubCameraFocusForObstruction
// (D:\Proj\GameZRecoil\Player\player_camera.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
AdjustSubCameraFocusForObstruction(zUtil_SaveGameState *saveState, zVec3 *focusPos) {
    const int kCameraProbeStopAfterFirstHitFlag = 0x40000;
    const float kSubCameraFocusObstructionYOffset = 0.200000003f;

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zClass_DiSegmentEndpoints segmentPairs[2] = {};
    segmentPairs[0].start = playerState->worldPos;
    segmentPairs[0].end = *focusPos;
    segmentPairs[1].start = *focusPos;
    segmentPairs[1].end = playerState->worldPos;

    zClass_Class::gwNodeSetRaycastable(playerState->rootNode, 0);
    zClass_cls_di::SetStopAfterFirstHit(kCameraProbeStopAfterFirstHitFlag);

    PlayerProbeSampleCandidateBuffer probeBatches[2] = {};
    zClass_cls_di::BuildProbeHitBatchesForSegments(g_Player_RuntimeDiScene, segmentPairs, 4,
                                                   probeBatches);

    zClass_Class::gwNodeSetRaycastable(playerState->rootNode, 1);
    FilterCameraProbeBlockingHits(probeBatches, 2);

    zVec3 hitPos = {};
    if (FindNearestThirdPersonCameraProbePoint(probeBatches, 2, &playerState->worldPos,
                                               &hitPos) != 0) {
        focusPos->y -= kSubCameraFocusObstructionYOffset;
        return 1;
    }

    return 0;
}

// Reimplements 0x405ee0: Player::AdjustThirdPersonCameraByOffsetProbes
// (D:\Proj\GameZRecoil\Player\player_camera.c)
RECOIL_NOINLINE int RECOIL_FASTCALL AdjustThirdPersonCameraByOffsetProbes(
    zUtil_SaveGameState *saveState, zVec3 *cameraPos, const zVec3 *sideDir) {
    const int kCameraProbeStopAfterFirstHitFlag = 0x40000;
    const float kCameraSideProbeDistance = 2.0f;
    const float kSubVerticalProbeDistance = 2.0f;

    PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;
    zClass_NodePartial *const rootNode = saveState->playerState->rootNode;

    zVec3 perpDir = {};
    zMath::Vec3PerpXZ(sideDir, &perpDir);
    zVec3 normalizedPerp = {};
    zMath::Vec3NormalizeXZ(&perpDir, &normalizedPerp);
    normalizedPerp.y = 0.0f;

    const zVec3 sideOffset = {
        normalizedPerp.x * kCameraSideProbeDistance,
        0.0f,
        normalizedPerp.z * kCameraSideProbeDistance,
    };

    zClass_DiSegmentEndpoints segmentPairs[3] = {};
    segmentPairs[0].start = *cameraPos;
    segmentPairs[0].end = {
        cameraPos->x + sideOffset.x,
        cameraPos->y + sideOffset.y,
        cameraPos->z + sideOffset.z,
    };
    segmentPairs[1].start = *cameraPos;
    segmentPairs[1].end = {
        cameraPos->x - sideOffset.x,
        cameraPos->y - sideOffset.y,
        cameraPos->z - sideOffset.z,
    };

    int endpointCount = 4;
    if (masterModalData->masterType == kPlayerMasterTypeSub) {
        endpointCount = 6;
        segmentPairs[2].start = *cameraPos;
        segmentPairs[2].end = {
            cameraPos->x,
            cameraPos->y + kSubVerticalProbeDistance,
            cameraPos->z,
        };
    }

    zClass_cls_di::SetStopAfterFirstHit(kCameraProbeStopAfterFirstHitFlag);
    zClass_Class::gwNodeSetRaycastable(rootNode, 0);

    PlayerProbeSampleCandidateBuffer probeBatches[3] = {};
    zClass_cls_di::BuildProbeHitBatchesForSegments(g_Player_RuntimeDiScene, segmentPairs,
                                                   endpointCount, probeBatches);

    zClass_Class::gwNodeSetRaycastable(rootNode, 1);
    FilterCameraProbeBlockingHits(probeBatches, endpointCount >> 1);

    zVec3 outHitPos = {};
    int result = 0;
    if (FindNearestThirdPersonCameraProbePoint(probeBatches, 1, cameraPos, &outHitPos) != 0) {
        result = 1;
        cameraPos->x += outHitPos.x - segmentPairs[0].end.x;
        cameraPos->z += outHitPos.z - segmentPairs[0].end.z;
    } else if (FindNearestThirdPersonCameraProbePoint(&probeBatches[1], 1, cameraPos,
                                                      &outHitPos) != 0) {
        result = 1;
        cameraPos->x += outHitPos.x - segmentPairs[1].end.x;
        cameraPos->z += outHitPos.z - segmentPairs[1].end.z;
    }

    if (masterModalData->masterType == kPlayerMasterTypeSub &&
        FindNearestThirdPersonCameraProbePoint(&probeBatches[2], 1, cameraPos,
                                               &outHitPos) != 0) {
        result |= 1;
        cameraPos->y += outHitPos.y - segmentPairs[2].end.y;
    }

    return result;
}

// Reimplements 0x406510: Player::UpdateCameraVariantFromAnchor
// (D:\Proj\GameZRecoil\Player\player_camera.c)
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateCameraVariantFromAnchor(
    PlayerProbeSampleCandidateBuffer *candidates, zVec3 *cameraPos, int selectedCandidateIndex) {
    (void)cameraPos;

    zUtil_PlayerStateStorage *const playerState =
        static_cast<zUtil_PlayerStateStorage *>(
            static_cast<void *>(g_GameStateOrMapTable->playerState));
    const zTag4Partial playerVariantTag = playerState->variantTag;

    zTag4Partial finalVariantTag = g_Player_LastValidCameraVariantTag;
    if (candidates->candidateCount > 0 &&
        candidates->entries[selectedCandidateIndex].variantTag.count > 0) {
        g_VariantTag_Current = candidates->entries[selectedCandidateIndex].variantTag;

        for (int playerTagIndex = 0; playerTagIndex < playerVariantTag.count;
             ++playerTagIndex) {
            const unsigned char playerTag = playerVariantTag.tags[playerTagIndex];
            int tagAlreadyPresent = 0;
            for (int tagIndex = 0; tagIndex < g_VariantTag_Current.count; ++tagIndex) {
                if (playerTag == g_VariantTag_Current.tags[tagIndex]) {
                    tagAlreadyPresent = 1;
                    break;
                }
            }

            if (tagAlreadyPresent == 0 && g_VariantTag_Current.count < 3) {
                g_VariantTag_Current.tags[g_VariantTag_Current.count] = playerTag;
                ++g_VariantTag_Current.count;
            }
        }

        int tagIsComplete = 1;
        for (int tagIndex = 0; tagIndex < g_VariantTag_Current.count; ++tagIndex) {
            if (g_VariantTag_Current.tags[tagIndex] == 0xff) {
                tagIsComplete = 0;
            }
        }

        if (tagIsComplete != 0) {
            g_Player_LastValidCameraVariantTag = g_VariantTag_Current;
            finalVariantTag = g_VariantTag_Current;
        }
    }

    g_VariantTag_Current = finalVariantTag;
    g_Variant_CurrentTag = finalVariantTag;
    zClass_Camera::gwCameraSetVariantTagOverride(g_MainCamera, &g_VariantTag_Current);
    zEffect::SetVariantOverridePackedIdsIfComplete(&g_VariantTag_Current);
}

// Reimplements 0x406470: Player::UpdateCameraVariantFromCameraPos
// (D:\Proj\GameZRecoil\Player\player_camera.c)
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateCameraVariantFromCameraPos(
    zUtil_SaveGameState *saveState, zVec3 *cameraPos) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerProbeSampleCandidateBuffer candidateBuffers[2] = {};

    zClass_Class::gwNodeSetCellPickable(playerState->rootNode, 0);
    const int pickResult = zClass_cls_di::BuildPickCandidateListBelowPoint(
        g_Player_RuntimeDiScene, candidateBuffers, cameraPos->x, 500.0f, cameraPos->z);
    zClass_Class::gwNodeSetCellPickable(playerState->rootNode, 1);

    if (pickResult == 0) {
        int selectedCandidateIndex = 0;
        int selectedImpactSlot = 0;
        float taggedHeight = 0.0f;
        SelectProbeSampleHeightFromCandidates(candidateBuffers, &selectedCandidateIndex,
                                              cameraPos->y, 0.00100000005f, pickResult,
                                              &selectedImpactSlot, &taggedHeight);
        UpdateCameraVariantFromAnchor(candidateBuffers, cameraPos, selectedCandidateIndex);
    }

    g_Player_CameraVariantUpdatedThisTick = 1;
}

// Reimplements 0x406110: Player::AdjustThirdPersonCameraBySideProbes
// (D:\Proj\GameZRecoil\Player\player_camera.c)
RECOIL_NOINLINE int RECOIL_FASTCALL AdjustThirdPersonCameraBySideProbes(
    zUtil_SaveGameState *saveState, zVec3 *cameraPos, const zVec3 *focusPos,
    zVec3 *cameraDirNext) {
    const int kCameraProbeStopAfterFirstHitFlag = 0x40000;
    const float kSubCameraProbeHeightOffset = 2.20000005f;
    const float kCameraPickMaxY = 500.0f;
    const float kCameraPickRiseWindow = 0.00100000005f;
    const float kCameraFloorLift = 0.5f;

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zClass_NodePartial *const rootNode = playerState->rootNode;
    const zTag4Partial savedVariantTag = g_Variant_CurrentTag;
    int cameraAdjusted = 0;

    zTag4::Clear(&g_Variant_CurrentTag);

    const zVec3 sideProbeOffset = {
        -g_Player_ThirdPersonCameraSideProbeOffsetScale * cameraDirNext->x,
        -g_Player_ThirdPersonCameraSideProbeOffsetScale * cameraDirNext->y,
        -g_Player_ThirdPersonCameraSideProbeOffsetScale * cameraDirNext->z,
    };
    const zVec3 sideProbeEndpoint = {
        cameraPos->x + sideProbeOffset.x,
        cameraPos->y + sideProbeOffset.y,
        cameraPos->z + sideProbeOffset.z,
    };

    zClass_DiSegmentEndpoints segmentPairs[2] = {};
    segmentPairs[0].start = sideProbeEndpoint;
    segmentPairs[0].end = *focusPos;
    segmentPairs[1].start = *focusPos;
    segmentPairs[1].end = sideProbeEndpoint;

    zClass_Class::gwNodeSetRaycastable(rootNode, 0);
    zClass_cls_di::SetStopAfterFirstHit(kCameraProbeStopAfterFirstHitFlag);

    PlayerProbeSampleCandidateBuffer probeBatches[2] = {};
    zClass_cls_di::BuildProbeHitBatchesForSegments(g_Player_RuntimeDiScene, segmentPairs, 4,
                                                   probeBatches);

    zClass_Class::gwNodeSetRaycastable(rootNode, 1);
    FilterCameraProbeBlockingHits(probeBatches, 2);

    zVec3 hitPos = {};
    if (FindNearestThirdPersonCameraProbePoint(probeBatches, 2, focusPos, &hitPos) != 0) {
        cameraPos->x =
            hitPos.x + g_Player_ThirdPersonCameraSideProbeOffsetScale * cameraDirNext->x;
        cameraPos->y =
            hitPos.y + g_Player_ThirdPersonCameraSideProbeOffsetScale * cameraDirNext->y;
        cameraPos->z =
            hitPos.z + g_Player_ThirdPersonCameraSideProbeOffsetScale * cameraDirNext->z;
        cameraAdjusted = 1;
    }

    cameraAdjusted |=
        AdjustThirdPersonCameraByOffsetProbes(saveState, cameraPos, cameraDirNext);

    int preferAttachmentSlot1 = 0;
    if (saveState->primaryModalState->masterModalData->masterType == kPlayerMasterTypeSub) {
        preferAttachmentSlot1 = 1;
        const float subClampY =
            playerState->subModeProbeBestHeight - kSubCameraProbeHeightOffset;
        if (subClampY < cameraPos->y) {
            cameraPos->y = subClampY;
        }
    }

    g_Variant_CurrentTag = savedVariantTag;
    zClass_Class::gwNodeSetCellPickable(rootNode, 0);
    const int pickResult = zClass_cls_di::BuildPickCandidateListBelowPoint(
        g_Player_RuntimeDiScene, probeBatches, cameraPos->x, kCameraPickMaxY, cameraPos->z);
    zClass_Class::gwNodeSetCellPickable(rootNode, 1);
    if (pickResult != 0) {
        return cameraAdjusted;
    }

    int selectedCandidateIndex = 0;
    int selectedImpactSlot = 0;
    float taggedHeight = 0.0f;
    const float selectedHeight = SelectProbeSampleHeightFromCandidates(
        probeBatches, &selectedCandidateIndex, cameraPos->y, kCameraPickRiseWindow,
        preferAttachmentSlot1, &selectedImpactSlot, &taggedHeight);
    UpdateCameraVariantFromAnchor(probeBatches, cameraPos, selectedCandidateIndex);

    const float targetY = selectedHeight + kCameraFloorLift;
    g_Player_CameraVariantUpdatedThisTick = 1;
    if (targetY <= cameraPos->y) {
        return cameraAdjusted;
    }

    cameraPos->y = targetY;

    const float dx = focusPos->x - cameraPos->x;
    const float dy = focusPos->y - cameraPos->y;
    const float dz = focusPos->z - cameraPos->z;
    const float invLength =
        1.0f / static_cast<float>(sqrt(dx * dx + dy * dy + dz * dz));
    cameraDirNext->x = dx * invLength;
    cameraDirNext->y = dy * invLength;
    cameraDirNext->z = dz * invLength;

    return 1;
}

// Reimplements 0x423c20: Player::ClassifyPendingContactsForSegment
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ClassifyPendingContactsForSegment(
    zUtil_SaveGameState *saveState, PlayerProbeSampleCandidateBuffer *sceneResults,
    const zVec3 *segmentStart, const zVec3 *segmentEnd, int segmentTag) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    for (int hitIndex = 0; hitIndex < sceneResults->candidateCount; ++hitIndex) {
        zClassDiPickCandidateEntry *const candidate = &sceneResults->entries[hitIndex];
        zClass_NodePartial *node = candidate->node;
        PlayerPendingContact *queuedContact = 0;

        if (g_HudSensorTracker.raceCheckpointMode != 0) {
            const int checkpointNumber =
                HudSensorTracker::ParseCheckpointNumberFromNode(candidate->node);
            g_PlayerPendingCheckpointNumber = checkpointNumber;
            if (checkpointNumber != 0) {
                queuedContact = AppendPendingContact(&playerState->checkpointQueue);
                CopyPendingContactPayload(queuedContact, candidate, segmentStart, segmentEnd,
                                          segmentTag);
                continue;
            }
        }

        if ((node->flags & 0x8000000) != 0) {
            continue;
        }

        if (Pickup::ResolveOwnerFromBvolHit(&candidate->node) != 0) {
            queuedContact = AppendPendingContact(&playerState->pickupQueue);
        } else {
            node = candidate->node;
            if ((node->flags & 0x100000) != 0 && node->callbackContext != 0) {
                int *const playerType = (int *)(node->callbackContext);
                if (*playerType == 2) {
                    queuedContact = AppendPendingContact(&playerState->playerCollisionQueue);
                }
            } else {
                OptCatalogDamageHandlerPartial *const damageHandler = GetNodeDamageHandler(node);
                if (damageHandler != 0 &&
                    damageHandler != (OptCatalogDamageHandlerPartial *)(1) &&
                    damageHandler->timerContext != 0) {
                    queuedContact = AppendPendingContact(&playerState->transferQueue);
                } else if (candidate->surfaceNormal.y < -0.9f) {
                    queuedContact = AppendPendingContact(&playerState->worldCollisionQueue);
                } else if (candidate->surfaceNormal.y < 0.71f) {
                    queuedContact = AppendPendingContact(&playerState->preferredCollisionQueue);
                } else {
                    PlayerContactSurfacePayload *const scenePayload =
                        static_cast<PlayerContactSurfacePayload *>(candidate->scenePayload);
                    const int impactSlot = scenePayload != 0 ? scenePayload->impactSlot : 0;
                    if (impactSlot == 5 && playerState->recentHitValid == 0) {
                        playerState->recentHitValid = 1;
                    }
                }
            }
        }

        if (queuedContact != 0) {
            CopyPendingContactPayload(queuedContact, candidate, segmentStart, segmentEnd,
                                      segmentTag);
        }
    }
}

// Reimplements 0x423b10: Player::CollectPendingContactsForSegments
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL CollectPendingContactsForSegments(
    zUtil_SaveGameState *saveState, zClass_DiSegmentEndpoints *segmentPairs, int endpointCount,
    int *segmentTags) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    zClass_Class::gwNodeSetRaycastable(playerState->rootNode, 0);
    g_Variant_CurrentTag = playerState->variantTag;

    PlayerProbeSampleCandidateBuffer hitBatches[24] = {};
    zClass_cls_di::BuildProbeHitBatchesForSegments(g_Player_RuntimeDiScene, segmentPairs,
                                                   endpointCount, hitBatches);

    g_Variant_CurrentTag = g_VariantTag_Current;
    zClass_Class::gwNodeSetRaycastable(playerState->rootNode, 1);

    for (int endpointIndex = 0; endpointIndex < endpointCount; endpointIndex += 2) {
        const int segmentIndex = endpointIndex >> 1;
        ClassifyPendingContactsForSegment(saveState, &hitBatches[segmentIndex],
                                          &segmentPairs[segmentIndex].start,
                                          &segmentPairs[segmentIndex].end,
                                          segmentTags[segmentIndex]);
    }

    return playerState->preferredCollisionQueue.count == 0 &&
                   playerState->playerCollisionQueue.count == 0 &&
                   playerState->worldCollisionQueue.count == 0 &&
                   playerState->transferQueue.count == 0 &&
                   playerState->checkpointQueue.count == 0 &&
                   playerState->pickupQueue.count == 0
               ? 1
               : 0;
}

// Reimplements 0x424210: Player::ProcessPendingPickupContacts
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
ProcessPendingPickupContacts(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if ((zInput_GameStateOrMapTablePartial *)(saveState) != g_GameStateOrMapTable) {
        return;
    }

    if (playerState->lifecycleState == 4 || playerState->lifecycleState == 5) {
        return;
    }

    PlayerPendingContact *contact = playerState->pickupQueue.head;
    while (contact != 0) {
        if (PlayerPickupContact::PassesCollectionTest(saveState, contact) != 0) {
            Pickup::OnCollected(contact->hit.node, saveState);
        }

        contact = contact != 0 ? contact->next : 0;
    }
}

} // namespace Player

namespace PlayerPickupContact {

// Reimplements 0x424150: PlayerPickupContact::PassesCollectionTest
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL PassesCollectionTest(
    zUtil_SaveGameState *saveState, PlayerPendingContact *contact) {
    (void)saveState;
    zUtil_PlayerStateStorage *const playerState =
        static_cast<zUtil_PlayerStateStorage *>(static_cast<void *>(g_GameStateOrMapTable->playerState));
    zClass_NodePartial *const pickupNode = contact->hit.node;

    g_Variant_CurrentTag = playerState->variantTag;
    zClass_Class::gwNodeSetRaycastable(pickupNode, 0);
    zClass_cls_di::SetBreakOnFirstCandidate(1);
    zClass_cls_di::SetStopAfterFirstHit(0x40000);

    const zVec3 startPoint = {
        contact->hit.hitPos.x,
        contact->hit.hitPos.y - 1.0f,
        contact->hit.hitPos.z,
    };
    const zVec3 endPoint = {
        pickupNode->cachedSphereCenter[0],
        pickupNode->cachedSphereCenter[1] - 1.0f,
        pickupNode->cachedSphereCenter[2],
    };

    PlayerProbeSampleCandidateBuffer rayData = {};
    const int raycastResult = zClass_cls_di::RaycastFindClosest(
        g_Player_RuntimeDiScene, &rayData, startPoint.x, startPoint.y, startPoint.z, endPoint.x,
        endPoint.y, endPoint.z);

    zClass_cls_di::SetBreakOnFirstCandidate(0);
    zClass_Class::gwNodeSetRaycastable(pickupNode, 1);

    return raycastResult == 0 && rayData.candidateCount != 0 ? 0 : 1;
}

} // namespace PlayerPickupContact

namespace Player {

// Reimplements 0x4251f0: Player::CollectPendingCollisionContactsForQuadProbe
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
CollectPendingCollisionContactsForQuadProbe(zUtil_SaveGameState *saveState, float expandRadius) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    const PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;

    ClearPendingContactQueues(saveState);

    enum {
        kQuadProbePointCount = 4,
        kQuadProbeSegmentCount = 6
    };

    const int probeIndices[kQuadProbePointCount] = {0, 2, 3, 5};
    for (int i = 0; i < kQuadProbePointCount; ++i) {
        zVec3 probePoint = masterModalData->probePoints[probeIndices[i]];
        probePoint.y += expandRadius;
        playerState->modalProbeWorldByIndex[probeIndices[i]] =
            TransformPointByMatrix(probePoint, playerState->motionBasis);
    }

    zClass_DiSegmentEndpoints segmentPairs[kQuadProbeSegmentCount];
    int segmentTags[kQuadProbeSegmentCount] = {0, 1, 2, 3, 4, 5};
    segmentPairs[0].start = playerState->modalProbeWorldByIndex[0];
    segmentPairs[0].end = playerState->modalProbeWorldByIndex[2];
    segmentPairs[1].start = playerState->modalProbeWorldByIndex[2];
    segmentPairs[1].end = playerState->modalProbeWorldByIndex[0];
    segmentPairs[2].start = playerState->modalProbeWorldByIndex[2];
    segmentPairs[2].end = playerState->modalProbeWorldByIndex[3];
    segmentPairs[3].start = playerState->modalProbeWorldByIndex[3];
    segmentPairs[3].end = playerState->modalProbeWorldByIndex[2];
    segmentPairs[4].start = playerState->modalProbeWorldByIndex[3];
    segmentPairs[4].end = playerState->modalProbeWorldByIndex[5];
    segmentPairs[5].start = playerState->modalProbeWorldByIndex[5];
    segmentPairs[5].end = playerState->modalProbeWorldByIndex[3];

    CollectPendingContactsForSegments(saveState, segmentPairs, kQuadProbeSegmentCount * 2,
                                      segmentTags);

    MoveTransferContactsToPreferredCollision(playerState);

    return playerState->preferredCollisionQueue.count != 0 ||
                   playerState->playerCollisionQueue.count != 0
               ? 1
               : 0;
}

// Reimplements 0x424ed0: Player::TryResolvePendingCollisionProbeSweep
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
TryResolvePendingCollisionProbeSweep(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    ClearPendingContactQueues(saveState);

    zClass_DiSegmentEndpoints segmentPairs[6];
    int segmentTags[6];
    for (int i = 0; i < 6; ++i) {
        segmentPairs[i].start = playerState->rootProbeWorldByIndex[i];
        segmentPairs[i].end = playerState->modalProbeWorldByIndex[i];
        segmentTags[i] = i;
    }

    CollectPendingContactsForSegments(saveState, segmentPairs, 12, segmentTags);
    MoveTransferContactsToPreferredCollision(playerState);

    if (playerState->preferredCollisionQueue.count == 0 &&
        playerState->playerCollisionQueue.count == 0) {
        playerState->collisionProbeResolved = 0;
        return 0;
    }

    ApplyPendingCollisionProbeVelocity(saveState);
    playerState->collisionProbeResolved = 1;
    return 1;
}

// Reimplements 0x423fc0: Player::SelectAndResolvePreferredPendingCollisionContact
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
SelectAndResolvePreferredPendingCollisionContact(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerPendingContact *selectedContact = playerState->preferredCollisionQueue.head;
    PlayerPendingContact *contact = selectedContact->next;
    while (contact != 0) {
        selectedContact = selectedContact->SelectPreferred(contact);
        contact = contact->next;
    }

    ResolvePendingCollisionContact(saveState, selectedContact);
    playerState->preferredCollisionResolved = 1;
}

// Reimplements 0x4248e0: Player::PreparePendingWorldCollisionResponse
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL PreparePendingWorldCollisionResponse(
    zUtil_SaveGameState *saveState, PlayerPendingContact *worldContacts) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;

    if (playerState->airborneFlag != 0 && playerState->projectileSpawnVel.y > 0.0f) {
        playerState->projectileSpawnVel.y = -playerState->projectileSpawnVel.y;
        playerState->localVel.y = playerState->projectileSpawnVel.y;
        playerState->worldPos.y = playerState->previousTransform.posY;
        while (worldContacts != 0) {
            playerState->worldPos.y -= kPlayerWorldCollisionStackDrop;
            worldContacts = worldContacts->next;
        }
        playerState->motionBasis.posY = playerState->worldPos.y;
        return;
    }

    const float restoreYOffset =
        masterModalData->masterType == kPlayerMasterTypeSub ? kPlayerWorldCollisionSubRestoreYOffset
                                                            : 0.0f;
    playerState->worldPos.x = playerState->previousTransform.posX;
    playerState->worldPos.y = playerState->previousTransform.posY + restoreYOffset;
    playerState->worldPos.z = playerState->previousTransform.posZ;
    playerState->vehiclePitchRad = playerState->cachedPitchRad;
    playerState->restartYawRad = playerState->cachedYawRad;
    playerState->vehicleRollRad = playerState->cachedRollRad;
    playerState->angVelPitch = 0.0f;
    playerState->angVelYaw = 0.0f;
    playerState->angVelRoll = 0.0f;

    if (playerState->projectileSpawnVel.y > 0.0f) {
        playerState->projectileSpawnVel.y *= kPlayerWorldCollisionUpwardBounceDamping;
    }

    zMath::MatBuildEulerRotation3x3(&playerState->motionBasis, playerState->vehiclePitchRad,
                                    playerState->restartYawRad, playerState->vehicleRollRad);
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;

    const zVec3 projectileVel = playerState->projectileSpawnVel;
    const zMat4x3 &motionBasis = playerState->motionBasis;
    playerState->localVel.x = projectileVel.x * motionBasis.xx +
                              projectileVel.y * motionBasis.xy +
                              projectileVel.z * motionBasis.xz;
    playerState->localVel.y = projectileVel.x * motionBasis.yx +
                              projectileVel.y * motionBasis.yy +
                              projectileVel.z * motionBasis.yz;
    playerState->localVel.z = projectileVel.x * motionBasis.zx +
                              projectileVel.y * motionBasis.zy +
                              projectileVel.z * motionBasis.zz;
}

// Reimplements 0x424110: Player::ResolvePendingWorldCollisionContact
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
ResolvePendingWorldCollisionContact(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerPendingContact *const contact = playerState->worldCollisionQueue.head;
    PreparePendingWorldCollisionResponse(saveState, contact);
    if (playerState->lifecycleState == kPlayerLifecycleLocal) {
        saveState->StartModalLoopSfxHandle(4, 1.0f);
    }
    ResolvePendingCollisionContact(saveState, playerState->worldCollisionQueue.head);
}

// Reimplements 0x429430: Player::ApplyPitchRollVelocityImpulseFromDirection
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ApplyPitchRollVelocityImpulseFromDirection(
    zUtil_SaveGameState *saveState, const zVec3 *direction, float angleScale,
    float velocityScale) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zVec3 localDirection = *direction;

    zMat4x3 slotBuffer;
    zMath::MatStackPushPtr((float *)(&slotBuffer));
    zMath::MatLoadRotationFrom3x3(
        (const zMat4x3 *)(zClass_Object3D::gwObject3DGetMatrixPtr(playerState->rootNode)));
    zMath::Vec3ArrayTransformDirection(&localDirection, 1);
    zMath::MatStackPopPtr();

    playerState->vehiclePitchRad -= localDirection.z * angleScale;
    playerState->vehicleRollRad += localDirection.x * angleScale;
    playerState->localVel.x -= localDirection.x * velocityScale;
    playerState->localVel.z -= localDirection.z * velocityScale;
}

// Reimplements 0x424270: Player::ResolvePendingCollisionContact
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
ResolvePendingCollisionContact(zUtil_SaveGameState *saveState, PlayerPendingContact *contact) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;
    zClass_NodePartial *const hitNode = contact->hit.node;

    zVec3 sweepStart = contact->sweepStart;
    zVec3 sweepEnd = contact->sweepEnd;
    zVec3 contactPoint = contact->hit.hitPos;
    zVec3 contactNormal = contact->hit.surfaceNormal;

    const zVec3 contactToSweepStart = {sweepStart.x - contactPoint.x,
                                       sweepStart.y - contactPoint.y,
                                       sweepStart.z - contactPoint.z};
    if (Vec3Dot(contactToSweepStart, contactNormal) < 0.0f) {
        contactNormal.x *= -1.0f;
        contactNormal.y *= -1.0f;
        contactNormal.z *= -1.0f;
    }

    if (contactNormal.x == 0.0f && contactNormal.z == 0.0f) {
        return;
    }

    const float localSpeed = Vec3Length(playerState->localVel);
    const float originalNormalY = contactNormal.y;
    sweepStart.y = 0.0f;
    sweepEnd.y = 0.0f;
    contactPoint.y = 0.0f;
    contactNormal.y = 0.0f;

    zVec3 contactToSweepEnd = {sweepEnd.x - contactPoint.x,
                               sweepEnd.y - contactPoint.y,
                               sweepEnd.z - contactPoint.z};
    zMath::Vec3NormalizeXZ(&contactNormal, &contactNormal);

    zVec3 reflectedSweepDir;
    zMath::Vec3Reflect(&contactNormal, &contactToSweepEnd, &reflectedSweepDir);
    const zVec3 reflectedContactPoint = {contactPoint.x + reflectedSweepDir.x,
                                         contactPoint.y + reflectedSweepDir.y,
                                         contactPoint.z + reflectedSweepDir.z};
    zVec3 worldPosCorrection = {reflectedContactPoint.x - sweepEnd.x,
                                reflectedContactPoint.y - sweepEnd.y,
                                reflectedContactPoint.z - sweepEnd.z};
    Vec3_FastNormalize(&worldPosCorrection);

    playerState->worldPos.x += worldPosCorrection.x;
    playerState->worldPos.y += worldPosCorrection.y;
    playerState->worldPos.z += worldPosCorrection.z;

    for (int i = 0; i < masterModalData->probePointCount; ++i) {
        playerState->modalProbeWorldByIndex[i].x += worldPosCorrection.x;
        playerState->modalProbeWorldByIndex[i].y += worldPosCorrection.y;
        playerState->modalProbeWorldByIndex[i].z += worldPosCorrection.z;
    }

    const int probeResolved = TryResolvePendingCollisionProbeSweep(saveState);
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posZ = playerState->worldPos.z;

    if (probeResolved == 0) {
        const float collisionDampingA = masterModalData->collisionDampingA;
        float projectileVelY = playerState->projectileSpawnVel.y;
        zMath::Vec3NormalizeXZ(&reflectedSweepDir, &reflectedSweepDir);

        zVec3 surfaceTangent = Vec3Cross(reflectedSweepDir, contactNormal);
        surfaceTangent = Vec3Cross(contactNormal, surfaceTangent);

        reflectedSweepDir.x *= localSpeed;
        reflectedSweepDir.y *= localSpeed;
        reflectedSweepDir.z *= localSpeed;

        const float tangentSpeed = Vec3DotXZ(reflectedSweepDir, surfaceTangent);
        const zVec3 tangentVelocityDelta = {surfaceTangent.x * tangentSpeed,
                                            surfaceTangent.y * tangentSpeed,
                                            surfaceTangent.z * tangentSpeed};
        const float normalSpeed = collisionDampingA * Vec3DotXZ(reflectedSweepDir, contactNormal);
        const zVec3 normalVelocityDelta = {contactNormal.x * normalSpeed,
                                           contactNormal.y * normalSpeed,
                                           contactNormal.z * normalSpeed};

        playerState->projectileSpawnVel.x = normalVelocityDelta.x + tangentVelocityDelta.x;
        playerState->projectileSpawnVel.y = normalVelocityDelta.y + tangentVelocityDelta.y;
        playerState->projectileSpawnVel.z = normalVelocityDelta.z + tangentVelocityDelta.z;

        if (playerState->airborneFlag != 0) {
            if (originalNormalY > 0.01f) {
                if (projectileVelY < 0.0f) {
                    projectileVelY *= -0.5f;
                }
                zClass_Class::gwNodeSetCellPickable(hitNode, 1);
            }

            if (Vec3Dot(playerState->projectileSpawnVel,
                        playerState->projectileSpawnVel) < 1.0f) {
                playerState->projectileSpawnVel.x = contactNormal.x * 10.0f;
                playerState->projectileSpawnVel.y = contactNormal.y * 10.0f;
                playerState->projectileSpawnVel.z = contactNormal.z * 10.0f;
            }
        }

        playerState->projectileSpawnVel.y = projectileVelY;
        zMath::Vec3RotateY(&playerState->localVel, &playerState->projectileSpawnVel,
                           -playerState->restartYawRad);
    }

    const float yawImpulseCross =
        reflectedSweepDir.x * contactToSweepEnd.z - reflectedSweepDir.z * contactToSweepEnd.x;
    const int yawImpulseSign = yawImpulseCross < 0.0f ? -1 : 1;
    playerState->angVelYaw +=
        static_cast<float>(yawImpulseSign) * localSpeed * masterModalData->collisionDampingB;

    if (saveState != (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        return;
    }

    float impactGain = localSpeed / masterModalData->maxSpeed;
    if (impactGain > 1.0f) {
        impactGain = 1.0f;
    }
    saveState->StartModalLoopSfxHandle(4, impactGain);
    if (zInput_DI_IsForceFeedbackEnabled() != 0 && g_zInputFfEffectSet != 0) {
        g_zInputFfEffectSet->PlayCollisionImpactEffect(&contactNormal, impactGain);
    }
}

// Reimplements 0x424ac0: Player::ResolvePendingPlayerCollisionContact
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
ResolvePendingPlayerCollisionContact(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;

    PlayerPendingContact *const queuedContact = playerState->playerCollisionQueue.head;
    zClassDiPickCandidateEntry contactSnapshot = queuedContact->hit;
    zVec3 transferredLocalVel = playerState->projectileSpawnVel;

    PlayerCollisionContactContextPartial *const targetContext =
        (PlayerCollisionContactContextPartial *)(void *)(contactSnapshot.node->callbackContext);
    zUtil_SaveGameState *const targetSaveState = targetContext->saveState;
    zUtil_PlayerStateStorage *const targetPlayerState = targetSaveState->playerState;
    PlayerMasterCommonData *const targetCommonData = targetPlayerState->masterCommonData;
    PlayerMasterModalData *const targetModalData =
        targetSaveState->primaryModalState->masterModalData;

    const float massScale = masterModalData->mass * targetModalData->invMass;
    transferredLocalVel.x *= massScale;
    transferredLocalVel.y *= massScale;
    transferredLocalVel.z *= massScale;
    zMath::Vec3RotateY(&transferredLocalVel, &transferredLocalVel,
                       -targetPlayerState->restartYawRad);
    transferredLocalVel.y = 0.0f;

    targetPlayerState->localVel.x += transferredLocalVel.x;
    targetPlayerState->localVel.y += transferredLocalVel.y;
    targetPlayerState->localVel.z += transferredLocalVel.z;

    ResolvePendingCollisionContact(saveState, playerState->playerCollisionQueue.head);

    if (targetPlayerState->lifecycleState == kPlayerLifecycleAi) {
        const float damage = massScale * 1.10000002f;
        const float remainingFraction =
            (targetPlayerState->statusMeterValue - damage) * targetCommonData->invMaxHealth;
        if (remainingFraction > 0.200000003f) {
            HitCallback_RecordContextAndTimedStatus(targetSaveState, 0, 0, damage);
        }
    }
}

// Reimplements 0x424d00: Player::ProcessTransferContactQueue
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ProcessTransferContactQueue(
    zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;
    const float localSpeedSq = playerState->localVel.x * playerState->localVel.x +
                               playerState->localVel.y * playerState->localVel.y +
                               playerState->localVel.z * playerState->localVel.z;
    const float transferDamage =
        (localSpeedSq * kPlayerTransferDamageScale) /
        (masterModalData->maxSpeed * masterModalData->maxSpeed);

    PlayerPendingContact *contact = playerState->transferQueue.head;
    while (contact != 0) {
        PlayerPendingContact *const next = contact->next;
        const float callbackResult = OptCatalog::CaptureHitSnapshotAndInvokeDamageTimerCallback(
            &contact->sweepStart, (OptCatalogHitEventPartial *)(void *)contact, transferDamage);
        if (callbackResult > 0.0f) {
            RemoveExistingPendingContact(&playerState->transferQueue, contact);
            AppendExistingPendingContact(&playerState->preferredCollisionQueue, contact);
        } else {
            zClass_NodePartial *const hitNode = contact->hit.node;
            RecordNodeFlagsForRestore(hitNode);
            zClass_Class::gwNodeSetCellPickable(hitNode, 0);
            zClass_Class::gwNodeSetRaycastable(hitNode, 0);
        }
        contact = next;
    }

    playerState->localVel.x *= kPlayerTransferVelocityDamping;
    playerState->localVel.y *= kPlayerTransferVelocityDamping;
    playerState->localVel.z *= kPlayerTransferVelocityDamping;
    playerState->projectileSpawnVel.x *= kPlayerTransferVelocityDamping;
    playerState->projectileSpawnVel.y *= kPlayerTransferVelocityDamping;
    playerState->projectileSpawnVel.z *= kPlayerTransferVelocityDamping;
}

// Reimplements 0x43b730: Player::RecordRecentHitFeedback
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
RecordRecentHitFeedback(zUtil_SaveGameState *saveState, OptCatalogEntryDef *hitSource,
                        float damage) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->recentHitValid = 1;
    playerState->lastHitOwnerOrCtx = HitContext::GetCurrentOwnerOrCtx();
    playerState->recentHitSource = hitSource;
    playerState->recentHitDamage = damage;
    playerState->recentHitFxExpireTime = g_Time_AccumulatedTimeSec + 4.0f;

    zEffectAnimEntry *const recentHitLightHandle = playerState->recentHitLightHandle;
    if (recentHitLightHandle != 0) {
        zEffectAnim::Stop(recentHitLightHandle);
    }

    playerState->recentHitLightHandle =
        zEffectAnim::SetPositionRefAndVelocity_Thunk(g_PlayerRecentHitFxAnimEntry, 0,
                                                     playerState->rootNode, 0, 0);
}

// Reimplements 0x43b800: Player::ClearDestroyedRespawnEffectHandleCallback
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ClearDestroyedRespawnEffectHandleCallback(
    zEffectAnimEntry *, zUtil_SaveGameState *saveState, int) {
    saveState->playerState->destroyedRespawnAsyncHandle = 0;
}

// Reimplements 0x41bd20: Player::DestroyedStateResetLocalFinalize
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_CDECL DestroyedStateResetLocalFinalize() {
    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)g_GameStateOrMapTable;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (playerState->lifecycleState == kPlayerLifecycleInactive) {
        playerState->lifecycleState = kPlayerLifecycleLocal;
        zOpt::SetSteeringMode(g_PlayerPrevSteeringMode);
        ApplyCameraState(g_PlayerPrevCameraState);
        ResetMouseControlStateAndRecenterCursor(saveState);
        ResetDamageStateAndTimedHitStatus(saveState);
    }

    Pickup::ApplyEffect(0x386, 0, saveState);
}

// Reimplements 0x41bca0: Player::DestroyedStateResetFinalizeCallback
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
DestroyedStateResetFinalizeCallback(zUtil_SaveGameState *saveState) {
    zUtil_SaveGameState *nearestSaveState = saveState;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterCommonData *const masterCommonData = playerState->masterCommonData;

    const float nearestDistanceSq = GameNet::GetNearestOtherPlayerDistanceToSpawnPoint(
        (GameNetSpawnPoint *)&playerState->worldPos,
        (GameNetPlayerSaveState **)&nearestSaveState);
    if (nearestDistanceSq < 20.0f &&
        saveState->netPlayerRow->playerColorIndex <
            nearestSaveState->netPlayerRow->playerColorIndex) {
        GameNet::RespawnPlayerAndDropWeaponPickupIfAllowed(saveState, 0);
    }

    DestroyedStateResetLocalFinalize();
    playerState->lifecycleState = kPlayerLifecycleLocal;
    playerState->statusMeterValue = masterCommonData->maxHealth;
    playerState->cameraTransitionTimer = 0;
}

// Reimplements 0x41bbf0: Player::DestroyedStateResetCallback
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
DestroyedStateResetCallback(zEffectAnimEntry *, zUtil_SaveGameState *saveState, int) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zEffect_Anim::NodeActionCallback(playerState->destroyedRespawnFxEntry,
                                     playerState->rootNode);
    ResetDamageStateAndTimedHitStatus(saveState);

    playerState->statusMeterValue = playerState->masterCommonData->maxHealth;
    zClass_Object3D::gwObject3DSetLitFlag(playerState->rootNode, 1);
    zClass_Object3D::gwObject3DSetAlphaScale(playerState->rootNode, 0.0f);
    zClass_Object3D_ModelRefLerpQueue::Add(
        playerState->rootNode, saveState, (void *)(&DestroyedStateResetFinalizeCallback), 0.0f,
        1.0f, 1.0f);

    playerState->aiMode = 0;
    playerState->nextModeSwitchAllowedTime = 0.0f;
    playerState->autoTurnSign = 0;
    playerState->motionInput = 0;
    TransitionToMasterTypeTrack(saveState, 0);
    GameNet::RespawnPlayerAndDropWeaponPickupIfAllowed(saveState, 0);
    LoadWeaponBanksAndSelectDefaults(saveState);
    RefreshHudFromState(saveState);
    ResetAltGunDoorAnimationState(saveState);
    HudUiMgr::TriggerCurrentLayoutOnActivated();
}

// Reimplements 0x41bd10: Player::ClearRespawnTransitionFlagCallback
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
ClearRespawnTransitionFlagCallback(zUtil_SaveGameState *saveState) {
    saveState->playerState->cameraTransitionTimer = 0;
}

// Reimplements 0x41bb30: Player::DestroyedStateRespawnCallback
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
DestroyedStateRespawnCallback(zEffectAnimEntry *, zUtil_SaveGameState *saveState, int) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zClass_Object3D::gwObject3DSetLitFlag(playerState->rootNode, 1);
    zClass_Object3D::gwObject3DSetAlphaScale(playerState->rootNode, 0.0f);
    zClass_Object3D_ModelRefLerpQueue::Add(
        playerState->rootNode, saveState, (void *)(&ClearRespawnTransitionFlagCallback), 0.0f,
        1.0f, 5.0f);

    zClass_NodePartial *const healthyNode =
        zClass_Class::FindNodeRecursiveByName(playerState->rootNode, "healthy");
    if (healthyNode != 0) {
        zClass_Object3D::gwObject3DSetPosition(healthyNode, 0.0f, 0.0f, 0.0f);
        zClass_Object3D::gwObject3DSetRotation(healthyNode, 0.0f, 0.0f, 0.0f);
    }

    if (playerState->destroyedRespawnAsyncHandle != 0) {
        zEffect_Anim::NodeActionCallback(playerState->destroyedRespawnAsyncHandle,
                                         playerState->rootNode);
    }

    ResetDamageStateAndTimedHitStatus(saveState);
    playerState->cachedAltSelectionCode = 0;
    playerState->statusMeterValue = playerState->masterCommonData->maxHealth;
    playerState->cachedPrimarySelectionCode = 0;
}

// Reimplements 0x43bc40: Player::EnterLocalInactiveDestroyedLifecycle
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
EnterLocalInactiveDestroyedLifecycle(zUtil_SaveGameState *saveState) {
    if (saveState != (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        return;
    }

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->lifecycleState = kPlayerLifecycleInactive;
    playerState->altGunTransitionState = 1;
    playerState->altGunTransitionController = 0;
    playerState->altGunTransitionTimerA = 0.0f;

    zEffectAnimEntry *const destroyedRespawnHandle =
        zEffectAnim::SetVelocity_Thunk(playerState->destroyedRespawnFxEntry,
                                       playerState->rootNode, 0.0f, 0.0f, 0.0f);
    StopBftBubbleFxHandle(saveState);

    if (zOpt::GetNetworkEnabled() != 0) {
        playerState->cameraTransitionTimer = 1;
        zEffectAnimEntry::SetOnStateDoneCallback(
            destroyedRespawnHandle, (void *)(&DestroyedStateResetCallback), saveState);
    }
}

// Reimplements 0x43bcc0: Player::EnterDestroyedState
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL EnterDestroyedState(
    zUtil_SaveGameState *saveState, OptCatalogEntryDef *hitSource,
    OptCatalogHitEventPartial *hitRenderPoint, float damage) {
    OptCatalogEntryDef *killEventContext = hitSource;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterCommonData *const masterCommonData = playerState->masterCommonData;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;

    if (playerState->transitionDamageSuppressed != 0 ||
        playerState->lifecycleState == kPlayerLifecycleInactive ||
        playerState->lifecycleState == kPlayerLifecycleDestroyed) {
        return 0;
    }

    if (hitSource != 0) {
        if ((hitSource->flags & kOptCatalogFlagRecordsRecentHit) != 0) {
            RecordRecentHitFeedback(saveState, hitSource, damage);
        }
        if ((hitSource->flags & kOptCatalogFlagAppliesTimedHitStatus) != 0) {
            damage = UpdateTimedHitStatusFromHitSource(saveState, hitSource, damage);
        }
    }

    if (damage != 0.0f) {
        if (playerState->damageProtectionActive != 0 &&
            (hitSource == 0 ||
             (hitSource->flags & kOptCatalogFlagBypassDamageProtection) == 0)) {
            damage = masterCommonData->maxHealth;
        }

        ApplyStatusMeterChange(saveState, 1, -damage);
        if (g_PlayerStatusMeterRatio <= 0.0f) {
            const int nanitePanelLevel = playerState->nanitePanelLevel;
            if (nanitePanelLevel != 0 &&
                nanitePanelLevel != kPlayerNanitePanelDisabledSentinel) {
                playerState->nanitePanelLevel = nanitePanelLevel - 1;
                HudUiMgr::SetNanitePanelCount(nanitePanelLevel - 1);
            }
            UpdateStatusMeter(saveState, 0, 0.0f);
        }
    }

    playerState->statusMeterScaled = 1.0f;
    if (playerState->statusMeterValue <= 0.0f) {
        if (saveState != (zUtil_SaveGameState *)g_GameStateOrMapTable) {
            return 0;
        }

        saveState->UpdateModalLoopSfx(0);
        if (playerState->cameraState == kPlayerCameraStateProjectileAttached) {
            ApplyCameraState(kPlayerCameraStateRestorePrevious);
        }
        g_PlayerPrevSteeringMode = zOpt::GetSteeringMode();
        g_PlayerPrevCameraState = playerState->cameraState;
        zOpt::SetSteeringMode(kPlayerCameraStateThirdPerson);
        ApplyCameraState(kPlayerCameraStateThirdPerson);
        HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x23e), 5.0f);
        playerState->statusMeterValue = 0.0f;
        g_HudSensorTracker.menuTransitionDelaySec = g_Time_AccumulatedTimeSec;
        ResetAltGunRuntimeState(saveState);

        if (hitSource != 0 && hitRenderPoint != 0) {
            OptCatalog::SetDamageContext(
                1, (OptCatalogHitEventPartial *)(void *)(&playerState->selectedProbeSample));
        }

        if (playerState->airborneFlag != 0) {
            playerState->lifecycleState = kPlayerLifecycleDestroyed;
        } else {
            EnterLocalInactiveDestroyedLifecycle(saveState);
        }

        if (zOpt::GetNetworkEnabled() != 0) {
            void *ownerOrCtx = 0;
            if (hitRenderPoint != 0) {
                ownerOrCtx = HitContext::GetCurrentOwnerOrCtx();
            } else if (playerState->recentHitValid != 0) {
                ownerOrCtx = playerState->lastHitOwnerOrCtx;
                killEventContext = playerState->recentHitSource;
            }

            HitOwnerOrContextPartial *const hitOwner =
                static_cast<HitOwnerOrContextPartial *>(ownerOrCtx);
            if (hitOwner != 0 && hitOwner->ownerLink != 0 &&
                hitOwner->ownerLink->ownerSaveState != 0) {
                GameNet::SendPkt08_PlayerKillEvent(
                    hitOwner->ownerLink->ownerSaveState,
                    static_cast<short>(killEventContext->ordinalIndex));
            } else {
                GameNet::SendPkt08_PlayerKillEvent(saveState, 0);
            }
        }

        HudLowMeterLoopSound::Disable();
        return 0;
    }

    if (hitSource != 0) {
        OptCatalog::SetDamageContext(
            0, (OptCatalogHitEventPartial *)(void *)(&playerState->selectedProbeSample));
        const unsigned int flags = hitSource->flags;
        if ((flags & kOptCatalogFlagAppliesTimedHitStatus) == 0) {
            if ((flags & kOptCatalogFlagNoSubUse) == 0) {
                const zVec3 *const sourcePos = OptCatalog::GetCapturedHitSourcePtr();
                const zVec3 *const hitPos = &g_OptCatalog_CapturedDamageHitPos;
                zVec3 direction = {sourcePos->x - hitPos->x, sourcePos->y - hitPos->y,
                                   sourcePos->z - hitPos->z};
                const float length =
                    sqrt(direction.x * direction.x + direction.y * direction.y +
                         direction.z * direction.z);
                const float invLength = 1.0f / length;
                direction.x *= invLength;
                direction.y *= invLength;
                direction.z *= invLength;

                if (damage > 5.0f) {
                    const float impulseBase = masterModalData->invMass * damage;
                    ApplyPitchRollVelocityImpulseFromDirection(
                        saveState, &direction, impulseBase * 0.00499999989f,
                        impulseBase * 0.333000004f);
                }

                if (zInput_DI_IsForceFeedbackEnabled() != 0) {
                    g_zInputFfEffectSet->PlayDamageHitEffect(&direction,
                                                             damage * 0.0500000007f);
                }
            }

            if (hitRenderPoint != 0) {
                OptCatalog::ApplyDamageMaskStampOnHit(hitRenderPoint);
            }
        }
    }

    return playerState->recentHitValid;
}

// Reimplements 0x43b790: Player::UpdateTimedHitStatusFromHitSource
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE float RECOIL_FASTCALL
UpdateTimedHitStatusFromHitSource(zUtil_SaveGameState *saveState, OptCatalogEntryDef *hitSource,
                                  float damage) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterCommonData *const masterCommonData = playerState->masterCommonData;
    if ((hitSource->flags & 0x800u) != 0) {
        HitSource::UpdateTimedStatus(hitSource, &playerState->timedHitStatus,
                                     masterCommonData->maxHealth);
        return damage;
    }

    const float contribution = masterCommonData->invMaxHealth * damage;
    if (HitSource::UpdateTimedStatus(hitSource, &playerState->timedHitStatus, contribution) == 1) {
        return 0.0f;
    }
    return damage;
}

// Reimplements 0x43b810: Player::HitCallback_RecordNetContextAndTimedStatus
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL HitCallback_RecordNetContextAndTimedStatus(
    zUtil_SaveGameState *saveState, OptCatalogEntryDef *hitSource, void *, float damage) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (playerState->lifecycleState == kPlayerLifecycleInactive) {
        return 0;
    }

    if (hitSource != 0) {
        if ((hitSource->flags & kOptCatalogFlagRecordsRecentHit) != 0) {
            RecordRecentHitFeedback(saveState, hitSource, damage);
        }
        if ((hitSource->flags & kOptCatalogFlagAppliesTimedHitStatus) != 0) {
            UpdateTimedHitStatusFromHitSource(saveState, hitSource, damage);
        }
    }

    return playerState->recentHitValid;
}

// Reimplements 0x43c010: Player::ApplyDamageLocal
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL ApplyDamageLocal(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    if (playerState->statusMeterValue > 0.0f) {
        DamageFeedback::SetIntensityScalar(playerState->masterCommonData->invMaxHealth *
                                           playerState->statusMeterValue);
        return 0;
    }

    zEffectAnimEntry *const destroyedRespawnHandle =
        zEffectAnim::SetVelocity_Thunk(playerState->destroyedRespawnFxEntry,
                                       playerState->rootNode, 0.0f, 0.0f, 0.0f);
    playerState->destroyedRespawnAsyncHandle = destroyedRespawnHandle;
    zEffectAnimEntry::SetOnStateDoneCallback(
        destroyedRespawnHandle, (void *)(&DestroyedStateRespawnCallback), saveState);

    if (playerState->recentHitValid != 0) {
        zEffect_Anim::NodeActionCallback(playerState->recentHitLightHandle, 0);
        playerState->recentHitLightHandle = 0;
        playerState->recentHitValid = 0;
    }

    ResetAltGunRuntimeState(saveState);
    OptCatalog::SetDamageContext(
        1, (OptCatalogHitEventPartial *)(void *)(&playerState->selectedProbeSample));
    HudUiMgr::HideTrackedProgressMeterIfOwnerMatches(saveState);
    return 1;
}

// Reimplements 0x41b950: Player::TickRemoteNetworkPlayer
// (GameZRecoil/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL TickRemoteNetworkPlayer(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    if (playerState->netUpdateReceived != 0) {
        SampleGroundAndAlignRootToSurface(saveState, 0);
        playerState->netUpdateReceived = 0;
    }

    playerState->fxOffsetWorld.x = playerState->worldPos.x + playerState->fxOffsetLocal.x;
    playerState->fxOffsetWorld.y = playerState->worldPos.y + playerState->fxOffsetLocal.y;
    playerState->fxOffsetWorld.z = playerState->worldPos.z + playerState->fxOffsetLocal.z;

    GameNet::UpdateRemotePlayerHudWidgetScreenPos(saveState);

    if (playerState->cameraTransitionTimer != 0) {
        playerState->worldPos = playerState->netReceivedPos;
        playerState->vehiclePitchRad = playerState->netReceivedAngles.x;
        playerState->restartYawRad = playerState->netReceivedAngles.y;
        playerState->vehicleRollRad = playerState->netReceivedAngles.z;
    } else {
        zMath::Vec3Lerp(&playerState->worldPos, &playerState->netReceivedPos, 0.649999976f);
        playerState->vehiclePitchRad = playerState->netReceivedAngles.x;
        playerState->restartYawRad = playerState->netReceivedAngles.y;
        playerState->vehicleRollRad = playerState->netReceivedAngles.z;

        if (playerState->lifecycleState != kPlayerLifecycleDestroyed) {
            UpdateAltGunAimDirection(saveState);
            UpdateGunDispatchRequestsFromTriggerLatches(saveState);
            TickAltGunRuntimeState(saveState);
        }

        ResetDamageVisualsAndTimedStatus(saveState);
        if (ApplyDamageLocal(saveState) != 0) {
            playerState->cameraTransitionTimer = 1;
        }
    }

    zClass_Object3D::gwObject3DSetPosition(playerState->rootNode, playerState->worldPos.x,
                                           playerState->worldPos.y, playerState->worldPos.z);
    zClass_Object3D::gwObject3DSetRotation(playerState->rootNode, playerState->vehiclePitchRad,
                                           playerState->restartYawRad,
                                           playerState->vehicleRollRad);
}

// Reimplements 0x43b870: Player::HitCallback_RecordContextAndTimedStatus
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL HitCallback_RecordContextAndTimedStatus(
    zUtil_SaveGameState *saveState, OptCatalogEntryDef *hitSource, void *hitRenderPointEntry,
    float damage) {
    OptCatalogEntryDef *killEventContext = hitSource;
    int pickupRewardMultiplier = 1;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    PlayerMasterCommonData *const masterCommonData = playerState->masterCommonData;

    if (playerState->lifecycleState == kPlayerLifecycleInactive) {
        return 0;
    }

    void *ownerOrCtx = 0;
    if (hitSource != 0) {
        ownerOrCtx = HitContext::GetCurrentOwnerOrCtx();
    } else if (playerState->recentHitValid != 0) {
        killEventContext = playerState->recentHitSource;
        hitSource = killEventContext;
        ownerOrCtx = playerState->lastHitOwnerOrCtx;
    }

    if (ownerOrCtx != 0) {
        HitOwnerOrContextPartial *const hitOwner =
            static_cast<HitOwnerOrContextPartial *>(ownerOrCtx);
        HitOwnerSaveStateLinkPartial *const ownerLink = hitOwner->ownerLink;
        if (ownerLink != 0 && ownerLink->ownerSaveState == saveState) {
            return 0;
        }
    }

    if (hitSource != 0) {
        if ((hitSource->flags & kOptCatalogFlagRecordsRecentHit) != 0) {
            RecordRecentHitFeedback(saveState, hitSource, damage);
        }
        if ((hitSource->flags & kOptCatalogFlagAppliesTimedHitStatus) != 0) {
            damage = UpdateTimedHitStatusFromHitSource(saveState, hitSource, damage);
        }
    }

    if (damage != 0.0f) {
        if (playerState->damageProtectionActive != 0 &&
            (hitSource == 0 ||
             (hitSource->flags & kOptCatalogFlagBypassDamageProtection) == 0)) {
            playerState->statusMeterValue = 0.0f;
            pickupRewardMultiplier = 2;
        } else {
            playerState->statusMeterValue -= damage;
        }
    }
    playerState->statusMeterScaled =
        masterCommonData->invMaxHealth * playerState->statusMeterValue;

    if (playerState->statusMeterValue <= 0.0f) {
        playerState->lifecycleState = kPlayerLifecycleInactive;
        playerState->statusMeterValue = 0.0f;

        if (zSnd::GetAudioApiOption() == 1) {
            saveState->UpdateModalLoopSfx(0);
        }
        AiDiscardNegativeBranchPathNodes(saveState);
        StartDestroyedStateVehicleEffect(saveState,
                                         (void *)ClearDestroyedRespawnEffectHandleCallback);
        ResetAltGunRuntimeState(saveState);

        if (killEventContext != 0 && hitRenderPointEntry != 0) {
            OptCatalog::SetDamageContext(
                1, (OptCatalogHitEventPartial *)(void *)(&playerState->selectedProbeSample));
        }
        AddScaledHudCounterValue(masterCommonData->maxHealth);

        int spawnedNaniteReward = 0;
        zUtil_SaveGameState *const localSaveState = (zUtil_SaveGameState *)g_GameStateOrMapTable;
        if (localSaveState->playerState->nanitePanelLevel != kPlayerNanitePanelDisabledSentinel &&
            masterCommonData->naniteBuildRate != 0) {
            ++masterCommonData->naniteSpawnCounter;
            if (masterCommonData->naniteSpawnCounter >= masterCommonData->naniteBuildRate) {
                zVec3 spawnPos = playerState->worldPos;
                spawnPos.y -= masterModalData->modeAltTransitionTime;
                zClass_cls_di::SnapProbePointYToBestCandidate(&spawnPos);
                Pickup::SpawnAt(34, masterCommonData->naniteMaxLevel, &spawnPos, 0, 0);
                masterCommonData->naniteSpawnCounter = 0;
                spawnedNaniteReward = 1;
            }
        }

        if (localSaveState->playerState->activeAltGunController->ammoOrCharge !=
                kPlayerAltAmmoDisabledSentinel &&
            spawnedNaniteReward == 0) {
            zVec3 spawnPos = playerState->worldPos;
            spawnPos.y -= masterModalData->modeAltTransitionTime;
            zClass_cls_di::SnapProbePointYToBestCandidate(&spawnPos);
            Pickup::SpawnAt(masterCommonData->pickupType,
                            masterCommonData->pickupCapacity * pickupRewardMultiplier, &spawnPos,
                            0, 0);
        }

        ++g_HudSensorTracker.missionStat0;
        return 0;
    }

    DamageFeedback::SetIntensityScalar(masterCommonData->invMaxHealth *
                                       playerState->statusMeterValue);

    if (playerState->lifecycleState == kPlayerLifecycleAi &&
        playerState->aiTopLevelState != kPlayerAiMode2TopSteering) {
        AiEnterMode2SteeringPursuit(saveState);
        if (playerState->aiRuntime != 0 && playerState->aiRuntime->attackBuddyNetId != 0) {
            AiAlertAttackBuddies(saveState);
        }
        playerState->recentHitFlag = 1;
        playerState->recentHitExpireTime = g_Time_AccumulatedTimeSec + kPlayerRecentHitAlertSec;
    }

    if (killEventContext != 0) {
        OptCatalog::SetDamageContext(
            0, (OptCatalogHitEventPartial *)(void *)(&playerState->selectedProbeSample));
        if (damage > 5.0f &&
            (killEventContext->flags & kOptCatalogFlagAppliesTimedHitStatus) == 0 &&
            (killEventContext->flags & kOptCatalogFlagNoSubUse) == 0) {
            const float impulseBase = masterModalData->invMass * damage;
            const float angleScale = impulseBase * 0.0250000004f;
            const float velocityScale = impulseBase * 1.66700006f;
            const zVec3 *const sourcePos = OptCatalog::GetCapturedHitSourcePtr();
            const zVec3 *const hitPos = &g_OptCatalog_CapturedDamageHitPos;
            zVec3 direction = {sourcePos->x - hitPos->x, sourcePos->y - hitPos->y,
                               sourcePos->z - hitPos->z};
            const float length =
                sqrt(direction.x * direction.x + direction.y * direction.y +
                     direction.z * direction.z);
            const float invLength = 1.0f / length;
            direction.x *= invLength;
            direction.y *= invLength;
            direction.z *= invLength;
            ApplyPitchRollVelocityImpulseFromDirection(saveState, &direction, angleScale,
                                                       velocityScale);
        }
    }

    return playerState->recentHitValid;
}

// Reimplements 0x423380: Player::IsMissionProbeType1EnabledById
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL IsMissionProbeType1EnabledById(int missionId) {
    return missionId == 9 || missionId == 11 || missionId == 12 || missionId == 13;
}

// Reimplements 0x43c0c0: Player::StartDestroyedStateVehicleEffect
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
StartDestroyedStateVehicleEffect(zUtil_SaveGameState *saveState, void *respawnCallback) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zEffectAnimEntry *vehicleEffect;
    zClass_NodePartial *rootNode;

    playerState->destroyedRespawnAsyncHandle = 0;
    if (playerState->queuedFixedDamageFlag != 0) {
        vehicleEffect = playerState->shockVehicleFxEntry;
        rootNode = 0;
    } else if (playerState->damageProtectionActive != 0) {
        vehicleEffect = playerState->shatterVehicleFxEntry;
        rootNode = playerState->rootNode;
    } else if (playerState->recentHitValid != 0) {
        vehicleEffect = playerState->napalmVehicleFxEntry;
        rootNode = playerState->rootNode;
    } else if (playerState->aiMode != 0) {
        vehicleEffect = playerState->subTransitionFxEntry;
        rootNode = playerState->rootNode;
    } else {
        vehicleEffect = playerState->destroyedRespawnFxEntry;
        rootNode = playerState->rootNode;
    }

    zEffectAnimEntry *const asyncHandle =
        zEffectAnim::SetVelocity_Thunk(vehicleEffect, rootNode, 0.0f, 0.0f, 0.0f);
    playerState->destroyedRespawnAsyncHandle = asyncHandle;

    if (playerState->recentHitValid != 0) {
        zEffect_Anim::NodeActionCallback(playerState->recentHitLightHandle, 0);
        playerState->recentHitLightHandle = 0;
        playerState->recentHitValid = 0;
    }

    if (respawnCallback != 0) {
        zEffectAnimEntry::SetOnStateDoneCallback(asyncHandle, respawnCallback, saveState);
    }

    HudUiMgr::HideTrackedProgressMeterIfOwnerMatches(saveState);
}

// Reimplements 0x4236b0: Player::BuildPendingContactQueues
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL BuildPendingContactQueues(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    int enabledSegmentFlags[15];

    const float localVelLengthSq = playerState->localVel.x * playerState->localVel.x +
                                   playerState->localVel.y * playerState->localVel.y +
                                   playerState->localVel.z * playerState->localVel.z;
    playerState->noPendingContactsQueued = 1;
    memset(enabledSegmentFlags, 0, sizeof(enabledSegmentFlags));

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        if (fabs(playerState->angVelYaw) > 0.0f) {
            EnableContactSegment(enabledSegmentFlags, 0);
            EnableContactSegment(enabledSegmentFlags, 1);
            EnableContactSegment(enabledSegmentFlags, 2);
            EnableContactSegment(enabledSegmentFlags, 3);
            EnableContactSegment(enabledSegmentFlags, 4);
            EnableContactSegment(enabledSegmentFlags, 5);
        }
    } else if (fabs(playerState->localVel.z) <
               fabs(playerState->angVelYaw * 3.29999995f)) {
        EnableContactSegment(enabledSegmentFlags, 0);
        EnableContactSegment(enabledSegmentFlags, 1);
        EnableContactSegment(enabledSegmentFlags, 2);
        EnableContactSegment(enabledSegmentFlags, 3);
        EnableContactSegment(enabledSegmentFlags, 4);
        EnableContactSegment(enabledSegmentFlags, 5);
    }

    if (localVelLengthSq > 0.0000001f) {
        if (playerState->localVel.z < 0.0f) {
            EnableContactSegment(enabledSegmentFlags, 0);
            EnableContactSegment(enabledSegmentFlags, 1);
            EnableContactSegment(enabledSegmentFlags, 2);
        } else {
            EnableContactSegment(enabledSegmentFlags, 3);
            EnableContactSegment(enabledSegmentFlags, 5);
        }

        if (masterModalData->masterType == kPlayerMasterTypeSub &&
            playerState->localVel.y > 0.001f) {
            EnableContactSegment(enabledSegmentFlags, 0);
            EnableContactSegment(enabledSegmentFlags, 1);
            EnableContactSegment(enabledSegmentFlags, 2);
            EnableContactSegment(enabledSegmentFlags, 3);
            EnableContactSegment(enabledSegmentFlags, 5);
        }

        if (playerState->localVel.x > 0.001f) {
            EnableContactSegment(enabledSegmentFlags, 6);
            EnableContactSegment(enabledSegmentFlags, 7);
            EnableContactSegment(enabledSegmentFlags, 8);
            EnableContactSegment(enabledSegmentFlags, 4);
        } else {
            if (playerState->localVel.x < -0.001f) {
                EnableContactSegment(enabledSegmentFlags, 9);
                EnableContactSegment(enabledSegmentFlags, 10);
                EnableContactSegment(enabledSegmentFlags, 11);
            }
            EnableContactSegment(enabledSegmentFlags, 4);
        }
    }

    BuildModalAndRootProbeWorldCaches(playerState, masterModalData);

    zClass_DiSegmentEndpoints segmentPairs[15];
    int segmentTags[15];
    int segmentCount = 0;
    const float probeYAdvance = playerState->projectileSpawnVel.y * g_Player_DeltaTime;

    for (int i = 0; i < 15; ++i) {
        if (enabledSegmentFlags[i] == 0) {
            continue;
        }

        segmentTags[segmentCount] = i;
        zVec3 *const modalPoint = &playerState->modalProbeWorldByIndex[i];
        zVec3 *const rootPoint = &playerState->rootProbeWorldByIndex[i];
        ConstrainToUnitDistanceFrom(rootPoint, modalPoint);
        segmentPairs[segmentCount].start = *rootPoint;
        segmentPairs[segmentCount].end = *modalPoint;
        segmentPairs[segmentCount].end.y += probeYAdvance;
        ++segmentCount;
    }

    if (segmentCount != 0) {
        playerState->noPendingContactsQueued =
            CollectPendingContactsForSegments(saveState, segmentPairs, segmentCount * 2,
                                              segmentTags);
    }

    if (masterModalData->masterType != kPlayerMasterTypeSub) {
        return;
    }

    segmentCount = 0;
    for (int i = 0; i < 15; ++i) {
        if (enabledSegmentFlags[i] == 0) {
            continue;
        }

        segmentPairs[segmentCount].start = playerState->rootProbeWorldByIndex[i];
        segmentPairs[segmentCount].start.y -= -3.0f;
        segmentPairs[segmentCount].end = playerState->modalProbeWorldByIndex[i];
        segmentPairs[segmentCount].end.y += probeYAdvance - -3.0f;
        ++segmentCount;
    }

    if (segmentCount != 0) {
        playerState->noPendingContactsQueued =
            CollectPendingContactsForSegments(saveState, segmentPairs, segmentCount * 2,
                                              segmentTags);
    }
}

// Reimplements 0x423460: Player::ProcessPendingContactQueues
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
ProcessPendingContactQueues(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    playerState->pickupQueueProcessed = 0;
    playerState->playerCollisionResolved = 0;
    playerState->worldCollisionResolved = 0;
    playerState->preferredCollisionResolved = 0;
    playerState->checkpointLapProgressNotified = 0;

    ClearPendingContactQueues(saveState);
    BuildPendingContactQueues(saveState);
    if (playerState->noPendingContactsQueued != 0) {
        return;
    }

    if (playerState->checkpointQueue.count != 0) {
        Checkpoint::UpdatePlayerLapProgressAndNotifyNet(saveState,
                                                        g_PlayerPendingCheckpointNumber);
        playerState->checkpointLapProgressNotified = 1;
    }

    if (playerState->pickupQueue.count != 0) {
        ProcessPendingPickupContacts(saveState);
        playerState->pickupQueueProcessed = 1;
    }

    if (playerState->playerCollisionQueue.count != 0) {
        ResolvePendingPlayerCollisionContact(saveState);
        playerState->playerCollisionResolved = 1;
    }

    if (playerState->worldCollisionQueue.count != 0) {
        ResolvePendingWorldCollisionContact(saveState);
        playerState->worldCollisionResolved = 1;
    }

    if (playerState->transferQueue.count != 0) {
        ProcessTransferContactQueue(saveState);
    }

    if (playerState->preferredCollisionQueue.count != 0) {
        SelectAndResolvePreferredPendingCollisionContact(saveState);
    }

    ClearPendingContactQueues(saveState);
}

// Reimplements 0x425770: Player::ApplyPendingCollisionProbeVelocity
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
ApplyPendingCollisionProbeVelocity(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    if (playerState->collisionProbeResolved == 0) {
        playerState->worldPos.x = playerState->previousTransform.posX;
        playerState->worldPos.y = playerState->previousTransform.posY;
        playerState->worldPos.z = playerState->previousTransform.posZ;
        playerState->restartYawRad = playerState->cachedYawRad;
        zMath::MatBuildEulerRotation3x3(&playerState->motionBasis,
                                        playerState->vehiclePitchRad,
                                        playerState->cachedYawRad,
                                        playerState->vehicleRollRad);
        playerState->motionBasis.posX = playerState->worldPos.x;
        playerState->motionBasis.posY = playerState->worldPos.y;
        playerState->motionBasis.posZ = playerState->worldPos.z;
    }

    PlayerPendingContact *contact = playerState->preferredCollisionQueue.head;
    if (contact == 0) {
        contact = playerState->playerCollisionQueue.head;
    }
    if (contact == 0) {
        return;
    }

    const float previousY = playerState->projectileSpawnVel.y;
    const zVec3 surfaceNormal = contact->hit.surfaceNormal;
    playerState->projectileSpawnVel.x = surfaceNormal.x * 20.0f;
    playerState->projectileSpawnVel.y = surfaceNormal.y * 20.0f;
    playerState->projectileSpawnVel.z = surfaceNormal.z * 20.0f;

    if (playerState->projectileSpawnVel.y > 0.0f) {
        if (previousY > playerState->projectileSpawnVel.y) {
            playerState->projectileSpawnVel.y = previousY;
        }
    } else if (previousY < playerState->projectileSpawnVel.y) {
        playerState->projectileSpawnVel.y = previousY;
    }

    const zVec3 pushVel = playerState->projectileSpawnVel;
    const zMat4x3 &motionBasis = playerState->motionBasis;
    playerState->localVel.x = pushVel.x * motionBasis.xx +
                              pushVel.y * motionBasis.xy +
                              pushVel.z * motionBasis.xz;
    playerState->localVel.y = pushVel.x * motionBasis.yx +
                              pushVel.y * motionBasis.yy +
                              pushVel.z * motionBasis.yz;
    playerState->localVel.z = pushVel.x * motionBasis.zx +
                              pushVel.y * motionBasis.zy +
                              pushVel.z * motionBasis.zz;
}

// Reimplements 0x425920: Player::RegisterGameplayCommandCallbacksAndCreateFfEffects
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_CDECL RegisterGameplayCommandCallbacksAndCreateFfEffects() {
    // zInput's keyboard bridge tail-jumps to these handlers with commandId in ECX.
    zInputCommandCallbackFn hudHotkeyCallback =
        (zInputCommandCallbackFn)(HudUi::HandleHotkeyCommand);
    zInput::BindMap_Current_SetCommandCallback(30, hudHotkeyCallback);
    zInput::BindMap_Current_SetCommandCallback(9, hudHotkeyCallback);
    zInput::BindMap_Current_SetCommandCallback(32, hudHotkeyCallback);
    zInput::BindMap_Current_SetCommandCallback(33, hudHotkeyCallback);

    if (zVid::GetAccelerationOption() == 0) {
        zInput::BindMap_Current_SetCommandCallback(
            34, (zInputCommandCallbackFn)(zVideo::HandleSoftwareModeHotkeyCommand));
    }

    zInput::BindMap_Current_SetCommandCallback(35, hudHotkeyCallback);
    zInput::BindMap_Current_SetCommandCallback(42, hudHotkeyCallback);
    zInput::BindMap_Current_SetCommandCallback(43, hudHotkeyCallback);
    zInput::BindMap_Current_SetCommandCallback(44, hudHotkeyCallback);
    zInput::BindMap_Current_SetCommandCallback(45, hudHotkeyCallback);

    zInput_FFEffectSet *const effectSet = new zInput_FFEffectSet;
    if (effectSet != 0) {
        g_zInputFfEffectSet = zInput_DI_InitForceFeedbackEffectSet(effectSet);
    } else {
        g_zInputFfEffectSet = 0;
    }
}

// Reimplements 0x42bb30: Player::AsyncCommandCallback
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL AsyncCommandCallback(zEffectAnimEntry *animEntry,
                                                          void *, int eventCode) {
    zUtil_SaveGameState *const localSaveState = (zUtil_SaveGameState *)g_GameStateOrMapTable;

    switch (eventCode) {
    case 0:
        if (animEntry == g_Player_ActiveDebugScriptAsyncEntry) {
            g_Player_ActiveDebugScriptAsyncEntry = 0;
        }
        return;

    case 1:
        g_Player_RebuildCameraDirFlatFromCurrentTarget = 1;
        BindActiveGameStateAsCurrentSaveState();
        return;

    case 2:
        UnbindCurrentSaveStateIfSinglePlayer();
        HudUiMgr::DisableHud();
        HudUiMgr::UpdateTargetReticleFromCursor(0, 0, 0.0f, 0.0f);
        HudUiMgr::DisableTopAndChatStacks();
        return;

    case 10:
        SyncLocalPoseFromRootNode();
        HudUiMgr::EnableTopAndChatStacks();
        return;

    case 11:
        if (zOpt::GetNetworkEnabled() == 0) {
            localSaveState->playerState->lifecycleState = kPlayerLifecycleState6Inactive;
            localSaveState->UpdateModalLoopSfx(0);
        }
        return;

    case 14:
        if (zOpt::GetNetworkEnabled() == 0) {
            g_Player_LocalControlEnabled = 0;
            HudUiMgr::DisableHud();
            HudUiMgr::UpdateTargetReticleFromCursor(0, 0, 0.0f, 0.0f);
            HudUiTimerPanel::SetRunning(0);
            HudUiMgr::TriggerCurrentLayoutOnActivated();
        }
        zTurret_System::DisableTickCallback();
        return;

    case 15:
        if (zOpt::GetNetworkEnabled() == 0) {
            g_Player_LocalControlEnabled = 1;
            if (zOpt::GetHudVisibilityOption() != 0) {
                HudUiMgr::ApplyHudModeSwitch(zOpt::GetHudTypeForCurrentHwMode());
                HudUiMgr::EnableHud();
            }
            HudUiMgr::UpdateTargetReticleFromCursor(1, 0, 0.5f, 0.5f);
            HudUi::ShowTopMessageLine(
                localSaveState->playerState->activeAltGunController->optCatalogEntry->description,
                5.0f);
            HudUiTimerPanel::SetRunning(1);
            HudUiMgr::TriggerCurrentLayoutOnActivated();
            if (g_HudSensorTracker.GetMissionId() == 1 &&
                g_HudSensorTracker.firstIncompleteObjectiveIndex == 0 &&
                g_HudSensorTracker.primaryGunDispatchCount == 0) {
                HudUi::PlayPowerupSfx(1);
            }
        }
        zTurret_System::EnableTickCallback();
        return;

    case 16:
        ResetMotionTransientState(localSaveState);
        return;

    case 17:
        CaptureCurrentObjectPoseAsRestartAnchor(localSaveState);
        return;

    case 20:
        g_Player_ActiveDebugScriptAsyncEntry = animEntry;
        return;

    case 25:
        localSaveState->playerState->nanitePanelLevel = 0;
        HudUiMgr::SetNanitePanelCount(0);
        EnterDestroyedState(localSaveState, 0, 0,
                            localSaveState->playerState->statusMeterValue - -1.0f);
        return;

    case 26:
        EnterDestroyedState(localSaveState, 0, 0,
                            localSaveState->playerState->statusMeterValue - -1.0f);
        return;

    case 27:
        EnterDestroyedState(localSaveState, 0, 0, 10.0f);
        return;

    case 99:
        g_HudSensorTracker.SaveAndQueueMissionState();
        return;

    case 911:
        PickupAirdropSpawnRef::TrySpawnRandomPickupFromGlobal();
        return;

    case 912:
        Pickup::SpawnAtCarrierNodeByName("vwbus", 32, 1);
        return;

    case 913:
        Pickup::SpawnAtCarrierNodeByName("crbox", 36, 1);
        return;

    case 914:
        Pickup::SpawnAtCarrierNodeByName("drop", 30, 1);
        return;

    default:
        return;
    }
}

// Reimplements 0x402f10: Player::AiFinalizeMode2State1ForAllPlayers
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_CDECL AiFinalizeMode2State1ForAllPlayers() {
    zUtil_SaveGameState *saveState = g_PlayerSaveStateListHead;
    while (saveState != 0) {
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        if (playerState->lifecycleState == 2 && playerState->aiTopLevelState == 1) {
            AiRestoreSavedTopLevelState(saveState);
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

// Reimplements 0x42be70: Player::CaptureCurrentObjectPoseAsRestartAnchor
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
CaptureCurrentObjectPoseAsRestartAnchor(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = g_LocalPlayerSaveState->playerState;

    zVec3 worldPos;
    zClass_Object3D::gwObject3DGetPosition(playerState->rootNode, &worldPos.x, &worldPos.y,
                                           &worldPos.z);

    float pitchRad;
    float yawRad;
    float rollRad;
    zClass_Object3D::gwObject3DGetRotation(playerState->rootNode, &pitchRad, &yawRad, &rollRad);

    SetWorldPoseAndRestartAnchor(saveState, &worldPos, yawRad);
}

// Reimplements 0x426330: Player::ResetMouseControlStateAndRecenterCursor
// (D:\Proj\GameZRecoil\zGame\Player\Player_Camera.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
ResetMouseControlStateAndRecenterCursor(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->thirdPersonYawOffset = 0.0f;
    playerState->cameraElevationOffset = 0.0f;
    zInput::Mouse_RecenterCursor();
}

// Reimplements 0x426390: PlayerMgr::TickAllPlayers
// (GameZRecoil/player.cpp)
RECOIL_NOINLINE void RECOIL_CDECL TickAllPlayers() {
    g_Player_DeltaTime =
        g_FrameDeltaTimeSec >= kPlayerMinFrameDeltaSec ? g_FrameDeltaTimeSec
                                                       : kPlayerMinFrameDeltaSec;
    g_Player_InvDeltaTime = 1.0f / g_Player_DeltaTime;
    g_Player_TotalTimeSecScaled = g_Time_AccumulatedTimeSec;
    g_Player_DeltaTimeScaled001 = g_Player_DeltaTime * kPlayerDeltaTimeScaled001Factor;

    int totalMode2Count = 0;
    int activeMode2Count = 0;
    zUtil_SaveGameState *saveState = g_PlayerSaveStateListHead;
    while (saveState != 0) {
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        const int lifecycleState = playerState->lifecycleState;
        playerState->generalFlags |= kPlayerPerFrameGeneralFlag;

        if (lifecycleState == kPlayerLifecycleInactive ||
            lifecycleState == kPlayerLifecycleState6Inactive) {
            if (playerState->altGunFireHeldFlag != 0) {
                PlayerGunFireController *const activeAltGunController =
                    playerState->activeAltGunController;
                playerState->altGunFireHeldFlag = 0;
                OptCatalog::DeactivateTrailRuntimeState(
                    activeAltGunController->trailRuntimeState);
            }

            if (saveState == g_LocalPlayerSaveState) {
                TickLocalPlayerControls(saveState);
            }

            if (playerState->cameraTickEnabled != 0) {
                TickActiveCameraState(saveState);
            }

            if (zOpt::GetNetworkEnabled() != 0 &&
                saveState != (zUtil_SaveGameState *)g_GameStateOrMapTable &&
                saveState != g_Player2SaveState) {
                zClass_Class::gwNodeSetActive(playerState->rootNode, 0);
            }
        } else if (lifecycleState == kPlayerLifecycleRemote) {
            TickRemoteNetworkPlayer(saveState);
        } else {
            if (saveState == g_LocalPlayerSaveState) {
                TickLocalPlayerControls(saveState);
            } else if (lifecycleState == kPlayerLifecycleAi) {
                ++totalMode2Count;
                if (VariantTag::TagsOverlap(&playerState->variantTag, &g_VariantTag_Current) !=
                    0) {
                    zUtil_PlayerStateStorage *const localPlayerState =
                        ((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState;
                    const float targetDistanceSq =
                        zMath::Vec3DistSqXZ(&playerState->worldPos,
                                            &localPlayerState->worldPos);
                    playerState->targetDistanceSq = targetDistanceSq;

                    if ((targetDistanceSq <= playerState->aiActivationRadiusSq ||
                         playerState->recentHitFlag != 0) &&
                        playerState->aiTickSuppressed == 0) {
                        playerState->aiActive = 1;
                        ++activeMode2Count;
                        TickAiMode2TopLevel(saveState);
                    } else {
                        if (playerState->cameraTickEnabled != 0) {
                            TickActiveCameraState(saveState);
                        }
                        if (zSnd::GetAudioApiOption() == 1) {
                            saveState->UpdateModalLoopSfx(0);
                        }

                        const int altGunFireHeldFlag = playerState->altGunFireHeldFlag;
                        playerState->aiActive = 0;
                        if (altGunFireHeldFlag != 0) {
                            PlayerGunFireController *const activeAltGunController =
                                playerState->activeAltGunController;
                            playerState->altGunFireHeldFlag = 0;
                            OptCatalog::DeactivateTrailRuntimeState(
                                activeAltGunController->trailRuntimeState);
                        }

                        saveState = saveState != 0 ? saveState->next : 0;
                        continue;
                    }
                } else {
                    if (playerState->cameraTickEnabled != 0) {
                        TickActiveCameraState(saveState);
                    }
                    if (zSnd::GetAudioApiOption() == 1) {
                        saveState->UpdateModalLoopSfx(0);
                    }

                    const int altGunFireHeldFlag = playerState->altGunFireHeldFlag;
                    playerState->aiActive = 0;
                    if (altGunFireHeldFlag != 0) {
                        PlayerGunFireController *const activeAltGunController =
                            playerState->activeAltGunController;
                        playerState->altGunFireHeldFlag = 0;
                        OptCatalog::DeactivateTrailRuntimeState(
                            activeAltGunController->trailRuntimeState);
                    }

                    saveState = saveState != 0 ? saveState->next : 0;
                    continue;
                }
            }

            const int postTickLifecycleState = playerState->lifecycleState;
            if (postTickLifecycleState == kPlayerLifecycleLocal ||
                postTickLifecycleState == 0 ||
                VariantTag::TagsOverlap(&playerState->variantTag, &g_VariantTag_Current) != 0) {
                TickMasterTypeAndForceFeedback(saveState);

                if (playerState->masterType != kPlayerMasterTypeAmphib) {
                    if (g_Player_LocalControlEnabled != 0) {
                        UpdateAltGunAimDirection(saveState);
                    }

                    int altGunLatch = 0;
                    if (playerState->altGunDispatchRequested != 0 &&
                        playerState->activeAltGunController->ammoOrCharge > 0.0f) {
                        altGunLatch = 1;
                    }
                    playerState->netInputBit16Latch = altGunLatch;

                    int primaryGunLatch = 0;
                    if (playerState->primaryGunDispatchRequested != 0 &&
                        playerState->activePrimaryGunController->ammoOrCharge > 0.0f) {
                        primaryGunLatch = 1;
                    }
                    playerState->netInputBit17Latch = primaryGunLatch;

                    TickAltGunRuntimeState(saveState);
                }

                ResetDamageVisualsAndTimedStatus(saveState);
            }

            if (playerState->cameraTickEnabled != 0) {
                TickActiveCameraState(saveState);
            }
            if (zSnd::GetAudioApiOption() == 1) {
                saveState->UpdateModalLoopSfx(1);
            }
        }

        saveState = saveState != 0 ? saveState->next : 0;
    }

    if (zSnd::GetAudioApiOption() != 1) {
        zUtil_SaveGameState *const localSaveState =
            (zUtil_SaveGameState *)g_GameStateOrMapTable;
        if (localSaveState->playerState->lifecycleState != kPlayerLifecycleInactive) {
            localSaveState->UpdateModalLoopSfx(1);
        }
    }

    if (zOpt::GetNetworkEnabled() != 0) {
        GameNet::TickLocalPlayerPkt06ReplicationAndHudTimer(
            (zUtil_SaveGameState *)g_GameStateOrMapTable);
    }

    UpdateDebugOverlayHud(g_CurrentPlayerSaveState, activeMode2Count, totalMode2Count);
}

// Reimplements 0x404e90: Player::TickActiveCameraState
// (D:\Proj\GameZRecoil\Player\player_camera.c)
RECOIL_NOINLINE void RECOIL_FASTCALL TickActiveCameraState(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    g_Player_CameraVariantUpdatedThisTick = 0;

    if (g_Player_RebuildCameraDirFlatFromCurrentTarget != 0) {
        zVec3 targetWorldPos = playerState->worldPos;
        zVec3 activeCameraTarget = {};
        zClass_Camera::gwCameraGetTarget(g_MainCamera, &activeCameraTarget.x,
                                         &activeCameraTarget.y, &activeCameraTarget.z);

        playerState->cameraTargetDistance =
            zMath::Vec3DeltaLength(&activeCameraTarget, &targetWorldPos);

        targetWorldPos.y += playerState->cameraYOffset;
        const float dirX = targetWorldPos.x - activeCameraTarget.x;
        const float dirY = targetWorldPos.y - activeCameraTarget.y;
        const float dirZ = targetWorldPos.z - activeCameraTarget.z;
        const float invLength =
            1.0f / static_cast<float>(sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ));
        playerState->cameraDirFlat.z = dirZ * invLength;
        playerState->cameraDirFlat.x = dirX * invLength;
        playerState->cameraDirFlat.y = dirY * invLength;
        g_Player_RebuildCameraDirFlatFromCurrentTarget = 0;
    }

    switch (playerState->cameraState) {
    case 1:
        UpdateChaseCameraFromInput(saveState);
        break;
    case kPlayerCameraStateClearScreen:
        UpdateTopDownCameraState(saveState);
        break;
    case kPlayerCameraStateFirstPerson:
        UpdateFirstPersonCameraFromInput(saveState);
        break;
    case kPlayerCameraStateTargeting:
        UpdateThirdPersonCamera(saveState);
        break;
    case 5:
        zGame::ReturnOnlyStub();
        break;
    case 6:
        UpdateCameraFromStoredTargetTowardPlayer(saveState);
        break;
    case kPlayerCameraStateProjectileAttached:
        RestoreThirdPersonCameraFromObstructionState(saveState);
        break;
    }

    if (g_Player_CameraVariantUpdatedThisTick == 0) {
        UpdateCameraVariantFromCameraPos(saveState, &playerState->cameraTarget);
    }

    UpdateCameraWeatherFxEmitterVisibility();

    if (playerState->cameraState == kPlayerCameraStateClearScreen) {
        playerState->cameraBasisCache = playerState->steerBasisNorm;
    } else {
        playerState->cameraBasisCache = playerState->cameraDirNext;
    }
}

// Reimplements 0x405040: Player::UpdateChaseCameraFromInput
// (D:\Proj\GameZRecoil\Player\player_camera.c)
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateChaseCameraFromInput(
    zUtil_SaveGameState *saveState) {
    const float kVerticalSpeedCameraInputCutoff = 11.0f;
    const float kCameraElevationInputScale = -8.0f;
    const float kCameraVelocitySwingScale = -0.0900000036f;
    const float kCameraElevationBaseClearance = 0.5f;
    const float kCameraHeadingDotEpsilon = 0.0000999999975f;
    const float kCameraDistanceDampingRate = -6.0f;
    const float kTrackYOffsetDampingRate = 5.0f;
    const float kNonTrackYOffsetDampingRate = 3.0f;

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterCommonData *const masterCommonData = playerState->masterCommonData;
    PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;
    const float cameraZone = g_Player_CameraZone;
    const float cameraZoneInvRange = g_Player_CameraZoneInvRange;
    const float maxCamYawRate = g_Player_MaxCamYawRate;

    zInput::MouseStateSnapshot mouseState = {};
    if (zOpt::GetCursorMode() != 0) {
        memcpy(&mouseState, zInput::Mouse_GetStateSnapshotPtr(), sizeof(mouseState));
    }

    float yawDelta = 0.0f;
    if (playerState->joyCameraYawInput != 0.0f) {
        yawDelta = maxCamYawRate * g_FrameDeltaTimeSec * playerState->joyCameraYawInput;
    } else if (zOpt::GetSteeringMode() != 0) {
        if (zOpt::GetCursorMode() != 0) {
            if (playerState->cursorDeltaX == 0.0f && mouseState.deltaX != 0) {
                yawDelta = static_cast<float>(mouseState.deltaX) * g_Player_MousePushX;
            }
        } else if (playerState->cursorNormX > cameraZone) {
            yawDelta = (playerState->cursorNormX - cameraZone) * cameraZoneInvRange *
                       maxCamYawRate * g_FrameDeltaTimeSec;
        } else if (playerState->cursorNormX < -cameraZone) {
            yawDelta = (cameraZone + playerState->cursorNormX) * cameraZoneInvRange *
                       maxCamYawRate * g_FrameDeltaTimeSec;
        }
    }
    playerState->thirdPersonYawOffset += yawDelta;

    const float invertedCameraZoneInvRange = -cameraZoneInvRange;
    if (static_cast<float>(fabs(playerState->localVel.z)) < kVerticalSpeedCameraInputCutoff) {
        float elevationDelta = 0.0f;
        if (zOpt::GetCursorMode() != 0) {
            if (playerState->cursorDeltaY == 0.0f && mouseState.deltaY != 0) {
                elevationDelta = static_cast<float>(mouseState.deltaY) * g_Player_MousePushY;
            }
        } else if (playerState->cursorNormY > cameraZone) {
            elevationDelta = (playerState->cursorNormY - cameraZone) *
                             invertedCameraZoneInvRange * g_FrameDeltaTimeSec *
                             kCameraElevationInputScale;
        } else if (playerState->cursorNormY < -cameraZone) {
            elevationDelta = (cameraZone + playerState->cursorNormY) *
                             invertedCameraZoneInvRange * g_FrameDeltaTimeSec *
                             kCameraElevationInputScale;
        }
        playerState->cameraElevationOffset -= elevationDelta;
    }

    const float thirdPersonSideOffset = playerState->thirdPersonSideOffset;
    const float thirdPersonBaseYOffset = playerState->thirdPersonBaseYOffset;
    const float cameraDistance = playerState->cameraDistance;

    const float horizontalProjectileSpeed = static_cast<float>(
        sqrt(playerState->projectileSpawnVel.x * playerState->projectileSpawnVel.x +
             playerState->projectileSpawnVel.z * playerState->projectileSpawnVel.z));
    const float speedSwingFactor = PlayerFloatFromBits(
        static_cast<int>(horizontalProjectileSpeed * kCameraVelocitySwingScale *
                         12102200.0f) +
        0x3f800000);
    float maxElevationOffset = masterCommonData->cameraUdSwing[0] * speedSwingFactor;
    const float baseElevationLimit = thirdPersonBaseYOffset - kCameraElevationBaseClearance;
    if (baseElevationLimit < maxElevationOffset) {
        maxElevationOffset = baseElevationLimit;
    }

    if (playerState->cameraElevationOffset > maxElevationOffset) {
        playerState->cameraElevationOffset = maxElevationOffset;
    } else if (playerState->cameraElevationOffset < -maxElevationOffset) {
        playerState->cameraElevationOffset = -maxElevationOffset;
    }

    const float headingLerpBase = playerState->slipSfxActive != 0
                                      ? g_Player_CameraHeadingLerpBaseWhenFlagSet
                                      : g_Player_CameraHeadingLerpBaseWhenFlagClear;
    const float headingBlend = PlayerFloatFromBits(
        static_cast<int>(-(headingLerpBase +
                           1.0f / (g_Player_CameraHeadingDotAbs +
                                   kCameraHeadingDotEpsilon)) *
                         g_FrameDeltaTimeSec * 12102200.0f) +
        0x3f800000);
    zVec3 flatSteerBasis = playerState->steerBasisNorm;
    zMath::Vec3LerpNormalize(&playerState->cameraDirFlat, &flatSteerBasis, headingBlend);
    g_Player_CameraHeadingDotAbs = static_cast<float>(
        fabs(playerState->steerBasisNorm.x * playerState->cameraDirFlat.x +
             playerState->steerBasisNorm.z * playerState->cameraDirFlat.z));

    float cameraDirX = playerState->cameraDirFlat.x;
    float cameraDirZ = playerState->cameraDirFlat.z;
    if (playerState->thirdPersonYawOffset != 0.0f) {
        const float yawSin = static_cast<float>(sin(playerState->thirdPersonYawOffset));
        const float yawCos = static_cast<float>(cos(playerState->thirdPersonYawOffset));
        cameraDirX =
            yawCos * playerState->cameraDirFlat.x - yawSin * playerState->cameraDirFlat.z;
        cameraDirZ =
            yawCos * playerState->cameraDirFlat.z + yawSin * playerState->cameraDirFlat.x;
    }

    float targetDistance = playerState->cameraTargetDistance;
    if (playerState->slipSfxActive == 0) {
        targetDistance = cameraDistance - g_Player_CameraElastic * playerState->localVel.z;
    }

    const float distanceBlend = PlayerFloatFromBits(
        static_cast<int>(g_FrameDeltaTimeSec * kCameraDistanceDampingRate * 12102200.0f) +
        0x3f800000);
    playerState->cameraTargetDistance =
        (1.0f - distanceBlend) * targetDistance +
        distanceBlend * playerState->cameraTargetDistance;

    zVec3 cameraOffset = {};
    cameraOffset.x = -cameraDirZ * thirdPersonSideOffset -
                     cameraDirX * playerState->cameraTargetDistance;
    cameraOffset.z = cameraDirX * thirdPersonSideOffset -
                     cameraDirZ * playerState->cameraTargetDistance;

    const float yOffsetRate = masterModalData->masterType == kPlayerMasterTypeTrack
                                  ? kTrackYOffsetDampingRate
                                  : kNonTrackYOffsetDampingRate;
    const float yOffsetBlend = PlayerFloatFromBits(
        static_cast<int>(-yOffsetRate * g_FrameDeltaTimeSec * 12102200.0f) +
        0x3f800000);
    const float yOffsetInvBlend = 1.0f - yOffsetBlend;
    float targetYOffset =
        (cameraOffset.x * playerState->steerBasisNorm.x +
         playerState->steerBasisNorm.z * cameraOffset.z) *
        playerState->steerBasisRaw.y;
    if (masterModalData->masterType == kPlayerMasterTypeTrack && targetYOffset <= 0.0f) {
        targetYOffset = 0.0f;
    }
    playerState->thirdPersonPositionYOffset =
        yOffsetBlend * playerState->thirdPersonPositionYOffset +
        targetYOffset * yOffsetInvBlend;

    zVec3 cameraPos = {};
    cameraPos.x = playerState->worldPos.x + cameraOffset.x;
    cameraPos.y = playerState->worldPos.y + thirdPersonBaseYOffset +
                  playerState->thirdPersonPositionYOffset -
                  playerState->cameraElevationOffset;
    cameraPos.z = playerState->worldPos.z + cameraOffset.z;

    zVec3 focusPos = playerState->worldPos;
    focusPos.y += playerState->cameraYOffset;
    if (masterModalData->masterType == kPlayerMasterTypeSub) {
        AdjustSubCameraFocusForObstruction(saveState, &focusPos);
    }

    const float dirX = focusPos.x - cameraPos.x;
    const float dirY = focusPos.y - cameraPos.y;
    const float dirZ = focusPos.z - cameraPos.z;
    const float invDirLength =
        1.0f / static_cast<float>(sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ));
    playerState->cameraDirNext.z = dirZ * invDirLength;
    playerState->cameraDirNext.x = dirX * invDirLength;
    playerState->cameraDirNext.y = dirY * invDirLength;

    AdjustThirdPersonCameraBySideProbes(saveState, &cameraPos, &focusPos,
                                        &playerState->cameraDirNext);

    zClass_Camera::gwCameraSetTarget(g_MainCamera, cameraPos.x, cameraPos.y, cameraPos.z);
    zVec3 cameraAngles = {};
    zVec3 *const cameraAnglesPtr =
        zMath::Vec3DirectionAnglesBetweenPoints(&cameraPos, &focusPos, &cameraAngles);
    zClass_Camera::gwCameraSetPosition(g_MainCamera, cameraAnglesPtr->x, cameraAnglesPtr->y,
                                       cameraAnglesPtr->z);

    playerState->cameraTarget = cameraPos;
    playerState->cameraDir = playerState->cameraDirNext;
}

// Reimplements 0x4057d0: Player::UpdateTopDownCameraState
// (D:\Proj\GameZRecoil\Player\player_camera.c)
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateTopDownCameraState(
    zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->cameraTarget.x =
        playerState->worldPos.x + playerState->cameraState2TargetOffset.x;
    playerState->cameraTarget.y =
        playerState->worldPos.y + playerState->cameraState2TargetOffset.y;
    playerState->cameraTarget.z =
        playerState->worldPos.z + playerState->cameraState2TargetOffset.z;

    zClass_Camera::gwCameraSetTarget(g_MainCamera, playerState->cameraTarget.x,
                                     playerState->cameraTarget.y,
                                     playerState->cameraTarget.z);
    zClass_Camera::gwCameraSetPosition(g_MainCamera, -1.54999995f, 0.0f, 0.0f);
    playerState->cameraDir.x = 0.0f;
    playerState->cameraDir.y = -1.0f;
    playerState->cameraDir.z = 0.0f;
}

// Reimplements 0x4059a0: Player::UpdateFirstPersonCameraFromInput
// (D:\Proj\GameZRecoil\Player\player_camera.c)
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateFirstPersonCameraFromInput(
    zUtil_SaveGameState *saveState) {
    const float kForwardSpeedClampThreshold = 10.0f;
    const float kForwardSpeedClampScale = -0.0153999999f;
    const float kElevationCameraPosScale = -0.349999994f;

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    const float cameraZone = g_Player_CameraZone;
    const float invertedCameraZoneInvRange = -g_Player_CameraZoneInvRange;

    if (zOpt::GetCursorMode() != 0) {
        zInput::MouseStateSnapshot mouseState = *zInput::Mouse_GetStateSnapshotPtr();
        if (playerState->cursorDeltaY == 0.0f && mouseState.deltaY != 0) {
            playerState->cameraElevationOffset -=
                static_cast<float>(mouseState.deltaY) * g_Player_MousePushY;
        }
    } else if (playerState->cursorNormY > cameraZone) {
        playerState->cameraElevationOffset +=
            (playerState->cursorNormY - cameraZone) * invertedCameraZoneInvRange *
            g_Player_FpCamElevationRate * g_FrameDeltaTimeSec;
    } else if (playerState->cursorNormY < -cameraZone) {
        playerState->cameraElevationOffset +=
            (cameraZone + playerState->cursorNormY) * invertedCameraZoneInvRange *
            g_Player_FpCamElevationRate * g_FrameDeltaTimeSec;
    }

    float elevationMin = g_Player_FpCamElevationMin;
    float elevationMax = g_Player_FpCamElevationMax;
    const float forwardSpeed =
        static_cast<float>(sqrt(playerState->projectileSpawnVel.x *
                                    playerState->projectileSpawnVel.x +
                                playerState->projectileSpawnVel.z *
                                    playerState->projectileSpawnVel.z));
    const float speedOverThreshold = forwardSpeed - kForwardSpeedClampThreshold;
    if (speedOverThreshold > 0.0f) {
        const float elevationScale = PlayerFloatFromBits(
            static_cast<int>(speedOverThreshold * kForwardSpeedClampScale * 12102200.0f) +
            0x3f800000);
        elevationMin *= elevationScale;
        elevationMax *= elevationScale;
    }

    if (playerState->cameraElevationOffset > elevationMax) {
        playerState->cameraElevationOffset = elevationMax;
    } else if (playerState->cameraElevationOffset < elevationMin) {
        playerState->cameraElevationOffset = elevationMin;
    }

    const zMat4x3 &motionBasis = playerState->motionBasis;
    const zVec3 &localOffset = playerState->cameraState6LocalOffset;
    zVec3 cameraPoint = playerState->worldPos;
    zVec3 cameraLocalOffsetWorld = {};
    cameraLocalOffsetWorld.x = localOffset.x * motionBasis.xx +
                               localOffset.y * motionBasis.yx +
                               localOffset.z * motionBasis.zx;
    cameraLocalOffsetWorld.y = localOffset.x * motionBasis.xy +
                               localOffset.y * motionBasis.yy +
                               localOffset.z * motionBasis.zy;
    cameraLocalOffsetWorld.z = localOffset.x * motionBasis.xz +
                               localOffset.y * motionBasis.yz +
                               localOffset.z * motionBasis.zz;
    cameraPoint.x += cameraLocalOffsetWorld.x;
    cameraPoint.y += cameraLocalOffsetWorld.y;
    cameraPoint.z += cameraLocalOffsetWorld.z;

    zClass_Camera::gwCameraSetTarget(g_MainCamera, cameraPoint.x, cameraPoint.y,
                                     cameraPoint.z);
    playerState->cameraTarget = cameraPoint;

    zVec3 cameraPosition = playerState->cameraState6BasePos;
    cameraPosition.x -= playerState->cameraElevationOffset * kElevationCameraPosScale;
    zClass_Camera::gwCameraSetPosition(g_MainCamera, cameraPosition.x, cameraPosition.y,
                                       cameraPosition.z);

    playerState->cameraDirNext = playerState->steerBasisRaw;
    playerState->cameraDirFlat = playerState->steerBasisRaw;
    playerState->cameraDir = playerState->steerBasisRaw;
}

// Reimplements 0x405870: Player::UpdateCameraFromStoredTargetTowardPlayer
// (D:\Proj\GameZRecoil\Player\player_camera.c)
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateCameraFromStoredTargetTowardPlayer(
    zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zVec3 lookAt = playerState->worldPos;
    lookAt.y += playerState->cameraState == kPlayerCameraStateThirdPerson
                    ? playerState->cameraYOffset
                    : playerState->cameraState6YOffset;

    zVec3 cameraTarget = playerState->cameraTarget;
    const float dirX = lookAt.x - cameraTarget.x;
    const float dirY = lookAt.y - cameraTarget.y;
    const float dirZ = lookAt.z - cameraTarget.z;
    const float invDirLength =
        1.0f / static_cast<float>(sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ));
    playerState->cameraDirNext.z = dirZ * invDirLength;
    playerState->cameraDirNext.x = dirX * invDirLength;
    playerState->cameraDirNext.y = dirY * invDirLength;

    zVec3 cameraAngles = {};
    zVec3 *const cameraAnglesPtr =
        zMath::Vec3DirectionAnglesBetweenPoints(&cameraTarget, &lookAt, &cameraAngles);
    zClass_Camera::gwCameraSetPosition(g_MainCamera, cameraAnglesPtr->x,
                                       cameraAnglesPtr->y, cameraAnglesPtr->z);

    playerState->cameraDir = playerState->cameraDirNext;
    playerState->cameraDirFlat = playerState->cameraDirNext;
    playerState->cameraDirFlat.y = 0.0f;
    zMath::Vec3NormalizeXZ(&playerState->cameraDirFlat, &playerState->cameraDirFlat);
}

// Reimplements 0x4063f0: Player::RestoreThirdPersonCameraFromObstructionState
// (D:\Proj\GameZRecoil\Player\player_camera.c)
RECOIL_NOINLINE void RECOIL_FASTCALL RestoreThirdPersonCameraFromObstructionState(
    zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    gwNode::GetWorldPosition(g_MainCamera, &playerState->cameraTarget);
    playerState->cameraDir = playerState->cameraObstructionDir;
}

// Reimplements 0x406610: Player::UpdateCameraWeatherFxEmitterVisibility
// (D:\Proj\GameZRecoil\Player\player_camera.c)
RECOIL_NOINLINE void RECOIL_CDECL UpdateCameraWeatherFxEmitterVisibility() {
    HudUiElement *const fxElement = g_HudSensorTracker.fxPass3Obj;
    if (fxElement == 0) {
        return;
    }

    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(g_GameStateOrMapTable);
    const int isSubMode =
        saveState->primaryModalState->masterModalData->masterType == kPlayerMasterTypeSub;
    if (isSubMode != 0) {
        if ((fxElement->flags & 0x10) == 0) {
            SetHudUiElementVisible(fxElement, 0);
        }
    } else {
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        zVec3 cameraTarget = {};
        zClass_Camera::gwCameraGetTarget(g_MainCamera, &cameraTarget.x, &cameraTarget.y,
                                         &cameraTarget.z);
        zClass_Class::gwNodeSetRaycastable(playerState->rootNode, 0);
        zClass_cls_di::SetStopAfterFirstHit(0x40000);
        zClass_cls_di::SetBreakOnFirstCandidate(1);

        PlayerProbeSampleCandidateBuffer raycastCandidates = {};
        const int raycastResult = zClass_cls_di::RaycastFindClosest(
            g_Player_RuntimeDiScene, &raycastCandidates, cameraTarget.x, cameraTarget.y,
            cameraTarget.z, cameraTarget.x, cameraTarget.y + 50.0f, cameraTarget.z);

        zClass_cls_di::SetBreakOnFirstCandidate(0);
        zClass_Class::gwNodeSetRaycastable(playerState->rootNode, 1);

        const int shouldHide =
            raycastResult == 0 && raycastCandidates.candidateCount > 0 ? 1 : 0;
        if (shouldHide != 0) {
            if ((fxElement->flags & 0x10) == 0) {
                SetHudUiElementVisible(fxElement, 0);
            }
        } else if ((fxElement->flags & 0x10) != 0) {
            SetHudUiElementVisible(fxElement, 1);
        }
    }

    if ((fxElement->flags & 0x10) != 0) {
        return;
    }

    PlayerWeatherFxEmitterOverlay *const fxEmitter =
        (PlayerWeatherFxEmitterOverlay *)(fxElement);
    fxEmitter->cameraNode = g_MainCamera;
    fxEmitter->particleAgeTick = zOpt::GetReplicateMode() == 0 ? 1 : 0;
}

// Reimplements 0x425a20: Player::TickLocalPlayerControls
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL TickLocalPlayerControls(zUtil_SaveGameState *saveState) {
    if (g_Player_LocalControlEnabled == 0) {
        return;
    }

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;

    zInput::MouseStateSnapshot mouseState = {};
    if (zInp::GetJoystickOption() != 0) {
        zInput::JoystickStatePartial *const joyState = zInput::DI_GetCurrentState();
        if (playerState->cameraState == kPlayerCameraStateProjectileAttached) {
            playerState->cursorDeltaX = 0.0f;
            playerState->cursorDeltaY = 0.0f;
            playerState->cursorNormX =
                static_cast<float>(joyState->lX) * g_zInput_JoystickAxisConfig_Gameplay.axes[0].normScale;
            playerState->cursorNormY =
                static_cast<float>(-joyState->lY) * g_zInput_JoystickAxisConfig_Gameplay.axes[1].normScale;
        } else {
            playerState->cursorNormX = 0.0f;
            const float joyCursorY =
                static_cast<float>(-joyState->lY) * g_zInput_JoystickAxisConfig_Gameplay.axes[1].normScale;
            const float cursorBlend =
                PlayerFloatFromBits(static_cast<int>(g_Player_DeltaTime * -3.2f * 12102200.0f) +
                                    0x3f800000);
            playerState->cursorNormY =
                cursorBlend * playerState->cursorNormY + (1.0f - cursorBlend) * joyCursorY;
            playerState->steeringInput =
                static_cast<float>(-joyState->lX) * g_zInput_JoystickAxisConfig_Gameplay.axes[0].normScale;
            playerState->throttleInput =
                static_cast<float>(-joyState->lZ) * g_zInput_JoystickAxisConfig_Gameplay.axes[2].normScale;
            playerState->joyCameraYawInput =
                static_cast<float>(joyState->lRz) * g_zInput_JoystickAxisConfig_Gameplay.axes[3].normScale;
        }
    } else if ((g_Player_RuntimeInputFlags & 2) != 0) {
        zInput::Mouse_GetStateSnapshot(&mouseState);
        playerState->cursorDeltaX = mouseState.cursorNormX - playerState->cursorNormX;
        playerState->cursorDeltaY = mouseState.cursorNormY - playerState->cursorNormY;
        playerState->cursorNormX = mouseState.cursorNormX;
        playerState->cursorNormY = mouseState.cursorNormY;
    }

    if ((zInput::BindMap_Current_ReadCommandInputState(4) & 3) != 0) {
        if (zOpt::GetThrottleMode() != 0) {
            playerState->throttleInput += g_FrameDeltaTimeSec;
        } else {
            playerState->throttleInput = 1.0f;
        }
    } else if ((zInput::BindMap_Current_ReadCommandInputState(1) & 3) != 0) {
        if (zOpt::GetThrottleMode() != 0) {
            playerState->throttleInput -= g_FrameDeltaTimeSec;
        } else {
            playerState->throttleInput = -1.0f;
        }
    } else if (zOpt::GetThrottleMode() == 0 && zInp::GetJoystickOption() == 0) {
        playerState->throttleInput = 0.0f;
    }

    if ((zInput::BindMap_Current_ReadCommandInputState(2) & 3) != 0) {
        playerState->steeringInput = 1.0f;
    } else if ((zInput::BindMap_Current_ReadCommandInputState(3) & 3) != 0) {
        playerState->steeringInput = -1.0f;
    } else if (zInp::GetJoystickOption() == 0) {
        playerState->steeringInput = 0.0f;
    }

    if (zOpt::GetSteeringMode() == 0 && playerState->steeringInput == 0.0f &&
        zInp::GetJoystickOption() == 0) {
        if (zOpt::GetCursorMode() == 0) {
            if (playerState->cursorNormX > g_Player_CameraZone) {
                playerState->steeringInput =
                    (playerState->cursorNormX - g_Player_CameraZone) *
                    -g_Player_CameraZoneInvRange;
            } else if (playerState->cursorNormX < -g_Player_CameraZone) {
                playerState->steeringInput =
                    (g_Player_CameraZone + playerState->cursorNormX) *
                    -g_Player_CameraZoneInvRange;
            }
        } else if (playerState->cursorDeltaX == 0.0f && mouseState.deltaX != 0) {
            playerState->steeringInput =
                static_cast<float>(-mouseState.deltaX) * g_Player_GameplayInputStepScale;
        }
    }

    if ((zInput::BindMap_Current_ReadCommandInputState(5) & 3) != 0) {
        playerState->subVerticalInput = 1.0f;
    } else if ((zInput::BindMap_Current_ReadCommandInputState(6) & 3) != 0) {
        playerState->subVerticalInput = -1.0f;
    } else {
        playerState->subVerticalInput = 0.0f;
    }

    playerState->subPitchInput = 0.0f;
    if (masterModalData->masterType == kPlayerMasterTypeSub &&
        static_cast<float>(fabs(playerState->localVel.z)) >= 10.0f) {
        if (playerState->cursorNormY > g_Player_CameraZone) {
            playerState->subPitchInput =
                (playerState->cursorNormY - g_Player_CameraZone) *
                -g_Player_CameraZoneInvRange;
        } else if (playerState->cursorNormY < -g_Player_CameraZone) {
            playerState->subPitchInput =
                (g_Player_CameraZone + playerState->cursorNormY) *
                -g_Player_CameraZoneInvRange;
        }
    }

    playerState->subVerticalInput = PlayerClampSigned(playerState->subVerticalInput, 1.0f);
    playerState->throttleInput = PlayerClampSigned(playerState->throttleInput, 1.0f);
    playerState->steeringInput = PlayerClampSigned(playerState->steeringInput, 1.0f);
    playerState->subPitchInput = PlayerClampSigned(playerState->subPitchInput, 1.0f);

    playerState->throttleInputCopy = playerState->throttleInput;
    playerState->subVerticalInputCopy = playerState->subVerticalInput;
    playerState->subPitchInputCopy = playerState->subPitchInput;
    playerState->steeringInputCopy = playerState->steeringInput;
    HudUiMgr::UpdateTargetReticleFromCursor(2, &playerState->storedTargetPos,
                                            playerState->cursorNormX,
                                            playerState->cursorNormY);

    const int altFireState = zInput::BindMap_Current_ReadCommandInputState(12);
    if ((altFireState & 3) != 0) {
        PlayerGunFireController *const activeAltGun = playerState->activeAltGunController;
        if ((activeAltGun->optCatalogEntry->flags & 2u) != 0) {
            if (activeAltGun->ammoOrCharge > 0.0f) {
                playerState->altGunDispatchRequested = 1;
            } else if (altFireState == 1) {
                playerState->altGunDispatchRequested = altFireState;
            }
        } else if ((playerState->altGunTransitionState & 0x180) == 0) {
            if (g_Player_TotalTimeSecScaled >= activeAltGun->nextDispatchTime &&
                playerState->playerOrdinal != 0 &&
                activeAltGun != &playerState->altWeaponBanks[1].controllerA) {
                playerState->altGunDispatchRequested = 1;
                activeAltGun->nextDispatchTime =
                    activeAltGun->dispatchRepeatDelay + g_Player_TotalTimeSecScaled;
            }
        } else if ((altFireState & 1) != 0) {
            playerState->pendingAltCameraToggle = 1;
        }
    } else {
        playerState->altGunDispatchRequested = 0;
    }

    playerState->usePresetGunFireDir = 0;
    if ((zInput::BindMap_Current_ReadCommandInputState(11) & 3) != 0) {
        PlayerGunFireController *const activePrimaryGun = playerState->activePrimaryGunController;
        if ((playerState->altGunTransitionState & 0x180) != 0) {
            playerState->usePresetGunFireDir = 1;
        } else if (g_Player_TotalTimeSecScaled >= activePrimaryGun->nextDispatchTime &&
                   playerState->playerOrdinal != 0 &&
                   g_Time_AccumulatedTimeSec >= playerState->primaryGunGateUntilTime) {
            playerState->primaryGunDispatchRequested = 1;
            activePrimaryGun->nextDispatchTime =
                activePrimaryGun->dispatchRepeatDelay + g_Player_TotalTimeSecScaled;
        }
    } else {
        playerState->primaryGunDispatchRequested = 0;
    }

    if (zInput::BindMap_Current_ReadCommandInputState(13) == 1) {
        playerState->altGunTriggerProcessFlag = 1;
        if (zOpt::GetNetworkEnabled() != 0 && g_HudSensorTracker.raceCheckpointMode != 0) {
            g_HudTimerPanelNetState.tenSecondWarningsEnabled = 1;
        }
    } else {
        playerState->altGunTriggerProcessFlag = 0;
    }

    if (zInput::BindMap_Current_ReadCommandInputState(7) == 1) {
        ResetMouseControlStateAndRecenterCursor(saveState);
    }

    if ((zInput::BindMap_Current_ReadCommandInputState(37) & 3) != 0 &&
        masterModalData->masterType == kPlayerMasterTypeHover && playerState->autoTurnSign == 0) {
        TransitionToMasterTypeTrack(g_LocalPlayerSaveState, 0);
    }

    if ((zInput::BindMap_Current_ReadCommandInputState(38) & 3) != 0 &&
        masterModalData->masterType == kPlayerMasterTypeHover &&
        playerState->nextModeSwitchAllowedTime != 0.0f) {
        TransitionToMasterTypeAmphib(g_LocalPlayerSaveState, 1, 0);
    }

    if ((zInput::BindMap_Current_ReadCommandInputState(39) & 3) != 0 &&
        (masterModalData->masterType == kPlayerMasterTypeTrack ||
         masterModalData->masterType == kPlayerMasterTypeAmphib)) {
        TransitionToMasterTypeHover(g_LocalPlayerSaveState, 0);
    }

    if ((zInput::BindMap_Current_ReadCommandInputState(40) & 3) != 0 &&
        masterModalData->masterType == kPlayerMasterTypeAmphib) {
        TransitionToMasterTypeSub(g_LocalPlayerSaveState, 0);
    }

    if (zInput::BindMap_Current_ReadCommandInputState(8) != 1) {
        return;
    }

    playerState->autoTurnTargetWorldPos = playerState->storedTargetPos;
    SetAutoTurnTargetDirFromWorldPoint(saveState, &playerState->autoTurnTargetWorldPos);

    zVec3 cameraTarget = {};
    zClass_Camera::gwCameraGetTarget(g_MainCamera, &cameraTarget.x, &cameraTarget.y,
                                     &cameraTarget.z);
    playerState->cameraLerpStart.x = cameraTarget.x - playerState->worldPos.x;
    playerState->cameraLerpStart.y = cameraTarget.y - playerState->worldPos.y;
    playerState->cameraLerpStart.z = cameraTarget.z - playerState->worldPos.z;

    const float cameraDistance = -playerState->cameraDistance;
    playerState->cameraLerpEnd.x = cameraDistance * playerState->autoTurnTargetDir.x;
    playerState->cameraLerpEnd.y = cameraDistance * playerState->autoTurnTargetDir.y;
    playerState->cameraLerpEnd.z = cameraDistance * playerState->autoTurnTargetDir.z;
    playerState->cameraLerpEnd.y = playerState->cameraLerpStart.y;
    ApplyCameraState(kPlayerCameraStateTargeting);
}

// Reimplements 0x405ec0: Player::ToggleSteeringModeAndResetMouseLook
// (D:\Proj\GameZRecoil\Player\player_camera.c)
RECOIL_NOINLINE void RECOIL_CDECL ToggleSteeringModeAndResetMouseLook() {
    ResetMouseControlStateAndRecenterCursor((zUtil_SaveGameState *)g_GameStateOrMapTable);
    zOpt::SetSteeringMode(zOpt::GetSteeringMode() == 0 ? 1 : 0);
}

// Reimplements 0x42bed0: Player::ResetMotionTransientState
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ResetMotionTransientState(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->localVel.x = 0.0f;
    playerState->localVel.y = 0.0f;
    playerState->localVel.z = 0.0f;
    playerState->projectileSpawnVel.x = 0.0f;
    playerState->projectileSpawnVel.y = 0.0f;
    playerState->projectileSpawnVel.z = 0.0f;
    playerState->yawRotatedLocalVel.x = 0.0f;
    playerState->yawRotatedLocalVel.y = 0.0f;
    playerState->yawRotatedLocalVel.z = 0.0f;
    playerState->angVelPitch = 0.0f;
    playerState->angVelYaw = 0.0f;
    playerState->angVelRoll = 0.0f;
    playerState->steeringInput = 0.0f;
    playerState->throttleInput = 0.0f;
    playerState->subVerticalInput = 0.0f;
    playerState->subVerticalInputCopy = 0.0f;
    playerState->steeringInputCopy = 0.0f;
    playerState->throttleInputCopy = 0.0f;
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

// Reimplements 0x42b8c0: Player::RebuildSteerBasisRawFromRef
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
RebuildSteerBasisRawFromRef(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    if (playerState->steerBasisRef.y == 0.0f) {
        return;
    }

    zVec3 rawBasis = playerState->steerBasisNorm;
    rawBasis.y = -((playerState->steerBasisRef.x * playerState->steerBasisNorm.x +
                    playerState->steerBasisRef.z * playerState->steerBasisNorm.z) /
                   playerState->steerBasisRef.y);
    zMath::Vec3Normalize(&rawBasis);
    playerState->steerBasisRaw = rawBasis;
}

// Reimplements 0x429240: Player::ApplyAmphibSpeedOscillation
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
ApplyAmphibSpeedOscillation(zUtil_SaveGameState *saveState, zVec3 *inOutUpVector,
                            int includeYawCoupling) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;

    const float speedAbs = static_cast<float>(fabs(playerState->localVel.z));
    const float pitchArg =
        (masterModalData->hoverPitchWaveSpeedRate * speedAbs +
         masterModalData->hoverPitchWaveBaseRate) *
        g_Time_AccumulatedTimeSec;
    const float rollArg =
        (masterModalData->hoverRollWaveSpeedRate * speedAbs +
         masterModalData->hoverRollWaveBaseRate) *
        g_Time_AccumulatedTimeSec;

    const float pitchAngle =
        static_cast<float>(sin(pitchArg)) * masterModalData->hoverPitchWaveAmplitude;
    float rollAngle =
        static_cast<float>(sin(rollArg)) * masterModalData->hoverRollWaveAmplitude;
    if (includeYawCoupling != 0) {
        rollAngle += playerState->angVelYaw * masterModalData->hoverRollYawCoupleScale *
                     playerState->localVel.z;
    }

    const float yawSin = -playerState->steerBasisNorm.x;
    const float yawCos = -playerState->steerBasisNorm.z;
    const float pitchSin = static_cast<float>(sin(pitchAngle));
    const float pitchCos = static_cast<float>(cos(pitchAngle));
    const float rollSin = static_cast<float>(sin(rollAngle));
    const float rollCos = static_cast<float>(cos(rollAngle));

    zMat4x3 oscillationBasis = {};
    oscillationBasis.xx = yawSin * pitchSin * rollSin + rollCos * yawCos;
    oscillationBasis.xy = rollSin * pitchCos;
    oscillationBasis.xz = rollSin * yawCos * pitchSin - rollCos * yawSin;
    oscillationBasis.yx = yawSin * pitchSin * rollCos - rollSin * yawCos;
    oscillationBasis.yy = rollCos * pitchCos;
    oscillationBasis.yz = rollCos * yawCos * pitchSin + rollSin * yawSin;
    oscillationBasis.zx = yawSin * pitchCos;
    oscillationBasis.zy = -pitchSin;
    oscillationBasis.zz = yawCos * pitchCos;

    const zVec3 original = *inOutUpVector;
    inOutUpVector->x = original.x * oscillationBasis.xx + original.y * oscillationBasis.yx +
                       original.z * oscillationBasis.zx;
    inOutUpVector->y = original.x * oscillationBasis.xy + original.y * oscillationBasis.yy +
                       original.z * oscillationBasis.zy;
    inOutUpVector->z = original.x * oscillationBasis.xz + original.y * oscillationBasis.yz +
                       original.z * oscillationBasis.zz;
}

// Reimplements 0x42b970: Player::RebuildMotionBasisFromSteerBasis
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
RebuildMotionBasisFromSteerBasis(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    zVec3 basisSide = {};
    basisSide.x = playerState->steerBasisRaw.y * playerState->steerBasisRef.z -
                  playerState->steerBasisRaw.z * playerState->steerBasisRef.y;
    basisSide.y = playerState->steerBasisRaw.z * playerState->steerBasisRef.x -
                  playerState->steerBasisRaw.x * playerState->steerBasisRef.z;
    basisSide.z = playerState->steerBasisRaw.x * playerState->steerBasisRef.y -
                  playerState->steerBasisRaw.y * playerState->steerBasisRef.x;

    zMat4x3 motionBasis = {};
    motionBasis.xx = basisSide.x;
    motionBasis.xy = basisSide.y;
    motionBasis.xz = basisSide.z;
    motionBasis.yx = playerState->steerBasisRef.x;
    motionBasis.yy = playerState->steerBasisRef.y;
    motionBasis.yz = playerState->steerBasisRef.z;
    motionBasis.zx = -playerState->steerBasisRaw.x;
    motionBasis.zy = -playerState->steerBasisRaw.y;
    motionBasis.zz = -playerState->steerBasisRaw.z;
    motionBasis.posX = playerState->worldPos.x;
    motionBasis.posY = playerState->worldPos.y;
    motionBasis.posZ = playerState->worldPos.z;

    playerState->motionBasis = motionBasis;
}

// Reimplements 0x429560: Player::RebuildSteerBasisFromMotionAxes
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
RebuildSteerBasisFromMotionAxes(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;

    if (playerState->autoTurnActive == 0) {
        return;
    }

    if (playerState->steeringInput != 0.0f) {
        playerState->autoTurnActive = 0;
        if (saveState == g_LocalPlayerSaveState) {
            ApplyCameraState(playerState->previousCameraState);
        }
    }

    if (playerState->autoTurnActive == 0) {
        return;
    }

    const float cross = playerState->steerBasisNorm.z * playerState->autoTurnTargetDir.x -
                        playerState->autoTurnTargetDir.z * playerState->steerBasisNorm.x;
    const float dot = playerState->steerBasisNorm.z * playerState->autoTurnTargetDir.z +
                      playerState->autoTurnTargetDir.x * playerState->steerBasisNorm.x;
    if (dot < static_cast<float>(cos(g_Player_DeltaTime * masterModalData->yawRateMax))) {
        const int turnSign = cross < 0.0f ? -1 : 1;
        const float turnSignFloat = static_cast<float>(turnSign);
        playerState->steeringInput = turnSignFloat;
        playerState->steeringInputCopy = turnSignFloat;
        playerState->angVelYaw = turnSignFloat * masterModalData->yawRateMax;

        if (saveState == g_LocalPlayerSaveState && playerState->lifecycleState != 2) {
            zVec3 normalizedCursor = {0};
            HudUiMgr::ProjectPointToNormalizedClamped(&playerState->autoTurnTargetWorldPos,
                                                      &normalizedCursor);
            playerState->autoTurnCursorNormX = normalizedCursor.x;
            playerState->autoTurnCursorNormY = normalizedCursor.y;
            zInput::Mouse_SetNormalizedCursorPos(normalizedCursor.x, normalizedCursor.y);

            const int lerpBits =
                static_cast<int>(g_FrameDeltaTimeSec * -2.0f * 12102200.0f) + 0x3f800000;
            float lerpFactor = 0.0f;
            memcpy(&lerpFactor, &lerpBits, sizeof(lerpFactor));
            zMath::Vec3Lerp(&playerState->cameraLerpStart, &playerState->cameraLerpEnd,
                            lerpFactor);
        }
        return;
    }

    playerState->thirdPersonYawOffset = 0.0f;
    playerState->cameraDirFlat = playerState->cameraDir;
    playerState->cameraDirFlat.y = 0.0f;
    zMath::Vec3NormalizeXZ(&playerState->cameraDirFlat, &playerState->cameraDirFlat);

    if (saveState == (zUtil_SaveGameState *)(g_GameStateOrMapTable) &&
        saveState == g_LocalPlayerSaveState) {
        ApplyCameraState(playerState->previousCameraState);
        zInput::Mouse_RecenterCursorX();
    }

    playerState->restartYawRad = static_cast<float>(
        atan2(-playerState->autoTurnTargetDir.z, -playerState->autoTurnTargetDir.x));
    playerState->autoTurnActive = 0;
    playerState->steeringInputCopy = 0.0f;
    playerState->angVelYaw = 0.0f;
}

// Reimplements 0x42bab0: Player::SetAutoTurnTargetDirFromWorldPoint
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
SetAutoTurnTargetDirFromWorldPoint(zUtil_SaveGameState *saveState, const zVec3 *worldPoint) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    zVec3 targetDir = {};
    targetDir.x = worldPoint->x - playerState->worldPos.x;
    targetDir.y = worldPoint->y - playerState->worldPos.y;
    targetDir.z = worldPoint->z - playerState->worldPos.z;
    targetDir.y = 0.0f;

    zVec3 normalizedTargetDir = {};
    zMath::Vec3NormalizeXZ(&targetDir, &normalizedTargetDir);
    playerState->autoTurnTargetDir = normalizedTargetDir;
    playerState->autoTurnActive = 1;
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

// Reimplements 0x41f1d0: Player::ApplyMissionSaveData
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ApplyMissionSaveData(PlayerMissionSaveData *saveData) {
    if (saveData->size != sizeof(PlayerMissionSaveData) &&
        saveData->size != kPlayerMissionSaveLegacySize) {
        zError::ReportOld(0x200, "D:\\Proj\\Battlesport\\player.cpp", 0xd1,
                          "Player save data structure has been modified. Cannot use this save set.");
        return;
    }

    zUtil_SaveGameState *const saveState = g_LocalPlayerSaveState;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    const int hasTimedHitStatus = saveData->size == sizeof(PlayerMissionSaveData);

    PlayerGunFireController *const oldAltController = playerState->activeAltGunController;
    PlayerGunFireController *const oldPrimaryController =
        playerState->activePrimaryGunController;

    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        const PlayerMissionSaveWeaponBank *const savedBank = &saveData->weaponBank[bankIndex];
        PlayerAltWeaponBank *const bank = &playerState->altWeaponBanks[bankIndex];

        bank->selectedSide = savedBank->selectedSide;
        PlayerRestoreSavedWeaponSide(&bank->controllerA, &savedBank->sides[0]);
        PlayerRestoreSavedWeaponSide(&bank->controllerB, &savedBank->sides[1]);
        PlayerRefreshSavedWeaponBankHud(bankIndex, bank);
    }

    PlayerAltWeaponBank *const altBank =
        &playerState->altWeaponBanks[saveData->altWeaponBankIndex];
    PlayerGunFireController *const newAltController =
        PlayerSavedWeaponController(altBank, saveData->altWeaponSideIndex);
    playerState->activeAltGunController = newAltController;
    if (oldAltController != newAltController) {
        ApplyAltWeaponSwitch(saveState, oldAltController, newAltController);
        PlayerRefreshPreviousWeaponControllerHud(oldAltController);
    } else {
        ApplyAltWeaponSwitch(saveState, 0, newAltController);
    }
    HudUiMessage::UpdateSelectedWeaponDisplay(newAltController->weaponBankIndex,
                                              newAltController->weaponSideIndex,
                                              newAltController->ammoOrCharge);

    PlayerAltWeaponBank *const primaryBank =
        &playerState->altWeaponBanks[saveData->primaryWeaponBankIndex];
    PlayerGunFireController *const newPrimaryController =
        PlayerSavedWeaponController(primaryBank, saveData->primaryWeaponSideIndex);
    playerState->activePrimaryGunController = newPrimaryController;
    if (oldPrimaryController != newPrimaryController) {
        ApplyPrimaryWeaponSwitch(saveState, oldPrimaryController, newPrimaryController);
        PlayerRefreshPreviousWeaponControllerHud(oldPrimaryController);
    } else {
        ApplyPrimaryWeaponSwitch(saveState, 0, newPrimaryController);
    }
    HudUiMessage::UpdateSelectedWeaponDisplay(newPrimaryController->weaponBankIndex,
                                              newPrimaryController->weaponSideIndex,
                                              newPrimaryController->ammoOrCharge);

    HudUiMgrSensor::SetShieldMessageRatio(
        playerState->statusMeterValue / playerState->masterCommonData->maxHealth);
    HudUiMgr::SetNanitePanelCount(playerState->nanitePanelLevel);

    g_PlayerStatusMeterRatio = saveData->playerStatusMeterRatio;
    g_Player_HudCounterValue = saveData->hudCounterValue;
    playerState->amphibUnlocked = saveData->amphibUnlocked;
    playerState->hoverUnlocked = saveData->hoverUnlocked;
    playerState->subUnlocked = saveData->subUnlocked;
    playerState->aiMode = saveData->aiMode;
    playerState->nextModeSwitchAllowedTime = saveData->nextModeSwitchAllowedTime;
    playerState->motionInput = saveData->motionInput;
    playerState->autoTurnSign = saveData->autoTurnSign;
    playerState->bankInput = saveData->bankInput;

    HudUiMgrObjective::RefreshCounterText(g_Player_HudCounterValue);
    ApplyMasterTypeTransition(saveState, saveData->playerMasterType, 1);
    playerState->primaryGunGateUntilTime = 0.0f;

    zClass_Camera::gwCameraSetTarget(g_MainCamera, saveData->cameraTarget.x,
                                     saveData->cameraTarget.y, saveData->cameraTarget.z);
    zClass_Camera::gwCameraSetPosition(g_MainCamera, saveData->cameraPosition.x,
                                       saveData->cameraPosition.y, saveData->cameraPosition.z);

    playerState->timedHitStatus.ClearLightAndReset();
    playerState->damageProtectionActive = 0;
    if (hasTimedHitStatus != 0) {
        memcpy(&playerState->timedHitStatus, &saveData->timedHitStatus,
               sizeof(saveData->timedHitStatus));
        playerState->timedHitStatus.lightParentNode = playerState->rootNode;

        if ((playerState->timedHitStatus.runtimeFlags & kPlayerTimedHitStatusActiveFlag) != 0) {
            OptCatalogEntryDef *const hitSource =
                OptCatalog::FindEntryById(saveData->timedHitStatus.savedHitSourceEntryId);
            playerState->timedHitStatus.hitSource = hitSource;
            HitSource::UpdateTimedStatus(hitSource, &playerState->timedHitStatus, 0.0f);
            playerState->timedHitStatus.nextUpdateTime += g_Time_AccumulatedTimeSec;
        }
    }
}

// Reimplements 0x41ecd0: Player::RecordNodeFlagsForRestore
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL RecordNodeFlagsForRestore(zClass_NodePartial *node) {
    PlayerNodeFlagRestoreEntry value;
    value.node = node;
    zClass_Class::gwNodeGetCellPickable(node, &value.wasCellPickable);
    zClass_Class::gwNodeGetRaycastable(node, &value.wasRaycastable);
    zClass_Class::gwNodeGetPickable(node, &value.wasPickable);

    PlayerNodeFlagRestoreEntry *begin = g_PlayerNodeFlagRestoreEntriesBegin;
    PlayerNodeFlagRestoreEntry *end = g_PlayerNodeFlagRestoreEntriesEnd;
    PlayerNodeFlagRestoreEntry *capacityEnd = g_PlayerNodeFlagRestoreEntriesCapacityEnd;
    const int count = begin != 0 ? static_cast<int>(end - begin) : 0;
    const int capacity = begin != 0 ? static_cast<int>(capacityEnd - begin) : 0;

    if (count >= capacity) {
        const int newCapacity = count <= 1 ? count + 1 : count * 2;
        PlayerNodeFlagRestoreEntry *const newBegin =
            static_cast<PlayerNodeFlagRestoreEntry *>(
                ::operator new(sizeof(PlayerNodeFlagRestoreEntry) * newCapacity));

        for (int i = 0; i < count; ++i) {
            newBegin[i] = begin[i];
        }

        ::operator delete(begin);
        g_PlayerNodeFlagRestoreEntriesBegin = newBegin;
        g_PlayerNodeFlagRestoreEntriesEnd = newBegin + count;
        g_PlayerNodeFlagRestoreEntriesCapacityEnd = newBegin + newCapacity;
        begin = newBegin;
        end = newBegin + count;
    }

    *end = value;
    g_PlayerNodeFlagRestoreEntriesEnd = end + 1;
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

// Reimplements 0x41f640: Player::ZAR_ReadMissionSaveDataSection
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ZAR_ReadMissionSaveDataSection(
    zZbdSectionCallbackCtx *, const char *, PlayerMissionSaveData *saveData, unsigned int,
    void *) {
    zUtil_PlayerStateStorage *const playerState = g_LocalPlayerSaveState->playerState;

    ApplyMissionSaveData(saveData);
    g_Variant_CurrentTag = saveData->lastValidCameraVariantTag;

    if (playerState->lifecycleState == kPlayerLifecycleInactive) {
        zEffect_Anim::NodeActionCallback(playerState->destroyedRespawnFxEntry,
                                         playerState->rootNode);
    }

    RefreshHudFromState((zUtil_SaveGameState *)(g_GameStateOrMapTable));
    HudUiMgr::TriggerCurrentLayoutOnActivated();
    RestoreRecordedNodeFlags();
}

// Reimplements 0x41f5b0: Player::ZAR_RegisterSections
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_CDECL ZAR_RegisterSections() {
    g_Player_RuntimeInputFlags = 0;
    zUtil_ZAR::RegisterSectionHandler("VehicleList", ZbdCallbackPtr(&ZAR_WriteVehicleListSection),
                                      ZbdCallbackPtr(&ZAR_ReadVehicleListSection), 100, 0);
    zUtil_ZAR::RegisterSectionHandler("Player", ZbdCallbackPtr(&ZAR_WriteMissionSaveDataSection),
                                      ZbdCallbackPtr(&ZAR_ReadMissionSaveDataSection), 200, 0);
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

// Reimplements 0x41f850: Player::ZAR_ReadVehicleListSection
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ZAR_ReadVehicleListSection(
    zZbdSectionCallbackCtx *, const char *sectionToken, PlayerVehicleListSaveEntry *saveData,
    unsigned int, void *) {
    if (saveData->size != 128) {
        zError::ReportOld(0x200, "D:\\Proj\\Battlesport\\player.cpp", 419,
                           "Vehicle save data structure has been modified. Cannot use this save "
                           "set.");
        return;
    }

    zUtil_SaveGameState *saveState = g_PlayerSaveStateListHead;
    while (saveState != 0) {
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        if (strcmp(playerState->rootNode->name, sectionToken) == 0) {
            break;
        }
        saveState = saveState->next;
    }

    if (saveState == 0) {
        return;
    }

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    const int restoreHealthyNode =
        playerState->lifecycleState == kPlayerLifecycleInactive &&
                saveData->masterType != kPlayerLifecycleInactive
            ? 1
            : 0;

    playerState->projectileSpawnVel = zVec3_Make(0.0f, 0.0f, 0.0f);
    playerState->localVel = zVec3_Make(0.0f, 0.0f, 0.0f);
    playerState->yawRotatedLocalVel = zVec3_Make(0.0f, 0.0f, 0.0f);
    playerState->worldPos = saveData->worldPos;
    playerState->vehicleRotationAngles = saveData->vehicleRotationAngles;
    playerState->aiNetId = saveData->aiNetId;
    playerState->aiTopLevelState = saveData->aiTopLevelState;
    playerState->aiSavedTopLevelState = saveData->aiSavedTopLevelState;
    playerState->aiReturnTopLevelState = saveData->aiReturnTopLevelState;

    const float now = g_Time_AccumulatedTimeSec;
    playerState->aiStateUntilTime = now;
    playerState->aiHideTime0 = now;
    playerState->aiHideTime1 = now;
    playerState->unknown_0fa4 = now;
    playerState->aiStateStartTime = now;
    playerState->aiStateEndTime = playerState->aiMode2AttackDwell + now;

    playerState->aiAttackRadiusSq = saveData->aiAttackRadiusSq;
    playerState->aiRestoreDistanceSq = saveData->aiRestoreDistanceSq;
    playerState->aiRestoreTarget = saveData->aiRestoreTarget;
    playerState->aiDynamicOffsetDir = saveData->aiDynamicOffsetDir;
    playerState->unknown_0fd0 = now;
    playerState->aiActivationRadiusSq = saveData->aiActivationRadiusSq;
    playerState->aiTickSuppressed = saveData->aiTickSuppressed;
    playerState->recentHitFlag = saveData->aiAlertFlag;
    playerState->recentHitMarkerHandle = saveData->aiStateMarkerHandle;
    playerState->aiActive = saveData->aiActive;
    playerState->aiPathCursorAdvanceRequested = saveData->aiPathCursorAdvanceRequested;
    playerState->aiCurrentSteeringSubstate = saveData->aiCurrentSteeringSubstate;
    playerState->aiReturnSteeringSubstate = saveData->aiReturnSteeringSubstate;
    playerState->lifecycleState = saveData->masterType;
    playerState->statusMeterScaled = saveData->statusMeterScaled;
    playerState->statusMeterValue = saveData->statusMeterValue;
    playerState->nanitePanelLevel = saveData->nanitePanelLevel;

    SetWorldPoseAndRestartAnchor(saveState, &playerState->worldPos, playerState->restartYawRad);

    if (saveState != (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        TickMasterTypeAndForceFeedback(saveState);
    }

    AiDiscardNegativeBranchPathNodes(saveState);
    playerState->aiCurrentPathNode = (AINetNode *)playerState->aiUnknown_0f7c;
    playerState->aiCurrentPathNeighborIndex = 0;

    if (saveState != (zUtil_SaveGameState *)g_GameStateOrMapTable && restoreHealthyNode != 0) {
        zClass_NodePartial *const healthyNode =
            zClass_Class::FindNodeRecursiveByName(playerState->rootNode, "healthy");
        if (healthyNode != 0) {
            zClass_Object3D::gwObject3DSetPosition(healthyNode, 0.0f, 0.0f, 0.0f);
            zClass_Object3D::gwObject3DSetRotation(healthyNode, 0.0f, 0.0f, 0.0f);
        }

        if (playerState->destroyedRespawnAsyncHandle != 0) {
            zEffect_Anim::NodeActionCallback(playerState->destroyedRespawnAsyncHandle, 0);
        } else {
            zEffect_Anim::NodeActionCallback(playerState->destroyedRespawnFxEntry,
                                             playerState->rootNode);
        }
    }

    zClass_Class::gwNodeSetActive(
        playerState->rootNode, playerState->lifecycleState == kPlayerLifecycleInactive ? 0 : 1);
    zClass_Node::LoadFlagBit8MaterialImagesAndTexturePack(playerState->rootNode);
    zTag4::Clear(&playerState->variantTag);
    zClass_Class::gwNodeSetNodeType(playerState->rootNode, playerState->variantTag.tags[0]);
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

// Reimplements 0x43cdf0: Player::Mines_ZAR_ReadEntryOrReset
// (D:\Proj\GameZRecoil\Player\player_weapon.c)
RECOIL_NOINLINE void RECOIL_FASTCALL
Mines_ZAR_ReadEntryOrReset(zZbdSectionCallbackCtx *, const char *, PlayerMineSaveEntry *mineData,
                           unsigned int, void *) {
    if (mineData->resetMarker != 0) {
        zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)g_GameStateOrMapTable;
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        for (int bankIndex = 4; bankIndex < 6; ++bankIndex) {
            PlayerAltWeaponBank &bank = playerState->altWeaponBanks[bankIndex];
            PlayerGunFireController *controllers[2] = {&bank.controllerA, &bank.controllerB};
            for (int sideIndex = 0; sideIndex < 2; ++sideIndex) {
                OptCatalogEntryDef *const entry = controllers[sideIndex]->optCatalogEntry;
                if (entry != 0) {
                    OptCatalog::ClearRuntimeInstances(entry);
                }
            }
        }
        return;
    }

    OptCatalogEntryDef *const entry = OptCatalog::FindEntryByName(mineData->optCatalogName);
    zClass_NodePartial *const ownerNode = zClass::FindByTypeAndName(6, mineData->ownerNodeName);
    if (entry != 0 && ownerNode != 0) {
        OptCatalogRuntimeInstanceStorage *const runtime =
            OptCatalog::SpawnRuntimeInstanceAt(entry, &mineData->spawnPos, ownerNode);
        zClass_Object3D::gwObject3DSetScale(runtime->projectileNode, mineData->scale.x,
                                            mineData->scale.y, mineData->scale.z);
    }
}

// Reimplements 0x43cc70: Player::WriteMinesZarSection
// (D:\Proj\GameZRecoil\Player\player_weapon.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
WriteMinesZarSection(zZbdSectionCallbackCtx *writer, void *userData) {
    (void)userData;

    PlayerMineSaveEntry data = {};
    data.resetMarker = 1;
    strncpy(data.ownerNodeName, "Dummy", 0x24);

    int writeOk = zUtil_ZAR::WriteSectionBlob(writer, "DummyMineData", &data, 0x60);
    int mineCount = 0;
    for (int bankIndex = 4; writeOk != 0 && bankIndex < 6; ++bankIndex) {
        zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)g_GameStateOrMapTable;
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        PlayerAltWeaponBank &bank = playerState->altWeaponBanks[bankIndex];
        PlayerGunFireController *controllers[2] = {&bank.controllerA, &bank.controllerB};

        for (int sideIndex = 0; writeOk != 0 && sideIndex < 2; ++sideIndex) {
            OptCatalogEntryDef *const entry = controllers[sideIndex]->optCatalogEntry;
            if (entry == 0) {
                continue;
            }

            strncpy(data.optCatalogName, entry->keyName, 0x20);
            OptCatalogRuntimeInstanceStorage *runtime = OptCatalog::MineIterator_Begin(entry);
            while (runtime != 0) {
                data.resetMarker = 0;
                data.spawnPos = runtime->pos;
                zClass_Object3D::gwObject3DGetScale(runtime->projectileNode, &data.scale.x,
                                                    &data.scale.y, &data.scale.z);
                strncpy(data.ownerNodeName, zClass_Class::gwNodeGetName(runtime->ownerNode),
                        0x24);

                char blobToken[0x14];
                sprintf(blobToken, "MineData%03d", mineCount);
                ++mineCount;
                writeOk = zUtil_ZAR::WriteSectionBlob(writer, blobToken, &data, 0x60);
                runtime = OptCatalog::MineIterator_Next();
            }
        }
    }

    return writeOk;
}

// Reimplements 0x42aa50: Player::UpdateDebugOverlayHud
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateDebugOverlayHud(zUtil_SaveGameState *saveState,
                                                           int unusedActiveMode2Count,
                                                           int unusedTotalMode2Count) {
    (void)unusedActiveMode2Count;
    (void)unusedTotalMode2Count;

    if (saveState == 0) {
        return;
    }

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const altController = playerState->activeAltGunController;
    const int reticleMode =
        altController->optCatalogEntry->range > playerState->aimTargetDistanceApprox &&
                altController->ammoOrCharge != 0.0f
            ? 1
            : 0;
    HudUiMgr::SetReticleMode(reticleMode);

    HudUiMessage::SetValueIfOwnerMatches(altController->weaponBankIndex,
                                         altController->weaponSideIndex,
                                         altController->ammoOrCharge);

    PlayerGunFireController *const primaryController = playerState->activePrimaryGunController;
    if (primaryController != 0) {
        HudUiMessage::SetValueIfOwnerMatches(primaryController->weaponBankIndex,
                                             primaryController->weaponSideIndex,
                                             primaryController->ammoOrCharge);
    }

    HudUiMgrObjective::RefreshCounterText(g_Player_HudCounterValue);

    char masterTypeName[12];
    strcpy(masterTypeName,
           PlayerDebugMasterTypeName(saveState->primaryModalState->masterModalData->masterType));

    char debugLine[256];
    const char *const rootName = playerState->rootNode->name;
    if (playerState->lifecycleState == kPlayerLifecycleAi) {
        sprintf(debugLine, "%s is in mode %d and had goal node %d", rootName,
                playerState->aiTopLevelState, playerState->aiCurrentPathNode->nodeIndex);
    } else if (playerState->lifecycleState == kPlayerLifecycleInactive) {
        sprintf(debugLine, "%s is DEAD!", rootName);
    } else if (playerState->lifecycleState == kPlayerLifecycleLocal ||
               playerState->lifecycleState == 0) {
        if (playerState->airborneFlag != 0) {
            sprintf(debugLine, "%s using %s dynamics - A", rootName, masterTypeName);
        } else if (playerState->slipSfxActive != 0) {
            sprintf(debugLine, "%s using %s dynamics - S", rootName, masterTypeName);
        } else {
            sprintf(debugLine, "%s using %s dynamics", rootName, masterTypeName);
        }
    }

    HudUiAuxOverlay::UpdateTextLine(2, 1, debugLine);

    sprintf(debugLine, "POS %d %d %d YAW %d", static_cast<int>(playerState->worldPos.x),
            static_cast<int>(playerState->worldPos.y), static_cast<int>(playerState->worldPos.z),
            static_cast<int>(static_cast<double>(playerState->restartYawRad) *
                             kPlayerRadiansToDegrees));
    HudUiAuxOverlay::UpdateTextLine(2, 2, debugLine);
}

// Reimplements 0x4231b0: Player::RefreshHudFromState
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL RefreshHudFromState(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zUtil_SaveGameState *const localSaveState = (zUtil_SaveGameState *)(g_GameStateOrMapTable);
    HudUiMgrSensor::SetShieldMessageRatio(
        playerState->statusMeterValue / localSaveState->playerState->masterCommonData->maxHealth);
    HudUiMgr::SetNanitePanelCount(playerState->nanitePanelLevel);

    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        PlayerAltWeaponBank &bank = playerState->altWeaponBanks[bankIndex];
        PlayerGunFireController &left = bank.controllerA;
        PlayerGunFireController &right = bank.controllerB;
        const int leftEnabled = (left.flags >> 2) & 1;
        const int rightEnabled = (right.flags >> 2) & 1;

        if (leftEnabled != 0) {
            if (left.ammoOrCharge != 0.0f) {
                HudUiMessage::SelectVariantDisplay(bankIndex, 0);
                HudUiMessage::SetValueIfOwnerMatches(bankIndex, 0, left.ammoOrCharge);
                bank.selectedSide = 0;
                if (rightEnabled != 0) {
                    HudUiMessage::ApplySideImageSwap(bankIndex, 1);
                }
            } else if (rightEnabled != 0 && right.ammoOrCharge != 0.0f) {
                HudUiMessage::SelectVariantDisplay(bankIndex, 1);
                HudUiMessage::SetValueIfOwnerMatches(bankIndex, 1, right.ammoOrCharge);
                bank.selectedSide = 1;
                HudUiMessage::ApplySideImageSwap(bankIndex, 0);
            }
        } else if (rightEnabled != 0) {
            HudUiMessage::SelectVariantDisplay(bankIndex, 1);
            HudUiMessage::SetValueIfOwnerMatches(bankIndex, 1, right.ammoOrCharge);
            bank.selectedSide = 1;
        } else {
            HudUiMessage::ClearDisplay(bankIndex);
            if (left.ammoOrCharge != 0.0f) {
                HudUiMessage::SetValueIfOwnerMatches(bankIndex, 0, left.ammoOrCharge);
            } else if (right.ammoOrCharge != 0.0f) {
                HudUiMessage::SetValueIfOwnerMatches(bankIndex, 0, right.ammoOrCharge);
            }
        }
    }

    HudUiMessage::UpdateSelectedWeaponDisplay(0, 0, 0.0f);

    PlayerGunFireController *const activeAltGunController = playerState->activeAltGunController;
    HudUiMessage::UpdateSelectedWeaponDisplay(
        activeAltGunController->weaponBankIndex, activeAltGunController->weaponSideIndex,
        activeAltGunController->ammoOrCharge);

    PlayerGunFireController *const activePrimaryGunController =
        playerState->activePrimaryGunController;
    HudUiMessage::UpdateSelectedWeaponDisplay(
        activePrimaryGunController->weaponBankIndex, activePrimaryGunController->weaponSideIndex,
        activePrimaryGunController->ammoOrCharge);

    HudUiMgr::SetModeCounterState(1, playerState->amphibUnlocked != 0 ? 1 : 0);
    HudUiMgr::SetModeCounterState(2, playerState->hoverUnlocked != 0 ? 1 : 0);
    HudUiMgr::SetModeCounterState(3, playerState->subUnlocked != 0 ? 1 : 0);
}

// Reimplements 0x43b5d0: Player::ApplyStatusMeterChange
// (D:\Proj\GameZRecoil\Player\player_status.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
ApplyStatusMeterChange(zUtil_SaveGameState *saveState, int mode, float delta) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterCommonData *const masterCommonData = playerState->masterCommonData;

    if (mode != 0) {
        playerState->statusMeterValue += delta;
    } else {
        playerState->statusMeterValue = delta;
    }

    if (!(playerState->statusMeterValue <= masterCommonData->maxHealth)) {
        playerState->statusMeterValue = masterCommonData->maxHealth;
    } else if (playerState->statusMeterValue < 0.0f) {
        playerState->statusMeterValue = 0.0f;
    }

    g_PlayerStatusMeterRatio =
        masterCommonData->invMaxHealth * playerState->statusMeterValue;
    HudUiMgrSensor::SetShieldMessageRatio(g_PlayerStatusMeterRatio);
}

// Reimplements 0x43b660: Player::UpdateStatusMeter
// (D:\Proj\GameZRecoil\Player\player_status.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
UpdateStatusMeter(zUtil_SaveGameState *saveState, int mode, float delta) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    if (mode == 0) {
        ApplyStatusMeterChange(saveState, mode, playerState->masterCommonData->maxHealth);
        HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x902), 5.0f);
        HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x246), 5.0f);
        zEffectAnim::SetVelocity_Thunk(playerState->regenSkinFxEntry, 0, 0.0f, 0.0f, 0.0f);
        ResetDamageStateAndTimedHitStatus(saveState);
        return 1;
    }

    const float oldStatusMeterRatio = g_PlayerStatusMeterRatio;
    ApplyStatusMeterChange(saveState, 1, delta);

    char message[64];
    const int percentGain =
        static_cast<int>((g_PlayerStatusMeterRatio - oldStatusMeterRatio) * 100.0f);
    zLoc::FormatMessage(message, sizeof(message), 0x903, percentGain);
    HudUi::ShowTopMessageLine(message, 5.0f);
    return 1;
}

// Reimplements 0x438b60: Player::FreeAltWeaponTrailRuntimeStates (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
FreeAltWeaponTrailRuntimeStates(zUtil_SaveGameState *saveState) {
    PlayerAltWeaponBank *bank = &saveState->playerState->altWeaponBanks[1];
    for (int i = 0; i < 9; ++i, ++bank) {
        OptCatalogTrailRuntimeState *const controllerATrail =
            bank->controllerA.trailRuntimeState;
        if (controllerATrail != 0) {
            OptCatalog::FreeTrailRuntimeStateStorage(controllerATrail);
        }

        OptCatalogTrailRuntimeState *const controllerBTrail =
            bank->controllerB.trailRuntimeState;
        if (controllerBTrail != 0) {
            OptCatalog::FreeTrailRuntimeStateStorage(controllerBTrail);
        }
    }
}

// Reimplements 0x438ba0: Player::LoadWeaponBanksAndSelectDefaults
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
LoadWeaponBanksAndSelectDefaults(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterCommonData *const masterCommonData = playerState->masterCommonData;

    FreeAltWeaponTrailRuntimeStates(saveState);

    const float resetAmmoOrCharge =
        playerState->lifecycleState == kPlayerLifecycleRemote
            ? kPlayerAltAmmoDisabledSentinel
            : 0.0f;
    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        PlayerAltWeaponBank &bank = playerState->altWeaponBanks[bankIndex];
        bank.selectedSide = 0;
        PlayerResetWeaponController(&bank.controllerA, bankIndex, 0, resetAmmoOrCharge);
        PlayerResetWeaponController(&bank.controllerB, bankIndex, 1, resetAmmoOrCharge);
    }

    int trailSegmentCount = 1;
    if (playerState->playerOrdinal == 1 ||
        strstr(playerState->rootNode->name, "net") != 0) {
        trailSegmentCount = 8;
    }

    if (masterCommonData->weaponNodeCount > 0 && masterCommonData->weaponSpecHead != 0) {
        int altDefaultSelected = 0;
        int primaryDefaultSelected = 0;
        PlayerMasterWeaponSpec *weaponSpec = masterCommonData->weaponSpecHead;
        while (weaponSpec != 0) {
            char optCatalogName[0x50];
            strncpy(optCatalogName, weaponSpec->optCatalogName, sizeof(optCatalogName));
            optCatalogName[sizeof(optCatalogName) - 1] = 0;

            const int bankIndex = optCatalogName[4] - '0';
            const int sideIndex = optCatalogName[6] - '0';
            PlayerAltWeaponBank &bank = playerState->altWeaponBanks[bankIndex];
            PlayerGunFireController *const controller =
                sideIndex == 0 ? &bank.controllerA : &bank.controllerB;

            controller->optCatalogEntry = OptCatalog::FindEntryByName(optCatalogName);

            int available = 0;
            const int packedWeaponSlotId = (bankIndex << 4) | sideIndex;
            CheckMissionWeaponAvailability(saveState, weaponSpec->missionRequirementOrGateId,
                                           packedWeaponSlotId, &available);

            controller->flags =
                (controller->flags & ~kPlayerGunControllerDualMountFlag) |
                ((weaponSpec->mountLayoutFlags & 1) << 1);
            if (available != 0) {
                if (controller->optCatalogEntry != 0) {
                    controller->flags |= kPlayerGunControllerAvailableFlag;
                } else {
                    controller->flags &= ~kPlayerGunControllerAvailableFlag;
                }
            }
            controller->ammoOrCharge =
                (controller->flags & kPlayerGunControllerAvailableFlag) != 0
                    ? weaponSpec->startAmmoOrCharge
                    : 0.0f;
            controller->nextDispatchTime = 0.0f;
            controller->dispatchRepeatDelay = weaponSpec->dispatchRepeatDelay;
            controller->aiAttackRangeMin = weaponSpec->aiAttackRangeMin;
            controller->aiAttackRangeMax = weaponSpec->aiAttackRangeMax;
            controller->flags =
                (controller->flags & ~kPlayerGunControllerRecoilFlag) |
                (weaponSpec->fireSlotRecoilFlags & kPlayerGunControllerRecoilFlag);
            controller->initialHardpointSelectState =
                weaponSpec->initialHardpointSelectState;

            if (playerState->gunNode != 0 && controller->optCatalogEntry != 0) {
                if ((controller->flags & kPlayerGunControllerDualMountFlag) != 0) {
                    PlayerBindDualWeaponMount(playerState, controller);
                } else {
                    PlayerBindSingleWeaponMount(playerState, controller);
                }
            }

            if (controller->optCatalogEntry != 0 &&
                (controller->optCatalogEntry->flags & kOptCatalogFlagCreateTrail) != 0) {
                controller->trailRuntimeState = OptCatalog::CreateTrailRuntimeState(
                    controller->optCatalogEntry, playerState->rootNode, &playerState->variantTag,
                    controller->attachNodePrimary, &playerState->altFireOrigin,
                    &playerState->gunFireDir, trailSegmentCount);
            }

            if (zOpt::GetNetworkEnabled() == 0 && available != 0) {
                if (altDefaultSelected == 0) {
                    ApplyAltWeaponSwitch(saveState, 0, controller);
                    altDefaultSelected = 1;
                } else if (primaryDefaultSelected == 0) {
                    ApplyPrimaryWeaponSwitch(saveState, 0, controller);
                    primaryDefaultSelected = 1;
                }
            }

            weaponSpec = weaponSpec->next;
        }
    }

    if (zOpt::GetNetworkEnabled() != 0) {
        int selected = 0;
        for (int bankIndex = 2; selected == 0 && bankIndex < 10; ++bankIndex) {
            PlayerAltWeaponBank &bank = playerState->altWeaponBanks[bankIndex];
            if ((bank.controllerA.flags & kPlayerGunControllerAvailableFlag) != 0) {
                ApplyAltWeaponSwitch(saveState, 0, &bank.controllerA);
                selected = 1;
            } else if ((bank.controllerB.flags & kPlayerGunControllerAvailableFlag) != 0) {
                ApplyAltWeaponSwitch(saveState, 0, &bank.controllerB);
                selected = 1;
            }
        }
    }

    if (playerState->activeAltGunController == 0) {
        ApplyAltWeaponSwitch(saveState, 0, &playerState->altWeaponBanks[1].controllerA);
    }

    PlayerGunFireController *primaryController = &playerState->altWeaponBanks[1].controllerA;
    if ((playerState->altWeaponBanks[1].controllerB.flags &
         kPlayerGunControllerAvailableFlag) != 0) {
        primaryController = &playerState->altWeaponBanks[1].controllerB;
    }
    ApplyPrimaryWeaponSwitch(saveState, 0, primaryController);

    PlayerGunFireController *const activeAltGunController =
        playerState->activeAltGunController;
    if (activeAltGunController->attachNodePrimary != 0) {
        zClass_Class::gwNodeSetActive(activeAltGunController->attachNodePrimary, 1);
    }

    playerState->altHardpointSelectState =
        activeAltGunController->initialHardpointSelectState == 2 ? 2 : 0;
    playerState->cachedAltSelectionCode =
        activeAltGunController->weaponBankIndex * 100 +
        activeAltGunController->weaponSideIndex;

    PlayerGunFireController *const activePrimaryGunController =
        playerState->activePrimaryGunController;
    if (activePrimaryGunController != 0) {
        playerState->cachedPrimarySelectionCode =
            activePrimaryGunController->weaponBankIndex * 100 +
            activePrimaryGunController->weaponSideIndex;
    }

    playerState->pendingAltCameraToggle = 0;
    playerState->timedHitStatus.lightParentNode = playerState->rootNode;
    playerState->timedHitStatus.ResetFields();

    zUtil_ZAR::RegisterSectionHandler(
        "Mines", ZbdCallbackPtr(&WriteMinesZarSection),
        ZbdCallbackPtr(&Mines_ZAR_ReadEntryOrReset), 1000, 0);
}

// Reimplements 0x43ca90: Player::CheckMissionWeaponAvailability
// (D:\Proj\GameZRecoil\Player\player_weapon.c)
RECOIL_NOINLINE void RECOIL_FASTCALL
CheckMissionWeaponAvailability(zUtil_SaveGameState *saveState, int missionThreshold,
                               int packedWeaponSlotId, int *availableOut) {
    (void)saveState;

    const int currentMissionId = g_HudSensorTracker.GetMissionId();
    if (zOpt::GetNetworkEnabled() == 0) {
        *availableOut =
            missionThreshold != 0 && missionThreshold <= currentMissionId ? 1 : 0;
        return;
    }

    const int networkWhitelist[13][4] = {
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0x10, 0x11, 0x20, 0},
        {0x10, 0x11, 0x61, 0},
        {0x10, 0x11, 0x31, 0},
        {0x10, 0x11, 0x20, 0},
        {0x10, 0x11, 0x80, 0},
        {0x10, 0x11, 0x20, 0},
        {0x10, 0x11, 0x31, 0},
    };

    *availableOut = 0;
    if (currentMissionId < 1 || currentMissionId > 13) {
        return;
    }

    const int *const row = networkWhitelist[currentMissionId - 1];
    for (int i = 0; i < 4; ++i) {
        if (packedWeaponSlotId == row[i]) {
            *availableOut = 1;
            return;
        }
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

// Reimplements 0x424bf0: Player::Vec3_FastNormalize
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL Vec3_FastNormalize(zVec3 *vec) {
    const float lengthSq = vec->x * vec->x + vec->y * vec->y + vec->z * vec->z;
    if (lengthSq >= 0.01f || lengthSq == 0.0f) {
        return 0;
    }

    int lengthSqBits = 0;
    memcpy(&lengthSqBits, &lengthSq, sizeof(lengthSqBits));
    lengthSqBits = (lengthSqBits >> 1) + 532676608;

    float approxLength = 0.0f;
    memcpy(&approxLength, &lengthSqBits, sizeof(approxLength));
    const float scale = g_Player_CollisionContactResolveScale / (approxLength + 0.00000001f);

    vec->x *= scale;
    vec->y *= scale;
    vec->z *= scale;
    return 1;
}

// Reimplements 0x424c90: Player::ConstrainToUnitDistanceFrom
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ConstrainToUnitDistanceFrom(zVec3 *pos,
                                                                 const zVec3 *center) {
    zVec3 delta = {pos->x - center->x, pos->y - center->y, pos->z - center->z};
    if (Vec3_FastNormalize(&delta) == 0) {
        return;
    }

    pos->x = center->x + delta.x;
    pos->y = center->y + delta.y;
    pos->z = center->z + delta.z;
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

// Reimplements 0x4289f0: Player::UpdateSubModeWaterProbeState
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateSubModeWaterProbeState(zUtil_SaveGameState *saveState) {
    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = primaryModalState->masterModalData;

    float probeHeightByPoint[kPlayerMaxModalProbePoints] = {};
    float outBestHeight = 0.0f;
    PlayerProbeTypeHistogram outTypeHistogram = {};
    int outAttachmentCandidateCount = 0;
    zClass_NodePartial *outAttachmentNode = 0;
    ProbeModalSampleHeights(saveState, probeHeightByPoint, &outBestHeight, 1,
                            &outTypeHistogram, &outAttachmentCandidateCount,
                            &outAttachmentNode);

    playerState->yawVelocityLimit = masterModalData->yawRateMax;
    if (outBestHeight == -300.0f) {
        outBestHeight = 1000.0f;
    }
    playerState->subModeProbeBestHeight = outBestHeight;

    float deepestSubmergedSampleHeight = -300.0f;
    int deepestSubmergedSampleIndex = 0;
    const int probePointCount = primaryModalState->modalStateCode;
    for (int i = 0; i < probePointCount; ++i) {
        const float sampleHeight = probeHeightByPoint[i];
        if (sampleHeight < outBestHeight && deepestSubmergedSampleHeight < sampleHeight) {
            deepestSubmergedSampleHeight = sampleHeight;
            deepestSubmergedSampleIndex = i;
        }
    }

    if (playerState->worldCollisionResolved != 1) {
        float resolvedY = masterModalData->modeAltTransitionTime + outBestHeight;
        if (playerState->worldPos.y >= resolvedY) {
            const float submergedProbeBaseHeight =
                deepestSubmergedSampleHeight -
                masterModalData->probePoints[15 + deepestSubmergedSampleIndex].y;
            if (playerState->worldPos.y > submergedProbeBaseHeight) {
                resolvedY = submergedProbeBaseHeight;
            } else {
                resolvedY = playerState->worldPos.y;
            }
        }

        playerState->worldPos.y = resolvedY;
        playerState->motionBasis.posY = resolvedY;
    }

    const float rollDampingFactor =
        PlayerFloatFromBits(static_cast<int>(-g_Player_DeltaTime * 12102200.0f) +
                            0x3f800000);
    playerState->angVelRoll = -(rollDampingFactor * playerState->vehicleRollRad);

    const float speedAbs = static_cast<float>(fabs(playerState->localVel.z));
    const float pitchWaveRate =
        speedAbs * masterModalData->hoverPitchWaveSpeedRate +
        masterModalData->hoverPitchWaveBaseRate;
    const float rollWaveRate =
        speedAbs * masterModalData->hoverRollWaveSpeedRate +
        masterModalData->hoverRollWaveBaseRate;
    const float pitchBobDelta =
        static_cast<float>(sin(pitchWaveRate * g_Time_AccumulatedTimeSec)) *
        masterModalData->hoverPitchWaveAmplitude;
    const float rollBobDelta =
        static_cast<float>(sin(rollWaveRate * g_Time_AccumulatedTimeSec)) *
        masterModalData->hoverRollWaveAmplitude;

    playerState->vehiclePitchRad += g_Player_DeltaTime * pitchBobDelta;
    playerState->vehicleRollRad += g_Player_DeltaTime * rollBobDelta;

    if (playerState->underwaterFxEnabled != 0 &&
        playerState->cameraTarget.y < outBestHeight) {
        SetHudUiElementVisible(&g_Player_UnderwaterFxPass3Ui, 1);
        g_Player_HorizonNodeFollowCameraEnabled = 0;

        zClass_NodePartial *const nodeCaustic1 = primaryModalState->nodeCaustic1;
        if (nodeCaustic1 != 0) {
            unsigned int displayInstanceValue = 0;
            zClass_Class::gwNodeGetUserData(nodeCaustic1, &displayInstanceValue);
            zDi::SetCurrentVariantCycleTextureSpeed((zDiPartial *)displayInstanceValue, 12.0f);
        }
    }

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable &&
        playerState->worldPos.y + 2.20000005f >= outBestHeight) {
        TransitionToMasterTypeAmphib(saveState, 0, 0);
    }
}

// Reimplements 0x428c20: Player::UpdateSubVerticalDamping
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateSubVerticalDamping(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;

    if (playerState->subVerticalInput != 0.0f) {
        if ((playerState->subVerticalInputCopy > 0.0f && playerState->localVel.y < 0.0f) ||
            (playerState->subVerticalInputCopy < 0.0f && playerState->localVel.y > 0.0f)) {
            playerState->localVel.y = 0.0f;
        }

        const float localY = masterModalData->accelRate * g_Player_DeltaTime *
                                 playerState->subVerticalInputCopy +
                             playerState->localVel.y;
        playerState->localVel.y = localY;
        if (localY > 20.0f) {
            playerState->localVel.y = 20.0f;
        } else if (localY < -20.0f) {
            playerState->localVel.y = -20.0f;
        }
        return;
    }

    if (playerState->throttleInputCopy == 0.0f) {
        const float dampingRate =
            g_Time_AccumulatedTimeSec < playerState->primaryGunGateUntilTime ? 2.0f : 10.0f;
        float dampingScale = dampingRate * g_Player_DeltaTime;
        dampingScale = -dampingScale;
        int dampingBits = static_cast<int>(dampingScale * 12102200.0f);
        const int dampingFloatBits = dampingBits + 0x3f800000;

        float dampingFactor = 0.0f;
        memcpy(&dampingFactor, &dampingFloatBits, sizeof(dampingFactor));
        playerState->localVel.y *= dampingFactor;
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

// Reimplements 0x428520: Player::UpdateMasterTypeSub
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateMasterTypeSub(zUtil_SaveGameState *saveState) {
    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = primaryModalState->masterModalData;

    CacheDisableCopterSndNodesAndStopSample();
    RebuildSteerBasisFromMotionAxes(saveState);
    UpdateAutoTurnAndSteerFromTarget(saveState);

    if (playerState->subPitchInput == 0.0f) {
        playerState->angVelPitch = 0.0f;
        playerState->vehiclePitchRad *= PlayerDampingFromRate(7.0f);
    } else {
        if ((playerState->subPitchInputCopy > 0.0f && playerState->angVelPitch < 0.0f) ||
            (playerState->subPitchInputCopy < 0.0f && playerState->angVelPitch > 0.0f)) {
            playerState->angVelPitch = 0.0f;
        }

        playerState->angVelPitch +=
            masterModalData->yawAccel * g_Player_DeltaTime *
            playerState->subPitchInputCopy * 0.5f;
        playerState->angVelPitch =
            PlayerClampSigned(playerState->angVelPitch, masterModalData->yawRateMax);
    }

    playerState->vehiclePitchRad += playerState->angVelPitch * g_Player_DeltaTime;
    playerState->restartYawRad =
        PlayerWrapSignedTwoPi(playerState->restartYawRad +
                              playerState->angVelYaw * g_Player_DeltaTime);
    playerState->vehicleRollRad += playerState->angVelRoll * g_Player_DeltaTime;
    playerState->vehicleRollRad -=
        masterModalData->hoverRollYawCoupleScale * playerState->angVelYaw *
        playerState->localVel.z;
    playerState->vehiclePitchRad = PlayerClampSigned(playerState->vehiclePitchRad, 0.5f);
    playerState->vehicleRollRad =
        PlayerClampSigned(playerState->vehicleRollRad, 0.349999994f);

    zMath::MatBuildEulerRotation3x3(&playerState->motionBasis,
                                    playerState->vehiclePitchRad,
                                    playerState->restartYawRad,
                                    playerState->vehicleRollRad);
    RebuildSteerBasisFromMotionBasis(saveState);
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;
    playerState->axisClampRuntime = masterModalData->maxSpeed;

    UpdateYawVelocityFromSteerInput(saveState);
    UpdateSubVerticalDamping(saveState);

    playerState->projectileSpawnVel =
        TransformLocalVectorToWorld(playerState->localVel, playerState->motionBasis);
    playerState->worldPos.x += playerState->projectileSpawnVel.x * g_Player_DeltaTime;
    playerState->worldPos.y += playerState->projectileSpawnVel.y * g_Player_DeltaTime;
    playerState->worldPos.z += playerState->projectileSpawnVel.z * g_Player_DeltaTime;
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;

    ProcessPendingContactQueues(saveState);
    UpdateSubModeWaterProbeState(saveState);
    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        ProcessPendingContactQueues(saveState);
        if (CollectPendingCollisionContactsForQuadProbe(saveState, 0.0f) != 0 ||
            CollectPendingCollisionContactsForQuadProbe(saveState, 1.25f) != 0) {
            ApplyPendingCollisionProbeVelocity(saveState);
            playerState->collisionProbeResolved = 1;
        } else {
            playerState->collisionProbeResolved = 0;
        }
    }

    zClass_Object3D::gwObject3DSetRotation(playerState->rootNode,
                                           playerState->vehiclePitchRad,
                                           playerState->restartYawRad,
                                           playerState->vehicleRollRad);
    zClass_Object3D::gwObject3DSetPosition(playerState->rootNode, playerState->worldPos.x,
                                           playerState->worldPos.y,
                                           playerState->worldPos.z);
    playerState->fxOffsetWorld.x = playerState->fxOffsetLocal.x + playerState->worldPos.x;
    playerState->fxOffsetWorld.y = playerState->fxOffsetLocal.y + playerState->worldPos.y;
    playerState->fxOffsetWorld.z = playerState->fxOffsetLocal.z + playerState->worldPos.z;
    zClass_Class::gwNodeUpdate(playerState->rootNode);

    float *const rootMatrix = zClass_Object3D::gwObject3DGetMatrixPtr(playerState->rootNode);
    memcpy(&playerState->previousTransform, rootMatrix, sizeof(playerState->previousTransform));
    playerState->bankBasis = playerState->steerBasisNorm;
    playerState->cachedPitchRad = playerState->vehiclePitchRad;
    playerState->cachedYawRad = playerState->restartYawRad;
    playerState->cachedRollRad = playerState->vehicleRollRad;

    zClass_NodePartial *const nodeProps = primaryModalState->nodeProps;
    if (nodeProps != 0) {
        const float cycleSpeed = 6.0f - playerState->localVel.z * 0.5f;
        unsigned int displayInstanceValue = 0;
        zClass_Class::gwNodeGetUserData(nodeProps, &displayInstanceValue);
        zDi::SetCurrentVariantCycleTextureSpeed((zDiPartial *)displayInstanceValue,
                                                cycleSpeed);
    }
}

// Reimplements 0x4266b0: Player::TickMasterTypeAndForceFeedback
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
TickMasterTypeAndForceFeedback(zUtil_SaveGameState *saveState) {
    if (saveState == 0) {
        return;
    }

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    if (playerState->lifecycleState == kPlayerLifecycleInactive) {
        return;
    }

    if (playerState->damageProtectionActive != 0) {
        playerState->subPitchInput = 0.0f;
        playerState->subVerticalInput = 0.0f;
        playerState->throttleInput = 0.0f;
        playerState->steeringInput = 0.0f;
    }

    switch (masterModalData->masterType) {
    case 0:
        UpdateMasterTypeBasic(saveState);
        break;
    case kPlayerMasterTypeSub:
        UpdateMasterTypeSub(saveState);
        break;
    case kPlayerMasterTypeTrack:
        UpdateMasterTypeTrack(saveState);
        break;
    case kPlayerMasterTypeHover:
        UpdateMasterTypeHover(saveState);
        break;
    case kPlayerMasterTypeAmphib:
        UpdateMasterTypeAmphib(saveState);
        break;
    default:
        break;
    }

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        zEffect::SetConditionalRefPos(&playerState->worldPos);
        if (zInput_DI_IsForceFeedbackEnabled() != 0) {
            zInput_DI_UpdateSteerAndPitchForceEffects(g_zInputFfEffectSet);
        }
    }
}

// Reimplements 0x426770: Player::UpdateMasterTypeTrack
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateMasterTypeTrack(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    RebuildSteerBasisFromMotionAxes(saveState);

    if (playerState->airborneFlag != 0) {
        playerState->angVelYaw *= PlayerDampingFromRate(1.0f);
        if (playerState->slipSfxActive != 0) {
            StopSlipSfx(saveState);
        }
    } else {
        UpdateAutoTurnAndSteerFromTarget(saveState);
    }

    const float yawDelta = g_Player_DeltaTime * playerState->angVelYaw;
    if (playerState->environmentAttachmentActive != 0) {
        playerState->yawPoseCache = PlayerWrapSignedTwoPi(playerState->yawPoseCache + yawDelta);
        zMath::MatStackPushPtr((float *)&playerState->environmentAttachmentMatrix);
        zMath::MatLoadIdentity();
        gwNode::BuildNodeToAncestorMatrix(playerState->environmentAttachmentNode, 3);
        zMath::MatStackPopPtr();
        playerState->restartYawRad =
            ExtractYawFromMatrix(&playerState->environmentAttachmentMatrix) +
            playerState->yawPoseCache;
    } else {
        playerState->restartYawRad =
            PlayerWrapSignedTwoPi(playerState->restartYawRad + yawDelta);
        playerState->pitchPoseCache = playerState->vehiclePitchRad;
        playerState->yawPoseCache = playerState->restartYawRad;
        playerState->rollPoseCache = playerState->vehicleRollRad;
    }

    zMath::MatBuildEulerRotation3x3(&playerState->motionBasis, playerState->vehiclePitchRad,
                                    playerState->restartYawRad, playerState->vehicleRollRad);
    RebuildSteerBasisFromMotionBasis(saveState);
    if (playerState->airborneFlag == 0) {
        UpdateYawVelocityFromSteerInput(saveState);
    }

    if (playerState->environmentAttachmentActive != 0) {
        zMath::Vec3RotateY(&playerState->yawRotatedLocalVel, &playerState->localVel,
                           playerState->yawPoseCache);
        playerState->fxOffsetLocal.x +=
            g_Player_DeltaTime * playerState->yawRotatedLocalVel.x;
        playerState->fxOffsetLocal.z +=
            g_Player_DeltaTime * playerState->yawRotatedLocalVel.z;

        const zVec3 attachedWorld =
            TransformPointByMatrix(playerState->fxOffsetLocal,
                                   playerState->environmentAttachmentMatrix);
        playerState->projectileSpawnVel.x =
            (attachedWorld.x - playerState->worldPos.x) * g_Player_InvDeltaTime;
        playerState->projectileSpawnVel.y =
            (attachedWorld.y - playerState->worldPos.y) * g_Player_InvDeltaTime;
        playerState->projectileSpawnVel.z =
            (attachedWorld.z - playerState->worldPos.z) * g_Player_InvDeltaTime;
        playerState->worldPos = attachedWorld;
    } else {
        if (playerState->airborneFlag != 0) {
            const float airborneDamping = PlayerDampingFromRate(0.200000003f);
            playerState->projectileSpawnVel.x *= airborneDamping;
            playerState->projectileSpawnVel.z *= airborneDamping;
            playerState->localVel = playerState->projectileSpawnVel;
            playerState->localVel =
                TransformWorldVectorToLocal(playerState->localVel, playerState->motionBasis);
        } else {
            playerState->projectileSpawnVel =
                TransformLocalVectorToWorld(playerState->localVel, playerState->motionBasis);
        }

        playerState->worldPos.x += g_Player_DeltaTime * playerState->projectileSpawnVel.x;
        playerState->yawRotatedLocalVel = playerState->projectileSpawnVel;
        playerState->worldPos.z += g_Player_DeltaTime * playerState->projectileSpawnVel.z;
    }

    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;
    if (playerState->lifecycleState != 0) {
        ProcessPendingContactQueues(saveState);
    }

    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    if (masterModalData->masterType == 0) {
        UpdateMasterTypeBasicOrTrack_FromModalProbe(saveState);
        playerState->airborneFlag = 0;
    } else if (masterModalData->masterType == kPlayerMasterTypeTrack) {
        UpdatePostMoveEnvironment(saveState,
                                  saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable ? 7
                                                                                              : 4);
    }

    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        ProcessPendingContactQueues(saveState);
        if (CollectPendingCollisionContactsForQuadProbe(saveState, 0.0f) != 0) {
            ApplyPendingCollisionProbeVelocity(saveState);
            playerState->collisionProbeResolved = 1;
        } else {
            playerState->collisionProbeResolved = 0;
        }
    }

    if (playerState->airborneFlag != playerState->airborneFlagPrev) {
        zClass_Class::gwNodeSetActive(playerState->modeVariantNode,
                                      playerState->airborneFlag == 0 ? 1 : 0);
    }
    playerState->airborneFlagPrev = playerState->airborneFlag;

    if (playerState->environmentAttachmentActive != 0) {
        CacheAttachmentLocalOffset(playerState);
    }

    zClass_Object3D::gwObject3DSetRotation(playerState->rootNode, playerState->vehiclePitchRad,
                                           playerState->restartYawRad,
                                           playerState->vehicleRollRad);
    zClass_Object3D::gwObject3DSetPosition(playerState->rootNode, playerState->worldPos.x,
                                           playerState->worldPos.y,
                                           playerState->worldPos.z);
    playerState->fxOffsetWorld.x = playerState->fxOffsetLocal.x + playerState->worldPos.x;
    playerState->fxOffsetWorld.y = playerState->fxOffsetLocal.y + playerState->worldPos.y;
    playerState->fxOffsetWorld.z = playerState->fxOffsetLocal.z + playerState->worldPos.z;

    if (primaryModalState->modalNode != 0 &&
        masterModalData->masterType == kPlayerMasterTypeTrack) {
        const float dampingWeight =
            PlayerDampingFromRate(masterModalData->chassisSmoothFactor);
        const float newWeight = 1.0f - dampingWeight;
        const float pitchTarget =
            masterModalData->chassisPitchRate * playerState->angVelPitch +
            masterModalData->chassisPitchMax * playerState->localVel.z;
        const float pitchFiltered =
            dampingWeight * primaryModalState->chassisPitchFilterState +
            newWeight * pitchTarget;
        primaryModalState->chassisPitchFilterState = pitchFiltered;
        const float rollFiltered =
            dampingWeight * primaryModalState->chassisRollFilterState;
        primaryModalState->chassisRollFilterState = rollFiltered;

        primaryModalState->chassisPitchAngleRad =
            PlayerClampSigned(pitchTarget - pitchFiltered,
                              masterModalData->chassisPitchDamping);
        primaryModalState->chassisRollAngleRad =
            PlayerClampSigned(masterModalData->chassisRollMax * playerState->angVelYaw *
                                  playerState->localVel.z -
                                  rollFiltered,
                              masterModalData->chassisRollDamping);
        zClass_Object3D::gwObject3DSetRotation(primaryModalState->modalNode,
                                               primaryModalState->chassisPitchAngleRad,
                                               0.0f,
                                               primaryModalState->chassisRollAngleRad);
    }

    zClass_Class::gwNodeUpdate(playerState->rootNode);
    if (primaryModalState->modalNode != 0) {
        zClass_Class::gwNodeUpdate(primaryModalState->modalNode);
    }
    float *const rootMatrix = zClass_Object3D::gwObject3DGetMatrixPtr(playerState->rootNode);
    memcpy(&playerState->previousTransform, rootMatrix, sizeof(playerState->previousTransform));
    playerState->bankBasis = playerState->steerBasisNorm;
    playerState->cachedVehicleRotationAngles = playerState->vehicleRotationAngles;

    if (primaryModalState->nodeRTracks != 0) {
        const float rightTrackSpeed =
            -playerState->localVel.z - playerState->angVelYaw * -2.25f;
        const float rightTrackSpeedAbs = static_cast<float>(fabs(rightTrackSpeed));
        int variantIndex = 0;
        if (rightTrackSpeedAbs >= playerState->masterCommonData->trackSwitchDist2) {
            variantIndex = 3;
        } else if (rightTrackSpeedAbs >= playerState->masterCommonData->trackSwitchDist1) {
            variantIndex = 2;
        } else if (rightTrackSpeedAbs >= playerState->masterCommonData->trackSwitchDist0) {
            variantIndex = 1;
        }

        unsigned int displayInstanceValue = 0;
        zClass_Class::gwNodeGetUserData(primaryModalState->nodeRTracks,
                                        &displayInstanceValue);
        zDi::SetCurrentVariant((zDiPartial *)displayInstanceValue, variantIndex);
        zClass_Class::gwNodeGetUserData(primaryModalState->nodeRTracks,
                                        &displayInstanceValue);
        zModel::SetDiTextureWorldPerMeter((zDiPartial *)displayInstanceValue, 1,
                                          rightTrackSpeed * 1.72000003f, 0);
        zModel_Instance_UpdateScrollingTexturesIfNeeded(
            (zModel_InstancePartial *)displayInstanceValue);

        const float leftTrackSpeed =
            -playerState->localVel.z - playerState->angVelYaw * 2.25f;
        zClass_Class::gwNodeGetUserData(primaryModalState->nodeLTracks,
                                        &displayInstanceValue);
        zModel::SetDiTextureWorldPerMeter((zDiPartial *)displayInstanceValue, 1,
                                          leftTrackSpeed * 1.72000003f, 0);
        zModel_Instance_UpdateScrollingTexturesIfNeeded(
            (zModel_InstancePartial *)displayInstanceValue);
    }

    if (primaryModalState->nodeDustL != 0 && primaryModalState->nodeDustR != 0) {
        if (playerState->airborneFlag != 0) {
            zClass_Object3D::gwObject3DSetScale(primaryModalState->nodeDustL, 0.0f, 0.0f,
                                                0.0f);
            zClass_Object3D::gwObject3DSetScale(primaryModalState->nodeDustR, 0.0f, 0.0f,
                                                0.0f);
        } else {
            const float dustScale =
                static_cast<float>(fabs(playerState->localVel.z)) /
                playerState->axisClampRuntime;
            zClass_Object3D::gwObject3DSetScale(primaryModalState->nodeDustL, dustScale,
                                                dustScale, dustScale);
            zClass_Object3D::gwObject3DSetScale(primaryModalState->nodeDustR, dustScale,
                                                dustScale, dustScale);
        }
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

// Reimplements 0x43c630: Player::IsAltWeaponAllowedInCurrentMasterMode
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL IsAltWeaponAllowedInCurrentMasterMode(
    zUtil_SaveGameState *saveState, OptCatalogEntryDef *entry) {
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    if (masterModalData->masterType == kPlayerMasterTypeSub) {
        const unsigned int flags = entry->flags;
        if ((flags & kOptCatalogFlagBlockedInSub) != 0 ||
            (flags & kOptCatalogFlagNoSubUse) != 0) {
            return 0;
        }
    }

    return 1;
}

static int IsUsableAltWeaponController(zUtil_SaveGameState *saveState,
                                       PlayerGunFireController *controller) {
    return (controller->flags & 4) != 0 &&
           IsAltWeaponAllowedInCurrentMasterMode(saveState, controller->optCatalogEntry) != 0 &&
           controller->ammoOrCharge > 0.0f;
}

// Reimplements 0x43c660: Player::AutoSwitchToNextUsableAltWeapon
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
AutoSwitchToNextUsableAltWeapon(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const activeController = playerState->activeAltGunController;
    const int activeBankIndex = activeController->weaponBankIndex;
    if (activeBankIndex == 0) {
        return;
    }

    PlayerAltWeaponBank *bank = &playerState->altWeaponBanks[activeBankIndex];
    PlayerGunFireController *candidate =
        activeController->weaponSideIndex == 0 ? &bank->controllerB : &bank->controllerA;
    if (IsUsableAltWeaponController(saveState, candidate) != 0) {
        HandleAltWeaponBankSelectInput(activeBankIndex + 14);
        return;
    }

    for (int bankIndex = activeBankIndex - 1; bankIndex > 1; --bankIndex) {
        bank = &playerState->altWeaponBanks[bankIndex];
        if (IsUsableAltWeaponController(saveState, &bank->controllerA) != 0 ||
            IsUsableAltWeaponController(saveState, &bank->controllerB) != 0) {
            HandleAltWeaponBankSelectInput(bankIndex + 14);
            return;
        }
    }

    for (int bankIndex = activeBankIndex + 1; bankIndex < 10; ++bankIndex) {
        bank = &playerState->altWeaponBanks[bankIndex];
        if (IsUsableAltWeaponController(saveState, &bank->controllerA) != 0 ||
            IsUsableAltWeaponController(saveState, &bank->controllerB) != 0) {
            HandleAltWeaponBankSelectInput(bankIndex + 14);
            return;
        }
    }
}

// Reimplements 0x439600: Player::ApplyPrimaryWeaponSwitch
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ApplyPrimaryWeaponSwitch(
    zUtil_SaveGameState *saveState, PlayerGunFireController *previousController,
    PlayerGunFireController *newController) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->activePrimaryGunController = newController;
    playerState->primaryHardpointSelectState = 2;

    if (previousController != 0 && previousController->attachNodePrimary != 0) {
        zClass_Class::gwNodeSetActive(previousController->attachNodePrimary, 0);
    }
    if (previousController != 0 && previousController->attachNodeSecondary != 0) {
        zClass_Class::gwNodeSetActive(previousController->attachNodeSecondary, 0);
    }
    if (newController != 0 && newController->attachNodePrimary != 0) {
        zClass_Class::gwNodeSetActive(newController->attachNodePrimary, 1);
    }
    if (newController != 0 && newController->attachNodeSecondary != 0) {
        zClass_Class::gwNodeSetActive(newController->attachNodeSecondary, 1);
    }

    PlayerGunFireController *const activeController = playerState->activePrimaryGunController;
    playerState->cachedPrimarySelectionCode =
        activeController->weaponSideIndex + activeController->weaponBankIndex * 100;
}

// Reimplements 0x439540: Player::ApplyAltWeaponSwitch
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ApplyAltWeaponSwitch(
    zUtil_SaveGameState *saveState, PlayerGunFireController *previousController,
    PlayerGunFireController *newController) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->activeAltGunController = newController;

    const int weaponBankIndex = newController->weaponBankIndex;
    playerState->activeAltBankIndex = weaponBankIndex;
    playerState->altWeaponBanks[weaponBankIndex].selectedSide =
        newController->weaponSideIndex;
    playerState->altHardpointSelectState = 0;
    playerState->altGunTransitionTimerA = 0.0f;
    playerState->altGunTransitionTimerB = 0.0f;

    if (previousController != 0) {
        if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
            saveState->StartMasterTypeLoopSfxHandle(0, 1.0f);
        }

        playerState->altGunTransitionState = 4;
        playerState->altGunTransitionController = previousController;
    } else {
        playerState->altGunTransitionState = 16;
        playerState->altGunTransitionController = newController;
    }

    if (playerState->altGunFireHeldFlag != 0) {
        playerState->altGunFireHeldFlag = 0;
        OptCatalog::DeactivateTrailRuntimeState(previousController->trailRuntimeState);
    }

    PlayerGunFireController *const activeController = playerState->activeAltGunController;
    playerState->cachedAltSelectionCode =
        activeController->weaponSideIndex + activeController->weaponBankIndex * 100;
}

// Reimplements 0x43c800: Player::ResetAltGunDoorAnimationState
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
ResetAltGunDoorAnimationState(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->altGunTransitionTimerB = 0.0f;

    zClass_NodePartial *const doorLeftNode = playerState->doorLeftNode;
    if (doorLeftNode != 0) {
        zClass_Object3D::gwObject3DSetScale(doorLeftNode, 1.0f, 1.0f, 1.0f);
    }

    zClass_NodePartial *const doorRightNode = playerState->doorRightNode;
    if (doorRightNode != 0) {
        zClass_Object3D::gwObject3DSetScale(doorRightNode, 1.0f, 1.0f, 1.0f);
    }
}

void ResetAltGunAttachNode(PlayerGunFireController *controller) {
    zClass_NodePartial *const attachNode = controller->attachNodePrimary;
    if (attachNode == 0) {
        return;
    }

    zClass_Class::gwNodeSetActive(attachNode, 0);
    zClass_Object3D::gwObject3DSetPosition(attachNode, controller->attachPosX,
                                           controller->attachPosY, controller->attachPosZ);
    zClass_Object3D::gwObject3DSetScale(attachNode, 1.0f, 1.0f, 1.0f);
}

// Reimplements 0x43c850: Player::ResetAltGunRuntimeState
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ResetAltGunRuntimeState(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const activeAltGunController =
        playerState->activeAltGunController;

    if (playerState->altGunFireHeldFlag != 0) {
        playerState->altGunFireHeldFlag = 0;
        OptCatalog::DeactivateTrailRuntimeState(activeAltGunController->trailRuntimeState);
    }

    OptCatalogRuntimeInstanceStorage *const attachState =
        (OptCatalogRuntimeInstanceStorage *)(activeAltGunController->attachState);
    if (attachState != 0) {
        zClass_Class::RemoveChild(activeAltGunController->attachNodePrimary,
                                  attachState->projectileNode);
        OptCatalog::RecycleRuntimeInstanceStorage(activeAltGunController->optCatalogEntry,
                                                  attachState);
        activeAltGunController->attachState = 0;
    }

    ResetAltGunDoorAnimationState(saveState);
    playerState->timedHitStatus.ClearLightAndReset();
    playerState->altGunTransitionState = 1;
    playerState->altGunTransitionController = 0;
    playerState->altGunTransitionTimerA = 0.0f;

    PlayerAltWeaponBank *bank = &playerState->altWeaponBanks[2];
    for (int i = 0; i < 8; ++i, ++bank) {
        ResetAltGunAttachNode(&bank->controllerA);
        ResetAltGunAttachNode(&bank->controllerB);
    }
}

// Reimplements 0x43c950: Player::RemoveAllDeployedMines
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL RemoveAllDeployedMines(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zClass_NodePartial *const ownerNode = playerState->rootNode;

    OptCatalogEntryDef *entry = playerState->altWeaponBanks[4].controllerA.optCatalogEntry;
    if (entry != 0) {
        OptCatalog::RemoveRuntimeInstance(entry, 0, ownerNode);
    }

    entry = playerState->altWeaponBanks[4].controllerB.optCatalogEntry;
    if (entry != 0) {
        OptCatalog::RemoveRuntimeInstance(entry, 0, ownerNode);
    }

    entry = playerState->altWeaponBanks[5].controllerA.optCatalogEntry;
    if (entry != 0) {
        OptCatalog::RemoveRuntimeInstance(entry, 0, ownerNode);
    }

    entry = playerState->altWeaponBanks[5].controllerB.optCatalogEntry;
    if (entry != 0) {
        OptCatalog::RemoveRuntimeInstance(entry, 0, ownerNode);
    }
}

// Reimplements 0x439260: Player::HandleAltWeaponBankSelectInput
// (D:\Proj\Battlesport\zWeapon.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL HandleAltWeaponBankSelectInput(int inputCode) {
    zUtil_SaveGameState *const saveState = g_LocalPlayerSaveState;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;
    PlayerGunFireController *const previousController =
        playerState->activeAltGunController;

    if (playerState->altGunTransitionState != 1) {
        return;
    }

    int bankIndex = inputCode - 14;
    if (inputCode < 14 || inputCode > 23) {
        bankIndex = playerState->activeAltBankIndex;
    }

    PlayerAltWeaponBank *const bank = &playerState->altWeaponBanks[bankIndex];
    PlayerGunFireController *newController = 0;
    PlayerGunFireController *failedController = 0;
    int switchAccepted = 0;

    if (bankIndex == playerState->activeAltBankIndex) {
        if (bank->selectedSide == 0) {
            newController = &bank->controllerB;
        } else {
            newController = &bank->controllerA;
        }
        failedController = newController;

        if (newController->optCatalogEntry != 0 && (newController->flags & 4) != 0 &&
            newController->ammoOrCharge != 0.0f) {
            switchAccepted = 1;
        }
    } else {
        const int selectedSide = bank->selectedSide;
        newController = selectedSide == 0 ? &bank->controllerA : &bank->controllerB;
        failedController = newController;

        if (newController->optCatalogEntry != 0 && (newController->flags & 4) != 0 &&
            newController->ammoOrCharge != 0.0f) {
            switchAccepted = 1;
        } else if (newController->optCatalogEntry != 0) {
            bank->selectedSide = selectedSide == 0 ? 1 : 0;
        }
    }

    if (switchAccepted != 0) {
        if (masterModalData->masterType == 2 &&
            (newController->optCatalogEntry->flags & 0x1000) != 0) {
            char message[64];
            OptCatalog::PlayNoAmmoWarning();
            zLoc::FormatMessage(message, sizeof(message), 0x24b,
                                newController->optCatalogEntry->description);
            HudUi::ShowTopMessageLine(message, 5.0f);
            return;
        }

        HudUi::ShowTopMessageLine(newController->optCatalogEntry->description, 5.0f);
        HudUiMessage::UpdateSelectedWeaponDisplay(
            newController->weaponBankIndex, newController->weaponSideIndex,
            newController->ammoOrCharge);
        ApplyAltWeaponSwitch(saveState, previousController, newController);
    } else if (failedController->optCatalogEntry != 0) {
        if ((failedController->flags & 4) == 0) {
            HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x916), 5.0f);
        } else if (failedController->ammoOrCharge == 0.0f) {
            HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x917), 5.0f);
        }
        HudUi::ShowTopMessageLine(failedController->optCatalogEntry->description, 5.0f);
    }

    zUtil_PlayerStateStorage *const displayPlayerState =
        static_cast<zUtil_PlayerStateStorage *>(
            static_cast<void *>(g_GameStateOrMapTable->playerState));
    PlayerGunFireController *const activeController =
        displayPlayerState->activeAltGunController;
    HudUiMessage::UpdateSelectedWeaponDisplay(
        activeController->weaponBankIndex, activeController->weaponSideIndex,
        activeController->ammoOrCharge);
}

// Reimplements 0x439460: Player::HandlePrimaryWeaponVariantToggleInput
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL HandlePrimaryWeaponVariantToggleInput(int keyCode) {
    (void)keyCode;

    zUtil_SaveGameState *const saveState = g_LocalPlayerSaveState;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const previousController =
        playerState->activePrimaryGunController;
    PlayerGunFireController *newController = 0;

    if (previousController->weaponSideIndex != 0) {
        newController = &playerState->altWeaponBanks[1].controllerA;
    } else {
        newController = &playerState->altWeaponBanks[1].controllerB;

        if ((newController->flags & 4) == 0) {
            HudUi::ShowTopMessageLine(newController->optCatalogEntry->description, 5.0f);
            HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x916), 5.0f);
            return;
        }

        if (newController->ammoOrCharge <= 0.0f) {
            HudUi::ShowTopMessageLine(newController->optCatalogEntry->description, 5.0f);
            HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x917), 5.0f);
            return;
        }
    }

    saveState->StartMasterTypeLoopSfxHandle(2, 1.0f);
    ApplyPrimaryWeaponSwitch(saveState, previousController, newController);
    HudUi::ShowTopMessageLine(newController->optCatalogEntry->description, 5.0f);

    zUtil_PlayerStateStorage *const displayPlayerState =
        static_cast<zUtil_PlayerStateStorage *>(
            static_cast<void *>(g_GameStateOrMapTable->playerState));
    PlayerGunFireController *const activeController =
        displayPlayerState->activePrimaryGunController;
    HudUiMessage::UpdateSelectedWeaponDisplay(
        activeController->weaponBankIndex, activeController->weaponSideIndex,
        activeController->ammoOrCharge);
}

// Reimplements 0x439990: Player::ResetDamageStateAndTimedHitStatus
// (D:\Proj\GameZRecoil\zGame\Player\Player_Damage.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
ResetDamageStateAndTimedHitStatus(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zClass_Node::LoadFlagBit8MaterialImagesAndTexturePack(playerState->rootNode);
    playerState->queuedFixedDamageFlag = 0;
    playerState->damageProtectionActive = 0;
    playerState->damageVisualFlag = 0;
    playerState->timedHitStatus.ClearLightAndReset();
}

// Reimplements 0x4399c0: Player::ResetDamageVisualsAndTimedStatus
// (D:\Proj\GameZRecoil\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
ResetDamageVisualsAndTimedStatus(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterCommonData *const masterCommonData = playerState->masterCommonData;

    if (playerState->statusMeterValue <= 0.0f) {
        if (playerState->airborneFlag == 0) {
            EnterLocalInactiveDestroyedLifecycle(saveState);
        }
        return;
    }

    if ((playerState->timedHitStatus.runtimeFlags & kPlayerTimedHitStatusActiveFlag) != 0) {
        const int timedResult =
            playerState->timedHitStatus.TickAndUpdateLight(playerState->rootNode->cachedBounds[0]);
        playerState->damageProtectionActive = timedResult == 2;
    }

    if (playerState->recentHitValid != 0) {
        if (g_Time_AccumulatedTimeSec < playerState->recentHitFxExpireTime) {
            if (playerState->lifecycleState != kPlayerLifecycleRemote) {
                const float damage = playerState->recentHitDamage * g_FrameDeltaTimeSec;
                if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
                    EnterDestroyedState(saveState, 0, 0, damage);
                } else {
                    HitCallback_RecordContextAndTimedStatus(saveState, 0, 0, damage);
                }
            }
        } else {
            zEffectAnim::Stop(playerState->recentHitLightHandle);
            playerState->recentHitLightHandle = 0;
            playerState->recentHitValid = 0;
        }
    }

    if (playerState->queuedFixedDamageFlag != 0) {
        if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
            EnterDestroyedState(saveState, 0, 0, masterCommonData->maxHealth);
        } else {
            HitCallback_RecordContextAndTimedStatus(saveState, 0, 0,
                                                   masterCommonData->maxHealth);
        }
        playerState->queuedFixedDamageFlag = 0;
    }

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable &&
        g_PlayerStatusMeterRatio < 0.25f) {
        HudLowMeterLoopSound::SetLoopActive(0);
        if (g_Time_AccumulatedTimeSec > g_Hud_LowMeterNextBeepTime) {
            g_Hud_LowMeterBeepSample->PlayA3DSimple(1.0f);
            g_Hud_LowMeterNextBeepTime =
                g_Hud_LowMeterBeepInterval + g_Time_AccumulatedTimeSec;
        }
    }
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

// Reimplements 0x401d50: Player::HasLineOfSightFromLocalPlayerFxOffset
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
HasLineOfSightFromLocalPlayerFxOffset(zClass_NodePartial *node, const zVec3 *point,
                                      int directionMode) {
    zUtil_PlayerStateStorage *const playerState =
        (zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState);

    g_Variant_CurrentTag = playerState->variantTag;
    zClass_Class::gwNodeSetRaycastable(node, 0);
    zClass_Class::gwNodeSetRaycastable(playerState->rootNode, 0);
    zClass_cls_di::SetBreakOnFirstCandidate(1);
    zClass_cls_di::SetStopAfterFirstHit(0x40000);

    zVec3 startPoint = {0};
    zVec3 endPoint = {0};
    if (directionMode == 1) {
        startPoint = playerState->fxOffsetWorld;
        endPoint = *point;
    } else {
        startPoint = *point;
        endPoint = playerState->fxOffsetWorld;
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

// Reimplements 0x43b500: Player::ApplyAimPitchToDirection
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ApplyAimPitchToDirection(zVec3 *direction, float pitchY) {
    const float horizontalLenSq = direction->x * direction->x + direction->z * direction->z;
    if (horizontalLenSq == 0.0f) {
        if (pitchY == 0.0f) {
            direction->x = 0.0f;
            direction->y = 0.0f;
            direction->z = -1.0f;
            return;
        }

        const float diagonal = PlayerFastSqrtEstimate((1.0f - pitchY * pitchY) * 0.5f);
        direction->x = diagonal;
        direction->y = pitchY;
        direction->z = diagonal;
        return;
    }

    const float scale = PlayerFastSqrtEstimate((1.0f - pitchY * pitchY) / horizontalLenSq);
    direction->x *= scale;
    direction->y = pitchY;
    direction->z *= scale;
}

// Reimplements 0x405650: Player::UpdateThirdPersonCamera
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateThirdPersonCamera(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zVec3 cameraTarget = {
        playerState->worldPos.x + playerState->cameraLerpStart.x,
        playerState->worldPos.y + playerState->cameraLerpStart.y,
        playerState->worldPos.z + playerState->cameraLerpStart.z,
    };

    zClass_Camera::gwCameraSetTarget(g_MainCamera, cameraTarget.x, cameraTarget.y,
                                     cameraTarget.z);
    if (g_Player_HorizonNode != 0) {
        zClass_Object3D::gwObject3DSetPosition(g_Player_HorizonNode, cameraTarget.x,
                                               cameraTarget.y, cameraTarget.z);
    }

    zVec3 cameraLookAt = playerState->worldPos;
    cameraLookAt.y += playerState->cameraYOffset;

    zVec3 cameraAngles = {0};
    zMath::Vec3DirectionAnglesBetweenPoints(&cameraTarget, &cameraLookAt, &cameraAngles);
    zClass_Camera::gwCameraSetPosition(g_MainCamera, cameraAngles.x, cameraAngles.y,
                                       cameraAngles.z);

    zVec3 dir = {
        playerState->autoTurnTargetWorldPos.x - cameraTarget.x,
        playerState->autoTurnTargetWorldPos.y - cameraTarget.y,
        playerState->autoTurnTargetWorldPos.z - cameraTarget.z,
    };
    const float invLength =
        1.0f / static_cast<float>(sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z));
    playerState->cameraDirNext.x = dir.x * invLength;
    playerState->cameraDirNext.y = dir.y * invLength;
    playerState->cameraDirNext.z = dir.z * invLength;

    playerState->cameraTarget = cameraTarget;
    playerState->cameraDir = playerState->cameraDirNext;
    playerState->cameraDirFlat = playerState->cameraDirNext;
    playerState->cameraDirFlat.y = 0.0f;
    zMath::Vec3NormalizeXZ(&playerState->cameraDirFlat, &playerState->cameraDirFlat);
}

// Reimplements 0x405c90: Player::ApplyCameraState
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ApplyCameraState(int newState) {
    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(g_GameStateOrMapTable);
    if (saveState == 0) {
        return;
    }

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    int currentState = playerState->cameraState;
    if (newState == currentState ||
        (currentState == kPlayerCameraStateClearScreen &&
         newState == kPlayerCameraStateProjectileAttached)) {
        return;
    }

    if (currentState == kPlayerCameraStateProjectileAttached &&
        newState != kPlayerCameraStateRestorePrevious) {
        ApplyCameraState(kPlayerCameraStateRestorePrevious);
        currentState = playerState->cameraState;
    }

    if (newState == kPlayerCameraStateToggleRequest) {
        switch (currentState) {
        case kPlayerCameraStateThirdPerson:
            newState = kPlayerCameraStateFirstPerson;
            break;
        case kPlayerCameraStateClearScreen:
            newState = kPlayerCameraStateRestorePrevious;
            break;
        case kPlayerCameraStateFirstPerson:
            newState = kPlayerCameraStateThirdPerson;
            break;
        default:
            break;
        }
    }

    int resolvedState = newState;
    switch (newState) {
    case kPlayerCameraStateThirdPerson:
        playerState->cameraLerpActive = 0;
        playerState->cameraTargetDistance = playerState->cameraDistance;
        if (currentState == kPlayerCameraStateFirstPerson) {
            zOpt::SetSteeringMode(g_Player_SavedSteeringMode);
        }
        break;

    case kPlayerCameraStateClearScreen:
        zVideo::ExchangeClearScreenBufferEnabled(1);
        break;

    case kPlayerCameraStateFirstPerson:
        g_Player_SavedSteeringMode = zOpt::GetSteeringMode();
        zOpt::SetSteeringMode(0);
        playerState->cameraElevationOffset = 0.0f;
        break;

    case kPlayerCameraStateProjectileAttached: {
        OptCatalogRuntimeInstanceStorage *const attachState =
            (OptCatalogRuntimeInstanceStorage *)(playerState->activeAltGunController->attachState);
        zClass_NodePartial *const projectileNode = attachState->projectileNode;
        zClass_Camera::gwCameraSetTarget(g_MainCamera, 0.0f, 1.0f, 1.0f);
        zClass_Camera::gwCameraSetPosition(g_MainCamera, 0.0f, 0.0f, 0.0f);
        zClass_Class::AddChild(projectileNode, g_MainCamera);
        zClass_Object3D::gwObject3DSetAlphaScale(projectileNode, 0.5f);
        zClass_Object3D::gwObject3DSetLitFlag(projectileNode, 1);
        SetState7FxPass3Visible(1);
        break;
    }

    case kPlayerCameraStateRestorePrevious:
        resolvedState = playerState->previousCameraState;
        if (currentState == kPlayerCameraStateProjectileAttached) {
            OptCatalogRuntimeInstanceStorage *const attachState =
                (OptCatalogRuntimeInstanceStorage *)(playerState->activeAltGunController->attachState);
            zClass_NodePartial *const projectileNode = attachState->projectileNode;
            zClass_Class::RemoveChild(projectileNode, g_MainCamera);
            zClass_Object3D::gwObject3DSetAlphaScale(projectileNode, 1.0f);
            zClass_Object3D::gwObject3DSetLitFlag(projectileNode, 0);
            UpdateThirdPersonCamera(saveState);
            SetState7FxPass3Visible(0);
            zTag4::Clear(&g_VariantTag_Current);
            g_Variant_CurrentTag = g_VariantTag_Current;
        } else if (currentState == kPlayerCameraStateClearScreen) {
            zVideo::ExchangeClearScreenBufferEnabled(0);
            UpdateThirdPersonCamera(saveState);
        }
        break;

    default:
        break;
    }

    playerState->previousCameraState = currentState;
    playerState->cameraState = resolvedState;

    if (resolvedState == kPlayerCameraStateThirdPerson) {
        zOpt::SetCameraMode(1);
    } else if (resolvedState == kPlayerCameraStateFirstPerson) {
        zOpt::SetCameraMode(0);
    }
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

// Reimplements 0x42cf90: Player::BuildEnvironmentProbeResult
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL BuildEnvironmentProbeResult(
    zUtil_SaveGameState *saveState, PlayerEnvProbeResult *outProbe) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    PlayerMasterModalData *const masterModalData = primaryModalState->masterModalData;

    const int modalPointCount = primaryModalState->modalStateCode;
    for (int i = 0; i < modalPointCount; ++i) {
        const zVec3 transformed = TransformPointByMatrix(
            masterModalData->probePoints[kPlayerEnvProbeBasePointOffset + i],
            playerState->motionBasis);
        primaryModalState->transformedProbePointWorldByIndex[i] = transformed;
        g_PlayerEnvProbeWorldPoints[i] = transformed;
    }

    if (g_PlayerEnvProbeSampleCount > 4) {
        zMath::Vec3Midpoint(&g_PlayerEnvProbeWorldPoints[0], &g_PlayerEnvProbeWorldPoints[3],
                            &g_PlayerEnvProbeWorldPoints[4]);
        zMath::Vec3Midpoint(&g_PlayerEnvProbeWorldPoints[1], &g_PlayerEnvProbeWorldPoints[2],
                            &g_PlayerEnvProbeWorldPoints[5]);
    }

    if (g_PlayerEnvProbeSampleCount > 6) {
        g_PlayerEnvProbeWorldPoints[6] = playerState->worldPos;
    }

    zUtil_PlayerStateStorage *const globalPlayerState =
        static_cast<zUtil_PlayerStateStorage *>(static_cast<void *>(g_GameStateOrMapTable->playerState));
    zClass_Class::gwNodeSetCellPickable(playerState->rootNode, 0);
    zClass_Class::gwNodeSetCellPickable(globalPlayerState->rootNode, 0);

    g_Variant_CurrentTag = playerState->variantTag;
    zClass_cls_di::BuildPickCandidatesForPointBatch(
        g_Player_RuntimeDiScene, g_PlayerEnvProbeWorldPoints, g_PlayerEnvProbeSampleCount, 500.0f,
        outProbe->candidateBuffers);
    g_Variant_CurrentTag = g_VariantTag_Current;

    outProbe->highestSelectedHitY = -300.0f;
    outProbe->attachmentCandidateCount = 0;

    float maxRiseWindow = -(playerState->projectileSpawnVel.y * g_Player_DeltaTime);
    if (outProbe->minProbeDepth > maxRiseWindow) {
        maxRiseWindow = outProbe->minProbeDepth;
    }

    for (int i = 0; i < g_PlayerEnvProbeSampleCount; ++i) {
        int bestCandidateIndex = 0;
        int selectedImpactSlot = 0;
        float taggedHeight = -300.0f;
        PlayerProbeSampleCandidateBuffer *const candidateBuffer = &outProbe->candidateBuffers[i];

        outProbe->candidateScoreBySample[i] = SelectProbeSampleHeightFromCandidates(
            candidateBuffer, &bestCandidateIndex, g_PlayerEnvProbeWorldPoints[i].y, maxRiseWindow,
            outProbe->preferAttachmentSlot1, &selectedImpactSlot, &taggedHeight);
        outProbe->bestIndexBySample[i] = bestCandidateIndex;
        outProbe->impactSlotBySample[i] = selectedImpactSlot;

        if (outProbe->highestSelectedHitY < taggedHeight) {
            outProbe->highestSelectedHitY = taggedHeight;
        }

        if (i < 4 && i == 0) {
            if (outProbe->candidateBuffers[0].candidateCount <= 0) {
                zClass_Class::gwNodeSetNodeType(playerState->rootNode, 0xff);
            } else {
                const zClassDiPickCandidateEntry *const selectedCandidate =
                    &outProbe->candidateBuffers[0].entries[outProbe->bestIndexBySample[0]];
                playerState->selectedProbeSample = *selectedCandidate;
                playerState->selectedProbeSample.hitPos.x =
                    primaryModalState->transformedProbePointWorldByIndex[0].x;
                playerState->selectedProbeSample.hitPos.z =
                    primaryModalState->transformedProbePointWorldByIndex[0].z;
                playerState->variantTag = selectedCandidate->variantTag;

                zClass_NodePartial *const worldChild =
                    zClass_Class::gwNodeGetWorldChild(selectedCandidate->node);
                const int nodeType =
                    worldChild != 0 ? worldChild->nodeType : selectedCandidate->variantTag.tags[0];
                zClass_Class::gwNodeSetNodeType(playerState->rootNode, nodeType);
            }
        }

        outProbe->hitHistogram.countByImpactSlot[selectedImpactSlot] += 1;

        if (candidateBuffer->candidateCount != 0) {
            zClass_NodePartial *const candidateNode =
                candidateBuffer->entries[bestCandidateIndex].node;
            if (candidateNode != 0 && candidateNode->auxFlags != 0) {
                outProbe->attachmentCandidateCount += 1;
                outProbe->attachmentNode =
                    static_cast<zClass_NodePartial *>(candidateNode->callbackContext);
            }
        }

        if (i >= 4 && (g_PlayerEnvProbeSampleMaskTable[i] & 0x0a) == 0) {
            outProbe->candidateScoreBySample[i] -= 0.2f;
        }
    }

    playerState->probeImpactSlot1SeenFlag = outProbe->hitHistogram.countByImpactSlot[1];
}

// Reimplements 0x42d5c0: Player::ApplyEnvironmentProbeResult
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
ApplyEnvironmentProbeResult(zUtil_SaveGameState *saveState, PlayerEnvProbeResult *envProbe) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    const int wasAttached = playerState->environmentAttachmentActive;

    if (envProbe->attachmentCandidateCount > 3) {
        if (wasAttached == 0) {
            playerState->environmentAttachmentActive = 1;
            playerState->environmentAttachmentNode = envProbe->attachmentNode;
            CopyNodeCachedWorldMatrix(&playerState->environmentAttachmentMatrix,
                                      envProbe->attachmentNode);
            playerState->yawPoseCache =
                playerState->restartYawRad -
                ExtractYawFromMatrix(&playerState->environmentAttachmentMatrix);
            CacheAttachmentLocalOffset(playerState);
        }
    } else if (wasAttached != 0) {
        CopyNodeCachedWorldMatrix(&playerState->environmentAttachmentMatrix,
                                  playerState->environmentAttachmentNode);
        playerState->restartYawRad =
            ExtractYawFromMatrix(&playerState->environmentAttachmentMatrix) +
            playerState->yawPoseCache;
        playerState->pitchPoseCache = playerState->vehiclePitchRad;
        playerState->yawPoseCache = playerState->restartYawRad;
        playerState->rollPoseCache = playerState->vehicleRollRad;
        playerState->environmentAttachmentActive = 0;
        playerState->environmentAttachmentNode = 0;
    }

    if (playerState->amphibUnlocked != 0 &&
        envProbe->hitHistogram.countByImpactSlot[1] > 1 &&
        TransitionToMasterTypeAmphib(saveState, 0, 0) != 0) {
        playerState->currentMasterType = masterModalData->masterType;
        if (playerState->projectileSpawnVel.y < -10.0f) {
            zEffectAnim::SetTransformRotAndVelocity_Thunk(
                g_Player_BftSplashAnimEntry, 0, playerState->worldPos.x,
                envProbe->highestSelectedHitY, playerState->worldPos.z, 0.0f,
                playerState->restartYawRad, 0.0f, 0.0f, 0.0f, 0.0f);
            playerState->projectileSpawnVel.z = 0.0f;
            playerState->projectileSpawnVel.x = 0.0f;
            return 0;
        }
    }

    playerState->gravityAccel = g_Player_NominalGravity;
    zUtil_SaveGameState *const originalSaveState = saveState;
    const int waterHitCount = envProbe->hitHistogram.countByImpactSlot[1];
    if (envProbe->highestSelectedHitY - playerState->worldPos.y > 1.0f &&
        waterHitCount > 1) {
        const int wasUnderwater = playerState->underwaterStatusActive;
        playerState->gravityAccel = g_Player_WaterGravity;
        if (wasUnderwater == 0) {
            playerState->underwaterStatusActive = 1;
            if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
                HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x909), 5.0f);
                HudLowMeterLoopSound::SetLoopActive(1);
            }
        }

        const float damage = g_Player_DeltaTime * 8.0f;
        if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
            EnterDestroyedState(saveState, 0, 0, damage);
            if (playerState->cameraTarget.y < envProbe->highestSelectedHitY) {
                SetHudUiElementVisible(&g_Player_UnderwaterFxPass3Ui, 1);
            } else {
                SetHudUiElementVisible(&g_Player_UnderwaterFxPass3Ui, 0);
            }
        } else {
            HitCallback_RecordContextAndTimedStatus(saveState, 0, 0, damage);
        }
    } else {
        if (playerState->underwaterStatusActive != 0) {
            playerState->underwaterStatusActive = 0;
            if (originalSaveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
                SetHudUiElementVisible(&g_Player_UnderwaterFxPass3Ui, 0);
                HudLowMeterLoopSound::SetLoopActive(0);
            }
        }
    }

    if (envProbe->hitHistogram.countByImpactSlot[3] > 1) {
        playerState->axisClampRuntime =
            masterModalData->maxSpeed * masterModalData->quicksandSlowdown;
        playerState->yawVelocityLimit =
            masterModalData->yawRateMax * masterModalData->quicksandSlowdown;
        playerState->gravityAccel = g_Player_QuicksandGravity;
        return 1;
    }

    if (envProbe->hitHistogram.countByImpactSlot[4] > 1) {
        if (playerState->hoverUnlocked != 0 &&
            TransitionToMasterTypeHover(saveState, 0) != 0) {
            return 0;
        }

        if (playerState->motionInput == 0) {
            playerState->motionInput = 1;
        }
        if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
            HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x910), 5.0f);
            HudLowMeterLoopSound::SetLoopActive(1);
        }

        playerState->axisClampRuntime =
            masterModalData->maxSpeed * masterModalData->lavaSlowdown;
        playerState->yawVelocityLimit =
            masterModalData->yawRateMax * masterModalData->lavaSlowdown;
        const float damage =
            static_cast<float>(envProbe->hitHistogram.countByImpactSlot[4]) *
            g_Player_DeltaTime * 12.0f;
        if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
            EnterDestroyedState(saveState, g_Player_MakeHotOptEntry, 0, damage);
        } else {
            HitCallback_RecordContextAndTimedStatus(saveState, g_Player_MakeHotOptEntry, 0,
                                                    damage);
        }
        return 1;
    }

    if (playerState->motionInput != 0) {
        playerState->motionInput = 0;
        if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
            HudLowMeterLoopSound::SetLoopActive(0);
        }
    }

    playerState->axisClampRuntime = masterModalData->maxSpeed;
    playerState->yawVelocityLimit = masterModalData->yawRateMax;
    return 1;
}

// Reimplements 0x42cde0: Player::SolveHeightOnSurface
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE float RECOIL_FASTCALL
SolveHeightOnSurface(zUtil_SaveGameState *saveState, float supportPlaneDot) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    float steerBasisRefY = playerState->steerBasisRef.y;
    if (steerBasisRefY == 0.0f) {
        steerBasisRefY = 0.0000999999975f;
    }

    return (supportPlaneDot - playerState->worldPos.x * playerState->steerBasisRef.x -
            playerState->worldPos.z * playerState->steerBasisRef.z) /
           steerBasisRefY;
}

// Reimplements 0x42cb50: Player::ResetTerrainContactImpulsesAndPlayImpactSfx
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
ResetTerrainContactImpulsesAndPlayImpactSfx(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->angVelRoll = 0.0f;
    playerState->angVelPitch = 0.0f;

    if (saveState != (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        return;
    }

    float sfxVolume = static_cast<float>(fabs(playerState->projectileSpawnVel.y * 0.100000001f));
    if (sfxVolume > 1.0f) {
        sfxVolume = 1.0f;
    } else if (sfxVolume < 0.0f) {
        sfxVolume = 0.0f;
    }
    saveState->StartModalLoopSfxHandle(5, sfxVolume);
}

// Reimplements 0x42c8d0: Player::ApplyTerrainTilt
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
ApplyTerrainTilt(zUtil_SaveGameState *saveState, const zVec3 *tiltVector, float tiltScale) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    const float tiltFactor = (g_Player_NominalGravity / playerState->gravityAccel) * tiltScale;
    zVec3 rotatedTilt = {
        tiltVector->x * tiltFactor,
        tiltVector->y * tiltFactor,
        tiltVector->z * tiltFactor,
    };
    zMath::Vec3RotateY(&rotatedTilt, &rotatedTilt, -playerState->restartYawRad);

    if (playerState->airborneFlag != 0) {
        ResetTerrainContactImpulsesAndPlayImpactSfx(saveState);
    }

    playerState->angVelPitch += rotatedTilt.x;
    playerState->angVelRoll += rotatedTilt.z;
    if (playerState->angVelPitch > 1.20000005f) {
        playerState->angVelPitch = 1.20000005f;
    } else if (playerState->angVelPitch < -1.20000005f) {
        playerState->angVelPitch = -1.20000005f;
    }
    if (playerState->angVelRoll > 1.20000005f) {
        playerState->angVelRoll = 1.20000005f;
    } else if (playerState->angVelRoll < -1.20000005f) {
        playerState->angVelRoll = -1.20000005f;
    }

    const float velocityScale = g_Player_DeltaTime * playerState->gravityAccel * 5.0f;
    zVec3 impulse = {
        playerState->steerBasisRef.x * velocityScale,
        0.0f,
        playerState->steerBasisRef.z * velocityScale,
    };
    playerState->projectileSpawnVel.x += impulse.x;
    playerState->projectileSpawnVel.y += impulse.y;
    playerState->projectileSpawnVel.z += impulse.z;
}

// Reimplements 0x42ce50: Player::ComputeTriangleNormal
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
ComputeTriangleNormal(zUtil_SaveGameState *saveState, const zVec3 *pointA, const zVec3 *pointB,
                      const zVec3 *pointC) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    const zVec3 edgeAB = {
        pointB->x - pointA->x,
        pointB->y - pointA->y,
        pointB->z - pointA->z,
    };
    const zVec3 edgeAC = {
        pointC->x - pointA->x,
        pointC->y - pointA->y,
        pointC->z - pointA->z,
    };
    zVec3 normal = {
        edgeAB.y * edgeAC.z - edgeAB.z * edgeAC.y,
        edgeAB.z * edgeAC.x - edgeAB.x * edgeAC.z,
        edgeAB.x * edgeAC.y - edgeAB.y * edgeAC.x,
    };
    zMath::Vec3Normalize(&normal);
    if (normal.y <= 0.0f) {
        normal.x = -normal.x;
        normal.y = -normal.y;
        normal.z = -normal.z;
    }
    playerState->steerBasisRef = normal;
}

// Reimplements 0x42c520: Player::ComputeSurfaceFrom1Probe
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
ComputeSurfaceFrom1Probe(zUtil_SaveGameState *saveState, PlayerEnvProbeResult *probeResult) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    const int sampleIndex = g_PlayerEnvProbe_AboveGroundIndices[0];
    zVec3 samplePoint = g_PlayerEnvProbeWorldPoints[sampleIndex];
    samplePoint.y = probeResult->candidateScoreBySample[sampleIndex];

    const float supportPlaneDot =
        playerState->steerBasisRef.x * samplePoint.x +
        playerState->steerBasisRef.y * samplePoint.y +
        playerState->steerBasisRef.z * samplePoint.z;
    playerState->worldPos.y = SolveHeightOnSurface(saveState, supportPlaneDot);

    const zVec3 sampleOffsetFromPlayer = {
        samplePoint.x - playerState->worldPos.x,
        samplePoint.y - playerState->worldPos.y,
        samplePoint.z - playerState->worldPos.z,
    };
    const zVec3 tiltVector = {
        sampleOffsetFromPlayer.y * playerState->steerBasisRef.z -
            sampleOffsetFromPlayer.z * playerState->steerBasisRef.y,
        sampleOffsetFromPlayer.z * playerState->steerBasisRef.x -
            sampleOffsetFromPlayer.x * playerState->steerBasisRef.z,
        sampleOffsetFromPlayer.x * playerState->steerBasisRef.y -
            sampleOffsetFromPlayer.y * playerState->steerBasisRef.x,
    };
    ApplyTerrainTilt(saveState, &tiltVector, 1.0f);
}

// Reimplements 0x42c640: Player::ComputeSurfaceFrom2Probes
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
ComputeSurfaceFrom2Probes(zUtil_SaveGameState *saveState, PlayerEnvProbeResult *probeResult) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    const int sampleIndexB = g_PlayerEnvProbe_AboveGroundIndices[1];
    const int sampleIndexA = g_PlayerEnvProbe_AboveGroundIndices[0];

    zVec3 pointA = g_PlayerEnvProbeWorldPoints[sampleIndexA];
    pointA.y = probeResult->candidateScoreBySample[sampleIndexA];

    zVec3 pointB = g_PlayerEnvProbeWorldPoints[sampleIndexB];
    const float pointASupportDot =
        playerState->steerBasisRef.x * pointA.x +
        playerState->steerBasisRef.y * pointA.y +
        playerState->steerBasisRef.z * pointA.z;
    pointB.y = SolveHeightOnSurface(saveState, pointASupportDot);

    zVec3 supportEdge = {
        pointB.x - pointA.x,
        pointB.y - pointA.y,
        pointB.z - pointA.z,
    };
    const zVec3 perpOffset = {
        playerState->steerBasisRef.y * supportEdge.z -
            playerState->steerBasisRef.z * supportEdge.y,
        playerState->steerBasisRef.z * supportEdge.x -
            playerState->steerBasisRef.x * supportEdge.z,
        playerState->steerBasisRef.x * supportEdge.y -
            playerState->steerBasisRef.y * supportEdge.x,
    };
    const zVec3 pointC = {
        pointA.x + perpOffset.x,
        pointA.y + perpOffset.y,
        pointA.z + perpOffset.z,
    };

    pointB.y = probeResult->candidateScoreBySample[sampleIndexB];
    ComputeTriangleNormal(saveState, &pointA, &pointB, &pointC);

    const float surfaceDot =
        playerState->steerBasisRef.x * pointA.x +
        playerState->steerBasisRef.y * pointA.y +
        playerState->steerBasisRef.z * pointA.z;
    playerState->worldPos.y = SolveHeightOnSurface(saveState, surfaceDot);

    zMath::Vec3Normalize(&supportEdge);
    const zVec3 pointOffsetFromPlayer = {
        pointA.x - playerState->worldPos.x,
        pointA.y - playerState->worldPos.y,
        pointA.z - playerState->worldPos.z,
    };
    const zVec3 tiltPerp = {
        playerState->steerBasisRef.y * supportEdge.z -
            playerState->steerBasisRef.z * supportEdge.y,
        playerState->steerBasisRef.z * supportEdge.x -
            playerState->steerBasisRef.x * supportEdge.z,
        playerState->steerBasisRef.x * supportEdge.y -
            playerState->steerBasisRef.y * supportEdge.x,
    };
    const float tiltScale =
        pointOffsetFromPlayer.x * tiltPerp.x +
        pointOffsetFromPlayer.y * tiltPerp.y +
        pointOffsetFromPlayer.z * tiltPerp.z;
    ApplyTerrainTilt(saveState, &supportEdge, tiltScale);
}

// Reimplements 0x42cbd0: Player::CheckProbeSampleMaskOverlap
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
CheckProbeSampleMaskOverlap(int sampleIndexA, int sampleIndexB, int sampleIndexC) {
    return g_PlayerEnvProbeSampleMaskTable[sampleIndexC] &
           g_PlayerEnvProbeSampleMaskTable[sampleIndexB] &
           g_PlayerEnvProbeSampleMaskTable[sampleIndexA];
}

// Reimplements 0x42cf60: Player::RebuildAboveGroundIndices
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL RebuildAboveGroundIndices() {
    int *aboveGroundIndexCursor = g_PlayerEnvProbe_AboveGroundIndices;
    for (int sampleIndex = 0; sampleIndex < g_PlayerEnvProbeSampleCount; ++sampleIndex) {
        if (g_PlayerEnvProbe_AboveGroundFlags[sampleIndex] != 0) {
            *aboveGroundIndexCursor = sampleIndex;
            ++aboveGroundIndexCursor;
        }
    }
}

// Reimplements 0x42cc00: Player::SelectBestProbesByDotProduct
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
SelectBestProbesByDotProduct(const zVec3 *referenceNormal, PlayerEnvProbeResult *probeResult) {
    int sampleIndexA = -1;
    int sampleIndexB = -1;
    int sampleIndexC = -1;
    int sampleIndexD = -1;
    float scoreA = -100000000.0f;
    float scoreB = -100000000.0f;
    float scoreC = -100000000.0f;
    float scoreD = -100000000.0f;

    for (int sampleIndex = 0; sampleIndex < g_PlayerEnvProbeSampleCount; ++sampleIndex) {
        if (g_PlayerEnvProbe_AboveGroundFlags[sampleIndex] == 0) {
            continue;
        }

        zVec3 candidatePoint = g_PlayerEnvProbeWorldPoints[sampleIndex];
        candidatePoint.y = probeResult->candidateScoreBySample[sampleIndex];
        const float score =
            referenceNormal->x * candidatePoint.x +
            referenceNormal->y * candidatePoint.y +
            referenceNormal->z * candidatePoint.z;

        if (score > scoreA) {
            if (sampleIndexD > -1) {
                g_PlayerEnvProbe_AboveGroundFlags[sampleIndexD] = 0;
            }
            scoreD = scoreC;
            sampleIndexD = sampleIndexC;
            sampleIndexC = sampleIndexB;
            scoreC = scoreB;
            scoreB = scoreA;
            sampleIndexB = sampleIndexA;
            scoreA = score;
            sampleIndexA = sampleIndex;
        } else if (score > scoreB) {
            if (sampleIndexD > -1) {
                g_PlayerEnvProbe_AboveGroundFlags[sampleIndexD] = 0;
            }
            sampleIndexD = sampleIndexC;
            sampleIndexC = sampleIndexB;
            scoreD = scoreC;
            scoreC = scoreB;
            scoreB = score;
            sampleIndexB = sampleIndex;
        } else if (score > scoreC) {
            if (sampleIndexD > -1) {
                g_PlayerEnvProbe_AboveGroundFlags[sampleIndexD] = 0;
            }
            sampleIndexD = sampleIndexC;
            scoreD = scoreC;
            scoreC = score;
            sampleIndexC = sampleIndex;
        } else if (score > scoreD) {
            if (sampleIndexD > -1) {
                g_PlayerEnvProbe_AboveGroundFlags[sampleIndexD] = 0;
            }
            scoreD = score;
            sampleIndexD = sampleIndex;
        } else {
            g_PlayerEnvProbe_AboveGroundFlags[sampleIndex] = 0;
        }
    }

    if (CheckProbeSampleMaskOverlap(sampleIndexA, sampleIndexB, sampleIndexC) == 0) {
        g_PlayerEnvProbe_AboveGroundFlags[sampleIndexD] = 0;
    } else {
        g_PlayerEnvProbe_AboveGroundFlags[sampleIndexC] = 0;
    }
    RebuildAboveGroundIndices();
}

// Reimplements 0x42ca40: Player::ComputeSurfaceFrom3Probes
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
ComputeSurfaceFrom3Probes(zUtil_SaveGameState *saveState, PlayerEnvProbeResult *probeResult) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (playerState->airborneFlag != 0) {
        ResetTerrainContactImpulsesAndPlayImpactSfx(saveState);
    }

    const int sampleIndexA = g_PlayerEnvProbe_AboveGroundIndices[0];
    const int sampleIndexB = g_PlayerEnvProbe_AboveGroundIndices[1];
    const int sampleIndexC = g_PlayerEnvProbe_AboveGroundIndices[2];
    zVec3 pointA = g_PlayerEnvProbeWorldPoints[sampleIndexA];
    zVec3 pointB = g_PlayerEnvProbeWorldPoints[sampleIndexB];
    zVec3 pointC = g_PlayerEnvProbeWorldPoints[sampleIndexC];
    pointA.y = probeResult->candidateScoreBySample[sampleIndexA];
    pointB.y = probeResult->candidateScoreBySample[sampleIndexB];
    pointC.y = probeResult->candidateScoreBySample[sampleIndexC];

    ComputeTriangleNormal(saveState, &pointA, &pointB, &pointC);
    const float surfaceDot =
        playerState->steerBasisRef.x * pointA.x +
        playerState->steerBasisRef.y * pointA.y +
        playerState->steerBasisRef.z * pointA.z;
    playerState->worldPos.y = SolveHeightOnSurface(saveState, surfaceDot);
    playerState->angVelPitch = 0.0f;
    playerState->angVelRoll = 0.0f;
}

// Reimplements 0x42bf90: Player::UpdatePostMoveEnvironment
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdatePostMoveEnvironment(zUtil_SaveGameState *saveState, int probeSampleCount) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    g_PlayerEnvProbeSampleCount = probeSampleCount;

    PlayerEnvProbeResult probeResult;
    memset(&probeResult, 0, sizeof(probeResult));
    probeResult.minProbeDepth = 4.0f;
    probeResult.preferAttachmentSlot1 = playerState->amphibUnlocked == 0 ? 1 : 0;

    const float restartYawRad = playerState->restartYawRad;
    playerState->vehiclePitchRad += playerState->angVelPitch * g_Player_DeltaTime;
    playerState->vehicleRollRad += playerState->angVelRoll * g_Player_DeltaTime;
    zMath::MatBuildEulerRotation3x3(&playerState->motionBasis, playerState->vehiclePitchRad,
                                    restartYawRad, playerState->vehicleRollRad);
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posZ = playerState->worldPos.z;

    RebuildSteerBasisFromMotionBasis(saveState);

    const float verticalVelocityAfterGravity =
        playerState->projectileSpawnVel.y - playerState->gravityAccel * g_Player_DeltaTime;
    playerState->projectileSpawnVel.y = verticalVelocityAfterGravity;
    const float advancedWorldPosY =
        playerState->worldPos.y + verticalVelocityAfterGravity * g_Player_DeltaTime;
    playerState->worldPos.y = advancedWorldPosY;
    playerState->motionBasis.posY = advancedWorldPosY;

    BuildEnvironmentProbeResult(saveState, &probeResult);
    if (ApplyEnvironmentProbeResult(saveState, &probeResult) == 0) {
        return;
    }

    ProcessEnvProbeResults(saveState, &probeResult);
    RebuildOrientationFromNormal(saveState);
    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable &&
        playerState->airborneFlag == 0) {
        FindThirdProbeAndComputeNormal(saveState, &probeResult);
    }
    UpdateVerticalVelocityAndTransform(saveState, &probeResult);
}

// Reimplements 0x42c0d0: Player::ProcessEnvProbeResults
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
ProcessEnvProbeResults(zUtil_SaveGameState *saveState, PlayerEnvProbeResult *probeResult) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    const float supportDepthThreshold = g_Player_DeltaTime * 5.0f;
    g_PlayerEnvProbe_AboveGroundCount = 0;

    for (int sampleIndex = 0; sampleIndex < g_PlayerEnvProbeSampleCount; ++sampleIndex) {
        const int impactSlot = probeResult->impactSlotBySample[sampleIndex];
        if (impactSlot == 3) {
            probeResult->candidateScoreBySample[sampleIndex] -= g_Player_QuicksandSinkRate;
        }
        if (impactSlot == 4) {
            probeResult->candidateScoreBySample[sampleIndex] -= g_Player_LavaSinkRate;
        }

        if (g_PlayerEnvProbeWorldPoints[sampleIndex].y - supportDepthThreshold >
            probeResult->candidateScoreBySample[sampleIndex]) {
            g_PlayerEnvProbe_AboveGroundFlags[sampleIndex] = 1;
            g_PlayerEnvProbe_AboveGroundIndices[g_PlayerEnvProbe_AboveGroundCount] =
                sampleIndex;
            ++g_PlayerEnvProbe_AboveGroundCount;
        } else {
            g_PlayerEnvProbe_AboveGroundFlags[sampleIndex] = 0;
        }
    }

    const int aboveGroundSampleCount = g_PlayerEnvProbe_AboveGroundCount;
    if (aboveGroundSampleCount == 0) {
        playerState->airborneFlag = 1;
        const float unclampedPitchRecoveryVel =
            (playerState->vehiclePitchRad - -0.523599982f) * -0.699999988f;
        const float targetPitchRecoveryVel =
            unclampedPitchRecoveryVel <= -0.699999988f ? -0.699999988f
                                                       : unclampedPitchRecoveryVel;
        const float targetRollRecoveryVel = playerState->vehicleRollRad * -0.699999988f;
        const float previousAngularVelocityBlendWeight =
            PlayerFloatFromBits(static_cast<int>(-saveState->primaryModalState->masterModalData
                                                      ->aDamping *
                                                  g_Player_DeltaTime * 12102200.0f) +
                                0x3f800000);
        const float newAngularVelocityBlendWeight = 1.0f - previousAngularVelocityBlendWeight;
        playerState->angVelPitch =
            previousAngularVelocityBlendWeight * playerState->angVelPitch +
            newAngularVelocityBlendWeight * targetPitchRecoveryVel;
        playerState->angVelRoll =
            previousAngularVelocityBlendWeight * playerState->angVelRoll +
            newAngularVelocityBlendWeight * targetRollRecoveryVel;
        return;
    }

    if (aboveGroundSampleCount == 1) {
        ComputeSurfaceFrom1Probe(saveState, probeResult);
        playerState->airborneFlag = 0;
        return;
    }

    if (aboveGroundSampleCount == 2) {
        ComputeSurfaceFrom2Probes(saveState, probeResult);
        playerState->airborneFlag = 0;
        return;
    }

    if (aboveGroundSampleCount != 3) {
        SelectBestProbesByDotProduct(&playerState->steerBasisRef, probeResult);
    }

    if (CheckProbeSampleMaskOverlap(g_PlayerEnvProbe_AboveGroundIndices[0],
                                    g_PlayerEnvProbe_AboveGroundIndices[1],
                                    g_PlayerEnvProbe_AboveGroundIndices[2]) != 0) {
        ComputeSurfaceFrom2Probes(saveState, probeResult);
    } else {
        ComputeSurfaceFrom3Probes(saveState, probeResult);
    }
    playerState->airborneFlag = 0;
}

// Reimplements 0x42da40: Player::RebuildOrientationFromNormal
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
RebuildOrientationFromNormal(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (playerState->steerBasisRef.y == 0.0f) {
        playerState->steerBasisRef.y = 0.00100000005f;
    }

    zVec3 rawBasis = playerState->steerBasisNorm;
    rawBasis.y = -((rawBasis.x * playerState->steerBasisRef.x +
                    playerState->steerBasisRef.z * rawBasis.z) /
                   playerState->steerBasisRef.y);
    zMath::Vec3Normalize(&rawBasis);
    playerState->steerBasisRaw = rawBasis;

    zVec3 yawRelativeNormal = {};
    zMath::Vec3RotateY(&yawRelativeNormal, &playerState->steerBasisRef,
                       -playerState->restartYawRad);
    playerState->vehiclePitchRad = static_cast<float>(asin(yawRelativeNormal.z));
    playerState->vehicleRollRad = static_cast<float>(asin(-yawRelativeNormal.x));
    zMath::MatBuildEulerRotation3x3(&playerState->motionBasis, playerState->vehiclePitchRad,
                                    playerState->restartYawRad, playerState->vehicleRollRad);
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;
}

// Reimplements 0x42d320: Player::FindThirdProbeAndComputeNormal
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
FindThirdProbeAndComputeNormal(zUtil_SaveGameState *saveState,
                               PlayerEnvProbeResult *probeResult) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    int thirdProbeCandidateScanCount = 4;
    if (g_PlayerEnvProbeSampleCount <= 4) {
        thirdProbeCandidateScanCount = g_PlayerEnvProbeSampleCount;
    }

    const int firstAboveGroundSampleIndex = g_PlayerEnvProbe_AboveGroundIndices[0];
    const int secondAboveGroundSampleIndex = g_PlayerEnvProbe_AboveGroundIndices[1];
    int bestThirdProbeSampleIndex = 0;
    float bestThirdProbeHeightDelta = 0.0f;
    for (int candidateProbeSampleIndex = 0;
         candidateProbeSampleIndex < thirdProbeCandidateScanCount;
         ++candidateProbeSampleIndex) {
        if (candidateProbeSampleIndex == firstAboveGroundSampleIndex ||
            candidateProbeSampleIndex == secondAboveGroundSampleIndex) {
            continue;
        }

        const zVec3 transformedCandidateProbePoint = TransformPointByMatrix(
            masterModalData->probePoints[kPlayerEnvProbeBasePointOffset +
                                         candidateProbeSampleIndex],
            playerState->motionBasis);
        const float candidateHeightDelta =
            probeResult->candidateScoreBySample[candidateProbeSampleIndex] -
            transformedCandidateProbePoint.y;
        if (candidateHeightDelta > bestThirdProbeHeightDelta &&
            CheckProbeSampleMaskOverlap(firstAboveGroundSampleIndex, secondAboveGroundSampleIndex,
                                        candidateProbeSampleIndex) == 0) {
            bestThirdProbeSampleIndex = candidateProbeSampleIndex;
            bestThirdProbeHeightDelta = candidateHeightDelta;
        }
    }

    if (bestThirdProbeHeightDelta <= g_Player_DeltaTime) {
        return;
    }

    zVec3 firstSynthSupportPoint = g_PlayerEnvProbeWorldPoints[firstAboveGroundSampleIndex];
    zVec3 secondSynthSupportPoint = g_PlayerEnvProbeWorldPoints[secondAboveGroundSampleIndex];
    zVec3 thirdSynthSupportPoint = g_PlayerEnvProbeWorldPoints[bestThirdProbeSampleIndex];
    firstSynthSupportPoint.y = probeResult->candidateScoreBySample[firstAboveGroundSampleIndex];
    secondSynthSupportPoint.y =
        probeResult->candidateScoreBySample[secondAboveGroundSampleIndex];
    thirdSynthSupportPoint.y = probeResult->candidateScoreBySample[bestThirdProbeSampleIndex];

    ComputeTriangleNormal(saveState, &firstSynthSupportPoint, &secondSynthSupportPoint,
                          &thirdSynthSupportPoint);
    const float surfaceDot =
        playerState->steerBasisRef.x * firstSynthSupportPoint.x +
        playerState->steerBasisRef.y * firstSynthSupportPoint.y +
        playerState->steerBasisRef.z * firstSynthSupportPoint.z;
    playerState->worldPos.y = SolveHeightOnSurface(saveState, surfaceDot);
    RebuildOrientationFromNormal(saveState);
}

// Reimplements 0x42c420: Player::AccumulateSlopeForces
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
AccumulateSlopeForces(zUtil_SaveGameState *saveState, PlayerEnvProbeResult *probeResult) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    int clampedAboveGroundCount = g_PlayerEnvProbe_AboveGroundCount;
    if (clampedAboveGroundCount >= 3) {
        clampedAboveGroundCount = 3;
    }

    for (int i = 0; i < clampedAboveGroundCount; ++i) {
        const int sampleIndex = g_PlayerEnvProbe_AboveGroundIndices[i];
        const int bestCandidateIndex = probeResult->bestIndexBySample[sampleIndex];
        const zVec3 &surfaceNormal =
            probeResult->candidateBuffers[sampleIndex].entries[bestCandidateIndex].surfaceNormal;
        if (surfaceNormal.y < g_Player_MaxSlope) {
            const float slopeScale = g_Player_DeltaTime * playerState->gravityAccel * 5.0f;
            playerState->projectileSpawnVel.x += surfaceNormal.x * slopeScale;
            playerState->projectileSpawnVel.y += (surfaceNormal.y - 1.0f) * slopeScale;
            playerState->projectileSpawnVel.z += surfaceNormal.z * slopeScale;
        }
    }
}

// Reimplements 0x42c2e0: Player::UpdateVerticalVelocityAndTransform
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateVerticalVelocityAndTransform(zUtil_SaveGameState *saveState,
                                   PlayerEnvProbeResult *probeResult) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    const float measuredFrameDeltaY =
        (playerState->worldPos.y - playerState->previousTransform.posY) *
        g_Player_InvDeltaTime;
    if (g_PlayerEnvProbe_AboveGroundCount >= 3) {
        playerState->projectileSpawnVel.y = measuredFrameDeltaY;
    } else {
        const float previousVerticalVelocityBlendWeight =
            PlayerFloatFromBits(static_cast<int>(g_Player_DeltaTime * -5.0f * 12102200.0f) +
                                0x3f800000);
        playerState->projectileSpawnVel.y =
            previousVerticalVelocityBlendWeight * playerState->projectileSpawnVel.y +
            (1.0f - previousVerticalVelocityBlendWeight) * measuredFrameDeltaY;
    }

    AccumulateSlopeForces(saveState, probeResult);
    if (playerState->projectileSpawnVel.y > 55.0f) {
        playerState->projectileSpawnVel.y = 0.0f;
    }

    if (playerState->environmentAttachmentActive != 0) {
        return;
    }

    const zVec3 worldVelocity = playerState->projectileSpawnVel;
    playerState->localVel.x = worldVelocity.x * playerState->motionBasis.xx +
                              worldVelocity.y * playerState->motionBasis.xy +
                              worldVelocity.z * playerState->motionBasis.xz;
    playerState->localVel.y = worldVelocity.x * playerState->motionBasis.yx +
                              worldVelocity.y * playerState->motionBasis.yy +
                              worldVelocity.z * playerState->motionBasis.yz;
    playerState->localVel.z = worldVelocity.x * playerState->motionBasis.zx +
                              worldVelocity.y * playerState->motionBasis.zy +
                              worldVelocity.z * playerState->motionBasis.zz;
    if (g_PlayerEnvProbe_AboveGroundCount >= 3) {
        playerState->localVel.y = 0.0f;
    }
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

// Reimplements 0x427440: Player::UpdateMasterTypeHover_FromModalProbe
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateMasterTypeHover_FromModalProbe(zUtil_SaveGameState *saveState) {
    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = primaryModalState->masterModalData;

    float probeHeightByPoint[kPlayerMaxModalProbePoints] = {};
    float outBestHeight = 0.0f;
    PlayerProbeTypeHistogram outTypeHistogram = {};
    int outAttachmentCandidateCount = 0;
    zClass_NodePartial *outAttachmentNode = 0;
    ProbeModalSampleHeights(saveState, probeHeightByPoint, &outBestHeight, 0,
                            &outTypeHistogram, &outAttachmentCandidateCount,
                            &outAttachmentNode);

    playerState->yawVelocityLimit = masterModalData->yawRateMax;

    int lowestProbeIndex = 0;
    float lowestProbeHeight = 5000.0f;
    const int probePointCount = primaryModalState->modalStateCode;
    for (int i = 0; i < probePointCount; ++i) {
        if (probeHeightByPoint[i] < lowestProbeHeight) {
            lowestProbeHeight = probeHeightByPoint[i];
            lowestProbeIndex = i;
        }
    }

    int supportPointIndex[3] = {};
    int supportCount = 0;
    for (int i = 0; i < probePointCount && supportCount < 3; ++i) {
        if (i != lowestProbeIndex) {
            supportPointIndex[supportCount] = i;
            ++supportCount;
        }
    }

    zVec3 supportPoint0 =
        primaryModalState->transformedProbePointWorldByIndex[supportPointIndex[0]];
    supportPoint0.y = probeHeightByPoint[supportPointIndex[0]];
    zVec3 supportPoint1 =
        primaryModalState->transformedProbePointWorldByIndex[supportPointIndex[1]];
    supportPoint1.y = probeHeightByPoint[supportPointIndex[1]];
    zVec3 supportPoint2 =
        primaryModalState->transformedProbePointWorldByIndex[supportPointIndex[2]];
    supportPoint2.y = probeHeightByPoint[supportPointIndex[2]];

    zVec3 probePlaneNormal = {};
    zMath_Vec3_TriangleNormal(&supportPoint0, &supportPoint1, &supportPoint2,
                              &probePlaneNormal);

    float gravityScale = playerState->gravityAccel;
    if (probePlaneNormal.y < g_Player_MaxSlope) {
        gravityScale *= 12.0f;
    }
    const float slopeBase = g_Player_DeltaTime * gravityScale;
    zVec3 slopeImpulse = {};
    slopeImpulse.x = probePlaneNormal.x * slopeBase;
    slopeImpulse.z = probePlaneNormal.z * slopeBase;
    slopeImpulse.y = (probePlaneNormal.y - 1.0f) * g_Player_DeltaTime * slopeBase;
    playerState->projectileSpawnVel.x += slopeImpulse.x;
    playerState->projectileSpawnVel.y += slopeImpulse.y;
    playerState->projectileSpawnVel.z += slopeImpulse.z;

    playerState->localVel =
        TransformWorldVectorToLocal(playerState->projectileSpawnVel, playerState->motionBasis);

    const int normalLerpBits =
        static_cast<int>(masterModalData->hoverNormalLerpRate * g_FrameDeltaTimeSec *
                         12102200.0f) +
        0x3f800000;
    zMath::Vec3LerpNormalize(&playerState->steerBasisRef, &probePlaneNormal,
                             PlayerFloatFromBits(normalLerpBits));
    RebuildSteerBasisRawFromRef(saveState);
    RebuildMotionBasisFromSteerBasis(saveState);

    float minHoverClearance = 1000.0f;
    for (int i = 0; i < probePointCount; ++i) {
        const zVec3 transformedProbePoint = TransformPointByMatrix(
            masterModalData->probePoints[kPlayerEnvProbeBasePointOffset + i],
            playerState->motionBasis);
        primaryModalState->transformedProbePointWorldByIndex[i] = transformedProbePoint;

        const float clearance = transformedProbePoint.y - probeHeightByPoint[i];
        if (clearance < minHoverClearance) {
            minHoverClearance = clearance;
        }
    }

    const float hoverLiftError = minHoverClearance - masterModalData->modeAltTransitionTime;
    if (playerState->modeVariantNode != 0) {
        zClass_Class::gwNodeSetActive(playerState->modeVariantNode,
                                      hoverLiftError <= 2.0f ? 1 : 0);
    }

    if (hoverLiftError >= 2.0f && playerState->localVel.y >= 0.0f) {
        playerState->localVel.y = 0.0f;
    }

    const int liftDampingBits =
        static_cast<int>(masterModalData->hoverLiftDampingRate * g_Player_DeltaTime *
                         12102200.0f) +
        0x3f800000;
    const float liftDamping = PlayerFloatFromBits(liftDampingBits);
    playerState->localVel.y =
        liftDamping * playerState->localVel.y -
        (1.0f - liftDamping) * masterModalData->hoverLiftScale * hoverLiftError;

    if (minHoverClearance < 0.0f) {
        playerState->worldPos.y -= minHoverClearance - 0.5f;
        playerState->motionBasis.posY = playerState->worldPos.y;
    }

    if (probePlaneNormal.y >= g_Player_MaxSlope) {
        playerState->localVel.y += 5.0f;
    }

    if (playerState->slipSfxActive != 0) {
        playerState->projectileSpawnVel =
            TransformLocalVectorToWorld(playerState->localVel, playerState->motionBasis);
    }

    zVec3 yawRelativeNormal = {};
    zMath::Vec3RotateY(&yawRelativeNormal, &playerState->steerBasisRef,
                       -playerState->restartYawRad);
    playerState->vehiclePitchRad = static_cast<float>(asin(yawRelativeNormal.z));
    playerState->vehicleRollRad = static_cast<float>(asin(-yawRelativeNormal.x));

    const float speedAbs = static_cast<float>(fabs(playerState->localVel.z));
    const float pitchWaveArg =
        (masterModalData->hoverPitchWaveSpeedRate * speedAbs +
         masterModalData->hoverPitchWaveBaseRate) *
        g_Time_AccumulatedTimeSec;
    const float rollWaveArg =
        (masterModalData->hoverRollWaveSpeedRate * speedAbs +
         masterModalData->hoverRollWaveBaseRate) *
        g_Time_AccumulatedTimeSec;
    const float pitchWave =
        static_cast<float>(sin(pitchWaveArg)) * masterModalData->hoverPitchWaveAmplitude;
    const float rollWave =
        static_cast<float>(sin(rollWaveArg)) * masterModalData->hoverRollWaveAmplitude +
        masterModalData->hoverRollYawCoupleScale * playerState->angVelYaw *
            playerState->localVel.z;
    playerState->vehiclePitchRad += g_Player_DeltaTime * pitchWave;
    playerState->vehicleRollRad += g_Player_DeltaTime * rollWave;

    playerState->vehiclePitchRad = PlayerClampSigned(playerState->vehiclePitchRad, 0.523599982f);
}

// Reimplements 0x427ec0: Player::UpdateMasterTypeAmphib_FromModalProbe
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateMasterTypeAmphib_FromModalProbe(zUtil_SaveGameState *saveState) {
    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = primaryModalState->masterModalData;

    float probeHeightByPoint[kPlayerMaxModalProbePoints] = {};
    float outBestHeight = 0.0f;
    PlayerProbeTypeHistogram outTypeHistogram = {};
    int outAttachmentCandidateCount = 0;
    zClass_NodePartial *outAttachmentNode = 0;
    ProbeModalSampleHeights(saveState, probeHeightByPoint, &outBestHeight, 0,
                            &outTypeHistogram, &outAttachmentCandidateCount,
                            &outAttachmentNode);

    playerState->yawVelocityLimit = masterModalData->yawRateMax;
    const int probePointCount = primaryModalState->modalStateCode;
    if (outTypeHistogram.countByImpactSlot[1] >= probePointCount) {
        playerState->amphibProbeCoverageFailed = 0;
    } else if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        playerState->amphibProbeCoverageFailed = 1;
        TransitionToMasterTypeTrack(saveState, 0);
    } else {
        playerState->projectileSpawnVel.x = 0.0f;
        playerState->projectileSpawnVel.z = 0.0f;
        playerState->localVel.x = 0.0f;
        playerState->localVel.z = 0.0f;
        playerState->aiTopLevelState = 0;
        playerState->aiStateUntilTime = g_Time_AccumulatedTimeSec + 8.0f;
    }

    float maxSampleHeight = outBestHeight;
    for (int i = 0; i < probePointCount; ++i) {
        if (maxSampleHeight < probeHeightByPoint[i]) {
            maxSampleHeight = probeHeightByPoint[i];
        }
    }

    const float oldWorldY = playerState->worldPos.y;
    playerState->worldPos.y = maxSampleHeight + masterModalData->modeAltTransitionTime;
    const float verticalVelocity =
        (playerState->worldPos.y - oldWorldY) * g_Player_InvDeltaTime;
    playerState->localVel.y = verticalVelocity;
    playerState->projectileSpawnVel.y = verticalVelocity;

    zVec3 amphibUpVector = g_Player_AmphibBasisUpRef;
    ApplyAmphibSpeedOscillation(saveState, &amphibUpVector, 1);

    const int steerLerpBits =
        static_cast<int>(-(g_FrameDeltaTimeSec * g_Player_AmphibSteerBasisLerpRate) *
                         12102200.0f) +
        0x3f800000;
    zMath::Vec3LerpNormalize(&playerState->steerBasisRef, &amphibUpVector,
                             PlayerFloatFromBits(steerLerpBits));
    if (playerState->steerBasisRef.y == 0.0f) {
        playerState->steerBasisRef.y = 0.00100000005f;
    }

    zVec3 rawBasis = playerState->steerBasisNorm;
    rawBasis.y = -((rawBasis.x * playerState->steerBasisRef.x +
                    rawBasis.z * playerState->steerBasisRef.z) /
                   playerState->steerBasisRef.y);
    zMath::Vec3Normalize(&rawBasis);
    playerState->steerBasisRaw = rawBasis;
    RebuildMotionBasisFromSteerBasis(saveState);

    zMath::Vec3RotateY(&amphibUpVector, &playerState->steerBasisRef,
                       -playerState->restartYawRad);
    playerState->vehiclePitchRad = static_cast<float>(asin(amphibUpVector.z));
    playerState->vehicleRollRad = static_cast<float>(asin(-amphibUpVector.x));
    playerState->vehiclePitchRad = PlayerClampSigned(playerState->vehiclePitchRad, 0.523599982f);
}

// Reimplements 0x4279f0: Player::UpdateMasterTypeAmphib
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateMasterTypeAmphib(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;

    RebuildSteerBasisFromMotionAxes(saveState);
    UpdateAutoTurnAndSteerFromTarget(saveState);

    const float yawDelta = playerState->angVelYaw * g_Player_DeltaTime;
    if (playerState->environmentAttachmentActive != 0) {
        playerState->yawPoseCache = PlayerWrapSignedTwoPi(playerState->yawPoseCache + yawDelta);
        zMath::MatStackPushPtr((float *)&playerState->environmentAttachmentMatrix);
        zMath::MatLoadIdentity();
        gwNode::BuildNodeToAncestorMatrix(playerState->environmentAttachmentNode, 3);
        zMath::MatStackPopPtr();
        playerState->restartYawRad =
            ExtractYawFromMatrix(&playerState->environmentAttachmentMatrix) +
            playerState->yawPoseCache;
    } else {
        playerState->restartYawRad =
            PlayerWrapSignedTwoPi(playerState->restartYawRad + yawDelta);
        playerState->pitchPoseCache = playerState->vehiclePitchRad;
        playerState->yawPoseCache = playerState->restartYawRad;
        playerState->rollPoseCache = playerState->vehicleRollRad;
    }

    zMath::MatBuildEulerRotation3x3(&playerState->motionBasis, playerState->vehiclePitchRad,
                                    playerState->restartYawRad, playerState->vehicleRollRad);
    RebuildSteerBasisFromMotionBasis(saveState);

    playerState->axisClampRuntime = masterModalData->maxSpeed;
    UpdateYawVelocityFromSteerInput(saveState);

    if (playerState->environmentAttachmentActive != 0) {
        zMath::Vec3RotateY(&playerState->yawRotatedLocalVel, &playerState->localVel,
                           playerState->yawPoseCache);
        playerState->fxOffsetLocal.x +=
            playerState->yawRotatedLocalVel.x * g_Player_DeltaTime;
        playerState->fxOffsetLocal.z +=
            playerState->yawRotatedLocalVel.z * g_Player_DeltaTime;
        playerState->fxOffsetLocal.y = 0.0f;

        const zVec3 attachedWorld =
            TransformPointByMatrix(playerState->fxOffsetLocal,
                                   playerState->environmentAttachmentMatrix);
        playerState->projectileSpawnVel.x =
            (attachedWorld.x - playerState->worldPos.x) * g_Player_InvDeltaTime;
        playerState->projectileSpawnVel.y =
            (attachedWorld.y - playerState->worldPos.y) * g_Player_InvDeltaTime;
        playerState->projectileSpawnVel.z =
            (attachedWorld.z - playerState->worldPos.z) * g_Player_InvDeltaTime;
        playerState->worldPos = attachedWorld;
    } else {
        const float negSteerX = -playerState->steerBasisNorm.x;
        const float negSteerZ = -playerState->steerBasisNorm.z;
        playerState->projectileSpawnVel.x =
            negSteerX * playerState->localVel.z + negSteerZ * playerState->localVel.x;
        playerState->projectileSpawnVel.y = playerState->localVel.y;
        playerState->projectileSpawnVel.z =
            negSteerZ * playerState->localVel.z - negSteerX * playerState->localVel.x;
        playerState->worldPos.x += playerState->projectileSpawnVel.x * g_Player_DeltaTime;
        playerState->yawRotatedLocalVel = playerState->projectileSpawnVel;
        playerState->worldPos.z += playerState->projectileSpawnVel.z * g_Player_DeltaTime;
    }

    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;

    if (playerState->lifecycleState != 0) {
        ProcessPendingContactQueues(saveState);
    }

    UpdateMasterTypeAmphib_FromModalProbe(saveState);

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        ProcessPendingContactQueues(saveState);
        if (CollectPendingCollisionContactsForQuadProbe(saveState, 0.0f) != 0) {
            ApplyPendingCollisionProbeVelocity(saveState);
            playerState->collisionProbeResolved = 1;
        } else {
            playerState->collisionProbeResolved = 0;
        }
    }

    zClass_Object3D::gwObject3DSetRotation(playerState->rootNode, playerState->vehiclePitchRad,
                                           playerState->restartYawRad,
                                           playerState->vehicleRollRad);
    zClass_Object3D::gwObject3DSetPosition(playerState->rootNode, playerState->worldPos.x,
                                           playerState->worldPos.y,
                                           playerState->worldPos.z);
    playerState->fxOffsetWorld.x = playerState->fxOffsetLocal.x + playerState->worldPos.x;
    playerState->fxOffsetWorld.y = playerState->fxOffsetLocal.y + playerState->worldPos.y;
    playerState->fxOffsetWorld.z = playerState->fxOffsetLocal.z + playerState->worldPos.z;

    zClass_Class::gwNodeUpdate(playerState->rootNode);
    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    float *const rootMatrix = zClass_Object3D::gwObject3DGetMatrixPtr(playerState->rootNode);
    memcpy(&playerState->previousTransform, rootMatrix, sizeof(playerState->previousTransform));
    playerState->bankBasis = playerState->steerBasisNorm;
    playerState->cachedVehicleRotationAngles = playerState->vehicleRotationAngles;

    if (primaryModalState->nodeWake != 0) {
        const float wakeScale =
            static_cast<float>(fabs(playerState->localVel.z)) / playerState->axisClampRuntime;
        zClass_Object3D::gwObject3DSetScale(primaryModalState->nodeWake, wakeScale, wakeScale,
                                            wakeScale);
        zClass_Object3D::gwObject3DSetScale(primaryModalState->nodeSplashL, wakeScale,
                                            wakeScale, wakeScale);
        zClass_Object3D::gwObject3DSetScale(primaryModalState->nodeSplashR, wakeScale,
                                            wakeScale, wakeScale);
    }
}

// Reimplements 0x427140: Player::UpdateMasterTypeHover
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateMasterTypeHover(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;

    RebuildSteerBasisFromMotionAxes(saveState);
    UpdateAutoTurnAndSteerFromTarget(saveState);

    playerState->restartYawRad =
        PlayerWrapSignedTwoPi(playerState->restartYawRad +
                              playerState->angVelYaw * g_Player_DeltaTime);

    zMath::MatBuildEulerRotation3x3(&playerState->motionBasis, playerState->vehiclePitchRad,
                                    playerState->restartYawRad, playerState->vehicleRollRad);
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;
    RebuildSteerBasisFromMotionBasis(saveState);

    playerState->axisClampRuntime = masterModalData->maxSpeed;
    UpdateYawVelocityFromSteerInput(saveState);

    playerState->projectileSpawnVel =
        TransformLocalVectorToWorld(playerState->localVel, playerState->motionBasis);

    playerState->worldPos.x += g_Player_DeltaTime * playerState->projectileSpawnVel.x;
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->worldPos.z += g_Player_DeltaTime * playerState->projectileSpawnVel.z;
    playerState->motionBasis.posZ = playerState->worldPos.z;
    playerState->worldPos.y += g_Player_DeltaTime * playerState->projectileSpawnVel.y;
    playerState->motionBasis.posY = playerState->worldPos.y;

    if (playerState->lifecycleState != 0) {
        ProcessPendingContactQueues(saveState);
    }

    if (playerState->slipSfxActive != 0 &&
        (playerState->playerCollisionResolved != 0 ||
         playerState->worldCollisionResolved != 0 ||
         playerState->preferredCollisionResolved != 0)) {
        StopSlipSfx(saveState);
    }

    UpdateMasterTypeHover_FromModalProbe(saveState);

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        ProcessPendingContactQueues(saveState);
        if (CollectPendingCollisionContactsForQuadProbe(saveState, 0.0f) != 0) {
            ApplyPendingCollisionProbeVelocity(saveState);
            playerState->collisionProbeResolved = 1;
        } else {
            playerState->collisionProbeResolved = 0;
        }
    }

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

// Reimplements 0x43b1b0: Player::BuildGunFireTransform
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL BuildGunFireTransform(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerModalState *const primaryModalState = saveState->primaryModalState;

    zMat4x3 rootMatrix = {0};
    memcpy(&rootMatrix, zClass_Object3D::gwObject3DGetMatrixPtr(playerState->rootNode),
           sizeof(rootMatrix));

    if (primaryModalState->modalNode == 0) {
        memcpy(&playerState->gunFireTransform, &rootMatrix, sizeof(rootMatrix));
        return;
    }

    zMat4x3 modalMatrix = {0};
    memcpy(&modalMatrix, zClass_Object3D::gwObject3DGetMatrixPtr(primaryModalState->modalNode),
           sizeof(modalMatrix));

    playerState->gunFireTransform.xx =
        modalMatrix.xx * rootMatrix.xx + modalMatrix.xy * rootMatrix.yx +
        modalMatrix.xz * rootMatrix.zx;
    playerState->gunFireTransform.xy =
        modalMatrix.xx * rootMatrix.xy + modalMatrix.xy * rootMatrix.yy +
        modalMatrix.xz * rootMatrix.zy;
    playerState->gunFireTransform.xz =
        modalMatrix.xx * rootMatrix.xz + modalMatrix.xy * rootMatrix.yz +
        modalMatrix.xz * rootMatrix.zz;
    playerState->gunFireTransform.yx =
        modalMatrix.yx * rootMatrix.xx + modalMatrix.yy * rootMatrix.yx +
        modalMatrix.yz * rootMatrix.zx;
    playerState->gunFireTransform.yy =
        modalMatrix.yx * rootMatrix.xy + modalMatrix.yy * rootMatrix.yy +
        modalMatrix.yz * rootMatrix.zy;
    playerState->gunFireTransform.yz =
        modalMatrix.yx * rootMatrix.xz + modalMatrix.yy * rootMatrix.yz +
        modalMatrix.yz * rootMatrix.zz;
    playerState->gunFireTransform.zx =
        modalMatrix.zy * rootMatrix.yx + modalMatrix.zz * rootMatrix.zx;
    playerState->gunFireTransform.zy =
        modalMatrix.zy * rootMatrix.yy + modalMatrix.zz * rootMatrix.zy;
    playerState->gunFireTransform.zz =
        modalMatrix.zy * rootMatrix.yz + modalMatrix.zz * rootMatrix.zz;
    playerState->gunFireTransform.posX =
        modalMatrix.posY * rootMatrix.yx + modalMatrix.posZ * rootMatrix.zx +
        rootMatrix.posX;
    playerState->gunFireTransform.posY =
        modalMatrix.posY * rootMatrix.yy + modalMatrix.posZ * rootMatrix.zy +
        rootMatrix.posY;
    playerState->gunFireTransform.posZ =
        modalMatrix.posY * rootMatrix.yz + modalMatrix.posZ * rootMatrix.zz +
        rootMatrix.posZ;
}

// Reimplements 0x43b3e0: Player::UpdateAltGunAimBasisOrigin
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateAltGunAimBasisOrigin(zUtil_SaveGameState *saveState, zVec3 *outBasisOrigin) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    zMat4x3 gunMatrix = {0};
    memcpy(&gunMatrix, zClass_Object3D::gwObject3DGetMatrixPtr(playerState->gunNode),
           sizeof(gunMatrix));

    zMat4x3 turretMatrix = {0};
    memcpy(&turretMatrix, zClass_Object3D::gwObject3DGetMatrixPtr(playerState->turretNode),
           sizeof(turretMatrix));

    zMat4x3 gunFireTransform = {0};
    memcpy(&gunFireTransform, &playerState->gunFireTransform, sizeof(gunFireTransform));

    const float localAimX = turretMatrix.zx * gunMatrix.posZ;
    const float localAimY = turretMatrix.posY + gunMatrix.posY;
    const float localAimZ = turretMatrix.zz * gunMatrix.posZ + turretMatrix.posZ;

    outBasisOrigin->x =
        gunFireTransform.xx * localAimX + gunFireTransform.yx * localAimY +
        gunFireTransform.zx * localAimZ + gunFireTransform.posX;
    outBasisOrigin->y =
        gunFireTransform.xy * localAimX + gunFireTransform.yy * localAimY +
        gunFireTransform.zy * localAimZ + gunFireTransform.posY;
    outBasisOrigin->z =
        gunFireTransform.xz * localAimX + gunFireTransform.yz * localAimY +
        gunFireTransform.zz * localAimZ + gunFireTransform.posZ;
}

// Reimplements 0x43a4f0: Player::UpdateGunAndTurretAimNodes
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateGunAndTurretAimNodes(const zVec3 *aimDirection, zClass_NodePartial *gunNode,
                           zClass_NodePartial *turretNode) {
    if (gunNode == 0 || turretNode == 0 || aimDirection == 0) {
        return;
    }

    const float horizontalLength =
        PlayerFastSqrtEstimate(aimDirection->x * aimDirection->x +
                               aimDirection->z * aimDirection->z);

    zMat4x3 *const gunMatrix =
        (zMat4x3 *)zClass_Object3D::gwObject3DGetMatrixPtr(gunNode);
    gunMatrix->xx = 1.0f;
    gunMatrix->xy = 0.0f;
    gunMatrix->xz = 0.0f;
    gunMatrix->yx = 0.0f;
    gunMatrix->yy = horizontalLength;
    gunMatrix->yz = aimDirection->y;
    gunMatrix->zx = 0.0f;
    gunMatrix->zy = -aimDirection->y;
    gunMatrix->zz = horizontalLength;
    zClass_Object3D::gwObject3DSetMatrix(gunNode, (float *)gunMatrix);

    float yawForward = 1.0f;
    float yawSide = 0.0f;
    if (horizontalLength != 0.0f) {
        const float invHorizontalLength = 1.0f / horizontalLength;
        yawForward = -(aimDirection->z * invHorizontalLength);
        yawSide = -(aimDirection->x * invHorizontalLength);
    }

    zMat4x3 *const turretMatrix =
        (zMat4x3 *)zClass_Object3D::gwObject3DGetMatrixPtr(turretNode);
    turretMatrix->xx = yawForward;
    turretMatrix->xy = 0.0f;
    turretMatrix->xz = -yawSide;
    turretMatrix->yx = 0.0f;
    turretMatrix->yy = 1.0f;
    turretMatrix->yz = 0.0f;
    turretMatrix->zx = yawSide;
    turretMatrix->zy = 0.0f;
    turretMatrix->zz = yawForward;
    zClass_Object3D::gwObject3DSetMatrix(turretNode, (float *)turretMatrix);
}

// Reimplements 0x43a600: Player::UpdateAltGunAimDirection
// (src/Battlesport/player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateAltGunAimDirection(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;

    if (playerState->gunNode == 0 || playerState->turretNode == 0) {
        return;
    }

    BuildGunFireTransform(saveState);
    UpdateAltGunAimBasisOrigin(saveState, &playerState->aimBasisOrigin);

    zVec3 aimDirection = {0};
    aimDirection.x = playerState->storedTargetPos.x - playerState->aimBasisOrigin.x;
    aimDirection.y = playerState->storedTargetPos.y - playerState->aimBasisOrigin.y;
    aimDirection.z = playerState->storedTargetPos.z - playerState->aimBasisOrigin.z;

    const float aimLength =
        static_cast<float>(sqrt(aimDirection.x * aimDirection.x +
                                aimDirection.y * aimDirection.y +
                                aimDirection.z * aimDirection.z));
    const float invAimLength = 1.0f / aimLength;
    aimDirection.x *= invAimLength;
    aimDirection.y *= invAimLength;
    aimDirection.z *= invAimLength;

    const float pitchY = OptCatalog::ComputeAimPitchForTarget(
        playerState->activeAltGunController->optCatalogEntry, &playerState->aimBasisOrigin,
        &playerState->gunFireDir, &playerState->storedTargetPos,
        &playerState->aimTargetDistanceApprox);
    playerState->aimPitchResult = pitchY;
    if (pitchY != -1.0f && playerState->altGunTransitionState == 1) {
        ApplyAimPitchToDirection(&aimDirection, pitchY);
    }

    if (playerState->cameraTickEnabled != 0 &&
        (playerState->cameraState == kPlayerCameraStateThirdPerson ||
         playerState->cameraState == kPlayerCameraStateFirstPerson)) {
        const float cameraDot =
            aimDirection.x * playerState->cameraDirNext.x +
            aimDirection.y * playerState->cameraDirNext.y +
            aimDirection.z * playerState->cameraDirNext.z;
        const float targetDistanceSq =
            zMath::Vec3DeltaLengthSq(&playerState->storedTargetPos, &playerState->worldPos);
        if (cameraDot < 0.0f || targetDistanceSq < 9.0f) {
            aimDirection = playerState->gunFireDir;
            playerState->usePresetGunFireDir = 1;
        }
    }

    aimDirection = TransformWorldVectorToLocal(aimDirection, playerState->gunFireTransform);
    if (aimDirection.y > masterModalData->gunPitchRate) {
        ApplyAimPitchToDirection(&aimDirection, masterModalData->gunPitchRate);
    }
    if (aimDirection.y < masterModalData->gunPitchMin) {
        ApplyAimPitchToDirection(&aimDirection, masterModalData->gunPitchMin);
    }

    const int smoothingBits =
        static_cast<int>(g_FrameDeltaTimeSec * -8.0f * 12102200.0f) + 0x3f800000;
    zMath::Vec3LerpNormalize(&playerState->altGunAimOrigin, &aimDirection,
                             PlayerFloatFromBits(smoothingBits));
    aimDirection = playerState->altGunAimOrigin;

    UpdateGunAndTurretAimNodes(&aimDirection, playerState->gunNode, playerState->turretNode);
    playerState->gunFireDir =
        TransformLocalVectorToWorld(aimDirection, playerState->gunFireTransform);
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

// Reimplements 0x43a900: Player::DecayAndApplyAltFireSlotOffsetToNode
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL DecayAndApplyAltFireSlotOffsetToNode(
    PlayerGunFireSlot *slot, zClass_NodePartial *slotNode, float slotAimY, int applyMatrix) {
    const int dampingBits =
        static_cast<int>(g_FrameDeltaTimeSec * -8.09f * 12102200.0f) + 0x3f800000;
    slot->offset *= PlayerFloatFromBits(dampingBits);
    if (fabsf(slot->offset) < 0.01f) {
        slot->offset = 0.0f;
    }

    float *const matrix = zClass_Object3D::gwObject3DGetMatrixPtr(slotNode);
    matrix[10] = -(slotAimY * slot->offset);
    matrix[11] = slot->offset;
    if (applyMatrix != 0) {
        zClass_Object3D::gwObject3DSetMatrix(slotNode, matrix);
    }
}

// Reimplements 0x43a980: Player::ApplyGunFireSlotOffsetToNode
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ApplyGunFireSlotOffsetToNode(
    zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (playerState->gunNode == 0) {
        return;
    }

    if (playerState->altFireSlotLeft.offset != 0.0f) {
        playerState->altFireSlotLeft.offset = 0.0f;
        DecayAndApplyAltFireSlotOffsetToNode(&playerState->altFireSlotLeft,
                                             playerState->altFireSlotLeft.attachNode,
                                             playerState->gunFireDir.y, 1);
    }
    if (playerState->altFireSlotRight.offset != 0.0f) {
        playerState->altFireSlotRight.offset = 0.0f;
        DecayAndApplyAltFireSlotOffsetToNode(&playerState->altFireSlotRight,
                                             playerState->altFireSlotRight.attachNode,
                                             playerState->gunFireDir.y, 1);
    }
    if (playerState->altFireSlotCenter.offset != 0.0f) {
        playerState->altFireSlotCenter.offset = 0.0f;
        DecayAndApplyAltFireSlotOffsetToNode(&playerState->altFireSlotCenter,
                                             playerState->altFireSlotCenter.attachNode,
                                             playerState->gunFireDir.y, 0);
    }
}

// Reimplements 0x43aa30: Player::SelectAltGunFireOriginAndSlot (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SelectAltGunFireOriginAndSlot(
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

// Reimplements 0x43acf0: Player::SelectPrimaryGunFireOriginAndSlot
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SelectPrimaryGunFireOriginAndSlot(
    zUtil_SaveGameState *saveState, PlayerGunFireSlot **outActiveFireSlotPtr) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const activePrimaryGunController =
        playerState->activePrimaryGunController;

    if (playerState->gunNode == 0 || playerState->turretNode == 0) {
        const float y = playerState->worldPos.y + 1.0f;
        playerState->primaryFireOrigin.x = playerState->worldPos.x;
        playerState->primaryFireOrigin.y = y;
        playerState->primaryFireOrigin.z = playerState->worldPos.z;
        playerState->aimBasisOrigin.x = playerState->worldPos.x;
        playerState->aimBasisOrigin.y = y;
        playerState->aimBasisOrigin.z = playerState->worldPos.z;
        playerState->gunFireDir = playerState->steerBasisRaw;
        return;
    }

    if (playerState->damageVisualFlag != 0) {
        CacheGunHardpointsAndDetachDisplays(saveState, 0);
        playerState->damageVisualFlag = 0;
    }

    zMat4x3 aimBasisWorldMatrix = {0};
    ComposeAimBasisWorldMatrix(saveState, &aimBasisWorldMatrix);

    switch (playerState->primaryHardpointSelectState) {
    case 0:
        if (activePrimaryGunController->attachState != 0) {
            playerState->primaryFireOrigin.x = aimBasisWorldMatrix.posX;
            playerState->primaryFireOrigin.y = aimBasisWorldMatrix.posY;
            playerState->primaryFireOrigin.z = aimBasisWorldMatrix.posZ;
        } else {
            playerState->primaryFireOrigin =
                TransformPointByMatrix(playerState->firePointCenter, aimBasisWorldMatrix);
        }
        playerState->altFireSlotCenter.attachNode =
            activePrimaryGunController->attachNodePrimary;
        *outActiveFireSlotPtr = &playerState->altFireSlotCenter;
        return;

    case 1:
        playerState->primaryFireOrigin =
            TransformPointByMatrix(playerState->firePointRight, aimBasisWorldMatrix);
        playerState->altFireSlotRight.attachNode =
            activePrimaryGunController->attachNodeSecondary;
        *outActiveFireSlotPtr = &playerState->altFireSlotRight;
        playerState->primaryHardpointSelectState = 2;
        return;

    case 2:
        playerState->primaryFireOrigin =
            TransformPointByMatrix(playerState->firePointLeft, aimBasisWorldMatrix);
        playerState->altFireSlotLeft.attachNode =
            activePrimaryGunController->attachNodePrimary;
        *outActiveFireSlotPtr = &playerState->altFireSlotLeft;
        playerState->primaryHardpointSelectState = 1;
        return;
    }
}

// Reimplements 0x43c2d0: Player::UpdateContinuousAltGunFireController
// (D:\Proj\GameZRecoil\zWeapon.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateContinuousAltGunFireController(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const activeAltGunController =
        playerState->activeAltGunController;

    if (saveState->primaryModalState->masterModalData->masterType == kPlayerMasterTypeSub) {
        playerState->queuedFixedDamageFlag = 1;
        return;
    }

    if (playerState->altGunFireHeldFlag == 0) {
        const int playerOrdinal = playerState->playerOrdinal;
        playerState->altGunFireHeldFlag = 1;
        OptCatalog::ActivateTrailRuntimeState(activeAltGunController->trailRuntimeState,
                                              playerOrdinal);
    }

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        ++g_HudSensorTracker.primaryGunDispatchCount;
    }
}

// Reimplements 0x43c330: Player::AltGunEnsureAuxEffectActive
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL AltGunEnsureAuxEffectActive(
    zUtil_SaveGameState *saveState, PlayerGunFireController *gunController, zVec3 *effectPos) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    zVec3 spawnDir = {0};
    if (playerState->usePresetGunFireDir != 0) {
        spawnDir = playerState->gunFireDir;
    } else {
        spawnDir.x = playerState->storedTargetPos.x - effectPos->x;
        spawnDir.y = playerState->storedTargetPos.y - effectPos->y;
        spawnDir.z = playerState->storedTargetPos.z - effectPos->z;

        const float length =
            static_cast<float>(sqrt(spawnDir.x * spawnDir.x + spawnDir.y * spawnDir.y +
                                    spawnDir.z * spawnDir.z));
        const float invLength = 1.0f / length;
        spawnDir.x *= invLength;
        spawnDir.y *= invLength;
        spawnDir.z *= invLength;
    }

    if (OptCatalog::AllocRuntimeInstance(gunController->optCatalogEntry, playerState->rootNode,
                                         &playerState->variantTag, effectPos, &spawnDir,
                                         &playerState->projectileSpawnVel, saveState, 0) == 0) {
        return 0;
    }

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable &&
        zInput_DI_IsForceFeedbackEnabled() != 0) {
        zInput_DI_RestartPrimaryFireEffect(g_zInputFfEffectSet);
    }

    return 1;
}

// Reimplements 0x43c430: Player::AltGunLaunchProjectile
// (D:\Proj\GameZRecoil\zWeapon.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL AltGunLaunchProjectile(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const activeAltGunController =
        playerState->activeAltGunController;

    if (saveState != (zUtil_SaveGameState *)g_GameStateOrMapTable &&
        (playerState->altGunTransitionState == 0x180 ||
         playerState->altGunTransitionState == 0x100)) {
        OptCatalog::RemoveRuntimeInstance(
            playerState->altWeaponBanks[8].controllerB.optCatalogEntry, 0,
            playerState->rootNode);
        OptCatalog::RemoveRuntimeInstance(
            playerState->altWeaponBanks[9].controllerB.optCatalogEntry, 0,
            playerState->rootNode);
        activeAltGunController->attachState =
            OptCatalog::AllocOrReuseAttachNodeClone(activeAltGunController->optCatalogEntry);
    } else {
        OptCatalogRuntimeInstanceStorage *const attachState =
            (OptCatalogRuntimeInstanceStorage *)activeAltGunController->attachState;
        zClass_Class::RemoveChild(activeAltGunController->attachNodePrimary,
                                  attachState->projectileNode);
    }

    if (OptCatalog::AllocRuntimeInstance(
            activeAltGunController->optCatalogEntry, playerState->rootNode,
            &playerState->variantTag, &playerState->altFireOrigin, &playerState->gunFireDir,
            &playerState->projectileSpawnVel, saveState,
            (OptCatalogRuntimeInstanceStorage *)activeAltGunController->attachState) == 0) {
        OptCatalogRuntimeInstanceStorage *const attachState =
            (OptCatalogRuntimeInstanceStorage *)activeAltGunController->attachState;
        zClass_Class::AddChild(activeAltGunController->attachNodePrimary,
                               attachState->projectileNode);
        return 0;
    }

    if ((activeAltGunController->optCatalogEntry->flags &
         kPlayerOptCatalogFlagTetherGuided) == 0) {
        activeAltGunController->attachState = 0;
        if (activeAltGunController->ammoOrCharge > 1.0f) {
            playerState->altGunTransitionState = 2;
        }
        playerState->altGunTransitionController = activeAltGunController;
        return 1;
    }

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        playerState->pendingAltCameraToggle = 1;
    }
    playerState->altGunTransitionState = 0x100;
    playerState->altGunTransitionController = activeAltGunController;
    return 1;
}

// Reimplements 0x43c550: Player::AltGunFireSimpleProjectile
// (D:\Proj\GameZRecoil\zWeapon.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL AltGunFireSimpleProjectile(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const activeAltGunController =
        playerState->activeAltGunController;

    zVec3 spawnDir = {0};
    if (activeAltGunController->optCatalogEntry->gravity == 0.0f) {
        spawnDir.x = playerState->storedTargetPos.x - playerState->altFireOrigin.x;
        spawnDir.y = playerState->storedTargetPos.y - playerState->altFireOrigin.y;
        spawnDir.z = playerState->storedTargetPos.z - playerState->altFireOrigin.z;

        const float length =
            static_cast<float>(sqrt(spawnDir.x * spawnDir.x + spawnDir.y * spawnDir.y +
                                    spawnDir.z * spawnDir.z));
        const float invLength = 1.0f / length;
        spawnDir.x *= invLength;
        spawnDir.y *= invLength;
        spawnDir.z *= invLength;
    } else {
        spawnDir = playerState->gunFireDir;
    }

    return OptCatalog::AllocRuntimeInstance(activeAltGunController->optCatalogEntry,
                                            playerState->rootNode, &playerState->variantTag,
                                            &playerState->altFireOrigin, &spawnDir,
                                            &playerState->projectileSpawnVel, saveState, 0) != 0;
}

// Reimplements 0x43c190: Player::ProcessAltGunFireDispatchRequest
// (D:\Proj\GameZRecoil\zWeapon.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
ProcessAltGunFireDispatchRequest(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const activeAltGunController =
        playerState->activeAltGunController;
    PlayerGunFireSlot *activeFireSlot = 0;
    SelectAltGunFireOriginAndSlot(saveState, &activeFireSlot);

    if (playerState->altGunFireHeldFlag != 0) {
        if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
            ++g_HudSensorTracker.primaryGunDispatchCount;
        }
        return;
    }

    playerState->altGunDispatchRequested = 0;
    if (activeAltGunController->ammoOrCharge <= 0.0f) {
        if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
            OptCatalog::PlayTriggerInactiveWarning();
        }
        return;
    }

    int didFire = 0;
    if (playerState->activeAltBankIndex == 1) {
        didFire = AltGunEnsureAuxEffectActive(saveState, activeAltGunController,
                                              &playerState->altFireOrigin);
    } else if ((activeAltGunController->optCatalogEntry->flags &
                kOptCatalogFlagCreateTrail) != 0) {
        UpdateContinuousAltGunFireController(saveState);
        didFire = activeFireSlot != 0;
    } else {
        if (activeAltGunController->attachState != 0) {
            didFire = AltGunLaunchProjectile(saveState);
        } else {
            didFire = AltGunFireSimpleProjectile(saveState);
        }

        if (didFire != 0 && saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable &&
            zInput_DI_IsForceFeedbackEnabled() != 0) {
            zInput_DI_PlayAltFireEffect(g_zInputFfEffectSet,
                                        activeAltGunController->optCatalogEntry->damage *
                                            0.0151515156f);
        }
    }

    if (didFire == 0) {
        return;
    }

    if ((activeAltGunController->flags & 1) != 0 &&
        activeAltGunController->attachNodePrimary != 0) {
        activeFireSlot->offset = 1.5f;
    }

    if (activeAltGunController->ammoOrCharge != kPlayerAltAmmoDisabledSentinel) {
        activeAltGunController->ammoOrCharge -= 1.0f;
        if (activeAltGunController->ammoOrCharge < 0.0f) {
            activeAltGunController->ammoOrCharge = 0.0f;
        }
    }

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        ++g_HudSensorTracker.primaryGunDispatchCount;
    }
}

// Reimplements 0x43a400: Player::ProcessPrimaryGunDispatchRequest
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
ProcessPrimaryGunDispatchRequest(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const activePrimaryGunController =
        playerState->activePrimaryGunController;

    if (activePrimaryGunController->scrollTextureModelA != 0) {
        zModel_Instance_UpdateScrollingTexturesIfNeeded(
            (zModel_InstancePartial *)activePrimaryGunController->scrollTextureModelA);
    }
    if (activePrimaryGunController->scrollTextureModelB != 0) {
        zModel_Instance_UpdateScrollingTexturesIfNeeded(
            (zModel_InstancePartial *)activePrimaryGunController->scrollTextureModelB);
    }

    if (playerState->primaryGunDispatchRequested == 0) {
        return;
    }

    PlayerGunFireSlot *activeFireSlot = 0;
    SelectPrimaryGunFireOriginAndSlot(saveState, &activeFireSlot);
    playerState->primaryGunDispatchRequested = 0;

    if (activePrimaryGunController->ammoOrCharge > 0.0f) {
        if (activePrimaryGunController->ammoOrCharge != kPlayerAltAmmoDisabledSentinel) {
            activePrimaryGunController->ammoOrCharge -= 1.0f;
            if (activePrimaryGunController->ammoOrCharge < 0.0f) {
                activePrimaryGunController->ammoOrCharge = 0.0f;
            }
        }

        AltGunEnsureAuxEffectActive(saveState, activePrimaryGunController,
                                    &playerState->primaryFireOrigin);

        if ((activePrimaryGunController->flags & 1) != 0 &&
            activePrimaryGunController->attachNodePrimary != 0 &&
            saveState->primaryModalState->masterModalData->masterType != kPlayerMasterTypeSub) {
            activeFireSlot->offset = 1.5f;
        }

        if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
            ++g_HudSensorTracker.primaryGunDispatchCount;
        }
    }

    if (activePrimaryGunController->ammoOrCharge == 0.0f) {
        HandlePrimaryWeaponVariantToggleInput(0x31);
    }
}

// Reimplements 0x439ba0: Player::TickAltGunRuntimeState
// (D:\Proj\Battlesport\player.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL TickAltGunRuntimeState(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const activeAltGunController =
        playerState->activeAltGunController;

    OptCatalog::SetPendingSpawnTargetOverrides(&playerState->progressTargetCount,
                                               playerState->progressTargetSlots);

    if (playerState->altGunTransitionState == 1 ||
        (playerState->altGunTransitionState & 0x180) != 0) {
        if (playerState->altGunDispatchRequested != 0) {
            ProcessAltGunFireDispatchRequest(saveState);
        } else if (playerState->altGunFireHeldFlag != 0) {
            playerState->altGunFireHeldFlag = 0;
            OptCatalog::DeactivateTrailRuntimeState(activeAltGunController->trailRuntimeState);
        }

        if ((playerState->altGunTransitionState & 0x180) != 0) {
            TickAltGunTetherCleanup(saveState);
        }

        TickAltGunTriggerProcessCleanup(saveState);
    } else {
        TickAltGunTransitionAnimation(saveState);
    }

    OptCatalog::SetPendingSpawnTargetOverrides(0, 0);

    if (saveState != (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        return;
    }

    if (TickAltGunLocalAmmoState(saveState) == 0) {
        return;
    }

    TickAltGunLocalSlotAndPrimaryState(saveState);
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
