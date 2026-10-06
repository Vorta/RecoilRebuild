#include "recoil/Mfc42Abi.h"
#include "Battlesport/turret.h"

#include "Battlesport/game_net.h"
#include "Battlesport/player.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zTime/time.h"
#include "opt_catalog.h"

#include <math.h>
#include <string.h>

extern char g_HudCfgKey_Weapon[7];
extern char g_HudCfgKey_Ammo[5];

/**
 * Data owner: zTurret writable runtime globals.
 * @recoil-anchor recoil:anchor:battlesport-turret-g-zturret-callbacknode
 * @recoil-artifact defines .data recoil:data:0x4f3fd0: g_zTurret_SystemStateStorage.
 * Purpose: Holds the turret system state: the tick callback node, the loaded
 * definition tree, the runtime list with its round-robin scan state, and the
 * napalm_vehicle destroy animation shared by turret destruction.
 * Evidence: retail reloads the runtime count after each runtime-list store
 * (0x437dc0) and after the Tick call (0x437ca0), which VC5 does only when the
 * list and the scalars share one storage object.
 *
 * Members (retail addresses): callbackNode 0x4f3fd0, loadedDefRoot 0x4f3fd4,
 * runtimeCount 0x4f3fd8, callbackIterationActive 0x4f3fdc, callbackStartIndex
 * 0x4f3fe0, callbackIterIndex 0x4f3fe4, runtimeList 0x4f3fe8 and
 * napalmVehicleDestroyAnim 0x4f41ec.
 */
extern "C" zTurret_SystemState g_zTurret_SystemStateStorage = { 0 };

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
 * @recoil-match byte
 *
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Applies the recovered default runtime state before turret field parsing.
 */
zTurret_Runtime::zTurret_Runtime()
{
    flags = 0;
    scenePathVisible = 0;
    healthyNode = 0;
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
    firePointLocal[0].x = firePointLocal[0].y = firePointLocal[0].z = 0.0f;
    firePointLocal[1].x = firePointLocal[1].y = firePointLocal[1].z = 0.0f;
    worldPos.x = worldPos.y = worldPos.z = 0.0f;
    forward.x = 0.0f;
    forward.y = 0.0f;
    forward.z = -1.0f;
    weaponAmmo = 50;
    detectionRange = 200.0f;
    fireRateSeconds = nextFireTime = damageModifier = 1.0f;
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
    runtimeInstanceActive = 0;
    enableLosCheck = 0;
    alwaysLookAtTarget = 0;
    healthCurrent = healthMax = 100.0f;
    damagePartNode = 0;
    intersectBvolEnabled = 1;
    destroyAnimEntry = 0;
    fireAnimEntry = 0;
    activateOnHitDamage = 0.0f;
    activateOnHitTimeout = (float)(_HUGE);
    for (int i = 0; i < 8; ++i) {
        targetTypes[i] = 0;
    }
    unknown_174[0] = 0;
    unknown_174[1] = 0;
    unknown_174[2] = 0;
    weaponCatalogEntry = 0;
    isFiring = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-runtime-initfromreadernode
 * @recoil-artifact defines .text recoil:function:0x4367a0: zTurret_Runtime::InitFromReaderNode.
 * @recoil-match byte
 *
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Parses a turret definition node and binds its scene parts, weapon, effects, and callbacks.
 */
void zTurret_Runtime::InitFromReaderNode(
    CZNodePartial* worldNode,
    CZNodePartial* turretWorldNode,
    zEffectAnimEntry* defaultDestroyAnim,
    zReader::Node* readerNode
)
{
    (void)worldNode;

    turretNode = turretWorldNode;
    zEffectAnimEntry* const namedDestroyAnim = zEffectAnim::FindEntryByName(turretWorldNode->name);
    if (namedDestroyAnim != 0) {
        defaultDestroyAnim = namedDestroyAnim;
    }

    zReader::Node* node = zRdrGetNode(readerNode, "PARTS");
    if (node != 0) {
        healthyNode = CZClass::FindNodeRecursiveByName(turretWorldNode, g_Player_HealthySubNodeName);
        if (healthyNode != 0) {
            flags = 1;
            scenePathVisible = 2;
            const int count = node->value.nodes[0].value.i32;
            if (count == 5) {
                firePointCount = 2;
                partBaseNode = CZClass::FindNodeRecursiveByName(turretWorldNode, node->value.nodes[1].value.str);
                partBarrelNode = CZClass::FindNodeRecursiveByName(turretWorldNode, node->value.nodes[2].value.str);
                firePointNode0 = CZClass::FindNodeRecursiveByName(turretWorldNode, node->value.nodes[3].value.str);
                firePointNode1 = CZClass::FindNodeRecursiveByName(turretWorldNode, node->value.nodes[4].value.str);
            } else if (count == 4) {
                firePointCount = 1;
                partBaseNode = CZClass::FindNodeRecursiveByName(turretWorldNode, node->value.nodes[1].value.str);
                partBarrelNode = CZClass::FindNodeRecursiveByName(turretWorldNode, node->value.nodes[2].value.str);
                firePointNode0 = CZClass::FindNodeRecursiveByName(turretWorldNode, node->value.nodes[3].value.str);
            } else if (count == 3) {
                firePointCount = 1;
                partBarrelNode = CZClass::FindNodeRecursiveByName(turretWorldNode, node->value.nodes[1].value.str);
                firePointNode0 = CZClass::FindNodeRecursiveByName(turretWorldNode, node->value.nodes[2].value.str);
            }
        }
    }

    node = zRdrGetNode(readerNode, "DEACTIVATE");
    if (node != 0) {
        deactivateNode = CZClass::FindByTypeAndName(kZClassNodeObject3D, node->value.nodes[1].value.str);
        for (int i = 2; i < node->value.nodes[0].value.i32; ++i) {
            deactivateNode = CZClass::FindNodeRecursiveByName(deactivateNode, node->value.nodes[i].value.str);
        }
    }

    node = zRdrGetNode(readerNode, "EFFECT");
    if (node != 0) {
        fireEffectNode = CZClass::FindNodeRecursiveByName(turretWorldNode, node->value.nodes[1].value.str);
        fireEffectDurationSec = node->value.nodes[2].value.f32;
        if (fireEffectNode != 0) {
            zModel::SetDiTextureWorldPerMeter((zDiPartial*)(fireEffectNode->userDataOrDiRef), 1, 10.0f, 0);
        }
    }

    node = zRdrGetNode(readerNode, "ACTIVATE_ON_HIT");
    if (node != 0) {
        activateOnHitDamage = node->value.nodes[1].value.f32;
        activateOnHitTimeout = 0.0f;
    }

    node = zRdrGetNode(readerNode, "ALWAYS_LOOK_AT");
    if (node != 0) {
        alwaysLookAtTarget = node->value.nodes[1].value.i32;
    }

    node = zRdrGetNode(readerNode, "DAMAGE_PART");
    if (node != 0) {
        damagePartNode = CZClass::FindNodeRecursiveByName(turretWorldNode, node->value.nodes[1].value.str);
    }

    node = zRdrGetNode(readerNode, "DESTROY_ANIM");
    if (node != 0) {
        destroyAnimEntry = zEffectAnim::FindEntryByName(node->value.nodes[1].value.str);
    }

    node = zRdrGetNode(readerNode, "FIRE_ANIM");
    if (node != 0) {
        fireAnimEntry = zEffectAnim::FindEntryByName(node->value.nodes[1].value.str);
    }

    node = zRdrGetNode(readerNode, "HEALTH");
    if (node != 0) {
        healthMax = healthCurrent = node->value.nodes[1].value.f32;
    }

    node = zRdrGetNode(readerNode, "INTERSECT_BVOL");
    if (node != 0) {
        intersectBvolEnabled = node->value.nodes[1].value.i32;
    }

    node = zRdrGetNode(readerNode, "LOS");
    if (node != 0) {
        enableLosCheck = node->value.nodes[1].value.i32;
    }

    zReader::Node* parentNode = zRdrGetNode(readerNode, "SOUNDS");
    if (parentNode != 0) {
        node = zRdrGetNode(parentNode, "ON");
        if (node != 0) {
            zSnd::FindSampleByName(node->value.nodes[1].value.str);
        }
        node = zRdrGetNode(parentNode, "START");
        if (node != 0) {
            zSnd::FindSampleByName(node->value.nodes[1].value.str);
        }
        node = zRdrGetNode(parentNode, "STOP");
        if (node != 0) {
            zSnd::FindSampleByName(node->value.nodes[1].value.str);
        }
    }

    parentNode = zRdrGetNode(readerNode, g_HudCfgKey_Weapon);
    if (parentNode != 0) {
        node = zRdrGetNode(parentNode, "NAME");
        if (node != 0) {
            weaponCatalogEntry = OptCatalog::FindEntryByName(node->value.nodes[1].value.str);
        }
        node = zRdrGetNode(parentNode, g_HudCfgKey_Ammo);
        if (node != 0) {
            weaponAmmo = node->value.nodes[1].value.i32;
        }
        node = zRdrGetNode(parentNode, "BASE_MOVES");
        if (node != 0) {
            weaponBaseMoves = node->value.nodes[1].value.i32;
        }
        node = zRdrGetNode(parentNode, "DAMAGE_MODIFIER");
        if (node != 0) {
            damageModifier = node->value.nodes[1].value.f32;
        }
        node = zRdrGetNode(parentNode, "DETECTION_RANGE");
        if (node != 0) {
            detectionRange = node->value.nodes[1].value.f32;
        }
        node = zRdrGetNode(parentNode, "FIRE_DWELL");
        if (node != 0) {
            fireDwellTime = node->value.nodes[1].value.f32;
        }
        node = zRdrGetNode(parentNode, "FIRE_RATE");
        if (node != 0) {
            fireRateSeconds = node->value.nodes[1].value.f32;
        }
        node = zRdrGetNode(parentNode, "FIRE_LIMITS");
        if (node != 0) {
            fireBurstDuration = node->value.nodes[1].value.f32;
            postBurstCooldown = node->value.nodes[2].value.f32;
        }
    }

    node = zRdrGetNode(readerNode, "TARGETS");
    if (node != 0) {
        for (int i = 1; i < node->value.nodes[0].value.i32; ++i) {
            targetTypes[i - 1] = CZClass::FindByTypeAndName(kZClassNodeObject3D, node->value.nodes[i].value.str);
        }
    }

    if (zRdrGetNode(readerNode, "MSL_LOCK") != 0) {
        hasMissileLock = 1;
        HudUiMgrSensor::TrackListAdd(HUD_SENSOR_TRACK_KIND_TURRET, this);
    }

    CZNode::GetWorldPosition(turretNode, &worldPos);
    CZObject3D::gwObject3DGetRotation(turretNode, &forward.x, &forward.y, &forward.z);
    zMath::Vec3Normalize(&forward);
    firePos = worldPos;

    if (partBaseNode != 0) {
        partBaseMatrix = (zMat4x3*)CZObject3D::gwObject3DGetMatrixPtr(partBaseNode);
        firePos.y += partBaseMatrix->posY;
    }

    if (partBarrelNode != 0) {
        partBarrelMatrix = (zMat4x3*)CZObject3D::gwObject3DGetMatrixPtr(partBarrelNode);
        firePos.y += partBarrelMatrix->posY;
    }

    if (firePointNode0 != 0) {
        CZObject3D::gwObject3DGetPosition(
            firePointNode0,
            &firePointLocal[0].x,
            &firePointLocal[0].y,
            &firePointLocal[0].z
        );
        CZClass::RemoveChild(partBarrelNode, firePointNode0);
        CZUtil::DestroyNodeRecursive(firePointNode0);
        firePointNode0 = 0;
        const float baseY = firePos.y;
        firePos.y = baseY + firePointLocal[0].y;
    }

    if (firePointNode1 != 0) {
        CZObject3D::gwObject3DGetPosition(
            firePointNode1,
            &firePointLocal[1].x,
            &firePointLocal[1].y,
            &firePointLocal[1].z
        );
        CZClass::RemoveChild(partBarrelNode, firePointNode1);
        CZUtil::DestroyNodeRecursive(firePointNode1);
        firePointNode1 = 0;
        firePos.y += (firePointLocal[1].y - firePointLocal[0].y) * 0.5f;
    }

    if (fireEffectNode != 0) {
        CZClass::gwNodeSetActive(fireEffectNode, 0);
    }

    CZClass::gwNodeSetCellPickable(turretNode, 0);
    if (damagePartNode == 0 && intersectBvolEnabled != 0) {
        CZClass::gwNodeSetPickable(healthyNode, 1);
    }

    if ((unsigned char)(weaponCatalogEntry->flags >> 1) & 1) {
        trailRuntimeState = OptCatalog::CreateTrailRuntimeState(
            weaponCatalogEntry,
            turretNode,
            0,
            partBarrelNode,
            &spawnPos,
            &fireDir,
            2
        );
        fireRateSeconds = fireBurstDuration;
    }

    if (destroyAnimEntry == 0) {
        destroyAnimEntry = defaultDestroyAnim;
    }

    if (destroyAnimEntry != 0) {
        zEffect_Anim::NodeActionCallback(destroyAnimEntry, turretNode);
        if (healthCurrent > 0.0f) {
            CZNode::SetDamageHitCallback(this, healthyNode, (void*)zTurret_Runtime::OnDamage);
        }
    }

    CZNodePartial* const destroyedNode = CZClass::FindNodeRecursiveByName(turretNode, "destroyed");
    if (destroyedNode != 0) {
        CZClass::gwNodeSetRaycastable(destroyedNode, 0);
    }

    strncmp(turretNode->name, "hel_", 4);
}

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-runtime-shutdown
 * @recoil-artifact defines .text recoil:function:0x436e00: zTurret_Runtime::Shutdown.
 * @recoil-match byte
 *
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Frees per-runtime trail state and clears the turret damage handler.
 */
int zTurret_Runtime::Shutdown()
{
    if (trailRuntimeState != 0) {
        OptCatalog::FreeTrailRuntimeStateStorage(trailRuntimeState);
    }

    return CZNode::ClearDamageHandler(healthyNode);
}

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-runtime-hasactivenode
 * @recoil-artifact defines .text recoil:function:0x436e20: zTurret_Runtime::HasActiveNode.
 * @recoil-match byte
 *
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Reports whether the parsed turret has an active scene node.
 */
int zTurret_Runtime::HasActiveNode()
{
    if (flags != 0 && (turretNode->flags & 0x04) != 0) {
        return 1;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-runtime-tick
 * @recoil-artifact defines .text recoil:function:0x436e40: zTurret_Runtime::Tick.
 *
 *
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Ticks target acquisition, aiming, firing, trail state, and turret deactivation.
 */
void zTurret_Runtime::Tick(const zVec3* playerFxOffsetWorld)
{
    int i = 0;

    if ((healthyNode->flags & kZClassNodeActiveFlag) == 0 || (turretNode->flags & kZClassNodeActiveFlag) == 0
        || (deactivateNode != 0 && (deactivateNode->flags & kZClassNodeActiveFlag) == 0)) {
        if (fireEffectNode != 0 && (fireEffectNode->flags & kZClassNodeActiveFlag) == 0) {
            CZClass::gwNodeSetActive(fireEffectNode, 0);
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

    if ((unsigned char)(weaponCatalogEntry->flags >> 13) & 1) {
        float nearestDistance = (float)(_HUGE);
        if (weaponBaseMoves != 0) {
            CZNode::GetWorldPosition(turretNode, &worldPos);
        }

        const zVec3* targetPos = playerFxOffsetWorld;
        for (; i < 8; ++i) {
            if (targetTypes[i] == 0) {
                break;
            }

            if ((targetTypes[i]->flags & kZClassNodeActiveFlag) != 0) {
                zMat4x3* const matrix = (zMat4x3*)CZObject3D::gwObject3DGetMatrixPtr(targetTypes[i]);
                const float distance = (float)fabs(worldPos.x - matrix->posX) + (float)fabs(worldPos.z - matrix->posZ);
                nearestTargetScore = distance;
                if (distance < nearestDistance) {
                    nearestDistance = distance;
                    targetPos = (const zVec3*)(&matrix->posX);
                }
            }
        }

        if (VariantTag::CurrentAllowsId(turretNode->nodeType) != 0
            && ((zUtil_PlayerStateStorage*)(g_GameStateOrMapTable->playerState))->lifecycleState
                != kPlayerLifecycleInactive) {
            const float distance
                = (float)fabs(worldPos.x - playerFxOffsetWorld->x) + (float)fabs(worldPos.z - playerFxOffsetWorld->z);
            nearestTargetScore = distance;
            if (distance < nearestDistance) {
                nearestDistance = distance;
                targetPos = playerFxOffsetWorld;
            }
        }

        if (nearestDistance <= detectionRange) {
            if (g_zTurret_CallbackIterationActive != 0) {
                g_zTurret_CallbackIterationActive = 0;
                g_zTurret_CallbackStartIndex = g_zTurret_CallbackIterIndex;
                if (weaponBaseMoves != 0) {
                    UpdateFirePositionFromParts();
                }

                int lineOfSight;
                if (enableLosCheck == 1) {
                    lineOfSight = AINet::HasLineOfSightFromLocalPlayerFxOffset(healthyNode, &firePos, 2);
                } else {
                    lineOfSight = AINet::HasLineOfSightFromLocalPlayerFxOffset(healthyNode, &firePos, 1);
                }

                if (lineOfSight != 0) {
                    isFiring = 1;
                    runtimeAimPending = 1;
                    runtimeAimTarget.targetPos = (zVec3*)targetPos;
                    if (fireDwellTime != 0.0f) {
                        fireDwellUntil = g_Time_AccumulatedTimeSec + fireDwellTime;
                    }
                } else if (fireDwellTime == 0.0f) {
                    isFiring = 0;
                } else if (g_Time_AccumulatedTimeSec >= fireDwellUntil) {
                    isFiring = 0;
                }
            } else if (weaponBaseMoves != 0 && hasMissileLock != 0) {
                UpdateFirePositionFromParts();
            }
        } else {
            isFiring = 0;
            if (weaponBaseMoves != 0 && hasMissileLock != 0) {
                UpdateFirePositionFromParts();
            }
        }

        if (isFiring != 0) {
            zEffectAnim::SetVelocityThunk(destroyAnimEntry, turretNode, 0.0f, 0.0f, 0.0f);
            OptCatalog::RemoveRuntimeInstance(weaponCatalogEntry, &worldPos, 0);
        }
        return;
    }

    if (fireEffectNode != 0 && (fireEffectNode->flags & kZClassNodeActiveFlag) != 0
        && g_Time_AccumulatedTimeSec < nextFireTime) {
        zModelInstanceUpdateScrollingTexturesIfNeeded((zModel_InstancePartial*)(fireEffectNode->userDataOrDiRef));
    }

    float nearestDistance = (float)(_HUGE);
    if (weaponBaseMoves != 0) {
        CZNode::GetWorldPosition(turretNode, &worldPos);
    }

    const zVec3* targetPos = playerFxOffsetWorld;
    for (; i < 8; ++i) {
        if (targetTypes[i] == 0) {
            break;
        }

        if ((targetTypes[i]->flags & kZClassNodeActiveFlag) != 0) {
            zMat4x3* const matrix = (zMat4x3*)CZObject3D::gwObject3DGetMatrixPtr(targetTypes[i]);
            const float distance = (float)fabs(worldPos.x - matrix->posX) + (float)fabs(worldPos.z - matrix->posZ);
            nearestTargetScore = distance;
            if (distance < nearestDistance) {
                nearestDistance = distance;
                targetPos = (const zVec3*)(&matrix->posX);
            }
        }
    }

    if (VariantTag::CurrentAllowsId(turretNode->nodeType) != 0
        && ((zUtil_PlayerStateStorage*)(g_GameStateOrMapTable->playerState))->lifecycleState
            != kPlayerLifecycleInactive) {
        const float distance
            = (float)fabs(worldPos.x - playerFxOffsetWorld->x) + (float)fabs(worldPos.z - playerFxOffsetWorld->z);
        nearestTargetScore = distance;
        if (distance < nearestDistance) {
            nearestDistance = distance;
            targetPos = playerFxOffsetWorld;
        }
    }

    if (nearestDistance <= detectionRange) {
        if (g_zTurret_CallbackIterationActive != 0) {
            g_zTurret_CallbackIterationActive = 0;
            g_zTurret_CallbackStartIndex = g_zTurret_CallbackIterIndex;
            if (weaponBaseMoves != 0) {
                UpdateFirePositionFromParts();
            }

            int lineOfSight;
            if (enableLosCheck == 1) {
                lineOfSight = AINet::HasLineOfSightFromLocalPlayerFxOffset(healthyNode, &firePos, 2);
            } else {
                lineOfSight = AINet::HasLineOfSightFromLocalPlayerFxOffset(healthyNode, &firePos, 1);
            }

            if (lineOfSight != 0) {
                isFiring = 1;
                runtimeAimPending = 1;
                runtimeAimTarget.targetPos = (zVec3*)targetPos;
                if (fireDwellTime != 0.0f) {
                    fireDwellUntil = g_Time_AccumulatedTimeSec + fireDwellTime;
                }
            } else if (fireDwellTime == 0.0f) {
                isFiring = 0;
            } else if (g_Time_AccumulatedTimeSec >= fireDwellUntil) {
                isFiring = 0;
            }
        } else if (weaponBaseMoves != 0 && hasMissileLock != 0) {
            UpdateFirePositionFromParts();
        }
    } else {
        isFiring = 0;
        if (weaponBaseMoves != 0 && hasMissileLock != 0) {
            UpdateFirePositionFromParts();
        }
    }

    if (isFiring != 0) {
        UpdateAimAndPartMatrices(runtimeAimTarget.targetPos);
        if (isFiring != 0) {
            if (g_Time_AccumulatedTimeSec >= nextFireTime) {
                if (fireEffectNode != 0 && (fireEffectNode->flags & kZClassNodeActiveFlag) == 0) {
                    CZClass::gwNodeSetActive(fireEffectNode, 1);
                    nextFireTime = fireEffectDurationSec + g_Time_AccumulatedTimeSec;
                } else {
                    if (fireEffectNode != 0 && (fireEffectNode->flags & kZClassNodeActiveFlag) != 0) {
                        CZClass::gwNodeSetActive(fireEffectNode, 0);
                    }

                    SelectFirePointAndAimAtTarget(playerFxOffsetWorld);
                    if (fireAnimEntry == 0) {
                        FireWeapon();
                    } else {
                        zEffectAnimEntry::SetOnStateDoneCallback(
                            fireAnimEntry,
                            (void*)zTurret_Runtime::FireWeaponCallback,
                            this
                        );
                        zEffectAnim::SetVelocityThunk(fireAnimEntry, turretNode, 0.0f, 0.0f, 0.0f);
                    }
                }
            }

            if (runtimeInstanceActive != 0) {
                UpdateFireBurstTimer(g_FrameDeltaTimeSec);
                if (weaponBaseMoves != 0 && isFiring != 0) {
                    SelectFirePointAndAimAtTarget(playerFxOffsetWorld);
                }
            }
        }
    }

    if (isFiring == 0) {
        if (alwaysLookAtTarget != 0) {
            UpdateAimAndPartMatrices(playerFxOffsetWorld);
        }

        if (trailRuntimeState != 0 && runtimeInstanceActive != 0) {
            runtimeInstanceActive = 0;
            OptCatalog::DeactivateTrailRuntimeState(trailRuntimeState);
        }

        if (fireBurstTimer != fireBurstDuration) {
            fireBurstTimer += g_FrameDeltaTimeSec;
            if (fireBurstTimer > fireBurstDuration) {
                fireBurstTimer = fireBurstDuration;
                nextFireTime = g_Time_AccumulatedTimeSec;
            }
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-runtime-updatefirepositionfromparts
 * @recoil-artifact defines .text recoil:function:0x437430: zTurret_Runtime::UpdateFirePositionFromParts.
 * @recoil-match byte
 *
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Recomputes the turret fire origin from the active base, barrel, and fire-point parts.
 */
void zTurret_Runtime::UpdateFirePositionFromParts()
{
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
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-dot
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.fast-exp-bits
 * @recoil-raw-consumer recoil:raw-asm:battlesport.turret.update-aim-and-part-matrices.fast-sqrt-estimate recoil:function:0x4374a0
 * @recoil-raw-asm recoil:raw-asm:battlesport.turret.update-aim-and-part-matrices.fast-sqrt-estimate
 * @recoil-match byte
 *
 * Raw assembly: inline zMath::Vec3Subtract [0x4374f3,0x437516) and
 * zMath::FastExp [0x4375ba,0x4375c5) expansions (requires turret.cpp /Ob1),
 * the reviewed full-XYZ dot island [0x437548,0x437567) and one in-body
 * 13-byte fast-sqrt estimate island at retail [0x43762b,0x437638).
 *
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Blends the turret aim direction and writes the recovered base/barrel matrices.
 */
void zTurret_Runtime::UpdateAimAndPartMatrices(const zVec3* targetPos)
{
    zVec3 localAimDir = { partBarrelMatrix->posX, partBarrelMatrix->posY, partBarrelMatrix->posZ };

    // Retail pushes the scratch matrix slot without clearing it.
    zMat4x3 slotBuffer;
    zMath::MatStackPushPtr((float*)(&slotBuffer));
    zMath::MatLoadIdentity();
    CZNode::gwNodeBuildNodeToAncestorMatrix(turretNode, 3);
    zMath::MatTransformPointBatchInPlace(&localAimDir, 1);

    zMath::Vec3Subtract(targetPos, &localAimDir, &localAimDir);
    zMath::Vec3Normalize(&localAimDir);
    zMath::Vec3ArrayTransformDirection(&localAimDir, 1);
    zMath::MatStackPopPtr();

    if (alwaysLookAtTarget == 0) {
        float alignment;
        ZMTH_VECTOR_DOT(alignment, &localAimDir, &forward);
        if (alignment > 0.89) {
            isFiring = 1;
        } else if (fireDwellTime == 0.0f) {
            isFiring = 0;
        }
    }

    const float oldForwardWeight = zMath::FastExp(g_FrameDeltaTimeSec * -3.0f);
    const float newForwardWeight = 1.0f - oldForwardWeight;
    forward.x = oldForwardWeight * forward.x + newForwardWeight * localAimDir.x;
    forward.y = oldForwardWeight * forward.y + newForwardWeight * localAimDir.y;
    forward.z = oldForwardWeight * forward.z + newForwardWeight * localAimDir.z;
    zMath::Vec3Normalize(&forward);

    localAimDir = forward;
    float horizontalLenSq = localAimDir.x * localAimDir.x + localAimDir.z * localAimDir.z;
    float horizontalLen;
    // Raw-assembly fast square-root estimate: retail transforms the named horizontalLenSq
    // bits through EAX ((bits >> 1) + 0x1fc00000) into the named horizontalLen local.
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    __asm {
        mov eax, horizontalLenSq
        sar eax, 1
        add eax, 01fc00000h
        mov horizontalLen, eax
    }
#else
    {
        int estimateBits;
        memcpy(&estimateBits, &horizontalLenSq, sizeof estimateBits);
        estimateBits = (estimateBits >> 1) + 0x1fc00000;
        memcpy(&horizontalLen, &estimateBits, sizeof horizontalLen);
    }
#endif

    float yawX;
    float yawZ;
    if (horizontalLen == 0.0f) {
        yawZ = 1.0f;
        yawX = 0.0f;
    } else {
        yawZ = -(localAimDir.z / horizontalLen);
        yawX = -(localAimDir.x / horizontalLen);
    }

    if (partBaseNode != 0) {
        partBaseMatrix->xx = yawZ;
        partBaseMatrix->xz = -yawX;
        partBaseMatrix->zx = yawX;
        partBaseMatrix->zz = yawZ;
        CZObject3D::gwObject3DSetMatrix(partBaseNode, (float*)partBaseMatrix);

        partBarrelMatrix->yy = horizontalLen;
        partBarrelMatrix->yz = localAimDir.y;
        partBarrelMatrix->zy = -localAimDir.y;
        partBarrelMatrix->zz = horizontalLen;
        CZObject3D::gwObject3DSetMatrix(partBarrelNode, (float*)partBarrelMatrix);
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
    CZObject3D::gwObject3DSetMatrix(partBarrelNode, (float*)partBarrelMatrix);
}

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-runtime-selectfirepointandaimattarget
 * @recoil-artifact defines .text recoil:function:0x437730: zTurret_Runtime::SelectFirePointAndAimAtTarget.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-match byte
 *
 * Raw assembly: the inline zMath::Vec3Subtract expansion [0x4377d9,0x4377fc)
 * (requires turret.cpp /Ob1).
 *
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Selects the next muzzle point and computes the projectile direction toward the target.
 */
void zTurret_Runtime::SelectFirePointAndAimAtTarget(const zVec3* targetPos)
{
    if (firePointCount > 1) {
        ++firePointIndex;
        if (firePointIndex >= firePointCount) {
            firePointIndex = 0;
        }
        spawnPos = firePointLocal[firePointIndex];
    } else {
        spawnPos = firePointLocal[0];
    }

    // Retail leaves the matrix-stack slot uninitialized before pushing it.
    zMat4x3 slotBuffer;
    zMath::MatStackPushPtr((float*)(&slotBuffer));
    zMath::MatLoadIdentity();
    CZNode::gwNodeBuildNodeToAncestorMatrix(partBarrelNode, 3);
    zMath::MatTransformPointBatchInPlace(&spawnPos, 1);

    zMath::Vec3Subtract(targetPos, &spawnPos, &fireDir);
    zMath::Vec3Normalize(&fireDir);
    zMath::MatStackPopPtr();
}

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-runtime-fireweapon
 * @recoil-artifact defines .text recoil:function:0x437820: zTurret_Runtime::FireWeapon.
 * @recoil-match byte
 *
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Spawns the configured OptCatalog weapon or activates its trail runtime state.
 */
void zTurret_Runtime::FireWeapon()
{
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
                Player::ApplyAimPitchToDirection(&fireDir, pitch);
            }
        }

        OptCatalog::SetPendingSpawnTargetOverrides(&runtimeAimPending, &runtimeAimTarget);
        if (weaponBaseMoves != 0) {
            CZClass::gwNodeSetRaycastable(turretNode->listA[0], 0);
        }

        const float spawnScale = damageModifier;
        g_OptCatalogNextSpawnScale = spawnScale;
        OptCatalog::AllocRuntimeInstance(
            weaponCatalogEntry,
            turretNode,
            &((zUtil_PlayerStateStorage*)(g_GameStateOrMapTable->playerState))->variantTag,
            &spawnPos,
            &fireDir,
            &spawnVel,
            0,
            0
        );

        if (weaponBaseMoves != 0) {
            CZClass::gwNodeSetRaycastable(turretNode->listA[0], 1);
        }

        OptCatalog::SetPendingSpawnTargetOverrides(0, 0);
        nextFireTime = g_Time_AccumulatedTimeSec + fireRateSeconds;
        UpdateFireBurstTimer(fireRateSeconds);
        return;
    }

    if (runtimeInstanceActive == 0) {
        runtimeInstanceActive = 1;
        OptCatalog::SetPendingSpawnTargetOverrides(&runtimeAimPending, &runtimeAimTarget);
        OptCatalog::ActivateTrailRuntimeState(trailRuntimeState, 0);
        OptCatalog::SetPendingSpawnTargetOverrides(0, 0);
        const float burstDuration = fireBurstDuration;
        nextFireTime = g_Time_AccumulatedTimeSec + burstDuration + postBurstCooldown;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-runtime-updatefirebursttimer
 * @recoil-artifact defines .text recoil:function:0x437990: zTurret_Runtime::UpdateFireBurstTimer.
 * @recoil-match byte
 *
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Advances burst timing and applies the post-burst fire cooldown.
 */
void zTurret_Runtime::UpdateFireBurstTimer(float deltaTime)
{
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
 * @recoil-match byte
 *
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Applies damage, activate-on-hit timing, and turret destruction effects.
 */
int zTurret_Runtime::ApplyDamageAndHandleDestruction(
    float damageAmount,
    OptCatalogEntryDef* entry,
    OptCatalogHitEventPartial* hitEvent
)
{
    if (activateOnHitDamage != 0.0f) {
        activateOnHitTimeout = g_Time_AccumulatedTimeSec + activateOnHitDamage;
    }

    if (damagePartNode != 0 && hitEvent->hitNode != damagePartNode) {
        return 0;
    }

    healthCurrent -= damageAmount;
    if (healthCurrent <= 0.0f) {
        if (((unsigned char)(entry->flags >> 12) & 1) != 0) {
            zEffectAnim::SetVelocityThunk(g_zTurret_NapalmVehicleDestroyAnim, turretNode, 0.0f, 0.0f, 0.0f);
        } else {
            zEffectAnim::SetVelocityThunk(destroyAnimEntry, turretNode, 0.0f, 0.0f, 0.0f);
        }

        if (runtimeInstanceActive != 0) {
            OptCatalogTrailRuntimeState* const trailState = trailRuntimeState;
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
 * @recoil-match byte
 *
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Clears turret runtime count and callback round-robin state.
 */
int __cdecl ResetIterationState()
{
    g_zTurret_RuntimeCount = 0;
    g_zTurret_CallbackStartIndex = 0;
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-system-shutdown
 * @recoil-artifact defines .text recoil:function:0x437ab0: zTurret_System::Shutdown.
 * @recoil-match byte
 *
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Shuts down the zTurret subsystem by freeing all loaded runtime state.
 */
int __cdecl Shutdown()
{
    FreeAllRuntimes();
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-system-loaddefinitionsfrompath
 * @recoil-artifact defines .text recoil:function:0x437ac0: zTurret_System::LoadDefinitionsFromPath.
 *
 *
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Loads turret definitions, allocates runtimes, and enables the tick callback.
 */
int __fastcall LoadDefinitionsFromPath(CZNodePartial* worldNode, const char* path)
{
    zEffectAnimEntry* defaultDestroyAnim = 0;
    if (zOpt::GetNetworkEnabled() != 0) {
        return -1;
    }

    zReader::Node* const rootNode = zReader::Load(path, 0, 0);
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

    zReader::Node* destroyAnimNode = zRdrGetNode(rootNode, "DESTROY_ANIM");
    if (destroyAnimNode != 0) {
        defaultDestroyAnim = zEffectAnim::FindEntryByName(destroyAnimNode->value.nodes[1].value.str);
    }

    zEffectAnimEntry* const napalmDestroyAnim = zEffectAnim::FindEntryByName(g_Player_NapalmVehicleEffectName);
    zReader::Node* const turretListNode = zRdrGetNode(rootNode, "TURRET");
    if (turretListNode != 0) {
        for (int index = 1; index < turretListNode->value.nodes[0].value.i32; index += 2) {
            zReader::Node* const readerNode = zRdrGetNode(turretListNode, turretListNode->value.nodes[index].value.str);
            if (readerNode != 0) {
                char* searchName = zRdrInitWildcardPath(turretListNode->value.nodes[index].value.str);
                do {
                    CZNodePartial* const turretWorldNode = CZClass::FindByTypeAndName(kZClassNodeObject3D, searchName);
                    if (turretWorldNode != 0) {
                        zTurret_Runtime* const runtime = new zTurret_Runtime;
                        runtime->InitFromReaderNode(worldNode, turretWorldNode, defaultDestroyAnim, readerNode);
                        g_zTurret_NapalmVehicleDestroyAnim = napalmDestroyAnim;
                        g_zTurret_RuntimeList[g_zTurret_RuntimeCount] = runtime;
                        ++g_zTurret_RuntimeCount;
                    }

                    searchName = zRdrNextWildcardPath();
                } while (searchName != 0);
            }
        }
    }

    g_zTurret_CallbackNode = CZObject3D::gwObject3DInit();
    CZClass::gwNodeSetActionCallback(g_zTurret_CallbackNode, (void*)zTurret_System::TickAllRuntimesRoundRobin);
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-system-tickallruntimesroundrobin
 * @recoil-artifact defines .text recoil:function:0x437ca0: zTurret_System::TickAllRuntimesRoundRobin.
 *
 *
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Advances active turret runtimes using the recovered round-robin globals.
 */
void __cdecl TickAllRuntimesRoundRobin()
{
    if (((zUtil_SaveGameState*)g_GameStateOrMapTable)->primaryModalState->masterModalData->masterType
        == kPlayerMasterTypeSub) {
        return;
    }

    g_zTurret_CallbackIterationActive = 1;
    g_zTurret_CallbackIterIndex = g_zTurret_CallbackStartIndex;
    for (int runtimeScanCount = 0; runtimeScanCount < g_zTurret_RuntimeCount;
        ++runtimeScanCount, ++g_zTurret_CallbackIterIndex) {
        if (g_zTurret_CallbackIterIndex >= g_zTurret_RuntimeCount) {
            g_zTurret_CallbackIterIndex = 0;
        }

        if (g_zTurret_RuntimeList[g_zTurret_CallbackIterIndex]->flags != 0) {
            g_zTurret_RuntimeList[g_zTurret_CallbackIterIndex]->Tick(
                &g_LocalPlayerSaveState->playerState->fxOffsetWorld
            );
        }
    }

    ++g_zTurret_CallbackStartIndex;
    if (g_zTurret_CallbackStartIndex >= g_zTurret_RuntimeCount) {
        g_zTurret_CallbackStartIndex = 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-system-disabletickcallback
 * @recoil-artifact defines .text recoil:function:0x437d40: zTurret_System::DisableTickCallback.
 * @recoil-match byte
 *
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Disables the zTurret round-robin action callback node.
 */
int __cdecl DisableTickCallback()
{
    return CZClass::gwNodeSetActionCallback(g_zTurret_CallbackNode, 0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-system-enabletickcallback
 * @recoil-artifact defines .text recoil:function:0x437d50: zTurret_System::EnableTickCallback.
 * @recoil-match byte
 *
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Enables the zTurret round-robin action callback node.
 */
int __cdecl EnableTickCallback()
{
    return CZClass::gwNodeSetActionCallback(g_zTurret_CallbackNode, (void*)zTurret_System::TickAllRuntimesRoundRobin);
}
} // namespace zTurret_System

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-runtime-ondamage
 * @recoil-artifact defines .text recoil:function:0x437d60: zTurret_Runtime::OnDamage.
 * @recoil-match byte
 *
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Handles incoming OptCatalog damage and updates destruction or damage feedback.
 */
int __fastcall zTurret_Runtime::OnDamage(
    zTurret_Runtime* self,
    OptCatalogEntryDef* entry,
    OptCatalogHitEventPartial* hitEvent,
    float damageAmount
)
{
    if (self->ApplyDamageAndHandleDestruction(damageAmount, entry, hitEvent) != 0) {
        OptCatalog::SetDamageContext(1, 0);
        const float healthMax = self->healthMax;
        Player::AddScaledHudCounterValue(healthMax);
    } else {
        DamageFeedback::SetIntensityScalar(self->healthCurrent / self->healthMax);
    }

    return 0;
}

namespace zTurret_System {
/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-system-freeallruntimes
 * @recoil-artifact defines .text recoil:function:0x437dc0: zTurret_System::FreeAllRuntimes.
 *
 *
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Releases turret runtimes, the loaded definition tree, and callback node.
 */
int __cdecl FreeAllRuntimes()
{
    for (int i = 0; i < g_zTurret_RuntimeCount; ++i) {
        g_zTurret_RuntimeList[i]->Shutdown();
        ::operator delete(g_zTurret_RuntimeList[i]);
        g_zTurret_RuntimeList[i] = 0;
    }

    g_zTurret_RuntimeCount = 0;
    if (g_zTurret_LoadedDefRoot != 0) {
        zReader::Free(g_zTurret_LoadedDefRoot);
        g_zTurret_LoadedDefRoot = 0;
    }

    if (g_zTurret_CallbackNode != 0) {
        CZClass::gwNodeSetActionCallback(g_zTurret_CallbackNode, 0);
        CZObject3D::DeleteNode(g_zTurret_CallbackNode);
        g_zTurret_CallbackNode = 0;
    }

    return 0;
}
} // namespace zTurret_System

/**
 * @recoil-anchor recoil:anchor:battlesport-turret-zturret-runtime-fireweaponcallback
 * @recoil-artifact defines .text recoil:function:0x437e50: zTurret_Runtime::FireWeaponCallback.
 * @recoil-match byte
 *
 * Source file: D:\Proj\Battlesport\turret.cpp.
 * Purpose: Bridges the fire animation completion callback to the turret weapon firing path.
 */
void __fastcall zTurret_Runtime::FireWeaponCallback(zEffectAnimEntry* entry, zTurret_Runtime* self, int eventCode)
{
    (void)entry;
    (void)eventCode;
    self->FireWeapon();
}
