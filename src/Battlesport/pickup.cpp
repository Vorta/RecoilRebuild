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
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zUtil/zSaveGame.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PickupType g_PickupTypes[40] = {0};
PickupSpawnList g_PickupSpawnList_NetworkCopy = {0};
PickupRespawnQueue g_PickupRespawnQueue = {0};
PickupSpawnList g_PickupSpawnList_Primary = {0};
int g_NextPickupId = 0;
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
RECOIL_STATIC_ASSERT(offsetof(PickupPkt11CreateDelta, flags) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(PickupPkt11CreateDelta, pickupId) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(PickupPkt11CreateDelta, typeKeyIndex) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(PickupPkt11CreateDelta, amount) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(PickupPkt11CreateDelta, position) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(PickupPkt11CreateDelta, rotation) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(PickupPkt11CreateDelta, respawnDelay) == 0x30);

PickupPkt11Delta g_PickupPkt11Flag2Delta = {{0x11, sizeof(PickupPkt11Delta), 0}, 0, 0, 0};
PickupPkt11Delta g_PickupPkt11Flag8Delta = {{0x11, sizeof(PickupPkt11Delta), 0}, 0, 0, 0};

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

namespace zClass_Node {
// Reimplements 0x41ceb0: zClass_Node::ClearPickupFlagsRecursive (D:\Proj\Battlesport\pickup.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL ClearPickupFlagsRecursive(zClass_NodePartial *node) {
    node->flags &= ~0x40018;

    for (int i = 0; i < node->listCountB; ++i) {
        ClearPickupFlagsRecursive(node->listB[i]);
    }

    return 1;
}
} // namespace zClass_Node

// Reimplements 0x438b10: PickupAirdropSpawnRef::ShutdownGlobal
RECOIL_NOINLINE void RECOIL_CDECL PickupAirdropSpawnRef::ShutdownGlobal() {
    PickupAirdropSpawnRef *const spawnRef = g_Pickup_GlobalAirdropSpawnRef;
    if (spawnRef != 0) {
        ::operator delete(spawnRef);
    }

    g_Pickup_GlobalAirdropSpawnRef = 0;
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

namespace Pickup {
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
