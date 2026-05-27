#include "Battlesport/GameNet.h"
#include "Battlesport/Briefing.h"
#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/ainet.h"
#include "Battlesport/hud.h"
#include "Battlesport/pickup.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/include/zDi.h"
#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zUtil/zZbd.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <windows.h>

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" std::uint32_t g_HudUi_InvalidateMask;
extern "C" int g_Player_MissionInitFirstRunFlag;
extern "C" HudUiPanel g_Player_TopMsgPanel1;
extern "C" HudUiPanel g_Player_TopMsgPanel2;
extern float g_HudUiLoadingCheckpointProgress[19];
extern unsigned int g_HudUiLoadingCheckpointMaxIndex;
extern unsigned int g_HudUiLoadingCheckpointCurrentIndex;
extern float g_HudUiLoadingCheckpointCurrentProgress;

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

void FreePlayerTestOptionList() {
    zOptionEntryPartial *entry = g_zGame_Options_OptionListHead;
    while (entry != nullptr) {
        zOptionEntryPartial *const next = entry->next;
        std::free(reinterpret_cast<void *>(entry->payloadOrBuffer));
        std::free(entry->name);
        std::free(entry);
        entry = next;
    }
    g_zGame_Options_OptionListHead = nullptr;
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

int g_playerTestFfStopCount = 0;
int g_playerTestFfStartCount = 0;
std::uint32_t g_playerTestFfLastIterations = 0;
std::uint32_t g_playerTestFfLastStartFlags = 0;

std::int32_t RECOIL_STDCALL PlayerTestEffectStart(zInput_DiEffect *,
                                                  std::uint32_t iterations,
                                                  std::uint32_t flags) {
    ++g_playerTestFfStartCount;
    g_playerTestFfLastIterations = iterations;
    g_playerTestFfLastStartFlags = flags;
    return 0;
}

std::int32_t RECOIL_STDCALL PlayerTestEffectStop(zInput_DiEffect *) {
    ++g_playerTestFfStopCount;
    return 0;
}

float PlayerDampingFactor(float rate, float deltaTime) {
    const int bits = static_cast<int>(-rate * deltaTime * 12102200.0f) + 0x3f800000;
    float factor = 0.0f;
    std::memcpy(&factor, &bits, sizeof(factor));
    return factor;
}

float PlayerFastSqrtEstimateForTest(float value) {
    std::int32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    bits = (bits >> 1) + 0x1fc00000;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

void MakeAinetReaderStringNode(zReader::Node &node, const char *value) {
    node.type = zReader::ZRDR_NODE_STRING;
    node.value.str = const_cast<char *>(value);
}

void MakeAinetReaderIntNode(zReader::Node &node, int value) {
    node.type = zReader::ZRDR_NODE_INT;
    node.value.i32 = value;
}

void MakeAinetReaderFloatNode(zReader::Node &node, float value) {
    node.type = zReader::ZRDR_NODE_FLOAT;
    node.value.f32 = value;
}

void MakeAinetReaderArrayNode(zReader::Node &node, zReader::Node *payload, int count) {
    payload[0].type = zReader::ZRDR_NODE_INT;
    payload[0].value.i32 = count;
    node.type = zReader::ZRDR_NODE_ARRAY;
    node.value.nodes = payload;
}

bool WriteAinetZrdU32(std::FILE *file, unsigned int value) {
    return std::fwrite(&value, sizeof(value), 1, file) == 1;
}

bool WriteAinetZrdNode(std::FILE *file, const zReader::Node &node) {
    if (!WriteAinetZrdU32(file, static_cast<unsigned int>(node.type))) {
        return false;
    }

    switch (node.type) {
    case zReader::ZRDR_NODE_INT:
    case zReader::ZRDR_NODE_FLOAT:
        return WriteAinetZrdU32(file, node.value.u32);
    case zReader::ZRDR_NODE_STRING: {
        const unsigned int length = static_cast<unsigned int>(std::strlen(node.value.str));
        return WriteAinetZrdU32(file, length) &&
               std::fwrite(node.value.str, 1, length, file) == length;
    }
    case zReader::ZRDR_NODE_ARRAY: {
        const int count = node.value.nodes[0].value.i32;
        if (!WriteAinetZrdU32(file, static_cast<unsigned int>(count))) {
            return false;
        }
        for (int index = 1; index < count; ++index) {
            if (!WriteAinetZrdNode(file, node.value.nodes[index])) {
                return false;
            }
        }
        return true;
    }
    }

    return false;
}

bool WriteAinetZrdFile(const char *path, const zReader::Node &root) {
    std::FILE *const file = std::fopen(path, "wb");
    if (file == nullptr) {
        return false;
    }

    const bool ok = WriteAinetZrdNode(file, root);
    return std::fclose(file) == 0 && ok;
}

struct AinetZrdArchiveEntry {
    const char *name;
    const zReader::Node *root;
};

bool MountAinetZrdArchive(const char *path, const AinetZrdArchiveEntry *entries, int entryCount,
                          zIndexArchive &archive, zZarFileRecord *records,
                          zArchiveListNode &archiveNode, zArchiveList &archiveList) {
    std::FILE *const file = std::fopen(path, "wb");
    if (file == nullptr) {
        return false;
    }

    bool ok = true;
    for (int index = 0; index < entryCount; ++index) {
        const long offset = std::ftell(file);
        if (offset < 0 || !WriteAinetZrdNode(file, *entries[index].root)) {
            ok = false;
            break;
        }
        const long endOffset = std::ftell(file);
        if (endOffset < offset) {
            ok = false;
            break;
        }

        records[index] = {};
        records[index].fileOffset = static_cast<unsigned int>(offset);
        records[index].fileSize = static_cast<unsigned int>(endOffset - offset);
        std::strcpy(records[index].name, entries[index].name);
    }

    if (std::fclose(file) != 0 || !ok) {
        std::remove(path);
        return false;
    }

    archive = {};
    archive.hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, nullptr);
    if (archive.hFile == INVALID_HANDLE_VALUE) {
        std::remove(path);
        return false;
    }

    archive.recordCount = static_cast<unsigned int>(entryCount);
    archive.records = records;

    archiveNode = {};
    archiveNode.payload = &archive;
    archiveNode.next = &archiveNode;
    archiveNode.prev = &archiveNode;

    archiveList = {};
    archiveList.count = 1;
    archiveList.head = &archiveNode;
    g_zArchive_MountedList = &archiveList;
    return true;
}

void MakeAinetMinimalZrdRoot(zReader::Node &root, zReader::Node *rootPayload,
                             zReader::Node *versionPayload, zReader::Node *namePayload,
                             const char *name) {
    MakeAinetReaderArrayNode(root, rootPayload, 5);
    MakeAinetReaderStringNode(rootPayload[1], "version");
    MakeAinetReaderArrayNode(rootPayload[2], versionPayload, 2);
    MakeAinetReaderIntNode(versionPayload[1], 105);
    MakeAinetReaderStringNode(rootPayload[3], "name");
    MakeAinetReaderArrayNode(rootPayload[4], namePayload, 2);
    MakeAinetReaderStringNode(namePayload[1], name);
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
HudUiStringMenu *g_PlayerTestDebugAuxMenu;
char g_PlayerTestDebugAuxText[23][256];
int g_PlayerTestDebugAuxVisible[23];
int g_PlayerTestDebugAuxSetTextCount[23];
int g_PlayerTestDebugAuxSetVisibleCount[23];

int PlayerTestDebugAuxIndex(void *self) {
    if (g_PlayerTestDebugAuxMenu == nullptr) {
        return -1;
    }

    for (int index = 0; index < 23; ++index) {
        if (&g_PlayerTestDebugAuxMenu->items[index] == self) {
            return index;
        }
    }
    return -1;
}

void RECOIL_CDECL PlayerTestDebugAuxSetTextFmt(HudUiPanel *self, const char *format, ...) {
    const int index = PlayerTestDebugAuxIndex(self);
    if (index < 0) {
        return;
    }

    va_list args;
    va_start(args, format);
    std::vsnprintf(g_PlayerTestDebugAuxText[index],
                   sizeof(g_PlayerTestDebugAuxText[index]), format, args);
    va_end(args);
    ++g_PlayerTestDebugAuxSetTextCount[index];
}

struct PlayerTestDebugAuxPanel {
    void RECOIL_THISCALL SetVisible(int visible) {
        const int index = PlayerTestDebugAuxIndex(this);
        if (index >= 0) {
            g_PlayerTestDebugAuxVisible[index] = visible;
            ++g_PlayerTestDebugAuxSetVisibleCount[index];
        }
    }
};

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

struct TestPlayerWeatherFxEmitter {
    HudUiElement ui;
    unsigned char unknown_34[0x1c];
    zClass_NodePartial *cameraNode;
    int particleAgeTick;

    void RECOIL_THISCALL SetVisible(int visible) {
        const int index = g_PlayerTestHudVisibleCount;
        if (index < 8) {
            g_PlayerTestHudVisibleThis[index] = this;
            g_PlayerTestHudVisibleValue[index] = visible;
        }
        ++g_PlayerTestHudVisibleCount;
        if (visible != 0) {
            ui.flags &= ~0x10u;
        } else {
            ui.flags |= 0x10u;
        }
    }
};
static_assert(offsetof(TestPlayerWeatherFxEmitter, cameraNode) == 0x50,
              "weather FX camera node offset");
static_assert(offsetof(TestPlayerWeatherFxEmitter, particleAgeTick) == 0x54,
              "weather FX particle tick offset");

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
    {}, nullptr, {}, nullptr, {}, nullptr, PlayerCheckpointSendFake, {}, nullptr, {}, nullptr, {}, nullptr,
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

extern "C" int ainet_alloc_smoke(void) {
    g_AINetListHead = nullptr;
    g_AINetListTail = nullptr;

    AINet *const first = AINet::Alloc();
    AINet *const second = AINet::Alloc();
    if (first == nullptr || second == nullptr) {
        AINet::FreeAll();
        g_AINetListTail = nullptr;
        return 1;
    }

    const bool linked = g_AINetListHead == first && g_AINetListTail == second &&
                        first->next == second && second->next == nullptr &&
                        first->nodeListHead == nullptr && second->nodeListHead == nullptr;

    AINet::FreeAll();
    g_AINetListTail = nullptr;
    return linked ? 0 : 2;
}

extern "C" int ainet_find_by_net_id_smoke(void) {
    AINet first = {};
    AINet second = {};
    AINet third = {};
    first.netId = 10;
    second.netId = 20;
    third.netId = 30;
    first.next = &second;
    second.next = &third;

    AINet *const oldHead = g_AINetListHead;
    g_AINetListHead = &first;

    const bool ok = AINet::FindByNetId(20) == &second &&
                    AINet::FindByNetId(99) == nullptr;
    g_AINetListHead = nullptr;
    const bool emptyOk = AINet::FindByNetId(10) == nullptr;

    g_AINetListHead = oldHead;
    return ok && emptyOk ? 0 : 1;
}

extern "C" int ainet_find_nearest_node_smoke(void) {
    AINetNode first = {};
    AINetNode second = {};
    AINetNode third = {};
    first.position = {10.0f, 0.0f, 0.0f};
    second.position = {1.0f, 2.0f, 3.0f};
    third.position = {-2.0f, 0.0f, 1.0f};
    first.next = &second;
    second.next = &third;

    const zVec3 query = {0.0f, 0.0f, 0.0f};
    return AINet::FindNearestNode(&query, &first) == &third &&
                   AINet::FindNearestNode(&query, nullptr) == nullptr
               ? 0
               : 1;
}

extern "C" int ainet_find_node_by_index_smoke(void) {
    AINetNode first = {};
    AINetNode second = {};
    AINetNode third = {};
    first.nodeIndex = 10;
    second.nodeIndex = 20;
    third.nodeIndex = 30;
    first.next = &second;
    second.next = &third;

    return AINet::FindNodeByIndex(20, &first) == &second &&
                   AINet::FindNodeByIndex(99, &first) == nullptr &&
                   AINet::FindNodeByIndex(10, nullptr) == nullptr
               ? 0
               : 1;
}

extern "C" int ainet_path_probe_fan_init_from_segment_smoke(void) {
    AINetPathProbeFan fan = {};
    const zVec3 from = {1.0f, 2.0f, 3.0f};
    const zVec3 to = {4.0f, 8.0f, 7.0f};
    fan.InitFromSegment(from, to, 2.0f);

    const float sin45 = std::sin(45.0f);
    const float cos45 = std::cos(45.0f);
    const zVec3 expectedPerp = {-0.8f, 0.0f, 0.6f};
    const zVec3 expectedPlus = {sin45 * expectedPerp.z + cos45 * expectedPerp.x, 0.0f,
                                cos45 * expectedPerp.z - sin45 * expectedPerp.x};
    const zVec3 expectedMinus = {-sin45 * expectedPerp.z + cos45 * expectedPerp.x, 0.0f,
                                 cos45 * expectedPerp.z + sin45 * expectedPerp.x};

    if (!FloatNear(fan.delta.x, 0.6f) || !FloatNear(fan.delta.y, 6.0f) ||
        !FloatNear(fan.delta.z, 0.8f) || !FloatNear(fan.clampedTravel, 3.0f) ||
        !FloatNear(fan.perpendicular.x, expectedPerp.x) || fan.perpendicular.y != 0.0f ||
        !FloatNear(fan.perpendicular.z, expectedPerp.z) ||
        !FloatNear(fan.probeDirPlus45.x, expectedPlus.x) ||
        !FloatNear(fan.probeDirPlus45.z, expectedPlus.z) ||
        !FloatNear(fan.probeDirMinus45.x, expectedMinus.x) ||
        !FloatNear(fan.probeDirMinus45.z, expectedMinus.z) ||
        !FloatNear(fan.pathWidth, 2.0f)) {
        return 1;
    }

    AINetPathProbeFan shortFan = {};
    const zVec3 shortTo = {2.0f, 2.0f, 3.0f};
    shortFan.InitFromSegment(from, shortTo, 4.0f);
    return FloatNear(shortFan.clampedTravel, 2.0f) ? 0 : 2;
}

extern "C" int ainet_resolve_neighbor_links_and_probe_fans_smoke(void) {
    AINetNode first = {};
    AINetNode second = {};
    AINetNode third = {};
    first.position = {0.0f, 1.0f, 0.0f};
    second.position = {3.0f, 5.0f, 4.0f};
    third.position = {-4.0f, 2.0f, 0.0f};
    first.nodeIndex = 1;
    second.nodeIndex = 2;
    third.nodeIndex = 3;
    first.next = &second;
    second.next = &third;
    first.neighborIndices[0] = 2;
    first.neighborIndices[1] = -1;
    first.neighborIndices[2] = 3;
    second.neighborIndices[0] = -1;
    second.neighborIndices[1] = -1;
    second.neighborIndices[2] = -1;
    third.neighborIndices[0] = -1;
    third.neighborIndices[1] = -1;
    third.neighborIndices[2] = -1;

    AINet::ResolveNeighborLinksAndBuildProbeFans(&first, 2.0f);

    const bool resolved = first.neighborNodes[0] == &second && first.neighborNodes[1] == nullptr &&
                          first.neighborNodes[2] == &third && first.probeFans[0] != nullptr &&
                          first.probeFans[1] == nullptr && first.probeFans[2] != nullptr &&
                          FloatNear(first.probeFans[0]->delta.x, 0.6f) &&
                          FloatNear(first.probeFans[0]->delta.y, 4.0f) &&
                          FloatNear(first.probeFans[0]->delta.z, 0.8f) &&
                          FloatNear(first.probeFans[0]->clampedTravel, 3.0f) &&
                          FloatNear(first.probeFans[2]->delta.x, -1.0f) &&
                          FloatNear(first.probeFans[2]->pathWidth, 2.0f);

    for (int i = 0; i < 3; ++i) {
        std::free(first.probeFans[i]);
        first.probeFans[i] = nullptr;
    }

    return resolved ? 0 : 1;
}

extern "C" int ainet_load_from_zrd_smoke(void) {
    AINet::FreeAll();
    g_AINetListTail = nullptr;

    zReader::Node root = {};
    zReader::Node rootPayload[21] = {};
    zReader::Node versionPayload[2] = {};
    zReader::Node namePayload[2] = {};
    zReader::Node typePayload[2] = {};
    zReader::Node pathWidthPayload[2] = {};
    zReader::Node hideTimesPayload[3] = {};
    zReader::Node attackBuddyPayload[2] = {};
    zReader::Node activateBuddyPayload[2] = {};
    zReader::Node attackStrategyPayload[2] = {};
    zReader::Node node0Payload[4] = {};
    zReader::Node node0Position[4] = {};
    zReader::Node node0Neighbors[4] = {};
    zReader::Node node1Payload[4] = {};
    zReader::Node node1Position[4] = {};
    zReader::Node node1Neighbors[4] = {};

    MakeAinetReaderArrayNode(root, rootPayload, 21);
    MakeAinetReaderStringNode(rootPayload[1], "version");
    MakeAinetReaderArrayNode(rootPayload[2], versionPayload, 2);
    MakeAinetReaderIntNode(versionPayload[1], 105);
    MakeAinetReaderStringNode(rootPayload[3], "name");
    MakeAinetReaderArrayNode(rootPayload[4], namePayload, 2);
    MakeAinetReaderStringNode(namePayload[1], "arena");
    MakeAinetReaderStringNode(rootPayload[5], "type");
    MakeAinetReaderArrayNode(rootPayload[6], typePayload, 2);
    MakeAinetReaderStringNode(typePayload[1], "fi");
    MakeAinetReaderStringNode(rootPayload[7], "path_width");
    MakeAinetReaderArrayNode(rootPayload[8], pathWidthPayload, 2);
    MakeAinetReaderFloatNode(pathWidthPayload[1], 2.0f);
    MakeAinetReaderStringNode(rootPayload[9], "hide_times");
    MakeAinetReaderArrayNode(rootPayload[10], hideTimesPayload, 3);
    MakeAinetReaderFloatNode(hideTimesPayload[1], 1.5f);
    MakeAinetReaderFloatNode(hideTimesPayload[2], 2.5f);
    MakeAinetReaderStringNode(rootPayload[11], "attack_buddy");
    MakeAinetReaderArrayNode(rootPayload[12], attackBuddyPayload, 2);
    MakeAinetReaderIntNode(attackBuddyPayload[1], 12);
    MakeAinetReaderStringNode(rootPayload[13], "activate_buddy");
    MakeAinetReaderArrayNode(rootPayload[14], activateBuddyPayload, 2);
    MakeAinetReaderIntNode(activateBuddyPayload[1], 34);
    MakeAinetReaderStringNode(rootPayload[15], "attack_strategy");
    MakeAinetReaderArrayNode(rootPayload[16], attackStrategyPayload, 2);
    MakeAinetReaderStringNode(attackStrategyPayload[1], "zig");
    MakeAinetReaderStringNode(rootPayload[17], "node_00");
    MakeAinetReaderArrayNode(rootPayload[18], node0Payload, 4);
    MakeAinetReaderIntNode(node0Payload[1], 77);
    MakeAinetReaderArrayNode(node0Payload[2], node0Position, 4);
    MakeAinetReaderFloatNode(node0Position[1], 0.0f);
    MakeAinetReaderFloatNode(node0Position[2], 0.0f);
    MakeAinetReaderFloatNode(node0Position[3], 0.0f);
    MakeAinetReaderArrayNode(node0Payload[3], node0Neighbors, 4);
    MakeAinetReaderIntNode(node0Neighbors[1], 1);
    MakeAinetReaderIntNode(node0Neighbors[2], -1);
    MakeAinetReaderIntNode(node0Neighbors[3], -1);
    MakeAinetReaderStringNode(rootPayload[19], "node_01");
    MakeAinetReaderArrayNode(rootPayload[20], node1Payload, 4);
    MakeAinetReaderIntNode(node1Payload[1], 88);
    MakeAinetReaderArrayNode(node1Payload[2], node1Position, 4);
    MakeAinetReaderFloatNode(node1Position[1], 3.0f);
    MakeAinetReaderFloatNode(node1Position[2], 0.0f);
    MakeAinetReaderFloatNode(node1Position[3], 4.0f);
    MakeAinetReaderArrayNode(node1Payload[3], node1Neighbors, 4);
    MakeAinetReaderIntNode(node1Neighbors[1], -1);
    MakeAinetReaderIntNode(node1Neighbors[2], -1);
    MakeAinetReaderIntNode(node1Neighbors[3], -1);

    zIndexArchive archive = {};
    zZarFileRecord records[1] = {};
    zArchiveListNode archiveNode = {};
    zArchiveList archiveList = {};
    const char *const path = "ainet_load_from_zrd_test.zar";
    const AinetZrdArchiveEntry entries[] = {{"net_07.zrd", &root}};
    std::remove(path);
    if (!MountAinetZrdArchive(path, entries, 1, archive, records, archiveNode, archiveList)) {
        return 1;
    }

    AINet *const net = AINet::LoadFromZrd(7);
    g_zArchive_MountedList = nullptr;
    CloseHandle(static_cast<HANDLE>(archive.hFile));
    std::remove(path);
    if (net == nullptr) {
        return 2;
    }

    AINetNode *const node0 = net->nodeListHead;
    AINetNode *const node1 = node0 != nullptr ? node0->next : nullptr;
    const bool ok = g_AINetListHead == net && g_AINetListTail == net && net->netId == 7 &&
                    std::strcmp(net->name, "arena") == 0 && net->aiType == AINET_TYPE_FI &&
                    FloatNear(net->pathWidth, 2.0f) && FloatNear(net->hideTime0, 1.5f) &&
                    FloatNear(net->hideTime1, 2.5f) && net->attackBuddyNetId == 12 &&
                    net->activateBuddyNetId == 34 && net->attackStrategy == AINET_STRAT_ZIG &&
                    node0 != nullptr && node1 != nullptr && node1->next == nullptr &&
                    node0->costOrType == 77 && node1->costOrType == 88 &&
                    node0->neighborNodes[0] == node1 && node0->neighborNodes[1] == nullptr &&
                    node0->neighborNodes[2] == nullptr && node0->probeFans[0] != nullptr &&
                    node0->probeFans[1] == nullptr && node0->probeFans[2] == nullptr &&
                    node1->neighborNodes[0] == nullptr && node1->probeFans[0] == nullptr &&
                    FloatNear(node0->probeFans[0]->delta.x, 0.6f) &&
                    FloatNear(node0->probeFans[0]->delta.z, 0.8f);

    AINet::FreeAll();
    g_AINetListTail = nullptr;
    return ok ? 0 : 3;
}

extern "C" int ainet_load_all_from_zrd_smoke(void) {
    AINet::FreeAll();
    g_AINetListTail = nullptr;

    zReader::Node root00 = {};
    zReader::Node root01 = {};
    zReader::Node root99 = {};
    zReader::Node root100 = {};
    zReader::Node rootPayload00[5] = {};
    zReader::Node rootPayload01[5] = {};
    zReader::Node rootPayload99[5] = {};
    zReader::Node rootPayload100[5] = {};
    zReader::Node versionPayload00[2] = {};
    zReader::Node versionPayload01[2] = {};
    zReader::Node versionPayload99[2] = {};
    zReader::Node versionPayload100[2] = {};
    zReader::Node namePayload00[2] = {};
    zReader::Node namePayload01[2] = {};
    zReader::Node namePayload99[2] = {};
    zReader::Node namePayload100[2] = {};

    MakeAinetMinimalZrdRoot(root00, rootPayload00, versionPayload00, namePayload00, "zero");
    MakeAinetMinimalZrdRoot(root01, rootPayload01, versionPayload01, namePayload01, "one");
    MakeAinetMinimalZrdRoot(root99, rootPayload99, versionPayload99, namePayload99, "last");
    MakeAinetMinimalZrdRoot(root100, rootPayload100, versionPayload100, namePayload100, "hundred");

    zIndexArchive archive = {};
    zZarFileRecord records[4] = {};
    zArchiveListNode archiveNode = {};
    zArchiveList archiveList = {};
    const char *const path = "ainet_load_all_from_zrd_test.zar";
    const AinetZrdArchiveEntry entries[] = {{"net_00.zrd", &root00},
                                            {"net_01.zrd", &root01},
                                            {"net_99.zrd", &root99},
                                            {"net_100.zrd", &root100}};
    std::remove(path);
    if (!MountAinetZrdArchive(path, entries, 4, archive, records, archiveNode, archiveList)) {
        return 1;
    }

    AINet::LoadAllFromZrd();
    g_zArchive_MountedList = nullptr;
    CloseHandle(static_cast<HANDLE>(archive.hFile));
    std::remove(path);

    AINet *const first = g_AINetListHead;
    AINet *const second = first != nullptr ? first->next : nullptr;
    const bool ok = first != nullptr && second != nullptr && second->next == nullptr &&
                    g_AINetListTail == second && first->netId == 1 && second->netId == 99 &&
                    std::strcmp(first->name, "one") == 0 &&
                    std::strcmp(second->name, "last") == 0 &&
                    first->nodeListHead == nullptr && second->nodeListHead == nullptr;

    AINet::FreeAll();
    g_AINetListTail = nullptr;
    return ok ? 0 : 2;
}

extern "C" int zvehicle_select_zrd_by_difficulty_smoke(void) {
    int *const oldDifficultyOption = g_zOpt_GameDifficultyOption;
    int difficulty = 1;
    g_zOpt_GameDifficultyOption = &difficulty;
    if (g_zUtil_ZRDR_FreePool == nullptr) {
        zUtil::ZRDR_PreallocNodePool(2);
    }

    const char *const dir = "zvehicle_select_zrd_by_difficulty_smoke_dir";
    CreateDirectoryA(dir, nullptr);

    char easyPath[MAX_PATH] = {};
    char hardPath[MAX_PATH] = {};
    std::sprintf(easyPath, "%s\\vehicle_easy.zrd", dir);
    std::sprintf(hardPath, "%s\\vehicle_hard.zrd", dir);
    std::remove(easyPath);
    std::remove(hardPath);

    std::FILE *file = std::fopen(easyPath, "wb");
    if (file == nullptr) {
        g_zOpt_GameDifficultyOption = oldDifficultyOption;
        RemoveDirectoryA(dir);
        return 1;
    }
    std::fclose(file);

    difficulty = 0;
    const bool easyOk = std::strcmp(zVehicle::SelectZrdByDifficulty(dir), "vehicle_easy.zrd") == 0;

    difficulty = 2;
    const bool missingHardFallsBack =
        std::strcmp(zVehicle::SelectZrdByDifficulty(dir), "vehicle.zrd") == 0;

    file = std::fopen(hardPath, "wb");
    if (file == nullptr) {
        std::remove(easyPath);
        g_zOpt_GameDifficultyOption = oldDifficultyOption;
        RemoveDirectoryA(dir);
        return 2;
    }
    std::fclose(file);

    const bool hardOk = std::strcmp(zVehicle::SelectZrdByDifficulty(dir), "vehicle_hard.zrd") == 0;

    difficulty = 1;
    const bool defaultOk = std::strcmp(zVehicle::SelectZrdByDifficulty(dir), "vehicle.zrd") == 0;

    std::remove(easyPath);
    std::remove(hardPath);
    RemoveDirectoryA(dir);
    g_zOpt_GameDifficultyOption = oldDifficultyOption;
    return easyOk && missingHardFallsBack && hardOk && defaultOk ? 0 : 3;
}

extern "C" int player_load_master_common_data_from_node_smoke(void) {
    int networkEnabled = 0;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    const char *const oldPickupLogicalName = g_PickupTypes[4].logicalName;
    const int oldPickupTypeIndex = g_PickupTypes[4].typeIndex;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_PickupTypes[4].logicalName = "pickup_test";
    g_PickupTypes[4].typeIndex = 77;

    zReader::Node root = {};
    zReader::Node rootItems[5] = {};
    zReader::Node commonItems[27] = {};
    zReader::Node dummyModalItems[1] = {};
    zReader::Node naniteItems[3] = {};
    zReader::Node soundsItems[7] = {};
    zReader::Node weaponUpItems[2] = {};
    zReader::Node weaponSelectItems[2] = {};
    zReader::Node pingingItems[2] = {};
    zReader::Node activationItems[2] = {};
    zReader::Node notPursuitItems[2] = {};
    zReader::Node returnRangeItems[2] = {};
    zReader::Node startAnimItems[2] = {};
    zReader::Node cambackItems[4] = {};
    zReader::Node camback0Items[4] = {};
    zReader::Node camback1Items[4] = {};
    zReader::Node camback2Items[4] = {};
    zReader::Node aimyItems[3] = {};
    zReader::Node cameraSwingItems[5] = {};
    zReader::Node trackSwitchItems[4] = {};
    zReader::Node healthItems[3] = {};
    zReader::Node pickupsItems[3] = {};
    zReader::Node weaponsItems[3] = {};
    zReader::Node weapon0Items[10] = {};
    zReader::Node weapon1Items[10] = {};

    MakeAinetReaderArrayNode(root, rootItems, 5);
    MakeAinetReaderStringNode(rootItems[1], "common_mode");
    MakeAinetReaderArrayNode(rootItems[2], commonItems, 27);
    MakeAinetReaderStringNode(rootItems[3], "modal_track");
    MakeAinetReaderArrayNode(rootItems[4], dummyModalItems, 1);

    int slot = 1;
    MakeAinetReaderStringNode(commonItems[slot++], "nanite");
    MakeAinetReaderArrayNode(commonItems[slot++], naniteItems, 3);
    MakeAinetReaderStringNode(commonItems[slot++], "sounds");
    MakeAinetReaderArrayNode(commonItems[slot++], soundsItems, 7);
    MakeAinetReaderStringNode(commonItems[slot++], "activation");
    MakeAinetReaderArrayNode(commonItems[slot++], activationItems, 2);
    MakeAinetReaderStringNode(commonItems[slot++], "not_pursuit_dwell");
    MakeAinetReaderArrayNode(commonItems[slot++], notPursuitItems, 2);
    MakeAinetReaderStringNode(commonItems[slot++], "return_range");
    MakeAinetReaderArrayNode(commonItems[slot++], returnRangeItems, 2);
    MakeAinetReaderStringNode(commonItems[slot++], "start_anims");
    MakeAinetReaderArrayNode(commonItems[slot++], startAnimItems, 2);
    MakeAinetReaderStringNode(commonItems[slot++], "camback");
    MakeAinetReaderArrayNode(commonItems[slot++], cambackItems, 4);
    MakeAinetReaderStringNode(commonItems[slot++], "aimy");
    MakeAinetReaderArrayNode(commonItems[slot++], aimyItems, 3);
    MakeAinetReaderStringNode(commonItems[slot++], "camera_ud_swing");
    MakeAinetReaderArrayNode(commonItems[slot++], cameraSwingItems, 5);
    MakeAinetReaderStringNode(commonItems[slot++], "track_switch");
    MakeAinetReaderArrayNode(commonItems[slot++], trackSwitchItems, 4);
    MakeAinetReaderStringNode(commonItems[slot++], "health");
    MakeAinetReaderArrayNode(commonItems[slot++], healthItems, 3);
    MakeAinetReaderStringNode(commonItems[slot++], "pickups");
    MakeAinetReaderArrayNode(commonItems[slot++], pickupsItems, 3);
    MakeAinetReaderStringNode(commonItems[slot++], "weapons");
    MakeAinetReaderArrayNode(commonItems[slot++], weaponsItems, 3);

    MakeAinetReaderIntNode(naniteItems[1], 12);
    MakeAinetReaderIntNode(naniteItems[2], 34);
    MakeAinetReaderStringNode(soundsItems[1], "weapon_up");
    MakeAinetReaderArrayNode(soundsItems[2], weaponUpItems, 2);
    MakeAinetReaderStringNode(weaponUpItems[1], "snd_weapon_up");
    MakeAinetReaderStringNode(soundsItems[3], "weapon_select");
    MakeAinetReaderArrayNode(soundsItems[4], weaponSelectItems, 2);
    MakeAinetReaderStringNode(weaponSelectItems[1], "snd_weapon_select");
    MakeAinetReaderStringNode(soundsItems[5], "pinging");
    MakeAinetReaderArrayNode(soundsItems[6], pingingItems, 2);
    MakeAinetReaderStringNode(pingingItems[1], "snd_ping");
    MakeAinetReaderFloatNode(activationItems[1], 13.0f);
    MakeAinetReaderFloatNode(notPursuitItems[1], 4.25f);
    MakeAinetReaderFloatNode(returnRangeItems[1], 30.0f);
    MakeAinetReaderStringNode(startAnimItems[1], "anim_start");

    MakeAinetReaderArrayNode(cambackItems[1], camback0Items, 4);
    MakeAinetReaderArrayNode(cambackItems[2], camback1Items, 4);
    MakeAinetReaderArrayNode(cambackItems[3], camback2Items, 4);
    MakeAinetReaderFloatNode(camback0Items[1], 1.0f);
    MakeAinetReaderFloatNode(camback0Items[2], 2.0f);
    MakeAinetReaderFloatNode(camback0Items[3], 3.0f);
    MakeAinetReaderFloatNode(camback1Items[1], 4.0f);
    MakeAinetReaderFloatNode(camback1Items[2], 5.0f);
    MakeAinetReaderFloatNode(camback1Items[3], 6.0f);
    MakeAinetReaderFloatNode(camback2Items[1], 7.0f);
    MakeAinetReaderFloatNode(camback2Items[2], 8.0f);
    MakeAinetReaderFloatNode(camback2Items[3], 9.0f);
    MakeAinetReaderFloatNode(aimyItems[1], 0.75f);
    MakeAinetReaderFloatNode(aimyItems[2], 1.25f);
    MakeAinetReaderFloatNode(cameraSwingItems[1], 5.0f);
    MakeAinetReaderFloatNode(cameraSwingItems[2], 6.0f);
    MakeAinetReaderFloatNode(cameraSwingItems[3], 7.0f);
    MakeAinetReaderFloatNode(cameraSwingItems[4], 8.0f);
    MakeAinetReaderFloatNode(trackSwitchItems[1], 11.0f);
    MakeAinetReaderFloatNode(trackSwitchItems[2], 22.0f);
    MakeAinetReaderFloatNode(trackSwitchItems[3], 33.0f);
    MakeAinetReaderFloatNode(healthItems[1], 125.0f);
    MakeAinetReaderFloatNode(healthItems[2], 250.0f);
    MakeAinetReaderStringNode(pickupsItems[1], "pickup_test");
    MakeAinetReaderIntNode(pickupsItems[2], 3);

    MakeAinetReaderArrayNode(weaponsItems[1], weapon0Items, 10);
    MakeAinetReaderArrayNode(weaponsItems[2], weapon1Items, 10);
    MakeAinetReaderStringNode(weapon0Items[1], "weapon_alpha");
    MakeAinetReaderIntNode(weapon0Items[2], 10);
    MakeAinetReaderIntNode(weapon0Items[3], 11);
    MakeAinetReaderIntNode(weapon0Items[4], 12);
    MakeAinetReaderFloatNode(weapon0Items[5], 1.5f);
    MakeAinetReaderFloatNode(weapon0Items[6], 20.0f);
    MakeAinetReaderFloatNode(weapon0Items[7], 30.0f);
    MakeAinetReaderIntNode(weapon0Items[8], 13);
    MakeAinetReaderIntNode(weapon0Items[9], 14);
    MakeAinetReaderStringNode(weapon1Items[1], "weapon_beta");
    MakeAinetReaderIntNode(weapon1Items[2], 20);
    MakeAinetReaderIntNode(weapon1Items[3], 21);
    MakeAinetReaderIntNode(weapon1Items[4], 22);
    MakeAinetReaderFloatNode(weapon1Items[5], 2.5f);
    MakeAinetReaderFloatNode(weapon1Items[6], 40.0f);
    MakeAinetReaderFloatNode(weapon1Items[7], 50.0f);
    MakeAinetReaderIntNode(weapon1Items[8], 23);
    MakeAinetReaderIntNode(weapon1Items[9], 24);

    PlayerMasterCommonData commonData = {};
    Player::LoadMasterCommonDataFromNode(&commonData, &root, "vehicle_name");

    PlayerMasterWeaponSpec *const firstWeapon = commonData.weaponSpecHead;
    PlayerMasterWeaponSpec *const secondWeapon = firstWeapon != nullptr ? firstWeapon->next : nullptr;
    const bool ok =
        std::strcmp(commonData.vehicleName, "vehicle_name") == 0 && commonData.modalCount == 1 &&
        commonData.naniteBuildRate == 12 && commonData.naniteMaxLevel == 34 &&
        FloatNear(commonData.activationRangeSq, 169.0f) &&
        FloatNear(commonData.notPursuitDwellTime, 4.25f) &&
        FloatNear(commonData.returnRangeSq, 900.0f) &&
        std::strcmp(commonData.startAnimsName, "anim_start") == 0 &&
        FloatNear(commonData.cambackSide0, 1.0f) && FloatNear(commonData.cambackBase0, 2.0f) &&
        FloatNear(commonData.cambackDist0, 3.0f) &&
        FloatNear(commonData.cambackSide1, 4.0f) && FloatNear(commonData.cambackBase1, 5.0f) &&
        FloatNear(commonData.cambackDist1, 6.0f) &&
        FloatNear(commonData.cambackSide2, 7.0f) && FloatNear(commonData.cambackBase2, 8.0f) &&
        FloatNear(commonData.cambackDist2, 9.0f) &&
        FloatNear(commonData.aimYawRate, 0.75f) && FloatNear(commonData.aimYawMax, 1.25f) &&
        FloatNear(commonData.cameraUdSwing[0], 5.0f) &&
        FloatNear(commonData.cameraUdSwing[1], 6.0f) &&
        FloatNear(commonData.cameraUdSwing[2], 7.0f) &&
        FloatNear(commonData.cameraUdSwing[3], 8.0f) &&
        FloatNear(commonData.trackSwitchDist0, 11.0f) &&
        FloatNear(commonData.trackSwitchDist1, 22.0f) &&
        FloatNear(commonData.trackSwitchDist2, 33.0f) &&
        FloatNear(commonData.maxHealth, 125.0f) &&
        FloatNear(commonData.invMaxHealth, 1.0f / 125.0f) && commonData.pickupType == 77 &&
        commonData.pickupCapacity == 3 && commonData.weaponNodeCount == 2 &&
        commonData.weaponSpecCount == 2 && firstWeapon != nullptr && secondWeapon != nullptr &&
        secondWeapon->next == nullptr &&
        std::strcmp(firstWeapon->optCatalogName, "weapon_alpha") == 0 &&
        firstWeapon->missionRequirementOrGateId == 10 && firstWeapon->mountLayoutFlags == 11 &&
        FloatNear(firstWeapon->startAmmoOrCharge, 12.0f) &&
        FloatNear(firstWeapon->dispatchRepeatDelay, 1.5f) &&
        FloatNear(firstWeapon->aiAttackRangeMin, 20.0f) &&
        FloatNear(firstWeapon->aiAttackRangeMax, 30.0f) &&
        firstWeapon->fireSlotRecoilFlags == 13 &&
        firstWeapon->initialHardpointSelectState == 14 &&
        std::strcmp(secondWeapon->optCatalogName, "weapon_beta") == 0 &&
        secondWeapon->missionRequirementOrGateId == 20 && secondWeapon->mountLayoutFlags == 21 &&
        FloatNear(secondWeapon->startAmmoOrCharge, 22.0f) &&
        FloatNear(secondWeapon->dispatchRepeatDelay, 2.5f) &&
        FloatNear(secondWeapon->aiAttackRangeMin, 40.0f) &&
        FloatNear(secondWeapon->aiAttackRangeMax, 50.0f) &&
        secondWeapon->fireSlotRecoilFlags == 23 &&
        secondWeapon->initialHardpointSelectState == 24;

    PlayerMasterWeaponSpec *weaponSpec = commonData.weaponSpecHead;
    while (weaponSpec != nullptr) {
        PlayerMasterWeaponSpec *const next = weaponSpec->next;
        ::operator delete(weaponSpec);
        weaponSpec = next;
    }
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_PickupTypes[4].logicalName = oldPickupLogicalName;
    g_PickupTypes[4].typeIndex = oldPickupTypeIndex;
    return ok ? 0 : 1;
}

extern "C" int player_get_save_state_list_head_smoke(void) {
    zUtil_SaveGameState *const oldHead = g_PlayerSaveStateListHead;
    zUtil_SaveGameState saveState = {};

    g_PlayerSaveStateListHead = nullptr;
    const bool nullOk = Player::GetSaveStateListHead() == nullptr;

    g_PlayerSaveStateListHead = &saveState;
    const bool valueOk = Player::GetSaveStateListHead() == &saveState;

    g_PlayerSaveStateListHead = oldHead;
    return nullOk && valueOk ? 0 : 1;
}

extern "C" int player_unbind_current_save_state_if_single_player_smoke(void) {
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    zUtil_SaveGameState *const oldCurrentSaveState = g_CurrentPlayerSaveState;

    int networkEnabled = 0;
    ZOPT_NETWORK_ENABLED = &networkEnabled;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    playerState.currentSaveStateBound = 1;
    g_CurrentPlayerSaveState = &saveState;

    Player::UnbindCurrentSaveStateIfSinglePlayer();
    const bool disabledOk =
        g_CurrentPlayerSaveState == nullptr && playerState.currentSaveStateBound == 0;

    networkEnabled = 1;
    playerState.currentSaveStateBound = 1;
    g_CurrentPlayerSaveState = &saveState;

    Player::UnbindCurrentSaveStateIfSinglePlayer();
    const bool enabledOk =
        g_CurrentPlayerSaveState == &saveState && playerState.currentSaveStateBound == 1;

    g_CurrentPlayerSaveState = oldCurrentSaveState;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    return disabledOk && enabledOk ? 0 : 1;
}

extern "C" int player_bind_active_game_state_as_current_save_state_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zUtil_SaveGameState *const oldCurrentSaveState = g_CurrentPlayerSaveState;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;
    g_CurrentPlayerSaveState = nullptr;

    Player::BindActiveGameStateAsCurrentSaveState();
    const bool ok =
        playerState.currentSaveStateBound == 1 && g_CurrentPlayerSaveState == &saveState;

    g_CurrentPlayerSaveState = oldCurrentSaveState;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    return ok ? 0 : 1;
}

extern "C" int player_async_command_callback_basic_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zUtil_SaveGameState *const oldCurrentSaveState = g_CurrentPlayerSaveState;
    zEffectAnimEntry *const oldDebugEntry = g_Player_ActiveDebugScriptAsyncEntry;
    const int oldRebuildCameraDir = g_Player_RebuildCameraDirFlatFromCurrentTarget;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;
    g_CurrentPlayerSaveState = nullptr;
    g_Player_RebuildCameraDirFlatFromCurrentTarget = 0;
    g_Player_ActiveDebugScriptAsyncEntry = nullptr;

    zEffectAnimEntry entryA = {};
    zEffectAnimEntry entryB = {};

    Player::AsyncCommandCallback(&entryA, nullptr, 20);
    const bool storesDebugEntry = g_Player_ActiveDebugScriptAsyncEntry == &entryA;

    Player::AsyncCommandCallback(&entryB, nullptr, 0);
    const bool ignoresMismatchedClear = g_Player_ActiveDebugScriptAsyncEntry == &entryA;

    Player::AsyncCommandCallback(&entryA, nullptr, 0);
    const bool clearsMatchedEntry = g_Player_ActiveDebugScriptAsyncEntry == nullptr;

    Player::AsyncCommandCallback(&entryA, nullptr, 1);
    const bool bindsActiveState = g_Player_RebuildCameraDirFlatFromCurrentTarget == 1 &&
                                  playerState.currentSaveStateBound == 1 &&
                                  g_CurrentPlayerSaveState == &saveState;

    g_Player_ActiveDebugScriptAsyncEntry = &entryB;
    Player::AsyncCommandCallback(&entryA, nullptr, 1234);
    const bool defaultNoOp = g_Player_ActiveDebugScriptAsyncEntry == &entryB;

    g_Player_ActiveDebugScriptAsyncEntry = oldDebugEntry;
    g_Player_RebuildCameraDirFlatFromCurrentTarget = oldRebuildCameraDir;
    g_CurrentPlayerSaveState = oldCurrentSaveState;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    return storesDebugEntry && ignoresMismatchedClear && clearsMatchedEntry && bindsActiveState &&
                   defaultNoOp
               ? 0
               : 1;
}

extern "C" int player_sync_local_pose_from_root_node_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    zClass_Object3DDataPartial objectData = {};
    objectData.localMatrix[9] = 12.0f;
    objectData.localMatrix[10] = 34.0f;
    objectData.localMatrix[11] = 56.0f;
    objectData.rotation = zVec3_Make(0.1f, 0.2f, 0.3f);
    zClass_NodePartial rootNode = {};
    rootNode.classId = 5;
    rootNode.classData = &objectData;

    zUtil_PlayerStateStorage playerState = {};
    playerState.rootNode = &rootNode;
    playerState.lifecycleState = 6;
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;

    zMat4x3 expectedBasis = {};
    zMath::MatBuildEulerRotation3x3(&expectedBasis, objectData.rotation.x, objectData.rotation.y,
                                    objectData.rotation.z);
    expectedBasis.posX = objectData.localMatrix[9];
    expectedBasis.posY = objectData.localMatrix[10];
    expectedBasis.posZ = objectData.localMatrix[11];

    Player::SyncLocalPoseFromRootNode();

    const bool ok =
        Vec3Equals(playerState.worldPos, zVec3_Make(12.0f, 34.0f, 56.0f)) &&
        Vec3Equals(playerState.vehicleRotationAngles, objectData.rotation) &&
        MatrixEquals(playerState.motionBasis, expectedBasis) &&
        MatrixEquals(playerState.previousTransform, expectedBasis) &&
        playerState.lifecycleState == 1;

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    return ok ? 0 : 1;
}

extern "C" int player_get_aiv_zrd_path_smoke(void) {
    const char *const path = Player::GetAivZrdPath();
    return path != nullptr && std::strcmp(path, "aiv.zrd") == 0 ? 0 : 1;
}

extern "C" int player_extract_vehicle_name_from_aiv_name_smoke(void) {
    char out[64] = {};

    Player::ExtractVehicleNameFromAivName("tank_01", out);
    if (std::strcmp(out, "tank") != 0) {
        return 1;
    }

    Player::ExtractVehicleNameFromAivName("hover_alpha", out);
    if (std::strcmp(out, "hover_alpha") != 0) {
        return 2;
    }

    Player::ExtractVehicleNameFromAivName("_1", out);
    if (std::strcmp(out, "") != 0) {
        return 3;
    }

    Player::ExtractVehicleNameFromAivName("", out);
    return std::strcmp(out, "") == 0 ? 0 : 4;
}

extern "C" int player_clone_type6_node_from_template_and_rename_smoke(void) {
    zClass_TypeListLink *const oldType6Head = zClass_TypeList::Head(6);
    zClass_TypeListLink *const oldType6Tail = zClass_TypeList::Tail(6);
    zClass_NodePartial *const oldRuntimeDiScene = g_Player_RuntimeDiScene;
    int *const oldNetworkEnabledPtr = ZOPT_NETWORK_ENABLED;

    int networkEnabled = 1;
    ZOPT_NETWORK_ENABLED = &networkEnabled;

    zClass_NodePartial world = {};
    zClass_NodePartial templateNode = {};
    zClass_TypeListLink templateLink = {&templateNode, nullptr, nullptr, 0};

    std::strcpy(templateNode.name, "template_lod");
    templateNode.classId = 6;
    templateNode.flags = 0x04000080;
    templateNode.gridCol = -1;
    templateNode.gridRow = -1;
    world.classId = 2;

    zClass_TypeList::Head(6) = &templateLink;
    zClass_TypeList::Tail(6) = &templateLink;
    g_Player_RuntimeDiScene = &world;

    zClass_NodePartial *const missing =
        Player::CloneType6NodeFromTemplateAndRename("missing_template", "unused");
    zClass_NodePartial *const cloned =
        Player::CloneType6NodeFromTemplateAndRename("template_lod", "runtime_lod");

    const bool ok = missing == nullptr && cloned == &templateNode &&
                    std::strcmp(templateNode.name, "runtime_lod") == 0 &&
                    (templateNode.flags & 0x04) != 0 && world.listCountB == 1 &&
                    world.listB != nullptr && world.listB[0] == &templateNode &&
                    templateNode.listCountA == 1 && templateNode.listA != nullptr &&
                    templateNode.listA[0] == &world && templateNode.gridCol == -1 &&
                    templateNode.gridRow == -1;

    std::free(world.listB);
    std::free(templateNode.listA);
    zClass_TypeList::Head(6) = oldType6Head;
    zClass_TypeList::Tail(6) = oldType6Tail;
    g_Player_RuntimeDiScene = oldRuntimeDiScene;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabledPtr;
    return ok ? 0 : 1;
}

extern "C" int player_create_from_names_at_pose_smoke(void) {
    zUtil_SaveGameState *const oldSaveHead = g_PlayerSaveStateListHead;
    zUtil_SaveGameState *const oldSaveTail = g_PlayerSaveStateListTail;
    const int oldSaveAux = g_PlayerSaveStateListAux;
    const int oldSaveCount = g_PlayerSaveStateCount;
    PlayerMasterCommonData *const oldCommonHead = g_PlayerMasterCommonDataHead;
    PlayerMasterCommonData *const oldCommonTail = g_PlayerMasterCommonDataTail;
    const int oldCommonAux = g_PlayerMasterCommonDataListAux;
    const int oldCommonCount = g_PlayerMasterCommonDataCount;
    PlayerMasterModalData *const oldModalHead = g_PlayerMasterModalDataHead;
    PlayerMasterModalData *const oldModalTail = g_PlayerMasterModalDataTail;
    const int oldModalAux = g_PlayerMasterModalDataListAux;
    const int oldModalCount = g_PlayerMasterModalDataCount;
    zClass_TypeListLink *const oldType6Head = zClass_TypeList::Head(6);
    zClass_TypeListLink *const oldType6Tail = zClass_TypeList::Tail(6);
    zClass_NodePartial *const oldRuntimeDiScene = g_Player_RuntimeDiScene;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    const int oldNextOrdinal = g_Player_NextOrdinal;
    const int oldMissionStat1 = g_HudSensorTracker.missionStat1;
    const float oldNominalGravity = g_Player_NominalGravity;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    int *const oldGameControlOptions = ZOPT_GAME_CONTROL_OPTIONS;
    zEffectAnimEntry *const oldEffectEntries = g_zEffectAnim_EntryList;
    const short oldEffectCount = g_zEffectAnim_EntryCount;
    zZbdManager *const oldZbdManager = g_zUtil_ZbdManager;
    AINet *const oldAiHead = g_AINetListHead;
    AINet *const oldAiTail = g_AINetListTail;
    int *const oldMatrixIdentityFlagSlot = zMath::g_currentMatrixIdentityFlagSlot;
    float **const oldMatrixPtrSlot = zMath::g_currentMatrixPtrSlot;
    zVec3 *const oldSharedScratchA = g_zModel_SharedVec3ScratchA;
    zVec3 *const oldSharedScratchB = g_zModel_SharedVec3ScratchB;

    int networkEnabled = 1;
    int gameControlOptions = 0;
    static std::int32_t matrixFlags[2];
    static float *matrixSlots[2];
    static zMat4x3 identityMatrix;
    identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixFlags[0] = 1;
    matrixSlots[0] = (float *)&identityMatrix;
    zClass_NodePartial world = {};
    zClass_WorldDataPartial worldData = {};
    zClass_NodeFreeListSlot rootSlot = {};
    zClass_NodePartial &rootNode = rootSlot.node;
    zClass_Object3DDataPartial rootData = {};
    zClass_TypeListLink rootLink = {&rootNode, nullptr, nullptr, 0};
    PlayerMasterCommonData commonData = {};
    PlayerMasterModalData modalData = {};

    world.classId = 2;
    world.classData = &worldData;
    std::strcpy(rootNode.name, "net_tank");
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    rootData.localMatrix[0] = 1.0f;
    rootData.localMatrix[4] = 1.0f;
    rootData.localMatrix[8] = 1.0f;

    std::strcpy(commonData.vehicleName, "tank_common");
    std::strcpy(commonData.modalNames[0], "track");
    std::strcpy(commonData.startAnimsName, "startup");
    commonData.modalCount = 1;
    commonData.maxHealth = 150.0f;
    commonData.cambackDist0 = 12.0f;
    commonData.cambackSide0 = 1.0f;
    commonData.cambackBase0 = 2.0f;
    std::strcpy(modalData.modalName, "tank_common");
    std::strcpy(modalData.modeName, "track");
    modalData.probePointCount = 1;
    modalData.platformPointCount = 1;
    modalData.probePoints[0] = zVec3_Make(0.5f, 1.0f, 1.5f);

    g_PlayerSaveStateListHead = nullptr;
    g_PlayerSaveStateListTail = nullptr;
    g_PlayerSaveStateListAux = 0;
    g_PlayerSaveStateCount = 0;
    g_PlayerMasterCommonDataHead = &commonData;
    g_PlayerMasterCommonDataTail = &commonData;
    g_PlayerMasterCommonDataListAux = 1;
    g_PlayerMasterCommonDataCount = 1;
    g_PlayerMasterModalDataHead = &modalData;
    g_PlayerMasterModalDataTail = &modalData;
    g_PlayerMasterModalDataListAux = 1;
    g_PlayerMasterModalDataCount = 1;
    zClass_TypeList::Head(6) = &rootLink;
    zClass_TypeList::Tail(6) = &rootLink;
    g_Player_RuntimeDiScene = &world;
    g_GameStateOrMapTable = nullptr;
    g_Player_NextOrdinal = 1;
    g_HudSensorTracker.missionStat1 = oldMissionStat1;
    g_Player_NominalGravity = 19.5f;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    ZOPT_GAME_CONTROL_OPTIONS = &gameControlOptions;
    g_zEffectAnim_EntryList = nullptr;
    g_zEffectAnim_EntryCount = 0;
    g_zUtil_ZbdManager = nullptr;
    g_AINetListHead = nullptr;
    g_AINetListTail = nullptr;
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchA = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    const zVec3 spawnPos = {11.0f, 22.0f, 33.0f};
    const int createResult =
        Player::CreateFromNamesAtPose(&spawnPos, 77, 90.0f, "tank_common", "net_tank");

    zUtil_SaveGameState *const createdSave = g_PlayerSaveStateListHead;
    zUtil_PlayerStateStorage *const playerState =
        createdSave != nullptr ? createdSave->playerState : nullptr;
    PlayerModalState *const modalState =
        createdSave != nullptr ? createdSave->primaryModalState : nullptr;
    zUtil_SaveGameState *wrapperSave = nullptr;
    PlayerModalState *wrapperModalState = nullptr;

    int result = 0;
    if (createResult != 1 || createdSave == nullptr || playerState == nullptr ||
        g_PlayerSaveStateListTail != createdSave || g_PlayerSaveStateCount != 1) {
        result = 1;
    } else if (playerState->rootNode != &rootNode || playerState->aiNetId != 77 ||
               !FloatNear(rootData.localMatrix[9], 11.0f) ||
               !FloatNear(rootData.localMatrix[10], 22.0f) ||
               !FloatNear(rootData.localMatrix[11], 33.0f) ||
               !FloatNear(playerState->restartYawRad, 1.57079637f)) {
        result = 2;
    } else if (world.listCountB != 1 || world.listB == nullptr || world.listB[0] != &rootNode ||
               rootNode.listCountA != 1 || rootNode.listA == nullptr ||
               rootNode.listA[0] != &world) {
        result = 3;
    } else if (playerState->masterCommonData != &commonData ||
               playerState->statusMeterValue != 150.0f ||
               modalState == nullptr || modalState->masterModalData != &modalData ||
               createdSave->primaryModalState != modalState) {
        result = 4;
    } else if (playerState->gravityAccel != 19.5f ||
               !Vec3Equals(playerState->rootProbeWorldByIndex[0],
                           zVec3_Make(11.5f, 23.0f, 34.5f)) ||
               g_GameStateOrMapTable != (zInput_GameStateOrMapTablePartial *)createdSave ||
               g_Player_NextOrdinal != 2 || g_HudSensorTracker.missionStat1 != oldMissionStat1) {
        result = 5;
    } else {
        const zVec3 wrapperSpawnPos = {44.0f, 55.0f, 66.0f};
        wrapperSave = Player::CreateFromNamesAtPoseGetState(&wrapperSpawnPos, "tank_common",
                                                            180.0f, "net_tank");
        zUtil_PlayerStateStorage *const wrapperState =
            wrapperSave != nullptr ? wrapperSave->playerState : nullptr;
        wrapperModalState = wrapperSave != nullptr ? wrapperSave->primaryModalState : nullptr;
        if (wrapperSave == nullptr || wrapperSave != g_PlayerSaveStateListTail ||
            createdSave->next != wrapperSave || g_PlayerSaveStateCount != 2 ||
            wrapperState == nullptr || wrapperState->aiNetId != 0 ||
            !FloatNear(wrapperState->restartYawRad, 3.14159274f) ||
            !FloatNear(rootData.localMatrix[9], 44.0f) ||
            !FloatNear(rootData.localMatrix[10], 55.0f) ||
            !FloatNear(rootData.localMatrix[11], 66.0f) || g_Player_NextOrdinal != 3) {
            result = 6;
        }
    }

    if (wrapperSave != nullptr) {
        if (wrapperModalState != nullptr) {
            std::free(wrapperModalState);
        }
        std::free(wrapperSave->playerState);
        ::operator delete(wrapperSave);
    }
    if (createdSave != nullptr) {
        if (modalState != nullptr) {
            std::free(modalState);
        }
        std::free(createdSave->playerState);
        ::operator delete(createdSave);
    }
    std::free(world.listB);
    std::free(rootNode.listA);

    g_PlayerSaveStateListHead = oldSaveHead;
    g_PlayerSaveStateListTail = oldSaveTail;
    g_PlayerSaveStateListAux = oldSaveAux;
    g_PlayerSaveStateCount = oldSaveCount;
    g_PlayerMasterCommonDataHead = oldCommonHead;
    g_PlayerMasterCommonDataTail = oldCommonTail;
    g_PlayerMasterCommonDataListAux = oldCommonAux;
    g_PlayerMasterCommonDataCount = oldCommonCount;
    g_PlayerMasterModalDataHead = oldModalHead;
    g_PlayerMasterModalDataTail = oldModalTail;
    g_PlayerMasterModalDataListAux = oldModalAux;
    g_PlayerMasterModalDataCount = oldModalCount;
    zClass_TypeList::Head(6) = oldType6Head;
    zClass_TypeList::Tail(6) = oldType6Tail;
    g_Player_RuntimeDiScene = oldRuntimeDiScene;
    g_GameStateOrMapTable = oldGameState;
    g_Player_NextOrdinal = oldNextOrdinal;
    g_HudSensorTracker.missionStat1 = oldMissionStat1;
    g_Player_NominalGravity = oldNominalGravity;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    ZOPT_GAME_CONTROL_OPTIONS = oldGameControlOptions;
    g_zEffectAnim_EntryList = oldEffectEntries;
    g_zEffectAnim_EntryCount = oldEffectCount;
    g_zUtil_ZbdManager = oldZbdManager;
    g_AINetListHead = oldAiHead;
    g_AINetListTail = oldAiTail;
    zMath::g_currentMatrixIdentityFlagSlot = oldMatrixIdentityFlagSlot;
    zMath::g_currentMatrixPtrSlot = oldMatrixPtrSlot;
    g_zModel_SharedVec3ScratchA = oldSharedScratchA;
    g_zModel_SharedVec3ScratchB = oldSharedScratchB;
    return result;
}

extern "C" int player_init_mission_runtime_missing_aiv_smoke(void) {
    zUtil_SaveGameState *const oldSaveHead = g_PlayerSaveStateListHead;
    zUtil_SaveGameState *const oldSaveTail = g_PlayerSaveStateListTail;
    const int oldSaveAux = g_PlayerSaveStateListAux;
    const int oldSaveCount = g_PlayerSaveStateCount;
    zUtil_SaveGameState *const oldPlayer2SaveState = g_Player2SaveState;
    zUtil_SaveGameState *const oldLocalSaveState = g_LocalPlayerSaveState;
    zUtil_SaveGameState *const oldCurrentSaveState = g_CurrentPlayerSaveState;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    PlayerMasterCommonData *const oldCommonHead = g_PlayerMasterCommonDataHead;
    PlayerMasterCommonData *const oldCommonTail = g_PlayerMasterCommonDataTail;
    const int oldCommonAux = g_PlayerMasterCommonDataListAux;
    const int oldCommonCount = g_PlayerMasterCommonDataCount;
    PlayerMasterModalData *const oldModalHead = g_PlayerMasterModalDataHead;
    PlayerMasterModalData *const oldModalTail = g_PlayerMasterModalDataTail;
    const int oldModalAux = g_PlayerMasterModalDataListAux;
    const int oldModalCount = g_PlayerMasterModalDataCount;
    const int oldMissionInitFirstRun = g_Player_MissionInitFirstRunFlag;
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    const HudUiPanel oldTopPanel1 = g_Player_TopMsgPanel1;
    const HudUiPanel oldTopPanel2 = g_Player_TopMsgPanel2;
    const HudUiElement oldUnderwaterFxPass3Ui = g_Player_UnderwaterFxPass3Ui;
    const HudUiElement oldState7FxPass3Ui = g_Player_State7FxPass3Ui;
    HudUiContainer *const fxContainer =
        reinterpret_cast<HudUiContainer *>(&g_zVideo_FxPass3ConfigLocal);
    const HudUiContainer oldFxContainer = *fxContainer;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    zClass_NodePartial *const oldHorizonNode = g_Player_HorizonNode;
    const int oldHorizonEnabled = g_Player_HorizonNodeFollowCameraEnabled;
    const int oldRuntimeInputFlags = g_Player_RuntimeInputFlags;
    const int oldLocalControlEnabled = g_Player_LocalControlEnabled;
    const int oldNextOrdinal = g_Player_NextOrdinal;
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;
    const float oldTotalTime = g_Player_TotalTimeSecScaled;
    const float oldCameraZone = g_Player_CameraZone;
    const float oldCameraZoneInvRange = g_Player_CameraZoneInvRange;
    const float oldNominalGravity = g_Player_NominalGravity;
    const float oldStatusMeterRatio = g_PlayerStatusMeterRatio;
    zClass_NodeFreeListSlot *const oldNodeArray = g_zClass_NodeArray;
    const int oldFreeHead = g_zClass_NodeFreeHeadIndex;
    const int oldActiveNodeCount = g_zClass_ActiveNodeCount;
    zClass_TypeListLink *const oldFreeLinkHead = g_zClass_TypeList_FreeLinkHead;
    zClass_TypeListLink *const oldPendingFreeHead = g_zClass_NodeList_PendingFreeHead;
    const int oldDeferredProcessing = g_zClass_DeferredProcessingEnabled;
    const int oldLiveLinkCount = g_zClass_TypeList_LiveLinkCount;
    const int oldPeakLiveLinkCount = g_zClass_TypeList_PeakLiveLinkCount;
    zClass_NodePartial *const oldHudWorldNode = g_HudSensorTracker.worldNode;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    int *const oldGameControlOptions = ZOPT_GAME_CONTROL_OPTIONS;
    int *const oldDifficultyOption = g_zOpt_GameDifficultyOption;
    zArchiveList *const oldMountedList = g_zArchive_MountedList;
    zClass_TypeListLink *oldTypeHeads[16] = {};
    zClass_TypeListLink *oldTypeTails[16] = {};
    int oldTypeDirty[16] = {};
    for (int i = 0; i < 16; ++i) {
        oldTypeHeads[i] = zClass_TypeList::Head(i);
        oldTypeTails[i] = zClass_TypeList::Tail(i);
        oldTypeDirty[i] = zClass_TypeList::PendingRemovalDirty(i);
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }

    char tempPath[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempPath), tempPath) == 0 ||
        GetTempFileNameA(tempPath, "pim", 0, tempFile) == 0) {
        return 1;
    }

    zReader::Node playerRoot = {};
    zReader::Node playerItems[3] = {};
    zReader::Node cameraZoneItems[2] = {};
    MakeAinetReaderArrayNode(playerRoot, playerItems, 3);
    MakeAinetReaderStringNode(playerItems[1], "camera_zone");
    MakeAinetReaderArrayNode(playerItems[2], cameraZoneItems, 2);
    MakeAinetReaderFloatNode(cameraZoneItems[1], 0.75f);

    zReader::Node vehicleRoot = {};
    zReader::Node vehicleItems[3] = {};
    zReader::Node stealthItems[5] = {};
    zReader::Node commonItems[1] = {};
    zReader::Node modalItems[3] = {};
    zReader::Node modeItems[2] = {};
    MakeAinetReaderArrayNode(vehicleRoot, vehicleItems, 3);
    MakeAinetReaderStringNode(vehicleItems[1], "stealth");
    MakeAinetReaderArrayNode(vehicleItems[2], stealthItems, 5);
    MakeAinetReaderStringNode(stealthItems[1], "common_mode");
    MakeAinetReaderArrayNode(stealthItems[2], commonItems, 1);
    MakeAinetReaderStringNode(stealthItems[3], "basic");
    MakeAinetReaderArrayNode(stealthItems[4], modalItems, 3);
    MakeAinetReaderStringNode(modalItems[1], "mode");
    MakeAinetReaderArrayNode(modalItems[2], modeItems, 2);
    MakeAinetReaderStringNode(modeItems[1], "stealth");

    const AinetZrdArchiveEntry entries[] = {
        {"player.zrd", &playerRoot},
        {"vehicle.zrd", &vehicleRoot},
    };
    zIndexArchive archive = {};
    zZarFileRecord records[2] = {};
    zArchiveListNode archiveNode = {};
    zArchiveList archiveList = {};
    if (!MountAinetZrdArchive(tempFile, entries, 2, archive, records, archiveNode,
                              archiveList)) {
        return 2;
    }

    HudUiTopMessageStack topStack = {};
    topStack.Constructor();
    HudUiElement *const oldTopStackTail = topStack.base.childTail;
    g_HudUiTopMessageStack = &topStack;
    HudUiCommon_FTable visibleTable = g_HudUiCommon_FTable;
    visibleTable.slots[24] = MethodAddress(&TestPlayerHudVisibleReceiver::SetVisible);
    g_Player_TopMsgPanel1 = {};
    g_Player_TopMsgPanel2 = {};
    g_Player_TopMsgPanel1.vtbl = &g_HudUiPanel_FTable;
    g_Player_TopMsgPanel2.vtbl = &g_HudUiPanel_FTable;
    g_Player_UnderwaterFxPass3Ui = {};
    g_Player_State7FxPass3Ui = {};
    g_Player_UnderwaterFxPass3Ui.ftable = &visibleTable;
    g_Player_State7FxPass3Ui.ftable = &visibleTable;
    *fxContainer = {};

    zClass_NodePartial worldNode = {};
    zClass_WorldDataPartial worldData = {};
    worldNode.classId = 2;
    worldNode.classData = &worldData;
    zClass_NodePartial cameraNode = {};
    zClass_CameraDataPartial cameraData = {};
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    cameraData.frustumWidth = 1.25f;
    cameraData.frustumHeight = 0.75f;

    zClass_NodeFreeListSlot slots[8] = {};
    for (int i = 0; i < 7; ++i) {
        slots[i].freeTag = i + 1;
    }
    slots[7].freeTag = 0x00ffffff;
    g_zClass_NodeArray = slots;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_DeferredProcessingEnabled = 1;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    int networkEnabled = 1;
    int gameControlOptions = 0;
    int difficultyOption = 1;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    ZOPT_GAME_CONTROL_OPTIONS = &gameControlOptions;
    g_zOpt_GameDifficultyOption = &difficultyOption;
    g_PlayerSaveStateListHead = nullptr;
    g_PlayerSaveStateListTail = nullptr;
    g_PlayerSaveStateListAux = 0;
    g_PlayerSaveStateCount = 0;
    g_Player2SaveState = nullptr;
    g_LocalPlayerSaveState = nullptr;
    g_CurrentPlayerSaveState = nullptr;
    g_GameStateOrMapTable = nullptr;
    g_PlayerMasterCommonDataHead = nullptr;
    g_PlayerMasterCommonDataTail = nullptr;
    g_PlayerMasterCommonDataListAux = 0;
    g_PlayerMasterCommonDataCount = 0;
    g_PlayerMasterModalDataHead = nullptr;
    g_PlayerMasterModalDataTail = nullptr;
    g_PlayerMasterModalDataListAux = 0;
    g_PlayerMasterModalDataCount = 0;
    g_Player_MissionInitFirstRunFlag = 1;
    g_Player_HorizonNode = nullptr;
    g_Player_HorizonNodeFollowCameraEnabled = 0;
    g_Player_RuntimeInputFlags = 0;
    g_Player_LocalControlEnabled = 0;
    g_Player_NextOrdinal = 1;
    g_Time_AccumulatedTimeSec = 9.5f;
    g_Player_TotalTimeSecScaled = 9.5f;
    g_Player_CameraZone = 0.5f;
    g_Player_CameraZoneInvRange = 2.0f;
    g_Player_NominalGravity = 0.0f;
    g_PlayerStatusMeterRatio = 0.0f;
    g_HudSensorTracker.worldNode = &worldNode;
    std::memset(g_PlayerTestHudVisibleThis, 0, sizeof(g_PlayerTestHudVisibleThis));
    std::memset(g_PlayerTestHudVisibleValue, 0, sizeof(g_PlayerTestHudVisibleValue));
    g_PlayerTestHudVisibleCount = 0;

    Player::InitMissionRuntimeFromWorldAndCamera(&worldNode, &cameraNode);

    zUtil_SaveGameState *const stealthSave = g_Player2SaveState;
    zUtil_PlayerStateStorage *const stealthState =
        stealthSave != nullptr ? stealthSave->playerState : nullptr;
    PlayerMasterCommonData *const commonData = g_PlayerMasterCommonDataHead;
    PlayerMasterModalData *const modalData = g_PlayerMasterModalDataHead;

    int result = 0;
    if (g_Player_MissionInitFirstRunFlag != 0 ||
        oldTopStackTail == nullptr ||
        oldTopStackTail->next != reinterpret_cast<HudUiElement *>(&g_Player_TopMsgPanel1) ||
        reinterpret_cast<HudUiElement *>(&g_Player_TopMsgPanel1)->next !=
            reinterpret_cast<HudUiElement *>(&g_Player_TopMsgPanel2) ||
        topStack.base.childTail != reinterpret_cast<HudUiElement *>(&g_Player_TopMsgPanel2)) {
        result = 3;
    } else if (g_Player_RuntimeDiScene != &worldNode || g_MainCamera != &cameraNode ||
               cameraData.posOffset.x != 0.0f || cameraData.posOffset.y != 0.0f ||
               cameraData.posOffset.z != 0.0f || cameraData.targetOrEuler.x != 0.0f ||
               cameraData.targetOrEuler.y != 0.0f || cameraData.targetOrEuler.z != 0.0f) {
        result = 4;
    } else if ((reinterpret_cast<HudUiElement *>(&g_Player_TopMsgPanel1)->flags & 0x10) == 0 ||
               (reinterpret_cast<HudUiElement *>(&g_Player_TopMsgPanel2)->flags & 0x10) == 0 ||
               g_PlayerTestHudVisibleCount != 2 ||
               g_PlayerTestHudVisibleThis[0] != &g_Player_UnderwaterFxPass3Ui ||
               g_PlayerTestHudVisibleThis[1] != &g_Player_State7FxPass3Ui ||
               g_PlayerTestHudVisibleValue[0] != 0 || g_PlayerTestHudVisibleValue[1] != 0) {
        result = 5;
    } else if (g_Player_LocalControlEnabled != 1 || g_Player_RuntimeInputFlags != 3 ||
               !FloatNear(g_Player_TotalTimeSecScaled, 9.5f) ||
               !FloatNear(g_Player_CameraZone, 0.75f) ||
               !FloatNear(g_Player_CameraZoneInvRange, 4.0f) ||
               !FloatNear(g_Player_NominalGravity, 28.0f) ||
               !FloatNear(g_PlayerStatusMeterRatio, 1.0f)) {
        result = 6;
    } else if (commonData == nullptr || modalData == nullptr ||
               g_PlayerMasterCommonDataCount != 1 || g_PlayerMasterModalDataCount != 1 ||
               std::strcmp(commonData->vehicleName, "stealth") != 0 ||
               commonData->modalCount != 1 || std::strcmp(modalData->modalName, "stealth") != 0 ||
               std::strcmp(modalData->modeName, "stealth") != 0) {
        result = 7;
    } else if (stealthSave == nullptr || stealthSave != g_PlayerSaveStateListHead ||
               stealthState == nullptr || stealthState->rootNode == nullptr ||
               std::strcmp(stealthState->rootNode->name, "Stealth") != 0 ||
               stealthState->lifecycleState != 4 || stealthState->cameraState != 3 ||
               !FloatNear(stealthState->worldPos.x, 500.0f) ||
               !FloatNear(stealthState->worldPos.y, 50.0f) ||
               !FloatNear(stealthState->worldPos.z, 500.0f)) {
        result = 8;
    } else if (g_LocalPlayerSaveState != nullptr || g_CurrentPlayerSaveState != nullptr ||
               g_PlayerSaveStateCount != 1 ||
               g_GameStateOrMapTable !=
                   reinterpret_cast<zInput_GameStateOrMapTablePartial *>(stealthSave)) {
        result = 9;
    }

    Player::ShutdownMissionRuntime();
    for (int i = 0; i < 8; ++i) {
        free(slots[i].node.classData);
        slots[i].node.classData = nullptr;
    }
    if (archive.hFile != INVALID_HANDLE_VALUE && archive.hFile != nullptr) {
        CloseHandle(archive.hFile);
    }
    DeleteFileA(tempFile);

    g_PlayerSaveStateListHead = oldSaveHead;
    g_PlayerSaveStateListTail = oldSaveTail;
    g_PlayerSaveStateListAux = oldSaveAux;
    g_PlayerSaveStateCount = oldSaveCount;
    g_Player2SaveState = oldPlayer2SaveState;
    g_LocalPlayerSaveState = oldLocalSaveState;
    g_CurrentPlayerSaveState = oldCurrentSaveState;
    g_GameStateOrMapTable = oldGameState;
    g_PlayerMasterCommonDataHead = oldCommonHead;
    g_PlayerMasterCommonDataTail = oldCommonTail;
    g_PlayerMasterCommonDataListAux = oldCommonAux;
    g_PlayerMasterCommonDataCount = oldCommonCount;
    g_PlayerMasterModalDataHead = oldModalHead;
    g_PlayerMasterModalDataTail = oldModalTail;
    g_PlayerMasterModalDataListAux = oldModalAux;
    g_PlayerMasterModalDataCount = oldModalCount;
    g_Player_MissionInitFirstRunFlag = oldMissionInitFirstRun;
    g_HudUiTopMessageStack = oldTopStack;
    g_Player_TopMsgPanel1 = oldTopPanel1;
    g_Player_TopMsgPanel2 = oldTopPanel2;
    g_Player_UnderwaterFxPass3Ui = oldUnderwaterFxPass3Ui;
    g_Player_State7FxPass3Ui = oldState7FxPass3Ui;
    *fxContainer = oldFxContainer;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_MainCamera = oldMainCamera;
    g_Player_HorizonNode = oldHorizonNode;
    g_Player_HorizonNodeFollowCameraEnabled = oldHorizonEnabled;
    g_Player_RuntimeInputFlags = oldRuntimeInputFlags;
    g_Player_LocalControlEnabled = oldLocalControlEnabled;
    g_Player_NextOrdinal = oldNextOrdinal;
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    g_Player_TotalTimeSecScaled = oldTotalTime;
    g_Player_CameraZone = oldCameraZone;
    g_Player_CameraZoneInvRange = oldCameraZoneInvRange;
    g_Player_NominalGravity = oldNominalGravity;
    g_PlayerStatusMeterRatio = oldStatusMeterRatio;
    g_zClass_NodeArray = oldNodeArray;
    g_zClass_NodeFreeHeadIndex = oldFreeHead;
    g_zClass_ActiveNodeCount = oldActiveNodeCount;
    g_zClass_TypeList_FreeLinkHead = oldFreeLinkHead;
    g_zClass_NodeList_PendingFreeHead = oldPendingFreeHead;
    g_zClass_DeferredProcessingEnabled = oldDeferredProcessing;
    g_zClass_TypeList_LiveLinkCount = oldLiveLinkCount;
    g_zClass_TypeList_PeakLiveLinkCount = oldPeakLiveLinkCount;
    g_HudSensorTracker.worldNode = oldHudWorldNode;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    ZOPT_GAME_CONTROL_OPTIONS = oldGameControlOptions;
    g_zOpt_GameDifficultyOption = oldDifficultyOption;
    g_zArchive_MountedList = oldMountedList;
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = oldTypeHeads[i];
        zClass_TypeList::Tail(i) = oldTypeTails[i];
        zClass_TypeList::PendingRemovalDirty(i) = oldTypeDirty[i];
    }

    return result;
}

extern "C" int hud_sensor_tracker_init_mission_gameplay_systems_smoke(void) {
    zUtil_SaveGameState *const oldSaveHead = g_PlayerSaveStateListHead;
    zUtil_SaveGameState *const oldSaveTail = g_PlayerSaveStateListTail;
    const int oldSaveAux = g_PlayerSaveStateListAux;
    const int oldSaveCount = g_PlayerSaveStateCount;
    zUtil_SaveGameState *const oldPlayer2SaveState = g_Player2SaveState;
    zUtil_SaveGameState *const oldLocalSaveState = g_LocalPlayerSaveState;
    zUtil_SaveGameState *const oldCurrentSaveState = g_CurrentPlayerSaveState;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    PlayerMasterCommonData *const oldCommonHead = g_PlayerMasterCommonDataHead;
    PlayerMasterCommonData *const oldCommonTail = g_PlayerMasterCommonDataTail;
    const int oldCommonAux = g_PlayerMasterCommonDataListAux;
    const int oldCommonCount = g_PlayerMasterCommonDataCount;
    PlayerMasterModalData *const oldModalHead = g_PlayerMasterModalDataHead;
    PlayerMasterModalData *const oldModalTail = g_PlayerMasterModalDataTail;
    const int oldModalAux = g_PlayerMasterModalDataListAux;
    const int oldModalCount = g_PlayerMasterModalDataCount;
    const int oldMissionInitFirstRun = g_Player_MissionInitFirstRunFlag;
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    HudUiTextStack4 *const oldChatStack = g_HudUiChatMessageStack;
    const HudUiPanel oldTopPanel1 = g_Player_TopMsgPanel1;
    const HudUiPanel oldTopPanel2 = g_Player_TopMsgPanel2;
    const HudUiElement oldUnderwaterFxPass3Ui = g_Player_UnderwaterFxPass3Ui;
    const HudUiElement oldState7FxPass3Ui = g_Player_State7FxPass3Ui;
    HudUiContainer *const fxContainer =
        reinterpret_cast<HudUiContainer *>(&g_zVideo_FxPass3ConfigLocal);
    const HudUiContainer oldFxContainer = *fxContainer;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    zClass_NodePartial *const oldHorizonNode = g_Player_HorizonNode;
    const int oldHorizonEnabled = g_Player_HorizonNodeFollowCameraEnabled;
    const int oldRuntimeInputFlags = g_Player_RuntimeInputFlags;
    const int oldLocalControlEnabled = g_Player_LocalControlEnabled;
    const int oldNextOrdinal = g_Player_NextOrdinal;
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;
    const float oldTotalTime = g_Player_TotalTimeSecScaled;
    const float oldCameraZone = g_Player_CameraZone;
    const float oldCameraZoneInvRange = g_Player_CameraZoneInvRange;
    const float oldNominalGravity = g_Player_NominalGravity;
    const float oldStatusMeterRatio = g_PlayerStatusMeterRatio;
    zClass_NodeFreeListSlot *const oldNodeArray = g_zClass_NodeArray;
    const int oldFreeHead = g_zClass_NodeFreeHeadIndex;
    const int oldActiveNodeCount = g_zClass_ActiveNodeCount;
    zClass_TypeListLink *const oldFreeLinkHead = g_zClass_TypeList_FreeLinkHead;
    zClass_TypeListLink *const oldPendingFreeHead = g_zClass_NodeList_PendingFreeHead;
    const int oldDeferredProcessing = g_zClass_DeferredProcessingEnabled;
    const int oldLiveLinkCount = g_zClass_TypeList_LiveLinkCount;
    const int oldPeakLiveLinkCount = g_zClass_TypeList_PeakLiveLinkCount;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    int *const oldGameControlOptions = ZOPT_GAME_CONTROL_OPTIONS;
    int *const oldDifficultyOption = g_zOpt_GameDifficultyOption;
    int *const oldReplicate = ZOPT_REPLICATE;
    zArchiveList *const oldMountedList = g_zArchive_MountedList;
    zInput_BindMapContext *const oldBindMap = g_zInput_BindMap_Current;
    zOpt_ViewRectSection **const oldDisplayOption = g_zOpt_DisplaySectionOption;
    zOpt_ViewRectSection **const oldWindowOption = g_zOpt_WindowSectionOption;
    HudUiShieldMessageWidget *const oldShieldWidget = g_HudUiMgrShieldMessageWidget;
    HudLayoutBase *const oldCurrentLayout = g_HudUiMgrCurrentLayout;
    const HudUiRect oldHudRect = g_HudUiMgrHudRect;
    const HudUiRect oldViewRect = g_HudUiMgrViewRect;
    const HudUiMgrSensorBlock oldSensorBlock = g_HudUiMgrSensorBlock;
    const HudUiRect oldSensorFxRect = g_HudUiMgrSensorFxRect;
    const int oldSensorFxViewportWidth = g_HudUiMgrSensorFxViewportWidth;
    const int oldSensorFxViewportHeight = g_HudUiMgrSensorFxViewportHeight;
    const HudUiNanitePanel oldNanitePanel = g_HudUiMgrNanitePanel;
    HudUiMessage oldMessages[10];
    std::memcpy(oldMessages, g_HudUiMgrMessages, sizeof(oldMessages));
    HudUiCounter oldModeCounters[4];
    std::memcpy(oldModeCounters, g_HudUiMgrModeCounters, sizeof(oldModeCounters));
    const int oldActiveWeaponMessageIndex = g_HudUiMgrActiveWeaponMessageIndex;
    const int oldActiveWeaponSideIndex = g_HudUiMgrActiveWeaponSideIndex;
    const int oldActiveModeCounterIndex = g_HudUiMgrActiveModeCounterIndex;
    const HudUiWidget oldObjectiveWidget = g_HudUiMgrObjectiveWidget;
    const int oldObjectivePhase = g_HudUiMgrObjectivePhase;
    zClass_NodePartial *const oldEffectWorld = g_zEffect_World;
    zClass_NodePartial *const oldEffectResource = g_zEffect_ResourceNode;
    zEffectAnimEntry *const oldEffectEntries = g_zEffectAnim_EntryList;
    const short oldEffectCount = g_zEffectAnim_EntryCount;
    const int oldEffectInstantiated = g_zEffectAnim_EntriesInstantiated;
    char oldEffectZbdFilename[sizeof(g_zEffectAnim_ZbdFilename)] = {};
    std::memcpy(oldEffectZbdFilename, g_zEffectAnim_ZbdFilename,
                sizeof(g_zEffectAnim_ZbdFilename));
    zClass_NodePartial *const oldThermalGlowFreeList = g_OptCatalogThermalGlowFreeList;
    const int oldHudCounterValue = g_Player_HudCounterValue;
    const int oldDamageFeedbackHitCount = g_OptCatalog_DamageFeedbackHitCount;
    zClass_NodePartial *const oldHudWorldNode = g_HudSensorTracker.worldNode;
    const int oldHudMissionId = g_HudSensorTracker.missionId;
    const int oldHudRaceCheckpointMode = g_HudSensorTracker.raceCheckpointMode;
    const int oldHudMissionStat1 = g_HudSensorTracker.missionStat1;
    zModel_MaterialSlot *const oldMatlPool = g_zModel_MatlPool;
    const int oldMatlPoolCapacity = g_zModel_MatlPoolCapacity;
    const int oldMatlPoolInUseCount = g_zModel_MatlPoolInUseCount;
    const int oldMatlFreeHeadIndex = g_zModel_MatlFreeHeadIndex;
    const int oldMatlActiveHeadIndex = g_zModel_MatlActiveHeadIndex;
    zModel_MaterialPartial *const oldMatlReuseCache = g_zModel_MatlReuseCache;
    const int oldTexDirEntryCount = g_zImage_TexDirEntryCount;
    zImage_TexDirEntryPartial oldTexDirEntries[0x1000] = {};
    std::memcpy(oldTexDirEntries, g_zImage_TexDirEntries, sizeof(oldTexDirEntries));
    const int oldZdeclientRebuildBltRect = g_zDEClient_RebuildBltRectOnReload;
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const int oldBuiltinTexturePackCount = g_zVid_BuiltinTexturePackCount;
    zVidTexturePackEntry *const oldBuiltinTexturePacks = g_zVid_BuiltinTexturePacks;
    const int oldTexturePackCount = g_zVid_TexturePackCount;
    zVidTexturePackEntry *const oldTexturePacks = g_zVid_TexturePacks;
    const zSndSampleSetRegistry oldSndRegistry = g_zSnd_SampleSetRegistry;
    const int oldSndInitialized = g_zSnd_IsInitialized;
    const int oldActiveBackend = g_zSnd_ActiveBackend;
    const unsigned int oldLoadingMaxIndex = g_HudUiLoadingCheckpointMaxIndex;
    const unsigned int oldLoadingCurrentIndex = g_HudUiLoadingCheckpointCurrentIndex;
    const float oldLoadingCurrentProgress = g_HudUiLoadingCheckpointCurrentProgress;
    HudUiBriefingRuntime *const oldBriefingRuntime = g_Briefing_Runtime;

    zClass_TypeListLink *oldTypeHeads[16] = {};
    zClass_TypeListLink *oldTypeTails[16] = {};
    int oldTypeDirty[16] = {};
    for (int i = 0; i < 16; ++i) {
        oldTypeHeads[i] = zClass_TypeList::Head(i);
        oldTypeTails[i] = zClass_TypeList::Tail(i);
        oldTypeDirty[i] = zClass_TypeList::PendingRemovalDirty(i);
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }

    char tempPath[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempPath), tempPath) == 0 ||
        GetTempFileNameA(tempPath, "hsg", 0, tempFile) == 0) {
        return 1;
    }

    zReader::Node playerRoot = {};
    zReader::Node playerItems[3] = {};
    zReader::Node cameraZoneItems[2] = {};
    MakeAinetReaderArrayNode(playerRoot, playerItems, 3);
    MakeAinetReaderStringNode(playerItems[1], "camera_zone");
    MakeAinetReaderArrayNode(playerItems[2], cameraZoneItems, 2);
    MakeAinetReaderFloatNode(cameraZoneItems[1], 0.75f);

    zReader::Node vehicleRoot = {};
    zReader::Node vehicleItems[3] = {};
    zReader::Node stealthItems[5] = {};
    zReader::Node commonItems[1] = {};
    zReader::Node modalItems[3] = {};
    zReader::Node modeItems[2] = {};
    MakeAinetReaderArrayNode(vehicleRoot, vehicleItems, 3);
    MakeAinetReaderStringNode(vehicleItems[1], "stealth");
    MakeAinetReaderArrayNode(vehicleItems[2], stealthItems, 5);
    MakeAinetReaderStringNode(stealthItems[1], "common_mode");
    MakeAinetReaderArrayNode(stealthItems[2], commonItems, 1);
    MakeAinetReaderStringNode(stealthItems[3], "basic");
    MakeAinetReaderArrayNode(stealthItems[4], modalItems, 3);
    MakeAinetReaderStringNode(modalItems[1], "mode");
    MakeAinetReaderArrayNode(modalItems[2], modeItems, 2);
    MakeAinetReaderStringNode(modeItems[1], "stealth");

    zReader::Node declientRoot = {};
    zReader::Node declientItems[3] = {};
    zReader::Node craterItems[13] = {};
    MakeAinetReaderArrayNode(declientRoot, declientItems, 3);
    MakeAinetReaderStringNode(declientItems[1], "CRATER");
    MakeAinetReaderArrayNode(declientItems[2], craterItems, 13);
    MakeAinetReaderStringNode(craterItems[1], "POINTS");
    MakeAinetReaderIntNode(craterItems[2], 7);
    MakeAinetReaderStringNode(craterItems[3], "SLOPE");
    MakeAinetReaderFloatNode(craterItems[4], 0.0f);
    MakeAinetReaderStringNode(craterItems[5], "DEPTH");
    MakeAinetReaderFloatNode(craterItems[6], 4.0f);
    MakeAinetReaderStringNode(craterItems[7], "RADIUS");
    MakeAinetReaderFloatNode(craterItems[8], 20.0f);
    MakeAinetReaderStringNode(craterItems[9], "DEFAULT_TEXTURE");
    MakeAinetReaderStringNode(craterItems[10], "crater_base.tga");
    MakeAinetReaderStringNode(craterItems[11], "DEFAULT_ANIM");
    MakeAinetReaderStringNode(craterItems[12], "crater_fx");

    const AinetZrdArchiveEntry entries[] = {
        {"player.zrd", &playerRoot},
        {"vehicle.zrd", &vehicleRoot},
        {"declient.zrd", &declientRoot},
    };
    zIndexArchive archive = {};
    zZarFileRecord records[3] = {};
    zArchiveListNode archiveNode = {};
    zArchiveList archiveList = {};
    if (!MountAinetZrdArchive(tempFile, entries, 3, archive, records, archiveNode,
                              archiveList)) {
        return 2;
    }

    HudUiTopMessageStack topStack = {};
    topStack.Constructor();
    g_HudUiTopMessageStack = &topStack;
    g_HudUiChatMessageStack = nullptr;
    HudUiCommon_FTable visibleTable = g_HudUiCommon_FTable;
    visibleTable.slots[24] = MethodAddress(&TestPlayerHudVisibleReceiver::SetVisible);
    g_Player_TopMsgPanel1 = {};
    g_Player_TopMsgPanel2 = {};
    g_Player_TopMsgPanel1.vtbl = &g_HudUiPanel_FTable;
    g_Player_TopMsgPanel2.vtbl = &g_HudUiPanel_FTable;
    g_Player_UnderwaterFxPass3Ui = {};
    g_Player_State7FxPass3Ui = {};
    g_Player_UnderwaterFxPass3Ui.ftable = &visibleTable;
    g_Player_State7FxPass3Ui.ftable = &visibleTable;
    *fxContainer = {};

    HudUiShieldMessageWidget shield = {};
    shield.widget.Constructor(0);
    reinterpret_cast<HudUiPanel *>(&shield.percentTextPanel)->ConstructorDefault("", 0, 0);
    shield.meter.Constructor();
    shield.meter.fillPixelsMax = 100;
    shield.meter.points[1].y = 100.0f;
    g_HudUiMgrShieldMessageWidget = &shield;
    g_HudUiMgrCurrentLayout = nullptr;
    g_HudUiMgrSensorBlock = {};
    g_HudUiMgrSensorBlock.sensorParam = 1.0f;
    g_HudUiMgrSensorFxRect = {0, 0, 100, 80};
    g_HudUiMgrSensorFxViewportWidth = 100;
    g_HudUiMgrSensorFxViewportHeight = 80;
    g_HudUiMgrNanitePanel = {};
    g_HudUiMgrNanitePanel.base.ftable =
        reinterpret_cast<const HudUiCommon_FTable *>(&g_HudUiTripletPanel_FTable);
    for (HudUiWidget &item : g_HudUiMgrNanitePanel.items) {
        item.ftable = &g_HudUiWidget_FTable;
    }
    for (HudUiMessage &message : g_HudUiMgrMessages) {
        message.Constructor();
    }
    for (HudUiCounter &counter : g_HudUiMgrModeCounters) {
        counter.Constructor();
    }
    g_HudUiMgrObjectiveWidget.Constructor(0);
    g_HudUiMgrObjectivePhase = 0;
    g_HudUiMgrActiveWeaponMessageIndex = 0;
    g_HudUiMgrActiveWeaponSideIndex = 0;
    g_HudUiMgrActiveModeCounterIndex = 0;

    zClass_NodePartial worldNode = {};
    zClass_WorldDataPartial worldData = {};
    worldNode.classId = 2;
    worldNode.classData = &worldData;
    std::strcpy(worldNode.name, "world1");
    zClass_NodePartial cameraNode = {};
    zClass_CameraDataPartial cameraData = {};
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    cameraData.frustumWidth = 1.25f;
    cameraData.frustumHeight = 0.75f;
    zClass_NodePartial resourceNode = {};

    zClass_NodeFreeListSlot slots[32] = {};
    for (int i = 0; i < 31; ++i) {
        slots[i].freeTag = i + 1;
    }
    slots[31].freeTag = 0x00ffffff;
    g_zClass_NodeArray = slots;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_DeferredProcessingEnabled = 1;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    int networkEnabled = 0;
    int gameControlOptions = 0;
    int difficultyOption = 1;
    int replicate = 0;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    ZOPT_GAME_CONTROL_OPTIONS = &gameControlOptions;
    g_zOpt_GameDifficultyOption = &difficultyOption;
    ZOPT_REPLICATE = &replicate;

    zOpt_ViewRectSection display = {};
    display.rightExclusive = 640;
    display.bottomExclusive = 480;
    display.width = 640;
    display.height = 480;
    display.bitsPerPixel = 16;
    zOpt_ViewRectSection window = {};
    window.rightExclusive = 640;
    window.bottomExclusive = 480;
    window.width = 640;
    window.height = 480;
    zOpt_ViewRectSection *displayPtr = &display;
    zOpt_ViewRectSection *windowPtr = &window;
    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_WindowSectionOption = &windowPtr;

    zInput_BindMapContext context = {};
    context.InitCommandMap(30);
    for (int commandId = 24; commandId <= 29; ++commandId) {
        char label[0x20];
        std::sprintf(label, "Command %d", commandId);
        context.SetBindingRecord(commandId, label, 0x20 + commandId, 0, 0, 0);
    }
    g_zInput_BindMap_Current = &context;

    zUtil_SaveGameState fakeLocalSave = {};
    zUtil_PlayerStateStorage fakeLocalState = {};
    PlayerMasterCommonData fakeCommon = {};
    PlayerGunFireController fakeAlt = {};
    PlayerGunFireController fakePrimary = {};
    fakeLocalSave.playerState = &fakeLocalState;
    fakeLocalState.masterCommonData = &fakeCommon;
    fakeLocalState.activeAltGunController = &fakeAlt;
    fakeLocalState.activePrimaryGunController = &fakePrimary;
    fakeLocalState.statusMeterValue = 25.0f;
    fakeLocalState.nanitePanelLevel = 2;
    fakeCommon.maxHealth = 100.0f;
    fakeCommon.invMaxHealth = 0.01f;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&fakeLocalSave);

    g_PlayerSaveStateListHead = nullptr;
    g_PlayerSaveStateListTail = nullptr;
    g_PlayerSaveStateListAux = 0;
    g_PlayerSaveStateCount = 0;
    g_Player2SaveState = nullptr;
    g_LocalPlayerSaveState = nullptr;
    g_CurrentPlayerSaveState = nullptr;
    g_PlayerMasterCommonDataHead = nullptr;
    g_PlayerMasterCommonDataTail = nullptr;
    g_PlayerMasterCommonDataListAux = 0;
    g_PlayerMasterCommonDataCount = 0;
    g_PlayerMasterModalDataHead = nullptr;
    g_PlayerMasterModalDataTail = nullptr;
    g_PlayerMasterModalDataListAux = 0;
    g_PlayerMasterModalDataCount = 0;
    g_Player_MissionInitFirstRunFlag = 1;
    g_Player_HorizonNode = nullptr;
    g_Player_HorizonNodeFollowCameraEnabled = 0;
    g_Player_RuntimeInputFlags = 0;
    g_Player_LocalControlEnabled = 0;
    g_Player_NextOrdinal = 1;
    g_Time_AccumulatedTimeSec = 9.5f;
    g_Player_TotalTimeSecScaled = 9.5f;
    g_Player_CameraZone = 0.5f;
    g_Player_CameraZoneInvRange = 2.0f;
    g_Player_NominalGravity = 0.0f;
    g_PlayerStatusMeterRatio = 0.0f;
    g_HudSensorTracker.worldNode = &worldNode;
    g_HudSensorTracker.missionId = 2;
    g_HudSensorTracker.raceCheckpointMode = 0;
    g_HudSensorTracker.missionStat1 = 0;
    std::memset(g_PlayerTestHudVisibleThis, 0, sizeof(g_PlayerTestHudVisibleThis));
    std::memset(g_PlayerTestHudVisibleValue, 0, sizeof(g_PlayerTestHudVisibleValue));
    g_PlayerTestHudVisibleCount = 0;

    g_zEffect_World = nullptr;
    g_zEffect_ResourceNode = nullptr;
    g_zEffectAnim_EntryList = nullptr;
    g_zEffectAnim_EntryCount = 0;
    g_zEffectAnim_EntriesInstantiated = 0;
    g_zEffectAnim_ZbdFilename[0] = '\0';
    g_OptCatalogThermalGlowFreeList = nullptr;
    g_Player_HudCounterValue = 123;
    g_OptCatalog_DamageFeedbackHitCount = 456;
    zModel_MaterialSlot materialSlots[8] = {};
    for (int i = 0; i < 8; ++i) {
        materialSlots[i].prevPoolIndex = static_cast<short>(i - 1);
        materialSlots[i].nextPoolIndex = static_cast<short>(i + 1);
    }
    materialSlots[0].prevPoolIndex = -1;
    materialSlots[7].nextPoolIndex = -1;
    g_zModel_MatlPool = materialSlots;
    g_zModel_MatlPoolCapacity = 8;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlFreeHeadIndex = 0;
    g_zModel_MatlActiveHeadIndex = -1;
    g_zModel_MatlReuseCache = nullptr;
    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    std::strcpy(g_zImage_TexDirEntries[0].baseName, "crater_base.tga");
    g_zImage_TexDirEntries[0].loadState = 1;
    g_zImage_TexDirEntryCount = 1;
    g_zDEClient_RebuildBltRectOnReload = 0;
    zVidTexturePackEntry texturePacks[2] = {};
    texturePacks[0].fileHandle = stdout;
    texturePacks[1].fileHandle = stdout;
    g_zVideo_ActiveRendererPath = 0;
    g_zVid_BuiltinTexturePackCount = 1;
    g_zVid_BuiltinTexturePacks = &texturePacks[0];
    g_zVid_TexturePackCount = 1;
    g_zVid_TexturePacks = &texturePacks[1];
    zSndSample samples[1] = {};
    samples[0].replayFields.sampleId = "snd_incoming";
    samples[0].primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(0x1000);
    zSndSampleSet sampleSet = {};
    sampleSet.sampleCount = 1;
    sampleSet.samples = samples;
    zSndSampleSet *sampleSetSlots[1] = {&sampleSet};
    g_zSnd_SampleSetRegistry.begin = sampleSetSlots;
    g_zSnd_SampleSetRegistry.end = sampleSetSlots + 1;
    g_zSnd_SampleSetRegistry.capacityEnd = sampleSetSlots + 1;
    g_zSnd_IsInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    HudUiLoadingCheckpoint::InitTable();
    g_Briefing_Runtime = nullptr;

    HudSensorTracker tracker = {};
    tracker.missionId = 2;
    tracker.worldNode = &worldNode;
    tracker.cameraNode = &cameraNode;
    tracker.effectResourceNode = &resourceNode;
    tracker.missionStat0 = 1;
    tracker.missionStat1 = 2;
    tracker.primaryGunDispatchCount = 3;
    tracker.missionStat3 = 4;
    tracker.weaponsFoundMask = 5;
    tracker.menuTransitionDelaySec = 2.0f;

    const int result = tracker.InitMissionGameplaySystems();
    const zInputCommandCallbackFn objectiveCallback =
        (zInputCommandCallbackFn)(HudSensorTracker::OnObjectiveCommand);
    const bool callbacksOk =
        context.m_commandCallbacks[24] == objectiveCallback &&
        context.m_commandCallbacks[25] == objectiveCallback &&
        context.m_commandCallbacks[26] == objectiveCallback &&
        context.m_commandCallbacks[27] == objectiveCallback &&
        context.m_commandCallbacks[28] == objectiveCallback &&
        context.m_commandCallbacks[29] == objectiveCallback;
    int verifyResult = 0;
    if (result != 1) {
        verifyResult = 10;
    } else if (tracker.missionStat0 != 0 || tracker.missionStat1 != 0 ||
               tracker.primaryGunDispatchCount != 0 || tracker.missionStat3 != 0 ||
               tracker.weaponsFoundMask != 0) {
        verifyResult = 11;
    } else if (g_Player_HudCounterValue != 0 || g_OptCatalog_DamageFeedbackHitCount != 0 ||
               !FloatNear(tracker.menuTransitionDelaySec, -1.0f)) {
        verifyResult = 12;
    } else if (tracker.mapWorldNode != &worldNode) {
        verifyResult = 13;
    } else if (g_zEffect_World != &worldNode) {
        verifyResult = 17;
    } else if (g_zEffect_ResourceNode != &resourceNode) {
        verifyResult = 18;
    } else if (!callbacksOk) {
        verifyResult = 19;
    } else if (g_Player_RuntimeDiScene != &worldNode || g_MainCamera != &cameraNode) {
        verifyResult = 14;
    } else if (shield.viewportResetFrame != -1 || shield.state != 0 ||
               g_HudUiMgrSensorBlock.state != 1) {
        verifyResult = 15;
    }

    Player::ShutdownMissionRuntime();
    zClass_NodePartial *lightNode = g_OptCatalogThermalGlowFreeList;
    while (lightNode != nullptr) {
        zClass_NodePartial *const next = lightNode->callbackContext;
        std::free(lightNode->classData);
        lightNode->classData = nullptr;
        lightNode = next;
    }
    for (int i = 0; i < 32; ++i) {
        std::free(slots[i].node.classData);
        slots[i].node.classData = nullptr;
    }
    context.FreeNonOwnedBuffers();
    if (archive.hFile != INVALID_HANDLE_VALUE && archive.hFile != nullptr) {
        CloseHandle(archive.hFile);
    }
    DeleteFileA(tempFile);

    g_PlayerSaveStateListHead = oldSaveHead;
    g_PlayerSaveStateListTail = oldSaveTail;
    g_PlayerSaveStateListAux = oldSaveAux;
    g_PlayerSaveStateCount = oldSaveCount;
    g_Player2SaveState = oldPlayer2SaveState;
    g_LocalPlayerSaveState = oldLocalSaveState;
    g_CurrentPlayerSaveState = oldCurrentSaveState;
    g_GameStateOrMapTable = oldGameState;
    g_PlayerMasterCommonDataHead = oldCommonHead;
    g_PlayerMasterCommonDataTail = oldCommonTail;
    g_PlayerMasterCommonDataListAux = oldCommonAux;
    g_PlayerMasterCommonDataCount = oldCommonCount;
    g_PlayerMasterModalDataHead = oldModalHead;
    g_PlayerMasterModalDataTail = oldModalTail;
    g_PlayerMasterModalDataListAux = oldModalAux;
    g_PlayerMasterModalDataCount = oldModalCount;
    g_Player_MissionInitFirstRunFlag = oldMissionInitFirstRun;
    g_HudUiTopMessageStack = oldTopStack;
    g_HudUiChatMessageStack = oldChatStack;
    g_Player_TopMsgPanel1 = oldTopPanel1;
    g_Player_TopMsgPanel2 = oldTopPanel2;
    g_Player_UnderwaterFxPass3Ui = oldUnderwaterFxPass3Ui;
    g_Player_State7FxPass3Ui = oldState7FxPass3Ui;
    *fxContainer = oldFxContainer;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_MainCamera = oldMainCamera;
    g_Player_HorizonNode = oldHorizonNode;
    g_Player_HorizonNodeFollowCameraEnabled = oldHorizonEnabled;
    g_Player_RuntimeInputFlags = oldRuntimeInputFlags;
    g_Player_LocalControlEnabled = oldLocalControlEnabled;
    g_Player_NextOrdinal = oldNextOrdinal;
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    g_Player_TotalTimeSecScaled = oldTotalTime;
    g_Player_CameraZone = oldCameraZone;
    g_Player_CameraZoneInvRange = oldCameraZoneInvRange;
    g_Player_NominalGravity = oldNominalGravity;
    g_PlayerStatusMeterRatio = oldStatusMeterRatio;
    g_zClass_NodeArray = oldNodeArray;
    g_zClass_NodeFreeHeadIndex = oldFreeHead;
    g_zClass_ActiveNodeCount = oldActiveNodeCount;
    g_zClass_TypeList_FreeLinkHead = oldFreeLinkHead;
    g_zClass_NodeList_PendingFreeHead = oldPendingFreeHead;
    g_zClass_DeferredProcessingEnabled = oldDeferredProcessing;
    g_zClass_TypeList_LiveLinkCount = oldLiveLinkCount;
    g_zClass_TypeList_PeakLiveLinkCount = oldPeakLiveLinkCount;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    ZOPT_GAME_CONTROL_OPTIONS = oldGameControlOptions;
    g_zOpt_GameDifficultyOption = oldDifficultyOption;
    ZOPT_REPLICATE = oldReplicate;
    g_zArchive_MountedList = oldMountedList;
    g_zInput_BindMap_Current = oldBindMap;
    g_zOpt_DisplaySectionOption = oldDisplayOption;
    g_zOpt_WindowSectionOption = oldWindowOption;
    g_HudUiMgrShieldMessageWidget = oldShieldWidget;
    g_HudUiMgrCurrentLayout = oldCurrentLayout;
    g_HudUiMgrHudRect = oldHudRect;
    g_HudUiMgrViewRect = oldViewRect;
    g_HudUiMgrSensorBlock = oldSensorBlock;
    g_HudUiMgrSensorFxRect = oldSensorFxRect;
    g_HudUiMgrSensorFxViewportWidth = oldSensorFxViewportWidth;
    g_HudUiMgrSensorFxViewportHeight = oldSensorFxViewportHeight;
    g_HudUiMgrNanitePanel = oldNanitePanel;
    std::memcpy(g_HudUiMgrMessages, oldMessages, sizeof(oldMessages));
    std::memcpy(g_HudUiMgrModeCounters, oldModeCounters, sizeof(oldModeCounters));
    g_HudUiMgrActiveWeaponMessageIndex = oldActiveWeaponMessageIndex;
    g_HudUiMgrActiveWeaponSideIndex = oldActiveWeaponSideIndex;
    g_HudUiMgrActiveModeCounterIndex = oldActiveModeCounterIndex;
    g_HudUiMgrObjectiveWidget = oldObjectiveWidget;
    g_HudUiMgrObjectivePhase = oldObjectivePhase;
    g_zEffect_World = oldEffectWorld;
    g_zEffect_ResourceNode = oldEffectResource;
    g_zEffectAnim_EntryList = oldEffectEntries;
    g_zEffectAnim_EntryCount = oldEffectCount;
    g_zEffectAnim_EntriesInstantiated = oldEffectInstantiated;
    std::memcpy(g_zEffectAnim_ZbdFilename, oldEffectZbdFilename,
                sizeof(g_zEffectAnim_ZbdFilename));
    g_OptCatalogThermalGlowFreeList = oldThermalGlowFreeList;
    g_Player_HudCounterValue = oldHudCounterValue;
    g_OptCatalog_DamageFeedbackHitCount = oldDamageFeedbackHitCount;
    g_HudSensorTracker.worldNode = oldHudWorldNode;
    g_HudSensorTracker.missionId = oldHudMissionId;
    g_HudSensorTracker.raceCheckpointMode = oldHudRaceCheckpointMode;
    g_HudSensorTracker.missionStat1 = oldHudMissionStat1;
    g_zModel_MatlPool = oldMatlPool;
    g_zModel_MatlPoolCapacity = oldMatlPoolCapacity;
    g_zModel_MatlPoolInUseCount = oldMatlPoolInUseCount;
    g_zModel_MatlFreeHeadIndex = oldMatlFreeHeadIndex;
    g_zModel_MatlActiveHeadIndex = oldMatlActiveHeadIndex;
    g_zModel_MatlReuseCache = oldMatlReuseCache;
    std::memcpy(g_zImage_TexDirEntries, oldTexDirEntries, sizeof(oldTexDirEntries));
    g_zImage_TexDirEntryCount = oldTexDirEntryCount;
    g_zDEClient_RebuildBltRectOnReload = oldZdeclientRebuildBltRect;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVid_BuiltinTexturePackCount = oldBuiltinTexturePackCount;
    g_zVid_BuiltinTexturePacks = oldBuiltinTexturePacks;
    g_zVid_TexturePackCount = oldTexturePackCount;
    g_zVid_TexturePacks = oldTexturePacks;
    g_zSnd_SampleSetRegistry = oldSndRegistry;
    g_zSnd_IsInitialized = oldSndInitialized;
    g_zSnd_ActiveBackend = oldActiveBackend;
    g_HudUiLoadingCheckpointMaxIndex = oldLoadingMaxIndex;
    g_HudUiLoadingCheckpointCurrentIndex = oldLoadingCurrentIndex;
    g_HudUiLoadingCheckpointCurrentProgress = oldLoadingCurrentProgress;
    g_Briefing_Runtime = oldBriefingRuntime;
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = oldTypeHeads[i];
        zClass_TypeList::Tail(i) = oldTypeTails[i];
        zClass_TypeList::PendingRemovalDirty(i) = oldTypeDirty[i];
    }

    return verifyResult;
}

extern "C" int player_cache_gun_hardpoints_and_detach_displays_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    zClass_Object3DDataPartial gunData = {};
    zClass_Object3DDataPartial centerData = {};
    zClass_Object3DDataPartial leftData = {};
    zClass_Object3DDataPartial rightData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial gunNode = {};
    zClass_NodePartial centerNode = {};
    zClass_NodePartial leftNode = {};
    zClass_NodePartial rightNode = {};
    zClass_NodePartial *rootChildren[] = {&gunNode};
    zClass_NodePartial *gunChildren[] = {&centerNode, &leftNode, &rightNode};

    std::strcpy(rootNode.name, "root");
    std::strcpy(gunNode.name, "gun");
    std::strcpy(centerNode.name, "fpnt_c");
    std::strcpy(leftNode.name, "fpnt_l");
    std::strcpy(rightNode.name, "fpnt_r");

    rootNode.listCountB = 1;
    rootNode.listB = rootChildren;
    gunNode.listCountB = 3;
    gunNode.listB = gunChildren;

    gunNode.classId = 5;
    centerNode.classId = 5;
    leftNode.classId = 5;
    rightNode.classId = 5;
    gunNode.classData = &gunData;
    centerNode.classData = &centerData;
    leftNode.classData = &leftData;
    rightNode.classData = &rightData;

    const zMat4x3 gunMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                               0.0f, 0.0f, 1.0f, 11.0f, 12.0f, 13.0f};
    const zMat4x3 centerMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                  0.0f, 0.0f, 1.0f, 21.0f, 22.0f, 23.0f};
    const zMat4x3 leftMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                0.0f, 0.0f, 1.0f, 31.0f, 32.0f, 33.0f};
    const zMat4x3 rightMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                 0.0f, 0.0f, 1.0f, 41.0f, 42.0f, 43.0f};
    SetObjectLocalMatrix(&gunData, gunMatrix);
    SetObjectLocalMatrix(&centerData, centerMatrix);
    SetObjectLocalMatrix(&leftData, leftMatrix);
    SetObjectLocalMatrix(&rightData, rightMatrix);

    playerState.rootNode = &rootNode;
    centerNode.userDataOrDiRef = 77;
    leftNode.userDataOrDiRef = 88;
    rightNode.userDataOrDiRef = 99;

    Player::CacheGunHardpointsAndDetachDisplays(&saveState, 0);
    if (playerState.gunNode != &gunNode ||
        !Vec3Equals(playerState.gunNodeMatrixPos, {11.0f, 12.0f, 13.0f}) ||
        !Vec3Equals(playerState.firePointCenter, {21.0f, 22.0f, 23.0f}) ||
        !Vec3Equals(playerState.firePointLeft, {31.0f, 32.0f, 33.0f}) ||
        !Vec3Equals(playerState.firePointRight, {41.0f, 42.0f, 43.0f}) ||
        centerNode.userDataOrDiRef != 77 || leftNode.userDataOrDiRef != 88 ||
        rightNode.userDataOrDiRef != 99) {
        return 1;
    }

    centerNode.userDataOrDiRef = 0;
    leftNode.userDataOrDiRef = 0;
    rightNode.userDataOrDiRef = 0;
    centerNode.flags = 1;
    leftNode.flags = 1;
    rightNode.flags = 1;

    Player::CacheGunHardpointsAndDetachDisplays(&saveState, 1);
    if (centerNode.userDataOrDiRef != 0 || leftNode.userDataOrDiRef != 0 ||
        rightNode.userDataOrDiRef != 0 || (centerNode.flags & 0x200) != 0 ||
        (leftNode.flags & 0x200) != 0 || (rightNode.flags & 0x200) != 0) {
        return 2;
    }

    zClass_NodePartial emptyRoot = {};
    std::strcpy(emptyRoot.name, "root");
    playerState.rootNode = &emptyRoot;
    playerState.gunNode = &gunNode;
    Player::CacheGunHardpointsAndDetachDisplays(&saveState, 0);
    return playerState.gunNode == nullptr ? 0 : 3;
}

extern "C" int player_init_state_from_name_and_master_common_data_smoke(void) {
    zUtil_SaveGameState *const oldSaveHead = g_PlayerSaveStateListHead;
    zUtil_SaveGameState *const oldSaveTail = g_PlayerSaveStateListTail;
    const int oldSaveCount = g_PlayerSaveStateCount;
    PlayerMasterCommonData *const oldCommonHead = g_PlayerMasterCommonDataHead;
    PlayerMasterCommonData *const oldCommonTail = g_PlayerMasterCommonDataTail;
    const int oldCommonAux = g_PlayerMasterCommonDataListAux;
    const int oldCommonCount = g_PlayerMasterCommonDataCount;
    const int oldNextOrdinal = g_Player_NextOrdinal;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;
    int *const oldGameControlOptions = ZOPT_GAME_CONTROL_OPTIONS;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    zZbdManager *const oldZbdManager = g_zUtil_ZbdManager;
    AINet *const oldAiHead = g_AINetListHead;
    AINet *const oldAiTail = g_AINetListTail;
    zEffectAnimEntry *const oldEffectEntries = g_zEffectAnim_EntryList;
    const short oldEffectCount = g_zEffectAnim_EntryCount;
    int gameControlOptions = 0;
    int networkEnabled = 0;

    PlayerMasterCommonData commonData = {};
    std::strcpy(commonData.vehicleName, "tank_common");
    std::strcpy(commonData.startAnimsName, "startup");
    commonData.cambackSide0 = 1.0f;
    commonData.cambackBase0 = 2.0f;
    commonData.cambackDist0 = 3.0f;
    commonData.cambackSide1 = 4.0f;
    commonData.cambackBase1 = 5.0f;
    commonData.cambackDist1 = 6.0f;
    commonData.cambackSide2 = 7.0f;
    commonData.cambackBase2 = 8.0f;
    commonData.cambackDist2 = 9.0f;
    commonData.aimYawRate = 10.0f;
    commonData.aimYawMax = 11.0f;
    commonData.maxHealth = 250.0f;

    zClass_Object3DDataPartial rootData = {};
    zClass_Object3DDataPartial targetData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial targetNode = {};
    zClass_NodePartial bodyNode = {};
    zClass_NodePartial turretNode = {};
    zClass_NodePartial doorLeftNode = {};
    zClass_NodePartial doorRightNode = {};
    zClass_NodePartial shadowNode = {};
    zClass_NodePartial *rootChildren[] = {&targetNode, &bodyNode, &turretNode,
                                          &doorLeftNode, &doorRightNode, &shadowNode};

    std::strcpy(rootNode.name, "tank");
    std::strcpy(targetNode.name, "target");
    std::strcpy(bodyNode.name, "body");
    std::strcpy(turretNode.name, "turret");
    std::strcpy(doorLeftNode.name, "doorleft");
    std::strcpy(doorRightNode.name, "doorright");
    std::strcpy(shadowNode.name, "shadow");
    rootNode.classId = 5;
    targetNode.classId = 5;
    rootNode.classData = &rootData;
    targetNode.classData = &targetData;
    rootNode.listCountB = 6;
    rootNode.listB = rootChildren;
    targetNode.flags = 0x04;
    const zMat4x3 rootMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                0.0f, 0.0f, 1.0f, 10.0f, 20.0f, 30.0f};
    const zMat4x3 targetMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                  0.0f, 0.0f, 1.0f, 2.0f, 3.0f, 4.0f};
    SetObjectLocalMatrix(&rootData, rootMatrix);
    SetObjectLocalMatrix(&targetData, targetMatrix);

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;
    playerState.rootNode = &rootNode;

    g_PlayerSaveStateListHead = &saveState;
    g_PlayerSaveStateListTail = &saveState;
    g_PlayerSaveStateCount = 1;
    g_PlayerMasterCommonDataHead = &commonData;
    g_PlayerMasterCommonDataTail = &commonData;
    g_PlayerMasterCommonDataListAux = 0;
    g_PlayerMasterCommonDataCount = 1;
    g_Player_NextOrdinal = 1;
    g_GameStateOrMapTable = 0;
    g_Time_AccumulatedTimeSec = 42.0f;
    ZOPT_GAME_CONTROL_OPTIONS = &gameControlOptions;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_zUtil_ZbdManager = 0;
    g_AINetListHead = 0;
    g_AINetListTail = 0;
    g_zEffectAnim_EntryList = 0;
    g_zEffectAnim_EntryCount = 0;

    Player::InitStateFromNameAndMasterCommonData(&saveState, "tank",
                                                 "tank_common");

    int result = 0;
    if (playerState.masterCommonData != &commonData || playerState.playerOrdinal != 1 ||
        g_Player_NextOrdinal != 2 ||
        g_GameStateOrMapTable != (zInput_GameStateOrMapTablePartial *)(&saveState)) {
        result = 1;
    } else if (!Vec3Equals(playerState.worldPos, zVec3_Make(10.0f, 20.0f, 30.0f)) ||
               playerState.pitchPoseCache != 0.0f || playerState.yawPoseCache != 0.0f ||
               playerState.rollPoseCache != 0.0f ||
               !Vec3Equals(playerState.steerBasisRaw, zVec3_Make(0.0f, 0.0f, -1.0f)) ||
               !Vec3Equals(playerState.steerBasisNorm, zVec3_Make(0.0f, 0.0f, -1.0f)) ||
               !Vec3Equals(playerState.cameraDirFlat, zVec3_Make(0.0f, 0.0f, -1.0f))) {
        result = 2;
    } else if (playerState.lifecycleState != 4 ||
               playerState.thirdPersonSideOffset != 1.0f ||
               playerState.thirdPersonBaseYOffset != 2.0f ||
               playerState.cameraDistance != 3.0f ||
               playerState.cameraConfigParam0 != 4.0f ||
               playerState.cameraConfigParam5 != 9.0f ||
               playerState.cameraYOffset != 11.0f ||
               !Vec3Equals(playerState.cameraState2TargetOffset,
                           zVec3_Make(0.0f, 150.0f, 0.0f)) ||
               !Vec3Equals(playerState.altGunAimOrigin,
                           zVec3_Make(0.0f, 0.0f, -1.0f)) ||
               playerState.activeAltBankIndex != 1 || playerState.autoTurnActive != 0 ||
               playerState.cameraTransitionTimer != 0 ||
               playerState.cameraTransitionBlend != 1.0f) {
        result = 3;
    } else if (!Vec3Equals(playerState.fxOffsetLocal, zVec3_Make(2.0f, 3.0f, 4.0f)) ||
               !Vec3Equals(playerState.fxOffsetWorld, zVec3_Make(12.0f, 23.0f, 34.0f)) ||
               (targetNode.flags & 0x04) != 0 || playerState.bodyNode != &bodyNode ||
               playerState.turretNode != &turretNode ||
               playerState.doorLeftNode != &doorLeftNode ||
               playerState.doorRightNode != &doorRightNode ||
               playerState.modeVariantNode != &shadowNode || playerState.gunNode != 0 ||
               playerState.statusMeterValue != 250.0f ||
               playerState.statusMeterScaled != 1.0f ||
               playerState.damageProtectionActive != 0 ||
               playerState.queuedFixedDamageFlag != 0 ||
               playerState.recentHitValid != 0 ||
               playerState.recentHitLightHandle != 0 ||
               playerState.nanitePanelLevel != 0) {
        result = 4;
    }

    AINetNode aiNode = {};
    aiNode.position = zVec3_Make(9.0f, 20.0f, 31.0f);
    AINet aiNet = {};
    aiNet.netId = 77;
    aiNet.aiType = AINET_TYPE_FI;
    aiNet.activateRadius = 5.0f;
    aiNet.attackRadius = 6.0f;
    aiNet.attackDwell = 7.0f;
    aiNet.notPursuitDwell = 8.0f;
    aiNet.returnRange = 9.0f;
    aiNet.hideTime0 = 10.0f;
    aiNet.hideTime1 = 11.0f;
    aiNet.attackStrategy = AINET_STRAT_ZIG;
    aiNet.nodeListHead = &aiNode;

    zClass_NodePartial healthyNode = {};
    std::strcpy(healthyNode.name, "healthy");
    healthyNode.flags = 0x08;
    zClass_NodePartial *aiRootChildren[] = {&healthyNode};
    rootNode.listCountB = 1;
    rootNode.listB = aiRootChildren;
    targetNode.flags = 0x04;
    playerState = {};
    saveState = {};
    saveState.playerState = &playerState;
    playerState.rootNode = &rootNode;
    playerState.aiNetId = 77;
    g_PlayerSaveStateListHead = &saveState;
    g_PlayerSaveStateListTail = &saveState;
    g_Player_NextOrdinal = 2;
    g_GameStateOrMapTable = 0;
    g_Time_AccumulatedTimeSec = 42.0f;
    ZOPT_GAME_CONTROL_OPTIONS = &gameControlOptions;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_zUtil_ZbdManager = 0;
    g_AINetListHead = &aiNet;
    g_AINetListTail = &aiNet;

    if (result == 0) {
        Player::InitStateFromNameAndMasterCommonData(&saveState, "net_tank",
                                                     "tank_common");

        if (playerState.lifecycleState != 2 || playerState.aiNet != &aiNet ||
            playerState.aiTopLevelState != 3 ||
            playerState.aiCurrentSteeringSubstate != AINET_STRAT_ZIG ||
            playerState.aiHideTime0 != 10.0f || playerState.aiHideTime1 != 11.0f ||
            playerState.aiCurrentPathNode != &aiNode ||
            playerState.aiHomePathNode != &aiNode ||
            playerState.aiActivationRadiusSq != 25.0f ||
            playerState.aiAttackRadiusSq != 36.0f ||
            playerState.aiMode2AttackDwell != 7.0f ||
            playerState.aiNotPursuitDwell != 8.0f ||
            playerState.aiRestoreDistanceSq != 81.0f ||
            saveState.aiPeerRingNext != &saveState ||
            playerState.aiStateUntilTime != 52.0f ||
            playerState.aiStateStartTime != 52.0f ||
            (healthyNode.flags & 0x08) != 0) {
            result = 5;
        }
    }

    g_PlayerSaveStateListHead = oldSaveHead;
    g_PlayerSaveStateListTail = oldSaveTail;
    g_PlayerSaveStateCount = oldSaveCount;
    g_PlayerMasterCommonDataHead = oldCommonHead;
    g_PlayerMasterCommonDataTail = oldCommonTail;
    g_PlayerMasterCommonDataListAux = oldCommonAux;
    g_PlayerMasterCommonDataCount = oldCommonCount;
    g_Player_NextOrdinal = oldNextOrdinal;
    g_GameStateOrMapTable = oldGameState;
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    ZOPT_GAME_CONTROL_OPTIONS = oldGameControlOptions;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_zUtil_ZbdManager = oldZbdManager;
    g_AINetListHead = oldAiHead;
    g_AINetListTail = oldAiTail;
    g_zEffectAnim_EntryList = oldEffectEntries;
    g_zEffectAnim_EntryCount = oldEffectCount;
    return result;
}

extern "C" int player_build_ai_peer_rings_by_ai_net_id_smoke(void) {
    zUtil_SaveGameState *const oldHead = g_PlayerSaveStateListHead;
    zUtil_SaveGameState *const oldTail = g_PlayerSaveStateListTail;
    const int oldCount = g_PlayerSaveStateCount;

    zUtil_SaveGameState saves[5] = {};
    zUtil_PlayerStateStorage playerStates[5] = {};
    for (int i = 0; i < 5; ++i) {
        saves[i].playerState = &playerStates[i];
        saves[i].aiPeerRingNext = &saves[i];
        if (i != 4) {
            saves[i].next = &saves[i + 1];
        }
    }

    playerStates[0].aiNetId = 7;
    playerStates[0].lifecycleState = 2;
    playerStates[1].aiNetId = 7;
    playerStates[1].lifecycleState = 2;
    playerStates[2].aiNetId = 9;
    playerStates[2].lifecycleState = 2;
    playerStates[3].aiNetId = 7;
    playerStates[3].lifecycleState = 4;
    playerStates[4].aiNetId = 7;
    playerStates[4].lifecycleState = 2;

    g_PlayerSaveStateListHead = &saves[0];
    g_PlayerSaveStateListTail = &saves[4];
    g_PlayerSaveStateCount = 5;

    Player::BuildAiPeerRingsByAiNetId();

    const bool firstPassOk =
        saves[0].aiPeerRingNext == &saves[4] && saves[4].aiPeerRingNext == &saves[1] &&
        saves[1].aiPeerRingNext == &saves[0] && saves[2].aiPeerRingNext == &saves[2] &&
        saves[3].aiPeerRingNext == &saves[3];

    Player::BuildAiPeerRingsByAiNetId();

    const bool stableOk =
        saves[0].aiPeerRingNext == &saves[4] && saves[4].aiPeerRingNext == &saves[1] &&
        saves[1].aiPeerRingNext == &saves[0] && saves[2].aiPeerRingNext == &saves[2] &&
        saves[3].aiPeerRingNext == &saves[3];

    g_PlayerSaveStateListHead = oldHead;
    g_PlayerSaveStateListTail = oldTail;
    g_PlayerSaveStateCount = oldCount;
    return firstPassOk && stableOk ? 0 : 1;
}

extern "C" int player_build_support_points_from_model_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterModalData modalData = {};
    PlayerModalState primaryModalState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &primaryModalState;
    primaryModalState.masterModalData = &modalData;

    zClass_NodePartial modelNode = {};
    zClass_NodePartial supportNodes[4] = {};
    zClass_Object3DDataPartial supportData[4] = {};
    zClass_NodePartial *children[4] = {
        &supportNodes[0], &supportNodes[1], &supportNodes[2], &supportNodes[3]};
    modelNode.listCountB = 4;
    modelNode.listB = children;
    std::strcpy(modelNode.name, "vehicle");

    for (int i = 0; i < 4; ++i) {
        std::sprintf(supportNodes[i].name, "support%02d", i);
        supportNodes[i].classId = 5;
        supportNodes[i].classData = &supportData[i];
        supportNodes[i].flags = 0x04;
        const float base = static_cast<float>(i * 10);
        const zMat4x3 matrix = {1.0f,      0.0f,      0.0f, 0.0f, 1.0f, 0.0f,
                                0.0f,      0.0f,      1.0f, base + 1.0f,
                                base + 2.0f, base + 3.0f};
        SetObjectLocalMatrix(&supportData[i], matrix);
    }

    const int ok = Player::BuildSupportPointsFromModel(&saveState, &modelNode);
    if (ok != 1) {
        return 1;
    }

    for (int i = 0; i < 4; ++i) {
        const float base = static_cast<float>(i * 10);
        if (!Vec3Equals(modalData.probePoints[15 + i],
                        zVec3_Make(base + 1.0f, base + 2.0f, base + 3.0f)) ||
            (supportNodes[i].flags & 0x04) != 0) {
            return 2;
        }
    }

    modalData.probePoints[17] = zVec3_Make(90.0f, 91.0f, 92.0f);
    modelNode.listCountB = 2;
    supportNodes[0].flags = 0x04;
    supportNodes[1].flags = 0x04;
    const int missing = Player::BuildSupportPointsFromModel(&saveState, &modelNode);
    if (missing != 0 || (supportNodes[0].flags & 0x04) != 0 ||
        (supportNodes[1].flags & 0x04) != 0 ||
        !Vec3Equals(modalData.probePoints[17], zVec3_Make(90.0f, 91.0f, 92.0f))) {
        return 3;
    }

    return 0;
}

extern "C" int player_build_collision_points_from_model_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterModalData modalData = {};
    PlayerModalState primaryModalState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &primaryModalState;
    primaryModalState.masterModalData = &modalData;

    zClass_NodePartial modelNode = {};
    zClass_NodePartial collisionNodes[12] = {};
    zClass_Object3DDataPartial collisionData[12] = {};
    zClass_NodePartial *children[12] = {};
    modelNode.listCountB = 12;
    modelNode.listB = children;
    std::strcpy(modelNode.name, "vehicle");

    for (int i = 0; i < 12; ++i) {
        children[i] = &collisionNodes[i];
        std::sprintf(collisionNodes[i].name, "collide%02d", i);
        collisionNodes[i].classId = 5;
        collisionNodes[i].classData = &collisionData[i];
        collisionNodes[i].flags = 0x04;
        const float base = static_cast<float>(i * 10);
        const zMat4x3 matrix = {1.0f,      0.0f,      0.0f, 0.0f, 1.0f, 0.0f,
                                0.0f,      0.0f,      1.0f, base + 1.0f,
                                base + 2.0f, base + 3.0f};
        SetObjectLocalMatrix(&collisionData[i], matrix);
    }

    const int ok = Player::BuildCollisionPointsFromModel(&saveState, &modelNode);
    if (ok != 1 || modalData.probePointCount != 12) {
        return 1;
    }

    const int expectedOrder[12] = {0, 1, 2, 6, 7, 8, 3, 4, 5, 9, 10, 11};
    for (int i = 0; i < 12; ++i) {
        const float base = static_cast<float>(expectedOrder[i] * 10);
        if (!Vec3Equals(modalData.probePoints[i],
                        zVec3_Make(base + 1.0f, base + 2.0f, base + 3.0f)) ||
            (collisionNodes[expectedOrder[i]].flags & 0x04) != 0) {
            return 2;
        }
    }

    modalData.probePoints[0] = zVec3_Make(90.0f, 91.0f, 92.0f);
    modalData.probePointCount = 99;
    modelNode.listCountB = 2;
    collisionNodes[0].flags = 0x04;
    collisionNodes[1].flags = 0x04;
    const int missing = Player::BuildCollisionPointsFromModel(&saveState, &modelNode);
    if (missing != 0 || (collisionNodes[0].flags & 0x04) != 0 ||
        (collisionNodes[1].flags & 0x04) != 0 || modalData.probePointCount != 99 ||
        !Vec3Equals(modalData.probePoints[0], zVec3_Make(90.0f, 91.0f, 92.0f))) {
        return 3;
    }

    return 0;
}

extern "C" int player_bind_modal_state_from_master_modal_data_smoke(void) {
    zUtil_SaveGameState *const oldSaveHead = g_PlayerSaveStateListHead;
    zUtil_SaveGameState *const oldSaveTail = g_PlayerSaveStateListTail;
    const int oldSaveAux = g_PlayerSaveStateListAux;
    const int oldSaveCount = g_PlayerSaveStateCount;
    PlayerMasterModalData *const oldModalHead = g_PlayerMasterModalDataHead;
    PlayerMasterModalData *const oldModalTail = g_PlayerMasterModalDataTail;
    const int oldModalAux = g_PlayerMasterModalDataListAux;
    const int oldModalCount = g_PlayerMasterModalDataCount;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData nonMatchingModalData = {};
    PlayerMasterModalData modalData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial cacheNodes[12] = {};
    zClass_NodePartial supportNodes[4] = {};
    zClass_Object3DDataPartial supportData[4] = {};
    zClass_NodePartial collisionNodes[12] = {};
    zClass_Object3DDataPartial collisionData[12] = {};
    zClass_NodePartial *children[28] = {};

    std::strcpy(commonData.vehicleName, "tank_common");
    std::strcpy(nonMatchingModalData.modalName, "tank_common");
    std::strcpy(nonMatchingModalData.modeName, "hover");
    nonMatchingModalData.next = &modalData;
    std::strcpy(modalData.modalName, "tank_common");
    std::strcpy(modalData.modeName, "track");

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    playerState.masterCommonData = &commonData;
    playerState.rootNode = &rootNode;
    std::strcpy(rootNode.name, "vehicle");
    rootNode.listCountB = 28;
    rootNode.listB = children;

    const char *const cacheNames[12] = {"right_morphs", "left_morphs", "chassis",
                                        "rtracks",      "ltracks",     "props",
                                        "caustic1",     "wake",        "splash_l",
                                        "splash_r",     "dust_l",      "dust_r"};
    for (int i = 0; i < 12; ++i) {
        std::strcpy(cacheNodes[i].name, cacheNames[i]);
        children[i] = &cacheNodes[i];
    }

    for (int i = 0; i < 4; ++i) {
        children[12 + i] = &supportNodes[i];
        std::sprintf(supportNodes[i].name, "support%02d", i);
        supportNodes[i].classId = 5;
        supportNodes[i].classData = &supportData[i];
        supportNodes[i].flags = 0x04;
        const float base = static_cast<float>(100 + i * 10);
        const zMat4x3 matrix = {1.0f,      0.0f,      0.0f, 0.0f, 1.0f, 0.0f,
                                0.0f,      0.0f,      1.0f, base + 1.0f,
                                base + 2.0f, base + 3.0f};
        SetObjectLocalMatrix(&supportData[i], matrix);
    }

    for (int i = 0; i < 12; ++i) {
        children[16 + i] = &collisionNodes[i];
        std::sprintf(collisionNodes[i].name, "collide%02d", i);
        collisionNodes[i].classId = 5;
        collisionNodes[i].classData = &collisionData[i];
        collisionNodes[i].flags = 0x04;
        const float base = static_cast<float>(i * 10);
        const zMat4x3 matrix = {1.0f,      0.0f,      0.0f, 0.0f, 1.0f, 0.0f,
                                0.0f,      0.0f,      1.0f, base + 1.0f,
                                base + 2.0f, base + 3.0f};
        SetObjectLocalMatrix(&collisionData[i], matrix);
    }

    g_PlayerSaveStateListHead = &saveState;
    g_PlayerSaveStateListTail = &saveState;
    g_PlayerSaveStateListAux = 1;
    g_PlayerSaveStateCount = 1;
    g_PlayerMasterModalDataHead = &nonMatchingModalData;
    g_PlayerMasterModalDataTail = &modalData;
    g_PlayerMasterModalDataListAux = 1;
    g_PlayerMasterModalDataCount = 2;

    Player::BindModalStateFromMasterModalData(&saveState, &modalState, "track",
                                              "tank_object");

    int result = 0;
    if (modalState.masterModalData != &modalData ||
        modalState.nodeRightMorphs != &cacheNodes[0] ||
        modalState.nodeLeftMorphs != &cacheNodes[1] ||
        modalState.modalNode != &cacheNodes[2] ||
        modalState.nodeRTracks != &cacheNodes[3] ||
        modalState.nodeLTracks != &cacheNodes[4] || modalState.nodeProps != &cacheNodes[5] ||
        modalState.nodeCaustic1 != &cacheNodes[6] ||
        modalState.nodeWake != &cacheNodes[7] ||
        modalState.nodeSplashL != &cacheNodes[8] ||
        modalState.nodeSplashR != &cacheNodes[9] ||
        modalState.nodeDustL != &cacheNodes[10] ||
        modalState.nodeDustR != &cacheNodes[11]) {
        result = 1;
    }

    if (result == 0 &&
        (modalState.modalStateCode != 4 || modalState.chassisPitchFilterState != 0.0f ||
         modalState.chassisRollFilterState != 0.0f)) {
        result = 2;
    }

    for (int i = 0; result == 0 && i < 4; ++i) {
        const float base = static_cast<float>(100 + i * 10);
        if (!Vec3Equals(modalData.probePoints[15 + i],
                        zVec3_Make(base + 1.0f, base + 2.0f, base + 3.0f)) ||
            (supportNodes[i].flags & 0x04) != 0) {
            result = 3;
        }
    }

    const int expectedOrder[12] = {0, 1, 2, 6, 7, 8, 3, 4, 5, 9, 10, 11};
    if (result == 0 && modalData.probePointCount != 12) {
        result = 4;
    }
    for (int i = 0; result == 0 && i < 12; ++i) {
        const float base = static_cast<float>(expectedOrder[i] * 10);
        if (!Vec3Equals(modalData.probePoints[i],
                        zVec3_Make(base + 1.0f, base + 2.0f, base + 3.0f)) ||
            (collisionNodes[expectedOrder[i]].flags & 0x04) != 0) {
            result = 5;
        }
    }

    g_PlayerSaveStateListHead = oldSaveHead;
    g_PlayerSaveStateListTail = oldSaveTail;
    g_PlayerSaveStateListAux = oldSaveAux;
    g_PlayerSaveStateCount = oldSaveCount;
    g_PlayerMasterModalDataHead = oldModalHead;
    g_PlayerMasterModalDataTail = oldModalTail;
    g_PlayerMasterModalDataListAux = oldModalAux;
    g_PlayerMasterModalDataCount = oldModalCount;
    return result;
}

extern "C" int player_sample_ground_and_align_root_to_surface_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;

    static std::int32_t matrixFlags[2];
    static float *matrixSlots[2];
    static zMat4x3 identityMatrix;
    identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixFlags[0] = 1;
    matrixSlots[0] = reinterpret_cast<float *>(&identityMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchA = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    zClass_Object3DDataPartial rootData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial emptyWorld = {};
    zClass_WorldDataPartial emptyWorldData = {};

    saveState.playerState = &playerState;
    playerState.rootNode = &rootNode;
    playerState.worldPos = {0.25f, 10.0f, 0.25f};
    playerState.restartYawRad = 0.25f;
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 7;
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    rootNode.flags = 0x08;
    rootNode.nodeType = 7;
    emptyWorld.classData = &emptyWorldData;
    g_Player_RuntimeDiScene = &emptyWorld;
    g_Variant_CurrentTag.count = 3;
    g_Variant_CurrentTag.tags[0] = 1;
    g_Variant_CurrentTag.tags[1] = 2;
    g_Variant_CurrentTag.tags[2] = 3;

    Player::SampleGroundAndAlignRootToSurface(&saveState, 1);
    if (rootNode.nodeType != 0xff || (rootNode.flags & 0x08) == 0 ||
        playerState.variantTag.count != 0 || playerState.variantTag.tags[0] != 0xff ||
        g_Variant_CurrentTag.count != 0 || g_Variant_CurrentTag.tags[0] != 0xff ||
        rootData.rotation.x != 0.0f || rootData.rotation.y != 0.0f ||
        rootData.rotation.z != 0.0f) {
        g_Player_RuntimeDiScene = oldRuntimeScene;
        g_Variant_CurrentTag = oldVariantCurrent;
        return 1;
    }

    zVec3 vertices[3] = {{0.0f, 12.0f, 0.0f}, {0.0f, 12.0f, 1.0f},
                         {1.0f, 12.0f, 0.0f}};
    std::int32_t indices[3] = {0, 1, 2};
    zModel_MaterialPartial material = {};
    zDiEntryPartial diEntry = {};
    diEntry.flagsAndIndexCount = 3;
    diEntry.vertexIndices = indices;
    diEntry.material = &material;
    diEntry.variantTagInitialized = 0;
    diEntry.variantTag = 0xff;
    zDiPartial di = {};
    di.entryCount = 1;
    di.vertCount = 3;
    di.entries = &diEntry;
    di.verts = vertices;

    zClass_Object3DDataPartial groundData = {};
    groundData.flags = 8;
    zClass_NodePartial groundNode = {};
    groundNode.flags = 0x11c;
    groundNode.nodeType = 0x37;
    groundNode.classId = 5;
    groundNode.classData = &groundData;
    groundNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&di);
    groundNode.cachedBounds[0] = -10.0f;
    groundNode.cachedBounds[1] = 0.0f;
    groundNode.cachedBounds[2] = -10.0f;
    groundNode.cachedBounds[3] = 10.0f;
    groundNode.cachedBounds[4] = 20.0f;
    groundNode.cachedBounds[5] = 10.0f;
    zClass_NodePartial *groundParents[1] = {};
    zClass_NodePartial *worldChildren[1] = {&groundNode};
    zClass_NodePartial *areaChildren[1] = {&groundNode};
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
    world.classId = 2;
    world.classData = &worldData;
    world.listCountB = 1;
    world.listB = worldChildren;
    groundParents[0] = &world;
    groundNode.listCountA = 1;
    groundNode.listA = groundParents;

    g_Player_RuntimeDiScene = &world;
    rootNode.nodeType = 7;
    rootNode.flags = 0x08;
    rootData.rotation = {9.0f, 8.0f, 7.0f};
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 7;
    playerState.steerBasisNorm = {0.0f, 0.0f, 1.0f};
    playerState.steerBasisRaw = {3.0f, 4.0f, 5.0f};
    playerState.restartYawRad = 0.0f;
    playerState.vehiclePitchRad = 6.0f;
    playerState.vehicleRollRad = 7.0f;

    Player::SampleGroundAndAlignRootToSurface(&saveState, 1);

    int hitFailure = 0;
    if (rootNode.nodeType == 0xff) {
        hitFailure = 12;
    } else if (rootNode.nodeType == 0) {
        hitFailure = 13;
    } else if (rootNode.nodeType != 0x37) {
        hitFailure = 2;
    } else if ((rootNode.flags & 0x08) == 0) {
        hitFailure = 3;
    } else if (playerState.variantTag.count != 0) {
        hitFailure = 4;
    } else if (!FloatNear(playerState.steerBasisRef.x, 0.0f)) {
        hitFailure = 5;
    } else if (!FloatNear(playerState.steerBasisRef.z, 0.0f)) {
        hitFailure = 6;
    } else if (!FloatNear(playerState.vehiclePitchRad, 0.0f)) {
        hitFailure = 7;
    } else if (!FloatNear(playerState.vehicleRollRad, 0.0f)) {
        hitFailure = 8;
    } else if (!FloatNear(rootData.rotation.x, 0.0f)) {
        hitFailure = 9;
    } else if (!FloatNear(rootData.rotation.y, 0.0f)) {
        hitFailure = 10;
    } else if (!FloatNear(rootData.rotation.z, 0.0f)) {
        hitFailure = 11;
    }

    rootData.rotation = {3.0f, 4.0f, 5.0f};
    rootNode.nodeType = 7;
    Player::SampleGroundAndAlignRootToSurface(&saveState, 0);
    const bool noRotationOk = rootNode.nodeType == 0x37 &&
                              rootData.rotation.x == 3.0f &&
                              rootData.rotation.y == 4.0f &&
                              rootData.rotation.z == 5.0f;

    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Variant_CurrentTag = oldVariantCurrent;
    if (hitFailure != 0) {
        return hitFailure;
    }
    return noRotationOk ? 0 : 3;
}

extern "C" int player_init_spawn_state_from_primary_modal_data_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const float oldNominalGravity = g_Player_NominalGravity;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    zClass_Object3DDataPartial rootData = {};
    zClass_NodePartial rootNode = {};
    zClass_WorldDataPartial worldData = {};
    zClass_NodePartial worldNode = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    playerState.rootNode = &rootNode;
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    rootNode.flags = 0x08;
    worldNode.classData = &worldData;
    g_Player_RuntimeDiScene = &worldNode;
    g_Player_NominalGravity = 19.5f;

    playerState.spawnStateInitialized = 7;
    playerState.primaryGunGateUntilTime = 8.0f;
    playerState.gravityAccel = 1.0f;
    playerState.primaryFireSlotIndex = 3;
    playerState.altFireSlotIndex = 4;
    playerState.worldPos = {10.0f, 20.0f, 30.0f};
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 9;
    modalData.probePointCount = 3;
    modalData.probePoints[0] = {1.0f, 2.0f, 3.0f};
    modalData.probePoints[1] = {-4.0f, 5.0f, -6.0f};
    modalData.probePoints[2] = {7.0f, -8.0f, 9.0f};

    Player::InitSpawnStateFromPrimaryModalData(&saveState);

    const bool ok = playerState.spawnStateInitialized == 0 &&
                    playerState.primaryGunGateUntilTime == 0.0f &&
                    playerState.gravityAccel == 19.5f &&
                    playerState.primaryFireSlotIndex == 0 &&
                    playerState.altFireSlotIndex == 0 &&
                    Vec3Equals(playerState.rootProbeWorldByIndex[0],
                               zVec3_Make(11.0f, 22.0f, 33.0f)) &&
                    Vec3Equals(playerState.rootProbeWorldByIndex[1],
                               zVec3_Make(6.0f, 25.0f, 24.0f)) &&
                    Vec3Equals(playerState.rootProbeWorldByIndex[2],
                               zVec3_Make(17.0f, 12.0f, 39.0f)) &&
                    rootNode.nodeType == 0xff && (rootNode.flags & 0x08) != 0 &&
                    playerState.variantTag.count == 0 &&
                    playerState.variantTag.tags[0] == 0xff;

    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_Player_NominalGravity = oldNominalGravity;
    return ok ? 0 : 1;
}

extern "C" int player_load_master_modal_data_from_node_smoke(void) {
    zEffectAnimEntry *const oldEntryList = g_zEffectAnim_EntryList;
    const short oldEntryCount = g_zEffectAnim_EntryCount;

    zReader::Node root = {};
    zReader::Node rootItems[47] = {};
    zReader::Node modeItems[2] = {};
    zReader::Node platformItems[3] = {};
    zReader::Node collisionItems[3] = {};
    zReader::Node platformPoint0[4] = {};
    zReader::Node platformPoint1[4] = {};
    zReader::Node collisionPoint0[4] = {};
    zReader::Node collisionPoint1[4] = {};
    zReader::Node ratesItems[3] = {};
    zReader::Node frictionItems[4] = {};
    zReader::Node stoppingItems[2] = {};
    zReader::Node lavaItems[2] = {};
    zReader::Node turnsItems[3] = {};
    zReader::Node turnDampingItems[2] = {};
    zReader::Node rateDampingItems[3] = {};
    zReader::Node aDampingItems[2] = {};
    zReader::Node altControlItems[4] = {};
    zReader::Node massItems[2] = {};
    zReader::Node gunPitchItems[3] = {};
    zReader::Node modeAltItems[2] = {};
    zReader::Node chasSmoothItems[2] = {};
    zReader::Node chasPitchItems[4] = {};
    zReader::Node hoverWaveItems[8] = {};
    zReader::Node subWaveItems[8] = {};
    zReader::Node collisionDItems[3] = {};
    zReader::Node t2aItems[3] = {};
    zReader::Node a2hItems[3] = {};
    zReader::Node soundsItems[5] = {};
    zReader::Node pitchScaleItems[2] = {};
    zReader::Node volumeScaleItems[2] = {};

    MakeAinetReaderArrayNode(root, rootItems, 47);
    int slot = 1;
    MakeAinetReaderStringNode(rootItems[slot++], "mode");
    MakeAinetReaderArrayNode(rootItems[slot++], modeItems, 2);
    MakeAinetReaderStringNode(rootItems[slot++], "platform");
    MakeAinetReaderArrayNode(rootItems[slot++], platformItems, 3);
    MakeAinetReaderStringNode(rootItems[slot++], "collision");
    MakeAinetReaderArrayNode(rootItems[slot++], collisionItems, 3);
    MakeAinetReaderStringNode(rootItems[slot++], "rates");
    MakeAinetReaderArrayNode(rootItems[slot++], ratesItems, 3);
    MakeAinetReaderStringNode(rootItems[slot++], "friction");
    MakeAinetReaderArrayNode(rootItems[slot++], frictionItems, 4);
    MakeAinetReaderStringNode(rootItems[slot++], "stopping");
    MakeAinetReaderArrayNode(rootItems[slot++], stoppingItems, 2);
    MakeAinetReaderStringNode(rootItems[slot++], "lava_slowdown");
    MakeAinetReaderArrayNode(rootItems[slot++], lavaItems, 2);
    MakeAinetReaderStringNode(rootItems[slot++], "turns");
    MakeAinetReaderArrayNode(rootItems[slot++], turnsItems, 3);
    MakeAinetReaderStringNode(rootItems[slot++], "turn_damping");
    MakeAinetReaderArrayNode(rootItems[slot++], turnDampingItems, 2);
    MakeAinetReaderStringNode(rootItems[slot++], "rate_damping");
    MakeAinetReaderArrayNode(rootItems[slot++], rateDampingItems, 3);
    MakeAinetReaderStringNode(rootItems[slot++], "a_damping");
    MakeAinetReaderArrayNode(rootItems[slot++], aDampingItems, 2);
    MakeAinetReaderStringNode(rootItems[slot++], "alt_control");
    MakeAinetReaderArrayNode(rootItems[slot++], altControlItems, 4);
    MakeAinetReaderStringNode(rootItems[slot++], "mass");
    MakeAinetReaderArrayNode(rootItems[slot++], massItems, 2);
    MakeAinetReaderStringNode(rootItems[slot++], "gun_pitch");
    MakeAinetReaderArrayNode(rootItems[slot++], gunPitchItems, 3);
    MakeAinetReaderStringNode(rootItems[slot++], "mode_alt");
    MakeAinetReaderArrayNode(rootItems[slot++], modeAltItems, 2);
    MakeAinetReaderStringNode(rootItems[slot++], "chas_smooth");
    MakeAinetReaderArrayNode(rootItems[slot++], chasSmoothItems, 2);
    MakeAinetReaderStringNode(rootItems[slot++], "chas_pitch");
    MakeAinetReaderArrayNode(rootItems[slot++], chasPitchItems, 4);
    MakeAinetReaderStringNode(rootItems[slot++], "hover_wave");
    MakeAinetReaderArrayNode(rootItems[slot++], hoverWaveItems, 8);
    MakeAinetReaderStringNode(rootItems[slot++], "sub_wave");
    MakeAinetReaderArrayNode(rootItems[slot++], subWaveItems, 8);
    MakeAinetReaderStringNode(rootItems[slot++], "collision_d");
    MakeAinetReaderArrayNode(rootItems[slot++], collisionDItems, 3);
    MakeAinetReaderStringNode(rootItems[slot++], "t2a_anims");
    MakeAinetReaderArrayNode(rootItems[slot++], t2aItems, 3);
    MakeAinetReaderStringNode(rootItems[slot++], "a2h_anims");
    MakeAinetReaderArrayNode(rootItems[slot++], a2hItems, 3);
    MakeAinetReaderStringNode(rootItems[slot++], "sounds");
    MakeAinetReaderArrayNode(rootItems[slot++], soundsItems, 5);

    MakeAinetReaderStringNode(modeItems[1], "hover");
    MakeAinetReaderArrayNode(platformItems[1], platformPoint0, 4);
    MakeAinetReaderArrayNode(platformItems[2], platformPoint1, 4);
    MakeAinetReaderArrayNode(collisionItems[1], collisionPoint0, 4);
    MakeAinetReaderArrayNode(collisionItems[2], collisionPoint1, 4);
    MakeAinetReaderFloatNode(platformPoint0[1], 1.0f);
    MakeAinetReaderFloatNode(platformPoint0[2], 2.0f);
    MakeAinetReaderFloatNode(platformPoint0[3], 3.0f);
    MakeAinetReaderFloatNode(platformPoint1[1], 4.0f);
    MakeAinetReaderFloatNode(platformPoint1[2], 5.0f);
    MakeAinetReaderFloatNode(platformPoint1[3], 6.0f);
    MakeAinetReaderFloatNode(collisionPoint0[1], -1.0f);
    MakeAinetReaderFloatNode(collisionPoint0[2], -2.0f);
    MakeAinetReaderFloatNode(collisionPoint0[3], -3.0f);
    MakeAinetReaderFloatNode(collisionPoint1[1], -4.0f);
    MakeAinetReaderFloatNode(collisionPoint1[2], -5.0f);
    MakeAinetReaderFloatNode(collisionPoint1[3], -6.0f);
    MakeAinetReaderFloatNode(ratesItems[1], 12.0f);
    MakeAinetReaderFloatNode(ratesItems[2], 34.0f);
    MakeAinetReaderFloatNode(frictionItems[1], 4.0f);
    MakeAinetReaderFloatNode(frictionItems[2], 5.0f);
    MakeAinetReaderFloatNode(frictionItems[3], 0.25f);
    MakeAinetReaderFloatNode(stoppingItems[1], 9.0f);
    MakeAinetReaderFloatNode(lavaItems[1], 0.7f);
    MakeAinetReaderFloatNode(turnsItems[1], 0.9f);
    MakeAinetReaderFloatNode(turnsItems[2], 1.9f);
    MakeAinetReaderFloatNode(turnDampingItems[1], 21.0f);
    MakeAinetReaderFloatNode(rateDampingItems[1], 22.0f);
    MakeAinetReaderFloatNode(rateDampingItems[2], 23.0f);
    MakeAinetReaderFloatNode(aDampingItems[1], 24.0f);
    MakeAinetReaderFloatNode(altControlItems[1], -7.0f);
    MakeAinetReaderFloatNode(altControlItems[2], 0.6f);
    MakeAinetReaderFloatNode(altControlItems[3], -2.0f);
    MakeAinetReaderFloatNode(massItems[1], 2.0f);
    MakeAinetReaderFloatNode(gunPitchItems[1], -0.5f);
    MakeAinetReaderFloatNode(gunPitchItems[2], 0.75f);
    MakeAinetReaderFloatNode(modeAltItems[1], 1.25f);
    MakeAinetReaderFloatNode(chasSmoothItems[1], -0.4f);
    MakeAinetReaderFloatNode(chasPitchItems[1], 8.0f);
    MakeAinetReaderFloatNode(chasPitchItems[2], 9.0f);
    MakeAinetReaderFloatNode(chasPitchItems[3], -10.0f);
    for (int index = 1; index < 8; ++index) {
        MakeAinetReaderFloatNode(hoverWaveItems[index], static_cast<float>(index));
        MakeAinetReaderFloatNode(subWaveItems[index], static_cast<float>(index + 10));
    }
    MakeAinetReaderFloatNode(collisionDItems[1], 0.3f);
    MakeAinetReaderFloatNode(collisionDItems[2], 0.4f);
    MakeAinetReaderStringNode(t2aItems[1], "fx_alpha");
    MakeAinetReaderStringNode(t2aItems[2], "fx_beta");
    MakeAinetReaderStringNode(a2hItems[1], "fx_gamma");
    MakeAinetReaderStringNode(a2hItems[2], "fx_missing");
    MakeAinetReaderStringNode(soundsItems[1], "pitch_scale");
    MakeAinetReaderArrayNode(soundsItems[2], pitchScaleItems, 2);
    MakeAinetReaderStringNode(soundsItems[3], "volume_scale");
    MakeAinetReaderArrayNode(soundsItems[4], volumeScaleItems, 2);
    MakeAinetReaderFloatNode(pitchScaleItems[1], 1.5f);
    MakeAinetReaderFloatNode(volumeScaleItems[1], 0.25f);

    zEffectAnimEntry effects[3] = {};
    std::strcpy(effects[0].name, "fx_alpha");
    std::strcpy(effects[1].name, "fx_beta");
    std::strcpy(effects[2].name, "fx_gamma");
    g_zEffectAnim_EntryList = effects;
    g_zEffectAnim_EntryCount = 3;

    PlayerMasterModalData modalData = {};
    Player::LoadMasterModalDataFromNode(&modalData, &root, "modal_hover");

    const bool ok =
        std::strcmp(modalData.modalName, "modal_hover") == 0 &&
        std::strcmp(modalData.modeName, "hover") == 0 && modalData.masterType == 4 &&
        modalData.platformPointCount == 2 && modalData.probePointCount == 2 &&
        Vec3Equals(modalData.probePoints[15], {1.0f, 2.0f, 3.0f}) &&
        Vec3Equals(modalData.probePoints[16], {4.0f, 5.0f, 6.0f}) &&
        Vec3Equals(modalData.probePoints[0], {-1.0f, -2.0f, -3.0f}) &&
        Vec3Equals(modalData.probePoints[1], {-4.0f, -5.0f, -6.0f}) &&
        FloatNear(modalData.accelRate, 12.0f) && FloatNear(modalData.maxSpeed, 34.0f) &&
        FloatNear(modalData.frictionStatic, 4.0f) &&
        FloatNear(modalData.frictionDynamic, 3.5999999f) &&
        FloatNear(modalData.frictionSlide, 0.25f) &&
        FloatNear(modalData.stoppingForce, 9.0f) &&
        FloatNear(modalData.quicksandSlowdown, 0.899999976f) &&
        FloatNear(modalData.lavaSlowdown, 0.7f) &&
        FloatNear(modalData.yawAccel, 0.9f) && FloatNear(modalData.yawRateMax, 1.9f) &&
        FloatNear(modalData.yawDamping, 21.0f) &&
        FloatNear(modalData.rateDampingAccel, 22.0f) &&
        FloatNear(modalData.rateDampingDecel, 23.0f) &&
        FloatNear(modalData.aDamping, 24.0f) &&
        FloatNear(modalData.hoverLiftDampingRate, -7.0f) &&
        FloatNear(modalData.hoverLiftScale, 0.6f) &&
        FloatNear(modalData.hoverNormalLerpRate, -2.0f) &&
        FloatNear(modalData.mass, 2.0f) && FloatNear(modalData.invMass, 0.5f) &&
        FloatNear(modalData.gunPitchMin, -0.5f) &&
        FloatNear(modalData.gunPitchRate, 0.75f) &&
        FloatNear(modalData.modeAltTransitionTime, 1.25f) &&
        FloatNear(modalData.chassisSmoothFactor, 0.4f) &&
        FloatNear(modalData.chassisPitchRate, 0.0f) &&
        FloatNear(modalData.chassisPitchMax, 0.0f) &&
        FloatNear(modalData.chassisPitchDamping, 0.0f) &&
        FloatNear(modalData.hoverPitchWaveBaseRate, 11.0f) &&
        FloatNear(modalData.hoverPitchWaveSpeedRate, 12.0f) &&
        FloatNear(modalData.hoverPitchWaveAmplitude, 13.0f) &&
        FloatNear(modalData.hoverRollWaveBaseRate, 14.0f) &&
        FloatNear(modalData.hoverRollWaveSpeedRate, 15.0f) &&
        FloatNear(modalData.hoverRollWaveAmplitude, 16.0f) &&
        FloatNear(modalData.hoverRollYawCoupleScale, 17.0f) &&
        FloatNear(modalData.collisionDampingA, 0.3f) &&
        FloatNear(modalData.collisionDampingB, 0.4f) &&
        modalData.fxList_fromTrackToAmphib[0] == &effects[0] &&
        modalData.fxList_fromTrackToAmphib[1] == &effects[1] &&
        modalData.fxList_fromAmphibToHover[0] == &effects[2] &&
        modalData.fxList_fromAmphibToHover[1] == nullptr &&
        FloatNear(modalData.sfxPitchScale, 1.5f) &&
        FloatNear(modalData.sfxVolumeScale, 0.25f);

    g_zEffectAnim_EntryList = oldEntryList;
    g_zEffectAnim_EntryCount = oldEntryCount;
    return ok ? 0 : 1;
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

extern "C" int player_tick_ai_mode2_top_level_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const int oldFinalized = g_Player_AiMode2State1Finalized;

    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState localGameState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage localPlayerState = {};
    AINetNode currentPathNode = {};
    AINetNode nextPathNode = {};
    saveState.playerState = &playerState;
    localGameState.playerState = &localPlayerState;
    currentPathNode.neighborNodes[0] = &nextPathNode;
    playerState.aiCurrentPathNode = &currentPathNode;
    playerState.aiCurrentPathNeighborIndex = 0;
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    localPlayerState.fxOffsetWorld = {12.0f, 13.0f, 14.0f};
    localPlayerState.worldPos = {10.0f, 0.0f, 0.0f};
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&localGameState;
    g_Player_AiMode2State1Finalized = 1;

    playerState.aiTopLevelState = 6;
    playerState.storedTargetPos = {};
    Player::TickAiMode2TopLevel(&saveState);
    const bool snapshotDefaultOk =
        Vec3Equals(playerState.storedTargetPos, {12.0f, 13.0f, 14.0f}) &&
        playerState.aiTopLevelState == 6;

    playerState.aiTopLevelState = 2;
    playerState.throttleInput = 0.75f;
    playerState.throttleInputCopy = 0.75f;
    playerState.steeringInput = 0.5f;
    playerState.steeringInputCopy = 0.5f;
    Player::TickAiMode2TopLevel(&saveState);
    const bool turnTowardOk =
        playerState.aiTopLevelState == 2 && playerState.throttleInput == 0.0f &&
        playerState.throttleInputCopy == 0.0f &&
        FloatNear(playerState.steeringInput, 0.0f) &&
        FloatNear(playerState.steeringInputCopy, 0.0f);

    playerState.aiTopLevelState = 5;
    playerState.aiReturnTopLevelState = 4;
    playerState.autoTurnActive = 0;
    playerState.autoTurnSign = -1;
    Player::TickAiMode2TopLevel(&saveState);
    const bool autoTurnRestoreOk =
        playerState.aiTopLevelState == 4 && playerState.autoTurnSign == 0;

    playerState.aiTopLevelState = 5;
    playerState.aiReturnTopLevelState = 3;
    playerState.autoTurnActive = 1;
    playerState.autoTurnSign = 1;
    Player::TickAiMode2TopLevel(&saveState);
    const bool autoTurnActiveOk =
        playerState.aiTopLevelState == 5 && playerState.autoTurnSign == 0;

    g_Player_AiMode2State1Finalized = oldFinalized;
    g_GameStateOrMapTable = oldGameStateOrMapTable;

    if (!snapshotDefaultOk) {
        return 1;
    }
    if (!turnTowardOk) {
        return 2;
    }
    if (!autoTurnRestoreOk) {
        return 3;
    }
    return autoTurnActiveOk ? 0 : 4;
}

extern "C" int player_mgr_tick_all_players_smoke(void) {
    zUtil_SaveGameState *const oldHead = g_PlayerSaveStateListHead;
    zUtil_SaveGameState *const oldLocalSaveState = g_LocalPlayerSaveState;
    zUtil_SaveGameState *const oldPlayer2SaveState = g_Player2SaveState;
    zUtil_SaveGameState *const oldCurrentSaveState = g_CurrentPlayerSaveState;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    int *const oldAudioApi = ZOPT_AUDIO_API;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    const float oldFrameDelta = g_FrameDeltaTimeSec;
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;
    const float oldPlayerDelta = g_Player_DeltaTime;
    const float oldInvDelta = g_Player_InvDeltaTime;
    const float oldScaled001 = g_Player_DeltaTimeScaled001;
    const float oldTotalScaled = g_Player_TotalTimeSecScaled;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;

    int audioApi = 0;
    int networkEnabled = 0;
    ZOPT_AUDIO_API = &audioApi;
    ZOPT_NETWORK_ENABLED = &networkEnabled;

    zUtil_SaveGameState localSaveState = {};
    zUtil_PlayerStateStorage localPlayerState = {};
    localSaveState.playerState = &localPlayerState;
    localPlayerState.lifecycleState = 4;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&localSaveState;
    g_CurrentPlayerSaveState = nullptr;
    g_PlayerSaveStateListHead = nullptr;
    g_LocalPlayerSaveState = nullptr;
    g_Player2SaveState = nullptr;

    g_FrameDeltaTimeSec = 0.001f;
    g_Time_AccumulatedTimeSec = 77.0f;
    Player::TickAllPlayers();
    const bool emptyListOk =
        FloatNear(g_Player_DeltaTime, 0.00499999989f) &&
        FloatNear(g_Player_InvDeltaTime, 200.0f) &&
        FloatNear(g_Player_DeltaTimeScaled001, 0.000049999997f) &&
        FloatNear(g_Player_TotalTimeSecScaled, 77.0f);

    zUtil_SaveGameState aiSaveState = {};
    zUtil_PlayerStateStorage aiPlayerState = {};
    aiSaveState.playerState = &aiPlayerState;
    g_PlayerSaveStateListHead = &aiSaveState;
    g_FrameDeltaTimeSec = 0.02f;
    g_Time_AccumulatedTimeSec = 88.0f;
    aiPlayerState.lifecycleState = 2;
    aiPlayerState.generalFlags = 0;
    aiPlayerState.aiActive = 9;
    aiPlayerState.targetDistanceSq = 123.0f;
    aiPlayerState.variantTag.count = 1;
    aiPlayerState.variantTag.tags[0] = 7;
    aiPlayerState.variantTag.tags[1] = 0xff;
    aiPlayerState.variantTag.tags[2] = 0xff;
    g_VariantTag_Current.count = 1;
    g_VariantTag_Current.tags[0] = 9;
    g_VariantTag_Current.tags[1] = 0xff;
    g_VariantTag_Current.tags[2] = 0xff;
    Player::TickAllPlayers();
    const bool inactiveAiOk =
        (aiPlayerState.generalFlags & 2) != 0 &&
        aiPlayerState.aiActive == 0 &&
        FloatNear(aiPlayerState.targetDistanceSq, 123.0f) &&
        FloatNear(g_Player_DeltaTime, 0.02f) &&
        FloatNear(g_Player_InvDeltaTime, 50.0f) &&
        FloatNear(g_Player_DeltaTimeScaled001, 0.00019999999f) &&
        FloatNear(g_Player_TotalTimeSecScaled, 88.0f);

    g_VariantTag_Current = oldVariantTagCurrent;
    g_Player_TotalTimeSecScaled = oldTotalScaled;
    g_Player_DeltaTimeScaled001 = oldScaled001;
    g_Player_InvDeltaTime = oldInvDelta;
    g_Player_DeltaTime = oldPlayerDelta;
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    g_FrameDeltaTimeSec = oldFrameDelta;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    ZOPT_AUDIO_API = oldAudioApi;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_CurrentPlayerSaveState = oldCurrentSaveState;
    g_Player2SaveState = oldPlayer2SaveState;
    g_LocalPlayerSaveState = oldLocalSaveState;
    g_PlayerSaveStateListHead = oldHead;

    if (!emptyListOk) {
        return 1;
    }
    return inactiveAiOk ? 0 : 2;
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

extern "C" int player_ai_try_enter_mode2_attack_pursuit_los_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const int oldFinalized = g_Player_AiMode2State1Finalized;
    const float oldTotalTime = g_Player_TotalTimeSecScaled;
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;

    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState buddySave = {};
    zUtil_SaveGameState localGameState = {};
    zUtil_PlayerStateStorage aiState = {};
    zUtil_PlayerStateStorage buddyState = {};
    zUtil_PlayerStateStorage localPlayerState = {};
    AINet aiNet = {};
    AINetNode currentPathNode = {};
    AINetNode restoreNode = {};
    zClass_NodePartial aiRootNode = {};
    zClass_NodePartial localRootNode = {};
    zClass_WorldDataPartial emptyWorldData = {};
    zClass_NodePartial emptyWorld = {};

    saveState.playerState = &aiState;
    saveState.aiPeerRingNext = &buddySave;
    buddySave.playerState = &buddyState;
    buddySave.aiPeerRingNext = &saveState;
    localGameState.playerState = &localPlayerState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&localGameState;
    emptyWorld.classData = &emptyWorldData;
    g_Player_RuntimeDiScene = &emptyWorld;
    g_Player_AiMode2State1Finalized = 0;
    g_Player_TotalTimeSecScaled = 12.0f;
    g_Time_AccumulatedTimeSec = 3.0f;

    aiState.rootNode = &aiRootNode;
    aiState.aiNet = &aiNet;
    aiState.aiCurrentPathNode = &currentPathNode;
    aiState.aiCurrentPathNeighborIndex = 0;
    aiState.aiTopLevelState = 7;
    aiState.aiStateUntilTime = 5.0f;
    aiState.aiAttackRadiusSq = 100.0f;
    aiState.fxOffsetWorld = {3.0f, 4.0f, 0.0f};
    aiState.aiMode2AttackDwell = 2.0f;
    aiState.aiMode2SteeringRetryCount = 6;
    aiState.aiTargetLineOfSightClear = 0;
    aiNet.attackBuddyNetId = 0;
    currentPathNode.neighborNodes[0] = &restoreNode;
    restoreNode.position = {8.0f, 9.0f, 10.0f};
    localPlayerState.rootNode = &localRootNode;
    localPlayerState.fxOffsetWorld = {0.0f, 0.0f, 0.0f};

    int result = Player::AiTryEnterMode2AttackPursuitIfLineOfSight(&saveState);
    const bool successOk =
        result == 1 && aiState.aiTopLevelState == 1 && aiState.aiSavedTopLevelState == 7 &&
        aiState.aiTargetLineOfSightClear == 1 && aiState.aiMode2SteeringRetryCount == 0 &&
        Vec3Equals(aiState.aiRestoreTarget, {8.0f, 9.0f, 10.0f}) &&
        FloatNear(g_DiPickQueryPoint.x, 0.0f) && FloatNear(g_DiPickQueryPoint.y, 0.0f) &&
        FloatNear(g_DiSegmentEnd.x, 3.0f) && FloatNear(g_DiSegmentEnd.y, 5.5f);

    aiState.aiTopLevelState = 9;
    aiState.aiSavedTopLevelState = 0;
    aiState.aiTargetLineOfSightClear = 5;
    aiState.aiMode2SteeringRetryCount = 4;
    g_Player_TotalTimeSecScaled = 4.0f;
    result = Player::AiTryEnterMode2AttackPursuitIfLineOfSight(&saveState);
    const bool timeGateOk =
        result == 0 && aiState.aiTopLevelState == 9 &&
        aiState.aiTargetLineOfSightClear == 5 && aiState.aiMode2SteeringRetryCount == 4;

    g_Player_TotalTimeSecScaled = 12.0f;
    aiState.aiAttackRadiusSq = 1.0f;
    result = Player::AiTryEnterMode2AttackPursuitIfLineOfSight(&saveState);
    const bool rangeGateOk =
        result == 0 && aiState.aiTopLevelState == 9 &&
        aiState.aiTargetLineOfSightClear == 5;

    aiState.aiAttackRadiusSq = 100.0f;
    g_Player_AiMode2State1Finalized = 1;
    result = Player::AiTryEnterMode2AttackPursuitIfLineOfSight(&saveState);
    const bool finalizedGateOk =
        result == 0 && aiState.aiTopLevelState == 9 &&
        aiState.aiTargetLineOfSightClear == 5;

    g_Player_TotalTimeSecScaled = oldTotalTime;
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    g_Player_AiMode2State1Finalized = oldFinalized;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_GameStateOrMapTable = oldGameStateOrMapTable;

    if (!successOk) {
        return 1;
    }
    if (!timeGateOk) {
        return 2;
    }
    if (!rangeGateOk) {
        return 3;
    }
    return finalizedGateOk ? 0 : 4;
}

extern "C" int player_ai_rebuild_synthetic_path_to_node_if_far_smoke(void) {
    const float oldTotalTime = g_Player_TotalTimeSecScaled;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    AINetNode currentPathNode = {};
    AINetNode targetNode = {};
    saveState.playerState = &playerState;

    playerState.worldPos = {3.0f, 0.0f, 4.0f};
    currentPathNode.position = {0.0f, 0.0f, 0.0f};
    playerState.aiCurrentPathNode = &currentPathNode;
    playerState.aiCurrentPathNeighborIndex = 2;
    playerState.aiNextPathRebuildTime = 99.0f;
    Player::AiRebuildSyntheticPathToNodeIfFar(&saveState, &targetNode);
    const bool nearOk =
        playerState.aiCurrentPathNode == &currentPathNode &&
        playerState.aiCurrentPathNeighborIndex == 2 &&
        FloatNear(playerState.aiNextPathRebuildTime, 99.0f);

    playerState.worldPos = {30.0f, 0.0f, 40.0f};
    playerState.aiCurrentPathNeighborIndex = 2;
    g_Player_TotalTimeSecScaled = 7.5f;
    Player::AiRebuildSyntheticPathToNodeIfFar(&saveState, &targetNode);
    AINetNode *const syntheticNode = playerState.aiCurrentPathNode;
    AINetPathProbeFan *const fan =
        syntheticNode != nullptr ? syntheticNode->probeFans[0] : nullptr;
    const bool farOk =
        syntheticNode != nullptr && syntheticNode != &currentPathNode &&
        syntheticNode->nodeIndex == -1 &&
        Vec3Equals(syntheticNode->position, {30.0f, 0.0f, 40.0f}) &&
        syntheticNode->neighborNodes[0] == &targetNode && fan != nullptr &&
        FloatNear(fan->pathWidth, 10.0f) &&
        playerState.aiCurrentPathNeighborIndex == 0 &&
        FloatNear(playerState.aiNextPathRebuildTime, 8.5f);

    if (syntheticNode != nullptr && syntheticNode != &currentPathNode) {
        syntheticNode->Free();
    }
    g_Player_TotalTimeSecScaled = oldTotalTime;

    if (!nearOk) {
        return 1;
    }
    return farOk ? 0 : 2;
}

extern "C" int player_tick_ai_mode2_steering_substate_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const float oldTotalTimeSecScaled = g_Player_TotalTimeSecScaled;

    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState targetState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage targetPlayerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    AINet aiNet = {};
    PlayerGunFireController activeAltGunController = {};
    OptCatalogEntryDef optCatalogEntry = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    targetState.playerState = &targetPlayerState;
    modalState.masterModalData = &modalData;
    playerState.aiNet = &aiNet;
    playerState.activeAltGunController = &activeAltGunController;
    activeAltGunController.optCatalogEntry = &optCatalogEntry;
    activeAltGunController.nextDispatchTime = 200.0f;
    activeAltGunController.dispatchRepeatDelay = 1.0f;
    optCatalogEntry.velocity = 10.0f;
    g_GameStateOrMapTable =
        static_cast<zInput_GameStateOrMapTablePartial *>(static_cast<void *>(&targetState));
    g_Player_TotalTimeSecScaled = 100.0f;

    modalData.masterType = 2;
    aiNet.pursuitParam0 = 1.0f;
    aiNet.pursuitParam1 = 10.0f;
    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.vehiclePitchRad = 0.1f;
    playerState.aiNextPathRebuildTime = 200.0f;
    playerState.aiCurrentSteeringSubstate = 0;
    playerState.aiRestoreTarget = {0.0f, 0.0f, 0.0f};
    playerState.aiRestoreDistanceSq = 1000.0f;
    playerState.aiTopLevelState = 5;
    playerState.aiSavedTopLevelState = 8;
    playerState.aiNotPursuitDwell = 2.0f;
    targetPlayerState.worldPos = {3.0f, 4.0f, 0.0f};
    targetPlayerState.lifecycleState = 1;

    Player::TickAiMode2SteeringSubstate(&saveState);
    const float verticalScale = 4.0f / 3.0f;
    const float expectedPitch =
        (g_Player_AiMode2_SteeringPitchInputScale * verticalScale - 0.1f) *
        g_Player_AiMode2_SteeringPitchTurnGain;
    const bool directSubOk =
        playerState.throttleInput == 0.0f && playerState.steeringInput == 0.0f &&
        FloatNear(playerState.subPitchInput, expectedPitch) &&
        FloatNear(playerState.subPitchInputCopy, expectedPitch) &&
        FloatNear(playerState.subVerticalInput, 0.4f) &&
        FloatNear(playerState.subVerticalInputCopy, 0.4f) &&
        playerState.aiTopLevelState == 5;

    targetPlayerState.lifecycleState = 4;
    playerState.aiTopLevelState = 5;
    playerState.aiSavedTopLevelState = 8;
    Player::TickAiMode2SteeringSubstate(&saveState);
    const bool restoreOk =
        playerState.aiTopLevelState == 8 && FloatNear(playerState.aiStateUntilTime, 102.0f);

    g_Player_TotalTimeSecScaled = oldTotalTimeSecScaled;
    g_GameStateOrMapTable = oldGameStateOrMapTable;

    if (!directSubOk) {
        return 1;
    }
    return restoreOk ? 0 : 2;
}

extern "C" int player_update_ai_mode2_move_and_turn_toward_target_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    AINet aiNet = {};
    saveState.playerState = &playerState;
    playerState.aiNet = &aiNet;
    aiNet.pursuitParam0 = 5.0f;
    aiNet.pursuitParam1 = 12.0f;

    Player::UpdateAiMode2MoveAndTurnTowardTarget(&saveState, -0.25f, -0.1f, 20.0f);
    const bool backwardLeftOk =
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        playerState.steeringInput == -1.0f && playerState.steeringInputCopy == -1.0f;

    Player::UpdateAiMode2MoveAndTurnTowardTarget(&saveState, 0.0f, 0.0f, 20.0f);
    const bool zeroForwardRightOk =
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        playerState.steeringInput == 1.0f && playerState.steeringInputCopy == 1.0f;

    Player::UpdateAiMode2MoveAndTurnTowardTarget(&saveState, 0.5f, -0.25f, 14.0f);
    const bool farOk =
        playerState.throttleInput == 1.0f && playerState.throttleInputCopy == 1.0f &&
        FloatNear(playerState.steeringInput, -0.25f) &&
        FloatNear(playerState.steeringInputCopy, -0.25f);

    Player::UpdateAiMode2MoveAndTurnTowardTarget(&saveState, 0.5f, 0.75f, 3.0f);
    const bool nearOk =
        playerState.throttleInput == -1.0f && playerState.throttleInputCopy == -1.0f &&
        FloatNear(playerState.steeringInput, 0.75f) &&
        FloatNear(playerState.steeringInputCopy, 0.75f);

    Player::UpdateAiMode2MoveAndTurnTowardTarget(&saveState, 0.5f, 0.25f, 8.0f);
    const bool bandOk =
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        FloatNear(playerState.steeringInput, 0.25f) &&
        FloatNear(playerState.steeringInputCopy, 0.25f);

    if (!backwardLeftOk) {
        return 1;
    }
    if (!zeroForwardRightOk) {
        return 2;
    }
    if (!farOk) {
        return 3;
    }
    if (!nearOk) {
        return 4;
    }
    return bandOk ? 0 : 5;
}

extern "C" int player_update_ai_mode2_turn_toward_player_no_throttle_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState targetState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage targetPlayerState = {};
    saveState.playerState = &playerState;
    targetState.playerState = &targetPlayerState;
    g_GameStateOrMapTable =
        static_cast<zInput_GameStateOrMapTablePartial *>(static_cast<void *>(&targetState));

    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.throttleInput = 9.0f;
    playerState.throttleInputCopy = 8.0f;

    targetPlayerState.worldPos = {10.0f, 0.0f, 10.0f};
    Player::UpdateAiMode2TurnTowardPlayerNoThrottle(&saveState);
    const float diagonal = static_cast<float>(std::sqrt(0.5f));
    const bool aheadOk =
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        FloatNear(playerState.steeringInput, -diagonal) &&
        FloatNear(playerState.steeringInputCopy, -diagonal);

    targetPlayerState.worldPos = {-10.0f, 0.0f, 10.0f};
    Player::UpdateAiMode2TurnTowardPlayerNoThrottle(&saveState);
    const bool behindLeftOk =
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        playerState.steeringInput == -1.0f && playerState.steeringInputCopy == -1.0f;

    targetPlayerState.worldPos = {-10.0f, 0.0f, -10.0f};
    Player::UpdateAiMode2TurnTowardPlayerNoThrottle(&saveState);
    const bool behindRightOk =
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        playerState.steeringInput == 1.0f && playerState.steeringInputCopy == 1.0f;

    g_GameStateOrMapTable = oldGameStateOrMapTable;

    if (!aheadOk) {
        return 1;
    }
    if (!behindLeftOk) {
        return 2;
    }
    return behindRightOk ? 0 : 3;
}

extern "C" int player_update_ai_mode2_turn_in_place_toward_player_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState targetState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage targetPlayerState = {};
    saveState.playerState = &playerState;
    targetState.playerState = &targetPlayerState;
    g_GameStateOrMapTable =
        static_cast<zInput_GameStateOrMapTablePartial *>(static_cast<void *>(&targetState));

    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.throttleInput = 9.0f;
    playerState.throttleInputCopy = 8.0f;

    targetPlayerState.worldPos = {10.0f, 0.0f, 10.0f};
    Player::UpdateAiMode2TurnInPlaceTowardPlayer(&saveState);
    const float diagonal = static_cast<float>(std::sqrt(0.5f));
    const bool aheadOk =
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        FloatNear(playerState.steeringInput, -diagonal) &&
        FloatNear(playerState.steeringInputCopy, -diagonal);

    targetPlayerState.worldPos = {-10.0f, 0.0f, 10.0f};
    Player::UpdateAiMode2TurnInPlaceTowardPlayer(&saveState);
    const bool behindLeftOk =
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        playerState.steeringInput == -1.0f && playerState.steeringInputCopy == -1.0f;

    targetPlayerState.worldPos = {-10.0f, 0.0f, -10.0f};
    Player::UpdateAiMode2TurnInPlaceTowardPlayer(&saveState);
    const bool behindRightOk =
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        playerState.steeringInput == 1.0f && playerState.steeringInputCopy == 1.0f;

    g_GameStateOrMapTable = oldGameStateOrMapTable;

    if (!aheadOk) {
        return 1;
    }
    if (!behindLeftOk) {
        return 2;
    }
    return behindRightOk ? 0 : 3;
}

extern "C" int player_solve_alt_gun_lead_target_point_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState targetState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage targetPlayerState = {};
    PlayerGunFireController activeAltGunController = {};
    OptCatalogEntryDef optCatalogEntry = {};
    zVec3 outTargetPos = {};
    saveState.playerState = &playerState;
    targetState.playerState = &targetPlayerState;
    playerState.activeAltGunController = &activeAltGunController;
    activeAltGunController.optCatalogEntry = &optCatalogEntry;

    optCatalogEntry.velocity = 10.0f;
    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    playerState.projectileSpawnVel = {0.0f, 0.0f, 0.0f};
    targetPlayerState.worldPos = {3.0f, 4.0f, 5.0f};
    targetPlayerState.fxOffsetWorld = {100.0f, 101.0f, 102.0f};
    targetPlayerState.projectileSpawnVel = {20.0f, 0.0f, 0.0f};
    Player::SolveAltGunLeadTargetPoint(&saveState, &targetState, &outTargetPos);
    const bool fallbackOk = Vec3Equals(outTargetPos, targetPlayerState.worldPos);

    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    playerState.projectileSpawnVel = {0.0f, 0.0f, 0.0f};
    targetPlayerState.worldPos = {30.0f, 0.0f, 0.0f};
    targetPlayerState.fxOffsetWorld = {31.0f, 2.0f, 3.0f};
    targetPlayerState.projectileSpawnVel = {1.0f, 0.0f, 0.0f};

    std::srand(12345);
    const int expectedRand = std::rand();
    std::srand(12345);
    Player::SolveAltGunLeadTargetPoint(&saveState, &targetState, &outTargetPos);

    const float leadScale = (PlayerFastSqrtEstimateForTest(9.0f) + 0.3f) / 0.99f;
    const float expectedY =
        2.0f - (static_cast<float>(expectedRand) * 3.05185094e-05f - 0.5f) * -2.0f;
    const bool leadOk =
        FloatNear(outTargetPos.x, 31.0f + leadScale) &&
        FloatNear(outTargetPos.y, expectedY) &&
        FloatNear(outTargetPos.z, 3.0f);

    if (!fallbackOk) {
        return 1;
    }
    return leadOk ? 0 : 2;
}

extern "C" int player_tick_ai_mode2_alt_gun_attack_window_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const float oldTotalTimeSecScaled = g_Player_TotalTimeSecScaled;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const int oldBreakOnFirst = g_cls_di_BreakOnFirstCandidate;
    const int oldStopAfterFirst = g_cls_di_StopAfterFirstHit;

    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState targetState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage targetPlayerState = {};
    PlayerGunFireController activeAltGunController = {};
    OptCatalogEntryDef optCatalogEntry = {};
    zClass_NodePartial aiRootNode = {};
    zClass_NodePartial targetRootNode = {};
    zClass_NodePartial worldNode = {};
    zClass_WorldDataPartial worldData = {};

    saveState.playerState = &playerState;
    targetState.playerState = &targetPlayerState;
    playerState.activeAltGunController = &activeAltGunController;
    playerState.rootNode = &aiRootNode;
    playerState.statusMeterScaled = 0.25f;
    playerState.fxOffsetWorld = {1.0f, 2.0f, 3.0f};
    playerState.projectileSpawnVel = {0.0f, 0.0f, 0.0f};
    targetPlayerState.rootNode = &targetRootNode;
    targetPlayerState.lifecycleState = 1;
    targetPlayerState.worldPos = {10.0f, 11.0f, 12.0f};
    targetPlayerState.fxOffsetWorld = {20.0f, 21.0f, 22.0f};
    targetPlayerState.projectileSpawnVel = {20.0f, 0.0f, 0.0f};
    activeAltGunController.optCatalogEntry = &optCatalogEntry;
    activeAltGunController.dispatchRepeatDelay = 4.0f;
    activeAltGunController.aiAttackRangeMin = 5.0f;
    activeAltGunController.aiAttackRangeMax = 50.0f;
    optCatalogEntry.velocity = 10.0f;
    worldNode.classData = &worldData;
    aiRootNode.flags = 0x10;
    targetRootNode.flags = 0x10;

    g_GameStateOrMapTable =
        static_cast<zInput_GameStateOrMapTablePartial *>(static_cast<void *>(&targetState));
    g_Player_RuntimeDiScene = &worldNode;
    g_cls_di_BreakOnFirstCandidate = 99;
    g_cls_di_StopAfterFirstHit = 99;

    g_Player_TotalTimeSecScaled = 10.0f;
    playerState.aiStateEndTime = 5.0f;
    playerState.aiNotPursuitDwell = 2.0f;
    playerState.aiMode2AttackDwell = 3.0f;
    activeAltGunController.nextDispatchTime = 20.0f;
    Player::TickAiMode2AltGunAttackWindow(&saveState, 10.0f, 1.0f);
    const bool refreshOk =
        FloatNear(playerState.aiStateStartTime, 12.0f) &&
        FloatNear(playerState.aiStateEndTime, 15.0f) &&
        playerState.altGunDispatchRequested == 0;

    g_Player_TotalTimeSecScaled = 20.0f;
    playerState.aiStateStartTime = 19.0f;
    playerState.aiStateEndTime = 30.0f;
    playerState.altGunDispatchRequested = 0;
    playerState.altGunFireHeldFlag = 0;
    playerState.progressTargetCount = 7;
    playerState.progressTargetSlots[0].targetPos = &playerState.worldPos;
    playerState.progressTargetSlots[0].targetVelocity = &playerState.projectileSpawnVel;
    activeAltGunController.nextDispatchTime = 1.0f;
    optCatalogEntry.flags = 0;
    Player::TickAiMode2AltGunAttackWindow(&saveState, 10.0f, 0.8f);
    const bool dispatchOk =
        playerState.altGunDispatchRequested == 1 &&
        FloatNear(activeAltGunController.nextDispatchTime, 28.0f) &&
        playerState.progressTargetCount == 0 &&
        playerState.progressTargetSlots[0].targetPos == nullptr &&
        playerState.progressTargetSlots[0].targetVelocity == nullptr &&
        Vec3Equals(playerState.storedTargetPos, targetPlayerState.worldPos);

    playerState.altGunFireHeldFlag = 1;
    playerState.altGunDispatchRequested = 5;
    activeAltGunController.nextDispatchTime = 30.0f;
    Player::TickAiMode2AltGunAttackWindow(&saveState, 50.0f, 0.75f);
    const bool heldCopyOk =
        playerState.altGunDispatchRequested == 5 &&
        Vec3Equals(playerState.storedTargetPos, targetPlayerState.fxOffsetWorld);

    targetPlayerState.lifecycleState = 4;
    Player::TickAiMode2AltGunAttackWindow(&saveState, 10.0f, 1.0f);
    const bool heldClearOk =
        playerState.altGunDispatchRequested == 0 &&
        FloatNear(activeAltGunController.nextDispatchTime, 24.0f);

    g_cls_di_StopAfterFirstHit = oldStopAfterFirst;
    g_cls_di_BreakOnFirstCandidate = oldBreakOnFirst;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_Player_TotalTimeSecScaled = oldTotalTimeSecScaled;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_GameStateOrMapTable = oldGameStateOrMapTable;

    if (!refreshOk) {
        return 1;
    }
    if (!dispatchOk) {
        return 2;
    }
    if (!heldCopyOk) {
        return 3;
    }
    return heldClearOk ? 0 : 4;
}

extern "C" int player_update_ai_mode2_move_and_turn_toward_offset_target_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState targetState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage targetPlayerState = {};
    AINet aiNet = {};
    saveState.playerState = &playerState;
    targetState.playerState = &targetPlayerState;
    playerState.aiNet = &aiNet;

    aiNet.pursuitParam0 = 0.0f;
    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    targetPlayerState.worldPos = {10.0f, 0.0f, 0.0f};
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    Player::UpdateAiMode2MoveAndTurnTowardOffsetTarget(&saveState, &targetState);
    const bool directAheadOk =
        playerState.throttleInput == 1.0f && playerState.throttleInputCopy == 1.0f &&
        playerState.steeringInput == 0.0f && playerState.steeringInputCopy == 0.0f;

    targetPlayerState.worldPos = {0.0f, 0.0f, 10.0f};
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    Player::UpdateAiMode2MoveAndTurnTowardOffsetTarget(&saveState, &targetState);
    const bool sideClampOk =
        playerState.throttleInput == 0.25f && playerState.throttleInputCopy == 0.25f &&
        playerState.steeringInput == -1.0f && playerState.steeringInputCopy == -1.0f;

    playerState.worldPos = {1.0f, 0.0f, 0.0f};
    targetPlayerState.worldPos = {0.0f, 0.0f, 0.0f};
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    Player::UpdateAiMode2MoveAndTurnTowardOffsetTarget(&saveState, &targetState);
    const bool behindOk =
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        playerState.steeringInput == 1.0f && playerState.steeringInputCopy == 1.0f;

    aiNet.pursuitParam0 = 10.0f;
    playerState.worldPos = {10.0f, 0.0f, 0.0f};
    targetPlayerState.worldPos = {0.0f, 0.0f, 0.0f};
    playerState.steerBasisNorm = {0.0f, 0.0f, 1.0f};
    Player::UpdateAiMode2MoveAndTurnTowardOffsetTarget(&saveState, &targetState);

    const float expectedX = 10.0f * 0.965925813f - 10.0f;
    const float expectedZ = 10.0f * 0.258819044f;
    const float expectedLen =
        static_cast<float>(std::sqrt(expectedX * expectedX + expectedZ * expectedZ));
    const float expectedSteer = expectedX / expectedLen;
    const float expectedThrottle = 1.0f - static_cast<float>(std::fabs(expectedSteer));
    const bool rotatedOffsetOk =
        FloatNear(playerState.throttleInput, expectedThrottle) &&
        FloatNear(playerState.throttleInputCopy, expectedThrottle) &&
        FloatNear(playerState.steeringInput, expectedSteer) &&
        FloatNear(playerState.steeringInputCopy, expectedSteer);

    if (!directAheadOk) {
        return 1;
    }
    if (!sideClampOk) {
        return 2;
    }
    if (!behindOk) {
        return 3;
    }
    return rotatedOffsetOk ? 0 : 4;
}

extern "C" int player_update_ai_mode2_move_and_turn_toward_dynamic_offset_target_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState targetState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage targetPlayerState = {};
    AINet aiNet = {};
    saveState.playerState = &playerState;
    targetState.playerState = &targetPlayerState;
    playerState.aiNet = &aiNet;
    aiNet.pursuitParam0 = 10.0f;
    aiNet.pursuitParam1 = 2.0f;
    targetPlayerState.worldPos = {0.0f, 0.0f, 0.0f};
    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.aiDynamicOffsetDir = {1.0f, 0.0f, 0.0f};

    playerState.localVel.z = 0.0f;
    Player::UpdateAiMode2MoveAndTurnTowardDynamicOffsetTarget(&saveState, &targetState, 25.0f);
    const bool forwardOk =
        playerState.throttleInput == 1.0f && playerState.throttleInputCopy == 1.0f &&
        playerState.steeringInput == 0.0f && playerState.steeringInputCopy == 0.0f;

    playerState.localVel.z = 3.0f;
    Player::UpdateAiMode2MoveAndTurnTowardDynamicOffsetTarget(&saveState, &targetState, 15.0f);
    const float reverseTargetX = 10.0f;
    const float reverseTargetZ = 1.0f;
    const float reverseLen = static_cast<float>(
        std::sqrt(reverseTargetX * reverseTargetX + reverseTargetZ * reverseTargetZ));
    const float reverseSteer = reverseTargetZ / reverseLen;
    const float reverseThrottle = -(1.0f - static_cast<float>(std::fabs(reverseSteer)));
    const bool reverseOk =
        FloatNear(playerState.throttleInput, reverseThrottle) &&
        FloatNear(playerState.throttleInputCopy, reverseThrottle) &&
        FloatNear(playerState.steeringInput, reverseSteer) &&
        FloatNear(playerState.steeringInputCopy, reverseSteer);

    aiNet.pursuitParam0 = 1.0f;
    aiNet.pursuitParam1 = 1.0f;
    playerState.localVel.z = 0.0f;
    playerState.aiDynamicOffsetDir = {-1.0f, 0.0f, 0.0f};
    Player::UpdateAiMode2MoveAndTurnTowardDynamicOffsetTarget(&saveState, &targetState, 3.0f);
    const bool backupOk =
        playerState.throttleInput == -1.0f && playerState.throttleInputCopy == -1.0f &&
        playerState.steeringInput == 0.0f && playerState.steeringInputCopy == 0.0f;

    if (!forwardOk) {
        return 1;
    }
    if (!reverseOk) {
        return 2;
    }
    return backupOk ? 0 : 3;
}

extern "C" int player_tick_ai_mode2_offset_target_steering_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

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
    zUtil_SaveGameState targetState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage targetPlayerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    AINet aiNet = {};
    zClass_NodePartial rootNode = {};
    zClass_WorldDataPartial emptyWorldData = {};
    zClass_NodePartial emptyWorld = {};
    emptyWorld.classData = &emptyWorldData;

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    targetState.playerState = &targetPlayerState;
    modalState.masterModalData = &modalData;
    playerState.rootNode = &rootNode;
    playerState.aiNet = &aiNet;
    playerState.projectileSpawnVel = {1.0f, 0.0f, 0.0f};
    modalData.probePoints[1].z = -1.0f;
    aiNet.pursuitParam0 = 0.0f;
    g_Player_RuntimeDiScene = &emptyWorld;
    g_GameStateOrMapTable =
        static_cast<zInput_GameStateOrMapTablePartial *>(static_cast<void *>(&targetState));

    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    targetPlayerState.worldPos = {10.0f, 0.0f, 0.0f};
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    Player::TickAiMode2OffsetTargetSteering(&saveState, 0.0f, 0.0f, 0.0f);
    const bool offsetBranchOk =
        playerState.throttleInput == 1.0f && playerState.throttleInputCopy == 1.0f &&
        playerState.steeringInput == 0.0f && playerState.steeringInputCopy == 0.0f &&
        playerState.aiCurrentSteeringSubstate == 0;

    playerState.playerCollisionResolved = 1;
    playerState.aiCurrentSteeringSubstate = 2;
    playerState.aiReturnSteeringSubstate = 99;
    playerState.throttleInput = 3.0f;
    playerState.steeringInput = 4.0f;
    playerState.throttleInputCopy = 5.0f;
    playerState.steeringInputCopy = 6.0f;
    playerState.aiMode2SteeringRetryCount = 7;
    Player::TickAiMode2OffsetTargetSteering(&saveState, 0.0f, 0.0f, 0.0f);
    const bool autoTurnBranchOk =
        playerState.throttleInput == 0.0f && playerState.steeringInput == 0.0f &&
        playerState.throttleInputCopy == 0.0f && playerState.steeringInputCopy == 0.0f &&
        playerState.aiReturnSteeringSubstate == 2 &&
        playerState.aiCurrentSteeringSubstate == 6 &&
        playerState.aiMode2SteeringRetryCount == 8 &&
        playerState.autoTurnActive == 1 &&
        FloatNear(playerState.autoTurnTargetDir.x, 1.0f) &&
        FloatNear(playerState.autoTurnTargetDir.y, 0.0f) &&
        FloatNear(playerState.autoTurnTargetDir.z, 0.0f);

    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_GameStateOrMapTable = oldGameStateOrMapTable;

    if (!offsetBranchOk) {
        return 1;
    }
    return autoTurnBranchOk ? 0 : 2;
}

extern "C" int player_tick_ai_mode2_dynamic_offset_target_steering_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

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
    zUtil_SaveGameState targetState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage targetPlayerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    AINet aiNet = {};
    zClass_NodePartial rootNode = {};
    zClass_WorldDataPartial emptyWorldData = {};
    zClass_NodePartial emptyWorld = {};
    emptyWorld.classData = &emptyWorldData;

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    targetState.playerState = &targetPlayerState;
    modalState.masterModalData = &modalData;
    playerState.rootNode = &rootNode;
    playerState.aiNet = &aiNet;
    playerState.projectileSpawnVel = {1.0f, 0.0f, 0.0f};
    playerState.aiDynamicOffsetDir = {1.0f, 0.0f, 0.0f};
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    modalData.probePoints[1].z = -1.0f;
    aiNet.pursuitParam0 = 10.0f;
    aiNet.pursuitParam1 = 2.0f;
    g_Player_RuntimeDiScene = &emptyWorld;
    g_GameStateOrMapTable =
        static_cast<zInput_GameStateOrMapTablePartial *>(static_cast<void *>(&targetState));

    Player::TickAiMode2DynamicOffsetTargetSteering(&saveState, 0.0f, 0.0f, 25.0f);
    const bool dynamicBranchOk =
        playerState.throttleInput == 1.0f && playerState.throttleInputCopy == 1.0f &&
        playerState.steeringInput == 0.0f && playerState.steeringInputCopy == 0.0f &&
        playerState.aiCurrentSteeringSubstate == 0;

    playerState.playerCollisionResolved = 1;
    playerState.aiCurrentSteeringSubstate = 2;
    playerState.aiReturnSteeringSubstate = 99;
    playerState.throttleInput = 3.0f;
    playerState.steeringInput = 4.0f;
    playerState.throttleInputCopy = 5.0f;
    playerState.steeringInputCopy = 6.0f;
    playerState.aiMode2SteeringRetryCount = 7;
    targetPlayerState.worldPos = {10.0f, 0.0f, 0.0f};
    Player::TickAiMode2DynamicOffsetTargetSteering(&saveState, 0.0f, 0.0f, 15.0f);
    const bool autoTurnBranchOk =
        playerState.throttleInput == 0.0f && playerState.steeringInput == 0.0f &&
        playerState.throttleInputCopy == 0.0f && playerState.steeringInputCopy == 0.0f &&
        playerState.aiReturnSteeringSubstate == 2 &&
        playerState.aiCurrentSteeringSubstate == 6 &&
        playerState.aiMode2SteeringRetryCount == 8 &&
        playerState.autoTurnActive == 1 &&
        FloatNear(playerState.autoTurnTargetDir.x, 1.0f) &&
        FloatNear(playerState.autoTurnTargetDir.y, 0.0f) &&
        FloatNear(playerState.autoTurnTargetDir.z, 0.0f);

    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_GameStateOrMapTable = oldGameStateOrMapTable;

    if (!dynamicBranchOk) {
        return 1;
    }
    return autoTurnBranchOk ? 0 : 2;
}

extern "C" int player_ai_restore_saved_top_level_state_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    playerState.aiTopLevelState = 7;
    playerState.aiSavedTopLevelState = 12;
    Player::AiRestoreSavedTopLevelState(&saveState);

    return playerState.aiTopLevelState == 12 && playerState.aiSavedTopLevelState == 12 ? 0 : 1;
}

extern "C" int player_ai_finalize_mode2_state1_for_all_players_smoke(void) {
    zUtil_SaveGameState *const oldHead = g_PlayerSaveStateListHead;
    const int oldFinalized = g_Player_AiMode2State1Finalized;

    zUtil_SaveGameState saves[3] = {};
    zUtil_PlayerStateStorage states[3] = {};
    for (int i = 0; i < 3; ++i) {
        saves[i].playerState = &states[i];
    }
    saves[0].next = &saves[1];
    saves[1].next = &saves[2];

    states[0].lifecycleState = 2;
    states[0].aiTopLevelState = 1;
    states[0].aiSavedTopLevelState = 11;
    states[1].lifecycleState = 2;
    states[1].aiTopLevelState = 3;
    states[1].aiSavedTopLevelState = 12;
    states[2].lifecycleState = 4;
    states[2].aiTopLevelState = 1;
    states[2].aiSavedTopLevelState = 13;

    g_PlayerSaveStateListHead = &saves[0];
    g_Player_AiMode2State1Finalized = 0;
    Player::AiFinalizeMode2State1ForAllPlayers();
    const bool listOk = states[0].aiTopLevelState == 11 &&
                        states[0].aiSavedTopLevelState == 11 &&
                        states[1].aiTopLevelState == 3 &&
                        states[1].aiSavedTopLevelState == 12 &&
                        states[2].aiTopLevelState == 1 &&
                        states[2].aiSavedTopLevelState == 13 &&
                        g_Player_AiMode2State1Finalized == 1;

    g_PlayerSaveStateListHead = nullptr;
    g_Player_AiMode2State1Finalized = 0;
    Player::AiFinalizeMode2State1ForAllPlayers();
    const bool emptyOk = g_Player_AiMode2State1Finalized == 1;

    g_PlayerSaveStateListHead = oldHead;
    g_Player_AiMode2State1Finalized = oldFinalized;
    return listOk && emptyOk ? 0 : 1;
}

extern "C" int player_ai_steer_toward_path_node_forward_smoke(void) {
    const float oldTotalTimeSecScaled = g_Player_TotalTimeSecScaled;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    AINetNode currentNode = {};
    AINetNode forwardNode = {};
    saveState.playerState = &playerState;
    currentNode.neighborNodes[0] = &forwardNode;
    playerState.aiCurrentPathNode = &currentNode;
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    g_Player_TotalTimeSecScaled = 40.0f;

    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    forwardNode.position = {3.0f, 10.0f, 0.0f};
    Player::AiSteerTowardPathNodeForward(&saveState);
    const bool advanceOk =
        playerState.aiCurrentPathNode == &forwardNode &&
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        playerState.steeringInput == 0.0f && playerState.steeringInputCopy == 0.0f &&
        FloatNear(playerState.unknown_0fa4, 44.0f);

    playerState.aiCurrentPathNode = &currentNode;
    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    forwardNode.position = {10.0f, 0.0f, 10.0f};
    Player::AiSteerTowardPathNodeForward(&saveState);
    const float diagonal = static_cast<float>(std::sqrt(0.5f));
    const bool forwardOk =
        FloatNear(playerState.throttleInput, 1.0f - diagonal) &&
        FloatNear(playerState.throttleInputCopy, 1.0f - diagonal) &&
        FloatNear(playerState.steeringInput, -diagonal) &&
        FloatNear(playerState.steeringInputCopy, -diagonal);

    playerState.aiCurrentPathNode = &currentNode;
    forwardNode.position = {-10.0f, 0.0f, 10.0f};
    Player::AiSteerTowardPathNodeForward(&saveState);
    const bool behindOk =
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        playerState.steeringInput == -1.0f && playerState.steeringInputCopy == -1.0f;

    g_Player_TotalTimeSecScaled = oldTotalTimeSecScaled;

    if (!advanceOk) {
        return 1;
    }
    if (!forwardOk) {
        return 2;
    }
    return behindOk ? 0 : 3;
}

extern "C" int player_ai_steer_toward_path_node_reverse_smoke(void) {
    const float oldTotalTimeSecScaled = g_Player_TotalTimeSecScaled;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    AINetNode currentNode = {};
    AINetNode forwardNode = {};
    saveState.playerState = &playerState;
    currentNode.neighborNodes[0] = &forwardNode;
    playerState.aiCurrentPathNode = &currentNode;
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    g_Player_TotalTimeSecScaled = 40.0f;

    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    forwardNode.position = {3.0f, 10.0f, 0.0f};
    Player::AiSteerTowardPathNodeReverse(&saveState);
    const bool advanceOk =
        playerState.aiCurrentPathNode == &forwardNode &&
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        playerState.steeringInput == 0.0f && playerState.steeringInputCopy == 0.0f &&
        FloatNear(playerState.unknown_0fa4, 54.0f);

    playerState.aiCurrentPathNode = &currentNode;
    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    forwardNode.position = {-10.0f, 0.0f, 10.0f};
    Player::AiSteerTowardPathNodeReverse(&saveState);
    const float diagonal = static_cast<float>(std::sqrt(0.5f));
    const bool reverseForwardOk =
        FloatNear(playerState.throttleInput, -(1.0f - diagonal)) &&
        FloatNear(playerState.throttleInputCopy, -(1.0f - diagonal)) &&
        FloatNear(playerState.steeringInput, diagonal) &&
        FloatNear(playerState.steeringInputCopy, diagonal);

    playerState.aiCurrentPathNode = &currentNode;
    forwardNode.position = {10.0f, 0.0f, 10.0f};
    Player::AiSteerTowardPathNodeReverse(&saveState);
    const bool behindOk =
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        playerState.steeringInput == 1.0f && playerState.steeringInputCopy == 1.0f;

    g_Player_TotalTimeSecScaled = oldTotalTimeSecScaled;

    if (!advanceOk) {
        return 1;
    }
    if (!reverseForwardOk) {
        return 2;
    }
    return behindOk ? 0 : 3;
}

extern "C" int player_tick_ai_mode2_timed_path_steering_smoke(void) {
    const float oldTotalTimeSecScaled = g_Player_TotalTimeSecScaled;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    AINetNode currentNode = {};
    AINetNode anchorNode = {};
    AINetNode forwardNode = {};
    saveState.playerState = &playerState;
    playerState.aiCurrentPathNode = &currentNode;
    playerState.aiHomePathNode = &anchorNode;
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    currentNode.neighborNodes[0] = &forwardNode;
    forwardNode.position = {10.0f, 0.0f, 0.0f};

    g_Player_TotalTimeSecScaled = 10.0f;
    playerState.unknown_0fa4 = 20.0f;
    playerState.throttleInput = 0.75f;
    playerState.throttleInputCopy = 0.5f;
    playerState.steeringInput = -0.25f;
    playerState.steeringInputCopy = -0.5f;
    playerState.recentHitFlag = 0;
    Player::TickAiMode2TimedPathSteering(&saveState);
    const bool timeGateOk =
        FloatNear(playerState.throttleInput, 0.75f) &&
        FloatNear(playerState.throttleInputCopy, 0.5f) &&
        FloatNear(playerState.steeringInput, -0.25f) &&
        FloatNear(playerState.steeringInputCopy, -0.5f) &&
        playerState.recentHitFlag == 1;

    playerState.aiCurrentPathNode = &currentNode;
    playerState.aiHomePathNode = &currentNode;
    currentNode.neighborNodes[0] = &forwardNode;
    forwardNode.position = {10.0f, 0.0f, 0.0f};
    playerState.throttleInput = 0.0f;
    playerState.throttleInputCopy = 0.0f;
    playerState.steeringInput = 0.0f;
    playerState.steeringInputCopy = 0.0f;
    playerState.recentHitFlag = 0;
    playerState.unknown_0fa4 = 5.0f;
    Player::TickAiMode2TimedPathSteering(&saveState);
    const bool forwardBranchOk =
        FloatNear(playerState.throttleInput, 1.0f) &&
        FloatNear(playerState.throttleInputCopy, 1.0f) &&
        FloatNear(playerState.steeringInput, 0.0f) &&
        FloatNear(playerState.steeringInputCopy, 0.0f) &&
        playerState.recentHitFlag == 1;

    playerState.aiCurrentPathNode = &currentNode;
    playerState.aiHomePathNode = &anchorNode;
    currentNode.neighborNodes[0] = &anchorNode;
    currentNode.nodeIndex = 7;
    anchorNode.position = {-10.0f, 0.0f, 0.0f};
    playerState.throttleInput = 0.0f;
    playerState.throttleInputCopy = 0.0f;
    playerState.steeringInput = 0.0f;
    playerState.steeringInputCopy = 0.0f;
    playerState.recentHitFlag = 0;
    playerState.unknown_0fa4 = 5.0f;
    Player::TickAiMode2TimedPathSteering(&saveState);
    const bool reverseBranchOk =
        FloatNear(playerState.throttleInput, -1.0f) &&
        FloatNear(playerState.throttleInputCopy, -1.0f) &&
        FloatNear(playerState.steeringInput, 0.0f) &&
        FloatNear(playerState.steeringInputCopy, 0.0f) &&
        playerState.recentHitFlag == 1;

    g_Player_TotalTimeSecScaled = oldTotalTimeSecScaled;

    if (!timeGateOk) {
        return 1;
    }
    if (!forwardBranchOk) {
        return 2;
    }
    return reverseBranchOk ? 0 : 3;
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

extern "C" int player_update_gun_dispatch_requests_from_trigger_latches_smoke(void) {
    const float oldTotalTimeSecScaled = g_Player_TotalTimeSecScaled;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerGunFireController altController = {};
    PlayerGunFireController primaryController = {};
    OptCatalogEntryDef altEntry = {};
    saveState.playerState = &playerState;
    playerState.activeAltGunController = &altController;
    playerState.activePrimaryGunController = &primaryController;
    altController.optCatalogEntry = &altEntry;

    playerState.netInputBit16Latch = 0;
    playerState.altGunDispatchRequested = 1;
    Player::UpdateGunDispatchRequestsFromTriggerLatches(&saveState);
    if (playerState.altGunDispatchRequested != 0) {
        g_Player_TotalTimeSecScaled = oldTotalTimeSecScaled;
        return 1;
    }

    playerState.netInputBit16Latch = 1;
    playerState.altGunDispatchRequested = 5;
    altEntry.flags = 0;
    Player::UpdateGunDispatchRequestsFromTriggerLatches(&saveState);
    if (playerState.altGunDispatchRequested != 5) {
        g_Player_TotalTimeSecScaled = oldTotalTimeSecScaled;
        return 2;
    }

    altEntry.flags = 2;
    Player::UpdateGunDispatchRequestsFromTriggerLatches(&saveState);
    if (playerState.altGunDispatchRequested != 1) {
        g_Player_TotalTimeSecScaled = oldTotalTimeSecScaled;
        return 3;
    }

    playerState.netInputBit17Latch = 0;
    playerState.primaryGunDispatchRequested = 1;
    Player::UpdateGunDispatchRequestsFromTriggerLatches(&saveState);
    if (playerState.primaryGunDispatchRequested != 0) {
        g_Player_TotalTimeSecScaled = oldTotalTimeSecScaled;
        return 4;
    }

    g_Player_TotalTimeSecScaled = 10.0f;
    playerState.netInputBit17Latch = 1;
    playerState.primaryGunDispatchRequested = 0;
    playerState.altGunTransitionState = 0;
    primaryController.nextDispatchTime = 9.5f;
    primaryController.dispatchRepeatDelay = 1.25f;
    Player::UpdateGunDispatchRequestsFromTriggerLatches(&saveState);
    if (playerState.primaryGunDispatchRequested != 1 ||
        primaryController.nextDispatchTime != 11.25f) {
        g_Player_TotalTimeSecScaled = oldTotalTimeSecScaled;
        return 5;
    }

    playerState.primaryGunDispatchRequested = 7;
    primaryController.nextDispatchTime = 12.0f;
    Player::UpdateGunDispatchRequestsFromTriggerLatches(&saveState);
    if (playerState.primaryGunDispatchRequested != 7 ||
        primaryController.nextDispatchTime != 12.0f) {
        g_Player_TotalTimeSecScaled = oldTotalTimeSecScaled;
        return 6;
    }

    playerState.primaryGunDispatchRequested = 8;
    primaryController.nextDispatchTime = 9.0f;
    playerState.altGunTransitionState = 0x80;
    Player::UpdateGunDispatchRequestsFromTriggerLatches(&saveState);
    if (playerState.primaryGunDispatchRequested != 8 ||
        primaryController.nextDispatchTime != 9.0f) {
        g_Player_TotalTimeSecScaled = oldTotalTimeSecScaled;
        return 7;
    }

    g_Player_TotalTimeSecScaled = oldTotalTimeSecScaled;
    return 0;
}

extern "C" int player_update_debug_overlay_hud_smoke(void) {
    HudUiStringMenu *const oldStringMenu = g_HudUiMgrStringMenu;
    HudUiCounterTextPanel *const oldObjectiveCounter = g_HudUiMgrObjectiveCounterTextPanel;
    const int oldReticleMode = g_HudUiMgrReticleMode;
    const int oldHudCounterValue = g_Player_HudCounterValue;
    HudUiMessage oldMessages[10] = {};
    for (int index = 0; index < 10; ++index) {
        oldMessages[index] = g_HudUiMgrMessages[index];
        g_HudUiMgrMessages[index] = {};
        TestFieldAt<int>(&g_HudUiMgrMessages[index].panel, 0x2a4) = 0;
    }

    HudUiCounterTextPanel counter = {};
    reinterpret_cast<HudUiPanel *>(&counter)->vtbl =
        reinterpret_cast<const HudUiPanel_FTable *>(&g_HudUiCounterTextPanel_FTable);
    g_HudUiMgrObjectiveCounterTextPanel = &counter;

    HudUiPanel_FTable auxTable = {};
    auxTable.slots[24] = MethodAddress(&PlayerTestDebugAuxPanel::SetVisible);
    auxTable.slots[29] = reinterpret_cast<std::uintptr_t>(&PlayerTestDebugAuxSetTextFmt);

    HudUiStringMenu menu = {};
    for (int index = 0; index < 23; ++index) {
        *reinterpret_cast<const HudUiPanel_FTable **>(&menu.items[index]) = &auxTable;
    }
    g_HudUiMgrStringMenu = &menu;
    g_PlayerTestDebugAuxMenu = &menu;
    std::memset(g_PlayerTestDebugAuxText, 0, sizeof(g_PlayerTestDebugAuxText));
    std::memset(g_PlayerTestDebugAuxVisible, 0, sizeof(g_PlayerTestDebugAuxVisible));
    std::memset(g_PlayerTestDebugAuxSetTextCount, 0, sizeof(g_PlayerTestDebugAuxSetTextCount));
    std::memset(g_PlayerTestDebugAuxSetVisibleCount, 0,
                sizeof(g_PlayerTestDebugAuxSetVisibleCount));

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    PlayerGunFireController altController = {};
    OptCatalogEntryDef altEntry = {};
    zClass_NodePartial rootNode = {};
    AINetNode currentPathNode = {};

    std::strcpy(rootNode.name, "DebugRoot");
    currentPathNode.nodeIndex = 42;
    modalData.masterType = 3;
    modalState.masterModalData = &modalData;
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    playerState.rootNode = &rootNode;
    playerState.activeAltGunController = &altController;
    playerState.activePrimaryGunController = nullptr;
    playerState.lifecycleState = 1;
    playerState.slipSfxActive = 1;
    playerState.aiCurrentPathNode = &currentPathNode;
    playerState.worldPos = {12.75f, -4.25f, 100.5f};
    playerState.restartYawRad = 1.0f;
    playerState.aimTargetDistanceApprox = 20.0f;
    altEntry.range = 25.0f;
    altController.optCatalogEntry = &altEntry;
    altController.weaponBankIndex = 3;
    altController.weaponSideIndex = 1;
    altController.ammoOrCharge = 7.5f;
    g_Player_HudCounterValue = 77;
    g_HudUiMgrReticleMode = 0;

    Player::UpdateDebugOverlayHud(&saveState, 123, 456);

    int failure = 0;
    if (g_HudUiMgrReticleMode != 1) {
        failure = 1;
    } else if (std::strcmp(&TestFieldAt<char>(&counter, 0x34), "77") != 0) {
        failure = 2;
    } else if (g_PlayerTestDebugAuxSetTextCount[1] != 1 ||
               g_PlayerTestDebugAuxSetVisibleCount[1] != 1 ||
               g_PlayerTestDebugAuxVisible[1] != 1 ||
               std::strcmp(g_PlayerTestDebugAuxText[1],
                           "DebugRoot using TRACK dynamics - S") != 0) {
        failure = 3;
    } else if (g_PlayerTestDebugAuxSetTextCount[2] != 1 ||
               g_PlayerTestDebugAuxSetVisibleCount[2] != 1 ||
               g_PlayerTestDebugAuxVisible[2] != 1 ||
               std::strcmp(g_PlayerTestDebugAuxText[2], "POS 12 -4 100 YAW 57") != 0) {
        failure = 4;
    }

    if (reinterpret_cast<HudUiPanel *>(&counter)->hFont != nullptr) {
        DeleteObject(reinterpret_cast<HudUiPanel *>(&counter)->hFont);
        reinterpret_cast<HudUiPanel *>(&counter)->hFont = nullptr;
    }
    g_HudUiMgrStringMenu = oldStringMenu;
    g_HudUiMgrObjectiveCounterTextPanel = oldObjectiveCounter;
    g_HudUiMgrReticleMode = oldReticleMode;
    g_Player_HudCounterValue = oldHudCounterValue;
    g_PlayerTestDebugAuxMenu = nullptr;
    for (int index = 0; index < 10; ++index) {
        g_HudUiMgrMessages[index] = oldMessages[index];
    }

    return failure;
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

    PlayerModalState modalState = {};
    saveState.primaryModalState = &modalState;
    zClass_Object3DDataPartial rootData = {};
    zClass_NodePartial rootNode = {};
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    playerState.rootNode = &rootNode;

    const zMat4x3 rootMatrix = {2.0f, 3.0f, 5.0f, 7.0f, 11.0f, 13.0f,
                                17.0f, 19.0f, 23.0f, 29.0f, 31.0f, 37.0f};
    SetObjectLocalMatrix(&rootData, rootMatrix);
    modalState.modalNode = nullptr;
    Player::BuildGunFireTransform(&saveState);
    if (!MatrixEquals(playerState.gunFireTransform, rootMatrix)) {
        return 20;
    }

    zClass_Object3DDataPartial modalData = {};
    zClass_NodePartial modalNode = {};
    modalNode.classId = 5;
    modalNode.classData = &modalData;
    const zMat4x3 modalMatrix = {41.0f, 43.0f, 47.0f, 53.0f, 59.0f, 61.0f,
                                 67.0f, 71.0f, 73.0f, 79.0f, 83.0f, 89.0f};
    SetObjectLocalMatrix(&modalData, modalMatrix);
    modalState.modalNode = &modalNode;
    Player::BuildGunFireTransform(&saveState);
    const zMat4x3 expectedGunFireTransform = {
        modalMatrix.xx * rootMatrix.xx + modalMatrix.xy * rootMatrix.yx +
            modalMatrix.xz * rootMatrix.zx,
        modalMatrix.xx * rootMatrix.xy + modalMatrix.xy * rootMatrix.yy +
            modalMatrix.xz * rootMatrix.zy,
        modalMatrix.xx * rootMatrix.xz + modalMatrix.xy * rootMatrix.yz +
            modalMatrix.xz * rootMatrix.zz,
        modalMatrix.yx * rootMatrix.xx + modalMatrix.yy * rootMatrix.yx +
            modalMatrix.yz * rootMatrix.zx,
        modalMatrix.yx * rootMatrix.xy + modalMatrix.yy * rootMatrix.yy +
            modalMatrix.yz * rootMatrix.zy,
        modalMatrix.yx * rootMatrix.xz + modalMatrix.yy * rootMatrix.yz +
            modalMatrix.yz * rootMatrix.zz,
        modalMatrix.zy * rootMatrix.yx + modalMatrix.zz * rootMatrix.zx,
        modalMatrix.zy * rootMatrix.yy + modalMatrix.zz * rootMatrix.zy,
        modalMatrix.zy * rootMatrix.yz + modalMatrix.zz * rootMatrix.zz,
        modalMatrix.posY * rootMatrix.yx + modalMatrix.posZ * rootMatrix.zx +
            rootMatrix.posX,
        modalMatrix.posY * rootMatrix.yy + modalMatrix.posZ * rootMatrix.zy +
            rootMatrix.posY,
        modalMatrix.posY * rootMatrix.yz + modalMatrix.posZ * rootMatrix.zz +
            rootMatrix.posZ};
    if (!MatrixEquals(playerState.gunFireTransform, expectedGunFireTransform)) {
        return 21;
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

    const float localAimX = turretMatrix.zx * gunMatrix.posZ;
    const float localAimY = turretMatrix.posY + gunMatrix.posY;
    const float localAimZ = turretMatrix.zz * gunMatrix.posZ + turretMatrix.posZ;
    const zVec3 expectedAimBasis = {
        playerState.gunFireTransform.xx * localAimX +
            playerState.gunFireTransform.yx * localAimY +
            playerState.gunFireTransform.zx * localAimZ + playerState.gunFireTransform.posX,
        playerState.gunFireTransform.xy * localAimX +
            playerState.gunFireTransform.yy * localAimY +
            playerState.gunFireTransform.zy * localAimZ + playerState.gunFireTransform.posY,
        playerState.gunFireTransform.xz * localAimX +
            playerState.gunFireTransform.yz * localAimY +
            playerState.gunFireTransform.zz * localAimZ + playerState.gunFireTransform.posZ};
    zVec3 aimBasis = {};
    Player::UpdateAltGunAimBasisOrigin(&saveState, &aimBasis);
    if (!Vec3Equals(aimBasis, expectedAimBasis)) {
        return 22;
    }

    const zVec3 aimDirection = {3.0f, 0.5f, 4.0f};
    const float aimHorizontalLen = PlayerFastSqrtEstimateForTest(
        aimDirection.x * aimDirection.x + aimDirection.z * aimDirection.z);
    Player::UpdateGunAndTurretAimNodes(&aimDirection, &gunNode, &turretNode);
    const bool gunAimOk =
        gunData.localMatrix[0] == 1.0f && gunData.localMatrix[1] == 0.0f &&
        gunData.localMatrix[2] == 0.0f && gunData.localMatrix[3] == 0.0f &&
        gunData.localMatrix[4] == aimHorizontalLen && gunData.localMatrix[5] == 0.5f &&
        gunData.localMatrix[6] == 0.0f && gunData.localMatrix[7] == -0.5f &&
        gunData.localMatrix[8] == aimHorizontalLen;
    const float yawForward = -(aimDirection.z / aimHorizontalLen);
    const float yawSide = -(aimDirection.x / aimHorizontalLen);
    const bool turretAimOk =
        turretData.localMatrix[0] == yawForward && turretData.localMatrix[1] == 0.0f &&
        turretData.localMatrix[2] == -yawSide && turretData.localMatrix[3] == 0.0f &&
        turretData.localMatrix[4] == 1.0f && turretData.localMatrix[5] == 0.0f &&
        turretData.localMatrix[6] == yawSide && turretData.localMatrix[7] == 0.0f &&
        turretData.localMatrix[8] == yawForward;
    if (!gunAimOk || !turretAimOk) {
        return 23;
    }
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

    OptCatalogEntryDef aimEntry = {};
    aimEntry.range = 1000.0f;
    aimEntry.gravity = 0.0f;
    controller.optCatalogEntry = &aimEntry;

    PlayerMasterModalData masterModalData = {};
    masterModalData.gunPitchRate = 0.8f;
    masterModalData.gunPitchMin = -0.8f;
    modalState.masterModalData = &masterModalData;
    modalState.modalNode = nullptr;

    SetObjectLocalMatrix(&rootData, identityMatrix);
    SetObjectLocalMatrix(&gunData, identityMatrix);
    SetObjectLocalMatrix(&turretData, identityMatrix);
    playerState.gunFireTransform = identityMatrix;
    playerState.aimBasisOrigin = {};
    playerState.storedTargetPos = {0.0f, 0.0f, 10.0f};
    playerState.altGunAimOrigin = {0.0f, 0.0f, 1.0f};
    playerState.gunFireDir = {9.0f, 8.0f, 7.0f};
    playerState.usePresetGunFireDir = 0;
    playerState.altGunTransitionState = 1;
    playerState.cameraTickEnabled = 0;

    const float oldFrameDeltaTimeSec = g_FrameDeltaTimeSec;
    g_FrameDeltaTimeSec = 0.0f;
    Player::UpdateAltGunAimDirection(&saveState);
    g_FrameDeltaTimeSec = oldFrameDeltaTimeSec;

    const float expectedAimDistanceApprox = PlayerFastSqrtEstimateForTest(100.0f);
    const bool updateAimOk =
        Vec3Equals(playerState.aimBasisOrigin, {0.0f, 0.0f, 0.0f}) &&
        Vec3Equals(playerState.altGunAimOrigin, {0.0f, 0.0f, 1.0f}) &&
        Vec3Equals(playerState.gunFireDir, {0.0f, 0.0f, 1.0f}) &&
        playerState.aimPitchResult == -1.0f &&
        playerState.aimTargetDistanceApprox == expectedAimDistanceApprox &&
        playerState.usePresetGunFireDir == 0 && gunData.localMatrix[0] == 1.0f &&
        gunData.localMatrix[4] == 1.0f && gunData.localMatrix[8] == 1.0f &&
        turretData.localMatrix[0] == -1.0f && turretData.localMatrix[8] == -1.0f;
    if (!updateAimOk) {
        return 24;
    }

    SetObjectLocalMatrix(&gunData, identityMatrix);
    SetObjectLocalMatrix(&turretData, identityMatrix);
    playerState.gunFireTransform = identityMatrix;
    playerState.aimBasisOrigin = {10.0f, 20.0f, 30.0f};

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

extern "C" int player_primary_gun_fire_point_selection_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    playerState.worldPos = {10.0f, 20.0f, 30.0f};
    playerState.steerBasisRaw = {1.0f, 2.0f, 3.0f};
    PlayerGunFireSlot sentinelSlot = {};
    PlayerGunFireSlot *outSlot = &sentinelSlot;
    Player::SelectPrimaryGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.primaryFireOrigin, {10.0f, 21.0f, 30.0f}) ||
        !Vec3Equals(playerState.aimBasisOrigin, {10.0f, 21.0f, 30.0f}) ||
        !Vec3Equals(playerState.gunFireDir, {1.0f, 2.0f, 3.0f}) || outSlot != &sentinelSlot) {
        return 1;
    }

    PlayerModalState modalState = {};
    saveState.primaryModalState = &modalState;
    zClass_Object3DDataPartial rootData = {};
    zClass_NodePartial rootNode = {};
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    playerState.rootNode = &rootNode;

    const zMat4x3 identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    SetObjectLocalMatrix(&rootData, identityMatrix);
    modalState.modalNode = nullptr;
    Player::BuildGunFireTransform(&saveState);

    zClass_Object3DDataPartial gunData = {};
    zClass_NodePartial gunNode = {};
    gunNode.classId = 5;
    gunNode.classData = &gunData;
    zClass_Object3DDataPartial turretData = {};
    zClass_NodePartial turretNode = {};
    turretNode.classId = 5;
    turretNode.classData = &turretData;
    SetObjectLocalMatrix(&gunData, identityMatrix);
    SetObjectLocalMatrix(&turretData, identityMatrix);
    playerState.gunNode = &gunNode;
    playerState.turretNode = &turretNode;
    playerState.gunFireTransform = identityMatrix;
    playerState.aimBasisOrigin = {10.0f, 20.0f, 30.0f};

    PlayerGunFireController controller = {};
    zClass_NodePartial primaryAttachNode = {};
    zClass_NodePartial secondaryAttachNode = {};
    zClass_NodePartial activeAttachState = {};
    controller.attachNodePrimary = &primaryAttachNode;
    controller.attachNodeSecondary = &secondaryAttachNode;
    playerState.activePrimaryGunController = &controller;
    playerState.firePointCenter = {1.0f, 2.0f, 3.0f};
    playerState.firePointRight = {4.0f, 5.0f, 6.0f};
    playerState.firePointLeft = {7.0f, 8.0f, 9.0f};

    outSlot = nullptr;
    playerState.primaryHardpointSelectState = 0;
    controller.attachState = nullptr;
    Player::SelectPrimaryGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.primaryFireOrigin, {11.0f, 22.0f, 33.0f}) ||
        outSlot != &playerState.altFireSlotCenter ||
        playerState.altFireSlotCenter.attachNode != &primaryAttachNode ||
        playerState.primaryHardpointSelectState != 0) {
        return 2;
    }

    outSlot = nullptr;
    controller.attachState = &activeAttachState;
    Player::SelectPrimaryGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.primaryFireOrigin, {10.0f, 20.0f, 30.0f}) ||
        outSlot != &playerState.altFireSlotCenter ||
        playerState.altFireSlotCenter.attachNode != &primaryAttachNode ||
        playerState.primaryHardpointSelectState != 0) {
        return 3;
    }

    outSlot = nullptr;
    controller.attachState = nullptr;
    playerState.primaryHardpointSelectState = 1;
    Player::SelectPrimaryGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.primaryFireOrigin, {14.0f, 25.0f, 36.0f}) ||
        outSlot != &playerState.altFireSlotRight ||
        playerState.altFireSlotRight.attachNode != &secondaryAttachNode ||
        playerState.primaryHardpointSelectState != 2) {
        return 4;
    }

    outSlot = nullptr;
    playerState.primaryHardpointSelectState = 2;
    Player::SelectPrimaryGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.primaryFireOrigin, {17.0f, 28.0f, 39.0f}) ||
        outSlot != &playerState.altFireSlotLeft ||
        playerState.altFireSlotLeft.attachNode != &primaryAttachNode ||
        playerState.primaryHardpointSelectState != 1) {
        return 5;
    }

    return 0;
}

extern "C" int player_alt_gun_ensure_aux_effect_active_smoke(void) {
    void *const oldFreeRuntimeList = g_OptCatalogFreeRuntimeInstanceList;
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    const float oldNextSpawnScale = g_OptCatalogNextSpawnScale;
    const int oldNetworkOptionState = g_OptCatalogNetworkOptionState;
    OptCatalogAllocRuntimeGateCallback const oldGateCallback =
        g_OptCatalog_AllocRuntimeGateCallback;
    int *const oldJoystickOption = ZOPT_INPUT_JOYSTICK;
    const int oldForceFeedbackCaps = g_zInput_JoystickCaps_ForceFeedback;
    zInput_FFEffectSet *const oldEffectSet = g_zInputFfEffectSet;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    int result = 0;
    int joystickEnabled = 1;
    ZOPT_INPUT_JOYSTICK = &joystickEnabled;
    g_zInput_JoystickCaps_ForceFeedback = 1;
    g_OptCatalogNetworkOptionState = 0;
    g_OptCatalog_AllocRuntimeGateCallback = nullptr;
    g_OptCatalogNextSpawnScale = 1.0f;

    zInput_DiEffectVtable effectVtable = {};
    effectVtable.Start_1c = &PlayerTestEffectStart;
    effectVtable.Stop_20 = &PlayerTestEffectStop;
    zInput_DiEffect primaryEffect = {&effectVtable};
    zInput_FFEffectSet effectSet = {};
    effectSet.PrimaryFire = &primaryEffect;
    g_zInputFfEffectSet = &effectSet;
    g_playerTestFfStopCount = 0;
    g_playerTestFfStartCount = 0;
    g_playerTestFfLastIterations = 0;
    g_playerTestFfLastStartFlags = 0;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerGunFireController controller = {};
    OptCatalogEntryDef entry = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial runtimeWorld = {};
    zClass_NodeFreeListSlot projectileSlot = {};
    zClass_Object3DDataPartial projectileData = {};
    OptCatalogRuntimeInstanceStorage runtime = {};

    saveState.playerState = &playerState;
    playerState.rootNode = &rootNode;
    playerState.variantTag.count = 2;
    playerState.variantTag.tags[0] = 3;
    playerState.variantTag.tags[1] = 4;
    playerState.variantTag.tags[2] = 5;
    playerState.projectileSpawnVel = {2.0f, 3.0f, 4.0f};
    playerState.gunFireDir = {0.0f, 1.0f, 0.0f};
    playerState.usePresetGunFireDir = 1;
    controller.optCatalogEntry = &entry;
    entry.velocity = 5.0f;
    entry.fireFxSelectedSoundIndex = -1;
    entry.fireFxSelectedEffectIndex = -1;
    entry.flyoutSelectedEffectIndex = -1;
    runtimeWorld.classId = 3;
    projectileSlot.node.classId = 5;
    projectileSlot.node.classData = &projectileData;
    runtime.projectileNode = &projectileSlot.node;
    g_OptCatalogRuntimeWorld = &runtimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = &runtime;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;

    zUtil_SaveGameState remoteSaveState = {};
    zUtil_PlayerStateStorage remotePlayerState = {};
    PlayerGunFireController remoteController = {};
    OptCatalogEntryDef remoteEntry = {};
    zClass_NodePartial remoteRootNode = {};
    zClass_NodePartial remoteRuntimeWorld = {};
    zClass_NodeFreeListSlot remoteProjectileSlot = {};
    zClass_Object3DDataPartial remoteProjectileData = {};
    OptCatalogRuntimeInstanceStorage remoteRuntime = {};

    zVec3 effectPos = {10.0f, 11.0f, 12.0f};
    const int presetResult =
        Player::EnsureGunAuxEffectActive(&saveState, &controller, &effectPos);

    if (presetResult != 1) {
        result = 1;
    } else if (entry.activeRuntimeListHead != &runtime) {
        result = 2;
    } else if (g_OptCatalogFreeRuntimeInstanceList != nullptr) {
        result = 3;
    } else if (runtime.ownerNode != &rootNode) {
        result = 4;
    } else if (!Vec3Equals(runtime.pos, effectPos) || !Vec3Equals(runtime.origin, effectPos)) {
        result = 5;
    } else if (!Vec3Equals(runtime.dir, {0.0f, 1.0f, 0.0f})) {
        result = 6;
    } else if (!Vec3Equals(runtime.velocity, {2.0f, 8.0f, 4.0f})) {
        result = 7;
    } else if (runtime.saveState != &saveState) {
        result = 8;
    } else if (runtimeWorld.listCountB != 1 || runtimeWorld.listB[0] != &projectileSlot.node) {
        result = 9;
    } else if (g_playerTestFfStopCount != 1 || g_playerTestFfStartCount != 1 ||
               g_playerTestFfLastIterations != 1 || g_playerTestFfLastStartFlags != 0) {
        result = 10;
    }

    zVec3 remoteEffectPos = {1.0f, 2.0f, 3.0f};
    if (result == 0) {
        zClass_Class::RemoveChild(&runtimeWorld, &projectileSlot.node);

        remoteSaveState.playerState = &remotePlayerState;
        remotePlayerState.rootNode = &remoteRootNode;
        remotePlayerState.storedTargetPos = {1.0f, 2.0f, 7.0f};
        remotePlayerState.projectileSpawnVel = {1.0f, 2.0f, 3.0f};
        remoteController.optCatalogEntry = &remoteEntry;
        remoteEntry.velocity = 0.0f;
    remoteEntry.fireFxSelectedSoundIndex = -1;
    remoteEntry.fireFxSelectedEffectIndex = -1;
    remoteEntry.flyoutSelectedEffectIndex = -1;
    remoteRuntimeWorld.classId = 3;
    remoteProjectileSlot.node.classId = 5;
        remoteProjectileSlot.node.classData = &remoteProjectileData;
        remoteRuntime.projectileNode = &remoteProjectileSlot.node;
        g_OptCatalogRuntimeWorld = &remoteRuntimeWorld;
        g_OptCatalogFreeRuntimeInstanceList = &remoteRuntime;
        g_GameStateOrMapTable = nullptr;
        g_playerTestFfStopCount = 0;
        g_playerTestFfStartCount = 0;

        const int normalizedResult = Player::EnsureGunAuxEffectActive(
            &remoteSaveState, &remoteController, &remoteEffectPos);
        if (normalizedResult != 1) {
            result = 11;
        } else if (remoteEntry.activeRuntimeListHead != &remoteRuntime) {
            result = 12;
        } else if (!Vec3Equals(remoteRuntime.dir, {0.0f, 0.0f, 1.0f})) {
            result = 13;
        } else if (g_playerTestFfStopCount != 0 || g_playerTestFfStartCount != 0) {
            result = 14;
        }
    }

    if (result == 0) {
        zClass_Class::RemoveChild(&remoteRuntimeWorld, &remoteProjectileSlot.node);
        g_OptCatalogFreeRuntimeInstanceList = nullptr;
        remoteEntry.activeRuntimeListHead = nullptr;
        const int failResult = Player::EnsureGunAuxEffectActive(
            &remoteSaveState, &remoteController, &remoteEffectPos);
        if (failResult != 0 || remoteEntry.activeRuntimeListHead != nullptr) {
            result = 15;
        }
    }

    g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    g_OptCatalogNextSpawnScale = oldNextSpawnScale;
    g_OptCatalogNetworkOptionState = oldNetworkOptionState;
    g_OptCatalog_AllocRuntimeGateCallback = oldGateCallback;
    ZOPT_INPUT_JOYSTICK = oldJoystickOption;
    g_zInput_JoystickCaps_ForceFeedback = oldForceFeedbackCaps;
    g_zInputFfEffectSet = oldEffectSet;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    return result;
}

extern "C" int player_update_continuous_alt_gun_fire_controller_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const int oldPrimaryGunDispatchCount = g_HudSensorTracker.primaryGunDispatchCount;
    const float oldNextSpawnScale = g_OptCatalogNextSpawnScale;
    void *const oldPendingSpawnTargetCountPtr = g_OptCatalogPendingSpawnTargetCountPtr;
    void *const oldPendingSpawnTargetListPtr = g_OptCatalogPendingSpawnTargetListPtr;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    PlayerGunFireController controller = {};
    OptCatalogEntryDef owner = {};
    OptCatalogTrailRuntimeState trailRuntime = {};
    zSndSample trailStopSample = {};
    zSndSample trailLoopSample = {};
    zVec3 spawnPos = {1.0f, 2.0f, 3.0f};
    zVec3 spawnDir = {0.0f, 0.0f, 1.0f};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    playerState.activeAltGunController = &controller;
    controller.trailRuntimeState = &trailRuntime;
    owner.trailStopSample = &trailStopSample;
    owner.trailLoopSample = &trailLoopSample;
    trailRuntime.ownerEntry = &owner;
    trailRuntime.spawnPos = &spawnPos;
    trailRuntime.spawnDir = &spawnDir;
    trailRuntime.trailDistance = 9.0f;
    trailRuntime.volumeFadeTimer = 8.0f;
    trailRuntime.alphaPulsePhase = 7.0f;

    modalData.masterType = 2;
    g_HudSensorTracker.primaryGunDispatchCount = 5;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;
    Player::UpdateContinuousAltGunFireController(&saveState);
    if (playerState.queuedFixedDamageFlag != 1 || playerState.altGunFireHeldFlag != 0 ||
        owner.activeTrailRuntime != nullptr || g_HudSensorTracker.primaryGunDispatchCount != 5) {
        g_GameStateOrMapTable = oldGameStateOrMapTable;
        g_HudSensorTracker.primaryGunDispatchCount = oldPrimaryGunDispatchCount;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        g_OptCatalogPendingSpawnTargetCountPtr = oldPendingSpawnTargetCountPtr;
        g_OptCatalogPendingSpawnTargetListPtr = oldPendingSpawnTargetListPtr;
        return 1;
    }

    playerState.queuedFixedDamageFlag = 0;
    playerState.playerOrdinal = 7;
    modalData.masterType = 3;
    g_OptCatalogNextSpawnScale = 2.5f;
    Player::UpdateContinuousAltGunFireController(&saveState);
    const bool firstFireOk =
        playerState.queuedFixedDamageFlag == 0 && playerState.altGunFireHeldFlag == 1 &&
        owner.activeTrailRuntime == &trailRuntime && trailRuntime.trailDistance == 0.0f &&
        trailRuntime.volumeFadeTimer == 0.0f && trailRuntime.alphaPulsePhase == 0.0f &&
        trailRuntime.spawnScale == 2.5f && g_OptCatalogNextSpawnScale == 1.0f &&
        g_HudSensorTracker.primaryGunDispatchCount == 6;

    g_GameStateOrMapTable = nullptr;
    Player::UpdateContinuousAltGunFireController(&saveState);
    const bool heldRemoteOk =
        owner.activeTrailRuntime == &trailRuntime &&
        g_HudSensorTracker.primaryGunDispatchCount == 6;

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_HudSensorTracker.primaryGunDispatchCount = oldPrimaryGunDispatchCount;
    g_OptCatalogNextSpawnScale = oldNextSpawnScale;
    g_OptCatalogPendingSpawnTargetCountPtr = oldPendingSpawnTargetCountPtr;
    g_OptCatalogPendingSpawnTargetListPtr = oldPendingSpawnTargetListPtr;
    return firstFireOk && heldRemoteOk ? 0 : 2;
}

extern "C" int player_alt_gun_projectile_dispatch_helpers_smoke(void) {
    void *const oldFreeRuntimeList = g_OptCatalogFreeRuntimeInstanceList;
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    const float oldNextSpawnScale = g_OptCatalogNextSpawnScale;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    zUtil_SaveGameState tetherSave = {};
    zUtil_PlayerStateStorage tetherPlayer = {};
    PlayerGunFireController tetherController = {};
    OptCatalogEntryDef tetherEntry = {};
    OptCatalogRuntimeInstanceStorage tetherRuntime = {};
    zClass_NodeFreeListSlot tetherProjectile = {};
    zClass_Object3DDataPartial tetherProjectileData = {};
    zClass_NodePartial tetherRoot = {};
    zClass_NodePartial tetherRuntimeWorld = {};
    zClass_NodePartial tetherMount = {};
    tetherSave.playerState = &tetherPlayer;
    tetherPlayer.activeAltGunController = &tetherController;
    tetherPlayer.rootNode = &tetherRoot;
    tetherPlayer.altFireOrigin = {4.0f, 5.0f, 6.0f};
    tetherPlayer.gunFireDir = {0.0f, 0.0f, 1.0f};
    tetherPlayer.projectileSpawnVel = {1.0f, 2.0f, 3.0f};
    tetherController.optCatalogEntry = &tetherEntry;
    tetherController.attachNodePrimary = &tetherMount;
    tetherController.attachState = &tetherRuntime;
    tetherEntry.flags = 1u << 20;
    tetherEntry.fireFxSelectedSoundIndex = -1;
    tetherEntry.fireFxSelectedEffectIndex = -1;
    tetherEntry.flyoutSelectedEffectIndex = -1;
    tetherProjectile.node.classId = 5;
    tetherProjectile.node.classData = &tetherProjectileData;
    tetherRuntime.projectileNode = &tetherProjectile.node;
    tetherRuntimeWorld.classId = 3;
    zClass_Class::AddChild(&tetherMount, &tetherProjectile.node);
    g_OptCatalogRuntimeWorld = &tetherRuntimeWorld;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&tetherSave;
    const int tetherResult = Player::AltGunLaunchProjectile(&tetherSave);
    const bool tetherOk =
        tetherResult == 1 && tetherPlayer.pendingAltCameraToggle == 1 &&
        tetherPlayer.altGunTransitionState == 0x100 &&
        tetherPlayer.altGunTransitionController == &tetherController &&
        tetherController.attachState == &tetherRuntime &&
        tetherEntry.activeRuntimeListHead == &tetherRuntime &&
        Vec3Equals(tetherRuntime.pos, tetherPlayer.altFireOrigin) &&
        Vec3Equals(tetherRuntime.dir, tetherPlayer.gunFireDir) &&
        tetherRuntimeWorld.listCountB == 1 &&
        tetherRuntimeWorld.listB[0] == &tetherProjectile.node && tetherMount.listCountB == 0;

    zUtil_SaveGameState nonTetherSave = {};
    zUtil_PlayerStateStorage nonTetherPlayer = {};
    PlayerGunFireController nonTetherController = {};
    OptCatalogEntryDef nonTetherEntry = {};
    OptCatalogRuntimeInstanceStorage nonTetherRuntime = {};
    zClass_NodeFreeListSlot nonTetherProjectile = {};
    zClass_Object3DDataPartial nonTetherProjectileData = {};
    zClass_NodePartial nonTetherRoot = {};
    zClass_NodePartial nonTetherRuntimeWorld = {};
    zClass_NodePartial nonTetherMount = {};
    nonTetherSave.playerState = &nonTetherPlayer;
    nonTetherPlayer.activeAltGunController = &nonTetherController;
    nonTetherPlayer.rootNode = &nonTetherRoot;
    nonTetherPlayer.altFireOrigin = {7.0f, 8.0f, 9.0f};
    nonTetherPlayer.gunFireDir = {1.0f, 0.0f, 0.0f};
    nonTetherPlayer.projectileSpawnVel = {2.0f, 3.0f, 4.0f};
    nonTetherController.optCatalogEntry = &nonTetherEntry;
    nonTetherController.attachNodePrimary = &nonTetherMount;
    nonTetherController.attachState = &nonTetherRuntime;
    nonTetherController.ammoOrCharge = 2.0f;
    nonTetherEntry.fireFxSelectedSoundIndex = -1;
    nonTetherEntry.fireFxSelectedEffectIndex = -1;
    nonTetherEntry.flyoutSelectedEffectIndex = -1;
    nonTetherProjectile.node.classId = 5;
    nonTetherProjectile.node.classData = &nonTetherProjectileData;
    nonTetherRuntime.projectileNode = &nonTetherProjectile.node;
    nonTetherRuntimeWorld.classId = 3;
    zClass_Class::AddChild(&nonTetherMount, &nonTetherProjectile.node);
    g_OptCatalogRuntimeWorld = &nonTetherRuntimeWorld;
    g_GameStateOrMapTable = nullptr;
    const int nonTetherResult = Player::AltGunLaunchProjectile(&nonTetherSave);
    const bool nonTetherOk =
        nonTetherResult == 1 && nonTetherController.attachState == nullptr &&
        nonTetherPlayer.altGunTransitionState == 2 &&
        nonTetherPlayer.altGunTransitionController == &nonTetherController &&
        nonTetherEntry.activeRuntimeListHead == &nonTetherRuntime &&
        Vec3Equals(nonTetherRuntime.dir, nonTetherPlayer.gunFireDir);

    zUtil_SaveGameState simpleSave = {};
    zUtil_PlayerStateStorage simplePlayer = {};
    PlayerGunFireController simpleController = {};
    OptCatalogEntryDef simpleEntry = {};
    OptCatalogRuntimeInstanceStorage simpleRuntime = {};
    zClass_NodeFreeListSlot simpleProjectile = {};
    zClass_Object3DDataPartial simpleProjectileData = {};
    zClass_NodePartial simpleRoot = {};
    zClass_NodePartial simpleRuntimeWorld = {};
    simpleSave.playerState = &simplePlayer;
    simplePlayer.activeAltGunController = &simpleController;
    simplePlayer.rootNode = &simpleRoot;
    simplePlayer.altFireOrigin = {1.0f, 2.0f, 3.0f};
    simplePlayer.storedTargetPos = {1.0f, 2.0f, 7.0f};
    simplePlayer.projectileSpawnVel = {5.0f, 6.0f, 7.0f};
    simpleController.optCatalogEntry = &simpleEntry;
    simpleEntry.fireFxSelectedSoundIndex = -1;
    simpleEntry.fireFxSelectedEffectIndex = -1;
    simpleEntry.flyoutSelectedEffectIndex = -1;
    simpleProjectile.node.classId = 5;
    simpleProjectile.node.classData = &simpleProjectileData;
    simpleRuntime.projectileNode = &simpleProjectile.node;
    simpleRuntimeWorld.classId = 3;
    g_OptCatalogRuntimeWorld = &simpleRuntimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = &simpleRuntime;
    const int simpleResult = Player::AltGunFireSimpleProjectile(&simpleSave);
    const bool simpleZeroGravityOk =
        simpleResult == 1 && simpleEntry.activeRuntimeListHead == &simpleRuntime &&
        Vec3Equals(simpleRuntime.pos, simplePlayer.altFireOrigin) &&
        Vec3Equals(simpleRuntime.dir, {0.0f, 0.0f, 1.0f});

    zUtil_SaveGameState ballisticSave = {};
    zUtil_PlayerStateStorage ballisticPlayer = {};
    PlayerGunFireController ballisticController = {};
    OptCatalogEntryDef ballisticEntry = {};
    OptCatalogRuntimeInstanceStorage ballisticRuntime = {};
    zClass_NodeFreeListSlot ballisticProjectile = {};
    zClass_Object3DDataPartial ballisticProjectileData = {};
    zClass_NodePartial ballisticRoot = {};
    zClass_NodePartial ballisticRuntimeWorld = {};
    ballisticSave.playerState = &ballisticPlayer;
    ballisticPlayer.activeAltGunController = &ballisticController;
    ballisticPlayer.rootNode = &ballisticRoot;
    ballisticPlayer.altFireOrigin = {3.0f, 4.0f, 5.0f};
    ballisticPlayer.gunFireDir = {0.25f, 0.5f, 0.75f};
    ballisticPlayer.projectileSpawnVel = {1.0f, 1.0f, 1.0f};
    ballisticController.optCatalogEntry = &ballisticEntry;
    ballisticEntry.gravity = 9.0f;
    ballisticEntry.fireFxSelectedSoundIndex = -1;
    ballisticEntry.fireFxSelectedEffectIndex = -1;
    ballisticEntry.flyoutSelectedEffectIndex = -1;
    ballisticProjectile.node.classId = 5;
    ballisticProjectile.node.classData = &ballisticProjectileData;
    ballisticRuntime.projectileNode = &ballisticProjectile.node;
    ballisticRuntimeWorld.classId = 3;
    g_OptCatalogRuntimeWorld = &ballisticRuntimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = &ballisticRuntime;
    const int ballisticResult = Player::AltGunFireSimpleProjectile(&ballisticSave);
    const bool simpleBallisticOk =
        ballisticResult == 1 && ballisticEntry.activeRuntimeListHead == &ballisticRuntime &&
        Vec3Equals(ballisticRuntime.dir, ballisticPlayer.gunFireDir);

    g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    g_OptCatalogNextSpawnScale = oldNextSpawnScale;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    return tetherOk && nonTetherOk && simpleZeroGravityOk && simpleBallisticOk ? 0 : 1;
}

extern "C" int player_process_alt_gun_fire_dispatch_request_smoke(void) {
    void *const oldFreeRuntimeList = g_OptCatalogFreeRuntimeInstanceList;
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    int *const oldJoystickOption = ZOPT_INPUT_JOYSTICK;
    const int oldForceFeedbackCaps = g_zInput_JoystickCaps_ForceFeedback;
    const int oldPrimaryGunDispatchCount = g_HudSensorTracker.primaryGunDispatchCount;

    int joystickEnabled = 0;
    ZOPT_INPUT_JOYSTICK = &joystickEnabled;
    g_zInput_JoystickCaps_ForceFeedback = 0;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerGunFireController controller = {};
    OptCatalogEntryDef entry = {};
    OptCatalogRuntimeInstanceStorage runtime = {};
    zClass_NodeFreeListSlot projectile = {};
    zClass_Object3DDataPartial projectileData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial runtimeWorld = {};
    zClass_NodePartial mountNode = {};
    zClass_NodePartial gunNode = {};
    zClass_NodePartial turretNode = {};
    zClass_Object3DDataPartial gunData = {};
    zClass_Object3DDataPartial turretData = {};

    const zMat4x3 identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    SetObjectLocalMatrix(&gunData, identityMatrix);
    SetObjectLocalMatrix(&turretData, identityMatrix);
    gunNode.classId = 5;
    gunNode.classData = &gunData;
    turretNode.classId = 5;
    turretNode.classData = &turretData;

    saveState.playerState = &playerState;
    playerState.activeAltGunController = &controller;
    playerState.rootNode = &rootNode;
    playerState.gunNode = &gunNode;
    playerState.turretNode = &turretNode;
    playerState.gunFireTransform = identityMatrix;
    playerState.aimBasisOrigin = {0.0f, 0.0f, 0.0f};
    playerState.firePointCenter = {1.0f, 0.0f, 0.0f};
    playerState.gunFireDir = {0.0f, 0.0f, 1.0f};
    playerState.projectileSpawnVel = {2.0f, 3.0f, 4.0f};
    playerState.altGunDispatchRequested = 1;
    controller.optCatalogEntry = &entry;
    controller.attachNodePrimary = &mountNode;
    controller.flags = 1;
    controller.ammoOrCharge = 2.0f;
    entry.gravity = 1.0f;
    entry.fireFxSelectedSoundIndex = -1;
    entry.fireFxSelectedEffectIndex = -1;
    entry.flyoutSelectedEffectIndex = -1;
    projectile.node.classId = 5;
    projectile.node.classData = &projectileData;
    runtime.projectileNode = &projectile.node;
    runtimeWorld.classId = 3;
    g_OptCatalogRuntimeWorld = &runtimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = &runtime;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;
    g_HudSensorTracker.primaryGunDispatchCount = 10;

    Player::ProcessAltGunDispatchRequest(&saveState);
    const bool fireOk =
        playerState.altGunDispatchRequested == 0 && entry.activeRuntimeListHead == &runtime &&
        Vec3Equals(runtime.dir, playerState.gunFireDir) &&
        playerState.altFireSlotCenter.offset == 1.5f && controller.ammoOrCharge == 1.0f &&
        g_HudSensorTracker.primaryGunDispatchCount == 11;

    playerState.altGunFireHeldFlag = 1;
    playerState.altGunDispatchRequested = 9;
    g_HudSensorTracker.primaryGunDispatchCount = 20;
    Player::ProcessAltGunDispatchRequest(&saveState);
    const bool heldOk =
        playerState.altGunDispatchRequested == 9 &&
        g_HudSensorTracker.primaryGunDispatchCount == 21;

    g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    ZOPT_INPUT_JOYSTICK = oldJoystickOption;
    g_zInput_JoystickCaps_ForceFeedback = oldForceFeedbackCaps;
    g_HudSensorTracker.primaryGunDispatchCount = oldPrimaryGunDispatchCount;
    return fireOk && heldOk ? 0 : 1;
}

extern "C" int player_process_primary_gun_dispatch_request_smoke(void) {
    void *const oldFreeRuntimeList = g_OptCatalogFreeRuntimeInstanceList;
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    const float oldNextSpawnScale = g_OptCatalogNextSpawnScale;
    const int oldNetworkOptionState = g_OptCatalogNetworkOptionState;
    OptCatalogAllocRuntimeGateCallback const oldGateCallback =
        g_OptCatalog_AllocRuntimeGateCallback;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    int *const oldJoystickOption = ZOPT_INPUT_JOYSTICK;
    const int oldForceFeedbackCaps = g_zInput_JoystickCaps_ForceFeedback;
    const int oldPrimaryGunDispatchCount = g_HudSensorTracker.primaryGunDispatchCount;

    int joystickEnabled = 0;
    ZOPT_INPUT_JOYSTICK = &joystickEnabled;
    g_zInput_JoystickCaps_ForceFeedback = 0;
    g_OptCatalogNetworkOptionState = 0;
    g_OptCatalog_AllocRuntimeGateCallback = nullptr;
    g_OptCatalogNextSpawnScale = 1.0f;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    PlayerGunFireController controller = {};
    OptCatalogEntryDef entry = {};
    OptCatalogRuntimeInstanceStorage runtime = {};
    zClass_NodeFreeListSlot projectile = {};
    zClass_Object3DDataPartial projectileData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial runtimeWorld = {};
    zClass_NodePartial mountNode = {};
    zClass_NodePartial gunNode = {};
    zClass_NodePartial turretNode = {};
    zClass_Object3DDataPartial gunData = {};
    zClass_Object3DDataPartial turretData = {};

    const zMat4x3 identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    SetObjectLocalMatrix(&gunData, identityMatrix);
    SetObjectLocalMatrix(&turretData, identityMatrix);
    gunNode.classId = 5;
    gunNode.classData = &gunData;
    turretNode.classId = 5;
    turretNode.classData = &turretData;

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    modalData.masterType = 3;
    playerState.activePrimaryGunController = &controller;
    playerState.rootNode = &rootNode;
    playerState.gunNode = &gunNode;
    playerState.turretNode = &turretNode;
    playerState.gunFireTransform = identityMatrix;
    playerState.aimBasisOrigin = {0.0f, 0.0f, 0.0f};
    playerState.firePointCenter = {1.0f, 0.0f, 0.0f};
    playerState.gunFireDir = {0.0f, 0.0f, 1.0f};
    playerState.usePresetGunFireDir = 1;
    playerState.projectileSpawnVel = {2.0f, 3.0f, 4.0f};
    controller.optCatalogEntry = &entry;
    controller.attachNodePrimary = &mountNode;
    controller.flags = 1;
    controller.ammoOrCharge = 2.0f;
    entry.velocity = 5.0f;
    entry.fireFxSelectedSoundIndex = -1;
    entry.fireFxSelectedEffectIndex = -1;
    entry.flyoutSelectedEffectIndex = -1;
    projectile.node.classId = 5;
    projectile.node.classData = &projectileData;
    runtime.projectileNode = &projectile.node;
    runtimeWorld.classId = 3;
    g_OptCatalogRuntimeWorld = &runtimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = &runtime;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;
    g_HudSensorTracker.primaryGunDispatchCount = 10;

    playerState.primaryGunDispatchRequested = 0;
    Player::ProcessPrimaryGunDispatchTick(&saveState);
    const bool noDispatchOk =
        playerState.primaryGunDispatchRequested == 0 && controller.ammoOrCharge == 2.0f &&
        entry.activeRuntimeListHead == nullptr && g_OptCatalogFreeRuntimeInstanceList == &runtime &&
        g_HudSensorTracker.primaryGunDispatchCount == 10;

    playerState.primaryGunDispatchRequested = 1;
    Player::ProcessPrimaryGunDispatchTick(&saveState);
    const bool fireOk =
        playerState.primaryGunDispatchRequested == 0 && controller.ammoOrCharge == 1.0f &&
        entry.activeRuntimeListHead == &runtime &&
        Vec3Equals(playerState.primaryFireOrigin, {1.0f, 0.0f, 0.0f}) &&
        Vec3Equals(runtime.pos, playerState.primaryFireOrigin) &&
        Vec3Equals(runtime.dir, playerState.gunFireDir) &&
        playerState.altFireSlotCenter.offset == 1.5f &&
        g_HudSensorTracker.primaryGunDispatchCount == 11;

    g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    g_OptCatalogNextSpawnScale = oldNextSpawnScale;
    g_OptCatalogNetworkOptionState = oldNetworkOptionState;
    g_OptCatalog_AllocRuntimeGateCallback = oldGateCallback;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    ZOPT_INPUT_JOYSTICK = oldJoystickOption;
    g_zInput_JoystickCaps_ForceFeedback = oldForceFeedbackCaps;
    g_HudSensorTracker.primaryGunDispatchCount = oldPrimaryGunDispatchCount;
    return noDispatchOk && fireOk ? 0 : 1;
}

extern "C" int player_tick_alt_gun_runtime_state_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const float oldFrameDelta = g_FrameDeltaTimeSec;
    const int oldPrimaryGunDispatchCount = g_HudSensorTracker.primaryGunDispatchCount;
    void *const oldPendingSpawnTargetCountPtr = g_OptCatalogPendingSpawnTargetCountPtr;
    void *const oldPendingSpawnTargetListPtr = g_OptCatalogPendingSpawnTargetListPtr;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerGunFireController controller = {};
    OptCatalogEntryDef entry = {};
    OptCatalogTrailRuntimeState trailRuntime = {};

    saveState.playerState = &playerState;
    playerState.activeAltGunController = &controller;
    playerState.altGunTransitionState = 1;
    playerState.altGunDispatchRequested = 1;
    playerState.altGunFireHeldFlag = 1;
    playerState.worldPos = {4.0f, 5.0f, 6.0f};
    playerState.steerBasisRaw = {0.0f, 0.0f, 1.0f};
    controller.optCatalogEntry = &entry;
    controller.trailRuntimeState = &trailRuntime;
    controller.ammoOrCharge = 2.0f;
    entry.fireRateInterval = 2.0f;

    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;
    g_FrameDeltaTimeSec = 0.5f;
    g_HudSensorTracker.primaryGunDispatchCount = 30;
    g_OptCatalogPendingSpawnTargetCountPtr = (void *)1;
    g_OptCatalogPendingSpawnTargetListPtr = (void *)1;

    Player::TickAltGunRuntimeState(&saveState);
    const bool heldAmmoOk =
        playerState.altGunDispatchRequested == 1 && playerState.altGunFireHeldFlag == 1 &&
        FloatNear(controller.ammoOrCharge, 1.75f) &&
        FloatNear(trailRuntime.ammoOrChargeMirror, 1.75f) &&
        g_HudSensorTracker.primaryGunDispatchCount == 31 &&
        g_OptCatalogPendingSpawnTargetCountPtr == nullptr &&
        g_OptCatalogPendingSpawnTargetListPtr == nullptr;

    zUtil_SaveGameState remoteSave = {};
    zUtil_PlayerStateStorage remotePlayer = {};
    PlayerGunFireController remoteController = {};
    zClass_NodePartial doorLeft = {};
    zClass_NodePartial doorRight = {};
    zClass_Object3DDataPartial doorLeftData = {};
    zClass_Object3DDataPartial doorRightData = {};
    remoteSave.playerState = &remotePlayer;
    remotePlayer.activeAltGunController = &remoteController;
    remotePlayer.altGunTransitionState = 8;
    remotePlayer.altGunTransitionTimerB = 0.2f;
    remotePlayer.doorLeftNode = &doorLeft;
    remotePlayer.doorRightNode = &doorRight;
    doorLeft.classId = 5;
    doorLeft.classData = &doorLeftData;
    doorRight.classId = 5;
    doorRight.classData = &doorRightData;
    g_GameStateOrMapTable = nullptr;
    g_FrameDeltaTimeSec = 0.1f;
    Player::TickAltGunRuntimeState(&remoteSave);
    const bool doorOpenOk =
        remotePlayer.altGunTransitionState == 16 &&
        remotePlayer.altGunTransitionTimerB == 0.0f &&
        doorLeftData.scale.x == 1.0f && doorLeftData.scale.y == 1.0f &&
        doorRightData.scale.x == 1.0f && doorRightData.scale.z == 1.0f &&
        g_OptCatalogPendingSpawnTargetCountPtr == nullptr &&
        g_OptCatalogPendingSpawnTargetListPtr == nullptr;

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_FrameDeltaTimeSec = oldFrameDelta;
    g_HudSensorTracker.primaryGunDispatchCount = oldPrimaryGunDispatchCount;
    g_OptCatalogPendingSpawnTargetCountPtr = oldPendingSpawnTargetCountPtr;
    g_OptCatalogPendingSpawnTargetListPtr = oldPendingSpawnTargetListPtr;
    return heldAmmoOk && doorOpenOk ? 0 : 1;
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

extern "C" int player_reset_damage_visuals_and_timed_status_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const float oldTime = g_Time_AccumulatedTimeSec;
    const float oldStatusMeterRatio = g_PlayerStatusMeterRatio;
    const float oldLowMeterNextBeepTime = g_Hud_LowMeterNextBeepTime;
    const float oldLowMeterBeepInterval = g_Hud_LowMeterBeepInterval;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    zClass_NodePartial rootNode = {};
    saveState.playerState = &playerState;
    playerState.masterCommonData = &commonData;
    playerState.rootNode = &rootNode;
    playerState.statusMeterValue = 10.0f;
    playerState.recentHitValid = 1;
    playerState.recentHitFxExpireTime = 5.0f;
    playerState.recentHitLightHandle = nullptr;

    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;
    g_Time_AccumulatedTimeSec = 10.0f;
    g_PlayerStatusMeterRatio = 0.2f;
    g_Hud_LowMeterNextBeepTime = 20.0f;
    g_Hud_LowMeterBeepInterval = 3.0f;

    Player::ResetDamageVisualsAndTimedStatus(&saveState);

    const bool cleanupOk =
        playerState.recentHitValid == 0 && playerState.recentHitLightHandle == nullptr &&
        g_Hud_LowMeterNextBeepTime == 20.0f;

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Time_AccumulatedTimeSec = oldTime;
    g_PlayerStatusMeterRatio = oldStatusMeterRatio;
    g_Hud_LowMeterNextBeepTime = oldLowMeterNextBeepTime;
    g_Hud_LowMeterBeepInterval = oldLowMeterBeepInterval;
    return cleanupOk ? 0 : 1;
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

extern "C" int player_clear_respawn_transition_flag_callback_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;
    playerState.cameraTransitionTimer = 17;

    Player::ClearRespawnTransitionFlagCallback(&saveState);
    return playerState.cameraTransitionTimer == 0 ? 0 : 1;
}

extern "C" int player_destroyed_state_respawn_callback_smoke(void) {
    zVidTexturePackEntry *const oldBuiltinTexturePacks = g_zVid_BuiltinTexturePacks;
    const int oldBuiltinTexturePackCount = g_zVid_BuiltinTexturePackCount;
    const int oldTexDirEntryCount = g_zImage_TexDirEntryCount;

    zClass_Object3D_ModelRefLerpQueue::Reset();

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial healthyNode = {};
    zClass_NodePartial *rootChildren[] = {&healthyNode};
    zClass_Object3DDataPartial rootData = {};
    zClass_Object3DDataPartial healthyData = {};
    saveState.playerState = &playerState;
    playerState.masterCommonData = &commonData;
    playerState.rootNode = &rootNode;
    std::strcpy(rootNode.name, "root");
    std::strcpy(healthyNode.name, "healthy");
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    rootNode.listCountB = 1;
    rootNode.listB = rootChildren;
    healthyNode.classId = 5;
    healthyNode.classData = &healthyData;
    healthyNode.flags = 0x01;
    healthyData.localMatrix[9] = 4.0f;
    healthyData.localMatrix[10] = 5.0f;
    healthyData.localMatrix[11] = 6.0f;
    healthyData.rotation = {7.0f, 8.0f, 9.0f};
    commonData.maxHealth = 90.0f;
    playerState.statusMeterValue = 12.0f;
    playerState.damageProtectionActive = 3;
    playerState.queuedFixedDamageFlag = 4;
    playerState.damageVisualFlag = 5;
    playerState.cachedAltSelectionCode = 201;
    playerState.cachedPrimarySelectionCode = 100;

    zVidTexturePackEntry builtinTexturePack = {};
    builtinTexturePack.fileHandle = std::tmpfile();
    if (builtinTexturePack.fileHandle == nullptr) {
        g_zVid_BuiltinTexturePacks = oldBuiltinTexturePacks;
        g_zVid_BuiltinTexturePackCount = oldBuiltinTexturePackCount;
        g_zImage_TexDirEntryCount = oldTexDirEntryCount;
        return 2;
    }
    g_zVid_BuiltinTexturePacks = &builtinTexturePack;
    g_zVid_BuiltinTexturePackCount = 1;
    g_zImage_TexDirEntryCount = 0;

    Player::DestroyedStateRespawnCallback(nullptr, &saveState, 0);

    zClass_Object3D_ModelRefLerpTask *const fadeTask = g_ModelRefLerpQueueState.head;
    const bool objectOk =
        (rootData.flags & 0x02) != 0 && rootData.alphaScale == 0.0f &&
        g_ModelRefLerpQueueState.count == 1 && fadeTask != nullptr &&
        fadeTask->node == &rootNode && fadeTask->callbackCtx == &saveState &&
        fadeTask->onComplete == (void *)(&Player::ClearRespawnTransitionFlagCallback) &&
        fadeTask->currentModelRef == 0.0f && fadeTask->targetModelRef == 1.0f &&
        FloatNear(fadeTask->modelRefDeltaPerSec, 0.2f) &&
        healthyData.localMatrix[9] == 0.0f && healthyData.localMatrix[10] == 0.0f &&
        healthyData.localMatrix[11] == 0.0f && healthyData.rotation.x == 0.0f &&
        healthyData.rotation.y == 0.0f && healthyData.rotation.z == 0.0f;
    const bool stateOk =
        playerState.damageProtectionActive == 0 && playerState.queuedFixedDamageFlag == 0 &&
        playerState.damageVisualFlag == 0 && playerState.statusMeterValue == 90.0f &&
        playerState.cachedAltSelectionCode == 0 && playerState.cachedPrimarySelectionCode == 0;

    const bool ok = objectOk && stateOk;
    zClass_Object3D_ModelRefLerpQueue::Reset();
    g_zVid_BuiltinTexturePacks = oldBuiltinTexturePacks;
    g_zVid_BuiltinTexturePackCount = oldBuiltinTexturePackCount;
    g_zImage_TexDirEntryCount = oldTexDirEntryCount;
    return ok ? 0 : 1;
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

extern "C" int player_remove_all_deployed_mines_smoke(void) {
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    void *const oldFreeRuntimeList = g_OptCatalogFreeRuntimeInstanceList;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    zClass_NodePartial rootNode = {};
    playerState.rootNode = &rootNode;

    OptCatalogEntryDef ignoredEntry = {};
    OptCatalogEntryDef mineEntries[4] = {};
    OptCatalogRuntimeInstanceStorage ignoredRuntime = {};
    OptCatalogRuntimeInstanceStorage mineRuntimes[4] = {};
    zClass_NodeFreeListSlot ignoredProjectile = {};
    zClass_NodeFreeListSlot mineProjectiles[4] = {};
    zClass_Object3DDataPartial ignoredData = {};
    zClass_Object3DDataPartial mineData[4] = {};
    zClass_NodePartial runtimeWorld = {};
    zClass_NodePartial *worldChildren[5] = {
        &ignoredProjectile.node,
        &mineProjectiles[0].node,
        &mineProjectiles[1].node,
        &mineProjectiles[2].node,
        &mineProjectiles[3].node,
    };

    runtimeWorld.classId = 3;
    runtimeWorld.listB = worldChildren;
    runtimeWorld.listCountB = 5;
    ignoredProjectile.node.classId = 5;
    ignoredProjectile.node.classData = &ignoredData;
    ignoredRuntime.ownerNode = &rootNode;
    ignoredRuntime.projectileNode = &ignoredProjectile.node;
    ignoredRuntime.lifetime = 0.0f;
    ignoredEntry.activeRuntimeListHead = &ignoredRuntime;
    playerState.altWeaponBanks[3].controllerA.optCatalogEntry = &ignoredEntry;

    for (int index = 0; index < 4; ++index) {
        mineProjectiles[index].node.classId = 5;
        mineProjectiles[index].node.classData = &mineData[index];
        mineRuntimes[index].ownerNode = &rootNode;
        mineRuntimes[index].projectileNode = &mineProjectiles[index].node;
        mineRuntimes[index].lifetime = 0.0f;
        mineEntries[index].activeRuntimeListHead = &mineRuntimes[index];
    }

    playerState.altWeaponBanks[4].controllerA.optCatalogEntry = &mineEntries[0];
    playerState.altWeaponBanks[4].controllerB.optCatalogEntry = &mineEntries[1];
    playerState.altWeaponBanks[5].controllerA.optCatalogEntry = &mineEntries[2];
    playerState.altWeaponBanks[5].controllerB.optCatalogEntry = &mineEntries[3];

    OptCatalogRuntimeInstanceStorage freeSentinel = {};
    g_OptCatalogRuntimeWorld = &runtimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = &freeSentinel;

    Player::RemoveAllDeployedMines(&saveState);

    const bool ignoredOk = ignoredEntry.activeRuntimeListHead == &ignoredRuntime &&
                           ignoredRuntime.next == nullptr &&
                           ignoredProjectile.node.listCountA == 0;
    const bool minesCleared =
        mineEntries[0].activeRuntimeListHead == nullptr &&
        mineEntries[1].activeRuntimeListHead == nullptr &&
        mineEntries[2].activeRuntimeListHead == nullptr &&
        mineEntries[3].activeRuntimeListHead == nullptr;
    const bool freeListOk =
        g_OptCatalogFreeRuntimeInstanceList == &mineRuntimes[3] &&
        mineRuntimes[3].next == &mineRuntimes[2] &&
        mineRuntimes[2].next == &mineRuntimes[1] &&
        mineRuntimes[1].next == &mineRuntimes[0] &&
        mineRuntimes[0].next == &freeSentinel;
    const bool worldOk = runtimeWorld.listCountB == 1 && runtimeWorld.listB[0] == &ignoredProjectile.node;

    int failure = 0;
    if (!ignoredOk) {
        failure = 1;
    } else if (!minesCleared) {
        failure = 2;
    } else if (!freeListOk) {
        failure = 3;
    } else if (!worldOk) {
        failure = 4;
    }

    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
    return failure;
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

extern "C" int player_capture_current_object_pose_as_restart_anchor_smoke(void) {
    zUtil_SaveGameState *const oldLocalSaveState = g_LocalPlayerSaveState;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;

    zClass_Object3DDataPartial objectData = {};
    objectData.localMatrix[9] = -8.0f;
    objectData.localMatrix[10] = 9.5f;
    objectData.localMatrix[11] = 12.25f;
    objectData.rotation = zVec3_Make(0.25f, 1.75f, -0.5f);
    zClass_NodePartial rootNode = {};
    rootNode.classId = 5;
    rootNode.classData = &objectData;

    zUtil_PlayerStateStorage localPlayerState = {};
    localPlayerState.rootNode = &rootNode;
    zUtil_SaveGameState localSaveState = {};
    localSaveState.playerState = &localPlayerState;

    zUtil_PlayerStateStorage targetPlayerState = {};
    zUtil_SaveGameState targetSaveState = {};
    targetSaveState.playerState = &targetPlayerState;

    g_VariantTag_Current.count = 2;
    g_VariantTag_Current.tags[0] = 1;
    g_VariantTag_Current.tags[1] = 2;
    g_VariantTag_Current.tags[2] = 3;
    g_Variant_CurrentTag = g_VariantTag_Current;
    g_LocalPlayerSaveState = &localSaveState;

    Player::CaptureCurrentObjectPoseAsRestartAnchor(&targetSaveState);

    const zVec3 expectedPosition = zVec3_Make(-8.0f, 9.5f, 12.25f);
    const bool ok = Vec3Equals(targetPlayerState.worldPos, expectedPosition) &&
                    targetPlayerState.previousTransform.posX == expectedPosition.x &&
                    targetPlayerState.previousTransform.posY == expectedPosition.y &&
                    targetPlayerState.previousTransform.posZ == expectedPosition.z &&
                    targetPlayerState.restartYawRad == 1.75f &&
                    targetPlayerState.variantTag.count == 0 &&
                    g_VariantTag_Current.count == 0 && g_Variant_CurrentTag.count == 0;

    g_LocalPlayerSaveState = oldLocalSaveState;
    g_VariantTag_Current = oldVariantTagCurrent;
    g_Variant_CurrentTag = oldVariantCurrent;
    return ok ? 0 : 1;
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

extern "C" int player_register_gameplay_callbacks_and_ff_smoke(void) {
    zInput_BindMapContext *const oldBindMap = g_zInput_BindMap_Current;
    int *const oldAcceleration = ZOPT_VIDEO_ACCELERATION;
    zInput_FFEffectSet *const oldEffectSet = g_zInputFfEffectSet;
    zInput::DIDevice *const oldJoystickDevice = g_zInput_JoystickDevice;

    zInput::Keyboard_ClearKeyCallbackTable();
    zInput_BindMapContext context = {};
    context.InitCommandMap(46);
    g_zInput_BindMap_Current = &context;

    const int commandIds[] = {9, 30, 32, 33, 34, 35, 42, 43, 44, 45};
    for (int index = 0; index < (int)(sizeof(commandIds) / sizeof(commandIds[0])); ++index) {
        const int commandId = commandIds[index];
        zInput::BindMap_Current_SetBindingRecord(commandId, "Gameplay", 0x100 + index, 0, 0, 0);
    }

    int acceleration = 0;
    ZOPT_VIDEO_ACCELERATION = &acceleration;
    g_zInput_JoystickDevice = nullptr;
    g_zInputFfEffectSet = nullptr;

    const zInputCommandCallbackFn hudCallback =
        (zInputCommandCallbackFn)(HudUi::HandleHotkeyCommand);
    const zInputCommandCallbackFn videoCallback =
        (zInputCommandCallbackFn)(zVideo::HandleSoftwareModeHotkeyCommand);

    Player::RegisterGameplayCommandCallbacksAndCreateFfEffects();

    int failureCode = 0;
    if (context.m_commandCallbacks[30] != hudCallback ||
        context.m_commandCallbacks[9] != hudCallback ||
        context.m_commandCallbacks[32] != hudCallback ||
        context.m_commandCallbacks[33] != hudCallback ||
        context.m_commandCallbacks[35] != hudCallback ||
        context.m_commandCallbacks[42] != hudCallback ||
        context.m_commandCallbacks[43] != hudCallback ||
        context.m_commandCallbacks[44] != hudCallback ||
        context.m_commandCallbacks[45] != hudCallback) {
        failureCode = 1;
    } else if (context.m_commandCallbacks[34] != videoCallback) {
        failureCode = 2;
    } else if (g_zInputFfEffectSet == nullptr ||
               g_zInputFfEffectSet->PrimaryFire != nullptr ||
               g_zInputFfEffectSet->PitchForce != nullptr) {
        failureCode = 3;
    } else if (g_zInputKbdKeyDispatchTable[0x100].callback == nullptr ||
               g_zInputKbdKeyDispatchTable[0x109].callback == nullptr) {
        failureCode = 4;
    }

    delete g_zInputFfEffectSet;
    g_zInputFfEffectSet = nullptr;
    context.FreeNonOwnedBuffers();

    if (failureCode == 0) {
        zInput::Keyboard_ClearKeyCallbackTable();
        zInput_BindMapContext hwContext = {};
        hwContext.InitCommandMap(46);
        g_zInput_BindMap_Current = &hwContext;
        for (int index = 0; index < (int)(sizeof(commandIds) / sizeof(commandIds[0])); ++index) {
            const int commandId = commandIds[index];
            zInput::BindMap_Current_SetBindingRecord(commandId, "Gameplay", 0x120 + index, 0, 0, 0);
        }

        acceleration = 1;
        Player::RegisterGameplayCommandCallbacksAndCreateFfEffects();
        if (hwContext.m_commandCallbacks[34] != nullptr ||
            hwContext.m_commandCallbacks[35] != hudCallback ||
            g_zInputFfEffectSet == nullptr) {
            failureCode = 5;
        }

        delete g_zInputFfEffectSet;
        g_zInputFfEffectSet = nullptr;
        hwContext.FreeNonOwnedBuffers();
    }

    g_zInput_BindMap_Current = oldBindMap;
    ZOPT_VIDEO_ACCELERATION = oldAcceleration;
    g_zInputFfEffectSet = oldEffectSet;
    g_zInput_JoystickDevice = oldJoystickDevice;
    zInput::Keyboard_ClearKeyCallbackTable();
    FreePlayerTestOptionList();
    return failureCode;
}

extern "C" int player_toggle_steering_mode_and_reset_mouse_look_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    int *const oldGameControlOptions = ZOPT_GAME_CONTROL_OPTIONS;
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
    int gameControlOptions = 0;
    saveState.playerState = &playerState;
    playerState.thirdPersonYawOffset = 13.0f;
    playerState.cameraElevationOffset = -4.0f;

    g_GameStateOrMapTable =
        reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);
    ZOPT_GAME_CONTROL_OPTIONS = &gameControlOptions;
    g_zInput_hWnd = hwnd;
    g_zInput_MouseClientCenterX = 31;
    g_zInput_MouseClientCenterY = 29;
    g_zInput_MouseStateSnapshot.cursorClientX = 5;
    g_zInput_MouseStateSnapshot.cursorClientY = 7;
    g_zInput_MouseStateSnapshot.cursorNormX = 0.5f;
    g_zInput_MouseStateSnapshot.cursorNormY = -0.25f;

    Player::ToggleSteeringModeAndResetMouseLook();
    const bool enabledOk =
        (gameControlOptions & 0x02) != 0 && zOpt::GetSteeringMode() == 1 &&
        playerState.thirdPersonYawOffset == 0.0f && playerState.cameraElevationOffset == 0.0f &&
        g_zInput_MouseStateSnapshot.cursorClientX == 31 &&
        g_zInput_MouseStateSnapshot.cursorClientY == 29 &&
        g_zInput_MouseStateSnapshot.cursorNormX == 0.0f &&
        g_zInput_MouseStateSnapshot.cursorNormY == 0.0f;

    playerState.thirdPersonYawOffset = 2.0f;
    playerState.cameraElevationOffset = 3.0f;
    g_zInput_MouseStateSnapshot.cursorClientX = 9;
    g_zInput_MouseStateSnapshot.cursorClientY = 11;
    g_zInput_MouseStateSnapshot.cursorNormX = 0.75f;
    g_zInput_MouseStateSnapshot.cursorNormY = 0.25f;
    Player::ToggleSteeringModeAndResetMouseLook();

    const bool disabledOk =
        (gameControlOptions & 0x02) == 0 && zOpt::GetSteeringMode() == 0 &&
        playerState.thirdPersonYawOffset == 0.0f && playerState.cameraElevationOffset == 0.0f &&
        g_zInput_MouseStateSnapshot.cursorClientX == 31 &&
        g_zInput_MouseStateSnapshot.cursorClientY == 29 &&
        g_zInput_MouseStateSnapshot.cursorNormX == 0.0f &&
        g_zInput_MouseStateSnapshot.cursorNormY == 0.0f;

    DestroyWindow(hwnd);
    SetCursorPos(originalCursor.x, originalCursor.y);
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    ZOPT_GAME_CONTROL_OPTIONS = oldGameControlOptions;
    g_zInput_hWnd = oldWindow;
    g_zInput_MouseClientCenterX = oldCenterX;
    g_zInput_MouseClientCenterY = oldCenterY;
    g_zInput_MouseStateSnapshot = oldMouseState;

    return enabledOk && disabledOk ? 0 : 2;
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

extern "C" int player_set_auto_turn_target_dir_from_world_point_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    playerState.worldPos = {2.0f, 10.0f, 3.0f};
    playerState.autoTurnActive = 0;
    playerState.autoTurnTargetDir = {9.0f, 9.0f, 9.0f};
    const zVec3 worldPoint = {5.0f, 99.0f, 7.0f};

    Player::SetAutoTurnTargetDirFromWorldPoint(&saveState, &worldPoint);

    return playerState.autoTurnActive == 1 &&
                   FloatNear(playerState.autoTurnTargetDir.x, 0.6f) &&
                   playerState.autoTurnTargetDir.y == 0.0f &&
                   FloatNear(playerState.autoTurnTargetDir.z, 0.8f)
               ? 0
               : 1;
}

extern "C" int player_tick_local_player_controls_smoke(void) {
    int *const oldJoystickOption = ZOPT_INPUT_JOYSTICK;
    int *const oldGameControlOptions = ZOPT_GAME_CONTROL_OPTIONS;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    zInput_BindMapContext *const oldBindMap = g_zInput_BindMap_Current;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    zOpt_ViewRectSection **const oldRenderOption = g_zOpt_RenderSectionOption;
    zOpt_ViewRectSection **const oldDisplayOption = g_zOpt_DisplaySectionOption;
    zOpt_ViewRectSection *const oldRenderSection =
        g_zOpt_RenderSectionOption != nullptr ? *g_zOpt_RenderSectionOption : nullptr;
    zOpt_ViewRectSection *const oldDisplaySection =
        g_zOpt_DisplaySectionOption != nullptr ? *g_zOpt_DisplaySectionOption : nullptr;
    std::int32_t *const oldReplicateOption = ZOPT_REPLICATE;
    zClass_TypeListLink *const oldTypeListHead0 = zClass_TypeList::Head(0);
    const int oldLocalControlEnabled = g_Player_LocalControlEnabled;
    const int oldRuntimeInputFlags = g_Player_RuntimeInputFlags;
    const float oldCameraZone = g_Player_CameraZone;
    const float oldCameraZoneInvRange = g_Player_CameraZoneInvRange;
    const float oldGameplayStepScale = g_Player_GameplayInputStepScale;

    zInput_BindMapContext context = {};
    context.InitCommandMap(41);
    g_zInput_BindMap_Current = &context;
    zInput::BindMap_Current_SetBindingRecord(4, "Throttle Up", 0x20, 0, 0, 0);
    zInput::BindMap_Current_SetBindingRecord(13, "Alt Trigger", 0x21, 0, 0, 0);
    g_zInputKbdKeyDispatchTable[0x20].state = 1;
    g_zInputKbdKeyDispatchTable[0x21].state = 1;

    int joystickEnabled = 0;
    int gameControlOptions = 0;
    int networkEnabled = 1;
    ZOPT_INPUT_JOYSTICK = &joystickEnabled;
    ZOPT_GAME_CONTROL_OPTIONS = &gameControlOptions;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_Player_LocalControlEnabled = 1;
    g_Player_RuntimeInputFlags = 2;
    g_Player_CameraZone = 0.5f;
    g_Player_CameraZoneInvRange = 2.0f;
    g_Player_GameplayInputStepScale = 0.03f;
    g_FrameDeltaTimeSec = 0.25f;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    modalData.masterType = 2;
    playerState.localVel.z = 12.0f;
    playerState.cursorNormX = 0.25f;
    playerState.cursorNormY = 0.25f;
    playerState.playerOrdinal = 1;

    OptCatalogEntryDef optEntry = {};
    optEntry.range = 50.0f;
    OptCatalogRuntimeInstanceStorage attachState = {};
    zClass_NodePartial projectileNode = {};
    projectileNode.flags = 0x10;
    attachState.projectileNode = &projectileNode;
    PlayerGunFireController altGun = {};
    PlayerGunFireController primaryGun = {};
    altGun.optCatalogEntry = &optEntry;
    altGun.attachState = &attachState;
    primaryGun.optCatalogEntry = &optEntry;
    playerState.activeAltGunController = &altGun;
    playerState.activePrimaryGunController = &primaryGun;

    zClass_NodePartial rootNode = {};
    rootNode.flags = 0x10;
    playerState.rootNode = &rootNode;

    zInput_GameStateOrMapTablePartial gameState = {};
    gameState.playerState = reinterpret_cast<zInput_PlayerStatePartial *>(&playerState);
    g_GameStateOrMapTable = &gameState;

    zVidImagePartial currentImage = {};
    currentImage.width = 10;
    currentImage.height = 8;
    zVidImagePartial missImage = {};
    g_HudUiMgrReticleWidget = {};
    g_HudUiMgrReticleWidget.ftable = &g_HudUiWidget_FTable;
    g_HudUiMgrReticleWidget.image = &currentImage;
    g_HudUiMgrReticleWidgetHalfW = 5;
    g_HudUiMgrReticleWidgetHalfH = 4;
    g_HudUiMgrReticleMapBiasX = 10.0f;
    g_HudUiMgrReticleMapBiasY = 20.0f;
    g_HudUiMgrReticleMapScaleHalfW = 100.0f;
    g_HudUiMgrReticleMapScaleHalfH = 50.0f;
    g_HudUiMgrReticleImages[0] = nullptr;
    g_HudUiMgrReticleImages[1] = &missImage;
    g_HudUiMgrReticleImages[2] = nullptr;
    g_HudLayoutHW.reticleClipInitFlags = 1;
    g_HudLayoutHW.reticleClipRect = {};

    zOpt_ViewRectSection renderSection = {};
    renderSection.x = 0;
    renderSection.y = 0;
    renderSection.rightExclusive = 100;
    renderSection.bottomExclusive = 80;
    zOpt_ViewRectSection displaySection = {};
    displaySection.x = 0;
    displaySection.y = 0;
    displaySection.rightExclusive = 200;
    displaySection.bottomExclusive = 150;
    zOpt_ViewRectSection *renderSectionPtr = &renderSection;
    zOpt_ViewRectSection *displaySectionPtr = &displaySection;
    g_zOpt_RenderSectionOption = &renderSectionPtr;
    g_zOpt_DisplaySectionOption = &displaySectionPtr;
    std::int32_t replicate = 0;
    ZOPT_REPLICATE = &replicate;

    zMath::g_zMath_CameraScratchA = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                     0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    g_zMath_ProjOffsetX = 0.0f;
    g_zMath_ProjOffsetY = 0.0f;
    g_zMath_InvProjScaleX = 0.0f;
    g_zMath_InvProjScaleY = 0.0f;

    zClass_CameraDataPartial cameraData = {};
    cameraData.nearClip = 2.0f;
    zClass_NodePartial cameraNode = {};
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    g_MainCamera = &cameraNode;

    zClass_WorldDataPartial worldData = {};
    zClass_NodePartial runtimeScene = {};
    runtimeScene.classData = &worldData;
    g_Player_RuntimeDiScene = &runtimeScene;
    zClass_TypeList::Head(0) = nullptr;
    g_HudUiMgrSensorBlock.sensorClampHalfW = 10.0f;
    g_HudUiMgrSensorBlock.sensorClampHalfH = 12.0f;
    g_zInput_MouseStateSnapshot.cursorNormX = 0.75f;
    g_zInput_MouseStateSnapshot.cursorNormY = -0.75f;
    g_HudSensorTracker.raceCheckpointMode = 1;
    g_HudTimerPanelNetState.tenSecondWarningsEnabled = 0;

    Player::TickLocalPlayerControls(&saveState);

    const bool ok =
        playerState.throttleInput == 1.0f && playerState.throttleInputCopy == 1.0f &&
        FloatNear(playerState.steeringInput, -0.5f) &&
        FloatNear(playerState.steeringInputCopy, -0.5f) &&
        FloatNear(playerState.subPitchInput, 0.5f) &&
        FloatNear(playerState.subPitchInputCopy, 0.5f) &&
        playerState.subVerticalInput == 0.0f && playerState.altGunDispatchRequested == 0 &&
        playerState.primaryGunDispatchRequested == 0 &&
        playerState.altGunTriggerProcessFlag == 1 &&
        g_HudTimerPanelNetState.tenSecondWarningsEnabled == 1;

    zClass_TypeList::Head(0) = oldTypeListHead0;
    g_GameStateOrMapTable = oldGameState;
    g_MainCamera = oldMainCamera;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_zOpt_RenderSectionOption = oldRenderOption;
    g_zOpt_DisplaySectionOption = oldDisplayOption;
    if (g_zOpt_RenderSectionOption != nullptr) {
        *g_zOpt_RenderSectionOption = oldRenderSection;
    }
    if (g_zOpt_DisplaySectionOption != nullptr) {
        *g_zOpt_DisplaySectionOption = oldDisplaySection;
    }
    ZOPT_REPLICATE = oldReplicateOption;
    ZOPT_INPUT_JOYSTICK = oldJoystickOption;
    ZOPT_GAME_CONTROL_OPTIONS = oldGameControlOptions;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_zInput_BindMap_Current = oldBindMap;
    context.FreeNonOwnedBuffers();
    FreePlayerTestOptionList();
    g_DiPickCandidateBuffer = nullptr;
    g_DiPickCandidateCursor = nullptr;
    g_HudUiMgrReticleWidget = {};
    for (zVidImagePartial *&image : g_HudUiMgrReticleImages) {
        image = nullptr;
    }
    g_Player_LocalControlEnabled = oldLocalControlEnabled;
    g_Player_RuntimeInputFlags = oldRuntimeInputFlags;
    g_Player_CameraZone = oldCameraZone;
    g_Player_CameraZoneInvRange = oldCameraZoneInvRange;
    g_Player_GameplayInputStepScale = oldGameplayStepScale;

    return ok ? 0 : 1;
}

extern "C" int player_filter_camera_probe_blocking_hits_smoke(void) {
    const int oldRaceCheckpointMode = g_HudSensorTracker.raceCheckpointMode;

    PlayerProbeSampleCandidateBuffer batches[2] = {};
    batches[0].candidateCount = 5;
    batches[1].candidateCount = 1;

    zClass_NodePartial keepNode = {};

    zClass_NodePartial ignoredFlagNode = {};
    ignoredFlagNode.flags = 0x8000000;

    int playerContextKind = 2;
    zClass_NodePartial playerNode = {};
    playerNode.flags = 0x100000;
    playerNode.callbackContext = reinterpret_cast<zClass_NodePartial *>(&playerContextKind);

    int otherContextKind = 3;
    zClass_NodePartial otherContextNode = {};
    otherContextNode.flags = 0x100000;
    otherContextNode.callbackContext = reinterpret_cast<zClass_NodePartial *>(&otherContextKind);

    zClass_NodePartial checkpointOwner = {};
    zClass_NodePartial checkpointNode = {};
    checkpointNode.flags = 0x200000;
    checkpointNode.callbackContext = &checkpointOwner;
    checkpointOwner.auxFlags = 2;
    std::strcpy(checkpointOwner.name, "checkpoint7");

    zClass_NodePartial secondBatchNode = {};

    batches[0].entries[0].node = &keepNode;
    batches[0].entries[1].node = &ignoredFlagNode;
    batches[0].entries[2].node = &playerNode;
    batches[0].entries[3].node = &checkpointNode;
    batches[0].entries[4].node = &otherContextNode;
    batches[1].entries[0].node = &secondBatchNode;

    g_HudSensorTracker.raceCheckpointMode = 1;
    Player::FilterCameraProbeBlockingHits(batches, 2);

    const bool filtered =
        batches[0].entries[0].node == &keepNode && batches[0].entries[1].node == nullptr &&
        batches[0].entries[2].node == nullptr && batches[0].entries[3].node == nullptr &&
        batches[0].entries[4].node == &otherContextNode &&
        batches[1].entries[0].node == &secondBatchNode;

    batches[0].entries[1].node = &ignoredFlagNode;
    Player::FilterCameraProbeBlockingHits(batches, 0);
    const bool nonPositiveNoop = batches[0].entries[1].node == &ignoredFlagNode;

    g_HudSensorTracker.raceCheckpointMode = oldRaceCheckpointMode;
    return filtered && nonPositiveNoop ? 0 : 1;
}

extern "C" int player_find_nearest_third_person_camera_probe_point_smoke(void) {
    PlayerProbeSampleCandidateBuffer batches[2] = {};
    zClass_NodePartial nodeA = {};
    zClass_NodePartial nodeB = {};
    zClass_NodePartial nodeC = {};

    batches[0].candidateCount = 3;
    batches[0].entries[0].hitPos = {100.0f, 0.0f, 0.0f};
    batches[0].entries[0].node = nullptr;
    batches[0].entries[1].hitPos = {8.0f, 0.0f, 0.0f};
    batches[0].entries[1].node = &nodeA;
    batches[0].entries[2].hitPos = {1.0f, 2.0f, 2.0f};
    batches[0].entries[2].node = &nodeB;
    batches[1].candidateCount = 1;
    batches[1].entries[0].hitPos = {3.0f, 0.0f, 4.0f};
    batches[1].entries[0].node = &nodeC;

    const zVec3 referencePos = {0.0f, 0.0f, 0.0f};
    zVec3 outHitPos = {9.0f, 9.0f, 9.0f};
    if (Player::FindNearestThirdPersonCameraProbePoint(batches, 2, &referencePos,
                                                       &outHitPos) != 1 ||
        !Vec3Equals(outHitPos, {1.0f, 2.0f, 2.0f})) {
        return 1;
    }

    batches[0].entries[1].node = nullptr;
    batches[0].entries[2].node = nullptr;
    batches[1].entries[0].node = nullptr;
    outHitPos = {9.0f, 9.0f, 9.0f};

    return Player::FindNearestThirdPersonCameraProbePoint(batches, 2, &referencePos,
                                                          &outHitPos) == 0 &&
                   Vec3Equals(outHitPos, {9.0f, 9.0f, 9.0f})
               ? 0
               : 2;
}

extern "C" int player_adjust_sub_camera_focus_for_obstruction_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    int *const oldMatrixIdentityFlagSlot = zMath::g_currentMatrixIdentityFlagSlot;
    float **const oldMatrixPtrSlot = zMath::g_currentMatrixPtrSlot;
    zVec3 *const oldScratchA = g_zModel_SharedVec3ScratchA;
    zVec3 *const oldScratchB = g_zModel_SharedVec3ScratchB;
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
    g_zModel_SharedVec3ScratchA = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

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
    zClass_NodePartial obstacleNode = {};
    obstacleNode.flags = 0x114;
    obstacleNode.nodeType = 0xff;
    obstacleNode.classId = 5;
    obstacleNode.classData = &objectData;
    obstacleNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    obstacleNode.cachedBounds[0] = -10.0f;
    obstacleNode.cachedBounds[1] = -10.0f;
    obstacleNode.cachedBounds[2] = -10.0f;
    obstacleNode.cachedBounds[3] = 10.0f;
    obstacleNode.cachedBounds[4] = 10.0f;
    obstacleNode.cachedBounds[5] = 10.0f;

    zClass_NodePartial *areaChildren[1] = {&obstacleNode};
    zWorldAreaPartial areaCell = {};
    areaCell.childCount = 1;
    areaCell.childList = areaChildren;
    zWorldAreaPartial *areaRows[1] = {&areaCell};
    zClass_WorldDataPartial worldData = {};
    worldData.originX = 0.0f;
    worldData.originZ = 0.0f;
    worldData.worldMaxX = 1.0f;
    worldData.worldMaxZ = -2.0f;
    worldData.clampQueriesToBounds = 1;
    worldData.areaCellSizeX = 1.0f;
    worldData.areaCellSizeZ = 1.0f;
    worldData.areaInvSizeX = 1.0f;
    worldData.areaInvSizeZ = 1.0f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = areaRows;
    zClass_NodePartial *worldChildren[1] = {&obstacleNode};
    zClass_NodePartial worldNode = {};
    worldNode.classData = &worldData;
    worldNode.listCountB = 1;
    worldNode.listB = worldChildren;
    g_Player_RuntimeDiScene = &worldNode;

    zClass_NodePartial rootNode = {};
    rootNode.flags = 0x10;
    zUtil_PlayerStateStorage playerState = {};
    playerState.rootNode = &rootNode;
    playerState.worldPos = {0.25f, 0.25f, 1.0f};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;

    zVec3 focusPos = {0.25f, 0.25f, -1.0f};
    g_cls_di_StopAfterFirstHit = 99;
    const int hitResult = Player::AdjustSubCameraFocusForObstruction(&saveState, &focusPos);
    const bool hitOk = hitResult == 1 && FloatNear(focusPos.x, 0.25f) &&
                       FloatNear(focusPos.y, 0.049999997f) &&
                       FloatNear(focusPos.z, -1.0f) && (rootNode.flags & 0x10) != 0 &&
                       g_cls_di_StopAfterFirstHit == 0;

    areaCell.childCount = 0;
    worldNode.listCountB = 0;
    focusPos = {0.25f, 0.25f, -1.0f};
    rootNode.flags = 0x10;
    g_cls_di_StopAfterFirstHit = 99;
    const int clearResult = Player::AdjustSubCameraFocusForObstruction(&saveState, &focusPos);
    const bool clearOk = clearResult == 0 && Vec3Equals(focusPos, {0.25f, 0.25f, -1.0f}) &&
                         (rootNode.flags & 0x10) != 0 && g_cls_di_StopAfterFirstHit == 0;

    g_Player_RuntimeDiScene = oldRuntimeScene;
    zMath::g_currentMatrixIdentityFlagSlot = oldMatrixIdentityFlagSlot;
    zMath::g_currentMatrixPtrSlot = oldMatrixPtrSlot;
    g_zModel_SharedVec3ScratchA = oldScratchA;
    g_zModel_SharedVec3ScratchB = oldScratchB;
    g_cls_di_StopAfterFirstHit = oldStopAfterFirstHit;

    return hitOk && clearOk ? 0 : 1;
}

extern "C" int player_adjust_third_person_camera_by_offset_probes_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    int *const oldMatrixIdentityFlagSlot = zMath::g_currentMatrixIdentityFlagSlot;
    float **const oldMatrixPtrSlot = zMath::g_currentMatrixPtrSlot;
    zVec3 *const oldScratchA = g_zModel_SharedVec3ScratchA;
    zVec3 *const oldScratchB = g_zModel_SharedVec3ScratchB;
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
    g_zModel_SharedVec3ScratchA = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zWorldAreaPartial emptyArea = {};
    zWorldAreaPartial *areaRows[1] = {&emptyArea};
    zClass_WorldDataPartial worldData = {};
    worldData.clampQueriesToBounds = 1;
    worldData.areaCellSizeX = 1.0f;
    worldData.areaCellSizeZ = 1.0f;
    worldData.areaInvSizeX = 1.0f;
    worldData.areaInvSizeZ = 1.0f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = areaRows;

    zClass_Object3DDataPartial objectData = {};
    objectData.flags = 8;
    zModel_PickFaceUvData faceUvData = {};
    zModel_PickFaceScenePayload facePayload = {};
    std::int32_t indices[3] = {0, 1, 2};
    zModel_PickFaceEntry faceEntry = {};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.vertexIndices = indices;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload = &facePayload;
    zModel_PickFaceData faceData = {};
    faceData.faceCount = 1;
    faceData.vertexCount = 3;
    faceData.faces = &faceEntry;

    zClass_NodePartial obstacleNode = {};
    obstacleNode.flags = 0x114;
    obstacleNode.nodeType = 0xff;
    obstacleNode.classId = 5;
    obstacleNode.classData = &objectData;
    obstacleNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    obstacleNode.cachedBounds[0] = -10.0f;
    obstacleNode.cachedBounds[1] = -10.0f;
    obstacleNode.cachedBounds[2] = -10.0f;
    obstacleNode.cachedBounds[3] = 10.0f;
    obstacleNode.cachedBounds[4] = 10.0f;
    obstacleNode.cachedBounds[5] = 10.0f;
    zClass_NodePartial *worldChildren[1] = {&obstacleNode};
    zClass_NodePartial worldNode = {};
    worldNode.classData = &worldData;
    worldNode.listCountB = 1;
    worldNode.listB = worldChildren;
    g_Player_RuntimeDiScene = &worldNode;

    zClass_NodePartial rootNode = {};
    rootNode.flags = 0x10;
    zUtil_PlayerStateStorage playerState = {};
    playerState.rootNode = &rootNode;
    PlayerMasterModalData masterModalData = {};
    PlayerModalState modalState = {};
    modalState.masterModalData = &masterModalData;
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    zVec3 sideVertices[3] = {{-1.0f, -1.0f, -1.0f},
                             {-1.0f, 1.0f, -1.0f},
                             {-1.0f, 0.0f, 1.0f}};
    faceData.baseVertices = sideVertices;
    masterModalData.masterType = 1;
    zVec3 cameraPos = {0.0f, 0.0f, 0.0f};
    const zVec3 sideDir = {0.0f, 0.0f, 1.0f};
    g_cls_di_StopAfterFirstHit = 99;
    const int sideResult =
        Player::AdjustThirdPersonCameraByOffsetProbes(&saveState, &cameraPos, &sideDir);
    const bool sideOk = sideResult == 1 && FloatNear(cameraPos.x, 1.0f) &&
                        FloatNear(cameraPos.y, 0.0f) && FloatNear(cameraPos.z, 0.0f) &&
                        (rootNode.flags & 0x10) != 0 && g_cls_di_StopAfterFirstHit == 0;

    zVec3 verticalVertices[3] = {{-1.0f, 1.0f, -1.0f},
                                 {1.0f, 1.0f, -1.0f},
                                 {0.0f, 1.0f, 1.0f}};
    faceData.baseVertices = verticalVertices;
    masterModalData.masterType = 2;
    cameraPos = {0.0f, 0.0f, 0.0f};
    rootNode.flags = 0x10;
    g_cls_di_StopAfterFirstHit = 99;
    const int verticalResult =
        Player::AdjustThirdPersonCameraByOffsetProbes(&saveState, &cameraPos, &sideDir);
    const bool verticalOk = verticalResult == 1 && FloatNear(cameraPos.x, 0.0f) &&
                            FloatNear(cameraPos.y, -1.0f) && FloatNear(cameraPos.z, 0.0f) &&
                            (rootNode.flags & 0x10) != 0 && g_cls_di_StopAfterFirstHit == 0;

    worldNode.listCountB = 0;
    masterModalData.masterType = 1;
    cameraPos = {3.0f, 4.0f, 5.0f};
    rootNode.flags = 0x10;
    g_cls_di_StopAfterFirstHit = 99;
    const int clearResult =
        Player::AdjustThirdPersonCameraByOffsetProbes(&saveState, &cameraPos, &sideDir);
    const bool clearOk = clearResult == 0 && Vec3Equals(cameraPos, {3.0f, 4.0f, 5.0f}) &&
                         (rootNode.flags & 0x10) != 0 && g_cls_di_StopAfterFirstHit == 0;

    g_Player_RuntimeDiScene = oldRuntimeScene;
    zMath::g_currentMatrixIdentityFlagSlot = oldMatrixIdentityFlagSlot;
    zMath::g_currentMatrixPtrSlot = oldMatrixPtrSlot;
    g_zModel_SharedVec3ScratchA = oldScratchA;
    g_zModel_SharedVec3ScratchB = oldScratchB;
    g_cls_di_StopAfterFirstHit = oldStopAfterFirstHit;

    return sideOk && verticalOk && clearOk ? 0 : 1;
}

extern "C" int player_adjust_third_person_camera_by_side_probes_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    int *const oldMatrixIdentityFlagSlot = zMath::g_currentMatrixIdentityFlagSlot;
    float **const oldMatrixPtrSlot = zMath::g_currentMatrixPtrSlot;
    zVec3 *const oldScratchA = g_zModel_SharedVec3ScratchA;
    zVec3 *const oldScratchB = g_zModel_SharedVec3ScratchB;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;
    const zTag4Partial oldLastValidCameraVariantTag = g_Player_LastValidCameraVariantTag;
    const float oldSideProbeScale = g_Player_ThirdPersonCameraSideProbeOffsetScale;
    const int oldCameraVariantUpdated = g_Player_CameraVariantUpdatedThisTick;
    const int oldStopAfterFirstHit = g_cls_di_StopAfterFirstHit;
    const int oldEffectOverrideEnabled = g_zEffect_VariantOverrideEnabled;
    const unsigned int oldEffectOverridePackedIds = g_zEffect_VariantOverridePackedIds;

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

    zVec3 floorVertices[3] = {
        {0.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 0.0f},
    };
    std::int32_t floorIndices[3] = {0, 1, 2};
    zModel_MaterialPartial floorMaterial = {};
    zDiEntryPartial floorEntry = {};
    floorEntry.flagsAndIndexCount = 3;
    floorEntry.vertexIndices = floorIndices;
    floorEntry.material = &floorMaterial;
    floorEntry.variantTagInitialized = 1;
    floorEntry.variantTag = 0x66;
    zDiPartial floorDi = {};
    floorDi.entryCount = 1;
    floorDi.vertCount = 3;
    floorDi.entries = &floorEntry;
    floorDi.verts = floorVertices;

    zClass_NodePartial floorNode = {};
    zClass_Object3DDataPartial floorObjectData = {};
    floorObjectData.flags = 8;
    floorNode.flags = 0x11c;
    floorNode.nodeType = 0xff;
    floorNode.classId = 5;
    floorNode.classData = &floorObjectData;
    floorNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&floorDi);
    floorNode.cachedBounds[0] = 0.0f;
    floorNode.cachedBounds[1] = 1.0f;
    floorNode.cachedBounds[2] = 0.0f;
    floorNode.cachedBounds[3] = 1.0f;
    floorNode.cachedBounds[4] = 1.0f;
    floorNode.cachedBounds[5] = 1.0f;

    zClass_NodePartial *areaChildren[1] = {&floorNode};
    zWorldAreaPartial area = {};
    area.childCount = 1;
    area.childList = areaChildren;
    zWorldAreaPartial *areaRows[1] = {&area};
    zClass_WorldDataPartial worldData = {};
    worldData.originX = 0.0f;
    worldData.originZ = 0.0f;
    worldData.areaCellSizeX = 1.0f;
    worldData.areaCellSizeZ = 1.0f;
    worldData.areaInvSizeX = 1.0f;
    worldData.areaInvSizeZ = 1.0f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = areaRows;
    zClass_NodePartial worldNode = {};
    worldNode.classData = &worldData;
    g_Player_RuntimeDiScene = &worldNode;

    zClass_NodePartial rootNode = {};
    rootNode.flags = 0x1c;
    zUtil_PlayerStateStorage playerState = {};
    playerState.rootNode = &rootNode;
    playerState.subModeProbeBestHeight = 20.0f;
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 0x55;
    playerState.variantTag.tags[1] = 0xff;
    playerState.variantTag.tags[2] = 0xff;
    zInput_GameStateOrMapTablePartial gameState = {};
    gameState.playerState =
        static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&playerState));
    g_GameStateOrMapTable = &gameState;

    zClass_CameraDataPartial cameraData = {};
    zClass_NodePartial camera = {};
    camera.classId = 1;
    camera.classData = &cameraData;
    g_MainCamera = &camera;

    PlayerMasterModalData masterModalData = {};
    masterModalData.masterType = 1;
    PlayerModalState modalState = {};
    modalState.masterModalData = &masterModalData;
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    g_Player_ThirdPersonCameraSideProbeOffsetScale = 1.0f;
    g_Player_CameraVariantUpdatedThisTick = 0;
    g_Player_LastValidCameraVariantTag = {};
    g_Variant_CurrentTag.count = 1;
    g_Variant_CurrentTag.tags[0] = 0x12;
    g_Variant_CurrentTag.tags[1] = 0xff;
    g_Variant_CurrentTag.tags[2] = 0xff;
    g_cls_di_StopAfterFirstHit = 99;
    g_zEffect_VariantOverrideEnabled = 0;
    g_zEffect_VariantOverridePackedIds = 0;

    zVec3 cameraPos = {0.25f, 0.0f, 0.25f};
    const zVec3 focusPos = {0.25f, 0.0f, 0.75f};
    zVec3 cameraDirNext = {0.0f, 0.0f, 1.0f};
    const int result =
        Player::AdjustThirdPersonCameraBySideProbes(&saveState, &cameraPos, &focusPos,
                                                    &cameraDirNext);

    const float expectedLen =
        static_cast<float>(std::sqrt(0.5f * 0.5f + 1.5f * 1.5f));
    const bool cameraOk =
        result == 1 && FloatNear(cameraPos.x, 0.25f) && FloatNear(cameraPos.y, 1.5f) &&
        FloatNear(cameraPos.z, 0.25f) && FloatNear(cameraDirNext.x, 0.0f) &&
        FloatNear(cameraDirNext.y, -1.5f / expectedLen) &&
        FloatNear(cameraDirNext.z, 0.5f / expectedLen);
    const bool globalOk =
        g_Player_CameraVariantUpdatedThisTick == 1 && g_cls_di_StopAfterFirstHit == 0 &&
        (rootNode.flags & 0x08) != 0 && (rootNode.flags & 0x10) != 0 &&
        cameraData.variantOverrideEnabled == 1 && g_zEffect_VariantOverrideEnabled == 1;
    const bool variantOk =
        g_Variant_CurrentTag.count == 2 && g_Variant_CurrentTag.tags[0] == 0x66 &&
        g_Variant_CurrentTag.tags[1] == 0x55 && g_Player_LastValidCameraVariantTag.count == 2 &&
        g_Player_LastValidCameraVariantTag.tags[0] == 0x66 &&
        g_Player_LastValidCameraVariantTag.tags[1] == 0x55;

    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_GameStateOrMapTable = oldGameState;
    g_MainCamera = oldMainCamera;
    zMath::g_currentMatrixIdentityFlagSlot = oldMatrixIdentityFlagSlot;
    zMath::g_currentMatrixPtrSlot = oldMatrixPtrSlot;
    g_zModel_SharedVec3ScratchA = oldScratchA;
    g_zModel_SharedVec3ScratchB = oldScratchB;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_VariantTag_Current = oldVariantTagCurrent;
    g_Player_LastValidCameraVariantTag = oldLastValidCameraVariantTag;
    g_Player_ThirdPersonCameraSideProbeOffsetScale = oldSideProbeScale;
    g_Player_CameraVariantUpdatedThisTick = oldCameraVariantUpdated;
    g_cls_di_StopAfterFirstHit = oldStopAfterFirstHit;
    g_zEffect_VariantOverrideEnabled = oldEffectOverrideEnabled;
    g_zEffect_VariantOverridePackedIds = oldEffectOverridePackedIds;

    if (!cameraOk) {
        if (!FloatNear(cameraPos.x, 0.25f)) {
            return 12;
        }
        if (!FloatNear(cameraPos.y, 1.5f)) {
            return 13;
        }
        if (!FloatNear(cameraPos.z, 0.25f)) {
            return 14;
        }
        if (!FloatNear(cameraDirNext.x, 0.0f)) {
            return 15;
        }
        if (!FloatNear(cameraDirNext.y, -1.5f / expectedLen)) {
            return 16;
        }
        if (!FloatNear(cameraDirNext.z, 0.5f / expectedLen)) {
            return 17;
        }
        if (result != 1) {
            return 11;
        }
        return 1;
    }
    if (!globalOk) {
        return 2;
    }
    return variantOk ? 0 : 3;
}

extern "C" int player_update_chase_camera_from_input_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    int *const oldMatrixIdentityFlagSlot = zMath::g_currentMatrixIdentityFlagSlot;
    float **const oldMatrixPtrSlot = zMath::g_currentMatrixPtrSlot;
    zVec3 *const oldScratchA = g_zModel_SharedVec3ScratchA;
    zVec3 *const oldScratchB = g_zModel_SharedVec3ScratchB;
    const float oldFrameDeltaTimeSec = g_FrameDeltaTimeSec;
    const float oldCameraZone = g_Player_CameraZone;
    const float oldCameraZoneInvRange = g_Player_CameraZoneInvRange;
    const float oldMaxCamYawRate = g_Player_MaxCamYawRate;
    const float oldMousePushX = g_Player_MousePushX;
    const float oldMousePushY = g_Player_MousePushY;
    const float oldCameraElastic = g_Player_CameraElastic;
    const float oldHeadingDotAbs = g_Player_CameraHeadingDotAbs;
    const float oldHeadingLerpClear = g_Player_CameraHeadingLerpBaseWhenFlagClear;
    const float oldHeadingLerpSet = g_Player_CameraHeadingLerpBaseWhenFlagSet;
    const float oldSideProbeScale = g_Player_ThirdPersonCameraSideProbeOffsetScale;
    const int oldStopAfterFirstHit = g_cls_di_StopAfterFirstHit;
    int *const oldGameControlOptions = ZOPT_GAME_CONTROL_OPTIONS;
    int gameControlOptions = oldGameControlOptions != nullptr ? *oldGameControlOptions : 0;
    ZOPT_GAME_CONTROL_OPTIONS = &gameControlOptions;
    const int oldCursorMode = zOpt::GetCursorMode();
    const int oldSteeringMode = zOpt::GetSteeringMode();

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

    zWorldAreaPartial emptyArea = {};
    zWorldAreaPartial *areaRows[1] = {&emptyArea};
    zClass_WorldDataPartial worldData = {};
    worldData.areaCellSizeX = 1.0f;
    worldData.areaCellSizeZ = 1.0f;
    worldData.areaInvSizeX = 1.0f;
    worldData.areaInvSizeZ = 1.0f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = areaRows;
    zClass_NodePartial worldNode = {};
    worldNode.classData = &worldData;
    g_Player_RuntimeDiScene = &worldNode;

    zClass_CameraDataPartial cameraData = {};
    zClass_NodePartial camera = {};
    camera.classId = 1;
    camera.classData = &cameraData;
    g_MainCamera = &camera;

    zClass_NodePartial rootNode = {};
    rootNode.flags = 0x1c;
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    PlayerMasterModalData modalData = {};
    PlayerModalState modalState = {};
    zUtil_SaveGameState saveState = {};
    playerState.masterCommonData = &commonData;
    playerState.rootNode = &rootNode;
    playerState.worldPos = {1.0f, 2.0f, 3.0f};
    playerState.steerBasisNorm = {0.0f, 0.0f, 1.0f};
    playerState.steerBasisRaw = {0.0f, 0.0f, 1.0f};
    playerState.cameraDirFlat = {0.0f, 0.0f, 1.0f};
    playerState.cameraTargetDistance = 10.0f;
    playerState.cameraDistance = 10.0f;
    playerState.thirdPersonSideOffset = 0.0f;
    playerState.thirdPersonBaseYOffset = 4.0f;
    playerState.cameraYOffset = 1.0f;
    commonData.cameraUdSwing[0] = 2.0f;
    modalData.masterType = 1;
    modalState.masterModalData = &modalData;
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    zOpt::SetCursorMode(0);
    zOpt::SetSteeringMode(1);
    g_FrameDeltaTimeSec = 0.0f;
    g_Player_CameraZone = 0.5f;
    g_Player_CameraZoneInvRange = 2.0f;
    g_Player_MaxCamYawRate = 2.0f;
    g_Player_MousePushX = 0.0f;
    g_Player_MousePushY = 0.0f;
    g_Player_CameraElastic = 0.0f;
    g_Player_CameraHeadingDotAbs = 1.0f;
    g_Player_CameraHeadingLerpBaseWhenFlagClear = 3.0f;
    g_Player_CameraHeadingLerpBaseWhenFlagSet = 2.0f;
    g_Player_ThirdPersonCameraSideProbeOffsetScale = 1.0f;
    g_cls_di_StopAfterFirstHit = 99;

    Player::UpdateChaseCameraFromInput(&saveState);

    const zVec3 expectedCameraPos = {1.0f, 6.0f, -7.0f};
    const zVec3 expectedFocusPos = {1.0f, 3.0f, 3.0f};
    zVec3 expectedAngles = {};
    zMath::Vec3DirectionAnglesBetweenPoints(&expectedCameraPos, &expectedFocusPos,
                                            &expectedAngles);
    const float expectedLength =
        static_cast<float>(std::sqrt(3.0f * 3.0f + 10.0f * 10.0f));
    const bool cameraOk =
        Vec3Equals(playerState.cameraTarget, expectedCameraPos) &&
        Vec3Equals(cameraData.targetOrEuler, expectedCameraPos) &&
        FloatNear(cameraData.posOffset.x, expectedAngles.x) &&
        FloatNear(cameraData.posOffset.y, expectedAngles.y) &&
        FloatNear(cameraData.posOffset.z, expectedAngles.z);
    const bool directionOk =
        FloatNear(playerState.cameraDirNext.x, 0.0f) &&
        FloatNear(playerState.cameraDirNext.y, -3.0f / expectedLength) &&
        FloatNear(playerState.cameraDirNext.z, 10.0f / expectedLength) &&
        Vec3Equals(playerState.cameraDir, playerState.cameraDirNext);
    const bool stateOk =
        playerState.thirdPersonYawOffset == 0.0f &&
        playerState.cameraElevationOffset == 0.0f &&
        playerState.cameraTargetDistance == 10.0f &&
        (rootNode.flags & 0x08) != 0 && (rootNode.flags & 0x10) != 0 &&
        g_cls_di_StopAfterFirstHit == 0;

    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_MainCamera = oldMainCamera;
    zMath::g_currentMatrixIdentityFlagSlot = oldMatrixIdentityFlagSlot;
    zMath::g_currentMatrixPtrSlot = oldMatrixPtrSlot;
    g_zModel_SharedVec3ScratchA = oldScratchA;
    g_zModel_SharedVec3ScratchB = oldScratchB;
    g_FrameDeltaTimeSec = oldFrameDeltaTimeSec;
    g_Player_CameraZone = oldCameraZone;
    g_Player_CameraZoneInvRange = oldCameraZoneInvRange;
    g_Player_MaxCamYawRate = oldMaxCamYawRate;
    g_Player_MousePushX = oldMousePushX;
    g_Player_MousePushY = oldMousePushY;
    g_Player_CameraElastic = oldCameraElastic;
    g_Player_CameraHeadingDotAbs = oldHeadingDotAbs;
    g_Player_CameraHeadingLerpBaseWhenFlagClear = oldHeadingLerpClear;
    g_Player_CameraHeadingLerpBaseWhenFlagSet = oldHeadingLerpSet;
    g_Player_ThirdPersonCameraSideProbeOffsetScale = oldSideProbeScale;
    g_cls_di_StopAfterFirstHit = oldStopAfterFirstHit;
    zOpt::SetCursorMode(oldCursorMode);
    zOpt::SetSteeringMode(oldSteeringMode);
    ZOPT_GAME_CONTROL_OPTIONS = oldGameControlOptions;

    if (!cameraOk) {
        return 1;
    }
    if (!directionOk) {
        return 2;
    }
    return stateOk ? 0 : 3;
}

extern "C" int player_update_top_down_camera_state_smoke(void) {
    zClass_NodePartial *const oldMainCamera = g_MainCamera;

    zClass_CameraDataPartial cameraData = {};
    zClass_NodePartial camera = {};
    camera.classId = 1;
    camera.classData = &cameraData;
    g_MainCamera = &camera;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    playerState.worldPos = {10.0f, 20.0f, 30.0f};
    playerState.cameraState2TargetOffset = {1.5f, -2.25f, 4.0f};
    playerState.cameraTarget = {-1.0f, -1.0f, -1.0f};
    playerState.cameraDir = {7.0f, 8.0f, 9.0f};

    Player::UpdateTopDownCameraState(&saveState);

    const zVec3 expectedTarget = {11.5f, 17.75f, 34.0f};
    const bool targetOk = Vec3Equals(playerState.cameraTarget, expectedTarget) &&
                          Vec3Equals(cameraData.targetOrEuler, expectedTarget);
    const bool cameraPosOk = FloatNear(cameraData.posOffset.x, -1.54999995f) &&
                             FloatNear(cameraData.posOffset.y, 0.0f) &&
                             FloatNear(cameraData.posOffset.z, 0.0f);
    const bool dirOk = FloatNear(playerState.cameraDir.x, 0.0f) &&
                       FloatNear(playerState.cameraDir.y, -1.0f) &&
                       FloatNear(playerState.cameraDir.z, 0.0f);

    g_MainCamera = oldMainCamera;

    if (!targetOk) {
        return 1;
    }
    if (!cameraPosOk) {
        return 2;
    }
    return dirOk ? 0 : 3;
}

extern "C" int player_update_first_person_camera_from_input_smoke(void) {
    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    const float oldFrameDeltaTimeSec = g_FrameDeltaTimeSec;
    const float oldCameraZone = g_Player_CameraZone;
    const float oldCameraZoneInvRange = g_Player_CameraZoneInvRange;
    const float oldFpCamElevationRate = g_Player_FpCamElevationRate;
    const float oldFpCamElevationMin = g_Player_FpCamElevationMin;
    const float oldFpCamElevationMax = g_Player_FpCamElevationMax;
    int *const oldGameControlOptions = ZOPT_GAME_CONTROL_OPTIONS;
    int gameControlOptions = oldGameControlOptions != nullptr ? *oldGameControlOptions : 0;
    ZOPT_GAME_CONTROL_OPTIONS = &gameControlOptions;
    const int oldCursorMode = zOpt::GetCursorMode();

    zClass_CameraDataPartial cameraData = {};
    zClass_NodePartial camera = {};
    camera.classId = 1;
    camera.classData = &cameraData;
    g_MainCamera = &camera;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    playerState.worldPos = {10.0f, 20.0f, 30.0f};
    playerState.motionBasis.xx = 1.0f;
    playerState.motionBasis.yy = 1.0f;
    playerState.motionBasis.zz = 1.0f;
    playerState.cameraState6LocalOffset = {1.0f, 2.0f, 3.0f};
    playerState.cameraState6BasePos = {5.0f, 6.0f, 7.0f};
    playerState.steerBasisRaw = {0.25f, -0.5f, 0.75f};
    playerState.cursorNormY = 0.75f;
    playerState.cameraElevationOffset = 0.1f;

    zOpt::SetCursorMode(0);
    g_FrameDeltaTimeSec = 0.25f;
    g_Player_CameraZone = 0.5f;
    g_Player_CameraZoneInvRange = 2.0f;
    g_Player_FpCamElevationRate = 4.0f;
    g_Player_FpCamElevationMin = -2.0f;
    g_Player_FpCamElevationMax = 2.0f;

    Player::UpdateFirstPersonCameraFromInput(&saveState);

    const zVec3 expectedTarget = {11.0f, 22.0f, 33.0f};
    const bool targetOk = Vec3Equals(playerState.cameraTarget, expectedTarget) &&
                          Vec3Equals(cameraData.targetOrEuler, expectedTarget);
    const bool elevationOk = FloatNear(playerState.cameraElevationOffset, -0.4f);
    const bool cameraPosOk = FloatNear(cameraData.posOffset.x, 4.86000013f) &&
                             FloatNear(cameraData.posOffset.y, 6.0f) &&
                             FloatNear(cameraData.posOffset.z, 7.0f);
    const bool dirOk = Vec3Equals(playerState.cameraDirNext, playerState.steerBasisRaw) &&
                       Vec3Equals(playerState.cameraDirFlat, playerState.steerBasisRaw) &&
                       Vec3Equals(playerState.cameraDir, playerState.steerBasisRaw);

    g_MainCamera = oldMainCamera;
    g_FrameDeltaTimeSec = oldFrameDeltaTimeSec;
    g_Player_CameraZone = oldCameraZone;
    g_Player_CameraZoneInvRange = oldCameraZoneInvRange;
    g_Player_FpCamElevationRate = oldFpCamElevationRate;
    g_Player_FpCamElevationMin = oldFpCamElevationMin;
    g_Player_FpCamElevationMax = oldFpCamElevationMax;
    zOpt::SetCursorMode(oldCursorMode);
    ZOPT_GAME_CONTROL_OPTIONS = oldGameControlOptions;

    if (!targetOk) {
        return 1;
    }
    if (!elevationOk) {
        return 2;
    }
    if (!cameraPosOk) {
        return 3;
    }
    return dirOk ? 0 : 4;
}

extern "C" int player_update_camera_from_stored_target_toward_player_smoke(void) {
    zClass_NodePartial *const oldMainCamera = g_MainCamera;

    zClass_CameraDataPartial cameraData = {};
    zClass_NodePartial camera = {};
    camera.classId = 1;
    camera.classData = &cameraData;
    g_MainCamera = &camera;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    playerState.worldPos = {10.0f, 20.0f, 30.0f};
    playerState.cameraTarget = {10.0f, 20.0f, 20.0f};
    playerState.cameraYOffset = 6.0f;
    playerState.cameraState6YOffset = 2.0f;
    playerState.cameraState = 1;

    Player::UpdateCameraFromStoredTargetTowardPlayer(&saveState);

    zVec3 expectedAngles = {};
    const zVec3 expectedLookAt = {10.0f, 26.0f, 30.0f};
    zMath::Vec3DirectionAnglesBetweenPoints(&playerState.cameraTarget, &expectedLookAt,
                                            &expectedAngles);
    const float expectedInvLen =
        1.0f / static_cast<float>(std::sqrt(6.0f * 6.0f + 10.0f * 10.0f));
    const bool thirdPersonOk =
        FloatNear(playerState.cameraDirNext.x, 0.0f) &&
        FloatNear(playerState.cameraDirNext.y, 6.0f * expectedInvLen) &&
        FloatNear(playerState.cameraDirNext.z, 10.0f * expectedInvLen) &&
        Vec3Equals(playerState.cameraDir, playerState.cameraDirNext) &&
        FloatNear(playerState.cameraDirFlat.x, 0.0f) &&
        FloatNear(playerState.cameraDirFlat.y, 0.0f) &&
        FloatNear(playerState.cameraDirFlat.z, 1.0f) &&
        FloatNear(cameraData.posOffset.x, expectedAngles.x) &&
        FloatNear(cameraData.posOffset.y, expectedAngles.y) &&
        FloatNear(cameraData.posOffset.z, expectedAngles.z);

    playerState.cameraTarget = {10.0f, 20.0f, 20.0f};
    playerState.cameraState = 3;
    Player::UpdateCameraFromStoredTargetTowardPlayer(&saveState);

    const zVec3 expectedLookAtState6 = {10.0f, 22.0f, 30.0f};
    zMath::Vec3DirectionAnglesBetweenPoints(&playerState.cameraTarget,
                                            &expectedLookAtState6, &expectedAngles);
    const float expectedInvLenState6 =
        1.0f / static_cast<float>(std::sqrt(2.0f * 2.0f + 10.0f * 10.0f));
    const bool state6Ok =
        FloatNear(playerState.cameraDirNext.y, 2.0f * expectedInvLenState6) &&
        FloatNear(playerState.cameraDirNext.z, 10.0f * expectedInvLenState6) &&
        FloatNear(cameraData.posOffset.x, expectedAngles.x) &&
        FloatNear(cameraData.posOffset.y, expectedAngles.y) &&
        FloatNear(cameraData.posOffset.z, expectedAngles.z);

    g_MainCamera = oldMainCamera;

    if (!thirdPersonOk) {
        return 1;
    }
    return state6Ok ? 0 : 2;
}

extern "C" int player_restore_third_person_camera_from_obstruction_state_smoke(void) {
    zClass_NodePartial *const oldMainCamera = g_MainCamera;

    zClass_Object3DDataPartial objectData = {};
    objectData.cachedWorldMatrix[9] = 12.0f;
    objectData.cachedWorldMatrix[10] = 34.0f;
    objectData.cachedWorldMatrix[11] = 56.0f;
    zClass_NodePartial camera = {};
    camera.classId = 5;
    camera.flags = 0x00080000;
    camera.classData = &objectData;
    g_MainCamera = &camera;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    playerState.cameraTarget = {-1.0f, -2.0f, -3.0f};
    playerState.cameraDir = {1.0f, 2.0f, 3.0f};
    playerState.cameraObstructionDir = {0.25f, 0.5f, 0.75f};

    Player::RestoreThirdPersonCameraFromObstructionState(&saveState);

    const bool targetOk = Vec3Equals(playerState.cameraTarget, {12.0f, 34.0f, 56.0f});
    const bool dirOk = Vec3Equals(playerState.cameraDir, playerState.cameraObstructionDir);

    g_MainCamera = oldMainCamera;

    if (!targetOk) {
        return 1;
    }
    return dirOk ? 0 : 2;
}

extern "C" int player_update_camera_variant_from_anchor_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const zTag4Partial oldLastValidCameraVariantTag = g_Player_LastValidCameraVariantTag;
    const int oldEffectOverrideEnabled = g_zEffect_VariantOverrideEnabled;
    const unsigned int oldEffectOverridePackedIds = g_zEffect_VariantOverridePackedIds;

    zUtil_PlayerStateStorage playerState = {};
    playerState.variantTag.count = 2;
    playerState.variantTag.tags[0] = 7;
    playerState.variantTag.tags[1] = 8;
    playerState.variantTag.tags[2] = 0xff;
    zInput_GameStateOrMapTablePartial gameState = {};
    gameState.playerState =
        static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&playerState));
    g_GameStateOrMapTable = &gameState;

    zClass_CameraDataPartial cameraData = {};
    zClass_NodePartial camera = {};
    camera.classId = 1;
    camera.classData = &cameraData;
    g_MainCamera = &camera;

    PlayerProbeSampleCandidateBuffer candidates = {};
    candidates.candidateCount = 1;
    candidates.entries[0].variantTag.count = 2;
    candidates.entries[0].variantTag.tags[0] = 8;
    candidates.entries[0].variantTag.tags[1] = 9;
    candidates.entries[0].variantTag.tags[2] = 0xff;
    g_Player_LastValidCameraVariantTag = {};
    g_zEffect_VariantOverrideEnabled = 0;
    g_zEffect_VariantOverridePackedIds = 0;
    zVec3 cameraPos = {};
    Player::UpdateCameraVariantFromAnchor(&candidates, &cameraPos, 0);
    const bool mergeOk =
        g_Player_LastValidCameraVariantTag.count == 3 &&
        g_Player_LastValidCameraVariantTag.tags[0] == 8 &&
        g_Player_LastValidCameraVariantTag.tags[1] == 9 &&
        g_Player_LastValidCameraVariantTag.tags[2] == 7 &&
        g_VariantTag_Current.count == 3 && g_Variant_CurrentTag.count == 3 &&
        cameraData.variantOverrideEnabled == 1 && cameraData.variantTag.count == 3 &&
        cameraData.variantTag.tags[0] == 8 && cameraData.variantTag.tags[1] == 9 &&
        cameraData.variantTag.tags[2] == 7 && g_zEffect_VariantOverrideEnabled == 1;

    unsigned int expectedPacked = 0;
    std::memcpy(&expectedPacked, &g_VariantTag_Current, sizeof(expectedPacked));
    const bool effectPackedOk = g_zEffect_VariantOverridePackedIds == expectedPacked;

    candidates.entries[0].variantTag.count = 2;
    candidates.entries[0].variantTag.tags[0] = 0xff;
    candidates.entries[0].variantTag.tags[1] = 11;
    g_Player_LastValidCameraVariantTag.count = 1;
    g_Player_LastValidCameraVariantTag.tags[0] = 5;
    g_Player_LastValidCameraVariantTag.tags[1] = 0xff;
    g_Player_LastValidCameraVariantTag.tags[2] = 0xff;
    cameraData.variantOverrideEnabled = 0;
    g_zEffect_VariantOverrideEnabled = 0;
    Player::UpdateCameraVariantFromAnchor(&candidates, &cameraPos, 0);
    const bool incompleteFallbackOk =
        g_VariantTag_Current.count == 1 && g_VariantTag_Current.tags[0] == 5 &&
        g_Variant_CurrentTag.count == 1 && cameraData.variantOverrideEnabled == 1 &&
        cameraData.variantTag.count == 1 && cameraData.variantTag.tags[0] == 5 &&
        g_zEffect_VariantOverrideEnabled == 1;

    candidates.candidateCount = 0;
    g_Player_LastValidCameraVariantTag.count = 2;
    g_Player_LastValidCameraVariantTag.tags[0] = 12;
    g_Player_LastValidCameraVariantTag.tags[1] = 13;
    g_Player_LastValidCameraVariantTag.tags[2] = 0xff;
    cameraData.variantOverrideEnabled = 0;
    Player::UpdateCameraVariantFromAnchor(&candidates, &cameraPos, 0);
    const bool emptyFallbackOk =
        g_VariantTag_Current.count == 2 && g_VariantTag_Current.tags[0] == 12 &&
        g_VariantTag_Current.tags[1] == 13 && cameraData.variantOverrideEnabled == 1 &&
        cameraData.variantTag.count == 2 && cameraData.variantTag.tags[0] == 12 &&
        cameraData.variantTag.tags[1] == 13;

    g_GameStateOrMapTable = oldGameState;
    g_MainCamera = oldMainCamera;
    g_VariantTag_Current = oldVariantTagCurrent;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_Player_LastValidCameraVariantTag = oldLastValidCameraVariantTag;
    g_zEffect_VariantOverrideEnabled = oldEffectOverrideEnabled;
    g_zEffect_VariantOverridePackedIds = oldEffectOverridePackedIds;

    return mergeOk && effectPackedOk && incompleteFallbackOk && emptyFallbackOk ? 0 : 1;
}

extern "C" int player_update_camera_variant_from_camera_pos_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const int oldCameraVariantUpdated = g_Player_CameraVariantUpdatedThisTick;

    zWorldAreaPartial area = {};
    zWorldAreaPartial *areaRows[1] = {&area};
    zClass_WorldDataPartial worldData = {};
    worldData.originX = 0.0f;
    worldData.originZ = 0.0f;
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

    zClass_NodePartial rootNode = {};
    rootNode.flags = 0x08;
    zUtil_PlayerStateStorage playerState = {};
    playerState.rootNode = &rootNode;
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    zVec3 cameraPos = {2.0f, 3.0f, 4.0f};

    g_Player_CameraVariantUpdatedThisTick = 0;
    Player::UpdateCameraVariantFromCameraPos(&saveState, &cameraPos);

    const bool ok = (rootNode.flags & 0x08) != 0 &&
                    g_Player_CameraVariantUpdatedThisTick == 1 &&
                    Vec3Equals(cameraPos, {2.0f, 3.0f, 4.0f});

    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Player_CameraVariantUpdatedThisTick = oldCameraVariantUpdated;

    return ok ? 0 : 1;
}

extern "C" int player_update_camera_weather_fx_emitter_visibility_smoke(void) {
    HudUiElement *const oldFxPass3Obj = g_HudSensorTracker.fxPass3Obj;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    int *const oldReplicate = ZOPT_REPLICATE;
    const int oldBreakOnFirst = g_cls_di_BreakOnFirstCandidate;
    const int oldStopAfterFirst = g_cls_di_StopAfterFirstHit;

    HudUiCommon_FTable visibleTable = {};
    visibleTable.slots[24] = MethodAddress(&TestPlayerWeatherFxEmitter::SetVisible);
    TestPlayerWeatherFxEmitter fxEmitter = {};
    fxEmitter.ui.ftable = &visibleTable;
    fxEmitter.ui.flags = 0x10;
    g_HudSensorTracker.fxPass3Obj = &fxEmitter.ui;

    zClass_CameraDataPartial cameraData = {};
    cameraData.targetOrEuler = {1.0f, 2.0f, 3.0f};
    zClass_NodePartial camera = {};
    camera.classId = 1;
    camera.classData = &cameraData;
    g_MainCamera = &camera;

    zWorldAreaPartial area = {};
    zWorldAreaPartial *areaRows[1] = {&area};
    zClass_WorldDataPartial worldData = {};
    worldData.originX = 0.0f;
    worldData.originZ = 0.0f;
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

    zClass_NodePartial rootNode = {};
    rootNode.flags = 0x10;
    zUtil_PlayerStateStorage playerState = {};
    playerState.rootNode = &rootNode;
    zUtil_SaveGameState saveState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)(&saveState);

    int replicate = 0;
    ZOPT_REPLICATE = &replicate;
    g_PlayerTestHudVisibleCount = 0;
    g_cls_di_BreakOnFirstCandidate = 9;
    g_cls_di_StopAfterFirstHit = 9;

    modalData.masterType = 1;
    Player::UpdateCameraWeatherFxEmitterVisibility();
    const bool visibleOk =
        g_PlayerTestHudVisibleCount == 1 && g_PlayerTestHudVisibleValue[0] == 1 &&
        (fxEmitter.ui.flags & 0x10) == 0 && fxEmitter.cameraNode == &camera &&
        fxEmitter.particleAgeTick == 1 && (rootNode.flags & 0x10) != 0 &&
        g_cls_di_BreakOnFirstCandidate == 0 && g_cls_di_StopAfterFirstHit == 0;

    g_PlayerTestHudVisibleCount = 0;
    fxEmitter.ui.flags = 0;
    fxEmitter.cameraNode = 0;
    fxEmitter.particleAgeTick = 7;
    modalData.masterType = 2;
    Player::UpdateCameraWeatherFxEmitterVisibility();
    const bool subModeOk = g_PlayerTestHudVisibleCount == 1 &&
                           g_PlayerTestHudVisibleValue[0] == 0 &&
                           (fxEmitter.ui.flags & 0x10) != 0 &&
                           fxEmitter.cameraNode == 0 && fxEmitter.particleAgeTick == 7;

    g_HudSensorTracker.fxPass3Obj = oldFxPass3Obj;
    g_GameStateOrMapTable = oldGameState;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_MainCamera = oldMainCamera;
    ZOPT_REPLICATE = oldReplicate;
    g_cls_di_BreakOnFirstCandidate = oldBreakOnFirst;
    g_cls_di_StopAfterFirstHit = oldStopAfterFirst;

    if (!visibleOk) {
        return 1;
    }
    return subModeOk ? 0 : 2;
}

extern "C" int player_tick_active_camera_state_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    HudUiElement *const oldFxPass3Obj = g_HudSensorTracker.fxPass3Obj;
    const int oldVariantUpdated = g_Player_CameraVariantUpdatedThisTick;
    const int oldRebuildFlat = g_Player_RebuildCameraDirFlatFromCurrentTarget;

    zWorldAreaPartial area = {};
    zWorldAreaPartial *areaRows[1] = {&area};
    zClass_WorldDataPartial worldData = {};
    worldData.originX = 0.0f;
    worldData.originZ = 0.0f;
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

    zClass_CameraDataPartial cameraData = {};
    cameraData.targetOrEuler = {1.0f, 2.0f, 3.0f};
    zClass_NodePartial camera = {};
    camera.classId = 1;
    camera.classData = &cameraData;
    g_MainCamera = &camera;
    g_HudSensorTracker.fxPass3Obj = nullptr;

    zClass_NodePartial rootNode = {};
    rootNode.flags = 0x08;
    zUtil_PlayerStateStorage playerState = {};
    playerState.rootNode = &rootNode;
    playerState.worldPos = {4.0f, 6.0f, 3.0f};
    playerState.cameraYOffset = 4.0f;
    playerState.cameraTarget = {12.0f, 13.0f, 14.0f};
    playerState.cameraDirNext = {0.25f, 0.5f, 0.75f};
    playerState.steerBasisNorm = {-1.0f, 0.0f, 0.0f};
    playerState.cameraState = 5;
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;

    g_Player_CameraVariantUpdatedThisTick = 99;
    g_Player_RebuildCameraDirFlatFromCurrentTarget = 1;
    Player::TickActiveCameraState(&saveState);

    const float expectedDistance =
        static_cast<float>(sqrt(3.0f * 3.0f + 4.0f * 4.0f + 0.0f * 0.0f));
    const float expectedLen =
        static_cast<float>(sqrt(3.0f * 3.0f + 8.0f * 8.0f + 0.0f * 0.0f));
    const bool rebuildOk =
        g_Player_CameraVariantUpdatedThisTick == 1 &&
        g_Player_RebuildCameraDirFlatFromCurrentTarget == 0 &&
        FloatNear(playerState.cameraTargetDistance, expectedDistance) &&
        FloatNear(playerState.cameraDirFlat.x, 3.0f / expectedLen) &&
        FloatNear(playerState.cameraDirFlat.y, 8.0f / expectedLen) &&
        FloatNear(playerState.cameraDirFlat.z, 0.0f) &&
        Vec3Equals(playerState.cameraBasisCache, playerState.cameraDirNext) &&
        (rootNode.flags & 0x08) != 0;

    playerState.cameraState = 2;
    playerState.cameraTarget = {0.0f, 0.0f, 0.0f};
    playerState.cameraState2TargetOffset = {1.0f, 2.0f, 3.0f};
    playerState.steerBasisNorm = {0.0f, 0.0f, -1.0f};
    g_Player_CameraVariantUpdatedThisTick = 1;
    Player::TickActiveCameraState(&saveState);
    const bool state2CacheOk = Vec3Equals(playerState.cameraBasisCache,
                                          playerState.steerBasisNorm) &&
                               g_Player_CameraVariantUpdatedThisTick == 1;

    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_MainCamera = oldMainCamera;
    g_HudSensorTracker.fxPass3Obj = oldFxPass3Obj;
    g_Player_CameraVariantUpdatedThisTick = oldVariantUpdated;
    g_Player_RebuildCameraDirFlatFromCurrentTarget = oldRebuildFlat;

    if (!rebuildOk) {
        return 1;
    }
    return state2CacheOk ? 0 : 2;
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

extern "C" int checkpoint_instantiate_named_objects_smoke(void) {
    const int oldCheckpointCount = g_HudSensorTracker.checkpointCount;
    zClass_TypeListLink *const oldType6Head = zClass_TypeList::Head(6);
    zClass_TypeListLink *const oldType6Tail = zClass_TypeList::Tail(6);

    zClass_NodePartial checkpoint1 = {};
    zClass_NodePartial checkpoint2 = {};
    zClass_NodePartial child = {};
    zClass_NodePartial grandchild = {};
    zClass_NodePartial unrelated = {};
    zClass_NodePartial *checkpoint2Children[] = {&child};
    zClass_NodePartial *childChildren[] = {&grandchild};
    zClass_TypeListLink checkpoint1Link = {&checkpoint1, nullptr, nullptr, 0};
    zClass_TypeListLink checkpoint2Link = {&checkpoint2, &checkpoint1Link, nullptr, 0};
    zClass_TypeListLink unrelatedLink = {&unrelated, &checkpoint2Link, nullptr, 0};

    std::strcpy(checkpoint1.name, "checkpoint1");
    std::strcpy(checkpoint2.name, "checkpoint2");
    std::strcpy(unrelated.name, "checkpoint4");
    checkpoint2.listCountB = 1;
    checkpoint2.listB = checkpoint2Children;
    child.listCountB = 1;
    child.listB = childChildren;

    checkpoint1Link.next = &checkpoint2Link;
    checkpoint2Link.next = &unrelatedLink;
    zClass_TypeList::Head(6) = &checkpoint1Link;
    zClass_TypeList::Tail(6) = &unrelatedLink;
    g_HudSensorTracker.checkpointCount = 3;

    Checkpoint::InstantiateNamedObjects();

    const bool checkpoint1Ok =
        checkpoint1.auxFlags == 2 && (checkpoint1.flags & 0x40000) != 0 &&
        checkpoint1.callbackContext == &checkpoint1 && (checkpoint1.flags & 0x200000) != 0;
    const bool checkpoint2Ok =
        checkpoint2.auxFlags == 2 && child.auxFlags == 2 && grandchild.auxFlags == 2 &&
        (checkpoint2.flags & 0x40000) != 0 && (child.flags & 0x40000) != 0 &&
        (grandchild.flags & 0x40000) != 0 && checkpoint2.callbackContext == &checkpoint2 &&
        child.callbackContext == &checkpoint2 && grandchild.callbackContext == &checkpoint2 &&
        (checkpoint2.flags & 0x200000) != 0 && (child.flags & 0x200000) != 0 &&
        (grandchild.flags & 0x200000) != 0;
    const bool unrelatedOk =
        unrelated.auxFlags == 0 && unrelated.flags == 0 && unrelated.callbackContext == nullptr;

    g_HudSensorTracker.checkpointCount = oldCheckpointCount;
    zClass_TypeList::Head(6) = oldType6Head;
    zClass_TypeList::Tail(6) = oldType6Tail;
    return checkpoint1Ok && checkpoint2Ok && unrelatedOk ? 0 : 1;
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

extern "C" int player_ai_mode2_forward_probe_requires_auto_turn_smoke(void) {
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

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    zClass_NodePartial rootNode = {};
    zClass_WorldDataPartial emptyWorldData = {};
    zClass_NodePartial emptyWorld = {};
    emptyWorld.classData = &emptyWorldData;

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    playerState.rootNode = &rootNode;
    playerState.worldPos = {10.0f, 20.0f, 30.0f};
    playerState.projectileSpawnVel = {3.0f, 0.0f, 4.0f};
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 0x33;
    playerState.variantTag.tags[1] = 0xff;
    playerState.variantTag.tags[2] = 0xff;
    modalData.probePoints[1].y = 2.5f;
    modalData.probePoints[1].z = -6.0f;
    g_Player_RuntimeDiScene = &emptyWorld;
    g_VariantTag_Current.count = 0;
    g_VariantTag_Current.tags[0] = 0xff;
    g_VariantTag_Current.tags[1] = 0xff;
    g_VariantTag_Current.tags[2] = 0xff;
    g_Variant_CurrentTag.count = 2;
    g_Variant_CurrentTag.tags[0] = 1;
    g_Variant_CurrentTag.tags[1] = 2;

    playerState.playerCollisionResolved = 1;
    playerState.aiMode2SteeringRetryCount = 4;
    const int earlyResult = Player::AiMode2ForwardProbeRequiresAutoTurn(&saveState);
    const bool earlyOk = earlyResult == 1 && playerState.aiMode2SteeringRetryCount == 5;

    playerState.playerCollisionResolved = 0;
    playerState.preferredCollisionResolved = 0;
    playerState.aiMode2SteeringRetryCount = 9;
    const int clearResult = Player::AiMode2ForwardProbeRequiresAutoTurn(&saveState);
    const bool clearOk =
        clearResult == 0 && playerState.aiMode2SteeringRetryCount == 9 &&
        PendingContactQueueCleared(playerState.preferredCollisionQueue) &&
        PendingContactQueueCleared(playerState.playerCollisionQueue) &&
        PendingContactQueueCleared(playerState.worldCollisionQueue) &&
        (rootNode.flags & 0x10) != 0 &&
        g_Variant_CurrentTag.count == g_VariantTag_Current.count &&
        g_Variant_CurrentTag.tags[0] == g_VariantTag_Current.tags[0];

    FillPendingContactQueue(&playerState.preferredCollisionQueue, 1);
    const int queuedResult = Player::AiMode2ForwardProbeRequiresAutoTurn(&saveState);
    const bool queuedOk =
        queuedResult == 1 &&
        PendingContactQueueCleared(playerState.preferredCollisionQueue);

    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_VariantTag_Current = oldVariantTagCurrent;

    if (!earlyOk) {
        return 1;
    }
    if (!clearOk) {
        return 2;
    }
    return queuedOk ? 0 : 3;
}

extern "C" int player_ai_choose_next_path_branch_index_smoke(void) {
    zUtil_SaveGameState saveState = {};
    AINetNode currentNode = {};
    AINetNode branch0 = {};
    AINetNode branch1 = {};
    AINetNode branch2 = {};
    AINetNode *currentNodePtr = &currentNode;
    int outBranchIndex = 99;

    currentNode.neighborNodes[0] = &branch0;
    int result = Player::AiChooseNextPathBranchIndex(&saveState, &currentNodePtr,
                                                     &outBranchIndex, -1);
    if (result != 1 || outBranchIndex != 0) {
        return 1;
    }

    currentNode.neighborNodes[1] = &branch1;
    outBranchIndex = 99;
    result = Player::AiChooseNextPathBranchIndex(&saveState, &currentNodePtr,
                                                 &outBranchIndex, 0);
    if (result != 1 || outBranchIndex != 1) {
        return 2;
    }

    currentNode.neighborNodes[2] = &branch2;
    outBranchIndex = 99;
    result = Player::AiChooseNextPathBranchIndex(&saveState, &currentNodePtr,
                                                 &outBranchIndex, -1);
    return result == 1 && outBranchIndex >= 0 && outBranchIndex < 3 ? 0 : 3;
}

extern "C" int player_ai_advance_path_cursor_and_compute_target_vec_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;
    playerState.worldPos = {10.0f, 20.0f, 30.0f};

    AINetNode previousNode = {};
    AINetNode nextNode = {};
    AINetNode forwardNode = {};
    AINetPathProbeFan nextFan = {};
    previousNode.nodeIndex = 10;
    nextNode.nodeIndex = 20;
    forwardNode.nodeIndex = 30;
    previousNode.neighborNodes[1] = &nextNode;
    nextNode.neighborNodes[0] = &previousNode;
    nextNode.neighborNodes[1] = &forwardNode;
    nextNode.probeFans[1] = &nextFan;
    nextNode.position = {1.0f, 2.0f, 3.0f};
    AINetNode *currentNode = &previousNode;
    playerState.aiCurrentPathNeighborIndex = 1;
    AINetPathProbeFan *outFan = nullptr;
    zVec3 targetVec = {};

    Player::AiAdvancePathCursorAndComputeTargetVec(&saveState, &currentNode, &outFan,
                                                   &targetVec);
    const bool normalOk =
        currentNode == &nextNode && playerState.aiCurrentPathNode == &nextNode &&
        playerState.aiCurrentPathNeighborIndex == 1 && outFan == &nextFan &&
        Vec3Equals(targetVec, {9.0f, 18.0f, 27.0f});

    AINetNode *negativeNode = AllocZeroedMalloc<AINetNode>();
    AINetNode positiveNode = {};
    AINetNode positiveForward = {};
    AINetPathProbeFan positiveFan = {};
    AINet aiNet = {};
    aiNet.aiType = AINET_TYPE_HI;
    negativeNode->nodeIndex = -1;
    negativeNode->neighborNodes[0] = &positiveNode;
    positiveNode.nodeIndex = 5;
    positiveForward.nodeIndex = 6;
    positiveNode.neighborNodes[0] = &positiveForward;
    positiveNode.probeFans[0] = &positiveFan;
    positiveNode.position = {4.0f, 6.0f, 8.0f};
    currentNode = negativeNode;
    playerState.aiNet = &aiNet;
    playerState.aiTopLevelState = 0;
    playerState.aiCurrentPathNeighborIndex = 0;
    outFan = nullptr;
    targetVec = {};

    Player::AiAdvancePathCursorAndComputeTargetVec(&saveState, &currentNode, &outFan,
                                                   &targetVec);
    const bool negativeOk =
        currentNode == &positiveNode && playerState.aiCurrentPathNode == &positiveNode &&
        playerState.aiCurrentPathNeighborIndex == 0 &&
        playerState.aiTopLevelState == 2 && outFan == &positiveFan &&
        Vec3Equals(targetVec, {6.0f, 14.0f, 22.0f});

    if (!normalOk) {
        return 1;
    }
    return negativeOk ? 0 : 2;
}

extern "C" int player_tick_ai_mode2_path_follow_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    AINetNode currentNode = {};
    AINetNode nextNode = {};
    AINetNode autoTargetNode = {};
    AINetPathProbeFan nextFan = {};
    zClass_NodePartial rootNode = {};
    zClass_WorldDataPartial emptyWorldData = {};
    zClass_NodePartial emptyWorld = {};
    emptyWorld.classData = &emptyWorldData;

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    playerState.rootNode = &rootNode;
    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    playerState.steerBasisNorm = {0.0f, 0.0f, 1.0f};
    playerState.projectileSpawnVel = {1.0f, 0.0f, 0.0f};
    currentNode.nodeIndex = 1;
    nextNode.nodeIndex = 2;
    autoTargetNode.nodeIndex = 3;
    currentNode.neighborNodes[0] = &nextNode;
    nextNode.neighborNodes[0] = &autoTargetNode;
    nextNode.probeFans[0] = &nextFan;
    nextNode.position = {0.0f, 0.0f, 20.0f};
    autoTargetNode.position = {3.0f, 0.0f, 4.0f};
    playerState.aiCurrentPathNode = &currentNode;
    playerState.aiCurrentPathNeighborIndex = 0;
    playerState.aiTopLevelState = 7;
    playerState.playerCollisionResolved = 1;
    g_Player_RuntimeDiScene = &emptyWorld;

    Player::TickAiMode2PathFollow(&saveState);
    const bool autoTurnOk =
        playerState.aiReturnTopLevelState == 7 && playerState.aiTopLevelState == 5 &&
        playerState.autoTurnActive == 1 &&
        FloatNear(playerState.autoTurnTargetDir.x, 0.6f) &&
        FloatNear(playerState.autoTurnTargetDir.z, 0.8f) &&
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        playerState.steeringInput == 0.0f;

    playerState = {};
    modalState = {};
    modalData = {};
    currentNode = {};
    nextNode = {};
    rootNode = {};
    emptyWorld = {};
    emptyWorld.classData = &emptyWorldData;
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    playerState.rootNode = &rootNode;
    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    playerState.steerBasisNorm = {0.0f, 0.0f, 1.0f};
    playerState.projectileSpawnVel = {1.0f, 0.0f, 0.0f};
    currentNode.nodeIndex = 10;
    nextNode.nodeIndex = 11;
    nextNode.position = {0.0f, 0.0f, 20.0f};
    currentNode.neighborNodes[0] = &nextNode;
    currentNode.probeFans[0] = &nextFan;
    playerState.aiCurrentPathNode = &currentNode;
    playerState.aiCurrentPathNeighborIndex = 0;

    Player::TickAiMode2PathFollow(&saveState);
    const bool followOk =
        playerState.aiCurrentPathNode == &currentNode &&
        playerState.aiCurrentPathNeighborIndex == 0 &&
        playerState.aiPathCursorAdvanceRequested == 1 &&
        FloatNear(playerState.throttleInput, 1.0f) &&
        FloatNear(playerState.throttleInputCopy, 1.0f) &&
        playerState.steeringInput == 0.0f &&
        playerState.steeringInputCopy == 0.0f;

    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_VariantTag_Current = oldVariantTagCurrent;

    if (!autoTurnOk) {
        return 1;
    }
    return followOk ? 0 : 2;
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

extern "C" int player_apply_damage_local_smoke(void) {
    const int oldQueueEnabled = g_zEffectAnim_RecordQueueEnabled;
    const int oldDispatchEnabled = g_zEffectAnim_DispatchEnabled;
    zEffectAnimActivationRecord *const oldRecordTable = g_zEffectAnim_ActivationRecordTable;
    const int oldRecordCapacity = g_zEffectAnim_ActivationRecordCapacity;
    const int oldRecordCount = g_zEffectAnim_ActivationRecordCount;
    HudUiSlot *const oldTrackedProgressSlot = g_HudUiMgrSensorTrackedProgressSlot;
    const HudUiMeter oldSensorMeter = g_HudUiMgrSensorMeter;
    const int oldDamageContextKind = g_OptCatalog_DamageContextKind;
    void *const oldDamageContextHitEvent = g_OptCatalog_DamageContextHitEvent;
    const float oldDamageFeedbackScalar = g_OptCatalogDamageFeedbackIntensityScalar;

    zUtil_SaveGameState liveSaveState = {};
    zUtil_PlayerStateStorage livePlayerState = {};
    PlayerMasterCommonData liveCommonData = {};
    liveSaveState.playerState = &livePlayerState;
    livePlayerState.masterCommonData = &liveCommonData;
    liveCommonData.invMaxHealth = 0.025f;
    livePlayerState.statusMeterValue = 20.0f;
    g_OptCatalogDamageFeedbackIntensityScalar = 0.0f;

    const int liveResult = Player::ApplyDamageLocal(&liveSaveState);
    const bool liveOk =
        liveResult == 0 && FloatNear(g_OptCatalogDamageFeedbackIntensityScalar, 0.5f) &&
        livePlayerState.destroyedRespawnAsyncHandle == nullptr;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial destroyedRuntimeNode = {};
    zClass_NodePartial recentRuntimeNode = {};
    zEffectAnimEntry destroyedRespawn = {};
    zEffectAnimEntry recentHitHandle = {};
    saveState.playerState = &playerState;
    playerState.masterCommonData = &commonData;
    playerState.rootNode = &rootNode;
    playerState.activeAltGunController = &playerState.altWeaponBanks[1].controllerA;
    playerState.statusMeterValue = 0.0f;
    playerState.destroyedRespawnFxEntry = &destroyedRespawn;
    playerState.recentHitValid = 1;
    playerState.recentHitLightHandle = &recentHitHandle;
    playerState.altGunTransitionState = 12;
    playerState.altGunTransitionController = &playerState.altWeaponBanks[2].controllerB;
    playerState.altGunTransitionTimerA = 3.0f;
    playerState.selectedProbeSample.node = &rootNode;
    rootNode.classId = 2;

    InitDestroyedEffectEntry(&destroyedRespawn, &rootNode, &destroyedRuntimeNode,
                             "destroyed_respawn");
    InitDestroyedEffectEntry(&recentHitHandle, &rootNode, &recentRuntimeNode, "recent_hit");
    recentHitHandle.activationState = 2;
    recentHitHandle.triggerCurrentValue = 8.0f;

    HudUiMeter_FTable visibleTable = {};
    visibleTable.slots[24] = MethodAddress(&TestPlayerHudVisibleReceiver::SetVisible);
    HudUiMgrSensorTrackNode trackNode = {};
    HudUiSlot trackedSlot = {};
    trackedSlot.trackNode = &trackNode;
    trackNode.payload = &saveState;
    std::memset(g_PlayerTestHudVisibleThis, 0, sizeof(g_PlayerTestHudVisibleThis));
    std::memset(g_PlayerTestHudVisibleValue, 0, sizeof(g_PlayerTestHudVisibleValue));
    g_PlayerTestHudVisibleCount = 0;
    g_HudUiMgrSensorMeter = {};
    g_HudUiMgrSensorMeter.ftable = &visibleTable;
    g_HudUiMgrSensorTrackedProgressSlot = &trackedSlot;

    g_zEffectAnim_RecordQueueEnabled = 1;
    g_zEffectAnim_DispatchEnabled = 0;
    zEffect_Anim::ClearActivationRecords();
    g_OptCatalog_DamageContextKind = -1;
    g_OptCatalog_DamageContextHitEvent = nullptr;

    const int depletedResult = Player::ApplyDamageLocal(&saveState);
    const bool depletedOk =
        depletedResult == 1 && playerState.destroyedRespawnAsyncHandle == &destroyedRespawn &&
        destroyedRespawn.activationState == 2 &&
        destroyedRespawn.eventCallback == (void *)(&Player::DestroyedStateRespawnCallback) &&
        destroyedRespawn.eventCallbackContext == &saveState && playerState.recentHitValid == 0 &&
        playerState.recentHitLightHandle == nullptr &&
        recentHitHandle.triggerCurrentValue == 0.0f &&
        playerState.altGunTransitionState == 1 &&
        playerState.altGunTransitionController == nullptr &&
        playerState.altGunTransitionTimerA == 0.0f && g_OptCatalog_DamageContextKind == 1 &&
        g_OptCatalog_DamageContextHitEvent == &playerState.selectedProbeSample &&
        g_PlayerTestHudVisibleCount == 1 && g_PlayerTestHudVisibleThis[0] == &g_HudUiMgrSensorMeter &&
        g_PlayerTestHudVisibleValue[0] == 0;

    zEffect_Anim::ClearActivationRecords();
    g_HudUiMgrSensorTrackedProgressSlot = oldTrackedProgressSlot;
    g_HudUiMgrSensorMeter = oldSensorMeter;
    g_zEffectAnim_ActivationRecordTable = oldRecordTable;
    g_zEffectAnim_ActivationRecordCapacity = oldRecordCapacity;
    g_zEffectAnim_ActivationRecordCount = oldRecordCount;
    g_zEffectAnim_RecordQueueEnabled = oldQueueEnabled;
    g_zEffectAnim_DispatchEnabled = oldDispatchEnabled;
    g_OptCatalog_DamageContextKind = oldDamageContextKind;
    g_OptCatalog_DamageContextHitEvent = oldDamageContextHitEvent;
    g_OptCatalogDamageFeedbackIntensityScalar = oldDamageFeedbackScalar;

    return liveOk && depletedOk ? 0 : 1;
}

extern "C" int player_tick_remote_network_player_smoke(void) {
    const int oldNameTags = g_GameNetStatus_NameTags;
    const float oldDamageFeedbackScalar = g_OptCatalogDamageFeedbackIntensityScalar;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    g_GameNetStatus_NameTags = 0;
    g_GameStateOrMapTable = nullptr;

    zUtil_SaveGameState transitionSaveState = {};
    zUtil_PlayerStateStorage transitionPlayerState = {};
    zClass_NodePartial transitionRootNode = {};
    zClass_Object3DDataPartial transitionRootData = {};
    GameNetPlayerRow transitionRow = {};
    transitionSaveState.playerState = &transitionPlayerState;
    transitionSaveState.netPlayerRow = &transitionRow;
    transitionPlayerState.rootNode = &transitionRootNode;
    transitionPlayerState.worldPos = {10.0f, 20.0f, 30.0f};
    transitionPlayerState.fxOffsetLocal = {1.0f, 2.0f, 3.0f};
    transitionPlayerState.netReceivedPos = {40.0f, 50.0f, 60.0f};
    transitionPlayerState.netReceivedAngles = {0.25f, 0.5f, 0.75f};
    transitionPlayerState.cameraTransitionTimer = 5;
    InitObjectPositionNode(&transitionRootNode, &transitionRootData, 0.0f, 0.0f, 0.0f);

    Player::TickRemoteNetworkPlayer(&transitionSaveState);

    const bool transitionOk =
        Vec3Equals(transitionPlayerState.fxOffsetWorld, {11.0f, 22.0f, 33.0f}) &&
        Vec3Equals(transitionPlayerState.worldPos, transitionPlayerState.netReceivedPos) &&
        FloatNear(transitionPlayerState.vehiclePitchRad, 0.25f) &&
        FloatNear(transitionPlayerState.restartYawRad, 0.5f) &&
        FloatNear(transitionPlayerState.vehicleRollRad, 0.75f) &&
        FloatNear(transitionRootData.localMatrix[9], 40.0f) &&
        FloatNear(transitionRootData.localMatrix[10], 50.0f) &&
        FloatNear(transitionRootData.localMatrix[11], 60.0f);

    zUtil_SaveGameState lerpSaveState = {};
    zUtil_PlayerStateStorage lerpPlayerState = {};
    PlayerMasterCommonData lerpCommonData = {};
    zClass_NodePartial lerpRootNode = {};
    zClass_Object3DDataPartial lerpRootData = {};
    GameNetPlayerRow lerpRow = {};
    lerpSaveState.playerState = &lerpPlayerState;
    lerpSaveState.netPlayerRow = &lerpRow;
    lerpPlayerState.rootNode = &lerpRootNode;
    lerpPlayerState.masterCommonData = &lerpCommonData;
    lerpCommonData.invMaxHealth = 0.01f;
    lerpPlayerState.worldPos = {0.0f, 10.0f, 20.0f};
    lerpPlayerState.fxOffsetLocal = {2.0f, 3.0f, 4.0f};
    lerpPlayerState.netReceivedPos = {10.0f, 20.0f, 40.0f};
    lerpPlayerState.netReceivedAngles = {1.0f, 1.5f, 2.0f};
    lerpPlayerState.cameraTransitionTimer = 0;
    lerpPlayerState.lifecycleState = 5;
    lerpPlayerState.statusMeterValue = 25.0f;
    g_OptCatalogDamageFeedbackIntensityScalar = 0.0f;
    InitObjectPositionNode(&lerpRootNode, &lerpRootData, 0.0f, 0.0f, 0.0f);

    Player::TickRemoteNetworkPlayer(&lerpSaveState);

    const bool lerpOk =
        Vec3Equals(lerpPlayerState.fxOffsetWorld, {2.0f, 13.0f, 24.0f}) &&
        FloatNear(lerpPlayerState.worldPos.x, 3.5f) &&
        FloatNear(lerpPlayerState.worldPos.y, 13.5f) &&
        FloatNear(lerpPlayerState.worldPos.z, 27.0f) &&
        FloatNear(lerpPlayerState.vehiclePitchRad, 1.0f) &&
        FloatNear(lerpPlayerState.restartYawRad, 1.5f) &&
        FloatNear(lerpPlayerState.vehicleRollRad, 2.0f) &&
        lerpPlayerState.cameraTransitionTimer == 0 &&
        FloatNear(g_OptCatalogDamageFeedbackIntensityScalar, 0.25f) &&
        FloatNear(lerpRootData.localMatrix[9], 3.5f) &&
        FloatNear(lerpRootData.localMatrix[10], 13.5f) &&
        FloatNear(lerpRootData.localMatrix[11], 27.0f);

    g_GameNetStatus_NameTags = oldNameTags;
    g_OptCatalogDamageFeedbackIntensityScalar = oldDamageFeedbackScalar;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    if (!transitionOk) {
        return 1;
    }
    return lerpOk ? 0 : 2;
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

extern "C" int player_hit_callback_record_net_context_and_timed_status_smoke(void) {
    void *const oldOwnerOrCtx = g_OptCatalog_CurrentDamageOwnerOrCtx;
    const float oldTime = g_Time_AccumulatedTimeSec;
    zEffectAnimEntry *const oldRecentHitFxAnimEntry = g_PlayerRecentHitFxAnimEntry;
    const int oldQueueEnabled = g_zEffectAnim_RecordQueueEnabled;
    const int oldDispatchEnabled = g_zEffectAnim_DispatchEnabled;
    zEffectAnimActivationRecord *const oldRecordTable = g_zEffectAnim_ActivationRecordTable;
    const int oldRecordCapacity = g_zEffectAnim_ActivationRecordCapacity;
    const int oldRecordCount = g_zEffectAnim_ActivationRecordCount;

    zUtil_SaveGameState inactiveSave = {};
    zUtil_PlayerStateStorage inactiveState = {};
    inactiveSave.playerState = &inactiveState;
    inactiveState.lifecycleState = 4;
    inactiveState.recentHitValid = 1;
    const bool inactiveOk =
        Player::HitCallback_RecordNetContextAndTimedStatus(&inactiveSave, nullptr, nullptr,
                                                           5.0f) == 0;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    zClass_NodePartial lightNode = {};
    zClass_NodePartial lightParent = {};
    saveState.playerState = &playerState;
    playerState.masterCommonData = &commonData;
    playerState.lifecycleState = 1;
    playerState.recentHitValid = 7;
    const bool nullSourceOk =
        Player::HitCallback_RecordNetContextAndTimedStatus(&saveState, nullptr, nullptr,
                                                           2.0f) == 7;

    OptCatalogEntryDef timedSource = {};
    timedSource.flags = 0x200000u;
    commonData.invMaxHealth = 0.25f;
    playerState.timedHitStatus = {};
    playerState.timedHitStatus.currentLevel = 0.25f;
    playerState.timedHitStatus.lightNode = &lightNode;
    playerState.timedHitStatus.lightParentNode = &lightParent;
    playerState.recentHitValid = 5;
    const int timedResult =
        Player::HitCallback_RecordNetContextAndTimedStatus(&saveState, &timedSource, nullptr,
                                                           2.0f);
    const bool timedOk = timedResult == 5 && playerState.timedHitStatus.hitSource == &timedSource &&
                         playerState.timedHitStatus.targetLevel == 0.5f;

    zClass_NodePartial rootNode = {};
    zClass_NodePartial runtimeNode = {};
    zEffectAnimEntry recentHitEffect = {};
    rootNode.classId = 2;
    InitDestroyedEffectEntry(&recentHitEffect, &rootNode, &runtimeNode, "recent_hit");
    recentHitEffect.priority = 3;
    OptCatalogEntryDef recentSource = {};
    recentSource.flags = 0x1000u;
    int owner = 0;
    playerState.rootNode = &rootNode;
    playerState.recentHitValid = 0;
    g_PlayerRecentHitFxAnimEntry = &recentHitEffect;
    g_OptCatalog_CurrentDamageOwnerOrCtx = &owner;
    g_Time_AccumulatedTimeSec = 20.0f;
    g_zEffectAnim_RecordQueueEnabled = 1;
    g_zEffectAnim_DispatchEnabled = 0;
    zEffect_Anim::ClearActivationRecords();
    const int recentResult =
        Player::HitCallback_RecordNetContextAndTimedStatus(&saveState, &recentSource, nullptr,
                                                           3.0f);
    const bool recentOk =
        recentResult == 1 && playerState.recentHitValid == 1 &&
        playerState.lastHitOwnerOrCtx == &owner && playerState.recentHitSource == &recentSource &&
        playerState.recentHitDamage == 3.0f && playerState.recentHitFxExpireTime == 24.0f &&
        playerState.recentHitLightHandle == &recentHitEffect;

    zEffect_Anim::ClearActivationRecords();
    g_zEffectAnim_ActivationRecordTable = oldRecordTable;
    g_zEffectAnim_ActivationRecordCapacity = oldRecordCapacity;
    g_zEffectAnim_ActivationRecordCount = oldRecordCount;
    g_zEffectAnim_RecordQueueEnabled = oldQueueEnabled;
    g_zEffectAnim_DispatchEnabled = oldDispatchEnabled;
    g_PlayerRecentHitFxAnimEntry = oldRecentHitFxAnimEntry;
    g_OptCatalog_CurrentDamageOwnerOrCtx = oldOwnerOrCtx;
    g_Time_AccumulatedTimeSec = oldTime;

    return inactiveOk && nullSourceOk && timedOk && recentOk ? 0 : 1;
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

extern "C" int player_los_from_fx_offset_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    PlayerProbeSampleCandidateBuffer *const oldPickBuffer = g_DiPickCandidateBuffer;
    const int oldBreakOnFirst = g_cls_di_BreakOnFirstCandidate;
    const int oldStopAfterFirst = g_cls_di_StopAfterFirstHit;

    zUtil_PlayerStateStorage playerState = {};
    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    zClass_NodePartial excludedNode = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial worldNode = {};
    zClass_WorldDataPartial worldData = {};
    zClass_NodePartial cameraNode = {};
    zClass_CameraDataPartial cameraData = {};
    const zVec3 targetPoint = {10.0f, 20.0f, 30.0f};

    playerState.rootNode = &rootNode;
    playerState.fxOffsetWorld = {1.0f, 2.0f, 3.0f};
    playerState.variantTag.count = 2;
    playerState.variantTag.tags[0] = 7;
    playerState.variantTag.tags[1] = 8;
    playerState.variantTag.tags[2] = 0xff;
    gameStateOrMap.playerState =
        static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&playerState));
    worldNode.classData = &worldData;
    excludedNode.flags = 0x10;
    rootNode.flags = 0x10;
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    cameraData.targetOrEuler = {4.0f, 5.0f, 6.0f};

    g_GameStateOrMapTable = &gameStateOrMap;
    g_Player_RuntimeDiScene = &worldNode;
    g_MainCamera = &cameraNode;
    g_cls_di_BreakOnFirstCandidate = 99;
    g_cls_di_StopAfterFirstHit = 99;

    const int forwardResult =
        Player::HasLineOfSightFromLocalPlayerFxOffset(&excludedNode, &targetPoint, 1);
    const bool forwardOk =
        forwardResult == 1 && FloatNear(g_DiPickQueryPoint.x, 1.0f) &&
        FloatNear(g_DiPickQueryPoint.y, 2.0f) && FloatNear(g_DiPickQueryPoint.z, 3.0f) &&
        FloatNear(g_DiSegmentEnd.x, 10.0f) && FloatNear(g_DiSegmentEnd.y, 20.0f) &&
        FloatNear(g_DiSegmentEnd.z, 30.0f) &&
        (excludedNode.flags & 0x10) != 0 && (rootNode.flags & 0x10) != 0 &&
        g_cls_di_BreakOnFirstCandidate == 0 && g_cls_di_StopAfterFirstHit == 0 &&
        g_Variant_CurrentTag.count == 2 && g_Variant_CurrentTag.tags[0] == 7 &&
        g_Variant_CurrentTag.tags[1] == 8;

    excludedNode.flags = 0x10;
    rootNode.flags = 0x10;
    const int reverseResult =
        Player::HasLineOfSightFromLocalPlayerFxOffset(&excludedNode, &targetPoint, 0);
    const bool reverseOk =
        reverseResult == 1 && FloatNear(g_DiPickQueryPoint.x, 10.0f) &&
        FloatNear(g_DiPickQueryPoint.y, 20.0f) && FloatNear(g_DiPickQueryPoint.z, 30.0f) &&
        FloatNear(g_DiSegmentEnd.x, 1.0f) && FloatNear(g_DiSegmentEnd.y, 2.0f) &&
        FloatNear(g_DiSegmentEnd.z, 3.0f) &&
        (excludedNode.flags & 0x10) != 0 && (rootNode.flags & 0x10) != 0;

    excludedNode.flags = 0x10;
    rootNode.flags = 0x10;
    const int cameraForwardResult =
        Player::TestScenePathBetweenCameraTargetAndPoint(&excludedNode, &targetPoint, 1);
    const bool cameraForwardOk =
        cameraForwardResult == 1 && FloatNear(g_DiPickQueryPoint.x, 4.0f) &&
        FloatNear(g_DiPickQueryPoint.y, 5.0f) && FloatNear(g_DiPickQueryPoint.z, 6.0f) &&
        FloatNear(g_DiSegmentEnd.x, 10.0f) && FloatNear(g_DiSegmentEnd.y, 20.0f) &&
        FloatNear(g_DiSegmentEnd.z, 30.0f) &&
        (excludedNode.flags & 0x10) != 0 && (rootNode.flags & 0x10) != 0 &&
        g_Variant_CurrentTag.count == 2 && g_Variant_CurrentTag.tags[0] == 7 &&
        g_Variant_CurrentTag.tags[1] == 8;

    excludedNode.flags = 0x10;
    rootNode.flags = 0x10;
    const int cameraReverseResult =
        Player::TestScenePathBetweenCameraTargetAndPoint(&excludedNode, &targetPoint, 0);
    const bool cameraReverseOk =
        cameraReverseResult == 1 && FloatNear(g_DiPickQueryPoint.x, 10.0f) &&
        FloatNear(g_DiPickQueryPoint.y, 20.0f) && FloatNear(g_DiPickQueryPoint.z, 30.0f) &&
        FloatNear(g_DiSegmentEnd.x, 4.0f) && FloatNear(g_DiSegmentEnd.y, 5.0f) &&
        FloatNear(g_DiSegmentEnd.z, 6.0f) &&
        (excludedNode.flags & 0x10) != 0 && (rootNode.flags & 0x10) != 0;

    g_cls_di_StopAfterFirstHit = oldStopAfterFirst;
    g_cls_di_BreakOnFirstCandidate = oldBreakOnFirst;
    g_DiPickCandidateBuffer = oldPickBuffer;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_MainCamera = oldMainCamera;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_GameStateOrMapTable = oldGameStateOrMapTable;

    return forwardOk && reverseOk && cameraForwardOk && cameraReverseOk ? 0 : 1;
}

extern "C" int player_apply_aim_pitch_to_direction_smoke(void) {
    zVec3 direction = {3.0f, 9.0f, 4.0f};
    Player::ApplyAimPitchToDirection(&direction, 0.0f);
    const float scale = PlayerFastSqrtEstimateForTest(1.0f / 25.0f);

    int failed = 0;
    failed |= !FloatNear(direction.x, 3.0f * scale);
    failed |= direction.y != 0.0f;
    failed |= !FloatNear(direction.z, 4.0f * scale);

    direction = {0.0f, 12.0f, 0.0f};
    Player::ApplyAimPitchToDirection(&direction, 0.0f);
    failed |= !Vec3Equals(direction, zVec3{0.0f, 0.0f, -1.0f});

    direction = {0.0f, 0.0f, 0.0f};
    Player::ApplyAimPitchToDirection(&direction, 0.5f);
    const float diagonal = PlayerFastSqrtEstimateForTest((1.0f - 0.25f) * 0.5f);
    failed |= !FloatNear(direction.x, diagonal);
    failed |= direction.y != 0.5f;
    failed |= !FloatNear(direction.z, diagonal);

    return failed != 0 ? 1 : 0;
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
        FloatNear(playerState.aiHideTime0, 100.0f) &&
        FloatNear(playerState.aiHideTime1, 100.0f) &&
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
