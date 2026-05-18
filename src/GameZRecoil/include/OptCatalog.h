#pragma once

#include "recoil/recoil_types.h"

#include "GameZRecoil/zReader/zReader.h"
#include "zClass.h"

struct zSndSample;
struct zEffectAnimEntry;

struct OptCatalogRuntimeInstanceStorage {
    OptCatalogRuntimeInstanceStorage *next;
    zClass_NodePartial *ownerNode;
    unsigned int variantTag;
    zClass_NodePartial *projectileNode;
    void *flyoutAnimPrimary;
    void *flyoutAnimSecondary;
    zEffectAnimEntry *asyncFxHandle;
    zClass_NodePartial *attachCloneChild;
    zVec3 origin;
    zVec3 aux;
    zVec3 dir;
    zVec3 pos;
    zVec3 velocity;
    void *pendingTargetA;
    void *pendingTargetB;
    float spawnGateAccum;
    float lifetime;
    float speed;
    float unknown_70;
    float rangeProgress;
    float scaleFade;
    float projectileScale;
    float spawnScale;
    void *saveState;
    void *updateCallback;
};

struct OptCatalogHitEventPartial {
    unsigned char unknown_00[0x0c];
    zVec3 hitPos;
    unsigned char unknown_18[0x0c];
    zClass_NodePartial *hitNode;
};

struct OptCatalogEntryDef {
    char *keyName;
    unsigned char unknown_004[0x04];
    char *description;
    char *militaryName;
    int ordinalIndex;
    float fireRateInterval;
    unsigned char unknown_018[0x04];
    float range;
    unsigned char unknown_020[0x14];
    float impactProximity;
    unsigned char unknown_038[0x08];
    float damage;
    unsigned char unknown_044[0x10];
    unsigned int flags;
    OptCatalogRuntimeInstanceStorage *activeRuntimeListHead;
    float flyoutHealth;
    unsigned char unknown_060[0x5c];
    zEffectAnimEntry *flyoutModelAnimationEntry;
    unsigned char unknown_0c0[0x04];
    zClass_NodePartial *attachCloneTemplateNode;
    unsigned char unknown_0c8[0x30];
    void *impactFxTable;
    union {
        zClass_NodePartial *impactNodeListHead;
        zClass_NodePartial *attachCloneChildFreeList;
    };
    unsigned char unknown_100[0x60];
    char *killVerbString;
};

typedef float (RECOIL_FASTCALL *OptCatalogDamageTimerCallback)(void *context, float damageAmount);

namespace OptCatalog {
RECOIL_NOINLINE OptCatalogEntryDef *RECOIL_FASTCALL FindEntryById(int entryId);
RECOIL_NOINLINE void RECOIL_FASTCALL
SetPendingSpawnTargetOverrides(void *pendingSpawnTargetCountPtr, void *pendingSpawnTargetListPtr);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL
AllocOrReuseAttachNodeChildClone(OptCatalogEntryDef *self);
RECOIL_NOINLINE void RECOIL_FASTCALL ClearRuntimeInstanceAsyncFxHandleCallback(
    void *unused, OptCatalogRuntimeInstanceStorage *runtimeInstance, void *unusedStackArg);
RECOIL_NOINLINE OptCatalogRuntimeInstanceStorage *RECOIL_FASTCALL
AllocOrReuseAttachNodeClone(OptCatalogEntryDef *self);
RECOIL_NOINLINE OptCatalogRuntimeInstanceStorage *RECOIL_FASTCALL
SpawnRuntimeInstanceAt(OptCatalogEntryDef *self, zVec3 *spawnPos, zClass_NodePartial *ownerNode);
RECOIL_NOINLINE int RECOIL_CDECL Shutdown();
RECOIL_NOINLINE int RECOIL_CDECL ShutdownCore();
RECOIL_NOINLINE void RECOIL_FASTCALL FreeTrailRuntimeStateStorage(void *trailRuntimeState);
RECOIL_NOINLINE void RECOIL_CDECL PlayTriggerInactiveWarning();
RECOIL_NOINLINE void RECOIL_CDECL PlayWeaponInactiveWarning();
RECOIL_NOINLINE void RECOIL_CDECL PlayNoAmmoWarning();
RECOIL_NOINLINE float RECOIL_FASTCALL CaptureHitSnapshotAndInvokeDamageTimerCallback(
    zVec3 *sourcePos, OptCatalogHitEventPartial *hitEvent, float damageAmount);
} // namespace OptCatalog

extern "C" {
extern int g_OptCatalog_CaptureHitSnapshotEnabled;
extern int g_OptCatalog_FallbackImpactProbeEnabled;
extern zVec3 g_OptCatalog_CapturedDamageSourcePos;
extern zVec3 g_OptCatalog_CapturedDamageHitPos;
extern int g_OptCatalog_EntryCount;
extern OptCatalogEntryDef *g_OptCatalog_EntryTable;
extern int g_OptCatalogRuntimeInstanceCount;
extern void *g_OptCatalogRuntimeInstancePool;
extern void *g_OptCatalogFreeRuntimeInstanceList;
extern zClass_NodePartial *g_OptCatalogRuntimeWorld;
extern void *g_OptCatalogPendingSpawnTargetCountPtr;
extern void *g_OptCatalogPendingSpawnTargetListPtr;
extern float g_OptCatalogMaxCraterRadius;
extern int g_OptCatalogQueuedImpactCount;
extern int g_OptCatalog_DamageContextKind;
extern void *g_OptCatalog_DamageContextHitEvent;
extern void *g_OptCatalogDamageFeedbackCallback;
extern float g_OptCatalogLockOnWarningGateTimeSec;
extern int g_OptCatalog_DamageFeedbackHitCount;
extern zClass_NodePartial *g_OptCatalogDamageFeedbackTrackedNode;
extern float g_OptCatalogNextSpawnScale;
extern zClass_NodePartial *g_OptCatalogThermalGlowFreeList;
extern zReader::Node *g_OptCatalogLoadedTreeRoot;
extern zSndSample *g_OptCatalogSndTriggerInactive;
extern zSndSample *g_OptCatalogSndWeaponInactive;
extern zSndSample *g_OptCatalogSndNoAmmoWarning;
}

RECOIL_STATIC_ASSERT(offsetof(OptCatalogHitEventPartial, hitPos) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogHitEventPartial, hitNode) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(OptCatalogRuntimeInstanceStorage) == 0x8c);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogRuntimeInstanceStorage, next) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogRuntimeInstanceStorage, projectileNode) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogRuntimeInstanceStorage, asyncFxHandle) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogRuntimeInstanceStorage, attachCloneChild) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogRuntimeInstanceStorage, lifetime) == 0x68);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogRuntimeInstanceStorage, updateCallback) == 0x88);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, keyName) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, description) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, militaryName) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, ordinalIndex) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, fireRateInterval) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, range) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, impactProximity) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, damage) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, flags) == 0x54);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, activeRuntimeListHead) == 0x58);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, flyoutHealth) == 0x5c);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, flyoutModelAnimationEntry) == 0xbc);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, attachCloneTemplateNode) == 0xc4);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, impactFxTable) == 0xf8);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, impactNodeListHead) == 0xfc);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, attachCloneChildFreeList) == 0xfc);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, killVerbString) == 0x160);
RECOIL_STATIC_ASSERT(sizeof(OptCatalogEntryDef) == 0x164);
