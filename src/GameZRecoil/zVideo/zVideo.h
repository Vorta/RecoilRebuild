#pragma once

#include "recoil/recoil_types.h"
#include <stdio.h>

#include <d3d.h>
#include <ddraw.h>
#include <windows.h>

#include "recoil/recoil_callconv.h"

struct zColorRgb;
struct zClass_CameraDataPartial;
struct zClass_NodePartial;
struct HudUiRect;
struct zTag4Partial;
struct zVec3;

extern "C" {
typedef void(*zVideo_ShutdownVideoSystemProc)();
typedef int(__fastcall *zVideo_StatusProc)(int modeIndex);
struct zVidRect32 {
    int left;
    int top;
    int right;
    int bottom;
};
struct zVideo_SurfaceStatePartial;
struct zVideo_TextureRecordPartial;
struct zVidImagePartial;
struct zVideo_XyzVertex;
struct zVideo_TexCoord;
struct zVideo_RenderClass;
struct zVideo_ColorRgbFloat;

struct zVideoFxColoredLineRecord {
    int x;
    int y;
    int width;
    int height;
    unsigned short color16;
    unsigned short reserved12;
    float alphaEnd;
    float alphaStart;
    int clipInset;
};
RECOIL_STATIC_ASSERT(sizeof(zVideoFxColoredLineRecord) == 0x20);

typedef void(__fastcall *zVideo_BltRectDirectProc)(
    zVidRect32 *srcRect,
    zVidRect32 *dstRect
);
typedef void(__fastcall *zVideo_ClearZBufferRectProc)(zVidRect32 *rect);
typedef void(__fastcall *zVideo_ClearSwSurfaceAndZBufferProc)(
    zVidRect32 *surfaceRect,
    zVidRect32 *zRect
);
typedef void(__fastcall *zVideo_ClearStateSurfaceAndZBufferProc)(
    zVidRect32 *rect,
    zVideo_SurfaceStatePartial *surfaceState
);
typedef int(__fastcall *zVideo_PaletteSetEntriesProc)(
    unsigned short firstEntry,
    unsigned short entryCount,
    PALETTEENTRY *entries
);
typedef int(__fastcall *zVideo_AdjustSurfacesProc)(
    zVidRect32 *srcRect,
    zVidRect32 *dstRect,
    int waitForPresent,
    int blitPrimaryToSwFirst
);
typedef int(__fastcall *zVideo_SurfaceStateProc)(zVideo_SurfaceStatePartial *surfaceState);
typedef int(__fastcall *zVideo_QueryMemoryBytesProc)(
    int flags,
    int *totalBytes,
    int *freeBytes
);
typedef int(__fastcall *zVideo_GetHwApiDeviceFeatureFlagsProc)(int deviceIndex);
typedef zVideo_TextureRecordPartial *(__fastcall *zVideo_CreateTextureRecordProc)(
    const char *textureName,
    zVidImagePartial *image,
    int useAlpha,
    int clampU,
    int clampV
);
typedef void(__fastcall *zVideo_DestroyTextureRecordProc)(
    zVideo_TextureRecordPartial *texture
);
typedef void(__fastcall *zVideo_TextureRecordReleaseUploadSurfaceRefProc)(
    zVideo_TextureRecordPartial *texture
);
typedef int(__fastcall *zVideo_TextureRecordLockUploadSurfaceProc)(
    zVideo_TextureRecordPartial *textureRecord,
    void **outPixels,
    int *outPitchBytes
);
typedef int(__fastcall *zVideo_TextureRecordUnlockUploadSurfaceProc)(
    zVideo_TextureRecordPartial *textureRecord
);
typedef void(__fastcall *zVideo_TextureRecordFinalizeUploadProc)(
    zVideo_TextureRecordPartial *textureRecord,
    void *reserved,
    zVidImagePartial *image
);
typedef void(*zVideo_ReleaseAllTextureUploadSurfacesProc)();
typedef void(*zVideo_UpdateFogColorProc)();
typedef void(*zVideo_FlushProc)();
typedef void(__fastcall *zVideo_ImageProc)(zVidImagePartial *image);
typedef IDirectDrawSurface3 *(__fastcall *zVideo_ImageLazyCreateSurfaceProc)(
    zVidImagePartial *image
);
typedef int(__fastcall *zVideo_ImageUploadPixelsProc)(
    zVidImagePartial *image,
    HDC *outHdc
);
typedef int(__fastcall *zVideo_ImageReleaseSurfaceProc)(
    zVidImagePartial *image,
    HDC hdc
);
typedef void(__fastcall *zVideo_BltImageRectProc)(
    zVidImagePartial *srcImage,
    int srcColorKeyEnable,
    zVidRect32 *srcRect,
    zVidRect32 *dstRect
);
typedef void(__fastcall *zVideo_SetFogEnableProc)(int enable);
typedef void(__stdcall *zVideo_SetFogFloatProc)(float value);
typedef void(__stdcall *zVideo_ApplyFogStateProc)(
    float fogStart,
    float fogEnd,
    float unused
);
typedef void(__fastcall *zVideo_SubmitPolyFlatColor16Proc)(
    zVideo_XyzVertex *vertices,
    unsigned int packedColor16,
    int alpha,
    int renderParam,
    int vertexCount,
    int queueMode
);
typedef void(__fastcall *zVideo_SubmitPolyGouraudColor16Proc)(
    zVideo_XyzVertex *vertices,
    unsigned int *packedColors16,
    int alpha,
    int renderParam,
    int vertexCount,
    int queueMode
);
typedef void(__fastcall *zVideo_SubmitPolyColorAttrProc)(
    zVideo_XyzVertex *vertices,
    unsigned int packedColor16,
    zVideo_ColorRgbFloat *baseColor,
    float *attr1,
    float *attr0,
    float *attr2,
    int alpha,
    int vertexCount,
    unsigned int renderParam,
    int queueMode
);
typedef void(__fastcall *zVideo_SubmitPolyRenderClassProc)(
    zVideo_XyzVertex *vertices,
    zVideo_TexCoord *texCoords,
    int vertexCount,
    zVideo_RenderClass *renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
);
typedef void(__fastcall *zVideo_SubmitPolygonProc)(
    zVideo_XyzVertex *vertices,
    zVideo_TexCoord *uvPairs,
    float *attr1,
    float *attr0,
    float *attr2,
    int vertexCount,
    zVideo_RenderClass *renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
);
typedef void(__fastcall *zVideo_DrawPointColor16Proc)(
    zVideo_XyzVertex *pointPos,
    unsigned int packedColor16,
    int pointCount
);

struct zVidD3DDriverRecordPartial {
    char m_deviceName[0x20];
    char m_deviceDescription[0x60];
    GUID *pD3DDeviceGuid;
    GUID m_d3dDeviceGuidStorage;
    D3DDEVICEDESC m_hwDesc;
};

struct zVideo_TextureRecordPartial {
    IDirectDrawSurface *m_uploadSurface;
    IDirectDrawSurface *m_textureSurface;
    IDirect3DTexture2 *m_texture;
    D3DTEXTUREHANDLE m_textureHandle;
    int m_alphaMode;
    D3DTEXTUREADDRESS m_uWrapMode;
    D3DTEXTUREADDRESS m_vWrapMode;
};

struct zVidImagePartial {
    int pixelCount;
    short width;
    short height;
    unsigned char headerFlagsByte;
    unsigned char formatFlagsPacked;
    unsigned char uPow2Shift;
    unsigned char vPow2Shift;
    short textureAddressFlagsPacked;
    short paletteMetaPacked;
    void *pixels;
    char *alphaMap;
    void *palette;
    float widthScale;
    char *queuedAlphaMap;
    int uShiftFrom20;
    int uMask;
    int vMaskFixed20;
    IDirectDrawSurface3 *surface;
    int pitchWords;
};

struct zVidHwApiDeviceRecordPartial {
    GUID *pDirectDrawGuid;
    GUID m_directDrawGuidStorage;
    char m_driverName[0x20];
    char m_driverDescription[0x60];
    int m_videoMemTotalBytes;
    int m_videoMemFreeBytes;
    int m_textureMemTotalBytes;
    int m_textureMemFreeBytes;
    int m_deviceFeatureFlags;
    int m_acceptedD3DDeviceCount;
    zVidD3DDriverRecordPartial m_d3dDrivers[4];
};

struct zVideo_SurfaceStatePartial {
    int width;
    int height;
    int pitch;
    int lockInfoValid;
    void *pixels;
    int locked;
    int pageLockActive;
    IDirectDrawSurface3 *surf;
};

struct zVideo_SurfaceLockVerifyArgs {
    unsigned int size;
    unsigned char reserved_04[0x18];
    int callerContext;
    unsigned char reserved_20[0x8];
};

struct zVideo_SurfaceLockVerifier;
struct zVideoFxPass3Config;
extern zVideoFxPass3Config g_zVideo_FxPass3ConfigLocal;

struct zVidTexturePackRecord {
    char name[0x20];
    int fileOffset;
    int paletteIndex;
};

struct zVidTexturePackHeader {
    int unknown_00;
    int fileFormat;
    int paletteTableCount;
    int recordCount;
    unsigned char unknown_10[0x08];
};

struct zVidTexturePackEntry {
    char filePath[0x80];
    FILE *fileHandle;
    zVidTexturePackHeader header;
    zVidTexturePackRecord *records;
    int paletteTableBaseIndex;
};

struct zVidPaletteRemapRecipe {
    float color0R;
    float color0G;
    float color0B;
    float color1R;
    float color1G;
    float color1B;
    float color0Strength;
    float color1Strength;
};

struct zVideo_SurfaceLockVerifier {
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID riid,
        void **object
    ) = 0;
    virtual ULONG STDMETHODCALLTYPE AddRef() = 0;
    virtual ULONG STDMETHODCALLTYPE Release() = 0;
    virtual HRESULT STDMETHODCALLTYPE Unknown0c() = 0;
    virtual HRESULT STDMETHODCALLTYPE VerifySurfaceState(
        zVideo_SurfaceLockVerifyArgs *args
    ) = 0;
};

struct zVideo_QuadBatchItemPartial {
    D3DTLVERTEX vertices[4];
};

struct zVideo_XyzVertex {
    float x;
    float y;
    float z;
};

struct zVideo_ColorRgbFloat {
    float r;
    float g;
    float b;
};

struct zVideo_TexCoord {
    float u;
    float v;
};

struct zVideo_RenderClass {
    unsigned char unknown_00[0x0c];
    D3DTEXTUREHANDLE textureHandle;
    D3DTEXTUREBLEND textureMapBlend;
    D3DTEXTUREADDRESS textureAddressU;
    D3DTEXTUREADDRESS textureAddressV;
};

struct zVideo_SortedPolyQueueEntry {
    int vertexCount;
    int renderClass;
    int renderParam;
    D3DTLVERTEX vertices[64];
};

struct zVideo_OverwriteQueueEntry {
    int type;
    int vertexCount;
    int renderClass;
    int renderParam;
    D3DTLVERTEX vertices[64];
};

/*
 * BN models the Direct3D render-state cache at 0x633408 as one 0x28-byte BSS
 * record shared by the sorted, overwrite, and solid-quad flush paths.
 */
struct zVideo_D3DRenderStateCacheLive {
    int alphaBlendEnable;
    int shadeMode;
    D3DTEXTUREBLEND textureMapBlend;
    D3DTEXTUREADDRESS textureAddressU;
    D3DTEXTUREADDRESS textureAddressV;
    int unknown_14;
    int unknown_18;
    D3DTEXTUREHANDLE textureHandle;
    int zWriteEnable;
    int unknown_24;
};

struct zVideo_PixelPackParams {
    int rBits;
    int gBits;
    int bBits;
    unsigned int rMask;
    unsigned int gMask;
    unsigned int bMask;
    int packedBase;
    int sumMinus8;
    int bShiftTo8;
    int rMaskShifted;
    int gMaskShifted;
    int bMaskShifted;
};

extern zVideo_PixelPackParams g_zVideo_PixelPack;
extern int g_zVideo_TexturePixelPack_RBits;
extern int g_zVideo_TexturePixelPack_GBits;
extern int g_zVideo_TexturePixelPack_BBits;
extern int g_zVideo_TexturePixelPack_ABits;
extern unsigned int g_zVideo_TexturePixelPack_RMask;
extern unsigned int g_zVideo_TexturePixelPack_GMask;
extern unsigned int g_zVideo_TexturePixelPack_BMask;
extern unsigned int g_zVideo_TexturePixelPack_AMask;
extern int g_zVideo_TexturePixelPack_RGBBitsTotalMinus8;
extern int g_zVideo_TexturePixelPack_GBBitsTotalMinus8;
extern int g_zVideo_TexturePixelPack_BShiftTo8;
extern int g_zVideo_TexturePixelPack_RGBBitsTotal;
extern int g_zVideo_TexturePixelPack_RMaskShifted;
extern int g_zVideo_TexturePixelPack_GMaskShifted;
extern int g_zVideo_TexturePixelPack_BMaskShifted;
extern int g_zVideo_TexturePixelPack_NonRgbMaskShifted;
extern int g_zVid_PaletteRemapRecipeCount;
extern zVidPaletteRemapRecipe *g_zVid_PaletteRemapRecipes;
extern int g_zVideo_RendererType;
extern int g_zVideo_ActiveRendererPath;
extern int g_zVideo_FrameTick;
extern zClass_CameraDataPartial *g_zVideo_pActiveViewContext;
extern zTag4Partial g_zVideo_ActiveViewVariantTag;
extern float g_zVideo_ProjectClipLeft;
extern float g_zVideo_ProjectClipTop;
extern float g_zVideo_ProjectClipRight;
extern float g_zVideo_ProjectClipBottom;
extern int gVideo_resolutionMenuValid;
extern unsigned char g_zVideo_PaletteBrightnessLevel;
extern unsigned int g_zVideo_ClearColorPacked16;
extern int g_zVideo_ClearScreenBufferEnabled;
extern int g_zVid_CachedClientRectUpdateMask;
extern int g_zVideo_IsInitialized;
extern int g_zVideo_AdjustSurfacesDisableGate;
extern int g_zVideo_FullscreenOption;
extern int g_zVideo_PrimaryHasAttachedBackbuffer;
extern int g_zVideo_UseHalfResBackbuffer;
extern int g_zVideo_HalfResAdjustMode;
extern int g_zVideo_SoftwareModeHotkeyEnabled;
extern int g_zVideo_CachedFogModeLightState;
extern int g_zVideo_CachedFogEnableRenderState;
extern float g_zVideo_CachedFogStartLightStateValue;
extern float g_zVideo_CachedFogEndLightStateValue;
extern float g_zVideo_FogColorPendingR255;
extern float g_zVideo_FogColorPendingG255;
extern float g_zVideo_FogColorPendingB255;
extern float g_zVideo_FogTargetColorR255;
extern float g_zVideo_FogTargetColorG255;
extern float g_zVideo_FogTargetColorB255;
extern float g_zVideo_FogColorAppliedR255;
extern float g_zVideo_FogColorAppliedG255;
extern float g_zVideo_FogColorAppliedB255;
extern int g_zVideo_PendingDitherEnable;
extern float g_zVideo_InverseZTolerancePending;
extern int g_zVideo_D3DAppendFanCloseVertexPending;
extern int g_zVideo_PendingWireframeState;
extern int g_zVideo_D3DSceneDepth;
extern int g_zVid_AcceptedHardwareRendererCount;
extern int g_zVideo_NumAcceptedDirectDrawDevices;
extern int g_zVideo_DirectDrawEnumOrdinal;
extern int g_zVid_TexturePackLoadState;
extern int g_zVid_BuiltinTexturePackCount;
extern zVidTexturePackEntry *g_zVid_BuiltinTexturePacks;
extern int g_zVid_TexturePackCount;
extern zVidTexturePackEntry *g_zVid_TexturePacks;
extern int g_zVid_PaletteRemapVariantTableCount;
extern unsigned short **g_zVid_PaletteRemapVariantTables;
extern int *ZOPT_VIDEO_MODE;
extern int *ZOPT_VIDEO_ACCELERATION;
extern int *ZOPT_HW_API;
extern DDCAPS g_zVideo_DDrawCapsHal;
extern DDCAPS g_zVideo_DDrawCapsHel;
extern zVideo_TextureRecordPartial *g_zImage_DefaultTextureRecord;
extern char g_zVideo_PalettePathBuffer[0x100];
extern PALETTEENTRY g_zVideo_PaletteFileEntries[0x100];
extern PALETTEENTRY g_zVideo_SystemPaletteEntries[0x100];
extern zVideo_StatusProc g_zVideo_pfnOpenVideoMode;
extern zVideo_ShutdownVideoSystemProc g_zVideo_pfnShutdownVideoSystem;
extern zVideo_PaletteSetEntriesProc g_zVideo_pfnPaletteSetEntries;
extern zVideo_StatusProc g_zVideo_pfnSetVideoMode;
extern zVideo_AdjustSurfacesProc g_zVideo_pfnAdjustSurfaces;
extern zVideo_SurfaceStateProc g_zVideo_pfnLockSurfaceState;
extern zVideo_SurfaceStateProc g_zVideo_pfnUnlockSurfaceState;
extern zVideo_ClearZBufferRectProc g_zVideo_pfnClearZBufferRect;
extern zVideo_QueryMemoryBytesProc g_zVideo_pfnQueryDeviceVideoMemoryBytes;
extern zVideo_QueryMemoryBytesProc g_zVideo_pfnQueryTextureMemoryBytes;
extern zVideo_BltRectDirectProc g_zVideo_pfnBltSwToPrimaryRectDirect;
extern zVideo_BltRectDirectProc g_zVideo_pfnBltPrimaryToSwRectDirect;
extern zVideo_BltImageRectProc g_zVideo_pfnBltSwToPrimaryRect;
extern zVideo_ClearSwSurfaceAndZBufferProc g_zVideo_pfnClearSwSurfaceAndZBuffer;
extern zVideo_ClearStateSurfaceAndZBufferProc g_zVideo_pfnClearStateSurfaceAndZBuffer;
extern zVideo_UpdateFogColorProc g_zVideo_pfnUpdateFogColor;
extern zVideo_CreateTextureRecordProc g_zVideo_pfnCreateTextureRecord;
extern zVideo_TextureRecordLockUploadSurfaceProc g_zVideo_pfnTextureRecordLockUploadSurface;
extern zVideo_TextureRecordUnlockUploadSurfaceProc g_zVideo_pfnTextureRecordUnlockUploadSurface;
extern zVideo_TextureRecordReleaseUploadSurfaceRefProc
    g_zVideo_pfnTextureRecordReleaseUploadSurfaceRef;
extern zVideo_TextureRecordFinalizeUploadProc g_zVideo_pfnTextureRecordFinalizeUpload;
extern zVideo_DestroyTextureRecordProc g_zVideo_pfnTextureRecordDestroy;
extern zVideo_ReleaseAllTextureUploadSurfacesProc
    g_zVideo_pfnTextureRecordReleaseAllUploadSurfaces;
extern zVideo_ImageProc g_zVideo_pfnImageEnsureSurfaceForCurrentDevice;
extern zVideo_FlushProc g_zVideo_pfnFlushSortedPolys;
extern zVideo_FlushProc g_zVideo_pfnFlushOverwritePolys;
extern zVideo_FlushProc g_zVideo_pfnFlushQuadBatch;
extern zVideo_SubmitPolyFlatColor16Proc g_zVideo_pfnSubmitPolyFlatColor16;
extern zVideo_SubmitPolyColorAttrProc g_zVideo_pfnSubmitPolyColorAttr;
extern zVideo_SubmitPolyRenderClassProc g_zVideo_pfnSubmitPolyRenderClass;
extern zVideo_SubmitPolygonProc g_zVideo_pfnSubmitPolygon;
extern zVideo_SubmitPolygonProc g_zVideo_pfnSubmitPolygonLit;
extern zVideo_DrawPointColor16Proc g_zVideo_pfnDrawPointColor16;
extern zVidHwApiDeviceRecordPartial g_zVideo_HwApiDeviceTable[4];
extern zVidHwApiDeviceRecordPartial *g_zVideo_pSelectedHwApiDeviceRecord;
extern zVidD3DDriverRecordPartial *g_zVideo_pSelectedD3DDeviceInfo;
extern D3DDEVICEDESC g_zVideo_D3DHalDeviceDesc;
extern D3DDEVICEDESC g_zVideo_D3DHelDeviceDesc;
extern D3DMATERIALHANDLE g_zVideo_D3DMaterialHandle;
extern int g_zVideo_QuadBatchCount;
extern zVideo_QuadBatchItemPartial g_zVideo_QuadBatchItemsBase[16];
extern D3DTLVERTEX g_zVideo_D3DSubmitTempVertices[64];
extern int g_zVideo_SortedPolyDrawOrder[256];
extern zVideo_SortedPolyQueueEntry g_zVideo_SortedPolyQueueBase[256];
extern zVideo_OverwriteQueueEntry g_zVideo_OverwriteQueueBase[0x180];
extern int g_zVideo_SortedPolyQueueCount;
extern int g_zVideo_OverwriteQueueCount;
extern zVideo_D3DRenderStateCacheLive g_zVideo_D3DRenderStateCache;
extern int g_zVideo_D3DColorNormalizeChannelIndex;
extern float g_zVideo_D3DColorAttrBiasR;
extern float g_zVideo_D3DColorAttrBiasG;
extern float g_zVideo_D3DColorAttrBiasB;
extern IDirect3DMaterial2 *g_zVideo_pD3DMaterial2;
extern IDirect3DViewport2 *g_zVideo_pD3DViewport2;
extern IDirect3DDevice2 *g_zVideo_pD3DDevice;
extern IDirect3D2 *g_zVideo_pD3D2;
extern IDirectDrawClipper *g_zVideo_pClipper;
extern IDirectDraw2 *g_zVideo_pDirectDraw2;
extern IDirectDrawSurface3 *g_zVideo_pZBufferSurface;
extern IDirectDrawSurface *g_zVideo_pZBufferAttachSurface;
extern IDirectDrawSurface3 *g_zVideo_pPageUnlockSurface;
extern zVideo_SurfaceLockVerifier *g_zVideo_pSurfaceLockVerifier;
extern int g_zVideo_SurfaceLockVerifyContext;
extern unsigned char g_zVideo_SurfaceLockVerifyFlags;
extern zVideo_SurfaceStatePartial g_zVideo_SwSurfaceState;
extern zVideo_SurfaceStatePartial g_zVideo_PrimarySurfaceState;
extern zVideo_SurfaceStatePartial g_zVideo_DisplayModeSurfaceState;
extern zVideo_SurfaceStatePartial g_zVideo_SurfaceStateSwapScratch;
extern zVidRect32 g_zVideo_PrimarySurfaceRectScratch;
extern int g_zVideo_DisplayModeBpp;
extern int g_zVid_NoiseByteTableSize;
extern unsigned char *g_zVid_NoiseByteTable;
extern unsigned short *g_zVideo_FxPass3_ScratchPixels16;
extern unsigned short *g_zVideo_FxSurfacePixels16;
extern int g_zVideo_FxSurfaceWidth;
extern int g_zVideo_FxSurfaceHeight;
extern int g_zVideo_FxSurfacePitchBytes;
extern int g_zVideo_FxSurfacePitchPixels16;
extern int g_zVideo_FxPass3_ScratchOffsetX;
extern int g_zVideo_FxPass3_ScratchOffsetY;
extern int g_zVideo_FxPass3_ClipMinX;
extern int g_zVideo_FxPass3_ClipMinY;
extern int g_zVideo_FxPass3_ClipMaxX;
extern int g_zVideo_FxPass3_ClipMaxY;
extern zVideo_ImageUploadPixelsProc g_zVideo_pfnImageUploadPixelsToSurface;
extern zVideo_ImageReleaseSurfaceProc g_zVideo_pfnImageReleaseSurface;
extern zVideo_GetHwApiDeviceFeatureFlagsProc g_zVideo_pfnGetHwApiDeviceFeatureFlags;
extern IDirectDrawPalette *g_zVideo_pDDPalette;
extern HWND g_zVideo_hWnd;
extern RECT g_zVideo_CachedClientRectScreen;

unsigned int __fastcall zVid_PackColorRGB(
    unsigned char red,
    unsigned char green,
    unsigned int blue
);
unsigned int __fastcall zVid_PackColor00RRGGBB(unsigned int color00RRGGBB);
unsigned short __fastcall zVid_PackColorRgbFloats(zVideo_ColorRgbFloat *color);
void __fastcall zVideo_SetClearColorPacked16(unsigned int packedColor16);
void __fastcall zVideo_SetPendingFogTargetColorFromRgb01(
    zVideo_ColorRgbFloat *color
);
void __fastcall zVideo_SetActiveViewContext(
    zClass_CameraDataPartial *viewContext
);
int __fastcall zVideo_sw_RenderFrame(
    zClass_NodePartial *camera,
    int updateFxPass3Local
);
void __fastcall zVideo_UpdateProjectionStateFromCameraData(
    zClass_CameraDataPartial *cameraData
);
int __fastcall zVideo_FrustumTestSphereClipMask(
    zVec3 *sphereCenter,
    int *clipMaskInOut,
    float radius
);
void zVideo_RestoreIconicFullscreenWindowIfNeeded();
}

namespace zVid {
void __fastcall SetAccelerationOption(int accelerationOption);
void __fastcall SetHwApiOption(int hwApiOption);
int GetAccelerationOption();
int GetHwApiOption();
int GetAcceptedDirectDrawDeviceCount();
int GetAcceptedHardwareRendererCount_Cached();
int HasAcceptedHardwareRenderer();
int GetTexturePackLoadState();
void __fastcall SetTexturePackLoadState(int texturePackLoadState);
int GetVideoModeIndexFromOptions();
void __fastcall SetVideoModeIndex(int modeIndex);
int __fastcall QueryDeviceVideoMemoryBytes(
    int deviceIndexOrMinus1,
    int *totalBytes,
    int *freeBytes
);
int __fastcall QueryTextureMemoryBytes(
    int deviceIndexOrMinus1,
    int *totalBytes,
    int *freeBytes
);
int QueryCachedClientRectUpdateMaskIf3dfx();
void UpdateCachedClientRectIfUpdateMaskEnabled();
void __fastcall SetCachedClientRectUpdateMask(int mask);
char *GetSelectedHwApiDescriptionOrDefault();
char *GetSelectedD3DDeviceNameOrDefault();
char *__fastcall GetHwApiDescription(int index);
char *__fastcall GetHwApiDriverName(int index);
void Noise_InitBuffers();
void Noise_ShutdownBuffers();
void __fastcall DrawNoiseRect(
    zVidRect32 *rectOrNull,
    double intensity
);
int InitFrameScratchBuffers();
int ShutdownFrameScratchBuffers();
} // namespace zVid

namespace zVideo_FxSurface {
void __fastcall ApplyBlueTintRect(zVidRect32 *rectOrNull);
void __fastcall ApplyGreenMaskRect(zVidRect32 *rectOrNull);
void __fastcall DrawAlphaBlendedLine(
    zVidRect32 *clipRect,
    int x1,
    int y1,
    int x0,
    int y0,
    unsigned int color16,
    float alphaEnd,
    float alphaStart,
    int clipInset
);
void __fastcall DrawColoredLinesBatch(
    zVideoFxColoredLineRecord *lines,
    int count,
    zVidRect32 *clipRectOrNull
);
} // namespace zVideo_FxSurface

namespace zVideo_buff {
int __fastcall ClipCoordToRange(
    int *coordPtr,
    int minCoord,
    int maxCoord
);
zVidImagePartial *__fastcall CopySurfaceRectToImage(
    int sourceSelector,
    zVidRect32 *rect,
    zVidImagePartial *imageOrNull
);
void __fastcall BltSourceToPrimaryClipped(
    zVidImagePartial *srcImage,
    int dstX,
    int dstY,
    int srcColorKeyEnable,
    zVidRect32 *srcRect
);
} // namespace zVideo_buff

namespace zVideo {
void __fastcall SetFogColorFromRgb01(zVideo_ColorRgbFloat *color);
void __fastcall SetFogTargetColorFromRgb01(zVideo_ColorRgbFloat *color);
void CommitFogColorIfChanged();
void CommitFogTargetColorIfChanged();
void __fastcall PixelPack_SetupFromMasks(
    int redBits,
    int greenBits,
    int blueBits,
    unsigned int redMask,
    unsigned int greenMask,
    unsigned int blueMask
);
void __fastcall TexturePixelPack_SetupFromMasks(
    int redBits,
    int greenBits,
    int blueBits,
    int alphaBits,
    unsigned int redMask,
    unsigned int greenMask,
    unsigned int blueMask,
    unsigned int alphaMask
);
void __fastcall PixelPack_GetRgbBits(
    int *outRBits,
    int *outGBits,
    int *outBBits
);
void __fastcall PixelPack_GetRgbMasks(
    unsigned int *outRMask,
    unsigned int *outGMask,
    unsigned int *outBMask
);
void __fastcall PixelPack_GetPackingParams(
    int *outPackedBase,
    int *outSumMinus8,
    int *outBShiftTo8
);
int __fastcall SetRendererTypeAndActivePath(int rendererType);
int __fastcall SetHalfResAdjustMode(int mode);
void __fastcall HandleSoftwareModeHotkeyCommand(int commandId);
zVidRect32 *GetPrimarySurfaceRectScratch();
void *GetSwSurfacePixels();
int GetSwSurfaceWidth();
int GetSwSurfaceHeight();
int GetSwSurfacePitch();
int GetSwSurfaceLockedFlag();
void *GetPrimarySurfacePixels();
int GetPrimarySurfaceWidth();
int GetPrimarySurfaceHeight();
int GetPrimarySurfacePitch();
int GetDisplayModeBpp();
int __fastcall LoadPaletteFileAndApplyBrightness(const char *palettePath);
int __fastcall ApplyBrightnessToPaletteEntries(PALETTEENTRY *paletteEntries);
int __fastcall Init_ApplyModeIndex(int modeIndex);
void __fastcall Init_SetSurfaceGeometryFromModeIndex(int modeIndex);
int __fastcall SetVideoMode(int modeIndex);
int __fastcall InitVideoSystem(
    HWND hWnd,
    int rendererBackend,
    int fullscreen,
    int modeIndex
);
void __fastcall CallClearSwSurfaceAndZBuffer(
    zVidRect32 *surfaceRect,
    zVidRect32 *zRect
);
void __fastcall CallClearPrimarySurfaceAndZBuffer(zVidRect32 *rect);
int __fastcall ExchangeClearScreenBufferEnabled(int enable);
int GetClearScreenBufferEnabled();
int Dispatch_LockDisplayModeSurfaceState();
int Dispatch_UnlockDisplayModeSurfaceState();
int Dispatch_UnlockSwSurfaceState();
int Dispatch_UnlockPrimarySurfaceState();
void __fastcall Fx_SetSurfaceState(
    void *pixels,
    int width,
    int height,
    int pitchBytes
);
void __fastcall FxPass3_CopySurfacePixelToScratchClipped(
    int dstDx,
    int dstDy,
    int srcDx,
    int srcDy
);
void __fastcall FxPass3_ApplyToCurrentSurface(
    int centerX,
    int centerY,
    int currentRadius,
    int maxRadius,
    int extent,
    float sinFreq,
    float sinPhase,
    zVidRect32 *clipRectOrNull
);
void __fastcall buff_BlurRegionCombined(
    zVidRect32 *rectOrNull,
    int mode
);
void __fastcall buff_BlurRegionVertical(
    zVidRect32 *rectOrNull,
    int mode
);
void __fastcall buff_BlurRegionHorizontal(
    zVidRect32 *rectOrNull,
    int mode
);
void __fastcall buff_BlurRegionByMode(
    zVidRect32 *rectOrNull,
    int mode
);
void __fastcall zVideoFxPass3Config_UpdateLocal(
    zVideoFxPass3Config *config,
    float deltaTime
);
void __fastcall zVideoFxPass3Config_SetPrimaryElementParamsLocal(
    zVideoFxPass3Config *config,
    unsigned int packedColor,
    double primaryAlpha
);
void __fastcall FxPass3_SetPrimaryElementParamsLocal(
    unsigned int packedColor,
    double primaryAlpha
);
void __fastcall zVideoFxPass3Config_QueueElementLocal(
    zVideoFxPass3Config *config,
    int rectLeftPixels,
    int rectTopPixels,
    int currentRadiusPixels,
    int maxRadiusPixels,
    int extentPixels,
    float sinFreq,
    float sinPhase
);
void __fastcall FxPass3_QueueElementLocal(
    int rectLeftPixels,
    int rectTopPixels,
    int currentRadiusPixels,
    int maxRadiusPixels,
    int extentPixels,
    float sinFreq,
    float sinPhase
);
void __fastcall FxPass3_QueuePrimitive(
    void *primitive,
    int width,
    int height,
    int pitchBytes
);
void __fastcall FxPass3_SetInputRectByIndex(
    int index,
    HudUiRect *rectOrNull
);
void __fastcall FxPass3_UpdateLocal(float deltaTime);
void RunPostprocessOnSwBuffer();
int RunPostprocessOnPrimaryBuffer();
int __fastcall AdjustSurfacesIfEnabled(
    zVidRect32 *srcRect,
    zVidRect32 *dstRect,
    int waitForPresent,
    int blitPrimaryToSwFirst
);
void __fastcall BindRendererDispatch(
    int rendererType,
    int fullscreenOption
);
void __fastcall CommitHwApiDeviceSelection(int hwApiIndex);
int __fastcall SelectHwApiDeviceOrFallback(int hwApiIndex);
int ReturnSuccessStub();
int ModuleInit();
int ShutdownVideoSystem();
int UpdateCachedClientRectScreenCoords();
void AtExitReleaseAllInterfacesAndSurfaces();
} // namespace zVideo

namespace zVid_Image {
extern zVidImagePartial g_zImage_DefaultImage;

zVidImagePartial *Create();
int __fastcall Destroy(zVidImagePartial *image);
int __fastcall ReleaseIfNotDefault(zVidImagePartial *image) throw();
void __fastcall ReleaseOwnedBuffers(zVidImagePartial *image);
void __fastcall CalcPow2ScratchFields(zVidImagePartial *image);
int __fastcall QueryBytesPerPixel(zVidImagePartial *image);
void __fastcall ClearZeroAlphaPixelsInPlace(zVidImagePartial *image);
int __fastcall SetHeaderFlagsByte(
    zVidImagePartial *image,
    unsigned char flags
);
int __fastcall SetFormatCode(
    zVidImagePartial *image,
    unsigned char formatCode
);
int __fastcall SetSize(
    zVidImagePartial *image,
    short width,
    short height
);
int __fastcall QueryPixelDataBytes(zVidImagePartial *image);
int __fastcall ReadHeader(
    FILE *file,
    zVidImagePartial *image
);
int __fastcall ReadData(
    FILE *file,
    zVidImagePartial *image,
    int bytesPerPixel = 0
);
zVidImagePartial *__fastcall ReadFromFile(FILE *file);
void __fastcall ResampleSquare(
    zVidImagePartial *image,
    int sideLength
);
void __fastcall BlitToActiveTarget(
    zVidImagePartial *image,
    int dstX,
    int dstY,
    int clipFlags,
    zVidRect32 *srcRect
);
void __fastcall BlitToFramebufferClipped(
    zVidImagePartial *image,
    int dstX,
    int dstY,
    int clipFlags,
    zVidRect32 *srcRect
);
} // namespace zVid_Image

namespace zVid_PaletteRemap {
int __fastcall FindRecipeIndex(zVidPaletteRemapRecipe *recipe);
void __fastcall ApplyRecipeToPaletteVariant(
    zVidPaletteRemapRecipe *recipe,
    unsigned short *sourceColors,
    int colorCount,
    int variantIndex,
    unsigned short *destColors
);
} // namespace zVid_PaletteRemap

extern "C" int __fastcall zVid_Image_SetPixels(
    zVidImagePartial *image,
    void *pixels,
    char *alphaMap
);

extern "C" zVidImagePartial *__fastcall zVideo_buff_CaptureSurfaceToImage(
    int sourceSelector
);
extern "C" unsigned short *__fastcall
zVid_PaletteRemap_BuildAllRecipeVariantsForPalette(
    unsigned short *palette,
    int colorCount
);
extern "C" int __fastcall zVid_PaletteRemap_BuildPaletteVariant(
    zVidPaletteRemapRecipe *recipe
);
extern "C" int __fastcall zVid_PaletteRemap_FindRecipeIndexFromRgb(
    zColorRgb *rgb
);
extern "C" FILE *__fastcall zVid_TexturePackEntry_LoadFromFile(
    zVidTexturePackEntry *entry
);
extern "C" void zVid_TexturePack_EnsureDefaultImagePackLoaded();
extern "C" RECOIL_NO_GS void zVid_TexturePack_EnsureBuiltinTexturePacksLoaded();
extern "C" zVidImagePartial *__fastcall zVid_TexturePack_LoadImageByName(
    const char *imageName
);
extern "C" zVidImagePartial *__fastcall
zVid_TexturePack_LoadBuiltinImageByName(const char *imageName);

namespace zVid_TexturePack {
void ShutdownBuiltinPacks();
void Shutdown();
} // namespace zVid_TexturePack

namespace zVideo_dd {
int GetAcceptedDirectDrawDeviceCountCached();
BOOL CALLBACK EnumDirectDrawDeviceCallback(
    GUID *guid,
    LPSTR driverDescription,
    LPSTR driverName,
    LPVOID context
);
HRESULT CALLBACK EnumDirect3DDeviceCallback(
    GUID *guid,
    LPSTR deviceDescription,
    LPSTR deviceName,
    D3DDEVICEDESC *hwDesc,
    D3DDEVICEDESC *helDesc,
    LPVOID context
);
int PrepareWindowForMode();
int __fastcall OpenVideoMode(int modeIndex);
int RunDirectDrawDeviceEnumeration();
void StartupEnumerateAndDefaultSelect();
int ShutdownVideoSystem();
int __fastcall LockDirectDrawSurface(
    IDirectDrawSurface3 *surface,
    DDSURFACEDESC *outLockedSurfaceDesc
);
int __fastcall UnlockDirectDrawSurface(IDirectDrawSurface3 *surface);
int __fastcall LockSurface_WaitRestore(
    IDirectDrawSurface3 *surface,
    DDSURFACEDESC *lockedDescOut
);
int __fastcall UnlockSurface_WaitRestore(IDirectDrawSurface3 *surface);
int __fastcall LockSurfaceState(zVideo_SurfaceStatePartial *surfaceState);
int __fastcall UnlockSurfaceState(zVideo_SurfaceStatePartial *surfaceState);
IDirectDrawSurface3 *__fastcall Image_LazyCreateBackingSurface(
    zVidImagePartial *image,
    unsigned int ddsCapsFlags
);
int __fastcall Image_PopulateSurfaceFromHeapPixels(zVidImagePartial *image);
IDirectDrawSurface3 *__fastcall Image_LazyCreateVideoMemorySurface(
    zVidImagePartial *image
);
void __fastcall Image_EnsureSurfaceForCurrentDevice(zVidImagePartial *image);
int __fastcall Image_UploadPixelsToSurface(
    zVidImagePartial *image,
    HDC *outHdc
);
int __fastcall Image_ReleaseSurface(
    zVidImagePartial *image,
    HDC hdc
);
void __fastcall BltSwToPrimaryRectDirect(
    zVidRect32 *srcRect,
    zVidRect32 *dstRect
);
void __fastcall BltPrimaryToSwRectDirect(
    zVidRect32 *srcRect,
    zVidRect32 *dstRect
);
int __fastcall PresentDisplayModeSurface(
    zVidRect32 *srcRect,
    zVidRect32 *dstRect,
    int waitForPresent,
    int skipSurfaceStateSwap
);
void __fastcall BltSwToPrimaryRect(
    zVidImagePartial *srcImage,
    int srcColorKeyEnable,
    zVidRect32 *srcRect,
    zVidRect32 *dstRect
);
void __fastcall ZBuffer_DepthFillRect(zVidRect32 *dstRect);
void __fastcall ClearScreenAndZBufferRect(
    zVidRect32 *dstRect,
    zVideo_SurfaceStatePartial *colorSurfaceState
);
void __fastcall ClearSwBackbufferAndZBufferRects(
    zVidRect32 *colorRect,
    zVidRect32 *zRect
);
void FlipToGDIIfAttached();
int SetDisplayMode();
int __fastcall SetVideoMode(int modeIndex);
int VerifyFullscreenSurfaceLocks();
int RestoreDisplaySurfaces();
int __fastcall InitFullscreenSoftwarePixelPack(
    IDirectDrawSurface3 *displaySurface
);
HRESULT __fastcall CreateSurface3FromDesc(
    IDirectDraw2 *directDraw,
    DDSURFACEDESC *desc,
    IDirectDrawSurface3 **outSurface,
    int reserved
);
int CreateFullscreenSurfacesForRenderer();
int CreateHalfResBackbufferSurfaces();
int CreateFullscreenSoftwareSurfaces();
int CreateFullscreenHardwareSurfaces();
int __fastcall GetHwApiDeviceFeatureFlags(int deviceIndex);
int CreateDirectDraw2ForSelectedDevice();
int __fastcall EnumerateDirect3DDevicesForRecord(
    zVidHwApiDeviceRecordPartial *entry
);
int ReleaseAllInterfacesAndSurfaces();
void __fastcall VerifySurfaceStateLocking(int callerContext);
void TeardownVideoSubsystem();
int __fastcall ReportError(
    int hresult,
    const char *sourceFile,
    int sourceLine
);
int __fastcall PaletteSetEntries(
    unsigned short firstEntry,
    unsigned short entryCount,
    PALETTEENTRY *entries
);
} // namespace zVideo_dd

namespace zVideo_dd3d {
void __fastcall CallClearZBufferRect(zVidRect32 *rect);
void __fastcall SetPendingWireframeState(int pendingWireframeState);
void __fastcall SetPendingDitherEnable(int enabled);
int BeginSceneAndFlushPendingRenderStates();
int EndScene();
int __fastcall PresentDisplayModeSurface(
    zVidRect32 *srcRect,
    zVidRect32 *dstRect,
    int waitForPresent,
    int blitPrimaryToSwFirst
);
zVideo_TextureRecordPartial *__fastcall CreateTextureRecord(
    const char *textureName,
    zVidImagePartial *image,
    int useAlpha,
    int clampU,
    int clampV
);
int CreateDeviceState();
void __fastcall SetFogEnable(int enable);
void __stdcall SetFogStart(float fogStart);
void __stdcall SetFogEnd(float fogEnd);
void __stdcall ApplyFogStateFromGlobals(
    float fogStart,
    float fogEnd,
    float unused
);
void UpdateFogColor();
void __stdcall SetQuadBatchDepthAndRhw(float depthAndRhw);
void __fastcall SubmitPolyFlatColor16(
    zVideo_XyzVertex *vertices,
    unsigned int packedColor16,
    int alpha,
    int renderParam,
    int vertexCount,
    int queueMode
);
void __fastcall SubmitPolyGouraudColor16(
    zVideo_XyzVertex *vertices,
    unsigned int *packedColors16,
    int alpha,
    int renderParam,
    int vertexCount,
    int queueMode
);
void __fastcall SubmitPolyColorAttr(
    zVideo_XyzVertex *vertices,
    unsigned int packedColor16,
    zVideo_ColorRgbFloat *baseColor,
    float *attr1,
    float *attr0,
    float *attr2,
    int alpha,
    int vertexCount,
    unsigned int renderParam,
    int queueMode
);
void __fastcall SubmitPolyRenderClass(
    zVideo_XyzVertex *vertices,
    zVideo_TexCoord *texCoords,
    int vertexCount,
    zVideo_RenderClass *renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
);
void __fastcall SubmitPolygon(
    zVideo_XyzVertex *vertices,
    zVideo_TexCoord *uvPairs,
    float *attr1,
    float *attr0,
    float *attr2,
    int vertexCount,
    zVideo_RenderClass *renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
);
void __fastcall SubmitPolygonLit(
    zVideo_XyzVertex *vertices,
    zVideo_TexCoord *uvPairs,
    float *attr1,
    float *attr0,
    float *attr2,
    int vertexCount,
    zVideo_RenderClass *renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
);
void __fastcall DrawPointColor16(
    zVideo_XyzVertex *pointPos,
    unsigned int packedColor16,
    int pointCount
);
void __fastcall QueueSolidQuad(
    unsigned int packedColor16,
    zVidRect32 *clipRect,
    double alpha
);
void FlushSortedPolys();
void FlushQuadBatch();
void FlushOverwritePolys();
int __fastcall FloorPowerOfTwo(int value);
zVideo_TextureRecordPartial *TextureRecord_Create();
int __fastcall TextureRecord_LockUploadSurface(
    zVideo_TextureRecordPartial *textureRecord,
    void **outPixels,
    int *outPitchBytes
);
void __fastcall ConvertImagePixelsForTexture(
    unsigned short *dstPixels,
    zVidImagePartial *image,
    int pitchBytes,
    int useAlpha
);
int __fastcall UploadImageToSurface(
    IDirectDrawSurface *uploadSurface,
    zVidImagePartial *image,
    int useAlpha
);
int __fastcall TextureRecord_UnlockUploadSurface(
    zVideo_TextureRecordPartial *textureRecord
);
void __fastcall TextureRecord_ReleaseUploadSurfaceRef(
    zVideo_TextureRecordPartial *textureRecord
);
void __fastcall TextureRecord_FinalizeUpload(
    zVideo_TextureRecordPartial *textureRecord,
    void *reserved,
    zVidImagePartial *image
);
void __fastcall TextureRecord_Destroy(
    zVideo_TextureRecordPartial *textureRecord
);
} // namespace zVideo_dd3d

namespace zVideoD3D {
int SceneEnter();
int SceneLeave();
} // namespace zVideoD3D
