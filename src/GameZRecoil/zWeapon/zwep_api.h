#pragma once

/*
 * C declarations of the zWeapon API for the zWeapon units (C++ units see the
 * same functions and data with C linkage in opt_catalog.h, zwep.h,
 * zsave_game.h, zdec.h, zloc.h and zhud_ui.h), with the C layouts of the
 * C++-header types these units use.
 */

#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/zClass/cls_api.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zWeapon/zwep.h"
#include "recoil/recoil_callconv.h"

/* zDEClient event templates (zdec.h layouts; zdec.h is a C++ header). */
typedef struct zDEClient_QSandEventTemplate {
    int featureFlags;
    int pointCount;
    zModel_MaterialPartial* material;
    zModel_MaterialPartial* materialCycle;
    float slope;
    float depth;
    float radius;
    zVec3 center;
    CZNodePartial* damageOwnerNode;
} zDEClient_QSandEventTemplate;

typedef struct zDEClient_CraterEventTemplate {
    int featureFlags;
    int pointCount;
    zModel_MaterialSlot* craterMaterialSlot;
    float slope;
    float depth;
    float radius;
    zVec3 center;
    CZNodePartial* damageOwnerNode;
} zDEClient_CraterEventTemplate;

/* Player timed-hit status (zsave_game.h layout). */
struct PlayerTimedHitStatus {
    unsigned int runtimeFlags;
    OptCatalogEntryDef* hitSource;
    float currentLevel;
    float targetLevel;
    CZNodePartial* lightNode;
    float nextUpdateTime;
    CZNodePartial* lightParentNode;
};

/* Player progress-target slot (zsave_game.h layout). */
struct PlayerProgressTargetSlotRuntime {
    zVec3* targetPos;
    zVec3* targetVelocity;
};

/* Player data (player.cpp; player.h is a C++ header). */
extern zVec3* g_Player_LocalFxOffsetWorldPtr;

/* zWeapon API (zwep_init.c). */
typedef void(__fastcall* zWeaponOptCatalogEntryCallback)(Node* entryNode, OptCatalogEntryDef* entry);

int __fastcall LoadOptCatalogFromPath(
    CZNodePartial* worldNode,
    const char* path,
    int networkState,
    zWeaponOptCatalogEntryCallback entryCallback
);
int __fastcall OnWeaponsSectionPreLoad(zZbdSectionCallbackCtx* callbackCtx, void* userData);
void __fastcall OnWeaponsSectionDataReady(
    zZbdSectionCallbackCtx* callbackCtx,
    const char* sectionToken,
    void* weaponData,
    unsigned int dataSize,
    void* userData
);
void __stdcall SetMaxTetherAltitude(float altitude);

/* OptCatalog API (zwep_ammo.c, zwep_dmg.c, zwep_init.c, zwep_light.c; RecoilNet.cpp defines the
 * network relay callbacks). */
void __fastcall
BlendDirectionTowardTarget(zVec3* direction, const zVec3* targetDirection, float xWeight, float yWeight, float zWeight);
OptCatalogEntryDef* __fastcall FindEntryById(int entryId);
CZNodePartial* __fastcall CreateTrailSegmentNodeFromTemplate(CZNodePartial* templateNode);
OptCatalogTrailRuntimeState* __fastcall CreateTrailRuntimeState(
    OptCatalogEntryDef* entry,
    CZNodePartial* projectileNode,
    zTag4Partial* variantTagPtr,
    void* reserved,
    zVec3* spawnPos,
    zVec3* spawnDir,
    int segmentCount
);
void __fastcall SetPendingSpawnTargetOverrides(void* pendingSpawnTargetCountPtr, void* pendingSpawnTargetListPtr);
int __fastcall AltGunDispatchAllocRuntimeGateCallback(OptCatalogEntryDef* self, void** saveStateSlot);
void __fastcall SendPkt0ARemoveRuntimeRelay(OptCatalogEntryDef* self, zVec3* pointOrVec3, CZNodePartial* ownerNode);
int __fastcall HandlePkt0ARemoveRuntimeRelay(int senderPlayerId, NetPkt0A_RemoveRuntimeRelay* packet);
void __fastcall LoadFxSpecFromReaderNode(Node* parentNode, OptCatalogFxSpec* spec, const char* childName);
CZNodePartial* __fastcall AllocOrReuseAttachNodeChildClone(OptCatalogEntryDef* self);
void __fastcall ClearRuntimeInstanceAsyncFxHandleCallback(
    void* unused,
    OptCatalogRuntimeInstanceStorage* runtimeInstance,
    void* unusedStackArg
);
void __fastcall RecycleAttachNodeClone(OptCatalogEntryDef* self, OptCatalogRuntimeInstanceStorage* runtimeInstance);
OptCatalogRuntimeInstanceStorage* __fastcall AllocOrReuseAttachNodeClone(OptCatalogEntryDef* self);
OptCatalogRuntimeInstanceStorage* __fastcall AllocRuntimeInstance(
    OptCatalogEntryDef* self,
    CZNodePartial* ownerNode,
    zTag4Partial* variantTagOrNull,
    zVec3* spawnPos,
    zVec3* spawnDir,
    zVec3* spawnVelocity,
    void* saveState,
    OptCatalogRuntimeInstanceStorage* runtimeInstanceOrNull
);
void __fastcall RecycleRuntimeInstance(OptCatalogEntryDef* self, OptCatalogRuntimeInstanceStorage* runtimeInstance);
void __fastcall ClearRuntimeInstances(OptCatalogEntryDef* self);
void __fastcall
RecycleRuntimeInstanceStorage(OptCatalogEntryDef* self, OptCatalogRuntimeInstanceStorage* runtimeInstance);
int __fastcall FreeTrailRuntimeStateStorage(void* trailRuntimeState);
int __fastcall DeactivateTrailRuntimeState(OptCatalogTrailRuntimeState* trailRuntimeState);
int __fastcall ActivateTrailRuntimeState(OptCatalogTrailRuntimeState* trailRuntimeState, int playerOrdinal);
void __cdecl PlayTriggerInactiveWarning();
void __cdecl PlayWeaponInactiveWarning();
void __cdecl PlayNoAmmoWarning();
float __fastcall ComputeAimPitchForTarget(
    OptCatalogEntryDef* self,
    const zVec3* origin,
    const zVec3* unusedDirection,
    const zVec3* target,
    float* distanceApproxOut
);
int __fastcall InvokeDamageFeedbackAndHitCallback(
    OptCatalogEntryDef* self,
    CZNodePartial* damageOwnerNode,
    zVec3* sourcePos,
    OptCatalogHitEventPartial* hitEvent,
    float damageAmount
);
void __fastcall SetDamageContext(int contextKind, OptCatalogHitEventPartial* contextHitEvent);
float __fastcall CaptureHitSnapshotAndInvokeDamageTimerCallback(
    zVec3* sourcePos,
    OptCatalogHitEventPartial* hitEvent,
    float damageAmount
);
zVec3* __cdecl GetCapturedHitSourcePtr();
int __fastcall EmitCraterImpactEvent(
    OptCatalogEntryDef* self,
    OptCatalogHitEventPartial* hitEvent,
    CZNodePartial* unusedOwnerNode,
    CZNodePartial* damageOwnerNode
);
void __fastcall EmitQSandImpactEvent(
    OptCatalogEntryDef* self,
    OptCatalogHitEventPartial* hitEvent,
    CZNodePartial* unusedOwnerNode,
    CZNodePartial* damageOwnerNode
);
void __fastcall
PlayImpactSound(OptCatalogEntryDef* self, OptCatalogHitEventPartial* hitEvent, int impactSlot, float gainScale);
void __fastcall HandleImpactEvent(
    OptCatalogEntryDef* self,
    OptCatalogHitEventPartial* hitEvent,
    OptCatalogRuntimeInstanceStorage* runtimeInstance
);
void __fastcall
HandleImpactEventFromRuntimeState(OptCatalogEntryDef* self, OptCatalogRuntimeInstanceStorage* runtimeInstance);
int __fastcall BuildImpactHitList(
    OptCatalogEntryDef* self,
    OptCatalogRuntimeInstanceStorage* runtimeInstance,
    int allowOwnerOnlyHit,
    OptCatalogRaycastHitList* outHitList
);
int __fastcall HandleImpactFromRuntimeProbe(
    OptCatalogEntryDef* self,
    OptCatalogRuntimeInstanceStorage* runtimeInstance,
    OptCatalogRaycastHitList* hitList,
    void* excludedDamageHandler
);
int __fastcall ProcessRuntimeInstance(OptCatalogEntryDef* self, OptCatalogRuntimeInstanceStorage* runtimeInstance);
void __cdecl ProcessRuntimeInstances();
int __fastcall RemoveRuntimeInstance(OptCatalogEntryDef* self, zVec3* pointOrVec3, CZNodePartial* ownerNode);
int __fastcall CanSpawnThroughRay(
    OptCatalogEntryDef* self,
    OptCatalogRaycastHitEntry* hit,
    const zVec3* rayStart,
    const zVec3* rayEnd,
    float* rayLengthOut,
    float* reflectedLengthOut,
    zVec3* reflectedDirOut
);
void __fastcall
PlayBounceSound(OptCatalogEntryDef* self, OptCatalogRaycastHitEntry* hitEvent, int impactSlot, float gainScale);
void __fastcall ReflectAndSortImpactTraceList(
    OptCatalogTrailRuntimeState* runtime,
    float* targetProjectionScratch,
    zVec3* directionOut
);
int __fastcall ComputeTrailImpactResponse(
    OptCatalogEntryDef* self,
    OptCatalogTrailRuntimeState* trailRuntime,
    OptCatalogTrailNodeSlot* segment,
    const zVec3* targetPos
);
void __fastcall UpdateTrailSegmentVisual(OptCatalogTrailNodeSlot* segment);
OptCatalogRuntimeInstanceStorage* __fastcall Begin(OptCatalogEntryDef* entry);
OptCatalogRuntimeInstanceStorage* __fastcall Next(OptCatalogEntryDef* entry);
void __stdcall SetIntensityScalar(float scalar);
int __fastcall UpdateTimedStatus(OptCatalogEntryDef* self, PlayerTimedHitStatus* status, float amount);
void* __cdecl GetCurrentOwnerOrCtx();
OptCatalogEntryDef* __fastcall OptCatalogFindEntryByName(const char* name);
OptCatalogRuntimeInstanceStorage* __fastcall
OptCatalogSpawnRuntimeInstanceAt(OptCatalogEntryDef* self, zVec3* spawnPos, CZNodePartial* ownerNode);
int __cdecl OptCatalogShutdown();
int __fastcall OptCatalogShutdownCore();

extern Node* g_OptCatalogLoadedTreeRoot;

/* PlayerTimedHitStatus operations (zwep_light.c). */
void __fastcall PlayerTimedHitStatusResetFields(PlayerTimedHitStatus* status);
void __fastcall PlayerTimedHitStatusClearLightAndReset(PlayerTimedHitStatus* status);
int __fastcall PlayerTimedHitStatusTickAndUpdateLight(PlayerTimedHitStatus* status, float hitStatus);

/* Foreign C entry points: zsnd_parm.cpp (util.cpp declares it too), zdec.h, zloc.h and zhud_ui.h. */
int __fastcall zSndPlayHandleSetFreqScaled(zSndPlayHandle* playHandle, float scale);
int __fastcall CopyQSandEventTemplateDefaults(zDEClient_QSandEventTemplate* eventTemplate);
int __fastcall InitEventTemplateDefaults(zDEClient_CraterEventTemplate* eventTemplate);
int __fastcall zDEClientCraterInstanceEventMaybeRelay(zDEClient_CraterEventTemplate* eventTemplate);
int __fastcall zDEClientQSandInstanceEventMaybeRelay(zDEClient_QSandEventTemplate* eventTemplate);
char* __fastcall ResolveMessageKeyOrFallback(const char* key);
extern char g_HudSensorTracker_ReadFileFailedFmt[18];
extern char g_HudZrd_Key_Sound[6];

/* Reader entry point and global string table (zreader.h declares them for the C++ units). */
Node* __fastcall zRdrFindTag(Node* parentNode, const char* name);
extern int g_zRndr_GlobalStringCount;
extern char* g_zRndr_GlobalStringTable[100];
