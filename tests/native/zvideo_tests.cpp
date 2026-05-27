#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/include/zClipRect.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "zClass.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ddraw.h>

extern "C" unsigned int g_HudUi_InvalidateMask;

namespace {
constexpr std::size_t kFxPass3ConfigSize = 0x1f0;
constexpr std::size_t kFxPass3InputRect0Offset = 0x10;
constexpr std::size_t kFxPass3InputRect1Offset = 0x14;
constexpr std::size_t kFxPass3SurfacePixelsOffset = 0x18;
constexpr std::size_t kFxPass3SurfaceWidthOffset = 0x1c;
constexpr std::size_t kFxPass3SurfaceHeightOffset = 0x20;
constexpr std::size_t kFxPass3SurfacePitchOffset = 0x24;
constexpr std::size_t kFxPass3RootElementOffset = 0x28;
constexpr std::size_t kFxPass3RootPackedColorOffset = 0x60;
constexpr std::size_t kFxPass3RootAlphaOffset = 0x68;
constexpr std::size_t kFxPass3SlotsOffset = 0x70;
constexpr std::size_t kFxPass3SlotSize = 0x4c;
constexpr std::size_t kFxPass3SlotCurrentRadiusOffset = 0x38;
constexpr std::size_t kFxPass3SlotMaxRadiusOffset = 0x3c;
constexpr std::size_t kFxPass3SlotExtentOffset = 0x40;
constexpr std::size_t kFxPass3SlotSinFreqOffset = 0x44;
constexpr std::size_t kFxPass3SlotSinPhaseOffset = 0x48;
constexpr std::size_t kFxPass3SlotWriteIndexOffset = 0x1ec;

unsigned char *FxPass3ConfigBytes() {
    return reinterpret_cast<unsigned char *>(&g_zVideo_FxPass3ConfigLocal);
}

template <typename T> T &FxPass3FieldAt(std::size_t offset) {
    return *reinterpret_cast<T *>(FxPass3ConfigBytes() + offset);
}

int g_zVideoPaletteCaptureCallCount;
unsigned short g_zVideoPaletteCaptureFirstEntry;
unsigned short g_zVideoPaletteCaptureEntryCount;
PALETTEENTRY g_zVideoPaletteCaptureEntries[256];
int g_zVideoPaletteCaptureReturnValue;
int g_fxPass3UpdateCount;
float g_fxPass3UpdateDelta[4];
int g_zVideoRenderFrameFlushSortedCount;
int g_zVideoRenderFrameFlushOverwriteCount;
int g_zVideoRenderFrameFlushQuadCount;
int g_zVideoRenderFrameClearRectCount;
zVidRect32 g_zVideoRenderFrameClearRects[4];
    int g_zVideoTestLockSurfaceCount;
    zVideo_SurfaceStatePartial *g_zVideoTestLockSurfaceState;
    int g_zVideoTestUnlockSurfaceCount;
    zVideo_SurfaceStatePartial *g_zVideoTestUnlockSurfaceState;

int RECOIL_FASTCALL CapturePaletteSetEntries(unsigned short firstEntry,
                                             unsigned short entryCount,
                                             PALETTEENTRY *entries) {
    ++g_zVideoPaletteCaptureCallCount;
    g_zVideoPaletteCaptureFirstEntry = firstEntry;
    g_zVideoPaletteCaptureEntryCount = entryCount;
    std::memcpy(g_zVideoPaletteCaptureEntries, entries, sizeof(g_zVideoPaletteCaptureEntries));
    return g_zVideoPaletteCaptureReturnValue;
}

struct FakeD3DDevice2Object {
    void **vtable;
};

void *gFakeD3DDevice2VTable[24];
HRESULT gFakeD3DBeginSceneResult;
HRESULT gFakeD3DEndSceneResult;
int gFakeD3DBeginSceneCalls;
int gFakeD3DEndSceneCalls;
int gFakeD3DSetRenderStateCalls;
D3DRENDERSTATETYPE gFakeD3DRenderStates[4];
DWORD gFakeD3DRenderStateValues[4];

HRESULT __stdcall FakeD3DDevice2_BeginScene(IDirect3DDevice2 *) {
    ++gFakeD3DBeginSceneCalls;
    return gFakeD3DBeginSceneResult;
}

HRESULT __stdcall FakeD3DDevice2_EndScene(IDirect3DDevice2 *) {
    ++gFakeD3DEndSceneCalls;
    return gFakeD3DEndSceneResult;
}

HRESULT __stdcall FakeD3DDevice2_SetRenderState(IDirect3DDevice2 *,
                                                D3DRENDERSTATETYPE renderState,
                                                DWORD value) {
    if (gFakeD3DSetRenderStateCalls < 4) {
        gFakeD3DRenderStates[gFakeD3DSetRenderStateCalls] = renderState;
        gFakeD3DRenderStateValues[gFakeD3DSetRenderStateCalls] = value;
    }
    ++gFakeD3DSetRenderStateCalls;
    return DD_OK;
}

void InstallFakeD3DDevice2(FakeD3DDevice2Object &device) {
    std::memset(gFakeD3DDevice2VTable, 0, sizeof(gFakeD3DDevice2VTable));
    gFakeD3DDevice2VTable[10] = reinterpret_cast<void *>(FakeD3DDevice2_BeginScene);
    gFakeD3DDevice2VTable[11] = reinterpret_cast<void *>(FakeD3DDevice2_EndScene);
    gFakeD3DDevice2VTable[23] = reinterpret_cast<void *>(FakeD3DDevice2_SetRenderState);
    device.vtable = gFakeD3DDevice2VTable;
    g_zVideo_pD3DDevice = reinterpret_cast<IDirect3DDevice2 *>(&device);
    gFakeD3DBeginSceneResult = DD_OK;
    gFakeD3DEndSceneResult = DD_OK;
    gFakeD3DBeginSceneCalls = 0;
    gFakeD3DEndSceneCalls = 0;
    gFakeD3DSetRenderStateCalls = 0;
    std::memset(gFakeD3DRenderStates, 0, sizeof(gFakeD3DRenderStates));
    std::memset(gFakeD3DRenderStateValues, 0, sizeof(gFakeD3DRenderStateValues));
}

void RECOIL_CDECL CaptureFlushSortedPolys() {
    ++g_zVideoRenderFrameFlushSortedCount;
}

void RECOIL_CDECL CaptureFlushOverwritePolys() {
    ++g_zVideoRenderFrameFlushOverwriteCount;
}

void RECOIL_CDECL CaptureFlushQuadBatch() {
    ++g_zVideoRenderFrameFlushQuadCount;
}

void RECOIL_FASTCALL CaptureClearZBufferRect(zVidRect32 *rect) {
    const int index = g_zVideoRenderFrameClearRectCount;
    if (index < 4) {
        g_zVideoRenderFrameClearRects[index] = *rect;
    }
    ++g_zVideoRenderFrameClearRectCount;
}

int RECOIL_FASTCALL CaptureLockSurfaceState(zVideo_SurfaceStatePartial *surfaceState) {
    ++g_zVideoTestLockSurfaceCount;
    g_zVideoTestLockSurfaceState = surfaceState;
    surfaceState->locked = 1;
    return 1;
}

int RECOIL_FASTCALL CaptureUnlockSurfaceState(zVideo_SurfaceStatePartial *surfaceState) {
    ++g_zVideoTestUnlockSurfaceCount;
    g_zVideoTestUnlockSurfaceState = surfaceState;
    surfaceState->locked = 0;
    return 0x6a5;
}

template <typename Fn> unsigned int FunctionPointerBits(Fn functionPointer) {
    static_assert(sizeof(functionPointer) <= sizeof(std::uintptr_t));
    std::uintptr_t address = 0;
    std::memcpy(&address, &functionPointer, sizeof(functionPointer));
    return static_cast<unsigned int>(address);
}

bool NearFloat(float lhs, float rhs) {
    float delta = lhs - rhs;
    if (delta < 0.0f) {
        delta = -delta;
    }
    return delta <= 0.0015f;
}

int FloatBits(float value) {
    int bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

struct TestFxPass3UpdateElement {
    HudUiElement base;

    void RECOIL_THISCALL Update(float deltaSeconds) {
        const int index = g_fxPass3UpdateCount;
        if (index < 4) {
            g_fxPass3UpdateDelta[index] = deltaSeconds;
        }
        ++g_fxPass3UpdateCount;
    }
};

template <typename Method> std::uintptr_t MethodAddress(Method method) {
    static_assert(sizeof(method) <= sizeof(std::uintptr_t));
    std::uintptr_t address = 0;
    std::memcpy(&address, &method, sizeof(method));
    return address;
}
} // namespace

extern "C" int directdraw_enumerate_import_provider_smoke(void) {
    HMODULE ddrawModule = LoadLibraryA("ddraw.dll");
    if (ddrawModule == 0) {
        return 1;
    }

    FARPROC enumerateProc = GetProcAddress(ddrawModule, "DirectDrawEnumerateA");
    FreeLibrary(ddrawModule);
    return enumerateProc != 0 ? 0 : 2;
}

extern "C" int zvid_pack_color_rgb_smoke(void) {
    g_zVideo_PixelPack_RMaskShifted = 0xf8;
    g_zVideo_PixelPack_GMaskShifted = 0xfc;
    g_zVideo_PixelPack_RShift = 8;
    g_zVideo_PixelPack_GShift = 3;
    g_zVideo_PixelPack_BShiftTo8 = 3;

    const std::uint32_t packed565 = zVid_PackColorRGB(0xff, 0x80, 0x20);
    const std::uint32_t packed00RRGGBB = zVid_PackColor00RRGGBB(0x00123456);
    zVideo_ColorRgbFloat color{255.0f, 128.0f, 32.0f};
    const std::uint16_t packedFloats = zVid_PackColorRgbFloats(&color);
    zVideo_SetClearColorPacked16(0xabcd);
    if (packed565 != 0xfc04 || packed00RRGGBB != 0x51a2 || packedFloats != 0xfc04 ||
        g_zVideo_ClearColorPacked16 != 0xabcd) {
        return 1;
    }

    zVideo_ColorRgbFloat fogTarget{0.25f, 1.0f, 0.5f};
    g_zVideo_RendererType = 1;
    g_zVideo_D3DColorNormalizeChannelIndex = -1;
    zVideo_SetPendingFogTargetColorFromRgb01(&fogTarget);
    if (g_zVideo_D3DColorAttrBiasR != 63.75f || g_zVideo_D3DColorAttrBiasG != 255.0f ||
        g_zVideo_D3DColorAttrBiasB != 127.5f || g_zVideo_D3DColorNormalizeChannelIndex != 1) {
        return 2;
    }

    fogTarget = {0.5f, 0.25f, 1.0f};
    g_zVideo_RendererType = 0;
    g_zVideo_D3DColorNormalizeChannelIndex = 7;
    zVideo_SetPendingFogTargetColorFromRgb01(&fogTarget);
    return g_zVideo_D3DColorAttrBiasR == 127.5f && g_zVideo_D3DColorAttrBiasG == 63.75f &&
                   g_zVideo_D3DColorAttrBiasB == 255.0f &&
                   g_zVideo_D3DColorNormalizeChannelIndex == 7
               ? 0
               : 3;
}

extern "C" int zvideo_pixel_pack_setup_smoke(void) {
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);

    if (g_zVideo_PixelPack_RBits != 5 || g_zVideo_PixelPack_GBits != 6 ||
        g_zVideo_PixelPack_BBits != 5 || g_zVideo_PixelPack_RShift != 8 ||
        g_zVideo_PixelPack_GShift != 3 || g_zVideo_PixelPack_BShiftTo8 != 3 ||
        g_zVideo_PixelPack_RMaskShifted != 0xf8 || g_zVideo_PixelPack_GMaskShifted != 0xfc ||
        g_zVideo_PixelPack_BMaskShifted != 0xf8) {
        return 1;
    }

    zVideo::TexturePixelPack_SetupFromMasks(4, 4, 4, 4, 0xf000, 0x0f00, 0x00f0, 0x000f);
    return g_zVideo_TexturePixelPack_RBits == 4 && g_zVideo_TexturePixelPack_GBits == 4 &&
                   g_zVideo_TexturePixelPack_BBits == 4 && g_zVideo_TexturePixelPack_ABits == 4 &&
                   g_zVideo_TexturePixelPack_RGBBitsTotal == 12 &&
                   g_zVideo_TexturePixelPack_RGBBitsTotalMinus8 == 4 &&
                   g_zVideo_TexturePixelPack_GBBitsTotalMinus8 == 0 &&
                   g_zVideo_TexturePixelPack_BShiftTo8 == 4 &&
                   g_zVideo_TexturePixelPack_RMaskShifted == 0xf0 &&
                   g_zVideo_TexturePixelPack_GMaskShifted == 0xf0 &&
                   g_zVideo_TexturePixelPack_BMaskShifted == 0xf0 &&
                   g_zVideo_TexturePixelPack_NonRgbMaskShifted == ~0xf0
               ? 0
               : 2;
}

extern "C" int zvideo_pixel_pack_getters_smoke(void) {
    zVideo::PixelPack_SetupFromMasks(5, 5, 5, 0x7c00, 0x03e0, 0x001f);

    std::int32_t rBits = 0;
    std::int32_t gBits = 0;
    std::int32_t bBits = 0;
    std::uint32_t rMask = 0;
    std::uint32_t gMask = 0;
    std::uint32_t bMask = 0;
    std::int32_t packedBase = 0;
    std::int32_t sumMinus8 = 0;
    std::int32_t bShiftTo8 = 0;

    zVideo::PixelPack_GetRgbBits(&rBits, &gBits, &bBits);
    zVideo::PixelPack_GetRgbMasks(&rMask, &gMask, &bMask);
    zVideo::PixelPack_GetPackingParams(&packedBase, &sumMinus8, &bShiftTo8);

    return rBits == 5 && gBits == 5 && bBits == 5 && rMask == 0x7c00 && gMask == 0x03e0 &&
                   bMask == 0x001f && packedBase == 7 && sumMinus8 == 2 && bShiftTo8 == 3
               ? 0
               : 1;
}

extern "C" int zvideo_set_renderer_type_smoke(void) {
    g_zVideo_RendererType = 7;
    g_zVideo_ActiveRendererPath = 3;

    const std::int32_t previous = zVideo::SetRendererTypeAndActivePath(2);

    return previous == 7 && g_zVideo_RendererType == 2 && g_zVideo_ActiveRendererPath == 2 ? 0 : 1;
}

extern "C" int zvideo_pending_dither_enable_smoke(void) {
    g_zVideo_PendingDitherEnable = -1;
    zVideo_dd3d::SetPendingDitherEnable(0);
    if (g_zVideo_PendingDitherEnable != 0) {
        return 1;
    }

    zVideo_dd3d::SetPendingDitherEnable(1);
    return g_zVideo_PendingDitherEnable == 1 ? 0 : 2;
}

extern "C" int zvideo_pending_wireframe_state_smoke(void) {
    const int savedPendingWireframeState = g_zVideo_PendingWireframeState;

    zVideo_dd3d::SetPendingWireframeState(0);
    if (g_zVideo_PendingWireframeState != 0) {
        g_zVideo_PendingWireframeState = savedPendingWireframeState;
        return 1;
    }

    zVideo_dd3d::SetPendingWireframeState(-3);
    const bool signedValueOk = g_zVideo_PendingWireframeState == -3;

    g_zVideo_PendingWireframeState = savedPendingWireframeState;
    return signedValueOk ? 0 : 2;
}

extern "C" int zvideo_dd3d_begin_scene_flush_pending_smoke(void) {
    FakeD3DDevice2Object fakeDevice = {};
    IDirect3DDevice2 *const oldDevice = g_zVideo_pD3DDevice;
    const int oldPendingWireframe = g_zVideo_PendingWireframeState;
    const int oldPendingDither = g_zVideo_PendingDitherEnable;
    const int oldSceneDepth = g_zVideo_D3DSceneDepth;

    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_PendingWireframeState = 0;
    g_zVideo_PendingDitherEnable = 1;

    const int firstResult = zVideo_dd3d::BeginSceneAndFlushPendingRenderStates();
    const bool solidFlushOk =
        firstResult == 0 && gFakeD3DBeginSceneCalls == 1 && gFakeD3DSetRenderStateCalls == 2 &&
        gFakeD3DRenderStates[0] == D3DRENDERSTATE_FILLMODE &&
        gFakeD3DRenderStateValues[0] == D3DFILL_SOLID &&
        gFakeD3DRenderStates[1] == D3DRENDERSTATE_DITHERENABLE &&
        gFakeD3DRenderStateValues[1] == 1 && g_zVideo_PendingWireframeState == -1 &&
        g_zVideo_PendingDitherEnable == -1;

    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_PendingWireframeState = 1;
    g_zVideo_PendingDitherEnable = -1;
    zVideo_dd3d::BeginSceneAndFlushPendingRenderStates();
    const bool wireframeFlushOk =
        gFakeD3DBeginSceneCalls == 1 && gFakeD3DSetRenderStateCalls == 1 &&
        gFakeD3DRenderStates[0] == D3DRENDERSTATE_FILLMODE &&
        gFakeD3DRenderStateValues[0] == D3DFILL_WIREFRAME &&
        g_zVideo_PendingWireframeState == -1 && g_zVideo_PendingDitherEnable == -1;

    InstallFakeD3DDevice2(fakeDevice);
    gFakeD3DBeginSceneResult = static_cast<HRESULT>(DDERR_INVALIDPARAMS);
    g_zVideo_PendingWireframeState = 0;
    g_zVideo_PendingDitherEnable = 0;
    const int errorResult = zVideo_dd3d::BeginSceneAndFlushPendingRenderStates();
    const bool failureLeavesPendingOk = errorResult == -1 && gFakeD3DBeginSceneCalls == 1 &&
                                        gFakeD3DSetRenderStateCalls == 0 &&
                                        g_zVideo_PendingWireframeState == 0 &&
                                        g_zVideo_PendingDitherEnable == 0;

    InstallFakeD3DDevice2(fakeDevice);
    const int endResult = zVideo_dd3d::EndScene();
    gFakeD3DEndSceneResult = static_cast<HRESULT>(DDERR_INVALIDPARAMS);
    const int endErrorResult = zVideo_dd3d::EndScene();
    const bool endSceneOk = endResult == 0 && endErrorResult == -1 && gFakeD3DEndSceneCalls == 2;

    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_PendingWireframeState = -1;
    g_zVideo_PendingDitherEnable = -1;
    g_zVideo_D3DSceneDepth = 0;
    const int enterResult = zVideoD3D::SceneEnter();
    const int enterAgainResult = zVideoD3D::SceneEnter();
    const bool enterDepthOk = enterResult == 0 && enterAgainResult == 0 &&
                              g_zVideo_D3DSceneDepth == 1 && gFakeD3DBeginSceneCalls == 1;

    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_D3DSceneDepth = 2;
    const int leaveNestedResult = zVideoD3D::SceneLeave();
    const bool leaveNestedOk = leaveNestedResult == 0 && g_zVideo_D3DSceneDepth == 1 &&
                               gFakeD3DEndSceneCalls == 0;
    const int leaveFinalResult = zVideoD3D::SceneLeave();
    const int leaveIdleResult = zVideoD3D::SceneLeave();
    const bool leaveDepthOk = leaveFinalResult == 0 && leaveIdleResult == 0 &&
                              g_zVideo_D3DSceneDepth == 0 && gFakeD3DEndSceneCalls == 1;

    g_zVideo_pD3DDevice = oldDevice;
    g_zVideo_PendingWireframeState = oldPendingWireframe;
    g_zVideo_PendingDitherEnable = oldPendingDither;
    g_zVideo_D3DSceneDepth = oldSceneDepth;

    return solidFlushOk && wireframeFlushOk && failureLeavesPendingOk && endSceneOk &&
                   enterDepthOk && leaveNestedOk && leaveDepthOk
               ? 0
               : 1;
}

extern "C" int zvideo_sw_render_frame_smoke(void) {
    zClass_CameraDataPartial *const savedViewContext = g_zVideo_pActiveViewContext;
    const int savedActiveRendererPath = g_zVideo_ActiveRendererPath;
    const int savedSceneDepth = g_zVideo_D3DSceneDepth;
    const unsigned int savedFlushSorted = g_zVideo_pfnFlushSortedPolys;
    const unsigned int savedFlushOverwrite = g_zVideo_pfnFlushOverwritePolys;
    const unsigned int savedFlushQuad = g_zVideo_pfnFlushQuadBatch;
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
    g_zVideo_pfnFlushSortedPolys = FunctionPointerBits(CaptureFlushSortedPolys);
    g_zVideo_pfnFlushOverwritePolys = FunctionPointerBits(CaptureFlushOverwritePolys);
    g_zVideo_pfnFlushQuadBatch = FunctionPointerBits(CaptureFlushQuadBatch);
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

extern "C" int zclass_camera_render_scene_smoke(void) {
    zClass_CameraDataPartial *const savedViewContext = g_zVideo_pActiveViewContext;
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
    const int savedLensFlareVisibilityActive = zRndr::g_lensFlareVisibilityActive;
    const int savedTransparentQueueCount = zRndr::g_transparentQueueCount;
    const int savedOverwriteQueueCount = zRndr::g_overwriteQueueCount;
    const int savedOverlayEnabled = zRndr::g_overlayBlendEnabled;
    const int savedSpanOccluderPolyCount = zRndr::g_spanOccluderPolyCount;
    zRndr::SpanNodePartial **const savedSpanColumnHeadTable = zRndr::g_spanColumnHeadTable;
    zRndr::SpanNodePartial *const savedSpanPoolBase = zRndr::g_spanPoolBase;
    zRndr::SpanNodePartial *const savedSpanAllocCursor = zRndr::g_spanAllocCursor;
    const int savedSpanColumnCount = zRndr::g_spanColumnCount;
    const int savedSpanColumnCountPadded = zRndr::g_spanColumnCountPadded;
    zRndr::SpanBuildProc const savedBuildSpanList = zRndr::g_pfnBuildSpanList;
    zRndr::SpanBuildProc const savedBuildSpanListSecondary = zRndr::g_pfnBuildSpanListSecondary;
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

    static zRndr::SpanNodePartial *spanHeads[192];
    static zRndr::SpanNodePartial spanPool[64 * 256];
    std::memset(spanHeads, 0, sizeof(spanHeads));
    std::memset(spanPool, 0, sizeof(spanPool));
    zRndr::g_spanColumnCount = 64;
    zRndr::g_spanColumnCountPadded = 192;
    zRndr::g_spanColumnHeadTable = spanHeads;
    zRndr::g_spanPoolBase = spanPool;
    zRndr::g_spanAllocCursor = spanPool;
    zRndr::g_spanOccluderPolyCount = 5;
    zRndr::g_pfnBuildSpanList = zRndr_SpanOcclusion_InsertSpanNode_Local;
    zRndr::g_pfnBuildSpanListSecondary = zRndr_SpanOcclusion_BuildSpanList;
    zRndr::g_transparentQueueCount = 0;
    zRndr::g_overwriteQueueCount = 0;
    zRndr::g_overlayBlendEnabled = 0;
    zRndr::g_lensFlareSampleQueueCount = 0;
    zRndr::g_lensFlareVisibleSampleCount = 0;
    zRndr::g_lensFlareVisibilityActive = 0;
    zClass_TypeList::Head(8) = &cameraListA;
    zClass_TypeList::Tail(8) = &cameraListB;
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
    if (zClass_Camera::RenderScene(&camera, 0) != 0) {
        status = 1;
    } else if (g_zVideo_pActiveViewContext != &cameraData ||
               g_zClass_LodDistanceStateStackTop != 0 ||
               g_zClass_CameraAutoClipDistanceScale != 0.8f ||
               cameraData.clipDistance != 0.8f) {
        status = 2;
    } else if (g_Variant_CurrentTag.count != 2 || g_Variant_CurrentTag.tags[0] != 0x44 ||
               g_Variant_CurrentTag.tags[1] != 0x55 ||
               g_zVideo_ActiveViewVariantTag.count != 2 ||
               g_zVideo_ActiveViewVariantTag.tags[0] != 0x44 ||
               g_zVideo_ActiveViewVariantTag.tags[1] != 0x55) {
        status = 3;
    } else if (zRndr::g_spanOccluderPolyCount != 1 ||
               zRndr::g_transparentQueueCount != 0 ||
               zRndr::g_overwriteQueueCount != 0 ||
               zRndr::g_lensFlareSampleQueueCount != 0 ||
               zRndr::g_lensFlareVisibleSampleCount != 0) {
        status = 4;
    }

    g_zVideo_pActiveViewContext = savedViewContext;
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
    zRndr::g_lensFlareVisibilityActive = savedLensFlareVisibilityActive;
    zRndr::g_transparentQueueCount = savedTransparentQueueCount;
    zRndr::g_overwriteQueueCount = savedOverwriteQueueCount;
    zRndr::g_overlayBlendEnabled = savedOverlayEnabled;
    zRndr::g_spanOccluderPolyCount = savedSpanOccluderPolyCount;
    zRndr::g_spanColumnHeadTable = savedSpanColumnHeadTable;
    zRndr::g_spanPoolBase = savedSpanPoolBase;
    zRndr::g_spanAllocCursor = savedSpanAllocCursor;
    zRndr::g_spanColumnCount = savedSpanColumnCount;
    zRndr::g_spanColumnCountPadded = savedSpanColumnCountPadded;
    zRndr::g_pfnBuildSpanList = savedBuildSpanList;
    zRndr::g_pfnBuildSpanListSecondary = savedBuildSpanListSecondary;
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

extern "C" int zclass_list_render_active_cameras_smoke(void) {
    zClass_TypeListLink *const savedTypeListHead8 = zClass_TypeList::Head(8);
    zClass_TypeListLink *const savedTypeListTail8 = zClass_TypeList::Tail(8);
    zClass_CameraDataPartial *const savedViewContext = g_zVideo_pActiveViewContext;
    const int savedActiveRendererPath = g_zVideo_ActiveRendererPath;
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
    const int savedLensFlareVisibilityActive = zRndr::g_lensFlareVisibilityActive;
    const int savedTransparentQueueCount = zRndr::g_transparentQueueCount;
    const int savedOverwriteQueueCount = zRndr::g_overwriteQueueCount;
    const int savedOverlayEnabled = zRndr::g_overlayBlendEnabled;
    const int savedSpanOccluderPolyCount = zRndr::g_spanOccluderPolyCount;
    zRndr::SpanNodePartial **const savedSpanColumnHeadTable = zRndr::g_spanColumnHeadTable;
    zRndr::SpanNodePartial *const savedSpanPoolBase = zRndr::g_spanPoolBase;
    zRndr::SpanNodePartial *const savedSpanAllocCursor = zRndr::g_spanAllocCursor;
    const int savedSpanColumnCount = zRndr::g_spanColumnCount;
    const int savedSpanColumnCountPadded = zRndr::g_spanColumnCountPadded;
    zRndr::SpanBuildProc const savedBuildSpanList = zRndr::g_pfnBuildSpanList;
    zRndr::SpanBuildProc const savedBuildSpanListSecondary = zRndr::g_pfnBuildSpanListSecondary;
    int *const savedClipStackTop = gModel_ClipMaskStackTop;
    const int savedClipStack0 = gModel_ClipMaskStack[0];
    const int savedObjectHseTestEnabled = g_zClass_ObjectHseTestEnabled;
    const int savedFogEnabled = zModel_Fog_IsEnabled();
    zVec3 *const savedPolygonVertices = g_zModel_PointInPolygonVertices;
    zVec3 *const savedPolygonNormals = g_zModel_PointInPolygonEdgeNormals;
    const int savedPolygonVertexCount = g_zModel_PointInPolygonVertexCount;
    int *const savedMatrixIdentitySlot = zMath::g_currentMatrixIdentityFlagSlot;
    float **const savedMatrixPtrSlot = zMath::g_currentMatrixPtrSlot;

    int status = 0;
    zClass_TypeList::Head(8) = nullptr;
    zClass_TypeList::Tail(8) = nullptr;
    if (zClass_List::RenderActiveCameras() != 1) {
        status = 1;
    }

    zClass_NodePartial inactiveCamera{};
    inactiveCamera.flags = 0;
    zClass_TypeListLink inactiveLink{};
    inactiveLink.node = &inactiveCamera;
    zClass_TypeList::Head(8) = &inactiveLink;
    zClass_TypeList::Tail(8) = &inactiveLink;
    if (status == 0 && zClass_List::RenderActiveCameras() != 0) {
        status = 2;
    }

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
    camera.flags = 4;
    camera.classData = &cameraData;

    zClass_TypeListLink activeLink{};
    activeLink.node = &camera;
    zClass_TypeList::Head(8) = &activeLink;
    zClass_TypeList::Tail(8) = &activeLink;

    g_zVideo_ActiveRendererPath = 0;
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
    zRndr::g_spanOccluderPolyCount = 0;
    zRndr::g_spanColumnHeadTable = nullptr;
    zRndr::g_spanPoolBase = nullptr;
    zRndr::g_spanAllocCursor = nullptr;
    zRndr::g_spanColumnCount = 0;
    zRndr::g_spanColumnCountPadded = 0;
    zRndr::g_pfnBuildSpanList = zRndr_SpanOcclusion_InsertSpanNode_Local;
    zRndr::g_pfnBuildSpanListSecondary = zRndr_SpanOcclusion_BuildSpanList;
    zRndr::g_transparentQueueCount = 0;
    zRndr::g_overwriteQueueCount = 0;
    zRndr::g_overlayBlendEnabled = 0;
    zRndr::g_lensFlareSampleQueueCount = 0;
    zRndr::g_lensFlareVisibleSampleCount = 0;
    zRndr::g_lensFlareVisibilityActive = 0;
    gModel_ClipMaskStackTop = gModel_ClipMaskStack;
    gModel_ClipMaskStack[0] = 0;
    g_zClass_ObjectHseTestEnabled = 0;
    zModel_Fog_SetEnabled(0);
    zVec3 polygonVertices[8] = {};
    zVec3 polygonNormals[8] = {};
    g_zModel_PointInPolygonVertices = polygonVertices;
    g_zModel_PointInPolygonEdgeNormals = polygonNormals;
    g_zModel_PointInPolygonVertexCount = 0;

    if (status == 0 && zClass_List::RenderActiveCameras() != 0) {
        status = 3;
    } else if (status == 0 &&
               (g_zVideo_pActiveViewContext != &cameraData ||
                g_zClass_LodDistanceStateStackTop != 0 ||
                g_zClass_CameraAutoClipDistanceScale != 0.8f ||
                cameraData.clipDistance != 0.8f)) {
        status = 4;
    } else if (status == 0 &&
               (g_Variant_CurrentTag.count != 2 || g_Variant_CurrentTag.tags[0] != 0x44 ||
                g_Variant_CurrentTag.tags[1] != 0x55 ||
                g_zVideo_ActiveViewVariantTag.count != 2 ||
                g_zVideo_ActiveViewVariantTag.tags[0] != 0x44 ||
                g_zVideo_ActiveViewVariantTag.tags[1] != 0x55)) {
        status = 5;
    }

    zClass_TypeList::Head(8) = savedTypeListHead8;
    zClass_TypeList::Tail(8) = savedTypeListTail8;
    g_zVideo_pActiveViewContext = savedViewContext;
    g_zVideo_ActiveRendererPath = savedActiveRendererPath;
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
    zRndr::g_lensFlareVisibilityActive = savedLensFlareVisibilityActive;
    zRndr::g_transparentQueueCount = savedTransparentQueueCount;
    zRndr::g_overwriteQueueCount = savedOverwriteQueueCount;
    zRndr::g_overlayBlendEnabled = savedOverlayEnabled;
    zRndr::g_spanOccluderPolyCount = savedSpanOccluderPolyCount;
    zRndr::g_spanColumnHeadTable = savedSpanColumnHeadTable;
    zRndr::g_spanPoolBase = savedSpanPoolBase;
    zRndr::g_spanAllocCursor = savedSpanAllocCursor;
    zRndr::g_spanColumnCount = savedSpanColumnCount;
    zRndr::g_spanColumnCountPadded = savedSpanColumnCountPadded;
    zRndr::g_pfnBuildSpanList = savedBuildSpanList;
    zRndr::g_pfnBuildSpanListSecondary = savedBuildSpanListSecondary;
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

extern "C" int zvideo_surface_accessors_smoke(void) {
    int swPixels = 0x1234;
    int primaryPixels = 0x5678;
    g_zVideo_SwSurfaceState.width = 320;
    g_zVideo_SwSurfaceState.height = 200;
    g_zVideo_SwSurfaceState.pitch = 640;
    g_zVideo_SwSurfaceState.locked = 1;
    g_zVideo_SwSurfaceState.pixels = &swPixels;
    g_zVideo_PrimarySurfaceState.width = 800;
    g_zVideo_PrimarySurfaceState.height = 600;
    g_zVideo_PrimarySurfaceState.pitch = 1600;
    g_zVideo_PrimarySurfaceState.pixels = &primaryPixels;

    return zVideo::GetSwSurfacePixels() == &swPixels && zVideo::GetSwSurfaceWidth() == 320 &&
                   zVideo::GetSwSurfaceHeight() == 200 && zVideo::GetSwSurfacePitch() == 640 &&
                   zVideo::GetSwSurfaceLockedFlag() == 1 &&
                   zVideo::GetPrimarySurfacePixels() == &primaryPixels &&
                   zVideo::GetPrimarySurfaceWidth() == 800 &&
                   zVideo::GetPrimarySurfaceHeight() == 600 &&
                   zVideo::GetPrimarySurfacePitch() == 1600
               ? 0
               : 1;
}

extern "C" int zvideo_fxpass3_local_queue_smoke(void) {
    unsigned char savedConfig[kFxPass3ConfigSize] = {};
    std::memcpy(savedConfig, FxPass3ConfigBytes(), sizeof(savedConfig));
    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;
    g_HudUi_InvalidateMask = 0;

    std::memset(FxPass3ConfigBytes(), 0, kFxPass3ConfigSize);

    HudUiElement *const rootElement =
        reinterpret_cast<HudUiElement *>(FxPass3ConfigBytes() + kFxPass3RootElementOffset);
    rootElement->ftable = &g_HudUiCommon_FTable;
    rootElement->flags = 0x10;
    rootElement->timer = 9.0f;

    zVideo::FxPass3_SetPrimaryElementParamsLocal(0x12345678u, 0.625);
    const bool rootWrapperOk =
        FxPass3FieldAt<unsigned short>(kFxPass3RootPackedColorOffset) == 0x5678u &&
        FxPass3FieldAt<double>(kFxPass3RootAlphaOffset) == 0.625 &&
        (rootElement->flags & 0x11u) == 0x01u && rootElement->timer == 0.0f;

    std::memset(FxPass3ConfigBytes(), 0, kFxPass3ConfigSize);
    zVideo::zVideoFxPass3Config_SetPrimaryElementParamsLocal(&g_zVideo_FxPass3ConfigLocal,
                                                        0x0000abcdu, 1.25);
    const bool rootConfigOk =
        FxPass3FieldAt<unsigned short>(kFxPass3RootPackedColorOffset) == 0xabcdu &&
        FxPass3FieldAt<double>(kFxPass3RootAlphaOffset) == 1.25 &&
        (FxPass3FieldAt<unsigned int>(kFxPass3RootElementOffset + offsetof(HudUiElement, flags)) &
         0x01u) != 0;

    std::memset(FxPass3ConfigBytes(), 0, kFxPass3ConfigSize);
    HudUiRect inputRect0 = {1, 2, 3, 4};
    HudUiRect inputRect1 = {5, 6, 7, 8};
    HudUiRect ignoredInputRect = {9, 10, 11, 12};
    zVideo::FxPass3_SetInputRectByIndex(0, &inputRect0);
    zVideo::FxPass3_SetInputRectByIndex(1, &inputRect1);
    zVideo::FxPass3_SetInputRectByIndex(2, &ignoredInputRect);
    const bool inputRectWrapperOk =
        FxPass3FieldAt<HudUiRect *>(kFxPass3InputRect0Offset) == &inputRect0 &&
        FxPass3FieldAt<HudUiRect *>(kFxPass3InputRect1Offset) == &inputRect1;

    std::memset(FxPass3ConfigBytes(), 0, kFxPass3ConfigSize);
    unsigned char *const slot0Bytes = FxPass3ConfigBytes() + kFxPass3SlotsOffset;
    HudUiElement *const slot0Element = reinterpret_cast<HudUiElement *>(slot0Bytes);
    slot0Element->ftable = &g_HudUiCommon_FTable;
    slot0Element->timer = 4.0f;

    zVideo::FxPass3_QueueElementLocal(11, 22, 33, 44, 55, 1.5f, 2.5f);
    const bool queueWrapperOk =
        FxPass3FieldAt<int>(kFxPass3SlotWriteIndexOffset) == 1 && slot0Element->x == 11 &&
        slot0Element->y == 22 && slot0Element->timer == 0.0f &&
        (slot0Element->flags & 0x01u) != 0 &&
        FxPass3FieldAt<float>(kFxPass3SlotsOffset + kFxPass3SlotCurrentRadiusOffset) == 33.0f &&
        FxPass3FieldAt<float>(kFxPass3SlotsOffset + kFxPass3SlotMaxRadiusOffset) == 44.0f &&
        FxPass3FieldAt<float>(kFxPass3SlotsOffset + kFxPass3SlotExtentOffset) == 55.0f &&
        FxPass3FieldAt<float>(kFxPass3SlotsOffset + kFxPass3SlotSinFreqOffset) == 1.5f &&
        FxPass3FieldAt<float>(kFxPass3SlotsOffset + kFxPass3SlotSinPhaseOffset) == 2.5f;

    std::memset(FxPass3ConfigBytes(), 0, kFxPass3ConfigSize);
    FxPass3FieldAt<int>(kFxPass3SlotWriteIndexOffset) = 4;
    const std::size_t slot4Offset = kFxPass3SlotsOffset + kFxPass3SlotSize * 4;
    HudUiElement *const slot4Element =
        reinterpret_cast<HudUiElement *>(FxPass3ConfigBytes() + slot4Offset);
    slot4Element->ftable = &g_HudUiCommon_FTable;
    zVideo::zVideoFxPass3Config_QueueElementLocal(&g_zVideo_FxPass3ConfigLocal, 1, 2, 3, 4, 5, 6.0f,
                                             7.0f);
    const bool queueCapOk = FxPass3FieldAt<int>(kFxPass3SlotWriteIndexOffset) == 4 &&
                            slot4Element->x == 1 && slot4Element->y == 2 &&
                            FxPass3FieldAt<float>(slot4Offset + kFxPass3SlotSinPhaseOffset) ==
                                7.0f;

    std::memset(FxPass3ConfigBytes(), 0, kFxPass3ConfigSize);
    HudUiCommon_FTable updateTable{};
    updateTable.slots[9] = MethodAddress(&TestFxPass3UpdateElement::Update);
    TestFxPass3UpdateElement updateA{};
    TestFxPass3UpdateElement updateB{};
    updateA.base.ftable = &updateTable;
    updateA.base.next = &updateB.base;
    updateB.base.ftable = &updateTable;
    HudUiContainer *const container =
        reinterpret_cast<HudUiContainer *>(&g_zVideo_FxPass3ConfigLocal);
    container->enabled = 1;
    container->childHead = &updateA.base;
    container->childTail = &updateB.base;
    FxPass3FieldAt<int>(kFxPass3SlotWriteIndexOffset) = 3;
    g_fxPass3UpdateCount = 0;
    zVideo::zVideoFxPass3Config_UpdateLocal(&g_zVideo_FxPass3ConfigLocal, 0.75f);
    const bool updateConfigOk = g_fxPass3UpdateCount == 2 &&
                                g_fxPass3UpdateDelta[0] == 0.75f &&
                                g_fxPass3UpdateDelta[1] == 0.75f &&
                                FxPass3FieldAt<int>(kFxPass3SlotWriteIndexOffset) == 0;

    FxPass3FieldAt<int>(kFxPass3SlotWriteIndexOffset) = 2;
    g_fxPass3UpdateCount = 0;
    zVideo::FxPass3_UpdateLocal(0.5f);
    const bool updateWrapperOk = g_fxPass3UpdateCount == 2 &&
                                 g_fxPass3UpdateDelta[0] == 0.5f &&
                                 g_fxPass3UpdateDelta[1] == 0.5f &&
                                 FxPass3FieldAt<int>(kFxPass3SlotWriteIndexOffset) == 0;

    std::memcpy(FxPass3ConfigBytes(), savedConfig, sizeof(savedConfig));
    g_HudUi_InvalidateMask = oldInvalidateMask;

    return rootWrapperOk && rootConfigOk && inputRectWrapperOk && queueWrapperOk && queueCapOk &&
                   updateConfigOk && updateWrapperOk
               ? 0
               : 1;
}

extern "C" int zvideo_primary_surface_rect_scratch_smoke(void) {
    g_zVideo_PrimarySurfaceRectScratch = {11, 22, 33, 44};
    g_zVideo_PrimarySurfaceState.width = 640;
    g_zVideo_PrimarySurfaceState.height = 480;

    zVidRect32 *const scratch = zVideo::GetPrimarySurfaceRectScratch();
    return scratch == &g_zVideo_PrimarySurfaceRectScratch && scratch->left == 11 &&
                   scratch->top == 22 && scratch->right == 640 && scratch->bottom == 480
               ? 0
               : 1;
}

extern "C" int zvideo_run_postprocess_on_sw_buffer_smoke(void) {
    zVideo_SurfaceStateProc const savedLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    const zVideo_SurfaceStatePartial savedSwSurfaceState = g_zVideo_SwSurfaceState;
    void *const savedFrameBuffer = zRndr::g_frameBuffer;
    const int savedPitchBytes = zRndr::g_pitchBytes;
    const int savedBytesPerPixel = zRndr::g_bytesPerPixel;
    unsigned char savedConfig[kFxPass3ConfigSize] = {};
    std::memcpy(savedConfig, FxPass3ConfigBytes(), sizeof(savedConfig));
    unsigned short *const savedFxSurfacePixels = g_zVideo_FxSurfacePixels16;
    const int savedFxSurfaceWidth = g_zVideo_FxSurfaceWidth;
    const int savedFxSurfaceHeight = g_zVideo_FxSurfaceHeight;
    const int savedFxSurfacePitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int savedFxSurfacePitchPixels = g_zVideo_FxSurfacePitchPixels16;

    unsigned short pixels[16] = {};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.pixels = pixels;
    g_zVideo_SwSurfaceState.width = 4;
    g_zVideo_SwSurfaceState.height = 3;
    g_zVideo_SwSurfaceState.pitch = 8;
    g_zVideo_pfnLockSurfaceState = CaptureLockSurfaceState;
    g_zVideoTestLockSurfaceCount = 0;
    g_zVideoTestLockSurfaceState = nullptr;
    zRndr::g_frameBuffer = nullptr;
    zRndr::g_pitchBytes = 0;
    zRndr::g_bytesPerPixel = 7;
    std::memset(FxPass3ConfigBytes(), 0, kFxPass3ConfigSize);

    zVideo::RunPostprocessOnSwBuffer();

    const bool lockOk = g_zVideoTestLockSurfaceCount == 1 &&
                        g_zVideoTestLockSurfaceState == &g_zVideo_SwSurfaceState &&
                        g_zVideo_SwSurfaceState.locked == 1;
    const bool frameBufferOk =
        zRndr::g_frameBuffer == pixels && zRndr::g_pitchBytes == 8 &&
        zRndr::g_bytesPerPixel == 7;
    const bool fxSurfaceOk =
        g_zVideo_FxSurfacePixels16 == pixels && g_zVideo_FxSurfaceWidth == 4 &&
        g_zVideo_FxSurfaceHeight == 3 && g_zVideo_FxSurfacePitchBytes == 8 &&
        g_zVideo_FxSurfacePitchPixels16 == 4;
    const bool primitiveOk =
        FxPass3FieldAt<unsigned short *>(kFxPass3SurfacePixelsOffset) == pixels &&
        FxPass3FieldAt<int>(kFxPass3SurfaceWidthOffset) == 4 &&
        FxPass3FieldAt<int>(kFxPass3SurfaceHeightOffset) == 3 &&
        FxPass3FieldAt<int>(kFxPass3SurfacePitchOffset) == 8;

    g_zVideo_pfnLockSurfaceState = savedLockSurfaceState;
    g_zVideo_SwSurfaceState = savedSwSurfaceState;
    zRndr::g_frameBuffer = savedFrameBuffer;
    zRndr::g_pitchBytes = savedPitchBytes;
    zRndr::g_bytesPerPixel = savedBytesPerPixel;
    std::memcpy(FxPass3ConfigBytes(), savedConfig, sizeof(savedConfig));
    g_zVideo_FxSurfacePixels16 = savedFxSurfacePixels;
    g_zVideo_FxSurfaceWidth = savedFxSurfaceWidth;
    g_zVideo_FxSurfaceHeight = savedFxSurfaceHeight;
    g_zVideo_FxSurfacePitchBytes = savedFxSurfacePitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = savedFxSurfacePitchPixels;

    return lockOk && frameBufferOk && fxSurfaceOk && primitiveOk ? 0 : 1;
}

extern "C" int zvideo_frame_scratch_buffers_smoke(void) {
    std::free(g_zVid_NoiseByteTable);
    std::free(g_zVideo_FxPass3_ScratchPixels16);
    g_zVid_NoiseByteTable = nullptr;
    g_zVideo_FxPass3_ScratchPixels16 = nullptr;

    g_zVideo_PrimarySurfaceState.width = 4;
    g_zVideo_PrimarySurfaceState.height = 3;
    std::uint16_t staleFxPixel = 0;
    g_zVideo_FxSurfacePixels16 = &staleFxPixel;
    g_zVideo_FxSurfaceWidth = 1;
    g_zVideo_FxSurfaceHeight = 1;
    g_zVideo_FxSurfacePitchBytes = 2;
    g_zVideo_FxSurfacePitchPixels16 = 1;

    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);
    zRndr::g_bytesPerPixel = 1;
    zRndr::g_defaultGraphicsFlags = 8;
    zRndr::g_graphicsFlags = &zRndr::g_defaultGraphicsFlags;
    zRndr::g_pfnOverlayBlendRow = nullptr;

    zVid::InitFrameScratchBuffers();

    const bool result =
        g_zVid_NoiseByteTableSize == 100 && g_zVid_NoiseByteTable != nullptr &&
        g_zVideo_FxPass3_ScratchPixels16 != nullptr && g_zVideo_FxSurfacePixels16 == nullptr &&
        g_zVideo_FxSurfaceWidth == 0 && g_zVideo_FxSurfaceHeight == 0 &&
        g_zVideo_FxSurfacePitchBytes == 0 && g_zVideo_FxSurfacePitchPixels16 == 0 &&
        zRndr::g_pfnOverlayBlendRow == zRndr::OverlayBlendRow555_Scalar &&
        zRndr::g_pixelPackGreenBits == 6 && zRndr::g_perspectiveAdaptiveMinSpan == 0x10 &&
        zRndr::g_perspectiveAdaptiveMaxSpan == 0x40 && (zRndr::g_defaultGraphicsFlags & 4) == 0;

    std::free(g_zVid_NoiseByteTable);
    std::free(g_zVideo_FxPass3_ScratchPixels16);
    g_zVid_NoiseByteTable = nullptr;
    g_zVideo_FxPass3_ScratchPixels16 = nullptr;
    return result ? 0 : 1;
}

extern "C" int zvideo_draw_noise_rect_smoke(void) {
    unsigned char *const oldNoiseTable = g_zVid_NoiseByteTable;
    const int oldNoiseTableSize = g_zVid_NoiseByteTableSize;
    unsigned short *const oldFxPixels = g_zVideo_FxSurfacePixels16;
    const int oldFxWidth = g_zVideo_FxSurfaceWidth;
    const int oldFxHeight = g_zVideo_FxSurfaceHeight;
    const int oldFxPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldFxPitchPixels16 = g_zVideo_FxSurfacePitchPixels16;

    unsigned char noiseTable[32] = {};
    for (int i = 0; i < 32; ++i) {
        noiseTable[i] = static_cast<unsigned char>(i);
    }

    unsigned short pixels[25] = {};
    for (int i = 0; i < 25; ++i) {
        pixels[i] = 0xaaaa;
    }

    g_zVid_NoiseByteTable = noiseTable;
    g_zVid_NoiseByteTableSize = 32;
    g_zVideo_FxSurfacePixels16 = pixels;
    g_zVideo_FxSurfaceWidth = 5;
    g_zVideo_FxSurfaceHeight = 5;
    g_zVideo_FxSurfacePitchBytes = 10;
    g_zVideo_FxSurfacePitchPixels16 = 5;
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);

    zVidRect32 rect{1, 1, 4, 3};
    zVid::DrawNoiseRect(&rect, 0.0);
    bool lowIntensityOk = true;
    for (int i = 0; i < 25; ++i) {
        lowIntensityOk = lowIntensityOk && pixels[i] == 0xaaaa;
    }

    std::srand(7);
    const int rowWidth = rect.right - rect.left;
    const int firstOffset = (std::rand() * (g_zVid_NoiseByteTableSize - rowWidth)) / 0x7fff;
    const int secondOffset = (std::rand() * (g_zVid_NoiseByteTableSize - rowWidth)) / 0x7fff;
    std::srand(7);
    zVid::DrawNoiseRect(&rect, 1.0);

    const auto gray565 = [](unsigned char value) -> unsigned short {
        const unsigned short level = static_cast<unsigned short>(value & 0x1f);
        return static_cast<unsigned short>((level << 11) | (level << 6) | level);
    };

    const bool rowOneOk = pixels[1 + 1 * 5] == gray565(noiseTable[firstOffset]) &&
                          pixels[2 + 1 * 5] == gray565(noiseTable[firstOffset + 1]) &&
                          pixels[3 + 1 * 5] == gray565(noiseTable[firstOffset + 2]);
    const bool rowTwoOk = pixels[1 + 2 * 5] == gray565(noiseTable[secondOffset]) &&
                          pixels[2 + 2 * 5] == gray565(noiseTable[secondOffset + 1]) &&
                          pixels[3 + 2 * 5] == gray565(noiseTable[secondOffset + 2]);
    const bool untouchedOk = pixels[0] == 0xaaaa && pixels[4] == 0xaaaa &&
                             pixels[1 + 3 * 5] == 0xaaaa && pixels[24] == 0xaaaa;

    g_zVid_NoiseByteTable = oldNoiseTable;
    g_zVid_NoiseByteTableSize = oldNoiseTableSize;
    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxSurfaceWidth = oldFxWidth;
    g_zVideo_FxSurfaceHeight = oldFxHeight;
    g_zVideo_FxSurfacePitchBytes = oldFxPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldFxPitchPixels16;

    return lowIntensityOk && rowOneOk && rowTwoOk && untouchedOk ? 0 : 1;
}

namespace {
int g_clearSwCalls;
int g_clearPrimaryCalls;
int g_setVideoModeCalls;
std::int32_t g_setVideoModeResult = 0x1234;
zVidRect32 *g_lastClearSwSurfaceRect;
zVidRect32 *g_lastClearSwZRect;
zVidRect32 *g_lastClearPrimaryRect;
zVideo_SurfaceStatePartial *g_lastClearPrimaryState;

void RECOIL_FASTCALL ClearSwFake(zVidRect32 *surfaceRect, zVidRect32 *zRect) {
    ++g_clearSwCalls;
    g_lastClearSwSurfaceRect = surfaceRect;
    g_lastClearSwZRect = zRect;
}

void RECOIL_FASTCALL ClearStateFake(zVidRect32 *rect, zVideo_SurfaceStatePartial *surfaceState) {
    ++g_clearPrimaryCalls;
    g_lastClearPrimaryRect = rect;
    g_lastClearPrimaryState = surfaceState;
}

std::int32_t RECOIL_FASTCALL SetVideoModeFake(std::int32_t) {
    ++g_setVideoModeCalls;
    return g_setVideoModeResult;
}
} // namespace

extern "C" int zvideo_clear_dispatch_and_exchange_smoke(void) {
    zVidRect32 surfaceRect{1, 2, 3, 4};
    zVidRect32 zRect{5, 6, 7, 8};
    g_clearSwCalls = 0;
    g_clearPrimaryCalls = 0;
    g_zVideo_pfnClearSwSurfaceAndZBuffer = ClearSwFake;
    g_zVideo_pfnClearStateSurfaceAndZBuffer = ClearStateFake;
    g_zVideo_ClearScreenBufferEnabled = 3;

    zVideo::CallClearSwSurfaceAndZBuffer(&surfaceRect, &zRect);
    zVideo::CallClearPrimarySurfaceAndZBuffer(&surfaceRect);
    if (g_clearSwCalls != 1 || g_lastClearSwSurfaceRect != &surfaceRect ||
        g_lastClearSwZRect != &zRect || g_clearPrimaryCalls != 1 ||
        g_lastClearPrimaryRect != &surfaceRect ||
        g_lastClearPrimaryState != &g_zVideo_PrimarySurfaceState) {
        return 1;
    }

    const std::int32_t previous = zVideo::ExchangeClearScreenBufferEnabled(1);
    return previous == 3 && zVideo::GetClearScreenBufferEnabled() == 1 ? 0 : 2;
}

extern "C" int zvideo_mode_geometry_and_set_video_mode_smoke(void) {
    gVideo_resolutionMenuValid = 1;
    g_zVideo_DisplayModeBpp = 0;
    zVideo::Init_SetSurfaceGeometryFromModeIndex(3);
    if (g_zVideo_UseHalfResBackbuffer != 1 || g_zVideo_DisplayModeSurfaceState.width != 0x280 ||
        g_zVideo_DisplayModeSurfaceState.height != 0x1e0 ||
        g_zVideo_PrimarySurfaceState.width != 0x280 ||
        g_zVideo_PrimarySurfaceState.height != 0x1e0 || g_zVideo_SwSurfaceState.width != 0x140 ||
        g_zVideo_SwSurfaceState.height != 0x0f0 || zVideo::GetDisplayModeBpp() != 0x10) {
        return 1;
    }

    zVideo::Init_SetSurfaceGeometryFromModeIndex(7);
    if (g_zVideo_UseHalfResBackbuffer != 0 || g_zVideo_DisplayModeSurfaceState.width != 0x400 ||
        g_zVideo_DisplayModeSurfaceState.height != 0x300 ||
        g_zVideo_SwSurfaceState.width != 0x400 || g_zVideo_SwSurfaceState.height != 0x300) {
        return 2;
    }

    zVideo::Init_SetSurfaceGeometryFromModeIndex(8);
    if (gVideo_resolutionMenuValid != 0) {
        return 3;
    }

    g_setVideoModeCalls = 0;
    g_zVideo_pfnSetVideoMode = SetVideoModeFake;
    g_zVideo_IsInitialized = 0;
    if (zVideo::SetVideoMode(5) != 0x5a560000 || g_setVideoModeCalls != 0) {
        return 4;
    }

    g_zVideo_IsInitialized = 1;
    if (zVideo::SetVideoMode(5) != 0x1234 || g_setVideoModeCalls != 1 ||
        g_zVideo_DisplayModeSurfaceState.height != 0x1e0) {
        return 5;
    }

    if (zVideo::Init_ApplyModeIndex(4) != 0x1234 || g_setVideoModeCalls != 2 ||
        g_zVideo_DisplayModeSurfaceState.height != 0x190) {
        return 6;
    }

    g_zVideo_pfnSetVideoMode = nullptr;
    return 0;
}

extern "C" int zvideo_init_video_system_reentry_guard_smoke(void) {
    HWND oldHwnd = g_zVideo_hWnd;
    g_zVideo_IsInitialized = 1;
    g_zVideo_FrameTick = 77;

    const std::int32_t result = zVideo::InitVideoSystem(reinterpret_cast<HWND>(0x1234), 1, 1, 5);
    const bool ok = result == 0x5a560001 && g_zVideo_FrameTick == 77 && g_zVideo_hWnd == oldHwnd;

    g_zVideo_IsInitialized = 0;
    return ok ? 0 : 1;
}

extern "C" int zvideo_dispatch_wrappers_smoke(void) {
    zVideo::BindRendererDispatch(0, 1);
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.locked = 1;

    zVidRect32 rect{0, 0, 1, 1};
    zVideo_dd3d::CallClearZBufferRect(&rect);
    if (zVideo::Dispatch_LockDisplayModeSurfaceState() != 0) {
        return 1;
    }

    g_zVideo_DisplayModeSurfaceState.locked = 0;
    if (zVideo::Dispatch_UnlockDisplayModeSurfaceState() != 0) {
        return 2;
    }

    zVideo_SurfaceStateProc const savedUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;
    const zVideo_SurfaceStatePartial savedSwSurfaceState = g_zVideo_SwSurfaceState;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.locked = 1;
    g_zVideo_pfnUnlockSurfaceState = CaptureUnlockSurfaceState;
    g_zVideoTestUnlockSurfaceCount = 0;
    g_zVideoTestUnlockSurfaceState = nullptr;

    const int unlockResult = zVideo::Dispatch_UnlockSwSurfaceState();
    const bool unlockOk = unlockResult == 0x6a5 && g_zVideoTestUnlockSurfaceCount == 1 &&
                          g_zVideoTestUnlockSurfaceState == &g_zVideo_SwSurfaceState &&
                          g_zVideo_SwSurfaceState.locked == 0;

    g_zVideo_pfnUnlockSurfaceState = savedUnlockSurfaceState;
    g_zVideo_SwSurfaceState = savedSwSurfaceState;

    return unlockOk ? 0 : 3;
}

extern "C" int zvideo_bind_renderer_dispatch_smoke(void) {
    zVidHwApiDeviceRecordPartial selectedDevice{};
    selectedDevice.m_deviceFeatureFlags = 0x1234;
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;

    zVideo::BindRendererDispatch(1, 2);
    if (g_zVideo_RendererType != 1 || g_zVideo_ActiveRendererPath != 1 ||
        g_zVideo_FullscreenOption != 2 || g_zVideo_pfnOpenVideoMode != zVideo_dd::OpenVideoMode ||
        g_zVideo_pfnSetVideoMode != zVideo_dd::SetVideoMode ||
        g_zVideo_pfnCreateTextureRecord != zVideo_dd3d::CreateTextureRecord ||
        g_zVideo_pfnAdjustSurfaces == nullptr ||
        reinterpret_cast<std::uintptr_t>(g_zVideo_pfnBltSwToPrimaryRectDirect) != 0x004a7d90 ||
        selectedDevice.m_deviceFeatureFlags != 0) {
        return 1;
    }

    g_zVideo_pSelectedHwApiDeviceRecord = nullptr;
    zVideo::BindRendererDispatch(0, 1);
    return g_zVideo_RendererType == 0 && g_zVideo_ActiveRendererPath == 0 &&
                   g_zVideo_FullscreenOption == 1 && g_zVideo_pfnAdjustSurfaces != nullptr
               ? 0
               : 2;
}

namespace {
int g_adjustSurfaceCalls;
int g_bltPrimaryToSwDirectCalls;
int g_textureMemoryQueryCalls;
int g_deviceMemoryQueryCalls;
std::int32_t g_lastAdjustWaitForPresent;
std::int32_t g_lastAdjustBlitPrimaryToSwFirst;
std::int32_t g_lastTextureMemoryQueryFlags;
std::int32_t g_lastDeviceMemoryQueryFlags;

void RECOIL_FASTCALL BltPrimaryToSwDirectFake(zVidRect32 *srcRect, zVidRect32 *dstRect) {
    if (srcRect == nullptr && dstRect == nullptr) {
        ++g_bltPrimaryToSwDirectCalls;
    }
}

std::int32_t RECOIL_FASTCALL AdjustSurfacesFake(zVidRect32 *, zVidRect32 *,
                                                std::int32_t waitForPresent,
                                                std::int32_t blitPrimaryToSwFirst) {
    ++g_adjustSurfaceCalls;
    g_lastAdjustWaitForPresent = waitForPresent;
    g_lastAdjustBlitPrimaryToSwFirst = blitPrimaryToSwFirst;
    return 0x123;
}

std::int32_t RECOIL_FASTCALL TextureMemoryQueryFake(std::int32_t flags, std::int32_t *totalBytes,
                                                    std::int32_t *freeBytes) {
    ++g_textureMemoryQueryCalls;
    g_lastTextureMemoryQueryFlags = flags;
    *totalBytes = 0x1000;
    *freeBytes = 0x200;
    return 1;
}

std::int32_t RECOIL_FASTCALL DeviceMemoryQueryFake(std::int32_t flags, std::int32_t *totalBytes,
                                                   std::int32_t *freeBytes) {
    ++g_deviceMemoryQueryCalls;
    g_lastDeviceMemoryQueryFlags = flags;
    *totalBytes = 0x2000;
    *freeBytes = 0x300;
    return 1;
}
} // namespace

extern "C" int zvideo_set_half_res_adjust_mode_smoke(void) {
    g_bltPrimaryToSwDirectCalls = 0;
    g_zVideo_HalfResAdjustMode = 3;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_RendererType = 1;
    g_zVideo_pfnBltPrimaryToSwRectDirect = BltPrimaryToSwDirectFake;

    if (zVideo::SetHalfResAdjustMode(3) != 3 || g_zVideo_HalfResAdjustMode != 3 ||
        g_bltPrimaryToSwDirectCalls != 0) {
        return 1;
    }

    g_zVideo_UseHalfResBackbuffer = 1;
    if (zVideo::SetHalfResAdjustMode(1) != 0 || g_zVideo_HalfResAdjustMode != 3) {
        return 2;
    }

    g_zVideo_UseHalfResBackbuffer = 0;
    if (zVideo::SetHalfResAdjustMode(1) != 3 || g_zVideo_HalfResAdjustMode != 1) {
        return 3;
    }

    g_zVideo_RendererType = 0;
    return zVideo::SetHalfResAdjustMode(0) == 1 && g_zVideo_HalfResAdjustMode == 0 &&
                   g_bltPrimaryToSwDirectCalls == 1
               ? 0
               : 4;
}

extern "C" int zvideo_handle_software_mode_hotkey_smoke(void) {
    int videoMode = 2;
    int acceleration = 0;
    int hudTypeSw = 2;
    int hudTypeHw = 9;
    int replicate = 0;
    zOpt_ViewRectSection render{};
    zOpt_ViewRectSection display{};
    zOpt_ViewRectSection window{};
    zOpt_ViewRectSection *renderPtr = &render;
    zOpt_ViewRectSection *displayPtr = &display;
    zOpt_ViewRectSection *windowPtr = &window;

    int *const oldVideoMode = ZOPT_VIDEO_MODE;
    int *const oldAcceleration = ZOPT_VIDEO_ACCELERATION;
    int *const oldHudTypeSw = ZOPT_HUD_TYPE_SW;
    int *const oldHudTypeHw = ZOPT_HUD_TYPE_HW;
    int *const oldReplicate = ZOPT_REPLICATE;
    zOpt_ViewRectSection **const oldRenderSection = g_zOpt_RenderSectionOption;
    zOpt_ViewRectSection **const oldDisplaySection = g_zOpt_DisplaySectionOption;
    zOpt_ViewRectSection **const oldWindowSection = g_zOpt_WindowSectionOption;
    const int oldHwMode = g_zOpt_HwMode;
    const int oldLayoutsInitialized = g_HudUiMgrHudLayoutsInitialized;
    zVideo_StatusProc oldSetVideoMode = g_zVideo_pfnSetVideoMode;
    const int oldHotkeyEnabled = g_zVideo_SoftwareModeHotkeyEnabled;
    const int oldHalfRes = g_zVideo_HalfResAdjustMode;
    const int oldUseHalfResBackbuffer = g_zVideo_UseHalfResBackbuffer;
    const int oldRendererType = g_zVideo_RendererType;
    zVideo_BltRectDirectProc oldBltPrimaryToSw = g_zVideo_pfnBltPrimaryToSwRectDirect;

    ZOPT_VIDEO_MODE = &videoMode;
    ZOPT_VIDEO_ACCELERATION = &acceleration;
    ZOPT_HUD_TYPE_SW = &hudTypeSw;
    ZOPT_HUD_TYPE_HW = &hudTypeHw;
    ZOPT_REPLICATE = &replicate;
    g_zOpt_RenderSectionOption = &renderPtr;
    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_WindowSectionOption = &windowPtr;
    g_zOpt_HwMode = 0;
    g_HudUiMgrHudLayoutsInitialized = 0;
    g_zVideo_pfnSetVideoMode = SetVideoModeFake;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_RendererType = 0;
    g_zVideo_pfnBltPrimaryToSwRectDirect = BltPrimaryToSwDirectFake;

    g_zVideo_SoftwareModeHotkeyEnabled = 0;
    g_setVideoModeCalls = 0;
    zVideo::HandleSoftwareModeHotkeyCommand(0);
    const bool disabledOk = g_setVideoModeCalls == 0 && videoMode == 2 && hudTypeSw == 2;

    g_zVideo_SoftwareModeHotkeyEnabled = 1;
    g_setVideoModeResult = 0;
    g_setVideoModeCalls = 0;
    g_zVideo_HalfResAdjustMode = 0;
    zVideo::HandleSoftwareModeHotkeyCommand(0);
    const bool fullResOk = g_setVideoModeCalls == 1 && videoMode == 4 &&
                           g_zVideo_HalfResAdjustMode == 1 && hudTypeSw == 2 &&
                           render.width == 640 && render.height == 400 && replicate == 0;

    videoMode = 3;
    g_setVideoModeCalls = 0;
    g_zVideo_HalfResAdjustMode = 1;
    zVideo::HandleSoftwareModeHotkeyCommand(0);
    const bool halfResOk = g_setVideoModeCalls == 1 && videoMode == 2 &&
                           g_zVideo_HalfResAdjustMode == 1 &&
                           g_zVideo_UseHalfResBackbuffer == 1 && hudTypeSw == 2 &&
                           render.width == 320 && render.height == 200 && replicate == 1;

    videoMode = 4;
    g_setVideoModeResult = 7;
    g_setVideoModeCalls = 0;
    zVideo::HandleSoftwareModeHotkeyCommand(0);
    const bool failureOk = g_setVideoModeCalls == 1 && videoMode == 4 && hudTypeSw == 2;

    videoMode = 6;
    g_setVideoModeCalls = 0;
    zVideo::HandleSoftwareModeHotkeyCommand(0);
    const bool unsupportedOk = g_setVideoModeCalls == 0 && videoMode == 6 && hudTypeSw == 2;

    ZOPT_VIDEO_MODE = oldVideoMode;
    ZOPT_VIDEO_ACCELERATION = oldAcceleration;
    ZOPT_HUD_TYPE_SW = oldHudTypeSw;
    ZOPT_HUD_TYPE_HW = oldHudTypeHw;
    ZOPT_REPLICATE = oldReplicate;
    g_zOpt_RenderSectionOption = oldRenderSection;
    g_zOpt_DisplaySectionOption = oldDisplaySection;
    g_zOpt_WindowSectionOption = oldWindowSection;
    g_zOpt_HwMode = oldHwMode;
    g_HudUiMgrHudLayoutsInitialized = oldLayoutsInitialized;
    g_zVideo_pfnSetVideoMode = oldSetVideoMode;
    g_zVideo_SoftwareModeHotkeyEnabled = oldHotkeyEnabled;
    g_zVideo_HalfResAdjustMode = oldHalfRes;
    g_zVideo_UseHalfResBackbuffer = oldUseHalfResBackbuffer;
    g_zVideo_RendererType = oldRendererType;
    g_zVideo_pfnBltPrimaryToSwRectDirect = oldBltPrimaryToSw;
    g_setVideoModeResult = 0x1234;

    if (!disabledOk) {
        return 1;
    }
    if (!fullResOk) {
        return 2;
    }
    if (!halfResOk) {
        return 3;
    }
    if (!failureOk) {
        return 4;
    }
    return unsupportedOk ? 0 : 5;
}

extern "C" int zvideo_adjust_surfaces_if_enabled_smoke(void) {
    zVidRect32 rect{0, 0, 1, 1};
    g_adjustSurfaceCalls = 0;
    g_lastAdjustWaitForPresent = 0;
    g_lastAdjustBlitPrimaryToSwFirst = 0;
    g_zVideo_FrameTick = 10;
    g_zVideo_AdjustSurfacesDisableGate = 0;
    g_zVideo_pfnAdjustSurfaces = AdjustSurfacesFake;

    if (zVideo::AdjustSurfacesIfEnabled(&rect, &rect, 7, 9) != 0x123 || g_adjustSurfaceCalls != 1 ||
        g_lastAdjustWaitForPresent != 7 || g_lastAdjustBlitPrimaryToSwFirst != 9 ||
        g_zVideo_FrameTick != 11) {
        return 1;
    }

    g_zVideo_AdjustSurfacesDisableGate = 2;
    return zVideo::AdjustSurfacesIfEnabled(&rect, &rect, 1, 1) == 2 && g_adjustSurfaceCalls == 1 &&
                   g_zVideo_FrameTick == 11
               ? 0
               : 2;
}

extern "C" int zvideo_dd_report_error_smoke(void) {
    g_textureMemoryQueryCalls = 0;
    g_deviceMemoryQueryCalls = 0;
    g_lastTextureMemoryQueryFlags = 0;
    g_lastDeviceMemoryQueryFlags = 0;
    g_zVideo_pfnQueryTextureMemoryBytes = TextureMemoryQueryFake;
    g_zVideo_pfnQueryDeviceVideoMemoryBytes = DeviceMemoryQueryFake;

    if (zVideo_dd::ReportError(DD_OK, "video.cpp", 10) != 0 || g_textureMemoryQueryCalls != 0 ||
        g_deviceMemoryQueryCalls != 0) {
        return 1;
    }

    if (zVideo_dd::ReportError(DDERR_INVALIDPARAMS, "video.cpp", 20) != -1 ||
        g_textureMemoryQueryCalls != 0 || g_deviceMemoryQueryCalls != 0) {
        return 2;
    }

    if (zVideo_dd::ReportError(DDERR_OUTOFVIDEOMEMORY, "video.cpp", 30) != -1 ||
        g_textureMemoryQueryCalls != 1 || g_deviceMemoryQueryCalls != 1 ||
        g_lastTextureMemoryQueryFlags != -1 || g_lastDeviceMemoryQueryFlags != -1) {
        return 3;
    }

    g_zVideo_pfnQueryTextureMemoryBytes = nullptr;
    g_zVideo_pfnQueryDeviceVideoMemoryBytes = nullptr;
    return 0;
}

extern "C" int zvideo_dd_run_device_enumeration_smoke(void) {
    const int result = zVideo_dd::RunDirectDrawDeviceEnumeration();
    return result == 0 || result == 1 ? 0 : 1;
}

extern "C" int zvideo_dd_startup_enumerate_default_select_smoke(void) {
    g_zVideo_pSelectedHwApiDeviceRecord = nullptr;
    g_zVideo_pSelectedD3DDeviceInfo = reinterpret_cast<zVidD3DDriverRecordPartial *>(0x1);

    zVideo_dd::StartupEnumerateAndDefaultSelect();
    return g_zVideo_pSelectedHwApiDeviceRecord == &g_zVideo_HwApiDeviceTable[0] &&
                   g_zVideo_pSelectedD3DDeviceInfo == nullptr
               ? 0
               : 1;
}

extern "C" int zvideo_module_init_smoke(void) {
    g_zVideo_RendererType = 7;
    g_zVideo_ActiveRendererPath = 7;
    g_zVideo_FrameTick = 9;
    gVideo_resolutionMenuValid = 1;
    g_zVideo_PaletteBrightnessLevel = 2;
    g_zVideo_ClearColorPacked16 = 0xabcd;
    g_zVideo_pSelectedHwApiDeviceRecord = nullptr;
    g_zVideo_pSelectedD3DDeviceInfo = reinterpret_cast<zVidD3DDriverRecordPartial *>(0x1);

    if (zVideo::ModuleInit() != 0) {
        return 1;
    }

    return g_zVideo_RendererType == 0 && g_zVideo_ActiveRendererPath == 0 &&
                   g_zVideo_FrameTick == 0 && gVideo_resolutionMenuValid == 0 &&
                   g_zVideo_PaletteBrightnessLevel == 4 && g_zVideo_ClearColorPacked16 == 0 &&
                   g_zVideo_FullscreenOption == 1 && g_zVideo_PendingDitherEnable == -1 &&
                   g_zVideo_TexturePixelPack_ABits == 4 &&
                   g_zVideo_TexturePixelPack_RMask == 0xf000 &&
                   g_zVideo_pSelectedHwApiDeviceRecord == &g_zVideo_HwApiDeviceTable[0] &&
                   g_zVideo_pSelectedD3DDeviceInfo == nullptr
               ? 0
               : 2;
}

extern "C" int zvid_hw_api_accessors_smoke(void) {
    std::strncpy(g_zVideo_HwApiDeviceTable[2].m_driverName, "driver-two",
                 sizeof(g_zVideo_HwApiDeviceTable[2].m_driverName));
    std::strncpy(g_zVideo_HwApiDeviceTable[2].m_driverDescription, "description-two",
                 sizeof(g_zVideo_HwApiDeviceTable[2].m_driverDescription));
    std::strncpy(g_zVideo_HwApiDeviceTable[2].m_d3dDrivers[1].m_deviceName, "device-one",
                 sizeof(g_zVideo_HwApiDeviceTable[2].m_d3dDrivers[1].m_deviceName));

    g_zVideo_pSelectedHwApiDeviceRecord = nullptr;
    if (std::strcmp(zVid::GetSelectedHwApiDescriptionOrDefault(), "Default") != 0) {
        return 1;
    }

    g_zVideo_pSelectedD3DDeviceInfo = nullptr;
    if (std::strcmp(zVid::GetSelectedD3DDeviceNameOrDefault(), "GameZ") != 0) {
        return 2;
    }

    g_zVideo_pSelectedHwApiDeviceRecord = &g_zVideo_HwApiDeviceTable[2];
    g_zVideo_pSelectedD3DDeviceInfo = &g_zVideo_HwApiDeviceTable[2].m_d3dDrivers[1];
    return zVid::GetHwApiDriverName(2) == g_zVideo_HwApiDeviceTable[2].m_driverName &&
                   zVid::GetHwApiDescription(2) ==
                       g_zVideo_HwApiDeviceTable[2].m_driverDescription &&
                   zVid::GetSelectedHwApiDescriptionOrDefault() ==
                       g_zVideo_HwApiDeviceTable[2].m_driverDescription &&
                   zVid::GetSelectedD3DDeviceNameOrDefault() ==
                       g_zVideo_HwApiDeviceTable[2].m_d3dDrivers[1].m_deviceName
               ? 0
               : 3;
}

extern "C" int zvid_cached_renderer_and_texture_counts_smoke(void) {
    g_zVid_AcceptedHardwareRendererCount = 3;
    g_zVid_TexturePackLoadState = 1;
    if (zVid::GetAcceptedHardwareRendererCount_Cached() != 3 ||
        zVid::HasAcceptedHardwareRenderer() != 1 || zVid::GetTexturePackLoadState() != 1) {
        return 1;
    }

    g_zVid_AcceptedHardwareRendererCount = 0;
    g_zVid_TexturePackLoadState = 0;
    return zVid::GetAcceptedHardwareRendererCount_Cached() == 0 &&
                   zVid::HasAcceptedHardwareRenderer() == 0 && zVid::GetTexturePackLoadState() == 0
               ? 0
               : 2;
}

extern "C" int zvid_texture_pack_load_state_getter_smoke(void) {
    g_zVid_TexturePackLoadState = 0;
    if (zVid::GetTexturePackLoadState() != 0) {
        return 1;
    }

    g_zVid_TexturePackLoadState = 7;
    return zVid::GetTexturePackLoadState() == 7 ? 0 : 2;
}

extern "C" int zvid_texture_pack_load_state_setter_smoke(void) {
    g_zVid_TexturePackLoadState = 0;
    zVid::SetTexturePackLoadState(3);
    if (g_zVid_TexturePackLoadState != 3) {
        return 1;
    }

    zVid::SetTexturePackLoadState(0);
    return g_zVid_TexturePackLoadState == 0 ? 0 : 2;
}

extern "C" int zvid_option_accessors_smoke(void) {
    std::int32_t mode = 6;
    std::int32_t acceleration = 1;
    std::int32_t hwApi = 2;
    ZOPT_VIDEO_MODE = &mode;
    ZOPT_VIDEO_ACCELERATION = &acceleration;
    ZOPT_HW_API = &hwApi;

    if (zVid::GetVideoModeIndexFromOptions() != 6 || zVid::GetAccelerationOption() != 1 ||
        zVid::GetHwApiOption() != 2) {
        return 1;
    }

    mode = 3;
    zVid::SetAccelerationOption(0);
    zVid::SetHwApiOption(1);
    g_zVideo_NumAcceptedDirectDrawDevices = 3;
    return zVid::GetVideoModeIndexFromOptions() == 3 && zVid::GetAccelerationOption() == 0 &&
                   zVid::GetAcceptedDirectDrawDeviceCount() == 3 && g_zOpt_HwMode == 0 && hwApi == 1
               ? 0
               : 2;
}

extern "C" int zvid_set_video_mode_index_smoke(void) {
    std::int32_t mode = -1;
    std::int32_t replicate = -1;
    zOpt_ViewRectSection render{};
    zOpt_ViewRectSection display{};
    zOpt_ViewRectSection window{};
    zOpt_ViewRectSection *renderPtr = &render;
    zOpt_ViewRectSection *displayPtr = &display;
    zOpt_ViewRectSection *windowPtr = &window;
    ZOPT_VIDEO_MODE = &mode;
    ZOPT_REPLICATE = &replicate;
    g_zOpt_RenderSectionOption = &renderPtr;
    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_WindowSectionOption = &windowPtr;

    zVid::SetVideoModeIndex(2);
    if (mode != 2 || replicate != 1 || render.width != 0x140 || render.height != 0xc8 ||
        window.width != 0x280 || window.height != 0x190 || display.width != 0x280 ||
        display.height != 0x190 || display.bitsPerPixel != 0x10) {
        return 1;
    }

    zVid::SetVideoModeIndex(7);
    if (mode != 7 || replicate != 0 || render.width != 0x400 || render.height != 0x300 ||
        window.width != 0x400 || window.height != 0x300 || display.maxXInclusive != 0x3ff ||
        display.maxYInclusive != 0x2ff) {
        return 2;
    }

    zVid::SetVideoModeIndex(8);
    return mode == 0 ? 0 : 3;
}

extern "C" int zvideo_buff_clip_coord_to_range_smoke(void) {
    std::int32_t coord = 5;
    if (zVideo_buff::ClipCoordToRange(&coord, 2, 8) != 0 || coord != 5) {
        return 1;
    }

    coord = -3;
    if (zVideo_buff::ClipCoordToRange(&coord, 2, 8) != -5 || coord != 2) {
        return 2;
    }

    coord = 12;
    return zVideo_buff::ClipCoordToRange(&coord, 2, 8) == 4 && coord == 8 ? 0 : 3;
}

extern "C" int zvideo_buff_copy_surface_rect_to_image_smoke(void) {
    std::uint16_t pixels[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    std::uint16_t captured[12] = {};
    zVidImagePartial image{};
    image.pixels = captured;

    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.width = 4;
    g_zVideo_SwSurfaceState.height = 3;
    g_zVideo_SwSurfaceState.pitch = 8;
    g_zVideo_SwSurfaceState.pixels = pixels;

    zVidRect32 rect{-1, 1, 3, 4};
    zVidImagePartial *result = zVideo_buff::CopySurfaceRectToImage(0, &rect, &image);
    if (result != &image) {
        return 1;
    }

    if (rect.left != 0 || rect.top != 1 || rect.right != 3 || rect.bottom != 3) {
        return 2;
    }

    return captured[0] == 0 && captured[1] == 5 && captured[2] == 6 && captured[3] == 7 &&
                   captured[4] == 0 && captured[5] == 9 && captured[6] == 10 &&
                   captured[7] == 11
               ? 0
               : 3;
}

extern "C" int zvideo_surface_state_lock_skip_smoke(void) {
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf = reinterpret_cast<IDirectDrawSurface3 *>(0x1234);
    g_zVideo_DisplayModeSurfaceState.locked = 1;
    g_zVideo_FullscreenOption = 0;

    if (zVideo_dd::LockSurfaceState(&g_zVideo_DisplayModeSurfaceState) != 0 ||
        g_zVideo_DisplayModeSurfaceState.locked != 1) {
        return 1;
    }

    if (zVideo_dd::UnlockSurfaceState(&g_zVideo_DisplayModeSurfaceState) != 0 ||
        g_zVideo_DisplayModeSurfaceState.locked != 1) {
        return 2;
    }

    zVideo_SurfaceStatePartial alreadyLocked{};
    alreadyLocked.locked = 1;
    zVideo_SurfaceStatePartial unlocked{};
    g_zVideo_FullscreenOption = 1;
    return zVideo_dd::LockSurfaceState(&alreadyLocked) == 0 &&
                   zVideo_dd::UnlockSurfaceState(&unlocked) == 0
               ? 0
               : 3;
}

extern "C" int zvideo_capture_surface_to_image_smoke(void) {
    zVideo::BindRendererDispatch(0, 0);
    std::uint16_t pixels[6] = {1, 2, 0xaaaa, 3, 4, 0xbbbb};
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.width = 2;
    g_zVideo_DisplayModeSurfaceState.height = 2;
    g_zVideo_DisplayModeSurfaceState.pitch = 6;
    g_zVideo_DisplayModeSurfaceState.pixels = pixels;

    zVidImagePartial *image = zVideo_buff_CaptureSurfaceToImage(2);
    if (image == nullptr) {
        return 1;
    }

    auto *captured = static_cast<std::uint16_t *>(image->pixels);
    const bool ok = image->width == 2 && image->height == 2 && image->pixelCount == 4 &&
                    (image->formatFlagsPacked & 0x20u) != 0 && captured != nullptr &&
                    captured[0] == 1 && captured[1] == 2 && captured[2] == 3 && captured[3] == 4;

    zVid_Image::ReleaseIfNotDefault(image);
    return ok ? 0 : 2;
}

extern "C" int zvideo_image_lazy_create_backing_surface_guards_smoke(void) {
    zVidImagePartial image{};
    image.width = 8;
    image.height = 4;
    image.pixels = reinterpret_cast<void *>(0x12345678);
    image.alphaMap = reinterpret_cast<char *>(0x1234);

    if (zVideo_dd::Image_LazyCreateBackingSurface(&image, 0) != nullptr) {
        return 1;
    }

    image.alphaMap = nullptr;
    image.pixels = nullptr;
    if (zVideo_dd::Image_LazyCreateBackingSurface(&image, 0) != nullptr) {
        return 2;
    }

    image.pixels = reinterpret_cast<void *>(0x12345678);
    image.width = 0;
    if (zVideo_dd::Image_LazyCreateBackingSurface(&image, 0) != nullptr) {
        return 3;
    }

    image.width = 8;
    image.height = 0;
    return zVideo_dd::Image_LazyCreateBackingSurface(&image, 0) == nullptr ? 0 : 4;
}

extern "C" int zvideo_blt_sw_to_primary_rect_lazy_failure_smoke(void) {
    zVidHwApiDeviceRecordPartial selectedDevice{};
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;

    zVidImagePartial image{};
    image.width = 8;
    image.height = 4;
    image.alphaMap = reinterpret_cast<char *>(0x1234);
    zVideo_dd::BltSwToPrimaryRect(&image, 0, nullptr, nullptr);
    return image.surface == nullptr ? 0 : 1;
}

extern "C" int zvideo_flip_to_gdi_if_attached_null_smoke(void) {
    g_zVideo_pDirectDraw2 = nullptr;
    g_zVideo_PrimaryHasAttachedBackbuffer = 1;
    zVideo_dd::FlipToGDIIfAttached();

    g_zVideo_PrimaryHasAttachedBackbuffer = 0;
    zVideo_dd::FlipToGDIIfAttached();
    return 0;
}

extern "C" int zvideo_clear_rect_skip_paths_smoke(void) {
    zVidRect32 rect{0, 0, 10, 10};
    zVideo_SurfaceStatePartial state{};

    g_zVideo_ClearScreenBufferEnabled = 0;
    g_zVideo_pZBufferSurface = nullptr;
    zVideo_dd::ZBuffer_DepthFillRect(&rect);
    zVideo_dd::ClearScreenAndZBufferRect(&rect, &state);
    zVideo_dd::ClearSwBackbufferAndZBufferRects(&rect, &rect);
    return 0;
}

extern "C" int zvideo_palette_set_entries_non8bpp_smoke(void) {
    PALETTEENTRY entries[2] = {};
    g_zVideo_DisplayModeBpp = 16;
    g_zVideo_pDDPalette = reinterpret_cast<IDirectDrawPalette *>(0x1234);
    const std::int32_t result = zVideo_dd::PaletteSetEntries(1, 2, entries);
    g_zVideo_pDDPalette = nullptr;
    return result;
}

extern "C" int zvideo_apply_brightness_to_palette_entries_smoke(void) {
    const zVideo_PaletteSetEntriesProc savedPaletteSetEntries = g_zVideo_pfnPaletteSetEntries;
    const int savedInitialized = g_zVideo_IsInitialized;
    const unsigned char savedBrightness = g_zVideo_PaletteBrightnessLevel;
    PALETTEENTRY savedSystemPalette[256];
    std::memcpy(savedSystemPalette, g_zVideo_SystemPaletteEntries, sizeof(savedSystemPalette));

    g_zVideo_pfnPaletteSetEntries = CapturePaletteSetEntries;
    g_zVideoPaletteCaptureCallCount = 0;
    g_zVideoPaletteCaptureReturnValue = 37;
    g_zVideo_IsInitialized = 0;
    if (zVideo::ApplyBrightnessToPaletteEntries(0) != 0x5a560000 ||
        g_zVideoPaletteCaptureCallCount != 0) {
        std::memcpy(g_zVideo_SystemPaletteEntries, savedSystemPalette, sizeof(savedSystemPalette));
        g_zVideo_pfnPaletteSetEntries = savedPaletteSetEntries;
        g_zVideo_IsInitialized = savedInitialized;
        g_zVideo_PaletteBrightnessLevel = savedBrightness;
        return 1;
    }

    PALETTEENTRY sourceEntries[256] = {};
    sourceEntries[0].peRed = 250;
    sourceEntries[0].peGreen = 10;
    sourceEntries[0].peBlue = 0;
    sourceEntries[0].peFlags = 7;
    sourceEntries[1].peRed = 20;
    sourceEntries[1].peGreen = 239;
    sourceEntries[1].peBlue = 240;
    sourceEntries[1].peFlags = 9;

    g_zVideoPaletteCaptureCallCount = 0;
    g_zVideoPaletteCaptureReturnValue = 38;
    g_zVideo_IsInitialized = 1;
    g_zVideo_PaletteBrightnessLevel = 6;
    if (zVideo::ApplyBrightnessToPaletteEntries(sourceEntries) != 38 ||
        g_zVideoPaletteCaptureCallCount != 1 || g_zVideoPaletteCaptureFirstEntry != 0 ||
        g_zVideoPaletteCaptureEntryCount != 256) {
        std::memcpy(g_zVideo_SystemPaletteEntries, savedSystemPalette, sizeof(savedSystemPalette));
        g_zVideo_pfnPaletteSetEntries = savedPaletteSetEntries;
        g_zVideo_IsInitialized = savedInitialized;
        g_zVideo_PaletteBrightnessLevel = savedBrightness;
        return 2;
    }

    if (g_zVideo_SystemPaletteEntries[0].peRed != 250 ||
        g_zVideoPaletteCaptureEntries[0].peRed != 255 ||
        g_zVideoPaletteCaptureEntries[0].peGreen != 26 ||
        g_zVideoPaletteCaptureEntries[0].peBlue != 16 ||
        g_zVideoPaletteCaptureEntries[0].peFlags != 7 ||
        g_zVideoPaletteCaptureEntries[1].peRed != 36 ||
        g_zVideoPaletteCaptureEntries[1].peGreen != 255 ||
        g_zVideoPaletteCaptureEntries[1].peBlue != 255 ||
        g_zVideoPaletteCaptureEntries[1].peFlags != 9) {
        std::memcpy(g_zVideo_SystemPaletteEntries, savedSystemPalette, sizeof(savedSystemPalette));
        g_zVideo_pfnPaletteSetEntries = savedPaletteSetEntries;
        g_zVideo_IsInitialized = savedInitialized;
        g_zVideo_PaletteBrightnessLevel = savedBrightness;
        return 3;
    }

    g_zVideo_SystemPaletteEntries[0].peRed = 12;
    g_zVideo_SystemPaletteEntries[0].peGreen = 16;
    g_zVideo_SystemPaletteEntries[0].peBlue = 220;
    g_zVideo_SystemPaletteEntries[0].peFlags = 11;
    g_zVideoPaletteCaptureCallCount = 0;
    g_zVideoPaletteCaptureReturnValue = 39;
    g_zVideo_PaletteBrightnessLevel = 2;
    if (zVideo::ApplyBrightnessToPaletteEntries(0) != 39 ||
        g_zVideoPaletteCaptureEntries[0].peRed != 0 ||
        g_zVideoPaletteCaptureEntries[0].peGreen != 0 ||
        g_zVideoPaletteCaptureEntries[0].peBlue != 204 ||
        g_zVideoPaletteCaptureEntries[0].peFlags != 11) {
        std::memcpy(g_zVideo_SystemPaletteEntries, savedSystemPalette, sizeof(savedSystemPalette));
        g_zVideo_pfnPaletteSetEntries = savedPaletteSetEntries;
        g_zVideo_IsInitialized = savedInitialized;
        g_zVideo_PaletteBrightnessLevel = savedBrightness;
        return 4;
    }

    std::memcpy(g_zVideo_SystemPaletteEntries, savedSystemPalette, sizeof(savedSystemPalette));
    g_zVideo_pfnPaletteSetEntries = savedPaletteSetEntries;
    g_zVideo_IsInitialized = savedInitialized;
    g_zVideo_PaletteBrightnessLevel = savedBrightness;
    return 0;
}

extern "C" int zvideo_load_palette_file_and_apply_brightness_smoke(void) {
    const zVideo_PaletteSetEntriesProc savedPaletteSetEntries = g_zVideo_pfnPaletteSetEntries;
    const int savedInitialized = g_zVideo_IsInitialized;
    const unsigned char savedBrightness = g_zVideo_PaletteBrightnessLevel;
    char savedPath[256];
    PALETTEENTRY savedFileEntries[256];
    PALETTEENTRY savedSystemPalette[256];
    std::memcpy(savedPath, g_zVideo_PalettePathBuffer, sizeof(savedPath));
    std::memcpy(savedFileEntries, g_zVideo_PaletteFileEntries, sizeof(savedFileEntries));
    std::memcpy(savedSystemPalette, g_zVideo_SystemPaletteEntries, sizeof(savedSystemPalette));

    char tempDirectory[MAX_PATH];
    char tempPath[MAX_PATH];
    if (GetTempPathA(MAX_PATH, tempDirectory) == 0 ||
        GetTempFileNameA(tempDirectory, "zvp", 0, tempPath) == 0) {
        return 1;
    }

    unsigned char payload[3 * 256];
    for (int index = 0; index < static_cast<int>(sizeof(payload)); ++index) {
        payload[index] = static_cast<unsigned char>((index * 7) & 0xff);
    }

    FILE *file = std::fopen(tempPath, "wb");
    if (file == nullptr) {
        std::remove(tempPath);
        return 2;
    }
    const std::size_t written = std::fwrite(payload, 1, sizeof(payload), file);
    std::fclose(file);
    if (written != sizeof(payload)) {
        std::remove(tempPath);
        return 3;
    }

    std::memset(g_zVideo_PaletteFileEntries, 0xcc, sizeof(g_zVideo_PaletteFileEntries));
    g_zVideo_pfnPaletteSetEntries = CapturePaletteSetEntries;
    g_zVideoPaletteCaptureCallCount = 0;
    g_zVideoPaletteCaptureReturnValue = 44;
    g_zVideo_IsInitialized = 1;
    g_zVideo_PaletteBrightnessLevel = 4;

    const int result = zVideo::LoadPaletteFileAndApplyBrightness(tempPath);
    const unsigned char *fileEntryBytes =
        reinterpret_cast<const unsigned char *>(g_zVideo_PaletteFileEntries);
    const unsigned char *capturedBytes =
        reinterpret_cast<const unsigned char *>(g_zVideoPaletteCaptureEntries);

    int status = 0;
    if (result != 44 || g_zVideoPaletteCaptureCallCount != 1 ||
        g_zVideoPaletteCaptureFirstEntry != 0 || g_zVideoPaletteCaptureEntryCount != 256) {
        status = 4;
    } else if (std::strcmp(g_zVideo_PalettePathBuffer, tempPath) != 0) {
        status = 5;
    } else if (std::memcmp(fileEntryBytes, payload, sizeof(payload)) != 0 ||
               std::memcmp(capturedBytes, payload, sizeof(payload)) != 0 ||
               fileEntryBytes[sizeof(payload)] != 0xcc ||
               capturedBytes[sizeof(payload)] != 0xcc) {
        status = 6;
    } else {
        g_zVideoPaletteCaptureCallCount = 0;
        g_zVideoPaletteCaptureReturnValue = 45;
        status = zVideo::LoadPaletteFileAndApplyBrightness(0) == 45 &&
                         g_zVideoPaletteCaptureCallCount == 1
                     ? 0
                     : 7;
    }

    std::remove(tempPath);
    std::memcpy(g_zVideo_PalettePathBuffer, savedPath, sizeof(savedPath));
    std::memcpy(g_zVideo_PaletteFileEntries, savedFileEntries, sizeof(savedFileEntries));
    std::memcpy(g_zVideo_SystemPaletteEntries, savedSystemPalette, sizeof(savedSystemPalette));
    g_zVideo_pfnPaletteSetEntries = savedPaletteSetEntries;
    g_zVideo_IsInitialized = savedInitialized;
    g_zVideo_PaletteBrightnessLevel = savedBrightness;
    return status;
}

extern "C" int zvideo_quad_batch_depth_and_rhw_smoke(void) {
    std::memset(g_zVideo_QuadBatchItemsBase, 0, sizeof(g_zVideo_QuadBatchItemsBase));

    zVideo_dd3d::SetQuadBatchDepthAndRhw(0.25f);

    for (std::int32_t itemIndex = 0; itemIndex < 16; ++itemIndex) {
        for (std::int32_t vertexIndex = 0; vertexIndex < 4; ++vertexIndex) {
            const D3DTLVERTEX &vertex =
                g_zVideo_QuadBatchItemsBase[itemIndex].vertices[vertexIndex];
            if (vertex.sz != 0.25f || vertex.rhw != 0.25f) {
                return itemIndex + vertexIndex + 1;
            }
        }
    }

    return 0;
}

extern "C" int zvideo_set_active_view_context_smoke(void) {
    zClass_CameraDataPartial *savedViewContext = g_zVideo_pActiveViewContext;
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

    std::memset(g_zVideo_QuadBatchItemsBase, 0, sizeof(g_zVideo_QuadBatchItemsBase));
    g_zVideo_ActiveRendererPath = 0;
    zVideo_SetActiveViewContext(&viewContext);

    int status = 0;
    if (g_zVideo_pActiveViewContext != &viewContext || !NearFloat(viewContext.nearClip, 1.0f) ||
        !NearFloat(gClipRect_Primary.zMin, 2.0f) ||
        !NearFloat(gClipRect_Primary.zMax, 500.0f)) {
        status = 1;
    } else if (!NearFloat(g_zVideo_QuadBatchItemsBase[0].vertices[0].sz, 0.5f) ||
               !NearFloat(g_zVideo_QuadBatchItemsBase[15].vertices[3].rhw, 0.5f)) {
        status = 2;
    } else if (!NearFloat(gClipRect_Primary.xMin, 9.500999f) ||
               !NearFloat(gClipRect_Primary.xMax, 311.499f) ||
               !NearFloat(gClipRect_Primary.xMaxAlt, 310.499f) ||
               !NearFloat(gClipRect_Primary.yMin, 19.500999f) ||
               !NearFloat(gClipRect_Primary.yMax, 221.499f) ||
               !NearFloat(gClipRect_Primary.yMaxAlt, 220.499f)) {
        status = 3;
    } else if (!NearFloat(g_zVideo_ProjectClipLeft, 10.0f) ||
               !NearFloat(g_zVideo_ProjectClipTop, 20.0f) ||
               !NearFloat(g_zVideo_ProjectClipRight, 309.999f) ||
               !NearFloat(g_zVideo_ProjectClipBottom, 219.999f)) {
        status = 4;
    } else if (!NearFloat(g_zMath_FocalScaleX, 2.0f) ||
               !NearFloat(g_zMath_FocalScaleY, 4.0f) ||
               !NearFloat(g_zMath_ProjScaleX, 300.0f) ||
               !NearFloat(g_zMath_ProjScaleY, 400.0f) ||
               !NearFloat(g_zMath_ProjOffsetX, 160.0f) ||
               !NearFloat(g_zMath_ProjOffsetY, 120.0f) ||
               !NearFloat(g_zMath_ProjSphereRadiusScale, 1.0f) ||
               !NearFloat(g_zMath_ProjDepth, 500.0f) ||
               g_zMath_ScreenWidthPx != FloatBits(111.0f) ||
               g_zMath_ScreenHeightPx != FloatBits(222.0f)) {
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

        if (!NearFloat(gClipRect_Primary.zMin, 6.0f) ||
            !NearFloat(gClipRect_Primary.zMax, 1200.0f) ||
            !NearFloat(gClipRect_Primary.xMin, 0.0f) ||
            !NearFloat(gClipRect_Primary.xMax, 640.001f) ||
            !NearFloat(gClipRect_Primary.xMaxAlt, 640.001f) ||
            !NearFloat(gClipRect_Primary.yMin, 0.0f) ||
            !NearFloat(gClipRect_Primary.yMax, 480.001f) ||
            !NearFloat(gClipRect_Primary.yMaxAlt, 480.001f) ||
            !NearFloat(g_zVideo_ProjectClipLeft, 0.0f) ||
            !NearFloat(g_zVideo_ProjectClipTop, 0.0f) ||
            !NearFloat(g_zVideo_ProjectClipRight, 639.999f) ||
            !NearFloat(g_zVideo_ProjectClipBottom, 480.0f) ||
            !NearFloat(g_zMath_ProjScaleX, 320.0f) ||
            !NearFloat(g_zMath_ProjScaleY, 480.0f) ||
            !NearFloat(g_zMath_ProjOffsetX, 320.0f) ||
            !NearFloat(g_zMath_ProjOffsetY, 240.0f) ||
            g_zMath_ScreenWidthPx != FloatBits(333.0f) ||
            g_zMath_ScreenHeightPx != FloatBits(444.0f)) {
            status = 6;
        }
    }

    g_zVideo_pActiveViewContext = savedViewContext;
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

extern "C" int zvideo_frustum_test_sphere_clip_mask_smoke(void) {
    zClass_CameraDataPartial *savedViewContext = g_zVideo_pActiveViewContext;
    zClass_CameraDataPartial viewContext{};
    viewContext.cameraPos = {0.0f, 0.0f, 0.0f};
    viewContext.nearClipCenter = {0.0f, 0.0f, 1.0f};
    viewContext.farClipCenter = {0.0f, 0.0f, 10.0f};
    viewContext.worldFrustumNormals[0] = {1.0f, 0.0f, 0.0f};
    viewContext.worldFrustumNormals[4] = {0.0f, 0.0f, 1.0f};
    viewContext.worldFrustumNormals[5] = {0.0f, 0.0f, -1.0f};
    g_zVideo_pActiveViewContext = &viewContext;

    zVec3 sphere{0.0f, 0.0f, 0.0f};
    int clipMask = 0x10;
    int result = zVideo_FrustumTestSphereClipMask(&sphere, &clipMask, 0.5f);
    if (result != 0x10 || clipMask != 0) {
        g_zVideo_pActiveViewContext = savedViewContext;
        return 1;
    }

    sphere = {0.0f, 0.0f, 1.25f};
    clipMask = 0x10;
    result = zVideo_FrustumTestSphereClipMask(&sphere, &clipMask, 0.5f);
    if (result != 0 || clipMask != 0x10) {
        g_zVideo_pActiveViewContext = savedViewContext;
        return 2;
    }

    sphere = {-1.0f, 0.0f, 2.0f};
    clipMask = 1;
    result = zVideo_FrustumTestSphereClipMask(&sphere, &clipMask, 0.5f);
    if (result != 1 || clipMask != 0) {
        g_zVideo_pActiveViewContext = savedViewContext;
        return 3;
    }

    sphere = {0.25f, 0.0f, 2.0f};
    clipMask = 1;
    result = zVideo_FrustumTestSphereClipMask(&sphere, &clipMask, 0.5f);
    if (result != 0 || clipMask != 1) {
        g_zVideo_pActiveViewContext = savedViewContext;
        return 4;
    }

    sphere = {2.0f, 0.0f, 9.75f};
    clipMask = 0x21;
    result = zVideo_FrustumTestSphereClipMask(&sphere, &clipMask, 0.5f);
    if (result != 0 || clipMask != 0x20) {
        g_zVideo_pActiveViewContext = savedViewContext;
        return 5;
    }

    sphere = {2.0f, 0.0f, 12.0f};
    clipMask = 0x20;
    result = zVideo_FrustumTestSphereClipMask(&sphere, &clipMask, 0.5f);
    if (result != 0x20 || clipMask != 0) {
        g_zVideo_pActiveViewContext = savedViewContext;
        return 6;
    }

    g_zVideo_pActiveViewContext = savedViewContext;
    return 0;
}

extern "C" int zvideo_queue_solid_quad_smoke(void) {
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);
    std::memset(g_zVideo_QuadBatchItemsBase, 0, sizeof(g_zVideo_QuadBatchItemsBase));
    g_zVideo_QuadBatchCount = 0;
    g_zVideo_PrimarySurfaceState.width = 640;
    g_zVideo_PrimarySurfaceState.height = 480;

    zVideo_dd3d::QueueSolidQuad(0xf800, nullptr, 0.5);
    if (g_zVideo_QuadBatchCount != 1) {
        return 1;
    }

    const zVideo_QuadBatchItemPartial &full = g_zVideo_QuadBatchItemsBase[0];
    if (full.vertices[0].sx != 0.0f || full.vertices[0].sy != 0.0f ||
        full.vertices[1].sx != 480.0f || full.vertices[1].sy != 0.0f ||
        full.vertices[2].sx != 480.0f || full.vertices[2].sy != 640.0f ||
        full.vertices[3].sx != 0.0f || full.vertices[3].sy != 640.0f ||
        full.vertices[0].color != 0x7ff80000 || full.vertices[3].color != 0x7ff80000) {
        return 2;
    }

    zVidRect32 rect{10, 20, 30, 40};
    zVideo_dd3d::QueueSolidQuad(0x07e0, &rect, 0.25);
    if (g_zVideo_QuadBatchCount != 2) {
        return 3;
    }

    const zVideo_QuadBatchItemPartial &clipped = g_zVideo_QuadBatchItemsBase[1];
    if (clipped.vertices[0].sx != 10.0f || clipped.vertices[0].sy != 20.0f ||
        clipped.vertices[1].sx != 30.0f || clipped.vertices[1].sy != 20.0f ||
        clipped.vertices[2].sx != 30.0f || clipped.vertices[2].sy != 40.0f ||
        clipped.vertices[3].sx != 10.0f || clipped.vertices[3].sy != 40.0f ||
        clipped.vertices[0].color != 0x3f00fc00 || clipped.vertices[2].color != 0x3f00fc00) {
        return 4;
    }

    g_zVideo_QuadBatchCount = 0x10;
    zVideo_dd3d::QueueSolidQuad(0xffff, &rect, 1.0);
    return g_zVideo_QuadBatchCount == 0x10 ? 0 : 5;
}

extern "C" int zvideo_flush_quad_batch_empty_smoke(void) {
    g_zVideo_QuadBatchCount = 0;
    g_zVideo_pD3DDevice = nullptr;
    zVideo_dd3d::FlushQuadBatch();
    return g_zVideo_QuadBatchCount == 0 ? 0 : 1;
}

extern "C" int zvideo_flush_sorted_polys_empty_smoke(void) {
    g_zVideo_SortedPolyQueueCount = 0;
    g_zVideo_pD3DDevice = nullptr;
    zVideo_dd3d::FlushSortedPolys();
    return g_zVideo_SortedPolyQueueCount == 0 ? 0 : 1;
}

extern "C" int zvideo_submit_poly_flat_color16_queue_smoke(void) {
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);
    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    std::memset(g_zVideo_OverwriteQueueBase, 0, sizeof(g_zVideo_OverwriteQueueBase[0]));
    std::memset(g_zVideo_SortedPolyQueueBase, 0, sizeof(g_zVideo_SortedPolyQueueBase[0]));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;

    zVideo_XyzVertex vertices[3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f},
    };

    zVideo_dd3d::SubmitPolyFlatColor16(vertices, 0xf800, 0xff, 0x1234, 3, 1);
    if (g_zVideo_OverwriteQueueCount != 1 || g_zVideo_OverwriteQueueBase[0].type != 1 ||
        g_zVideo_OverwriteQueueBase[0].vertexCount != 3 ||
        g_zVideo_OverwriteQueueBase[0].renderClass != 0 ||
        g_zVideo_OverwriteQueueBase[0].renderParam != 0x1234) {
        return 1;
    }

    const D3DTLVERTEX &firstOpaque = g_zVideo_OverwriteQueueBase[0].vertices[0];
    const D3DTLVERTEX &lastOpaque = g_zVideo_OverwriteQueueBase[0].vertices[2];
    if (firstOpaque.sx != 7.0f || firstOpaque.sy != 8.0f || firstOpaque.sz != 9.0f ||
        firstOpaque.rhw != 9.0f || firstOpaque.color != 0xfff80000 ||
        firstOpaque.specular != 0xff000000 || lastOpaque.sx != 1.0f) {
        return 2;
    }

    zVideo_dd3d::SubmitPolyFlatColor16(vertices, 0x07e0, 0x80, 0x5678, 2, 0);
    if (g_zVideo_SortedPolyQueueCount != 1 || g_zVideo_SortedPolyQueueBase[0].vertexCount != 2 ||
        g_zVideo_SortedPolyQueueBase[0].renderClass != 0 ||
        g_zVideo_SortedPolyQueueBase[0].renderParam != 0x5678) {
        return 3;
    }

    const D3DTLVERTEX &firstSorted = g_zVideo_SortedPolyQueueBase[0].vertices[0];
    return firstSorted.sx == 4.0f && firstSorted.sy == 5.0f && firstSorted.sz == 6.0f &&
                   firstSorted.color == 0x8000fc00 && firstSorted.specular == 0xff000000
               ? 0
               : 4;
}

extern "C" int zvideo_submit_poly_gouraud_color16_queue_smoke(void) {
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);
    std::memset(g_zVideo_OverwriteQueueBase, 0, sizeof(g_zVideo_OverwriteQueueBase[0]));
    std::memset(g_zVideo_SortedPolyQueueBase, 0, sizeof(g_zVideo_SortedPolyQueueBase[0]));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;

    zVideo_XyzVertex vertices[3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f},
    };
    std::uint32_t colors[3] = {0xf800, 0x07e0, 0x001f};

    zVideo_dd3d::SubmitPolyGouraudColor16(vertices, colors, 0xff, 0x99, 3, 1);
    if (g_zVideo_OverwriteQueueCount != 1 || g_zVideo_OverwriteQueueBase[0].type != 2 ||
        g_zVideo_OverwriteQueueBase[0].vertexCount != 3 ||
        g_zVideo_OverwriteQueueBase[0].renderParam != 0x99) {
        return 1;
    }

    if (g_zVideo_OverwriteQueueBase[0].vertices[0].sx != 7.0f ||
        g_zVideo_OverwriteQueueBase[0].vertices[0].color != 0xff0000f8 ||
        g_zVideo_OverwriteQueueBase[0].vertices[2].sx != 1.0f ||
        g_zVideo_OverwriteQueueBase[0].vertices[2].color != 0xfff80000) {
        return 2;
    }

    zVideo_dd3d::SubmitPolyGouraudColor16(vertices, colors, 0x40, 0x77, 2, 0);
    if (g_zVideo_SortedPolyQueueCount != 1 || g_zVideo_SortedPolyQueueBase[0].vertexCount != 2 ||
        g_zVideo_SortedPolyQueueBase[0].renderParam != 0x77) {
        return 3;
    }

    return g_zVideo_SortedPolyQueueBase[0].vertices[0].sx == 4.0f &&
                   g_zVideo_SortedPolyQueueBase[0].vertices[0].color == 0x4000fc00 &&
                   g_zVideo_SortedPolyQueueBase[0].vertices[1].sx == 1.0f &&
                   g_zVideo_SortedPolyQueueBase[0].vertices[1].color == 0x40f80000
               ? 0
               : 4;
}

extern "C" int zvideo_submit_poly_color_attr_smoke(void) {
    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    std::memset(g_zVideo_OverwriteQueueBase, 0, sizeof(g_zVideo_OverwriteQueueBase[0]));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;

    zVideo_XyzVertex vertices[3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f},
    };
    zVideo_ColorRgbFloat baseColor{16.0f, 32.0f, 48.0f};
    float attr1[1] = {0.0f};
    float attr0[3] = {0.0f, 0.5f, 1.0f};
    float attr2[3] = {0.0f, 0.5f, 1.0f};

    g_zVideo_D3DColorNormalizeChannelIndex = 2;
    g_zVideo_D3DColorAttrBiasR = 100.0f;
    g_zVideo_D3DColorAttrBiasG = 200.0f;
    g_zVideo_D3DColorAttrBiasB = 300.0f;

    zVideo_dd3d::SubmitPolyColorAttr(vertices, 0, &baseColor, attr1, attr0, attr2, 0x80, 3, 0x44,
                                     1);

    if (g_zVideo_OverwriteQueueCount != 0 || g_zVideo_SortedPolyQueueCount != 0) {
        return 1;
    }

    if (g_zVideo_D3DSubmitTempVertices[0].color != 0x8055aaff ||
        g_zVideo_D3DSubmitTempVertices[0].specular != 0x00000000 ||
        g_zVideo_D3DSubmitTempVertices[1].color != 0x804284c6 ||
        g_zVideo_D3DSubmitTempVertices[1].specular != 0x80000000 ||
        g_zVideo_D3DSubmitTempVertices[2].color != 0x80102030 ||
        g_zVideo_D3DSubmitTempVertices[2].specular != 0xff000000) {
        return 2;
    }

    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    attr1[0] = 0.0f;
    zVideo_dd3d::SubmitPolyColorAttr(vertices, 0, &baseColor, attr1, nullptr, nullptr, 0xff, 3,
                                     0x55, 1);

    if (g_zVideo_OverwriteQueueCount != 1 || g_zVideo_OverwriteQueueBase[0].type != 3 ||
        g_zVideo_OverwriteQueueBase[0].vertexCount != 3 ||
        g_zVideo_OverwriteQueueBase[0].renderClass != 0 ||
        g_zVideo_OverwriteQueueBase[0].renderParam != 0x55) {
        return 3;
    }

    const D3DTLVERTEX &first = g_zVideo_OverwriteQueueBase[0].vertices[0];
    const D3DTLVERTEX &last = g_zVideo_OverwriteQueueBase[0].vertices[2];
    return first.sx == 7.0f && first.sy == 8.0f && first.sz == 9.0f && first.rhw == 9.0f &&
                   first.color == 0xff102030 && first.specular == 0xff000000 && last.sx == 1.0f &&
                   last.color == 0xff102030
               ? 0
               : 4;
}

extern "C" int zvideo_submit_poly_render_class_queue_smoke(void) {
    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    std::memset(g_zVideo_OverwriteQueueBase, 0, sizeof(g_zVideo_OverwriteQueueBase[0]));
    std::memset(g_zVideo_SortedPolyQueueBase, 0, sizeof(g_zVideo_SortedPolyQueueBase[0]));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;

    zVideo_XyzVertex vertices[3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f},
    };
    zVideo_TexCoord texCoords[3] = {
        {0.1f, 0.2f},
        {0.3f, 0.4f},
        {0.5f, 0.6f},
    };
    zVideo_RenderClass renderClass{};
    renderClass.textureHandle = 0x1234;
    renderClass.textureMapBlend = static_cast<D3DTEXTUREBLEND>(3);
    renderClass.textureAddressU = static_cast<D3DTEXTUREADDRESS>(1);
    renderClass.textureAddressV = static_cast<D3DTEXTUREADDRESS>(2);

    zVideo_dd3d::SubmitPolyRenderClass(vertices, texCoords, 3, &renderClass, 0x77, 1.0f, 1);

    if (g_zVideo_OverwriteQueueCount != 1 || g_zVideo_OverwriteQueueBase[0].type != 4 ||
        g_zVideo_OverwriteQueueBase[0].vertexCount != 3 ||
        g_zVideo_OverwriteQueueBase[0].renderClass !=
            reinterpret_cast<std::int32_t>(&renderClass) ||
        g_zVideo_OverwriteQueueBase[0].renderParam != 0x77) {
        return 1;
    }

    const D3DTLVERTEX &opaqueFirst = g_zVideo_OverwriteQueueBase[0].vertices[0];
    if (opaqueFirst.sx != 7.0f || opaqueFirst.sy != 8.0f || opaqueFirst.sz != 9.0f ||
        opaqueFirst.rhw != 9.0f || opaqueFirst.color != 0xffffffff ||
        opaqueFirst.specular != 0xff000000 || opaqueFirst.tu != 0.5f || opaqueFirst.tv != 0.6f) {
        return 2;
    }

    renderClass.textureMapBlend = static_cast<D3DTEXTUREBLEND>(4);
    zVideo_dd3d::SubmitPolyRenderClass(vertices, texCoords, 2, &renderClass, 0x88, 0.25f, 1);
    if (g_zVideo_OverwriteQueueCount != 2 || g_zVideo_OverwriteQueueBase[1].type != 0 ||
        g_zVideo_OverwriteQueueBase[1].vertexCount != 2 ||
        g_zVideo_OverwriteQueueBase[1].renderClass !=
            reinterpret_cast<std::int32_t>(&renderClass) ||
        g_zVideo_OverwriteQueueBase[1].renderParam != 0x88 ||
        g_zVideo_OverwriteQueueBase[1].vertices[0].sx != 4.0f ||
        g_zVideo_OverwriteQueueBase[1].vertices[0].color != 0x3fffffff ||
        g_zVideo_OverwriteQueueBase[1].vertices[0].tu != 0.3f) {
        return 3;
    }

    zVideo_dd3d::SubmitPolyRenderClass(vertices, texCoords, 2, &renderClass, 0x99, 0.5f, 0);
    if (g_zVideo_SortedPolyQueueCount != 1 || g_zVideo_SortedPolyQueueBase[0].vertexCount != 2 ||
        g_zVideo_SortedPolyQueueBase[0].renderClass !=
            reinterpret_cast<std::int32_t>(&renderClass) ||
        g_zVideo_SortedPolyQueueBase[0].renderParam != 0x99) {
        return 4;
    }

    return g_zVideo_SortedPolyQueueBase[0].vertices[0].sx == 4.0f &&
                   g_zVideo_SortedPolyQueueBase[0].vertices[0].color == 0x7fffffff &&
                   g_zVideo_SortedPolyQueueBase[0].vertices[0].tu == 0.3f &&
                   g_zVideo_SortedPolyQueueBase[0].vertices[1].sx == 1.0f
               ? 0
               : 5;
}

extern "C" int zvideo_submit_polygon_queue_smoke(void) {
    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    std::memset(g_zVideo_OverwriteQueueBase, 0, sizeof(g_zVideo_OverwriteQueueBase[0]));
    std::memset(g_zVideo_SortedPolyQueueBase, 0, sizeof(g_zVideo_SortedPolyQueueBase[0]));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;

    zVideo_XyzVertex vertices[3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f},
    };
    zVideo_TexCoord uvPairs[3] = {
        {0.1f, 0.2f},
        {0.3f, 0.4f},
        {0.5f, 0.6f},
    };
    zVideo_RenderClass renderClass{};
    renderClass.textureHandle = 0x2345;
    renderClass.textureMapBlend = static_cast<D3DTEXTUREBLEND>(3);
    renderClass.textureAddressU = static_cast<D3DTEXTUREADDRESS>(1);
    renderClass.textureAddressV = static_cast<D3DTEXTUREADDRESS>(2);
    float attr1[1] = {0.0f};

    g_zVideo_D3DAppendFanCloseVertexPending = 1;
    zVideo_dd3d::SubmitPolygon(vertices, uvPairs, attr1, nullptr, nullptr, 3, &renderClass, 0x66,
                               1.0f, 1);
    if (g_zVideo_OverwriteQueueCount != 1 || g_zVideo_OverwriteQueueBase[0].type != 5 ||
        g_zVideo_OverwriteQueueBase[0].vertexCount != 4 ||
        g_zVideo_OverwriteQueueBase[0].renderClass !=
            reinterpret_cast<std::int32_t>(&renderClass) ||
        g_zVideo_OverwriteQueueBase[0].renderParam != 0x66 ||
        g_zVideo_D3DAppendFanCloseVertexPending != 0) {
        return 1;
    }

    const D3DTLVERTEX &opaqueFirst = g_zVideo_OverwriteQueueBase[0].vertices[0];
    const D3DTLVERTEX &opaqueClose = g_zVideo_OverwriteQueueBase[0].vertices[3];
    if (opaqueFirst.sx != 7.0f || opaqueFirst.color != 0xffffffff ||
        opaqueFirst.specular != 0xff000000 || opaqueFirst.tu != 0.5f || opaqueClose.sx != 4.0f ||
        opaqueClose.tu != 0.3f) {
        return 2;
    }

    renderClass.textureMapBlend = static_cast<D3DTEXTUREBLEND>(4);
    attr1[0] = 0.5f;
    float attr2[2] = {0.0f, 1.0f};
    zVideo_dd3d::SubmitPolygon(vertices, uvPairs, attr1, nullptr, attr2, 2, &renderClass, 0x77,
                               0.5f, 0);

    if (g_zVideo_SortedPolyQueueCount != 1 || g_zVideo_SortedPolyQueueBase[0].vertexCount != 2 ||
        g_zVideo_SortedPolyQueueBase[0].renderClass !=
            reinterpret_cast<std::int32_t>(&renderClass) ||
        g_zVideo_SortedPolyQueueBase[0].renderParam != 0x77) {
        return 3;
    }

    const D3DTLVERTEX &transparentFirst = g_zVideo_SortedPolyQueueBase[0].vertices[0];
    const D3DTLVERTEX &transparentLast = g_zVideo_SortedPolyQueueBase[0].vertices[1];
    return transparentFirst.sx == 4.0f && transparentFirst.color == 0x7f7f7f7f &&
                   transparentFirst.specular == 0x00000000 && transparentFirst.tu == 0.3f &&
                   transparentLast.sx == 1.0f && transparentLast.color == 0x7f7f7f7f &&
                   transparentLast.specular == 0xff000000
               ? 0
               : 4;
}

extern "C" int zvideo_submit_polygon_lit_queue_smoke(void) {
    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    std::memset(g_zVideo_OverwriteQueueBase, 0, sizeof(g_zVideo_OverwriteQueueBase[0]));
    std::memset(g_zVideo_SortedPolyQueueBase, 0, sizeof(g_zVideo_SortedPolyQueueBase[0]));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;

    zVideo_XyzVertex vertices[3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f},
    };
    zVideo_TexCoord uvPairs[3] = {
        {0.1f, 0.2f},
        {0.3f, 0.4f},
        {0.5f, 0.6f},
    };
    zVideo_RenderClass renderClass{};
    renderClass.textureMapBlend = static_cast<D3DTEXTUREBLEND>(3);
    float attr1[3] = {0.0f, 0.5f, 1.0f};

    g_zVideo_D3DAppendFanCloseVertexPending = 1;
    zVideo_dd3d::SubmitPolygonLit(vertices, uvPairs, attr1, nullptr, nullptr, 3, &renderClass, 0x44,
                                  1.0f, 1);

    if (g_zVideo_OverwriteQueueCount != 1 || g_zVideo_OverwriteQueueBase[0].type != 6 ||
        g_zVideo_OverwriteQueueBase[0].vertexCount != 4 ||
        g_zVideo_OverwriteQueueBase[0].renderClass !=
            reinterpret_cast<std::int32_t>(&renderClass) ||
        g_zVideo_D3DAppendFanCloseVertexPending != 0) {
        return 1;
    }

    if (g_zVideo_OverwriteQueueBase[0].vertices[0].sx != 7.0f ||
        g_zVideo_OverwriteQueueBase[0].vertices[0].color != 0xff000000 ||
        g_zVideo_OverwriteQueueBase[0].vertices[1].color != 0xff7f7f7f ||
        g_zVideo_OverwriteQueueBase[0].vertices[2].color != 0xffffffff ||
        g_zVideo_OverwriteQueueBase[0].vertices[3].sx != 4.0f) {
        return 2;
    }

    renderClass.textureMapBlend = static_cast<D3DTEXTUREBLEND>(4);
    float attr2[2] = {0.0f, 1.0f};
    zVideo_dd3d::SubmitPolygonLit(vertices, uvPairs, attr1, nullptr, attr2, 2, &renderClass, 0x55,
                                  0.5f, 0);

    if (g_zVideo_SortedPolyQueueCount != 1 || g_zVideo_SortedPolyQueueBase[0].vertexCount != 2 ||
        g_zVideo_SortedPolyQueueBase[0].renderClass !=
            reinterpret_cast<std::int32_t>(&renderClass) ||
        g_zVideo_SortedPolyQueueBase[0].renderParam != 0x55) {
        return 3;
    }

    return g_zVideo_SortedPolyQueueBase[0].vertices[0].sx == 4.0f &&
                   g_zVideo_SortedPolyQueueBase[0].vertices[0].color == 0x7f7f7f7f &&
                   g_zVideo_SortedPolyQueueBase[0].vertices[0].specular == 0x00000000 &&
                   g_zVideo_SortedPolyQueueBase[0].vertices[1].sx == 1.0f &&
                   g_zVideo_SortedPolyQueueBase[0].vertices[1].color == 0x7fffffff &&
                   g_zVideo_SortedPolyQueueBase[0].vertices[1].specular == 0xff000000
               ? 0
               : 4;
}

extern "C" int zvideo_present_display_mode_surface_null_smoke(void) {
    g_zVideo_DisplayModeSurfaceState.surf = nullptr;
    zVidRect32 rect{};
    return zVideo_dd3d::PresentDisplayModeSurface(&rect, &rect, 0, 0) == 0x400 ? 0 : 1;
}

extern "C" int zvideo_texture_record_release_upload_null_smoke(void) {
    zVideo_TextureRecordPartial textureRecord{};
    zVideo_dd3d::TextureRecord_ReleaseUploadSurfaceRef(&textureRecord);
    return textureRecord.m_uploadSurface == nullptr ? 0 : 1;
}

extern "C" int zvideo_texture_record_create_and_power_smoke(void) {
    if (zVideo_dd3d::FloorPowerOfTwo(1) != 1 || zVideo_dd3d::FloorPowerOfTwo(3) != 2 ||
        zVideo_dd3d::FloorPowerOfTwo(64) != 64 || zVideo_dd3d::FloorPowerOfTwo(65) != 64) {
        return 1;
    }

    zVideo_TextureRecordPartial *textureRecord = zVideo_dd3d::TextureRecord_Create();
    if (textureRecord == nullptr) {
        return 2;
    }

    const bool zeroed = textureRecord->m_uploadSurface == nullptr &&
                        textureRecord->m_textureSurface == nullptr &&
                        textureRecord->m_texture == nullptr &&
                        textureRecord->m_textureHandle == 0 && textureRecord->m_alphaMode == 0 &&
                        textureRecord->m_uWrapMode == 0 && textureRecord->m_vWrapMode == 0;
    std::free(textureRecord);
    return zeroed ? 0 : 3;
}

extern "C" int zvideo_convert_image_pixels_for_texture_smoke(void) {
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);

    std::uint16_t srcPixels[4] = {0xf800, 0x07e0, 0x001f, 0};
    std::uint16_t dstPixels[8] = {};
    zVidImagePartial image{};
    image.width = 2;
    image.height = 2;
    image.pixels = srcPixels;

    zVideo_dd3d::ConvertImagePixelsForTexture(dstPixels, &image, 8, 0);

    if (dstPixels[0] != 0xfc00 || dstPixels[1] != 0x83f0 || dstPixels[4] != 0x801f ||
        dstPixels[5] != 0) {
        return 1;
    }

    std::uint8_t alphaMap[4] = {0xf0, 0x80, 0x10, 0};
    std::memset(dstPixels, 0, sizeof(dstPixels));
    image.alphaMap = reinterpret_cast<char *>(alphaMap);
    zVideo_dd3d::ConvertImagePixelsForTexture(dstPixels, &image, 8, 1);

    return dstPixels[0] == 0xff00 && dstPixels[1] == 0x80f0 && dstPixels[4] == 0x100f &&
                   dstPixels[5] == 0
               ? 0
               : 2;
}

extern "C" int zvid_image_resample_square_smoke(void) {
    std::uint16_t *pixels = static_cast<std::uint16_t *>(std::malloc(8 * sizeof(std::uint16_t)));
    char *alphaMap = static_cast<char *>(std::malloc(8));
    if (pixels == nullptr || alphaMap == nullptr) {
        std::free(pixels);
        std::free(alphaMap);
        return 1;
    }

    for (std::int32_t i = 0; i < 8; ++i) {
        pixels[i] = static_cast<std::uint16_t>(0x10 + i);
        alphaMap[i] = static_cast<char>(0x40 + i);
    }

    zVidImagePartial image{};
    image.width = 4;
    image.height = 2;
    image.pixels = pixels;
    image.alphaMap = alphaMap;

    zVid_Image::ResampleSquare(&image, 2);

    std::uint16_t *newPixels = static_cast<std::uint16_t *>(image.pixels);
    char *newAlphaMap = image.alphaMap;
    const bool ok =
        image.width == 2 && image.height == 2 && newPixels != nullptr && newAlphaMap != nullptr &&
        newPixels[0] == 0x10 && newPixels[1] == 0x12 && newPixels[2] == 0x14 &&
        newPixels[3] == 0x16 && newAlphaMap[0] == static_cast<char>(0x40) &&
        newAlphaMap[1] == static_cast<char>(0x42) && newAlphaMap[2] == static_cast<char>(0x44) &&
        newAlphaMap[3] == static_cast<char>(0x46);

    std::free(image.pixels);
    std::free(image.alphaMap);
    return ok ? 0 : 2;
}

extern "C" int zvid_image_calc_pow2_scratch_fields_smoke(void) {
    zVidImagePartial image{};
    image.width = 64;
    image.height = 16;
    image.uPow2Shift = 9;
    image.vPow2Shift = 9;

    zVid_Image::CalcPow2ScratchFields(&image);
    const bool pow2Ok = image.uPow2Shift == 6 && image.vPow2Shift == 4 &&
                        image.widthScale == 1.0f && image.uShiftFrom20 == 14 &&
                        image.uMask == 0x3f && image.vMaskFixed20 == 0x00f00000;

    image = {};
    image.width = 1;
    image.height = -2;
    zVid_Image::CalcPow2ScratchFields(&image);
    const bool smallOk = image.uPow2Shift == 0 && image.vPow2Shift == 0 &&
                         image.uShiftFrom20 == 20 && image.uMask == 0 && image.vMaskFixed20 == 0;

    return pow2Ok && smallOk ? 0 : 1;
}

extern "C" int zvid_image_release_owned_buffers_smoke(void) {
    zVidImagePartial image{};
    image.pixels = std::malloc(4);
    image.alphaMap = static_cast<char *>(std::malloc(4));
    image.palette = std::malloc(4);
    if (image.pixels == nullptr || image.alphaMap == nullptr || image.palette == nullptr) {
        std::free(image.pixels);
        std::free(image.alphaMap);
        std::free(image.palette);
        return 1;
    }

    image.formatFlagsPacked = 0xe0;
    zVid_Image::ReleaseOwnedBuffers(&image);
    if (image.pixels != nullptr || image.alphaMap != nullptr || image.palette != nullptr ||
        (image.formatFlagsPacked & 0xe0) != 0) {
        return 2;
    }

    void *palette = std::malloc(4);
    if (palette == nullptr) {
        return 3;
    }

    image.palette = palette;
    image.formatFlagsPacked = 0x90;
    zVid_Image::ReleaseOwnedBuffers(&image);
    const bool keptSharedPalette = image.palette == palette && image.formatFlagsPacked == 0x90;
    std::free(palette);
    image.palette = nullptr;
    return keptSharedPalette ? 0 : 4;
}

extern "C" int zvid_image_destroy_smoke(void) {
    zVidImagePartial *image =
        static_cast<zVidImagePartial *>(std::malloc(sizeof(zVidImagePartial)));
    if (image == nullptr) {
        return 1;
    }

    *image = {};
    image->pixels = std::malloc(4);
    if (image->pixels == nullptr) {
        std::free(image);
        return 2;
    }
    image->formatFlagsPacked = 0x20;

    return zVid_Image::Destroy(image) == 0 && zVid_Image::Destroy(nullptr) == 0 ? 0 : 3;
}

extern "C" int zvideo_create_texture_record_guards_smoke(void) {
    zVidD3DDriverRecordPartial selectedD3DDevice{};
    D3DDEVICEDESC *selectedDesc = reinterpret_cast<D3DDEVICEDESC *>(selectedD3DDevice.m_hwDesc);
    selectedDesc->dwMaxTextureWidth = 64;
    selectedDesc->dwMaxTextureHeight = 64;
    g_zVideo_pSelectedD3DDeviceInfo = &selectedD3DDevice;

    zVideo_TextureRecordPartial defaultRecord{};
    g_zImage_DefaultTextureRecord = &defaultRecord;

    zVidImagePartial image{};
    image.width = 128;
    image.height = 8;
    g_zVideo_D3DHalDeviceDesc.dpcTriCaps.dwTextureCaps = 0;
    if (zVideo_dd3d::CreateTextureRecord("too-large", &image, 0, 0, 0) != &defaultRecord) {
        return 1;
    }

    image.width = 10;
    image.height = 8;
    g_zVideo_D3DHalDeviceDesc.dpcTriCaps.dwTextureCaps = D3DPTEXTURECAPS_POW2;
    if (zVideo_dd3d::CreateTextureRecord("non-pow2", &image, 0, 0, 0) != &defaultRecord) {
        return 2;
    }

    image.width = 64;
    image.height = 4;
    g_zVideo_D3DHalDeviceDesc.dpcTriCaps.dwTextureCaps = 0;
    if (zVideo_dd3d::CreateTextureRecord("bad-aspect", &image, 0, 0, 0) != &defaultRecord) {
        return 3;
    }

    image.width = 8;
    image.height = 8;
    image.palette = reinterpret_cast<void *>(0x1234);
    if (zVideo_dd3d::CreateTextureRecord("paletted", &image, 0, 0, 0) != &defaultRecord) {
        return 4;
    }

    image.palette = nullptr;
    g_zImage_DefaultTextureRecord = nullptr;
    g_zVideo_pSelectedD3DDeviceInfo = nullptr;
    return 0;
}

extern "C" int zvideo_image_alpha_clear_smoke(void) {
    std::uint16_t pixels[4] = {0x1111, 0x2222, 0x3333, 0x4444};
    char alpha[4] = {1, 0, 1, 0};

    zVidImagePartial image{};
    image.pixelCount = 4;
    image.formatFlagsPacked = 1;
    image.paletteMetaPacked = 0;
    image.pixels = pixels;
    image.alphaMap = alpha;

    if (zVid_Image::QueryBytesPerPixel(&image) != 2) {
        return 1;
    }

    zVid_Image::ClearZeroAlphaPixelsInPlace(&image);
    return pixels[0] == 0x1111 && pixels[1] == 0 && pixels[2] == 0x3333 && pixels[3] == 0 ? 0 : 2;
}

extern "C" int zvideo_image_set_pixels_smoke(void) {
    zVidImagePartial image{};
    std::uint16_t pixels[2] = {0x1111, 0x2222};
    char alpha[2] = {1, 0};

    image.formatFlagsPacked = 0x20;
    const bool withAlpha = zVid_Image_SetPixels(&image, pixels, alpha) == 0 &&
                           image.pixels == pixels && image.alphaMap == alpha &&
                           (image.formatFlagsPacked & 0x22u) == 0x22u;

    image.formatFlagsPacked = 0x20;
    const bool withoutAlpha = zVid_Image_SetPixels(&image, pixels, nullptr) == 0 &&
                              image.pixels == pixels && image.alphaMap == nullptr &&
                              image.formatFlagsPacked == 0x20u;

    return withAlpha && withoutAlpha ? 0 : 1;
}

extern "C" int zvid_image_create_format_size_pixels_smoke(void) {
    zVidImagePartial *const image = zVid_Image::Create();
    if (image == nullptr) {
        return 1;
    }

    const bool createdZeroed =
        image->pixelCount == 0 && image->width == 0 && image->height == 0 &&
        image->formatFlagsPacked == 0 && image->pixels == nullptr && image->alphaMap == nullptr;

    std::uint16_t pixels[128] = {};
    char alpha[128] = {};
    const bool configured =
        zVid_Image::SetFormatCode(image, 1) == 0 &&
        zVid_Image_SetPixels(image, pixels, alpha) == 0 &&
        zVid_Image::SetSize(image, 16, 8) == 0 &&
        image->formatFlagsPacked == 3 && image->pixels == pixels && image->alphaMap == alpha &&
        image->width == 16 && image->height == 8 && image->pixelCount == 128 &&
        image->pitchWords == 16 && image->widthScale == 16.0f &&
        image->uShiftFrom20 == 20 && image->uMask == 15 &&
        image->vMaskFixed20 == (7 << 20);

    zVid_Image::Destroy(image);
    return createdZeroed && configured ? 0 : 2;
}

extern "C" int zvideo_image_file_read_helpers_smoke(void) {
    unsigned char header[0x10] = {};
    header[0] = 1;
    *reinterpret_cast<std::int16_t *>(&header[4]) = 2;
    *reinterpret_cast<std::int16_t *>(&header[6]) = 1;
    header[8] = 0x12;
    *reinterpret_cast<std::int16_t *>(&header[0x0c]) = 0x3456;
    *reinterpret_cast<std::int16_t *>(&header[0x0e]) = 0;
    std::uint16_t pixels[2] = {0x7fff, 0x001f};

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        return 1;
    }

    std::fwrite(header, 1, sizeof(header), file);
    std::fwrite(pixels, 1, sizeof(pixels), file);
    std::rewind(file);

    g_zVideo_PixelPack_RBits = 5;
    zVidImagePartial *image = zVid_Image::ReadFromFile(file);
    std::fclose(file);

    if (image == nullptr) {
        return 2;
    }

    std::uint16_t *readPixels = static_cast<std::uint16_t *>(image->pixels);
    const bool ok = image->width == 2 && image->height == 1 && image->pixelCount == 2 &&
                    image->pitchWords == 2 && image->headerFlagsByte == 0x12 &&
                    image->textureAddressFlagsPacked == 0x3456 && image->paletteMetaPacked == 0 &&
                    (image->formatFlagsPacked & 0x21) == 0x21 && readPixels[0] == 0x3fff &&
                    readPixels[1] == 0x001f;

    zVid_Image::Destroy(image);
    return ok ? 0 : 3;
}

extern "C" int zvideo_palette_remap_no_recipes_smoke(void) {
    std::uint16_t palette[2] = {1, 2};
    g_zVid_PaletteRemapRecipeCount = 0;
    g_zVid_PaletteRemapRecipes = nullptr;
    return zVid_PaletteRemap_BuildAllRecipeVariantsForPalette(palette, 2) == palette ? 0 : 1;
}

extern "C" int zvideo_palette_remap_recipe_variants_smoke(void) {
    g_zVideo_PixelPack_RMaskShifted = 0xf8;
    g_zVideo_PixelPack_GMaskShifted = 0xfc;
    g_zVideo_PixelPack_RShift = 8;
    g_zVideo_PixelPack_GShift = 3;
    g_zVideo_PixelPack_BShiftTo8 = 3;
    g_zVideo_PixelPack_RBits = 5;
    g_zVideo_PixelPack_GBits = 6;
    g_zVideo_PixelPack_BBits = 5;

    zVidPaletteRemapRecipe recipe{};
    std::uint16_t source[2] = {0x0000, 0xffff};
    std::uint16_t directDest[2] = {0x1111, 0x2222};
    zVid_PaletteRemap::ApplyRecipeToPaletteVariant(&recipe, source, 2, 31, directDest);
    const bool directOk = directDest[0] == 0x0000 && directDest[1] == 0xffff;

    std::uint16_t *palette = static_cast<std::uint16_t *>(std::malloc(0x200));
    if (palette == nullptr) {
        return 1;
    }
    std::memset(palette, 0, 0x200);
    palette[0] = 0x0000;
    palette[1] = 0xffff;

    g_zVid_PaletteRemapRecipeCount = 1;
    g_zVid_PaletteRemapRecipes = &recipe;
    std::uint16_t *expanded = zVid_PaletteRemap_BuildAllRecipeVariantsForPalette(palette, 2);
    if (expanded == nullptr) {
        g_zVid_PaletteRemapRecipeCount = 0;
        g_zVid_PaletteRemapRecipes = nullptr;
        return 2;
    }

    const int firstVariant = 0x200 / sizeof(std::uint16_t);
    const int lastVariant = firstVariant + 31 * (0x200 / sizeof(std::uint16_t));
    const bool buildOk = expanded[0] == 0x0000 && expanded[1] == 0xffff &&
                         expanded[firstVariant] == 0x0000 &&
                         expanded[firstVariant + 1] == 0xffff &&
                         expanded[lastVariant] == 0x0000 &&
                         expanded[lastVariant + 1] == 0xffff;

    std::free(expanded);
    g_zVid_PaletteRemapRecipeCount = 0;
    g_zVid_PaletteRemapRecipes = nullptr;
    return directOk && buildOk ? 0 : 3;
}

extern "C" int zvideo_texture_pack_load_image_smoke(void) {
    char tempDir[MAX_PATH] = {};
    char packPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "ztp", 0, packPath) == 0) {
        return 1;
    }

    zVidTexturePackHeader packHeader{};
    packHeader.fileFormat = 1;
    packHeader.recordCount = 1;
    zVidTexturePackRecord record{};
    std::strcpy(record.name, "font.tex");
    record.fileOffset = sizeof(packHeader) + sizeof(record);
    record.paletteIndex = -1;

    unsigned char imageHeader[0x10] = {};
    imageHeader[0] = 1;
    *reinterpret_cast<std::int16_t *>(&imageHeader[4]) = 1;
    *reinterpret_cast<std::int16_t *>(&imageHeader[6]) = 1;
    std::uint16_t pixel = 0x1234;

    FILE *out = std::fopen(packPath, "wb");
    if (out == nullptr) {
        DeleteFileA(packPath);
        return 2;
    }
    std::fwrite(&packHeader, sizeof(packHeader), 1, out);
    std::fwrite(&record, sizeof(record), 1, out);
    std::fwrite(imageHeader, 1, sizeof(imageHeader), out);
    std::fwrite(&pixel, sizeof(pixel), 1, out);
    std::fclose(out);

    zVidTexturePackEntry entry{};
    std::strcpy(entry.filePath, packPath);
    g_zVid_TexturePackLoadState = 1;
    g_zVideo_PixelPack_RBits = 0;
    if (zVid_TexturePackEntry_LoadFromFile(&entry) == nullptr) {
        DeleteFileA(packPath);
        return 3;
    }

    g_zVid_TexturePacks = &entry;
    g_zVid_TexturePackCount = 1;
    zVidImagePartial *image = zVid_TexturePack_LoadImageByName("font.tex");
    const bool ok = image != nullptr && image->width == 1 && image->height == 1 &&
                    image->pixelCount == 1 &&
                    static_cast<std::uint16_t *>(image->pixels)[0] == pixel;

    zVid_Image::Destroy(image);
    std::fclose(entry.fileHandle);
    std::free(entry.records);
    g_zVid_TexturePacks = nullptr;
    g_zVid_TexturePackCount = 0;
    g_zVid_TexturePackLoadState = 0;
    DeleteFileA(packPath);
    return ok ? 0 : 4;
}

extern "C" int zvideo_image_surface_helpers_guard_smoke(void) {
    zVidHwApiDeviceRecordPartial selectedDevice{};
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    g_zVideo_UseHalfResBackbuffer = 0;

    zVidImagePartial image{};
    if (zVideo_dd::Image_LazyCreateVideoMemorySurface(&image) != nullptr) {
        return 1;
    }

    image.surface = reinterpret_cast<IDirectDrawSurface3 *>(0x1234);
    image.pixels = reinterpret_cast<void *>(0x5678);
    g_zVideo_IsInitialized = 0;
    zVideo_dd::Image_EnsureSurfaceForCurrentDevice(&image);
    if (image.surface != nullptr || image.pixels != nullptr) {
        return 2;
    }

    HDC hdc = nullptr;
    g_zVideo_RendererType = 2;
    if (zVideo_dd::Image_UploadPixelsToSurface(&image, &hdc) != 0) {
        return 3;
    }

    return zVideo_dd::Image_ReleaseSurface(&image, hdc) == 0 ? 0 : 4;
}

extern "C" int zvideo_restore_display_surfaces_null_smoke(void) {
    g_zVideo_DisplayModeSurfaceState.surf = nullptr;
    g_zVideo_PrimarySurfaceState.surf = nullptr;
    g_zVideo_SwSurfaceState.surf = nullptr;
    return zVideo_dd::RestoreDisplaySurfaces();
}

extern "C" int zvideo_select_hw_api_device_smoke(void) {
    g_zVideo_pSelectedHwApiDeviceRecord = nullptr;
    g_zVideo_pSelectedD3DDeviceInfo = nullptr;

    if (zVideo::SelectHwApiDeviceOrFallback(-1) != 0 || g_zVideo_RendererType != 0 ||
        g_zVideo_FullscreenOption != 1 ||
        g_zVideo_pSelectedHwApiDeviceRecord != &g_zVideo_HwApiDeviceTable[0] ||
        g_zVideo_pSelectedD3DDeviceInfo != nullptr) {
        return 1;
    }

    if (zVideo::SelectHwApiDeviceOrFallback(2) != 1 || g_zVideo_RendererType != 1 ||
        g_zVideo_FullscreenOption != 1 ||
        g_zVideo_pSelectedHwApiDeviceRecord != &g_zVideo_HwApiDeviceTable[2] ||
        g_zVideo_pSelectedD3DDeviceInfo != g_zVideo_HwApiDeviceTable[2].m_d3dDrivers) {
        return 2;
    }

    return 0;
}

extern "C" int zvideo_return_success_stub_smoke(void) {
    return zVideo::ReturnSuccessStub();
}

extern "C" int zvid_cached_client_rect_smoke(void) {
    zVid::SetCachedClientRectUpdateMask(0x55);
    g_zVideo_ActiveRendererPath = 1;
    if (zVid_QueryCachedClientRectUpdateMaskIf3dfx() != 0) {
        return 1;
    }

    g_zVideo_ActiveRendererPath = 2;
    if (zVid_QueryCachedClientRectUpdateMaskIf3dfx() != 0x55) {
        return 2;
    }

    HWND hwnd = CreateWindowExA(0, "STATIC", "recoil-video-test", WS_OVERLAPPEDWINDOW, 20, 30, 160,
                                120, nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
    if (hwnd == nullptr) {
        return 3;
    }

    RECT client{};
    GetClientRect(hwnd, &client);
    g_zVideo_hWnd = hwnd;
    g_zVideo_CachedClientRectScreen = {100, 100, 100, 100};

    const std::int32_t result = zVideo::UpdateCachedClientRectScreenCoords();
    const LONG cachedWidth =
        g_zVideo_CachedClientRectScreen.right - g_zVideo_CachedClientRectScreen.left;
    const LONG cachedHeight =
        g_zVideo_CachedClientRectScreen.bottom - g_zVideo_CachedClientRectScreen.top;

    g_zVideo_CachedClientRectScreen = {7, 8, 9, 10};
    zVid::SetCachedClientRectUpdateMask(0);
    zVid_UpdateCachedClientRectIfUpdateMaskEnabled();
    const bool noUpdateOk =
        g_zVideo_CachedClientRectScreen.left == 7 && g_zVideo_CachedClientRectScreen.top == 8 &&
        g_zVideo_CachedClientRectScreen.right == 9 && g_zVideo_CachedClientRectScreen.bottom == 10;

    zVid::SetCachedClientRectUpdateMask(1);
    zVid_UpdateCachedClientRectIfUpdateMaskEnabled();
    const LONG helperWidth =
        g_zVideo_CachedClientRectScreen.right - g_zVideo_CachedClientRectScreen.left;
    const LONG helperHeight =
        g_zVideo_CachedClientRectScreen.bottom - g_zVideo_CachedClientRectScreen.top;

    DestroyWindow(hwnd);
    return result == 0 && cachedWidth == client.right - client.left &&
                   cachedHeight == client.bottom - client.top && noUpdateOk &&
                   helperWidth == client.right - client.left &&
                   helperHeight == client.bottom - client.top
               ? 0
               : 4;
}

extern "C" int zvideo_restore_iconic_fullscreen_window_smoke(void) {
    HWND hwnd = CreateWindowExA(0, "STATIC", "recoil-video-iconic-test", WS_OVERLAPPEDWINDOW, 20,
                                30, 160, 120, nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
    if (hwnd == nullptr) {
        return 1;
    }

    g_zVideo_hWnd = hwnd;
    g_zVideo_IsInitialized = 1;
    g_zVideo_FullscreenOption = 1;
    ShowWindow(hwnd, SW_MINIMIZE);
    zVideo_RestoreIconicFullscreenWindowIfNeeded();
    const bool restored = IsIconic(hwnd) == 0;

    ShowWindow(hwnd, SW_MINIMIZE);
    g_zVideo_FullscreenOption = 0;
    zVideo_RestoreIconicFullscreenWindowIfNeeded();
    const bool skipped = IsIconic(hwnd) != 0;

    DestroyWindow(hwnd);
    return restored && skipped ? 0 : 2;
}

namespace {
int g_videoShutdownCalls;

void RECOIL_CDECL VideoShutdownFake() {
    ++g_videoShutdownCalls;
}
} // namespace

extern "C" int zvideo_shutdown_video_system_smoke(void) {
    g_videoShutdownCalls = 0;
    g_zVideo_IsInitialized = 0;
    g_zVideo_pfnShutdownVideoSystem = VideoShutdownFake;

    if (zVideo::ShutdownVideoSystem() != 0x5a560000 || g_videoShutdownCalls != 0) {
        return 1;
    }

    g_zVideo_IsInitialized = 1;
    const std::int32_t result = zVideo::ShutdownVideoSystem();
    return result == 0 && g_zVideo_IsInitialized == 0 && g_videoShutdownCalls == 1 ? 0 : 2;
}
