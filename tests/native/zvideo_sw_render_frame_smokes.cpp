#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zRndr/zrndr.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <cstring>

namespace {
int g_zVideoRenderFrameFlushSortedCount;
int g_zVideoRenderFrameFlushOverwriteCount;
int g_zVideoRenderFrameFlushQuadCount;
int g_zVideoRenderFrameClearRectCount;
zVidRect32 g_zVideoRenderFrameClearRects[4];

void CaptureFlushSortedPolys() {
    ++g_zVideoRenderFrameFlushSortedCount;
}

void CaptureFlushOverwritePolys() {
    ++g_zVideoRenderFrameFlushOverwriteCount;
}

void CaptureFlushQuadBatch() {
    ++g_zVideoRenderFrameFlushQuadCount;
}

int __fastcall CaptureClearZBufferRect(
    zVidRect32 *rect
) {
    const int index = g_zVideoRenderFrameClearRectCount;
    if (index < 4) {
        g_zVideoRenderFrameClearRects[index] = *rect;
    }
    ++g_zVideoRenderFrameClearRectCount;
    return 0;
}

bool ActiveContextNearFloat(
    float lhs,
    float rhs
) {
    const float diff = lhs - rhs;
    return diff > -0.0002f && diff < 0.0002f;
}

int ActiveContextFloatBits(
    float value
) {
    int bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}
}

extern "C" int zvideo_set_active_view_context_smoke(void) {
    zClass_CameraDataPartial *const savedViewContext = g_zVideo_pActiveViewContext;
    zClass_CameraDataPartial *const savedProjectionViewContext =
        g_zVideo_pActiveProjectionViewContext;
    const int savedRendererPath = g_zVideo_ActiveRendererPath;
    const zClipRectPartial savedClipRect = gClipRect_Primary;
    const float savedProjectClipLeft = g_zVideo_ProjectClipLeft;
    const float savedProjectClipTop = g_zVideo_ProjectClipTop;
    const float savedProjectClipRight = g_zVideo_ProjectClipRight;
    const float savedProjectClipBottom = g_zVideo_ProjectClipBottom;
    const zVideo_SurfaceStatePartial savedPrimarySurfaceState = g_zVideo_PrimarySurfaceState;
    const int savedScreenWidth = g_zMath_ScreenWidthPx;
    const int savedScreenHeight = g_zMath_ScreenHeightPx;

    zClass_WindowDataPartial windowData{};
    windowData.viewportWidth = 10;
    windowData.viewportHeight = 20;
    windowData.resolutionWidth = 300;
    windowData.resolutionHeight = 200;

    zClass_NodePartial windowNode{};
    windowNode.classId = 3;
    windowNode.classData = &windowData;

    zClass_CameraDataPartial viewContext{};
    viewContext.windowNode = &windowNode;
    viewContext.nearClip = 0.25f;
    viewContext.farClip = 500.0f;
    viewContext.viewportScaleX = 2.0f;
    viewContext.viewportScaleY = 4.0f;
    viewContext.fovX = 111.0f;
    viewContext.fovY = 222.0f;

    zClass_CameraDataPartial renderFrameContext{};
    g_zVideo_pActiveViewContext = &renderFrameContext;
    g_zVideo_pActiveProjectionViewContext = 0;

    std::memset(g_zVideo_QuadBatchItemsBase, 0, sizeof(g_zVideo_QuadBatchItemsBase));
    g_zVideo_ActiveRendererPath = 0;
    zVideo_SetActiveViewContext(&viewContext);

    int status = 0;
    if (g_zVideo_pActiveProjectionViewContext != &viewContext ||
        g_zVideo_pActiveViewContext != &renderFrameContext ||
        !ActiveContextNearFloat(viewContext.nearClip, 1.0f) ||
        !ActiveContextNearFloat(gClipRect_Primary.zMin, 2.0f) ||
        !ActiveContextNearFloat(gClipRect_Primary.zMax, 500.0f)) {
        status = 1;
    } else if (!ActiveContextNearFloat(g_zVideo_QuadBatchItemsBase[0].vertices[0].sz, 0.5f) ||
               !ActiveContextNearFloat(g_zVideo_QuadBatchItemsBase[15].vertices[3].rhw, 0.5f)) {
        status = 2;
    } else if (!ActiveContextNearFloat(gClipRect_Primary.xMin, 9.500999f) ||
               !ActiveContextNearFloat(gClipRect_Primary.xMax, 311.499f) ||
               !ActiveContextNearFloat(gClipRect_Primary.xMaxAlt, 310.499f) ||
               !ActiveContextNearFloat(gClipRect_Primary.yMin, 19.500999f) ||
               !ActiveContextNearFloat(gClipRect_Primary.yMax, 221.499f) ||
               !ActiveContextNearFloat(gClipRect_Primary.yMaxAlt, 220.499f)) {
        status = 3;
    } else if (!ActiveContextNearFloat(g_zVideo_ProjectClipLeft, 10.0f) ||
               !ActiveContextNearFloat(g_zVideo_ProjectClipTop, 20.0f) ||
               !ActiveContextNearFloat(g_zVideo_ProjectClipRight, 309.999f) ||
               !ActiveContextNearFloat(g_zVideo_ProjectClipBottom, 219.999f)) {
        status = 4;
    } else if (!ActiveContextNearFloat(g_zMath_FocalScaleX, 2.0f) ||
               !ActiveContextNearFloat(g_zMath_FocalScaleY, 4.0f) ||
               !ActiveContextNearFloat(g_zMath_ProjScaleX, 300.0f) ||
               !ActiveContextNearFloat(g_zMath_ProjScaleY, 400.0f) ||
               !ActiveContextNearFloat(g_zMath_ProjOffsetX, 160.0f) ||
               !ActiveContextNearFloat(g_zMath_ProjOffsetY, 120.0f) ||
               !ActiveContextNearFloat(g_zMath_ProjSphereRadiusScale, 1.0f) ||
               !ActiveContextNearFloat(g_zMath_ProjDepth, 500.0f) ||
               g_zMath_ScreenWidthPx != ActiveContextFloatBits(111.0f) ||
               g_zMath_ScreenHeightPx != ActiveContextFloatBits(222.0f)) {
        status = 5;
    } else {
        zClass_CameraDataPartial fallbackContext{};
        fallbackContext.nearClip = 3.0f;
        fallbackContext.farClip = 1200.0f;
        fallbackContext.viewportScaleX = 1.0f;
        fallbackContext.viewportScaleY = 2.0f;
        fallbackContext.fovX = 333.0f;
        fallbackContext.fovY = 444.0f;
        g_zVideo_PrimarySurfaceState.width = 640;
        g_zVideo_PrimarySurfaceState.height = 480;
        g_zVideo_ActiveRendererPath = 1;
        zVideo_SetActiveViewContext(&fallbackContext);

        if (g_zVideo_pActiveProjectionViewContext != &fallbackContext ||
            g_zVideo_pActiveViewContext != &renderFrameContext ||
            !ActiveContextNearFloat(gClipRect_Primary.zMin, 6.0f) ||
            !ActiveContextNearFloat(gClipRect_Primary.zMax, 1200.0f) ||
            !ActiveContextNearFloat(gClipRect_Primary.xMin, 0.0f) ||
            !ActiveContextNearFloat(gClipRect_Primary.xMax, 640.001f) ||
            !ActiveContextNearFloat(gClipRect_Primary.xMaxAlt, 640.001f) ||
            !ActiveContextNearFloat(gClipRect_Primary.yMin, 0.0f) ||
            !ActiveContextNearFloat(gClipRect_Primary.yMax, 480.001f) ||
            !ActiveContextNearFloat(gClipRect_Primary.yMaxAlt, 480.001f) ||
            !ActiveContextNearFloat(g_zVideo_ProjectClipLeft, 0.0f) ||
            !ActiveContextNearFloat(g_zVideo_ProjectClipTop, 0.0f) ||
            !ActiveContextNearFloat(g_zVideo_ProjectClipRight, 639.999f) ||
            !ActiveContextNearFloat(g_zVideo_ProjectClipBottom, 480.0f) ||
            !ActiveContextNearFloat(g_zMath_ProjScaleX, 320.0f) ||
            !ActiveContextNearFloat(g_zMath_ProjScaleY, 480.0f) ||
            !ActiveContextNearFloat(g_zMath_ProjOffsetX, 320.0f) ||
            !ActiveContextNearFloat(g_zMath_ProjOffsetY, 240.0f) ||
            g_zMath_ScreenWidthPx != ActiveContextFloatBits(333.0f) ||
            g_zMath_ScreenHeightPx != ActiveContextFloatBits(444.0f)) {
            status = 6;
        }
    }

    g_zVideo_pActiveViewContext = savedViewContext;
    g_zVideo_pActiveProjectionViewContext = savedProjectionViewContext;
    g_zVideo_ActiveRendererPath = savedRendererPath;
    gClipRect_Primary = savedClipRect;
    g_zVideo_ProjectClipLeft = savedProjectClipLeft;
    g_zVideo_ProjectClipTop = savedProjectClipTop;
    g_zVideo_ProjectClipRight = savedProjectClipRight;
    g_zVideo_ProjectClipBottom = savedProjectClipBottom;
    g_zVideo_PrimarySurfaceState = savedPrimarySurfaceState;
    g_zMath_ScreenWidthPx = savedScreenWidth;
    g_zMath_ScreenHeightPx = savedScreenHeight;
    return status;
}

extern "C" int zvideo_sw_render_frame_smoke(void) {
    zClass_CameraDataPartial *const savedViewContext = g_zVideo_pActiveViewContext;
    const int savedActiveRendererPath = g_zVideo_ActiveRendererPath;
    const int savedSceneDepth = g_zVideo_D3DSceneDepth;
    zVideo_FlushProc const savedFlushSorted = g_zVideo_pfnFlushSortedPolys;
    zVideo_FlushProc const savedFlushOverwrite = g_zVideo_pfnFlushOverwritePolys;
    zVideo_FlushProc const savedFlushQuad = g_zVideo_pfnFlushQuadBatch;
    zVideo_ClearZBufferRectProc const savedClearZBufferRect = g_zVideo_pfnClearZBufferRect;
    const int savedVariantFilterEnabled = g_Variant_FilterEnabled;
    const zTag4Partial savedVariantTag = g_Variant_CurrentTag;
    const zTag4Partial savedActiveVariantTag = g_zVideo_ActiveViewVariantTag;
    const int savedLodStackTop = g_zClass_LodDistanceStateStackTop;
    const int savedAutoClipEnabled = g_zClass_CameraAutoClipDistanceAdjustEnabled;
    const float savedAutoClipThreshold = g_zClass_CameraAutoClipDistanceThreshold;
    const float savedAutoClipScale = g_zClass_CameraAutoClipDistanceScale;
    const float savedAutoClipStep = g_zClass_CameraAutoClipDistanceStep;
    const float savedAutoClipMinScale = g_zClass_CameraAutoClipDistanceMinScale;
    const float savedFrameDelta = g_FrameDeltaTimeSec;
    const int savedLensFlareQueueCount = zRndr::g_lensFlareSampleQueueCount;
    const int savedLensFlareVisibleCount = zRndr::g_lensFlareVisibleSampleCount;
    zClass_TypeListLink *const savedTypeListHead8 = zClass_TypeList::Head(8);
    zClass_TypeListLink *const savedTypeListTail8 = zClass_TypeList::Tail(8);
    int *const savedClipStackTop = gModel_ClipMaskStackTop;
    const int savedClipStack0 = gModel_ClipMaskStack[0];
    const int savedObjectHseTestEnabled = g_zClass_ObjectHseTestEnabled;
    const int savedFogEnabled = zModel_Fog_IsEnabled();
    zVec3 *const savedPolygonVertices = g_zModel_PointInPolygonVertices;
    zVec3 *const savedPolygonNormals = g_zModel_PointInPolygonEdgeNormals;
    const int savedPolygonVertexCount = g_zModel_PointInPolygonVertexCount;
    int *const savedMatrixIdentitySlot = zMath::g_currentMatrixIdentityFlagSlot;
    float **const savedMatrixPtrSlot = zMath::g_currentMatrixPtrSlot;

    int matrixIdentityFlags[16] = {};
    float *matrixSlots[16] = {};
    zMat4x3 matrixStorage[16] = {};
    const zMat4x3 identity{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                           0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    std::memcpy(&matrixStorage[0], &identity, sizeof(identity));
    matrixIdentityFlags[0] = 1;
    matrixSlots[0] = reinterpret_cast<float *>(&matrixStorage[0]);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixIdentityFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];

    zWorldAreaPartial row0[3] = {};
    zWorldAreaPartial row1[3] = {};
    zWorldAreaPartial row2[3] = {};
    zWorldAreaPartial *rows[3] = {row0, row1, row2};
    for (int rowIndex = 0; rowIndex < 3; ++rowIndex) {
        for (int colIndex = 0; colIndex < 3; ++colIndex) {
            rows[rowIndex][colIndex].areaIndex = 1;
            rows[rowIndex][colIndex].cellMinX = static_cast<float>(colIndex * 10);
            rows[rowIndex][colIndex].cellMinZ = static_cast<float>(rowIndex * 10);
            rows[rowIndex][colIndex].bboxRadius = 1.0f;
        }
    }

    zClass_WorldDataPartial worldData{};
    worldData.originX = 0.0f;
    worldData.originZ = 0.0f;
    worldData.worldMaxX = 30.0f;
    worldData.worldMaxZ = 19.0f;
    worldData.areaCellSizeX = 10.0f;
    worldData.areaCellSizeZ = 10.0f;
    worldData.areaInvSizeX = 0.1f;
    worldData.areaInvSizeZ = 0.1f;
    worldData.areaHalfSizeX = 5.0f;
    worldData.areaHalfSizeZ = 5.0f;
    worldData.areaCellRadiusBias = -1000.0f;
    worldData.areaGridColCount = 3;
    worldData.areaGridRowCount = 3;
    worldData.areaGridRows = rows;

    zClass_NodePartial world{};
    world.classId = 2;
    world.classData = &worldData;

    zClass_WindowDataPartial windowData{};
    windowData.viewportWidth = 64;
    windowData.viewportHeight = 48;
    windowData.resolutionWidth = 64;
    windowData.resolutionHeight = 48;
    windowData.clearPolyIndexFlags = 0x80000001;
    windowData.clearPolys[0].vertices[0] = {4.9f, 7.9f, 0.0f};
    windowData.clearPolys[0].vertices[1] = {1.1f, 3.2f, 0.0f};
    windowData.clearPolys[0].vertices[2] = {6.6f, 2.2f, 0.0f};
    windowData.clearPolys[0].vertCount = 0x80000003;

    zClass_NodePartial window{};
    window.classId = 3;
    window.classData = &windowData;

    zClass_CameraDataPartial cameraData{};
    cameraData.worldNode = &world;
    cameraData.windowNode = &window;
    cameraData.posOffset = {0.0f, 0.0f, -1.0f};
    cameraData.nearClip = 1.0f;
    cameraData.farClip = 100.0f;
    cameraData.clipDistance = 100.0f;
    cameraData.invClipDistanceSq = 0.0001f;
    cameraData.viewportScaleX = 1.0f;
    cameraData.viewportScaleY = 1.0f;
    cameraData.fovX = 1.0f;
    cameraData.fovY = 1.0f;
    cameraData.frustumOrigin = {0.0f, 0.0f, 0.0f};
    cameraData.frustumCorners[0] = {10.0f, 0.0f, 0.0f};
    cameraData.frustumCorners[1] = {0.0f, 0.0f, 2.0f};
    cameraData.variantOverrideEnabled = 1;
    cameraData.variantTag.count = 2;
    cameraData.variantTag.tags[0] = 0x44;
    cameraData.variantTag.tags[1] = 0x55;
    cameraData.variantTag.tags[2] = 0xff;

    zClass_NodePartial camera{};
    camera.classId = 1;
    camera.classData = &cameraData;

    zClass_TypeListLink cameraListA{};
    zClass_TypeListLink cameraListB{};
    cameraListA.next = &cameraListB;
    cameraListA.node = &camera;
    cameraListB.node = &camera;

    g_zVideoRenderFrameFlushSortedCount = 0;
    g_zVideoRenderFrameFlushOverwriteCount = 0;
    g_zVideoRenderFrameFlushQuadCount = 0;
    g_zVideoRenderFrameClearRectCount = 0;
    std::memset(g_zVideoRenderFrameClearRects, 0, sizeof(g_zVideoRenderFrameClearRects));
    g_zVideo_ActiveRendererPath = 1;
    g_zVideo_D3DSceneDepth = 2;
    g_zVideo_pfnFlushSortedPolys = CaptureFlushSortedPolys;
    g_zVideo_pfnFlushOverwritePolys = CaptureFlushOverwritePolys;
    g_zVideo_pfnFlushQuadBatch = CaptureFlushQuadBatch;
    g_zVideo_pfnClearZBufferRect = CaptureClearZBufferRect;
    g_Variant_FilterEnabled = 1;
    g_Variant_CurrentTag = {};
    g_zVideo_ActiveViewVariantTag = {};
    g_zClass_LodDistanceStateStackTop = 7;
    g_zClass_CameraAutoClipDistanceAdjustEnabled = 1;
    g_zClass_CameraAutoClipDistanceThreshold = 0.04f;
    g_zClass_CameraAutoClipDistanceScale = 0.75f;
    g_zClass_CameraAutoClipDistanceStep = 0.05f;
    g_zClass_CameraAutoClipDistanceMinScale = 0.6f;
    g_FrameDeltaTimeSec = 0.02f;
    zRndr::g_lensFlareSampleQueueCount = 0;
    zRndr::g_lensFlareVisibleSampleCount = 0;
    zClass_TypeList::Head(8) = &cameraListA;
    zClass_TypeList::Tail(8) = &cameraListB;
    gModel_ClipMaskStackTop = gModel_ClipMaskStack;
    gModel_ClipMaskStack[0] = 0;
    g_zClass_ObjectHseTestEnabled = 0;
    zModel_Fog_SetEnabled(0);
    zVec3 polygonVertices[8] = {};
    zVec3 polygonNormals[8] = {};
    g_zModel_PointInPolygonVertices = polygonVertices;
    g_zModel_PointInPolygonEdgeNormals = polygonNormals;
    g_zModel_PointInPolygonVertexCount = 0;

    int status = 0;
    if (zVideo_sw_RenderFrame(&camera, 0) != 0) {
        status = 1;
    } else if (g_zVideo_pActiveViewContext != &cameraData ||
               g_zVideoRenderFrameFlushSortedCount != 3 ||
               g_zVideoRenderFrameFlushOverwriteCount != 2 ||
               g_zVideoRenderFrameFlushQuadCount != 1) {
        status = 2;
    } else if (g_zVideo_D3DSceneDepth != 1 ||
               g_zClass_LodDistanceStateStackTop != 0 ||
               g_zClass_CameraAutoClipDistanceScale != 0.8f ||
               cameraData.clipDistance != 0.8f) {
        status = 3;
    } else if (g_Variant_CurrentTag.count != 2 || g_Variant_CurrentTag.tags[0] != 0x44 ||
               g_Variant_CurrentTag.tags[1] != 0x55 ||
               g_zVideo_ActiveViewVariantTag.count != 2 ||
               g_zVideo_ActiveViewVariantTag.tags[0] != 0x44 ||
               g_zVideo_ActiveViewVariantTag.tags[1] != 0x55) {
        status = 4;
    } else if (g_zVideoRenderFrameClearRectCount != 1 ||
               g_zVideoRenderFrameClearRects[0].left != 1 ||
               g_zVideoRenderFrameClearRects[0].top != 2 ||
               g_zVideoRenderFrameClearRects[0].right != 6 ||
               g_zVideoRenderFrameClearRects[0].bottom != 7) {
        status = 5;
    }

    g_zVideo_pActiveViewContext = savedViewContext;
    g_zVideo_ActiveRendererPath = savedActiveRendererPath;
    g_zVideo_D3DSceneDepth = savedSceneDepth;
    g_zVideo_pfnFlushSortedPolys = savedFlushSorted;
    g_zVideo_pfnFlushOverwritePolys = savedFlushOverwrite;
    g_zVideo_pfnFlushQuadBatch = savedFlushQuad;
    g_zVideo_pfnClearZBufferRect = savedClearZBufferRect;
    g_Variant_FilterEnabled = savedVariantFilterEnabled;
    g_Variant_CurrentTag = savedVariantTag;
    g_zVideo_ActiveViewVariantTag = savedActiveVariantTag;
    g_zClass_LodDistanceStateStackTop = savedLodStackTop;
    g_zClass_CameraAutoClipDistanceAdjustEnabled = savedAutoClipEnabled;
    g_zClass_CameraAutoClipDistanceThreshold = savedAutoClipThreshold;
    g_zClass_CameraAutoClipDistanceScale = savedAutoClipScale;
    g_zClass_CameraAutoClipDistanceStep = savedAutoClipStep;
    g_zClass_CameraAutoClipDistanceMinScale = savedAutoClipMinScale;
    g_FrameDeltaTimeSec = savedFrameDelta;
    zRndr::g_lensFlareSampleQueueCount = savedLensFlareQueueCount;
    zRndr::g_lensFlareVisibleSampleCount = savedLensFlareVisibleCount;
    zClass_TypeList::Head(8) = savedTypeListHead8;
    zClass_TypeList::Tail(8) = savedTypeListTail8;
    gModel_ClipMaskStackTop = savedClipStackTop;
    gModel_ClipMaskStack[0] = savedClipStack0;
    g_zClass_ObjectHseTestEnabled = savedObjectHseTestEnabled;
    zModel_Fog_SetEnabled(savedFogEnabled);
    g_zModel_PointInPolygonVertices = savedPolygonVertices;
    g_zModel_PointInPolygonEdgeNormals = savedPolygonNormals;
    g_zModel_PointInPolygonVertexCount = savedPolygonVertexCount;
    zMath::g_currentMatrixIdentityFlagSlot = savedMatrixIdentitySlot;
    zMath::g_currentMatrixPtrSlot = savedMatrixPtrSlot;
    return status;
}
