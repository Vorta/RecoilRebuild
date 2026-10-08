#pragma once

#include "recoil/recoil_types.h"
#include <stdio.h>

#include <d3d.h>
#include <ddraw.h>
#include <windows.h>

#include "recoil/recoil_callconv.h"

#include "GameZRecoil/include/zClass.h"
struct CZCameraDataPartial;
struct CZNodePartial;
struct HudUiRect;
struct zTag4Partial;
struct zVec3;

extern "C" {
typedef void (*zVideo_ShutdownVideoSystemProc)();
typedef int(__fastcall* zVideo_StatusProc)(int modeIndex);
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

typedef void(__fastcall* zVideo_BltRectDirectProc)(zVidRect32* srcRect, zVidRect32* dstRect);
typedef int(__fastcall* zVideo_ClearZBufferRectProc)(zVidRect32* rect);
typedef int(__fastcall* zVideo_ClearSwSurfaceAndZBufferProc)(zVidRect32* surfaceRect, zVidRect32* zRect);
typedef int(__fastcall* zVideo_ClearStateSurfaceAndZBufferProc)(
    zVidRect32* rect,
    zVideo_SurfaceStatePartial* surfaceState
);
typedef int(__fastcall* zVideo_PaletteSetEntriesProc)(
    unsigned short firstEntry,
    unsigned short entryCount,
    PALETTEENTRY* entries
);
typedef int(__fastcall* zVideo_AdjustSurfacesProc)(
    zVidRect32* srcRect,
    zVidRect32* dstRect,
    int waitForPresent,
    int blitPrimaryToSwFirst
);
typedef int(__fastcall* zVideo_SurfaceStateProc)(zVideo_SurfaceStatePartial* surfaceState);
typedef int(__fastcall* zVideo_QueryMemoryBytesProc)(int flags, int* totalBytes, int* freeBytes);
typedef int(__fastcall* zVideo_GetHwApiDeviceFeatureFlagsProc)(int deviceIndex);
typedef zVideo_TextureRecordPartial*(__fastcall* zVideo_CreateTextureRecordProc)(
    const char* textureName,
    zVidImagePartial* image,
    int useAlpha,
    int clampU,
    int clampV
);
typedef void(__fastcall* zVideo_DestroyTextureRecordProc)(zVideo_TextureRecordPartial* texture);
typedef void(__fastcall* zVideo_TextureRecordReleaseUploadSurfaceRefProc)(zVideo_TextureRecordPartial* texture);
typedef int(__fastcall* zVideo_TextureRecordLockUploadSurfaceProc)(
    zVideo_TextureRecordPartial* textureRecord,
    void** outPixels,
    int* outPitchBytes
);
typedef int(__fastcall* zVideo_TextureRecordUnlockUploadSurfaceProc)(zVideo_TextureRecordPartial* textureRecord);
typedef void(__fastcall* zVideo_TextureRecordFinalizeUploadProc)(
    zVideo_TextureRecordPartial* textureRecord,
    void* reserved,
    zVidImagePartial* image
);
typedef void(__cdecl* zVideo_ReleaseAllTextureUploadSurfacesProc)();
typedef void(__cdecl* zVideo_UpdateFogColorProc)();
typedef void(__cdecl* zVideo_FlushProc)();
typedef void(__fastcall* zVideo_ImageProc)(zVidImagePartial* image);
typedef IDirectDrawSurface3*(__fastcall* zVideo_ImageLazyCreateSurfaceProc)(zVidImagePartial* image);
typedef int(__fastcall* zVideo_ImageUploadPixelsProc)(zVidImagePartial* image, HDC* outHdc);
typedef int(__fastcall* zVideo_ImageReleaseSurfaceProc)(zVidImagePartial* image, HDC hdc);
typedef void(__fastcall* zVideo_BltImageRectProc)(
    zVidImagePartial* srcImage,
    int srcColorKeyEnable,
    zVidRect32* srcRect,
    zVidRect32* dstRect
);
typedef void(__fastcall* zVideo_SetFogEnableProc)(int enable);
typedef void(__stdcall* zVideo_SetFogFloatProc)(float value);
typedef void(__fastcall* zVideo_ApplyFogStateProc)(float fogStart, float fogEnd, float unused);
typedef void(__fastcall* zVideo_SubmitPolyFlatColor16Proc)(
    zVideo_XyzVertex* vertices,
    unsigned int packedColor16,
    int alpha,
    int vertexCount,
    int renderParam,
    int queueMode
);
typedef void(__fastcall* zVideo_SubmitPolyGouraudColor16Proc)(
    zVideo_XyzVertex* vertices,
    unsigned int* packedColors16,
    int alpha,
    int vertexCount,
    int renderParam,
    int queueMode
);
typedef void(__fastcall* zVideo_SubmitPolyColorAttrProc)(
    zVideo_XyzVertex* vertices,
    unsigned int packedColor16,
    zVideo_ColorRgbFloat* baseColor,
    float* attr1,
    float* attr0,
    float* attr2,
    int alpha,
    int vertexCount,
    unsigned int renderParam,
    int queueMode
);
typedef void(__fastcall* zVideo_SubmitPolyRenderClassProc)(
    zVideo_XyzVertex* vertices,
    zVideo_TexCoord* texCoords,
    int vertexCount,
    zVideo_RenderClass* renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
);
typedef void(__fastcall* zVideo_SubmitPolygonProc)(
    zVideo_XyzVertex* vertices,
    zVideo_TexCoord* uvPairs,
    float* attr1,
    float* attr0,
    float* attr2,
    int vertexCount,
    zVideo_RenderClass* renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
);
typedef void(__fastcall* zVideo_DrawPointColor16Proc)(
    zVideo_XyzVertex* pointPos,
    unsigned int packedColor16,
    int pointCount
);

struct zVidD3DDriverRecordPartial {
    char m_deviceName[0x20];
    char m_deviceDescription[0x60];
    GUID* pD3DDeviceGuid;
    GUID m_d3dDeviceGuidStorage;
    D3DDEVICEDESC m_hwDesc;
};

struct zVideo_TextureRecordPartial {
    IDirectDrawSurface* m_uploadSurface;
    IDirectDrawSurface* m_textureSurface;
    IDirect3DTexture2* m_texture;
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
    void* pixels;
    char* alphaMap;
    void* palette;
    float widthScale;
    char* queuedAlphaMap;
    int uShiftFrom20;
    int uMask;
    int vMaskFixed20;
    IDirectDrawSurface3* surface;
    int pitchWords;
};

struct zVidHwApiDeviceRecordPartial {
    GUID* pDirectDrawGuid;
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
    void* pixels;
    int locked;
    int pageLockActive;
    IDirectDrawSurface3* surf;
};

struct zVideo_SurfaceLockVerifyArgs {
    unsigned int size;
    unsigned char reserved_04[0x18];
    int callerContext;
    unsigned char reserved_20[0x8];
};

struct zVideo_SurfaceLockVerifier;
struct zVideoFxPass3Config;
extern zVideo_SurfaceStatePartial g_zVideo_SurfaceStateSwapScratch;
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
    FILE* fileHandle;
    zVidTexturePackHeader header;
    zVidTexturePackRecord* records;
    int paletteTableBaseIndex;
};

struct zVidPaletteRemapRecipe {
    /*
     * The endpoints use the same RGB representation as the light colours.
     * Their strengths control each endpoint contribution to the remap.
     */
    zColorRgb color0;
    zColorRgb color1;
    float color0Strength;
    float color1Strength;
};

struct zVideo_SurfaceLockVerifier {
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) = 0;
    virtual ULONG STDMETHODCALLTYPE AddRef() = 0;
    virtual ULONG STDMETHODCALLTYPE Release() = 0;
    virtual HRESULT STDMETHODCALLTYPE Unknown0c() = 0;
    virtual HRESULT STDMETHODCALLTYPE VerifySurfaceState(zVideo_SurfaceLockVerifyArgs* args) = 0;
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

struct zVideo_PackedColor16 {
    unsigned short value;
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
    zVideo_RenderClass* renderClass;
    int renderParam;
    D3DTLVERTEX vertices[64];
};

struct zVideo_OverwriteQueueEntry {
    int type;
    int vertexCount;
    zVideo_RenderClass* renderClass;
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

/*
 * zVideo module state recovered as one zero-filled aggregate: retail
 * zVideo::ModuleInit (0x4a7530) clears [0x632120, 0x778918) with a single
 * rep stosd, and VC5 treats stores into its array members as clobbering the
 * sibling counters (FlushSortedPolys reloads the count after each draw-order
 * store). Offsets are relative to 0x632120; unknown_* are unrecovered spans.
 */
struct zVideo_GlobalState {
    int rendererType; /* +0x00000 0x632120 */
    int fullscreenOption; /* +0x00004 0x632124 */
    int useHalfResBackbuffer; /* +0x00008 0x632128 */
    int halfResAdjustMode; /* +0x0000c 0x63212c */
    int clearScreenBufferEnabled; /* +0x00010 0x632130 */
    int primaryHasAttachedBackbuffer; /* +0x00014 0x632134 */
    int pendingWireframeState; /* +0x00018 0x632138 */
    int pendingDitherEnable; /* +0x0001c 0x63213c */
    int d3dColorNormalizeChannelIndex; /* +0x00020 0x632140 */
    int adjustSurfacesDisableGate; /* +0x00024 0x632144 */
    int d3dSceneDepth; /* +0x00028 0x632148 */
    int resolutionMenuValid; /* +0x0002c 0x63214c */
    int displayModeBpp; /* +0x00030 0x632150 */
    int isInitialized; /* +0x00034 0x632154 */
    zVideo_PixelPackParams pixelPack; /* +0x00038 0x632158 */
    int texturePixelPackRBits; /* +0x00068 0x632188 */
    int texturePixelPackGBits; /* +0x0006c 0x63218c */
    int texturePixelPackBBits; /* +0x00070 0x632190 */
    int texturePixelPackABits; /* +0x00074 0x632194 */
    unsigned int texturePixelPackRMask; /* +0x00078 0x632198 */
    unsigned int texturePixelPackGMask; /* +0x0007c 0x63219c */
    unsigned int texturePixelPackBMask; /* +0x00080 0x6321a0 */
    unsigned int texturePixelPackAMask; /* +0x00084 0x6321a4 */
    int texturePixelPackRGBBitsTotalMinus8; /* +0x00088 0x6321a8 */
    int texturePixelPackGBBitsTotalMinus8; /* +0x0008c 0x6321ac */
    int texturePixelPackBShiftTo8; /* +0x00090 0x6321b0 */
    int texturePixelPackRGBBitsTotal; /* +0x00094 0x6321b4 */
    int texturePixelPackRMaskShifted; /* +0x00098 0x6321b8 */
    int texturePixelPackGMaskShifted; /* +0x0009c 0x6321bc */
    int texturePixelPackBMaskShifted; /* +0x000a0 0x6321c0 */
    int texturePixelPackNonRgbMaskShifted; /* +0x000a4 0x6321c4 */
    HWND hWnd; /* +0x000a8 0x6321c8 */
    unsigned int clearColorPacked16; /* +0x000ac 0x6321cc */
    float fogColorPendingR255; /* +0x000b0 0x6321d0 */
    float fogColorPendingG255; /* +0x000b4 0x6321d4 */
    float fogColorPendingB255; /* +0x000b8 0x6321d8 */
    float d3dColorAttrBiasR; /* +0x000bc 0x6321dc */
    float d3dColorAttrBiasG; /* +0x000c0 0x6321e0 */
    float d3dColorAttrBiasB; /* +0x000c4 0x6321e4 */
    float fogTargetColorR255; /* +0x000c8 0x6321e8 */
    float fogTargetColorG255; /* +0x000cc 0x6321ec */
    float fogTargetColorB255; /* +0x000d0 0x6321f0 */
    float fogColorAppliedR255; /* +0x000d4 0x6321f4 */
    float fogColorAppliedG255; /* +0x000d8 0x6321f8 */
    float fogColorAppliedB255; /* +0x000dc 0x6321fc */
    zVideo_SurfaceStatePartial swSurfaceState; /* +0x000e0 0x632200 */
    zVideo_SurfaceStatePartial primarySurfaceState; /* +0x00100 0x632220 */
    zVideo_SurfaceStatePartial displayModeSurfaceState; /* +0x00120 0x632240 */
    char palettePathBuffer[0x100]; /* +0x00140 0x632260 */
    unsigned char paletteBrightnessLevel; /* +0x00240 0x632360 */
    unsigned char unknown_0241[3]; /* +0x00241 0x632361 */
    unsigned char unknown_0244[4]; /* +0x00244 0x632364 */
    PALETTEENTRY paletteFileEntries[0x100]; /* +0x00248 0x632368 */
    PALETTEENTRY systemPaletteEntries[0x100]; /* +0x00648 0x632768 */
    unsigned char unknown_0a48[0x420]; /* +0x00a48 0x632b68 */
    RECT cachedClientRectScreen; /* +0x00e68 0x632f88 */
    int numAcceptedDirectDrawDevices; /* +0x00e78 0x632f98 */
    int acceptedHardwareRendererCount; /* +0x00e7c 0x632f9c */
    unsigned int sortedPolyQueueCount; /* +0x00e80 0x632fa0 */
    int sortedPolyDrawOrder[256]; /* +0x00e84 0x632fa4 */
    int overwriteQueueCount; /* +0x01284 0x6333a4 */
    zVideo_TextureRecordPartial* defaultTextureRecord; /* +0x01288 0x6333a8 */
    zVideo_StatusProc pfnOpenVideoMode; /* +0x0128c 0x6333ac */
    zVideo_ShutdownVideoSystemProc pfnShutdownVideoSystem; /* +0x01290 0x6333b0 */
    zVideo_AdjustSurfacesProc pfnAdjustSurfaces; /* +0x01294 0x6333b4 */
    zVideo_PaletteSetEntriesProc pfnPaletteSetEntries; /* +0x01298 0x6333b8 */
    zVideo_StatusProc pfnSetVideoMode; /* +0x0129c 0x6333bc */
    zVideo_SurfaceStateProc pfnUnlockSurfaceState; /* +0x012a0 0x6333c0 */
    zVideo_SurfaceStateProc pfnLockSurfaceState; /* +0x012a4 0x6333c4 */
    zVideo_ClearStateSurfaceAndZBufferProc pfnClearStateSurfaceAndZBuffer; /* +0x012a8 0x6333c8 */
    zVideo_ClearSwSurfaceAndZBufferProc pfnClearSwSurfaceAndZBuffer; /* +0x012ac 0x6333cc */
    zVideo_ClearZBufferRectProc pfnClearZBufferRect; /* +0x012b0 0x6333d0 */
    zVideo_UpdateFogColorProc pfnUpdateFogColor; /* +0x012b4 0x6333d4 */
    IDirectDraw2* pDirectDraw2; /* +0x012b8 0x6333d8 */
    IDirectDrawClipper* pClipper; /* +0x012bc 0x6333dc */
    IDirectDrawSurface3* pPageUnlockSurface; /* +0x012c0 0x6333e0 */
    IDirectDrawPalette* pDDPalette; /* +0x012c4 0x6333e4 */
    zVideo_SurfaceLockVerifier* pSurfaceLockVerifier; /* +0x012c8 0x6333e8 */
    IDirect3D2* pD3D2; /* +0x012cc 0x6333ec */
    IDirect3DDevice2* pD3DDevice; /* +0x012d0 0x6333f0 */
    IDirectDrawSurface3* pZBufferSurface; /* +0x012d4 0x6333f4 */
    IDirectDrawSurface* pZBufferAttachSurface; /* +0x012d8 0x6333f8 */
    IDirect3DViewport2* pD3DViewport2; /* +0x012dc 0x6333fc */
    IDirect3DMaterial2* pD3DMaterial2; /* +0x012e0 0x633400 */
    D3DMATERIALHANDLE d3dMaterialHandle; /* +0x012e4 0x633404 */
    zVideo_D3DRenderStateCacheLive d3dRenderStateCache; /* +0x012e8 0x633408 */
    int cachedFogEnableRenderState; /* +0x01310 0x633430 */
    int cachedFogModeLightState; /* +0x01314 0x633434 */
    float cachedFogStartLightStateValue; /* +0x01318 0x633438 */
    float cachedFogEndLightStateValue; /* +0x0131c 0x63343c */
    D3DDEVICEDESC d3dHalDeviceDesc; /* +0x01320 0x633440 */
    D3DDEVICEDESC d3dHelDeviceDesc; /* +0x0141c 0x63353c */
    int quadBatchCount; /* +0x01518 0x633638 */
    zVideo_QuadBatchItemPartial quadBatchItems[16]; /* +0x0151c 0x63363c */
    unsigned char unknown_1d1c[4]; /* +0x01d1c 0x633e3c */
    zVidHwApiDeviceRecordPartial* pSelectedHwApiDeviceRecord; /* +0x01d20 0x633e40 */
    zVidHwApiDeviceRecordPartial hwApiDeviceTable[4]; /* +0x01d24 0x633e44 */
    zVidD3DDriverRecordPartial* pSelectedD3DDeviceInfo; /* +0x038d4 0x6359f4 */
    DDCAPS ddrawCapsHal; /* +0x038d8 0x6359f8 */
    DDCAPS ddrawCapsHel; /* +0x03a54 0x635b74 */
    unsigned char unknown_3bd0[4]; /* +0x03bd0 0x635cf0 */
    unsigned char surfaceLockVerifyFlags; /* +0x03bd4 0x635cf4 */
    unsigned char unknown_3bd5[0x17]; /* +0x03bd5 0x635cf5 */
    int surfaceLockVerifyContext; /* +0x03bec 0x635d0c */
    unsigned char unknown_3bf0[8]; /* +0x03bf0 0x635d10 */
    D3DTLVERTEX d3dSubmitTempVertices[64]; /* +0x03bf8 0x635d18 */
    zVideo_SortedPolyQueueEntry sortedPolyQueue[256]; /* +0x043f8 0x636518 */
    zVideo_OverwriteQueueEntry overwriteQueue[0x180]; /* +0x84ff8 0x6b7118 */
};

extern int g_zVid_PaletteRemapRecipeCount;
extern zVidPaletteRemapRecipe* g_zVid_PaletteRemapRecipes;
extern int g_zVideo_ActiveRendererPath;
extern int g_zVideo_FrameTick;
extern CZCameraDataPartial* g_zVideo_pActiveViewContext;
extern zTag4Partial g_zVideo_ActiveViewVariantTag;
extern int g_zVid_CachedClientRectUpdateMask;
extern int g_zVideo_SoftwareModeHotkeyEnabled;
extern float g_zVideo_InverseZTolerancePending;
extern int g_zVideo_D3DAppendFanCloseVertexPending;
extern int g_zVideo_DirectDrawEnumOrdinal;
extern int g_zVid_TexturePackLoadState;
extern int g_zVid_BuiltinTexturePackCount;
extern zVidTexturePackEntry* g_zVid_BuiltinTexturePacks;
extern int g_zVid_TexturePackCount;
extern zVidTexturePackEntry* g_zVid_TexturePacks;
extern int g_zVid_PaletteRemapVariantTableCount;
extern unsigned short** g_zVid_PaletteRemapVariantTables;
extern zVidImagePartial g_zVideo_DefaultTextureImage;
extern char g_zVideo_DefaultHwApiDescription[8];
extern char g_zVideo_InitFailSetModeMsg[0x19];
extern char g_zVideo_SourceFile_ZvidInitC[0x27];
extern char g_zVideo_InitFailOpenVideoModeMsg[0x1a];
extern char g_zVideo_SourceFile_ZvidDdC[0x25];
extern char g_zVideo_UnrecognizedPixelFormatMsg[0x1a];
extern char g_zVideo_DDrawEnumBeginMsg[0x20];
extern char g_zVideo_DDrawEnumAgpSuffix[0x6];
extern char g_zVideo_DDrawEnumTooManyDevicesMsg[0x34];
extern char g_zVideo_DDrawEnumDevicePrintfFmt[0x17];
extern char g_zVideo_D3DEnumNoUsableDriversMsg[0x14];
extern char g_zVideo_D3DEnumBeginMsgFmt[0x1c];
extern char g_zVideo_D3DEnumAcceptedMsg[0x9];
extern char g_zVideo_D3DEnumTooManyDriversMsg[0x2c];
extern char g_zVideo_D3DEnumSkipNo16BitZBufferMsg[0x31];
extern char g_zVideo_D3DEnumSkipNoRgbColorMsg[0x2b];
extern char g_zVideo_D3DEnumSkipNoHardwareMsg[0x31];
extern char g_zVideo_D3DEnumDriverPrintfFmt[0x10];
extern char g_zVideo_DefaultD3DDeviceName[0x6];
extern zVideo_QueryMemoryBytesProc g_zVideo_pfnQueryDeviceVideoMemoryBytes;
extern zVideo_QueryMemoryBytesProc g_zVideo_pfnQueryTextureMemoryBytes;
extern zVideo_BltRectDirectProc g_zVideo_pfnBltSwToPrimaryRectDirect;
extern zVideo_BltRectDirectProc g_zVideo_pfnBltPrimaryToSwRectDirect;
extern zVideo_BltImageRectProc g_zVideo_pfnBltSwToPrimaryRect;
extern zVideo_CreateTextureRecordProc g_zVideo_pfnCreateTextureRecord;
extern zVideo_TextureRecordLockUploadSurfaceProc g_zVideo_pfnTextureRecordLockUploadSurface;
extern zVideo_TextureRecordUnlockUploadSurfaceProc g_zVideo_pfnTextureRecordUnlockUploadSurface;
extern zVideo_TextureRecordReleaseUploadSurfaceRefProc g_zVideo_pfnTextureRecordReleaseUploadSurfaceRef;
extern zVideo_TextureRecordFinalizeUploadProc g_zVideo_pfnTextureRecordFinalizeUpload;
extern zVideo_DestroyTextureRecordProc g_zVideo_pfnTextureRecordDestroy;
extern zVideo_ReleaseAllTextureUploadSurfacesProc g_zVideo_pfnTextureRecordReleaseAllUploadSurfaces;
extern zVideo_ImageProc g_zVideo_pfnImageEnsureSurfaceForCurrentDevice;
extern zVideo_ImageLazyCreateSurfaceProc g_zVideo_pfnImageLazyCreateVideoMemorySurface;
extern zVideo_SetFogEnableProc g_zVideo_pfnSetFogEnable;
extern zVideo_SetFogFloatProc g_zVideo_pfnSetFogStart;
extern zVideo_SetFogFloatProc g_zVideo_pfnSetFogEnd;
extern zVideo_ApplyFogStateProc g_zVideo_pfnApplyFogStateFromGlobals;
extern zVideo_FlushProc g_zVideo_pfnFlushSortedPolys;
extern zVideo_FlushProc g_zVideo_pfnFlushOverwritePolys;
extern zVideo_FlushProc g_zVideo_pfnFlushQuadBatch;
extern zVideo_SubmitPolyFlatColor16Proc g_zVideo_pfnSubmitPolyFlatColor16;
extern zVideo_SubmitPolyGouraudColor16Proc g_zVideo_pfnSubmitPolyGouraudColor16;
extern zVideo_SubmitPolyColorAttrProc g_zVideo_pfnSubmitPolyColorAttr;
extern zVideo_SubmitPolyRenderClassProc g_zVideo_pfnSubmitPolyRenderClass;
extern zVideo_SubmitPolygonProc g_zVideo_pfnSubmitPolygon;
extern zVideo_SubmitPolygonProc g_zVideo_pfnSubmitPolygonLit;
extern zVideo_DrawPointColor16Proc g_zVideo_pfnDrawPointColor16;
extern zVidRect32 g_zVideo_PrimarySurfaceRectScratch;
extern zVideo_ImageUploadPixelsProc g_zVideo_pfnImageUploadPixelsToSurface;
extern zVideo_ImageReleaseSurfaceProc g_zVideo_pfnImageReleaseSurface;
extern zVideo_GetHwApiDeviceFeatureFlagsProc g_zVideo_pfnGetHwApiDeviceFeatureFlags;
extern unsigned int g_zVideo_OpaqueWhiteArgb;
extern char g_zVideo_SourceFile_ZvidDdd3dC[0x28];
extern char g_zVideo_TextureTooLargeUsingDefaultFmt[0x49];
extern char g_zVideo_TextureBadAspectUsingDefaultFmt[0x4f];
extern char g_zVideo_TexturePaletteUnsupportedUsingDefaultFmt[0x3c];
extern char g_zVideo_TextureNotPowerOf2UsingDefaultFmt[0x4c];
extern char g_zVideo_NotEnoughMaxTransparentPolysFmt[0x2a];
extern char g_zVideo_NotEnoughMaxOverwritePolysNeedFmt[0x2d];
extern char g_zVideo_NotEnoughMaxOverwritePolysNeedsFmt[0x2e];
extern char g_zVideo_DirectDrawErrorFmt[0x1d];
extern char g_zVideo_D3DErrorName_ViewportDataNotSet[0x1a];
extern char g_zVideo_D3DErrorName_SceneNotInScene[0x1a];
extern char g_zVideo_D3DErrorName_SceneInScene[0x16];
extern char g_zVideo_D3DErrorName_SceneEndFailed[0x18];
extern char g_zVideo_D3DErrorName_SceneBeginFailed[0x1a];
extern char g_zVideo_D3DErrorName_NoViewports[0x13];
extern char g_zVideo_D3DErrorName_NotInBegin[0x12];
extern char g_zVideo_D3DErrorName_InBegin[0x0f];
extern char g_zVideo_D3DErrorName_LightSetFailed[0x18];
extern char g_zVideo_D3DErrorName_ZBuffNeedsVideoMemory[0x1f];
extern char g_zVideo_D3DErrorName_ZBuffNeedsSystemMemory[0x20];
extern char g_zVideo_D3DErrorName_TextureUnlockFailed[0x1d];
extern char g_zVideo_D3DErrorName_TextureSwapFailed[0x1b];
extern char g_zVideo_D3DErrorName_TextureNotLocked[0x1a];
extern char g_zVideo_D3DErrorName_TextureNoSupport[0x1a];
extern char g_zVideo_D3DErrorName_TextureLocked[0x16];
extern char g_zVideo_D3DErrorName_TextureLockFailed[0x1b];
extern char g_zVideo_D3DErrorName_TextureLoadFailed[0x1b];
extern char g_zVideo_D3DErrorName_TextureGetSurfFailed[0x1e];
extern char g_zVideo_D3DErrorName_TextureDestroyFailed[0x1e];
extern char g_zVideo_D3DErrorName_TextureCreateFailed[0x1d];
extern char g_zVideo_D3DErrorName_TextureBadSize[0x17];
extern char g_zVideo_D3DErrorName_SetViewportDataFailed[0x1e];
extern char g_zVideo_D3DErrorName_MatrixSetDataFailed[0x1d];
extern char g_zVideo_D3DErrorName_MatrixGetDataFailed[0x1d];
extern char g_zVideo_D3DErrorName_MatrixDestroyFailed[0x1d];
extern char g_zVideo_D3DErrorName_MatrixCreateFailed[0x1c];
extern char g_zVideo_D3DErrorName_MaterialSetDataFailed[0x1f];
extern char g_zVideo_D3DErrorName_MaterialGetDataFailed[0x1f];
extern char g_zVideo_D3DErrorName_MaterialDestroyFailed[0x1f];
extern char g_zVideo_D3DErrorName_MaterialCreateFailed[0x1e];
extern char g_zVideo_D3DErrorName_InvalidVertexType[0x19];
extern char g_zVideo_D3DErrorName_InvalidPrimitiveType[0x1c];
extern char g_zVideo_D3DErrorName_InvalidCurrentViewport[0x1e];
extern char g_zVideo_D3DErrorName_ExecuteUnlockFailed[0x1d];
extern char g_zVideo_D3DErrorName_ExecuteNotLocked[0x1a];
extern char g_zVideo_D3DErrorName_ExecuteLocked[0x16];
extern char g_zVideo_D3DErrorName_ExecuteLockFailed[0x1b];
extern char g_zVideo_D3DErrorName_ExecuteFailed[0x16];
extern char g_zVideo_D3DErrorName_ExecuteDestroyFailed[0x1e];
extern char g_zVideo_D3DErrorName_ExecuteCreateFailed[0x1d];
extern char g_zVideo_D3DErrorName_ExecuteClippedFailed[0x1e];
extern char g_zVideo_D3DErrorName_InvalidDevice[0x16];
extern char g_zVideo_D3DErrorName_BadMajorVersion[0x17];
extern char g_zVideo_D3DErrorName_BadMinorVersion[0x17];
extern char g_zVideo_DDErrorName_NotPageLocked[0x14];
extern char g_zVideo_DDErrorName_CantPageUnlock[0x15];
extern char g_zVideo_DDErrorName_CantPageLock[0x13];
extern char g_zVideo_DDErrorName_XAlign[0x0d];
extern char g_zVideo_DDErrorName_WrongMode[0x10];
extern char g_zVideo_DDErrorName_UnsupportedMode[0x16];
extern char g_zVideo_DDErrorName_RegionTooSmall[0x15];
extern char g_zVideo_DDErrorName_PrimarySurfaceAlreadyExists[0x22];
extern char g_zVideo_DDErrorName_OverlayNotVisible[0x18];
extern char g_zVideo_DDErrorName_NotPalettized[0x14];
extern char g_zVideo_DDErrorName_NotLocked[0x10];
extern char g_zVideo_DDErrorName_NotFlippable[0x13];
extern char g_zVideo_DDErrorName_NoAOverlaySurface[0x18];
extern char g_zVideo_DDErrorName_NoPaletteHw[0x12];
extern char g_zVideo_DDErrorName_NoPaletteAttached[0x18];
extern char g_zVideo_DDErrorName_NoMipMapHw[0x11];
extern char g_zVideo_DDErrorName_NoHwnd[0x0d];
extern char g_zVideo_DDErrorName_NoEmulation[0x12];
extern char g_zVideo_DDErrorName_NoDirectDrawHw[0x15];
extern char g_zVideo_DDErrorName_NoDdRopsHw[0x11];
extern char g_zVideo_DDErrorName_NoDirectDc[0x11];
extern char g_zVideo_DDErrorName_NoClipperAttached[0x18];
extern char g_zVideo_DDErrorName_NoBltHw[0x0e];
extern char g_zVideo_DDErrorName_InvalidSurfaceType[0x19];
extern char g_zVideo_DDErrorName_InvalidPosition[0x16];
extern char g_zVideo_DDErrorName_InvalidDirectDrawGuid[0x1c];
extern char g_zVideo_DDErrorName_ImplicitlyCreated[0x18];
extern char g_zVideo_DDErrorName_HwndSubclassed[0x15];
extern char g_zVideo_DDErrorName_HwndAlreadySet[0x15];
extern char g_zVideo_DDErrorName_ExclusiveModeAlreadySet[0x1e];
extern char g_zVideo_DDErrorName_DirectDrawAlreadyCreated[0x1f];
extern char g_zVideo_DDErrorName_DcAlreadyCreated[0x17];
extern char g_zVideo_DDErrorName_ClipperIsUsingHwnd[0x19];
extern char g_zVideo_DDErrorName_CantDuplicate[0x14];
extern char g_zVideo_DDErrorName_CantCreateDc[0x13];
extern char g_zVideo_DDErrorName_BltFastCantClip[0x16];
extern char g_zVideo_DDErrorName_WasStillDrawing[0x16];
extern char g_zVideo_DDErrorName_VerticalBlankInProgress[0x1e];
extern char g_zVideo_DDErrorName_UnsupportedMask[0x16];
extern char g_zVideo_DDErrorName_UnsupportedFormat[0x18];
extern char g_zVideo_DDErrorName_TooBigWidth[0x12];
extern char g_zVideo_DDErrorName_TooBigSize[0x11];
extern char g_zVideo_DDErrorName_TooBigHeight[0x13];
extern char g_zVideo_DDErrorName_SurfaceNotAttached[0x19];
extern char g_zVideo_DDErrorName_SurfaceLost[0x12];
extern char g_zVideo_DDErrorName_SurfaceIsObscured[0x18];
extern char g_zVideo_DDErrorName_CantLockSurface[0x16];
extern char g_zVideo_DDErrorName_SurfaceBusy[0x12];
extern char g_zVideo_DDErrorName_SurfaceAlreadyDependent[0x1e];
extern char g_zVideo_DDErrorName_SurfaceAlreadyAttached[0x1d];
extern char g_zVideo_DDErrorName_ColorKeyNotSet[0x15];
extern char g_zVideo_DDErrorName_OverlayCantClip[0x16];
extern char g_zVideo_DDErrorName_OverlayColorKeyOnlyOneActive[0x23];
extern char g_zVideo_DDErrorName_PaletteBusy[0x12];
extern char g_zVideo_DDErrorName_OutOfVideoMemory[0x17];
extern char g_zVideo_DDErrorName_OutOfCaps[0x10];
extern char g_zVideo_DDErrorName_NoZOverlayHw[0x13];
extern char g_zVideo_DDErrorName_NoZBufferHw[0x12];
extern char g_zVideo_DDErrorName_NoVSyncHw[0x10];
extern char g_zVideo_DDErrorName_NoTextureHw[0x12];
extern char g_zVideo_DDErrorName_Not8BitColor[0x13];
extern char g_zVideo_DDErrorName_Not4BitColorIndex[0x18];
extern char g_zVideo_DDErrorName_Not4BitColor[0x13];
extern char g_zVideo_DDErrorName_NoStretchHw[0x12];
extern char g_zVideo_DDErrorName_NoRotationHw[0x13];
extern char g_zVideo_DDErrorName_NoRasterOpHw[0x13];
extern char g_zVideo_DDErrorName_NoOverlayHw[0x12];
extern char g_zVideo_DDErrorName_NotFound[0x0f];
extern char g_zVideo_DDErrorName_NoMirrorHw[0x11];
extern char g_zVideo_DDErrorName_NoGdi[0x0c];
extern char g_zVideo_DDErrorName_NoFlipHw[0x0f];
extern char g_zVideo_DDErrorName_NoColorKeyHw[0x13];
extern char g_zVideo_DDErrorName_NoDirectDrawSupport[0x1a];
extern char g_zVideo_DDErrorName_NoExclusiveMode[0x16];
extern char g_zVideo_DDErrorName_NoColorKey[0x11];
extern char g_zVideo_DDErrorName_NoCooperativeLevelSet[0x1c];
extern char g_zVideo_DDErrorName_NoColorConvHw[0x14];
extern char g_zVideo_DDErrorName_NoClipList[0x11];
extern char g_zVideo_DDErrorName_NoAlphaHw[0x10];
extern char g_zVideo_DDErrorName_No3d[0x0b];
extern char g_zVideo_DDErrorName_LockedSurfaces[0x15];
extern char g_zVideo_DDErrorName_InvalidRect[0x12];
extern char g_zVideo_DDErrorName_InvalidPixelFormat[0x19];
extern char g_zVideo_DDErrorName_InvalidObject[0x14];
extern char g_zVideo_DDErrorName_InvalidMode[0x12];
extern char g_zVideo_DDErrorName_InvalidClipList[0x16];
extern char g_zVideo_DDErrorName_InvalidCaps[0x12];
extern char g_zVideo_DDErrorName_HeightAlign[0x12];
extern char g_zVideo_DDErrorName_Exception[0x10];
extern char g_zVideo_DDErrorName_CurrentlyNotAvail[0x18];
extern char g_zVideo_DDErrorName_CannotDetachSurface[0x1a];
extern char g_zVideo_DDErrorName_CannotAttachSurface[0x1a];
extern char g_zVideo_DDErrorName_AlreadyInitialized[0x19];
extern char g_zVideo_DDErrorName_InvalidParams[0x14];
extern char g_zVideo_DDErrorName_OutOfMemory[0x12];
extern char g_zVideo_DDErrorName_NotInitialized[0x15];
extern char g_zVideo_DDErrorName_Generic[0x0e];
extern char g_zVideo_DDErrorName_Unsupported[0x12];

unsigned short __fastcall zVidPackColorRGB(unsigned char red, unsigned char green, unsigned char blue);
unsigned short __fastcall zVidPackColor00RRGGBB(unsigned int color00RRGGBB);
zVideo_PackedColor16 __fastcall zVidPackColorRgbFloats(zVideo_ColorRgbFloat* color);
void __fastcall zVideoSetClearColorPacked16(unsigned int packedColor16);
void __fastcall zVideoSetPendingFogTargetColorFromRgb01(zVideo_ColorRgbFloat* color);
void __cdecl zVideoRestoreIconicFullscreenWindowIfNeeded();
}

void __fastcall zVideoSetActiveViewContext(CZCameraDataPartial* viewContext);
void __fastcall zVideoUpdateProjectionStateFromCameraData(CZCameraDataPartial* cameraData);
int __fastcall zVideoFrustumTestSphereClipMask(zVec3* sphereCenter, float radius, int* clipMaskInOut);

int __fastcall zVideoswRenderFrame(CZNodePartial* camera, int updateFxPass3Local);

namespace zVid {
void __fastcall SetAccelerationOption(int accelerationOption);
void __fastcall SetHwApiOption(int hwApiOption);
int GetAccelerationOption();
int GetHwApiOption();
int __cdecl GetAcceptedDirectDrawDeviceCount();
int __cdecl GetAcceptedHardwareRendererCount();
int __cdecl GetAcceptedHardwareRendererCountCached();
int __cdecl HasAcceptedHardwareRenderer();
int __cdecl GetTexturePackLoadState();
void __fastcall SetTexturePackLoadState(int texturePackLoadState);
int GetVideoModeIndexFromOptions();
void __fastcall SetVideoModeIndex(int modeIndex);
int __fastcall QueryDeviceVideoMemoryBytes(int deviceIndexOrMinus1, int* totalBytes, int* freeBytes);
int __fastcall QueryTextureMemoryBytes(int deviceIndexOrMinus1, int* totalBytes, int* freeBytes);
int __cdecl QueryCachedClientRectUpdateMaskIf3dfx();
/**
 * Original source-shape evidence: the retail contribution lies between
 * CZGameFrame::OnSize and CZGameFrame::OnMove, so this externally linked
 * inline body is emitted by the first CZGameFrame call site.
 *
 * Purpose: refresh the cached client rectangle when the renderer-path update
 * mask is set.
 *
 * Evidence: BN calls zVid::QueryCachedClientRectUpdateMaskIf3dfx and
 * tail-jumps to zVideo::UpdateCachedClientRectScreenCoords only when the query
 * is nonzero.
 */
void __cdecl UpdateCachedClientRectIfUpdateMaskEnabled();
void __fastcall SetCachedClientRectUpdateMask(int mask);
char* __cdecl GetSelectedHwApiDescriptionOrDefault();
char* __cdecl GetSelectedD3DDeviceNameOrDefault();
char* __fastcall GetHwApiDescription(int index);
char* __fastcall GetHwApiDriverName(int index);
void __cdecl NoiseInitBuffers();
void __cdecl NoiseShutdownBuffers();
void __fastcall DrawNoiseRect(zVidRect32* rectOrNull, double intensity);
int __cdecl InitFrameScratchBuffers();
int __cdecl ShutdownFrameScratchBuffers();
} // namespace zVid

namespace zVideo_FxSurface {
void __fastcall ApplyBlueTintRect(zVidRect32* rectOrNull);
void __fastcall ApplyGreenMaskRect(zVidRect32* rectOrNull);
void __fastcall DrawAlphaBlendedLine(
    zVidRect32* clipRect,
    int x1,
    int y1,
    int x0,
    int y0,
    unsigned short color16,
    float alphaEnd,
    float alphaStart,
    int clipInset
);
void __fastcall DrawColoredLinesBatch(zVideoFxColoredLineRecord* lines, int count, zVidRect32* clipRectOrNull);
} // namespace zVideo_FxSurface

namespace zVideo_buff {
int __fastcall ClipCoordToRange(int* coordPtr, int minCoord, int maxCoord);
zVidImagePartial* __fastcall
CopySurfaceRectToImage(int sourceSelector, zVidRect32* rect, zVidImagePartial* imageOrNull);
void __fastcall
BltSourceToPrimaryClipped(zVidImagePartial* srcImage, int dstX, int dstY, int srcColorKeyEnable, zVidRect32* srcRect);
} // namespace zVideo_buff

namespace zVideo {
void __fastcall SetFogColorFromRgb01(zVideo_ColorRgbFloat* color);
void __fastcall SetFogTargetColorFromRgb01(zVideo_ColorRgbFloat* color);
void __cdecl CommitFogColorIfChanged();
void __cdecl CommitFogTargetColorIfChanged();
void __fastcall PixelPackSetupFromMasks(
    int redBits,
    int greenBits,
    int blueBits,
    unsigned int redMask,
    unsigned int greenMask,
    unsigned int blueMask
);
void __fastcall TexturePixelPackSetupFromMasks(
    int redBits,
    int greenBits,
    int blueBits,
    int alphaBits,
    unsigned int redMask,
    unsigned int greenMask,
    unsigned int blueMask,
    unsigned int alphaMask
);
void __fastcall PixelPackGetRgbBits(int* outRBits, int* outGBits, int* outBBits);
void __fastcall PixelPackGetRgbMasks(unsigned int* outRMask, unsigned int* outGMask, unsigned int* outBMask);
void __fastcall PixelPackGetPackingParams(int* outPackedBase, int* outSumMinus8, int* outBShiftTo8);
int __fastcall SetRendererTypeAndActivePath(int rendererType);
int __fastcall SetHalfResAdjustMode(int mode);
void __fastcall HandleSoftwareModeHotkeyCommand(int commandId);
zVidRect32* __cdecl GetPrimarySurfaceRectScratch();
void* __cdecl GetSwSurfacePixels();
int __cdecl GetSwSurfaceWidth();
int __cdecl GetSwSurfaceHeight();
int __cdecl GetSwSurfacePitch();
int __cdecl GetSwSurfaceLockedFlag();
void* __cdecl GetPrimarySurfacePixels();
int __cdecl GetPrimarySurfaceWidth();
int __cdecl GetPrimarySurfaceHeight();
int __cdecl GetPrimarySurfacePitch();
int __cdecl GetDisplayModeBpp();
int __fastcall LoadPaletteFileAndApplyBrightness(const char* palettePath);
int __fastcall ApplyBrightnessToPaletteEntries(PALETTEENTRY* paletteEntries);
int __fastcall InitApplyModeIndex(int modeIndex);
void __fastcall InitSetSurfaceGeometryFromModeIndex(int modeIndex);
int __fastcall SetVideoMode(int modeIndex);
int __fastcall InitVideoSystem(HWND hWnd, int rendererBackend, int fullscreen, int modeIndex);
void __fastcall CallClearSwSurfaceAndZBuffer(zVidRect32* surfaceRect, zVidRect32* zRect);
void __fastcall CallClearPrimarySurfaceAndZBuffer(zVidRect32* rect);
int __fastcall ExchangeClearScreenBufferEnabled(int enable);
int __cdecl GetClearScreenBufferEnabled();
int __cdecl DispatchLockDisplayModeSurfaceState();
int __cdecl DispatchUnlockDisplayModeSurfaceState();
int __cdecl DispatchUnlockSwSurfaceState();
int __cdecl DispatchUnlockPrimarySurfaceState();
void __fastcall FxSetSurfaceState(void* pixels, int width, int height, int pitchBytes);
void __fastcall FxPass3CopySurfacePixelToScratchClipped(int dstDx, int dstDy, int srcDx, int srcDy);
void __fastcall FxPass3ApplyToCurrentSurface(
    int centerX,
    int centerY,
    int currentRadius,
    int maxRadius,
    int extent,
    float sinFreq,
    float sinPhase,
    zVidRect32* clipRectOrNull
);
void __fastcall buffBlurRegionCombined(zVidRect32* rectOrNull, int mode);
void __fastcall buffBlurRegionVertical(zVidRect32* rectOrNull, int mode);
void __fastcall buffBlurRegionHorizontal(zVidRect32* rectOrNull, int mode);
void __fastcall buffBlurRegionByMode(zVidRect32* rectOrNull, int mode);
void __fastcall FxPass3SetPrimaryElementParamsLocal(unsigned short packedColor, double primaryAlpha);
void __fastcall FxPass3QueueElementLocal(
    int rectLeftPixels,
    int rectTopPixels,
    int currentRadiusPixels,
    int maxRadiusPixels,
    int extentPixels,
    float sinFreq,
    float sinPhase
);
void __fastcall FxPass3QueuePrimitive(void* primitive, int width, int height, int pitchBytes);
void __fastcall FxPass3SetInputRectByIndex(int index, HudUiRect* rectOrNull);
void __fastcall FxPass3UpdateLocal(float deltaTime);
int __cdecl RunPostprocessOnSwBuffer();
int __cdecl RunPostprocessOnPrimaryBuffer();
int __fastcall
AdjustSurfacesIfEnabled(zVidRect32* srcRect, zVidRect32* dstRect, int waitForPresent, int blitPrimaryToSwFirst);
void __fastcall BindRendererDispatch(int rendererType, int fullscreenOption);
void __fastcall CommitHwApiDeviceSelection(int hwApiIndex);
int __fastcall SelectHwApiDeviceOrFallback(int hwApiIndex);
int __cdecl ReturnSuccessStub() throw();
int __cdecl ModuleInit() throw();
int __cdecl ShutdownVideoSystem();
int __fastcall UpdateCachedClientRectScreenCoords();
void __cdecl AtExitReleaseAllInterfacesAndSurfaces();
} // namespace zVideo

namespace zVid_Image {
extern zVidImagePartial g_zImage_DefaultImage;

zVidImagePartial* __cdecl Create();
int __fastcall Destroy(zVidImagePartial* image) throw();
zVidImagePartial* __fastcall ReleaseIfNotDefault(zVidImagePartial* image) throw();
void __fastcall ReleaseOwnedBuffers(zVidImagePartial* image);
void __fastcall CalcPow2ScratchFields(zVidImagePartial* image);
int __fastcall QueryBytesPerPixel(zVidImagePartial* image);
void __fastcall ClearZeroAlphaPixelsInPlace(zVidImagePartial* image);
int __fastcall SetHeaderFlagsByte(zVidImagePartial* image, unsigned char flags);
int __fastcall SetFormatCode(zVidImagePartial* image, unsigned char formatCode);
int __fastcall SetSize(zVidImagePartial* image, short width, short height);
int __fastcall QueryPixelDataBytes(zVidImagePartial* image);
int __fastcall ReadHeader(FILE* file, zVidImagePartial* image);
int __fastcall ReadData(FILE* file, zVidImagePartial* image, int bytesPerPixel = 0);
zVidImagePartial* __fastcall ReadFromFile(FILE* file);
void __fastcall ResampleSquare(zVidImagePartial* image, int sideLength);
void __fastcall
BlitToActiveTarget(zVidImagePartial* image, int dstX, int dstY, unsigned short colorKey, zVidRect32* srcRect);
void __fastcall
BlitToFramebufferClipped(zVidImagePartial* image, int dstX, int dstY, unsigned short clipFlags, zVidRect32* srcRect);
} // namespace zVid_Image

namespace zVid_PaletteRemap {
int __fastcall FindRecipeIndex(zVidPaletteRemapRecipe* recipe);
void __fastcall ApplyRecipeToPaletteVariant(
    zVidPaletteRemapRecipe* recipe,
    unsigned short* sourceColors,
    int colorCount,
    int variantIndex,
    unsigned short* destColors
);
} // namespace zVid_PaletteRemap

extern "C" int __fastcall zVidImageSetPixels(zVidImagePartial* image, void* pixels, char* alphaMap);

extern "C" zVidImagePartial* __fastcall zVideobuffCaptureSurfaceToImage(int sourceSelector);
extern "C" unsigned short* __fastcall
zVidPaletteRemapBuildAllRecipeVariantsForPalette(unsigned short* palette, int colorCount);
extern "C" int __fastcall zVidPaletteRemapBuildPaletteVariant(zVidPaletteRemapRecipe* recipe);
extern "C" int __fastcall zVidPaletteRemapFindRecipeIndexFromRgb(zColorRgb* rgb);
extern "C" FILE* __fastcall zVidTexturePackEntryLoadFromFile(zVidTexturePackEntry* entry);
extern "C" void __cdecl zVidTexturePackEnsureDefaultImagePackLoaded();
extern "C" RECOIL_NO_GS void __cdecl zVidTexturePackEnsureBuiltinTexturePacksLoaded();
extern "C" zVidImagePartial* __fastcall zVidTexturePackLoadImageByName(const char* imageName);
extern "C" zVidImagePartial* __fastcall zVidTexturePackLoadBuiltinImageByName(const char* imageName);

namespace zVid_TexturePack {
void __cdecl ShutdownBuiltinPacks();
void __fastcall Shutdown();
} // namespace zVid_TexturePack

namespace zVideo_dd {
int __cdecl GetAcceptedDirectDrawDeviceCountCached();
BOOL CALLBACK EnumDirectDrawDeviceCallback(GUID* guid, LPSTR driverDescription, LPSTR driverName, LPVOID context);
HRESULT CALLBACK EnumDirect3DDeviceCallback(
    GUID* guid,
    LPSTR deviceDescription,
    LPSTR deviceName,
    D3DDEVICEDESC* hwDesc,
    D3DDEVICEDESC* helDesc,
    LPVOID context
);
int __cdecl PrepareWindowForMode();
int __fastcall OpenVideoMode(int modeIndex);
int __cdecl RunDirectDrawDeviceEnumeration();
void __cdecl StartupEnumerateAndDefaultSelect();
int __cdecl ShutdownVideoSystem();
int __fastcall LockDirectDrawSurface(IDirectDrawSurface3* surface, DDSURFACEDESC* outLockedSurfaceDesc);
int __fastcall UnlockDirectDrawSurface(IDirectDrawSurface3* surface);
int __fastcall LockSurfaceWaitRestore(IDirectDrawSurface3* surface, DDSURFACEDESC* lockedDescOut);
int __fastcall UnlockSurfaceWaitRestore(IDirectDrawSurface3* surface);
int __fastcall LockSurfaceState(zVideo_SurfaceStatePartial* surfaceState);
int __fastcall UnlockSurfaceState(zVideo_SurfaceStatePartial* surfaceState);
IDirectDrawSurface3* __fastcall ImageLazyCreateBackingSurface(zVidImagePartial* image, unsigned int ddsCapsFlags);
int __fastcall ImagePopulateSurfaceFromHeapPixels(zVidImagePartial* image);
IDirectDrawSurface3* __fastcall ImageLazyCreateVideoMemorySurface(zVidImagePartial* image);
void __fastcall ImageEnsureSurfaceForCurrentDevice(zVidImagePartial* image);
int __fastcall ImageUploadPixelsToSurface(zVidImagePartial* image, HDC* outHdc);
int __fastcall ImageReleaseSurface(zVidImagePartial* image, HDC hdc);
void __fastcall BltSwToPrimaryRectDirect(zVidRect32* srcRect, zVidRect32* dstRect);
void __fastcall BltPrimaryToSwRectDirect(zVidRect32* srcRect, zVidRect32* dstRect);
int __fastcall
PresentDisplayModeSurface(zVidRect32* srcRect, zVidRect32* dstRect, int waitForPresent, int skipSurfaceStateSwap);
void __fastcall
BltSwToPrimaryRect(zVidImagePartial* srcImage, int srcColorKeyEnable, zVidRect32* srcRect, zVidRect32* dstRect);
int __fastcall ZBufferDepthFillRect(zVidRect32* dstRect);
int __fastcall ClearScreenAndZBufferRect(zVidRect32* dstRect, zVideo_SurfaceStatePartial* colorSurfaceState);
int __fastcall ClearSwBackbufferAndZBufferRects(zVidRect32* colorRect, zVidRect32* zRect);
void __cdecl FlipToGDIIfAttached();
int __cdecl SetDisplayMode();
int __fastcall SetVideoMode(int modeIndex);
int __cdecl VerifyFullscreenSurfaceLocks();
int __cdecl RestoreDisplaySurfaces();
int __fastcall InitFullscreenSoftwarePixelPack(IDirectDrawSurface3* displaySurface);
HRESULT __fastcall
CreateSurface3FromDesc(IDirectDraw2* directDraw, DDSURFACEDESC* desc, IDirectDrawSurface3** outSurface, int reserved);
int __cdecl CreateFullscreenSurfacesForRenderer();
int __cdecl CreateHalfResBackbufferSurfaces();
int __fastcall CreateFullscreenSoftwareSurfaces();
int __cdecl CreateFullscreenHardwareSurfaces();
int __fastcall GetHwApiDeviceFeatureFlags(int deviceIndex);
int __cdecl CreateDirectDraw2ForSelectedDevice();
int __fastcall EnumerateDirect3DDevicesForRecord(zVidHwApiDeviceRecordPartial* entry);
int __cdecl ReleaseAllInterfacesAndSurfaces();
void __fastcall VerifySurfaceStateLocking(int callerContext);
void __cdecl TeardownVideoSubsystem();
int __fastcall ReportError(int hresult, const char* sourceFile, int sourceLine);
int __fastcall PaletteSetEntries(unsigned short firstEntry, unsigned short entryCount, PALETTEENTRY* entries);
} // namespace zVideo_dd

namespace zVideo_dd3d {
void __fastcall CallClearZBufferRect(zVidRect32* rect);
void __fastcall SetPendingWireframeState(int pendingWireframeState);
void __fastcall SetPendingDitherEnable(int enabled);
int __cdecl BeginSceneAndFlushPendingRenderStates();
int __cdecl EndScene();
int __fastcall
PresentDisplayModeSurface(zVidRect32* srcRect, zVidRect32* dstRect, int waitForPresent, int blitPrimaryToSwFirst);
zVideo_TextureRecordPartial* __fastcall
CreateTextureRecord(const char* textureName, zVidImagePartial* image, int useAlpha, int clampU, int clampV);
int __fastcall CreateDeviceState();
void __fastcall SetFogEnable(int enable);
void __stdcall SetFogStart(float fogStart);
void __stdcall SetFogEnd(float fogEnd);
void __fastcall ApplyFogStateFromGlobals(float fogStart, float fogEnd, float unused);
void __cdecl UpdateFogColor();
void __stdcall SetQuadBatchDepthAndRhw(float depthAndRhw);
void __fastcall SubmitPolyFlatColor16(
    zVideo_XyzVertex* vertices,
    unsigned int packedColor16,
    int alpha,
    int renderParam,
    int vertexCount,
    int queueMode
);
void __fastcall SubmitPolyGouraudColor16(
    zVideo_XyzVertex* vertices,
    unsigned int* packedColors16,
    int alpha,
    int renderParam,
    int vertexCount,
    int queueMode
);
void __fastcall SubmitPolyColorAttr(
    zVideo_XyzVertex* vertices,
    unsigned int packedColor16,
    zVideo_ColorRgbFloat* baseColor,
    float* attr1,
    float* attr0,
    float* attr2,
    int alpha,
    int vertexCount,
    unsigned int renderParam,
    int queueMode
);
void __fastcall SubmitPolyRenderClass(
    zVideo_XyzVertex* vertices,
    zVideo_TexCoord* texCoords,
    int vertexCount,
    zVideo_RenderClass* renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
);
void __fastcall SubmitPolygon(
    zVideo_XyzVertex* vertices,
    zVideo_TexCoord* uvPairs,
    float* attr1,
    float* attr0,
    float* attr2,
    int vertexCount,
    zVideo_RenderClass* renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
);
void __fastcall SubmitPolygonLit(
    zVideo_XyzVertex* vertices,
    zVideo_TexCoord* uvPairs,
    float* attr1,
    float* attr0,
    float* attr2,
    int vertexCount,
    zVideo_RenderClass* renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
);
void __fastcall DrawPointColor16(zVideo_XyzVertex* pointPos, unsigned int packedColor16, int pointCount);
void __fastcall QueueSolidQuad(unsigned int packedColor16, double alpha, zVidRect32* clipRect);
void __cdecl FlushSortedPolys();
void __cdecl FlushQuadBatch();
void __cdecl FlushOverwritePolys();
int __fastcall FloorPowerOfTwo(int value);
zVideo_TextureRecordPartial* __cdecl TextureRecordCreate();
int __fastcall
TextureRecordLockUploadSurface(zVideo_TextureRecordPartial* textureRecord, void** outPixels, int* outPitchBytes);
void __fastcall
ConvertImagePixelsForTexture(unsigned short* dstPixels, zVidImagePartial* image, int pitchBytes, int useAlpha);
int __fastcall UploadImageToSurface(IDirectDrawSurface* uploadSurface, zVidImagePartial* image, int useAlpha);
int __fastcall TextureRecordUnlockUploadSurface(zVideo_TextureRecordPartial* textureRecord);
void __fastcall TextureRecordReleaseUploadSurfaceRef(zVideo_TextureRecordPartial* textureRecord);
void __fastcall
TextureRecordFinalizeUpload(zVideo_TextureRecordPartial* textureRecord, void* reserved, zVidImagePartial* image);
void __fastcall TextureRecordDestroy(zVideo_TextureRecordPartial* textureRecord);
} // namespace zVideo_dd3d

namespace zVideoD3D {
int __cdecl SceneEnter();
int __cdecl SceneLeave();
} // namespace zVideoD3D
