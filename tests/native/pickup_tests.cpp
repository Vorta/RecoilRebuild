#include "Battlesport/pickup.h"

#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/include/zDi.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"

#include <cstdlib>
#include <cstring>

namespace {
template <typename T>
T &TestFieldAt(void *base, std::size_t offset) {
    return *reinterpret_cast<T *>(static_cast<unsigned char *>(base) + offset);
}

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

extern "C" int pickup_remove_object_smoke(void) {
    std::int32_t networkEnabled = 0;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    ZOPT_NETWORK_ENABLED = &networkEnabled;

    PickupRespawnQueue::Init();
    PickupSpawnList::Primary_Init();

    PickupSpawnDef *const respawnSpawn = NewSpawnDef();
    if (respawnSpawn == nullptr) {
        ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
        return 1;
    }

    zClass_NodePartial respawnNode = {};
    respawnNode.classId = 5;
    respawnNode.flags = 4;
    respawnNode.callbackContext = (zClass_NodePartial *)(respawnSpawn);
    respawnSpawn->pickupId = 37;
    respawnSpawn->pickupObj = &respawnNode;
    respawnSpawn->respawnDelay = 2.5f;
    g_Time_UnscaledAccumulatedTimeSec = 10.0f;

    Pickup::RemoveObject(nullptr, &respawnNode, 0);

    const bool respawnQueued =
        (respawnNode.flags & 4) == 0 && g_PickupRespawnQueue.count == 1 &&
        g_PickupRespawnQueue.head == g_PickupRespawnQueue.tail &&
        g_PickupRespawnQueue.head->spawn == respawnSpawn &&
        g_PickupRespawnQueue.head->when == 12.5f;

    g_PickupRespawnQueue.ClearAndFree();
    std::free(respawnSpawn);

    PickupSpawnDef *const first = NewSpawnDef();
    PickupSpawnDef *const removed = NewSpawnDef();
    PickupSpawnDef *const third = NewSpawnDef();
    if (first == nullptr || removed == nullptr || third == nullptr) {
        std::free(first);
        std::free(removed);
        std::free(third);
        ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
        return 2;
    }

    first->next = removed;
    removed->next = third;
    zClass_NodePartial parent = {};
    zClass_NodePartial pickupNode = {};
    zClass_NodePartial *parentList[] = {&parent};
    zClass_NodePartial *childList[] = {&pickupNode};
    parent.classId = 3;
    parent.listCountB = 1;
    parent.listB = childList;
    pickupNode.classId = 5;
    pickupNode.flags = 4;
    pickupNode.listCountA = 1;
    pickupNode.listA = parentList;
    pickupNode.callbackContext = (zClass_NodePartial *)(removed);
    removed->pickupObj = &pickupNode;

    parent.flags = 1;
    g_PickupSpawnList_Primary.head = first;
    g_PickupSpawnList_Primary.tail = third;
    g_PickupSpawnList_Primary.count = 3;

    Pickup::RemoveObject(nullptr, &pickupNode, 0);

    const bool removedNow = (pickupNode.flags & 4) == 0 && parent.listCountB == 0 &&
                            pickupNode.listCountA == 0 &&
                            g_PickupSpawnList_Primary.head == first &&
                            g_PickupSpawnList_Primary.tail == third &&
                            g_PickupSpawnList_Primary.count == 2 && first->next == third;

    std::free(first);
    std::free(third);
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    return respawnQueued && removedNow ? 0 : 3;
}

extern "C" int pickup_on_collected_no_anim_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    HudUiShieldMessageWidget shield = {};
    HudUiTextStack4 topStack = {};
    zClass_NodePartial pickupNode = {};
    zClass_NodePartial hitNode = {};
    PickupBvolHitCallbackContext hitContext = {};

    std::int32_t networkEnabled = 0;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    zEffectAnimEntry *const oldEntryList = g_zEffectAnim_EntryList;
    const short oldEntryCount = g_zEffectAnim_EntryCount;
    HudUiShieldMessageWidget *const oldShieldWidget = g_HudUiMgrShieldMessageWidget;
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    const float oldStatusMeterRatio = g_PlayerStatusMeterRatio;

    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_zEffectAnim_EntryList = nullptr;
    g_zEffectAnim_EntryCount = 0;
    g_HudUiMgrShieldMessageWidget = &shield;
    g_HudUiTopMessageStack = &topStack;

    saveState.playerState = &playerState;
    playerState.masterCommonData = &commonData;
    commonData.maxHealth = 100.0f;
    commonData.invMaxHealth = 0.01f;
    playerState.statusMeterValue = 20.0f;
    g_PlayerStatusMeterRatio = 0.2f;

    shield.meter.ftable = &g_HudUiMeter_FTable;
    shield.meter.fillPixelsMax = 20;
    shield.meter.points[1].y = 100.0f;
    reinterpret_cast<HudUiPanel *>(&shield.percentTextPanel)->vtbl =
        &g_HudUiPanelSimple_FTable;

    PickupSpawnDef *const spawn = NewSpawnDef();
    if (spawn == nullptr) {
        ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
        g_zEffectAnim_EntryList = oldEntryList;
        g_zEffectAnim_EntryCount = oldEntryCount;
        g_HudUiMgrShieldMessageWidget = oldShieldWidget;
        g_HudUiTopMessageStack = oldTopStack;
        g_PlayerStatusMeterRatio = oldStatusMeterRatio;
        return 1;
    }

    spawn->amount = 30;
    spawn->pickupObj = &pickupNode;
    pickupNode.classId = 5;
    pickupNode.flags = 0x4001c;
    pickupNode.callbackContext = (zClass_NodePartial *)(spawn);
    TestFieldAt<std::int32_t>(pickupNode.name, 0x1c) = 0x22;
    hitContext.ownerNode = &pickupNode;
    hitNode.flags = 0x40000;
    hitNode.callbackContext = (zClass_NodePartial *)(&hitContext);

    const int result = Pickup::OnCollected(&hitNode, &saveState);
    const bool collected = result == 1 && playerState.statusMeterValue == 50.0f &&
                           (pickupNode.flags & 0x4001c) == 0;

    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_zEffectAnim_EntryList = oldEntryList;
    g_zEffectAnim_EntryCount = oldEntryCount;
    g_HudUiMgrShieldMessageWidget = oldShieldWidget;
    g_HudUiTopMessageStack = oldTopStack;
    g_PlayerStatusMeterRatio = oldStatusMeterRatio;
    return collected ? 0 : 2;
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

    OptCatalogEntryDef optA = {};
    OptCatalogEntryDef optB = {};
    zVidImagePartial metaImage = {};
    g_PickupTypes[17] = {};
    g_PickupTypes[18] = {};
    g_PickupTypes[17].optEntry = &optA;
    g_PickupTypes[17].optMetaImage = &metaImage;
    g_PickupTypes[18].optEntry = &optB;
    if (Pickup::FindOptMetaImageByOptEntry(&optA) != &metaImage ||
        Pickup::FindOptMetaImageByOptEntry(&optB) != nullptr ||
        Pickup::FindOptMetaImageByOptEntry(nullptr) != nullptr) {
        g_PickupTypes[17] = {};
        g_PickupTypes[18] = {};
        return 4;
    }
    g_PickupTypes[17] = {};
    g_PickupTypes[18] = {};

    zDiPartial pickupDi{0, 1};
    zDiPartial bvolDi{0, 1};
    zClass_NodePartial pickupObj = {};
    zClass_NodePartial bvolNode = {};
    zClass_NodePartial *pickupChildren[1] = {&bvolNode};
    std::strcpy(pickupObj.name, "pu1234");
    std::strcpy(bvolNode.name, "bvol");
    pickupObj.flags = 0x0c;
    bvolNode.flags = 0x04;
    bvolNode.classId = 5;
    pickupObj.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&pickupDi);
    bvolNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&bvolDi);
    pickupObj.listCountB = 1;
    pickupObj.listB = pickupChildren;
    g_NextPickupId = 77;
    g_PickupTypes[12] = {};
    g_PickupTypes[12].nameSuffixMax = 20;
    if (Pickup::AssignBvolGroupAndId(&pickupObj) != 1 ||
        *(int *)(pickupObj.name + 0x1c) != 12 || *(int *)(pickupObj.name + 0x18) != 77 ||
        g_NextPickupId != 78 || g_PickupTypes[12].nameSuffixMax != 35 ||
        (pickupObj.flags & 0x28) != 0x20 || (bvolNode.flags & 0x04) != 0 ||
        (pickupDi.flags & 1) != 0 || (bvolDi.flags & 1) != 0) {
        g_PickupTypes[12] = {};
        return 5;
    }
    g_PickupTypes[12] = {};

    zClass_NodePartial missingBvol = {};
    std::strcpy(missingBvol.name, "pu4100");
    g_NextPickupId = 91;
    if (Pickup::AssignBvolGroupAndId(&missingBvol) != 0 || g_NextPickupId != 91) {
        return 6;
    }

    zClass_NodePartial templateRoot = {};
    zClass_NodePartial templateBvol = {};
    zClass_NodePartial *templateChildren[1] = {&templateBvol};
    std::strcpy(templateRoot.name, "template");
    std::strcpy(templateBvol.name, "bvol");
    templateRoot.flags = 0x0400000c;
    templateRoot.classId = 5;
    templateRoot.listCountB = 1;
    templateRoot.listB = templateChildren;
    templateBvol.classId = 5;
    templateBvol.flags = 0x04;
    g_PickupTypes[12] = {};
    g_PickupTypes[12].typeIndex = 12;
    g_PickupTypes[12].defaultAmount = 6;
    g_PickupTypes[12].nameSuffixMax = 34;
    g_PickupTypes[12].templateNode = &templateRoot;
    g_NextPickupId = 200;
    zClass_NodePartial *const createdPickup = Pickup::CreateObjectInstance(12, 0);
    const bool createdOk =
        createdPickup != nullptr && std::strcmp(createdPickup->name, "pu01234") == 0 &&
        *(int *)(createdPickup->name + 0x1c) == 12 &&
        *(int *)(createdPickup->name + 0x18) == 200 &&
        *(int *)(createdPickup->name + 0x20) == 6 && g_NextPickupId == 201 &&
        g_PickupTypes[12].nameSuffixMax == 35 && createdPickup->listCountB == 1 &&
        std::strcmp(createdPickup->listB[0]->name, "bvol") == 0 &&
        (createdPickup->listB[0]->flags & 0x04) == 0;
    g_PickupTypes[12] = {};
    if (!createdOk || Pickup::CreateObjectInstance(99, 0) != nullptr) {
        return 7;
    }

    static std::int32_t matrixFlags[8];
    static float *matrixSlots[8];
    static zMat4x3 matrix;
    matrixFlags[0] = 1;
    matrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
              0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixSlots[0] = reinterpret_cast<float *>(&matrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchA = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zVec3 terrainVertices[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
                                {1.0f, 0.0f, 0.0f}};
    std::int32_t terrainIndices[3] = {0, 1, 2};
    zModel_MaterialPartial terrainMaterial = {};
    zDiEntryPartial terrainEntry = {};
    terrainEntry.flagsAndIndexCount = 3;
    terrainEntry.vertexIndices = terrainIndices;
    terrainEntry.material = &terrainMaterial;
    terrainEntry.variantTagInitialized = 1;
    terrainEntry.variantTag = 0x33;
    zDiPartial terrainDi = {};
    terrainDi.entryCount = 1;
    terrainDi.vertCount = 3;
    terrainDi.entries = &terrainEntry;
    terrainDi.verts = terrainVertices;
    zClass_Object3DDataPartial terrainObjectData = {};
    terrainObjectData.flags = 8;
    zClass_NodePartial terrainNode = {};
    terrainNode.flags = 0x11c;
    terrainNode.nodeType = 0x44;
    terrainNode.classId = 5;
    terrainNode.classData = &terrainObjectData;
    terrainNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&terrainDi);
    terrainNode.cachedBounds[0] = 0.0f;
    terrainNode.cachedBounds[1] = 0.0f;
    terrainNode.cachedBounds[2] = 0.0f;
    terrainNode.cachedBounds[3] = 1.0f;
    terrainNode.cachedBounds[4] = 1.0f;
    terrainNode.cachedBounds[5] = 1.0f;
    zClass_NodePartial *areaChildren[1] = {&terrainNode};
    zWorldAreaPartial area = {};
    area.childCount = 1;
    area.childList = areaChildren;
    zWorldAreaPartial *rows[1] = {&area};
    zClass_WorldDataPartial worldData = {};
    worldData.originX = 0.0f;
    worldData.originZ = 0.0f;
    worldData.areaCellSizeX = 1.0f;
    worldData.areaCellSizeZ = 1.0f;
    worldData.areaInvSizeX = 1.0f;
    worldData.areaInvSizeZ = 1.0f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = rows;
    zClass_NodePartial world = {};
    world.classId = 2;
    world.classData = &worldData;
    g_Pickup_SceneNode = &world;

    zDiEntryPartial pickupVariantEntries[1] = {};
    zDiPartial pickupVariantDi = {};
    pickupVariantDi.entryCount = 1;
    pickupVariantDi.entries = pickupVariantEntries;
    zClass_NodePartial terrainPickup = {};
    terrainPickup.classId = 5;
    terrainPickup.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&pickupVariantDi);
    zVec3 terrainPos{0.25f, 0.5f, 0.25f};
    zClass_NodePartial *terrainParents[1] = {&world};
    terrainNode.listCountA = 1;
    terrainNode.listA = terrainParents;
    Pickup::SetVariantFromTerrain(&terrainPickup, &terrainPos);
    if (terrainPickup.nodeType != 0x44 ||
        pickupVariantEntries[0].variantTagInitialized != 1 ||
        pickupVariantEntries[0].variantTag != 0x44 || g_Variant_CurrentTag.count != 0) {
        return 8;
    }

    pickupVariantEntries[0] = {};
    terrainPickup.nodeType = 0;
    terrainNode.listCountA = 0;
    terrainNode.listA = nullptr;
    Pickup::SetVariantFromTerrain(&terrainPickup, &terrainPos);
    if (terrainPickup.nodeType != 0x33 ||
        pickupVariantEntries[0].variantTagInitialized != 1 ||
        pickupVariantEntries[0].variantTag != 0x33) {
        return 9;
    }

    pickupVariantEntries[0] = {};
    terrainPickup.nodeType = 0;
    zVec3 emptyTerrainPos{2.0f, 0.5f, 0.25f};
    Pickup::SetVariantFromTerrain(&terrainPickup, &emptyTerrainPos);
    if (terrainPickup.nodeType != 0xff ||
        pickupVariantEntries[0].variantTagInitialized != 1 ||
        pickupVariantEntries[0].variantTag != 0xff) {
        return 10;
    }
    g_Pickup_SceneNode = nullptr;

    PickupSpawnList::Primary_Init();
    zClass_NodePartial spawnParent = {};
    zClass_NodePartial spawnPickup = {};
    zClass_NodePartial spawnChild = {};
    zClass_NodePartial *spawnPickupChildren[1] = {&spawnChild};
    spawnParent.classId = 3;
    spawnParent.flags = 1;
    spawnPickup.classId = 5;
    spawnPickup.listCountB = 1;
    spawnPickup.listB = spawnPickupChildren;
    TestFieldAt<int>(spawnPickup.name, 0x18) = 301;
    TestFieldAt<int>(spawnPickup.name, 0x1c) = 321;
    TestFieldAt<int>(spawnPickup.name, 0x20) = 7;
    g_PickupTypes[5] = {};
    g_PickupTypes[5].typeIndex = 321;
    g_Pickup_SceneNode = &spawnParent;
    zVec3 spawnPosition{1.0f, 2.0f, 3.0f};
    zVec3 spawnRotation{4.0f, 5.0f, 6.0f};
    PickupSpawnDef *const linkedSpawn =
        Pickup::CreateSpawnDefAndLink(&spawnPickup, &spawnPosition, &spawnRotation, 99, 1);
    const bool linkedSpawnOk =
        linkedSpawn != nullptr && linkedSpawn->pickupId == 301 &&
        linkedSpawn->pickupType == &g_PickupTypes[5] && linkedSpawn->amount == 7 &&
        linkedSpawn->position.x == 1.0f && linkedSpawn->position.y == 2.0f &&
        linkedSpawn->position.z == 3.0f && linkedSpawn->rotation.x == 4.0f &&
        linkedSpawn->rotation.y == 5.0f && linkedSpawn->rotation.z == 6.0f &&
        linkedSpawn->pickupObj == &spawnPickup && linkedSpawn->spawnParam == 99 &&
        linkedSpawn->refCount == 0 && linkedSpawn->respawnDelay == 0.0f &&
        linkedSpawn->next == nullptr && g_PickupSpawnList_Primary.head == linkedSpawn &&
        g_PickupSpawnList_Primary.tail == linkedSpawn && g_PickupSpawnList_Primary.count == 1 &&
        spawnParent.listCountB == 1 && spawnParent.listB[0] == &spawnPickup &&
        spawnPickup.listCountA == 1 && spawnPickup.listA[0] == &spawnParent &&
        spawnPickup.callbackContext == reinterpret_cast<zClass_NodePartial *>(linkedSpawn) &&
        spawnChild.callbackContext == reinterpret_cast<zClass_NodePartial *>(linkedSpawn) &&
        (spawnPickup.flags & 0x240000) == 0x240000 &&
        (spawnChild.flags & 0x240000) == 0x240000;

    std::free(spawnParent.listB);
    std::free(spawnPickup.listA);
    g_PickupSpawnList_Primary.head = nullptr;
    g_PickupSpawnList_Primary.tail = nullptr;
    g_PickupSpawnList_Primary.count = 0;
    std::free(linkedSpawn);
    spawnParent.listB = nullptr;
    spawnParent.listCountB = 0;
    spawnPickup.listA = nullptr;
    spawnPickup.listCountA = 0;
    spawnPickup.callbackContext = nullptr;
    spawnChild.callbackContext = nullptr;
    g_Pickup_SceneNode = nullptr;
    if (!linkedSpawnOk) {
        g_PickupTypes[5] = {};
        return 11;
    }

    PickupSpawnDef *const unrotatedSpawn =
        Pickup::CreateSpawnDefAndLink(&spawnPickup, &spawnPosition, nullptr, 55, 0);
    const bool unrotatedSpawnOk =
        unrotatedSpawn != nullptr && unrotatedSpawn->rotation.x == 0.0f &&
        unrotatedSpawn->rotation.y == 0.0f && unrotatedSpawn->rotation.z == 0.0f &&
        unrotatedSpawn->spawnParam == 55 && spawnParent.listCountB == 0 &&
        spawnPickup.listCountA == 0 && g_PickupSpawnList_Primary.head == unrotatedSpawn &&
        g_PickupSpawnList_Primary.tail == unrotatedSpawn && g_PickupSpawnList_Primary.count == 1;
    g_PickupSpawnList_Primary.head = nullptr;
    g_PickupSpawnList_Primary.tail = nullptr;
    g_PickupSpawnList_Primary.count = 0;
    std::free(unrotatedSpawn);
    g_PickupTypes[5] = {};
    if (!unrotatedSpawnOk) {
        return 12;
    }

    PickupSpawnList::Primary_Init();
    zClass_Object3DDataPartial spawnTemplateData = {};
    zClass_NodePartial spawnTemplateRoot = {};
    zClass_NodePartial spawnTemplateBvol = {};
    zClass_NodePartial *spawnTemplateChildren[1] = {&spawnTemplateBvol};
    std::strcpy(spawnTemplateRoot.name, "spawn-template");
    std::strcpy(spawnTemplateBvol.name, "bvol");
    spawnTemplateRoot.classId = 5;
    spawnTemplateRoot.flags = 0x04000081;
    spawnTemplateRoot.classData = &spawnTemplateData;
    spawnTemplateRoot.listCountB = 1;
    spawnTemplateRoot.listB = spawnTemplateChildren;
    spawnTemplateBvol.classId = 3;
    spawnTemplateBvol.flags = 0x04;
    g_PickupTypes[6] = {};
    g_PickupTypes[6].typeIndex = 6;
    g_PickupTypes[6].defaultAmount = 4;
    g_PickupTypes[6].nameSuffixMax = 8;
    g_PickupTypes[6].templateNode = &spawnTemplateRoot;
    g_Pickup_SceneNode = &world;
    g_NextPickupId = 400;
    zVec3 spawnAtPosition{0.25f, 0.5f, 0.25f};
    zVec3 spawnAtRotation{0.1f, 0.2f, 0.3f};
    PickupSpawnDef *const spawned =
        Pickup::SpawnAt(6, 11, &spawnAtPosition, &spawnAtRotation, 77);
    zClass_Object3DDataPartial *const spawnedObjectData =
        spawned != nullptr
            ? static_cast<zClass_Object3DDataPartial *>(spawned->pickupObj->classData)
            : nullptr;
    const bool spawnAtOk =
        spawned != nullptr && std::strcmp(spawned->name, "pu00608") == 0 &&
        spawned->pickupId == 400 && spawned->pickupType == &g_PickupTypes[6] &&
        spawned->amount == 11 && spawned->spawnParam == 77 &&
        spawned->position.x == 0.25f && spawned->position.y == 0.5f &&
        spawned->position.z == 0.25f && spawned->rotation.x == 0.1f &&
        spawned->rotation.y == 0.2f && spawned->rotation.z == 0.3f &&
        g_NextPickupId == 401 && g_PickupTypes[6].nameSuffixMax == 9 &&
        g_PickupSpawnList_Primary.head == spawned &&
        g_PickupSpawnList_Primary.tail == spawned && g_PickupSpawnList_Primary.count == 1 &&
        world.listCountB == 1 && world.listB[0] == spawned->pickupObj &&
        spawned->pickupObj->listCountA == 1 && spawned->pickupObj->listA[0] == &world &&
        spawnedObjectData != nullptr && spawnedObjectData->localMatrix[9] == 0.25f &&
        spawnedObjectData->localMatrix[10] == 0.5f &&
        spawnedObjectData->localMatrix[11] == 0.25f &&
        spawnedObjectData->rotation.x == 0.1f && spawnedObjectData->rotation.y == 0.2f &&
        spawnedObjectData->rotation.z == 0.3f && spawned->pickupObj->nodeType == 0x33;

    if (spawned != nullptr) {
        std::free(spawned->pickupObj->listA);
        std::free(spawned);
    }
    std::free(world.listB);
    world.listB = nullptr;
    world.listCountB = 0;
    g_PickupSpawnList_Primary.head = nullptr;
    g_PickupSpawnList_Primary.tail = nullptr;
    g_PickupSpawnList_Primary.count = 0;
    g_PickupTypes[6] = {};
    g_Pickup_SceneNode = nullptr;
    if (!spawnAtOk) {
        return 13;
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
        return 14;
    }

    zClass_NodePartial pickupNode = {};
    pickupNode.callbackContext = reinterpret_cast<zClass_NodePartial *>(&second);
    if (Pickup::GetSpawnDefFromNode(&pickupNode) != &second) {
        return 15;
    }

    return 0;
}

extern "C" int pickup_find_droppable_type_for_current_weapon_smoke(void) {
    PickupType oldTypes[34] = {};
    for (int index = 0; index <= 33; ++index) {
        oldTypes[index] = g_PickupTypes[index];
    }

    for (int index = 17; index <= 33; ++index) {
        g_PickupTypes[index] = {};
        g_PickupTypes[index].weaponKeyName = "unused";
    }
    g_PickupTypes[0] = {};

    OptCatalogEntryDef optEntry = {};
    PlayerGunFireController controller = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    playerState.activeAltGunController = &controller;
    controller.optCatalogEntry = &optEntry;

    g_PickupTypes[18].weaponKeyName = "cannon";
    optEntry.keyName = const_cast<char *>("cannon");
    const bool matchOk =
        Pickup::FindDroppableTypeForPlayerCurrentWeapon(&saveState) == &g_PickupTypes[18];

    g_PickupTypes[33].weaponKeyName = "last";
    optEntry.keyName = const_cast<char *>("last");
    const bool lastOk =
        Pickup::FindDroppableTypeForPlayerCurrentWeapon(&saveState) == &g_PickupTypes[33];

    optEntry.keyName = const_cast<char *>("missing");
    const bool fallbackOk =
        Pickup::FindDroppableTypeForPlayerCurrentWeapon(&saveState) == &g_PickupTypes[0];

    for (int index = 0; index <= 33; ++index) {
        g_PickupTypes[index] = oldTypes[index];
    }

    return matchOk && lastOk && fallbackOk ? 0 : 1;
}

extern "C" int pickup_type_key_table_find_index_smoke(void) {
    PickupType oldTypes[40] = {};
    for (int index = 0; index < 40; ++index) {
        oldTypes[index] = g_PickupTypes[index];
        g_PickupTypes[index] = {};
    }

    g_PickupTypes[0].logicalName = "first";
    g_PickupTypes[7].logicalName = "mid";
    g_PickupTypes[39].logicalName = "last";

    const bool firstOk = PickupTypeKeyTable::FindIndex("first") == 0;
    const bool nullSkipOk = PickupTypeKeyTable::FindIndex("mid") == 7;
    const bool lastOk = PickupTypeKeyTable::FindIndex("last") == 39;
    const bool missOk = PickupTypeKeyTable::FindIndex("missing") == -1;

    for (int index = 0; index < 40; ++index) {
        g_PickupTypes[index] = oldTypes[index];
    }

    return firstOk && nullSkipOk && lastOk && missOk ? 0 : 1;
}

extern "C" int pickup_spawn_from_parsed_zrd_entry_smoke(void) {
    const PickupType oldType = g_PickupTypes[7];
    zClass_NodePartial *const oldSceneNode = g_Pickup_SceneNode;
    const int oldNextPickupId = g_NextPickupId;

    PickupSpawnList::Primary_Init();
    zClass_NodePartial world = {};
    zClass_Object3DDataPartial templateData = {};
    zClass_NodePartial templateRoot = {};
    zClass_NodePartial templateBvol = {};
    zClass_NodePartial *templateChildren[1] = {&templateBvol};
    std::strcpy(templateRoot.name, "parsed-template");
    std::strcpy(templateBvol.name, "bvol");
    templateRoot.classId = 5;
    templateRoot.flags = 0x04000081;
    templateRoot.classData = &templateData;
    templateRoot.listCountB = 1;
    templateRoot.listB = templateChildren;
    templateBvol.classId = 3;
    templateBvol.flags = 0x04;

    static std::int32_t matrixFlags[8];
    static float *matrixSlots[8];
    static zMat4x3 matrix;
    matrixFlags[0] = 1;
    matrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
              0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixSlots[0] = reinterpret_cast<float *>(&matrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchA = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zVec3 terrainVertices[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
                                {1.0f, 0.0f, 0.0f}};
    std::int32_t terrainIndices[3] = {0, 1, 2};
    zModel_MaterialPartial terrainMaterial = {};
    zDiEntryPartial terrainEntry = {};
    terrainEntry.flagsAndIndexCount = 3;
    terrainEntry.vertexIndices = terrainIndices;
    terrainEntry.material = &terrainMaterial;
    terrainEntry.variantTagInitialized = 1;
    terrainEntry.variantTag = 0x33;
    zDiPartial terrainDi = {};
    terrainDi.entryCount = 1;
    terrainDi.vertCount = 3;
    terrainDi.entries = &terrainEntry;
    terrainDi.verts = terrainVertices;
    zClass_Object3DDataPartial terrainObjectData = {};
    terrainObjectData.flags = 8;
    zClass_NodePartial terrainNode = {};
    terrainNode.flags = 0x11c;
    terrainNode.nodeType = 0x44;
    terrainNode.classId = 5;
    terrainNode.classData = &terrainObjectData;
    terrainNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&terrainDi);
    terrainNode.cachedBounds[0] = 0.0f;
    terrainNode.cachedBounds[1] = 0.0f;
    terrainNode.cachedBounds[2] = 0.0f;
    terrainNode.cachedBounds[3] = 1.0f;
    terrainNode.cachedBounds[4] = 1.0f;
    terrainNode.cachedBounds[5] = 1.0f;
    zClass_NodePartial *areaChildren[1] = {&terrainNode};
    zWorldAreaPartial area = {};
    area.childCount = 1;
    area.childList = areaChildren;
    zWorldAreaPartial *rows[1] = {&area};
    zClass_WorldDataPartial worldData = {};
    worldData.originX = 0.0f;
    worldData.originZ = 0.0f;
    worldData.areaCellSizeX = 1.0f;
    worldData.areaCellSizeZ = 1.0f;
    worldData.areaInvSizeX = 1.0f;
    worldData.areaInvSizeZ = 1.0f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = rows;
    world.classId = 2;
    world.classData = &worldData;

    g_PickupTypes[7] = {};
    g_PickupTypes[7].typeIndex = 7;
    g_PickupTypes[7].defaultAmount = 5;
    g_PickupTypes[7].nameSuffixMax = 2;
    g_PickupTypes[7].templateNode = &templateRoot;
    g_Pickup_SceneNode = &world;
    g_NextPickupId = 500;

    PickupParsedZrdEntry entry = {};
    entry.typeDesc = &g_PickupTypes[7];
    entry.amount = 13;
    entry.position.x = 1.0f;
    entry.position.y = 2.0f;
    entry.position.z = 3.0f;
    entry.rotation.x = 0.4f;
    entry.rotation.y = 0.5f;
    entry.rotation.z = 0.6f;
    entry.param = 88;
    entry.unknown_2c = 99;
    entry.respawnDelay = 12.5f;

    PickupSpawnDef *const spawn = Pickup::SpawnFromParsedZrdEntry(&entry);
    zClass_Object3DDataPartial *const spawnedData =
        spawn != nullptr ? static_cast<zClass_Object3DDataPartial *>(spawn->pickupObj->classData)
                         : nullptr;
    int failure = 0;
    if (spawn == nullptr) {
        failure = 2;
    } else if (std::strcmp(spawn->name, "pu00702") != 0 || spawn->pickupId != 500 ||
               spawn->pickupType != &g_PickupTypes[7] || spawn->amount != 13 ||
               spawn->spawnParam != 88 || spawn->respawnDelay != 12.5f) {
        failure = 3;
    } else if (spawn->position.x != 1.0f || spawn->position.y != 2.0f ||
               spawn->position.z != 3.0f || spawn->rotation.x != 0.4f ||
               spawn->rotation.y != 0.5f || spawn->rotation.z != 0.6f) {
        failure = 4;
    } else if (g_NextPickupId != 501 || g_PickupTypes[7].nameSuffixMax != 3 ||
               g_PickupSpawnList_Primary.head != spawn ||
               g_PickupSpawnList_Primary.tail != spawn || g_PickupSpawnList_Primary.count != 1) {
        failure = 5;
    } else if (world.listCountB != 1 || world.listB[0] != spawn->pickupObj ||
               spawn->pickupObj->listCountA != 1 || spawn->pickupObj->listA[0] != &world) {
        failure = 6;
    } else if (spawnedData == nullptr || spawnedData->localMatrix[9] != 1.0f ||
               spawnedData->localMatrix[10] != 2.0f || spawnedData->localMatrix[11] != 3.0f) {
        failure = 7;
    } else if (spawnedData->rotation.x != 0.4f || spawnedData->rotation.y != 0.5f ||
               spawnedData->rotation.z != 0.6f) {
        failure = 8;
    }

    if (spawn != nullptr) {
        std::free(spawn->pickupObj->listA);
        std::free(spawn);
    }
    std::free(world.listB);
    g_PickupSpawnList_Primary.head = nullptr;
    g_PickupSpawnList_Primary.tail = nullptr;
    g_PickupSpawnList_Primary.count = 0;
    g_PickupTypes[7] = oldType;
    g_Pickup_SceneNode = oldSceneNode;
    g_NextPickupId = oldNextPickupId;

    return failure;
}

extern "C" int pickup_remove_other_spawns_same_opt_entry_smoke(void) {
    PickupSpawnList::Primary_Init();

    PickupType matchingType = {};
    PickupType otherType = {};
    OptCatalogEntryDef *const matchingOpt =
        reinterpret_cast<OptCatalogEntryDef *>(&matchingType);
    OptCatalogEntryDef *const otherOpt =
        reinterpret_cast<OptCatalogEntryDef *>(&otherType);
    matchingType.optEntry = matchingOpt;
    otherType.optEntry = otherOpt;

    PickupSpawnDef *const removeA = NewSpawnDef();
    PickupSpawnDef *const keep = NewSpawnDef();
    PickupSpawnDef *const keepOther = NewSpawnDef();
    PickupSpawnDef *const removeB = NewSpawnDef();
    if (removeA == nullptr || keep == nullptr || keepOther == nullptr || removeB == nullptr) {
        std::free(removeA);
        std::free(keep);
        std::free(keepOther);
        std::free(removeB);
        return 1;
    }

    zClass_NodePartial keptPickupObj = {};
    removeA->pickupId = 1;
    removeA->pickupType = &matchingType;
    removeA->next = keep;
    keep->pickupId = 2;
    keep->pickupType = &matchingType;
    keep->pickupObj = &keptPickupObj;
    keep->next = keepOther;
    keepOther->pickupId = 3;
    keepOther->pickupType = &otherType;
    keepOther->next = removeB;
    removeB->pickupId = 4;
    removeB->pickupType = &matchingType;
    g_PickupSpawnList_Primary.head = removeA;
    g_PickupSpawnList_Primary.tail = removeB;
    g_PickupSpawnList_Primary.count = 4;

    Pickup::RemoveOtherSpawnsWithSameOptEntry(matchingOpt, &keptPickupObj);

    const bool ok = g_PickupSpawnList_Primary.head == keep &&
                    g_PickupSpawnList_Primary.tail == keepOther &&
                    g_PickupSpawnList_Primary.count == 2 &&
                    keep->next == keepOther && keepOther->next == nullptr;

    PickupSpawnList::RemoveAndFreeNode(keep, &g_PickupSpawnList_Primary);
    PickupSpawnList::RemoveAndFreeNode(keepOther, &g_PickupSpawnList_Primary);
    return ok ? 0 : 2;
}

extern "C" int pickup_resolve_owner_from_bvol_hit_smoke(void) {
    zClass_NodePartial ownerNode = {};
    PickupBvolHitCallbackContext context = {};
    context.ownerNode = &ownerNode;

    zClass_NodePartial bvolNode = {};
    bvolNode.flags = 0x40000;
    bvolNode.callbackContext = reinterpret_cast<zClass_NodePartial *>(&context);

    zClass_NodePartial *node = &bvolNode;
    if (Pickup::ResolveOwnerFromBvolHit(&node) != 1 || node != &ownerNode) {
        return 1;
    }

    zClass_NodePartial regularNode = {};
    node = &regularNode;
    if (Pickup::ResolveOwnerFromBvolHit(&node) != 0 || node != &regularNode) {
        return 2;
    }

    node = nullptr;
    return Pickup::ResolveOwnerFromBvolHit(&node) == 0 && node == nullptr ? 0 : 3;
}

extern "C" int pickup_grant_ammo_or_weapon_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    HudUiMessage &message = g_HudUiMgrMessages[2];
    message = {};
    message.base.ftable = &g_HudUiWidget_FTable;
    message.widget.ftable = &g_HudUiWidget_FTable;
    reinterpret_cast<HudUiPanel *>(&message.panel)->vtbl = &g_HudUiPanelSimple_FTable;
    zVidImagePartial images[5] = {};
    message.variantImages[0] = &images[0];
    message.variantImages[3] = &images[3];
    message.sideImageSwaps[0] = &images[4];
    TestFieldAt<int>(&message.panel, 0x2a4) = 0;

    OptCatalogEntryDef entry = {};
    entry.ammoOrChargeMax = 10.0f;
    PickupType pickupType = {};
    pickupType.msgIdOrClassId = 1;
    pickupType.defaultAmount = 4;

    PlayerAltWeaponBank &bank = playerState.altWeaponBanks[2];
    bank.selectedSide = 1;
    bank.controllerA.optCatalogEntry = &entry;
    bank.controllerA.weaponBankIndex = 2;
    bank.controllerA.weaponSideIndex = 0;
    bank.controllerA.ammoOrCharge = 10.0f;
    bank.controllerB.optCatalogEntry = &entry;
    bank.controllerB.weaponBankIndex = 2;
    bank.controllerB.weaponSideIndex = 1;
    bank.controllerB.flags = 4;
    bank.controllerB.ammoOrCharge = 3.0f;

    char messageBuffer[64] = {};
    if (Pickup::GrantAmmoOrWeapon(&pickupType, messageBuffer, &saveState, 2, 0, 1, 0) != 0 ||
        bank.controllerA.ammoOrCharge != 10.0f) {
        g_HudUiMgrMessages[2] = {};
        return 1;
    }

    const int oldPrimaryDispatchCount = g_HudSensorTracker.primaryGunDispatchCount;
    pickupType.weaponKeyName = "weapon";
    pickupType.optEntry = &entry;
    bank.controllerA.flags = 0;
    bank.controllerA.ammoOrCharge = 6.0f;
    bank.controllerB.flags = 4;
    bank.controllerB.ammoOrCharge = 3.0f;

    if (Pickup::GrantAmmoOrWeapon(&pickupType, messageBuffer, &saveState, 2, 0, 1, 8) != 1) {
        g_HudUiMgrMessages[2] = {};
        return 2;
    }

    const bool weaponOk =
        (bank.controllerA.flags & 4) != 0 &&
        g_HudSensorTracker.primaryGunDispatchCount == oldPrimaryDispatchCount + 1 &&
        bank.controllerA.ammoOrCharge == 10.0f &&
        bank.selectedSide == 1 &&
        message.widget.image == &images[4] &&
        std::strcmp(&TestFieldAt<char>(&message.panel, 0x34), "") == 0;
    if (!weaponOk) {
        g_HudUiMgrMessages[2] = {};
        return 3;
    }

    pickupType.weaponKeyName = nullptr;
    pickupType.optEntry = nullptr;
    pickupType.defaultAmount = 2;
    bank.controllerA.ammoOrCharge = 0.0f;
    bank.controllerB.ammoOrCharge = 0.0f;
    playerState.activeAltGunController = &bank.controllerA;

    if (Pickup::GrantAmmoOrWeapon(&pickupType, messageBuffer, &saveState, 2, 0, 1, 0) != 1) {
        g_HudUiMgrMessages[2] = {};
        return 4;
    }

    const bool ammoOk = bank.controllerA.ammoOrCharge == 2.0f && bank.selectedSide == 0 &&
                        message.base.image == &images[3] &&
                        std::strcmp(&TestFieldAt<char>(&message.panel, 0x34), "2") == 0;

    g_HudUiMgrMessages[2] = {};
    return ammoOk ? 0 : 5;
}

extern "C" int pickup_apply_effect_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    HudUiShieldMessageWidget shield = {};
    HudUiTextStack4 topStack = {};

    saveState.playerState = &playerState;
    playerState.masterCommonData = &commonData;
    commonData.maxHealth = 100.0f;
    commonData.invMaxHealth = 0.01f;

    shield.meter.ftable = &g_HudUiMeter_FTable;
    shield.meter.fillPixelsMax = 20;
    shield.meter.points[1].y = 100.0f;
    reinterpret_cast<HudUiPanel *>(&shield.percentTextPanel)->vtbl =
        &g_HudUiPanelSimple_FTable;

    HudUiShieldMessageWidget *const oldShieldWidget = g_HudUiMgrShieldMessageWidget;
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    const float oldStatusMeterRatio = g_PlayerStatusMeterRatio;
    g_HudUiMgrShieldMessageWidget = &shield;
    g_HudUiTopMessageStack = &topStack;

    playerState.statusMeterValue = 20.0f;
    g_PlayerStatusMeterRatio = 0.2f;
    const int statusAccepted = Pickup::ApplyEffect(0x22, 30, &saveState);
    const bool statusAcceptedOk =
        statusAccepted == 1 && playerState.statusMeterValue == 50.0f &&
        std::strcmp(&TestFieldAt<char>(&shield.percentTextPanel, 0x34), "50") == 0;

    playerState.statusMeterValue = 100.0f;
    g_PlayerStatusMeterRatio = 1.0f;
    const int statusRejected = Pickup::ApplyEffect(0x22, 30, &saveState);
    const bool statusRejectedOk =
        statusRejected == 0 && playerState.statusMeterValue == 100.0f;

    const int invalidResult = Pickup::ApplyEffect(0x40, 0, &saveState);
    const bool invalidOk = invalidResult == 0;

    g_HudUiMgrShieldMessageWidget = oldShieldWidget;
    g_HudUiTopMessageStack = oldTopStack;
    g_PlayerStatusMeterRatio = oldStatusMeterRatio;
    return statusAcceptedOk && statusRejectedOk && invalidOk ? 0 : 1;
}
