#include "Battlesport/GameNet.h"
#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/pickup.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/include/zDi.h"
#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zUtil/zZbd.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" std::uint32_t g_HudUi_InvalidateMask;

namespace {
using TestBackendSimpleFn = std::int32_t(__stdcall *)(void *self);
using TestBackendGetStatusFn = std::int32_t(__stdcall *)(void *self, std::int32_t *status);
using TestBackendPlayDirectSoundFn = std::int32_t(__stdcall *)(void *self,
                                                               std::uint32_t reserved1,
                                                               std::uint32_t reserved2,
                                                               std::uint32_t flags);
using TestBackendSetIntFn = std::int32_t(__stdcall *)(void *self, std::int32_t value);

struct TestDirectSoundBufferVTable {
    void *slots00_1c[8];
    void *GetFrequency;
    TestBackendGetStatusFn GetStatus;
    void *slot28;
    void *slot2c;
    TestBackendPlayDirectSoundFn Play;
    TestBackendSetIntFn SetCurrentPosition;
    void *slot38;
    TestBackendSetIntFn SetVolume;
    TestBackendSetIntFn SetPan;
    TestBackendSetIntFn SetFrequency;
    TestBackendSimpleFn Stop;
};

struct TestDirectSoundBuffer {
    TestDirectSoundBufferVTable *vtable;
};

template <typename T> T *AllocZeroedMalloc() {
    void *const mem = std::calloc(1, sizeof(T));
    return static_cast<T *>(mem);
}

template <typename T> T *AllocZeroedNew() {
    T *const value = static_cast<T *>(::operator new(sizeof(T)));
    std::memset(value, 0, sizeof(T));
    return value;
}

template <typename T> zZbdSectionCallback TestZbdCallbackPtr(T callback) {
    static_assert(sizeof(T) == sizeof(zZbdSectionCallback));
    union {
        T callback;
        zZbdSectionCallback raw;
    } value = {};
    value.callback = callback;
    return value.raw;
}

zZbdManager MakePlayerZbdManager(zZbdSectionHandlerNode &sentinel) {
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    sentinel.sectionHandler = {};

    zZbdManager manager = {};
    manager.sectionHandlerListSentinel = &sentinel;
    return manager;
}

void ClearPlayerRegisteredHandlers(zZbdSectionHandlerNode &sentinel) {
    zZbdSectionHandlerNode *node = sentinel.next;
    while (node != &sentinel) {
        zZbdSectionHandlerNode *next = node->next;
        delete node;
        node = next;
    }
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
}

void SetObjectLocalMatrix(zClass_Object3DDataPartial *data, const zMat4x3 &matrix) {
    std::memcpy(data->localMatrix, &matrix, sizeof(matrix));
}

bool Vec3Equals(const zVec3 &value, const zVec3 &expected) {
    return value.x == expected.x && value.y == expected.y && value.z == expected.z;
}

bool MatrixEquals(const zMat4x3 &value, const zMat4x3 &expected) {
    return value.xx == expected.xx && value.xy == expected.xy && value.xz == expected.xz &&
           value.yx == expected.yx && value.yy == expected.yy && value.yz == expected.yz &&
           value.zx == expected.zx && value.zy == expected.zy && value.zz == expected.zz &&
           value.posX == expected.posX && value.posY == expected.posY &&
           value.posZ == expected.posZ;
}

bool FloatNear(float actual, float expected) {
    return std::fabs(actual - expected) < 0.0001f;
}

float PlayerDampingFactor(float rate, float deltaTime) {
    const int bits = static_cast<int>(-rate * deltaTime * 12102200.0f) + 0x3f800000;
    float factor = 0.0f;
    std::memcpy(&factor, &bits, sizeof(factor));
    return factor;
}

template <typename T>
T &TestFieldAt(void *base, std::size_t offset) {
    return *reinterpret_cast<T *>(static_cast<unsigned char *>(base) + offset);
}

template <typename T>
T &PlayerStateFieldAt(zUtil_PlayerStateStorage &playerState, std::size_t offset) {
    return *reinterpret_cast<T *>(playerState.bytes + offset);
}

template <typename Method>
std::uintptr_t MethodAddress(Method method) {
    std::uintptr_t address = 0;
    std::memcpy(&address, &method, sizeof(address));
    return address;
}

int g_PlayerTestCheckpointNetSendCalls;
std::uint32_t g_PlayerTestCheckpointNetSendFlags;
std::uint32_t g_PlayerTestCheckpointNetSendSize;
unsigned char g_PlayerTestCheckpointNetPacketBytes[0x40];
int g_PlayerTestStopCount;
int g_PlayerTestPlayCount;
int g_PlayerTestSetFrequencyCount;
int g_PlayerTestLastFrequency;
int g_PlayerTestSetPanCount;
int g_PlayerTestSetVolumeCount;
int g_PlayerTestLastVolume;
int g_PlayerTestHudVisibleCount;
void *g_PlayerTestHudVisibleThis[8];
int g_PlayerTestHudVisibleValue[8];
int g_PlayerTestTransferDamageCalls;
float g_PlayerTestTransferDamageArgs[4];

float RECOIL_FASTCALL PlayerTransferDamageTimerCallback(void *context, float damageAmount) {
    const int index = g_PlayerTestTransferDamageCalls;
    if (index < 4) {
        g_PlayerTestTransferDamageArgs[index] = damageAmount;
    }
    ++g_PlayerTestTransferDamageCalls;
    return *static_cast<float *>(context);
}

struct TestPlayerHudVisibleReceiver {
    void RECOIL_THISCALL SetVisible(int visible) {
        const int index = g_PlayerTestHudVisibleCount;
        if (index < 8) {
            g_PlayerTestHudVisibleThis[index] = this;
            g_PlayerTestHudVisibleValue[index] = visible;
        }
        ++g_PlayerTestHudVisibleCount;
    }
};

std::int32_t RECOIL_STDCALL PlayerCheckpointSendFake(zNetwork_DPlay4 *, std::uint32_t,
                                                     std::uint32_t, std::uint32_t flags,
                                                     void *packet,
                                                     std::uint32_t packetSizeBytes) {
    ++g_PlayerTestCheckpointNetSendCalls;
    g_PlayerTestCheckpointNetSendFlags = flags;
    g_PlayerTestCheckpointNetSendSize = packetSizeBytes;
    if (packetSizeBytes <= sizeof(g_PlayerTestCheckpointNetPacketBytes)) {
        std::memcpy(g_PlayerTestCheckpointNetPacketBytes, packet, packetSizeBytes);
    }
    return 0;
}

const zNetwork_DPlay4Vtable kPlayerCheckpointDPlayVtable = {
    {}, nullptr, {}, PlayerCheckpointSendFake, {}, nullptr, {}, nullptr, {}, nullptr,
};

std::int32_t __stdcall TestDirectSoundGetStatus(void *, std::int32_t *status) {
    *status = 0;
    return 0;
}

std::int32_t __stdcall TestDirectSoundSetInt(void *, std::int32_t) {
    return 0;
}

std::int32_t __stdcall TestPlayerDirectSoundSetFrequency(void *, std::int32_t frequency) {
    ++g_PlayerTestSetFrequencyCount;
    g_PlayerTestLastFrequency = frequency;
    return 0;
}

std::int32_t __stdcall TestPlayerDirectSoundSetPan(void *, std::int32_t) {
    ++g_PlayerTestSetPanCount;
    return 0;
}

std::int32_t __stdcall TestPlayerDirectSoundSetVolume(void *, std::int32_t volume) {
    ++g_PlayerTestSetVolumeCount;
    g_PlayerTestLastVolume = volume;
    return 0;
}

std::int32_t __stdcall TestPlayerDirectSoundStop(void *) {
    ++g_PlayerTestStopCount;
    return 0;
}

std::int32_t __stdcall TestDirectSoundPlay(void *, std::uint32_t, std::uint32_t,
                                           std::uint32_t) {
    ++g_PlayerTestPlayCount;
    return 0;
}

PlayerPendingContact *MakePendingContactChain(int count) {
    PlayerPendingContact *head = nullptr;
    PlayerPendingContact *tail = nullptr;
    for (int i = 0; i < count; ++i) {
        PlayerPendingContact *const contact = AllocZeroedNew<PlayerPendingContact>();
        if (head == nullptr) {
            head = contact;
        } else {
            tail->next = contact;
        }
        tail = contact;
    }
    return head;
}

void FillPendingContactQueue(PlayerPendingContactQueue *queue, int count) {
    queue->listAux = 0x12340000 + count;
    queue->head = MakePendingContactChain(count);
    queue->tail = queue->head;
    while (queue->tail != nullptr && queue->tail->next != nullptr) {
        queue->tail = queue->tail->next;
    }
    queue->count = count;
}

bool PendingContactQueueCleared(const PlayerPendingContactQueue &queue) {
    return queue.listAux == 0 && queue.head == nullptr && queue.tail == nullptr &&
           queue.count == 0;
}

bool PendingContactPayloadMatches(const PlayerPendingContact *contact,
                                  const zClassDiPickCandidateEntry &candidate,
                                  const zVec3 &segmentStart, const zVec3 &segmentEnd,
                                  int segmentTag) {
    return contact != nullptr && contact->hit.node == candidate.node &&
           contact->hit.scenePayload == candidate.scenePayload &&
           contact->hit.surfaceNormal.y == candidate.surfaceNormal.y &&
           Vec3Equals(contact->sweepStart, segmentStart) &&
           Vec3Equals(contact->sweepEnd, segmentEnd) && contact->segmentTag == segmentTag;
}

float PlayerFastNormalizeScale(float lengthSq) {
    int lengthSqBits = 0;
    std::memcpy(&lengthSqBits, &lengthSq, sizeof(lengthSqBits));
    lengthSqBits = (lengthSqBits >> 1) + 532676608;

    float approxLength = 0.0f;
    std::memcpy(&approxLength, &lengthSqBits, sizeof(approxLength));
    return g_Player_CollisionContactResolveScale / (approxLength + 0.00000001f);
}
} // namespace

void InitDestroyedEffectEntry(zEffectAnimEntry *entry, zClass_NodePartial *boundNode,
                              zClass_NodePartial *runtimeNode, const char *name);

extern "C" int ainet_free_all_smoke(void) {
    AINet *const first = AllocZeroedMalloc<AINet>();
    AINet *const second = AllocZeroedMalloc<AINet>();
    AINetNode *const nodeA = AllocZeroedMalloc<AINetNode>();
    AINetNode *const nodeB = AllocZeroedMalloc<AINetNode>();

    nodeA->next = nodeB;
    first->nodeListHead = nodeA;
    first->next = second;
    g_AINetListHead = first;

    AINet::FreeAll();
    return g_AINetListHead == nullptr ? 0 : 1;
}

extern "C" int player_ai_discard_negative_branch_nodes_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    AINetNode *const negativeA = AllocZeroedMalloc<AINetNode>();
    AINetNode *const negativeB = AllocZeroedMalloc<AINetNode>();
    AINetNode positive = {};

    negativeA->nodeIndex = -2;
    negativeB->nodeIndex = -1;
    positive.nodeIndex = 3;
    negativeA->neighborNodes[0] = negativeB;
    negativeB->neighborNodes[0] = &positive;
    playerState.aiCurrentPathNode = negativeA;
    saveState.playerState = &playerState;

    Player::AiDiscardNegativeBranchPathNodes(&saveState);
    return playerState.aiCurrentPathNode == &positive ? 0 : 1;
}

extern "C" int player_ai_enter_mode2_steering_pursuit_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const int oldFinalized = g_Player_AiMode2State1Finalized;
    const float oldTime = g_Player_TotalTimeSecScaled;

    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState localGameState = {};
    zUtil_PlayerStateStorage aiState = {};
    zUtil_PlayerStateStorage localPlayerState = {};
    AINetNode currentPathNode = {};
    AINetNode restoreNode = {};

    saveState.playerState = &aiState;
    localGameState.playerState = &localPlayerState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&localGameState;

    aiState.aiTopLevelState = 7;
    aiState.aiSavedTopLevelState = -1;
    aiState.aiCurrentPathNode = &currentPathNode;
    aiState.aiCurrentPathNeighborIndex = 1;
    aiState.aiMode2AttackDwell = 2.5f;
    aiState.aiCurrentSteeringSubstate = 2;
    aiState.worldPos = {13.0f, 7.0f, 4.0f};
    localPlayerState.worldPos = {10.0f, 99.0f, 0.0f};
    restoreNode.position = {21.0f, 22.0f, 23.0f};
    currentPathNode.neighborNodes[1] = &restoreNode;

    g_Player_AiMode2State1Finalized = 1;
    g_Player_TotalTimeSecScaled = 12.25f;
    Player::AiEnterMode2SteeringPursuit(&saveState);
    const bool finalizedGateOk =
        aiState.aiTopLevelState == 7 && aiState.aiSavedTopLevelState == -1 &&
        aiState.aiStateStartTime == 0.0f;

    g_Player_AiMode2State1Finalized = 0;
    Player::AiEnterMode2SteeringPursuit(&saveState);
    const bool dynamicOffsetOk =
        aiState.aiSavedTopLevelState == 7 && aiState.aiTopLevelState == 1 &&
        FloatNear(aiState.aiStateStartTime, 12.25f) &&
        FloatNear(aiState.aiStateEndTime, 14.75f) &&
        Vec3Equals(aiState.aiRestoreTarget, {21.0f, 22.0f, 23.0f}) &&
        FloatNear(aiState.aiDynamicOffsetDir.x, 0.6f) &&
        FloatNear(aiState.aiDynamicOffsetDir.y, 0.0f) &&
        FloatNear(aiState.aiDynamicOffsetDir.z, 0.8f);

    aiState.aiTopLevelState = 1;
    aiState.aiCurrentSteeringSubstate = 0;
    aiState.aiDynamicOffsetDir = {9.0f, 8.0f, 7.0f};
    Player::AiEnterMode2SteeringPursuit(&saveState);
    const bool nonDynamicOk =
        aiState.aiSavedTopLevelState == 1 && aiState.aiTopLevelState == 1 &&
        Vec3Equals(aiState.aiDynamicOffsetDir, {9.0f, 8.0f, 7.0f});

    g_Player_TotalTimeSecScaled = oldTime;
    g_Player_AiMode2State1Finalized = oldFinalized;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    return finalizedGateOk && dynamicOffsetOk && nonDynamicOk ? 0 : 1;
}

extern "C" int player_ai_alert_attack_buddies_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const int oldFinalized = g_Player_AiMode2State1Finalized;
    const float oldTime = g_Time_AccumulatedTimeSec;
    const float oldTotalTime = g_Player_TotalTimeSecScaled;

    zUtil_SaveGameState selfSave = {};
    zUtil_SaveGameState buddySave = {};
    zUtil_SaveGameState steeringBuddySave = {};
    zUtil_SaveGameState localGameState = {};
    zUtil_PlayerStateStorage selfState = {};
    zUtil_PlayerStateStorage buddyState = {};
    zUtil_PlayerStateStorage steeringBuddyState = {};
    zUtil_PlayerStateStorage localPlayerState = {};
    AINetNode currentPathNode = {};
    AINetNode restoreNode = {};

    selfSave.playerState = &selfState;
    buddySave.playerState = &buddyState;
    steeringBuddySave.playerState = &steeringBuddyState;
    localGameState.playerState = &localPlayerState;
    selfSave.aiPeerRingNext = &buddySave;
    buddySave.aiPeerRingNext = &steeringBuddySave;
    steeringBuddySave.aiPeerRingNext = &selfSave;

    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&localGameState;
    g_Player_AiMode2State1Finalized = 0;
    g_Time_AccumulatedTimeSec = 4.5f;
    g_Player_TotalTimeSecScaled = 20.0f;

    buddyState.aiTopLevelState = 7;
    buddyState.aiCurrentPathNode = &currentPathNode;
    buddyState.aiCurrentPathNeighborIndex = 0;
    buddyState.aiMode2AttackDwell = 1.5f;
    buddyState.aiCurrentSteeringSubstate = 0;
    currentPathNode.neighborNodes[0] = &restoreNode;
    restoreNode.position = {2.0f, 3.0f, 4.0f};

    steeringBuddyState.aiTopLevelState = 1;
    steeringBuddyState.recentHitFlag = 0;
    steeringBuddyState.recentHitExpireTime = 0.0f;

    Player::AiAlertAttackBuddies(&selfSave);
    const bool alertedBuddy =
        buddyState.aiTopLevelState == 1 && buddyState.aiSavedTopLevelState == 7 &&
        buddyState.recentHitFlag == 1 && FloatNear(buddyState.recentHitExpireTime, 14.5f) &&
        Vec3Equals(buddyState.aiRestoreTarget, {2.0f, 3.0f, 4.0f});
    const bool skippedSteeringBuddy =
        steeringBuddyState.recentHitFlag == 0 &&
        FloatNear(steeringBuddyState.recentHitExpireTime, 0.0f);

    buddyState.aiTopLevelState = 9;
    buddyState.recentHitFlag = 0;
    buddyState.recentHitExpireTime = 0.0f;
    g_Player_AiMode2State1Finalized = 1;
    Player::AiAlertAttackBuddies(&selfSave);
    const bool finalizedGateOk =
        buddyState.aiTopLevelState == 9 && buddyState.recentHitFlag == 0 &&
        FloatNear(buddyState.recentHitExpireTime, 0.0f);

    g_Player_TotalTimeSecScaled = oldTotalTime;
    g_Time_AccumulatedTimeSec = oldTime;
    g_Player_AiMode2State1Finalized = oldFinalized;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    return alertedBuddy && skippedSteeringBuddy && finalizedGateOk ? 0 : 1;
}

extern "C" int player_find_alt_gun_controller_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    OptCatalogEntryDef entries[3] = {};
    entries[0].keyName = const_cast<char *>("weapon-a");
    entries[0].ordinalIndex = 101;
    entries[1].keyName = const_cast<char *>("weapon-b");
    entries[1].ordinalIndex = 202;
    entries[2].keyName = const_cast<char *>("other");
    entries[2].ordinalIndex = 303;

    const std::int32_t oldEntryCount = g_OptCatalog_EntryCount;
    OptCatalogEntryDef *const oldEntryTable = g_OptCatalog_EntryTable;
    g_OptCatalog_EntryCount = 3;
    g_OptCatalog_EntryTable = entries;

    playerState.altWeaponBanks[4].controllerA.optCatalogEntry = &entries[0];
    playerState.altWeaponBanks[5].controllerB.optCatalogEntry = &entries[1];
    playerState.altWeaponBanks[6].controllerA.optCatalogEntry = &entries[2];
    playerState.altWeaponBanks[6].controllerB.optCatalogEntry = &entries[2];

    const bool ctrlAFound = Player::FindAltGunFireControllerForWeaponId(&saveState, 101) ==
                            &playerState.altWeaponBanks[4].controllerA;
    const bool ctrlBFound = Player::FindAltGunFireControllerForWeaponId(&saveState, 202) ==
                            &playerState.altWeaponBanks[5].controllerB;
    const bool ctrlAPriority = Player::FindAltGunFireControllerForWeaponId(&saveState, 303) ==
                               &playerState.altWeaponBanks[6].controllerA;

    for (std::int32_t i = 2; i < 10; ++i) {
        playerState.altWeaponBanks[i].controllerA.optCatalogEntry = &entries[2];
        playerState.altWeaponBanks[i].controllerB.optCatalogEntry = &entries[2];
    }
    const bool fallback = Player::FindAltGunFireControllerForWeaponId(&saveState, 404) ==
                          &playerState.altWeaponBanks[1].controllerA;

    g_OptCatalog_EntryCount = oldEntryCount;
    g_OptCatalog_EntryTable = oldEntryTable;

    return ctrlAFound && ctrlBFound && ctrlAPriority && fallback ? 0 : 1;
}

extern "C" int player_is_alt_weapon_allowed_in_current_master_mode_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    OptCatalogEntryDef entry = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;

    modalData.masterType = 2;
    entry.flags = 0;
    if (Player::IsAltWeaponAllowedInCurrentMasterMode(&saveState, &entry) != 1) {
        return 1;
    }

    entry.flags = 0x1000;
    if (Player::IsAltWeaponAllowedInCurrentMasterMode(&saveState, &entry) != 0) {
        return 2;
    }

    entry.flags = 0x02;
    if (Player::IsAltWeaponAllowedInCurrentMasterMode(&saveState, &entry) != 0) {
        return 3;
    }

    modalData.masterType = 1;
    entry.flags = 0x1002;
    return Player::IsAltWeaponAllowedInCurrentMasterMode(&saveState, &entry) == 1 ? 0 : 4;
}

extern "C" int player_auto_switch_to_next_usable_alt_weapon_smoke(void) {
    zUtil_SaveGameState *const oldLocalSaveState = g_LocalPlayerSaveState;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    HudUiTextStack4 *const oldTopMessageStack = g_HudUiTopMessageStack;
    const int oldActiveWeaponMessageIndex = g_HudUiMgrActiveWeaponMessageIndex;
    const int oldActiveWeaponSideIndex = g_HudUiMgrActiveWeaponSideIndex;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    modalData.masterType = 1;

    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    gameStateOrMap.playerState =
        static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&playerState));
    g_GameStateOrMapTable = &gameStateOrMap;
    g_LocalPlayerSaveState = &saveState;

    HudUiTopMessageStack topStack = {};
    topStack.Constructor();
    topStack.base.enabled = 0;
    g_HudUiTopMessageStack = &topStack;

    zVidImagePartial images[10][5] = {};
    for (int i = 0; i < 10; ++i) {
        HudUiMessage &message = g_HudUiMgrMessages[i];
        message = {};
        message.base.ftable = &g_HudUiWidget_FTable;
        message.widget.ftable = &g_HudUiWidget_FTable;
        reinterpret_cast<HudUiPanel *>(&message.panel)->vtbl = &g_HudUiPanelSimple_FTable;
        message.variantImages[3] = &images[i][3];
        message.variantImages[4] = &images[i][4];
    }
    TestFieldAt<int>(&g_HudUiMgrMessages[4].panel, 0x2a4) = 1;
    TestFieldAt<int>(&g_HudUiMgrMessages[6].panel, 0x2a4) = 1;
    g_HudUiMgrActiveWeaponMessageIndex = 0;
    g_HudUiMgrActiveWeaponSideIndex = 0;

    OptCatalogEntryDef entryA = {};
    entryA.description = const_cast<char *>("auto-a");
    OptCatalogEntryDef entryB = {};
    entryB.description = const_cast<char *>("auto-b");

    auto reset_state = [&]() {
        playerState = {};
        saveState.playerState = &playerState;
        saveState.primaryModalState = &modalState;
        playerState.altGunTransitionState = 1;
        modalData.masterType = 1;
    };
    auto arm_controller = [&](PlayerGunFireController &controller, OptCatalogEntryDef *entry,
                              int bankIndex, int sideIndex, float ammo) {
        controller.optCatalogEntry = entry;
        controller.weaponBankIndex = bankIndex;
        controller.weaponSideIndex = sideIndex;
        controller.flags = 4;
        controller.ammoOrCharge = ammo;
    };
    auto cleanup = [&]() {
        g_LocalPlayerSaveState = oldLocalSaveState;
        g_GameStateOrMapTable = oldGameStateOrMapTable;
        g_HudUiTopMessageStack = oldTopMessageStack;
        g_HudUiMgrActiveWeaponMessageIndex = oldActiveWeaponMessageIndex;
        g_HudUiMgrActiveWeaponSideIndex = oldActiveWeaponSideIndex;
        for (int i = 0; i < 10; ++i) {
            g_HudUiMgrMessages[i] = {};
        }
    };

    reset_state();
    PlayerAltWeaponBank &sameBank = playerState.altWeaponBanks[4];
    sameBank.selectedSide = 0;
    arm_controller(sameBank.controllerA, &entryA, 4, 0, 0.0f);
    arm_controller(sameBank.controllerB, &entryB, 4, 1, 5.0f);
    playerState.activeAltGunController = &sameBank.controllerA;
    playerState.activeAltBankIndex = 4;
    Player::AutoSwitchToNextUsableAltWeapon(&saveState);
    if (playerState.activeAltGunController != &sameBank.controllerB) {
        cleanup();
        return 1;
    }
    if (playerState.activeAltBankIndex != 4) {
        cleanup();
        return 11;
    }
    if (playerState.cachedAltSelectionCode != 401) {
        cleanup();
        return 12;
    }

    reset_state();
    PlayerAltWeaponBank &activeBank = playerState.altWeaponBanks[5];
    activeBank.selectedSide = 0;
    arm_controller(activeBank.controllerA, &entryA, 5, 0, 0.0f);
    arm_controller(activeBank.controllerB, &entryB, 5, 1, 0.0f);
    playerState.activeAltGunController = &activeBank.controllerB;
    playerState.activeAltBankIndex = 5;
    PlayerAltWeaponBank &lowerBank = playerState.altWeaponBanks[4];
    lowerBank.selectedSide = 0;
    arm_controller(lowerBank.controllerA, &entryA, 4, 0, 6.0f);
    Player::AutoSwitchToNextUsableAltWeapon(&saveState);
    if (playerState.activeAltGunController != &lowerBank.controllerA ||
        playerState.activeAltBankIndex != 4 || playerState.cachedAltSelectionCode != 400) {
        cleanup();
        return 2;
    }

    reset_state();
    PlayerAltWeaponBank &activeBankUp = playerState.altWeaponBanks[4];
    activeBankUp.selectedSide = 0;
    arm_controller(activeBankUp.controllerA, &entryA, 4, 0, 0.0f);
    arm_controller(activeBankUp.controllerB, &entryB, 4, 1, 0.0f);
    playerState.activeAltGunController = &activeBankUp.controllerA;
    playerState.activeAltBankIndex = 4;
    PlayerAltWeaponBank &upperBank = playerState.altWeaponBanks[6];
    upperBank.selectedSide = 1;
    arm_controller(upperBank.controllerA, &entryA, 6, 0, 0.0f);
    arm_controller(upperBank.controllerB, &entryB, 6, 1, 7.0f);
    Player::AutoSwitchToNextUsableAltWeapon(&saveState);
    const bool upwardOk = playerState.activeAltGunController == &upperBank.controllerB &&
                          playerState.activeAltBankIndex == 6 &&
                          playerState.cachedAltSelectionCode == 601;

    cleanup();
    return upwardOk ? 0 : 3;
}

extern "C" int player_alt_gun_fire_point_selection_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    playerState.worldPos = {10.0f, 20.0f, 30.0f};
    playerState.steerBasisRaw = {1.0f, 2.0f, 3.0f};
    PlayerGunFireSlot sentinelSlot = {};
    PlayerGunFireSlot *outSlot = &sentinelSlot;
    Player::SelectAltGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.altFireOrigin, {10.0f, 21.0f, 30.0f}) ||
        !Vec3Equals(playerState.aimBasisOrigin, {10.0f, 21.0f, 30.0f}) ||
        !Vec3Equals(playerState.gunFireDir, {1.0f, 2.0f, 3.0f}) || outSlot != &sentinelSlot) {
        return 1;
    }

    zClass_Object3DDataPartial gunData = {};
    zClass_NodePartial gunNode = {};
    gunNode.classId = 5;
    gunNode.classData = &gunData;

    zClass_Object3DDataPartial turretData = {};
    zClass_NodePartial turretNode = {};
    turretNode.classId = 5;
    turretNode.classData = &turretData;

    zMat4x3 gunMatrix = {0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 3.0f, 0.0f, 5.0f, 7.0f, 0.0f, 0.0f, 0.0f};
    zMat4x3 turretMatrix = {11.0f, 0.0f, 13.0f, 0.0f, 0.0f, 0.0f,
                            17.0f, 0.0f, 19.0f, 0.0f, 0.0f, 0.0f};
    playerState.gunFireTransform = {23.0f, 29.0f, 31.0f, 37.0f, 41.0f, 43.0f,
                                    47.0f, 53.0f, 59.0f, 61.0f, 67.0f, 71.0f};
    playerState.aimBasisOrigin = {101.0f, 102.0f, 103.0f};
    playerState.gunNode = &gunNode;
    playerState.turretNode = &turretNode;
    SetObjectLocalMatrix(&gunData, gunMatrix);
    SetObjectLocalMatrix(&turretData, turretMatrix);

    const float yawX = turretMatrix.zx * playerState.gunFireTransform.xx +
                       turretMatrix.zz * playerState.gunFireTransform.zx;
    const float yawY = turretMatrix.zx * playerState.gunFireTransform.xy +
                       turretMatrix.zz * playerState.gunFireTransform.zy;
    const float yawZ = turretMatrix.zx * playerState.gunFireTransform.xz +
                       turretMatrix.zz * playerState.gunFireTransform.zz;
    const zMat4x3 expectedComposed = {
        turretMatrix.xx * playerState.gunFireTransform.xx +
            turretMatrix.xz * playerState.gunFireTransform.zx,
        turretMatrix.xx * playerState.gunFireTransform.xy +
            turretMatrix.xz * playerState.gunFireTransform.zy,
        turretMatrix.xx * playerState.gunFireTransform.xz +
            turretMatrix.xz * playerState.gunFireTransform.zz,
        playerState.gunFireTransform.yx * gunMatrix.yy + gunMatrix.yz * yawX,
        playerState.gunFireTransform.yy * gunMatrix.yy + gunMatrix.yz * yawY,
        playerState.gunFireTransform.yz * gunMatrix.yy + gunMatrix.yz * yawZ,
        playerState.gunFireTransform.yx * gunMatrix.zy + gunMatrix.zz * yawX,
        playerState.gunFireTransform.yy * gunMatrix.zy + gunMatrix.zz * yawY,
        playerState.gunFireTransform.yz * gunMatrix.zy + gunMatrix.zz * yawZ,
        101.0f,
        102.0f,
        103.0f};
    zMat4x3 composed = {};
    Player::ComposeAimBasisWorldMatrix(&saveState, &composed);
    if (!MatrixEquals(composed, expectedComposed)) {
        return 2;
    }

    const zMat4x3 identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    SetObjectLocalMatrix(&gunData, identityMatrix);
    SetObjectLocalMatrix(&turretData, identityMatrix);
    playerState.gunFireTransform = identityMatrix;
    playerState.aimBasisOrigin = {10.0f, 20.0f, 30.0f};

    PlayerGunFireController controller = {};
    zClass_NodePartial primaryAttachNode = {};
    zClass_NodePartial secondaryAttachNode = {};
    zClass_NodePartial activeAttachState = {};
    controller.attachNodePrimary = &primaryAttachNode;
    controller.attachNodeSecondary = &secondaryAttachNode;
    playerState.activeAltGunController = &controller;
    playerState.firePointCenter = {1.0f, 2.0f, 3.0f};
    playerState.firePointRight = {4.0f, 5.0f, 6.0f};
    playerState.firePointLeft = {7.0f, 8.0f, 9.0f};

    outSlot = nullptr;
    playerState.altHardpointSelectState = 0;
    controller.attachState = nullptr;
    Player::SelectAltGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.altFireOrigin, {11.0f, 22.0f, 33.0f}) ||
        outSlot != &playerState.altFireSlotCenter ||
        playerState.altFireSlotCenter.attachNode != &primaryAttachNode ||
        playerState.altHardpointSelectState != 0) {
        return 3;
    }

    outSlot = nullptr;
    controller.attachState = &activeAttachState;
    Player::SelectAltGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.altFireOrigin, {10.0f, 20.0f, 30.0f}) ||
        outSlot != &playerState.altFireSlotCenter ||
        playerState.altFireSlotCenter.attachNode != &primaryAttachNode ||
        playerState.altHardpointSelectState != 0) {
        return 4;
    }

    outSlot = nullptr;
    controller.attachState = nullptr;
    playerState.altHardpointSelectState = 1;
    Player::SelectAltGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.altFireOrigin, {14.0f, 25.0f, 36.0f}) ||
        outSlot != &playerState.altFireSlotRight ||
        playerState.altFireSlotRight.attachNode != &secondaryAttachNode ||
        playerState.altHardpointSelectState != 2) {
        return 5;
    }

    outSlot = nullptr;
    playerState.altHardpointSelectState = 2;
    Player::SelectAltGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.altFireOrigin, {17.0f, 28.0f, 39.0f}) ||
        outSlot != &playerState.altFireSlotLeft ||
        playerState.altFireSlotLeft.attachNode != &primaryAttachNode ||
        playerState.altHardpointSelectState != 1) {
        return 6;
    }

    return 0;
}

extern "C" int player_alt_gun_fire_slot_offset_smoke(void) {
    const float oldFrameDelta = g_FrameDeltaTimeSec;
    g_FrameDeltaTimeSec = 0.0f;

    zClass_Object3DDataPartial directNodeData = {};
    zClass_NodePartial directNode = {};
    directNode.classId = 5;
    directNode.classData = &directNodeData;
    const zMat4x3 identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    SetObjectLocalMatrix(&directNodeData, identityMatrix);

    PlayerGunFireSlot slot = {};
    slot.offset = 2.0f;
    slot.attachNode = &directNode;
    Player::DecayAndApplyAltFireSlotOffsetToNode(&slot, &directNode, 0.25f, 1);
    if (!FloatNear(slot.offset, 2.0f) || !FloatNear(directNodeData.localMatrix[10], -0.5f) ||
        !FloatNear(directNodeData.localMatrix[11], 2.0f)) {
        g_FrameDeltaTimeSec = oldFrameDelta;
        return 1;
    }

    slot.offset = 0.005f;
    directNodeData.localMatrix[10] = 7.0f;
    directNodeData.localMatrix[11] = 8.0f;
    Player::DecayAndApplyAltFireSlotOffsetToNode(&slot, &directNode, 3.0f, 0);
    if (slot.offset != 0.0f || directNodeData.localMatrix[10] != 0.0f ||
        directNodeData.localMatrix[11] != 0.0f) {
        g_FrameDeltaTimeSec = oldFrameDelta;
        return 2;
    }

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;
    zClass_NodePartial gunNode = {};
    playerState.gunNode = &gunNode;
    playerState.gunFireDir.y = 4.0f;

    zClass_Object3DDataPartial leftData = {};
    zClass_Object3DDataPartial rightData = {};
    zClass_Object3DDataPartial centerData = {};
    zClass_NodePartial leftNode = {};
    zClass_NodePartial rightNode = {};
    zClass_NodePartial centerNode = {};
    leftNode.classId = 5;
    rightNode.classId = 5;
    centerNode.classId = 5;
    leftNode.classData = &leftData;
    rightNode.classData = &rightData;
    centerNode.classData = &centerData;
    SetObjectLocalMatrix(&leftData, identityMatrix);
    SetObjectLocalMatrix(&rightData, identityMatrix);
    SetObjectLocalMatrix(&centerData, identityMatrix);
    playerState.altFireSlotLeft.offset = 1.0f;
    playerState.altFireSlotRight.offset = -2.0f;
    playerState.altFireSlotCenter.offset = 3.0f;
    playerState.altFireSlotLeft.attachNode = &leftNode;
    playerState.altFireSlotRight.attachNode = &rightNode;
    playerState.altFireSlotCenter.attachNode = &centerNode;

    Player::ApplyGunFireSlotOffsetToNode(&saveState);
    if (playerState.altFireSlotLeft.offset != 0.0f ||
        playerState.altFireSlotRight.offset != 0.0f ||
        playerState.altFireSlotCenter.offset != 0.0f || leftData.localMatrix[10] != 0.0f ||
        leftData.localMatrix[11] != 0.0f || rightData.localMatrix[10] != 0.0f ||
        rightData.localMatrix[11] != 0.0f || centerData.localMatrix[10] != 0.0f ||
        centerData.localMatrix[11] != 0.0f) {
        g_FrameDeltaTimeSec = oldFrameDelta;
        return 3;
    }

    playerState.gunNode = nullptr;
    playerState.altFireSlotLeft.offset = 5.0f;
    leftData.localMatrix[10] = 6.0f;
    leftData.localMatrix[11] = 7.0f;
    Player::ApplyGunFireSlotOffsetToNode(&saveState);
    const bool gunlessUnchanged = playerState.altFireSlotLeft.offset == 5.0f &&
                                  leftData.localMatrix[10] == 6.0f &&
                                  leftData.localMatrix[11] == 7.0f;

    g_FrameDeltaTimeSec = oldFrameDelta;
    return gunlessUnchanged ? 0 : 4;
}

extern "C" int player_build_mission_save_data_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    zClass_NodePartial cameraNode = {};
    zClass_CameraDataPartial cameraData = {};
    OptCatalogEntryDef hitSource = {};

    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        PlayerAltWeaponBank &bank = playerState.altWeaponBanks[bankIndex];
        bank.selectedSide = bankIndex & 1;
        bank.controllerA.flags = bankIndex & 1 ? 4 : 0;
        bank.controllerA.ammoOrCharge = static_cast<float>(10 + bankIndex);
        bank.controllerA.weaponBankIndex = bankIndex;
        bank.controllerA.weaponSideIndex = 0;
        bank.controllerB.flags = bankIndex & 1 ? 0 : 4;
        bank.controllerB.ammoOrCharge = static_cast<float>(20 + bankIndex);
        bank.controllerB.weaponBankIndex = bankIndex;
        bank.controllerB.weaponSideIndex = 1;
    }

    playerState.activeAltGunController = &playerState.altWeaponBanks[3].controllerB;
    playerState.activePrimaryGunController = &playerState.altWeaponBanks[4].controllerA;
    playerState.amphibUnlocked = 1;
    playerState.hoverUnlocked = 2;
    playerState.subUnlocked = 3;
    playerState.aiMode = 4;
    playerState.nextModeSwitchAllowedTime = 12.5f;
    playerState.motionInput = 5;
    playerState.autoTurnSign = -1;
    playerState.bankInput = 6;
    playerState.timedHitStatus.runtimeFlags = 1;
    playerState.timedHitStatus.hitSource = &hitSource;
    playerState.timedHitStatus.currentLevel = 0.25f;
    playerState.timedHitStatus.targetLevel = -0.75f;
    playerState.timedHitStatus.lightNode = &cameraNode;
    playerState.timedHitStatus.nextUpdateTime = 50.0f;
    playerState.timedHitStatus.lightParentNode = &cameraNode;
    hitSource.ordinalIndex = 77;

    modalData.masterType = 11;
    modalState.masterModalData = &modalData;
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    cameraData.cameraFlags = 2;
    cameraData.worldTarget = {31.0f, 32.0f, 33.0f};
    cameraData.targetOrEuler = {21.0f, 22.0f, 23.0f};
    cameraData.posOffset = {41.0f, 42.0f, 43.0f};

    zUtil_SaveGameState *const oldLocalSaveState = g_LocalPlayerSaveState;
    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    const float oldStatusMeterRatio = g_PlayerStatusMeterRatio;
    const int oldHudCounterValue = g_Player_HudCounterValue;
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;

    g_LocalPlayerSaveState = &saveState;
    g_MainCamera = &cameraNode;
    g_PlayerStatusMeterRatio = 0.625f;
    g_Player_HudCounterValue = 1234;
    g_Time_AccumulatedTimeSec = 45.0f;

    PlayerMissionSaveData outData = {};
    Player::BuildMissionSaveData(&outData);

    bool ok = outData.size == sizeof(PlayerMissionSaveData) &&
              outData.altWeaponBankIndex == 3 && outData.altWeaponSideIndex == 1 &&
              outData.primaryWeaponBankIndex == 4 && outData.primaryWeaponSideIndex == 0 &&
              outData.playerStatusMeterRatio == 0.625f && outData.hudCounterValue == 1234 &&
              outData.amphibUnlocked == 1 && outData.hoverUnlocked == 2 &&
              outData.subUnlocked == 3 && outData.aiMode == 4 &&
              outData.nextModeSwitchAllowedTime == 12.5f && outData.motionInput == 5 &&
              outData.autoTurnSign == -1 && outData.bankInput == 6 &&
              outData.playerMasterType == 11 &&
              Vec3Equals(outData.cameraTarget, cameraData.worldTarget) &&
              Vec3Equals(outData.cameraPosition, cameraData.posOffset) &&
              outData.timedHitStatus.runtimeFlags == 1 &&
              outData.timedHitStatus.savedHitSourceEntryId == 77 &&
              outData.timedHitStatus.currentLevel == 0.25f &&
              outData.timedHitStatus.targetLevel == -0.75f &&
              outData.timedHitStatus.lightNode == nullptr &&
              outData.timedHitStatus.nextUpdateTime == 5.0f &&
              outData.timedHitStatus.lightParentNode == &cameraNode;

    for (int bankIndex = 0; ok && bankIndex < 10; ++bankIndex) {
        const PlayerMissionSaveWeaponBank &savedBank = outData.weaponBank[bankIndex];
        const PlayerAltWeaponBank &sourceBank = playerState.altWeaponBanks[bankIndex];
        ok = savedBank.selectedSide == sourceBank.selectedSide &&
             savedBank.sides[0].enabled == ((sourceBank.controllerA.flags >> 2) & 1) &&
             savedBank.sides[0].ammoOrCharge == sourceBank.controllerA.ammoOrCharge &&
             savedBank.sides[1].enabled == ((sourceBank.controllerB.flags >> 2) & 1) &&
             savedBank.sides[1].ammoOrCharge == sourceBank.controllerB.ammoOrCharge;
    }

    g_LocalPlayerSaveState = oldLocalSaveState;
    g_MainCamera = oldMainCamera;
    g_PlayerStatusMeterRatio = oldStatusMeterRatio;
    g_Player_HudCounterValue = oldHudCounterValue;
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    return ok ? 0 : 1;
}

extern "C" int player_apply_mission_save_data_smoke(void) {
    zUtil_SaveGameState *const oldLocalSaveState = g_LocalPlayerSaveState;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    HudUiShieldMessageWidget *const oldShieldWidget = g_HudUiMgrShieldMessageWidget;
    HudUiCounterTextPanel *const oldObjectiveCounter = g_HudUiMgrObjectiveCounterTextPanel;
    const int oldActiveWeaponMessageIndex = g_HudUiMgrActiveWeaponMessageIndex;
    const int oldActiveWeaponSideIndex = g_HudUiMgrActiveWeaponSideIndex;
    const float oldStatusMeterRatio = g_PlayerStatusMeterRatio;
    const int oldHudCounterValue = g_Player_HudCounterValue;
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;
    const std::uint32_t oldInvalidateMask = g_HudUi_InvalidateMask;
    HudUiNanitePanel oldNanitePanel = g_HudUiMgrNanitePanel;

    HudUiMessage oldMessages[10] = {};
    for (int index = 0; index < 10; ++index) {
        oldMessages[index] = g_HudUiMgrMessages[index];
    }

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    PlayerMasterModalData flyModalData = {};
    PlayerModalState flyModal = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial cameraNode = {};
    zClass_CameraDataPartial cameraData = {};
    HudUiShieldMessageWidget shield = {};
    HudUiCounterTextPanel counter = {};
    zVidImagePartial messageImages[10][4] = {};

    rootNode.classId = 5;
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;

    commonData.maxHealth = 200.0f;
    playerState.masterCommonData = &commonData;
    playerState.rootNode = &rootNode;
    playerState.statusMeterValue = 50.0f;
    playerState.nanitePanelLevel = 2;
    playerState.primaryGunGateUntilTime = 99.0f;
    playerState.damageProtectionActive = 1;
    playerState.timedHitStatus.runtimeFlags = 0xffffu;
    playerState.timedHitStatus.currentLevel = 9.0f;

    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        PlayerAltWeaponBank &bank = playerState.altWeaponBanks[bankIndex];
        bank.selectedSide = 0;
        bank.controllerA.weaponBankIndex = bankIndex;
        bank.controllerA.weaponSideIndex = 0;
        bank.controllerA.flags = 0;
        bank.controllerA.ammoOrCharge = 1.0f;
        bank.controllerB.weaponBankIndex = bankIndex;
        bank.controllerB.weaponSideIndex = 1;
        bank.controllerB.flags = 0;
        bank.controllerB.ammoOrCharge = 2.0f;

        HudUiMessage &message = g_HudUiMgrMessages[bankIndex];
        message = {};
        message.base.ftable = &g_HudUiWidget_FTable;
        message.widget.ftable = &g_HudUiWidget_FTable;
        reinterpret_cast<HudUiPanel *>(&message.panel)->vtbl = &g_HudUiPanelSimple_FTable;
        message.variantImages[0] = &messageImages[bankIndex][0];
        message.variantImages[1] = &messageImages[bankIndex][1];
        message.variantImages[4] = &messageImages[bankIndex][2];
        message.sideImageSwaps[0] = &messageImages[bankIndex][2];
        message.sideImageSwaps[1] = &messageImages[bankIndex][3];
        TestFieldAt<int>(&message.panel, 0x2a4) = bankIndex & 1;
    }

    playerState.activeAltGunController = &playerState.altWeaponBanks[1].controllerA;
    playerState.activePrimaryGunController = &playerState.altWeaponBanks[2].controllerB;

    flyModalData.masterType = 1;
    flyModal.masterModalData = &flyModalData;
    saveState.playerState = &playerState;
    saveState.primaryModalState = &flyModal;
    saveState.modalStateListHead = &flyModal;
    saveState.modalStateListTail = &flyModal;
    saveState.modalStateCount = 1;

    shield.meter.ftable = &g_HudUiMeter_FTable;
    shield.meter.fillPixelsMax = 20;
    shield.meter.points[1].y = 100.0f;
    reinterpret_cast<HudUiPanel *>(&shield.percentTextPanel)->vtbl =
        &g_HudUiPanelSimple_FTable;

    g_HudUiMgrNanitePanel = {};
    g_HudUiMgrNanitePanel.base.ftable =
        reinterpret_cast<const HudUiCommon_FTable *>(&g_HudUiTripletPanel_FTable);
    for (int index = 0; index < 3; ++index) {
        g_HudUiMgrNanitePanel.items[index].ftable = &g_HudUiWidget_FTable;
    }

    reinterpret_cast<HudUiPanel *>(&counter)->vtbl =
        reinterpret_cast<const HudUiPanel_FTable *>(&g_HudUiCounterTextPanel_FTable);

    PlayerMissionSaveData saveData = {};
    saveData.size = sizeof(saveData);
    saveData.altWeaponBankIndex = 3;
    saveData.altWeaponSideIndex = 1;
    saveData.primaryWeaponBankIndex = 4;
    saveData.primaryWeaponSideIndex = 0;
    saveData.playerStatusMeterRatio = 0.375f;
    saveData.hudCounterValue = 345;
    saveData.amphibUnlocked = 1;
    saveData.hoverUnlocked = 0;
    saveData.subUnlocked = 1;
    saveData.aiMode = 7;
    saveData.nextModeSwitchAllowedTime = 12.25f;
    saveData.motionInput = 2;
    saveData.autoTurnSign = -1;
    saveData.bankInput = 3;
    saveData.playerMasterType = 1;
    saveData.cameraTarget = {11.0f, 12.0f, 13.0f};
    saveData.cameraPosition = {21.0f, 22.0f, 23.0f};
    saveData.timedHitStatus.runtimeFlags = 0;
    saveData.timedHitStatus.savedHitSourceEntryId = 77;
    saveData.timedHitStatus.currentLevel = 0.25f;
    saveData.timedHitStatus.targetLevel = 0.5f;
    saveData.timedHitStatus.lightNode = nullptr;
    saveData.timedHitStatus.nextUpdateTime = 8.0f;
    saveData.timedHitStatus.lightParentNode = nullptr;

    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        PlayerMissionSaveWeaponBank &bank = saveData.weaponBank[bankIndex];
        bank.selectedSide = bankIndex & 1;
        bank.sides[0].enabled = bankIndex % 3 == 0 ? 1 : 0;
        bank.sides[0].ammoOrCharge = static_cast<float>(10 + bankIndex);
        bank.sides[1].enabled = bankIndex % 2 == 0 ? 1 : 0;
        bank.sides[1].ammoOrCharge = static_cast<float>(20 + bankIndex);
    }

    g_LocalPlayerSaveState = &saveState;
    g_GameStateOrMapTable = nullptr;
    g_MainCamera = &cameraNode;
    g_HudUiMgrShieldMessageWidget = &shield;
    g_HudUiMgrObjectiveCounterTextPanel = &counter;
    g_HudUiMgrActiveWeaponMessageIndex = 0;
    g_HudUiMgrActiveWeaponSideIndex = 0;
    g_HudUi_InvalidateMask = 0x80;
    g_PlayerStatusMeterRatio = 0.0f;
    g_Player_HudCounterValue = 0;
    g_Time_AccumulatedTimeSec = 100.0f;

    Player::ApplyMissionSaveData(&saveData);

    int failure = 0;
    if (playerState.altWeaponBanks[3].selectedSide != 1) {
        failure = 1;
    } else if (playerState.activeAltGunController != &playerState.altWeaponBanks[3].controllerB) {
        failure = 6;
    } else if (playerState.activePrimaryGunController !=
               &playerState.altWeaponBanks[4].controllerA) {
        failure = 7;
    } else if (playerState.cachedAltSelectionCode != 301) {
        failure = 8;
    } else if (playerState.cachedPrimarySelectionCode != 400) {
        failure = 9;
    } else if (g_PlayerStatusMeterRatio != 0.375f || g_Player_HudCounterValue != 345 ||
               playerState.amphibUnlocked != 1 || playerState.hoverUnlocked != 0 ||
               playerState.subUnlocked != 1 || playerState.aiMode != 7 ||
               playerState.nextModeSwitchAllowedTime != 12.25f ||
               playerState.motionInput != 2 || playerState.autoTurnSign != -1 ||
               playerState.bankInput != 3 || playerState.primaryGunGateUntilTime != 0.0f ||
               playerState.damageProtectionActive != 0) {
        failure = 2;
    } else if (!Vec3Equals(cameraData.targetOrEuler, saveData.cameraTarget) ||
               !Vec3Equals(cameraData.posOffset, saveData.cameraPosition)) {
        failure = 3;
    } else if (playerState.timedHitStatus.runtimeFlags != 0 ||
               playerState.timedHitStatus.currentLevel != 0.25f ||
               playerState.timedHitStatus.targetLevel != 0.5f ||
               playerState.timedHitStatus.nextUpdateTime != 8.0f ||
               playerState.timedHitStatus.lightParentNode != &rootNode) {
        failure = 4;
    } else if (std::strcmp(&TestFieldAt<char>(&counter, 0x34), "345") != 0 ||
               shield.meter.points[0].y != 95.0f ||
               g_HudUiMgrNanitePanel.visibleCount != 2) {
        failure = 5;
    }

    for (int bankIndex = 0; failure == 0 && bankIndex < 10; ++bankIndex) {
        const PlayerMissionSaveWeaponBank &savedBank = saveData.weaponBank[bankIndex];
        const PlayerAltWeaponBank &bank = playerState.altWeaponBanks[bankIndex];
        if (bank.selectedSide != savedBank.selectedSide ||
            ((bank.controllerA.flags >> 2) & 1) != (savedBank.sides[0].enabled & 1) ||
            ((bank.controllerB.flags >> 2) & 1) != (savedBank.sides[1].enabled & 1) ||
            bank.controllerA.ammoOrCharge != savedBank.sides[0].ammoOrCharge ||
            bank.controllerB.ammoOrCharge != savedBank.sides[1].ammoOrCharge) {
            failure = 20 + bankIndex;
        }
    }

    if (reinterpret_cast<HudUiPanel *>(&counter)->hFont != nullptr) {
        DeleteObject(reinterpret_cast<HudUiPanel *>(&counter)->hFont);
        reinterpret_cast<HudUiPanel *>(&counter)->hFont = nullptr;
    }
    g_LocalPlayerSaveState = oldLocalSaveState;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_MainCamera = oldMainCamera;
    g_HudUiMgrShieldMessageWidget = oldShieldWidget;
    g_HudUiMgrObjectiveCounterTextPanel = oldObjectiveCounter;
    g_HudUiMgrActiveWeaponMessageIndex = oldActiveWeaponMessageIndex;
    g_HudUiMgrActiveWeaponSideIndex = oldActiveWeaponSideIndex;
    g_PlayerStatusMeterRatio = oldStatusMeterRatio;
    g_Player_HudCounterValue = oldHudCounterValue;
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    g_HudUiMgrNanitePanel = oldNanitePanel;
    for (int index = 0; index < 10; ++index) {
        g_HudUiMgrMessages[index] = oldMessages[index];
    }

    return failure;
}

extern "C" int player_zar_read_mission_save_data_section_smoke(void) {
    zUtil_SaveGameState *const oldLocalSaveState = g_LocalPlayerSaveState;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    HudUiShieldMessageWidget *const oldShieldWidget = g_HudUiMgrShieldMessageWidget;
    HudUiCounterTextPanel *const oldObjectiveCounter = g_HudUiMgrObjectiveCounterTextPanel;
    HudLayoutBase *const oldCurrentLayout = g_HudUiMgrCurrentLayout;
    const zTag4Partial oldVariantTag = g_Variant_CurrentTag;
    const int oldActiveWeaponMessageIndex = g_HudUiMgrActiveWeaponMessageIndex;
    const int oldActiveWeaponSideIndex = g_HudUiMgrActiveWeaponSideIndex;
    const std::uint32_t oldInvalidateMask = g_HudUi_InvalidateMask;
    HudUiNanitePanel oldNanitePanel = g_HudUiMgrNanitePanel;
    HudUiCounter oldModeCounters[4] = {};
    HudUiMessage oldMessages[10] = {};
    for (int index = 0; index < 10; ++index) {
        oldMessages[index] = g_HudUiMgrMessages[index];
    }
    for (int index = 0; index < 4; ++index) {
        oldModeCounters[index] = g_HudUiMgrModeCounters[index];
    }
    PlayerNodeFlagRestoreEntry *const oldRestoreBegin = g_PlayerNodeFlagRestoreEntriesBegin;
    PlayerNodeFlagRestoreEntry *const oldRestoreEnd = g_PlayerNodeFlagRestoreEntriesEnd;
    PlayerNodeFlagRestoreEntry *const oldRestoreCapacity =
        g_PlayerNodeFlagRestoreEntriesCapacityEnd;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    PlayerMasterModalData flyModalData = {};
    PlayerModalState flyModal = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial cameraNode = {};
    zClass_CameraDataPartial cameraData = {};
    HudUiShieldMessageWidget shield = {};
    HudUiCounterTextPanel counter = {};
    zVidImagePartial messageImages[10][4] = {};
    zVidImagePartial modeImages[6] = {};
    zClass_NodePartial restoreNode = {};
    PlayerNodeFlagRestoreEntry restoreEntries[1] = {};

    rootNode.classId = 5;
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    commonData.maxHealth = 100.0f;
    playerState.masterCommonData = &commonData;
    playerState.rootNode = &rootNode;
    playerState.statusMeterValue = 25.0f;
    playerState.nanitePanelLevel = 1;
    playerState.lifecycleState = 1;

    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        PlayerAltWeaponBank &bank = playerState.altWeaponBanks[bankIndex];
        bank.controllerA.weaponBankIndex = bankIndex;
        bank.controllerA.weaponSideIndex = 0;
        bank.controllerB.weaponBankIndex = bankIndex;
        bank.controllerB.weaponSideIndex = 1;

        HudUiMessage &message = g_HudUiMgrMessages[bankIndex];
        message = {};
        message.base.ftable = &g_HudUiWidget_FTable;
        message.widget.ftable = &g_HudUiWidget_FTable;
        reinterpret_cast<HudUiPanel *>(&message.panel)->vtbl = &g_HudUiPanelSimple_FTable;
        message.variantImages[0] = &messageImages[bankIndex][0];
        message.variantImages[1] = &messageImages[bankIndex][1];
        message.variantImages[4] = &messageImages[bankIndex][2];
        message.sideImageSwaps[0] = &messageImages[bankIndex][2];
        message.sideImageSwaps[1] = &messageImages[bankIndex][3];
    }

    playerState.activeAltGunController = &playerState.altWeaponBanks[3].controllerB;
    playerState.activePrimaryGunController = &playerState.altWeaponBanks[4].controllerA;

    flyModalData.masterType = 1;
    flyModal.masterModalData = &flyModalData;
    saveState.playerState = &playerState;
    saveState.primaryModalState = &flyModal;
    saveState.modalStateListHead = &flyModal;

    shield.meter.ftable = &g_HudUiMeter_FTable;
    shield.meter.fillPixelsMax = 20;
    shield.meter.points[1].y = 100.0f;
    reinterpret_cast<HudUiPanel *>(&shield.percentTextPanel)->vtbl =
        &g_HudUiPanelSimple_FTable;

    g_HudUiMgrNanitePanel = {};
    g_HudUiMgrNanitePanel.base.ftable =
        reinterpret_cast<const HudUiCommon_FTable *>(&g_HudUiTripletPanel_FTable);
    for (int index = 0; index < 3; ++index) {
        g_HudUiMgrNanitePanel.items[index].ftable = &g_HudUiWidget_FTable;
    }
    for (int index = 1; index < 4; ++index) {
        g_HudUiMgrModeCounters[index] = {};
        reinterpret_cast<HudUiWidget *>(&g_HudUiMgrModeCounters[index])->ftable =
            reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCounter_FTable);
        TestFieldAt<zVidImagePartial *>(&g_HudUiMgrModeCounters[index], 0xbc) =
            &modeImages[(index - 1) * 2];
        TestFieldAt<zVidImagePartial *>(&g_HudUiMgrModeCounters[index], 0xc0) =
            &modeImages[(index - 1) * 2 + 1];
    }

    reinterpret_cast<HudUiPanel *>(&counter)->vtbl =
        reinterpret_cast<const HudUiPanel_FTable *>(&g_HudUiCounterTextPanel_FTable);

    restoreEntries[0].node = &restoreNode;
    restoreEntries[0].wasPickable = 1;
    g_PlayerNodeFlagRestoreEntriesBegin = restoreEntries;
    g_PlayerNodeFlagRestoreEntriesEnd = restoreEntries + 1;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = restoreEntries + 1;

    PlayerMissionSaveData saveData = {};
    saveData.size = sizeof(saveData);
    saveData.altWeaponBankIndex = 3;
    saveData.altWeaponSideIndex = 1;
    saveData.primaryWeaponBankIndex = 4;
    saveData.primaryWeaponSideIndex = 0;
    saveData.hudCounterValue = 12;
    saveData.playerStatusMeterRatio = 0.25f;
    saveData.amphibUnlocked = 1;
    saveData.hoverUnlocked = 1;
    saveData.subUnlocked = 0;
    saveData.playerMasterType = 1;
    saveData.cameraTarget = {1.0f, 2.0f, 3.0f};
    saveData.cameraPosition = {4.0f, 5.0f, 6.0f};
    saveData.lastValidCameraVariantTag.count = 2;
    saveData.lastValidCameraVariantTag.tags[0] = 9;
    saveData.lastValidCameraVariantTag.tags[1] = 10;
    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        saveData.weaponBank[bankIndex].selectedSide = bankIndex & 1;
        saveData.weaponBank[bankIndex].sides[0].enabled = 1;
        saveData.weaponBank[bankIndex].sides[0].ammoOrCharge = 3.0f;
        saveData.weaponBank[bankIndex].sides[1].enabled = 1;
        saveData.weaponBank[bankIndex].sides[1].ammoOrCharge = 4.0f;
    }

    g_LocalPlayerSaveState = &saveState;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);
    g_MainCamera = &cameraNode;
    g_HudUiMgrShieldMessageWidget = &shield;
    g_HudUiMgrObjectiveCounterTextPanel = &counter;
    g_HudUiMgrCurrentLayout = nullptr;
    g_HudUiMgrActiveWeaponMessageIndex = 0;
    g_HudUiMgrActiveWeaponSideIndex = 0;
    g_HudUi_InvalidateMask = 0x80;

    Player::ZAR_ReadMissionSaveDataSection(nullptr, nullptr, &saveData, sizeof(saveData),
                                           nullptr);

    int failure = 0;
    if (g_Variant_CurrentTag.count != 2 || g_Variant_CurrentTag.tags[0] != 9 ||
        g_Variant_CurrentTag.tags[1] != 10) {
        failure = 1;
    } else if (playerState.activeAltGunController !=
                   &playerState.altWeaponBanks[3].controllerB ||
               playerState.activePrimaryGunController !=
                   &playerState.altWeaponBanks[4].controllerA) {
        failure = 2;
    } else if (g_HudUiMgrNanitePanel.visibleCount != 1) {
        failure = 3;
    } else if (shield.meter.points[0].y != 95.0f) {
        failure = 4;
    } else if ((restoreNode.flags & 0x20) == 0) {
        failure = 5;
    }

    g_LocalPlayerSaveState = oldLocalSaveState;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_MainCamera = oldMainCamera;
    g_HudUiMgrShieldMessageWidget = oldShieldWidget;
    g_HudUiMgrObjectiveCounterTextPanel = oldObjectiveCounter;
    g_HudUiMgrCurrentLayout = oldCurrentLayout;
    g_Variant_CurrentTag = oldVariantTag;
    g_HudUiMgrActiveWeaponMessageIndex = oldActiveWeaponMessageIndex;
    g_HudUiMgrActiveWeaponSideIndex = oldActiveWeaponSideIndex;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    g_HudUiMgrNanitePanel = oldNanitePanel;
    for (int index = 0; index < 10; ++index) {
        g_HudUiMgrMessages[index] = oldMessages[index];
    }
    for (int index = 0; index < 4; ++index) {
        g_HudUiMgrModeCounters[index] = oldModeCounters[index];
    }
    g_PlayerNodeFlagRestoreEntriesBegin = oldRestoreBegin;
    g_PlayerNodeFlagRestoreEntriesEnd = oldRestoreEnd;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = oldRestoreCapacity;
    return failure;
}

extern "C" int player_refresh_hud_from_state_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState localSaveState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage localPlayerState = {};
    PlayerMasterCommonData localCommonData = {};
    HudUiShieldMessageWidget shield = {};
    zVidImagePartial images[40] = {};

    saveState.playerState = &playerState;
    localSaveState.playerState = &localPlayerState;
    localPlayerState.masterCommonData = &localCommonData;
    localCommonData.maxHealth = 100.0f;
    playerState.statusMeterValue = 25.0f;
    playerState.nanitePanelLevel = 3;
    playerState.amphibUnlocked = 1;
    playerState.hoverUnlocked = 0;
    playerState.subUnlocked = 1;

    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        HudUiMessage &message = g_HudUiMgrMessages[bankIndex];
        message = {};
        message.base.ftable = &g_HudUiWidget_FTable;
        message.widget.ftable = &g_HudUiWidget_FTable;
        reinterpret_cast<HudUiPanel *>(&message.panel)->vtbl = &g_HudUiPanelSimple_FTable;
        message.variantImages[0] = &images[bankIndex * 4 + 0];
        message.variantImages[1] = &images[bankIndex * 4 + 1];
        message.variantImages[4] = &images[bankIndex * 4 + 2];
        message.sideImageSwaps[0] = &images[bankIndex * 4 + 2];
        message.sideImageSwaps[1] = &images[bankIndex * 4 + 3];
    }

    playerState.altWeaponBanks[0].controllerA.flags = 4;
    playerState.altWeaponBanks[0].controllerA.ammoOrCharge = 5.0f;
    playerState.altWeaponBanks[0].controllerB.flags = 4;
    playerState.altWeaponBanks[0].controllerB.ammoOrCharge = 7.0f;

    playerState.altWeaponBanks[1].controllerB.flags = 4;
    playerState.altWeaponBanks[1].controllerB.ammoOrCharge = 9.0f;

    playerState.altWeaponBanks[2].controllerA.ammoOrCharge = 4.0f;

    playerState.altWeaponBanks[3].controllerA.flags = 4;
    playerState.altWeaponBanks[3].controllerB.flags = 4;
    playerState.altWeaponBanks[3].controllerB.ammoOrCharge = 11.0f;

    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        playerState.altWeaponBanks[bankIndex].controllerA.weaponBankIndex = bankIndex;
        playerState.altWeaponBanks[bankIndex].controllerA.weaponSideIndex = 0;
        playerState.altWeaponBanks[bankIndex].controllerB.weaponBankIndex = bankIndex;
        playerState.altWeaponBanks[bankIndex].controllerB.weaponSideIndex = 1;
    }
    playerState.activeAltGunController = &playerState.altWeaponBanks[3].controllerB;
    playerState.activePrimaryGunController = &playerState.altWeaponBanks[1].controllerB;

    shield.meter.ftable = &g_HudUiMeter_FTable;
    shield.meter.fillPixelsMax = 20;
    shield.meter.points[1].y = 100.0f;
    reinterpret_cast<HudUiPanel *>(&shield.percentTextPanel)->vtbl =
        &g_HudUiPanelSimple_FTable;
    g_HudUiMgrShieldMessageWidget = &shield;

    g_HudUiMgrNanitePanel.base.ftable =
        reinterpret_cast<const HudUiCommon_FTable *>(&g_HudUiTripletPanel_FTable);
    for (int index = 0; index < 3; ++index) {
        g_HudUiMgrNanitePanel.items[index].ftable = &g_HudUiWidget_FTable;
    }

    zVidImagePartial counterImages[6] = {};
    for (int index = 1; index < 4; ++index) {
        g_HudUiMgrModeCounters[index] = {};
        reinterpret_cast<HudUiWidget *>(&g_HudUiMgrModeCounters[index])->ftable =
            reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCounter_FTable);
        TestFieldAt<zVidImagePartial *>(&g_HudUiMgrModeCounters[index], 0xbc) =
            &counterImages[(index - 1) * 2];
        TestFieldAt<zVidImagePartial *>(&g_HudUiMgrModeCounters[index], 0xc0) =
            &counterImages[(index - 1) * 2 + 1];
    }

    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const std::uint32_t oldInvalidateMask = g_HudUi_InvalidateMask;
    g_GameStateOrMapTable =
        reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&localSaveState);
    g_HudUi_InvalidateMask = 0x80;

    Player::RefreshHudFromState(&saveState);

    const bool shieldOk =
        shield.meter.color565 == (zVid_PackColorRGB(255, 255, 0) & 0xffffu) &&
        shield.meter.points[0].y == 95.0f && shield.meter.points[3].y == 95.0f &&
        std::strcmp(&TestFieldAt<char>(&shield.percentTextPanel, 0x34), "25") == 0;
    const bool naniteOk = g_HudUiMgrNanitePanel.visibleCount == 3;
    const bool bank0Ok =
        playerState.altWeaponBanks[0].selectedSide == 0 &&
        g_HudUiMgrMessages[0].base.image == &images[0] &&
        g_HudUiMgrMessages[0].widget.image == &images[3] &&
        std::strcmp(&TestFieldAt<char>(&g_HudUiMgrMessages[0].panel, 0x34), "5") == 0;
    const bool bank1Ok =
        playerState.altWeaponBanks[1].selectedSide == 1 &&
        g_HudUiMgrMessages[1].base.image == &images[6] &&
        g_HudUiMgrMessages[1].widget.image == nullptr &&
        std::strcmp(&TestFieldAt<char>(&g_HudUiMgrMessages[1].panel, 0x34), "9") == 0;
    const bool bank2Ok =
        g_HudUiMgrMessages[2].base.image == nullptr &&
        std::strcmp(&TestFieldAt<char>(&g_HudUiMgrMessages[2].panel, 0x34), "4") == 0;
    const bool activeOk =
        playerState.altWeaponBanks[3].selectedSide == 1 &&
        g_HudUiMgrMessages[3].base.image == &images[14] &&
        std::strcmp(&TestFieldAt<char>(&g_HudUiMgrMessages[3].panel, 0x34), "11") == 0 &&
        g_HudUiMgrActiveWeaponMessageIndex == 3 && g_HudUiMgrActiveWeaponSideIndex == 1;
    const bool modesOk =
        reinterpret_cast<HudUiWidget *>(&g_HudUiMgrModeCounters[1])->image == &counterImages[1] &&
        reinterpret_cast<HudUiWidget *>(&g_HudUiMgrModeCounters[2])->image == &counterImages[2] &&
        reinterpret_cast<HudUiWidget *>(&g_HudUiMgrModeCounters[3])->image == &counterImages[5];

    g_HudUi_InvalidateMask = oldInvalidateMask;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_HudUiMgrShieldMessageWidget = nullptr;
    if (!shieldOk) {
        return 1;
    }
    if (!naniteOk) {
        return 2;
    }
    if (!bank0Ok) {
        return 3;
    }
    if (!bank1Ok) {
        return 4;
    }
    if (!bank2Ok) {
        return 5;
    }
    if (!activeOk) {
        return 6;
    }
    if (!modesOk) {
        return 7;
    }
    return 0;
}

extern "C" int player_apply_status_meter_change_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    HudUiShieldMessageWidget shield = {};

    saveState.playerState = &playerState;
    playerState.masterCommonData = &commonData;
    commonData.maxHealth = 80.0f;
    commonData.invMaxHealth = 0.0125f;
    playerState.statusMeterValue = 30.0f;

    shield.meter.ftable = &g_HudUiMeter_FTable;
    shield.meter.fillPixelsMax = 20;
    shield.meter.points[1].y = 100.0f;
    reinterpret_cast<HudUiPanel *>(&shield.percentTextPanel)->vtbl =
        &g_HudUiPanelSimple_FTable;

    HudUiShieldMessageWidget *const oldShieldWidget = g_HudUiMgrShieldMessageWidget;
    const float oldStatusMeterRatio = g_PlayerStatusMeterRatio;
    const std::uint32_t oldInvalidateMask = g_HudUi_InvalidateMask;
    g_HudUiMgrShieldMessageWidget = &shield;
    g_PlayerStatusMeterRatio = -1.0f;
    g_HudUi_InvalidateMask = 0;

    Player::ApplyStatusMeterChange(&saveState, 0, 60.0f);
    const bool replaceOk =
        playerState.statusMeterValue == 60.0f && g_PlayerStatusMeterRatio == 0.75f &&
        shield.meter.points[0].y == 85.0f &&
        std::strcmp(&TestFieldAt<char>(&shield.percentTextPanel, 0x34), "75") == 0;

    Player::ApplyStatusMeterChange(&saveState, 1, 50.0f);
    const bool maxClampOk =
        playerState.statusMeterValue == 80.0f && g_PlayerStatusMeterRatio == 1.0f &&
        shield.meter.points[0].y == 80.0f &&
        std::strcmp(&TestFieldAt<char>(&shield.percentTextPanel, 0x34), "100") == 0;

    Player::ApplyStatusMeterChange(&saveState, 0, -7.0f);
    const bool minClampOk =
        playerState.statusMeterValue == 0.0f && g_PlayerStatusMeterRatio == 0.0f &&
        shield.meter.points[0].y == 100.0f &&
        std::strcmp(&TestFieldAt<char>(&shield.percentTextPanel, 0x34), "0") == 0;

    g_HudUiMgrShieldMessageWidget = oldShieldWidget;
    g_PlayerStatusMeterRatio = oldStatusMeterRatio;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    return replaceOk && maxClampOk && minClampOk ? 0 : 1;
}

extern "C" int player_reset_damage_state_and_timed_hit_status_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    playerState.damageProtectionActive = 11;
    playerState.queuedFixedDamageFlag = 12;
    playerState.damageVisualFlag = 13;
    playerState.timedHitStatus.runtimeFlags = 3;
    playerState.timedHitStatus.currentLevel = 0.25f;
    playerState.timedHitStatus.targetLevel = -0.5f;
    playerState.timedHitStatus.nextUpdateTime = 7.0f;
    playerState.timedHitStatus.lightNode = nullptr;

    Player::ResetDamageStateAndTimedHitStatus(&saveState);

    return playerState.damageProtectionActive == 0 &&
                   playerState.queuedFixedDamageFlag == 0 &&
                   playerState.damageVisualFlag == 0 &&
                   playerState.timedHitStatus.runtimeFlags == 3 &&
                   playerState.timedHitStatus.currentLevel == 0.25f &&
                   playerState.timedHitStatus.targetLevel == -0.5f &&
                   playerState.timedHitStatus.nextUpdateTime == 7.0f
               ? 0
               : 1;
}

extern "C" int player_destroyed_state_reset_local_finalize_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    HudUiShieldMessageWidget shield = {};
    HudUiTextStack4 topStack = {};

    saveState.playerState = &playerState;
    playerState.masterCommonData = &commonData;
    playerState.lifecycleState = 4;
    playerState.cameraState = 3;
    playerState.thirdPersonYawOffset = 8.0f;
    playerState.cameraElevationOffset = 9.0f;
    playerState.damageProtectionActive = 1;
    playerState.queuedFixedDamageFlag = 1;
    playerState.damageVisualFlag = 1;
    playerState.statusMeterValue = 10.0f;
    commonData.maxHealth = 100.0f;
    commonData.invMaxHealth = 0.01f;

    shield.meter.ftable = &g_HudUiMeter_FTable;
    shield.meter.fillPixelsMax = 20;
    shield.meter.points[1].y = 100.0f;
    reinterpret_cast<HudUiPanel *>(&shield.percentTextPanel)->vtbl =
        &g_HudUiPanelSimple_FTable;

    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    int *const oldGameControlOptions = ZOPT_GAME_CONTROL_OPTIONS;
    HudUiShieldMessageWidget *const oldShieldWidget = g_HudUiMgrShieldMessageWidget;
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    const float oldStatusMeterRatio = g_PlayerStatusMeterRatio;
    const int oldPrevCameraState = g_PlayerPrevCameraState;
    const int oldPrevSteeringMode = g_PlayerPrevSteeringMode;
    const std::uint32_t oldInvalidateMask = g_HudUi_InvalidateMask;
    HMODULE const oldMessagesDll = g_zLoc_MessagesDllHandle;

    int gameControlOptions = 0;
    HMODULE messagesDll = LoadLibraryA("support\\messages.dll");
    if (messagesDll == nullptr) {
        messagesDll = LoadLibraryA("..\\..\\..\\..\\support\\messages.dll");
    }
    if (messagesDll == nullptr) {
        return 6;
    }
    g_GameStateOrMapTable =
        reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);
    ZOPT_GAME_CONTROL_OPTIONS = &gameControlOptions;
    g_HudUiMgrShieldMessageWidget = &shield;
    g_HudUiTopMessageStack = &topStack;
    g_PlayerStatusMeterRatio = 0.1f;
    g_PlayerPrevCameraState = 3;
    g_PlayerPrevSteeringMode = 1;
    g_HudUi_InvalidateMask = 0;
    g_zLoc_MessagesDllHandle = messagesDll;

    Player::DestroyedStateResetLocalFinalize();

    const bool lifecycleOk = playerState.lifecycleState == 1;
    const bool steeringOk = (gameControlOptions & 0x02) != 0;
    const bool mouseOk = playerState.thirdPersonYawOffset == 0.0f &&
                         playerState.cameraElevationOffset == 0.0f;
    const bool damageOk = playerState.damageProtectionActive == 0 &&
                          playerState.queuedFixedDamageFlag == 0 &&
                          playerState.damageVisualFlag == 0;
    const bool statusOk = playerState.statusMeterValue == 100.0f &&
                          g_PlayerStatusMeterRatio == 1.0f &&
                          std::strcmp(&TestFieldAt<char>(&shield.percentTextPanel, 0x34),
                                      "100") == 0;

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    ZOPT_GAME_CONTROL_OPTIONS = oldGameControlOptions;
    g_HudUiMgrShieldMessageWidget = oldShieldWidget;
    g_HudUiTopMessageStack = oldTopStack;
    g_PlayerStatusMeterRatio = oldStatusMeterRatio;
    g_PlayerPrevCameraState = oldPrevCameraState;
    g_PlayerPrevSteeringMode = oldPrevSteeringMode;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    g_zLoc_MessagesDllHandle = oldMessagesDll;
    FreeLibrary(messagesDll);
    if (!lifecycleOk) {
        return 1;
    }
    if (!steeringOk) {
        return 2;
    }
    if (!mouseOk) {
        return 3;
    }
    if (!damageOk) {
        return 4;
    }
    return statusOk ? 0 : 5;
}

extern "C" int player_destroyed_state_reset_finalize_callback_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    GameNetPlayerRow row = {};
    HudUiShieldMessageWidget shield = {};
    HudUiTextStack4 topStack = {};

    saveState.playerState = &playerState;
    saveState.netPlayerRow = &row;
    playerState.masterCommonData = &commonData;
    playerState.lifecycleState = 4;
    playerState.cameraState = 3;
    playerState.cameraTransitionTimer = 123;
    playerState.statusMeterValue = 10.0f;
    commonData.maxHealth = 120.0f;
    commonData.invMaxHealth = 1.0f / 120.0f;
    row.playerColorIndex = 1;

    shield.meter.ftable = &g_HudUiMeter_FTable;
    shield.meter.fillPixelsMax = 20;
    shield.meter.points[1].y = 100.0f;
    reinterpret_cast<HudUiPanel *>(&shield.percentTextPanel)->vtbl =
        &g_HudUiPanelSimple_FTable;

    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    int *const oldGameControlOptions = ZOPT_GAME_CONTROL_OPTIONS;
    HudUiShieldMessageWidget *const oldShieldWidget = g_HudUiMgrShieldMessageWidget;
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    GameNetPlayerRow *const oldRowHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldRowTail = g_GameNetPlayerRowTail;
    const int oldRowCount = g_GameNetPlayerRowCount;
    HMODULE const oldMessagesDll = g_zLoc_MessagesDllHandle;
    const float oldStatusMeterRatio = g_PlayerStatusMeterRatio;
    const int oldPrevCameraState = g_PlayerPrevCameraState;
    const int oldPrevSteeringMode = g_PlayerPrevSteeringMode;
    const std::uint32_t oldInvalidateMask = g_HudUi_InvalidateMask;

    HMODULE messagesDll = LoadLibraryA("support\\messages.dll");
    if (messagesDll == nullptr) {
        messagesDll = LoadLibraryA("..\\..\\..\\..\\support\\messages.dll");
    }
    if (messagesDll == nullptr) {
        return 6;
    }

    int gameControlOptions = 0;
    g_GameStateOrMapTable =
        reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);
    ZOPT_GAME_CONTROL_OPTIONS = &gameControlOptions;
    g_HudUiMgrShieldMessageWidget = &shield;
    g_HudUiTopMessageStack = &topStack;
    g_GameNetPlayerRowHead = nullptr;
    g_GameNetPlayerRowTail = nullptr;
    g_GameNetPlayerRowCount = 0;
    g_zLoc_MessagesDllHandle = messagesDll;
    g_PlayerStatusMeterRatio = 10.0f / 120.0f;
    g_PlayerPrevCameraState = 3;
    g_PlayerPrevSteeringMode = 1;
    g_HudUi_InvalidateMask = 0;

    Player::DestroyedStateResetFinalizeCallback(&saveState);

    const bool ok = playerState.lifecycleState == 1 &&
                    playerState.statusMeterValue == 120.0f &&
                    playerState.cameraTransitionTimer == 0 &&
                    (gameControlOptions & 0x02) != 0 &&
                    g_PlayerStatusMeterRatio == 1.0f &&
                    std::strcmp(&TestFieldAt<char>(&shield.percentTextPanel, 0x34),
                                "100") == 0;

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    ZOPT_GAME_CONTROL_OPTIONS = oldGameControlOptions;
    g_HudUiMgrShieldMessageWidget = oldShieldWidget;
    g_HudUiTopMessageStack = oldTopStack;
    g_GameNetPlayerRowHead = oldRowHead;
    g_GameNetPlayerRowTail = oldRowTail;
    g_GameNetPlayerRowCount = oldRowCount;
    g_zLoc_MessagesDllHandle = oldMessagesDll;
    g_PlayerStatusMeterRatio = oldStatusMeterRatio;
    g_PlayerPrevCameraState = oldPrevCameraState;
    g_PlayerPrevSteeringMode = oldPrevSteeringMode;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    FreeLibrary(messagesDll);
    return ok ? 0 : 1;
}

extern "C" int player_destroyed_state_reset_callback_smoke(void) {
    const int oldMissionId = g_HudSensorTracker.missionId;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    zZbdManager *const oldZbdManager = g_zUtil_ZbdManager;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zUtil_SaveGameState *const oldLocalPlayerSaveState = g_LocalPlayerSaveState;
    const int oldAllowMaps = g_GameNetStatus_AllowMaps;
    GameNetSpawnPoint *const oldSpawnHead = g_GameNetSpawnPointHead;
    GameNetSpawnPoint *const oldSpawnTail = g_GameNetSpawnPointTail;
    const unsigned int oldSpawnCount = g_GameNetSpawnPointCount;
    HudLayoutBase *const oldCurrentLayout = g_HudUiMgrCurrentLayout;
    HudUiShieldMessageWidget *const oldShieldWidget = g_HudUiMgrShieldMessageWidget;
    const std::uint32_t oldInvalidateMask = g_HudUi_InvalidateMask;
    const int oldActiveWeaponMessageIndex = g_HudUiMgrActiveWeaponMessageIndex;
    const int oldActiveWeaponSideIndex = g_HudUiMgrActiveWeaponSideIndex;
    const HudUiNanitePanel oldNanitePanel = g_HudUiMgrNanitePanel;
    zVidTexturePackEntry *const oldBuiltinTexturePacks = g_zVid_BuiltinTexturePacks;
    const int oldBuiltinTexturePackCount = g_zVid_BuiltinTexturePackCount;
    const int oldTexDirEntryCount = g_zImage_TexDirEntryCount;
    HudUiMessage oldMessages[10] = {};
    HudUiCounter oldModeCounters[4] = {};
    for (int index = 0; index < 10; ++index) {
        oldMessages[index] = g_HudUiMgrMessages[index];
    }
    for (int index = 0; index < 4; ++index) {
        oldModeCounters[index] = g_HudUiMgrModeCounters[index];
    }

    zClass_Object3D_ModelRefLerpQueue::Reset();

    int networkEnabled = 0;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_HudSensorTracker.missionId = 8;

    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager = MakePlayerZbdManager(sentinel);
    g_zUtil_ZbdManager = &manager;

    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState localSaveState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage localPlayerState = {};
    PlayerMasterCommonData commonData = {};
    PlayerMasterCommonData localCommonData = {};
    PlayerMasterModalData sourceData = {};
    PlayerMasterModalData trackData = {};
    PlayerModalState sourceModal = {};
    PlayerModalState trackModal = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial modeVariantNode = {};
    zClass_NodePartial runtimeNode = {};
    zClass_Object3DDataPartial rootData = {};
    zEffectAnimEntry destroyedRespawn = {};
    HudUiShieldMessageWidget shield = {};
    zVidImagePartial messageImages[40] = {};
    zVidImagePartial counterImages[6] = {};
    zVidTexturePackEntry builtinTexturePack = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &sourceModal;
    saveState.modalStateListHead = &sourceModal;
    saveState.modalStateListTail = &trackModal;
    saveState.modalStateCount = 2;
    localSaveState.playerState = &localPlayerState;
    sourceModal.masterModalData = &sourceData;
    sourceModal.next = &trackModal;
    trackModal.masterModalData = &trackData;
    sourceData.masterType = 1;
    trackData.masterType = 3;
    playerState.masterCommonData = &commonData;
    localPlayerState.masterCommonData = &localCommonData;
    commonData.maxHealth = 120.0f;
    commonData.invMaxHealth = 1.0f / 120.0f;
    localCommonData.maxHealth = 120.0f;
    localCommonData.invMaxHealth = 1.0f / 120.0f;

    rootNode.classId = 5;
    rootNode.classData = &rootData;
    std::strcpy(rootNode.name, "destroyed_root");
    modeVariantNode.classId = 2;
    playerState.rootNode = &rootNode;
    playerState.modeVariantNode = &modeVariantNode;
    InitDestroyedEffectEntry(&destroyedRespawn, &rootNode, &runtimeNode, "destroyed_respawn");
    destroyedRespawn.activationState = 2;
    playerState.destroyedRespawnFxEntry = &destroyedRespawn;
    playerState.statusMeterValue = 5.0f;
    playerState.damageProtectionActive = 1;
    playerState.queuedFixedDamageFlag = 1;
    playerState.damageVisualFlag = 1;
    playerState.timedHitStatus.runtimeFlags = 3;
    playerState.aiMode = 9;
    playerState.nextModeSwitchAllowedTime = 7.0f;
    playerState.motionInput = 2;
    playerState.autoTurnSign = -1;
    playerState.thirdPersonYawOffset = 4.0f;
    playerState.cameraElevationOffset = 5.0f;
    playerState.localVel = {1.0f, 2.0f, 3.0f};
    playerState.projectileSpawnVel = {4.0f, 5.0f, 6.0f};
    playerState.yawRotatedLocalVel = {7.0f, 8.0f, 9.0f};
    playerState.angVelPitch = 1.5f;
    playerState.angVelYaw = 2.5f;
    playerState.angVelRoll = 3.5f;
    playerState.steeringInput = 0.5f;
    playerState.throttleInput = 0.6f;
    playerState.subVerticalInput = 0.7f;

    shield.meter.ftable = &g_HudUiMeter_FTable;
    shield.meter.fillPixelsMax = 20;
    shield.meter.points[1].y = 100.0f;
    reinterpret_cast<HudUiPanel *>(&shield.percentTextPanel)->vtbl =
        &g_HudUiPanelSimple_FTable;
    g_HudUiMgrShieldMessageWidget = &shield;

    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        HudUiMessage &message = g_HudUiMgrMessages[bankIndex];
        message = {};
        message.base.ftable = &g_HudUiWidget_FTable;
        message.widget.ftable = &g_HudUiWidget_FTable;
        reinterpret_cast<HudUiPanel *>(&message.panel)->vtbl = &g_HudUiPanelSimple_FTable;
        message.variantImages[0] = &messageImages[bankIndex * 4 + 0];
        message.variantImages[1] = &messageImages[bankIndex * 4 + 1];
        message.variantImages[4] = &messageImages[bankIndex * 4 + 2];
        message.sideImageSwaps[0] = &messageImages[bankIndex * 4 + 2];
        message.sideImageSwaps[1] = &messageImages[bankIndex * 4 + 3];
    }

    g_HudUiMgrNanitePanel = {};
    g_HudUiMgrNanitePanel.base.ftable =
        reinterpret_cast<const HudUiCommon_FTable *>(&g_HudUiTripletPanel_FTable);
    for (int index = 0; index < 3; ++index) {
        g_HudUiMgrNanitePanel.items[index].ftable = &g_HudUiWidget_FTable;
    }

    for (int index = 1; index < 4; ++index) {
        g_HudUiMgrModeCounters[index] = {};
        reinterpret_cast<HudUiWidget *>(&g_HudUiMgrModeCounters[index])->ftable =
            reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCounter_FTable);
        TestFieldAt<zVidImagePartial *>(&g_HudUiMgrModeCounters[index], 0xbc) =
            &counterImages[(index - 1) * 2];
        TestFieldAt<zVidImagePartial *>(&g_HudUiMgrModeCounters[index], 0xc0) =
            &counterImages[(index - 1) * 2 + 1];
    }

    g_GameStateOrMapTable =
        reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&localSaveState);
    g_LocalPlayerSaveState = &localSaveState;
    g_GameNetStatus_AllowMaps = 1;
    g_GameNetSpawnPointHead = nullptr;
    g_GameNetSpawnPointTail = nullptr;
    g_GameNetSpawnPointCount = 0;
    g_HudUiMgrCurrentLayout = nullptr;
    g_HudUi_InvalidateMask = 0x80;
    std::strcpy(builtinTexturePack.filePath, "test.zbd");
    builtinTexturePack.fileHandle = std::tmpfile();
    g_zVid_BuiltinTexturePacks = &builtinTexturePack;
    g_zVid_BuiltinTexturePackCount = 1;
    g_zImage_TexDirEntryCount = 0;

    Player::DestroyedStateResetCallback(nullptr, &saveState, 0);

    zClass_Object3D_ModelRefLerpTask *const fadeTask = g_ModelRefLerpQueueState.head;
    zZbdSectionHandlerNode *const minesNode = sentinel.next;
    const bool effectOk =
        destroyedRespawn.activationState == 1 && (destroyedRespawn.flags & 0x40u) != 0;
    const bool damageOk = playerState.damageProtectionActive == 0 &&
                          playerState.queuedFixedDamageFlag == 0 &&
                          playerState.damageVisualFlag == 0;
    const bool objectOk =
        playerState.statusMeterValue == 120.0f && (rootData.flags & 0x02) != 0 &&
        rootData.alphaScale == 0.0f && g_ModelRefLerpQueueState.count == 1 &&
        fadeTask != nullptr && fadeTask->node == &rootNode && fadeTask->callbackCtx == &saveState &&
        fadeTask->onComplete == (void *)(&Player::DestroyedStateResetFinalizeCallback) &&
        fadeTask->currentModelRef == 0.0f && fadeTask->targetModelRef == 1.0f &&
        fadeTask->modelRefDeltaPerSec == 1.0f;
    const bool motionOk = playerState.aiMode == 0 &&
                          playerState.nextModeSwitchAllowedTime == 0.0f &&
                          playerState.motionInput == 0 && playerState.autoTurnSign == 0 &&
                          playerState.thirdPersonYawOffset == 0.0f &&
                          playerState.cameraElevationOffset == 0.0f &&
                          Vec3Equals(playerState.localVel, {0.0f, 0.0f, 0.0f}) &&
                          Vec3Equals(playerState.projectileSpawnVel, {0.0f, 0.0f, 0.0f}) &&
                          Vec3Equals(playerState.yawRotatedLocalVel, {0.0f, 0.0f, 0.0f}) &&
                          playerState.angVelPitch == 0.0f && playerState.angVelYaw == 0.0f &&
                          playerState.angVelRoll == 0.0f;
    const bool transitionOk = saveState.primaryModalState == &trackModal &&
                              playerState.currentMasterType == 1 &&
                              (modeVariantNode.flags & 0x04) != 0;
    const bool weaponOk =
        playerState.activeAltGunController == &playerState.altWeaponBanks[1].controllerA &&
        playerState.activePrimaryGunController == &playerState.altWeaponBanks[1].controllerA &&
        playerState.cachedAltSelectionCode == 100 &&
        playerState.cachedPrimarySelectionCode == 100 &&
        playerState.timedHitStatus.runtimeFlags == 0;
    const bool hudOk =
        std::strcmp(&TestFieldAt<char>(&shield.percentTextPanel, 0x34), "100") == 0;
    const bool zbdOk =
        minesNode != &sentinel && minesNode->sectionHandler.sectionName != nullptr &&
        std::strcmp(minesNode->sectionHandler.sectionName, "Mines") == 0;

    int result = 0;
    if (!effectOk) {
        result = 1;
    } else if (!damageOk) {
        result = 2;
    } else if (!objectOk) {
        result = 3;
    } else if (!motionOk) {
        result = 4;
    } else if (!transitionOk) {
        result = 5;
    } else if (!weaponOk) {
        result = 6;
    } else if (!hudOk) {
        result = 7;
    } else if (!zbdOk) {
        result = 8;
    }

    zClass_Object3D_ModelRefLerpQueue::Reset();
    ClearPlayerRegisteredHandlers(sentinel);
    g_HudSensorTracker.missionId = oldMissionId;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_zUtil_ZbdManager = oldZbdManager;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_LocalPlayerSaveState = oldLocalPlayerSaveState;
    g_GameNetStatus_AllowMaps = oldAllowMaps;
    g_GameNetSpawnPointHead = oldSpawnHead;
    g_GameNetSpawnPointTail = oldSpawnTail;
    g_GameNetSpawnPointCount = oldSpawnCount;
    g_HudUiMgrCurrentLayout = oldCurrentLayout;
    g_HudUiMgrShieldMessageWidget = oldShieldWidget;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    g_HudUiMgrActiveWeaponMessageIndex = oldActiveWeaponMessageIndex;
    g_HudUiMgrActiveWeaponSideIndex = oldActiveWeaponSideIndex;
    g_HudUiMgrNanitePanel = oldNanitePanel;
    g_zVid_BuiltinTexturePacks = oldBuiltinTexturePacks;
    g_zVid_BuiltinTexturePackCount = oldBuiltinTexturePackCount;
    g_zImage_TexDirEntryCount = oldTexDirEntryCount;
    for (int index = 0; index < 10; ++index) {
        g_HudUiMgrMessages[index] = oldMessages[index];
    }
    for (int index = 0; index < 4; ++index) {
        g_HudUiMgrModeCounters[index] = oldModeCounters[index];
    }
    return result;
}

extern "C" int player_enter_local_inactive_destroyed_lifecycle_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    const int oldQueueEnabled = g_zEffectAnim_RecordQueueEnabled;
    const int oldDispatchEnabled = g_zEffectAnim_DispatchEnabled;
    zEffectAnimActivationRecord *const oldRecordTable = g_zEffectAnim_ActivationRecordTable;
    const int oldRecordCapacity = g_zEffectAnim_ActivationRecordCapacity;
    const int oldRecordCount = g_zEffectAnim_ActivationRecordCount;

    zEffect_Anim::ClearActivationRecords();
    g_zEffectAnim_RecordQueueEnabled = 1;
    g_zEffectAnim_DispatchEnabled = 0;

    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState otherSaveState = {};
    zUtil_PlayerStateStorage playerState = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial destroyedRuntimeNode = {};
    zClass_NodePartial oldBubbleRuntimeNode = {};
    zEffectAnimEntry destroyedRespawn = {};
    zEffectAnimEntry oldBubble = {};
    saveState.playerState = &playerState;
    playerState.rootNode = &rootNode;
    playerState.destroyedRespawnFxEntry = &destroyedRespawn;
    rootNode.classId = 2;
    InitDestroyedEffectEntry(&destroyedRespawn, &rootNode, &destroyedRuntimeNode,
                             "destroyed_respawn");
    InitDestroyedEffectEntry(&oldBubble, &rootNode, &oldBubbleRuntimeNode, "bft_bubble");
    oldBubble.activationState = 2;
    playerState.masterTypeTransitionToSubLightHandle = &oldBubble;

    int networkEnabled = 0;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_GameStateOrMapTable =
        reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&otherSaveState);
    playerState.lifecycleState = 99;
    playerState.altGunTransitionState = 7;
    playerState.altGunTransitionController = &playerState.altWeaponBanks[2].controllerA;
    playerState.altGunTransitionTimerA = 3.0f;
    Player::EnterLocalInactiveDestroyedLifecycle(&saveState);
    const bool nonLocalOk = playerState.lifecycleState == 99 &&
                            playerState.altGunTransitionState == 7 &&
                            playerState.altGunTransitionController ==
                                &playerState.altWeaponBanks[2].controllerA &&
                            playerState.altGunTransitionTimerA == 3.0f &&
                            destroyedRespawn.activationState == 0 &&
                            playerState.masterTypeTransitionToSubLightHandle == &oldBubble;

    g_GameStateOrMapTable =
        reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);
    playerState.lifecycleState = 2;
    playerState.altGunTransitionState = 7;
    playerState.altGunTransitionController = &playerState.altWeaponBanks[3].controllerB;
    playerState.altGunTransitionTimerA = 4.0f;
    playerState.cameraTransitionTimer = 77;
    oldBubble.activationState = 2;
    playerState.masterTypeTransitionToSubLightHandle = &oldBubble;
    Player::EnterLocalInactiveDestroyedLifecycle(&saveState);
    const bool localOk =
        playerState.lifecycleState == 4 && playerState.altGunTransitionState == 1 &&
        playerState.altGunTransitionController == nullptr &&
        playerState.altGunTransitionTimerA == 0.0f &&
        playerState.masterTypeTransitionToSubLightHandle == nullptr &&
        destroyedRespawn.activationState == 2 && destroyedRespawn.velocityX == 0.0f &&
        destroyedRespawn.velocityY == 0.0f && destroyedRespawn.velocityZ == 0.0f &&
        playerState.cameraTransitionTimer == 77 && destroyedRespawn.eventCallback == nullptr &&
        destroyedRespawn.eventCallbackContext == nullptr;

    zEffect_Anim::ClearActivationRecords();
    InitDestroyedEffectEntry(&destroyedRespawn, &rootNode, &destroyedRuntimeNode,
                             "destroyed_respawn");
    oldBubble = {};
    InitDestroyedEffectEntry(&oldBubble, &rootNode, &oldBubbleRuntimeNode, "bft_bubble");
    oldBubble.activationState = 2;
    playerState.destroyedRespawnFxEntry = &destroyedRespawn;
    playerState.masterTypeTransitionToSubLightHandle = &oldBubble;
    playerState.cameraTransitionTimer = 0;
    networkEnabled = 1;
    Player::EnterLocalInactiveDestroyedLifecycle(&saveState);
    const bool networkOk =
        playerState.cameraTransitionTimer == 1 &&
        destroyedRespawn.eventCallback ==
            reinterpret_cast<zEffectAnimEventCallback>(&Player::DestroyedStateResetCallback) &&
        destroyedRespawn.eventCallbackContext == &saveState;

    zEffect_Anim::ClearActivationRecords();
    g_zEffectAnim_ActivationRecordTable = oldRecordTable;
    g_zEffectAnim_ActivationRecordCapacity = oldRecordCapacity;
    g_zEffectAnim_ActivationRecordCount = oldRecordCount;
    g_zEffectAnim_RecordQueueEnabled = oldQueueEnabled;
    g_zEffectAnim_DispatchEnabled = oldDispatchEnabled;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;

    if (!nonLocalOk) {
        return 1;
    }
    if (!localOk) {
        return 2;
    }
    return networkOk ? 0 : 3;
}

extern "C" int player_update_status_meter_smoke(void) {
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
    const std::uint32_t oldInvalidateMask = g_HudUi_InvalidateMask;
    g_HudUiMgrShieldMessageWidget = &shield;
    g_HudUiTopMessageStack = &topStack;
    g_HudUi_InvalidateMask = 0;

    playerState.statusMeterValue = 20.0f;
    g_PlayerStatusMeterRatio = 0.2f;
    const int addResult = Player::UpdateStatusMeter(&saveState, 1, 25.0f);
    const bool addOk =
        addResult == 1 && FloatNear(playerState.statusMeterValue, 45.0f) &&
        FloatNear(g_PlayerStatusMeterRatio, 0.45f) &&
        std::strcmp(&TestFieldAt<char>(&shield.percentTextPanel, 0x34), "45") == 0;

    playerState.statusMeterValue = 10.0f;
    playerState.damageProtectionActive = 1;
    playerState.queuedFixedDamageFlag = 1;
    playerState.damageVisualFlag = 1;
    g_PlayerStatusMeterRatio = 0.1f;
    const int refillResult = Player::UpdateStatusMeter(&saveState, 0, 0.0f);
    const bool refillOk =
        refillResult == 1 && playerState.statusMeterValue == 100.0f &&
        g_PlayerStatusMeterRatio == 1.0f && playerState.damageProtectionActive == 0 &&
        playerState.queuedFixedDamageFlag == 0 && playerState.damageVisualFlag == 0 &&
        std::strcmp(&TestFieldAt<char>(&shield.percentTextPanel, 0x34), "100") == 0;

    g_HudUiMgrShieldMessageWidget = oldShieldWidget;
    g_HudUiTopMessageStack = oldTopStack;
    g_PlayerStatusMeterRatio = oldStatusMeterRatio;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    return addOk && refillOk ? 0 : 1;
}

extern "C" int player_load_weapon_banks_and_select_defaults_smoke(void) {
    const int oldMissionId = g_HudSensorTracker.missionId;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    OptCatalogEntryDef *const oldEntryTable = g_OptCatalog_EntryTable;
    const int oldEntryCount = g_OptCatalog_EntryCount;
    zZbdManager *const oldZbdManager = g_zUtil_ZbdManager;

    int networkEnabled = 0;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_HudSensorTracker.missionId = 8;

    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager = MakePlayerZbdManager(sentinel);
    g_zUtil_ZbdManager = &manager;

    zClass_NodePartial rootNode = {};
    std::strcpy(rootNode.name, "local_vehicle");

    zUtil_PlayerStateStorage playerState = {};
    zUtil_SaveGameState saveState = {};
    PlayerMasterCommonData commonData = {};
    PlayerMasterWeaponSpec specA = {};
    PlayerMasterWeaponSpec specB = {};
    saveState.playerState = &playerState;
    playerState.masterCommonData = &commonData;
    playerState.rootNode = &rootNode;
    playerState.lifecycleState = 1;

    commonData.weaponSpecHead = &specA;
    commonData.weaponSpecTail = &specB;
    commonData.weaponSpecCount = 2;
    commonData.weaponNodeCount = 2;
    specA.next = &specB;

    std::strcpy(specA.optCatalogName, "WEP_2_0");
    specA.missionRequirementOrGateId = 7;
    specA.mountLayoutFlags = 0;
    specA.startAmmoOrCharge = 12.5f;
    specA.dispatchRepeatDelay = 1.25f;
    specA.aiAttackRangeMin = 2.5f;
    specA.aiAttackRangeMax = 9.5f;
    specA.fireSlotRecoilFlags = 1;
    specA.initialHardpointSelectState = 2;

    std::strcpy(specB.optCatalogName, "WEP_1_1");
    specB.missionRequirementOrGateId = 9;
    specB.startAmmoOrCharge = 33.0f;
    specB.dispatchRepeatDelay = 4.0f;
    specB.aiAttackRangeMin = 5.0f;
    specB.aiAttackRangeMax = 6.0f;

    OptCatalogEntryDef entries[2] = {};
    entries[0].keyName = const_cast<char *>("WEP_2_0");
    entries[0].displayName = const_cast<char *>("MountA");
    entries[1].keyName = const_cast<char *>("WEP_1_1");
    entries[1].displayName = const_cast<char *>("MountB");
    g_OptCatalog_EntryTable = entries;
    g_OptCatalog_EntryCount = 2;

    PlayerGunFireController &oldController = playerState.altWeaponBanks[5].controllerB;
    oldController.flags = 4;
    oldController.ammoOrCharge = 99.0f;
    oldController.attachNodePrimary =
        reinterpret_cast<zClass_NodePartial *>(static_cast<std::uintptr_t>(1));
    oldController.trailRuntimeState =
        static_cast<OptCatalogTrailRuntimeState *>(std::malloc(8));
    if (oldController.trailRuntimeState == nullptr) {
        return 7;
    }
    playerState.pendingAltCameraToggle = 1;
    playerState.timedHitStatus.runtimeFlags = 3;
    playerState.timedHitStatus.currentLevel = 0.5f;
    playerState.timedHitStatus.targetLevel = 1.0f;

    Player::LoadWeaponBanksAndSelectDefaults(&saveState);

    PlayerGunFireController &availableController = playerState.altWeaponBanks[2].controllerA;
    PlayerGunFireController &lockedController = playerState.altWeaponBanks[1].controllerB;
    zZbdSectionHandlerNode *const minesNode = sentinel.next;

    const bool resetOk =
        oldController.weaponBankIndex == 5 && oldController.weaponSideIndex == 1 &&
        (oldController.flags & 4) == 0 && oldController.ammoOrCharge == 0.0f &&
        oldController.attachNodePrimary == nullptr && oldController.trailRuntimeState == nullptr;
    const bool availableOk =
        availableController.optCatalogEntry == &entries[0] &&
        (availableController.flags & 4) != 0 && (availableController.flags & 1) != 0 &&
        availableController.ammoOrCharge == 12.5f &&
        availableController.dispatchRepeatDelay == 1.25f &&
        availableController.aiAttackRangeMin == 2.5f &&
        availableController.aiAttackRangeMax == 9.5f &&
        availableController.initialHardpointSelectState == 2;
    const bool lockedOk = lockedController.optCatalogEntry == &entries[1] &&
                          (lockedController.flags & 4) == 0 &&
                          lockedController.ammoOrCharge == 0.0f;
    const bool selectionOk =
        playerState.activeAltGunController == &availableController &&
        playerState.activePrimaryGunController == &playerState.altWeaponBanks[1].controllerA &&
        playerState.altHardpointSelectState == 2 &&
        playerState.cachedAltSelectionCode == 200 &&
        playerState.cachedPrimarySelectionCode == 100 &&
        playerState.primaryHardpointSelectState == 2;
    const bool finalStateOk =
        playerState.pendingAltCameraToggle == 0 &&
        playerState.timedHitStatus.runtimeFlags == 0 &&
        playerState.timedHitStatus.currentLevel == 0.0f &&
        playerState.timedHitStatus.targetLevel == 0.0f &&
        playerState.timedHitStatus.lightParentNode == &rootNode;
    const bool zbdOk =
        minesNode != &sentinel && minesNode->sectionHandler.sectionName != nullptr &&
        std::strcmp(minesNode->sectionHandler.sectionName, "Mines") == 0 &&
        minesNode->sectionHandler.onPreLoad != nullptr &&
        minesNode->sectionHandler.onDataReady != nullptr &&
        minesNode->sectionHandler.sortOrder == 1000 &&
        minesNode->sectionHandler.userData == nullptr;

    ClearPlayerRegisteredHandlers(sentinel);
    g_HudSensorTracker.missionId = oldMissionId;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_OptCatalog_EntryTable = oldEntryTable;
    g_OptCatalog_EntryCount = oldEntryCount;
    g_zUtil_ZbdManager = oldZbdManager;

    if (!resetOk) {
        return 1;
    }
    if (!availableOk) {
        return 2;
    }
    if (!lockedOk) {
        return 3;
    }
    if (!selectionOk) {
        return 4;
    }
    if (!finalStateOk) {
        return 5;
    }
    return zbdOk ? 0 : 6;
}

extern "C" int player_free_alt_weapon_trail_runtime_states_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    playerState.altWeaponBanks[0].controllerA.trailRuntimeState =
        reinterpret_cast<OptCatalogTrailRuntimeState *>(static_cast<std::uintptr_t>(1));
    playerState.altWeaponBanks[0].controllerB.trailRuntimeState =
        reinterpret_cast<OptCatalogTrailRuntimeState *>(static_cast<std::uintptr_t>(2));
    playerState.altWeaponBanks[1].controllerA.trailRuntimeState =
        static_cast<OptCatalogTrailRuntimeState *>(std::malloc(8));
    playerState.altWeaponBanks[4].controllerB.trailRuntimeState =
        static_cast<OptCatalogTrailRuntimeState *>(std::malloc(8));
    playerState.altWeaponBanks[9].controllerA.trailRuntimeState =
        static_cast<OptCatalogTrailRuntimeState *>(std::malloc(8));

    if (playerState.altWeaponBanks[1].controllerA.trailRuntimeState == nullptr ||
        playerState.altWeaponBanks[4].controllerB.trailRuntimeState == nullptr ||
        playerState.altWeaponBanks[9].controllerA.trailRuntimeState == nullptr) {
        return 1;
    }

    Player::FreeAltWeaponTrailRuntimeStates(&saveState);
    return playerState.altWeaponBanks[0].controllerA.trailRuntimeState ==
                       reinterpret_cast<OptCatalogTrailRuntimeState *>(
                           static_cast<std::uintptr_t>(1)) &&
                   playerState.altWeaponBanks[0].controllerB.trailRuntimeState ==
                       reinterpret_cast<OptCatalogTrailRuntimeState *>(
                           static_cast<std::uintptr_t>(2))
               ? 0
               : 2;
}

extern "C" int player_check_mission_weapon_availability_smoke(void) {
    const int oldMissionId = g_HudSensorTracker.missionId;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    int networkEnabled = 0;
    ZOPT_NETWORK_ENABLED = &networkEnabled;

    int available = -1;
    g_HudSensorTracker.missionId = 8;
    Player::CheckMissionWeaponAvailability(nullptr, 7, 0x61, &available);
    const bool singlePlayerUnlocked = available == 1;

    available = -1;
    Player::CheckMissionWeaponAvailability(nullptr, 9, 0x61, &available);
    const bool singlePlayerLockedByThreshold = available == 0;

    available = -1;
    Player::CheckMissionWeaponAvailability(nullptr, 0, 0x10, &available);
    const bool singlePlayerZeroThresholdLocked = available == 0;

    networkEnabled = 1;
    available = -1;
    Player::CheckMissionWeaponAvailability(nullptr, 0, 0x61, &available);
    const bool networkMission8LaserSabre = available == 1;

    available = -1;
    Player::CheckMissionWeaponAvailability(nullptr, 0, 0x31, &available);
    const bool networkMission8NapalmLocked = available == 0;

    g_HudSensorTracker.missionId = 11;
    available = -1;
    Player::CheckMissionWeaponAvailability(nullptr, 0, 0x80, &available);
    const bool networkMission11Missile = available == 1;

    g_HudSensorTracker.missionId = 5;
    available = -1;
    Player::CheckMissionWeaponAvailability(nullptr, 0, 0x10, &available);
    const bool earlyNetworkMissionLocked = available == 0;

    g_HudSensorTracker.missionId = oldMissionId;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    return singlePlayerUnlocked && singlePlayerLockedByThreshold &&
                   singlePlayerZeroThresholdLocked && networkMission8LaserSabre &&
                   networkMission8NapalmLocked && networkMission11Missile &&
                   earlyNetworkMissionLocked
               ? 0
               : 1;
}

extern "C" int player_apply_primary_weapon_switch_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerGunFireController previousController = {};
    PlayerGunFireController newController = {};
    zClass_NodePartial previousPrimary = {};
    zClass_NodePartial previousSecondary = {};
    zClass_NodePartial newPrimary = {};
    zClass_NodePartial newSecondary = {};

    saveState.playerState = &playerState;
    previousController.attachNodePrimary = &previousPrimary;
    previousController.attachNodeSecondary = &previousSecondary;
    newController.attachNodePrimary = &newPrimary;
    newController.attachNodeSecondary = &newSecondary;
    newController.weaponBankIndex = 6;
    newController.weaponSideIndex = 1;

    previousPrimary.classId = 5;
    previousSecondary.classId = 5;
    newPrimary.classId = 5;
    newSecondary.classId = 5;
    previousPrimary.flags = 0x04;
    previousSecondary.flags = 0x04;

    Player::ApplyPrimaryWeaponSwitch(&saveState, &previousController, &newController);
    const bool firstSwitchOk =
        playerState.activePrimaryGunController == &newController &&
        playerState.primaryHardpointSelectState == 2 &&
        playerState.cachedPrimarySelectionCode == 601 &&
        (previousPrimary.flags & 0x04) == 0 && (previousSecondary.flags & 0x04) == 0 &&
        (newPrimary.flags & 0x04) != 0 && (newSecondary.flags & 0x04) != 0;
    if (!firstSwitchOk) {
        return 1;
    }

    PlayerGunFireController nullSecondaryPrevious = {};
    PlayerGunFireController nextController = {};
    zClass_NodePartial nullPreviousPrimary = {};
    zClass_NodePartial nextPrimary = {};
    nullPreviousPrimary.classId = 5;
    nullPreviousPrimary.flags = 0x04;
    nextPrimary.classId = 5;
    nullSecondaryPrevious.attachNodePrimary = &nullPreviousPrimary;
    nextController.attachNodePrimary = &nextPrimary;
    nextController.weaponBankIndex = 2;
    nextController.weaponSideIndex = 0;

    Player::ApplyPrimaryWeaponSwitch(&saveState, &nullSecondaryPrevious, &nextController);
    return playerState.activePrimaryGunController == &nextController &&
                   playerState.cachedPrimarySelectionCode == 200 &&
                   (nullPreviousPrimary.flags & 0x04) == 0 && (nextPrimary.flags & 0x04) != 0
               ? 0
               : 2;
}

extern "C" int player_apply_alt_weapon_switch_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    void *const oldVolumeScalePtr = g_zSnd_GlobalVolumeScalePtr;
    const int oldInitialized = g_zSnd_IsInitialized;
    const int oldPreInitialized = g_zSnd_PreInitialized;
    const int oldBackend = g_zSnd_ActiveBackend;
    const int oldMuteDepth = g_zSnd_MuteDepth;
    const int oldPlaybackEnabled = g_zSnd_Flag10PlaybackEnabled;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    PlayerGunFireController initialController = {};
    initialController.weaponBankIndex = 2;
    initialController.weaponSideIndex = 1;
    playerState.altGunTransitionTimerA = 3.0f;
    playerState.altGunTransitionTimerB = 4.0f;

    Player::ApplyAltWeaponSwitch(&saveState, nullptr, &initialController);
    const bool initialOk =
        playerState.activeAltGunController == &initialController &&
        playerState.activeAltBankIndex == 2 &&
        playerState.altWeaponBanks[2].selectedSide == 1 &&
        playerState.altHardpointSelectState == 0 &&
        playerState.altGunTransitionTimerA == 0.0f &&
        playerState.altGunTransitionTimerB == 0.0f &&
        playerState.altGunTransitionState == 16 &&
        playerState.altGunTransitionController == &initialController &&
        playerState.cachedAltSelectionCode == 201;
    if (!initialOk) {
        return 1;
    }

    TestDirectSoundBufferVTable vtable = {};
    vtable.GetStatus = &TestDirectSoundGetStatus;
    vtable.Play = &TestDirectSoundPlay;
    vtable.SetCurrentPosition = &TestDirectSoundSetInt;
    vtable.SetVolume = &TestDirectSoundSetInt;
    TestDirectSoundBuffer directSoundBuffer = {&vtable};

    float globalVolume = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_MuteDepth = 0;
    g_zSnd_Flag10PlaybackEnabled = 1;
    g_PlayerTestPlayCount = 0;

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    sample.replayFields.gain = 0.5f;
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);

    PlayerMasterCommonData commonData = {};
    commonData.sfxWeaponUp[0] = &sample;
    playerState.masterCommonData = &commonData;

    PlayerGunFireController previousController = {};
    PlayerGunFireController nextController = {};
    previousController.weaponBankIndex = 2;
    previousController.weaponSideIndex = 1;
    nextController.weaponBankIndex = 4;
    nextController.weaponSideIndex = 0;

    OptCatalogEntryDef owner = {};
    OptCatalogTrailRuntimeState runtime = {};
    zClass_NodePartial trailNode = {};
    owner.activeTrailRuntime = &runtime;
    runtime.ownerEntry = &owner;
    runtime.activeNodeSlotCount = 1;
    runtime.activeNodeSlotCursor = 1;
    runtime.activeNodeSlots[0].node = &trailNode;
    trailNode.classId = 5;
    trailNode.flags = 0x04;
    previousController.trailRuntimeState = &runtime;

    playerState.altGunFireHeldFlag = 1;
    playerState.altGunTransitionTimerA = 5.0f;
    playerState.altGunTransitionTimerB = 6.0f;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);

    Player::ApplyAltWeaponSwitch(&saveState, &previousController, &nextController);

    const bool switchOk =
        playerState.activeAltGunController == &nextController &&
        playerState.activeAltBankIndex == 4 &&
        playerState.altWeaponBanks[4].selectedSide == 0 &&
        playerState.altHardpointSelectState == 0 &&
        playerState.altGunTransitionTimerA == 0.0f &&
        playerState.altGunTransitionTimerB == 0.0f &&
        playerState.altGunTransitionState == 4 &&
        playerState.altGunTransitionController == &previousController &&
        playerState.altGunFireHeldFlag == 0 &&
        playerState.cachedAltSelectionCode == 400 &&
        playerState.modeLoopSfxHandle[0] == &sample.primaryVoice &&
        sample.primaryVoice.ownerSample == &sample &&
        g_PlayerTestPlayCount == 1 &&
        owner.activeTrailRuntime == nullptr &&
        runtime.activeNodeSlotCursor == 0 &&
        (trailNode.flags & 0x04) == 0;

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_zSnd_GlobalVolumeScalePtr = oldVolumeScalePtr;
    g_zSnd_IsInitialized = oldInitialized;
    g_zSnd_PreInitialized = oldPreInitialized;
    g_zSnd_ActiveBackend = oldBackend;
    g_zSnd_MuteDepth = oldMuteDepth;
    g_zSnd_Flag10PlaybackEnabled = oldPlaybackEnabled;

    return switchOk ? 0 : 2;
}

extern "C" int player_reset_alt_gun_door_animation_state_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    zClass_NodePartial leftDoor = {};
    zClass_NodePartial rightDoor = {};
    zClass_Object3DDataPartial leftData = {};
    zClass_Object3DDataPartial rightData = {};
    leftDoor.classId = 5;
    leftDoor.classData = &leftData;
    rightDoor.classId = 5;
    rightDoor.classData = &rightData;
    leftData.scale = {0.25f, 0.5f, 0.75f};
    rightData.scale = {2.0f, 3.0f, 4.0f};
    playerState.doorLeftNode = &leftDoor;
    playerState.doorRightNode = &rightDoor;
    playerState.altGunTransitionTimerB = 6.0f;

    Player::ResetAltGunDoorAnimationState(&saveState);

    return playerState.altGunTransitionTimerB == 0.0f && leftData.scale.x == 1.0f &&
                   leftData.scale.y == 1.0f && leftData.scale.z == 1.0f &&
                   rightData.scale.x == 1.0f && rightData.scale.y == 1.0f &&
                   rightData.scale.z == 1.0f
               ? 0
               : 1;
}

extern "C" int player_reset_alt_gun_runtime_state_smoke(void) {
    void *const oldFreeRuntimeList = g_OptCatalogFreeRuntimeInstanceList;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    PlayerGunFireController *const activeController =
        &playerState.altWeaponBanks[2].controllerA;
    playerState.activeAltGunController = activeController;
    playerState.altGunFireHeldFlag = 1;
    playerState.altGunTransitionState = 7;
    playerState.altGunTransitionTimerA = 3.0f;
    playerState.altGunTransitionTimerB = 4.0f;
    playerState.altGunTransitionController = activeController;

    zClass_NodePartial mountNode = {};
    zClass_Object3DDataPartial mountData = {};
    mountNode.classId = 5;
    mountNode.classData = &mountData;
    mountNode.flags = 0x04;
    mountData.scale = {0.25f, 0.5f, 0.75f};
    activeController->attachNodePrimary = &mountNode;
    activeController->attachPosX = 7.0f;
    activeController->attachPosY = 8.0f;
    activeController->attachPosZ = 9.0f;

    OptCatalogEntryDef owner = {};
    OptCatalogTrailRuntimeState trailRuntime = {};
    zClass_NodePartial trailNode = {};
    owner.activeTrailRuntime = &trailRuntime;
    trailRuntime.ownerEntry = &owner;
    trailRuntime.activeNodeSlotCount = 1;
    trailRuntime.activeNodeSlotCursor = 1;
    trailRuntime.activeNodeSlots[0].node = &trailNode;
    trailNode.classId = 5;
    trailNode.flags = 0x04;
    activeController->trailRuntimeState = &trailRuntime;
    activeController->optCatalogEntry = &owner;

    OptCatalogRuntimeInstanceStorage runtime = {};
    zClass_NodeFreeListSlot projectileSlot = {};
    zClass_Object3DDataPartial projectileData = {};
    projectileSlot.node.classId = 5;
    projectileSlot.node.classData = &projectileData;
    projectileSlot.damageHandler = &owner;
    runtime.projectileNode = &projectileSlot.node;
    runtime.lifetime = 0.0f;
    activeController->attachState = &runtime;
    zClass_Class::AddChild(&mountNode, &projectileSlot.node);

    zClass_NodePartial doorLeft = {};
    zClass_Object3DDataPartial doorLeftData = {};
    doorLeft.classId = 5;
    doorLeft.classData = &doorLeftData;
    doorLeftData.scale = {0.3f, 0.4f, 0.5f};
    playerState.doorLeftNode = &doorLeft;

    zClass_NodePartial bank9Node = {};
    zClass_Object3DDataPartial bank9Data = {};
    bank9Node.classId = 5;
    bank9Node.classData = &bank9Data;
    bank9Node.flags = 0x04;
    bank9Data.scale = {2.0f, 2.0f, 2.0f};
    PlayerGunFireController &bank9Controller = playerState.altWeaponBanks[9].controllerB;
    bank9Controller.attachNodePrimary = &bank9Node;
    bank9Controller.attachPosX = 1.0f;
    bank9Controller.attachPosY = 2.0f;
    bank9Controller.attachPosZ = 3.0f;

    OptCatalogRuntimeInstanceStorage freeSentinel = {};
    g_OptCatalogFreeRuntimeInstanceList = &freeSentinel;

    Player::ResetAltGunRuntimeState(&saveState);

    const bool cleanupOk =
        playerState.altGunFireHeldFlag == 0 && owner.activeTrailRuntime == nullptr &&
        trailRuntime.activeNodeSlotCursor == 0 && (trailNode.flags & 0x04) == 0 &&
        activeController->attachState == nullptr &&
        g_OptCatalogFreeRuntimeInstanceList == &runtime && runtime.next == &freeSentinel &&
        projectileSlot.damageHandler == nullptr && mountNode.listCountB == 0 &&
        projectileSlot.node.listCountA == 0;
    const bool stateOk =
        playerState.altGunTransitionState == 1 &&
        playerState.altGunTransitionController == nullptr &&
        playerState.altGunTransitionTimerA == 0.0f &&
        playerState.altGunTransitionTimerB == 0.0f;
    const bool attachResetOk =
        (mountNode.flags & 0x04) == 0 && mountData.scale.x == 1.0f &&
        mountData.scale.y == 1.0f && mountData.scale.z == 1.0f &&
        mountData.localMatrix[9] == 7.0f && mountData.localMatrix[10] == 8.0f &&
        mountData.localMatrix[11] == 9.0f && (bank9Node.flags & 0x04) == 0 &&
        bank9Data.scale.x == 1.0f && bank9Data.localMatrix[9] == 1.0f &&
        bank9Data.localMatrix[10] == 2.0f && bank9Data.localMatrix[11] == 3.0f &&
        doorLeftData.scale.x == 1.0f && doorLeftData.scale.y == 1.0f &&
        doorLeftData.scale.z == 1.0f;

    g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
    return cleanupOk && stateOk && attachResetOk ? 0 : 1;
}

extern "C" int player_handle_alt_weapon_bank_select_smoke(void) {
    zUtil_SaveGameState *const oldLocalSaveState = g_LocalPlayerSaveState;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    HudUiTextStack4 *const oldTopMessageStack = g_HudUiTopMessageStack;
    void *const oldVolumeScalePtr = g_zSnd_GlobalVolumeScalePtr;
    const int oldInitialized = g_zSnd_IsInitialized;
    const int oldPreInitialized = g_zSnd_PreInitialized;
    const int oldBackend = g_zSnd_ActiveBackend;
    const int oldMuteDepth = g_zSnd_MuteDepth;
    const int oldPlaybackEnabled = g_zSnd_Flag10PlaybackEnabled;

    TestDirectSoundBufferVTable vtable = {};
    vtable.GetStatus = &TestDirectSoundGetStatus;
    vtable.Play = &TestDirectSoundPlay;
    vtable.SetCurrentPosition = &TestDirectSoundSetInt;
    vtable.SetVolume = &TestDirectSoundSetInt;
    TestDirectSoundBuffer directSoundBuffer = {&vtable};

    float globalVolume = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_MuteDepth = 0;
    g_zSnd_Flag10PlaybackEnabled = 1;
    g_PlayerTestPlayCount = 0;

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    sample.replayFields.gain = 0.5f;
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);

    PlayerMasterCommonData commonData = {};
    commonData.sfxWeaponUp[0] = &sample;
    PlayerMasterModalData modalData = {};
    modalData.masterType = 1;
    PlayerModalState modalState = {};
    modalState.masterModalData = &modalData;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    playerState.masterCommonData = &commonData;
    playerState.altGunTransitionState = 1;
    playerState.activeAltBankIndex = 2;

    OptCatalogEntryDef entryA = {};
    entryA.description = const_cast<char *>("alt-a");
    OptCatalogEntryDef entryB = {};
    entryB.description = const_cast<char *>("alt-b");
    PlayerAltWeaponBank &activeBank = playerState.altWeaponBanks[2];
    activeBank.controllerA.optCatalogEntry = &entryA;
    activeBank.controllerA.weaponBankIndex = 2;
    activeBank.controllerA.weaponSideIndex = 0;
    activeBank.controllerA.flags = 4;
    activeBank.controllerA.ammoOrCharge = 3.0f;
    playerState.activeAltGunController = &activeBank.controllerA;

    PlayerAltWeaponBank &targetBank = playerState.altWeaponBanks[3];
    targetBank.selectedSide = 1;
    targetBank.controllerA.optCatalogEntry = &entryA;
    targetBank.controllerA.weaponBankIndex = 3;
    targetBank.controllerA.weaponSideIndex = 0;
    targetBank.controllerA.flags = 4;
    targetBank.controllerA.ammoOrCharge = 0.0f;
    targetBank.controllerB.optCatalogEntry = &entryB;
    targetBank.controllerB.weaponBankIndex = 3;
    targetBank.controllerB.weaponSideIndex = 1;
    targetBank.controllerB.flags = 4;
    targetBank.controllerB.ammoOrCharge = 5.0f;

    HudUiTopMessageStack topStack = {};
    topStack.Constructor();
    topStack.base.enabled = 0;
    g_HudUiTopMessageStack = &topStack;

    HudUiMessage &message = g_HudUiMgrMessages[3];
    message = {};
    message.base.ftable = &g_HudUiWidget_FTable;
    message.widget.ftable = &g_HudUiWidget_FTable;
    reinterpret_cast<HudUiPanel *>(&message.panel)->vtbl = &g_HudUiPanelSimple_FTable;
    HudUiMessage &previousMessage = g_HudUiMgrMessages[0];
    previousMessage = {};
    previousMessage.base.ftable = &g_HudUiWidget_FTable;
    previousMessage.widget.ftable = &g_HudUiWidget_FTable;
    reinterpret_cast<HudUiPanel *>(&previousMessage.panel)->vtbl = &g_HudUiPanelSimple_FTable;
    zVidImagePartial images[5] = {};
    message.variantImages[3] = &images[3];
    message.variantImages[4] = &images[4];
    TestFieldAt<int>(&message.panel, 0x2a4) = 1;

    g_LocalPlayerSaveState = &saveState;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);

    Player::HandleAltWeaponBankSelectInput(0x11);
    int result = 0;
    if (playerState.activeAltGunController != &targetBank.controllerB) {
        result = 1;
    } else if (playerState.activeAltBankIndex != 3) {
        result = 2;
    } else if (targetBank.selectedSide != 1) {
        result = 3;
    } else if (playerState.cachedAltSelectionCode != 301) {
        result = 4;
    } else if (playerState.modeLoopSfxHandle[0] != &sample.primaryVoice) {
        result = 5;
    } else if (g_PlayerTestPlayCount != 1) {
        result = 6;
    } else if (g_HudUiMgrMessages[3].base.image != &images[4]) {
        result = 7;
    } else if (std::strcmp(&TestFieldAt<char>(&message.panel, 0x34), "5") != 0) {
        result = 8;
    }

    playerState.altGunTransitionState = 1;
    Player::HandleAltWeaponBankSelectInput(0x11);
    const bool failureOk =
        playerState.activeAltGunController == &targetBank.controllerB &&
        targetBank.selectedSide == 1 &&
        g_PlayerTestPlayCount == 1;

    if (result == 0 && !failureOk) {
        result = 9;
    }

    g_LocalPlayerSaveState = oldLocalSaveState;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_HudUiTopMessageStack = oldTopMessageStack;
    g_zSnd_GlobalVolumeScalePtr = oldVolumeScalePtr;
    g_zSnd_IsInitialized = oldInitialized;
    g_zSnd_PreInitialized = oldPreInitialized;
    g_zSnd_ActiveBackend = oldBackend;
    g_zSnd_MuteDepth = oldMuteDepth;
    g_zSnd_Flag10PlaybackEnabled = oldPlaybackEnabled;
    g_HudUiMgrMessages[0] = {};
    g_HudUiMgrMessages[3] = {};

    return result;
}

extern "C" int player_handle_primary_weapon_variant_toggle_smoke(void) {
    zUtil_SaveGameState *const oldLocalSaveState = g_LocalPlayerSaveState;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    HudUiTextStack4 *const oldTopMessageStack = g_HudUiTopMessageStack;
    void *const oldVolumeScalePtr = g_zSnd_GlobalVolumeScalePtr;
    const int oldInitialized = g_zSnd_IsInitialized;
    const int oldPreInitialized = g_zSnd_PreInitialized;
    const int oldBackend = g_zSnd_ActiveBackend;
    const int oldMuteDepth = g_zSnd_MuteDepth;
    const int oldPlaybackEnabled = g_zSnd_Flag10PlaybackEnabled;

    TestDirectSoundBufferVTable vtable = {};
    vtable.GetStatus = &TestDirectSoundGetStatus;
    vtable.Play = &TestDirectSoundPlay;
    vtable.SetCurrentPosition = &TestDirectSoundSetInt;
    vtable.SetVolume = &TestDirectSoundSetInt;
    TestDirectSoundBuffer directSoundBuffer = {&vtable};

    float globalVolume = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_MuteDepth = 0;
    g_zSnd_Flag10PlaybackEnabled = 1;
    g_PlayerTestPlayCount = 0;

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    sample.replayFields.gain = 0.5f;
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);

    PlayerMasterCommonData commonData = {};
    commonData.sfxWeaponUp[2] = &sample;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;
    playerState.masterCommonData = &commonData;

    OptCatalogEntryDef entryA = {};
    entryA.description = const_cast<char *>("primary-a");
    OptCatalogEntryDef entryB = {};
    entryB.description = const_cast<char *>("primary-b");

    PlayerAltWeaponBank &bank = playerState.altWeaponBanks[1];
    bank.controllerA.optCatalogEntry = &entryA;
    bank.controllerA.weaponBankIndex = 1;
    bank.controllerA.weaponSideIndex = 0;
    bank.controllerA.flags = 0;
    bank.controllerA.ammoOrCharge = 0.0f;
    bank.controllerB.optCatalogEntry = &entryB;
    bank.controllerB.weaponBankIndex = 1;
    bank.controllerB.weaponSideIndex = 1;

    zClass_NodePartial nodeA = {};
    zClass_NodePartial nodeB = {};
    nodeA.classId = 5;
    nodeA.flags = 0x04;
    nodeB.classId = 5;
    bank.controllerA.attachNodePrimary = &nodeA;
    bank.controllerB.attachNodePrimary = &nodeB;

    HudUiTopMessageStack topStack = {};
    topStack.Constructor();
    topStack.base.enabled = 0;
    g_HudUiTopMessageStack = &topStack;

    HudUiMessage &message = g_HudUiMgrMessages[1];
    message = {};
    message.base.ftable = &g_HudUiWidget_FTable;
    message.widget.ftable = &g_HudUiWidget_FTable;
    reinterpret_cast<HudUiPanel *>(&message.panel)->vtbl = &g_HudUiPanelSimple_FTable;
    zVidImagePartial images[5] = {};
    message.variantImages[3] = &images[3];
    message.variantImages[4] = &images[4];

    g_LocalPlayerSaveState = &saveState;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);

    auto cleanup = [&]() {
        g_LocalPlayerSaveState = oldLocalSaveState;
        g_GameStateOrMapTable = oldGameStateOrMapTable;
        g_HudUiTopMessageStack = oldTopMessageStack;
        g_zSnd_GlobalVolumeScalePtr = oldVolumeScalePtr;
        g_zSnd_IsInitialized = oldInitialized;
        g_zSnd_PreInitialized = oldPreInitialized;
        g_zSnd_ActiveBackend = oldBackend;
        g_zSnd_MuteDepth = oldMuteDepth;
        g_zSnd_Flag10PlaybackEnabled = oldPlaybackEnabled;
        g_HudUiMgrMessages[1] = {};
    };

    playerState.activePrimaryGunController = &bank.controllerA;
    bank.controllerB.flags = 0;
    bank.controllerB.ammoOrCharge = 7.0f;
    Player::HandlePrimaryWeaponVariantToggleInput(123);
    if (playerState.activePrimaryGunController != &bank.controllerA ||
        playerState.modeLoopSfxHandle[2] != nullptr || g_PlayerTestPlayCount != 0) {
        cleanup();
        return 1;
    }

    bank.controllerB.flags = 4;
    bank.controllerB.ammoOrCharge = 0.0f;
    Player::HandlePrimaryWeaponVariantToggleInput(456);
    if (playerState.activePrimaryGunController != &bank.controllerA ||
        playerState.modeLoopSfxHandle[2] != nullptr || g_PlayerTestPlayCount != 0) {
        cleanup();
        return 2;
    }

    bank.controllerB.ammoOrCharge = 7.0f;
    TestFieldAt<int>(&message.panel, 0x2a4) = 1;
    Player::HandlePrimaryWeaponVariantToggleInput(789);
    const bool sideBOk =
        playerState.activePrimaryGunController == &bank.controllerB &&
        playerState.cachedPrimarySelectionCode == 101 &&
        playerState.primaryHardpointSelectState == 2 &&
        playerState.modeLoopSfxHandle[2] == &sample.primaryVoice &&
        sample.primaryVoice.ownerSample == &sample &&
        g_HudUiMgrMessages[1].base.image == &images[4] &&
        std::strcmp(&TestFieldAt<char>(&message.panel, 0x34), "7") == 0 &&
        (nodeA.flags & 0x04) == 0 && (nodeB.flags & 0x04) != 0 &&
        g_PlayerTestPlayCount == 1;
    if (!sideBOk) {
        cleanup();
        return 3;
    }

    TestFieldAt<int>(&message.panel, 0x2a4) = 0;
    Player::HandlePrimaryWeaponVariantToggleInput(321);
    const bool sideAOk =
        playerState.activePrimaryGunController == &bank.controllerA &&
        playerState.cachedPrimarySelectionCode == 100 &&
        g_HudUiMgrMessages[1].base.image == &images[3] &&
        std::strcmp(&TestFieldAt<char>(&message.panel, 0x34), "0") == 0 &&
        (nodeA.flags & 0x04) != 0 && (nodeB.flags & 0x04) == 0 &&
        g_PlayerTestPlayCount == 2;

    cleanup();
    return sideAOk ? 0 : 4;
}

extern "C" int player_update_third_person_camera_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    zClass_NodePartial cameraNode = {};
    zClass_CameraDataPartial cameraData = {};
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    cameraData.cameraFlags = 2;

    zClass_NodePartial horizonNode = {};
    zClass_Object3DDataPartial horizonData = {};
    horizonNode.classId = 6;
    horizonNode.classData = &horizonData;

    playerState.worldPos = {10.0f, 20.0f, 30.0f};
    playerState.cameraLerpStart = {1.0f, 2.0f, 3.0f};
    playerState.cameraYOffset = 5.0f;
    playerState.autoTurnTargetWorldPos = {11.0f, 22.0f, 43.0f};

    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    zClass_NodePartial *const oldHorizonNode = g_Player_HorizonNode;
    g_MainCamera = &cameraNode;
    g_Player_HorizonNode = &horizonNode;

    Player::UpdateThirdPersonCamera(&saveState);

    const float expectedPitch =
        static_cast<float>(std::atan2(3.0, std::sqrt(10.0)));
    const float expectedYaw = static_cast<float>(std::atan2(1.0, 3.0));
    const bool ok =
        Vec3Equals(playerState.cameraTarget, {11.0f, 22.0f, 33.0f}) &&
        Vec3Equals(cameraData.worldTarget, {11.0f, 22.0f, 33.0f}) &&
        FloatNear(cameraData.posOffset.x, expectedPitch) &&
        FloatNear(cameraData.posOffset.y, expectedYaw) && cameraData.posOffset.z == 0.0f &&
        horizonData.localMatrix[9] == 11.0f && horizonData.localMatrix[10] == 22.0f &&
        horizonData.localMatrix[11] == 33.0f &&
        Vec3Equals(playerState.cameraDirNext, {0.0f, 0.0f, 1.0f}) &&
        Vec3Equals(playerState.cameraDir, {0.0f, 0.0f, 1.0f}) &&
        Vec3Equals(playerState.cameraDirFlat, {0.0f, 0.0f, 1.0f});

    g_MainCamera = oldMainCamera;
    g_Player_HorizonNode = oldHorizonNode;
    return ok ? 0 : 1;
}

extern "C" int player_apply_camera_state_and_zopt_set_camera_mode_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    int *const oldGameControlOptions = ZOPT_GAME_CONTROL_OPTIONS;
    const int oldSavedSteeringMode = g_Player_SavedSteeringMode;

    int gameControlOptions = 0;
    g_GameStateOrMapTable =
        reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);
    ZOPT_GAME_CONTROL_OPTIONS = &gameControlOptions;

    g_Player_SavedSteeringMode = 1;
    playerState.cameraState = 3;
    playerState.cameraDistance = 77.0f;
    playerState.cameraTargetDistance = 12.0f;
    playerState.cameraLerpActive = 5;
    zOpt::SetCameraMode(1);

    bool ok = (gameControlOptions & 0x08) != 0 && zOpt::GetSteeringMode() == 1 &&
              playerState.cameraState == 1 && playerState.previousCameraState == 3 &&
              playerState.cameraTargetDistance == 77.0f &&
              playerState.cameraLerpActive == 0;

    gameControlOptions = 0x0a;
    playerState.cameraState = 1;
    playerState.previousCameraState = 3;
    playerState.cameraElevationOffset = 9.0f;
    zOpt::SetCameraMode(0);

    ok = ok && (gameControlOptions & 0x08) == 0 && zOpt::GetSteeringMode() == 0 &&
         playerState.cameraState == 3 && playerState.previousCameraState == 1 &&
         playerState.cameraElevationOffset == 0.0f && g_Player_SavedSteeringMode == 1;

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    ZOPT_GAME_CONTROL_OPTIONS = oldGameControlOptions;
    g_Player_SavedSteeringMode = oldSavedSteeringMode;
    return ok ? 0 : 1;
}

extern "C" int player_set_world_pose_and_restart_anchor_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    playerState.worldPos = {1.0f, 2.0f, 3.0f};
    playerState.previousTransform.posX = 4.0f;
    playerState.previousTransform.posY = 5.0f;
    playerState.previousTransform.posZ = 6.0f;
    playerState.restartYawRad = 7.0f;
    playerState.variantTag.count = 3;
    playerState.variantTag.tags[0] = 10;
    playerState.variantTag.tags[1] = 11;
    playerState.variantTag.tags[2] = 12;

    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    g_VariantTag_Current.count = 2;
    g_VariantTag_Current.tags[0] = 20;
    g_VariantTag_Current.tags[1] = 21;
    g_VariantTag_Current.tags[2] = 22;
    g_Variant_CurrentTag.count = 1;
    g_Variant_CurrentTag.tags[0] = 30;
    g_Variant_CurrentTag.tags[1] = 31;
    g_Variant_CurrentTag.tags[2] = 32;

    Player::SetWorldPoseAndRestartAnchor(nullptr, nullptr, 9.0f);
    if (!Vec3Equals(playerState.worldPos, {1.0f, 2.0f, 3.0f}) ||
        playerState.restartYawRad != 7.0f || g_VariantTag_Current.count != 2 ||
        g_Variant_CurrentTag.count != 1) {
        g_VariantTag_Current = oldVariantTagCurrent;
        g_Variant_CurrentTag = oldVariantCurrent;
        return 1;
    }

    const zVec3 newPosition = {40.0f, 41.0f, 42.0f};
    Player::SetWorldPoseAndRestartAnchor(&saveState, &newPosition, 1.25f);

    const bool ok = Vec3Equals(playerState.worldPos, newPosition) &&
                    playerState.previousTransform.posX == newPosition.x &&
                    playerState.previousTransform.posY == newPosition.y &&
                    playerState.previousTransform.posZ == newPosition.z &&
                    playerState.restartYawRad == 1.25f &&
                    g_VariantTag_Current.count == 0 &&
                    g_VariantTag_Current.tags[0] == 0xff &&
                    g_VariantTag_Current.tags[1] == 0xff &&
                    g_VariantTag_Current.tags[2] == 0xff &&
                    g_Variant_CurrentTag.count == 0 &&
                    g_Variant_CurrentTag.tags[0] == 0xff &&
                    g_Variant_CurrentTag.tags[1] == 0xff &&
                    g_Variant_CurrentTag.tags[2] == 0xff &&
                    playerState.variantTag.count == 0 &&
                    playerState.variantTag.tags[0] == 0xff &&
                    playerState.variantTag.tags[1] == 0xff &&
                    playerState.variantTag.tags[2] == 0xff;

    g_VariantTag_Current = oldVariantTagCurrent;
    g_Variant_CurrentTag = oldVariantCurrent;
    return ok ? 0 : 2;
}

extern "C" int player_reset_mouse_control_state_and_recenter_cursor_smoke(void) {
    HWND const oldWindow = g_zInput_hWnd;
    const int oldCenterX = g_zInput_MouseClientCenterX;
    const int oldCenterY = g_zInput_MouseClientCenterY;
    const zInput::MouseStateSnapshot oldMouseState = g_zInput_MouseStateSnapshot;
    POINT originalCursor = {};
    GetCursorPos(&originalCursor);

    HWND const hwnd = CreateWindowExA(0, "STATIC", "recoil", WS_POPUP, 30, 40, 200, 120,
                                      nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
    if (hwnd == nullptr) {
        return 1;
    }

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;
    playerState.thirdPersonYawOffset = 11.0f;
    playerState.cameraElevationOffset = -3.0f;
    playerState.cameraYOffset = 5.0f;

    g_zInput_hWnd = hwnd;
    g_zInput_MouseClientCenterX = 64;
    g_zInput_MouseClientCenterY = 48;
    g_zInput_MouseStateSnapshot.cursorClientX = 7;
    g_zInput_MouseStateSnapshot.cursorClientY = 9;
    g_zInput_MouseStateSnapshot.cursorNormX = 0.25f;
    g_zInput_MouseStateSnapshot.cursorNormY = -0.5f;

    Player::ResetMouseControlStateAndRecenterCursor(&saveState);

    const bool resetOk = playerState.thirdPersonYawOffset == 0.0f &&
                         playerState.cameraElevationOffset == 0.0f &&
                         playerState.cameraYOffset == 5.0f &&
                         g_zInput_MouseStateSnapshot.cursorClientX == 64 &&
                         g_zInput_MouseStateSnapshot.cursorClientY == 48 &&
                         g_zInput_MouseStateSnapshot.cursorNormX == 0.0f &&
                         g_zInput_MouseStateSnapshot.cursorNormY == 0.0f;

    DestroyWindow(hwnd);
    SetCursorPos(originalCursor.x, originalCursor.y);
    g_zInput_hWnd = oldWindow;
    g_zInput_MouseClientCenterX = oldCenterX;
    g_zInput_MouseClientCenterY = oldCenterY;
    g_zInput_MouseStateSnapshot = oldMouseState;

    return resetOk ? 0 : 2;
}

extern "C" int player_reset_motion_transient_state_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    playerState.throttleInput = 1.0f;
    playerState.steeringInput = 2.0f;
    playerState.subVerticalInput = 3.0f;
    playerState.subPitchInput = 4.0f;
    playerState.joyCameraYawInput = 5.0f;
    playerState.throttleInputCopy = 6.0f;
    playerState.steeringInputCopy = 7.0f;
    playerState.subVerticalInputCopy = 8.0f;
    playerState.subPitchInputCopy = 9.0f;
    playerState.angVelPitch = 10.0f;
    playerState.angVelYaw = 11.0f;
    playerState.angVelRoll = 12.0f;
    playerState.projectileSpawnVel = {13.0f, 14.0f, 15.0f};
    playerState.localVel = {16.0f, 17.0f, 18.0f};
    playerState.yawRotatedLocalVel = {19.0f, 20.0f, 21.0f};

    Player::ResetMotionTransientState(&saveState);

    return playerState.throttleInput == 0.0f && playerState.steeringInput == 0.0f &&
                   playerState.subVerticalInput == 0.0f && playerState.subPitchInput == 4.0f &&
                   playerState.joyCameraYawInput == 5.0f &&
                   playerState.throttleInputCopy == 0.0f &&
                   playerState.steeringInputCopy == 0.0f &&
                   playerState.subVerticalInputCopy == 0.0f &&
                   playerState.subPitchInputCopy == 9.0f &&
                   playerState.angVelPitch == 0.0f && playerState.angVelYaw == 0.0f &&
                   playerState.angVelRoll == 0.0f &&
                   Vec3Equals(playerState.projectileSpawnVel, {0.0f, 0.0f, 0.0f}) &&
                   Vec3Equals(playerState.localVel, {0.0f, 0.0f, 0.0f}) &&
                   Vec3Equals(playerState.yawRotatedLocalVel, {0.0f, 0.0f, 0.0f})
               ? 0
               : 1;
}

extern "C" int player_is_mission_probe_type1_enabled_by_id_smoke(void) {
    const bool enabledOk = Player::IsMissionProbeType1EnabledById(9) == 1 &&
                           Player::IsMissionProbeType1EnabledById(11) == 1 &&
                           Player::IsMissionProbeType1EnabledById(12) == 1 &&
                           Player::IsMissionProbeType1EnabledById(13) == 1;
    const bool disabledOk = Player::IsMissionProbeType1EnabledById(8) == 0 &&
                            Player::IsMissionProbeType1EnabledById(10) == 0 &&
                            Player::IsMissionProbeType1EnabledById(14) == 0 &&
                            Player::IsMissionProbeType1EnabledById(-1) == 0;

    return enabledOk && disabledOk ? 0 : 1;
}

extern "C" int player_update_bank_velocity_from_steer_input_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    modalData.accelRate = 6.0f;
    g_Player_DeltaTime = 0.25f;

    playerState.restartYawRad = 2.0f;
    playerState.steeringInput = 1.0f;
    playerState.steeringInputCopy = 0.5f;
    playerState.localVel.x = -3.0f;
    Player::UpdateBankVelocityFromSteerInput(&saveState);
    if (playerState.restartYawRad != 0.0f || playerState.localVel.x != -3.75f) {
        return 1;
    }

    playerState.steeringInputCopy = 0.5f;
    playerState.localVel.x = 2.0f;
    Player::UpdateBankVelocityFromSteerInput(&saveState);
    if (playerState.localVel.x != -0.75f) {
        return 2;
    }

    playerState.steeringInputCopy = -0.5f;
    playerState.localVel.x = -2.0f;
    Player::UpdateBankVelocityFromSteerInput(&saveState);
    if (playerState.localVel.x != 0.75f) {
        return 3;
    }

    playerState.steeringInput = 0.0f;
    playerState.steeringInputCopy = -0.5f;
    playerState.localVel.x = 4.0f;
    playerState.restartYawRad = 8.0f;
    Player::UpdateBankVelocityFromSteerInput(&saveState);
    return playerState.restartYawRad == 0.0f && playerState.localVel.x == 0.0f ? 0 : 4;
}

extern "C" int player_update_auto_turn_and_steer_from_target_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    modalData.yawAccel = 4.0f;
    modalData.yawDamping = 3.0f;
    playerState.yawVelocityLimit = 1.25f;
    g_Player_DeltaTime = 0.5f;

    playerState.steeringInput = 1.0f;
    playerState.steeringInputCopy = 0.25f;
    playerState.angVelYaw = -2.0f;
    Player::UpdateAutoTurnAndSteerFromTarget(&saveState);
    if (playerState.angVelYaw != 0.5f) {
        return 1;
    }

    playerState.steeringInputCopy = 0.25f;
    playerState.angVelYaw = 1.0f;
    Player::UpdateAutoTurnAndSteerFromTarget(&saveState);
    if (playerState.angVelYaw != 1.25f) {
        return 2;
    }

    playerState.steeringInputCopy = -0.25f;
    playerState.angVelYaw = -1.0f;
    Player::UpdateAutoTurnAndSteerFromTarget(&saveState);
    if (playerState.angVelYaw != -1.25f) {
        return 3;
    }

    playerState.steeringInput = 0.0f;
    playerState.angVelYaw = 2.0f;
    g_Player_DeltaTime = 0.0f;
    Player::UpdateAutoTurnAndSteerFromTarget(&saveState);
    return playerState.angVelYaw == 2.0f ? 0 : 4;
}

extern "C" int player_integrate_yaw_and_wrap_from_yaw_velocity_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    g_Player_DeltaTime = 1.0f;

    playerState.autoTurnActive = 1;
    playerState.autoTurnTargetDir.x = -1.0f;
    playerState.autoTurnTargetDir.z = 0.0f;
    playerState.steeringInput = 1.0f;
    playerState.angVelYaw = 3.0f;
    Player::IntegrateYawAndWrapFromYawVelocity(&saveState);
    if (playerState.autoTurnActive != 0 || playerState.steeringInput != 0.0f ||
        playerState.angVelYaw != 0.0f || !FloatNear(playerState.restartYawRad, 0.0f)) {
        return 1;
    }

    playerState.restartYawRad = 6.2f;
    playerState.angVelYaw = 0.2f;
    playerState.steeringInput = 0.0f;
    modalData.yawDamping = 0.0f;
    Player::IntegrateYawAndWrapFromYawVelocity(&saveState);
    if (!FloatNear(playerState.restartYawRad, 0.1168146f)) {
        return 2;
    }

    playerState.restartYawRad = -6.2f;
    playerState.angVelYaw = -0.2f;
    Player::IntegrateYawAndWrapFromYawVelocity(&saveState);
    if (!FloatNear(playerState.restartYawRad, -0.1168146f)) {
        return 3;
    }

    playerState.restartYawRad = 1.0f;
    playerState.angVelYaw = 0.5f;
    g_Player_DeltaTime = 0.5f;
    Player::IntegrateYawAndWrapFromYawVelocity(&saveState);
    return FloatNear(playerState.restartYawRad, 1.25f) ? 0 : 4;
}

extern "C" int player_rebuild_steer_basis_from_motion_basis_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    playerState.motionBasis.yx = 7.0f;
    playerState.motionBasis.yy = 8.0f;
    playerState.motionBasis.yz = 9.0f;
    playerState.motionBasis.zx = 3.0f;
    playerState.motionBasis.zy = 4.0f;
    playerState.motionBasis.zz = 0.0f;
    playerState.steerBasisNorm.y = 99.0f;

    Player::RebuildSteerBasisFromMotionBasis(&saveState);
    if (!FloatNear(playerState.steerBasisRaw.x, -3.0f) ||
        !FloatNear(playerState.steerBasisRaw.y, -4.0f) ||
        !FloatNear(playerState.steerBasisRaw.z, 0.0f)) {
        return 1;
    }
    if (!FloatNear(playerState.steerBasisRef.x, 7.0f) ||
        !FloatNear(playerState.steerBasisRef.y, 8.0f) ||
        !FloatNear(playerState.steerBasisRef.z, 9.0f)) {
        return 2;
    }

    return FloatNear(playerState.steerBasisNorm.x, -1.0f) &&
                   FloatNear(playerState.steerBasisNorm.y, 0.0f) &&
                   FloatNear(playerState.steerBasisNorm.z, 0.0f)
               ? 0
               : 3;
}

extern "C" int player_rebuild_steer_basis_from_motion_axes_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    modalData.yawRateMax = 2.0f;
    g_Player_DeltaTime = 0.016f;

    playerState.autoTurnActive = 1;
    playerState.lifecycleState = 2;
    playerState.steeringInput = 0.0f;
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.autoTurnTargetDir = {0.0f, 0.0f, 1.0f};
    Player::RebuildSteerBasisFromMotionAxes(&saveState);
    if (playerState.autoTurnActive != 1 || playerState.steeringInput != -1.0f ||
        playerState.steeringInputCopy != -1.0f || playerState.angVelYaw != -2.0f) {
        return 1;
    }

    playerState = {};
    playerState.autoTurnActive = 1;
    playerState.steerBasisNorm = {0.0f, 0.0f, 1.0f};
    playerState.autoTurnTargetDir = {0.0f, 0.0f, 1.0f};
    playerState.cameraDir = {3.0f, 4.0f, 4.0f};
    playerState.thirdPersonYawOffset = 7.0f;
    playerState.steeringInputCopy = 1.0f;
    playerState.angVelYaw = 2.0f;
    Player::RebuildSteerBasisFromMotionAxes(&saveState);

    return playerState.autoTurnActive == 0 && playerState.steeringInputCopy == 0.0f &&
                   playerState.angVelYaw == 0.0f && playerState.thirdPersonYawOffset == 0.0f &&
                   FloatNear(playerState.cameraDirFlat.x, 0.6f) &&
                   playerState.cameraDirFlat.y == 0.0f &&
                   FloatNear(playerState.cameraDirFlat.z, 0.8f) &&
                   FloatNear(playerState.restartYawRad, -1.57079637f)
               ? 0
               : 2;
}

extern "C" int player_clear_pending_contact_queues_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    FillPendingContactQueue(&playerState.preferredCollisionQueue, 2);
    FillPendingContactQueue(&playerState.playerCollisionQueue, 3);
    FillPendingContactQueue(&playerState.worldCollisionQueue, 1);
    FillPendingContactQueue(&playerState.pickupQueue, 4);
    FillPendingContactQueue(&playerState.checkpointQueue, 1);
    FillPendingContactQueue(&playerState.transferQueue, 0);
    PlayerPendingContact transferTailOnly = {};
    playerState.transferQueue.tail = &transferTailOnly;
    playerState.transferQueue.count = 7;

    Player::ClearPendingContactQueues(&saveState);

    return PendingContactQueueCleared(playerState.preferredCollisionQueue) &&
                   PendingContactQueueCleared(playerState.playerCollisionQueue) &&
                   PendingContactQueueCleared(playerState.worldCollisionQueue) &&
                   PendingContactQueueCleared(playerState.pickupQueue) &&
                   PendingContactQueueCleared(playerState.checkpointQueue) &&
                   PendingContactQueueCleared(playerState.transferQueue)
               ? 0
               : 1;
}

extern "C" int player_pending_contact_select_preferred_smoke(void) {
    PlayerPendingContact self = {};
    PlayerPendingContact rhs = {};
    self.hit.surfaceNormal = {1.0f, 3.0f, 0.0f};
    rhs.hit.surfaceNormal = {1.0f, -5.0f, 0.0f};

    self.hit.hitPos = {0.0f, 4.0f, 0.0f};
    self.sweepEnd = {3.0f, -100.0f, 0.0f};
    rhs.hit.hitPos = {0.0f, -8.0f, 0.0f};
    rhs.sweepEnd = {-2.0f, 200.0f, 0.0f};
    if (self.SelectPreferred(&rhs) != &rhs) {
        return 1;
    }

    self.sweepEnd = {-4.0f, 10.0f, 0.0f};
    rhs.sweepEnd = {1.0f, 20.0f, 0.0f};
    if (self.SelectPreferred(&rhs) != &self) {
        return 2;
    }

    self.sweepEnd = {-2.0f, 10.0f, 0.0f};
    rhs.sweepEnd = {-2.0f, 20.0f, 0.0f};
    return self.SelectPreferred(&rhs) == &rhs ? 0 : 3;
}

extern "C" int player_select_and_resolve_preferred_pending_collision_contact_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    g_GameStateOrMapTable = nullptr;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;

    PlayerPendingContact *const first = AllocZeroedNew<PlayerPendingContact>();
    PlayerPendingContact *const second = AllocZeroedNew<PlayerPendingContact>();
    first->next = second;
    playerState.preferredCollisionQueue.head = first;
    playerState.preferredCollisionQueue.tail = second;
    playerState.preferredCollisionQueue.count = 2;

    first->hit.surfaceNormal = {1.0f, 0.0f, 0.0f};
    first->hit.hitPos = {0.0f, 0.0f, 0.0f};
    first->sweepStart = {4.0f, 0.0f, 0.0f};
    first->sweepEnd = {3.0f, 0.0f, 0.0f};

    second->hit.surfaceNormal = {0.0f, 0.0f, 0.0f};
    second->hit.hitPos = {0.0f, 0.0f, 0.0f};
    second->sweepEnd = {0.0f, 0.0f, 0.0f};

    Player::SelectAndResolvePreferredPendingCollisionContact(&saveState);
    const bool ok = playerState.preferredCollisionResolved == 1 &&
                    playerState.preferredCollisionQueue.count == 2 &&
                    playerState.preferredCollisionQueue.head == first &&
                    playerState.preferredCollisionQueue.tail == second;

    Player::ClearPendingContactQueues(&saveState);
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    return ok ? 0 : 1;
}

extern "C" int player_prepare_pending_world_collision_response_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData masterModalData = {};
    PlayerPendingContact firstContact = {};
    PlayerPendingContact secondContact = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &masterModalData;

    firstContact.next = &secondContact;
    playerState.airborneFlag = 1;
    playerState.projectileSpawnVel.y = 3.0f;
    playerState.previousTransform.posY = 50.0f;
    playerState.worldPos.y = 10.0f;
    playerState.motionBasis.posY = 10.0f;
    Player::PreparePendingWorldCollisionResponse(&saveState, &firstContact);
    const bool upwardBounceOk =
        FloatNear(playerState.projectileSpawnVel.y, -3.0f) &&
        FloatNear(playerState.localVel.y, -3.0f) &&
        FloatNear(playerState.worldPos.y, 49.6f) &&
        FloatNear(playerState.motionBasis.posY, 49.6f);

    playerState = {};
    playerState.previousTransform.posX = 10.0f;
    playerState.previousTransform.posY = 20.0f;
    playerState.previousTransform.posZ = 30.0f;
    playerState.cachedPitchRad = 0.0f;
    playerState.cachedYawRad = 0.0f;
    playerState.cachedRollRad = 0.0f;
    playerState.projectileSpawnVel = {2.0f, 5.0f, 6.0f};
    playerState.angVelPitch = 1.0f;
    playerState.angVelYaw = 2.0f;
    playerState.angVelRoll = 3.0f;
    masterModalData.masterType = 2;
    Player::PreparePendingWorldCollisionResponse(&saveState, nullptr);
    const bool restoredOk =
        Vec3Equals(playerState.worldPos, {10.0f, 19.0f, 30.0f}) &&
        FloatNear(playerState.projectileSpawnVel.y, -4.0f) &&
        Vec3Equals(playerState.localVel, {2.0f, -4.0f, 6.0f}) &&
        FloatNear(playerState.motionBasis.posX, 10.0f) &&
        FloatNear(playerState.motionBasis.posY, 19.0f) &&
        FloatNear(playerState.motionBasis.posZ, 30.0f) &&
        playerState.angVelPitch == 0.0f && playerState.angVelYaw == 0.0f &&
        playerState.angVelRoll == 0.0f;

    return upwardBounceOk && restoredOk ? 0 : 1;
}

extern "C" int player_resolve_pending_world_collision_contact_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData masterModalData = {};
    PlayerPendingContact contact = {};
    zClass_NodePartial hitNode = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &masterModalData;
    playerState.worldCollisionQueue.head = &contact;
    playerState.lifecycleState = 0;
    playerState.previousTransform.posX = 3.0f;
    playerState.previousTransform.posY = 4.0f;
    playerState.previousTransform.posZ = 5.0f;
    playerState.projectileSpawnVel = {1.0f, -2.0f, 3.0f};
    contact.hit.node = &hitNode;
    contact.hit.surfaceNormal = {0.0f, 1.0f, 0.0f};
    contact.hit.hitPos = {0.0f, 0.0f, 0.0f};
    contact.sweepStart = {0.0f, 1.0f, 0.0f};

    Player::ResolvePendingWorldCollisionContact(&saveState);
    return Vec3Equals(playerState.worldPos, {3.0f, 4.0f, 5.0f}) &&
                   Vec3Equals(playerState.localVel, {1.0f, -2.0f, 3.0f}) &&
                   FloatNear(playerState.motionBasis.posX, 3.0f) &&
                   FloatNear(playerState.motionBasis.posY, 4.0f) &&
                   FloatNear(playerState.motionBasis.posZ, 5.0f)
               ? 0
               : 1;
}

extern "C" int checkpoint_update_player_lap_progress_and_notify_net_smoke(void) {
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;
    const int oldCheckpointCount = g_HudSensorTracker.checkpointCount;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldIsHost = g_zNetwork_IsHostFlag;

    zNetwork_DPlay4 dplay = {&kPlayerCheckpointDPlayVtable};
    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x1234;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x1234;
    g_zNetwork_IsHostFlag = 0;
    g_PlayerTestCheckpointNetSendCalls = 0;
    g_PlayerTestCheckpointNetSendFlags = 0;
    g_PlayerTestCheckpointNetSendSize = 0;
    std::memset(g_PlayerTestCheckpointNetPacketBytes, 0,
                sizeof(g_PlayerTestCheckpointNetPacketBytes));

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;
    g_HudSensorTracker.checkpointCount = 3;

    PlayerStateFieldAt<int>(playerState, 0x1020) = 1;
    Checkpoint::UpdatePlayerLapProgressAndNotifyNet(&saveState, 2);
    if (PlayerStateFieldAt<int>(playerState, 0x1020) != 1 ||
        g_PlayerTestCheckpointNetSendCalls != 0) {
        g_Time_AccumulatedTimeSec = oldAccumulatedTime;
        g_HudSensorTracker.checkpointCount = oldCheckpointCount;
        g_zNetwork_pDirectPlay4 = oldDPlay;
        g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
        g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
        g_zNetwork_IsHostFlag = oldIsHost;
        return 1;
    }

    Checkpoint::UpdatePlayerLapProgressAndNotifyNet(&saveState, 1);
    if (PlayerStateFieldAt<int>(playerState, 0x101c) != 1 ||
        g_PlayerTestCheckpointNetSendCalls != 0) {
        g_Time_AccumulatedTimeSec = oldAccumulatedTime;
        g_HudSensorTracker.checkpointCount = oldCheckpointCount;
        g_zNetwork_pDirectPlay4 = oldDPlay;
        g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
        g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
        g_zNetwork_IsHostFlag = oldIsHost;
        return 2;
    }

    PlayerStateFieldAt<int>(playerState, 0x1020) = 0;
    Checkpoint::UpdatePlayerLapProgressAndNotifyNet(&saveState, 3);
    if (PlayerStateFieldAt<int>(playerState, 0x101c) != 0 ||
        PlayerStateFieldAt<int>(playerState, 0x1020) != 0 ||
        PlayerStateFieldAt<int>(playerState, 0x1024) != 0 ||
        g_PlayerTestCheckpointNetSendCalls != 0) {
        g_Time_AccumulatedTimeSec = oldAccumulatedTime;
        g_HudSensorTracker.checkpointCount = oldCheckpointCount;
        g_zNetwork_pDirectPlay4 = oldDPlay;
        g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
        g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
        g_zNetwork_IsHostFlag = oldIsHost;
        return 3;
    }

    PlayerStateFieldAt<int>(playerState, 0x101c) = 1;
    PlayerStateFieldAt<int>(playerState, 0x1020) = 1;
    PlayerStateFieldAt<float>(playerState, 0x10a4) = 35.0f;
    PlayerStateFieldAt<float>(playerState, 0x10a8) = 10.0f;
    PlayerStateFieldAt<int>(playerState, 0x10ac) = 7;
    g_Time_AccumulatedTimeSec = 50.0f;
    Checkpoint::UpdatePlayerLapProgressAndNotifyNet(&saveState, 3);

    const NetPkt0E_PlayerLapProgress *const packet =
        reinterpret_cast<const NetPkt0E_PlayerLapProgress *>(g_PlayerTestCheckpointNetPacketBytes);
    const bool ok =
        PlayerStateFieldAt<int>(playerState, 0x101c) == 0 &&
        PlayerStateFieldAt<int>(playerState, 0x1020) == 0 &&
        PlayerStateFieldAt<int>(playerState, 0x1024) == 0 &&
        FloatNear(PlayerStateFieldAt<float>(playerState, 0x109c), 15.0f) &&
        FloatNear(PlayerStateFieldAt<float>(playerState, 0x10a0), 40.0f) &&
        FloatNear(PlayerStateFieldAt<float>(playerState, 0x10a4), 50.0f) &&
        PlayerStateFieldAt<int>(playerState, 0x10ac) == 8 &&
        g_PlayerTestCheckpointNetSendCalls == 1 &&
        g_PlayerTestCheckpointNetSendFlags == 1 &&
        g_PlayerTestCheckpointNetSendSize == sizeof(NetPkt0E_PlayerLapProgress) &&
        packet->header.packetType == 0x0e && packet->header.payloadDword0 == 0x1234 &&
        packet->lapCountPacked == 8 && FloatNear(packet->lapTimeSec, 40.0f);

    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    g_HudSensorTracker.checkpointCount = oldCheckpointCount;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    return ok ? 0 : 4;
}

extern "C" int player_classify_pending_contacts_for_segment_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;
    const zVec3 segmentStart = {1.0f, 2.0f, 3.0f};
    const zVec3 segmentEnd = {4.0f, 5.0f, 6.0f};
    const int segmentTag = 77;

    const int oldRaceCheckpointMode = g_HudSensorTracker.raceCheckpointMode;
    g_HudSensorTracker.raceCheckpointMode = 1;

    zClass_NodePartial checkpointContext = {};
    checkpointContext.auxFlags = 2;
    std::strcpy(checkpointContext.name, "checkpoint42");
    zClass_NodePartial checkpointNode = {};
    checkpointNode.flags = 0x200000;
    checkpointNode.callbackContext = &checkpointContext;

    PlayerProbeSampleCandidateBuffer buffer = {};
    buffer.candidateCount = 1;
    buffer.entries[0].node = &checkpointNode;
    Player::ClassifyPendingContactsForSegment(&saveState, &buffer, &segmentStart, &segmentEnd,
                                               segmentTag);
    if (g_PlayerPendingCheckpointNumber != 42 || playerState.checkpointQueue.count != 1 ||
        !PendingContactPayloadMatches(playerState.checkpointQueue.head, buffer.entries[0],
                                      segmentStart, segmentEnd, segmentTag)) {
        g_HudSensorTracker.raceCheckpointMode = oldRaceCheckpointMode;
        Player::ClearPendingContactQueues(&saveState);
        return 1;
    }

    Player::ClearPendingContactQueues(&saveState);
    g_HudSensorTracker.raceCheckpointMode = 0;

    zClass_NodePartial pickupOwner = {};
    PickupBvolHitCallbackContext pickupContext = {};
    pickupContext.ownerNode = &pickupOwner;
    zClass_NodePartial pickupBvol = {};
    pickupBvol.flags = 0x40000;
    pickupBvol.callbackContext = reinterpret_cast<zClass_NodePartial *>(&pickupContext);

    int playerType = 2;
    zClass_NodePartial playerNode = {};
    playerNode.flags = 0x100000;
    playerNode.callbackContext = reinterpret_cast<zClass_NodePartial *>(&playerType);

    OptCatalogDamageHandlerPartial transferHandler = {};
    transferHandler.timerContext = &transferHandler;
    zClass_NodeFreeListSlot transferSlot = {};
    transferSlot.damageHandler = &transferHandler;

    zClass_NodePartial worldNode = {};
    zClass_NodePartial preferredNode = {};
    zClass_NodePartial recentHitNode = {};
    struct TestContactSurfacePayload {
        unsigned char unknown_00[0x20];
        int impactSlot;
    } recentHitPayload = {};
    recentHitPayload.impactSlot = 5;

    buffer = {};
    buffer.candidateCount = 6;
    buffer.entries[0].node = &pickupBvol;
    buffer.entries[1].node = &playerNode;
    buffer.entries[2].node = &transferSlot.node;
    buffer.entries[3].node = &worldNode;
    buffer.entries[3].surfaceNormal.y = -1.0f;
    buffer.entries[4].node = &preferredNode;
    buffer.entries[4].surfaceNormal.y = 0.0f;
    buffer.entries[5].node = &recentHitNode;
    buffer.entries[5].surfaceNormal.y = 1.0f;
    buffer.entries[5].scenePayload = &recentHitPayload;

    Player::ClassifyPendingContactsForSegment(&saveState, &buffer, &segmentStart, &segmentEnd,
                                               segmentTag);

    const bool queuesOk =
        playerState.pickupQueue.count == 1 && buffer.entries[0].node == &pickupOwner &&
        PendingContactPayloadMatches(playerState.pickupQueue.head, buffer.entries[0],
                                     segmentStart, segmentEnd, segmentTag) &&
        playerState.playerCollisionQueue.count == 1 &&
        PendingContactPayloadMatches(playerState.playerCollisionQueue.head, buffer.entries[1],
                                     segmentStart, segmentEnd, segmentTag) &&
        playerState.transferQueue.count == 1 &&
        PendingContactPayloadMatches(playerState.transferQueue.head, buffer.entries[2],
                                     segmentStart, segmentEnd, segmentTag) &&
        playerState.worldCollisionQueue.count == 1 &&
        PendingContactPayloadMatches(playerState.worldCollisionQueue.head, buffer.entries[3],
                                     segmentStart, segmentEnd, segmentTag) &&
        playerState.preferredCollisionQueue.count == 1 &&
        PendingContactPayloadMatches(playerState.preferredCollisionQueue.head, buffer.entries[4],
                                     segmentStart, segmentEnd, segmentTag) &&
        playerState.recentHitValid == 1;

    g_HudSensorTracker.raceCheckpointMode = oldRaceCheckpointMode;
    Player::ClearPendingContactQueues(&saveState);
    return queuesOk ? 0 : 2;
}

extern "C" int player_collect_pending_contacts_for_segments_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;

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
    g_zModel_SharedVec3ScratchA = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    zClass_NodePartial rootNode = {};
    saveState.playerState = &playerState;
    playerState.rootNode = &rootNode;
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 0x42;
    playerState.variantTag.tags[1] = 0xff;
    playerState.variantTag.tags[2] = 0xff;

    g_VariantTag_Current.count = 0;
    g_VariantTag_Current.tags[0] = 0xff;
    g_VariantTag_Current.tags[1] = 0xff;
    g_VariantTag_Current.tags[2] = 0xff;
    g_Variant_CurrentTag.count = 3;
    g_Variant_CurrentTag.tags[0] = 1;
    g_Variant_CurrentTag.tags[1] = 2;
    g_Variant_CurrentTag.tags[2] = 3;

    zVec3 vertices[3] = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    std::int32_t indices[3] = {0, 1, 2};
    zModel_PickFaceUvData faceUvData = {};
    zModel_PickFaceScenePayload facePayload = {};
    zModel_PickFaceEntry faceEntry = {};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.vertexIndices = indices;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload = &facePayload;
    zModel_PickFaceData faceData = {};
    faceData.faceCount = 1;
    faceData.vertexCount = 3;
    faceData.faces = &faceEntry;
    faceData.baseVertices = vertices;

    zClass_Object3DDataPartial objectData = {};
    objectData.flags = 8;
    zClass_NodePartial objectNode = {};
    objectNode.flags = 0x114;
    objectNode.nodeType = 0xff;
    objectNode.classId = 5;
    objectNode.classData = &objectData;
    objectNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    objectNode.cachedBounds[0] = -10.0f;
    objectNode.cachedBounds[1] = -10.0f;
    objectNode.cachedBounds[2] = -10.0f;
    objectNode.cachedBounds[3] = 10.0f;
    objectNode.cachedBounds[4] = 10.0f;
    objectNode.cachedBounds[5] = 10.0f;
    zClass_NodePartial *worldChildren[1] = {&objectNode};
    zWorldAreaPartial area = {};
    zWorldAreaPartial *rows[1] = {&area};
    zClass_WorldDataPartial worldData = {};
    worldData.clampQueriesToBounds = 1;
    worldData.areaCellSizeX = 1.0f;
    worldData.areaCellSizeZ = 1.0f;
    worldData.areaInvSizeX = 1.0f;
    worldData.areaInvSizeZ = 1.0f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = rows;
    zClass_NodePartial world = {};
    world.classData = &worldData;
    world.listCountB = 1;
    world.listB = worldChildren;
    g_Player_RuntimeDiScene = &world;

    zClass_DiSegmentEndpoints segments[1] = {};
    segments[0].start = {0.25f, 0.25f, 1.0f};
    segments[0].end = {0.25f, 0.25f, -1.0f};
    int segmentTags[1] = {77};

    const int result =
        Player::CollectPendingContactsForSegments(&saveState, segments, 2, segmentTags);
    const bool contactsOk =
        result == 0 && playerState.preferredCollisionQueue.count == 1 &&
        PendingContactPayloadMatches(playerState.preferredCollisionQueue.head,
                                     playerState.preferredCollisionQueue.head->hit,
                                     segments[0].start, segments[0].end, 77) &&
        playerState.playerCollisionQueue.count == 0 && playerState.worldCollisionQueue.count == 0 &&
        playerState.pickupQueue.count == 0 && playerState.checkpointQueue.count == 0 &&
        playerState.transferQueue.count == 0 && (rootNode.flags & 0x10) != 0 &&
        g_Variant_CurrentTag.count == g_VariantTag_Current.count &&
        g_Variant_CurrentTag.tags[0] == g_VariantTag_Current.tags[0];

    Player::ClearPendingContactQueues(&saveState);

    PlayerPendingContactQueue emptyQueue = {};
    playerState.preferredCollisionQueue = emptyQueue;
    rootNode.flags = 0x10;
    world.listCountB = 0;
    const int emptyResult =
        Player::CollectPendingContactsForSegments(&saveState, segments, 0, segmentTags);
    const bool emptyOk = emptyResult == 1 && (rootNode.flags & 0x10) != 0;

    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_VariantTag_Current = oldVariantTagCurrent;

    return contactsOk && emptyOk ? 0 : 1;
}

extern "C" int player_pickup_contact_passes_collection_test_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const int oldBreakOnFirstCandidate = g_cls_di_BreakOnFirstCandidate;
    const int oldStopAfterFirstHit = g_cls_di_StopAfterFirstHit;

    static std::int32_t matrixFlags[8];
    static float *matrixSlots[8];
    static zMat4x3 matrix;
    matrixFlags[0] = 1;
    matrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
              0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixSlots[0] = reinterpret_cast<float *>(&matrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];

    zUtil_PlayerStateStorage playerState = {};
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 0x42;
    playerState.variantTag.tags[1] = 0xff;
    playerState.variantTag.tags[2] = 0xff;
    zInput_GameStateOrMapTablePartial gameState = {};
    gameState.playerState =
        static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&playerState));
    g_GameStateOrMapTable = &gameState;

    zClass_NodePartial pickupNode = {};
    pickupNode.flags = 0x10;
    pickupNode.cachedSphereCenter[0] = 0.25f;
    pickupNode.cachedSphereCenter[1] = -1.0f;
    pickupNode.cachedSphereCenter[2] = 0.25f;
    PlayerPendingContact contact = {};
    contact.hit.hitPos = {0.25f, 2.0f, 0.25f};
    contact.hit.node = &pickupNode;

    zClass_WorldDataPartial emptyWorldData = {};
    zClass_NodePartial emptyWorld = {};
    emptyWorld.classData = &emptyWorldData;
    g_Player_RuntimeDiScene = &emptyWorld;
    g_cls_di_BreakOnFirstCandidate = 7;
    g_cls_di_StopAfterFirstHit = 9;
    g_Variant_CurrentTag.count = 3;
    g_Variant_CurrentTag.tags[0] = 1;
    g_Variant_CurrentTag.tags[1] = 2;
    g_Variant_CurrentTag.tags[2] = 3;

    const int clearResult = PlayerPickupContact::PassesCollectionTest(nullptr, &contact);
    const bool clearOk = clearResult == 1 && (pickupNode.flags & 0x10) != 0 &&
                         g_cls_di_BreakOnFirstCandidate == 0 &&
                         g_cls_di_StopAfterFirstHit == 0 &&
                         g_Variant_CurrentTag.count == 1 &&
                         g_Variant_CurrentTag.tags[0] == 0x42;

    zVec3 vertices[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};
    std::int32_t indices[3] = {0, 1, 2};
    zModel_PickFaceUvData faceUvData = {};
    zModel_PickFaceScenePayload facePayload = {};
    zModel_PickFaceEntry faceEntry = {};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.vertexIndices = indices;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload = &facePayload;
    zModel_PickFaceData faceData = {};
    faceData.faceCount = 1;
    faceData.vertexCount = 3;
    faceData.faces = &faceEntry;
    faceData.baseVertices = vertices;

    zClass_Object3DDataPartial objectData = {};
    objectData.flags = 8;
    zClass_NodePartial obstacleNode = {};
    obstacleNode.flags = 0x11c;
    obstacleNode.nodeType = 0xff;
    obstacleNode.classId = 5;
    obstacleNode.classData = &objectData;
    obstacleNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    obstacleNode.cachedBounds[0] = 0.0f;
    obstacleNode.cachedBounds[1] = 0.0f;
    obstacleNode.cachedBounds[2] = 0.0f;
    obstacleNode.cachedBounds[3] = 1.0f;
    obstacleNode.cachedBounds[4] = 1.0f;
    obstacleNode.cachedBounds[5] = 1.0f;
    zClass_NodePartial *areaChildren[1] = {&obstacleNode};
    zWorldAreaPartial area = {};
    area.childCount = 1;
    area.childList = areaChildren;
    zWorldAreaPartial *areaRows[1] = {&area};
    zClass_WorldDataPartial blockedWorldData = {};
    blockedWorldData.clampQueriesToBounds = 1;
    blockedWorldData.areaCellSizeX = 1.0f;
    blockedWorldData.areaCellSizeZ = 1.0f;
    blockedWorldData.areaInvSizeX = 1.0f;
    blockedWorldData.areaInvSizeZ = 1.0f;
    blockedWorldData.areaGridColCount = 1;
    blockedWorldData.areaGridRowCount = 1;
    blockedWorldData.areaGridRows = areaRows;
    zClass_NodePartial blockedWorld = {};
    blockedWorld.classData = &blockedWorldData;
    g_Player_RuntimeDiScene = &blockedWorld;
    pickupNode.flags = 0x10;

    const int blockedResult = PlayerPickupContact::PassesCollectionTest(nullptr, &contact);
    const bool blockedOk = blockedResult == 0 && (pickupNode.flags & 0x10) != 0 &&
                           g_cls_di_BreakOnFirstCandidate == 0 &&
                           g_cls_di_StopAfterFirstHit == 0;

    g_GameStateOrMapTable = oldGameState;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_cls_di_BreakOnFirstCandidate = oldBreakOnFirstCandidate;
    g_cls_di_StopAfterFirstHit = oldStopAfterFirstHit;

    return clearOk && blockedOk ? 0 : 1;
}

extern "C" int player_process_pending_pickup_contacts_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    zEffectAnimEntry *const oldEntryList = g_zEffectAnim_EntryList;
    const short oldEntryCount = g_zEffectAnim_EntryCount;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    HudUiShieldMessageWidget *const oldShieldWidget = g_HudUiMgrShieldMessageWidget;
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    const float oldStatusMeterRatio = g_PlayerStatusMeterRatio;

    static std::int32_t matrixFlags[8];
    static float *matrixSlots[8];
    static zMat4x3 matrix;
    matrixFlags[0] = 1;
    matrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
              0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixSlots[0] = reinterpret_cast<float *>(&matrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    HudUiShieldMessageWidget shield = {};
    HudUiTextStack4 topStack = {};
    std::int32_t networkEnabled = 0;
    zClass_NodePartial rootNode = {};
    zClass_NodePartial pickupNode = {};
    zClass_NodePartial hitNode = {};
    PickupBvolHitCallbackContext hitContext = {};
    PlayerPendingContact contact = {};

    saveState.playerState = &playerState;
    playerState.masterCommonData = &commonData;
    playerState.rootNode = &rootNode;
    playerState.statusMeterValue = 20.0f;
    commonData.maxHealth = 100.0f;
    commonData.invMaxHealth = 0.01f;
    playerState.pickupQueue.head = &contact;
    playerState.pickupQueue.tail = &contact;
    playerState.pickupQueue.count = 1;

    PickupSpawnDef *const spawn =
        static_cast<PickupSpawnDef *>(std::malloc(sizeof(PickupSpawnDef)));
    if (spawn == nullptr) {
        g_GameStateOrMapTable = oldGameState;
        g_Player_RuntimeDiScene = oldRuntimeScene;
        g_zEffectAnim_EntryList = oldEntryList;
        g_zEffectAnim_EntryCount = oldEntryCount;
        ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
        g_HudUiMgrShieldMessageWidget = oldShieldWidget;
        g_HudUiTopMessageStack = oldTopStack;
        g_PlayerStatusMeterRatio = oldStatusMeterRatio;
        return 1;
    }

    *spawn = {};
    spawn->amount = 30;
    spawn->pickupObj = &pickupNode;
    pickupNode.classId = 5;
    pickupNode.flags = 0x4001c;
    pickupNode.callbackContext = (zClass_NodePartial *)(spawn);
    TestFieldAt<std::int32_t>(pickupNode.name, 0x1c) = 0x22;
    hitContext.ownerNode = &pickupNode;
    hitNode.classId = 5;
    hitNode.flags = 0x40010;
    hitNode.callbackContext = (zClass_NodePartial *)(&hitContext);
    hitNode.cachedSphereCenter[0] = 0.25f;
    hitNode.cachedSphereCenter[1] = -1.0f;
    hitNode.cachedSphereCenter[2] = 0.25f;
    contact.hit.node = &hitNode;
    contact.hit.hitPos = {0.25f, 2.0f, 0.25f};

    zClass_WorldDataPartial emptyWorldData = {};
    zClass_NodePartial emptyWorld = {};
    emptyWorld.classData = &emptyWorldData;
    g_Player_RuntimeDiScene = &emptyWorld;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);
    g_zEffectAnim_EntryList = nullptr;
    g_zEffectAnim_EntryCount = 0;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_HudUiMgrShieldMessageWidget = &shield;
    g_HudUiTopMessageStack = &topStack;
    g_PlayerStatusMeterRatio = 0.2f;
    shield.meter.ftable = &g_HudUiMeter_FTable;
    shield.meter.fillPixelsMax = 20;
    shield.meter.points[1].y = 100.0f;
    reinterpret_cast<HudUiPanel *>(&shield.percentTextPanel)->vtbl =
        &g_HudUiPanelSimple_FTable;

    Player::ProcessPendingPickupContacts(&saveState);

    const bool processed = playerState.statusMeterValue == 50.0f &&
                           (pickupNode.flags & 0x4001c) == 0 &&
                           playerState.pickupQueue.head == &contact;

    g_GameStateOrMapTable = oldGameState;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_zEffectAnim_EntryList = oldEntryList;
    g_zEffectAnim_EntryCount = oldEntryCount;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_HudUiMgrShieldMessageWidget = oldShieldWidget;
    g_HudUiTopMessageStack = oldTopStack;
    g_PlayerStatusMeterRatio = oldStatusMeterRatio;
    return processed ? 0 : 2;
}

extern "C" int player_build_pending_contact_queues_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    const float oldDeltaTime = g_Player_DeltaTime;

    static std::int32_t matrixFlags[8];
    static float *matrixSlots[8];
    static zMat4x3 matrix;
    matrixFlags[0] = 1;
    matrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
              0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixSlots[0] = reinterpret_cast<float *>(&matrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData masterModalData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial world = {};
    zClass_WorldDataPartial worldData = {};
    zWorldAreaPartial area = {};
    zWorldAreaPartial *rows[1] = {&area};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &masterModalData;
    playerState.rootNode = &rootNode;
    g_Player_RuntimeDiScene = &world;
    world.classData = &worldData;
    worldData.clampQueriesToBounds = 1;
    worldData.areaCellSizeX = 1.0f;
    worldData.areaCellSizeZ = 1.0f;
    worldData.areaInvSizeX = 1.0f;
    worldData.areaInvSizeZ = 1.0f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = rows;
    g_GameStateOrMapTable = nullptr;
    g_Player_DeltaTime = 0.5f;

    playerState.localVel = {0.0f, 0.0f, -2.0f};
    playerState.projectileSpawnVel = {0.0f, 6.0f, 0.0f};
    playerState.motionBasis = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                               0.0f, 0.0f, 1.0f, 10.0f, 20.0f, 30.0f};
    playerState.previousTransform = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                     0.0f, 0.0f, 1.0f, 100.0f, 200.0f, 300.0f};

    masterModalData.masterType = 1;
    masterModalData.probePointCount = 5;
    for (int i = 0; i < masterModalData.probePointCount; ++i) {
        masterModalData.probePoints[i] = {static_cast<float>(i + 1),
                                          static_cast<float>(i + 2),
                                          static_cast<float>(i + 3)};
    }

    Player::BuildPendingContactQueues(&saveState);

    const bool modalCacheOk = Vec3Equals(playerState.modalProbeWorldByIndex[0],
                                         {11.0f, 22.0f, 33.0f});
    const bool rootCacheOk = Vec3Equals(playerState.rootProbeWorldByIndex[0],
                                        {101.0f, 202.0f, 303.0f});
    const bool queuesOk = playerState.noPendingContactsQueued == 1 &&
                          playerState.preferredCollisionQueue.count == 0 &&
                          playerState.playerCollisionQueue.count == 0 &&
                          playerState.worldCollisionQueue.count == 0 &&
                          playerState.pickupQueue.count == 0 &&
                          playerState.checkpointQueue.count == 0 &&
                          playerState.transferQueue.count == 0 &&
                          (rootNode.flags & 0x10) != 0;

    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_GameStateOrMapTable = oldGameState;
    g_Player_DeltaTime = oldDeltaTime;

    if (!modalCacheOk) {
        return 1;
    }
    if (!rootCacheOk) {
        return 2;
    }
    return queuesOk ? 0 : 3;
}

extern "C" int player_process_pending_contact_queues_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData masterModalData = {};
    PlayerPendingContact *const staleContact = new PlayerPendingContact;
    memset(staleContact, 0, sizeof(*staleContact));

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &masterModalData;
    g_GameStateOrMapTable = nullptr;

    playerState.preferredCollisionQueue.head = staleContact;
    playerState.preferredCollisionQueue.tail = staleContact;
    playerState.preferredCollisionQueue.count = 1;
    playerState.pickupQueueProcessed = 1;
    playerState.playerCollisionResolved = 1;
    playerState.worldCollisionResolved = 1;
    playerState.preferredCollisionResolved = 1;
    playerState.checkpointLapProgressNotified = 1;
    playerState.motionBasis = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                               0.0f, 0.0f, 1.0f, 10.0f, 20.0f, 30.0f};
    playerState.previousTransform = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                     0.0f, 0.0f, 1.0f, 100.0f, 200.0f, 300.0f};
    masterModalData.probePointCount = 1;
    masterModalData.probePoints[0] = {1.0f, 2.0f, 3.0f};

    Player::ProcessPendingContactQueues(&saveState);

    const bool flagsOk = playerState.pickupQueueProcessed == 0 &&
                         playerState.playerCollisionResolved == 0 &&
                         playerState.worldCollisionResolved == 0 &&
                         playerState.preferredCollisionResolved == 0 &&
                         playerState.checkpointLapProgressNotified == 0;
    const bool queuesOk = playerState.noPendingContactsQueued == 1 &&
                          playerState.preferredCollisionQueue.count == 0 &&
                          playerState.playerCollisionQueue.count == 0 &&
                          playerState.worldCollisionQueue.count == 0 &&
                          playerState.pickupQueue.count == 0 &&
                          playerState.checkpointQueue.count == 0 &&
                          playerState.transferQueue.count == 0;
    const bool cachesOk = Vec3Equals(playerState.modalProbeWorldByIndex[0],
                                     {11.0f, 22.0f, 33.0f}) &&
                          Vec3Equals(playerState.rootProbeWorldByIndex[0],
                                     {101.0f, 202.0f, 303.0f});

    g_GameStateOrMapTable = oldGameState;
    return flagsOk && queuesOk && cachesOk ? 0 : 1;
}

extern "C" int player_collect_pending_collision_contacts_for_quad_probe_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;

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

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData masterModalData = {};
    zClass_NodePartial rootNode = {};
    zClass_WorldDataPartial worldData = {};
    zWorldAreaPartial area = {};
    zWorldAreaPartial *rows[1] = {&area};
    zClass_NodePartial world = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &masterModalData;
    playerState.rootNode = &rootNode;
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 7;
    playerState.motionBasis = {1.0f, 0.0f, 0.0f,
                               0.0f, 1.0f, 0.0f,
                               0.0f, 0.0f, 1.0f,
                               10.0f, 20.0f, 30.0f};

    masterModalData.probePoints[0] = {1.0f, 2.0f, 3.0f};
    masterModalData.probePoints[2] = {4.0f, 5.0f, 6.0f};
    masterModalData.probePoints[3] = {-1.0f, 0.0f, 2.0f};
    masterModalData.probePoints[5] = {0.5f, 1.5f, -2.0f};

    world.classData = &worldData;
    worldData.clampQueriesToBounds = 1;
    worldData.areaCellSizeX = 1.0f;
    worldData.areaCellSizeZ = 1.0f;
    worldData.areaInvSizeX = 1.0f;
    worldData.areaInvSizeZ = 1.0f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = rows;
    g_Player_RuntimeDiScene = &world;

    const int result = Player::CollectPendingCollisionContactsForQuadProbe(&saveState, 2.5f);
    const bool transformed =
        Vec3Equals(playerState.modalProbeWorldByIndex[0], {11.0f, 24.5f, 33.0f}) &&
        Vec3Equals(playerState.modalProbeWorldByIndex[2], {14.0f, 27.5f, 36.0f}) &&
        Vec3Equals(playerState.modalProbeWorldByIndex[3], {9.0f, 22.5f, 32.0f}) &&
        Vec3Equals(playerState.modalProbeWorldByIndex[5], {10.5f, 24.0f, 28.0f});
    const bool queuesOk = result == 0 && playerState.preferredCollisionQueue.count == 0 &&
                          playerState.playerCollisionQueue.count == 0 &&
                          playerState.transferQueue.count == 0 && (rootNode.flags & 0x10) != 0;

    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_VariantTag_Current = oldVariantTagCurrent;
    return transformed && queuesOk ? 0 : 1;
}

extern "C" int player_try_resolve_pending_collision_probe_sweep_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;

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

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    zClass_NodePartial rootNode = {};
    saveState.playerState = &playerState;
    playerState.rootNode = &rootNode;
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 0x42;
    playerState.variantTag.tags[1] = 0xff;
    playerState.variantTag.tags[2] = 0xff;
    playerState.previousTransform.posX = 4.0f;
    playerState.previousTransform.posY = 5.0f;
    playerState.previousTransform.posZ = 6.0f;
    playerState.motionBasis.xx = 1.0f;
    playerState.motionBasis.yy = 1.0f;
    playerState.motionBasis.zz = 1.0f;
    playerState.collisionProbeResolved = 1;

    g_VariantTag_Current.count = 0;
    g_VariantTag_Current.tags[0] = 0xff;
    g_VariantTag_Current.tags[1] = 0xff;
    g_VariantTag_Current.tags[2] = 0xff;

    zClass_WorldDataPartial emptyWorldData = {};
    zClass_NodePartial emptyWorld = {};
    emptyWorld.classData = &emptyWorldData;
    g_Player_RuntimeDiScene = &emptyWorld;

    for (int i = 0; i < 6; ++i) {
        playerState.rootProbeWorldByIndex[i] = {20.0f + static_cast<float>(i), 20.0f, 20.0f};
        playerState.modalProbeWorldByIndex[i] = {21.0f + static_cast<float>(i), 21.0f, 21.0f};
    }

    int result = Player::TryResolvePendingCollisionProbeSweep(&saveState);
    if (result != 0 || playerState.collisionProbeResolved != 0 ||
        playerState.preferredCollisionQueue.count != 0 ||
        playerState.playerCollisionQueue.count != 0) {
        g_Player_RuntimeDiScene = oldRuntimeScene;
        g_Variant_CurrentTag = oldVariantCurrent;
        g_VariantTag_Current = oldVariantTagCurrent;
        return 1;
    }

    zVec3 vertices[3] = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    std::int32_t indices[3] = {0, 1, 2};
    zModel_PickFaceUvData faceUvData = {};
    zModel_PickFaceScenePayload facePayload = {};
    zModel_PickFaceEntry faceEntry = {};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.vertexIndices = indices;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload = &facePayload;
    zModel_PickFaceData faceData = {};
    faceData.faceCount = 1;
    faceData.vertexCount = 3;
    faceData.faces = &faceEntry;
    faceData.baseVertices = vertices;

    OptCatalogDamageHandlerPartial transferHandler = {};
    transferHandler.timerContext = &transferHandler;
    zClass_NodeFreeListSlot transferSlot = {};
    transferSlot.damageHandler = &transferHandler;
    zClass_Object3DDataPartial objectData = {};
    objectData.flags = 8;
    transferSlot.node.flags = 0x114;
    transferSlot.node.nodeType = 0xff;
    transferSlot.node.classId = 5;
    transferSlot.node.classData = &objectData;
    transferSlot.node.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    transferSlot.node.cachedBounds[0] = -10.0f;
    transferSlot.node.cachedBounds[1] = -10.0f;
    transferSlot.node.cachedBounds[2] = -10.0f;
    transferSlot.node.cachedBounds[3] = 10.0f;
    transferSlot.node.cachedBounds[4] = 10.0f;
    transferSlot.node.cachedBounds[5] = 10.0f;
    zClass_NodePartial *worldChildren[1] = {&transferSlot.node};
    zWorldAreaPartial area = {};
    zWorldAreaPartial *rows[1] = {&area};
    zClass_WorldDataPartial worldData = {};
    worldData.clampQueriesToBounds = 1;
    worldData.areaCellSizeX = 1.0f;
    worldData.areaCellSizeZ = 1.0f;
    worldData.areaInvSizeX = 1.0f;
    worldData.areaInvSizeZ = 1.0f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = rows;
    zClass_NodePartial world = {};
    world.classData = &worldData;
    world.listCountB = 1;
    world.listB = worldChildren;
    g_Player_RuntimeDiScene = &world;

    for (int i = 0; i < 6; ++i) {
        playerState.rootProbeWorldByIndex[i] = {20.0f + static_cast<float>(i), 20.0f, 20.0f};
        playerState.modalProbeWorldByIndex[i] = {21.0f + static_cast<float>(i), 21.0f, 21.0f};
    }
    playerState.rootProbeWorldByIndex[0] = {0.25f, 0.25f, 1.0f};
    playerState.modalProbeWorldByIndex[0] = {0.25f, 0.25f, -1.0f};

    result = Player::TryResolvePendingCollisionProbeSweep(&saveState);
    const bool transferResolved =
        result == 1 && playerState.collisionProbeResolved == 1 &&
        playerState.transferQueue.count == 0 && playerState.preferredCollisionQueue.count == 1 &&
        playerState.preferredCollisionQueue.head != nullptr &&
        playerState.preferredCollisionQueue.head->hit.node == &transferSlot.node &&
        playerState.preferredCollisionQueue.head->segmentTag == 0;

    Player::ClearPendingContactQueues(&saveState);
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_VariantTag_Current = oldVariantTagCurrent;
    return transferResolved ? 0 : 2;
}

extern "C" int player_resolve_pending_collision_contact_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const float oldScale = g_Player_CollisionContactResolveScale;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData masterModalData = {};
    zClass_WorldDataPartial worldData = {};
    zClass_NodePartial world = {};
    zClass_NodePartial hitNode = {};
    PlayerPendingContact contact = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &masterModalData;
    masterModalData.probePointCount = 2;
    masterModalData.maxSpeed = 10.0f;
    masterModalData.collisionDampingA = 0.5f;
    masterModalData.collisionDampingB = 0.1f;
    playerState.localVel = {3.0f, 0.0f, 4.0f};
    playerState.worldPos = {10.0f, 0.0f, 10.0f};
    playerState.motionBasis.xx = 1.0f;
    playerState.motionBasis.yy = 1.0f;
    playerState.motionBasis.zz = 1.0f;
    playerState.modalProbeWorldByIndex[0] = {1.0f, 2.0f, 3.0f};
    playerState.modalProbeWorldByIndex[1] = {4.0f, 5.0f, 6.0f};
    playerState.projectileSpawnVel.y = 0.0f;
    contact.hit.node = &hitNode;
    contact.hit.surfaceNormal = {1.0f, 0.0f, 0.0f};
    contact.hit.hitPos = {0.0f, 0.0f, 0.0f};
    contact.sweepStart = {-1.0f, 0.0f, 0.0f};
    contact.sweepEnd = {1.0f, 0.0f, 0.0f};
    world.classData = &worldData;
    g_Player_RuntimeDiScene = &world;
    g_GameStateOrMapTable = nullptr;
    g_Player_CollisionContactResolveScale = 0.2f;

    Player::ResolvePendingCollisionContact(&saveState, &contact);
    const bool resolved =
        FloatNear(playerState.worldPos.x, 8.0f) &&
        FloatNear(playerState.motionBasis.posX, 8.0f) &&
        FloatNear(playerState.motionBasis.posZ, 10.0f) &&
        Vec3Equals(playerState.modalProbeWorldByIndex[0], {-1.0f, 2.0f, 3.0f}) &&
        Vec3Equals(playerState.modalProbeWorldByIndex[1], {2.0f, 5.0f, 6.0f}) &&
        FloatNear(playerState.projectileSpawnVel.x, -2.5f) &&
        FloatNear(playerState.projectileSpawnVel.y, 0.0f) &&
        FloatNear(playerState.projectileSpawnVel.z, 0.0f) &&
        FloatNear(playerState.localVel.x, -2.5f) &&
        FloatNear(playerState.localVel.y, 0.0f) &&
        FloatNear(playerState.localVel.z, 0.0f) &&
        FloatNear(playerState.angVelYaw, 0.5f);

    zUtil_PlayerStateStorage degenerateState = {};
    zUtil_SaveGameState degenerateSave = {};
    PlayerModalState degenerateModal = {};
    PlayerMasterModalData degenerateMaster = {};
    PlayerPendingContact degenerateContact = {};
    degenerateSave.playerState = &degenerateState;
    degenerateSave.primaryModalState = &degenerateModal;
    degenerateModal.masterModalData = &degenerateMaster;
    degenerateState.worldPos = {1.0f, 2.0f, 3.0f};
    degenerateContact.hit.surfaceNormal = {0.0f, 1.0f, 0.0f};
    degenerateContact.sweepStart = {1.0f, 0.0f, 0.0f};
    degenerateContact.hit.hitPos = {0.0f, 0.0f, 0.0f};
    Player::ResolvePendingCollisionContact(&degenerateSave, &degenerateContact);
    const bool degenerateOk = Vec3Equals(degenerateState.worldPos, {1.0f, 2.0f, 3.0f});

    g_Player_CollisionContactResolveScale = oldScale;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    return resolved && degenerateOk ? 0 : 1;
}

extern "C" int player_resolve_pending_player_collision_contact_smoke(void) {
    const float oldDamageFeedbackScalar = g_OptCatalogDamageFeedbackIntensityScalar;

    struct PlayerCollisionContactContextTest {
        void *unknown_00;
        zUtil_SaveGameState *saveState;
    };

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData masterModalData = {};
    PlayerPendingContact contact = {};
    zClass_NodePartial targetNode = {};
    PlayerCollisionContactContextTest targetContext = {};

    zUtil_SaveGameState targetSaveState = {};
    zUtil_PlayerStateStorage targetPlayerState = {};
    PlayerModalState targetModalState = {};
    PlayerMasterModalData targetModalData = {};
    PlayerMasterCommonData targetCommonData = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &masterModalData;
    masterModalData.mass = 4.0f;
    playerState.playerCollisionQueue.head = &contact;
    playerState.projectileSpawnVel = {2.0f, 5.0f, 0.0f};
    contact.hit.node = &targetNode;
    contact.hit.surfaceNormal = {0.0f, 1.0f, 0.0f};
    contact.hit.hitPos = {0.0f, 0.0f, 0.0f};
    contact.sweepStart = {0.0f, 1.0f, 0.0f};
    targetNode.callbackContext = (zClass_NodePartial *)(void *)(&targetContext);
    targetContext.saveState = &targetSaveState;

    targetSaveState.playerState = &targetPlayerState;
    targetSaveState.primaryModalState = &targetModalState;
    targetModalState.masterModalData = &targetModalData;
    targetModalData.invMass = 0.5f;
    targetPlayerState.masterCommonData = &targetCommonData;
    targetCommonData.invMaxHealth = 0.01f;
    targetPlayerState.localVel = {1.0f, 7.0f, 3.0f};
    targetPlayerState.statusMeterValue = 100.0f;
    targetPlayerState.lifecycleState = 1;

    Player::ResolvePendingPlayerCollisionContact(&saveState);
    const bool transferOnlyOk =
        FloatNear(targetPlayerState.localVel.x, 5.0f) &&
        FloatNear(targetPlayerState.localVel.y, 7.0f) &&
        FloatNear(targetPlayerState.localVel.z, 3.0f) &&
        FloatNear(targetPlayerState.statusMeterValue, 100.0f);

    targetPlayerState.localVel = {1.0f, 7.0f, 3.0f};
    targetPlayerState.statusMeterValue = 100.0f;
    targetPlayerState.statusMeterScaled = 1.0f;
    targetPlayerState.lifecycleState = 2;
    targetPlayerState.aiTopLevelState = 1;

    Player::ResolvePendingPlayerCollisionContact(&saveState);
    const bool aiDamageOk =
        FloatNear(targetPlayerState.localVel.x, 5.0f) &&
        FloatNear(targetPlayerState.localVel.y, 7.0f) &&
        FloatNear(targetPlayerState.localVel.z, 3.0f) &&
        FloatNear(targetPlayerState.statusMeterValue, 97.8f) &&
        FloatNear(targetPlayerState.statusMeterScaled, 0.978f) &&
        FloatNear(g_OptCatalogDamageFeedbackIntensityScalar, 0.978f);

    g_OptCatalogDamageFeedbackIntensityScalar = oldDamageFeedbackScalar;
    return transferOnlyOk && aiDamageOk ? 0 : 1;
}

extern "C" int player_process_transfer_contact_queue_smoke(void) {
    const int oldCaptureEnabled = g_OptCatalog_CaptureHitSnapshotEnabled;
    const zVec3 oldSourcePos = g_OptCatalog_CapturedDamageSourcePos;
    const zVec3 oldHitPos = g_OptCatalog_CapturedDamageHitPos;
    PlayerNodeFlagRestoreEntry *const oldRestoreBegin = g_PlayerNodeFlagRestoreEntriesBegin;
    PlayerNodeFlagRestoreEntry *const oldRestoreEnd = g_PlayerNodeFlagRestoreEntriesEnd;
    PlayerNodeFlagRestoreEntry *const oldRestoreCapacity =
        g_PlayerNodeFlagRestoreEntriesCapacityEnd;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData masterModalData = {};
    PlayerPendingContact positive = {};
    PlayerPendingContact blocked = {};
    zClass_NodeFreeListSlot positiveNode = {};
    zClass_NodeFreeListSlot blockedNode = {};
    OptCatalogDamageHandlerPartial positiveHandler = {};
    OptCatalogDamageHandlerPartial blockedHandler = {};
    float positiveResult = 1.0f;
    float blockedResult = 0.0f;

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &masterModalData;
    masterModalData.maxSpeed = 10.0f;
    playerState.localVel = {3.0f, 4.0f, 0.0f};
    playerState.projectileSpawnVel = {6.0f, 0.0f, -3.0f};
    playerState.transferQueue.head = &positive;
    playerState.transferQueue.tail = &blocked;
    playerState.transferQueue.count = 2;
    positive.next = &blocked;
    positive.hit.node = &positiveNode.node;
    positive.sweepStart = {4.0f, 5.0f, 6.0f};
    positive.hit.hitPos = {7.0f, 8.0f, 9.0f};
    blocked.hit.node = &blockedNode.node;
    blocked.sweepStart = {10.0f, 11.0f, 12.0f};
    blocked.hit.hitPos = {13.0f, 14.0f, 15.0f};
    positiveNode.damageHandler = &positiveHandler;
    blockedNode.damageHandler = &blockedHandler;
    blockedNode.node.flags = 0x38;
    positiveHandler.timerCallback = (void *)PlayerTransferDamageTimerCallback;
    positiveHandler.timerContext = &positiveResult;
    blockedHandler.timerCallback = (void *)PlayerTransferDamageTimerCallback;
    blockedHandler.timerContext = &blockedResult;
    g_PlayerTestTransferDamageCalls = 0;
    g_PlayerTestTransferDamageArgs[0] = 0.0f;
    g_PlayerTestTransferDamageArgs[1] = 0.0f;
    g_OptCatalog_CaptureHitSnapshotEnabled = 1;
    g_PlayerNodeFlagRestoreEntriesBegin = nullptr;
    g_PlayerNodeFlagRestoreEntriesEnd = nullptr;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = nullptr;

    Player::ProcessTransferContactQueue(&saveState);

    const bool queuesOk =
        playerState.transferQueue.count == 1 && playerState.transferQueue.head == &blocked &&
        playerState.transferQueue.tail == &blocked && blocked.next == nullptr &&
        playerState.preferredCollisionQueue.count == 1 &&
        playerState.preferredCollisionQueue.head == &positive &&
        playerState.preferredCollisionQueue.tail == &positive && positive.next == nullptr;
    const bool callbackOk = g_PlayerTestTransferDamageCalls == 2 &&
                            FloatNear(g_PlayerTestTransferDamageArgs[0], 1.25f) &&
                            FloatNear(g_PlayerTestTransferDamageArgs[1], 1.25f);
    const bool disabledOk = (blockedNode.node.flags & 0x18) == 0 &&
                            (blockedNode.node.flags & 0x20) != 0 &&
                            g_PlayerNodeFlagRestoreEntriesEnd ==
                                g_PlayerNodeFlagRestoreEntriesBegin + 1 &&
                            g_PlayerNodeFlagRestoreEntriesBegin[0].node == &blockedNode.node &&
                            g_PlayerNodeFlagRestoreEntriesBegin[0].wasCellPickable == 1 &&
                            g_PlayerNodeFlagRestoreEntriesBegin[0].wasRaycastable == 1 &&
                            g_PlayerNodeFlagRestoreEntriesBegin[0].wasPickable == 1;
    const bool captureOk =
        Vec3Equals(g_OptCatalog_CapturedDamageSourcePos, {10.0f, 11.0f, 12.0f}) &&
        Vec3Equals(g_OptCatalog_CapturedDamageHitPos, {13.0f, 14.0f, 15.0f});
    const bool dampingOk =
        FloatNear(playerState.localVel.x, 2.00010014f) &&
        FloatNear(playerState.localVel.y, 2.66680002f) &&
        FloatNear(playerState.localVel.z, 0.0f) &&
        FloatNear(playerState.projectileSpawnVel.x, 4.00020027f) &&
        FloatNear(playerState.projectileSpawnVel.y, 0.0f) &&
        FloatNear(playerState.projectileSpawnVel.z, -2.00010014f);

    ::operator delete(g_PlayerNodeFlagRestoreEntriesBegin);
    g_PlayerNodeFlagRestoreEntriesBegin = oldRestoreBegin;
    g_PlayerNodeFlagRestoreEntriesEnd = oldRestoreEnd;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = oldRestoreCapacity;
    g_OptCatalog_CaptureHitSnapshotEnabled = oldCaptureEnabled;
    g_OptCatalog_CapturedDamageSourcePos = oldSourcePos;
    g_OptCatalog_CapturedDamageHitPos = oldHitPos;
    return queuesOk && callbackOk && disabledOk && captureOk && dampingOk ? 0 : 1;
}

extern "C" int player_apply_pitch_roll_velocity_impulse_from_direction_smoke(void) {
    int *const oldMatrixIdentityFlagSlot = zMath::g_currentMatrixIdentityFlagSlot;
    float **const oldMatrixPtrSlot = zMath::g_currentMatrixPtrSlot;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    zClass_NodePartial rootNode = {};
    zClass_Object3DDataPartial rootData = {};
    int matrixFlags[3] = {};
    float *matrixSlots[3] = {};
    zMat4x3 currentMatrix = {};

    saveState.playerState = &playerState;
    playerState.rootNode = &rootNode;
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    rootData.localMatrix[0] = 1.0f;
    rootData.localMatrix[1] = 2.0f;
    rootData.localMatrix[2] = 3.0f;
    rootData.localMatrix[3] = 4.0f;
    rootData.localMatrix[4] = 5.0f;
    rootData.localMatrix[5] = 6.0f;
    rootData.localMatrix[6] = 7.0f;
    rootData.localMatrix[7] = 8.0f;
    rootData.localMatrix[8] = 9.0f;
    rootData.localMatrix[9] = 100.0f;
    rootData.localMatrix[10] = 200.0f;
    rootData.localMatrix[11] = 300.0f;

    playerState.vehiclePitchRad = 10.0f;
    playerState.vehicleRollRad = 20.0f;
    playerState.localVel = {5.0f, 6.0f, -1.0f};

    const zMat4x3 currentBefore = {0.5f, 0.0f, 0.0f, 0.0f, 0.75f, 0.0f,
                                   0.0f, 0.0f, 1.25f, 7.0f, 8.0f, 9.0f};
    matrixSlots[0] = (float *)(&currentMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    zMath::MatLoadCurrentFrom(&currentBefore);

    const zVec3 direction = {1.0f, 2.0f, 3.0f};
    Player::ApplyPitchRollVelocityImpulseFromDirection(&saveState, &direction, 0.5f, 0.25f);

    zMat4x3 currentAfter = {};
    zMath::MatCopyCurrentTo(&currentAfter);

    const bool impulseApplied =
        FloatNear(playerState.vehiclePitchRad, -11.0f) &&
        FloatNear(playerState.vehicleRollRad, 35.0f) &&
        FloatNear(playerState.localVel.x, -2.5f) &&
        FloatNear(playerState.localVel.y, 6.0f) &&
        FloatNear(playerState.localVel.z, -11.5f);
    const bool stackRestored =
        FloatNear(currentAfter.xx, currentBefore.xx) &&
        FloatNear(currentAfter.yy, currentBefore.yy) &&
        FloatNear(currentAfter.zz, currentBefore.zz) &&
        FloatNear(currentAfter.posX, currentBefore.posX) &&
        FloatNear(currentAfter.posY, currentBefore.posY) &&
        FloatNear(currentAfter.posZ, currentBefore.posZ) &&
        zMath::g_currentMatrixIdentityFlagSlot == &matrixFlags[0] &&
        zMath::g_currentMatrixPtrSlot == &matrixSlots[0];

    zMath::g_currentMatrixIdentityFlagSlot = oldMatrixIdentityFlagSlot;
    zMath::g_currentMatrixPtrSlot = oldMatrixPtrSlot;

    return impulseApplied && stackRestored ? 0 : 2;
}

extern "C" int player_record_recent_hit_feedback_smoke(void) {
    zEffectAnimEntry *const oldRecentHitFxAnimEntry = g_PlayerRecentHitFxAnimEntry;
    void *const oldOwnerOrCtx = g_OptCatalog_CurrentDamageOwnerOrCtx;
    const float oldTime = g_Time_AccumulatedTimeSec;
    const int oldQueueEnabled = g_zEffectAnim_RecordQueueEnabled;
    const int oldDispatchEnabled = g_zEffectAnim_DispatchEnabled;
    zEffectAnimActivationRecord *const oldRecordTable = g_zEffectAnim_ActivationRecordTable;
    const int oldRecordCapacity = g_zEffectAnim_ActivationRecordCapacity;
    const int oldRecordCount = g_zEffectAnim_ActivationRecordCount;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    zClass_NodePartial rootNode = {};
    rootNode.classId = 2;
    saveState.playerState = &playerState;
    playerState.rootNode = &rootNode;

    zClass_NodePartial runtimeNode = {};
    zEffectAnimEntry recentHitEffect = {};
    std::strcpy(recentHitEffect.name, "recent_hit");
    recentHitEffect.boundNode = &rootNode;
    recentHitEffect.runtimeNode = &runtimeNode;
    recentHitEffect.callbackNode = &rootNode;
    recentHitEffect.priority = 3;

    zClass_NodePartial oldRuntimeNode = {};
    zEffectAnimEntry oldHandle = {};
    oldHandle.activationState = 2;
    oldHandle.runtimeNode = &oldRuntimeNode;
    oldHandle.triggerBaseValue = 1.0f;
    oldHandle.triggerCurrentValue = 9.0f;
    playerState.recentHitLightHandle = &oldHandle;

    int owner = 0;
    OptCatalogEntryDef hitSource = {};
    g_PlayerRecentHitFxAnimEntry = &recentHitEffect;
    g_OptCatalog_CurrentDamageOwnerOrCtx = &owner;
    g_Time_AccumulatedTimeSec = 12.5f;
    g_zEffectAnim_RecordQueueEnabled = 1;
    g_zEffectAnim_DispatchEnabled = 0;
    zEffect_Anim::ClearActivationRecords();

    Player::RecordRecentHitFeedback(&saveState, &hitSource, 3.25f);

    const bool stateOk =
        playerState.recentHitValid == 1 && playerState.lastHitOwnerOrCtx == &owner &&
        playerState.recentHitSource == &hitSource && playerState.recentHitDamage == 3.25f &&
        playerState.recentHitFxExpireTime == 16.5f &&
        playerState.recentHitLightHandle == &recentHitEffect;
    const bool oldHandleStopped =
        oldHandle.triggerCurrentValue == 0.0f && oldRuntimeNode.actionCallback != nullptr;
    const bool effectOk =
        recentHitEffect.activationState == 2 &&
        recentHitEffect.resetScratch[0] ==
            static_cast<unsigned int>(reinterpret_cast<std::uintptr_t>(&rootNode));

    zEffect_Anim::ClearActivationRecords();
    g_zEffectAnim_ActivationRecordTable = oldRecordTable;
    g_zEffectAnim_ActivationRecordCapacity = oldRecordCapacity;
    g_zEffectAnim_ActivationRecordCount = oldRecordCount;
    g_zEffectAnim_RecordQueueEnabled = oldQueueEnabled;
    g_zEffectAnim_DispatchEnabled = oldDispatchEnabled;
    g_PlayerRecentHitFxAnimEntry = oldRecentHitFxAnimEntry;
    g_OptCatalog_CurrentDamageOwnerOrCtx = oldOwnerOrCtx;
    g_Time_AccumulatedTimeSec = oldTime;

    return stateOk && oldHandleStopped && effectOk ? 0 : 1;
}

void RECOIL_FASTCALL PlayerDestroyedEffectDoneCallback(zEffectAnimEntry *, void *, int) {
}

void InitDestroyedEffectEntry(zEffectAnimEntry *entry, zClass_NodePartial *boundNode,
                              zClass_NodePartial *runtimeNode, const char *name) {
    std::memset(entry, 0, sizeof(*entry));
    std::strcpy(entry->name, name);
    entry->boundNode = boundNode;
    entry->callbackNode = boundNode;
    entry->runtimeNode = runtimeNode;
    entry->priority = 3;
}

void InitObjectPositionNode(zClass_NodePartial *node, zClass_Object3DDataPartial *data,
                            float x, float y, float z) {
    std::memset(node, 0, sizeof(*node));
    std::memset(data, 0, sizeof(*data));
    node->classId = 5;
    node->classData = data;
    node->flags = 1;
    data->localMatrix[9] = x;
    data->localMatrix[10] = y;
    data->localMatrix[11] = z;
}

extern "C" int player_start_destroyed_state_vehicle_effect_smoke(void) {
    const int oldQueueEnabled = g_zEffectAnim_RecordQueueEnabled;
    const int oldDispatchEnabled = g_zEffectAnim_DispatchEnabled;
    zEffectAnimActivationRecord *const oldRecordTable = g_zEffectAnim_ActivationRecordTable;
    const int oldRecordCapacity = g_zEffectAnim_ActivationRecordCapacity;
    const int oldRecordCount = g_zEffectAnim_ActivationRecordCount;
    HudUiSlot *const oldTrackedProgressSlot = g_HudUiMgrSensorTrackedProgressSlot;
    const HudUiMeter oldSensorMeter = g_HudUiMgrSensorMeter;

    zClass_NodePartial rootNode = {};
    std::strcpy(rootNode.name, "destroyed_root");
    rootNode.classId = 2;

    HudUiMeter_FTable visibleTable = {};
    visibleTable.slots[24] = MethodAddress(&TestPlayerHudVisibleReceiver::SetVisible);
    HudUiMgrSensorTrackNode trackNode = {};
    HudUiSlot trackedSlot = {};
    trackedSlot.trackNode = &trackNode;

    std::memset(g_PlayerTestHudVisibleThis, 0, sizeof(g_PlayerTestHudVisibleThis));
    std::memset(g_PlayerTestHudVisibleValue, 0, sizeof(g_PlayerTestHudVisibleValue));
    g_PlayerTestHudVisibleCount = 0;
    g_HudUiMgrSensorMeter = {};
    g_HudUiMgrSensorMeter.ftable = &visibleTable;

    g_zEffectAnim_RecordQueueEnabled = 1;
    g_zEffectAnim_DispatchEnabled = 0;
    zEffect_Anim::ClearActivationRecords();

    bool ok = true;
    for (int scenario = 0; scenario < 5; ++scenario) {
        zUtil_SaveGameState saveState = {};
        zUtil_PlayerStateStorage playerState = {};
        saveState.playerState = &playerState;
        playerState.rootNode = &rootNode;

        zClass_NodePartial runtimeNodes[6] = {};
        zEffectAnimEntry napalm = {};
        zEffectAnimEntry shatter = {};
        zEffectAnimEntry shock = {};
        zEffectAnimEntry subTransition = {};
        zEffectAnimEntry destroyedRespawn = {};
        zEffectAnimEntry oldRecentHitHandle = {};
        InitDestroyedEffectEntry(&napalm, &rootNode, &runtimeNodes[0], "napalm_vehicle");
        InitDestroyedEffectEntry(&shatter, &rootNode, &runtimeNodes[1], "shatter_vehicle");
        InitDestroyedEffectEntry(&shock, &rootNode, &runtimeNodes[2], "shock_vehicle");
        InitDestroyedEffectEntry(&subTransition, &rootNode, &runtimeNodes[3], "sub_transition");
        InitDestroyedEffectEntry(&destroyedRespawn, &rootNode, &runtimeNodes[4], "destroyed_respawn");
        InitDestroyedEffectEntry(&oldRecentHitHandle, &rootNode, &runtimeNodes[5], "recent_hit");
        oldRecentHitHandle.activationState = 2;
        oldRecentHitHandle.triggerBaseValue = 1.0f;
        oldRecentHitHandle.triggerCurrentValue = 9.0f;

        playerState.napalmVehicleFxEntry = &napalm;
        playerState.shatterVehicleFxEntry = &shatter;
        playerState.shockVehicleFxEntry = &shock;
        playerState.subTransitionFxEntry = &subTransition;
        playerState.destroyedRespawnFxEntry = &destroyedRespawn;
        playerState.destroyedRespawnAsyncHandle = &oldRecentHitHandle;

        zEffectAnimEntry *expected = &destroyedRespawn;
        if (scenario == 0) {
            playerState.queuedFixedDamageFlag = 1;
            playerState.recentHitValid = 1;
            playerState.recentHitLightHandle = &oldRecentHitHandle;
            expected = &shock;
            trackNode.payload = &saveState;
            g_HudUiMgrSensorTrackedProgressSlot = &trackedSlot;
        } else if (scenario == 1) {
            playerState.damageProtectionActive = 1;
            expected = &shatter;
            g_HudUiMgrSensorTrackedProgressSlot = 0;
        } else if (scenario == 2) {
            playerState.recentHitValid = 1;
            playerState.recentHitLightHandle = &oldRecentHitHandle;
            expected = &napalm;
            g_HudUiMgrSensorTrackedProgressSlot = 0;
        } else if (scenario == 3) {
            playerState.aiMode = 1;
            expected = &subTransition;
            g_HudUiMgrSensorTrackedProgressSlot = 0;
        } else {
            g_HudUiMgrSensorTrackedProgressSlot = 0;
        }

        void *const callback = scenario == 0 ? (void *)(&PlayerDestroyedEffectDoneCallback) : 0;
        Player::StartDestroyedStateVehicleEffect(&saveState, callback);

        ok = ok && playerState.destroyedRespawnAsyncHandle == expected &&
             expected->activationState == 2 && expected->velocityX == 0.0f &&
             expected->velocityY == 0.0f && expected->velocityZ == 0.0f;
        if (scenario == 0 || scenario == 2) {
            ok = ok && playerState.recentHitValid == 0 && playerState.recentHitLightHandle == 0 &&
                 oldRecentHitHandle.triggerCurrentValue == 0.0f;
        }
        if (scenario == 0) {
            ok = ok && expected->eventCallback == &PlayerDestroyedEffectDoneCallback &&
                 expected->eventCallbackContext == &saveState && g_PlayerTestHudVisibleCount == 1 &&
                 g_PlayerTestHudVisibleThis[0] == &g_HudUiMgrSensorMeter &&
                 g_PlayerTestHudVisibleValue[0] == 0;
        }

        zEffect_Anim::ClearActivationRecords();
    }

    g_HudUiMgrSensorTrackedProgressSlot = oldTrackedProgressSlot;
    g_HudUiMgrSensorMeter = oldSensorMeter;
    g_zEffectAnim_ActivationRecordTable = oldRecordTable;
    g_zEffectAnim_ActivationRecordCapacity = oldRecordCapacity;
    g_zEffectAnim_ActivationRecordCount = oldRecordCount;
    g_zEffectAnim_RecordQueueEnabled = oldQueueEnabled;
    g_zEffectAnim_DispatchEnabled = oldDispatchEnabled;

    return ok ? 0 : 1;
}

extern "C" int player_update_timed_hit_status_from_source_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData masterCommonData = {};
    zClass_NodePartial lightNode = {};
    zClass_NodePartial lightParent = {};
    saveState.playerState = &playerState;
    playerState.masterCommonData = &masterCommonData;
    playerState.timedHitStatus.lightNode = &lightNode;
    playerState.timedHitStatus.lightParentNode = &lightParent;

    OptCatalogEntryDef fixedSource = {};
    fixedSource.flags = 0x800u;
    masterCommonData.maxHealth = 25.0f;
    const float fixedReturn =
        Player::UpdateTimedHitStatusFromHitSource(&saveState, &fixedSource, 3.5f);
    const bool fixedOk =
        fixedReturn == 3.5f && playerState.timedHitStatus.hitSource == &fixedSource &&
        playerState.timedHitStatus.targetLevel == 1.0f;

    OptCatalogEntryDef scaledSource = {};
    masterCommonData.invMaxHealth = 0.25f;
    playerState.timedHitStatus = {};
    playerState.timedHitStatus.currentLevel = 0.25f;
    playerState.timedHitStatus.lightNode = &lightNode;
    playerState.timedHitStatus.lightParentNode = &lightParent;
    const float scaledReturn =
        Player::UpdateTimedHitStatusFromHitSource(&saveState, &scaledSource, 2.0f);
    const bool scaledOk =
        scaledReturn == 2.0f && playerState.timedHitStatus.hitSource == &scaledSource &&
        playerState.timedHitStatus.targetLevel == 0.5f;

    playerState.timedHitStatus = {};
    playerState.timedHitStatus.currentLevel = 0.0f;
    playerState.timedHitStatus.lightNode = &lightNode;
    playerState.timedHitStatus.lightParentNode = &lightParent;
    const float suppressedReturn =
        Player::UpdateTimedHitStatusFromHitSource(&saveState, &scaledSource, 2.0f);
    const bool suppressedOk =
        suppressedReturn == 0.0f && playerState.timedHitStatus.targetLevel == 0.5f;

    return fixedOk && scaledOk && suppressedOk ? 0 : 1;
}

extern "C" int player_hit_callback_record_context_and_timed_status_smoke(void) {
    int *const oldMatrixIdentityFlagSlot = zMath::g_currentMatrixIdentityFlagSlot;
    float **const oldMatrixPtrSlot = zMath::g_currentMatrixPtrSlot;
    const float oldDamageFeedbackScalar = g_OptCatalogDamageFeedbackIntensityScalar;
    const int oldDamageContextKind = g_OptCatalog_DamageContextKind;
    void *const oldDamageContextHitEvent = g_OptCatalog_DamageContextHitEvent;
    const zVec3 oldCapturedSourcePos = g_OptCatalog_CapturedDamageSourcePos;
    const zVec3 oldCapturedHitPos = g_OptCatalog_CapturedDamageHitPos;

    zUtil_SaveGameState inactiveSave = {};
    zUtil_PlayerStateStorage inactiveState = {};
    PlayerModalState inactiveModalState = {};
    PlayerMasterCommonData inactiveCommonData = {};
    PlayerMasterModalData inactiveModalData = {};
    inactiveSave.playerState = &inactiveState;
    inactiveSave.primaryModalState = &inactiveModalState;
    inactiveModalState.masterModalData = &inactiveModalData;
    inactiveState.masterCommonData = &inactiveCommonData;
    inactiveState.lifecycleState = 4;
    const int inactiveResult =
        Player::HitCallback_RecordContextAndTimedStatus(&inactiveSave, nullptr, nullptr, 5.0f);
    const bool inactiveOk = inactiveResult == 0;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterCommonData commonData = {};
    PlayerMasterModalData modalData = {};
    zClass_NodePartial rootNode = {};
    zClass_Object3DDataPartial rootData = {};
    zClass_NodePartial hitNode = {};
    OptCatalogEntryDef hitSource = {};
    int matrixFlags[3] = {};
    float *matrixSlots[3] = {};
    zMat4x3 currentMatrix = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    playerState.masterCommonData = &commonData;
    commonData.maxHealth = 100.0f;
    commonData.invMaxHealth = 0.01f;
    modalData.invMass = 2.0f;
    playerState.lifecycleState = 1;
    playerState.statusMeterValue = 50.0f;
    playerState.localVel = {5.0f, 0.0f, 2.0f};
    playerState.rootNode = &rootNode;
    playerState.selectedProbeSample.node = &hitNode;
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    rootData.localMatrix[0] = 1.0f;
    rootData.localMatrix[4] = 1.0f;
    rootData.localMatrix[8] = 1.0f;

    matrixSlots[0] = (float *)(&currentMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    zMath::MatLoadIdentity();

    g_OptCatalogDamageFeedbackIntensityScalar = 0.0f;
    g_OptCatalog_DamageContextKind = -1;
    g_OptCatalog_DamageContextHitEvent = nullptr;
    g_OptCatalog_CapturedDamageSourcePos = {10.0f, 0.0f, 0.0f};
    g_OptCatalog_CapturedDamageHitPos = {0.0f, 0.0f, 0.0f};

    const int liveResult =
        Player::HitCallback_RecordContextAndTimedStatus(&saveState, &hitSource, &hitNode, 10.0f);
    const bool liveDamageOk =
        liveResult == 0 && FloatNear(playerState.statusMeterValue, 40.0f) &&
        FloatNear(playerState.statusMeterScaled, 0.4f) &&
        FloatNear(g_OptCatalogDamageFeedbackIntensityScalar, 0.4f) &&
        g_OptCatalog_DamageContextKind == 0 &&
        g_OptCatalog_DamageContextHitEvent == &playerState.selectedProbeSample &&
        FloatNear(playerState.vehiclePitchRad, 0.0f) &&
        FloatNear(playerState.vehicleRollRad, 0.5f) &&
        FloatNear(playerState.localVel.x, -28.340002f) &&
        FloatNear(playerState.localVel.z, 2.0f) &&
        zMath::g_currentMatrixIdentityFlagSlot == &matrixFlags[0] &&
        zMath::g_currentMatrixPtrSlot == &matrixSlots[0];

    zMath::g_currentMatrixIdentityFlagSlot = oldMatrixIdentityFlagSlot;
    zMath::g_currentMatrixPtrSlot = oldMatrixPtrSlot;
    g_OptCatalog_CapturedDamageSourcePos = oldCapturedSourcePos;
    g_OptCatalog_CapturedDamageHitPos = oldCapturedHitPos;
    g_OptCatalog_DamageContextHitEvent = oldDamageContextHitEvent;
    g_OptCatalog_DamageContextKind = oldDamageContextKind;
    g_OptCatalogDamageFeedbackIntensityScalar = oldDamageFeedbackScalar;

    return inactiveOk && liveDamageOk ? 0 : 1;
}

extern "C" int player_enter_destroyed_state_smoke(void) {
    int *const oldMatrixIdentityFlagSlot = zMath::g_currentMatrixIdentityFlagSlot;
    float **const oldMatrixPtrSlot = zMath::g_currentMatrixPtrSlot;
    const int oldDamageContextKind = g_OptCatalog_DamageContextKind;
    void *const oldDamageContextHitEvent = g_OptCatalog_DamageContextHitEvent;
    const zVec3 oldCapturedSourcePos = g_OptCatalog_CapturedDamageSourcePos;
    const zVec3 oldCapturedHitPos = g_OptCatalog_CapturedDamageHitPos;
    const int oldDamageMaskEnabled = g_OptCatalogDamageMaskEnabled;
    int *const oldJoystickOption = ZOPT_INPUT_JOYSTICK;
    const int oldForceFeedbackCaps = g_zInput_JoystickCaps_ForceFeedback;
    zInput_FFEffectSet *const oldEffectSet = g_zInputFfEffectSet;
    HudUiShieldMessageWidget *const oldShieldWidget = g_HudUiMgrShieldMessageWidget;
    const std::uint32_t oldInvalidateMask = g_HudUi_InvalidateMask;

    zUtil_SaveGameState inactiveSave = {};
    zUtil_PlayerStateStorage inactiveState = {};
    PlayerModalState inactiveModalState = {};
    PlayerMasterCommonData inactiveCommonData = {};
    PlayerMasterModalData inactiveModalData = {};
    inactiveSave.playerState = &inactiveState;
    inactiveSave.primaryModalState = &inactiveModalState;
    inactiveModalState.masterModalData = &inactiveModalData;
    inactiveState.masterCommonData = &inactiveCommonData;
    inactiveState.lifecycleState = 4;
    inactiveState.statusMeterValue = 25.0f;
    const int inactiveResult =
        Player::EnterDestroyedState(&inactiveSave, nullptr, nullptr, 10.0f);
    int failureCode = 0;
    if (inactiveResult != 0 || !FloatNear(inactiveState.statusMeterValue, 25.0f)) {
        failureCode = 1;
    }

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterCommonData commonData = {};
    PlayerMasterModalData modalData = {};
    HudUiShieldMessageWidget shield = {};
    zClass_NodePartial rootNode = {};
    zClass_Object3DDataPartial rootData = {};
    zClass_NodePartial hitNode = {};
    OptCatalogEntryDef hitSource = {};
    OptCatalogHitEventPartial hitRenderPoint = {};
    int matrixFlags[3] = {};
    float *matrixSlots[3] = {};
    zMat4x3 currentMatrix = {};
    int joystickEnabled = 0;

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    playerState.masterCommonData = &commonData;
    commonData.maxHealth = 100.0f;
    commonData.invMaxHealth = 0.01f;
    modalData.invMass = 2.0f;
    hitSource.flags = 2;
    shield.meter.ftable = &g_HudUiMeter_FTable;
    shield.meter.fillPixelsMax = 20;
    shield.meter.points[1].y = 100.0f;
    reinterpret_cast<HudUiPanel *>(&shield.percentTextPanel)->vtbl =
        &g_HudUiPanelSimple_FTable;
    playerState.lifecycleState = 1;
    playerState.statusMeterValue = 50.0f;
    playerState.statusMeterScaled = 0.5f;
    playerState.localVel = {5.0f, 0.0f, 2.0f};
    playerState.rootNode = &rootNode;
    playerState.selectedProbeSample.node = &hitNode;
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    rootData.localMatrix[0] = 1.0f;
    rootData.localMatrix[4] = 1.0f;
    rootData.localMatrix[8] = 1.0f;

    matrixSlots[0] = (float *)(&currentMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    zMath::MatLoadIdentity();

    g_OptCatalog_DamageContextKind = -1;
    g_OptCatalog_DamageContextHitEvent = nullptr;
    g_OptCatalog_CapturedDamageSourcePos = {10.0f, 0.0f, 0.0f};
    g_OptCatalog_CapturedDamageHitPos = {0.0f, 0.0f, 0.0f};
    g_OptCatalogDamageMaskEnabled = 0;
    ZOPT_INPUT_JOYSTICK = &joystickEnabled;
    g_zInput_JoystickCaps_ForceFeedback = 0;
    g_zInputFfEffectSet = nullptr;
    g_HudUiMgrShieldMessageWidget = &shield;
    g_HudUi_InvalidateMask = 0;

    if (failureCode == 0) {
        const int liveResult =
            Player::EnterDestroyedState(&saveState, &hitSource, nullptr, 10.0f);
        if (liveResult != 0) {
            failureCode = 2;
        } else if (!FloatNear(playerState.statusMeterValue, 40.0f)) {
            failureCode = 3;
        } else if (!FloatNear(playerState.statusMeterScaled, 1.0f)) {
            failureCode = 4;
        } else if (!FloatNear(g_PlayerStatusMeterRatio, 0.4f)) {
            failureCode = 5;
        } else if (g_OptCatalog_DamageContextKind != 0 ||
                   g_OptCatalog_DamageContextHitEvent != &playerState.selectedProbeSample) {
            failureCode = 6;
        } else if (!FloatNear(playerState.vehiclePitchRad, 0.0f)) {
            failureCode = 7;
        } else if (!FloatNear(playerState.vehicleRollRad, 0.0f)) {
            failureCode = 8;
        } else if (!FloatNear(playerState.localVel.x, 5.0f)) {
            failureCode = 9;
        } else if (!FloatNear(playerState.localVel.z, 2.0f)) {
            failureCode = 10;
        } else if (zMath::g_currentMatrixIdentityFlagSlot != &matrixFlags[0] ||
                   zMath::g_currentMatrixPtrSlot != &matrixSlots[0]) {
            failureCode = 11;
        }
    }

    g_zInputFfEffectSet = oldEffectSet;
    g_zInput_JoystickCaps_ForceFeedback = oldForceFeedbackCaps;
    ZOPT_INPUT_JOYSTICK = oldJoystickOption;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    g_HudUiMgrShieldMessageWidget = oldShieldWidget;
    g_OptCatalogDamageMaskEnabled = oldDamageMaskEnabled;
    g_OptCatalog_CapturedDamageSourcePos = oldCapturedSourcePos;
    g_OptCatalog_CapturedDamageHitPos = oldCapturedHitPos;
    g_OptCatalog_DamageContextHitEvent = oldDamageContextHitEvent;
    g_OptCatalog_DamageContextKind = oldDamageContextKind;
    zMath::g_currentMatrixIdentityFlagSlot = oldMatrixIdentityFlagSlot;
    zMath::g_currentMatrixPtrSlot = oldMatrixPtrSlot;

    return failureCode;
}

extern "C" int player_clear_destroyed_respawn_effect_handle_callback_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    zEffectAnimEntry effect = {};
    zEffectAnimEntry handle = {};
    saveState.playerState = &playerState;
    playerState.destroyedRespawnAsyncHandle = &handle;

    Player::ClearDestroyedRespawnEffectHandleCallback(&effect, &saveState, 17);
    return playerState.destroyedRespawnAsyncHandle == nullptr ? 0 : 1;
}

extern "C" int player_apply_pending_collision_probe_velocity_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    PlayerPendingContact preferredContact = {};
    preferredContact.hit.surfaceNormal = {0.25f, 0.25f, -0.5f};
    playerState.preferredCollisionQueue.head = &preferredContact;
    playerState.projectileSpawnVel.y = 15.0f;
    playerState.previousTransform.posX = 4.0f;
    playerState.previousTransform.posY = 5.0f;
    playerState.previousTransform.posZ = 6.0f;
    playerState.vehiclePitchRad = 0.0f;
    playerState.cachedYawRad = 0.0f;
    playerState.vehicleRollRad = 0.0f;

    Player::ApplyPendingCollisionProbeVelocity(&saveState);
    if (!FloatNear(playerState.worldPos.x, 4.0f) ||
        !FloatNear(playerState.worldPos.y, 5.0f) ||
        !FloatNear(playerState.worldPos.z, 6.0f) ||
        !FloatNear(playerState.restartYawRad, 0.0f) ||
        !FloatNear(playerState.motionBasis.posX, 4.0f) ||
        !FloatNear(playerState.motionBasis.posY, 5.0f) ||
        !FloatNear(playerState.motionBasis.posZ, 6.0f)) {
        return 1;
    }
    if (!FloatNear(playerState.projectileSpawnVel.x, 5.0f) ||
        !FloatNear(playerState.projectileSpawnVel.y, 15.0f) ||
        !FloatNear(playerState.projectileSpawnVel.z, -10.0f) ||
        !FloatNear(playerState.localVel.x, 5.0f) ||
        !FloatNear(playerState.localVel.y, 15.0f) ||
        !FloatNear(playerState.localVel.z, -10.0f)) {
        return 2;
    }

    PlayerPendingContact playerContact = {};
    playerContact.hit.surfaceNormal = {0.0f, -0.25f, 1.0f};
    playerState.preferredCollisionQueue.head = 0;
    playerState.playerCollisionQueue.head = &playerContact;
    playerState.collisionProbeResolved = 1;
    playerState.projectileSpawnVel.y = -15.0f;
    playerState.motionBasis = {1.0f, 0.0f, 0.0f,
                               0.0f, 1.0f, 0.0f,
                               0.0f, 0.0f, 1.0f,
                               20.0f, 30.0f, 40.0f};

    Player::ApplyPendingCollisionProbeVelocity(&saveState);
    if (!FloatNear(playerState.projectileSpawnVel.x, 0.0f) ||
        !FloatNear(playerState.projectileSpawnVel.y, -15.0f) ||
        !FloatNear(playerState.projectileSpawnVel.z, 20.0f) ||
        !FloatNear(playerState.localVel.x, 0.0f) ||
        !FloatNear(playerState.localVel.y, -15.0f) ||
        !FloatNear(playerState.localVel.z, 20.0f)) {
        return 3;
    }

    playerState.playerCollisionQueue.head = 0;
    playerState.localVel = {1.0f, 2.0f, 3.0f};
    Player::ApplyPendingCollisionProbeVelocity(&saveState);
    return FloatNear(playerState.localVel.x, 1.0f) &&
                   FloatNear(playerState.localVel.y, 2.0f) &&
                   FloatNear(playerState.localVel.z, 3.0f)
               ? 0
               : 4;
}

extern "C" int player_vec3_fast_normalize_smoke(void) {
    const float oldScale = g_Player_CollisionContactResolveScale;
    g_Player_CollisionContactResolveScale = 0.2f;

    zVec3 vec = {0.03f, 0.04f, 0.0f};
    const float lengthSq = vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
    const float expectedScale = PlayerFastNormalizeScale(lengthSq);
    const int normalized = Player::Vec3_FastNormalize(&vec);
    if (normalized != 1 || !FloatNear(vec.x, 0.03f * expectedScale) ||
        !FloatNear(vec.y, 0.04f * expectedScale) || vec.z != 0.0f) {
        g_Player_CollisionContactResolveScale = oldScale;
        return 1;
    }

    zVec3 atThreshold = {0.1f, 0.0f, 0.0f};
    if (Player::Vec3_FastNormalize(&atThreshold) != 0 || atThreshold.x != 0.1f ||
        atThreshold.y != 0.0f || atThreshold.z != 0.0f) {
        g_Player_CollisionContactResolveScale = oldScale;
        return 2;
    }

    zVec3 zero = {};
    const int zeroResult = Player::Vec3_FastNormalize(&zero);
    g_Player_CollisionContactResolveScale = oldScale;
    return zeroResult == 0 && zero.x == 0.0f && zero.y == 0.0f && zero.z == 0.0f ? 0 : 3;
}

extern "C" int player_constrain_to_unit_distance_from_smoke(void) {
    const float oldScale = g_Player_CollisionContactResolveScale;
    g_Player_CollisionContactResolveScale = 0.2f;

    const zVec3 center = {10.0f, 20.0f, 30.0f};
    zVec3 pos = {10.03f, 20.04f, 30.0f};
    const float expectedScale = PlayerFastNormalizeScale(0.03f * 0.03f + 0.04f * 0.04f);
    Player::ConstrainToUnitDistanceFrom(&pos, &center);
    if (!FloatNear(pos.x, center.x + 0.03f * expectedScale) ||
        !FloatNear(pos.y, center.y + 0.04f * expectedScale) || pos.z != center.z) {
        g_Player_CollisionContactResolveScale = oldScale;
        return 1;
    }

    zVec3 farPos = {11.0f, 20.0f, 30.0f};
    Player::ConstrainToUnitDistanceFrom(&farPos, &center);
    g_Player_CollisionContactResolveScale = oldScale;
    return farPos.x == 11.0f && farPos.y == 20.0f && farPos.z == 30.0f ? 0 : 2;
}

extern "C" int hud_sensor_tracker_parse_checkpoint_number_from_node_smoke(void) {
    zClass_NodePartial node = {};
    zClass_NodePartial contextNode = {};
    node.flags = 0x200000;
    node.callbackContext = &contextNode;
    contextNode.auxFlags = 2;
    std::strcpy(contextNode.name, "checkpoint42");

    if (HudSensorTracker::ParseCheckpointNumberFromNode(&node) != 42) {
        return 1;
    }

    node.flags = 0;
    if (HudSensorTracker::ParseCheckpointNumberFromNode(&node) != 0) {
        return 2;
    }

    node.flags = 0x200000;
    contextNode.auxFlags = 0;
    if (HudSensorTracker::ParseCheckpointNumberFromNode(&node) != 0) {
        return 3;
    }

    contextNode.auxFlags = 2;
    std::strcpy(contextNode.name, "checkpoint-5");
    if (HudSensorTracker::ParseCheckpointNumberFromNode(&node) != 0) {
        return 4;
    }

    std::strcpy(contextNode.name, "checkpoint");
    return HudSensorTracker::ParseCheckpointNumberFromNode(&node) == 0 ? 0 : 5;
}

extern "C" int player_start_modal_loop_sfx_handle_smoke(void) {
    TestDirectSoundBufferVTable vtable = {};
    vtable.GetStatus = &TestDirectSoundGetStatus;
    vtable.Play = &TestDirectSoundPlay;
    vtable.SetCurrentPosition = &TestDirectSoundSetInt;
    vtable.SetVolume = &TestDirectSoundSetInt;
    TestDirectSoundBuffer directSoundBuffer = {&vtable};

    float globalVolume = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_MuteDepth = 0;
    g_zSnd_Flag10PlaybackEnabled = 1;

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    sample.replayFields.gain = 0.5f;
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);

    PlayerMasterModalData modalData = {};
    modalData.sfxEngine[1] = &sample;
    PlayerModalState modalState = {};
    modalState.masterModalData = &modalData;
    zUtil_PlayerStateStorage playerState = {};
    playerState.worldPos = {2.0f, 3.0f, 4.0f};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    saveState.StartModalLoopSfxHandle(1, 0.25f);

    g_zSnd_GlobalVolumeScalePtr = nullptr;
    g_zSnd_IsInitialized = 0;
    g_zSnd_PreInitialized = 0;

    if (modalState.modalSfxHandle[1] == nullptr) {
        return 1;
    }
    if (modalState.modalSfxHandle[1] != &sample.primaryVoice) {
        return 2;
    }

    return sample.primaryVoice.ownerSample == &sample ? 0 : 3;
}

extern "C" int player_start_master_type_loop_sfx_handle_smoke(void) {
    TestDirectSoundBufferVTable vtable = {};
    vtable.GetStatus = &TestDirectSoundGetStatus;
    vtable.Play = &TestDirectSoundPlay;
    vtable.SetCurrentPosition = &TestDirectSoundSetInt;
    vtable.SetVolume = &TestDirectSoundSetInt;
    TestDirectSoundBuffer directSoundBuffer = {&vtable};

    float globalVolume = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_MuteDepth = 0;
    g_zSnd_Flag10PlaybackEnabled = 1;

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    sample.replayFields.gain = 0.5f;
    sample.markerBaseTime = 12.0f;
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);

    PlayerMasterCommonData commonData = {};
    commonData.sfxWeaponUp[3] = &sample;
    zUtil_PlayerStateStorage playerState = {};
    playerState.masterCommonData = &commonData;
    playerState.worldPos = {2.0f, 3.0f, 4.0f};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;

    zSndPlayHandle *const handle = saveState.StartMasterTypeLoopSfxHandle(3, 0.25f);

    g_zSnd_GlobalVolumeScalePtr = nullptr;
    g_zSnd_IsInitialized = 0;
    g_zSnd_PreInitialized = 0;

    if (handle == nullptr || handle != &sample.primaryVoice) {
        return 1;
    }
    if (playerState.modeLoopSfxHandle[3] != handle || sample.primaryVoice.ownerSample != &sample) {
        return 2;
    }

    return sample.markerBaseTime == 0.0f ? 0 : 3;
}

extern "C" int player_ensure_master_type_loop_sfx_handle_smoke(void) {
    TestDirectSoundBufferVTable vtable = {};
    vtable.GetStatus = &TestDirectSoundGetStatus;
    vtable.Play = &TestDirectSoundPlay;
    vtable.SetCurrentPosition = &TestDirectSoundSetInt;
    vtable.SetVolume = &TestDirectSoundSetInt;
    TestDirectSoundBuffer directSoundBuffer = {&vtable};

    float globalVolume = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_MuteDepth = 0;
    g_zSnd_Flag10PlaybackEnabled = 1;
    g_PlayerTestPlayCount = 0;

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    sample.replayFields.gain = 0.5f;
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);

    PlayerMasterCommonData commonData = {};
    zUtil_PlayerStateStorage playerState = {};
    playerState.masterCommonData = &commonData;
    playerState.worldPos = {2.0f, 3.0f, 4.0f};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;

    zSndPlayHandle existingHandle = {};
    playerState.modeLoopSfxHandle[1] = &existingHandle;
    commonData.sfxWeaponUp[1] = &sample;
    saveState.EnsureMasterTypeLoopSfxHandle(1, 0.25f);
    if (playerState.modeLoopSfxHandle[1] != &existingHandle || g_PlayerTestPlayCount != 0) {
        return 1;
    }

    saveState.EnsureMasterTypeLoopSfxHandle(2, 0.5f);
    if (playerState.modeLoopSfxHandle[2] != nullptr || g_PlayerTestPlayCount != 0) {
        return 2;
    }

    commonData.sfxWeaponUp[2] = &sample;
    saveState.EnsureMasterTypeLoopSfxHandle(2, 0.75f);

    g_zSnd_GlobalVolumeScalePtr = nullptr;
    g_zSnd_IsInitialized = 0;
    g_zSnd_PreInitialized = 0;

    return playerState.modeLoopSfxHandle[2] == &sample.primaryVoice &&
                   sample.primaryVoice.ownerSample == &sample && g_PlayerTestPlayCount == 1
               ? 0
               : 3;
}

extern "C" int player_stop_master_type_loop_sfx_handle_smoke(void) {
    TestDirectSoundBufferVTable vtable = {};
    vtable.GetStatus = &TestDirectSoundGetStatus;
    vtable.Stop = &TestPlayerDirectSoundStop;
    TestDirectSoundBuffer directSoundBuffer = {&vtable};

    const int oldInitialized = g_zSnd_IsInitialized;
    const int oldPreInitialized = g_zSnd_PreInitialized;
    const int oldBackend = g_zSnd_ActiveBackend;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_PlayerTestStopCount = 0;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    zSndPlayHandle handle = {};
    handle.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);
    saveState.playerState = &playerState;
    playerState.modeLoopSfxHandle[2] = &handle;

    saveState.StopMasterTypeLoopSfxHandle(2);
    const bool stopped = playerState.modeLoopSfxHandle[2] == nullptr &&
                         g_PlayerTestStopCount == 1;

    saveState.StopMasterTypeLoopSfxHandle(1);
    const bool nullSlotOk = playerState.modeLoopSfxHandle[1] == nullptr &&
                            g_PlayerTestStopCount == 1;

    g_zSnd_IsInitialized = oldInitialized;
    g_zSnd_PreInitialized = oldPreInitialized;
    g_zSnd_ActiveBackend = oldBackend;

    return stopped && nullSlotOk ? 0 : 1;
}

extern "C" int player_stop_modal_loop_sfx_handle_smoke(void) {
    zUtil_SaveGameState saveState = {};
    PlayerModalState modalState = {};
    zSndPlayHandle handle = {};
    saveState.primaryModalState = &modalState;
    modalState.modalSfxHandle[2] = &handle;

    saveState.StopModalLoopSfxHandle(2);
    if (modalState.modalSfxHandle[2] != nullptr) {
        return 1;
    }

    saveState.StopModalLoopSfxHandle(1);
    return modalState.modalSfxHandle[1] == nullptr ? 0 : 2;
}

extern "C" int player_update_modal_loop_sfx_smoke(void) {
    const int oldBackend = g_zSnd_ActiveBackend;
    void *const oldGlobalVolume = g_zSnd_GlobalVolumeScalePtr;
    const int oldMuteDepth = g_zSnd_MuteDepth;
    const int oldListenerValid = g_zSnd_ListenerStateValid;
    const int oldInitialized = g_zSnd_IsInitialized;
    const int oldPreInitialized = g_zSnd_PreInitialized;

    TestDirectSoundBufferVTable vtable = {};
    vtable.SetPan = &TestPlayerDirectSoundSetPan;
    vtable.SetVolume = &TestPlayerDirectSoundSetVolume;
    vtable.SetFrequency = &TestPlayerDirectSoundSetFrequency;
    TestDirectSoundBuffer directSoundBuffer = {&vtable};

    float globalVolume = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_MuteDepth = 0;
    g_zSnd_ListenerStateValid = 0;
    g_PlayerTestSetFrequencyCount = 0;
    g_PlayerTestLastFrequency = 0;
    g_PlayerTestSetPanCount = 0;
    g_PlayerTestSetVolumeCount = 0;
    g_PlayerTestLastVolume = 0;

    zSndSample engineSample = {};
    zSndSample externalSample = {};
    zSndSample idleSample = {};
    zSndSample *samples[3] = {&engineSample, &externalSample, &idleSample};
    for (int index = 0; index < 3; ++index) {
        samples[index]->replayFields.flags = 4;
        samples[index]->playbackParam2 = 2000.0f;
        samples[index]->playbackParam3 = 1000.0f;
        samples[index]->sampleRate = 2000.0f;
        samples[index]->primaryVoice.handleKind = ZSND_PLAYHANDLE_BACKEND;
        samples[index]->primaryVoice.backendBuffer =
            reinterpret_cast<zSndBuffer *>(&directSoundBuffer);
        samples[index]->primaryVoice.ownerSample = samples[index];
    }

    PlayerMasterModalData modalData = {};
    modalData.maxSpeed = 10.0f;
    modalData.sfxPitchScale = 0.8f;
    modalData.sfxVolumeScale = 0.6f;
    PlayerModalState modalState = {};
    modalState.masterModalData = &modalData;
    modalState.modalSfxHandle[0] = &engineSample.primaryVoice;
    modalState.modalSfxHandle[1] = &externalSample.primaryVoice;
    modalState.modalSfxHandle[2] = &idleSample.primaryVoice;

    zUtil_PlayerStateStorage playerState = {};
    playerState.localVel.z = -5.0f;
    playerState.worldPos = {3.0f, 4.0f, 5.0f};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    saveState.modeLoopBlend = 0.25f;

    saveState.UpdateModalLoopSfx(1);
    const bool updateOk = FloatNear(saveState.modeLoopBlend, 0.5f) &&
                          g_PlayerTestSetFrequencyCount == 2 &&
                          g_PlayerTestLastFrequency == 1400 &&
                          g_PlayerTestSetPanCount == 6 &&
                          g_PlayerTestSetVolumeCount == 6 &&
                          g_PlayerTestLastVolume == -602 &&
                          engineSample.primaryVoice.gainScaled == 0 &&
                          externalSample.primaryVoice.gainScaled == -602 &&
                          idleSample.primaryVoice.gainScaled == -602;

    g_zSnd_IsInitialized = 0;
    g_zSnd_PreInitialized = 0;
    saveState.UpdateModalLoopSfx(0);
    const bool disabledOk = modalState.modalSfxHandle[0] == nullptr &&
                            modalState.modalSfxHandle[1] == nullptr &&
                            modalState.modalSfxHandle[2] == nullptr &&
                            playerState.modeLoopSfxHandle[3] == nullptr;

    g_zSnd_ActiveBackend = oldBackend;
    g_zSnd_GlobalVolumeScalePtr = oldGlobalVolume;
    g_zSnd_MuteDepth = oldMuteDepth;
    g_zSnd_ListenerStateValid = oldListenerValid;
    g_zSnd_IsInitialized = oldInitialized;
    g_zSnd_PreInitialized = oldPreInitialized;

    return updateOk && disabledOk ? 0 : 1;
}

extern "C" int player_select_modal_state_by_master_type_smoke(void) {
    zUtil_SaveGameState saveState = {};

    if (saveState.SelectModalStateByMasterType(7) != 0) {
        return 1;
    }

    PlayerMasterModalData firstData = {};
    firstData.masterType = 3;
    PlayerMasterModalData secondData = {};
    secondData.masterType = 7;
    PlayerMasterModalData thirdData = {};
    thirdData.masterType = 9;

    PlayerModalState firstModal = {};
    firstModal.masterModalData = &firstData;
    PlayerModalState secondModal = {};
    secondModal.masterModalData = &secondData;
    PlayerModalState thirdModal = {};
    thirdModal.masterModalData = &thirdData;
    firstModal.next = &secondModal;
    secondModal.next = &thirdModal;

    zSndPlayHandle handle0 = {};
    zSndPlayHandle handle1 = {};
    zSndPlayHandle handle2 = {};
    firstModal.modalSfxHandle[0] = &handle0;
    firstModal.modalSfxHandle[1] = &handle1;
    firstModal.modalSfxHandle[2] = &handle2;

    saveState.primaryModalState = &firstModal;
    saveState.modalStateListHead = &firstModal;

    if (saveState.SelectModalStateByMasterType(8) != 0) {
        return 2;
    }
    if (saveState.primaryModalState != &firstModal) {
        return 3;
    }

    if (saveState.SelectModalStateByMasterType(7) != 1) {
        return 4;
    }
    if (saveState.primaryModalState != &secondModal) {
        return 5;
    }

    return firstModal.modalSfxHandle[0] == nullptr &&
                   firstModal.modalSfxHandle[1] == nullptr &&
                   firstModal.modalSfxHandle[2] == nullptr
               ? 0
               : 6;
}

extern "C" int player_master_type_transition_leaf_smoke(void) {
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    zEffectAnimEntry transitionHandle = {};
    playerState.masterTypeTransitionToSubLightHandle = &transitionHandle;
    Player::StopBftBubbleFxHandle(&saveState);
    if (playerState.masterTypeTransitionToSubLightHandle != nullptr) {
        g_Time_AccumulatedTimeSec = oldAccumulatedTime;
        return 1;
    }

    Player::StopBftBubbleFxHandle(&saveState);

    PlayerMasterModalData subData = {};
    subData.masterType = 2;
    PlayerMasterModalData flyData = {};
    flyData.masterType = 1;
    PlayerModalState subModal = {};
    subModal.masterModalData = &subData;
    PlayerModalState flyModal = {};
    flyModal.masterModalData = &flyData;
    subModal.next = &flyModal;

    saveState.primaryModalState = &subModal;
    saveState.modalStateListHead = &subModal;
    playerState.masterTypeTransitionCooldownUntilTime = 9.0f;
    playerState.currentMasterType = 77;
    playerState.damageVisualFlag = 0;
    g_Time_AccumulatedTimeSec = 10.0f;

    const int flyResult = Player::TransitionToMasterTypeFly(&saveState, 0);
    if (flyResult != 1 || saveState.primaryModalState != &flyModal ||
        playerState.currentMasterType != 2 || playerState.damageVisualFlag != 1 ||
        !FloatNear(playerState.masterTypeTransitionCooldownUntilTime, 15.0f)) {
        g_Time_AccumulatedTimeSec = oldAccumulatedTime;
        return 2;
    }

    saveState.primaryModalState = &subModal;
    playerState.masterTypeTransitionCooldownUntilTime = 20.0f;
    playerState.currentMasterType = 44;
    playerState.damageVisualFlag = 0;
    g_Time_AccumulatedTimeSec = 12.0f;

    const int blockedResult = Player::TransitionToMasterTypeFly(&saveState, 0);
    const bool blockedOk = blockedResult == 0 && saveState.primaryModalState == &subModal &&
                           playerState.currentMasterType == 44 &&
                           playerState.damageVisualFlag == 0 &&
                           FloatNear(playerState.masterTypeTransitionCooldownUntilTime, 20.0f);

    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    return blockedOk ? 0 : 3;
}

extern "C" int player_apply_master_type_transition_smoke(void) {
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState sourceModal = {};
    PlayerModalState flyModal = {};
    PlayerMasterModalData sourceData = {};
    PlayerMasterModalData flyData = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &sourceModal;
    saveState.modalStateListHead = &sourceModal;
    sourceModal.masterModalData = &sourceData;
    sourceModal.next = &flyModal;
    flyModal.masterModalData = &flyData;
    sourceData.masterType = 2;
    flyData.masterType = 1;

    g_Time_AccumulatedTimeSec = 12.0f;
    playerState.primaryGunGateUntilTime = 99.0f;
    const int invalidResult = Player::ApplyMasterTypeTransition(&saveState, 0, 7);
    const bool invalidOk = invalidResult == -1 && playerState.primaryGunGateUntilTime == 12.0f &&
                           saveState.primaryModalState == &sourceModal;

    g_Time_AccumulatedTimeSec = 20.0f;
    playerState.primaryGunGateUntilTime = 88.0f;
    playerState.masterTypeTransitionCooldownUntilTime = 0.0f;
    playerState.currentMasterType = 77;
    playerState.damageVisualFlag = 0;
    const int flyResult = Player::ApplyMasterTypeTransition(&saveState, 1, 0);
    const bool flyOk = flyResult == 1 && saveState.primaryModalState == &flyModal &&
                       playerState.currentMasterType == 2 && playerState.damageVisualFlag == 1 &&
                       FloatNear(playerState.masterTypeTransitionCooldownUntilTime, 25.0f);

    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    return invalidOk && flyOk ? 0 : 1;
}

extern "C" int player_transition_to_master_type_track_smoke(void) {
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const HudUiElement oldUnderwaterFxPass3Ui = g_Player_UnderwaterFxPass3Ui;
    const int oldHorizonEnabled = g_Player_HorizonNodeFollowCameraEnabled;
    zClass_NodePartial *const oldHealthy1 = g_Player_CopterHealthyNode1;
    zClass_NodePartial *const oldHealthy2 = g_Player_CopterHealthyNode2;
    zClass_NodePartial *const oldSnd1 = g_Player_CopterSndNode1;
    zClass_NodePartial *const oldSnd2 = g_Player_CopterSndNode2;
    zSndSample *const oldCopterSample = g_Player_CopterSndSample;

    HudUiCommon_FTable visibleTable = {};
    visibleTable.slots[24] = MethodAddress(&TestPlayerHudVisibleReceiver::SetVisible);
    g_Player_UnderwaterFxPass3Ui = {};
    g_Player_UnderwaterFxPass3Ui.ftable = &visibleTable;
    std::memset(g_PlayerTestHudVisibleThis, 0, sizeof(g_PlayerTestHudVisibleThis));
    std::memset(g_PlayerTestHudVisibleValue, 0, sizeof(g_PlayerTestHudVisibleValue));
    g_PlayerTestHudVisibleCount = 0;
    g_Player_HorizonNodeFollowCameraEnabled = 0;
    g_Player_CopterHealthyNode1 = nullptr;
    g_Player_CopterHealthyNode2 = nullptr;
    g_Player_CopterSndNode1 = nullptr;
    g_Player_CopterSndNode2 = nullptr;
    g_Player_CopterSndSample = nullptr;
    g_GameStateOrMapTable = nullptr;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState sourceModal = {};
    PlayerModalState trackModal = {};
    PlayerMasterModalData sourceData = {};
    PlayerMasterModalData trackData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial modeVariantNode = {};
    zClass_NodePartial attachNodes[4] = {};
    zClass_Object3DDataPartial attachData[4] = {};
    rootNode.classId = 2;
    modeVariantNode.classId = 2;

    saveState.playerState = &playerState;
    saveState.primaryModalState = &sourceModal;
    saveState.modalStateListHead = &sourceModal;
    sourceModal.masterModalData = &sourceData;
    sourceModal.next = &trackModal;
    trackModal.masterModalData = &trackData;
    trackData.masterType = 3;
    playerState.rootNode = &rootNode;
    playerState.modeVariantNode = &modeVariantNode;

    sourceData.masterType = 1;
    playerState.masterTypeTransitionCooldownUntilTime = 9.0f;
    playerState.currentMasterType = 77;
    g_Time_AccumulatedTimeSec = 10.0f;
    int result = Player::TransitionToMasterTypeTrack(&saveState, 0);
    if (result != 1 || saveState.primaryModalState != &trackModal ||
        playerState.currentMasterType != 1 ||
        !FloatNear(playerState.masterTypeTransitionCooldownUntilTime, 11.0f) ||
        (modeVariantNode.flags & 0x04) == 0) {
        result = 1;
        goto restore;
    }

    saveState.primaryModalState = &sourceModal;
    sourceData.masterType = 1;
    playerState.masterTypeTransitionCooldownUntilTime = 20.0f;
    playerState.currentMasterType = 44;
    modeVariantNode.flags = 0;
    g_Time_AccumulatedTimeSec = 12.0f;
    if (Player::TransitionToMasterTypeTrack(&saveState, 0) != 0 ||
        saveState.primaryModalState != &sourceModal || playerState.currentMasterType != 44 ||
        (modeVariantNode.flags & 0x04) != 0) {
        result = 2;
        goto restore;
    }

    sourceData.masterType = 4;
    playerState.autoTurnSign = -1;
    playerState.masterTypeTransitionCooldownUntilTime = 0.0f;
    g_Time_AccumulatedTimeSec = 12.0f;
    if (Player::TransitionToMasterTypeTrack(&saveState, 0) != 0) {
        result = 3;
        goto restore;
    }
    playerState.autoTurnSign = 0;

    sourceData.masterType = 2;
    if (Player::TransitionToMasterTypeTrack(&saveState, 0) != 0) {
        result = 4;
        goto restore;
    }

    for (int i = 0; i < 4; ++i) {
        InitObjectPositionNode(&attachNodes[i], &attachData[i], 10.0f + i, 20.0f + i,
                               30.0f + i);
    }
    playerState.altWeaponBanks[1].controllerA.attachNodePrimary = &attachNodes[0];
    playerState.altWeaponBanks[1].controllerA.attachNodeSecondary = &attachNodes[1];
    playerState.altWeaponBanks[1].controllerB.attachNodePrimary = &attachNodes[2];
    playerState.altWeaponBanks[1].controllerB.attachNodeSecondary = &attachNodes[3];
    saveState.primaryModalState = &sourceModal;
    playerState.damageVisualFlag = 0;
    playerState.currentMasterType = 55;
    playerState.masterTypeTransitionCooldownUntilTime = 0.0f;
    modeVariantNode.flags = 0;
    g_PlayerTestHudVisibleCount = 0;
    g_Player_HorizonNodeFollowCameraEnabled = 0;

    result = Player::TransitionToMasterTypeTrack(&saveState, 1);
    if (result != 1 || saveState.primaryModalState != &trackModal ||
        playerState.currentMasterType != 2 || playerState.damageVisualFlag != 1 ||
        g_Player_HorizonNodeFollowCameraEnabled != 1 || g_PlayerTestHudVisibleCount != 1 ||
        g_PlayerTestHudVisibleThis[0] != &g_Player_UnderwaterFxPass3Ui ||
        g_PlayerTestHudVisibleValue[0] != 0 ||
        !FloatNear(playerState.masterTypeTransitionCooldownUntilTime, 13.0f) ||
        (modeVariantNode.flags & 0x04) == 0) {
        result = 5;
        goto restore;
    }

    for (int i = 0; i < 4; ++i) {
        if (attachData[i].localMatrix[9] != 0.0f || attachData[i].localMatrix[10] != 0.0f ||
            attachData[i].localMatrix[11] != 0.0f) {
            result = 6;
            goto restore;
        }
    }

    result = 0;

restore:
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Player_UnderwaterFxPass3Ui = oldUnderwaterFxPass3Ui;
    g_Player_HorizonNodeFollowCameraEnabled = oldHorizonEnabled;
    g_Player_CopterHealthyNode1 = oldHealthy1;
    g_Player_CopterHealthyNode2 = oldHealthy2;
    g_Player_CopterSndNode1 = oldSnd1;
    g_Player_CopterSndNode2 = oldSnd2;
    g_Player_CopterSndSample = oldCopterSample;
    return result;
}

extern "C" int player_transition_to_master_type_amphib_smoke(void) {
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const HudUiElement oldUnderwaterFxPass3Ui = g_Player_UnderwaterFxPass3Ui;
    const int oldHorizonEnabled = g_Player_HorizonNodeFollowCameraEnabled;
    zClass_NodePartial *const oldHealthy1 = g_Player_CopterHealthyNode1;
    zClass_NodePartial *const oldHealthy2 = g_Player_CopterHealthyNode2;
    zClass_NodePartial *const oldSnd1 = g_Player_CopterSndNode1;
    zClass_NodePartial *const oldSnd2 = g_Player_CopterSndNode2;
    zSndSample *const oldCopterSample = g_Player_CopterSndSample;

    HudUiCommon_FTable visibleTable = {};
    visibleTable.slots[24] = MethodAddress(&TestPlayerHudVisibleReceiver::SetVisible);
    g_Player_UnderwaterFxPass3Ui = {};
    g_Player_UnderwaterFxPass3Ui.ftable = &visibleTable;
    std::memset(g_PlayerTestHudVisibleThis, 0, sizeof(g_PlayerTestHudVisibleThis));
    std::memset(g_PlayerTestHudVisibleValue, 0, sizeof(g_PlayerTestHudVisibleValue));
    g_PlayerTestHudVisibleCount = 0;
    g_Player_HorizonNodeFollowCameraEnabled = 0;
    g_Player_CopterHealthyNode1 = nullptr;
    g_Player_CopterHealthyNode2 = nullptr;
    g_Player_CopterSndNode1 = nullptr;
    g_Player_CopterSndNode2 = nullptr;
    g_Player_CopterSndSample = nullptr;
    g_GameStateOrMapTable = nullptr;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState sourceModal = {};
    PlayerModalState amphibModal = {};
    PlayerMasterModalData sourceData = {};
    PlayerMasterModalData amphibData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial modeVariantNode = {};
    zClass_NodePartial modalNode = {};
    zClass_Object3DDataPartial modalObjectData = {};
    rootNode.classId = 2;
    modeVariantNode.classId = 2;
    InitObjectPositionNode(&modalNode, &modalObjectData, 1.0f, 2.0f, 3.0f);

    saveState.playerState = &playerState;
    saveState.primaryModalState = &sourceModal;
    saveState.modalStateListHead = &sourceModal;
    sourceModal.masterModalData = &sourceData;
    sourceModal.modalNode = &modalNode;
    sourceModal.next = &amphibModal;
    amphibModal.masterModalData = &amphibData;
    amphibData.masterType = 5;
    playerState.rootNode = &rootNode;
    playerState.modeVariantNode = &modeVariantNode;

    sourceData.masterType = 3;
    playerState.amphibUnlocked = 1;
    playerState.masterTypeTransitionCooldownUntilTime = 20.0f;
    playerState.currentMasterType = 33;
    g_Time_AccumulatedTimeSec = 10.0f;
    int result = Player::TransitionToMasterTypeAmphib(&saveState, 0, 0);
    if (result != 0 || saveState.primaryModalState != &sourceModal ||
        playerState.currentMasterType != 33) {
        result = 1;
        goto restore;
    }

    playerState.masterTypeTransitionCooldownUntilTime = 0.0f;
    playerState.amphibUnlocked = 0;
    if (Player::TransitionToMasterTypeAmphib(&saveState, 0, 0) != 0) {
        result = 2;
        goto restore;
    }

    playerState.amphibUnlocked = 1;
    playerState.airborneFlag = 1;
    playerState.vehiclePitchRad = 2.5f;
    playerState.vehicleRollRad = -3.5f;
    modalObjectData.rotation.x = 4.0f;
    modalObjectData.rotation.y = 5.0f;
    modalObjectData.rotation.z = 6.0f;
    modeVariantNode.flags = 0;
    saveState.primaryModalState = &sourceModal;
    sourceData.masterType = 3;
    g_Time_AccumulatedTimeSec = 10.0f;

    result = Player::TransitionToMasterTypeAmphib(&saveState, 0, 0);
    if (result != 1 || saveState.primaryModalState != &amphibModal ||
        playerState.currentMasterType != 3 || playerState.airborneFlag != 0 ||
        !FloatNear(playerState.masterTypeTransitionCooldownUntilTime, 11.0f) ||
        (modeVariantNode.flags & 0x04) == 0 || playerState.vehiclePitchRad != 0.0f ||
        playerState.vehicleRollRad != 0.0f || modalObjectData.rotation.x != 0.0f ||
        modalObjectData.rotation.y != 0.0f || modalObjectData.rotation.z != 0.0f) {
        result = 3;
        goto restore;
    }

    saveState.primaryModalState = &sourceModal;
    sourceData.masterType = 2;
    playerState.masterTypeTransitionCooldownUntilTime = 0.0f;
    g_Time_AccumulatedTimeSec = 12.0f;
    if (Player::TransitionToMasterTypeAmphib(&saveState, 1, 0) != 0) {
        result = 4;
        goto restore;
    }

    playerState.damageVisualFlag = 0;
    playerState.currentMasterType = 44;
    modeVariantNode.flags = 0;
    g_PlayerTestHudVisibleCount = 0;
    g_Player_HorizonNodeFollowCameraEnabled = 0;
    result = Player::TransitionToMasterTypeAmphib(&saveState, 0, 1);
    if (result != 1 || saveState.primaryModalState != &amphibModal ||
        playerState.currentMasterType != 2 || playerState.damageVisualFlag != 1 ||
        g_Player_HorizonNodeFollowCameraEnabled != 1 || g_PlayerTestHudVisibleCount != 1 ||
        g_PlayerTestHudVisibleThis[0] != &g_Player_UnderwaterFxPass3Ui ||
        g_PlayerTestHudVisibleValue[0] != 0 ||
        !FloatNear(playerState.masterTypeTransitionCooldownUntilTime, 13.0f)) {
        result = 5;
        goto restore;
    }

    result = 0;

restore:
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Player_UnderwaterFxPass3Ui = oldUnderwaterFxPass3Ui;
    g_Player_HorizonNodeFollowCameraEnabled = oldHorizonEnabled;
    g_Player_CopterHealthyNode1 = oldHealthy1;
    g_Player_CopterHealthyNode2 = oldHealthy2;
    g_Player_CopterSndNode1 = oldSnd1;
    g_Player_CopterSndNode2 = oldSnd2;
    g_Player_CopterSndSample = oldCopterSample;
    return result;
}

extern "C" int player_transition_to_master_type_sub_smoke(void) {
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    g_GameStateOrMapTable = nullptr;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState sourceModal = {};
    PlayerModalState subModal = {};
    PlayerMasterModalData sourceData = {};
    PlayerMasterModalData subData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial modalNode = {};
    zClass_Object3DDataPartial modalObjectData = {};
    OptCatalogEntryDef allowedAltEntry = {};
    PlayerGunFireController activeAltController = {};
    rootNode.classId = 2;
    InitObjectPositionNode(&modalNode, &modalObjectData, 1.0f, 2.0f, 3.0f);

    saveState.playerState = &playerState;
    saveState.primaryModalState = &sourceModal;
    saveState.modalStateListHead = &sourceModal;
    sourceModal.masterModalData = &sourceData;
    sourceModal.modalNode = &modalNode;
    sourceModal.next = &subModal;
    subModal.masterModalData = &subData;
    subData.masterType = 2;
    playerState.rootNode = &rootNode;
    playerState.activeAltGunController = &activeAltController;
    activeAltController.optCatalogEntry = &allowedAltEntry;

    sourceData.masterType = 5;
    playerState.subUnlocked = 1;
    playerState.masterTypeTransitionCooldownUntilTime = 20.0f;
    playerState.currentMasterType = 33;
    playerState.damageVisualFlag = 0;
    g_Time_AccumulatedTimeSec = 10.0f;
    int result = Player::TransitionToMasterTypeSub(&saveState, 0);
    if (result != 0 || saveState.primaryModalState != &sourceModal ||
        playerState.currentMasterType != 33 || playerState.damageVisualFlag != 1) {
        result = 1;
        goto restore;
    }

    playerState.masterTypeTransitionCooldownUntilTime = 0.0f;
    playerState.subUnlocked = 0;
    if (Player::TransitionToMasterTypeSub(&saveState, 0) != 0) {
        result = 2;
        goto restore;
    }

    playerState.subUnlocked = 1;
    sourceData.masterType = 2;
    saveState.primaryModalState = &sourceModal;
    playerState.currentMasterType = 44;
    playerState.masterTypeTransitionCooldownUntilTime = 0.0f;
    g_Time_AccumulatedTimeSec = 12.0f;
    result = Player::TransitionToMasterTypeSub(&saveState, 0);
    if (result != 1 || saveState.primaryModalState != &sourceModal ||
        playerState.currentMasterType != 44 ||
        !FloatNear(playerState.masterTypeTransitionCooldownUntilTime, 0.0f)) {
        result = 3;
        goto restore;
    }

    sourceData.masterType = 3;
    saveState.primaryModalState = &sourceModal;
    playerState.currentMasterType = 55;
    if (Player::TransitionToMasterTypeSub(&saveState, 0) != 0) {
        result = 4;
        goto restore;
    }

    sourceData.masterType = 5;
    playerState.bankInput = 1;
    if (Player::TransitionToMasterTypeSub(&saveState, 0) != 0) {
        result = 5;
        goto restore;
    }

    saveState.primaryModalState = &sourceModal;
    sourceData.masterType = 5;
    playerState.bankInput = 0;
    playerState.currentMasterType = 77;
    playerState.localVel.y = 1.0f;
    playerState.worldPos.y = 20.0f;
    playerState.masterTypeTransitionCooldownUntilTime = 0.0f;
    g_Time_AccumulatedTimeSec = 14.0f;
    result = Player::TransitionToMasterTypeSub(&saveState, 0);
    if (result != 1 || saveState.primaryModalState != &subModal ||
        playerState.currentMasterType != 5 ||
        !FloatNear(playerState.masterTypeTransitionCooldownUntilTime, 15.0f) ||
        !FloatNear(playerState.localVel.y, -3.0f) ||
        !FloatNear(playerState.worldPos.y, 15.9f)) {
        result = 6;
        goto restore;
    }

    saveState.primaryModalState = &sourceModal;
    sourceData.masterType = 3;
    playerState.airborneFlag = 1;
    playerState.currentMasterType = 88;
    playerState.localVel.y = 2.0f;
    playerState.worldPos.y = 30.0f;
    playerState.masterTypeTransitionCooldownUntilTime = 0.0f;
    modalObjectData.rotation.x = 4.0f;
    modalObjectData.rotation.y = 5.0f;
    modalObjectData.rotation.z = 6.0f;
    g_Time_AccumulatedTimeSec = 16.0f;
    result = Player::TransitionToMasterTypeSub(&saveState, 1);
    if (result != 1 || saveState.primaryModalState != &subModal ||
        playerState.currentMasterType != 3 || playerState.airborneFlag != 0 ||
        !FloatNear(playerState.masterTypeTransitionCooldownUntilTime, 17.0f) ||
        !FloatNear(playerState.localVel.y, -3.0f) ||
        !FloatNear(playerState.worldPos.y, 25.9f) || modalObjectData.rotation.x != 0.0f ||
        modalObjectData.rotation.y != 0.0f || modalObjectData.rotation.z != 0.0f) {
        result = 7;
        goto restore;
    }

    result = 0;

restore:
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    return result;
}

extern "C" int player_transition_to_master_type_hover_smoke(void) {
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const HudUiElement oldUnderwaterFxPass3Ui = g_Player_UnderwaterFxPass3Ui;
    const int oldHorizonEnabled = g_Player_HorizonNodeFollowCameraEnabled;
    zClass_NodePartial *const oldHealthy1 = g_Player_CopterHealthyNode1;
    zClass_NodePartial *const oldHealthy2 = g_Player_CopterHealthyNode2;
    zClass_NodePartial *const oldSnd1 = g_Player_CopterSndNode1;
    zClass_NodePartial *const oldSnd2 = g_Player_CopterSndNode2;
    zSndSample *const oldCopterSample = g_Player_CopterSndSample;

    HudUiCommon_FTable visibleTable = {};
    visibleTable.slots[24] = MethodAddress(&TestPlayerHudVisibleReceiver::SetVisible);
    g_Player_UnderwaterFxPass3Ui = {};
    g_Player_UnderwaterFxPass3Ui.ftable = &visibleTable;
    std::memset(g_PlayerTestHudVisibleThis, 0, sizeof(g_PlayerTestHudVisibleThis));
    std::memset(g_PlayerTestHudVisibleValue, 0, sizeof(g_PlayerTestHudVisibleValue));
    g_PlayerTestHudVisibleCount = 0;
    g_Player_HorizonNodeFollowCameraEnabled = 0;
    g_Player_CopterHealthyNode1 = nullptr;
    g_Player_CopterHealthyNode2 = nullptr;
    g_Player_CopterSndNode1 = nullptr;
    g_Player_CopterSndNode2 = nullptr;
    g_Player_CopterSndSample = nullptr;
    g_GameStateOrMapTable = nullptr;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState sourceModal = {};
    PlayerModalState hoverModal = {};
    PlayerMasterModalData sourceData = {};
    PlayerMasterModalData hoverData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial modeVariantNode = {};
    zClass_NodePartial modalNode = {};
    zClass_Object3DDataPartial modalObjectData = {};
    rootNode.classId = 2;
    modeVariantNode.classId = 2;
    InitObjectPositionNode(&modalNode, &modalObjectData, 1.0f, 2.0f, 3.0f);

    saveState.playerState = &playerState;
    saveState.primaryModalState = &sourceModal;
    saveState.modalStateListHead = &sourceModal;
    sourceModal.masterModalData = &sourceData;
    sourceModal.modalNode = &modalNode;
    sourceModal.next = &hoverModal;
    hoverModal.masterModalData = &hoverData;
    hoverData.masterType = 4;
    playerState.rootNode = &rootNode;
    playerState.modeVariantNode = &modeVariantNode;

    sourceData.masterType = 3;
    playerState.hoverUnlocked = 1;
    playerState.masterTypeTransitionCooldownUntilTime = 20.0f;
    playerState.currentMasterType = 33;
    g_Time_AccumulatedTimeSec = 10.0f;
    int result = Player::TransitionToMasterTypeHover(&saveState, 0);
    if (result != 0 || saveState.primaryModalState != &sourceModal ||
        playerState.currentMasterType != 33) {
        result = 1;
        goto restore;
    }

    playerState.masterTypeTransitionCooldownUntilTime = 0.0f;
    playerState.hoverUnlocked = 0;
    if (Player::TransitionToMasterTypeHover(&saveState, 0) != 0) {
        result = 2;
        goto restore;
    }

    playerState.hoverUnlocked = 1;
    playerState.airborneFlag = 1;
    modalObjectData.rotation.x = 4.0f;
    modalObjectData.rotation.y = 5.0f;
    modalObjectData.rotation.z = 6.0f;
    modeVariantNode.flags = 0;
    saveState.primaryModalState = &sourceModal;
    sourceData.masterType = 3;
    g_Time_AccumulatedTimeSec = 10.0f;

    result = Player::TransitionToMasterTypeHover(&saveState, 0);
    if (result != 1 || saveState.primaryModalState != &hoverModal ||
        playerState.currentMasterType != 3 || playerState.airborneFlag != 0 ||
        !FloatNear(playerState.masterTypeTransitionCooldownUntilTime, 11.0f) ||
        (modeVariantNode.flags & 0x04) == 0 || modalObjectData.rotation.x != 0.0f ||
        modalObjectData.rotation.y != 0.0f || modalObjectData.rotation.z != 0.0f) {
        result = 3;
        goto restore;
    }

    saveState.primaryModalState = &sourceModal;
    sourceData.masterType = 2;
    playerState.masterTypeTransitionCooldownUntilTime = 0.0f;
    g_Time_AccumulatedTimeSec = 12.0f;
    if (Player::TransitionToMasterTypeHover(&saveState, 0) != 0) {
        result = 4;
        goto restore;
    }

    playerState.damageVisualFlag = 0;
    playerState.currentMasterType = 44;
    modeVariantNode.flags = 0;
    g_PlayerTestHudVisibleCount = 0;
    g_Player_HorizonNodeFollowCameraEnabled = 0;
    result = Player::TransitionToMasterTypeHover(&saveState, 1);
    if (result != 1 || saveState.primaryModalState != &hoverModal ||
        playerState.currentMasterType != 2 || playerState.damageVisualFlag != 1 ||
        g_Player_HorizonNodeFollowCameraEnabled != 1 || g_PlayerTestHudVisibleCount != 1 ||
        g_PlayerTestHudVisibleThis[0] != &g_Player_UnderwaterFxPass3Ui ||
        g_PlayerTestHudVisibleValue[0] != 0 ||
        !FloatNear(playerState.masterTypeTransitionCooldownUntilTime, 13.0f)) {
        result = 5;
        goto restore;
    }

    result = 0;

restore:
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Player_UnderwaterFxPass3Ui = oldUnderwaterFxPass3Ui;
    g_Player_HorizonNodeFollowCameraEnabled = oldHorizonEnabled;
    g_Player_CopterHealthyNode1 = oldHealthy1;
    g_Player_CopterHealthyNode2 = oldHealthy2;
    g_Player_CopterSndNode1 = oldSnd1;
    g_Player_CopterSndNode2 = oldSnd2;
    g_Player_CopterSndSample = oldCopterSample;
    return result;
}

extern "C" int player_cache_disable_copter_snd_nodes_smoke(void) {
    zClass_TypeListLink *const oldHead = zClass_TypeList::Head(6);
    zClass_TypeListLink *const oldTail = zClass_TypeList::Tail(6);
    zClass_NodePartial *const oldHealthy1 = g_Player_CopterHealthyNode1;
    zClass_NodePartial *const oldHealthy2 = g_Player_CopterHealthyNode2;
    zClass_NodePartial *const oldSnd1 = g_Player_CopterSndNode1;
    zClass_NodePartial *const oldSnd2 = g_Player_CopterSndNode2;
    zSndSample *const oldSample = g_Player_CopterSndSample;
    const int oldBackend = g_zSnd_ActiveBackend;

    zClass_NodePartial copter1 = {};
    zClass_NodePartial copter2 = {};
    zClass_NodePartial healthy1 = {};
    zClass_NodePartial healthy2 = {};
    zClass_NodePartial snd1 = {};
    zClass_NodePartial snd2 = {};
    zClass_NodePartial *copter1Children[2] = {&healthy1, &snd1};
    zClass_NodePartial *copter2Children[2] = {&healthy2, &snd2};
    zClass_SoundDataPartial snd1Data = {};
    zClass_SoundDataPartial snd2Data = {};
    zClass_TypeListLink copter1Link = {&copter1, nullptr, nullptr, 0};
    zClass_TypeListLink copter2Link = {&copter2, &copter1Link, nullptr, 0};
    copter1Link.next = &copter2Link;

    std::strcpy(copter1.name, "copter01");
    std::strcpy(copter2.name, "copter02");
    std::strcpy(healthy1.name, "healthy");
    std::strcpy(healthy2.name, "healthy");
    std::strcpy(snd1.name, "snd_chopper");
    std::strcpy(snd2.name, "snd_chopper");
    copter1.listCountB = 2;
    copter1.listB = copter1Children;
    copter2.listCountB = 2;
    copter2.listB = copter2Children;
    snd1.classId = 10;
    snd1.flags = 0x04;
    snd1.classData = &snd1Data;
    snd2.classId = 10;
    snd2.flags = 0x04;
    snd2.classData = &snd2Data;

    TestDirectSoundBufferVTable vtable = {};
    vtable.Stop = &TestPlayerDirectSoundStop;
    TestDirectSoundBuffer directSoundBuffer = {&vtable};
    zSndSample sample = {};
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);

    zClass_TypeList::Head(6) = &copter1Link;
    zClass_TypeList::Tail(6) = &copter2Link;
    g_Player_CopterHealthyNode1 = nullptr;
    g_Player_CopterHealthyNode2 = nullptr;
    g_Player_CopterSndNode1 = nullptr;
    g_Player_CopterSndNode2 = nullptr;
    g_Player_CopterSndSample = &sample;
    g_zSnd_ActiveBackend = 0;
    g_PlayerTestStopCount = 0;

    Player::CacheDisableCopterSndNodesAndStopSample();
    const bool lazyLookupOk =
        g_Player_CopterHealthyNode1 == &healthy1 && g_Player_CopterHealthyNode2 == &healthy2 &&
        g_Player_CopterSndNode1 == &snd1 && g_Player_CopterSndNode2 == &snd2 &&
        (snd1.flags & 0x04) == 0 && (snd2.flags & 0x04) == 0 && g_PlayerTestStopCount == 1;

    snd1.flags = 0x04;
    snd2.flags = 0x04;
    zClass_TypeList::Head(6) = nullptr;
    zClass_TypeList::Tail(6) = nullptr;

    Player::CacheDisableCopterSndNodesAndStopSample();
    const bool cachedPathOk =
        (snd1.flags & 0x04) == 0 && (snd2.flags & 0x04) == 0 && g_PlayerTestStopCount == 2;

    zClass_TypeList::Head(6) = oldHead;
    zClass_TypeList::Tail(6) = oldTail;
    g_Player_CopterHealthyNode1 = oldHealthy1;
    g_Player_CopterHealthyNode2 = oldHealthy2;
    g_Player_CopterSndNode1 = oldSnd1;
    g_Player_CopterSndNode2 = oldSnd2;
    g_Player_CopterSndSample = oldSample;
    g_zSnd_ActiveBackend = oldBackend;

    return lazyLookupOk && cachedPathOk ? 0 : 1;
}

extern "C" int player_reactivate_copter_snd_nodes_if_healthy_smoke(void) {
    zClass_NodePartial *const oldHealthy1 = g_Player_CopterHealthyNode1;
    zClass_NodePartial *const oldHealthy2 = g_Player_CopterHealthyNode2;
    zClass_NodePartial *const oldSnd1 = g_Player_CopterSndNode1;
    zClass_NodePartial *const oldSnd2 = g_Player_CopterSndNode2;
    zSndSample *const oldSample = g_Player_CopterSndSample;
    const int oldBackend = g_zSnd_ActiveBackend;

    TestDirectSoundBufferVTable vtable = {};
    vtable.Play = &TestDirectSoundPlay;
    TestDirectSoundBuffer directSoundBuffer = {&vtable};

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    zSndPlayHandle handle1 = {};
    handle1.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);
    zSndPlayHandle handle2 = {};
    handle2.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);
    zClass_SoundDataPartial sndData1 = {};
    sndData1.playHandle = &handle1;
    zClass_SoundDataPartial sndData2 = {};
    sndData2.playHandle = &handle2;

    zClass_NodePartial healthy1 = {};
    zClass_NodePartial healthy2 = {};
    zClass_NodePartial snd1 = {};
    zClass_NodePartial snd2 = {};
    healthy1.flags = 0x04;
    healthy2.flags = 0x04;
    snd1.classId = 10;
    snd1.classData = &sndData1;
    snd2.classId = 10;
    snd2.classData = &sndData2;

    g_Player_CopterHealthyNode1 = &healthy1;
    g_Player_CopterHealthyNode2 = &healthy2;
    g_Player_CopterSndNode1 = &snd1;
    g_Player_CopterSndNode2 = &snd2;
    g_Player_CopterSndSample = &sample;
    g_zSnd_ActiveBackend = 0;
    g_PlayerTestPlayCount = 0;

    Player::ReactivateCopterSndNodesIfHealthy();
    const bool activePlaybackOk =
        (snd1.flags & 0x04) != 0 && (snd2.flags & 0x04) != 0 &&
        g_PlayerTestPlayCount == 2;

    healthy1.flags = 0;
    snd1.flags = 0;
    snd2.flags = 0;
    sndData2.playHandle = nullptr;
    g_PlayerTestPlayCount = 0;

    Player::ReactivateCopterSndNodesIfHealthy();
    const bool inactiveAndMissingHandleOk =
        (snd1.flags & 0x04) == 0 && (snd2.flags & 0x04) != 0 &&
        g_PlayerTestPlayCount == 0;

    g_Player_CopterHealthyNode1 = oldHealthy1;
    g_Player_CopterHealthyNode2 = oldHealthy2;
    g_Player_CopterSndNode1 = oldSnd1;
    g_Player_CopterSndNode2 = oldSnd2;
    g_Player_CopterSndSample = oldSample;
    g_zSnd_ActiveBackend = oldBackend;

    return activePlaybackOk && inactiveAndMissingHandleOk ? 0 : 1;
}

extern "C" int player_start_slip_sfx_smoke(void) {
    TestDirectSoundBufferVTable vtable = {};
    vtable.GetStatus = &TestDirectSoundGetStatus;
    vtable.Play = &TestDirectSoundPlay;
    vtable.SetCurrentPosition = &TestDirectSoundSetInt;
    vtable.SetVolume = &TestDirectSoundSetInt;
    TestDirectSoundBuffer directSoundBuffer = {&vtable};

    float globalVolume = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_MuteDepth = 0;
    g_zSnd_Flag10PlaybackEnabled = 1;

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    sample.replayFields.gain = 1.0f;
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);

    PlayerMasterModalData modalData = {};
    modalData.sfxEngine[3] = &sample;
    PlayerModalState modalState = {};
    modalState.masterModalData = &modalData;
    zUtil_PlayerStateStorage playerState = {};
    playerState.worldPos = {10.0f, 20.0f, 30.0f};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    Player::StartSlipSfx(&saveState);

    g_zSnd_GlobalVolumeScalePtr = nullptr;
    g_zSnd_IsInitialized = 0;
    g_zSnd_PreInitialized = 0;

    return playerState.slipSfxActive == 1 &&
                   modalState.modalSfxHandle[3] == &sample.primaryVoice
               ? 0
               : 1;
}

extern "C" int player_stop_slip_sfx_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    zSndPlayHandle handle = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    playerState.slipSfxActive = 1;
    modalState.modalSfxHandle[3] = &handle;

    Player::StopSlipSfx(&saveState);
    return playerState.slipSfxActive == 0 && modalState.modalSfxHandle[3] == nullptr ? 0 : 1;
}

extern "C" int player_float_sign_smoke(void) {
    if (Player::FloatSign(0.0f) != 0) {
        return 1;
    }
    if (Player::FloatSign(-0.25f) != -1) {
        return 2;
    }

    return Player::FloatSign(4.0f) == 1 ? 0 : 3;
}

extern "C" int player_update_bank_and_turn_dynamics_smoke(void) {
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldPlayerInvDeltaTime = g_Player_InvDeltaTime;
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;

    g_Player_DeltaTime = 0.0f;
    if (Player::UpdateBankAndTurnDynamics(&saveState) != 0.0f) {
        g_Player_DeltaTime = oldPlayerDeltaTime;
        g_Player_InvDeltaTime = oldPlayerInvDeltaTime;
        return 1;
    }

    TestDirectSoundBufferVTable vtable = {};
    vtable.GetStatus = &TestDirectSoundGetStatus;
    vtable.Play = &TestDirectSoundPlay;
    vtable.SetCurrentPosition = &TestDirectSoundSetInt;
    vtable.SetVolume = &TestDirectSoundSetInt;
    TestDirectSoundBuffer directSoundBuffer = {&vtable};

    float globalVolume = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_MuteDepth = 0;
    g_zSnd_Flag10PlaybackEnabled = 1;

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    sample.replayFields.gain = 1.0f;
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);
    modalData.sfxEngine[3] = &sample;

    g_Player_DeltaTime = 0.5f;
    g_Player_InvDeltaTime = 2.0f;
    modalData.frictionStatic = 5.0f;
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.bankBasis = {0.0f, 0.0f, 2.0f};
    playerState.localVel = {0.0f, 0.0f, -3.0f};
    playerState.motionBasis.xy = 0.0f;

    const float staticResidual = Player::UpdateBankAndTurnDynamics(&saveState);
    if (!FloatNear(staticResidual, 7.0f) || playerState.slipSfxActive != 1 ||
        modalState.modalSfxHandle[3] != &sample.primaryVoice) {
        g_zSnd_GlobalVolumeScalePtr = nullptr;
        g_zSnd_IsInitialized = 0;
        g_zSnd_PreInitialized = 0;
        g_Player_DeltaTime = oldPlayerDeltaTime;
        g_Player_InvDeltaTime = oldPlayerInvDeltaTime;
        return 2;
    }

    std::memset(&playerState, 0, sizeof(playerState));
    std::memset(&modalState, 0, sizeof(modalState));
    modalState.masterModalData = &modalData;
    saveState.primaryModalState = &modalState;
    modalData.frictionStatic = 20.0f;
    modalData.frictionDynamic = 2.0f;
    playerState.slipSfxActive = 1;
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.bankBasis = {0.0f, 0.0f, 2.0f};
    playerState.localVel = {1.0f, 0.0f, -1.0f};
    playerState.motionBasis.xy = 0.0f;

    const float dynamicResidual = Player::UpdateBankAndTurnDynamics(&saveState);

    g_zSnd_GlobalVolumeScalePtr = nullptr;
    g_zSnd_IsInitialized = 0;
    g_zSnd_PreInitialized = 0;
    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_Player_InvDeltaTime = oldPlayerInvDeltaTime;

    return FloatNear(dynamicResidual, 2.0f) ? 0 : 3;
}

extern "C" int player_compute_turn_slip_delta_smoke(void) {
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldPlayerInvDeltaTime = g_Player_InvDeltaTime;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;

    g_Player_DeltaTime = 0.5f;
    g_Player_InvDeltaTime = 2.0f;
    playerState.motionBasis.xx = 1.0f;
    playerState.motionBasis.yy = 1.0f;
    playerState.motionBasis.zz = 1.0f;
    playerState.projectileSpawnVel = {2.0f, 0.0f, 6.0f};
    playerState.axisClampRuntime = 3.0f;
    playerState.throttleInputCopy = 1.0f;
    modalData.accelRate = 4.0f;
    modalData.frictionDynamic = 0.0f;
    modalData.frictionStatic = 100.0f;

    Player::ComputeTurnSlipDelta(&saveState);
    if (!FloatNear(playerState.localVel.x, 2.0f) ||
        !FloatNear(playerState.localVel.z, 3.0f)) {
        g_Player_DeltaTime = oldPlayerDeltaTime;
        g_Player_InvDeltaTime = oldPlayerInvDeltaTime;
        return 1;
    }

    zSndPlayHandle handle = {};
    std::memset(&playerState, 0, sizeof(playerState));
    playerState.motionBasis.xx = 1.0f;
    playerState.motionBasis.yy = 1.0f;
    playerState.motionBasis.zz = 1.0f;
    playerState.projectileSpawnVel = {1.0f, 0.0f, 1.0f};
    playerState.axisClampRuntime = 10.0f;
    playerState.slipSfxActive = 1;
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.bankBasis = {0.0f, 0.0f, 4.0f};
    modalState.modalSfxHandle[3] = &handle;
    modalData.accelRate = 0.0f;
    modalData.frictionDynamic = 0.0f;
    modalData.frictionStatic = 100.0f;
    g_Player_DeltaTime = 0.5f;
    g_Player_InvDeltaTime = 1.0f;

    Player::ComputeTurnSlipDelta(&saveState);

    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_Player_InvDeltaTime = oldPlayerInvDeltaTime;
    return FloatNear(playerState.localVel.x, 0.0f) && playerState.slipSfxActive == 0 &&
                   modalState.modalSfxHandle[3] == nullptr
               ? 0
               : 2;
}

extern "C" int player_update_yaw_velocity_from_steer_input_smoke(void) {
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldPlayerInvDeltaTime = g_Player_InvDeltaTime;
    const float oldPlayerDeltaTimeScaled001 = g_Player_DeltaTimeScaled001;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;

    g_Player_DeltaTime = 0.5f;
    g_Player_InvDeltaTime = 2.0f;
    g_Player_DeltaTimeScaled001 = 0.01f;
    g_GameStateOrMapTable = nullptr;

    playerState.localVel = {0.001f, 0.0f, -0.002f};
    playerState.axisClampRuntime = 5.0f;
    Player::UpdateYawVelocityFromSteerInput(&saveState);
    if (playerState.localVel.x != 0.0f || playerState.localVel.z != 0.0f) {
        g_Player_DeltaTime = oldPlayerDeltaTime;
        g_Player_InvDeltaTime = oldPlayerInvDeltaTime;
        g_Player_DeltaTimeScaled001 = oldPlayerDeltaTimeScaled001;
        g_GameStateOrMapTable = oldGameStateOrMapTable;
        return 1;
    }

    std::memset(&playerState, 0, sizeof(playerState));
    playerState.localVel = {2.0f, 0.0f, 4.0f};
    playerState.throttleInput = 1.0f;
    playerState.axisClampRuntime = 6.0f;
    modalData.rateDampingAccel = 0.125f;
    modalData.rateDampingDecel = 0.25f;
    g_Player_DeltaTimeScaled001 = 0.0f;

    Player::UpdateYawVelocityFromSteerInput(&saveState);
    if (!FloatNear(playerState.localVel.x, 2.0f * PlayerDampingFactor(0.125f, 0.5f)) ||
        !FloatNear(playerState.localVel.z, 4.0f * PlayerDampingFactor(0.25f, 0.5f))) {
        g_Player_DeltaTime = oldPlayerDeltaTime;
        g_Player_InvDeltaTime = oldPlayerInvDeltaTime;
        g_Player_DeltaTimeScaled001 = oldPlayerDeltaTimeScaled001;
        g_GameStateOrMapTable = oldGameStateOrMapTable;
        return 2;
    }

    std::memset(&playerState, 0, sizeof(playerState));
    playerState.localVel = {1.0f, 0.0f, -3.0f};
    playerState.throttleInput = 1.0f;
    playerState.axisClampRuntime = 10.0f;
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.bankBasis = {0.0f, 0.0f, 2.0f};
    modalData.rateDampingDecel = 0.0f;
    modalData.frictionDynamic = 1.0f;
    modalData.frictionStatic = 100.0f;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;

    Player::UpdateYawVelocityFromSteerInput(&saveState);

    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_Player_InvDeltaTime = oldPlayerInvDeltaTime;
    g_Player_DeltaTimeScaled001 = oldPlayerDeltaTimeScaled001;
    g_GameStateOrMapTable = oldGameStateOrMapTable;

    return FloatNear(playerState.localVel.x, 6.5f) &&
                   FloatNear(playerState.localVel.z, -3.0f)
               ? 0
               : 3;
}

extern "C" int player_update_sub_vertical_damping_smoke(void) {
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;

    g_Player_DeltaTime = 0.5f;
    modalData.accelRate = 40.0f;

    playerState.subVerticalInput = 1.0f;
    playerState.subVerticalInputCopy = 2.0f;
    playerState.localVel.y = -3.0f;
    Player::UpdateSubVerticalDamping(&saveState);
    if (!FloatNear(playerState.localVel.y, 20.0f)) {
        g_Player_DeltaTime = oldPlayerDeltaTime;
        g_Time_AccumulatedTimeSec = oldAccumulatedTime;
        return 1;
    }

    playerState.subVerticalInput = 1.0f;
    playerState.subVerticalInputCopy = -2.0f;
    playerState.localVel.y = 4.0f;
    Player::UpdateSubVerticalDamping(&saveState);
    if (!FloatNear(playerState.localVel.y, -20.0f)) {
        g_Player_DeltaTime = oldPlayerDeltaTime;
        g_Time_AccumulatedTimeSec = oldAccumulatedTime;
        return 2;
    }

    playerState.subVerticalInput = 0.0f;
    playerState.throttleInputCopy = 1.0f;
    playerState.localVel.y = 5.0f;
    Player::UpdateSubVerticalDamping(&saveState);
    if (!FloatNear(playerState.localVel.y, 5.0f)) {
        g_Player_DeltaTime = oldPlayerDeltaTime;
        g_Time_AccumulatedTimeSec = oldAccumulatedTime;
        return 3;
    }

    playerState.throttleInputCopy = 0.0f;
    playerState.primaryGunGateUntilTime = 20.0f;
    playerState.localVel.y = 10.0f;
    g_Time_AccumulatedTimeSec = 10.0f;
    Player::UpdateSubVerticalDamping(&saveState);
    if (!FloatNear(playerState.localVel.y, 10.0f * PlayerDampingFactor(2.0f, 0.5f))) {
        g_Player_DeltaTime = oldPlayerDeltaTime;
        g_Time_AccumulatedTimeSec = oldAccumulatedTime;
        return 4;
    }

    playerState.primaryGunGateUntilTime = 20.0f;
    playerState.localVel.y = 10.0f;
    g_Time_AccumulatedTimeSec = 20.0f;
    Player::UpdateSubVerticalDamping(&saveState);

    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    return FloatNear(playerState.localVel.y, 10.0f * PlayerDampingFactor(10.0f, 0.5f))
               ? 0
               : 5;
}

extern "C" int player_select_probe_sample_height_smoke(void) {
    PlayerProbeSampleCandidateBuffer emptyBuffer = {};
    int bestIndex = 99;
    int selectedImpactSlot = 88;
    float taggedHeight = 77.0f;
    float selected = Player::SelectProbeSampleHeightFromCandidates(
        &emptyBuffer, &bestIndex, 10.0f, 4.0f, 0, &selectedImpactSlot, &taggedHeight);
    if (selected != 10.0f || bestIndex != 0 || selectedImpactSlot != 0 ||
        taggedHeight != -300.0f) {
        return 1;
    }

    PlayerProbeSampleCandidateBuffer buffer = {};
    buffer.candidateCount = 3;
    buffer.entries[0].hitPos.y = 8.0f;
    buffer.entries[1].hitPos.y = 12.0f;
    buffer.entries[2].hitPos.y = 15.0f;
    selected = Player::SelectProbeSampleHeightFromCandidates(
        &buffer, &bestIndex, 10.0f, 4.0f, 0, &selectedImpactSlot, &taggedHeight);
    if (selected != 12.0f || bestIndex != 1 || selectedImpactSlot != 0 ||
        taggedHeight != -300.0f) {
        return 2;
    }

    buffer = {};
    buffer.candidateCount = 1;
    buffer.entries[0].hitPos.y = 20.0f;
    selected = Player::SelectProbeSampleHeightFromCandidates(
        &buffer, &bestIndex, 10.0f, 4.0f, 0, &selectedImpactSlot, &taggedHeight);
    if (selected != 20.0f || bestIndex != 0 || selectedImpactSlot != 0) {
        return 3;
    }

    zModel_MaterialPartial material = {};
    material.userTag = 1;
    buffer = {};
    buffer.candidateCount = 1;
    buffer.entries[0].hitPos.y = 12.0f;
    buffer.entries[0].scenePayload = &material;
    selected = Player::SelectProbeSampleHeightFromCandidates(
        &buffer, &bestIndex, 10.0f, 4.0f, 1, &selectedImpactSlot, &taggedHeight);
    if (selected != -250.0f || taggedHeight != 12.0f || selectedImpactSlot != 1) {
        return 4;
    }

    material.userTag = 2;
    buffer.entries[0].hitPos.y = 4.0f;
    selected = Player::SelectProbeSampleHeightFromCandidates(
        &buffer, &bestIndex, 10.0f, 4.0f, 0, &selectedImpactSlot, &taggedHeight);
    return selected == 4.0f && taggedHeight == 4.0f && selectedImpactSlot == 0 ? 0 : 5;
}

extern "C" int player_build_environment_probe_result_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeDiScene = g_Player_RuntimeDiScene;
    const float oldDeltaTime = g_Player_DeltaTime;
    const int oldSampleCount = g_PlayerEnvProbeSampleCount;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage localPlayerState = {};
    zUtil_SaveGameState saveState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData masterModalData = {};
    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial localRootNode = {};
    zClass_NodePartial worldNode = {};
    zClass_WorldDataPartial worldData = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &masterModalData;
    modalState.modalStateCode = 4;
    playerState.rootNode = &rootNode;
    localPlayerState.rootNode = &localRootNode;
    gameStateOrMap.playerState =
        static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&localPlayerState));
    g_GameStateOrMapTable = &gameStateOrMap;
    worldNode.classData = &worldData;
    g_Player_RuntimeDiScene = &worldNode;

    playerState.motionBasis.xx = 1.0f;
    playerState.motionBasis.yy = 1.0f;
    playerState.motionBasis.zz = 1.0f;
    playerState.motionBasis.posX = 10.0f;
    playerState.motionBasis.posY = 20.0f;
    playerState.motionBasis.posZ = 30.0f;
    playerState.worldPos = {100.0f, 200.0f, 300.0f};
    playerState.projectileSpawnVel.y = 2.0f;
    masterModalData.probePoints[15] = {1.0f, 2.0f, 3.0f};
    masterModalData.probePoints[16] = {4.0f, 5.0f, 6.0f};
    masterModalData.probePoints[17] = {7.0f, 8.0f, 9.0f};
    masterModalData.probePoints[18] = {10.0f, 11.0f, 12.0f};
    g_Player_DeltaTime = 0.25f;
    g_PlayerEnvProbeSampleCount = 7;

    PlayerEnvProbeResult result = {};
    result.minProbeDepth = 1.0f;
    Player::BuildEnvironmentProbeResult(&saveState, &result);

    const int ok = FloatNear(modalState.transformedProbePointWorldByIndex[0].x, 11.0f) &&
                   FloatNear(modalState.transformedProbePointWorldByIndex[3].z, 42.0f) &&
                   FloatNear(g_PlayerEnvProbeWorldPoints[4].x, 15.5f) &&
                   FloatNear(g_PlayerEnvProbeWorldPoints[5].y, 26.5f) &&
                   FloatNear(g_PlayerEnvProbeWorldPoints[6].z, 300.0f) &&
                   result.hitHistogram.countByImpactSlot[0] == 7 &&
                   result.bestIndexBySample[0] == 0 &&
                   FloatNear(result.candidateScoreBySample[0], 22.0f) &&
                   FloatNear(result.candidateScoreBySample[6], 199.8f) &&
                   result.highestSelectedHitY == -300.0f &&
                   result.attachmentCandidateCount == 0 && rootNode.nodeType == 0xff &&
                   playerState.probeImpactSlot1SeenFlag == 0;

    g_PlayerEnvProbeSampleCount = oldSampleCount;
    g_Player_DeltaTime = oldDeltaTime;
    g_Player_RuntimeDiScene = oldRuntimeDiScene;
    g_GameStateOrMapTable = oldGameStateOrMapTable;

    return ok ? 0 : 1;
}

extern "C" int player_probe_modal_sample_heights_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;

    static std::int32_t matrixFlags[2];
    static float *matrixSlots[2];
    static zMat4x3 identityMatrix;
    identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixFlags[0] = 1;
    matrixSlots[0] = reinterpret_cast<float *>(&identityMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage globalPlayerState = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial globalRootNode = {};
    zClass_NodePartial attachmentNode = {};
    playerState.rootNode = &rootNode;
    globalPlayerState.rootNode = &globalRootNode;
    playerState.motionBasis.xx = 1.0f;
    playerState.motionBasis.yy = 1.0f;
    playerState.motionBasis.zz = 1.0f;
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 0x42;
    playerState.variantTag.tags[1] = 0xff;
    playerState.variantTag.tags[2] = 0xff;

    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    gameStateOrMap.playerState =
        static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&globalPlayerState));
    g_GameStateOrMapTable = &gameStateOrMap;
    g_Player_DeltaTime = 0.5f;
    g_VariantTag_Current.count = 0;
    g_VariantTag_Current.tags[0] = 0xff;
    g_VariantTag_Current.tags[1] = 0xff;
    g_VariantTag_Current.tags[2] = 0xff;

    PlayerMasterModalData masterModalData = {};
    masterModalData.masterType = 1;
    masterModalData.probePoints[0] = {0.25f, 0.5f, 0.25f};
    masterModalData.probePoints[1] = {0.6f, 0.75f, 0.2f};
    PlayerModalState modalState = {};
    modalState.masterModalData = &masterModalData;
    modalState.modalStateCode = 2;
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    zVec3 probeFaceVertices[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 10.0f},
                                  {10.0f, 0.0f, 0.0f}};
    std::int32_t faceIndices[3] = {0, 1, 2};
    zModel_MaterialPartial materialPayload = {};
    materialPayload.userTag = 4;
    zModel_PickFaceUvData faceUvData = {};
    zModel_PickFaceEntry faceEntry = {};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.vertexIndices = faceIndices;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload =
        static_cast<zModel_PickFaceScenePayload *>(static_cast<void *>(&materialPayload));
    faceEntry.variantTag.count = 1;
    faceEntry.variantTag.tags[0] = 0x42;
    zModel_PickFaceData faceData = {};
    faceData.faceCount = 1;
    faceData.vertexCount = 3;
    faceData.faces = &faceEntry;
    faceData.baseVertices = probeFaceVertices;

    zClass_Object3DDataPartial objectData = {};
    objectData.flags = 8;
    zClass_NodePartial objectNode = {};
    objectNode.flags = 0x11c;
    objectNode.auxFlags = 1;
    objectNode.nodeType = 0xff;
    objectNode.classId = 5;
    objectNode.classData = &objectData;
    objectNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    objectNode.callbackContext = &attachmentNode;
    objectNode.cachedBounds[0] = 0.0f;
    objectNode.cachedBounds[1] = 0.0f;
    objectNode.cachedBounds[2] = 0.0f;
    objectNode.cachedBounds[3] = 10.0f;
    objectNode.cachedBounds[4] = 1.0f;
    objectNode.cachedBounds[5] = 10.0f;
    zClass_NodePartial *areaChildren[1] = {&objectNode};
    zWorldAreaPartial area = {};
    area.childCount = 1;
    area.childList = areaChildren;
    zWorldAreaPartial *areaRows[1] = {&area};
    zClass_WorldDataPartial worldData = {};
    worldData.clampQueriesToBounds = 1;
    worldData.areaCellSizeX = 10.0f;
    worldData.areaCellSizeZ = 10.0f;
    worldData.areaInvSizeX = 0.1f;
    worldData.areaInvSizeZ = 0.1f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = areaRows;
    zClass_NodePartial world = {};
    world.classData = &worldData;
    g_Player_RuntimeDiScene = &world;

    float sampleHeights[2] = {};
    float bestHeight = -9.0f;
    PlayerProbeTypeHistogram histogram = {};
    int attachmentCandidateCount = 0;
    zClass_NodePartial *attachmentOut = nullptr;
    Player::ProbeModalSampleHeights(&saveState, sampleHeights, &bestHeight, 0, &histogram,
                                    &attachmentCandidateCount, &attachmentOut);

    playerState.yawVelocityLimit = -1.0f;
    playerState.vehiclePitchRad = 3.0f;
    playerState.vehicleRollRad = 4.0f;
    playerState.worldPos.y = -5.0f;
    masterModalData.yawRateMax = 12.5f;
    masterModalData.modeAltTransitionTime = 10.0f;
    Player::UpdateMasterTypeBasicOrTrack_FromModalProbe(&saveState);
    const bool updateModalProbeOk =
        playerState.yawVelocityLimit == 12.5f && playerState.vehiclePitchRad == 0.0f &&
        playerState.vehicleRollRad == 0.0f && playerState.worldPos.y == 10.0f;

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_VariantTag_Current = oldVariantTagCurrent;

    if (sampleHeights[0] != 0.0f || sampleHeights[1] != 0.0f || bestHeight != 0.0f ||
        histogram.countByImpactSlot[4] != 2 || playerState.probeImpactSlot4SeenFlag != 1 ||
        playerState.probeImpactSlot1SeenFlag != 0 || attachmentCandidateCount != 2 ||
        attachmentOut != &attachmentNode || playerState.selectedProbeSample.node != &objectNode ||
        playerState.selectedProbeSample.hitPos.x != 0.25f ||
        playerState.selectedProbeSample.hitPos.y != 0.0f ||
        playerState.selectedProbeSample.hitPos.z != 0.25f ||
        playerState.variantTag.tags[0] != 0x42 || g_Variant_CurrentTag.count != 0 ||
        !updateModalProbeOk) {
        return 1;
    }

    return 0;
}

extern "C" int player_update_master_type_hover_from_modal_probe_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldFrameDeltaTimeSec = g_FrameDeltaTimeSec;
    const float oldAccumulatedTimeSec = g_Time_AccumulatedTimeSec;
    const float oldMaxSlope = g_Player_MaxSlope;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;

    static std::int32_t matrixFlags[2];
    static float *matrixSlots[2];
    static zMat4x3 identityMatrix;
    identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixFlags[0] = 1;
    matrixSlots[0] = reinterpret_cast<float *>(&identityMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage globalPlayerState = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial globalRootNode = {};
    zClass_NodePartial hoverVariantNode = {};
    playerState.rootNode = &rootNode;
    playerState.modeVariantNode = &hoverVariantNode;
    hoverVariantNode.classId = 5;
    globalPlayerState.rootNode = &globalRootNode;
    playerState.motionBasis = identityMatrix;
    playerState.previousTransform = identityMatrix;
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.steerBasisRef = {0.0f, 1.0f, 0.0f};
    playerState.gravityAccel = 10.0f;
    playerState.slipSfxActive = 1;
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 0x42;
    playerState.variantTag.tags[1] = 0xff;
    playerState.variantTag.tags[2] = 0xff;

    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    gameStateOrMap.playerState =
        static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&globalPlayerState));
    g_GameStateOrMapTable = &gameStateOrMap;
    g_Player_DeltaTime = 0.25f;
    g_FrameDeltaTimeSec = 0.25f;
    g_Time_AccumulatedTimeSec = 0.0f;
    g_Player_MaxSlope = 0.5f;
    g_VariantTag_Current.count = 0;
    g_VariantTag_Current.tags[0] = 0xff;
    g_VariantTag_Current.tags[1] = 0xff;
    g_VariantTag_Current.tags[2] = 0xff;

    PlayerMasterModalData masterModalData = {};
    masterModalData.masterType = 1;
    masterModalData.yawRateMax = 2.5f;
    masterModalData.modeAltTransitionTime = 2.0f;
    masterModalData.probePoints[0] = {0.5f, 0.0f, 0.5f};
    masterModalData.probePoints[1] = {0.0f, 0.0f, 0.0f};
    masterModalData.probePoints[2] = {0.0f, 0.0f, 1.0f};
    masterModalData.probePoints[3] = {1.0f, 0.0f, 0.0f};
    masterModalData.probePoints[15] = {0.5f, 3.0f, 0.5f};
    masterModalData.probePoints[16] = {0.0f, 3.0f, 0.0f};
    masterModalData.probePoints[17] = {0.0f, 3.0f, 1.0f};
    masterModalData.probePoints[18] = {1.0f, 3.0f, 0.0f};

    PlayerModalState modalState = {};
    modalState.masterModalData = &masterModalData;
    modalState.modalStateCode = 4;
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    zVec3 probeFaceVertices[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 10.0f},
                                  {10.0f, 0.0f, 0.0f}};
    std::int32_t faceIndices[3] = {0, 1, 2};
    zModel_MaterialPartial materialPayload = {};
    materialPayload.userTag = 4;
    zModel_PickFaceUvData faceUvData = {};
    zModel_PickFaceEntry faceEntry = {};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.vertexIndices = faceIndices;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload =
        static_cast<zModel_PickFaceScenePayload *>(static_cast<void *>(&materialPayload));
    faceEntry.variantTag.count = 1;
    faceEntry.variantTag.tags[0] = 0x42;
    zModel_PickFaceData faceData = {};
    faceData.faceCount = 1;
    faceData.vertexCount = 3;
    faceData.faces = &faceEntry;
    faceData.baseVertices = probeFaceVertices;

    zClass_Object3DDataPartial objectData = {};
    objectData.flags = 8;
    zClass_NodePartial objectNode = {};
    objectNode.flags = 0x11c;
    objectNode.nodeType = 0xff;
    objectNode.classId = 5;
    objectNode.classData = &objectData;
    objectNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    objectNode.cachedBounds[0] = 0.0f;
    objectNode.cachedBounds[1] = 0.0f;
    objectNode.cachedBounds[2] = 0.0f;
    objectNode.cachedBounds[3] = 10.0f;
    objectNode.cachedBounds[4] = 1.0f;
    objectNode.cachedBounds[5] = 10.0f;
    zClass_NodePartial *areaChildren[1] = {&objectNode};
    zWorldAreaPartial area = {};
    area.childCount = 1;
    area.childList = areaChildren;
    zWorldAreaPartial *areaRows[1] = {&area};
    zClass_WorldDataPartial worldData = {};
    worldData.clampQueriesToBounds = 1;
    worldData.areaCellSizeX = 10.0f;
    worldData.areaCellSizeZ = 10.0f;
    worldData.areaInvSizeX = 0.1f;
    worldData.areaInvSizeZ = 0.1f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = areaRows;
    zClass_NodePartial world = {};
    world.classData = &worldData;
    g_Player_RuntimeDiScene = &world;

    Player::UpdateMasterTypeHover_FromModalProbe(&saveState);

    int result = 0;
    if (playerState.yawVelocityLimit != 2.5f) {
        result = 2;
    } else if (!FloatNear(playerState.steerBasisRef.x, 0.0f) ||
               !FloatNear(playerState.steerBasisRef.y, 1.0f) ||
               !FloatNear(playerState.steerBasisRef.z, 0.0f)) {
        result = 3;
    } else if (!FloatNear(playerState.localVel.y, 5.0f)) {
        result = 4;
    } else if (!FloatNear(playerState.projectileSpawnVel.y, 5.0f)) {
        result = 5;
    } else if (!FloatNear(playerState.vehiclePitchRad, 0.0f) ||
               !FloatNear(playerState.vehicleRollRad, 0.0f)) {
        result = 6;
    } else if (!FloatNear(modalState.transformedProbePointWorldByIndex[0].y, 3.0f)) {
        result = 8;
    } else if ((hoverVariantNode.flags & 0x04) == 0) {
        result = 7;
    }

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_FrameDeltaTimeSec = oldFrameDeltaTimeSec;
    g_Time_AccumulatedTimeSec = oldAccumulatedTimeSec;
    g_Player_MaxSlope = oldMaxSlope;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_VariantTag_Current = oldVariantTagCurrent;

    return result;
}

extern "C" int player_update_master_type_amphib_from_modal_probe_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldInvDeltaTime = g_Player_InvDeltaTime;
    const float oldFrameDeltaTimeSec = g_FrameDeltaTimeSec;
    const float oldAccumulatedTimeSec = g_Time_AccumulatedTimeSec;
    const zVec3 oldAmphibBasisUpRef = g_Player_AmphibBasisUpRef;
    const float oldAmphibSteerBasisLerpRate = g_Player_AmphibSteerBasisLerpRate;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;

    static std::int32_t matrixFlags[2];
    static float *matrixSlots[2];
    static zMat4x3 identityMatrix;
    identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixFlags[0] = 1;
    matrixSlots[0] = reinterpret_cast<float *>(&identityMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage globalPlayerState = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial globalRootNode = {};
    playerState.rootNode = &rootNode;
    globalPlayerState.rootNode = &globalRootNode;
    playerState.motionBasis = identityMatrix;
    playerState.steerBasisNorm = {0.0f, 0.0f, -1.0f};
    playerState.steerBasisRef = {0.0f, 1.0f, 0.0f};
    playerState.worldPos = {5.0f, 1.0f, 7.0f};
    playerState.projectileSpawnVel = {3.0f, -2.0f, 4.0f};
    playerState.localVel = {6.0f, -1.0f, 8.0f};
    playerState.restartYawRad = 0.0f;
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 0x42;
    playerState.variantTag.tags[1] = 0xff;
    playerState.variantTag.tags[2] = 0xff;

    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    gameStateOrMap.playerState =
        static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&globalPlayerState));
    g_GameStateOrMapTable = &gameStateOrMap;
    g_Player_DeltaTime = 0.25f;
    g_Player_InvDeltaTime = 4.0f;
    g_FrameDeltaTimeSec = 0.25f;
    g_Time_AccumulatedTimeSec = 12.0f;
    g_Player_AmphibBasisUpRef = {0.0f, 1.0f, 0.0f};
    g_Player_AmphibSteerBasisLerpRate = 0.0f;
    g_VariantTag_Current.count = 0;
    g_VariantTag_Current.tags[0] = 0xff;
    g_VariantTag_Current.tags[1] = 0xff;
    g_VariantTag_Current.tags[2] = 0xff;

    PlayerMasterModalData masterModalData = {};
    masterModalData.masterType = 1;
    masterModalData.yawRateMax = 3.5f;
    masterModalData.modeAltTransitionTime = 2.0f;
    masterModalData.probePoints[0] = {0.5f, 0.0f, 0.5f};
    masterModalData.probePoints[1] = {0.0f, 0.0f, 0.0f};
    masterModalData.probePoints[2] = {0.0f, 0.0f, 1.0f};
    masterModalData.probePoints[3] = {1.0f, 0.0f, 0.0f};

    PlayerModalState modalState = {};
    modalState.masterModalData = &masterModalData;
    modalState.modalStateCode = 4;
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    zVec3 probeFaceVertices[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
                                  {1.0f, 0.0f, 0.0f}};
    std::int32_t faceIndices[3] = {0, 1, 2};
    zModel_MaterialPartial materialPayload = {};
    materialPayload.userTag = 1;
    zModel_PickFaceUvData faceUvData = {};
    zModel_PickFaceEntry faceEntry = {};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.vertexIndices = faceIndices;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload =
        static_cast<zModel_PickFaceScenePayload *>(static_cast<void *>(&materialPayload));
    faceEntry.variantTag.count = 1;
    faceEntry.variantTag.tags[0] = 0x42;
    zModel_PickFaceData faceData = {};
    faceData.faceCount = 1;
    faceData.vertexCount = 3;
    faceData.faces = &faceEntry;
    faceData.baseVertices = probeFaceVertices;

    zClass_Object3DDataPartial objectData = {};
    objectData.flags = 8;
    zClass_NodePartial objectNode = {};
    objectNode.flags = 0x11c;
    objectNode.nodeType = 0xff;
    objectNode.classId = 5;
    objectNode.classData = &objectData;
    objectNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    objectNode.cachedBounds[0] = 0.0f;
    objectNode.cachedBounds[1] = 0.0f;
    objectNode.cachedBounds[2] = 0.0f;
    objectNode.cachedBounds[3] = 1.0f;
    objectNode.cachedBounds[4] = 1.0f;
    objectNode.cachedBounds[5] = 1.0f;
    zClass_NodePartial *areaChildren[1] = {&objectNode};
    zWorldAreaPartial area = {};
    area.childCount = 1;
    area.childList = areaChildren;
    zWorldAreaPartial *areaRows[1] = {&area};
    zClass_WorldDataPartial worldData = {};
    worldData.clampQueriesToBounds = 1;
    worldData.areaCellSizeX = 1.0f;
    worldData.areaCellSizeZ = 1.0f;
    worldData.areaInvSizeX = 1.0f;
    worldData.areaInvSizeZ = 1.0f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = areaRows;
    zClass_NodePartial world = {};
    world.classData = &worldData;
    g_Player_RuntimeDiScene = &world;

    Player::UpdateMasterTypeAmphib_FromModalProbe(&saveState);

    const bool ok =
        playerState.yawVelocityLimit == 3.5f && playerState.amphibProbeCoverageFailed == 0 &&
        FloatNear(playerState.worldPos.y, 2.0f) && FloatNear(playerState.localVel.y, 4.0f) &&
        FloatNear(playerState.projectileSpawnVel.y, 4.0f) &&
        FloatNear(playerState.steerBasisRef.x, 0.0f) &&
        FloatNear(playerState.steerBasisRef.y, 1.0f) &&
        FloatNear(playerState.steerBasisRef.z, 0.0f) &&
        FloatNear(playerState.steerBasisRaw.x, 0.0f) &&
        FloatNear(playerState.steerBasisRaw.y, 0.0f) &&
        FloatNear(playerState.steerBasisRaw.z, -1.0f) &&
        FloatNear(playerState.motionBasis.xx, 1.0f) &&
        FloatNear(playerState.motionBasis.yy, 1.0f) &&
        FloatNear(playerState.motionBasis.zz, 1.0f) &&
        FloatNear(playerState.motionBasis.posY, 2.0f) &&
        FloatNear(playerState.vehiclePitchRad, 0.0f) &&
        FloatNear(playerState.vehicleRollRad, 0.0f);

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_Player_InvDeltaTime = oldInvDeltaTime;
    g_FrameDeltaTimeSec = oldFrameDeltaTimeSec;
    g_Time_AccumulatedTimeSec = oldAccumulatedTimeSec;
    g_Player_AmphibBasisUpRef = oldAmphibBasisUpRef;
    g_Player_AmphibSteerBasisLerpRate = oldAmphibSteerBasisLerpRate;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_VariantTag_Current = oldVariantTagCurrent;

    return ok ? 0 : 1;
}

extern "C" int player_update_sub_mode_water_probe_state_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldPlayerDeltaTimeScaled001 = g_Player_DeltaTimeScaled001;
    const float oldAccumulatedTimeSec = g_Time_AccumulatedTimeSec;
    const int oldHorizonFollow = g_Player_HorizonNodeFollowCameraEnabled;
    const HudUiElement oldUnderwaterFx = g_Player_UnderwaterFxPass3Ui;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;

    static std::int32_t matrixFlags[2];
    static float *matrixSlots[2];
    static zMat4x3 identityMatrix;
    identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixFlags[0] = 1;
    matrixSlots[0] = reinterpret_cast<float *>(&identityMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    HudUiCommon_FTable visibleTable = {};
    visibleTable.slots[24] = MethodAddress(&TestPlayerHudVisibleReceiver::SetVisible);
    g_Player_UnderwaterFxPass3Ui.ftable = &visibleTable;
    std::memset(g_PlayerTestHudVisibleThis, 0, sizeof(g_PlayerTestHudVisibleThis));
    std::memset(g_PlayerTestHudVisibleValue, 0, sizeof(g_PlayerTestHudVisibleValue));
    g_PlayerTestHudVisibleCount = 0;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage globalPlayerState = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial globalRootNode = {};
    zClass_NodePartial attachmentNode = {};
    playerState.rootNode = &rootNode;
    globalPlayerState.rootNode = &globalRootNode;
    playerState.motionBasis = identityMatrix;
    playerState.worldPos = {0.0f, -1.0f, 0.0f};
    playerState.localVel.z = 4.0f;
    playerState.vehiclePitchRad = 0.25f;
    playerState.vehicleRollRad = 0.5f;
    playerState.underwaterFxEnabled = 1;
    playerState.cameraTarget = {0.0f, -0.5f, 0.0f};
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 0x42;
    playerState.variantTag.tags[1] = 0xff;
    playerState.variantTag.tags[2] = 0xff;

    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    gameStateOrMap.playerState =
        static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&globalPlayerState));
    g_GameStateOrMapTable = &gameStateOrMap;
    g_Player_DeltaTime = 0.25f;
    g_Player_DeltaTimeScaled001 = 0.0f;
    g_Time_AccumulatedTimeSec = 12.0f;
    g_Player_HorizonNodeFollowCameraEnabled = 1;
    g_VariantTag_Current.count = 0;
    g_VariantTag_Current.tags[0] = 0xff;
    g_VariantTag_Current.tags[1] = 0xff;
    g_VariantTag_Current.tags[2] = 0xff;

    PlayerMasterModalData masterModalData = {};
    masterModalData.masterType = 2;
    masterModalData.yawRateMax = 7.0f;
    masterModalData.modeAltTransitionTime = 2.0f;
    masterModalData.probePoints[0] = {0.5f, 0.0f, 0.5f};
    masterModalData.probePoints[1] = {0.0f, 0.0f, 0.0f};
    masterModalData.probePoints[2] = {0.0f, 0.0f, 1.0f};
    masterModalData.probePoints[3] = {1.0f, 0.0f, 0.0f};
    masterModalData.probePoints[15] = {0.5f, 3.0f, 0.5f};
    masterModalData.probePoints[16] = {0.0f, 3.0f, 0.0f};
    masterModalData.probePoints[17] = {0.0f, 3.0f, 1.0f};
    masterModalData.probePoints[18] = {1.0f, 3.0f, 0.0f};

    PlayerModalState modalState = {};
    modalState.masterModalData = &masterModalData;
    modalState.modalStateCode = 4;
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    zVec3 probeFaceVertices[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
                                  {1.0f, 0.0f, 0.0f}};
    std::int32_t faceIndices[3] = {0, 1, 2};
    zModel_MaterialPartial materialPayload = {};
    materialPayload.userTag = 1;
    zModel_PickFaceUvData faceUvData = {};
    zModel_PickFaceEntry faceEntry = {};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.vertexIndices = faceIndices;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload =
        static_cast<zModel_PickFaceScenePayload *>(static_cast<void *>(&materialPayload));
    faceEntry.variantTag.count = 1;
    faceEntry.variantTag.tags[0] = 0x42;
    zModel_PickFaceData faceData = {};
    faceData.faceCount = 1;
    faceData.vertexCount = 3;
    faceData.faces = &faceEntry;
    faceData.baseVertices = probeFaceVertices;

    zClass_Object3DDataPartial objectData = {};
    objectData.flags = 8;
    zClass_NodePartial objectNode = {};
    objectNode.flags = 0x11c;
    objectNode.auxFlags = 1;
    objectNode.nodeType = 0xff;
    objectNode.classId = 5;
    objectNode.classData = &objectData;
    objectNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    objectNode.callbackContext = &attachmentNode;
    objectNode.cachedBounds[0] = 0.0f;
    objectNode.cachedBounds[1] = 0.0f;
    objectNode.cachedBounds[2] = 0.0f;
    objectNode.cachedBounds[3] = 1.0f;
    objectNode.cachedBounds[4] = 1.0f;
    objectNode.cachedBounds[5] = 1.0f;
    zClass_NodePartial *areaChildren[1] = {&objectNode};
    zWorldAreaPartial area = {};
    area.childCount = 1;
    area.childList = areaChildren;
    zWorldAreaPartial *areaRows[1] = {&area};
    zClass_WorldDataPartial worldData = {};
    worldData.clampQueriesToBounds = 1;
    worldData.areaCellSizeX = 1.0f;
    worldData.areaCellSizeZ = 1.0f;
    worldData.areaInvSizeX = 1.0f;
    worldData.areaInvSizeZ = 1.0f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = areaRows;
    zClass_NodePartial world = {};
    world.classData = &worldData;
    g_Player_RuntimeDiScene = &world;

    Player::UpdateSubModeWaterProbeState(&saveState);

    int result = 0;
    const float expectedRollDamping = -(PlayerDampingFactor(1.0f, 0.25f) * 0.5f);
    if (!FloatNear(playerState.yawVelocityLimit, 7.0f)) {
        result = 1;
    } else if (!FloatNear(playerState.subModeProbeBestHeight, 0.0f)) {
        result = 2;
    } else if (!FloatNear(playerState.worldPos.y, 2.0f) ||
               !FloatNear(playerState.motionBasis.posY, 2.0f)) {
        result = 3;
    } else if (!FloatNear(playerState.angVelRoll, expectedRollDamping)) {
        result = 4;
    } else if (!FloatNear(playerState.vehiclePitchRad, 0.25f) ||
               !FloatNear(playerState.vehicleRollRad, 0.5f)) {
        result = 5;
    } else if (g_PlayerTestHudVisibleCount != 1 ||
               g_PlayerTestHudVisibleThis[0] != &g_Player_UnderwaterFxPass3Ui ||
               g_PlayerTestHudVisibleValue[0] != 1 ||
               g_Player_HorizonNodeFollowCameraEnabled != 0) {
        result = 6;
    } else if (playerState.probeImpactSlot1SeenFlag != 1 ||
               playerState.selectedProbeSample.node != &objectNode ||
               playerState.variantTag.tags[0] != 0x42) {
        result = 7;
    }

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_Player_DeltaTimeScaled001 = oldPlayerDeltaTimeScaled001;
    g_Time_AccumulatedTimeSec = oldAccumulatedTimeSec;
    g_Player_HorizonNodeFollowCameraEnabled = oldHorizonFollow;
    g_Player_UnderwaterFxPass3Ui = oldUnderwaterFx;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_VariantTag_Current = oldVariantTagCurrent;

    return result;
}

extern "C" int player_update_master_type_sub_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldPlayerDeltaTimeScaled001 = g_Player_DeltaTimeScaled001;
    const float oldAccumulatedTimeSec = g_Time_AccumulatedTimeSec;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;

    static std::int32_t matrixFlags[2];
    static float *matrixSlots[2];
    static zMat4x3 identityMatrix;
    identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixFlags[0] = 1;
    matrixSlots[0] = reinterpret_cast<float *>(&identityMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage globalPlayerState = {};
    zClass_Object3DDataPartial rootData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial globalRootNode = {};
    zClass_NodePartial attachmentNode = {};
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    playerState.rootNode = &rootNode;
    globalPlayerState.rootNode = &globalRootNode;
    playerState.motionBasis = identityMatrix;
    playerState.previousTransform = identityMatrix;
    playerState.worldPos = {0.0f, -1.0f, 0.0f};
    playerState.localVel = {1.0f, 0.0f, 1.0f};
    playerState.throttleInputCopy = 1.0f;
    playerState.fxOffsetLocal = {0.25f, 0.5f, 0.75f};
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 0x42;
    playerState.variantTag.tags[1] = 0xff;
    playerState.variantTag.tags[2] = 0xff;

    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    gameStateOrMap.playerState =
        static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&globalPlayerState));
    g_GameStateOrMapTable = &gameStateOrMap;
    g_Player_DeltaTime = 0.25f;
    g_Player_DeltaTimeScaled001 = 0.0f;
    g_Time_AccumulatedTimeSec = 12.0f;
    g_VariantTag_Current.count = 0;
    g_VariantTag_Current.tags[0] = 0xff;
    g_VariantTag_Current.tags[1] = 0xff;
    g_VariantTag_Current.tags[2] = 0xff;

    PlayerMasterModalData masterModalData = {};
    masterModalData.masterType = 2;
    masterModalData.maxSpeed = 10.0f;
    masterModalData.yawRateMax = 3.5f;
    masterModalData.modeAltTransitionTime = 2.0f;
    masterModalData.probePoints[0] = {0.5f, 0.0f, 0.5f};
    masterModalData.probePoints[1] = {0.0f, 0.0f, 0.0f};
    masterModalData.probePoints[2] = {0.0f, 0.0f, 1.0f};
    masterModalData.probePoints[3] = {1.0f, 0.0f, 0.0f};
    masterModalData.probePoints[15] = {0.5f, 3.0f, 0.5f};
    masterModalData.probePoints[16] = {0.0f, 3.0f, 0.0f};
    masterModalData.probePoints[17] = {0.0f, 3.0f, 1.0f};
    masterModalData.probePoints[18] = {1.0f, 3.0f, 0.0f};

    PlayerModalState modalState = {};
    modalState.masterModalData = &masterModalData;
    modalState.modalStateCode = 4;
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    zVec3 probeFaceVertices[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 10.0f},
                                  {10.0f, 0.0f, 0.0f}};
    std::int32_t faceIndices[3] = {0, 1, 2};
    zModel_MaterialPartial materialPayload = {};
    materialPayload.userTag = 1;
    zModel_PickFaceUvData faceUvData = {};
    zModel_PickFaceEntry faceEntry = {};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.vertexIndices = faceIndices;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload =
        static_cast<zModel_PickFaceScenePayload *>(static_cast<void *>(&materialPayload));
    faceEntry.variantTag.count = 1;
    faceEntry.variantTag.tags[0] = 0x42;
    zModel_PickFaceData faceData = {};
    faceData.faceCount = 1;
    faceData.vertexCount = 3;
    faceData.faces = &faceEntry;
    faceData.baseVertices = probeFaceVertices;

    zClass_Object3DDataPartial objectData = {};
    objectData.flags = 8;
    zClass_NodePartial objectNode = {};
    objectNode.flags = 0x11c;
    objectNode.auxFlags = 1;
    objectNode.nodeType = 0xff;
    objectNode.classId = 5;
    objectNode.classData = &objectData;
    objectNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    objectNode.callbackContext = &attachmentNode;
    objectNode.cachedBounds[0] = 0.0f;
    objectNode.cachedBounds[1] = 0.0f;
    objectNode.cachedBounds[2] = 0.0f;
    objectNode.cachedBounds[3] = 10.0f;
    objectNode.cachedBounds[4] = 1.0f;
    objectNode.cachedBounds[5] = 10.0f;
    zClass_NodePartial *areaChildren[1] = {&objectNode};
    zWorldAreaPartial area = {};
    area.childCount = 1;
    area.childList = areaChildren;
    zWorldAreaPartial *areaRows[1] = {&area};
    zClass_WorldDataPartial worldData = {};
    worldData.clampQueriesToBounds = 1;
    worldData.areaCellSizeX = 10.0f;
    worldData.areaCellSizeZ = 10.0f;
    worldData.areaInvSizeX = 0.1f;
    worldData.areaInvSizeZ = 0.1f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = areaRows;
    zClass_NodePartial world = {};
    world.classData = &worldData;
    g_Player_RuntimeDiScene = &world;

    Player::UpdateMasterTypeSub(&saveState);

    int result = 0;
    if (!FloatNear(playerState.axisClampRuntime, 10.0f)) {
        result = 1;
    } else if (!FloatNear(playerState.projectileSpawnVel.x, 1.0f)) {
        result = 21;
    } else if (!FloatNear(playerState.projectileSpawnVel.y, 0.0f)) {
        result = 22;
    } else if (!FloatNear(playerState.projectileSpawnVel.z, 1.0f)) {
        result = 23;
    } else if (!FloatNear(playerState.worldPos.x, 0.25f) ||
               !FloatNear(playerState.worldPos.y, 2.0f) ||
               !FloatNear(playerState.worldPos.z, 0.25f)) {
        result = 3;
    } else if (!FloatNear(playerState.motionBasis.posX, 0.25f) ||
               !FloatNear(playerState.motionBasis.posY, 2.0f) ||
               !FloatNear(playerState.motionBasis.posZ, 0.25f)) {
        result = 4;
    } else if (!FloatNear(playerState.fxOffsetWorld.x, 0.5f) ||
               !FloatNear(playerState.fxOffsetWorld.y, 2.5f) ||
               !FloatNear(playerState.fxOffsetWorld.z, 1.0f)) {
        result = 5;
    } else if (!FloatNear(playerState.bankBasis.x, 0.0f) ||
               !FloatNear(playerState.bankBasis.y, 0.0f) ||
               !FloatNear(playerState.bankBasis.z, -1.0f)) {
        result = 6;
    } else if (!FloatNear(playerState.cachedPitchRad, playerState.vehiclePitchRad) ||
               !FloatNear(playerState.cachedYawRad, playerState.restartYawRad) ||
               !FloatNear(playerState.cachedRollRad, playerState.vehicleRollRad)) {
        result = 7;
    } else if (!FloatNear(playerState.previousTransform.posX, rootData.localMatrix[9]) ||
               !FloatNear(playerState.previousTransform.posY, rootData.localMatrix[10]) ||
               !FloatNear(playerState.previousTransform.posZ, rootData.localMatrix[11])) {
        result = 8;
    } else if (!FloatNear(playerState.subModeProbeBestHeight, 0.0f) ||
               playerState.probeImpactSlot1SeenFlag != 1 ||
               playerState.selectedProbeSample.node != &objectNode) {
        result = 9;
    }

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_Player_DeltaTimeScaled001 = oldPlayerDeltaTimeScaled001;
    g_Time_AccumulatedTimeSec = oldAccumulatedTimeSec;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_VariantTag_Current = oldVariantTagCurrent;

    return result;
}

extern "C" int player_tick_master_type_and_force_feedback_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const int oldConditionalEnabled = g_zEffect_ConditionalRefPosEnabled;
    const float oldConditionalX = g_zEffect_ConditionalRefPosX;
    const float oldConditionalY = g_zEffect_ConditionalRefPosY;
    const float oldConditionalZ = g_zEffect_ConditionalRefPosZ;
    int *const oldJoystickOption = ZOPT_INPUT_JOYSTICK;
    const int oldForceFeedbackCaps = g_zInput_JoystickCaps_ForceFeedback;
    zInput_FFEffectSet *const oldEffectSet = g_zInputFfEffectSet;

    Player::TickMasterTypeAndForceFeedback(nullptr);

    zUtil_SaveGameState inactiveSave = {};
    zUtil_PlayerStateStorage inactiveState = {};
    PlayerModalState inactiveModalState = {};
    PlayerMasterModalData inactiveModalData = {};
    inactiveSave.playerState = &inactiveState;
    inactiveSave.primaryModalState = &inactiveModalState;
    inactiveModalState.masterModalData = &inactiveModalData;
    inactiveState.lifecycleState = 4;
    inactiveState.damageProtectionActive = 1;
    inactiveState.throttleInput = 1.0f;
    Player::TickMasterTypeAndForceFeedback(&inactiveSave);
    if (!FloatNear(inactiveState.throttleInput, 1.0f)) {
        g_GameStateOrMapTable = oldGameStateOrMapTable;
        return 1;
    }

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    int joystickEnabled = 0;
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    modalData.masterType = 6;
    playerState.lifecycleState = 1;
    playerState.damageProtectionActive = 1;
    playerState.throttleInput = 1.0f;
    playerState.steeringInput = 2.0f;
    playerState.subVerticalInput = 3.0f;
    playerState.subPitchInput = 4.0f;
    playerState.worldPos = {5.0f, 6.0f, 7.0f};
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);
    ZOPT_INPUT_JOYSTICK = &joystickEnabled;
    g_zInput_JoystickCaps_ForceFeedback = 0;
    g_zInputFfEffectSet = nullptr;
    g_zEffect_ConditionalRefPosEnabled = 0;

    Player::TickMasterTypeAndForceFeedback(&saveState);
    const bool ok =
        playerState.throttleInput == 0.0f &&
        playerState.steeringInput == 0.0f &&
        playerState.subVerticalInput == 0.0f &&
        playerState.subPitchInput == 0.0f &&
        g_zEffect_ConditionalRefPosEnabled == 1 &&
        FloatNear(g_zEffect_ConditionalRefPosX, 5.0f) &&
        FloatNear(g_zEffect_ConditionalRefPosY, 6.0f) &&
        FloatNear(g_zEffect_ConditionalRefPosZ, 7.0f);

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_zEffect_ConditionalRefPosEnabled = oldConditionalEnabled;
    g_zEffect_ConditionalRefPosX = oldConditionalX;
    g_zEffect_ConditionalRefPosY = oldConditionalY;
    g_zEffect_ConditionalRefPosZ = oldConditionalZ;
    ZOPT_INPUT_JOYSTICK = oldJoystickOption;
    g_zInput_JoystickCaps_ForceFeedback = oldForceFeedbackCaps;
    g_zInputFfEffectSet = oldEffectSet;

    return ok ? 0 : 2;
}

extern "C" int player_update_master_type_amphib_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldInvDeltaTime = g_Player_InvDeltaTime;
    const float oldFrameDeltaTimeSec = g_FrameDeltaTimeSec;
    const float oldAccumulatedTimeSec = g_Time_AccumulatedTimeSec;
    const zVec3 oldAmphibBasisUpRef = g_Player_AmphibBasisUpRef;
    const float oldAmphibSteerBasisLerpRate = g_Player_AmphibSteerBasisLerpRate;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;

    static std::int32_t matrixFlags[2];
    static float *matrixSlots[2];
    static zMat4x3 identityMatrix;
    identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixFlags[0] = 1;
    matrixSlots[0] = reinterpret_cast<float *>(&identityMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage globalPlayerState = {};
    zClass_Object3DDataPartial rootData = {};
    zClass_Object3DDataPartial wakeData = {};
    zClass_Object3DDataPartial splashLData = {};
    zClass_Object3DDataPartial splashRData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial globalRootNode = {};
    zClass_NodePartial wakeNode = {};
    zClass_NodePartial splashLNode = {};
    zClass_NodePartial splashRNode = {};
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    wakeNode.classId = 5;
    wakeNode.classData = &wakeData;
    splashLNode.classId = 5;
    splashLNode.classData = &splashLData;
    splashRNode.classId = 5;
    splashRNode.classData = &splashRData;
    playerState.rootNode = &rootNode;
    globalPlayerState.rootNode = &globalRootNode;
    playerState.motionBasis = identityMatrix;
    playerState.previousTransform = identityMatrix;
    playerState.localVel = {0.5f, 0.0f, 0.5f};
    playerState.throttleInputCopy = 1.0f;
    playerState.worldPos = {0.0f, 1.0f, 0.0f};
    playerState.fxOffsetLocal = {0.25f, 0.5f, 0.75f};
    playerState.restartYawRad = 0.0f;
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 0x42;
    playerState.variantTag.tags[1] = 0xff;
    playerState.variantTag.tags[2] = 0xff;

    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    gameStateOrMap.playerState =
        static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&globalPlayerState));
    g_GameStateOrMapTable = &gameStateOrMap;
    g_Player_DeltaTime = 0.25f;
    g_Player_InvDeltaTime = 4.0f;
    g_FrameDeltaTimeSec = 0.25f;
    g_Time_AccumulatedTimeSec = 12.0f;
    g_Player_AmphibBasisUpRef = {0.0f, 1.0f, 0.0f};
    g_Player_AmphibSteerBasisLerpRate = 0.0f;
    g_VariantTag_Current.count = 0;
    g_VariantTag_Current.tags[0] = 0xff;
    g_VariantTag_Current.tags[1] = 0xff;
    g_VariantTag_Current.tags[2] = 0xff;

    PlayerMasterModalData masterModalData = {};
    masterModalData.masterType = 1;
    masterModalData.maxSpeed = 10.0f;
    masterModalData.yawRateMax = 3.5f;
    masterModalData.modeAltTransitionTime = 2.0f;
    masterModalData.probePoints[0] = {0.5f, 0.0f, 0.5f};
    masterModalData.probePoints[1] = {0.0f, 0.0f, 0.0f};
    masterModalData.probePoints[2] = {0.0f, 0.0f, 1.0f};
    masterModalData.probePoints[3] = {1.0f, 0.0f, 0.0f};

    PlayerModalState modalState = {};
    modalState.masterModalData = &masterModalData;
    modalState.modalStateCode = 4;
    modalState.nodeWake = &wakeNode;
    modalState.nodeSplashL = &splashLNode;
    modalState.nodeSplashR = &splashRNode;
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    zVec3 probeFaceVertices[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 10.0f},
                                  {10.0f, 0.0f, 0.0f}};
    std::int32_t faceIndices[3] = {0, 1, 2};
    zModel_MaterialPartial materialPayload = {};
    materialPayload.userTag = 1;
    zModel_PickFaceUvData faceUvData = {};
    zModel_PickFaceEntry faceEntry = {};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.vertexIndices = faceIndices;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload =
        static_cast<zModel_PickFaceScenePayload *>(static_cast<void *>(&materialPayload));
    faceEntry.variantTag.count = 1;
    faceEntry.variantTag.tags[0] = 0x42;
    zModel_PickFaceData faceData = {};
    faceData.faceCount = 1;
    faceData.vertexCount = 3;
    faceData.faces = &faceEntry;
    faceData.baseVertices = probeFaceVertices;

    zClass_Object3DDataPartial objectData = {};
    objectData.flags = 8;
    zClass_NodePartial objectNode = {};
    objectNode.flags = 0x11c;
    objectNode.nodeType = 0xff;
    objectNode.classId = 5;
    objectNode.classData = &objectData;
    objectNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    objectNode.cachedBounds[0] = 0.0f;
    objectNode.cachedBounds[1] = 0.0f;
    objectNode.cachedBounds[2] = 0.0f;
    objectNode.cachedBounds[3] = 10.0f;
    objectNode.cachedBounds[4] = 1.0f;
    objectNode.cachedBounds[5] = 10.0f;
    zClass_NodePartial *areaChildren[1] = {&objectNode};
    zWorldAreaPartial area = {};
    area.childCount = 1;
    area.childList = areaChildren;
    zWorldAreaPartial *areaRows[1] = {&area};
    zClass_WorldDataPartial worldData = {};
    worldData.clampQueriesToBounds = 1;
    worldData.areaCellSizeX = 10.0f;
    worldData.areaCellSizeZ = 10.0f;
    worldData.areaInvSizeX = 0.1f;
    worldData.areaInvSizeZ = 0.1f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = areaRows;
    zClass_NodePartial world = {};
    world.classData = &worldData;
    g_Player_RuntimeDiScene = &world;

    Player::UpdateMasterTypeAmphib(&saveState);
    int result = 0;
    if (!FloatNear(playerState.projectileSpawnVel.x, 0.5f)) {
        result = FloatNear(playerState.projectileSpawnVel.x, 0.0f) ? 16 : 2;
    } else if (!FloatNear(playerState.projectileSpawnVel.y, 4.0f)) {
        result = FloatNear(playerState.projectileSpawnVel.y, 0.0f)
                     ? 17
                     : (FloatNear(playerState.projectileSpawnVel.y, 8.0f) ? 18 : 3);
    } else if (!FloatNear(playerState.projectileSpawnVel.z, 0.5f)) {
        result = FloatNear(playerState.projectileSpawnVel.z, 0.0f) ? 15 : 4;
    } else if (!FloatNear(playerState.worldPos.x, 0.125f)) {
        result = 5;
    } else if (!FloatNear(playerState.worldPos.y, 2.0f)) {
        result = 6;
    } else if (!FloatNear(playerState.worldPos.z, 0.125f)) {
        result = 7;
    } else if (!FloatNear(playerState.motionBasis.posX, 0.125f) ||
               !FloatNear(playerState.motionBasis.posY, 2.0f) ||
               !FloatNear(playerState.motionBasis.posZ, 0.125f)) {
        result = 8;
    } else if (!FloatNear(playerState.fxOffsetWorld.x, 0.375f) ||
               !FloatNear(playerState.fxOffsetWorld.y, 2.5f) ||
               !FloatNear(playerState.fxOffsetWorld.z, 0.875f)) {
        result = 9;
    } else if (!FloatNear(playerState.bankBasis.x, 0.0f) ||
               !FloatNear(playerState.bankBasis.y, 0.0f) ||
               !FloatNear(playerState.bankBasis.z, -1.0f)) {
        result = 10;
    } else if (!FloatNear(playerState.previousTransform.posX, rootData.localMatrix[9]) ||
               !FloatNear(playerState.previousTransform.posY, rootData.localMatrix[10]) ||
               !FloatNear(playerState.previousTransform.posZ, rootData.localMatrix[11])) {
        result = 11;
    } else if (!FloatNear(wakeData.scale.x, 0.05f) ||
               !FloatNear(splashLData.scale.y, 0.05f) ||
               !FloatNear(splashRData.scale.z, 0.05f)) {
        result = 12;
    }

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_Player_InvDeltaTime = oldInvDeltaTime;
    g_FrameDeltaTimeSec = oldFrameDeltaTimeSec;
    g_Time_AccumulatedTimeSec = oldAccumulatedTimeSec;
    g_Player_AmphibBasisUpRef = oldAmphibBasisUpRef;
    g_Player_AmphibSteerBasisLerpRate = oldAmphibSteerBasisLerpRate;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_VariantTag_Current = oldVariantTagCurrent;

    return result;
}

extern "C" int player_update_master_type_hover_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldFrameDeltaTimeSec = g_FrameDeltaTimeSec;
    const float oldAccumulatedTimeSec = g_Time_AccumulatedTimeSec;
    const float oldMaxSlope = g_Player_MaxSlope;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;

    static std::int32_t matrixFlags[2];
    static float *matrixSlots[2];
    static zMat4x3 identityMatrix;
    identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixFlags[0] = 1;
    matrixSlots[0] = reinterpret_cast<float *>(&identityMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage globalPlayerState = {};
    zClass_Object3DDataPartial rootData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial globalRootNode = {};
    zClass_NodePartial hoverVariantNode = {};
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    hoverVariantNode.classId = 5;
    playerState.rootNode = &rootNode;
    playerState.modeVariantNode = &hoverVariantNode;
    globalPlayerState.rootNode = &globalRootNode;
    playerState.motionBasis = identityMatrix;
    playerState.previousTransform = identityMatrix;
    playerState.steerBasisRef = {0.0f, 1.0f, 0.0f};
    playerState.localVel = {0.0f, 0.0f, 0.0f};
    playerState.worldPos = {1.0f, 0.0f, 2.0f};
    playerState.fxOffsetLocal = {0.25f, 0.5f, 0.75f};
    playerState.gravityAccel = 10.0f;
    playerState.slipSfxActive = 1;
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 0x42;
    playerState.variantTag.tags[1] = 0xff;
    playerState.variantTag.tags[2] = 0xff;

    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    gameStateOrMap.playerState =
        static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&globalPlayerState));
    g_GameStateOrMapTable = &gameStateOrMap;
    g_Player_DeltaTime = 0.25f;
    g_FrameDeltaTimeSec = 0.25f;
    g_Time_AccumulatedTimeSec = 0.0f;
    g_Player_MaxSlope = 0.5f;
    g_VariantTag_Current.count = 0;
    g_VariantTag_Current.tags[0] = 0xff;
    g_VariantTag_Current.tags[1] = 0xff;
    g_VariantTag_Current.tags[2] = 0xff;

    PlayerMasterModalData masterModalData = {};
    masterModalData.masterType = 1;
    masterModalData.maxSpeed = 12.0f;
    masterModalData.yawRateMax = 2.5f;
    masterModalData.modeAltTransitionTime = 2.0f;
    masterModalData.probePoints[0] = {0.5f, 0.0f, 0.5f};
    masterModalData.probePoints[1] = {0.0f, 0.0f, 0.0f};
    masterModalData.probePoints[2] = {0.0f, 0.0f, 1.0f};
    masterModalData.probePoints[3] = {1.0f, 0.0f, 0.0f};
    masterModalData.probePoints[15] = {0.5f, 3.0f, 0.5f};
    masterModalData.probePoints[16] = {0.0f, 3.0f, 0.0f};
    masterModalData.probePoints[17] = {0.0f, 3.0f, 1.0f};
    masterModalData.probePoints[18] = {1.0f, 3.0f, 0.0f};

    PlayerModalState modalState = {};
    modalState.masterModalData = &masterModalData;
    modalState.modalStateCode = 4;
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    zVec3 probeFaceVertices[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
                                  {1.0f, 0.0f, 0.0f}};
    std::int32_t faceIndices[3] = {0, 1, 2};
    zModel_MaterialPartial materialPayload = {};
    materialPayload.userTag = 4;
    zModel_PickFaceUvData faceUvData = {};
    zModel_PickFaceEntry faceEntry = {};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.vertexIndices = faceIndices;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload =
        static_cast<zModel_PickFaceScenePayload *>(static_cast<void *>(&materialPayload));
    faceEntry.variantTag.count = 1;
    faceEntry.variantTag.tags[0] = 0x42;
    zModel_PickFaceData faceData = {};
    faceData.faceCount = 1;
    faceData.vertexCount = 3;
    faceData.faces = &faceEntry;
    faceData.baseVertices = probeFaceVertices;

    zClass_Object3DDataPartial objectData = {};
    objectData.flags = 8;
    zClass_NodePartial objectNode = {};
    objectNode.flags = 0x11c;
    objectNode.nodeType = 0xff;
    objectNode.classId = 5;
    objectNode.classData = &objectData;
    objectNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    objectNode.cachedBounds[0] = 0.0f;
    objectNode.cachedBounds[1] = 0.0f;
    objectNode.cachedBounds[2] = 0.0f;
    objectNode.cachedBounds[3] = 1.0f;
    objectNode.cachedBounds[4] = 1.0f;
    objectNode.cachedBounds[5] = 1.0f;
    zClass_NodePartial *areaChildren[1] = {&objectNode};
    zWorldAreaPartial area = {};
    area.childCount = 1;
    area.childList = areaChildren;
    zWorldAreaPartial *areaRows[1] = {&area};
    zClass_WorldDataPartial worldData = {};
    worldData.clampQueriesToBounds = 1;
    worldData.areaCellSizeX = 1.0f;
    worldData.areaCellSizeZ = 1.0f;
    worldData.areaInvSizeX = 1.0f;
    worldData.areaInvSizeZ = 1.0f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = areaRows;
    zClass_NodePartial world = {};
    world.classData = &worldData;
    g_Player_RuntimeDiScene = &world;

    Player::UpdateMasterTypeHover(&saveState);

    const bool ok =
        FloatNear(playerState.axisClampRuntime, 12.0f) &&
        FloatNear(playerState.yawVelocityLimit, 2.5f) &&
        FloatNear(playerState.projectileSpawnVel.y, 5.0f) &&
        FloatNear(playerState.worldPos.x, 1.0f) &&
        FloatNear(playerState.worldPos.y, 0.0f) &&
        FloatNear(playerState.worldPos.z, 2.0f) &&
        FloatNear(playerState.fxOffsetWorld.x, 1.25f) &&
        FloatNear(playerState.fxOffsetWorld.y, 0.5f) &&
        FloatNear(playerState.fxOffsetWorld.z, 2.75f) &&
        FloatNear(playerState.bankBasis.z, -1.0f) &&
        FloatNear(playerState.cachedPitchRad, playerState.vehiclePitchRad) &&
        FloatNear(playerState.cachedYawRad, playerState.restartYawRad) &&
        FloatNear(playerState.cachedRollRad, playerState.vehicleRollRad) &&
        (hoverVariantNode.flags & 0x04) != 0;

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_FrameDeltaTimeSec = oldFrameDeltaTimeSec;
    g_Time_AccumulatedTimeSec = oldAccumulatedTimeSec;
    g_Player_MaxSlope = oldMaxSlope;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_VariantTag_Current = oldVariantTagCurrent;

    return ok ? 0 : 1;
}

extern "C" int player_apply_environment_probe_result_smoke(void) {
    const float oldNominalGravity = g_Player_NominalGravity;
    const float oldWaterGravity = g_Player_WaterGravity;
    const float oldQuicksandGravity = g_Player_QuicksandGravity;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    g_GameStateOrMapTable = nullptr;
    g_Player_NominalGravity = 9.0f;
    g_Player_WaterGravity = 2.0f;
    g_Player_QuicksandGravity = 3.0f;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    PlayerEnvProbeResult quicksandProbe = {};
    zClass_NodePartial attachmentNode = {};
    zClass_Object3DDataPartial attachmentData = {};
    PlayerEnvProbeResult attachProbe = {};
    PlayerEnvProbeResult detachProbe = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    modalData.maxSpeed = 100.0f;
    modalData.yawRateMax = 12.0f;
    modalData.quicksandSlowdown = 0.25f;
    modalData.lavaSlowdown = 0.1f;

    PlayerEnvProbeResult normalProbe = {};
    playerState.motionInput = 5;
    playerState.underwaterStatusActive = 1;
    playerState.worldPos.y = 0.0f;
    int result = Player::ApplyEnvironmentProbeResult(&saveState, &normalProbe);
    if (result != 1 || playerState.underwaterStatusActive != 0 ||
        playerState.motionInput != 0 || !FloatNear(playerState.gravityAccel, 9.0f) ||
        !FloatNear(playerState.axisClampRuntime, 100.0f) ||
        !FloatNear(playerState.yawVelocityLimit, 12.0f)) {
        result = 1;
        goto restore;
    }

    quicksandProbe.hitHistogram.countByImpactSlot[3] = 2;
    result = Player::ApplyEnvironmentProbeResult(&saveState, &quicksandProbe);
    if (result != 1 || !FloatNear(playerState.gravityAccel, 3.0f) ||
        !FloatNear(playerState.axisClampRuntime, 25.0f) ||
        !FloatNear(playerState.yawVelocityLimit, 3.0f)) {
        result = 2;
        goto restore;
    }

    attachmentNode.classData = &attachmentData;
    attachmentData.cachedWorldMatrix[0] = 1.0f;
    attachmentData.cachedWorldMatrix[4] = 1.0f;
    attachmentData.cachedWorldMatrix[8] = 1.0f;
    attachmentData.cachedWorldMatrix[9] = 10.0f;
    attachmentData.cachedWorldMatrix[10] = 20.0f;
    attachmentData.cachedWorldMatrix[11] = 30.0f;

    attachProbe.attachmentCandidateCount = 4;
    attachProbe.attachmentNode = &attachmentNode;
    playerState.environmentAttachmentActive = 0;
    playerState.environmentAttachmentNode = nullptr;
    playerState.worldPos = {12.0f, 23.0f, 34.0f};
    playerState.restartYawRad = 0.5f;
    result = Player::ApplyEnvironmentProbeResult(&saveState, &attachProbe);
    if (result != 1 || playerState.environmentAttachmentActive != 1 ||
        playerState.environmentAttachmentNode != &attachmentNode ||
        !FloatNear(playerState.yawPoseCache, 0.5f) ||
        !FloatNear(playerState.fxOffsetLocal.x, 2.0f) ||
        !FloatNear(playerState.fxOffsetLocal.y, 3.0f) ||
        !FloatNear(playerState.fxOffsetLocal.z, 4.0f)) {
        result = 3;
        goto restore;
    }

    playerState.environmentAttachmentActive = 1;
    playerState.environmentAttachmentNode = &attachmentNode;
    playerState.yawPoseCache = 0.25f;
    playerState.vehiclePitchRad = 1.0f;
    playerState.vehicleRollRad = 2.0f;
    result = Player::ApplyEnvironmentProbeResult(&saveState, &detachProbe);
    if (result != 1 || playerState.environmentAttachmentActive != 0 ||
        playerState.environmentAttachmentNode != nullptr ||
        !FloatNear(playerState.restartYawRad, 0.25f) ||
        !FloatNear(playerState.pitchPoseCache, 1.0f) ||
        !FloatNear(playerState.yawPoseCache, 0.25f) ||
        !FloatNear(playerState.rollPoseCache, 2.0f)) {
        result = 4;
        goto restore;
    }

    result = 0;

restore:
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Player_NominalGravity = oldNominalGravity;
    g_Player_WaterGravity = oldWaterGravity;
    g_Player_QuicksandGravity = oldQuicksandGravity;
    return result;
}

extern "C" int player_surface_height_and_terrain_tilt_smoke(void) {
    const float oldNominalGravity = g_Player_NominalGravity;
    const float oldDeltaTime = g_Player_DeltaTime;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    g_GameStateOrMapTable = nullptr;
    g_Player_NominalGravity = 10.0f;
    g_Player_DeltaTime = 0.1f;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    playerState.worldPos = {4.0f, 0.0f, 3.0f};
    playerState.steerBasisRef = {1.0f, 2.0f, 3.0f};
    float solvedHeight = Player::SolveHeightOnSurface(&saveState, 20.0f);
    if (!FloatNear(solvedHeight, 3.5f)) {
        g_GameStateOrMapTable = oldGameStateOrMapTable;
        g_Player_DeltaTime = oldDeltaTime;
        g_Player_NominalGravity = oldNominalGravity;
        return 1;
    }

    playerState.steerBasisRef.y = 0.0f;
    solvedHeight = Player::SolveHeightOnSurface(&saveState, 7.0f);
    if (!FloatNear(solvedHeight, -60000.0f)) {
        g_GameStateOrMapTable = oldGameStateOrMapTable;
        g_Player_DeltaTime = oldDeltaTime;
        g_Player_NominalGravity = oldNominalGravity;
        return 2;
    }

    playerState.angVelPitch = 7.0f;
    playerState.angVelRoll = -8.0f;
    playerState.projectileSpawnVel.y = -30.0f;
    Player::ResetTerrainContactImpulsesAndPlayImpactSfx(&saveState);
    if (playerState.angVelPitch != 0.0f || playerState.angVelRoll != 0.0f) {
        g_GameStateOrMapTable = oldGameStateOrMapTable;
        g_Player_DeltaTime = oldDeltaTime;
        g_Player_NominalGravity = oldNominalGravity;
        return 3;
    }

    playerState.gravityAccel = 5.0f;
    playerState.restartYawRad = 0.0f;
    playerState.airborneFlag = 0;
    playerState.angVelPitch = 0.5f;
    playerState.angVelRoll = -0.25f;
    playerState.projectileSpawnVel = {1.0f, 2.0f, 3.0f};
    playerState.steerBasisRef = {1.0f, 9.0f, 2.0f};
    const zVec3 tilt = {2.0f, 0.0f, 3.0f};
    Player::ApplyTerrainTilt(&saveState, &tilt, 0.5f);

    const bool ok =
        FloatNear(playerState.angVelPitch, 1.20000005f) &&
        FloatNear(playerState.angVelRoll, 1.20000005f) &&
        FloatNear(playerState.projectileSpawnVel.x, 3.5f) &&
        FloatNear(playerState.projectileSpawnVel.y, 2.0f) &&
        FloatNear(playerState.projectileSpawnVel.z, 8.0f);

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Player_DeltaTime = oldDeltaTime;
    g_Player_NominalGravity = oldNominalGravity;
    return ok ? 0 : 4;
}

extern "C" int player_compute_surface_from1_probe_smoke(void) {
    const float oldNominalGravity = g_Player_NominalGravity;
    const float oldDeltaTime = g_Player_DeltaTime;
    const int oldProbeIndex0 = g_PlayerEnvProbe_AboveGroundIndices[0];
    const zVec3 oldProbePoint2 = g_PlayerEnvProbeWorldPoints[2];

    g_Player_NominalGravity = 10.0f;
    g_Player_DeltaTime = 0.1f;
    g_PlayerEnvProbe_AboveGroundIndices[0] = 2;
    g_PlayerEnvProbeWorldPoints[2] = {12.0f, 100.0f, 25.0f};

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerEnvProbeResult probeResult = {};
    saveState.playerState = &playerState;
    playerState.worldPos = {10.0f, 0.0f, 20.0f};
    playerState.steerBasisRef = {0.0f, 1.0f, 0.0f};
    playerState.gravityAccel = 10.0f;
    playerState.restartYawRad = 0.0f;
    probeResult.candidateScoreBySample[2] = 7.0f;

    Player::ComputeSurfaceFrom1Probe(&saveState, &probeResult);

    const bool ok =
        FloatNear(playerState.worldPos.y, 7.0f) &&
        FloatNear(playerState.angVelPitch, -1.20000005f) &&
        FloatNear(playerState.angVelRoll, 1.20000005f) &&
        FloatNear(playerState.projectileSpawnVel.x, 0.0f) &&
        FloatNear(playerState.projectileSpawnVel.z, 0.0f);

    g_PlayerEnvProbeWorldPoints[2] = oldProbePoint2;
    g_PlayerEnvProbe_AboveGroundIndices[0] = oldProbeIndex0;
    g_Player_DeltaTime = oldDeltaTime;
    g_Player_NominalGravity = oldNominalGravity;
    return ok ? 0 : 1;
}

extern "C" int player_compute_triangle_normal_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    const zVec3 pointA = {0.0f, 0.0f, 0.0f};
    const zVec3 pointB = {1.0f, 0.0f, 0.0f};
    const zVec3 pointC = {0.0f, 0.0f, 1.0f};
    Player::ComputeTriangleNormal(&saveState, &pointA, &pointB, &pointC);
    if (!FloatNear(playerState.steerBasisRef.x, 0.0f) ||
        !FloatNear(playerState.steerBasisRef.y, 1.0f) ||
        !FloatNear(playerState.steerBasisRef.z, 0.0f)) {
        return 1;
    }

    const zVec3 pointD = {0.0f, 2.0f, 0.0f};
    const zVec3 pointE = {0.0f, 2.0f, 1.0f};
    const zVec3 pointF = {1.0f, 2.0f, 0.0f};
    Player::ComputeTriangleNormal(&saveState, &pointD, &pointE, &pointF);
    return FloatNear(playerState.steerBasisRef.x, 0.0f) &&
                   FloatNear(playerState.steerBasisRef.y, 1.0f) &&
                   FloatNear(playerState.steerBasisRef.z, 0.0f)
               ? 0
               : 2;
}

extern "C" int player_compute_surface_from2_probes_smoke(void) {
    const float oldNominalGravity = g_Player_NominalGravity;
    const float oldDeltaTime = g_Player_DeltaTime;
    const int oldProbeIndex0 = g_PlayerEnvProbe_AboveGroundIndices[0];
    const int oldProbeIndex1 = g_PlayerEnvProbe_AboveGroundIndices[1];
    const zVec3 oldProbePoint0 = g_PlayerEnvProbeWorldPoints[0];
    const zVec3 oldProbePoint1 = g_PlayerEnvProbeWorldPoints[1];

    g_Player_NominalGravity = 10.0f;
    g_Player_DeltaTime = 0.1f;
    g_PlayerEnvProbe_AboveGroundIndices[0] = 0;
    g_PlayerEnvProbe_AboveGroundIndices[1] = 1;
    g_PlayerEnvProbeWorldPoints[0] = {0.0f, 50.0f, 0.0f};
    g_PlayerEnvProbeWorldPoints[1] = {2.0f, 75.0f, 0.0f};

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerEnvProbeResult probeResult = {};
    saveState.playerState = &playerState;
    playerState.worldPos = {0.0f, 0.0f, 1.0f};
    playerState.steerBasisRef = {0.0f, 1.0f, 0.0f};
    playerState.gravityAccel = 10.0f;
    playerState.restartYawRad = 0.0f;
    probeResult.candidateScoreBySample[0] = 2.0f;
    probeResult.candidateScoreBySample[1] = 4.0f;

    Player::ComputeSurfaceFrom2Probes(&saveState, &probeResult);

    const float invSqrt2 = 0.70710677f;
    const bool ok =
        FloatNear(playerState.steerBasisRef.x, -invSqrt2) &&
        FloatNear(playerState.steerBasisRef.y, invSqrt2) &&
        FloatNear(playerState.steerBasisRef.z, 0.0f) &&
        FloatNear(playerState.worldPos.y, 2.0f) &&
        FloatNear(playerState.angVelPitch, invSqrt2) &&
        FloatNear(playerState.angVelRoll, 0.0f) &&
        FloatNear(playerState.projectileSpawnVel.x, -3.5355339f) &&
        FloatNear(playerState.projectileSpawnVel.z, 0.0f);

    g_PlayerEnvProbeWorldPoints[1] = oldProbePoint1;
    g_PlayerEnvProbeWorldPoints[0] = oldProbePoint0;
    g_PlayerEnvProbe_AboveGroundIndices[1] = oldProbeIndex1;
    g_PlayerEnvProbe_AboveGroundIndices[0] = oldProbeIndex0;
    g_Player_DeltaTime = oldDeltaTime;
    g_Player_NominalGravity = oldNominalGravity;
    return ok ? 0 : 1;
}

extern "C" int player_check_probe_sample_mask_overlap_smoke(void) {
    const int overlap = Player::CheckProbeSampleMaskOverlap(0, 3, 4);
    if (overlap != 0x08) {
        return 1;
    }

    const int noOverlap = Player::CheckProbeSampleMaskOverlap(1, 4, 6);
    return noOverlap == 0 ? 0 : 2;
}

extern "C" int player_rebuild_above_ground_indices_smoke(void) {
    const int oldSampleCount = g_PlayerEnvProbeSampleCount;
    int oldFlags[10] = {};
    int oldIndices[10] = {};
    for (int i = 0; i < 10; ++i) {
        oldFlags[i] = g_PlayerEnvProbe_AboveGroundFlags[i];
        oldIndices[i] = g_PlayerEnvProbe_AboveGroundIndices[i];
        g_PlayerEnvProbe_AboveGroundFlags[i] = 0;
        g_PlayerEnvProbe_AboveGroundIndices[i] = 100 + i;
    }

    g_PlayerEnvProbeSampleCount = 7;
    g_PlayerEnvProbe_AboveGroundFlags[1] = 1;
    g_PlayerEnvProbe_AboveGroundFlags[3] = 2;
    g_PlayerEnvProbe_AboveGroundFlags[4] = 1;
    g_PlayerEnvProbe_AboveGroundFlags[6] = 1;
    Player::RebuildAboveGroundIndices();

    const bool ok = g_PlayerEnvProbe_AboveGroundIndices[0] == 1 &&
                    g_PlayerEnvProbe_AboveGroundIndices[1] == 3 &&
                    g_PlayerEnvProbe_AboveGroundIndices[2] == 4 &&
                    g_PlayerEnvProbe_AboveGroundIndices[3] == 6 &&
                    g_PlayerEnvProbe_AboveGroundIndices[4] == 104;

    for (int i = 0; i < 10; ++i) {
        g_PlayerEnvProbe_AboveGroundFlags[i] = oldFlags[i];
        g_PlayerEnvProbe_AboveGroundIndices[i] = oldIndices[i];
    }
    g_PlayerEnvProbeSampleCount = oldSampleCount;
    return ok ? 0 : 1;
}

extern "C" int player_select_best_probes_by_dot_product_smoke(void) {
    const int oldSampleCount = g_PlayerEnvProbeSampleCount;
    int oldFlags[10] = {};
    int oldIndices[10] = {};
    zVec3 oldPoints[7] = {};
    for (int i = 0; i < 10; ++i) {
        oldFlags[i] = g_PlayerEnvProbe_AboveGroundFlags[i];
        oldIndices[i] = g_PlayerEnvProbe_AboveGroundIndices[i];
        g_PlayerEnvProbe_AboveGroundFlags[i] = 0;
        g_PlayerEnvProbe_AboveGroundIndices[i] = 0;
    }
    for (int i = 0; i < 7; ++i) {
        oldPoints[i] = g_PlayerEnvProbeWorldPoints[i];
        g_PlayerEnvProbeWorldPoints[i] = {};
    }

    zVec3 referenceNormal = {0.0f, 1.0f, 0.0f};
    PlayerEnvProbeResult probeResult = {};
    g_PlayerEnvProbeSampleCount = 5;
    for (int i = 0; i < 5; ++i) {
        g_PlayerEnvProbe_AboveGroundFlags[i] = 1;
    }
    probeResult.candidateScoreBySample[0] = 10.0f;
    probeResult.candidateScoreBySample[1] = 9.0f;
    probeResult.candidateScoreBySample[2] = 8.0f;
    probeResult.candidateScoreBySample[3] = 7.0f;
    probeResult.candidateScoreBySample[4] = 6.0f;
    Player::SelectBestProbesByDotProduct(&referenceNormal, &probeResult);
    bool ok = g_PlayerEnvProbe_AboveGroundFlags[0] == 1 &&
              g_PlayerEnvProbe_AboveGroundFlags[1] == 1 &&
              g_PlayerEnvProbe_AboveGroundFlags[2] == 1 &&
              g_PlayerEnvProbe_AboveGroundFlags[3] == 0 &&
              g_PlayerEnvProbe_AboveGroundFlags[4] == 0 &&
              g_PlayerEnvProbe_AboveGroundIndices[0] == 0 &&
              g_PlayerEnvProbe_AboveGroundIndices[1] == 1 &&
              g_PlayerEnvProbe_AboveGroundIndices[2] == 2;

    for (int i = 0; i < 10; ++i) {
        g_PlayerEnvProbe_AboveGroundFlags[i] = 0;
        g_PlayerEnvProbe_AboveGroundIndices[i] = 0;
    }
    for (int i = 0; i < 5; ++i) {
        g_PlayerEnvProbe_AboveGroundFlags[i] = 1;
        probeResult.candidateScoreBySample[i] = 0.0f;
    }
    probeResult.candidateScoreBySample[0] = 10.0f;
    probeResult.candidateScoreBySample[1] = 7.0f;
    probeResult.candidateScoreBySample[3] = 9.0f;
    probeResult.candidateScoreBySample[4] = 8.0f;
    Player::SelectBestProbesByDotProduct(&referenceNormal, &probeResult);
    ok = ok && g_PlayerEnvProbe_AboveGroundFlags[0] == 1 &&
         g_PlayerEnvProbe_AboveGroundFlags[1] == 1 &&
         g_PlayerEnvProbe_AboveGroundFlags[2] == 0 &&
         g_PlayerEnvProbe_AboveGroundFlags[3] == 1 &&
         g_PlayerEnvProbe_AboveGroundFlags[4] == 0 &&
         g_PlayerEnvProbe_AboveGroundIndices[0] == 0 &&
         g_PlayerEnvProbe_AboveGroundIndices[1] == 1 &&
         g_PlayerEnvProbe_AboveGroundIndices[2] == 3;

    for (int i = 0; i < 7; ++i) {
        g_PlayerEnvProbeWorldPoints[i] = oldPoints[i];
    }
    for (int i = 0; i < 10; ++i) {
        g_PlayerEnvProbe_AboveGroundFlags[i] = oldFlags[i];
        g_PlayerEnvProbe_AboveGroundIndices[i] = oldIndices[i];
    }
    g_PlayerEnvProbeSampleCount = oldSampleCount;
    return ok ? 0 : 1;
}

extern "C" int player_compute_surface_from3_probes_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const int oldIndex0 = g_PlayerEnvProbe_AboveGroundIndices[0];
    const int oldIndex1 = g_PlayerEnvProbe_AboveGroundIndices[1];
    const int oldIndex2 = g_PlayerEnvProbe_AboveGroundIndices[2];
    const zVec3 oldPoint0 = g_PlayerEnvProbeWorldPoints[0];
    const zVec3 oldPoint1 = g_PlayerEnvProbeWorldPoints[1];
    const zVec3 oldPoint2 = g_PlayerEnvProbeWorldPoints[2];

    g_GameStateOrMapTable = nullptr;
    g_PlayerEnvProbe_AboveGroundIndices[0] = 0;
    g_PlayerEnvProbe_AboveGroundIndices[1] = 1;
    g_PlayerEnvProbe_AboveGroundIndices[2] = 2;
    g_PlayerEnvProbeWorldPoints[0] = {0.0f, 50.0f, 0.0f};
    g_PlayerEnvProbeWorldPoints[1] = {2.0f, 75.0f, 0.0f};
    g_PlayerEnvProbeWorldPoints[2] = {0.0f, 25.0f, -2.0f};

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerEnvProbeResult probeResult = {};
    saveState.playerState = &playerState;
    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    playerState.airborneFlag = 1;
    playerState.angVelPitch = 5.0f;
    playerState.angVelRoll = -6.0f;
    probeResult.candidateScoreBySample[0] = 2.0f;
    probeResult.candidateScoreBySample[1] = 4.0f;
    probeResult.candidateScoreBySample[2] = 2.0f;

    Player::ComputeSurfaceFrom3Probes(&saveState, &probeResult);

    const float invSqrt2 = 0.70710677f;
    const bool ok =
        FloatNear(playerState.steerBasisRef.x, -invSqrt2) &&
        FloatNear(playerState.steerBasisRef.y, invSqrt2) &&
        FloatNear(playerState.steerBasisRef.z, 0.0f) &&
        FloatNear(playerState.worldPos.y, 2.0f) &&
        FloatNear(playerState.angVelPitch, 0.0f) &&
        FloatNear(playerState.angVelRoll, 0.0f);

    g_PlayerEnvProbeWorldPoints[2] = oldPoint2;
    g_PlayerEnvProbeWorldPoints[1] = oldPoint1;
    g_PlayerEnvProbeWorldPoints[0] = oldPoint0;
    g_PlayerEnvProbe_AboveGroundIndices[2] = oldIndex2;
    g_PlayerEnvProbe_AboveGroundIndices[1] = oldIndex1;
    g_PlayerEnvProbe_AboveGroundIndices[0] = oldIndex0;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    return ok ? 0 : 1;
}

extern "C" int player_process_env_probe_results_smoke(void) {
    const float oldDeltaTime = g_Player_DeltaTime;
    const float oldNominalGravity = g_Player_NominalGravity;
    const float oldQuicksandSinkRate = g_Player_QuicksandSinkRate;
    const float oldLavaSinkRate = g_Player_LavaSinkRate;
    const int oldSampleCount = g_PlayerEnvProbeSampleCount;
    const int oldAboveGroundCount = g_PlayerEnvProbe_AboveGroundCount;
    int oldFlags[10] = {};
    int oldIndices[10] = {};
    zVec3 oldPoints[7] = {};
    for (int i = 0; i < 10; ++i) {
        oldFlags[i] = g_PlayerEnvProbe_AboveGroundFlags[i];
        oldIndices[i] = g_PlayerEnvProbe_AboveGroundIndices[i];
        g_PlayerEnvProbe_AboveGroundFlags[i] = 0;
        g_PlayerEnvProbe_AboveGroundIndices[i] = 0;
    }
    for (int i = 0; i < 7; ++i) {
        oldPoints[i] = g_PlayerEnvProbeWorldPoints[i];
        g_PlayerEnvProbeWorldPoints[i] = {};
    }

    g_Player_DeltaTime = 0.1f;
    g_Player_NominalGravity = 10.0f;
    g_Player_QuicksandSinkRate = 0.25f;
    g_Player_LavaSinkRate = 0.5f;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    PlayerEnvProbeResult probeResult = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    playerState.gravityAccel = 10.0f;
    playerState.steerBasisRef = {0.0f, 1.0f, 0.0f};

    g_PlayerEnvProbeSampleCount = 1;
    g_PlayerEnvProbeWorldPoints[0] = {0.0f, 10.0f, 0.0f};
    probeResult.candidateScoreBySample[0] = 10.0f;
    playerState.vehiclePitchRad = 0.0f;
    playerState.vehicleRollRad = 0.25f;
    playerState.angVelPitch = 1.0f;
    playerState.angVelRoll = 2.0f;
    Player::ProcessEnvProbeResults(&saveState, &probeResult);
    bool ok = playerState.airborneFlag == 1 &&
              g_PlayerEnvProbe_AboveGroundCount == 0 &&
              g_PlayerEnvProbe_AboveGroundFlags[0] == 0;

    probeResult = {};
    playerState = {};
    saveState.playerState = &playerState;
    playerState.gravityAccel = 10.0f;
    playerState.steerBasisRef = {0.0f, 1.0f, 0.0f};
    g_PlayerEnvProbeSampleCount = 2;
    g_PlayerEnvProbeWorldPoints[0] = {0.0f, 10.0f, 0.0f};
    g_PlayerEnvProbeWorldPoints[1] = {0.0f, 10.0f, 0.0f};
    probeResult.candidateScoreBySample[0] = 9.0f;
    probeResult.candidateScoreBySample[1] = 9.75f;
    probeResult.impactSlotBySample[1] = 3;
    Player::ProcessEnvProbeResults(&saveState, &probeResult);
    ok = ok && playerState.airborneFlag == 0 &&
         g_PlayerEnvProbe_AboveGroundCount == 1 &&
         g_PlayerEnvProbe_AboveGroundIndices[0] == 0 &&
         g_PlayerEnvProbe_AboveGroundFlags[0] == 1 &&
         g_PlayerEnvProbe_AboveGroundFlags[1] == 0 &&
         FloatNear(playerState.worldPos.y, 9.0f) &&
         FloatNear(probeResult.candidateScoreBySample[1], 9.5f);

    for (int i = 0; i < 7; ++i) {
        g_PlayerEnvProbeWorldPoints[i] = oldPoints[i];
    }
    for (int i = 0; i < 10; ++i) {
        g_PlayerEnvProbe_AboveGroundFlags[i] = oldFlags[i];
        g_PlayerEnvProbe_AboveGroundIndices[i] = oldIndices[i];
    }
    g_PlayerEnvProbe_AboveGroundCount = oldAboveGroundCount;
    g_PlayerEnvProbeSampleCount = oldSampleCount;
    g_Player_LavaSinkRate = oldLavaSinkRate;
    g_Player_QuicksandSinkRate = oldQuicksandSinkRate;
    g_Player_NominalGravity = oldNominalGravity;
    g_Player_DeltaTime = oldDeltaTime;
    return ok ? 0 : 1;
}

extern "C" int player_rebuild_orientation_from_normal_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    playerState.steerBasisRef = {0.25f, 0.5f, 0.0f};
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.restartYawRad = 0.0f;
    playerState.worldPos = {3.0f, 4.0f, 5.0f};
    Player::RebuildOrientationFromNormal(&saveState);

    if (!FloatNear(playerState.steerBasisRaw.x, 0.89442718f) ||
        !FloatNear(playerState.steerBasisRaw.y, -0.44721359f) ||
        !FloatNear(playerState.steerBasisRaw.z, 0.0f) ||
        !FloatNear(playerState.vehiclePitchRad, 0.0f) ||
        !FloatNear(playerState.vehicleRollRad, static_cast<float>(asin(-0.25f))) ||
        !FloatNear(playerState.motionBasis.posX, 3.0f) ||
        !FloatNear(playerState.motionBasis.posY, 4.0f) ||
        !FloatNear(playerState.motionBasis.posZ, 5.0f)) {
        return 1;
    }

    playerState.steerBasisRef = {0.0f, 0.0f, 0.0f};
    playerState.steerBasisNorm = {0.0f, 0.0f, 1.0f};
    Player::RebuildOrientationFromNormal(&saveState);
    return FloatNear(playerState.steerBasisRef.y, 0.00100000005f) ? 0 : 2;
}

extern "C" int player_rebuild_steer_basis_raw_from_ref_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    playerState.steerBasisRef = {0.25f, 0.5f, 0.0f};
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.steerBasisRaw = {9.0f, 8.0f, 7.0f};
    Player::RebuildSteerBasisRawFromRef(&saveState);

    if (!FloatNear(playerState.steerBasisRaw.x, 0.89442718f) ||
        !FloatNear(playerState.steerBasisRaw.y, -0.44721359f) ||
        !FloatNear(playerState.steerBasisRaw.z, 0.0f)) {
        return 1;
    }

    playerState.steerBasisRef = {0.0f, 0.0f, 0.0f};
    playerState.steerBasisNorm = {0.0f, 0.0f, 1.0f};
    playerState.steerBasisRaw = {3.0f, 4.0f, 5.0f};
    Player::RebuildSteerBasisRawFromRef(&saveState);

    return playerState.steerBasisRaw.x == 3.0f && playerState.steerBasisRaw.y == 4.0f &&
                   playerState.steerBasisRaw.z == 5.0f
               ? 0
               : 2;
}

extern "C" int player_rebuild_motion_basis_from_steer_basis_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    playerState.steerBasisRaw = {1.0f, 2.0f, 3.0f};
    playerState.steerBasisRef = {4.0f, 5.0f, 6.0f};
    playerState.worldPos = {7.0f, 8.0f, 9.0f};
    Player::RebuildMotionBasisFromSteerBasis(&saveState);

    return FloatNear(playerState.motionBasis.xx, -3.0f) &&
                   FloatNear(playerState.motionBasis.xy, 6.0f) &&
                   FloatNear(playerState.motionBasis.xz, -3.0f) &&
                   FloatNear(playerState.motionBasis.yx, 4.0f) &&
                   FloatNear(playerState.motionBasis.yy, 5.0f) &&
                   FloatNear(playerState.motionBasis.yz, 6.0f) &&
                   FloatNear(playerState.motionBasis.zx, -1.0f) &&
                   FloatNear(playerState.motionBasis.zy, -2.0f) &&
                   FloatNear(playerState.motionBasis.zz, -3.0f) &&
                   FloatNear(playerState.motionBasis.posX, 7.0f) &&
                   FloatNear(playerState.motionBasis.posY, 8.0f) &&
                   FloatNear(playerState.motionBasis.posZ, 9.0f)
               ? 0
               : 1;
}

extern "C" int player_apply_amphib_speed_oscillation_smoke(void) {
    const float oldAccumulatedTimeSec = g_Time_AccumulatedTimeSec;
    g_Time_AccumulatedTimeSec = 2.0f;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;

    playerState.localVel.z = 3.0f;
    playerState.angVelYaw = 0.25f;
    playerState.steerBasisNorm = {0.6f, 0.0f, 0.8f};
    modalData.hoverPitchWaveBaseRate = 1.0f;
    modalData.hoverPitchWaveSpeedRate = 0.25f;
    modalData.hoverPitchWaveAmplitude = 0.3f;
    modalData.hoverRollWaveBaseRate = 0.5f;
    modalData.hoverRollWaveSpeedRate = -0.1f;
    modalData.hoverRollWaveAmplitude = 0.2f;
    modalData.hoverRollYawCoupleScale = 0.4f;

    const zVec3 input = {1.0f, 2.0f, -0.5f};

    const float speedAbs = std::fabs(playerState.localVel.z);
    const float pitchArg =
        (modalData.hoverPitchWaveSpeedRate * speedAbs + modalData.hoverPitchWaveBaseRate) *
        g_Time_AccumulatedTimeSec;
    const float rollArg =
        (modalData.hoverRollWaveSpeedRate * speedAbs + modalData.hoverRollWaveBaseRate) *
        g_Time_AccumulatedTimeSec;
    const float pitchAngle =
        static_cast<float>(std::sin(pitchArg)) * modalData.hoverPitchWaveAmplitude;
    const float rollAngleBase =
        static_cast<float>(std::sin(rollArg)) * modalData.hoverRollWaveAmplitude;

    const float yawSin = -playerState.steerBasisNorm.x;
    const float yawCos = -playerState.steerBasisNorm.z;
    const float pitchSin = static_cast<float>(std::sin(pitchAngle));
    const float pitchCos = static_cast<float>(std::cos(pitchAngle));

    zVec3 expected[2] = {};
    for (int i = 0; i < 2; ++i) {
        float rollAngle = rollAngleBase;
        if (i != 0) {
            rollAngle +=
                playerState.angVelYaw * modalData.hoverRollYawCoupleScale * playerState.localVel.z;
        }
        const float rollSin = static_cast<float>(std::sin(rollAngle));
        const float rollCos = static_cast<float>(std::cos(rollAngle));
        const float xx = yawSin * pitchSin * rollSin + rollCos * yawCos;
        const float xy = rollSin * pitchCos;
        const float xz = rollSin * yawCos * pitchSin - rollCos * yawSin;
        const float yx = yawSin * pitchSin * rollCos - rollSin * yawCos;
        const float yy = rollCos * pitchCos;
        const float yz = rollCos * yawCos * pitchSin + rollSin * yawSin;
        const float zx = yawSin * pitchCos;
        const float zy = -pitchSin;
        const float zz = yawCos * pitchCos;
        expected[i].x = input.x * xx + input.y * yx + input.z * zx;
        expected[i].y = input.x * xy + input.y * yy + input.z * zy;
        expected[i].z = input.x * xz + input.y * yz + input.z * zz;
    }

    zVec3 uncoupled = input;
    Player::ApplyAmphibSpeedOscillation(&saveState, &uncoupled, 0);
    zVec3 coupled = input;
    Player::ApplyAmphibSpeedOscillation(&saveState, &coupled, 1);

    const bool ok =
        FloatNear(uncoupled.x, expected[0].x) && FloatNear(uncoupled.y, expected[0].y) &&
        FloatNear(uncoupled.z, expected[0].z) && FloatNear(coupled.x, expected[1].x) &&
        FloatNear(coupled.y, expected[1].y) && FloatNear(coupled.z, expected[1].z) &&
        !FloatNear(coupled.z, uncoupled.z);

    g_Time_AccumulatedTimeSec = oldAccumulatedTimeSec;
    return ok ? 0 : 1;
}

extern "C" int player_find_third_probe_and_compute_normal_smoke(void) {
    const float oldDeltaTime = g_Player_DeltaTime;
    const int oldSampleCount = g_PlayerEnvProbeSampleCount;
    const int oldIndex0 = g_PlayerEnvProbe_AboveGroundIndices[0];
    const int oldIndex1 = g_PlayerEnvProbe_AboveGroundIndices[1];
    const zVec3 oldPoint0 = g_PlayerEnvProbeWorldPoints[0];
    const zVec3 oldPoint1 = g_PlayerEnvProbeWorldPoints[1];
    const zVec3 oldPoint2 = g_PlayerEnvProbeWorldPoints[2];

    g_Player_DeltaTime = 0.1f;
    g_PlayerEnvProbeSampleCount = 4;
    g_PlayerEnvProbe_AboveGroundIndices[0] = 0;
    g_PlayerEnvProbe_AboveGroundIndices[1] = 1;
    g_PlayerEnvProbeWorldPoints[0] = {0.0f, 50.0f, 0.0f};
    g_PlayerEnvProbeWorldPoints[1] = {2.0f, 75.0f, 0.0f};
    g_PlayerEnvProbeWorldPoints[2] = {0.0f, 25.0f, -2.0f};

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    PlayerEnvProbeResult probeResult = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    playerState.motionBasis.xx = 1.0f;
    playerState.motionBasis.yy = 1.0f;
    playerState.motionBasis.zz = 1.0f;
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    modalData.probePoints[17] = {0.0f, 0.0f, -2.0f};
    modalData.probePoints[18] = {0.0f, 3.0f, -3.0f};
    probeResult.candidateScoreBySample[0] = 2.0f;
    probeResult.candidateScoreBySample[1] = 4.0f;
    probeResult.candidateScoreBySample[2] = 2.0f;
    probeResult.candidateScoreBySample[3] = 2.0f;

    Player::FindThirdProbeAndComputeNormal(&saveState, &probeResult);

    const float invSqrt2 = 0.70710677f;
    const bool ok =
        FloatNear(playerState.steerBasisRef.x, -invSqrt2) &&
        FloatNear(playerState.steerBasisRef.y, invSqrt2) &&
        FloatNear(playerState.steerBasisRef.z, 0.0f) &&
        FloatNear(playerState.worldPos.y, 2.0f) &&
        FloatNear(playerState.vehicleRollRad, static_cast<float>(asin(invSqrt2))) &&
        FloatNear(playerState.motionBasis.posY, 2.0f);

    g_PlayerEnvProbeWorldPoints[2] = oldPoint2;
    g_PlayerEnvProbeWorldPoints[1] = oldPoint1;
    g_PlayerEnvProbeWorldPoints[0] = oldPoint0;
    g_PlayerEnvProbe_AboveGroundIndices[1] = oldIndex1;
    g_PlayerEnvProbe_AboveGroundIndices[0] = oldIndex0;
    g_PlayerEnvProbeSampleCount = oldSampleCount;
    g_Player_DeltaTime = oldDeltaTime;
    return ok ? 0 : 1;
}

extern "C" int player_accumulate_slope_forces_smoke(void) {
    const float oldDeltaTime = g_Player_DeltaTime;
    const float oldMaxSlope = g_Player_MaxSlope;
    const int oldAboveGroundCount = g_PlayerEnvProbe_AboveGroundCount;
    int oldIndices[10] = {};
    for (int i = 0; i < 10; ++i) {
        oldIndices[i] = g_PlayerEnvProbe_AboveGroundIndices[i];
    }

    g_Player_DeltaTime = 0.1f;
    g_Player_MaxSlope = 0.8f;
    g_PlayerEnvProbe_AboveGroundCount = 4;
    g_PlayerEnvProbe_AboveGroundIndices[0] = 2;
    g_PlayerEnvProbe_AboveGroundIndices[1] = 0;
    g_PlayerEnvProbe_AboveGroundIndices[2] = 1;
    g_PlayerEnvProbe_AboveGroundIndices[3] = 3;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerEnvProbeResult probeResult = {};
    saveState.playerState = &playerState;
    playerState.gravityAccel = 10.0f;
    playerState.projectileSpawnVel = {1.0f, 2.0f, 3.0f};
    probeResult.bestIndexBySample[0] = 1;
    probeResult.bestIndexBySample[1] = 2;
    probeResult.bestIndexBySample[2] = 3;
    probeResult.bestIndexBySample[3] = 4;
    probeResult.candidateBuffers[2].entries[3].surfaceNormal = {0.1f, 0.5f, 0.2f};
    probeResult.candidateBuffers[0].entries[1].surfaceNormal = {3.0f, 0.9f, 3.0f};
    probeResult.candidateBuffers[1].entries[2].surfaceNormal = {-0.2f, 0.7f, 0.3f};
    probeResult.candidateBuffers[3].entries[4].surfaceNormal = {10.0f, 0.1f, 10.0f};

    Player::AccumulateSlopeForces(&saveState, &probeResult);
    const bool ok =
        FloatNear(playerState.projectileSpawnVel.x, 0.5f) &&
        FloatNear(playerState.projectileSpawnVel.y, -2.0f) &&
        FloatNear(playerState.projectileSpawnVel.z, 5.5f);

    for (int i = 0; i < 10; ++i) {
        g_PlayerEnvProbe_AboveGroundIndices[i] = oldIndices[i];
    }
    g_PlayerEnvProbe_AboveGroundCount = oldAboveGroundCount;
    g_Player_MaxSlope = oldMaxSlope;
    g_Player_DeltaTime = oldDeltaTime;
    return ok ? 0 : 1;
}

extern "C" int player_update_vertical_velocity_and_transform_smoke(void) {
    const float oldDeltaTime = g_Player_DeltaTime;
    const float oldInvDeltaTime = g_Player_InvDeltaTime;
    const float oldMaxSlope = g_Player_MaxSlope;
    const int oldAboveGroundCount = g_PlayerEnvProbe_AboveGroundCount;

    g_Player_DeltaTime = 0.1f;
    g_Player_InvDeltaTime = 10.0f;
    g_Player_MaxSlope = -1.0f;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerEnvProbeResult probeResult = {};
    saveState.playerState = &playerState;
    playerState.motionBasis.xx = 1.0f;
    playerState.motionBasis.yy = 1.0f;
    playerState.motionBasis.zz = 1.0f;
    playerState.worldPos.y = 6.0f;
    playerState.previousTransform.posY = 5.0f;
    playerState.projectileSpawnVel = {1.0f, 2.0f, 3.0f};
    g_PlayerEnvProbe_AboveGroundCount = 1;

    Player::UpdateVerticalVelocityAndTransform(&saveState, &probeResult);
    const float blend = PlayerDampingFactor(5.0f, 0.1f);
    const float expectedY = blend * 2.0f + (1.0f - blend) * 10.0f;
    bool ok = FloatNear(playerState.projectileSpawnVel.y, expectedY) &&
              FloatNear(playerState.localVel.x, 1.0f) &&
              FloatNear(playerState.localVel.y, expectedY) &&
              FloatNear(playerState.localVel.z, 3.0f);

    playerState = {};
    saveState.playerState = &playerState;
    playerState.motionBasis.xx = 1.0f;
    playerState.motionBasis.yy = 1.0f;
    playerState.motionBasis.zz = 1.0f;
    playerState.worldPos.y = 12.0f;
    playerState.previousTransform.posY = 5.0f;
    playerState.projectileSpawnVel = {4.0f, 2.0f, 6.0f};
    g_PlayerEnvProbe_AboveGroundCount = 3;
    Player::UpdateVerticalVelocityAndTransform(&saveState, &probeResult);
    ok = ok && FloatNear(playerState.projectileSpawnVel.y, 0.0f) &&
         FloatNear(playerState.localVel.x, 4.0f) &&
         FloatNear(playerState.localVel.y, 0.0f) &&
         FloatNear(playerState.localVel.z, 6.0f);

    g_PlayerEnvProbe_AboveGroundCount = oldAboveGroundCount;
    g_Player_MaxSlope = oldMaxSlope;
    g_Player_InvDeltaTime = oldInvDeltaTime;
    g_Player_DeltaTime = oldDeltaTime;
    return ok ? 0 : 1;
}

extern "C" int player_update_post_move_environment_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeDiScene = g_Player_RuntimeDiScene;
    const float oldDeltaTime = g_Player_DeltaTime;
    const float oldInvDeltaTime = g_Player_InvDeltaTime;
    const float oldNominalGravity = g_Player_NominalGravity;
    const float oldMaxSlope = g_Player_MaxSlope;
    const int oldSampleCount = g_PlayerEnvProbeSampleCount;
    const int oldAboveGroundCount = g_PlayerEnvProbe_AboveGroundCount;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;
    zVec3 oldProbePoints[7];
    int oldAboveGroundFlags[10];
    int oldAboveGroundIndices[10];
    for (int i = 0; i < 7; ++i) {
        oldProbePoints[i] = g_PlayerEnvProbeWorldPoints[i];
    }
    for (int i = 0; i < 10; ++i) {
        oldAboveGroundFlags[i] = g_PlayerEnvProbe_AboveGroundFlags[i];
        oldAboveGroundIndices[i] = g_PlayerEnvProbe_AboveGroundIndices[i];
    }

    g_Player_DeltaTime = 0.2f;
    g_Player_InvDeltaTime = 5.0f;
    g_Player_NominalGravity = 10.0f;
    g_Player_MaxSlope = -1.0f;
    g_Variant_CurrentTag = {};
    g_VariantTag_Current = {};

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage globalPlayerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData masterModalData = {};
    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial globalRootNode = {};
    zClass_NodePartial worldNode = {};
    zClass_WorldDataPartial worldData = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &masterModalData;
    modalState.modalStateCode = 1;
    playerState.rootNode = &rootNode;
    globalPlayerState.rootNode = &globalRootNode;
    gameStateOrMap.playerState =
        static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&globalPlayerState));
    g_GameStateOrMapTable = &gameStateOrMap;
    worldNode.classData = &worldData;
    g_Player_RuntimeDiScene = &worldNode;

    playerState.motionBasis.xx = 1.0f;
    playerState.motionBasis.yy = 1.0f;
    playerState.motionBasis.zz = 1.0f;
    playerState.worldPos = {10.0f, 2.0f, 30.0f};
    playerState.previousTransform.posY = 2.0f;
    playerState.projectileSpawnVel = {0.0f, 4.0f, 0.0f};
    playerState.gravityAccel = 10.0f;
    playerState.angVelPitch = 0.5f;
    playerState.angVelRoll = -0.25f;
    masterModalData.probePoints[15] = {0.0f, 0.0f, 0.0f};

    Player::UpdatePostMoveEnvironment(&saveState, 1);

    const bool ok = g_PlayerEnvProbeSampleCount == 1 &&
                    g_PlayerEnvProbe_AboveGroundCount == 0 &&
                    playerState.airborneFlag == 1 &&
                    std::fabs(playerState.vehiclePitchRad - 0.1f) < 0.001f &&
                    FloatNear(playerState.vehicleRollRad, -0.05f) &&
                    FloatNear(playerState.projectileSpawnVel.y, 2.0f) &&
                    FloatNear(playerState.worldPos.y, 2.4f) &&
                    FloatNear(playerState.motionBasis.posX, 10.0f) &&
                    FloatNear(playerState.motionBasis.posY, 2.4f) &&
                    FloatNear(playerState.motionBasis.posZ, 30.0f) &&
                    FloatNear(playerState.gravityAccel, 10.0f);

    for (int i = 0; i < 7; ++i) {
        g_PlayerEnvProbeWorldPoints[i] = oldProbePoints[i];
    }
    for (int i = 0; i < 10; ++i) {
        g_PlayerEnvProbe_AboveGroundFlags[i] = oldAboveGroundFlags[i];
        g_PlayerEnvProbe_AboveGroundIndices[i] = oldAboveGroundIndices[i];
    }
    g_VariantTag_Current = oldVariantTagCurrent;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_PlayerEnvProbe_AboveGroundCount = oldAboveGroundCount;
    g_PlayerEnvProbeSampleCount = oldSampleCount;
    g_Player_MaxSlope = oldMaxSlope;
    g_Player_NominalGravity = oldNominalGravity;
    g_Player_InvDeltaTime = oldInvDeltaTime;
    g_Player_DeltaTime = oldDeltaTime;
    g_Player_RuntimeDiScene = oldRuntimeDiScene;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    return ok ? 0 : 1;
}

extern "C" int player_update_master_type_track_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const float oldDeltaTime = g_Player_DeltaTime;
    const float oldInvDeltaTime = g_Player_InvDeltaTime;

    g_GameStateOrMapTable = nullptr;
    g_Player_DeltaTime = 0.1f;
    g_Player_InvDeltaTime = 10.0f;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    PlayerMasterCommonData commonData = {};
    zClass_NodePartial rootNode = {};
    zClass_Object3DDataPartial rootData = {};
    zClass_NodePartial modeVariantNode = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    modalData.masterType = 2;
    playerState.masterCommonData = &commonData;
    playerState.rootNode = &rootNode;
    playerState.modeVariantNode = &modeVariantNode;
    InitObjectPositionNode(&rootNode, &rootData, 0.0f, 0.0f, 0.0f);
    rootData.localMatrix[0] = 1.0f;
    rootData.localMatrix[4] = 1.0f;
    rootData.localMatrix[8] = 1.0f;

    playerState.airborneFlag = 1;
    playerState.airborneFlagPrev = 0;
    playerState.projectileSpawnVel = {10.0f, 2.0f, 20.0f};
    playerState.worldPos = {1.0f, 5.0f, 3.0f};
    playerState.fxOffsetLocal = {0.5f, 1.0f, 1.5f};

    Player::UpdateMasterTypeTrack(&saveState);

    const float damping = PlayerDampingFactor(0.200000003f, 0.1f);
    const float expectedX = 10.0f * damping;
    const float expectedZ = 20.0f * damping;
    const bool ok =
        playerState.airborneFlagPrev == 1 &&
        FloatNear(playerState.projectileSpawnVel.x, expectedX) &&
        FloatNear(playerState.projectileSpawnVel.y, 2.0f) &&
        FloatNear(playerState.projectileSpawnVel.z, expectedZ) &&
        FloatNear(playerState.worldPos.x, 1.0f + expectedX * 0.1f) &&
        FloatNear(playerState.worldPos.y, 5.0f) &&
        FloatNear(playerState.worldPos.z, 3.0f + expectedZ * 0.1f) &&
        FloatNear(playerState.motionBasis.posX, playerState.worldPos.x) &&
        FloatNear(playerState.motionBasis.posY, playerState.worldPos.y) &&
        FloatNear(playerState.motionBasis.posZ, playerState.worldPos.z) &&
        FloatNear(playerState.fxOffsetWorld.x, playerState.worldPos.x + 0.5f) &&
        FloatNear(playerState.fxOffsetWorld.y, playerState.worldPos.y + 1.0f) &&
        FloatNear(playerState.fxOffsetWorld.z, playerState.worldPos.z + 1.5f) &&
        FloatNear(playerState.previousTransform.posX, rootData.localMatrix[9]) &&
        FloatNear(playerState.previousTransform.posY, rootData.localMatrix[10]) &&
        FloatNear(playerState.previousTransform.posZ, rootData.localMatrix[11]);

    g_Player_InvDeltaTime = oldInvDeltaTime;
    g_Player_DeltaTime = oldDeltaTime;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    return ok ? 0 : 1;
}

extern "C" int player_update_master_type_basic_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldPlayerDeltaTimeScaled001 = g_Player_DeltaTimeScaled001;

    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage globalPlayerState = {};
    PlayerMasterModalData modalData = {};
    PlayerModalState modalState = {};
    zUtil_SaveGameState saveState = {};
    zClass_Object3DDataPartial objectData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial globalRootNode = {};
    zClass_WorldDataPartial worldData = {};
    zClass_NodePartial worldNode = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    modalState.modalStateCode = 0;

    rootNode.classId = 5;
    rootNode.classData = &objectData;
    playerState.rootNode = &rootNode;
    globalPlayerState.rootNode = &globalRootNode;
    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    gameStateOrMap.playerState =
        static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&globalPlayerState));
    g_GameStateOrMapTable = &gameStateOrMap;
    worldNode.classData = &worldData;
    g_Player_RuntimeDiScene = &worldNode;

    objectData.scale = {1.0f, 1.0f, 1.0f};
    objectData.localMatrix[0] = 1.0f;
    objectData.localMatrix[4] = 1.0f;
    objectData.localMatrix[8] = 1.0f;

    modalData.maxSpeed = 100.0f;
    modalData.yawRateMax = 12.0f;
    modalData.accelRate = 0.0f;
    modalData.yawAccel = 0.0f;
    modalData.yawDamping = 0.0f;
    modalData.rateDampingAccel = 0.0f;
    modalData.rateDampingDecel = 0.0f;
    modalData.modeAltTransitionTime = 7.0f;

    playerState.worldPos = {10.0f, 5.0f, 20.0f};
    playerState.localVel = {2.0f, 3.0f, 4.0f};
    playerState.throttleInputCopy = 1.0f;
    playerState.fxOffsetLocal = {1.0f, 2.0f, 3.0f};
    playerState.cameraState = 0;
    g_Player_DeltaTime = 0.5f;
    g_Player_DeltaTimeScaled001 = 0.0f;

    Player::UpdateMasterTypeBasic(&saveState);

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_Player_DeltaTimeScaled001 = oldPlayerDeltaTimeScaled001;

    if (!FloatNear(playerState.worldPos.x, 11.0f) ||
        !FloatNear(playerState.worldPos.y, 7.0f) ||
        !FloatNear(playerState.worldPos.z, 22.0f) ||
        !FloatNear(playerState.projectileSpawnVel.x, 2.0f) ||
        !FloatNear(playerState.projectileSpawnVel.y, 3.0f) ||
        !FloatNear(playerState.projectileSpawnVel.z, 4.0f) ||
        playerState.axisClampRuntime != 100.0f || playerState.yawVelocityLimit != 12.0f ||
        playerState.vehiclePitchRad != 0.0f || playerState.vehicleRollRad != 0.0f) {
        return 1;
    }

    if (!FloatNear(playerState.fxOffsetWorld.x, 12.0f) ||
        !FloatNear(playerState.fxOffsetWorld.y, 9.0f) ||
        !FloatNear(playerState.fxOffsetWorld.z, 25.0f)) {
        return 2;
    }
    if (!FloatNear(playerState.previousTransform.posX, objectData.localMatrix[9]) ||
        !FloatNear(playerState.previousTransform.posY, objectData.localMatrix[10]) ||
        !FloatNear(playerState.previousTransform.posZ, objectData.localMatrix[11])) {
        return 4;
    }

    return FloatNear(playerState.bankBasis.x, 0.0f) &&
                   FloatNear(playerState.bankBasis.y, 0.0f) &&
                   FloatNear(playerState.bankBasis.z, -1.0f) &&
                   playerState.cachedPitchRad == playerState.vehiclePitchRad &&
                   playerState.cachedYawRad == playerState.restartYawRad &&
                   playerState.cachedRollRad == playerState.vehicleRollRad
               ? 0
               : 3;
}

extern "C" int player_zar_write_mission_save_data_section_smoke(void) {
    char tempPath[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    GetTempPathA(sizeof(tempPath), tempPath);
    GetTempFileNameA(tempPath, "pms", 0, tempFile);

    HANDLE const file =
        CreateFileA(tempFile, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return 1;
    }

    zZbdManager manager = {};
    manager.indexArchive.hFile = file;

    zZbdSectionHandler handler = {};
    handler.sectionName = "Player";
    zZbdSectionCallbackCtx callbackCtx = {};
    callbackCtx.manager = &manager;
    callbackCtx.sectionHandler = &handler;

    zClass_NodePartial rootNode = {};
    std::strcpy(rootNode.name, "local_player");
    zClass_NodePartial cameraNode = {};
    zClass_CameraDataPartial cameraData = {};
    PlayerMasterModalData modalData = {};
    PlayerModalState modalState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_SaveGameState saveState = {};
    OptCatalogEntryDef hitSource = {};

    playerState.rootNode = &rootNode;
    playerState.activeAltGunController = &playerState.altWeaponBanks[1].controllerA;
    playerState.activePrimaryGunController = &playerState.altWeaponBanks[2].controllerB;
    playerState.altWeaponBanks[1].controllerA.weaponBankIndex = 1;
    playerState.altWeaponBanks[1].controllerA.weaponSideIndex = 0;
    playerState.altWeaponBanks[2].controllerB.weaponBankIndex = 2;
    playerState.altWeaponBanks[2].controllerB.weaponSideIndex = 1;
    playerState.timedHitStatus.runtimeFlags = 1;
    playerState.timedHitStatus.hitSource = &hitSource;
    playerState.timedHitStatus.nextUpdateTime = 100.0f;
    hitSource.ordinalIndex = 88;

    modalData.masterType = 66;
    modalState.masterModalData = &modalData;
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    cameraData.targetOrEuler = {1.0f, 2.0f, 3.0f};
    cameraData.posOffset = {4.0f, 5.0f, 6.0f};

    zUtil_SaveGameState *const oldLocalSaveState = g_LocalPlayerSaveState;
    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    const zTag4Partial oldVariantTag = g_Variant_CurrentTag;
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;
    g_LocalPlayerSaveState = &saveState;
    g_MainCamera = &cameraNode;
    g_Variant_CurrentTag.count = 2;
    g_Variant_CurrentTag.tags[0] = 7;
    g_Variant_CurrentTag.tags[1] = 8;
    g_Variant_CurrentTag.tags[2] = 0xff;
    g_Time_AccumulatedTimeSec = 90.0f;

    const int result = Player::ZAR_WriteMissionSaveDataSection(&callbackCtx, nullptr);

    PlayerMissionSaveData readBack = {};
    DWORD read = 0;
    SetFilePointer(file, 0, nullptr, FILE_BEGIN);
    ReadFile(file, &readBack, sizeof(readBack), &read, nullptr);

    const bool ok = result == 1 && read == sizeof(readBack) && manager.indexArchive.recordCount == 1 &&
                    manager.indexArchive.records != nullptr &&
                    std::strcmp(manager.indexArchive.records[0].name, "Player/local_player") == 0 &&
                    readBack.size == sizeof(PlayerMissionSaveData) &&
                    readBack.primaryWeaponBankIndex == 2 &&
                    readBack.primaryWeaponSideIndex == 1 &&
                    readBack.playerMasterType == 66 &&
                    Vec3Equals(readBack.cameraTarget, cameraData.targetOrEuler) &&
                    Vec3Equals(readBack.cameraPosition, cameraData.posOffset) &&
                    readBack.lastValidCameraVariantTag.count == 2 &&
                    readBack.lastValidCameraVariantTag.tags[0] == 7 &&
                    readBack.lastValidCameraVariantTag.tags[1] == 8 &&
                    readBack.lastValidCameraVariantTag.tags[2] == 0xff &&
                    readBack.timedHitStatus.savedHitSourceEntryId == 88 &&
                    readBack.timedHitStatus.nextUpdateTime == 10.0f;

    g_LocalPlayerSaveState = oldLocalSaveState;
    g_MainCamera = oldMainCamera;
    g_Variant_CurrentTag = oldVariantTag;
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    std::free(manager.indexArchive.records);
    manager.indexArchive.records = nullptr;
    CloseHandle(file);
    return ok ? 0 : 2;
}

extern "C" int player_restore_recorded_node_flags_smoke(void) {
    zClass_NodePartial untouched = {};
    zClass_NodePartial allFlags = {};
    zClass_NodePartial rayOnly = {};
    untouched.flags = 0;
    allFlags.flags = 0;
    rayOnly.flags = 0x20;

    PlayerNodeFlagRestoreEntry entries[3] = {};
    entries[0].node = &untouched;
    entries[1].node = &allFlags;
    entries[1].wasCellPickable = 1;
    entries[1].wasRaycastable = 1;
    entries[1].wasPickable = 1;
    entries[2].node = &rayOnly;
    entries[2].wasRaycastable = 1;

    PlayerNodeFlagRestoreEntry *const oldBegin = g_PlayerNodeFlagRestoreEntriesBegin;
    PlayerNodeFlagRestoreEntry *const oldEnd = g_PlayerNodeFlagRestoreEntriesEnd;
    PlayerNodeFlagRestoreEntry *const oldCapacity = g_PlayerNodeFlagRestoreEntriesCapacityEnd;
    g_PlayerNodeFlagRestoreEntriesBegin = entries;
    g_PlayerNodeFlagRestoreEntriesEnd = entries + 3;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = entries + 3;

    Player::RestoreRecordedNodeFlags();

    const bool ok = untouched.flags == 0 && (allFlags.flags & 0x38) == 0x38 &&
                    (rayOnly.flags & 0x10) != 0 && (rayOnly.flags & 0x20) != 0 &&
                    (rayOnly.flags & 0x08) == 0;

    g_PlayerNodeFlagRestoreEntriesBegin = oldBegin;
    g_PlayerNodeFlagRestoreEntriesEnd = oldEnd;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = oldCapacity;
    return ok ? 0 : 1;
}

extern "C" int player_record_node_flags_for_restore_smoke(void) {
    PlayerNodeFlagRestoreEntry *const oldBegin = g_PlayerNodeFlagRestoreEntriesBegin;
    PlayerNodeFlagRestoreEntry *const oldEnd = g_PlayerNodeFlagRestoreEntriesEnd;
    PlayerNodeFlagRestoreEntry *const oldCapacity = g_PlayerNodeFlagRestoreEntriesCapacityEnd;
    g_PlayerNodeFlagRestoreEntriesBegin = nullptr;
    g_PlayerNodeFlagRestoreEntriesEnd = nullptr;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = nullptr;

    zClass_NodePartial first = {};
    zClass_NodePartial second = {};
    zClass_NodePartial third = {};
    first.flags = 0x28;
    second.flags = 0x10;
    third.flags = 0x38;

    Player::RecordNodeFlagsForRestore(&first);
    const bool firstOk = g_PlayerNodeFlagRestoreEntriesEnd ==
                             g_PlayerNodeFlagRestoreEntriesBegin + 1 &&
                         g_PlayerNodeFlagRestoreEntriesCapacityEnd ==
                             g_PlayerNodeFlagRestoreEntriesBegin + 1 &&
                         g_PlayerNodeFlagRestoreEntriesBegin[0].node == &first &&
                         g_PlayerNodeFlagRestoreEntriesBegin[0].wasCellPickable == 1 &&
                         g_PlayerNodeFlagRestoreEntriesBegin[0].wasRaycastable == 0 &&
                         g_PlayerNodeFlagRestoreEntriesBegin[0].wasPickable == 1;

    Player::RecordNodeFlagsForRestore(&second);
    const bool secondOk = g_PlayerNodeFlagRestoreEntriesEnd ==
                              g_PlayerNodeFlagRestoreEntriesBegin + 2 &&
                          g_PlayerNodeFlagRestoreEntriesCapacityEnd ==
                              g_PlayerNodeFlagRestoreEntriesBegin + 2 &&
                          g_PlayerNodeFlagRestoreEntriesBegin[1].node == &second &&
                          g_PlayerNodeFlagRestoreEntriesBegin[1].wasCellPickable == 0 &&
                          g_PlayerNodeFlagRestoreEntriesBegin[1].wasRaycastable == 1 &&
                          g_PlayerNodeFlagRestoreEntriesBegin[1].wasPickable == 0;

    Player::RecordNodeFlagsForRestore(&third);
    const bool thirdOk = g_PlayerNodeFlagRestoreEntriesEnd ==
                             g_PlayerNodeFlagRestoreEntriesBegin + 3 &&
                         g_PlayerNodeFlagRestoreEntriesCapacityEnd ==
                             g_PlayerNodeFlagRestoreEntriesBegin + 4 &&
                         g_PlayerNodeFlagRestoreEntriesBegin[0].node == &first &&
                         g_PlayerNodeFlagRestoreEntriesBegin[2].node == &third &&
                         g_PlayerNodeFlagRestoreEntriesBegin[2].wasCellPickable == 1 &&
                         g_PlayerNodeFlagRestoreEntriesBegin[2].wasRaycastable == 1 &&
                         g_PlayerNodeFlagRestoreEntriesBegin[2].wasPickable == 1;

    ::operator delete(g_PlayerNodeFlagRestoreEntriesBegin);
    g_PlayerNodeFlagRestoreEntriesBegin = oldBegin;
    g_PlayerNodeFlagRestoreEntriesEnd = oldEnd;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = oldCapacity;
    return firstOk && secondOk && thirdOk ? 0 : 1;
}

extern "C" int zutil_save_game_state_free_owned_resources_smoke(void) {
    zUtil_SaveGameState saveState = {};
    saveState.playerState = AllocZeroedMalloc<zUtil_PlayerStateStorage>();

    GameNetPlayerRow row = {};
    row.saveState = reinterpret_cast<GameNetPlayerSaveState *>(&saveState);
    saveState.netPlayerRow = &row;

    PlayerModalState *const firstModal = AllocZeroedMalloc<PlayerModalState>();
    PlayerModalState *const secondModal = AllocZeroedMalloc<PlayerModalState>();
    firstModal->next = secondModal;
    saveState.primaryModalState = firstModal;
    saveState.modalStateListHead = firstModal;
    saveState.modalStateListTail = secondModal;
    saveState.modalStateCount = 2;
    saveState.modalStateListAux = 1;

    saveState.FreeOwnedResources();

    return row.saveState == nullptr && saveState.primaryModalState == nullptr &&
                   saveState.modalStateListHead == nullptr &&
                   saveState.modalStateListTail == nullptr && saveState.modalStateCount == 0 &&
                   saveState.modalStateListAux == 0
               ? 0
               : 1;
}

extern "C" int player_zar_register_sections_smoke(void) {
    zZbdManager *const oldManager = g_zUtil_ZbdManager;
    const int oldRuntimeInputFlags = g_Player_RuntimeInputFlags;

    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager = MakePlayerZbdManager(sentinel);
    g_zUtil_ZbdManager = &manager;
    g_Player_RuntimeInputFlags = 99;

    Player::ZAR_RegisterSections();

    zZbdSectionHandlerNode *const vehicleNode = sentinel.next;
    zZbdSectionHandlerNode *const playerNode = vehicleNode != &sentinel ? vehicleNode->next : &sentinel;
    const bool ok =
        g_Player_RuntimeInputFlags == 0 && manager.sectionHandlerCount == 2 &&
        vehicleNode != &sentinel && playerNode != &sentinel && playerNode->next == &sentinel &&
        std::strcmp(vehicleNode->sectionHandler.sectionName, "VehicleList") == 0 &&
        vehicleNode->sectionHandler.onPreLoad ==
            TestZbdCallbackPtr(&Player::ZAR_WriteVehicleListSection) &&
        vehicleNode->sectionHandler.onDataReady ==
            TestZbdCallbackPtr(&Player::ZAR_ReadVehicleListSection) &&
        vehicleNode->sectionHandler.sortOrder == 100 &&
        vehicleNode->sectionHandler.userData == nullptr &&
        std::strcmp(playerNode->sectionHandler.sectionName, "Player") == 0 &&
        playerNode->sectionHandler.onPreLoad ==
            TestZbdCallbackPtr(&Player::ZAR_WriteMissionSaveDataSection) &&
        playerNode->sectionHandler.onDataReady ==
            TestZbdCallbackPtr(&Player::ZAR_ReadMissionSaveDataSection) &&
        playerNode->sectionHandler.sortOrder == 200 &&
        playerNode->sectionHandler.userData == nullptr;

    ClearPlayerRegisteredHandlers(sentinel);
    g_zUtil_ZbdManager = oldManager;
    g_Player_RuntimeInputFlags = oldRuntimeInputFlags;
    return ok ? 0 : 1;
}

extern "C" int player_zar_write_vehicle_list_section_smoke(void) {
    char tempPath[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    GetTempPathA(sizeof(tempPath), tempPath);
    GetTempFileNameA(tempPath, "pvl", 0, tempFile);

    HANDLE const file =
        CreateFileA(tempFile, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return 1;
    }

    zZbdManager manager = {};
    manager.indexArchive.hFile = file;

    zZbdSectionHandler handler = {};
    handler.sectionName = "VehicleList";
    zZbdSectionCallbackCtx callbackCtx = {};
    callbackCtx.manager = &manager;
    callbackCtx.sectionHandler = &handler;

    zClass_NodePartial rootNode = {};
    std::strcpy(rootNode.name, "local_vehicle");

    PlayerMasterModalData modalData = {};
    modalData.masterType = 77;
    PlayerModalState modalState = {};
    modalState.masterModalData = &modalData;

    zUtil_PlayerStateStorage playerState = {};
    playerState.rootNode = &rootNode;
    playerState.vehicleRotationAngles = {1.0f, 2.0f, 3.0f};
    playerState.worldPos = {4.0f, 5.0f, 6.0f};
    playerState.aiNetId = 1001;
    playerState.aiTopLevelState = 11;
    playerState.aiSavedTopLevelState = 12;
    playerState.aiReturnTopLevelState = 13;
    playerState.aiAttackRadiusSq = 14.0f;
    playerState.aiRestoreDistanceSq = 15.0f;
    playerState.aiRestoreTarget = {16.0f, 17.0f, 18.0f};
    playerState.aiDynamicOffsetDir = {19.0f, 20.0f, 21.0f};
    playerState.aiActivationRadiusSq = 22.0f;
    playerState.aiTickSuppressed = 23;
    playerState.recentHitFlag = 24;
    playerState.recentHitMarkerHandle = 25;
    playerState.aiActive = 26;
    playerState.aiPathCursorAdvanceRequested = 27;
    playerState.aiCurrentSteeringSubstate = 28;
    playerState.aiReturnSteeringSubstate = 29;
    playerState.masterType = 30;
    playerState.statusMeterScaled = 31.0f;
    playerState.statusMeterValue = 32.0f;
    playerState.nanitePanelLevel = 33;

    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    zUtil_SaveGameState *const oldHead = g_PlayerSaveStateListHead;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    g_PlayerSaveStateListHead = &saveState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;

    const int result = Player::ZAR_WriteVehicleListSection(&callbackCtx, nullptr);

    PlayerVehicleListSaveEntry readBack = {};
    DWORD read = 0;
    SetFilePointer(file, 0, nullptr, FILE_BEGIN);
    ReadFile(file, &readBack, sizeof(readBack), &read, nullptr);

    const bool ok = result == 1 && read == sizeof(readBack) && manager.indexArchive.recordCount == 1 &&
                    manager.indexArchive.records != nullptr &&
                    std::strcmp(manager.indexArchive.records[0].name,
                                "VehicleList/local_vehicle") == 0 &&
                    readBack.size == 128 &&
                    Vec3Equals(readBack.vehicleRotationAngles, playerState.vehicleRotationAngles) &&
                    Vec3Equals(readBack.worldPos, playerState.worldPos) &&
                    readBack.aiNetId == playerState.aiNetId &&
                    readBack.aiTopLevelState == playerState.aiTopLevelState &&
                    readBack.aiSavedTopLevelState == playerState.aiSavedTopLevelState &&
                    readBack.aiReturnTopLevelState == playerState.aiReturnTopLevelState &&
                    readBack.aiAttackRadiusSq == playerState.aiAttackRadiusSq &&
                    readBack.aiRestoreDistanceSq == playerState.aiRestoreDistanceSq &&
                    Vec3Equals(readBack.aiRestoreTarget, playerState.aiRestoreTarget) &&
                    Vec3Equals(readBack.aiDynamicOffsetDir, playerState.aiDynamicOffsetDir) &&
                    readBack.aiActivationRadiusSq == playerState.aiActivationRadiusSq &&
                    readBack.aiTickSuppressed == playerState.aiTickSuppressed &&
                    readBack.aiAlertFlag == playerState.recentHitFlag &&
                    readBack.aiStateMarkerHandle == playerState.recentHitMarkerHandle &&
                    readBack.aiActive == playerState.aiActive &&
                    readBack.aiPathCursorAdvanceRequested == playerState.aiPathCursorAdvanceRequested &&
                    readBack.aiCurrentSteeringSubstate == playerState.aiCurrentSteeringSubstate &&
                    readBack.aiReturnSteeringSubstate == playerState.aiReturnSteeringSubstate &&
                    readBack.masterType == playerState.masterType &&
                    readBack.statusMeterScaled == playerState.statusMeterScaled &&
                    readBack.statusMeterValue == playerState.statusMeterValue &&
                    readBack.nanitePanelLevel == playerState.nanitePanelLevel &&
                    readBack.localMasterType == modalData.masterType;

    g_PlayerSaveStateListHead = oldHead;
    g_GameStateOrMapTable = oldGameState;
    std::free(manager.indexArchive.records);
    manager.indexArchive.records = nullptr;
    CloseHandle(file);
    return ok ? 0 : 2;
}

extern "C" int player_zar_read_vehicle_list_section_smoke(void) {
    zUtil_SaveGameState *const oldHead = g_PlayerSaveStateListHead;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const int oldBuiltinTexturePackCount = g_zVid_BuiltinTexturePackCount;
    zVidTexturePackEntry *const oldBuiltinTexturePacks = g_zVid_BuiltinTexturePacks;
    const int oldTexDirEntryCount = g_zImage_TexDirEntryCount;

    zClass_NodePartial skippedRoot = {};
    std::strcpy(skippedRoot.name, "skip_vehicle");
    skippedRoot.classId = 5;
    zUtil_PlayerStateStorage skippedPlayer = {};
    skippedPlayer.rootNode = &skippedRoot;
    skippedPlayer.aiNetId = -100;
    zUtil_SaveGameState skippedSaveState = {};
    skippedSaveState.playerState = &skippedPlayer;

    zClass_Object3DDataPartial rootObject = {};
    zClass_NodePartial rootNode = {};
    std::strcpy(rootNode.name, "target_vehicle");
    rootNode.classId = 5;
    rootNode.classData = &rootObject;

    zClass_Object3DDataPartial healthyObject = {};
    healthyObject.rotation = {9.0f, 8.0f, 7.0f};
    healthyObject.localMatrix[9] = 6.0f;
    healthyObject.localMatrix[10] = 5.0f;
    healthyObject.localMatrix[11] = 4.0f;
    zClass_NodePartial healthyNode = {};
    std::strcpy(healthyNode.name, "healthy");
    healthyNode.flags = 0x01;
    healthyNode.classId = 5;
    healthyNode.classData = &healthyObject;
    zClass_NodePartial *children[1] = {&healthyNode};
    rootNode.listCountB = 1;
    rootNode.listB = children;

    PlayerMasterModalData modalData = {};
    modalData.masterType = 77;
    PlayerModalState modalState = {};
    modalState.masterModalData = &modalData;

    AINetNode currentPathNode = {};
    zUtil_PlayerStateStorage playerState = {};
    playerState.rootNode = &rootNode;
    playerState.projectileSpawnVel = {1.0f, 2.0f, 3.0f};
    playerState.localVel = {4.0f, 5.0f, 6.0f};
    playerState.yawRotatedLocalVel = {7.0f, 8.0f, 9.0f};
    playerState.lifecycleState = 4;
    playerState.aiMode2AttackDwell = 2.5f;
    playerState.aiUnknown_0f7c = reinterpret_cast<int>(&currentPathNode);
    playerState.aiCurrentPathNeighborIndex = 9;
    playerState.restartYawRad = 1.25f;
    playerState.variantTag.count = 3;
    playerState.variantTag.tags[0] = 11;
    playerState.variantTag.tags[1] = 12;
    playerState.variantTag.tags[2] = 13;

    zUtil_SaveGameState targetSaveState = {};
    targetSaveState.playerState = &playerState;
    targetSaveState.primaryModalState = &modalState;
    skippedSaveState.next = &targetSaveState;

    PlayerVehicleListSaveEntry saveData = {};
    saveData.size = 128;
    saveData.vehicleRotationAngles = {10.0f, 11.0f, 12.0f};
    saveData.worldPos = {13.0f, 14.0f, 15.0f};
    saveData.aiNetId = 1001;
    saveData.aiTopLevelState = 21;
    saveData.aiSavedTopLevelState = 22;
    saveData.aiReturnTopLevelState = 23;
    saveData.aiAttackRadiusSq = 24.0f;
    saveData.aiRestoreDistanceSq = 25.0f;
    saveData.aiRestoreTarget = {26.0f, 27.0f, 28.0f};
    saveData.aiDynamicOffsetDir = {29.0f, 30.0f, 31.0f};
    saveData.aiActivationRadiusSq = 32.0f;
    saveData.aiTickSuppressed = 33;
    saveData.aiAlertFlag = 34;
    saveData.aiStateMarkerHandle = 35;
    saveData.aiActive = 36;
    saveData.aiPathCursorAdvanceRequested = 37;
    saveData.aiCurrentSteeringSubstate = 38;
    saveData.aiReturnSteeringSubstate = 39;
    saveData.masterType = 1;
    saveData.statusMeterScaled = 40.0f;
    saveData.statusMeterValue = 41.0f;
    saveData.nanitePanelLevel = 42;

    g_PlayerSaveStateListHead = &skippedSaveState;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&skippedSaveState);
    g_Time_AccumulatedTimeSec = 100.0f;
    zVidTexturePackEntry builtinPack = {};
    builtinPack.fileHandle = std::tmpfile();
    if (builtinPack.fileHandle == nullptr) {
        g_PlayerSaveStateListHead = oldHead;
        g_GameStateOrMapTable = oldGameState;
        g_Time_AccumulatedTimeSec = oldAccumulatedTime;
        g_VariantTag_Current = oldVariantTagCurrent;
        g_Variant_CurrentTag = oldVariantCurrent;
        return 3;
    }
    g_zVid_BuiltinTexturePackCount = 1;
    g_zVid_BuiltinTexturePacks = &builtinPack;
    g_zImage_TexDirEntryCount = 0;

    PlayerVehicleListSaveEntry badSize = saveData;
    badSize.size = 0;
    Player::ZAR_ReadVehicleListSection(nullptr, "target_vehicle", &badSize, sizeof(badSize),
                                       nullptr);
    if (playerState.aiNetId != 0) {
        std::fclose(builtinPack.fileHandle);
        g_PlayerSaveStateListHead = oldHead;
        g_GameStateOrMapTable = oldGameState;
        g_Time_AccumulatedTimeSec = oldAccumulatedTime;
        g_VariantTag_Current = oldVariantTagCurrent;
        g_Variant_CurrentTag = oldVariantCurrent;
        g_zVid_BuiltinTexturePackCount = oldBuiltinTexturePackCount;
        g_zVid_BuiltinTexturePacks = oldBuiltinTexturePacks;
        g_zImage_TexDirEntryCount = oldTexDirEntryCount;
        return 1;
    }

    Player::ZAR_ReadVehicleListSection(nullptr, "target_vehicle", &saveData, sizeof(saveData),
                                       nullptr);

    const bool ok =
        skippedPlayer.aiNetId == -100 &&
        Vec3Equals(playerState.projectileSpawnVel, zVec3_Make(0.0f, 0.0f, 0.0f)) &&
        Vec3Equals(playerState.localVel, zVec3_Make(0.0f, 0.0f, 0.0f)) &&
        Vec3Equals(playerState.yawRotatedLocalVel, zVec3_Make(0.0f, 0.0f, 0.0f)) &&
        Vec3Equals(playerState.worldPos, saveData.worldPos) &&
        Vec3Equals(playerState.vehicleRotationAngles, saveData.vehicleRotationAngles) &&
        Vec3Equals(playerState.aiRestoreTarget, saveData.aiRestoreTarget) &&
        Vec3Equals(playerState.aiDynamicOffsetDir, saveData.aiDynamicOffsetDir) &&
        playerState.aiNetId == saveData.aiNetId &&
        playerState.aiTopLevelState == saveData.aiTopLevelState &&
        playerState.aiSavedTopLevelState == saveData.aiSavedTopLevelState &&
        playerState.aiReturnTopLevelState == saveData.aiReturnTopLevelState &&
        playerState.aiAttackRadiusSq == saveData.aiAttackRadiusSq &&
        playerState.aiRestoreDistanceSq == saveData.aiRestoreDistanceSq &&
        playerState.aiActivationRadiusSq == saveData.aiActivationRadiusSq &&
        playerState.aiTickSuppressed == saveData.aiTickSuppressed &&
        playerState.recentHitFlag == saveData.aiAlertFlag &&
        playerState.recentHitMarkerHandle == saveData.aiStateMarkerHandle &&
        playerState.aiActive == saveData.aiActive &&
        playerState.aiPathCursorAdvanceRequested == saveData.aiPathCursorAdvanceRequested &&
        playerState.aiCurrentSteeringSubstate == saveData.aiCurrentSteeringSubstate &&
        playerState.aiReturnSteeringSubstate == saveData.aiReturnSteeringSubstate &&
        playerState.lifecycleState == saveData.masterType &&
        playerState.statusMeterScaled == saveData.statusMeterScaled &&
        playerState.statusMeterValue == saveData.statusMeterValue &&
        playerState.nanitePanelLevel == saveData.nanitePanelLevel &&
        FloatNear(playerState.aiStateUntilTime, 100.0f) &&
        FloatNear(playerState.unknown_0f9c, 100.0f) &&
        FloatNear(playerState.unknown_0fa0, 100.0f) &&
        FloatNear(playerState.unknown_0fa4, 100.0f) &&
        FloatNear(playerState.aiStateStartTime, 100.0f) &&
        FloatNear(playerState.aiStateEndTime, 102.5f) &&
        FloatNear(playerState.unknown_0fd0, 100.0f) &&
        playerState.aiCurrentPathNode == &currentPathNode &&
        playerState.aiCurrentPathNeighborIndex == 0 &&
        (rootNode.flags & 0x04) != 0 && rootNode.nodeType == 0xff &&
        playerState.variantTag.count == 0 && playerState.variantTag.tags[0] == 0xff &&
        playerState.variantTag.tags[1] == 0xff && playerState.variantTag.tags[2] == 0xff &&
        healthyObject.localMatrix[9] == 0.0f && healthyObject.localMatrix[10] == 0.0f &&
        healthyObject.localMatrix[11] == 0.0f && healthyObject.rotation.x == 0.0f &&
        healthyObject.rotation.y == 0.0f && healthyObject.rotation.z == 0.0f;

    g_PlayerSaveStateListHead = oldHead;
    g_GameStateOrMapTable = oldGameState;
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    g_VariantTag_Current = oldVariantTagCurrent;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_zVid_BuiltinTexturePackCount = oldBuiltinTexturePackCount;
    g_zVid_BuiltinTexturePacks = oldBuiltinTexturePacks;
    g_zImage_TexDirEntryCount = oldTexDirEntryCount;

    return ok ? 0 : 2;
}

extern "C" int player_write_mines_zar_section_smoke(void) {
    char tempPath[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    GetTempPathA(sizeof(tempPath), tempPath);
    GetTempFileNameA(tempPath, "pmn", 0, tempFile);

    HANDLE const file =
        CreateFileA(tempFile, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return 1;
    }

    zZbdManager manager = {};
    manager.indexArchive.hFile = file;

    zZbdSectionHandler handler = {};
    handler.sectionName = "Mines";
    zZbdSectionCallbackCtx callbackCtx = {};
    callbackCtx.manager = &manager;
    callbackCtx.sectionHandler = &handler;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;

    OptCatalogEntryDef ignoredEntry = {};
    ignoredEntry.keyName = const_cast<char *>("ignored");
    playerState.altWeaponBanks[3].controllerA.optCatalogEntry = &ignoredEntry;

    OptCatalogEntryDef entry = {};
    entry.keyName = const_cast<char *>("P_HEMINE");
    playerState.altWeaponBanks[4].controllerA.optCatalogEntry = &entry;

    zClass_NodePartial ownerNode = {};
    std::strcpy(ownerNode.name, "mine_node");
    zClass_NodePartial projectileNode = {};
    projectileNode.classId = 5;
    zClass_Object3DDataPartial projectileData = {};
    projectileData.scale = {2.0f, 3.0f, 4.0f};
    projectileNode.classData = &projectileData;

    OptCatalogRuntimeInstanceStorage runtime = {};
    runtime.ownerNode = &ownerNode;
    runtime.projectileNode = &projectileNode;
    runtime.pos = {5.0f, 6.0f, 7.0f};
    entry.activeRuntimeListHead = &runtime;

    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;

    const int result = Player::WriteMinesZarSection(&callbackCtx, nullptr);

    PlayerMineSaveEntry dummy = {};
    PlayerMineSaveEntry mine = {};
    DWORD readDummy = 0;
    DWORD readMine = 0;
    SetFilePointer(file, 0, nullptr, FILE_BEGIN);
    ReadFile(file, &dummy, 0x60, &readDummy, nullptr);
    ReadFile(file, &mine, 0x60, &readMine, nullptr);

    const bool ok =
        result == 1 && manager.indexArchive.recordCount == 2 &&
        manager.indexArchive.records != nullptr &&
        std::strcmp(manager.indexArchive.records[0].name, "Mines/DummyMineData") == 0 &&
        std::strcmp(manager.indexArchive.records[1].name, "Mines/MineData000") == 0 &&
        readDummy == 0x60 && readMine == 0x60 && dummy.resetMarker == 1 &&
        std::strcmp(dummy.ownerNodeName, "Dummy") == 0 && mine.resetMarker == 0 &&
        std::strncmp(mine.optCatalogName, "P_HEMINE", 0x20) == 0 &&
        Vec3Equals(mine.spawnPos, runtime.pos) && Vec3Equals(mine.scale, projectileData.scale) &&
        std::strncmp(mine.ownerNodeName, "mine_node", 0x20) == 0;

    g_GameStateOrMapTable = oldGameState;
    std::free(manager.indexArchive.records);
    manager.indexArchive.records = nullptr;
    CloseHandle(file);
    return ok ? 0 : 2;
}

extern "C" int player_mines_zar_read_entry_or_reset_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    OptCatalogEntryDef *const oldEntryTable = g_OptCatalog_EntryTable;
    const int oldEntryCount = g_OptCatalog_EntryCount;
    void *const oldFreeRuntimeList = g_OptCatalogFreeRuntimeInstanceList;
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    const float oldNextSpawnScale = g_OptCatalogNextSpawnScale;
    zClass_TypeListLink *const oldType6Head = zClass_TypeList::Head(6);
    zClass_TypeListLink *const oldType6Tail = zClass_TypeList::Tail(6);

    int result = 0;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;

    OptCatalogEntryDef ignoredEntry = {};
    OptCatalogEntryDef resetEntryA = {};
    OptCatalogEntryDef resetEntryB = {};
    playerState.altWeaponBanks[3].controllerA.optCatalogEntry = &ignoredEntry;
    playerState.altWeaponBanks[4].controllerA.optCatalogEntry = &resetEntryA;
    playerState.altWeaponBanks[5].controllerB.optCatalogEntry = &resetEntryB;

    OptCatalogRuntimeInstanceStorage resetRuntimeA = {};
    OptCatalogRuntimeInstanceStorage resetRuntimeB = {};
    OptCatalogRuntimeInstanceStorage freeSentinel = {};
    zClass_NodeFreeListSlot resetProjectileA = {};
    zClass_NodeFreeListSlot resetProjectileB = {};
    zClass_Object3DDataPartial resetProjectileDataA = {};
    zClass_Object3DDataPartial resetProjectileDataB = {};
    zClass_NodePartial resetRuntimeWorld = {};
    resetRuntimeWorld.classId = 3;
    resetRuntimeWorld.flags = 1;
    resetProjectileA.node.classId = 5;
    resetProjectileA.node.flags = 1;
    resetProjectileA.node.classData = &resetProjectileDataA;
    resetProjectileA.damageHandler = &resetEntryA;
    resetProjectileB.node.classId = 5;
    resetProjectileB.node.flags = 1;
    resetProjectileB.node.classData = &resetProjectileDataB;
    resetProjectileB.damageHandler = &resetEntryB;
    resetRuntimeA.projectileNode = &resetProjectileA.node;
    resetRuntimeA.lifetime = 4.0f;
    resetRuntimeB.projectileNode = &resetProjectileB.node;
    resetRuntimeB.lifetime = 5.0f;
    resetEntryA.activeRuntimeListHead = &resetRuntimeA;
    resetEntryB.activeRuntimeListHead = &resetRuntimeB;
    g_OptCatalogRuntimeWorld = &resetRuntimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = &freeSentinel;
    zClass_Class::AddChild(&resetRuntimeWorld, &resetProjectileA.node);
    zClass_Class::AddChild(&resetRuntimeWorld, &resetProjectileB.node);

    PlayerMineSaveEntry resetData = {};
    resetData.resetMarker = 1;
    Player::Mines_ZAR_ReadEntryOrReset(nullptr, nullptr, &resetData, sizeof(resetData), nullptr);

    OptCatalogEntryDef spawnEntry = {};
    OptCatalogRuntimeInstanceStorage spawnRuntime = {};
    zClass_NodeFreeListSlot spawnProjectile = {};
    zClass_Object3DDataPartial spawnProjectileData = {};
    zClass_NodePartial spawnRuntimeWorld = {};
    zClass_NodePartial ownerNode = {};
    zClass_TypeListLink ownerLink = {&ownerNode, nullptr, nullptr, 0};
    PlayerMineSaveEntry mineData = {};

    if (ignoredEntry.activeRuntimeListHead != nullptr ||
        resetEntryA.activeRuntimeListHead != nullptr ||
        resetEntryB.activeRuntimeListHead != nullptr || resetRuntimeWorld.listCountB != 0 ||
        resetProjectileA.node.listCountA != 0 || resetProjectileB.node.listCountA != 0 ||
        g_OptCatalogFreeRuntimeInstanceList != &resetRuntimeB ||
        resetRuntimeB.next != &resetRuntimeA || resetRuntimeA.next != &freeSentinel ||
        resetProjectileA.damageHandler != nullptr || resetProjectileB.damageHandler != nullptr ||
        resetProjectileDataA.scale.x != 1.0f || resetProjectileDataB.scale.x != 1.0f) {
        result = 1;
        goto restore;
    }

    spawnEntry.keyName = const_cast<char *>("P_HEMINE");
    spawnEntry.flyoutHealth = 8.0f;
    g_OptCatalog_EntryTable = &spawnEntry;
    g_OptCatalog_EntryCount = 1;

    spawnRuntimeWorld.classId = 3;
    spawnRuntimeWorld.flags = 1;
    spawnProjectile.node.classId = 5;
    spawnProjectile.node.flags = 1;
    spawnProjectile.node.classData = &spawnProjectileData;
    spawnRuntime.projectileNode = &spawnProjectile.node;
    std::strcpy(ownerNode.name, "mine_owner");
    zClass_TypeList::Head(6) = &ownerLink;
    zClass_TypeList::Tail(6) = &ownerLink;
    g_OptCatalogRuntimeWorld = &spawnRuntimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = &spawnRuntime;
    g_OptCatalogNextSpawnScale = 3.0f;

    std::strcpy(mineData.optCatalogName, "P_HEMINE");
    mineData.spawnPos = {10.0f, 11.0f, 12.0f};
    mineData.scale = {2.0f, 3.0f, 4.0f};
    std::strcpy(mineData.ownerNodeName, "mine_owner");
    Player::Mines_ZAR_ReadEntryOrReset(nullptr, nullptr, &mineData, sizeof(mineData), nullptr);

    if (spawnEntry.activeRuntimeListHead != &spawnRuntime ||
        g_OptCatalogFreeRuntimeInstanceList != nullptr || spawnRuntime.ownerNode != &ownerNode ||
        !Vec3Equals(spawnRuntime.pos, mineData.spawnPos) || spawnRuntime.spawnScale != 3.0f ||
        g_OptCatalogNextSpawnScale != 1.0f || spawnRuntimeWorld.listCountB != 1 ||
        spawnRuntimeWorld.listB[0] != &spawnProjectile.node ||
        spawnProjectile.node.callbackContext !=
            reinterpret_cast<zClass_NodePartial *>(&spawnRuntime) ||
        spawnProjectileData.scale.x != 2.0f || spawnProjectileData.scale.y != 3.0f ||
        spawnProjectileData.scale.z != 4.0f) {
        result = 2;
        goto restore;
    }

restore:
    g_GameStateOrMapTable = oldGameState;
    g_OptCatalog_EntryTable = oldEntryTable;
    g_OptCatalog_EntryCount = oldEntryCount;
    g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    g_OptCatalogNextSpawnScale = oldNextSpawnScale;
    zClass_TypeList::Head(6) = oldType6Head;
    zClass_TypeList::Tail(6) = oldType6Tail;
    return result;
}

extern "C" int player_destroy_save_game_state_smoke(void) {
    zUtil_SaveGameState *const saveState = AllocZeroedNew<zUtil_SaveGameState>();
    saveState->playerState = AllocZeroedMalloc<zUtil_PlayerStateStorage>();

    zClass_NodeFreeListSlot rootSlot = {};
    saveState->playerState->rootNode = &rootSlot.node;

    HudUiMgrSensorTrackNode *const trackNode = AllocZeroedMalloc<HudUiMgrSensorTrackNode>();
    rootSlot.node.callbackContext = reinterpret_cast<zClass_NodePartial *>(trackNode);

    g_HudUiMgrSensor_TrackList.trackListAux = 1;
    g_HudUiMgrSensor_TrackList.head = trackNode;
    g_HudUiMgrSensor_TrackList.tail = trackNode;
    g_HudUiMgrSensor_TrackList.count = 1;

    g_PlayerSaveStateListAux = 1;
    g_PlayerSaveStateListHead = saveState;
    g_PlayerSaveStateListTail = saveState;
    g_PlayerSaveStateCount = 1;

    Player::DestroySaveGameState(saveState);

    return g_HudUiMgrSensor_TrackList.head == nullptr &&
                   g_HudUiMgrSensor_TrackList.tail == nullptr &&
                   g_HudUiMgrSensor_TrackList.count == 0 &&
                   g_HudUiMgrSensor_TrackList.trackListAux == 0 &&
                   g_PlayerSaveStateListHead == nullptr && g_PlayerSaveStateListTail == nullptr &&
                   g_PlayerSaveStateCount == 0 && g_PlayerSaveStateListAux == 0
               ? 0
               : 1;
}

extern "C" int player_shutdown_mission_runtime_smoke(void) {
    auto *const trackNode = AllocZeroedNew<HudUiMgrSensorTrackNode>();
    g_HudUiMgrSensor_TrackList.head = trackNode;
    g_HudUiMgrSensor_TrackList.tail = trackNode;
    g_HudUiMgrSensor_TrackList.count = 1;
    g_HudUiMgrSensor_TrackList.trackListAux = 1;

    PlayerMasterCommonData *const commonData = AllocZeroedNew<PlayerMasterCommonData>();
    PlayerMasterWeaponSpec *const weaponSpec = AllocZeroedNew<PlayerMasterWeaponSpec>();
    commonData->weaponSpecHead = weaponSpec;
    commonData->weaponSpecTail = weaponSpec;
    commonData->weaponSpecCount = 1;
    commonData->weaponSpecListAux = 1;
    g_PlayerMasterCommonDataHead = commonData;
    g_PlayerMasterCommonDataTail = commonData;
    g_PlayerMasterCommonDataCount = 1;
    g_PlayerMasterCommonDataListAux = 1;

    PlayerMasterModalData *const modalData = AllocZeroedNew<PlayerMasterModalData>();
    g_PlayerMasterModalDataHead = modalData;
    g_PlayerMasterModalDataTail = modalData;
    g_PlayerMasterModalDataCount = 1;
    g_PlayerMasterModalDataListAux = 1;

    g_Player_NextOrdinal = 7;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(0x1234);

    HudUiContainer *const fxContainer =
        reinterpret_cast<HudUiContainer *>(&g_zVideo_FxPass3ConfigLocal);
    fxContainer->childHead = &g_Player_UnderwaterFxPass3Ui;
    fxContainer->childTail = &g_Player_State7FxPass3Ui;
    g_Player_UnderwaterFxPass3Ui.next = &g_Player_State7FxPass3Ui;
    g_Player_UnderwaterFxPass3Ui.parent = fxContainer;
    g_Player_State7FxPass3Ui.next = nullptr;
    g_Player_State7FxPass3Ui.parent = fxContainer;

    Player::ShutdownMissionRuntime();

    return g_HudUiMgrSensor_TrackList.head == nullptr &&
                   g_HudUiMgrSensor_TrackList.tail == nullptr &&
                   g_HudUiMgrSensor_TrackList.count == 0 &&
                   g_PlayerMasterCommonDataHead == nullptr &&
                   g_PlayerMasterCommonDataTail == nullptr && g_PlayerMasterCommonDataCount == 0 &&
                   g_PlayerMasterModalDataHead == nullptr &&
                   g_PlayerMasterModalDataTail == nullptr && g_PlayerMasterModalDataCount == 0 &&
                   g_Player_NextOrdinal == 0 && g_GameStateOrMapTable == nullptr &&
                   fxContainer->childHead == nullptr && fxContainer->childTail == nullptr
               ? 0
               : 1;
}
