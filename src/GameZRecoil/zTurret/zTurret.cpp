#include "GameZRecoil/zTurret/zTurret.h"

#include "Battlesport/player.h"
#include "Battlesport/GameNet.h"
#include "GameZRecoil/Time/Time.h"
#include "OptCatalog.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zSound/zSound.h"

#include <math.h>
#include <string.h>


extern "C" {
zClass_NodePartial *g_zTurret_CallbackNode = 0;
zReader::Node *g_zTurret_LoadedDefRoot = 0;
zEffectAnimEntry *g_zTurret_NapalmVehicleDestroyAnim = 0;
int g_zTurret_RuntimeCount = 0;
int g_zTurret_CallbackIterationActive = 0;
int g_zTurret_CallbackStartIndex = 0;
int g_zTurret_CallbackIterIndex = 0;
zTurret_Runtime *g_zTurret_RuntimeList[64] = {0};
}

namespace {
const int kPlayerLifecycleInactive = 4;
const int kPlayerMasterTypeSub = 2;
const unsigned int kZClassNodeActiveFlag = 0x04;
const int kZClassNodeObject3D = 6;
const unsigned int kOptCatalogFlagCreateTrail = 0x02;
const unsigned int kOptCatalogFlagUseNapalmVehicleDestroyAnim = 0x1000;
const unsigned int kOptCatalogFlagRemoveRuntimeOnTurretFire = 0x2000;
const char *const kZTurretSourceFile = "D:\\Proj\\Battlesport\\turret.cpp";

int zTurret_ReaderArrayCount(zReader::Node *node) {
    return node != 0 && node->type == zReader::ZRDR_NODE_ARRAY ? node->value.nodes[0].value.i32
                                                               : 0;
}

zReader::Node *zTurret_ReaderArraySlot(zReader::Node *node, int index) {
    if (node == 0 || node->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    if (index < 0 || index >= node->value.nodes[0].value.i32) {
        return 0;
    }

    return &node->value.nodes[index];
}

const char *zTurret_ReaderArrayString(zReader::Node *node, int index) {
    zReader::Node *const slot = zTurret_ReaderArraySlot(node, index);
    return slot != 0 && slot->type == zReader::ZRDR_NODE_STRING ? slot->value.str : 0;
}

char *zTurret_ReaderArrayMutableString(zReader::Node *node, int index) {
    zReader::Node *const slot = zTurret_ReaderArraySlot(node, index);
    return slot != 0 && slot->type == zReader::ZRDR_NODE_STRING ? slot->value.str : 0;
}

int zTurret_ReaderArrayInt(zReader::Node *node, int index) {
    zReader::Node *const slot = zTurret_ReaderArraySlot(node, index);
    return slot != 0 ? slot->value.i32 : 0;
}

float zTurret_ReaderArrayFloat(zReader::Node *node, int index) {
    zReader::Node *const slot = zTurret_ReaderArraySlot(node, index);
    if (slot == 0) {
        return 0.0f;
    }

    return slot->type == zReader::ZRDR_NODE_INT ? static_cast<float>(slot->value.i32)
                                                : slot->value.f32;
}

zClass_NodePartial *zTurret_FindChildByArrayString(zClass_NodePartial *root, zReader::Node *node,
                                                   int index) {
    const char *const name = zTurret_ReaderArrayString(node, index);
    return name != 0 ? zClass_Class::FindNodeRecursiveByName(root, name) : 0;
}

float zTurret_FloatFromBits(int bits) {
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

float zTurret_FastSqrtEstimate(float value) {
    unsigned int bits = 0;
    memcpy(&bits, &value, sizeof(bits));
    bits = (bits >> 1) + 0x1fc00000u;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

int zTurret_NodeIsActive(zClass_NodePartial *node) {
    return node != 0 && (node->flags & kZClassNodeActiveFlag) != 0;
}

void zTurret_DeactivateRuntimeInstance(zTurret_Runtime *runtime) {
    if (runtime->runtimeInstanceActive != 0) {
        runtime->runtimeInstanceActive = 0;
        OptCatalog::DeactivateTrailRuntimeState(runtime->trailRuntimeState);
    }
}

void zTurret_AddCandidateTarget(zTurret_Runtime *runtime, const zVec3 *candidatePos,
                                float *bestDistance, const zVec3 **bestTarget) {
    const float distance = static_cast<float>(fabs(runtime->worldPos.x - candidatePos->x) +
                                              fabs(runtime->worldPos.z - candidatePos->z));
    runtime->nearestTargetScore = distance;
    if (distance < *bestDistance) {
        *bestDistance = distance;
        *bestTarget = candidatePos;
    }
}

const zVec3 *zTurret_FindNearestTarget(zTurret_Runtime *runtime,
                                       zUtil_PlayerStateStorage *playerState,
                                       const zVec3 *playerFxOffsetWorld,
                                       float *bestDistance) {
    const zVec3 *const playerTarget =
        playerFxOffsetWorld != 0 ? playerFxOffsetWorld
                                 : (playerState != 0 ? &playerState->fxOffsetWorld : 0);
    const zVec3 *bestTarget = playerTarget != 0 ? playerTarget : &runtime->worldPos;
    *bestDistance = static_cast<float>(_HUGE);

    for (int i = 0; i < 8; ++i) {
        zClass_NodePartial *const targetNode = runtime->targetTypes[i];
        if (targetNode == 0) {
            break;
        }

        if ((targetNode->flags & kZClassNodeActiveFlag) == 0) {
            continue;
        }

        zMat4x3 *const matrix = (zMat4x3 *)zClass_Object3D::gwObject3DGetMatrixPtr(targetNode);
        const zVec3 *const targetPos = (const zVec3 *)(&matrix->posX);
        zTurret_AddCandidateTarget(runtime, targetPos, bestDistance, &bestTarget);
    }

    if (playerTarget != 0 && playerState != 0 &&
        VariantTag::CurrentAllowsId(runtime->turretNode->nodeType) != 0 &&
        playerState->lifecycleState != kPlayerLifecycleInactive) {
        zTurret_AddCandidateTarget(runtime, playerTarget, bestDistance, &bestTarget);
    }

    return bestTarget;
}
}

// Reimplements 0x436630: zTurret_Runtime::InitDefaults
// (D:\Proj\Battlesport\turret.cpp)
RECOIL_NOINLINE zTurret_Runtime *RECOIL_THISCALL zTurret_Runtime::InitDefaults() {
    flags = 0;
    scenePathVisible = 0;
    healthyNode = 0;
    worldPos.x = 0.0f;
    worldPos.y = 0.0f;
    worldPos.z = 0.0f;
    deactivateNode = 0;
    partBaseNode = 0;
    partBarrelNode = 0;
    firePointNode0 = 0;
    firePointNode1 = 0;
    fireEffectNode = 0;
    weaponBaseMoves = 0;
    hasMissileLock = 0;
    firePointIndex = 0;
    firePointCount = 0;
    firePointLocal[0].x = 0.0f;
    firePointLocal[0].y = 0.0f;
    firePointLocal[0].z = 0.0f;
    firePointLocal[1].x = 0.0f;
    firePointLocal[1].y = 0.0f;
    firePointLocal[1].z = 0.0f;
    forward.x = 0.0f;
    forward.y = 0.0f;
    forward.z = -1.0f;
    weaponAmmo = 50;
    detectionRange = 200.0f;
    damageModifier = 1.0f;
    fireAnimEntry = 0;
    nextFireTime = 1.0f;
    fireRateSeconds = 1.0f;
    spawnPos.x = 0.0f;
    spawnPos.y = 0.0f;
    spawnPos.z = -1.0f;
    fireDir.x = 0.0f;
    fireDir.y = 0.0f;
    fireDir.z = 0.0f;
    spawnVel.x = 0.0f;
    spawnVel.y = 0.0f;
    spawnVel.z = 0.0f;
    fireBurstTimer = 0.0f;
    fireBurstDuration = 0.0f;
    postBurstCooldown = 0.0f;
    fireDwellTime = 0.0f;
    fireDwellUntil = 0;
    trailRuntimeState = 0;
    enableLosCheck = 0;
    alwaysLookAtTarget = 0;
    healthCurrent = 100.0f;
    healthMax = 100.0f;
    damagePartNode = 0;
    intersectBvolEnabled = 1;
    destroyAnimEntry = 0;
    activateOnHitDamage = 0.0f;
    activateOnHitTimeout = static_cast<float>(_HUGE);
    for (int i = 0; i < 8; ++i) {
        targetTypes[i] = 0;
    }
    unknown_174[0] = 0;
    unknown_174[1] = 0;
    unknown_174[2] = 0;
    unknown_174[3] = 0;
    unknown_174[4] = 0;
    unknown_174[5] = 0;
    unknown_174[6] = 0;
    unknown_174[7] = 0;
    unknown_174[8] = 0;
    unknown_174[9] = 0;
    unknown_174[10] = 0;
    unknown_174[11] = 0;
    weaponCatalogEntry = 0;
    isFiring = 0;
    return this;
}

// Reimplements 0x4367a0: zTurret_Runtime::InitFromReaderNode
// (D:\Proj\Battlesport\turret.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL zTurret_Runtime::InitFromReaderNode(
    zClass_NodePartial *worldNode, zClass_NodePartial *turretWorldNode,
    zEffectAnimEntry *defaultDestroyAnim, zReader::Node *readerNode) {
    (void)worldNode;

    turretNode = turretWorldNode;
    zEffectAnimEntry *const namedDestroyAnim = zEffectAnim::FindEntryByName(turretWorldNode->name);
    if (namedDestroyAnim != 0) {
        defaultDestroyAnim = namedDestroyAnim;
    }

    zReader::Node *node = zReader_GetNamedNode(readerNode, "PARTS");
    if (node != 0) {
        healthyNode = zClass_Class::FindNodeRecursiveByName(turretWorldNode, "healthy");
        if (healthyNode != 0) {
            flags = 1;
            scenePathVisible = 2;
            const int count = zTurret_ReaderArrayCount(node);
            if (count == 5) {
                firePointCount = 2;
                partBaseNode = zTurret_FindChildByArrayString(turretWorldNode, node, 1);
                partBarrelNode = zTurret_FindChildByArrayString(turretWorldNode, node, 2);
                firePointNode0 = zTurret_FindChildByArrayString(turretWorldNode, node, 3);
                firePointNode1 = zTurret_FindChildByArrayString(turretWorldNode, node, 4);
            } else if (count == 4) {
                firePointCount = 1;
                partBaseNode = zTurret_FindChildByArrayString(turretWorldNode, node, 1);
                partBarrelNode = zTurret_FindChildByArrayString(turretWorldNode, node, 2);
                firePointNode0 = zTurret_FindChildByArrayString(turretWorldNode, node, 3);
            } else if (count == 3) {
                firePointCount = 1;
                partBarrelNode = zTurret_FindChildByArrayString(turretWorldNode, node, 1);
                firePointNode0 = zTurret_FindChildByArrayString(turretWorldNode, node, 2);
            }
        }
    }

    node = zReader_GetNamedNode(readerNode, "DEACTIVATE");
    if (node != 0) {
        deactivateNode = zClass::FindByTypeAndName(kZClassNodeObject3D,
                                                   zTurret_ReaderArrayString(node, 1));
        for (int i = 2; i < zTurret_ReaderArrayCount(node); ++i) {
            deactivateNode = zClass_Class::FindNodeRecursiveByName(
                deactivateNode, zTurret_ReaderArrayString(node, i));
        }
    }

    node = zReader_GetNamedNode(readerNode, "EFFECT");
    if (node != 0) {
        fireEffectNode = zTurret_FindChildByArrayString(turretWorldNode, node, 1);
        fireEffectDurationSec = zTurret_ReaderArrayFloat(node, 2);
        if (fireEffectNode != 0) {
            zModel::SetDiTextureWorldPerMeter((zDiPartial *)(fireEffectNode->classData), 1, 10.0f,
                                              0);
        }
    }

    node = zReader_GetNamedNode(readerNode, "ACTIVATE_ON_HIT");
    if (node != 0) {
        activateOnHitTimeout = 0.0f;
        activateOnHitDamage = zTurret_ReaderArrayFloat(node, 1);
    }

    node = zReader_GetNamedNode(readerNode, "ALWAYS_LOOK_AT");
    if (node != 0) {
        alwaysLookAtTarget = zTurret_ReaderArrayInt(node, 1);
    }

    node = zReader_GetNamedNode(readerNode, "DAMAGE_PART");
    if (node != 0) {
        damagePartNode = zTurret_FindChildByArrayString(turretWorldNode, node, 1);
    }

    node = zReader_GetNamedNode(readerNode, "DESTROY_ANIM");
    if (node != 0) {
        destroyAnimEntry = zEffectAnim::FindEntryByName(zTurret_ReaderArrayString(node, 1));
    }

    node = zReader_GetNamedNode(readerNode, "FIRE_ANIM");
    if (node != 0) {
        fireAnimEntry = zEffectAnim::FindEntryByName(zTurret_ReaderArrayString(node, 1));
    }

    node = zReader_GetNamedNode(readerNode, "HEALTH");
    if (node != 0) {
        healthCurrent = zTurret_ReaderArrayFloat(node, 1);
        healthMax = healthCurrent;
    }

    node = zReader_GetNamedNode(readerNode, "INTERSECT_BVOL");
    if (node != 0) {
        intersectBvolEnabled = zTurret_ReaderArrayInt(node, 1);
    }

    node = zReader_GetNamedNode(readerNode, "LOS");
    if (node != 0) {
        enableLosCheck = zTurret_ReaderArrayInt(node, 1);
    }

    zReader::Node *parentNode = zReader_GetNamedNode(readerNode, "SOUNDS");
    if (parentNode != 0) {
        node = zReader_GetNamedNode(parentNode, "ON");
        if (node != 0) {
            zSnd::FindSampleByName(zTurret_ReaderArrayString(node, 1));
        }
        node = zReader_GetNamedNode(parentNode, "START");
        if (node != 0) {
            zSnd::FindSampleByName(zTurret_ReaderArrayString(node, 1));
        }
        node = zReader_GetNamedNode(parentNode, "STOP");
        if (node != 0) {
            zSnd::FindSampleByName(zTurret_ReaderArrayString(node, 1));
        }
    }

    parentNode = zReader_GetNamedNode(readerNode, "WEAPON");
    if (parentNode != 0) {
        node = zReader_GetNamedNode(parentNode, "NAME");
        if (node != 0) {
            weaponCatalogEntry = OptCatalog::FindEntryByName(zTurret_ReaderArrayString(node, 1));
        }
        node = zReader_GetNamedNode(parentNode, "AMMO");
        if (node != 0) {
            weaponAmmo = zTurret_ReaderArrayInt(node, 1);
        }
        node = zReader_GetNamedNode(parentNode, "BASE_MOVES");
        if (node != 0) {
            weaponBaseMoves = zTurret_ReaderArrayInt(node, 1);
        }
        node = zReader_GetNamedNode(parentNode, "DAMAGE_MODIFIER");
        if (node != 0) {
            damageModifier = zTurret_ReaderArrayFloat(node, 1);
        }
        node = zReader_GetNamedNode(parentNode, "DETECTION_RANGE");
        if (node != 0) {
            detectionRange = zTurret_ReaderArrayFloat(node, 1);
        }
        node = zReader_GetNamedNode(parentNode, "FIRE_DWELL");
        if (node != 0) {
            fireDwellTime = zTurret_ReaderArrayFloat(node, 1);
        }
        node = zReader_GetNamedNode(parentNode, "FIRE_RATE");
        if (node != 0) {
            fireRateSeconds = zTurret_ReaderArrayFloat(node, 1);
        }
        node = zReader_GetNamedNode(parentNode, "FIRE_LIMITS");
        if (node != 0) {
            fireBurstDuration = zTurret_ReaderArrayFloat(node, 1);
            postBurstCooldown = zTurret_ReaderArrayFloat(node, 2);
        }
    }

    node = zReader_GetNamedNode(readerNode, "TARGETS");
    if (node != 0) {
        for (int i = 1; i < zTurret_ReaderArrayCount(node); ++i) {
            targetTypes[i - 1] =
                zClass::FindByTypeAndName(kZClassNodeObject3D, zTurret_ReaderArrayString(node, i));
        }
    }

    if (zReader_GetNamedNode(readerNode, "MSL_LOCK") != 0) {
        hasMissileLock = 1;
        HudUiMgrSensor::TrackList_Add(HUD_SENSOR_TRACK_KIND_TURRET, this);
    }

    gwNode::GetWorldPosition(turretNode, &worldPos);
    zClass_Object3D::gwObject3DGetRotation(turretNode, &forward.x, &forward.y, &forward.z);
    zMath::Vec3Normalize(&forward);
    firePos = worldPos;

    if (partBaseNode != 0) {
        partBaseMatrix = (zMat4x3 *)zClass_Object3D::gwObject3DGetMatrixPtr(partBaseNode);
        firePos.y += partBaseMatrix->posY;
    }

    if (partBarrelNode != 0) {
        partBarrelMatrix = (zMat4x3 *)zClass_Object3D::gwObject3DGetMatrixPtr(partBarrelNode);
        firePos.y += partBarrelMatrix->posY;
    }

    if (firePointNode0 != 0) {
        zClass_Object3D::gwObject3DGetPosition(firePointNode0, &firePointLocal[0].x,
                                               &firePointLocal[0].y, &firePointLocal[0].z);
        zClass_Class::RemoveChild(partBarrelNode, firePointNode0);
        zClass_Util::DestroyNodeRecursive(firePointNode0);
        firePointNode0 = 0;
        firePos.y += firePointLocal[0].y;
    }

    if (firePointNode1 != 0) {
        zClass_Object3D::gwObject3DGetPosition(firePointNode1, &firePointLocal[1].x,
                                               &firePointLocal[1].y, &firePointLocal[1].z);
        zClass_Class::RemoveChild(partBarrelNode, firePointNode1);
        zClass_Util::DestroyNodeRecursive(firePointNode1);
        firePointNode1 = 0;
        firePos.y += (firePointLocal[1].y - firePointLocal[0].y) * 0.5f;
    }

    if (fireEffectNode != 0) {
        zClass_Class::gwNodeSetActive(fireEffectNode, 0);
    }

    zClass_Class::gwNodeSetCellPickable(turretNode, 0);
    if (damagePartNode == 0 && intersectBvolEnabled != 0) {
        zClass_Class::gwNodeSetPickable(healthyNode, 1);
    }

    if ((weaponCatalogEntry->flags & kOptCatalogFlagCreateTrail) != 0) {
        trailRuntimeState = OptCatalog::CreateTrailRuntimeState(
            weaponCatalogEntry, turretNode, 0, partBarrelNode, &fireDir, &spawnPos, 2);
        fireRateSeconds = fireBurstDuration;
    }

    if (destroyAnimEntry == 0) {
        destroyAnimEntry = defaultDestroyAnim;
    }

    if (destroyAnimEntry != 0) {
        zEffect_Anim::NodeActionCallback(destroyAnimEntry, turretNode);
        if (healthCurrent > 0.0f) {
            zClass_Node::SetDamageHitCallback(this, healthyNode, (void *)zTurret_Runtime::OnDamage);
        }
    }

    zClass_NodePartial *const destroyedNode =
        zClass_Class::FindNodeRecursiveByName(turretNode, "destroyed");
    if (destroyedNode != 0) {
        zClass_Class::gwNodeSetRaycastable(destroyedNode, 0);
    }

    strncmp(turretNode->name, "hel_", 4);
}

// Reimplements 0x437430: zTurret_Runtime::UpdateFirePositionFromParts
// (D:\Proj\Battlesport\turret.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL zTurret_Runtime::UpdateFirePositionFromParts() {
    firePos = worldPos;

    if (partBaseNode != 0) {
        firePos.y += partBaseMatrix->posY;
    }

    if (partBarrelNode != 0) {
        firePos.y += partBarrelMatrix->posY;
    }

    if (firePointNode0 != 0) {
        firePos.y += firePointLocal[0].y;
        return;
    }

    if (firePointNode1 != 0) {
        firePos.y += (firePointLocal[1].y - firePointLocal[0].y) * 0.5f;
    }
}

// Reimplements 0x4374a0: zTurret_Runtime::UpdateAimAndPartMatrices
// (D:\Proj\Battlesport\turret.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL
zTurret_Runtime::UpdateAimAndPartMatrices(const zVec3 *targetPos) {
    zVec3 localAimDir = {partBarrelMatrix->posX, partBarrelMatrix->posY, partBarrelMatrix->posZ};

    zMat4x3 slotBuffer = {0};
    zMath::MatStackPushPtr((float *)(&slotBuffer));
    zMath::MatLoadIdentity();
    gwNode::BuildNodeToAncestorMatrix(turretNode, 3);
    zMath::MatTransformPointBatchInPlace(&localAimDir, 1);

    localAimDir.x = targetPos->x - localAimDir.x;
    localAimDir.y = targetPos->y - localAimDir.y;
    localAimDir.z = targetPos->z - localAimDir.z;
    zMath::Vec3Normalize(&localAimDir);
    zMath::Vec3ArrayTransformDirection(&localAimDir, 1);
    zMath::MatStackPopPtr();

    if (alwaysLookAtTarget == 0) {
        const float alignment = localAimDir.x * forward.x + localAimDir.y * forward.y +
                                localAimDir.z * forward.z;
        if (alignment > 0.89) {
            isFiring = 1;
        } else if (fireDwellTime == 0.0f) {
            isFiring = 0;
        }
    }

    const int forwardBlendBits =
        static_cast<int>(g_FrameDeltaTimeSec * -3.0f * 12102200.0f) + 0x3f800000;
    const float oldForwardWeight = zTurret_FloatFromBits(forwardBlendBits);
    const float newForwardWeight = 1.0f - oldForwardWeight;
    forward.x = oldForwardWeight * forward.x + newForwardWeight * localAimDir.x;
    forward.y = oldForwardWeight * forward.y + newForwardWeight * localAimDir.y;
    forward.z = oldForwardWeight * forward.z + newForwardWeight * localAimDir.z;
    zMath::Vec3Normalize(&forward);

    localAimDir = forward;
    const float horizontalLen =
        zTurret_FastSqrtEstimate(localAimDir.x * localAimDir.x + localAimDir.z * localAimDir.z);

    float yawX = 0.0f;
    float yawZ = 1.0f;
    if (horizontalLen != 0.0f) {
        yawX = -(localAimDir.x / horizontalLen);
        yawZ = -(localAimDir.z / horizontalLen);
    }

    if (partBaseNode != 0) {
        partBaseMatrix->xx = yawZ;
        partBaseMatrix->xz = -yawX;
        partBaseMatrix->zx = yawX;
        partBaseMatrix->zz = yawZ;
        zClass_Object3D::gwObject3DSetMatrix(partBaseNode, (float *)partBaseMatrix);

        partBarrelMatrix->yy = horizontalLen;
        partBarrelMatrix->yz = localAimDir.y;
        partBarrelMatrix->zy = -localAimDir.y;
        partBarrelMatrix->zz = horizontalLen;
        zClass_Object3D::gwObject3DSetMatrix(partBarrelNode, (float *)partBarrelMatrix);
        return;
    }

    partBarrelMatrix->xx = yawZ;
    partBarrelMatrix->xz = -yawX;
    partBarrelMatrix->yx = yawX * localAimDir.y;
    partBarrelMatrix->yy = horizontalLen;
    partBarrelMatrix->yz = yawZ * localAimDir.y;
    partBarrelMatrix->zx = yawX * horizontalLen;
    partBarrelMatrix->zy = -localAimDir.y;
    partBarrelMatrix->zz = yawZ * horizontalLen;
    zClass_Object3D::gwObject3DSetMatrix(partBarrelNode, (float *)partBarrelMatrix);
}

// Reimplements 0x437730: zTurret_Runtime::SelectFirePointAndAimAtTarget
// (D:\Proj\Battlesport\turret.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL
zTurret_Runtime::SelectFirePointAndAimAtTarget(const zVec3 *targetPos) {
    if (firePointCount > 1) {
        ++firePointIndex;
        if (firePointIndex >= firePointCount) {
            firePointIndex = 0;
        }
        spawnPos = firePointLocal[firePointIndex];
    } else {
        spawnPos = firePointLocal[0];
    }

    zMat4x3 slotBuffer = {0};
    zMath::MatStackPushPtr((float *)(&slotBuffer));
    zMath::MatLoadIdentity();
    gwNode::BuildNodeToAncestorMatrix(partBarrelNode, 3);
    zMath::MatTransformPointBatchInPlace(&spawnPos, 1);

    fireDir.x = targetPos->x - spawnPos.x;
    fireDir.y = targetPos->y - spawnPos.y;
    fireDir.z = targetPos->z - spawnPos.z;
    zMath::Vec3Normalize(&fireDir);
    zMath::MatStackPopPtr();
}

// Reimplements 0x437820: zTurret_Runtime::FireWeapon
// (D:\Proj\Battlesport\turret.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL zTurret_Runtime::FireWeapon() {
    if (trailRuntimeState != 0) {
        if (runtimeInstanceActive == 0) {
            runtimeInstanceActive = 1;
            OptCatalog::SetPendingSpawnTargetOverrides(&runtimeAimPending, &runtimeAimTarget);
            OptCatalog::ActivateTrailRuntimeState(trailRuntimeState, 0);
            OptCatalog::SetPendingSpawnTargetOverrides(0, 0);
            nextFireTime = g_Time_AccumulatedTimeSec + fireBurstDuration + postBurstCooldown;
        }
        return;
    }

    if (weaponCatalogEntry->gravity != 0.0f) {
        const float pitch = OptCatalog::ComputeAimPitchForTarget(
            weaponCatalogEntry, &spawnPos, 0, runtimeAimTarget.targetPos, &nearestTargetScore);
        if (pitch != -1.0f) {
            Player::ApplyAimPitchToDirection(&fireDir, pitch);
        }
    }

    OptCatalog::SetPendingSpawnTargetOverrides(&runtimeAimPending, &runtimeAimTarget);
    if (weaponBaseMoves != 0) {
        zClass_Class::gwNodeSetRaycastable(turretNode->listA[0], 0);
    }

    zUtil_PlayerStateStorage *const playerState =
        (zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState);
    g_OptCatalogNextSpawnScale = damageModifier;
    OptCatalog::AllocRuntimeInstance(weaponCatalogEntry, turretNode, &playerState->variantTag,
                                     &spawnPos, &fireDir, &spawnVel, 0, 0);

    if (weaponBaseMoves != 0) {
        zClass_Class::gwNodeSetRaycastable(turretNode->listA[0], 1);
    }

    OptCatalog::SetPendingSpawnTargetOverrides(0, 0);
    nextFireTime = g_Time_AccumulatedTimeSec + fireRateSeconds;
    UpdateFireBurstTimer(fireRateSeconds);
}

// Reimplements 0x437990: zTurret_Runtime::UpdateFireBurstTimer
// (D:\Proj\Battlesport\turret.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL zTurret_Runtime::UpdateFireBurstTimer(float deltaTime) {
    if (fireBurstDuration == 0.0f) {
        return;
    }

    fireBurstTimer -= deltaTime;
    if (fireBurstTimer <= 0.0f) {
        isFiring = 0;
        fireBurstTimer = fireBurstDuration;
        nextFireTime = g_Time_AccumulatedTimeSec + postBurstCooldown;
    }
}

// Reimplements 0x436e40: zTurret_Runtime::Tick
// (D:\Proj\Battlesport\turret.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL zTurret_Runtime::Tick(const zVec3 *playerFxOffsetWorld) {
    if (!zTurret_NodeIsActive(healthyNode) || !zTurret_NodeIsActive(turretNode) ||
        (deactivateNode != 0 && !zTurret_NodeIsActive(deactivateNode))) {
        if (fireEffectNode != 0 && !zTurret_NodeIsActive(fireEffectNode)) {
            zClass_Class::gwNodeSetActive(fireEffectNode, 0);
        }
        zTurret_DeactivateRuntimeInstance(this);
        return;
    }

    if (activateOnHitTimeout < g_Time_AccumulatedTimeSec) {
        return;
    }

    zUtil_PlayerStateStorage *const playerState =
        (zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState);
    const int removeRuntimeOnFire =
        (weaponCatalogEntry->flags & kOptCatalogFlagRemoveRuntimeOnTurretFire) != 0;

    if (removeRuntimeOnFire != 0 && weaponBaseMoves != 0) {
        gwNode::GetWorldPosition(turretNode, &worldPos);
    } else if (removeRuntimeOnFire == 0 && fireEffectNode != 0 &&
               zTurret_NodeIsActive(fireEffectNode) &&
               g_Time_AccumulatedTimeSec < nextFireTime) {
        zModel_Instance_UpdateScrollingTexturesIfNeeded(
            (zModel_InstancePartial *)(fireEffectNode->userDataOrDiRef));
    }

    float nearestDistance = 0.0f;
    const zVec3 *const targetPos =
        zTurret_FindNearestTarget(this, playerState, playerFxOffsetWorld, &nearestDistance);
    const int targetInRange = nearestDistance < detectionRange;

    if (targetInRange == 0) {
        isFiring = 0;
        if (weaponBaseMoves != 0 && hasMissileLock != 0) {
            UpdateFirePositionFromParts();
        }
    } else {
        if (g_zTurret_CallbackIterationActive != 0) {
            g_zTurret_CallbackIterationActive = 0;
            g_zTurret_CallbackStartIndex = g_zTurret_CallbackIterIndex;
        }

        if (weaponBaseMoves != 0) {
            UpdateFirePositionFromParts();
        }

        const int losDirection = enableLosCheck == 1 ? 2 : 1;
        if (Player::HasLineOfSightFromLocalPlayerFxOffset(healthyNode, &firePos, losDirection) !=
            0) {
            isFiring = 1;
            runtimeAimPending = 1;
            runtimeAimTarget.targetPos = (zVec3 *)targetPos;
            if (fireDwellTime != 0.0f) {
                fireDwellUntil = g_Time_AccumulatedTimeSec + fireDwellTime;
            }
        } else if (fireDwellTime == 0.0f ||
                   g_Time_AccumulatedTimeSec >= fireDwellUntil) {
            isFiring = 0;
        }

        if (removeRuntimeOnFire != 0 && isFiring != 0) {
            zEffectAnim::SetVelocity_Thunk(destroyAnimEntry, turretNode, 0.0f, 0.0f, 0.0f);
            OptCatalog::RemoveRuntimeInstance(weaponCatalogEntry, &worldPos, 0);
            return;
        }
    }

    if (isFiring != 0) {
        UpdateAimAndPartMatrices(targetPos);
        if (isFiring != 0 && g_Time_AccumulatedTimeSec >= nextFireTime) {
            if (fireEffectNode != 0) {
                if (!zTurret_NodeIsActive(fireEffectNode)) {
                    zClass_Class::gwNodeSetActive(fireEffectNode, 1);
                    nextFireTime = g_Time_AccumulatedTimeSec + fireEffectDurationSec;
                    return;
                }

                zClass_Class::gwNodeSetActive(fireEffectNode, 0);
            }

            SelectFirePointAndAimAtTarget(targetPos);
            if (fireAnimEntry == 0) {
                FireWeapon();
            } else {
                zEffectAnimEntry::SetOnStateDoneCallback(
                    fireAnimEntry, (void *)zTurret_Runtime::FireWeaponCallback, this);
                zEffectAnim::SetVelocity_Thunk(fireAnimEntry, turretNode, 0.0f, 0.0f, 0.0f);
            }
        }
    }

    if (runtimeInstanceActive != 0) {
        UpdateFireBurstTimer(g_FrameDeltaTimeSec);
        if (weaponBaseMoves != 0 && isFiring != 0) {
            SelectFirePointAndAimAtTarget(targetPos);
        }
    }

    if (isFiring == 0) {
        if (alwaysLookAtTarget != 0) {
            UpdateAimAndPartMatrices(targetPos);
        }

        zTurret_DeactivateRuntimeInstance(this);
    }

    if (fireBurstTimer != fireBurstDuration) {
        fireBurstTimer += g_FrameDeltaTimeSec;
        if (fireBurstTimer >= fireBurstDuration) {
            fireBurstTimer = fireBurstDuration;
            nextFireTime = g_Time_AccumulatedTimeSec;
        }
    }
}

// Reimplements 0x437e50: zTurret_Runtime::FireWeaponCallback
// (D:\Proj\Battlesport\turret.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL zTurret_Runtime::FireWeaponCallback(
    zEffectAnimEntry *entry, zTurret_Runtime *self, int eventCode) {
    (void)entry;
    (void)eventCode;
    self->FireWeapon();
}

// Reimplements 0x4379f0: zTurret_Runtime::ApplyDamageAndHandleDestruction
// (D:\Proj\Battlesport\turret.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL zTurret_Runtime::ApplyDamageAndHandleDestruction(
    float damageAmount, OptCatalogEntryDef *entry, OptCatalogHitEventPartial *hitEvent) {
    if (activateOnHitDamage != 0.0f) {
        activateOnHitTimeout = g_Time_AccumulatedTimeSec + activateOnHitDamage;
    }

    if (damagePartNode != 0 && hitEvent->hitNode != damagePartNode) {
        return 0;
    }

    healthCurrent -= damageAmount;
    if (healthCurrent <= 0.0f) {
        zEffectAnimEntry *destroyAnim = g_zTurret_NapalmVehicleDestroyAnim;
        if ((entry->flags & kOptCatalogFlagUseNapalmVehicleDestroyAnim) == 0) {
            destroyAnim = destroyAnimEntry;
        }

        zEffectAnim::SetVelocity_Thunk(destroyAnim, turretNode, 0.0f, 0.0f, 0.0f);

        if (runtimeInstanceActive != 0) {
            OptCatalogTrailRuntimeState *const trailState = trailRuntimeState;
            runtimeInstanceActive = 0;
            OptCatalog::DeactivateTrailRuntimeState(trailState);
        }

        return 1;
    }

    return 0;
}

// Reimplements 0x437d60: zTurret_Runtime::OnDamage
// (D:\Proj\Battlesport\turret.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL zTurret_Runtime::OnDamage(
    zTurret_Runtime *self, OptCatalogEntryDef *entry, OptCatalogHitEventPartial *hitEvent,
    float damageAmount) {
    if (self->ApplyDamageAndHandleDestruction(damageAmount, entry, hitEvent) != 0) {
        OptCatalog::SetDamageContext(1, 0);
        Player::AddScaledHudCounterValue(self->healthMax);
    } else {
        DamageFeedback::SetIntensityScalar(self->healthCurrent / self->healthMax);
    }

    return 0;
}

// Reimplements 0x436e00: zTurret_Runtime::Shutdown
RECOIL_NOINLINE int RECOIL_THISCALL zTurret_Runtime::Shutdown() {
    if (trailRuntimeState != 0) {
        OptCatalog::FreeTrailRuntimeStateStorage(trailRuntimeState);
    }

    return zClass_Node::ClearDamageHandler(healthyNode);
}

// Reimplements 0x436e20: zTurret_Runtime::HasActiveNode
// (D:\Proj\Battlesport\turret.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL zTurret_Runtime::HasActiveNode() {
    if (flags != 0 && (turretNode->flags & 0x04) != 0) {
        return 1;
    }

    return 0;
}

namespace zTurret_System {
// Reimplements 0x437aa0: zTurret_System::ResetIterationState
RECOIL_NOINLINE int RECOIL_CDECL ResetIterationState() {
    g_zTurret_RuntimeCount = 0;
    g_zTurret_CallbackStartIndex = 0;
    return 0;
}

// Reimplements 0x437ac0: zTurret_System::LoadDefinitionsFromPath
// (D:\Proj\Battlesport\turret.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL LoadDefinitionsFromPath(zClass_NodePartial *worldNode,
                                                            const char *path) {
    if (zOpt::GetNetworkEnabled() != 0) {
        return -1;
    }

    zReader::Node *const rootNode = zReader::LoadNodeFromPath(path, 0, 0);
    if (rootNode == 0) {
        zError::ReportOld(0x200, kZTurretSourceFile, 0x4ce, "Failed to read %s", path);
        return -1;
    }

    g_zTurret_LoadedDefRoot = rootNode;

    zEffectAnimEntry *defaultDestroyAnim = 0;
    zReader::Node *destroyAnimNode = zReader_GetNamedNode(rootNode, "DESTROY_ANIM");
    if (destroyAnimNode != 0) {
        defaultDestroyAnim = zEffectAnim::FindEntryByName(zTurret_ReaderArrayString(
            destroyAnimNode, 1));
    }

    zEffectAnimEntry *const napalmDestroyAnim =
        zEffectAnim::FindEntryByName("napalm_vehicle");
    zReader::Node *const turretListNode = zReader_GetNamedNode(rootNode, "TURRET");
    if (turretListNode != 0) {
        int index = 1;
        while (index < zTurret_ReaderArrayCount(turretListNode)) {
            char *const turretName = zTurret_ReaderArrayMutableString(turretListNode, index);
            zReader::Node *const readerNode =
                turretName != 0 ? zReader_GetNamedNode(turretListNode, turretName) : 0;
            if (readerNode != 0) {
                char *searchName = zUtil_ZRDR_InitWildcardPath(turretName);
                while (searchName != 0) {
                    zClass_NodePartial *const turretWorldNode =
                        zClass::FindByTypeAndName(kZClassNodeObject3D, searchName);
                    if (turretWorldNode != 0) {
                        zTurret_Runtime *runtime =
                            static_cast<zTurret_Runtime *>(::operator new(sizeof(zTurret_Runtime)));
                        if (runtime != 0) {
                            runtime = runtime->InitDefaults();
                        }

                        runtime->InitFromReaderNode(worldNode, turretWorldNode, defaultDestroyAnim,
                                                    readerNode);
                        g_zTurret_NapalmVehicleDestroyAnim = napalmDestroyAnim;
                        g_zTurret_RuntimeList[g_zTurret_RuntimeCount] = runtime;
                        ++g_zTurret_RuntimeCount;
                    }

                    searchName = zUtil_ZRDR_NextWildcardPath();
                }
            }

            index += 2;
        }
    }

    g_zTurret_CallbackNode = zClass_Object3D::gwObject3DInit();
    zClass_Class::gwNodeSetActionCallback(g_zTurret_CallbackNode,
                                          (void *)zTurret_System::TickAllRuntimesRoundRobin);
    return 0;
}

// Reimplements 0x437ca0: zTurret_System::TickAllRuntimesRoundRobin
// (D:\Proj\Battlesport\turret.cpp)
RECOIL_NOINLINE void RECOIL_CDECL TickAllRuntimesRoundRobin() {
    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)g_GameStateOrMapTable;
    if (saveState->primaryModalState->masterModalData->masterType == kPlayerMasterTypeSub) {
        return;
    }

    int runtimeCount = g_zTurret_RuntimeCount;
    int index = g_zTurret_CallbackStartIndex;
    int runtimeScanCount = 0;

    g_zTurret_CallbackIterationActive = 1;
    g_zTurret_CallbackIterIndex = index;

    while (runtimeScanCount < runtimeCount) {
        if (index >= runtimeCount) {
            index = 0;
            g_zTurret_CallbackIterIndex = 0;
        }

        zTurret_Runtime *const runtime = g_zTurret_RuntimeList[index];
        if (runtime->flags != 0) {
            runtime->Tick(&g_LocalPlayerSaveState->playerState->fxOffsetWorld);
            runtimeCount = g_zTurret_RuntimeCount;
            index = g_zTurret_CallbackIterIndex;
        }

        ++runtimeScanCount;
        ++index;
        g_zTurret_CallbackIterIndex = index;
    }

    ++g_zTurret_CallbackStartIndex;
    if (g_zTurret_CallbackStartIndex >= runtimeCount) {
        g_zTurret_CallbackStartIndex = 0;
    }
}

// Reimplements 0x437d40: zTurret_System::DisableTickCallback
// (D:\Proj\Battlesport\turret.cpp)
RECOIL_NOINLINE int RECOIL_CDECL DisableTickCallback() {
    return zClass_Class::gwNodeSetActionCallback(g_zTurret_CallbackNode, 0);
}

// Reimplements 0x437d50: zTurret_System::EnableTickCallback
// (D:\Proj\Battlesport\turret.cpp)
RECOIL_NOINLINE int RECOIL_CDECL EnableTickCallback() {
    return zClass_Class::gwNodeSetActionCallback(
        g_zTurret_CallbackNode, (void *)zTurret_System::TickAllRuntimesRoundRobin);
}

// Reimplements 0x437dc0: zTurret_System::FreeAllRuntimes
RECOIL_NOINLINE int RECOIL_CDECL FreeAllRuntimes() {
    for (int i = 0; i < g_zTurret_RuntimeCount; ++i) {
        zTurret_Runtime *const runtime = g_zTurret_RuntimeList[i];
        runtime->Shutdown();
        ::operator delete(runtime);
        g_zTurret_RuntimeList[i] = 0;
    }

    g_zTurret_RuntimeCount = 0;
    if (g_zTurret_LoadedDefRoot != 0) {
        zReader::FreeLoadedTree(g_zTurret_LoadedDefRoot);
        g_zTurret_LoadedDefRoot = 0;
    }

    if (g_zTurret_CallbackNode != 0) {
        zClass_Class::gwNodeSetActionCallback(g_zTurret_CallbackNode, 0);
        zClass_Object3D::DeleteNode(g_zTurret_CallbackNode);
        g_zTurret_CallbackNode = 0;
    }

    return 0;
}

// Reimplements 0x437ab0: zTurret_System::Shutdown
RECOIL_NOINLINE int RECOIL_CDECL Shutdown() {
    FreeAllRuntimes();
    return 0;
}
} // namespace zTurret_System
