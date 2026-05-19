#include "pickup.h"

#include <stdlib.h>

PickupType g_PickupTypes[40] = {0};
PickupSpawnList g_PickupSpawnList_NetworkCopy = {0};
PickupRespawnQueue g_PickupRespawnQueue = {0};
PickupSpawnList g_PickupSpawnList_Primary = {0};
int g_NextPickupId = 0;

extern "C" {
PickupAirdropSpawnRef *g_Pickup_GlobalAirdropSpawnRef = 0;
}

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
