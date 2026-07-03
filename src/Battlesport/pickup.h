#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "recoil/recoil_callconv.h"

struct OptCatalogEntryDef;
struct PickupType;
struct zEffectAnimEntry;
struct zSndSample;
struct zUtil_SaveGameState;

struct PickupAirdropSpawnRef {
    zClass_NodePartial *carrierNode;
    zClass_NodePartial *dropAttachNode;
    zVec3 worldPos;

    PickupAirdropSpawnRef * InitNodesFromCarrierNodeName(
        const char *carrierNodeName
    );
    zVec3 * GetWorldPos();
    int CanSpawnWithClearance(float clearanceRadius);
    int SpawnPickupTypeAndRelay(int pickupTypeIndex);
    static void __fastcall InitGlobalFromCarrierNodeName(
        const char *carrierNodeName
    );
    static void ShutdownGlobal();
    static int TrySpawnRandomPickupFromGlobal();
};
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupAirdropSpawnRef,
        worldPos
    ) == 0x08
);
RECOIL_STATIC_ASSERT(sizeof(PickupAirdropSpawnRef) == 0x14);

struct PickupBvolHitCallbackContext {
    unsigned char unknown_00[0x24];
    zClass_NodePartial *ownerNode;
};

struct PickupNodeRuntimeFields {
    char visibleName[0x18];
    int pickupId;
    int pickupTypeIndex;
    int amount;
};

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed callers 0x41de70 Pickup::InitAndLoadPuppySpawns, 0x41ccf0 Pickup::Init, 0x41db60 Pickup::AssignBvolGroupAndId, 0x41dab0 Pickup::CreateObjectInstance.
 * Purpose: provide the recovered pickup node fields helper for
 * the Player/Pickup gameplay source cluster.
 */
inline PickupNodeRuntimeFields *PickupNodeFields(
    zClass_NodePartial *node
) {
    return (PickupNodeRuntimeFields *)(node->name);
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed callers 0x41de70 Pickup::InitAndLoadPuppySpawns, 0x41ccf0 Pickup::Init, 0x41db60 Pickup::AssignBvolGroupAndId, 0x41dab0 Pickup::CreateObjectInstance.
 * Purpose: provide the recovered pickup node fields helper for
 * the Player/Pickup gameplay source cluster.
 */
inline const PickupNodeRuntimeFields *PickupNodeFields(
    const zClass_NodePartial *node
) {
    return (const PickupNodeRuntimeFields *)(node->name);
}

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
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupParsedZrdEntry,
        typeDesc
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupParsedZrdEntry,
        amount
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupParsedZrdEntry,
        position
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupParsedZrdEntry,
        rotation
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupParsedZrdEntry,
        unknown_24
    ) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupParsedZrdEntry,
        param
    ) == 0x28
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupParsedZrdEntry,
        unknown_2c
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupParsedZrdEntry,
        respawnDelay
    ) == 0x30
);
RECOIL_STATIC_ASSERT(sizeof(PickupParsedZrdEntry) == 0x34);

struct PickupPkt11CreateDelta {
    zNetworkPacketHeader header;
    unsigned short flags;
    unsigned short reserved_0a;
    int pickupId;
    unsigned short typeKeyIndex;
    unsigned short reserved_12;
    int amount;
    zVec3 position;
    zVec3 rotation;
    float respawnDelay;
};
RECOIL_STATIC_ASSERT(sizeof(PickupPkt11CreateDelta) == 0x34);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupPkt11CreateDelta,
        flags
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupPkt11CreateDelta,
        pickupId
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupPkt11CreateDelta,
        typeKeyIndex
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupPkt11CreateDelta,
        amount
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupPkt11CreateDelta,
        position
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupPkt11CreateDelta,
        rotation
    ) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupPkt11CreateDelta,
        respawnDelay
    ) == 0x30
);

struct PickupPkt12AirdropSpawnChuteRelay {
    zNetworkPacketHeader header;
    zVec3 spawnPos;
    unsigned short pickupTypeIndex;
    unsigned short reserved_16;
    int nextPickupId;
};
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupPkt12AirdropSpawnChuteRelay,
        spawnPos
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupPkt12AirdropSpawnChuteRelay,
        pickupTypeIndex
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupPkt12AirdropSpawnChuteRelay,
        nextPickupId
    ) == 0x18
);
RECOIL_STATIC_ASSERT(sizeof(PickupPkt12AirdropSpawnChuteRelay) == 0x1c);

struct PickupSpawnList {
    void *unused;
    PickupSpawnDef *head;
    PickupSpawnDef *tail;
    int count;

    static void Primary_Init();
    static void NetCopy_Init();
    static void __fastcall RemoveAndFreeNode(
        PickupSpawnDef *node,
        PickupSpawnList *list
    );
    void Clear();
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

    static void Init();
    static void Update();
    void ClearAndFree();
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

    static PickupType *__fastcall GetByIndex_Pure(int pickupTypeIndex);
    static PickupType *__fastcall GetByIndex(int pickupTypeIndex);
    static int __fastcall FindByLogicalName(
        const char *logicalName,
        int *outTypeIndex
    );
};

RECOIL_STATIC_ASSERT(
    offsetof(
        PickupType,
        optMetaImage
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupType,
        pickupSound
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(sizeof(PickupType) == 0x30);

namespace PickupTypeKeyTable {
int __fastcall FindIndex(const char *logicalName);
}

namespace PickupTypeMeta {
PickupType *__fastcall FindByName(const char *typeName);
}

namespace Net {
int __fastcall IsOptEntryActiveInAnySlot(OptCatalogEntryDef *optEntry);
}

extern PickupType g_PickupTypes[40];
extern PickupSpawnList g_PickupSpawnList_NetworkCopy;
extern PickupRespawnQueue g_PickupRespawnQueue;
extern PickupSpawnList g_PickupSpawnList_Primary;
extern int g_NextPickupId;
extern int g_Pickup_LastVTOLDropIndex;
extern zClass_NodePartial *g_Pickup_SceneNode;

extern "C" {
extern PickupAirdropSpawnRef *g_Pickup_GlobalAirdropSpawnRef;
}

namespace Pickup {
int __fastcall Init(
    zClass_NodePartial *sceneNode,
    const char *pickupsCfgPath
);
int InitAndLoadPuppySpawns();
void Shutdown();
int __fastcall ArchiveWriteAll(
    zZbdSectionCallbackCtx *callbackCtx,
    void *userData
);
void __fastcall ArchiveReadRecord(
    zZbdSectionCallbackCtx *callbackCtx,
    const char *sectionToken,
    void *buffer,
    unsigned int size,
    void *userData
);
int __fastcall ResolveOwnerFromBvolHit(zClass_NodePartial **nodeInOut);
PickupSpawnDef *__fastcall FindSpawnByPickupId(
    int pickupId,
    PickupSpawnList *list
);
int __fastcall SpawnListContainsPickupId(
    PickupSpawnDef *spawn,
    PickupSpawnList *list
);
void ReconcilePrimaryAndNetworkCopySpawnLists();
PickupSpawnDef *__fastcall GetSpawnDefFromNode(zClass_NodePartial *pickupNode);
zVidImagePartial *__fastcall FindOptMetaImageByOptEntry(
    OptCatalogEntryDef *optEntry
);
PickupType *__fastcall FindDroppableTypeForPlayerCurrentWeapon(
    zUtil_SaveGameState *saveState
);
void __fastcall RemoveOtherSpawnsWithSameOptEntry(
    OptCatalogEntryDef *optEntry,
    zClass_NodePartial *keepPickupObj
);
int __fastcall SendPkt11_Flag2Delta(PickupSpawnDef *spawn);
int __fastcall SendPkt11_Flag8Delta(PickupSpawnDef *spawn);
void __fastcall SendPkt11_CreateDelta(PickupSpawnDef *spawn);
int __fastcall HandlePkt11_SpawnDelta(
    int senderPlayerId,
    PickupPkt11CreateDelta *packet
);
int __fastcall HandlePkt12_AirdropSpawnChuteRelay(
    int senderPlayerId,
    PickupPkt12AirdropSpawnChuteRelay *packet
);
void __fastcall SendPkt12_AirdropSpawnChuteRelay(
    int pickupTypeIndex,
    zVec3 *spawnPos,
    int nextPickupId
);
int __fastcall AssignBvolGroupAndId(zClass_NodePartial *pickupObj);
zClass_NodePartial *__fastcall CreateObjectInstance(
    int typeIndex,
    int overrideAmount
);
PickupSpawnDef *__fastcall SpawnAt(
    int typeIndex,
    int amount,
    zVec3 *position,
    zVec3 *rotation,
    int spawnParam
);
void __fastcall SpawnAtCarrierNodeByName(
    const char *carrierNodeName,
    int typeIndex,
    int amount
);
int __fastcall SpawnWithAirdropChute(
    int typeIndex,
    zVec3 *position
);
PickupSpawnDef *__fastcall SpawnFromParsedZrdEntry(
    PickupParsedZrdEntry *entry
);
PickupSpawnDef *__fastcall CreateSpawnDefAndLink(
    zClass_NodePartial *pickupObj,
    zVec3 *position,
    zVec3 *rotation,
    int spawnParam,
    int linkToScene
);
void __fastcall RegisterExistingObject(
    int unused,
    zClass_NodePartial *pickupObj,
    int eventValue
);
void __fastcall SetVariantFromTerrain(
    zClass_NodePartial *pickupObj,
    zVec3 *position
);
void __fastcall RespawnSpawnDef(PickupSpawnDef *spawn);
int __fastcall MapVTOLDropGroupVariantToTypeIndex(
    int dropGroupIndex,
    int dropVariantIndex
);
int SelectNextVTOLSpawnTypeIndex();
const char *__fastcall SelectPuppiesZrdByDifficulty(
    const char *extraSearchPath
);
int __fastcall SpawnListHasEntryNearXZ(
    zVec3 *position,
    float clearanceRadius
);
void __fastcall RemoveObject(
    zEffectAnimEntry *animEntry,
    zClass_NodePartial *pickupObj,
    int eventValue
);
int __fastcall OnCollected(
    zClass_NodePartial *hitNode,
    zUtil_SaveGameState *saveState
);
int __fastcall GrantAmmoOrWeapon(
    PickupType *pickupType,
    char *messageBuffer,
    zUtil_SaveGameState *saveState,
    int weaponBankIndex,
    int weaponSideIndex,
    int pairedWeaponSideIndex,
    int overrideAmount
);
int __fastcall ApplyEffect(
    int pickupTypeId,
    int overrideAmount,
    zUtil_SaveGameState *saveState
);
int __fastcall SetNextPickupId(int nextPickupId);
int GetNextPickupId();
} // namespace Pickup

namespace PickupTypeTable {
void FreeOptMeta();
}

RECOIL_STATIC_ASSERT(
    offsetof(
        PickupBvolHitCallbackContext,
        ownerNode
    ) == 0x24
);
RECOIL_STATIC_ASSERT(sizeof(PickupBvolHitCallbackContext) == 0x28);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupNodeRuntimeFields,
        pickupId
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupNodeRuntimeFields,
        pickupTypeIndex
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupNodeRuntimeFields,
        amount
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    sizeof(PickupNodeRuntimeFields) == sizeof(((zClass_NodePartial *)0)->name)
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupSpawnDef,
        amount
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupSpawnDef,
        position
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupSpawnDef,
        rotation
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupSpawnDef,
        pickupObj
    ) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupSpawnDef,
        pickupType
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupSpawnDef,
        spawnParam
    ) == 0x28
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupSpawnDef,
        refCount
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupSpawnDef,
        respawnDelay
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupSpawnDef,
        name
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupSpawnDef,
        next
    ) == 0x50
);
RECOIL_STATIC_ASSERT(sizeof(PickupSpawnDef) == 0x54);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupRespawnEntry,
        spawn
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupRespawnEntry,
        when
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupRespawnEntry,
        next
    ) == 0x08
);
RECOIL_STATIC_ASSERT(sizeof(PickupRespawnEntry) == 0x0c);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupSpawnList,
        head
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupSpawnList,
        tail
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupSpawnList,
        count
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(sizeof(PickupSpawnList) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupRespawnQueue,
        head
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupRespawnQueue,
        tail
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupRespawnQueue,
        count
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(sizeof(PickupRespawnQueue) == 0x10);
