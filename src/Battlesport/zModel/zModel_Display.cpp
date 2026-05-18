#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/include/zClipRect.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include "recoil/recoil_types.h"
#include <stdlib.h>
#include <string.h>

int g_zModel_DisplayInitFlagA = 0;
int g_zModel_DisplayInitFlagB = 0;
int g_zModel_DisplayInitFlagC = 0;
int g_zModel_DisplayClipMode = 0;
int g_zModel_DisplayClipX = 0;
int g_zModel_DisplayClipY = 0;
float g_zModel_DisplayClipWidth = 0.0f;
float g_zModel_DisplayClipHeight = 0.0f;
float g_zModel_DisplayClipMaxX = 0.0f;
float g_zModel_DisplayClipMaxY = 0.0f;
float g_zModel_DisplayParam4 = 0.0f;
float g_zModel_DisplayParam40 = 0.0f;
int g_zModel_DisplayClipReserved = 0;
void *g_zModel_SpanOcclusionProc = 0;
void *g_zModel_DisplayScratchPtr = 0;
int g_zModel_DisplayScratchCount = 0;
float g_zModel_ViewScaleX = 0.0f;
int g_zModel_ViewScaleYRaw = 0;
float g_zModel_ViewScaleZ = 0.0f;
float g_zModel_FogStart = 0.0f;
float g_zModel_FogEnd = 0.0f;
float g_zModel_FogHeightHigh = 0.0f;
float g_zModel_FogHeightLow = 0.0f;
float g_zModel_FogDistanceInvRange = 0.0f;
float g_zModel_FogHeightInvRange = 0.0f;
float g_zModel_FogDensity = 0.0f;
int g_zModel_FogReserved = 0;
float g_zModel_FogScale = 0.0f;
float g_zModel_InverseZTolerance = 0.0f;
float g_zModel_HardwareInverseZTolerance = 0.0f;
zVec3 g_zModel_SharedVec3ScratchAStorage[0x400] = {0};
zVec3 g_zModel_SharedVec3ScratchBStorage[0x400] = {0};
zVec3 *g_zModel_TransformedVerts = 0;
zVec3 *g_zModel_TransformedNormals = 0;
zVec3 *g_zModel_SharedVec3ScratchA = 0;
zVec3 *g_zModel_SharedVec3ScratchB = 0;
zVec3 *g_zModel_PointInPolygonVertices = 0;
zVec3 *g_zModel_PointInPolygonEdgeNormals = 0;
int g_zModel_PointInPolygonVertexCount = 0;
int g_zModel_ScratchCounters[8] = {0};
float g_zModel_PointInPolyTolX = 0.0f;
float g_zModel_PointInPolyTolY = 0.0f;
unsigned char g_zModel_DamageMaskStorage[0x200] = {0};
void *g_zModel_DamageMaskCurrent = 0;
void *g_OptCatalogDamageMaskHandles[3] = {0};
float g_OptCatalogDamageMaskPhaseU = 0.0f;
float g_OptCatalogDamageMaskPhaseV = 0.0f;
int g_OptCatalogDamageMaskEnabled = 0;
int g_zModel_OptCatalogSlot = 0;
int g_zModel_OptCatalogAux0 = 0;
int g_zModel_OptCatalogAux1 = 0;
int gModel_DefaultGraphicsFlags = 0;
void *g_zModel_GraphicsFlagsOption = 0;
int g_Variant_FilterEnabled = 1;
zTag4Partial g_Variant_CurrentTag = {0};
zDiPartial *g_zModel_DiPoolBase = 0;
int g_zModel_DiPoolCapacity = 0;
int g_zModel_DiPoolInUseCount = 0;
int g_zModel_DiPoolFreeHeadIndex = -1;
zModel_MaterialSlot *g_zModel_MatlPool = 0;
int g_zModel_MatlPoolCapacity = 0;
int g_zModel_MatlPoolInUseCount = 0;
int g_zModel_MatlFreeHeadIndex = -1;
int g_zModel_MatlActiveHeadIndex = -1;
zModel_MaterialPartial *g_zModel_MatlReuseCache = 0;
zModel_MaterialPartial g_zModel_DefaultMaterial = {0};
int g_zRndr_GlobalStringCount = 6;
char *g_zRndr_GlobalStringTable[100] = {0};

namespace {
const unsigned int kSpanOcclusionTestSphereVisible = 0x00476cf0;

int TruncateToInt(float value) {
    return static_cast<int>(value);
}

bool TestSpanColumnVisible(int columnIndex) {
    int isVisible = 0;
    zRndr_SpanOcclusion_TestColumnVisibility(columnIndex, &isVisible);
    return isVisible > 0;
}
}

namespace zScene {
// Reimplements 0x476700: zScene::TestProjectedSphereVisible
// (Battlesport/zModel/gmod_scene.c)
RECOIL_NOINLINE int RECOIL_FASTCALL TestProjectedSphereVisible(zVec3 *center,
                                                                        float radius) {
    zMat4x3 slotBuffer = {0};
    zMath::MatStackPushPtr(reinterpret_cast<float *>(&slotBuffer));
    zMath::MatLoadCameraScratchB();

    zVec3 viewPoint = *center;
    zMath::MatTransformPointBatchInPlace(&viewPoint, 1);
    zMath::MatStackPopPtr();

    const float depthMinusRadius = viewPoint.z - radius;
    if (depthMinusRadius <= 0.0000999999975f) {
        return 1;
    }

    zProjectedPoint projectedPoint = {0};
    zMath::ProjectPointBatch(&viewPoint, &projectedPoint, 1);
    const zVec2 screenScale = zMath_Project_GetLastScreenScaleXY();
    const int projectedRadius =
        TruncateToInt((screenScale.x * radius) / depthMinusRadius);
    if (projectedRadius < 1) {
        return 0;
    }

    const int centerX = TruncateToInt(projectedPoint.x);
    zRndr::g_spanAllocCursor->sampleXMin = centerX - projectedRadius;
    if (static_cast<float>(zRndr::g_spanAllocCursor->sampleXMin) >= gClipRect_Primary.xMax) {
        return 0;
    }

    zRndr::g_spanAllocCursor->sampleXMax = centerX + projectedRadius;
    if (static_cast<float>(zRndr::g_spanAllocCursor->sampleXMax) < gClipRect_Primary.xMin) {
        return 0;
    }

    const int centerY = TruncateToInt(projectedPoint.y);
    int columnMin = centerY - projectedRadius;
    if (gClipRect_Primary.yMax - 2.0f < static_cast<float>(columnMin)) {
        return 0;
    }

    int columnMax = centerY + projectedRadius;
    if (static_cast<float>(columnMax) <= gClipRect_Primary.yMin) {
        return 0;
    }

    const int clipXMin = TruncateToInt(gClipRect_Primary.xMin);
    if (clipXMin > zRndr::g_spanAllocCursor->sampleXMin) {
        zRndr::g_spanAllocCursor->sampleXMin = clipXMin;
    }

    const int savedSampleXMin = zRndr::g_spanAllocCursor->sampleXMin;
    const int clipXMax = TruncateToInt(gClipRect_Primary.xMax - 2.0f);
    if (clipXMax < zRndr::g_spanAllocCursor->sampleXMax) {
        zRndr::g_spanAllocCursor->sampleXMax = clipXMax;
    }

    zRndr::g_spanAllocCursor->invDepth = 1.0f / depthMinusRadius;
    zRndr::g_spanAllocCursor->invDepthStep = zRndr::g_spanAllocCursor->invDepth;
    zRndr::g_spanAllocCursor->depthSlope = 0.0f;

    const int clipYMin = TruncateToInt(gClipRect_Primary.yMin + 1.0f);
    if (clipYMin > columnMin) {
        columnMin = clipYMin;
    }

    if (TestSpanColumnVisible(columnMin)) {
        return 1;
    }

    const int clipYMax = TruncateToInt(gClipRect_Primary.yMax - 2.0f);
    if (clipYMax < columnMax) {
        columnMax = clipYMax;
    }

    zRndr::g_spanAllocCursor->sampleXMin = savedSampleXMin;
    if (TestSpanColumnVisible(columnMax)) {
        return 1;
    }

    const int columnDelta = columnMax - columnMin;
    if (columnDelta <= 1) {
        return 0;
    }

    int midColumn = (columnDelta >> 1) + columnMin;
    zRndr::g_spanAllocCursor->sampleXMin = savedSampleXMin;
    if (TestSpanColumnVisible(midColumn)) {
        return 1;
    }

    int columnIndex;
    for (columnIndex = midColumn - 8; columnIndex > columnMin; columnIndex -= 8) {
        zRndr::g_spanAllocCursor->sampleXMin = savedSampleXMin;
        if (TestSpanColumnVisible(columnIndex)) {
            return 1;
        }
    }

    for (columnIndex = midColumn + 8; columnIndex < columnMax; columnIndex += 8) {
        zRndr::g_spanAllocCursor->sampleXMin = savedSampleXMin;
        if (TestSpanColumnVisible(columnIndex)) {
            return 1;
        }
    }

    return 0;
}
} // namespace zScene

// Reimplements 0x475c40: zModel_Display_Init
RECOIL_NOINLINE int RECOIL_CDECL zModel_Display_Init() {
    g_zModel_DisplayInitFlagA = 1;
    g_zModel_DisplayInitFlagB = 1;
    g_zModel_DisplayInitFlagC = 1;

    g_zModel_DisplayClipMode = 2;
    g_zModel_SpanOcclusionProc = reinterpret_cast<void *>(kSpanOcclusionTestSphereVisible);
    g_zModel_DisplayScratchCount = 0;
    g_zModel_DisplayScratchPtr = &g_zModel_ViewScaleX;
    g_zModel_DisplayClipX = 0;
    g_zModel_DisplayClipY = 0;
    g_zModel_DisplayClipWidth = 320.0f;
    g_zModel_DisplayClipHeight = 200.0f;
    g_zModel_DisplayClipMaxX = 319.0f;
    g_zModel_DisplayClipMaxY = 199.0f;
    g_zModel_DisplayParam4 = 4.0f;
    g_zModel_DisplayParam40 = 40.0f;
    g_zModel_DisplayClipReserved = 0;

    gModel_FogEnabled = 1;
    gModel_FogLinearModeEnabled = 1;
    gModel_FogDistanceStart = 500.0f;
    gModel_FogDistanceEnd = 700.0f;
    gModel_FogDistanceInvRange = 0.005f;
    gModel_FogHeightHigh = 300.0f;
    gModel_FogHeightLow = 200.0f;
    gModel_FogHeightInvRange = 0.01f;
    gModel_FogDensity = 2.0f;

    g_zModel_FogStart = 500.0f;
    g_zModel_FogEnd = 700.0f;
    g_zModel_FogHeightHigh = 300.0f;
    g_zModel_FogHeightLow = 200.0f;
    g_zModel_FogDistanceInvRange = 0.005f;
    g_zModel_FogHeightInvRange = 0.01f;
    g_zModel_FogDensity = 2.0f;
    g_zModel_FogReserved = 0;
    g_zModel_FogScale = 1.0f;

    if (g_zVideo_ActiveRendererPath != 0) {
        g_zModel_InverseZTolerance = 0.02f;
        g_zModel_HardwareInverseZTolerance = 0.02f;
    } else {
        g_zModel_InverseZTolerance = 0.01f;
    }

    g_zModel_TransformedVerts = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_SharedVec3ScratchA = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_PointInPolygonVertices = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_TransformedNormals = g_zModel_SharedVec3ScratchBStorage;
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;
    g_zModel_PointInPolygonEdgeNormals = g_zModel_SharedVec3ScratchBStorage;
    g_zModel_PointInPolygonVertexCount = 0;
    {
    for (int counterIndex = 0; counterIndex < 8; ++counterIndex) {
        g_zModel_ScratchCounters[counterIndex] = 0;
    }
    }
    g_zModel_PointInPolyTolX = 0.2f;
    g_zModel_PointInPolyTolY = 0.2f;

    g_zModel_DamageMaskCurrent = g_zModel_DamageMaskStorage;
    g_zModel_OptCatalogSlot = 0;
    g_zModel_OptCatalogAux0 = 0;
    g_zModel_OptCatalogAux1 = 0;
    gModel_DefaultGraphicsFlags = -1;

    g_zModel_GraphicsFlagsOption =
        zGame::Options_FindOption(g_zVideo_ActiveRendererPath != 0 ? "GfxFlags_HW" : "GfxFlags_SW");
    if (g_zModel_GraphicsFlagsOption == 0) {
        g_zModel_GraphicsFlagsOption = &gModel_DefaultGraphicsFlags;
    }

    zTag4::Clear(&g_Variant_CurrentTag);
    return 0;
}

// Reimplements 0x479c90: OptCatalog_SetDamageMaskUv (GameZRecoil/zModel/zModel_Display.cpp)
RECOIL_NOINLINE void RECOIL_STDCALL OptCatalog_SetDamageMaskUv(float u, float v) {
    g_OptCatalogDamageMaskPhaseU = u;
    g_OptCatalogDamageMaskPhaseV = v;
}

// Reimplements 0x479cb0: OptCatalog_SetDamageMaskEnabled
// (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL OptCatalog_SetDamageMaskEnabled(int enabled) {
    g_OptCatalogDamageMaskEnabled = enabled;
}

// Reimplements 0x479cc0: OptCatalog_IsDamageMaskSlotPtrRegistered
// (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
OptCatalog_IsDamageMaskSlotPtrRegistered(void *slotPtr) {
    for (int i = 0; i < 3; ++i) {
        if (g_OptCatalogDamageMaskHandles[i] == slotPtr) {
            return 1;
        }
    }

    return 0;
}

namespace zRndr {
// Reimplements 0x480ec0: zRndr::GlobalStringTable_ReleaseDynamicEntries
// (D:\Proj\GameZRecoil\zModel\gmod_matl.c)
RECOIL_NOINLINE void RECOIL_CDECL GlobalStringTable_ReleaseDynamicEntries() {
    for (int i = 6; i < g_zRndr_GlobalStringCount; ++i) {
        free(g_zRndr_GlobalStringTable[i]);
        g_zRndr_GlobalStringTable[i] = 0;
    }

    g_zRndr_GlobalStringCount = 6;
}
} // namespace zRndr

namespace zDi {
// Reimplements 0x481570: zDi::PtrToIndexOrMinus1
// (D:\Proj\GameZRecoil\zModel\gmod_const.c)
RECOIL_NOINLINE int RECOIL_FASTCALL PtrToIndexOrMinus1(zDiPartial *self) {
    if (self == 0) {
        return -1;
    }

    return static_cast<int>(self - g_zModel_DiPoolBase);
}

// Reimplements 0x4815a0: zDi::IndexToPtrOrNull
// (D:\Proj\GameZRecoil\zModel\gmod_const.c)
RECOIL_NOINLINE zDiPartial *RECOIL_FASTCALL IndexToPtrOrNull(int index) {
    if (index < 0) {
        return 0;
    }

    return &g_zModel_DiPoolBase[index];
}
} // namespace zDi

namespace zModel_DiPool {
// Reimplements 0x482080: zModel_DiPool::AllocFromFreeList
// (D:\Proj\GameZRecoil\zModel\gmod_const.c)
RECOIL_NOINLINE zDiPartial *RECOIL_CDECL AllocFromFreeList() {
    const int slotIndex = g_zModel_DiPoolFreeHeadIndex;
    if (slotIndex < 0) {
        zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c", 0x4a1,
                          "ERROR: Creating Model3D; model buffer full.");
        return 0;
    }

    zDiPartial *const entry = &g_zModel_DiPoolBase[slotIndex];
    g_zModel_DiPoolFreeHeadIndex = entry->nextFreeIndex;
    ++g_zModel_DiPoolInUseCount;
    memset(entry, 0, offsetof(zDiPartial, nextFreeIndex));
    entry->flags = (entry->flags & 0xffffffdf) | 0x03;
    return entry;
}

// Reimplements 0x4820f0: zModel_DiPool::FreeIfUnreferenced
// (D:\Proj\GameZRecoil\zModel\gmod_const.c)
RECOIL_NOINLINE int RECOIL_FASTCALL FreeIfUnreferenced(zDiPartial *di) {
    if (di == 0) {
        return 5;
    }

    if (di->refCount != 0) {
        return 1;
    }

    zDi::FreeContents(di);
    memset(di, 0, offsetof(zDiPartial, nextFreeIndex));

    const ptrdiff_t slotIndex = di - g_zModel_DiPoolBase;
    g_zModel_DiPoolBase[slotIndex].nextFreeIndex = g_zModel_DiPoolFreeHeadIndex;
    --g_zModel_DiPoolInUseCount;
    g_zModel_DiPoolFreeHeadIndex = static_cast<int>(slotIndex);
    return 0;
}
} // namespace zModel_DiPool

namespace zModel_MatlSlot {
// Reimplements 0x480dc0: zModel_MatlSlot::Release
// (D:\Proj\GameZRecoil\zModel\zModel_Matl.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL Release(zModel_MaterialSlot *slot) {
    if (slot == 0) {
        return;
    }

    if ((slot->material.flags & 0x0400) != 0 && slot->material.cycle != 0) {
        if (slot->material.cycle->frameTable != 0) {
            free(slot->material.cycle->frameTable);
            slot->material.cycle->frameTable = 0;
        }
        free(slot->material.cycle);
        slot->material.cycle = 0;
    }

    memset(&slot->material, 0, sizeof(slot->material));

    const int slotIndex = zModel_MatlSlot::IndexFromPtrOrMinus1(slot);
    const short prevIndex = slot->prevPoolIndex;
    const short nextIndex = slot->nextPoolIndex;

    if (prevIndex >= 0) {
        g_zModel_MatlPool[prevIndex].nextPoolIndex = nextIndex;
    }
    if (nextIndex >= 0) {
        g_zModel_MatlPool[nextIndex].prevPoolIndex = prevIndex;
    }
    if (g_zModel_MatlActiveHeadIndex == slotIndex) {
        g_zModel_MatlActiveHeadIndex = nextIndex;
    }

    slot->prevPoolIndex = -1;
    slot->nextPoolIndex = static_cast<short>(g_zModel_MatlFreeHeadIndex);
    if (g_zModel_MatlFreeHeadIndex >= 0) {
        g_zModel_MatlPool[g_zModel_MatlFreeHeadIndex].prevPoolIndex =
            static_cast<short>(slotIndex);
    }

    g_zModel_MatlFreeHeadIndex = slotIndex;
    --g_zModel_MatlPoolInUseCount;
}
} // namespace zModel_MatlSlot

namespace zModel_MatlBuffer {
// Reimplements 0x480d80: zModel_MatlBuffer::ReleaseAllActive
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE int RECOIL_CDECL ReleaseAllActive() {
    while (g_zModel_MatlActiveHeadIndex >= 0) {
        if (g_zModel_MatlPool == 0 ||
            g_zModel_MatlActiveHeadIndex >= g_zModel_MatlPoolCapacity) {
            g_zModel_MatlActiveHeadIndex = -1;
            break;
        }
        zModel_MatlSlot::Release(&g_zModel_MatlPool[g_zModel_MatlActiveHeadIndex]);
    }

    g_zModel_MatlReuseCache = 0;
    return 0;
}

// Reimplements 0x480f10: zModel_MatlBuffer::Shutdown
// (D:\Proj\GameZRecoil\zModel\gmod_matl.c)
// Reimplements 0x475fa0: zModel_Display::Shutdown
// (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp)
RECOIL_NOINLINE int RECOIL_CDECL Shutdown() {
    ReleaseAllActive();
    if (g_zModel_MatlPool != 0) {
        free(g_zModel_MatlPool);
        g_zModel_MatlPool = 0;
    }

    g_zModel_MatlPoolCapacity = 0;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlFreeHeadIndex = -1;
    g_zModel_MatlActiveHeadIndex = -1;
    zRndr::GlobalStringTable_ReleaseDynamicEntries();
    g_zModel_MatlReuseCache = 0;
    return 0;
}
} // namespace zModel_MatlBuffer

namespace zModel_Matl {
// Reimplements 0x4805e0: zModel_Matl::GetPoolEntry
RECOIL_NOINLINE zModel_MaterialSlot *RECOIL_FASTCALL GetPoolEntry(int index) {
    if (index < 0) {
        return 0;
    }

    return &g_zModel_MatlPool[index];
}
} // namespace zModel_Matl

namespace zModel_Display {
// Reimplements 0x475f60: zModel_Display::Reset
RECOIL_NOINLINE int RECOIL_CDECL Reset() {
    if (g_zModel_DiPoolCapacity > 0) {
        for (int i = 0; i < g_zModel_DiPoolInUseCount; ++i) {
            zModel_DiPool::FreeIfUnreferenced(&g_zModel_DiPoolBase[i]);
        }
    }

    return 0;
}

RECOIL_NOINLINE int RECOIL_CDECL Shutdown() {
    zModel_MatlBuffer::Shutdown();
    if (g_zModel_DiPoolCapacity > 0) {
        Reset();
        free(g_zModel_DiPoolBase);
        g_zModel_DiPoolBase = 0;
    }

    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;
    return 0;
}

// Reimplements 0x475e60: zModel_Display::ShutdownThunk
// (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp)
RECOIL_NOINLINE int RECOIL_CDECL ShutdownThunk() {
    Shutdown();
    return 0;
}
} // namespace zModel_Display

namespace zTag4 {
// Reimplements 0x476320: zTag4::Clear
RECOIL_NOINLINE void RECOIL_FASTCALL Clear(zTag4Partial *tag) {
    if (tag == 0) {
        return;
    }

    tag->count = 0;
    tag->tags[0] = 0xff;
    tag->tags[1] = 0xff;
    tag->tags[2] = 0xff;
}
} // namespace zTag4

namespace VariantTag {
// Reimplements 0x476370: VariantTag::TagsOverlap
// (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL TagsOverlap(const zTag4Partial *tagA,
                                                         const zTag4Partial *tagB) {
    if (g_Variant_FilterEnabled == 0) {
        return 1;
    }

    const unsigned char countA = tagA->count;
    if (countA == 0 || tagB->count == 0) {
        return 1;
    }

    {
    for (int indexA = 0; indexA < countA; ++indexA) {
        const unsigned char tagIdA = tagA->tags[indexA];
        if (tagIdA == 0xff) {
            return 1;
        }

        {
        for (int indexB = 0; indexB < tagB->count; ++indexB) {
            const unsigned char tagIdB = tagB->tags[indexB];
            if (tagIdB == 0xff || tagIdA == tagIdB) {
                return 1;
            }
        }
        }
    }
    }

    return 0;
}

// Reimplements 0x476400: VariantTag::CurrentAllowsId
// (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL CurrentAllowsId(int variantId) {
    if (g_Variant_FilterEnabled == 0) {
        return 1;
    }

    if (variantId == 0xff) {
        return 1;
    }

    const unsigned char count = g_Variant_CurrentTag.count;
    if (count == 0) {
        return 1;
    }

    const unsigned char id = static_cast<unsigned char>(variantId);
    for (int i = 0; i < count; ++i) {
        const unsigned char tag = g_Variant_CurrentTag.tags[i];
        if (tag == 0xff || tag == id) {
            return 1;
        }
    }

    return 0;
}
} // namespace VariantTag
