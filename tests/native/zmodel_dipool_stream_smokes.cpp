// Checked-in focused native smoke translation unit, formerly extracted from zgame_tests.cpp.
// Emits only the zModel DI-pool stream smokes needed by functional manifests.

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

extern "C" int zmodel_dipool_write_to_stream_smoke() {
    zVec3 verts[1] = {{1.0f, 2.0f, 3.0f}};
    zVec3 blendVerts[1] = {{4.0f, 5.0f, 6.0f}};
    zVec3 pointCamList[1] = {{7.0f, 8.0f, 9.0f}};
    zModel_PointEntryPartial pointEntries[1] = {};
    pointEntries[0].pointCamCount = 1;
    pointEntries[0].pointCamList = pointCamList;

    std::int32_t vertexIndices[3] = {0, 1, 2};
    std::int32_t normalIndices[3] = {2, 1, 0};
    zModel_Uv uvPairs[3] = {
        {0.0f, 0.25f},
        {0.5f, 0.75f},
        {1.0f, 1.25f},
    };

    zModel_MaterialSlot materialSlots[2] = {};
    materialSlots[1].material.flags = 0x0100;
    g_zModel_MatlPool = materialSlots;

    zDiEntryPartial entries[1] = {};
    entries[0].flagsAndIndexCount = 0x0203;
    entries[0].vertexIndices = vertexIndices;
    entries[0].normalIndices = normalIndices;
    entries[0].uvPairs = uvPairs;
    entries[0].material = &materialSlots[1].material;

    zDiPartial pool[2] = {};
    pool[0].entryCount = 1;
    pool[0].vertCount = 1;
    pool[0].blendVertCount = 1;
    pool[0].pointCount = 1;
    pool[0].entries = entries;
    pool[0].verts = verts;
    pool[0].pointEntries = pointEntries;
    pool[0].blendVerts = blendVerts;
    pool[0].nextFreeIndex = -1;
    pool[1].nextFreeIndex = 7;

    g_zModel_DiPoolBase = pool;
    g_zModel_DiPoolCapacity = 2;
    g_zModel_DiPoolInUseCount = 1;
    g_zModel_DiPoolFreeHeadIndex = 1;

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        return 1;
    }

    if (zModel_DiPool::WriteToStream(file) != 2) {
        std::fclose(file);
        return 2;
    }

    const long tableOffset = 12;
    const long tableBytes = static_cast<long>(sizeof(zDiPartial) * 2);
    const long dynamicOffset = tableOffset + tableBytes;
    const long expectedBytes =
        dynamicOffset + static_cast<long>(sizeof(zVec3)) + static_cast<long>(sizeof(zVec3)) +
        static_cast<long>(sizeof(zModel_PointEntryPartial)) + static_cast<long>(sizeof(zVec3)) +
        static_cast<long>(sizeof(zDiEntryPartial)) + static_cast<long>(sizeof(vertexIndices)) +
        static_cast<long>(sizeof(normalIndices)) + static_cast<long>(sizeof(uvPairs));

    if (std::fseek(file, 0, SEEK_END) != 0 || std::ftell(file) != expectedBytes ||
        pool[0].nextFreeIndex != dynamicOffset) {
        std::fclose(file);
        return 3;
    }

    std::int32_t header[3] = {};
    zDiPartial writtenPool[2] = {};
    std::rewind(file);
    if (std::fread(header, sizeof(header), 1, file) != 1 || header[0] != 2 || header[1] != 1 ||
        header[2] != 1 || std::fread(writtenPool, sizeof(writtenPool), 1, file) != 1 ||
        writtenPool[0].nextFreeIndex != dynamicOffset || writtenPool[1].nextFreeIndex != 7) {
        std::fclose(file);
        return 4;
    }

    zVec3 writtenVerts[2] = {};
    zModel_PointEntryPartial writtenPoint = {};
    zVec3 writtenPointCam = {};
    zDiEntryPartial writtenEntry = {};
    std::int32_t writtenVertexIndices[3] = {};
    std::int32_t writtenNormalIndices[3] = {};
    zModel_Uv writtenUvs[3] = {};
    if (std::fread(&writtenVerts[0], sizeof(zVec3), 1, file) != 1 ||
        std::fread(&writtenVerts[1], sizeof(zVec3), 1, file) != 1 ||
        std::fread(&writtenPoint, sizeof(writtenPoint), 1, file) != 1 ||
        std::fread(&writtenPointCam, sizeof(writtenPointCam), 1, file) != 1 ||
        std::fread(&writtenEntry, sizeof(writtenEntry), 1, file) != 1 ||
        std::fread(writtenVertexIndices, sizeof(writtenVertexIndices), 1, file) != 1 ||
        std::fread(writtenNormalIndices, sizeof(writtenNormalIndices), 1, file) != 1 ||
        std::fread(writtenUvs, sizeof(writtenUvs), 1, file) != 1) {
        std::fclose(file);
        return 5;
    }

    const bool ok =
        writtenVerts[0].x == 1.0f && writtenVerts[1].x == 4.0f && writtenPoint.pointCamCount == 1 &&
        writtenPointCam.z == 9.0f && reinterpret_cast<std::intptr_t>(writtenEntry.material) == 1 &&
        writtenVertexIndices[2] == 2 && writtenNormalIndices[0] == 2 && writtenUvs[2].v == 1.25f;

    std::fclose(file);
    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;
    g_zModel_MatlPool = nullptr;
    return ok ? 0 : 6;
}

extern "C" int zmodel_dipool_read_from_stream_smoke() {
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);

    zModel_MaterialSlot materialSlots[1] = {};
    materialSlots[0].material.flags = 0x0100;
    g_zModel_MatlPool = materialSlots;
    g_zModel_MatlPoolCapacity = 1;

    zDiPartial serializedDi = {};
    serializedDi.entryCount = 1;
    serializedDi.vertCount = 1;
    serializedDi.normalCount = 1;
    serializedDi.blendVertCount = 1;
    serializedDi.pointCount = 1;
    serializedDi.nextFreeIndex = 12;

    zVec3 verts[1] = {{1.0f, 2.0f, 3.0f}};
    zVec3 normals[1] = {{4.0f, 5.0f, 6.0f}};
    zVec3 blendVerts[1] = {{7.0f, 8.0f, 9.0f}};
    zModel_PointEntryPartial pointEntry = {};
    pointEntry.pointCamCount = 1;
    pointEntry.colorB = 255.0f;
    pointEntry.colorG = 0.0f;
    pointEntry.colorR = 0.0f;
    pointEntry.packedColor16 = static_cast<std::int32_t>(0xabcd0000u);
    zVec3 pointCam[1] = {{10.0f, 11.0f, 12.0f}};

    zDiEntryPartial diEntry = {};
    diEntry.flagsAndIndexCount = 0x0203;
    diEntry.material = reinterpret_cast<zModel_MaterialPartial *>(static_cast<std::intptr_t>(0));
    std::int32_t vertexIndices[3] = {0, 1, 2};
    std::int32_t normalIndices[3] = {2, 1, 0};
    zModel_Uv uvs[3] = {
        {0.0f, 0.25f},
        {0.5f, 0.75f},
        {1.0f, 1.25f},
    };

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        return 1;
    }

    std::int32_t header[3] = {1, 1, -1};
    if (std::fwrite(header, sizeof(header), 1, file) != 1 ||
        std::fwrite(&serializedDi, sizeof(serializedDi), 1, file) != 1 ||
        std::fwrite(verts, sizeof(verts), 1, file) != 1 ||
        std::fwrite(normals, sizeof(normals), 1, file) != 1 ||
        std::fwrite(blendVerts, sizeof(blendVerts), 1, file) != 1 ||
        std::fwrite(&pointEntry, sizeof(pointEntry), 1, file) != 1 ||
        std::fwrite(pointCam, sizeof(pointCam), 1, file) != 1 ||
        std::fwrite(&diEntry, sizeof(diEntry), 1, file) != 1 ||
        std::fwrite(vertexIndices, sizeof(vertexIndices), 1, file) != 1 ||
        std::fwrite(normalIndices, sizeof(normalIndices), 1, file) != 1 ||
        std::fwrite(uvs, sizeof(uvs), 1, file) != 1) {
        std::fclose(file);
        return 2;
    }

    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;

    std::rewind(file);
    if (zModel_DiPool::ReadFromStream(file) != 1) {
        std::fclose(file);
        std::free(g_zModel_DiPoolBase);
        g_zModel_DiPoolBase = nullptr;
        g_zModel_DiPoolCapacity = 0;
        g_zModel_MatlPool = nullptr;
        g_zModel_MatlPoolCapacity = 0;
        return 3;
    }
    std::fclose(file);

    zDiPartial *const loadedDi = g_zModel_DiPoolBase;
    zDiEntryPartial *const loadedEntry = loadedDi->entries;
    zModel_PointEntryPartial *const loadedPoint = loadedDi->pointEntries;
    const bool ok =
        g_zModel_DiPoolCapacity == 1 && g_zModel_DiPoolInUseCount == 1 &&
        g_zModel_DiPoolFreeHeadIndex == -1 && loadedDi->verts != nullptr &&
        loadedDi->verts[0].z == 3.0f && loadedDi->normals != nullptr &&
        loadedDi->normals[0].x == 4.0f && loadedDi->blendVerts != nullptr &&
        loadedDi->blendVerts[0].y == 8.0f && loadedPoint != nullptr &&
        loadedPoint[0].pointCamList != nullptr && loadedPoint[0].pointCamList[0].z == 12.0f &&
        loadedPoint[0].packedColor16 == static_cast<std::int32_t>(0xabcdf800u) &&
        loadedEntry != nullptr && loadedEntry[0].material == &materialSlots[0].material &&
        static_cast<std::int32_t *>(loadedEntry[0].vertexIndices)[2] == 2 &&
        static_cast<std::int32_t *>(loadedEntry[0].normalIndices)[0] == 2 &&
        static_cast<zModel_Uv *>(loadedEntry[0].uvPairs)[2].v == 1.25f;

    std::free(loadedEntry[0].uvPairs);
    std::free(loadedEntry[0].normalIndices);
    std::free(loadedEntry[0].vertexIndices);
    std::free(loadedDi->entries);
    std::free(loadedPoint[0].pointCamList);
    std::free(loadedDi->pointEntries);
    std::free(loadedDi->blendVerts);
    std::free(loadedDi->normals);
    std::free(loadedDi->verts);
    std::free(g_zModel_DiPoolBase);
    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;
    g_zModel_MatlPool = nullptr;
    g_zModel_MatlPoolCapacity = 0;

    return ok ? 0 : 4;
}

extern "C" int zmodel_dipool_read_entry_by_index_from_stream_smoke() {
    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        return 1;
    }

    const std::int32_t dynamicOffset = 12 + 2 * static_cast<std::int32_t>(sizeof(zDiPartial));
    std::int32_t header[3] = {2, 1, 0};
    zDiPartial serializedSlots[2] = {};
    serializedSlots[0].mode = 11;
    serializedSlots[0].nextFreeIndex = dynamicOffset;
    serializedSlots[1].mode = 22;
    serializedSlots[1].flags = 0x44;
    serializedSlots[1].bboxRadius = 3.5f;
    serializedSlots[1].nextFreeIndex = dynamicOffset;

    if (std::fwrite(header, sizeof(header), 1, file) != 1 ||
        std::fwrite(serializedSlots, sizeof(serializedSlots), 1, file) != 1) {
        std::fclose(file);
        return 2;
    }

    zDiPartial livePool[2] = {};
    livePool[1].nextFreeIndex = 0;
    g_zModel_DiPoolBase = livePool;
    g_zModel_DiPoolCapacity = 2;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = 1;

    std::rewind(file);
    zDiPartial *const loaded = zModel_DiPool::ReadEntryByIndexFromStream(file, 1);
    const bool loadedOk = loaded == &livePool[1] && livePool[1].mode == 22 &&
                          livePool[1].flags == 0x44 && livePool[1].bboxRadius == 3.5f &&
                          livePool[1].nextFreeIndex == 0 && g_zModel_DiPoolInUseCount == 1 &&
                          g_zModel_DiPoolFreeHeadIndex == 0;

    std::rewind(file);
    zDiPartial *const outOfRange = zModel_DiPool::ReadEntryByIndexFromStream(file, 2);
    const bool rangeOk = outOfRange == nullptr && g_zModel_DiPoolInUseCount == 1 &&
                         g_zModel_DiPoolFreeHeadIndex == 0;

    std::fclose(file);
    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;

    return loadedOk && rangeOk ? 0 : 3;
}
