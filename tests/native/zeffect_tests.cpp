#include "Battlesport/HudSensorTracker.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zUtil/zZbd.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "zDi.h"

#include <windows.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

extern "C" unsigned int g_HudUi_InvalidateMask;
extern "C" int g_Hud_MapOverlayRefCount;

namespace {
constexpr std::size_t kEffectFxPass3ConfigSize = 0x1f0;
constexpr std::size_t kEffectFxPass3RootElementOffset = 0x28;
constexpr std::size_t kEffectFxPass3RootPackedColorOffset = 0x60;
constexpr std::size_t kEffectFxPass3RootAlphaOffset = 0x68;
constexpr std::size_t kEffectFxPass3SlotsOffset = 0x70;
constexpr std::size_t kEffectFxPass3SlotSize = 0x4c;
constexpr std::size_t kEffectFxPass3SlotCurrentRadiusOffset = 0x38;
constexpr std::size_t kEffectFxPass3SlotMaxRadiusOffset = 0x3c;
constexpr std::size_t kEffectFxPass3SlotExtentOffset = 0x40;
constexpr std::size_t kEffectFxPass3SlotSinFreqOffset = 0x44;
constexpr std::size_t kEffectFxPass3SlotSinPhaseOffset = 0x48;
constexpr std::size_t kEffectFxPass3SlotWriteIndexOffset = 0x1ec;

int g_hudSensorVisibleCallCount = 0;
int g_hudSensorLastVisible = -1;
int g_hudSensorDeleteCallCount = 0;
std::uint32_t g_hudSensorDeleteFlags = 0;
int g_effectAnimDispatchCallCount = 0;
zEffectAnimActivationRecord *g_effectAnimDispatchRecord = nullptr;
int g_effectAnimEventCallbackCallCount = 0;
zEffectAnimEntry *g_effectAnimEventCallbackSelf = nullptr;
void *g_effectAnimEventCallbackContext = nullptr;
int g_zbdDataReadyCallCount = 0;
zZbdSectionCallbackCtx *g_zbdDataReadyCtx = nullptr;
const char *g_zbdDataReadyToken = nullptr;
void *g_zbdDataReadyBuffer = nullptr;
unsigned int g_zbdDataReadySize = 0;
void *g_zbdDataReadyUserData = nullptr;
int g_zbdLoadCallCount = 0;
int g_zbdPreLoadCallCount = 0;
zZbdSectionCallbackCtx *g_zbdPreLoadCtx = nullptr;
void *g_zbdPreLoadUserData = nullptr;

struct ZbdLoadDataReadyCall {
    zZbdManager *manager;
    zZbdSectionHandler *sectionHandler;
    void *userData;
    char sectionToken[32];
    unsigned int size;
    unsigned char bytes[16];
};

ZbdLoadDataReadyCall g_zbdLoadCalls[4] = {};

struct ZbdLoadDataReadyControl {
    bool stopOnFirstCall;
};

struct ZbdPreLoadCall {
    zZbdManager *manager;
    zZbdSectionHandler *sectionHandler;
    void *userData;
    char sectionName[32];
};

ZbdPreLoadCall g_zbdPreLoadCalls[4] = {};

struct ZbdPreLoadControl {
    int result;
    int failAtCall;
    bool writeRecord;
};

bool WriteHudSensorTestMap(const char *path,
                           const HudSensorMapBounds &fileBounds,
                           const unsigned char (&rgb)[3],
                           const HudSensorMapPoint *points,
                           int pointCount,
                           int objectiveIndex) {
    std::FILE *stream = std::fopen(path, "wb");
    if (stream == nullptr) {
        return false;
    }

    const std::int32_t version = 5;
    const std::int32_t header = 0x13572468;
    const bool ok = std::fwrite(&version, sizeof(version), 1, stream) == 1 &&
                    std::fwrite(&header, sizeof(header), 1, stream) == 1 &&
                    std::fwrite(&fileBounds, sizeof(fileBounds), 1, stream) == 1 &&
                    std::fwrite(rgb, 3, 1, stream) == 1 &&
                    std::fwrite(&pointCount, sizeof(pointCount), 1, stream) == 1 &&
                    std::fwrite(points, sizeof(points[0]), static_cast<std::size_t>(pointCount),
                                stream) == static_cast<std::size_t>(pointCount) &&
                    std::fwrite(&objectiveIndex, sizeof(objectiveIndex), 1, stream) == 1;

    std::fclose(stream);
    return ok;
}

struct EffectAnimLoadZbdHeaderBlock {
    int entriesInstantiated;
    void *heapPtr;
    short countsPackedLoWord;
    short entryCount;
    zEffectAnimEntry *entryList;
    int textIdEntryCount;
    zEffectAnimTextIdEntry *textIdEntryList;
    zClass_NodePartial *worldNode;
    float defaultGravity;
    int conditionalRefPosEnabled;
    int variantOverrideEnabled;
    float conditionalRefPosX;
    float conditionalRefPosY;
    float conditionalRefPosZ;
    unsigned int variantOverridePackedIds;
    float frameDeltaRemainingSec;
};

static_assert(sizeof(EffectAnimLoadZbdHeaderBlock) == 0x3c,
              "LoadZbd smoke header mirror must match the saved ZBD header block");

bool WriteEffectAnimLoadZbdMinimalFile(const char *path,
                                       const EffectAnimLoadZbdHeaderBlock &header) {
    std::FILE *stream = std::fopen(path, "wb");
    if (stream == nullptr) {
        return false;
    }

    const int signature = 0x08170616;
    const int stampRecordSize = sizeof(zEffectAnimSourceFileStamp);
    const int stampCount = 0;
    const bool ok = std::fwrite(&signature, sizeof(signature), 1, stream) == 1 &&
                    std::fwrite(&stampRecordSize, sizeof(stampRecordSize), 1, stream) == 1 &&
                    std::fwrite(&stampCount, sizeof(stampCount), 1, stream) == 1 &&
                    std::fwrite(&header, sizeof(header), 1, stream) == 1;
    std::fclose(stream);
    return ok;
}
std::int32_t g_effectAnimEventCallbackValue = 0;
int g_effectDirectSoundGetFrequencyCount = 0;
int g_effectDirectSoundGetStatusCount = 0;
int g_effectDirectSoundSetVolumeCount = 0;
int g_effectDirectSoundSetPanCount = 0;
int g_effectDirectSoundSetFrequencyCount = 0;
int g_effectDirectSoundSetCurrentPositionCount = 0;
int g_effectDirectSoundPlayCount = 0;

using EffectBackendGetStatusFn = std::int32_t(__stdcall *)(void *self, std::int32_t *status);
using EffectBackendGetUint32Fn = std::int32_t(__stdcall *)(void *self, std::uint32_t *value);
using EffectBackendSetIntFn = std::int32_t(__stdcall *)(void *self, std::int32_t value);
using EffectBackendPlayDirectSoundFn = std::int32_t(__stdcall *)(void *self,
                                                                 std::uint32_t reserved1,
                                                                 std::uint32_t reserved2,
                                                                 std::uint32_t flags);
using EffectBackendSimpleFn = std::int32_t(__stdcall *)(void *self);

struct EffectDirectSoundBufferVTable {
    void *slots00_1c[8];
    EffectBackendGetUint32Fn GetFrequency;
    EffectBackendGetStatusFn GetStatus;
    void *slot28;
    void *slot2c;
    EffectBackendPlayDirectSoundFn Play;
    EffectBackendSetIntFn SetCurrentPosition;
    void *slot38;
    EffectBackendSetIntFn SetVolume;
    EffectBackendSetIntFn SetPan;
    EffectBackendSetIntFn SetFrequency;
    EffectBackendSimpleFn Stop;
    void *slot4c;
    EffectBackendSimpleFn Restore;
};

struct EffectDirectSoundBuffer {
    EffectDirectSoundBufferVTable *vtable;
};

void ResetEffectDirectSoundCounters() {
    g_effectDirectSoundGetFrequencyCount = 0;
    g_effectDirectSoundGetStatusCount = 0;
    g_effectDirectSoundSetVolumeCount = 0;
    g_effectDirectSoundSetPanCount = 0;
    g_effectDirectSoundSetFrequencyCount = 0;
    g_effectDirectSoundSetCurrentPositionCount = 0;
    g_effectDirectSoundPlayCount = 0;
}

template <typename T> T &EffectTestFieldAt(void *base, std::size_t offset) {
    return *reinterpret_cast<T *>(static_cast<std::uint8_t *>(base) + offset);
}

std::uint8_t *EffectFxPass3ConfigBytes() {
    return reinterpret_cast<std::uint8_t *>(&g_zVideo_FxPass3ConfigLocal);
}

template <typename T> T &EffectFxPass3FieldAt(std::size_t offset) {
    return *reinterpret_cast<T *>(EffectFxPass3ConfigBytes() + offset);
}

HudUiPanel *EffectTextStackLineAt(HudUiTextStack4 *stack, int index) {
    return reinterpret_cast<HudUiPanel *>(&stack->lines[index][0]);
}

void EffectDeleteTextStackLineFonts(HudUiTextStack4 *stack) {
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = EffectTextStackLineAt(stack, index);
        DeleteObject(panel->hFont);
        panel->hFont = nullptr;
    }
}

std::int32_t __stdcall EffectDirectSoundGetFrequency(void *, std::uint32_t *value) {
    ++g_effectDirectSoundGetFrequencyCount;
    *value = 22050;
    return 0;
}

std::int32_t __stdcall EffectDirectSoundGetStatus(void *, std::int32_t *status) {
    ++g_effectDirectSoundGetStatusCount;
    *status = 0;
    return 0;
}

std::int32_t __stdcall EffectDirectSoundSetVolume(void *, std::int32_t) {
    ++g_effectDirectSoundSetVolumeCount;
    return 0;
}

std::int32_t __stdcall EffectDirectSoundSetPan(void *, std::int32_t) {
    ++g_effectDirectSoundSetPanCount;
    return 0;
}

std::int32_t __stdcall EffectDirectSoundSetFrequency(void *, std::int32_t) {
    ++g_effectDirectSoundSetFrequencyCount;
    return 0;
}

std::int32_t __stdcall EffectDirectSoundSetCurrentPosition(void *, std::int32_t) {
    ++g_effectDirectSoundSetCurrentPositionCount;
    return 0;
}

std::int32_t __stdcall EffectDirectSoundPlay(void *, std::uint32_t, std::uint32_t,
                                             std::uint32_t) {
    ++g_effectDirectSoundPlayCount;
    return 0;
}

template <typename Method> std::uintptr_t MethodAddress(Method method) {
    static_assert(sizeof(method) <= sizeof(std::uintptr_t));
    std::uintptr_t address = 0;
    std::memcpy(&address, &method, sizeof(method));
    return address;
}

bool FloatNear(float lhs, float rhs) {
    return std::fabs(lhs - rhs) < 0.0001f;
}

struct HudSensorTestElement : HudUiElement {
    HudSensorTestElement *RECOIL_THISCALL ScalarDeletingDestructor(std::uint32_t flags) {
        ++g_hudSensorDeleteCallCount;
        g_hudSensorDeleteFlags = flags;
        if ((flags & 1u) != 0) {
            ::operator delete(this);
        }

        return this;
    }

    void RECOIL_THISCALL SetVisible(std::int32_t visible) {
        ++g_hudSensorVisibleCallCount;
        g_hudSensorLastVisible = visible;
    }
};

HudUiCommon_FTable MakeHudSensorTestFTable() {
    HudUiCommon_FTable table = {};
    table.slots[0] = MethodAddress(&HudSensorTestElement::ScalarDeletingDestructor);
    table.slots[24] = MethodAddress(&HudSensorTestElement::SetVisible);
    return table;
}

bool CStringIsEmpty(const CString &value) {
    return value.m_pchData != nullptr && value.m_pchData[0] == '\0';
}

bool CStringEquals(const CString &value, const char *text) {
    return value.m_pchData != nullptr && std::strcmp(value.m_pchData, text) == 0;
}

void ResetZClassRuntimeForZEffectTest() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;
}

void FreeZClassRuntimeForZEffectTest() {
    for (int i = 0; i < 16; ++i) {
        for (zClass_TypeListLink *link = zClass_TypeList::Head(i); link != nullptr;) {
            zClass_TypeListLink *next = link->next;
            std::free(link);
            link = next;
        }
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }

    for (zClass_TypeListLink *link = g_zClass_TypeList_FreeLinkHead; link != nullptr;) {
        zClass_TypeListLink *next = link->next;
        std::free(link);
        link = next;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;
}

void EnsureZrdrFreePool() {
    if (g_zUtil_ZRDR_FreePool == nullptr) {
        g_zUtil_ZRDR_FreePool = zArchiveList_CreateEmpty();
    }
}

void WriteEffectTestU32(HANDLE file, std::uint32_t value) {
    DWORD written = 0;
    WriteFile(file, &value, sizeof(value), &written, nullptr);
}

void WriteEffectTestBytes(HANDLE file, const void *data, std::uint32_t size) {
    DWORD written = 0;
    WriteFile(file, data, size, &written, nullptr);
}

void WriteEffectTestZrdArray(HANDLE file, std::uint32_t count) {
    WriteEffectTestU32(file, zReader::ZRDR_NODE_ARRAY);
    WriteEffectTestU32(file, count);
}

void WriteEffectTestZrdString(HANDLE file, const char *text) {
    const std::uint32_t length = static_cast<std::uint32_t>(std::strlen(text));
    WriteEffectTestU32(file, zReader::ZRDR_NODE_STRING);
    WriteEffectTestU32(file, length);
    WriteEffectTestBytes(file, text, length);
}

void WriteEffectTestZrdFloat(HANDLE file, float value) {
    WriteEffectTestU32(file, zReader::ZRDR_NODE_FLOAT);
    WriteEffectTestBytes(file, &value, sizeof(value));
}

void RECOIL_FASTCALL EffectAnimDispatchCallback(zEffectAnimActivationRecord *record) {
    ++g_effectAnimDispatchCallCount;
    g_effectAnimDispatchRecord = record;
}

void RECOIL_FASTCALL EffectAnimEventCallback(zEffectAnimEntry *self, void *context,
                                             std::int32_t value) {
    ++g_effectAnimEventCallbackCallCount;
    g_effectAnimEventCallbackSelf = self;
    g_effectAnimEventCallbackContext = context;
    g_effectAnimEventCallbackValue = value;
}

zZbdManager MakeManager(zZbdSectionHandlerNode &sentinel) {
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    sentinel.sectionHandler = {};

    zZbdManager manager = {};
    manager.sectionHandlerListSentinel = &sentinel;
    return manager;
}

void ClearRegisteredHandlers(zZbdSectionHandlerNode &sentinel) {
    zZbdSectionHandlerNode *node = sentinel.next;
    while (node != &sentinel) {
        zZbdSectionHandlerNode *next = node->next;
        delete node;
        node = next;
    }
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
}

void StoreFloatBits(std::uint32_t &target, float value) {
    std::memcpy(&target, &value, sizeof(value));
}

void RECOIL_FASTCALL TestZbdDataReadyCallback(zZbdSectionCallbackCtx *callbackCtx,
                                              const char *sectionToken, void *buffer,
                                              unsigned int size, void *userData) {
    ++g_zbdDataReadyCallCount;
    g_zbdDataReadyCtx = callbackCtx;
    g_zbdDataReadyToken = sectionToken;
    g_zbdDataReadyBuffer = buffer;
    g_zbdDataReadySize = size;
    g_zbdDataReadyUserData = userData;
}

void ResetZbdLoadCalls() {
    g_zbdLoadCallCount = 0;
    std::memset(g_zbdLoadCalls, 0, sizeof(g_zbdLoadCalls));
}

void ResetZbdPreLoadCalls() {
    g_zbdPreLoadCallCount = 0;
    g_zbdPreLoadCtx = nullptr;
    g_zbdPreLoadUserData = nullptr;
    std::memset(g_zbdPreLoadCalls, 0, sizeof(g_zbdPreLoadCalls));
}

int RECOIL_FASTCALL TestZbdPreLoadCallback(zZbdSectionCallbackCtx *callbackCtx,
                                           void *userData) {
    g_zbdPreLoadCtx = callbackCtx;
    g_zbdPreLoadUserData = userData;

    const int index = g_zbdPreLoadCallCount++;
    if (index < static_cast<int>(sizeof(g_zbdPreLoadCalls) / sizeof(g_zbdPreLoadCalls[0]))) {
        ZbdPreLoadCall &call = g_zbdPreLoadCalls[index];
        call.manager = callbackCtx->manager;
        call.sectionHandler = callbackCtx->sectionHandler;
        call.userData = userData;
        if (callbackCtx->sectionHandler != nullptr &&
            callbackCtx->sectionHandler->sectionName != nullptr) {
            std::strncpy(call.sectionName, callbackCtx->sectionHandler->sectionName,
                         sizeof(call.sectionName) - 1);
        }
    }

    ZbdPreLoadControl *const control = static_cast<ZbdPreLoadControl *>(userData);
    if (control == nullptr) {
        return 1;
    }

    if (control->writeRecord) {
        const std::uint32_t payload = static_cast<std::uint32_t>(index + 1);
        if (callbackCtx->manager->WriteSectionRecord(callbackCtx, "PreLoad", &payload,
                                                     sizeof(payload)) == 0) {
            return 0;
        }
    }

    if (control->failAtCall == index) {
        return 0;
    }

    return control->result;
}

void RECOIL_FASTCALL TestZbdLoadDataReadyCallback(zZbdSectionCallbackCtx *callbackCtx,
                                                  const char *sectionToken, void *buffer,
                                                  unsigned int size, void *userData) {
    const int index = g_zbdLoadCallCount++;
    if (index < static_cast<int>(sizeof(g_zbdLoadCalls) / sizeof(g_zbdLoadCalls[0]))) {
        ZbdLoadDataReadyCall &call = g_zbdLoadCalls[index];
        call.manager = callbackCtx->manager;
        call.sectionHandler = callbackCtx->sectionHandler;
        call.userData = userData;
        std::strncpy(call.sectionToken, sectionToken, sizeof(call.sectionToken) - 1);
        call.size = size;

        const unsigned int copySize =
            size < sizeof(call.bytes) ? size : static_cast<unsigned int>(sizeof(call.bytes));
        std::memcpy(call.bytes, buffer, copySize);
    }

    ZbdLoadDataReadyControl *const control =
        static_cast<ZbdLoadDataReadyControl *>(userData);
    if (control != nullptr && control->stopOnFirstCall) {
        callbackCtx->manager->RequestStop();
    }
}

template <typename T>
zZbdSectionCallback TestZbdCallbackPtr(T callback) {
    static_assert(sizeof(T) == sizeof(zZbdSectionCallback));
    union {
        T callback;
        zZbdSectionCallback raw;
    } value = {};
    value.callback = callback;
    return value.raw;
}
} // namespace

extern "C" int zutil_zar_register_section_handler_smoke(void) {
    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager = MakeManager(sentinel);
    g_zUtil_ZbdManager = &manager;

    zUtil_ZAR::RegisterSectionHandler("Anim", reinterpret_cast<void *>(1),
                                      reinterpret_cast<void *>(2), 0x34,
                                      reinterpret_cast<void *>(3));
    zUtil_ZAR::RegisterSectionHandler("Anim", reinterpret_cast<void *>(4),
                                      reinterpret_cast<void *>(5), 0x35,
                                      reinterpret_cast<void *>(6));

    const bool ok = manager.sectionHandlerCount == 1 && sentinel.next != &sentinel &&
                    sentinel.prev == sentinel.next &&
                    sentinel.next->sectionHandler.sectionName != nullptr &&
                    std::strcmp(sentinel.next->sectionHandler.sectionName, "Anim") == 0 &&
                    sentinel.next->sectionHandler.onPreLoad == reinterpret_cast<void *>(1) &&
                    sentinel.next->sectionHandler.onDataReady == reinterpret_cast<void *>(2) &&
                    sentinel.next->sectionHandler.sortOrder == 0x34 &&
                    sentinel.next->sectionHandler.userData == reinterpret_cast<void *>(3);

    ClearRegisteredHandlers(sentinel);
    g_zUtil_ZbdManager = nullptr;
    return ok ? 0 : 1;
}

extern "C" int zutil_zbd_init_smoke(void) {
    g_zUtil_ZbdManager = nullptr;
    if (zUtil::ZBD_Init() != 0 || g_zUtil_ZbdManager == nullptr) {
        return 1;
    }

    zZbdManager *manager = g_zUtil_ZbdManager;
    const bool ok =
        manager->sectionHandlerListSentinel != nullptr &&
        manager->sectionHandlerListSentinel->next == manager->sectionHandlerListSentinel &&
        manager->sectionHandlerListSentinel->prev == manager->sectionHandlerListSentinel &&
        manager->sectionHandlerCount == 0 && manager->indexArchive.hFile == INVALID_HANDLE_VALUE &&
        manager->indexArchive.records == nullptr && manager->tempBufferSize == 0 &&
        manager->tempBuffer == nullptr;

    delete manager->sectionHandlerListSentinel;
    delete manager;
    g_zUtil_ZbdManager = nullptr;
    return ok ? 0 : 2;
}

extern "C" int zutil_zbd_destroy_global_manager_smoke(void) {
    g_zUtil_ZbdManager = nullptr;
    if (zUtil::ZBD_Init() != 0 || g_zUtil_ZbdManager == nullptr) {
        return 1;
    }

    zZbdManager *const manager = g_zUtil_ZbdManager;
    manager->tempBuffer = ::operator new(32);
    manager->tempBufferSize = 32;
    manager->indexArchive.records =
        static_cast<zZarFileRecord *>(std::calloc(1, sizeof(zZarFileRecord)));
    manager->indexArchive.recordCount = 1;
    manager->indexArchive.recordCapacity = 1;

    manager->RegisterSectionHandler("Mission", reinterpret_cast<void *>(1),
                                    reinterpret_cast<void *>(2), 0x20,
                                    reinterpret_cast<void *>(3));
    manager->RegisterSectionHandler("MissionLate", reinterpret_cast<void *>(4),
                                    reinterpret_cast<void *>(5), 0x30,
                                    reinterpret_cast<void *>(6));
    if (manager->sectionHandlerCount != 2) {
        zUtil::ZBD_DestroyGlobalManager();
        return 2;
    }

    zUtil::ZBD_DestroyGlobalManager();
    if (g_zUtil_ZbdManager != nullptr) {
        zUtil::ZBD_DestroyGlobalManager();
        return 3;
    }

    zUtil::ZBD_DestroyGlobalManager();
    return g_zUtil_ZbdManager == nullptr ? 0 : 4;
}

extern "C" int hud_sensor_register_mission_sections_smoke(void) {
    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager = MakeManager(sentinel);
    g_zUtil_ZbdManager = &manager;

    HudSensorTracker tracker = {};
    tracker.RegisterMissionSectionHandlers();

    zZbdSectionHandlerNode *mission = sentinel.next;
    zZbdSectionHandlerNode *lateMission = mission->next;
    const bool ok = manager.sectionHandlerCount == 2 && mission != &sentinel &&
                    lateMission != &sentinel && lateMission->next == &sentinel &&
                    std::strcmp(mission->sectionHandler.sectionName, "Mission") == 0 &&
                    mission->sectionHandler.sortOrder == 0 &&
                    mission->sectionHandler.userData == &tracker &&
                    mission->sectionHandler.onPreLoad != nullptr &&
                    mission->sectionHandler.onDataReady != nullptr &&
                    std::strcmp(lateMission->sectionHandler.sectionName, "MissionLate") == 0 &&
                    lateMission->sectionHandler.sortOrder == 0x7d0 &&
                    lateMission->sectionHandler.userData == &tracker &&
                    lateMission->sectionHandler.onPreLoad != nullptr &&
                    lateMission->sectionHandler.onDataReady != nullptr;

    ClearRegisteredHandlers(sentinel);
    g_zUtil_ZbdManager = nullptr;
    return ok ? 0 : 1;
}

extern "C" int hud_sensor_mission_identity_smoke(void) {
    HudSensorTracker tracker = {};
    if (tracker.SetZbdPath("missions\\m01.zbd") != 1 ||
        !CStringEquals(tracker.zbdPath, "missions\\m01.zbd")) {
        return 1;
    }

    const bool initOk = tracker.InitMissionIdAndFlags(7, 0x55) == 1 && tracker.missionId == 7 &&
                        tracker.GetMissionId() == 7 && tracker.missionFlags == 0x55 &&
                        CStringIsEmpty(tracker.zbdPath);

    const bool clearOk = tracker.SetZbdPath("alternate.zbd") == 1 &&
                         CStringEquals(tracker.zbdPath, "alternate.zbd") &&
                         tracker.SetZbdPath(nullptr) == 1 && CStringIsEmpty(tracker.zbdPath);

    const bool setIdOk = tracker.SetZbdPath("pending.zbd") == 1 && tracker.SetMissionId(12) == 1 &&
                         tracker.GetMissionId() == 12 && CStringIsEmpty(tracker.zbdPath);

    return initOk && clearOk && setIdOk ? 0 : 2;
}

extern "C" int hud_sensor_map_node_basics_smoke(void) {
    g_zVideo_PixelPack_RMaskShifted = 0xf8;
    g_zVideo_PixelPack_GMaskShifted = 0xfc;
    g_zVideo_PixelPack_RShift = 8;
    g_zVideo_PixelPack_GShift = 3;
    g_zVideo_PixelPack_BShiftTo8 = 3;

    HudSensorMapNode node;
    std::memset(&node, 0x7f, sizeof(node));
    if (node.Init() != &node || node.next != nullptr ||
        static_cast<std::uint8_t>(node.colorRgb[0]) != 0xff ||
        static_cast<std::uint8_t>(node.colorRgb[1]) != 0xff ||
        static_cast<std::uint8_t>(node.colorRgb[2]) != 0xff || node.pointCount != 0 ||
        node.points != nullptr || node.objectiveIndex != -1 || node.selectedPointIndex != -1 ||
        node.isEnabled != 0 || node.blinkTimerSec != 0.0f || node.packedColor565Pair != -1) {
        return 1;
    }

    HudSensorMapPoint points[2] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
    };
    node.points = points;
    node.pointCount = 2;
    if (node.SelectPoint(1) != &points[1] || node.selectedPointIndex != 1 ||
        node.SelectPoint(2) != nullptr || node.selectedPointIndex != -1) {
        return 2;
    }

    const std::uint8_t rgb[3] = {0x20, 0x40, 0x60};
    const std::uint16_t fullColor = static_cast<std::uint16_t>(zVid_PackColorRGB(0x20, 0x60, 0x40));
    const std::uint16_t halfColor = static_cast<std::uint16_t>(zVid_PackColorRGB(0x10, 0x30, 0x20));
    if (node.SetColorRgb(rgb) != 1 ||
        node.packedColor565Pair != ((static_cast<std::int32_t>(halfColor) << 16) | fullColor)) {
        return 3;
    }

    if (node.SetEnabled(1) != 1 || node.isEnabled != 1 || node.selectedPointIndex != -1 ||
        node.SetEnabled(1) != 0) {
        return 4;
    }

    node.cachedBounds.maxY = 99.0f;
    if (node.UpdateCachedBounds(nullptr) != 1 || node.cachedBounds.minX != 1.0f ||
        node.cachedBounds.maxX != 4.0f || node.cachedBounds.minY != 0.0f ||
        node.cachedBounds.minZ != 3.0f || node.cachedBounds.maxY != 99.0f ||
        node.cachedBounds.maxZ != 6.0f) {
        return 5;
    }

    HudSensorMapBounds copiedBounds = {};
    node.cachedBounds.minX = -7.0f;
    if (node.UpdateCachedBounds(&copiedBounds) != 1 || copiedBounds.minX != -7.0f ||
        copiedBounds.maxZ != 6.0f) {
        return 6;
    }

    if (node.LoadFromStream(nullptr) != 0) {
        return 7;
    }

    std::FILE *stream = std::tmpfile();
    if (stream == nullptr) {
        return 8;
    }

    const std::uint8_t streamRgb[3] = {0x20, 0x40, 0x60};
    const std::int32_t streamPointCount = 2;
    const HudSensorMapPoint streamPoints[2] = {
        {-2.0f, 7.0f, 10.0f},
        {9.0f, 8.0f, -5.0f},
    };
    const std::int32_t streamObjectiveIndex = 13;

    const bool writeOk = std::fwrite(streamRgb, 3, 1, stream) == 1 &&
                         std::fwrite(&streamPointCount, 4, 1, stream) == 1 &&
                         std::fwrite(streamPoints, sizeof(HudSensorMapPoint), streamPointCount,
                                     stream) == static_cast<std::size_t>(streamPointCount) &&
                         std::fwrite(&streamObjectiveIndex, 4, 1, stream) == 1;
    std::rewind(stream);

    HudSensorMapNode loadedNode = {};
    loadedNode.cachedBounds.maxY = 44.0f;
    const std::int32_t loadResult = writeOk ? loadedNode.LoadFromStream(stream) : 0;
    std::fclose(stream);

    const std::uint16_t streamFullColor =
        static_cast<std::uint16_t>(zVid_PackColorRGB(0x20, 0x60, 0x40));
    const std::uint16_t streamHalfColor =
        static_cast<std::uint16_t>(zVid_PackColorRGB(0x10, 0x30, 0x20));
    const bool loadOk =
        writeOk && loadResult == 1 && loadedNode.pointCount == streamPointCount &&
        loadedNode.objectiveIndex == streamObjectiveIndex && loadedNode.points != nullptr &&
        loadedNode.points[1].z == -5.0f && loadedNode.cachedBounds.minX == -2.0f &&
        loadedNode.cachedBounds.maxX == 9.0f && loadedNode.cachedBounds.minY == 0.0f &&
        loadedNode.cachedBounds.minZ == -5.0f && loadedNode.cachedBounds.maxY == 44.0f &&
        loadedNode.cachedBounds.maxZ == 10.0f &&
        loadedNode.packedColor565Pair ==
            ((static_cast<std::int32_t>(streamHalfColor) << 16) | streamFullColor);
    loadedNode.FreePointArray();
    if (!loadOk) {
        return 9;
    }

    return 0;
}

extern "C" int hud_sensor_objective_marker_enable_color_smoke(void) {
    g_zVideo_PixelPack_RMaskShifted = 0xf8;
    g_zVideo_PixelPack_GMaskShifted = 0xfc;
    g_zVideo_PixelPack_RShift = 8;
    g_zVideo_PixelPack_GShift = 3;
    g_zVideo_PixelPack_BShiftTo8 = 3;

    HudSensorTracker tracker = {};
    HudSensorMapPoint matchedPoints[1] = {{1.0f, 2.0f, 3.0f}};
    HudSensorMapNode first = {};
    first.Init();
    first.objectiveIndex = 2;
    first.points = matchedPoints;
    first.pointCount = 1;
    first.selectedPointIndex = 0;

    HudSensorMapNode second = {};
    second.Init();
    second.objectiveIndex = 5;
    second.isEnabled = 1;

    HudSensorMapNode third = {};
    third.Init();
    third.objectiveIndex = 2;
    third.selectedPointIndex = 7;

    first.next = &second;
    second.next = &third;
    tracker.mapNodeListHead = &first;

    const std::uint8_t rgb[3] = {0x10, 0x30, 0x50};
    const std::uint16_t fullColor = static_cast<std::uint16_t>(zVid_PackColorRGB(0x10, 0x50, 0x30));

    const int result = tracker.SetObjectiveMarkerEnabledAndColor(2, 1, rgb);
    const bool firstOk = first.isEnabled == 1 && first.selectedPointIndex == -1 &&
                         static_cast<std::uint8_t>(first.colorRgb[0]) == 0x10 &&
                         static_cast<std::uint8_t>(first.colorRgb[1]) == 0x30 &&
                         static_cast<std::uint8_t>(first.colorRgb[2]) == 0x50 &&
                         (first.packedColor565Pair & 0xffff) == fullColor;
    const bool secondOk = second.isEnabled == 1 &&
                          static_cast<std::uint8_t>(second.colorRgb[0]) == 0xff &&
                          second.selectedPointIndex == -1;
    const bool thirdOk = third.isEnabled == 1 && third.selectedPointIndex == -1 &&
                         static_cast<std::uint8_t>(third.colorRgb[0]) == 0x10;

    const std::int32_t secondPackedColor = second.packedColor565Pair;
    const std::uint8_t blinkRgb[3] = {0x20, 0x40, 0x60};
    const std::uint16_t blinkFullColor =
        static_cast<std::uint16_t>(zVid_PackColorRGB(0x20, 0x60, 0x40));
    const std::uint16_t blinkHalfColor =
        static_cast<std::uint16_t>(zVid_PackColorRGB(0x10, 0x30, 0x20));
    const std::uint32_t expectedBlinkPair =
        (static_cast<std::uint32_t>(blinkFullColor) << 16) | blinkHalfColor;

    const int blinkResult = tracker.SetObjectiveMarkerColorBlink(2, blinkRgb);
    const bool firstBlinkOk =
        static_cast<std::uint8_t>(first.colorRgb[0]) == 0x20 &&
        static_cast<std::uint8_t>(first.colorRgb[1]) == 0x40 &&
        static_cast<std::uint8_t>(first.colorRgb[2]) == 0x60 &&
        static_cast<std::uint32_t>(first.packedColor565Pair) == expectedBlinkPair;
    const bool secondBlinkOk = second.packedColor565Pair == secondPackedColor &&
                               static_cast<std::uint8_t>(second.colorRgb[0]) == 0xff;
    const bool thirdBlinkOk =
        static_cast<std::uint8_t>(third.colorRgb[0]) == 0x20 &&
        static_cast<std::uint32_t>(third.packedColor565Pair) == expectedBlinkPair;

    return result == 1 && firstOk && secondOk && thirdOk && blinkResult == 1 && firstBlinkOk &&
                   secondBlinkOk && thirdBlinkOk
               ? 0
               : 1;
}

extern "C" int hud_sensor_find_first_incomplete_objective_smoke(void) {
    g_zVideo_PixelPack_RMaskShifted = 0xf8;
    g_zVideo_PixelPack_GMaskShifted = 0xfc;
    g_zVideo_PixelPack_RShift = 8;
    g_zVideo_PixelPack_GShift = 3;
    g_zVideo_PixelPack_BShiftTo8 = 3;

    HudSensorTracker tracker = {};
    tracker.objectiveCount = 3;
    tracker.objectiveSlots[0].completedFlag = 1;
    tracker.objectiveSlots[1].completedFlag = 0;
    tracker.objectiveSlots[2].completedFlag = 0;

    HudSensorMapNode first = {};
    first.Init();
    first.objectiveIndex = 1;
    HudSensorMapNode second = {};
    second.Init();
    second.objectiveIndex = 2;
    first.next = &second;
    tracker.mapNodeListHead = &first;

    const int firstIncomplete = tracker.FindAndHighlightFirstIncompleteObjective();
    const bool highlightOk =
        firstIncomplete == 1 && first.isEnabled == 1 &&
        static_cast<std::uint8_t>(first.colorRgb[0]) == 0x00 &&
        static_cast<std::uint8_t>(first.colorRgb[1]) == 0x00 &&
        static_cast<std::uint8_t>(first.colorRgb[2]) == 0xff && second.isEnabled == 0;

    tracker.objectiveSlots[1].completedFlag = 1;
    tracker.objectiveSlots[2].completedFlag = 1;
    first.isEnabled = 0;
    const int allComplete = tracker.FindAndHighlightFirstIncompleteObjective();

    return highlightOk && allComplete == 3 && first.isEnabled == 0 ? 0 : 1;
}

extern "C" int hud_sensor_map_bounds_and_save_state_smoke(void) {
    HudSensorTracker tracker = {};
    const HudUiRect outer{10, 20, 30, 50};
    const HudUiRect inner{12, 23, 28, 45};

    tracker.SetBounds(&outer, &inner);
    if (tracker.outerRect.left != 10 || tracker.innerRectExpanded.left != 11 ||
        tracker.innerRectExpanded.top != 22 || tracker.innerRectExpanded.right != 29 ||
        tracker.innerRectExpanded.bottom != 46 || tracker.mapOverlayCenterX != 20 ||
        tracker.mapOverlayCenterY != 35) {
        return 1;
    }

    zUtil_PlayerStateStorage playerState = {};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    if (tracker.SetTrackedSaveState(&saveState) != 1 ||
        tracker.trackedSaveStateSelection != &saveState ||
        tracker.trackedWorldOriginPtr != reinterpret_cast<zVec3 *>(playerState.bytes + 0x3ec) ||
        tracker.trackedForwardVecPtr != reinterpret_cast<zVec3 *>(playerState.bytes + 0x580) ||
        tracker.mapZoom != 1.0f) {
        return 2;
    }

    if (tracker.SetTrackedSaveState(nullptr) != 1 || tracker.trackedSaveStateSelection != nullptr ||
        tracker.trackedWorldOriginPtr != nullptr || tracker.trackedForwardVecPtr != nullptr) {
        return 3;
    }

    tracker.SetSaveStateMarkerMaxDistance(12.0f);
    tracker.Init(&outer);
    return tracker.mapFileVersion == 5 && tracker.mapNodeListHead == nullptr &&
                   tracker.loadedMapPath == nullptr && tracker.mapScaleCurrent.x == 0.0f &&
                   tracker.mapScaleCurrent.z == 0.0f && tracker.mapZoom == 1.0f &&
                   tracker.saveStateMarkerMaxDistSq == 202500.0f
               ? 0
               : 4;
}

extern "C" int hud_sensor_map_remove_and_shutdown_smoke(void) {
    HudSensorTracker tracker = {};
    HudSensorMapNode first = {};
    HudSensorMapNode second = {};
    HudSensorMapNode third = {};
    first.next = &second;
    second.next = &third;
    tracker.mapNodeListHead = &first;

    if (tracker.MapRemoveNode(&second) != 1 || first.next != &third ||
        tracker.MapRemoveNode(&second) != 0) {
        return 1;
    }

    auto *nodeA = static_cast<HudSensorMapNode *>(::operator new(sizeof(HudSensorMapNode)));
    auto *nodeB = static_cast<HudSensorMapNode *>(::operator new(sizeof(HudSensorMapNode)));
    std::memset(nodeA, 0, sizeof(*nodeA));
    std::memset(nodeB, 0, sizeof(*nodeB));
    nodeA->next = nodeB;
    nodeA->points = static_cast<HudSensorMapPoint *>(std::malloc(sizeof(HudSensorMapPoint)));
    nodeB->points = static_cast<HudSensorMapPoint *>(std::malloc(sizeof(HudSensorMapPoint)));

    tracker.mapNodeListHead = nodeA;
    tracker.loadedMapPath = static_cast<char *>(std::malloc(8));
    std::strcpy(tracker.loadedMapPath, "m01.map");
    tracker.mapScaleLerpActive = 1;
    tracker.mapScaleCurrent = {2.0f, 3.0f, 4.0f};
    tracker.mapLoadedFlag = 0;

    return tracker.MapShutdownAndReset() == 1 && tracker.mapNodeListHead == nullptr &&
                   tracker.loadedMapPath == nullptr && tracker.mapFileVersion == 5 &&
                   tracker.mapScaleLerpActive == 0 && tracker.mapScaleStart.x == 2.0f &&
                   tracker.mapScaleStart.y == 3.0f && tracker.mapScaleStart.z == 4.0f
               ? 0
               : 2;
}

extern "C" int hud_sensor_map_overlay_toggle_smoke(void) {
    const int oldRefCount = g_Hud_MapOverlayRefCount;

    HudSensorTracker tracker = {};
    tracker.outerRect.left = 0;
    tracker.outerRect.top = 0;
    tracker.outerRect.right = 100;
    tracker.outerRect.bottom = 80;
    tracker.mapBoundsMinX = -10.0f;
    tracker.mapBoundsMaxX = 10.0f;
    tracker.mapBoundsMinZ = -20.0f;
    tracker.mapBoundsMaxZ = 20.0f;
    tracker.mapScaleCurrent = {1.0f, 2.0f, 3.0f};
    tracker.mapLoadedFlag = 0;
    g_Hud_MapOverlayRefCount = 0;

    const int beginResult = tracker.MapOverlayBeginShow();
    const bool beginOk =
        beginResult == 1 && tracker.mapScaleLerpActive == 1 &&
        tracker.mapScaleLerpT == 0.0f && tracker.mapScaleStart.x == 1.0f &&
        tracker.mapScaleStart.y == 2.0f && tracker.mapScaleStart.z == 3.0f &&
        tracker.mapScaleGoal.x == 4.0f && tracker.mapScaleGoal.z == 2.0f &&
        tracker.mapScaleLerpRunning == 0 && tracker.MapOverlayBeginShow() == 0;

    tracker.MapOverlayEndShow();
    const bool endOk = tracker.mapScaleLerpActive == 0 && tracker.mapScaleLerpT == 0.0f &&
                       tracker.mapScaleStart.x == 1.0f &&
                       tracker.mapScaleStart.y == 2.0f &&
                       tracker.mapScaleStart.z == 3.0f &&
                       tracker.mapScaleGoal.x == 0.0f &&
                       tracker.mapScaleGoal.z == 0.0f &&
                       tracker.mapScaleLerpRunning == 0;

    tracker.mapScaleCurrent = {5.0f, 6.0f, 7.0f};
    const int toggleOnA = tracker.MapOverlayRefToggle(1);
    const int toggleOnB = tracker.MapOverlayRefToggle(1);
    const bool toggleOnOk = toggleOnA == 1 && toggleOnB == 1 &&
                            g_Hud_MapOverlayRefCount == 2 &&
                            tracker.mapScaleLerpActive == 1 &&
                            tracker.mapScaleStart.x == 5.0f &&
                            tracker.mapScaleStart.y == 6.0f &&
                            tracker.mapScaleStart.z == 7.0f;

    const int toggleOffA = tracker.MapOverlayRefToggle(0);
    const bool stillShown = toggleOffA == 1 && g_Hud_MapOverlayRefCount == 1 &&
                            tracker.mapScaleLerpActive == 1;
    const int toggleOffB = tracker.MapOverlayRefToggle(0);
    const bool toggleOffOk = toggleOffB == 1 && g_Hud_MapOverlayRefCount == 0 &&
                             tracker.mapScaleLerpActive == 0 &&
                             tracker.mapScaleGoal.x == 0.0f &&
                             tracker.mapScaleGoal.z == 0.0f;

    tracker.mapScaleLerpActive = 0;
    tracker.mapZoom = 2.0f;
    tracker.MapZoomIn();
    tracker.MapZoomOut();
    const bool zoomInactiveOk = tracker.mapZoom == 2.0f;

    tracker.mapScaleLerpActive = 1;
    tracker.mapSndClick = nullptr;
    tracker.MapZoomIn();
    const bool zoomInOk = tracker.mapZoom > 2.199f && tracker.mapZoom < 2.201f;
    tracker.MapZoomOut();
    const bool zoomOutOk = tracker.mapZoom > 1.979f && tracker.mapZoom < 1.981f;

    g_Hud_MapOverlayRefCount = oldRefCount;
    return beginOk && endOk && toggleOnOk && stillShown && toggleOffOk && zoomInactiveOk &&
                   zoomInOk && zoomOutOk
               ? 0
               : 1;
}

extern "C" int hud_sensor_tracker_load_map_paths_smoke(void) {
    g_zVideo_PixelPack_RMaskShifted = 0xf8;
    g_zVideo_PixelPack_GMaskShifted = 0xfc;
    g_zVideo_PixelPack_RShift = 8;
    g_zVideo_PixelPack_GShift = 3;
    g_zVideo_PixelPack_BShiftTo8 = 3;

    const HudSensorMapBounds fileBounds = {-1.0f, 0.0f, -2.0f, 1.0f, 0.0f, 2.0f};
    const unsigned char rgb[3] = {0x30, 0x50, 0x70};
    const HudSensorMapPoint points[2] = {
        {-3.0f, 4.0f, 8.0f},
        {5.0f, 6.0f, -7.0f},
    };

    const char *path = "hud_sensor_tracker_load_map_paths.zmap";
    if (!WriteHudSensorTestMap(path, fileBounds, rgb, points, 2, 21)) {
        return 1;
    }

    HudSensorTracker tracker = {};
    const int loadResult = tracker.LoadMapFromPath(path);
    std::remove(path);

    HudSensorMapNode *const loadedNode = tracker.mapNodeListHead;
    const bool pathOk = loadResult == 1 && tracker.mapLoadedFlag == 1 &&
                        tracker.loadedMapPath != nullptr &&
                        std::strcmp(tracker.loadedMapPath, path) == 0 &&
                        tracker.mapFileVersion == 5 && tracker.mapHeaderDword == 0x13572468 &&
                        tracker.mapBoundsMinX == -3.0f && tracker.mapBoundsMinZ == -7.0f &&
                        tracker.mapBoundsMaxX == 5.0f && tracker.mapBoundsMaxZ == 8.0f &&
                        loadedNode != nullptr && loadedNode->next == nullptr &&
                        loadedNode->pointCount == 2 && loadedNode->objectiveIndex == 21 &&
                        loadedNode->points != nullptr && loadedNode->points[1].x == 5.0f &&
                        loadedNode->points[1].z == -7.0f;
    tracker.MapShutdownAndReset();
    if (!pathOk) {
        return 2;
    }

    CreateDirectoryA("maps", nullptr);
    const char *missionPath = ".\\maps\\m9.zmap";
    if (!WriteHudSensorTestMap(missionPath, fileBounds, rgb, points, 2, 22)) {
        return 3;
    }

    const int oldInitialized = g_zSnd_IsInitialized;
    const int oldActiveBackend = g_zSnd_ActiveBackend;
    const zSndSampleSetRegistry oldRegistry = g_zSnd_SampleSetRegistry;

    zSndSample samples[3] = {};
    samples[0].replayFields.sampleId = "snd_mapOn";
    samples[1].replayFields.sampleId = "snd_mapOff";
    samples[2].replayFields.sampleId = "snd_mapClick";
    samples[0].primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&samples[0]);
    samples[1].primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&samples[1]);
    samples[2].primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&samples[2]);

    zSndSampleSet sampleSet = {};
    sampleSet.sampleCount = 3;
    sampleSet.samples = samples;
    zSndSampleSet *sampleSets[1] = {&sampleSet};

    g_zSnd_IsInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_SampleSetRegistry.begin = sampleSets;
    g_zSnd_SampleSetRegistry.end = sampleSets + 1;
    g_zSnd_SampleSetRegistry.capacityEnd = sampleSets + 1;

    HudSensorTracker missionTracker = {};
    const int missionResult = missionTracker.LoadMissionMapAndSfx(9);
    DeleteFileA(missionPath);

    const bool missionOk = missionResult == 1 &&
                           missionTracker.loadedMapPath != nullptr &&
                           std::strcmp(missionTracker.loadedMapPath, missionPath) == 0 &&
                           missionTracker.mapSndOn == &samples[0] &&
                           missionTracker.mapSndOff == &samples[1] &&
                           missionTracker.mapSndClick == &samples[2] &&
                           missionTracker.mapNodeListHead != nullptr &&
                           missionTracker.mapNodeListHead->objectiveIndex == 22;
    missionTracker.MapShutdownAndReset();

    g_zSnd_SampleSetRegistry = oldRegistry;
    g_zSnd_ActiveBackend = oldActiveBackend;
    g_zSnd_IsInitialized = oldInitialized;

    return missionOk ? 0 : 4;
}

extern "C" int hud_sensor_reset_mission_state_smoke(void) {
    static const HudUiCommon_FTable ftable = MakeHudSensorTestFTable();

    g_hudSensorVisibleCallCount = 0;
    g_hudSensorLastVisible = -1;
    g_hudSensorDeleteCallCount = 0;
    g_hudSensorDeleteFlags = 0;

    auto *fxElement =
        static_cast<HudSensorTestElement *>(::operator new(sizeof(HudSensorTestElement)));
    std::memset(fxElement, 0, sizeof(*fxElement));
    fxElement->ftable = &ftable;

    auto *const fxContainer = reinterpret_cast<HudUiContainer *>(&g_zVideo_FxPass3ConfigLocal);
    fxContainer->childHead = fxElement;
    fxContainer->childTail = fxElement;
    fxElement->parent = fxContainer;

    HudSensorTracker tracker = {};
    zClass_NodePartial worldNode = {};
    tracker.missionLoaded = 1;
    tracker.missionId = 9;
    tracker.missionFlags = 7;
    tracker.objectiveCount = 3;
    tracker.worldNode = &worldNode;
    tracker.missionDataPath = "mission.dat";
    tracker.zbdPath = "mission.zbd";
    tracker.fxPass3Obj = fxElement;

    const bool ok = tracker.ResetMissionState() == 1 && tracker.missionLoaded == 0 &&
                    tracker.missionId == 0 && tracker.missionFlags == 1 &&
                    tracker.objectiveCount == 0 && tracker.worldNode == nullptr &&
                    CStringIsEmpty(tracker.missionDataPath) && CStringIsEmpty(tracker.zbdPath) &&
                    tracker.fxPass3Obj == nullptr && fxContainer->childHead == nullptr &&
                    fxContainer->childTail == nullptr && g_hudSensorVisibleCallCount == 1 &&
                    g_hudSensorLastVisible == 0 && g_hudSensorDeleteCallCount == 1 &&
                    g_hudSensorDeleteFlags == 1;

    fxContainer->childHead = nullptr;
    fxContainer->childTail = nullptr;
    return ok ? 0 : 1;
}

extern "C" int hud_sensor_shutdown_mission_gameplay_systems_early_smoke(void) {
    HudSensorTracker tracker = {};
    zClass_NodePartial worldNode = {};
    tracker.missionLoaded = 0;
    tracker.worldNode = &worldNode;
    tracker.missionId = 17;

    const bool ok = tracker.ShutdownMissionGameplaySystems() == 1 &&
                    tracker.missionLoaded == 0 && tracker.worldNode == &worldNode &&
                    tracker.missionId == 17;

    return ok ? 0 : 1;
}

extern "C" int zutil_zar_write_section_blob_smoke(void) {
    char tempPath[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    GetTempPathA(sizeof(tempPath), tempPath);
    GetTempFileNameA(tempPath, "zar", 0, tempFile);

    HANDLE const file =
        CreateFileA(tempFile, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return 1;
    }

    zZbdManager manager = {};
    manager.indexArchive.hFile = file;

    zZbdSectionHandler handler = {};
    handler.sectionName = "HUD";
    zZbdSectionCallbackCtx callbackCtx = {};
    callbackCtx.manager = &manager;
    callbackCtx.sectionHandler = &handler;

    const std::uint32_t payload = 0x44332211;
    const std::int32_t result =
        zUtil_ZAR::WriteSectionBlob(&callbackCtx, "TimerData", &payload, sizeof(payload));

    SetFilePointer(file, 0, nullptr, FILE_BEGIN);
    std::uint32_t readBack = 0;
    DWORD read = 0;
    ReadFile(file, &readBack, sizeof(readBack), &read, nullptr);

    const bool ok = result == 1 && manager.indexArchive.recordCount == 1 &&
                    manager.indexArchive.recordCapacity == 2 && manager.indexArchive.dirty == 1 &&
                    manager.indexArchive.records != nullptr &&
                    manager.indexArchive.records[0].fileOffset == 0 &&
                    manager.indexArchive.records[0].fileSize == sizeof(payload) &&
                    std::strcmp(manager.indexArchive.records[0].name, "HUD/TimerData") == 0 &&
                    read == sizeof(readBack) && readBack == payload;

    std::free(manager.indexArchive.records);
    manager.indexArchive.records = nullptr;
    CloseHandle(file);
    return ok ? 0 : 1;
}

extern "C" int zutil_zbd_section_handler_invoke_data_ready_smoke(void) {
    zZbdSectionHandler handler = {};
    zZbdSectionCallbackCtx callbackCtx = {};
    unsigned char payload[4] = {1, 2, 3, 4};
    int userData = 7;

    g_zbdDataReadyCallCount = 0;
    handler.InvokeDataReady(&callbackCtx, "Token", payload, sizeof(payload));
    const bool nullOk = g_zbdDataReadyCallCount == 0;

    handler.onDataReady = TestZbdCallbackPtr(&TestZbdDataReadyCallback);
    handler.userData = &userData;
    handler.InvokeDataReady(&callbackCtx, "Token", payload, sizeof(payload));

    const bool callbackOk =
        g_zbdDataReadyCallCount == 1 && g_zbdDataReadyCtx == &callbackCtx &&
        g_zbdDataReadyToken != nullptr && std::strcmp(g_zbdDataReadyToken, "Token") == 0 &&
        g_zbdDataReadyBuffer == payload && g_zbdDataReadySize == sizeof(payload) &&
        g_zbdDataReadyUserData == &userData;

    return nullOk && callbackOk ? 0 : 1;
}

extern "C" int zutil_zbd_section_handler_compare_sort_order_smoke(void) {
    zZbdSectionHandler low = {};
    zZbdSectionHandler high = {};
    zZbdSectionHandler equal = {};
    low.sortOrder = 10;
    high.sortOrder = 20;
    equal.sortOrder = 10;

    const bool ok = zZbdSectionHandler::CompareSortOrderLessThan(&low, &high) &&
                    !zZbdSectionHandler::CompareSortOrderLessThan(&high, &low) &&
                    !zZbdSectionHandler::CompareSortOrderLessThan(&low, &equal);

    return ok ? 0 : 1;
}

extern "C" int zutil_zbd_section_handler_invoke_pre_load_smoke(void) {
    zZbdSectionHandler handler = {};
    zZbdSectionCallbackCtx callbackCtx = {};
    ZbdPreLoadControl control = {};
    control.result = 77;
    control.failAtCall = -1;

    ResetZbdPreLoadCalls();
    const bool nullOk = handler.InvokePreLoad(&callbackCtx) == 1 &&
                        g_zbdPreLoadCallCount == 0;

    handler.onPreLoad = TestZbdCallbackPtr(&TestZbdPreLoadCallback);
    handler.userData = &control;
    const int callbackResult = handler.InvokePreLoad(&callbackCtx);

    const bool callbackOk = callbackResult == control.result &&
                            g_zbdPreLoadCallCount == 1 &&
                            g_zbdPreLoadCtx == &callbackCtx &&
                            g_zbdPreLoadUserData == &control;

    return nullOk && callbackOk ? 0 : 1;
}

extern "C" int zutil_zbd_manager_sort_section_handlers_smoke(void) {
    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager = MakeManager(sentinel);
    manager.RegisterSectionHandler("Late", nullptr, nullptr, 30, nullptr);
    manager.RegisterSectionHandler("Early", nullptr, nullptr, 10, nullptr);
    manager.RegisterSectionHandler("Middle", nullptr, nullptr, 20, nullptr);
    manager.RegisterSectionHandler("Tie", nullptr, nullptr, 20, nullptr);

    manager.SortSectionHandlers();

    zZbdSectionHandlerNode *first = sentinel.next;
    zZbdSectionHandlerNode *second = first != &sentinel ? first->next : &sentinel;
    zZbdSectionHandlerNode *third = second != &sentinel ? second->next : &sentinel;
    zZbdSectionHandlerNode *fourth = third != &sentinel ? third->next : &sentinel;
    const bool ok = manager.sectionHandlerCount == 4 && first != &sentinel &&
                    second != &sentinel && third != &sentinel && fourth != &sentinel &&
                    fourth->next == &sentinel &&
                    std::strcmp(first->sectionHandler.sectionName, "Early") == 0 &&
                    std::strcmp(second->sectionHandler.sectionName, "Middle") == 0 &&
                    std::strcmp(third->sectionHandler.sectionName, "Tie") == 0 &&
                    std::strcmp(fourth->sectionHandler.sectionName, "Late") == 0 &&
                    sentinel.prev == fourth;

    ClearRegisteredHandlers(sentinel);
    return ok ? 0 : 1;
}

extern "C" int zutil_zbd_manager_load_entries_smoke(void) {
    char tempDir[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zle", 0, tempFile) == 0) {
        return 1;
    }

    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager = MakeManager(sentinel);
    ZbdPreLoadControl control = {};
    control.result = 1;
    control.failAtCall = -1;
    control.writeRecord = true;
    manager.RegisterSectionHandler("Late", TestZbdCallbackPtr(&TestZbdPreLoadCallback),
                                   nullptr, 30, &control);
    manager.RegisterSectionHandler("Early", TestZbdCallbackPtr(&TestZbdPreLoadCallback),
                                   nullptr, 10, &control);
    manager.RegisterSectionHandler("Middle", TestZbdCallbackPtr(&TestZbdPreLoadCallback),
                                   nullptr, 20, &control);

    ResetZbdPreLoadCalls();
    const int loadResult = manager.LoadEntries(tempFile);
    const bool loadOk = loadResult == 1 && g_zbdPreLoadCallCount == 3 &&
                        std::strcmp(g_zbdPreLoadCalls[0].sectionName, "Early") == 0 &&
                        std::strcmp(g_zbdPreLoadCalls[1].sectionName, "Middle") == 0 &&
                        std::strcmp(g_zbdPreLoadCalls[2].sectionName, "Late") == 0 &&
                        manager.indexArchive.hFile == INVALID_HANDLE_VALUE &&
                        manager.indexArchive.records == nullptr &&
                        manager.indexArchive.recordCount == 0;

    control.writeRecord = false;
    control.failAtCall = 1;
    ResetZbdPreLoadCalls();
    const int failedResult = manager.LoadEntries(tempFile);
    const bool failOk = failedResult == 0 && g_zbdPreLoadCallCount == 2 &&
                        std::strcmp(g_zbdPreLoadCalls[0].sectionName, "Early") == 0 &&
                        std::strcmp(g_zbdPreLoadCalls[1].sectionName, "Middle") == 0 &&
                        manager.indexArchive.hFile == INVALID_HANDLE_VALUE &&
                        manager.indexArchive.records == nullptr;

    ClearRegisteredHandlers(sentinel);
    DeleteFileA(tempFile);
    return loadOk && failOk ? 0 : 2;
}

extern "C" int zutil_zbd_load_entries_global_smoke(void) {
    g_zUtil_ZbdManager = nullptr;
    const bool nullManagerOk = zUtil::ZBD_LoadEntriesGlobal("missing.zbd") == 0;

    char tempDir[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zlg", 0, tempFile) == 0) {
        return 1;
    }

    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager = MakeManager(sentinel);
    ZbdPreLoadControl control = {};
    control.result = 1;
    control.failAtCall = -1;
    manager.RegisterSectionHandler("Global", TestZbdCallbackPtr(&TestZbdPreLoadCallback),
                                   nullptr, 5, &control);

    g_zUtil_ZbdManager = &manager;
    ResetZbdPreLoadCalls();
    const int loadResult = zUtil::ZBD_LoadEntriesGlobal(tempFile);
    const bool delegateOk = loadResult == 1 && g_zbdPreLoadCallCount == 1 &&
                            g_zbdPreLoadCalls[0].manager == &manager &&
                            std::strcmp(g_zbdPreLoadCalls[0].sectionName, "Global") == 0 &&
                            manager.indexArchive.hFile == INVALID_HANDLE_VALUE &&
                            manager.indexArchive.records == nullptr;

    g_zUtil_ZbdManager = nullptr;
    ClearRegisteredHandlers(sentinel);
    DeleteFileA(tempFile);
    return nullManagerOk && delegateOk ? 0 : 2;
}

extern "C" int zutil_zbd_manager_load_zar_file_smoke(void) {
    char tempDir[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zzb", 0, tempFile) == 0) {
        return 1;
    }

    const unsigned char firstPayload[3] = {17, 34, 51};
    const unsigned char ignoredPayload[2] = {68, 85};
    const unsigned char secondPayload[5] = {102, 119, 136, 153, 170};

    zIndexArchive writer = {};
    writer.Reset();
    if (writer.OpenCreateWrite(tempFile) == 0 ||
        writer.AddFileRecord("Demo/TokenOne", firstPayload, sizeof(firstPayload), nullptr,
                             nullptr) == 0 ||
        writer.AddFileRecord("Other/Skip", ignoredPayload, sizeof(ignoredPayload), nullptr,
                             nullptr) == 0 ||
        writer.AddFileRecord("Demo/TokenTwo", secondPayload, sizeof(secondPayload), nullptr,
                             nullptr) == 0 ||
        writer.CloseAndFreeRecords() == 0) {
        writer.CloseAndFreeRecords();
        DeleteFileA(tempFile);
        return 2;
    }

    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager = MakeManager(sentinel);
    ZbdLoadDataReadyControl control = {};
    manager.RegisterSectionHandler("Demo", nullptr,
                                   TestZbdCallbackPtr(&TestZbdLoadDataReadyCallback), 0,
                                   &control);

    ResetZbdLoadCalls();
    const int loadResult = manager.LoadZarFile(tempFile);
    zZbdSectionHandler *const handler = sentinel.next != &sentinel
                                            ? &sentinel.next->sectionHandler
                                            : nullptr;
    const bool fullLoadOk =
        loadResult == 1 && g_zbdLoadCallCount == 2 && g_zbdLoadCalls[0].manager == &manager &&
        g_zbdLoadCalls[0].sectionHandler == handler && g_zbdLoadCalls[0].userData == &control &&
        std::strcmp(g_zbdLoadCalls[0].sectionToken, "TokenOne") == 0 &&
        g_zbdLoadCalls[0].size == sizeof(firstPayload) &&
        std::memcmp(g_zbdLoadCalls[0].bytes, firstPayload, sizeof(firstPayload)) == 0 &&
        g_zbdLoadCalls[1].manager == &manager && g_zbdLoadCalls[1].sectionHandler == handler &&
        g_zbdLoadCalls[1].userData == &control &&
        std::strcmp(g_zbdLoadCalls[1].sectionToken, "TokenTwo") == 0 &&
        g_zbdLoadCalls[1].size == sizeof(secondPayload) &&
        std::memcmp(g_zbdLoadCalls[1].bytes, secondPayload, sizeof(secondPayload)) == 0 &&
        manager.indexArchive.hFile == INVALID_HANDLE_VALUE &&
        manager.indexArchive.records == nullptr && manager.indexArchive.recordCount == 0 &&
        manager.tempBuffer != nullptr && manager.tempBufferSize >= sizeof(secondPayload) &&
        manager.stopRequested == 0;

    control.stopOnFirstCall = true;
    ResetZbdLoadCalls();
    const int stoppedLoadResult = manager.LoadZarFile(tempFile);
    const bool stopOk = stoppedLoadResult == 1 && g_zbdLoadCallCount == 1 &&
                        std::strcmp(g_zbdLoadCalls[0].sectionToken, "TokenOne") == 0 &&
                        manager.stopRequested == 1 &&
                        manager.indexArchive.hFile == INVALID_HANDLE_VALUE &&
                        manager.indexArchive.records == nullptr;

    ClearRegisteredHandlers(sentinel);
    if (manager.tempBuffer != nullptr) {
        ::operator delete(manager.tempBuffer);
        manager.tempBuffer = nullptr;
    }
    DeleteFileA(tempFile);
    return fullLoadOk && stopOk ? 0 : 3;
}

extern "C" int zutil_zar_load_file_global_smoke(void) {
    g_zUtil_ZbdManager = nullptr;
    const bool nullManagerOk = zUtil::ZAR_LoadFileGlobal("missing.zar") == 0;

    char tempDir[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zlg", 0, tempFile) == 0) {
        return 1;
    }

    const unsigned char payload[4] = {3, 1, 4, 1};
    zIndexArchive writer = {};
    writer.Reset();
    if (writer.OpenCreateWrite(tempFile) == 0 ||
        writer.AddFileRecord("Demo/Global", payload, sizeof(payload), nullptr, nullptr) == 0 ||
        writer.CloseAndFreeRecords() == 0) {
        writer.CloseAndFreeRecords();
        DeleteFileA(tempFile);
        return 2;
    }

    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager = MakeManager(sentinel);
    ZbdLoadDataReadyControl control = {};
    manager.RegisterSectionHandler("Demo", nullptr,
                                   TestZbdCallbackPtr(&TestZbdLoadDataReadyCallback), 0,
                                   &control);

    g_zUtil_ZbdManager = &manager;
    ResetZbdLoadCalls();
    const int loadResult = zUtil::ZAR_LoadFileGlobal(tempFile);
    const bool delegateOk =
        loadResult == 1 && g_zbdLoadCallCount == 1 &&
        g_zbdLoadCalls[0].manager == &manager &&
        std::strcmp(g_zbdLoadCalls[0].sectionToken, "Global") == 0 &&
        g_zbdLoadCalls[0].size == sizeof(payload) &&
        std::memcmp(g_zbdLoadCalls[0].bytes, payload, sizeof(payload)) == 0;

    g_zUtil_ZbdManager = nullptr;
    ClearRegisteredHandlers(sentinel);
    if (manager.tempBuffer != nullptr) {
        ::operator delete(manager.tempBuffer);
        manager.tempBuffer = nullptr;
    }
    DeleteFileA(tempFile);
    return nullManagerOk && delegateOk ? 0 : 3;
}

extern "C" int zeffect_anim_shutdown_entry_smoke(void) {
    zEffectAnimEntry entry = {};
    entry.activationState = 0;
    entry.activationMode = 3;
    entry.surfacePrimary.eventStream = std::malloc(8);
    entry.surfacePrimary.eventStreamSize = 8;
    entry.runtimeSequenceCount = 2;
    entry.runtimeList =
        static_cast<zEffectAnimSurfaceRuntime *>(std::calloc(2, sizeof(zEffectAnimSurfaceRuntime)));
    entry.runtimeList[0].eventStream = std::malloc(4);
    entry.runtimeList[1].eventStream = std::malloc(4);
    entry.trackedNodeList =
        static_cast<zEffectAnimTrackedNode *>(std::malloc(sizeof(zEffectAnimTrackedNode)));
    entry.nodeRefList =
        static_cast<zEffectAnimNodeRef28 *>(std::malloc(sizeof(zEffectAnimNodeRef28)));
    entry.lightRefList =
        static_cast<zEffectAnimRuntimeNodeRef *>(std::malloc(sizeof(zEffectAnimRuntimeNodeRef)));
    entry.soundRefList =
        static_cast<zEffectAnimRuntimeNodeRef *>(std::malloc(sizeof(zEffectAnimRuntimeNodeRef)));
    entry.sampleRefList =
        static_cast<zEffectAnimSampleRef *>(std::malloc(sizeof(zEffectAnimSampleRef)));
    entry.effectTemplateRefList = static_cast<zEffectAnimTemplateIndexRef *>(
        std::malloc(sizeof(zEffectAnimTemplateIndexRef)));
    entry.activationPrereqList = static_cast<zEffectAnimActivationPrereq *>(
        std::malloc(sizeof(zEffectAnimActivationPrereq)));
    entry.runtimeRefList =
        static_cast<zEffectAnimRuntimeRef *>(std::malloc(sizeof(zEffectAnimRuntimeRef)));
    entry.runtimeSibling =
        static_cast<zEffectAnimEntry *>(std::calloc(1, sizeof(zEffectAnimEntry)));
    entry.runtimeSibling->activationState = 5;

    return zEffectAnim::ShutdownEntry(&entry) == 0 && entry.surfacePrimary.eventStream == nullptr &&
                   entry.surfacePrimary.eventStreamSize == 0
               ? 0
               : 1;
}

extern "C" int zeffect_anim_find_entry_by_name_smoke(void) {
    g_zEffectAnim_EntryList = nullptr;
    g_zEffectAnim_EntryCount = 2;
    if (zEffectAnim::FindEntryByName(nullptr) != nullptr) {
        g_zEffectAnim_EntryCount = 0;
        return 1;
    }

    g_zEffectAnim_EntryList =
        static_cast<zEffectAnimEntry *>(std::calloc(2, sizeof(zEffectAnimEntry)));
    if (g_zEffectAnim_EntryList == nullptr) {
        g_zEffectAnim_EntryCount = 0;
        return 2;
    }

    std::strcpy(g_zEffectAnim_EntryList[0].name, "introflash");
    std::strcpy(g_zEffectAnim_EntryList[1].name, "startcountdown");

    const bool ok = zEffectAnim::FindEntryByName("missing") == nullptr &&
                    zEffectAnim::FindEntryByName("introflash") == &g_zEffectAnim_EntryList[0] &&
                    zEffectAnim::FindEntryByName("startcountdown") == &g_zEffectAnim_EntryList[1];

    std::free(g_zEffectAnim_EntryList);
    g_zEffectAnim_EntryList = nullptr;
    g_zEffectAnim_EntryCount = 0;
    return ok ? 0 : 3;
}

extern "C" int zeffect_anim_find_node_recursive_by_name_smoke(void) {
    zClass_NodePartial root = {};
    zClass_NodePartial firstChild = {};
    zClass_NodePartial secondChild = {};
    zClass_NodePartial grandchild = {};
    zClass_NodePartial *rootChildren[] = {&firstChild, &secondChild};
    zClass_NodePartial *secondChildren[] = {&grandchild};

    std::strcpy(root.name, "root");
    std::strcpy(firstChild.name, "shared");
    std::strcpy(secondChild.name, "shared");
    std::strcpy(grandchild.name, "deep");
    root.listCountB = 2;
    root.listB = rootChildren;
    secondChild.listCountB = 1;
    secondChild.listB = secondChildren;

    if (zEffectAnim::FindNodeRecursiveByName(nullptr, "root") != nullptr) {
        return 1;
    }

    if (zEffectAnim::FindNodeRecursiveByName(&root, "root") != &root) {
        return 2;
    }

    if (zEffectAnim::FindNodeRecursiveByName(&root, "shared") != &firstChild) {
        return 3;
    }

    if (zEffectAnim::FindNodeRecursiveByName(&root, "deep") != &grandchild) {
        return 4;
    }

    return zEffectAnim::FindNodeRecursiveByName(&root, "missing") == nullptr ? 0 : 5;
}

extern "C" int zeffect_anim_ref_resolution_smoke(void) {
    zEffectAnimEntry entry = {};
    zClass_NodePartial callbackRoot = {};
    zClass_NodePartial callbackChild = {};
    zClass_NodePartial boundRoot = {};
    zClass_NodePartial boundChild = {};
    zClass_NodePartial lightZero = {};
    zClass_NodePartial lightOne = {};
    zClass_NodePartial soundZero = {};
    zClass_NodePartial soundOne = {};
    zClass_NodePartial *callbackChildren[] = {&callbackChild};
    zClass_NodePartial *boundChildren[] = {&boundChild};
    zEffectAnimRuntimeNodeRef lights[2] = {};
    zEffectAnimRuntimeNodeRef sounds[2] = {};

    std::strcpy(callbackRoot.name, "callback_root");
    std::strcpy(callbackChild.name, "callback_hit");
    std::strcpy(boundRoot.name, "bound_root");
    std::strcpy(boundChild.name, "bound_hit");
    std::strcpy(lightZero.name, "light_zero");
    std::strcpy(lightOne.name, "light_hit");
    std::strcpy(soundZero.name, "sound_zero");
    std::strcpy(soundOne.name, "sound_hit");

    callbackRoot.listCountB = 1;
    callbackRoot.listB = callbackChildren;
    boundRoot.listCountB = 1;
    boundRoot.listB = boundChildren;

    lights[0].runtimeNode = &lightZero;
    lights[1].runtimeNode = &lightOne;
    sounds[0].runtimeNode = &soundZero;
    sounds[1].runtimeNode = &soundOne;

    entry.callbackNode = &callbackRoot;
    entry.boundNode = &boundRoot;
    entry.lightRefCount = 2;
    entry.lightRefList = lights;
    entry.soundRefCount = 2;
    entry.soundRefList = sounds;

    if (zEffectAnim::FindLightRefIndexByName(&entry, "light_zero") != 0 ||
        zEffectAnim::FindLightRefIndexByName(&entry, "light_hit") != 1 ||
        zEffectAnim::FindLightRefIndexByName(&entry, "missing") != -1) {
        return 1;
    }

    if (zEffectAnim::FindSoundRefIndexByName(&entry, "sound_zero") != 0 ||
        zEffectAnim::FindSoundRefIndexByName(&entry, "sound_hit") != 1 ||
        zEffectAnim::FindSoundRefIndexByName(&entry, "missing") != -1) {
        return 2;
    }

    if (zEffectAnim::ResolveNodeByName(&entry, "callback_hit") != &callbackChild ||
        zEffectAnim::ResolveNodeByName(&entry, "bound_hit") != &boundChild ||
        zEffectAnim::ResolveNodeByName(&entry, "light_hit") != &lightOne ||
        zEffectAnim::ResolveNodeByName(&entry, "sound_hit") != &soundOne) {
        return 3;
    }

    return zEffectAnim::ResolveNodeByName(&entry, "light_zero") == &lightZero ? 4 : 0;
}

extern "C" int zeffect_anim_find_or_create_runtime_refs_smoke(void) {
    const std::int32_t oldInitialized = g_zSnd_IsInitialized;
    g_zSnd_IsInitialized = 0;

    ResetZClassRuntimeForZEffectTest();
    zClass_NodeFreeListSlot slots[2] = {};
    slots[0].freeTag = 1;
    slots[1].freeTag = 0x00ffffff;
    g_zClass_NodeArray = slots;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zEffectAnimEntry entry = {};
    const std::int32_t soundIndex = zEffectAnim::FindOrCreateSoundRef(&entry, "snd_ref");
    if (soundIndex != 1 || entry.soundRefCount != 2 || entry.soundRefList == nullptr ||
        entry.soundRefList[0].runtimeNode != nullptr ||
        entry.soundRefList[1].runtimeNode != &slots[0].node ||
        entry.soundRefList[1].isAttached != 0 ||
        std::strcmp(entry.soundRefList[1].name.text, "snd_ref") != 0) {
        g_zSnd_IsInitialized = oldInitialized;
        return 1;
    }

    zClass_SoundDataPartial *const soundData =
        static_cast<zClass_SoundDataPartial *>(slots[0].node.classData);
    if (std::strcmp(slots[0].node.name, "snd_ref") != 0 ||
        std::strcmp(soundData->sampleSetName, "snd_ref") != 0 ||
        zEffectAnim::FindOrCreateSoundRef(&entry, "snd_ref") != 1 || entry.soundRefCount != 2) {
        g_zSnd_IsInitialized = oldInitialized;
        return 2;
    }

    const std::int32_t lightIndex = zEffectAnim::FindOrCreateLightRef(&entry, "light_ref");
    if (lightIndex != 1 || entry.lightRefCount != 2 || entry.lightRefList == nullptr ||
        entry.lightRefList[0].runtimeNode != nullptr ||
        entry.lightRefList[1].runtimeNode != &slots[1].node ||
        entry.lightRefList[1].isAttached != 0 ||
        std::strcmp(entry.lightRefList[1].name.text, "light_ref") != 0 ||
        std::strcmp(slots[1].node.name, "light_ref") != 0 ||
        zEffectAnim::FindOrCreateLightRef(&entry, "light_ref") != 1 || entry.lightRefCount != 2) {
        g_zSnd_IsInitialized = oldInitialized;
        return 3;
    }

    zClass_Sound::DeleteNode(entry.soundRefList[1].runtimeNode);
    zClass_Light::DeleteNode(entry.lightRefList[1].runtimeNode);
    zClass_TypeList::FreeAll();
    std::free(entry.soundRefList);
    std::free(entry.lightRefList);
    g_zClass_NodeArray = nullptr;
    g_zClass_NodeFreeHeadIndex = -1;
    g_zSnd_IsInitialized = oldInitialized;
    return 0;
}

extern "C" int zeffect_anim_rebind_entry_to_node_smoke(void) {
    zEffectAnimEntry sameEntry = {};
    zClass_NodePartial oldSameRoot = {};
    zClass_NodePartial newSameRoot = {};
    std::strcpy(oldSameRoot.name, "old_same");
    std::strcpy(newSameRoot.name, "new_same");
    sameEntry.boundNode = &oldSameRoot;
    sameEntry.callbackNode = &oldSameRoot;

    if (zEffectAnim::RebindEntryToNode(nullptr, &newSameRoot) != nullptr ||
        zEffectAnim::RebindEntryToNode(&sameEntry, nullptr) != nullptr) {
        return 1;
    }

    if (zEffectAnim::RebindEntryToNode(&sameEntry, &newSameRoot) != &sameEntry ||
        sameEntry.boundNode != &newSameRoot || sameEntry.callbackNode != &newSameRoot ||
        std::strcmp(sameEntry.rootNodeName, "new_same") != 0 ||
        std::strcmp(sameEntry.attachNodeName, "new_same") != 0) {
        return 2;
    }

    zEffectAnimEntry stoppedEntry = {};
    stoppedEntry.activationState = 5;
    if (zEffectAnim::RebindEntryToNode(&stoppedEntry, &newSameRoot) != nullptr) {
        return 3;
    }

    zEffectAnimEntry entry = {};
    zClass_NodePartial oldBound = {};
    zClass_NodePartial oldCallback = {};
    zClass_NodePartial newBound = {};
    zClass_NodePartial attachNode = {};
    zClass_NodePartial trackedOld = {};
    zClass_NodePartial trackedTarget = {};
    zClass_NodePartial nodeRefOld = {};
    zClass_NodePartial nodeRefTarget = {};
    zClass_NodePartial prereqParent = {};
    zClass_NodePartial prereqChild = {};
    zClass_NodePartial *newBoundChildren[] = {&attachNode, &trackedTarget, &nodeRefTarget,
                                              &prereqParent};
    zClass_NodePartial *prereqChildren[] = {&prereqChild};
    zEffectAnimTrackedNode tracked[1] = {};
    zEffectAnimNodeRef28 nodeRefs[1] = {};
    zEffectAnimActivationPrereq prereqs[2] = {};

    std::strcpy(oldBound.name, "old_bound");
    std::strcpy(oldCallback.name, "old_callback");
    std::strcpy(newBound.name, "new_bound");
    std::strcpy(attachNode.name, "attach");
    std::strcpy(trackedTarget.name, "tracked_new");
    std::strcpy(nodeRefTarget.name, "node_ref_new");
    std::strcpy(prereqParent.name, "prereq_parent");
    std::strcpy(prereqChild.name, "prereq_child");

    newBound.listCountB = 4;
    newBound.listB = newBoundChildren;
    prereqParent.listCountB = 1;
    prereqParent.listB = prereqChildren;

    std::strcpy(entry.attachNodeName, "attach");
    entry.boundNode = &oldBound;
    entry.callbackNode = &oldCallback;
    entry.trackedNodeCount = 1;
    entry.trackedNodeList = tracked;
    entry.nodeRefCount = 1;
    entry.nodeRefList = nodeRefs;
    entry.activationPrereqCount = 2;
    entry.activationPrereqList = prereqs;

    std::strcpy(tracked[0].trackedNodeName, "tracked_new");
    tracked[0].trackedNode = &trackedOld;
    std::strcpy(nodeRefs[0].name.text, "node_ref_new");
    nodeRefs[0].node = &nodeRefOld;
    prereqs[0].mode = 3;
    std::strcpy(&prereqs[0].targetName[4], "prereq_parent");
    prereqs[1].mode = 2;
    std::strcpy(&prereqs[1].targetName[4], "prereq_child");

    if (zEffectAnim::RebindEntryToNode(&entry, &newBound) != &entry) {
        return 4;
    }

    const bool ok = entry.boundNode == &newBound && entry.callbackNode == &attachNode &&
                    std::strcmp(entry.rootNodeName, "new_bound") == 0 &&
                    std::strcmp(entry.attachNodeName, "attach") == 0 &&
                    tracked[0].trackedNode == &trackedTarget &&
                    nodeRefs[0].node == &nodeRefTarget && prereqs[1].targetNode == &prereqChild &&
                    entry.activationState == 0;

    return ok ? 0 : 5;
}

extern "C" int zeffect_anim_ensure_copied_root_tree_smoke(void) {
    if (zEffectAnim::EnsureCopiedRootTree(nullptr, nullptr) != 0) {
        return 1;
    }

    zEffectAnimEntry noCopyEntry = {};
    zClass_NodePartial oldRoot = {};
    noCopyEntry.boundNode = &oldRoot;
    if (zEffectAnim::EnsureCopiedRootTree(&noCopyEntry, nullptr) != 1 ||
        noCopyEntry.boundNode != &oldRoot) {
        return 2;
    }

    zEffectAnimEntry entry = {};
    zClass_NodePartial sourceRoot = {};
    std::strcpy(sourceRoot.name, "copied_root");
    sourceRoot.flags = 0x04000000;
    entry.flags = 0x00008000;
    entry.boundNode = &sourceRoot;
    entry.callbackNode = &sourceRoot;

    if (zEffectAnim::EnsureCopiedRootTree(&entry, &sourceRoot) != 1) {
        return 3;
    }

    const bool ok = entry.boundNode == &sourceRoot && entry.callbackNode == &sourceRoot &&
                    std::strcmp(entry.rootNodeName, "copied_root") == 0 &&
                    std::strcmp(entry.attachNodeName, "copied_root") == 0 &&
                    (entry.flags & 0x00008000) == 0;

    return ok ? 0 : 4;
}

extern "C" int zeffect_anim_clone_entry_for_node_smoke(void) {
    if (zEffectAnim::CloneEntryForNode(nullptr, nullptr) != nullptr) {
        return 1;
    }

    ResetZClassRuntimeForZEffectTest();
    zClass_NodeFreeListSlot runtimeSlot = {};
    runtimeSlot.freeTag = 0x00ffffff;
    g_zClass_NodeArray = &runtimeSlot;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zEffectAnimEntry entry = {};
    zClass_NodePartial oldBound = {};
    zClass_NodePartial cloneRoot = {};
    zClass_NodePartial trackedTarget = {};
    zClass_NodePartial nodeRefTarget = {};
    zClass_NodePartial *cloneChildren[] = {&trackedTarget, &nodeRefTarget};
    zEffectAnimRuntimeNodeRef lightRefs[1] = {};
    zEffectAnimRuntimeNodeRef soundRefs[1] = {};
    zEffectAnimTrackedNode tracked[1] = {};
    zEffectAnimNodeRef28 nodeRefs[1] = {};
    zEffectAnimSurfaceRuntime runtime[1] = {};
    char runtimeEventStream[4] = {'r', 'u', 'n', '!'};
    char primaryEventStream[4] = {'m', 'a', 'i', 'n'};
    zEffectAnimSampleRef sampleRefs[1] = {};
    zEffectAnimTemplateIndexRef templateRefs[1] = {};
    zEffectAnimActivationPrereq prereqs[1] = {};
    zEffectAnimRuntimeRef runtimeRefs[1] = {};
    zEffectAnimEntry cachedChild = {};

    std::strcpy(entry.name, "anim");
    entry.flags = 0x00000120;
    entry.priority = 7;
    entry.activationMode = 3;
    entry.boundNode = &oldBound;
    entry.callbackNode = &oldBound;
    entry.lightRefCount = 1;
    entry.lightRefList = lightRefs;
    entry.soundRefCount = 1;
    entry.soundRefList = soundRefs;
    entry.trackedNodeCount = 1;
    entry.trackedNodeList = tracked;
    entry.nodeRefCount = 1;
    entry.nodeRefList = nodeRefs;
    entry.runtimeSequenceCount = 1;
    entry.runtimeList = runtime;
    entry.surfacePrimary.eventStream = primaryEventStream;
    entry.surfacePrimary.eventStreamSize = sizeof(primaryEventStream);
    entry.sampleRefCount = 1;
    entry.sampleRefList = sampleRefs;
    entry.effectTemplateRefCount = 1;
    entry.effectTemplateRefList = templateRefs;
    entry.activationPrereqCount = 1;
    entry.activationPrereqList = prereqs;
    entry.runtimeRefCount = 1;
    entry.runtimeRefList = runtimeRefs;
    entry.runtimeSibling = &cachedChild;

    std::strcpy(oldBound.name, "old_bound");
    std::strcpy(cloneRoot.name, "clone_root");
    std::strcpy(trackedTarget.name, "tracked_target");
    std::strcpy(nodeRefTarget.name, "node_ref_target");
    cloneRoot.listCountB = 2;
    cloneRoot.listB = cloneChildren;

    std::strcpy(lightRefs[0].name.text, "light_ref");
    lightRefs[0].isAttached = 1;
    std::strcpy(soundRefs[0].name.text, "sound_ref");
    soundRefs[0].isAttached = 1;
    std::strcpy(tracked[0].trackedNodeName, "tracked_target");
    tracked[0].trackedNode = &oldBound;
    std::strcpy(nodeRefs[0].name.text, "node_ref_target");
    nodeRefs[0].node = &oldBound;
    runtime[0].eventStream = runtimeEventStream;
    runtime[0].eventStreamSize = sizeof(runtimeEventStream);
    std::strcpy(sampleRefs[0].name, "sample");
    std::strcpy(templateRefs[0].name, "template");
    templateRefs[0].templateIndex = 9;
    prereqs[0].requireMatch = 1;
    prereqs[0].mode = 2;
    std::strcpy(runtimeRefs[0].entryName, "child");
    runtimeRefs[0].stopCachedChildOnCleanup = 1;
    runtimeRefs[0].cachedChildEntry = &cachedChild;

    zEffectAnimEntry *const cloned = zEffectAnim::CloneEntryForNode(&entry, &cloneRoot);
    if (cloned == nullptr) {
        g_zClass_NodeArray = nullptr;
        g_zClass_NodeFreeHeadIndex = -1;
        return 2;
    }

    const bool ok = cloned != &entry && cloned->runtimeNode == &runtimeSlot.node &&
                    std::strcmp(cloned->runtimeNode->name, "_anim") == 0 &&
                    cloned->runtimeNode->callbackPriority == entry.priority &&
                    cloned->runtimeSibling == nullptr && (cloned->flags & 0x00000100) == 0 &&
                    (cloned->flags & 0x00000020) != 0 && cloned->boundNode == &cloneRoot &&
                    cloned->callbackNode == &cloneRoot &&
                    std::strcmp(cloned->rootNodeName, "clone_root") == 0 &&
                    std::strcmp(cloned->attachNodeName, "clone_root") == 0 &&
                    cloned->lightRefList != lightRefs && cloned->lightRefList[0].isAttached == 0 &&
                    cloned->soundRefList != soundRefs && cloned->soundRefList[0].isAttached == 0 &&
                    cloned->trackedNodeList != tracked &&
                    cloned->trackedNodeList[0].trackedNode == &trackedTarget &&
                    cloned->nodeRefList != nodeRefs &&
                    cloned->nodeRefList[0].node == &nodeRefTarget &&
                    cloned->runtimeList != runtime &&
                    cloned->runtimeList[0].eventStream != runtimeEventStream &&
                    std::memcmp(cloned->runtimeList[0].eventStream, runtimeEventStream,
                                sizeof(runtimeEventStream)) == 0 &&
                    cloned->surfacePrimary.eventStream != primaryEventStream &&
                    std::memcmp(cloned->surfacePrimary.eventStream, primaryEventStream,
                                sizeof(primaryEventStream)) == 0 &&
                    cloned->sampleRefList != sampleRefs &&
                    std::strcmp(cloned->sampleRefList[0].name, "sample") == 0 &&
                    cloned->effectTemplateRefList != templateRefs &&
                    cloned->effectTemplateRefList[0].templateIndex == 9 &&
                    cloned->activationPrereqList != prereqs &&
                    cloned->activationPrereqList[0].requireMatch == 1 &&
                    cloned->runtimeRefList != runtimeRefs &&
                    cloned->runtimeRefList[0].stopCachedChildOnCleanup == 1 &&
                    cloned->runtimeRefList[0].cachedChildEntry == nullptr;

    zEffectAnim::ShutdownEntry(cloned);
    std::free(cloned);
    zClass_TypeList::FreeAll();
    g_zClass_NodeArray = nullptr;
    g_zClass_NodeFreeHeadIndex = -1;
    return ok ? 0 : 3;
}

extern "C" int zeffect_cleanup_light_sound_refs_smoke(void) {
    zClass_WorldDataPartial worldData{};
    zClass_NodePartial world{};
    world.classData = &worldData;
    g_zEffect_World = &world;

    zClass_NodePartial otherWorld{};
    zClass_NodePartial lightNode{};
    zClass_LightDataPartial lightData{};
    zClass_NodePartial *lightNodes[] = {&lightNode};
    zClass_LightDataPartial *lightDataList[] = {&lightData};
    zClass_NodePartial *lightWorlds[] = {&world, &otherWorld};
    lightNode.classId = 5;
    lightNode.flags = 0x04;
    lightData.attachedWorldCount = 2;
    lightData.attachedWorlds = lightWorlds;
    worldData.lightCount = 1;
    worldData.lightNodes = lightNodes;
    worldData.lightDataList = lightDataList;

    zClass_NodePartial soundNode{};
    zClass_SoundDataPartial soundData{};
    zClass_NodePartial *soundNodes[] = {&soundNode};
    zClass_SoundDataPartial *soundDataList[] = {&soundData};
    zClass_NodePartial *soundWorlds[] = {&otherWorld, &world};
    soundNode.classId = 5;
    soundNode.flags = 0x04;
    soundData.attachedWorldCount = 2;
    soundData.attachedWorlds = soundWorlds;
    worldData.soundCount = 1;
    worldData.soundNodes = soundNodes;
    worldData.soundDataList = soundDataList;

    zEffectAnimRuntimeNodeRef lightRefs[1] = {};
    zEffectAnimRuntimeNodeRef soundRefs[1] = {};
    lightRefs[0].runtimeNode = &lightNode;
    lightRefs[0].isAttached = 1;
    soundRefs[0].runtimeNode = &soundNode;
    soundRefs[0].isAttached = 1;

    zEffectAnimEntry entry = {};
    entry.lightRefCount = 1;
    entry.lightRefList = lightRefs;
    entry.soundRefCount = 1;
    entry.soundRefList = soundRefs;

    if (zEffect::CleanupLightRefs(&entry) != 0 || worldData.lightCount != 0 ||
        lightData.attachedWorldCount != 1 || lightData.attachedWorlds[0] != &otherWorld ||
        lightRefs[0].isAttached != 0 || (lightNode.flags & 0x04) != 0) {
        g_zEffect_World = nullptr;
        return 1;
    }

    if (zEffect::CleanupSoundRefs(&entry) != 0 || worldData.soundCount != 0 ||
        soundData.attachedWorldCount != 1 || soundData.attachedWorlds[0] != &otherWorld ||
        soundRefs[0].isAttached != 0 || (soundNode.flags & 0x04) != 0) {
        g_zEffect_World = nullptr;
        return 2;
    }

    g_zEffect_World = nullptr;
    return 0;
}

extern "C" int zeffect_handle_sample_ref_offset_event_smoke(void) {
    const std::int32_t oldInitialized = g_zSnd_IsInitialized;
    const std::int32_t oldPreInitialized = g_zSnd_PreInitialized;
    const std::int32_t oldActiveBackend = g_zSnd_ActiveBackend;
    const std::int32_t oldMuteDepth = g_zSnd_MuteDepth;
    const std::int32_t oldPlaybackEnabled = g_zSnd_Flag10PlaybackEnabled;
    void *const oldGlobalVolumePtr = g_zSnd_GlobalVolumeScalePtr;
    const std::int32_t oldListenerValid = g_zSnd_ListenerStateValid;
    const zSndListenerState oldListenerState = g_zSnd_ListenerState;
    const zVec3 oldListenerVelocity = g_zSnd_ListenerVelocity;
    const float oldInvSpeedOfSound = g_zSndInvSpeedOfSoundMps;

    auto restoreSoundGlobals = [&]() {
        g_zSnd_IsInitialized = oldInitialized;
        g_zSnd_PreInitialized = oldPreInitialized;
        g_zSnd_ActiveBackend = oldActiveBackend;
        g_zSnd_MuteDepth = oldMuteDepth;
        g_zSnd_Flag10PlaybackEnabled = oldPlaybackEnabled;
        g_zSnd_GlobalVolumeScalePtr = oldGlobalVolumePtr;
        g_zSnd_ListenerStateValid = oldListenerValid;
        g_zSnd_ListenerState = oldListenerState;
        g_zSnd_ListenerVelocity = oldListenerVelocity;
        g_zSndInvSpeedOfSoundMps = oldInvSpeedOfSound;
    };

    g_zSnd_IsInitialized = 0;
    g_zSnd_PreInitialized = 0;

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    sample.replayFields.gain = 1.0f;
    sample.rangeMin = 0.0f;
    sample.rangeMax = 100.0f;
    zEffectAnimSampleRef samples[1] = {};
    samples[0].sample = &sample;

    zClass_Object3DDataPartial nodeData = {};
    nodeData.cachedWorldMatrix[9] = 1.0f;
    nodeData.cachedWorldMatrix[10] = 2.0f;
    nodeData.cachedWorldMatrix[11] = 3.0f;
    zClass_NodePartial node = {};
    node.classId = 5;
    node.flags = 0x00080000;
    node.classData = &nodeData;
    zEffectAnimNodeRef28 nodeRefs[2] = {};
    nodeRefs[1].node = &node;

    zEffectAnimEntry entry = {};
    entry.sampleRefList = samples;
    entry.nodeRefList = nodeRefs;

    zEffectAnimRefOffsetEvent event = {};
    event.refIndex = 0;
    event.nodeRefIndex = 0;
    if (zEffect::HandleSampleRefOffsetEvent(&entry, &event) != 2) {
        restoreSoundGlobals();
        return 1;
    }

    EffectDirectSoundBufferVTable directSoundVTable = {};
    directSoundVTable.GetFrequency = &EffectDirectSoundGetFrequency;
    directSoundVTable.GetStatus = &EffectDirectSoundGetStatus;
    directSoundVTable.SetVolume = &EffectDirectSoundSetVolume;
    directSoundVTable.SetPan = &EffectDirectSoundSetPan;
    directSoundVTable.SetFrequency = &EffectDirectSoundSetFrequency;
    directSoundVTable.SetCurrentPosition = &EffectDirectSoundSetCurrentPosition;
    directSoundVTable.Play = &EffectDirectSoundPlay;
    EffectDirectSoundBuffer directSoundBuffer{&directSoundVTable};
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);

    float globalVolume = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_MuteDepth = 0;
    g_zSnd_Flag10PlaybackEnabled = 1;
    g_zSnd_ListenerStateValid = 1;
    g_zSnd_ListenerState.position = {};
    g_zSnd_ListenerState.right = {1.0f, 0.0f, 0.0f};
    g_zSnd_ListenerVelocity = {};
    g_zSndInvSpeedOfSoundMps = 0.0f;

    ResetEffectDirectSoundCounters();
    event.nodeRefIndex = 0;
    if (zEffect::HandleSampleRefOffsetEvent(&entry, &event) != 2 ||
        sample.primaryVoice.ownerSample != &sample || sample.primaryVoice.hasWorldPos != 0 ||
        g_effectDirectSoundSetVolumeCount != 1 ||
        g_effectDirectSoundSetCurrentPositionCount != 1 || g_effectDirectSoundPlayCount != 1) {
        restoreSoundGlobals();
        return 2;
    }

    sample.replayFields.flags = 0x0c;
    ResetEffectDirectSoundCounters();
    event.nodeRefIndex = 1;
    event.offsetX = 4.0f;
    event.offsetY = 5.0f;
    event.offsetZ = 6.0f;
    const std::int32_t result = zEffect::HandleSampleRefOffsetEvent(&entry, &event);
    const bool offsetOk = result == 2 && sample.primaryVoice.hasWorldPos == 1 &&
                          sample.primaryVoice.worldPos.x == 5.0f &&
                          sample.primaryVoice.worldPos.y == 7.0f &&
                          sample.primaryVoice.worldPos.z == 9.0f &&
                          g_effectDirectSoundSetPanCount == 1 &&
                          g_effectDirectSoundSetVolumeCount == 1 &&
                          g_effectDirectSoundSetFrequencyCount == 0 &&
                          g_effectDirectSoundSetCurrentPositionCount == 1 &&
                          g_effectDirectSoundPlayCount == 1;
    if (!offsetOk) {
        const int code = result != 2 ? 30
                         : sample.primaryVoice.hasWorldPos != 1 ? 31
                         : sample.primaryVoice.worldPos.x != 5.0f ? 32
                         : sample.primaryVoice.worldPos.y != 7.0f ? 33
                         : sample.primaryVoice.worldPos.z != 9.0f ? 34
                         : g_effectDirectSoundSetPanCount != 1 ? 35
                         : g_effectDirectSoundSetVolumeCount != 1 ? 36
                         : g_effectDirectSoundSetFrequencyCount != 0 ? 37
                         : g_effectDirectSoundSetCurrentPositionCount != 1 ? 38
                         : 39;
        restoreSoundGlobals();
        return code;
    }
    restoreSoundGlobals();
    return 0;
}

extern "C" int zeffect_handle_sound_light_events_smoke(void) {
    zClass_WorldDataPartial worldData{};
    zClass_NodePartial world{};
    world.classData = &worldData;
    g_zEffect_World = &world;

    zClass_Object3DDataPartial parentData = {};
    parentData.cachedWorldMatrix[9] = 10.0f;
    parentData.cachedWorldMatrix[10] = 20.0f;
    parentData.cachedWorldMatrix[11] = 30.0f;
    zClass_NodePartial parentNode = {};
    parentNode.classId = 5;
    parentNode.flags = 0x00080000;
    parentNode.classData = &parentData;

    zEffectAnimNodeRef28 nodeRefs[2] = {};
    nodeRefs[1].node = &parentNode;

    zClass_SoundDataPartial soundData{};
    zClass_NodePartial soundNode{};
    soundNode.classId = 10;
    soundNode.classData = &soundData;
    zEffectAnimRuntimeNodeRef soundRefs[2] = {};
    soundRefs[1].runtimeNode = &soundNode;

    zClass_LightDataPartial lightData{};
    zClass_NodePartial lightNode{};
    lightNode.classId = 9;
    lightNode.classData = &lightData;
    zEffectAnimRuntimeNodeRef lightRefs[2] = {};
    lightRefs[1].runtimeNode = &lightNode;

    zEffectAnimEntry entry = {};
    entry.nodeRefList = nodeRefs;
    entry.soundRefList = soundRefs;
    entry.lightRefList = lightRefs;

    zEffectAnimSoundEvent soundEvent = {};
    soundEvent.soundRefIndex = 1;
    soundEvent.fieldMask = 0x03;
    soundEvent.activeState = 1;
    soundEvent.parentNodeRefIndex = 1;
    soundEvent.offsetX = 1.0f;
    soundEvent.offsetY = 2.0f;
    soundEvent.offsetZ = 3.0f;
    if (zEffect::HandleSoundEvent(&entry, &soundEvent) != 2 || worldData.soundCount != 1 ||
        worldData.soundNodes[0] != &soundNode || worldData.soundDataList[0] != &soundData ||
        soundData.attachedWorldCount != 1 || soundData.attachedWorlds[0] != &world ||
        soundRefs[1].isAttached != 1 || (soundNode.flags & 0x04) == 0 ||
        soundData.localPosition.x != 11.0f || soundData.localPosition.y != 22.0f ||
        soundData.localPosition.z != 33.0f || (soundData.runtimeFlags & 0x03) != 0x03) {
        g_zEffect_World = nullptr;
        return 1;
    }

    soundEvent.fieldMask = 0;
    soundEvent.activeState = 0;
    if (zEffect::HandleSoundEvent(&entry, &soundEvent) != 2 || worldData.soundCount != 0 ||
        soundData.attachedWorldCount != 0 || soundRefs[1].isAttached != 0 ||
        (soundNode.flags & 0x04) != 0) {
        g_zEffect_World = nullptr;
        return 2;
    }

    std::uint32_t coneAngleBits = 0;
    StoreFloatBits(coneAngleBits, 0.5f);
    zEffectAnimLightEvent lightEvent = {};
    lightEvent.lightRefIndex = 1;
    lightEvent.fieldMask = 0x1ff;
    lightEvent.activeState = 1;
    lightEvent.mode = 0;
    lightEvent.coneAngleBits = coneAngleBits;
    lightEvent.param = 7;
    lightEvent.basisNodeRefIndex = 1;
    lightEvent.basisOrColorX = 4.0f;
    lightEvent.basisOrColorY = 5.0f;
    lightEvent.basisOrColorZ = 6.0f;
    lightEvent.positionX = 0.1f;
    lightEvent.positionY = 0.2f;
    lightEvent.positionZ = 0.3f;
    lightEvent.rangeInner = 3.0f;
    lightEvent.rangeOuter = 9.0f;
    lightEvent.specularR = 0.2f;
    lightEvent.specularG = 0.3f;
    lightEvent.specularB = 0.4f;
    lightEvent.intensity = 0.75f;
    lightEvent.falloff = 0.25f;

    if (zEffect::HandleLightEvent(&entry, &lightEvent) != 2 || worldData.lightCount != 1 ||
        worldData.lightNodes[0] != &lightNode || worldData.lightDataList[0] != &lightData ||
        lightData.attachedWorldCount != 1 || lightData.attachedWorlds[0] != &world ||
        lightRefs[1].isAttached != 1 || (lightNode.flags & 0x04) == 0 ||
        lightData.isPointMode != 1 || lightData.isDirectionalMode != 0 ||
        lightData.localPosition.x != 14.0f || lightData.localPosition.y != 25.0f ||
        lightData.localPosition.z != 36.0f || lightData.localRotation.x != 0.1f ||
        lightData.localRotation.y != 0.2f || lightData.localRotation.z != 0.3f ||
        lightData.range1 != 3.0f || lightData.range2 != 9.0f ||
        lightData.specularColor.red != 0.2f || lightData.specularColor.green != 0.3f ||
        lightData.specularColor.blue != 0.4f || lightData.intensityScale != 0.75f ||
        lightData.falloff != 0.25f || lightData.coneAngle != 0.5f || lightData.lightParam != 7) {
        g_zEffect_World = nullptr;
        return 3;
    }

    lightEvent.fieldMask = 0;
    lightEvent.activeState = 0;
    if (zEffect::HandleLightEvent(&entry, &lightEvent) != 2 || worldData.lightCount != 0 ||
        lightData.attachedWorldCount != 0 || lightRefs[1].isAttached != 0 ||
        (lightNode.flags & 0x04) != 0) {
        g_zEffect_World = nullptr;
        return 4;
    }

    std::free(worldData.soundNodes);
    std::free(worldData.soundDataList);
    std::free(soundData.attachedWorlds);
    std::free(worldData.lightNodes);
    std::free(worldData.lightDataList);
    std::free(lightData.attachedWorlds);
    g_zEffect_World = nullptr;
    return 0;
}

extern "C" int zeffect_handle_light_anim_event_smoke(void) {
    g_zVideo_RendererType = 0;
    g_zVideo_PixelPack_RShift = 8;
    g_zVideo_PixelPack_GShift = 3;
    g_zVideo_PixelPack_BShiftTo8 = 3;
    g_zVideo_PixelPack_RMask = 0xf800;
    g_zVideo_PixelPack_GMask = 0x07e0;

    zClass_LightDataPartial lightData{};
    lightData.range1 = 8.0f;
    lightData.range2 = 16.0f;
    lightData.specularColor = {0.25f, 0.5f, 0.75f};

    zClass_NodePartial lightNode{};
    lightNode.classId = 9;
    lightNode.classData = &lightData;

    zEffectAnimRuntimeNodeRef lightRefs[2] = {};
    lightRefs[1].runtimeNode = &lightNode;

    zEffectAnimEntry entry = {};
    entry.lightRefList = lightRefs;

    zEffectAnimSurfaceRuntime runtime = {};
    runtime.runState = 0;
    runtime.eventElapsedSec = 0.5f;

    zEffectLightRangeSpecularAnimEvent event = {};
    event.lightRefIndex = 1;
    event.initialRangeInner = 4.0f;
    event.initialRangeOuter = 8.0f;
    event.rangeInnerDelta = 4.0f;
    event.rangeOuterDelta = -4.0f;
    event.initialSpecularR = 0.5f;
    event.initialSpecularG = -1.0f;
    event.initialSpecularB = 1.0f;
    event.specularRDelta = 0.5f;
    event.specularGDelta = 0.25f;
    event.specularBDelta = -0.5f;
    event.durationSec = 1.0f;

    g_zEffect_FrameDeltaRemainingSec = 0.25f;
    if (zEffect::HandleLightAnimEvent(&entry, &runtime, &event) != 1 ||
        g_zEffect_FrameDeltaRemainingSec != 0.0f || lightData.range1 != 9.0f ||
        lightData.range2 != 18.0f || event.currentRangeInner != 5.0f ||
        event.currentRangeOuter != 7.0f || lightData.specularColor.red != 0.375f ||
        lightData.specularColor.green != 0.25f || lightData.specularColor.blue != 1.0f ||
        event.currentSpecularR != 0.625f || event.currentSpecularG != -0.9375f ||
        event.currentSpecularB != 0.875f) {
        return 1;
    }

    runtime.runState = 1;
    runtime.eventElapsedSec = 1.25f;
    event.currentRangeInner = -40.0f;
    event.currentRangeOuter = -100.0f;
    event.rangeInnerDelta = 0.0f;
    event.rangeOuterDelta = 0.0f;
    event.currentSpecularR = 4.0f;
    event.currentSpecularG = -4.0f;
    event.currentSpecularB = 0.0f;
    event.specularRDelta = 0.0f;
    event.specularGDelta = 0.0f;
    event.specularBDelta = 0.0f;
    g_zEffect_FrameDeltaRemainingSec = 0.5f;

    return zEffect::HandleLightAnimEvent(&entry, &runtime, &event) == 2 &&
                   g_zEffect_FrameDeltaRemainingSec == 0.25f && lightData.range1 == 1.0f &&
                   lightData.range2 == 2.0f && lightData.specularColor.red == 1.0f &&
                   lightData.specularColor.green == 0.0f && lightData.specularColor.blue == 1.0f
               ? 0
               : 2;
}

extern "C" int zeffect_handle_fog_event_smoke(void) {
    zClass_WorldDataPartial worldData{};
    zClass_NodePartial world{};
    world.classData = &worldData;
    g_zEffect_World = &world;

    zEffectAnimEntry entry{};
    zEffectFogEvent event{};
    event.flags = 0x0f;
    event.fogState = 2;
    event.fogColorR = 0.2f;
    event.fogColorG = 0.4f;
    event.fogColorB = 0.6f;
    event.fogAltitudeMin = -3.0f;
    event.fogAltitudeMax = 9.0f;
    event.fogRangeStart = 12.0f;
    event.fogRangeEnd = 48.0f;

    if (zEffect::HandleFogEvent(&entry, &event) != 2 || worldData.flags != 0x27 ||
        worldData.fogState != 2 || worldData.ambientColor.red != 0.2f ||
        worldData.ambientColor.green != 0.4f || worldData.ambientColor.blue != 0.6f ||
        worldData.fogHeightLow != -3.0f || worldData.fogHeightHigh != 9.0f ||
        worldData.fogDistanceStart != 12.0f || worldData.fogDistanceEnd != 48.0f) {
        g_zEffect_World = nullptr;
        return 1;
    }

    worldData = {};
    event.flags = 0;
    const int result = zEffect::HandleFogEvent(&entry, &event);
    g_zEffect_World = nullptr;
    return result == 2 && worldData.flags == 0 ? 0 : 2;
}

extern "C" int zeffect_handle_camera_params_event_smoke(void) {
    zClass_CameraDataPartial cameraData{};
    cameraData.nearClip = 0.25f;
    cameraData.farClip = 500.0f;
    cameraData.clipDistance = 16.0f;
    cameraData.invClipDistanceSq = 1.0f / 256.0f;
    cameraData.viewportWidth = 320.0f;
    cameraData.viewportHeight = 200.0f;
    cameraData.frustumWidth = 640.0f;
    cameraData.frustumHeight = 480.0f;

    zClass_NodePartial cameraNode{};
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;

    zEffectAnimNodeRef28 nodeRefs[1] = {};
    nodeRefs[0].node = &cameraNode;

    zEffectAnimEntry entry{};
    entry.nodeRefList = nodeRefs;

    zEffectAnimSurfaceRuntime runtime{};
    zEffectCameraEvent event{};
    event.flags = 0x7f;
    event.targetNodeRefIndex = 0;
    event.nearClip = 0.5f;
    event.farClip = 250.0f;
    event.clipDistance = 4.0f;
    event.fovPrimary = 1.0f;
    event.fovSecondary = 0.5f;
    event.viewportPrimary = 400.0f;
    event.viewportSecondary = 300.0f;

    if (zEffect::HandleCameraParamsEvent(&entry, &runtime, &event) != 2 ||
        cameraData.nearClip != 0.5f || cameraData.farClip != 250.0f ||
        cameraData.clipDistance != 4.0f || cameraData.invClipDistanceSq != 0.0625f ||
        cameraData.frustumWidth != 1.0f || cameraData.frustumHeight != 0.5f ||
        cameraData.viewportWidth != 400.0f || cameraData.viewportHeight != 300.0f ||
        !FloatNear(cameraData.fovX, 1.0f / 400.0f) ||
        !FloatNear(cameraData.fovY, 0.5f / 300.0f) ||
        !FloatNear(cameraData.frustumYaw, (1.0f / 400.0f) * 0.5f) ||
        !FloatNear(cameraData.frustumPitch, (0.5f / 300.0f) * 0.5f) ||
        cameraData.localFrustumNormalsDirty != 1 || cameraData.frustumVectorsDirty != 1) {
        return 1;
    }

    event.targetNodeRefIndex = -1;
    cameraData.nearClip = 0.75f;
    if (zEffect::HandleCameraParamsEvent(&entry, &runtime, &event) != 2 ||
        cameraData.nearClip != 0.75f) {
        return 2;
    }

    event.targetNodeRefIndex = 0;
    if (zEffect::HandleCameraParamsEvent(nullptr, &runtime, &event) != 2 ||
        zEffect::HandleCameraParamsEvent(&entry, nullptr, &event) != 2 ||
        zEffect::HandleCameraParamsEvent(&entry, &runtime, nullptr) != 2) {
        return 3;
    }

    return 0;
}

extern "C" int zeffect_animate_camera_params_over_time_smoke(void) {
    zClass_CameraDataPartial cameraData{};
    cameraData.nearClip = 0.25f;
    cameraData.farClip = 500.0f;
    cameraData.clipDistance = 16.0f;
    cameraData.invClipDistanceSq = 1.0f / 256.0f;
    cameraData.viewportWidth = 320.0f;
    cameraData.viewportHeight = 200.0f;
    cameraData.frustumWidth = 640.0f;
    cameraData.frustumHeight = 480.0f;

    zClass_NodePartial cameraNode{};
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;

    zEffectAnimNodeRef28 nodeRefs[1] = {};
    nodeRefs[0].node = &cameraNode;

    zEffectAnimEntry entry{};
    entry.nodeRefList = nodeRefs;

    zEffectAnimSurfaceRuntime runtime{};
    runtime.runState = 0;
    runtime.eventElapsedSec = 0.25f;

    zEffectCameraAnimEvent event{};
    event.flags = 0x7f;
    event.targetNodeRefIndex = 0;
    event.nearClipStart = 1.0f;
    event.nearClipEnd = 3.0f;
    event.nearClipRate = 4.0f;
    event.farClipStart = 100.0f;
    event.farClipEnd = 120.0f;
    event.farClipRate = 8.0f;
    event.clipDistanceStart = 10.0f;
    event.clipDistanceEnd = 12.0f;
    event.clipDistanceRate = 4.0f;
    event.fovPrimaryStart = 2.0f;
    event.fovPrimaryEnd = 2.5f;
    event.fovPrimaryRate = 0.4f;
    event.fovSecondaryStart = 1.0f;
    event.fovSecondaryEnd = 1.5f;
    event.fovSecondaryRate = 0.8f;
    event.viewportPrimaryStart = 400.0f;
    event.viewportPrimaryEnd = 420.0f;
    event.viewportPrimaryRate = 40.0f;
    event.viewportSecondaryStart = 200.0f;
    event.viewportSecondaryEnd = 240.0f;
    event.viewportSecondaryRate = 80.0f;
    event.endTime = 1.0f;

    g_zEffect_FrameDeltaRemainingSec = 0.25f;
    if (zEffect::AnimateCameraParamsOverTime(&entry, &runtime, &event) != 1 ||
        g_zEffect_FrameDeltaRemainingSec != 0.0f || !FloatNear(cameraData.nearClip, 2.0f) ||
        !FloatNear(cameraData.farClip, 102.0f) ||
        !FloatNear(cameraData.clipDistance, 11.0f) ||
        !FloatNear(cameraData.invClipDistanceSq, 1.0f / 121.0f) ||
        !FloatNear(cameraData.frustumWidth, 2.1f) ||
        !FloatNear(cameraData.frustumHeight, 1.2f) ||
        !FloatNear(cameraData.viewportWidth, 410.0f) ||
        !FloatNear(cameraData.viewportHeight, 220.0f) ||
        !FloatNear(cameraData.fovX, 2.1f / 410.0f) ||
        !FloatNear(cameraData.fovY, 1.2f / 220.0f)) {
        return 1;
    }

    runtime.runState = 1;
    runtime.eventElapsedSec = 1.25f;
    g_zEffect_FrameDeltaRemainingSec = 0.5f;
    cameraData.nearClip = 2.0f;
    cameraData.farClip = 102.0f;
    cameraData.clipDistance = 11.0f;
    cameraData.viewportWidth = 410.0f;
    cameraData.viewportHeight = 220.0f;
    cameraData.frustumWidth = 2.1f;
    cameraData.frustumHeight = 1.2f;

    if (zEffect::AnimateCameraParamsOverTime(&entry, &runtime, &event) != 2 ||
        !FloatNear(g_zEffect_FrameDeltaRemainingSec, 0.25f) ||
        !FloatNear(cameraData.nearClip, 3.0f) ||
        !FloatNear(cameraData.farClip, 120.0f) ||
        !FloatNear(cameraData.clipDistance, 12.0f) ||
        !FloatNear(cameraData.invClipDistanceSq, 1.0f / 144.0f) ||
        !FloatNear(cameraData.frustumWidth, 2.5f) ||
        !FloatNear(cameraData.frustumHeight, 1.5f) ||
        !FloatNear(cameraData.viewportWidth, 420.0f) ||
        !FloatNear(cameraData.viewportHeight, 240.0f) ||
        !FloatNear(cameraData.fovX, 2.5f / 420.0f) ||
        !FloatNear(cameraData.fovY, 1.5f / 240.0f)) {
        return 2;
    }

    event.targetNodeRefIndex = -1;
    if (zEffect::AnimateCameraParamsOverTime(&entry, &runtime, &event) != 2 ||
        zEffect::AnimateCameraParamsOverTime(nullptr, &runtime, &event) != 2 ||
        zEffect::AnimateCameraParamsOverTime(&entry, nullptr, &event) != 2 ||
        zEffect::AnimateCameraParamsOverTime(&entry, &runtime, nullptr) != 2) {
        return 3;
    }

    return 0;
}

extern "C" int zeffect_handle_rotation_event_smoke(void) {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_CameraDataPartial cameraData{};
    cameraData.posOffset = {1.0f, 2.0f, 3.0f};
    zClass_NodePartial cameraNode{};
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;

    zClass_Object3DDataPartial targetData{};
    zClass_NodePartial targetNode{};
    targetNode.classId = 5;
    targetNode.classData = &targetData;

    zClass_Object3DDataPartial basisData{};
    basisData.rotation = {10.0f, 20.0f, 30.0f};
    basisData.localMatrix[0] = 1.0f;
    basisData.localMatrix[4] = 1.0f;
    basisData.localMatrix[8] = 1.0f;
    zClass_NodePartial basisNode{};
    basisNode.classId = 5;
    basisNode.classData = &basisData;

    zEffectAnimNodeRef28 nodeRefs[2] = {};
    zEffectAnimEntry entry{};
    entry.nodeRefList = nodeRefs;

    zEffectTransformEvent event{};
    event.targetNodeRefIndex = 0;
    event.vecX = 0.5f;
    event.vecY = -1.0f;
    event.vecZ = 4.0f;
    nodeRefs[0].node = &cameraNode;

    event.flags = 1;
    if (zEffect::HandleRotationEvent(&entry, &event) != 2 ||
        !FloatNear(cameraData.posOffset.x, 1.5f) ||
        !FloatNear(cameraData.posOffset.y, 1.0f) ||
        !FloatNear(cameraData.posOffset.z, 7.0f) || cameraData.transformDirty != 1) {
        return 1;
    }

    event.flags = 0;
    event.vecX = 8.0f;
    event.vecY = 9.0f;
    event.vecZ = 10.0f;
    if (zEffect::HandleRotationEvent(&entry, &event) != 2 ||
        !FloatNear(cameraData.posOffset.x, 8.0f) ||
        !FloatNear(cameraData.posOffset.y, 9.0f) ||
        !FloatNear(cameraData.posOffset.z, 10.0f)) {
        return 2;
    }

    nodeRefs[0].node = &targetNode;
    event.flags = 1;
    event.vecX = 0.25f;
    event.vecY = 0.5f;
    event.vecZ = 0.75f;
    targetData.rotation = {1.0f, 2.0f, 3.0f};
    targetData.flags = 0x18;
    if (zEffect::HandleRotationEvent(&entry, &event) != 2 ||
        !FloatNear(targetData.rotation.x, 1.25f) ||
        !FloatNear(targetData.rotation.y, 2.5f) ||
        !FloatNear(targetData.rotation.z, 3.75f) || (targetData.flags & 0x08) != 0) {
        return 3;
    }

    event.flags = 0;
    event.vecX = 4.0f;
    event.vecY = 5.0f;
    event.vecZ = 6.0f;
    if (zEffect::HandleRotationEvent(&entry, &event) != 2 ||
        !FloatNear(targetData.rotation.x, 4.0f) ||
        !FloatNear(targetData.rotation.y, 5.0f) ||
        !FloatNear(targetData.rotation.z, 6.0f)) {
        return 4;
    }

    nodeRefs[1].node = &basisNode;
    event.flags = 2;
    event.basisNodeRefIndex = 1;
    event.vecX = 1.0f;
    event.vecY = 2.0f;
    event.vecZ = 3.0f;
    if (zEffect::HandleRotationEvent(&entry, &event) != 2 ||
        !FloatNear(targetData.rotation.x, 11.0f) ||
        !FloatNear(targetData.rotation.y, 22.0f) ||
        !FloatNear(targetData.rotation.z, 33.0f)) {
        return 5;
    }

    int matrixIdentityFlags[3] = {};
    float *matrixSlots[3] = {};
    zMath::g_currentMatrixIdentityFlagSlot = &matrixIdentityFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    event.flags = 4;
    event.vecX = 7.0f;
    event.vecY = 8.0f;
    event.vecZ = 9.0f;
    if (zEffect::HandleRotationEvent(&entry, &event) != 2 ||
        !FloatNear(targetData.rotation.x, 7.0f) ||
        !FloatNear(targetData.rotation.y, 8.0f) ||
        !FloatNear(targetData.rotation.z, 9.0f)) {
        zMath::g_currentMatrixIdentityFlagSlot = nullptr;
        zMath::g_currentMatrixPtrSlot = nullptr;
        return 6;
    }

    zMath::g_currentMatrixIdentityFlagSlot = nullptr;
    zMath::g_currentMatrixPtrSlot = nullptr;
    for (zClass_TypeListLink *link = zClass_TypeList::Head(7); link != nullptr;) {
        zClass_TypeListLink *next = link->next;
        std::free(link);
        link = next;
    }
    zClass_TypeList::Head(7) = nullptr;
    zClass_TypeList::Tail(7) = nullptr;
    return 0;
}

extern "C" int zeffect_handle_position_event_smoke(void) {
    ResetZClassRuntimeForZEffectTest();

    zClass_CameraDataPartial cameraData{};
    cameraData.targetOrEuler = {1.0f, 2.0f, 3.0f};
    zClass_NodePartial cameraNode{};
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;

    zClass_Object3DDataPartial objectData{};
    objectData.localMatrix[0] = 1.0f;
    objectData.localMatrix[4] = 1.0f;
    objectData.localMatrix[8] = 1.0f;
    objectData.localMatrix[9] = 1.0f;
    objectData.localMatrix[10] = 2.0f;
    objectData.localMatrix[11] = 3.0f;
    zClass_NodePartial objectNode{};
    objectNode.classId = 5;
    objectNode.classData = &objectData;

    zClass_Object3DDataPartial basisData{};
    basisData.localMatrix[0] = 1.0f;
    basisData.localMatrix[4] = 1.0f;
    basisData.localMatrix[8] = 1.0f;
    basisData.localMatrix[9] = 10.0f;
    basisData.localMatrix[10] = 20.0f;
    basisData.localMatrix[11] = 30.0f;
    zClass_NodePartial basisNode{};
    basisNode.classId = 5;
    basisNode.classData = &basisData;

    zEffectAnimNodeRef28 nodeRefs[2] = {};
    zEffectAnimEntry entry{};
    entry.nodeRefList = nodeRefs;

    zEffectTransformEvent event{};
    event.targetNodeRefIndex = 0;
    event.vecX = 4.0f;
    event.vecY = 5.0f;
    event.vecZ = 6.0f;
    nodeRefs[0].node = &cameraNode;

    event.flags = 0;
    if (zEffect::HandlePositionEvent(&entry, &event) != 2 ||
        !FloatNear(cameraData.targetOrEuler.x, 4.0f) ||
        !FloatNear(cameraData.targetOrEuler.y, 5.0f) ||
        !FloatNear(cameraData.targetOrEuler.z, 6.0f)) {
        return 1;
    }

    cameraData.targetOrEuler = {1.0f, 2.0f, 3.0f};
    event.flags = 1;
    if (zEffect::HandlePositionEvent(&entry, &event) != 2 ||
        !FloatNear(cameraData.targetOrEuler.x, 5.0f) ||
        !FloatNear(cameraData.targetOrEuler.y, 7.0f) ||
        !FloatNear(cameraData.targetOrEuler.z, 9.0f)) {
        return 2;
    }

    nodeRefs[0].node = &objectNode;
    event.flags = 0;
    event.vecX = 7.0f;
    event.vecY = 8.0f;
    event.vecZ = 9.0f;
    if (zEffect::HandlePositionEvent(&entry, &event) != 2 ||
        !FloatNear(objectData.localMatrix[9], 7.0f) ||
        !FloatNear(objectData.localMatrix[10], 8.0f) ||
        !FloatNear(objectData.localMatrix[11], 9.0f)) {
        return 3;
    }

    objectData.localMatrix[9] = 1.0f;
    objectData.localMatrix[10] = 2.0f;
    objectData.localMatrix[11] = 3.0f;
    objectData.flags = 0x18;
    objectNode.flags = 0;
    event.flags = 1;
    event.vecX = 2.0f;
    event.vecY = 3.0f;
    event.vecZ = 4.0f;
    if (zEffect::HandlePositionEvent(&entry, &event) != 2 ||
        !FloatNear(objectData.localMatrix[9], 3.0f) ||
        !FloatNear(objectData.localMatrix[10], 5.0f) ||
        !FloatNear(objectData.localMatrix[11], 7.0f) || (objectData.flags & 0x08) != 0 ||
        zClass_TypeList::Head(7) == nullptr || zClass_TypeList::Head(7)->node != &objectNode) {
        return 4;
    }

    for (zClass_TypeListLink *link = zClass_TypeList::Head(7); link != nullptr;) {
        zClass_TypeListLink *next = link->next;
        std::free(link);
        link = next;
    }
    zClass_TypeList::Head(7) = nullptr;
    zClass_TypeList::Tail(7) = nullptr;
    objectNode.flags = 0;

    nodeRefs[1].node = &basisNode;
    event.flags = 0;
    event.basisNodeRefIndex = 1;
    event.vecX = 1.0f;
    event.vecY = 2.0f;
    event.vecZ = 3.0f;
    if (zEffect::HandlePositionEvent(&entry, &event) != 2 ||
        !FloatNear(objectData.localMatrix[9], 11.0f) ||
        !FloatNear(objectData.localMatrix[10], 22.0f) ||
        !FloatNear(objectData.localMatrix[11], 33.0f)) {
        return 5;
    }

    nodeRefs[0].node = &cameraNode;
    entry.resetScratch[0] = 0;
    StoreFloatBits(entry.resetScratch[1], 1.0f);
    StoreFloatBits(entry.resetScratch[2], 2.0f);
    StoreFloatBits(entry.resetScratch[3], 3.0f);
    event.basisNodeRefIndex = -200;
    event.vecX = 4.0f;
    event.vecY = 5.0f;
    event.vecZ = 6.0f;
    if (zEffect::HandlePositionEvent(&entry, &event) != 2 ||
        !FloatNear(cameraData.targetOrEuler.x, 5.0f) ||
        !FloatNear(cameraData.targetOrEuler.y, 7.0f) ||
        !FloatNear(cameraData.targetOrEuler.z, 9.0f)) {
        return 6;
    }

    return 0;
}

extern "C" int zeffect_handle_activate_event_smoke(void) {
    zClass_NodePartial targetNode{};
    targetNode.classId = 5;
    targetNode.flags = 0;

    zClass_NodePartial boundNode{};
    boundNode.classId = 1;
    boundNode.flags = 0x04;

    zEffectAnimNodeRef28 nodeRefs[1] = {};
    nodeRefs[0].node = &targetNode;

    zEffectAnimEntry entry{};
    entry.nodeRefList = nodeRefs;
    entry.boundNode = &boundNode;

    zEffectActivateEvent event{};
    event.targetNodeRefIndex = 0;
    event.activeValue = 1;
    if (zEffect::HandleActivateEvent(&entry, &event) != 2 || (targetNode.flags & 0x04) == 0 ||
        (boundNode.flags & 0x04) == 0) {
        return 1;
    }

    event.targetNodeRefIndex = -100;
    event.activeValue = 0;
    if (zEffect::HandleActivateEvent(&entry, &event) != 2 || (boundNode.flags & 0x04) != 0) {
        return 2;
    }

    targetNode.flags = 0;
    boundNode.flags = 0;
    event.targetNodeRefIndex = -1;
    event.activeValue = 1;
    if (zEffect::HandleActivateEvent(&entry, &event) != 2 || (targetNode.flags & 0x04) != 0 ||
        (boundNode.flags & 0x04) != 0) {
        return 3;
    }

    return 0;
}

extern "C" int zeffect_handle_node_anim_event_smoke(void) {
    ResetZClassRuntimeForZEffectTest();

    zClass_Object3DDataPartial objectData{};
    objectData.localMatrix[0] = 1.0f;
    objectData.localMatrix[4] = 1.0f;
    objectData.localMatrix[8] = 1.0f;
    objectData.scale = {1.0f, 1.0f, 1.0f};
    zClass_NodePartial objectNode{};
    objectNode.classId = 5;
    objectNode.classData = &objectData;

    zClass_CameraDataPartial cameraData{};
    cameraData.targetOrEuler = {10.0f, 20.0f, 30.0f};
    zClass_NodePartial cameraNode{};
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;

    zEffectAnimNodeRef28 nodeRefs[1] = {};
    zEffectAnimEntry entry{};
    entry.nodeRefList = nodeRefs;
    zEffectAnimSurfaceRuntime runtime{};
    runtime.runState = 0;

    zEffectNodeAnimEvent event{};
    event.targetNodeRefIndex = 0;
    if (zEffect::HandleNodeAnimEvent(nullptr, &runtime, &event) != 1 ||
        zEffect::HandleNodeAnimEvent(&entry, nullptr, &event) != 1 ||
        zEffect::HandleNodeAnimEvent(&entry, &runtime, nullptr) != 1) {
        return 1;
    }

    event.targetNodeRefIndex = -1;
    if (zEffect::HandleNodeAnimEvent(&entry, &runtime, &event) != 1) {
        return 2;
    }

    nodeRefs[0].node = &objectNode;
    event = {};
    event.targetNodeRefIndex = 0;
    event.flags = 0x04;
    event.positionOrTargetRate.z = 2.0f;
    event.rotationOrCameraPosStart = {3.0f, 4.0f, 5.0f};
    event.rotationOrCameraPosEnd = {6.0f, 7.0f, 99.0f};
    g_zEffect_FrameDeltaRemainingSec = 0.5f;
    if (zEffect::HandleNodeAnimEvent(&entry, &runtime, &event) != 1 ||
        !FloatNear(objectData.localMatrix[9], 1.0f) ||
        !FloatNear(objectData.localMatrix[10], 1.5f) ||
        !FloatNear(objectData.localMatrix[11], 2.0f) ||
        !FloatNear(event.rotationOrCameraPosEnd.z, 4.5f) ||
        !FloatNear(event.rotationOrCameraPosRate.x, 6.0f) ||
        !FloatNear(event.rotationOrCameraPosRate.y, 7.5f) ||
        !FloatNear(g_zEffect_FrameDeltaRemainingSec, 0.0f)) {
        FreeZClassRuntimeForZEffectTest();
        return 3;
    }

    nodeRefs[0].node = &cameraNode;
    event = {};
    event.targetNodeRefIndex = 0;
    event.flags = 0x04;
    event.positionOrTargetRate.z = 2.0f;
    event.rotationOrCameraPosStart = {3.0f, 4.0f, 0.0f};
    g_zEffect_FrameDeltaRemainingSec = 0.5f;
    if (zEffect::HandleNodeAnimEvent(&entry, &runtime, &event) != 1 ||
        !FloatNear(cameraData.targetOrEuler.x, 11.0f) ||
        !FloatNear(cameraData.targetOrEuler.y, 21.5f) ||
        !FloatNear(cameraData.targetOrEuler.z, 32.0f)) {
        FreeZClassRuntimeForZEffectTest();
        return 4;
    }

    nodeRefs[0].node = &objectNode;
    objectData.rotation = {0.0f, 0.0f, 0.0f};
    event = {};
    event.targetNodeRefIndex = 0;
    event.flags = 0x20;
    event.scaleRate.z = 2.0f;
    event.endTimeSec = 3.0f;
    const float unknownRuntimeVecBZ = 4.0f;
    std::memcpy(event.unknown_90, &unknownRuntimeVecBZ, sizeof(unknownRuntimeVecBZ));
    event.runtimeVecA = {0.2f, 0.4f, 0.6f};
    g_zEffect_FrameDeltaRemainingSec = 0.5f;
    if (zEffect::HandleNodeAnimEvent(&entry, &runtime, &event) != 1 ||
        !FloatNear(objectData.rotation.x, 1.0f) ||
        !FloatNear(objectData.rotation.y, 1.5f) ||
        !FloatNear(objectData.rotation.z, 2.0f) ||
        !FloatNear(event.runtimeVecB.x, 2.1f) ||
        !FloatNear(event.runtimeVecB.y, 3.2f) ||
        !FloatNear(event.runtimeVecB.z, 4.3f)) {
        FreeZClassRuntimeForZEffectTest();
        return 5;
    }

    objectData.scale = {1.0f, 1.0f, 1.0f};
    event = {};
    event.targetNodeRefIndex = 0;
    event.flags = 0x0100;
    event.runtimeVecC = {-3.0f, 2.0f, 4.0f};
    event.runtimeVecD = {2.0f, 4.0f, 6.0f};
    g_zEffect_FrameDeltaRemainingSec = 0.5f;
    if (zEffect::HandleNodeAnimEvent(&entry, &runtime, &event) != 1 ||
        !FloatNear(objectData.scale.x, 0.001f) ||
        !FloatNear(objectData.scale.y, 2.0f) ||
        !FloatNear(objectData.scale.z, 3.0f) ||
        !FloatNear(event.runtimeVecE.x, -3.0f) ||
        !FloatNear(event.runtimeVecD.x, 3.0f) ||
        !FloatNear(event.runtimeVecD.y, 6.0f) ||
        !FloatNear(event.runtimeVecD.z, 9.0f)) {
        FreeZClassRuntimeForZEffectTest();
        return 6;
    }

    g_zEffect_FrameDeltaRemainingSec = 0.0f;
    FreeZClassRuntimeForZEffectTest();
    return 0;
}

extern "C" int zeffect_animate_node_over_time_smoke(void) {
    ResetZClassRuntimeForZEffectTest();

    zClass_Object3DDataPartial objectData{};
    objectData.localMatrix[0] = 1.0f;
    objectData.localMatrix[4] = 1.0f;
    objectData.localMatrix[8] = 1.0f;
    objectData.localMatrix[9] = 1.0f;
    objectData.localMatrix[10] = 2.0f;
    objectData.localMatrix[11] = 3.0f;
    objectData.rotation = {1.0f, 2.0f, 3.0f};
    objectData.scale = {1.0f, 1.0f, 1.0f};
    zDiPartial objectDi{};
    zClass_NodePartial objectNode{};
    objectNode.classId = 5;
    objectNode.classData = &objectData;
    objectNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&objectDi);

    zClass_CameraDataPartial cameraData{};
    cameraData.targetOrEuler = {1.0f, 2.0f, 3.0f};
    cameraData.posOffset = {4.0f, 5.0f, 6.0f};
    zClass_NodePartial cameraNode{};
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;

    zEffectAnimNodeRef28 nodeRefs[1] = {};
    zEffectAnimEntry entry{};
    entry.nodeRefList = nodeRefs;
    zEffectAnimSurfaceRuntime runtime{};
    zEffectNodeAnimEvent event{};
    event.targetNodeRefIndex = 0;

    if (zEffect::AnimateNodeOverTime(nullptr, &runtime, &event) != 2 ||
        zEffect::AnimateNodeOverTime(&entry, nullptr, &event) != 2 ||
        zEffect::AnimateNodeOverTime(&entry, &runtime, nullptr) != 2) {
        FreeZClassRuntimeForZEffectTest();
        return 1;
    }

    event.targetNodeRefIndex = -1;
    if (zEffect::AnimateNodeOverTime(&entry, &runtime, &event) != 2) {
        FreeZClassRuntimeForZEffectTest();
        return 2;
    }

    nodeRefs[0].node = &objectNode;
    runtime = {};
    runtime.runState = 0;
    runtime.eventElapsedSec = 1.0f;
    event = {};
    event.targetNodeRefIndex = 0;
    event.flags = 0x0f;
    event.nodeAlphaStart = 0.5f;
    event.nodeAlphaRate = 0.2f;
    event.positionOrTargetStart = {10.0f, 20.0f, 30.0f};
    event.positionOrTargetRate = {4.0f, 8.0f, 12.0f};
    event.rotationOrCameraPosStart = {1.0f, 2.0f, 3.0f};
    event.rotationOrCameraPosRate = {4.0f, 8.0f, 12.0f};
    event.scaleStart = {2.0f, 3.0f, 4.0f};
    event.scaleRate = {-20.0f, 4.0f, 8.0f};
    event.endTimeSec = 2.0f;
    g_zEffect_FrameDeltaRemainingSec = 0.25f;
    if (zEffect::AnimateNodeOverTime(&entry, &runtime, &event) != 1 ||
        !FloatNear(objectData.localMatrix[9], 11.0f) ||
        !FloatNear(objectData.localMatrix[10], 22.0f) ||
        !FloatNear(objectData.localMatrix[11], 33.0f) ||
        !FloatNear(objectData.rotation.x, 2.0f) ||
        !FloatNear(objectData.rotation.y, 4.0f) ||
        !FloatNear(objectData.rotation.z, 6.0f) ||
        !FloatNear(objectData.scale.x, 0.001f) ||
        !FloatNear(objectData.scale.y, 4.0f) ||
        !FloatNear(objectData.scale.z, 6.0f) ||
        !FloatNear(objectDi.blendScale, 0.55f) || (objectDi.flags & 0x08) == 0 ||
        !FloatNear(g_zEffect_FrameDeltaRemainingSec, 0.0f)) {
        FreeZClassRuntimeForZEffectTest();
        return 3;
    }

    runtime.runState = 1;
    runtime.eventElapsedSec = 3.0f;
    objectData.scale = {1.0f, 1.0f, 1.0f};
    objectData.rotation = {0.0f, 0.0f, 0.0f};
    objectData.localMatrix[9] = 1.0f;
    objectData.localMatrix[10] = 2.0f;
    objectData.localMatrix[11] = 3.0f;
    objectDi.flags = 0x08;
    objectDi.blendScale = 0.5f;
    event = {};
    event.targetNodeRefIndex = 0;
    event.flags = 0x0f;
    event.nodeAlphaEnd = 0.0f;
    event.positionOrTargetEnd = {70.0f, 80.0f, 90.0f};
    event.positionOrTargetRate = {2.0f, 4.0f, 6.0f};
    event.rotationOrCameraPosEnd = {7.0f, 8.0f, 9.0f};
    event.rotationOrCameraPosRate = {2.0f, 4.0f, 6.0f};
    event.scaleEnd = {1.5f, 1.6f, 1.7f};
    event.scaleRate = {2.0f, 4.0f, 6.0f};
    event.endTimeSec = 2.5f;
    g_zEffect_FrameDeltaRemainingSec = 1.0f;
    if (zEffect::AnimateNodeOverTime(&entry, &runtime, &event) != 2 ||
        !FloatNear(objectData.localMatrix[9], 70.0f) ||
        !FloatNear(objectData.localMatrix[10], 80.0f) ||
        !FloatNear(objectData.localMatrix[11], 90.0f) ||
        !FloatNear(objectData.rotation.x, 7.0f) ||
        !FloatNear(objectData.rotation.y, 8.0f) ||
        !FloatNear(objectData.rotation.z, 9.0f) ||
        !FloatNear(objectData.scale.x, 1.5f) ||
        !FloatNear(objectData.scale.y, 1.6f) ||
        !FloatNear(objectData.scale.z, 1.7f) ||
        !FloatNear(objectDi.blendScale, 0.0f) || (objectDi.flags & 0x08) != 0 ||
        !FloatNear(g_zEffect_FrameDeltaRemainingSec, 0.5f)) {
        FreeZClassRuntimeForZEffectTest();
        return 4;
    }

    nodeRefs[0].node = &cameraNode;
    runtime = {};
    runtime.runState = 0;
    runtime.eventElapsedSec = 0.5f;
    event = {};
    event.targetNodeRefIndex = 0;
    event.flags = 0x03;
    event.positionOrTargetStart = {10.0f, 20.0f, 30.0f};
    event.positionOrTargetRate = {0.4f, 0.8f, 1.2f};
    event.rotationOrCameraPosStart = {1.0f, 2.0f, 3.0f};
    event.rotationOrCameraPosRate = {4.0f, 8.0f, 12.0f};
    event.endTimeSec = 1.0f;
    g_zEffect_FrameDeltaRemainingSec = 0.25f;
    if (zEffect::AnimateNodeOverTime(&entry, &runtime, &event) != 1 ||
        !FloatNear(cameraData.targetOrEuler.x, 10.1f) ||
        !FloatNear(cameraData.targetOrEuler.y, 20.2f) ||
        !FloatNear(cameraData.targetOrEuler.z, 30.3f) ||
        !FloatNear(cameraData.posOffset.x, 2.0f) ||
        !FloatNear(cameraData.posOffset.y, 4.0f) ||
        !FloatNear(cameraData.posOffset.z, 6.0f) ||
        !FloatNear(g_zEffect_FrameDeltaRemainingSec, 0.0f)) {
        FreeZClassRuntimeForZEffectTest();
        return 5;
    }

    g_zEffect_FrameDeltaRemainingSec = 0.0f;
    FreeZClassRuntimeForZEffectTest();
    return 0;
}

extern "C" int zeffect_handle_node_scale_event_smoke(void) {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_Object3DDataPartial data{};
    data.flags = 0x18;
    zClass_NodePartial node{};
    node.classId = 5;
    node.classData = &data;

    zEffectAnimNodeRef28 nodeRefs[1] = {};
    nodeRefs[0].node = &node;

    zEffectAnimEntry entry{};
    entry.nodeRefList = nodeRefs;

    zEffectNodeScaleEvent event{};
    event.targetNodeRefIndex = 0;
    event.scaleX = 2.0f;
    event.scaleY = 3.0f;
    event.scaleZ = 4.0f;

    if (zEffect::HandleNodeScaleEvent(&entry, &event) != 2 ||
        !FloatNear(data.scale.x, 2.0f) || !FloatNear(data.scale.y, 3.0f) ||
        !FloatNear(data.scale.z, 4.0f) || (data.flags & 0x18) != 0 ||
        (data.flags & 0x21) != 0x21 || (node.flags & 0x02000003) != 0x02000003 ||
        zClass_TypeList::Head(7) == nullptr || zClass_TypeList::Head(7)->node != &node) {
        return 1;
    }

    for (zClass_TypeListLink *link = zClass_TypeList::Head(7); link != nullptr;) {
        zClass_TypeListLink *next = link->next;
        std::free(link);
        link = next;
    }
    zClass_TypeList::Head(7) = nullptr;
    zClass_TypeList::Tail(7) = nullptr;
    return 0;
}

extern "C" int zeffect_anim_keyframe_sample_smoke(void) {
    struct KeyframeBlock {
        zEffectKeyframeEvent event;
        zEffectKeyframeSampleHeader sample;
        zEffectKeyframeSampleChannel channels[3];
    };

    KeyframeBlock block = {};
    zEffectAnimSurfaceRuntime runtime = {};
    runtime.currentEvent = &block.event.header;
    runtime.eventElapsedSec = 0.5f;

    block.event.header.recordSize = sizeof(block);
    block.event.currentKeyframeOffset = sizeof(zEffectKeyframeEvent);
    block.event.keyframeLocalTime = 12.0f;
    block.event.lookaheadAdvanceCount = 2;
    block.sample.channelFlags = 7;

    if (zEffect_Anim::AdvanceKeyframeSample(&runtime, &block.event, &block.sample) != 0 ||
        block.event.currentKeyframeOffset != sizeof(block) ||
        block.event.keyframeLocalTime != 0.0f || block.event.lookaheadAdvanceCount != 3) {
        return 1;
    }

    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    KeyframeBlock runBlock = {};
    zClass_Object3DDataPartial data = {};
    zClass_NodePartial node = {};
    node.classData = &data;
    node.classId = 5;

    zEffectAnimSurfaceRuntime runRuntime = {};
    runRuntime.currentEvent = &runBlock.event.header;
    runRuntime.eventElapsedSec = 0.5f;

    runBlock.event.header.recordSize = sizeof(runBlock);
    runBlock.event.targetNodeRefIndex = 0;
    runBlock.event.currentKeyframeOffset = sizeof(zEffectKeyframeEvent);
    runBlock.sample.channelFlags = 7;
    runBlock.sample.startTimeSec = 0.0f;
    runBlock.sample.endTimeSec = 1.0f;

    runBlock.channels[0].baseQuat = {1.0f, 2.0f, 3.0f, 0.0f};
    runBlock.channels[0].rate = {10.0f, 20.0f, 30.0f};
    runBlock.channels[1].baseQuat = {1.0f, 0.0f, 0.0f, 0.0f};
    runBlock.channels[1].rate = {0.0f, 0.0f, 0.0f};
    runBlock.channels[2].baseQuat = {2.0f, 3.0f, 4.0f, 0.0f};
    runBlock.channels[2].rate = {2.0f, 4.0f, 6.0f};

    float deltaTime = 0.0f;
    const float consumed = zEffect_Anim::AnimateKeyframeSample(
        &runRuntime, &runBlock.event, &node, &runBlock.sample, &deltaTime);
    if (!FloatNear(consumed, 0.5f) || !FloatNear(deltaTime, 0.5f) ||
        !FloatNear(runBlock.event.keyframeLocalTime, 0.5f) ||
        !FloatNear(data.localMatrix[9], 6.0f) || !FloatNear(data.localMatrix[10], 12.0f) ||
        !FloatNear(data.localMatrix[11], 18.0f) || !FloatNear(data.rotation.x, 0.0f) ||
        !FloatNear(data.rotation.y, 0.0f) || !FloatNear(data.rotation.z, 0.0f) ||
        !FloatNear(data.scale.x, 3.0f) || !FloatNear(data.scale.y, 5.0f) ||
        !FloatNear(data.scale.z, 7.0f)) {
        return 2;
    }

    zEffectAnimEntry entry = {};
    zEffectAnimNodeRef28 nodeRefs[1] = {};
    nodeRefs[0].node = &node;
    entry.nodeRefList = nodeRefs;
    runBlock.event.currentKeyframeOffset = sizeof(zEffectKeyframeEvent);
    runBlock.event.keyframeLocalTime = 0.0f;
    runBlock.event.lookaheadAdvanceCount = 0;
    runRuntime.runState = 0;
    g_zEffect_FrameDeltaRemainingSec = 0.5f;
    if (zEffect_Anim::AdvanceKeyframe(&entry, &runRuntime, &runBlock.event) != 1 ||
        !FloatNear(g_zEffect_FrameDeltaRemainingSec, 0.0f)) {
        return 3;
    }

    zEffectEvaluateKeyframeEvent evaluateEvent = {};
    evaluateEvent.targetNodeRefIndex = 0;
    evaluateEvent.litFlag = 1;
    evaluateEvent.hasAlphaScale = 1;
    evaluateEvent.alphaScale = 0.25f;
    data.flags = 0;
    data.alphaScale = 0.0f;
    if (zEffect_Anim::EvaluateKeyframe(&entry, &evaluateEvent) != 2 ||
        (data.flags & 0x02) == 0 || !FloatNear(data.alphaScale, 0.25f)) {
        return 4;
    }

    zEffectRunKeyframeEvent runEvent = {};
    runEvent.targetNodeRefIndex = 0;
    runEvent.startLitFlag = 1;
    runEvent.endLitFlag = 0;
    runEvent.startAlphaScale = 0.2f;
    runEvent.endAlphaScale = 0.8f;
    runEvent.alphaScaleRate = 0.5f;
    runEvent.endTimeSec = 1.0f;
    runRuntime.runState = 0;
    runRuntime.eventElapsedSec = 0.25f;
    g_zEffect_FrameDeltaRemainingSec = 0.25f;
    data.flags = 0;
    data.alphaScale = 0.0f;
    if (zEffect_Anim::RunKeyframes(&entry, &runRuntime, &runEvent) != 1 ||
        (data.flags & 0x02) == 0 || !FloatNear(data.alphaScale, 0.325f) ||
        !FloatNear(g_zEffect_FrameDeltaRemainingSec, 0.0f)) {
        return 5;
    }

    runRuntime.runState = 1;
    runRuntime.eventElapsedSec = 1.5f;
    g_zEffect_FrameDeltaRemainingSec = 0.75f;
    data.flags = 0x02;
    data.alphaScale = 0.5f;
    if (zEffect_Anim::RunKeyframes(&entry, &runRuntime, &runEvent) != 2 ||
        (data.flags & 0x02) != 0 || !FloatNear(data.alphaScale, 0.8f) ||
        !FloatNear(g_zEffect_FrameDeltaRemainingSec, 0.5f)) {
        return 6;
    }

    for (zClass_TypeListLink *link = zClass_TypeList::Head(7); link != nullptr;) {
        zClass_TypeListLink *next = link->next;
        std::free(link);
        link = next;
    }
    zClass_TypeList::Head(7) = nullptr;
    zClass_TypeList::Tail(7) = nullptr;
    return 0;
}

extern "C" int zeffect_parent_attach_detach_events_smoke(void) {
    ResetZClassRuntimeForZEffectTest();

    zClass_NodePartial parentNode = {};
    zClass_NodePartial childNode = {};
    zClass_NodePartial beamNode = {};
    zClass_Object3DDataPartial beamData = {};
    zClass_NodePartial attachNode = {};
    zDiPartial attachDi = {};
    zDiEntryPartial attachEntry = {};
    zModel_MaterialPartial attachMaterial = {};
    zModel_MaterialCyclePartial attachCycle = {};
    zImage_TexDirEntryPartial frameA = {};
    zImage_TexDirEntryPartial frameB = {};
    zImage_TexDirEntryPartial *frames[2] = {&frameA, &frameB};

    parentNode.classId = 3;
    childNode.classId = 3;
    beamNode.classId = 5;
    beamNode.classData = &beamData;
    beamData.scale = {1.0f, 1.5f, 1.0f};

    attachDi.entries = &attachEntry;
    attachEntry.material = &attachMaterial;
    attachMaterial.cycle = &attachCycle;
    attachMaterial.currentTextureDirectoryEntry = &frameB;
    attachCycle.currentFrame = 1.0f;
    attachCycle.frameCount = 2;
    attachCycle.frameTable = frames;
    attachNode.userDataOrDiRef =
        static_cast<unsigned int>(reinterpret_cast<std::uintptr_t>(&attachDi));

    zEffectAnimNodeRef28 nodeRefs[4] = {};
    nodeRefs[1].node = &parentNode;
    nodeRefs[2].node = &childNode;
    nodeRefs[3].node = &beamNode;

    zEffectAnimEntry entry = {};
    entry.nodeRefList = nodeRefs;

    zEffectParentChildEvent parentEvent = {};
    parentEvent.parentNodeRefIndex = 1;
    parentEvent.childNodeRefIndex = 2;
    if (zEffect::HandleAddChildEvent(&entry, &parentEvent) != 2 ||
        parentNode.listCountB != 1 || parentNode.listB[0] != &childNode ||
        childNode.listCountA != 1 || childNode.listA[0] != &parentNode) {
        FreeZClassRuntimeForZEffectTest();
        std::free(parentNode.listB);
        std::free(childNode.listA);
        return 1;
    }

    if (zEffect::HandleAddChildEvent(&entry, &parentEvent) != 2 ||
        parentNode.listCountB != 1 || childNode.listCountA != 1) {
        FreeZClassRuntimeForZEffectTest();
        std::free(parentNode.listB);
        std::free(childNode.listA);
        return 2;
    }

    if (zEffect::HandleRemoveChildEvent(&entry, &parentEvent) != 2 ||
        parentNode.listCountB != 0 || childNode.listCountA != 0) {
        FreeZClassRuntimeForZEffectTest();
        std::free(parentNode.listB);
        std::free(childNode.listA);
        return 3;
    }

    zEffectAnimNodeRef28 attachRefs[1] = {};
    attachRefs[0].node = &attachNode;
    entry.nodeRefList = attachRefs;

    zEffectAnimSurfaceRuntime runtime = {};
    zEffectAttachEvent attachEvent = {};
    attachEvent.flags = 1;
    attachEvent.targetNodeRefIndex = 0;
    attachEvent.variantIndex = 0;
    if (zEffect::HandleAttachEvent(&entry, &runtime, &attachEvent) != 2 ||
        attachCycle.currentFrame != 0.0f || attachMaterial.currentTextureDirectoryEntry != &frameA) {
        FreeZClassRuntimeForZEffectTest();
        std::free(parentNode.listB);
        std::free(childNode.listA);
        return 4;
    }

    const zVec3 src{0.0f, 0.0f, 0.0f};
    const zVec3 dest{0.0f, 0.0f, -2.0f};
    const float beamLength = zEffect::UpdateBeamNodeBetweenPoints(&beamNode, &src, &dest);
    if (!FloatNear(beamLength, 2.0f) || !FloatNear(beamData.localMatrix[9], 0.0f) ||
        !FloatNear(beamData.localMatrix[10], 0.0f) ||
        !FloatNear(beamData.localMatrix[11], 0.0f) || !FloatNear(beamData.rotation.x, 0.0f) ||
        !FloatNear(beamData.rotation.y, 0.0f) || !FloatNear(beamData.rotation.z, 0.0f) ||
        !FloatNear(beamData.scale.x, 1.0f) || !FloatNear(beamData.scale.y, 1.5f) ||
        !FloatNear(beamData.scale.z, 2.0f)) {
        FreeZClassRuntimeForZEffectTest();
        std::free(parentNode.listB);
        std::free(childNode.listA);
        return 5;
    }

    const zVec3 fracDest{0.0f, 0.0f, -4.0f};
    beamData.scale = {2.0f, 2.5f, 1.0f};
    const float fracLength =
        zEffect::UpdateBeamNodeBetweenFractions(&beamNode, &src, 0.25f, &fracDest, 0.75f);
    if (!FloatNear(fracLength, 2.0f) || !FloatNear(beamData.localMatrix[9], 0.0f) ||
        !FloatNear(beamData.localMatrix[10], 0.0f) ||
        !FloatNear(beamData.localMatrix[11], -1.0f) || !FloatNear(beamData.scale.x, 2.0f) ||
        !FloatNear(beamData.scale.y, 2.5f) || !FloatNear(beamData.scale.z, 2.0f)) {
        FreeZClassRuntimeForZEffectTest();
        std::free(parentNode.listB);
        std::free(childNode.listA);
        return 6;
    }

    zEffectAnimNodeRef28 detachRefs[4] = {};
    detachRefs[3].node = &beamNode;
    entry.nodeRefList = detachRefs;
    runtime.runState = 1;
    runtime.eventElapsedSec = 0.25f;
    g_zEffect_FrameDeltaRemainingSec = 0.25f;

    zEffectBeamDetachEvent detachEvent = {};
    detachEvent.flags = 0x0108;
    detachEvent.beamNodeRefIndex = 3;
    detachEvent.pointA = {0.0f, 0.0f, 0.0f};
    detachEvent.pointB = {0.0f, 0.0f, -2.0f};
    detachEvent.endTimeSec = 1.0f;
    if (zEffect::HandleDetachEvent(&entry, &runtime, &detachEvent) != 1 ||
        !FloatNear(g_zEffect_FrameDeltaRemainingSec, 0.0f) ||
        !FloatNear(beamData.localMatrix[11], 0.0f) || !FloatNear(beamData.scale.z, 2.0f)) {
        FreeZClassRuntimeForZEffectTest();
        std::free(parentNode.listB);
        std::free(childNode.listA);
        return 7;
    }

    FreeZClassRuntimeForZEffectTest();
    std::free(parentNode.listB);
    std::free(childNode.listA);
    return 0;
}

extern "C" int zeffect_handle_screen_fx_events_smoke(void) {
    const zVideo_SurfaceStatePartial oldSwSurface = g_zVideo_SwSurfaceState;
    const float oldScaleX = g_zMath_ProjScaleX;
    const float oldScaleY = g_zMath_ProjScaleY;
    const float oldFrameDelta = g_zEffect_FrameDeltaRemainingSec;
    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;
    std::uint8_t savedConfig[kEffectFxPass3ConfigSize] = {};
    std::memcpy(savedConfig, EffectFxPass3ConfigBytes(), sizeof(savedConfig));

    std::memset(EffectFxPass3ConfigBytes(), 0, kEffectFxPass3ConfigSize);
    EffectFxPass3FieldAt<const HudUiCommon_FTable *>(kEffectFxPass3RootElementOffset) =
        &g_HudUiCommon_FTable;
    g_HudUi_InvalidateMask = 0;
    g_zVideo_PixelPack_RMaskShifted = 0xf8;
    g_zVideo_PixelPack_GMaskShifted = 0xfc;
    g_zVideo_PixelPack_RShift = 8;
    g_zVideo_PixelPack_GShift = 3;
    g_zVideo_PixelPack_BShiftTo8 = 3;

    zEffectAnimEntry entry = {};
    zEffectAnimSurfaceRuntime runtime = {};
    zEffectScreenColorFxEvent colorEvent = {};
    runtime.eventElapsedSec = 0.5f;
    colorEvent.redBase = 0.75f;
    colorEvent.redSlope = 1.0f;
    colorEvent.greenBase = 0.25f;
    colorEvent.greenSlope = -1.0f;
    colorEvent.alphaBase = 0.25f;
    colorEvent.alphaSlope = 0.5f;
    colorEvent.blueBase = 0.25f;
    colorEvent.blueSlope = 0.5f;
    colorEvent.endTimeSec = 1.0f;
    g_zEffect_FrameDeltaRemainingSec = 0.25f;

    if (zEffect::HandleScreenColorFxEvent(&entry, &runtime, &colorEvent) != 1 ||
        !FloatNear(g_zEffect_FrameDeltaRemainingSec, 0.0f) ||
        EffectFxPass3FieldAt<unsigned short>(kEffectFxPass3RootPackedColorOffset) != 0xf810u ||
        !FloatNear(static_cast<float>(
                       EffectFxPass3FieldAt<double>(kEffectFxPass3RootAlphaOffset)),
                   0.5f)) {
        std::memcpy(EffectFxPass3ConfigBytes(), savedConfig, sizeof(savedConfig));
        g_zVideo_SwSurfaceState = oldSwSurface;
        g_zMath_ProjScaleX = oldScaleX;
        g_zMath_ProjScaleY = oldScaleY;
        g_zEffect_FrameDeltaRemainingSec = oldFrameDelta;
        g_HudUi_InvalidateMask = oldInvalidateMask;
        return 1;
    }

    colorEvent.redEnd = 0.0f;
    colorEvent.greenEnd = 1.0f;
    colorEvent.alphaEnd = 1.0f;
    colorEvent.blueEnd = 0.0f;
    runtime.eventElapsedSec = 1.25f;
    g_zEffect_FrameDeltaRemainingSec = 0.5f;
    if (zEffect::HandleScreenColorFxEvent(&entry, &runtime, &colorEvent) != 2 ||
        !FloatNear(g_zEffect_FrameDeltaRemainingSec, 0.25f) ||
        EffectFxPass3FieldAt<unsigned short>(kEffectFxPass3RootPackedColorOffset) != 0x07e0u ||
        !FloatNear(static_cast<float>(
                       EffectFxPass3FieldAt<double>(kEffectFxPass3RootAlphaOffset)),
                   1.0f)) {
        std::memcpy(EffectFxPass3ConfigBytes(), savedConfig, sizeof(savedConfig));
        g_zVideo_SwSurfaceState = oldSwSurface;
        g_zMath_ProjScaleX = oldScaleX;
        g_zMath_ProjScaleY = oldScaleY;
        g_zEffect_FrameDeltaRemainingSec = oldFrameDelta;
        g_HudUi_InvalidateMask = oldInvalidateMask;
        return 2;
    }

    std::memset(EffectFxPass3ConfigBytes(), 0, kEffectFxPass3ConfigSize);
    EffectFxPass3FieldAt<const HudUiCommon_FTable *>(kEffectFxPass3RootElementOffset) =
        &g_HudUiCommon_FTable;
    HudUiElement *const slot0 =
        reinterpret_cast<HudUiElement *>(EffectFxPass3ConfigBytes() + kEffectFxPass3SlotsOffset);
    HudUiElement *const slot1 = reinterpret_cast<HudUiElement *>(
        EffectFxPass3ConfigBytes() + kEffectFxPass3SlotsOffset + kEffectFxPass3SlotSize);
    slot0->ftable = &g_HudUiCommon_FTable;
    slot1->ftable = &g_HudUiCommon_FTable;

    zClass_NodePartial callbackNode = {};
    entry.callbackNode = &callbackNode;
    g_zEffect_ConditionalRefPosX = -8.0f;
    g_zEffect_ConditionalRefPosY = 0.0f;
    g_zEffect_ConditionalRefPosZ = 0.0f;
    g_zMath_ProjScaleX = 80.0f;
    g_zMath_ProjScaleY = -60.0f;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.width = 320;
    g_zVideo_SwSurfaceState.height = 240;

    zEffectScreenOverlayFxEvent overlayEvent = {};
    overlayEvent.flagsAndAnchorNodePacked = 0x09;
    overlayEvent.centerXBase = 100.0f;
    overlayEvent.centerXSlope = 20.0f;
    overlayEvent.centerYBase = 200.0f;
    overlayEvent.centerYSlope = -40.0f;
    overlayEvent.maxRadiusNearWorld = 4.0f;
    overlayEvent.maxRadiusFarWorld = 8.0f;
    overlayEvent.extentBase = 10.0f;
    overlayEvent.extentSlope = 8.0f;
    overlayEvent.sinFreqBase = 5.0f;
    overlayEvent.sinFreqSlope = 2.0f;
    overlayEvent.sinPhaseBase = 9.0f;
    overlayEvent.sinPhaseSlope = 4.0f;
    overlayEvent.endTimeSec = 2.0f;
    runtime.runState = 0;
    runtime.eventElapsedSec = 0.5f;
    g_zEffect_FrameDeltaRemainingSec = 0.5f;

    if (zEffect::HandleScreenOverlayFxEvent(&entry, &runtime, &overlayEvent) != 1 ||
        !FloatNear(g_zEffect_FrameDeltaRemainingSec, 0.0f) ||
        !FloatNear(overlayEvent.maxRadiusNearPixels, 40.0f) ||
        !FloatNear(overlayEvent.maxRadiusFarPixels, 80.0f) ||
        !FloatNear(overlayEvent.maxRadiusPixelsSlope, 20.0f) ||
        EffectFxPass3FieldAt<int>(kEffectFxPass3SlotWriteIndexOffset) != 1 ||
        slot0->x != 110 || slot0->y != 180 ||
        !FloatNear(EffectFxPass3FieldAt<float>(kEffectFxPass3SlotsOffset +
                                               kEffectFxPass3SlotMaxRadiusOffset),
                   50.0f) ||
        !FloatNear(EffectFxPass3FieldAt<float>(kEffectFxPass3SlotsOffset +
                                               kEffectFxPass3SlotExtentOffset),
                   14.0f) ||
        !FloatNear(EffectFxPass3FieldAt<float>(kEffectFxPass3SlotsOffset +
                                               kEffectFxPass3SlotSinFreqOffset),
                   6.0f) ||
        !FloatNear(EffectFxPass3FieldAt<float>(kEffectFxPass3SlotsOffset +
                                               kEffectFxPass3SlotSinPhaseOffset),
                   11.0f)) {
        std::memcpy(EffectFxPass3ConfigBytes(), savedConfig, sizeof(savedConfig));
        g_zVideo_SwSurfaceState = oldSwSurface;
        g_zMath_ProjScaleX = oldScaleX;
        g_zMath_ProjScaleY = oldScaleY;
        g_zEffect_FrameDeltaRemainingSec = oldFrameDelta;
        g_HudUi_InvalidateMask = oldInvalidateMask;
        return 3;
    }

    overlayEvent.centerXEnd = 150.0f;
    overlayEvent.centerYEnd = 175.0f;
    overlayEvent.extentEnd = 44.0f;
    overlayEvent.sinFreqEnd = 7.0f;
    overlayEvent.sinPhaseEnd = 13.0f;
    runtime.runState = 1;
    runtime.eventElapsedSec = 2.25f;
    g_zEffect_FrameDeltaRemainingSec = 0.5f;
    const std::size_t slot1Offset = kEffectFxPass3SlotsOffset + kEffectFxPass3SlotSize;
    if (zEffect::HandleScreenOverlayFxEvent(&entry, &runtime, &overlayEvent) != 2 ||
        !FloatNear(g_zEffect_FrameDeltaRemainingSec, 0.25f) ||
        EffectFxPass3FieldAt<int>(kEffectFxPass3SlotWriteIndexOffset) != 2 ||
        slot1->x != 150 || slot1->y != 175 ||
        !FloatNear(EffectFxPass3FieldAt<float>(slot1Offset + kEffectFxPass3SlotMaxRadiusOffset),
                   80.0f) ||
        !FloatNear(EffectFxPass3FieldAt<float>(slot1Offset + kEffectFxPass3SlotExtentOffset),
                   44.0f) ||
        !FloatNear(EffectFxPass3FieldAt<float>(slot1Offset + kEffectFxPass3SlotSinFreqOffset),
                   7.0f) ||
        !FloatNear(EffectFxPass3FieldAt<float>(slot1Offset + kEffectFxPass3SlotSinPhaseOffset),
                   13.0f)) {
        std::memcpy(EffectFxPass3ConfigBytes(), savedConfig, sizeof(savedConfig));
        g_zVideo_SwSurfaceState = oldSwSurface;
        g_zMath_ProjScaleX = oldScaleX;
        g_zMath_ProjScaleY = oldScaleY;
        g_zEffect_FrameDeltaRemainingSec = oldFrameDelta;
        g_HudUi_InvalidateMask = oldInvalidateMask;
        return 4;
    }

    std::memcpy(EffectFxPass3ConfigBytes(), savedConfig, sizeof(savedConfig));
    g_zVideo_SwSurfaceState = oldSwSurface;
    g_zMath_ProjScaleX = oldScaleX;
    g_zMath_ProjScaleY = oldScaleY;
    g_zEffect_FrameDeltaRemainingSec = oldFrameDelta;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    return 0;
}

extern "C" int zeffect_anim_activation_prereqs_smoke(void) {
    zEffectAnimEntry entry = {};
    entry.activationPrereqCount = 7;
    zEffectAnim::ResetActivationPrereqCount(&entry);
    if (entry.activationPrereqCount != 0 || zEffectAnim::CheckActivationPrereqs(&entry) != 1) {
        return 1;
    }

    zEffectAnimActivationPrereq prereqs[1] = {};
    zEffectAnimEntry candidates[1] = {};
    std::strcpy(candidates[0].name, "gate");
    candidates[0].activationState = 0;
    g_zEffectAnim_EntryList = candidates;
    g_zEffectAnim_EntryCount = 1;

    entry.activationPrereqList = prereqs;
    entry.activationPrereqCount = 1;
    entry.activationPrereqMinimumMatchCount = 1;
    prereqs[0].requireMatch = 1;
    prereqs[0].mode = 1;
    std::strcpy(prereqs[0].targetName, "gate");

    if (zEffectAnim::CheckActivationPrereqs(&entry) != 1 ||
        prereqs[0].targetEntry != &candidates[0]) {
        g_zEffectAnim_EntryList = nullptr;
        g_zEffectAnim_EntryCount = 0;
        return 2;
    }

    candidates[0].activationState = 1;
    prereqs[0].targetEntry = nullptr;
    if (zEffectAnim::CheckActivationPrereqs(&entry) != 0) {
        g_zEffectAnim_EntryList = nullptr;
        g_zEffectAnim_EntryCount = 0;
        return 3;
    }

    zClass_NodePartial node = {};
    node.flags = 4;
    std::memset(prereqs, 0, sizeof(prereqs));
    prereqs[0].requireMatch = 1;
    prereqs[0].mode = 2;
    prereqs[0].targetNode = &node;
    const std::int32_t expectedFlag = 1;
    std::memcpy(prereqs[0].targetName, &expectedFlag, sizeof(expectedFlag));

    if (zEffectAnim::CheckActivationPrereqs(&entry) != 1) {
        g_zEffectAnim_EntryList = nullptr;
        g_zEffectAnim_EntryCount = 0;
        return 4;
    }

    node.flags = 0;
    const bool mismatchOk = zEffectAnim::CheckActivationPrereqs(&entry) == 0;
    g_zEffectAnim_EntryList = nullptr;
    g_zEffectAnim_EntryCount = 0;
    return mismatchOk ? 0 : 5;
}

extern "C" int hud_sensor_run_start_anims_from_zrd_smoke(void) {
    zArchiveList *const oldMountedList = g_zArchive_MountedList;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    zEffectAnimEntry *const oldEntryList = g_zEffectAnim_EntryList;
    const short oldEntryCount = g_zEffectAnim_EntryCount;
    const int oldQueueEnabled = g_zEffectAnim_RecordQueueEnabled;
    const int oldDispatchEnabled = g_zEffectAnim_DispatchEnabled;
    const unsigned int oldDispatchTag = g_zEffectAnim_ActivationDispatchTagHigh;
    const auto oldDispatchCallback = g_zEffectAnim_ActivationDispatchCallback;

    char tempDir[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "hsa", 0, tempFile) == 0) {
        return 1;
    }

    HANDLE const file =
        CreateFileA(tempFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                    CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        DeleteFileA(tempFile);
        return 2;
    }

    WriteEffectTestZrdArray(file, 3);
    WriteEffectTestZrdString(file, "START_ANIMS");
    WriteEffectTestZrdArray(file, 4);
    WriteEffectTestZrdArray(file, 2);
    WriteEffectTestZrdString(file, "introflash");
    WriteEffectTestZrdArray(file, 2);
    WriteEffectTestZrdString(file, "missing_anim");
    WriteEffectTestZrdArray(file, 2);
    WriteEffectTestZrdString(file, "startcountdown");
    FlushFileBuffers(file);

    zZarFileRecord record = {};
    record.fileOffset = 0;
    record.fileSize = SetFilePointer(file, 0, nullptr, FILE_CURRENT);
    std::strcpy(record.name, "start.zrd");
    zIndexArchive archive = {};
    archive.hFile = file;
    archive.recordCount = 1;
    archive.records = &record;
    zArchiveListNode archiveNode = {};
    archiveNode.payload = &archive;
    archiveNode.next = &archiveNode;
    archiveNode.prev = &archiveNode;
    zArchiveList mountedList = {};
    mountedList.count = 1;
    mountedList.head = &archiveNode;

    zClass_NodePartial boundNodes[2] = {};
    boundNodes[0].classId = 2;
    boundNodes[1].classId = 2;
    zClass_NodePartial runtimeNodes[2] = {};
    runtimeNodes[0].callbackPriority = 1;
    runtimeNodes[1].callbackPriority = 2;
    zEffectAnimEntry entries[2] = {};
    std::strcpy(entries[0].name, "introflash");
    std::strcpy(entries[1].name, "startcountdown");
    entries[0].boundNode = &boundNodes[0];
    entries[1].boundNode = &boundNodes[1];
    entries[0].callbackNode = &boundNodes[0];
    entries[1].callbackNode = &boundNodes[1];
    entries[0].runtimeNode = &runtimeNodes[0];
    entries[1].runtimeNode = &runtimeNodes[1];
    entries[0].activationPrereqCount = 5;
    entries[1].activationPrereqCount = 6;
    entries[0].velocityX = 11.0f;
    entries[0].velocityY = 12.0f;
    entries[0].velocityZ = 13.0f;
    entries[1].velocityX = 21.0f;
    entries[1].velocityY = 22.0f;
    entries[1].velocityZ = 23.0f;

    int networkEnabled = 1;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_zArchive_MountedList = &mountedList;
    g_zEffectAnim_EntryList = entries;
    g_zEffectAnim_EntryCount = 2;
    zEffect_Anim::ClearActivationRecords();
    g_zEffectAnim_RecordQueueEnabled = 1;
    g_zEffectAnim_DispatchEnabled = 0;
    g_zEffectAnim_ActivationDispatchCallback = nullptr;
    g_zEffectAnim_ActivationDispatchTagHigh = 0;

    HudSensorTracker tracker = {};
    tracker.RunStartAnimsFromZrd("C:\\dummy\\start.zrd", "START_ANIMS");
    int networkSkipResult = 0;
    if (entries[0].activationPrereqCount != 5 || entries[1].activationPrereqCount != 6) {
        networkSkipResult = 31;
    } else if (entries[0].activationState != 0 || entries[1].activationState != 0) {
        networkSkipResult = 32;
    } else if (g_zEffectAnim_ActivationRecordCount != 0) {
        networkSkipResult = 33;
    }

    networkEnabled = 0;
    tracker.RunStartAnimsFromZrd("C:\\dummy\\start.zrd", "START_ANIMS");
    int loadResult = 0;
    if (entries[0].activationPrereqCount != 0 || entries[1].activationPrereqCount != 0) {
        loadResult = 4;
    } else if (entries[0].activationState != 2) {
        loadResult = 51;
    } else if (entries[1].activationState != 2) {
        loadResult = 52;
    } else if (entries[0].velocityX != 0.0f || entries[0].velocityY != 0.0f ||
               entries[0].velocityZ != 0.0f || entries[1].velocityX != 0.0f ||
               entries[1].velocityY != 0.0f || entries[1].velocityZ != 0.0f) {
        loadResult = 6;
    } else if (runtimeNodes[0].callbackContext !=
                   reinterpret_cast<zClass_NodePartial *>(&entries[0]) ||
               runtimeNodes[1].callbackContext !=
                   reinterpret_cast<zClass_NodePartial *>(&entries[1])) {
        loadResult = 7;
    } else if (g_zEffectAnim_ActivationRecordCount != 2 ||
               g_zEffectAnim_ActivationRecordTable[0].commandType != 2 ||
               g_zEffectAnim_ActivationRecordTable[1].commandType != 2) {
        loadResult = 8;
    }

    zEffect_Anim::ClearActivationRecords();
    g_zEffectAnim_RecordQueueEnabled = oldQueueEnabled;
    g_zEffectAnim_DispatchEnabled = oldDispatchEnabled;
    g_zEffectAnim_ActivationDispatchCallback = oldDispatchCallback;
    g_zEffectAnim_ActivationDispatchTagHigh = oldDispatchTag;
    g_zEffectAnim_EntryList = oldEntryList;
    g_zEffectAnim_EntryCount = oldEntryCount;
    g_zArchive_MountedList = oldMountedList;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    CloseHandle(file);
    DeleteFileA(tempFile);
    if (networkSkipResult != 0) {
        return networkSkipResult;
    }
    return loadResult;
}

extern "C" int zeffect_anim_activation_record_queue_smoke(void) {
    zEffect_Anim::ClearActivationRecords();
    g_zEffectAnim_ActivationRecordCapacity = 0;
    g_zEffectAnim_ActivationDispatchTagHigh = 0x00ffffff;

    zEffectAnimActivationRecord *record = zEffect_Anim::AllocActivationRecord();
    if (record == nullptr || record != g_zEffectAnim_ActivationRecordTable ||
        g_zEffectAnim_ActivationRecordCapacity != 1000 ||
        g_zEffectAnim_ActivationRecordCount != 1 || zEffect_Anim::GetActivationRecordCount() != 1 ||
        zEffect_Anim::GetActivationRecordAt(0) != record || record->recordId != 0) {
        zEffect_Anim::ClearActivationRecords();
        return 1;
    }

    zEffect_Anim::DiscardLastActivationRecord();
    if (g_zEffectAnim_ActivationRecordCount != 0) {
        zEffect_Anim::ClearActivationRecords();
        return 2;
    }

    zEffect_Anim::ClearActivationRecords();
    g_zEffectAnim_ActivationRecordCapacity = 1;
    g_zEffectAnim_ActivationRecordCount = 1;
    g_zEffectAnim_ActivationRecordTable = static_cast<zEffectAnimActivationRecord *>(
        std::calloc(1, sizeof(zEffectAnimActivationRecord)));
    if (g_zEffectAnim_ActivationRecordTable == nullptr) {
        g_zEffectAnim_ActivationRecordCount = 0;
        return 3;
    }
    g_zEffectAnim_ActivationRecordTable[0].commandType = 77;

    record = zEffect_Anim::AllocActivationRecord();
    if (record != &g_zEffectAnim_ActivationRecordTable[1] ||
        g_zEffectAnim_ActivationRecordCapacity != 2 || g_zEffectAnim_ActivationRecordCount != 2 ||
        g_zEffectAnim_ActivationRecordTable[0].commandType != 77 || record->recordId != 1) {
        zEffect_Anim::ClearActivationRecords();
        return 3;
    }

    zEffect_Anim::ClearActivationRecords();
    g_zEffectAnim_ActivationRecordCapacity = 0;
    g_zEffectAnim_ActivationRecordCount = 0;
    g_zEffectAnim_RecordQueueEnabled = 0;
    g_zEffectAnim_DispatchEnabled = 0;
    g_zEffectAnim_ActivationDispatchCallback = nullptr;

    zEffectAnimEntry entry = {};
    std::strcpy(entry.name, "startcountdown");
    if (zEffectAnim::QueueCmdType2Velocity(&entry, nullptr, 1.0f, 2.0f, 3.0f) != nullptr ||
        g_zEffectAnim_ActivationRecordCount != 0) {
        zEffect_Anim::ClearActivationRecords();
        return 4;
    }

    g_zEffectAnim_RecordQueueEnabled = 1;
    if (zEffectAnim::QueueCmdType2Velocity(&entry, nullptr, 1.0f, 2.0f, 3.0f) != nullptr ||
        g_zEffectAnim_ActivationRecordCount != 1 ||
        g_zEffectAnim_ActivationRecordTable[0].commandType != 2 ||
        zEffect_Anim::GetActivationRecordPackedSize(&g_zEffectAnim_ActivationRecordTable[0]) !=
            0x48 ||
        std::strcmp(g_zEffectAnim_ActivationRecordTable[0].animName, "startcountdown") != 0 ||
        g_zEffectAnim_ActivationRecordTable[0].nodeToken != -1 ||
        g_zEffectAnim_ActivationRecordTable[0].params[0].f32 != 1.0f ||
        g_zEffectAnim_ActivationRecordTable[0].params[1].f32 != 2.0f ||
        g_zEffectAnim_ActivationRecordTable[0].params[2].f32 != 3.0f) {
        zEffect_Anim::ClearActivationRecords();
        return 5;
    }

    if (zEffectAnim::QueueCmdType1TransformRotVelocity(&entry, nullptr, 1.0f, 2.0f, 3.0f, 4.0f,
                                                       5.0f, 6.0f, 7.0f, 8.0f, 9.0f) != nullptr ||
        g_zEffectAnim_ActivationRecordCount != 2 ||
        g_zEffectAnim_ActivationRecordTable[1].commandType != 1 ||
        g_zEffectAnim_ActivationRecordTable[1].params[8].f32 != 9.0f) {
        zEffect_Anim::ClearActivationRecords();
        return 6;
    }

    const zVec3 refVec{10.0f, 11.0f, 12.0f};
    const zVec3 velocityVec{13.0f, 14.0f, 15.0f};
    record = zEffectAnim::QueueCmdType3PositionRefAndVelocity(&entry, nullptr, nullptr, &refVec,
                                                              &velocityVec);
    if (record != &g_zEffectAnim_ActivationRecordTable[2] ||
        g_zEffectAnim_ActivationRecordCount != 3 || record->commandType != 3 ||
        record->params[0].i32 != -1 || record->params[1].f32 != 10.0f ||
        record->params[6].f32 != 15.0f) {
        zEffect_Anim::ClearActivationRecords();
        return 7;
    }

    if (zEffectAnim::QueueCmdType4TransformRefs(&entry, nullptr, nullptr, nullptr, nullptr,
                                                nullptr) != nullptr ||
        g_zEffectAnim_ActivationRecordCount != 4 ||
        g_zEffectAnim_ActivationRecordTable[3].commandType != 4 ||
        g_zEffectAnim_ActivationRecordTable[3].params[0].i32 != -1 ||
        g_zEffectAnim_ActivationRecordTable[3].params[1].u32 != 0 ||
        g_zEffectAnim_ActivationRecordTable[3].params[4].i32 != -1 ||
        g_zEffectAnim_ActivationRecordTable[3].params[5].u32 != 0) {
        zEffect_Anim::ClearActivationRecords();
        return 8;
    }

    zEffectAnimActivationRecord queryRecord = {};
    queryRecord.nodeToken = -1;
    std::strcpy(queryRecord.animName, "startcountdown");
    if (zEffect_Anim::HasActivationRecord(&queryRecord) != 1) {
        zEffect_Anim::ClearActivationRecords();
        return 9;
    }

    queryRecord.nodeToken = 12;
    if (zEffect_Anim::HasActivationRecord(&queryRecord) != 0) {
        zEffect_Anim::ClearActivationRecords();
        return 10;
    }

    zEffect_Anim::SetActivationDispatchContext(EffectAnimDispatchCallback, 0x7f);
    if (g_zEffectAnim_ActivationDispatchCallback != EffectAnimDispatchCallback ||
        g_zEffectAnim_ActivationDispatchTagHigh != 0x7f000000u) {
        zEffect_Anim::ClearActivationRecords();
        return 11;
    }

    zEffectAnimActivationRecord sizeRecord = {};
    sizeRecord.commandType = 1;
    const bool packedSizeOk = zEffect_Anim::GetActivationRecordPackedSize(&sizeRecord) == 0x38;
    sizeRecord.commandType = 3;
    const bool packedSizeOk2 = zEffect_Anim::GetActivationRecordPackedSize(&sizeRecord) == 0x4c;
    sizeRecord.commandType = 4;
    const bool packedSizeOk3 = zEffect_Anim::GetActivationRecordPackedSize(&sizeRecord) == 0x50;
    sizeRecord.commandType = 99;
    if (!packedSizeOk || !packedSizeOk2 || !packedSizeOk3 ||
        zEffect_Anim::GetActivationRecordPackedSize(&sizeRecord) != 0x50) {
        zEffect_Anim::ClearActivationRecords();
        return 12;
    }

    g_zEffectAnim_RecordQueueEnabled = 0;
    g_zEffectAnim_DispatchEnabled = 1;
    g_effectAnimDispatchCallCount = 0;
    g_effectAnimDispatchRecord = nullptr;
    g_zEffectAnim_ActivationDispatchCallback = EffectAnimDispatchCallback;
    record = zEffectAnim::QueueCmdType2Velocity(&entry, nullptr, 4.0f, 5.0f, 6.0f);
    if (record == nullptr || g_effectAnimDispatchCallCount != 1 ||
        g_effectAnimDispatchRecord != record || g_zEffectAnim_ActivationRecordCount != 4 ||
        record->params[2].f32 != 6.0f) {
        zEffect_Anim::ClearActivationRecords();
        return 13;
    }

    entry.flags = 0x00001000u | 0x00002000u | 0x00000400u | 0x00000800u;
    g_zEffectAnim_RecordQueueEnabled = 0;
    g_zEffectAnim_DispatchEnabled = 0;
    g_effectAnimDispatchCallCount = 0;
    record = zEffectAnim::QueueCmdType2Velocity(&entry, nullptr, 7.0f, 8.0f, 9.0f);
    const bool overrideOk = record != nullptr && g_effectAnimDispatchCallCount == 1 &&
                            g_zEffectAnim_ActivationRecordCount == 5 &&
                            record == &g_zEffectAnim_ActivationRecordTable[4] &&
                            record->params[0].f32 == 7.0f;

    g_zEffectAnim_ActivationDispatchCallback = nullptr;
    g_zEffectAnim_RecordQueueEnabled = 0;
    g_zEffectAnim_DispatchEnabled = 0;
    g_zEffectAnim_ActivationDispatchTagHigh = 0;
    zEffect_Anim::ClearActivationRecords();
    return overrideOk ? 0 : 14;
}

extern "C" int zeffect_anim_process_activation_record_smoke(void) {
    zEffectAnimActivationRecord record = {};
    record.commandType = 2;
    std::strcpy(record.animName, "missing_activation_record_entry");

    return zEffect_Anim::ProcessActivationRecord(&record) == nullptr ? 0 : 1;
}

extern "C" int zeffect_handle_emitter_reset_event_smoke(void) {
    if (zEffect::HandleEmitterResetEvent(nullptr) != -1) {
        return 1;
    }

    int eventStreamSentinel = 0;
    int staleCurrentEvent = 0;
    zEffectAnimSurfaceRuntime runtime = {};
    runtime.runState = 0xaa;
    runtime.resetMode = 0x17;
    runtime.sequenceElapsedSec = 12.0f;
    runtime.eventElapsedSec = 13.0f;
    runtime.loopElapsedSec = 14.0f;
    runtime.loopIterationCount = 15;
    runtime.currentEvent = &staleCurrentEvent;
    runtime.eventStream = &eventStreamSentinel;

    if (zEffect::HandleEmitterResetEvent(&runtime) != 0) {
        return 2;
    }

    return runtime.runState == 0x17 && runtime.sequenceElapsedSec == 0.0f &&
                   runtime.eventElapsedSec == 0.0f && runtime.loopElapsedSec == 14.0f &&
                   runtime.loopIterationCount == 15 && runtime.currentEvent == &eventStreamSentinel
               ? 0
               : 3;
}

extern "C" int zeffect_handle_emitter_loop_event_smoke(void) {
    int eventStreamSentinel = 0;
    int staleCurrentEvent = 0;
    zEffectAnimEntry entry = {};
    zEffectAnimSurfaceRuntime runtime = {};
    zEffectAnimLoopEvent loopEvent = {};

    entry.triggerCurrentValue = 99999.0f;
    runtime.sequenceElapsedSec = 0.25f;
    runtime.loopElapsedSec = 1.0f;
    runtime.loopIterationCount = 2;
    loopEvent.stopModeFlags = 1;
    loopEvent.stopValue.u16 = 3;
    if (zEffect::HandleEmitterLoopEvent(&entry, &runtime, &loopEvent) != 2 ||
        runtime.runState != 2 || runtime.loopIterationCount != 3 ||
        runtime.loopElapsedSec != 1.25f || entry.triggerCurrentValue != 99999.0f) {
        return 1;
    }

    runtime = {};
    loopEvent = {};
    runtime.sequenceElapsedSec = 0.75f;
    runtime.loopElapsedSec = 0.75f;
    loopEvent.stopModeFlags = 2;
    loopEvent.stopValue.f32 = 1.5f;
    if (zEffect::HandleEmitterLoopEvent(&entry, &runtime, &loopEvent) != 2 ||
        runtime.runState != 2 || runtime.loopElapsedSec != 1.5f) {
        return 2;
    }

    entry.triggerCurrentValue = 90000.0f;
    runtime = {};
    loopEvent = {};
    runtime.runState = 1;
    runtime.resetMode = 5;
    runtime.sequenceElapsedSec = 2.0f;
    runtime.eventElapsedSec = 9.0f;
    runtime.loopElapsedSec = 3.0f;
    runtime.loopIterationCount = 8;
    runtime.currentEvent = &staleCurrentEvent;
    runtime.eventStream = &eventStreamSentinel;
    loopEvent.stopModeFlags = 3;
    loopEvent.stopValue.u16 = 0xffffu;
    if (zEffect::HandleEmitterLoopEvent(&entry, &runtime, &loopEvent) != 0) {
        return 3;
    }

    return runtime.runState == 5 && runtime.sequenceElapsedSec == 0.0f &&
                   runtime.eventElapsedSec == 0.0f && runtime.loopElapsedSec == 5.0f &&
                   runtime.loopIterationCount == 9 &&
                   runtime.currentEvent == &eventStreamSentinel &&
                   entry.triggerCurrentValue == 86400.0f
               ? 0
               : 4;
}

extern "C" int zeffect_handle_emitter_stop_event_smoke(void) {
    g_zEffectAnim_EntryCount = 3;
    g_zEffectAnim_EntryList =
        static_cast<zEffectAnimEntry *>(std::calloc(3, sizeof(zEffectAnimEntry)));
    if (g_zEffectAnim_EntryList == nullptr) {
        return 1;
    }

    std::strcpy(g_zEffectAnim_EntryList[0].name, "first");
    std::strcpy(g_zEffectAnim_EntryList[1].name, "target");
    std::strcpy(g_zEffectAnim_EntryList[2].name, "frozen");
    g_zEffectAnim_EntryList[0].activationState = 2;
    g_zEffectAnim_EntryList[1].activationState = 2;
    g_zEffectAnim_EntryList[2].activationState = 5;

    zEffectAnimEmitterEvent event = {};
    std::strcpy(event.animName, "target");
    if (zEffect::HandleEmitterStopEvent(nullptr, &event) != 2 || event.cachedEntryIndex != 1 ||
        g_zEffectAnim_EntryList[1].activationState != 6) {
        std::free(g_zEffectAnim_EntryList);
        g_zEffectAnim_EntryList = nullptr;
        g_zEffectAnim_EntryCount = 0;
        return 2;
    }

    event.cachedEntryIndex = 1;
    g_zEffectAnim_EntryList[1].activationState = 3;
    zEffect::HandleEmitterStopEvent(nullptr, &event);
    const bool runningStopOk = g_zEffectAnim_EntryList[1].activationState == 4;

    event.cachedEntryIndex = 2;
    zEffect::HandleEmitterStopEvent(nullptr, &event);
    const bool frozenOk = g_zEffectAnim_EntryList[2].activationState == 5;

    event = {};
    std::strcpy(event.animName, "first");
    zEffect::HandleEmitterStopEvent(nullptr, &event);
    const bool zeroIndexOk =
        event.cachedEntryIndex == 0 && g_zEffectAnim_EntryList[0].activationState == 2;

    std::free(g_zEffectAnim_EntryList);
    g_zEffectAnim_EntryList = nullptr;
    g_zEffectAnim_EntryCount = 0;
    return runningStopOk && frozenOk && zeroIndexOk ? 0 : 3;
}

extern "C" int zeffect_surface_control_events_smoke(void) {
    zEffectAnimSurfaceRuntime runtimes[2] = {};
    std::strcpy(runtimes[0].sequenceName, "idle");
    std::strcpy(runtimes[1].sequenceName, "burst");
    runtimes[0].runState = 3;
    runtimes[1].runState = 3;

    zEffectAnimEntry entry = {};
    entry.runtimeList = runtimes;
    entry.runtimeSequenceCount = 2;

    zEffectSurfaceControlEvent event = {};
    event.surfaceSlotIndex = -1;
    std::strcpy(event.sequenceName, "burst");
    if (zEffect::HandleSurfaceStopEvent(&entry, &event) != 2 || event.surfaceSlotIndex != 1 ||
        runtimes[0].runState != 3 || runtimes[1].runState != 0) {
        return 1;
    }

    event.surfaceSlotIndex = 0;
    if (zEffect::HandleSurfacePlayEvent(&entry, &event) != 2 || runtimes[0].runState != 2) {
        return 2;
    }

    event.surfaceSlotIndex = -1;
    std::strcpy(event.sequenceName, "missing");
    runtimes[0].runState = 0;
    runtimes[1].runState = 0;
    if (zEffect::HandleSurfacePlayEvent(&entry, &event) != 2 || event.surfaceSlotIndex != -1 ||
        runtimes[0].runState != 0 || runtimes[1].runState != 0) {
        return 3;
    }

    return 0;
}

extern "C" int zeffect_conditional_ref_pos_smoke(void) {
    g_zEffect_ConditionalRefPosEnabled = 0;
    zVec3 refPosition{1.0f, 2.0f, 3.0f};
    zEffect::SetConditionalRefPos(&refPosition);
    if (g_zEffect_ConditionalRefPosEnabled != 1 || g_zEffect_ConditionalRefPosX != 1.0f ||
        g_zEffect_ConditionalRefPosY != 2.0f || g_zEffect_ConditionalRefPosZ != 3.0f) {
        return 1;
    }

    if (zEffect::GetConditionalRefPosDistanceSq(nullptr) != 0.0f) {
        return 2;
    }

    zClass_Object3DDataPartial data = {};
    zClass_NodePartial node = {};
    node.classId = 5;
    node.flags = 0x00080000;
    node.classData = &data;
    data.cachedWorldMatrix[9] = 4.0f;
    data.cachedWorldMatrix[10] = 6.0f;
    data.cachedWorldMatrix[11] = 8.0f;

    return zEffect::GetConditionalRefPosDistanceSq(&node) == 50.0f ? 0 : 3;
}

extern "C" int zeffect_set_variant_override_packed_ids_if_complete_smoke(void) {
    const int oldEnabled = g_zEffect_VariantOverrideEnabled;
    const unsigned int oldPackedIds = g_zEffect_VariantOverridePackedIds;

    zTag4Partial complete = {};
    complete.count = 2;
    complete.tags[0] = 7;
    complete.tags[1] = 8;
    complete.tags[2] = 0xff;
    unsigned int expectedPacked = 0;
    std::memcpy(&expectedPacked, &complete, sizeof(expectedPacked));

    g_zEffect_VariantOverrideEnabled = 0;
    g_zEffect_VariantOverridePackedIds = 0;
    zEffect::SetVariantOverridePackedIdsIfComplete(&complete);
    const bool completeOk = g_zEffect_VariantOverrideEnabled == 1 &&
                            g_zEffect_VariantOverridePackedIds == expectedPacked;

    zTag4Partial empty = {};
    empty.count = 0;
    empty.tags[0] = 1;
    g_zEffect_VariantOverrideEnabled = 3;
    g_zEffect_VariantOverridePackedIds = 0x12345678;
    zEffect::SetVariantOverridePackedIdsIfComplete(&empty);
    const bool emptyOk = g_zEffect_VariantOverrideEnabled == 3 &&
                         g_zEffect_VariantOverridePackedIds == 0x12345678;

    zTag4Partial incomplete = {};
    incomplete.count = 2;
    incomplete.tags[0] = 9;
    incomplete.tags[1] = 0xff;
    g_zEffect_VariantOverrideEnabled = 4;
    g_zEffect_VariantOverridePackedIds = 0x87654321;
    zEffect::SetVariantOverridePackedIdsIfComplete(&incomplete);
    const bool incompleteOk = g_zEffect_VariantOverrideEnabled == 4 &&
                              g_zEffect_VariantOverridePackedIds == 0x87654321;

    g_zEffect_VariantOverrideEnabled = oldEnabled;
    g_zEffect_VariantOverridePackedIds = oldPackedIds;

    return completeOk && emptyOk && incompleteOk ? 0 : 1;
}

extern "C" int zeffect_skip_conditional_chain_smoke(void) {
    alignas(4) unsigned char events[24] = {};
    auto *event0 = reinterpret_cast<zEffectAnimEventHeader *>(&events[0]);
    auto *event1 = reinterpret_cast<zEffectAnimEventHeader *>(&events[8]);
    auto *event2 = reinterpret_cast<zEffectAnimEventHeader *>(&events[16]);
    event0->typeAndStartMode = 0x20;
    event0->byteSize = 8;
    event1->typeAndStartMode = 0x21;
    event1->byteSize = 8;
    event2->typeAndStartMode = 0x22;
    event2->byteSize = 8;

    zEffectAnimSurfaceRuntime runtime = {};
    runtime.currentEvent = event0;
    runtime.eventStream = events;
    runtime.eventStreamSize = sizeof(events);

    return zEffect::SkipConditionalChainToEnd(nullptr, &runtime, event0) == 2 &&
                   runtime.currentEvent == event2
               ? 0
               : 1;
}

extern "C" int zeffect_trace_upward_hit_smoke(void) {
    zClass_NodePartial *const oldWorld = g_zEffect_World;
    const int oldBreakOnFirst = g_cls_di_BreakOnFirstCandidate;
    const int oldStopAfterFirst = g_cls_di_StopAfterFirstHit;

    zClass_WorldDataPartial worldData = {};
    zClass_NodePartial world = {};
    world.classData = &worldData;
    g_zEffect_World = &world;

    zClass_Object3DDataPartial nodeData = {};
    nodeData.cachedWorldMatrix[9] = 1.0f;
    nodeData.cachedWorldMatrix[10] = 2.0f;
    nodeData.cachedWorldMatrix[11] = 3.0f;

    zClass_NodePartial node = {};
    node.flags = 0x10;
    node.classId = 5;
    node.classData = &nodeData;

    int hit = -1;
    const float rayHeight = 12.0f;
    const int nodeResult = zEffect::TraceUpwardHitFromNodeOrPos(&node, nullptr, &rayHeight, &hit);
    if (nodeResult != 1 || hit != 0 || (node.flags & 0x10) == 0 ||
        g_cls_di_BreakOnFirstCandidate != 0 || g_cls_di_StopAfterFirstHit != 0) {
        g_zEffect_World = oldWorld;
        g_cls_di_BreakOnFirstCandidate = oldBreakOnFirst;
        g_cls_di_StopAfterFirstHit = oldStopAfterFirst;
        return 1;
    }

    zVec3 position = {4.0f, 5.0f, 6.0f};
    hit = -1;
    const int posResult = zEffect::TraceUpwardHitFromNodeOrPos(nullptr, &position, nullptr, &hit);
    if (posResult != 1 || hit != 0 || g_DiPickQueryPoint.x != 4.0f ||
        g_DiPickQueryPoint.y != 5.0f || g_DiPickQueryPoint.z != 6.0f ||
        g_DiSegmentEnd.y != 55.0f || g_cls_di_BreakOnFirstCandidate != 0 ||
        g_cls_di_StopAfterFirstHit != 0) {
        g_zEffect_World = oldWorld;
        g_cls_di_BreakOnFirstCandidate = oldBreakOnFirst;
        g_cls_di_StopAfterFirstHit = oldStopAfterFirst;
        return 2;
    }

    hit = 99;
    if (zEffect::TraceUpwardHitFromNodeOrPos(nullptr, nullptr, &rayHeight, &hit) != 1 ||
        hit != 99) {
        g_zEffect_World = oldWorld;
        g_cls_di_BreakOnFirstCandidate = oldBreakOnFirst;
        g_cls_di_StopAfterFirstHit = oldStopAfterFirst;
        return 3;
    }

    g_zEffect_World = oldWorld;
    g_cls_di_BreakOnFirstCandidate = oldBreakOnFirst;
    g_cls_di_StopAfterFirstHit = oldStopAfterFirst;
    return 0;
}

extern "C" int zeffect_handle_conditional_chain_event_smoke(void) {
    const int oldEffectLevel = g_zEffect_ConditionalEffectLevel;
    const int oldRandIndex = g_zEffect_RandTableIndex;
    const float oldRandValue = g_zEffect_RandUnitTable[5];

    zEffectConditionalEvent events[3] = {};
    events[0].header.typeAndStartMode = 0x20;
    events[0].header.byteSize = sizeof(zEffectConditionalEvent);
    events[0].conditionMask = 0x04;
    events[0].conditionThreshold.i32 = 2;
    events[1].header.typeAndStartMode = 0x21;
    events[1].header.byteSize = sizeof(zEffectConditionalEvent);
    events[1].conditionMask = 0x01;
    events[1].conditionThreshold.f32 = 1.0f;
    events[2].header.typeAndStartMode = 0x22;
    events[2].header.byteSize = sizeof(zEffectConditionalEvent);

    zEffectAnimEntry entry = {};
    zEffectAnimSurfaceRuntime runtime = {};
    runtime.eventStream = events;
    runtime.eventStreamSize = sizeof(events);
    runtime.currentEvent = &events[0];

    g_zEffect_ConditionalEffectLevel = 3;
    if (zEffect::HandleConditionalChainEvent(&entry, &runtime, &events[0]) != 2 ||
        runtime.currentEvent != &events[0]) {
        g_zEffect_ConditionalEffectLevel = oldEffectLevel;
        g_zEffect_RandTableIndex = oldRandIndex;
        g_zEffect_RandUnitTable[5] = oldRandValue;
        return 1;
    }

    runtime.currentEvent = &events[0];
    g_zEffect_ConditionalEffectLevel = 1;
    g_zEffect_RandTableIndex = 5;
    g_zEffect_RandUnitTable[5] = 0.25f;
    if (zEffect::HandleConditionalChainEvent(&entry, &runtime, &events[0]) != 2 ||
        runtime.currentEvent != &events[1] || g_zEffect_RandTableIndex != 6) {
        g_zEffect_ConditionalEffectLevel = oldEffectLevel;
        g_zEffect_RandTableIndex = oldRandIndex;
        g_zEffect_RandUnitTable[5] = oldRandValue;
        return 2;
    }

    runtime.currentEvent = &events[0];
    events[1].conditionMask = 0x04;
    events[1].conditionThreshold.i32 = 2;
    if (zEffect::HandleConditionalChainEvent(&entry, &runtime, &events[0]) != 2 ||
        runtime.currentEvent != &events[2]) {
        g_zEffect_ConditionalEffectLevel = oldEffectLevel;
        g_zEffect_RandTableIndex = oldRandIndex;
        g_zEffect_RandUnitTable[5] = oldRandValue;
        return 3;
    }

    g_zEffect_ConditionalEffectLevel = oldEffectLevel;
    g_zEffect_RandTableIndex = oldRandIndex;
    g_zEffect_RandUnitTable[5] = oldRandValue;
    return 0;
}

extern "C" int zeffect_marker_and_callback_event_smoke(void) {
    zEffectAnimEntry entry = {};
    zEffectAnimSurfaceRuntime runtime = {};
    zEffectAnimCallbackEvent event = {};
    int context = 0;

    if (zEffect::HandleNoOpMarkerEvent(&entry, &runtime, &event) != 2) {
        return 1;
    }

    g_effectAnimEventCallbackCallCount = 0;
    if (zEffect::HandleCallbackEvent(&entry, &runtime, &event) != 2 ||
        g_effectAnimEventCallbackCallCount != 0) {
        return 2;
    }

    entry.eventCallback = EffectAnimEventCallback;
    entry.eventCallbackContext = &context;
    event.value = 77;
    if (zEffect::HandleCallbackEvent(&entry, &runtime, &event) != 2) {
        return 3;
    }

    return g_effectAnimEventCallbackCallCount == 1 && g_effectAnimEventCallbackSelf == &entry &&
                   g_effectAnimEventCallbackContext == &context &&
                   g_effectAnimEventCallbackValue == 77
               ? 0
               : 4;
}

extern "C" int zeffect_handle_top_message_event_smoke(void) {
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    zEffectAnimTextIdEntry *const oldTextEntries = g_zEffectAnim_TextIdEntryList;
    const int oldTextEntryCount = g_zEffectAnim_TextIdEntryCount;

    HudUiTopMessageStack top{};
    top.Constructor();
    g_HudUiTopMessageStack = &top;

    zEffectAnimTextIdEntry textEntries[1] = {};
    std::strcpy(textEntries[0].messageKey, "fallback");
    textEntries[0].messageId = 0;
    g_zEffectAnim_TextIdEntryList = textEntries;
    g_zEffectAnim_TextIdEntryCount = 1;

    zEffectTopMessageEvent event = {};
    event.textIdIndex = -1;
    const int negativeResult = zEffect::HandleTopMessageEvent(nullptr, &event);
    HudUiPanel *const line0 = EffectTextStackLineAt(&top, 0);
    const bool negativeSkipped =
        negativeResult == 2 && (EffectTestFieldAt<std::uint32_t>(line0, 0x0c) & 0x10u) != 0;

    event.textIdIndex = 0;
    const int fallbackResult = zEffect::HandleTopMessageEvent(nullptr, &event);
    const bool fallbackPushed = fallbackResult == 2 &&
                                std::strcmp(line0->GetLastTextPtr(), "fallback") == 0 &&
                                EffectTestFieldAt<float>(line0, 0x10) == 3.0f &&
                                (EffectTestFieldAt<std::uint32_t>(line0, 0x0c) & 0x10u) == 0;

    EffectDeleteTextStackLineFonts(&top);
    g_HudUiTopMessageStack = oldTopStack;
    g_zEffectAnim_TextIdEntryList = oldTextEntries;
    g_zEffectAnim_TextIdEntryCount = oldTextEntryCount;

    return negativeSkipped && fallbackPushed ? 0 : 1;
}

extern "C" int zeffect_anim_set_on_state_done_callback_smoke(void) {
    zEffectAnimEntry entry = {};
    void *context = &entry;

    zEffectAnimEntry::SetOnStateDoneCallback(
        nullptr, reinterpret_cast<void *>(&EffectAnimEventCallback), &context);
    zEffectAnimEntry::SetOnStateDoneCallback(
        &entry, reinterpret_cast<void *>(&EffectAnimEventCallback), &context);

    return entry.eventCallback == EffectAnimEventCallback && entry.eventCallbackContext == &context
               ? 0
               : 1;
}

extern "C" int zeffect_world_and_zbd_setters_smoke(void) {
    zClass_NodePartial *const oldWorld = g_zEffect_World;
    zClass_NodePartial *const oldResourceNode = g_zEffect_ResourceNode;

    zClass_NodePartial world = {};
    zEffect::SetWorldNode(&world);
    if (g_zEffect_World != &world) {
        g_zEffect_World = oldWorld;
        g_zEffect_ResourceNode = oldResourceNode;
        return 1;
    }

    zClass_NodePartial resource = {};
    zEffect::SetResourceNode(&resource);
    if (g_zEffect_ResourceNode != &resource) {
        g_zEffect_World = oldWorld;
        g_zEffect_ResourceNode = oldResourceNode;
        return 2;
    }

    std::strcpy(g_zEffectAnim_ZbdFilename, "old.zbd");
    zEffect_Anim::SetZbdFilename("anim/new.zbd");
    if (std::strcmp(g_zEffectAnim_ZbdFilename, "anim/new.zbd") != 0) {
        g_zEffect_World = oldWorld;
        g_zEffect_ResourceNode = oldResourceNode;
        return 3;
    }

    char tooLong[0x90] = {};
    std::memset(tooLong, 'x', 0x81);
    tooLong[0x81] = '\0';
    zEffect_Anim::SetZbdFilename(tooLong);
    const bool longNameSkipped = std::strcmp(g_zEffectAnim_ZbdFilename, "anim/new.zbd") == 0;

    g_zEffect_World = oldWorld;
    g_zEffect_ResourceNode = oldResourceNode;
    g_zEffectAnim_ZbdFilename[0] = '\0';
    return longNameSkipped ? 0 : 4;
}

extern "C" int zeffect_anim_load_zbd_minimal_smoke(void) {
    zClass_NodePartial *const oldWorld = g_zEffect_World;
    zClass_NodePartial *const oldResourceNode = g_zEffect_ResourceNode;
    const int oldEntriesInstantiated = g_zEffectAnim_EntriesInstantiated;
    void *const oldHeapPtr = g_zEffectAnim_HeapPtr;
    const short oldCountsPackedLoWord = g_zEffectAnim_CountsPackedLoWord;
    const short oldEntryCount = g_zEffectAnim_EntryCount;
    zEffectAnimEntry *const oldEntryList = g_zEffectAnim_EntryList;
    const int oldTextIdEntryCount = g_zEffectAnim_TextIdEntryCount;
    zEffectAnimTextIdEntry *const oldTextIdEntryList = g_zEffectAnim_TextIdEntryList;
    const int oldSourceFileStampCount = g_zEffectAnim_SourceFileStampCount;
    zEffectAnimSourceFileStamp *const oldSourceFileStampList = g_zEffectAnim_SourceFileStampList;
    const float oldDefaultGravity = g_zEffect_DefaultGravity;
    const int oldConditionalRefPosEnabled = g_zEffect_ConditionalRefPosEnabled;
    const int oldVariantOverrideEnabled = g_zEffect_VariantOverrideEnabled;
    const float oldConditionalRefPosX = g_zEffect_ConditionalRefPosX;
    const float oldConditionalRefPosY = g_zEffect_ConditionalRefPosY;
    const float oldConditionalRefPosZ = g_zEffect_ConditionalRefPosZ;
    const unsigned int oldVariantOverridePackedIds = g_zEffect_VariantOverridePackedIds;
    const float oldFrameDeltaRemainingSec = g_zEffect_FrameDeltaRemainingSec;
    char oldFilename[sizeof(g_zEffectAnim_ZbdFilename)] = {};
    std::memcpy(oldFilename, g_zEffectAnim_ZbdFilename, sizeof(oldFilename));

    char tempPath[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempPath), tempPath) == 0 ||
        GetTempFileNameA(tempPath, "zld", 0, tempFile) == 0) {
        return 1;
    }

    zClass_NodePartial world = {};
    zClass_NodePartial resource = {};
    EffectAnimLoadZbdHeaderBlock header = {};
    header.entriesInstantiated = 3;
    header.heapPtr = &resource;
    header.countsPackedLoWord = 5;
    header.entryCount = 0;
    header.textIdEntryCount = 0;
    header.worldNode = &resource;
    header.defaultGravity = 9.75f;
    header.conditionalRefPosEnabled = 1;
    header.variantOverrideEnabled = 1;
    header.conditionalRefPosX = 1.25f;
    header.conditionalRefPosY = 2.5f;
    header.conditionalRefPosZ = 3.75f;
    header.variantOverridePackedIds = 0x00120034;
    header.frameDeltaRemainingSec = 0.5f;

    if (!WriteEffectAnimLoadZbdMinimalFile(tempFile, header)) {
        DeleteFileA(tempFile);
        return 2;
    }

    g_zEffect_World = &world;
    g_zEffect_ResourceNode = &resource;
    zEffect_Anim::SetZbdFilename(tempFile);
    const int result = zEffect_Anim::LoadZbd();
    zEffectAnimEntry *const loadedEntryList = g_zEffectAnim_EntryList;

    const bool globalsLoaded =
        result == 0 && g_zEffect_World == &world &&
        g_zEffectAnim_EntriesInstantiated == header.entriesInstantiated &&
        g_zEffectAnim_HeapPtr == header.heapPtr &&
        g_zEffectAnim_CountsPackedLoWord == header.countsPackedLoWord &&
        g_zEffectAnim_EntryCount == 0 && g_zEffectAnim_TextIdEntryCount == 0 &&
        g_zEffectAnim_TextIdEntryList == nullptr && g_zEffectAnim_SourceFileStampCount == 0 &&
        g_zEffectAnim_SourceFileStampList == nullptr &&
        g_zEffect_DefaultGravity == header.defaultGravity &&
        g_zEffect_ConditionalRefPosEnabled == header.conditionalRefPosEnabled &&
        g_zEffect_VariantOverrideEnabled == header.variantOverrideEnabled &&
        g_zEffect_ConditionalRefPosX == header.conditionalRefPosX &&
        g_zEffect_ConditionalRefPosY == header.conditionalRefPosY &&
        g_zEffect_ConditionalRefPosZ == header.conditionalRefPosZ &&
        g_zEffect_VariantOverridePackedIds == header.variantOverridePackedIds &&
        g_zEffect_FrameDeltaRemainingSec == header.frameDeltaRemainingSec;

    if (loadedEntryList != nullptr && loadedEntryList != oldEntryList) {
        std::free(loadedEntryList);
    }
    g_zEffect_World = oldWorld;
    g_zEffect_ResourceNode = oldResourceNode;
    g_zEffectAnim_EntriesInstantiated = oldEntriesInstantiated;
    g_zEffectAnim_HeapPtr = oldHeapPtr;
    g_zEffectAnim_CountsPackedLoWord = oldCountsPackedLoWord;
    g_zEffectAnim_EntryCount = oldEntryCount;
    g_zEffectAnim_EntryList = oldEntryList;
    g_zEffectAnim_TextIdEntryCount = oldTextIdEntryCount;
    g_zEffectAnim_TextIdEntryList = oldTextIdEntryList;
    g_zEffectAnim_SourceFileStampCount = oldSourceFileStampCount;
    g_zEffectAnim_SourceFileStampList = oldSourceFileStampList;
    g_zEffect_DefaultGravity = oldDefaultGravity;
    g_zEffect_ConditionalRefPosEnabled = oldConditionalRefPosEnabled;
    g_zEffect_VariantOverrideEnabled = oldVariantOverrideEnabled;
    g_zEffect_ConditionalRefPosX = oldConditionalRefPosX;
    g_zEffect_ConditionalRefPosY = oldConditionalRefPosY;
    g_zEffect_ConditionalRefPosZ = oldConditionalRefPosZ;
    g_zEffect_VariantOverridePackedIds = oldVariantOverridePackedIds;
    g_zEffect_FrameDeltaRemainingSec = oldFrameDeltaRemainingSec;
    std::memcpy(g_zEffectAnim_ZbdFilename, oldFilename, sizeof(oldFilename));
    DeleteFileA(tempFile);

    return globalsLoaded ? 0 : 3;
}

extern "C" int zeffect_anim_load_and_instantiate_minimal_smoke(void) {
    zClass_NodePartial *const oldWorld = g_zEffect_World;
    zClass_NodePartial *const oldResourceNode = g_zEffect_ResourceNode;
    const int oldEntriesInstantiated = g_zEffectAnim_EntriesInstantiated;
    void *const oldHeapPtr = g_zEffectAnim_HeapPtr;
    const short oldCountsPackedLoWord = g_zEffectAnim_CountsPackedLoWord;
    const short oldEntryCount = g_zEffectAnim_EntryCount;
    zEffectAnimEntry *const oldEntryList = g_zEffectAnim_EntryList;
    const int oldTextIdEntryCount = g_zEffectAnim_TextIdEntryCount;
    zEffectAnimTextIdEntry *const oldTextIdEntryList = g_zEffectAnim_TextIdEntryList;
    const int oldSourceFileStampCount = g_zEffectAnim_SourceFileStampCount;
    zEffectAnimSourceFileStamp *const oldSourceFileStampList = g_zEffectAnim_SourceFileStampList;
    const float oldDefaultGravity = g_zEffect_DefaultGravity;
    const int oldConditionalRefPosEnabled = g_zEffect_ConditionalRefPosEnabled;
    const int oldVariantOverrideEnabled = g_zEffect_VariantOverrideEnabled;
    const float oldConditionalRefPosX = g_zEffect_ConditionalRefPosX;
    const float oldConditionalRefPosY = g_zEffect_ConditionalRefPosY;
    const float oldConditionalRefPosZ = g_zEffect_ConditionalRefPosZ;
    const unsigned int oldVariantOverridePackedIds = g_zEffect_VariantOverridePackedIds;
    const float oldFrameDeltaRemainingSec = g_zEffect_FrameDeltaRemainingSec;
    char oldFilename[sizeof(g_zEffectAnim_ZbdFilename)] = {};
    std::memcpy(oldFilename, g_zEffectAnim_ZbdFilename, sizeof(oldFilename));

    char tempPath[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempPath), tempPath) == 0 ||
        GetTempFileNameA(tempPath, "zli", 0, tempFile) == 0) {
        return 1;
    }

    zClass_NodePartial world = {};
    zClass_NodePartial resource = {};
    EffectAnimLoadZbdHeaderBlock header = {};
    header.entriesInstantiated = 0;
    header.heapPtr = &resource;
    header.countsPackedLoWord = 7;
    header.entryCount = 0;
    header.textIdEntryCount = 0;
    header.worldNode = &resource;
    header.defaultGravity = 8.5f;

    if (!WriteEffectAnimLoadZbdMinimalFile(tempFile, header)) {
        DeleteFileA(tempFile);
        return 2;
    }

    g_zEffect_World = &world;
    g_zEffect_ResourceNode = &resource;
    g_zEffectAnim_EntriesInstantiated = 0;
    zEffect_Anim::SetZbdFilename(tempFile);
    const int result = zEffect_Anim::LoadAndInstantiate();
    zEffectAnimEntry *const loadedEntryList = g_zEffectAnim_EntryList;

    const bool ok = result == 0 && g_zEffect_World == &world &&
                    g_zEffectAnim_EntriesInstantiated == 1 &&
                    g_zEffectAnim_EntryCount == 0 && g_zEffectAnim_TextIdEntryCount == 0 &&
                    g_zEffectAnim_SourceFileStampCount == 0 &&
                    g_zEffect_DefaultGravity == header.defaultGravity;

    if (loadedEntryList != nullptr && loadedEntryList != oldEntryList) {
        std::free(loadedEntryList);
    }
    g_zEffect_World = oldWorld;
    g_zEffect_ResourceNode = oldResourceNode;
    g_zEffectAnim_EntriesInstantiated = oldEntriesInstantiated;
    g_zEffectAnim_HeapPtr = oldHeapPtr;
    g_zEffectAnim_CountsPackedLoWord = oldCountsPackedLoWord;
    g_zEffectAnim_EntryCount = oldEntryCount;
    g_zEffectAnim_EntryList = oldEntryList;
    g_zEffectAnim_TextIdEntryCount = oldTextIdEntryCount;
    g_zEffectAnim_TextIdEntryList = oldTextIdEntryList;
    g_zEffectAnim_SourceFileStampCount = oldSourceFileStampCount;
    g_zEffectAnim_SourceFileStampList = oldSourceFileStampList;
    g_zEffect_DefaultGravity = oldDefaultGravity;
    g_zEffect_ConditionalRefPosEnabled = oldConditionalRefPosEnabled;
    g_zEffect_VariantOverrideEnabled = oldVariantOverrideEnabled;
    g_zEffect_ConditionalRefPosX = oldConditionalRefPosX;
    g_zEffect_ConditionalRefPosY = oldConditionalRefPosY;
    g_zEffect_ConditionalRefPosZ = oldConditionalRefPosZ;
    g_zEffect_VariantOverridePackedIds = oldVariantOverridePackedIds;
    g_zEffect_FrameDeltaRemainingSec = oldFrameDeltaRemainingSec;
    std::memcpy(g_zEffectAnim_ZbdFilename, oldFilename, sizeof(oldFilename));
    DeleteFileA(tempFile);

    return ok ? 0 : 3;
}

extern "C" int zeffect_tick_reset_delay_callbacks_smoke(void) {
    zEffectAnimEntry timerEntry = {};
    timerEntry.activationState = 5;
    timerEntry.activationMode = 1;
    timerEntry.activationCountdown = 1.5f;
    if (zEffect::TickResetDelayOnTimer(&timerEntry, 0.25f) != 1.25f ||
        timerEntry.activationCountdown != 1.25f) {
        return 1;
    }

    timerEntry.activationCountdown = 0.25f;
    if (zEffect::TickResetDelayOnTimer(&timerEntry, 0.5f) != -0.25f ||
        timerEntry.activationCountdown != -0.25f) {
        return 2;
    }

    timerEntry.activationMode = 0;
    timerEntry.activationCountdown = 3.0f;
    if (zEffect::TickResetDelayOnTimer(&timerEntry, 1.0f) != 3.0f ||
        timerEntry.activationCountdown != 3.0f) {
        return 3;
    }

    zEffectAnimEntry hitEntry = {};
    zClass_NodePartial hitNode = {};
    hitEntry.activationState = 5;
    hitEntry.activationMode = 0;
    hitEntry.activationCountdown = 2.0f;
    if (zEffect::TickResetDelayOnHit(&hitEntry, &hitNode, 0, 0.75f) != 0 ||
        hitEntry.activationCountdown != 1.25f) {
        return 4;
    }

    hitNode.listCountA = 0x200;
    if (zEffect::TickResetDelayOnHit(&hitEntry, &hitNode, 0, 0.75f) != 0 ||
        hitEntry.activationCountdown != 1.25f) {
        return 5;
    }

    hitNode.listCountA = 0;
    hitEntry.activationMode = 1;
    if (zEffect::TickResetDelayOnHit(&hitEntry, &hitNode, 0, 0.75f) != 0 ||
        hitEntry.activationCountdown != 1.25f) {
        return 6;
    }

    hitEntry.activationMode = 2;
    if (zEffect::TickResetDelayOnHit(&hitEntry, &hitNode, 0, 1.5f) != 0 ||
        hitEntry.activationCountdown != -0.25f) {
        return 7;
    }

    return 0;
}

extern "C" int zeffect_anim_debug_frame_tag_smoke(void) {
    const int oldFrameTick = g_zVideo_FrameTick;
    const int oldDebugFrameTag = g_zEffect_Anim_DebugFrameTag;

    g_zVideo_FrameTick = 123;
    g_zEffect_Anim_DebugFrameTag = 0;
    const int result = zEffect::SetAnimDebugFrameTag();
    const bool ok = result == 124 && g_zEffect_Anim_DebugFrameTag == 124;

    g_zVideo_FrameTick = oldFrameTick;
    g_zEffect_Anim_DebugFrameTag = oldDebugFrameTag;
    return ok ? 0 : 1;
}

extern "C" int zeffect_anim_find_next_async_entry_smoke(void) {
    zEffectAnimEntry *const oldEntryList = g_zEffectAnim_EntryList;
    const std::int16_t oldEntryCount = g_zEffectAnim_EntryCount;

    zEffectAnimEntry entries[4] = {};
    entries[1].flags = 0x10;
    entries[3].flags = 0x10;
    g_zEffectAnim_EntryList = entries;
    g_zEffectAnim_EntryCount = 4;

    const bool found = zEffectAnim::FindNextAsyncEntry(nullptr) == &entries[1] &&
                       zEffectAnim::FindNextAsyncEntry(&entries[1]) == &entries[3] &&
                       zEffectAnim::FindNextAsyncEntry(&entries[3]) == nullptr;

    g_zEffectAnim_EntryList = oldEntryList;
    g_zEffectAnim_EntryCount = oldEntryCount;
    return found ? 0 : 1;
}

extern "C" int zeffect_anim_get_root_node_or_null_smoke(void) {
    if (zEffectAnim::GetRootNodeOrNull(nullptr) != nullptr) {
        return 1;
    }

    zClass_NodePartial root = {};
    zEffectAnimEntry entry = {};
    entry.boundNode = &root;
    return zEffectAnim::GetRootNodeOrNull(&entry) == &root ? 0 : 2;
}

extern "C" int zeffect_anim_capture_node_states_smoke(void) {
    if (zEffect_Anim::CaptureNodeStates(nullptr) != -1) {
        return 1;
    }

    zEffectAnimTrackedNode records[4] = {};
    for (int i = 0; i < 4; ++i) {
        records[i].capturedState.activeFlag = -99;
        records[i].capturedState.usesCachedMatrix = -88;
        for (float &value : records[i].capturedState.transformSnapshot) {
            value = -77.0f;
        }
    }

    zEffectAnimEntry entry = {};
    entry.trackedNodeCount = 4;
    entry.trackedNodeList = records;

    zClass_NodePartial nonObjectNode = {};
    nonObjectNode.flags = 0x04;
    nonObjectNode.classId = 1;
    records[1].trackedNode = &nonObjectNode;

    zClass_Object3DDataPartial matrixData = {};
    zClass_NodePartial matrixNode = {};
    matrixNode.flags = 0x04;
    matrixNode.classId = 5;
    matrixNode.classData = &matrixData;
    matrixData.flags = 0x10;
    for (int i = 0; i < 12; ++i) {
        matrixData.localMatrix[i] = static_cast<float>(i + 1);
    }
    records[2].trackedNode = &matrixNode;

    zClass_Object3DDataPartial decomposedData = {};
    zClass_NodePartial decomposedNode = {};
    decomposedNode.classId = 5;
    decomposedNode.classData = &decomposedData;
    decomposedData.localMatrix[9] = 7.0f;
    decomposedData.localMatrix[10] = 8.0f;
    decomposedData.localMatrix[11] = 9.0f;
    decomposedData.rotation = {1.0f, 2.0f, 3.0f};
    decomposedData.scale = {4.0f, 5.0f, 6.0f};
    records[3].trackedNode = &decomposedNode;

    if (zEffect_Anim::CaptureNodeStates(&entry) != 0) {
        return 2;
    }

    bool matrixOk = true;
    for (int i = 0; i < 12; ++i) {
        matrixOk =
            matrixOk && records[2].capturedState.transformSnapshot[i] == static_cast<float>(i + 1);
    }

    return records[0].capturedState.activeFlag == -99 && records[1].capturedState.activeFlag == 1 &&
                   records[1].capturedState.usesCachedMatrix == -88 &&
                   records[2].capturedState.activeFlag == 1 &&
                   records[2].capturedState.usesCachedMatrix == 1 && matrixOk &&
                   records[3].capturedState.activeFlag == 0 &&
                   records[3].capturedState.usesCachedMatrix == 0 &&
                   records[3].capturedState.transformSnapshot[0] == 7.0f &&
                   records[3].capturedState.transformSnapshot[1] == 8.0f &&
                   records[3].capturedState.transformSnapshot[2] == 9.0f &&
                   records[3].capturedState.transformSnapshot[3] == 1.0f &&
                   records[3].capturedState.transformSnapshot[4] == 2.0f &&
                   records[3].capturedState.transformSnapshot[5] == 3.0f &&
                   records[3].capturedState.transformSnapshot[6] == 4.0f &&
                   records[3].capturedState.transformSnapshot[7] == 5.0f &&
                   records[3].capturedState.transformSnapshot[8] == 6.0f &&
                   records[3].capturedState.transformSnapshot[9] == -77.0f
               ? 0
               : 3;
}

extern "C" int zeffect_anim_restore_node_states_smoke(void) {
    if (zEffect_Anim::RestoreNodeStates(nullptr) != -1) {
        return 1;
    }

    zEffectAnimTrackedNode records[4] = {};
    zEffectAnimEntry entry = {};
    entry.trackedNodeCount = 4;
    entry.trackedNodeList = records;

    zClass_NodePartial nonObjectNode = {};
    nonObjectNode.classId = 1;
    records[1].trackedNode = &nonObjectNode;
    records[1].capturedState.activeFlag = 1;

    zClass_NodePartial matrixNode = {};
    zClass_Object3DDataPartial matrixData = {};
    zDiPartial matrixDi = {};
    matrixNode.classId = 5;
    matrixNode.flags = 0x04;
    matrixNode.classData = &matrixData;
    matrixNode.userDataOrDiRef =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&matrixDi));
    matrixDi.flags = 0x08;
    matrixDi.blendScale = 3.0f;
    records[2].trackedNode = &matrixNode;
    records[2].capturedState.activeFlag = 0;
    records[2].capturedState.usesCachedMatrix = 1;
    for (int i = 0; i < 12; ++i) {
        records[2].capturedState.transformSnapshot[i] = static_cast<float>(i + 1);
    }

    zClass_NodePartial decomposedNode = {};
    zClass_Object3DDataPartial decomposedData = {};
    decomposedNode.classId = 5;
    decomposedNode.classData = &decomposedData;
    records[3].trackedNode = &decomposedNode;
    records[3].capturedState.activeFlag = 1;
    records[3].capturedState.usesCachedMatrix = 0;
    records[3].capturedState.transformSnapshot[0] = 10.0f;
    records[3].capturedState.transformSnapshot[1] = 11.0f;
    records[3].capturedState.transformSnapshot[2] = 12.0f;
    records[3].capturedState.transformSnapshot[3] = 1.0f;
    records[3].capturedState.transformSnapshot[4] = 2.0f;
    records[3].capturedState.transformSnapshot[5] = 3.0f;
    records[3].capturedState.transformSnapshot[6] = 4.0f;
    records[3].capturedState.transformSnapshot[7] = 5.0f;
    records[3].capturedState.transformSnapshot[8] = 6.0f;

    if (zEffect_Anim::RestoreNodeStates(&entry) != 0) {
        return 2;
    }

    bool matrixOk = true;
    for (int i = 0; i < 12; ++i) {
        matrixOk = matrixOk && matrixData.localMatrix[i] == static_cast<float>(i + 1);
    }

    return (nonObjectNode.flags & 0x04) != 0 && (matrixNode.flags & 0x04) == 0 && matrixOk &&
                   (matrixDi.flags & 0x08) == 0 && matrixDi.blendScale == 0.0f &&
                   (decomposedNode.flags & 0x04) != 0 && decomposedData.localMatrix[9] == 10.0f &&
                   decomposedData.localMatrix[10] == 11.0f &&
                   decomposedData.localMatrix[11] == 12.0f && decomposedData.rotation.x == 1.0f &&
                   decomposedData.rotation.y == 2.0f && decomposedData.rotation.z == 3.0f &&
                   decomposedData.scale.x == 4.0f && decomposedData.scale.y == 5.0f &&
                   decomposedData.scale.z == 6.0f
               ? 0
               : 3;
}

extern "C" int zeffect_anim_reset_for_node_smoke(void) {
    if (zEffectAnim::ResetForNode(nullptr) != -1) {
        return 1;
    }

    zEffectAnimEntry entry = {};
    if (zEffectAnim::ResetForNode(&entry) != -1) {
        return 2;
    }

    zEffectAnimSurfaceRuntime runtime[2] = {};
    runtime[0].resetMode = 4;
    runtime[0].runState = 9;
    runtime[0].sequenceElapsedSec = 1.0f;
    runtime[0].eventElapsedSec = 2.0f;
    runtime[0].loopElapsedSec = 3.0f;
    runtime[0].loopIterationCount = 4;
    runtime[0].eventStream = &runtime[0];
    runtime[1].resetMode = 6;
    runtime[1].runState = 9;
    runtime[1].sequenceElapsedSec = 5.0f;
    runtime[1].eventElapsedSec = 7.0f;
    runtime[1].loopElapsedSec = 11.0f;
    runtime[1].loopIterationCount = 12;
    runtime[1].eventStream = &runtime[1];

    zClass_NodePartial worldRoot = {};
    worldRoot.classId = 2;
    entry = {};
    entry.boundNode = &worldRoot;
    entry.flags = 0x100;
    entry.triggerCurrentValue = 123.0f;
    entry.runtimeSequenceCount = 2;
    entry.runtimeList = runtime;
    if (zEffectAnim::ResetForNode(&entry) != 0 || (entry.flags & 0x100) != 0 ||
        entry.triggerCurrentValue != 0.0f || runtime[0].runState != 4 ||
        runtime[0].sequenceElapsedSec != 0.0f || runtime[0].eventElapsedSec != 0.0f ||
        runtime[0].loopElapsedSec != 0.0f || runtime[0].loopIterationCount != 0 ||
        runtime[0].currentEvent != &runtime[0] || runtime[1].runState != 6 ||
        runtime[1].loopElapsedSec != 0.0f) {
        return 3;
    }

    zClass_NodePartial effectWorld = {};
    effectWorld.flags = 1;
    zClass_NodePartial objectRoot = {};
    objectRoot.classId = 5;
    objectRoot.flags = 0x80;
    g_zEffect_World = &effectWorld;
    entry = {};
    entry.boundNode = &objectRoot;
    if (zEffectAnim::ResetForNode(&entry) != 0 || (entry.flags & 0x100) == 0 ||
        effectWorld.listCountB != 1 || effectWorld.listB[0] != &objectRoot ||
        objectRoot.listCountA != 1 || objectRoot.listA[0] != &effectWorld) {
        std::free(effectWorld.listB);
        std::free(objectRoot.listA);
        g_zEffect_World = nullptr;
        return 4;
    }

    std::free(effectWorld.listB);
    std::free(objectRoot.listA);
    g_zEffect_World = nullptr;
    return 0;
}

extern "C" int zeffect_anim_runtime_sequence_group_smoke(void) {
    zClass_NodePartial *const oldWorld = g_zEffect_World;
    zEffectAnimEntry *const oldEntryList = g_zEffectAnim_EntryList;
    const short oldEntryCount = g_zEffectAnim_EntryCount;
    const int oldQueueEnabled = g_zEffectAnim_RecordQueueEnabled;
    const int oldDispatchEnabled = g_zEffectAnim_DispatchEnabled;
    const unsigned int oldDispatchTag = g_zEffectAnim_ActivationDispatchTagHigh;
    const auto oldDispatchCallback = g_zEffectAnim_ActivationDispatchCallback;
    const int oldSkipStopDelay = g_zEffect_SkipStopDelay;
    const float oldFrameDelta = g_FrameDeltaTimeSec;
    const float oldEffectDelta = g_zEffect_FrameDeltaRemainingSec;

    auto cleanup = [&]() {
        zEffect_Anim::ClearActivationRecords();
        g_zEffectAnim_RecordQueueEnabled = oldQueueEnabled;
        g_zEffectAnim_DispatchEnabled = oldDispatchEnabled;
        g_zEffectAnim_ActivationDispatchCallback = oldDispatchCallback;
        g_zEffectAnim_ActivationDispatchTagHigh = oldDispatchTag;
        g_zEffectAnim_EntryList = oldEntryList;
        g_zEffectAnim_EntryCount = oldEntryCount;
        g_zEffect_SkipStopDelay = oldSkipStopDelay;
        g_FrameDeltaTimeSec = oldFrameDelta;
        g_zEffect_FrameDeltaRemainingSec = oldEffectDelta;
        g_zEffect_World = oldWorld;
        FreeZClassRuntimeForZEffectTest();
    };

    ResetZClassRuntimeForZEffectTest();
    zEffect_Anim::ClearActivationRecords();
    g_zEffectAnim_RecordQueueEnabled = 1;
    g_zEffectAnim_DispatchEnabled = 0;
    g_zEffectAnim_ActivationDispatchCallback = nullptr;
    g_zEffectAnim_ActivationDispatchTagHigh = 0;
    g_zEffect_SkipStopDelay = 0;
    g_zEffect_World = nullptr;

    zClass_NodePartial boundNode = {};
    boundNode.classId = 2;
    zClass_NodePartial runtimeNode = {};
    runtimeNode.callbackPriority = 1;

    zEffectAnimEntry entry = {};
    std::strcpy(entry.name, "group_entry");
    entry.boundNode = &boundNode;
    entry.callbackNode = &boundNode;
    entry.runtimeNode = &runtimeNode;
    entry.priority = 3;

    if (zEffectAnim::ActivateRuntime(&entry, nullptr) != &entry || entry.activationState != 2 ||
        runtimeNode.callbackContext != reinterpret_cast<zClass_NodePartial *>(&entry) ||
        runtimeNode.actionCallback == nullptr) {
        cleanup();
        return 1;
    }

    entry.activationState = 0;
    if (zEffectAnim::SetVelocity(&entry, nullptr, 1.0f, 0.0f, 0.0f) != &entry ||
        (entry.flags & 0x80u) == 0 || entry.velocityX != 1.0f || entry.velocityY != 0.0f ||
        entry.velocityZ != 0.0f || g_zEffectAnim_ActivationRecordCount != 1 ||
        g_zEffectAnim_ActivationRecordTable[0].commandType != 2 ||
        g_zEffectAnim_ActivationRecordTable[0].params[0].f32 != 1.0f) {
        cleanup();
        return 2;
    }

    zClass_NodePartial refNode = {};
    zVec3 refVec = {2.0f, 3.0f, 4.0f};
    zVec3 velocityVec = {0.0f, 5.0f, 0.0f};
    entry.activationState = 0;
    zEffect_Anim::ClearActivationRecords();
    if (zEffectAnim::SetPositionRefAndVelocity(&entry, nullptr, &refNode, &refVec,
                                                &velocityVec) != &entry) {
        cleanup();
        return 31;
    }
    if ((entry.flags & 0x80u) == 0 || entry.velocityY != 5.0f) {
        cleanup();
        return 32;
    }
    if (entry.resetScratch[0] !=
            static_cast<unsigned int>(reinterpret_cast<std::uintptr_t>(&refNode)) ||
        entry.resetScratch[1] != 0x40000000u || entry.resetScratch[2] != 0x40400000u ||
        entry.resetScratch[3] != 0x40800000u) {
        cleanup();
        return 33;
    }

    zVec3 refVecB = {6.0f, 7.0f, 8.0f};
    entry.activationState = 0;
    zEffect_Anim::ClearActivationRecords();
    if (zEffectAnim::SetTransformRefs(&entry, nullptr, &refNode, &refVec, &boundNode, &refVecB) !=
            &entry ||
        entry.resetScratch[0] !=
            static_cast<unsigned int>(reinterpret_cast<std::uintptr_t>(&refNode)) ||
        entry.resetScratch[4] !=
            static_cast<unsigned int>(reinterpret_cast<std::uintptr_t>(&boundNode)) ||
        entry.resetScratch[5] != 0x40c00000u || entry.resetScratch[7] != 0x41000000u) {
        cleanup();
        return 4;
    }

    entry.activationState = 0;
    zEffect_Anim::ClearActivationRecords();
    if (zEffectAnim::SetTransformRotAndVelocity(&entry, nullptr, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f,
                                                 6.0f, 7.0f, 8.0f, 9.0f) != &entry ||
        entry.velocityX != 7.0f || entry.velocityY != 8.0f || entry.velocityZ != 9.0f ||
        g_zEffectAnim_ActivationRecordCount != 1 ||
        g_zEffectAnim_ActivationRecordTable[0].commandType != 1 ||
        g_zEffectAnim_ActivationRecordTable[0].params[8].f32 != 9.0f) {
        cleanup();
        return 5;
    }

    zEffectAnimEventHeader noOpEvent = {};
    noOpEvent.eventType = 0x22;
    noOpEvent.startMode = 2;
    noOpEvent.recordSize = sizeof(noOpEvent);
    zEffectAnimSurfaceRuntime runtime = {};
    runtime.eventStream = &noOpEvent;
    runtime.eventStreamSize = sizeof(noOpEvent);
    runtime.currentEvent = &noOpEvent;
    runtime.runState = 0;
    g_zEffect_FrameDeltaRemainingSec = 0.25f;
    if (zEffect_Anim::RunSequenceEvents(&entry, &runtime) != 0 || runtime.runState != 2 ||
        runtime.currentEvent !=
            static_cast<void *>(reinterpret_cast<unsigned char *>(&noOpEvent) +
                                sizeof(noOpEvent)) ||
        runtime.eventElapsedSec != 0.0f) {
        cleanup();
        return 6;
    }

    zEffectAnimEventHeader sequenceEvent = {};
    sequenceEvent.eventType = 0x22;
    sequenceEvent.startMode = 2;
    sequenceEvent.recordSize = sizeof(sequenceEvent);
    runtime = {};
    runtime.eventStream = &sequenceEvent;
    runtime.eventStreamSize = sizeof(sequenceEvent);
    runtime.currentEvent = &sequenceEvent;
    runtime.runState = 0;
    entry.activationState = 2;
    entry.runtimeList = &runtime;
    entry.runtimeSequenceCount = 1;
    entry.triggerBaseValue = -1.0f;
    entry.triggerCurrentValue = 0.0f;
    entry.eventCallback = EffectAnimEventCallback;
    entry.eventCallbackContext = &entry;
    g_effectAnimEventCallbackCallCount = 0;
    g_FrameDeltaTimeSec = 0.25f;
    runtimeNode.callbackContext = reinterpret_cast<zClass_NodePartial *>(&entry);
    if (zEffect_Anim::RunSequence(&runtimeNode) != 0 || runtime.runState != 2 ||
        entry.triggerCurrentValue != 0.25f || g_effectAnimEventCallbackCallCount != 1 ||
        entry.activationState != 3) {
        cleanup();
        return 61;
    }

    zEffectAnimEntry entries[2] = {};
    zClass_NodePartial childBound = {};
    childBound.classId = 2;
    zClass_NodePartial childRuntime = {};
    childRuntime.callbackPriority = 1;
    std::strcpy(entries[1].name, "child");
    entries[1].boundNode = &childBound;
    entries[1].callbackNode = &childBound;
    entries[1].runtimeNode = &childRuntime;
    entries[1].priority = 2;
    g_zEffectAnim_EntryList = entries;
    g_zEffectAnim_EntryCount = 2;

    zEffectAnimRuntimeRef runtimeRefs[1] = {};
    entry.runtimeRefList = runtimeRefs;
    entry.runtimeRefCount = 1;
    entry.velocityX = 10.0f;
    entry.velocityY = 11.0f;
    entry.velocityZ = 12.0f;

    zEffectSurfaceRefEvent surfaceRef = {};
    std::strcpy(surfaceRef.sequenceName, "child");
    surfaceRef.animEntryIndex = 1;
    surfaceRef.runtimeRefIndex = 0;
    runtime = {};
    if (zEffect::HandleSurfaceRefEvent(&entry, &runtime, &surfaceRef) != 2 ||
        runtime.runState != 2 || runtimeRefs[0].cachedChildEntry != &entries[1] ||
        entries[1].activationState != 2 || entries[1].velocityZ != 12.0f) {
        cleanup();
        return 7;
    }

    entries[1].activationState = 0;
    runtimeRefs[0].cachedChildEntry = nullptr;
    zEffectTransformRefsEvent transformRefs = {};
    std::strcpy(transformRefs.animName, "child");
    transformRefs.animEntryIndex = 1;
    transformRefs.runtimeRefIndex = 0;
    transformRefs.flags = 0x10 | 0x0400;
    transformRefs.refPointA = {13.0f, 14.0f, 15.0f};
    transformRefs.refPointB = {16.0f, 17.0f, 18.0f};
    if (zEffect::HandleTransformRefsEvent(&entry, &transformRefs) != 2 ||
        runtimeRefs[0].cachedChildEntry != &entries[1] || entries[1].activationState != 2 ||
        entries[1].resetScratch[1] != 0x41500000u ||
        entries[1].resetScratch[7] != 0x41900000u) {
        cleanup();
        return 8;
    }

    zEffectAnimEmitterEvent emitterEvent = {};
    std::strcpy(emitterEvent.animName, "child");
    emitterEvent.cachedEntryIndex = 1;
    entries[1].activationState = 2;
    entries[1].triggerBaseValue = -1.0f;
    if (zEffect::HandleNamedAnimStopEvent(&entry, &emitterEvent) != 2 ||
        entries[1].activationState != 3 || childRuntime.actionCallback != nullptr) {
        cleanup();
        return 9;
    }

    entries[1].activationState = 2;
    entries[1].triggerBaseValue = -1.0f;
    if (zEffect::HandleEmitterPlayEvent(&entry, &emitterEvent) != 2 ||
        entries[1].activationState != 1 || (entries[1].flags & 0x40u) == 0) {
        cleanup();
        return 10;
    }

    zEffectAnimEntry stopEntry = {};
    zClass_NodePartial stopRuntime = {};
    stopRuntime.callbackPriority = 1;
    stopEntry.activationState = 2;
    stopEntry.runtimeNode = &stopRuntime;
    stopEntry.triggerBaseValue = 1.0f;
    stopEntry.triggerCurrentValue = 0.5f;
    if (zEffectAnim::Stop(&stopEntry) != 0 || stopRuntime.actionCallback == nullptr ||
        stopEntry.triggerCurrentValue != 0.0f) {
        cleanup();
        return 11;
    }

    g_FrameDeltaTimeSec = 1.1f;
    stopRuntime.callbackContext = reinterpret_cast<zClass_NodePartial *>(&stopEntry);
    if (zEffectAnim::RunStopDelayCallback(&stopRuntime) != 0 ||
        stopEntry.activationState != 1 || stopRuntime.actionCallback != nullptr ||
        (stopEntry.flags & 0x40u) == 0) {
        cleanup();
        return 12;
    }

    cleanup();
    return 0;
}

extern "C" int zeffect_anim_init_shutdown_smoke(void) {
    g_zEffectAnim_EntriesInstantiated = 0;
    g_zEffectAnim_HeapPtr = std::malloc(4);
    g_zEffectAnim_EntryCount = 1;
    g_zEffectAnim_CountsPackedLoWord = 7;
    g_zEffectAnim_EntryList =
        static_cast<zEffectAnimEntry *>(std::calloc(1, sizeof(zEffectAnimEntry)));
    g_zEffectAnim_EntryList[0].activationState = 5;
    g_zEffectAnim_TextIdEntryCount = 3;
    g_zEffectAnim_TextIdEntryList = static_cast<zEffectAnimTextIdEntry *>(std::malloc(4));
    g_zEffectAnim_ActivationRecordTable =
        static_cast<zEffectAnimActivationRecord *>(std::malloc(4));
    g_zEffectAnim_ActivationRecordCount = 2;

    if (zEffect_Anim::Shutdown() != 0) {
        return 1;
    }

    if (g_zEffectAnim_HeapPtr != nullptr || g_zEffectAnim_EntryList != nullptr ||
        g_zEffectAnim_EntryCount != 0 || g_zEffectAnim_TextIdEntryList != nullptr ||
        g_zEffectAnim_TextIdEntryCount != 0 || g_zEffectAnim_ActivationRecordTable != nullptr ||
        g_zEffectAnim_ActivationRecordCount != 0 || g_zEffectAnim_EntriesInstantiated != 0) {
        return 2;
    }

    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager = MakeManager(sentinel);
    g_zUtil_ZbdManager = &manager;
    g_zEffectAnim_EnableZarRegistration = 1;
    g_zEffectAnim_ZbdFilename[0] = 'x';
    g_zEffect_World = reinterpret_cast<zClass_NodePartial *>(0x1234);
    g_zEffect_ConditionalRefPosEnabled = 1;
    g_zEffect_VariantOverrideEnabled = 1;

    const int initResult = zEffect_Anim::Init();
    const bool initOk = initResult == 0 && g_zEffectAnim_ZbdFilename[0] == '\0' &&
                        g_zEffect_World == nullptr && g_zEffect_ConditionalRefPosEnabled == 0 &&
                        g_zEffect_VariantOverrideEnabled == 0 &&
                        g_zEffect_DefaultGravity < -9.79f && g_zEffect_DefaultGravity > -9.81f &&
                        manager.sectionHandlerCount == 3 && g_zEffect_RandUnitScale > 0.0f;

    ClearRegisteredHandlers(sentinel);
    g_zUtil_ZbdManager = nullptr;
    return initOk ? 0 : 3;
}

extern "C" int zeffect_shutdown_all_smoke(void) {
    g_zEffectAnim_EnableZarRegistration = 0;
    g_zEffectAnim_EntriesInstantiated = 1;
    g_zEffectAnim_HeapPtr = std::malloc(4);
    g_zEffectAnim_EntryCount = 1;
    g_zEffectAnim_EntryList =
        static_cast<zEffectAnimEntry *>(std::calloc(1, sizeof(zEffectAnimEntry)));
    g_zEffectAnim_TextIdEntryCount = 1;
    g_zEffectAnim_TextIdEntryList = static_cast<zEffectAnimTextIdEntry *>(std::malloc(4));
    g_zEffectAnim_ActivationRecordTable =
        static_cast<zEffectAnimActivationRecord *>(std::malloc(4));
    g_zEffectAnim_ActivationRecordCount = 1;
    g_zEffect_RuntimeManager.templates = static_cast<zEffect_RuntimeEntry *>(std::malloc(4));
    g_zEffect_RuntimeManager.freeList = zArchiveList_CreateEmpty();
    g_zEffect_RuntimeManager.recycleCount = 3;

    if (zEffect::ShutdownAll() != 0) {
        return 1;
    }

    return g_zEffect_RuntimeManager.templates == nullptr &&
                   g_zEffect_RuntimeManager.freeList == nullptr &&
                   g_zEffect_RuntimeManager.recycleCount == 0 &&
                   g_zEffectAnim_EntriesInstantiated == 0 &&
                   g_zEffectAnim_HeapPtr == nullptr && g_zEffectAnim_EntryList == nullptr &&
                   g_zEffectAnim_TextIdEntryList == nullptr &&
                   g_zEffectAnim_ActivationRecordTable == nullptr
               ? 0
               : 2;
}

extern "C" int zeffect_find_template_index_by_name_smoke(void) {
    const zEffect_RuntimeManager oldManager = g_zEffect_RuntimeManager;

    char alpha[] = "alpha";
    char target[] = "target";
    char beta[] = "beta";
    zEffect_RuntimeEntry templates[3] = {};
    templates[0].effectName = alpha;
    templates[1].effectName = target;
    templates[2].effectName = beta;
    g_zEffect_RuntimeManager.templateCount = 3;
    g_zEffect_RuntimeManager.templates = templates;

    const bool found = zEffect::FindTemplateIndexByName("alpha") == 0 &&
                       zEffect::FindTemplateIndexByName("target") == 1 &&
                       zEffect::FindTemplateIndexByName("missing") == -1;

    g_zEffect_RuntimeManager.templateCount = 0;
    const bool emptyTable = zEffect::FindTemplateIndexByName("alpha") == -1;

    g_zEffect_RuntimeManager = oldManager;
    return found && emptyTable ? 0 : 1;
}

extern "C" int zeffect_find_node_user_data_recursive_smoke(void) {
    zClass_NodePartial root = {};
    zClass_NodePartial firstChild = {};
    zClass_NodePartial secondChild = {};
    zClass_NodePartial grandchild = {};
    zClass_NodePartial *rootChildren[2] = {&firstChild, &secondChild};
    zClass_NodePartial *firstChildren[1] = {&grandchild};
    int rootDi = 1;
    int secondDi = 2;
    int grandchildDi = 3;

    root.listCountB = 2;
    root.listB = rootChildren;
    firstChild.listCountB = 1;
    firstChild.listB = firstChildren;
    secondChild.userDataOrDiRef =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&secondDi));
    grandchild.userDataOrDiRef =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&grandchildDi));

    if (zEffect::FindNodeUserDataRecursive(&root) != &grandchildDi) {
        return 1;
    }

    grandchild.userDataOrDiRef = 0;
    if (zEffect::FindNodeUserDataRecursive(&root) != &secondDi) {
        return 2;
    }

    root.userDataOrDiRef = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&rootDi));
    if (zEffect::FindNodeUserDataRecursive(&root) != &rootDi) {
        return 3;
    }

    root.userDataOrDiRef = 0;
    secondChild.userDataOrDiRef = 0;
    if (zEffect::FindNodeUserDataRecursive(&root) != nullptr) {
        return 4;
    }

    return 0;
}

extern "C" int zeffect_clone_runtime_entry_from_template_smoke(void) {
    const zEffect_RuntimeManager oldManager = g_zEffect_RuntimeManager;
    const int oldCloneMode = g_zEffect_CloneCopyMode;
    const int oldCloneChildrenMode = g_zEffect_CloneCopyChildrenMode;

    zEffect_RuntimeEntry templates[2] = {};
    zClass_NodePartial effectNode = {};
    int displayInstance = 1;

    effectNode.flags = 0x04000000;
    effectNode.userDataOrDiRef =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&displayInstance));
    templates[1].effectIndex = 7;
    templates[1].effectNode = &effectNode;
    templates[1].currentScale = 3.0f;
    g_zEffect_RuntimeManager.templates = templates;
    g_zEffect_CloneCopyMode = 4;
    g_zEffect_CloneCopyChildrenMode = 5;

    if (zEffect::CloneRuntimeEntryFromTemplate(-1) != nullptr ||
        zEffect::CloneRuntimeEntryFromTemplate(0) != nullptr) {
        g_zEffect_RuntimeManager = oldManager;
        g_zEffect_CloneCopyMode = oldCloneMode;
        g_zEffect_CloneCopyChildrenMode = oldCloneChildrenMode;
        return 1;
    }

    zEffect_RuntimeEntry *const clone = zEffect::CloneRuntimeEntryFromTemplate(1);
    const bool cloneOk = clone != nullptr && clone != &templates[1] && clone->effectIndex == 7 &&
                         clone->effectNode == &effectNode &&
                         clone->effectGfxData == &displayInstance && clone->currentScale == 3.0f;
    std::free(clone);

    g_zEffect_RuntimeManager = oldManager;
    g_zEffect_CloneCopyMode = oldCloneMode;
    g_zEffect_CloneCopyChildrenMode = oldCloneChildrenMode;
    return cloneOk ? 0 : 2;
}

extern "C" int zeffect_acquire_runtime_entry_by_index_smoke(void) {
    const zEffect_RuntimeManager oldManager = g_zEffect_RuntimeManager;
    EnsureZrdrFreePool();

    zArchiveList *freeList = zArchiveList_CreateEmpty();
    zEffect_RuntimeEntry templates[2] = {};
    zClass_NodePartial effectNode = {};
    int displayInstance = 1;

    effectNode.flags = 0x04000000;
    effectNode.userDataOrDiRef =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&displayInstance));
    templates[1].effectIndex = 1;
    templates[1].effectNode = &effectNode;
    templates[1].currentScale = 2.0f;
    g_zEffect_RuntimeManager.freeList = freeList;
    g_zEffect_RuntimeManager.templates = templates;
    g_zEffect_RuntimeManager.freshAllocCount = 0;
    g_zEffect_RuntimeManager.recycleCount = 0;

    if (zEffect::AcquireRuntimeEntryByIndex(-1) != nullptr ||
        g_zEffect_RuntimeManager.freshAllocCount != 0 ||
        g_zEffect_RuntimeManager.recycleCount != 0) {
        std::free(freeList);
        g_zEffect_RuntimeManager = oldManager;
        return 1;
    }

    zEffect_RuntimeEntry *const fresh = zEffect::AcquireRuntimeEntryByIndex(1);
    if (fresh == nullptr || fresh->effectIndex != 1 || fresh->effectGfxData != &displayInstance ||
        g_zEffect_RuntimeManager.freshAllocCount != 1 ||
        g_zEffect_RuntimeManager.recycleCount != 0) {
        std::free(fresh);
        std::free(freeList);
        g_zEffect_RuntimeManager = oldManager;
        return 2;
    }

    zArchiveList_PushBackPayload(freeList, fresh);
    zEffect_RuntimeEntry *const reused = zEffect::AcquireRuntimeEntryByIndex(1);
    const bool reusedOk = reused == fresh && freeList->count == 0 &&
                          g_zEffect_RuntimeManager.freshAllocCount == 1 &&
                          g_zEffect_RuntimeManager.recycleCount == 1;

    std::free(reused);
    std::free(freeList);
    g_zEffect_RuntimeManager = oldManager;
    return reusedOk ? 0 : 3;
}

extern "C" int zeffect_compute_distance_sq_to_listener_smoke(void) {
    const zEffect_RuntimeManager oldManager = g_zEffect_RuntimeManager;
    zClass_NodePartial listenerNode = {};
    zClass_Object3DDataPartial listenerData = {};
    zVec3 worldPos{1.0f, 2.0f, 3.0f};

    listenerNode.classId = 5;
    listenerNode.flags = 0x00080000;
    listenerNode.classData = &listenerData;
    listenerData.cachedWorldMatrix[9] = 4.0f;
    listenerData.cachedWorldMatrix[10] = 6.0f;
    listenerData.cachedWorldMatrix[11] = 8.0f;
    g_zEffect_RuntimeManager.listenerNode = &listenerNode;

    const float distanceSq = zEffect::ComputeDistanceSqToListener(&worldPos);
    g_zEffect_RuntimeManager = oldManager;
    return distanceSq == 50.0f ? 0 : 1;
}

extern "C" int zeffect_activate_runtime_entry_at_position_smoke(void) {
    const zEffect_RuntimeManager oldManager = g_zEffect_RuntimeManager;
    zClass_NodePartial parentNode = {};
    zClass_NodePartial listenerNode = {};
    zClass_NodePartial effectNode = {};
    zClass_Object3DDataPartial listenerData = {};
    zClass_Object3DDataPartial effectData = {};
    zDiPartial effectDi = {};
    zDiEntryPartial effectDiEntry = {};
    zModel_MaterialPartial effectMaterial = {};
    zModel_MaterialCyclePartial effectCycle = {};
    zImage_TexDirEntryPartial frameA = {};
    zImage_TexDirEntryPartial frameB = {};
    zImage_TexDirEntryPartial *frameTable[1] = {&frameA};
    zEffect_RuntimeEntry entry = {};

    parentNode.classId = 3;
    listenerNode.classId = 5;
    listenerNode.flags = 0x00080000;
    listenerNode.classData = &listenerData;
    effectNode.classId = 5;
    effectNode.flags = 0x00080000;
    effectNode.classData = &effectData;
    effectDi.entries = &effectDiEntry;
    effectDiEntry.material = &effectMaterial;
    effectMaterial.cycle = &effectCycle;
    effectMaterial.currentTextureDirectoryEntry = &frameB;
    effectCycle.currentFrame = 4.0f;
    effectCycle.frameTable = frameTable;
    entry.effectNode = &effectNode;
    entry.effectGfxData = &effectDi;
    g_zEffect_RuntimeManager.parentNode = &parentNode;
    g_zEffect_RuntimeManager.listenerNode = &listenerNode;
    g_zEffect_RuntimeManager.activatedCount = 0;

    zVec3 nearPos{1.0f, 2.0f, 2.0f};
    if (zEffect::ActivateRuntimeEntryAtPosition(&entry, &nearPos) != 0 ||
        entry.currentScale != 0.0f || entry.fadeInTimeSec != 0.0f ||
        entry.fadeOutStartTimeSec != 0.0f || g_zEffect_RuntimeManager.activatedCount != 0 ||
        parentNode.listCountB != 0) {
        g_zEffect_RuntimeManager = oldManager;
        return 1;
    }

    zVec3 farFadePos{20.0f, 0.0f, 0.0f};
    effectMaterial.currentTextureDirectoryEntry = &frameB;
    effectCycle.currentFrame = 4.0f;
    if (zEffect::ActivateRuntimeEntryAtPosition(&entry, &farFadePos) != 1 ||
        entry.currentScale != 1.25f || entry.fadeInScaleRate != 1.5f ||
        effectData.scale.x != 1.25f || effectData.scale.y != 1.25f ||
        effectData.scale.z != 1.25f || effectData.localMatrix[9] != 20.0f ||
        effectData.localMatrix[10] != 0.0f || effectData.localMatrix[11] != 0.0f ||
        effectMaterial.currentTextureDirectoryEntry != &frameA || effectCycle.currentFrame != 0.0f ||
        parentNode.listCountB != 1 || parentNode.listB[0] != &effectNode ||
        g_zEffect_RuntimeManager.activatedCount != 1) {
        std::free(parentNode.listB);
        std::free(effectNode.listA);
        g_zEffect_RuntimeManager = oldManager;
        return 2;
    }

    std::free(parentNode.listB);
    std::free(effectNode.listA);
    g_zEffect_RuntimeManager = oldManager;
    return 0;
}

extern "C" int zeffect_runtime_node_action_callback_smoke(void) {
    const zEffect_RuntimeManager oldManager = g_zEffect_RuntimeManager;
    const float oldDelta = g_FrameDeltaTimeSec;
    EnsureZrdrFreePool();

    zArchiveList *freeList = zArchiveList_CreateEmpty();
    zClass_NodePartial parentNode = {};
    zClass_NodePartial effectNode = {};
    zClass_Object3DDataPartial effectData = {};
    zEffect_RuntimeEntry entry = {};

    effectNode.classId = 5;
    effectNode.classData = &effectData;
    effectNode.callbackContext = reinterpret_cast<zClass_NodePartial *>(&entry);
    g_zEffect_RuntimeManager.freeList = freeList;
    g_zEffect_RuntimeManager.parentNode = &parentNode;

    if (zEffect::RuntimeNodeActionCallback(&effectNode) != 0 || entry.elapsedSec != 0.0f) {
        std::free(freeList);
        g_zEffect_RuntimeManager = oldManager;
        g_FrameDeltaTimeSec = oldDelta;
        return 1;
    }

    effectNode.flags = 0x04;
    entry.elapsedSec = 0.0f;
    entry.fadeInTimeSec = 2.0f;
    entry.currentScale = 1.0f;
    entry.fadeInScaleRate = 3.0f;
    g_FrameDeltaTimeSec = 0.5f;
    if (zEffect::RuntimeNodeActionCallback(&effectNode) != 0 || entry.elapsedSec != 0.5f ||
        entry.currentScale != 2.5f || effectData.scale.x != 2.5f ||
        effectData.scale.y != 2.5f || effectData.scale.z != 2.5f) {
        std::free(freeList);
        g_zEffect_RuntimeManager = oldManager;
        g_FrameDeltaTimeSec = oldDelta;
        return 2;
    }

    entry.elapsedSec = 2.0f;
    entry.fadeInTimeSec = 1.0f;
    entry.fadeOutStartTimeSec = 5.0f;
    entry.currentScale = 1.0f;
    entry.fadeOutScaleRate = 10.0f;
    g_FrameDeltaTimeSec = 0.2f;
    if (zEffect::RuntimeNodeActionCallback(&effectNode) != 0 || entry.elapsedSec != 2.2f ||
        entry.currentScale != 0.01f || effectData.scale.x != 0.01f ||
        effectData.scale.y != 0.01f || effectData.scale.z != 0.01f) {
        std::free(freeList);
        g_zEffect_RuntimeManager = oldManager;
        g_FrameDeltaTimeSec = oldDelta;
        return 3;
    }

    zClass_Class::AddChild(&parentNode, &effectNode);
    effectNode.callbackContext = reinterpret_cast<zClass_NodePartial *>(&entry);
    effectNode.actionCallback = &entry;
    entry.elapsedSec = 5.0f;
    entry.fadeInTimeSec = 1.0f;
    entry.fadeOutStartTimeSec = 5.0f;
    g_FrameDeltaTimeSec = 0.1f;
    if (zEffect::RuntimeNodeActionCallback(&effectNode) != 0 ||
        effectNode.callbackContext != nullptr || effectNode.actionCallback != nullptr ||
        (effectNode.flags & 0x04) != 0 || freeList->count != 1 ||
        freeList->head->payload != &entry || parentNode.listCountB != 0 ||
        effectNode.listCountA != 0) {
        std::free(parentNode.listB);
        std::free(effectNode.listA);
        std::free(freeList);
        g_zEffect_RuntimeManager = oldManager;
        g_FrameDeltaTimeSec = oldDelta;
        return 4;
    }

    std::free(parentNode.listB);
    std::free(effectNode.listA);
    std::free(freeList);
    g_zEffect_RuntimeManager = oldManager;
    g_FrameDeltaTimeSec = oldDelta;
    return 0;
}

extern "C" int zeffect_spawn_runtime_instance_at_smoke(void) {
    const zEffect_RuntimeManager oldManager = g_zEffect_RuntimeManager;
    zVec3 pos{1.0f, 2.0f, 3.0f};

    g_zEffect_RuntimeManager = {};
    if (zEffect::SpawnRuntimeInstanceAt(0, &pos) != 0 ||
        g_zEffect_RuntimeManager.freshAllocCount != 0) {
        g_zEffect_RuntimeManager = oldManager;
        return 1;
    }

    g_zEffect_RuntimeManager.initialized = 1;
    if (zEffect::SpawnRuntimeInstanceAt(-1, &pos) != 1 ||
        g_zEffect_RuntimeManager.freshAllocCount != 0 ||
        g_zEffect_RuntimeManager.activatedCount != 0) {
        g_zEffect_RuntimeManager = oldManager;
        return 2;
    }

    g_zEffect_RuntimeManager = oldManager;
    return 0;
}

extern "C" int zeffect_init_smoke(void) {
    g_zEffect_RuntimeManager.initialized = 1;
    g_zEffect_RuntimeManager.templateCount = 2;
    g_zEffect_RuntimeManager.loadedTemplateTree = reinterpret_cast<zClass_NodePartial *>(0x1111);
    g_zEffect_RuntimeManager.freeList = reinterpret_cast<zArchiveList *>(0x2222);
    g_zEffect_RuntimeManager.templates = reinterpret_cast<zEffect_RuntimeEntry *>(0x3333);
    g_zEffect_RuntimeManager.parentNode = reinterpret_cast<zClass_NodePartial *>(0x4444);
    g_zEffect_RuntimeManager.listenerNode = reinterpret_cast<zClass_NodePartial *>(0x5555);
    g_zEffect_RuntimeManager.freshAllocCount = 3;
    g_zEffect_RuntimeManager.activatedCount = 4;
    g_zEffect_RuntimeManager.recycleCount = 5;
    g_zEffectAnim_EnableZarRegistration = 0;

    if (zEffect::Init() != 0) {
        return 1;
    }

    const bool initOk =
        g_zEffect_RuntimeManager.initialized == 0 && g_zEffect_RuntimeManager.templateCount == 0 &&
        g_zEffect_RuntimeManager.loadedTemplateTree == nullptr &&
        g_zEffect_RuntimeManager.freeList == nullptr &&
        g_zEffect_RuntimeManager.templates == nullptr &&
        g_zEffect_RuntimeManager.parentNode == nullptr &&
        g_zEffect_RuntimeManager.listenerNode == reinterpret_cast<zClass_NodePartial *>(0x5555) &&
        g_zEffect_RuntimeManager.freshAllocCount == 0 &&
        g_zEffect_RuntimeManager.activatedCount == 0 && g_zEffect_RuntimeManager.recycleCount == 0;
    if (!initOk) {
        return 2;
    }

    ResetZClassRuntimeForZEffectTest();
    EnsureZrdrFreePool();
    zArchiveList *runtimeFreeList = zArchiveList_CreateEmpty();
    zEffect_RuntimeEntry templates[1] = {};
    zClass_NodePartial parentNode = {};
    zClass_NodePartial listenerNode = {};
    zClass_NodePartial effectNode = {};
    zClass_Object3DDataPartial listenerData = {};
    zClass_Object3DDataPartial effectData = {};
    zDiPartial effectDi = {};
    zDiEntryPartial effectDiEntry = {};
    zModel_MaterialPartial effectMaterial = {};
    zModel_MaterialCyclePartial effectCycle = {};
    zImage_TexDirEntryPartial frameA = {};
    zImage_TexDirEntryPartial frameB = {};
    zImage_TexDirEntryPartial *frameTable[1] = {&frameA};

    parentNode.classId = 3;
    listenerNode.classId = 5;
    listenerNode.flags = 0x00080000;
    listenerNode.classData = &listenerData;
    listenerData.cachedWorldMatrix[9] = 100.0f;

    effectNode.classId = 5;
    effectNode.flags = 0x04000000;
    effectNode.classData = &effectData;
    effectNode.userDataOrDiRef =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&effectDi));
    effectDi.entries = &effectDiEntry;
    effectDiEntry.material = &effectMaterial;
    effectMaterial.cycle = &effectCycle;
    effectMaterial.currentTextureDirectoryEntry = &frameB;
    effectCycle.currentFrame = 4.0f;
    effectCycle.frameTable = frameTable;

    templates[0].effectIndex = 0;
    templates[0].effectNode = &effectNode;
    g_zEffect_RuntimeManager.initialized = 1;
    g_zEffect_RuntimeManager.freeList = runtimeFreeList;
    g_zEffect_RuntimeManager.templates = templates;
    g_zEffect_RuntimeManager.parentNode = &parentNode;
    g_zEffect_RuntimeManager.listenerNode = &listenerNode;
    g_zEffect_RuntimeManager.freshAllocCount = 0;
    g_zEffect_RuntimeManager.activatedCount = 0;
    g_zEffect_RuntimeManager.recycleCount = 0;

    zVec3 spawnPos{1.0f, 2.0f, 3.0f};
    if (zEffect::SpawnRuntimeInstanceAt(0, &spawnPos) != 1) {
        return 3;
    }

    zEffect_RuntimeEntry *runtimeEntry =
        reinterpret_cast<zEffect_RuntimeEntry *>(effectNode.callbackContext);
    if (runtimeEntry == nullptr || g_zEffect_RuntimeManager.freshAllocCount != 1 ||
        g_zEffect_RuntimeManager.activatedCount != 1 || effectData.localMatrix[9] != 1.0f ||
        effectData.localMatrix[10] != 2.0f || effectData.localMatrix[11] != 3.0f ||
        effectData.scale.x != 5.0f || effectMaterial.currentTextureDirectoryEntry != &frameA ||
        effectCycle.currentFrame != 0.0f || parentNode.listCountB != 1 ||
        parentNode.listB[0] != &effectNode || (effectNode.flags & 0x04) == 0 ||
        effectNode.actionCallback == nullptr) {
        return 4;
    }

    g_FrameDeltaTimeSec = 2.0f;
    if (zEffect::RuntimeNodeActionCallback(&effectNode) != 0 ||
        effectNode.callbackContext != nullptr || runtimeFreeList->count != 1 ||
        parentNode.listCountB != 0 || effectNode.listCountA != 0 ||
        (effectNode.flags & 0x04) != 0) {
        return 5;
    }

    zEffect_RuntimeEntry *const reusedEntry = zEffect::AcquireRuntimeEntryByIndex(0);
    if (reusedEntry != runtimeEntry || runtimeFreeList->count != 0 ||
        g_zEffect_RuntimeManager.recycleCount != 1) {
        return 6;
    }
    std::free(reusedEntry);

    zEffectAnimTemplateIndexRef templateRefs[1] = {};
    zEffectAnimEntry animEntry = {};
    zEffectAnimRefOffsetEvent offsetEvent = {};
    animEntry.effectTemplateRefList = templateRefs;
    templateRefs[0].templateIndex = 0;
    offsetEvent.refIndex = 0;
    offsetEvent.nodeRefIndex = -200;
    StoreFloatBits(animEntry.resetScratch[1], 4.0f);
    StoreFloatBits(animEntry.resetScratch[2], 5.0f);
    StoreFloatBits(animEntry.resetScratch[3], 6.0f);
    offsetEvent.offsetX = 0.5f;
    offsetEvent.offsetY = 1.5f;
    offsetEvent.offsetZ = 2.5f;

    if (zEffect::HandleEffectTemplateOffsetEvent(&animEntry, &offsetEvent) != 2 ||
        effectData.localMatrix[9] != 4.5f || effectData.localMatrix[10] != 6.5f ||
        effectData.localMatrix[11] != 8.5f) {
        return 7;
    }

    zEffect_RuntimeEntry *const eventEntry =
        reinterpret_cast<zEffect_RuntimeEntry *>(effectNode.callbackContext);
    if (eventEntry == nullptr) {
        return 8;
    }
    zEffect::RuntimeNodeActionCallback(&effectNode);
    zEffect_RuntimeEntry *const recycledEventEntry =
        static_cast<zEffect_RuntimeEntry *>(zArchiveList_PopFrontPayload(runtimeFreeList));
    if (recycledEventEntry != eventEntry) {
        return 9;
    }
    std::free(recycledEventEntry);

    std::free(parentNode.listB);
    std::free(effectNode.listA);
    zArchiveList_Destroy(runtimeFreeList);
    g_zEffect_RuntimeManager.freeList = nullptr;
    g_zEffect_RuntimeManager.templates = nullptr;
    g_zEffect_RuntimeManager.parentNode = nullptr;
    g_zEffect_RuntimeManager.listenerNode = nullptr;
    g_zEffect_RuntimeManager.initialized = 0;
    g_FrameDeltaTimeSec = 0.0f;
    zClass_TypeList::FreeAll();
    zClass_TypeList::FreeAll();
    return 0;
}

extern "C" int zeffect_init_from_path_smoke(void) {
    const zEffect_RuntimeManager oldManager = g_zEffect_RuntimeManager;
    zArchiveList *const oldMountedList = g_zArchive_MountedList;
    zClass_TypeListLink *const oldHead = zClass_TypeList::Head(6);
    zClass_TypeListLink *const oldTail = zClass_TypeList::Tail(6);
    const int oldTexDirEntryCount = g_zImage_TexDirEntryCount;
    zVidTexturePackEntry *const oldBuiltinPacks = g_zVid_BuiltinTexturePacks;
    const int oldBuiltinPackCount = g_zVid_BuiltinTexturePackCount;

    char tempPath[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempPath), tempPath) == 0 ||
        GetTempFileNameA(tempPath, "zfx", 0, tempFile) == 0) {
        return 1;
    }

    HANDLE const file =
        CreateFileA(tempFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                    CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        DeleteFileA(tempFile);
        return 2;
    }

    WriteEffectTestU32(file, 0xaaaaaaaa);
    const std::uint32_t zrdOffset = SetFilePointer(file, 0, nullptr, FILE_CURRENT);
    WriteEffectTestZrdArray(file, 3);
    WriteEffectTestZrdString(file, "EFFECTS");
    WriteEffectTestZrdArray(file, 2);
    WriteEffectTestZrdArray(file, 10);
    WriteEffectTestZrdString(file, "fx_model");
    WriteEffectTestZrdString(file, "NAME");
    WriteEffectTestZrdString(file, "spark");
    WriteEffectTestZrdString(file, "MAPS");
    WriteEffectTestZrdArray(file, 1);
    WriteEffectTestZrdString(file, "SPEED");
    WriteEffectTestZrdFloat(file, 2.5f);
    WriteEffectTestZrdString(file, "LOOPING");
    WriteEffectTestZrdString(file, "ON");
    FlushFileBuffers(file);

    zZarFileRecord record = {};
    record.fileOffset = zrdOffset;
    record.fileSize = SetFilePointer(file, 0, nullptr, FILE_CURRENT) - zrdOffset;
    std::strcpy(record.name, "effect.zrd");
    zIndexArchive archive = {};
    archive.hFile = file;
    archive.recordCount = 1;
    archive.records = &record;
    zArchiveListNode archiveNode = {};
    archiveNode.payload = &archive;
    archiveNode.next = &archiveNode;
    archiveNode.prev = &archiveNode;
    zArchiveList mountedList = {};
    mountedList.count = 1;
    mountedList.head = &archiveNode;
    g_zArchive_MountedList = &mountedList;

    zClass_NodePartial worldNode = {};
    zClass_NodePartial cameraNode = {};
    zClass_NodePartial templateNode = {};
    zDiEntryPartial entry = {};
    zDiPartial di = {};
    zModel_MaterialPartial material = {};
    std::strcpy(templateNode.name, "fx_model");
    templateNode.userDataOrDiRef = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&material));
    di.entryCount = 1;
    di.entries = &entry;
    entry.material = &material;

    zClass_TypeListLink link = {&templateNode, nullptr, nullptr, 0};
    zClass_TypeList::Head(6) = &link;
    zClass_TypeList::Tail(6) = &link;
    g_zEffect_RuntimeManager = {};
    g_zImage_TexDirEntryCount = 0;
    std::FILE *const dummyBuiltinFile = std::tmpfile();
    if (dummyBuiltinFile == nullptr) {
        g_zEffect_RuntimeManager = oldManager;
        g_zArchive_MountedList = oldMountedList;
        zClass_TypeList::Head(6) = oldHead;
        zClass_TypeList::Tail(6) = oldTail;
        g_zImage_TexDirEntryCount = oldTexDirEntryCount;
        CloseHandle(file);
        return 3;
    }
    zVidTexturePackEntry builtinPack = {};
    builtinPack.fileHandle = dummyBuiltinFile;
    g_zVid_BuiltinTexturePacks = &builtinPack;
    g_zVid_BuiltinTexturePackCount = 1;

    const int result = zEffect::InitFromPath(&worldNode, &cameraNode, "C:\\dummy\\effect.zrd");
    zEffect_RuntimeEntry *const runtimeEntry = g_zEffect_RuntimeManager.templates;
    const bool ok =
        result == 0 && g_zEffect_RuntimeManager.initialized == 1 &&
        g_zEffect_RuntimeManager.templateCount == 1 &&
        g_zEffect_RuntimeManager.parentNode == &worldNode &&
        g_zEffect_RuntimeManager.listenerNode == &cameraNode &&
        g_zEffect_RuntimeManager.loadedTemplateTree != nullptr &&
        g_zEffect_RuntimeManager.freeList != nullptr && runtimeEntry != nullptr &&
        runtimeEntry[0].effectIndex == 0 && runtimeEntry[0].effectNode == &templateNode &&
        runtimeEntry[0].effectGfxData == &material &&
        std::strcmp(runtimeEntry[0].modelNodeName, "fx_model") == 0 &&
        std::strcmp(runtimeEntry[0].effectName, "spark") == 0 &&
        material.cycle != nullptr && material.cycle->frameCount == 0 &&
        material.cycle->framesPerSecond == 2.5f && material.cycle->loopEnabled == 1;

    if (g_zEffect_RuntimeManager.loadedTemplateTree != nullptr) {
        zReader::FreeLoadedTree(
            reinterpret_cast<zReader::Node *>(g_zEffect_RuntimeManager.loadedTemplateTree));
    }
    if (g_zEffect_RuntimeManager.templates != nullptr) {
        std::free(g_zEffect_RuntimeManager.templates);
    }
    if (g_zEffect_RuntimeManager.freeList != nullptr) {
        zArchiveList_Destroy(g_zEffect_RuntimeManager.freeList);
    }
    if (material.cycle != nullptr) {
        std::free(material.cycle->frameTable);
        std::free(material.cycle);
    }

    g_zEffect_RuntimeManager = oldManager;
    g_zArchive_MountedList = oldMountedList;
    zClass_TypeList::Head(6) = oldHead;
    zClass_TypeList::Tail(6) = oldTail;
    g_zImage_TexDirEntryCount = oldTexDirEntryCount;
    g_zVid_BuiltinTexturePacks = oldBuiltinPacks;
    g_zVid_BuiltinTexturePackCount = oldBuiltinPackCount;
    CloseHandle(file);
    return ok ? 0 : 3;
}
