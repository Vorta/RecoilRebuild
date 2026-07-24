// Checked-in focused native smoke translation unit, formerly extracted from zgame_tests.cpp.
// Emits only the HudSensorTracker objective smokes needed by functional manifests.

#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/hud.h"
#include "Battlesport/player.h"
#include "GameZRecoil/zUtil/zutil.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zGeometry/zgeo.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "opt_catalog.h"
#include "zclass.h"
#include "zclip_alt.h"
#include "zdi.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <io.h>
#include <limits>


namespace {
int g_damageMaskUploadLockCount = 0;
int g_damageMaskUploadUnlockCount = 0;
int g_damageMaskUploadFinalizeCount = 0;
unsigned short *g_damageMaskUploadPixels = nullptr;
int g_damageMaskUploadPitchBytes = 0;
zVideo_TextureRecordPartial *g_damageMaskLastTextureRecord = nullptr;
int g_zgameFogColorUpdateCount = 0;
int g_modelRefLerpCallbackCount = 0;
void *g_modelRefLerpLastCallbackCtx = nullptr;
int g_zclassUpdateBucketCallbackCount = 0;
zClass_NodePartial *g_zclassUpdateBucketLastNode = nullptr;
int g_zclassUpdateBucketDeferredDuringCallback = -1;
int g_zclassRenderTraverseCallCount = 0;
zClass_NodePartial *g_zclassRenderTraverseNodes[16] = {};
int g_zclassRenderTraverseClipMasks[16] = {};
float g_zclassRenderTraverseAlphaScales[16] = {};
int g_zclassRenderTraverseVertexAlphaEnabled[16] = {};

void TestZGameUpdateFogColor(void) {
    ++g_zgameFogColorUpdateCount;
}

void __fastcall TestModelRefLerpCallback(void *callbackCtx) {
    ++g_modelRefLerpCallbackCount;
    g_modelRefLerpLastCallbackCtx = callbackCtx;
}

int __fastcall TestZClassUpdateBucketCallback(zClass_NodePartial *node) {
    ++g_zclassUpdateBucketCallbackCount;
    g_zclassUpdateBucketLastNode = node;
    g_zclassUpdateBucketDeferredDuringCallback = g_zClass_DeferredProcessingEnabled;
    return 0;
}

void __fastcall TestZClassRenderTraverseCallback(zClass_NodePartial *node, int clipMask) {
    if (g_zclassRenderTraverseCallCount < 16) {
        g_zclassRenderTraverseNodes[g_zclassRenderTraverseCallCount] = node;
        g_zclassRenderTraverseClipMasks[g_zclassRenderTraverseCallCount] = clipMask;
        g_zclassRenderTraverseAlphaScales[g_zclassRenderTraverseCallCount] =
            gModel_RenderAlphaScaleCurrent;
        g_zclassRenderTraverseVertexAlphaEnabled[g_zclassRenderTraverseCallCount] =
            gModel_RenderVertexAlphaEnabled;
    }
    ++g_zclassRenderTraverseCallCount;
}

int g_zmodelReleaseTextureUploadCount = 0;
zVideo_TextureRecordPartial *g_zmodelReleaseTextureUploadLast = nullptr;

void __fastcall TestReleaseTextureUploadSurfaceRef(
    zVideo_TextureRecordPartial *textureRecord) {
    ++g_zmodelReleaseTextureUploadCount;
    g_zmodelReleaseTextureUploadLast = textureRecord;
}

int __fastcall TextureMemoryQueryMissingStub(int, int *, int *) {
    return 0;
}

int __fastcall DamageMaskLockUploadStub(zVideo_TextureRecordPartial *textureRecord,
                                             void **outPixels, int *outPitchBytes) {
    ++g_damageMaskUploadLockCount;
    g_damageMaskLastTextureRecord = textureRecord;
    *outPixels = g_damageMaskUploadPixels;
    *outPitchBytes = g_damageMaskUploadPitchBytes;
    return 1;
}

void __fastcall DamageMaskUnlockUploadStub(zVideo_TextureRecordPartial *textureRecord) {
    ++g_damageMaskUploadUnlockCount;
    g_damageMaskLastTextureRecord = textureRecord;
}

void __fastcall DamageMaskFinalizeUploadStub(zVideo_TextureRecordPartial *textureRecord,
                                                  void *, void *) {
    ++g_damageMaskUploadFinalizeCount;
    g_damageMaskLastTextureRecord = textureRecord;
}

void WriteU32(HANDLE file, std::uint32_t value) {
    DWORD written = 0;
    WriteFile(file, &value, sizeof(value), &written, nullptr);
}

void WriteBytes(HANDLE file, const char *text, std::uint32_t length) {
    DWORD written = 0;
    WriteFile(file, text, length, &written, nullptr);
}

void WriteZrdStringNode(HANDLE file, const char *text) {
    WriteU32(file, zReader::ZRDR_NODE_STRING);
    WriteU32(file, static_cast<std::uint32_t>(std::strlen(text)));
    WriteBytes(file, text, static_cast<std::uint32_t>(std::strlen(text)));
}

void WriteZrdIntNode(HANDLE file, std::int32_t value) {
    WriteU32(file, zReader::ZRDR_NODE_INT);
    WriteU32(file, static_cast<std::uint32_t>(value));
}

void WriteZrdFloatNode(HANDLE file, float value) {
    DWORD written = 0;
    WriteU32(file, zReader::ZRDR_NODE_FLOAT);
    WriteFile(file, &value, sizeof(value), &written, nullptr);
}

void WriteZrdNamedStringNode(HANDLE file, const char *name, const char *value) {
    WriteZrdStringNode(file, name);
    WriteZrdStringNode(file, value);
}

void WriteZrdNamedFloatNode(HANDLE file, const char *name, float value) {
    WriteZrdStringNode(file, name);
    WriteZrdFloatNode(file, value);
}

void WriteZrdNamedIntArray(HANDLE file, const char *name, std::int32_t value) {
    WriteZrdStringNode(file, name);
    WriteU32(file, zReader::ZRDR_NODE_ARRAY);
    WriteU32(file, 2);
    WriteZrdIntNode(file, value);
}

void WriteZrdNamedDirectInt(HANDLE file, const char *name, std::int32_t value) {
    WriteZrdStringNode(file, name);
    WriteZrdIntNode(file, value);
}

void WriteZrdNamedColorArray(HANDLE file, const char *name, std::int32_t red,
                             std::int32_t green, std::int32_t blue) {
    WriteZrdStringNode(file, name);
    WriteU32(file, zReader::ZRDR_NODE_ARRAY);
    WriteU32(file, 4);
    WriteZrdIntNode(file, red);
    WriteZrdIntNode(file, green);
    WriteZrdIntNode(file, blue);
}

void WriteZrdNamedFloatPairArray(HANDLE file, const char *name, float first, float second) {
    WriteZrdStringNode(file, name);
    WriteU32(file, zReader::ZRDR_NODE_ARRAY);
    WriteU32(file, 3);
    WriteZrdFloatNode(file, first);
    WriteZrdFloatNode(file, second);
}

void FreeHudWeatherFxForTest(HudUiElement *element) {
    if (element == nullptr) {
        return;
    }

    HudWeatherFx *const weatherFx = static_cast<HudWeatherFx *>(element);
    if (weatherFx->softwareImage != nullptr) {
        char *const alphaMap = weatherFx->softwareImage->alphaMap;
        zVid_Image::Destroy(weatherFx->softwareImage);
        if (alphaMap != nullptr) {
            std::free(alphaMap);
        }
    }
    ::operator delete(weatherFx->particleQuads);
    ::operator delete(weatherFx->particlePositions[0]);
    ::operator delete(weatherFx->particlePositions[1]);
    ::operator delete(weatherFx);
}

bool EnterSupportDirectoryForRetailZbdTest(char *oldDir, DWORD oldDirSize) {
    if (GetCurrentDirectoryA(oldDirSize, oldDir) == 0) {
        return false;
    }

    const char *candidates[] = {
        "support",
        "..\\..\\..\\..\\support",
    };
    for (int i = 0; i < static_cast<int>(sizeof(candidates) / sizeof(candidates[0])); ++i) {
        const DWORD attributes = GetFileAttributesA(candidates[i]);
        if (attributes != INVALID_FILE_ATTRIBUTES &&
            (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
            SetCurrentDirectoryA(candidates[i]) != 0) {
            return true;
        }
    }

    return false;
}

bool ExistingNonEmptyFileForTest(const char *path) {
    WIN32_FILE_ATTRIBUTE_DATA data = {};
    if (GetFileAttributesExA(path, GetFileExInfoStandard, &data) == 0) {
        return false;
    }
    return (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 &&
        (data.nFileSizeHigh != 0 || data.nFileSizeLow != 0);
}

bool RetailMissionScriptFixturesAvailableForTest(void) {
    const char *const requiredFiles[] = {
        "support\\initm1.gw",
        "support\\initm3.gw",
        "support\\commonm1.gw",
        "support\\loadm1.gw",
        "support\\tex_fxm1.gw",
        "support\\commonm3.gw",
        "support\\loadm3.gw",
        "support\\tex_fxm3.gw",
        "m1.gs",
        "m1_zbd.gs",
        "m3.gs",
        "m3_zbd.gs",
    };
    for (int index = 0;
         index < static_cast<int>(sizeof(requiredFiles) / sizeof(requiredFiles[0]));
         ++index) {
        if (!ExistingNonEmptyFileForTest(requiredFiles[index])) {
            return false;
        }
    }
    return true;
}

bool EnterRetailSupportScriptsDirectoryForTest(char *oldDir, DWORD oldDirSize) {
    if (GetCurrentDirectoryA(oldDirSize, oldDir) == 0) {
        return false;
    }

    const char *candidates[] = {
        "support\\scripts",
        "..\\support\\scripts",
        "..\\..\\support\\scripts",
        "..\\..\\..\\support\\scripts",
        "..\\..\\..\\..\\support\\scripts",
    };
    for (int i = 0; i < static_cast<int>(sizeof(candidates) / sizeof(candidates[0])); ++i) {
        const DWORD attributes = GetFileAttributesA(candidates[i]);
        if (attributes != INVALID_FILE_ATTRIBUTES &&
            (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
            SetCurrentDirectoryA(candidates[i]) != 0) {
            if (RetailMissionScriptFixturesAvailableForTest()) {
                return true;
            }
            if (SetCurrentDirectoryA(oldDir) == 0) {
                return false;
            }
        }
    }

    return false;
}

std::int32_t FloatBitsForTest(float value) {
    std::int32_t raw = 0;
    std::memcpy(&raw, &value, sizeof(raw));
    return raw;
}

std::int32_t __fastcall zclass_test_node_type_0x42(zClass_NodePartial *node) {
    return node->nodeType == 0x42 ? 1 : 0;
}

float __fastcall zclass_damage_timer_stub(void *context, float damageAmount) {
    *static_cast<float *>(context) = damageAmount;
    return damageAmount + 1.0f;
}

zProjectedPoint g_drawPointLastPoint{};
std::uint32_t g_drawPointLastColor = 0;
std::int32_t g_drawPointLastCount = 0;
std::int32_t g_drawPointCallCount = 0;
std::int32_t g_submitFlatCallCount = 0;
std::int32_t g_submitFlatLastVertexCount = 0;
std::int32_t g_submitFlatLastAlpha = 0;
std::uint32_t g_submitFlatLastColor = 0;
std::uint32_t g_submitFlatLastRenderParam = 0;
std::int32_t g_submitFlatLastQueueMode = 0;
zVideo_XyzVertex g_submitFlatLastVerts[0x40]{};

void __fastcall zmodel_draw_point_color16_stub(zVideo_XyzVertex *point,
                                                    std::uint32_t packedColor16,
                                                    std::int32_t pointCount) {
    g_drawPointLastPoint = *(zProjectedPoint *)(point);
    g_drawPointLastColor = packedColor16;
    g_drawPointLastCount = pointCount;
    ++g_drawPointCallCount;
}

void __fastcall zmodel_submit_poly_flat_stub(zVideo_XyzVertex *vertices,
                                                  std::uint32_t packedColor16,
                                                  std::int32_t alpha,
                                                  std::int32_t renderParam,
                                                  std::int32_t vertexCount,
                                                  std::int32_t queueMode) {
    ++g_submitFlatCallCount;
    g_submitFlatLastVertexCount = vertexCount;
    g_submitFlatLastAlpha = alpha;
    g_submitFlatLastColor = packedColor16;
    g_submitFlatLastRenderParam = static_cast<std::uint32_t>(renderParam);
    g_submitFlatLastQueueMode = queueMode;
    for (int i = 0; i < vertexCount && i < 0x40; ++i) {
        g_submitFlatLastVerts[i] = vertices[i];
    }
}

void reset_zclass_type_lists_for_test() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;
}

void free_zclass_type_lists_for_test() {
    for (int i = 0; i < 16; ++i) {
        for (zClass_TypeListLink *link = zClass_TypeList::Head(i); link != nullptr;) {
            zClass_TypeListLink *const next = link->next;
            std::free(link);
            link = next;
        }
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }

    for (zClass_TypeListLink *link = g_zClass_TypeList_FreeLinkHead; link != nullptr;) {
        zClass_TypeListLink *const next = link->next;
        std::free(link);
        link = next;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;
}

bool zclass_bucket_has_pending_node_for_test(int bucket, zClass_NodePartial *node) {
    for (zClass_TypeListLink *link = zClass_TypeList::Head(bucket); link != nullptr;
         link = link->next) {
        if (link->node == node && link->pendingRemove != 0) {
            return true;
        }
    }

    return false;
}
} // namespace

extern "C" int hud_sensor_objective_slot_reset_smoke() {
    HudSensorObjectiveSlot slot{};
    slot.completedFlag = 7;
    slot.objectiveTitle[0] = 'T';
    slot.objectiveDesc[0] = 'D';
    slot.objectiveSummary[0] = 'S';

    slot.Reset();

    return slot.completedFlag == 0 && slot.objectiveImage == nullptr &&
                   slot.objectiveTitle[0] == '\0' && slot.objectiveDesc[0] == '\0' &&
                   slot.objectiveSummary[0] == '\0'
               ? 0
               : 1;
}

extern "C" int hud_sensor_tracker_unload_objectives_smoke() {
    std::int32_t networkEnabled = 0;
    g_zGame_Options_PointerCache.networkEnabled = &networkEnabled;

    HudSensorTracker tracker{};
    tracker.objectiveCount = 2;
    tracker.currentObjectiveIndex = 4;
    tracker.firstIncompleteObjectiveIndex = 5;
    tracker.completedObjectiveCount = 6;
    tracker.objectiveSlots[0].completedFlag = 1;
    tracker.objectiveSlots[0].objectiveTitle[0] = 'A';
    tracker.objectiveSlots[1].completedFlag = 1;
    tracker.objectiveSlots[1].objectiveTitle[0] = 'B';

    const bool disabledResult =
        tracker.UnloadObjectives() == 1 && tracker.currentObjectiveIndex == -1 &&
        tracker.firstIncompleteObjectiveIndex == 0 && tracker.objectiveCount == 0 &&
        tracker.completedObjectiveCount == 0 && tracker.objectiveSlots[0].completedFlag == 0 &&
        tracker.objectiveSlots[0].objectiveTitle[0] == '\0' &&
        tracker.objectiveSlots[1].completedFlag == 0 &&
        tracker.objectiveSlots[1].objectiveTitle[0] == '\0';

    networkEnabled = 1;
    tracker.objectiveCount = 1;
    tracker.currentObjectiveIndex = 8;
    tracker.completedObjectiveCount = 9;
    tracker.objectiveSlots[0].completedFlag = 3;
    tracker.objectiveSlots[0].objectiveTitle[0] = 'N';

    const bool enabledResult = tracker.UnloadObjectives() == 1 &&
                               tracker.currentObjectiveIndex == 8 && tracker.objectiveCount == 1 &&
                               tracker.completedObjectiveCount == 9 &&
                               tracker.objectiveSlots[0].completedFlag == 3 &&
                               tracker.objectiveSlots[0].objectiveTitle[0] == 'N';

    g_zGame_Options_PointerCache.networkEnabled = nullptr;

    return disabledResult && enabledResult ? 0 : 1;
}

extern "C" int hud_sensor_tracker_load_objectives_from_path_smoke() {
    char tempDir[MAX_PATH] = {};
    char tempPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "obj", 0, tempPath) == 0) {
        return 1;
    }

    HANDLE file = CreateFileA(tempPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        DeleteFileA(tempPath);
        return 2;
    }

    WriteU32(file, zReader::ZRDR_NODE_ARRAY);
    WriteU32(file, 7);
    WriteZrdNamedIntArray(file, "READ_TIME", 9);
    WriteZrdNamedIntArray(file, "REVIEW_DELAY", 6);
    WriteZrdNamedIntArray(file, "FINAL_MISSION", 7);
    FlushFileBuffers(file);

    zZarFileRecord record = {};
    record.fileOffset = 0;
    record.fileSize = SetFilePointer(file, 0, nullptr, FILE_CURRENT);
    std::strcpy(record.name, "objectives.zrd");

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
    zArchiveList *const oldMissionSearchPathList = g_zImage_MissionSearchPathList;
    int *const oldNetwork = g_zGame_Options_PointerCache.networkEnabled;
    g_zArchive_MountedList = &list;
    g_zImage_MissionSearchPathList = nullptr;

    std::int32_t networkEnabled = 0;
    g_zGame_Options_PointerCache.networkEnabled = &networkEnabled;

    HudSensorTracker tracker = {};
    tracker.missionId = 12;
    const int loaded = tracker.LoadObjectivesFromPath("objectives.zrd");
    const bool disabledOk =
        loaded == 0 && tracker.objectivesRootNode != nullptr &&
        tracker.objectiveReadTimeSecRaw == FloatBitsForTest(9.0f) &&
        tracker.objectiveReviewDelaySecRaw == FloatBitsForTest(6.0f) &&
        tracker.objectiveReadSoundDelaySecRaw == FloatBitsForTest(2.0f) &&
        tracker.finalMissionFlag == 7 && tracker.currentObjectiveIndex == -1 &&
        tracker.firstIncompleteObjectiveIndex == 0 && tracker.objectiveCount == 1 &&
        tracker.completedObjectiveCount == 0;

    if (tracker.objectivesRootNode != nullptr) {
        zReader::FreeLoadedTree(tracker.objectivesRootNode);
        tracker.objectivesRootNode = nullptr;
    }

    networkEnabled = 1;
    HudSensorTracker networkTracker = {};
    networkTracker.missionId = 12;
    networkTracker.finalMissionFlag = 99;
    const int networkLoaded = networkTracker.LoadObjectivesFromPath("objectives.zrd");
    const bool networkOk = networkLoaded == 0 && networkTracker.objectivesRootNode != nullptr &&
                           networkTracker.finalMissionFlag == 99 &&
                           networkTracker.objectiveCount == 0;

    if (networkTracker.objectivesRootNode != nullptr) {
        zReader::FreeLoadedTree(networkTracker.objectivesRootNode);
        networkTracker.objectivesRootNode = nullptr;
    }

    if (g_zImage_MissionSearchPathList != nullptr) {
        zUtil_ZRDR_FreeSearchPathList(g_zImage_MissionSearchPathList);
    }

    g_zArchive_MountedList = oldMountedList;
    g_zImage_MissionSearchPathList = oldMissionSearchPathList;
    g_zGame_Options_PointerCache.networkEnabled = oldNetwork;
    CloseHandle(file);
    DeleteFileA(tempPath);
    return disabledOk && networkOk ? 0 : 3;
}

extern "C" int hud_sensor_tracker_load_objectives_from_zrd_smoke() {
    zReader::Node reviewSound[2] = {};
    reviewSound[0].type = zReader::ZRDR_NODE_INT;
    reviewSound[0].value.i32 = 2;
    reviewSound[1].type = zReader::ZRDR_NODE_STRING;
    reviewSound[1].value.str = const_cast<char *>("snd_review");

    zReader::Node readSound[3] = {};
    readSound[0].type = zReader::ZRDR_NODE_INT;
    readSound[0].value.i32 = 3;
    readSound[1].type = zReader::ZRDR_NODE_STRING;
    readSound[1].value.str = const_cast<char *>("snd_read");
    readSound[2].type = zReader::ZRDR_NODE_INT;
    readSound[2].value.i32 = FloatBitsForTest(3.5f);

    zReader::Node objective1[3] = {};
    objective1[0].type = zReader::ZRDR_NODE_INT;
    objective1[0].value.i32 = 3;
    objective1[1].type = zReader::ZRDR_NODE_STRING;
    objective1[1].value.str = const_cast<char *>("READ_SOUND");
    objective1[2].type = zReader::ZRDR_NODE_ARRAY;
    objective1[2].value.nodes = readSound;

    zReader::Node objectiveSound[2] = {};
    objectiveSound[0].type = zReader::ZRDR_NODE_INT;
    objectiveSound[0].value.i32 = 2;
    objectiveSound[1].type = zReader::ZRDR_NODE_STRING;
    objectiveSound[1].value.str = const_cast<char *>("snd_complete");

    zReader::Node rootNodes[7] = {};
    rootNodes[0].type = zReader::ZRDR_NODE_INT;
    rootNodes[0].value.i32 = 7;
    rootNodes[1].type = zReader::ZRDR_NODE_STRING;
    rootNodes[1].value.str = const_cast<char *>("REVIEW_SOUND");
    rootNodes[2].type = zReader::ZRDR_NODE_ARRAY;
    rootNodes[2].value.nodes = reviewSound;
    rootNodes[3].type = zReader::ZRDR_NODE_STRING;
    rootNodes[3].value.str = const_cast<char *>("OBJECTIVE1");
    rootNodes[4].type = zReader::ZRDR_NODE_ARRAY;
    rootNodes[4].value.nodes = objective1;
    rootNodes[5].type = zReader::ZRDR_NODE_STRING;
    rootNodes[5].value.str = const_cast<char *>("OBJECTIVE_SOUND");
    rootNodes[6].type = zReader::ZRDR_NODE_ARRAY;
    rootNodes[6].value.nodes = objectiveSound;

    zReader::Node root = {};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = rootNodes;

    zSndSample samples[4] = {};
    samples[0].replayFields.sampleId = "snd_review";
    samples[0].primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(0x1000);
    samples[1].replayFields.sampleId = "snd_read";
    samples[1].primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(0x1004);
    samples[2].replayFields.sampleId = "snd_complete";
    samples[2].primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(0x1008);
    samples[3].replayFields.sampleId = "snd_incoming";
    samples[3].primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(0x100c);

    zSndSampleSet sampleSet = {};
    sampleSet.sampleCount = 4;
    sampleSet.samples = samples;
    const zSndSampleSetRegistry oldRegistry = g_zSnd_SampleSetRegistry;
    const int oldSndInitialized = g_zSnd_IsInitialized;
    const int oldActiveBackend = g_zSnd_ActiveBackend;
    int *const oldNetwork = g_zGame_Options_PointerCache.networkEnabled;

    g_zSnd_SampleSetRegistry.clear();
    g_zSnd_SampleSetRegistry.push_back(&sampleSet);
    g_zSnd_IsInitialized = 1;
    g_zSnd_ActiveBackend = 0;

    std::int32_t networkEnabled = 0;
    g_zGame_Options_PointerCache.networkEnabled = &networkEnabled;

    HudSensorTracker tracker = {};
    tracker.objectivesRootNode = &root;
    tracker.objectiveCount = 2;
    tracker.objectiveSlots[0].completedFlag = 1;
    tracker.objectiveSlots[1].completedFlag = 0;
    const int loaded = tracker.LoadObjectivesFromZrd();
    const bool loadedOk =
        loaded == 0 &&
        tracker.objectiveReviewSfx == &samples[0] &&
        tracker.objectiveSlots[0].readSoundSample == &samples[1] &&
        samples[1].playbackEventHandler == HudSensorTracker::OnObjectiveReadSoundEvent &&
        tracker.objectiveReadSoundDelaySecRaw == FloatBitsForTest(3.5f) &&
        tracker.objectiveCompleteSfx == &samples[2] &&
        tracker.objectiveIncomingSfx == &samples[3] &&
        tracker.firstIncompleteObjectiveIndex == 1;

    networkEnabled = 1;
    HudSensorTracker networkTracker = {};
    networkTracker.objectivesRootNode = &root;
    networkTracker.firstIncompleteObjectiveIndex = 9;
    const int networkLoaded = networkTracker.LoadObjectivesFromZrd();
    const bool networkOk =
        networkLoaded == 0 &&
        networkTracker.objectiveReviewSfx == &samples[0] &&
        networkTracker.objectiveCompleteSfx == nullptr &&
        networkTracker.objectiveIncomingSfx == nullptr &&
        networkTracker.firstIncompleteObjectiveIndex == 9;

    g_zSnd_SampleSetRegistry = oldRegistry;
    g_zSnd_IsInitialized = oldSndInitialized;
    g_zSnd_ActiveBackend = oldActiveBackend;
    g_zGame_Options_PointerCache.networkEnabled = oldNetwork;

    return loadedOk && networkOk ? 0 : 1;
}
