#include "GameZRecoil/zDEClient/zdec.h"

#include "Battlesport/GameNet.h"
#include "Battlesport/player.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zUtil/zZbd.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "zDi.h"

#include <windows.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <new>

namespace {
zDEClient_MapTreeNode *AllocTreeNode() {
    auto *node =
        static_cast<zDEClient_MapTreeNode *>(::operator new(sizeof(zDEClient_MapTreeNode)));
    *node = {};
    return node;
}

void ReleaseTreeSentinels() {
    ::operator delete(g_zDEClient_FeatureMapTree.header);
    ::operator delete(g_zDEClient_FeatureMapTreeNil);
    g_zDEClient_FeatureMapTree = {};
    g_zDEClient_FeatureMapTreeNil = nullptr;
}

void SetupSingleNodeFeatureTree() {
    zDEClient_MapTreeNode *const nil = AllocTreeNode();
    nil->left = nil;
    nil->parent = nil;
    nil->right = nil;
    nil->colorOrNil = 1;

    zDEClient_MapTreeNode *const header = AllocTreeNode();
    zDEClient_MapTreeNode *const root = AllocTreeNode();

    root->left = nil;
    root->parent = header;
    root->right = nil;
    root->colorOrNil = 1;

    header->left = root;
    header->parent = root;
    header->right = root;
    header->colorOrNil = 1;

    g_zDEClient_FeatureMapTreeNil = nil;
    g_zDEClient_FeatureMapTree.header = header;
    g_zDEClient_FeatureMapTree.nodeCount = 1;
}

void ResetZClassTypeLists() {
    zClass_TypeList::FreeAll();
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;
}

bool NearlyEqual(float lhs, float rhs) {
    return std::fabs(lhs - rhs) < 0.001f;
}

int g_dispatchCraterCallCount = 0;
int g_dispatchQSandCallCount = 0;
float g_dispatchCraterRadius[2] = {};
float g_dispatchQSandRadius = 0.0f;

int RECOIL_FASTCALL DispatchCraterCallback(zDEClient_CraterEventTemplate *eventTemplate) {
    if (g_dispatchCraterCallCount < 2) {
        g_dispatchCraterRadius[g_dispatchCraterCallCount] = eventTemplate->radius;
    }
    ++g_dispatchCraterCallCount;
    eventTemplate->radius = -100.0f;
    return 123;
}

int RECOIL_FASTCALL DispatchQSandCallback(zDEClient_QSandEventTemplate *eventTemplate) {
    g_dispatchQSandRadius = eventTemplate->radius;
    ++g_dispatchQSandCallCount;
    eventTemplate->radius = -200.0f;
    return 456;
}

void WriteZdeclientTestU32(HANDLE file, std::uint32_t value) {
    DWORD written = 0;
    WriteFile(file, &value, sizeof(value), &written, nullptr);
}

void WriteZdeclientTestBytes(HANDLE file, const void *data, std::uint32_t size) {
    DWORD written = 0;
    WriteFile(file, data, size, &written, nullptr);
}

void WriteZdeclientTestStringNode(HANDLE file, const char *text) {
    const std::uint32_t length = static_cast<std::uint32_t>(std::strlen(text));
    WriteZdeclientTestU32(file, zReader::ZRDR_NODE_STRING);
    WriteZdeclientTestU32(file, length);
    WriteZdeclientTestBytes(file, text, length);
}

void WriteZdeclientTestIntNode(HANDLE file, int value) {
    WriteZdeclientTestU32(file, zReader::ZRDR_NODE_INT);
    WriteZdeclientTestU32(file, static_cast<std::uint32_t>(value));
}

void WriteZdeclientTestFloatNode(HANDLE file, float value) {
    WriteZdeclientTestU32(file, zReader::ZRDR_NODE_FLOAT);
    WriteZdeclientTestBytes(file, &value, sizeof(value));
}

void WriteZdeclientTestArrayHeader(HANDLE file, int nodeCount) {
    WriteZdeclientTestU32(file, zReader::ZRDR_NODE_ARRAY);
    WriteZdeclientTestU32(file, static_cast<std::uint32_t>(nodeCount));
}

void WriteZdeclientTestNamedInt(HANDLE file, const char *name, int value) {
    WriteZdeclientTestStringNode(file, name);
    WriteZdeclientTestIntNode(file, value);
}

void WriteZdeclientTestNamedFloat(HANDLE file, const char *name, float value) {
    WriteZdeclientTestStringNode(file, name);
    WriteZdeclientTestFloatNode(file, value);
}

void WriteZdeclientTestNamedString(HANDLE file, const char *name, const char *value) {
    WriteZdeclientTestStringNode(file, name);
    WriteZdeclientTestStringNode(file, value);
}

zZbdManager MakeZdeclientTestManager(zZbdSectionHandlerNode &sentinel) {
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    sentinel.sectionHandler = {};

    zZbdManager manager = {};
    manager.sectionHandlerListSentinel = &sentinel;
    return manager;
}

void ClearZdeclientTestHandlers(zZbdSectionHandlerNode &sentinel) {
    zZbdSectionHandlerNode *node = sentinel.next;
    while (node != &sentinel) {
        zZbdSectionHandlerNode *next = node->next;
        delete node;
        node = next;
    }
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
}

void SetupSingleFeatureGridCell(zClass_NodePartial *world, zClass_WorldDataPartial *worldData,
                                zDEClient_CameraNodeClassDataPartial *cameraData,
                                zDEClient_FeatureGridCell *cell,
                                zDEClient_FeatureGridCell **rows) {
    *world = {};
    *worldData = {};
    *cameraData = {};
    *cell = {};

    world->classData = worldData;
    worldData->partitionMaxDecFeatureCount = 2;
    worldData->originX = 0.0f;
    worldData->originZ = 0.0f;
    worldData->worldMaxX = 10.0f;
    worldData->worldMaxZ = -10.0f;
    worldData->areaCellSizeX = 10.0f;
    worldData->areaCellSizeZ = -10.0f;
    worldData->areaInvSizeX = 0.0f;
    worldData->areaInvSizeZ = 0.0f;

    cell->originX = 0.0f;
    cell->originZ = 0.0f;
    rows[0] = cell;
    cameraData->featureGridRows = rows;

    g_zDEClient_CameraNode = world;
    g_zDEClient_CameraNodeClassData = reinterpret_cast<zClass_CameraDataPartial *>(cameraData);
}

bool ValidateCircleFeaturePoints(const zVec3 *points, float centerX, float centerY, float centerZ,
                                 float radius) {
    return NearlyEqual(points[0].x, centerX) && NearlyEqual(points[0].y, centerY) &&
           NearlyEqual(points[0].z, centerZ + radius) &&
           NearlyEqual(points[1].x, centerX + radius) && NearlyEqual(points[1].y, centerY) &&
           NearlyEqual(points[1].z, centerZ) && NearlyEqual(points[2].x, centerX) &&
           NearlyEqual(points[2].y, centerY) && NearlyEqual(points[2].z, centerZ - radius) &&
           NearlyEqual(points[3].x, centerX - radius) && NearlyEqual(points[3].y, centerY) &&
           NearlyEqual(points[3].z, centerZ);
}

int g_qsandRelayCallCount = 0;
int g_qsandRelayResult = 0;
int g_craterRelayCallCount = 0;
int g_craterRelayResult = 0;
int g_craterNetRelaySendCalls = 0;
std::uint32_t g_craterNetRelaySendFlags = 0;
void *g_craterNetRelaySendPacket = nullptr;
std::uint32_t g_craterNetRelaySendPacketSize = 0;

int RECOIL_FASTCALL TestQSandRelayCallback(void *) {
    ++g_qsandRelayCallCount;
    return g_qsandRelayResult;
}

int RECOIL_FASTCALL TestCraterRelayCallback(void *) {
    ++g_craterRelayCallCount;
    return g_craterRelayResult;
}

std::int32_t RECOIL_STDCALL CraterNetRelaySendFake(zNetwork_DPlay4 *, std::uint32_t,
                                                   std::uint32_t, std::uint32_t flags,
                                                   void *packet,
                                                   std::uint32_t packetSizeBytes) {
    ++g_craterNetRelaySendCalls;
    g_craterNetRelaySendFlags = flags;
    g_craterNetRelaySendPacket = packet;
    g_craterNetRelaySendPacketSize = packetSizeBytes;
    return 0;
}

const zNetwork_DPlay4Vtable kCraterNetRelayDPlayVtable = {
    {}, nullptr, {}, nullptr, {}, nullptr, CraterNetRelaySendFake, {}, nullptr, {}, nullptr, {}, nullptr,
};
} // namespace

extern "C" int zdeclient_set_camera_node_smoke(void) {
    zClass_CameraDataPartial cameraData = {};
    zClass_NodePartial cameraNode = {};
    cameraNode.classId = 3;
    cameraNode.classData = &cameraData;

    g_zDEClient_CameraNode = nullptr;
    g_zDEClient_CameraNodeClassData = nullptr;
    zDEClient::SetCameraNode(nullptr);
    if (g_zDEClient_CameraNode != nullptr || g_zDEClient_CameraNodeClassData != nullptr) {
        return 1;
    }

    zDEClient::SetCameraNode(&cameraNode);
    const bool ok =
        g_zDEClient_CameraNode == &cameraNode && g_zDEClient_CameraNodeClassData == &cameraData;

    g_zDEClient_CameraNode = nullptr;
    g_zDEClient_CameraNodeClassData = nullptr;
    return ok ? 0 : 2;
}

extern "C" int zdeclient_load_material_from_texture_path_smoke(void) {
    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    g_zImage_TexDirEntryCount = 1;
    std::strcpy(g_zImage_TexDirEntries[0].baseName, "stone");
    g_zImage_TexDirEntries[0].loadState = 2;

    zModel_MaterialSlot reuseSlots[1] = {};
    reuseSlots[0].material.currentTextureDirectoryEntry = &g_zImage_TexDirEntries[0];
    reuseSlots[0].nextPoolIndex = -1;
    g_zModel_MatlPool = reuseSlots;
    g_zModel_MatlActiveHeadIndex = 0;
    g_zModel_MatlFreeHeadIndex = -1;
    g_zModel_MatlReuseCache = nullptr;

    zModel_MaterialPartial *material = nullptr;
    char existingPath[] = "stone";
    const bool reuseOk =
        zDEClient::LoadMaterialFromTexturePath_Local(&material, existingPath) == 0 &&
        material == &reuseSlots[0].material && g_zImage_TexDirEntryCount == 1;

    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    g_zImage_TexDirEntryCount = 0;
    zModel_MaterialSlot cloneSlots[1] = {};
    cloneSlots[0].prevPoolIndex = -1;
    cloneSlots[0].nextPoolIndex = -1;
    g_zModel_MatlPool = cloneSlots;
    g_zModel_MatlActiveHeadIndex = -1;
    g_zModel_MatlFreeHeadIndex = 0;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlReuseCache = nullptr;

    material = nullptr;
    char newPath[] = "newtex.tga";
    const bool appendOk = zDEClient::LoadMaterialFromTexturePath_Local(&material, newPath) == 1 &&
                          material == &cloneSlots[0].material && (material->flags & 0x0100) != 0 &&
                          material->currentTextureDirectoryEntry == &g_zImage_TexDirEntries[0] &&
                          g_zImage_TexDirEntries[0].loadState == 2 &&
                          g_zImage_TexDirEntryCount == 1 && g_zModel_MatlActiveHeadIndex == 0 &&
                          g_zModel_MatlFreeHeadIndex == -1;

    g_zImage_TexDirEntryCount = 0;
    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    g_zModel_MatlPool = nullptr;
    g_zModel_MatlActiveHeadIndex = -1;
    g_zModel_MatlFreeHeadIndex = -1;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlReuseCache = nullptr;

    return reuseOk && appendOk ? 0 : 1;
}

extern "C" int zdeclient_load_config_resources_smoke(void) {
    zArchiveList *const oldMountedList = g_zArchive_MountedList;
    zZbdManager *const oldZbdManager = g_zUtil_ZbdManager;
    zModel_MaterialSlot *const oldMatlPool = g_zModel_MatlPool;
    const int oldMatlPoolCapacity = g_zModel_MatlPoolCapacity;
    const int oldMatlPoolInUseCount = g_zModel_MatlPoolInUseCount;
    const int oldMatlFreeHeadIndex = g_zModel_MatlFreeHeadIndex;
    const int oldMatlActiveHeadIndex = g_zModel_MatlActiveHeadIndex;
    zModel_MaterialPartial *const oldMatlReuseCache = g_zModel_MatlReuseCache;
    const int oldTexDirEntryCount = g_zImage_TexDirEntryCount;
    zImage_TexDirEntryPartial oldTexDirEntries[0x1000] = {};
    std::memcpy(oldTexDirEntries, g_zImage_TexDirEntries, sizeof(oldTexDirEntries));
    zEffectAnimEntry *const oldEffectEntries = g_zEffectAnim_EntryList;
    const short oldEffectEntryCount = g_zEffectAnim_EntryCount;
    const int oldRebuildBltRect = g_zDEClient_RebuildBltRectOnReload;
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const int oldBuiltinTexturePackCount = g_zVid_BuiltinTexturePackCount;
    zVidTexturePackEntry *const oldBuiltinTexturePacks = g_zVid_BuiltinTexturePacks;
    const int oldTexturePackCount = g_zVid_TexturePackCount;
    zVidTexturePackEntry *const oldTexturePacks = g_zVid_TexturePacks;

    char tempDir[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zdc", 0, tempFile) == 0) {
        return 1;
    }

    HANDLE file = CreateFileA(tempFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        DeleteFileA(tempFile);
        return 2;
    }

    WriteZdeclientTestArrayHeader(file, 5);
    WriteZdeclientTestStringNode(file, "CRATER");
    WriteZdeclientTestArrayHeader(file, 13);
    WriteZdeclientTestNamedInt(file, "POINTS", 9);
    WriteZdeclientTestNamedFloat(file, "SLOPE", 1.5f);
    WriteZdeclientTestNamedFloat(file, "DEPTH", 2.5f);
    WriteZdeclientTestNamedFloat(file, "RADIUS", 3.5f);
    WriteZdeclientTestNamedString(file, "DEFAULT_TEXTURE", "crater_base.tga");
    WriteZdeclientTestNamedString(file, "DEFAULT_ANIM", "crater_fx");

    WriteZdeclientTestStringNode(file, "QUICK_SAND");
    WriteZdeclientTestArrayHeader(file, 11);
    WriteZdeclientTestStringNode(file, "DEFAULT_TEXTURE");
    WriteZdeclientTestArrayHeader(file, 4);
    WriteZdeclientTestFloatNode(file, 6.0f);
    WriteZdeclientTestStringNode(file, "sand_a.tga");
    WriteZdeclientTestStringNode(file, "sand_b.tga");
    WriteZdeclientTestNamedInt(file, "POINTS", 8);
    WriteZdeclientTestNamedFloat(file, "SLOPE", 11.0f);
    WriteZdeclientTestNamedFloat(file, "DEPTH", 12.0f);
    WriteZdeclientTestNamedFloat(file, "RADIUS", 13.0f);
    FlushFileBuffers(file);

    zZarFileRecord record = {};
    record.fileOffset = 0;
    record.fileSize = SetFilePointer(file, 0, nullptr, FILE_CURRENT);
    std::strcpy(record.name, "declient.zrd");

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

    zEffectAnimEntry effects[1] = {};
    std::strcpy(effects[0].name, "crater_fx");
    g_zEffectAnim_EntryList = effects;
    g_zEffectAnim_EntryCount = 1;

    zVidTexturePackEntry texturePacks[2] = {};
    texturePacks[0].fileHandle = stdout;
    texturePacks[1].fileHandle = stdout;
    g_zVideo_ActiveRendererPath = 0;
    g_zVid_BuiltinTexturePackCount = 1;
    g_zVid_BuiltinTexturePacks = &texturePacks[0];
    g_zVid_TexturePackCount = 1;
    g_zVid_TexturePacks = &texturePacks[1];

    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager = MakeZdeclientTestManager(sentinel);
    g_zUtil_ZbdManager = &manager;
    g_zDEClient_RebuildBltRectOnReload = 1;

    zClass_CameraDataPartial cameraData = {};
    zClass_NodePartial worldNode = {};
    worldNode.classData = &cameraData;

    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    g_zImage_TexDirEntryCount = 0;
    g_zDEClient_CraterDisplaySourceList = nullptr;
    g_zDEClient_CraterDisplaySourceCount = 0;
    g_zDEClient_QuickSandTexturePaths = nullptr;
    g_zDEClient_QuickSandTextureCount = 0;
    g_zDEClient_QuickSandEnabled = 0;
    g_zDEClient_QuickSandMaterial = nullptr;
    g_zDEClient_QuickSandMaterialCycle = nullptr;
    g_zDEClient_CameraNode = nullptr;
    g_zDEClient_CameraNodeClassData = nullptr;

    const int result = zDEClient::LoadConfigResources(&worldNode);
    const bool ok =
        result == 0 && g_zDEClient_CameraNode == &worldNode &&
        g_zDEClient_CameraNodeClassData == &cameraData && g_zDEClient_ConfigReaderRoot == nullptr &&
        g_zDEClient_CraterEventTemplateDefaults.featureFlags == 0x100c &&
        g_zDEClient_CraterEventTemplateDefaults.pointCount == 9 &&
        NearlyEqual(g_zDEClient_CraterEventTemplateDefaults.slope, 1.5f) &&
        NearlyEqual(g_zDEClient_CraterEventTemplateDefaults.depth, 2.5f) &&
        NearlyEqual(g_zDEClient_CraterEventTemplateDefaults.radius, 3.5f) &&
        g_zDEClient_CraterDisplaySourceCount == 1 &&
        g_zDEClient_CraterDisplaySourceList != nullptr &&
        g_zDEClient_CraterDisplaySourceList[0].craterMaterial != nullptr &&
        g_zDEClient_CraterDisplaySourceList[0].effectAnimEntry == &effects[0] &&
        g_zDEClient_QuickSandEnabled == 1 && g_zDEClient_QuickSandTextureCount == 2 &&
        NearlyEqual(g_zDEClient_QuickSandAnimSpeed, 6.0f) &&
        g_zDEClient_QuickSandTexturePaths != nullptr &&
        g_zDEClient_QuickSandEventTemplateDefaults.featureFlags == 0x1008 &&
        g_zDEClient_QuickSandEventTemplateDefaults.pointCount == 8 &&
        NearlyEqual(g_zDEClient_QuickSandEventTemplateDefaults.slope, 11.0f) &&
        NearlyEqual(g_zDEClient_QuickSandEventTemplateDefaults.depth, 12.0f) &&
        NearlyEqual(g_zDEClient_QuickSandEventTemplateDefaults.radius, 13.0f) &&
        g_zDEClient_QuickSandMaterial != nullptr &&
        g_zDEClient_QuickSandMaterial->userTag == 3 &&
        g_zDEClient_QuickSandMaterial->cycle != nullptr &&
        g_zDEClient_QuickSandMaterial->cycle->frameCount == 2 &&
        std::strcmp(g_zDEClient_QuickSandMaterial->cycle->frameTable[0]->baseName, "sand_a") ==
            0 &&
        std::strcmp(g_zDEClient_QuickSandMaterial->cycle->frameTable[1]->baseName, "sand_b") ==
            0 &&
        g_zDEClient_QuickSandMaterialCycle != nullptr &&
        g_zDEClient_QuickSandMaterialCycle->userTag == 0 &&
        manager.sectionHandlerCount == 1 && sentinel.next != &sentinel &&
        std::strcmp(sentinel.next->sectionHandler.sectionName, "zDEClient") == 0;

    for (int i = 0; i < 8; ++i) {
        if (materialSlots[i].material.cycle != nullptr) {
            std::free(materialSlots[i].material.cycle->frameTable);
            std::free(materialSlots[i].material.cycle);
            materialSlots[i].material.cycle = nullptr;
        }
    }
    ClearZdeclientTestHandlers(sentinel);
    std::free(g_zDEClient_CraterDisplaySourceList);
    std::free(g_zDEClient_QuickSandTexturePaths);
    g_zDEClient_CraterDisplaySourceList = nullptr;
    g_zDEClient_CraterDisplaySourceCount = 0;
    g_zDEClient_QuickSandTexturePaths = nullptr;
    g_zDEClient_QuickSandTextureCount = 0;
    g_zDEClient_QuickSandEnabled = 0;
    g_zDEClient_QuickSandMaterial = nullptr;
    g_zDEClient_QuickSandMaterialCycle = nullptr;
    g_zDEClient_CameraNode = nullptr;
    g_zDEClient_CameraNodeClassData = nullptr;
    g_zDEClient_RebuildBltRectOnReload = oldRebuildBltRect;
    g_zUtil_ZbdManager = oldZbdManager;
    g_zEffectAnim_EntryList = oldEffectEntries;
    g_zEffectAnim_EntryCount = oldEffectEntryCount;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVid_BuiltinTexturePackCount = oldBuiltinTexturePackCount;
    g_zVid_BuiltinTexturePacks = oldBuiltinTexturePacks;
    g_zVid_TexturePackCount = oldTexturePackCount;
    g_zVid_TexturePacks = oldTexturePacks;
    std::memcpy(g_zImage_TexDirEntries, oldTexDirEntries, sizeof(g_zImage_TexDirEntries));
    g_zImage_TexDirEntryCount = oldTexDirEntryCount;
    g_zModel_MatlPool = oldMatlPool;
    g_zModel_MatlPoolCapacity = oldMatlPoolCapacity;
    g_zModel_MatlPoolInUseCount = oldMatlPoolInUseCount;
    g_zModel_MatlFreeHeadIndex = oldMatlFreeHeadIndex;
    g_zModel_MatlActiveHeadIndex = oldMatlActiveHeadIndex;
    g_zModel_MatlReuseCache = oldMatlReuseCache;
    g_zArchive_MountedList = oldMountedList;
    CloseHandle(file);
    DeleteFileA(tempFile);

    return ok ? 0 : 3;
}

extern "C" int zdeclient_crater_init_event_template_defaults_smoke(void) {
    const zDEClient_CraterEventTemplate oldDefaults = g_zDEClient_CraterEventTemplateDefaults;

    zModel_MaterialSlot materialSlot = {};
    zClass_NodePartial ownerNode = {};
    g_zDEClient_CraterEventTemplateDefaults.featureFlags = 0x100c;
    g_zDEClient_CraterEventTemplateDefaults.pointCount = 7;
    g_zDEClient_CraterEventTemplateDefaults.craterMaterialSlot = &materialSlot;
    g_zDEClient_CraterEventTemplateDefaults.slope = 1.25f;
    g_zDEClient_CraterEventTemplateDefaults.depth = 4.5f;
    g_zDEClient_CraterEventTemplateDefaults.radius = 9.75f;
    g_zDEClient_CraterEventTemplateDefaults.center = {2.0f, 3.0f, 4.0f};
    g_zDEClient_CraterEventTemplateDefaults.damageOwnerNode = &ownerNode;

    zDEClient_CraterEventTemplate eventTemplate = {};
    std::memset(&eventTemplate, 0xcc, sizeof(eventTemplate));
    zDEClient_Crater::InitEventTemplateDefaults(&eventTemplate);

    const bool ok =
        std::memcmp(&eventTemplate, &g_zDEClient_CraterEventTemplateDefaults,
                    sizeof(eventTemplate)) == 0 &&
        eventTemplate.featureFlags == 0x100c && eventTemplate.pointCount == 7 &&
        eventTemplate.craterMaterialSlot == &materialSlot && eventTemplate.slope == 1.25f &&
        eventTemplate.depth == 4.5f && eventTemplate.radius == 9.75f &&
        eventTemplate.center.x == 2.0f && eventTemplate.center.y == 3.0f &&
        eventTemplate.center.z == 4.0f && eventTemplate.damageOwnerNode == &ownerNode;

    g_zDEClient_CraterEventTemplateDefaults = oldDefaults;
    return ok ? 0 : 1;
}

extern "C" int zdeclient_qsand_copy_event_template_defaults_smoke(void) {
    const zDEClient_QSandEventTemplate oldDefaults = g_zDEClient_QuickSandEventTemplateDefaults;

    zModel_MaterialPartial material = {};
    zModel_MaterialPartial materialCycle = {};
    zClass_NodePartial ownerNode = {};
    g_zDEClient_QuickSandEventTemplateDefaults.featureFlags = 0x1008;
    g_zDEClient_QuickSandEventTemplateDefaults.pointCount = 9;
    g_zDEClient_QuickSandEventTemplateDefaults.material = &material;
    g_zDEClient_QuickSandEventTemplateDefaults.materialCycle = &materialCycle;
    g_zDEClient_QuickSandEventTemplateDefaults.slope = 2.25f;
    g_zDEClient_QuickSandEventTemplateDefaults.depth = 5.5f;
    g_zDEClient_QuickSandEventTemplateDefaults.radius = 12.75f;
    g_zDEClient_QuickSandEventTemplateDefaults.center = {6.0f, 7.0f, 8.0f};
    g_zDEClient_QuickSandEventTemplateDefaults.damageOwnerNode = &ownerNode;

    zDEClient_QSandEventTemplate eventTemplate = {};
    std::memset(&eventTemplate, 0xcc, sizeof(eventTemplate));
    zDEClient::CopyQSandEventTemplateDefaults(&eventTemplate);

    const bool ok =
        std::memcmp(&eventTemplate, &g_zDEClient_QuickSandEventTemplateDefaults,
                    sizeof(eventTemplate)) == 0 &&
        eventTemplate.featureFlags == 0x1008 && eventTemplate.pointCount == 9 &&
        eventTemplate.material == &material && eventTemplate.materialCycle == &materialCycle &&
        eventTemplate.slope == 2.25f && eventTemplate.depth == 5.5f &&
        eventTemplate.radius == 12.75f && eventTemplate.center.x == 6.0f &&
        eventTemplate.center.y == 7.0f && eventTemplate.center.z == 8.0f &&
        eventTemplate.damageOwnerNode == &ownerNode;

    g_zDEClient_QuickSandEventTemplateDefaults = oldDefaults;
    return ok ? 0 : 1;
}

extern "C" int zdeclient_write_feature_sections_to_zar_smoke(void) {
    zDEClient_FeatureEntry *const oldBegin = g_zDEClient_FeatureListBegin;
    zDEClient_FeatureEntry *const oldEnd = g_zDEClient_FeatureListEnd;
    zDEClient_FeatureEntry *const oldCapacityEnd = g_zDEClient_FeatureListCapacityEnd;

    char tempPath[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempPath), tempPath) == 0 ||
        GetTempFileNameA(tempPath, "zdc", 0, tempFile) == 0) {
        return 1;
    }

    HANDLE const file =
        CreateFileA(tempFile, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        DeleteFileA(tempFile);
        return 2;
    }

    zDEClient_FeatureEntry entries[3] = {};
    entries[0].featureType = 1;
    entries[0].eventData.crater.radius = 3.0f;
    entries[0].reloadFlag = 77;
    entries[1].featureType = 3;
    entries[1].eventData.quickSand.radius = 4.0f;
    entries[1].reloadFlag = 88;
    entries[2].featureType = 2;
    entries[2].reloadFlag = 99;
    g_zDEClient_FeatureListBegin = entries;
    g_zDEClient_FeatureListEnd = entries + 3;
    g_zDEClient_FeatureListCapacityEnd = entries + 3;

    zZbdManager manager = {};
    manager.indexArchive.hFile = file;
    zZbdSectionHandler handler = {};
    handler.sectionName = "zDEClient";
    zZbdSectionCallbackCtx callbackCtx = {};
    callbackCtx.manager = &manager;
    callbackCtx.sectionHandler = &handler;

    const int result = zDEClient::WriteFeatureSectionsToZAR(&callbackCtx);
    const bool namesOk =
        result == 1 && manager.indexArchive.recordCount == 3 &&
        std::strcmp(manager.indexArchive.records[0].name, "zDEClient/Dummy") == 0 &&
        std::strcmp(manager.indexArchive.records[1].name, "zDEClient/Crater0") == 0 &&
        std::strcmp(manager.indexArchive.records[2].name, "zDEClient/QSand0") == 0;

    zDEClient_FeatureEntry craterPayload = {};
    zDEClient_FeatureEntry qSandPayload = {};
    DWORD readCrater = 0;
    DWORD readQSand = 0;
    if (namesOk) {
        SetFilePointer(file, manager.indexArchive.records[1].fileOffset, nullptr, FILE_BEGIN);
        ReadFile(file, &craterPayload, sizeof(craterPayload), &readCrater, nullptr);
        SetFilePointer(file, manager.indexArchive.records[2].fileOffset, nullptr, FILE_BEGIN);
        ReadFile(file, &qSandPayload, sizeof(qSandPayload), &readQSand, nullptr);
    }

    const bool payloadsOk =
        readCrater == sizeof(craterPayload) && readQSand == sizeof(qSandPayload) &&
        craterPayload.featureType == 1 && craterPayload.reloadFlag == 0 &&
        craterPayload.eventData.crater.radius == 3.0f && qSandPayload.featureType == 3 &&
        qSandPayload.reloadFlag == 0 && qSandPayload.eventData.quickSand.radius == 4.0f;

    std::free(manager.indexArchive.records);
    CloseHandle(file);
    g_zDEClient_FeatureListBegin = oldBegin;
    g_zDEClient_FeatureListEnd = oldEnd;
    g_zDEClient_FeatureListCapacityEnd = oldCapacityEnd;
    DeleteFileA(tempFile);

    return namesOk && payloadsOk ? 0 : 3;
}

extern "C" int zdeclient_shutdown_globals_smoke(void) {
    g_zDEClient_QuickSandEnabled = 0;
    if (zDEClient::ShutdownGlobals() != 0) {
        return 1;
    }

    SetupSingleNodeFeatureTree();
    auto *entries =
        static_cast<zDEClient_FeatureEntry *>(std::malloc(sizeof(zDEClient_FeatureEntry) * 2));
    g_zDEClient_FeatureListBegin = entries;
    g_zDEClient_FeatureListEnd = entries + 1;
    g_zDEClient_FeatureListCapacityEnd = entries + 2;

    if (zDEClient::ClearFeatureEntriesAndMapTree() != 0 ||
        g_zDEClient_FeatureListEnd != g_zDEClient_FeatureListBegin ||
        g_zDEClient_FeatureMapTree.nodeCount != 0 ||
        g_zDEClient_FeatureMapTree.header->parent != g_zDEClient_FeatureMapTreeNil ||
        g_zDEClient_FeatureMapTree.header->left != g_zDEClient_FeatureMapTree.header ||
        g_zDEClient_FeatureMapTree.header->right != g_zDEClient_FeatureMapTree.header) {
        std::free(entries);
        ReleaseTreeSentinels();
        return 2;
    }

    std::free(entries);
    ReleaseTreeSentinels();

    SetupSingleNodeFeatureTree();
    entries = static_cast<zDEClient_FeatureEntry *>(std::malloc(sizeof(zDEClient_FeatureEntry)));
    g_zDEClient_FeatureListBegin = entries;
    g_zDEClient_FeatureListEnd = entries + 1;
    g_zDEClient_FeatureListCapacityEnd = entries + 1;
    g_zDEClient_QuickSandEnabled = 1;
    g_zDEClient_QuickSandTexturePaths = static_cast<char **>(std::malloc(sizeof(char *)));
    g_zDEClient_CraterDisplaySourceList = static_cast<zDEClient_CraterDisplaySourceEntry *>(
        std::malloc(sizeof(zDEClient_CraterDisplaySourceEntry)));

    const int result = zDEClient::ShutdownGlobals();
    const bool ok = result == 0 && g_zDEClient_QuickSandEnabled == 0 &&
                    g_zDEClient_QuickSandTexturePaths == nullptr &&
                    g_zDEClient_CraterDisplaySourceList == nullptr &&
                    g_zDEClient_FeatureMapTree.nodeCount == 0;

    std::free(entries);
    ReleaseTreeSentinels();
    g_zDEClient_FeatureListBegin = nullptr;
    g_zDEClient_FeatureListEnd = nullptr;
    g_zDEClient_FeatureListCapacityEnd = nullptr;

    return ok ? 0 : 3;
}

extern "C" int zdeclient_map_tree_iter_next_node_ref_smoke(void) {
    zDEClient_MapTreeNode nil = {};
    zDEClient_MapTreeNode header = {};
    zDEClient_MapTreeNode left = {};
    zDEClient_MapTreeNode root = {};
    zDEClient_MapTreeNode right = {};
    zDEClient_MapTreeNode rightLeft = {};

    nil.left = &nil;
    nil.parent = &nil;
    nil.right = &nil;
    nil.colorOrNil = 1;
    g_zDEClient_FeatureMapTreeNil = &nil;
    g_zDEClient_FeatureMapTree.header = &header;

    header.left = &left;
    header.parent = &root;
    header.right = &right;
    header.colorOrNil = 1;

    left.left = &nil;
    left.parent = &root;
    left.right = &nil;
    left.colorOrNil = 1;

    root.left = &left;
    root.parent = &header;
    root.right = &right;
    root.colorOrNil = 1;

    right.left = &rightLeft;
    right.parent = &root;
    right.right = &nil;
    right.colorOrNil = 1;

    rightLeft.left = &nil;
    rightLeft.parent = &right;
    rightLeft.right = &nil;
    rightLeft.colorOrNil = 1;

    zDEClient_MapTreeNode *cursor = &left;
    if (g_zDEClient_FeatureMapTree.IterNextNodeRef(&cursor) != &cursor || cursor != &root) {
        return 1;
    }

    cursor = &root;
    if (g_zDEClient_FeatureMapTree.IterNextNodeRef(&cursor) != &cursor ||
        cursor != &rightLeft) {
        return 2;
    }

    cursor = &rightLeft;
    if (g_zDEClient_FeatureMapTree.IterNextNodeRef(&cursor) != &cursor || cursor != &right) {
        return 3;
    }

    cursor = &right;
    if (g_zDEClient_FeatureMapTree.IterNextNodeRef(&cursor) != &cursor || cursor != &header) {
        return 4;
    }

    g_zDEClient_FeatureMapTree = {};
    g_zDEClient_FeatureMapTreeNil = nullptr;
    return 0;
}

extern "C" int zdeclient_map_tree_iter_prev_node_ref_smoke(void) {
    zDEClient_MapTreeNode nil = {};
    zDEClient_MapTreeNode header = {};
    zDEClient_MapTreeNode left = {};
    zDEClient_MapTreeNode root = {};
    zDEClient_MapTreeNode right = {};
    zDEClient_MapTreeNode rightLeft = {};

    nil.left = &nil;
    nil.parent = &nil;
    nil.right = &nil;
    nil.colorOrNil = 1;
    g_zDEClient_FeatureMapTreeNil = &nil;
    g_zDEClient_FeatureMapTree.header = &header;

    header.left = &left;
    header.parent = &root;
    header.right = &right;
    header.colorOrNil = 0;

    left.left = &nil;
    left.parent = &root;
    left.right = &nil;
    left.colorOrNil = 1;

    root.left = &left;
    root.parent = &header;
    root.right = &right;
    root.colorOrNil = 1;

    right.left = &rightLeft;
    right.parent = &root;
    right.right = &nil;
    right.colorOrNil = 1;

    rightLeft.left = &nil;
    rightLeft.parent = &right;
    rightLeft.right = &nil;
    rightLeft.colorOrNil = 1;

    zDEClient_MapTreeNode *cursor = &header;
    g_zDEClient_FeatureMapTree.IterPrevNodeRef(&cursor);
    if (cursor != &right) {
        return 1;
    }

    cursor = &right;
    g_zDEClient_FeatureMapTree.IterPrevNodeRef(&cursor);
    if (cursor != &rightLeft) {
        return 2;
    }

    cursor = &rightLeft;
    g_zDEClient_FeatureMapTree.IterPrevNodeRef(&cursor);
    if (cursor != &root) {
        return 3;
    }

    cursor = &root;
    g_zDEClient_FeatureMapTree.IterPrevNodeRef(&cursor);
    if (cursor != &left) {
        return 4;
    }

    g_zDEClient_FeatureMapTree = {};
    g_zDEClient_FeatureMapTreeNil = nullptr;
    return 0;
}

extern "C" int zdeclient_map_tree_insert_at_smoke(void) {
    zDEClient_MapTreeNode nil = {};
    zDEClient_MapTreeNode header = {};
    zGeometry_ClipPatchNodeView keys[3] = {};
    zGeometry_ClipPatchNodeDiPair pairs[3] = {};
    pairs[0].node = &keys[0];
    pairs[1].node = &keys[1];
    pairs[2].node = &keys[2];

    nil.left = &nil;
    nil.parent = &nil;
    nil.right = &nil;
    nil.colorOrNil = 1;
    header.left = &header;
    header.parent = &nil;
    header.right = &header;
    header.colorOrNil = 0;
    g_zDEClient_FeatureMapTree = {};
    g_zDEClient_FeatureMapTree.header = &header;
    g_zDEClient_FeatureMapTreeNil = &nil;

    zDEClient_MapTreeNode *root = nullptr;
    if (g_zDEClient_FeatureMapTree.InsertAt(&root, &nil, &header, &pairs[1]) != &root ||
        root == nullptr || header.parent != root || header.left != root ||
        header.right != root || root->key != &keys[1] || root->colorOrNil != 1 ||
        g_zDEClient_FeatureMapTree.nodeCount != 1) {
        return 1;
    }

    zDEClient_MapTreeNode *left = nullptr;
    if (g_zDEClient_FeatureMapTree.InsertAt(&left, &nil, root, &pairs[0]) != &left ||
        left == nullptr || root->left != left || header.left != left ||
        left->parent != root || left->key != &keys[0] ||
        g_zDEClient_FeatureMapTree.nodeCount != 2) {
        ::operator delete(root);
        return 2;
    }

    zDEClient_MapTreeNode *right = nullptr;
    if (g_zDEClient_FeatureMapTree.InsertAt(&right, &nil, root, &pairs[2]) != &right ||
        right == nullptr || root->right != right || header.right != right ||
        right->parent != root || right->key != &keys[2] ||
        g_zDEClient_FeatureMapTree.nodeCount != 3) {
        ::operator delete(left);
        ::operator delete(root);
        return 3;
    }

    zDEClient_MapTreeNode *cursor = left;
    if (g_zDEClient_FeatureMapTree.IterNextNodeRef(&cursor) != &cursor || cursor != root) {
        ::operator delete(right);
        ::operator delete(left);
        ::operator delete(root);
        return 4;
    }

    ::operator delete(right);
    ::operator delete(left);
    ::operator delete(root);
    g_zDEClient_FeatureMapTree = {};
    g_zDEClient_FeatureMapTreeNil = nullptr;
    return 0;
}

extern "C" int zdeclient_map_tree_find_or_insert_key_smoke(void) {
    zGeometry_ClipPatchNodeView keys[3] = {};
    zGeometry_ClipPatchNodeDiPair pairs[3] = {};
    pairs[0].node = &keys[0];
    pairs[1].node = &keys[1];
    pairs[2].node = &keys[2];

    g_zDEClient_FeatureMapTree = {};
    g_zDEClient_FeatureMapTreeNil = nullptr;

    zDEClient_MapTreeLocateResult rootResult = {};
    if (g_zDEClient_FeatureMapTree.FindOrInsertKey(&rootResult, &pairs[1]) != &rootResult ||
        rootResult.inserted != 1 || rootResult.node == nullptr ||
        rootResult.node->key != &keys[1] || g_zDEClient_FeatureMapTree.nodeCount != 1) {
        return 1;
    }

    zDEClient_MapTreeLocateResult duplicateResult = {};
    if (g_zDEClient_FeatureMapTree.FindOrInsertKey(&duplicateResult, &pairs[1]) !=
            &duplicateResult ||
        duplicateResult.inserted != 0 || duplicateResult.node != rootResult.node ||
        g_zDEClient_FeatureMapTree.nodeCount != 1) {
        zDEClient_MapTreeNode *outNext = nullptr;
        g_zDEClient_FeatureMapTree.EraseRange(&outNext, g_zDEClient_FeatureMapTree.header->left,
                                              g_zDEClient_FeatureMapTree.header);
        ReleaseTreeSentinels();
        return 2;
    }

    zDEClient_MapTreeLocateResult leftResult = {};
    zDEClient_MapTreeLocateResult rightResult = {};
    if (g_zDEClient_FeatureMapTree.FindOrInsertKey(&leftResult, &pairs[0]) != &leftResult ||
        g_zDEClient_FeatureMapTree.FindOrInsertKey(&rightResult, &pairs[2]) != &rightResult ||
        leftResult.inserted != 1 || rightResult.inserted != 1 ||
        g_zDEClient_FeatureMapTree.header->left != leftResult.node ||
        g_zDEClient_FeatureMapTree.header->right != rightResult.node ||
        g_zDEClient_FeatureMapTree.nodeCount != 3) {
        zDEClient_MapTreeNode *outNext = nullptr;
        g_zDEClient_FeatureMapTree.EraseRange(&outNext, g_zDEClient_FeatureMapTree.header->left,
                                              g_zDEClient_FeatureMapTree.header);
        ReleaseTreeSentinels();
        return 3;
    }

    zDEClient_MapTreeNode *outNext = nullptr;
    g_zDEClient_FeatureMapTree.EraseRange(&outNext, g_zDEClient_FeatureMapTree.header->left,
                                          g_zDEClient_FeatureMapTree.header);
    const bool cleared =
        g_zDEClient_FeatureMapTree.nodeCount == 0 &&
        g_zDEClient_FeatureMapTree.header->parent == g_zDEClient_FeatureMapTreeNil;

    ReleaseTreeSentinels();
    return cleared ? 0 : 4;
}

extern "C" int zdeclient_submit_feature_geometry_smoke(void) {
    zGeometry_ClipPatchNodeView nodes[3] = {};
    zGeometry_ClipPatchNodeDiPair firstPairs[2] = {};
    zGeometry_ClipPatchNodeDiPair secondPairs[2] = {};
    firstPairs[0].node = &nodes[0];
    firstPairs[1].node = &nodes[1];
    secondPairs[0].node = &nodes[1];
    secondPairs[1].node = &nodes[2];

    zGeometry_ClipPatchPartitionOutput partitions[2] = {};
    partitions[0].nodeDiPairCount = 2;
    partitions[0].nodeDiPairs = firstPairs;
    partitions[1].nodeDiPairCount = 2;
    partitions[1].nodeDiPairs = secondPairs;

    zGeometry_ClipPatchOutputPartial output = {};
    output.partitionCount = 2;
    output.partitions = partitions;

    g_zDEClient_FeatureMapTree = {};
    g_zDEClient_FeatureMapTreeNil = nullptr;

    zDEClient::SubmitFeatureGeometry(&output);

    const bool ok =
        g_zDEClient_FeatureMapTree.nodeCount == 3 &&
        g_zDEClient_FeatureMapTree.header->left->key == &nodes[0] &&
        g_zDEClient_FeatureMapTree.header->right->key == &nodes[2];

    zDEClient_MapTreeNode *outNext = nullptr;
    g_zDEClient_FeatureMapTree.EraseRange(&outNext, g_zDEClient_FeatureMapTree.header->left,
                                          g_zDEClient_FeatureMapTree.header);
    ReleaseTreeSentinels();
    return ok ? 0 : 1;
}

extern "C" int zdeclient_feature_entry_copy_fill_helpers_smoke(void) {
    zDEClient_FeatureEntry source[2] = {};
    source[0].featureType = 1;
    source[0].eventData.crater.radius = 3.0f;
    source[0].reloadFlag = 7;
    source[1].featureType = 3;
    source[1].eventData.quickSand.radius = 5.0f;
    source[1].reloadFlag = 9;

    zDEClient_FeatureEntry dest[3] = {};
    zDEClient_FeatureEntry *const afterCopy =
        zDEClient::CopyFeatureEntriesForward(source, source + 2, dest);
    if (afterCopy != dest + 2 || dest[0].featureType != 1 ||
        dest[0].eventData.crater.radius != 3.0f || dest[1].featureType != 3 ||
        dest[1].eventData.quickSand.radius != 5.0f) {
        return 1;
    }

    zDEClient::FillFeatureEntries(dest + 2, 1, &source[0]);
    if (dest[2].featureType != 1 || dest[2].eventData.crater.radius != 3.0f ||
        dest[2].reloadFlag != 7) {
        return 2;
    }

    zDEClient::FillFeatureEntries(nullptr, 1, &source[1]);
    return 0;
}

extern "C" int zdeclient_append_feature_entry_smoke(void) {
    g_zDEClient_FeatureListBegin = nullptr;
    g_zDEClient_FeatureListEnd = nullptr;
    g_zDEClient_FeatureListCapacityEnd = nullptr;

    zDEClient_QSandEventTemplate qSand = {};
    qSand.radius = 4.0f;
    if (zDEClient::AppendFeatureEntry(2, &qSand) != 0 ||
        g_zDEClient_FeatureListBegin != nullptr || g_zDEClient_FeatureListEnd != nullptr ||
        g_zDEClient_FeatureListCapacityEnd != nullptr) {
        return 1;
    }

    zDEClient_CraterEventTemplate crater = {};
    crater.radius = 2.0f;
    if (zDEClient::AppendFeatureEntry(1, &crater) != 0 ||
        g_zDEClient_FeatureListBegin == nullptr ||
        g_zDEClient_FeatureListEnd != g_zDEClient_FeatureListBegin + 1 ||
        g_zDEClient_FeatureListBegin[0].featureType != 1 ||
        g_zDEClient_FeatureListBegin[0].eventData.crater.radius != 2.0f ||
        g_zDEClient_FeatureListBegin[0].reloadFlag != 0) {
        ::operator delete(g_zDEClient_FeatureListBegin);
        g_zDEClient_FeatureListBegin = nullptr;
        g_zDEClient_FeatureListEnd = nullptr;
        g_zDEClient_FeatureListCapacityEnd = nullptr;
        return 2;
    }

    if (zDEClient::AppendFeatureEntry(3, &qSand) != 0 ||
        g_zDEClient_FeatureListEnd != g_zDEClient_FeatureListBegin + 2 ||
        g_zDEClient_FeatureListCapacityEnd < g_zDEClient_FeatureListEnd ||
        g_zDEClient_FeatureListBegin[0].featureType != 1 ||
        g_zDEClient_FeatureListBegin[1].featureType != 3 ||
        g_zDEClient_FeatureListBegin[1].eventData.quickSand.radius != 4.0f ||
        g_zDEClient_FeatureListBegin[1].reloadFlag != 0) {
        ::operator delete(g_zDEClient_FeatureListBegin);
        g_zDEClient_FeatureListBegin = nullptr;
        g_zDEClient_FeatureListEnd = nullptr;
        g_zDEClient_FeatureListCapacityEnd = nullptr;
        return 3;
    }

    ::operator delete(g_zDEClient_FeatureListBegin);
    g_zDEClient_FeatureListBegin = nullptr;
    g_zDEClient_FeatureListEnd = nullptr;
    g_zDEClient_FeatureListCapacityEnd = nullptr;
    return 0;
}

extern "C" int zdeclient_clear_feature_display_nodes_smoke(void) {
    ResetZClassTypeLists();
    g_zClass_CurrentZbdPath[0] = '\0';

    zDEClient_MapTreeNode nil = {};
    zDEClient_MapTreeNode header = {};
    zDEClient_MapTreeNode mapNode = {};
    nil.left = &nil;
    nil.parent = &nil;
    nil.right = &nil;
    nil.colorOrNil = 1;
    header.left = &mapNode;
    header.parent = &mapNode;
    header.right = &mapNode;
    header.colorOrNil = 1;
    mapNode.left = &nil;
    mapNode.parent = &header;
    mapNode.right = &nil;
    mapNode.colorOrNil = 1;
    g_zDEClient_FeatureMapTreeNil = &nil;
    g_zDEClient_FeatureMapTree.header = &header;
    g_zDEClient_FeatureMapTree.nodeCount = 1;

    zClass_NodePartial world = {};
    zClass_WorldDataPartial worldData = {};
    zWorldAreaPartial row0[1] = {};
    zWorldAreaPartial *rows[1] = {row0};
    world.classData = &worldData;
    worldData.areaGridRows = rows;
    row0[0].displayRefreshQueued = 1;

    zClass_NodePartial *parents[1] = {&world};
    zGeometry_ClipPatchNodeView clipNode = {};
    clipNode.gridCol = 0;
    clipNode.gridRow = 0;
    clipNode.listA = parents;
    clipNode.listCountA = 1;
    mapNode.key = &clipNode;

    zClass_NodeFreeListSlot slots[1] = {};
    zClass_NodePartial *featureNode = &slots[0].node;
    std::strcpy(featureNode->name, "ZDEC_FEATURE");
    featureNode->classId = 0;
    zDiPartial diPool[1] = {};
    diPool[0].refCount = 1;
    g_zModel_DiPoolBase = diPool;
    g_zModel_DiPoolInUseCount = 1;
    g_zModel_DiPoolFreeHeadIndex = -1;
    featureNode->userDataOrDiRef =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&diPool[0]));
    g_zClass_NodeArray = slots;
    g_zClass_NodeFreeHeadIndex = -1;
    g_zClass_ActiveNodeCount = 1;
    zClass_TypeList::Insert(6, featureNode);

    zDEClient::ClearFeatureDisplayNodes();

    int result = 0;
    if (row0[0].displayRefreshQueued != 0) {
        result = 1;
    } else if (zClass_TypeList::Head(6) != nullptr) {
        result = 2;
    } else if (g_zModel_DiPoolFreeHeadIndex != 0 || g_zModel_DiPoolInUseCount != 0) {
        result = 3;
    } else if (g_zClass_NodeFreeHeadIndex != 0 || g_zClass_ActiveNodeCount != 0) {
        result = 4;
    }

    ResetZClassTypeLists();
    g_zDEClient_FeatureMapTree = {};
    g_zDEClient_FeatureMapTreeNil = nullptr;
    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;
    g_zClass_NodeArray = nullptr;
    g_zClass_NodeFreeHeadIndex = -1;
    g_zClass_ActiveNodeCount = 0;
    return result;
}

extern "C" int zdeclient_feature_leaf_helpers_smoke(void) {
    zGeometry_ClipPatchOutputPartial *clipOutput = zGeometry_ClipPatchOutput::Create();
    if (clipOutput == nullptr || clipOutput->pointCount != 0 || clipOutput->points != nullptr ||
        clipOutput->partitionCount != 0 || clipOutput->partitions != nullptr) {
        return 1;
    }

    clipOutput->partitions = static_cast<zGeometry_ClipPatchPartitionOutput *>(
        std::malloc(sizeof(zGeometry_ClipPatchPartitionOutput)));
    if (clipOutput->partitions == nullptr) {
        zGeometry_ClipPatchOutput::Destroy(clipOutput);
        return 2;
    }
    zGeometry_ClipPatchOutput::Destroy(clipOutput);

    zClass_NodePartial cameraNode = {};
    zDEClient_CameraNodeClassDataPartial cameraData = {};
    zDEClient_FeatureGridCell row0[2] = {};
    zDEClient_FeatureGridCell row1[2] = {};
    zDEClient_FeatureGridCell *rows[2] = {row0, row1};
    cameraData.featureGridRows = rows;
    g_zDEClient_CameraNode = &cameraNode;
    g_zDEClient_CameraNodeClassData = reinterpret_cast<zClass_CameraDataPartial *>(&cameraData);
    row1[1].featureCount = 7;

    if (zDEClient::GetCameraNode() != &cameraNode ||
        zDEClient::GetFeatureGridCell(1, 1) != &row1[1] || row1[1].featureCount != 7) {
        return 3;
    }

    g_zDEClient_CameraNodeClassData = nullptr;
    if (zDEClient::GetFeatureGridCell(0, 0) != nullptr) {
        return 4;
    }
    g_zDEClient_CameraNodeClassData = reinterpret_cast<zClass_CameraDataPartial *>(&cameraData);

    zModel_MaterialPartial quickSandMaterial = {};
    zModel_MaterialPartial quickSandCycle = {};
    g_zDEClient_QuickSandMaterial = &quickSandMaterial;
    g_zDEClient_QuickSandMaterialCycle = &quickSandCycle;
    zDEClient_QSandEventTemplate qSandTemplate = {};
    qSandTemplate.featureFlags = 0x1008;
    qSandTemplate.pointCount = 3;
    qSandTemplate.radius = 4.0f;
    zDEClient_QSandFeature *qSand =
        zDEClient_QSand::CreateFeatureStructFromEventTemplate(&qSandTemplate);
    if (qSand == nullptr || qSand->featureType != 3 || qSand->eventTemplate.pointCount != 3 ||
        qSand->eventTemplate.material != &quickSandMaterial ||
        qSand->eventTemplate.materialCycle != &quickSandCycle || qSand->points == nullptr ||
        qSand->clipPatchOutput == nullptr || qSand->featureGridCell != nullptr) {
        zDEClient_QSand::DestroyFeature(qSand);
        return 5;
    }
    zDEClient_QSand::DestroyFeature(qSand);
    zDEClient_QSand::DestroyFeature(nullptr);

    zModel_MaterialSlot sourceSlot = {};
    zDEClient_CraterDisplaySourceEntry displaySources[2] = {};
    displaySources[0].craterMaterial = &quickSandMaterial;
    displaySources[1].sourceMaterial = reinterpret_cast<zModel_MaterialPartial *>(&sourceSlot);
    displaySources[1].craterMaterial = &quickSandCycle;
    g_zDEClient_CraterDisplaySourceList = displaySources;
    g_zDEClient_CraterDisplaySourceCount = 2;
    zDEClient_CraterEventTemplate craterTemplate = {};
    craterTemplate.featureFlags = 0x1008;
    craterTemplate.pointCount = 2;
    craterTemplate.craterMaterialSlot = &sourceSlot;
    craterTemplate.radius = 5.0f;
    zDEClient_CraterFeature *crater =
        zDEClient_Crater::CreateFeatureStructFromEventTemplate(&craterTemplate);
    if (crater == nullptr || crater->featureType != 1 ||
        crater->eventTemplate.pointCount != 2 || crater->points == nullptr ||
        crater->clipPatchOutput == nullptr ||
        crater->displaySourceEntry != &displaySources[1]) {
        zDEClient_Crater::DestroyFeature(crater);
        return 6;
    }
    zDEClient_Crater::DestroyFeature(crater);
    zDEClient_Crater::DestroyFeature(nullptr);

    g_zDEClient_CameraNode = nullptr;
    g_zDEClient_CameraNodeClassData = nullptr;
    g_zDEClient_QuickSandMaterial = nullptr;
    g_zDEClient_QuickSandMaterialCycle = nullptr;
    g_zDEClient_CraterDisplaySourceList = nullptr;
    g_zDEClient_CraterDisplaySourceCount = 0;
    return 0;
}

extern "C" int zdeclient_feature_init_helpers_smoke(void) {
    zClass_NodePartial world = {};
    zClass_WorldDataPartial worldData = {};
    zDEClient_CameraNodeClassDataPartial cameraData = {};
    zDEClient_FeatureGridCell cell = {};
    zDEClient_FeatureGridCell *rows[1] = {};
    SetupSingleFeatureGridCell(&world, &worldData, &cameraData, &cell, rows);

    zModel_MaterialPartial quickSandMaterial = {};
    zModel_MaterialPartial quickSandCycle = {};
    g_zDEClient_QuickSandMaterial = &quickSandMaterial;
    g_zDEClient_QuickSandMaterialCycle = &quickSandCycle;

    zDEClient_QSandEventTemplate qSandTemplate = {};
    qSandTemplate.featureFlags = 0x1008;
    qSandTemplate.pointCount = 4;
    qSandTemplate.radius = 2.0f;
    qSandTemplate.center.x = 5.0f;
    qSandTemplate.center.y = 3.0f;
    qSandTemplate.center.z = -5.0f;

    zDEClient_QSandFeature *qSand =
        zDEClient_QSand::InitFeatureFromEventTemplate(&qSandTemplate);
    if (qSand == nullptr || qSand->featureGridCell != &cell ||
        !ValidateCircleFeaturePoints(qSand->points, 5.0f, 3.0f, -5.0f, 2.0f) ||
        !NearlyEqual(qSand->boundsMinX, 3.0f) || !NearlyEqual(qSand->boundsMaxX, 7.0f) ||
        !NearlyEqual(qSand->boundsMinZ, -7.0f) || !NearlyEqual(qSand->boundsMaxZ, -3.0f)) {
        zDEClient_QSand::DestroyFeature(qSand);
        return 1;
    }
    zDEClient_QSand::DestroyFeature(qSand);

    cell.featureCount = worldData.partitionMaxDecFeatureCount;
    if (zDEClient_QSand::InitFeatureFromEventTemplate(&qSandTemplate) != nullptr) {
        return 2;
    }
    cell.featureCount = 0;

    zModel_MaterialPartial craterMaterial = {};
    zDEClient_CraterDisplaySourceEntry displaySource = {};
    displaySource.craterMaterial = &craterMaterial;
    g_zDEClient_CraterDisplaySourceList = &displaySource;
    g_zDEClient_CraterDisplaySourceCount = 1;

    zDEClient_CraterEventTemplate craterTemplate = {};
    craterTemplate.featureFlags = 0x1008;
    craterTemplate.pointCount = 4;
    craterTemplate.radius = 2.0f;
    craterTemplate.center.x = 5.0f;
    craterTemplate.center.y = 3.0f;
    craterTemplate.center.z = -5.0f;

    zDEClient_CraterFeature *crater =
        zDEClient_Crater::InitFeatureFromEventTemplate(&craterTemplate);
    if (crater == nullptr || crater->featureGridCell != &cell ||
        crater->displaySourceEntry != &displaySource ||
        !ValidateCircleFeaturePoints(crater->points, 5.0f, 3.0f, -5.0f, 2.0f) ||
        !NearlyEqual(crater->boundsMinX, 3.0f) || !NearlyEqual(crater->boundsMaxX, 7.0f) ||
        !NearlyEqual(crater->boundsMinZ, -7.0f) ||
        !NearlyEqual(crater->boundsMaxZ, -3.0f)) {
        zDEClient_Crater::DestroyFeature(crater);
        return 3;
    }
    zDEClient_Crater::DestroyFeature(crater);

    cell.featureCount = worldData.partitionMaxDecFeatureCount;
    if (zDEClient_Crater::InitFeatureFromEventTemplate(&craterTemplate) != nullptr) {
        return 4;
    }

    g_zDEClient_CameraNode = nullptr;
    g_zDEClient_CameraNodeClassData = nullptr;
    g_zDEClient_QuickSandMaterial = nullptr;
    g_zDEClient_QuickSandMaterialCycle = nullptr;
    g_zDEClient_CraterDisplaySourceList = nullptr;
    g_zDEClient_CraterDisplaySourceCount = 0;
    return 0;
}

extern "C" int zdeclient_create_feature_node_from_partition_smoke(void) {
    ResetZClassTypeLists();

    zClass_NodeFreeListSlot nodeSlots[1] = {};
    nodeSlots[0].freeTag = 0x00ffffff;
    g_zClass_NodeArray = nodeSlots;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;

    zDiPartial diPool[1] = {};
    diPool[0].nextFreeIndex = -1;
    g_zModel_DiPoolBase = diPool;
    g_zModel_DiPoolCapacity = 1;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = 0;

    zClass_NodePartial parent = {};
    parent.classId = 3;

    zClass_NodePartial sourceNode = {};
    sourceNode.flags = 0x10000;
    sourceNode.nodeType = 0x7d;
    zGeometry_ClipPatchNodeDiPair pairs[1] = {};
    pairs[0].node = &sourceNode;

    zGeometry_ClipPatchPartitionOutput partition = {};
    partition.nodeDiPairCount = 1;
    partition.nodeDiPairs = pairs;

    zClass_NodePartial *createdNode = nullptr;
    zDiPartial *displayInstance =
        zDEClient::CreateFeatureNodeAndDiFromClipPatchPartition(&partition, &parent, &createdNode);
    const bool ok =
        displayInstance == &diPool[0] && createdNode == &nodeSlots[0].node &&
        createdNode->nodeType == 0x7d && (createdNode->flags & 0x20000) != 0 &&
        reinterpret_cast<zDiPartial *>(createdNode->userDataOrDiRef) == displayInstance &&
        displayInstance->refCount == 1 && parent.listCountB == 1 &&
        parent.listB[0] == createdNode && createdNode->listCountA == 1 &&
        createdNode->listA[0] == &parent && g_zModel_DiPoolInUseCount == 1 &&
        g_zModel_DiPoolFreeHeadIndex == -1;

    std::free(parent.listB);
    std::free(createdNode->listA);
    std::free(createdNode->classData);
    ResetZClassTypeLists();
    g_zClass_NodeArray = nullptr;
    g_zClass_NodeFreeHeadIndex = -1;
    g_zClass_ActiveNodeCount = 0;
    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;

    if (!ok) {
        return 1;
    }

    return zDEClient::CreateFeatureNodeAndDiFromClipPatchPartition(nullptr, &parent,
                                                                   &createdNode) == nullptr
               ? 0
               : 2;
}

extern "C" int zdeclient_qsand_create_feature_smoke(void) {
    ResetZClassTypeLists();

    zClass_NodeFreeListSlot nodeSlots[2] = {};
    nodeSlots[0].freeTag = 1;
    nodeSlots[1].freeTag = 0x00ffffff;
    g_zClass_NodeArray = nodeSlots;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;

    zDiPartial diPool[2] = {};
    diPool[0].nextFreeIndex = 1;
    diPool[1].nextFreeIndex = -1;
    g_zModel_DiPoolBase = diPool;
    g_zModel_DiPoolCapacity = 2;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = 0;
    g_zModel_MaxPolygonVertexCountBeforeSplit = 64;
    g_zModel_CoplanarTolerance = 0.01;

    zClass_NodePartial cameraNode = {};
    cameraNode.classId = 3;
    g_zDEClient_CameraNode = &cameraNode;

    zGeometry_ClipPatchNodeView sourceNode = {};
    sourceNode.flags = 0x10000;
    sourceNode.nodeType = 0x42;
    zGeometry_ClipPatchNodeDiPair pairs[1] = {};
    pairs[0].node = &sourceNode;

    zGeometry_ClipPatchPartitionOutput partition = {};
    partition.nodeDiPairCount = 1;
    partition.nodeDiPairs = pairs;

    zGeometry_ClipPatchOutputPartial clipOutput = {};
    clipOutput.partitionCount = 1;
    clipOutput.partitions = &partition;

    zVec3 points[3] = {
        {0.0f, 0.0f, 2.0f},
        {2.0f, 0.0f, -1.0f},
        {-2.0f, 0.0f, -1.0f},
    };
    zModel_MaterialPartial sideMaterial = {};
    zModel_MaterialPartial cycleMaterial = {};

    zDEClient_QSandFeature feature = {};
    feature.eventTemplate.featureFlags = 0x1008;
    feature.eventTemplate.pointCount = 3;
    feature.eventTemplate.material = &sideMaterial;
    feature.eventTemplate.materialCycle = &cycleMaterial;
    feature.eventTemplate.depth = 1.0f;
    feature.eventTemplate.radius = 3.0f;
    feature.eventTemplate.center = {0.0f, 0.0f, 0.0f};
    feature.points = points;
    feature.clipPatchOutput = &clipOutput;

    zClass_NodePartial *sideNode = &nodeSlots[0].node;
    zClass_NodePartial *capNode = &nodeSlots[1].node;
    const int result = zDEClient_QSand::CreateFeature(&feature);
    int failCode = 0;
    if (result != 0) {
        failCode = 10;
    } else if (g_zModel_DiPoolInUseCount != 2 || g_zModel_DiPoolFreeHeadIndex != -1) {
        failCode = 11;
    } else if (g_zClass_ActiveNodeCount != 2 || cameraNode.listCountB != 2) {
        failCode = 12;
    } else if (cameraNode.listB[0] != sideNode || cameraNode.listB[1] != capNode) {
        failCode = 13;
    } else if (std::strcmp(sideNode->name, "ZDEC_FEATURE") != 0 ||
               std::strcmp(capNode->name, "ZDEC_FEATURE") != 0) {
        failCode = 14;
    } else if (sideNode->callbackContext != reinterpret_cast<zClass_NodePartial *>(&feature) ||
               capNode->callbackContext != reinterpret_cast<zClass_NodePartial *>(&feature)) {
        failCode = 15;
    } else if (reinterpret_cast<zDiPartial *>(sideNode->userDataOrDiRef) != &diPool[0] ||
               reinterpret_cast<zDiPartial *>(capNode->userDataOrDiRef) != &diPool[1]) {
        failCode = 16;
    } else if (diPool[0].entryCount != 6 || diPool[0].vertCount <= 0) {
        failCode = 17;
    } else if (diPool[1].entryCount != 3 || diPool[1].vertCount <= 0) {
        failCode = 18;
    } else if (!NearlyEqual(feature.eventTemplate.center.y, -1.0f)) {
        failCode = 19;
    }

    zDi::FreeContents(&diPool[0]);
    zDi::FreeContents(&diPool[1]);
    std::free(cameraNode.listB);
    std::free(sideNode->listA);
    std::free(capNode->listA);
    std::free(sideNode->classData);
    std::free(capNode->classData);
    ResetZClassTypeLists();
    g_zClass_NodeArray = nullptr;
    g_zClass_NodeFreeHeadIndex = -1;
    g_zClass_ActiveNodeCount = 0;
    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;
    g_zDEClient_CameraNode = nullptr;

    if (failCode != 0) {
        return failCode;
    }

    zDEClient_QSandFeature failingFeature = {};
    failingFeature.clipPatchOutput = &clipOutput;
    return zDEClient_QSand::CreateFeature(&failingFeature) == -1 ? 0 : 2;
}

extern "C" int zdeclient_crater_build_and_create_feature_smoke(void) {
    zDEClient_CraterFeature buildFeature = {};
    zGeometry_ClipPatchOutputPartial buildOutput = {};
    buildFeature.clipPatchOutput = &buildOutput;
    if (zDEClient_Crater::Build(&buildFeature) != 0) {
        return 1;
    }

    zClass_NodePartial clipCameraNode = {};
    g_zDEClient_CameraNode = &clipCameraNode;

    zDEClient_FeatureGridCell cell = {};
    buildFeature.featureGridCell = &cell;
    buildFeature.clipPatchOutput = &buildOutput;

    zVec3 outline[4] = {
        {0.0f, 0.0f, 0.0f},
        {10.0f, 0.0f, 0.0f},
        {10.0f, 0.0f, -10.0f},
        {0.0f, 0.0f, -10.0f},
    };
    buildFeature.points = outline;
    buildFeature.eventTemplate.pointCount = 4;
    if (zDEClient_Crater::Build(&buildFeature) != 0 || buildFeature.points != outline ||
        buildFeature.eventTemplate.pointCount != 4 || buildOutput.partitionCount != 0 ||
        buildOutput.partitions != nullptr || buildOutput.points != nullptr) {
        zGeometry_ClipPatchOutput::Destroy(&buildOutput);
        g_zDEClient_CameraNode = nullptr;
        return 2;
    }

    ResetZClassTypeLists();

    zClass_NodeFreeListSlot nodeSlots[1] = {};
    nodeSlots[0].freeTag = 0x00ffffff;
    g_zClass_NodeArray = nodeSlots;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;

    zDiPartial diPool[1] = {};
    diPool[0].nextFreeIndex = -1;
    g_zModel_DiPoolBase = diPool;
    g_zModel_DiPoolCapacity = 1;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = 0;
    g_zModel_MaxPolygonVertexCountBeforeSplit = 64;
    g_zModel_CoplanarTolerance = 0.01;

    zClass_NodePartial cameraNode = {};
    cameraNode.classId = 3;
    g_zDEClient_CameraNode = &cameraNode;

    zGeometry_ClipPatchNodeView sourceNode = {};
    sourceNode.flags = 0x10000;
    sourceNode.nodeType = 0x24;
    zGeometry_ClipPatchNodeDiPair pairs[1] = {};
    pairs[0].node = &sourceNode;

    zGeometry_ClipPatchPartitionOutput partition = {};
    partition.nodeDiPairCount = 1;
    partition.nodeDiPairs = pairs;

    zGeometry_ClipPatchOutputPartial clipOutput = {};
    clipOutput.partitionCount = 1;
    clipOutput.partitions = &partition;

    zVec3 craterPoints[3] = {
        {0.0f, 0.0f, 2.0f},
        {2.0f, 0.0f, -1.0f},
        {-2.0f, 0.0f, -1.0f},
    };
    zModel_MaterialPartial craterMaterial = {};
    zDEClient_CraterDisplaySourceEntry displaySource = {};
    displaySource.craterMaterial = &craterMaterial;

    zDEClient_CraterFeature feature = {};
    feature.eventTemplate.featureFlags = 0x1008;
    feature.eventTemplate.pointCount = 3;
    feature.eventTemplate.depth = 1.0f;
    feature.eventTemplate.radius = 3.0f;
    feature.eventTemplate.center = {0.0f, 0.0f, 0.0f};
    feature.points = craterPoints;
    feature.clipPatchOutput = &clipOutput;
    feature.displaySourceEntry = &displaySource;

    zClass_NodePartial *featureNode = &nodeSlots[0].node;
    const int createResult = zDEClient_Crater::CreateFeature(&feature);
    int failCode = 0;
    if (createResult != 0) {
        failCode = 10;
    } else if (g_zModel_DiPoolInUseCount != 1 || g_zModel_DiPoolFreeHeadIndex != -1) {
        failCode = 11;
    } else if (g_zClass_ActiveNodeCount != 1 || cameraNode.listCountB != 1 ||
               cameraNode.listB[0] != featureNode) {
        failCode = 12;
    } else if (std::strcmp(featureNode->name, "ZDEC_FEATURE") != 0) {
        failCode = 13;
    } else if (featureNode->callbackContext != reinterpret_cast<zClass_NodePartial *>(&feature)) {
        failCode = 14;
    } else if (reinterpret_cast<zDiPartial *>(featureNode->userDataOrDiRef) != &diPool[0]) {
        failCode = 15;
    } else if (diPool[0].entryCount != 6 || diPool[0].vertCount <= 0) {
        failCode = 16;
    } else if (!NearlyEqual(feature.eventTemplate.center.y, -1.0f)) {
        failCode = 17;
    }

    zDi::FreeContents(&diPool[0]);
    std::free(cameraNode.listB);
    std::free(featureNode->listA);
    std::free(featureNode->classData);
    ResetZClassTypeLists();
    g_zClass_NodeArray = nullptr;
    g_zClass_NodeFreeHeadIndex = -1;
    g_zClass_ActiveNodeCount = 0;
    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;
    g_zDEClient_CameraNode = nullptr;

    return failCode;
}

extern "C" int zdeclient_crater_instance_event_smoke(void) {
    zClass_NodePartial world = {};
    zClass_WorldDataPartial worldData = {};
    zDEClient_CameraNodeClassDataPartial cameraData = {};
    zDEClient_FeatureGridCell cell = {};
    zDEClient_FeatureGridCell *rows[1] = {};
    SetupSingleFeatureGridCell(&world, &worldData, &cameraData, &cell, rows);

    zModel_MaterialPartial craterMaterial = {};
    zDEClient_CraterDisplaySourceEntry displaySource = {};
    displaySource.craterMaterial = &craterMaterial;
    g_zDEClient_CraterDisplaySourceList = &displaySource;
    g_zDEClient_CraterDisplaySourceCount = 1;

    zDEClient_CraterEventTemplate eventTemplate = {};
    eventTemplate.featureFlags = 0x1008;
    eventTemplate.pointCount = 4;
    eventTemplate.depth = 1.0f;
    eventTemplate.radius = 2.0f;
    eventTemplate.center = {5.0f, 3.0f, -5.0f};

    zModel_Const::SetVertexMergeEpsilon(0.25f);
    const int result = zDEClient_Crater::InstanceEvent(&eventTemplate, 1);
    const bool ok = result == -1 && zModel_Const::GetVertexMergeEpsilon() == 0.25f;

    g_zDEClient_CameraNode = nullptr;
    g_zDEClient_CameraNodeClassData = nullptr;
    g_zDEClient_CraterDisplaySourceList = nullptr;
    g_zDEClient_CraterDisplaySourceCount = 0;
    return ok ? 0 : 1;
}

extern "C" int zdeclient_crater_instance_event_maybe_relay_smoke(void) {
    zDEClient_CraterEventTemplate eventTemplate = {};
    eventTemplate.featureFlags = 0x1008;
    eventTemplate.pointCount = 4;
    eventTemplate.depth = 1.0f;
    eventTemplate.radius = 2.0f;
    eventTemplate.center = {5.0f, 3.0f, -5.0f};

    g_craterRelayCallCount = 0;
    g_craterRelayResult = 0;
    g_zDEClientCraterNetRelayCallback = TestCraterRelayCallback;
    zModel_Const::SetVertexMergeEpsilon(0.25f);
    if (zDEClient_Crater::InstanceEventMaybeRelay(&eventTemplate) != -1 ||
        g_craterRelayCallCount != 1 || zModel_Const::GetVertexMergeEpsilon() != 0.25f) {
        g_zDEClientCraterNetRelayCallback = nullptr;
        return 1;
    }

    g_craterRelayCallCount = 0;
    g_craterRelayResult = 1;
    zClass_NodePartial cameraNode = {};
    g_zDEClient_CameraNode = &cameraNode;
    g_zDEClient_CameraNodeClassData = nullptr;
    if (zDEClient_Crater::InstanceEventMaybeRelay(&eventTemplate) != -1 ||
        g_craterRelayCallCount != 1 || zModel_Const::GetVertexMergeEpsilon() != 0.25f) {
        g_zDEClientCraterNetRelayCallback = nullptr;
        g_zDEClient_CameraNode = nullptr;
        return 2;
    }

    g_craterRelayCallCount = 0;
    g_zDEClientCraterNetRelayCallback = nullptr;
    if (zDEClient_Crater::InstanceEventMaybeRelay(&eventTemplate) != -1 ||
        g_craterRelayCallCount != 0 || zModel_Const::GetVertexMergeEpsilon() != 0.25f) {
        g_zDEClient_CameraNode = nullptr;
        return 3;
    }

    g_zDEClient_CameraNode = nullptr;
    return 0;
}

extern "C" int zdeclient_crater_net_relay_callback_smoke(void) {
    const int oldIsHost = g_zNetwork_IsHostFlag;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    zModel_MaterialSlot *const oldMatlPool = g_zModel_MatlPool;
    const int oldMatlCapacity = g_zModel_MatlPoolCapacity;
    const int oldMatlInUse = g_zModel_MatlPoolInUseCount;
    const zDEClient_CraterEventTemplate oldDefaults = g_zDEClient_CraterEventTemplateDefaults;
    zDEClient_NetRelayCallback const oldRelayCallback = g_zDEClientCraterNetRelayCallback;

    zModel_MaterialSlot materialSlots[4] = {};
    g_zModel_MatlPool = materialSlots;
    g_zModel_MatlPoolCapacity = 4;
    g_zModel_MatlPoolInUseCount = 4;
    g_zDEClient_CraterEventTemplateDefaults = {};
    g_zDEClient_CraterEventTemplateDefaults.featureFlags = 0x1008;
    g_zDEClient_CraterEventTemplateDefaults.pointCount = 4;
    g_zDEClient_CraterEventTemplateDefaults.depth = 1.0f;

    zNetwork_DPlay4 dplay = {&kCraterNetRelayDPlayVtable};
    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x34567890;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x34567890;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_zDEClientCraterNetRelayCallback = TestCraterRelayCallback;

    NetPkt0F_CraterEvent packet = {
        {0x0f, sizeof(NetPkt0F_CraterEvent), 0x1234}, 0, 2, {1.0f, 2.0f, 3.0f}, 4.5f,
    };

    g_zNetwork_IsHostFlag = 0;
    g_craterRelayCallCount = 0;
    g_craterRelayResult = 0;
    g_craterNetRelaySendCalls = 0;
    const int nonHostNoRelayResult = zDEClient_Crater::NetRelayCallback(0x77, &packet);
    const bool nonHostNoRelayOk =
        nonHostNoRelayResult == 1 && g_craterRelayCallCount == 0 &&
        g_craterNetRelaySendCalls == 0 && packet.header.payloadDword0 == 0x1234 &&
        packet.eventFlags == 0;

    packet.eventFlags = 0x80u;
    g_craterRelayCallCount = 0;
    const int nonHostRelayResult = zDEClient_Crater::NetRelayCallback(0x77, &packet);
    const bool nonHostRelayOk =
        nonHostRelayResult == 1 && g_craterRelayCallCount == 1 &&
        g_craterNetRelaySendCalls == 0 && packet.header.payloadDword0 == 0x1234 &&
        packet.eventFlags == 0x80u;

    packet.eventFlags = 0;
    g_zNetwork_IsHostFlag = 1;
    g_craterRelayCallCount = 0;
    const int hostRejectedResult = zDEClient_Crater::NetRelayCallback(0x77, &packet);
    const bool hostRejectedOk =
        hostRejectedResult == 1 && g_craterRelayCallCount == 1 &&
        g_craterNetRelaySendCalls == 0 && packet.header.payloadDword0 == 0x1234 &&
        packet.eventFlags == 0;

    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_zModel_MatlPool = oldMatlPool;
    g_zModel_MatlPoolCapacity = oldMatlCapacity;
    g_zModel_MatlPoolInUseCount = oldMatlInUse;
    g_zDEClient_CraterEventTemplateDefaults = oldDefaults;
    g_zDEClientCraterNetRelayCallback = oldRelayCallback;
    g_craterNetRelaySendCalls = 0;
    g_craterNetRelaySendFlags = 0;
    g_craterNetRelaySendPacket = nullptr;
    g_craterNetRelaySendPacketSize = 0;

    return nonHostNoRelayOk && nonHostRelayOk && hostRejectedOk ? 0 : 1;
}

extern "C" int zdeclient_crater_execute_smoke(void) {
    const NetPkt0F_CraterEvent oldPacket = g_NetPkt0F_CraterEventSendBuf;
    const int oldIsHost = g_zNetwork_IsHostFlag;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zModel_MaterialSlot *const oldMatlPool = g_zModel_MatlPool;
    const int oldMatlCapacity = g_zModel_MatlPoolCapacity;
    const int oldMatlInUse = g_zModel_MatlPoolInUseCount;
    const zDEClient_CraterEventTemplate oldDefaults = g_zDEClient_CraterEventTemplateDefaults;
    zDEClient_NetRelayCallback const oldRelayCallback = g_zDEClientCraterNetRelayCallback;

    zClass_NodePartial ownerRoot = {};
    zClass_NodePartial otherRoot = {};
    zUtil_PlayerStateStorage playerState = {};
    playerState.rootNode = &ownerRoot;
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;

    zModel_MaterialSlot materialSlots[4] = {};
    g_zModel_MatlPool = materialSlots;
    g_zModel_MatlPoolCapacity = 4;
    g_zModel_MatlPoolInUseCount = 4;

    zNetwork_DPlay4 dplay = {&kCraterNetRelayDPlayVtable};
    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x24681357;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = localPlayer.playerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_zNetwork_IsHostFlag = 0;
    g_zDEClientCraterNetRelayCallback = TestCraterRelayCallback;
    g_zDEClient_CraterEventTemplateDefaults = {};
    g_zDEClient_CraterEventTemplateDefaults.pointCount = 4;

    zDEClient_CraterEventTemplate negativeEvent = {};
    negativeEvent.radius = -3.5f;
    const int negativeResult = zDEClient_Crater::Execute(&negativeEvent);
    const bool negativeOk = negativeResult == 1 && NearlyEqual(negativeEvent.radius, 3.5f);

    g_NetPkt0F_CraterEventSendBuf = {{0x0f, sizeof(NetPkt0F_CraterEvent), 0}, 0x20u, -1,
                                     {0.0f, 0.0f, 0.0f}, 0.0f};
    g_craterNetRelaySendCalls = 0;
    zDEClient_CraterEventTemplate otherOwnerEvent = {};
    otherOwnerEvent.craterMaterialSlot = &materialSlots[1];
    otherOwnerEvent.radius = 6.0f;
    otherOwnerEvent.center = {1.0f, 2.0f, 3.0f};
    otherOwnerEvent.damageOwnerNode = &otherRoot;
    const int otherOwnerResult = zDEClient_Crater::Execute(&otherOwnerEvent);
    const bool otherOwnerOk = otherOwnerResult == 0 && g_craterNetRelaySendCalls == 0 &&
                              g_NetPkt0F_CraterEventSendBuf.craterTypeId == -1 &&
                              g_NetPkt0F_CraterEventSendBuf.header.payloadDword0 == 0;

    zDEClient_CraterEventTemplate nonHostEvent = {};
    nonHostEvent.craterMaterialSlot = &materialSlots[2];
    nonHostEvent.radius = 7.25f;
    nonHostEvent.center = {4.0f, 5.0f, 6.0f};
    nonHostEvent.damageOwnerNode = &ownerRoot;
    g_NetPkt0F_CraterEventSendBuf = {{0x0f, sizeof(NetPkt0F_CraterEvent), 0}, 0x20u, -1,
                                     {0.0f, 0.0f, 0.0f}, 0.0f};
    g_craterNetRelaySendCalls = 0;
    const int nonHostResult = zDEClient_Crater::Execute(&nonHostEvent);
    const NetPkt0F_CraterEvent *const sentPacket =
        reinterpret_cast<const NetPkt0F_CraterEvent *>(g_craterNetRelaySendPacket);
    const bool nonHostOk =
        nonHostResult == 0 && g_craterNetRelaySendCalls == 1 &&
        g_craterNetRelaySendFlags == 1 && g_craterNetRelaySendPacketSize == sizeof(NetPkt0F_CraterEvent) &&
        sentPacket != nullptr && sentPacket->header.payloadDword0 == localPlayer.playerKey &&
        sentPacket->eventFlags == 0x20u && sentPacket->craterTypeId == 2 &&
        NearlyEqual(sentPacket->center.x, 4.0f) && NearlyEqual(sentPacket->center.y, 5.0f) &&
        NearlyEqual(sentPacket->center.z, 6.0f) && NearlyEqual(sentPacket->radius, 7.25f);

    g_zNetwork_IsHostFlag = 1;
    g_craterRelayCallCount = 0;
    g_craterRelayResult = 0;
    g_craterNetRelaySendCalls = 0;
    zDEClient_CraterEventTemplate hostEvent = nonHostEvent;
    hostEvent.radius = 8.5f;
    g_NetPkt0F_CraterEventSendBuf = {{0x0f, sizeof(NetPkt0F_CraterEvent), 0x1111}, 0x10u, -1,
                                     {0.0f, 0.0f, 0.0f}, 0.0f};
    const int hostResult = zDEClient_Crater::Execute(&hostEvent);
    const bool hostOk =
        hostResult == 0 && g_craterRelayCallCount == 1 && g_craterNetRelaySendCalls == 0 &&
        g_NetPkt0F_CraterEventSendBuf.header.payloadDword0 == localPlayer.playerKey &&
        g_NetPkt0F_CraterEventSendBuf.eventFlags == 0x10u &&
        g_NetPkt0F_CraterEventSendBuf.craterTypeId == 2 &&
        NearlyEqual(g_NetPkt0F_CraterEventSendBuf.radius, 8.5f);

    g_NetPkt0F_CraterEventSendBuf = oldPacket;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_zModel_MatlPool = oldMatlPool;
    g_zModel_MatlPoolCapacity = oldMatlCapacity;
    g_zModel_MatlPoolInUseCount = oldMatlInUse;
    g_zDEClient_CraterEventTemplateDefaults = oldDefaults;
    g_zDEClientCraterNetRelayCallback = oldRelayCallback;
    g_craterNetRelaySendCalls = 0;
    g_craterNetRelaySendFlags = 0;
    g_craterNetRelaySendPacket = nullptr;
    g_craterNetRelaySendPacketSize = 0;

    if (!negativeOk) {
        return 1;
    }
    if (!otherOwnerOk) {
        return 2;
    }
    if (!nonHostOk) {
        return 3;
    }
    return hostOk ? 0 : 4;
}

extern "C" int zdeclient_qsand_net_relay_callback_smoke(void) {
    const int oldIsHost = g_zNetwork_IsHostFlag;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    const zDEClient_QSandEventTemplate oldDefaults = g_zDEClient_QuickSandEventTemplateDefaults;
    zDEClient_NetRelayCallback const oldRelayCallback = g_zDEClientQSandNetRelayCallback;

    g_zDEClient_QuickSandEventTemplateDefaults = {};
    g_zDEClient_QuickSandEventTemplateDefaults.pointCount = 4;
    g_zDEClient_QuickSandEventTemplateDefaults.depth = 1.0f;

    zNetwork_DPlay4 dplay = {&kCraterNetRelayDPlayVtable};
    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x45678901;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x45678901;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_zDEClientQSandNetRelayCallback = TestQSandRelayCallback;

    NetPkt10_QSandEvent packet = {
        {0x10, sizeof(NetPkt10_QSandEvent), 0x2345}, 0, 0, {2.0f, 3.0f, 4.0f}, 6.5f,
    };

    g_zNetwork_IsHostFlag = 0;
    g_qsandRelayCallCount = 0;
    g_qsandRelayResult = 0;
    g_craterNetRelaySendCalls = 0;
    const int nonHostNoRelayResult = zDEClient_QSand::NetRelayCallback(0x77, &packet);
    const bool nonHostNoRelayOk =
        nonHostNoRelayResult == 1 && g_qsandRelayCallCount == 0 &&
        g_craterNetRelaySendCalls == 0 && packet.header.payloadDword0 == 0x2345 &&
        packet.eventFlags == 0;

    packet.eventFlags = 0x80u;
    g_qsandRelayCallCount = 0;
    const int nonHostRelayResult = zDEClient_QSand::NetRelayCallback(0x77, &packet);
    const bool nonHostRelayOk =
        nonHostRelayResult == 1 && g_qsandRelayCallCount == 1 &&
        g_craterNetRelaySendCalls == 0 && packet.header.payloadDword0 == 0x2345 &&
        packet.eventFlags == 0x80u;

    packet.eventFlags = 0;
    g_zNetwork_IsHostFlag = 1;
    g_qsandRelayCallCount = 0;
    const int hostRejectedResult = zDEClient_QSand::NetRelayCallback(0x77, &packet);
    const bool hostRejectedOk =
        hostRejectedResult == 1 && g_qsandRelayCallCount == 1 &&
        g_craterNetRelaySendCalls == 0 && packet.header.payloadDword0 == 0x2345 &&
        packet.eventFlags == 0;

    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_zDEClient_QuickSandEventTemplateDefaults = oldDefaults;
    g_zDEClientQSandNetRelayCallback = oldRelayCallback;
    g_craterNetRelaySendCalls = 0;
    g_craterNetRelaySendFlags = 0;
    g_craterNetRelaySendPacket = nullptr;
    g_craterNetRelaySendPacketSize = 0;

    return nonHostNoRelayOk && nonHostRelayOk && hostRejectedOk ? 0 : 1;
}

extern "C" int zdeclient_apply_feature_entry_reload_smoke(void) {
    SetupSingleNodeFeatureTree();
    auto *entries =
        static_cast<zDEClient_FeatureEntry *>(std::malloc(sizeof(zDEClient_FeatureEntry) * 2));
    if (entries == nullptr) {
        ReleaseTreeSentinels();
        return 1;
    }

    g_zDEClient_FeatureListBegin = entries;
    g_zDEClient_FeatureListEnd = entries + 2;
    g_zDEClient_FeatureListCapacityEnd = entries + 2;

    zDEClient_FeatureEntry entry = {};
    entry.reloadFlag = 1;
    zDEClient::ApplyFeatureEntry(&entry, nullptr, nullptr);

    const bool ok =
        g_zDEClient_FeatureListEnd == g_zDEClient_FeatureListBegin &&
        g_zDEClient_FeatureMapTree.nodeCount == 0 &&
        g_zDEClient_FeatureMapTree.header->parent == g_zDEClient_FeatureMapTreeNil &&
        g_zDEClient_FeatureMapTree.header->left == g_zDEClient_FeatureMapTree.header &&
        g_zDEClient_FeatureMapTree.header->right == g_zDEClient_FeatureMapTree.header;

    std::free(entries);
    ReleaseTreeSentinels();
    g_zDEClient_FeatureListBegin = nullptr;
    g_zDEClient_FeatureListEnd = nullptr;
    g_zDEClient_FeatureListCapacityEnd = nullptr;
    return ok ? 0 : 2;
}

extern "C" int zdeclient_dispatch_feature_event_templates_smoke(void) {
    zDEClient_FeatureEntry *const oldBegin = g_zDEClient_FeatureListBegin;
    zDEClient_FeatureEntry *const oldEnd = g_zDEClient_FeatureListEnd;
    zDEClient_FeatureEntry *const oldCapacityEnd = g_zDEClient_FeatureListCapacityEnd;

    zDEClient_FeatureEntry entries[4] = {};
    entries[0].featureType = 1;
    entries[0].eventData.crater.radius = 3.0f;
    entries[1].featureType = 3;
    entries[1].eventData.quickSand.radius = 4.0f;
    entries[2].featureType = 2;
    entries[2].eventData.crater.radius = 99.0f;
    entries[3].featureType = 1;
    entries[3].eventData.crater.radius = 5.0f;

    g_zDEClient_FeatureListBegin = entries;
    g_zDEClient_FeatureListEnd = entries + 4;
    g_zDEClient_FeatureListCapacityEnd = entries + 4;

    g_dispatchCraterCallCount = 0;
    g_dispatchQSandCallCount = 0;
    g_dispatchCraterRadius[0] = 0.0f;
    g_dispatchCraterRadius[1] = 0.0f;
    g_dispatchQSandRadius = 0.0f;
    zDEClient::DispatchFeatureEventTemplates(DispatchCraterCallback, DispatchQSandCallback);

    const bool callbacksOk =
        g_dispatchCraterCallCount == 2 && g_dispatchQSandCallCount == 1 &&
        g_dispatchCraterRadius[0] == 3.0f && g_dispatchCraterRadius[1] == 5.0f &&
        g_dispatchQSandRadius == 4.0f;
    const bool copiedEntryOk = entries[0].eventData.crater.radius == 3.0f &&
                               entries[1].eventData.quickSand.radius == 4.0f &&
                               entries[3].eventData.crater.radius == 5.0f;

    g_dispatchCraterCallCount = 0;
    g_dispatchQSandCallCount = 0;
    zDEClient::DispatchFeatureEventTemplates(0, DispatchQSandCallback);
    const bool nullCraterOk = g_dispatchCraterCallCount == 0 && g_dispatchQSandCallCount == 1;

    g_zDEClient_FeatureListEnd = g_zDEClient_FeatureListBegin;
    g_dispatchQSandCallCount = 0;
    zDEClient::DispatchFeatureEventTemplates(DispatchCraterCallback, DispatchQSandCallback);
    const bool emptyOk = g_dispatchQSandCallCount == 0;

    g_zDEClient_FeatureListBegin = oldBegin;
    g_zDEClient_FeatureListEnd = oldEnd;
    g_zDEClient_FeatureListCapacityEnd = oldCapacityEnd;

    return callbacksOk && copiedEntryOk && nullCraterOk && emptyOk ? 0 : 1;
}

extern "C" int zdeclient_qsand_instance_event_maybe_relay_smoke(void) {
    zDEClient_QSandEventTemplate eventTemplate = {};
    eventTemplate.pointCount = 4;
    eventTemplate.radius = 2.0f;
    eventTemplate.center = {5.0f, 3.0f, -5.0f};

    g_qsandRelayCallCount = 0;
    g_qsandRelayResult = 0;
    g_zDEClientQSandNetRelayCallback = TestQSandRelayCallback;
    g_zDEClient_QuickSandEnabled = 1;
    zModel_Const::SetVertexMergeEpsilon(0.25f);
    if (zDEClient_QSand::InstanceEventMaybeRelay(&eventTemplate) != -1 ||
        g_qsandRelayCallCount != 1 || zModel_Const::GetVertexMergeEpsilon() != 0.25f) {
        g_zDEClientQSandNetRelayCallback = nullptr;
        g_zDEClient_QuickSandEnabled = 0;
        return 1;
    }

    g_qsandRelayCallCount = 0;
    g_qsandRelayResult = 1;
    g_zDEClient_QuickSandEnabled = 0;
    if (zDEClient_QSand::InstanceEventMaybeRelay(&eventTemplate) != -1 ||
        g_qsandRelayCallCount != 1 || zModel_Const::GetVertexMergeEpsilon() != 0.25f) {
        g_zDEClientQSandNetRelayCallback = nullptr;
        return 2;
    }

    g_qsandRelayCallCount = 0;
    g_zDEClientQSandNetRelayCallback = nullptr;
    g_zDEClient_QuickSandEnabled = 1;
    zClass_NodePartial cameraNode = {};
    g_zDEClient_CameraNode = &cameraNode;
    g_zDEClient_CameraNodeClassData = nullptr;
    zModel_MaterialPartial quickSandMaterial = {};
    zModel_MaterialPartial quickSandCycle = {};
    g_zDEClient_QuickSandMaterial = &quickSandMaterial;
    g_zDEClient_QuickSandMaterialCycle = &quickSandCycle;
    if (zDEClient_QSand::InstanceEventMaybeRelay(&eventTemplate) != -1 ||
        g_qsandRelayCallCount != 0 || zModel_Const::GetVertexMergeEpsilon() != 0.25f) {
        g_zDEClient_QuickSandEnabled = 0;
        g_zDEClient_QuickSandMaterial = nullptr;
        g_zDEClient_QuickSandMaterialCycle = nullptr;
        return 3;
    }

    g_zDEClient_QuickSandEnabled = 0;
    g_zDEClient_QuickSandMaterial = nullptr;
    g_zDEClient_QuickSandMaterialCycle = nullptr;
    g_zDEClient_CameraNode = nullptr;
    return 0;
}

extern "C" int zgeometry_clip_patch_output_apply_node_di_pairs_smoke(void) {
    zDiPartial diPool[2] = {};
    diPool[0].refCount = 1;
    diPool[1].mode = 0;
    g_zModel_DiPoolBase = diPool;
    g_zModel_DiPoolCapacity = 2;
    g_zModel_DiPoolInUseCount = 2;
    g_zModel_DiPoolFreeHeadIndex = -1;

    zClass_NodePartial node = {};
    node.userDataOrDiRef = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&diPool[0]));

    zGeometry_ClipPatchNodeDiPair *pairs =
        static_cast<zGeometry_ClipPatchNodeDiPair *>(std::malloc(sizeof(zGeometry_ClipPatchNodeDiPair)));
    if (pairs == nullptr) {
        return 1;
    }
    pairs[0].node = &node;
    pairs[0].di = &diPool[1];

    zDEClient_FeatureGridCell cell = {};
    zGeometry_ClipPatchPartitionOutput partition = {};
    partition.nodeDiPairCount = 1;
    partition.nodeDiPairs = pairs;
    partition.featureGridCell = &cell;

    zGeometry_ClipPatchOutputPartial output = {};
    output.partitionCount = 1;
    output.partitions = &partition;

    const int result = zGeometry_ClipPatchOutput::ApplyNodeDiPairs(&output);
    const bool ok =
        result == 0 && reinterpret_cast<zDiPartial *>(node.userDataOrDiRef) == &diPool[1] &&
        diPool[1].refCount == 1 && g_zModel_DiPoolFreeHeadIndex == 0 &&
        g_zModel_DiPoolInUseCount == 1 && cell.featureCount == 1 &&
        partition.nodeDiPairCount == 0 && partition.nodeDiPairs == nullptr;

    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;
    return ok ? 0 : 2;
}
