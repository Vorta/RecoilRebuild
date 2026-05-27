#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zUtil/zSaveGame.h"
#include "recoil/recoil_callconv.h"
#include "zClass.h"

struct OptCatalogEntryDef;
struct OptCatalogHitEventPartial;
struct OptCatalogTrailRuntimeState;
struct PlayerProgressTargetSlotRuntime;
struct zEffectAnimEntry;
struct zMat4x3;

struct zTurret_Runtime {
    int flags;
    int scenePathVisible;
    zClass_NodePartial *turretNode;
    zClass_NodePartial *healthyNode;
    zVec3 worldPos;
    zVec3 firePos;
    int weaponBaseMoves;
    int hasMissileLock;
    zClass_NodePartial *deactivateNode;
    unsigned char unknown_034[0x04];
    zClass_NodePartial *partBaseNode;
    zMat4x3 *partBaseMatrix;
    zClass_NodePartial *partBarrelNode;
    zVec3 firePointLocal[2];
    zMat4x3 *partBarrelMatrix;
    zVec3 forward;
    zClass_NodePartial *fireEffectNode;
    float fireEffectDurationSec;
    unsigned char unknown_074[0x04];
    zClass_NodePartial *targetTypes[8];
    OptCatalogEntryDef *weaponCatalogEntry;
    int weaponAmmo;
    float detectionRange;
    float nearestTargetScore;
    int isFiring;
    zEffectAnimEntry *fireAnimEntry;
    float damageModifier;
    int firePointIndex;
    int firePointCount;
    zClass_NodePartial *firePointNode0;
    zClass_NodePartial *firePointNode1;
    zVec3 spawnPos;
    zVec3 fireDir;
    zVec3 spawnVel;
    float fireRateSeconds;
    float nextFireTime;
    float fireBurstTimer;
    float fireBurstDuration;
    float postBurstCooldown;
    float fireDwellTime;
    float fireDwellUntil;
    OptCatalogTrailRuntimeState *trailRuntimeState;
    int runtimeInstanceActive;
    int runtimeAimPending;
    PlayerProgressTargetSlotRuntime runtimeAimTarget;
    unsigned char unknown_118[0x38];
    int enableLosCheck;
    int alwaysLookAtTarget;
    zEffectAnimEntry *destroyAnimEntry;
    float healthCurrent;
    float healthMax;
    zClass_NodePartial *damagePartNode;
    float activateOnHitDamage;
    float activateOnHitTimeout;
    int intersectBvolEnabled;
    unsigned char unknown_174[0x0c];

    RECOIL_NOINLINE zTurret_Runtime *RECOIL_THISCALL InitDefaults();
    RECOIL_NOINLINE void RECOIL_THISCALL
    InitFromReaderNode(zClass_NodePartial *worldNode, zClass_NodePartial *turretWorldNode,
                       zEffectAnimEntry *defaultDestroyAnim, zReader::Node *readerNode);
    RECOIL_NOINLINE void RECOIL_THISCALL UpdateFirePositionFromParts();
    RECOIL_NOINLINE void RECOIL_THISCALL UpdateAimAndPartMatrices(const zVec3 *targetPos);
    RECOIL_NOINLINE void RECOIL_THISCALL SelectFirePointAndAimAtTarget(const zVec3 *targetPos);
    RECOIL_NOINLINE void RECOIL_THISCALL FireWeapon();
    RECOIL_NOINLINE void RECOIL_THISCALL UpdateFireBurstTimer(float deltaTime);
    RECOIL_NOINLINE void RECOIL_THISCALL Tick(const zVec3 *playerFxOffsetWorld);
    RECOIL_NOINLINE int RECOIL_THISCALL
    ApplyDamageAndHandleDestruction(float damageAmount, OptCatalogEntryDef *entry,
                                    OptCatalogHitEventPartial *hitEvent);
    static RECOIL_NOINLINE int RECOIL_FASTCALL OnDamage(zTurret_Runtime *self,
                                                        OptCatalogEntryDef *entry,
                                                        OptCatalogHitEventPartial *hitEvent,
                                                        float damageAmount);
    static RECOIL_NOINLINE void RECOIL_FASTCALL FireWeaponCallback(zEffectAnimEntry *entry,
                                                                   zTurret_Runtime *self,
                                                                   int eventCode);
    RECOIL_NOINLINE int RECOIL_THISCALL Shutdown();
    RECOIL_NOINLINE int RECOIL_THISCALL HasActiveNode();
};

namespace zTurret_LayoutAssertions {
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, flags) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, scenePathVisible) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, turretNode) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, healthyNode) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, worldPos) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, firePos) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, weaponBaseMoves) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, hasMissileLock) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, deactivateNode) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, partBaseNode) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, partBaseMatrix) == 0x3c);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, partBarrelNode) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, firePointLocal) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, partBarrelMatrix) == 0x5c);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, forward) == 0x60);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, fireEffectNode) == 0x6c);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, fireEffectDurationSec) == 0x70);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, targetTypes) == 0x78);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, weaponCatalogEntry) == 0x98);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, weaponAmmo) == 0x9c);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, detectionRange) == 0xa0);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, nearestTargetScore) == 0xa4);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, isFiring) == 0xa8);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, fireAnimEntry) == 0xac);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, damageModifier) == 0xb0);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, firePointIndex) == 0xb4);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, firePointCount) == 0xb8);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, firePointNode0) == 0xbc);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, firePointNode1) == 0xc0);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, spawnPos) == 0xc4);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, fireDir) == 0xd0);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, spawnVel) == 0xdc);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, fireRateSeconds) == 0xe8);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, nextFireTime) == 0xec);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, fireBurstTimer) == 0xf0);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, fireBurstDuration) == 0xf4);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, postBurstCooldown) == 0xf8);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, fireDwellTime) == 0xfc);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, trailRuntimeState) == 0x104);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, runtimeInstanceActive) == 0x108);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, runtimeAimPending) == 0x10c);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, runtimeAimTarget) == 0x110);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, enableLosCheck) == 0x150);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, alwaysLookAtTarget) == 0x154);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, destroyAnimEntry) == 0x158);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, healthCurrent) == 0x15c);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, healthMax) == 0x160);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, damagePartNode) == 0x164);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, activateOnHitDamage) == 0x168);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, activateOnHitTimeout) == 0x16c);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, intersectBvolEnabled) == 0x170);
RECOIL_STATIC_ASSERT(sizeof(zTurret_Runtime) == 0x180);
} // namespace zTurret_LayoutAssertions

extern "C" {
extern zClass_NodePartial *g_zTurret_CallbackNode;
extern zReader::Node *g_zTurret_LoadedDefRoot;
extern zEffectAnimEntry *g_zTurret_NapalmVehicleDestroyAnim;
extern int g_zTurret_RuntimeCount;
extern int g_zTurret_CallbackIterationActive;
extern int g_zTurret_CallbackStartIndex;
extern int g_zTurret_CallbackIterIndex;
extern zTurret_Runtime *g_zTurret_RuntimeList[64];
}

namespace zTurret_System {
RECOIL_NOINLINE int RECOIL_CDECL ResetIterationState();
RECOIL_NOINLINE int RECOIL_FASTCALL LoadDefinitionsFromPath(zClass_NodePartial *worldNode,
                                                            const char *path);
RECOIL_NOINLINE void RECOIL_CDECL TickAllRuntimesRoundRobin();
RECOIL_NOINLINE int RECOIL_CDECL DisableTickCallback();
RECOIL_NOINLINE int RECOIL_CDECL EnableTickCallback();
RECOIL_NOINLINE int RECOIL_CDECL Shutdown();
RECOIL_NOINLINE int RECOIL_CDECL FreeAllRuntimes();
} // namespace zTurret_System
