#include "recoil/Mfc42Abi.h"
#include "Battlesport/turret.h"

#include "Battlesport/game_net.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "opt_catalog.h"

#include <math.h>
#include <string.h>

extern char g_HudCfgKey_Weapon[7];
extern char g_HudCfgKey_Ammo[5];

extern "C" {
/**
 * Data owner: zTurret writable runtime globals.
 * @recoil-anchor recoil:anchor:battlesport-turret-g-zturret-callbacknode
 * @recoil-artifact defines .data recoil:data:0x4f3fd0: g_zTurret_CallbackNode.
 * Purpose: Holds the action-callback node used to tick the turret runtime list.
 */
zClass_NodePartial *g_zTurret_CallbackNode = 0;
/**
 * Data owner: zTurret writable runtime globals.
 * @recoil-anchor recoil:anchor:battlesport-turret-g-zturret-loadeddefroot
 * @recoil-artifact defines .data recoil:data:0x4f3fd4: g_zTurret_LoadedDefRoot.
 * Purpose: Retains the loaded turret definition tree until turret shutdown.
 */
zReader::Node *g_zTurret_LoadedDefRoot = 0;
/**
 * Data owner: zTurret writable runtime globals.
 * @recoil-anchor recoil:anchor:battlesport-turret-g-zturret-napalmvehicledestroyanim
 * @recoil-artifact defines .data recoil:data:0x4f41ec: g_zTurret_NapalmVehicleDestroyAnim.
 * Purpose: Stores the napalm_vehicle destroy animation shared by turret destruction.
 */
zEffectAnimEntry *g_zTurret_NapalmVehicleDestroyAnim = 0;
/**
 * Data owner: zTurret writable runtime globals.
 * @recoil-anchor recoil:anchor:battlesport-turret-g-zturret-runtimecount
 * @recoil-artifact defines .data recoil:data:0x4f3fd8: g_zTurret_RuntimeCount.
 * Purpose: Counts active entries in g_zTurret_RuntimeList.
 */
int g_zTurret_RuntimeCount = 0;
/**
 * Data owner: zTurret writable runtime globals.
 * @recoil-anchor recoil:anchor:battlesport-turret-g-zturret-callbackiterationactive
 * @recoil-artifact defines .data recoil:data:0x4f3fdc: g_zTurret_CallbackIterationActive.
 * Purpose: Marks reentrant callback iteration so runtime removal can preserve scan state.
 */
int g_zTurret_CallbackIterationActive = 0;
/**
 * Data owner: zTurret writable runtime globals.
 * @recoil-anchor recoil:anchor:battlesport-turret-g-zturret-callbackstartindex
 * @recoil-artifact defines .data recoil:data:0x4f3fe0: g_zTurret_CallbackStartIndex.
 * Purpose: Stores the rotating round-robin start index for turret ticking.
 */
int g_zTurret_CallbackStartIndex = 0;
/**
 * Data owner: zTurret writable runtime globals.
 * @recoil-anchor recoil:anchor:battlesport-turret-g-zturret-callbackiterindex
 * @recoil-artifact defines .data recoil:data:0x4f3fe4: g_zTurret_CallbackIterIndex.
 * Purpose: Tracks the current round-robin scan index while callbacks are active.
 */
int g_zTurret_CallbackIterIndex = 0;
/**
 * Data owner: zTurret writable runtime globals.
 * @recoil-anchor recoil:anchor:battlesport-turret-g-zturret-runtimelist
 * @recoil-artifact defines .data recoil:data:0x4f3fe8: g_zTurret_RuntimeList.
 * Purpose: Stores the nine turret runtime pointers allocated from loaded definitions.
 */
zTurret_Runtime *g_zTurret_RuntimeList[9] = {0};
}

namespace {
const int kPlayerLifecycleInactive = 4;
const int kPlayerMasterTypeSub = 2;
const unsigned int kZClassNodeActiveFlag = 0x04;
const int kZClassNodeObject3D = 6;
const unsigned int kOptCatalogFlagCreateTrail = 0x02;
const unsigned int kOptCatalogFlagUseNapalmVehicleDestroyAnim = 0x1000;
const unsigned int kOptCatalogFlagRemoveRuntimeOnTurretFire = 0x2000;

} // namespace














namespace zTurret_System {






} // namespace zTurret_System

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-runtime-initdefaults
 * @recoil-artifact defines .text recoil:function:0x436630: zTurret_Runtime::InitDefaults.
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Applies the recovered default runtime state before turret field parsing.
 */
zTurret_Runtime * zTurret_Runtime::InitDefaults() {
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
    fireDir.x = 0.0f;
    fireDir.y = 0.0f;
    fireDir.z = -1.0f;
    spawnPos.x = 0.0f;
    spawnPos.y = 0.0f;
    spawnPos.z = 0.0f;
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
    activateOnHitTimeout = (float)(_HUGE);
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

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-runtime-initfromreadernode
 * @recoil-artifact defines .text recoil:function:0x4367a0: zTurret_Runtime::InitFromReaderNode.
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Parses a turret definition node and binds its scene parts, weapon, effects, and callbacks.
 */
void zTurret_Runtime::InitFromReaderNode(
    zClass_NodePartial *worldNode,
    zClass_NodePartial *turretWorldNode,
    zEffectAnimEntry *defaultDestroyAnim,
    zReader::Node *readerNode
) {
    (void)worldNode;

    turretNode = turretWorldNode;
    zEffectAnimEntry *const namedDestroyAnim = zEffectAnim::FindEntryByName(turretWorldNode->name);
    if (namedDestroyAnim != 0) {
        defaultDestroyAnim = namedDestroyAnim;
    }

    zReader::Node *node = zReader_GetNamedNode(
        readerNode,
        "PARTS"
    );
    if (node != 0) {
        healthyNode = zClass_Class::FindNodeRecursiveByName(
            turretWorldNode,
            g_Player_HealthySubNodeName
        );
        if (healthyNode != 0) {
            flags = 1;
            scenePathVisible = 2;
            const int count = node->value.nodes[0].value.i32;
            if (count == 5) {
                firePointCount = 2;
                partBaseNode = zClass_Class::FindNodeRecursiveByName(
                    turretWorldNode,
                    node->value.nodes[1].value.str
                );
                partBarrelNode = zClass_Class::FindNodeRecursiveByName(
                    turretWorldNode,
                    node->value.nodes[2].value.str
                );
                firePointNode0 = zClass_Class::FindNodeRecursiveByName(
                    turretWorldNode,
                    node->value.nodes[3].value.str
                );
                firePointNode1 = zClass_Class::FindNodeRecursiveByName(
                    turretWorldNode,
                    node->value.nodes[4].value.str
                );
            } else if (count == 4) {
                firePointCount = 1;
                partBaseNode = zClass_Class::FindNodeRecursiveByName(
                    turretWorldNode,
                    node->value.nodes[1].value.str
                );
                partBarrelNode = zClass_Class::FindNodeRecursiveByName(
                    turretWorldNode,
                    node->value.nodes[2].value.str
                );
                firePointNode0 = zClass_Class::FindNodeRecursiveByName(
                    turretWorldNode,
                    node->value.nodes[3].value.str
                );
            } else if (count == 3) {
                firePointCount = 1;
                partBarrelNode = zClass_Class::FindNodeRecursiveByName(
                    turretWorldNode,
                    node->value.nodes[1].value.str
                );
                firePointNode0 = zClass_Class::FindNodeRecursiveByName(
                    turretWorldNode,
                    node->value.nodes[2].value.str
                );
            }
        }
    }

    node = zReader_GetNamedNode(
        readerNode,
        "DEACTIVATE"
    );
    if (node != 0) {
        deactivateNode =
            zClass::FindByTypeAndName(
                kZClassNodeObject3D,
                node->value.nodes[1].value.str
            );
        for (int i = 2; i < node->value.nodes[0].value.i32; ++i) {
            deactivateNode = zClass_Class::FindNodeRecursiveByName(
                deactivateNode,
                node->value.nodes[i].value.str
            );
        }
    }

    node = zReader_GetNamedNode(
        readerNode,
        "EFFECT"
    );
    if (node != 0) {
        fireEffectNode = zClass_Class::FindNodeRecursiveByName(
            turretWorldNode,
            node->value.nodes[1].value.str
        );
        fireEffectDurationSec = node->value.nodes[2].type == zReader::ZRDR_NODE_INT
                                    ? (float)(node->value.nodes[2].value.i32)
                                    : node->value.nodes[2].value.f32;
        if (fireEffectNode != 0) {
            zModel::SetDiTextureWorldPerMeter(
                (zDiPartial *)(fireEffectNode->classData),
                1,
                10.0f,
                0
            );
        }
    }

    node = zReader_GetNamedNode(
        readerNode,
        "ACTIVATE_ON_HIT"
    );
    if (node != 0) {
        activateOnHitTimeout = 0.0f;
        activateOnHitDamage = node->value.nodes[1].type == zReader::ZRDR_NODE_INT
                                  ? (float)(node->value.nodes[1].value.i32)
                                  : node->value.nodes[1].value.f32;
    }

    node = zReader_GetNamedNode(
        readerNode,
        "ALWAYS_LOOK_AT"
    );
    if (node != 0) {
        alwaysLookAtTarget = node->value.nodes[1].value.i32;
    }

    node = zReader_GetNamedNode(
        readerNode,
        "DAMAGE_PART"
    );
    if (node != 0) {
        damagePartNode = zClass_Class::FindNodeRecursiveByName(
            turretWorldNode,
            node->value.nodes[1].value.str
        );
    }

    node = zReader_GetNamedNode(
        readerNode,
        "DESTROY_ANIM"
    );
    if (node != 0) {
        destroyAnimEntry = zEffectAnim::FindEntryByName(node->value.nodes[1].value.str);
    }

    node = zReader_GetNamedNode(
        readerNode,
        "FIRE_ANIM"
    );
    if (node != 0) {
        fireAnimEntry = zEffectAnim::FindEntryByName(node->value.nodes[1].value.str);
    }

    node = zReader_GetNamedNode(
        readerNode,
        "HEALTH"
    );
    if (node != 0) {
        healthCurrent = node->value.nodes[1].type == zReader::ZRDR_NODE_INT
                            ? (float)(node->value.nodes[1].value.i32)
                            : node->value.nodes[1].value.f32;
        healthMax = healthCurrent;
    }

    node = zReader_GetNamedNode(
        readerNode,
        "INTERSECT_BVOL"
    );
    if (node != 0) {
        intersectBvolEnabled = node->value.nodes[1].value.i32;
    }

    node = zReader_GetNamedNode(
        readerNode,
        "LOS"
    );
    if (node != 0) {
        enableLosCheck = node->value.nodes[1].value.i32;
    }

    zReader::Node *parentNode = zReader_GetNamedNode(
        readerNode,
        "SOUNDS"
    );
    if (parentNode != 0) {
        node = zReader_GetNamedNode(
            parentNode,
            "ON"
        );
        if (node != 0) {
            zSnd::FindSampleByName(node->value.nodes[1].value.str);
        }
        node = zReader_GetNamedNode(
            parentNode,
            "START"
        );
        if (node != 0) {
            zSnd::FindSampleByName(node->value.nodes[1].value.str);
        }
        node = zReader_GetNamedNode(
            parentNode,
            "STOP"
        );
        if (node != 0) {
            zSnd::FindSampleByName(node->value.nodes[1].value.str);
        }
    }

    parentNode = zReader_GetNamedNode(
        readerNode,
        g_HudCfgKey_Weapon
    );
    if (parentNode != 0) {
        node = zReader_GetNamedNode(
            parentNode,
            "NAME"
        );
        if (node != 0) {
            weaponCatalogEntry = OptCatalog::FindEntryByName(node->value.nodes[1].value.str);
        }
        node = zReader_GetNamedNode(
            parentNode,
            g_HudCfgKey_Ammo
        );
        if (node != 0) {
            weaponAmmo = node->value.nodes[1].value.i32;
        }
        node = zReader_GetNamedNode(
            parentNode,
            "BASE_MOVES"
        );
        if (node != 0) {
            weaponBaseMoves = node->value.nodes[1].value.i32;
        }
        node = zReader_GetNamedNode(
            parentNode,
            "DAMAGE_MODIFIER"
        );
        if (node != 0) {
            damageModifier = node->value.nodes[1].type == zReader::ZRDR_NODE_INT
                                 ? (float)(node->value.nodes[1].value.i32)
                                 : node->value.nodes[1].value.f32;
        }
        node = zReader_GetNamedNode(
            parentNode,
            "DETECTION_RANGE"
        );
        if (node != 0) {
            detectionRange = node->value.nodes[1].type == zReader::ZRDR_NODE_INT
                                 ? (float)(node->value.nodes[1].value.i32)
                                 : node->value.nodes[1].value.f32;
        }
        node = zReader_GetNamedNode(
            parentNode,
            "FIRE_DWELL"
        );
        if (node != 0) {
            fireDwellTime = node->value.nodes[1].type == zReader::ZRDR_NODE_INT
                                ? (float)(node->value.nodes[1].value.i32)
                                : node->value.nodes[1].value.f32;
        }
        node = zReader_GetNamedNode(
            parentNode,
            "FIRE_RATE"
        );
        if (node != 0) {
            fireRateSeconds = node->value.nodes[1].type == zReader::ZRDR_NODE_INT
                                  ? (float)(node->value.nodes[1].value.i32)
                                  : node->value.nodes[1].value.f32;
        }
        node = zReader_GetNamedNode(
            parentNode,
            "FIRE_LIMITS"
        );
        if (node != 0) {
            fireBurstDuration = node->value.nodes[1].type == zReader::ZRDR_NODE_INT
                                    ? (float)(node->value.nodes[1].value.i32)
                                    : node->value.nodes[1].value.f32;
            postBurstCooldown = node->value.nodes[2].type == zReader::ZRDR_NODE_INT
                                    ? (float)(node->value.nodes[2].value.i32)
                                    : node->value.nodes[2].value.f32;
        }
    }

    node = zReader_GetNamedNode(
        readerNode,
        "TARGETS"
    );
    if (node != 0) {
        for (int i = 1; i < node->value.nodes[0].value.i32; ++i) {
            targetTypes[i - 1] =
                zClass::FindByTypeAndName(
                    kZClassNodeObject3D,
                    node->value.nodes[i].value.str
                );
        }
    }

    if (zReader_GetNamedNode(
        readerNode,
        "MSL_LOCK"
    ) != 0) {
        hasMissileLock = 1;
        HudUiMgrSensor::TrackList_Add(
            HUD_SENSOR_TRACK_KIND_TURRET,
            this
        );
    }

    gwNode::GetWorldPosition(
        turretNode,
        &worldPos
    );
    zClass_Object3D::gwObject3DGetRotation(
        turretNode,
        &forward.x,
        &forward.y,
        &forward.z
    );
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
        zClass_Object3D::gwObject3DGetPosition(
            firePointNode0,
            &firePointLocal[0].x,
            &firePointLocal[0].y,
            &firePointLocal[0].z
        );
        zClass_Class::RemoveChild(
            partBarrelNode,
            firePointNode0
        );
        zClass_Util::DestroyNodeRecursive(firePointNode0);
        firePointNode0 = 0;
        firePos.y += firePointLocal[0].y;
    }

    if (firePointNode1 != 0) {
        zClass_Object3D::gwObject3DGetPosition(
            firePointNode1,
            &firePointLocal[1].x,
            &firePointLocal[1].y,
            &firePointLocal[1].z
        );
        zClass_Class::RemoveChild(
            partBarrelNode,
            firePointNode1
        );
        zClass_Util::DestroyNodeRecursive(firePointNode1);
        firePointNode1 = 0;
        firePos.y += (firePointLocal[1].y - firePointLocal[0].y) * 0.5f;
    }

    if (fireEffectNode != 0) {
        zClass_Class::gwNodeSetActive(
            fireEffectNode,
            0
        );
    }

    zClass_Class::gwNodeSetCellPickable(
        turretNode,
        0
    );
    if (damagePartNode == 0 && intersectBvolEnabled != 0) {
        zClass_Class::gwNodeSetPickable(
            healthyNode,
            1
        );
    }

    if ((weaponCatalogEntry->flags & kOptCatalogFlagCreateTrail) != 0) {
        trailRuntimeState = OptCatalog::CreateTrailRuntimeState(
            weaponCatalogEntry,
            turretNode,
            0,
            partBarrelNode,
            &fireDir,
            &spawnPos,
            2
        );
        fireRateSeconds = fireBurstDuration;
    }

    if (destroyAnimEntry == 0) {
        destroyAnimEntry = defaultDestroyAnim;
    }

    if (destroyAnimEntry != 0) {
        zEffect_Anim::NodeActionCallback(
            destroyAnimEntry,
            turretNode
        );
        if (healthCurrent > 0.0f) {
            zClass_Node::SetDamageHitCallback(
                this,
                healthyNode,
                (void *)zTurret_Runtime::OnDamage
            );
        }
    }

    zClass_NodePartial *const destroyedNode =
        zClass_Class::FindNodeRecursiveByName(
            turretNode,
            "destroyed"
        );
    if (destroyedNode != 0) {
        zClass_Class::gwNodeSetRaycastable(
            destroyedNode,
            0
        );
    }

    strncmp(
        turretNode->name,
        "hel_",
        4
    );
}

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-runtime-shutdown
 * @recoil-artifact defines .text recoil:function:0x436e00: zTurret_Runtime::Shutdown.
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Frees per-runtime trail state and clears the turret damage handler.
 */
int zTurret_Runtime::Shutdown() {
    if (trailRuntimeState != 0) {
        OptCatalog::FreeTrailRuntimeStateStorage(trailRuntimeState);
    }

    return zClass_Node::ClearDamageHandler(healthyNode);
}

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-runtime-hasactivenode
 * @recoil-artifact defines .text recoil:function:0x436e20: zTurret_Runtime::HasActiveNode.
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Reports whether the parsed turret has an active scene node.
 */
int zTurret_Runtime::HasActiveNode() {
    if (flags != 0 && (turretNode->flags & 0x04) != 0) {
        return 1;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-runtime-tick
 * @recoil-artifact defines .text recoil:function:0x436e40: zTurret_Runtime::Tick.
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Ticks target acquisition, aiming, firing, trail state, and turret deactivation.
 */
void zTurret_Runtime::Tick(
    const zVec3 *playerFxOffsetWorld
) {
    if (healthyNode == 0 || (healthyNode->flags & kZClassNodeActiveFlag) == 0 ||
        turretNode == 0 || (turretNode->flags & kZClassNodeActiveFlag) == 0 ||
        (deactivateNode != 0 && (deactivateNode->flags & kZClassNodeActiveFlag) == 0)) {
        if (fireEffectNode != 0 &&
            (fireEffectNode->flags & kZClassNodeActiveFlag) == 0) {
            zClass_Class::gwNodeSetActive(
                fireEffectNode,
                0
            );
        }
        if (runtimeInstanceActive != 0) {
            runtimeInstanceActive = 0;
            OptCatalog::DeactivateTrailRuntimeState(trailRuntimeState);
        }
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
        gwNode::GetWorldPosition(
            turretNode,
            &worldPos
        );
    }

    const zVec3 *const playerTarget = playerFxOffsetWorld != 0
                                          ? playerFxOffsetWorld
                                          : (playerState != 0 ? &playerState->fxOffsetWorld : 0);
    const zVec3 *targetPos = playerTarget != 0 ? playerTarget : &worldPos;
    float nearestDistance = (float)(_HUGE);

    for (int i = 0; i < 8; ++i) {
        zClass_NodePartial *const targetNode = targetTypes[i];
        if (targetNode == 0) {
            break;
        }

        if ((targetNode->flags & kZClassNodeActiveFlag) == 0) {
            continue;
        }

        zMat4x3 *const matrix =
            (zMat4x3 *)zClass_Object3D::gwObject3DGetMatrixPtr(targetNode);
        if ((targetNode->flags & 0x01000000) != 0 &&
            VariantTag::CurrentAllowsId(targetNode->nodeType) == 0) {
            continue;
        }
        const zVec3 *const targetNodePos = (const zVec3 *)(&matrix->posX);
        const float distance =
            (float)(fabs(worldPos.x - targetNodePos->x) + fabs(worldPos.z - targetNodePos->z));
        nearestTargetScore = distance;
        if (distance < nearestDistance) {
            nearestDistance = distance;
            targetPos = targetNodePos;
        }
    }

    if (playerTarget != 0 && playerState != 0 &&
        playerState->lifecycleState != kPlayerLifecycleInactive) {
        const float distance =
            (float)(fabs(worldPos.x - playerTarget->x) + fabs(worldPos.z - playerTarget->z));
        nearestTargetScore = distance;
        if (distance < nearestDistance) {
            nearestDistance = distance;
            targetPos = playerTarget;
        }
    }
    const int targetInRange = nearestDistance < detectionRange;

    if (targetInRange == 0) {
        isFiring = 0;
        if (weaponBaseMoves != 0 && hasMissileLock != 0) {
            UpdateFirePositionFromParts();
        }
    }

    if (targetInRange != 0) {
        if (g_zTurret_CallbackIterationActive != 0) {
            g_zTurret_CallbackIterationActive = 0;
            g_zTurret_CallbackStartIndex = g_zTurret_CallbackIterIndex;
        }

        const int losDirection = enableLosCheck == 1 ? 2 : 1;
        if (AINet::HasLineOfSightFromLocalPlayerFxOffset(healthyNode, &firePos, losDirection) !=
            0) {
            isFiring = 1;
            runtimeAimPending = 1;
            runtimeAimTarget.targetPos = (zVec3 *)targetPos;
            if (fireDwellTime != 0.0f) {
                fireDwellUntil = g_Time_AccumulatedTimeSec + fireDwellTime;
            }
        } else if (fireDwellTime == 0.0f || g_Time_AccumulatedTimeSec >= fireDwellUntil) {
            isFiring = 0;
        }

        if (weaponBaseMoves != 0) {
            UpdateFirePositionFromParts();
        }

        if (removeRuntimeOnFire != 0 && isFiring != 0) {
            zEffectAnim::SetVelocity_Thunk(
                destroyAnimEntry,
                turretNode,
                0.0f,
                0.0f,
                0.0f
            );
            OptCatalog::RemoveRuntimeInstance(
                weaponCatalogEntry,
                &worldPos,
                0
            );
            return;
        }
    }

    if (removeRuntimeOnFire == 0 && fireEffectNode != 0 &&
        (fireEffectNode->flags & kZClassNodeActiveFlag) != 0 &&
        g_Time_AccumulatedTimeSec < nextFireTime) {
        zModel_Instance_UpdateScrollingTexturesIfNeeded(
            (zModel_InstancePartial *)(fireEffectNode->userDataOrDiRef)
        );
    }

    if (removeRuntimeOnFire == 0 && weaponBaseMoves != 0) {
        gwNode::GetWorldPosition(
            turretNode,
            &worldPos
        );
    }

    if (partBarrelNode != 0) {
        partBarrelMatrix =
            (zMat4x3 *)zClass_Object3D::gwObject3DGetMatrixPtr(partBarrelNode);
    }

    if (isFiring != 0 && VariantTag::CurrentAllowsId(turretNode->nodeType) != 0) {
        UpdateFirePositionFromParts();
        if (AINet::HasLineOfSightFromLocalPlayerFxOffset(
                healthyNode,
                &firePos,
                enableLosCheck == 1 ? 2 : 1
            ) == 0) {
            isFiring = 0;
        }
        UpdateFirePositionFromParts();
        UpdateAimAndPartMatrices(targetPos);
        if (isFiring != 0 && g_Time_AccumulatedTimeSec >= nextFireTime) {
            if (fireEffectNode != 0) {
                if ((fireEffectNode->flags & kZClassNodeActiveFlag) == 0) {
                    zClass_Class::gwNodeSetActive(
                        fireEffectNode,
                        1
                    );
                    nextFireTime = g_Time_AccumulatedTimeSec + fireEffectDurationSec;
                    return;
                }

                zClass_Class::gwNodeSetActive(
                    fireEffectNode,
                    0
                );
            }

            SelectFirePointAndAimAtTarget(targetPos);
            if (fireAnimEntry == 0) {
                FireWeapon();
            } else {
                zEffectAnimEntry::SetOnStateDoneCallback(
                    fireAnimEntry,
                    (void *)zTurret_Runtime::FireWeaponCallback,
                    this
                );
                zEffectAnim::SetVelocity_Thunk(
                    fireAnimEntry,
                    turretNode,
                    0.0f,
                    0.0f,
                    0.0f
                );
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

        if (runtimeInstanceActive != 0) {
            runtimeInstanceActive = 0;
            OptCatalog::DeactivateTrailRuntimeState(trailRuntimeState);
        }
    }

    if (fireBurstTimer != fireBurstDuration) {
        fireBurstTimer += g_FrameDeltaTimeSec;
        if (fireBurstTimer >= fireBurstDuration) {
            fireBurstTimer = fireBurstDuration;
            nextFireTime = g_Time_AccumulatedTimeSec;
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-runtime-updatefirepositionfromparts
 * @recoil-artifact defines .text recoil:function:0x437430: zTurret_Runtime::UpdateFirePositionFromParts.
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Recomputes the turret fire origin from the active base, barrel, and fire-point parts.
 */
void zTurret_Runtime::UpdateFirePositionFromParts() {
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

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-runtime-updateaimandpartmatrices
 * @recoil-artifact defines .text recoil:function:0x4374a0: zTurret_Runtime::UpdateAimAndPartMatrices.
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Blends the turret aim direction and writes the recovered base/barrel matrices.
 */
void zTurret_Runtime::UpdateAimAndPartMatrices(
    const zVec3 *targetPos
) {
    zVec3 localAimDir = {partBarrelMatrix->posX, partBarrelMatrix->posY, partBarrelMatrix->posZ};

    zMat4x3 slotBuffer = {0};
    zMath::MatStackPushPtr((float *)(&slotBuffer));
    zMath::MatLoadIdentity();
    gwNode::BuildNodeToAncestorMatrix(
        turretNode,
        3
    );
    zMath::MatTransformPointBatchInPlace(
        &localAimDir,
        1
    );

    localAimDir.x = targetPos->x - localAimDir.x;
    localAimDir.y = targetPos->y - localAimDir.y;
    localAimDir.z = targetPos->z - localAimDir.z;
    zMath::Vec3Normalize(&localAimDir);
    zMath::Vec3ArrayTransformDirection(
        &localAimDir,
        1
    );
    zMath::MatStackPopPtr();

    if (alwaysLookAtTarget == 0) {
        const float alignment =
            localAimDir.x * forward.x + localAimDir.y * forward.y + localAimDir.z * forward.z;
        if (alignment > 0.89) {
            isFiring = 1;
        } else if (fireDwellTime == 0.0f) {
            isFiring = 0;
        }
    }

    const int forwardBlendBits = (int)(g_FrameDeltaTimeSec * -3.0f * 12102200.0f) + 0x3f800000;
    const float oldForwardWeight = *(const float *)(&forwardBlendBits);
    const float newForwardWeight = 1.0f - oldForwardWeight;
    forward.x = oldForwardWeight * forward.x + newForwardWeight * localAimDir.x;
    forward.y = oldForwardWeight * forward.y + newForwardWeight * localAimDir.y;
    forward.z = oldForwardWeight * forward.z + newForwardWeight * localAimDir.z;
    zMath::Vec3Normalize(&forward);

    localAimDir = forward;
    float horizontalLen = localAimDir.x * localAimDir.x + localAimDir.z * localAimDir.z;
    unsigned int horizontalLenBits = *(unsigned int *)(&horizontalLen);
    horizontalLenBits = (horizontalLenBits >> 1) + 0x1fc00000u;
    horizontalLen = *(float *)(&horizontalLenBits);

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
        zClass_Object3D::gwObject3DSetMatrix(
            partBaseNode,
            (float *)partBaseMatrix
        );

        partBarrelMatrix->yy = horizontalLen;
        partBarrelMatrix->yz = localAimDir.y;
        partBarrelMatrix->zy = -localAimDir.y;
        partBarrelMatrix->zz = horizontalLen;
        zClass_Object3D::gwObject3DSetMatrix(
            partBarrelNode,
            (float *)partBarrelMatrix
        );
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
    zClass_Object3D::gwObject3DSetMatrix(
        partBarrelNode,
        (float *)partBarrelMatrix
    );
}

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-runtime-selectfirepointandaimattarget
 * @recoil-artifact defines .text recoil:function:0x437730: zTurret_Runtime::SelectFirePointAndAimAtTarget.
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Selects the next muzzle point and computes the projectile direction toward the target.
 */
void zTurret_Runtime::SelectFirePointAndAimAtTarget(
    const zVec3 *targetPos
) {
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
    gwNode::BuildNodeToAncestorMatrix(
        partBarrelNode,
        3
    );
    zMath::MatTransformPointBatchInPlace(
        &spawnPos,
        1
    );

    fireDir.x = targetPos->x - spawnPos.x;
    fireDir.y = targetPos->y - spawnPos.y;
    fireDir.z = targetPos->z - spawnPos.z;
    zMath::Vec3Normalize(&fireDir);
    zMath::MatStackPopPtr();
}

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-runtime-fireweapon
 * @recoil-artifact defines .text recoil:function:0x437820: zTurret_Runtime::FireWeapon.
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Spawns the configured OptCatalog weapon or activates its trail runtime state.
 */
void zTurret_Runtime::FireWeapon() {
    if (trailRuntimeState == 0) {
        if (weaponCatalogEntry->gravity != 0.0f) {
            const float pitch = OptCatalog::ComputeAimPitchForTarget(
                weaponCatalogEntry,
                &spawnPos,
                0,
                runtimeAimTarget.targetPos,
                &nearestTargetScore
            );
            if (pitch != -1.0f) {
                Player::ApplyAimPitchToDirection(
                    &fireDir,
                    pitch
                );
            }
        }

        OptCatalog::SetPendingSpawnTargetOverrides(
            &runtimeAimPending,
            &runtimeAimTarget
        );
        if (weaponBaseMoves != 0) {
            zClass_Class::gwNodeSetRaycastable(
                turretNode->listA[0],
                0
            );
        }

        zUtil_PlayerStateStorage *const playerState =
            (zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState);
        g_OptCatalogNextSpawnScale = damageModifier;
        OptCatalog::AllocRuntimeInstance(
            weaponCatalogEntry,
            turretNode,
            &playerState->variantTag,
            &spawnPos,
            &fireDir,
            &spawnVel,
            0,
            0
        );

        if (weaponBaseMoves != 0) {
            zClass_Class::gwNodeSetRaycastable(
                turretNode->listA[0],
                1
            );
        }

        OptCatalog::SetPendingSpawnTargetOverrides(
            0,
            0
        );
        nextFireTime = g_Time_AccumulatedTimeSec + fireRateSeconds;
        UpdateFireBurstTimer(fireRateSeconds);
        return;
    }

    if (runtimeInstanceActive == 0) {
        runtimeInstanceActive = 1;
        OptCatalog::SetPendingSpawnTargetOverrides(
            &runtimeAimPending,
            &runtimeAimTarget
        );
        OptCatalog::ActivateTrailRuntimeState(
            trailRuntimeState,
            0
        );
        OptCatalog::SetPendingSpawnTargetOverrides(
            0,
            0
        );
        nextFireTime = g_Time_AccumulatedTimeSec + fireBurstDuration + postBurstCooldown;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-runtime-updatefirebursttimer
 * @recoil-artifact defines .text recoil:function:0x437990: zTurret_Runtime::UpdateFireBurstTimer.
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Advances burst timing and applies the post-burst fire cooldown.
 */
void zTurret_Runtime::UpdateFireBurstTimer(
    float deltaTime
) {
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

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-runtime-applydamageandhandledestruction
 * @recoil-artifact defines .text recoil:function:0x4379f0: zTurret_Runtime::ApplyDamageAndHandleDestruction.
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Applies damage, activate-on-hit timing, and turret destruction effects.
 */
int zTurret_Runtime::ApplyDamageAndHandleDestruction(
    float damageAmount,
    OptCatalogEntryDef *entry,
    OptCatalogHitEventPartial *hitEvent
) {
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

        zEffectAnim::SetVelocity_Thunk(
            destroyAnim,
            turretNode,
            0.0f,
            0.0f,
            0.0f
        );

        if (runtimeInstanceActive != 0) {
            OptCatalogTrailRuntimeState *const trailState = trailRuntimeState;
            runtimeInstanceActive = 0;
            OptCatalog::DeactivateTrailRuntimeState(trailState);
        }

        return 1;
    }

    return 0;
}

namespace zTurret_System {
/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-system-resetiterationstate
 * @recoil-artifact defines .text recoil:function:0x437aa0: zTurret_System::ResetIterationState.
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Clears turret runtime count and callback round-robin state.
 */
int __cdecl ResetIterationState() {
    g_zTurret_RuntimeCount = 0;
    g_zTurret_CallbackStartIndex = 0;
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-system-shutdown
 * @recoil-artifact defines .text recoil:function:0x437ab0: zTurret_System::Shutdown.
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Shuts down the zTurret subsystem by freeing all loaded runtime state.
 */
int __cdecl Shutdown() {
    FreeAllRuntimes();
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-system-loaddefinitionsfrompath
 * @recoil-artifact defines .text recoil:function:0x437ac0: zTurret_System::LoadDefinitionsFromPath.
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Loads turret definitions, allocates runtimes, and enables the tick callback.
 */
int __fastcall LoadDefinitionsFromPath(
    zClass_NodePartial *worldNode,
    const char *path
) {
    if (zOpt::GetNetworkEnabled() != 0) {
        return -1;
    }

    zReader::Node *const rootNode = zReader::LoadNodeFromPath(
        path,
        0,
        0
    );
    if (rootNode == 0) {
        zError::ReportOld(
            0x200,
            "D:\\Proj\\Battlesport\\turret.cpp",
            0x4ce,
            g_HudSensorTracker_ReadFileFailedFmt,
            path
        );
        return -1;
    }

    g_zTurret_LoadedDefRoot = rootNode;

    zEffectAnimEntry *defaultDestroyAnim = 0;
    zReader::Node *destroyAnimNode = zReader_GetNamedNode(
        rootNode,
        "DESTROY_ANIM"
    );
    if (destroyAnimNode != 0) {
        defaultDestroyAnim = zEffectAnim::FindEntryByName(
            destroyAnimNode->value.nodes[1].value.str
        );
    }

    zEffectAnimEntry *const napalmDestroyAnim = zEffectAnim::FindEntryByName(
        g_Player_NapalmVehicleEffectName
    );
    zReader::Node *const turretListNode = zReader_GetNamedNode(
        rootNode,
        "TURRET"
    );
    if (turretListNode != 0) {
        int index = 1;
        while (index < turretListNode->value.nodes[0].value.i32) {
            char *const turretName = turretListNode->value.nodes[index].value.str;
            zReader::Node *const readerNode =
                turretName != 0 ? zReader_GetNamedNode(
                    turretListNode,
                    turretName
                ) : 0;
            if (readerNode != 0) {
                char *searchName = zUtil_ZRDR_InitWildcardPath(turretName);
                while (searchName != 0) {
                    zClass_NodePartial *const turretWorldNode =
                        zClass::FindByTypeAndName(
                            kZClassNodeObject3D,
                            searchName
                        );
                    if (turretWorldNode != 0) {
                        zTurret_Runtime *runtime =
                            (zTurret_Runtime *)(::operator new(sizeof(zTurret_Runtime)));
                        if (runtime != 0) {
                            runtime = runtime->InitDefaults();
                        }

                        runtime->InitFromReaderNode(
                            worldNode,
                            turretWorldNode,
                            defaultDestroyAnim,
                            readerNode
                        );
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
    zClass_Class::gwNodeSetActionCallback(
        g_zTurret_CallbackNode,
        (void *)zTurret_System::TickAllRuntimesRoundRobin
    );
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-system-tickallruntimesroundrobin
 * @recoil-artifact defines .text recoil:function:0x437ca0: zTurret_System::TickAllRuntimesRoundRobin.
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Advances active turret runtimes using the recovered round-robin globals.
 */
void __cdecl TickAllRuntimesRoundRobin() {
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

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-system-disabletickcallback
 * @recoil-artifact defines .text recoil:function:0x437d40: zTurret_System::DisableTickCallback.
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Disables the zTurret round-robin action callback node.
 */
int __cdecl DisableTickCallback() {
    return zClass_Class::gwNodeSetActionCallback(
        g_zTurret_CallbackNode,
        0
    );
}

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-system-enabletickcallback
 * @recoil-artifact defines .text recoil:function:0x437d50: zTurret_System::EnableTickCallback.
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Enables the zTurret round-robin action callback node.
 */
int __cdecl EnableTickCallback() {
    return zClass_Class::gwNodeSetActionCallback(
        g_zTurret_CallbackNode,
        (void *)zTurret_System::TickAllRuntimesRoundRobin
    );
}
} // namespace zTurret_System

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-runtime-ondamage
 * @recoil-artifact defines .text recoil:function:0x437d60: zTurret_Runtime::OnDamage.
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Handles incoming OptCatalog damage and updates destruction or damage feedback.
 */
int __fastcall zTurret_Runtime::OnDamage(
    zTurret_Runtime *self,
    OptCatalogEntryDef *entry,
    OptCatalogHitEventPartial *hitEvent,
    float damageAmount
) {
    if (self->ApplyDamageAndHandleDestruction(
        damageAmount,
        entry,
        hitEvent
    ) != 0) {
        OptCatalog::SetDamageContext(
            1,
            0
        );
        Player::AddScaledHudCounterValue(self->healthMax);
    } else {
        DamageFeedback::SetIntensityScalar(self->healthCurrent / self->healthMax);
    }

    return 0;
}

namespace zTurret_System {
/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-system-freeallruntimes
 * @recoil-artifact defines .text recoil:function:0x437dc0: zTurret_System::FreeAllRuntimes.
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Releases turret runtimes, the loaded definition tree, and callback node.
 */
int __cdecl FreeAllRuntimes() {
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
        zClass_Class::gwNodeSetActionCallback(
            g_zTurret_CallbackNode,
            0
        );
        zClass_Object3D::DeleteNode(g_zTurret_CallbackNode);
        g_zTurret_CallbackNode = 0;
    }

    return 0;
}
} // namespace zTurret_System

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-runtime-fireweaponcallback
 * @recoil-artifact defines .text recoil:function:0x437e50: zTurret_Runtime::FireWeaponCallback.
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Bridges the fire animation completion callback to the turret weapon firing path.
 */
void __fastcall zTurret_Runtime::FireWeaponCallback(
    zEffectAnimEntry *entry,
    zTurret_Runtime *self,
    int eventCode
) {
    (void)entry;
    (void)eventCode;
    self->FireWeapon();
}
