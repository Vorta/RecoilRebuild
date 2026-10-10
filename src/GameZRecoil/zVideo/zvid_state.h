#pragma once

#include "GameZRecoil/zVideo/zvid.h"

extern zVideo_GlobalState g_zVideo_GlobalStateStorage;

#define g_zVideo_RendererType (g_zVideo_GlobalStateStorage.rendererType)
#define g_zVideo_FullscreenOption (g_zVideo_GlobalStateStorage.fullscreenOption)
#define g_zVideo_UseHalfResBackbuffer (g_zVideo_GlobalStateStorage.useHalfResBackbuffer)
#define g_zVideo_HalfResAdjustMode (g_zVideo_GlobalStateStorage.halfResAdjustMode)
#define g_zVideo_ClearScreenBufferEnabled (g_zVideo_GlobalStateStorage.clearScreenBufferEnabled)
#define g_zVideo_PrimaryHasAttachedBackbuffer (g_zVideo_GlobalStateStorage.primaryHasAttachedBackbuffer)
#define g_zVideo_PendingWireframeState (g_zVideo_GlobalStateStorage.pendingWireframeState)
#define g_zVideo_PendingDitherEnable (g_zVideo_GlobalStateStorage.pendingDitherEnable)
#define g_zVideo_D3DColorNormalizeChannelIndex (g_zVideo_GlobalStateStorage.d3dColorNormalizeChannelIndex)
#define g_zVideo_AdjustSurfacesDisableGate (g_zVideo_GlobalStateStorage.adjustSurfacesDisableGate)
#define g_zVideo_D3DSceneDepth (g_zVideo_GlobalStateStorage.d3dSceneDepth)
#define gVideo_resolutionMenuValid (g_zVideo_GlobalStateStorage.resolutionMenuValid)
#define g_zVideo_DisplayModeBpp (g_zVideo_GlobalStateStorage.displayModeBpp)
#define g_zVideo_IsInitialized (g_zVideo_GlobalStateStorage.isInitialized)
#define g_zVideo_PixelPack (g_zVideo_GlobalStateStorage.pixelPack)
#define g_zVideo_TexturePixelPack_RBits (g_zVideo_GlobalStateStorage.texturePixelPackRBits)
#define g_zVideo_TexturePixelPack_GBits (g_zVideo_GlobalStateStorage.texturePixelPackGBits)
#define g_zVideo_TexturePixelPack_BBits (g_zVideo_GlobalStateStorage.texturePixelPackBBits)
#define g_zVideo_TexturePixelPack_ABits (g_zVideo_GlobalStateStorage.texturePixelPackABits)
#define g_zVideo_TexturePixelPack_RMask (g_zVideo_GlobalStateStorage.texturePixelPackRMask)
#define g_zVideo_TexturePixelPack_GMask (g_zVideo_GlobalStateStorage.texturePixelPackGMask)
#define g_zVideo_TexturePixelPack_BMask (g_zVideo_GlobalStateStorage.texturePixelPackBMask)
#define g_zVideo_TexturePixelPack_AMask (g_zVideo_GlobalStateStorage.texturePixelPackAMask)
#define g_zVideo_TexturePixelPack_RGBBitsTotalMinus8 (g_zVideo_GlobalStateStorage.texturePixelPackRGBBitsTotalMinus8)
#define g_zVideo_TexturePixelPack_GBBitsTotalMinus8 (g_zVideo_GlobalStateStorage.texturePixelPackGBBitsTotalMinus8)
#define g_zVideo_TexturePixelPack_BShiftTo8 (g_zVideo_GlobalStateStorage.texturePixelPackBShiftTo8)
#define g_zVideo_TexturePixelPack_RGBBitsTotal (g_zVideo_GlobalStateStorage.texturePixelPackRGBBitsTotal)
#define g_zVideo_TexturePixelPack_RMaskShifted (g_zVideo_GlobalStateStorage.texturePixelPackRMaskShifted)
#define g_zVideo_TexturePixelPack_GMaskShifted (g_zVideo_GlobalStateStorage.texturePixelPackGMaskShifted)
#define g_zVideo_TexturePixelPack_BMaskShifted (g_zVideo_GlobalStateStorage.texturePixelPackBMaskShifted)
#define g_zVideo_TexturePixelPack_NonRgbMaskShifted (g_zVideo_GlobalStateStorage.texturePixelPackNonRgbMaskShifted)
#define g_zVideo_hWnd (g_zVideo_GlobalStateStorage.hWnd)
#define g_zVideo_ClearColorPacked16 (g_zVideo_GlobalStateStorage.clearColorPacked16)
#define g_zVideo_FogColorPendingR255 (g_zVideo_GlobalStateStorage.fogColorPendingR255)
#define g_zVideo_FogColorPendingG255 (g_zVideo_GlobalStateStorage.fogColorPendingG255)
#define g_zVideo_FogColorPendingB255 (g_zVideo_GlobalStateStorage.fogColorPendingB255)
#define g_zVideo_D3DColorAttrBiasR (g_zVideo_GlobalStateStorage.d3dColorAttrBiasR)
#define g_zVideo_D3DColorAttrBiasG (g_zVideo_GlobalStateStorage.d3dColorAttrBiasG)
#define g_zVideo_D3DColorAttrBiasB (g_zVideo_GlobalStateStorage.d3dColorAttrBiasB)
#define g_zVideo_FogTargetColorR255 (g_zVideo_GlobalStateStorage.fogTargetColorR255)
#define g_zVideo_FogTargetColorG255 (g_zVideo_GlobalStateStorage.fogTargetColorG255)
#define g_zVideo_FogTargetColorB255 (g_zVideo_GlobalStateStorage.fogTargetColorB255)
#define g_zVideo_FogColorAppliedR255 (g_zVideo_GlobalStateStorage.fogColorAppliedR255)
#define g_zVideo_FogColorAppliedG255 (g_zVideo_GlobalStateStorage.fogColorAppliedG255)
#define g_zVideo_FogColorAppliedB255 (g_zVideo_GlobalStateStorage.fogColorAppliedB255)
#define g_zVideo_SwSurfaceState (g_zVideo_GlobalStateStorage.swSurfaceState)
#define g_zVideo_PrimarySurfaceState (g_zVideo_GlobalStateStorage.primarySurfaceState)
#define g_zVideo_DisplayModeSurfaceState (g_zVideo_GlobalStateStorage.displayModeSurfaceState)
#define g_zVideo_PalettePathBuffer (g_zVideo_GlobalStateStorage.palettePathBuffer)
#define g_zVideo_PaletteBrightnessLevel (g_zVideo_GlobalStateStorage.paletteBrightnessLevel)
#define g_zVideo_PaletteFileEntries (g_zVideo_GlobalStateStorage.paletteFileEntries)
#define g_zVideo_SystemPaletteEntries (g_zVideo_GlobalStateStorage.systemPaletteEntries)
#define g_zVideo_CachedClientRectScreen (g_zVideo_GlobalStateStorage.cachedClientRectScreen)
#define g_zVideo_NumAcceptedDirectDrawDevices (g_zVideo_GlobalStateStorage.numAcceptedDirectDrawDevices)
#define g_zVid_AcceptedHardwareRendererCount (g_zVideo_GlobalStateStorage.acceptedHardwareRendererCount)
#define g_zVideo_SortedPolyQueueCount (g_zVideo_GlobalStateStorage.sortedPolyQueueCount)
#define g_zVideo_SortedPolyDrawOrder (g_zVideo_GlobalStateStorage.sortedPolyDrawOrder)
#define g_zVideo_OverwriteQueueCount (g_zVideo_GlobalStateStorage.overwriteQueueCount)
#define g_zVideo_DefaultTextureRecord (g_zVideo_GlobalStateStorage.defaultTextureRecord)
#define g_zVideo_pfnOpenVideoMode (g_zVideo_GlobalStateStorage.pfnOpenVideoMode)
#define g_zVideo_pfnShutdownVideoSystem (g_zVideo_GlobalStateStorage.pfnShutdownVideoSystem)
#define g_zVideo_pfnAdjustSurfaces (g_zVideo_GlobalStateStorage.pfnAdjustSurfaces)
#define g_zVideo_pfnPaletteSetEntries (g_zVideo_GlobalStateStorage.pfnPaletteSetEntries)
#define g_zVideo_pfnSetVideoMode (g_zVideo_GlobalStateStorage.pfnSetVideoMode)
#define g_zVideo_pfnUnlockSurfaceState (g_zVideo_GlobalStateStorage.pfnUnlockSurfaceState)
#define g_zVideo_pfnLockSurfaceState (g_zVideo_GlobalStateStorage.pfnLockSurfaceState)
#define g_zVideo_pfnClearStateSurfaceAndZBuffer (g_zVideo_GlobalStateStorage.pfnClearStateSurfaceAndZBuffer)
#define g_zVideo_pfnClearSwSurfaceAndZBuffer (g_zVideo_GlobalStateStorage.pfnClearSwSurfaceAndZBuffer)
#define g_zVideo_pfnClearZBufferRect (g_zVideo_GlobalStateStorage.pfnClearZBufferRect)
#define g_zVideo_pfnUpdateFogColor (g_zVideo_GlobalStateStorage.pfnUpdateFogColor)
#define g_zVideo_pDirectDraw2 (g_zVideo_GlobalStateStorage.pDirectDraw2)
#define g_zVideo_pClipper (g_zVideo_GlobalStateStorage.pClipper)
#define g_zVideo_pPageUnlockSurface (g_zVideo_GlobalStateStorage.pPageUnlockSurface)
#define g_zVideo_pDDPalette (g_zVideo_GlobalStateStorage.pDDPalette)
#define g_zVideo_pSurfaceLockVerifier (g_zVideo_GlobalStateStorage.pSurfaceLockVerifier)
#define g_zVideo_pD3D2 (g_zVideo_GlobalStateStorage.pD3D2)
#define g_zVideo_pD3DDevice (g_zVideo_GlobalStateStorage.pD3DDevice)
#define g_zVideo_pZBufferSurface (g_zVideo_GlobalStateStorage.pZBufferSurface)
#define g_zVideo_pZBufferAttachSurface (g_zVideo_GlobalStateStorage.pZBufferAttachSurface)
#define g_zVideo_pD3DViewport2 (g_zVideo_GlobalStateStorage.pD3DViewport2)
#define g_zVideo_pD3DMaterial2 (g_zVideo_GlobalStateStorage.pD3DMaterial2)
#define g_zVideo_D3DMaterialHandle (g_zVideo_GlobalStateStorage.d3dMaterialHandle)
#define g_zVideo_D3DRenderStateCache (g_zVideo_GlobalStateStorage.d3dRenderStateCache)
#define g_zVideo_CachedFogEnableRenderState (g_zVideo_GlobalStateStorage.cachedFogEnableRenderState)
#define g_zVideo_CachedFogModeLightState (g_zVideo_GlobalStateStorage.cachedFogModeLightState)
#define g_zVideo_CachedFogStartLightStateValue (g_zVideo_GlobalStateStorage.cachedFogStartLightStateValue)
#define g_zVideo_CachedFogEndLightStateValue (g_zVideo_GlobalStateStorage.cachedFogEndLightStateValue)
#define g_zVideo_D3DHalDeviceDesc (g_zVideo_GlobalStateStorage.d3dHalDeviceDesc)
#define g_zVideo_D3DHelDeviceDesc (g_zVideo_GlobalStateStorage.d3dHelDeviceDesc)
#define g_zVideo_QuadBatchCount (g_zVideo_GlobalStateStorage.quadBatchCount)
#define g_zVideo_QuadBatchItemsBase (g_zVideo_GlobalStateStorage.quadBatchItems)
#define g_zVideo_pSelectedHwApiDeviceRecord (g_zVideo_GlobalStateStorage.pSelectedHwApiDeviceRecord)
#define g_zVideo_HwApiDeviceTable (g_zVideo_GlobalStateStorage.hwApiDeviceTable)
#define g_zVideo_pSelectedD3DDeviceInfo (g_zVideo_GlobalStateStorage.pSelectedD3DDeviceInfo)
#define g_zVideo_DDrawCapsHal (g_zVideo_GlobalStateStorage.ddrawCapsHal)
#define g_zVideo_DDrawCapsHel (g_zVideo_GlobalStateStorage.ddrawCapsHel)
#define g_zVideo_SurfaceLockVerifyFlags (g_zVideo_GlobalStateStorage.surfaceLockVerifyFlags)
#define g_zVideo_SurfaceLockVerifyContext (g_zVideo_GlobalStateStorage.surfaceLockVerifyContext)
#define g_zVideo_D3DSubmitTempVertices (g_zVideo_GlobalStateStorage.d3dSubmitTempVertices)
#define g_zVideo_SortedPolyQueueBase (g_zVideo_GlobalStateStorage.sortedPolyQueue)
#define g_zVideo_OverwriteQueueBase (g_zVideo_GlobalStateStorage.overwriteQueue)

/*
 * C declarations of the zVideo API for the zVideo units (C++ units see the same
 * functions with C linkage in zvid.h's namespaces; the other C units see only
 * the zvid.h C view).
 */

int __cdecl GetAcceptedDirectDrawDeviceCount();
int __cdecl GetAcceptedHardwareRendererCount();
int __cdecl GetAcceptedHardwareRendererCountCached();
int __fastcall QueryDeviceVideoMemoryBytes(int deviceIndexOrMinus1, int* totalBytes, int* freeBytes);
int __fastcall QueryTextureMemoryBytes(int deviceIndexOrMinus1, int* totalBytes, int* freeBytes);
char* __cdecl GetSelectedHwApiDescriptionOrDefault();
char* __cdecl GetSelectedD3DDeviceNameOrDefault();
char* __fastcall GetHwApiDescription(int index);
char* __fastcall GetHwApiDriverName(int index);

int __fastcall ClipCoordToRange(int* coordPtr, int minCoord, int maxCoord);
zVidImagePartial* __fastcall
CopySurfaceRectToImage(int sourceSelector, zVidRect32* rect, zVidImagePartial* imageOrNull);
void __fastcall
BltSourceToPrimaryClipped(zVidImagePartial* srcImage, int dstX, int dstY, int srcColorKeyEnable, zVidRect32* srcRect);

void __fastcall SetFogTargetColorFromRgb01(zVideo_ColorRgbFloat* color);
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
zVidRect32* __cdecl GetPrimarySurfaceRectScratch();
void* __cdecl GetSwSurfacePixels();
int __cdecl GetSwSurfaceWidth();
int __cdecl GetSwSurfaceHeight();
int __cdecl GetSwSurfacePitch();
int __cdecl GetSwSurfaceLockedFlag();
void* __cdecl GetPrimarySurfacePixels();
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
void __fastcall FxPass3QueuePrimitive(void* primitive, int width, int height, int pitchBytes);
int __cdecl RunPostprocessOnSwBuffer();
int __cdecl RunPostprocessOnPrimaryBuffer();
int __fastcall
AdjustSurfacesIfEnabled(zVidRect32* srcRect, zVidRect32* dstRect, int waitForPresent, int blitPrimaryToSwFirst);
void __fastcall BindRendererDispatch(int rendererType, int fullscreenOption);
void __fastcall CommitHwApiDeviceSelection(int hwApiIndex);
int __fastcall SelectHwApiDeviceOrFallback(int hwApiIndex);
int __cdecl ReturnSuccessStub();
int __cdecl ModuleInit();
int __cdecl ShutdownVideoSystem();
int __fastcall UpdateCachedClientRectScreenCoords();
void __cdecl AtExitReleaseAllInterfacesAndSurfaces();

zVidImagePartial* __cdecl Create();
int __fastcall SetSize(zVidImagePartial* image, short width, short height);
void __fastcall ResampleSquare(zVidImagePartial* image, int sideLength);

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
void __fastcall
BltSwToPrimaryRect(zVidImagePartial* srcImage, int srcColorKeyEnable, zVidRect32* srcRect, zVidRect32* dstRect);
int __fastcall ZBufferDepthFillRect(zVidRect32* dstRect);
int __fastcall ClearScreenAndZBufferRect(zVidRect32* dstRect, zVideo_SurfaceStatePartial* colorSurfaceState);
int __fastcall ClearSwBackbufferAndZBufferRects(zVidRect32* colorRect, zVidRect32* zRect);
void __cdecl FlipToGDIIfAttached();
int __cdecl SetDisplayMode();
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
int __cdecl zVideoddShutdownVideoSystem();
int __fastcall zVideoddSetVideoMode(int modeIndex);
int __fastcall zVideoddPresentDisplayModeSurface(
    zVidRect32* srcRect,
    zVidRect32* dstRect,
    int waitForPresent,
    int skipSurfaceStateSwap
);

void __fastcall CallClearZBufferRect(zVidRect32* rect);
void __fastcall SetPendingWireframeState(int pendingWireframeState);
void __fastcall SetPendingDitherEnable(int enabled);
int __cdecl BeginSceneAndFlushPendingRenderStates();
int __cdecl EndScene();
zVideo_TextureRecordPartial* __fastcall
CreateTextureRecord(const char* textureName, zVidImagePartial* image, int useAlpha, int clampU, int clampV);
int __fastcall CreateDeviceState();
void __fastcall SetFogEnable(int enable);
void __stdcall SetFogStart(float fogStart);
void __stdcall SetFogEnd(float fogEnd);
void __fastcall ApplyFogStateFromGlobals(float fogStart, float fogEnd, float unused);
void __cdecl UpdateFogColor();
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
int __fastcall zVideodd3dPresentDisplayModeSurface(
    zVidRect32* srcRect,
    zVidRect32* dstRect,
    int waitForPresent,
    int blitPrimaryToSwFirst
);

int __cdecl SceneEnter();
int __cdecl SceneLeave();

/*
 * C view of the surface-lock verifier COM interface (zvid.h declares the C++
 * struct): VerifySurfaceStateLocking and TeardownVideoSubsystem call it through
 * the SDK interface macros, as the DirectX headers declare their interfaces.
 */
#undef INTERFACE
#define INTERFACE zVideo_SurfaceLockVerifier
DECLARE_INTERFACE_(zVideo_SurfaceLockVerifier, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** object) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(Unknown0c)(THIS) PURE;
    STDMETHOD(VerifySurfaceState)(THIS_ zVideo_SurfaceLockVerifyArgs * args) PURE;
};
#undef INTERFACE
