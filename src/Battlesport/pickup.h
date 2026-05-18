#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "recoil/recoil_callconv.h"

struct OptCatalogEntryDef;

struct PickupAirdropSpawnRef {
    zClass_NodePartial *carrierNode;
    zClass_NodePartial *dropAttachNode;
    unsigned char unknown_08[0x0c];

    RECOIL_NOINLINE static void RECOIL_CDECL ShutdownGlobal();
};
RECOIL_STATIC_ASSERT(sizeof(PickupAirdropSpawnRef) == 0x14);

struct PickupSpawnDef {
    int pickupId;
    unsigned char unknown_04[0x20];
    zClass_NodePartial *pickupObj;
    unsigned char unknown_28[0x04];
    int refCount;
    float respawnDelay;
    unsigned char unknown_34[0x1c];
    PickupSpawnDef *next;
};

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
    int hasWeapon;
    zVidImagePartial *optMetaImage;
    int unknown_24;
    OptCatalogEntryDef *optEntry;
    int weaponPresenceCount;

    RECOIL_NOINLINE static PickupType *RECOIL_FASTCALL
    GetByIndex_Pure(int pickupTypeIndex);
    RECOIL_NOINLINE static PickupType *RECOIL_FASTCALL GetByIndex(int pickupTypeIndex);
};

RECOIL_STATIC_ASSERT(offsetof(PickupType, optMetaImage) == 0x20);
RECOIL_STATIC_ASSERT(sizeof(PickupType) == 0x30);

extern PickupType g_PickupTypes[40];
extern PickupSpawnList g_PickupSpawnList_NetworkCopy;
extern PickupRespawnQueue g_PickupRespawnQueue;
extern PickupSpawnList g_PickupSpawnList_Primary;
extern int g_NextPickupId;

extern "C" {
extern PickupAirdropSpawnRef *g_Pickup_GlobalAirdropSpawnRef;
}

namespace Pickup {
RECOIL_NOINLINE void RECOIL_CDECL Shutdown();
RECOIL_NOINLINE PickupSpawnDef *RECOIL_FASTCALL FindSpawnByPickupId(int pickupId,
                                                                    PickupSpawnList *list);
RECOIL_NOINLINE PickupSpawnDef *RECOIL_FASTCALL GetSpawnDefFromNode(zClass_NodePartial *pickupNode);
RECOIL_NOINLINE zVidImagePartial *RECOIL_FASTCALL
FindOptMetaImageByOptEntry(OptCatalogEntryDef *optEntry);
RECOIL_NOINLINE int RECOIL_FASTCALL SetNextPickupId(int nextPickupId);
RECOIL_NOINLINE int RECOIL_CDECL GetNextPickupId();
} // namespace Pickup

namespace PickupTypeTable {
RECOIL_NOINLINE void RECOIL_CDECL FreeOptMeta();
}

RECOIL_STATIC_ASSERT(offsetof(PickupSpawnDef, pickupObj) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(PickupSpawnDef, refCount) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(PickupSpawnDef, respawnDelay) == 0x30);
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
