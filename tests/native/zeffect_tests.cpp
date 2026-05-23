#include "Battlesport/HudSensorTracker.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zUtil/zZbd.h"
#include "zDi.h"

#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

namespace {
int g_hudSensorVisibleCallCount = 0;
int g_hudSensorLastVisible = -1;
int g_hudSensorDeleteCallCount = 0;
std::uint32_t g_hudSensorDeleteFlags = 0;
int g_effectAnimDispatchCallCount = 0;
zEffectAnimActivationRecord *g_effectAnimDispatchRecord = nullptr;
int g_effectAnimEventCallbackCallCount = 0;
zEffectAnimEntry *g_effectAnimEventCallbackSelf = nullptr;
void *g_effectAnimEventCallbackContext = nullptr;
std::int32_t g_effectAnimEventCallbackValue = 0;

template <typename Method> std::uintptr_t MethodAddress(Method method) {
    static_assert(sizeof(method) <= sizeof(std::uintptr_t));
    std::uintptr_t address = 0;
    std::memcpy(&address, &method, sizeof(method));
    return address;
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

void EnsureZrdrFreePool() {
    if (g_zUtil_ZRDR_FreePool == nullptr) {
        g_zUtil_ZRDR_FreePool = zArchiveList_CreateEmpty();
    }
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
    g_zSnd_IsInitialized = 0;
    g_zSnd_PreInitialized = 0;

    zSndSample sample = {};
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
        g_zSnd_IsInitialized = oldInitialized;
        g_zSnd_PreInitialized = oldPreInitialized;
        return 1;
    }

    event.nodeRefIndex = 1;
    event.offsetX = 4.0f;
    event.offsetY = 5.0f;
    event.offsetZ = 6.0f;
    const std::int32_t result = zEffect::HandleSampleRefOffsetEvent(&entry, &event);
    g_zSnd_IsInitialized = oldInitialized;
    g_zSnd_PreInitialized = oldPreInitialized;
    return result == 2 ? 0 : 2;
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
