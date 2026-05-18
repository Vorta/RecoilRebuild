#include "Battlesport/pickup.h"

#include <cstdlib>

namespace {
PickupSpawnDef *NewSpawnDef(PickupSpawnDef *next = nullptr) {
    auto *const node = static_cast<PickupSpawnDef *>(std::malloc(sizeof(PickupSpawnDef)));
    if (node != nullptr) {
        *node = {};
        node->next = next;
    }
    return node;
}

PickupRespawnEntry *NewRespawnEntry(PickupRespawnEntry *next = nullptr) {
    auto *const node =
        static_cast<PickupRespawnEntry *>(::operator new(sizeof(PickupRespawnEntry)));
    *node = {};
    node->next = next;
    return node;
}
} // namespace

extern "C" int pickup_type_table_free_opt_meta_smoke(void) {
    for (PickupType &pickupType : g_PickupTypes) {
        pickupType.optMetaImage = nullptr;
    }

    zVidImagePartial *const ownedImage = zVid_Image::Create();
    if (ownedImage == nullptr) {
        return 1;
    }

    g_PickupTypes[0].optMetaImage = &zVid_Image::g_zImage_DefaultImage;
    g_PickupTypes[17].optMetaImage = ownedImage;
    g_PickupTypes[39].optMetaImage = &zVid_Image::g_zImage_DefaultImage;

    PickupTypeTable::FreeOptMeta();

    for (const PickupType &pickupType : g_PickupTypes) {
        if (pickupType.optMetaImage != nullptr) {
            return 2;
        }
    }

    return 0;
}

extern "C" int pickup_airdrop_spawn_ref_shutdown_global_smoke(void) {
    auto *const spawnRef =
        static_cast<PickupAirdropSpawnRef *>(::operator new(sizeof(PickupAirdropSpawnRef)));
    g_Pickup_GlobalAirdropSpawnRef = spawnRef;

    PickupAirdropSpawnRef::ShutdownGlobal();

    const bool cleared = g_Pickup_GlobalAirdropSpawnRef == nullptr;
    PickupAirdropSpawnRef::ShutdownGlobal();

    return cleared ? 0 : 1;
}

extern "C" int pickup_spawn_list_clear_smoke(void) {
    PickupSpawnList::Primary_Init();
    PickupSpawnList::NetCopy_Init();

    PickupSpawnDef *const second = NewSpawnDef();
    PickupSpawnDef *const first = NewSpawnDef(second);
    if (first == nullptr || second == nullptr) {
        std::free(first);
        std::free(second);
        return 1;
    }

    g_NextPickupId = 123;
    g_PickupSpawnList_Primary.unused = &g_PickupSpawnList_Primary;
    g_PickupSpawnList_Primary.head = first;
    g_PickupSpawnList_Primary.tail = second;
    g_PickupSpawnList_Primary.count = 2;

    g_PickupSpawnList_Primary.Clear();

    const bool primaryCleared = g_PickupSpawnList_Primary.unused == nullptr &&
                                g_PickupSpawnList_Primary.head == nullptr &&
                                g_PickupSpawnList_Primary.tail == nullptr &&
                                g_PickupSpawnList_Primary.count == 0 && g_NextPickupId == 0;

    PickupSpawnDef *const networkNode = NewSpawnDef();
    if (networkNode == nullptr) {
        return 2;
    }

    g_NextPickupId = 77;
    g_PickupSpawnList_NetworkCopy.head = networkNode;
    g_PickupSpawnList_NetworkCopy.tail = networkNode;
    g_PickupSpawnList_NetworkCopy.count = 1;

    g_PickupSpawnList_NetworkCopy.Clear();

    const bool networkCleared = g_PickupSpawnList_NetworkCopy.head == nullptr &&
                                g_PickupSpawnList_NetworkCopy.tail == nullptr &&
                                g_PickupSpawnList_NetworkCopy.count == 0 && g_NextPickupId == 77;

    return primaryCleared && networkCleared ? 0 : 3;
}

extern "C" int pickup_respawn_queue_clear_smoke(void) {
    PickupRespawnQueue::Init();

    PickupRespawnEntry *const second = NewRespawnEntry();
    PickupRespawnEntry *const first = NewRespawnEntry(second);
    g_PickupRespawnQueue.unused = &g_PickupRespawnQueue;
    g_PickupRespawnQueue.head = first;
    g_PickupRespawnQueue.tail = second;
    g_PickupRespawnQueue.count = 2;

    g_PickupRespawnQueue.ClearAndFree();

    const bool cleared = g_PickupRespawnQueue.unused == nullptr &&
                         g_PickupRespawnQueue.head == nullptr &&
                         g_PickupRespawnQueue.tail == nullptr && g_PickupRespawnQueue.count == 0;

    return cleared ? 0 : 1;
}

extern "C" int pickup_shutdown_smoke(void) {
    PickupSpawnList::Primary_Init();
    PickupSpawnList::NetCopy_Init();
    PickupRespawnQueue::Init();

    PickupSpawnDef *const primaryNode = NewSpawnDef();
    PickupSpawnDef *const networkNode = NewSpawnDef();
    PickupRespawnEntry *const respawnNode = NewRespawnEntry();
    if (primaryNode == nullptr || networkNode == nullptr) {
        std::free(primaryNode);
        std::free(networkNode);
        ::operator delete(respawnNode);
        return 1;
    }

    g_NextPickupId = 91;
    g_PickupSpawnList_Primary.head = primaryNode;
    g_PickupSpawnList_Primary.tail = primaryNode;
    g_PickupSpawnList_Primary.count = 1;
    g_PickupSpawnList_NetworkCopy.head = networkNode;
    g_PickupSpawnList_NetworkCopy.tail = networkNode;
    g_PickupSpawnList_NetworkCopy.count = 1;
    g_PickupRespawnQueue.head = respawnNode;
    g_PickupRespawnQueue.tail = respawnNode;
    g_PickupRespawnQueue.count = 1;

    Pickup::Shutdown();

    const bool cleared = g_PickupSpawnList_Primary.head == nullptr &&
                         g_PickupSpawnList_NetworkCopy.head == nullptr &&
                         g_PickupRespawnQueue.head == nullptr && g_NextPickupId == 0;

    return cleared ? 0 : 2;
}

extern "C" int pickup_leaf_helpers_smoke(void) {
    PickupSpawnList::Primary_Init();

    PickupSpawnDef first = {};
    PickupSpawnDef second = {};
    first.pickupId = 17;
    first.next = &second;
    second.pickupId = 23;
    g_PickupSpawnList_Primary.head = &first;
    g_PickupSpawnList_Primary.tail = &second;
    g_PickupSpawnList_Primary.count = 2;

    if (Pickup::FindSpawnByPickupId(17, &g_PickupSpawnList_Primary) != &first ||
        Pickup::FindSpawnByPickupId(23, &g_PickupSpawnList_Primary) != &second ||
        Pickup::FindSpawnByPickupId(99, &g_PickupSpawnList_Primary) != nullptr) {
        return 1;
    }

    if (PickupType::GetByIndex(0) != &g_PickupTypes[0] ||
        PickupType::GetByIndex(39) != &g_PickupTypes[39] || PickupType::GetByIndex(-1) != nullptr ||
        PickupType::GetByIndex(40) != nullptr ||
        PickupType::GetByIndex_Pure(39) != &g_PickupTypes[39] ||
        PickupType::GetByIndex_Pure(40) != nullptr) {
        return 2;
    }

    g_NextPickupId = 41;
    if (Pickup::GetNextPickupId() != 41 || Pickup::SetNextPickupId(52) != 41 ||
        Pickup::GetNextPickupId() != 52) {
        return 3;
    }

    zClass_NodePartial childA = {};
    zClass_NodePartial childB = {};
    zClass_NodePartial *children[] = {&childA, &childB};
    zClass_NodePartial root = {};
    root.flags = 0x40018 | 0x82;
    root.listCountB = 2;
    root.listB = children;
    childA.flags = 0x40018 | 0x20;
    childB.flags = 0x40018 | 0x40;

    if (zClass_Node::ClearPickupFlagsRecursive(&root) != 1 || root.flags != 0x82 ||
        childA.flags != 0x20 || childB.flags != 0x40) {
        return 4;
    }

    zClass_NodePartial pickupNode = {};
    pickupNode.callbackContext = reinterpret_cast<zClass_NodePartial *>(&second);
    if (Pickup::GetSpawnDefFromNode(&pickupNode) != &second) {
        return 5;
    }

    return 0;
}
