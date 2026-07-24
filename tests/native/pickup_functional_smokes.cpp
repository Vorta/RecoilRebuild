#include "Battlesport/pickup.h"

#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zUtil/zsave_game.h"

#include <cstring>

namespace {
int g_pickupSendCalls;
DPID g_pickupSendFromPlayer;
DWORD g_pickupSendFlags;
DWORD g_pickupSendPacketSize;
unsigned char g_pickupSendPacketBytes[64];
void *g_pickupDirectPlayVtable[52];

HRESULT __stdcall CapturePickupSend(
    IDirectPlay4A *,
    DPID fromPlayer,
    DPID,
    DWORD flags,
    void *packet,
    DWORD packetSizeBytes
) {
    ++g_pickupSendCalls;
    g_pickupSendFromPlayer = fromPlayer;
    g_pickupSendFlags = flags;
    g_pickupSendPacketSize = packetSizeBytes;
    if (packet != 0 && packetSizeBytes <= sizeof(g_pickupSendPacketBytes)) {
        std::memcpy(g_pickupSendPacketBytes, packet, packetSizeBytes);
    }
    return DP_OK;
}

IDirectPlay4A *MakePickupDirectPlaySendFake() {
    struct FakeDirectPlayObject {
        void **vtable;
    };
    static FakeDirectPlayObject object;

    std::memset(g_pickupDirectPlayVtable, 0, sizeof(g_pickupDirectPlayVtable));
    /* IDirectPlay4A::Send is the vtable entry at byte offset 0x68. */
    g_pickupDirectPlayVtable[0x68 / sizeof(void *)] =
        reinterpret_cast<void *>(&CapturePickupSend);
    object.vtable = g_pickupDirectPlayVtable;
    return reinterpret_cast<IDirectPlay4A *>(&object);
}
} // namespace

extern "C" int pickup_airdrop_spawn_ref_get_world_pos_smoke(void) {
    zClass_Object3DDataPartial objectData = {};
    objectData.cachedWorldMatrix[9] = 11.0f;
    objectData.cachedWorldMatrix[10] = 22.0f;
    objectData.cachedWorldMatrix[11] = 33.0f;

    zClass_NodePartial carrier = {};
    carrier.classId = 5;
    carrier.flags = 0x80000;
    carrier.classData = &objectData;

    PickupAirdropSpawnRef spawnRef = {};
    spawnRef.carrierNode = &carrier;
    spawnRef.worldPos.x = -1.0f;
    spawnRef.worldPos.y = -2.0f;
    spawnRef.worldPos.z = -3.0f;

    zVec3 *const result = spawnRef.GetWorldPos();
    return result == &spawnRef.worldPos &&
                   spawnRef.worldPos.x == 11.0f &&
                   spawnRef.worldPos.y == 22.0f &&
                   spawnRef.worldPos.z == 33.0f
               ? 0
               : 1;
}

extern "C" int pickup_airdrop_spawn_ref_can_spawn_with_clearance_smoke(void) {
    const PickupSpawnList savedPrimary = g_PickupSpawnList_Primary;
    zInput_GameStateOrMapTablePartial *const savedGameState = g_GameStateOrMapTable;

    zClass_Object3DDataPartial objectData = {};
    objectData.cachedWorldMatrix[9] = 10.0f;
    objectData.cachedWorldMatrix[10] = 0.0f;
    objectData.cachedWorldMatrix[11] = 30.0f;

    zClass_NodePartial carrier = {};
    carrier.classId = 5;
    carrier.flags = 0x80000;
    carrier.classData = &objectData;

    zClass_NodePartial dropAttach = {};
    dropAttach.flags = 4;

    PlayerGunFireController controller = {};
    controller.ammoOrCharge = 2.0f;

    zUtil_PlayerStateStorage playerState = {};
    playerState.activeAltGunController = &controller;

    zInput_GameStateOrMapTablePartial gameState = {};
    gameState.playerState =
        reinterpret_cast<zInput_PlayerStatePartial *>(&playerState);
    g_GameStateOrMapTable = &gameState;

    PickupSpawnDef occupied = {};
    occupied.position = zVec3{12.0f, 0.0f, 33.0f};
    g_PickupSpawnList_Primary.head = 0;

    PickupAirdropSpawnRef spawnRef = {};
    spawnRef.carrierNode = &carrier;
    spawnRef.dropAttachNode = &dropAttach;

    const int clear = spawnRef.CanSpawnWithClearance(5.0f);

    g_PickupSpawnList_Primary.head = &occupied;
    const int occupiedBlocked = spawnRef.CanSpawnWithClearance(5.0f);

    controller.ammoOrCharge = 123456792.0f;
    g_PickupSpawnList_Primary.head = 0;
    const int disabledBlocked = spawnRef.CanSpawnWithClearance(5.0f);

    controller.ammoOrCharge = 2.0f;
    dropAttach.flags = 0;
    const int inactiveBlocked = spawnRef.CanSpawnWithClearance(5.0f);

    g_PickupSpawnList_Primary = savedPrimary;
    g_GameStateOrMapTable = savedGameState;
    return clear == 1 &&
                   occupiedBlocked == 0 &&
                   disabledBlocked == 0 &&
                   inactiveBlocked == 0
               ? 0
               : 1;
}

extern "C" int pickup_spawn_list_has_entry_near_xz_smoke(void) {
    const PickupSpawnList savedPrimary = g_PickupSpawnList_Primary;

    g_PickupSpawnList_Primary.head = 0;
    zVec3 probe = {10.0f, 20.0f, 30.0f};
    if (Pickup::SpawnListHasEntryNearXZ(&probe, 5.0f) != 0) {
        g_PickupSpawnList_Primary = savedPrimary;
        return 1;
    }

    PickupSpawnDef first = {};
    PickupSpawnDef second = {};
    PickupSpawnDef third = {};
    first.position = zVec3{1.0f, 0.0f, 1.0f};
    first.next = &second;
    second.position = zVec3{13.0f, 0.0f, 34.0f};
    second.next = &third;
    third.position = zVec3{15.0f, 0.0f, 30.0f};
    g_PickupSpawnList_Primary.head = &first;

    const int nearHit = Pickup::SpawnListHasEntryNearXZ(&probe, 5.0f);
    second.position.z = 36.0f;
    const int boundaryMiss = Pickup::SpawnListHasEntryNearXZ(&probe, 5.0f);
    probe.x = -100.0f;
    probe.z = -100.0f;
    const int farMiss = Pickup::SpawnListHasEntryNearXZ(&probe, 5.0f);

    g_PickupSpawnList_Primary = savedPrimary;
    return nearHit == 1 && boundaryMiss == 0 && farMiss == 0 ? 0 : 2;
}

extern "C" int pickup_find_droppable_type_for_current_weapon_smoke(void) {
    PickupType savedTypes[34] = {};
    for (int index = 0; index <= 33; ++index) {
        savedTypes[index] = g_PickupTypes[index];
    }

    for (int index = 17; index <= 33; ++index) {
        g_PickupTypes[index] = PickupType{};
        g_PickupTypes[index].weaponKeyName = "unused";
    }
    g_PickupTypes[0] = PickupType{};

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
        Pickup::FindDroppableTypeForPlayerCurrentWeapon(&saveState) ==
        &g_PickupTypes[18];

    g_PickupTypes[33].weaponKeyName = "last";
    optEntry.keyName = const_cast<char *>("last");
    const bool lastOk =
        Pickup::FindDroppableTypeForPlayerCurrentWeapon(&saveState) ==
        &g_PickupTypes[33];

    optEntry.keyName = const_cast<char *>("missing");
    const bool fallbackOk =
        Pickup::FindDroppableTypeForPlayerCurrentWeapon(&saveState) ==
        &g_PickupTypes[0];

    for (int index = 0; index <= 33; ++index) {
        g_PickupTypes[index] = savedTypes[index];
    }

    return matchOk && lastOk && fallbackOk ? 0 : 1;
}

extern "C" int pickup_send_pkt12_airdrop_spawn_chute_relay_smoke(void) {
    zNetwork_DPlay4 *const savedDirectPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const savedLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int savedLocalPlayerKey = g_zNetwork_LocalPlayerKey;

    g_pickupSendCalls = 0;
    g_pickupSendFromPlayer = 0;
    g_pickupSendFlags = 0;
    g_pickupSendPacketSize = 0;
    std::memset(g_pickupSendPacketBytes, 0, sizeof(g_pickupSendPacketBytes));

    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x10203040;
    g_zNetwork_pDirectPlay4 = MakePickupDirectPlaySendFake();
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = localPlayer.playerKey;

    zVec3 spawnPos = {1.0f, 2.0f, 3.0f};
    Pickup::SendPkt12_AirdropSpawnChuteRelay(0x1234, &spawnPos, 77);

    PickupPkt12AirdropSpawnChuteRelay packet = {};
    if (g_pickupSendPacketSize == sizeof(packet)) {
        std::memcpy(&packet, g_pickupSendPacketBytes, sizeof(packet));
    }

    const bool packetOk =
        g_pickupSendCalls == 1 &&
        g_pickupSendFromPlayer == static_cast<DPID>(localPlayer.playerKey) &&
        g_pickupSendPacketSize == sizeof(packet) &&
        packet.header.packetType == 0x12 &&
        packet.header.packetSizeBytes == sizeof(packet) &&
        packet.header.payloadDword0 == localPlayer.playerKey &&
        packet.spawnPos.x == 1.0f &&
        packet.spawnPos.y == 2.0f &&
        packet.spawnPos.z == 3.0f &&
        packet.pickupTypeIndex == 0x1234 &&
        packet.nextPickupId == 77;

    g_zNetwork_pDirectPlay4 = savedDirectPlay;
    g_zNetwork_LocalPlayerRecord = savedLocalPlayer;
    g_zNetwork_LocalPlayerKey = savedLocalPlayerKey;
    return packetOk ? 0 : 1;
}
