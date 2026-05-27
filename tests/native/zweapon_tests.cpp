#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zUtil/zSaveGame.h"
#include "GameZRecoil/zUtil/zZbd.h"
#include "GameZRecoil/zWeapon/zWeapon.h"
#include "OptCatalog.h"
#include "zDi.h"

#include <windows.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace {
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

int g_TestDamageTimerCalls;
void *g_TestDamageTimerContext;
float g_TestDamageTimerDamage;
int g_TestHitCallbackCalls;
void *g_TestHitCallbackContext;
OptCatalogEntryDef *g_TestHitCallbackEntry;
OptCatalogHitEventPartial *g_TestHitCallbackEvent;
float g_TestHitCallbackDamage;
int g_TestFeedbackCallbackCalls;
OptCatalogDamageHandlerPartial *g_TestFeedbackHandler;
zClass_NodePartial *g_TestFeedbackHitNode;
float g_TestFeedbackDamage;
int g_TestImpactCallbackCalls;
OptCatalogEntryDef *g_TestImpactCallbackEntry;
OptCatalogHitEventPartial *g_TestImpactCallbackEvent;
OptCatalogRuntimeInstanceStorage *g_TestImpactCallbackRuntime;
zVec3 g_TestImpactCallbackHitPos;
zClass_NodePartial *g_TestImpactCallbackHitNode;
int g_TestImpactCallbackImpactSlot;
int g_TestCraterRelayCalls;
int g_TestCraterRelayResult;
int g_TestQSandRelayCalls;
int g_TestQSandRelayResult;
int g_TestRemoveRelayCalls;
OptCatalogEntryDef *g_TestRemoveRelayEntry;
zVec3 *g_TestRemoveRelayPoint;
zClass_NodePartial *g_TestRemoveRelayOwner;
int g_TestRuntimeUpdateCalls;
OptCatalogRuntimeInstanceStorage *g_TestRuntimeUpdateLast;
int g_TestAllocRuntimeGateCalls;
void **g_TestAllocRuntimeGateSaveStateSlot;
int g_TestLoadOptCatalogCallbackCalls;
zReader::Node *g_TestLoadOptCatalogCallbackNode;
OptCatalogEntryDef *g_TestLoadOptCatalogCallbackEntry;

bool TestFloatNear(float actual, float expected) {
    float delta = actual - expected;
    if (delta < 0.0f) {
        delta = -delta;
    }
    return delta < 0.0001f;
}

float RECOIL_FASTCALL TestDamageTimerCallback(void *context, float damageAmount) {
    ++g_TestDamageTimerCalls;
    g_TestDamageTimerContext = context;
    g_TestDamageTimerDamage = damageAmount;
    return damageAmount + *static_cast<float *>(context);
}

int RECOIL_FASTCALL TestOptCatalogHitCallback(void *context, OptCatalogEntryDef *entry,
                                              OptCatalogHitEventPartial *hitEvent,
                                              float damageAmount) {
    ++g_TestHitCallbackCalls;
    g_TestHitCallbackContext = context;
    g_TestHitCallbackEntry = entry;
    g_TestHitCallbackEvent = hitEvent;
    g_TestHitCallbackDamage = damageAmount;
    return 77;
}

void RECOIL_FASTCALL TestOptCatalogFeedbackCallback(OptCatalogDamageHandlerPartial *handler,
                                                    zClass_NodePartial *hitNode,
                                                    float damageAmount) {
    ++g_TestFeedbackCallbackCalls;
    g_TestFeedbackHandler = handler;
    g_TestFeedbackHitNode = hitNode;
    g_TestFeedbackDamage = damageAmount;
    g_OptCatalogDamageFeedbackIntensityScalar = 0.5f;
}

void RECOIL_FASTCALL TestOptCatalogImpactCallback(
    OptCatalogEntryDef *entry, OptCatalogHitEventPartial *hitEvent,
    OptCatalogRuntimeInstanceStorage *runtimeInstance) {
    ++g_TestImpactCallbackCalls;
    g_TestImpactCallbackEntry = entry;
    g_TestImpactCallbackEvent = hitEvent;
    g_TestImpactCallbackRuntime = runtimeInstance;
    g_TestImpactCallbackHitPos = hitEvent->hitPos;
    g_TestImpactCallbackHitNode = hitEvent->hitNode;
    g_TestImpactCallbackImpactSlot = hitEvent->surfaceRef != nullptr
                                         ? hitEvent->surfaceRef->impactSlot
                                         : -1;
}

int RECOIL_FASTCALL TestOptCatalogCraterRelayCallback(void *) {
    ++g_TestCraterRelayCalls;
    return g_TestCraterRelayResult;
}

int RECOIL_FASTCALL TestOptCatalogQSandRelayCallback(void *) {
    ++g_TestQSandRelayCalls;
    return g_TestQSandRelayResult;
}

void RECOIL_FASTCALL TestOptCatalogRemoveRuntimeRelayCallback(OptCatalogEntryDef *entry,
                                                              zVec3 *pointOrVec3,
                                                              zClass_NodePartial *ownerNode) {
    ++g_TestRemoveRelayCalls;
    g_TestRemoveRelayEntry = entry;
    g_TestRemoveRelayPoint = pointOrVec3;
    g_TestRemoveRelayOwner = ownerNode;
}

void RECOIL_FASTCALL
TestOptCatalogRuntimeUpdateCallback(OptCatalogRuntimeInstanceStorage *runtimeInstance) {
    ++g_TestRuntimeUpdateCalls;
    g_TestRuntimeUpdateLast = runtimeInstance;
}

int RECOIL_FASTCALL TestOptCatalogAllocRuntimeGateCallback(OptCatalogEntryDef *,
                                                           void **saveStateSlot) {
    ++g_TestAllocRuntimeGateCalls;
    g_TestAllocRuntimeGateSaveStateSlot = saveStateSlot;
    return 0;
}

void RECOIL_FASTCALL TestLoadOptCatalogEntryCallback(zReader::Node *entryNode,
                                                     OptCatalogEntryDef *entry) {
    ++g_TestLoadOptCatalogCallbackCalls;
    g_TestLoadOptCatalogCallbackNode = entryNode;
    g_TestLoadOptCatalogCallbackEntry = entry;
}

bool WriteU32(HANDLE file, std::uint32_t value) {
    DWORD written = 0;
    return WriteFile(file, &value, sizeof(value), &written, nullptr) != 0 &&
           written == sizeof(value);
}

bool WriteBytes(HANDLE file, const void *data, std::uint32_t size) {
    DWORD written = 0;
    return WriteFile(file, data, size, &written, nullptr) != 0 && written == size;
}

bool WriteZrdString(HANDLE file, const char *value) {
    return WriteU32(file, zReader::ZRDR_NODE_STRING) &&
           WriteU32(file, static_cast<std::uint32_t>(std::strlen(value) + 1)) &&
           WriteBytes(file, value, static_cast<std::uint32_t>(std::strlen(value) + 1));
}

bool WriteZrdInt(HANDLE file, int value) {
    return WriteU32(file, zReader::ZRDR_NODE_INT) &&
           WriteU32(file, static_cast<std::uint32_t>(value));
}

bool WriteZrdFloat(HANDLE file, float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return WriteU32(file, zReader::ZRDR_NODE_FLOAT) && WriteU32(file, bits);
}

bool WriteZrdArrayHeader(HANDLE file, int count) {
    return WriteU32(file, zReader::ZRDR_NODE_ARRAY) &&
           WriteU32(file, static_cast<std::uint32_t>(count));
}

extern "C" int zweapon_optcatalog_blend_direction_toward_target_smoke(void) {
    zVec3 direction = {3.0f, 0.0f, 0.0f};
    zVec3 target = {3.0f, 4.0f, 9.0f};

    OptCatalog::BlendDirectionTowardTarget(&direction, &target, 1.0f, 1.0f, 0.0f);
    if (!TestFloatNear(direction.x, 0.6f) || !TestFloatNear(direction.y, 0.8f) ||
        !TestFloatNear(direction.z, 0.0f)) {
        return 1;
    }

    direction = {0.0f, 0.0f, 0.0f};
    target = {4.0f, 8.0f, 12.0f};
    OptCatalog::BlendDirectionTowardTarget(&direction, &target, 0.0f, 0.0f, 0.0f);
    return direction.x == 0.0f && direction.y == 0.0f && direction.z == 0.0f ? 0 : 2;
}

void MakeReaderNameNode(zReader::Node &node, const char *name) {
    node.type = zReader::ZRDR_NODE_STRING;
    node.value.str = const_cast<char *>(name);
}

void MakeReaderStringValueNode(zReader::Node &node, const char *value) {
    node.type = zReader::ZRDR_NODE_STRING;
    node.value.str = const_cast<char *>(value);
}

void MakeReaderIntValueNode(zReader::Node &node, int value) {
    node.type = zReader::ZRDR_NODE_INT;
    node.value.i32 = value;
}

void MakeReaderArrayPayload(zReader::Node *payload, int count) {
    payload[0].type = zReader::ZRDR_NODE_INT;
    payload[0].value.i32 = count;
}

void MakeReaderArrayNode(zReader::Node &node, zReader::Node *payload, int count) {
    MakeReaderArrayPayload(payload, count);
    node.type = zReader::ZRDR_NODE_ARRAY;
    node.value.nodes = payload;
}

void MakeReaderStringArrayNode(zReader::Node &node, zReader::Node *payload, const char *value) {
    MakeReaderArrayNode(node, payload, 2);
    MakeReaderStringValueNode(payload[1], value);
}

struct TestDamageHealthOverlay {
    unsigned char unknown_00[0x7c];
    float health;
};
} // namespace

extern "C" int zweapon_init_smoke(void) {
    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager = MakeManager(sentinel);
    g_zUtil_ZbdManager = &manager;
    g_zWeapon_ZarHandlerRegistered = 1;

    g_OptCatalog_FallbackImpactProbeEnabled = 0;
    g_OptCatalog_CaptureHitSnapshotEnabled = 0;
    g_OptCatalog_EntryCount = 9;
    g_OptCatalog_EntryTable = reinterpret_cast<OptCatalogEntryDef *>(0x1111);
    g_OptCatalogRuntimeInstanceCount = 8;
    g_OptCatalogRuntimeInstancePool = reinterpret_cast<void *>(0x2222);
    g_OptCatalogFreeRuntimeInstanceList = reinterpret_cast<void *>(0x3333);
    g_OptCatalogRuntimeWorld = reinterpret_cast<zClass_NodePartial *>(0x4444);
    g_OptCatalogPendingSpawnTargetCountPtr = reinterpret_cast<void *>(0x5555);
    g_OptCatalogPendingSpawnTargetListPtr = reinterpret_cast<void *>(0x6666);
    g_OptCatalogQueuedImpactCount = 7;
    g_OptCatalog_DamageContextKind = 6;
    g_OptCatalog_DamageContextHitEvent = reinterpret_cast<void *>(0x7777);
    g_OptCatalogDamageFeedbackCallback = reinterpret_cast<void *>(0x8888);
    g_OptCatalog_DamageFeedbackHitCount = 5;
    g_OptCatalogDamageFeedbackTrackedNode = reinterpret_cast<zClass_NodePartial *>(0x9999);

    if (zWepInit() != 0) {
        return 1;
    }

    const bool resetOk =
        g_OptCatalog_FallbackImpactProbeEnabled == 1 &&
        g_OptCatalog_CaptureHitSnapshotEnabled == 1 && g_OptCatalog_EntryCount == 0 &&
        g_OptCatalog_EntryTable == nullptr && g_OptCatalogRuntimeInstanceCount == 0 &&
        g_OptCatalogRuntimeInstancePool == nullptr &&
        g_OptCatalogFreeRuntimeInstanceList == nullptr && g_OptCatalogRuntimeWorld == nullptr &&
        g_OptCatalogPendingSpawnTargetCountPtr == nullptr &&
        g_OptCatalogPendingSpawnTargetListPtr == nullptr && g_OptCatalogMaxCraterRadius == 30.0f &&
        g_OptCatalogQueuedImpactCount == 0 && g_OptCatalog_DamageContextKind == 0 &&
        g_OptCatalog_DamageContextHitEvent == nullptr && g_zWeapon_MaxTetherAltitude == 30.0f &&
        g_OptCatalogDamageFeedbackCallback == nullptr &&
        g_OptCatalogLockOnWarningGateTimeSec == 0.0f && g_OptCatalog_DamageFeedbackHitCount == 0 &&
        g_OptCatalogDamageFeedbackTrackedNode == nullptr && g_OptCatalogNextSpawnScale == 1.0f;

    const bool registerOk =
        manager.sectionHandlerCount == 1 && sentinel.next != &sentinel &&
        sentinel.next->sectionHandler.sectionName == g_zWeapon_ArchiveName &&
        sentinel.next->sectionHandler.onPreLoad != nullptr &&
        sentinel.next->sectionHandler.onDataReady != nullptr &&
        sentinel.next->sectionHandler.sortOrder == 0x3e8 &&
        sentinel.next->sectionHandler.userData == nullptr;

    ClearRegisteredHandlers(sentinel);
    g_zUtil_ZbdManager = nullptr;
    if (!resetOk || !registerOk) {
        return 2;
    }

    g_zWeapon_ZarHandlerRegistered = 0;
    manager = MakeManager(sentinel);
    g_zUtil_ZbdManager = &manager;
    zWepInit();
    const bool skipOk = manager.sectionHandlerCount == 0;
    ClearRegisteredHandlers(sentinel);
    g_zUtil_ZbdManager = nullptr;
    g_zWeapon_ZarHandlerRegistered = 1;
    return skipOk ? 0 : 3;
}

extern "C" int zweapon_optcatalog_load_kill_verb_string_smoke(void) {
    zReader::Node killVerbPayload[2] = {};
    zReader::Node entryPayload[3] = {};
    zReader::Node entryNode = {};
    const char *const longVerb = "abcdefghijklmnopqrstuv";

    MakeReaderNameNode(entryPayload[1], "KILL_VERB");
    MakeReaderStringArrayNode(entryPayload[2], killVerbPayload, longVerb);
    MakeReaderArrayNode(entryNode, entryPayload, 3);

    OptCatalogEntryDef entry = {};
    zWeapon_OptCatalog::LoadKillVerbString(&entryNode, &entry);

    const bool copiedAndTerminated =
        entry.killVerbString != nullptr &&
        std::strncmp(entry.killVerbString, "abcdefghijklmnopqrs", 19) == 0 &&
        entry.killVerbString[19] == '\0';

    std::free(entry.killVerbString);
    return copiedAndTerminated ? 0 : 1;
}

extern "C" int zweapon_load_opt_catalog_from_path_smoke(void) {
    char tempDir[MAX_PATH] = {};
    char tempPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zwp", 0, tempPath) == 0) {
        return 1;
    }

    HANDLE file = CreateFileA(tempPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        DeleteFileA(tempPath);
        return 2;
    }

    WriteU32(file, 0xaaaaaaaa);
    WriteU32(file, 0xbbbbbbbb);
    const std::uint32_t zrdOffset =
        static_cast<std::uint32_t>(SetFilePointer(file, 0, nullptr, FILE_CURRENT));

    bool writeOk = true;
    writeOk = writeOk && WriteZrdArrayHeader(file, 9);
    writeOk = writeOk && WriteZrdString(file, "VERSION") && WriteZrdInt(file, 2);
    writeOk = writeOk && WriteZrdString(file, "BASIC");
    writeOk = writeOk && WriteZrdArrayHeader(file, 21);
    writeOk = writeOk && WriteZrdString(file, "NAME") && WriteZrdString(file, "Basic weapon");
    writeOk = writeOk && WriteZrdString(file, "DESC") && WriteZrdString(file, "BASIC_DESC");
    writeOk =
        writeOk && WriteZrdString(file, "MILITARY_NAME") && WriteZrdString(file, "BASIC_MIL");
    writeOk = writeOk && WriteZrdString(file, "AMMO_LIMIT") && WriteZrdInt(file, 12);
    writeOk = writeOk && WriteZrdString(file, "RANGE") && WriteZrdFloat(file, 250.0f);
    writeOk = writeOk && WriteZrdString(file, "VELOCITY") && WriteZrdFloat(file, 50.0f);
    writeOk = writeOk && WriteZrdString(file, "FIRE_RATE") && WriteZrdFloat(file, 4.0f);
    writeOk = writeOk && WriteZrdString(file, "DAMAGE") && WriteZrdFloat(file, 0.75f);
    writeOk = writeOk && WriteZrdString(file, "CRATER") && WriteZrdArrayHeader(file, 3) &&
              WriteZrdInt(file, 4) && WriteZrdInt(file, 9);
    writeOk = writeOk && WriteZrdString(file, "FREEZE") && WriteZrdArrayHeader(file, 7) &&
              WriteZrdFloat(file, 2.0f) && WriteZrdFloat(file, 5.0f) &&
              WriteZrdFloat(file, 1.5f) && WriteZrdFloat(file, 0.25f) &&
              WriteZrdFloat(file, 0.5f) && WriteZrdFloat(file, 0.75f);
    writeOk =
        writeOk && WriteZrdString(file, "MAX_CRATER_RADIUS") && WriteZrdFloat(file, 40.0f);
    writeOk = writeOk && WriteZrdString(file, "BALLISTICS") && WriteZrdArrayHeader(file, 3) &&
              WriteZrdString(file, "BASIC") && WriteZrdInt(file, 0);
    FlushFileBuffers(file);

    zZarFileRecord record = {};
    record.fileOffset = zrdOffset;
    record.fileSize =
        static_cast<std::uint32_t>(SetFilePointer(file, 0, nullptr, FILE_CURRENT)) - zrdOffset;
    std::strcpy(record.name, "weapons.zrd");

    zIndexArchive archive = {};
    archive.hFile = file;
    archive.recordCount = 1;
    archive.records = &record;

    zArchiveListNode node = {};
    node.payload = &archive;
    node.next = &node;
    node.prev = &node;

    zArchiveList list = {};
    list.count = 1;
    list.head = &node;

    zArchiveList *const oldMountedList = g_zArchive_MountedList;
    const int oldRendererStringCount = g_zRndr_GlobalStringCount;
    g_zArchive_MountedList = &list;
    g_zRndr_GlobalStringCount = 0;
    g_zWeapon_ZarHandlerRegistered = 0;
    zWepInit();

    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;
    zClass_NodeFreeListSlot slots[40] = {};
    for (int i = 0; i < 39; ++i) {
        slots[i].freeTag = i + 1;
    }
    slots[39].freeTag = 0x00ffffff;
    g_zClass_NodeArray = slots;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial worldNode = {};
    g_TestLoadOptCatalogCallbackCalls = 0;
    g_TestLoadOptCatalogCallbackNode = nullptr;
    g_TestLoadOptCatalogCallbackEntry = nullptr;
    const int loadResult = writeOk ? zWeapon::LoadOptCatalogFromPath(
                                         &worldNode, "C:\\dummy\\weapons.zrd", 42,
                                         TestLoadOptCatalogEntryCallback)
                                   : -99;

    const OptCatalogEntryDef *const entry = g_OptCatalog_EntryTable;
    const bool ok = loadResult == 0 && g_OptCatalogLoadedTreeRoot != nullptr &&
                    g_OptCatalogRuntimeWorld == &worldNode && g_OptCatalogNetworkOptionState == 42 &&
                    g_OptCatalog_EntryCount == 1 && entry != nullptr &&
                    g_TestLoadOptCatalogCallbackCalls == 1 &&
                    g_TestLoadOptCatalogCallbackNode != nullptr &&
                    g_TestLoadOptCatalogCallbackEntry == entry &&
                    std::strcmp(entry->keyName, "BASIC") == 0 &&
                    std::strcmp(entry->displayName, "Basic weapon") == 0 &&
                    std::strcmp(entry->description, "BASIC_DESC") == 0 &&
                    std::strcmp(entry->militaryName, "BASIC_MIL") == 0 &&
                    entry->ordinalIndex == 0 && entry->ammoOrChargeMax == 12.0f &&
                    entry->range == 250.0f && entry->rangeSq == 62500.0f &&
                    entry->velocity == 50.0f && entry->fireRateInterval == 0.25f &&
                    entry->damage == 0.75f && entry->craterRadiusBase == 4 &&
                    entry->craterRadiusRandomRange == 5 &&
                    entry->timedStatusLightRangeMin == 2.0f &&
                    entry->timedStatusLightRangeMax == 5.0f &&
                    entry->timedStatusUpdateDelay == 1.5f &&
                    entry->timedStatusLightSpecularColor.red == 0.25f &&
                    entry->timedStatusLightSpecularColor.green == 0.5f &&
                    entry->timedStatusLightSpecularColor.blue == 0.75f &&
                    g_OptCatalogMaxCraterRadius == 40.0f &&
                    g_OptCatalogRuntimeInstanceCount == 21 &&
                    g_OptCatalogRuntimeInstancePool != nullptr &&
                    g_OptCatalogFreeRuntimeInstanceList != nullptr;

    OptCatalog::Shutdown();
    zClass_TypeList::FreeAll();
    g_zClass_NodeArray = nullptr;
    g_zClass_NodeFreeHeadIndex = -1;
    g_zClass_ActiveNodeCount = 0;
    g_zArchive_MountedList = oldMountedList;
    g_zRndr_GlobalStringCount = oldRendererStringCount;
    g_zWeapon_ZarHandlerRegistered = 1;
    CloseHandle(file);
    DeleteFileA(tempPath);
    return ok ? 0 : 3;
}

extern "C" int zweapon_zar_weapons_section_callbacks_smoke(void) {
    char tempPath[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    GetTempPathA(sizeof(tempPath), tempPath);
    GetTempFileNameA(tempPath, "zwe", 0, tempFile);

    HANDLE const file =
        CreateFileA(tempFile, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return 1;
    }

    zZbdManager manager = {};
    manager.indexArchive.hFile = file;

    zZbdSectionHandler handler = {};
    handler.sectionName = "Weapons";
    zZbdSectionCallbackCtx callbackCtx = {};
    callbackCtx.manager = &manager;
    callbackCtx.sectionHandler = &handler;

    g_OptCatalog_DamageFeedbackHitCount = 37;
    if (zWeapon::OnWeaponsSectionPreLoad(&callbackCtx, nullptr) != 1) {
        CloseHandle(file);
        return 2;
    }

    SetFilePointer(file, 0, nullptr, FILE_BEGIN);
    int readBack = 0;
    DWORD read = 0;
    ReadFile(file, &readBack, sizeof(readBack), &read, nullptr);

    const bool writeOk = manager.indexArchive.recordCount == 1 &&
                         manager.indexArchive.recordCapacity == 2 &&
                         manager.indexArchive.records != nullptr &&
                         std::strcmp(manager.indexArchive.records[0].name,
                                     "Weapons/WeaponData") == 0 &&
                         manager.indexArchive.records[0].fileSize == sizeof(readBack) &&
                         read == sizeof(readBack) && readBack == 37;

    int restoredHitCount = 11;
    g_OptCatalogLockOnWarningGateTimeSec = 12.5f;
    g_OptCatalog_DamageFeedbackHitCount = 0;
    zWeapon::OnWeaponsSectionDataReady(&callbackCtx, "WeaponData", &restoredHitCount,
                                       sizeof(restoredHitCount), nullptr);
    const bool readOk = g_OptCatalogLockOnWarningGateTimeSec == 0.0f &&
                        g_OptCatalog_DamageFeedbackHitCount == 11;

    std::free(manager.indexArchive.records);
    manager.indexArchive.records = nullptr;
    CloseHandle(file);
    return writeOk && readOk ? 0 : 3;
}

extern "C" int zweapon_optcatalog_shutdown_smoke(void) {
    g_OptCatalog_EntryCount = 1;
    g_OptCatalog_EntryTable =
        static_cast<OptCatalogEntryDef *>(std::calloc(1, sizeof(OptCatalogEntryDef)));
    g_OptCatalog_EntryTable[0].description = static_cast<char *>(std::malloc(4));
    g_OptCatalog_EntryTable[0].militaryName = static_cast<char *>(std::malloc(4));
    g_OptCatalog_EntryTable[0].impactFxTable =
        static_cast<OptCatalogFxSpec *>(std::malloc(4));
    g_OptCatalog_EntryTable[0].killVerbString = static_cast<char *>(std::malloc(4));
    g_OptCatalogRuntimeInstanceCount = 3;
    g_OptCatalogRuntimeInstancePool = std::malloc(4);
    g_OptCatalogFreeRuntimeInstanceList = reinterpret_cast<void *>(0x1234);
    g_OptCatalogRuntimeWorld = reinterpret_cast<zClass_NodePartial *>(0x5678);
    g_OptCatalogPendingSpawnTargetCountPtr = reinterpret_cast<void *>(0x9abc);
    g_OptCatalogPendingSpawnTargetListPtr = reinterpret_cast<void *>(0xdef0);
    g_OptCatalog_FallbackImpactProbeEnabled = 0;
    g_OptCatalog_CaptureHitSnapshotEnabled = 0;
    g_OptCatalogQueuedImpactCount = 9;
    g_OptCatalog_DamageFeedbackHitCount = 8;
    g_OptCatalogThermalGlowFreeList = nullptr;
    g_OptCatalogLoadedTreeRoot = nullptr;

    if (OptCatalog::Shutdown() != 0) {
        return 1;
    }

    return g_OptCatalog_EntryCount == 0 && g_OptCatalog_EntryTable == nullptr &&
                   g_OptCatalogRuntimeInstanceCount == 0 &&
                   g_OptCatalogRuntimeInstancePool == nullptr &&
                   g_OptCatalogFreeRuntimeInstanceList == nullptr &&
                   g_OptCatalogRuntimeWorld == nullptr &&
                   g_OptCatalogPendingSpawnTargetCountPtr == nullptr &&
                   g_OptCatalogPendingSpawnTargetListPtr == nullptr &&
                   g_OptCatalog_FallbackImpactProbeEnabled == 1 &&
                   g_OptCatalog_CaptureHitSnapshotEnabled == 1 &&
                   g_OptCatalogQueuedImpactCount == 0 && g_OptCatalog_DamageFeedbackHitCount == 0
               ? 0
               : 2;
}

extern "C" int zweapon_set_max_tether_altitude_smoke(void) {
    const float oldMaxTetherAltitude = g_zWeapon_MaxTetherAltitude;

    zWeapon::SetMaxTetherAltitude(47.5f);
    const bool firstOk = g_zWeapon_MaxTetherAltitude == 47.5f;

    zWeapon::SetMaxTetherAltitude(-3.25f);
    const bool secondOk = g_zWeapon_MaxTetherAltitude == -3.25f;

    g_zWeapon_MaxTetherAltitude = oldMaxTetherAltitude;
    return firstOk && secondOk ? 0 : 1;
}

extern "C" int zweapon_optcatalog_find_entry_by_id_smoke(void) {
    const std::int32_t oldCount = g_OptCatalog_EntryCount;
    OptCatalogEntryDef *const oldTable = g_OptCatalog_EntryTable;

    OptCatalogEntryDef entries[3] = {};
    entries[0].keyName = const_cast<char *>("cannon");
    entries[0].ordinalIndex = 11;
    entries[1].keyName = nullptr;
    entries[1].ordinalIndex = 12;
    entries[2].keyName = const_cast<char *>("rocket");
    entries[2].ordinalIndex = 13;

    g_OptCatalog_EntryTable = entries;
    g_OptCatalog_EntryCount = 3;

    const bool found = OptCatalog::FindEntryById(13) == &entries[2];
    const bool nullKeySkipped = OptCatalog::FindEntryById(12) == nullptr;
    const bool missing = OptCatalog::FindEntryById(99) == nullptr;

    g_OptCatalog_EntryCount = 0;
    const bool empty = OptCatalog::FindEntryById(11) == nullptr;

    g_OptCatalog_EntryCount = oldCount;
    g_OptCatalog_EntryTable = oldTable;

    return found && nullKeySkipped && missing && empty ? 0 : 1;
}

extern "C" int zweapon_optcatalog_find_entry_by_name_smoke(void) {
    const std::int32_t oldCount = g_OptCatalog_EntryCount;
    OptCatalogEntryDef *const oldTable = g_OptCatalog_EntryTable;

    OptCatalogEntryDef entries[4] = {};
    entries[0].keyName = const_cast<char *>("cannon");
    entries[1].keyName = nullptr;
    entries[2].keyName = const_cast<char *>("rocket");
    entries[3].keyName = const_cast<char *>("rocket");

    g_OptCatalog_EntryTable = entries;
    g_OptCatalog_EntryCount = 4;

    const bool foundFirst = OptCatalog::FindEntryByName("rocket") == &entries[2];
    const bool nullKeySkipped = OptCatalog::FindEntryByName("cannon") == &entries[0];
    const bool missing = OptCatalog::FindEntryByName("mine") == nullptr;

    g_OptCatalog_EntryCount = 0;
    const bool empty = OptCatalog::FindEntryByName("cannon") == nullptr;

    g_OptCatalog_EntryCount = oldCount;
    g_OptCatalog_EntryTable = oldTable;

    return foundFirst && nullKeySkipped && missing && empty ? 0 : 1;
}

extern "C" int zweapon_optcatalog_create_trail_segment_node_smoke(void) {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slots[2] = {};
    slots[0].freeTag = 1;
    slots[1].freeTag = 0x00ffffff;
    g_zClass_NodeArray = slots;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial templateNode = {};
    zClass_NodePartial *const parent =
        OptCatalog::CreateTrailSegmentNodeFromTemplate(&templateNode);

    const bool attachedOk = parent == &slots[0].node && parent->classId == 5 &&
                            (parent->flags & 0x04) != 0 && parent->listCountB == 1 &&
                            parent->listB[0] == &templateNode && templateNode.listCountA == 1 &&
                            templateNode.listA[0] == parent;

    zClass_Object3D::DeleteNode(parent);
    zClass_TypeList::FreeAll();

    slots[0].freeTag = 0x00ffffff;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    zClass_NodePartial *const noTemplate =
        OptCatalog::CreateTrailSegmentNodeFromTemplate(nullptr);
    const bool noTemplateOk = noTemplate == &slots[0].node && noTemplate->classId == 5 &&
                              (noTemplate->flags & 0x04) != 0 &&
                              noTemplate->listCountB == 0;

    zClass_Object3D::DeleteNode(noTemplate);
    zClass_TypeList::FreeAll();
    g_zClass_NodeArray = nullptr;
    g_zClass_NodeFreeHeadIndex = -1;
    return attachedOk && noTemplateOk ? 0 : 1;
}

extern "C" int zweapon_optcatalog_create_trail_runtime_state_smoke(void) {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slots[10] = {};
    for (int i = 0; i < 9; ++i) {
        slots[i].freeTag = i + 1;
    }
    slots[9].freeTag = 0x00ffffff;
    g_zClass_NodeArray = slots;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial runtimeWorld = {};
    runtimeWorld.classId = 3;
    zClass_NodePartial templateNode = {};
    zClass_NodePartial projectileNode = {};
    zTag4Partial variantTag = {};
    zVec3 spawnPos = {1.0f, 2.0f, 3.0f};
    zVec3 spawnDir = {4.0f, 5.0f, 6.0f};
    OptCatalogEntryDef entry = {};
    entry.attachCloneTemplateNode = &templateNode;

    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    g_OptCatalogRuntimeWorld = &runtimeWorld;

    OptCatalogTrailRuntimeState *const runtime = OptCatalog::CreateTrailRuntimeState(
        &entry, &projectileNode, &variantTag, nullptr, &spawnPos, &spawnDir, 10);
    const bool clampedOk =
        runtime != nullptr && runtime->ownerEntry == &entry &&
        runtime->projectileNode == &projectileNode && runtime->variantTagPtr == &variantTag &&
        runtime->spawnPos == &spawnPos && runtime->spawnDir == &spawnDir &&
        runtime->activeNodeSlotCount == 8 && runtime->activeNodeSlots[0].node != nullptr &&
        runtime->activeNodeSlots[7].node != nullptr &&
        std::strcmp(runtime->activeNodeSlots[0].node->name, "BeamReflect_0") == 0 &&
        std::strcmp(runtime->activeNodeSlots[7].node->name, "BeamReflect_7") == 0 &&
        (runtime->activeNodeSlots[0].node->flags & 0x04) == 0 &&
        runtimeWorld.listCountB == 8 && templateNode.listCountA == 8;

    std::free(runtime);
    std::free(runtimeWorld.listB);
    std::free(templateNode.listA);
    runtimeWorld = {};
    runtimeWorld.classId = 3;
    templateNode = {};

    entry.flags = 0x800;
    OptCatalogTrailRuntimeState *const single = OptCatalog::CreateTrailRuntimeState(
        &entry, &projectileNode, &variantTag, nullptr, &spawnPos, &spawnDir, 4);
    const bool singleSegmentOk =
        single != nullptr && single->activeNodeSlotCount == 1 &&
        single->activeNodeSlots[0].node != nullptr && runtimeWorld.listCountB == 1 &&
        std::strcmp(single->activeNodeSlots[0].node->name, "BeamReflect_0") == 0;

    std::free(single);
    std::free(runtimeWorld.listB);
    std::free(templateNode.listA);
    zClass_TypeList::FreeAll();
    g_zClass_NodeArray = nullptr;
    g_zClass_NodeFreeHeadIndex = -1;
    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    if (!clampedOk) {
        return 1;
    }
    return singleSegmentOk ? 0 : 2;
}

extern "C" int zweapon_optcatalog_mine_iterator_smoke(void) {
    OptCatalogRuntimeInstanceStorage first = {};
    OptCatalogRuntimeInstanceStorage second = {};
    OptCatalogEntryDef entry = {};
    first.next = &second;
    entry.activeRuntimeListHead = &first;

    OptCatalogRuntimeInstanceStorage *const oldCursor = g_OptCatalog_MineIteratorCursor;
    g_OptCatalog_MineIteratorCursor = nullptr;

    const bool beginOk =
        OptCatalog::MineIterator_Begin(&entry) == &first &&
        g_OptCatalog_MineIteratorCursor == &first;
    const bool nextOk =
        OptCatalog::MineIterator_Next() == &second &&
        g_OptCatalog_MineIteratorCursor == &second;
    const bool endOk =
        OptCatalog::MineIterator_Next() == nullptr &&
        g_OptCatalog_MineIteratorCursor == nullptr &&
        OptCatalog::MineIterator_Next() == nullptr;

    g_OptCatalog_MineIteratorCursor = oldCursor;
    return beginOk && nextOk && endOk ? 0 : 1;
}

extern "C" int zweapon_optcatalog_pending_spawn_override_smoke(void) {
    std::int32_t pendingCount = 3;
    void *pendingList = &pendingCount;

    OptCatalog::SetPendingSpawnTargetOverrides(&pendingCount, &pendingList);
    const bool setOk = g_OptCatalogPendingSpawnTargetCountPtr == &pendingCount &&
                       g_OptCatalogPendingSpawnTargetListPtr == &pendingList;

    OptCatalog::SetPendingSpawnTargetOverrides(nullptr, nullptr);
    const bool resetOk = g_OptCatalogPendingSpawnTargetCountPtr == nullptr &&
                         g_OptCatalogPendingSpawnTargetListPtr == nullptr;

    return setOk && resetOk ? 0 : 1;
}

extern "C" int zweapon_optcatalog_load_fx_spec_from_reader_node_smoke(void) {
    zEffect_RuntimeManager oldRuntimeManager = g_zEffect_RuntimeManager;
    const short oldAnimEntryCount = g_zEffectAnim_EntryCount;
    zEffectAnimEntry *const oldAnimEntryList = g_zEffectAnim_EntryList;
    const int oldSndInitialized = g_zSnd_IsInitialized;
    const int oldSndActiveBackend = g_zSnd_ActiveBackend;
    const zSndSampleSetRegistry oldSampleRegistry = g_zSnd_SampleSetRegistry;
    zClass_TypeListLink *const oldTypeHead = zClass_TypeList::Head(6);
    zClass_TypeListLink *const oldTypeTail = zClass_TypeList::Tail(6);
    const int oldPendingDirty = zClass_TypeList::PendingRemovalDirty(6);

    zEffect_RuntimeEntry effectTemplates[2] = {};
    effectTemplates[0].effectName = const_cast<char *>("spark");
    effectTemplates[1].effectName = const_cast<char *>("blast");
    g_zEffect_RuntimeManager = {};
    g_zEffect_RuntimeManager.templateCount = 2;
    g_zEffect_RuntimeManager.templates = effectTemplates;

    zEffectAnimEntry animEntries[4] = {};
    std::strcpy(animEntries[1].name, "attach");
    std::strcpy(animEntries[2].name, "model");
    std::strcpy(animEntries[3].name, "main");
    g_zEffectAnim_EntryCount = 4;
    g_zEffectAnim_EntryList = animEntries;

    zSndSample samples[3] = {};
    samples[0].replayFields.sampleId = "boom";
    samples[0].primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(0x1111);
    samples[1].replayFields.sampleId = "clank";
    samples[1].primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(0x2222);
    samples[2].replayFields.sampleId = "ring";
    samples[2].primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(0x3333);
    zSndSampleSet sampleSet = {};
    sampleSet.sampleCount = 3;
    sampleSet.samples = samples;
    zSndSampleSet *sampleSetSlots[1] = {&sampleSet};
    g_zSnd_SampleSetRegistry.begin = sampleSetSlots;
    g_zSnd_SampleSetRegistry.end = sampleSetSlots + 1;
    g_zSnd_SampleSetRegistry.capacityEnd = sampleSetSlots + 1;
    g_zSnd_IsInitialized = 1;
    g_zSnd_ActiveBackend = 0;

    zClass_NodePartial modelNode = {};
    std::strcpy(modelNode.name, "modelNode");
    zClass_TypeListLink modelLink = {};
    modelLink.node = &modelNode;
    zClass_TypeList::Head(6) = &modelLink;
    zClass_TypeList::Tail(6) = &modelLink;
    zClass_TypeList::PendingRemovalDirty(6) = 0;

    zReader::Node effectValue[2] = {};
    zReader::Node ignoredModelValue[2] = {};
    zReader::Node attachedValue[2] = {};
    zReader::Node modelAnimValue[2] = {};
    zReader::Node animValue[2] = {};
    zReader::Node randomRotateValue[2] = {};
    zReader::Node soundValue[3] = {};
    zReader::Node bounceSoundValue[2] = {};

    zReader::Node primaryFields[17] = {};
    MakeReaderArrayPayload(primaryFields, 17);
    MakeReaderNameNode(primaryFields[1], "EFFECT");
    MakeReaderStringArrayNode(primaryFields[2], effectValue, "blast");
    MakeReaderNameNode(primaryFields[3], "MODEL");
    MakeReaderStringArrayNode(primaryFields[4], ignoredModelValue, "modelNode");
    MakeReaderNameNode(primaryFields[5], "ANIMATION_ATTACHED");
    MakeReaderStringArrayNode(primaryFields[6], attachedValue, "attach");
    MakeReaderNameNode(primaryFields[7], "MODEL_ANIMATION");
    MakeReaderStringArrayNode(primaryFields[8], modelAnimValue, "model");
    MakeReaderNameNode(primaryFields[9], "ANIMATION");
    MakeReaderStringArrayNode(primaryFields[10], animValue, "main");
    MakeReaderNameNode(primaryFields[11], "RANDOM_ROTATE");
    MakeReaderArrayNode(primaryFields[12], randomRotateValue, 2);
    MakeReaderIntValueNode(randomRotateValue[1], 1);
    MakeReaderNameNode(primaryFields[13], "SOUND");
    MakeReaderArrayNode(primaryFields[14], soundValue, 3);
    MakeReaderStringValueNode(soundValue[1], "boom");
    MakeReaderStringValueNode(soundValue[2], "clank");
    MakeReaderNameNode(primaryFields[15], "BOUNCE_SOUND");
    MakeReaderStringArrayNode(primaryFields[16], bounceSoundValue, "ring");

    zReader::Node modelOnlyValue[2] = {};
    zReader::Node modelOnlyFields[3] = {};
    MakeReaderArrayPayload(modelOnlyFields, 3);
    MakeReaderNameNode(modelOnlyFields[1], "MODEL");
    MakeReaderStringArrayNode(modelOnlyFields[2], modelOnlyValue, "modelNode");

    zReader::Node rootFields[5] = {};
    MakeReaderArrayPayload(rootFields, 5);
    MakeReaderNameNode(rootFields[1], "PRIMARY");
    rootFields[2].type = zReader::ZRDR_NODE_ARRAY;
    rootFields[2].value.nodes = primaryFields;
    MakeReaderNameNode(rootFields[3], "MODEL_ONLY");
    rootFields[4].type = zReader::ZRDR_NODE_ARRAY;
    rootFields[4].value.nodes = modelOnlyFields;
    zReader::Node root = {};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = rootFields;

    OptCatalogFxSpec spec = {};
    spec.flags = 0xfe;
    spec.effectTemplateIndex = -1;
    spec.modelNode = reinterpret_cast<zClass_NodePartial *>(0x4444);
    OptCatalog::LoadFxSpecFromReaderNode(&root, &spec, "PRIMARY");
    const bool primaryOk =
        spec.flags == 0xff && spec.animationEntry == &animEntries[3] &&
        spec.attachedAnimationEntry == &animEntries[1] &&
        spec.modelAnimationEntry == &animEntries[2] && spec.effectTemplateIndex == 1 &&
        spec.modelNode == reinterpret_cast<zClass_NodePartial *>(0x4444) &&
        spec.soundCount == 2 && spec.soundSamples[0] == &samples[0] &&
        spec.soundSamples[1] == &samples[1] && spec.bounceSoundCount == 1 &&
        spec.bounceSoundSamples[0] == &samples[2];

    OptCatalogFxSpec modelSpec = {};
    OptCatalog::LoadFxSpecFromReaderNode(&root, &modelSpec, "MODEL_ONLY");
    const bool modelOk = modelSpec.modelNode == &modelNode && modelSpec.effectTemplateIndex == 0;

    OptCatalogFxSpec missingSpec = {};
    missingSpec.flags = 0x80;
    missingSpec.effectTemplateIndex = -7;
    missingSpec.modelNode = &modelNode;
    OptCatalog::LoadFxSpecFromReaderNode(&root, &missingSpec, "MISSING");
    const bool missingOk = missingSpec.flags == 0x80 && missingSpec.effectTemplateIndex == -7 &&
                           missingSpec.modelNode == &modelNode;

    g_zEffect_RuntimeManager = oldRuntimeManager;
    g_zEffectAnim_EntryCount = oldAnimEntryCount;
    g_zEffectAnim_EntryList = oldAnimEntryList;
    g_zSnd_IsInitialized = oldSndInitialized;
    g_zSnd_ActiveBackend = oldSndActiveBackend;
    g_zSnd_SampleSetRegistry = oldSampleRegistry;
    zClass_TypeList::Head(6) = oldTypeHead;
    zClass_TypeList::Tail(6) = oldTypeTail;
    zClass_TypeList::PendingRemovalDirty(6) = oldPendingDirty;

    if (!primaryOk) {
        return 1;
    }
    if (!modelOk) {
        return 2;
    }
    return missingOk ? 0 : 3;
}

extern "C" int zweapon_optcatalog_invoke_damage_feedback_and_hit_callback_smoke(void) {
    const int oldContextKind = g_OptCatalog_DamageContextKind;
    void *const oldContextHitEvent = g_OptCatalog_DamageContextHitEvent;
    const int oldCaptureEnabled = g_OptCatalog_CaptureHitSnapshotEnabled;
    const zVec3 oldSourcePos = g_OptCatalog_CapturedDamageSourcePos;
    const zVec3 oldHitPos = g_OptCatalog_CapturedDamageHitPos;
    void *const oldFeedbackCallback = g_OptCatalogDamageFeedbackCallback;
    void *const oldOwnerOrCtx = g_OptCatalog_CurrentDamageOwnerOrCtx;
    const int oldHitCount = g_OptCatalog_DamageFeedbackHitCount;
    zClass_NodePartial *const oldTrackedNode = g_OptCatalogDamageFeedbackTrackedNode;
    const float oldScalar = g_OptCatalogDamageFeedbackIntensityScalar;

    OptCatalogEntryDef entry = {};
    entry.flags = 0x200000;
    zClass_NodePartial ownerNode = {};
    zVec3 sourcePos = {1.0f, 2.0f, 3.0f};
    OptCatalogHitEventPartial hitEvent = {};
    hitEvent.hitPos = {4.0f, 5.0f, 6.0f};

    zClass_NodeFreeListSlot healthSlot = {};
    TestDamageHealthOverlay healthOverlay = {};
    healthOverlay.health = 10.0f;
    healthSlot.damageHandler = reinterpret_cast<void *>(1);
    healthSlot.node.callbackContext = reinterpret_cast<zClass_NodePartial *>(&healthOverlay);
    hitEvent.hitNode = &healthSlot.node;
    g_OptCatalog_DamageContextKind = 9;
    g_OptCatalog_DamageContextHitEvent = &entry;
    g_OptCatalogDamageFeedbackTrackedNode = &ownerNode;
    g_OptCatalog_DamageFeedbackHitCount = 0;

    const int healthResult = OptCatalog::InvokeDamageFeedbackAndHitCallback(
        &entry, &ownerNode, &sourcePos, &hitEvent, 2.5f);
    const bool healthOk = healthResult == 0 && healthOverlay.health == 7.5f &&
                          g_OptCatalog_DamageContextKind == 0 &&
                          g_OptCatalog_DamageContextHitEvent == nullptr &&
                          g_OptCatalog_DamageFeedbackHitCount == 1;

    OptCatalogDamageHandlerPartial handler = {};
    int callbackContext = 1234;
    handler.hitCallback = &callbackContext;
    handler.hitContext = reinterpret_cast<void *>(TestOptCatalogHitCallback);

    zClass_NodeFreeListSlot callbackSlot = {};
    callbackSlot.damageHandler = &handler;
    hitEvent.hitNode = &callbackSlot.node;

    g_TestHitCallbackCalls = 0;
    g_TestHitCallbackContext = nullptr;
    g_TestHitCallbackEntry = nullptr;
    g_TestHitCallbackEvent = nullptr;
    g_TestHitCallbackDamage = 0.0f;
    g_TestFeedbackCallbackCalls = 0;
    g_TestFeedbackHandler = nullptr;
    g_TestFeedbackHitNode = nullptr;
    g_TestFeedbackDamage = 0.0f;
    g_OptCatalog_CaptureHitSnapshotEnabled = 1;
    g_OptCatalogDamageFeedbackCallback =
        reinterpret_cast<void *>(TestOptCatalogFeedbackCallback);
    g_OptCatalog_DamageFeedbackHitCount = 0;
    g_OptCatalogDamageFeedbackTrackedNode = &ownerNode;
    g_OptCatalog_CurrentDamageOwnerOrCtx = nullptr;
    g_OptCatalogDamageFeedbackIntensityScalar = 9.0f;

    const int callbackResult = OptCatalog::InvokeDamageFeedbackAndHitCallback(
        &entry, &ownerNode, &sourcePos, &hitEvent, 3.25f);
    const bool callbackOk =
        callbackResult == 77 && g_TestHitCallbackCalls == 1 &&
        g_TestHitCallbackContext == &callbackContext && g_TestHitCallbackEntry == &entry &&
        g_TestHitCallbackEvent == &hitEvent && g_TestHitCallbackDamage == 3.25f &&
        g_TestFeedbackCallbackCalls == 1 && g_TestFeedbackHandler == &handler &&
        g_TestFeedbackHitNode == &callbackSlot.node && g_TestFeedbackDamage == 3.25f &&
        g_OptCatalog_CapturedDamageSourcePos.x == 1.0f &&
        g_OptCatalog_CapturedDamageSourcePos.y == 2.0f &&
        g_OptCatalog_CapturedDamageSourcePos.z == 3.0f &&
        g_OptCatalog_CapturedDamageHitPos.x == 4.0f &&
        g_OptCatalog_CapturedDamageHitPos.y == 5.0f &&
        g_OptCatalog_CapturedDamageHitPos.z == 6.0f &&
        g_OptCatalog_CurrentDamageOwnerOrCtx == &ownerNode &&
        g_OptCatalogDamageFeedbackIntensityScalar == 0.5f &&
        g_OptCatalog_DamageFeedbackHitCount == 1;

    g_OptCatalog_DamageContextKind = oldContextKind;
    g_OptCatalog_DamageContextHitEvent = oldContextHitEvent;
    g_OptCatalog_CaptureHitSnapshotEnabled = oldCaptureEnabled;
    g_OptCatalog_CapturedDamageSourcePos = oldSourcePos;
    g_OptCatalog_CapturedDamageHitPos = oldHitPos;
    g_OptCatalogDamageFeedbackCallback = oldFeedbackCallback;
    g_OptCatalog_CurrentDamageOwnerOrCtx = oldOwnerOrCtx;
    g_OptCatalog_DamageFeedbackHitCount = oldHitCount;
    g_OptCatalogDamageFeedbackTrackedNode = oldTrackedNode;
    g_OptCatalogDamageFeedbackIntensityScalar = oldScalar;

    if (!healthOk) {
        return 1;
    }
    return callbackOk ? 0 : 2;
}

extern "C" int hitcontext_get_current_owner_smoke(void) {
    void *const oldOwnerOrCtx = g_OptCatalog_CurrentDamageOwnerOrCtx;
    int ownerSentinel = 0;

    g_OptCatalog_CurrentDamageOwnerOrCtx = &ownerSentinel;
    const bool ownerReturned = HitContext::GetCurrentOwnerOrCtx() == &ownerSentinel;

    g_OptCatalog_CurrentDamageOwnerOrCtx = nullptr;
    const bool nullReturned = HitContext::GetCurrentOwnerOrCtx() == nullptr;

    g_OptCatalog_CurrentDamageOwnerOrCtx = oldOwnerOrCtx;
    return ownerReturned && nullReturned ? 0 : 1;
}

extern "C" int optcatalog_set_damage_context_smoke(void) {
    const int oldContextKind = g_OptCatalog_DamageContextKind;
    void *const oldContextHitEvent = g_OptCatalog_DamageContextHitEvent;

    OptCatalogHitEventPartial hitEvent = {};
    zClass_NodePartial hitNode = {};
    int previous = 0;
    g_OptCatalog_DamageContextKind = 11;
    g_OptCatalog_DamageContextHitEvent = &previous;

    OptCatalog::SetDamageContext(3, nullptr);
    const bool nullEventDoesNotCapture =
        g_OptCatalog_DamageContextKind == 3 && g_OptCatalog_DamageContextHitEvent == &previous;

    OptCatalog::SetDamageContext(4, &hitEvent);
    const bool nullHitNodeDoesNotCapture =
        g_OptCatalog_DamageContextKind == 4 && g_OptCatalog_DamageContextHitEvent == &previous;

    hitEvent.hitNode = &hitNode;
    OptCatalog::SetDamageContext(5, &hitEvent);
    const bool hitNodeCaptures =
        g_OptCatalog_DamageContextKind == 5 && g_OptCatalog_DamageContextHitEvent == &hitEvent;

    g_OptCatalog_DamageContextKind = oldContextKind;
    g_OptCatalog_DamageContextHitEvent = oldContextHitEvent;
    return nullEventDoesNotCapture && nullHitNodeDoesNotCapture && hitNodeCaptures ? 0 : 1;
}

extern "C" int optcatalog_damage_feedback_leaf_helpers_smoke(void) {
    const float oldScalar = g_OptCatalogDamageFeedbackIntensityScalar;
    const zVec3 oldSourcePos = g_OptCatalog_CapturedDamageSourcePos;
    const zVec3 oldHitPos = g_OptCatalog_CapturedDamageHitPos;

    DamageFeedback::SetIntensityScalar(2.25f);
    const bool scalarOk = g_OptCatalogDamageFeedbackIntensityScalar == 2.25f;

    g_OptCatalog_CapturedDamageSourcePos = {1.0f, 2.0f, 3.0f};
    g_OptCatalog_CapturedDamageHitPos = {4.0f, 5.0f, 6.0f};
    zVec3 *const captured = OptCatalog::GetCapturedHitSourcePtr();
    const bool captureOk = captured == &g_OptCatalog_CapturedDamageSourcePos &&
                           captured[0].x == 1.0f && captured[0].y == 2.0f &&
                           captured[0].z == 3.0f && captured[1].x == 4.0f &&
                           captured[1].y == 5.0f && captured[1].z == 6.0f;

    g_OptCatalogDamageFeedbackIntensityScalar = oldScalar;
    g_OptCatalog_CapturedDamageSourcePos = oldSourcePos;
    g_OptCatalog_CapturedDamageHitPos = oldHitPos;
    return scalarOk && captureOk ? 0 : 1;
}

extern "C" int optcatalog_capture_hit_snapshot_and_invoke_damage_timer_callback_smoke(void) {
    const int oldCaptureEnabled = g_OptCatalog_CaptureHitSnapshotEnabled;
    const zVec3 oldSourcePos = g_OptCatalog_CapturedDamageSourcePos;
    const zVec3 oldHitPos = g_OptCatalog_CapturedDamageHitPos;

    zClass_NodeFreeListSlot hitNodeSlot = {};
    OptCatalogDamageHandlerPartial handler = {};
    OptCatalogHitEventPartial hitEvent = {};
    zVec3 sourcePos = {7.0f, 8.0f, 9.0f};
    float contextBonus = 4.0f;
    hitNodeSlot.damageHandler = &handler;
    handler.timerCallback = (void *)TestDamageTimerCallback;
    handler.timerContext = &contextBonus;
    hitEvent.hitNode = &hitNodeSlot.node;
    hitEvent.hitPos = {1.0f, 2.0f, 3.0f};
    g_TestDamageTimerCalls = 0;
    g_TestDamageTimerContext = nullptr;
    g_TestDamageTimerDamage = 0.0f;
    g_OptCatalog_CaptureHitSnapshotEnabled = 1;

    const float result =
        OptCatalog::CaptureHitSnapshotAndInvokeDamageTimerCallback(&sourcePos, &hitEvent, 2.5f);
    const bool callbackOk = result == 6.5f && g_TestDamageTimerCalls == 1 &&
                            g_TestDamageTimerContext == &contextBonus &&
                            g_TestDamageTimerDamage == 2.5f;
    const bool captureOk =
        g_OptCatalog_CapturedDamageSourcePos.x == 7.0f &&
        g_OptCatalog_CapturedDamageSourcePos.y == 8.0f &&
        g_OptCatalog_CapturedDamageSourcePos.z == 9.0f &&
        g_OptCatalog_CapturedDamageHitPos.x == 1.0f &&
        g_OptCatalog_CapturedDamageHitPos.y == 2.0f &&
        g_OptCatalog_CapturedDamageHitPos.z == 3.0f;

    g_OptCatalog_CapturedDamageSourcePos = {11.0f, 12.0f, 13.0f};
    g_OptCatalog_CapturedDamageHitPos = {14.0f, 15.0f, 16.0f};
    g_OptCatalog_CaptureHitSnapshotEnabled = 0;
    OptCatalog::CaptureHitSnapshotAndInvokeDamageTimerCallback(&sourcePos, &hitEvent, 1.0f);
    const bool disabledCaptureOk =
        g_OptCatalog_CapturedDamageSourcePos.x == 11.0f &&
        g_OptCatalog_CapturedDamageSourcePos.y == 12.0f &&
        g_OptCatalog_CapturedDamageSourcePos.z == 13.0f &&
        g_OptCatalog_CapturedDamageHitPos.x == 14.0f &&
        g_OptCatalog_CapturedDamageHitPos.y == 15.0f &&
        g_OptCatalog_CapturedDamageHitPos.z == 16.0f;

    g_OptCatalog_CaptureHitSnapshotEnabled = oldCaptureEnabled;
    g_OptCatalog_CapturedDamageSourcePos = oldSourcePos;
    g_OptCatalog_CapturedDamageHitPos = oldHitPos;
    return callbackOk && captureOk && disabledCaptureOk ? 0 : 1;
}

extern "C" int zweapon_optcatalog_emit_crater_impact_event_smoke(void) {
    zDEClient_NetRelayCallback const oldRelayCallback = g_zDEClientCraterNetRelayCallback;
    zClass_NodePartial *const oldCameraNode = g_zDEClient_CameraNode;
    zClass_CameraDataPartial *const oldCameraData = g_zDEClient_CameraNodeClassData;
    const float oldMaxCraterRadius = g_OptCatalogMaxCraterRadius;

    OptCatalogEntryDef entry = {};
    entry.impactProximity = 12.0f;
    zClass_NodePartial damageOwnerNode = {};
    zClass_NodePartial unusedOwnerNode = {};
    zClass_NodePartial cameraNode = {};
    zClass_NodePartial hitNode = {};
    OptCatalogSurfaceMaterialRef surfaceRef = {};
    OptCatalogHitEventPartial hitEvent = {};
    hitEvent.hitNode = &hitNode;
    hitEvent.surfaceRef = &surfaceRef;
    hitEvent.hitPos = {1.0f, 2.0f, 3.0f};

    g_TestCraterRelayCalls = 0;
    g_TestCraterRelayResult = 0;
    g_zDEClientCraterNetRelayCallback = TestOptCatalogCraterRelayCallback;
    g_zDEClient_CameraNode = nullptr;
    g_zDEClient_CameraNodeClassData = nullptr;
    g_OptCatalogMaxCraterRadius = 5.0f;
    zModel_Const::SetVertexMergeEpsilon(0.25f);

    const int ignoredResult = OptCatalog::EmitCraterImpactEvent(
        &entry, &hitEvent, &unusedOwnerNode, &damageOwnerNode);
    int failCode = 0;
    if (ignoredResult != 0) {
        failCode = 1;
    } else if (g_TestCraterRelayCalls != 0) {
        failCode = 2;
    } else if (zModel_Const::GetVertexMergeEpsilon() != 0.25f) {
        failCode = 3;
    }

    hitNode.flags = 0x10000;
    const int relayRejectedResult = OptCatalog::EmitCraterImpactEvent(
        &entry, &hitEvent, &unusedOwnerNode, &damageOwnerNode);
    if (failCode == 0 && relayRejectedResult != 0) {
        failCode = 4;
    } else if (failCode == 0 && g_TestCraterRelayCalls != 1) {
        failCode = 5;
    } else if (failCode == 0 && zModel_Const::GetVertexMergeEpsilon() != 0.25f) {
        failCode = 6;
    }

    g_TestCraterRelayCalls = 0;
    g_TestCraterRelayResult = 1;
    entry.craterRadiusBase = 3;
    entry.craterRadiusRandomRange = 4;
    g_zDEClient_CameraNode = &cameraNode;
    const int instanceFailureResult = OptCatalog::EmitCraterImpactEvent(
        &entry, &hitEvent, &unusedOwnerNode, &damageOwnerNode);
    if (failCode == 0 && instanceFailureResult != 0) {
        failCode = 7;
    } else if (failCode == 0 && g_TestCraterRelayCalls != 1) {
        failCode = 8;
    } else if (failCode == 0 && zModel_Const::GetVertexMergeEpsilon() != 0.25f) {
        failCode = 9;
    }

    g_zDEClientCraterNetRelayCallback = oldRelayCallback;
    g_zDEClient_CameraNode = oldCameraNode;
    g_zDEClient_CameraNodeClassData = oldCameraData;
    g_OptCatalogMaxCraterRadius = oldMaxCraterRadius;

    return failCode;
}

extern "C" int zweapon_optcatalog_emit_qsand_impact_event_smoke(void) {
    zDEClient_NetRelayCallback const oldRelayCallback = g_zDEClientQSandNetRelayCallback;
    zClass_NodePartial *const oldCameraNode = g_zDEClient_CameraNode;
    zClass_CameraDataPartial *const oldCameraData = g_zDEClient_CameraNodeClassData;
    const int oldQSandEnabled = g_zDEClient_QuickSandEnabled;
    const float oldMaxCraterRadius = g_OptCatalogMaxCraterRadius;

    OptCatalogEntryDef entry = {};
    entry.impactProximity = 16.0f;
    zClass_NodePartial damageOwnerNode = {};
    zClass_NodePartial unusedOwnerNode = {};
    zClass_NodePartial cameraNode = {};
    zClass_NodePartial hitNode = {};
    OptCatalogHitEventPartial hitEvent = {};
    hitEvent.hitNode = &hitNode;
    hitEvent.hitPos = {4.0f, 5.0f, 6.0f};

    g_TestQSandRelayCalls = 0;
    g_TestQSandRelayResult = 0;
    g_zDEClientQSandNetRelayCallback = TestOptCatalogQSandRelayCallback;
    g_zDEClient_CameraNode = nullptr;
    g_zDEClient_CameraNodeClassData = nullptr;
    g_zDEClient_QuickSandEnabled = 0;
    g_OptCatalogMaxCraterRadius = 5.0f;
    zModel_Const::SetVertexMergeEpsilon(0.25f);

    OptCatalog::EmitQSandImpactEvent(&entry, &hitEvent, &unusedOwnerNode, &damageOwnerNode);
    int failCode = 0;
    if (g_TestQSandRelayCalls != 0) {
        failCode = 1;
    } else if (zModel_Const::GetVertexMergeEpsilon() != 0.25f) {
        failCode = 2;
    }

    hitNode.flags = 0x10000;
    OptCatalog::EmitQSandImpactEvent(&entry, &hitEvent, &unusedOwnerNode, &damageOwnerNode);
    if (failCode == 0 && g_TestQSandRelayCalls != 1) {
        failCode = 3;
    } else if (failCode == 0 && zModel_Const::GetVertexMergeEpsilon() != 0.25f) {
        failCode = 4;
    }

    g_TestQSandRelayCalls = 0;
    g_TestQSandRelayResult = 1;
    entry.craterRadiusBase = 2;
    entry.craterRadiusRandomRange = 6;
    g_zDEClient_QuickSandEnabled = 1;
    g_zDEClient_CameraNode = &cameraNode;
    OptCatalog::EmitQSandImpactEvent(&entry, &hitEvent, &unusedOwnerNode, &damageOwnerNode);
    if (failCode == 0 && g_TestQSandRelayCalls != 1) {
        failCode = 5;
    } else if (failCode == 0 && zModel_Const::GetVertexMergeEpsilon() != 0.25f) {
        failCode = 6;
    }

    g_zDEClientQSandNetRelayCallback = oldRelayCallback;
    g_zDEClient_CameraNode = oldCameraNode;
    g_zDEClient_CameraNodeClassData = oldCameraData;
    g_zDEClient_QuickSandEnabled = oldQSandEnabled;
    g_OptCatalogMaxCraterRadius = oldMaxCraterRadius;

    return failCode;
}

extern "C" int zweapon_optcatalog_play_impact_sound_smoke(void) {
    const int oldInitialized = g_zSnd_IsInitialized;
    const int oldPreInitialized = g_zSnd_PreInitialized;
    const int oldActiveBackend = g_zSnd_ActiveBackend;
    void *const oldGlobalVolumeScalePtr = g_zSnd_GlobalVolumeScalePtr;

    OptCatalogFxSpec specs[2] = {};
    OptCatalogEntryDef entry = {};
    entry.impactFxTable = specs;
    OptCatalogHitEventPartial hitEvent = {};
    hitEvent.hitPos = {1.0f, 2.0f, 3.0f};

    zSndSample sample = {};
    sample.replayFields.flags = 0x08;
    sample.replayFields.gain = 1.0f;
    sample.markerBaseTime = 7.0f;
    float globalVolume = 1.0f;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 2;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;

    OptCatalog::PlayImpactSound(&entry, &hitEvent, 0, 0.5f);
    int failCode = sample.markerBaseTime != 7.0f ? 1 : 0;

    specs[1].soundCount = 1;
    specs[1].soundSamples[0] = &sample;
    OptCatalog::PlayImpactSound(&entry, &hitEvent, 1, 0.5f);
    if (failCode == 0 && sample.markerBaseTime != 0.0f) {
        failCode = 2;
    }

    g_zSnd_IsInitialized = oldInitialized;
    g_zSnd_PreInitialized = oldPreInitialized;
    g_zSnd_ActiveBackend = oldActiveBackend;
    g_zSnd_GlobalVolumeScalePtr = oldGlobalVolumeScalePtr;

    return failCode;
}

extern "C" int zweapon_optcatalog_play_bounce_sound_smoke(void) {
    const int oldInitialized = g_zSnd_IsInitialized;
    const int oldPreInitialized = g_zSnd_PreInitialized;
    const int oldActiveBackend = g_zSnd_ActiveBackend;
    void *const oldGlobalVolumeScalePtr = g_zSnd_GlobalVolumeScalePtr;

    OptCatalogFxSpec specs[2] = {};
    OptCatalogEntryDef entry = {};
    entry.impactFxTable = specs;
    OptCatalogRaycastHitEntry hitEvent = {};
    hitEvent.pos = {4.0f, 5.0f, 6.0f};

    zSndSample sample = {};
    sample.replayFields.flags = 0x08;
    sample.replayFields.gain = 1.0f;
    sample.markerBaseTime = 9.0f;
    float globalVolume = 1.0f;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 2;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;

    OptCatalog::PlayBounceSound(&entry, &hitEvent, 0, 0.75f);
    int failCode = sample.markerBaseTime != 9.0f ? 1 : 0;

    specs[1].bounceSoundCount = 1;
    specs[1].bounceSoundSamples[0] = &sample;
    OptCatalog::PlayBounceSound(&entry, &hitEvent, 1, 0.75f);
    if (failCode == 0 && sample.markerBaseTime != 0.0f) {
        failCode = 2;
    }

    g_zSnd_IsInitialized = oldInitialized;
    g_zSnd_PreInitialized = oldPreInitialized;
    g_zSnd_ActiveBackend = oldActiveBackend;
    g_zSnd_GlobalVolumeScalePtr = oldGlobalVolumeScalePtr;

    return failCode;
}

extern "C" int zweapon_optcatalog_handle_impact_event_smoke(void) {
    const int oldCaptureEnabled = g_OptCatalog_CaptureHitSnapshotEnabled;
    const zVec3 oldSourcePos = g_OptCatalog_CapturedDamageSourcePos;
    const zVec3 oldHitPos = g_OptCatalog_CapturedDamageHitPos;
    void *const oldOwnerOrCtx = g_OptCatalog_CurrentDamageOwnerOrCtx;
    const int oldContextKind = g_OptCatalog_DamageContextKind;
    void *const oldContextHitEvent = g_OptCatalog_DamageContextHitEvent;

    OptCatalogFxSpec specs[2] = {};
    OptCatalogEntryDef entry = {};
    entry.flags = 0x200000;
    entry.damage = 4.0f;
    entry.impactFxTable = specs;
    entry.impactCallback = TestOptCatalogImpactCallback;

    zClass_NodePartial ownerNode = {};
    OptCatalogRuntimeInstanceStorage runtimeInstance = {};
    runtimeInstance.ownerNode = &ownerNode;
    runtimeInstance.pos = {7.0f, 8.0f, 9.0f};
    runtimeInstance.spawnScale = 2.5f;

    OptCatalogSurfaceMaterialRef surfaceRef = {};
    surfaceRef.impactSlot = 1;
    OptCatalogHitEventPartial hitEvent = {};
    hitEvent.surfaceRef = &surfaceRef;
    hitEvent.hitPos = {1.0f, 2.0f, 3.0f};

    OptCatalogDamageHandlerPartial handler = {};
    int callbackContext = 4321;
    handler.hitCallback = &callbackContext;
    handler.hitContext = reinterpret_cast<void *>(TestOptCatalogHitCallback);

    zClass_NodeFreeListSlot callbackSlot = {};
    callbackSlot.damageHandler = &handler;
    hitEvent.hitNode = &callbackSlot.node;

    g_TestImpactCallbackCalls = 0;
    g_TestImpactCallbackEntry = nullptr;
    g_TestImpactCallbackEvent = nullptr;
    g_TestImpactCallbackRuntime = nullptr;
    g_TestImpactCallbackHitPos = {};
    g_TestImpactCallbackHitNode = nullptr;
    g_TestImpactCallbackImpactSlot = -1;
    g_TestHitCallbackCalls = 0;
    g_TestHitCallbackContext = nullptr;
    g_TestHitCallbackEntry = nullptr;
    g_TestHitCallbackEvent = nullptr;
    g_TestHitCallbackDamage = 0.0f;
    g_OptCatalog_CaptureHitSnapshotEnabled = 1;
    g_OptCatalog_CurrentDamageOwnerOrCtx = nullptr;
    g_OptCatalog_DamageContextKind = 0;
    g_OptCatalog_DamageContextHitEvent = nullptr;

    OptCatalog::HandleImpactEvent(&entry, &hitEvent, &runtimeInstance);

    int failCode = 0;
    if (g_TestImpactCallbackCalls != 1 || g_TestImpactCallbackEntry != &entry ||
        g_TestImpactCallbackEvent != &hitEvent ||
        g_TestImpactCallbackRuntime != &runtimeInstance) {
        failCode = 1;
    } else if (g_TestHitCallbackCalls != 1 || g_TestHitCallbackContext != &callbackContext ||
               g_TestHitCallbackEntry != &entry || g_TestHitCallbackEvent != &hitEvent ||
               g_TestHitCallbackDamage != 10.0f) {
        failCode = 2;
    } else if (g_OptCatalog_CurrentDamageOwnerOrCtx != &ownerNode) {
        failCode = 3;
    } else if (g_OptCatalog_CapturedDamageSourcePos.x != 7.0f ||
               g_OptCatalog_CapturedDamageSourcePos.y != 8.0f ||
               g_OptCatalog_CapturedDamageSourcePos.z != 9.0f ||
               g_OptCatalog_CapturedDamageHitPos.x != 1.0f ||
               g_OptCatalog_CapturedDamageHitPos.y != 2.0f ||
               g_OptCatalog_CapturedDamageHitPos.z != 3.0f) {
        failCode = 4;
    }

    g_OptCatalog_CaptureHitSnapshotEnabled = oldCaptureEnabled;
    g_OptCatalog_CapturedDamageSourcePos = oldSourcePos;
    g_OptCatalog_CapturedDamageHitPos = oldHitPos;
    g_OptCatalog_CurrentDamageOwnerOrCtx = oldOwnerOrCtx;
    g_OptCatalog_DamageContextKind = oldContextKind;
    g_OptCatalog_DamageContextHitEvent = oldContextHitEvent;

    return failCode;
}

extern "C" int zweapon_optcatalog_handle_impact_event_from_runtime_state_smoke(void) {
    const int oldCaptureEnabled = g_OptCatalog_CaptureHitSnapshotEnabled;
    const zVec3 oldSourcePos = g_OptCatalog_CapturedDamageSourcePos;
    const zVec3 oldHitPos = g_OptCatalog_CapturedDamageHitPos;
    void *const oldOwnerOrCtx = g_OptCatalog_CurrentDamageOwnerOrCtx;
    const int oldContextKind = g_OptCatalog_DamageContextKind;
    void *const oldContextHitEvent = g_OptCatalog_DamageContextHitEvent;

    OptCatalogFxSpec specs[1] = {};
    OptCatalogEntryDef entry = {};
    entry.flags = 0x200000;
    entry.damage = 3.0f;
    entry.impactFxTable = specs;
    entry.impactCallback = TestOptCatalogImpactCallback;

    zClass_NodePartial ownerNode = {};
    OptCatalogRuntimeInstanceStorage runtimeInstance = {};
    runtimeInstance.ownerNode = &ownerNode;
    runtimeInstance.pos = {4.0f, 5.0f, 6.0f};
    runtimeInstance.spawnScale = 2.0f;

    OptCatalogDamageHandlerPartial handler = {};
    int callbackContext = 6789;
    handler.hitCallback = &callbackContext;
    handler.hitContext = reinterpret_cast<void *>(TestOptCatalogHitCallback);

    zClass_NodeFreeListSlot projectileSlot = {};
    projectileSlot.damageHandler = &handler;
    runtimeInstance.projectileNode = &projectileSlot.node;

    g_TestImpactCallbackCalls = 0;
    g_TestImpactCallbackEntry = nullptr;
    g_TestImpactCallbackEvent = nullptr;
    g_TestImpactCallbackRuntime = nullptr;
    g_TestImpactCallbackHitPos = {};
    g_TestImpactCallbackHitNode = nullptr;
    g_TestImpactCallbackImpactSlot = -1;
    g_TestHitCallbackCalls = 0;
    g_TestHitCallbackContext = nullptr;
    g_TestHitCallbackEntry = nullptr;
    g_TestHitCallbackEvent = nullptr;
    g_TestHitCallbackDamage = 0.0f;
    g_OptCatalog_CaptureHitSnapshotEnabled = 1;
    g_OptCatalog_CurrentDamageOwnerOrCtx = nullptr;
    g_OptCatalog_DamageContextKind = 0;
    g_OptCatalog_DamageContextHitEvent = nullptr;

    OptCatalog::HandleImpactEventFromRuntimeState(&entry, &runtimeInstance);

    int failCode = 0;
    if (g_TestImpactCallbackCalls != 1 || g_TestImpactCallbackEntry != &entry ||
        g_TestImpactCallbackRuntime != &runtimeInstance) {
        failCode = 1;
    } else if (g_TestImpactCallbackHitPos.x != 4.0f ||
               g_TestImpactCallbackHitPos.y != 5.0f ||
               g_TestImpactCallbackHitPos.z != 6.0f ||
               g_TestImpactCallbackHitNode != &projectileSlot.node ||
               g_TestImpactCallbackImpactSlot != 0) {
        failCode = 2;
    } else if (g_TestHitCallbackCalls != 1 || g_TestHitCallbackContext != &callbackContext ||
               g_TestHitCallbackEntry != &entry || g_TestHitCallbackDamage != 6.0f) {
        failCode = 3;
    } else if (g_OptCatalog_CapturedDamageSourcePos.x != 4.0f ||
               g_OptCatalog_CapturedDamageSourcePos.y != 5.0f ||
               g_OptCatalog_CapturedDamageSourcePos.z != 6.0f ||
               g_OptCatalog_CapturedDamageHitPos.x != 4.0f ||
               g_OptCatalog_CapturedDamageHitPos.y != 5.0f ||
               g_OptCatalog_CapturedDamageHitPos.z != 6.0f) {
        failCode = 4;
    }

    g_OptCatalog_CaptureHitSnapshotEnabled = oldCaptureEnabled;
    g_OptCatalog_CapturedDamageSourcePos = oldSourcePos;
    g_OptCatalog_CapturedDamageHitPos = oldHitPos;
    g_OptCatalog_CurrentDamageOwnerOrCtx = oldOwnerOrCtx;
    g_OptCatalog_DamageContextKind = oldContextKind;
    g_OptCatalog_DamageContextHitEvent = oldContextHitEvent;

    return failCode;
}

extern "C" int zweapon_optcatalog_build_impact_hit_list_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;

    OptCatalogEntryDef entry = {};
    entry.impactProximity = 0.5f;

    zClass_NodePartial projectileNode = {};
    projectileNode.flags = 0x10;
    zClass_NodePartial ownerNode = {};
    OptCatalogRuntimeInstanceStorage runtimeInstance = {};
    runtimeInstance.ownerNode = &ownerNode;
    runtimeInstance.projectileNode = &projectileNode;
    runtimeInstance.pos = {0.5f, 0.5f, 0.5f};

    zClass_NodePartial targetNode = {};
    std::strcpy(targetNode.name, "impactTarget");
    targetNode.flags = 0x144;
    targetNode.nodeType = 0xff;
    int targetClassData = 0;
    targetNode.classId = 2;
    targetNode.classData = &targetClassData;
    targetNode.cachedBounds[0] = 0.0f;
    targetNode.cachedBounds[1] = 0.0f;
    targetNode.cachedBounds[2] = 0.0f;
    targetNode.cachedBounds[3] = 1.0f;
    targetNode.cachedBounds[4] = 1.0f;
    targetNode.cachedBounds[5] = 1.0f;

    zClass_NodePartial *areaChildren[1] = {&targetNode};
    zWorldAreaPartial area{};
    area.childCount = 1;
    area.childList = areaChildren;
    zWorldAreaPartial emptyRow{};
    zWorldAreaPartial *rows[2] = {&emptyRow, &area};
    zClass_WorldDataPartial worldData{};
    worldData.originX = 0.0f;
    worldData.originZ = -1.0f;
    worldData.worldMaxX = 2.0f;
    worldData.worldMaxZ = 1.0f;
    worldData.areaInvSizeX = 0.5f;
    worldData.areaInvSizeZ = 0.5f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 2;
    worldData.areaGridRows = rows;
    zClass_NodePartial world{};
    world.classData = &worldData;
    g_Player_RuntimeDiScene = &world;

    OptCatalogRaycastHitList hits{};
    int result = OptCatalog::BuildImpactHitList(&entry, &runtimeInstance, 1, &hits);
    int failCode = 0;
    if (result != 1 || hits.hitCount != 1 || hits.hits[0].hitNode != &targetNode ||
        hits.hits[0].pos.x != 0.5f || hits.hits[0].pos.y != 0.5f ||
        hits.hits[0].pos.z != 0.5f || (projectileNode.flags & 0x10) == 0) {
        failCode = 1;
    }

    runtimeInstance.ownerNode = &targetNode;
    hits = {};
    if (failCode == 0 &&
        OptCatalog::BuildImpactHitList(&entry, &runtimeInstance, 0, &hits) != 0) {
        failCode = 2;
    }

    g_Player_RuntimeDiScene = oldRuntimeScene;
    return failCode;
}

extern "C" int zweapon_optcatalog_handle_impact_from_runtime_probe_smoke(void) {
    const int oldCaptureEnabled = g_OptCatalog_CaptureHitSnapshotEnabled;
    const int oldHitCount = g_OptCatalog_DamageFeedbackHitCount;

    OptCatalogEntryDef entry = {};
    entry.flags = 1u << 12;
    entry.damage = 10.0f;
    entry.damageFalloffRange = 20.0f;

    zClass_NodePartial ownerNode = {};
    OptCatalogRuntimeInstanceStorage runtimeInstance = {};
    runtimeInstance.ownerNode = &ownerNode;
    runtimeInstance.pos = {2.0f, 3.0f, 4.0f};
    runtimeInstance.spawnScale = 2.0f;

    OptCatalogDamageHandlerPartial handler = {};
    int callbackContext = 123;
    handler.hitCallback = &callbackContext;
    handler.hitContext = reinterpret_cast<void *>(TestOptCatalogHitCallback);
    zClass_NodeFreeListSlot hitSlot = {};
    hitSlot.damageHandler = &handler;

    OptCatalogRaycastHitList hitList = {};
    hitList.hitCount = 1;
    hitList.hits[0].hitNode = &hitSlot.node;
    hitList.hits[0].pos = {5.0f, 6.0f, 7.0f};
    hitList.hits[0].distance = 5.0f;

    g_TestHitCallbackCalls = 0;
    g_TestHitCallbackContext = nullptr;
    g_TestHitCallbackEntry = nullptr;
    g_TestHitCallbackEvent = nullptr;
    g_TestHitCallbackDamage = 0.0f;
    g_OptCatalog_CaptureHitSnapshotEnabled = 0;

    int failCode = 0;
    if (OptCatalog::HandleImpactFromRuntimeProbe(&entry, &runtimeInstance, &hitList, nullptr) !=
            1 ||
        g_TestHitCallbackCalls != 1 || g_TestHitCallbackContext != &callbackContext ||
        g_TestHitCallbackEntry != &entry || g_TestHitCallbackDamage != 15.0f ||
        g_TestHitCallbackEvent->hitPos.x != 5.0f ||
        g_TestHitCallbackEvent->hitNode != &hitSlot.node) {
        failCode = 1;
    } else {
        g_TestHitCallbackCalls = 0;
        if (OptCatalog::HandleImpactFromRuntimeProbe(&entry, &runtimeInstance, &hitList,
                                                     &handler) != 0 ||
            g_TestHitCallbackCalls != 0) {
            failCode = 2;
        }
    }

    g_OptCatalog_CaptureHitSnapshotEnabled = oldCaptureEnabled;
    g_OptCatalog_DamageFeedbackHitCount = oldHitCount;
    return failCode;
}

extern "C" int zweapon_optcatalog_process_runtime_instance_smoke(void) {
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const int oldFallbackEnabled = g_OptCatalog_FallbackImpactProbeEnabled;
    const int oldCaptureEnabled = g_OptCatalog_CaptureHitSnapshotEnabled;
    const int oldHitCount = g_OptCatalog_DamageFeedbackHitCount;

    OptCatalogEntryDef entry = {};
    entry.flags = 1u << 12;
    entry.impactProximity = 0.5f;
    entry.damageFalloffRange = 20.0f;
    entry.damage = 10.0f;

    zClass_WorldDataPartial directWorldData = {};
    zClass_NodePartial directWorld = {};
    directWorld.classData = &directWorldData;
    g_OptCatalogRuntimeWorld = &directWorld;

    zClass_NodePartial ownerNode = {};
    zClass_NodePartial projectileNode = {};
    projectileNode.flags = 0x14;
    OptCatalogRuntimeInstanceStorage runtimeInstance = {};
    runtimeInstance.ownerNode = &ownerNode;
    runtimeInstance.projectileNode = &projectileNode;
    runtimeInstance.pos = {0.5f, 0.5f, 0.5f};
    runtimeInstance.spawnScale = 2.0f;

    int callbackContext = 456;
    OptCatalogDamageHandlerPartial handler = {};
    handler.hitCallback = &callbackContext;
    handler.hitContext = reinterpret_cast<void *>(TestOptCatalogHitCallback);

    zClass_NodeFreeListSlot targetSlot = {};
    std::strcpy(targetSlot.node.name, "processTarget");
    targetSlot.damageHandler = &handler;
    targetSlot.node.flags = 0x144;
    targetSlot.node.nodeType = 0xff;
    int targetClassData = 0;
    targetSlot.node.classId = 2;
    targetSlot.node.classData = &targetClassData;
    targetSlot.node.cachedBounds[0] = 0.0f;
    targetSlot.node.cachedBounds[1] = 0.0f;
    targetSlot.node.cachedBounds[2] = 0.0f;
    targetSlot.node.cachedBounds[3] = 1.0f;
    targetSlot.node.cachedBounds[4] = 1.0f;
    targetSlot.node.cachedBounds[5] = 1.0f;

    zClass_NodePartial *areaChildren[1] = {&targetSlot.node};
    zWorldAreaPartial area = {};
    area.childCount = 1;
    area.childList = areaChildren;
    zWorldAreaPartial emptyRow = {};
    zWorldAreaPartial *rows[2] = {&emptyRow, &area};
    zClass_WorldDataPartial fallbackWorldData = {};
    fallbackWorldData.originX = 0.0f;
    fallbackWorldData.originZ = -1.0f;
    fallbackWorldData.worldMaxX = 2.0f;
    fallbackWorldData.worldMaxZ = 1.0f;
    fallbackWorldData.areaInvSizeX = 0.5f;
    fallbackWorldData.areaInvSizeZ = 0.5f;
    fallbackWorldData.areaGridColCount = 1;
    fallbackWorldData.areaGridRowCount = 2;
    fallbackWorldData.areaGridRows = rows;
    zClass_NodePartial fallbackWorld = {};
    fallbackWorld.classData = &fallbackWorldData;
    g_Player_RuntimeDiScene = &fallbackWorld;

    g_TestHitCallbackCalls = 0;
    g_TestHitCallbackContext = nullptr;
    g_TestHitCallbackEntry = nullptr;
    g_TestHitCallbackEvent = nullptr;
    g_TestHitCallbackDamage = 0.0f;
    g_OptCatalog_FallbackImpactProbeEnabled = 1;
    g_OptCatalog_CaptureHitSnapshotEnabled = 0;

    int failCode = 0;
    const int result = OptCatalog::ProcessRuntimeInstance(&entry, &runtimeInstance);
    if (result != 0) {
        failCode = 1;
    } else if ((projectileNode.flags & 0x04) == 0 || (projectileNode.flags & 0x10) == 0) {
        failCode = 2;
    } else if (runtimeInstance.pos.x != 0.5f || runtimeInstance.pos.y != 1.5f ||
               runtimeInstance.pos.z != 0.5f) {
        failCode = 3;
    } else if (g_TestHitCallbackCalls != 1 || g_TestHitCallbackContext != &callbackContext ||
               g_TestHitCallbackEntry != &entry || g_TestHitCallbackEvent->hitNode !=
                                                    &targetSlot.node) {
        failCode = 4;
    }

    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_OptCatalog_FallbackImpactProbeEnabled = oldFallbackEnabled;
    g_OptCatalog_CaptureHitSnapshotEnabled = oldCaptureEnabled;
    g_OptCatalog_DamageFeedbackHitCount = oldHitCount;
    return failCode;
}

extern "C" int zweapon_optcatalog_process_runtime_instances_smoke(void) {
    const int oldEntryCount = g_OptCatalog_EntryCount;
    OptCatalogEntryDef *const oldEntryTable = g_OptCatalog_EntryTable;
    const int oldQueuedImpactCount = g_OptCatalogQueuedImpactCount;
    const float oldRuntimeDelta = g_OptCatalogRuntimeDeltaTime;
    const float oldRuntimeNow = g_OptCatalogRuntimeNowSec;
    const float oldTimeDelta = g_Time_UnscaledDeltaTimeSec;
    const float oldTimeNow = g_Time_UnscaledAccumulatedTimeSec;
    const zTag4Partial oldVariantTag = g_Variant_CurrentTag;
    const int oldCaptureEnabled = g_OptCatalog_CaptureHitSnapshotEnabled;
    const int oldHitCount = g_OptCatalog_DamageFeedbackHitCount;
    zSndSample *const oldLockOnWarning = g_OptCatalogSndLockOnWarning;
    const float oldLockOnWarningGateTime = g_OptCatalogLockOnWarningGateTimeSec;

    OptCatalogEntryDef queuedEntry = {};
    queuedEntry.flags = 0;
    queuedEntry.damage = 10.0f;
    queuedEntry.damageFalloffRange = 20.0f;

    zClass_NodePartial ownerNode = {};
    OptCatalogRuntimeInstanceStorage queuedRuntime = {};
    queuedRuntime.ownerNode = &ownerNode;
    queuedRuntime.pos = {2.0f, 3.0f, 4.0f};
    queuedRuntime.spawnScale = 2.0f;

    int callbackContext = 789;
    OptCatalogDamageHandlerPartial handler = {};
    handler.hitCallback = &callbackContext;
    handler.hitContext = reinterpret_cast<void *>(TestOptCatalogHitCallback);
    zClass_NodeFreeListSlot hitSlot = {};
    hitSlot.damageHandler = &handler;

    OptCatalogRaycastHitList hitList = {};
    hitList.hitCount = 1;
    hitList.hits[0].hitNode = &hitSlot.node;
    hitList.hits[0].pos = {5.0f, 6.0f, 7.0f};
    hitList.hits[0].distance = 5.0f;

    g_TestHitCallbackCalls = 0;
    g_TestHitCallbackContext = nullptr;
    g_TestHitCallbackEntry = nullptr;
    g_TestHitCallbackEvent = nullptr;
    g_TestHitCallbackDamage = 0.0f;
    g_OptCatalogQueuedImpactCount = 0;
    g_OptCatalog_CaptureHitSnapshotEnabled = 0;
    OptCatalog::HandleImpactFromRuntimeProbe(&queuedEntry, &queuedRuntime, &hitList, nullptr);

    OptCatalogEntryDef entry = {};
    entry.keyName = const_cast<char *>("runtime");
    entry.range = 10.0f;
    entry.impactProximity = 0.0f;

    zClass_Object3DDataPartial projectileData = {};
    zClass_NodePartial projectileNode = {};
    projectileNode.classId = 5;
    projectileNode.classData = &projectileData;

    OptCatalogRuntimeInstanceStorage runtimeA = {};
    OptCatalogRuntimeInstanceStorage runtimeB = {};
    runtimeA.next = &runtimeB;
    runtimeA.projectileNode = &projectileNode;
    runtimeA.variantTag = 0x00000901u;
    runtimeA.dir = {1.0f, 0.0f, 0.0f};
    runtimeA.speed = 8.0f;
    runtimeA.updateCallback = reinterpret_cast<void *>(TestOptCatalogRuntimeUpdateCallback);
    zVec3 lockOnTarget = {4.0f, 0.0f, 0.0f};
    runtimeA.pendingTargetA = static_cast<void *>(&lockOnTarget);
    runtimeB.variantTag = 4u;
    runtimeB.updateCallback = reinterpret_cast<void *>(TestOptCatalogRuntimeUpdateCallback);
    entry.activeRuntimeListHead = &runtimeA;

    OptCatalogEntryDef entries[1] = {entry};
    g_OptCatalog_EntryTable = entries;
    g_OptCatalog_EntryCount = 1;
    g_Time_UnscaledDeltaTimeSec = 0.25f;
    g_Time_UnscaledAccumulatedTimeSec = 12.5f;
    g_Variant_CurrentTag.count = 2;
    g_Variant_CurrentTag.tags[0] = 7;
    g_Variant_CurrentTag.tags[1] = 8;
    g_Variant_CurrentTag.tags[2] = 0xff;
    g_TestRuntimeUpdateCalls = 0;
    g_TestRuntimeUpdateLast = nullptr;
    g_OptCatalogSndLockOnWarning = nullptr;
    g_OptCatalogLockOnWarningGateTimeSec = 10.0f;

    OptCatalog::ProcessRuntimeInstances();

    int failCode = 0;
    if (g_OptCatalogQueuedImpactCount != 0 || g_TestHitCallbackCalls != 1 ||
        g_TestHitCallbackContext != &callbackContext ||
        g_TestHitCallbackEntry != &queuedEntry || g_TestHitCallbackDamage != 15.0f) {
        failCode = 1;
    } else if (g_OptCatalogRuntimeDeltaTime != 0.25f || g_OptCatalogRuntimeNowSec != 12.5f) {
        failCode = 2;
    } else if (g_TestRuntimeUpdateCalls != 2 || g_TestRuntimeUpdateLast != &runtimeB) {
        failCode = 3;
    } else if (entries[0].activeRuntimeListHead != &runtimeA || runtimeA.next != &runtimeB) {
        failCode = 4;
    } else if (!TestFloatNear(runtimeA.pos.x, 2.0f) ||
               !TestFloatNear(runtimeA.lifetime, 0.25f)) {
        failCode = 5;
    } else if (!TestFloatNear(g_OptCatalogLockOnWarningGateTimeSec, 12.0f)) {
        failCode = 6;
    } else if (g_Variant_CurrentTag.count != 2 || g_Variant_CurrentTag.tags[0] != 7 ||
               g_Variant_CurrentTag.tags[1] != 8 || g_Variant_CurrentTag.tags[2] != 0xff) {
        failCode = 7;
    }

    g_OptCatalog_EntryCount = oldEntryCount;
    g_OptCatalog_EntryTable = oldEntryTable;
    g_OptCatalogQueuedImpactCount = oldQueuedImpactCount;
    g_OptCatalogRuntimeDeltaTime = oldRuntimeDelta;
    g_OptCatalogRuntimeNowSec = oldRuntimeNow;
    g_Time_UnscaledDeltaTimeSec = oldTimeDelta;
    g_Time_UnscaledAccumulatedTimeSec = oldTimeNow;
    g_Variant_CurrentTag = oldVariantTag;
    g_OptCatalog_CaptureHitSnapshotEnabled = oldCaptureEnabled;
    g_OptCatalog_DamageFeedbackHitCount = oldHitCount;
    g_OptCatalogSndLockOnWarning = oldLockOnWarning;
    g_OptCatalogLockOnWarningGateTimeSec = oldLockOnWarningGateTime;
    return failCode;
}

extern "C" int zweapon_optcatalog_remove_runtime_instance_smoke(void) {
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    void *const oldFreeRuntimeList = g_OptCatalogFreeRuntimeInstanceList;
    OptCatalogRemoveRuntimeRelayCallback const oldRelay =
        g_OptCatalog_RemoveRuntimeRelayCallback;

    OptCatalogEntryDef entry = {};
    entry.impactProximity = 10.0f;
    entry.impactCallback = TestOptCatalogImpactCallback;

    zClass_NodePartial ownerNode = {};
    zClass_NodePartial projectileA = {};
    zClass_NodePartial projectileB = {};
    OptCatalogRuntimeInstanceStorage runtimeA = {};
    OptCatalogRuntimeInstanceStorage runtimeB = {};
    runtimeA.next = &runtimeB;
    runtimeA.ownerNode = &ownerNode;
    runtimeA.projectileNode = &projectileA;
    runtimeA.pos = {0.25f, 0.5f, 0.25f};
    runtimeA.spawnScale = 1.0f;
    runtimeA.lifetime = 0.0f;
    runtimeB.projectileNode = &projectileB;
    runtimeB.lifetime = 5.0f;
    entry.activeRuntimeListHead = &runtimeA;

    zModel_PickFaceScenePayload facePayload = {};
    int faceIndices[3] = {0, 1, 2};
    zVec3 faceVertices[3] = {
        {0.0f, 0.25f, 0.0f},
        {1.0f, 0.25f, 0.0f},
        {0.0f, 0.25f, 1.0f},
    };
    zModel_PickFaceEntry face = {};
    face.flagsAndVertexCount = 3;
    face.vertexIndices = faceIndices;
    face.scenePayload = &facePayload;
    zModel_PickFaceData faceData = {};
    faceData.faceCount = 1;
    faceData.faces = &face;
    faceData.baseVertices = faceVertices;

    zClass_Object3DDataPartial objectData = {};
    zClass_NodePartial targetNode = {};
    targetNode.flags = 0x114;
    targetNode.nodeType = 0xff;
    targetNode.classId = 5;
    targetNode.classData = &objectData;
    targetNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    targetNode.cachedBounds[0] = 0.0f;
    targetNode.cachedBounds[1] = 0.0f;
    targetNode.cachedBounds[2] = 0.0f;
    targetNode.cachedBounds[3] = 1.0f;
    targetNode.cachedBounds[4] = 1.0f;
    targetNode.cachedBounds[5] = 1.0f;

    zClass_NodePartial *areaChildren[1] = {&targetNode};
    zWorldAreaPartial area = {};
    area.childCount = 1;
    area.childList = areaChildren;
    zWorldAreaPartial *rows[1] = {&area};
    zClass_WorldDataPartial worldData = {};
    worldData.originX = 0.0f;
    worldData.originZ = 0.0f;
    worldData.worldMaxX = 1.0f;
    worldData.worldMaxZ = 1.0f;
    worldData.areaCellSizeX = 1.0f;
    worldData.areaCellSizeZ = 1.0f;
    worldData.areaInvSizeX = 1.0f;
    worldData.areaInvSizeZ = 1.0f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = rows;
    zClass_NodePartial *worldChildren[2] = {&targetNode, &projectileA};
    zClass_NodePartial world = {};
    world.classData = &worldData;
    world.listCountB = 2;
    world.listB = worldChildren;
    g_OptCatalogRuntimeWorld = &world;

    g_TestImpactCallbackCalls = 0;
    g_TestImpactCallbackEntry = nullptr;
    g_TestImpactCallbackRuntime = nullptr;
    g_TestImpactCallbackHitNode = nullptr;
    g_TestRemoveRelayCalls = 0;
    g_TestRemoveRelayEntry = nullptr;
    g_TestRemoveRelayPoint = reinterpret_cast<zVec3 *>(1);
    g_TestRemoveRelayOwner = nullptr;
    g_OptCatalog_RemoveRuntimeRelayCallback = TestOptCatalogRemoveRuntimeRelayCallback;

    int failCode = 0;
    const int result = OptCatalog::RemoveRuntimeInstance(&entry, nullptr, &ownerNode);
    if (result != 0) {
        failCode = 1;
    } else if (entry.activeRuntimeListHead != &runtimeB) {
        failCode = 10;
    } else if (g_TestImpactCallbackCalls != 0) {
        failCode = 2;
    } else if (g_TestRemoveRelayCalls != 0) {
        failCode = 3;
    } else if (g_OptCatalogFreeRuntimeInstanceList != &runtimeA || runtimeA.next != oldFreeRuntimeList) {
        failCode = 4;
    }

    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
    g_OptCatalog_RemoveRuntimeRelayCallback = oldRelay;
    return failCode;
}

extern "C" int zweapon_optcatalog_can_spawn_through_ray_smoke(void) {
    const int oldCaptureEnabled = g_OptCatalog_CaptureHitSnapshotEnabled;
    const zVec3 oldSourcePos = g_OptCatalog_CapturedDamageSourcePos;
    const zVec3 oldHitPos = g_OptCatalog_CapturedDamageHitPos;

    zVec3 rayStart = {0.0f, 0.0f, 0.0f};
    zVec3 rayEnd = {3.0f, 4.0f, 0.0f};
    OptCatalogRaycastHitEntry hit = {};
    hit.pos = rayEnd;
    hit.unknown_00[4] = 0;
    hit.unknown_00[5] = 0;
    hit.unknown_00[6] = 0x80;
    hit.unknown_00[7] = 0x3f;
    zClass_NodeFreeListSlot hitSlot = {};
    hit.hitNode = &hitSlot.node;

    OptCatalogEntryDef entry = {};
    entry.flags = 1u;
    float rayLength = 0.0f;
    float reflectedLength = 0.0f;
    zVec3 reflectedDir = {};
    int failCode = 0;
    if (OptCatalog::CanSpawnThroughRay(&entry, &hit, &rayStart, &rayEnd, &rayLength,
                                       &reflectedLength, &reflectedDir) != 1 ||
        !TestFloatNear(rayLength, 5.0f) || !TestFloatNear(reflectedLength, 5.0f) ||
        !TestFloatNear(reflectedDir.x, 0.6f) || !TestFloatNear(reflectedDir.y, -0.8f) ||
        !TestFloatNear(reflectedDir.z, 0.0f)) {
        failCode = 1;
    }

    hitSlot.damageHandler = &hitSlot;
    hit.hitNode = &hitSlot.node;
    entry.flags = 0;
    g_OptCatalog_CaptureHitSnapshotEnabled = 1;
    if (failCode == 0 &&
        (OptCatalog::CanSpawnThroughRay(&entry, &hit, &rayStart, &rayEnd, &rayLength,
                                        &reflectedLength, &reflectedDir) != 0 ||
         !TestFloatNear(rayLength, 5.0f) ||
         !TestFloatNear(g_OptCatalog_CapturedDamageSourcePos.x, rayStart.x) ||
         !TestFloatNear(g_OptCatalog_CapturedDamageHitPos.y, rayEnd.y))) {
        failCode = 2;
    }

    hit.pos = rayStart;
    if (failCode == 0 &&
        (OptCatalog::CanSpawnThroughRay(&entry, &hit, &rayStart, &rayEnd, &rayLength,
                                        &reflectedLength, &reflectedDir) != 2 ||
         rayLength != 0.0f)) {
        failCode = 3;
    }

    g_OptCatalog_CaptureHitSnapshotEnabled = oldCaptureEnabled;
    g_OptCatalog_CapturedDamageSourcePos = oldSourcePos;
    g_OptCatalog_CapturedDamageHitPos = oldHitPos;
    return failCode;
}

extern "C" int zweapon_optcatalog_compute_aim_pitch_for_target_smoke(void) {
    OptCatalogEntryDef entry = {};
    entry.range = 10.0f;
    entry.gravity = 1.0f;

    const zVec3 origin = {1.0f, 2.0f, 3.0f};
    const zVec3 inRangeTarget = {4.0f, 6.0f, 3.0f};
    const zVec3 unusedDirection = {99.0f, 88.0f, 77.0f};
    float distanceApprox = 0.0f;

    const float pitch = OptCatalog::ComputeAimPitchForTarget(
        &entry, &origin, &unusedDirection, &inRangeTarget, &distanceApprox);

    const float expectedDistance = 5.125f;
    const float expectedPitch =
        4.0f / expectedDistance + 0.239999995f * (expectedDistance / entry.range);
    if (!TestFloatNear(distanceApprox, expectedDistance) ||
        !TestFloatNear(pitch, expectedPitch)) {
        return 1;
    }

    const zVec3 outOfRangeTarget = {1.0f, 12.0f, 3.0f};
    distanceApprox = 0.0f;
    if (OptCatalog::ComputeAimPitchForTarget(&entry, &origin, nullptr, &outOfRangeTarget,
                                             &distanceApprox) != -1.0f ||
        !TestFloatNear(distanceApprox, 10.25f)) {
        return 2;
    }

    entry.flags = 0x2000;
    const float fallbackPitch = OptCatalog::ComputeAimPitchForTarget(
        &entry, &origin, nullptr, &outOfRangeTarget, &distanceApprox);
    if (!TestFloatNear(fallbackPitch, (10.0f / distanceApprox) + 0.239999995f)) {
        return 3;
    }

    entry.gravity = 0.0f;
    if (OptCatalog::ComputeAimPitchForTarget(&entry, &origin, nullptr, &inRangeTarget,
                                             &distanceApprox) != -1.0f) {
        return 4;
    }

    return 0;
}

extern "C" int zweapon_optcatalog_reflect_and_sort_impact_trace_list_smoke(void) {
    zVec3 origin = {0.0f, 0.0f, 0.0f};
    zVec3 targetNear = {1.0f, 0.0f, 0.0f};
    zVec3 targetMid = {2.0f, 0.0f, 0.0f};
    zVec3 targetFar = {3.0f, 0.0f, 0.0f};
    zVec3 velocityNear = {10.0f, 0.0f, 0.0f};
    zVec3 velocityMid = {20.0f, 0.0f, 0.0f};
    zVec3 velocityFar = {30.0f, 0.0f, 0.0f};

    PlayerProgressTargetSlotRuntime targets[3] = {};
    targets[0].targetPos = &targetFar;
    targets[0].targetVelocity = &velocityFar;
    targets[1].targetPos = &targetNear;
    targets[1].targetVelocity = &velocityNear;
    targets[2].targetPos = &targetMid;
    targets[2].targetVelocity = &velocityMid;

    int targetCount = 3;
    OptCatalogTrailRuntimeState runtime = {};
    runtime.spawnPos = &origin;
    runtime.pendingSpawnTargetCountPtr = &targetCount;
    runtime.pendingSpawnTargetListPtr = targets;

    float projections[3] = {};
    zVec3 direction = {};
    OptCatalog::ReflectAndSortImpactTraceList(&runtime, projections, &direction);

    if (!TestFloatNear(direction.x, 1.0f) || !TestFloatNear(direction.y, 0.0f) ||
        !TestFloatNear(direction.z, 0.0f)) {
        return 1;
    }

    if (targets[0].targetPos != &targetNear || targets[0].targetVelocity != &velocityNear ||
        targets[1].targetPos != &targetMid || targets[1].targetVelocity != &velocityMid ||
        targets[2].targetPos != &targetFar || targets[2].targetVelocity != &velocityFar) {
        return 2;
    }

    return TestFloatNear(projections[0], 1.0f) && TestFloatNear(projections[1], 2.0f) &&
                   TestFloatNear(projections[2], 3.0f)
               ? 0
               : 3;
}

extern "C" int zweapon_optcatalog_compute_trail_impact_response_smoke(void) {
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    const int oldMaskSlot = g_OptCatalog_DamageContextKind;
    const int oldCaptureEnabled = g_OptCatalog_CaptureHitSnapshotEnabled;
    const float oldRuntimeDelta = g_OptCatalogRuntimeDeltaTime;
    const int oldFeedbackHitCount = g_OptCatalog_DamageFeedbackHitCount;
    void *const oldFeedbackCallback = g_OptCatalogDamageFeedbackCallback;
    zClass_NodePartial *const oldTrackedNode = g_OptCatalogDamageFeedbackTrackedNode;
    PlayerProbeSampleCandidateBuffer *const oldPickBuffer = g_DiPickCandidateBuffer;
    zClassDiPickCandidateEntry *const oldPickCursor = g_DiPickCandidateCursor;
    const int oldStopAfterFirstHit = g_cls_di_StopAfterFirstHit;
    const int oldBreakOnFirstCandidate = g_cls_di_BreakOnFirstCandidate;
    const zTag4Partial oldVariantTag = g_Variant_CurrentTag;
    int *const oldMatrixIdentityFlagSlot = zMath::g_currentMatrixIdentityFlagSlot;
    float **const oldMatrixPtrSlot = zMath::g_currentMatrixPtrSlot;

    OptCatalogFxSpec impactFx[1] = {};
    OptCatalogEntryDef entry = {};
    entry.flags = 0x200000;
    entry.range = 10.0f;
    entry.damage = 8.0f;
    entry.damageMaskSlotIndex = 3;
    entry.impactFxTable = impactFx;

    zClass_NodeFreeListSlot projectileSlot = {};
    projectileSlot.node.flags = 0x10;
    OptCatalogTrailRuntimeState runtime = {};
    runtime.projectileNode = &projectileSlot.node;
    runtime.trailDistance = 5.0f;
    runtime.spawnScale = 2.0f;

    OptCatalogTrailNodeSlot segment = {};
    segment.node = &projectileSlot.node;
    segment.pos = {0.0f, 0.0f, 0.0f};
    zVec3 noHitTarget = {3.0f, 4.0f, 0.0f};
    zClass_WorldDataPartial emptyWorldData = {};
    zClass_NodePartial emptyWorld = {};
    emptyWorld.classData = &emptyWorldData;
    g_OptCatalogRuntimeWorld = &emptyWorld;

    int failCode = 0;
    if (OptCatalog::ComputeTrailImpactResponse(&entry, &runtime, &segment, &noHitTarget) != 0 ||
        !TestFloatNear(segment.scale, 5.0f) || (projectileSlot.node.flags & 0x10) == 0) {
        failCode = 1;
    }

    zVec3 selectTris[3][3] = {
        {
            {0.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
        },
        {
            {0.0f, 0.0f, 0.75f},
            {1.0f, 0.0f, 0.75f},
            {0.0f, 1.0f, 0.75f},
        },
        {
            {0.0f, 0.0f, -0.5f},
            {1.0f, 0.0f, -0.5f},
            {0.0f, 1.0f, -0.5f},
        },
    };
    int faceIndices[3] = {0, 1, 2};
    OptCatalogSurfaceMaterialRef surfaceRef = {};
    surfaceRef.impactSlot = 0;
    zModel_PickFaceUvData faceUvData = {};
    zModel_PickFaceEntry selectFaces[3] = {};
    zModel_PickFaceData selectFaceData[3] = {};
    zClass_Object3DDataPartial objectData = {};
    objectData.flags = 8;
    zClass_NodeFreeListSlot selectSlots[3] = {};
    for (std::int32_t i = 0; i < 3; ++i) {
        selectFaces[i].flagsAndVertexCount = 3;
        selectFaces[i].vertexIndices = faceIndices;
        selectFaces[i].faceUvData = &faceUvData;
        selectFaces[i].scenePayload =
            static_cast<zModel_PickFaceScenePayload *>(static_cast<void *>(&surfaceRef));
        selectFaceData[i].faceCount = 1;
        selectFaceData[i].faces = &selectFaces[i];
        selectFaceData[i].baseVertices = selectTris[i];

        selectSlots[i].node.flags = 0x114;
        selectSlots[i].node.nodeType = 0xff;
        selectSlots[i].node.classId = 5;
        selectSlots[i].node.classData = &objectData;
        selectSlots[i].node.userDataOrDiRef =
            static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&selectFaceData[i]));
        selectSlots[i].node.cachedBounds[0] = 0.0f;
        selectSlots[i].node.cachedBounds[1] = 0.0f;
        selectSlots[i].node.cachedBounds[2] = -1.0f;
        selectSlots[i].node.cachedBounds[3] = 1.0f;
        selectSlots[i].node.cachedBounds[4] = 1.0f;
        selectSlots[i].node.cachedBounds[5] = 1.0f;
    }

    int callbackContext = 987;
    OptCatalogDamageHandlerPartial handler = {};
    handler.hitCallback = &callbackContext;
    handler.hitContext = reinterpret_cast<void *>(TestOptCatalogHitCallback);
    selectSlots[1].damageHandler = &handler;

    zClass_NodePartial *selectChildren[3] = {
        &selectSlots[0].node, &selectSlots[1].node, &selectSlots[2].node};
    zWorldAreaPartial selectArea = {};
    selectArea.childCount = 3;
    selectArea.childList = selectChildren;
    zWorldAreaPartial *selectRows[1] = {&selectArea};
    zClass_WorldDataPartial selectWorldData = {};
    selectWorldData.originX = 0.0f;
    selectWorldData.originZ = -2.0f;
    selectWorldData.worldMaxX = 2.0f;
    selectWorldData.worldMaxZ = 2.0f;
    selectWorldData.areaCellSizeX = 2.0f;
    selectWorldData.areaCellSizeZ = 4.0f;
    selectWorldData.areaInvSizeX = 0.5f;
    selectWorldData.areaInvSizeZ = 0.25f;
    selectWorldData.areaGridColCount = 1;
    selectWorldData.areaGridRowCount = 1;
    selectWorldData.areaGridRows = selectRows;
    zClass_NodePartial selectWorld = {};
    selectWorld.classData = &selectWorldData;
    selectWorld.listCountB = 3;
    selectWorld.listB = selectChildren;

    segment.pos = {0.25f, 0.25f, 2.0f};
    segment.scale = 0.0f;
    zVec3 hitTarget = {0.25f, 0.25f, -2.0f};
    PlayerProbeSampleCandidateBuffer pickBuffer = {};
    g_DiPickCandidateBuffer = &pickBuffer;
    g_DiPickCandidateCursor = pickBuffer.entries;
    g_cls_di_StopAfterFirstHit = 0;
    g_cls_di_BreakOnFirstCandidate = 0;
    g_Variant_CurrentTag = {};
    int matrixFlags[8] = {};
    float matrix[12] = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    float matrixClone[8][12] = {};
    float *matrixSlots[8] = {};
    matrixFlags[0] = 1;
    matrixSlots[0] = matrix;
    for (std::int32_t i = 1; i < 8; ++i) {
        matrixSlots[i] = matrixClone[i];
    }
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_OptCatalogRuntimeWorld = &selectWorld;
    g_OptCatalogRuntimeDeltaTime = 0.5f;
    g_OptCatalog_CaptureHitSnapshotEnabled = 1;
    g_OptCatalogDamageFeedbackCallback = nullptr;
    g_OptCatalogDamageFeedbackTrackedNode = &projectileSlot.node;
    g_OptCatalog_DamageFeedbackHitCount = 0;
    g_TestHitCallbackCalls = 0;
    g_TestHitCallbackContext = nullptr;
    g_TestHitCallbackEntry = nullptr;
    g_TestHitCallbackEvent = nullptr;
    g_TestHitCallbackDamage = 0.0f;

    if (failCode == 0) {
        const int hitResult =
            OptCatalog::ComputeTrailImpactResponse(&entry, &runtime, &segment, &hitTarget);
        if (hitResult != 1) {
            failCode = 20;
        } else if (!TestFloatNear(segment.scale, 1.25f)) {
            failCode = 21;
        } else if (!TestFloatNear(runtime.trailBlend, 0.25f)) {
            failCode = 22;
        } else if (g_TestHitCallbackCalls != 1) {
            failCode = 23;
        } else if (g_TestHitCallbackContext != &callbackContext) {
            failCode = 24;
        } else if (g_TestHitCallbackEntry != &entry) {
            failCode = 25;
        } else if (g_TestHitCallbackEvent->hitNode != &selectSlots[1].node) {
            failCode = 26;
        } else if (!TestFloatNear(g_TestHitCallbackDamage, 2.0f)) {
            failCode = 27;
        } else if (g_OptCatalog_DamageFeedbackHitCount != 1) {
            failCode = 28;
        } else if (!TestFloatNear(g_OptCatalog_CapturedDamageSourcePos.z, 2.0f)) {
            failCode = 29;
        } else if (!TestFloatNear(g_OptCatalog_CapturedDamageHitPos.z, 0.75f)) {
            failCode = 30;
        } else if ((projectileSlot.node.flags & 0x10) == 0) {
            failCode = 31;
        }
    }

    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    g_OptCatalog_DamageContextKind = oldMaskSlot;
    g_OptCatalog_CaptureHitSnapshotEnabled = oldCaptureEnabled;
    g_OptCatalogRuntimeDeltaTime = oldRuntimeDelta;
    g_OptCatalog_DamageFeedbackHitCount = oldFeedbackHitCount;
    g_OptCatalogDamageFeedbackCallback = oldFeedbackCallback;
    g_OptCatalogDamageFeedbackTrackedNode = oldTrackedNode;
    g_DiPickCandidateBuffer = oldPickBuffer;
    g_DiPickCandidateCursor = oldPickCursor;
    g_cls_di_StopAfterFirstHit = oldStopAfterFirstHit;
    g_cls_di_BreakOnFirstCandidate = oldBreakOnFirstCandidate;
    g_Variant_CurrentTag = oldVariantTag;
    zMath::g_currentMatrixIdentityFlagSlot = oldMatrixIdentityFlagSlot;
    zMath::g_currentMatrixPtrSlot = oldMatrixPtrSlot;
    return failCode;
}

extern "C" int zweapon_optcatalog_update_trail_segment_visual_smoke(void) {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_Object3DDataPartial data = {};
    data.flags = 0x18;
    zClass_NodePartial node = {};
    node.classId = 5;
    node.classData = &data;

    OptCatalogTrailNodeSlot segment = {};
    segment.node = &node;
    segment.pos = {3.0f, 4.0f, 5.0f};
    segment.dir = {0.6f, 0.5f, -0.8f};
    segment.scale = 7.0f;

    OptCatalog::UpdateTrailSegmentVisual(&segment);

    int failCode = 0;
    if ((node.flags & 0x04) == 0) {
        failCode = 1;
    } else if (!TestFloatNear(data.localMatrix[9], 3.0f) ||
               !TestFloatNear(data.localMatrix[10], 4.0f) ||
               !TestFloatNear(data.localMatrix[11], 5.0f)) {
        failCode = 2;
    } else if (!TestFloatNear(data.rotation.x, static_cast<float>(asin(0.5))) ||
               !TestFloatNear(data.rotation.y, static_cast<float>(atan2(-0.6, 0.8))) ||
               !TestFloatNear(data.rotation.z, 0.0f)) {
        failCode = 3;
    } else if (!TestFloatNear(data.scale.x, 1.0f) ||
               !TestFloatNear(data.scale.y, 1.0f) ||
               !TestFloatNear(data.scale.z, 7.0f)) {
        failCode = 4;
    } else if ((data.flags & 0x18) != 0 || (node.flags & 0x02000003) != 0x02000003) {
        failCode = 5;
    }

    for (zClass_TypeListLink *link = zClass_TypeList::Head(7); link != nullptr;) {
        zClass_TypeListLink *next = link->next;
        std::free(link);
        link = next;
    }
    zClass_TypeList::Head(7) = nullptr;
    zClass_TypeList::Tail(7) = nullptr;
    return failCode;
}

extern "C" int zweapon_optcatalog_runtime_free_list_helpers_smoke(void) {
    void *const oldFreeRuntimeList = g_OptCatalogFreeRuntimeInstanceList;
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    const float oldNextSpawnScale = g_OptCatalogNextSpawnScale;

    OptCatalogEntryDef entry = {};
    zClass_NodePartial cloneA = {};
    zClass_NodePartial cloneB = {};
    cloneA.callbackContext = &cloneB;
    entry.attachCloneChildFreeList = &cloneA;

    zClass_NodePartial *const reusedClone = OptCatalog::AllocOrReuseAttachNodeChildClone(&entry);
    if (reusedClone != &cloneA || entry.attachCloneChildFreeList != &cloneB ||
        cloneA.callbackContext != nullptr) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 1;
    }

    OptCatalogRuntimeInstanceStorage runtimeA = {};
    OptCatalogRuntimeInstanceStorage runtimeB = {};
    zClass_NodePartial projectileA = {};
    zClass_Object3DDataPartial projectileDataA = {};
    projectileA.classId = 5;
    projectileA.classData = &projectileDataA;
    zClass_NodePartial attachNode = {};
    runtimeA.next = &runtimeB;
    runtimeA.projectileNode = &projectileA;
    runtimeA.lifetime = 9.0f;
    runtimeA.updateCallback = &entry;
    entry.attachCloneTemplateNode = &attachNode;
    entry.flyoutModelAnimationEntry = nullptr;
    g_OptCatalogFreeRuntimeInstanceList = &runtimeA;

    OptCatalogRuntimeInstanceStorage *const directRuntime =
        OptCatalog::AllocOrReuseAttachNodeClone(&entry);
    const bool directAttachOk = directRuntime == &runtimeA &&
                                g_OptCatalogFreeRuntimeInstanceList == &runtimeB &&
                                runtimeA.attachCloneChild == nullptr && runtimeA.lifetime == 0.0f &&
                                runtimeA.updateCallback == nullptr && projectileA.listCountB == 1 &&
                                projectileA.listB[0] == &attachNode && attachNode.listCountA == 1 &&
                                attachNode.listA[0] == &projectileA;
    zClass_Object3D::RemoveChild(&projectileA, &attachNode);
    if (!directAttachOk) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 2;
    }

    zClass_NodePartial projectileB = {};
    zClass_Object3DDataPartial projectileDataB = {};
    projectileB.classId = 5;
    projectileB.classData = &projectileDataB;
    runtimeB.next = nullptr;
    runtimeB.projectileNode = &projectileB;
    runtimeB.attachCloneChild = nullptr;
    runtimeB.lifetime = 4.0f;
    runtimeB.updateCallback = &runtimeA;
    cloneA.callbackContext = &cloneB;
    entry.attachCloneChildFreeList = &cloneA;
    entry.flyoutModelAnimationEntry = reinterpret_cast<zEffectAnimEntry *>(&entry);
    g_OptCatalogFreeRuntimeInstanceList = &runtimeB;

    OptCatalogRuntimeInstanceStorage *const clonedRuntime =
        OptCatalog::AllocOrReuseAttachNodeClone(&entry);
    const bool clonedAttachOk =
        clonedRuntime == &runtimeB && g_OptCatalogFreeRuntimeInstanceList == nullptr &&
        runtimeB.attachCloneChild == &cloneA && entry.attachCloneChildFreeList == &cloneB &&
        cloneA.callbackContext == nullptr && projectileB.listCountB == 1 &&
        projectileB.listB[0] == &cloneA && cloneA.listCountA == 1 &&
        cloneA.listA[0] == &projectileB && runtimeB.lifetime == 0.0f &&
        runtimeB.updateCallback == nullptr;
    if (!clonedAttachOk) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 3;
    }

    zClass_NodePartial asyncRuntimeNode = {};
    zEffectAnimEntry asyncEffect = {};
    asyncEffect.activationState = 2;
    asyncEffect.runtimeNode = &asyncRuntimeNode;
    asyncEffect.triggerBaseValue = 1.0f;
    asyncEffect.triggerCurrentValue = 8.0f;
    runtimeB.asyncFxHandle = &asyncEffect;
    OptCatalog::RecycleAttachNodeClone(&entry, &runtimeB);
    if (runtimeB.attachCloneChild != nullptr) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 40;
    }
    if (entry.attachCloneChildFreeList != &cloneA || cloneA.callbackContext != &cloneB) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 41;
    }
    if (projectileB.listCountB != 0 || cloneA.listCountA != 0) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 42;
    }
    if (asyncEffect.triggerCurrentValue != 0.0f) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 43;
    }

    runtimeB.asyncFxHandle = reinterpret_cast<zEffectAnimEntry *>(&runtimeA);
    OptCatalog::ClearRuntimeInstanceAsyncFxHandleCallback(nullptr, &runtimeB, nullptr);
    const bool clearCallbackOk = runtimeB.asyncFxHandle == nullptr;
    if (!clearCallbackOk) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 5;
    }

    OptCatalogRuntimeInstanceStorage runtimeC = {};
    zClass_NodeFreeListSlot projectileSlot = {};
    zClass_Object3DDataPartial projectileDataC = {};
    projectileSlot.node.classId = 5;
    projectileSlot.node.classData = &projectileDataC;
    runtimeC.projectileNode = &projectileSlot.node;
    runtimeC.lifetime = 3.0f;
    runtimeC.next = nullptr;
    g_OptCatalogFreeRuntimeInstanceList = &runtimeC;

    zClass_NodePartial runtimeWorld = {};
    runtimeWorld.classId = 3;
    g_OptCatalogRuntimeWorld = &runtimeWorld;
    g_OptCatalogNextSpawnScale = 2.5f;
    entry.attachCloneTemplateNode = nullptr;
    entry.flyoutModelAnimationEntry = nullptr;
    entry.activeRuntimeListHead = &runtimeA;
    entry.flyoutHealth = 7.5f;
    zClass_NodePartial ownerNode = {};
    zVec3 spawnPos = {11.0f, 12.0f, 13.0f};

    OptCatalogRuntimeInstanceStorage *const spawnedRuntime =
        OptCatalog::SpawnRuntimeInstanceAt(&entry, &spawnPos, &ownerNode);
    const bool spawnOk =
        spawnedRuntime == &runtimeC && entry.activeRuntimeListHead == &runtimeC &&
        runtimeC.next == &runtimeA && runtimeC.pos.x == 11.0f && runtimeC.pos.y == 12.0f &&
        runtimeC.pos.z == 13.0f && runtimeC.lifetime == 0.0f && runtimeC.ownerNode == &ownerNode &&
        runtimeC.spawnScale == 2.5f && g_OptCatalogNextSpawnScale == 1.0f &&
        (projectileSlot.node.flags & 0x08000000) != 0 &&
        projectileSlot.damageHandler == reinterpret_cast<void *>(1) &&
        projectileSlot.node.callbackContext == reinterpret_cast<zClass_NodePartial *>(&runtimeC) &&
        runtimeC.projectileScale == 7.5f && projectileDataC.localMatrix[9] == 11.0f &&
        projectileDataC.localMatrix[10] == 12.0f && projectileDataC.localMatrix[11] == 13.0f &&
        runtimeWorld.listCountB == 1 && runtimeWorld.listB[0] == &projectileSlot.node;
    zClass_Class::RemoveChild(&runtimeWorld, &projectileSlot.node);
    if (!spawnOk) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 6;
    }

    OptCatalogRuntimeInstanceStorage freeSentinel = {};
    OptCatalogRuntimeInstanceStorage runtimeD = {};
    zClass_NodePartial parentD = {};
    zClass_NodeFreeListSlot projectileSlotD = {};
    zClass_Object3DDataPartial projectileDataD = {};
    zClass_NodePartial templateChildD = {};
    zClass_NodePartial childD = {};
    projectileSlotD.node.classId = 5;
    projectileSlotD.node.classData = &projectileDataD;
    projectileSlotD.damageHandler = &entry;
    runtimeD.projectileNode = &projectileSlotD.node;
    runtimeD.lifetime = 0.0f;
    entry.attachCloneTemplateNode = &templateChildD;
    g_OptCatalogFreeRuntimeInstanceList = &freeSentinel;
    zClass_Class::AddChild(&parentD, &projectileSlotD.node);
    zClass_Object3D::gwObject3DAddChild(&projectileSlotD.node, &templateChildD);
    zClass_Object3D::gwObject3DAddChild(&projectileSlotD.node, &childD);

    OptCatalog::RecycleRuntimeInstanceStorage(&entry, &runtimeD);
    const bool recycleRuntimeOk =
        g_OptCatalogFreeRuntimeInstanceList == &runtimeD && runtimeD.next == &freeSentinel &&
        parentD.listCountB == 0 && projectileSlotD.node.listCountA == 0 &&
        projectileSlotD.node.listCountB == 0 && templateChildD.listCountA == 0 &&
        childD.listCountA == 0 && projectileSlotD.damageHandler == nullptr &&
        projectileDataD.scale.x == 1.0f && projectileDataD.scale.y == 1.0f &&
        projectileDataD.scale.z == 1.0f && projectileDataD.localMatrix[9] == 0.0f &&
        projectileDataD.localMatrix[10] == 0.0f && projectileDataD.localMatrix[11] == 0.0f;
    if (!recycleRuntimeOk) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 7;
    }

    OptCatalogRuntimeInstanceStorage freeSentinelE = {};
    OptCatalogRuntimeInstanceStorage runtimeE = {};
    zClass_NodePartial runtimeWorldE = {};
    zClass_NodeFreeListSlot projectileSlotE = {};
    zClass_Object3DDataPartial projectileDataE = {};
    runtimeWorldE.classId = 3;
    projectileSlotE.node.classId = 5;
    projectileSlotE.node.classData = &projectileDataE;
    projectileSlotE.damageHandler = &entry;
    runtimeE.projectileNode = &projectileSlotE.node;
    runtimeE.lifetime = 12.0f;
    runtimeE.flyoutAnimPrimary = nullptr;
    runtimeE.flyoutAnimSecondary = nullptr;
    g_OptCatalogRuntimeWorld = &runtimeWorldE;
    g_OptCatalogFreeRuntimeInstanceList = &freeSentinelE;
    zClass_Class::AddChild(&runtimeWorldE, &projectileSlotE.node);

    OptCatalog::RecycleRuntimeInstance(&entry, &runtimeE);
    const bool recycleInstanceOk =
        runtimeE.lifetime == 0.0f && runtimeWorldE.listCountB == 0 &&
        projectileSlotE.node.listCountA == 0 &&
        g_OptCatalogFreeRuntimeInstanceList == &runtimeE && runtimeE.next == &freeSentinelE &&
        projectileSlotE.damageHandler == nullptr && projectileDataE.scale.x == 1.0f &&
        projectileDataE.scale.y == 1.0f && projectileDataE.scale.z == 1.0f;
    if (!recycleInstanceOk) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 8;
    }

    OptCatalogRuntimeInstanceStorage runtimeF = {};
    OptCatalogRuntimeInstanceStorage runtimeG = {};
    OptCatalogRuntimeInstanceStorage freeSentinelF = {};
    zClass_NodeFreeListSlot projectileSlotF = {};
    zClass_Object3DDataPartial projectileDataF = {};
    zClass_NodeFreeListSlot projectileSlotG = {};
    zClass_Object3DDataPartial projectileDataG = {};
    zClass_NodePartial runtimeWorldF = {};
    runtimeWorldF.classId = 3;
    projectileSlotF.node.classId = 5;
    projectileSlotF.node.classData = &projectileDataF;
    projectileSlotG.node.classId = 5;
    projectileSlotG.node.classData = &projectileDataG;
    runtimeF.next = &runtimeG;
    runtimeF.projectileNode = &projectileSlotF.node;
    runtimeG.next = nullptr;
    runtimeG.projectileNode = &projectileSlotG.node;
    entry.activeRuntimeListHead = &runtimeF;
    g_OptCatalogRuntimeWorld = &runtimeWorldF;
    g_OptCatalogFreeRuntimeInstanceList = &freeSentinelF;
    zClass_Class::AddChild(&runtimeWorldF, &projectileSlotF.node);
    zClass_Class::AddChild(&runtimeWorldF, &projectileSlotG.node);

    OptCatalog::ClearRuntimeInstances(&entry);
    const bool clearInstancesOk =
        entry.activeRuntimeListHead == nullptr && runtimeWorldF.listCountB == 0 &&
        projectileSlotF.node.listCountA == 0 && projectileSlotG.node.listCountA == 0 &&
        g_OptCatalogFreeRuntimeInstanceList == &runtimeG && runtimeG.next == &runtimeF &&
        runtimeF.next == &freeSentinelF && projectileDataF.scale.x == 1.0f &&
        projectileDataG.scale.x == 1.0f;
    if (!clearInstancesOk) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 9;
    }

    void *const oldPendingSpawnTargetCountPtr = g_OptCatalogPendingSpawnTargetCountPtr;
    void *const oldPendingSpawnTargetListPtr = g_OptCatalogPendingSpawnTargetListPtr;
    const int oldNetworkOptionState = g_OptCatalogNetworkOptionState;
    OptCatalogAllocRuntimeGateCallback const oldAllocRuntimeGateCallback =
        g_OptCatalog_AllocRuntimeGateCallback;

    OptCatalogEntryDef allocEntry = {};
    OptCatalogRuntimeInstanceStorage previousRuntime = {};
    OptCatalogRuntimeInstanceStorage allocRuntime = {};
    zClass_NodeFreeListSlot allocProjectileSlot = {};
    zClass_Object3DDataPartial allocProjectileData = {};
    zClass_NodePartial allocRuntimeWorld = {};
    zClass_NodePartial allocOwner = {};
    allocRuntimeWorld.classId = 3;
    allocProjectileSlot.node.classId = 5;
    allocProjectileSlot.node.classData = &allocProjectileData;
    allocRuntime.projectileNode = &allocProjectileSlot.node;
    allocEntry.activeRuntimeListHead = &previousRuntime;
    allocEntry.velocity = 3.0f;
    allocEntry.flags = 0x00400004u;
    allocEntry.flyoutHealth = 6.5f;
    allocEntry.fireFxSelectedSoundIndex = -1;
    allocEntry.fireFxSelectedEffectIndex = -1;
    allocEntry.flyoutSelectedEffectIndex = -1;

    zTag4Partial allocVariant = {};
    allocVariant.count = 2;
    allocVariant.tags[0] = 9;
    allocVariant.tags[1] = 8;
    allocVariant.tags[2] = 7;
    unsigned int expectedVariant = 0;
    std::memcpy(&expectedVariant, &allocVariant, sizeof(expectedVariant));
    zVec3 allocSpawnPos = {11.0f, 12.0f, 13.0f};
    zVec3 allocSpawnDir = {0.0f, 0.0f, 1.0f};
    zVec3 allocSpawnVelocity = {1.0f, 2.0f, 3.0f};
    int saveState = 1234;
    int pendingTargetCount = 1;
    zVec3 pendingTargetPos = {20.0f, 0.0f, 0.0f};
    zVec3 pendingTargetVelocity = {0.5f, 0.0f, 0.0f};
    PlayerProgressTargetSlotRuntime pendingTarget = {&pendingTargetPos, &pendingTargetVelocity};
    g_OptCatalogRuntimeWorld = &allocRuntimeWorld;
    g_OptCatalogNextSpawnScale = 4.5f;
    g_OptCatalogNetworkOptionState = 0;
    g_OptCatalog_AllocRuntimeGateCallback = nullptr;
    g_OptCatalogPendingSpawnTargetCountPtr = &pendingTargetCount;
    g_OptCatalogPendingSpawnTargetListPtr = &pendingTarget;

    OptCatalogRuntimeInstanceStorage *const allocatedRuntime = OptCatalog::AllocRuntimeInstance(
        &allocEntry, &allocOwner, &allocVariant, &allocSpawnPos, &allocSpawnDir,
        &allocSpawnVelocity, &saveState, &allocRuntime);
    const bool allocRuntimeOk =
        allocatedRuntime == &allocRuntime && allocEntry.activeRuntimeListHead == &allocRuntime &&
        allocRuntime.next == &previousRuntime && allocRuntime.ownerNode == &allocOwner &&
        allocRuntime.origin.x == 11.0f && allocRuntime.origin.y == 12.0f &&
        allocRuntime.origin.z == 13.0f && allocRuntime.pos.x == 11.0f &&
        allocRuntime.pos.y == 12.0f && allocRuntime.pos.z == 13.0f &&
        allocRuntime.dir.z == 1.0f && allocRuntime.rangeProgress == 0.0f &&
        allocRuntime.scaleFade == 0.0f && allocRuntime.saveState == &saveState &&
        allocRuntime.variantTag == expectedVariant && allocRuntime.spawnScale == 4.5f &&
        g_OptCatalogNextSpawnScale == 1.0f && allocRuntime.speed == 3.0f &&
        allocRuntime.lifetime == 3.0f && allocRuntime.velocity.x == 1.0f &&
        allocRuntime.velocity.y == 2.0f && allocRuntime.velocity.z == 6.0f &&
        allocRuntime.aux.x == 1.0f && allocRuntime.aux.y == 2.0f &&
        allocRuntime.aux.z == 3.0f && allocRuntime.pendingTargetA == &pendingTargetPos &&
        allocRuntime.pendingTargetB == &pendingTargetVelocity &&
        g_OptCatalogPendingSpawnTargetCountPtr == nullptr &&
        (allocProjectileSlot.node.flags & 0x08000000) != 0 &&
        allocProjectileSlot.damageHandler == reinterpret_cast<void *>(1) &&
        allocProjectileSlot.node.callbackContext ==
            reinterpret_cast<zClass_NodePartial *>(&allocRuntime) &&
        allocRuntime.projectileScale == 6.5f && allocProjectileData.localMatrix[9] == 11.0f &&
        allocProjectileData.localMatrix[10] == 12.0f &&
        allocProjectileData.localMatrix[11] == 13.0f && allocRuntimeWorld.listCountB == 1 &&
        allocRuntimeWorld.listB[0] == &allocProjectileSlot.node &&
        allocEntry.fireFxSelectedSoundIndex == 0 && allocEntry.fireFxSelectedEffectIndex == 0 &&
        allocEntry.flyoutSelectedEffectIndex == 0;
    zClass_Class::RemoveChild(&allocRuntimeWorld, &allocProjectileSlot.node);
    if (!allocRuntimeOk) {
        g_OptCatalogPendingSpawnTargetCountPtr = oldPendingSpawnTargetCountPtr;
        g_OptCatalogPendingSpawnTargetListPtr = oldPendingSpawnTargetListPtr;
        g_OptCatalogNetworkOptionState = oldNetworkOptionState;
        g_OptCatalog_AllocRuntimeGateCallback = oldAllocRuntimeGateCallback;
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 10;
    }

    g_TestAllocRuntimeGateCalls = 0;
    g_TestAllocRuntimeGateSaveStateSlot = nullptr;
    g_OptCatalogNetworkOptionState = 1;
    g_OptCatalog_AllocRuntimeGateCallback = TestOptCatalogAllocRuntimeGateCallback;
    allocEntry.activeRuntimeListHead = &previousRuntime;
    void *const blockedSaveState = &saveState;
    OptCatalogRuntimeInstanceStorage *const blockedRuntime = OptCatalog::AllocRuntimeInstance(
        &allocEntry, &allocOwner, nullptr, &allocSpawnPos, &allocSpawnDir, &allocSpawnVelocity,
        blockedSaveState, &allocRuntime);
    const bool gateOk = blockedRuntime == nullptr && allocEntry.activeRuntimeListHead ==
                                                      &previousRuntime &&
                        g_TestAllocRuntimeGateCalls == 1 &&
                        g_TestAllocRuntimeGateSaveStateSlot != nullptr &&
                        *g_TestAllocRuntimeGateSaveStateSlot == blockedSaveState;
    if (!gateOk) {
        g_OptCatalogPendingSpawnTargetCountPtr = oldPendingSpawnTargetCountPtr;
        g_OptCatalogPendingSpawnTargetListPtr = oldPendingSpawnTargetListPtr;
        g_OptCatalogNetworkOptionState = oldNetworkOptionState;
        g_OptCatalog_AllocRuntimeGateCallback = oldAllocRuntimeGateCallback;
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 11;
    }

    g_OptCatalogPendingSpawnTargetCountPtr = oldPendingSpawnTargetCountPtr;
    g_OptCatalogPendingSpawnTargetListPtr = oldPendingSpawnTargetListPtr;
    g_OptCatalogNetworkOptionState = oldNetworkOptionState;
    g_OptCatalog_AllocRuntimeGateCallback = oldAllocRuntimeGateCallback;
    g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    g_OptCatalogNextSpawnScale = oldNextSpawnScale;
    return 0;
}

extern "C" int light_alloc_from_free_list_and_attach_smoke(void) {
    zClass_NodePartial *const oldThermalGlowFreeList = g_OptCatalogThermalGlowFreeList;
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;

    g_OptCatalogThermalGlowFreeList = nullptr;
    zColorRgb color = {0.25f, 0.5f, 0.75f};
    if (Light::AllocFromFreeListAndAttach(&color) != nullptr) {
        g_OptCatalogThermalGlowFreeList = oldThermalGlowFreeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        return 1;
    }

    zClass_NodePartial light = {};
    zClass_NodePartial nextLight = {};
    zClass_LightDataPartial lightData = {};
    light.classData = &lightData;
    light.callbackContext = &nextLight;

    zClass_WorldDataPartial worldData = {};
    zClass_NodePartial world = {};
    world.classData = &worldData;

    g_OptCatalogThermalGlowFreeList = &light;
    g_OptCatalogRuntimeWorld = &world;

    zClass_NodePartial *const allocated = Light::AllocFromFreeListAndAttach(&color);
    const bool ok = allocated == &light && g_OptCatalogThermalGlowFreeList == &nextLight &&
                    lightData.range1 == 0.1f && lightData.range2 == 0.2f &&
                    lightData.specularColor.red == 0.25f && lightData.specularColor.green == 0.5f &&
                    lightData.specularColor.blue == 0.75f && worldData.lightCount == 1 &&
                    worldData.lightNodes[0] == &light && worldData.lightDataList[0] == &lightData &&
                    lightData.attachedWorldCount == 1 && lightData.attachedWorlds[0] == &world;

    Light::ReturnToFreeList(&light);
    const bool returnedOk = g_OptCatalogThermalGlowFreeList == &light &&
                            light.callbackContext == &nextLight && worldData.lightCount == 0 &&
                            lightData.attachedWorldCount == 0 && lightData.range1 == 0.1f &&
                            lightData.range2 == 0.2f;

    std::free(worldData.lightNodes);
    std::free(worldData.lightDataList);
    std::free(lightData.attachedWorlds);
    g_OptCatalogThermalGlowFreeList = oldThermalGlowFreeList;
    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    return ok && returnedOk ? 0 : 2;
}

extern "C" int light_init_thermal_glow_pool_smoke(void) {
    zClass_NodePartial *const oldThermalGlowFreeList = g_OptCatalogThermalGlowFreeList;
    zClass_NodeFreeListSlot *const oldNodeArray = g_zClass_NodeArray;
    const int oldFreeHead = g_zClass_NodeFreeHeadIndex;
    const int oldActiveNodeCount = g_zClass_ActiveNodeCount;
    const int oldDeferredProcessing = g_zClass_DeferredProcessingEnabled;

    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slots[8] = {};
    for (int i = 0; i < 7; ++i) {
        slots[i].freeTag = static_cast<unsigned int>(i + 1);
    }
    slots[7].freeTag = 0x00ffffff;

    g_zClass_NodeArray = slots;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;
    g_OptCatalogThermalGlowFreeList = nullptr;

    int failCode = 0;
    if (Light::InitThermalGlowPool() != 1) {
        failCode = 1;
    } else if (g_zClass_NodeFreeHeadIndex != -1 || g_zClass_ActiveNodeCount != 8 ||
               g_OptCatalogThermalGlowFreeList != &slots[7].node) {
        failCode = 2;
    } else {
        zClass_NodePartial *node = g_OptCatalogThermalGlowFreeList;
        for (int i = 7; i >= 0; --i) {
            zClass_LightDataPartial *data =
                static_cast<zClass_LightDataPartial *>(slots[i].node.classData);
            if (node != &slots[i].node || std::strcmp(node->name, "Thermal glow") != 0 ||
                node->classId != 9 || data == nullptr || data->localPosition.x != 0.0f ||
                data->localPosition.y != 0.0f || data->localPosition.z != 0.0f ||
                data->range1 != 0.1f || data->range2 != 0.2f) {
                failCode = 3;
                break;
            }

            node = node->callbackContext;
        }

        if (failCode == 0 && node != nullptr) {
            failCode = 4;
        }
    }

    if (failCode == 0 && Light::DestroyThermalGlowPool() != 1) {
        failCode = 5;
    } else if (failCode == 0 &&
               (g_OptCatalogThermalGlowFreeList != nullptr || g_zClass_ActiveNodeCount != 0)) {
        failCode = 6;
    }

    zClass_TypeList::FreeAll();
    g_OptCatalogThermalGlowFreeList = oldThermalGlowFreeList;
    g_zClass_NodeArray = oldNodeArray;
    g_zClass_NodeFreeHeadIndex = oldFreeHead;
    g_zClass_ActiveNodeCount = oldActiveNodeCount;
    g_zClass_DeferredProcessingEnabled = oldDeferredProcessing;
    return failCode;
}

extern "C" int player_timed_hit_status_smoke(void) {
    zClass_NodePartial parent = {};
    zClass_NodePartial oldHitSourceNode = {};
    OptCatalogEntryDef oldHitSource = {};
    PlayerTimedHitStatus status = {};
    status.runtimeFlags = 0xff;
    status.hitSource = &oldHitSource;
    status.currentLevel = 0.5f;
    status.targetLevel = -0.5f;
    status.lightNode = &oldHitSourceNode;
    status.nextUpdateTime = 10.0f;
    status.lightParentNode = &parent;

    status.ResetFields();
    if (status.runtimeFlags != 0xfcu || status.hitSource != &oldHitSource ||
        status.currentLevel != 0.0f || status.targetLevel != 0.0f ||
        status.lightNode != nullptr || status.nextUpdateTime != 0.0f ||
        status.lightParentNode != &parent) {
        return 1;
    }

    status.runtimeFlags = 3;
    status.currentLevel = 0.25f;
    status.targetLevel = 0.75f;
    status.nextUpdateTime = 1.0f;
    status.ClearLightAndReset();
    if (status.runtimeFlags != 3 || status.currentLevel != 0.25f ||
        status.targetLevel != 0.75f || status.nextUpdateTime != 1.0f) {
        return 2;
    }

    zClass_NodePartial *const oldThermalGlowFreeList = g_OptCatalogThermalGlowFreeList;
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;

    zClass_NodePartial light = {};
    zClass_NodePartial nextLight = {};
    zClass_LightDataPartial lightData = {};
    light.classData = &lightData;
    light.callbackContext = &nextLight;

    zClass_WorldDataPartial worldData = {};
    zClass_NodePartial world = {};
    world.classData = &worldData;
    parent.classId = 3;

    OptCatalogEntryDef source = {};
    source.timedStatusLightSpecularColor = {0.2f, 0.4f, 0.6f};
    status = {};
    status.currentLevel = -0.75f;
    status.targetLevel = 0.9f;
    status.lightParentNode = &parent;
    g_OptCatalogThermalGlowFreeList = &light;
    g_OptCatalogRuntimeWorld = &world;

    const int lowBand = HitSource::UpdateTimedStatus(&source, &status, 0.4f);
    const bool allocOk =
        lowBand == 2 && status.runtimeFlags == 3 && status.hitSource == &source &&
        status.targetLevel == 1.0f && status.lightNode == &light &&
        g_OptCatalogThermalGlowFreeList == &nextLight && lightData.range1 == 0.1f &&
        lightData.range2 == 0.2f && lightData.specularColor.red == 0.2f &&
        lightData.specularColor.green == 0.4f && lightData.specularColor.blue == 0.6f &&
        worldData.lightCount == 1 && parent.listCountB == 1 && parent.listB[0] == &light;
    if (!allocOk) {
        std::free(worldData.lightNodes);
        std::free(worldData.lightDataList);
        std::free(lightData.attachedWorlds);
        std::free(parent.listB);
        std::free(light.listA);
        g_OptCatalogThermalGlowFreeList = oldThermalGlowFreeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        return 3;
    }

    status.ClearLightAndReset();
    const bool clearOk = status.runtimeFlags == 0 && status.lightNode == nullptr &&
                         status.currentLevel == 0.0f && status.targetLevel == 0.0f &&
                         status.nextUpdateTime == 0.0f &&
                         g_OptCatalogThermalGlowFreeList == &light &&
                         worldData.lightCount == 0 && parent.listCountB == 0;
    if (!clearOk) {
        std::free(worldData.lightNodes);
        std::free(worldData.lightDataList);
        std::free(lightData.attachedWorlds);
        std::free(parent.listB);
        std::free(light.listA);
        g_OptCatalogThermalGlowFreeList = oldThermalGlowFreeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        return 4;
    }

    source.flags = 0x200;
    status = {};
    status.currentLevel = 0.25f;
    status.targetLevel = -0.8f;
    status.lightNode = &nextLight;
    status.lightParentNode = &parent;
    const int highBand = HitSource::UpdateTimedStatus(&source, &status, 0.5f);
    status.currentLevel = 0.0f;
    const int middleBand = HitSource::UpdateTimedStatus(&source, &status, 0.1f);
    const bool clampAndBandOk = highBand == 0 && middleBand == 1 &&
                                status.targetLevel == -1.0f && status.lightNode == &nextLight;
    if (!clampAndBandOk) {
        std::free(worldData.lightNodes);
        std::free(worldData.lightDataList);
        std::free(lightData.attachedWorlds);
        std::free(parent.listB);
        std::free(light.listA);
        g_OptCatalogThermalGlowFreeList = oldThermalGlowFreeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        return 5;
    }

    const float oldFrameDeltaTimeSec = g_FrameDeltaTimeSec;
    const float oldAccumulatedTimeSec = g_Time_AccumulatedTimeSec;
    const int oldApproxExpNegDirty = g_zMath_ApproxExpNegDirty;

    zClass_NodePartial tickLight = {};
    zClass_LightDataPartial tickLightData = {};
    tickLight.classData = &tickLightData;
    source.timedStatusInterpRate = 1.0f;
    source.timedStatusLightRangeMin = 2.0f;
    source.timedStatusLightRangeMax = 6.0f;
    source.timedStatusUpdateDelay = 1.5f;
    source.timedStatusLightSpecularColor = {0.3f, 0.5f, 0.7f};
    status = {};
    status.runtimeFlags = 2;
    status.hitSource = &source;
    status.currentLevel = 0.25f;
    status.targetLevel = -0.75f;
    status.lightNode = &tickLight;
    status.nextUpdateTime = 0.25f;
    g_FrameDeltaTimeSec = 0.5f;
    g_Time_AccumulatedTimeSec = 4.0f;

    const int interpolatingBand = status.TickAndUpdateLight(2.0f);
    const bool interpolationOk =
        interpolatingBand == 1 && status.runtimeFlags == 2 &&
        TestFloatNear(status.currentLevel, -0.25f) &&
        TestFloatNear(status.nextUpdateTime, 5.5f) &&
        TestFloatNear(tickLightData.range1, 1.0f) &&
        TestFloatNear(tickLightData.range2, 3.0f) &&
        TestFloatNear(tickLightData.specularColor.red, 0.3f) &&
        TestFloatNear(tickLightData.specularColor.green, 0.5f) &&
        TestFloatNear(tickLightData.specularColor.blue, 0.7f);

    zClass_NodePartial decayLight = {};
    zClass_LightDataPartial decayLightData = {};
    decayLight.classData = &decayLightData;
    status = {};
    status.hitSource = &source;
    status.currentLevel = 0.5f;
    status.targetLevel = 0.9f;
    status.lightNode = &decayLight;
    status.nextUpdateTime = 3.0f;
    g_FrameDeltaTimeSec = 0.5f;
    g_Time_AccumulatedTimeSec = 4.0f;

    const int decayBand = status.TickAndUpdateLight(1.5f);
    const float expectedFade = g_zMath_ApproxExpNegTable[19] * 0.5f;
    const float expectedScale = 1.5f * expectedFade;
    const bool decayOk = decayBand == 0 && TestFloatNear(status.currentLevel, expectedFade) &&
                         TestFloatNear(status.targetLevel, expectedFade) &&
                         TestFloatNear(decayLightData.range1, 2.0f * expectedScale) &&
                         TestFloatNear(decayLightData.range2, 6.0f * expectedScale);

    status = {};
    status.hitSource = &source;
    status.currentLevel = -0.25f;
    status.targetLevel = -0.25f;
    status.nextUpdateTime = 10.0f;
    g_Time_AccumulatedTimeSec = 4.0f;
    const int waitingBand = status.TickAndUpdateLight(1.0f);
    const bool waitingOk = waitingBand == 1 && status.currentLevel == -0.25f &&
                           status.targetLevel == -0.25f;

    g_FrameDeltaTimeSec = oldFrameDeltaTimeSec;
    g_Time_AccumulatedTimeSec = oldAccumulatedTimeSec;
    g_zMath_ApproxExpNegDirty = oldApproxExpNegDirty;

    std::free(worldData.lightNodes);
    std::free(worldData.lightDataList);
    std::free(lightData.attachedWorlds);
    std::free(parent.listB);
    std::free(light.listA);
    g_OptCatalogThermalGlowFreeList = oldThermalGlowFreeList;
    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    if (!interpolationOk) {
        return 6;
    }
    if (!decayOk) {
        return 7;
    }
    return waitingOk ? 0 : 8;
}

extern "C" int zweapon_optcatalog_warning_samples_smoke(void) {
    zSndSample trigger = {};
    zSndSample weapon = {};
    zSndSample noAmmo = {};

    g_OptCatalogSndTriggerInactive = &trigger;
    g_OptCatalogSndWeaponInactive = &weapon;
    g_OptCatalogSndNoAmmoWarning = &noAmmo;
    g_zSnd_IsInitialized = 0;
    g_zSnd_PreInitialized = 0;

    OptCatalog::PlayTriggerInactiveWarning();
    OptCatalog::PlayWeaponInactiveWarning();
    OptCatalog::PlayNoAmmoWarning();

    const bool samplesKept = g_OptCatalogSndTriggerInactive == &trigger &&
                             g_OptCatalogSndWeaponInactive == &weapon &&
                             g_OptCatalogSndNoAmmoWarning == &noAmmo;

    g_OptCatalogSndTriggerInactive = nullptr;
    g_OptCatalogSndWeaponInactive = nullptr;
    g_OptCatalogSndNoAmmoWarning = nullptr;

    return samplesKept ? 0 : 1;
}

extern "C" int zweapon_optcatalog_activate_trail_runtime_state_smoke(void) {
    const float oldNextSpawnScale = g_OptCatalogNextSpawnScale;
    void *const oldPendingSpawnTargetCountPtr = g_OptCatalogPendingSpawnTargetCountPtr;
    void *const oldPendingSpawnTargetListPtr = g_OptCatalogPendingSpawnTargetListPtr;

    zSndSample trailStopSample = {};
    zSndSample trailLoopSample = {};
    OptCatalogEntryDef owner = {};
    OptCatalogTrailRuntimeState previous = {};
    OptCatalogTrailRuntimeState runtime = {};
    zVec3 spawnPos = {1.0f, 2.0f, 3.0f};
    zVec3 spawnDir = {0.0f, 0.0f, 1.0f};
    int pendingTargetCount = 2;
    PlayerProgressTargetSlotRuntime pendingTargets[2] = {};

    owner.trailStopSample = &trailStopSample;
    owner.trailLoopSample = &trailLoopSample;
    owner.flags = 1u << 14;
    owner.trailEffectAnim = reinterpret_cast<zEffectAnimEntry *>(&owner);
    owner.activeTrailRuntime = &previous;
    runtime.ownerEntry = &owner;
    runtime.spawnPos = &spawnPos;
    runtime.spawnDir = &spawnDir;
    runtime.trailDistance = 9.0f;
    runtime.volumeFadeTimer = 8.0f;
    runtime.alphaPulsePhase = 7.0f;
    g_OptCatalogNextSpawnScale = 2.25f;
    g_OptCatalogPendingSpawnTargetCountPtr = &pendingTargetCount;
    g_OptCatalogPendingSpawnTargetListPtr = pendingTargets;

    OptCatalog::ActivateTrailRuntimeState(&runtime, 3);

    const bool runtimeOk = runtime.trailDistance == 0.0f && runtime.volumeFadeTimer == 0.0f &&
                           runtime.alphaPulsePhase == 0.0f && runtime.spawnScale == 2.25f &&
                           g_OptCatalogNextSpawnScale == 1.0f &&
                           runtime.pendingSpawnTargetCountPtr == &pendingTargetCount &&
                           runtime.pendingSpawnTargetListPtr == pendingTargets &&
                           g_OptCatalogPendingSpawnTargetCountPtr == nullptr &&
                           owner.trailEffectAnim == nullptr && runtime.prev == nullptr &&
                           runtime.next == &previous && previous.prev == &runtime &&
                           owner.activeTrailRuntime == &runtime;

    g_OptCatalogNextSpawnScale = oldNextSpawnScale;
    g_OptCatalogPendingSpawnTargetCountPtr = oldPendingSpawnTargetCountPtr;
    g_OptCatalogPendingSpawnTargetListPtr = oldPendingSpawnTargetListPtr;
    return runtimeOk ? 0 : 1;
}

extern "C" int zweapon_optcatalog_deactivate_trail_runtime_state_smoke(void) {
    OptCatalogEntryDef owner = {};
    OptCatalogTrailRuntimeState previous = {};
    OptCatalogTrailRuntimeState runtime = {};
    OptCatalogTrailRuntimeState next = {};
    zClass_NodePartial node0 = {};
    zClass_NodePartial node2 = {};

    owner.activeTrailRuntime = &runtime;
    runtime.ownerEntry = &owner;
    runtime.prev = &previous;
    runtime.next = &next;
    previous.next = &runtime;
    next.prev = &runtime;
    runtime.activeNodeSlotCount = 3;
    runtime.activeNodeSlotCursor = 3;
    runtime.activeNodeSlots[0].node = &node0;
    runtime.activeNodeSlots[2].node = &node2;

    node0.classId = 5;
    node0.flags = 0x04;
    node2.classId = 5;
    node2.flags = 0x04;

    const int result = OptCatalog::DeactivateTrailRuntimeState(&runtime);

    const bool listOk = result == 0 && owner.activeTrailRuntime == &next &&
                        previous.next == &next && next.prev == &previous &&
                        runtime.next == nullptr && runtime.prev == nullptr;
    const bool slotsOk = runtime.activeNodeSlotCursor == 0 &&
                         (node0.flags & 0x04) == 0 && (node2.flags & 0x04) == 0;
    return listOk && slotsOk ? 0 : 1;
}
