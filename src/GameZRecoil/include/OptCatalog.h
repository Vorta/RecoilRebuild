#pragma once

#include "recoil/recoil_types.h"

#include "GameZRecoil/zReader/zReader.h"
#include "zClass.h"

struct zSndSample;
struct zSndPlayHandle;
struct zEffectAnimEntry;
struct PlayerTimedHitStatus;
struct PlayerProgressTargetSlotRuntime;
struct OptCatalogEntryDef;
struct OptCatalogTrailRuntimeState;
struct zVideo_TextureRecordPartial;

struct OptCatalogTrailNodeSlot {
    zClass_NodePartial *node;
    unsigned char unknown_04[0x1c];
};

struct OptCatalogTrailRuntimeState {
    OptCatalogTrailRuntimeState *next;
    OptCatalogTrailRuntimeState *prev;
    OptCatalogEntryDef *ownerEntry;
    zClass_NodePartial *projectileNode;
    zTag4Partial *variantTagPtr;
    zSndPlayHandle *stopSoundHandle;
    unsigned char unknown_18[0x04];
    zClass_NodePartial *lightNode;
    zVec3 *spawnPos;
    zVec3 *spawnDir;
    int unknown_28;
    float trailDistance;
    float volumeFadeTimer;
    float alphaPulsePhase;
    float spawnScale;
    int activeNodeSlotCount;
    int activeNodeSlotCursor;
    OptCatalogTrailNodeSlot activeNodeSlots[8];
    int *pendingSpawnTargetCountPtr;
    PlayerProgressTargetSlotRuntime *pendingSpawnTargetListPtr;
};

struct OptCatalogRuntimeInstanceStorage {
    OptCatalogRuntimeInstanceStorage *next;
    zClass_NodePartial *ownerNode;
    unsigned int variantTag;
    zClass_NodePartial *projectileNode;
    zEffectAnimEntry *flyoutAnimPrimary;
    zEffectAnimEntry *flyoutAnimSecondary;
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

struct OptCatalogDamageMaskSurface {
    unsigned char unknown_00[0x04];
    short width;
    short height;
    unsigned char unknown_08[0x06];
    short format;
    unsigned short *pixels;
    unsigned char *alpha;
};

struct OptCatalogSurfaceTextureHandle {
    OptCatalogDamageMaskSurface *surface;
    zVideo_TextureRecordPartial *textureRecord;
};

struct OptCatalogSurfaceMaterialRef {
    unsigned short flags;
    unsigned char unknown_02[0x0e];
    OptCatalogSurfaceTextureHandle *textureHandle;
};

struct OptCatalogHitEventPartial {
    unsigned char unknown_00[0x0c];
    zVec3 hitPos;
    unsigned char unknown_18[0x08];
    OptCatalogSurfaceMaterialRef *surfaceRef;
    zClass_NodePartial *hitNode;
};

struct OptCatalogEntryDef {
    char *keyName;
    char *displayName;
    char *description;
    char *militaryName;
    int ordinalIndex;
    float fireRateInterval;
    float ammoOrChargeMax;
    float range;
    unsigned char unknown_020[0x14];
    float impactProximity;
    unsigned char unknown_038[0x08];
    float damage;
    unsigned char unknown_044[0x10];
    unsigned int flags;
    union {
        OptCatalogRuntimeInstanceStorage *activeRuntimeListHead;
        OptCatalogTrailRuntimeState *activeTrailRuntime;
    };
    float flyoutHealth;
    unsigned char unknown_060[0x08];
    zEffectAnimEntry *trailEffectAnim;
    unsigned char unknown_06c[0x14];
    zSndSample *trailStopSample;
    unsigned char unknown_084[0x38];
    zEffectAnimEntry *flyoutModelAnimationEntry;
    unsigned char unknown_0c0[0x04];
    zClass_NodePartial *attachCloneTemplateNode;
    unsigned char unknown_0c8[0x30];
    void *impactFxTable;
    union {
        zClass_NodePartial *impactNodeListHead;
        zClass_NodePartial *attachCloneChildFreeList;
    };
    unsigned char unknown_100[0x44];
    zColorRgb timedStatusLightSpecularColor;
    unsigned char unknown_150[0x10];
    char *killVerbString;
};

typedef float (RECOIL_FASTCALL *OptCatalogDamageTimerCallback)(void *context, float damageAmount);

namespace OptCatalog {
RECOIL_NOINLINE OptCatalogEntryDef *RECOIL_FASTCALL FindEntryByName(const char *name);
RECOIL_NOINLINE OptCatalogEntryDef *RECOIL_FASTCALL FindEntryById(int entryId);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL
CreateTrailSegmentNodeFromTemplate(zClass_NodePartial *templateNode);
RECOIL_NOINLINE OptCatalogTrailRuntimeState *RECOIL_FASTCALL
CreateTrailRuntimeState(OptCatalogEntryDef *entry, zClass_NodePartial *projectileNode,
                        zTag4Partial *variantTagPtr, void *reserved, zVec3 *spawnPos,
                        zVec3 *spawnDir, int segmentCount);
RECOIL_NOINLINE OptCatalogRuntimeInstanceStorage *RECOIL_FASTCALL
MineIterator_Begin(OptCatalogEntryDef *entry);
RECOIL_NOINLINE OptCatalogRuntimeInstanceStorage *RECOIL_CDECL MineIterator_Next();
RECOIL_NOINLINE void RECOIL_FASTCALL
SetPendingSpawnTargetOverrides(void *pendingSpawnTargetCountPtr, void *pendingSpawnTargetListPtr);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL
AllocOrReuseAttachNodeChildClone(OptCatalogEntryDef *self);
RECOIL_NOINLINE void RECOIL_FASTCALL ClearRuntimeInstanceAsyncFxHandleCallback(
    void *unused, OptCatalogRuntimeInstanceStorage *runtimeInstance, void *unusedStackArg);
RECOIL_NOINLINE void RECOIL_FASTCALL
RecycleAttachNodeClone(OptCatalogEntryDef *self,
                       OptCatalogRuntimeInstanceStorage *runtimeInstance);
RECOIL_NOINLINE OptCatalogRuntimeInstanceStorage *RECOIL_FASTCALL
AllocOrReuseAttachNodeClone(OptCatalogEntryDef *self);
RECOIL_NOINLINE OptCatalogRuntimeInstanceStorage *RECOIL_FASTCALL
SpawnRuntimeInstanceAt(OptCatalogEntryDef *self, zVec3 *spawnPos, zClass_NodePartial *ownerNode);
RECOIL_NOINLINE void RECOIL_FASTCALL
RecycleRuntimeInstance(OptCatalogEntryDef *self,
                       OptCatalogRuntimeInstanceStorage *runtimeInstance);
RECOIL_NOINLINE void RECOIL_FASTCALL ClearRuntimeInstances(OptCatalogEntryDef *self);
RECOIL_NOINLINE void RECOIL_FASTCALL
RecycleRuntimeInstanceStorage(OptCatalogEntryDef *self,
                              OptCatalogRuntimeInstanceStorage *runtimeInstance);
RECOIL_NOINLINE int RECOIL_CDECL Shutdown();
RECOIL_NOINLINE int RECOIL_CDECL ShutdownCore();
RECOIL_NOINLINE void RECOIL_FASTCALL FreeTrailRuntimeStateStorage(void *trailRuntimeState);
RECOIL_NOINLINE int RECOIL_FASTCALL
DeactivateTrailRuntimeState(OptCatalogTrailRuntimeState *trailRuntimeState);
RECOIL_NOINLINE void RECOIL_CDECL PlayTriggerInactiveWarning();
RECOIL_NOINLINE void RECOIL_CDECL PlayWeaponInactiveWarning();
RECOIL_NOINLINE void RECOIL_CDECL PlayNoAmmoWarning();
RECOIL_NOINLINE void RECOIL_FASTCALL SetDamageContext(int contextKind,
                                                       OptCatalogHitEventPartial *contextHitEvent);
RECOIL_NOINLINE void RECOIL_FASTCALL SetDamageMaskSlotIndex(int slotIndex);
RECOIL_NOINLINE void RECOIL_FASTCALL RegisterDamageMaskSlotPtr(void *slotPtr);
RECOIL_NOINLINE void RECOIL_FASTCALL ApplyDamageMaskStampOnHit(
    OptCatalogHitEventPartial *hitEvent);
RECOIL_NOINLINE float RECOIL_FASTCALL CaptureHitSnapshotAndInvokeDamageTimerCallback(
    zVec3 *sourcePos, OptCatalogHitEventPartial *hitEvent, float damageAmount);
RECOIL_NOINLINE zVec3 *RECOIL_CDECL GetCapturedHitSourcePtr();
} // namespace OptCatalog

namespace DamageFeedback {
RECOIL_NOINLINE void RECOIL_STDCALL SetIntensityScalar(float scalar);
} // namespace DamageFeedback

namespace HitSource {
RECOIL_NOINLINE int RECOIL_FASTCALL
UpdateTimedStatus(OptCatalogEntryDef *self, PlayerTimedHitStatus *status, float amount);
} // namespace HitSource

namespace HitContext {
RECOIL_NOINLINE void *RECOIL_CDECL GetCurrentOwnerOrCtx();
} // namespace HitContext

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
extern void *g_OptCatalog_CurrentDamageOwnerOrCtx;
extern void *g_OptCatalogDamageFeedbackCallback;
extern float g_OptCatalogLockOnWarningGateTimeSec;
extern int g_OptCatalog_DamageFeedbackHitCount;
extern zClass_NodePartial *g_OptCatalogDamageFeedbackTrackedNode;
extern float g_OptCatalogDamageFeedbackIntensityScalar;
extern float g_OptCatalogNextSpawnScale;
extern zClass_NodePartial *g_OptCatalogThermalGlowFreeList;
extern OptCatalogRuntimeInstanceStorage *g_OptCatalog_MineIteratorCursor;
extern zReader::Node *g_OptCatalogLoadedTreeRoot;
extern zSndSample *g_OptCatalogSndTriggerInactive;
extern zSndSample *g_OptCatalogSndWeaponInactive;
extern zSndSample *g_OptCatalogSndNoAmmoWarning;
}

RECOIL_STATIC_ASSERT(offsetof(OptCatalogHitEventPartial, hitPos) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogHitEventPartial, surfaceRef) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogHitEventPartial, hitNode) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogDamageMaskSurface, width) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogDamageMaskSurface, height) == 0x06);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogDamageMaskSurface, format) == 0x0e);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogDamageMaskSurface, pixels) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogDamageMaskSurface, alpha) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogSurfaceMaterialRef, textureHandle) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(OptCatalogRuntimeInstanceStorage) == 0x8c);
RECOIL_STATIC_ASSERT(sizeof(OptCatalogTrailNodeSlot) == 0x20);
RECOIL_STATIC_ASSERT(sizeof(OptCatalogTrailRuntimeState) == 0x14c);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogTrailRuntimeState, projectileNode) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogTrailRuntimeState, variantTagPtr) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogTrailRuntimeState, stopSoundHandle) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogTrailRuntimeState, lightNode) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogTrailRuntimeState, activeNodeSlotCount) == 0x3c);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogTrailRuntimeState, activeNodeSlots) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogRuntimeInstanceStorage, next) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogRuntimeInstanceStorage, projectileNode) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogRuntimeInstanceStorage, asyncFxHandle) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogRuntimeInstanceStorage, attachCloneChild) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogRuntimeInstanceStorage, lifetime) == 0x68);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogRuntimeInstanceStorage, updateCallback) == 0x88);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, keyName) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, displayName) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, description) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, militaryName) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, ordinalIndex) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, fireRateInterval) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, ammoOrChargeMax) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, range) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, impactProximity) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, damage) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, flags) == 0x54);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, activeRuntimeListHead) == 0x58);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, activeTrailRuntime) == 0x58);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, flyoutHealth) == 0x5c);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, trailEffectAnim) == 0x68);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, trailStopSample) == 0x80);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, flyoutModelAnimationEntry) == 0xbc);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, attachCloneTemplateNode) == 0xc4);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, impactFxTable) == 0xf8);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, impactNodeListHead) == 0xfc);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, attachCloneChildFreeList) == 0xfc);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, timedStatusLightSpecularColor) == 0x144);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogEntryDef, killVerbString) == 0x160);
RECOIL_STATIC_ASSERT(sizeof(OptCatalogEntryDef) == 0x164);
