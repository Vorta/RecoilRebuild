#include "GameZRecoil/zTurret/zTurret.h"

#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/GameNet.h"
#include "GameZRecoil/Time/Time.h"
#include "Battlesport/player.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zMath/zMath.h"

#include <windows.h>

#include <cstring>
#include <cstdlib>
#include <new>

static int CheckVec3(const zVec3 &value, float x, float y, float z) {
    return value.x == x && value.y == y && value.z == z ? 0 : 1;
}

static void MakeReaderStringNode(zReader::Node &node, const char *value) {
    node.type = zReader::ZRDR_NODE_STRING;
    node.value.str = const_cast<char *>(value);
}

static void MakeReaderIntNode(zReader::Node &node, int value) {
    node.type = zReader::ZRDR_NODE_INT;
    node.value.i32 = value;
}

static void MakeReaderFloatNode(zReader::Node &node, float value) {
    node.type = zReader::ZRDR_NODE_FLOAT;
    node.value.f32 = value;
}

static void MakeReaderArrayNode(zReader::Node &node, zReader::Node *payload, int count) {
    payload[0].type = zReader::ZRDR_NODE_INT;
    payload[0].value.i32 = count;
    node.type = zReader::ZRDR_NODE_ARRAY;
    node.value.nodes = payload;
}

static void WriteZrdU32(HANDLE file, unsigned int value) {
    DWORD written = 0;
    WriteFile(file, &value, sizeof(value), &written, nullptr);
}

static void WriteZrdBytes(HANDLE file, const char *text, unsigned int length) {
    DWORD written = 0;
    WriteFile(file, text, length, &written, nullptr);
}

static void WriteZrdNode(HANDLE file, const zReader::Node &node) {
    WriteZrdU32(file, static_cast<unsigned int>(node.type));
    switch (node.type) {
    case zReader::ZRDR_NODE_INT:
    case zReader::ZRDR_NODE_FLOAT:
        WriteZrdU32(file, node.value.u32);
        break;
    case zReader::ZRDR_NODE_STRING: {
        const unsigned int length = static_cast<unsigned int>(std::strlen(node.value.str));
        WriteZrdU32(file, length);
        WriteZrdBytes(file, node.value.str, length);
        break;
    }
    case zReader::ZRDR_NODE_ARRAY: {
        const int count = node.value.nodes[0].value.i32;
        WriteZrdU32(file, static_cast<unsigned int>(count));
        for (int i = 1; i < count; ++i) {
            WriteZrdNode(file, node.value.nodes[i]);
        }
        break;
    }
    }
}

static void InitObjectNode(zClass_NodePartial &node, zClass_Object3DDataPartial &data,
                           const char *name) {
    std::memset(&node, 0, sizeof(node));
    std::memset(&data, 0, sizeof(data));
    std::strncpy(node.name, name, sizeof(node.name) - 1);
    node.classId = 5;
    node.classData = &data;
    data.localMatrix[0] = 1.0f;
    data.localMatrix[4] = 1.0f;
    data.localMatrix[8] = 1.0f;
    data.cachedWorldMatrix[0] = 1.0f;
    data.cachedWorldMatrix[4] = 1.0f;
    data.cachedWorldMatrix[8] = 1.0f;
}

extern "C" int zturret_reset_iteration_state_smoke(void) {
    g_zTurret_RuntimeCount = 17;
    g_zTurret_CallbackStartIndex = 9;

    return zTurret_System::ResetIterationState() == 0 && g_zTurret_RuntimeCount == 0 &&
                   g_zTurret_CallbackStartIndex == 0
               ? 0
               : 1;
}

extern "C" int zturret_runtime_init_defaults_smoke(void) {
    zTurret_Runtime *const runtime =
        static_cast<zTurret_Runtime *>(::operator new(sizeof(zTurret_Runtime)));
    std::memset(runtime, 0xcd, sizeof(*runtime));

    zTurret_Runtime *const result = runtime->InitDefaults();
    if (result != runtime) {
        ::operator delete(runtime);
        return 1;
    }

    int failed = 0;
    failed |= runtime->flags != 0;
    failed |= runtime->scenePathVisible != 0;
    failed |= runtime->healthyNode != nullptr;
    failed |= CheckVec3(runtime->worldPos, 0.0f, 0.0f, 0.0f);
    failed |= runtime->weaponBaseMoves != 0;
    failed |= runtime->hasMissileLock != 0;
    failed |= runtime->deactivateNode != nullptr;
    failed |= runtime->partBaseNode != nullptr;
    failed |= runtime->partBarrelNode != nullptr;
    failed |= CheckVec3(runtime->firePointLocal[0], 0.0f, 0.0f, 0.0f);
    failed |= CheckVec3(runtime->firePointLocal[1], 0.0f, 0.0f, 0.0f);
    failed |= CheckVec3(runtime->forward, 0.0f, 0.0f, -1.0f);
    failed |= runtime->fireEffectNode != nullptr;
    for (int i = 0; i < 8; ++i) {
        failed |= runtime->targetTypes[i] != nullptr;
    }
    failed |= runtime->weaponCatalogEntry != nullptr;
    failed |= runtime->weaponAmmo != 50;
    failed |= runtime->detectionRange != 200.0f;
    failed |= runtime->isFiring != 0;
    failed |= runtime->fireAnimEntry != nullptr;
    failed |= runtime->damageModifier != 1.0f;
    failed |= runtime->firePointIndex != 0;
    failed |= runtime->firePointCount != 0;
    failed |= runtime->firePointNode0 != nullptr;
    failed |= runtime->firePointNode1 != nullptr;
    failed |= CheckVec3(runtime->spawnPos, 0.0f, 0.0f, -1.0f);
    failed |= CheckVec3(runtime->fireDir, 0.0f, 0.0f, 0.0f);
    failed |= CheckVec3(runtime->spawnVel, 0.0f, 0.0f, 0.0f);
    failed |= runtime->fireRateSeconds != 1.0f;
    failed |= runtime->nextFireTime != 1.0f;
    failed |= runtime->fireBurstTimer != 0.0f;
    failed |= runtime->fireBurstDuration != 0.0f;
    failed |= runtime->postBurstCooldown != 0.0f;
    failed |= runtime->fireDwellTime != 0.0f;
    failed |= runtime->fireDwellUntil != 0.0f;
    failed |= runtime->trailRuntimeState != nullptr;
    failed |= runtime->enableLosCheck != 0;
    failed |= runtime->alwaysLookAtTarget != 0;
    failed |= runtime->destroyAnimEntry != nullptr;
    failed |= runtime->healthCurrent != 100.0f;
    failed |= runtime->healthMax != 100.0f;
    failed |= runtime->damagePartNode != nullptr;
    failed |= runtime->activateOnHitDamage != 0.0f;
    failed |= runtime->activateOnHitTimeout < 1.0e30f;
    failed |= runtime->intersectBvolEnabled != 1;
    for (int i = 0; i < 12; ++i) {
        failed |= runtime->unknown_174[i] != 0;
    }

    ::operator delete(runtime);
    return failed != 0 ? 2 : 0;
}

extern "C" int zturret_runtime_has_active_node_smoke(void) {
    zClass_NodePartial turretNode{};
    zTurret_Runtime runtime{};
    runtime.turretNode = &turretNode;

    runtime.flags = 0;
    turretNode.flags = 4;
    const bool inactiveRuntime = runtime.HasActiveNode() == 0;

    runtime.flags = 1;
    turretNode.flags = 0;
    const bool inactiveNode = runtime.HasActiveNode() == 0;

    turretNode.flags = 4;
    const bool activeNode = runtime.HasActiveNode() == 1;

    return inactiveRuntime && inactiveNode && activeNode ? 0 : 1;
}

extern "C" int zturret_runtime_init_from_reader_node_smoke(void) {
    const int oldEntryCount = g_OptCatalog_EntryCount;
    OptCatalogEntryDef *const oldEntryTable = g_OptCatalog_EntryTable;
    const short oldAnimEntryCount = g_zEffectAnim_EntryCount;
    zEffectAnimEntry *const oldAnimEntryList = g_zEffectAnim_EntryList;

    OptCatalogEntryDef weaponEntry = {};
    weaponEntry.keyName = const_cast<char *>("turret_weapon");
    weaponEntry.flags = 0;
    g_OptCatalog_EntryTable = &weaponEntry;
    g_OptCatalog_EntryCount = 1;
    g_zEffectAnim_EntryList = nullptr;
    g_zEffectAnim_EntryCount = 0;

    zClass_NodePartial turretNode = {};
    zClass_NodePartial healthyNode = {};
    zClass_NodePartial barrelNode = {};
    zClass_NodePartial destroyedNode = {};
    zClass_Object3DDataPartial turretData = {};
    zClass_Object3DDataPartial healthyData = {};
    zClass_Object3DDataPartial barrelData = {};
    zClass_Object3DDataPartial destroyedData = {};

    InitObjectNode(turretNode, turretData, "turret_alpha");
    InitObjectNode(healthyNode, healthyData, "healthy");
    InitObjectNode(barrelNode, barrelData, "barrel");
    InitObjectNode(destroyedNode, destroyedData, "destroyed");
    turretNode.flags = 0x00080008;
    destroyedNode.flags = 0x10;
    turretData.cachedWorldMatrix[9] = 7.0f;
    turretData.cachedWorldMatrix[10] = 8.0f;
    turretData.cachedWorldMatrix[11] = 9.0f;
    turretData.rotation.z = -2.0f;
    barrelData.localMatrix[10] = 1.5f;

    zClass_NodePartial *turretChildren[] = {&healthyNode, &barrelNode, &destroyedNode};
    turretNode.listCountB = 3;
    turretNode.listB = turretChildren;

    zReader::Node partsPayload[3] = {};
    zReader::Node activatePayload[2] = {};
    zReader::Node lookAtPayload[2] = {};
    zReader::Node healthPayload[2] = {};
    zReader::Node intersectPayload[2] = {};
    zReader::Node losPayload[2] = {};
    zReader::Node weaponNamePayload[2] = {};
    zReader::Node weaponAmmoPayload[2] = {};
    zReader::Node weaponBaseMovesPayload[2] = {};
    zReader::Node weaponDamagePayload[2] = {};
    zReader::Node weaponRangePayload[2] = {};
    zReader::Node weaponDwellPayload[2] = {};
    zReader::Node weaponRatePayload[2] = {};
    zReader::Node weaponLimitsPayload[3] = {};
    zReader::Node weaponFields[17] = {};
    zReader::Node rootFields[15] = {};
    zReader::Node rootNode = {};

    MakeReaderArrayNode(rootNode, rootFields, 15);
    MakeReaderStringNode(rootFields[1], "PARTS");
    MakeReaderArrayNode(rootFields[2], partsPayload, 3);
    MakeReaderStringNode(partsPayload[1], "barrel");
    MakeReaderStringNode(partsPayload[2], "missing_firepoint");
    MakeReaderStringNode(rootFields[3], "ACTIVATE_ON_HIT");
    MakeReaderArrayNode(rootFields[4], activatePayload, 2);
    MakeReaderFloatNode(activatePayload[1], 2.5f);
    MakeReaderStringNode(rootFields[5], "ALWAYS_LOOK_AT");
    MakeReaderArrayNode(rootFields[6], lookAtPayload, 2);
    MakeReaderIntNode(lookAtPayload[1], 1);
    MakeReaderStringNode(rootFields[7], "HEALTH");
    MakeReaderArrayNode(rootFields[8], healthPayload, 2);
    MakeReaderFloatNode(healthPayload[1], 275.0f);
    MakeReaderStringNode(rootFields[9], "INTERSECT_BVOL");
    MakeReaderArrayNode(rootFields[10], intersectPayload, 2);
    MakeReaderIntNode(intersectPayload[1], 1);
    MakeReaderStringNode(rootFields[11], "LOS");
    MakeReaderArrayNode(rootFields[12], losPayload, 2);
    MakeReaderIntNode(losPayload[1], 1);
    MakeReaderStringNode(rootFields[13], "WEAPON");
    MakeReaderArrayNode(rootFields[14], weaponFields, 17);

    MakeReaderStringNode(weaponFields[1], "NAME");
    MakeReaderArrayNode(weaponFields[2], weaponNamePayload, 2);
    MakeReaderStringNode(weaponNamePayload[1], "turret_weapon");
    MakeReaderStringNode(weaponFields[3], "AMMO");
    MakeReaderArrayNode(weaponFields[4], weaponAmmoPayload, 2);
    MakeReaderIntNode(weaponAmmoPayload[1], 19);
    MakeReaderStringNode(weaponFields[5], "BASE_MOVES");
    MakeReaderArrayNode(weaponFields[6], weaponBaseMovesPayload, 2);
    MakeReaderIntNode(weaponBaseMovesPayload[1], 1);
    MakeReaderStringNode(weaponFields[7], "DAMAGE_MODIFIER");
    MakeReaderArrayNode(weaponFields[8], weaponDamagePayload, 2);
    MakeReaderFloatNode(weaponDamagePayload[1], 1.75f);
    MakeReaderStringNode(weaponFields[9], "DETECTION_RANGE");
    MakeReaderArrayNode(weaponFields[10], weaponRangePayload, 2);
    MakeReaderFloatNode(weaponRangePayload[1], 325.0f);
    MakeReaderStringNode(weaponFields[11], "FIRE_DWELL");
    MakeReaderArrayNode(weaponFields[12], weaponDwellPayload, 2);
    MakeReaderFloatNode(weaponDwellPayload[1], 0.25f);
    MakeReaderStringNode(weaponFields[13], "FIRE_RATE");
    MakeReaderArrayNode(weaponFields[14], weaponRatePayload, 2);
    MakeReaderFloatNode(weaponRatePayload[1], 0.75f);
    MakeReaderStringNode(weaponFields[15], "FIRE_LIMITS");
    MakeReaderArrayNode(weaponFields[16], weaponLimitsPayload, 3);
    MakeReaderFloatNode(weaponLimitsPayload[1], 3.5f);
    MakeReaderFloatNode(weaponLimitsPayload[2], 4.5f);

    zTurret_Runtime runtime = {};
    runtime.InitDefaults();
    runtime.InitFromReaderNode(nullptr, &turretNode, nullptr, &rootNode);

    int failed = 0;
    failed |= runtime.turretNode != &turretNode;
    failed |= runtime.healthyNode != &healthyNode;
    failed |= runtime.flags != 1;
    failed |= runtime.scenePathVisible != 2;
    failed |= runtime.partBarrelNode != &barrelNode;
    failed |= runtime.partBarrelMatrix != (zMat4x3 *)(barrelData.localMatrix);
    failed |= runtime.firePointCount != 1;
    failed |= runtime.firePointNode0 != nullptr;
    failed |= CheckVec3(runtime.worldPos, 7.0f, 8.0f, 9.0f);
    failed |= CheckVec3(runtime.forward, 0.0f, 0.0f, -1.0f);
    failed |= CheckVec3(runtime.firePos, 7.0f, 9.5f, 9.0f);
    failed |= runtime.activateOnHitTimeout != 0.0f;
    failed |= runtime.activateOnHitDamage != 2.5f;
    failed |= runtime.alwaysLookAtTarget != 1;
    failed |= runtime.healthCurrent != 275.0f;
    failed |= runtime.healthMax != 275.0f;
    failed |= runtime.intersectBvolEnabled != 1;
    failed |= runtime.enableLosCheck != 1;
    failed |= runtime.weaponCatalogEntry != &weaponEntry;
    failed |= runtime.weaponAmmo != 19;
    failed |= runtime.weaponBaseMoves != 1;
    failed |= runtime.damageModifier != 1.75f;
    failed |= runtime.detectionRange != 325.0f;
    failed |= runtime.fireDwellTime != 0.25f;
    failed |= runtime.fireRateSeconds != 0.75f;
    failed |= runtime.fireBurstDuration != 3.5f;
    failed |= runtime.postBurstCooldown != 4.5f;
    failed |= runtime.destroyAnimEntry != nullptr;
    failed |= runtime.trailRuntimeState != nullptr;
    failed |= (turretNode.flags & 0x08) != 0;
    failed |= (healthyNode.flags & 0x20) == 0;
    failed |= (destroyedNode.flags & 0x10) != 0;

    g_OptCatalog_EntryCount = oldEntryCount;
    g_OptCatalog_EntryTable = oldEntryTable;
    g_zEffectAnim_EntryCount = oldAnimEntryCount;
    g_zEffectAnim_EntryList = oldAnimEntryList;

    return failed != 0 ? 1 : 0;
}

extern "C" int zturret_update_fire_position_from_parts_smoke(void) {
    zMat4x3 baseMatrix = {};
    zMat4x3 barrelMatrix = {};
    zClass_NodePartial baseNode = {};
    zClass_NodePartial barrelNode = {};
    zClass_NodePartial firePoint0 = {};
    zClass_NodePartial firePoint1 = {};
    zTurret_Runtime runtime = {};

    runtime.worldPos = {10.0f, 20.0f, 30.0f};
    runtime.partBaseNode = &baseNode;
    runtime.partBaseMatrix = &baseMatrix;
    runtime.partBarrelNode = &barrelNode;
    runtime.partBarrelMatrix = &barrelMatrix;
    baseMatrix.posY = 2.0f;
    barrelMatrix.posY = 3.0f;
    runtime.UpdateFirePositionFromParts();
    if (CheckVec3(runtime.firePos, 10.0f, 25.0f, 30.0f) != 0) {
        return 1;
    }

    runtime.firePointLocal[0].y = 4.0f;
    runtime.firePointLocal[1].y = 20.0f;
    runtime.firePointNode0 = &firePoint0;
    runtime.firePointNode1 = &firePoint1;
    runtime.UpdateFirePositionFromParts();
    if (CheckVec3(runtime.firePos, 10.0f, 29.0f, 30.0f) != 0) {
        return 2;
    }

    runtime.firePointNode0 = nullptr;
    runtime.firePointNode1 = &firePoint1;
    runtime.UpdateFirePositionFromParts();
    return CheckVec3(runtime.firePos, 10.0f, 33.0f, 30.0f) == 0 ? 0 : 3;
}

extern "C" int zturret_update_aim_and_part_matrices_smoke(void) {
    const float oldFrameDelta = g_FrameDeltaTimeSec;
    g_FrameDeltaTimeSec = 0.0f;

    zClass_NodePartial turretNode = {};
    zClass_NodePartial baseNode = {};
    zClass_NodePartial barrelNode = {};
    zClass_Object3DDataPartial turretData = {};
    zClass_Object3DDataPartial baseData = {};
    zClass_Object3DDataPartial barrelData = {};
    InitObjectNode(turretNode, turretData, "turret");
    InitObjectNode(baseNode, baseData, "base");
    InitObjectNode(barrelNode, barrelData, "barrel");
    turretNode.flags |= 0x00080000;

    zTurret_Runtime runtime = {};
    runtime.turretNode = &turretNode;
    runtime.partBaseNode = &baseNode;
    runtime.partBaseMatrix = (zMat4x3 *)(baseData.localMatrix);
    runtime.partBarrelNode = &barrelNode;
    runtime.partBarrelMatrix = (zMat4x3 *)(barrelData.localMatrix);
    runtime.forward = {0.0f, 0.0f, -1.0f};
    runtime.isFiring = 0;

    zVec3 targetPos = {0.0f, 0.0f, -10.0f};
    runtime.UpdateAimAndPartMatrices(&targetPos);

    int failed = 0;
    failed |= runtime.isFiring != 1;
    failed |= CheckVec3(runtime.forward, 0.0f, 0.0f, -1.0f);
    failed |= runtime.partBaseMatrix->xx != 1.0f;
    failed |= runtime.partBaseMatrix->xz != 0.0f;
    failed |= runtime.partBaseMatrix->zx != 0.0f;
    failed |= runtime.partBaseMatrix->zz != 1.0f;
    failed |= runtime.partBarrelMatrix->yy != 1.0f;
    failed |= runtime.partBarrelMatrix->yz != 0.0f;
    failed |= runtime.partBarrelMatrix->zy != 0.0f;
    failed |= runtime.partBarrelMatrix->zz != 1.0f;
    failed |= (baseNode.flags & 0x02) == 0;
    failed |= (barrelNode.flags & 0x02) == 0;

    zTurret_Runtime noBaseRuntime = {};
    zClass_NodePartial noBaseTurretNode = {};
    zClass_NodePartial noBaseBarrelNode = {};
    zClass_Object3DDataPartial noBaseTurretData = {};
    zClass_Object3DDataPartial noBaseBarrelData = {};
    InitObjectNode(noBaseTurretNode, noBaseTurretData, "turret2");
    InitObjectNode(noBaseBarrelNode, noBaseBarrelData, "barrel2");
    noBaseTurretNode.flags |= 0x00080000;

    noBaseRuntime.turretNode = &noBaseTurretNode;
    noBaseRuntime.partBarrelNode = &noBaseBarrelNode;
    noBaseRuntime.partBarrelMatrix = (zMat4x3 *)(noBaseBarrelData.localMatrix);
    noBaseRuntime.forward = {1.0f, 0.0f, 0.0f};
    noBaseRuntime.alwaysLookAtTarget = 1;
    targetPos = {10.0f, 0.0f, 0.0f};
    noBaseRuntime.UpdateAimAndPartMatrices(&targetPos);

    failed |= noBaseRuntime.partBarrelMatrix->xx != 0.0f;
    failed |= noBaseRuntime.partBarrelMatrix->xz != 1.0f;
    failed |= noBaseRuntime.partBarrelMatrix->yx != 0.0f;
    failed |= noBaseRuntime.partBarrelMatrix->yy != 1.0f;
    failed |= noBaseRuntime.partBarrelMatrix->yz != 0.0f;
    failed |= noBaseRuntime.partBarrelMatrix->zx != -1.0f;
    failed |= noBaseRuntime.partBarrelMatrix->zy != 0.0f;
    failed |= noBaseRuntime.partBarrelMatrix->zz != 0.0f;

    runtime.forward = {0.0f, 0.0f, -1.0f};
    runtime.isFiring = 1;
    runtime.fireDwellTime = 0.0f;
    targetPos = {10.0f, 0.0f, 0.0f};
    runtime.UpdateAimAndPartMatrices(&targetPos);
    failed |= runtime.isFiring != 0;

    g_FrameDeltaTimeSec = oldFrameDelta;
    return failed != 0 ? 1 : 0;
}

extern "C" int zturret_select_fire_point_and_aim_smoke(void) {
    zClass_NodePartial barrelNode = {};
    zClass_Object3DDataPartial barrelData = {};
    InitObjectNode(barrelNode, barrelData, "barrel");
    barrelNode.flags |= 0x00080000;
    barrelData.cachedWorldMatrix[9] = 10.0f;

    zTurret_Runtime runtime = {};
    runtime.partBarrelNode = &barrelNode;
    runtime.firePointCount = 2;
    runtime.firePointIndex = 0;
    runtime.firePointLocal[0] = {100.0f, 100.0f, 100.0f};
    runtime.firePointLocal[1] = {1.0f, 2.0f, 3.0f};

    zVec3 targetPos = {11.0f, 2.0f, 8.0f};
    runtime.SelectFirePointAndAimAtTarget(&targetPos);

    int failed = 0;
    failed |= runtime.firePointIndex != 1;
    failed |= CheckVec3(runtime.spawnPos, 11.0f, 2.0f, 3.0f);
    failed |= CheckVec3(runtime.fireDir, 0.0f, 0.0f, 1.0f);

    runtime.SelectFirePointAndAimAtTarget(&targetPos);
    failed |= runtime.firePointIndex != 0;
    failed |= CheckVec3(runtime.spawnPos, 110.0f, 100.0f, 100.0f);

    zTurret_Runtime singleRuntime = {};
    singleRuntime.partBarrelNode = &barrelNode;
    singleRuntime.firePointCount = 1;
    singleRuntime.firePointIndex = 4;
    singleRuntime.firePointLocal[0] = {2.0f, 0.0f, 0.0f};
    targetPos = {12.0f, 0.0f, 0.0f};
    singleRuntime.SelectFirePointAndAimAtTarget(&targetPos);
    failed |= singleRuntime.firePointIndex != 4;
    failed |= CheckVec3(singleRuntime.spawnPos, 12.0f, 0.0f, 0.0f);
    failed |= CheckVec3(singleRuntime.fireDir, 0.0f, 0.0f, 0.0f);

    return failed != 0 ? 1 : 0;
}

extern "C" int zturret_update_fire_burst_timer_smoke(void) {
    const float oldTime = g_Time_AccumulatedTimeSec;
    g_Time_AccumulatedTimeSec = 100.0f;

    zTurret_Runtime runtime = {};
    runtime.fireBurstDuration = 0.0f;
    runtime.fireBurstTimer = 7.0f;
    runtime.isFiring = 1;
    runtime.nextFireTime = 3.0f;
    runtime.UpdateFireBurstTimer(2.0f);

    int failed = 0;
    failed |= runtime.fireBurstTimer != 7.0f;
    failed |= runtime.isFiring != 1;
    failed |= runtime.nextFireTime != 3.0f;

    runtime.fireBurstDuration = 5.0f;
    runtime.fireBurstTimer = 3.0f;
    runtime.postBurstCooldown = 8.0f;
    runtime.UpdateFireBurstTimer(1.0f);
    failed |= runtime.fireBurstTimer != 2.0f;
    failed |= runtime.isFiring != 1;
    failed |= runtime.nextFireTime != 3.0f;

    runtime.UpdateFireBurstTimer(2.5f);
    failed |= runtime.fireBurstTimer != 5.0f;
    failed |= runtime.isFiring != 0;
    failed |= runtime.nextFireTime != 108.0f;

    g_Time_AccumulatedTimeSec = oldTime;
    return failed != 0 ? 1 : 0;
}

extern "C" int zturret_runtime_tick_smoke(void) {
    const float oldTime = g_Time_AccumulatedTimeSec;
    const float oldFrameDelta = g_FrameDeltaTimeSec;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const int oldVariantFilter = g_Variant_FilterEnabled;
    const int oldCallbackIterationActive = g_zTurret_CallbackIterationActive;
    const int oldCallbackStartIndex = g_zTurret_CallbackStartIndex;
    const int oldCallbackIterIndex = g_zTurret_CallbackIterIndex;

    zClass_NodePartial turretNode = {};
    zClass_NodePartial healthyNode = {};
    zClass_NodePartial barrelNode = {};
    zClass_NodePartial targetNode = {};
    zClass_NodePartial playerRootNode = {};
    zClass_NodePartial worldNode = {};
    zClass_Object3DDataPartial turretData = {};
    zClass_Object3DDataPartial healthyData = {};
    zClass_Object3DDataPartial barrelData = {};
    zClass_Object3DDataPartial targetData = {};
    zClass_Object3DDataPartial playerRootData = {};
    zClass_WorldDataPartial worldData = {};

    InitObjectNode(turretNode, turretData, "tick_turret");
    InitObjectNode(healthyNode, healthyData, "healthy");
    InitObjectNode(barrelNode, barrelData, "barrel");
    InitObjectNode(targetNode, targetData, "target");
    InitObjectNode(playerRootNode, playerRootData, "player");
    turretNode.flags = 0x14;
    healthyNode.flags = 0x14;
    barrelNode.flags = 0x14;
    targetNode.flags = 0x14;
    playerRootNode.flags = 0x10;
    worldNode.classData = &worldData;
    targetData.localMatrix[9] = 12.0f;
    targetData.localMatrix[10] = 1.0f;
    targetData.localMatrix[11] = 5.0f;
    targetData.cachedWorldMatrix[9] = 12.0f;
    targetData.cachedWorldMatrix[10] = 1.0f;
    targetData.cachedWorldMatrix[11] = 5.0f;

    zUtil_PlayerStateStorage playerState = {};
    playerState.rootNode = &playerRootNode;
    playerState.lifecycleState = 1;
    playerState.fxOffsetWorld = {100.0f, 0.0f, 100.0f};
    zInput_GameStateOrMapTablePartial gameState = {};
    gameState.playerState =
        static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&playerState));

    OptCatalogEntryDef weaponEntry = {};
    weaponEntry.flags = 0;

    zTurret_Runtime runtime = {};
    runtime.turretNode = &turretNode;
    runtime.healthyNode = &healthyNode;
    runtime.partBarrelNode = &barrelNode;
    runtime.partBarrelMatrix = (zMat4x3 *)barrelData.localMatrix;
    runtime.targetTypes[0] = &targetNode;
    runtime.weaponCatalogEntry = &weaponEntry;
    runtime.detectionRange = 30.0f;
    runtime.worldPos = {0.0f, 0.0f, 0.0f};
    runtime.firePos = {0.0f, 0.0f, 0.0f};
    runtime.forward = {0.0f, 0.0f, -1.0f};
    runtime.firePointCount = 1;
    runtime.firePointLocal[0] = {0.0f, 0.0f, 0.0f};
    runtime.fireRateSeconds = 1.0f;
    runtime.nextFireTime = 100.0f;
    runtime.fireDwellTime = 2.0f;
    runtime.activateOnHitTimeout = 1000.0f;
    runtime.enableLosCheck = 0;

    g_Time_AccumulatedTimeSec = 10.0f;
    g_FrameDeltaTimeSec = 0.016f;
    g_GameStateOrMapTable = &gameState;
    g_Player_RuntimeDiScene = &worldNode;
    g_Variant_FilterEnabled = 0;
    g_zTurret_CallbackIterationActive = 1;
    g_zTurret_CallbackStartIndex = 0;
    g_zTurret_CallbackIterIndex = 5;

    runtime.Tick(&playerState.fxOffsetWorld);

    const zVec3 *const targetPos = (const zVec3 *)(&targetData.localMatrix[9]);
    int failed = 0;
    failed |= runtime.isFiring != 1 ? 1 : 0;
    failed |= runtime.runtimeAimPending != 1 ? 2 : 0;
    failed |= runtime.runtimeAimTarget.targetPos != targetPos ? 4 : 0;
    failed |= runtime.fireDwellUntil != 12.0f ? 8 : 0;
    failed |= runtime.nearestTargetScore <= 0.0f ? 16 : 0;
    failed |= g_zTurret_CallbackIterationActive != 0 ? 32 : 0;
    failed |= g_zTurret_CallbackStartIndex != 5 ? 64 : 0;
    failed |= (healthyNode.flags & 0x10) == 0 ? 128 : 0;
    failed |= (playerRootNode.flags & 0x10) == 0 ? 256 : 0;

    g_zTurret_CallbackIterIndex = oldCallbackIterIndex;
    g_zTurret_CallbackStartIndex = oldCallbackStartIndex;
    g_zTurret_CallbackIterationActive = oldCallbackIterationActive;
    g_Variant_FilterEnabled = oldVariantFilter;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_FrameDeltaTimeSec = oldFrameDelta;
    g_Time_AccumulatedTimeSec = oldTime;

    return failed;
}

extern "C" int zturret_tick_all_runtimes_round_robin_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zUtil_SaveGameState *const oldLocalPlayerSaveState = g_LocalPlayerSaveState;
    const int oldRuntimeCount = g_zTurret_RuntimeCount;
    const int oldCallbackIterationActive = g_zTurret_CallbackIterationActive;
    const int oldCallbackStartIndex = g_zTurret_CallbackStartIndex;
    const int oldCallbackIterIndex = g_zTurret_CallbackIterIndex;
    zTurret_Runtime *oldRuntimeList[3] = {
        g_zTurret_RuntimeList[0],
        g_zTurret_RuntimeList[1],
        g_zTurret_RuntimeList[2],
    };

    PlayerMasterModalData masterModalData = {};
    PlayerModalState modalState = {};
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    zTurret_Runtime inactiveRuntime = {};
    zClass_NodePartial turretNode = {};
    zClass_NodePartial healthyNode = {};

    modalState.masterModalData = &masterModalData;
    saveState.primaryModalState = &modalState;
    saveState.playerState = &playerState;
    inactiveRuntime.flags = 1;
    inactiveRuntime.turretNode = &turretNode;
    inactiveRuntime.healthyNode = &healthyNode;

    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)(&saveState);
    g_LocalPlayerSaveState = &saveState;
    g_zTurret_RuntimeCount = 3;
    g_zTurret_RuntimeList[0] = &inactiveRuntime;
    g_zTurret_RuntimeList[1] = &inactiveRuntime;
    g_zTurret_RuntimeList[2] = &inactiveRuntime;
    g_zTurret_CallbackStartIndex = 2;
    g_zTurret_CallbackIterIndex = 99;
    g_zTurret_CallbackIterationActive = 0;

    masterModalData.masterType = 2;
    zTurret_System::TickAllRuntimesRoundRobin();
    const bool subSkipped = g_zTurret_CallbackStartIndex == 2 &&
                            g_zTurret_CallbackIterIndex == 99 &&
                            g_zTurret_CallbackIterationActive == 0;

    masterModalData.masterType = 1;
    zTurret_System::TickAllRuntimesRoundRobin();
    const bool roundRobinAdvanced = g_zTurret_CallbackStartIndex == 0 &&
                                    g_zTurret_CallbackIterIndex == 2 &&
                                    g_zTurret_CallbackIterationActive == 1;

    g_zTurret_RuntimeList[2] = oldRuntimeList[2];
    g_zTurret_RuntimeList[1] = oldRuntimeList[1];
    g_zTurret_RuntimeList[0] = oldRuntimeList[0];
    g_zTurret_CallbackIterIndex = oldCallbackIterIndex;
    g_zTurret_CallbackStartIndex = oldCallbackStartIndex;
    g_zTurret_CallbackIterationActive = oldCallbackIterationActive;
    g_zTurret_RuntimeCount = oldRuntimeCount;
    g_LocalPlayerSaveState = oldLocalPlayerSaveState;
    g_GameStateOrMapTable = oldGameStateOrMapTable;

    return subSkipped && roundRobinAdvanced ? 0 : 1;
}

extern "C" int zturret_disable_tick_callback_smoke(void) {
    zClass_NodePartial callbackNode{};
    callbackNode.callbackPriority = 2;
    callbackNode.actionCallback = reinterpret_cast<void *>(&zturret_disable_tick_callback_smoke);

    zClass_NodePartial *const oldCallbackNode = g_zTurret_CallbackNode;
    g_zTurret_CallbackNode = &callbackNode;

    const int result = zTurret_System::DisableTickCallback();
    const bool cleared = result == 0 && callbackNode.actionCallback == nullptr;

    g_zTurret_CallbackNode = oldCallbackNode;
    return cleared ? 0 : 1;
}

extern "C" int zturret_enable_tick_callback_smoke(void) {
    zClass_NodePartial callbackNode{};
    callbackNode.callbackPriority = 2;
    callbackNode.actionCallback = reinterpret_cast<void *>(&zturret_enable_tick_callback_smoke);

    zClass_NodePartial *const oldCallbackNode = g_zTurret_CallbackNode;
    g_zTurret_CallbackNode = &callbackNode;

    const int result = zTurret_System::EnableTickCallback();
    const bool enabled = result == 0 &&
                         callbackNode.actionCallback ==
                             reinterpret_cast<void *>(zTurret_System::TickAllRuntimesRoundRobin);

    g_zTurret_CallbackNode = oldCallbackNode;
    return enabled ? 0 : 1;
}

extern "C" int zturret_load_definitions_from_path_smoke(void) {
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    const int oldEntryCount = g_OptCatalog_EntryCount;
    OptCatalogEntryDef *const oldEntryTable = g_OptCatalog_EntryTable;
    const short oldAnimEntryCount = g_zEffectAnim_EntryCount;
    zEffectAnimEntry *const oldAnimEntryList = g_zEffectAnim_EntryList;
    zArchiveList *const oldMountedList = g_zArchive_MountedList;
    zReader::Node *const oldLoadedRoot = g_zTurret_LoadedDefRoot;
    zClass_NodePartial *const oldCallbackNode = g_zTurret_CallbackNode;
    zEffectAnimEntry *const oldNapalmDestroyAnim = g_zTurret_NapalmVehicleDestroyAnim;
    const int oldRuntimeCount = g_zTurret_RuntimeCount;
    zClass_NodeFreeListSlot *const oldNodeArray = g_zClass_NodeArray;
    const int oldNodeFreeHeadIndex = g_zClass_NodeFreeHeadIndex;
    const int oldActiveNodeCount = g_zClass_ActiveNodeCount;
    zClass_TypeListLink *const oldFreeLinkHead = g_zClass_TypeList_FreeLinkHead;
    zClass_TypeListLink *const oldPendingFreeHead = g_zClass_NodeList_PendingFreeHead;
    const int oldLiveLinkCount = g_zClass_TypeList_LiveLinkCount;
    const int oldPeakLiveLinkCount = g_zClass_TypeList_PeakLiveLinkCount;
    zClass_TypeListLink *oldHead[16] = {};
    zClass_TypeListLink *oldTail[16] = {};
    int oldPendingDirty[16] = {};
    for (int i = 0; i < 16; ++i) {
        oldHead[i] = zClass_TypeList::Head(i);
        oldTail[i] = zClass_TypeList::Tail(i);
        oldPendingDirty[i] = zClass_TypeList::PendingRemovalDirty(i);
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }

    char tempDir[MAX_PATH] = {};
    char tempPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "ztr", 0, tempPath) == 0) {
        return 1;
    }

    HANDLE file = CreateFileA(tempPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        DeleteFileA(tempPath);
        return 2;
    }

    int networkEnabled = 1;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    const bool networkSkipped =
        zTurret_System::LoadDefinitionsFromPath(nullptr, "C:\\dummy\\turret.zrd") == -1;
    networkEnabled = 0;

    zReader::Node partsPayload[3] = {};
    zReader::Node weaponNamePayload[2] = {};
    zReader::Node weaponFields[3] = {};
    zReader::Node runtimeFields[5] = {};
    zReader::Node turretPayload[3] = {};
    zReader::Node rootFields[3] = {};
    zReader::Node rootNode = {};

    MakeReaderArrayNode(rootNode, rootFields, 3);
    MakeReaderStringNode(rootFields[1], "TURRET");
    MakeReaderArrayNode(rootFields[2], turretPayload, 3);

    MakeReaderStringNode(turretPayload[1], "turret_alpha*");
    MakeReaderArrayNode(turretPayload[2], runtimeFields, 5);

    MakeReaderStringNode(runtimeFields[1], "PARTS");
    MakeReaderArrayNode(runtimeFields[2], partsPayload, 3);
    MakeReaderStringNode(partsPayload[1], "barrel");
    MakeReaderStringNode(partsPayload[2], "missing_firepoint");
    MakeReaderStringNode(runtimeFields[3], "WEAPON");
    MakeReaderArrayNode(runtimeFields[4], weaponFields, 3);
    MakeReaderStringNode(weaponFields[1], "NAME");
    MakeReaderArrayNode(weaponFields[2], weaponNamePayload, 2);
    MakeReaderStringNode(weaponNamePayload[1], "turret_weapon");

    WriteZrdU32(file, 0xaaaaaaaa);
    const unsigned int zrdOffset = SetFilePointer(file, 0, nullptr, FILE_CURRENT);
    WriteZrdNode(file, rootNode);
    FlushFileBuffers(file);

    zZarFileRecord record = {};
    record.fileOffset = zrdOffset;
    record.fileSize = SetFilePointer(file, 0, nullptr, FILE_CURRENT) - zrdOffset;
    std::strcpy(record.name, "turret.zrd");

    zIndexArchive archive = {};
    archive.hFile = file;
    archive.recordCount = 1;
    archive.records = &record;

    zArchiveListNode archiveNode = {};
    archiveNode.payload = &archive;
    archiveNode.next = &archiveNode;
    archiveNode.prev = &archiveNode;
    zArchiveList archiveList = {};
    archiveList.count = 1;
    archiveList.head = &archiveNode;
    g_zArchive_MountedList = &archiveList;

    zClass_NodePartial worldNode = {};
    zClass_NodePartial turretNode = {};
    zClass_NodePartial healthyNode = {};
    zClass_NodePartial barrelNode = {};
    zClass_Object3DDataPartial turretData = {};
    zClass_Object3DDataPartial healthyData = {};
    zClass_Object3DDataPartial barrelData = {};
    InitObjectNode(turretNode, turretData, "turret_alpha0");
    InitObjectNode(healthyNode, healthyData, "healthy");
    InitObjectNode(barrelNode, barrelData, "barrel");
    zClass_NodePartial *turretChildren[] = {&healthyNode, &barrelNode};
    turretNode.listCountB = 2;
    turretNode.listB = turretChildren;
    zClass_TypeList::Insert(6, &turretNode);

    zClass_NodeFreeListSlot callbackSlot = {};
    callbackSlot.freeTag = 0x00ffffff;
    g_zClass_NodeArray = &callbackSlot;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;

    OptCatalogEntryDef weaponEntry = {};
    weaponEntry.keyName = const_cast<char *>("turret_weapon");
    weaponEntry.flags = 0;
    g_OptCatalog_EntryTable = &weaponEntry;
    g_OptCatalog_EntryCount = 1;
    g_zEffectAnim_EntryList = nullptr;
    g_zEffectAnim_EntryCount = 0;
    g_zTurret_RuntimeCount = 0;
    g_zTurret_LoadedDefRoot = nullptr;
    g_zTurret_CallbackNode = nullptr;
    g_zTurret_NapalmVehicleDestroyAnim = nullptr;

    const int loadResult = zTurret_System::LoadDefinitionsFromPath(&worldNode, "turret.zrd");
    zTurret_Runtime *const runtime = g_zTurret_RuntimeList[0];
    int failed = 0;
    failed |= loadResult != 0 ? 1 : 0;
    failed |= g_zTurret_LoadedDefRoot == nullptr ? 2 : 0;
    failed |= g_zTurret_CallbackNode != &callbackSlot.node ? 4 : 0;
    failed |= g_zTurret_RuntimeCount != 1 ? 8 : 0;
    failed |= runtime == nullptr ? 16 : 0;
    if (runtime != nullptr) {
        failed |= runtime->turretNode != &turretNode ? 32 : 0;
        failed |= runtime->healthyNode != &healthyNode ? 64 : 0;
        failed |= runtime->partBarrelNode != &barrelNode ? 128 : 0;
        failed |= runtime->weaponCatalogEntry != &weaponEntry ? 256 : 0;
    }
    failed |= g_zTurret_CallbackNode == nullptr ||
                      g_zTurret_CallbackNode->actionCallback !=
                          (void *)zTurret_System::TickAllRuntimesRoundRobin
                  ? 512
                  : 0;

    if (g_zTurret_LoadedDefRoot != nullptr) {
        zReader::FreeLoadedTree(g_zTurret_LoadedDefRoot);
    }
    if (g_zTurret_CallbackNode != nullptr) {
        std::free(g_zTurret_CallbackNode->classData);
    }
    if (runtime != nullptr) {
        ::operator delete(runtime);
    }
    g_zTurret_RuntimeList[0] = nullptr;

    g_zClass_TypeList_LiveLinkCount = oldLiveLinkCount;
    g_zClass_TypeList_PeakLiveLinkCount = oldPeakLiveLinkCount;
    g_zClass_NodeList_PendingFreeHead = oldPendingFreeHead;
    g_zClass_TypeList_FreeLinkHead = oldFreeLinkHead;
    g_zClass_ActiveNodeCount = oldActiveNodeCount;
    g_zClass_NodeFreeHeadIndex = oldNodeFreeHeadIndex;
    g_zClass_NodeArray = oldNodeArray;
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = oldHead[i];
        zClass_TypeList::Tail(i) = oldTail[i];
        zClass_TypeList::PendingRemovalDirty(i) = oldPendingDirty[i];
    }
    g_zTurret_RuntimeCount = oldRuntimeCount;
    g_zTurret_NapalmVehicleDestroyAnim = oldNapalmDestroyAnim;
    g_zTurret_CallbackNode = oldCallbackNode;
    g_zTurret_LoadedDefRoot = oldLoadedRoot;
    g_zEffectAnim_EntryCount = oldAnimEntryCount;
    g_zEffectAnim_EntryList = oldAnimEntryList;
    g_OptCatalog_EntryCount = oldEntryCount;
    g_OptCatalog_EntryTable = oldEntryTable;
    g_zArchive_MountedList = oldMountedList;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    CloseHandle(file);
    DeleteFileA(tempPath);

    return networkSkipped ? failed : (failed | 1024);
}

extern "C" int zturret_fire_weapon_smoke(void) {
    const float oldTime = g_Time_AccumulatedTimeSec;
    const float oldNextSpawnScale = g_OptCatalogNextSpawnScale;
    void *const oldPendingSpawnTargetCountPtr = g_OptCatalogPendingSpawnTargetCountPtr;
    void *const oldPendingSpawnTargetListPtr = g_OptCatalogPendingSpawnTargetListPtr;
    void *const oldFreeRuntimeList = g_OptCatalogFreeRuntimeInstanceList;
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const int oldNetworkOptionState = g_OptCatalogNetworkOptionState;
    OptCatalogAllocRuntimeGateCallback oldAllocGate = g_OptCatalog_AllocRuntimeGateCallback;

    zSndSample trailStopSample = {};
    zSndSample trailLoopSample = {};
    OptCatalogEntryDef trailEntry = {};
    OptCatalogTrailRuntimeState trailState = {};
    zVec3 trailTargetPos = {4.0f, 5.0f, 6.0f};
    zVec3 trailTargetVel = {0.0f, 1.0f, 0.0f};
    zVec3 trailSpawnPos = {1.0f, 2.0f, 3.0f};
    zVec3 trailSpawnDir = {0.0f, 0.0f, 1.0f};
    trailEntry.trailStopSample = &trailStopSample;
    trailEntry.trailLoopSample = &trailLoopSample;
    trailEntry.flags = 1u << 14;
    trailState.ownerEntry = &trailEntry;
    trailState.spawnPos = &trailSpawnPos;
    trailState.spawnDir = &trailSpawnDir;

    zTurret_Runtime trailRuntime = {};
    trailRuntime.trailRuntimeState = &trailState;
    trailRuntime.runtimeAimPending = 1;
    trailRuntime.runtimeAimTarget.targetPos = &trailTargetPos;
    trailRuntime.runtimeAimTarget.targetVelocity = &trailTargetVel;
    trailRuntime.fireBurstDuration = 3.0f;
    trailRuntime.postBurstCooldown = 4.0f;
    g_Time_AccumulatedTimeSec = 10.0f;
    g_OptCatalogNextSpawnScale = 2.5f;

    trailRuntime.FireWeapon();

    int failed = 0;
    failed |= trailRuntime.runtimeInstanceActive != 1;
    failed |= trailRuntime.nextFireTime != 17.0f;
    failed |= trailState.spawnScale != 2.5f;
    failed |= trailState.pendingSpawnTargetCountPtr != &trailRuntime.runtimeAimPending;
    failed |= trailState.pendingSpawnTargetListPtr != &trailRuntime.runtimeAimTarget;
    failed |= g_OptCatalogPendingSpawnTargetCountPtr != nullptr;
    failed |= g_OptCatalogNextSpawnScale != 1.0f;

    zClass_NodePartial runtimeWorld = {};
    zClass_NodePartial turretNode = {};
    zClass_Object3DDataPartial turretData = {};
    zClass_NodeFreeListSlot projectileSlot = {};
    zClass_Object3DDataPartial projectileData = {};
    InitObjectNode(turretNode, turretData, "turret");
    InitObjectNode(projectileSlot.node, projectileData, "projectile");

    OptCatalogRuntimeInstanceStorage freeRuntime = {};
    OptCatalogEntryDef projectileEntry = {};
    projectileEntry.velocity = 3.0f;
    projectileEntry.fireFxSelectedSoundIndex = -1;
    projectileEntry.fireFxSelectedEffectIndex = -1;
    projectileEntry.flyoutSelectedEffectIndex = -1;
    freeRuntime.projectileNode = &projectileSlot.node;

    zUtil_PlayerStateStorage playerState = {};
    playerState.variantTag.count = 2;
    playerState.variantTag.tags[0] = 9;
    playerState.variantTag.tags[1] = 8;
    playerState.variantTag.tags[2] = 7;
    unsigned int expectedVariant = 0;
    std::memcpy(&expectedVariant, &playerState.variantTag, sizeof(expectedVariant));

    zInput_GameStateOrMapTablePartial gameState = {};
    gameState.playerState =
        static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&playerState));
    g_GameStateOrMapTable = &gameState;
    g_OptCatalogRuntimeWorld = &runtimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = &freeRuntime;
    g_OptCatalogNetworkOptionState = 0;
    g_OptCatalog_AllocRuntimeGateCallback = nullptr;
    g_Time_AccumulatedTimeSec = 20.0f;

    zVec3 pendingTargetPos = {7.0f, 8.0f, 9.0f};
    zVec3 pendingTargetVel = {1.0f, 0.0f, 0.0f};
    zTurret_Runtime projectileRuntime = {};
    projectileRuntime.weaponCatalogEntry = &projectileEntry;
    projectileRuntime.turretNode = &turretNode;
    projectileRuntime.spawnPos = {1.0f, 2.0f, 3.0f};
    projectileRuntime.fireDir = {0.0f, 0.0f, 1.0f};
    projectileRuntime.spawnVel = {1.0f, 0.0f, 0.0f};
    projectileRuntime.damageModifier = 1.75f;
    projectileRuntime.fireRateSeconds = 2.0f;
    projectileRuntime.runtimeAimPending = 1;
    projectileRuntime.runtimeAimTarget.targetPos = &pendingTargetPos;
    projectileRuntime.runtimeAimTarget.targetVelocity = &pendingTargetVel;

    projectileRuntime.FireWeapon();

    failed |= projectileEntry.activeRuntimeListHead != &freeRuntime;
    failed |= freeRuntime.ownerNode != &turretNode;
    failed |= freeRuntime.origin.x != 1.0f || freeRuntime.origin.y != 2.0f ||
              freeRuntime.origin.z != 3.0f;
    failed |= freeRuntime.dir.z != 1.0f;
    failed |= freeRuntime.velocity.x != 1.0f || freeRuntime.velocity.y != 0.0f ||
              freeRuntime.velocity.z != 3.0f;
    failed |= freeRuntime.variantTag != expectedVariant;
    failed |= freeRuntime.spawnScale != 1.75f;
    failed |= projectileRuntime.nextFireTime != 22.0f;
    failed |= g_OptCatalogPendingSpawnTargetCountPtr != nullptr;
    failed |= g_OptCatalogNextSpawnScale != 1.0f;

    g_Time_AccumulatedTimeSec = oldTime;
    g_OptCatalogNextSpawnScale = oldNextSpawnScale;
    g_OptCatalogPendingSpawnTargetCountPtr = oldPendingSpawnTargetCountPtr;
    g_OptCatalogPendingSpawnTargetListPtr = oldPendingSpawnTargetListPtr;
    g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_OptCatalogNetworkOptionState = oldNetworkOptionState;
    g_OptCatalog_AllocRuntimeGateCallback = oldAllocGate;

    return failed != 0 ? 1 : 0;
}

extern "C" int zturret_fire_weapon_callback_smoke(void) {
    const float oldTime = g_Time_AccumulatedTimeSec;
    const float oldNextSpawnScale = g_OptCatalogNextSpawnScale;
    void *const oldPendingSpawnTargetCountPtr = g_OptCatalogPendingSpawnTargetCountPtr;
    void *const oldPendingSpawnTargetListPtr = g_OptCatalogPendingSpawnTargetListPtr;

    zSndSample trailStopSample = {};
    zSndSample trailLoopSample = {};
    OptCatalogEntryDef trailEntry = {};
    OptCatalogTrailRuntimeState trailState = {};
    zVec3 trailTargetPos = {4.0f, 5.0f, 6.0f};
    zVec3 trailTargetVel = {0.0f, 1.0f, 0.0f};
    zVec3 trailSpawnPos = {1.0f, 2.0f, 3.0f};
    zVec3 trailSpawnDir = {0.0f, 0.0f, 1.0f};
    trailEntry.trailStopSample = &trailStopSample;
    trailEntry.trailLoopSample = &trailLoopSample;
    trailEntry.flags = 1u << 14;
    trailState.ownerEntry = &trailEntry;
    trailState.spawnPos = &trailSpawnPos;
    trailState.spawnDir = &trailSpawnDir;

    zTurret_Runtime runtime = {};
    runtime.trailRuntimeState = &trailState;
    runtime.runtimeAimPending = 1;
    runtime.runtimeAimTarget.targetPos = &trailTargetPos;
    runtime.runtimeAimTarget.targetVelocity = &trailTargetVel;
    runtime.fireBurstDuration = 2.0f;
    runtime.postBurstCooldown = 6.0f;
    g_Time_AccumulatedTimeSec = 30.0f;
    g_OptCatalogNextSpawnScale = 4.0f;

    zTurret_Runtime::FireWeaponCallback(nullptr, &runtime, 99);

    const bool ok = runtime.runtimeInstanceActive == 1 && runtime.nextFireTime == 38.0f &&
                    trailState.spawnScale == 4.0f &&
                    trailState.pendingSpawnTargetCountPtr == &runtime.runtimeAimPending &&
                    trailState.pendingSpawnTargetListPtr == &runtime.runtimeAimTarget;

    g_Time_AccumulatedTimeSec = oldTime;
    g_OptCatalogNextSpawnScale = oldNextSpawnScale;
    g_OptCatalogPendingSpawnTargetCountPtr = oldPendingSpawnTargetCountPtr;
    g_OptCatalogPendingSpawnTargetListPtr = oldPendingSpawnTargetListPtr;
    return ok ? 0 : 1;
}

extern "C" int zturret_damage_and_on_damage_smoke(void) {
    const float oldTime = g_Time_AccumulatedTimeSec;
    const int oldHudCounter = g_Player_HudCounterValue;
    const int oldPrimaryGunDispatchCount = g_HudSensorTracker.primaryGunDispatchCount;
    const int oldDamageContextKind = g_OptCatalog_DamageContextKind;
    const float oldFeedbackScalar = g_OptCatalogDamageFeedbackIntensityScalar;

    zClass_NodePartial damagePartNode = {};
    zClass_NodePartial otherNode = {};
    OptCatalogHitEventPartial hitEvent = {};
    OptCatalogEntryDef entry = {};

    zTurret_Runtime runtime = {};
    runtime.healthCurrent = 75.0f;
    runtime.healthMax = 100.0f;
    runtime.damagePartNode = &damagePartNode;
    runtime.activateOnHitDamage = 2.0f;
    g_Time_AccumulatedTimeSec = 10.0f;
    hitEvent.hitNode = &otherNode;
    const bool filtered =
        runtime.ApplyDamageAndHandleDestruction(25.0f, &entry, &hitEvent) == 0 &&
        runtime.healthCurrent == 75.0f && runtime.activateOnHitTimeout == 12.0f;

    runtime.damagePartNode = nullptr;
    const bool damaged =
        runtime.ApplyDamageAndHandleDestruction(25.0f, &entry, &hitEvent) == 0 &&
        runtime.healthCurrent == 50.0f;

    runtime.healthCurrent = 5.0f;
    runtime.destroyAnimEntry = nullptr;
    runtime.turretNode = nullptr;
    runtime.runtimeInstanceActive = 0;
    entry.flags = 0;
    const bool destroyed =
        runtime.ApplyDamageAndHandleDestruction(5.0f, &entry, &hitEvent) == 1 &&
        runtime.healthCurrent == 0.0f;

    g_OptCatalogDamageFeedbackIntensityScalar = -1.0f;
    runtime.healthCurrent = 25.0f;
    runtime.healthMax = 100.0f;
    runtime.activateOnHitDamage = 0.0f;
    const bool feedback =
        zTurret_Runtime::OnDamage(&runtime, &entry, &hitEvent, 5.0f) == 0 &&
        runtime.healthCurrent == 20.0f &&
        g_OptCatalogDamageFeedbackIntensityScalar == 0.2f;

    g_Player_HudCounterValue = 0;
    g_HudSensorTracker.primaryGunDispatchCount = 0;
    g_OptCatalog_DamageContextKind = 0;
    runtime.healthCurrent = 5.0f;
    runtime.healthMax = 100.0f;
    const bool onDamageDestroyed =
        zTurret_Runtime::OnDamage(&runtime, &entry, &hitEvent, 5.0f) == 0 &&
        g_OptCatalog_DamageContextKind == 1 &&
        g_Player_HudCounterValue == 100000;

    g_Time_AccumulatedTimeSec = oldTime;
    g_Player_HudCounterValue = oldHudCounter;
    g_HudSensorTracker.primaryGunDispatchCount = oldPrimaryGunDispatchCount;
    g_OptCatalog_DamageContextKind = oldDamageContextKind;
    g_OptCatalogDamageFeedbackIntensityScalar = oldFeedbackScalar;

    return filtered && damaged && destroyed && feedback && onDamageDestroyed ? 0 : 1;
}

extern "C" int zturret_shutdown_leaf_smoke(void) {
    auto *runtime = static_cast<zTurret_Runtime *>(::operator new(sizeof(zTurret_Runtime)));
    *runtime = {};
    runtime->trailRuntimeState =
        static_cast<OptCatalogTrailRuntimeState *>(std::malloc(sizeof(OptCatalogTrailRuntimeState)));
    g_zTurret_RuntimeList[0] = runtime;
    g_zTurret_RuntimeList[1] = nullptr;
    g_zTurret_RuntimeCount = 1;
    g_zTurret_LoadedDefRoot = nullptr;
    g_zTurret_CallbackNode = nullptr;

    const std::int32_t freeResult = zTurret_System::FreeAllRuntimes();
    if (freeResult != 0 || g_zTurret_RuntimeCount != 0 || g_zTurret_RuntimeList[0] != nullptr) {
        return 1;
    }

    runtime = static_cast<zTurret_Runtime *>(::operator new(sizeof(zTurret_Runtime)));
    *runtime = {};
    runtime->trailRuntimeState =
        static_cast<OptCatalogTrailRuntimeState *>(std::malloc(sizeof(OptCatalogTrailRuntimeState)));
    g_zTurret_RuntimeList[0] = runtime;
    g_zTurret_RuntimeCount = 1;

    const std::int32_t shutdownResult = zTurret_System::Shutdown();
    return shutdownResult == 0 && g_zTurret_RuntimeCount == 0 && g_zTurret_RuntimeList[0] == nullptr
               ? 0
               : 2;
}
