#pragma once

#include "recoil/recoil_types.h"
#include <stdio.h>

#include <windows.h>
#include <d3d.h>
#include <ddraw.h>

#include "recoil/recoil_callconv.h"

struct zColorRgb;
struct zClass_CameraDataPartial;
struct HudUiRect;
struct zTag4Partial;
struct zVec3;

extern "C" {
typedef void (RECOIL_CDECL *zVideo_ShutdownVideoSystemProc)();
typedef int (RECOIL_FASTCALL *zVideo_StatusProc)(int modeIndex);
struct zVidRect32 {
    int left;
    int top;
    int right;
    int bottom;
};
struct zVideo_SurfaceStatePartial;
struct zVideo_TextureRecordPartial;
struct zVidImagePartial;

typedef void (RECOIL_FASTCALL *zVideo_BltRectDirectProc)(zVidRect32 *srcRect, zVidRect32 *dstRect);
typedef void (RECOIL_FASTCALL *zVideo_ClearSwSurfaceAndZBufferProc)(zVidRect32 *surfaceRect,
                                                                    zVidRect32 *zRect);
typedef void (RECOIL_FASTCALL *zVideo_ClearStateSurfaceAndZBufferProc)(zVidRect32 *rect, zVideo_SurfaceStatePartial *surfaceState);
typedef int (RECOIL_FASTCALL *zVideo_AdjustSurfacesProc)(zVidRect32 *srcRect, zVidRect32 *dstRect,
                                    int waitForPresent, int blitPrimaryToSwFirst);
typedef int (RECOIL_FASTCALL *zVideo_QueryMemoryBytesProc)(int flags,
                                                                    int *totalBytes,
                                                                    int *freeBytes);
typedef zVideo_TextureRecordPartial * (RECOIL_FASTCALL *zVideo_CreateTextureRecordProc)(const char *textureName, zVidImagePartial *image, int useAlpha,
                       int clampU, int clampV);
typedef void (RECOIL_FASTCALL *zVideo_DestroyTextureRecordProc)(zVideo_TextureRecordPartial *texture);
typedef void (RECOIL_CDECL *zVideo_ReleaseAllTextureUploadSurfacesProc)();

struct zVidD3DDriverRecordPartial {
    char m_deviceName[0x20];
    char m_deviceDescription[0x60];
    GUID *pD3DDeviceGuid;
    GUID m_d3dDeviceGuidStorage;
    unsigned char m_hwDesc[0xfc];
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

struct zVideo_SurfaceLockVerifierVtbl {
    unsigned int(RECOIL_STDCALL *QueryInterface)(zVideo_SurfaceLockVerifier *self, REFIID riid,
                                                  void **object);
    unsigned int(RECOIL_STDCALL *AddRef)(zVideo_SurfaceLockVerifier *self);
    unsigned int(RECOIL_STDCALL *Release)(zVideo_SurfaceLockVerifier *self);
    unsigned int(RECOIL_STDCALL *unknown_0c)(zVideo_SurfaceLockVerifier *self);
    int(RECOIL_STDCALL *VerifySurfaceState)(zVideo_SurfaceLockVerifier *self,
                                                     zVideo_SurfaceLockVerifyArgs *args);
};

struct zVideo_SurfaceLockVerifier {
    zVideo_SurfaceLockVerifierVtbl *vtable;
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

extern int g_zVideo_PixelPack_RShift;
extern int g_zVideo_PixelPack_GShift;
extern int g_zVideo_PixelPack_BShiftTo8;
extern int g_zVideo_PixelPack_RMaskShifted;
extern int g_zVideo_PixelPack_GMaskShifted;
extern int g_zVideo_PixelPack_BMaskShifted;
extern int g_zVideo_PixelPack_RBits;
extern int g_zVideo_PixelPack_GBits;
extern int g_zVideo_PixelPack_BBits;
extern unsigned int g_zVideo_PixelPack_RMask;
extern unsigned int g_zVideo_PixelPack_GMask;
extern unsigned int g_zVideo_PixelPack_BMask;
extern int g_zVideo_TexturePixelPack_RBits;
extern int g_zVideo_TexturePixelPack_GBits;
extern int g_zVideo_TexturePixelPack_BBits;
extern int g_zVideo_TexturePixelPack_ABits;
extern unsigned int g_zVideo_TexturePixelPack_RMask;
extern unsigned int g_zVideo_TexturePixelPack_GMask;
extern unsigned int g_zVideo_TexturePixelPack_BMask;
extern unsigned int g_zVideo_TexturePixelPack_AMask;
extern int g_zVideo_TexturePixelPack_RGBBitsTotal;
extern int g_zVideo_TexturePixelPack_RGBBitsTotalMinus8;
extern int g_zVideo_TexturePixelPack_GBBitsTotalMinus8;
extern int g_zVideo_TexturePixelPack_BShiftTo8;
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
extern PALETTEENTRY g_zVideo_SystemPaletteEntries[0x100];
extern zVideo_StatusProc g_zVideo_pfnOpenVideoMode;
extern zVideo_ShutdownVideoSystemProc g_zVideo_pfnShutdownVideoSystem;
extern zVideo_StatusProc g_zVideo_pfnSetVideoMode;
extern zVideo_AdjustSurfacesProc g_zVideo_pfnAdjustSurfaces;
extern zVideo_QueryMemoryBytesProc g_zVideo_pfnQueryTextureMemoryBytes;
extern zVideo_QueryMemoryBytesProc g_zVideo_pfnQueryDeviceVideoMemoryBytes;
extern zVideo_BltRectDirectProc g_zVideo_pfnBltSwToPrimaryRectDirect;
extern zVideo_BltRectDirectProc g_zVideo_pfnBltPrimaryToSwRectDirect;
extern zVideo_ClearSwSurfaceAndZBufferProc g_zVideo_pfnClearSwSurfaceAndZBuffer;
extern zVideo_ClearStateSurfaceAndZBufferProc g_zVideo_pfnClearStateSurfaceAndZBuffer;
extern zVideo_CreateTextureRecordProc g_zVideo_pfnCreateTextureRecord;
extern unsigned int g_zVideo_pfnTextureRecordFinalizeUpload;
extern unsigned int g_zVideo_pfnTextureRecordDestroy;
extern unsigned int g_zVideo_pfnTextureRecordReleaseAllUploadSurfaces;
extern unsigned int g_zVideo_pfnImageEnsureSurfaceForCurrentDevice;
extern unsigned int g_zVideo_pfnSubmitPolyRenderClass;
extern unsigned int g_zVideo_pfnDrawPointColor16;
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
extern D3DTEXTUREHANDLE g_zVideo_D3DRenderState_TextureHandle;
extern int g_zVideo_D3DRenderState_ShadeMode;
extern int g_zVideo_D3DRenderState_AlphaBlendEnable;
extern int g_zVideo_D3DRenderState_ZWriteEnable;
extern D3DTEXTUREBLEND g_zVideo_D3DRenderState_TextureMapBlend;
extern D3DTEXTUREADDRESS g_zVideo_D3DRenderState_TextureAddressU;
extern D3DTEXTUREADDRESS g_zVideo_D3DRenderState_TextureAddressV;
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
extern unsigned int g_zVideo_pfnImageUploadPixelsToSurface;
extern unsigned int g_zVideo_pfnImageReleaseSurface;
extern IDirectDrawPalette *g_zVideo_pDDPalette;
extern HWND g_zVideo_hWnd;
extern RECT g_zVideo_CachedClientRectScreen;

RECOIL_NOINLINE unsigned int RECOIL_FASTCALL zVid_PackColorRGB(unsigned char red,
                                                                unsigned char green,
                                                                unsigned char blue);
RECOIL_NOINLINE unsigned int RECOIL_FASTCALL zVid_PackColor00RRGGBB(unsigned int color00RRGGBB);
RECOIL_NOINLINE unsigned short RECOIL_FASTCALL zVid_PackColorRgbFloats(zVideo_ColorRgbFloat *color);
RECOIL_NOINLINE void RECOIL_FASTCALL zVideo_SetClearColorPacked16(unsigned int packedColor16);
RECOIL_NOINLINE void RECOIL_FASTCALL
zVideo_SetPendingFogTargetColorFromRgb01(zVideo_ColorRgbFloat *color);
RECOIL_NOINLINE void RECOIL_FASTCALL
zVideo_SetActiveViewContext(zClass_CameraDataPartial *viewContext);
RECOIL_NOINLINE void RECOIL_FASTCALL
zVideo_UpdateProjectionStateFromCameraData(zClass_CameraDataPartial *cameraData);
RECOIL_NOINLINE int RECOIL_FASTCALL
zVideo_FrustumTestSphereClipMask(zVec3 *sphereCenter, int *clipMaskInOut,
                                 float radius);
RECOIL_NOINLINE int RECOIL_CDECL zVid_QueryCachedClientRectUpdateMaskIf3dfx();
RECOIL_NOINLINE void RECOIL_CDECL zVid_UpdateCachedClientRectIfUpdateMaskEnabled();
RECOIL_NOINLINE void RECOIL_CDECL zVideo_RestoreIconicFullscreenWindowIfNeeded();
}

namespace zVid {
RECOIL_NOINLINE void RECOIL_FASTCALL SetAccelerationOption(int accelerationOption);
RECOIL_NOINLINE void RECOIL_FASTCALL SetHwApiOption(int hwApiOption);
RECOIL_NOINLINE int RECOIL_CDECL GetAccelerationOption();
RECOIL_NOINLINE int RECOIL_CDECL GetHwApiOption();
RECOIL_NOINLINE int RECOIL_CDECL GetAcceptedDirectDrawDeviceCount();
RECOIL_NOINLINE int RECOIL_CDECL GetAcceptedHardwareRendererCount_Cached();
RECOIL_NOINLINE int RECOIL_CDECL HasAcceptedHardwareRenderer();
RECOIL_NOINLINE int RECOIL_CDECL GetTexturePackLoadState();
RECOIL_NOINLINE void RECOIL_FASTCALL SetTexturePackLoadState(int texturePackLoadState);
RECOIL_NOINLINE int RECOIL_CDECL GetVideoModeIndexFromOptions();
RECOIL_NOINLINE void RECOIL_FASTCALL SetVideoModeIndex(int modeIndex);
RECOIL_NOINLINE int RECOIL_FASTCALL QueryDeviceVideoMemoryBytes(
    int deviceIndexOrMinus1, int *totalBytes, int *freeBytes);
RECOIL_NOINLINE int RECOIL_FASTCALL QueryTextureMemoryBytes(
    int deviceIndexOrMinus1, int *totalBytes, int *freeBytes);
RECOIL_NOINLINE void RECOIL_FASTCALL SetCachedClientRectUpdateMask(int mask);
RECOIL_NOINLINE char *RECOIL_CDECL GetSelectedHwApiDescriptionOrDefault();
RECOIL_NOINLINE char *RECOIL_CDECL GetSelectedD3DDeviceNameOrDefault();
RECOIL_NOINLINE char *RECOIL_FASTCALL GetHwApiDescription(int index);
RECOIL_NOINLINE char *RECOIL_FASTCALL GetHwApiDriverName(int index);
RECOIL_NOINLINE void RECOIL_CDECL Noise_InitBuffers();
RECOIL_NOINLINE void RECOIL_CDECL Noise_ShutdownBuffers();
RECOIL_NOINLINE void RECOIL_FASTCALL DrawNoiseRect(zVidRect32 *rectOrNull, double intensity);
RECOIL_NOINLINE void RECOIL_CDECL InitFrameScratchBuffers();
RECOIL_NOINLINE int RECOIL_CDECL ShutdownFrameScratchBuffers();
} // namespace zVid

namespace zVideo_buff {
RECOIL_NOINLINE int RECOIL_FASTCALL ClipCoordToRange(int *coordPtr,
                                                              int minCoord,
                                                              int maxCoord);
RECOIL_NOINLINE zVidImagePartial *RECOIL_FASTCALL
CopySurfaceRectToImage(int sourceSelector, zVidRect32 *rect,
                       zVidImagePartial *imageOrNull);
RECOIL_NOINLINE void RECOIL_FASTCALL BltSourceToPrimaryClipped(zVidImagePartial *srcImage,
                                                               int dstX, int dstY,
                                                               int srcColorKeyEnable,
                                                               zVidRect32 *srcRect);
} // namespace zVideo_buff

namespace zVideo {
RECOIL_NOINLINE void RECOIL_FASTCALL SetFogColorFromRgb01(zVideo_ColorRgbFloat *color);
RECOIL_NOINLINE void RECOIL_FASTCALL SetFogTargetColorFromRgb01(zVideo_ColorRgbFloat *color);
RECOIL_NOINLINE void RECOIL_FASTCALL
PixelPack_SetupFromMasks(int redBits, int greenBits, int blueBits,
                         unsigned int redMask, unsigned int greenMask, unsigned int blueMask);
RECOIL_NOINLINE void RECOIL_FASTCALL TexturePixelPack_SetupFromMasks(
    int redBits, int greenBits, int blueBits, int alphaBits,
    unsigned int redMask, unsigned int greenMask, unsigned int blueMask,
    unsigned int alphaMask);
RECOIL_NOINLINE void RECOIL_FASTCALL PixelPack_GetRgbBits(int *outRBits,
                                                          int *outGBits,
                                                          int *outBBits);
RECOIL_NOINLINE void RECOIL_FASTCALL PixelPack_GetRgbMasks(unsigned int *outRMask,
                                                           unsigned int *outGMask,
                                                           unsigned int *outBMask);
RECOIL_NOINLINE void RECOIL_FASTCALL PixelPack_GetPackingParams(int *outPackedBase,
                                                                int *outSumMinus8,
                                                                int *outBShiftTo8);
RECOIL_NOINLINE int RECOIL_FASTCALL
SetRendererTypeAndActivePath(int rendererType);
RECOIL_NOINLINE int RECOIL_FASTCALL SetHalfResAdjustMode(int mode);
RECOIL_NOINLINE zVidRect32 *RECOIL_CDECL GetPrimarySurfaceRectScratch();
RECOIL_NOINLINE void *RECOIL_CDECL GetSwSurfacePixels();
RECOIL_NOINLINE int RECOIL_CDECL GetSwSurfaceWidth();
RECOIL_NOINLINE int RECOIL_CDECL GetSwSurfaceHeight();
RECOIL_NOINLINE int RECOIL_CDECL GetSwSurfacePitch();
RECOIL_NOINLINE int RECOIL_CDECL GetSwSurfaceLockedFlag();
RECOIL_NOINLINE void *RECOIL_CDECL GetPrimarySurfacePixels();
RECOIL_NOINLINE int RECOIL_CDECL GetPrimarySurfaceWidth();
RECOIL_NOINLINE int RECOIL_CDECL GetPrimarySurfaceHeight();
RECOIL_NOINLINE int RECOIL_CDECL GetPrimarySurfacePitch();
RECOIL_NOINLINE int RECOIL_CDECL GetDisplayModeBpp();
RECOIL_NOINLINE int RECOIL_FASTCALL Init_ApplyModeIndex(int modeIndex);
RECOIL_NOINLINE void RECOIL_FASTCALL Init_SetSurfaceGeometryFromModeIndex(int modeIndex);
RECOIL_NOINLINE int RECOIL_FASTCALL SetVideoMode(int modeIndex);
RECOIL_NOINLINE int RECOIL_FASTCALL InitVideoSystem(HWND hWnd,
                                                             int rendererBackend,
                                                             int fullscreen,
                                                             int modeIndex);
RECOIL_NOINLINE void RECOIL_FASTCALL CallClearSwSurfaceAndZBuffer(zVidRect32 *surfaceRect,
                                                                  zVidRect32 *zRect);
RECOIL_NOINLINE void RECOIL_FASTCALL CallClearPrimarySurfaceAndZBuffer(zVidRect32 *rect);
RECOIL_NOINLINE int RECOIL_FASTCALL ExchangeClearScreenBufferEnabled(int enable);
RECOIL_NOINLINE int RECOIL_CDECL GetClearScreenBufferEnabled();
RECOIL_NOINLINE int RECOIL_CDECL Dispatch_LockDisplayModeSurfaceState();
RECOIL_NOINLINE int RECOIL_CDECL Dispatch_UnlockDisplayModeSurfaceState();
RECOIL_NOINLINE int RECOIL_CDECL Dispatch_UnlockSwSurfaceState();
RECOIL_NOINLINE int RECOIL_CDECL Dispatch_UnlockPrimarySurfaceState();
RECOIL_NOINLINE void RECOIL_FASTCALL Fx_SetSurfaceState(void *pixels, int width,
                                                        int height,
                                                        int pitchBytes);
RECOIL_NOINLINE void RECOIL_FASTCALL FxPass3Config_QueuePrimitiveRaw(zVideoFxPass3Config *config,
                                                                     void *primitive,
                                                                     int width,
                                                                     int height,
                                                                     int pitchBytes);
RECOIL_NOINLINE void RECOIL_FASTCALL FxPass3Config_UpdateLocal(zVideoFxPass3Config *config,
                                                               float deltaTime);
RECOIL_NOINLINE void RECOIL_FASTCALL FxPass3Config_SetPrimaryElementParamsLocal(
    zVideoFxPass3Config *config, unsigned int packedColor, double primaryAlpha);
RECOIL_NOINLINE void RECOIL_FASTCALL FxPass3_SetPrimaryElementParamsLocal(unsigned int packedColor,
                                                                          double primaryAlpha);
RECOIL_NOINLINE void RECOIL_FASTCALL FxPass3Config_QueueElementLocal(
    zVideoFxPass3Config *config, int rectLeftPixels, int rectTopPixels,
    int currentRadiusPixels, int maxRadiusPixels, int extentPixels,
    float sinFreq, float sinPhase);
RECOIL_NOINLINE void RECOIL_FASTCALL FxPass3_QueueElementLocal(
    int rectLeftPixels, int rectTopPixels, int currentRadiusPixels,
    int maxRadiusPixels, int extentPixels, float sinFreq, float sinPhase);
RECOIL_NOINLINE void RECOIL_FASTCALL FxPass3_QueuePrimitive(void *primitive, int width,
                                                            int height,
                                                            int pitchBytes);
RECOIL_NOINLINE void RECOIL_FASTCALL FxPass3_SetInputRectByIndex(int index,
                                                                 HudUiRect *rectOrNull);
RECOIL_NOINLINE void RECOIL_FASTCALL FxPass3_UpdateLocal(float deltaTime);
RECOIL_NOINLINE void RECOIL_CDECL RunPostprocessOnSwBuffer();
RECOIL_NOINLINE void RECOIL_CDECL RunPostprocessOnPrimaryBuffer();
RECOIL_NOINLINE int RECOIL_FASTCALL
AdjustSurfacesIfEnabled(zVidRect32 *srcRect, zVidRect32 *dstRect, int waitForPresent,
                        int blitPrimaryToSwFirst);
RECOIL_NOINLINE void RECOIL_FASTCALL BindRendererDispatch(int rendererType,
                                                          int fullscreenOption);
RECOIL_NOINLINE void RECOIL_FASTCALL CommitHwApiDeviceSelection(int hwApiIndex);
RECOIL_NOINLINE int RECOIL_FASTCALL SelectHwApiDeviceOrFallback(int hwApiIndex);
RECOIL_NOINLINE int RECOIL_CDECL ReturnSuccessStub();
RECOIL_NOINLINE int RECOIL_CDECL ModuleInit();
RECOIL_NOINLINE int RECOIL_CDECL ShutdownVideoSystem();
RECOIL_NOINLINE int RECOIL_CDECL UpdateCachedClientRectScreenCoords();
RECOIL_NOINLINE void RECOIL_CDECL AtExitReleaseAllInterfacesAndSurfaces();
} // namespace zVideo

namespace zVid_Image {
extern zVidImagePartial g_zImage_DefaultImage;

RECOIL_NOINLINE zVidImagePartial *RECOIL_CDECL Create();
RECOIL_NOINLINE int RECOIL_FASTCALL Destroy(zVidImagePartial *image);
RECOIL_NOINLINE int RECOIL_FASTCALL ReleaseIfNotDefault(zVidImagePartial *image);
RECOIL_NOINLINE void RECOIL_FASTCALL ReleaseOwnedBuffers(zVidImagePartial *image);
RECOIL_NOINLINE void RECOIL_FASTCALL CalcPow2ScratchFields(zVidImagePartial *image);
RECOIL_NOINLINE int RECOIL_FASTCALL QueryBytesPerPixel(zVidImagePartial *image);
RECOIL_NOINLINE void RECOIL_FASTCALL ClearZeroAlphaPixelsInPlace(zVidImagePartial *image);
RECOIL_NOINLINE int RECOIL_FASTCALL SetHeaderFlagsByte(zVidImagePartial *image,
                                                                unsigned char flags);
RECOIL_NOINLINE int RECOIL_FASTCALL SetFormatCode(zVidImagePartial *image,
                                                           unsigned char formatCode);
RECOIL_NOINLINE int RECOIL_FASTCALL SetSize(zVidImagePartial *image, short width,
                                                     short height);
RECOIL_NOINLINE int RECOIL_FASTCALL QueryPixelDataBytes(zVidImagePartial *image);
RECOIL_NOINLINE int RECOIL_FASTCALL ReadHeader(FILE *file, zVidImagePartial *image);
RECOIL_NOINLINE int RECOIL_FASTCALL ReadData(FILE *file, zVidImagePartial *image,
                                                      int bytesPerPixel = 0);
RECOIL_NOINLINE zVidImagePartial *RECOIL_FASTCALL ReadFromFile(FILE *file);
RECOIL_NOINLINE void RECOIL_FASTCALL ResampleSquare(zVidImagePartial *image,
                                                    int sideLength);
RECOIL_NOINLINE void RECOIL_FASTCALL BlitToActiveTarget(zVidImagePartial *image, int dstX,
                                                        int dstY, int clipFlags,
                                                        zVidRect32 *srcRect);
} // namespace zVid_Image

namespace zVid_PaletteRemap {
RECOIL_NOINLINE int RECOIL_FASTCALL FindRecipeIndex(zVidPaletteRemapRecipe *recipe);
RECOIL_NOINLINE void RECOIL_FASTCALL ApplyRecipeToPaletteVariant(zVidPaletteRemapRecipe *recipe,
                                                                 unsigned short *sourceColors,
                                                                 int colorCount,
                                                                 int variantIndex,
                                                                 unsigned short *destColors);
} // namespace zVid_PaletteRemap

extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zVid_Image_SetPixels(zVidImagePartial *image, void *pixels, char *alphaMap);

extern "C" RECOIL_NOINLINE zVidImagePartial *RECOIL_FASTCALL
zVideo_buff_CaptureSurfaceToImage(int sourceSelector);
extern "C" RECOIL_NOINLINE unsigned short *RECOIL_FASTCALL
zVid_PaletteRemap_BuildAllRecipeVariantsForPalette(unsigned short *palette, int colorCount);
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zVid_PaletteRemap_BuildPaletteVariant(zVidPaletteRemapRecipe *recipe);
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zVid_PaletteRemap_FindRecipeIndexFromRgb(zColorRgb *rgb);
extern "C" RECOIL_NOINLINE FILE *RECOIL_FASTCALL
zVid_TexturePackEntry_LoadFromFile(zVidTexturePackEntry *entry);
extern "C" RECOIL_NOINLINE void RECOIL_CDECL zVid_TexturePack_EnsureDefaultImagePackLoaded();
extern "C" RECOIL_NOINLINE RECOIL_NO_GS void RECOIL_CDECL
zVid_TexturePack_EnsureBuiltinTexturePacksLoaded();
extern "C" RECOIL_NOINLINE zVidImagePartial *RECOIL_FASTCALL
zVid_TexturePack_LoadImageByName(const char *imageName);
extern "C" RECOIL_NOINLINE zVidImagePartial *RECOIL_FASTCALL
zVid_TexturePack_LoadBuiltinImageByName(const char *imageName);

namespace zVid_TexturePack {
RECOIL_NOINLINE void RECOIL_CDECL ShutdownBuiltinPacks();
RECOIL_NOINLINE void RECOIL_CDECL Shutdown();
} // namespace zVid_TexturePack

namespace zVideo_dd {
RECOIL_NOINLINE int RECOIL_CDECL GetAcceptedDirectDrawDeviceCountCached();
BOOL CALLBACK EnumDirectDrawDeviceCallback(GUID *guid, LPSTR driverDescription, LPSTR driverName,
                                           LPVOID context);
HRESULT CALLBACK EnumDirect3DDeviceCallback(GUID *guid, LPSTR deviceDescription, LPSTR deviceName,
                                            D3DDEVICEDESC *hwDesc, D3DDEVICEDESC *helDesc,
                                            LPVOID context);
RECOIL_NOINLINE int RECOIL_CDECL PrepareWindowForMode();
RECOIL_NOINLINE int RECOIL_FASTCALL OpenVideoMode(int modeIndex);
RECOIL_NOINLINE int RECOIL_CDECL RunDirectDrawDeviceEnumeration();
RECOIL_NOINLINE void RECOIL_CDECL StartupEnumerateAndDefaultSelect();
RECOIL_NOINLINE int RECOIL_CDECL ShutdownVideoSystem();
RECOIL_NOINLINE int RECOIL_FASTCALL
LockDirectDrawSurface(IDirectDrawSurface3 *surface, DDSURFACEDESC *outLockedSurfaceDesc);
RECOIL_NOINLINE int RECOIL_FASTCALL UnlockDirectDrawSurface(IDirectDrawSurface3 *surface);
RECOIL_NOINLINE int RECOIL_FASTCALL LockSurface_WaitRestore(IDirectDrawSurface3 *surface,
                                                                     DDSURFACEDESC *lockedDescOut);
RECOIL_NOINLINE int RECOIL_FASTCALL
UnlockSurface_WaitRestore(IDirectDrawSurface3 *surface);
RECOIL_NOINLINE int RECOIL_FASTCALL
LockSurfaceState(zVideo_SurfaceStatePartial *surfaceState);
RECOIL_NOINLINE int RECOIL_FASTCALL
UnlockSurfaceState(zVideo_SurfaceStatePartial *surfaceState);
RECOIL_NOINLINE IDirectDrawSurface3 *RECOIL_FASTCALL
Image_LazyCreateBackingSurface(zVidImagePartial *image, unsigned int ddsCapsFlags);
RECOIL_NOINLINE int RECOIL_FASTCALL
Image_PopulateSurfaceFromHeapPixels(zVidImagePartial *image);
RECOIL_NOINLINE IDirectDrawSurface3 *RECOIL_FASTCALL
Image_LazyCreateVideoMemorySurface(zVidImagePartial *image);
RECOIL_NOINLINE void RECOIL_FASTCALL Image_EnsureSurfaceForCurrentDevice(zVidImagePartial *image);
RECOIL_NOINLINE int RECOIL_FASTCALL Image_UploadPixelsToSurface(zVidImagePartial *image,
                                                                         HDC *outHdc);
RECOIL_NOINLINE int RECOIL_FASTCALL Image_ReleaseSurface(zVidImagePartial *image, HDC hdc);
RECOIL_NOINLINE void RECOIL_FASTCALL BltSwToPrimaryRectDirect(zVidRect32 *srcRect,
                                                              zVidRect32 *dstRect);
RECOIL_NOINLINE void RECOIL_FASTCALL BltPrimaryToSwRectDirect(zVidRect32 *srcRect,
                                                              zVidRect32 *dstRect);
RECOIL_NOINLINE void RECOIL_FASTCALL BltSwToPrimaryRect(zVidImagePartial *srcImage,
                                                        int srcColorKeyEnable,
                                                        zVidRect32 *srcRect, zVidRect32 *dstRect);
RECOIL_NOINLINE void RECOIL_FASTCALL ZBuffer_DepthFillRect(zVidRect32 *dstRect);
RECOIL_NOINLINE void RECOIL_FASTCALL
ClearScreenAndZBufferRect(zVidRect32 *dstRect, zVideo_SurfaceStatePartial *colorSurfaceState);
RECOIL_NOINLINE void RECOIL_FASTCALL ClearSwBackbufferAndZBufferRects(zVidRect32 *colorRect,
                                                                      zVidRect32 *zRect);
RECOIL_NOINLINE void RECOIL_CDECL FlipToGDIIfAttached();
RECOIL_NOINLINE int RECOIL_CDECL SetDisplayMode();
RECOIL_NOINLINE int RECOIL_FASTCALL SetVideoMode(int modeIndex);
RECOIL_NOINLINE int RECOIL_CDECL VerifyFullscreenSurfaceLocks();
RECOIL_NOINLINE int RECOIL_CDECL RestoreDisplaySurfaces();
RECOIL_NOINLINE int RECOIL_FASTCALL
InitFullscreenSoftwarePixelPack(IDirectDrawSurface3 *displaySurface);
RECOIL_NOINLINE HRESULT RECOIL_FASTCALL CreateSurface3FromDesc(IDirectDraw2 *directDraw,
                                                               DDSURFACEDESC *desc,
                                                               IDirectDrawSurface3 **outSurface);
RECOIL_NOINLINE int RECOIL_CDECL CreateFullscreenSurfacesForRenderer();
RECOIL_NOINLINE int RECOIL_CDECL CreateHalfResBackbufferSurfaces();
RECOIL_NOINLINE int RECOIL_CDECL CreateFullscreenSoftwareSurfaces();
RECOIL_NOINLINE int RECOIL_CDECL CreateFullscreenHardwareSurfaces();
RECOIL_NOINLINE int RECOIL_CDECL CreateDirectDraw2ForSelectedDevice();
RECOIL_NOINLINE int RECOIL_FASTCALL
EnumerateDirect3DDevicesForRecord(zVidHwApiDeviceRecordPartial *entry);
RECOIL_NOINLINE int RECOIL_CDECL ReleaseAllInterfacesAndSurfaces();
RECOIL_NOINLINE void RECOIL_FASTCALL VerifySurfaceStateLocking(int callerContext);
RECOIL_NOINLINE void RECOIL_CDECL TeardownVideoSubsystem();
RECOIL_NOINLINE int RECOIL_FASTCALL ReportError(int hresult,
                                                         const char *sourceFile,
                                                         int sourceLine);
RECOIL_NOINLINE int RECOIL_FASTCALL PaletteSetEntries(unsigned short firstEntry,
                                                               unsigned short entryCount,
                                                               PALETTEENTRY *entries);
} // namespace zVideo_dd

namespace zVideo_dd3d {
RECOIL_NOINLINE void RECOIL_FASTCALL CallClearZBufferRect(zVidRect32 *rect);
RECOIL_NOINLINE void RECOIL_FASTCALL SetPendingDitherEnable(int enabled);
RECOIL_NOINLINE int RECOIL_FASTCALL
PresentDisplayModeSurface(zVidRect32 *srcRect, zVidRect32 *dstRect, int waitForPresent,
                          int blitPrimaryToSwFirst);
RECOIL_NOINLINE zVideo_TextureRecordPartial *RECOIL_FASTCALL
CreateTextureRecord(const char *textureName, zVidImagePartial *image, int useAlpha,
                    int clampU, int clampV);
RECOIL_NOINLINE int RECOIL_CDECL CreateDeviceState();
RECOIL_NOINLINE void RECOIL_FASTCALL SetFogEnable(int enable);
RECOIL_NOINLINE void RECOIL_STDCALL SetFogStart(float fogStart);
RECOIL_NOINLINE void RECOIL_STDCALL SetFogEnd(float fogEnd);
RECOIL_NOINLINE void RECOIL_STDCALL ApplyFogStateFromGlobals(float fogStart, float fogEnd,
                                                             float unused);
RECOIL_NOINLINE void RECOIL_CDECL UpdateFogColor();
RECOIL_NOINLINE void RECOIL_STDCALL SetQuadBatchDepthAndRhw(float depthAndRhw);
RECOIL_NOINLINE void RECOIL_FASTCALL
SubmitPolyFlatColor16(zVideo_XyzVertex *vertices, unsigned int packedColor16, int alpha,
                      int renderParam, int vertexCount, int queueMode);
RECOIL_NOINLINE void RECOIL_FASTCALL SubmitPolyGouraudColor16(
    zVideo_XyzVertex *vertices, unsigned int *packedColors16, int alpha,
    int renderParam, int vertexCount, int queueMode);
RECOIL_NOINLINE void RECOIL_FASTCALL SubmitPolyColorAttr(
    zVideo_XyzVertex *vertices, unsigned int packedColor16, zVideo_ColorRgbFloat *baseColor,
    float *attr1, float *attr0, float *attr2, int alpha, int vertexCount,
    unsigned int renderParam, int queueMode);
RECOIL_NOINLINE void RECOIL_FASTCALL SubmitPolyRenderClass(zVideo_XyzVertex *vertices,
                                                           zVideo_TexCoord *texCoords,
                                                           int vertexCount,
                                                           zVideo_RenderClass *renderClass,
                                                           unsigned int renderParam, float alpha,
                                                           int queueMode);
RECOIL_NOINLINE void RECOIL_FASTCALL
SubmitPolygon(zVideo_XyzVertex *vertices, zVideo_TexCoord *uvPairs, float *attr1, float *attr0,
              float *attr2, int vertexCount, zVideo_RenderClass *renderClass,
              unsigned int renderParam, float alpha, int queueMode);
RECOIL_NOINLINE void RECOIL_FASTCALL
SubmitPolygonLit(zVideo_XyzVertex *vertices, zVideo_TexCoord *uvPairs, float *attr1, float *attr0,
                 float *attr2, int vertexCount, zVideo_RenderClass *renderClass,
                 unsigned int renderParam, float alpha, int queueMode);
RECOIL_NOINLINE void RECOIL_FASTCALL DrawPointColor16(zVideo_XyzVertex *pointPos,
                                                      unsigned int packedColor16,
                                                      int pointCount);
RECOIL_NOINLINE void RECOIL_FASTCALL QueueSolidQuad(unsigned int packedColor16,
                                                    zVidRect32 *clipRect, double alpha);
RECOIL_NOINLINE void RECOIL_CDECL FlushSortedPolys();
RECOIL_NOINLINE void RECOIL_CDECL FlushQuadBatch();
RECOIL_NOINLINE void RECOIL_CDECL FlushOverwritePolys();
RECOIL_NOINLINE int RECOIL_FASTCALL FloorPowerOfTwo(int value);
RECOIL_NOINLINE zVideo_TextureRecordPartial *RECOIL_CDECL TextureRecord_Create();
RECOIL_NOINLINE int RECOIL_FASTCALL TextureRecord_LockUploadSurface(
    zVideo_TextureRecordPartial *textureRecord, void **outPixels, int *outPitchBytes);
RECOIL_NOINLINE void RECOIL_FASTCALL ConvertImagePixelsForTexture(unsigned short *dstPixels,
                                                                  zVidImagePartial *image,
                                                                  int pitchBytes,
                                                                  int useAlpha);
RECOIL_NOINLINE int RECOIL_FASTCALL UploadImageToSurface(IDirectDrawSurface *uploadSurface,
                                                                  zVidImagePartial *image,
                                                                  int useAlpha);
RECOIL_NOINLINE int RECOIL_FASTCALL
TextureRecord_UnlockUploadSurface(zVideo_TextureRecordPartial *textureRecord);
RECOIL_NOINLINE void RECOIL_FASTCALL
TextureRecord_ReleaseUploadSurfaceRef(zVideo_TextureRecordPartial *textureRecord);
RECOIL_NOINLINE void RECOIL_FASTCALL
TextureRecord_FinalizeUpload(zVideo_TextureRecordPartial *textureRecord, zVidImagePartial *image);
RECOIL_NOINLINE void RECOIL_FASTCALL
TextureRecord_Destroy(zVideo_TextureRecordPartial *textureRecord);
} // namespace zVideo_dd3d
