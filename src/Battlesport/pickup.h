#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "recoil/recoil_callconv.h"

struct OptCatalogEntryDef;
struct PickupType;
struct zEffectAnimEntry;
struct zSndSample;
struct zUtil_SaveGameState;

struct PickupAirdropSpawnRef {
    zClass_NodePartial *carrierNode;
    zClass_NodePartial *dropAttachNode;
    unsigned char unknown_08[0x0c];

    RECOIL_NOINLINE static void RECOIL_CDECL ShutdownGlobal();
};
RECOIL_STATIC_ASSERT(sizeof(PickupAirdropSpawnRef) == 0x14);

struct PickupBvolHitCallbackContext {
    unsigned char unknown_00[0x24];
    zClass_NodePartial *ownerNode;
};

struct PickupSpawnDef {
    int pickupId;
    PickupType *pickupType;
    int amount;
    zVec3 position;
    zVec3 rotation;
    zClass_NodePartial *pickupObj;
    int spawnParam;
    int refCount;
    float respawnDelay;
    unsigned char unknown_34[0x04];
    char name[0x18];
    PickupSpawnDef *next;
};

struct PickupParsedZrdEntry {
    int unknown_00;
    PickupType *typeDesc;
    int amount;
    zVec3 position;
    zVec3 rotation;
    int unknown_24;
    int param;
    int unknown_2c;
    float respawnDelay;
};
RECOIL_STATIC_ASSERT(offsetof(PickupParsedZrdEntry, typeDesc) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(PickupParsedZrdEntry, amount) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(PickupParsedZrdEntry, position) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(PickupParsedZrdEntry, rotation) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(PickupParsedZrdEntry, unknown_24) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(PickupParsedZrdEntry, param) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(PickupParsedZrdEntry, unknown_2c) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(PickupParsedZrdEntry, respawnDelay) == 0x30);
RECOIL_STATIC_ASSERT(sizeof(PickupParsedZrdEntry) == 0x34);

struct PickupSpawnList {
    void *unused;
    PickupSpawnDef *head;
    PickupSpawnDef *tail;
    int count;

    RECOIL_NOINLINE static void RECOIL_CDECL Primary_Init();
    RECOIL_NOINLINE static void RECOIL_CDECL NetCopy_Init();
    RECOIL_NOINLINE static void RECOIL_FASTCALL RemoveAndFreeNode(PickupSpawnDef *node,
                                                                  PickupSpawnList *list);
    RECOIL_NOINLINE void RECOIL_THISCALL Clear();
};

struct PickupRespawnEntry {
    PickupSpawnDef *spawn;
    float when;
    PickupRespawnEntry *next;
};

struct PickupRespawnQueue {
    void *unused;
    PickupRespawnEntry *head;
    PickupRespawnEntry *tail;
    int count;

    RECOIL_NOINLINE static void RECOIL_CDECL Init();
    RECOIL_NOINLINE void RECOIL_THISCALL ClearAndFree();
};

struct PickupType {
    const char *weaponKeyName;
    int msgIdOrClassId;
    int typeIndex;
    int defaultAmount;
    const char *logicalName;
    int nameSuffixMax;
    zClass_NodePartial *templateNode;
    zSndSample *pickupSound;
    zVidImagePartial *optMetaImage;
    int unknown_24;
    OptCatalogEntryDef *optEntry;
    int weaponPresenceCount;

    RECOIL_NOINLINE static PickupType *RECOIL_FASTCALL
    GetByIndex_Pure(int pickupTypeIndex);
    RECOIL_NOINLINE static PickupType *RECOIL_FASTCALL GetByIndex(int pickupTypeIndex);
};

RECOIL_STATIC_ASSERT(offsetof(PickupType, optMetaImage) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(PickupType, pickupSound) == 0x1c);
RECOIL_STATIC_ASSERT(sizeof(PickupType) == 0x30);

namespace PickupTypeKeyTable {
RECOIL_NOINLINE int RECOIL_FASTCALL FindIndex(const char *logicalName);
}

extern PickupType g_PickupTypes[40];
extern PickupSpawnList g_PickupSpawnList_NetworkCopy;
extern PickupRespawnQueue g_PickupRespawnQueue;
extern PickupSpawnList g_PickupSpawnList_Primary;
extern int g_NextPickupId;
extern zClass_NodePartial *g_Pickup_SceneNode;

extern "C" {
extern PickupAirdropSpawnRef *g_Pickup_GlobalAirdropSpawnRef;
}

namespace Pickup {
RECOIL_NOINLINE void RECOIL_CDECL Shutdown();
RECOIL_NOINLINE int RECOIL_FASTCALL ResolveOwnerFromBvolHit(zClass_NodePartial **nodeInOut);
RECOIL_NOINLINE PickupSpawnDef *RECOIL_FASTCALL FindSpawnByPickupId(int pickupId,
                                                                    PickupSpawnList *list);
RECOIL_NOINLINE PickupSpawnDef *RECOIL_FASTCALL GetSpawnDefFromNode(zClass_NodePartial *pickupNode);
RECOIL_NOINLINE zVidImagePartial *RECOIL_FASTCALL
FindOptMetaImageByOptEntry(OptCatalogEntryDef *optEntry);
RECOIL_NOINLINE PickupType *RECOIL_FASTCALL
FindDroppableTypeForPlayerCurrentWeapon(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL
RemoveOtherSpawnsWithSameOptEntry(OptCatalogEntryDef *optEntry, zClass_NodePartial *keepPickupObj);
RECOIL_NOINLINE int RECOIL_FASTCALL SendPkt11_Flag2Delta(PickupSpawnDef *spawn);
RECOIL_NOINLINE int RECOIL_FASTCALL SendPkt11_Flag8Delta(PickupSpawnDef *spawn);
RECOIL_NOINLINE void RECOIL_FASTCALL SendPkt11_CreateDelta(PickupSpawnDef *spawn);
RECOIL_NOINLINE int RECOIL_FASTCALL AssignBvolGroupAndId(zClass_NodePartial *pickupObj);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL CreateObjectInstance(int typeIndex,
                                                                         int overrideAmount);
RECOIL_NOINLINE PickupSpawnDef *RECOIL_FASTCALL SpawnAt(int typeIndex, int amount,
                                                        zVec3 *position, zVec3 *rotation,
                                                        int spawnParam);
RECOIL_NOINLINE PickupSpawnDef *RECOIL_FASTCALL
SpawnFromParsedZrdEntry(PickupParsedZrdEntry *entry);
RECOIL_NOINLINE PickupSpawnDef *RECOIL_FASTCALL CreateSpawnDefAndLink(
    zClass_NodePartial *pickupObj, zVec3 *position, zVec3 *rotation, int spawnParam,
    int linkToScene);
RECOIL_NOINLINE void RECOIL_FASTCALL SetVariantFromTerrain(zClass_NodePartial *pickupObj,
                                                           zVec3 *position);
RECOIL_NOINLINE void RECOIL_FASTCALL RemoveObject(zEffectAnimEntry *animEntry,
                                                  zClass_NodePartial *pickupObj, int eventValue);
RECOIL_NOINLINE int RECOIL_FASTCALL OnCollected(zClass_NodePartial *hitNode,
                                                zUtil_SaveGameState *saveState);
RECOIL_NOINLINE int RECOIL_FASTCALL
GrantAmmoOrWeapon(PickupType *pickupType, char *messageBuffer, zUtil_SaveGameState *saveState,
                  int weaponBankIndex, int weaponSideIndex, int pairedWeaponSideIndex,
                  int overrideAmount);
RECOIL_NOINLINE int RECOIL_FASTCALL
ApplyEffect(int pickupTypeId, int overrideAmount, zUtil_SaveGameState *saveState);
RECOIL_NOINLINE int RECOIL_FASTCALL SetNextPickupId(int nextPickupId);
RECOIL_NOINLINE int RECOIL_CDECL GetNextPickupId();
} // namespace Pickup

namespace PickupTypeTable {
RECOIL_NOINLINE void RECOIL_CDECL FreeOptMeta();
}

RECOIL_STATIC_ASSERT(offsetof(PickupBvolHitCallbackContext, ownerNode) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(PickupBvolHitCallbackContext) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(PickupSpawnDef, amount) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(PickupSpawnDef, position) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(PickupSpawnDef, rotation) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(PickupSpawnDef, pickupObj) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(PickupSpawnDef, pickupType) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(PickupSpawnDef, spawnParam) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(PickupSpawnDef, refCount) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(PickupSpawnDef, respawnDelay) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(PickupSpawnDef, name) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(PickupSpawnDef, next) == 0x50);
RECOIL_STATIC_ASSERT(sizeof(PickupSpawnDef) == 0x54);
RECOIL_STATIC_ASSERT(offsetof(PickupRespawnEntry, spawn) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(PickupRespawnEntry, when) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(PickupRespawnEntry, next) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(PickupRespawnEntry) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(PickupSpawnList, head) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(PickupSpawnList, tail) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(PickupSpawnList, count) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(PickupSpawnList) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(PickupRespawnQueue, head) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(PickupRespawnQueue, tail) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(PickupRespawnQueue, count) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(PickupRespawnQueue) == 0x10);
