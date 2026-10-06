#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "recoil/recoil_callconv.h"
#include "zclass.h"

struct OptCatalogEntryDef;
struct OptCatalogHitEventPartial;
struct OptCatalogTrailRuntimeState;
struct PlayerProgressTargetSlotRuntime;
struct zEffectAnimEntry;
struct zMat4x3;

struct zTurret_Runtime {
    int flags;
    int scenePathVisible;
    CZNodePartial* turretNode;
    CZNodePartial* healthyNode;
    zVec3 worldPos;
    zVec3 firePos;
    int weaponBaseMoves;
    int hasMissileLock;
    CZNodePartial* deactivateNode;
    unsigned char unknown_034[0x04];
    CZNodePartial* partBaseNode;
    zMat4x3* partBaseMatrix;
    CZNodePartial* partBarrelNode;
    zVec3 firePointLocal[2];
    zMat4x3* partBarrelMatrix;
    zVec3 forward;
    CZNodePartial* fireEffectNode;
    float fireEffectDurationSec;
    unsigned char unknown_074[0x04];
    CZNodePartial* targetTypes[8];
    OptCatalogEntryDef* weaponCatalogEntry;
    int weaponAmmo;
    float detectionRange;
    float nearestTargetScore;
    int isFiring;
    zEffectAnimEntry* fireAnimEntry;
    float damageModifier;
    int firePointIndex;
    int firePointCount;
    CZNodePartial* firePointNode0;
    CZNodePartial* firePointNode1;
    zVec3 fireDir;
    zVec3 spawnPos;
    zVec3 spawnVel;
    float fireRateSeconds;
    float nextFireTime;
    float fireBurstTimer;
    float fireBurstDuration;
    float postBurstCooldown;
    float fireDwellTime;
    float fireDwellUntil;
    OptCatalogTrailRuntimeState* trailRuntimeState;
    int runtimeInstanceActive;
    int runtimeAimPending;
    // OptCatalog pending-spawn target slot at 0x110: two pointer fields copied by the
    // zWeapon target-list contract before the remaining turret padding at 0x118.
    PlayerProgressTargetSlotRuntime runtimeAimTarget;
    unsigned char unknown_118[0x38];
    int enableLosCheck;
    int alwaysLookAtTarget;
    zEffectAnimEntry* destroyAnimEntry;
    float healthCurrent;
    float healthMax;
    CZNodePartial* damagePartNode;
    float activateOnHitDamage;
    float activateOnHitTimeout;
    int intersectBvolEnabled;
    int unknown_174[3];

    zTurret_Runtime();
    void InitFromReaderNode(
        CZNodePartial* worldNode,
        CZNodePartial* turretWorldNode,
        zEffectAnimEntry* defaultDestroyAnim,
        zReader::Node* readerNode
    );
    void UpdateFirePositionFromParts();
    void UpdateAimAndPartMatrices(const zVec3* targetPos);
    void SelectFirePointAndAimAtTarget(const zVec3* targetPos);
    void FireWeapon();
    void UpdateFireBurstTimer(float deltaTime);
    void Tick(const zVec3* playerFxOffsetWorld);
    int
    ApplyDamageAndHandleDestruction(float damageAmount, OptCatalogEntryDef* entry, OptCatalogHitEventPartial* hitEvent);
    static int __fastcall
    OnDamage(zTurret_Runtime* self, OptCatalogEntryDef* entry, OptCatalogHitEventPartial* hitEvent, float damageAmount);
    static void __fastcall FireWeaponCallback(zEffectAnimEntry* entry, zTurret_Runtime* self, int eventCode);
    int Shutdown();
    int HasActiveNode();
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
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, fireDir) == 0xc4);
RECOIL_STATIC_ASSERT(offsetof(zTurret_Runtime, spawnPos) == 0xd0);
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

/*
 * Reconstructed turret system storage at [0x4f3fd0, 0x4f41f0). The runtime
 * list capacity is a play-test choice: retail uses the list base and the
 * scalar at 0x4f41ec, but the intervening extent is unresolved (128 pointers
 * plus an unknown word versus 129 pointers). The compatibility macros below
 * expose the recovered names as members of the single definition in turret.cpp.
 */
struct zTurret_SystemState {
    CZNodePartial* callbackNode; /* +0x000 0x4f3fd0 */
    zReader::Node* loadedDefRoot; /* +0x004 0x4f3fd4 */
    int runtimeCount; /* +0x008 0x4f3fd8 */
    int callbackIterationActive; /* +0x00c 0x4f3fdc */
    int callbackStartIndex; /* +0x010 0x4f3fe0 */
    int callbackIterIndex; /* +0x014 0x4f3fe4 */
    zTurret_Runtime* runtimeList[128]; /* +0x018 0x4f3fe8 */
    int unknown_218; /* +0x218 0x4f41e8 */
    zEffectAnimEntry* napalmVehicleDestroyAnim; /* +0x21c 0x4f41ec */
};
RECOIL_STATIC_ASSERT(offsetof(zTurret_SystemState, runtimeList) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zTurret_SystemState, napalmVehicleDestroyAnim) == 0x21c);
RECOIL_STATIC_ASSERT(sizeof(zTurret_SystemState) == 0x220);

extern "C" zTurret_SystemState g_zTurret_SystemStateStorage;

#define g_zTurret_CallbackNode (g_zTurret_SystemStateStorage.callbackNode)
#define g_zTurret_LoadedDefRoot (g_zTurret_SystemStateStorage.loadedDefRoot)
#define g_zTurret_RuntimeCount (g_zTurret_SystemStateStorage.runtimeCount)
#define g_zTurret_CallbackIterationActive (g_zTurret_SystemStateStorage.callbackIterationActive)
#define g_zTurret_CallbackStartIndex (g_zTurret_SystemStateStorage.callbackStartIndex)
#define g_zTurret_CallbackIterIndex (g_zTurret_SystemStateStorage.callbackIterIndex)
#define g_zTurret_RuntimeList (g_zTurret_SystemStateStorage.runtimeList)
#define g_zTurret_NapalmVehicleDestroyAnim (g_zTurret_SystemStateStorage.napalmVehicleDestroyAnim)

namespace zTurret_System {
int __cdecl ResetIterationState();
int __fastcall LoadDefinitionsFromPath(CZNodePartial* worldNode, const char* path);
void __cdecl TickAllRuntimesRoundRobin();
int __cdecl DisableTickCallback();
int __cdecl EnableTickCallback();
int __cdecl Shutdown();
int __cdecl FreeAllRuntimes();
} // namespace zTurret_System
