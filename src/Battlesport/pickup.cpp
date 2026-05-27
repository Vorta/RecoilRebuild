#include "pickup.h"

#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/include/zDi.h"
#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zUtil/zSaveGame.h"
#include "GameZRecoil/zUtil/zZbd.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PickupType g_PickupTypes[40] = {0};
PickupSpawnList g_PickupSpawnList_NetworkCopy = {0};
PickupRespawnQueue g_PickupRespawnQueue = {0};
PickupSpawnList g_PickupSpawnList_Primary = {0};
int g_NextPickupId = 0;
int g_Pickup_LastVTOLDropIndex = 19;
zClass_NodePartial *g_Pickup_SceneNode = 0;

extern "C" {
PickupAirdropSpawnRef *g_Pickup_GlobalAirdropSpawnRef = 0;
}

struct PickupPkt11Delta {
    zNetworkPacketHeader header;
    unsigned short flags;
    unsigned short reserved;
    int pickupId;
};

struct PickupArchiveRecord {
    int firstRecord;
    int typeIndex;
    int pickupId;
    int amount;
    zVec3 position;
    zVec3 rotation;
    int spawnParam;
    float respawnDelay;
};

RECOIL_STATIC_ASSERT(offsetof(PickupArchiveRecord, typeIndex) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(PickupArchiveRecord, pickupId) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(PickupArchiveRecord, amount) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(PickupArchiveRecord, position) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(PickupArchiveRecord, rotation) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(PickupArchiveRecord, spawnParam) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(PickupArchiveRecord, respawnDelay) == 0x2c);
RECOIL_STATIC_ASSERT(sizeof(PickupArchiveRecord) == 0x30);

namespace {
const float kPickupAltAmmoDisabledSentinel = 123456792.0f;
const char kPickupPuppiesEasyZrd[] = "puppies_easy.zrd";
const char kPickupPuppiesHardZrd[] = "puppies_hard.zrd";
const char kPickupPuppiesDefaultZrd[] = "puppies.zrd";

template <typename T> zZbdSectionCallback ZbdCallbackPtr(T callback) {
    RECOIL_STATIC_ASSERT(sizeof(T) == sizeof(zZbdSectionCallback));
    union {
        T callback;
        zZbdSectionCallback raw;
    } value = {0};
    value.callback = callback;
    return value.raw;
}
} // namespace

PickupPkt11Delta g_PickupPkt11Flag2Delta = {{0x11, sizeof(PickupPkt11Delta), 0}, 0, 0, 0};
PickupPkt11Delta g_PickupPkt11Flag8Delta = {{0x11, sizeof(PickupPkt11Delta), 0}, 0, 0, 0};
PickupPkt12AirdropSpawnChuteRelay g_PickupPkt12AirdropSpawnChuteRelay = {
    {0x12, sizeof(PickupPkt12AirdropSpawnChuteRelay), 0}, {0.0f, 0.0f, 0.0f}, 0, 0, 0};

// Reimplements 0x41db40: PickupType::GetByIndex_Pure (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE PickupType *RECOIL_FASTCALL
PickupType::GetByIndex_Pure(int pickupTypeIndex) {
    if (pickupTypeIndex < 40) {
        return &g_PickupTypes[pickupTypeIndex];
    }

    return 0;
}

// Reimplements 0x41e1c0: PickupType::GetByIndex (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE PickupType *RECOIL_FASTCALL PickupType::GetByIndex(int pickupTypeIndex) {
    if (pickupTypeIndex >= 0 && pickupTypeIndex < 40) {
        return &g_PickupTypes[pickupTypeIndex];
    }

    return 0;
}

// Reimplements 0x41dd60: PickupType::FindByLogicalName
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
PickupType::FindByLogicalName(const char *logicalName, int *outTypeIndex) {
    for (int index = 0; index < 40; ++index) {
        const PickupType &pickupType = g_PickupTypes[index];
        if (pickupType.logicalName != 0 && strcmp(logicalName, pickupType.logicalName) == 0) {
            *outTypeIndex = pickupType.typeIndex;
            return 1;
        }
    }

    *outTypeIndex = 0;
    return 0;
}

namespace PickupTypeKeyTable {
// Reimplements 0x41e1e0: PickupTypeKeyTable::FindIndex
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL FindIndex(const char *logicalName) {
    for (int index = 0; index < 40; ++index) {
        const char *const candidateName = g_PickupTypes[index].logicalName;
        if (candidateName != 0 && strcmp(logicalName, candidateName) == 0) {
            return index;
        }
    }

    return -1;
}
}

namespace PickupTypeMeta {
// Reimplements 0x41e1a0: PickupTypeMeta::FindByName
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE PickupType *RECOIL_FASTCALL FindByName(const char *typeName) {
    const int index = PickupTypeKeyTable::FindIndex(typeName);
    if (index < 0) {
        return 0;
    }

    return &g_PickupTypes[index];
}
} // namespace PickupTypeMeta

namespace Net {
// Reimplements 0x41de30: Net::IsOptEntryActiveInAnySlot
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL IsOptEntryActiveInAnySlot(OptCatalogEntryDef *optEntry) {
    zUtil_PlayerStateStorage *const playerState =
        static_cast<zUtil_PlayerStateStorage *>(static_cast<void *>(g_GameStateOrMapTable->playerState));

    for (int index = 0; index < 10; ++index) {
        PlayerAltWeaponBank &bank = playerState->altWeaponBanks[index];
        if (bank.controllerA.optCatalogEntry == optEntry && (bank.controllerA.flags & 4) != 0) {
            return 1;
        }
        if (bank.controllerB.optCatalogEntry == optEntry && (bank.controllerB.flags & 4) != 0) {
            return 1;
        }
    }

    return 0;
}
} // namespace Net

namespace zClass_Node {
// Reimplements 0x41ceb0: zClass_Node::ClearPickupFlagsRecursive (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL ClearPickupFlagsRecursive(zClass_NodePartial *node) {
    node->flags &= ~0x40018;

    for (int i = 0; i < node->listCountB; ++i) {
        ClearPickupFlagsRecursive(node->listB[i]);
    }

    return 1;
}

// Reimplements 0x41cef0: zClass_Node::SetPickupFlagsRecursive (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL SetPickupFlagsRecursive(zClass_NodePartial *node) {
    node->flags = (node->flags & ~0x08) | 0x40010;

    for (int i = 0; i < node->listCountB; ++i) {
        SetPickupFlagsRecursive(node->listB[i]);
    }

    return 1;
}
} // namespace zClass_Node

// Reimplements 0x438990: PickupAirdropSpawnRef::InitNodesFromCarrierNodeName
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE PickupAirdropSpawnRef *RECOIL_THISCALL
PickupAirdropSpawnRef::InitNodesFromCarrierNodeName(const char *carrierNodeName) {
    carrierNode = zClass::FindByTypeAndName(6, carrierNodeName);
    dropAttachNode = zClass_Class::FindSubNodeByName(carrierNode, "healthy");
    return this;
}

// Reimplements 0x438a70: PickupAirdropSpawnRef::GetWorldPos
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE zVec3 *RECOIL_THISCALL PickupAirdropSpawnRef::GetWorldPos() {
    gwNode::GetWorldPosition(carrierNode, &worldPos);
    return &worldPos;
}

// Reimplements 0x438a20: PickupAirdropSpawnRef::CanSpawnWithClearance
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL
PickupAirdropSpawnRef::CanSpawnWithClearance(float clearanceRadius) {
    if ((dropAttachNode->flags & 4) == 0) {
        return 0;
    }

    zUtil_PlayerStateStorage *const playerState =
        static_cast<zUtil_PlayerStateStorage *>(static_cast<void *>(g_GameStateOrMapTable->playerState));
    if (playerState->activeAltGunController->ammoOrCharge == kPickupAltAmmoDisabledSentinel) {
        return 0;
    }

    return Pickup::SpawnListHasEntryNearXZ(GetWorldPos(), clearanceRadius) == 0;
}

// Reimplements 0x4389c0: PickupAirdropSpawnRef::SpawnPickupTypeAndRelay
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL
PickupAirdropSpawnRef::SpawnPickupTypeAndRelay(int pickupTypeIndex) {
    if ((dropAttachNode->flags & 4) == 0) {
        return 0;
    }

    if (zOpt::GetNetworkEnabled() != 0) {
        if (zNetwork::IsHost() == 0) {
            return 0;
        }

        Pickup::SendPkt12_AirdropSpawnChuteRelay(pickupTypeIndex, &worldPos,
                                                  Pickup::GetNextPickupId());
    }

    Pickup::SpawnWithAirdropChute(pickupTypeIndex, &worldPos);
    return 1;
}

// Reimplements 0x438a90: PickupAirdropSpawnRef::InitGlobalFromCarrierNodeName
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
PickupAirdropSpawnRef::InitGlobalFromCarrierNodeName(const char *carrierNodeName) {
    PickupAirdropSpawnRef *const spawnRef = new PickupAirdropSpawnRef;
    g_Pickup_GlobalAirdropSpawnRef = spawnRef->InitNodesFromCarrierNodeName(carrierNodeName);

    if (g_Pickup_GlobalAirdropSpawnRef->carrierNode == 0 ||
        g_Pickup_GlobalAirdropSpawnRef->dropAttachNode == 0) {
        ::operator delete(g_Pickup_GlobalAirdropSpawnRef);
        g_Pickup_GlobalAirdropSpawnRef = 0;
    }
}

// Reimplements 0x438b10: PickupAirdropSpawnRef::ShutdownGlobal
RECOIL_NOINLINE void RECOIL_CDECL PickupAirdropSpawnRef::ShutdownGlobal() {
    PickupAirdropSpawnRef *const spawnRef = g_Pickup_GlobalAirdropSpawnRef;
    if (spawnRef != 0) {
        ::operator delete(spawnRef);
    }

    g_Pickup_GlobalAirdropSpawnRef = 0;
}

// Reimplements 0x438b30: PickupAirdropSpawnRef::TrySpawnRandomPickupFromGlobal
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_CDECL PickupAirdropSpawnRef::TrySpawnRandomPickupFromGlobal() {
    zOpt::GetNetworkEnabled();
    if (g_Pickup_GlobalAirdropSpawnRef->CanSpawnWithClearance(20.0f) == 0) {
        return 0;
    }

    return g_Pickup_GlobalAirdropSpawnRef->SpawnPickupTypeAndRelay(
        Pickup::SelectNextVTOLSpawnTypeIndex());
}

// Reimplements 0x41cc10: PickupSpawnList::Primary_Init (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE void RECOIL_CDECL PickupSpawnList::Primary_Init() {
    g_PickupSpawnList_Primary.unused = 0;
    g_PickupSpawnList_Primary.tail = 0;
    g_PickupSpawnList_Primary.head = 0;
    g_PickupSpawnList_Primary.count = 0;
}

// Reimplements 0x41cc40: PickupSpawnList::NetCopy_Init (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE void RECOIL_CDECL PickupSpawnList::NetCopy_Init() {
    g_PickupSpawnList_NetworkCopy.unused = 0;
    g_PickupSpawnList_NetworkCopy.tail = 0;
    g_PickupSpawnList_NetworkCopy.head = 0;
    g_PickupSpawnList_NetworkCopy.count = 0;
}

// Reimplements 0x41cc70: PickupRespawnQueue::Init (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE void RECOIL_CDECL PickupRespawnQueue::Init() {
    g_PickupRespawnQueue.unused = 0;
    g_PickupRespawnQueue.tail = 0;
    g_PickupRespawnQueue.head = 0;
    g_PickupRespawnQueue.count = 0;
}

// Reimplements 0x41d8a0: PickupSpawnList::RemoveAndFreeNode (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL PickupSpawnList::RemoveAndFreeNode(PickupSpawnDef *node,
                                                                        PickupSpawnList *list) {
    if (node != 0 && list->count != 0) {
        PickupSpawnDef *current = list->head;
        if (node == current) {
            --list->count;
            PickupSpawnDef *const next = node->next;
            list->head = next;
            if (next == 0) {
                list->unused = 0;
                list->tail = 0;
            }
        } else {
            while (current != 0) {
                PickupSpawnDef *const next = current->next;
                if (next == node) {
                    --list->count;
                    current->next = node->next;
                    if (list->tail == node) {
                        list->tail = current;
                    }
                    break;
                }
                current = next;
            }
        }
    }

    zClass_NodePartial *const pickupObj = node->pickupObj;
    if (pickupObj != 0) {
        if (pickupObj->listA != 0) {
            zClass_World::RemoveChildAtGrid(pickupObj->listA[0], pickupObj);
        }

        zClass_Util::DestroyNodeRecursive(pickupObj);
    }

    free(node);
}

// Reimplements 0x41e240: PickupSpawnList::Clear (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL PickupSpawnList::Clear() {
    PickupSpawnDef *node = head;
    while (node != 0) {
        PickupSpawnDef *const next = node->next;
        PickupSpawnList::RemoveAndFreeNode(node, this);
        node = next;
    }

    if (this == &g_PickupSpawnList_Primary) {
        g_NextPickupId = 0;
    }
}

// Reimplements 0x41e270: PickupRespawnQueue::ClearAndFree (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL PickupRespawnQueue::ClearAndFree() {
    PickupRespawnEntry *node = head;
    while (node != 0) {
        PickupRespawnEntry *const nextNode = node->next;
        bool removed = false;
        if (count != 0) {
            PickupRespawnEntry *current = head;
            if (node == current) {
                --count;
                head = node->next;
                removed = true;
                if (head == 0) {
                    unused = 0;
                    tail = 0;
                }
            } else {
                while (current != 0) {
                    PickupRespawnEntry *const next = current->next;
                    if (next == node) {
                        --count;
                        current->next = node->next;
                        if (tail == node) {
                            tail = current;
                        }
                        removed = true;
                        break;
                    }
                    current = next;
                }
            }
        }

        if (removed) {
            ::operator delete(node);
        }
        node = nextNode;
    }
}

// Reimplements 0x41e5d0: PickupRespawnQueue::Update (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE void RECOIL_CDECL PickupRespawnQueue::Update() {
    if (g_PickupRespawnQueue.count == 0) {
        return;
    }

    PickupRespawnEntry *entry = g_PickupRespawnQueue.head;
    if (entry == 0) {
        return;
    }

    while (entry != 0) {
        if (entry->when < g_Time_UnscaledAccumulatedTimeSec) {
            Pickup::RespawnSpawnDef(entry->spawn);

            PickupRespawnEntry *const nextEntry = entry->next;
            if (g_PickupRespawnQueue.count != 0) {
                PickupRespawnEntry *prev = g_PickupRespawnQueue.head;
                if (entry == prev) {
                    --g_PickupRespawnQueue.count;
                    g_PickupRespawnQueue.head = entry->next;
                    if (g_PickupRespawnQueue.head == 0) {
                        g_PickupRespawnQueue.unused = 0;
                        g_PickupRespawnQueue.tail = 0;
                    }
                    ::operator delete(entry);
                } else if (prev != 0) {
                    while (prev != 0) {
                        PickupRespawnEntry *const prevNext = prev->next;
                        if (prevNext == entry) {
                            --g_PickupRespawnQueue.count;
                            prev->next = entry->next;
                            if (g_PickupRespawnQueue.tail == entry) {
                                g_PickupRespawnQueue.tail = prev;
                            }
                            ::operator delete(entry);
                            break;
                        }
                        prev = prevNext;
                    }
                }
            }
            entry = nextEntry;
        } else {
            entry = entry->next;
        }
    }
}

namespace Pickup {
// Reimplements 0x41ddf0: Pickup::SelectPuppiesZrdByDifficulty
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE const char *RECOIL_FASTCALL
SelectPuppiesZrdByDifficulty(const char *extraSearchPath) {
    const char *filename = kPickupPuppiesDefaultZrd;
    const int difficultyMode = zOpt::GetGameDifficultyMode();
    if (difficultyMode == 0) {
        filename = kPickupPuppiesEasyZrd;
    } else if (difficultyMode == 2) {
        filename = kPickupPuppiesHardZrd;
    }

    if (zReader::TryResolvePath(filename, extraSearchPath) == 0) {
        filename = kPickupPuppiesDefaultZrd;
    }

    return filename;
}

// Reimplements 0x41de70: Pickup::InitAndLoadPuppySpawns
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_CDECL InitAndLoadPuppySpawns() {
    for (int index = 17; index <= 33; ++index) {
        PickupType &pickupType = g_PickupTypes[index];
        if (pickupType.weaponKeyName != 0) {
            pickupType.optEntry = OptCatalog::FindEntryByName(pickupType.weaponKeyName);
            pickupType.weaponPresenceCount = 0;
        }
    }

    g_PickupSpawnList_Primary.Clear();

    zClass::FindNextByTypePrefix("pu", 6);
    zClass_NodePartial *pickupObj = zClass::FindNextByTypePrefix(0, 6);
    while (pickupObj != 0) {
        if (strlen(pickupObj->name) > 5 &&
            isdigit(static_cast<unsigned char>(pickupObj->name[2])) != 0) {
            zVec3 zeroVec = {0.0f, 0.0f, 0.0f};
            *(int *)(pickupObj->name + 0x18) = g_NextPickupId;
            if (AssignBvolGroupAndId(pickupObj) != 0) {
                PickupSpawnDef *const spawn =
                    CreateSpawnDefAndLink(pickupObj, &zeroVec, &zeroVec, 0, 0);
                if (spawn->pickupType->weaponKeyName != 0) {
                    ++spawn->pickupType->weaponPresenceCount;
                }
            }
        }

        pickupObj = zClass::FindNextByTypePrefix(0, 6);
    }

    zReader::Node *const treeRoot =
        zReader::LoadNodeFromPath(SelectPuppiesZrdByDifficulty(0), 0, 0);
    if (treeRoot == 0) {
        return 0;
    }

    zReader::Node *const rootFields = treeRoot->value.nodes;
    zReader::Node *const spawnList = rootFields[1].value.nodes;
    const int spawnCount = spawnList[0].value.i32 - 1;
    for (int index = 0; index < spawnCount; ++index) {
        zReader::Node *const entryFields = spawnList[index + 1].value.nodes;
        PickupType *const pickupType = PickupTypeMeta::FindByName(entryFields[1].value.str);
        if (pickupType == 0) {
            continue;
        }

        if (zOpt::GetNetworkEnabled() == 0 && pickupType->weaponKeyName != 0 &&
            Net::IsOptEntryActiveInAnySlot(pickupType->optEntry) != 0) {
            continue;
        }

        zReader::Node *const position = entryFields[3].value.nodes;
        zReader::Node *const rotation = entryFields[4].value.nodes;
        PickupParsedZrdEntry parsedEntry = {};
        parsedEntry.typeDesc = pickupType;
        parsedEntry.amount = entryFields[2].value.i32;
        parsedEntry.position.x = position[1].value.f32;
        parsedEntry.position.y = position[2].value.f32;
        parsedEntry.position.z = position[3].value.f32;
        parsedEntry.rotation.x = rotation[1].value.f32;
        parsedEntry.rotation.y = rotation[2].value.f32;
        parsedEntry.rotation.z = rotation[3].value.f32;
        parsedEntry.param = 1;
        parsedEntry.unknown_2c = 0;
        parsedEntry.respawnDelay = entryFields[5].value.f32;

        SpawnFromParsedZrdEntry(&parsedEntry);
        if (pickupType->weaponKeyName != 0) {
            ++pickupType->weaponPresenceCount;
        }
    }

    zUtil_PlayerStateStorage *const playerState =
        static_cast<zUtil_PlayerStateStorage *>(static_cast<void *>(g_GameStateOrMapTable->playerState));
    for (int index = 17; index <= 33; ++index) {
        PickupType &pickupType = g_PickupTypes[index];
        if (pickupType.weaponPresenceCount != 0) {
            ++g_HudSensorTracker.weaponsFoundMask;
        } else if (zOpt::GetNetworkEnabled() != 0 && pickupType.weaponKeyName != 0 &&
                   index < 32) {
            const int bankIndex = pickupType.weaponKeyName[4] - '0';
            const int sideIndex = pickupType.weaponKeyName[6] - '0';
            PlayerAltWeaponBank &bank = playerState->altWeaponBanks[bankIndex];
            PlayerGunFireController *const controller =
                sideIndex == 0 ? &bank.controllerA : &bank.controllerB;
            if (controller->ammoOrCharge != 0.0f) {
                pickupType.weaponPresenceCount = 1;
            }
        }
    }

    zReader::FreeLoadedTree(treeRoot);

    if (zOpt::GetNetworkEnabled() != 0) {
        g_PickupSpawnList_NetworkCopy.Clear();
        PickupSpawnDef *primarySpawn = g_PickupSpawnList_Primary.head;
        while (primarySpawn != 0) {
            PickupSpawnDef *const copy =
                static_cast<PickupSpawnDef *>(malloc(sizeof(PickupSpawnDef)));
            if (copy != 0) {
                memset(copy, 0, sizeof(*copy));
                memcpy(copy, primarySpawn, sizeof(*copy));
                copy->next = 0;
                if (g_PickupSpawnList_NetworkCopy.count == 0) {
                    g_PickupSpawnList_NetworkCopy.head = copy;
                } else {
                    g_PickupSpawnList_NetworkCopy.tail->next = copy;
                }
                g_PickupSpawnList_NetworkCopy.tail = copy;
                copy->next = 0;
                ++g_PickupSpawnList_NetworkCopy.count;
            }

            primarySpawn = primarySpawn->next;
        }
    }

    return 1;
}

// Reimplements 0x41e430: Pickup::SpawnListHasEntryNearXZ
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL SpawnListHasEntryNearXZ(zVec3 *position,
                                                            float clearanceRadius) {
    PickupSpawnDef *spawn = g_PickupSpawnList_Primary.head;
    while (spawn != 0) {
        if (fabs(spawn->position.x - position->x) < clearanceRadius &&
            fabs(spawn->position.z - position->z) < clearanceRadius) {
            return 1;
        }
        spawn = spawn->next;
    }

    return 0;
}

// Reimplements 0x41e540: Pickup::MapVTOLDropGroupVariantToTypeIndex
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
MapVTOLDropGroupVariantToTypeIndex(int dropGroupIndex, int dropVariantIndex) {
    if (dropGroupIndex < 2 || dropGroupIndex > 9) {
        return 0;
    }

    return (dropGroupIndex - 2) * 2 + 1 + (dropVariantIndex != 0);
}

// Reimplements 0x41e480: Pickup::SelectNextVTOLSpawnTypeIndex
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_CDECL SelectNextVTOLSpawnTypeIndex() {
    zUtil_PlayerStateStorage *const playerState =
        static_cast<zUtil_PlayerStateStorage *>(static_cast<void *>(g_GameStateOrMapTable->playerState));
    int cursor = g_Pickup_LastVTOLDropIndex + 1;

    while (cursor != g_Pickup_LastVTOLDropIndex) {
        if (cursor >= 20) {
            cursor = 3;
        }

        if (cursor != 18 && cursor != 19) {
            const int dropVariantIndex = cursor & 1;
            const int dropGroupIndex = cursor >> 1;
            int available = 0;

            if (zOpt::GetNetworkEnabled() != 0) {
                available = g_PickupTypes[14 + cursor].weaponPresenceCount != 0;
            } else {
                PlayerAltWeaponBank *const bank = &playerState->altWeaponBanks[dropGroupIndex];
                PlayerGunFireController *const controller =
                    dropVariantIndex != 0 ? &bank->controllerB : &bank->controllerA;
                available = (controller->flags & 4) != 0;
            }

            if (available != 0) {
                g_Pickup_LastVTOLDropIndex = cursor;
                return MapVTOLDropGroupVariantToTypeIndex(dropGroupIndex, dropVariantIndex);
            }
        }

        ++cursor;
    }

    g_Pickup_LastVTOLDropIndex = 3;
    return MapVTOLDropGroupVariantToTypeIndex(1, 1);
}

// Reimplements 0x41ccf0: Pickup::Init (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL Init(zClass_NodePartial *sceneNode,
                                         const char *pickupsCfgPath) {
    g_Pickup_SceneNode = sceneNode;

    zSndSample *const defaultPickupSound = zSnd::FindSampleByName("snd_pickup");
    for (int index = 0; index < 40; ++index) {
        PickupType &pickupType = g_PickupTypes[index];

        char templateName[0x28];
        sprintf(templateName, "pu%03d", pickupType.typeIndex);
        pickupType.templateNode = zClass::FindByTypeAndName(6, templateName);
        pickupType.pickupSound = defaultPickupSound;
        pickupType.nameSuffixMax = 0;

        zClass_NodePartial *const templateNode = pickupType.templateNode;
        if (templateNode != 0) {
            *(int *)(templateNode->name + 0x18) = 0;
            *(int *)(templateNode->name + 0x1c) = pickupType.typeIndex;
            *(int *)(templateNode->name + 0x20) = pickupType.defaultAmount;
        }
    }

    zReader::Node *const rootNode = zReader::LoadNodeFromPath(pickupsCfgPath, 0, 0);
    if (rootNode == 0) {
        zError::ReportOld(0x200, "D:\\Proj\\Battlesport\\pickup.cpp", 0xc1,
                          "Pickup: Unable to load %s", pickupsCfgPath);
        return 0;
    }

    zReader::Node *const pickupDataNode = zReader_GetNamedNode(rootNode, "PICKUP_DATA");
    if (pickupDataNode != 0) {
        zReader::Node *const pickupData = pickupDataNode->value.nodes;
        const int pickupDataCount = pickupData[0].value.i32;
        for (int fieldIndex = 1; fieldIndex < pickupDataCount; fieldIndex += 2) {
            int pickupTypeIndex = 0;
            const char *const logicalName = pickupData[fieldIndex].value.str;
            if (PickupType::FindByLogicalName(logicalName, &pickupTypeIndex) == 0) {
                continue;
            }

            PickupType &pickupType = g_PickupTypes[pickupTypeIndex];
            zReader::Node *const entryNode = zReader_GetNamedNode(pickupDataNode, logicalName);
            zReader::Node *const soundNode = zReader_GetNamedNode(entryNode, "SOUND");
            if (soundNode != 0) {
                zSndSample *const pickupSound =
                    zSnd::FindSampleByName(soundNode->value.nodes[1].value.str);
                if (pickupSound != 0) {
                    pickupType.pickupSound = pickupSound;
                } else {
                    pickupType.pickupSound = defaultPickupSound;
                }
            }

            zReader::Node *const imageNode = zReader_GetNamedNode(entryNode, "IMAGE");
            if (imageNode != 0 && pickupType.optMetaImage == 0) {
                pickupType.optMetaImage =
                    zImage::TexDir_FindOrCreateByPath(imageNode->value.nodes[1].value.str);
            }
        }
    }

    zReader::FreeLoadedTree(rootNode);
    zUtil_ZAR::RegisterSectionHandler("Pickup", ZbdCallbackPtr(&ArchiveWriteAll),
                                      ZbdCallbackPtr(&ArchiveReadRecord), 300, 0);
    return 1;
}

// Reimplements 0x433e40: Pickup::SendPkt11_Flag2Delta
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL SendPkt11_Flag2Delta(PickupSpawnDef *spawn) {
    g_PickupPkt11Flag2Delta.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    g_PickupPkt11Flag2Delta.flags = 2;
    g_PickupPkt11Flag2Delta.pickupId = spawn->pickupId;
    return zNetwork_SendPacketReliable(&g_PickupPkt11Flag2Delta.header);
}

// Reimplements 0x433e70: Pickup::SendPkt11_Flag8Delta
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL SendPkt11_Flag8Delta(PickupSpawnDef *spawn) {
    g_PickupPkt11Flag8Delta.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    g_PickupPkt11Flag8Delta.flags = 8;
    g_PickupPkt11Flag8Delta.pickupId = spawn->pickupId;
    return zNetwork_SendPacketReliable(&g_PickupPkt11Flag8Delta.header);
}

// Reimplements 0x433ea0: Pickup::SendPkt11_CreateDelta
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SendPkt11_CreateDelta(PickupSpawnDef *spawn) {
    PickupPkt11CreateDelta *const packet =
        static_cast<PickupPkt11CreateDelta *>(malloc(sizeof(PickupPkt11CreateDelta)));
    memset(packet, 0, sizeof(PickupPkt11CreateDelta));

    packet->header.packetType = 0x11;
    packet->header.packetSizeBytes = sizeof(PickupPkt11CreateDelta);
    packet->header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    packet->flags = 1;
    packet->pickupId = spawn->pickupId;
    packet->typeKeyIndex =
        static_cast<unsigned short>(PickupTypeKeyTable::FindIndex(spawn->pickupType->logicalName));
    packet->amount = spawn->amount;
    packet->position = spawn->position;
    packet->rotation = spawn->rotation;
    packet->respawnDelay = spawn->respawnDelay;

    zNetwork_SendPacketReliable(&packet->header);
    free(packet);
}

// Reimplements 0x433f40: Pickup::HandlePkt11_SpawnDelta
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
HandlePkt11_SpawnDelta(int, PickupPkt11CreateDelta *packet) {
    PickupSpawnDef *const spawn =
        FindSpawnByPickupId(packet->pickupId, &g_PickupSpawnList_Primary);
    const unsigned int flags = packet->flags;

    if ((flags & 1u) != 0 && spawn == 0) {
        PickupParsedZrdEntry entry;
        memset(&entry, 0, sizeof(entry));
        entry.typeDesc = PickupType::GetByIndex(static_cast<int>(packet->typeKeyIndex));
        entry.amount = packet->amount;
        entry.position = packet->position;
        entry.rotation = packet->rotation;
        entry.respawnDelay = packet->respawnDelay;

        PickupSpawnDef *const newSpawn = SpawnFromParsedZrdEntry(&entry);
        if (newSpawn != 0) {
            newSpawn->pickupId = packet->pickupId;
        }
        SetNextPickupId(packet->pickupId + 1);
        return 1;
    }

    if (spawn == 0 || spawn->refCount != 0) {
        return 1;
    }

    if ((flags & 2u) != 0) {
        PickupSpawnList::RemoveAndFreeNode(spawn, &g_PickupSpawnList_Primary);
        return 1;
    }

    if ((flags & 8u) != 0) {
        zClass_NodePartial *const pickupObj = spawn->pickupObj;
        zClass_Node::ClearPickupFlagsRecursive(pickupObj);
        zClass_Class::gwNodeSetRaycastable(pickupObj, 0);
        zClass_Class::gwNodeSetPickable(pickupObj, 0);
        RemoveObject(0, pickupObj, 0);
    }

    return 1;
}

// Reimplements 0x434050: Pickup::SendPkt12_AirdropSpawnChuteRelay
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
SendPkt12_AirdropSpawnChuteRelay(int pickupTypeIndex, zVec3 *spawnPos, int nextPickupId) {
    g_PickupPkt12AirdropSpawnChuteRelay.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    g_PickupPkt12AirdropSpawnChuteRelay.spawnPos = *spawnPos;
    g_PickupPkt12AirdropSpawnChuteRelay.pickupTypeIndex =
        static_cast<unsigned short>(pickupTypeIndex);
    g_PickupPkt12AirdropSpawnChuteRelay.nextPickupId = nextPickupId;
    zNetwork_SendPacketReliable(&g_PickupPkt12AirdropSpawnChuteRelay.header);
}

// Reimplements 0x4340a0: Pickup::HandlePkt12_AirdropSpawnChuteRelay
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
HandlePkt12_AirdropSpawnChuteRelay(int, PickupPkt12AirdropSpawnChuteRelay *packet) {
    SetNextPickupId(packet->nextPickupId);
    SpawnWithAirdropChute(static_cast<int>(packet->pickupTypeIndex), &packet->spawnPos);
    return 1;
}

// Reimplements 0x41db60: Pickup::AssignBvolGroupAndId
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL AssignBvolGroupAndId(zClass_NodePartial *pickupObj) {
    zClass_NodePartial *const bvolNode = zClass_Class::FindSubNodeByName(pickupObj, "bvol");
    if (bvolNode == 0) {
        zError::ReportOld(0x400, "D:\\Proj\\Battlesport\\pickup.cpp", 0x152,
                          "Pickup: (%s) has no bvol child node", pickupObj);
        return 0;
    }

    const int parsedSuffix = atol(pickupObj->name + 2);
    const int pickupTypeIndex = parsedSuffix / 100;
    const int pickupNameSuffix = parsedSuffix % 100;
    if (pickupTypeIndex > 40) {
        return 0;
    }

    *(int *)(pickupObj->name + 0x1c) = pickupTypeIndex;
    *(int *)(pickupObj->name + 0x18) = g_NextPickupId;
    ++g_NextPickupId;

    PickupType *const pickupType = PickupType::GetByIndex_Pure(pickupTypeIndex);
    if (pickupType != 0 && pickupNameSuffix + 1 > pickupType->nameSuffixMax) {
        pickupType->nameSuffixMax = pickupNameSuffix + 1;
    }

    pickupObj->flags = (pickupObj->flags & ~0x08) | 0x20;
    zClass_Class::gwNodeSetActive(bvolNode, 0);
    zClass_Node::SetDiFlagBit0Recursive(pickupObj, 0);
    return 1;
}

// Reimplements 0x41dab0: Pickup::CreateObjectInstance
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL CreateObjectInstance(int typeIndex,
                                                                         int overrideAmount) {
    PickupType *const pickupType = PickupType::GetByIndex_Pure(typeIndex);
    if (pickupType == 0 || pickupType->templateNode == 0) {
        return 0;
    }

    zClass_NodePartial *const pickupObj =
        zClass_cls_util::CopyNodeWithCloneOptions(pickupType->templateNode, 1, 0);
    if (pickupObj == 0) {
        return 0;
    }

    char pickupName[0x40];
    sprintf(pickupName, "pu%03d%02d", pickupType->typeIndex, pickupType->nameSuffixMax);
    zClass_Class::gwNodeSetName(pickupObj, pickupName);

    int amount = overrideAmount;
    if (amount == 0) {
        amount = pickupType->defaultAmount;
    }
    *(int *)(pickupObj->name + 0x20) = amount;

    if (AssignBvolGroupAndId(pickupObj) != 0) {
        return pickupObj;
    }

    zClass_Util::DestroyNodeRecursive(pickupObj);
    return 0;
}

// Reimplements 0x41d920: Pickup::CreateSpawnDefAndLink
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE PickupSpawnDef *RECOIL_FASTCALL CreateSpawnDefAndLink(
    zClass_NodePartial *pickupObj, zVec3 *position, zVec3 *rotation, int spawnParam,
    int linkToScene) {
    if (linkToScene != 0) {
        zClass_Class::AddChild(g_Pickup_SceneNode, pickupObj);
    }

    PickupSpawnDef *const spawn =
        static_cast<PickupSpawnDef *>(malloc(sizeof(PickupSpawnDef)));

    PickupType *pickupType = 0;
    const int pickupTypeIndex = *(int *)(pickupObj->name + 0x1c);
    for (int index = 0; index < 40; ++index) {
        if (g_PickupTypes[index].typeIndex == pickupTypeIndex) {
            pickupType = &g_PickupTypes[index];
            break;
        }
    }

    spawn->pickupId = *(int *)(pickupObj->name + 0x18);
    spawn->pickupType = pickupType;
    spawn->amount = *(int *)(pickupObj->name + 0x20);
    spawn->position = *position;
    if (rotation != 0) {
        spawn->rotation = *rotation;
    } else {
        spawn->rotation.x = 0.0f;
        spawn->rotation.y = 0.0f;
        spawn->rotation.z = 0.0f;
    }

    spawn->pickupObj = pickupObj;
    spawn->spawnParam = spawnParam;
    spawn->refCount = 0;
    spawn->respawnDelay = 0.0f;
    spawn->next = 0;

    if (g_PickupSpawnList_Primary.count == 0) {
        g_PickupSpawnList_Primary.head = spawn;
    } else {
        g_PickupSpawnList_Primary.tail->next = spawn;
    }
    g_PickupSpawnList_Primary.tail = spawn;
    spawn->next = 0;
    ++g_PickupSpawnList_Primary.count;

    zClass_Node::SetContextRecursive(pickupObj, (zClass_NodePartial *)spawn, 0x240000);
    return spawn;
}

// Reimplements 0x41dcf0: Pickup::RegisterExistingObject
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL RegisterExistingObject(int, zClass_NodePartial *pickupObj,
                                                            int) {
    zVec3 worldPos;
    gwNode::GetWorldPosition(pickupObj, &worldPos);

    if (pickupObj->listCountA == 1 && pickupObj->listA[0] != 0) {
        zClass_Class::RemoveChild(pickupObj->listA[0], pickupObj);
    }

    zClass_Class::gwNodeSetActive(pickupObj, 1);
    zClass_Object3D::gwObject3DSetPosition(pickupObj, worldPos.x, worldPos.y, worldPos.z);
    CreateSpawnDefAndLink(pickupObj, &worldPos, 0, 0, 1);
}

// Reimplements 0x41dc60: Pickup::SpawnWithAirdropChute
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL SpawnWithAirdropChute(int typeIndex, zVec3 *position) {
    zClass_NodePartial *const pickupObj = CreateObjectInstance(typeIndex, 0);
    if (pickupObj == 0) {
        return 0;
    }

    SetVariantFromTerrain(pickupObj, position);

    zEffectAnimEntry *const chuteTemplate = zEffectAnim::FindEntryByName("chutes");
    zEffectAnimEntry *const chuteEntry = zEffectAnim::SetTransformRotAndVelocity_Thunk(
        chuteTemplate, 0, position->x, position->y, position->z, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f);
    zClass_NodePartial *const chuteRoot = zEffectAnim::GetRootNodeOrNull(chuteEntry);
    zClass_NodePartial *const attachNode = zClass_Class::FindSubNodeByName(chuteRoot, "airdroppup");
    zClass_Class::AddChild(attachNode, pickupObj);
    zClass_Class::gwNodeSetActive(pickupObj, 0);
    zEffectAnimEntry::SetOnStateDoneCallback(
        chuteEntry, (void *)&RegisterExistingObject, pickupObj);
    return 1;
}

// Reimplements 0x41da20: Pickup::SpawnAt
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE PickupSpawnDef *RECOIL_FASTCALL SpawnAt(int typeIndex, int amount,
                                                        zVec3 *position, zVec3 *rotation,
    int spawnParam) {
    zClass_NodePartial *const pickupObj = CreateObjectInstance(typeIndex, amount);
    if (pickupObj == 0) {
        return 0;
    }

    SetVariantFromTerrain(pickupObj, position);
    zClass_Object3D::gwObject3DSetPosition(pickupObj, position->x, position->y, position->z);

    if (rotation != 0) {
        zClass_Object3D::gwObject3DSetRotation(pickupObj, rotation->x, rotation->y, rotation->z);
    }

    PickupSpawnDef *const spawn =
        CreateSpawnDefAndLink(pickupObj, position, rotation, spawnParam, 1);
    strcpy(spawn->name, pickupObj->name);
    return spawn;
}

// Reimplements 0x41ea30: Pickup::SpawnAtCarrierNodeByName
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SpawnAtCarrierNodeByName(const char *carrierNodeName,
                                                              int typeIndex, int amount) {
    zClass_NodePartial *const carrierNode = zClass::FindByTypeAndName(6, carrierNodeName);
    if (carrierNode == 0) {
        return;
    }

    zVec3 position;
    gwNode::GetWorldPosition(carrierNode, &position);

    zVec3 rotation;
    zClass_Object3D::gwObject3DGetRotation(carrierNode, &rotation.x, &rotation.y, &rotation.z);
    SpawnAt(typeIndex, amount, &position, &rotation, 0);
}

// Reimplements 0x41dc30: Pickup::SpawnFromParsedZrdEntry
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE PickupSpawnDef *RECOIL_FASTCALL
SpawnFromParsedZrdEntry(PickupParsedZrdEntry *entry) {
    PickupSpawnDef *const spawn =
        SpawnAt(entry->typeDesc->typeIndex, entry->amount, &entry->position, &entry->rotation,
                entry->param);
    if (spawn != 0) {
        spawn->respawnDelay = entry->respawnDelay;
    }
    return spawn;
}

// Reimplements 0x41e330: Pickup::SetVariantFromTerrain
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SetVariantFromTerrain(zClass_NodePartial *pickupObj,
                                                           zVec3 *position) {
    zTag4::Clear(&g_VariantTag_Current);
    g_Variant_CurrentTag = g_VariantTag_Current;

    PlayerProbeSampleCandidateBuffer candidateBuffer;
    zClass_cls_di::BuildPickCandidateListBelowPoint(g_Pickup_SceneNode, &candidateBuffer,
                                                    position->x, 500.0f, position->z);

    int bestCandidateIndex;
    int selectedImpactSlot;
    float taggedHeight;
    Player::SelectProbeSampleHeightFromCandidates(&candidateBuffer, &bestCandidateIndex,
                                                  position->y, 0.5f, 1, &selectedImpactSlot,
                                                  &taggedHeight);

    int variantTag = 255;
    if (candidateBuffer.candidateCount != 0) {
        zClassDiPickCandidateEntry *const candidate =
            &candidateBuffer.entries[bestCandidateIndex];
        zClass_NodePartial *const worldChild = zClass_Class::gwNodeGetWorldChild(candidate->node);
        if (worldChild != 0) {
            variantTag = worldChild->nodeType;
        } else {
            variantTag = candidate->variantTag.tags[0];
        }
    }

    zClass_Class::gwNodeSetNodeType(pickupObj, variantTag);
    zDi::SetVariantTagIfUnset((zDiPartial *)(pickupObj->userDataOrDiRef), variantTag);
}

// Reimplements 0x41e6c0: Pickup::RespawnSpawnDef
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL RespawnSpawnDef(PickupSpawnDef *spawn) {
    zClass_NodePartial *const pickupObj = spawn->pickupObj;

    zClass_Class::gwNodeSetActive(pickupObj, 1);
    zClass_Class::gwNodeSetRaycastable(pickupObj, 1);
    zClass_Class::gwNodeSetPickable(pickupObj, 1);
    zClass_Class::gwNodeSetName(pickupObj, spawn->name);
    zClass_Node::SetPickupFlagsRecursive(pickupObj);

    zClass_Object3D::gwObject3DSetPosition(pickupObj, spawn->position.x, spawn->position.y,
                                           spawn->position.z);
    zClass_Object3D::gwObject3DSetRotation(pickupObj, spawn->rotation.x, spawn->rotation.y,
                                           spawn->rotation.z);
    zClass_Object3D::gwObject3DSetScale(pickupObj, 1.0f, 1.0f, 1.0f);
    zClass_Object3D::gwObject3DSetLitFlag(pickupObj, 1);
    zClass_Object3D::gwObject3DSetAlphaScale(pickupObj, 0.0f);
    zClass_Object3D_ModelRefLerpQueue::Add(pickupObj, 0, 0, 0.0f, 1.0f, 7.0f);
}

// Reimplements 0x41cf50: Pickup::RemoveObject
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL RemoveObject(zEffectAnimEntry *animEntry,
                                                  zClass_NodePartial *pickupObj, int) {
    PickupSpawnDef *const spawn = (PickupSpawnDef *)(pickupObj->callbackContext);
    zClass_Class::gwNodeSetActive(pickupObj, 0);

    if (spawn != 0 && spawn->respawnDelay != 0.0f) {
        if (zOpt::GetNetworkEnabled() != 0 && animEntry != 0) {
            SendPkt11_Flag8Delta(spawn);
        }

        PickupRespawnEntry *const respawnEntry =
            static_cast<PickupRespawnEntry *>(::operator new(sizeof(PickupRespawnEntry)));
        respawnEntry->spawn = 0;
        respawnEntry->when = 0.0f;
        respawnEntry->next = 0;

        if (g_PickupRespawnQueue.count == 0) {
            g_PickupRespawnQueue.head = respawnEntry;
        } else {
            g_PickupRespawnQueue.tail->next = respawnEntry;
        }

        g_PickupRespawnQueue.tail = respawnEntry;
        respawnEntry->next = 0;
        ++g_PickupRespawnQueue.count;
        respawnEntry->spawn = spawn;
        respawnEntry->when = g_Time_UnscaledAccumulatedTimeSec + spawn->respawnDelay;
        return;
    }

    if (pickupObj->listCountA == 1 && pickupObj->listA[0] != 0) {
        if (zOpt::GetNetworkEnabled() != 0 && spawn != 0) {
            SendPkt11_Flag2Delta(spawn);
        }

        zClass_Class::RemoveChild(pickupObj->listA[0], pickupObj);
    }

    if (spawn != 0 && g_PickupSpawnList_Primary.count != 0) {
        PickupSpawnDef *current = g_PickupSpawnList_Primary.head;
        if (spawn == current) {
            --g_PickupSpawnList_Primary.count;
            g_PickupSpawnList_Primary.head = spawn->next;
            if (spawn->next == 0) {
                g_PickupSpawnList_Primary.unused = 0;
                g_PickupSpawnList_Primary.tail = 0;
            }
            free(spawn);
            return;
        }

        if (current != 0) {
            while (current->next != spawn) {
                current = current->next;
                if (current == 0) {
                    free(spawn);
                    return;
                }
            }

            --g_PickupSpawnList_Primary.count;
            current->next = spawn->next;
            if (g_PickupSpawnList_Primary.tail == spawn) {
                g_PickupSpawnList_Primary.tail = current;
            }
        }
    }

    free(spawn);
}

// Reimplements 0x41d0c0: Pickup::OnCollected
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL OnCollected(zClass_NodePartial *hitNode,
                                                zUtil_SaveGameState *saveState) {
    zClass_NodePartial *pickupObj = hitNode;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (ResolveOwnerFromBvolHit(&pickupObj) == 0) {
        return 0;
    }

    const int pickupTypeId = *(int *)(pickupObj->name + 0x1c);
    PickupSpawnDef *spawn = GetSpawnDefFromNode(pickupObj);
    if (spawn == 0 || ApplyEffect(pickupTypeId, spawn->amount, saveState) == 0) {
        return 0;
    }

    zClass_Node::ClearPickupFlagsRecursive(pickupObj);
    zClass_Class::gwNodeSetRaycastable(pickupObj, 0);
    zClass_Class::gwNodeSetPickable(pickupObj, 0);

    PickupType *const pickupType = &g_PickupTypes[pickupTypeId];
    if (pickupType->weaponKeyName != 0) {
        g_HudSensorTracker.ShowObjectivePickupInfo(1, 1, pickupType->optEntry);
        if (zOpt::GetNetworkEnabled() == 0) {
            RemoveOtherSpawnsWithSameOptEntry(pickupType->optEntry, pickupObj);
        }
    }

    char pickupAnimName[8];
    sprintf(pickupAnimName, "pu%03d", pickupTypeId);
    zEffectAnimEntry *const animEntry = zEffectAnim::FindEntryByName(pickupAnimName);
    if (animEntry == 0) {
        RemoveObject(0, pickupObj, 0);
        return 1;
    }

    zVec3 worldPosition;
    gwNode::GetWorldPosition(pickupObj, &worldPosition);
    pickupType->pickupSound->PlayA3DSimple(1.0f);
    zClass_Class::gwNodeSetName(pickupObj, pickupAnimName);
    zEffectAnimEntry *const runtimeEntry = zEffectAnim::SetTransformRefs_Thunk(
        animEntry, pickupObj, pickupObj, &worldPosition, playerState->rootNode, 0);
    zEffectAnimEntry::SetOnStateDoneCallback(runtimeEntry, (void *)RemoveObject, pickupObj);
    GetSpawnDefFromNode(pickupObj)->refCount = 1;
    return 1;
}

// Reimplements 0x41e890: Pickup::ReconcilePrimaryAndNetworkCopySpawnLists
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE void RECOIL_CDECL ReconcilePrimaryAndNetworkCopySpawnLists() {
    PickupSpawnDef *primarySpawn = g_PickupSpawnList_Primary.head;
    while (primarySpawn != 0) {
        if (SpawnListContainsPickupId(primarySpawn, &g_PickupSpawnList_NetworkCopy) == 0) {
            SendPkt11_CreateDelta(primarySpawn);
        }

        primarySpawn = primarySpawn->next;
    }

    PickupSpawnDef *networkCopySpawn = g_PickupSpawnList_NetworkCopy.head;
    while (networkCopySpawn != 0) {
        if (SpawnListContainsPickupId(networkCopySpawn, &g_PickupSpawnList_Primary) == 0) {
            SendPkt11_Flag2Delta(networkCopySpawn);
        }

        networkCopySpawn = networkCopySpawn->next;
    }
}

// Reimplements 0x41e900: Pickup::SpawnListContainsPickupId
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL SpawnListContainsPickupId(PickupSpawnDef *spawn,
                                                              PickupSpawnList *list) {
    PickupSpawnDef *entry = list->head;
    if (entry == 0) {
        return 0;
    }

    const int targetPickupId = spawn->pickupId;
    while (entry != 0) {
        const int entryPickupId = entry->pickupId;
        if (targetPickupId == entryPickupId) {
            return 1;
        }

        if (targetPickupId < entryPickupId) {
            return 0;
        }

        entry = entry->next;
    }

    return 0;
}

// Reimplements 0x41cf30: Pickup::ResolveOwnerFromBvolHit
// (src/Battlesport/pickup.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL ResolveOwnerFromBvolHit(zClass_NodePartial **nodeInOut) {
    zClass_NodePartial *const node = *nodeInOut;
    if (node == 0 || (node->flags & 0x40000) == 0) {
        return 0;
    }

    PickupBvolHitCallbackContext *const context =
        (PickupBvolHitCallbackContext *)(node->callbackContext);
    *nodeInOut = context->ownerNode;
    return 1;
}

// Reimplements 0x41e930: Pickup::FindSpawnByPickupId (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE PickupSpawnDef *RECOIL_FASTCALL FindSpawnByPickupId(int pickupId,
                                                                    PickupSpawnList *list) {
    PickupSpawnDef *spawn = list->head;
    while (spawn != 0) {
        if (spawn->pickupId == pickupId) {
            return spawn;
        }

        spawn = spawn->next;
    }

    return 0;
}

// Reimplements 0x41e950: Pickup::GetSpawnDefFromNode (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE PickupSpawnDef *RECOIL_FASTCALL
GetSpawnDefFromNode(zClass_NodePartial *pickupNode) {
    return (PickupSpawnDef *)(pickupNode->callbackContext);
}

// Reimplements 0x41ea00: Pickup::FindOptMetaImageByOptEntry
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE zVidImagePartial *RECOIL_FASTCALL
FindOptMetaImageByOptEntry(OptCatalogEntryDef *optEntry) {
    {
    for (int index = 0x11; index <= 0x21; ++index) {
        if (g_PickupTypes[index].optEntry == optEntry) {
            return g_PickupTypes[index].optMetaImage;
        }
    }
    }

    return 0;
}

// Reimplements 0x41e980: Pickup::FindDroppableTypeForPlayerCurrentWeapon
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE PickupType *RECOIL_FASTCALL
FindDroppableTypeForPlayerCurrentWeapon(zUtil_SaveGameState *saveState) {
    const char *const keyName =
        saveState->playerState->activeAltGunController->optCatalogEntry->keyName;
    {
    for (int index = 0x11; index <= 0x21; ++index) {
        if (strcmp(keyName, g_PickupTypes[index].weaponKeyName) == 0) {
            return &g_PickupTypes[index];
        }
    }
    }

    return &g_PickupTypes[0];
}

// Reimplements 0x41e2f0: Pickup::RemoveOtherSpawnsWithSameOptEntry
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
RemoveOtherSpawnsWithSameOptEntry(OptCatalogEntryDef *optEntry,
                                  zClass_NodePartial *keepPickupObj) {
    PickupSpawnDef *spawn = g_PickupSpawnList_Primary.head;
    while (spawn != 0) {
        PickupSpawnDef *const next = spawn->next;
        if (spawn->pickupObj != keepPickupObj && spawn->pickupType->optEntry == optEntry) {
            PickupSpawnList::RemoveAndFreeNode(spawn, &g_PickupSpawnList_Primary);
        }
        spawn = next;
    }
}

// Reimplements 0x41d650: Pickup::GrantAmmoOrWeapon
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
GrantAmmoOrWeapon(PickupType *pickupType, char *messageBuffer, zUtil_SaveGameState *saveState,
                  int weaponBankIndex, int weaponSideIndex, int pairedWeaponSideIndex,
                  int overrideAmount) {
    const float kUnlimitedAmmoSentinel = 123456792.0f;

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerAltWeaponBank *const bank = &playerState->altWeaponBanks[weaponBankIndex];
    PlayerGunFireController *const controller =
        weaponSideIndex == 0 ? &bank->controllerA : &bank->controllerB;
    PlayerGunFireController *const pairedController =
        pairedWeaponSideIndex == 0 ? &bank->controllerA : &bank->controllerB;

    const float maxAmount = controller->optCatalogEntry->ammoOrChargeMax;
    if (controller->ammoOrCharge != kUnlimitedAmmoSentinel &&
        controller->ammoOrCharge >= maxAmount && pickupType->weaponKeyName == 0) {
        zLoc::FormatMessage(messageBuffer, 64, 0x237,
                            zLoc::GetMessageString(pickupType->msgIdOrClassId));
        return 0;
    }

    if (overrideAmount == 0) {
        overrideAmount = pickupType->defaultAmount;
    }

    int updateValueText = 1;
    if (pickupType->optEntry != 0) {
        if ((controller->flags & 4) == 0) {
            controller->flags |= 4;
            ++g_HudSensorTracker.primaryGunDispatchCount;
            if ((pairedController->flags & 4) != 0 &&
                pairedController->ammoOrCharge != 0.0f) {
                HudUiMessage::ApplySideImageSwap(weaponBankIndex, weaponSideIndex);
                updateValueText = 0;
            } else {
                HudUiMessage::SelectVariantDisplay(weaponBankIndex, weaponSideIndex);
                bank->selectedSide = weaponSideIndex;
            }
        }
    } else if (controller->ammoOrCharge == 0.0f) {
        if (playerState->activeAltGunController == controller) {
            HudUiMessage::SelectVariantDisplay(controller->weaponBankIndex,
                                               controller->weaponSideIndex + 3);
        }

        if (pairedController->ammoOrCharge == 0.0f) {
            bank->selectedSide = weaponSideIndex;
        }
    }

    if (controller->ammoOrCharge != kUnlimitedAmmoSentinel) {
        controller->ammoOrCharge += static_cast<float>(overrideAmount);
        if (controller->ammoOrCharge > maxAmount) {
            controller->ammoOrCharge = maxAmount;
        }

        if (updateValueText != 0) {
            HudUiMessage::SetValueIfOwnerMatches(weaponBankIndex, weaponSideIndex,
                                                 controller->ammoOrCharge);
        }
    }

    if (overrideAmount == 1) {
        zLoc::FormatMessage(messageBuffer, 64, 0x23f,
                            zLoc::GetMessageString(pickupType->msgIdOrClassId));
    } else {
        zLoc::FormatMessage(messageBuffer, 64, 0x240, overrideAmount,
                            zLoc::GetMessageString(pickupType->msgIdOrClassId));
    }

    if (pickupType->weaponKeyName != 0) {
        if (weaponBankIndex == 1) {
            Player::HandlePrimaryWeaponVariantToggleInput(weaponBankIndex);
            return weaponBankIndex;
        }

        if (playerState->activeAltGunController == pairedController) {
            Player::HandleAltWeaponBankSelectInput(controller->weaponBankIndex + 14);
        }
    }

    return 1;
}

// Reimplements 0x41d220: Pickup::ApplyEffect
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
ApplyEffect(int pickupTypeId, int overrideAmount, zUtil_SaveGameState *saveState) {
    const float kUnlimitedAmmoSentinel = 123456792.0f;
    const float kStatusPickupFullThreshold = 0.99000001f;

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterCommonData *const masterCommonData = playerState->masterCommonData;
    int result = 1;
    char message[64];

    if (pickupTypeId == 0x385) {
        zLoc::FormatMessage(message, sizeof(message), 0x243);
        HudUiMessage::ClearDisplay(playerState->activeAltGunController->weaponBankIndex);
        HudUiMessage::ClearDisplay(playerState->activePrimaryGunController->weaponBankIndex);

        for (int bankIndex = 1; bankIndex < 10; ++bankIndex) {
            PlayerAltWeaponBank &bank = playerState->altWeaponBanks[bankIndex];
            bank.controllerA.ammoOrCharge = kUnlimitedAmmoSentinel;
            bank.controllerA.flags |= 4;
            bank.controllerB.ammoOrCharge = kUnlimitedAmmoSentinel;
            bank.controllerB.flags |= 4;
            bank.selectedSide = 0;
            HudUiMessage::ApplySideImageSwap(bankIndex, 1);
            HudUiMessage::SetValueIfOwnerMatches(bankIndex, 0, kUnlimitedAmmoSentinel);
            HudUiMessage::SelectVariantDisplay(bankIndex, 0);
        }

        zUtil_PlayerStateStorage *const displayPlayerState =
            static_cast<zUtil_PlayerStateStorage *>(
                static_cast<void *>(g_GameStateOrMapTable->playerState));
        PlayerGunFireController *activeController = displayPlayerState->activeAltGunController;
        HudUiMessage::UpdateSelectedWeaponDisplay(
            activeController->weaponBankIndex, activeController->weaponSideIndex,
            activeController->ammoOrCharge);
        activeController = displayPlayerState->activePrimaryGunController;
        HudUiMessage::UpdateSelectedWeaponDisplay(
            activeController->weaponBankIndex, activeController->weaponSideIndex,
            activeController->ammoOrCharge);
    } else if (pickupTypeId == 0x386) {
        sprintf(message, zLoc::GetMessageString(0x20d));
        Player::UpdateStatusMeter(saveState, 0, 0.0f);
    } else if (pickupTypeId == 0x387) {
        zLoc::FormatMessage(message, sizeof(message), 0x247);
        playerState->nanitePanelLevel = 123456789;
        HudUiMgr::SetNanitePanelCount(3);
    } else if (pickupTypeId < 0 || pickupTypeId > 0x27) {
        zError::ReportOld(0x200, "D:\\Proj\\Battlesport\\pickup.cpp", 0x370,
                          "Unhandled Pickup Type: %d", pickupTypeId);
        return 0;
    } else {
        const int normalizedWeaponId =
            pickupTypeId >= 0x11 && pickupTypeId <= 0x21 ? pickupTypeId - 0x11
                                                         : pickupTypeId;
        if (normalizedWeaponId >= 0 && normalizedWeaponId <= 0x10) {
            int weaponBankIndex;
            int weaponSideIndex;
            if (normalizedWeaponId == 0) {
                weaponBankIndex = 1;
                weaponSideIndex = 1;
            } else {
                weaponBankIndex = (normalizedWeaponId + 3) / 2;
                weaponSideIndex = (normalizedWeaponId + 1) & 1;
            }

            result = GrantAmmoOrWeapon(
                &g_PickupTypes[pickupTypeId], message, saveState, weaponBankIndex,
                weaponSideIndex, weaponSideIndex == 0 ? 1 : 0, overrideAmount);
        } else {
            switch (pickupTypeId) {
            case 0x22:
            case 0x23:
                if (pickupTypeId == 0x23) {
                    overrideAmount = static_cast<int>(masterCommonData->maxHealth);
                }
                zLoc::FormatMessage(message, sizeof(message),
                                    g_PickupTypes[pickupTypeId].msgIdOrClassId);
                if (masterCommonData->invMaxHealth * playerState->statusMeterValue >
                    kStatusPickupFullThreshold) {
                    result = 0;
                } else {
                    result = Player::UpdateStatusMeter(saveState, 1,
                                                       static_cast<float>(overrideAmount));
                }
                break;

            case 0x24:
                zLoc::FormatMessage(message, sizeof(message), 0x20d);
                if (playerState->nanitePanelLevel >= 3) {
                    result = 0;
                } else {
                    ++playerState->nanitePanelLevel;
                    HudUiMgr::SetNanitePanelCount(playerState->nanitePanelLevel);
                }
                break;

            case 0x25:
                zLoc::FormatMessage(message, sizeof(message), 0x241);
                HudUiMgr::SetModeCounterState(1, 1);
                playerState->amphibUnlocked = 1;
                break;

            case 0x26:
                zLoc::FormatMessage(message, sizeof(message), 0x242);
                HudUiMgr::SetModeCounterState(2, 1);
                playerState->hoverUnlocked = 1;
                break;

            case 0x27:
                zLoc::FormatMessage(message, sizeof(message), 0x245);
                HudUiMgr::SetModeCounterState(3, 1);
                playerState->subUnlocked = 1;
                break;

            default:
                zError::ReportOld(0x200, "D:\\Proj\\Battlesport\\pickup.cpp", 0x370,
                                  "Unhandled Pickup Type: %d", pickupTypeId);
                return 0;
            }
        }
    }

    HudUi::ShowTopMessageLine(message, 5.0f);
    return result;
}

// Reimplements 0x41e780: Pickup::ArchiveWriteAll
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL ArchiveWriteAll(zZbdSectionCallbackCtx *callbackCtx,
                                                    void *userData) {
    (void)userData;

    int result = 1;
    int firstRecord = 1;
    PickupSpawnDef *spawn = g_PickupSpawnList_Primary.head;
    while (spawn != 0 && result != 0) {
        zClass_NodePartial *const pickupObj = spawn->pickupObj;
        if ((pickupObj->flags & 0x40000) != 0) {
            PickupArchiveRecord record;
            record.firstRecord = firstRecord;
            record.typeIndex = spawn->pickupType->typeIndex;
            record.pickupId = spawn->pickupId;
            record.amount = spawn->amount;
            record.position = spawn->position;
            record.rotation = spawn->rotation;
            record.spawnParam = spawn->spawnParam;
            record.respawnDelay = spawn->respawnDelay;
            result = zUtil_ZAR::WriteSectionBlob(callbackCtx, pickupObj->name, &record,
                                                 sizeof(record));
        }

        spawn = spawn->next;
        firstRecord = 0;
    }

    return result;
}

// Reimplements 0x41e840: Pickup::ArchiveReadRecord
// (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ArchiveReadRecord(zZbdSectionCallbackCtx *callbackCtx,
                                                       const char *sectionToken, void *buffer,
                                                       unsigned int size, void *userData) {
    (void)callbackCtx;
    (void)sectionToken;
    (void)size;
    (void)userData;

    const PickupArchiveRecord *const record = static_cast<const PickupArchiveRecord *>(buffer);
    if (record->firstRecord != 0) {
        g_PickupSpawnList_Primary.Clear();
        for (int index = 0; index < 40; ++index) {
            g_PickupTypes[index].nameSuffixMax = 0;
        }
    }

    PickupSpawnDef *const spawn = SpawnAt(record->typeIndex, record->amount,
                                          const_cast<zVec3 *>(&record->position),
                                          const_cast<zVec3 *>(&record->rotation),
                                          record->spawnParam);
    if (spawn != 0) {
        spawn->respawnDelay = record->respawnDelay;
    }
}

// Reimplements 0x41e960: Pickup::SetNextPickupId (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL SetNextPickupId(int nextPickupId) {
    const int oldNextPickupId = g_NextPickupId;
    g_NextPickupId = nextPickupId;
    return oldNextPickupId;
}

// Reimplements 0x41e970: Pickup::GetNextPickupId (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_CDECL GetNextPickupId() {
    return g_NextPickupId;
}

// Reimplements 0x41ccd0: Pickup::Shutdown (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE void RECOIL_CDECL Shutdown() {
    g_PickupSpawnList_Primary.Clear();
    g_PickupSpawnList_NetworkCopy.Clear();
    g_PickupRespawnQueue.ClearAndFree();
}
} // namespace Pickup

namespace PickupTypeTable {
// Reimplements 0x41cca0: PickupTypeTable::FreeOptMeta
RECOIL_NOINLINE void RECOIL_CDECL FreeOptMeta() {
    {
    for (int index = 0; index < 40; ++index) {
        PickupType &pickupType = g_PickupTypes[index];
        zVidImagePartial *const image = pickupType.optMetaImage;
        if (image != 0) {
            zVid_Image::ReleaseIfNotDefault(image);
            pickupType.optMetaImage = 0;
        }
    }
    }
}
} // namespace PickupTypeTable
