#ifndef GAMEZRECOIL_INCLUDE_OPT_CATALOG_H
#define GAMEZRECOIL_INCLUDE_OPT_CATALOG_H

#pragma once

#include "recoil/recoil_types.h"

#include "GameZRecoil/zReader/zreader.h"
#include "zclass.h"

struct zSndSample;
struct zSndPlayHandle;
struct zEffectAnimEntry;
struct PlayerTimedHitStatus;
struct PlayerProgressTargetSlotRuntime;
struct NetPkt0A_RemoveRuntimeRelay;
struct OptCatalogEntryDef;
struct OptCatalogTrailRuntimeState;
struct OptCatalogFxSpec;
struct zVideo_TextureRecordPartial;
struct OptCatalogRaycastHitEntry;
struct OptCatalogRaycastHitList;

struct OptCatalogTrailNodeSlot {
    zClass_NodePartial *node;
    zVec3 pos;
    zVec3 dir;
    float scale;
};

struct OptCatalogTrailRuntimeState {
    OptCatalogTrailRuntimeState *next;
    OptCatalogTrailRuntimeState *prev;
    OptCatalogEntryDef *ownerEntry;
    zClass_NodePartial *projectileNode;
    zTag4Partial *variantTagPtr;
    zSndPlayHandle *stopSoundHandle;
    float ammoOrChargeMirror;
    zClass_NodePartial *lightNode;
    zVec3 *spawnPos;
    zVec3 *spawnDir;
    float trailBlend;
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

struct OptCatalogFxSpec {
    unsigned int flags;
    zEffectAnimEntry *animationEntry;
    unsigned char unknown_08[0x04];
    zEffectAnimEntry *attachedAnimationEntry;
    zEffectAnimEntry *modelAnimationEntry;
    int effectTemplateIndex;
    zClass_NodePartial *modelNode;
    int soundCount;
    zSndSample *soundSamples[4];
    int bounceSoundCount;
    zSndSample *bounceSoundSamples[6];
};

struct OptCatalogDamageFeedbackVariant {
    float minFeedbackScale;
    zEffectAnimEntry *effect;
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
    unsigned char unknown_14[0x0c];
    union {
        int impactSlot;
        zClass_NodePartial *impactOwnerNode;
    };
};

struct OptCatalogHitEventPartial {
    unsigned char unknown_00[0x0c];
    zVec3 hitPos;
    unsigned char unknown_18[0x08];
    OptCatalogSurfaceMaterialRef *surfaceRef;
    zClass_NodePartial *hitNode;
};

typedef void(__fastcall *OptCatalogImpactCallback)(
    OptCatalogEntryDef *entry,
    OptCatalogHitEventPartial *hitEvent,
    OptCatalogRuntimeInstanceStorage *runtimeInstance
);

struct OptCatalogEntryDef {
    char *keyName;
    char *displayName;
    char *description;
    char *militaryName;
    int ordinalIndex;
    float fireRateInterval;
    float ammoOrChargeMax;
    float range;
    float rangeSq;
    float velocity;
    float turnRate;
    float pitchRate;
    float acceleration;
    float impactProximity;
    float damageFalloffRange;
    float detonationDistSq;
    float damage;
    float gravity;
    float lockOnTime;
    float turnSuspendTime;
    float timedStatusInterpRate;
    unsigned int flags;
    union {
        OptCatalogRuntimeInstanceStorage *activeRuntimeListHead;
        OptCatalogTrailRuntimeState *activeTrailRuntime;
    };
    float flyoutHealth;
    union {
        OptCatalogFxSpec fireFxSpec;
        struct {
            unsigned int fireFxFlags;
            zEffectAnimEntry *fireFxAnimationEntries[4];
            int fireFxEffectTemplateIndex;
            unsigned char unknown_078[0x08];
            zSndSample *fireFxSoundSamples[4];
            unsigned char unknown_090[0x14];
            int fireFxSelectedSoundIndex;
            int fireFxSelectedEffectIndex;
        };
        struct {
            unsigned char unknown_060[0x08];
            zEffectAnimEntry *trailEffectAnim;
            unsigned char unknown_06c[0x14];
            zSndSample *trailStopSample;
            unsigned char unknown_084[0x28];
        };
    };
    union {
        OptCatalogFxSpec flyoutFxSpec;
        struct {
            unsigned char unknown_0ac[0x04];
            zEffectAnimEntry *flyoutAnimationEntry;
            unsigned char unknown_0b4[0x04];
            zEffectAnimEntry *flyoutAttachedAnimationEntry;
            zEffectAnimEntry *flyoutModelAnimationEntry;
            unsigned char unknown_0c0[0x04];
            zClass_NodePartial *attachCloneTemplateNode;
            unsigned char unknown_0c8[0x04];
            zSndSample *trailLoopSample;
            unsigned char unknown_0d0[0x24];
            int flyoutSelectedEffectIndex;
        };
    };
    OptCatalogFxSpec *impactFxTable;
    union {
        zClass_NodePartial *impactNodeListHead;
        zClass_NodePartial *attachCloneChildFreeList;
    };
    int damageFeedbackVariantCount;
    OptCatalogDamageFeedbackVariant damageFeedbackVariants[4];
    zEffectAnimEntry *damageContextEffect;
    int damageMaskSlotIndex;
    int craterRadiusBase;
    int craterRadiusRandomRange;
    unsigned char unknown_134[0x04];
    float timedStatusLightRangeMin;
    float timedStatusLightRangeMax;
    float timedStatusUpdateDelay;
    zColorRgb timedStatusLightSpecularColor;
    float trailSegmentTimeSec;
    unsigned char unknown_154[0x08];
    OptCatalogImpactCallback impactCallback;
    char *killVerbString;
};

typedef float(__fastcall *OptCatalogDamageTimerCallback)(
    void *context,
    float damageAmount
);
typedef int(__fastcall *OptCatalogHitCallback)(
    void *context,
    OptCatalogEntryDef *entry,
    OptCatalogHitEventPartial *hitEvent,
    float damageAmount
);
typedef void(__fastcall *OptCatalogDamageFeedbackCallback)(
    OptCatalogDamageHandlerPartial *handler,
    zClass_NodePartial *hitNode,
    float damageAmount
);
typedef void(__fastcall *OptCatalogRemoveRuntimeRelayCallback)(
    OptCatalogEntryDef *entry,
    zVec3 *pointOrVec3,
    zClass_NodePartial *ownerNode
);

namespace OptCatalog {
void __fastcall BlendDirectionTowardTarget(
    zVec3 *direction,
    const zVec3 *targetDirection,
    float xWeight,
    float yWeight,
    float zWeight
);
OptCatalogEntryDef *__fastcall FindEntryByName(const char *name);
OptCatalogEntryDef *__fastcall FindEntryById(int entryId);
zClass_NodePartial *__fastcall CreateTrailSegmentNodeFromTemplate(
    zClass_NodePartial *templateNode
);
OptCatalogTrailRuntimeState *__fastcall CreateTrailRuntimeState(
    OptCatalogEntryDef *entry,
    zClass_NodePartial *projectileNode,
    zTag4Partial *variantTagPtr,
    void *reserved,
    zVec3 *spawnPos,
    zVec3 *spawnDir,
    int segmentCount
);
void __fastcall SetPendingSpawnTargetOverrides(
    void *pendingSpawnTargetCountPtr,
    void *pendingSpawnTargetListPtr
);
int __fastcall AltGunDispatchAllocRuntimeGateCallback(
    OptCatalogEntryDef *self,
    void **saveStateSlot
);
void __fastcall SendPkt0A_RemoveRuntimeRelay(
    OptCatalogEntryDef *self,
    zVec3 *pointOrVec3,
    zClass_NodePartial *ownerNode
);
int __fastcall HandlePkt0A_RemoveRuntimeRelay(
    int senderPlayerId,
    NetPkt0A_RemoveRuntimeRelay *packet
);
void __fastcall LoadFxSpecFromReaderNode(
    zReader::Node *parentNode,
    OptCatalogFxSpec *spec,
    const char *childName
);
zClass_NodePartial *__fastcall AllocOrReuseAttachNodeChildClone(
    OptCatalogEntryDef *self
);
void __fastcall ClearRuntimeInstanceAsyncFxHandleCallback(
    void *unused,
    OptCatalogRuntimeInstanceStorage *runtimeInstance,
    void *unusedStackArg
);
void __fastcall RecycleAttachNodeClone(
    OptCatalogEntryDef *self,
    OptCatalogRuntimeInstanceStorage *runtimeInstance
);
OptCatalogRuntimeInstanceStorage *__fastcall AllocOrReuseAttachNodeClone(
    OptCatalogEntryDef *self
);
OptCatalogRuntimeInstanceStorage *__fastcall AllocRuntimeInstance(
    OptCatalogEntryDef *self,
    zClass_NodePartial *ownerNode,
    zTag4Partial *variantTagOrNull,
    zVec3 *spawnPos,
    zVec3 *spawnDir,
    zVec3 *spawnVelocity,
    void *saveState,
    OptCatalogRuntimeInstanceStorage *runtimeInstanceOrNull
);
OptCatalogRuntimeInstanceStorage *__fastcall SpawnRuntimeInstanceAt(
    OptCatalogEntryDef *self,
    zVec3 *spawnPos,
    zClass_NodePartial *ownerNode
);
void __fastcall RecycleRuntimeInstance(
    OptCatalogEntryDef *self,
    OptCatalogRuntimeInstanceStorage *runtimeInstance
);
void __fastcall ClearRuntimeInstances(OptCatalogEntryDef *self);
void __fastcall RecycleRuntimeInstanceStorage(
    OptCatalogEntryDef *self,
    OptCatalogRuntimeInstanceStorage *runtimeInstance
);
int Shutdown();
int ShutdownCore();
void __fastcall FreeTrailRuntimeStateStorage(void *trailRuntimeState);
int __fastcall DeactivateTrailRuntimeState(
    OptCatalogTrailRuntimeState *trailRuntimeState
);
void __fastcall ActivateTrailRuntimeState(
    OptCatalogTrailRuntimeState *trailRuntimeState,
    int playerOrdinal
);
void PlayTriggerInactiveWarning();
void PlayWeaponInactiveWarning();
void PlayNoAmmoWarning();
float __fastcall ComputeAimPitchForTarget(
    OptCatalogEntryDef *self,
    const zVec3 *origin,
    const zVec3 *unusedDirection,
    const zVec3 *target,
    float *distanceApproxOut
);
int __fastcall InvokeDamageFeedbackAndHitCallback(
    OptCatalogEntryDef *self,
    zClass_NodePartial *damageOwnerNode,
    zVec3 *sourcePos,
    OptCatalogHitEventPartial *hitEvent,
    float damageAmount
);
void __fastcall SetDamageContext(
    int contextKind,
    OptCatalogHitEventPartial *contextHitEvent
);
void __fastcall SetDamageMaskSlotIndex(int slotIndex);
void __fastcall RegisterDamageMaskSlotPtr(void *slotPtr);
void __fastcall ApplyDamageMaskStampOnHit(OptCatalogHitEventPartial *hitEvent);
float __fastcall CaptureHitSnapshotAndInvokeDamageTimerCallback(
    zVec3 *sourcePos,
    OptCatalogHitEventPartial *hitEvent,
    float damageAmount
);
zVec3 *GetCapturedHitSourcePtr();
int __fastcall EmitCraterImpactEvent(
    OptCatalogEntryDef *self,
    OptCatalogHitEventPartial *hitEvent,
    zClass_NodePartial *unusedOwnerNode,
    zClass_NodePartial *damageOwnerNode
);
void __fastcall EmitQSandImpactEvent(
    OptCatalogEntryDef *self,
    OptCatalogHitEventPartial *hitEvent,
    zClass_NodePartial *unusedOwnerNode,
    zClass_NodePartial *damageOwnerNode
);
void __fastcall PlayImpactSound(
    OptCatalogEntryDef *self,
    OptCatalogHitEventPartial *hitEvent,
    int impactSlot,
    float gainScale
);
void __fastcall HandleImpactEvent(
    OptCatalogEntryDef *self,
    OptCatalogHitEventPartial *hitEvent,
    OptCatalogRuntimeInstanceStorage *runtimeInstance
);
void __fastcall HandleImpactEventFromRuntimeState(
    OptCatalogEntryDef *self,
    OptCatalogRuntimeInstanceStorage *runtimeInstance
);
int __fastcall BuildImpactHitList(
    OptCatalogEntryDef *self,
    OptCatalogRuntimeInstanceStorage *runtimeInstance,
    int allowOwnerOnlyHit,
    OptCatalogRaycastHitList *outHitList
);
int __fastcall HandleImpactFromRuntimeProbe(
    OptCatalogEntryDef *self,
    OptCatalogRuntimeInstanceStorage *runtimeInstance,
    OptCatalogRaycastHitList *hitList,
    void *excludedDamageHandler
);
int __fastcall ProcessRuntimeInstance(
    OptCatalogEntryDef *self,
    OptCatalogRuntimeInstanceStorage *runtimeInstance
);
void ProcessRuntimeInstances();
int __fastcall RemoveRuntimeInstance(
    OptCatalogEntryDef *self,
    zVec3 *pointOrVec3,
    zClass_NodePartial *ownerNode
);
int __fastcall CanSpawnThroughRay(
    OptCatalogEntryDef *self,
    OptCatalogRaycastHitEntry *hit,
    const zVec3 *rayStart,
    const zVec3 *rayEnd,
    float *rayLengthOut,
    float *reflectedLengthOut,
    zVec3 *reflectedDirOut
);
void __fastcall PlayBounceSound(
    OptCatalogEntryDef *self,
    OptCatalogRaycastHitEntry *hitEvent,
    int impactSlot,
    float gainScale
);
void __fastcall ReflectAndSortImpactTraceList(
    OptCatalogTrailRuntimeState *runtime,
    float *targetProjectionScratch,
    zVec3 *directionOut
);
int __fastcall ComputeTrailImpactResponse(
    OptCatalogEntryDef *self,
    OptCatalogTrailRuntimeState *trailRuntime,
    OptCatalogTrailNodeSlot *segment,
    const zVec3 *targetPos
);
void __fastcall UpdateTrailSegmentVisual(OptCatalogTrailNodeSlot *segment);
} // namespace OptCatalog

namespace OptCatalog_MineIterator {
OptCatalogRuntimeInstanceStorage *__fastcall Begin(
    OptCatalogEntryDef *entry
);
OptCatalogRuntimeInstanceStorage *Next();
} // namespace OptCatalog_MineIterator

namespace DamageFeedback {
void __stdcall SetIntensityScalar(float scalar);
} // namespace DamageFeedback

namespace HitSource {
int __fastcall UpdateTimedStatus(
    OptCatalogEntryDef *self,
    PlayerTimedHitStatus *status,
    float amount
);
} // namespace HitSource

namespace HitContext {
void *GetCurrentOwnerOrCtx();
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
extern OptCatalogRuntimeInstanceStorage *g_OptCatalogFreeRuntimeInstanceList;
extern zClass_NodePartial *g_OptCatalogRuntimeWorld;
extern int *g_OptCatalogPendingSpawnTargetCountPtr;
extern PlayerProgressTargetSlotRuntime *g_OptCatalogPendingSpawnTargetListPtr;
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
extern float g_OptCatalogRuntimeDeltaTime;
extern float g_OptCatalogRuntimeNowSec;
extern zClass_NodePartial *g_OptCatalogThermalGlowFreeList;
extern OptCatalogRuntimeInstanceStorage *g_OptCatalog_MineIteratorCursor;
extern zReader::Node *g_OptCatalogLoadedTreeRoot;
extern zSndSample *g_OptCatalogSndTriggerInactive;
extern zSndSample *g_OptCatalogSndWeaponInactive;
extern zSndSample *g_OptCatalogSndNoAmmoWarning;
extern zSndSample *g_OptCatalogSndLockOnWarning;
extern OptCatalogRemoveRuntimeRelayCallback g_OptCatalog_RemoveRuntimeRelayCallback;
typedef int(__fastcall *OptCatalogAllocRuntimeGateCallback)(
    OptCatalogEntryDef *entry,
    void **saveStateSlot
);
extern int g_OptCatalogNetworkOptionState;
extern OptCatalogAllocRuntimeGateCallback g_OptCatalog_AllocRuntimeGateCallback;
extern OptCatalogAllocRuntimeGateCallback g_OptCatalog_AltGunDispatchNoOpCallback;
extern int g_OptCatalogProcessRuntimeRelayEnabled;
}

RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogHitEventPartial,
        hitPos
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogHitEventPartial,
        surfaceRef
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogHitEventPartial,
        hitNode
    ) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogDamageMaskSurface,
        width
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogDamageMaskSurface,
        height
    ) == 0x06
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogDamageMaskSurface,
        format
    ) == 0x0e
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogDamageMaskSurface,
        pixels
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogDamageMaskSurface,
        alpha
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogSurfaceMaterialRef,
        textureHandle
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogSurfaceMaterialRef,
        impactSlot
    ) == 0x20
);
RECOIL_STATIC_ASSERT(sizeof(OptCatalogFxSpec) == 0x4c);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogFxSpec,
        animationEntry
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogFxSpec,
        attachedAnimationEntry
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogFxSpec,
        modelAnimationEntry
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogFxSpec,
        effectTemplateIndex
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogFxSpec,
        modelNode
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogFxSpec,
        soundCount
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogFxSpec,
        soundSamples
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogFxSpec,
        bounceSoundCount
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogFxSpec,
        bounceSoundSamples
    ) == 0x34
);
RECOIL_STATIC_ASSERT(sizeof(OptCatalogDamageFeedbackVariant) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(OptCatalogRuntimeInstanceStorage) == 0x8c);
RECOIL_STATIC_ASSERT(sizeof(OptCatalogTrailNodeSlot) == 0x20);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogTrailNodeSlot,
        pos
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogTrailNodeSlot,
        dir
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogTrailNodeSlot,
        scale
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(sizeof(OptCatalogTrailRuntimeState) == 0x14c);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogTrailRuntimeState,
        projectileNode
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogTrailRuntimeState,
        variantTagPtr
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogTrailRuntimeState,
        stopSoundHandle
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogTrailRuntimeState,
        lightNode
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogTrailRuntimeState,
        trailBlend
    ) == 0x28
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogTrailRuntimeState,
        trailDistance
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogTrailRuntimeState,
        activeNodeSlotCount
    ) == 0x3c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogTrailRuntimeState,
        activeNodeSlots
    ) == 0x44
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogRuntimeInstanceStorage,
        next
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogRuntimeInstanceStorage,
        projectileNode
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogRuntimeInstanceStorage,
        asyncFxHandle
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogRuntimeInstanceStorage,
        attachCloneChild
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogRuntimeInstanceStorage,
        lifetime
    ) == 0x68
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogRuntimeInstanceStorage,
        updateCallback
    ) == 0x88
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        keyName
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        displayName
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        description
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        militaryName
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        ordinalIndex
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        fireRateInterval
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        ammoOrChargeMax
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        range
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        rangeSq
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        velocity
    ) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        turnRate
    ) == 0x28
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        pitchRate
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        acceleration
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        impactProximity
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        damageFalloffRange
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        detonationDistSq
    ) == 0x3c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        damage
    ) == 0x40
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        gravity
    ) == 0x44
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        lockOnTime
    ) == 0x48
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        turnSuspendTime
    ) == 0x4c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        timedStatusInterpRate
    ) == 0x50
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        flags
    ) == 0x54
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        activeRuntimeListHead
    ) == 0x58
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        activeTrailRuntime
    ) == 0x58
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        flyoutHealth
    ) == 0x5c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        fireFxSpec
    ) == 0x60
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        fireFxFlags
    ) == 0x60
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        fireFxAnimationEntries
    ) == 0x64
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        fireFxEffectTemplateIndex
    ) == 0x74
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        fireFxSoundSamples
    ) == 0x80
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        fireFxSelectedSoundIndex
    ) == 0xa4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        fireFxSelectedEffectIndex
    ) == 0xa8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        trailEffectAnim
    ) == 0x68
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        trailStopSample
    ) == 0x80
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        flyoutFxSpec
    ) == 0xac
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        flyoutAnimationEntry
    ) == 0xb0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        flyoutAttachedAnimationEntry
    ) == 0xb8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        flyoutModelAnimationEntry
    ) == 0xbc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        attachCloneTemplateNode
    ) == 0xc4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        trailLoopSample
    ) == 0xcc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        flyoutSelectedEffectIndex
    ) == 0xf4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        impactFxTable
    ) == 0xf8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        impactNodeListHead
    ) == 0xfc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        attachCloneChildFreeList
    ) == 0xfc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        damageFeedbackVariantCount
    ) == 0x100
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        damageFeedbackVariants
    ) == 0x104
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        damageContextEffect
    ) == 0x124
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        damageMaskSlotIndex
    ) == 0x128
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        craterRadiusBase
    ) == 0x12c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        craterRadiusRandomRange
    ) == 0x130
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        timedStatusLightRangeMin
    ) == 0x138
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        timedStatusLightRangeMax
    ) == 0x13c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        timedStatusUpdateDelay
    ) == 0x140
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        timedStatusLightSpecularColor
    ) == 0x144
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        trailSegmentTimeSec
    ) == 0x150
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        impactCallback
    ) == 0x15c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogEntryDef,
        killVerbString
    ) == 0x160
);
RECOIL_STATIC_ASSERT(sizeof(OptCatalogEntryDef) == 0x164);

#endif // GAMEZRECOIL_INCLUDE_OPT_CATALOG_H
