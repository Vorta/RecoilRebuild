#include "GameZRecoil/zVideo/zVideo.h"

#include "GameZRecoil/include/zClipRect.h"
#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "zClass.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {
typedef int (RECOIL_FASTCALL *zVideo_SurfaceStateProc)(zVideo_SurfaceStatePartial *surfaceState);
typedef void (RECOIL_FASTCALL *zVideo_ClearZBufferRectProc)(zVidRect32 *rect);
typedef void (RECOIL_FASTCALL *zVideo_ImageProc)(zVidImagePartial *image);
} // namespace

extern "C" {
int g_zVideo_PixelPack_RShift = 0;
int g_zVideo_PixelPack_GShift = 0;
int g_zVideo_PixelPack_BShiftTo8 = 0;
int g_zVideo_PixelPack_RMaskShifted = 0;
int g_zVideo_PixelPack_GMaskShifted = 0;
int g_zVideo_PixelPack_BMaskShifted = 0;
int g_zVideo_PixelPack_RBits = 0;
int g_zVideo_PixelPack_GBits = 0;
int g_zVideo_PixelPack_BBits = 0;
unsigned int g_zVideo_PixelPack_RMask = 0;
unsigned int g_zVideo_PixelPack_GMask = 0;
unsigned int g_zVideo_PixelPack_BMask = 0;
int g_zVideo_TexturePixelPack_RBits = 0;
int g_zVideo_TexturePixelPack_GBits = 0;
int g_zVideo_TexturePixelPack_BBits = 0;
int g_zVideo_TexturePixelPack_ABits = 0;
unsigned int g_zVideo_TexturePixelPack_RMask = 0;
unsigned int g_zVideo_TexturePixelPack_GMask = 0;
unsigned int g_zVideo_TexturePixelPack_BMask = 0;
unsigned int g_zVideo_TexturePixelPack_AMask = 0;
int g_zVideo_TexturePixelPack_RGBBitsTotal = 0;
int g_zVideo_TexturePixelPack_RGBBitsTotalMinus8 = 0;
int g_zVideo_TexturePixelPack_GBBitsTotalMinus8 = 0;
int g_zVideo_TexturePixelPack_BShiftTo8 = 0;
int g_zVideo_TexturePixelPack_RMaskShifted = 0;
int g_zVideo_TexturePixelPack_GMaskShifted = 0;
int g_zVideo_TexturePixelPack_BMaskShifted = 0;
int g_zVideo_TexturePixelPack_NonRgbMaskShifted = 0;
int g_zVid_PaletteRemapRecipeCount = 0;
zVidPaletteRemapRecipe *g_zVid_PaletteRemapRecipes = 0;
int g_zVideo_RendererType = 0;
int g_zVideo_ActiveRendererPath = 0;
int g_zVideo_FrameTick = 0;
zClass_CameraDataPartial *g_zVideo_pActiveViewContext = 0;
zTag4Partial g_zVideo_ActiveViewVariantTag = {0};
float g_zVideo_ProjectClipLeft = 0.0f;
float g_zVideo_ProjectClipTop = 0.0f;
float g_zVideo_ProjectClipRight = 0.0f;
float g_zVideo_ProjectClipBottom = 0.0f;
int gVideo_resolutionMenuValid = 0;
unsigned char g_zVideo_PaletteBrightnessLevel = 0;
unsigned int g_zVideo_ClearColorPacked16 = 0;
int g_zVideo_ClearScreenBufferEnabled = 0;
int g_zVid_CachedClientRectUpdateMask = 0;
int g_zVideo_IsInitialized = 0;
int g_zVideo_AdjustSurfacesDisableGate = 0;
int g_zVideo_FullscreenOption = 0;
int g_zVideo_PrimaryHasAttachedBackbuffer = 0;
int g_zVideo_UseHalfResBackbuffer = 0;
int g_zVideo_HalfResAdjustMode = 0;
int g_zVideo_CachedFogModeLightState = 0;
int g_zVideo_CachedFogEnableRenderState = 0;
float g_zVideo_CachedFogStartLightStateValue = 0.0f;
float g_zVideo_CachedFogEndLightStateValue = 0.0f;
float g_zVideo_FogColorPendingR255 = 0.0f;
float g_zVideo_FogColorPendingG255 = 0.0f;
float g_zVideo_FogColorPendingB255 = 0.0f;
float g_zVideo_FogTargetColorR255 = 0.0f;
float g_zVideo_FogTargetColorG255 = 0.0f;
float g_zVideo_FogTargetColorB255 = 0.0f;
float g_zVideo_FogColorAppliedR255 = 0.0f;
float g_zVideo_FogColorAppliedG255 = 0.0f;
float g_zVideo_FogColorAppliedB255 = 0.0f;
int g_zVideo_PendingDitherEnable = 0;
float g_zVideo_InverseZTolerancePending = 0.0f;
int g_zVideo_D3DAppendFanCloseVertexPending = 0;
int g_zVideo_PendingWireframeState = 0;
int g_zVid_AcceptedHardwareRendererCount = 0;
int g_zVideo_NumAcceptedDirectDrawDevices = 0;
int g_zVideo_DirectDrawEnumOrdinal = 0;
int g_zVid_TexturePackLoadState = 0;
int g_zVid_BuiltinTexturePackCount = 0;
zVidTexturePackEntry *g_zVid_BuiltinTexturePacks = 0;
int g_zVid_TexturePackCount = 0;
zVidTexturePackEntry *g_zVid_TexturePacks = 0;
int g_zVid_PaletteRemapVariantTableCount = 0;
unsigned short **g_zVid_PaletteRemapVariantTables = 0;
int *ZOPT_VIDEO_MODE = 0;
int *ZOPT_VIDEO_ACCELERATION = 0;
int *ZOPT_HW_API = 0;
DDCAPS g_zVideo_DDrawCapsHal = {0};
DDCAPS g_zVideo_DDrawCapsHel = {0};
zVideo_TextureRecordPartial *g_zImage_DefaultTextureRecord = 0;
PALETTEENTRY g_zVideo_SystemPaletteEntries[0x100] = {0};
zVideo_StatusProc g_zVideo_pfnOpenVideoMode = 0;
zVideo_ShutdownVideoSystemProc g_zVideo_pfnShutdownVideoSystem = 0;
unsigned int g_zVideo_pfnPaletteSetEntries = 0;
zVideo_StatusProc g_zVideo_pfnSetVideoMode = 0;
zVideo_AdjustSurfacesProc g_zVideo_pfnAdjustSurfaces = 0;
zVideo_SurfaceStateProc g_zVideo_pfnLockSurfaceState = 0;
zVideo_SurfaceStateProc g_zVideo_pfnUnlockSurfaceState = 0;
zVideo_ClearZBufferRectProc g_zVideo_pfnClearZBufferRect = 0;
zVideo_ClearSwSurfaceAndZBufferProc g_zVideo_pfnClearSwSurfaceAndZBuffer = 0;
zVideo_ClearStateSurfaceAndZBufferProc g_zVideo_pfnClearStateSurfaceAndZBuffer = 0;
unsigned int g_zVideo_pfnUpdateFogColor = 0;
zVideo_QueryMemoryBytesProc g_zVideo_pfnQueryTextureMemoryBytes = 0;
zVideo_QueryMemoryBytesProc g_zVideo_pfnQueryDeviceVideoMemoryBytes = 0;
zVideo_BltRectDirectProc g_zVideo_pfnBltSwToPrimaryRectDirect = 0;
zVideo_BltRectDirectProc g_zVideo_pfnBltPrimaryToSwRectDirect = 0;
unsigned int g_zVideo_pfnBltSwToPrimaryRect = 0;
unsigned int g_zVideo_pfnGetHwApiDeviceFeatureFlags = 0;
unsigned int g_zVideo_pfnImageUploadPixelsToSurface = 0;
unsigned int g_zVideo_pfnImageReleaseSurface = 0;
zVideo_CreateTextureRecordProc g_zVideo_pfnCreateTextureRecord = 0;
unsigned int g_zVideo_pfnTextureRecordLockUploadSurface = 0;
unsigned int g_zVideo_pfnTextureRecordUnlockUploadSurface = 0;
unsigned int g_zVideo_pfnTextureRecordReleaseUploadSurfaceRef = 0;
unsigned int g_zVideo_pfnTextureRecordFinalizeUpload = 0;
unsigned int g_zVideo_pfnTextureRecordDestroy = 0;
unsigned int g_zVideo_pfnTextureRecordReleaseAllUploadSurfaces = 0;
unsigned int g_zVideo_pfnImageLazyCreateVideoMemorySurface = 0;
unsigned int g_zVideo_pfnImageEnsureSurfaceForCurrentDevice = 0;
unsigned int g_zVideo_pfnSetFogEnable = 0;
unsigned int g_zVideo_pfnSetFogStart = 0;
unsigned int g_zVideo_pfnSetFogEnd = 0;
unsigned int g_zVideo_pfnApplyFogStateFromGlobals = 0;
unsigned int g_zVideo_pfnSubmitPolyFlatColor16 = 0;
unsigned int g_zVideo_pfnSubmitPolyGouraudColor16 = 0;
unsigned int g_zVideo_pfnSubmitPolyColorAttr = 0;
unsigned int g_zVideo_pfnSubmitPolyRenderClass = 0;
unsigned int g_zVideo_pfnSubmitPolygon = 0;
unsigned int g_zVideo_pfnSubmitPolygonLit = 0;
unsigned int g_zVideo_pfnDrawPointColor16 = 0;
unsigned int g_zVideo_pfnFlushSortedPolys = 0;
unsigned int g_zVideo_pfnFlushOverwritePolys = 0;
unsigned int g_zVideo_pfnFlushQuadBatch = 0;
zVidHwApiDeviceRecordPartial g_zVideo_HwApiDeviceTable[4] = {0};
zVidHwApiDeviceRecordPartial *g_zVideo_pSelectedHwApiDeviceRecord = 0;
zVidD3DDriverRecordPartial *g_zVideo_pSelectedD3DDeviceInfo = 0;
D3DDEVICEDESC g_zVideo_D3DHalDeviceDesc = {0};
D3DDEVICEDESC g_zVideo_D3DHelDeviceDesc = {0};
D3DMATERIALHANDLE g_zVideo_D3DMaterialHandle = 0;
int g_zVideo_QuadBatchCount = 0;
zVideo_QuadBatchItemPartial g_zVideo_QuadBatchItemsBase[16] = {0};
D3DTLVERTEX g_zVideo_D3DSubmitTempVertices[64] = {0};
int g_zVideo_SortedPolyDrawOrder[256] = {0};
zVideo_SortedPolyQueueEntry g_zVideo_SortedPolyQueueBase[256] = {0};
zVideo_OverwriteQueueEntry g_zVideo_OverwriteQueueBase[0x180] = {0};
int g_zVideo_SortedPolyQueueCount = 0;
int g_zVideo_OverwriteQueueCount = 0;
D3DTEXTUREHANDLE g_zVideo_D3DRenderState_TextureHandle = 0;
int g_zVideo_D3DRenderState_ShadeMode = 0;
int g_zVideo_D3DRenderState_AlphaBlendEnable = 0;
int g_zVideo_D3DRenderState_ZWriteEnable = 0;
D3DTEXTUREBLEND g_zVideo_D3DRenderState_TextureMapBlend = static_cast<D3DTEXTUREBLEND>(0);
D3DTEXTUREADDRESS g_zVideo_D3DRenderState_TextureAddressU = static_cast<D3DTEXTUREADDRESS>(0);
D3DTEXTUREADDRESS g_zVideo_D3DRenderState_TextureAddressV = static_cast<D3DTEXTUREADDRESS>(0);
int g_zVideo_D3DColorNormalizeChannelIndex = 0;
float g_zVideo_D3DColorAttrBiasR = 0.0f;
float g_zVideo_D3DColorAttrBiasG = 0.0f;
float g_zVideo_D3DColorAttrBiasB = 0.0f;
IDirect3DMaterial2 *g_zVideo_pD3DMaterial2 = 0;
IDirect3DViewport2 *g_zVideo_pD3DViewport2 = 0;
IDirect3DDevice2 *g_zVideo_pD3DDevice = 0;
IDirect3D2 *g_zVideo_pD3D2 = 0;
IDirectDrawClipper *g_zVideo_pClipper = 0;
IDirectDraw2 *g_zVideo_pDirectDraw2 = 0;
IDirectDrawSurface3 *g_zVideo_pZBufferSurface = 0;
IDirectDrawSurface *g_zVideo_pZBufferAttachSurface = 0;
IDirectDrawSurface3 *g_zVideo_pPageUnlockSurface = 0;
zVideo_SurfaceLockVerifier *g_zVideo_pSurfaceLockVerifier = 0;
int g_zVideo_SurfaceLockVerifyContext = 0;
unsigned char g_zVideo_SurfaceLockVerifyFlags = 0;
zVideo_SurfaceStatePartial g_zVideo_SwSurfaceState = {0};
zVideo_SurfaceStatePartial g_zVideo_PrimarySurfaceState = {0};
zVideo_SurfaceStatePartial g_zVideo_DisplayModeSurfaceState = {0};
struct zVideoFxPass3RootElement {
    HudUiElement base;
    unsigned char unknown_34[0x04];
    unsigned short packedColor16;
    unsigned char unknown_3a[0x06];
    double alpha;
};

struct zVideoFxPass3Slot {
    HudUiElement base;
    unsigned char unknown_34[0x04];
    float currentRadius;
    float maxRadius;
    float extent;
    float sinFreq;
    float sinPhase;

    RECOIL_NOINLINE void RECOIL_THISCALL SetRectAndPayload(int rectLeftPixels,
                                                           int rectTopPixels,
                                                           int currentRadiusPixels,
                                                           int maxRadiusPixels,
                                                           int extentPixels,
                                                           float sinFreqValue, float sinPhaseValue);
};

struct zVideoFxPass3Config {
    void *vptr;
    int enabled;
    HudUiElement *childHead;
    HudUiElement *childTail;
    HudUiRect *inputRectsOrNull[2];
    unsigned short *surfacePixels;
    int surfaceWidth;
    int surfaceHeight;
    int surfacePitchBytes;
    zVideoFxPass3RootElement rootElement;
    zVideoFxPass3Slot slots[5];
    int slotWriteIndex;

    RECOIL_NOINLINE void RECOIL_THISCALL SetInputRectByIndex(int index,
                                                             HudUiRect *rectOrNull);
    RECOIL_NOINLINE void RECOIL_THISCALL QueuePrimitiveRaw(void *primitive,
                                                           int width,
                                                           int height,
                                                           int pitchBytes);
};
#if defined(_M_IX86) || defined(__i386__)
RECOIL_STATIC_ASSERT(sizeof(zVideoFxPass3RootElement) == 0x48);
RECOIL_STATIC_ASSERT(offsetof(zVideoFxPass3RootElement, packedColor16) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(zVideoFxPass3RootElement, alpha) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(zVideoFxPass3Config, rootElement) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(zVideoFxPass3Slot, currentRadius) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(zVideoFxPass3Slot, sinPhase) == 0x48);
RECOIL_STATIC_ASSERT(sizeof(zVideoFxPass3Slot) == 0x4c);
RECOIL_STATIC_ASSERT(offsetof(zVideoFxPass3Config, slots) == 0x70);
RECOIL_STATIC_ASSERT(offsetof(zVideoFxPass3Config, slotWriteIndex) == 0x1ec);
#endif
zVideoFxPass3Config g_zVideo_FxPass3ConfigLocal = {0};
zVidRect32 g_zVideo_PrimarySurfaceRectScratch = {0};
int g_zVideo_DisplayModeBpp = 0;
int g_zVid_NoiseByteTableSize = 0;
unsigned char *g_zVid_NoiseByteTable = 0;
unsigned short *g_zVideo_FxPass3_ScratchPixels16 = 0;
unsigned short *g_zVideo_FxSurfacePixels16 = 0;
int g_zVideo_FxSurfaceWidth = 0;
int g_zVideo_FxSurfaceHeight = 0;
int g_zVideo_FxSurfacePitchBytes = 0;
int g_zVideo_FxSurfacePitchPixels16 = 0;
IDirectDrawPalette *g_zVideo_pDDPalette = 0;
HWND g_zVideo_hWnd = 0;
RECT g_zVideo_CachedClientRectScreen = {0};

// Reimplements 0x4a6cf0: zVid_PackColorRGB
RECOIL_NOINLINE unsigned int RECOIL_FASTCALL zVid_PackColorRGB(unsigned char red,
                                                                unsigned char green,
                                                                unsigned char blue) {
    return ((g_zVideo_PixelPack_GMaskShifted & green) << g_zVideo_PixelPack_GShift) |
           ((g_zVideo_PixelPack_RMaskShifted & red) << g_zVideo_PixelPack_RShift) |
           (blue >> g_zVideo_PixelPack_BShiftTo8);
}

// Reimplements 0x4a6ca0: zVid_PackColor00RRGGBB
RECOIL_NOINLINE unsigned int RECOIL_FASTCALL zVid_PackColor00RRGGBB(unsigned int color00RRGGBB) {
    const unsigned char red = static_cast<unsigned char>(color00RRGGBB);
    const unsigned char green = static_cast<unsigned char>(color00RRGGBB >> 8);
    const unsigned char blue = static_cast<unsigned char>(color00RRGGBB >> 16);

    return ((g_zVideo_PixelPack_GMaskShifted & green) << g_zVideo_PixelPack_GShift) |
           ((g_zVideo_PixelPack_RMaskShifted & red) << g_zVideo_PixelPack_RShift) |
           (blue >> g_zVideo_PixelPack_BShiftTo8);
}

// Reimplements 0x4a6d40: zVid_PackColorRgbFloats
RECOIL_NOINLINE unsigned short RECOIL_FASTCALL zVid_PackColorRgbFloats(zVideo_ColorRgbFloat *color) {
    const int red = static_cast<int>(color->r + 0.5f);
    const int green = static_cast<int>(color->g + 0.5f);
    const int blue = static_cast<int>(color->b + 0.5f);
    const unsigned int packed =
        ((g_zVideo_PixelPack_RMaskShifted & red) << g_zVideo_PixelPack_RShift) |
        ((g_zVideo_PixelPack_GMaskShifted & green) << g_zVideo_PixelPack_GShift) |
        (static_cast<unsigned int>(blue) >> g_zVideo_PixelPack_BShiftTo8);
    return static_cast<unsigned short>(packed);
}

// Reimplements 0x4a6b80: zVideo_SetClearColorPacked16
RECOIL_NOINLINE void RECOIL_FASTCALL zVideo_SetClearColorPacked16(unsigned int packedColor16) {
    g_zVideo_ClearColorPacked16 = packedColor16;
}

// Reimplements 0x4a7250: zVideo_SetPendingFogTargetColorFromRgb01
RECOIL_NOINLINE void RECOIL_FASTCALL
zVideo_SetPendingFogTargetColorFromRgb01(zVideo_ColorRgbFloat *color) {
    g_zVideo_D3DColorAttrBiasR = color->r * 255.0f;
    g_zVideo_D3DColorAttrBiasG = color->g * 255.0f;
    g_zVideo_D3DColorAttrBiasB = color->b * 255.0f;
    if (g_zVideo_RendererType == 0) {
        return;
    }

    if (g_zVideo_D3DColorAttrBiasR >= g_zVideo_D3DColorAttrBiasG) {
        g_zVideo_D3DColorNormalizeChannelIndex =
            g_zVideo_D3DColorAttrBiasR >= g_zVideo_D3DColorAttrBiasB ? 0 : 2;
        return;
    }

    if (g_zVideo_D3DColorAttrBiasG >= g_zVideo_D3DColorAttrBiasB) {
        g_zVideo_D3DColorNormalizeChannelIndex = 1;
        return;
    }

    g_zVideo_D3DColorNormalizeChannelIndex =
        g_zVideo_D3DColorAttrBiasR >= g_zVideo_D3DColorAttrBiasB ? 0 : 2;
}

// Reimplements 0x479ce0: zVideo_SetActiveViewContext
// (D:\Proj\GameZRecoil\zVideo\zVideo.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
zVideo_SetActiveViewContext(zClass_CameraDataPartial *viewContext) {
    g_zVideo_pActiveViewContext = viewContext;

    if (viewContext->nearClip < 1.0f) {
        viewContext->nearClip = 1.0f;
    }

    gClipRect_Primary.zMin = viewContext->nearClip + viewContext->nearClip;
    if (g_zVideo_ActiveRendererPath == 0) {
        zVideo_dd3d::SetQuadBatchDepthAndRhw(1.0f / gClipRect_Primary.zMin);
        viewContext = g_zVideo_pActiveViewContext;
    }

    gClipRect_Primary.zMax = viewContext->farClip;

    int windowX;
    int windowY;
    if (zClass_Window::gwWindowGetSize(viewContext->windowNode, &windowX, &windowY) != 0) {
        windowX = 0;
        windowY = 0;
    }

    viewContext = g_zVideo_pActiveViewContext;
    int width;
    int height;
    if (zClass_Window::gwWindowGetResolution(viewContext->windowNode, &width, &height) != 0) {
        width = zVideo::GetPrimarySurfaceWidth();
        height = zVideo::GetPrimarySurfaceHeight();
    }

    const int rightPx = windowX + width;
    const int bottomPx = windowY + height;
    const float left = static_cast<float>(windowX);
    const float top = static_cast<float>(windowY);
    const float right = static_cast<float>(rightPx);
    const float bottom = static_cast<float>(bottomPx);
    float viewportOriginX;
    float viewportOriginY;
    float viewportBottom;
    float projectClipLeft;

    if (g_zVideo_ActiveRendererPath == 0) {
        viewportOriginX = left;
        viewportOriginY = top;
        viewportBottom = bottom;
        projectClipLeft = left;
        gClipRect_Primary.xMin = left + 0.5f - 0.999000013f;
        gClipRect_Primary.xMax = right + 1.49900007f;
        gClipRect_Primary.xMaxAlt = right + 0.5f - 0.00100000005f;
        gClipRect_Primary.yMin = top + 0.5f - 0.999000013f;
        gClipRect_Primary.yMax = bottom + 1.49900007f;
        gClipRect_Primary.yMaxAlt = bottom + 0.5f - 0.00100000005f;
    } else {
        const float rightWithSlop = right + 0.00100000005f;
        const float bottomWithSlop = bottom + 0.00100000005f;
        viewportOriginX = left;
        viewportOriginY = top;
        viewportBottom = bottomWithSlop;
        projectClipLeft = left;
        gClipRect_Primary.xMin = left;
        gClipRect_Primary.xMax = rightWithSlop;
        gClipRect_Primary.xMaxAlt = rightWithSlop;
        gClipRect_Primary.yMin = top;
        gClipRect_Primary.yMax = bottomWithSlop;
        gClipRect_Primary.yMaxAlt = bottomWithSlop;
    }

    g_zVideo_ProjectClipLeft = projectClipLeft;
    g_zVideo_ProjectClipTop = viewportOriginY;
    g_zVideo_ProjectClipRight = right - 0.00100000005f;
    g_zVideo_ProjectClipBottom = viewportBottom - 0.00100000005f;

    viewContext = g_zVideo_pActiveViewContext;
    zMath_Setup_Projection(viewportOriginX, viewportOriginY, static_cast<float>(width) * 0.5f,
                           static_cast<float>(height) * 0.5f, viewContext->viewportScaleX,
                           viewContext->viewportScaleY, viewContext->nearClip,
                           viewContext->farClip);

    int fovXBits;
    int fovYBits;
    memcpy(&fovXBits, &viewContext->fovX, sizeof(fovXBits));
    memcpy(&fovYBits, &viewContext->fovY, sizeof(fovYBits));
    zMath_SetScreenSize(fovXBits, fovYBits);
}

// Reimplements 0x47a0c0: zVideo_UpdateProjectionStateFromCameraData
// (GameZRecoil/zVideo/zVideo.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
zVideo_UpdateProjectionStateFromCameraData(zClass_CameraDataPartial *cameraData) {
    zMat4x3 slotBuffer = {0};
    zMath::MatStackPushPtr(reinterpret_cast<float *>(&slotBuffer));
    zMath::MatLoadIdentity();

    zMat4x3 yawSlotBuffer = {0};
    zMath::MatStackPushAndCloneParent(reinterpret_cast<float *>(&yawSlotBuffer));
    cameraData->localFrustumLeftNormal.x = 1.0f;
    cameraData->localFrustumLeftNormal.y = 0.0f;
    cameraData->localFrustumLeftNormal.z = 0.0f;
    zMath::MatRotateY(cameraData->frustumYaw);
    zMath_Vec3Array_UntransformDirection(&cameraData->localFrustumLeftNormal, 1);
    zMath::MatStackPopPtr();

    cameraData->localFrustumRightNormal.x = -cameraData->localFrustumLeftNormal.x;
    cameraData->localFrustumRightNormal.y = cameraData->localFrustumLeftNormal.y;
    cameraData->localFrustumRightNormal.z = cameraData->localFrustumLeftNormal.z;

    cameraData->localFrustumBottomNormal.x = 0.0f;
    cameraData->localFrustumBottomNormal.y = -1.0f;
    cameraData->localFrustumBottomNormal.z = 0.0f;
    zMath::MatRotateX(cameraData->frustumPitch);
    zMath_Vec3Array_UntransformDirection(&cameraData->localFrustumBottomNormal, 1);
    zMath::MatStackPopPtr();

    cameraData->localFrustumTopNormal.x = cameraData->localFrustumBottomNormal.x;
    cameraData->localFrustumTopNormal.y = -cameraData->localFrustumBottomNormal.y;
    cameraData->localFrustumTopNormal.z = cameraData->localFrustumBottomNormal.z;

    cameraData->localFrustumNearNormal.x = 0.0f;
    cameraData->localFrustumNearNormal.y = 0.0f;
    cameraData->localFrustumNearNormal.z = -1.0f;
    cameraData->localFrustumFarNormal.x = 0.0f;
    cameraData->localFrustumFarNormal.y = 0.0f;
    cameraData->localFrustumFarNormal.z = 1.0f;
}

static zVec3 zVideo_SubtractVec3(zVec3 *lhs, zVec3 *rhs) {
    zVec3 delta;
    delta.x = lhs->x - rhs->x;
    delta.y = lhs->y - rhs->y;
    delta.z = lhs->z - rhs->z;
    return delta;
}

static float zVideo_DotVec3(zVec3 *lhs, zVec3 *rhs) {
    return lhs->x * rhs->x + lhs->y * rhs->y + lhs->z * rhs->z;
}

static int zVideo_TestSpherePlane(zVec3 *delta, zVec3 *normal, float radius,
                                           int planeBit,
                                           int *clipMaskInOut) {
    const float dot = zVideo_DotVec3(delta, normal);
    if (-radius >= dot) {
        return planeBit;
    }

    if (dot < radius) {
        *clipMaskInOut |= planeBit;
    }

    return 0;
}

// Reimplements 0x478c70: zVideo_FrustumTestSphereClipMask
// (GameZRecoil/zModel/zModel_Display.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
zVideo_FrustumTestSphereClipMask(zVec3 *sphereCenter, int *clipMaskInOut, float radius) {
    const int oldMask = *clipMaskInOut;
    *clipMaskInOut = 0;

    zClass_CameraDataPartial *viewContext = g_zVideo_pActiveViewContext;
    zVec3 delta;
    if ((oldMask & 0x10) != 0) {
        delta = zVideo_SubtractVec3(sphereCenter, &viewContext->nearClipCenter);
        const float dot = zVideo_DotVec3(&delta, &viewContext->worldFrustumNormals[4]);
        if (dot < radius) {
            if (-radius >= dot) {
                return 0x10;
            }
            *clipMaskInOut = 0x10;
        } else {
            *clipMaskInOut = 0;
        }
    }

    viewContext = g_zVideo_pActiveViewContext;
    delta = zVideo_SubtractVec3(sphereCenter, &viewContext->cameraPos);

    if ((oldMask & 1) != 0) {
        const int result =
            zVideo_TestSpherePlane(&delta, &viewContext->worldFrustumNormals[0], radius, 1,
                                   clipMaskInOut);
        if (result != 0) {
            return result;
        }
    }

    if ((oldMask & 2) != 0) {
        const int result =
            zVideo_TestSpherePlane(&delta, &viewContext->worldFrustumNormals[1], radius, 2,
                                   clipMaskInOut);
        if (result != 0) {
            return result;
        }
    }

    if ((oldMask & 4) != 0) {
        const int result =
            zVideo_TestSpherePlane(&delta, &viewContext->worldFrustumNormals[2], radius, 4,
                                   clipMaskInOut);
        if (result != 0) {
            return result;
        }
    }

    if ((oldMask & 8) != 0) {
        const int result =
            zVideo_TestSpherePlane(&delta, &viewContext->worldFrustumNormals[3], radius, 8,
                                   clipMaskInOut);
        if (result != 0) {
            return result;
        }
    }

    if ((oldMask & 0x20) != 0) {
        viewContext = g_zVideo_pActiveViewContext;
        delta = zVideo_SubtractVec3(sphereCenter, &viewContext->farClipCenter);
        const int result =
            zVideo_TestSpherePlane(&delta, &viewContext->worldFrustumNormals[5], radius, 0x20,
                                   clipMaskInOut);
        if (result != 0) {
            return result;
        }
    }

    return 0;
}

// Reimplements 0x4a59b0: zVid_QueryCachedClientRectUpdateMaskIf3dfx
RECOIL_NOINLINE int RECOIL_CDECL zVid_QueryCachedClientRectUpdateMaskIf3dfx() {
    return g_zVideo_ActiveRendererPath == 2 ? g_zVid_CachedClientRectUpdateMask : 0;
}

// Reimplements 0x443a40: zVid_UpdateCachedClientRectIfUpdateMaskEnabled
RECOIL_NOINLINE void RECOIL_CDECL zVid_UpdateCachedClientRectIfUpdateMaskEnabled() {
    if (zVid_QueryCachedClientRectUpdateMaskIf3dfx() != 0) {
        zVideo::UpdateCachedClientRectScreenCoords();
    }
}

// Reimplements 0x4a7770: zVideo_RestoreIconicFullscreenWindowIfNeeded
RECOIL_NOINLINE void RECOIL_CDECL zVideo_RestoreIconicFullscreenWindowIfNeeded() {
    if (g_zVideo_IsInitialized != 0 && g_zVideo_FullscreenOption != 0 &&
        IsIconic(g_zVideo_hWnd) != 0) {
        OpenIcon(g_zVideo_hWnd);
    }
}
}

RECOIL_STATIC_ASSERT(sizeof(zVidHwApiDeviceRecordPartial) == 0x6ec);
RECOIL_STATIC_ASSERT(sizeof(zVidD3DDriverRecordPartial) == 0x190);
RECOIL_STATIC_ASSERT(offsetof(zVidHwApiDeviceRecordPartial, m_deviceFeatureFlags) == 0xa4);
RECOIL_STATIC_ASSERT(offsetof(zVidHwApiDeviceRecordPartial, m_acceptedD3DDeviceCount) == 0xa8);
RECOIL_STATIC_ASSERT(offsetof(zVidHwApiDeviceRecordPartial, m_d3dDrivers) == 0xac);
RECOIL_STATIC_ASSERT(sizeof(DDCAPS) == 0x17c);
RECOIL_STATIC_ASSERT(sizeof(DDSURFACEDESC) == 0x6c);
RECOIL_STATIC_ASSERT(sizeof(D3DDEVICEDESC) == 0xfc);
RECOIL_STATIC_ASSERT(sizeof(D3DVIEWPORT2) == 0x2c);
RECOIL_STATIC_ASSERT(sizeof(D3DMATERIAL) == 0x50);
RECOIL_STATIC_ASSERT(sizeof(D3DTLVERTEX) == 0x20);
RECOIL_STATIC_ASSERT(sizeof(zVideo_QuadBatchItemPartial) == 0x80);
RECOIL_STATIC_ASSERT(sizeof(zVideo_XyzVertex) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zVideo_ColorRgbFloat) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zVideo_TexCoord) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zVideo_RenderClass, textureHandle) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zVideo_RenderClass, textureMapBlend) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zVideo_RenderClass, textureAddressU) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zVideo_RenderClass, textureAddressV) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(zVideo_RenderClass) == 0x1c);
RECOIL_STATIC_ASSERT(sizeof(zVideo_SortedPolyQueueEntry) == 0x80c);
RECOIL_STATIC_ASSERT(sizeof(zVideo_OverwriteQueueEntry) == 0x810);
RECOIL_STATIC_ASSERT(offsetof(zVideo_SortedPolyQueueEntry, vertices) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zVideo_OverwriteQueueEntry, vertices) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(zVideo_TextureRecordPartial) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zVideo_TextureRecordPartial, m_textureHandle) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zVideo_TextureRecordPartial, m_alphaMode) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zVideo_TextureRecordPartial, m_uWrapMode) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zVideo_TextureRecordPartial, m_vWrapMode) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zVideo_SurfaceStatePartial, locked) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zVideo_SurfaceStatePartial, pageLockActive) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zVideo_SurfaceStatePartial, surf) == 0x1c);
RECOIL_STATIC_ASSERT(sizeof(zVidTexturePackRecord) == 0x28);
RECOIL_STATIC_ASSERT(sizeof(zVidTexturePackHeader) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zVidTexturePackEntry, fileHandle) == 0x80);
RECOIL_STATIC_ASSERT(offsetof(zVidTexturePackEntry, header) == 0x84);
RECOIL_STATIC_ASSERT(offsetof(zVidTexturePackEntry, records) == 0x9c);
RECOIL_STATIC_ASSERT(offsetof(zVidTexturePackEntry, paletteTableBaseIndex) == 0xa0);
RECOIL_STATIC_ASSERT(sizeof(zVidTexturePackEntry) == 0xa4);
RECOIL_STATIC_ASSERT(offsetof(zVidPaletteRemapRecipe, color1R) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zVidPaletteRemapRecipe, color0Strength) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(zVidPaletteRemapRecipe) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zVidImagePartial, width) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zVidImagePartial, formatFlagsPacked) == 0x09);
RECOIL_STATIC_ASSERT(offsetof(zVidImagePartial, uPow2Shift) == 0x0a);
RECOIL_STATIC_ASSERT(offsetof(zVidImagePartial, vPow2Shift) == 0x0b);
RECOIL_STATIC_ASSERT(offsetof(zVidImagePartial, textureAddressFlagsPacked) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zVidImagePartial, paletteMetaPacked) == 0x0e);
RECOIL_STATIC_ASSERT(offsetof(zVidImagePartial, pixels) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zVidImagePartial, alphaMap) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zVidImagePartial, palette) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zVidImagePartial, widthScale) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zVidImagePartial, queuedAlphaMap) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zVidImagePartial, uShiftFrom20) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(zVidImagePartial, uMask) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(zVidImagePartial, vMaskFixed20) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(zVidImagePartial, surface) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zVidImagePartial, pitchWords) == 0x34);
RECOIL_STATIC_ASSERT(sizeof(zVidRect32) == sizeof(RECT));

namespace zVid {
// Reimplements 0x408280: zVid::SetAccelerationOption
RECOIL_NOINLINE void RECOIL_FASTCALL SetAccelerationOption(int accelerationOption) {
    *ZOPT_VIDEO_ACCELERATION = accelerationOption;
    g_zOpt_HwMode = accelerationOption;
}

// Reimplements 0x408290: zVid::SetHwApiOption
RECOIL_NOINLINE void RECOIL_FASTCALL SetHwApiOption(int hwApiOption) {
    *ZOPT_HW_API = hwApiOption;
}

// Reimplements 0x408310: zVid::GetAccelerationOption
RECOIL_NOINLINE int RECOIL_CDECL GetAccelerationOption() {
    return *ZOPT_VIDEO_ACCELERATION;
}

// Reimplements 0x408320: zVid::GetHwApiOption
RECOIL_NOINLINE int RECOIL_CDECL GetHwApiOption() {
    return *ZOPT_HW_API;
}

// Reimplements 0x4a7480: zVid::GetAcceptedDirectDrawDeviceCount
RECOIL_NOINLINE int RECOIL_CDECL GetAcceptedDirectDrawDeviceCount() {
    return zVideo_dd::GetAcceptedDirectDrawDeviceCountCached();
}

// Reimplements 0x4a9910: zVid::GetAcceptedHardwareRendererCount_Cached
RECOIL_NOINLINE int RECOIL_CDECL GetAcceptedHardwareRendererCount_Cached() {
    return g_zVid_AcceptedHardwareRendererCount;
}

// Reimplements 0x4b3220: zVid::HasAcceptedHardwareRenderer
RECOIL_NOINLINE int RECOIL_CDECL HasAcceptedHardwareRenderer() {
    return GetAcceptedHardwareRendererCount_Cached() > 0 ? 1 : 0;
}

// Reimplements 0x46d5c0: zVid::GetTexturePackLoadState
RECOIL_NOINLINE int RECOIL_CDECL GetTexturePackLoadState() {
    return g_zVid_TexturePackLoadState;
}

// Reimplements 0x46d5b0: zVid::SetTexturePackLoadState
RECOIL_NOINLINE void RECOIL_FASTCALL SetTexturePackLoadState(int texturePackLoadState) {
    g_zVid_TexturePackLoadState = texturePackLoadState;
}

// Reimplements 0x4086b0: zVid::GetVideoModeIndexFromOptions
RECOIL_NOINLINE int RECOIL_CDECL GetVideoModeIndexFromOptions() {
    return *ZOPT_VIDEO_MODE;
}

// Reimplements 0x408720: zVid::SetVideoModeIndex
RECOIL_NOINLINE void RECOIL_FASTCALL SetVideoModeIndex(int modeIndex) {
    switch (modeIndex) {
    case 2:
        *ZOPT_VIDEO_MODE = modeIndex;
        zOpt::RenderSection_SetPosition(0, 0);
        zOpt::RenderSection_SetSize(0x140, 0xc8);
        zOpt::WindowSection_SetPosition(0, 0);
        zOpt::WindowSection_SetSize(0x280, 0x190);
        zOpt::DisplaySection_SetPosition(0, 0);
        zOpt::DisplaySection_SetSize(0x280, 0x190);
        zOpt::DisplaySection_SetBitsPerPixel(0x10);
        zOpt::SetReplicateMode(1);
        return;

    case 3:
        *ZOPT_VIDEO_MODE = modeIndex;
        zOpt::RenderSection_SetPosition(0, 0);
        zOpt::RenderSection_SetSize(0x140, 0xf0);
        zOpt::WindowSection_SetPosition(0, 0);
        zOpt::WindowSection_SetSize(0x280, 0x1e0);
        zOpt::DisplaySection_SetPosition(0, 0);
        zOpt::DisplaySection_SetSize(0x280, 0x1e0);
        zOpt::DisplaySection_SetBitsPerPixel(0x10);
        zOpt::SetReplicateMode(1);
        return;

    case 4:
        *ZOPT_VIDEO_MODE = modeIndex;
        zOpt::RenderSection_SetPosition(0, 0);
        zOpt::RenderSection_SetSize(0x280, 0x190);
        zOpt::WindowSection_SetPosition(0, 0);
        zOpt::WindowSection_SetSize(0x280, 0x190);
        zOpt::DisplaySection_SetPosition(0, 0);
        zOpt::DisplaySection_SetSize(0x280, 0x190);
        zOpt::DisplaySection_SetBitsPerPixel(0x10);
        zOpt::SetReplicateMode(0);
        return;

    case 5:
        *ZOPT_VIDEO_MODE = modeIndex;
        zOpt::RenderSection_SetPosition(0, 0);
        zOpt::RenderSection_SetSize(0x280, 0x1e0);
        zOpt::WindowSection_SetPosition(0, 0);
        zOpt::WindowSection_SetSize(0x280, 0x1e0);
        zOpt::DisplaySection_SetPosition(0, 0);
        zOpt::DisplaySection_SetSize(0x280, 0x1e0);
        zOpt::DisplaySection_SetBitsPerPixel(0x10);
        zOpt::SetReplicateMode(0);
        return;

    case 6:
        *ZOPT_VIDEO_MODE = modeIndex;
        zOpt::RenderSection_SetPosition(0, 0);
        zOpt::RenderSection_SetSize(0x320, 0x258);
        zOpt::WindowSection_SetPosition(0, 0);
        zOpt::WindowSection_SetSize(0x320, 0x258);
        zOpt::DisplaySection_SetPosition(0, 0);
        zOpt::DisplaySection_SetSize(0x320, 0x258);
        zOpt::DisplaySection_SetBitsPerPixel(0x10);
        zOpt::SetReplicateMode(0);
        return;

    case 7:
        *ZOPT_VIDEO_MODE = modeIndex;
        zOpt::RenderSection_SetPosition(0, 0);
        zOpt::RenderSection_SetSize(0x400, 0x300);
        zOpt::WindowSection_SetPosition(0, 0);
        zOpt::WindowSection_SetSize(0x400, 0x300);
        zOpt::DisplaySection_SetPosition(0, 0);
        zOpt::DisplaySection_SetSize(0x400, 0x300);
        zOpt::DisplaySection_SetBitsPerPixel(0x10);
        zOpt::SetReplicateMode(0);
        return;

    default:
        return;
    }
}

RECOIL_NOINLINE int RECOIL_FASTCALL QueryDeviceVideoMemoryBytes(
    int deviceIndexOrMinus1, int *totalBytes, int *freeBytes) {
    if (g_zVideo_RendererType == 0) {
        *freeBytes = 0;
        *totalBytes = 0;
        return 0;
    }

    if (deviceIndexOrMinus1 == -1) {
        *totalBytes = 0;
        *freeBytes = 0;
        return 1;
    }

    const zVidHwApiDeviceRecordPartial &device = g_zVideo_HwApiDeviceTable[deviceIndexOrMinus1];
    *totalBytes = device.m_videoMemTotalBytes;
    if (device.m_videoMemTotalBytes == device.m_textureMemTotalBytes) {
        *freeBytes = device.m_videoMemFreeBytes - 0x1f4000;
    } else {
        *freeBytes = device.m_videoMemFreeBytes - device.m_textureMemTotalBytes;
    }

    return 1;
}

RECOIL_NOINLINE int RECOIL_FASTCALL QueryTextureMemoryBytes(
    int deviceIndexOrMinus1, int *totalBytes, int *freeBytes) {
    if (g_zVideo_pDirectDraw2 == 0 && deviceIndexOrMinus1 == -1) {
        *freeBytes = 0;
        *totalBytes = 0;
        return 0;
    }

    if (deviceIndexOrMinus1 == -1) {
        *totalBytes = 0;
        *freeBytes = 0;
        return 1;
    }

    const zVidHwApiDeviceRecordPartial &device = g_zVideo_HwApiDeviceTable[deviceIndexOrMinus1];
    *totalBytes = device.m_textureMemTotalBytes;
    *freeBytes = device.m_textureMemFreeBytes;
    return 1;
}

// Reimplements 0x4a59a0: zVid::SetCachedClientRectUpdateMask
RECOIL_NOINLINE void RECOIL_FASTCALL SetCachedClientRectUpdateMask(int mask) {
    g_zVid_CachedClientRectUpdateMask = mask;
}

// Reimplements 0x4a7410: zVid::GetSelectedHwApiDescriptionOrDefault
RECOIL_NOINLINE char *RECOIL_CDECL GetSelectedHwApiDescriptionOrDefault() {
    return g_zVideo_pSelectedHwApiDeviceRecord != 0
               ? g_zVideo_pSelectedHwApiDeviceRecord->m_driverDescription
               : const_cast<char *>("Default");
}

// Reimplements 0x4a9940: zVid::GetSelectedD3DDeviceNameOrDefault
RECOIL_NOINLINE char *RECOIL_CDECL GetSelectedD3DDeviceNameOrDefault() {
    return g_zVideo_pSelectedD3DDeviceInfo != 0
               ? g_zVideo_pSelectedD3DDeviceInfo->m_deviceName
               : const_cast<char *>("GameZ");
}

// Reimplements 0x4a7430: zVid::GetHwApiDescription
RECOIL_NOINLINE char *RECOIL_FASTCALL GetHwApiDescription(int index) {
    return g_zVideo_HwApiDeviceTable[index].m_driverDescription;
}

// Reimplements 0x4a7450: zVid::GetHwApiDriverName
RECOIL_NOINLINE char *RECOIL_FASTCALL GetHwApiDriverName(int index) {
    return g_zVideo_HwApiDeviceTable[index].m_driverName;
}
} // namespace zVid

// Reimplements 0x4bdc00: zVideoFxPass3Slot::SetRectAndPayload
// (D:\Proj\GameZRecoil\zVideo\zVideo.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL zVideoFxPass3Slot::SetRectAndPayload(
    int rectLeftPixels, int rectTopPixels, int currentRadiusPixels,
    int maxRadiusPixels, int extentPixels, float sinFreqValue,
    float sinPhaseValue) {
    if (base.ftable != 0) {
        base.SetPos(rectLeftPixels, rectTopPixels);
    } else {
        base.x = rectLeftPixels;
        base.y = rectTopPixels;
    }

    currentRadius = static_cast<float>(currentRadiusPixels);
    maxRadius = static_cast<float>(maxRadiusPixels);
    extent = static_cast<float>(extentPixels);
    sinFreq = sinFreqValue;
    sinPhase = sinPhaseValue;
}

// Reimplements 0x4bee00: zVideo_FxPass3Config::SetInputRectByIndex
// (D:\Proj\GameZRecoil\zVideo\zVideo.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL zVideoFxPass3Config::SetInputRectByIndex(
    int index, HudUiRect *rectOrNull) {
    if (index < 2) {
        inputRectsOrNull[index] = rectOrNull;
    }
}

namespace zVideo_buff {
// Reimplements 0x4a69c0: zVideo_buff::ClipCoordToRange
RECOIL_NOINLINE int RECOIL_FASTCALL ClipCoordToRange(int *coordPtr,
                                                              int minCoord,
                                                              int maxCoord) {
    const int coord = *coordPtr;
    int clipped = 0;
    if (coord < minCoord) {
        clipped = coord - minCoord;
        *coordPtr = minCoord;
    } else if (coord > maxCoord) {
        clipped = coord - maxCoord;
        *coordPtr = maxCoord;
    }

    return clipped;
}

// Reimplements 0x4a6fe0: zVideo_buff::CopySurfaceRectToImage
// (GameZRecoil/zImage/zvid_buff.c)
RECOIL_NOINLINE zVidImagePartial *RECOIL_FASTCALL
CopySurfaceRectToImage(int sourceSelector, zVidRect32 *rect,
                       zVidImagePartial *imageOrNull) {
    zVideo_SurfaceStatePartial *surfaceState = 0;
    if (sourceSelector == 0) {
        surfaceState = &g_zVideo_SwSurfaceState;
    } else if (sourceSelector == 1) {
        surfaceState = &g_zVideo_PrimarySurfaceState;
    } else if (sourceSelector == 2) {
        surfaceState = &g_zVideo_DisplayModeSurfaceState;
    } else {
        return 0;
    }

    const int surfaceWidth = surfaceState->width;
    const int surfaceHeight = surfaceState->height;
    const int pitchWords = surfaceState->pitch >> 1;
    unsigned char *const surfacePixels = static_cast<unsigned char *>(surfaceState->pixels);

    int dstOffsetX = 0;
    int dstOffsetY = 0;
    const int originalWidth = rect->right - rect->left;

    int clipped = ClipCoordToRange(&rect->left, 0, surfaceWidth);
    if (clipped < 0) {
        dstOffsetX = -clipped;
    } else if (clipped > 0) {
        return 0;
    }

    clipped = ClipCoordToRange(&rect->right, 0, surfaceWidth);
    if (clipped < 0) {
        return 0;
    }

    clipped = ClipCoordToRange(&rect->top, 0, surfaceHeight);
    if (clipped < 0) {
        dstOffsetY = -clipped;
    } else if (clipped > 0) {
        return 0;
    }

    clipped = ClipCoordToRange(&rect->bottom, 0, surfaceHeight);
    if (clipped < 0) {
        return 0;
    }

    const int clippedWidth = rect->right - rect->left;
    const int clippedHeight = rect->bottom - rect->top;
    if (clippedWidth <= 0 || clippedHeight <= 0) {
        return 0;
    }

    zVidImagePartial *image = imageOrNull;
    if (image == 0) {
        image = zVid_Image::Create();
        if (image == 0) {
            return 0;
        }

        zVid_Image::SetSize(image, static_cast<short>(clippedHeight),
                            static_cast<short>(clippedWidth));
        image->pixels =
            malloc(static_cast<size_t>(image->pixelCount) * sizeof(unsigned short));
    }

    unsigned char * dstBytes = static_cast<unsigned char *>(image->pixels) +
                     (originalWidth * dstOffsetY + dstOffsetX) * sizeof(unsigned short);
    unsigned char *srcBytes =
        surfacePixels + (pitchWords * rect->top + rect->left) * sizeof(unsigned short);
    const int rowBytes = clippedWidth * static_cast<int>(sizeof(unsigned short));
    const int dstStrideBytes =
        originalWidth * static_cast<int>(sizeof(unsigned short));
    const int srcStrideBytes =
        pitchWords * static_cast<int>(sizeof(unsigned short));

    {
    for (int row = clippedHeight; row > 0; --row) {
        memcpy(dstBytes, srcBytes, static_cast<size_t>(rowBytes));
        dstBytes += dstStrideBytes;
        srcBytes += srcStrideBytes;
    }
    }

    return image;
}

// Reimplements 0x4a69e0: zVideo_buff::BltSourceToPrimaryClipped
RECOIL_NOINLINE void RECOIL_FASTCALL BltSourceToPrimaryClipped(zVidImagePartial *srcImage,
                                                               int dstX, int dstY,
                                                               int srcColorKeyEnable,
                                                               zVidRect32 *srcRect) {
    zVidRect32 srcRectLocal;
    int srcX;
    int srcY;
    int srcRight;
    int srcBottom;
    if (srcRect != 0) {
        srcX = srcRect->left;
        srcY = srcRect->top;
        srcRight = srcRect->right;
        srcBottom = srcRect->bottom;
        srcRectLocal.left = srcX;
        srcRectLocal.top = srcY;
        srcRectLocal.right = srcRight;
    } else {
        srcRight = srcImage->width;
        srcBottom = srcImage->height;
        srcX = 0;
        srcY = 0;
        srcRectLocal.left = srcX;
        srcRectLocal.top = srcY;
        srcRectLocal.right = srcRight;
    }

    srcRectLocal.bottom = srcBottom;

    zVidRect32 dstRectLocal;
    dstRectLocal.left = dstX;
    dstRectLocal.top = dstY;
    dstRectLocal.right = srcRight - srcX + dstX;
    dstRectLocal.bottom = srcBottom - srcY + dstY;

    int clipped =
        ClipCoordToRange(&dstRectLocal.left, 0, g_zVideo_PrimarySurfaceState.width - 1);
    if (clipped < 0) {
        srcRectLocal.left -= clipped;
    } else if (clipped > 0) {
        return;
    }

    clipped = ClipCoordToRange(&dstRectLocal.right, 0, g_zVideo_PrimarySurfaceState.width);
    if (clipped < 0) {
        return;
    }
    if (clipped > 0) {
        srcRectLocal.right -= clipped;
    }

    clipped = ClipCoordToRange(&dstRectLocal.top, 0, g_zVideo_PrimarySurfaceState.height - 1);
    if (clipped < 0) {
        srcRectLocal.top -= clipped;
    } else if (clipped > 0) {
        return;
    }

    clipped = ClipCoordToRange(&dstRectLocal.bottom, 0, g_zVideo_PrimarySurfaceState.height);
    if (clipped < 0) {
        return;
    }
    if (clipped > 0) {
        srcRectLocal.bottom -= clipped;
    }

    IDirectDrawSurface3 *const primarySurface = g_zVideo_PrimarySurfaceState.surf;
    if (primarySurface == 0) {
        return;
    }

    const int wasLocked = g_zVideo_PrimarySurfaceState.locked;
    if (wasLocked != 0) {
        zVideo_dd::UnlockSurfaceState(&g_zVideo_PrimarySurfaceState);
    }

    const DWORD bltFlags = DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE |
                           ((srcImage->formatFlagsPacked & 0x02u) != 0 ? DDBLT_KEYSRC : 0);
    const HRESULT hresult =
        primarySurface->Blt((RECT *)&dstRectLocal, srcImage->surface,
                            (RECT *)&srcRectLocal, bltFlags, 0);

    if (wasLocked != 0) {
        zVideo_dd::LockSurfaceState(&g_zVideo_PrimarySurfaceState);
    }

    if (hresult != DD_OK) {
        zVideo_dd::ReportError(static_cast<int>(hresult),
                               "D:\\Proj\\GameZRecoil\\zVideo\\zvid_buff.c", 0x150);
    }
}
} // namespace zVideo_buff

namespace zVideo {
namespace {
int MakeShiftedMask(int bits) {
    return ((1 << bits) - 1) << (8 - bits);
}
} // namespace

// Reimplements 0x4a6bf0: zVideo::PixelPack_SetupFromMasks
RECOIL_NOINLINE void RECOIL_FASTCALL
PixelPack_SetupFromMasks(int redBits, int greenBits, int blueBits,
                         unsigned int redMask, unsigned int greenMask, unsigned int blueMask) {
    g_zVideo_PixelPack_RMask = redMask;
    g_zVideo_PixelPack_GMask = greenMask;
    g_zVideo_PixelPack_BMask = blueMask;
    const int greenBlueBits = greenBits + blueBits;
    g_zVideo_PixelPack_RBits = redBits;
    g_zVideo_PixelPack_GShift = greenBlueBits - 8;
    g_zVideo_PixelPack_RShift = redBits + greenBlueBits - 8;
    g_zVideo_PixelPack_GBits = greenBits;
    g_zVideo_PixelPack_BShiftTo8 = 8 - blueBits;
    g_zVideo_PixelPack_BBits = blueBits;
    g_zVideo_PixelPack_RMaskShifted = MakeShiftedMask(redBits);
    g_zVideo_PixelPack_GMaskShifted = MakeShiftedMask(greenBits);
    g_zVideo_PixelPack_BMaskShifted = MakeShiftedMask(blueBits);
}

// Reimplements 0x4a6db0: zVideo::TexturePixelPack_SetupFromMasks
RECOIL_NOINLINE void RECOIL_FASTCALL TexturePixelPack_SetupFromMasks(
    int redBits, int greenBits, int blueBits, int alphaBits,
    unsigned int redMask, unsigned int greenMask, unsigned int blueMask,
    unsigned int alphaMask) {
    g_zVideo_TexturePixelPack_ABits = alphaBits;
    g_zVideo_TexturePixelPack_RMask = redMask;
    g_zVideo_TexturePixelPack_AMask = alphaMask;
    g_zVideo_TexturePixelPack_BMask = blueMask;
    const int greenBlueBits = greenBits + blueBits;
    g_zVideo_TexturePixelPack_GMask = greenMask;
    g_zVideo_TexturePixelPack_RBits = redBits;
    const int rgbBitsTotal = redBits + greenBlueBits;
    g_zVideo_TexturePixelPack_RGBBitsTotal = rgbBitsTotal;
    g_zVideo_TexturePixelPack_RGBBitsTotalMinus8 = rgbBitsTotal - 8;
    g_zVideo_TexturePixelPack_GBBitsTotalMinus8 = greenBlueBits - 8;
    g_zVideo_TexturePixelPack_GBits = greenBits;
    g_zVideo_TexturePixelPack_BBits = blueBits;
    g_zVideo_TexturePixelPack_BShiftTo8 = 8 - blueBits;
    const int rMaskShifted = MakeShiftedMask(redBits);
    g_zVideo_TexturePixelPack_RMaskShifted = rMaskShifted;
    const int gMaskShifted = MakeShiftedMask(greenBits);
    g_zVideo_TexturePixelPack_GMaskShifted = gMaskShifted;
    const int bMaskShifted = MakeShiftedMask(blueBits);
    g_zVideo_TexturePixelPack_BMaskShifted = bMaskShifted;
    g_zVideo_TexturePixelPack_NonRgbMaskShifted = ~(rMaskShifted | gMaskShifted | bMaskShifted);
}

// Reimplements 0x4a6b40: zVideo::SetRendererTypeAndActivePath
RECOIL_NOINLINE int RECOIL_FASTCALL
SetRendererTypeAndActivePath(int rendererType) {
    const int previousRendererType = g_zVideo_RendererType;
    g_zVideo_RendererType = rendererType;
    g_zVideo_ActiveRendererPath = rendererType;
    return previousRendererType;
}

// Reimplements 0x4a71c0: zVideo::SetHalfResAdjustMode
RECOIL_NOINLINE int RECOIL_FASTCALL SetHalfResAdjustMode(int mode) {
    const int previousMode = g_zVideo_HalfResAdjustMode;
    if (mode == previousMode) {
        return mode;
    }

    if (g_zVideo_UseHalfResBackbuffer != 0) {
        return 0;
    }

    g_zVideo_HalfResAdjustMode = mode;
    if (mode == 0 && g_zVideo_RendererType == 0) {
        g_zVideo_pfnBltPrimaryToSwRectDirect(0, 0);
    }

    return previousMode;
}

// Reimplements 0x4a7200: zVideo::GetPrimarySurfaceRectScratch
RECOIL_NOINLINE zVidRect32 *RECOIL_CDECL GetPrimarySurfaceRectScratch() {
    g_zVideo_PrimarySurfaceRectScratch.right = g_zVideo_PrimarySurfaceState.width;
    g_zVideo_PrimarySurfaceRectScratch.bottom = g_zVideo_PrimarySurfaceState.height;
    return &g_zVideo_PrimarySurfaceRectScratch;
}

// Reimplements 0x4a6710: zVideo::GetSwSurfacePixels
RECOIL_NOINLINE void *RECOIL_CDECL GetSwSurfacePixels() {
    return g_zVideo_SwSurfaceState.pixels;
}

// Reimplements 0x4a6720: zVideo::GetSwSurfaceWidth
RECOIL_NOINLINE int RECOIL_CDECL GetSwSurfaceWidth() {
    return g_zVideo_SwSurfaceState.width;
}

// Reimplements 0x4a6730: zVideo::GetSwSurfaceHeight
RECOIL_NOINLINE int RECOIL_CDECL GetSwSurfaceHeight() {
    return g_zVideo_SwSurfaceState.height;
}

// Reimplements 0x4a6740: zVideo::GetSwSurfacePitch
RECOIL_NOINLINE int RECOIL_CDECL GetSwSurfacePitch() {
    return g_zVideo_SwSurfaceState.pitch;
}

// Reimplements 0x4a67e0: zVideo::GetSwSurfaceLockedFlag
RECOIL_NOINLINE int RECOIL_CDECL GetSwSurfaceLockedFlag() {
    return g_zVideo_SwSurfaceState.locked;
}

// Reimplements 0x4a67f0: zVideo::GetPrimarySurfacePixels
RECOIL_NOINLINE void *RECOIL_CDECL GetPrimarySurfacePixels() {
    return g_zVideo_PrimarySurfaceState.pixels;
}

// Reimplements 0x4a6800: zVideo::GetPrimarySurfaceWidth
RECOIL_NOINLINE int RECOIL_CDECL GetPrimarySurfaceWidth() {
    return g_zVideo_PrimarySurfaceState.width;
}

// Reimplements 0x4a6810: zVideo::GetPrimarySurfaceHeight
RECOIL_NOINLINE int RECOIL_CDECL GetPrimarySurfaceHeight() {
    return g_zVideo_PrimarySurfaceState.height;
}

// Reimplements 0x4a6820: zVideo::GetPrimarySurfacePitch
RECOIL_NOINLINE int RECOIL_CDECL GetPrimarySurfacePitch() {
    return g_zVideo_PrimarySurfaceState.pitch;
}

// Reimplements 0x4a66e0: zVideo::GetDisplayModeBpp
RECOIL_NOINLINE int RECOIL_CDECL GetDisplayModeBpp() {
    return g_zVideo_DisplayModeBpp;
}

// Reimplements 0x4a7990: zVideo::Init_SetSurfaceGeometryFromModeIndex
RECOIL_NOINLINE void RECOIL_FASTCALL Init_SetSurfaceGeometryFromModeIndex(int modeIndex) {
    int displayWidth = 0;
    int displayHeight = 0;
    int swWidth = 0;
    int swHeight = 0;
    int useHalfRes = 0;

    switch (modeIndex) {
    case 2:
        displayWidth = 0x280;
        displayHeight = 0x190;
        swWidth = 0x140;
        swHeight = 0x0c8;
        useHalfRes = 1;
        break;
    case 3:
        displayWidth = 0x280;
        displayHeight = 0x1e0;
        swWidth = 0x140;
        swHeight = 0x0f0;
        useHalfRes = 1;
        break;
    case 4:
        displayWidth = 0x280;
        displayHeight = 0x190;
        swWidth = 0x280;
        swHeight = 0x190;
        break;
    case 5:
        displayWidth = 0x280;
        displayHeight = 0x1e0;
        swWidth = 0x280;
        swHeight = 0x1e0;
        break;
    case 6:
        displayWidth = 0x320;
        displayHeight = 0x258;
        swWidth = 0x320;
        swHeight = 0x258;
        break;
    case 7:
        displayWidth = 0x400;
        displayHeight = 0x300;
        swWidth = 0x400;
        swHeight = 0x300;
        break;
    default:
        gVideo_resolutionMenuValid = 0;
        return;
    }

    g_zVideo_UseHalfResBackbuffer = useHalfRes;
    g_zVideo_DisplayModeSurfaceState.width = displayWidth;
    g_zVideo_DisplayModeSurfaceState.height = displayHeight;
    g_zVideo_PrimarySurfaceState.width = displayWidth;
    g_zVideo_PrimarySurfaceState.height = displayHeight;
    g_zVideo_SwSurfaceState.width = swWidth;
    g_zVideo_SwSurfaceState.height = swHeight;
    g_zVideo_DisplayModeBpp = 0x10;
}

// Reimplements 0x4a66f0: zVideo::Init_ApplyModeIndex
RECOIL_NOINLINE int RECOIL_FASTCALL Init_ApplyModeIndex(int modeIndex) {
    Init_SetSurfaceGeometryFromModeIndex(modeIndex);
    return g_zVideo_pfnSetVideoMode(modeIndex);
}

RECOIL_NOINLINE int RECOIL_FASTCALL SetVideoMode(int modeIndex) {
    if (g_zVideo_IsInitialized == 0) {
        return 0x5a560000;
    }

    Init_SetSurfaceGeometryFromModeIndex(modeIndex);
    return g_zVideo_pfnSetVideoMode(modeIndex);
}

// Reimplements 0x4a6760: zVideo::CallClearSwSurfaceAndZBuffer
RECOIL_NOINLINE void RECOIL_FASTCALL CallClearSwSurfaceAndZBuffer(zVidRect32 *surfaceRect,
                                                                  zVidRect32 *zRect) {
    g_zVideo_pfnClearSwSurfaceAndZBuffer(surfaceRect, zRect);
}

// Reimplements 0x4a6830: zVideo::CallClearPrimarySurfaceAndZBuffer
RECOIL_NOINLINE void RECOIL_FASTCALL CallClearPrimarySurfaceAndZBuffer(zVidRect32 *rect) {
    g_zVideo_pfnClearStateSurfaceAndZBuffer(rect, &g_zVideo_PrimarySurfaceState);
}

// Reimplements 0x4a7b20: zVideo::ExchangeClearScreenBufferEnabled
RECOIL_NOINLINE int RECOIL_FASTCALL ExchangeClearScreenBufferEnabled(int enable) {
    const int previous = g_zVideo_ClearScreenBufferEnabled;
    g_zVideo_ClearScreenBufferEnabled = enable;
    return previous;
}

// Reimplements 0x4a7b30: zVideo::GetClearScreenBufferEnabled
RECOIL_NOINLINE int RECOIL_CDECL GetClearScreenBufferEnabled() {
    return g_zVideo_ClearScreenBufferEnabled;
}

// Reimplements 0x4a68e0: zVideo::Dispatch_LockDisplayModeSurfaceState
RECOIL_NOINLINE int RECOIL_CDECL Dispatch_LockDisplayModeSurfaceState() {
    return g_zVideo_pfnLockSurfaceState(&g_zVideo_DisplayModeSurfaceState);
}

// Reimplements 0x4a68f0: zVideo::Dispatch_UnlockDisplayModeSurfaceState
RECOIL_NOINLINE int RECOIL_CDECL Dispatch_UnlockDisplayModeSurfaceState() {
    return g_zVideo_pfnUnlockSurfaceState(&g_zVideo_DisplayModeSurfaceState);
}

// Reimplements 0x4a67d0: zVideo::Dispatch_UnlockSwSurfaceState
// (D:\Proj\GameZRecoil\zVideo\zVideo.cpp)
RECOIL_NOINLINE int RECOIL_CDECL Dispatch_UnlockSwSurfaceState() {
    return g_zVideo_pfnUnlockSurfaceState(&g_zVideo_SwSurfaceState);
}

// Reimplements 0x4a68d0: zVideo::Dispatch_UnlockPrimarySurfaceState
RECOIL_NOINLINE int RECOIL_CDECL Dispatch_UnlockPrimarySurfaceState() {
    return g_zVideo_pfnUnlockSurfaceState(&g_zVideo_PrimarySurfaceState);
}

// Reimplements 0x48d420: zVideo::Fx_SetSurfaceState
RECOIL_NOINLINE void RECOIL_FASTCALL Fx_SetSurfaceState(void *pixels, int width,
                                                        int height,
                                                        int pitchBytes) {
    g_zVideo_FxSurfaceWidth = width;
    g_zVideo_FxSurfaceHeight = height;
    g_zVideo_FxSurfacePitchBytes = pitchBytes;
    g_zVideo_FxSurfacePixels16 = static_cast<unsigned short *>(pixels);
    g_zVideo_FxSurfacePitchPixels16 = pitchBytes / 2;
}

} // namespace zVideo

// Reimplements 0x4bee20: zVideo::FxPass3Config_QueuePrimitiveRaw
RECOIL_NOINLINE void RECOIL_THISCALL zVideoFxPass3Config::QueuePrimitiveRaw(
    void *primitive, int width, int height, int pitchBytes) {
    surfacePixels = static_cast<unsigned short *>(primitive);
    surfaceWidth = width;
    surfaceHeight = height;
    surfacePitchBytes = pitchBytes;
}

namespace zVideo {

// Reimplements 0x4bed30: zVideo::FxPass3Config_UpdateLocal
// (D:\Proj\GameZRecoil\zVideo\zVideo.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL FxPass3Config_UpdateLocal(zVideoFxPass3Config *config,
                                                               float deltaTime) {
    reinterpret_cast<HudUiContainer *>(config)->UpdateAll(deltaTime);
    config->slotWriteIndex = 0;
}

// Reimplements 0x4bed50: zVideo::FxPass3Config_SetPrimaryElementParamsLocal
RECOIL_NOINLINE void RECOIL_FASTCALL FxPass3Config_SetPrimaryElementParamsLocal(
    zVideoFxPass3Config *config, unsigned int packedColor, double primaryAlpha) {
    config->rootElement.packedColor16 = static_cast<unsigned short>(packedColor);
    config->rootElement.alpha = primaryAlpha;
    if (config->rootElement.base.ftable != 0) {
        config->rootElement.base.SetVisible(1);
    }
    config->rootElement.base.timer = 0.0f;
    config->rootElement.base.flags |= 0x01u;
}

// Reimplements 0x4beee0: zVideo::FxPass3_SetPrimaryElementParamsLocal
RECOIL_NOINLINE void RECOIL_FASTCALL FxPass3_SetPrimaryElementParamsLocal(unsigned int packedColor,
                                                                          double primaryAlpha) {
    FxPass3Config_SetPrimaryElementParamsLocal(&g_zVideo_FxPass3ConfigLocal, packedColor,
                                               primaryAlpha);
}

// Reimplements 0x4bed90: zVideo::FxPass3Config_QueueElementLocal
RECOIL_NOINLINE void RECOIL_FASTCALL FxPass3Config_QueueElementLocal(
    zVideoFxPass3Config *config, int rectLeftPixels, int rectTopPixels,
    int currentRadiusPixels, int maxRadiusPixels, int extentPixels,
    float sinFreq, float sinPhase) {
    const int slotIndex = config->slotWriteIndex;
    zVideoFxPass3Slot *const slot = &config->slots[slotIndex];
    if (slotIndex < 4) {
        config->slotWriteIndex = slotIndex + 1;
    }

    slot->SetRectAndPayload(rectLeftPixels, rectTopPixels, currentRadiusPixels, maxRadiusPixels,
                            extentPixels, sinFreq, sinPhase);
    if (slot->base.ftable != 0) {
        slot->base.SetVisible(1);
    }
    slot->base.timer = 0.0f;
    slot->base.flags |= 0x01u;
}

// Reimplements 0x4bef10: zVideo::FxPass3_QueueElementLocal
RECOIL_NOINLINE void RECOIL_FASTCALL FxPass3_QueueElementLocal(
    int rectLeftPixels, int rectTopPixels, int currentRadiusPixels,
    int maxRadiusPixels, int extentPixels, float sinFreq, float sinPhase) {
    FxPass3Config_QueueElementLocal(&g_zVideo_FxPass3ConfigLocal, rectLeftPixels, rectTopPixels,
                                    currentRadiusPixels, maxRadiusPixels, extentPixels, sinFreq,
                                    sinPhase);
}

// Reimplements 0x4bef50: zVideo::FxPass3_QueuePrimitive
RECOIL_NOINLINE void RECOIL_FASTCALL FxPass3_QueuePrimitive(void *primitive, int width,
                                                            int height,
                                                            int pitchBytes) {
    g_zVideo_FxPass3ConfigLocal.QueuePrimitiveRaw(primitive, width, height, pitchBytes);
}

// Reimplements 0x4bef40: zVideo::FxPass3_SetInputRectByIndex
// (D:\Proj\GameZRecoil\zVideo\zVideo.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL FxPass3_SetInputRectByIndex(int index,
                                                                 HudUiRect *rectOrNull) {
    g_zVideo_FxPass3ConfigLocal.SetInputRectByIndex(index, rectOrNull);
}

// Reimplements 0x4bef70: zVideo::FxPass3_UpdateLocal
// (D:\Proj\GameZRecoil\zVideo\zVideo.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL FxPass3_UpdateLocal(float deltaTime) {
    FxPass3Config_UpdateLocal(&g_zVideo_FxPass3ConfigLocal, deltaTime);
}

// Reimplements 0x4a6770: zVideo::RunPostprocessOnSwBuffer
// (D:\Proj\GameZRecoil\zVideo\zVideo.cpp)
RECOIL_NOINLINE void RECOIL_CDECL RunPostprocessOnSwBuffer() {
    g_zVideo_pfnLockSurfaceState(&g_zVideo_SwSurfaceState);
    zRndr::SetFrameBufferRegion(g_zVideo_SwSurfaceState.pixels, 0, 0,
                                g_zVideo_SwSurfaceState.pitch);
    Fx_SetSurfaceState(g_zVideo_SwSurfaceState.pixels, g_zVideo_SwSurfaceState.width,
                       g_zVideo_SwSurfaceState.height, g_zVideo_SwSurfaceState.pitch);
    FxPass3_QueuePrimitive(g_zVideo_SwSurfaceState.pixels, g_zVideo_SwSurfaceState.width,
                           g_zVideo_SwSurfaceState.height, g_zVideo_SwSurfaceState.pitch);
}

// Reimplements 0x4a6840: zVideo::RunPostprocessOnPrimaryBuffer
RECOIL_NOINLINE int RECOIL_CDECL RunPostprocessOnPrimaryBuffer() {
    if (g_zVideo_RendererType != 0 || g_zVideo_UseHalfResBackbuffer != 0) {
        g_zVideo_pfnLockSurfaceState(&g_zVideo_PrimarySurfaceState);
    }

    zRndr::SetFrameBufferRegion(g_zVideo_PrimarySurfaceState.pixels, 0, 0,
                                g_zVideo_PrimarySurfaceState.pitch);
    Fx_SetSurfaceState(g_zVideo_PrimarySurfaceState.pixels, g_zVideo_PrimarySurfaceState.width,
                       g_zVideo_PrimarySurfaceState.height, g_zVideo_PrimarySurfaceState.pitch);
    FxPass3_QueuePrimitive(g_zVideo_PrimarySurfaceState.pixels, g_zVideo_PrimarySurfaceState.width,
                           g_zVideo_PrimarySurfaceState.height, g_zVideo_PrimarySurfaceState.pitch);

    if (g_zVideo_UseHalfResBackbuffer != 0) {
        g_zVideo_pfnUnlockSurfaceState(&g_zVideo_PrimarySurfaceState);
    }

    return 0;
}

// Reimplements 0x4a6900: zVideo::AdjustSurfacesIfEnabled
RECOIL_NOINLINE int RECOIL_FASTCALL
AdjustSurfacesIfEnabled(zVidRect32 *srcRect, zVidRect32 *dstRect, int waitForPresent,
                        int blitPrimaryToSwFirst) {
    int result = g_zVideo_AdjustSurfacesDisableGate;
    if (result <= 0) {
        result = g_zVideo_pfnAdjustSurfaces(srcRect, dstRect, waitForPresent,
                                            blitPrimaryToSwFirst);
        ++g_zVideo_FrameTick;
    }

    return result;
}

// Reimplements 0x4a77a0: zVideo::BindRendererDispatch
RECOIL_NOINLINE void RECOIL_FASTCALL BindRendererDispatch(int rendererType,
                                                          int fullscreenOption) {
    SetRendererTypeAndActivePath(rendererType);
    g_zVideo_FullscreenOption = fullscreenOption;
    g_zVideo_pfnOpenVideoMode = zVideo_dd::OpenVideoMode;
    g_zVideo_pfnShutdownVideoSystem = reinterpret_cast<zVideo_ShutdownVideoSystemProc>(0x004a7d40);
    g_zVideo_pfnPaletteSetEntries = 0x004a9890;
    g_zVideo_pfnSetVideoMode = zVideo_dd::SetVideoMode;
    g_zVideo_pfnAdjustSurfaces = zVideo_dd3d::PresentDisplayModeSurface;
    g_zVideo_pfnLockSurfaceState = zVideo_dd::LockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = zVideo_dd::UnlockSurfaceState;
    g_zVideo_pfnClearZBufferRect = zVideo_dd::ZBuffer_DepthFillRect;
    g_zVideo_pfnClearSwSurfaceAndZBuffer =
        reinterpret_cast<zVideo_ClearSwSurfaceAndZBufferProc>(0x004a82f0);
    g_zVideo_pfnClearStateSurfaceAndZBuffer =
        reinterpret_cast<zVideo_ClearStateSurfaceAndZBufferProc>(0x004a8220);
    g_zVideo_pfnUpdateFogColor = 0x004aab30;
    g_zVideo_pfnQueryTextureMemoryBytes = zVid::QueryTextureMemoryBytes;
    g_zVideo_pfnQueryDeviceVideoMemoryBytes = zVid::QueryDeviceVideoMemoryBytes;
    g_zVideo_pfnBltSwToPrimaryRectDirect = reinterpret_cast<zVideo_BltRectDirectProc>(0x004a7d90);
    g_zVideo_pfnBltPrimaryToSwRectDirect = zVideo_dd::BltPrimaryToSwRectDirect;
    g_zVideo_pfnBltSwToPrimaryRect = 0x004a7e10;
    g_zVideo_pfnGetHwApiDeviceFeatureFlags = 0x004a9920;
    g_zVideo_pfnImageUploadPixelsToSurface = 0x004a8680;
    g_zVideo_pfnImageReleaseSurface = 0x004a86f0;
    g_zVideo_pfnCreateTextureRecord = zVideo_dd3d::CreateTextureRecord;
    g_zVideo_pfnTextureRecordLockUploadSurface = 0x004aa8b0;
    g_zVideo_pfnTextureRecordUnlockUploadSurface = 0x004aa8f0;
    g_zVideo_pfnTextureRecordReleaseUploadSurfaceRef = 0x004aa900;
    g_zVideo_pfnTextureRecordFinalizeUpload = 0x004aa920;
    g_zVideo_pfnTextureRecordDestroy = 0x004aa980;
    g_zVideo_pfnTextureRecordReleaseAllUploadSurfaces = 0x004076f0;
    g_zVideo_pfnImageLazyCreateVideoMemorySurface = 0x004a84c0;
    g_zVideo_pfnImageEnsureSurfaceForCurrentDevice = 0x004a8650;
    g_zVideo_pfnSetFogEnable = 0x004aa9e0;
    g_zVideo_pfnSetFogStart = 0x004aaa30;
    g_zVideo_pfnSetFogEnd = 0x004aaa60;
    g_zVideo_pfnApplyFogStateFromGlobals = 0x004aaa90;
    g_zVideo_pfnSubmitPolyFlatColor16 = 0x004aab90;
    g_zVideo_pfnSubmitPolyGouraudColor16 = 0x004aaef0;
    g_zVideo_pfnSubmitPolyColorAttr = 0x004ab320;
    g_zVideo_pfnSubmitPolyRenderClass = 0x004ab6d0;
    g_zVideo_pfnSubmitPolygon = 0x004abb20;
    g_zVideo_pfnSubmitPolygonLit = 0x004ac370;
    g_zVideo_pfnDrawPointColor16 = 0x004acbd0;
    g_zVideo_pfnFlushSortedPolys = 0x004ace30;
    g_zVideo_pfnFlushOverwritePolys = 0x004ad250;
    g_zVideo_pfnFlushQuadBatch = 0x004ad120;

    if (g_zVideo_pSelectedHwApiDeviceRecord != 0 &&
        g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags != 0) {
        g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags = 0;
    }
}

// Reimplements 0x4a8870: zVideo::CommitHwApiDeviceSelection
RECOIL_NOINLINE void RECOIL_FASTCALL CommitHwApiDeviceSelection(int hwApiIndex) {
    BindRendererDispatch(1, 1);
    zVidHwApiDeviceRecordPartial &selected = g_zVideo_HwApiDeviceTable[hwApiIndex];
    g_zVideo_pSelectedHwApiDeviceRecord = &selected;
    g_zVideo_pSelectedD3DDeviceInfo = selected.m_d3dDrivers;
}

// Reimplements 0x4a7490: zVideo::SelectHwApiDeviceOrFallback
RECOIL_NOINLINE int RECOIL_FASTCALL SelectHwApiDeviceOrFallback(int hwApiIndex) {
    if (hwApiIndex != -1) {
        CommitHwApiDeviceSelection(hwApiIndex);
        return 1;
    }

    BindRendererDispatch(0, 1);
    g_zVideo_pSelectedHwApiDeviceRecord = &g_zVideo_HwApiDeviceTable[0];
    g_zVideo_pSelectedD3DDeviceInfo = 0;
    return 0;
}

// Reimplements 0x4a75e0: zVideo::ReturnSuccessStub
RECOIL_NOINLINE int RECOIL_CDECL ReturnSuccessStub() {
    return 0;
}

// Reimplements 0x4a7530: zVideo::ModuleInit
RECOIL_NOINLINE int RECOIL_CDECL ModuleInit() {
    g_zVideo_RendererType = 0;
    g_zVideo_ActiveRendererPath = 0;
    g_zVideo_FrameTick = 0;
    gVideo_resolutionMenuValid = 0;
    g_zVideo_PaletteBrightnessLevel = 4;
    g_zVideo_ClearColorPacked16 = 0;
    g_zVideo_FullscreenOption = 1;
    g_zVideo_CachedFogModeLightState = 0;
    g_zVideo_PendingDitherEnable = -1;
    g_zVideo_InverseZTolerancePending = 0.0199999996f;
    g_zVideo_D3DAppendFanCloseVertexPending = 0;

    PixelPack_SetupFromMasks(0, 0, 0, 0, 0, 0);
    TexturePixelPack_SetupFromMasks(4, 4, 4, 4, 0xf000, 0x0f00, 0x00f0, 0x000f);
    BindRendererDispatch(0, 1);
    zVideo_dd::StartupEnumerateAndDefaultSelect();
    atexit(AtExitReleaseAllInterfacesAndSurfaces);
    return 0;
}

// Reimplements 0x4a75f0: zVideo::InitVideoSystem
RECOIL_NOINLINE int RECOIL_FASTCALL InitVideoSystem(HWND hWnd,
                                                             int rendererBackend,
                                                             int fullscreen,
                                                             int modeIndex) {
    if (g_zVideo_IsInitialized != 0) {
        return 0x5a560001;
    }

    g_zVideo_hWnd = hWnd;
    g_zVideo_FrameTick = 0;
    BindRendererDispatch(rendererBackend, fullscreen);

    const int openResult = g_zVideo_pfnOpenVideoMode(modeIndex);
    if (openResult != 0) {
        zError::ReportOld(0x800, "D:\\Proj\\GameZRecoil\\zVideo\\zvid_init.c", 0x7a,
                          "Failed to open video mode");
        return openResult;
    }

    ShowCursor(FALSE);
    g_zVideo_IsInitialized = 1;
    const int setModeResult = SetVideoMode(modeIndex);
    if (setModeResult != 0) {
        zError::ReportOld(0x800, "D:\\Proj\\GameZRecoil\\zVideo\\zvid_init.c", 0x86,
                          "Failed to set video mode");
        ShutdownVideoSystem();
        return setModeResult;
    }

    if (g_zVideo_RendererType != 0) {
        g_zImage_DefaultTextureRecord =
            g_zVideo_pfnCreateTextureRecord(0, &zVid_Image::g_zImage_DefaultImage, 0, 0, 0);
        g_zVideo_QuadBatchCount = 0;
        {
        for (int itemIndex = 0; itemIndex < 16; ++itemIndex) {
            zVideo_QuadBatchItemPartial &item = g_zVideo_QuadBatchItemsBase[itemIndex];
            {
            for (int vertexIndex = 0; vertexIndex < 4; ++vertexIndex) {
                item.vertices[vertexIndex].specular = 0xff000000;
            }
            }
        }
        }
    }

    UpdateCachedClientRectScreenCoords();
    return 0;
}

RECOIL_NOINLINE int RECOIL_CDECL ShutdownVideoSystem() {
    if (g_zVideo_IsInitialized == 0) {
        return 0x5a560000;
    }

    g_zVideo_IsInitialized = 0;
    g_zVideo_pfnShutdownVideoSystem();
    ShowCursor(TRUE);
    return 0;
}

// Reimplements 0x4a7700: zVideo::UpdateCachedClientRectScreenCoords
RECOIL_NOINLINE int RECOIL_CDECL UpdateCachedClientRectScreenCoords() {
    GetClientRect(g_zVideo_hWnd, &g_zVideo_CachedClientRectScreen);
    ClientToScreen(g_zVideo_hWnd, reinterpret_cast<POINT *>(&g_zVideo_CachedClientRectScreen.left));
    ClientToScreen(g_zVideo_hWnd,
                   reinterpret_cast<POINT *>(&g_zVideo_CachedClientRectScreen.right));
    return 0;
}

// Reimplements 0x4a7520: zVideo::AtExitReleaseAllInterfacesAndSurfaces
RECOIL_NOINLINE void RECOIL_CDECL AtExitReleaseAllInterfacesAndSurfaces() {
    zVideo_dd::ReleaseAllInterfacesAndSurfaces();
}
} // namespace zVideo

namespace zVideo {
// Reimplements 0x4a7220: zVideo::SetFogColorFromRgb01
RECOIL_NOINLINE void RECOIL_FASTCALL SetFogColorFromRgb01(zVideo_ColorRgbFloat *color) {
    g_zVideo_FogColorPendingR255 = color->r * 255.0f;
    g_zVideo_FogColorPendingG255 = color->g * 255.0f;
    g_zVideo_FogColorPendingB255 = color->b * 255.0f;
}

// Reimplements 0x4a7300: zVideo::SetFogTargetColorFromRgb01
RECOIL_NOINLINE void RECOIL_FASTCALL SetFogTargetColorFromRgb01(zVideo_ColorRgbFloat *color) {
    g_zVideo_FogTargetColorR255 = color->r * 255.0f;
    g_zVideo_FogTargetColorG255 = color->g * 255.0f;
    g_zVideo_FogTargetColorB255 = color->b * 255.0f;
}

// Reimplements 0x4a6b90: zVideo::PixelPack_GetRgbBits
RECOIL_NOINLINE void RECOIL_FASTCALL PixelPack_GetRgbBits(int *outRBits,
                                                          int *outGBits,
                                                          int *outBBits) {
    *outRBits = g_zVideo_PixelPack_RBits;
    *outGBits = g_zVideo_PixelPack_GBits;
    *outBBits = g_zVideo_PixelPack_BBits;
}

// Reimplements 0x4a6bb0: zVideo::PixelPack_GetRgbMasks
RECOIL_NOINLINE void RECOIL_FASTCALL PixelPack_GetRgbMasks(unsigned int *outRMask,
                                                           unsigned int *outGMask,
                                                           unsigned int *outBMask) {
    *outRMask = g_zVideo_PixelPack_RMask;
    *outGMask = g_zVideo_PixelPack_GMask;
    *outBMask = g_zVideo_PixelPack_BMask;
}

// Reimplements 0x4a6bd0: zVideo::PixelPack_GetPackingParams
RECOIL_NOINLINE void RECOIL_FASTCALL PixelPack_GetPackingParams(int *outPackedBase,
                                                                int *outSumMinus8,
                                                                int *outBShiftTo8) {
    *outPackedBase = g_zVideo_PixelPack_RShift;
    *outSumMinus8 = g_zVideo_PixelPack_GShift;
    *outBShiftTo8 = g_zVideo_PixelPack_BShiftTo8;
}
} // namespace zVideo

namespace zVid {
// Reimplements 0x48d340: zVid::Noise_InitBuffers
RECOIL_NOINLINE void RECOIL_CDECL Noise_InitBuffers() {
    const int width = zVideo::GetPrimarySurfaceWidth();
    const int height = zVideo::GetPrimarySurfaceHeight();

    g_zVid_NoiseByteTableSize = width * 0x19;
    g_zVid_NoiseByteTable = static_cast<unsigned char *>(
        malloc(static_cast<size_t>(g_zVid_NoiseByteTableSize)));
    for (int i = 0; i < g_zVid_NoiseByteTableSize; ++i) {
        g_zVid_NoiseByteTable[i] = static_cast<unsigned char>(rand());
    }

    g_zVideo_FxPass3_ScratchPixels16 = static_cast<unsigned short *>(
        malloc(static_cast<size_t>(height * width) * sizeof(unsigned short)));
    g_zVideo_FxSurfacePixels16 = 0;
    g_zVideo_FxSurfaceWidth = 0;
    g_zVideo_FxSurfaceHeight = 0;
    g_zVideo_FxSurfacePitchBytes = 0;
    g_zVideo_FxSurfacePitchPixels16 = 0;
    zRndr::g_pfnOverlayBlendRow = zRndr::OverlayBlendRow555_Scalar;
}

// Reimplements 0x48d3e0: zVid::Noise_ShutdownBuffers
RECOIL_NOINLINE void RECOIL_CDECL Noise_ShutdownBuffers() {
    if (g_zVid_NoiseByteTable != 0) {
        free(g_zVid_NoiseByteTable);
        g_zVid_NoiseByteTable = 0;
    }

    if (g_zVideo_FxPass3_ScratchPixels16 != 0) {
        free(g_zVideo_FxPass3_ScratchPixels16);
        g_zVideo_FxPass3_ScratchPixels16 = 0;
    }
}

// Reimplements 0x48d910: zVid::DrawNoiseRect (GameZRecoil/zImage/zvid_buff.c)
RECOIL_NOINLINE void RECOIL_FASTCALL DrawNoiseRect(zVidRect32 *rectOrNull, double intensity) {
    if (intensity < 0.00390625) {
        return;
    }

    const int threshold = static_cast<int>(intensity * 256.0);
    int xMin = 0;
    int yMin = 0;
    int xMax = g_zVideo_FxSurfaceWidth - 1;
    int yMax = g_zVideo_FxSurfaceHeight - 1;
    if (rectOrNull != 0) {
        xMin = rectOrNull->left;
        yMin = rectOrNull->top;
        xMax = rectOrNull->right;
        yMax = rectOrNull->bottom;
    }

    const int rowWidth = xMax - xMin;
    int rBits = 0;
    int gBits = 0;
    int bBits = 0;
    zVideo::PixelPack_GetRgbBits(&rBits, &gBits, &bBits);

    int gShift = bBits;
    const int rShift = bBits + gBits;
    if (gBits == 6) {
        ++gShift;
    }

    for (int y = yMin; y < yMax; ++y) {
        const int noiseRange = g_zVid_NoiseByteTableSize - rowWidth;
        unsigned char *noiseBytes =
            g_zVid_NoiseByteTable + (rand() * noiseRange) / 0x7fff;
        unsigned short *dstPixels =
            g_zVideo_FxSurfacePixels16 + y * g_zVideo_FxSurfacePitchPixels16 + xMin;

        for (int x = 0; x < rowWidth; ++x) {
            const unsigned char noiseValue = noiseBytes[x];
            if (noiseValue < threshold) {
                const unsigned short level = static_cast<unsigned short>(noiseValue & 0x1f);
                dstPixels[x] = static_cast<unsigned short>(
                    (level << rShift) | (level << gShift) | level);
            }
        }
    }
}

// Reimplements 0x48ff70: zVid::InitFrameScratchBuffers
RECOIL_NOINLINE void RECOIL_CDECL InitFrameScratchBuffers() {
    Noise_InitBuffers();
    zRndr::SelectSpanRoutines();
}

// Reimplements 0x48ff60: zVid::ShutdownFrameScratchBuffers
RECOIL_NOINLINE int RECOIL_CDECL ShutdownFrameScratchBuffers() {
    Noise_ShutdownBuffers();
    return 0;
}
} // namespace zVid

namespace zVid_Image {
zVidImagePartial g_zImage_DefaultImage = {0};

// Reimplements 0x46ec00: zVid_Image::Create
RECOIL_NOINLINE zVidImagePartial *RECOIL_CDECL Create() {
    zVidImagePartial *image =
        static_cast<zVidImagePartial *>(malloc(sizeof(zVidImagePartial)));
    if (image != 0) {
        memset(image, 0, sizeof(zVidImagePartial));
    }
    return image;
}

// Reimplements 0x46ecc0: zVid_Image::Destroy
RECOIL_NOINLINE int RECOIL_FASTCALL Destroy(zVidImagePartial *image) {
    if (image != 0) {
        if (image->surface != 0 && g_zVideo_pfnImageEnsureSurfaceForCurrentDevice != 0) {
            reinterpret_cast<zVideo_ImageProc>(g_zVideo_pfnImageEnsureSurfaceForCurrentDevice)(
                image);
        }

        ReleaseOwnedBuffers(image);
        free(image);
    }

    return 0;
}

// Reimplements 0x46d5a0: zVid_Image::ReleaseIfNotDefault
RECOIL_NOINLINE int RECOIL_FASTCALL ReleaseIfNotDefault(zVidImagePartial *image) {
    if (image != &g_zImage_DefaultImage) {
        Destroy(image);
    }

    return 0;
}

// Reimplements 0x48f500: zVid_Image::BlitToActiveTarget
RECOIL_NOINLINE void RECOIL_FASTCALL BlitToActiveTarget(zVidImagePartial *image, int dstX,
                                                        int dstY, int clipFlags,
                                                        zVidRect32 *srcRect) {
    if (image->surface != 0 && zRndr::g_frameBuffer == zVideo::GetPrimarySurfacePixels()) {
        zVideo_buff::BltSourceToPrimaryClipped(image, dstX, dstY, clipFlags & 0xffff, srcRect);
        return;
    }

    g_zVideo_pfnBltSourceToPrimary(image, dstX, dstY, clipFlags, srcRect);
}

// Reimplements 0x46ecf0: zVid_Image::ReleaseOwnedBuffers
RECOIL_NOINLINE void RECOIL_FASTCALL ReleaseOwnedBuffers(zVidImagePartial *image) {
    if (image->pixels != 0 && (image->formatFlagsPacked & 0x20) != 0) {
        free(image->pixels);
        image->pixels = 0;
        image->formatFlagsPacked &= static_cast<unsigned char>(~0x20);
    }

    if (image->alphaMap != 0 && (image->formatFlagsPacked & 0x40) != 0) {
        free(image->alphaMap);
        image->alphaMap = 0;
        image->formatFlagsPacked &= static_cast<unsigned char>(~0x40);
    }

    if (image->palette != 0 && (image->formatFlagsPacked & 0x80) != 0 &&
        (image->formatFlagsPacked & 0x10) == 0) {
        free(image->palette);
        image->palette = 0;
        image->formatFlagsPacked &= static_cast<unsigned char>(~0x80);
    }
}

// Reimplements 0x46ec20: zVid_Image::QueryBytesPerPixel
RECOIL_NOINLINE int RECOIL_FASTCALL QueryBytesPerPixel(zVidImagePartial *image) {
    return (image->formatFlagsPacked & 1) != 0 ? 2 : 1;
}

// Reimplements 0x46ec30: zVid_Image::SetHeaderFlagsByte
RECOIL_NOINLINE int RECOIL_FASTCALL SetHeaderFlagsByte(zVidImagePartial *image,
                                                                unsigned char flags) {
    image->headerFlagsByte = flags;
    return 0;
}

// Reimplements 0x46ec60: zVid_Image::SetFormatCode
RECOIL_NOINLINE int RECOIL_FASTCALL SetFormatCode(zVidImagePartial *image,
                                                           unsigned char formatCode) {
    image->formatFlagsPacked = formatCode;
    return 0;
}

// Reimplements 0x46ec90: zVid_Image::SetSize
RECOIL_NOINLINE int RECOIL_FASTCALL SetSize(zVidImagePartial *image, short width,
                                                     short height) {
    image->width = width;
    image->height = height;
    image->pixelCount = static_cast<int>(width) * static_cast<int>(height);
    image->pitchWords = width;
    image->widthScale = width != 0 ? static_cast<float>(width) : 1.0f;
    image->uShiftFrom20 = 20;
    image->uMask = width > 0 ? width - 1 : 0;
    image->vMaskFixed20 = height > 0 ? (height - 1) << 20 : 0;
    return 0;
}

// Reimplements 0x4902b0: zVid_Image::CalcPow2ScratchFields
// (GameZRecoil/zImage/zimg_texture.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL CalcPow2ScratchFields(zVidImagePartial *image) {
    image->vPow2Shift = 0;
    image->uPow2Shift = 0;

    int width = image->width;
    while (width > 1) {
        width >>= 1;
        ++image->uPow2Shift;
    }

    int height = image->height;
    while (height > 1) {
        height >>= 1;
        ++image->vPow2Shift;
    }

    const unsigned char uShift = image->uPow2Shift;
    image->widthScale = 1.0f;
    image->uShiftFrom20 = 20 - uShift;
    image->uMask = (1 << uShift) - 1;
    image->vMaskFixed20 = (1 << image->vPow2Shift << 20) - 0x100000;
}

// Reimplements 0x46ec70: zVid_Image_SetPixels
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zVid_Image_SetPixels(zVidImagePartial *image, void *pixels, char *alphaMap) {
    image->pixels = pixels;
    image->alphaMap = alphaMap;
    if (alphaMap != 0) {
        image->formatFlagsPacked |= 0x02u;
    }

    return 0;
}

// Reimplements 0x4a6e80: zVideo_buff_CaptureSurfaceToImage
extern "C" RECOIL_NOINLINE zVidImagePartial *RECOIL_FASTCALL
zVideo_buff_CaptureSurfaceToImage(int sourceSelector) {
    zVideo::Dispatch_LockDisplayModeSurfaceState();

    zVideo_SurfaceStatePartial *surfaceState = 0;
    if (sourceSelector == 0) {
        surfaceState = &g_zVideo_SwSurfaceState;
    } else if (sourceSelector == 1) {
        surfaceState = &g_zVideo_PrimarySurfaceState;
    } else if (sourceSelector == 2) {
        surfaceState = &g_zVideo_DisplayModeSurfaceState;
    } else {
        return 0;
    }

    const int width = surfaceState->width;
    const int height = surfaceState->height;
    const unsigned int pitchWords = static_cast<unsigned int>(surfaceState->pitch) >> 1;
    unsigned char * srcPixels = static_cast<unsigned char *>(surfaceState->pixels);

    zVidImagePartial *image = zVid_Image::Create();
    if (image == 0) {
        return 0;
    }

    zVid_Image::SetSize(image, static_cast<short>(width), static_cast<short>(height));
    void *dstPixels =
        malloc(static_cast<size_t>(image->pixelCount) * sizeof(unsigned short));
    image->formatFlagsPacked |= 0x20u;
    zVid_Image_SetPixels(image, dstPixels, 0);

    unsigned char * dstBytes = static_cast<unsigned char *>(dstPixels);
    if (width == static_cast<int>(pitchWords)) {
        memcpy(dstBytes, srcPixels,
                    static_cast<size_t>(image->pixelCount) * sizeof(unsigned short));
    } else if (height > 0) {
        const int rowBytes = width * sizeof(unsigned short);
        const int pitchBytes =
            static_cast<int>(pitchWords * sizeof(unsigned short));
        {
        for (int row = 0; row < height; ++row) {
            memcpy(dstBytes, srcPixels, static_cast<size_t>(rowBytes));
            dstBytes += rowBytes;
            srcPixels += pitchBytes;
        }
        }
    }

    zVideo::Dispatch_UnlockDisplayModeSurfaceState();
    return image;
}

// Reimplements 0x46ec40: zVid_Image::QueryPixelDataBytes
RECOIL_NOINLINE int RECOIL_FASTCALL QueryPixelDataBytes(zVidImagePartial *image) {
    if (image->paletteMetaPacked != 0) {
        return image->pixelCount;
    }

    return QueryBytesPerPixel(image) * image->pixelCount;
}

RECOIL_NOINLINE void RECOIL_FASTCALL ClearZeroAlphaPixelsInPlace(zVidImagePartial *image) {
    if (image->paletteMetaPacked != 0) {
        return;
    }

    const int bytesPerPixel = QueryBytesPerPixel(image);
    if (image->pixelCount <= 0) {
        return;
    }

    unsigned char *alpha = reinterpret_cast<unsigned char *>(image->alphaMap);
    if (bytesPerPixel == 1) {
        unsigned char *pixels = static_cast<unsigned char *>(image->pixels);
        for (int i = 0; i < image->pixelCount; ++i) {
            if (alpha[i] == 0) {
                pixels[i] = 0;
            }
        }
        return;
    }

    if (bytesPerPixel == 2) {
        unsigned short *pixels = static_cast<unsigned short *>(image->pixels);
        for (int i = 0; i < image->pixelCount; ++i) {
            if (alpha[i] == 0) {
                pixels[i] = 0;
            }
        }
        return;
    }

    if (bytesPerPixel == 4) {
        unsigned int *pixels = static_cast<unsigned int *>(image->pixels);
        for (int i = 0; i < image->pixelCount; ++i) {
            if (alpha[i] == 0) {
                pixels[i] = 0;
            }
        }
    }
}

namespace {
struct zVidImageFileHeader {
    unsigned char formatCode;
    unsigned char unknown_01[3];
    short width;
    short height;
    unsigned char headerFlags;
    unsigned char unknown_09[3];
    short textureAddressFlagsPacked;
    short paletteMeta;
};

RECOIL_STATIC_ASSERT(sizeof(zVidImageFileHeader) == 0x10);
} // namespace

// Reimplements 0x46ed70: zVid_Image::ReadHeader
RECOIL_NOINLINE int RECOIL_FASTCALL ReadHeader(FILE *file, zVidImagePartial *image) {
    if (file == 0 || image == 0) {
        return -1;
    }

    zVidImageFileHeader header = {0};
    fread(&header, 0x10, 1, file);
    SetSize(image, header.width, header.height);
    SetFormatCode(image, header.formatCode);
    SetHeaderFlagsByte(image, header.headerFlags);
    image->paletteMetaPacked = header.paletteMeta;
    image->textureAddressFlagsPacked = header.textureAddressFlagsPacked;
    return 0;
}

// Reimplements 0x46ede0: zVid_Image::ReadData
RECOIL_NOINLINE int RECOIL_FASTCALL ReadData(FILE *file, zVidImagePartial *image,
                                                      int bytesPerPixel) {
    if (bytesPerPixel == 0) {
        bytesPerPixel = QueryBytesPerPixel(image);
    }

    if (bytesPerPixel != QueryBytesPerPixel(image) && bytesPerPixel > QueryBytesPerPixel(image)) {
        return -1;
    }

    const int pixelBytes = QueryPixelDataBytes(image);
    if (fread(image->pixels, 1, pixelBytes, file) != static_cast<size_t>(pixelBytes)) {
        return -1;
    }

    if ((image->formatFlagsPacked & 0x08) != 0) {
        image->alphaMap =
            static_cast<char *>(malloc(static_cast<size_t>(image->pixelCount)));
        if (fread(image->alphaMap, 1, image->pixelCount, file) !=
            static_cast<size_t>(image->pixelCount)) {
            image->formatFlagsPacked |= 0x40;
            return -1;
        }
        image->formatFlagsPacked |= 0x40;
    }

    if ((image->formatFlagsPacked & 0x10) == 0 && image->paletteMetaPacked != 0) {
        const int paletteBytes = bytesPerPixel * image->paletteMetaPacked;
        image->palette = malloc(static_cast<size_t>(paletteBytes));
        if (fread(image->palette, 1, paletteBytes, file) !=
            static_cast<size_t>(paletteBytes)) {
            image->formatFlagsPacked |= 0x80;
            return -1;
        }
        image->formatFlagsPacked |= 0x80;
    }

    if (bytesPerPixel == 2 && (image->formatFlagsPacked & 0x10) == 0) {
        int rBits = 0;
        int gBits = 0;
        int bBits = 0;
        zVideo::PixelPack_GetRgbBits(&rBits, &gBits, &bBits);
        if (rBits == 5) {
            unsigned short *colors = image->paletteMetaPacked == 0
                                        ? static_cast<unsigned short *>(image->pixels)
                                        : static_cast<unsigned short *>(image->palette);
            int count =
                image->paletteMetaPacked == 0 ? image->pixelCount : image->paletteMetaPacked;
            while (count > 0) {
                const unsigned short value = *colors;
                *colors = static_cast<unsigned short>(((value >> 1) & 0x7fe0) | (value & 0x1f));
                ++colors;
                --count;
            }
        }
    }

    if (image->paletteMetaPacked != 0) {
        image->palette = zVid_PaletteRemap_BuildAllRecipeVariantsForPalette(
            static_cast<unsigned short *>(image->palette), image->paletteMetaPacked);
    }

    return 0;
}

// Reimplements 0x46ef70: zVid_Image::ReadFromFile
RECOIL_NOINLINE zVidImagePartial *RECOIL_FASTCALL ReadFromFile(FILE *file) {
    zVidImagePartial *image = Create();
    if (ReadHeader(file, image) != 0) {
        return 0;
    }

    image->pixels = malloc(QueryPixelDataBytes(image));
    ReadData(file, image, 0);
    image->formatFlagsPacked |= 0x20;
    return image;
}

// Reimplements 0x46e9b0: zVid_Image::ResampleSquare
RECOIL_NOINLINE void RECOIL_FASTCALL ResampleSquare(zVidImagePartial *image,
                                                    int sideLength) {
    const float inverseSideLength = 1.0f / static_cast<float>(sideLength);
    unsigned short *oldPixels = static_cast<unsigned short *>(image->pixels);
    char *oldAlphaMap = image->alphaMap;
    const int sourceWidth = image->width;
    const int sourceHeight = image->height;
    const float xScale = static_cast<float>(sourceWidth) * inverseSideLength;
    const float yScale = static_cast<float>(sourceHeight) * inverseSideLength;

    const unsigned int pixelCount = static_cast<unsigned int>(sideLength * sideLength);
    unsigned short *newPixels =
        static_cast<unsigned short *>(malloc(pixelCount * sizeof(unsigned short)));
    char *newAlphaMap = 0;
    if (oldAlphaMap != 0) {
        newAlphaMap = static_cast<char *>(malloc(pixelCount));
    }

    {
    for (int dstY = 0; dstY < sideLength; ++dstY) {
        const int srcY = static_cast<int>(static_cast<float>(dstY) * yScale);
        unsigned short *newPixelCursor = &newPixels[dstY * sideLength];
        char *newAlphaCursor = newAlphaMap != 0 ? &newAlphaMap[dstY * sideLength] : 0;

        {
        for (int dstX = 0; dstX < sideLength; ++dstX) {
            const int srcX = static_cast<int>(static_cast<float>(dstX) * xScale);
            const int sourceIndex = srcY * sourceWidth + srcX;
            *newPixelCursor++ = oldPixels[sourceIndex];
            if (oldAlphaMap != 0) {
                *newAlphaCursor++ = oldAlphaMap[sourceIndex];
            }
        }
        }
    }
    }

    free(image->pixels);
    image->height = static_cast<short>(sideLength);
    image->width = static_cast<short>(sideLength);
    image->pixels = newPixels;
    if (oldAlphaMap != 0) {
        free(oldAlphaMap);
        image->alphaMap = newAlphaMap;
    }
}
} // namespace zVid_Image

namespace zVid_PaletteRemap {
// Reimplements 0x46e680: zVid_PaletteRemap::FindRecipeIndex
RECOIL_NOINLINE int RECOIL_FASTCALL FindRecipeIndex(zVidPaletteRemapRecipe *recipe) {
    for (int i = 0; i < g_zVid_PaletteRemapRecipeCount; ++i) {
        zVidPaletteRemapRecipe *candidate = &g_zVid_PaletteRemapRecipes[i];
        if (recipe->color0R == candidate->color0R && recipe->color0G == candidate->color0G &&
            recipe->color0B == candidate->color0B && recipe->color1B == candidate->color1B &&
            recipe->color1R == candidate->color1R && recipe->color1G == candidate->color1G &&
            recipe->color0Strength == candidate->color0Strength &&
            recipe->color1Strength == candidate->color1Strength) {
            return i;
        }
    }

    return -1;
}

// Reimplements 0x46e4e0: zVid_PaletteRemap::ApplyRecipeToPaletteVariant
RECOIL_NOINLINE void RECOIL_FASTCALL ApplyRecipeToPaletteVariant(zVidPaletteRemapRecipe *recipe,
                                                                 unsigned short *sourceColors,
                                                                 int colorCount,
                                                                 int variantIndex,
                                                                 unsigned short *destColors) {
    int rBits = 0;
    int gBits = 0;
    int bBits = 0;
    zVideo::PixelPack_GetRgbBits(&rBits, &gBits, &bBits);

    const float variantWeight = static_cast<float>(variantIndex) * 0.0322580636f;
    const float inverseVariantWeight = 1.0f - variantWeight;

    while (colorCount > 0) {
        const unsigned short packed = *sourceColors++;
        float r = 0.0f;
        float g = 0.0f;
        if (gBits == 5) {
            r = static_cast<float>(packed & 0x7c00) * 3.15020152e-05f;
            g = static_cast<float>(packed & 0x03e0) * 0.00100806449f;
        } else {
            r = static_cast<float>(packed & 0xf800) * 1.57510076e-05f;
            g = static_cast<float>(packed & 0x07e0) * 0.000496031775f;
        }
        const float b = static_cast<float>(packed & 0x001f) * 0.0322580636f;

        zVideo_ColorRgbFloat color = {0};
        color.r = ((recipe->color0R - r) * inverseVariantWeight * recipe->color0Strength +
                   (recipe->color1R - r) * variantWeight * recipe->color1Strength + r) *
                  255.0f;
        color.g = ((recipe->color1G - g) * variantWeight * recipe->color1Strength +
                   (recipe->color0G - g) * inverseVariantWeight * recipe->color0Strength + g) *
                  255.0f;
        color.b = ((recipe->color1B - b) * variantWeight * recipe->color1Strength +
                   (recipe->color0B - b) * inverseVariantWeight * recipe->color0Strength + b) *
                  255.0f;

        *destColors++ = zVid_PackColorRgbFloats(&color);
        --colorCount;
    }
}
} // namespace zVid_PaletteRemap

// Reimplements 0x46e720: zVid_PaletteRemap_BuildPaletteVariant
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zVid_PaletteRemap_BuildPaletteVariant(zVidPaletteRemapRecipe *recipe) {
    const int existingIndex = zVid_PaletteRemap::FindRecipeIndex(recipe);
    if (existingIndex >= 0) {
        return existingIndex;
    }

    ++g_zVid_PaletteRemapRecipeCount;
    g_zVid_PaletteRemapRecipes = static_cast<zVidPaletteRemapRecipe *>(realloc(
        g_zVid_PaletteRemapRecipes,
        static_cast<size_t>(g_zVid_PaletteRemapRecipeCount) * sizeof(zVidPaletteRemapRecipe)));
    g_zVid_PaletteRemapRecipes[g_zVid_PaletteRemapRecipeCount - 1] = *recipe;

    const size_t expandedPaletteBytes =
        (static_cast<size_t>(g_zVid_PaletteRemapRecipeCount) << 14) + 0x200;
    for (int i = 0; i < g_zImage_TexDirEntryCount; ++i) {
        zVidImagePartial *image = g_zImage_TexDirEntries[i].image;
        if (image == 0 || image->paletteMetaPacked == 0 ||
            (image->formatFlagsPacked & 0x10) != 0) {
            continue;
        }

        image->palette = realloc(image->palette, expandedPaletteBytes);
        unsigned short *palette = static_cast<unsigned short *>(image->palette);
        {
        for (int variant = 0; variant < 32; ++variant) {
            zVid_PaletteRemap::ApplyRecipeToPaletteVariant(
                recipe, palette, image->paletteMetaPacked, variant,
                &palette[((g_zVid_PaletteRemapRecipeCount << 5) + variant - 0x1f) * 0x100]);
        }
        }
    }

    for (int tableIndex = 0; tableIndex < g_zVid_PaletteRemapVariantTableCount;
         ++tableIndex) {
        unsigned short *oldTable = g_zVid_PaletteRemapVariantTables[tableIndex];
        g_zVid_PaletteRemapVariantTables[tableIndex] =
            static_cast<unsigned short *>(realloc(oldTable, expandedPaletteBytes));
        unsigned short *table = g_zVid_PaletteRemapVariantTables[tableIndex];

        {
        for (int variant = 0; variant < 32; ++variant) {
            zVid_PaletteRemap::ApplyRecipeToPaletteVariant(
                recipe, table, 0x100, variant,
                &table[((g_zVid_PaletteRemapRecipeCount << 5) + variant - 0x1f) * 0x100]);
        }
        }

        for (int i = 0; i < g_zImage_TexDirEntryCount; ++i) {
            zVidImagePartial *image = g_zImage_TexDirEntries[i].image;
            if (image != 0 && image->palette == oldTable) {
                image->palette = table;
            }
        }
    }

    return g_zVid_PaletteRemapRecipeCount - 1;
}

// Reimplements 0x46e8d0: zVid_PaletteRemap_BuildAllRecipeVariantsForPalette
extern "C" RECOIL_NOINLINE unsigned short *RECOIL_FASTCALL
zVid_PaletteRemap_BuildAllRecipeVariantsForPalette(unsigned short *palette,
                                                   int colorCount) {
    if (g_zVid_PaletteRemapRecipeCount == 0) {
        return palette;
    }

    unsigned short *result = static_cast<unsigned short *>(realloc(
        palette, (static_cast<size_t>(g_zVid_PaletteRemapRecipeCount) << 14) + 0x200));

    for (int recipeIndex = 0; recipeIndex < g_zVid_PaletteRemapRecipeCount;
         ++recipeIndex) {
        unsigned short *dest = reinterpret_cast<unsigned short *>(
            reinterpret_cast<unsigned char *>(result) + 0x200 + recipeIndex * 0x4000);
        zVidPaletteRemapRecipe *recipe = &g_zVid_PaletteRemapRecipes[recipeIndex];
        {
        for (int variant = 0; variant < 32; ++variant) {
            zVid_PaletteRemap::ApplyRecipeToPaletteVariant(recipe, result, colorCount, variant,
                                                           dest);
            dest =
                reinterpret_cast<unsigned short *>(reinterpret_cast<unsigned char *>(dest) + 0x200);
        }
        }
    }

    return result;
}

// Reimplements 0x46e960: zVid_PaletteRemap_FindRecipeIndexFromRgb
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zVid_PaletteRemap_FindRecipeIndexFromRgb(zColorRgb *rgb) {
    zVidPaletteRemapRecipe recipe = {0};
    recipe.color1R = rgb->red;
    recipe.color1G = rgb->green;
    recipe.color1B = rgb->blue;
    recipe.color1Strength = 1.0f;
    return zVid_PaletteRemap::FindRecipeIndex(&recipe);
}

// Reimplements 0x46dae0: zVid_TexturePackEntry_LoadFromFile
extern "C" RECOIL_NOINLINE FILE *RECOIL_FASTCALL
zVid_TexturePackEntry_LoadFromFile(zVidTexturePackEntry *entry) {
    if (g_zVid_TexturePackLoadState == 0) {
        return 0;
    }

    entry->fileHandle = zUtil_ZRDR_OpenFileResolved(0, entry->filePath, "rb");
    if (entry->fileHandle == 0) {
        return 0;
    }

    if (fread(&entry->header, sizeof(entry->header), 1, entry->fileHandle) != 1 ||
        entry->header.fileFormat != 1) {
        fclose(entry->fileHandle);
        entry->fileHandle = 0;
        return 0;
    }

    entry->records = static_cast<zVidTexturePackRecord *>(malloc(
        static_cast<size_t>(entry->header.recordCount) * sizeof(zVidTexturePackRecord)));
    if (fread(entry->records, sizeof(zVidTexturePackRecord), entry->header.recordCount,
                   entry->fileHandle) != static_cast<size_t>(entry->header.recordCount)) {
        fclose(entry->fileHandle);
        entry->fileHandle = 0;
        free(entry->records);
        entry->records = 0;
        return 0;
    }

    entry->paletteTableBaseIndex = g_zVid_PaletteRemapVariantTableCount;
    if (entry->header.paletteTableCount <= 0) {
        return entry->fileHandle;
    }

    int tableIndex = g_zVid_PaletteRemapVariantTableCount;
    g_zVid_PaletteRemapVariantTableCount += entry->header.paletteTableCount;
    g_zVid_PaletteRemapVariantTables = static_cast<unsigned short **>(realloc(
        g_zVid_PaletteRemapVariantTables,
        static_cast<size_t>(g_zVid_PaletteRemapVariantTableCount) * sizeof(unsigned short *)));

    int rBits = 0;
    int gBits = 0;
    int bBits = 0;
    zVideo::PixelPack_GetRgbBits(&rBits, &gBits, &bBits);

    while (tableIndex < g_zVid_PaletteRemapVariantTableCount) {
        unsigned short *table = static_cast<unsigned short *>(malloc(0x200));
        g_zVid_PaletteRemapVariantTables[tableIndex] = table;
        if (fread(table, 2, 0x100, entry->fileHandle) != 0x100) {
            fclose(entry->fileHandle);
            entry->fileHandle = 0;
            free(entry->records);
            entry->records = 0;
            return 0;
        }

        if (gBits == 5) {
            {
            for (int byteOffset = 0; byteOffset < 0x200; byteOffset += 2) {
                unsigned short *color = reinterpret_cast<unsigned short *>(
                    reinterpret_cast<unsigned char *>(table) + byteOffset);
                const unsigned short value = *color;
                const unsigned short shifted = static_cast<unsigned short>(value >> 1);
                const unsigned short lowXor = static_cast<unsigned char>(
                    static_cast<unsigned char>(value) ^ static_cast<unsigned char>(shifted));
                *color = static_cast<unsigned short>((lowXor & 0x1f) ^ shifted);
            }
            }
        }

        g_zVid_PaletteRemapVariantTables[tableIndex] =
            zVid_PaletteRemap_BuildAllRecipeVariantsForPalette(table, 0x100);
        ++tableIndex;
    }

    return entry->fileHandle;
}

// Reimplements 0x46da40: zVid_TexturePack_EnsureDefaultImagePackLoaded
extern "C" RECOIL_NOINLINE void RECOIL_CDECL zVid_TexturePack_EnsureDefaultImagePackLoaded() {
    if (g_zVid_TexturePackCount > 0) {
        return;
    }

    g_zVid_TexturePacks = static_cast<zVidTexturePackEntry *>(
        realloc(g_zVid_TexturePacks, static_cast<size_t>(g_zVid_TexturePackCount + 1) *
                                              sizeof(zVidTexturePackEntry)));
    zVidTexturePackEntry *entry = &g_zVid_TexturePacks[g_zVid_TexturePackCount];
    memset(entry, 0, sizeof(*entry));
    strcpy(entry->filePath, "image.zbd");

    if (zVid_TexturePackEntry_LoadFromFile(entry) == 0) {
        sprintf(entry->filePath, "r%s", "image.zbd");
        if (zVid_TexturePackEntry_LoadFromFile(entry) == 0) {
            return;
        }
    }

    ++g_zVid_TexturePackCount;
}

// Reimplements 0x46df50: zVid_TexturePack_EnsureBuiltinTexturePacksLoaded
// (D:\Proj\GameZRecoil\zVideo\zVideo.cpp)
extern "C" RECOIL_NOINLINE RECOIL_NO_GS void RECOIL_CDECL
zVid_TexturePack_EnsureBuiltinTexturePacksLoaded() {
    if (g_zVid_BuiltinTexturePackCount > 0) {
        for (int i = 0; i < g_zVid_BuiltinTexturePackCount; ++i) {
            zVidTexturePackEntry *const entry = &g_zVid_BuiltinTexturePacks[i];
            if (entry->fileHandle == 0) {
                entry->fileHandle = zUtil_ZRDR_OpenFileResolved(g_zImage_MissionResourcePaths,
                                                                entry->filePath, "rb");
            }
        }
        return;
    }

    char filePath[0x20];
    int probeWasRendererMemory = 0;
    int candidateSize = 8;
    int totalBytes = 0;
    int freeBytes = 0;

    if (g_zVideo_pfnQueryTextureMemoryBytes(-1, &totalBytes, &freeBytes) != 0 &&
        g_zVideo_ActiveRendererPath != 0) {
        candidateSize = static_cast<unsigned int>(totalBytes) >> 20;
        sprintf(filePath, "r%s%d.%s", "texture", candidateSize, "zbd");
        probeWasRendererMemory = 1;
    } else {
        switch (*g_zImage_TextureMemoryOption) {
        case 1:
            sprintf(filePath, "%s8.%s", "texture", "zbd");
            candidateSize = 8;
            break;
        case 2:
            sprintf(filePath, "%s6.%s", "texture", "zbd");
            candidateSize = 6;
            break;
        case 3:
            sprintf(filePath, "%s4.%s", "texture", "zbd");
            candidateSize = 4;
            break;
        case 4:
            sprintf(filePath, "%s2.%s", "texture", "zbd");
            candidateSize = 2;
            break;
        default:
            sprintf(filePath, "%s", "texturemax.zbd");
            candidateSize = 8;
            break;
        }
    }

    g_zVid_BuiltinTexturePacks = static_cast<zVidTexturePackEntry *>(realloc(
        g_zVid_BuiltinTexturePacks, static_cast<size_t>(g_zVid_BuiltinTexturePackCount + 1) *
                                        sizeof(zVidTexturePackEntry)));
    zVidTexturePackEntry *const entry = &g_zVid_BuiltinTexturePacks[g_zVid_BuiltinTexturePackCount];
    memset(entry, 0, sizeof(*entry));
    strcpy(entry->filePath, filePath);

    if (zVid_TexturePackEntry_LoadFromFile(entry) == 0) {
        {
        for (int size = candidateSize; size >= -1; --size) {
            if (size > 0) {
                if (probeWasRendererMemory != 0) {
                    sprintf(filePath, "r%s%d.%s", "texture", size, "zbd");
                } else {
                    sprintf(filePath, "%s%d.%s", "texture", size, "zbd");
                }
            } else if (size == 0) {
                sprintf(filePath, "%s", "texturemax.zbd");
            } else {
                sprintf(filePath, "%s.%s", "texture", "zbd");
            }

            strcpy(entry->filePath, filePath);
            if (zVid_TexturePackEntry_LoadFromFile(entry) != 0) {
                break;
            }
        }
        }
    }

    if (entry->fileHandle != 0) {
        ++g_zVid_BuiltinTexturePackCount;
    }
}

namespace {
zVidImagePartial *LoadTexturePackImageByName(zVidTexturePackEntry *entries, int count,
                                             const char *imageName, bool builtin) {
    zVidImagePartial *result = 0;
    for (int i = 0; i < count && result == 0; ++i) {
        zVidTexturePackEntry *entry = &entries[i];
        if (entry->fileHandle == 0) {
            continue;
        }

        for (int recordIndex = 0;
             recordIndex < entry->header.recordCount && result == 0; ++recordIndex) {
            zVidTexturePackRecord *record = &entry->records[recordIndex];
            if (_stricmp(record->name, imageName) != 0) {
                continue;
            }

            fseek(entry->fileHandle, record->fileOffset, SEEK_SET);
            result = zVid_Image::ReadFromFile(entry->fileHandle);
            if (record->paletteIndex != -1) {
                const int tableIndex = entry->paletteTableBaseIndex + record->paletteIndex;
                if (builtin) {
                    if (result->palette != 0) {
                        free(result->palette);
                        result->palette = 0;
                        result->formatFlagsPacked &= static_cast<unsigned char>(~0x80);
                    }
                    result->paletteMetaPacked = 0x100;
                }
                result->palette = g_zVid_PaletteRemapVariantTables[tableIndex];
            }
        }
    }

    return result;
}
} // namespace

// Reimplements 0x46d940: zVid_TexturePack_LoadImageByName
extern "C" RECOIL_NOINLINE zVidImagePartial *RECOIL_FASTCALL
zVid_TexturePack_LoadImageByName(const char *imageName) {
    if (g_zVid_TexturePackCount == 0) {
        zVid_TexturePack_EnsureDefaultImagePackLoaded();
    }

    return LoadTexturePackImageByName(g_zVid_TexturePacks, g_zVid_TexturePackCount, imageName,
                                      false);
}

// Reimplements 0x46dd30: zVid_TexturePack_LoadBuiltinImageByName
extern "C" RECOIL_NOINLINE zVidImagePartial *RECOIL_FASTCALL
zVid_TexturePack_LoadBuiltinImageByName(const char *imageName) {
    return LoadTexturePackImageByName(g_zVid_BuiltinTexturePacks, g_zVid_BuiltinTexturePackCount,
                                      imageName, true);
}

namespace zVid_TexturePack {
void ClosePackEntry(zVidTexturePackEntry &entry) {
    if (entry.fileHandle != 0) {
        fclose(entry.fileHandle);
        entry.fileHandle = 0;
    }

    if (entry.records != 0) {
        free(entry.records);
        entry.records = 0;
    }
}

void FreePackEntryRecords(zVidTexturePackEntry &entry) {
    if (entry.records != 0) {
        free(entry.records);
        entry.records = 0;
    }
}
} // namespace zVid_TexturePack

namespace zImage {
// Reimplements 0x46d730: zImage::ShutdownTextureDirectoryRuntime
RECOIL_NOINLINE int RECOIL_CDECL ShutdownTextureDirectoryRuntime() {
    int count = g_zVid_BuiltinTexturePackCount;
    for (int i = 0; i < count; ++i) {
        zVidTexturePackEntry &entry = g_zVid_BuiltinTexturePacks[i];
        if (entry.fileHandle != 0) {
            fclose(entry.fileHandle);
            entry.fileHandle = 0;
        }
        count = g_zVid_BuiltinTexturePackCount;
    }

    return count;
}
} // namespace zImage

namespace zVid_TexturePack {
// Reimplements 0x46d6b0: zVid_TexturePack::ShutdownBuiltinPacks
RECOIL_NOINLINE void RECOIL_CDECL ShutdownBuiltinPacks() {
    zImage::ShutdownTextureDirectoryRuntime();

    for (int i = 0; i < g_zVid_BuiltinTexturePackCount; ++i) {
        FreePackEntryRecords(g_zVid_BuiltinTexturePacks[i]);
    }

    if (g_zVid_BuiltinTexturePacks != 0) {
        free(g_zVid_BuiltinTexturePacks);
        g_zVid_BuiltinTexturePacks = 0;
    }

    g_zVid_BuiltinTexturePackCount = 0;
}

RECOIL_NOINLINE void RECOIL_CDECL Shutdown() {
    for (int i = 0; i < g_zVid_TexturePackCount; ++i) {
        ClosePackEntry(g_zVid_TexturePacks[i]);
    }

    if (g_zVid_TexturePacks != 0) {
        free(g_zVid_TexturePacks);
        g_zVid_TexturePacks = 0;
    }

    g_zVid_TexturePackCount = 0;
}
} // namespace zVid_TexturePack

namespace zVid_TexDir {
RECOIL_NOINLINE int RECOIL_CDECL Shutdown() {
    for (int i = 0; i < g_zImage_TexDirEntryCount; ++i) {
        zImage_TexDirEntryPartial &entry = g_zImage_TexDirEntries[i];
        if (entry.loadState == 1) {
            if (entry.image != 0) {
                zVid_Image::ReleaseIfNotDefault(entry.image);
                entry.image = 0;
            }

            if (entry.texture != 0 && g_zVideo_pfnTextureRecordDestroy != 0) {
                reinterpret_cast<zVideo_DestroyTextureRecordProc>(g_zVideo_pfnTextureRecordDestroy)(
                    entry.texture);
                entry.texture = 0;
            }
        }

        if (entry.loadState == 1 || entry.loadState == 2) {
            entry.loadState = 0;
        }
    }

    g_zImage_TexDirEntryCount = 0;

    if (g_zVideo_pfnTextureRecordReleaseAllUploadSurfaces != 0) {
        reinterpret_cast<zVideo_ReleaseAllTextureUploadSurfacesProc>(
            g_zVideo_pfnTextureRecordReleaseAllUploadSurfaces)();
    }

    for (int i_2693 = 0; i_2693 < g_zVid_PaletteRemapVariantTableCount; ++i_2693) {
        free(g_zVid_PaletteRemapVariantTables[i_2693]);
        g_zVid_PaletteRemapVariantTables[i_2693] = 0;
    }

    g_zVid_PaletteRemapVariantTableCount = 0;
    if (g_zVid_PaletteRemapVariantTables != 0) {
        free(g_zVid_PaletteRemapVariantTables);
        g_zVid_PaletteRemapVariantTables = 0;
    }

    if (g_zVid_PaletteRemapRecipes != 0) {
        free(g_zVid_PaletteRemapRecipes);
        g_zVid_PaletteRemapRecipes = 0;
    }
    g_zVid_PaletteRemapRecipeCount = 0;

    zVid_TexturePack::ShutdownBuiltinPacks();
    zVid_TexturePack::Shutdown();
    return 0;
}
} // namespace zVid_TexDir

namespace zVideo_dd3d {
// Reimplements 0x4a6750: zVideo_dd3d::CallClearZBufferRect
RECOIL_NOINLINE void RECOIL_FASTCALL CallClearZBufferRect(zVidRect32 *rect) {
    g_zVideo_pfnClearZBufferRect(rect);
}

// Reimplements 0x4a6b70: zVideo_dd3d::SetPendingDitherEnable
RECOIL_NOINLINE void RECOIL_FASTCALL SetPendingDitherEnable(int enabled) {
    g_zVideo_PendingDitherEnable = enabled;
}

namespace {
const char *kZVideoDirect3DSourceFile = "D:\\Proj\\GameZRecoil\\zVideo\\zvid_ddd3d.c";

DWORD PackFogColorFrom255Floats(float red, float green, float blue) {
    const DWORD redByte = static_cast<DWORD>(static_cast<int>(red + 0.5f));
    const DWORD greenByte = static_cast<DWORD>(static_cast<int>(green + 0.5f));
    const DWORD blueByte = static_cast<DWORD>(static_cast<int>(blue + 0.5f));
    return ((redByte << 8) | greenByte) << 8 | blueByte;
}

DWORD PackD3DColorFrom16(unsigned int packedColor16, int alpha) {
    const DWORD red = (packedColor16 & g_zVideo_PixelPack_RMask) >> g_zVideo_PixelPack_RShift;
    const DWORD green = (packedColor16 & g_zVideo_PixelPack_GMask) >> g_zVideo_PixelPack_GShift;
    const DWORD blue = (packedColor16 & g_zVideo_PixelPack_BMask) << g_zVideo_PixelPack_BShiftTo8;
    return ((((red | (static_cast<DWORD>(alpha) << 8)) << 8) | green) << 8) | blue;
}

void WriteFlatTlVertex(D3DTLVERTEX &dst, const zVideo_XyzVertex &src, DWORD packedColor) {
    dst.sx = src.x;
    dst.sy = src.y;
    dst.sz = src.z;
    dst.rhw = src.z;
    dst.color = packedColor;
    dst.specular = 0xff000000;
}

void CopyFlatVerticesReverse(D3DTLVERTEX *dst, const zVideo_XyzVertex *vertices,
                             int vertexCount, DWORD packedColor) {
    for (int i = 0; i < vertexCount; ++i) {
        WriteFlatTlVertex(dst[i], vertices[vertexCount - 1 - i], packedColor);
    }
}

void CopyGouraudVerticesReverse(D3DTLVERTEX *dst, const zVideo_XyzVertex *vertices,
                                const unsigned int *packedColors16, int vertexCount,
                                int alpha) {
    for (int i = 0; i < vertexCount; ++i) {
        const int sourceIndex = vertexCount - 1 - i;
        WriteFlatTlVertex(dst[i], vertices[sourceIndex],
                          PackD3DColorFrom16(packedColors16[sourceIndex], alpha));
    }
}

DWORD PackColorAttrConstant(const zVideo_ColorRgbFloat &baseColor, float attr1Scale,
                            DWORD alphaBits) {
    const DWORD red =
        static_cast<DWORD>(static_cast<int>(baseColor.r * attr1Scale + 0.5f));
    const DWORD green =
        static_cast<DWORD>(static_cast<int>(baseColor.g * attr1Scale + 0.5f));
    const DWORD blue =
        static_cast<DWORD>(static_cast<int>(baseColor.b * attr1Scale + 0.5f));
    return alphaBits | (((red << 8) | green) << 8) | blue;
}

DWORD PackColorAttrBiased(const zVideo_ColorRgbFloat &baseColor, float attr1Scale, float attr0Value,
                          DWORD alphaBits) {
    float red = baseColor.r * attr1Scale + attr0Value * g_zVideo_D3DColorAttrBiasR;
    float green = baseColor.g * attr1Scale + attr0Value * g_zVideo_D3DColorAttrBiasG;
    float blue = baseColor.b * attr1Scale + attr0Value * g_zVideo_D3DColorAttrBiasB;

    const float channels[3] = {red, green, blue};
    const float selected = channels[g_zVideo_D3DColorNormalizeChannelIndex];
    if (selected > 255.0f) {
        const float scale = 255.0f / selected;
        red *= scale;
        green *= scale;
        blue *= scale;
    }

    const DWORD redByte = static_cast<DWORD>(static_cast<int>(red));
    const DWORD greenByte = static_cast<DWORD>(static_cast<int>(green));
    const DWORD blueByte = static_cast<DWORD>(static_cast<int>(blue));
    return alphaBits | (((redByte << 8) | greenByte) << 8) | blueByte;
}

void FillColorAttrSpecularReverse(const float *attr2, int vertexCount) {
    for (int i = 0; i < vertexCount; ++i) {
        DWORD specular = 0xff000000;
        if (attr2 != 0) {
            const float source = attr2[vertexCount - 1 - i];
            specular =
                static_cast<DWORD>(static_cast<int>(0.5f + (1.0f - source) * 255.0f))
                << 24;
        }
        g_zVideo_D3DSubmitTempVertices[i].specular = specular;
    }
}

void FillColorAttrColorsReverse(const zVideo_ColorRgbFloat &baseColor, const float *attr0,
                                float attr1Scale, DWORD alphaBits, int vertexCount) {
    const DWORD constantColor = PackColorAttrConstant(baseColor, attr1Scale, alphaBits);
    for (int i = 0; i < vertexCount; ++i) {
        DWORD color = constantColor;
        if (attr0 != 0) {
            const float attr0Value = attr0[vertexCount - 1 - i];
            if (attr0Value > (1.0f / 255.0f)) {
                color = PackColorAttrBiased(baseColor, attr1Scale, attr0Value, alphaBits);
            }
        }
        g_zVideo_D3DSubmitTempVertices[i].color = color;
    }
}

void CopyPositionsReverse(D3DTLVERTEX *dst, const zVideo_XyzVertex *vertices,
                          int vertexCount) {
    for (int i = 0; i < vertexCount; ++i) {
        const zVideo_XyzVertex &src = vertices[vertexCount - 1 - i];
        dst[i].sx = src.x;
        dst[i].sy = src.y;
        dst[i].sz = src.z;
        dst[i].rhw = src.z;
    }
}

DWORD PackAlphaWhite(float alpha) {
    return (static_cast<DWORD>(static_cast<int>(alpha * 255.0f)) << 24) | 0x00ffffff;
}

void WriteTexturedTlVertex(D3DTLVERTEX &dst, const zVideo_XyzVertex &src,
                           const zVideo_TexCoord &texCoord, DWORD color) {
    dst.sx = src.x;
    dst.sy = src.y;
    dst.sz = src.z;
    dst.rhw = src.z;
    dst.color = color;
    dst.specular = 0xff000000;
    dst.tu = texCoord.u;
    dst.tv = texCoord.v;
}

void CopyTexturedVerticesReverse(D3DTLVERTEX *dst, const zVideo_XyzVertex *vertices,
                                 const zVideo_TexCoord *texCoords, int vertexCount,
                                 DWORD color) {
    for (int i = 0; i < vertexCount; ++i) {
        const int sourceIndex = vertexCount - 1 - i;
        WriteTexturedTlVertex(dst[i], vertices[sourceIndex], texCoords[sourceIndex], color);
    }
}

DWORD PackGrayColor(float gray, DWORD alphaBits, bool clampHigh) {
    DWORD grayByte = static_cast<DWORD>(static_cast<int>(gray));
    if (clampHigh && grayByte > 0xff) {
        grayByte = 0xff;
    }
    return alphaBits | (((grayByte << 8) | grayByte) << 8) | grayByte;
}

DWORD PackPolygonBiasedColor(float grayBase, float attr0Value, DWORD alphaBits) {
    float red = grayBase + attr0Value * g_zVideo_D3DColorAttrBiasR;
    float green = grayBase + attr0Value * g_zVideo_D3DColorAttrBiasG;
    float blue = grayBase + attr0Value * g_zVideo_D3DColorAttrBiasB;

    const float channels[3] = {red, green, blue};
    const float selected = channels[g_zVideo_D3DColorNormalizeChannelIndex];
    if (selected > 255.0f) {
        const float scale = 255.0f / selected;
        red *= scale;
        green *= scale;
        blue *= scale;
    }

    const DWORD redByte = static_cast<DWORD>(static_cast<int>(red));
    const DWORD greenByte = static_cast<DWORD>(static_cast<int>(green));
    const DWORD blueByte = static_cast<DWORD>(static_cast<int>(blue));
    return alphaBits | (((redByte << 8) | greenByte) << 8) | blueByte;
}

void FillPolygonColorsReverse(const float *attr0, float grayBase, DWORD alphaBits,
                              int vertexCount) {
    if (attr0 == 0) {
        const DWORD color = PackGrayColor(grayBase, alphaBits, false);
        for (int i = 0; i < vertexCount; ++i) {
            g_zVideo_D3DSubmitTempVertices[i].color = color;
        }
        return;
    }

    for (int i = 0; i < vertexCount; ++i) {
        const float attr0Value = attr0[vertexCount - 1 - i];
        DWORD color;
        if (attr0Value > (1.0f / 255.0f)) {
            color = PackPolygonBiasedColor(grayBase, attr0Value, alphaBits);
        } else {
            color = PackGrayColor(grayBase, alphaBits, true);
        }
        g_zVideo_D3DSubmitTempVertices[i].color = color;
    }
}

void FillPolygonLitColorsReverse(const float *attr1, const float *attr0, DWORD alphaBits,
                                 int vertexCount) {
    for (int i = 0; i < vertexCount; ++i) {
        const int sourceIndex = vertexCount - 1 - i;
        const float grayBase = (1.0f - attr1[sourceIndex]) * 255.0f;
        DWORD color;
        if (attr0 != 0 && attr0[sourceIndex] > (1.0f / 255.0f)) {
            color = PackPolygonBiasedColor(grayBase, attr0[sourceIndex], alphaBits);
        } else {
            color = PackGrayColor(grayBase, alphaBits, attr0 != 0);
        }
        g_zVideo_D3DSubmitTempVertices[i].color = color;
    }
}

void CopyPositionUvReversePreserveColor(D3DTLVERTEX *dst, const zVideo_XyzVertex *vertices,
                                        const zVideo_TexCoord *uvPairs, int vertexCount) {
    for (int i = 0; i < vertexCount; ++i) {
        const int sourceIndex = vertexCount - 1 - i;
        const zVideo_XyzVertex &src = vertices[sourceIndex];
        const zVideo_TexCoord &uv = uvPairs[sourceIndex];
        dst[i].sx = src.x;
        dst[i].sy = src.y;
        dst[i].sz = src.z;
        dst[i].rhw = src.z;
        dst[i].tu = uv.u;
        dst[i].tv = uv.v;
    }
}

void CopyPositionUvWithPreparedColorReverse(D3DTLVERTEX *dst, const zVideo_XyzVertex *vertices,
                                            const zVideo_TexCoord *uvPairs,
                                            const D3DTLVERTEX *prepared, int vertexCount) {
    for (int i = 0; i < vertexCount; ++i) {
        const int sourceIndex = vertexCount - 1 - i;
        const zVideo_XyzVertex &src = vertices[sourceIndex];
        const zVideo_TexCoord &uv = uvPairs[sourceIndex];
        dst[i].sx = src.x;
        dst[i].sy = src.y;
        dst[i].sz = src.z;
        dst[i].rhw = src.z;
        dst[i].color = prepared[i].color;
        dst[i].specular = prepared[i].specular;
        dst[i].tu = uv.u;
        dst[i].tv = uv.v;
    }
}

void AppendFanCloseVertexIfNeeded(D3DTLVERTEX *vertices, int &count) {
    if (g_zVideo_D3DAppendFanCloseVertexPending != 0) {
        vertices[count] = vertices[1];
        ++count;
        g_zVideo_D3DAppendFanCloseVertexPending = 0;
    }
}
} // namespace

// Reimplements 0x4a9b70: zVideo_dd3d::PresentDisplayModeSurface
RECOIL_NOINLINE int RECOIL_FASTCALL
PresentDisplayModeSurface(zVidRect32 *srcRect, zVidRect32 *dstRect, int waitForPresent,
                          int blitPrimaryToSwFirst) {
    if (g_zVideo_DisplayModeSurfaceState.surf == 0) {
        return 0x400;
    }

    for (;;) {
        if (blitPrimaryToSwFirst != 0) {
            const DWORD bltFlags = waitForPresent != 0 ? DDBLT_WAIT : 0;
            g_zVideo_SwSurfaceState.surf->Blt(reinterpret_cast<RECT *>(dstRect),
                                              g_zVideo_PrimarySurfaceState.surf,
                                              reinterpret_cast<RECT *>(srcRect), bltFlags, 0);
        }

        HRESULT hresult = g_zVideo_DisplayModeSurfaceState.surf->Flip(
            0, waitForPresent != 0 ? DDFLIP_WAIT : 0);
        if (hresult == DD_OK) {
            return 0;
        }

        if (hresult == DDERR_WASSTILLDRAWING) {
            continue;
        }

        if (hresult == DDERR_SURFACELOST) {
            hresult = g_zVideo_DisplayModeSurfaceState.surf->Restore();
        }

        if (hresult == DD_OK) {
            continue;
        }

        zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile, 0xae);
        return 0x5a56ffff;
    }
}

// Reimplements 0x4aa0f0: zVideo_dd3d::CreateTextureRecord
RECOIL_NOINLINE zVideo_TextureRecordPartial *RECOIL_FASTCALL
CreateTextureRecord(const char *textureName, zVidImagePartial *image, int useAlpha,
                    int clampU, int clampV) {
    IDirectDrawSurface *uploadSurface = 0;
    IDirectDrawSurface *textureSurface = 0;
    IDirect3DTexture2 *uploadTexture = 0;
    IDirect3DTexture2 *texture = 0;

    const D3DDEVICEDESC *selectedDeviceDesc =
        reinterpret_cast<const D3DDEVICEDESC *>(g_zVideo_pSelectedD3DDeviceInfo->m_hwDesc);
    int width = image->width;
    int height = image->height;
    if (static_cast<DWORD>(width) > selectedDeviceDesc->dwMaxTextureWidth ||
        static_cast<DWORD>(height) > selectedDeviceDesc->dwMaxTextureHeight) {
        zError::ReportOld(
            0x200, kZVideoDirect3DSourceFile, 0x20e,
            "Texture [%s] dimensions [%d x %d] are too large.  Using default texture.", textureName,
            width, height);
        return g_zImage_DefaultTextureRecord;
    }

    if ((g_zVideo_D3DHalDeviceDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) != 0 &&
        (FloorPowerOfTwo(width) != width || FloorPowerOfTwo(height) != height)) {
        zError::ReportOld(
            0x200, kZVideoDirect3DSourceFile, 0x224,
            "Texture [%s] dimensions [%d x %d] are not power of 2.Using default texture.",
            textureName, image->width, image->height);
        return g_zImage_DefaultTextureRecord;
    }

    if (width > height * 8 || height > width * 8) {
        zError::ReportOld(
            0x200, kZVideoDirect3DSourceFile, 0x233,
            "Texture [%s] dimensions [%d x %d] have bad aspect ratio.Using default texture.",
            textureName, width, height);
        return g_zImage_DefaultTextureRecord;
    }

    if ((g_zVideo_D3DHalDeviceDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY) != 0 &&
        image->width != image->height) {
        const int squareSide = FloorPowerOfTwo(
            static_cast<int>(sqrt(static_cast<double>(height * width))));
        zVid_Image::ResampleSquare(image, squareSide);
        width = image->width;
        height = image->height;
    }

    if (image->palette != 0) {
        zError::ReportOld(0x200, kZVideoDirect3DSourceFile, 0x24a,
                          "Texture [%s] Palettes not supported  Using default texture.",
                          textureName);
        return g_zImage_DefaultTextureRecord;
    }

    DDSURFACEDESC desc = {0};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    desc.dwHeight = static_cast<DWORD>(height);
    desc.dwWidth = static_cast<DWORD>(width);
    desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    desc.ddpfPixelFormat.dwSize = sizeof(desc.ddpfPixelFormat);
    desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
    desc.ddpfPixelFormat.dwRGBBitCount = 16;

    int redBits;
    int greenBits;
    int blueBits;
    int alphaBits;
    DWORD redMask;
    DWORD greenMask;
    DWORD blueMask;
    DWORD alphaMask;
    if (useAlpha == 0) {
        redBits = g_zVideo_PixelPack_RBits;
        greenBits = g_zVideo_PixelPack_GBits;
        blueBits = g_zVideo_PixelPack_BBits;
        alphaBits = 0;
        redMask = g_zVideo_PixelPack_RMask;
        greenMask = g_zVideo_PixelPack_GMask;
        blueMask = g_zVideo_PixelPack_BMask;
        alphaMask = 0;
    } else {
        desc.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
        if (image->alphaMap != 0) {
            redBits = 4;
            greenBits = 4;
            blueBits = 4;
            alphaBits = 4;
            redMask = 0x0f00;
            greenMask = 0x00f0;
            blueMask = 0x000f;
            alphaMask = 0xf000;
        } else {
            redBits = 5;
            greenBits = 5;
            blueBits = 5;
            alphaBits = 1;
            redMask = 0x7c00;
            greenMask = 0x03e0;
            blueMask = 0x001f;
            alphaMask = 0x8000;
        }
    }

    desc.ddpfPixelFormat.dwRBitMask = redMask;
    desc.ddpfPixelFormat.dwGBitMask = greenMask;
    desc.ddpfPixelFormat.dwBBitMask = blueMask;
    desc.ddpfPixelFormat.dwRGBAlphaBitMask = alphaMask;
    zVideo::TexturePixelPack_SetupFromMasks(redBits, greenBits, blueBits, alphaBits, redMask,
                                            greenMask, blueMask, alphaMask);

    HRESULT hresult = g_zVideo_pDirectDraw2->CreateSurface(&desc, &uploadSurface, 0);
    zVideo_TextureRecordPartial *result = 0;
    if (hresult == DD_OK) {
        UploadImageToSurface(uploadSurface, image, useAlpha);
        hresult = uploadSurface->QueryInterface(IID_IDirect3DTexture2,
                                                reinterpret_cast<void **>(&uploadTexture));
    }

    D3DTEXTUREHANDLE textureHandle = 0;
    if (hresult == DD_OK) {
        desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_ALLOCONLOAD;
        if ((g_zVideo_D3DHalDeviceDesc.dwDevCaps & D3DDEVCAPS_TEXTURENONLOCALVIDMEM) != 0) {
            desc.ddsCaps.dwCaps |= DDSCAPS_NONLOCALVIDMEM;
        }

        hresult = g_zVideo_pDirectDraw2->CreateSurface(&desc, &textureSurface, 0);
    }
    if (hresult == DD_OK) {
        hresult = textureSurface->QueryInterface(IID_IDirect3DTexture2,
                                                 reinterpret_cast<void **>(&texture));
    }
    if (hresult == DD_OK) {
        hresult = texture->Load(uploadTexture);
    }
    if (hresult == DD_OK) {
        hresult = texture->GetHandle(g_zVideo_pD3DDevice, &textureHandle);
    }
    if (hresult == DD_OK) {
        result = TextureRecord_Create();
        if (result != 0) {
            result->m_uploadSurface = uploadSurface;
            result->m_textureSurface = textureSurface;
            result->m_texture = texture;
            result->m_textureHandle = textureHandle;
            result->m_alphaMode = useAlpha == 0 ? 1 : (image->alphaMap != 0 ? 4 : 5);
            result->m_uWrapMode = clampU != 0 ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP;
            result->m_vWrapMode = clampV != 0 ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP;
        }
        uploadTexture->Release();
    }

    if (hresult != DD_OK) {
        zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile,
                               0x30f);
        if (texture != 0) {
            texture->Release();
        }
        if (uploadTexture != 0) {
            uploadTexture->Release();
        }
        if (textureSurface != 0) {
            textureSurface->Release();
        }
        if (uploadSurface != 0) {
            uploadSurface->Release();
        }
    }

    return result != 0 ? result : g_zImage_DefaultTextureRecord;
}

// Reimplements 0x4a9c20: zVideo_dd3d::CreateDeviceState
RECOIL_NOINLINE int RECOIL_CDECL CreateDeviceState() {
    DDSURFACEDESC zBufferDesc = {0};
    zBufferDesc.dwWidth = static_cast<DWORD>(g_zVideo_SwSurfaceState.width);
    zBufferDesc.dwHeight = static_cast<DWORD>(g_zVideo_SwSurfaceState.height);
    g_zVideo_ClearScreenBufferEnabled = 1;
    zBufferDesc.dwSize = sizeof(zBufferDesc);
    zBufferDesc.dwFlags = 0x47;
    zBufferDesc.ddsCaps.dwCaps = 0x24000;
    zBufferDesc.dwMipMapCount = 0x10;

    HRESULT hresult = g_zVideo_pDirectDraw2->CreateSurface(
        &zBufferDesc, reinterpret_cast<IDirectDrawSurface **>(&g_zVideo_pZBufferSurface), 0);
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile,
                                      0xd3);
    }

    hresult = g_zVideo_pZBufferSurface->QueryInterface(
        IID_IDirectDrawSurface, reinterpret_cast<void **>(&g_zVideo_pZBufferAttachSurface));
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile,
                                      0xd9);
    }

    hresult = g_zVideo_SwSurfaceState.surf->AddAttachedSurface(
        reinterpret_cast<IDirectDrawSurface3 *>(g_zVideo_pZBufferAttachSurface));
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile,
                                      0xde);
    }

    hresult = g_zVideo_pDirectDraw2->QueryInterface(IID_IDirect3D2,
                                                    reinterpret_cast<void **>(&g_zVideo_pD3D2));
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile,
                                      0xe5);
    }

    hresult = g_zVideo_pD3D2->CreateDevice(
        *g_zVideo_pSelectedD3DDeviceInfo->pD3DDeviceGuid,
        reinterpret_cast<IDirectDrawSurface *>(g_zVideo_SwSurfaceState.surf), &g_zVideo_pD3DDevice);
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile,
                                      0xed);
    }

    hresult = g_zVideo_pD3D2->CreateViewport(&g_zVideo_pD3DViewport2, 0);
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile,
                                      0xf4);
    }

    hresult = g_zVideo_pD3DDevice->AddViewport(g_zVideo_pD3DViewport2);
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile,
                                      0xf9);
    }

    const int width = g_zVideo_DisplayModeSurfaceState.width;
    const int height = g_zVideo_DisplayModeSurfaceState.height;
    D3DVIEWPORT2 viewport2 = {0};
    viewport2.dwSize = sizeof(viewport2);
    viewport2.dwX = 0;
    viewport2.dwY = 0;
    viewport2.dwWidth = static_cast<DWORD>(width);
    viewport2.dwHeight = static_cast<DWORD>(height);
    viewport2.dvClipX = 0.0f;
    viewport2.dvClipY = 0.0f;
    viewport2.dvClipWidth = static_cast<D3DVALUE>(width);
    viewport2.dvClipHeight = static_cast<D3DVALUE>(height);
    viewport2.dvMinZ = 0.0f;
    viewport2.dvMaxZ = 1.0f;

    hresult = g_zVideo_pD3DViewport2->SetViewport2(&viewport2);
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile,
                                      0x10a);
    }

    hresult = g_zVideo_pD3DDevice->SetCurrentViewport(g_zVideo_pD3DViewport2);
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile,
                                      0x10f);
    }

    hresult = g_zVideo_pD3D2->CreateMaterial(&g_zVideo_pD3DMaterial2, 0);
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile,
                                      0x116);
    }

    D3DMATERIAL mat = {0};
    mat.dwSize = sizeof(mat);
    mat.diffuse.r = 0.0f;
    mat.diffuse.g = 0.0f;
    mat.diffuse.b = 0.0f;
    mat.ambient.r = 1.0f;
    mat.ambient.g = 1.0f;
    mat.ambient.b = 1.0f;
    mat.dwRampSize = 0x100;

    hresult = g_zVideo_pD3DMaterial2->SetMaterial(&mat);
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile,
                                      0x124);
    }

    hresult = g_zVideo_pD3DMaterial2->GetHandle(g_zVideo_pD3DDevice, &g_zVideo_D3DMaterialHandle);
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile,
                                      0x12a);
    }

    hresult = g_zVideo_pD3DViewport2->SetBackground(g_zVideo_D3DMaterialHandle);
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile,
                                      0x12f);
    }

    g_zVideo_D3DHelDeviceDesc.dwSize = sizeof(g_zVideo_D3DHelDeviceDesc);
    g_zVideo_D3DHalDeviceDesc.dwSize = sizeof(g_zVideo_D3DHalDeviceDesc);
    hresult = g_zVideo_pD3DDevice->GetCaps(&g_zVideo_D3DHalDeviceDesc, &g_zVideo_D3DHelDeviceDesc);
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile,
                                      0x139);
    }

    g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_CULLMODE, 1);
    g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, 1);
    g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZFUNC, 7);
    g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, 0);
    g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, 1);
    g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE, 1);
    g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMAG, 2);
    g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMIN, 2);
    g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, 5);
    g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, 6);

    g_zVideo_PendingWireframeState = -1;
    SetFogEnable(1);
    SetQuadBatchDepthAndRhw(0.99000001f);
    return 0;
}

// Reimplements 0x4aa9e0: zVideo_dd3d::SetFogEnable
RECOIL_NOINLINE void RECOIL_FASTCALL SetFogEnable(int enable) {
    if (g_zVideo_CachedFogEnableRenderState != enable) {
        g_zVideo_pD3DDevice->SetRenderState(static_cast<D3DRENDERSTATETYPE>(0x1c),
                                            static_cast<DWORD>(enable));
        g_zVideo_CachedFogEnableRenderState = enable;
    }

    if (g_zVideo_CachedFogModeLightState != 3) {
        g_zVideo_pD3DDevice->SetLightState(static_cast<D3DLIGHTSTATETYPE>(4), 3);
        g_zVideo_CachedFogModeLightState = 3;
    }
}

// Reimplements 0x4aaa30: zVideo_dd3d::SetFogStart
RECOIL_NOINLINE void RECOIL_STDCALL SetFogStart(float fogStart) {
    if (g_zVideo_CachedFogStartLightStateValue != fogStart) {
        g_zVideo_pD3DDevice->SetLightState(static_cast<D3DLIGHTSTATETYPE>(5),
                                           *reinterpret_cast<DWORD *>(&fogStart));
        g_zVideo_CachedFogStartLightStateValue = fogStart;
    }
}

// Reimplements 0x4aaa60: zVideo_dd3d::SetFogEnd
RECOIL_NOINLINE void RECOIL_STDCALL SetFogEnd(float fogEnd) {
    if (g_zVideo_CachedFogEndLightStateValue != fogEnd) {
        g_zVideo_pD3DDevice->SetLightState(static_cast<D3DLIGHTSTATETYPE>(5),
                                           *reinterpret_cast<DWORD *>(&fogEnd));
        g_zVideo_CachedFogEndLightStateValue = fogEnd;
    }
}

// Reimplements 0x4aaa90: zVideo_dd3d::ApplyFogStateFromGlobals
RECOIL_NOINLINE void RECOIL_STDCALL ApplyFogStateFromGlobals(float fogStart, float fogEnd,
                                                             float unused) {
    (void)unused;
    g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, 1);

    g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGCOLOR,
                                        PackFogColorFrom255Floats(g_zVideo_FogColorPendingR255,
                                                                  g_zVideo_FogColorPendingG255,
                                                                  g_zVideo_FogColorPendingB255));

    g_zVideo_pD3DDevice->SetLightState(D3DLIGHTSTATE_FOGMODE, D3DFOG_LINEAR);
    g_zVideo_pD3DDevice->SetLightState(static_cast<D3DLIGHTSTATETYPE>(5),
                                       *reinterpret_cast<DWORD *>(&fogStart));
    g_zVideo_pD3DDevice->SetLightState(static_cast<D3DLIGHTSTATETYPE>(6),
                                       *reinterpret_cast<DWORD *>(&fogEnd));
}

// Reimplements 0x4aab30: zVideo_dd3d::UpdateFogColor
RECOIL_NOINLINE void RECOIL_CDECL UpdateFogColor() {
    g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGCOLOR,
                                        PackFogColorFrom255Floats(g_zVideo_FogColorAppliedR255,
                                                                  g_zVideo_FogColorAppliedG255,
                                                                  g_zVideo_FogColorAppliedB255));
}

// Reimplements 0x4accc0: zVideo_dd3d::SetQuadBatchDepthAndRhw
RECOIL_NOINLINE void RECOIL_STDCALL SetQuadBatchDepthAndRhw(float depthAndRhw) {
    {
    for (int itemIndex = 0; itemIndex < 16; ++itemIndex) {
        zVideo_QuadBatchItemPartial &item = g_zVideo_QuadBatchItemsBase[itemIndex];
        {
        for (int vertexIndex = 0; vertexIndex < 4; ++vertexIndex) {
            item.vertices[vertexIndex].sz = depthAndRhw;
            item.vertices[vertexIndex].rhw = depthAndRhw;
        }
        }
    }
    }
}

// Reimplements 0x4aab90: zVideo_dd3d::SubmitPolyFlatColor16
RECOIL_NOINLINE void RECOIL_FASTCALL
SubmitPolyFlatColor16(zVideo_XyzVertex *vertices, unsigned int packedColor16, int alpha,
                      int renderParam, int vertexCount, int queueMode) {
    const DWORD packedColor = PackD3DColorFrom16(packedColor16, alpha);

    if (alpha >= 0xff) {
        CopyFlatVerticesReverse(g_zVideo_D3DSubmitTempVertices, vertices, vertexCount, packedColor);

        if (queueMode != 0) {
            const int queueIndex = g_zVideo_OverwriteQueueCount;
            if (queueIndex >= 0x180) {
                zError::ReportOld(0x400, kZVideoDirect3DSourceFile, 0x503,
                                  "Not enough ZVID_MAX_OVERWRITE_POLYS: needs %d", queueIndex);
                return;
            }

            zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
            ++g_zVideo_OverwriteQueueCount;
            entry.type = 1;
            entry.vertexCount = vertexCount;
            entry.renderClass = 0;
            entry.renderParam = renderParam;
            if (vertexCount > 0) {
                memcpy(entry.vertices, g_zVideo_D3DSubmitTempVertices,
                            static_cast<size_t>(vertexCount) * sizeof(D3DTLVERTEX));
            }
            return;
        }

        if (g_zVideo_D3DRenderState_TextureHandle != 0) {
            g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, 0);
            g_zVideo_D3DRenderState_TextureHandle = 0;
        }
        if (g_zVideo_D3DRenderState_ShadeMode != 1) {
            g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, 1);
            g_zVideo_D3DRenderState_ShadeMode = 1;
        }

        const HRESULT hresult = g_zVideo_pD3DDevice->DrawPrimitive(
            static_cast<D3DPRIMITIVETYPE>(6), static_cast<D3DVERTEXTYPE>(3),
            g_zVideo_D3DSubmitTempVertices, static_cast<DWORD>(vertexCount), 0);
        if (hresult != DD_OK) {
            zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile,
                                   0x520);
        }
        return;
    }

    if (queueMode != 0) {
        const int queueIndex = g_zVideo_OverwriteQueueCount;
        if (queueIndex >= 0x180) {
            zError::ReportOld(0x400, kZVideoDirect3DSourceFile, 0x528,
                              "Not enough ZVID_MAX_OVERWRITE_POLYS: need %d", queueIndex);
            return;
        }

        zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
        ++g_zVideo_OverwriteQueueCount;
        entry.type = 0;
        entry.vertexCount = vertexCount;
        entry.renderClass = 0;
        entry.renderParam = renderParam;
        if (vertexCount > 0) {
            CopyFlatVerticesReverse(entry.vertices, vertices, vertexCount, packedColor);
        }
        return;
    }

    const int queueIndex = g_zVideo_SortedPolyQueueCount;
    if (static_cast<unsigned int>(queueIndex) >= 0x100) {
        zError::ReportOld(0x400, kZVideoDirect3DSourceFile, 0x547,
                          "Not enough MAX_TRANSPARENT_POLYS: need %d", queueIndex);
        return;
    }

    zVideo_SortedPolyQueueEntry &entry = g_zVideo_SortedPolyQueueBase[queueIndex];
    entry.vertexCount = vertexCount;
    entry.renderClass = 0;
    entry.renderParam = renderParam;
    if (vertexCount > 0) {
        CopyFlatVerticesReverse(entry.vertices, vertices, vertexCount, packedColor);
    }
    ++g_zVideo_SortedPolyQueueCount;
}

// Reimplements 0x4aaef0: zVideo_dd3d::SubmitPolyGouraudColor16
RECOIL_NOINLINE void RECOIL_FASTCALL SubmitPolyGouraudColor16(
    zVideo_XyzVertex *vertices, unsigned int *packedColors16, int alpha,
    int renderParam, int vertexCount, int queueMode) {
    if (alpha >= 0xff) {
        CopyGouraudVerticesReverse(g_zVideo_D3DSubmitTempVertices, vertices, packedColors16,
                                   vertexCount, alpha);

        if (queueMode != 0) {
            const int queueIndex = g_zVideo_OverwriteQueueCount;
            if (queueIndex >= 0x180) {
                zError::ReportOld(0x400, kZVideoDirect3DSourceFile, 0x59d,
                                  "Not enough ZVID_MAX_OVERWRITE_POLYS: need %d", queueIndex);
                return;
            }

            zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
            ++g_zVideo_OverwriteQueueCount;
            entry.type = 2;
            entry.vertexCount = vertexCount;
            entry.renderClass = 0;
            entry.renderParam = renderParam;
            if (vertexCount > 0) {
                memcpy(entry.vertices, g_zVideo_D3DSubmitTempVertices,
                            static_cast<size_t>(vertexCount) * sizeof(D3DTLVERTEX));
            }
            return;
        }

        if (g_zVideo_D3DRenderState_TextureHandle != 0) {
            g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, 0);
            g_zVideo_D3DRenderState_TextureHandle = 0;
        }
        if (g_zVideo_D3DRenderState_ShadeMode != 1) {
            g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, 1);
            g_zVideo_D3DRenderState_ShadeMode = 1;
        }

        const HRESULT hresult = g_zVideo_pD3DDevice->DrawPrimitive(
            static_cast<D3DPRIMITIVETYPE>(6), static_cast<D3DVERTEXTYPE>(3),
            g_zVideo_D3DSubmitTempVertices, static_cast<DWORD>(vertexCount), 0);
        if (hresult != DD_OK) {
            zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile,
                                   0x5bb);
        }
        return;
    }

    if (queueMode != 0) {
        const int queueIndex = g_zVideo_OverwriteQueueCount;
        if (queueIndex >= 0x180) {
            zError::ReportOld(0x400, kZVideoDirect3DSourceFile, 0x5c3,
                              "Not enough ZVID_MAX_OVERWRITE_POLYS: need %d", queueIndex);
            return;
        }

        zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
        ++g_zVideo_OverwriteQueueCount;
        entry.type = 0;
        entry.vertexCount = vertexCount;
        entry.renderClass = 0;
        entry.renderParam = renderParam;
        if (vertexCount > 0) {
            CopyGouraudVerticesReverse(entry.vertices, vertices, packedColors16, vertexCount,
                                       alpha);
        }
        return;
    }

    const int queueIndex = g_zVideo_SortedPolyQueueCount;
    if (static_cast<unsigned int>(queueIndex) >= 0x100) {
        zError::ReportOld(0x400, kZVideoDirect3DSourceFile, 0x5e2,
                          "Not enough MAX_TRANSPARENT_POLYS: need %d", queueIndex);
        return;
    }

    zVideo_SortedPolyQueueEntry &entry = g_zVideo_SortedPolyQueueBase[queueIndex];
    entry.vertexCount = vertexCount;
    entry.renderClass = 0;
    entry.renderParam = renderParam;
    if (vertexCount > 0) {
        CopyGouraudVerticesReverse(entry.vertices, vertices, packedColors16, vertexCount, alpha);
    }
    ++g_zVideo_SortedPolyQueueCount;
}

// Reimplements 0x4ab320: zVideo_dd3d::SubmitPolyColorAttr
RECOIL_NOINLINE void RECOIL_FASTCALL SubmitPolyColorAttr(
    zVideo_XyzVertex *vertices, unsigned int packedColor16, zVideo_ColorRgbFloat *baseColor,
    float *attr1, float *attr0, float *attr2, int alpha, int vertexCount,
    unsigned int renderParam, int queueMode) {
    (void)packedColor16;

    const float attr1Scale = 1.0f - *attr1;
    const DWORD alphaBits = alpha < 0xff ? static_cast<DWORD>(alpha << 24) : 0xff000000;

    FillColorAttrSpecularReverse(attr2, vertexCount);
    FillColorAttrColorsReverse(*baseColor, attr0, attr1Scale, alphaBits, vertexCount);

    if (alpha < 0xff) {
        return;
    }

    CopyPositionsReverse(g_zVideo_D3DSubmitTempVertices, vertices, vertexCount);

    if (queueMode != 0) {
        const int queueIndex = g_zVideo_OverwriteQueueCount;
        if (queueIndex >= 0x180) {
            zError::ReportOld(0x400, kZVideoDirect3DSourceFile, 0x69c,
                              "Not enough ZVID_MAX_OVERWRITE_POLYS: need %d", queueIndex);
            return;
        }

        zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
        ++g_zVideo_OverwriteQueueCount;
        entry.type = 3;
        entry.vertexCount = vertexCount;
        entry.renderClass = 0;
        entry.renderParam = static_cast<int>(renderParam);
        if (vertexCount > 0) {
            memcpy(entry.vertices, g_zVideo_D3DSubmitTempVertices,
                        static_cast<size_t>(vertexCount) * sizeof(D3DTLVERTEX));
        }
        return;
    }

    if (g_zVideo_D3DRenderState_TextureHandle != 0) {
        g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, 0);
        g_zVideo_D3DRenderState_TextureHandle = 0;
    }
    if (g_zVideo_D3DRenderState_ShadeMode != 1) {
        g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, 1);
        g_zVideo_D3DRenderState_ShadeMode = 1;
    }

    const HRESULT hresult = g_zVideo_pD3DDevice->DrawPrimitive(
        static_cast<D3DPRIMITIVETYPE>(6), static_cast<D3DVERTEXTYPE>(3),
        g_zVideo_D3DSubmitTempVertices, static_cast<DWORD>(vertexCount), 0);
    if (hresult != DD_OK) {
        zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile,
                               0x6ba);
    }
}

// Reimplements 0x4ab6d0: zVideo_dd3d::SubmitPolyRenderClass
RECOIL_NOINLINE void RECOIL_FASTCALL SubmitPolyRenderClass(zVideo_XyzVertex *vertices,
                                                           zVideo_TexCoord *texCoords,
                                                           int vertexCount,
                                                           zVideo_RenderClass *renderClass,
                                                           unsigned int renderParam, float alpha,
                                                           int queueMode) {
    const bool opaquePath =
        renderClass->textureMapBlend != static_cast<D3DTEXTUREBLEND>(4) && alpha >= 1.0f;

    if (opaquePath) {
        CopyTexturedVerticesReverse(g_zVideo_D3DSubmitTempVertices, vertices, texCoords,
                                    vertexCount, 0xffffffff);

        if (queueMode != 0) {
            const int queueIndex = g_zVideo_OverwriteQueueCount;
            if (queueIndex >= 0x180) {
                zError::ReportOld(0x400, kZVideoDirect3DSourceFile, 0x6fd,
                                  "Not enough ZVID_MAX_OVERWRITE_POLYS: need %d", queueIndex);
                return;
            }

            zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
            ++g_zVideo_OverwriteQueueCount;
            entry.type = 4;
            entry.vertexCount = vertexCount;
            entry.renderClass = reinterpret_cast<int>(renderClass);
            entry.renderParam = static_cast<int>(renderParam);
            if (vertexCount > 0) {
                memcpy(entry.vertices, g_zVideo_D3DSubmitTempVertices,
                            static_cast<size_t>(vertexCount) * sizeof(D3DTLVERTEX));
            }
            return;
        }

        if (g_zVideo_D3DRenderState_ShadeMode != 1) {
            g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, 1);
            g_zVideo_D3DRenderState_ShadeMode = 1;
        }
        if (g_zVideo_D3DRenderState_TextureHandle != renderClass->textureHandle) {
            g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE,
                                                renderClass->textureHandle);
            g_zVideo_D3DRenderState_TextureHandle = renderClass->textureHandle;
        }
        if (g_zVideo_D3DRenderState_TextureMapBlend != renderClass->textureMapBlend) {
            g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND,
                                                renderClass->textureMapBlend);
            g_zVideo_D3DRenderState_TextureMapBlend = renderClass->textureMapBlend;
        }
        if (g_zVideo_D3DRenderState_TextureAddressU != renderClass->textureAddressU) {
            g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREADDRESSU,
                                                renderClass->textureAddressU);
            g_zVideo_D3DRenderState_TextureAddressU = renderClass->textureAddressU;
        }
        if (g_zVideo_D3DRenderState_TextureAddressV != renderClass->textureAddressV) {
            g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREADDRESSV,
                                                renderClass->textureAddressV);
            g_zVideo_D3DRenderState_TextureAddressV = renderClass->textureAddressV;
        }

        const HRESULT hresult = g_zVideo_pD3DDevice->DrawPrimitive(
            static_cast<D3DPRIMITIVETYPE>(6), static_cast<D3DVERTEXTYPE>(3),
            g_zVideo_D3DSubmitTempVertices, static_cast<DWORD>(vertexCount), 0);
        if (hresult != DD_OK) {
            zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile,
                                   0x71d);
        }
        return;
    }

    const DWORD alphaWhite = PackAlphaWhite(alpha);
    if (queueMode != 0) {
        const int queueIndex = g_zVideo_OverwriteQueueCount;
        if (queueIndex >= 0x180) {
            zError::ReportOld(0x400, kZVideoDirect3DSourceFile, 0x725,
                              "Not enough ZVID_MAX_OVERWRITE_POLYS: need %d", queueIndex);
            return;
        }

        zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
        ++g_zVideo_OverwriteQueueCount;
        entry.type = 0;
        entry.vertexCount = vertexCount;
        entry.renderClass = reinterpret_cast<int>(renderClass);
        entry.renderParam = static_cast<int>(renderParam);
        if (vertexCount > 0) {
            CopyTexturedVerticesReverse(entry.vertices, vertices, texCoords, vertexCount,
                                        alphaWhite);
        }
        return;
    }

    const int queueIndex = g_zVideo_SortedPolyQueueCount;
    if (static_cast<unsigned int>(queueIndex) >= 0x100) {
        zError::ReportOld(0x400, kZVideoDirect3DSourceFile, 0x74c,
                          "Not enough MAX_TRANSPARENT_POLYS: need %d", queueIndex);
        return;
    }

    zVideo_SortedPolyQueueEntry &entry = g_zVideo_SortedPolyQueueBase[queueIndex];
    entry.vertexCount = vertexCount;
    entry.renderClass = reinterpret_cast<int>(renderClass);
    entry.renderParam = static_cast<int>(renderParam);
    if (vertexCount > 0) {
        CopyTexturedVerticesReverse(entry.vertices, vertices, texCoords, vertexCount, alphaWhite);
    }
    ++g_zVideo_SortedPolyQueueCount;
}

// Reimplements 0x4abb20: zVideo_dd3d::SubmitPolygon
RECOIL_NOINLINE void RECOIL_FASTCALL
SubmitPolygon(zVideo_XyzVertex *vertices, zVideo_TexCoord *uvPairs, float *attr1, float *attr0,
              float *attr2, int vertexCount, zVideo_RenderClass *renderClass,
              unsigned int renderParam, float alpha, int queueMode) {
    const float attr1Scale = 1.0f - *attr1;
    const DWORD alphaBits =
        alpha < 1.0f ? (static_cast<DWORD>(static_cast<int>(alpha * 255.0f)) << 24)
                     : 0xff000000;
    const float grayBase = attr1Scale * 255.0f;

    FillColorAttrSpecularReverse(attr2, vertexCount);
    FillPolygonColorsReverse(attr0, grayBase, alphaBits, vertexCount);

    const bool opaquePath =
        renderClass->textureMapBlend != static_cast<D3DTEXTUREBLEND>(4) && alpha >= 1.0f;

    if (opaquePath) {
        int preparedVertexCount = vertexCount;
        CopyPositionUvReversePreserveColor(g_zVideo_D3DSubmitTempVertices, vertices, uvPairs,
                                           preparedVertexCount);
        AppendFanCloseVertexIfNeeded(g_zVideo_D3DSubmitTempVertices, preparedVertexCount);

        if (queueMode != 0) {
            const int queueIndex = g_zVideo_OverwriteQueueCount;
            if (queueIndex >= 0x180) {
                zError::ReportOld(0x400, kZVideoDirect3DSourceFile, 0x82a,
                                  "Not enough ZVID_MAX_OVERWRITE_POLYS: need %d", queueIndex);
                return;
            }

            zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
            ++g_zVideo_OverwriteQueueCount;
            entry.type = 5;
            entry.vertexCount = preparedVertexCount;
            entry.renderClass = reinterpret_cast<int>(renderClass);
            entry.renderParam = static_cast<int>(renderParam);
            if (preparedVertexCount > 0) {
                memcpy(entry.vertices, g_zVideo_D3DSubmitTempVertices,
                            static_cast<size_t>(preparedVertexCount) * sizeof(D3DTLVERTEX));
            }
            return;
        }

        if (g_zVideo_D3DRenderState_ShadeMode != 2) {
            g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, 2);
            g_zVideo_D3DRenderState_ShadeMode = 2;
        }
        if (g_zVideo_D3DRenderState_TextureHandle != renderClass->textureHandle) {
            g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE,
                                                renderClass->textureHandle);
            g_zVideo_D3DRenderState_TextureHandle = renderClass->textureHandle;
        }
        if (g_zVideo_D3DRenderState_TextureMapBlend != static_cast<D3DTEXTUREBLEND>(2)) {
            g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND, 2);
            g_zVideo_D3DRenderState_TextureMapBlend = static_cast<D3DTEXTUREBLEND>(2);
        }
        if (g_zVideo_D3DRenderState_TextureAddressU != renderClass->textureAddressU) {
            g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREADDRESSU,
                                                renderClass->textureAddressU);
            g_zVideo_D3DRenderState_TextureAddressU = renderClass->textureAddressU;
        }
        if (g_zVideo_D3DRenderState_TextureAddressV != renderClass->textureAddressV) {
            g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREADDRESSV,
                                                renderClass->textureAddressV);
            g_zVideo_D3DRenderState_TextureAddressV = renderClass->textureAddressV;
        }

        const HRESULT hresult = g_zVideo_pD3DDevice->DrawPrimitive(
            static_cast<D3DPRIMITIVETYPE>(6), static_cast<D3DVERTEXTYPE>(3),
            g_zVideo_D3DSubmitTempVertices, static_cast<DWORD>(preparedVertexCount), 0);
        if (hresult != DD_OK) {
            zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile,
                                   0x84a);
        }
        return;
    }

    if (queueMode != 0) {
        const int queueIndex = g_zVideo_OverwriteQueueCount;
        if (queueIndex >= 0x180) {
            zError::ReportOld(0x400, kZVideoDirect3DSourceFile, 0x853,
                              "Not enough ZVID_MAX_OVERWRITE_POLYS: need %d", queueIndex);
            return;
        }

        zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
        ++g_zVideo_OverwriteQueueCount;
        entry.type = 0;
        entry.renderClass = reinterpret_cast<int>(renderClass);
        entry.renderParam = static_cast<int>(renderParam);
        int preparedVertexCount = vertexCount;
        if (vertexCount > 0) {
            CopyPositionUvWithPreparedColorReverse(entry.vertices, vertices, uvPairs,
                                                   g_zVideo_D3DSubmitTempVertices, vertexCount);
        }
        AppendFanCloseVertexIfNeeded(entry.vertices, preparedVertexCount);
        entry.vertexCount = preparedVertexCount;
        return;
    }

    const int queueIndex = g_zVideo_SortedPolyQueueCount;
    if (static_cast<unsigned int>(queueIndex) >= 0x100) {
        zError::ReportOld(0x400, kZVideoDirect3DSourceFile, 0x88a,
                          "Not enough MAX_TRANSPARENT_POLYS: need %d", queueIndex);
        return;
    }

    zVideo_SortedPolyQueueEntry &entry = g_zVideo_SortedPolyQueueBase[queueIndex];
    entry.renderClass = reinterpret_cast<int>(renderClass);
    entry.renderParam = static_cast<int>(renderParam);
    int preparedVertexCount = vertexCount;
    if (vertexCount > 0) {
        CopyPositionUvWithPreparedColorReverse(entry.vertices, vertices, uvPairs,
                                               g_zVideo_D3DSubmitTempVertices, vertexCount);
    }
    AppendFanCloseVertexIfNeeded(entry.vertices, preparedVertexCount);
    entry.vertexCount = preparedVertexCount;
    ++g_zVideo_SortedPolyQueueCount;
}

// Reimplements 0x4ac370: zVideo_dd3d::SubmitPolygonLit
RECOIL_NOINLINE void RECOIL_FASTCALL
SubmitPolygonLit(zVideo_XyzVertex *vertices, zVideo_TexCoord *uvPairs, float *attr1, float *attr0,
                 float *attr2, int vertexCount, zVideo_RenderClass *renderClass,
                 unsigned int renderParam, float alpha, int queueMode) {
    const DWORD alphaBits =
        alpha < 1.0f ? (static_cast<DWORD>(static_cast<int>(alpha * 255.0f)) << 24)
                     : 0xff000000;

    FillColorAttrSpecularReverse(attr2, vertexCount);
    FillPolygonLitColorsReverse(attr1, attr0, alphaBits, vertexCount);

    const bool opaquePath =
        renderClass->textureMapBlend != static_cast<D3DTEXTUREBLEND>(4) && alpha >= 1.0f;

    if (opaquePath) {
        int preparedVertexCount = vertexCount;
        CopyPositionUvReversePreserveColor(g_zVideo_D3DSubmitTempVertices, vertices, uvPairs,
                                           preparedVertexCount);
        AppendFanCloseVertexIfNeeded(g_zVideo_D3DSubmitTempVertices, preparedVertexCount);

        if (queueMode != 0) {
            const int queueIndex = g_zVideo_OverwriteQueueCount;
            if (queueIndex >= 0x180) {
                zError::ReportOld(0x400, kZVideoDirect3DSourceFile, 0x983,
                                  "Not enough ZVID_MAX_OVERWRITE_POLYS: need %d", queueIndex);
                return;
            }

            zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
            ++g_zVideo_OverwriteQueueCount;
            entry.type = 6;
            entry.vertexCount = preparedVertexCount;
            entry.renderClass = reinterpret_cast<int>(renderClass);
            entry.renderParam = static_cast<int>(renderParam);
            if (preparedVertexCount > 0) {
                memcpy(entry.vertices, g_zVideo_D3DSubmitTempVertices,
                            static_cast<size_t>(preparedVertexCount) * sizeof(D3DTLVERTEX));
            }
            return;
        }

        if (g_zVideo_D3DRenderState_ShadeMode != 2) {
            g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, 2);
            g_zVideo_D3DRenderState_ShadeMode = 2;
        }
        if (g_zVideo_D3DRenderState_TextureHandle != renderClass->textureHandle) {
            g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE,
                                                renderClass->textureHandle);
            g_zVideo_D3DRenderState_TextureHandle = renderClass->textureHandle;
        }
        if (g_zVideo_D3DRenderState_TextureMapBlend != static_cast<D3DTEXTUREBLEND>(2)) {
            g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND, 2);
            g_zVideo_D3DRenderState_TextureMapBlend = static_cast<D3DTEXTUREBLEND>(2);
        }
        if (g_zVideo_D3DRenderState_TextureAddressU != renderClass->textureAddressU) {
            g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREADDRESSU,
                                                renderClass->textureAddressU);
            g_zVideo_D3DRenderState_TextureAddressU = renderClass->textureAddressU;
        }
        if (g_zVideo_D3DRenderState_TextureAddressV != renderClass->textureAddressV) {
            g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREADDRESSV,
                                                renderClass->textureAddressV);
            g_zVideo_D3DRenderState_TextureAddressV = renderClass->textureAddressV;
        }

        const HRESULT hresult = g_zVideo_pD3DDevice->DrawPrimitive(
            static_cast<D3DPRIMITIVETYPE>(6), static_cast<D3DVERTEXTYPE>(3),
            g_zVideo_D3DSubmitTempVertices, static_cast<DWORD>(preparedVertexCount), 0);
        if (hresult != DD_OK) {
            zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile,
                                   0x9a4);
        }
        return;
    }

    if (queueMode != 0) {
        const int queueIndex = g_zVideo_OverwriteQueueCount;
        if (queueIndex >= 0x180) {
            zError::ReportOld(0x400, kZVideoDirect3DSourceFile, 0x9ad,
                              "Not enough ZVID_MAX_OVERWRITE_POLYS: need %d", queueIndex);
            return;
        }

        zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
        ++g_zVideo_OverwriteQueueCount;
        entry.type = 0;
        entry.renderClass = reinterpret_cast<int>(renderClass);
        entry.renderParam = static_cast<int>(renderParam);
        int preparedVertexCount = vertexCount;
        if (vertexCount > 0) {
            CopyPositionUvWithPreparedColorReverse(entry.vertices, vertices, uvPairs,
                                                   g_zVideo_D3DSubmitTempVertices, vertexCount);
        }
        AppendFanCloseVertexIfNeeded(entry.vertices, preparedVertexCount);
        entry.vertexCount = preparedVertexCount;
        return;
    }

    const int queueIndex = g_zVideo_SortedPolyQueueCount;
    if (static_cast<unsigned int>(queueIndex) >= 0x100) {
        zError::ReportOld(0x400, kZVideoDirect3DSourceFile, 0x9e4,
                          "Not enough MAX_TRANSPARENT_POLYS: need %d", queueIndex);
        return;
    }

    zVideo_SortedPolyQueueEntry &entry = g_zVideo_SortedPolyQueueBase[queueIndex];
    entry.renderClass = reinterpret_cast<int>(renderClass);
    entry.renderParam = static_cast<int>(renderParam);
    int preparedVertexCount = vertexCount;
    if (vertexCount > 0) {
        CopyPositionUvWithPreparedColorReverse(entry.vertices, vertices, uvPairs,
                                               g_zVideo_D3DSubmitTempVertices, vertexCount);
    }
    AppendFanCloseVertexIfNeeded(entry.vertices, preparedVertexCount);
    entry.vertexCount = preparedVertexCount;
    ++g_zVideo_SortedPolyQueueCount;
}

// Reimplements 0x4acbd0: zVideo_dd3d::DrawPointColor16
RECOIL_NOINLINE void RECOIL_FASTCALL DrawPointColor16(zVideo_XyzVertex *pointPos,
                                                      unsigned int packedColor16,
                                                      int pointCount) {
    (void)pointCount;

    D3DTLVERTEX &vertex = g_zVideo_D3DSubmitTempVertices[0];
    vertex.sx = pointPos->x;
    vertex.sy = pointPos->y;
    vertex.sz = pointPos->z;
    vertex.rhw = pointPos->z;
    vertex.color = PackD3DColorFrom16(packedColor16, 0xff);
    vertex.specular = 0xff000000;

    if (g_zVideo_D3DRenderState_TextureHandle != 0) {
        g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, 0);
        g_zVideo_D3DRenderState_TextureHandle = 0;
    }
    if (g_zVideo_D3DRenderState_ShadeMode != 1) {
        g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, 1);
        g_zVideo_D3DRenderState_ShadeMode = 1;
    }

    const HRESULT hresult = g_zVideo_pD3DDevice->DrawPrimitive(
        static_cast<D3DPRIMITIVETYPE>(1), static_cast<D3DVERTEXTYPE>(3),
        g_zVideo_D3DSubmitTempVertices, 1, 0);
    if (hresult != DD_OK) {
        zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile,
                               0xa4c);
    }
}

// Reimplements 0x4acd00: zVideo_dd3d::QueueSolidQuad
RECOIL_NOINLINE void RECOIL_FASTCALL QueueSolidQuad(unsigned int packedColor16,
                                                    zVidRect32 *clipRect, double alpha) {
    const int batchIndex = g_zVideo_QuadBatchCount;
    if (static_cast<unsigned int>(batchIndex) >= 0x10) {
        return;
    }

    zVideo_QuadBatchItemPartial &item = g_zVideo_QuadBatchItemsBase[batchIndex];

    float left;
    float top;
    float right;
    float bottom;
    if (clipRect != 0) {
        left = static_cast<float>(clipRect->left);
        top = static_cast<float>(clipRect->top);
        right = static_cast<float>(clipRect->right);
        bottom = static_cast<float>(clipRect->bottom);
    } else {
        left = 0.0f;
        top = 0.0f;
        right = static_cast<float>(g_zVideo_PrimarySurfaceState.height);
        bottom = static_cast<float>(g_zVideo_PrimarySurfaceState.width);
    }

    item.vertices[0].sx = left;
    item.vertices[0].sy = top;
    item.vertices[1].sx = right;
    item.vertices[1].sy = top;
    item.vertices[2].sx = right;
    item.vertices[2].sy = bottom;
    item.vertices[3].sx = left;
    item.vertices[3].sy = bottom;

    const int alphaByte = static_cast<int>(alpha * 255.0);
    const DWORD packedColor = PackD3DColorFrom16(packedColor16, alphaByte);
    for (int i = 0; i < 4; ++i) {
        item.vertices[i].color = packedColor;
    }

    ++g_zVideo_QuadBatchCount;
}

// Reimplements 0x4ace30: zVideo_dd3d::FlushSortedPolys
RECOIL_NOINLINE void RECOIL_CDECL FlushSortedPolys() {
    int queueCount = g_zVideo_SortedPolyQueueCount;
    if (queueCount == 0) {
        return;
    }

    if (g_zVideo_D3DRenderState_ShadeMode != 2) {
        g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, 2);
        queueCount = g_zVideo_SortedPolyQueueCount;
        g_zVideo_D3DRenderState_ShadeMode = 2;
    }
    if (g_zVideo_D3DRenderState_AlphaBlendEnable != 1) {
        g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, 1);
        queueCount = g_zVideo_SortedPolyQueueCount;
        g_zVideo_D3DRenderState_AlphaBlendEnable = 1;
    }
    if (g_zVideo_D3DRenderState_ZWriteEnable != 0) {
        g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, 0);
        queueCount = g_zVideo_SortedPolyQueueCount;
        g_zVideo_D3DRenderState_ZWriteEnable = 0;
    }

    for (unsigned int i = 0; i < static_cast<unsigned int>(queueCount); ++i) {
        g_zVideo_SortedPolyDrawOrder[i] = queueCount - static_cast<int>(i) - 1;
        queueCount = g_zVideo_SortedPolyQueueCount;
    }

    bool swapped;
    do {
        swapped = false;
        for (unsigned int i = 1; i < static_cast<unsigned int>(queueCount); ++i) {
            const int currentIndex = g_zVideo_SortedPolyDrawOrder[i];
            const int previousIndex = g_zVideo_SortedPolyDrawOrder[i - 1];
            if (g_zVideo_SortedPolyQueueBase[currentIndex].vertices[0].sz <
                g_zVideo_SortedPolyQueueBase[previousIndex].vertices[0].sz) {
                g_zVideo_SortedPolyDrawOrder[i - 1] = currentIndex;
                g_zVideo_SortedPolyDrawOrder[i] = previousIndex;
                queueCount = g_zVideo_SortedPolyQueueCount;
                swapped = true;
            }
        }
    } while (swapped);

    for (unsigned int i_4102 = 0; i_4102 < static_cast<unsigned int>(g_zVideo_SortedPolyQueueCount); ++i_4102) {
        const int drawIndex = g_zVideo_SortedPolyDrawOrder[i_4102];
        zVideo_SortedPolyQueueEntry &entry = g_zVideo_SortedPolyQueueBase[drawIndex];
        zVideo_RenderClass *renderClass = reinterpret_cast<zVideo_RenderClass *>(entry.renderClass);

        if (renderClass != 0) {
            if (g_zVideo_D3DRenderState_TextureHandle != renderClass->textureHandle) {
                g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE,
                                                    renderClass->textureHandle);
                g_zVideo_D3DRenderState_TextureHandle = renderClass->textureHandle;
            }

            const D3DTEXTUREBLEND textureMapBlend = renderClass->textureMapBlend;
            const bool forceTransparentTextureBlend =
                textureMapBlend != static_cast<D3DTEXTUREBLEND>(4) &&
                (entry.vertices[0].color & 0xff000000) != 0xff000000;
            if (forceTransparentTextureBlend) {
                if (g_zVideo_D3DRenderState_TextureMapBlend != static_cast<D3DTEXTUREBLEND>(4)) {
                    g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND, 4);
                    g_zVideo_D3DRenderState_TextureMapBlend = static_cast<D3DTEXTUREBLEND>(4);
                }
            } else if (g_zVideo_D3DRenderState_TextureMapBlend != textureMapBlend) {
                g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND,
                                                    textureMapBlend);
                g_zVideo_D3DRenderState_TextureMapBlend = textureMapBlend;
            }

            if (g_zVideo_D3DRenderState_TextureAddressU != renderClass->textureAddressU) {
                g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREADDRESSU,
                                                    renderClass->textureAddressU);
                g_zVideo_D3DRenderState_TextureAddressU = renderClass->textureAddressU;
            }
            if (g_zVideo_D3DRenderState_TextureAddressV != renderClass->textureAddressV) {
                g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREADDRESSV,
                                                    renderClass->textureAddressV);
                g_zVideo_D3DRenderState_TextureAddressV = renderClass->textureAddressV;
            }
        } else if (g_zVideo_D3DRenderState_TextureHandle != 0) {
            g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, 0);
            g_zVideo_D3DRenderState_TextureHandle = 0;
        }

        const HRESULT hresult = g_zVideo_pD3DDevice->DrawPrimitive(
            D3DPT_TRIANGLEFAN, static_cast<D3DVERTEXTYPE>(3), entry.vertices,
            static_cast<DWORD>(entry.vertexCount), 0);
        if (hresult != DD_OK) {
            zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile,
                                   0xb09);
        }
    }

    if (g_zVideo_D3DRenderState_AlphaBlendEnable != 0) {
        g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, 0);
        g_zVideo_D3DRenderState_AlphaBlendEnable = 0;
    }
    if (g_zVideo_D3DRenderState_ZWriteEnable != 1) {
        g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, 1);
        g_zVideo_D3DRenderState_ZWriteEnable = 1;
    }
    g_zVideo_SortedPolyQueueCount = 0;
}

// Reimplements 0x4ad120: zVideo_dd3d::FlushQuadBatch
RECOIL_NOINLINE void RECOIL_CDECL FlushQuadBatch() {
    if (g_zVideo_QuadBatchCount == 0) {
        return;
    }

    if (g_zVideo_D3DRenderState_ShadeMode != 2) {
        g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, 2);
        g_zVideo_D3DRenderState_ShadeMode = 2;
    }
    if (g_zVideo_D3DRenderState_AlphaBlendEnable != 1) {
        g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, 1);
        g_zVideo_D3DRenderState_AlphaBlendEnable = 1;
    }
    if (g_zVideo_D3DRenderState_ZWriteEnable != 0) {
        g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, 0);
        g_zVideo_D3DRenderState_ZWriteEnable = 0;
    }
    if (g_zVideo_D3DRenderState_TextureHandle != 0) {
        g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, 0);
        g_zVideo_D3DRenderState_TextureHandle = 0;
    }

    g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_ALWAYS);

    for (unsigned int i = 0; i < static_cast<unsigned int>(g_zVideo_QuadBatchCount); ++i) {
        g_zVideo_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, static_cast<D3DVERTEXTYPE>(3),
                                           g_zVideo_QuadBatchItemsBase[i].vertices, 4, 0);
    }

    g_zVideo_QuadBatchCount = 0;
    g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_GREATEREQUAL);

    if (g_zVideo_D3DRenderState_AlphaBlendEnable != 0) {
        g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, 0);
        g_zVideo_D3DRenderState_AlphaBlendEnable = 0;
    }
    if (g_zVideo_D3DRenderState_ZWriteEnable != 1) {
        g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, 1);
        g_zVideo_D3DRenderState_ZWriteEnable = 1;
    }
}

// Reimplements 0x4ad250: zVideo_dd3d::FlushOverwritePolys
RECOIL_NOINLINE void RECOIL_CDECL FlushOverwritePolys() {
    g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_ALWAYS);

    for (int i = 0; i < g_zVideo_OverwriteQueueCount; ++i) {
        zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[i];
        HRESULT hresult = DD_OK;

        switch (entry.type) {
        case 0: {
            if (g_zVideo_D3DRenderState_ShadeMode != 2) {
                g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, 2);
                g_zVideo_D3DRenderState_ShadeMode = 2;
            }
            if (g_zVideo_D3DRenderState_AlphaBlendEnable != 1) {
                g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, 1);
                g_zVideo_D3DRenderState_AlphaBlendEnable = 1;
            }
            if (g_zVideo_D3DRenderState_ZWriteEnable != 0) {
                g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, 0);
                g_zVideo_D3DRenderState_ZWriteEnable = 0;
            }

            zVideo_RenderClass *renderClass =
                reinterpret_cast<zVideo_RenderClass *>(entry.renderClass);
            if (renderClass != 0) {
                if (g_zVideo_D3DRenderState_TextureHandle != renderClass->textureHandle) {
                    g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE,
                                                        renderClass->textureHandle);
                    g_zVideo_D3DRenderState_TextureHandle = renderClass->textureHandle;
                }

                const D3DTEXTUREBLEND textureMapBlend = renderClass->textureMapBlend;
                const bool forceTransparentTextureBlend =
                    textureMapBlend != static_cast<D3DTEXTUREBLEND>(4) &&
                    (entry.vertices[0].color & 0xff000000) != 0xff000000;
                if (forceTransparentTextureBlend) {
                    if (g_zVideo_D3DRenderState_TextureMapBlend !=
                        static_cast<D3DTEXTUREBLEND>(4)) {
                        g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND, 4);
                        g_zVideo_D3DRenderState_TextureMapBlend = static_cast<D3DTEXTUREBLEND>(4);
                    }
                } else if (g_zVideo_D3DRenderState_TextureMapBlend != textureMapBlend) {
                    g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND,
                                                        textureMapBlend);
                    g_zVideo_D3DRenderState_TextureMapBlend = textureMapBlend;
                }

                if (g_zVideo_D3DRenderState_TextureAddressU != renderClass->textureAddressU) {
                    g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREADDRESSU,
                                                        renderClass->textureAddressU);
                    g_zVideo_D3DRenderState_TextureAddressU = renderClass->textureAddressU;
                }
                if (g_zVideo_D3DRenderState_TextureAddressV != renderClass->textureAddressV) {
                    g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREADDRESSV,
                                                        renderClass->textureAddressV);
                    g_zVideo_D3DRenderState_TextureAddressV = renderClass->textureAddressV;
                }
            } else if (g_zVideo_D3DRenderState_TextureHandle != 0) {
                g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, 0);
                g_zVideo_D3DRenderState_TextureHandle = 0;
            }

            hresult = g_zVideo_pD3DDevice->DrawPrimitive(
                D3DPT_TRIANGLEFAN, static_cast<D3DVERTEXTYPE>(3), entry.vertices,
                static_cast<DWORD>(entry.vertexCount), 0);

            if (g_zVideo_D3DRenderState_AlphaBlendEnable != 0) {
                g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, 0);
                g_zVideo_D3DRenderState_AlphaBlendEnable = 0;
            }
            if (g_zVideo_D3DRenderState_ZWriteEnable != 1) {
                g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, 1);
                g_zVideo_D3DRenderState_ZWriteEnable = 1;
            }
            break;
        }

        case 1:
        case 2:
        case 3:
            if (g_zVideo_D3DRenderState_TextureHandle != 0) {
                g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, 0);
                g_zVideo_D3DRenderState_TextureHandle = 0;
            }
            if (g_zVideo_D3DRenderState_ShadeMode != 1) {
                g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, 1);
                g_zVideo_D3DRenderState_ShadeMode = 1;
            }
            hresult = g_zVideo_pD3DDevice->DrawPrimitive(
                D3DPT_TRIANGLEFAN, static_cast<D3DVERTEXTYPE>(3), entry.vertices,
                static_cast<DWORD>(entry.vertexCount), 0);
            break;

        case 4: {
            if (g_zVideo_D3DRenderState_ShadeMode != 1) {
                g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, 1);
                g_zVideo_D3DRenderState_ShadeMode = 1;
            }

            zVideo_RenderClass *renderClass =
                reinterpret_cast<zVideo_RenderClass *>(entry.renderClass);
            if (g_zVideo_D3DRenderState_TextureHandle != renderClass->textureHandle) {
                g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE,
                                                    renderClass->textureHandle);
                g_zVideo_D3DRenderState_TextureHandle = renderClass->textureHandle;
            }
            if (g_zVideo_D3DRenderState_TextureMapBlend != renderClass->textureMapBlend) {
                g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND,
                                                    renderClass->textureMapBlend);
                g_zVideo_D3DRenderState_TextureMapBlend = renderClass->textureMapBlend;
            }
            if (g_zVideo_D3DRenderState_TextureAddressU != renderClass->textureAddressU) {
                g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREADDRESSU,
                                                    renderClass->textureAddressU);
                g_zVideo_D3DRenderState_TextureAddressU = renderClass->textureAddressU;
            }
            if (g_zVideo_D3DRenderState_TextureAddressV != renderClass->textureAddressV) {
                g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREADDRESSV,
                                                    renderClass->textureAddressV);
                g_zVideo_D3DRenderState_TextureAddressV = renderClass->textureAddressV;
            }

            hresult = g_zVideo_pD3DDevice->DrawPrimitive(
                D3DPT_TRIANGLEFAN, static_cast<D3DVERTEXTYPE>(3), entry.vertices,
                static_cast<DWORD>(entry.vertexCount), 0);
            break;
        }

        case 5:
        case 6: {
            if (g_zVideo_D3DRenderState_ShadeMode != 2) {
                g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, 2);
                g_zVideo_D3DRenderState_ShadeMode = 2;
            }

            zVideo_RenderClass *renderClass =
                reinterpret_cast<zVideo_RenderClass *>(entry.renderClass);
            if (g_zVideo_D3DRenderState_TextureHandle != renderClass->textureHandle) {
                g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE,
                                                    renderClass->textureHandle);
                g_zVideo_D3DRenderState_TextureHandle = renderClass->textureHandle;
            }
            if (g_zVideo_D3DRenderState_TextureMapBlend != static_cast<D3DTEXTUREBLEND>(2)) {
                g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND, 2);
                g_zVideo_D3DRenderState_TextureMapBlend = static_cast<D3DTEXTUREBLEND>(2);
            }
            if (g_zVideo_D3DRenderState_TextureAddressU != renderClass->textureAddressU) {
                g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREADDRESSU,
                                                    renderClass->textureAddressU);
                g_zVideo_D3DRenderState_TextureAddressU = renderClass->textureAddressU;
            }
            if (g_zVideo_D3DRenderState_TextureAddressV != renderClass->textureAddressV) {
                g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREADDRESSV,
                                                    renderClass->textureAddressV);
                g_zVideo_D3DRenderState_TextureAddressV = renderClass->textureAddressV;
            }

            hresult = g_zVideo_pD3DDevice->DrawPrimitive(
                D3DPT_TRIANGLEFAN, static_cast<D3DVERTEXTYPE>(3), entry.vertices,
                static_cast<DWORD>(entry.vertexCount), 0);
            break;
        }

        default:
            break;
        }

        if (hresult != DD_OK) {
            zVideo_dd::ReportError(static_cast<int>(hresult), kZVideoDirect3DSourceFile,
                                   0xbb7);
        }
    }

    g_zVideo_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_GREATEREQUAL);
    g_zVideo_OverwriteQueueCount = 0;
}

// Reimplements 0x4ad680: zVideo_dd3d::FloorPowerOfTwo
RECOIL_NOINLINE int RECOIL_FASTCALL FloorPowerOfTwo(int value) {
    int powerOfTwo = 1;
    do {
        powerOfTwo <<= 1;
    } while (powerOfTwo < value);

    if (powerOfTwo == value) {
        return value;
    }

    return powerOfTwo >> 1;
}

// Reimplements 0x4aa9d0: zVideo_dd3d::TextureRecord_Create
RECOIL_NOINLINE zVideo_TextureRecordPartial *RECOIL_CDECL TextureRecord_Create() {
    return static_cast<zVideo_TextureRecordPartial *>(
        calloc(1, sizeof(zVideo_TextureRecordPartial)));
}

// Reimplements 0x4aa8b0: zVideo_dd3d::TextureRecord_LockUploadSurface
RECOIL_NOINLINE int RECOIL_FASTCALL TextureRecord_LockUploadSurface(
    zVideo_TextureRecordPartial *textureRecord, void **outPixels, int *outPitchBytes) {
    DDSURFACEDESC lockedDescOut = {0};
    if (zVideo_dd::LockSurface_WaitRestore(
            reinterpret_cast<IDirectDrawSurface3 *>(textureRecord->m_uploadSurface),
            &lockedDescOut) != 0) {
        return 0;
    }

    *outPitchBytes = lockedDescOut.lPitch;
    *outPixels = lockedDescOut.lpSurface;
    return 1;
}

// Reimplements 0x4aa6f0: zVideo_dd3d::ConvertImagePixelsForTexture
RECOIL_NOINLINE void RECOIL_FASTCALL ConvertImagePixelsForTexture(unsigned short *dstPixels,
                                                                  zVidImagePartial *image,
                                                                  int pitchBytes,
                                                                  int useAlpha) {
    (void)useAlpha;

    const int width = image->width;
    const int height = image->height;
    unsigned short *srcPixels = static_cast<unsigned short *>(image->pixels);
    unsigned char *dstRowBytes = reinterpret_cast<unsigned char *>(dstPixels);

    if (image->alphaMap == 0) {
        const unsigned int redGreenMask = g_zVideo_PixelPack_RMask | g_zVideo_PixelPack_GMask;
        {
        for (int row = 0; row < height; ++row) {
            unsigned short *dstCursor = reinterpret_cast<unsigned short *>(dstRowBytes);
            {
            for (int column = 0; column < width; ++column) {
                const unsigned short src = *srcPixels++;
                const unsigned short alphaBit = src != 0 ? 0x8000 : 0;
                *dstCursor++ =
                    static_cast<unsigned short>((src & g_zVideo_PixelPack_BMask) |
                                               ((src >> 1) & (redGreenMask >> 1)) | alphaBit);
            }
            }
            dstRowBytes += pitchBytes;
        }
        }
        return;
    }

    unsigned char *alphaCursor = reinterpret_cast<unsigned char *>(image->alphaMap);
    unsigned int redAlphaMask;
    int redAlphaShift;
    unsigned int greenAlphaMask;
    int greenAlphaShift;
    if (g_zVideo_PixelPack_GBits == 6) {
        redAlphaMask = 0xf000;
        redAlphaShift = 4;
        greenAlphaMask = 0x780;
        greenAlphaShift = 3;
    } else {
        redAlphaMask = 0x7800;
        redAlphaShift = 3;
        greenAlphaMask = 0x3c0;
        greenAlphaShift = 2;
    }

    {
    for (int row = 0; row < height; ++row) {
        unsigned short *dstCursor = reinterpret_cast<unsigned short *>(dstRowBytes);
        {
        for (int column = 0; column < width; ++column) {
            const unsigned short src = *srcPixels++;
            const unsigned int alpha = (*alphaCursor++ & 0xf0) << 8;
            *dstCursor++ =
                static_cast<unsigned short>(((src >> 1) & (g_zVideo_PixelPack_BMask >> 1)) |
                                           ((greenAlphaMask & src) >> greenAlphaShift) |
                                           ((redAlphaMask & src) >> redAlphaShift) | alpha);
        }
        }
        dstRowBytes += pitchBytes;
    }
    }
}

// Reimplements 0x4aa600: zVideo_dd3d::UploadImageToSurface
RECOIL_NOINLINE int RECOIL_FASTCALL UploadImageToSurface(IDirectDrawSurface *uploadSurface,
                                                                  zVidImagePartial *image,
                                                                  int useAlpha) {
    DDSURFACEDESC lockedDescOut = {0};
    zVideo_dd::LockSurface_WaitRestore(reinterpret_cast<IDirectDrawSurface3 *>(uploadSurface),
                                       &lockedDescOut);

    unsigned char *dstPixels = static_cast<unsigned char *>(lockedDescOut.lpSurface);
    unsigned char *srcPixels = static_cast<unsigned char *>(image->pixels);
    if (useAlpha != 0) {
        ConvertImagePixelsForTexture(reinterpret_cast<unsigned short *>(dstPixels), image,
                                     lockedDescOut.lPitch, useAlpha);
    } else {
        const int width = image->width;
        const int height = image->height;
        if (lockedDescOut.lPitch == width) {
            const int bytesPerPixel = (g_zVideo_DisplayModeBpp + 7) >> 3;
            memcpy(dstPixels, srcPixels,
                        static_cast<size_t>(height * bytesPerPixel * width));
        } else {
            const int rowCopyBytes = (g_zVideo_DisplayModeBpp * width + 7) >> 3;
            {
            for (int row = 0; row < height; ++row) {
                memcpy(dstPixels, srcPixels, static_cast<size_t>(rowCopyBytes));
                dstPixels += lockedDescOut.lPitch;
                srcPixels += width << 1;
            }
            }
        }
    }

    zVideo_dd::UnlockSurface_WaitRestore(reinterpret_cast<IDirectDrawSurface3 *>(uploadSurface));
    return 1;
}

// Reimplements 0x4aa8f0: zVideo_dd3d::TextureRecord_UnlockUploadSurface
RECOIL_NOINLINE int RECOIL_FASTCALL
TextureRecord_UnlockUploadSurface(zVideo_TextureRecordPartial *textureRecord) {
    return zVideo_dd::UnlockSurface_WaitRestore(
               reinterpret_cast<IDirectDrawSurface3 *>(textureRecord->m_uploadSurface)) == 0
               ? 1
               : 0;
}

// Reimplements 0x4aa900: zVideo_dd3d::TextureRecord_ReleaseUploadSurfaceRef
RECOIL_NOINLINE void RECOIL_FASTCALL
TextureRecord_ReleaseUploadSurfaceRef(zVideo_TextureRecordPartial *textureRecord) {
    if (textureRecord->m_uploadSurface != 0) {
        textureRecord->m_uploadSurface->Release();
        textureRecord->m_uploadSurface = 0;
    }
}

// Reimplements 0x4aa920: zVideo_dd3d::TextureRecord_FinalizeUpload
RECOIL_NOINLINE void RECOIL_FASTCALL
TextureRecord_FinalizeUpload(zVideo_TextureRecordPartial *textureRecord, zVidImagePartial *image) {
    IDirectDrawSurface *uploadSurface = textureRecord->m_uploadSurface;
    if (uploadSurface == 0) {
        return;
    }

    IDirect3DTexture2 *targetTexture = textureRecord->m_texture;
    if (image != 0) {
        UploadImageToSurface(uploadSurface, image, image->formatFlagsPacked & 2);
    }

    IDirect3DTexture2 *uploadTexture = 0;
    HRESULT hresult = uploadSurface->QueryInterface(IID_IDirect3DTexture2,
                                                    reinterpret_cast<void **>(&uploadTexture));
    if (hresult != DD_OK) {
        return;
    }

    hresult = targetTexture->Load(uploadTexture);
    if (hresult == DD_OK) {
        uploadTexture->Release();
    }
}

// Reimplements 0x4aa980: zVideo_dd3d::TextureRecord_Destroy
RECOIL_NOINLINE void RECOIL_FASTCALL
TextureRecord_Destroy(zVideo_TextureRecordPartial *textureRecord) {
    if (textureRecord == g_zImage_DefaultTextureRecord) {
        return;
    }

    if (textureRecord->m_uploadSurface != 0) {
        textureRecord->m_uploadSurface->Release();
    }
    if (textureRecord->m_textureSurface != 0) {
        textureRecord->m_textureSurface->Release();
    }
    if (textureRecord->m_texture != 0) {
        textureRecord->m_texture->Release();
    }

    free(textureRecord);
}
} // namespace zVideo_dd3d

namespace zVideo_dd {
// Reimplements 0x4a9900: zVideo_dd::GetAcceptedDirectDrawDeviceCountCached
RECOIL_NOINLINE int RECOIL_CDECL GetAcceptedDirectDrawDeviceCountCached() {
    return g_zVideo_NumAcceptedDirectDrawDevices;
}

namespace {
const char *kZVideoDirectDrawSourceFile = "D:\\Proj\\GameZRecoil\\zVideo\\zvid_dd.c";
const size_t kD3DDescCopyBytes = 0xfc;
const size_t kD3DDescFlagsOffset = 0x04;
const size_t kD3DDescColorModelOffset = 0x08;
const size_t kD3DDescZBufferBitDepthOffset = 0xa0;
const size_t kD3DDescMaxTextureWidthOffset = 0xb4;
const size_t kD3DDescMaxTextureHeightOffset = 0xb8;

unsigned int D3DDescReadDword(const D3DDEVICEDESC *desc, size_t offset) {
    return *reinterpret_cast<const unsigned int *>(reinterpret_cast<const unsigned char *>(desc) +
                                                    offset);
}

void D3DDriverDescWriteDword(zVidD3DDriverRecordPartial &driver, size_t offset,
                             unsigned int value) {
    *reinterpret_cast<unsigned int *>(&driver.m_hwDesc[offset]) = value;
}

template <typename InterfaceT> void ReleaseComInterface(InterfaceT *&value) {
    if (value != 0) {
        value->Release();
        value = 0;
    }
}

bool PageUnlockBeforeRelease(zVideo_SurfaceStatePartial &state, int reportLine) {
    if (state.surf != 0 && state.pageLockActive != 0) {
        const HRESULT hresult = state.surf->PageUnlock(0);
        if (hresult != DD_OK) {
            ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile,
                        reportLine);
            return false;
        }

        state.pageLockActive = 0;
    }

    return true;
}

bool BltFillWithRestore(IDirectDrawSurface3 *surface, zVidRect32 *rect, DWORD flags, DDBLTFX *bltFx,
                        int reportLine) {
    for (;;) {
        HRESULT hresult =
            surface->Blt(reinterpret_cast<RECT *>(rect), 0, 0, flags, bltFx);
        if (hresult == DD_OK) {
            return true;
        }

        if (hresult == DDERR_SURFACELOST) {
            hresult = surface->Restore();
            if (hresult == DD_OK) {
                continue;
            }
        }

        ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, reportLine);
        return false;
    }
}

struct DirectDrawErrorName {
    int hresult;
    const char *name;
};

const DirectDrawErrorName kDirectDrawErrorNames[] = {
    {DDERR_GENERIC, "DDERR_GENERIC"},
    {DDERR_UNSUPPORTED, "DDERR_UNSUPPORTED"},
    {DDERR_OUTOFMEMORY, "DDERR_OUTOFMEMORY"},
    {DDERR_NOTINITIALIZED, "DDERR_NOTINITIALIZED"},
    {DDERR_INVALIDPARAMS, "DDERR_INVALIDPARAMS"},
    {DDERR_ALREADYINITIALIZED, "DDERR_ALREADYINITIALIZED"},
    {DDERR_CANNOTATTACHSURFACE, "DDERR_CANNOTATTACHSURFACE"},
    {DDERR_CANNOTDETACHSURFACE, "DDERR_CANNOTDETACHSURFACE"},
    {DDERR_CURRENTLYNOTAVAIL, "DDERR_CURRENTLYNOTAVAIL"},
    {DDERR_EXCEPTION, "DDERR_EXCEPTION"},
    {DDERR_HEIGHTALIGN, "DDERR_HEIGHTALIGN"},
    {DDERR_INVALIDCAPS, "DDERR_INVALIDCAPS"},
    {DDERR_INVALIDCLIPLIST, "DDERR_INVALIDCLIPLIST"},
    {DDERR_INVALIDMODE, "DDERR_INVALIDMODE"},
    {DDERR_INVALIDOBJECT, "DDERR_INVALIDOBJECT"},
    {DDERR_INVALIDPIXELFORMAT, "DDERR_INVALIDPIXELFORMAT"},
    {DDERR_INVALIDRECT, "DDERR_INVALIDRECT"},
    {DDERR_LOCKEDSURFACES, "DDERR_LOCKEDSURFACES"},
    {DDERR_NO3D, "DDERR_NO3D"},
    {DDERR_NOALPHAHW, "DDERR_NOALPHAHW"},
    {DDERR_NOCLIPLIST, "DDERR_NOCLIPLIST"},
    {DDERR_NOCOLORCONVHW, "DDERR_NOCOLORCONVHW"},
    {DDERR_NOCOOPERATIVELEVELSET, "DDERR_NOCOOPERATIVELEVELSET"},
    {DDERR_NOCOLORKEY, "DDERR_NOCOLORKEY"},
    {DDERR_NOCOLORKEYHW, "DDERR_NOCOLORKEYHW"},
    {DDERR_NODIRECTDRAWSUPPORT, "DDERR_NODIRECTDRAWSUPPORT"},
    {DDERR_NOEXCLUSIVEMODE, "DDERR_NOEXCLUSIVEMODE"},
    {DDERR_NOFLIPHW, "DDERR_NOFLIPHW"},
    {DDERR_NOGDI, "DDERR_NOGDI"},
    {DDERR_NOMIRRORHW, "DDERR_NOMIRRORHW"},
    {DDERR_NOTFOUND, "DDERR_NOTFOUND"},
    {DDERR_NOOVERLAYHW, "DDERR_NOOVERLAYHW"},
    {DDERR_NORASTEROPHW, "DDERR_NORASTEROPHW"},
    {DDERR_NOROTATIONHW, "DDERR_NOROTATIONHW"},
    {DDERR_NOSTRETCHHW, "DDERR_NOSTRETCHHW"},
    {DDERR_NOT4BITCOLOR, "DDERR_NOT4BITCOLOR"},
    {DDERR_NOT4BITCOLORINDEX, "DDERR_NOT4BITCOLORINDEX"},
    {DDERR_NOT8BITCOLOR, "DDERR_NOT8BITCOLOR"},
    {DDERR_NOTEXTUREHW, "DDERR_NOTEXTUREHW"},
    {DDERR_NOVSYNCHW, "DDERR_NOVSYNCHW"},
    {DDERR_NOZBUFFERHW, "DDERR_NOZBUFFERHW"},
    {DDERR_NOZOVERLAYHW, "DDERR_NOZOVERLAYHW"},
    {DDERR_OUTOFCAPS, "DDERR_OUTOFCAPS"},
    {DDERR_OUTOFVIDEOMEMORY, "DDERR_OUTOFVIDEOMEMORY"},
    {DDERR_OVERLAYCANTCLIP, "DDERR_OVERLAYCANTCLIP"},
    {DDERR_OVERLAYCOLORKEYONLYONEACTIVE, "DDERR_OVERLAYCOLORKEYONLYONEACTIVE"},
    {DDERR_PALETTEBUSY, "DDERR_PALETTEBUSY"},
    {DDERR_COLORKEYNOTSET, "DDERR_COLORKEYNOTSET"},
    {DDERR_SURFACEALREADYATTACHED, "DDERR_SURFACEALREADYATTACHED"},
    {DDERR_SURFACEALREADYDEPENDENT, "DDERR_SURFACEALREADYDEPENDENT"},
    {DDERR_SURFACEBUSY, "DDERR_SURFACEBUSY"},
    {DDERR_CANTLOCKSURFACE, "DDERR_CANTLOCKSURFACE"},
    {DDERR_SURFACEISOBSCURED, "DDERR_SURFACEISOBSCURED"},
    {DDERR_SURFACELOST, "DDERR_SURFACELOST"},
    {DDERR_SURFACENOTATTACHED, "DDERR_SURFACENOTATTACHED"},
    {DDERR_TOOBIGHEIGHT, "DDERR_TOOBIGHEIGHT"},
    {DDERR_TOOBIGSIZE, "DDERR_TOOBIGSIZE"},
    {DDERR_TOOBIGWIDTH, "DDERR_TOOBIGWIDTH"},
    {DDERR_UNSUPPORTEDFORMAT, "DDERR_UNSUPPORTEDFORMAT"},
    {DDERR_UNSUPPORTEDMASK, "DDERR_UNSUPPORTEDMASK"},
    {DDERR_VERTICALBLANKINPROGRESS, "DDERR_VERTICALBLANKINPROGRESS"},
    {DDERR_WASSTILLDRAWING, "DDERR_WASSTILLDRAWING"},
    {DDERR_CANTPAGELOCK, "DDERR_CANTPAGELOCK"},
    {DDERR_CANTPAGEUNLOCK, "DDERR_CANTPAGEUNLOCK"},
    {DDERR_NOTPAGELOCKED, "DDERR_NOTPAGELOCKED"},
    {DDERR_XALIGN, "DDERR_XALIGN"},
    {DDERR_INVALIDDIRECTDRAWGUID, "DDERR_INVALIDDIRECTDRAWGUID"},
    {DDERR_DIRECTDRAWALREADYCREATED, "DDERR_DIRECTDRAWALREADYCREATED"},
    {DDERR_NODIRECTDRAWHW, "DDERR_NODIRECTDRAWHW"},
    {DDERR_PRIMARYSURFACEALREADYEXISTS, "DDERR_PRIMARYSURFACEALREADYEXISTS"},
    {DDERR_NOEMULATION, "DDERR_NOEMULATION"},
    {DDERR_REGIONTOOSMALL, "DDERR_REGIONTOOSMALL"},
    {DDERR_CLIPPERISUSINGHWND, "DDERR_CLIPPERISUSINGHWND"},
    {DDERR_NOCLIPPERATTACHED, "DDERR_NOCLIPPERATTACHED"},
    {DDERR_NOHWND, "DDERR_NOHWND"},
    {DDERR_HWNDSUBCLASSED, "DDERR_HWNDSUBCLASSED"},
    {DDERR_HWNDALREADYSET, "DDERR_HWNDALREADYSET"},
    {DDERR_NOPALETTEATTACHED, "DDERR_NOPALETTEATTACHED"},
    {DDERR_NOPALETTEHW, "DDERR_NOPALETTEHW"},
    {DDERR_BLTFASTCANTCLIP, "DDERR_BLTFASTCANTCLIP"},
    {DDERR_NOBLTHW, "DDERR_NOBLTHW"},
    {DDERR_NODDROPSHW, "DDERR_NODDROPSHW"},
    {DDERR_OVERLAYNOTVISIBLE, "DDERR_OVERLAYNOTVISIBLE"},
    {DDERR_INVALIDPOSITION, "DDERR_INVALIDPOSITION"},
    {DDERR_NOTAOVERLAYSURFACE, "DDERR_NOAOVERLAYSURFACE"},
    {DDERR_EXCLUSIVEMODEALREADYSET, "DDERR_EXCLUSIVEMODEALREADYSET"},
    {DDERR_NOTFLIPPABLE, "DDERR_NOTFLIPPABLE"},
    {DDERR_CANTDUPLICATE, "DDERR_CANTDUPLICATE"},
    {DDERR_NOTLOCKED, "DDERR_NOTLOCKED"},
    {DDERR_CANTCREATEDC, "DDERR_CANTCREATEDC"},
    {DDERR_NODC, "DDERR_NODIRECTDC"},
    {DDERR_WRONGMODE, "DDERR_WRONGMODE"},
    {DDERR_IMPLICITLYCREATED, "DDERR_IMPLICITLYCREATED"},
    {DDERR_NOTPALETTIZED, "DDERR_NOTPALETTIZED"},
    {DDERR_UNSUPPORTEDMODE, "DDERR_UNSUPPORTEDMODE"},
    {DDERR_NOMIPMAPHW, "DDERR_NOMIPMAPHW"},
    {DDERR_INVALIDSURFACETYPE, "DDERR_INVALIDSURFACETYPE"},
    {DDERR_DCALREADYCREATED, "DDERR_DCALREADYCREATED"},
    {D3DERR_BADMAJORVERSION, "D3DERR_BADMAJORVERSION"},
    {D3DERR_BADMINORVERSION, "D3DERR_BADMINORVERSION"},
    {D3DERR_INVALID_DEVICE, "D3DERR_INVALID_DEVICE"},
    {D3DERR_EXECUTE_CREATE_FAILED, "D3DERR_EXECUTE_CREATE_FAILED"},
    {D3DERR_EXECUTE_DESTROY_FAILED, "D3DERR_EXECUTE_DESTROY_FAILED"},
    {D3DERR_EXECUTE_LOCK_FAILED, "D3DERR_EXECUTE_LOCK_FAILED"},
    {D3DERR_EXECUTE_UNLOCK_FAILED, "D3DERR_EXECUTE_UNLOCK_FAILED"},
    {D3DERR_EXECUTE_LOCKED, "D3DERR_EXECUTE_LOCKED"},
    {D3DERR_EXECUTE_NOT_LOCKED, "D3DERR_EXECUTE_NOT_LOCKED"},
    {D3DERR_EXECUTE_FAILED, "D3DERR_EXECUTE_FAILED"},
    {D3DERR_EXECUTE_CLIPPED_FAILED, "D3DERR_EXECUTE_CLIPPED_FAILED"},
    {D3DERR_TEXTURE_NO_SUPPORT, "D3DERR_TEXTURE_NO_SUPPORT"},
    {D3DERR_TEXTURE_CREATE_FAILED, "D3DERR_TEXTURE_CREATE_FAILED"},
    {D3DERR_TEXTURE_DESTROY_FAILED, "D3DERR_TEXTURE_DESTROY_FAILED"},
    {D3DERR_TEXTURE_LOCK_FAILED, "D3DERR_TEXTURE_LOCK_FAILED"},
    {D3DERR_TEXTURE_UNLOCK_FAILED, "D3DERR_TEXTURE_UNLOCK_FAILED"},
    {D3DERR_TEXTURE_LOAD_FAILED, "D3DERR_TEXTURE_LOAD_FAILED"},
    {D3DERR_TEXTURE_SWAP_FAILED, "D3DERR_TEXTURE_SWAP_FAILED"},
    {D3DERR_TEXTURE_LOCKED, "D3DERR_TEXTURE_LOCKED"},
    {D3DERR_TEXTURE_NOT_LOCKED, "D3DERR_TEXTURE_NOT_LOCKED"},
    {D3DERR_TEXTURE_GETSURF_FAILED, "D3DERR_TEXTURE_GETSURF_FAILED"},
    {D3DERR_MATRIX_CREATE_FAILED, "D3DERR_MATRIX_CREATE_FAILED"},
    {D3DERR_MATRIX_DESTROY_FAILED, "D3DERR_MATRIX_DESTROY_FAILED"},
    {D3DERR_MATRIX_SETDATA_FAILED, "D3DERR_MATRIX_SETDATA_FAILED"},
    {D3DERR_MATRIX_GETDATA_FAILED, "D3DERR_MATRIX_GETDATA_FAILED"},
    {D3DERR_SETVIEWPORTDATA_FAILED, "D3DERR_SETVIEWPORTDATA_FAILED"},
    {D3DERR_INVALIDCURRENTVIEWPORT, "D3DERR_INVALIDCURRENTVIEWPORT"},
    {D3DERR_INVALIDPRIMITIVETYPE, "D3DERR_INVALIDPRIMITIVETYPE"},
    {D3DERR_INVALIDVERTEXTYPE, "D3DERR_INVALIDVERTEXTYPE"},
    {D3DERR_TEXTURE_BADSIZE, "D3DERR_TEXTURE_BADSIZE"},
    {D3DERR_MATERIAL_CREATE_FAILED, "D3DERR_MATERIAL_CREATE_FAILED"},
    {D3DERR_MATERIAL_DESTROY_FAILED, "D3DERR_MATERIAL_DESTROY_FAILED"},
    {D3DERR_MATERIAL_SETDATA_FAILED, "D3DERR_MATERIAL_SETDATA_FAILED"},
    {D3DERR_MATERIAL_GETDATA_FAILED, "D3DERR_MATERIAL_GETDATA_FAILED"},
    {D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY, "D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY"},
    {D3DERR_ZBUFF_NEEDS_VIDEOMEMORY, "D3DERR_ZBUFF_NEEDS_VIDEOMEMORY"},
    {D3DERR_LIGHT_SET_FAILED, "D3DERR_LIGHT_SET_FAILED"},
    {D3DERR_SCENE_IN_SCENE, "D3DERR_SCENE_IN_SCENE"},
    {D3DERR_SCENE_NOT_IN_SCENE, "D3DERR_SCENE_NOT_IN_SCENE"},
    {D3DERR_SCENE_BEGIN_FAILED, "D3DERR_SCENE_BEGIN_FAILED"},
    {D3DERR_SCENE_END_FAILED, "D3DERR_SCENE_END_FAILED"},
    {D3DERR_INBEGIN, "D3DERR_INBEGIN"},
    {D3DERR_NOTINBEGIN, "D3DERR_NOTINBEGIN"},
    {D3DERR_NOVIEWPORTS, "D3DERR_NOVIEWPORTS"},
    {D3DERR_VIEWPORTDATANOTSET, "D3DERR_VIEWPORTDATANOTSET"},
};

const char *GetDirectDrawErrorName(int hresult) {
    {
        int entryIndex1;
        for (entryIndex1 = 0; entryIndex1 < (int)(sizeof(kDirectDrawErrorNames) / sizeof((kDirectDrawErrorNames)[0])); ++entryIndex1) {
            const DirectDrawErrorName & entry = (kDirectDrawErrorNames)[entryIndex1];
        if (entry.hresult == hresult) {
            return entry.name;
        }
    }
    }

    return "Unknown Error";
}
} // namespace

// Reimplements 0x4a93d0: zVideo_dd::EnumDirectDrawDeviceCallback
BOOL CALLBACK EnumDirectDrawDeviceCallback(GUID *guid, LPSTR driverDescription, LPSTR driverName,
                                           LPVOID context) {
    (void)context;

    const int acceptedIndex = g_zVideo_NumAcceptedDirectDrawDevices;
    const int ordinal = g_zVideo_DirectDrawEnumOrdinal;
    g_zVideo_DirectDrawEnumOrdinal = ordinal + 1;

    printf("\n%d: Device [%s] - %s\n", ordinal, driverName, driverDescription);
    fflush(stdout);

    if (g_zVideo_NumAcceptedDirectDrawDevices >= 4) {
        printf("\nCan't handle this many devices - IGNORING THE REST");
        return FALSE;
    }

    zVidHwApiDeviceRecordPartial &entry = g_zVideo_HwApiDeviceTable[acceptedIndex];
    memset(&entry, 0, sizeof(entry));
    if (guid != 0) {
        entry.pDirectDrawGuid = &entry.m_directDrawGuidStorage;
        entry.m_directDrawGuidStorage = *guid;
    } else {
        entry.pDirectDrawGuid = 0;
    }

    strncpy(entry.m_driverName, driverName, sizeof(entry.m_driverName));
    strncpy(entry.m_driverDescription, driverDescription, sizeof(entry.m_driverDescription));
    g_zVideo_pSelectedHwApiDeviceRecord = &entry;

    CreateDirectDraw2ForSelectedDevice();

    memset(&g_zVideo_DDrawCapsHal, 0, sizeof(g_zVideo_DDrawCapsHal));
    memset(&g_zVideo_DDrawCapsHel, 0, sizeof(g_zVideo_DDrawCapsHel));
    g_zVideo_DDrawCapsHal.dwSize = sizeof(g_zVideo_DDrawCapsHal);
    g_zVideo_DDrawCapsHel.dwSize = sizeof(g_zVideo_DDrawCapsHel);

    const HRESULT capsResult =
        g_zVideo_pDirectDraw2->GetCaps(&g_zVideo_DDrawCapsHal, &g_zVideo_DDrawCapsHel);
    if (capsResult != DD_OK) {
        ReportError(static_cast<int>(capsResult), kZVideoDirectDrawSourceFile, 0x739);
        return FALSE;
    }

    if ((g_zVideo_DDrawCapsHal.dwCaps & 0x200) != 0 ||
        (g_zVideo_DDrawCapsHel.dwCaps & 0x200) != 0) {
        entry.m_deviceFeatureFlags = 1;
        strncat(entry.m_driverName, "[AGP]",
                     sizeof(entry.m_driverName) - strlen(entry.m_driverName) - 1);
    }

    DDSCAPS videoMemCaps = {0};
    videoMemCaps.dwCaps = DDSCAPS_VIDEOMEMORY;
    if (g_zVideo_pDirectDraw2->GetAvailableVidMem(
            &videoMemCaps, reinterpret_cast<DWORD *>(&entry.m_videoMemTotalBytes),
            reinterpret_cast<DWORD *>(&entry.m_videoMemFreeBytes)) != DD_OK) {
        entry.m_videoMemFreeBytes = 0;
        entry.m_videoMemTotalBytes = 0;
    }

    DDSCAPS textureMemCaps = {0};
    textureMemCaps.dwCaps = DDSCAPS_TEXTURE;
    if (g_zVideo_pDirectDraw2->GetAvailableVidMem(
            &textureMemCaps, reinterpret_cast<DWORD *>(&entry.m_textureMemTotalBytes),
            reinterpret_cast<DWORD *>(&entry.m_textureMemFreeBytes)) != DD_OK) {
        entry.m_textureMemFreeBytes = 0;
        entry.m_textureMemTotalBytes = 0;
    }

    if (EnumerateDirect3DDevicesForRecord(&entry) != 0) {
        g_zVideo_NumAcceptedDirectDrawDevices += 1;
    }

    TeardownVideoSubsystem();
    return TRUE;
}

// Reimplements 0x4a96b0: zVideo_dd::EnumDirect3DDeviceCallback
HRESULT CALLBACK EnumDirect3DDeviceCallback(GUID *guid, LPSTR deviceDescription, LPSTR deviceName,
                                            D3DDEVICEDESC *hwDesc, D3DDEVICEDESC *,
                                            LPVOID context) {
    zVidHwApiDeviceRecordPartial *entry = static_cast<zVidHwApiDeviceRecordPartial *>(context);
    zVidD3DDriverRecordPartial &driver = entry->m_d3dDrivers[entry->m_acceptedD3DDeviceCount];

    printf("DRIVER:%s - %s\n", deviceName, deviceDescription);
    fflush(stdout);

    const unsigned int descFlags = D3DDescReadDword(hwDesc, kD3DDescFlagsOffset);
    if (descFlags == 0) {
        printf("-----SKIPPED - Does not interface with hardware\n");
        fflush(stdout);
        return 1;
    }

    if ((descFlags & D3DDD_COLORMODEL) != 0 &&
        D3DDescReadDword(hwDesc, kD3DDescColorModelOffset) != D3DCOLOR_RGB) {
        printf("-----SKIPPED - Does not support RGB color\n");
        fflush(stdout);
        return 1;
    }

    if ((D3DDescReadDword(hwDesc, kD3DDescZBufferBitDepthOffset) & DDBD_16) == 0) {
        printf("-----SKIPPED - Does not support 16-bit Z buffer\n");
        fflush(stdout);
        return 1;
    }

    if (entry->m_acceptedD3DDeviceCount >= 4) {
        TeardownVideoSubsystem();
        zError::ReportOld(0x800, kZVideoDirectDrawSourceFile, 0x7d3,
                          "Maximum number of Direct3D drivers exceeded");
        return 0;
    }

    if (guid != 0) {
        driver.pD3DDeviceGuid = &driver.m_d3dDeviceGuidStorage;
        driver.m_d3dDeviceGuidStorage = *guid;
    } else {
        driver.pD3DDeviceGuid = 0;
    }

    memcpy(driver.m_hwDesc, hwDesc, kD3DDescCopyBytes);
    if (D3DDescReadDword(reinterpret_cast<D3DDEVICEDESC *>(driver.m_hwDesc),
                         kD3DDescMaxTextureWidthOffset) == 0) {
        D3DDriverDescWriteDword(driver, kD3DDescMaxTextureWidthOffset, 0x100);
    }
    if (D3DDescReadDword(reinterpret_cast<D3DDEVICEDESC *>(driver.m_hwDesc),
                         kD3DDescMaxTextureHeightOffset) == 0) {
        D3DDriverDescWriteDword(driver, kD3DDescMaxTextureHeightOffset, 0x100);
    }

    strncpy(driver.m_deviceName, deviceName, sizeof(driver.m_deviceName));
    strncpy(driver.m_deviceDescription, deviceDescription, sizeof(driver.m_deviceDescription));
    printf("+++++OK\n");
    fflush(stdout);
    entry->m_acceptedD3DDeviceCount += 1;
    g_zVid_AcceptedHardwareRendererCount += 1;
    return 1;
}

// Reimplements 0x4a6930: zVideo_dd::PrepareWindowForMode
RECOIL_NOINLINE int RECOIL_CDECL PrepareWindowForMode() {
    SetMenu(g_zVideo_hWnd, 0);
    SetWindowLongA(g_zVideo_hWnd, GWL_EXSTYLE, 0x00040000);
    SetWindowLongA(g_zVideo_hWnd, GWL_STYLE, static_cast<LONG>(0x82000000u));
    UpdateWindow(g_zVideo_hWnd);
    SetFocus(g_zVideo_hWnd);

    if (g_zVideo_hWnd != 0) {
        HDC screenDc = GetDC(0);
        if ((GetDeviceCaps(screenDc, RASTERCAPS) & RC_PALETTE) != 0) {
            GetSystemPaletteEntries(screenDc, 0, 0x100, g_zVideo_SystemPaletteEntries);
        }
        ReleaseDC(0, screenDc);
    }

    return 0;
}

// Reimplements 0x4a7d20: zVideo_dd::OpenVideoMode
RECOIL_NOINLINE int RECOIL_FASTCALL OpenVideoMode(int) {
    if (PrepareWindowForMode() != 0) {
        return 1;
    }

    return CreateDirectDraw2ForSelectedDevice() != 0 ? 1 : 0;
}

// Reimplements 0x4a9390: zVideo_dd::RunDirectDrawDeviceEnumeration
RECOIL_NOINLINE int RECOIL_CDECL RunDirectDrawDeviceEnumeration() {
    printf("\nENUMERATE GRAPHICS DEVICES...\n");
    const HRESULT hresult = DirectDrawEnumerateA(EnumDirectDrawDeviceCallback, 0);
    if (hresult == DD_OK) {
        return 1;
    }

    ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x6ad);
    return 0;
}

// Reimplements 0x4a8800: zVideo_dd::CreateDirectDraw2ForSelectedDevice
RECOIL_NOINLINE int RECOIL_CDECL CreateDirectDraw2ForSelectedDevice() {
    IDirectDraw *directDraw1 = 0;
    const HRESULT createResult = DirectDrawCreate(
        g_zVideo_pSelectedHwApiDeviceRecord->pDirectDrawGuid, &directDraw1, 0);
    if (createResult != DD_OK) {
        return ReportError(static_cast<int>(createResult), kZVideoDirectDrawSourceFile,
                           0x3c4);
    }

    const HRESULT queryResult = directDraw1->QueryInterface(
        IID_IDirectDraw2, reinterpret_cast<void **>(&g_zVideo_pDirectDraw2));
    if (queryResult != DD_OK) {
        return ReportError(static_cast<int>(queryResult), kZVideoDirectDrawSourceFile,
                           0x3cb);
    }

    directDraw1->Release();
    return 0;
}

// Reimplements 0x4a95e0: zVideo_dd::EnumerateDirect3DDevicesForRecord
RECOIL_NOINLINE int RECOIL_FASTCALL
EnumerateDirect3DDevicesForRecord(zVidHwApiDeviceRecordPartial *entry) {
    unsigned char unusedStackZeroing[0x68] = {0};
    (void)unusedStackZeroing;

    printf("  Direct3D drivers for %s\n", entry->m_driverName);
    fflush(stdout);

    const HRESULT queryResult = g_zVideo_pDirectDraw2->QueryInterface(
        IID_IDirect3D2, reinterpret_cast<void **>(&g_zVideo_pD3D2));
    if (queryResult != DD_OK) {
        ReportError(static_cast<int>(queryResult), kZVideoDirectDrawSourceFile, 0x781);
        return 0;
    }

    entry->m_acceptedD3DDeviceCount = 0;
    g_zVideo_pD3D2->EnumDevices(EnumDirect3DDeviceCallback, entry);
    ReleaseComInterface(g_zVideo_pD3D2);

    if (entry->m_acceptedD3DDeviceCount == 0) {
        printf("    No usable Direct3D drivers\n");
        return 0;
    }

    return 1;
}

// Reimplements 0x4a7b40: zVideo_dd::StartupEnumerateAndDefaultSelect
RECOIL_NOINLINE void RECOIL_CDECL StartupEnumerateAndDefaultSelect() {
    RunDirectDrawDeviceEnumeration();
    g_zVideo_pSelectedHwApiDeviceRecord = &g_zVideo_HwApiDeviceTable[0];
    g_zVideo_pSelectedD3DDeviceInfo = 0;
}

RECOIL_NOINLINE int RECOIL_CDECL ShutdownVideoSystem() {
    if (g_zImage_DefaultTextureRecord != 0) {
        zVideo_dd3d::TextureRecord_Destroy(g_zImage_DefaultTextureRecord);
        g_zImage_DefaultTextureRecord = 0;
    }

    TeardownVideoSubsystem();
    return 0;
}

// Reimplements 0x4a8060: zVideo_dd::LockDirectDrawSurface
RECOIL_NOINLINE int RECOIL_FASTCALL
LockDirectDrawSurface(IDirectDrawSurface3 *surface, DDSURFACEDESC *outLockedSurfaceDesc) {
    memset(outLockedSurfaceDesc, 0, sizeof(*outLockedSurfaceDesc));
    outLockedSurfaceDesc->dwSize = sizeof(*outLockedSurfaceDesc);

    for (;;) {
        HRESULT hresult = surface->Lock(0, outLockedSurfaceDesc, DDLOCK_WAIT, 0);
        if (hresult == DD_OK) {
            return 0;
        }

        if (hresult == DDERR_SURFACELOST) {
            hresult = surface->Restore();
            if (hresult == DD_OK) {
                continue;
            }
        }

        ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x1b9);
        return 0x5a56ffff;
    }
}

// Reimplements 0x4a80c0: zVideo_dd::UnlockDirectDrawSurface
RECOIL_NOINLINE int RECOIL_FASTCALL UnlockDirectDrawSurface(IDirectDrawSurface3 *surface) {
    for (;;) {
        HRESULT hresult = surface->Unlock(0);
        if (hresult == DD_OK) {
            return 0;
        }

        if (hresult == DDERR_SURFACELOST) {
            hresult = surface->Restore();
            if (hresult == DD_OK) {
                continue;
            }
        }

        ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x1d7);
        return 0x5a56ffff;
    }
}

// Reimplements 0x4a8100: zVideo_dd::LockSurface_WaitRestore
RECOIL_NOINLINE int RECOIL_FASTCALL LockSurface_WaitRestore(IDirectDrawSurface3 *surface,
                                                                     DDSURFACEDESC *lockedDescOut) {
    memset(lockedDescOut, 0, sizeof(*lockedDescOut));
    lockedDescOut->dwSize = sizeof(*lockedDescOut);

    for (;;) {
        HRESULT hresult = surface->Lock(0, lockedDescOut, DDLOCK_WAIT, 0);
        if (hresult == DD_OK) {
            return 0;
        }

        if (hresult == DDERR_SURFACELOST) {
            hresult = surface->Restore();
            if (hresult == DD_OK) {
                continue;
            }
        }

        ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x1fd);
        return 0x5a56ffff;
    }
}

// Reimplements 0x4a8160: zVideo_dd::UnlockSurface_WaitRestore
RECOIL_NOINLINE int RECOIL_FASTCALL
UnlockSurface_WaitRestore(IDirectDrawSurface3 *surface) {
    for (;;) {
        HRESULT hresult = surface->Unlock(0);
        if (hresult == DD_OK) {
            return 0;
        }

        if (hresult == DDERR_SURFACELOST) {
            hresult = surface->Restore();
            if (hresult == DD_OK) {
                continue;
            }
        }

        ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x21b);
        return 0x5a56ffff;
    }
}

// Reimplements 0x4a7fc0: zVideo_dd::LockSurfaceState
RECOIL_NOINLINE int RECOIL_FASTCALL
LockSurfaceState(zVideo_SurfaceStatePartial *surfaceState) {
    if (g_zVideo_FullscreenOption != 0) {
        goto CheckLocked;
    }
    if (surfaceState != &g_zVideo_DisplayModeSurfaceState) {
        goto CheckLocked;
    }
    return 0;

CheckLocked:
    if (surfaceState->locked != 0) {
        return 0;
    }

    DDSURFACEDESC lockedSurfaceDesc;
    const int result = LockDirectDrawSurface(surfaceState->surf, &lockedSurfaceDesc);
    if (result == 0) {
        const int locked = 1;
        surfaceState->width = static_cast<int>(lockedSurfaceDesc.dwWidth);
        surfaceState->locked = locked;
        surfaceState->height = static_cast<int>(lockedSurfaceDesc.dwHeight);
        surfaceState->lockInfoValid = locked;
        surfaceState->pitch = static_cast<int>(lockedSurfaceDesc.lPitch);
        surfaceState->pixels = lockedSurfaceDesc.lpSurface;
    }

    return result;
}

// Reimplements 0x4a8030: zVideo_dd::UnlockSurfaceState
RECOIL_NOINLINE int RECOIL_FASTCALL
UnlockSurfaceState(zVideo_SurfaceStatePartial *surfaceState) {
    if (g_zVideo_FullscreenOption != 0) {
        goto CheckLocked;
    }
    if (surfaceState != &g_zVideo_DisplayModeSurfaceState) {
        goto CheckLocked;
    }
    return 0;

CheckLocked:
    if (surfaceState->locked == 0) {
        return 0;
    }

    surfaceState->locked = 0;
    return UnlockDirectDrawSurface(surfaceState->surf);
}

// Reimplements 0x4a83d0: zVideo_dd::Image_LazyCreateBackingSurface
RECOIL_NOINLINE IDirectDrawSurface3 *RECOIL_FASTCALL
Image_LazyCreateBackingSurface(zVidImagePartial *image, unsigned int ddsCapsFlags) {
    if (image->alphaMap != 0 || image->pixels == 0 || image->height == 0 ||
        image->width == 0) {
        return 0;
    }

    IDirectDrawSurface *baseSurface = 0;
    IDirectDrawSurface3 *surface3 = 0;
    DDSURFACEDESC desc = {0};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = 0x10007;
    desc.dwHeight = static_cast<DWORD>(image->height);
    desc.dwWidth = static_cast<DWORD>(image->width);
    desc.ddsCaps.dwCaps = ddsCapsFlags | DDSCAPS_OFFSCREENPLAIN;
    image->surface = 0;

    HRESULT hresult = g_zVideo_pDirectDraw2->CreateSurface(&desc, &baseSurface, 0);
    if (hresult == DD_OK) {
        hresult = baseSurface->QueryInterface(IID_IDirectDrawSurface3,
                                              reinterpret_cast<void **>(&surface3));
        if (hresult == DD_OK) {
            image->surface = surface3;
            Image_PopulateSurfaceFromHeapPixels(image);
            return image->surface;
        }
    }

    ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x2ed);
    return image->surface;
}

// Reimplements 0x4a8500: zVideo_dd::Image_PopulateSurfaceFromHeapPixels
RECOIL_NOINLINE int RECOIL_FASTCALL
Image_PopulateSurfaceFromHeapPixels(zVidImagePartial *image) {
    DDSURFACEDESC lockedSurfaceDesc = {0};
    lockedSurfaceDesc.dwSize = sizeof(lockedSurfaceDesc);

    for (;;) {
        HRESULT hresult = image->surface->Lock(0, &lockedSurfaceDesc, DDLOCK_WAIT, 0);
        if (hresult == DD_OK) {
            break;
        }

        if (hresult == DDERR_SURFACELOST) {
            const HRESULT restoreResult = image->surface->Restore();
            if (restoreResult != DD_OK) {
                ReportError(static_cast<int>(restoreResult), kZVideoDirectDrawSourceFile,
                            0x31b);
            }
            continue;
        }

        ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x31f);
        return 0;
    }

    const int rowBytes = static_cast<int>(image->width) << 1;
    unsigned char *srcPixels = static_cast<unsigned char *>(image->pixels);
    unsigned char *dstPixels = static_cast<unsigned char *>(lockedSurfaceDesc.lpSurface);
    {
    for (int row = 0; row < image->height; ++row) {
        memcpy(dstPixels, srcPixels, rowBytes);
        dstPixels += lockedSurfaceDesc.lPitch;
        srcPixels += rowBytes;
    }
    }

    free(image->pixels);
    image->pixels = lockedSurfaceDesc.lpSurface;
    image->pitchWords = lockedSurfaceDesc.lPitch >> 1;

    for (;;) {
        HRESULT hresult = image->surface->Unlock(&lockedSurfaceDesc);
        if (hresult == DD_OK) {
            return 1;
        }

        if (hresult == DDERR_SURFACELOST) {
            const HRESULT restoreResult = image->surface->Restore();
            if (restoreResult != DD_OK) {
                ReportError(static_cast<int>(restoreResult), kZVideoDirectDrawSourceFile,
                            0x33b);
            }
            continue;
        }

        ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x33f);
        return 0;
    }
}

// Reimplements 0x4a84c0: zVideo_dd::Image_LazyCreateVideoMemorySurface
RECOIL_NOINLINE IDirectDrawSurface3 *RECOIL_FASTCALL
Image_LazyCreateVideoMemorySurface(zVidImagePartial *image) {
    const int featureFlags =
        g_zVideo_pSelectedHwApiDeviceRecord != 0
            ? g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags
            : 0;
    if (g_zVideo_UseHalfResBackbuffer == 0 && featureFlags == 0) {
        return 0;
    }

    const unsigned int caps = (featureFlags != 0 ? 0x20000000 : 0) + DDSCAPS_VIDEOMEMORY;
    return Image_LazyCreateBackingSurface(image, caps);
}

// Reimplements 0x4a8650: zVideo_dd::Image_EnsureSurfaceForCurrentDevice
RECOIL_NOINLINE void RECOIL_FASTCALL Image_EnsureSurfaceForCurrentDevice(zVidImagePartial *image) {
    if (g_zVideo_IsInitialized != 0 && image->surface != 0) {
        image->surface->Release();
    }

    if (image->surface != 0) {
        image->surface = 0;
        image->pixels = 0;
    }
}

// Reimplements 0x4a8680: zVideo_dd::Image_UploadPixelsToSurface
RECOIL_NOINLINE int RECOIL_FASTCALL Image_UploadPixelsToSurface(zVidImagePartial *image,
                                                                         HDC *outHdc) {
    if (g_zVideo_RendererType == 2) {
        return 0;
    }

    if (image->surface == 0) {
        const unsigned int caps =
            g_zVideo_pSelectedHwApiDeviceRecord != 0 &&
                    g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags != 0
                ? 0x20004000
                : DDSCAPS_SYSTEMMEMORY;
        if (Image_LazyCreateBackingSurface(image, caps) == 0) {
            return 0;
        }
    }

    const HRESULT hresult = image->surface->GetDC(outHdc);
    if (hresult == DD_OK) {
        return 1;
    }

    ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x36d);
    return 0;
}

// Reimplements 0x4a86f0: zVideo_dd::Image_ReleaseSurface
RECOIL_NOINLINE int RECOIL_FASTCALL Image_ReleaseSurface(zVidImagePartial *image,
                                                                  HDC hdc) {
    if (image->surface == 0) {
        return 0;
    }

    const HRESULT hresult = image->surface->ReleaseDC(hdc);
    if (hresult == DD_OK) {
        return 1;
    }

    ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x382);
    return 0;
}

// Reimplements 0x4a7d90: zVideo_dd::BltSwToPrimaryRectDirect
RECOIL_NOINLINE void RECOIL_FASTCALL BltSwToPrimaryRectDirect(zVidRect32 *srcRect,
                                                              zVidRect32 *dstRect) {
    const HRESULT hresult = g_zVideo_PrimarySurfaceState.surf->Blt(
        reinterpret_cast<RECT *>(dstRect), g_zVideo_SwSurfaceState.surf,
        reinterpret_cast<RECT *>(srcRect), DDBLT_WAIT, 0);
    if (hresult != DD_OK) {
        ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0xe9);
    }
}

// Reimplements 0x4a7dd0: zVideo_dd::BltPrimaryToSwRectDirect
RECOIL_NOINLINE void RECOIL_FASTCALL BltPrimaryToSwRectDirect(zVidRect32 *srcRect,
                                                              zVidRect32 *dstRect) {
    const HRESULT hresult = g_zVideo_SwSurfaceState.surf->Blt(
        reinterpret_cast<RECT *>(dstRect), g_zVideo_PrimarySurfaceState.surf,
        reinterpret_cast<RECT *>(srcRect), DDBLT_WAIT, 0);
    if (hresult != DD_OK) {
        ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0xfc);
    }
}

// Reimplements 0x4a7e10: zVideo_dd::BltSwToPrimaryRect
RECOIL_NOINLINE void RECOIL_FASTCALL BltSwToPrimaryRect(zVidImagePartial *srcImage,
                                                        int srcColorKeyEnable,
                                                        zVidRect32 *srcRect, zVidRect32 *dstRect) {
    if (srcImage->surface == 0) {
        const unsigned int caps =
            g_zVideo_pSelectedHwApiDeviceRecord != 0 &&
                    g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags != 0
                ? 0x20004000
                : DDSCAPS_SYSTEMMEMORY;
        if (Image_LazyCreateBackingSurface(srcImage, caps) == 0) {
            return;
        }
    }

    zVidRect32 srcRectLocal;
    if (srcRect != 0) {
        srcRectLocal = *srcRect;
    } else {
        srcRectLocal.left = 0;
        srcRectLocal.top = 0;
        srcRectLocal.right = srcImage->width;
        srcRectLocal.bottom = srcImage->height;
    }

    zVidRect32 dstRectLocal;
    if (dstRect != 0) {
        dstRectLocal = *dstRect;
    } else {
        dstRectLocal.left = 0;
        dstRectLocal.top = 0;
        dstRectLocal.right = srcRectLocal.right - srcRectLocal.left;
        dstRectLocal.bottom = srcRectLocal.bottom - srcRectLocal.top;
    }

    int clipped = zVideo_buff::ClipCoordToRange(&dstRectLocal.left, 0,
                                                         g_zVideo_PrimarySurfaceState.width - 1);
    if (clipped < 0) {
        srcRectLocal.left -= clipped;
    } else if (clipped > 0) {
        return;
    }

    clipped =
        zVideo_buff::ClipCoordToRange(&dstRectLocal.right, 0, g_zVideo_PrimarySurfaceState.width);
    if (clipped < 0) {
        return;
    }
    if (clipped > 0) {
        srcRectLocal.right -= clipped;
    }

    clipped = zVideo_buff::ClipCoordToRange(&dstRectLocal.top, 0,
                                            g_zVideo_PrimarySurfaceState.height - 1);
    if (clipped < 0) {
        srcRectLocal.top -= clipped;
    } else if (clipped > 0) {
        return;
    }

    clipped =
        zVideo_buff::ClipCoordToRange(&dstRectLocal.bottom, 0, g_zVideo_PrimarySurfaceState.height);
    if (clipped < 0) {
        return;
    }
    if (clipped > 0) {
        srcRectLocal.bottom -= clipped;
    }

    const int wasLocked = g_zVideo_PrimarySurfaceState.locked;
    if (wasLocked != 0) {
        UnlockSurfaceState(&g_zVideo_PrimarySurfaceState);
    }

    const DWORD bltFlags =
        DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE | (srcColorKeyEnable != 0 ? DDBLT_KEYSRC : 0);
    const HRESULT hresult = g_zVideo_PrimarySurfaceState.surf->Blt(
        reinterpret_cast<RECT *>(&dstRectLocal), srcImage->surface,
        reinterpret_cast<RECT *>(&srcRectLocal), bltFlags, 0);

    if (wasLocked != 0) {
        LockSurfaceState(&g_zVideo_PrimarySurfaceState);
    }

    if (hresult != DD_OK) {
        ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x159);
    }
}

// Reimplements 0x4a81a0: zVideo_dd::ZBuffer_DepthFillRect
RECOIL_NOINLINE void RECOIL_FASTCALL ZBuffer_DepthFillRect(zVidRect32 *dstRect) {
    if (g_zVideo_pZBufferSurface == 0) {
        return;
    }

    DDBLTFX bltFx = {0};
    bltFx.dwSize = sizeof(bltFx);
    bltFx.dwFillDepth = 0;
    BltFillWithRestore(g_zVideo_pZBufferSurface, dstRect, DDBLT_DEPTHFILL, &bltFx, 0x242);
}

// Reimplements 0x4a8220: zVideo_dd::ClearScreenAndZBufferRect
RECOIL_NOINLINE void RECOIL_FASTCALL
ClearScreenAndZBufferRect(zVidRect32 *dstRect, zVideo_SurfaceStatePartial *colorSurfaceState) {
    DDBLTFX bltFx = {0};
    bltFx.dwSize = sizeof(bltFx);

    if (g_zVideo_ClearScreenBufferEnabled != 0) {
        bltFx.dwFillColor = g_zVideo_ClearColorPacked16;
        if (!BltFillWithRestore(colorSurfaceState->surf, dstRect, DDBLT_COLORFILL | DDBLT_WAIT,
                                &bltFx, 0x267)) {
            return;
        }
    }

    if (g_zVideo_pZBufferSurface == 0) {
        return;
    }

    bltFx.dwFillDepth = 0;
    BltFillWithRestore(g_zVideo_pZBufferSurface, dstRect, DDBLT_DEPTHFILL, &bltFx, 0x27f);
}

// Reimplements 0x4a82f0: zVideo_dd::ClearSwBackbufferAndZBufferRects
RECOIL_NOINLINE void RECOIL_FASTCALL ClearSwBackbufferAndZBufferRects(zVidRect32 *colorRect,
                                                                      zVidRect32 *zRect) {
    DDBLTFX bltFx = {0};
    bltFx.dwSize = sizeof(bltFx);

    if (g_zVideo_ClearScreenBufferEnabled != 0) {
        bltFx.dwFillColor = g_zVideo_ClearColorPacked16;
        if (!BltFillWithRestore(g_zVideo_SwSurfaceState.surf, colorRect,
                                DDBLT_COLORFILL | DDBLT_WAIT, &bltFx, 0x2a5)) {
            return;
        }
    }

    if (g_zVideo_pZBufferSurface == 0) {
        return;
    }

    bltFx.dwFillDepth = 0;
    BltFillWithRestore(g_zVideo_pZBufferSurface, zRect, DDBLT_DEPTHFILL, &bltFx, 0x2bd);
}

// Reimplements 0x4a7d70: zVideo_dd::FlipToGDIIfAttached
RECOIL_NOINLINE void RECOIL_CDECL FlipToGDIIfAttached() {
    if (g_zVideo_pDirectDraw2 != 0 && g_zVideo_PrimaryHasAttachedBackbuffer != 0) {
        g_zVideo_pDirectDraw2->FlipToGDISurface();
    }
}

// Reimplements 0x4a8720: zVideo_dd::SetDisplayMode
RECOIL_NOINLINE int RECOIL_CDECL SetDisplayMode() {
    HRESULT hresult = g_zVideo_pDirectDraw2->SetCooperativeLevel(g_zVideo_hWnd, 0x13);
    if (hresult != DD_OK) {
        ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x393);
        return 0;
    }

    hresult = g_zVideo_pDirectDraw2->SetDisplayMode(g_zVideo_DisplayModeSurfaceState.width,
                                                    g_zVideo_DisplayModeSurfaceState.height,
                                                    g_zVideo_DisplayModeBpp, 0, 0);
    if (hresult == DD_OK) {
        return 1;
    }

    ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x39c);
    return 0;
}

RECOIL_NOINLINE int RECOIL_FASTCALL SetVideoMode(int) {
    if (SetDisplayMode() == 0) {
        return 1;
    }

    if (RestoreDisplaySurfaces() != 0) {
        return 1;
    }

    if (ReleaseAllInterfacesAndSurfaces() != 0) {
        return 1;
    }

    if (CreateFullscreenSurfacesForRenderer() != 0) {
        return 1;
    }

    if (g_zVideo_RendererType == 1 && zVideo_dd3d::CreateDeviceState() != 0) {
        return 1;
    }

    if (RestoreDisplaySurfaces() != 0) {
        return 1;
    }

    return VerifyFullscreenSurfaceLocks() != 0 ? 1 : 0;
}

// Reimplements 0x4a9060: zVideo_dd::VerifyFullscreenSurfaceLocks
RECOIL_NOINLINE int RECOIL_CDECL VerifyFullscreenSurfaceLocks() {
    if (LockSurfaceState(&g_zVideo_SwSurfaceState) != 0) {
        return 1;
    }
    if (UnlockSurfaceState(&g_zVideo_SwSurfaceState) != 0) {
        return 1;
    }
    if (LockSurfaceState(&g_zVideo_PrimarySurfaceState) != 0) {
        return 1;
    }
    if (UnlockSurfaceState(&g_zVideo_PrimarySurfaceState) != 0) {
        return 1;
    }
    if (LockSurfaceState(&g_zVideo_DisplayModeSurfaceState) != 0) {
        return 1;
    }

    return UnlockSurfaceState(&g_zVideo_DisplayModeSurfaceState) != 0 ? 1 : 0;
}

// Reimplements 0x4a90e0: zVideo_dd::RestoreDisplaySurfaces
RECOIL_NOINLINE int RECOIL_CDECL RestoreDisplaySurfaces() {
    if (g_zVideo_DisplayModeSurfaceState.surf != 0) {
        const HRESULT hresult = g_zVideo_DisplayModeSurfaceState.surf->Restore();
        if (hresult != DD_OK) {
            return ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile,
                               0x5e1);
        }
    }

    if (g_zVideo_PrimarySurfaceState.surf != 0) {
        const HRESULT hresult = g_zVideo_PrimarySurfaceState.surf->Restore();
        if (hresult != DD_OK) {
            return ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile,
                               0x5e8);
        }
    }

    if (g_zVideo_SwSurfaceState.surf != 0) {
        const HRESULT hresult = g_zVideo_SwSurfaceState.surf->Restore();
        if (hresult != DD_OK) {
            return ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile,
                               0x5ef);
        }
    }

    return 0;
}

// Reimplements 0x4a8f80: zVideo_dd::InitFullscreenSoftwarePixelPack
RECOIL_NOINLINE int RECOIL_FASTCALL
InitFullscreenSoftwarePixelPack(IDirectDrawSurface3 *displaySurface) {
    DDPIXELFORMAT pixelFormat = {0};
    pixelFormat.dwSize = sizeof(pixelFormat);

    const HRESULT hresult = displaySurface->GetPixelFormat(&pixelFormat);
    if (hresult != DD_OK) {
        return ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x597);
    }

    if (pixelFormat.dwGBitMask == 0x07e0) {
        zVideo::PixelPack_SetupFromMasks(5, 6, 5, pixelFormat.dwRBitMask, pixelFormat.dwGBitMask,
                                         pixelFormat.dwBBitMask);
        return 0;
    }

    if (pixelFormat.dwGBitMask == 0x03e0) {
        zVideo::PixelPack_SetupFromMasks(5, 5, 5, pixelFormat.dwRBitMask, pixelFormat.dwGBitMask,
                                         pixelFormat.dwBBitMask);
        return 0;
    }

    if (pixelFormat.dwGBitMask == 0xff00) {
        zVideo::PixelPack_SetupFromMasks(5, 6, 5, pixelFormat.dwRBitMask, pixelFormat.dwGBitMask,
                                         pixelFormat.dwBBitMask);
        return 0;
    }

    TeardownVideoSubsystem();
    zError::ReportOld(0x800, kZVideoDirectDrawSourceFile, 0x5bd, "Unrecognized pixel format");
    return 0x5a56ffff;
}

// Reimplements 0x4a88b0: zVideo_dd::CreateSurface3FromDesc
RECOIL_NOINLINE HRESULT RECOIL_FASTCALL CreateSurface3FromDesc(IDirectDraw2 *directDraw,
                                                               DDSURFACEDESC *desc,
                                                               IDirectDrawSurface3 **outSurface) {
    IDirectDrawSurface *createdSurface = 0;
    HRESULT result = directDraw->CreateSurface(desc, &createdSurface, 0);
    if (result == DD_OK) {
        result = createdSurface->QueryInterface(IID_IDirectDrawSurface3,
                                                reinterpret_cast<void **>(outSurface));
        if (result == DD_OK) {
            return createdSurface->Release();
        }
    }

    return result;
}

// Reimplements 0x4a88f0: zVideo_dd::CreateFullscreenSurfacesForRenderer
RECOIL_NOINLINE int RECOIL_CDECL CreateFullscreenSurfacesForRenderer() {
    if (g_zVideo_UseHalfResBackbuffer != 0) {
        return CreateHalfResBackbufferSurfaces();
    }

    if (g_zVideo_RendererType == 1) {
        return CreateFullscreenHardwareSurfaces();
    }

    return CreateFullscreenSoftwareSurfaces();
}

// Reimplements 0x4a8920: zVideo_dd::CreateHalfResBackbufferSurfaces
RECOIL_NOINLINE int RECOIL_CDECL CreateHalfResBackbufferSurfaces() {
    zOptionEntryPartial defaultGfxFlags = {0};
    zOptionEntryPartial *gfxFlagsOption =
        zGame::Options_FindOption(g_zVideo_ActiveRendererPath != 0 ? "GfxFlags_HW" : "GfxFlags_SW");
    if (gfxFlagsOption == 0) {
        gfxFlagsOption = &defaultGfxFlags;
    }

    DDSURFACEDESC desc = {0};
    desc.dwSize = sizeof(desc);
    desc.dwBackBufferCount = 1;
    desc.dwFlags = 0x21;
    desc.ddsCaps.dwCaps = 0x218;

    HRESULT hresult = CreateSurface3FromDesc(g_zVideo_pDirectDraw2, &desc,
                                             &g_zVideo_DisplayModeSurfaceState.surf);
    if (hresult != DD_OK) {
        return ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x41f);
    }

    DDSCAPS attachedCaps = {0};
    attachedCaps.dwCaps = DDSCAPS_BACKBUFFER;
    g_zVideo_PrimaryHasAttachedBackbuffer = 1;
    hresult = g_zVideo_DisplayModeSurfaceState.surf->GetAttachedSurface(
        &attachedCaps, &g_zVideo_PrimarySurfaceState.surf);
    if (hresult != DD_OK) {
        return ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x429);
    }

    desc.dwFlags = 0x07;
    if ((gfxFlagsOption->payloadOrBuffer & 0x10000) != 0) {
        desc.ddsCaps.dwCaps = 0x4040;
    } else {
        const int featureFlags =
            g_zVideo_pSelectedHwApiDeviceRecord != 0
                ? g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags
                : 0;
        desc.ddsCaps.dwCaps = (featureFlags != 0 ? 0x20003800 : 0) + 0x840;
    }
    desc.dwWidth = static_cast<DWORD>(g_zVideo_SwSurfaceState.width);
    desc.dwHeight = static_cast<DWORD>(g_zVideo_SwSurfaceState.height);

    hresult = CreateSurface3FromDesc(g_zVideo_pDirectDraw2, &desc, &g_zVideo_SwSurfaceState.surf);
    if (hresult != DD_OK) {
        return ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x43f);
    }

    if (InitFullscreenSoftwarePixelPack(g_zVideo_DisplayModeSurfaceState.surf) != 0) {
        return 1;
    }

    hresult = g_zVideo_pDirectDraw2->CreateClipper(0, &g_zVideo_pClipper, 0);
    if (hresult != DD_OK) {
        return ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x447);
    }

    hresult = g_zVideo_pClipper->SetHWnd(0, g_zVideo_hWnd);
    if (hresult != DD_OK) {
        return ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x44b);
    }

    hresult = g_zVideo_DisplayModeSurfaceState.surf->SetClipper(g_zVideo_pClipper);
    if (hresult == DD_OK) {
        return 0;
    }

    return ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x450);
}

// Reimplements 0x4a8b20: zVideo_dd::CreateFullscreenSoftwareSurfaces
RECOIL_NOINLINE int RECOIL_CDECL CreateFullscreenSoftwareSurfaces() {
    zOptionEntryPartial defaultGfxFlags = {0};
    zOptionEntryPartial *gfxFlagsOption =
        zGame::Options_FindOption(g_zVideo_ActiveRendererPath != 0 ? "GfxFlags_HW" : "GfxFlags_SW");
    if (gfxFlagsOption == 0) {
        gfxFlagsOption = &defaultGfxFlags;
    }

    DDSURFACEDESC desc = {0};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = 1;
    desc.ddsCaps.dwCaps = 0xa00;

    HRESULT hresult = CreateSurface3FromDesc(g_zVideo_pDirectDraw2, &desc,
                                             &g_zVideo_DisplayModeSurfaceState.surf);
    if (hresult != DD_OK) {
        return ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x4cc);
    }

    if (LockSurfaceState(&g_zVideo_DisplayModeSurfaceState) == 0) {
        UnlockSurfaceState(&g_zVideo_DisplayModeSurfaceState);
    } else {
        g_zVideo_DisplayModeSurfaceState.surf->Release();
        desc.ddsCaps.dwCaps = 0x200;
        hresult = CreateSurface3FromDesc(g_zVideo_pDirectDraw2, &desc,
                                         &g_zVideo_DisplayModeSurfaceState.surf);
        if (hresult != DD_OK) {
            return ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile,
                               0x4da);
        }
    }

    g_zVideo_PrimaryHasAttachedBackbuffer = 0;
    desc.dwFlags = 7;
    if ((gfxFlagsOption->payloadOrBuffer & 0x10000) != 0) {
        desc.ddsCaps.dwCaps = 0x4040;
    } else {
        const int featureFlags = g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags;
        desc.ddsCaps.dwCaps = (featureFlags != 0 ? 0x20003800 : 0) + 0x840;
    }
    desc.dwWidth = static_cast<DWORD>(g_zVideo_DisplayModeSurfaceState.width);
    desc.dwHeight = static_cast<DWORD>(g_zVideo_DisplayModeSurfaceState.height);

    hresult =
        CreateSurface3FromDesc(g_zVideo_pDirectDraw2, &desc, &g_zVideo_PrimarySurfaceState.surf);
    if (hresult != DD_OK) {
        return ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x4f7);
    }

    desc.dwFlags = 7;
    if ((gfxFlagsOption->payloadOrBuffer & 0x10000) != 0) {
        desc.ddsCaps.dwCaps = 0x4040;
    } else {
        const int featureFlags = g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags;
        desc.ddsCaps.dwCaps = (featureFlags != 0 ? 0x20003800 : 0) + 0x840;
    }
    desc.dwWidth = static_cast<DWORD>(g_zVideo_DisplayModeSurfaceState.width);
    desc.dwHeight = static_cast<DWORD>(g_zVideo_DisplayModeSurfaceState.height);

    hresult = CreateSurface3FromDesc(g_zVideo_pDirectDraw2, &desc, &g_zVideo_SwSurfaceState.surf);
    if (hresult != DD_OK) {
        return ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x50d);
    }

    if (InitFullscreenSoftwarePixelPack(g_zVideo_DisplayModeSurfaceState.surf) != 0) {
        return 1;
    }

    hresult = g_zVideo_pDirectDraw2->CreateClipper(0, &g_zVideo_pClipper, 0);
    if (hresult != DD_OK) {
        return ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x515);
    }

    hresult = g_zVideo_pClipper->SetHWnd(0, g_zVideo_hWnd);
    if (hresult != DD_OK) {
        return ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x519);
    }

    hresult = g_zVideo_DisplayModeSurfaceState.surf->SetClipper(g_zVideo_pClipper);
    if (hresult == DD_OK) {
        return 0;
    }

    return ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x51d);
}

// Reimplements 0x4a8dc0: zVideo_dd::CreateFullscreenHardwareSurfaces
RECOIL_NOINLINE int RECOIL_CDECL CreateFullscreenHardwareSurfaces() {
    DDSURFACEDESC desc = {0};
    desc.dwSize = sizeof(desc);
    desc.dwBackBufferCount = 1;
    desc.dwFlags = 0x21;
    desc.ddsCaps.dwCaps = 0x2218;

    HRESULT hresult = CreateSurface3FromDesc(g_zVideo_pDirectDraw2, &desc,
                                             &g_zVideo_DisplayModeSurfaceState.surf);
    if (hresult != DD_OK) {
        return ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x53b);
    }

    g_zVideo_PrimaryHasAttachedBackbuffer = 1;
    DDSCAPS attachedCaps = {0};
    attachedCaps.dwCaps = DDSCAPS_BACKBUFFER;
    hresult = g_zVideo_DisplayModeSurfaceState.surf->GetAttachedSurface(
        &attachedCaps, &g_zVideo_SwSurfaceState.surf);
    if (hresult != DD_OK) {
        return ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x546);
    }

    desc.dwFlags = 7;
    desc.dwWidth = static_cast<DWORD>(g_zVideo_DisplayModeSurfaceState.width);
    desc.dwHeight = static_cast<DWORD>(g_zVideo_DisplayModeSurfaceState.height);
    const int featureFlags = g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags;
    desc.ddsCaps.dwCaps = (featureFlags != 0 ? 0x20003800 : 0) + 0x840;

    hresult =
        CreateSurface3FromDesc(g_zVideo_pDirectDraw2, &desc, &g_zVideo_PrimarySurfaceState.surf);
    if (hresult != DD_OK) {
        return ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x557);
    }

    if (InitFullscreenSoftwarePixelPack(g_zVideo_DisplayModeSurfaceState.surf) != 0) {
        return 1;
    }

    hresult = g_zVideo_pDirectDraw2->CreateClipper(0, &g_zVideo_pClipper, 0);
    if (hresult != DD_OK) {
        return ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x55f);
    }

    hresult = g_zVideo_pClipper->SetHWnd(0, g_zVideo_hWnd);
    if (hresult != DD_OK) {
        return ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x563);
    }

    hresult = g_zVideo_DisplayModeSurfaceState.surf->SetClipper(g_zVideo_pClipper);
    if (hresult == DD_OK) {
        return 0;
    }

    return ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x567);
}

// Reimplements 0x4a91b0: zVideo_dd::ReleaseAllInterfacesAndSurfaces
RECOIL_NOINLINE int RECOIL_CDECL ReleaseAllInterfacesAndSurfaces() {
    ReleaseComInterface(g_zVideo_pD3DMaterial2);
    ReleaseComInterface(g_zVideo_pD3DViewport2);
    ReleaseComInterface(g_zVideo_pD3DDevice);
    ReleaseComInterface(g_zVideo_pD3D2);
    ReleaseComInterface(g_zVideo_pClipper);
    ReleaseComInterface(g_zVideo_pZBufferSurface);

    if (!PageUnlockBeforeRelease(g_zVideo_SwSurfaceState, 0x652)) {
        return 0;
    }
    ReleaseComInterface(g_zVideo_SwSurfaceState.surf);

    if (!PageUnlockBeforeRelease(g_zVideo_PrimarySurfaceState, 0x662)) {
        return 0;
    }
    ReleaseComInterface(g_zVideo_PrimarySurfaceState.surf);

    ReleaseComInterface(g_zVideo_DisplayModeSurfaceState.surf);
    ReleaseComInterface(g_zVideo_pDDPalette);
    return 0;
}

// Reimplements 0x4a9160: zVideo_dd::VerifySurfaceStateLocking
RECOIL_NOINLINE void RECOIL_FASTCALL VerifySurfaceStateLocking(int callerContext) {
    if ((g_zVideo_SurfaceLockVerifyFlags & 0x20) == 0) {
        return;
    }

    zVideo_SurfaceLockVerifyArgs args = {0};
    args.size = sizeof(args);
    args.callerContext = callerContext;
    const int hresult = g_zVideo_pSurfaceLockVerifier->vtable->VerifySurfaceState(
        g_zVideo_pSurfaceLockVerifier, &args);
    if (hresult != DD_OK) {
        ReportError(hresult, kZVideoDirectDrawSourceFile, 0x61a);
    }
}

// Reimplements 0x4a9300: zVideo_dd::TeardownVideoSubsystem
RECOIL_NOINLINE void RECOIL_CDECL TeardownVideoSubsystem() {
    ReleaseAllInterfacesAndSurfaces();

    if (g_zVideo_pPageUnlockSurface != 0) {
        g_zVideo_pPageUnlockSurface->PageUnlock(0);
        g_zVideo_pPageUnlockSurface->Release();
        g_zVideo_pPageUnlockSurface = 0;
    }

    if (g_zVideo_pSurfaceLockVerifier != 0) {
        VerifySurfaceStateLocking(g_zVideo_SurfaceLockVerifyContext);
        g_zVideo_pSurfaceLockVerifier->vtable->Release(g_zVideo_pSurfaceLockVerifier);
        g_zVideo_pSurfaceLockVerifier = 0;
    }

    if (g_zVideo_pDirectDraw2 != 0) {
        g_zVideo_pDirectDraw2->SetCooperativeLevel(g_zVideo_hWnd, 8);
        g_zVideo_pDirectDraw2->Release();
        g_zVideo_pDirectDraw2 = 0;
    }
}

// Reimplements 0x4a9890: zVideo_dd::PaletteSetEntries
RECOIL_NOINLINE int RECOIL_FASTCALL PaletteSetEntries(unsigned short firstEntry,
                                                               unsigned short entryCount,
                                                               PALETTEENTRY *entries) {
    if (g_zVideo_DisplayModeBpp != 8) {
        return 0;
    }

    const HRESULT hresult = g_zVideo_pDDPalette->SetEntries(0, firstEntry, entryCount, entries);
    if (hresult == DD_OK) {
        return 0;
    }

    ReportError(static_cast<int>(hresult), kZVideoDirectDrawSourceFile, 0x823);
    return 0x5a56ffff;
}

// Reimplements 0x4ad6a0: zVideo_dd::ReportError
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_FASTCALL ReportError(int hresult,
                                                                      const char *sourceFile,
                                                                      int sourceLine) {
    if (hresult == DD_OK) {
        return 0;
    }

    char errorNameBuffer[0x100];
    sprintf(errorNameBuffer, GetDirectDrawErrorName(hresult));

    if (hresult == DDERR_OUTOFVIDEOMEMORY) {
        int textureMemTotalBytes = 0;
        int textureMemFreeBytes = 0;
        int videoMemTotalBytes = 0;
        int videoMemFreeBytes = 0;

        if (g_zVideo_pfnQueryTextureMemoryBytes != 0) {
            g_zVideo_pfnQueryTextureMemoryBytes(-1, &textureMemTotalBytes, &textureMemFreeBytes);
        }
        if (g_zVideo_pfnQueryDeviceVideoMemoryBytes != 0) {
            g_zVideo_pfnQueryDeviceVideoMemoryBytes(-1, &videoMemTotalBytes, &videoMemFreeBytes);
        }
    }

    char reportMessageBuffer[0x100];
    sprintf(reportMessageBuffer, "DirectDraw Error [%s] %s:%d\n", errorNameBuffer, sourceFile,
                 sourceLine);
    zError::ReportOld(0x400, sourceFile, sourceLine, reportMessageBuffer);
    return -1;
}
} // namespace zVideo_dd
