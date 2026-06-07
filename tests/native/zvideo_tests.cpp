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

struct zVideoFxPass3Element {
    HudUiElement base;
    HudUiRect *clipRectOrNull;

    void Draw();
};

struct zVideoFxPass3RootElement : zVideoFxPass3Element {
    unsigned short packedColor16;
    unsigned char unknown_3a[0x06];
    double alpha;

    void ApplyOverlayRect();
};

struct zVideoFxPass3Slot : zVideoFxPass3Element {
    int currentRadius;
    int maxRadius;
    int extent;
    float sinFreq;
    float sinPhase;

    zVideoFxPass3Slot * Constructor();
    void ApplyToCurrentSurface();
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

    zVideoFxPass3Config * Constructor();
    void Destructor();
    static void CrtInitGlobalSingleton();
    static zVideoFxPass3Config *ConstructGlobalSingleton();
    static void DestroyGlobalSingleton();
};

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
int g_fxPass3DrawBaseCount;
int g_fxPass3ApplyCount;
HudUiRect *g_fxPass3ApplyRects[4];
int g_zVideoRenderFrameFlushSortedCount;
int g_zVideoRenderFrameFlushOverwriteCount;
int g_zVideoRenderFrameFlushQuadCount;
int g_zVideoRenderFrameClearRectCount;
zVidRect32 g_zVideoRenderFrameClearRects[4];
    int g_zVideoTestLockSurfaceCount;
    zVideo_SurfaceStatePartial *g_zVideoTestLockSurfaceState;
    int g_zVideoTestUnlockSurfaceCount;
    zVideo_SurfaceStatePartial *g_zVideoTestUnlockSurfaceState;

int __fastcall CapturePaletteSetEntries(unsigned short firstEntry,
                                             unsigned short entryCount,
                                             PALETTEENTRY *entries) {
    ++g_zVideoPaletteCaptureCallCount;
    g_zVideoPaletteCaptureFirstEntry = firstEntry;
    g_zVideoPaletteCaptureEntryCount = entryCount;
    std::memcpy(g_zVideoPaletteCaptureEntries, entries, sizeof(g_zVideoPaletteCaptureEntries));
    return g_zVideoPaletteCaptureReturnValue;
}

void ResetFxPass3DrawCapture() {
    g_fxPass3DrawBaseCount = 0;
    g_fxPass3ApplyCount = 0;
    for (int i = 0; i < 4; ++i) {
        g_fxPass3ApplyRects[i] = nullptr;
    }
}

void __fastcall CaptureFxPass3DrawBase(zVideoFxPass3Element *) {
    ++g_fxPass3DrawBaseCount;
}

void __fastcall CaptureFxPass3ApplyCurrentInput(zVideoFxPass3Element *element) {
    if (g_fxPass3ApplyCount < 4) {
        g_fxPass3ApplyRects[g_fxPass3ApplyCount] = element->clipRectOrNull;
    }
    ++g_fxPass3ApplyCount;
}

std::uint16_t ExpectedFramebufferBlend565(
    std::uint16_t dstPixel,
    std::uint16_t srcPixel,
    int alpha
) {
    const std::int32_t dstColor = static_cast<std::int16_t>(dstPixel);
    const std::int32_t srcColor = srcPixel;
    const std::int32_t greenDelta = (((srcColor & 0x07e0) - (dstColor & 0x07e0)) * alpha) >> 8;
    const std::int32_t redDelta = (((srcColor & 0xf800) - (dstColor & 0xf800)) * alpha) >> 8;
    std::int32_t blended = dstColor + (redDelta & 0xfffff800);
    const std::int32_t blueDelta = (((srcColor & 0x001f) - (blended & 0x001f)) * alpha) >> 8;
    blended += (greenDelta & 0xffffffe0) + blueDelta;
    return static_cast<std::uint16_t>(blended);
}

struct FakeD3DDevice2Object {
    void **vtable;
};

struct FakeD3D2Object {
    void **vtable;
};

struct FakeD3DViewport2Object {
    void **vtable;
};

struct FakeD3DMaterial2Object {
    void **vtable;
};

struct FakeD3DTexture2Object {
    void **vtable;
};

struct FakeComObject {
    void **vtable;
};

struct FakeDirectDrawObject {
    void **vtable;
};

struct FakeDirectDraw2Object {
    void **vtable;
};

struct FakeDirectDrawClipperObject {
    void **vtable;
};

struct FakeDirectDrawPaletteObject {
    void **vtable;
};

struct FakeDirectDrawSurfaceObject {
    void **vtable;
};

struct FakeDirectDrawSurface3Object {
    void **vtable;
};

void *gFakeD3DDevice2VTable[30];
HRESULT gFakeD3DBeginSceneResult;
HRESULT gFakeD3DEndSceneResult;
HRESULT gFakeD3DAddViewportResult;
HRESULT gFakeD3DSetCurrentViewportResult;
HRESULT gFakeD3DGetCapsResult;
HRESULT gFakeD3DDrawPrimitiveResult;
int gFakeD3DBeginSceneCalls;
int gFakeD3DEndSceneCalls;
int gFakeD3DAddViewportCalls;
int gFakeD3DSetCurrentViewportCalls;
int gFakeD3DGetCapsCalls;
int gFakeD3DDrawPrimitiveCalls;
IDirect3DViewport2 *gFakeD3DLastAddViewport;
IDirect3DViewport2 *gFakeD3DLastSetCurrentViewport;
D3DDEVICEDESC *gFakeD3DLastGetCapsHalDesc;
D3DDEVICEDESC *gFakeD3DLastGetCapsHelDesc;
D3DDEVICEDESC gFakeD3DLastGetCapsHalDescValue;
D3DDEVICEDESC gFakeD3DLastGetCapsHelDescValue;
D3DPRIMITIVETYPE gFakeD3DLastPrimitiveType;
D3DVERTEXTYPE gFakeD3DLastVertexType;
void *gFakeD3DLastVertices;
DWORD gFakeD3DLastVertexCount;
DWORD gFakeD3DLastDrawFlags;
D3DPRIMITIVETYPE gFakeD3DDrawPrimitiveTypes[8];
D3DVERTEXTYPE gFakeD3DDrawPrimitiveVertexTypes[8];
void *gFakeD3DDrawPrimitiveVertices[8];
DWORD gFakeD3DDrawPrimitiveVertexCounts[8];
DWORD gFakeD3DDrawPrimitiveFlags[8];
int gFakeD3DSetRenderStateCalls;
D3DRENDERSTATETYPE gFakeD3DRenderStates[16];
DWORD gFakeD3DRenderStateValues[16];
int gFakeD3DSetLightStateCalls;
D3DLIGHTSTATETYPE gFakeD3DLightStates[4];
DWORD gFakeD3DLightStateValues[4];
void *gFakeD3D2VTable[9];
HRESULT gFakeD3D2CreateDeviceResult;
HRESULT gFakeD3D2CreateViewportResult;
HRESULT gFakeD3D2CreateMaterialResult;
int gFakeD3D2ReleaseCalls;
int gFakeD3D2EnumDevicesCalls;
int gFakeD3D2CreateDeviceCalls;
int gFakeD3D2CreateViewportCalls;
int gFakeD3D2CreateMaterialCalls;
LPD3DENUMDEVICESCALLBACK gFakeD3D2LastEnumDevicesCallback;
void *gFakeD3D2LastEnumDevicesContext;
int gFakeD3D2EnumDevicesInitialAcceptedCount;
int gFakeD3D2EnumDevicesAcceptedCount;
const GUID *gFakeD3D2LastCreateDeviceGuid;
IDirectDrawSurface *gFakeD3D2LastCreateDeviceSurface;
IDirect3DDevice2 **gFakeD3D2LastCreateDeviceOut;
IDirect3DViewport2 **gFakeD3D2LastCreateViewportOut;
IUnknown *gFakeD3D2LastCreateViewportOuter;
IDirect3DMaterial2 **gFakeD3D2LastCreateMaterialOut;
IUnknown *gFakeD3D2LastCreateMaterialOuter;
IDirect3DDevice2 *gFakeD3D2CreatedDevice;
IDirect3DViewport2 *gFakeD3D2CreatedViewport;
IDirect3DMaterial2 *gFakeD3D2CreatedMaterial;
void *gFakeD3DViewport2VTable[18];
HRESULT gFakeD3DViewport2SetViewport2Result;
HRESULT gFakeD3DViewport2SetBackgroundResult;
int gFakeD3DViewport2SetViewport2Calls;
int gFakeD3DViewport2SetBackgroundCalls;
D3DVIEWPORT2 *gFakeD3DViewport2LastViewport;
D3DVIEWPORT2 gFakeD3DViewport2LastViewportValue;
D3DMATERIALHANDLE gFakeD3DViewport2LastBackground;
void *gFakeD3DMaterial2VTable[6];
HRESULT gFakeD3DMaterial2SetMaterialResult;
HRESULT gFakeD3DMaterial2GetHandleResult;
int gFakeD3DMaterial2SetMaterialCalls;
int gFakeD3DMaterial2GetHandleCalls;
D3DMATERIAL *gFakeD3DMaterial2LastMaterial;
D3DMATERIAL gFakeD3DMaterial2LastMaterialValue;
IDirect3DDevice2 *gFakeD3DMaterial2LastGetHandleDevice;
D3DMATERIALHANDLE *gFakeD3DMaterial2LastGetHandleOut;
D3DMATERIALHANDLE gFakeD3DMaterial2HandleValue;
void *gFakeD3DTexture2VTable[6];
HRESULT gFakeD3DTexture2LoadResult;
HRESULT gFakeD3DTexture2GetHandleResult;
int gFakeD3DTexture2LoadCalls;
int gFakeD3DTexture2GetHandleCalls;
int gFakeD3DTexture2ReleaseCalls;
IDirect3DTexture2 *gFakeD3DTexture2LastLoadSelf;
IDirect3DTexture2 *gFakeD3DTexture2LastLoadSource;
IDirect3DTexture2 *gFakeD3DTexture2LastGetHandleSelf;
IDirect3DDevice2 *gFakeD3DTexture2LastGetHandleDevice;
D3DTEXTUREHANDLE *gFakeD3DTexture2LastGetHandleOut;
D3DTEXTUREHANDLE gFakeD3DTexture2HandleValue;
IDirect3DTexture2 *gFakeD3DTexture2ReleaseObjects[8];
void *gFakeDirectDrawVTable[3];
HRESULT gFakeDirectDrawCreateResult;
int gFakeDirectDrawCreateCalls;
GUID *gFakeDirectDrawCreateGuid;
IDirectDraw **gFakeDirectDrawCreateOut;
IUnknown *gFakeDirectDrawCreateOuter;
IDirectDraw *gFakeDirectDrawCreateValue;
HRESULT gFakeDirectDrawQueryInterfaceResult;
int gFakeDirectDrawQueryInterfaceCalls;
IDirectDraw *gFakeDirectDrawQueryInterfaceSelf;
const GUID *gFakeDirectDrawQueryInterfaceIid;
void **gFakeDirectDrawQueryInterfaceOut;
void *gFakeDirectDrawQueryInterfaceValue;
int gFakeDirectDrawReleaseCalls;
IDirectDraw *gFakeDirectDrawReleaseSelf;
void *gFakeDirectDraw2VTable[24];
HRESULT gFakeDirectDraw2QueryInterfaceResult;
int gFakeDirectDraw2QueryInterfaceCalls;
const GUID *gFakeDirectDraw2LastQueryInterfaceIid;
void **gFakeDirectDraw2LastQueryInterfaceOut;
void *gFakeDirectDraw2QueryInterfaceValue;
HRESULT gFakeDirectDraw2CreateSurfaceResult;
int gFakeDirectDraw2CreateSurfaceCalls;
LPDDSURFACEDESC gFakeDirectDraw2LastCreateSurfaceDesc;
DDSURFACEDESC gFakeDirectDraw2CreateSurfaceDescs[4];
LPDIRECTDRAWSURFACE *gFakeDirectDraw2LastCreateSurfaceOut;
IUnknown *gFakeDirectDraw2LastCreateSurfaceOuter;
IDirectDrawSurface *gFakeDirectDraw2CreatedSurface;
HRESULT gFakeDirectDraw2CreatePaletteResult;
int gFakeDirectDraw2CreatePaletteCalls;
DWORD gFakeDirectDraw2LastCreatePaletteFlags;
LPPALETTEENTRY gFakeDirectDraw2LastCreatePaletteEntries;
LPDIRECTDRAWPALETTE *gFakeDirectDraw2LastCreatePaletteOut;
IUnknown *gFakeDirectDraw2LastCreatePaletteOuter;
IDirectDrawPalette *gFakeDirectDraw2CreatedPalette;
zVidImagePartial *gFakeDirectDraw2MutateImageOnFirstCreateSurface;
void *gFakeDirectDraw2MutatedPalette;
short gFakeDirectDraw2MutatedPaletteMetaPacked;
int gFakeDirectDraw2ReleaseCalls;
HRESULT gFakeDirectDraw2CreateClipperResult;
int gFakeDirectDraw2CreateClipperCalls;
DWORD gFakeDirectDraw2LastCreateClipperFlags;
LPDIRECTDRAWCLIPPER *gFakeDirectDraw2LastCreateClipperOut;
IUnknown *gFakeDirectDraw2LastCreateClipperOuter;
IDirectDrawClipper *gFakeDirectDraw2CreatedClipper;
HRESULT gFakeDirectDraw2SetCooperativeLevelResult;
int gFakeDirectDraw2SetCooperativeLevelCalls;
HWND gFakeDirectDraw2LastSetCooperativeHwnd;
DWORD gFakeDirectDraw2LastSetCooperativeFlags;
HRESULT gFakeDirectDraw2SetDisplayModeResult;
int gFakeDirectDraw2SetDisplayModeCalls;
DWORD gFakeDirectDraw2LastDisplayModeWidth;
DWORD gFakeDirectDraw2LastDisplayModeHeight;
DWORD gFakeDirectDraw2LastDisplayModeBpp;
DWORD gFakeDirectDraw2LastDisplayModeRefreshRate;
DWORD gFakeDirectDraw2LastDisplayModeFlags;
HRESULT gFakeDirectDraw2GetCapsResult;
int gFakeDirectDraw2GetCapsCalls;
LPDDCAPS gFakeDirectDraw2LastGetCapsHal;
LPDDCAPS gFakeDirectDraw2LastGetCapsHel;
DDCAPS gFakeDirectDraw2GetCapsHalInput;
DDCAPS gFakeDirectDraw2GetCapsHelInput;
DDCAPS gFakeDirectDraw2GetCapsHalValue;
DDCAPS gFakeDirectDraw2GetCapsHelValue;
HRESULT gFakeDirectDraw2GetAvailableVidMemResult;
int gFakeDirectDraw2GetAvailableVidMemCalls;
LPDDSCAPS gFakeDirectDraw2LastAvailableVidMemCaps;
DDSCAPS gFakeDirectDraw2LastAvailableVidMemCapsValue;
LPDWORD gFakeDirectDraw2LastAvailableVidMemTotal;
LPDWORD gFakeDirectDraw2LastAvailableVidMemFree;
DWORD gFakeDirectDraw2AvailableVidMemTotal;
DWORD gFakeDirectDraw2AvailableVidMemFree;
void *gFakeDirectDrawClipperVTable[9];
HRESULT gFakeDirectDrawClipperSetHWndResult;
int gFakeDirectDrawClipperSetHWndCalls;
DWORD gFakeDirectDrawClipperLastSetHWndFlags;
HWND gFakeDirectDrawClipperLastSetHWnd;
void *gFakeDirectDrawPaletteVTable[7];
HRESULT gFakeDirectDrawPaletteSetEntriesResult;
int gFakeDirectDrawPaletteSetEntriesCalls;
IDirectDrawPalette *gFakeDirectDrawPaletteLastSetEntriesSelf;
DWORD gFakeDirectDrawPaletteLastSetEntriesFlags;
DWORD gFakeDirectDrawPaletteLastSetEntriesFirst;
DWORD gFakeDirectDrawPaletteLastSetEntriesCount;
LPPALETTEENTRY gFakeDirectDrawPaletteLastSetEntriesEntries;
void *gFakeDirectDrawSurfaceVTable[32];
HRESULT gFakeDirectDrawSurfaceQueryInterfaceResult;
int gFakeDirectDrawSurfaceQueryInterfaceCalls;
const GUID *gFakeDirectDrawSurfaceLastQueryInterfaceIid;
void **gFakeDirectDrawSurfaceLastQueryInterfaceOut;
void *gFakeDirectDrawSurfaceQueryInterfaceValue;
void *gFakeDirectDrawSurfaceQueryInterfaceValues[4];
int gFakeDirectDrawSurfaceQueryInterfaceValueCount;
int gFakeDirectDrawSurfaceReleaseCalls;
ULONG gFakeDirectDrawSurfaceReleaseResult;
HRESULT gFakeDirectDrawSurfaceSetPaletteResult;
int gFakeDirectDrawSurfaceSetPaletteCalls;
IDirectDrawSurface *gFakeDirectDrawSurfaceSetPaletteSurfaces[4];
IDirectDrawPalette *gFakeDirectDrawSurfaceSetPalettePalettes[4];
void *gFakeDirectDrawSurface3VTable[40];
HRESULT gFakeDirectDrawSurface3QueryInterfaceResult;
int gFakeDirectDrawSurface3QueryInterfaceCalls;
const GUID *gFakeDirectDrawSurface3LastQueryInterfaceIid;
void **gFakeDirectDrawSurface3LastQueryInterfaceOut;
void *gFakeDirectDrawSurface3QueryInterfaceValue;
HRESULT gFakeDirectDrawSurface3AddAttachedSurfaceResult;
int gFakeDirectDrawSurface3AddAttachedSurfaceCalls;
IDirectDrawSurface3 *gFakeDirectDrawSurface3LastAddAttachedSelf;
IDirectDrawSurface3 *gFakeDirectDrawSurface3LastAttachedSurfaceArg;
HRESULT gFakeDirectDrawSurface3BltResults[4];
int gFakeDirectDrawSurface3BltResultCount;
int gFakeDirectDrawSurface3BltCalls;
RECT gFakeDirectDrawSurface3LastBltDstRect;
RECT gFakeDirectDrawSurface3LastBltSrcRect;
LPRECT gFakeDirectDrawSurface3LastBltDstRectArg;
LPRECT gFakeDirectDrawSurface3LastBltSrcRectArg;
IDirectDrawSurface3 *gFakeDirectDrawSurface3LastBltSource;
DWORD gFakeDirectDrawSurface3LastBltFlags;
LPDDBLTFX gFakeDirectDrawSurface3LastBltFx;
DDBLTFX gFakeDirectDrawSurface3LastBltFxValue;
IDirectDrawSurface3 *gFakeDirectDrawSurface3BltSurfaces[8];
LPRECT gFakeDirectDrawSurface3BltDstRectArgs[8];
LPRECT gFakeDirectDrawSurface3BltSrcRectArgs[8];
DWORD gFakeDirectDrawSurface3BltFlags[8];
DDBLTFX gFakeDirectDrawSurface3BltFxValues[8];
HRESULT gFakeDirectDrawSurface3LockResults[4];
int gFakeDirectDrawSurface3LockResultCount;
int gFakeDirectDrawSurface3LockCalls;
IDirectDrawSurface3 *gFakeDirectDrawSurface3LockSurfaces[8];
LPRECT gFakeDirectDrawSurface3LastLockRect;
LPDDSURFACEDESC gFakeDirectDrawSurface3LastLockDesc;
DWORD gFakeDirectDrawSurface3LastLockFlags;
HANDLE gFakeDirectDrawSurface3LastLockEvent;
DWORD gFakeDirectDrawSurface3LockDescSize;
void *gFakeDirectDrawSurface3LockPixels;
LONG gFakeDirectDrawSurface3LockPitch;
HRESULT gFakeDirectDrawSurface3UnlockResults[4];
int gFakeDirectDrawSurface3UnlockResultCount;
int gFakeDirectDrawSurface3UnlockCalls;
IDirectDrawSurface3 *gFakeDirectDrawSurface3UnlockSurfaces[8];
LPVOID gFakeDirectDrawSurface3LastUnlockArg;
HRESULT gFakeDirectDrawSurface3FlipResults[4];
int gFakeDirectDrawSurface3FlipResultCount;
int gFakeDirectDrawSurface3FlipCalls;
IDirectDrawSurface3 *gFakeDirectDrawSurface3FlipSurfaces[8];
IDirectDrawSurface3 *gFakeDirectDrawSurface3LastFlipTarget;
DWORD gFakeDirectDrawSurface3LastFlipFlags;
HRESULT gFakeDirectDrawSurface3RestoreResult;
int gFakeDirectDrawSurface3RestoreCalls;
IDirectDrawSurface3 *gFakeDirectDrawSurface3RestoreSurfaces[8];
int gFakeDirectDrawSurface3ReleaseCalls;
IDirectDrawSurface3 *gFakeDirectDrawSurface3ReleaseSurfaces[8];
HRESULT gFakeDirectDrawSurface3GetAttachedSurfaceResult;
int gFakeDirectDrawSurface3GetAttachedSurfaceCalls;
LPDDSCAPS gFakeDirectDrawSurface3LastAttachedCaps;
DDSCAPS gFakeDirectDrawSurface3LastAttachedCapsValue;
IDirectDrawSurface3 **gFakeDirectDrawSurface3LastAttachedSurfaceOut;
IDirectDrawSurface3 *gFakeDirectDrawSurface3AttachedSurface;
HRESULT gFakeDirectDrawSurface3SetClipperResult;
int gFakeDirectDrawSurface3SetClipperCalls;
IDirectDrawClipper *gFakeDirectDrawSurface3LastSetClipper;
HRESULT gFakeDirectDrawSurface3GetDCResult;
int gFakeDirectDrawSurface3GetDCCalls;
IDirectDrawSurface3 *gFakeDirectDrawSurface3LastGetDCSurface;
HDC *gFakeDirectDrawSurface3LastGetDCOut;
HDC gFakeDirectDrawSurface3GetDCValue;
HRESULT gFakeDirectDrawSurface3ReleaseDCResult;
int gFakeDirectDrawSurface3ReleaseDCCalls;
IDirectDrawSurface3 *gFakeDirectDrawSurface3LastReleaseDCSurface;
HDC gFakeDirectDrawSurface3LastReleaseDCHdc;
HRESULT gFakeDirectDrawSurface3GetPixelFormatResult;
int gFakeDirectDrawSurface3GetPixelFormatCalls;
DDPIXELFORMAT gFakeDirectDrawSurface3PixelFormat;
LPDDPIXELFORMAT gFakeDirectDrawSurface3LastPixelFormat;
DWORD gFakeDirectDrawSurface3LastPixelFormatInputSize;
HRESULT gFakeDirectDrawSurface3PageLockResults[4];
int gFakeDirectDrawSurface3PageLockResultCount;
int gFakeDirectDrawSurface3PageLockCalls;
IDirectDrawSurface3 *gFakeDirectDrawSurface3LastPageLockSurface;
DWORD gFakeDirectDrawSurface3LastPageLockFlags;
HRESULT gFakeDirectDrawSurface3PageUnlockResults[4];
int gFakeDirectDrawSurface3PageUnlockResultCount;
int gFakeDirectDrawSurface3PageUnlockCalls;
IDirectDrawSurface3 *gFakeDirectDrawSurface3LastPageUnlockSurface;
DWORD gFakeDirectDrawSurface3LastPageUnlockFlags;
void *gFakeComVTable[3];
int gFakeComReleaseCalls;
void *gFakeComReleaseObjects[8];

struct FakeSurfaceLockVerifier : zVideo_SurfaceLockVerifier {
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void **);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();
    virtual HRESULT STDMETHODCALLTYPE Unknown0c();
    virtual HRESULT STDMETHODCALLTYPE VerifySurfaceState(zVideo_SurfaceLockVerifyArgs *args);
};

FakeSurfaceLockVerifier gFakeSurfaceLockVerifier;
int gFakeSurfaceLockVerifierVerifyCalls;
zVideo_SurfaceLockVerifyArgs gFakeSurfaceLockVerifierLastArgs;
HRESULT gFakeSurfaceLockVerifierVerifyResult;
int gFakeSurfaceLockVerifierReleaseCalls;
int gFakeCreateFullscreenHalfResCalls;
int gFakeCreateFullscreenSoftwareCalls;
int gFakeCreateFullscreenHardwareCalls;
int gFakeTeardownVideoSubsystemCalls;
int gFakeCreateDirectDraw2ForSelectedDeviceCalls;
int gFakeEnumerateDirect3DDevicesForRecordCalls;
zVidHwApiDeviceRecordPartial *gFakeEnumerateDirect3DDevicesForRecordEntry;
int gFakeEnumerateDirect3DDevicesForRecordResult;
int gFakeSetVideoModeStepCount;
int gFakeSetVideoModeSteps[8];
int gFakeSetVideoModeRestoreCalls;
int gFakeSetVideoModeSetDisplayModeResult;
int gFakeSetVideoModeRestoreResults[2];
int gFakeSetVideoModeReleaseResult;
int gFakeSetVideoModeCreateSurfacesResult;
int gFakeSetVideoModeCreateDeviceResult;
int gFakeSetVideoModeVerifyLocksResult;
int gFakeUploadImageToSurfaceCalls;
IDirectDrawSurface *gFakeUploadImageToSurfaceSurface;
zVidImagePartial *gFakeUploadImageToSurfaceImage;
int gFakeUploadImageToSurfaceUseAlpha;
int gFakeUploadImageToSurfaceResult;

struct CodeFunctionPatch {
    unsigned char *address;
    unsigned char original[5];
    int active;
};

struct ImportFunctionPatch {
    ULONG_PTR *slot;
    ULONG_PTR original;
};

bool PatchImportByName(
    const char *dllName,
    const char *functionName,
    void *replacement,
    ImportFunctionPatch &patch
) {
    unsigned char *const imageBase = (unsigned char *)GetModuleHandleA(0);
    IMAGE_DOS_HEADER *const dos = (IMAGE_DOS_HEADER *)imageBase;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }

    IMAGE_NT_HEADERS *const nt = (IMAGE_NT_HEADERS *)(imageBase + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }

    const IMAGE_DATA_DIRECTORY &imports =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (imports.VirtualAddress == 0) {
        return false;
    }

    IMAGE_IMPORT_DESCRIPTOR *descriptor =
        (IMAGE_IMPORT_DESCRIPTOR *)(imageBase + imports.VirtualAddress);
    for (; descriptor->Name != 0; ++descriptor) {
        const char *const importedDll = (const char *)(imageBase + descriptor->Name);
        if (_stricmp(importedDll, dllName) != 0) {
            continue;
        }

        IMAGE_THUNK_DATA *nameThunk = (IMAGE_THUNK_DATA *)(
            imageBase + (descriptor->OriginalFirstThunk != 0
                             ? descriptor->OriginalFirstThunk
                             : descriptor->FirstThunk)
        );
        IMAGE_THUNK_DATA *addressThunk =
            (IMAGE_THUNK_DATA *)(imageBase + descriptor->FirstThunk);
        for (; nameThunk->u1.AddressOfData != 0; ++nameThunk, ++addressThunk) {
            if (IMAGE_SNAP_BY_ORDINAL(nameThunk->u1.Ordinal)) {
                continue;
            }

            IMAGE_IMPORT_BY_NAME *importName =
                (IMAGE_IMPORT_BY_NAME *)(imageBase + nameThunk->u1.AddressOfData);
            if (std::strcmp((const char *)importName->Name, functionName) != 0) {
                continue;
            }

            DWORD oldProtect = 0;
            patch.slot = &addressThunk->u1.Function;
            patch.original = addressThunk->u1.Function;
            if (VirtualProtect(
                    patch.slot,
                    sizeof(*patch.slot),
                    PAGE_EXECUTE_READWRITE,
                    &oldProtect
                ) == 0) {
                return false;
            }

            *patch.slot = (ULONG_PTR)replacement;
            DWORD ignored = 0;
            VirtualProtect(patch.slot, sizeof(*patch.slot), oldProtect, &ignored);
            FlushInstructionCache(GetCurrentProcess(), patch.slot, sizeof(*patch.slot));
            return true;
        }
    }

    return false;
}

void RestoreImportPatch(
    ImportFunctionPatch &patch
) {
    if (patch.slot == 0) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.slot,
            sizeof(*patch.slot),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) != 0) {
        *patch.slot = patch.original;
        DWORD ignored = 0;
        VirtualProtect(patch.slot, sizeof(*patch.slot), oldProtect, &ignored);
        FlushInstructionCache(GetCurrentProcess(), patch.slot, sizeof(*patch.slot));
    }
    patch.slot = 0;
}

bool PatchFunctionJump(
    void *target,
    void *replacement,
    CodeFunctionPatch &patch
) {
    patch.address = reinterpret_cast<unsigned char *>(target);
    patch.active = 0;
    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) == 0) {
        return false;
    }

    std::memcpy(patch.original, patch.address, sizeof(patch.original));
    const std::intptr_t relative =
        reinterpret_cast<unsigned char *>(replacement) - patch.address - 5;
    patch.address[0] = 0xe9;
    std::memcpy(patch.address + 1, &relative, 4);
    FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
    DWORD ignored = 0;
    VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
    patch.active = 1;
    return true;
}

void RestoreFunctionPatch(CodeFunctionPatch &patch) {
    if (patch.active == 0) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) != 0) {
        std::memcpy(patch.address, patch.original, sizeof(patch.original));
        FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
        DWORD ignored = 0;
        VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
    }
    patch.active = 0;
}

int FakeCreateFullscreenHalfResSurfaces() {
    ++gFakeCreateFullscreenHalfResCalls;
    return 11;
}

int FakeCreateFullscreenSoftwareSurfaces() {
    ++gFakeCreateFullscreenSoftwareCalls;
    return 22;
}

int FakeCreateFullscreenHardwareSurfaces() {
    ++gFakeCreateFullscreenHardwareCalls;
    return 33;
}

void FakeTeardownVideoSubsystem() {
    ++gFakeTeardownVideoSubsystemCalls;
}

int FakeCreateDirectDraw2ForSelectedDevice() {
    ++gFakeCreateDirectDraw2ForSelectedDeviceCalls;
    return 0;
}

int __fastcall FakeEnumerateDirect3DDevicesForRecord(
    zVidHwApiDeviceRecordPartial *entry
) {
    ++gFakeEnumerateDirect3DDevicesForRecordCalls;
    gFakeEnumerateDirect3DDevicesForRecordEntry = entry;
    return gFakeEnumerateDirect3DDevicesForRecordResult;
}

void RecordSetVideoModeStep(int step) {
    if (gFakeSetVideoModeStepCount < 8) {
        gFakeSetVideoModeSteps[gFakeSetVideoModeStepCount] = step;
    }
    ++gFakeSetVideoModeStepCount;
}

int FakeSetVideoMode_SetDisplayMode() {
    RecordSetVideoModeStep(1);
    return gFakeSetVideoModeSetDisplayModeResult;
}

int FakeSetVideoMode_RestoreDisplaySurfaces() {
    RecordSetVideoModeStep(2);
    int index = gFakeSetVideoModeRestoreCalls;
    if (index > 1) {
        index = 1;
    }
    ++gFakeSetVideoModeRestoreCalls;
    return gFakeSetVideoModeRestoreResults[index];
}

int FakeSetVideoMode_ReleaseAllInterfacesAndSurfaces() {
    RecordSetVideoModeStep(3);
    return gFakeSetVideoModeReleaseResult;
}

int FakeSetVideoMode_CreateFullscreenSurfacesForRenderer() {
    RecordSetVideoModeStep(4);
    return gFakeSetVideoModeCreateSurfacesResult;
}

int FakeSetVideoMode_CreateDeviceState() {
    RecordSetVideoModeStep(5);
    return gFakeSetVideoModeCreateDeviceResult;
}

int FakeSetVideoMode_VerifyFullscreenSurfaceLocks() {
    RecordSetVideoModeStep(6);
    return gFakeSetVideoModeVerifyLocksResult;
}

int __fastcall FakeUploadImageToSurface(
    IDirectDrawSurface *uploadSurface,
    zVidImagePartial *image,
    int useAlpha
) {
    ++gFakeUploadImageToSurfaceCalls;
    gFakeUploadImageToSurfaceSurface = uploadSurface;
    gFakeUploadImageToSurfaceImage = image;
    gFakeUploadImageToSurfaceUseAlpha = useAlpha;
    return gFakeUploadImageToSurfaceResult;
}

void ResetSetVideoModeCapture() {
    gFakeSetVideoModeStepCount = 0;
    std::memset(
        gFakeSetVideoModeSteps,
        0,
        sizeof(gFakeSetVideoModeSteps)
    );
    gFakeSetVideoModeRestoreCalls = 0;
    gFakeSetVideoModeSetDisplayModeResult = 1;
    gFakeSetVideoModeRestoreResults[0] = 0;
    gFakeSetVideoModeRestoreResults[1] = 0;
    gFakeSetVideoModeReleaseResult = 0;
    gFakeSetVideoModeCreateSurfacesResult = 0;
    gFakeSetVideoModeCreateDeviceResult = 0;
    gFakeSetVideoModeVerifyLocksResult = 0;
}

ULONG __stdcall FakeDirectDrawSurface3_Release(IDirectDrawSurface3 *surface) {
    if (gFakeDirectDrawSurface3ReleaseCalls < 8) {
        gFakeDirectDrawSurface3ReleaseSurfaces[gFakeDirectDrawSurface3ReleaseCalls] =
            surface;
    }
    ++gFakeDirectDrawSurface3ReleaseCalls;
    return 1;
}

ULONG __stdcall FakeCom_Release(void *self) {
    if (gFakeComReleaseCalls < 8) {
        gFakeComReleaseObjects[gFakeComReleaseCalls] = self;
    }
    ++gFakeComReleaseCalls;
    return 1;
}

HRESULT STDMETHODCALLTYPE FakeSurfaceLockVerifier::QueryInterface(
    REFIID,
    void **
) {
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE FakeSurfaceLockVerifier::AddRef() {
    return 1;
}

ULONG STDMETHODCALLTYPE FakeSurfaceLockVerifier::Release() {
    ++gFakeSurfaceLockVerifierReleaseCalls;
    return 1;
}

HRESULT STDMETHODCALLTYPE FakeSurfaceLockVerifier::Unknown0c() {
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE FakeSurfaceLockVerifier::VerifySurfaceState(
    zVideo_SurfaceLockVerifyArgs *args
) {
    ++gFakeSurfaceLockVerifierVerifyCalls;
    gFakeSurfaceLockVerifierLastArgs = *args;
    return gFakeSurfaceLockVerifierVerifyResult;
}

HRESULT __stdcall FakeDirectDraw2_CreateSurface(
    IDirectDraw2 *,
    LPDDSURFACEDESC desc,
    LPDIRECTDRAWSURFACE *outSurface,
    IUnknown *outer
) {
    ++gFakeDirectDraw2CreateSurfaceCalls;
    gFakeDirectDraw2LastCreateSurfaceDesc = desc;
    if (gFakeDirectDraw2CreateSurfaceCalls <= 4) {
        gFakeDirectDraw2CreateSurfaceDescs[gFakeDirectDraw2CreateSurfaceCalls - 1] =
            *desc;
    }
    gFakeDirectDraw2LastCreateSurfaceOut = outSurface;
    gFakeDirectDraw2LastCreateSurfaceOuter = outer;
    if (gFakeDirectDraw2CreateSurfaceResult == DD_OK) {
        *outSurface = gFakeDirectDraw2CreatedSurface;
    }
    if (gFakeDirectDraw2CreateSurfaceCalls == 1 &&
        gFakeDirectDraw2MutateImageOnFirstCreateSurface != nullptr) {
        gFakeDirectDraw2MutateImageOnFirstCreateSurface->palette =
            gFakeDirectDraw2MutatedPalette;
        gFakeDirectDraw2MutateImageOnFirstCreateSurface->paletteMetaPacked =
            gFakeDirectDraw2MutatedPaletteMetaPacked;
    }
    return gFakeDirectDraw2CreateSurfaceResult;
}

HRESULT __stdcall FakeDirectDraw2_CreatePalette(
    IDirectDraw2 *,
    DWORD flags,
    LPPALETTEENTRY entries,
    LPDIRECTDRAWPALETTE *outPalette,
    IUnknown *outer
) {
    ++gFakeDirectDraw2CreatePaletteCalls;
    gFakeDirectDraw2LastCreatePaletteFlags = flags;
    gFakeDirectDraw2LastCreatePaletteEntries = entries;
    gFakeDirectDraw2LastCreatePaletteOut = outPalette;
    gFakeDirectDraw2LastCreatePaletteOuter = outer;
    if (gFakeDirectDraw2CreatePaletteResult == DD_OK) {
        *outPalette = gFakeDirectDraw2CreatedPalette;
    }
    return gFakeDirectDraw2CreatePaletteResult;
}

HRESULT WINAPI FakeDirectDrawCreate(
    GUID *guid,
    IDirectDraw **outDirectDraw,
    IUnknown *outer
) {
    ++gFakeDirectDrawCreateCalls;
    gFakeDirectDrawCreateGuid = guid;
    gFakeDirectDrawCreateOut = outDirectDraw;
    gFakeDirectDrawCreateOuter = outer;
    if (gFakeDirectDrawCreateResult == DD_OK) {
        *outDirectDraw = gFakeDirectDrawCreateValue;
    }
    return gFakeDirectDrawCreateResult;
}

HRESULT __stdcall FakeDirectDraw_QueryInterface(
    IDirectDraw *self,
    REFIID iid,
    void **outInterface
) {
    ++gFakeDirectDrawQueryInterfaceCalls;
    gFakeDirectDrawQueryInterfaceSelf = self;
    gFakeDirectDrawQueryInterfaceIid = &iid;
    gFakeDirectDrawQueryInterfaceOut = outInterface;
    if (gFakeDirectDrawQueryInterfaceResult == DD_OK) {
        *outInterface = gFakeDirectDrawQueryInterfaceValue;
    }
    return gFakeDirectDrawQueryInterfaceResult;
}

ULONG __stdcall FakeDirectDraw_Release(
    IDirectDraw *self
) {
    ++gFakeDirectDrawReleaseCalls;
    gFakeDirectDrawReleaseSelf = self;
    return 1;
}

ULONG __stdcall FakeDirectDraw2_Release(IDirectDraw2 *) {
    ++gFakeDirectDraw2ReleaseCalls;
    return 1;
}

HRESULT __stdcall FakeDirectDraw2_QueryInterface(
    IDirectDraw2 *,
    REFIID iid,
    void **outInterface
) {
    ++gFakeDirectDraw2QueryInterfaceCalls;
    gFakeDirectDraw2LastQueryInterfaceIid = &iid;
    gFakeDirectDraw2LastQueryInterfaceOut = outInterface;
    if (gFakeDirectDraw2QueryInterfaceResult == DD_OK) {
        *outInterface = gFakeDirectDraw2QueryInterfaceValue;
    }
    return gFakeDirectDraw2QueryInterfaceResult;
}

HRESULT __stdcall FakeDirectDraw2_CreateClipper(
    IDirectDraw2 *,
    DWORD flags,
    LPDIRECTDRAWCLIPPER *outClipper,
    IUnknown *outer
) {
    ++gFakeDirectDraw2CreateClipperCalls;
    gFakeDirectDraw2LastCreateClipperFlags = flags;
    gFakeDirectDraw2LastCreateClipperOut = outClipper;
    gFakeDirectDraw2LastCreateClipperOuter = outer;
    if (gFakeDirectDraw2CreateClipperResult == DD_OK) {
        *outClipper = gFakeDirectDraw2CreatedClipper;
    }
    return gFakeDirectDraw2CreateClipperResult;
}

HRESULT __stdcall FakeDirectDraw2_SetCooperativeLevel(
    IDirectDraw2 *,
    HWND hwnd,
    DWORD flags
) {
    ++gFakeDirectDraw2SetCooperativeLevelCalls;
    gFakeDirectDraw2LastSetCooperativeHwnd = hwnd;
    gFakeDirectDraw2LastSetCooperativeFlags = flags;
    return gFakeDirectDraw2SetCooperativeLevelResult;
}

HRESULT __stdcall FakeDirectDraw2_SetDisplayMode(
    IDirectDraw2 *,
    DWORD width,
    DWORD height,
    DWORD bpp,
    DWORD refreshRate,
    DWORD flags
) {
    ++gFakeDirectDraw2SetDisplayModeCalls;
    gFakeDirectDraw2LastDisplayModeWidth = width;
    gFakeDirectDraw2LastDisplayModeHeight = height;
    gFakeDirectDraw2LastDisplayModeBpp = bpp;
    gFakeDirectDraw2LastDisplayModeRefreshRate = refreshRate;
    gFakeDirectDraw2LastDisplayModeFlags = flags;
    return gFakeDirectDraw2SetDisplayModeResult;
}

HRESULT __stdcall FakeDirectDraw2_GetCaps(
    IDirectDraw2 *,
    LPDDCAPS halCaps,
    LPDDCAPS helCaps
) {
    ++gFakeDirectDraw2GetCapsCalls;
    gFakeDirectDraw2LastGetCapsHal = halCaps;
    gFakeDirectDraw2LastGetCapsHel = helCaps;
    if (halCaps != nullptr) {
        gFakeDirectDraw2GetCapsHalInput = *halCaps;
    }
    if (helCaps != nullptr) {
        gFakeDirectDraw2GetCapsHelInput = *helCaps;
    }
    if (gFakeDirectDraw2GetCapsResult == DD_OK) {
        *halCaps = gFakeDirectDraw2GetCapsHalValue;
        *helCaps = gFakeDirectDraw2GetCapsHelValue;
    }
    return gFakeDirectDraw2GetCapsResult;
}

HRESULT __stdcall FakeDirectDraw2_GetAvailableVidMem(
    IDirectDraw2 *,
    LPDDSCAPS caps,
    LPDWORD totalBytes,
    LPDWORD freeBytes
) {
    ++gFakeDirectDraw2GetAvailableVidMemCalls;
    gFakeDirectDraw2LastAvailableVidMemCaps = caps;
    gFakeDirectDraw2LastAvailableVidMemCapsValue = *caps;
    gFakeDirectDraw2LastAvailableVidMemTotal = totalBytes;
    gFakeDirectDraw2LastAvailableVidMemFree = freeBytes;
    if (gFakeDirectDraw2GetAvailableVidMemResult == DD_OK) {
        *totalBytes = gFakeDirectDraw2AvailableVidMemTotal;
        *freeBytes = gFakeDirectDraw2AvailableVidMemFree;
    }
    return gFakeDirectDraw2GetAvailableVidMemResult;
}

HRESULT __stdcall FakeDirectDrawClipper_SetHWnd(
    IDirectDrawClipper *,
    DWORD flags,
    HWND hwnd
) {
    ++gFakeDirectDrawClipperSetHWndCalls;
    gFakeDirectDrawClipperLastSetHWndFlags = flags;
    gFakeDirectDrawClipperLastSetHWnd = hwnd;
    return gFakeDirectDrawClipperSetHWndResult;
}

HRESULT __stdcall FakeDirectDrawPalette_SetEntries(
    IDirectDrawPalette *palette,
    DWORD flags,
    DWORD firstEntry,
    DWORD entryCount,
    LPPALETTEENTRY entries
) {
    ++gFakeDirectDrawPaletteSetEntriesCalls;
    gFakeDirectDrawPaletteLastSetEntriesSelf = palette;
    gFakeDirectDrawPaletteLastSetEntriesFlags = flags;
    gFakeDirectDrawPaletteLastSetEntriesFirst = firstEntry;
    gFakeDirectDrawPaletteLastSetEntriesCount = entryCount;
    gFakeDirectDrawPaletteLastSetEntriesEntries = entries;
    return gFakeDirectDrawPaletteSetEntriesResult;
}

HRESULT __stdcall FakeDirectDrawSurface_QueryInterface(
    IDirectDrawSurface *,
    REFIID iid,
    void **outInterface
) {
    ++gFakeDirectDrawSurfaceQueryInterfaceCalls;
    gFakeDirectDrawSurfaceLastQueryInterfaceIid = &iid;
    gFakeDirectDrawSurfaceLastQueryInterfaceOut = outInterface;
    if (gFakeDirectDrawSurfaceQueryInterfaceResult == DD_OK) {
        if (gFakeDirectDrawSurfaceQueryInterfaceValueCount != 0 &&
            gFakeDirectDrawSurfaceQueryInterfaceCalls <=
                gFakeDirectDrawSurfaceQueryInterfaceValueCount) {
            *outInterface =
                gFakeDirectDrawSurfaceQueryInterfaceValues[
                    gFakeDirectDrawSurfaceQueryInterfaceCalls - 1
                ];
        } else {
            *outInterface = gFakeDirectDrawSurfaceQueryInterfaceValue;
        }
    }
    return gFakeDirectDrawSurfaceQueryInterfaceResult;
}

ULONG __stdcall FakeDirectDrawSurface_Release(IDirectDrawSurface *) {
    ++gFakeDirectDrawSurfaceReleaseCalls;
    return gFakeDirectDrawSurfaceReleaseResult;
}

HRESULT __stdcall FakeDirectDrawSurface_SetPalette(
    IDirectDrawSurface *surface,
    IDirectDrawPalette *palette
) {
    if (gFakeDirectDrawSurfaceSetPaletteCalls < 4) {
        gFakeDirectDrawSurfaceSetPaletteSurfaces[gFakeDirectDrawSurfaceSetPaletteCalls] =
            surface;
        gFakeDirectDrawSurfaceSetPalettePalettes[gFakeDirectDrawSurfaceSetPaletteCalls] =
            palette;
    }
    ++gFakeDirectDrawSurfaceSetPaletteCalls;
    return gFakeDirectDrawSurfaceSetPaletteResult;
}

ULONG __stdcall FakeD3DTexture2_Release(IDirect3DTexture2 *texture) {
    if (gFakeD3DTexture2ReleaseCalls < 8) {
        gFakeD3DTexture2ReleaseObjects[gFakeD3DTexture2ReleaseCalls] = texture;
    }
    ++gFakeD3DTexture2ReleaseCalls;
    return 1;
}

HRESULT __stdcall FakeD3DTexture2_GetHandle(
    IDirect3DTexture2 *texture,
    IDirect3DDevice2 *device,
    D3DTEXTUREHANDLE *outHandle
) {
    ++gFakeD3DTexture2GetHandleCalls;
    gFakeD3DTexture2LastGetHandleSelf = texture;
    gFakeD3DTexture2LastGetHandleDevice = device;
    gFakeD3DTexture2LastGetHandleOut = outHandle;
    if (gFakeD3DTexture2GetHandleResult == DD_OK) {
        *outHandle = gFakeD3DTexture2HandleValue;
    }
    return gFakeD3DTexture2GetHandleResult;
}

HRESULT __stdcall FakeD3DTexture2_Load(
    IDirect3DTexture2 *texture,
    IDirect3DTexture2 *sourceTexture
) {
    ++gFakeD3DTexture2LoadCalls;
    gFakeD3DTexture2LastLoadSelf = texture;
    gFakeD3DTexture2LastLoadSource = sourceTexture;
    return gFakeD3DTexture2LoadResult;
}

HRESULT __stdcall FakeDirectDrawSurface3_QueryInterface(
    IDirectDrawSurface3 *,
    REFIID iid,
    void **outInterface
) {
    ++gFakeDirectDrawSurface3QueryInterfaceCalls;
    gFakeDirectDrawSurface3LastQueryInterfaceIid = &iid;
    gFakeDirectDrawSurface3LastQueryInterfaceOut = outInterface;
    if (gFakeDirectDrawSurface3QueryInterfaceResult == DD_OK) {
        *outInterface = gFakeDirectDrawSurface3QueryInterfaceValue;
    }
    return gFakeDirectDrawSurface3QueryInterfaceResult;
}

HRESULT __stdcall FakeDirectDrawSurface3_AddAttachedSurface(
    IDirectDrawSurface3 *surface,
    IDirectDrawSurface3 *attachedSurface
) {
    ++gFakeDirectDrawSurface3AddAttachedSurfaceCalls;
    gFakeDirectDrawSurface3LastAddAttachedSelf = surface;
    gFakeDirectDrawSurface3LastAttachedSurfaceArg = attachedSurface;
    return gFakeDirectDrawSurface3AddAttachedSurfaceResult;
}

HRESULT __stdcall FakeD3DDevice2_GetCaps(
    IDirect3DDevice2 *,
    D3DDEVICEDESC *halDesc,
    D3DDEVICEDESC *helDesc
) {
    ++gFakeD3DGetCapsCalls;
    gFakeD3DLastGetCapsHalDesc = halDesc;
    gFakeD3DLastGetCapsHelDesc = helDesc;
    if (halDesc != nullptr) {
        gFakeD3DLastGetCapsHalDescValue = *halDesc;
    }
    if (helDesc != nullptr) {
        gFakeD3DLastGetCapsHelDescValue = *helDesc;
    }
    return gFakeD3DGetCapsResult;
}

HRESULT __stdcall FakeD3DDevice2_AddViewport(
    IDirect3DDevice2 *,
    IDirect3DViewport2 *viewport
) {
    ++gFakeD3DAddViewportCalls;
    gFakeD3DLastAddViewport = viewport;
    return gFakeD3DAddViewportResult;
}

HRESULT __stdcall FakeD3DDevice2_BeginScene(IDirect3DDevice2 *) {
    ++gFakeD3DBeginSceneCalls;
    return gFakeD3DBeginSceneResult;
}

HRESULT __stdcall FakeD3DDevice2_EndScene(IDirect3DDevice2 *) {
    ++gFakeD3DEndSceneCalls;
    return gFakeD3DEndSceneResult;
}

HRESULT __stdcall FakeD3DDevice2_SetCurrentViewport(
    IDirect3DDevice2 *,
    IDirect3DViewport2 *viewport
) {
    ++gFakeD3DSetCurrentViewportCalls;
    gFakeD3DLastSetCurrentViewport = viewport;
    return gFakeD3DSetCurrentViewportResult;
}

HRESULT __stdcall FakeD3DDevice2_SetRenderState(
    IDirect3DDevice2 *,
    D3DRENDERSTATETYPE renderState,
    DWORD value
) {
    if (gFakeD3DSetRenderStateCalls < 16) {
        gFakeD3DRenderStates[gFakeD3DSetRenderStateCalls] = renderState;
        gFakeD3DRenderStateValues[gFakeD3DSetRenderStateCalls] = value;
    }
    ++gFakeD3DSetRenderStateCalls;
    return DD_OK;
}

HRESULT __stdcall FakeD3DDevice2_SetLightState(
    IDirect3DDevice2 *,
    D3DLIGHTSTATETYPE lightState,
    DWORD value
) {
    if (gFakeD3DSetLightStateCalls < 4) {
        gFakeD3DLightStates[gFakeD3DSetLightStateCalls] = lightState;
        gFakeD3DLightStateValues[gFakeD3DSetLightStateCalls] = value;
    }
    ++gFakeD3DSetLightStateCalls;
    return DD_OK;
}

HRESULT __stdcall FakeD3DDevice2_DrawPrimitive(
    IDirect3DDevice2 *,
    D3DPRIMITIVETYPE primitiveType,
    D3DVERTEXTYPE vertexType,
    void *vertices,
    DWORD vertexCount,
    DWORD flags
) {
    if (gFakeD3DDrawPrimitiveCalls < 8) {
        gFakeD3DDrawPrimitiveTypes[gFakeD3DDrawPrimitiveCalls] = primitiveType;
        gFakeD3DDrawPrimitiveVertexTypes[gFakeD3DDrawPrimitiveCalls] = vertexType;
        gFakeD3DDrawPrimitiveVertices[gFakeD3DDrawPrimitiveCalls] = vertices;
        gFakeD3DDrawPrimitiveVertexCounts[gFakeD3DDrawPrimitiveCalls] = vertexCount;
        gFakeD3DDrawPrimitiveFlags[gFakeD3DDrawPrimitiveCalls] = flags;
    }
    ++gFakeD3DDrawPrimitiveCalls;
    gFakeD3DLastPrimitiveType = primitiveType;
    gFakeD3DLastVertexType = vertexType;
    gFakeD3DLastVertices = vertices;
    gFakeD3DLastVertexCount = vertexCount;
    gFakeD3DLastDrawFlags = flags;
    return gFakeD3DDrawPrimitiveResult;
}

ULONG __stdcall FakeD3D2_Release(IDirect3D2 *) {
    ++gFakeD3D2ReleaseCalls;
    return 1;
}

HRESULT __stdcall FakeD3D2_EnumDevices(
    IDirect3D2 *,
    LPD3DENUMDEVICESCALLBACK callback,
    void *context
) {
    ++gFakeD3D2EnumDevicesCalls;
    gFakeD3D2LastEnumDevicesCallback = callback;
    gFakeD3D2LastEnumDevicesContext = context;
    zVidHwApiDeviceRecordPartial *entry =
        reinterpret_cast<zVidHwApiDeviceRecordPartial *>(context);
    gFakeD3D2EnumDevicesInitialAcceptedCount = entry->m_acceptedD3DDeviceCount;
    entry->m_acceptedD3DDeviceCount = gFakeD3D2EnumDevicesAcceptedCount;
    return DD_OK;
}

HRESULT __stdcall FakeD3D2_CreateMaterial(
    IDirect3D2 *,
    IDirect3DMaterial2 **outMaterial,
    IUnknown *outer
) {
    ++gFakeD3D2CreateMaterialCalls;
    gFakeD3D2LastCreateMaterialOut = outMaterial;
    gFakeD3D2LastCreateMaterialOuter = outer;
    if (gFakeD3D2CreateMaterialResult == DD_OK) {
        *outMaterial = gFakeD3D2CreatedMaterial;
    }
    return gFakeD3D2CreateMaterialResult;
}

HRESULT __stdcall FakeD3D2_CreateViewport(
    IDirect3D2 *,
    IDirect3DViewport2 **outViewport,
    IUnknown *outer
) {
    ++gFakeD3D2CreateViewportCalls;
    gFakeD3D2LastCreateViewportOut = outViewport;
    gFakeD3D2LastCreateViewportOuter = outer;
    if (gFakeD3D2CreateViewportResult == DD_OK) {
        *outViewport = gFakeD3D2CreatedViewport;
    }
    return gFakeD3D2CreateViewportResult;
}

HRESULT __stdcall FakeD3D2_CreateDevice(
    IDirect3D2 *,
    REFCLSID deviceGuid,
    IDirectDrawSurface *renderTarget,
    IDirect3DDevice2 **outDevice
) {
    ++gFakeD3D2CreateDeviceCalls;
    gFakeD3D2LastCreateDeviceGuid = &deviceGuid;
    gFakeD3D2LastCreateDeviceSurface = renderTarget;
    gFakeD3D2LastCreateDeviceOut = outDevice;
    if (gFakeD3D2CreateDeviceResult == DD_OK) {
        *outDevice = gFakeD3D2CreatedDevice;
    }
    return gFakeD3D2CreateDeviceResult;
}

HRESULT __stdcall FakeD3DViewport2_SetBackground(
    IDirect3DViewport2 *,
    D3DMATERIALHANDLE handle
) {
    ++gFakeD3DViewport2SetBackgroundCalls;
    gFakeD3DViewport2LastBackground = handle;
    return gFakeD3DViewport2SetBackgroundResult;
}

HRESULT __stdcall FakeD3DViewport2_SetViewport2(
    IDirect3DViewport2 *,
    D3DVIEWPORT2 *viewport
) {
    ++gFakeD3DViewport2SetViewport2Calls;
    gFakeD3DViewport2LastViewport = viewport;
    if (viewport != nullptr) {
        gFakeD3DViewport2LastViewportValue = *viewport;
    }
    return gFakeD3DViewport2SetViewport2Result;
}

HRESULT __stdcall FakeD3DMaterial2_SetMaterial(
    IDirect3DMaterial2 *,
    D3DMATERIAL *material
) {
    ++gFakeD3DMaterial2SetMaterialCalls;
    gFakeD3DMaterial2LastMaterial = material;
    if (material != nullptr) {
        gFakeD3DMaterial2LastMaterialValue = *material;
    }
    return gFakeD3DMaterial2SetMaterialResult;
}

HRESULT __stdcall FakeD3DMaterial2_GetHandle(
    IDirect3DMaterial2 *,
    IDirect3DDevice2 *device,
    D3DMATERIALHANDLE *outHandle
) {
    ++gFakeD3DMaterial2GetHandleCalls;
    gFakeD3DMaterial2LastGetHandleDevice = device;
    gFakeD3DMaterial2LastGetHandleOut = outHandle;
    if (gFakeD3DMaterial2GetHandleResult == DD_OK) {
        *outHandle = gFakeD3DMaterial2HandleValue;
    }
    return gFakeD3DMaterial2GetHandleResult;
}

HRESULT __stdcall FakeDirectDrawSurface3_Blt(
    IDirectDrawSurface3 *surface,
    LPRECT dstRect,
    IDirectDrawSurface3 *sourceSurface,
    LPRECT srcRect,
    DWORD flags,
    LPDDBLTFX bltFx
) {
    int callIndex = gFakeDirectDrawSurface3BltCalls;
    int resultIndex = callIndex;
    if (resultIndex >= gFakeDirectDrawSurface3BltResultCount) {
        resultIndex = gFakeDirectDrawSurface3BltResultCount - 1;
    }
    ++gFakeDirectDrawSurface3BltCalls;
    gFakeDirectDrawSurface3LastBltDstRectArg = dstRect;
    gFakeDirectDrawSurface3LastBltSrcRectArg = srcRect;
    gFakeDirectDrawSurface3LastBltDstRect = dstRect != nullptr ? *dstRect : RECT{};
    gFakeDirectDrawSurface3LastBltSrcRect = srcRect != nullptr ? *srcRect : RECT{};
    gFakeDirectDrawSurface3LastBltSource = sourceSurface;
    gFakeDirectDrawSurface3LastBltFlags = flags;
    gFakeDirectDrawSurface3LastBltFx = bltFx;
    gFakeDirectDrawSurface3LastBltFxValue = bltFx != nullptr ? *bltFx : DDBLTFX{};
    if (callIndex < 8) {
        gFakeDirectDrawSurface3BltSurfaces[callIndex] = surface;
        gFakeDirectDrawSurface3BltDstRectArgs[callIndex] = dstRect;
        gFakeDirectDrawSurface3BltSrcRectArgs[callIndex] = srcRect;
        gFakeDirectDrawSurface3BltFlags[callIndex] = flags;
        gFakeDirectDrawSurface3BltFxValues[callIndex] =
            bltFx != nullptr ? *bltFx : DDBLTFX{};
    }
    return gFakeDirectDrawSurface3BltResults[resultIndex];
}

HRESULT __stdcall FakeDirectDrawSurface3_Lock(
    IDirectDrawSurface3 *surface,
    LPRECT rect,
    LPDDSURFACEDESC surfaceDesc,
    DWORD flags,
    HANDLE eventHandle
) {
    int index = gFakeDirectDrawSurface3LockCalls;
    if (index >= gFakeDirectDrawSurface3LockResultCount) {
        index = gFakeDirectDrawSurface3LockResultCount - 1;
    }
    if (gFakeDirectDrawSurface3LockCalls < 8) {
        gFakeDirectDrawSurface3LockSurfaces[gFakeDirectDrawSurface3LockCalls] =
            surface;
    }
    ++gFakeDirectDrawSurface3LockCalls;
    gFakeDirectDrawSurface3LastLockRect = rect;
    gFakeDirectDrawSurface3LastLockDesc = surfaceDesc;
    gFakeDirectDrawSurface3LastLockFlags = flags;
    gFakeDirectDrawSurface3LastLockEvent = eventHandle;
    gFakeDirectDrawSurface3LockDescSize = surfaceDesc->dwSize;

    if (gFakeDirectDrawSurface3LockResults[index] == DD_OK) {
        surfaceDesc->dwWidth = 640;
        surfaceDesc->dwHeight = 480;
        surfaceDesc->lPitch = gFakeDirectDrawSurface3LockPitch;
        surfaceDesc->lpSurface = gFakeDirectDrawSurface3LockPixels;
    }

    return gFakeDirectDrawSurface3LockResults[index];
}

HRESULT __stdcall FakeDirectDrawSurface3_Unlock(
    IDirectDrawSurface3 *surface,
    LPVOID surfaceData
) {
    int index = gFakeDirectDrawSurface3UnlockCalls;
    if (index >= gFakeDirectDrawSurface3UnlockResultCount) {
        index = gFakeDirectDrawSurface3UnlockResultCount - 1;
    }
    if (gFakeDirectDrawSurface3UnlockCalls < 8) {
        gFakeDirectDrawSurface3UnlockSurfaces[gFakeDirectDrawSurface3UnlockCalls] =
            surface;
    }
    ++gFakeDirectDrawSurface3UnlockCalls;
    gFakeDirectDrawSurface3LastUnlockArg = surfaceData;
    return gFakeDirectDrawSurface3UnlockResults[index];
}

HRESULT __stdcall FakeDirectDrawSurface3_Flip(
    IDirectDrawSurface3 *surface,
    IDirectDrawSurface3 *targetOverride,
    DWORD flags
) {
    int index = gFakeDirectDrawSurface3FlipCalls;
    if (index >= gFakeDirectDrawSurface3FlipResultCount) {
        index = gFakeDirectDrawSurface3FlipResultCount - 1;
    }
    if (gFakeDirectDrawSurface3FlipCalls < 8) {
        gFakeDirectDrawSurface3FlipSurfaces[gFakeDirectDrawSurface3FlipCalls] =
            surface;
    }
    ++gFakeDirectDrawSurface3FlipCalls;
    gFakeDirectDrawSurface3LastFlipTarget = targetOverride;
    gFakeDirectDrawSurface3LastFlipFlags = flags;
    return gFakeDirectDrawSurface3FlipResults[index];
}

HRESULT __stdcall FakeDirectDrawSurface3_Restore(IDirectDrawSurface3 *surface) {
    if (gFakeDirectDrawSurface3RestoreCalls < 8) {
        gFakeDirectDrawSurface3RestoreSurfaces[gFakeDirectDrawSurface3RestoreCalls] =
            surface;
    }
    ++gFakeDirectDrawSurface3RestoreCalls;
    return gFakeDirectDrawSurface3RestoreResult;
}

HRESULT __stdcall FakeDirectDrawSurface3_GetAttachedSurface(
    IDirectDrawSurface3 *,
    LPDDSCAPS caps,
    IDirectDrawSurface3 **outSurface
) {
    ++gFakeDirectDrawSurface3GetAttachedSurfaceCalls;
    gFakeDirectDrawSurface3LastAttachedCaps = caps;
    gFakeDirectDrawSurface3LastAttachedCapsValue = *caps;
    gFakeDirectDrawSurface3LastAttachedSurfaceOut = outSurface;
    if (gFakeDirectDrawSurface3GetAttachedSurfaceResult == DD_OK) {
        *outSurface = gFakeDirectDrawSurface3AttachedSurface;
    }
    return gFakeDirectDrawSurface3GetAttachedSurfaceResult;
}

HRESULT __stdcall FakeDirectDrawSurface3_SetClipper(
    IDirectDrawSurface3 *,
    IDirectDrawClipper *clipper
) {
    ++gFakeDirectDrawSurface3SetClipperCalls;
    gFakeDirectDrawSurface3LastSetClipper = clipper;
    return gFakeDirectDrawSurface3SetClipperResult;
}

HRESULT __stdcall FakeDirectDrawSurface3_GetDC(
    IDirectDrawSurface3 *surface,
    HDC *outHdc
) {
    ++gFakeDirectDrawSurface3GetDCCalls;
    gFakeDirectDrawSurface3LastGetDCSurface = surface;
    gFakeDirectDrawSurface3LastGetDCOut = outHdc;
    if (gFakeDirectDrawSurface3GetDCResult == DD_OK) {
        *outHdc = gFakeDirectDrawSurface3GetDCValue;
    }
    return gFakeDirectDrawSurface3GetDCResult;
}

HRESULT __stdcall FakeDirectDrawSurface3_ReleaseDC(
    IDirectDrawSurface3 *surface,
    HDC hdc
) {
    ++gFakeDirectDrawSurface3ReleaseDCCalls;
    gFakeDirectDrawSurface3LastReleaseDCSurface = surface;
    gFakeDirectDrawSurface3LastReleaseDCHdc = hdc;
    return gFakeDirectDrawSurface3ReleaseDCResult;
}

HRESULT __stdcall FakeDirectDrawSurface3_GetPixelFormat(
    IDirectDrawSurface3 *,
    LPDDPIXELFORMAT pixelFormat
) {
    ++gFakeDirectDrawSurface3GetPixelFormatCalls;
    gFakeDirectDrawSurface3LastPixelFormat = pixelFormat;
    gFakeDirectDrawSurface3LastPixelFormatInputSize = pixelFormat->dwSize;
    if (gFakeDirectDrawSurface3GetPixelFormatResult == DD_OK) {
        *pixelFormat = gFakeDirectDrawSurface3PixelFormat;
    }
    return gFakeDirectDrawSurface3GetPixelFormatResult;
}

HRESULT __stdcall FakeDirectDrawSurface3_PageLock(
    IDirectDrawSurface3 *surface,
    DWORD flags
) {
    int index = gFakeDirectDrawSurface3PageLockCalls;
    if (index >= gFakeDirectDrawSurface3PageLockResultCount) {
        index = gFakeDirectDrawSurface3PageLockResultCount - 1;
    }
    ++gFakeDirectDrawSurface3PageLockCalls;
    gFakeDirectDrawSurface3LastPageLockSurface = surface;
    gFakeDirectDrawSurface3LastPageLockFlags = flags;
    return gFakeDirectDrawSurface3PageLockResults[index];
}

HRESULT __stdcall FakeDirectDrawSurface3_PageUnlock(
    IDirectDrawSurface3 *surface,
    DWORD flags
) {
    int index = gFakeDirectDrawSurface3PageUnlockCalls;
    if (index >= gFakeDirectDrawSurface3PageUnlockResultCount) {
        index = gFakeDirectDrawSurface3PageUnlockResultCount - 1;
    }
    ++gFakeDirectDrawSurface3PageUnlockCalls;
    gFakeDirectDrawSurface3LastPageUnlockSurface = surface;
    gFakeDirectDrawSurface3LastPageUnlockFlags = flags;
    return gFakeDirectDrawSurface3PageUnlockResults[index];
}

void InstallFakeD3DDevice2(FakeD3DDevice2Object &device) {
    std::memset(gFakeD3DDevice2VTable, 0, sizeof(gFakeD3DDevice2VTable));
    gFakeD3DDevice2VTable[3] = reinterpret_cast<void *>(FakeD3DDevice2_GetCaps);
    gFakeD3DDevice2VTable[6] = reinterpret_cast<void *>(FakeD3DDevice2_AddViewport);
    gFakeD3DDevice2VTable[10] = reinterpret_cast<void *>(FakeD3DDevice2_BeginScene);
    gFakeD3DDevice2VTable[11] = reinterpret_cast<void *>(FakeD3DDevice2_EndScene);
    gFakeD3DDevice2VTable[13] =
        reinterpret_cast<void *>(FakeD3DDevice2_SetCurrentViewport);
    gFakeD3DDevice2VTable[23] = reinterpret_cast<void *>(FakeD3DDevice2_SetRenderState);
    gFakeD3DDevice2VTable[25] = reinterpret_cast<void *>(FakeD3DDevice2_SetLightState);
    gFakeD3DDevice2VTable[29] = reinterpret_cast<void *>(FakeD3DDevice2_DrawPrimitive);
    device.vtable = gFakeD3DDevice2VTable;
    g_zVideo_pD3DDevice = reinterpret_cast<IDirect3DDevice2 *>(&device);
    gFakeD3DBeginSceneResult = DD_OK;
    gFakeD3DEndSceneResult = DD_OK;
    gFakeD3DAddViewportResult = DD_OK;
    gFakeD3DSetCurrentViewportResult = DD_OK;
    gFakeD3DGetCapsResult = DD_OK;
    gFakeD3DDrawPrimitiveResult = DD_OK;
    gFakeD3DBeginSceneCalls = 0;
    gFakeD3DEndSceneCalls = 0;
    gFakeD3DAddViewportCalls = 0;
    gFakeD3DSetCurrentViewportCalls = 0;
    gFakeD3DGetCapsCalls = 0;
    gFakeD3DDrawPrimitiveCalls = 0;
    gFakeD3DLastAddViewport = nullptr;
    gFakeD3DLastSetCurrentViewport = nullptr;
    gFakeD3DLastGetCapsHalDesc = nullptr;
    gFakeD3DLastGetCapsHelDesc = nullptr;
    gFakeD3DLastGetCapsHalDescValue = {};
    gFakeD3DLastGetCapsHelDescValue = {};
    gFakeD3DLastPrimitiveType = (D3DPRIMITIVETYPE)(0);
    gFakeD3DLastVertexType = (D3DVERTEXTYPE)(0);
    gFakeD3DLastVertices = nullptr;
    gFakeD3DLastVertexCount = 0;
    gFakeD3DLastDrawFlags = 0xffffffff;
    gFakeD3DSetRenderStateCalls = 0;
    gFakeD3DSetLightStateCalls = 0;
    std::memset(gFakeD3DDrawPrimitiveTypes, 0, sizeof(gFakeD3DDrawPrimitiveTypes));
    std::memset(
        gFakeD3DDrawPrimitiveVertexTypes,
        0,
        sizeof(gFakeD3DDrawPrimitiveVertexTypes)
    );
    std::memset(gFakeD3DDrawPrimitiveVertices, 0, sizeof(gFakeD3DDrawPrimitiveVertices));
    std::memset(
        gFakeD3DDrawPrimitiveVertexCounts,
        0,
        sizeof(gFakeD3DDrawPrimitiveVertexCounts)
    );
    std::memset(gFakeD3DDrawPrimitiveFlags, 0, sizeof(gFakeD3DDrawPrimitiveFlags));
    std::memset(gFakeD3DRenderStates, 0, sizeof(gFakeD3DRenderStates));
    std::memset(gFakeD3DRenderStateValues, 0, sizeof(gFakeD3DRenderStateValues));
    std::memset(gFakeD3DLightStates, 0, sizeof(gFakeD3DLightStates));
    std::memset(gFakeD3DLightStateValues, 0, sizeof(gFakeD3DLightStateValues));
}

void InstallFakeD3D2(
    FakeD3D2Object &d3d,
    IDirect3DDevice2 *createdDevice,
    IDirect3DViewport2 *createdViewport,
    IDirect3DMaterial2 *createdMaterial
) {
    std::memset(gFakeD3D2VTable, 0, sizeof(gFakeD3D2VTable));
    gFakeD3D2VTable[2] = reinterpret_cast<void *>(FakeD3D2_Release);
    gFakeD3D2VTable[3] = reinterpret_cast<void *>(FakeD3D2_EnumDevices);
    gFakeD3D2VTable[5] = reinterpret_cast<void *>(FakeD3D2_CreateMaterial);
    gFakeD3D2VTable[6] = reinterpret_cast<void *>(FakeD3D2_CreateViewport);
    gFakeD3D2VTable[8] = reinterpret_cast<void *>(FakeD3D2_CreateDevice);
    d3d.vtable = gFakeD3D2VTable;
    gFakeD3D2CreateDeviceResult = DD_OK;
    gFakeD3D2CreateViewportResult = DD_OK;
    gFakeD3D2CreateMaterialResult = DD_OK;
    gFakeD3D2ReleaseCalls = 0;
    gFakeD3D2EnumDevicesCalls = 0;
    gFakeD3D2CreateDeviceCalls = 0;
    gFakeD3D2CreateViewportCalls = 0;
    gFakeD3D2CreateMaterialCalls = 0;
    gFakeD3D2LastEnumDevicesCallback = nullptr;
    gFakeD3D2LastEnumDevicesContext = nullptr;
    gFakeD3D2EnumDevicesInitialAcceptedCount = -1;
    gFakeD3D2EnumDevicesAcceptedCount = 0;
    gFakeD3D2LastCreateDeviceGuid = nullptr;
    gFakeD3D2LastCreateDeviceSurface = nullptr;
    gFakeD3D2LastCreateDeviceOut = nullptr;
    gFakeD3D2LastCreateViewportOut = nullptr;
    gFakeD3D2LastCreateViewportOuter = reinterpret_cast<IUnknown *>(1);
    gFakeD3D2LastCreateMaterialOut = nullptr;
    gFakeD3D2LastCreateMaterialOuter = reinterpret_cast<IUnknown *>(1);
    gFakeD3D2CreatedDevice = createdDevice;
    gFakeD3D2CreatedViewport = createdViewport;
    gFakeD3D2CreatedMaterial = createdMaterial;
}

void InstallFakeD3DViewport2(FakeD3DViewport2Object &viewport) {
    std::memset(gFakeD3DViewport2VTable, 0, sizeof(gFakeD3DViewport2VTable));
    gFakeD3DViewport2VTable[8] =
        reinterpret_cast<void *>(FakeD3DViewport2_SetBackground);
    gFakeD3DViewport2VTable[17] =
        reinterpret_cast<void *>(FakeD3DViewport2_SetViewport2);
    viewport.vtable = gFakeD3DViewport2VTable;
    gFakeD3DViewport2SetViewport2Result = DD_OK;
    gFakeD3DViewport2SetBackgroundResult = DD_OK;
    gFakeD3DViewport2SetViewport2Calls = 0;
    gFakeD3DViewport2SetBackgroundCalls = 0;
    gFakeD3DViewport2LastViewport = nullptr;
    gFakeD3DViewport2LastViewportValue = {};
    gFakeD3DViewport2LastBackground = 0;
}

void InstallFakeD3DMaterial2(FakeD3DMaterial2Object &material) {
    std::memset(gFakeD3DMaterial2VTable, 0, sizeof(gFakeD3DMaterial2VTable));
    gFakeD3DMaterial2VTable[3] =
        reinterpret_cast<void *>(FakeD3DMaterial2_SetMaterial);
    gFakeD3DMaterial2VTable[5] =
        reinterpret_cast<void *>(FakeD3DMaterial2_GetHandle);
    material.vtable = gFakeD3DMaterial2VTable;
    gFakeD3DMaterial2SetMaterialResult = DD_OK;
    gFakeD3DMaterial2GetHandleResult = DD_OK;
    gFakeD3DMaterial2SetMaterialCalls = 0;
    gFakeD3DMaterial2GetHandleCalls = 0;
    gFakeD3DMaterial2LastMaterial = nullptr;
    gFakeD3DMaterial2LastMaterialValue = {};
    gFakeD3DMaterial2LastGetHandleDevice = nullptr;
    gFakeD3DMaterial2LastGetHandleOut = nullptr;
    gFakeD3DMaterial2HandleValue = 0x2468;
}

void InstallFakeD3DTexture2(
    FakeD3DTexture2Object &uploadTexture,
    FakeD3DTexture2Object &texture
) {
    std::memset(gFakeD3DTexture2VTable, 0, sizeof(gFakeD3DTexture2VTable));
    gFakeD3DTexture2VTable[2] = reinterpret_cast<void *>(FakeD3DTexture2_Release);
    gFakeD3DTexture2VTable[3] = reinterpret_cast<void *>(FakeD3DTexture2_GetHandle);
    gFakeD3DTexture2VTable[5] = reinterpret_cast<void *>(FakeD3DTexture2_Load);
    uploadTexture.vtable = gFakeD3DTexture2VTable;
    texture.vtable = gFakeD3DTexture2VTable;
    gFakeD3DTexture2LoadResult = DD_OK;
    gFakeD3DTexture2GetHandleResult = DD_OK;
    gFakeD3DTexture2LoadCalls = 0;
    gFakeD3DTexture2GetHandleCalls = 0;
    gFakeD3DTexture2ReleaseCalls = 0;
    gFakeD3DTexture2LastLoadSelf = nullptr;
    gFakeD3DTexture2LastLoadSource = nullptr;
    gFakeD3DTexture2LastGetHandleSelf = nullptr;
    gFakeD3DTexture2LastGetHandleDevice = nullptr;
    gFakeD3DTexture2LastGetHandleOut = nullptr;
    gFakeD3DTexture2HandleValue = 0x3579;
    std::memset(
        gFakeD3DTexture2ReleaseObjects,
        0,
        sizeof(gFakeD3DTexture2ReleaseObjects)
    );
}

void InstallFakeComObject(FakeComObject &object) {
    std::memset(gFakeComVTable, 0, sizeof(gFakeComVTable));
    gFakeComVTable[2] = reinterpret_cast<void *>(FakeCom_Release);
    object.vtable = gFakeComVTable;
}

void InstallFakeDirectDrawPalette(FakeDirectDrawPaletteObject &palette) {
    std::memset(gFakeDirectDrawPaletteVTable, 0, sizeof(gFakeDirectDrawPaletteVTable));
    gFakeDirectDrawPaletteVTable[6] =
        reinterpret_cast<void *>(FakeDirectDrawPalette_SetEntries);
    palette.vtable = gFakeDirectDrawPaletteVTable;
    gFakeDirectDrawPaletteSetEntriesResult = DD_OK;
    gFakeDirectDrawPaletteSetEntriesCalls = 0;
    gFakeDirectDrawPaletteLastSetEntriesSelf = nullptr;
    gFakeDirectDrawPaletteLastSetEntriesFlags = 0xffffffff;
    gFakeDirectDrawPaletteLastSetEntriesFirst = 0xffffffff;
    gFakeDirectDrawPaletteLastSetEntriesCount = 0xffffffff;
    gFakeDirectDrawPaletteLastSetEntriesEntries = nullptr;
}

void ResetFakeComReleaseTracking() {
    gFakeComReleaseCalls = 0;
    std::memset(gFakeComReleaseObjects, 0, sizeof(gFakeComReleaseObjects));
}

void InstallFakeSurfaceLockVerifier() {
    gFakeSurfaceLockVerifierVerifyCalls = 0;
    gFakeSurfaceLockVerifierLastArgs = {};
    gFakeSurfaceLockVerifierVerifyResult = DD_OK;
    gFakeSurfaceLockVerifierReleaseCalls = 0;
    g_zVideo_pSurfaceLockVerifier = &gFakeSurfaceLockVerifier;
}

void InstallFakeDirectDraw(
    FakeDirectDrawObject &directDraw,
    IDirectDraw2 *queryResult
) {
    std::memset(gFakeDirectDrawVTable, 0, sizeof(gFakeDirectDrawVTable));
    gFakeDirectDrawVTable[0] = reinterpret_cast<void *>(FakeDirectDraw_QueryInterface);
    gFakeDirectDrawVTable[2] = reinterpret_cast<void *>(FakeDirectDraw_Release);
    directDraw.vtable = gFakeDirectDrawVTable;

    gFakeDirectDrawCreateResult = DD_OK;
    gFakeDirectDrawCreateCalls = 0;
    gFakeDirectDrawCreateGuid = nullptr;
    gFakeDirectDrawCreateOut = nullptr;
    gFakeDirectDrawCreateOuter = reinterpret_cast<IUnknown *>(1);
    gFakeDirectDrawCreateValue = reinterpret_cast<IDirectDraw *>(&directDraw);
    gFakeDirectDrawQueryInterfaceResult = DD_OK;
    gFakeDirectDrawQueryInterfaceCalls = 0;
    gFakeDirectDrawQueryInterfaceSelf = nullptr;
    gFakeDirectDrawQueryInterfaceIid = nullptr;
    gFakeDirectDrawQueryInterfaceOut = nullptr;
    gFakeDirectDrawQueryInterfaceValue = queryResult;
    gFakeDirectDrawReleaseCalls = 0;
    gFakeDirectDrawReleaseSelf = nullptr;
}

void InstallFakeDirectDraw2(
    FakeDirectDraw2Object &directDraw,
    FakeDirectDrawSurfaceObject &createdSurface,
    FakeDirectDrawSurface3Object &surface3
) {
    std::memset(gFakeDirectDraw2VTable, 0, sizeof(gFakeDirectDraw2VTable));
    gFakeDirectDraw2VTable[0] =
        reinterpret_cast<void *>(FakeDirectDraw2_QueryInterface);
    gFakeDirectDraw2VTable[2] = reinterpret_cast<void *>(FakeDirectDraw2_Release);
    gFakeDirectDraw2VTable[5] = reinterpret_cast<void *>(FakeDirectDraw2_CreatePalette);
    gFakeDirectDraw2VTable[6] = reinterpret_cast<void *>(FakeDirectDraw2_CreateSurface);
    gFakeDirectDraw2VTable[4] = reinterpret_cast<void *>(FakeDirectDraw2_CreateClipper);
    gFakeDirectDraw2VTable[8] = reinterpret_cast<void *>(FakeDirectDraw2_CreateClipper);
    gFakeDirectDraw2VTable[11] = reinterpret_cast<void *>(FakeDirectDraw2_GetCaps);
    gFakeDirectDraw2VTable[20] =
        reinterpret_cast<void *>(FakeDirectDraw2_SetCooperativeLevel);
    gFakeDirectDraw2VTable[21] =
        reinterpret_cast<void *>(FakeDirectDraw2_SetDisplayMode);
    gFakeDirectDraw2VTable[23] =
        reinterpret_cast<void *>(FakeDirectDraw2_GetAvailableVidMem);
    directDraw.vtable = gFakeDirectDraw2VTable;

    std::memset(gFakeDirectDrawClipperVTable, 0, sizeof(gFakeDirectDrawClipperVTable));
    gFakeDirectDrawClipperVTable[4] =
        reinterpret_cast<void *>(FakeDirectDrawClipper_SetHWnd);
    gFakeDirectDrawClipperVTable[8] =
        reinterpret_cast<void *>(FakeDirectDrawClipper_SetHWnd);

    std::memset(gFakeDirectDrawSurfaceVTable, 0, sizeof(gFakeDirectDrawSurfaceVTable));
    gFakeDirectDrawSurfaceVTable[0] =
        reinterpret_cast<void *>(FakeDirectDrawSurface_QueryInterface);
    gFakeDirectDrawSurfaceVTable[2] =
        reinterpret_cast<void *>(FakeDirectDrawSurface_Release);
    gFakeDirectDrawSurfaceVTable[31] =
        reinterpret_cast<void *>(FakeDirectDrawSurface_SetPalette);
    createdSurface.vtable = gFakeDirectDrawSurfaceVTable;

    gFakeDirectDraw2CreateSurfaceResult = DD_OK;
    gFakeDirectDraw2CreateSurfaceCalls = 0;
    gFakeDirectDraw2LastCreateSurfaceDesc = nullptr;
    std::memset(
        gFakeDirectDraw2CreateSurfaceDescs,
        0,
        sizeof(gFakeDirectDraw2CreateSurfaceDescs)
    );
    gFakeDirectDraw2LastCreateSurfaceOut = nullptr;
    gFakeDirectDraw2LastCreateSurfaceOuter = reinterpret_cast<IUnknown *>(1);
    gFakeDirectDraw2CreatedSurface =
        reinterpret_cast<IDirectDrawSurface *>(&createdSurface);
    gFakeDirectDraw2CreatePaletteResult = DD_OK;
    gFakeDirectDraw2CreatePaletteCalls = 0;
    gFakeDirectDraw2LastCreatePaletteFlags = 0xffffffff;
    gFakeDirectDraw2LastCreatePaletteEntries = nullptr;
    gFakeDirectDraw2LastCreatePaletteOut = nullptr;
    gFakeDirectDraw2LastCreatePaletteOuter = reinterpret_cast<IUnknown *>(1);
    gFakeDirectDraw2CreatedPalette = nullptr;
    gFakeDirectDraw2MutateImageOnFirstCreateSurface = nullptr;
    gFakeDirectDraw2MutatedPalette = nullptr;
    gFakeDirectDraw2MutatedPaletteMetaPacked = 0;
    gFakeDirectDraw2ReleaseCalls = 0;
    gFakeDirectDraw2CreateClipperResult = DD_OK;
    gFakeDirectDraw2CreateClipperCalls = 0;
    gFakeDirectDraw2LastCreateClipperFlags = 0xffffffff;
    gFakeDirectDraw2LastCreateClipperOut = nullptr;
    gFakeDirectDraw2LastCreateClipperOuter = reinterpret_cast<IUnknown *>(1);
    gFakeDirectDraw2CreatedClipper = nullptr;
    gFakeDirectDraw2SetCooperativeLevelResult = DD_OK;
    gFakeDirectDraw2SetCooperativeLevelCalls = 0;
    gFakeDirectDraw2LastSetCooperativeHwnd = reinterpret_cast<HWND>(1);
    gFakeDirectDraw2LastSetCooperativeFlags = 0xffffffff;
    gFakeDirectDraw2SetDisplayModeResult = DD_OK;
    gFakeDirectDraw2SetDisplayModeCalls = 0;
    gFakeDirectDraw2LastDisplayModeWidth = 0xffffffff;
    gFakeDirectDraw2LastDisplayModeHeight = 0xffffffff;
    gFakeDirectDraw2LastDisplayModeBpp = 0xffffffff;
    gFakeDirectDraw2LastDisplayModeRefreshRate = 0xffffffff;
    gFakeDirectDraw2LastDisplayModeFlags = 0xffffffff;
    gFakeDirectDraw2GetCapsResult = DD_OK;
    gFakeDirectDraw2GetCapsCalls = 0;
    gFakeDirectDraw2LastGetCapsHal = nullptr;
    gFakeDirectDraw2LastGetCapsHel = nullptr;
    gFakeDirectDraw2GetCapsHalInput = {};
    gFakeDirectDraw2GetCapsHelInput = {};
    gFakeDirectDraw2GetCapsHalValue = {};
    gFakeDirectDraw2GetCapsHelValue = {};
    gFakeDirectDraw2GetCapsHalValue.dwSize = sizeof(DDCAPS);
    gFakeDirectDraw2GetCapsHelValue.dwSize = sizeof(DDCAPS);
    gFakeDirectDraw2GetAvailableVidMemResult = DD_OK;
    gFakeDirectDraw2GetAvailableVidMemCalls = 0;
    gFakeDirectDraw2LastAvailableVidMemCaps = nullptr;
    gFakeDirectDraw2LastAvailableVidMemCapsValue = {};
    gFakeDirectDraw2LastAvailableVidMemTotal = nullptr;
    gFakeDirectDraw2LastAvailableVidMemFree = nullptr;
    gFakeDirectDraw2AvailableVidMemTotal = 0;
    gFakeDirectDraw2AvailableVidMemFree = 0;
    gFakeDirectDrawClipperSetHWndResult = DD_OK;
    gFakeDirectDrawClipperSetHWndCalls = 0;
    gFakeDirectDrawClipperLastSetHWndFlags = 0xffffffff;
    gFakeDirectDrawClipperLastSetHWnd = reinterpret_cast<HWND>(1);
    gFakeDirectDrawSurfaceQueryInterfaceResult = DD_OK;
    gFakeDirectDrawSurfaceQueryInterfaceCalls = 0;
    gFakeDirectDrawSurfaceLastQueryInterfaceIid = nullptr;
    gFakeDirectDrawSurfaceLastQueryInterfaceOut = nullptr;
    gFakeDirectDrawSurfaceQueryInterfaceValue =
        reinterpret_cast<void *>(&surface3);
    std::memset(
        gFakeDirectDrawSurfaceQueryInterfaceValues,
        0,
        sizeof(gFakeDirectDrawSurfaceQueryInterfaceValues)
    );
    gFakeDirectDrawSurfaceQueryInterfaceValueCount = 0;
    gFakeDirectDrawSurfaceReleaseCalls = 0;
    gFakeDirectDrawSurfaceReleaseResult = 0;
    gFakeDirectDrawSurfaceSetPaletteResult = DD_OK;
    gFakeDirectDrawSurfaceSetPaletteCalls = 0;
    std::memset(
        gFakeDirectDrawSurfaceSetPaletteSurfaces,
        0,
        sizeof(gFakeDirectDrawSurfaceSetPaletteSurfaces)
    );
    std::memset(
        gFakeDirectDrawSurfaceSetPalettePalettes,
        0,
        sizeof(gFakeDirectDrawSurfaceSetPalettePalettes)
    );
    gFakeDirectDraw2QueryInterfaceResult = DD_OK;
    gFakeDirectDraw2QueryInterfaceCalls = 0;
    gFakeDirectDraw2LastQueryInterfaceIid = nullptr;
    gFakeDirectDraw2LastQueryInterfaceOut = nullptr;
    gFakeDirectDraw2QueryInterfaceValue = nullptr;
}

void InstallFakeDirectDrawSurface3(FakeDirectDrawSurface3Object &surface,
                                   HRESULT firstUnlockResult,
                                   HRESULT secondUnlockResult,
                                   HRESULT restoreResult) {
    std::memset(gFakeDirectDrawSurface3VTable, 0, sizeof(gFakeDirectDrawSurface3VTable));
    gFakeDirectDrawSurface3VTable[0] =
        reinterpret_cast<void *>(FakeDirectDrawSurface3_QueryInterface);
    gFakeDirectDrawSurface3VTable[2] =
        reinterpret_cast<void *>(FakeDirectDrawSurface3_Release);
    gFakeDirectDrawSurface3VTable[3] =
        reinterpret_cast<void *>(FakeDirectDrawSurface3_AddAttachedSurface);
    gFakeDirectDrawSurface3VTable[5] =
        reinterpret_cast<void *>(FakeDirectDrawSurface3_Blt);
    gFakeDirectDrawSurface3VTable[11] =
        reinterpret_cast<void *>(FakeDirectDrawSurface3_Flip);
    gFakeDirectDrawSurface3VTable[12] =
        reinterpret_cast<void *>(FakeDirectDrawSurface3_GetAttachedSurface);
    gFakeDirectDrawSurface3VTable[17] =
        reinterpret_cast<void *>(FakeDirectDrawSurface3_GetDC);
    gFakeDirectDrawSurface3VTable[26] =
        reinterpret_cast<void *>(FakeDirectDrawSurface3_ReleaseDC);
    gFakeDirectDrawSurface3VTable[28] =
        reinterpret_cast<void *>(FakeDirectDrawSurface3_SetClipper);
    gFakeDirectDrawSurface3VTable[25] =
        reinterpret_cast<void *>(FakeDirectDrawSurface3_Lock);
    gFakeDirectDrawSurface3VTable[21] =
        reinterpret_cast<void *>(FakeDirectDrawSurface3_GetPixelFormat);
    gFakeDirectDrawSurface3VTable[27] =
        reinterpret_cast<void *>(FakeDirectDrawSurface3_Restore);
    gFakeDirectDrawSurface3VTable[32] =
        reinterpret_cast<void *>(FakeDirectDrawSurface3_Unlock);
    gFakeDirectDrawSurface3VTable[37] =
        reinterpret_cast<void *>(FakeDirectDrawSurface3_PageLock);
    gFakeDirectDrawSurface3VTable[38] =
        reinterpret_cast<void *>(FakeDirectDrawSurface3_PageUnlock);
    surface.vtable = gFakeDirectDrawSurface3VTable;
    gFakeDirectDrawSurface3BltResults[0] = DD_OK;
    gFakeDirectDrawSurface3BltResults[1] = DD_OK;
    gFakeDirectDrawSurface3BltResultCount = 2;
    gFakeDirectDrawSurface3BltCalls = 0;
    gFakeDirectDrawSurface3LastBltDstRect = {};
    gFakeDirectDrawSurface3LastBltSrcRect = {};
    gFakeDirectDrawSurface3LastBltDstRectArg = nullptr;
    gFakeDirectDrawSurface3LastBltSrcRectArg = nullptr;
    gFakeDirectDrawSurface3LastBltSource = nullptr;
    gFakeDirectDrawSurface3LastBltFlags = 0;
    gFakeDirectDrawSurface3LastBltFx = reinterpret_cast<LPDDBLTFX>(1);
    gFakeDirectDrawSurface3LastBltFxValue = {};
    std::memset(
        gFakeDirectDrawSurface3BltSurfaces,
        0,
        sizeof(gFakeDirectDrawSurface3BltSurfaces)
    );
    std::memset(
        gFakeDirectDrawSurface3BltDstRectArgs,
        0,
        sizeof(gFakeDirectDrawSurface3BltDstRectArgs)
    );
    std::memset(
        gFakeDirectDrawSurface3BltSrcRectArgs,
        0,
        sizeof(gFakeDirectDrawSurface3BltSrcRectArgs)
    );
    std::memset(
        gFakeDirectDrawSurface3BltFlags,
        0,
        sizeof(gFakeDirectDrawSurface3BltFlags)
    );
    std::memset(
        gFakeDirectDrawSurface3BltFxValues,
        0,
        sizeof(gFakeDirectDrawSurface3BltFxValues)
    );
    gFakeDirectDrawSurface3LockResults[0] = DD_OK;
    gFakeDirectDrawSurface3LockResults[1] = DD_OK;
    gFakeDirectDrawSurface3LockResultCount = 2;
    gFakeDirectDrawSurface3LockCalls = 0;
    std::memset(
        gFakeDirectDrawSurface3LockSurfaces,
        0,
        sizeof(gFakeDirectDrawSurface3LockSurfaces)
    );
    gFakeDirectDrawSurface3LastLockRect = reinterpret_cast<LPRECT>(1);
    gFakeDirectDrawSurface3LastLockDesc = nullptr;
    gFakeDirectDrawSurface3LastLockFlags = 0;
    gFakeDirectDrawSurface3LastLockEvent = reinterpret_cast<HANDLE>(1);
    gFakeDirectDrawSurface3LockDescSize = 0;
    gFakeDirectDrawSurface3LockPixels = reinterpret_cast<void *>(0x12345678);
    gFakeDirectDrawSurface3LockPitch = 1280;
    gFakeDirectDrawSurface3UnlockResults[0] = firstUnlockResult;
    gFakeDirectDrawSurface3UnlockResults[1] = secondUnlockResult;
    gFakeDirectDrawSurface3UnlockResultCount = 2;
    gFakeDirectDrawSurface3UnlockCalls = 0;
    std::memset(
        gFakeDirectDrawSurface3UnlockSurfaces,
        0,
        sizeof(gFakeDirectDrawSurface3UnlockSurfaces)
    );
    gFakeDirectDrawSurface3LastUnlockArg = reinterpret_cast<LPVOID>(1);
    gFakeDirectDrawSurface3FlipResults[0] = DD_OK;
    gFakeDirectDrawSurface3FlipResults[1] = DD_OK;
    gFakeDirectDrawSurface3FlipResultCount = 2;
    gFakeDirectDrawSurface3FlipCalls = 0;
    std::memset(
        gFakeDirectDrawSurface3FlipSurfaces,
        0,
        sizeof(gFakeDirectDrawSurface3FlipSurfaces)
    );
    gFakeDirectDrawSurface3LastFlipTarget = reinterpret_cast<IDirectDrawSurface3 *>(1);
    gFakeDirectDrawSurface3LastFlipFlags = 0xffffffff;
    gFakeDirectDrawSurface3RestoreResult = restoreResult;
    gFakeDirectDrawSurface3RestoreCalls = 0;
    std::memset(
        gFakeDirectDrawSurface3RestoreSurfaces,
        0,
        sizeof(gFakeDirectDrawSurface3RestoreSurfaces)
    );
    gFakeDirectDrawSurface3ReleaseCalls = 0;
    std::memset(
        gFakeDirectDrawSurface3ReleaseSurfaces,
        0,
        sizeof(gFakeDirectDrawSurface3ReleaseSurfaces)
    );
    gFakeDirectDrawSurface3GetAttachedSurfaceResult = DD_OK;
    gFakeDirectDrawSurface3GetAttachedSurfaceCalls = 0;
    gFakeDirectDrawSurface3LastAttachedCaps = nullptr;
    gFakeDirectDrawSurface3LastAttachedCapsValue = {};
    gFakeDirectDrawSurface3LastAttachedSurfaceOut = nullptr;
    gFakeDirectDrawSurface3AttachedSurface = nullptr;
    gFakeDirectDrawSurface3SetClipperResult = DD_OK;
    gFakeDirectDrawSurface3SetClipperCalls = 0;
    gFakeDirectDrawSurface3LastSetClipper = nullptr;
    gFakeDirectDrawSurface3GetDCResult = DD_OK;
    gFakeDirectDrawSurface3GetDCCalls = 0;
    gFakeDirectDrawSurface3LastGetDCSurface = nullptr;
    gFakeDirectDrawSurface3LastGetDCOut = nullptr;
    gFakeDirectDrawSurface3GetDCValue = reinterpret_cast<HDC>(0x4321);
    gFakeDirectDrawSurface3ReleaseDCResult = DD_OK;
    gFakeDirectDrawSurface3ReleaseDCCalls = 0;
    gFakeDirectDrawSurface3LastReleaseDCSurface = nullptr;
    gFakeDirectDrawSurface3LastReleaseDCHdc = reinterpret_cast<HDC>(1);
    gFakeDirectDrawSurface3GetPixelFormatResult = DD_OK;
    gFakeDirectDrawSurface3GetPixelFormatCalls = 0;
    gFakeDirectDrawSurface3PixelFormat = {};
    gFakeDirectDrawSurface3PixelFormat.dwSize = sizeof(gFakeDirectDrawSurface3PixelFormat);
    gFakeDirectDrawSurface3LastPixelFormat = nullptr;
    gFakeDirectDrawSurface3LastPixelFormatInputSize = 0;
    gFakeDirectDrawSurface3PageLockResults[0] = DD_OK;
    gFakeDirectDrawSurface3PageLockResults[1] = DD_OK;
    gFakeDirectDrawSurface3PageLockResultCount = 2;
    gFakeDirectDrawSurface3PageLockCalls = 0;
    gFakeDirectDrawSurface3LastPageLockSurface = nullptr;
    gFakeDirectDrawSurface3LastPageLockFlags = 0xffffffff;
    gFakeDirectDrawSurface3PageUnlockResults[0] = DD_OK;
    gFakeDirectDrawSurface3PageUnlockResults[1] = DD_OK;
    gFakeDirectDrawSurface3PageUnlockResultCount = 2;
    gFakeDirectDrawSurface3PageUnlockCalls = 0;
    gFakeDirectDrawSurface3LastPageUnlockSurface = nullptr;
    gFakeDirectDrawSurface3LastPageUnlockFlags = 0xffffffff;
    gFakeDirectDrawSurface3QueryInterfaceResult = DD_OK;
    gFakeDirectDrawSurface3QueryInterfaceCalls = 0;
    gFakeDirectDrawSurface3LastQueryInterfaceIid = nullptr;
    gFakeDirectDrawSurface3LastQueryInterfaceOut = nullptr;
    gFakeDirectDrawSurface3QueryInterfaceValue = nullptr;
    gFakeDirectDrawSurface3AddAttachedSurfaceResult = DD_OK;
    gFakeDirectDrawSurface3AddAttachedSurfaceCalls = 0;
    gFakeDirectDrawSurface3LastAddAttachedSelf = nullptr;
    gFakeDirectDrawSurface3LastAttachedSurfaceArg = nullptr;
}

void ConfigureFakeDirectDrawSurface3LockResults(HRESULT firstLockResult,
                                                HRESULT secondLockResult) {
    gFakeDirectDrawSurface3LockResults[0] = firstLockResult;
    gFakeDirectDrawSurface3LockResults[1] = secondLockResult;
}

void ConfigureFakeDirectDrawSurface3BltResults(
    HRESULT firstBltResult,
    HRESULT secondBltResult
) {
    gFakeDirectDrawSurface3BltResults[0] = firstBltResult;
    gFakeDirectDrawSurface3BltResults[1] = secondBltResult;
    gFakeDirectDrawSurface3BltResultCount = 2;
}

void ConfigureFakeDirectDrawSurface3FlipResults(
    HRESULT firstFlipResult,
    HRESULT secondFlipResult
) {
    gFakeDirectDrawSurface3FlipResults[0] = firstFlipResult;
    gFakeDirectDrawSurface3FlipResults[1] = secondFlipResult;
    gFakeDirectDrawSurface3FlipResultCount = 2;
}

void CaptureFlushSortedPolys() {
    ++g_zVideoRenderFrameFlushSortedCount;
}

void CaptureFlushOverwritePolys() {
    ++g_zVideoRenderFrameFlushOverwriteCount;
}

void CaptureFlushQuadBatch() {
    ++g_zVideoRenderFrameFlushQuadCount;
}

void __fastcall CaptureClearZBufferRect(zVidRect32 *rect) {
    const int index = g_zVideoRenderFrameClearRectCount;
    if (index < 4) {
        g_zVideoRenderFrameClearRects[index] = *rect;
    }
    ++g_zVideoRenderFrameClearRectCount;
}

int __fastcall CaptureLockSurfaceState(zVideo_SurfaceStatePartial *surfaceState) {
    ++g_zVideoTestLockSurfaceCount;
    g_zVideoTestLockSurfaceState = surfaceState;
    surfaceState->locked = 1;
    return 1;
}

int __fastcall CaptureUnlockSurfaceState(zVideo_SurfaceStatePartial *surfaceState) {
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

    void Update(float deltaSeconds) {
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
    g_zVideo_PixelPack.rMaskShifted = 0xf8;
    g_zVideo_PixelPack.gMaskShifted = 0xfc;
    g_zVideo_PixelPack.packedBase = 8;
    g_zVideo_PixelPack.sumMinus8 = 3;
    g_zVideo_PixelPack.bShiftTo8 = 3;

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

    if (g_zVideo_PixelPack.rBits != 5 || g_zVideo_PixelPack.gBits != 6 ||
        g_zVideo_PixelPack.bBits != 5 || g_zVideo_PixelPack.packedBase != 8 ||
        g_zVideo_PixelPack.sumMinus8 != 3 || g_zVideo_PixelPack.bShiftTo8 != 3 ||
        g_zVideo_PixelPack.rMaskShifted != 0xf8 || g_zVideo_PixelPack.gMaskShifted != 0xfc ||
        g_zVideo_PixelPack.bMaskShifted != 0xf8) {
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

extern "C" int zvideo_dd3d_set_fog_enable_smoke(void) {
    FakeD3DDevice2Object fakeDevice = {};
    IDirect3DDevice2 *const oldDevice = g_zVideo_pD3DDevice;
    const int oldFogEnable = g_zVideo_CachedFogEnableRenderState;
    const int oldFogMode = g_zVideo_CachedFogModeLightState;

    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_CachedFogEnableRenderState = 0;
    g_zVideo_CachedFogModeLightState = 0;
    zVideo_dd3d::SetFogEnable(1);
    const bool firstCallOk =
        gFakeD3DSetRenderStateCalls == 1 &&
        gFakeD3DRenderStates[0] == D3DRENDERSTATE_FOGENABLE &&
        gFakeD3DRenderStateValues[0] == 1 &&
        gFakeD3DSetLightStateCalls == 1 &&
        gFakeD3DLightStates[0] == D3DLIGHTSTATE_FOGMODE &&
        gFakeD3DLightStateValues[0] == D3DFOG_LINEAR &&
        g_zVideo_CachedFogEnableRenderState == 1 &&
        g_zVideo_CachedFogModeLightState == D3DFOG_LINEAR;

    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_CachedFogEnableRenderState = 1;
    g_zVideo_CachedFogModeLightState = D3DFOG_LINEAR;
    zVideo_dd3d::SetFogEnable(1);
    const bool cacheHitOk =
        gFakeD3DSetRenderStateCalls == 0 &&
        gFakeD3DSetLightStateCalls == 0 &&
        g_zVideo_CachedFogEnableRenderState == 1 &&
        g_zVideo_CachedFogModeLightState == D3DFOG_LINEAR;

    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_CachedFogEnableRenderState = 1;
    g_zVideo_CachedFogModeLightState = D3DFOG_LINEAR;
    zVideo_dd3d::SetFogEnable(0);
    const bool renderOnlyOk =
        gFakeD3DSetRenderStateCalls == 1 &&
        gFakeD3DRenderStates[0] == D3DRENDERSTATE_FOGENABLE &&
        gFakeD3DRenderStateValues[0] == 0 &&
        gFakeD3DSetLightStateCalls == 0 &&
        g_zVideo_CachedFogEnableRenderState == 0 &&
        g_zVideo_CachedFogModeLightState == D3DFOG_LINEAR;

    g_zVideo_pD3DDevice = oldDevice;
    g_zVideo_CachedFogEnableRenderState = oldFogEnable;
    g_zVideo_CachedFogModeLightState = oldFogMode;
    return firstCallOk && cacheHitOk && renderOnlyOk ? 0 : 1;
}

extern "C" int zvideo_dd3d_set_fog_start_smoke(void) {
    FakeD3DDevice2Object fakeDevice = {};
    IDirect3DDevice2 *const oldDevice = g_zVideo_pD3DDevice;
    const float oldFogStart = g_zVideo_CachedFogStartLightStateValue;

    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_CachedFogStartLightStateValue = 10.0f;
    const float newFogStart = 25.5f;
    DWORD newFogStartBits = 0;
    std::memcpy(
        &newFogStartBits,
        &newFogStart,
        sizeof(newFogStartBits)
    );
    zVideo_dd3d::SetFogStart(newFogStart);
    const bool updateOk =
        gFakeD3DSetLightStateCalls == 1 &&
        gFakeD3DLightStates[0] == D3DLIGHTSTATE_FOGSTART &&
        gFakeD3DLightStateValues[0] == newFogStartBits &&
        g_zVideo_CachedFogStartLightStateValue == newFogStart;

    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_CachedFogStartLightStateValue = newFogStart;
    zVideo_dd3d::SetFogStart(newFogStart);
    const bool cacheHitOk =
        gFakeD3DSetLightStateCalls == 0 &&
        g_zVideo_CachedFogStartLightStateValue == newFogStart;

    g_zVideo_pD3DDevice = oldDevice;
    g_zVideo_CachedFogStartLightStateValue = oldFogStart;
    return updateOk && cacheHitOk ? 0 : 1;
}

extern "C" int zvideo_dd3d_set_fog_end_smoke(void) {
    FakeD3DDevice2Object fakeDevice = {};
    IDirect3DDevice2 *const oldDevice = g_zVideo_pD3DDevice;
    const float oldFogEnd = g_zVideo_CachedFogEndLightStateValue;

    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_CachedFogEndLightStateValue = 100.0f;
    const float newFogEnd = 400.25f;
    DWORD newFogEndBits = 0;
    std::memcpy(
        &newFogEndBits,
        &newFogEnd,
        sizeof(newFogEndBits)
    );
    zVideo_dd3d::SetFogEnd(newFogEnd);
    const bool updateOk =
        gFakeD3DSetLightStateCalls == 1 &&
        gFakeD3DLightStates[0] == D3DLIGHTSTATE_FOGSTART &&
        gFakeD3DLightStateValues[0] == newFogEndBits &&
        g_zVideo_CachedFogEndLightStateValue == newFogEnd;

    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_CachedFogEndLightStateValue = newFogEnd;
    zVideo_dd3d::SetFogEnd(newFogEnd);
    const bool cacheHitOk =
        gFakeD3DSetLightStateCalls == 0 &&
        g_zVideo_CachedFogEndLightStateValue == newFogEnd;

    g_zVideo_pD3DDevice = oldDevice;
    g_zVideo_CachedFogEndLightStateValue = oldFogEnd;
    return updateOk && cacheHitOk ? 0 : 1;
}

extern "C" int zvideo_dd3d_apply_fog_state_from_globals_smoke(void) {
    FakeD3DDevice2Object fakeDevice = {};
    IDirect3DDevice2 *const oldDevice = g_zVideo_pD3DDevice;
    const float oldPendingRed = g_zVideo_FogColorPendingR255;
    const float oldPendingGreen = g_zVideo_FogColorPendingG255;
    const float oldPendingBlue = g_zVideo_FogColorPendingB255;

    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_FogColorPendingR255 = 10.4f;
    g_zVideo_FogColorPendingG255 = 20.6f;
    g_zVideo_FogColorPendingB255 = 30.5f;
    const float fogStart = 11.5f;
    const float fogEnd = 99.25f;
    DWORD fogStartBits = 0;
    DWORD fogEndBits = 0;
    std::memcpy(
        &fogStartBits,
        &fogStart,
        sizeof(fogStartBits)
    );
    std::memcpy(
        &fogEndBits,
        &fogEnd,
        sizeof(fogEndBits)
    );

    zVideo_dd3d::ApplyFogStateFromGlobals(
        fogStart,
        fogEnd,
        -1.0f
    );

    const bool ok =
        gFakeD3DSetRenderStateCalls == 2 &&
        gFakeD3DRenderStates[0] == D3DRENDERSTATE_FOGENABLE &&
        gFakeD3DRenderStateValues[0] == 1 &&
        gFakeD3DRenderStates[1] == D3DRENDERSTATE_FOGCOLOR &&
        gFakeD3DRenderStateValues[1] == 0x0a151f &&
        gFakeD3DSetLightStateCalls == 3 &&
        gFakeD3DLightStates[0] == D3DLIGHTSTATE_FOGMODE &&
        gFakeD3DLightStateValues[0] == D3DFOG_LINEAR &&
        gFakeD3DLightStates[1] == D3DLIGHTSTATE_FOGSTART &&
        gFakeD3DLightStateValues[1] == fogStartBits &&
        gFakeD3DLightStates[2] == D3DLIGHTSTATE_FOGEND &&
        gFakeD3DLightStateValues[2] == fogEndBits;

    g_zVideo_pD3DDevice = oldDevice;
    g_zVideo_FogColorPendingR255 = oldPendingRed;
    g_zVideo_FogColorPendingG255 = oldPendingGreen;
    g_zVideo_FogColorPendingB255 = oldPendingBlue;
    return ok ? 0 : 1;
}

extern "C" int zvideo_dd3d_update_fog_color_smoke(void) {
    FakeD3DDevice2Object fakeDevice = {};
    IDirect3DDevice2 *const oldDevice = g_zVideo_pD3DDevice;
    const float oldAppliedRed = g_zVideo_FogColorAppliedR255;
    const float oldAppliedGreen = g_zVideo_FogColorAppliedG255;
    const float oldAppliedBlue = g_zVideo_FogColorAppliedB255;

    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_FogColorAppliedR255 = 40.4f;
    g_zVideo_FogColorAppliedG255 = 50.6f;
    g_zVideo_FogColorAppliedB255 = 60.5f;

    zVideo_dd3d::UpdateFogColor();

    const bool ok =
        gFakeD3DSetRenderStateCalls == 1 &&
        gFakeD3DRenderStates[0] == D3DRENDERSTATE_FOGCOLOR &&
        gFakeD3DRenderStateValues[0] == 0x28333d &&
        gFakeD3DSetLightStateCalls == 0;

    g_zVideo_pD3DDevice = oldDevice;
    g_zVideo_FogColorAppliedR255 = oldAppliedRed;
    g_zVideo_FogColorAppliedG255 = oldAppliedGreen;
    g_zVideo_FogColorAppliedB255 = oldAppliedBlue;
    return ok ? 0 : 1;
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
        FxPass3FieldAt<int>(kFxPass3SlotsOffset + kFxPass3SlotCurrentRadiusOffset) == 33 &&
        FxPass3FieldAt<int>(kFxPass3SlotsOffset + kFxPass3SlotMaxRadiusOffset) == 44 &&
        FxPass3FieldAt<int>(kFxPass3SlotsOffset + kFxPass3SlotExtentOffset) == 55 &&
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

extern "C" int zvideo_fxpass3_element_draw_smoke(void) {
    struct TestFxPass3ElementFTable {
        unsigned int slots[30];
    };

    unsigned char savedConfig[kFxPass3ConfigSize] = {};
    std::memcpy(savedConfig, FxPass3ConfigBytes(), sizeof(savedConfig));
    unsigned short *const oldFxPixels = g_zVideo_FxSurfacePixels16;
    const int oldFxWidth = g_zVideo_FxSurfaceWidth;
    const int oldFxHeight = g_zVideo_FxSurfaceHeight;
    const int oldFxPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldFxPitchPixels = g_zVideo_FxSurfacePitchPixels16;

    TestFxPass3ElementFTable table = {};
    table.slots[2] =
        static_cast<unsigned int>(reinterpret_cast<std::uintptr_t>(&CaptureFxPass3DrawBase));
    table.slots[0x74 / 4] = static_cast<unsigned int>(
        reinterpret_cast<std::uintptr_t>(&CaptureFxPass3ApplyCurrentInput)
    );

    HudUiRect rect0 = {1, 2, 3, 4};
    HudUiRect rect1 = {5, 6, 7, 8};
    HudUiRect sentinel = {9, 10, 11, 12};
    unsigned short pixels[12] = {};

    std::memset(FxPass3ConfigBytes(), 0, kFxPass3ConfigSize);
    FxPass3FieldAt<HudUiRect *>(kFxPass3InputRect0Offset) = &rect0;
    FxPass3FieldAt<HudUiRect *>(kFxPass3InputRect1Offset) = &rect1;
    FxPass3FieldAt<unsigned short *>(kFxPass3SurfacePixelsOffset) = pixels;
    FxPass3FieldAt<int>(kFxPass3SurfaceWidthOffset) = 4;
    FxPass3FieldAt<int>(kFxPass3SurfaceHeightOffset) = 3;
    FxPass3FieldAt<int>(kFxPass3SurfacePitchOffset) = 8;

    zVideoFxPass3Element element = {};
    element.base.ftable = reinterpret_cast<const HudUiCommon_FTable *>(&table);
    element.base.parent = &g_zVideo_FxPass3ConfigLocal;
    element.clipRectOrNull = &sentinel;

    ResetFxPass3DrawCapture();
    element.Draw();
    const bool parentConfigOk =
        g_fxPass3DrawBaseCount == 1 && g_fxPass3ApplyCount == 2 &&
        g_fxPass3ApplyRects[0] == &rect0 && g_fxPass3ApplyRects[1] == &rect1 &&
        element.clipRectOrNull == &rect0 && g_zVideo_FxSurfacePixels16 == pixels &&
        g_zVideo_FxSurfaceWidth == 4 && g_zVideo_FxSurfaceHeight == 3 &&
        g_zVideo_FxSurfacePitchBytes == 8 && g_zVideo_FxSurfacePitchPixels16 == 4;

    std::memset(FxPass3ConfigBytes(), 0, kFxPass3ConfigSize);
    FxPass3FieldAt<HudUiRect *>(kFxPass3InputRect1Offset) = &rect1;
    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxSurfaceWidth = oldFxWidth;
    g_zVideo_FxSurfaceHeight = oldFxHeight;
    g_zVideo_FxSurfacePitchBytes = oldFxPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldFxPitchPixels;
    element.base.parent = &g_zVideo_FxPass3ConfigLocal;
    element.clipRectOrNull = &sentinel;

    ResetFxPass3DrawCapture();
    element.Draw();
    const bool nullFirstInputOk =
        g_fxPass3DrawBaseCount == 1 && g_fxPass3ApplyCount == 1 &&
        g_fxPass3ApplyRects[0] == &rect1 && element.clipRectOrNull == nullptr &&
        g_zVideo_FxSurfacePixels16 == oldFxPixels && g_zVideo_FxSurfaceWidth == oldFxWidth &&
        g_zVideo_FxSurfaceHeight == oldFxHeight &&
        g_zVideo_FxSurfacePitchBytes == oldFxPitchBytes &&
        g_zVideo_FxSurfacePitchPixels16 == oldFxPitchPixels;

    element.base.parent = nullptr;
    element.clipRectOrNull = &sentinel;
    ResetFxPass3DrawCapture();
    element.Draw();
    const bool noParentOk = g_fxPass3DrawBaseCount == 1 && g_fxPass3ApplyCount == 1 &&
                            g_fxPass3ApplyRects[0] == &sentinel &&
                            element.clipRectOrNull == &sentinel;

    std::memcpy(FxPass3ConfigBytes(), savedConfig, sizeof(savedConfig));
    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxSurfaceWidth = oldFxWidth;
    g_zVideo_FxSurfaceHeight = oldFxHeight;
    g_zVideo_FxSurfacePitchBytes = oldFxPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldFxPitchPixels;

    return parentConfigOk && nullFirstInputOk && noParentOk ? 0 : 1;
}

extern "C" int zvideo_fxpass3_config_constructor_destructor_smoke(void) {
    zVideoFxPass3Config config;
    std::memset(&config, 0xcc, sizeof(config));

    zVideoFxPass3Config *const constructed = config.Constructor();
    const unsigned int *const rootTable =
        reinterpret_cast<const unsigned int *>(config.rootElement.base.ftable);
    const unsigned int *const slot0Table =
        reinterpret_cast<const unsigned int *>(config.slots[0].base.ftable);

    bool childChainOk = config.childHead == &config.rootElement.base &&
                        config.rootElement.base.parent == &config &&
                        config.rootElement.base.next == &config.slots[0].base &&
                        config.childTail == &config.slots[4].base;
    for (int i = 0; i < 5; ++i) {
        childChainOk = childChainOk && config.slots[i].base.parent == &config;
        if (i < 4) {
            childChainOk =
                childChainOk && config.slots[i].base.next == &config.slots[i + 1].base;
        } else {
            childChainOk = childChainOk && config.slots[i].base.next == nullptr;
        }
    }

    const bool constructorOk =
        constructed == &config && config.enabled == 1 && childChainOk &&
        config.inputRectsOrNull[0] == nullptr && config.inputRectsOrNull[1] == nullptr &&
        config.surfacePixels == nullptr && config.surfaceWidth == 0 &&
        config.surfaceHeight == 0 &&
        config.surfacePitchBytes == static_cast<int>(0xccccccccu) &&
        config.slotWriteIndex == 0 && config.rootElement.clipRectOrNull == nullptr &&
        config.slots[0].clipRectOrNull == nullptr &&
        config.rootElement.base.ftable != &g_HudUiCommon_FTable &&
        config.slots[0].base.ftable != &g_HudUiCommon_FTable &&
        rootTable[1] == MethodAddress(&zVideoFxPass3Element::Draw) &&
        rootTable[0x74 / 4] == MethodAddress(&zVideoFxPass3RootElement::ApplyOverlayRect) &&
        slot0Table[1] == MethodAddress(&zVideoFxPass3Element::Draw) &&
        slot0Table[0x74 / 4] == MethodAddress(&zVideoFxPass3Slot::ApplyToCurrentSurface);

    config.Destructor();
    bool destructorOk = config.rootElement.base.ftable == &g_HudUiCommon_FTable;
    for (int i = 0; i < 5; ++i) {
        destructorOk = destructorOk && config.slots[i].base.ftable == &g_HudUiCommon_FTable;
    }

    std::memset(&g_zVideo_FxPass3ConfigLocal, 0xcc, sizeof(g_zVideo_FxPass3ConfigLocal));
    zVideoFxPass3Config *const globalConstructed =
        zVideoFxPass3Config::ConstructGlobalSingleton();
    const bool globalConstructOk =
        globalConstructed == &g_zVideo_FxPass3ConfigLocal &&
        reinterpret_cast<HudUiContainer *>(&g_zVideo_FxPass3ConfigLocal)->enabled == 1;
    zVideoFxPass3Config::DestroyGlobalSingleton();
    const bool globalDestroyOk =
        reinterpret_cast<HudUiElement *>(
            FxPass3ConfigBytes() + kFxPass3RootElementOffset
        )->ftable == &g_HudUiCommon_FTable;
    std::memset(&g_zVideo_FxPass3ConfigLocal, 0, sizeof(g_zVideo_FxPass3ConfigLocal));

    return constructorOk && destructorOk && globalConstructOk && globalDestroyOk ? 0 : 1;
}

extern "C" int zvideo_fxpass3_slot_constructor_and_apply_smoke(void) {
    unsigned short *const oldFxPixels = g_zVideo_FxSurfacePixels16;
    unsigned short *const oldScratch = g_zVideo_FxPass3_ScratchPixels16;
    const int oldWidth = g_zVideo_FxSurfaceWidth;
    const int oldHeight = g_zVideo_FxSurfaceHeight;
    const int oldPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldPitchPixels = g_zVideo_FxSurfacePitchPixels16;
    const int oldOffsetX = g_zVideo_FxPass3_ScratchOffsetX;
    const int oldOffsetY = g_zVideo_FxPass3_ScratchOffsetY;
    const int oldClipMinX = g_zVideo_FxPass3_ClipMinX;
    const int oldClipMinY = g_zVideo_FxPass3_ClipMinY;
    const int oldClipMaxX = g_zVideo_FxPass3_ClipMaxX;
    const int oldClipMaxY = g_zVideo_FxPass3_ClipMaxY;

    zVideoFxPass3Slot slot;
    std::memset(&slot, 0xcc, sizeof(slot));
    zVideoFxPass3Slot *const constructed = slot.Constructor();
    const unsigned int *const ftableSlots =
        reinterpret_cast<const unsigned int *>(slot.base.ftable);
    int constructorFailures = 0;
    if (constructed != &slot) {
        constructorFailures |= 1;
    }
    if (slot.base.ftable == &g_HudUiCommon_FTable) {
        constructorFailures |= 2;
    }
    if (slot.base.next != nullptr || slot.base.parent != nullptr || slot.base.x != 0 ||
        slot.base.y != 0 || slot.clipRectOrNull != nullptr) {
        constructorFailures |= 4;
    }
    if (ftableSlots[1] != MethodAddress(&zVideoFxPass3Element::Draw)) {
        constructorFailures |= 8;
    }
    if (ftableSlots[2] != g_HudUiCommon_FTable.slots[2]) {
        constructorFailures |= 16;
    }
    if (ftableSlots[0x74 / 4] != MethodAddress(&zVideoFxPass3Slot::ApplyToCurrentSurface)) {
        constructorFailures |= 32;
    }

    unsigned short pixels[49];
    unsigned short original[49];
    unsigned short scratch[49];
    for (int i = 0; i < 49; ++i) {
        pixels[i] = static_cast<unsigned short>(0x2000 + i);
        original[i] = pixels[i];
        scratch[i] = 0xffff;
    }

    g_zVideo_FxSurfacePixels16 = pixels;
    g_zVideo_FxPass3_ScratchPixels16 = scratch;
    g_zVideo_FxSurfaceWidth = 7;
    g_zVideo_FxSurfaceHeight = 7;
    g_zVideo_FxSurfacePitchBytes = 14;
    g_zVideo_FxSurfacePitchPixels16 = 7;

    HudUiRect clip = {1, 1, 5, 5};
    slot.base.x = 3;
    slot.base.y = 3;
    slot.clipRectOrNull = &clip;
    slot.currentRadius = 1;
    slot.maxRadius = 2;
    slot.extent = 2;
    slot.sinFreq = 1.0f;
    slot.sinPhase = 0.0f;
    slot.ApplyToCurrentSurface();

    const bool callbackOk = pixels[2 + 2 * 7] == original[1 + 1 * 7] &&
                            pixels[0] == original[0] &&
                            g_zVideo_FxPass3_ScratchOffsetX == 3 &&
                            g_zVideo_FxPass3_ScratchOffsetY == 3 &&
                            g_zVideo_FxPass3_ClipMinX == 1 &&
                            g_zVideo_FxPass3_ClipMinY == 1 &&
                            g_zVideo_FxPass3_ClipMaxX == 5 &&
                            g_zVideo_FxPass3_ClipMaxY == 5;

    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxPass3_ScratchPixels16 = oldScratch;
    g_zVideo_FxSurfaceWidth = oldWidth;
    g_zVideo_FxSurfaceHeight = oldHeight;
    g_zVideo_FxSurfacePitchBytes = oldPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldPitchPixels;
    g_zVideo_FxPass3_ScratchOffsetX = oldOffsetX;
    g_zVideo_FxPass3_ScratchOffsetY = oldOffsetY;
    g_zVideo_FxPass3_ClipMinX = oldClipMinX;
    g_zVideo_FxPass3_ClipMinY = oldClipMinY;
    g_zVideo_FxPass3_ClipMaxX = oldClipMaxX;
    g_zVideo_FxPass3_ClipMaxY = oldClipMaxY;

    if (constructorFailures != 0) {
        return constructorFailures;
    }
    return callbackOk ? 0 : 2;
}

extern "C" int zvideo_fxpass3_root_overlay_smoke(void) {
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const int oldOverlayEnabled = zRndr::g_overlayBlendEnabled;
    const int oldOverlayLeft = zRndr::g_overlayBlendRectLeft;
    const int oldOverlayTop = zRndr::g_overlayBlendRectTop;
    const int oldOverlayRight = zRndr::g_overlayBlendRectRight;
    const int oldOverlayBottom = zRndr::g_overlayBlendRectBottom;
    const unsigned int oldOverlayColor = zRndr::g_overlayBlendPackedColor16;
    const double oldOverlayAlpha = zRndr::g_overlayBlendAlpha;
    const unsigned short *const oldFxPixels = g_zVideo_FxSurfacePixels16;
    const int oldFxWidth = g_zVideo_FxSurfaceWidth;
    const int oldFxHeight = g_zVideo_FxSurfaceHeight;
    const int oldFxPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldFxPitchPixels = g_zVideo_FxSurfacePitchPixels16;

    zVideoFxPass3RootElement root = {};
    HudUiRect rect = {2, 3, 8, 9};
    root.clipRectOrNull = &rect;
    root.packedColor16 = 0xabcd;
    root.alpha = 0.375;

    g_zVideo_ActiveRendererPath = 0;
    zRndr::g_overlayBlendEnabled = 0;
    root.ApplyOverlayRect();
    const bool explicitRectOk =
        zRndr::g_overlayBlendEnabled == 1 && zRndr::g_overlayBlendRectLeft == 2 &&
        zRndr::g_overlayBlendRectTop == 3 && zRndr::g_overlayBlendRectRight == 8 &&
        zRndr::g_overlayBlendRectBottom == 9 &&
        zRndr::g_overlayBlendPackedColor16 == 0xabcd && zRndr::g_overlayBlendAlpha == 0.375;

    zRndr::g_overlayBlendEnabled = 0;
    root.clipRectOrNull = nullptr;
    root.packedColor16 = 0xf81f;
    root.alpha = 0.25;
    g_zVideo_FxSurfaceWidth = 13;
    g_zVideo_FxSurfaceHeight = 17;
    root.ApplyOverlayRect();
    const bool fallbackRectOk =
        zRndr::g_overlayBlendEnabled == 1 && zRndr::g_overlayBlendRectLeft == 0 &&
        zRndr::g_overlayBlendRectTop == 0 && zRndr::g_overlayBlendRectRight == 12 &&
        zRndr::g_overlayBlendRectBottom == 17 &&
        zRndr::g_overlayBlendPackedColor16 == 0xf81f && zRndr::g_overlayBlendAlpha == 0.25;

    g_zVideo_ActiveRendererPath = oldRendererPath;
    zRndr::g_overlayBlendEnabled = oldOverlayEnabled;
    zRndr::g_overlayBlendRectLeft = oldOverlayLeft;
    zRndr::g_overlayBlendRectTop = oldOverlayTop;
    zRndr::g_overlayBlendRectRight = oldOverlayRight;
    zRndr::g_overlayBlendRectBottom = oldOverlayBottom;
    zRndr::g_overlayBlendPackedColor16 = oldOverlayColor;
    zRndr::g_overlayBlendAlpha = oldOverlayAlpha;
    g_zVideo_FxSurfacePixels16 = (unsigned short *)(oldFxPixels);
    g_zVideo_FxSurfaceWidth = oldFxWidth;
    g_zVideo_FxSurfaceHeight = oldFxHeight;
    g_zVideo_FxSurfacePitchBytes = oldFxPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldFxPitchPixels;

    return explicitRectOk && fallbackRectOk ? 0 : 1;
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

extern "C" int zvideo_run_postprocess_on_primary_buffer_smoke(void) {
    zVideo_SurfaceStateProc const savedLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    zVideo_SurfaceStateProc const savedUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;
    const zVideo_SurfaceStatePartial savedPrimarySurfaceState = g_zVideo_PrimarySurfaceState;
    const int savedRendererType = g_zVideo_RendererType;
    const int savedUseHalfResBackbuffer = g_zVideo_UseHalfResBackbuffer;
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
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_PrimarySurfaceState.pixels = pixels;
    g_zVideo_PrimarySurfaceState.width = 4;
    g_zVideo_PrimarySurfaceState.height = 3;
    g_zVideo_PrimarySurfaceState.pitch = 8;
    g_zVideo_pfnLockSurfaceState = CaptureLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = CaptureUnlockSurfaceState;

    auto resetObservedState = [&]() {
        g_zVideoTestLockSurfaceCount = 0;
        g_zVideoTestLockSurfaceState = nullptr;
        g_zVideoTestUnlockSurfaceCount = 0;
        g_zVideoTestUnlockSurfaceState = nullptr;
        zRndr::g_frameBuffer = nullptr;
        zRndr::g_pitchBytes = 0;
        zRndr::g_bytesPerPixel = 7;
        std::memset(FxPass3ConfigBytes(), 0, kFxPass3ConfigSize);
        g_zVideo_FxSurfacePixels16 = nullptr;
        g_zVideo_FxSurfaceWidth = 0;
        g_zVideo_FxSurfaceHeight = 0;
        g_zVideo_FxSurfacePitchBytes = 0;
        g_zVideo_FxSurfacePitchPixels16 = 0;
    };

    auto pipelineOk = [&]() {
        return zRndr::g_frameBuffer == pixels && zRndr::g_pitchBytes == 8 &&
               zRndr::g_bytesPerPixel == 7 && g_zVideo_FxSurfacePixels16 == pixels &&
               g_zVideo_FxSurfaceWidth == 4 && g_zVideo_FxSurfaceHeight == 3 &&
               g_zVideo_FxSurfacePitchBytes == 8 && g_zVideo_FxSurfacePitchPixels16 == 4 &&
               FxPass3FieldAt<unsigned short *>(kFxPass3SurfacePixelsOffset) == pixels &&
               FxPass3FieldAt<int>(kFxPass3SurfaceWidthOffset) == 4 &&
               FxPass3FieldAt<int>(kFxPass3SurfaceHeightOffset) == 3 &&
               FxPass3FieldAt<int>(kFxPass3SurfacePitchOffset) == 8;
    };

    resetObservedState();
    g_zVideo_RendererType = 0;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_PrimarySurfaceState.locked = 0;
    const int softwareResult = zVideo::RunPostprocessOnPrimaryBuffer();
    const bool softwareOk = softwareResult == 0 && g_zVideoTestLockSurfaceCount == 0 &&
                            g_zVideoTestUnlockSurfaceCount == 0 &&
                            g_zVideo_PrimarySurfaceState.locked == 0 && pipelineOk();

    resetObservedState();
    g_zVideo_RendererType = 1;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_PrimarySurfaceState.locked = 0;
    const int hardwareResult = zVideo::RunPostprocessOnPrimaryBuffer();
    const bool hardwareOk = hardwareResult == 0 && g_zVideoTestLockSurfaceCount == 1 &&
                            g_zVideoTestLockSurfaceState == &g_zVideo_PrimarySurfaceState &&
                            g_zVideoTestUnlockSurfaceCount == 0 &&
                            g_zVideo_PrimarySurfaceState.locked == 1 && pipelineOk();

    resetObservedState();
    g_zVideo_RendererType = 0;
    g_zVideo_UseHalfResBackbuffer = 1;
    g_zVideo_PrimarySurfaceState.locked = 0;
    const int halfResResult = zVideo::RunPostprocessOnPrimaryBuffer();
    const bool halfResOk = halfResResult == 0 && g_zVideoTestLockSurfaceCount == 1 &&
                           g_zVideoTestLockSurfaceState == &g_zVideo_PrimarySurfaceState &&
                           g_zVideoTestUnlockSurfaceCount == 1 &&
                           g_zVideoTestUnlockSurfaceState == &g_zVideo_PrimarySurfaceState &&
                           g_zVideo_PrimarySurfaceState.locked == 0 && pipelineOk();

    g_zVideo_pfnLockSurfaceState = savedLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = savedUnlockSurfaceState;
    g_zVideo_PrimarySurfaceState = savedPrimarySurfaceState;
    g_zVideo_RendererType = savedRendererType;
    g_zVideo_UseHalfResBackbuffer = savedUseHalfResBackbuffer;
    zRndr::g_frameBuffer = savedFrameBuffer;
    zRndr::g_pitchBytes = savedPitchBytes;
    zRndr::g_bytesPerPixel = savedBytesPerPixel;
    std::memcpy(FxPass3ConfigBytes(), savedConfig, sizeof(savedConfig));
    g_zVideo_FxSurfacePixels16 = savedFxSurfacePixels;
    g_zVideo_FxSurfaceWidth = savedFxSurfaceWidth;
    g_zVideo_FxSurfaceHeight = savedFxSurfaceHeight;
    g_zVideo_FxSurfacePitchBytes = savedFxSurfacePitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = savedFxSurfacePitchPixels;

    return softwareOk && hardwareOk && halfResOk ? 0 : 1;
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

extern "C" int zvideo_noise_shutdown_buffers_smoke(void) {
    unsigned char *const savedNoiseTable = g_zVid_NoiseByteTable;
    unsigned short *const savedScratchPixels = g_zVideo_FxPass3_ScratchPixels16;

    unsigned char *const noiseTable = (unsigned char *)(std::malloc(8));
    unsigned short *const scratchPixels = (unsigned short *)(std::malloc(8));
    if (noiseTable == nullptr || scratchPixels == nullptr) {
        std::free(noiseTable);
        std::free(scratchPixels);
        return 1;
    }

    noiseTable[0] = 0x5a;
    scratchPixels[0] = 0xa55a;
    g_zVid_NoiseByteTable = noiseTable;
    g_zVideo_FxPass3_ScratchPixels16 = scratchPixels;
    zVid::Noise_ShutdownBuffers();
    const bool allocatedShutdownOk =
        g_zVid_NoiseByteTable == nullptr &&
        g_zVideo_FxPass3_ScratchPixels16 == nullptr;

    zVid::Noise_ShutdownBuffers();
    const bool nullShutdownOk =
        g_zVid_NoiseByteTable == nullptr &&
        g_zVideo_FxPass3_ScratchPixels16 == nullptr;

    g_zVid_NoiseByteTable = savedNoiseTable;
    g_zVideo_FxPass3_ScratchPixels16 = savedScratchPixels;
    return allocatedShutdownOk && nullShutdownOk ? 0 : 2;
}

extern "C" int zvideo_fxpass3_copy_surface_pixel_clipped_smoke(void) {
    unsigned short *const oldFxPixels = g_zVideo_FxSurfacePixels16;
    unsigned short *const oldScratch = g_zVideo_FxPass3_ScratchPixels16;
    const int oldWidth = g_zVideo_FxSurfaceWidth;
    const int oldHeight = g_zVideo_FxSurfaceHeight;
    const int oldPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldPitchPixels = g_zVideo_FxSurfacePitchPixels16;
    const int oldOffsetX = g_zVideo_FxPass3_ScratchOffsetX;
    const int oldOffsetY = g_zVideo_FxPass3_ScratchOffsetY;
    const int oldClipMinX = g_zVideo_FxPass3_ClipMinX;
    const int oldClipMinY = g_zVideo_FxPass3_ClipMinY;
    const int oldClipMaxX = g_zVideo_FxPass3_ClipMaxX;
    const int oldClipMaxY = g_zVideo_FxPass3_ClipMaxY;

    unsigned short pixels[25] = {};
    unsigned short scratch[25] = {};
    for (int i = 0; i < 25; ++i) {
        pixels[i] = (unsigned short)(0x1000 + i);
        scratch[i] = 0xffff;
    }

    g_zVideo_FxSurfacePixels16 = pixels;
    g_zVideo_FxPass3_ScratchPixels16 = scratch;
    g_zVideo_FxSurfaceWidth = 5;
    g_zVideo_FxSurfaceHeight = 5;
    g_zVideo_FxSurfacePitchBytes = 10;
    g_zVideo_FxSurfacePitchPixels16 = 5;
    g_zVideo_FxPass3_ScratchOffsetX = 2;
    g_zVideo_FxPass3_ScratchOffsetY = 2;
    g_zVideo_FxPass3_ClipMinX = 1;
    g_zVideo_FxPass3_ClipMinY = 1;
    g_zVideo_FxPass3_ClipMaxX = 4;
    g_zVideo_FxPass3_ClipMaxY = 4;

    zVideo::FxPass3_CopySurfacePixelToScratchClipped(
        0,
        0,
        1,
        1
    );
    const bool copiedOk = scratch[2 + 2 * 5] == pixels[3 + 3 * 5];

    zVideo::FxPass3_CopySurfacePixelToScratchClipped(
        2,
        0,
        0,
        0
    );
    const bool dstClipOk = scratch[4 + 2 * 5] == 0xffff;

    zVideo::FxPass3_CopySurfacePixelToScratchClipped(
        0,
        0,
        -2,
        0
    );
    const bool srcClipOk = scratch[2 + 2 * 5] == pixels[3 + 3 * 5];

    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxPass3_ScratchPixels16 = oldScratch;
    g_zVideo_FxSurfaceWidth = oldWidth;
    g_zVideo_FxSurfaceHeight = oldHeight;
    g_zVideo_FxSurfacePitchBytes = oldPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldPitchPixels;
    g_zVideo_FxPass3_ScratchOffsetX = oldOffsetX;
    g_zVideo_FxPass3_ScratchOffsetY = oldOffsetY;
    g_zVideo_FxPass3_ClipMinX = oldClipMinX;
    g_zVideo_FxPass3_ClipMinY = oldClipMinY;
    g_zVideo_FxPass3_ClipMaxX = oldClipMaxX;
    g_zVideo_FxPass3_ClipMaxY = oldClipMaxY;

    return copiedOk && dstClipOk && srcClipOk ? 0 : 1;
}

extern "C" int zvideo_fxpass3_apply_to_current_surface_smoke(void) {
    unsigned short *const oldFxPixels = g_zVideo_FxSurfacePixels16;
    unsigned short *const oldScratch = g_zVideo_FxPass3_ScratchPixels16;
    const int oldWidth = g_zVideo_FxSurfaceWidth;
    const int oldHeight = g_zVideo_FxSurfaceHeight;
    const int oldPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldPitchPixels = g_zVideo_FxSurfacePitchPixels16;
    const int oldOffsetX = g_zVideo_FxPass3_ScratchOffsetX;
    const int oldOffsetY = g_zVideo_FxPass3_ScratchOffsetY;
    const int oldClipMinX = g_zVideo_FxPass3_ClipMinX;
    const int oldClipMinY = g_zVideo_FxPass3_ClipMinY;
    const int oldClipMaxX = g_zVideo_FxPass3_ClipMaxX;
    const int oldClipMaxY = g_zVideo_FxPass3_ClipMaxY;

    unsigned short pixels[49];
    unsigned short original[49];
    unsigned short scratch[49];
    for (int i = 0; i < 49; ++i) {
        pixels[i] = static_cast<unsigned short>(0x1000 + i);
        original[i] = pixels[i];
        scratch[i] = 0xffff;
    }

    g_zVideo_FxSurfacePixels16 = pixels;
    g_zVideo_FxPass3_ScratchPixels16 = scratch;
    g_zVideo_FxSurfaceWidth = 7;
    g_zVideo_FxSurfaceHeight = 7;
    g_zVideo_FxSurfacePitchBytes = 14;
    g_zVideo_FxSurfacePitchPixels16 = 7;

    zVideo::FxPass3_ApplyToCurrentSurface(
        3,
        3,
        2,
        2,
        2,
        1.0f,
        0.0f,
        nullptr
    );
    bool cappedNoopOk = true;
    for (int i = 0; i < 49; ++i) {
        cappedNoopOk = cappedNoopOk && pixels[i] == original[i];
    }

    for (int i = 0; i < 49; ++i) {
        pixels[i] = original[i];
        scratch[i] = 0xffff;
    }
    zVideo::FxPass3_ApplyToCurrentSurface(
        3,
        3,
        1,
        2,
        2,
        1.0f,
        0.0f,
        nullptr
    );
    const bool fullWarpOk = pixels[2 + 2 * 7] == original[1 + 1 * 7] &&
                            pixels[3 + 3 * 7] == original[3 + 3 * 7];

    for (int i = 0; i < 49; ++i) {
        pixels[i] = original[i];
        scratch[i] = 0xffff;
    }
    zVidRect32 clip{1, 1, 5, 5};
    zVideo::FxPass3_ApplyToCurrentSurface(
        3,
        3,
        1,
        2,
        2,
        1.0f,
        0.0f,
        &clip
    );
    const bool clippedWarpOk = pixels[2 + 2 * 7] == original[1 + 1 * 7] &&
                               pixels[0 + 0 * 7] == original[0 + 0 * 7] &&
                               g_zVideo_FxPass3_ScratchOffsetX == 3 &&
                               g_zVideo_FxPass3_ScratchOffsetY == 3;

    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxPass3_ScratchPixels16 = oldScratch;
    g_zVideo_FxSurfaceWidth = oldWidth;
    g_zVideo_FxSurfaceHeight = oldHeight;
    g_zVideo_FxSurfacePitchBytes = oldPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldPitchPixels;
    g_zVideo_FxPass3_ScratchOffsetX = oldOffsetX;
    g_zVideo_FxPass3_ScratchOffsetY = oldOffsetY;
    g_zVideo_FxPass3_ClipMinX = oldClipMinX;
    g_zVideo_FxPass3_ClipMinY = oldClipMinY;
    g_zVideo_FxPass3_ClipMaxX = oldClipMaxX;
    g_zVideo_FxPass3_ClipMaxY = oldClipMaxY;

    return cappedNoopOk && fullWarpOk && clippedWarpOk ? 0 : 1;
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

extern "C" int zvideo_fx_surface_alpha_line_smoke(void) {
    unsigned short *const oldFxPixels = g_zVideo_FxSurfacePixels16;
    const int oldFxWidth = g_zVideo_FxSurfaceWidth;
    const int oldFxHeight = g_zVideo_FxSurfaceHeight;
    const int oldFxPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldFxPitchPixels16 = g_zVideo_FxSurfacePitchPixels16;
    const int oldPitchBytes = zRndr::g_pitchBytes;
    const int oldGreenBits = zRndr::g_pixelPackGreenBits;

    unsigned short pixels[36] = {};
    for (int i = 0; i < 36; ++i) {
        pixels[i] = 0x001f;
    }

    g_zVideo_FxSurfacePixels16 = pixels;
    g_zVideo_FxSurfaceWidth = 6;
    g_zVideo_FxSurfaceHeight = 6;
    g_zVideo_FxSurfacePitchBytes = 12;
    g_zVideo_FxSurfacePitchPixels16 = 6;
    zRndr::g_pitchBytes = 12;
    zRndr::g_pixelPackGreenBits = 6;

    zVidRect32 clip{0, 0, 5, 5};
    zVideo_FxSurface::DrawAlphaBlendedLine(&clip, 4, 1, 1, 1, 0xf800, 1.0f, 1.0f, 1);
    const bool horizontalOk = pixels[1 + 1 * 6] == 0xf800 && pixels[2 + 1 * 6] == 0xf800 &&
                              pixels[3 + 1 * 6] == 0xf800 && pixels[4 + 1 * 6] == 0xf800;

    for (int i = 0; i < 36; ++i) {
        pixels[i] = 0x07e0;
    }
    zVideo_FxSurface::DrawAlphaBlendedLine(&clip, 4, 2, 1, 2, 0xf800, 0.0f, 0.0f, 1);
    bool lowAlphaOk = true;
    for (int x = 1; x <= 4; ++x) {
        lowAlphaOk = lowAlphaOk && pixels[x + 2 * 6] == 0x07e0;
    }

    for (int i = 0; i < 36; ++i) {
        pixels[i] = 0x0000;
    }
    zVideo_FxSurface::DrawAlphaBlendedLine(&clip, 3, 3, 1, 3, 0xf800, 0.5f, 0.5f, 1);
    const unsigned short blended565 = pixels[1 + 3 * 6];
    const bool blendOk = blended565 != 0x0000 && blended565 != 0xf800 &&
                         pixels[2 + 3 * 6] == blended565 && pixels[3 + 3 * 6] == blended565;

    for (int i = 0; i < 36; ++i) {
        pixels[i] = 0xaaaa;
    }
    zVidRect32 clipped{1, 0, 5, 5};
    zVideo_FxSurface::DrawAlphaBlendedLine(&clipped, 5, 2, 0, 2, 0xf800, 1.0f, 1.0f, 1);
    const bool clipOk = pixels[1 + 2 * 6] == 0xaaaa && pixels[2 + 2 * 6] == 0xf800 &&
                        pixels[3 + 2 * 6] == 0xf800 && pixels[4 + 2 * 6] == 0xf800 &&
                        pixels[5 + 2 * 6] == 0xaaaa;

    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxSurfaceWidth = oldFxWidth;
    g_zVideo_FxSurfaceHeight = oldFxHeight;
    g_zVideo_FxSurfacePitchBytes = oldFxPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldFxPitchPixels16;
    zRndr::g_pitchBytes = oldPitchBytes;
    zRndr::g_pixelPackGreenBits = oldGreenBits;

    return horizontalOk && lowAlphaOk && blendOk && clipOk ? 0 : 1;
}

extern "C" int zvideo_fx_surface_apply_blue_tint_rect_smoke(void) {
    unsigned short *const oldFxPixels = g_zVideo_FxSurfacePixels16;
    const int oldFxWidth = g_zVideo_FxSurfaceWidth;
    const int oldFxHeight = g_zVideo_FxSurfaceHeight;
    const int oldFxPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldFxPitchPixels16 = g_zVideo_FxSurfacePitchPixels16;
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const int oldQuadBatchCount = g_zVideo_QuadBatchCount;
    zVideo_QuadBatchItemPartial oldQuadBatchItems[16];
    std::memcpy(oldQuadBatchItems, g_zVideo_QuadBatchItemsBase, sizeof(oldQuadBatchItems));

    unsigned short pixels[24] = {};
    for (int i = 0; i < 24; ++i) {
        pixels[i] = 0xffff;
    }

    g_zVideo_FxSurfacePixels16 = pixels;
    g_zVideo_FxSurfaceWidth = 6;
    g_zVideo_FxSurfaceHeight = 4;
    g_zVideo_FxSurfacePitchBytes = 12;
    g_zVideo_FxSurfacePitchPixels16 = 6;
    g_zVideo_ActiveRendererPath = 0;
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);

    zVidRect32 rect{1, 1, 5, 3};
    zVideo_FxSurface::ApplyBlueTintRect(&rect);

    const bool softwareRegionOk =
        pixels[1 + 1 * 6] == 0x7bff && pixels[2 + 1 * 6] == 0x7bff &&
        pixels[3 + 1 * 6] == 0x7bff && pixels[4 + 1 * 6] == 0x7bff &&
        pixels[1 + 2 * 6] == 0x7bff && pixels[2 + 2 * 6] == 0x7bff &&
        pixels[3 + 2 * 6] == 0x7bff && pixels[4 + 2 * 6] == 0x7bff;
    const bool softwareEdgesOk =
        pixels[0 + 1 * 6] == 0xffff && pixels[5 + 1 * 6] == 0xffff &&
        pixels[0 + 2 * 6] == 0xffff && pixels[5 + 2 * 6] == 0xffff &&
        pixels[1 + 3 * 6] == 0xffff;

    std::memset(g_zVideo_QuadBatchItemsBase, 0, sizeof(g_zVideo_QuadBatchItemsBase));
    g_zVideo_QuadBatchCount = 0;
    g_zVideo_ActiveRendererPath = 1;
    zVidRect32 hwRect{2, 3, 4, 5};
    zVideo_FxSurface::ApplyBlueTintRect(&hwRect);
    const zVideo_QuadBatchItemPartial &quad = g_zVideo_QuadBatchItemsBase[0];
    const bool hardwareOk =
        g_zVideo_QuadBatchCount == 1 &&
        quad.vertices[0].sx == 2.0f && quad.vertices[0].sy == 3.0f &&
        quad.vertices[1].sx == 4.0f && quad.vertices[1].sy == 3.0f &&
        quad.vertices[2].sx == 4.0f && quad.vertices[2].sy == 5.0f &&
        quad.vertices[3].sx == 2.0f && quad.vertices[3].sy == 5.0f &&
        quad.vertices[0].color == 0x4c0000f8 && quad.vertices[3].color == 0x4c0000f8;

    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxSurfaceWidth = oldFxWidth;
    g_zVideo_FxSurfaceHeight = oldFxHeight;
    g_zVideo_FxSurfacePitchBytes = oldFxPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldFxPitchPixels16;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVideo_QuadBatchCount = oldQuadBatchCount;
    std::memcpy(g_zVideo_QuadBatchItemsBase, oldQuadBatchItems, sizeof(oldQuadBatchItems));

    if (!softwareRegionOk) {
        return 1;
    }
    if (!softwareEdgesOk) {
        return 2;
    }
    if (!hardwareOk) {
        return 3;
    }
    return 0;
}

extern "C" int zvideo_fx_surface_apply_green_mask_rect_smoke(void) {
    unsigned short *const oldFxPixels = g_zVideo_FxSurfacePixels16;
    const int oldFxWidth = g_zVideo_FxSurfaceWidth;
    const int oldFxHeight = g_zVideo_FxSurfaceHeight;
    const int oldFxPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldFxPitchPixels16 = g_zVideo_FxSurfacePitchPixels16;
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const int oldQuadBatchCount = g_zVideo_QuadBatchCount;
    zVideo_QuadBatchItemPartial oldQuadBatchItems[16];
    std::memcpy(oldQuadBatchItems, g_zVideo_QuadBatchItemsBase, sizeof(oldQuadBatchItems));

    unsigned short pixels[24] = {};
    for (int i = 0; i < 24; ++i) {
        pixels[i] = 0xffff;
    }

    g_zVideo_FxSurfacePixels16 = pixels;
    g_zVideo_FxSurfaceWidth = 6;
    g_zVideo_FxSurfaceHeight = 4;
    g_zVideo_FxSurfacePitchBytes = 12;
    g_zVideo_FxSurfacePitchPixels16 = 6;
    g_zVideo_ActiveRendererPath = 0;
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);

    zVidRect32 rect{1, 1, 5, 3};
    zVideo_FxSurface::ApplyGreenMaskRect(&rect);

    const bool softwareRegionOk =
        pixels[1 + 1 * 6] == 0x07e0 && pixels[2 + 1 * 6] == 0x07e0 &&
        pixels[3 + 1 * 6] == 0x07e0 && pixels[4 + 1 * 6] == 0x07e0 &&
        pixels[1 + 2 * 6] == 0x07e0 && pixels[2 + 2 * 6] == 0x07e0 &&
        pixels[3 + 2 * 6] == 0x07e0 && pixels[4 + 2 * 6] == 0x07e0;
    const bool softwareEdgesOk =
        pixels[0 + 1 * 6] == 0xffff && pixels[5 + 1 * 6] == 0xffff &&
        pixels[0 + 2 * 6] == 0xffff && pixels[5 + 2 * 6] == 0xffff &&
        pixels[1 + 3 * 6] == 0xffff;

    std::memset(g_zVideo_QuadBatchItemsBase, 0, sizeof(g_zVideo_QuadBatchItemsBase));
    g_zVideo_QuadBatchCount = 0;
    g_zVideo_ActiveRendererPath = 1;
    zVidRect32 hwRect{2, 3, 4, 5};
    zVideo_FxSurface::ApplyGreenMaskRect(&hwRect);
    const zVideo_QuadBatchItemPartial &quad = g_zVideo_QuadBatchItemsBase[0];
    const bool hardwareOk =
        g_zVideo_QuadBatchCount == 1 &&
        quad.vertices[0].sx == 2.0f && quad.vertices[0].sy == 3.0f &&
        quad.vertices[1].sx == 4.0f && quad.vertices[1].sy == 3.0f &&
        quad.vertices[2].sx == 4.0f && quad.vertices[2].sy == 5.0f &&
        quad.vertices[3].sx == 2.0f && quad.vertices[3].sy == 5.0f &&
        quad.vertices[0].color == 0x4c00fc00 && quad.vertices[3].color == 0x4c00fc00;

    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxSurfaceWidth = oldFxWidth;
    g_zVideo_FxSurfaceHeight = oldFxHeight;
    g_zVideo_FxSurfacePitchBytes = oldFxPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldFxPitchPixels16;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVideo_QuadBatchCount = oldQuadBatchCount;
    std::memcpy(g_zVideo_QuadBatchItemsBase, oldQuadBatchItems, sizeof(oldQuadBatchItems));

    if (!softwareRegionOk) {
        return 1;
    }
    if (!softwareEdgesOk) {
        return 2;
    }
    if (!hardwareOk) {
        return 3;
    }
    return 0;
}

extern "C" int zvideo_fx_surface_colored_lines_batch_smoke(void) {
    unsigned short *const oldFxPixels = g_zVideo_FxSurfacePixels16;
    const int oldFxWidth = g_zVideo_FxSurfaceWidth;
    const int oldFxHeight = g_zVideo_FxSurfaceHeight;
    const int oldFxPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldFxPitchPixels16 = g_zVideo_FxSurfacePitchPixels16;
    const int oldPitchBytes = zRndr::g_pitchBytes;
    const int oldGreenBits = zRndr::g_pixelPackGreenBits;

    unsigned short pixels[25] = {};
    for (int i = 0; i < 25; ++i) {
        pixels[i] = 0x0000;
    }

    g_zVideo_FxSurfacePixels16 = pixels;
    g_zVideo_FxSurfaceWidth = 5;
    g_zVideo_FxSurfaceHeight = 5;
    g_zVideo_FxSurfacePitchBytes = 10;
    g_zVideo_FxSurfacePitchPixels16 = 5;
    zRndr::g_pitchBytes = 10;
    zRndr::g_pixelPackGreenBits = 6;

    zVideoFxColoredLineRecord lines[2] = {};
    lines[0].x = 1;
    lines[0].y = 1;
    lines[0].width = 2;
    lines[0].height = 0;
    lines[0].color16 = 0xf800;
    lines[0].alphaEnd = 1.0f;
    lines[0].alphaStart = 1.0f;
    lines[0].clipInset = 1;
    lines[1].x = 0;
    lines[1].y = 3;
    lines[1].width = 4;
    lines[1].height = 0;
    lines[1].color16 = 0x07e0;
    lines[1].alphaEnd = 1.0f;
    lines[1].alphaStart = 1.0f;
    lines[1].clipInset = 1;

    zVidRect32 clip{-2, -1, 5, 6};
    zVideo_FxSurface::DrawColoredLinesBatch(lines, 2, &clip);
    const bool firstLineOk = pixels[1 + 1 * 5] == 0xf800 &&
                             pixels[2 + 1 * 5] == 0xf800 &&
                             pixels[3 + 1 * 5] == 0xf800;
    const bool secondLineClippedOk = pixels[0 + 3 * 5] == 0x0000 &&
                                     pixels[1 + 3 * 5] == 0x07e0 &&
                                     pixels[2 + 3 * 5] == 0x07e0 &&
                                     pixels[3 + 3 * 5] == 0x07e0 &&
                                     pixels[4 + 3 * 5] == 0x0000;

    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxSurfaceWidth = oldFxWidth;
    g_zVideo_FxSurfaceHeight = oldFxHeight;
    g_zVideo_FxSurfacePitchBytes = oldFxPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldFxPitchPixels16;
    zRndr::g_pitchBytes = oldPitchBytes;
    zRndr::g_pixelPackGreenBits = oldGreenBits;

    return firstLineOk && secondLineClippedOk ? 0 : 1;
}

namespace {
unsigned short BlurTestAverage(unsigned short before, unsigned short center,
                               unsigned short after) {
    const unsigned int rbMask = 0xf81f;
    const unsigned int greenMask = 0x07e0;
    const unsigned int rb =
        (before & rbMask) + ((center & rbMask) << 1) + (after & rbMask);
    const unsigned int green =
        (before & greenMask) + ((center & greenMask) << 1) + (after & greenMask);
    return static_cast<unsigned short>(((rb >> 2) & rbMask) |
                                       ((green >> 2) & greenMask));
}

void SaveBlurGlobals(unsigned short **oldFxPixels, unsigned short **oldScratch,
                     int *oldWidth, int *oldHeight, int *oldPitchBytes,
                     int *oldPitchPixels) {
    *oldFxPixels = g_zVideo_FxSurfacePixels16;
    *oldScratch = g_zVideo_FxPass3_ScratchPixels16;
    *oldWidth = g_zVideo_FxSurfaceWidth;
    *oldHeight = g_zVideo_FxSurfaceHeight;
    *oldPitchBytes = g_zVideo_FxSurfacePitchBytes;
    *oldPitchPixels = g_zVideo_FxSurfacePitchPixels16;
}

void RestoreBlurGlobals(unsigned short *oldFxPixels, unsigned short *oldScratch,
                        int oldWidth, int oldHeight, int oldPitchBytes,
                        int oldPitchPixels) {
    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxPass3_ScratchPixels16 = oldScratch;
    g_zVideo_FxSurfaceWidth = oldWidth;
    g_zVideo_FxSurfaceHeight = oldHeight;
    g_zVideo_FxSurfacePitchBytes = oldPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldPitchPixels;
}

void SetupBlurSurface(unsigned short *pixels, unsigned short *scratch, int width,
                      int height) {
    g_zVideo_FxSurfacePixels16 = pixels;
    g_zVideo_FxPass3_ScratchPixels16 = scratch;
    g_zVideo_FxSurfaceWidth = width;
    g_zVideo_FxSurfaceHeight = height;
    g_zVideo_FxSurfacePitchBytes = width * 2;
    g_zVideo_FxSurfacePitchPixels16 = width;
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);
}
} // namespace

extern "C" int zvideo_blur_region_horizontal_smoke(void) {
    unsigned short *oldFxPixels;
    unsigned short *oldScratch;
    int oldWidth;
    int oldHeight;
    int oldPitchBytes;
    int oldPitchPixels;
    SaveBlurGlobals(&oldFxPixels, &oldScratch, &oldWidth, &oldHeight, &oldPitchBytes,
                    &oldPitchPixels);

    unsigned short pixels[20];
    unsigned short original[20];
    unsigned short scratch[20] = {};
    for (int i = 0; i < 20; ++i) {
        pixels[i] = static_cast<unsigned short>(i);
        original[i] = pixels[i];
    }

    SetupBlurSurface(pixels, scratch, 5, 4);
    zVidRect32 rect{1, 1, 4, 2};
    zVideo::buff_BlurRegionHorizontal(&rect, 1);

    bool ok = true;
    for (int i = 0; i < 20; ++i) {
        unsigned short expected = original[i];
        const int x = i % 5;
        const int y = i / 5;
        if (y >= 1 && y <= 2 && x >= 1 && x < 4) {
            expected = BlurTestAverage(original[i - 1], original[i], original[i + 1]);
        }
        ok = ok && pixels[i] == expected;
    }

    RestoreBlurGlobals(oldFxPixels, oldScratch, oldWidth, oldHeight, oldPitchBytes,
                       oldPitchPixels);
    return ok ? 0 : 1;
}

extern "C" int zvideo_blur_region_vertical_smoke(void) {
    unsigned short *oldFxPixels;
    unsigned short *oldScratch;
    int oldWidth;
    int oldHeight;
    int oldPitchBytes;
    int oldPitchPixels;
    SaveBlurGlobals(&oldFxPixels, &oldScratch, &oldWidth, &oldHeight, &oldPitchBytes,
                    &oldPitchPixels);

    unsigned short pixels[25];
    unsigned short original[25];
    unsigned short scratch[25] = {};
    for (int i = 0; i < 25; ++i) {
        pixels[i] = static_cast<unsigned short>(i);
        original[i] = pixels[i];
    }

    SetupBlurSurface(pixels, scratch, 5, 5);
    zVidRect32 rect{0, 1, 4, 3};
    zVideo::buff_BlurRegionVertical(&rect, 2);

    bool ok = true;
    for (int i = 0; i < 25; ++i) {
        unsigned short expected = original[i];
        const int y = i / 5;
        if (y >= 1 && y < 3) {
            expected = BlurTestAverage(original[i - 5], original[i], original[i + 5]);
        }
        ok = ok && pixels[i] == expected;
    }

    RestoreBlurGlobals(oldFxPixels, oldScratch, oldWidth, oldHeight, oldPitchBytes,
                       oldPitchPixels);
    return ok ? 0 : 1;
}

extern "C" int zvideo_blur_region_combined_smoke(void) {
    unsigned short *oldFxPixels;
    unsigned short *oldScratch;
    int oldWidth;
    int oldHeight;
    int oldPitchBytes;
    int oldPitchPixels;
    SaveBlurGlobals(&oldFxPixels, &oldScratch, &oldWidth, &oldHeight, &oldPitchBytes,
                    &oldPitchPixels);

    unsigned short pixels[25];
    unsigned short expectedScratch[25];
    unsigned short expected[25];
    unsigned short scratch[25] = {};
    for (int i = 0; i < 25; ++i) {
        pixels[i] = static_cast<unsigned short>((i * 3) & 0xffff);
        expected[i] = pixels[i];
        expectedScratch[i] = pixels[i];
    }

    SetupBlurSurface(pixels, scratch, 5, 5);
    zVideo::buff_BlurRegionCombined(0, 3);

    for (int y = 1; y < 4; ++y) {
        for (int x = 0; x < 5; ++x) {
            const int index = y * 5 + x;
            expectedScratch[index] =
                BlurTestAverage(expected[index - 5], expected[index], expected[index + 5]);
        }
    }
    for (int i = 0; i < 25; ++i) {
        expected[i] = expectedScratch[i];
    }
    for (int y = 0; y < 5; ++y) {
        for (int x = 1; x < 4; ++x) {
            const int index = y * 5 + x;
            expected[index] = BlurTestAverage(expectedScratch[index - 1],
                                              expectedScratch[index],
                                              expectedScratch[index + 1]);
        }
    }

    bool ok = true;
    for (int i = 0; i < 25; ++i) {
        ok = ok && pixels[i] == expected[i];
    }

    RestoreBlurGlobals(oldFxPixels, oldScratch, oldWidth, oldHeight, oldPitchBytes,
                       oldPitchPixels);
    return ok ? 0 : 1;
}

extern "C" int zvideo_blur_region_by_mode_smoke(void) {
    unsigned short *oldFxPixels;
    unsigned short *oldScratch;
    int oldWidth;
    int oldHeight;
    int oldPitchBytes;
    int oldPitchPixels;
    SaveBlurGlobals(&oldFxPixels, &oldScratch, &oldWidth, &oldHeight, &oldPitchBytes,
                    &oldPitchPixels);

    unsigned short pixelsA[25];
    unsigned short pixelsB[25];
    unsigned short scratch[25] = {};
    bool ok = true;
    for (int mode = 1; mode <= 3; ++mode) {
        for (int i = 0; i < 25; ++i) {
            pixelsA[i] = static_cast<unsigned short>((i * 5 + mode) & 0xffff);
            pixelsB[i] = pixelsA[i];
            scratch[i] = 0;
        }

        SetupBlurSurface(pixelsA, scratch, 5, 5);
        zVideo::buff_BlurRegionByMode(0, mode);
        for (int i = 0; i < 25; ++i) {
            scratch[i] = 0;
        }
        SetupBlurSurface(pixelsB, scratch, 5, 5);
        if (mode == 1) {
            zVideo::buff_BlurRegionHorizontal(0, mode);
        } else if (mode == 2) {
            zVideo::buff_BlurRegionVertical(0, mode);
        } else {
            zVideo::buff_BlurRegionCombined(0, mode);
        }

        for (int i = 0; i < 25; ++i) {
            ok = ok && pixelsA[i] == pixelsB[i];
        }
    }

    RestoreBlurGlobals(oldFxPixels, oldScratch, oldWidth, oldHeight, oldPitchBytes,
                       oldPitchPixels);
    return ok ? 0 : 1;
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

void __fastcall ClearSwFake(zVidRect32 *surfaceRect, zVidRect32 *zRect) {
    ++g_clearSwCalls;
    g_lastClearSwSurfaceRect = surfaceRect;
    g_lastClearSwZRect = zRect;
}

void __fastcall ClearStateFake(zVidRect32 *rect, zVideo_SurfaceStatePartial *surfaceState) {
    ++g_clearPrimaryCalls;
    g_lastClearPrimaryRect = rect;
    g_lastClearPrimaryState = surfaceState;
}

std::int32_t __fastcall SetVideoModeFake(std::int32_t) {
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

    zVideo_SurfaceStateProc const savedLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    zVideo_SurfaceStateProc const savedUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;
    const zVideo_SurfaceStatePartial savedDisplayModeSurfaceState =
        g_zVideo_DisplayModeSurfaceState;

    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_pfnLockSurfaceState = CaptureLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = CaptureUnlockSurfaceState;
    g_zVideoTestLockSurfaceCount = 0;
    g_zVideoTestLockSurfaceState = nullptr;
    g_zVideoTestUnlockSurfaceCount = 0;
    g_zVideoTestUnlockSurfaceState = nullptr;

    const int displayLockResult = zVideo::Dispatch_LockDisplayModeSurfaceState();
    const int displayUnlockResult = zVideo::Dispatch_UnlockDisplayModeSurfaceState();
    const bool displayDispatchOk =
        displayLockResult == 1 && displayUnlockResult == 0x6a5 &&
        g_zVideoTestLockSurfaceCount == 1 &&
        g_zVideoTestLockSurfaceState == &g_zVideo_DisplayModeSurfaceState &&
        g_zVideoTestUnlockSurfaceCount == 1 &&
        g_zVideoTestUnlockSurfaceState == &g_zVideo_DisplayModeSurfaceState &&
        g_zVideo_DisplayModeSurfaceState.locked == 0;

    g_zVideo_pfnLockSurfaceState = savedLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = savedUnlockSurfaceState;
    g_zVideo_DisplayModeSurfaceState = savedDisplayModeSurfaceState;

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

    return displayDispatchOk && unlockOk ? 0 : 3;
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
        g_zVideo_pfnAdjustSurfaces != zVideo_dd3d::PresentDisplayModeSurface ||
        g_zVideo_pfnGetHwApiDeviceFeatureFlags != zVideo_dd::GetHwApiDeviceFeatureFlags ||
        g_zVideo_pfnBltSwToPrimaryRectDirect != zVideo_dd::BltSwToPrimaryRectDirect ||
        g_zVideo_pfnBltPrimaryToSwRectDirect != zVideo_dd::BltPrimaryToSwRectDirect ||
        selectedDevice.m_deviceFeatureFlags != 0) {
        return 1;
    }

    g_zVideo_pSelectedHwApiDeviceRecord = nullptr;
    zVideo::BindRendererDispatch(0, 1);
    return g_zVideo_RendererType == 0 && g_zVideo_ActiveRendererPath == 0 &&
                   g_zVideo_FullscreenOption == 1 &&
                   g_zVideo_pfnAdjustSurfaces == zVideo_dd::PresentDisplayModeSurface
               ? 0
               : 2;
}

extern "C" int zvideo_dd_get_hw_api_device_feature_flags_smoke(void) {
    const int savedFeature0 = g_zVideo_HwApiDeviceTable[0].m_deviceFeatureFlags;
    const int savedFeature2 = g_zVideo_HwApiDeviceTable[2].m_deviceFeatureFlags;
    const zVideo_GetHwApiDeviceFeatureFlagsProc savedProc =
        g_zVideo_pfnGetHwApiDeviceFeatureFlags;

    g_zVideo_HwApiDeviceTable[0].m_deviceFeatureFlags = 0x1357;
    g_zVideo_HwApiDeviceTable[2].m_deviceFeatureFlags = 0x2468;
    g_zVideo_pfnGetHwApiDeviceFeatureFlags = zVideo_dd::GetHwApiDeviceFeatureFlags;

    const bool ok =
        zVideo_dd::GetHwApiDeviceFeatureFlags(0) == 0x1357 &&
        zVideo_dd::GetHwApiDeviceFeatureFlags(2) == 0x2468 &&
        g_zVideo_pfnGetHwApiDeviceFeatureFlags(2) == 0x2468;

    g_zVideo_HwApiDeviceTable[0].m_deviceFeatureFlags = savedFeature0;
    g_zVideo_HwApiDeviceTable[2].m_deviceFeatureFlags = savedFeature2;
    g_zVideo_pfnGetHwApiDeviceFeatureFlags = savedProc;
    return ok ? 0 : 1;
}

extern "C" int zvid_query_device_video_memory_bytes_smoke(void) {
    FakeDirectDraw2Object directDraw{};
    FakeDirectDrawSurfaceObject createdSurface{};
    FakeDirectDrawSurface3Object surface3{};
    InstallFakeDirectDraw2(
        directDraw,
        createdSurface,
        surface3
    );

    const int savedRendererType = g_zVideo_RendererType;
    IDirectDraw2 *const savedDirectDraw2 = g_zVideo_pDirectDraw2;
    zVidHwApiDeviceRecordPartial *const savedSelectedDevice =
        g_zVideo_pSelectedHwApiDeviceRecord;
    const zVidHwApiDeviceRecordPartial savedRecord1 = g_zVideo_HwApiDeviceTable[1];
    const zVidHwApiDeviceRecordPartial savedRecord2 = g_zVideo_HwApiDeviceTable[2];

    int failCode = 0;
    int totalBytes = 123;
    int freeBytes = 456;
    g_zVideo_RendererType = 0;
    if (zVid::QueryDeviceVideoMemoryBytes(
            1,
            &totalBytes,
            &freeBytes
        ) != 0 ||
        totalBytes != 0 || freeBytes != 0 ||
        gFakeDirectDraw2GetAvailableVidMemCalls != 0) {
        failCode = 1;
    }

    zVidHwApiDeviceRecordPartial selectedDevice{};
    selectedDevice.m_textureMemTotalBytes = 0x2000;
    g_zVideo_RendererType = 1;
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    gFakeDirectDraw2AvailableVidMemTotal = 0x900000;
    gFakeDirectDraw2AvailableVidMemFree = 0x700000;
    totalBytes = 0;
    freeBytes = 0;
    if (failCode == 0 &&
        (zVid::QueryDeviceVideoMemoryBytes(
             -1,
             &totalBytes,
             &freeBytes
         ) != 1 ||
         gFakeDirectDraw2GetAvailableVidMemCalls != 1 ||
         gFakeDirectDraw2LastAvailableVidMemCapsValue.dwCaps != DDSCAPS_VIDEOMEMORY ||
         gFakeDirectDraw2LastAvailableVidMemTotal != (LPDWORD)(&totalBytes) ||
         gFakeDirectDraw2LastAvailableVidMemFree != (LPDWORD)(&freeBytes) ||
         totalBytes != 0x900000 || freeBytes != 0x6fe000)) {
        failCode = 2;
    }

    gFakeDirectDraw2GetAvailableVidMemResult = DDERR_INVALIDPARAMS;
    totalBytes = 333;
    freeBytes = 444;
    if (failCode == 0 &&
        (zVid::QueryDeviceVideoMemoryBytes(
             -1,
             &totalBytes,
             &freeBytes
         ) != 1 ||
         gFakeDirectDraw2GetAvailableVidMemCalls != 2 ||
         totalBytes != 0 || freeBytes != 0)) {
        failCode = 3;
    }

    g_zVideo_HwApiDeviceTable[1].m_videoMemTotalBytes = 0x900000;
    g_zVideo_HwApiDeviceTable[1].m_videoMemFreeBytes = 0x700000;
    g_zVideo_HwApiDeviceTable[1].m_textureMemTotalBytes = 0x200000;
    totalBytes = 0;
    freeBytes = 0;
    if (failCode == 0 &&
        (zVid::QueryDeviceVideoMemoryBytes(
             1,
             &totalBytes,
             &freeBytes
         ) != 1 ||
         totalBytes != 0x900000 || freeBytes != 0x500000 ||
         gFakeDirectDraw2GetAvailableVidMemCalls != 2)) {
        failCode = 4;
    }

    g_zVideo_HwApiDeviceTable[2].m_videoMemTotalBytes = 0x500000;
    g_zVideo_HwApiDeviceTable[2].m_videoMemFreeBytes = 0x480000;
    g_zVideo_HwApiDeviceTable[2].m_textureMemTotalBytes = 0x500000;
    totalBytes = 0;
    freeBytes = 0;
    if (failCode == 0 &&
        (zVid::QueryDeviceVideoMemoryBytes(
             2,
             &totalBytes,
             &freeBytes
         ) != 1 ||
         totalBytes != 0x500000 || freeBytes != 0x28c000)) {
        failCode = 5;
    }

    g_zVideo_RendererType = savedRendererType;
    g_zVideo_pDirectDraw2 = savedDirectDraw2;
    g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
    g_zVideo_HwApiDeviceTable[1] = savedRecord1;
    g_zVideo_HwApiDeviceTable[2] = savedRecord2;
    return failCode;
}

extern "C" int zvid_query_texture_memory_bytes_smoke(void) {
    FakeDirectDraw2Object directDraw{};
    FakeDirectDrawSurfaceObject createdSurface{};
    FakeDirectDrawSurface3Object surface3{};
    InstallFakeDirectDraw2(
        directDraw,
        createdSurface,
        surface3
    );

    IDirectDraw2 *const savedDirectDraw2 = g_zVideo_pDirectDraw2;
    const zVidHwApiDeviceRecordPartial savedRecord2 = g_zVideo_HwApiDeviceTable[2];

    int failCode = 0;
    int totalBytes = 123;
    int freeBytes = 456;
    g_zVideo_pDirectDraw2 = nullptr;
    if (zVid::QueryTextureMemoryBytes(
            -1,
            &totalBytes,
            &freeBytes
        ) != 0 ||
        totalBytes != 0 || freeBytes != 0 ||
        gFakeDirectDraw2GetAvailableVidMemCalls != 0) {
        failCode = 1;
    }

    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    gFakeDirectDraw2AvailableVidMemTotal = 0x810000;
    gFakeDirectDraw2AvailableVidMemFree = 0x610000;
    totalBytes = 0;
    freeBytes = 0;
    if (failCode == 0 &&
        (zVid::QueryTextureMemoryBytes(
             -1,
             &totalBytes,
             &freeBytes
         ) != 1 ||
         gFakeDirectDraw2GetAvailableVidMemCalls != 1 ||
         gFakeDirectDraw2LastAvailableVidMemCapsValue.dwCaps != DDSCAPS_TEXTURE ||
         gFakeDirectDraw2LastAvailableVidMemTotal != (LPDWORD)(&totalBytes) ||
         gFakeDirectDraw2LastAvailableVidMemFree != (LPDWORD)(&freeBytes) ||
         totalBytes != 0x810000 || freeBytes != 0x610000)) {
        failCode = 2;
    }

    gFakeDirectDraw2GetAvailableVidMemResult = DDERR_INVALIDPARAMS;
    totalBytes = 333;
    freeBytes = 444;
    if (failCode == 0 &&
        (zVid::QueryTextureMemoryBytes(
             -1,
             &totalBytes,
             &freeBytes
         ) != 1 ||
         gFakeDirectDraw2GetAvailableVidMemCalls != 2 ||
         totalBytes != 0 || freeBytes != 0)) {
        failCode = 3;
    }

    g_zVideo_HwApiDeviceTable[2].m_textureMemTotalBytes = 0x500000;
    g_zVideo_HwApiDeviceTable[2].m_textureMemFreeBytes = 0x320000;
    totalBytes = 0;
    freeBytes = 0;
    if (failCode == 0 &&
        (zVid::QueryTextureMemoryBytes(
             2,
             &totalBytes,
             &freeBytes
         ) != 1 ||
         totalBytes != 0x500000 || freeBytes != 0x320000 ||
         gFakeDirectDraw2GetAvailableVidMemCalls != 2)) {
        failCode = 4;
    }

    g_zVideo_pDirectDraw2 = savedDirectDraw2;
    g_zVideo_HwApiDeviceTable[2] = savedRecord2;
    return failCode;
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

void __fastcall BltPrimaryToSwDirectFake(zVidRect32 *srcRect, zVidRect32 *dstRect) {
    if (srcRect == nullptr && dstRect == nullptr) {
        ++g_bltPrimaryToSwDirectCalls;
    }
}

std::int32_t __fastcall AdjustSurfacesFake(zVidRect32 *, zVidRect32 *,
                                                std::int32_t waitForPresent,
                                                std::int32_t blitPrimaryToSwFirst) {
    ++g_adjustSurfaceCalls;
    g_lastAdjustWaitForPresent = waitForPresent;
    g_lastAdjustBlitPrimaryToSwFirst = blitPrimaryToSwFirst;
    return 0x123;
}

std::int32_t __fastcall TextureMemoryQueryFake(std::int32_t flags, std::int32_t *totalBytes,
                                                    std::int32_t *freeBytes) {
    ++g_textureMemoryQueryCalls;
    g_lastTextureMemoryQueryFlags = flags;
    *totalBytes = 0x1000;
    *freeBytes = 0x200;
    return 1;
}

std::int32_t __fastcall DeviceMemoryQueryFake(std::int32_t flags, std::int32_t *totalBytes,
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

extern "C" int zvideo_dd_enum_direct3d_device_callback_smoke(void) {
    const int savedAcceptedHardwareCount = g_zVid_AcceptedHardwareRendererCount;
    CodeFunctionPatch teardownPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(zVideo_dd::TeardownVideoSubsystem),
            reinterpret_cast<void *>(FakeTeardownVideoSubsystem),
            teardownPatch
        )) {
        return 1;
    }

    GUID guid = {0x12345678, 0x1111, 0x2222, {3, 4, 5, 6, 7, 8, 9, 10}};
    zVidHwApiDeviceRecordPartial entry{};
    D3DDEVICEDESC desc{};
    g_zVid_AcceptedHardwareRendererCount = 0;
    gFakeTeardownVideoSubsystemCalls = 0;

    desc.dwFlags = 0;
    desc.dcmColorModel = D3DCOLOR_RGB;
    desc.dwDeviceZBufferBitDepth = DDBD_16;
    const bool noHardwareOk =
        zVideo_dd::EnumDirect3DDeviceCallback(
            &guid,
            const_cast<LPSTR>("desc-skip-hw"),
            const_cast<LPSTR>("name-skip-hw"),
            &desc,
            0,
            &entry
        ) == 1 &&
        entry.m_acceptedD3DDeviceCount == 0 &&
        g_zVid_AcceptedHardwareRendererCount == 0;

    desc = {};
    desc.dwFlags = D3DDD_COLORMODEL;
    desc.dcmColorModel = D3DCOLOR_MONO;
    desc.dwDeviceZBufferBitDepth = DDBD_16;
    const bool nonRgbOk =
        zVideo_dd::EnumDirect3DDeviceCallback(
            &guid,
            const_cast<LPSTR>("desc-skip-rgb"),
            const_cast<LPSTR>("name-skip-rgb"),
            &desc,
            0,
            &entry
        ) == 1 &&
        entry.m_acceptedD3DDeviceCount == 0 &&
        g_zVid_AcceptedHardwareRendererCount == 0;

    desc = {};
    desc.dwFlags = D3DDD_COLORMODEL;
    desc.dcmColorModel = D3DCOLOR_RGB;
    desc.dwDeviceZBufferBitDepth = 0;
    const bool noZBufferOk =
        zVideo_dd::EnumDirect3DDeviceCallback(
            &guid,
            const_cast<LPSTR>("desc-skip-z"),
            const_cast<LPSTR>("name-skip-z"),
            &desc,
            0,
            &entry
        ) == 1 &&
        entry.m_acceptedD3DDeviceCount == 0 &&
        g_zVid_AcceptedHardwareRendererCount == 0;

    desc = {};
    desc.dwFlags = D3DDD_COLORMODEL;
    desc.dcmColorModel = D3DCOLOR_RGB;
    desc.dwDeviceZBufferBitDepth = DDBD_16;
    desc.dwMaxTextureWidth = 0;
    desc.dwMaxTextureHeight = 0;
    const bool acceptWithGuidResult =
        zVideo_dd::EnumDirect3DDeviceCallback(
            &guid,
            const_cast<LPSTR>("accepted-description"),
            const_cast<LPSTR>("accepted-name"),
            &desc,
            0,
            &entry
        ) == 1;
    D3DDEVICEDESC *storedDesc0 = &entry.m_d3dDrivers[0].m_hwDesc;
    const bool acceptWithGuidOk =
        acceptWithGuidResult &&
        entry.m_acceptedD3DDeviceCount == 1 &&
        g_zVid_AcceptedHardwareRendererCount == 1 &&
        entry.m_d3dDrivers[0].pD3DDeviceGuid ==
            &entry.m_d3dDrivers[0].m_d3dDeviceGuidStorage &&
        IsEqualGUID(entry.m_d3dDrivers[0].m_d3dDeviceGuidStorage, guid) &&
        std::strcmp(entry.m_d3dDrivers[0].m_deviceName, "accepted-name") == 0 &&
        std::strcmp(
            entry.m_d3dDrivers[0].m_deviceDescription,
            "accepted-description"
        ) == 0 &&
        storedDesc0->dwDeviceZBufferBitDepth == DDBD_16 &&
        storedDesc0->dwMaxTextureWidth == 0x100 &&
        storedDesc0->dwMaxTextureHeight == 0x100;

    desc = {};
    desc.dwFlags = D3DDD_COLORMODEL;
    desc.dcmColorModel = D3DCOLOR_RGB;
    desc.dwDeviceZBufferBitDepth = DDBD_16;
    desc.dwMaxTextureWidth = 64;
    desc.dwMaxTextureHeight = 128;
    const bool acceptNullGuidResult =
        zVideo_dd::EnumDirect3DDeviceCallback(
            0,
            const_cast<LPSTR>("second-description"),
            const_cast<LPSTR>("second-name"),
            &desc,
            0,
            &entry
        ) == 1;
    D3DDEVICEDESC *storedDesc1 = &entry.m_d3dDrivers[1].m_hwDesc;
    const bool acceptNullGuidOk =
        acceptNullGuidResult &&
        entry.m_acceptedD3DDeviceCount == 2 &&
        g_zVid_AcceptedHardwareRendererCount == 2 &&
        entry.m_d3dDrivers[1].pD3DDeviceGuid == 0 &&
        storedDesc1->dwMaxTextureWidth == 64 &&
        storedDesc1->dwMaxTextureHeight == 128;

    zVidHwApiDeviceRecordPartial fullEntry{};
    fullEntry.m_acceptedD3DDeviceCount = 4;
    desc = {};
    desc.dwFlags = D3DDD_COLORMODEL;
    desc.dcmColorModel = D3DCOLOR_RGB;
    desc.dwDeviceZBufferBitDepth = DDBD_16;
    const bool capacityOk =
        zVideo_dd::EnumDirect3DDeviceCallback(
            &guid,
            const_cast<LPSTR>("desc-full"),
            const_cast<LPSTR>("name-full"),
            &desc,
            0,
            &fullEntry
        ) == 0 &&
        fullEntry.m_acceptedD3DDeviceCount == 4 &&
        gFakeTeardownVideoSubsystemCalls == 1;

    RestoreFunctionPatch(teardownPatch);
    g_zVid_AcceptedHardwareRendererCount = savedAcceptedHardwareCount;
    return noHardwareOk && nonRgbOk && noZBufferOk && acceptWithGuidOk &&
                   acceptNullGuidOk && capacityOk
               ? 0
               : 2;
}

extern "C" int zvideo_dd_enumerate_direct3d_devices_for_record_smoke(void) {
    IDirectDraw2 *const savedDirectDraw = g_zVideo_pDirectDraw2;
    IDirect3D2 *const savedD3D = g_zVideo_pD3D2;

    FakeDirectDraw2Object directDraw{};
    FakeDirectDrawSurfaceObject createdSurface{};
    FakeDirectDrawSurface3Object surface3{};
    FakeD3D2Object d3d{};
    FakeD3DDevice2Object d3dDevice{};
    FakeD3DViewport2Object viewport{};
    FakeD3DMaterial2Object material{};

    InstallFakeDirectDraw2(
        directDraw,
        createdSurface,
        surface3
    );
    InstallFakeD3D2(
        d3d,
        reinterpret_cast<IDirect3DDevice2 *>(&d3dDevice),
        reinterpret_cast<IDirect3DViewport2 *>(&viewport),
        reinterpret_cast<IDirect3DMaterial2 *>(&material)
    );
    gFakeDirectDraw2QueryInterfaceValue = reinterpret_cast<void *>(&d3d);
    gFakeD3D2EnumDevicesAcceptedCount = 2;
    g_zVideo_pDirectDraw2 = reinterpret_cast<IDirectDraw2 *>(&directDraw);
    g_zVideo_pD3D2 = nullptr;

    zVidHwApiDeviceRecordPartial acceptedEntry{};
    std::strncpy(
        acceptedEntry.m_driverName,
        "driver-a",
        sizeof(acceptedEntry.m_driverName)
    );
    acceptedEntry.m_acceptedD3DDeviceCount = 99;

    const int acceptedResult =
        zVideo_dd::EnumerateDirect3DDevicesForRecord(&acceptedEntry);
    const bool acceptedOk =
        acceptedResult == 1 &&
        gFakeDirectDraw2QueryInterfaceCalls == 1 &&
        IsEqualGUID(*gFakeDirectDraw2LastQueryInterfaceIid, IID_IDirect3D2) &&
        gFakeDirectDraw2LastQueryInterfaceOut == (void **)(&g_zVideo_pD3D2) &&
        gFakeD3D2EnumDevicesCalls == 1 &&
        gFakeD3D2LastEnumDevicesCallback ==
            zVideo_dd::EnumDirect3DDeviceCallback &&
        gFakeD3D2LastEnumDevicesContext == &acceptedEntry &&
        gFakeD3D2EnumDevicesInitialAcceptedCount == 0 &&
        acceptedEntry.m_acceptedD3DDeviceCount == 2 &&
        gFakeD3D2ReleaseCalls == 1 &&
        g_zVideo_pD3D2 == nullptr;

    InstallFakeDirectDraw2(
        directDraw,
        createdSurface,
        surface3
    );
    InstallFakeD3D2(
        d3d,
        reinterpret_cast<IDirect3DDevice2 *>(&d3dDevice),
        reinterpret_cast<IDirect3DViewport2 *>(&viewport),
        reinterpret_cast<IDirect3DMaterial2 *>(&material)
    );
    gFakeDirectDraw2QueryInterfaceValue = reinterpret_cast<void *>(&d3d);
    gFakeD3D2EnumDevicesAcceptedCount = 0;
    g_zVideo_pDirectDraw2 = reinterpret_cast<IDirectDraw2 *>(&directDraw);
    g_zVideo_pD3D2 = nullptr;

    zVidHwApiDeviceRecordPartial emptyEntry{};
    std::strncpy(
        emptyEntry.m_driverName,
        "driver-empty",
        sizeof(emptyEntry.m_driverName)
    );
    emptyEntry.m_acceptedD3DDeviceCount = 7;

    const int emptyResult =
        zVideo_dd::EnumerateDirect3DDevicesForRecord(&emptyEntry);
    const bool emptyOk =
        emptyResult == 0 &&
        gFakeD3D2EnumDevicesCalls == 1 &&
        gFakeD3D2EnumDevicesInitialAcceptedCount == 0 &&
        emptyEntry.m_acceptedD3DDeviceCount == 0 &&
        gFakeD3D2ReleaseCalls == 1 &&
        g_zVideo_pD3D2 == nullptr;

    InstallFakeDirectDraw2(
        directDraw,
        createdSurface,
        surface3
    );
    InstallFakeD3D2(
        d3d,
        reinterpret_cast<IDirect3DDevice2 *>(&d3dDevice),
        reinterpret_cast<IDirect3DViewport2 *>(&viewport),
        reinterpret_cast<IDirect3DMaterial2 *>(&material)
    );
    gFakeDirectDraw2QueryInterfaceResult = DDERR_GENERIC;
    g_zVideo_pDirectDraw2 = reinterpret_cast<IDirectDraw2 *>(&directDraw);
    g_zVideo_pD3D2 = reinterpret_cast<IDirect3D2 *>(&d3d);

    zVidHwApiDeviceRecordPartial failureEntry{};
    std::strncpy(
        failureEntry.m_driverName,
        "driver-fail",
        sizeof(failureEntry.m_driverName)
    );
    failureEntry.m_acceptedD3DDeviceCount = 3;

    const int failureResult =
        zVideo_dd::EnumerateDirect3DDevicesForRecord(&failureEntry);
    const bool failureOk =
        failureResult == 0 &&
        gFakeDirectDraw2QueryInterfaceCalls == 1 &&
        gFakeD3D2EnumDevicesCalls == 0 &&
        gFakeD3D2ReleaseCalls == 0 &&
        failureEntry.m_acceptedD3DDeviceCount == 3 &&
        g_zVideo_pD3D2 == reinterpret_cast<IDirect3D2 *>(&d3d);

    g_zVideo_pDirectDraw2 = savedDirectDraw;
    g_zVideo_pD3D2 = savedD3D;
    return acceptedOk && emptyOk && failureOk ? 0 : 1;
}

extern "C" int zvideo_dd_enum_directdraw_device_callback_smoke(void) {
    const int savedAcceptedCount = g_zVideo_NumAcceptedDirectDrawDevices;
    const int savedOrdinal = g_zVideo_DirectDrawEnumOrdinal;
    IDirectDraw2 *const savedDirectDraw = g_zVideo_pDirectDraw2;
    zVidHwApiDeviceRecordPartial *const savedSelected =
        g_zVideo_pSelectedHwApiDeviceRecord;
    const DDCAPS savedHalCaps = g_zVideo_DDrawCapsHal;
    const DDCAPS savedHelCaps = g_zVideo_DDrawCapsHel;
    const zVidHwApiDeviceRecordPartial savedEntry0 = g_zVideo_HwApiDeviceTable[0];
    const zVidHwApiDeviceRecordPartial savedEntry1 = g_zVideo_HwApiDeviceTable[1];

    CodeFunctionPatch createPatch{};
    CodeFunctionPatch enumPatch{};
    CodeFunctionPatch teardownPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(zVideo_dd::CreateDirectDraw2ForSelectedDevice),
            reinterpret_cast<void *>(FakeCreateDirectDraw2ForSelectedDevice),
            createPatch
        )) {
        return 1;
    }
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(zVideo_dd::EnumerateDirect3DDevicesForRecord),
            reinterpret_cast<void *>(FakeEnumerateDirect3DDevicesForRecord),
            enumPatch
        )) {
        RestoreFunctionPatch(createPatch);
        return 2;
    }
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(zVideo_dd::TeardownVideoSubsystem),
            reinterpret_cast<void *>(FakeTeardownVideoSubsystem),
            teardownPatch
        )) {
        RestoreFunctionPatch(enumPatch);
        RestoreFunctionPatch(createPatch);
        return 3;
    }

    FakeDirectDraw2Object directDraw{};
    FakeDirectDrawSurfaceObject createdSurface{};
    FakeDirectDrawSurface3Object surface3{};

    g_zVideo_NumAcceptedDirectDrawDevices = 4;
    g_zVideo_DirectDrawEnumOrdinal = 5;
    gFakeCreateDirectDraw2ForSelectedDeviceCalls = 0;
    gFakeEnumerateDirect3DDevicesForRecordCalls = 0;
    gFakeTeardownVideoSubsystemCalls = 0;
    const BOOL capacityResult =
        zVideo_dd::EnumDirectDrawDeviceCallback(
            0,
            const_cast<LPSTR>("capacity-description"),
            const_cast<LPSTR>("capacity-driver"),
            0
        );
    const bool capacityOk =
        capacityResult == FALSE &&
        g_zVideo_DirectDrawEnumOrdinal == 6 &&
        g_zVideo_NumAcceptedDirectDrawDevices == 4 &&
        gFakeCreateDirectDraw2ForSelectedDeviceCalls == 0 &&
        gFakeEnumerateDirect3DDevicesForRecordCalls == 0 &&
        gFakeTeardownVideoSubsystemCalls == 0;

    InstallFakeDirectDraw2(
        directDraw,
        createdSurface,
        surface3
    );
    g_zVideo_pDirectDraw2 = reinterpret_cast<IDirectDraw2 *>(&directDraw);
    g_zVideo_NumAcceptedDirectDrawDevices = 0;
    g_zVideo_DirectDrawEnumOrdinal = 8;
    gFakeCreateDirectDraw2ForSelectedDeviceCalls = 0;
    gFakeEnumerateDirect3DDevicesForRecordCalls = 0;
    gFakeEnumerateDirect3DDevicesForRecordEntry = nullptr;
    gFakeEnumerateDirect3DDevicesForRecordResult = 1;
    gFakeTeardownVideoSubsystemCalls = 0;
    gFakeDirectDraw2GetCapsHalValue = {};
    gFakeDirectDraw2GetCapsHelValue = {};
    gFakeDirectDraw2GetCapsHalValue.dwSize = sizeof(DDCAPS);
    gFakeDirectDraw2GetCapsHelValue.dwSize = sizeof(DDCAPS);
    gFakeDirectDraw2GetCapsHalValue.dwCaps = 0x200;
    gFakeDirectDraw2AvailableVidMemTotal = 0x400000;
    gFakeDirectDraw2AvailableVidMemFree = 0x300000;
    std::memset(&g_zVideo_HwApiDeviceTable[0], 0x7f, sizeof(g_zVideo_HwApiDeviceTable[0]));
    GUID guid = {0x87654321, 0x3333, 0x4444, {10, 9, 8, 7, 6, 5, 4, 3}};

    const BOOL acceptedResult =
        zVideo_dd::EnumDirectDrawDeviceCallback(
            &guid,
            const_cast<LPSTR>("accepted-description"),
            const_cast<LPSTR>("driver"),
            reinterpret_cast<LPVOID>(0x1234)
        );
    zVidHwApiDeviceRecordPartial &entry0 = g_zVideo_HwApiDeviceTable[0];
    const bool acceptedOk =
        acceptedResult == TRUE &&
        g_zVideo_DirectDrawEnumOrdinal == 9 &&
        g_zVideo_NumAcceptedDirectDrawDevices == 1 &&
        entry0.pDirectDrawGuid == &entry0.m_directDrawGuidStorage &&
        IsEqualGUID(entry0.m_directDrawGuidStorage, guid) &&
        std::strcmp(entry0.m_driverName, "driver[AGP]") == 0 &&
        std::strcmp(entry0.m_driverDescription, "accepted-description") == 0 &&
        entry0.m_deviceFeatureFlags == 1 &&
        entry0.m_videoMemTotalBytes == 0x400000 &&
        entry0.m_videoMemFreeBytes == 0x300000 &&
        entry0.m_textureMemTotalBytes == 0x400000 &&
        entry0.m_textureMemFreeBytes == 0x300000 &&
        g_zVideo_pSelectedHwApiDeviceRecord == &entry0 &&
        gFakeCreateDirectDraw2ForSelectedDeviceCalls == 1 &&
        gFakeDirectDraw2GetCapsCalls == 1 &&
        gFakeDirectDraw2GetCapsHalInput.dwSize == sizeof(DDCAPS) &&
        gFakeDirectDraw2GetCapsHelInput.dwSize == sizeof(DDCAPS) &&
        gFakeDirectDraw2GetAvailableVidMemCalls == 2 &&
        gFakeDirectDraw2LastAvailableVidMemCapsValue.dwCaps == DDSCAPS_TEXTURE &&
        gFakeEnumerateDirect3DDevicesForRecordCalls == 1 &&
        gFakeEnumerateDirect3DDevicesForRecordEntry == &entry0 &&
        gFakeTeardownVideoSubsystemCalls == 1;

    InstallFakeDirectDraw2(
        directDraw,
        createdSurface,
        surface3
    );
    g_zVideo_pDirectDraw2 = reinterpret_cast<IDirectDraw2 *>(&directDraw);
    g_zVideo_NumAcceptedDirectDrawDevices = 1;
    g_zVideo_DirectDrawEnumOrdinal = 2;
    gFakeCreateDirectDraw2ForSelectedDeviceCalls = 0;
    gFakeEnumerateDirect3DDevicesForRecordCalls = 0;
    gFakeEnumerateDirect3DDevicesForRecordEntry = nullptr;
    gFakeEnumerateDirect3DDevicesForRecordResult = 0;
    gFakeTeardownVideoSubsystemCalls = 0;
    gFakeDirectDraw2GetAvailableVidMemResult = DDERR_INVALIDPARAMS;
    std::memset(&g_zVideo_HwApiDeviceTable[1], 0x7f, sizeof(g_zVideo_HwApiDeviceTable[1]));

    const BOOL rejectedResult =
        zVideo_dd::EnumDirectDrawDeviceCallback(
            0,
            const_cast<LPSTR>("rejected-description"),
            const_cast<LPSTR>("rejected-driver"),
            0
        );
    zVidHwApiDeviceRecordPartial &entry1 = g_zVideo_HwApiDeviceTable[1];
    const bool rejectedOk =
        rejectedResult == TRUE &&
        g_zVideo_DirectDrawEnumOrdinal == 3 &&
        g_zVideo_NumAcceptedDirectDrawDevices == 1 &&
        entry1.pDirectDrawGuid == 0 &&
        std::strcmp(entry1.m_driverName, "rejected-driver") == 0 &&
        std::strcmp(entry1.m_driverDescription, "rejected-description") == 0 &&
        entry1.m_deviceFeatureFlags == 0 &&
        entry1.m_videoMemTotalBytes == 0 &&
        entry1.m_videoMemFreeBytes == 0 &&
        entry1.m_textureMemTotalBytes == 0 &&
        entry1.m_textureMemFreeBytes == 0 &&
        gFakeDirectDraw2GetAvailableVidMemCalls == 2 &&
        gFakeEnumerateDirect3DDevicesForRecordCalls == 1 &&
        gFakeEnumerateDirect3DDevicesForRecordEntry == &entry1 &&
        gFakeTeardownVideoSubsystemCalls == 1;

    InstallFakeDirectDraw2(
        directDraw,
        createdSurface,
        surface3
    );
    g_zVideo_pDirectDraw2 = reinterpret_cast<IDirectDraw2 *>(&directDraw);
    g_zVideo_NumAcceptedDirectDrawDevices = 0;
    g_zVideo_DirectDrawEnumOrdinal = 11;
    gFakeCreateDirectDraw2ForSelectedDeviceCalls = 0;
    gFakeEnumerateDirect3DDevicesForRecordCalls = 0;
    gFakeTeardownVideoSubsystemCalls = 0;
    gFakeDirectDraw2GetCapsResult = DDERR_GENERIC;
    gFakeDirectDraw2GetAvailableVidMemResult = DD_OK;
    gFakeEnumerateDirect3DDevicesForRecordResult = 1;

    const BOOL capsFailureResult =
        zVideo_dd::EnumDirectDrawDeviceCallback(
            &guid,
            const_cast<LPSTR>("caps-fail-description"),
            const_cast<LPSTR>("caps-fail-driver"),
            0
        );
    const bool capsFailureOk =
        capsFailureResult == FALSE &&
        g_zVideo_DirectDrawEnumOrdinal == 12 &&
        g_zVideo_NumAcceptedDirectDrawDevices == 0 &&
        gFakeCreateDirectDraw2ForSelectedDeviceCalls == 1 &&
        gFakeDirectDraw2GetCapsCalls == 1 &&
        gFakeDirectDraw2GetAvailableVidMemCalls == 0 &&
        gFakeEnumerateDirect3DDevicesForRecordCalls == 0 &&
        gFakeTeardownVideoSubsystemCalls == 0;

    RestoreFunctionPatch(teardownPatch);
    RestoreFunctionPatch(enumPatch);
    RestoreFunctionPatch(createPatch);
    g_zVideo_NumAcceptedDirectDrawDevices = savedAcceptedCount;
    g_zVideo_DirectDrawEnumOrdinal = savedOrdinal;
    g_zVideo_pDirectDraw2 = savedDirectDraw;
    g_zVideo_pSelectedHwApiDeviceRecord = savedSelected;
    g_zVideo_DDrawCapsHal = savedHalCaps;
    g_zVideo_DDrawCapsHel = savedHelCaps;
    g_zVideo_HwApiDeviceTable[0] = savedEntry0;
    g_zVideo_HwApiDeviceTable[1] = savedEntry1;
    return capacityOk && acceptedOk && rejectedOk && capsFailureOk ? 0 : 4;
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

extern "C" int zvideo_buff_blt_source_to_primary_clipped_smoke(void) {
    FakeDirectDrawSurface3Object primarySurface{};
    FakeDirectDrawSurface3Object sourceSurface{};
    InstallFakeDirectDrawSurface3(primarySurface, DD_OK, DD_OK, DD_OK);
    sourceSurface.vtable = gFakeDirectDrawSurface3VTable;

    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_PrimarySurfaceState.width = 100;
    g_zVideo_PrimarySurfaceState.height = 80;
    g_zVideo_PrimarySurfaceState.locked = 1;
    g_zVideo_PrimarySurfaceState.surf =
        reinterpret_cast<IDirectDrawSurface3 *>(&primarySurface);

    zVidImagePartial image{};
    image.width = 20;
    image.height = 10;
    image.formatFlagsPacked = 2;
    image.surface = reinterpret_cast<IDirectDrawSurface3 *>(&sourceSurface);

    zVidRect32 srcRect{2, 3, 12, 9};
    zVideo_buff::BltSourceToPrimaryClipped(&image, -1, 4, 0, &srcRect);

    return gFakeDirectDrawSurface3UnlockCalls == 1 &&
                   gFakeDirectDrawSurface3BltCalls == 1 &&
                   gFakeDirectDrawSurface3LockCalls == 1 &&
                   gFakeDirectDrawSurface3RestoreCalls == 0 &&
                   g_zVideo_PrimarySurfaceState.locked == 1 &&
                   gFakeDirectDrawSurface3LastBltSource == image.surface &&
                   gFakeDirectDrawSurface3LastBltFx == nullptr &&
                   gFakeDirectDrawSurface3LastBltFlags ==
                       (DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE | DDBLT_KEYSRC) &&
                   gFakeDirectDrawSurface3LastBltDstRect.left == 0 &&
                   gFakeDirectDrawSurface3LastBltDstRect.top == 4 &&
                   gFakeDirectDrawSurface3LastBltDstRect.right == 9 &&
                   gFakeDirectDrawSurface3LastBltDstRect.bottom == 10 &&
                   gFakeDirectDrawSurface3LastBltSrcRect.left == 3 &&
                   gFakeDirectDrawSurface3LastBltSrcRect.top == 3 &&
                   gFakeDirectDrawSurface3LastBltSrcRect.right == 12 &&
                   gFakeDirectDrawSurface3LastBltSrcRect.bottom == 9
               ? 0
               : 1;
}

extern "C" int zvid_image_blit_to_framebuffer_clipped_smoke(void) {
    std::uint16_t frame[16];
    for (int i = 0; i < 16; ++i) {
        frame[i] = 0xaaaa;
    }

    std::uint16_t pixels16[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    zVidImagePartial image{};
    image.width = 4;
    image.height = 3;
    image.pitchWords = 4;
    image.pixels = pixels16;

    zRndr::g_frameBuffer = frame;
    zRndr::g_activeRegionWidth = 4;
    zRndr::g_activeRegionHeight = 4;
    zRndr::g_pitchBytes = 8;
    zRndr::g_pixelPackGreenBits = 6;

    zVid_Image::BlitToFramebufferClipped(&image, -1, 1, 0, nullptr);
    if (frame[4] != 2 || frame[5] != 3 || frame[6] != 4 || frame[7] != 0xaaaa ||
        frame[8] != 6 || frame[9] != 7 || frame[10] != 8 || frame[12] != 10 ||
        frame[13] != 11 || frame[14] != 12) {
        return 1;
    }

    std::uint16_t maskedPixels[2] = {0x1234, 0x00ff};
    frame[0] = 0x1111;
    frame[1] = 0x2222;
    image.width = 2;
    image.height = 1;
    image.pitchWords = 2;
    image.pixels = maskedPixels;
    image.alphaMap = nullptr;
    image.palette = nullptr;
    image.formatFlagsPacked = 2;
    zVid_Image::BlitToFramebufferClipped(&image, 0, 0, 0x00ff, nullptr);
    if (frame[0] != 0x1234 || frame[1] != 0x2222) {
        return 2;
    }

    std::uint8_t alphaMap[2] = {0x80, 3};
    std::uint16_t alphaPixels[2] = {0xffff, 0xf800};
    frame[0] = 0x001f;
    frame[1] = 0x3333;
    image.pixels = alphaPixels;
    image.alphaMap = reinterpret_cast<char *>(alphaMap);
    image.formatFlagsPacked = 0;
    zVid_Image::BlitToFramebufferClipped(&image, 0, 0, 0, nullptr);
    if (frame[0] != ExpectedFramebufferBlend565(0x001f, 0xffff, 0x80) ||
        frame[1] != 0x3333) {
        return 3;
    }

    std::uint8_t palPixels[3] = {1, 2, 3};
    std::uint16_t palette[4] = {0, 0x0101, 0x0202, 0x0303};
    frame[0] = 0xaaaa;
    frame[1] = 0xbbbb;
    frame[2] = 0xcccc;
    image.width = 3;
    image.height = 1;
    image.pitchWords = 3;
    image.pixels = palPixels;
    image.alphaMap = nullptr;
    image.palette = palette;
    image.formatFlagsPacked = 2;
    zVid_Image::BlitToFramebufferClipped(&image, 0, 0, 2, nullptr);
    return frame[0] == 0x0101 && frame[1] == 0xbbbb && frame[2] == 0x0303 ? 0 : 4;
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

extern "C" int zvideo_dd_unlock_directdraw_surface_smoke(void) {
    FakeDirectDrawSurface3Object surface{};
    InstallFakeDirectDrawSurface3(surface, DD_OK, DD_OK, DD_OK);
    IDirectDrawSurface3 *surfaceInterface = reinterpret_cast<IDirectDrawSurface3 *>(&surface);

    if (zVideo_dd::UnlockDirectDrawSurface(surfaceInterface) != 0 ||
        gFakeDirectDrawSurface3UnlockCalls != 1 || gFakeDirectDrawSurface3RestoreCalls != 0 ||
        gFakeDirectDrawSurface3LastUnlockArg != nullptr) {
        return 1;
    }

    InstallFakeDirectDrawSurface3(surface, DDERR_SURFACELOST, DD_OK, DD_OK);
    if (zVideo_dd::UnlockDirectDrawSurface(surfaceInterface) != 0 ||
        gFakeDirectDrawSurface3UnlockCalls != 2 || gFakeDirectDrawSurface3RestoreCalls != 1 ||
        gFakeDirectDrawSurface3LastUnlockArg != nullptr) {
        return 2;
    }

    return 0;
}

extern "C" int zvideo_dd_lock_directdraw_surface_smoke(void) {
    FakeDirectDrawSurface3Object surface{};
    InstallFakeDirectDrawSurface3(surface, DD_OK, DD_OK, DD_OK);
    IDirectDrawSurface3 *surfaceInterface = reinterpret_cast<IDirectDrawSurface3 *>(&surface);
    DDSURFACEDESC surfaceDesc;
    std::memset(&surfaceDesc, 0xcc, sizeof(surfaceDesc));

    if (zVideo_dd::LockDirectDrawSurface(surfaceInterface, &surfaceDesc) != 0 ||
        gFakeDirectDrawSurface3LockCalls != 1 || gFakeDirectDrawSurface3RestoreCalls != 0 ||
        gFakeDirectDrawSurface3LastLockRect != nullptr ||
        gFakeDirectDrawSurface3LastLockDesc != &surfaceDesc ||
        gFakeDirectDrawSurface3LastLockFlags != DDLOCK_WAIT ||
        gFakeDirectDrawSurface3LastLockEvent != nullptr ||
        gFakeDirectDrawSurface3LockDescSize != sizeof(surfaceDesc) ||
        surfaceDesc.dwSize != sizeof(surfaceDesc) || surfaceDesc.dwWidth != 640 ||
        surfaceDesc.dwHeight != 480 || surfaceDesc.lPitch != 1280 ||
        surfaceDesc.lpSurface != gFakeDirectDrawSurface3LockPixels) {
        return 1;
    }

    std::memset(&surfaceDesc, 0xcc, sizeof(surfaceDesc));
    InstallFakeDirectDrawSurface3(surface, DD_OK, DD_OK, DD_OK);
    ConfigureFakeDirectDrawSurface3LockResults(DDERR_SURFACELOST, DD_OK);
    if (zVideo_dd::LockDirectDrawSurface(surfaceInterface, &surfaceDesc) != 0 ||
        gFakeDirectDrawSurface3LockCalls != 2 || gFakeDirectDrawSurface3RestoreCalls != 1 ||
        gFakeDirectDrawSurface3LockDescSize != sizeof(surfaceDesc) ||
        surfaceDesc.dwSize != sizeof(surfaceDesc) || surfaceDesc.dwWidth != 640 ||
        surfaceDesc.dwHeight != 480 || surfaceDesc.lPitch != 1280 ||
        surfaceDesc.lpSurface != gFakeDirectDrawSurface3LockPixels) {
        return 2;
    }

    return 0;
}

extern "C" int zvideo_dd_lock_surface_wait_restore_smoke(void) {
    FakeDirectDrawSurface3Object surface{};
    IDirectDrawSurface3 *surfaceInterface = (IDirectDrawSurface3 *)(&surface);
    DDSURFACEDESC surfaceDesc;

    InstallFakeDirectDrawSurface3(
        surface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    std::memset(
        &surfaceDesc,
        0xcc,
        sizeof(surfaceDesc)
    );
    const bool successOk =
        zVideo_dd::LockSurface_WaitRestore(surfaceInterface, &surfaceDesc) == 0 &&
        gFakeDirectDrawSurface3LockCalls == 1 &&
        gFakeDirectDrawSurface3RestoreCalls == 0 &&
        gFakeDirectDrawSurface3LastLockRect == nullptr &&
        gFakeDirectDrawSurface3LastLockDesc == &surfaceDesc &&
        gFakeDirectDrawSurface3LastLockFlags == DDLOCK_WAIT &&
        gFakeDirectDrawSurface3LastLockEvent == nullptr &&
        gFakeDirectDrawSurface3LockDescSize == sizeof(surfaceDesc) &&
        surfaceDesc.dwSize == sizeof(surfaceDesc) &&
        surfaceDesc.dwWidth == 640 &&
        surfaceDesc.dwHeight == 480 &&
        surfaceDesc.lPitch == gFakeDirectDrawSurface3LockPitch &&
        surfaceDesc.lpSurface == gFakeDirectDrawSurface3LockPixels;

    InstallFakeDirectDrawSurface3(
        surface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    ConfigureFakeDirectDrawSurface3LockResults(
        DDERR_SURFACELOST,
        DD_OK
    );
    std::memset(
        &surfaceDesc,
        0xcc,
        sizeof(surfaceDesc)
    );
    const bool retryOk =
        zVideo_dd::LockSurface_WaitRestore(surfaceInterface, &surfaceDesc) == 0 &&
        gFakeDirectDrawSurface3LockCalls == 2 &&
        gFakeDirectDrawSurface3RestoreCalls == 1 &&
        gFakeDirectDrawSurface3LockDescSize == sizeof(surfaceDesc) &&
        surfaceDesc.dwSize == sizeof(surfaceDesc) &&
        surfaceDesc.dwWidth == 640 &&
        surfaceDesc.dwHeight == 480 &&
        surfaceDesc.lPitch == gFakeDirectDrawSurface3LockPitch &&
        surfaceDesc.lpSurface == gFakeDirectDrawSurface3LockPixels;

    InstallFakeDirectDrawSurface3(
        surface,
        DD_OK,
        DD_OK,
        DDERR_GENERIC
    );
    ConfigureFakeDirectDrawSurface3LockResults(
        DDERR_SURFACELOST,
        DD_OK
    );
    std::memset(
        &surfaceDesc,
        0xcc,
        sizeof(surfaceDesc)
    );
    const bool restoreFailureOk =
        zVideo_dd::LockSurface_WaitRestore(surfaceInterface, &surfaceDesc) ==
            0x5a56ffff &&
        gFakeDirectDrawSurface3LockCalls == 1 &&
        gFakeDirectDrawSurface3RestoreCalls == 1;

    InstallFakeDirectDrawSurface3(
        surface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    ConfigureFakeDirectDrawSurface3LockResults(
        DDERR_GENERIC,
        DDERR_GENERIC
    );
    std::memset(
        &surfaceDesc,
        0xcc,
        sizeof(surfaceDesc)
    );
    const bool lockFailureOk =
        zVideo_dd::LockSurface_WaitRestore(surfaceInterface, &surfaceDesc) ==
            0x5a56ffff &&
        gFakeDirectDrawSurface3LockCalls == 1 &&
        gFakeDirectDrawSurface3RestoreCalls == 0 &&
        surfaceDesc.dwSize == sizeof(surfaceDesc);

    return successOk && retryOk && restoreFailureOk && lockFailureOk ? 0 : 1;
}

extern "C" int zvideo_dd_unlock_surface_wait_restore_smoke(void) {
    FakeDirectDrawSurface3Object surface{};
    IDirectDrawSurface3 *surfaceInterface = (IDirectDrawSurface3 *)(&surface);

    InstallFakeDirectDrawSurface3(
        surface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    const bool successOk =
        zVideo_dd::UnlockSurface_WaitRestore(surfaceInterface) == 0 &&
        gFakeDirectDrawSurface3UnlockCalls == 1 &&
        gFakeDirectDrawSurface3RestoreCalls == 0 &&
        gFakeDirectDrawSurface3LastUnlockArg == nullptr;

    InstallFakeDirectDrawSurface3(
        surface,
        DDERR_SURFACELOST,
        DD_OK,
        DD_OK
    );
    const bool retryOk =
        zVideo_dd::UnlockSurface_WaitRestore(surfaceInterface) == 0 &&
        gFakeDirectDrawSurface3UnlockCalls == 2 &&
        gFakeDirectDrawSurface3RestoreCalls == 1 &&
        gFakeDirectDrawSurface3LastUnlockArg == nullptr;

    InstallFakeDirectDrawSurface3(
        surface,
        DDERR_SURFACELOST,
        DD_OK,
        DDERR_GENERIC
    );
    const bool restoreFailureOk =
        zVideo_dd::UnlockSurface_WaitRestore(surfaceInterface) == 0x5a56ffff &&
        gFakeDirectDrawSurface3UnlockCalls == 1 &&
        gFakeDirectDrawSurface3RestoreCalls == 1 &&
        gFakeDirectDrawSurface3LastUnlockArg == nullptr;

    InstallFakeDirectDrawSurface3(
        surface,
        DDERR_GENERIC,
        DDERR_GENERIC,
        DD_OK
    );
    const bool unlockFailureOk =
        zVideo_dd::UnlockSurface_WaitRestore(surfaceInterface) == 0x5a56ffff &&
        gFakeDirectDrawSurface3UnlockCalls == 1 &&
        gFakeDirectDrawSurface3RestoreCalls == 0 &&
        gFakeDirectDrawSurface3LastUnlockArg == nullptr;

    return successOk && retryOk && restoreFailureOk && unlockFailureOk ? 0 : 1;
}

extern "C" int zvideo_dd_lock_surface_state_smoke(void) {
    FakeDirectDrawSurface3Object surface{};
    InstallFakeDirectDrawSurface3(surface, DD_OK, DD_OK, DD_OK);

    zVideo_SurfaceStatePartial surfaceState{};
    surfaceState.surf = reinterpret_cast<IDirectDrawSurface3 *>(&surface);
    g_zVideo_FullscreenOption = 1;

    if (zVideo_dd::LockSurfaceState(&surfaceState) != 0 ||
        gFakeDirectDrawSurface3LockCalls != 1 || gFakeDirectDrawSurface3RestoreCalls != 0 ||
        surfaceState.width != 640 || surfaceState.height != 480 || surfaceState.pitch != 1280 ||
        surfaceState.pixels != gFakeDirectDrawSurface3LockPixels ||
        surfaceState.locked != 1 || surfaceState.lockInfoValid != 1) {
        return 1;
    }

    return 0;
}

extern "C" int zvideo_dd_unlock_surface_state_smoke(void) {
    FakeDirectDrawSurface3Object surface{};
    InstallFakeDirectDrawSurface3(surface, DD_OK, DD_OK, DD_OK);

    zVideo_SurfaceStatePartial surfaceState{};
    surfaceState.locked = 1;
    surfaceState.surf = reinterpret_cast<IDirectDrawSurface3 *>(&surface);
    g_zVideo_FullscreenOption = 1;

    if (zVideo_dd::UnlockSurfaceState(&surfaceState) != 0 || surfaceState.locked != 0 ||
        gFakeDirectDrawSurface3UnlockCalls != 1 || gFakeDirectDrawSurface3RestoreCalls != 0 ||
        gFakeDirectDrawSurface3LastUnlockArg != nullptr) {
        return 1;
    }

    return 0;
}

extern "C" int zvideo_dd_verify_fullscreen_surface_locks_smoke(void) {
    FakeDirectDrawSurface3Object swSurface{};
    FakeDirectDrawSurface3Object primarySurface{};
    FakeDirectDrawSurface3Object displaySurface{};
    InstallFakeDirectDrawSurface3(
        swSurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    primarySurface.vtable = gFakeDirectDrawSurface3VTable;
    displaySurface.vtable = gFakeDirectDrawSurface3VTable;

    const int savedFullscreenOption = g_zVideo_FullscreenOption;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;
    const zVideo_SurfaceStatePartial savedPrimaryState = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial savedDisplayState =
        g_zVideo_DisplayModeSurfaceState;

    IDirectDrawSurface3 *const swInterface =
        reinterpret_cast<IDirectDrawSurface3 *>(&swSurface);
    IDirectDrawSurface3 *const primaryInterface =
        reinterpret_cast<IDirectDrawSurface3 *>(&primarySurface);
    IDirectDrawSurface3 *const displayInterface =
        reinterpret_cast<IDirectDrawSurface3 *>(&displaySurface);
    g_zVideo_FullscreenOption = 1;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_SwSurfaceState.surf = swInterface;
    g_zVideo_PrimarySurfaceState.surf = primaryInterface;
    g_zVideo_DisplayModeSurfaceState.surf = displayInterface;

    const int successResult = zVideo_dd::VerifyFullscreenSurfaceLocks();
    const bool successOk =
        successResult == 0 &&
        gFakeDirectDrawSurface3LockCalls == 3 &&
        gFakeDirectDrawSurface3UnlockCalls == 3 &&
        gFakeDirectDrawSurface3LockSurfaces[0] == swInterface &&
        gFakeDirectDrawSurface3UnlockSurfaces[0] == swInterface &&
        gFakeDirectDrawSurface3LockSurfaces[1] == primaryInterface &&
        gFakeDirectDrawSurface3UnlockSurfaces[1] == primaryInterface &&
        gFakeDirectDrawSurface3LockSurfaces[2] == displayInterface &&
        gFakeDirectDrawSurface3UnlockSurfaces[2] == displayInterface &&
        g_zVideo_SwSurfaceState.locked == 0 &&
        g_zVideo_PrimarySurfaceState.locked == 0 &&
        g_zVideo_DisplayModeSurfaceState.locked == 0;

    InstallFakeDirectDrawSurface3(
        swSurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    ConfigureFakeDirectDrawSurface3LockResults(
        DDERR_GENERIC,
        DDERR_GENERIC
    );
    primarySurface.vtable = gFakeDirectDrawSurface3VTable;
    displaySurface.vtable = gFakeDirectDrawSurface3VTable;
    g_zVideo_FullscreenOption = 1;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_SwSurfaceState.surf = swInterface;
    g_zVideo_PrimarySurfaceState.surf = primaryInterface;
    g_zVideo_DisplayModeSurfaceState.surf = displayInterface;

    const int firstLockFailureResult = zVideo_dd::VerifyFullscreenSurfaceLocks();
    const bool firstLockFailureOk =
        firstLockFailureResult == 1 &&
        gFakeDirectDrawSurface3LockCalls == 1 &&
        gFakeDirectDrawSurface3UnlockCalls == 0 &&
        gFakeDirectDrawSurface3LockSurfaces[0] == swInterface;

    InstallFakeDirectDrawSurface3(
        swSurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    ConfigureFakeDirectDrawSurface3BltResults(
        DD_OK,
        DD_OK
    );
    gFakeDirectDrawSurface3UnlockResults[0] = DD_OK;
    gFakeDirectDrawSurface3UnlockResults[1] = DD_OK;
    gFakeDirectDrawSurface3UnlockResults[2] = DDERR_GENERIC;
    gFakeDirectDrawSurface3UnlockResultCount = 3;
    primarySurface.vtable = gFakeDirectDrawSurface3VTable;
    displaySurface.vtable = gFakeDirectDrawSurface3VTable;
    g_zVideo_FullscreenOption = 1;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_SwSurfaceState.surf = swInterface;
    g_zVideo_PrimarySurfaceState.surf = primaryInterface;
    g_zVideo_DisplayModeSurfaceState.surf = displayInterface;

    const int finalUnlockFailureResult = zVideo_dd::VerifyFullscreenSurfaceLocks();
    const bool finalUnlockFailureOk =
        finalUnlockFailureResult == 1 &&
        gFakeDirectDrawSurface3LockCalls == 3 &&
        gFakeDirectDrawSurface3UnlockCalls == 3 &&
        gFakeDirectDrawSurface3UnlockSurfaces[2] == displayInterface;

    g_zVideo_FullscreenOption = savedFullscreenOption;
    g_zVideo_SwSurfaceState = savedSwState;
    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    return successOk && firstLockFailureOk && finalUnlockFailureOk ? 0 : 1;
}

extern "C" int zvideo_dd_blt_sw_to_primary_rect_direct_smoke(void) {
    FakeDirectDrawSurface3Object primarySurface{};
    FakeDirectDrawSurface3Object swSurface{};
    InstallFakeDirectDrawSurface3(primarySurface, DD_OK, DD_OK, DD_OK);
    swSurface.vtable = gFakeDirectDrawSurface3VTable;

    const zVideo_SurfaceStatePartial savedPrimaryState = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;

    IDirectDrawSurface3 *const primaryInterface =
        reinterpret_cast<IDirectDrawSurface3 *>(&primarySurface);
    IDirectDrawSurface3 *const swInterface =
        reinterpret_cast<IDirectDrawSurface3 *>(&swSurface);
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState.surf = primaryInterface;
    g_zVideo_SwSurfaceState.surf = swInterface;

    zVidRect32 srcRect{2, 3, 8, 9};
    zVidRect32 dstRect{4, 5, 10, 11};
    zVideo_dd::BltSwToPrimaryRectDirect(
        &srcRect,
        &dstRect
    );

    const bool ok =
        gFakeDirectDrawSurface3BltCalls == 1 &&
        gFakeDirectDrawSurface3LastBltSource == swInterface &&
        gFakeDirectDrawSurface3LastBltFlags == DDBLT_WAIT &&
        gFakeDirectDrawSurface3LastBltFx == 0 &&
        gFakeDirectDrawSurface3LastBltDstRect.left == 4 &&
        gFakeDirectDrawSurface3LastBltDstRect.top == 5 &&
        gFakeDirectDrawSurface3LastBltDstRect.right == 10 &&
        gFakeDirectDrawSurface3LastBltDstRect.bottom == 11 &&
        gFakeDirectDrawSurface3LastBltSrcRect.left == 2 &&
        gFakeDirectDrawSurface3LastBltSrcRect.top == 3 &&
        gFakeDirectDrawSurface3LastBltSrcRect.right == 8 &&
        gFakeDirectDrawSurface3LastBltSrcRect.bottom == 9;

    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_SwSurfaceState = savedSwState;
    return ok ? 0 : 1;
}

extern "C" int zvideo_dd_blt_primary_to_sw_rect_direct_smoke(void) {
    FakeDirectDrawSurface3Object primarySurface{};
    FakeDirectDrawSurface3Object swSurface{};
    InstallFakeDirectDrawSurface3(swSurface, DD_OK, DD_OK, DD_OK);
    primarySurface.vtable = gFakeDirectDrawSurface3VTable;

    const zVideo_SurfaceStatePartial savedPrimaryState = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;

    IDirectDrawSurface3 *const primaryInterface =
        reinterpret_cast<IDirectDrawSurface3 *>(&primarySurface);
    IDirectDrawSurface3 *const swInterface =
        reinterpret_cast<IDirectDrawSurface3 *>(&swSurface);
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState.surf = primaryInterface;
    g_zVideo_SwSurfaceState.surf = swInterface;

    zVidRect32 srcRect{6, 7, 14, 15};
    zVidRect32 dstRect{8, 9, 16, 17};
    zVideo_dd::BltPrimaryToSwRectDirect(
        &srcRect,
        &dstRect
    );

    const bool ok =
        gFakeDirectDrawSurface3BltCalls == 1 &&
        gFakeDirectDrawSurface3LastBltSource == primaryInterface &&
        gFakeDirectDrawSurface3LastBltFlags == DDBLT_WAIT &&
        gFakeDirectDrawSurface3LastBltFx == 0 &&
        gFakeDirectDrawSurface3LastBltDstRect.left == 8 &&
        gFakeDirectDrawSurface3LastBltDstRect.top == 9 &&
        gFakeDirectDrawSurface3LastBltDstRect.right == 16 &&
        gFakeDirectDrawSurface3LastBltDstRect.bottom == 17 &&
        gFakeDirectDrawSurface3LastBltSrcRect.left == 6 &&
        gFakeDirectDrawSurface3LastBltSrcRect.top == 7 &&
        gFakeDirectDrawSurface3LastBltSrcRect.right == 14 &&
        gFakeDirectDrawSurface3LastBltSrcRect.bottom == 15;

    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_SwSurfaceState = savedSwState;
    return ok ? 0 : 1;
}

extern "C" int zvideo_dd_create_surface3_from_desc_smoke(void) {
    FakeDirectDraw2Object directDraw{};
    FakeDirectDrawSurfaceObject createdSurface{};
    FakeDirectDrawSurface3Object surface3{};
    InstallFakeDirectDraw2(
        directDraw,
        createdSurface,
        surface3
    );

    DDSURFACEDESC desc{};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS;
    IDirectDrawSurface3 *outSurface = nullptr;
    HRESULT result = zVideo_dd::CreateSurface3FromDesc(
        reinterpret_cast<IDirectDraw2 *>(&directDraw),
        &desc,
        &outSurface
    );

    const bool successPathOk =
        result == DD_OK &&
        gFakeDirectDraw2CreateSurfaceCalls == 1 &&
        gFakeDirectDraw2LastCreateSurfaceDesc == &desc &&
        gFakeDirectDraw2LastCreateSurfaceOut != nullptr &&
        gFakeDirectDraw2LastCreateSurfaceOuter == nullptr &&
        gFakeDirectDrawSurfaceQueryInterfaceCalls == 1 &&
        IsEqualGUID(
            *gFakeDirectDrawSurfaceLastQueryInterfaceIid,
            IID_IDirectDrawSurface3
        ) &&
        gFakeDirectDrawSurfaceLastQueryInterfaceOut == (void **)(&outSurface) &&
        gFakeDirectDrawSurfaceReleaseCalls == 1 &&
        outSurface == reinterpret_cast<IDirectDrawSurface3 *>(&surface3);

    InstallFakeDirectDraw2(
        directDraw,
        createdSurface,
        surface3
    );
    gFakeDirectDraw2CreateSurfaceResult = DDERR_INVALIDPARAMS;
    outSurface = nullptr;
    result = zVideo_dd::CreateSurface3FromDesc(
        reinterpret_cast<IDirectDraw2 *>(&directDraw),
        &desc,
        &outSurface
    );

    return successPathOk &&
                   result == DDERR_INVALIDPARAMS &&
                   gFakeDirectDraw2CreateSurfaceCalls == 1 &&
                   gFakeDirectDrawSurfaceQueryInterfaceCalls == 0 &&
                   gFakeDirectDrawSurfaceReleaseCalls == 0 &&
                   outSurface == nullptr
               ? 0
               : 1;
}

extern "C" int zvideo_dd_create_directdraw2_for_selected_device_smoke(void) {
    FakeDirectDrawObject directDraw{};
    FakeDirectDraw2Object directDraw2{};
    ImportFunctionPatch directDrawCreatePatch{};
    if (!PatchImportByName(
            "DDRAW.dll",
            "DirectDrawCreate",
            reinterpret_cast<void *>(&FakeDirectDrawCreate),
            directDrawCreatePatch
        )) {
        return 1;
    }

    zVidHwApiDeviceRecordPartial *const savedSelectedDevice =
        g_zVideo_pSelectedHwApiDeviceRecord;
    IDirectDraw2 *const savedDirectDraw2 = g_zVideo_pDirectDraw2;

    GUID directDrawGuid = {0x12345678, 0x1111, 0x2222, {3, 4, 5, 6, 7, 8, 9, 10}};
    zVidHwApiDeviceRecordPartial selectedDevice{};
    selectedDevice.pDirectDrawGuid = &directDrawGuid;
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;

    InstallFakeDirectDraw(
        directDraw,
        reinterpret_cast<IDirectDraw2 *>(&directDraw2)
    );
    g_zVideo_pDirectDraw2 = nullptr;
    const int successResult = zVideo_dd::CreateDirectDraw2ForSelectedDevice();
    const bool successOk =
        successResult == 0 &&
        gFakeDirectDrawCreateCalls == 1 &&
        gFakeDirectDrawCreateGuid == &directDrawGuid &&
        gFakeDirectDrawCreateOut != nullptr &&
        gFakeDirectDrawCreateOuter == nullptr &&
        gFakeDirectDrawQueryInterfaceCalls == 1 &&
        gFakeDirectDrawQueryInterfaceSelf == reinterpret_cast<IDirectDraw *>(&directDraw) &&
        IsEqualGUID(*gFakeDirectDrawQueryInterfaceIid, IID_IDirectDraw2) &&
        gFakeDirectDrawQueryInterfaceOut == (void **)(&g_zVideo_pDirectDraw2) &&
        g_zVideo_pDirectDraw2 == reinterpret_cast<IDirectDraw2 *>(&directDraw2) &&
        gFakeDirectDrawReleaseCalls == 1 &&
        gFakeDirectDrawReleaseSelf == reinterpret_cast<IDirectDraw *>(&directDraw);

    InstallFakeDirectDraw(
        directDraw,
        reinterpret_cast<IDirectDraw2 *>(&directDraw2)
    );
    gFakeDirectDrawCreateResult = DDERR_GENERIC;
    g_zVideo_pDirectDraw2 = reinterpret_cast<IDirectDraw2 *>(0x1357);
    const int createFailureResult = zVideo_dd::CreateDirectDraw2ForSelectedDevice();
    const bool createFailureOk =
        createFailureResult != 0 &&
        gFakeDirectDrawCreateCalls == 1 &&
        gFakeDirectDrawQueryInterfaceCalls == 0 &&
        gFakeDirectDrawReleaseCalls == 0 &&
        g_zVideo_pDirectDraw2 == reinterpret_cast<IDirectDraw2 *>(0x1357);

    InstallFakeDirectDraw(
        directDraw,
        reinterpret_cast<IDirectDraw2 *>(&directDraw2)
    );
    gFakeDirectDrawQueryInterfaceResult = DDERR_GENERIC;
    g_zVideo_pDirectDraw2 = reinterpret_cast<IDirectDraw2 *>(0x2468);
    const int queryFailureResult = zVideo_dd::CreateDirectDraw2ForSelectedDevice();
    const bool queryFailureOk =
        queryFailureResult != 0 &&
        gFakeDirectDrawCreateCalls == 1 &&
        gFakeDirectDrawQueryInterfaceCalls == 1 &&
        gFakeDirectDrawReleaseCalls == 0 &&
        g_zVideo_pDirectDraw2 == reinterpret_cast<IDirectDraw2 *>(0x2468);

    g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
    g_zVideo_pDirectDraw2 = savedDirectDraw2;
    RestoreImportPatch(directDrawCreatePatch);

    return successOk && createFailureOk && queryFailureOk ? 0 : 2;
}

extern "C" int zvideo_dd_open_video_mode_smoke(void) {
    FakeDirectDrawObject directDraw{};
    FakeDirectDraw2Object directDraw2{};
    ImportFunctionPatch directDrawCreatePatch{};
    if (!PatchImportByName(
            "DDRAW.dll",
            "DirectDrawCreate",
            reinterpret_cast<void *>(&FakeDirectDrawCreate),
            directDrawCreatePatch
        )) {
        return 1;
    }

    zVidHwApiDeviceRecordPartial *const savedSelectedDevice =
        g_zVideo_pSelectedHwApiDeviceRecord;
    IDirectDraw2 *const savedDirectDraw2 = g_zVideo_pDirectDraw2;
    const HWND savedHwnd = g_zVideo_hWnd;
    PALETTEENTRY savedSystemPalette[256];
    std::memcpy(savedSystemPalette, g_zVideo_SystemPaletteEntries, sizeof(savedSystemPalette));

    HWND hwnd = CreateWindowExA(
        0,
        "STATIC",
        "recoil-video-open-mode-test",
        WS_OVERLAPPEDWINDOW,
        20,
        30,
        160,
        120,
        nullptr,
        nullptr,
        GetModuleHandleA(nullptr),
        nullptr
    );
    if (hwnd == nullptr) {
        g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
        g_zVideo_pDirectDraw2 = savedDirectDraw2;
        g_zVideo_hWnd = savedHwnd;
        std::memcpy(g_zVideo_SystemPaletteEntries, savedSystemPalette, sizeof(savedSystemPalette));
        RestoreImportPatch(directDrawCreatePatch);
        return 2;
    }

    GUID directDrawGuid = {0x87654321, 0x3333, 0x4444, {10, 9, 8, 7, 6, 5, 4, 3}};
    zVidHwApiDeviceRecordPartial selectedDevice{};
    selectedDevice.pDirectDrawGuid = &directDrawGuid;
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    g_zVideo_hWnd = hwnd;

    InstallFakeDirectDraw(
        directDraw,
        reinterpret_cast<IDirectDraw2 *>(&directDraw2)
    );
    g_zVideo_pDirectDraw2 = nullptr;
    const int successResult = zVideo_dd::OpenVideoMode(99);
    const bool successOk =
        successResult == 0 &&
        gFakeDirectDrawCreateCalls == 1 &&
        gFakeDirectDrawQueryInterfaceCalls == 1 &&
        gFakeDirectDrawReleaseCalls == 1 &&
        g_zVideo_pDirectDraw2 == reinterpret_cast<IDirectDraw2 *>(&directDraw2);

    InstallFakeDirectDraw(
        directDraw,
        reinterpret_cast<IDirectDraw2 *>(&directDraw2)
    );
    gFakeDirectDrawCreateResult = DDERR_GENERIC;
    g_zVideo_pDirectDraw2 = reinterpret_cast<IDirectDraw2 *>(0x9753);
    const int failureResult = zVideo_dd::OpenVideoMode(123);
    const bool failureOk =
        failureResult == 1 &&
        gFakeDirectDrawCreateCalls == 1 &&
        gFakeDirectDrawQueryInterfaceCalls == 0 &&
        gFakeDirectDrawReleaseCalls == 0 &&
        g_zVideo_pDirectDraw2 == reinterpret_cast<IDirectDraw2 *>(0x9753);

    DestroyWindow(hwnd);
    g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
    g_zVideo_pDirectDraw2 = savedDirectDraw2;
    g_zVideo_hWnd = savedHwnd;
    std::memcpy(g_zVideo_SystemPaletteEntries, savedSystemPalette, sizeof(savedSystemPalette));
    RestoreImportPatch(directDrawCreatePatch);

    return successOk && failureOk ? 0 : 3;
}

int RunReleaseAllInterfacesAndSurfacesSmoke(
    int useAtExitWrapper
) {
    FakeComObject d3dMaterial{};
    FakeComObject d3dViewport{};
    FakeComObject d3dDevice{};
    FakeComObject d3d{};
    FakeComObject clipper{};
    FakeComObject palette{};
    InstallFakeComObject(d3dMaterial);
    InstallFakeComObject(d3dViewport);
    InstallFakeComObject(d3dDevice);
    InstallFakeComObject(d3d);
    InstallFakeComObject(clipper);
    InstallFakeComObject(palette);
    ResetFakeComReleaseTracking();

    FakeDirectDrawSurface3Object zBufferSurface{};
    FakeDirectDrawSurface3Object swSurface{};
    FakeDirectDrawSurface3Object primarySurface{};
    FakeDirectDrawSurface3Object displaySurface{};
    InstallFakeDirectDrawSurface3(zBufferSurface, DD_OK, DD_OK, DD_OK);
    swSurface.vtable = gFakeDirectDrawSurface3VTable;
    primarySurface.vtable = gFakeDirectDrawSurface3VTable;
    displaySurface.vtable = gFakeDirectDrawSurface3VTable;

    IDirect3DMaterial2 *const savedMaterial = g_zVideo_pD3DMaterial2;
    IDirect3DViewport2 *const savedViewport = g_zVideo_pD3DViewport2;
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    IDirect3D2 *const savedD3D = g_zVideo_pD3D2;
    IDirectDrawClipper *const savedClipper = g_zVideo_pClipper;
    IDirectDrawSurface3 *const savedZBuffer = g_zVideo_pZBufferSurface;
    IDirectDrawPalette *const savedPalette = g_zVideo_pDDPalette;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;
    const zVideo_SurfaceStatePartial savedPrimaryState = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial savedDisplayState = g_zVideo_DisplayModeSurfaceState;

    g_zVideo_pD3DMaterial2 = reinterpret_cast<IDirect3DMaterial2 *>(&d3dMaterial);
    g_zVideo_pD3DViewport2 = reinterpret_cast<IDirect3DViewport2 *>(&d3dViewport);
    g_zVideo_pD3DDevice = reinterpret_cast<IDirect3DDevice2 *>(&d3dDevice);
    g_zVideo_pD3D2 = reinterpret_cast<IDirect3D2 *>(&d3d);
    g_zVideo_pClipper = reinterpret_cast<IDirectDrawClipper *>(&clipper);
    g_zVideo_pZBufferSurface = reinterpret_cast<IDirectDrawSurface3 *>(&zBufferSurface);
    g_zVideo_pDDPalette = reinterpret_cast<IDirectDrawPalette *>(&palette);
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_SwSurfaceState.surf = reinterpret_cast<IDirectDrawSurface3 *>(&swSurface);
    g_zVideo_SwSurfaceState.pageLockActive = 1;
    g_zVideo_PrimarySurfaceState.surf =
        reinterpret_cast<IDirectDrawSurface3 *>(&primarySurface);
    g_zVideo_PrimarySurfaceState.pageLockActive = 1;
    g_zVideo_DisplayModeSurfaceState.surf =
        reinterpret_cast<IDirectDrawSurface3 *>(&displaySurface);

    int result = 0;
    if (useAtExitWrapper != 0) {
        zVideo::AtExitReleaseAllInterfacesAndSurfaces();
    } else {
        result = zVideo_dd::ReleaseAllInterfacesAndSurfaces();
    }
    const bool ok =
        result == 0 && gFakeComReleaseCalls == 6 &&
        gFakeDirectDrawSurface3ReleaseCalls == 4 &&
        gFakeDirectDrawSurface3ReleaseSurfaces[0] ==
            reinterpret_cast<IDirectDrawSurface3 *>(&zBufferSurface) &&
        gFakeDirectDrawSurface3ReleaseSurfaces[1] ==
            reinterpret_cast<IDirectDrawSurface3 *>(&swSurface) &&
        gFakeDirectDrawSurface3ReleaseSurfaces[2] ==
            reinterpret_cast<IDirectDrawSurface3 *>(&primarySurface) &&
        gFakeDirectDrawSurface3ReleaseSurfaces[3] ==
            reinterpret_cast<IDirectDrawSurface3 *>(&displaySurface) &&
        gFakeDirectDrawSurface3PageUnlockCalls == 2 &&
        gFakeDirectDrawSurface3LastPageUnlockSurface ==
            reinterpret_cast<IDirectDrawSurface3 *>(&primarySurface) &&
        gFakeDirectDrawSurface3LastPageUnlockFlags == 0 &&
        g_zVideo_pD3DMaterial2 == nullptr && g_zVideo_pD3DViewport2 == nullptr &&
        g_zVideo_pD3DDevice == nullptr && g_zVideo_pD3D2 == nullptr &&
        g_zVideo_pClipper == nullptr && g_zVideo_pZBufferSurface == nullptr &&
        g_zVideo_pDDPalette == nullptr && g_zVideo_SwSurfaceState.surf == nullptr &&
        g_zVideo_PrimarySurfaceState.surf == nullptr &&
        g_zVideo_DisplayModeSurfaceState.surf == nullptr &&
        g_zVideo_SwSurfaceState.pageLockActive == 0 &&
        g_zVideo_PrimarySurfaceState.pageLockActive == 0;

    g_zVideo_pD3DMaterial2 = savedMaterial;
    g_zVideo_pD3DViewport2 = savedViewport;
    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_pD3D2 = savedD3D;
    g_zVideo_pClipper = savedClipper;
    g_zVideo_pZBufferSurface = savedZBuffer;
    g_zVideo_pDDPalette = savedPalette;
    g_zVideo_SwSurfaceState = savedSwState;
    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    return ok ? 0 : 1;
}

extern "C" int zvideo_dd_release_all_interfaces_and_surfaces_smoke(void) {
    return RunReleaseAllInterfacesAndSurfacesSmoke(0);
}

extern "C" int zvideo_at_exit_release_all_interfaces_and_surfaces_smoke(void) {
    return RunReleaseAllInterfacesAndSurfacesSmoke(1);
}

extern "C" int zvideo_dd3d_create_device_state_smoke(void) {
    IDirectDraw2 *const savedDirectDraw = g_zVideo_pDirectDraw2;
    IDirectDrawSurface3 *const savedZBuffer = g_zVideo_pZBufferSurface;
    IDirectDrawSurface *const savedZBufferAttach = g_zVideo_pZBufferAttachSurface;
    IDirect3D2 *const savedD3D = g_zVideo_pD3D2;
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    IDirect3DViewport2 *const savedViewport = g_zVideo_pD3DViewport2;
    IDirect3DMaterial2 *const savedMaterial = g_zVideo_pD3DMaterial2;
    zVidD3DDriverRecordPartial *const savedSelectedD3D =
        g_zVideo_pSelectedD3DDeviceInfo;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;
    const zVideo_SurfaceStatePartial savedDisplayState =
        g_zVideo_DisplayModeSurfaceState;
    const int savedClearScreen = g_zVideo_ClearScreenBufferEnabled;
    const int savedPendingWireframe = g_zVideo_PendingWireframeState;
    const int savedCachedFogEnable = g_zVideo_CachedFogEnableRenderState;
    const int savedCachedFogMode = g_zVideo_CachedFogModeLightState;
    const D3DMATERIALHANDLE savedMaterialHandle = g_zVideo_D3DMaterialHandle;
    const D3DDEVICEDESC savedHalDesc = g_zVideo_D3DHalDeviceDesc;
    const D3DDEVICEDESC savedHelDesc = g_zVideo_D3DHelDeviceDesc;
    zVideo_QuadBatchItemPartial savedQuadItems[16];
    std::memcpy(
        savedQuadItems,
        g_zVideo_QuadBatchItemsBase,
        sizeof(savedQuadItems)
    );

    FakeDirectDraw2Object directDraw{};
    FakeDirectDrawSurfaceObject createdSurface{};
    FakeDirectDrawSurface3Object zBufferSurface3{};
    FakeDirectDrawSurface3Object swSurface{};
    FakeD3D2Object d3d{};
    FakeD3DDevice2Object d3dDevice{};
    FakeD3DViewport2Object d3dViewport{};
    FakeD3DMaterial2Object d3dMaterial{};

    InstallFakeDirectDraw2(
        directDraw,
        createdSurface,
        zBufferSurface3
    );
    InstallFakeDirectDrawSurface3(
        zBufferSurface3,
        DD_OK,
        DD_OK,
        DD_OK
    );
    swSurface.vtable = gFakeDirectDrawSurface3VTable;
    InstallFakeD3DDevice2(d3dDevice);
    InstallFakeD3DViewport2(d3dViewport);
    InstallFakeD3DMaterial2(d3dMaterial);
    InstallFakeD3D2(
        d3d,
        reinterpret_cast<IDirect3DDevice2 *>(&d3dDevice),
        reinterpret_cast<IDirect3DViewport2 *>(&d3dViewport),
        reinterpret_cast<IDirect3DMaterial2 *>(&d3dMaterial)
    );
    gFakeDirectDraw2QueryInterfaceValue = reinterpret_cast<void *>(&d3d);
    gFakeD3DMaterial2HandleValue = 0x2468;

    GUID deviceGuid = {0x33445566, 0x7788, 0x99aa, {1, 3, 5, 7, 9, 11, 13, 15}};
    zVidD3DDriverRecordPartial selectedD3D{};
    selectedD3D.pD3DDeviceGuid = &deviceGuid;
    g_zVideo_pSelectedD3DDeviceInfo = &selectedD3D;
    g_zVideo_pDirectDraw2 = reinterpret_cast<IDirectDraw2 *>(&directDraw);
    g_zVideo_pZBufferSurface = nullptr;
    g_zVideo_pZBufferAttachSurface = nullptr;
    g_zVideo_pD3D2 = nullptr;
    g_zVideo_pD3DDevice = nullptr;
    g_zVideo_pD3DViewport2 = nullptr;
    g_zVideo_pD3DMaterial2 = nullptr;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_SwSurfaceState.width = 320;
    g_zVideo_SwSurfaceState.height = 240;
    g_zVideo_SwSurfaceState.surf =
        reinterpret_cast<IDirectDrawSurface3 *>(&swSurface);
    g_zVideo_DisplayModeSurfaceState.width = 800;
    g_zVideo_DisplayModeSurfaceState.height = 600;
    g_zVideo_ClearScreenBufferEnabled = 0;
    g_zVideo_PendingWireframeState = 17;
    g_zVideo_CachedFogEnableRenderState = 0;
    g_zVideo_CachedFogModeLightState = 0;
    g_zVideo_D3DMaterialHandle = 0;

    const int successResult = zVideo_dd3d::CreateDeviceState();
    const bool renderStateOk =
        gFakeD3DSetRenderStateCalls == 11 &&
        gFakeD3DRenderStates[0] == D3DRENDERSTATE_CULLMODE &&
        gFakeD3DRenderStateValues[0] == 1 &&
        gFakeD3DRenderStates[1] == D3DRENDERSTATE_ZENABLE &&
        gFakeD3DRenderStateValues[1] == 1 &&
        gFakeD3DRenderStates[2] == D3DRENDERSTATE_ZFUNC &&
        gFakeD3DRenderStateValues[2] == 7 &&
        gFakeD3DRenderStates[3] == D3DRENDERSTATE_SPECULARENABLE &&
        gFakeD3DRenderStateValues[3] == 0 &&
        gFakeD3DRenderStates[4] == D3DRENDERSTATE_SHADEMODE &&
        gFakeD3DRenderStateValues[4] == 1 &&
        gFakeD3DRenderStates[5] == D3DRENDERSTATE_TEXTUREPERSPECTIVE &&
        gFakeD3DRenderStateValues[5] == 1 &&
        gFakeD3DRenderStates[6] == D3DRENDERSTATE_TEXTUREMAG &&
        gFakeD3DRenderStateValues[6] == 2 &&
        gFakeD3DRenderStates[7] == D3DRENDERSTATE_TEXTUREMIN &&
        gFakeD3DRenderStateValues[7] == 2 &&
        gFakeD3DRenderStates[8] == D3DRENDERSTATE_SRCBLEND &&
        gFakeD3DRenderStateValues[8] == 5 &&
        gFakeD3DRenderStates[9] == D3DRENDERSTATE_DESTBLEND &&
        gFakeD3DRenderStateValues[9] == 6 &&
        gFakeD3DRenderStates[10] == D3DRENDERSTATE_FOGENABLE &&
        gFakeD3DRenderStateValues[10] == 1;
    const bool successOk =
        successResult == 0 &&
        g_zVideo_ClearScreenBufferEnabled == 1 &&
        gFakeDirectDraw2CreateSurfaceCalls == 1 &&
        gFakeDirectDraw2LastCreateSurfaceOut ==
            (IDirectDrawSurface **)(&g_zVideo_pZBufferSurface) &&
        gFakeDirectDraw2LastCreateSurfaceOuter == nullptr &&
        gFakeDirectDraw2CreateSurfaceDescs[0].dwSize == sizeof(DDSURFACEDESC) &&
        gFakeDirectDraw2CreateSurfaceDescs[0].dwFlags == 0x47 &&
        gFakeDirectDraw2CreateSurfaceDescs[0].dwWidth == 320 &&
        gFakeDirectDraw2CreateSurfaceDescs[0].dwHeight == 240 &&
        gFakeDirectDraw2CreateSurfaceDescs[0].ddsCaps.dwCaps == 0x24000 &&
        gFakeDirectDraw2CreateSurfaceDescs[0].dwMipMapCount == 0x10 &&
        gFakeDirectDrawSurfaceQueryInterfaceCalls == 1 &&
        IsEqualGUID(
            *gFakeDirectDrawSurfaceLastQueryInterfaceIid,
            IID_IDirectDrawSurface
        ) &&
        gFakeDirectDrawSurfaceLastQueryInterfaceOut ==
            (void **)(&g_zVideo_pZBufferAttachSurface) &&
        gFakeDirectDrawSurface3AddAttachedSurfaceCalls == 1 &&
        gFakeDirectDrawSurface3LastAddAttachedSelf ==
            reinterpret_cast<IDirectDrawSurface3 *>(&swSurface) &&
        gFakeDirectDrawSurface3LastAttachedSurfaceArg ==
            reinterpret_cast<IDirectDrawSurface3 *>(g_zVideo_pZBufferAttachSurface) &&
        gFakeDirectDraw2QueryInterfaceCalls == 1 &&
        IsEqualGUID(*gFakeDirectDraw2LastQueryInterfaceIid, IID_IDirect3D2) &&
        gFakeDirectDraw2LastQueryInterfaceOut == (void **)(&g_zVideo_pD3D2) &&
        gFakeD3D2CreateDeviceCalls == 1 &&
        IsEqualGUID(*gFakeD3D2LastCreateDeviceGuid, deviceGuid) &&
        gFakeD3D2LastCreateDeviceSurface ==
            reinterpret_cast<IDirectDrawSurface *>(g_zVideo_SwSurfaceState.surf) &&
        gFakeD3D2LastCreateDeviceOut == &g_zVideo_pD3DDevice &&
        gFakeD3D2CreateViewportCalls == 1 &&
        gFakeD3D2LastCreateViewportOut == &g_zVideo_pD3DViewport2 &&
        gFakeD3D2LastCreateViewportOuter == nullptr &&
        gFakeD3DAddViewportCalls == 1 &&
        gFakeD3DLastAddViewport == g_zVideo_pD3DViewport2 &&
        gFakeD3DViewport2SetViewport2Calls == 1 &&
        gFakeD3DViewport2LastViewportValue.dwSize == sizeof(D3DVIEWPORT2) &&
        gFakeD3DViewport2LastViewportValue.dwWidth == 800 &&
        gFakeD3DViewport2LastViewportValue.dwHeight == 600 &&
        gFakeD3DViewport2LastViewportValue.dvClipWidth == 800.0f &&
        gFakeD3DViewport2LastViewportValue.dvClipHeight == 600.0f &&
        gFakeD3DViewport2LastViewportValue.dvMaxZ == 1.0f &&
        gFakeD3DSetCurrentViewportCalls == 1 &&
        gFakeD3DLastSetCurrentViewport == g_zVideo_pD3DViewport2 &&
        gFakeD3D2CreateMaterialCalls == 1 &&
        gFakeD3D2LastCreateMaterialOut == &g_zVideo_pD3DMaterial2 &&
        gFakeD3D2LastCreateMaterialOuter == nullptr &&
        gFakeD3DMaterial2SetMaterialCalls == 1 &&
        gFakeD3DMaterial2LastMaterialValue.dwSize == sizeof(D3DMATERIAL) &&
        gFakeD3DMaterial2LastMaterialValue.diffuse.r == 0.0f &&
        gFakeD3DMaterial2LastMaterialValue.ambient.r == 1.0f &&
        gFakeD3DMaterial2LastMaterialValue.ambient.g == 1.0f &&
        gFakeD3DMaterial2LastMaterialValue.ambient.b == 1.0f &&
        gFakeD3DMaterial2LastMaterialValue.dwRampSize == 0x100 &&
        gFakeD3DMaterial2GetHandleCalls == 1 &&
        gFakeD3DMaterial2LastGetHandleDevice == g_zVideo_pD3DDevice &&
        gFakeD3DMaterial2LastGetHandleOut == &g_zVideo_D3DMaterialHandle &&
        g_zVideo_D3DMaterialHandle == 0x2468 &&
        gFakeD3DViewport2SetBackgroundCalls == 1 &&
        gFakeD3DViewport2LastBackground == 0x2468 &&
        gFakeD3DGetCapsCalls == 1 &&
        gFakeD3DLastGetCapsHalDesc == &g_zVideo_D3DHalDeviceDesc &&
        gFakeD3DLastGetCapsHelDesc == &g_zVideo_D3DHelDeviceDesc &&
        gFakeD3DLastGetCapsHalDescValue.dwSize == sizeof(D3DDEVICEDESC) &&
        gFakeD3DLastGetCapsHelDescValue.dwSize == sizeof(D3DDEVICEDESC) &&
        renderStateOk &&
        gFakeD3DSetLightStateCalls == 1 &&
        gFakeD3DLightStates[0] == D3DLIGHTSTATE_FOGMODE &&
        gFakeD3DLightStateValues[0] == D3DFOG_LINEAR &&
        g_zVideo_PendingWireframeState == -1 &&
        g_zVideo_CachedFogEnableRenderState == 1 &&
        g_zVideo_CachedFogModeLightState == D3DFOG_LINEAR &&
        g_zVideo_QuadBatchItemsBase[0].vertices[0].sz == 0.99000001f &&
        g_zVideo_QuadBatchItemsBase[15].vertices[3].rhw == 0.99000001f;

    InstallFakeDirectDraw2(
        directDraw,
        createdSurface,
        zBufferSurface3
    );
    InstallFakeDirectDrawSurface3(
        zBufferSurface3,
        DD_OK,
        DD_OK,
        DD_OK
    );
    swSurface.vtable = gFakeDirectDrawSurface3VTable;
    InstallFakeD3DDevice2(d3dDevice);
    InstallFakeD3DViewport2(d3dViewport);
    InstallFakeD3DMaterial2(d3dMaterial);
    InstallFakeD3D2(
        d3d,
        reinterpret_cast<IDirect3DDevice2 *>(&d3dDevice),
        reinterpret_cast<IDirect3DViewport2 *>(&d3dViewport),
        reinterpret_cast<IDirect3DMaterial2 *>(&d3dMaterial)
    );
    gFakeDirectDraw2QueryInterfaceValue = reinterpret_cast<void *>(&d3d);
    gFakeDirectDraw2CreateSurfaceResult = DDERR_GENERIC;
    g_zVideo_pDirectDraw2 = reinterpret_cast<IDirectDraw2 *>(&directDraw);
    g_zVideo_pSelectedD3DDeviceInfo = &selectedD3D;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.width = 320;
    g_zVideo_SwSurfaceState.height = 240;
    g_zVideo_SwSurfaceState.surf =
        reinterpret_cast<IDirectDrawSurface3 *>(&swSurface);

    const int createSurfaceFailureResult = zVideo_dd3d::CreateDeviceState();
    const bool createSurfaceFailureOk =
        createSurfaceFailureResult != 0 &&
        gFakeDirectDraw2CreateSurfaceCalls == 1 &&
        gFakeDirectDrawSurfaceQueryInterfaceCalls == 0 &&
        gFakeDirectDraw2QueryInterfaceCalls == 0 &&
        gFakeD3D2CreateDeviceCalls == 0;

    g_zVideo_pDirectDraw2 = savedDirectDraw;
    g_zVideo_pZBufferSurface = savedZBuffer;
    g_zVideo_pZBufferAttachSurface = savedZBufferAttach;
    g_zVideo_pD3D2 = savedD3D;
    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_pD3DViewport2 = savedViewport;
    g_zVideo_pD3DMaterial2 = savedMaterial;
    g_zVideo_pSelectedD3DDeviceInfo = savedSelectedD3D;
    g_zVideo_SwSurfaceState = savedSwState;
    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    g_zVideo_ClearScreenBufferEnabled = savedClearScreen;
    g_zVideo_PendingWireframeState = savedPendingWireframe;
    g_zVideo_CachedFogEnableRenderState = savedCachedFogEnable;
    g_zVideo_CachedFogModeLightState = savedCachedFogMode;
    g_zVideo_D3DMaterialHandle = savedMaterialHandle;
    g_zVideo_D3DHalDeviceDesc = savedHalDesc;
    g_zVideo_D3DHelDeviceDesc = savedHelDesc;
    std::memcpy(
        g_zVideo_QuadBatchItemsBase,
        savedQuadItems,
        sizeof(savedQuadItems)
    );

    return successOk && createSurfaceFailureOk ? 0 : 1;
}

extern "C" int zvideo_dd_verify_surface_state_locking_smoke(void) {
    zVideo_SurfaceLockVerifier *const savedVerifier = g_zVideo_pSurfaceLockVerifier;
    const unsigned char savedFlags = g_zVideo_SurfaceLockVerifyFlags;

    InstallFakeSurfaceLockVerifier();
    g_zVideo_SurfaceLockVerifyFlags = 0;
    zVideo_dd::VerifySurfaceStateLocking(0x12345678);
    if (gFakeSurfaceLockVerifierVerifyCalls != 0) {
        g_zVideo_pSurfaceLockVerifier = savedVerifier;
        g_zVideo_SurfaceLockVerifyFlags = savedFlags;
        return 1;
    }

    g_zVideo_SurfaceLockVerifyFlags = 0x20;
    zVideo_dd::VerifySurfaceStateLocking(0x12345678);
    const bool ok =
        gFakeSurfaceLockVerifierVerifyCalls == 1 &&
        gFakeSurfaceLockVerifierLastArgs.size ==
            sizeof(gFakeSurfaceLockVerifierLastArgs) &&
        gFakeSurfaceLockVerifierLastArgs.callerContext == 0x12345678;

    g_zVideo_pSurfaceLockVerifier = savedVerifier;
    g_zVideo_SurfaceLockVerifyFlags = savedFlags;
    return ok ? 0 : 2;
}

extern "C" int zvideo_dd_teardown_video_subsystem_smoke(void) {
    FakeDirectDrawSurface3Object pageUnlockSurface{};
    InstallFakeDirectDrawSurface3(pageUnlockSurface, DD_OK, DD_OK, DD_OK);
    FakeDirectDraw2Object directDraw{};
    FakeDirectDrawSurfaceObject unusedCreatedSurface{};
    FakeDirectDrawSurface3Object unusedSurface3{};
    InstallFakeDirectDraw2(
        directDraw,
        unusedCreatedSurface,
        unusedSurface3
    );

    IDirect3DMaterial2 *const savedMaterial = g_zVideo_pD3DMaterial2;
    IDirect3DViewport2 *const savedViewport = g_zVideo_pD3DViewport2;
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    IDirect3D2 *const savedD3D = g_zVideo_pD3D2;
    IDirectDrawClipper *const savedClipper = g_zVideo_pClipper;
    IDirectDraw2 *const savedDirectDraw = g_zVideo_pDirectDraw2;
    IDirectDrawSurface3 *const savedZBuffer = g_zVideo_pZBufferSurface;
    IDirectDrawSurface3 *const savedPageUnlock = g_zVideo_pPageUnlockSurface;
    IDirectDrawPalette *const savedPalette = g_zVideo_pDDPalette;
    zVideo_SurfaceLockVerifier *const savedVerifier = g_zVideo_pSurfaceLockVerifier;
    const int savedVerifyContext = g_zVideo_SurfaceLockVerifyContext;
    const unsigned char savedVerifyFlags = g_zVideo_SurfaceLockVerifyFlags;
    const HWND savedHwnd = g_zVideo_hWnd;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;
    const zVideo_SurfaceStatePartial savedPrimaryState = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial savedDisplayState = g_zVideo_DisplayModeSurfaceState;

    g_zVideo_pD3DMaterial2 = nullptr;
    g_zVideo_pD3DViewport2 = nullptr;
    g_zVideo_pD3DDevice = nullptr;
    g_zVideo_pD3D2 = nullptr;
    g_zVideo_pClipper = nullptr;
    g_zVideo_pZBufferSurface = nullptr;
    g_zVideo_pDDPalette = nullptr;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_pPageUnlockSurface =
        reinterpret_cast<IDirectDrawSurface3 *>(&pageUnlockSurface);
    InstallFakeSurfaceLockVerifier();
    g_zVideo_SurfaceLockVerifyContext = 0x13579;
    g_zVideo_SurfaceLockVerifyFlags = 0x20;
    g_zVideo_pDirectDraw2 = reinterpret_cast<IDirectDraw2 *>(&directDraw);
    g_zVideo_hWnd = reinterpret_cast<HWND>(0x2468);

    zVideo_dd::TeardownVideoSubsystem();
    const bool ok =
        gFakeDirectDrawSurface3PageUnlockCalls == 1 &&
        gFakeDirectDrawSurface3LastPageUnlockSurface ==
            reinterpret_cast<IDirectDrawSurface3 *>(&pageUnlockSurface) &&
        gFakeDirectDrawSurface3LastPageUnlockFlags == 0 &&
        gFakeDirectDrawSurface3ReleaseCalls == 1 &&
        gFakeDirectDrawSurface3ReleaseSurfaces[0] ==
            reinterpret_cast<IDirectDrawSurface3 *>(&pageUnlockSurface) &&
        gFakeSurfaceLockVerifierVerifyCalls == 1 &&
        gFakeSurfaceLockVerifierLastArgs.callerContext == 0x13579 &&
        gFakeSurfaceLockVerifierReleaseCalls == 1 &&
        gFakeDirectDraw2SetCooperativeLevelCalls == 1 &&
        gFakeDirectDraw2LastSetCooperativeHwnd == reinterpret_cast<HWND>(0x2468) &&
        gFakeDirectDraw2LastSetCooperativeFlags == 8 &&
        gFakeDirectDraw2ReleaseCalls == 1 &&
        g_zVideo_pPageUnlockSurface == nullptr &&
        g_zVideo_pSurfaceLockVerifier == nullptr &&
        g_zVideo_pDirectDraw2 == nullptr;

    g_zVideo_pD3DMaterial2 = savedMaterial;
    g_zVideo_pD3DViewport2 = savedViewport;
    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_pD3D2 = savedD3D;
    g_zVideo_pClipper = savedClipper;
    g_zVideo_pDirectDraw2 = savedDirectDraw;
    g_zVideo_pZBufferSurface = savedZBuffer;
    g_zVideo_pPageUnlockSurface = savedPageUnlock;
    g_zVideo_pDDPalette = savedPalette;
    g_zVideo_pSurfaceLockVerifier = savedVerifier;
    g_zVideo_SurfaceLockVerifyContext = savedVerifyContext;
    g_zVideo_SurfaceLockVerifyFlags = savedVerifyFlags;
    g_zVideo_hWnd = savedHwnd;
    g_zVideo_SwSurfaceState = savedSwState;
    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    return ok ? 0 : 1;
}

extern "C" int zvideo_dd_set_display_mode_smoke(void) {
    IDirectDraw2 *const savedDirectDraw = g_zVideo_pDirectDraw2;
    const HWND savedHwnd = g_zVideo_hWnd;
    const zVideo_SurfaceStatePartial savedDisplayState = g_zVideo_DisplayModeSurfaceState;
    const int savedDisplayModeBpp = g_zVideo_DisplayModeBpp;

    FakeDirectDraw2Object directDraw{};
    FakeDirectDrawSurfaceObject baseSurface{};
    FakeDirectDrawSurface3Object surface3{};

    InstallFakeDirectDraw2(
        directDraw,
        baseSurface,
        surface3
    );
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_zVideo_hWnd = reinterpret_cast<HWND>(0x1357);
    g_zVideo_DisplayModeSurfaceState.width = 800;
    g_zVideo_DisplayModeSurfaceState.height = 600;
    g_zVideo_DisplayModeBpp = 16;
    const bool successOk =
        zVideo_dd::SetDisplayMode() == 1 &&
        gFakeDirectDraw2SetCooperativeLevelCalls == 1 &&
        gFakeDirectDraw2LastSetCooperativeHwnd == reinterpret_cast<HWND>(0x1357) &&
        gFakeDirectDraw2LastSetCooperativeFlags == 0x13 &&
        gFakeDirectDraw2SetDisplayModeCalls == 1 &&
        gFakeDirectDraw2LastDisplayModeWidth == 800 &&
        gFakeDirectDraw2LastDisplayModeHeight == 600 &&
        gFakeDirectDraw2LastDisplayModeBpp == 16 &&
        gFakeDirectDraw2LastDisplayModeRefreshRate == 0 &&
        gFakeDirectDraw2LastDisplayModeFlags == 0;

    InstallFakeDirectDraw2(
        directDraw,
        baseSurface,
        surface3
    );
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_zVideo_hWnd = reinterpret_cast<HWND>(0x2468);
    gFakeDirectDraw2SetCooperativeLevelResult = DDERR_GENERIC;
    g_zVideo_DisplayModeSurfaceState.width = 1024;
    g_zVideo_DisplayModeSurfaceState.height = 768;
    g_zVideo_DisplayModeBpp = 32;
    const bool cooperativeFailureOk =
        zVideo_dd::SetDisplayMode() == 0 &&
        gFakeDirectDraw2SetCooperativeLevelCalls == 1 &&
        gFakeDirectDraw2LastSetCooperativeHwnd == reinterpret_cast<HWND>(0x2468) &&
        gFakeDirectDraw2LastSetCooperativeFlags == 0x13 &&
        gFakeDirectDraw2SetDisplayModeCalls == 0;

    InstallFakeDirectDraw2(
        directDraw,
        baseSurface,
        surface3
    );
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_zVideo_hWnd = reinterpret_cast<HWND>(0x369c);
    gFakeDirectDraw2SetDisplayModeResult = DDERR_GENERIC;
    g_zVideo_DisplayModeSurfaceState.width = 640;
    g_zVideo_DisplayModeSurfaceState.height = 480;
    g_zVideo_DisplayModeBpp = 8;
    const bool displayFailureOk =
        zVideo_dd::SetDisplayMode() == 0 &&
        gFakeDirectDraw2SetCooperativeLevelCalls == 1 &&
        gFakeDirectDraw2SetDisplayModeCalls == 1 &&
        gFakeDirectDraw2LastDisplayModeWidth == 640 &&
        gFakeDirectDraw2LastDisplayModeHeight == 480 &&
        gFakeDirectDraw2LastDisplayModeBpp == 8 &&
        gFakeDirectDraw2LastDisplayModeRefreshRate == 0 &&
        gFakeDirectDraw2LastDisplayModeFlags == 0;

    g_zVideo_pDirectDraw2 = savedDirectDraw;
    g_zVideo_hWnd = savedHwnd;
    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    g_zVideo_DisplayModeBpp = savedDisplayModeBpp;
    return successOk && cooperativeFailureOk && displayFailureOk ? 0 : 1;
}

extern "C" int zvideo_dd_set_video_mode_smoke(void) {
    CodeFunctionPatch setDisplayModePatch{};
    CodeFunctionPatch restorePatch{};
    CodeFunctionPatch releasePatch{};
    CodeFunctionPatch createSurfacesPatch{};
    CodeFunctionPatch createDevicePatch{};
    CodeFunctionPatch verifyLocksPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(zVideo_dd::SetDisplayMode),
            reinterpret_cast<void *>(FakeSetVideoMode_SetDisplayMode),
            setDisplayModePatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(zVideo_dd::RestoreDisplaySurfaces),
            reinterpret_cast<void *>(FakeSetVideoMode_RestoreDisplaySurfaces),
            restorePatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(zVideo_dd::ReleaseAllInterfacesAndSurfaces),
            reinterpret_cast<void *>(FakeSetVideoMode_ReleaseAllInterfacesAndSurfaces),
            releasePatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(zVideo_dd::CreateFullscreenSurfacesForRenderer),
            reinterpret_cast<void *>(FakeSetVideoMode_CreateFullscreenSurfacesForRenderer),
            createSurfacesPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(zVideo_dd3d::CreateDeviceState),
            reinterpret_cast<void *>(FakeSetVideoMode_CreateDeviceState),
            createDevicePatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(zVideo_dd::VerifyFullscreenSurfaceLocks),
            reinterpret_cast<void *>(FakeSetVideoMode_VerifyFullscreenSurfaceLocks),
            verifyLocksPatch
        )) {
        RestoreFunctionPatch(verifyLocksPatch);
        RestoreFunctionPatch(createDevicePatch);
        RestoreFunctionPatch(createSurfacesPatch);
        RestoreFunctionPatch(releasePatch);
        RestoreFunctionPatch(restorePatch);
        RestoreFunctionPatch(setDisplayModePatch);
        return 1;
    }

    const int savedRendererType = g_zVideo_RendererType;

    ResetSetVideoModeCapture();
    g_zVideo_RendererType = 0;
    const int softwareResult = zVideo_dd::SetVideoMode(77);
    const bool softwareOk =
        softwareResult == 0 &&
        gFakeSetVideoModeStepCount == 6 &&
        gFakeSetVideoModeSteps[0] == 1 &&
        gFakeSetVideoModeSteps[1] == 2 &&
        gFakeSetVideoModeSteps[2] == 3 &&
        gFakeSetVideoModeSteps[3] == 4 &&
        gFakeSetVideoModeSteps[4] == 2 &&
        gFakeSetVideoModeSteps[5] == 6 &&
        gFakeSetVideoModeRestoreCalls == 2;

    ResetSetVideoModeCapture();
    g_zVideo_RendererType = 1;
    const int hardwareResult = zVideo_dd::SetVideoMode(88);
    const bool hardwareOk =
        hardwareResult == 0 &&
        gFakeSetVideoModeStepCount == 7 &&
        gFakeSetVideoModeSteps[0] == 1 &&
        gFakeSetVideoModeSteps[1] == 2 &&
        gFakeSetVideoModeSteps[2] == 3 &&
        gFakeSetVideoModeSteps[3] == 4 &&
        gFakeSetVideoModeSteps[4] == 5 &&
        gFakeSetVideoModeSteps[5] == 2 &&
        gFakeSetVideoModeSteps[6] == 6 &&
        gFakeSetVideoModeRestoreCalls == 2;

    ResetSetVideoModeCapture();
    g_zVideo_RendererType = 1;
    gFakeSetVideoModeSetDisplayModeResult = 0;
    const int setDisplayFailureResult = zVideo_dd::SetVideoMode(99);
    const bool setDisplayFailureOk =
        setDisplayFailureResult == 1 &&
        gFakeSetVideoModeStepCount == 1 &&
        gFakeSetVideoModeSteps[0] == 1;

    ResetSetVideoModeCapture();
    g_zVideo_RendererType = 1;
    gFakeSetVideoModeCreateDeviceResult = 1;
    const int createDeviceFailureResult = zVideo_dd::SetVideoMode(100);
    const bool createDeviceFailureOk =
        createDeviceFailureResult == 1 &&
        gFakeSetVideoModeStepCount == 5 &&
        gFakeSetVideoModeSteps[4] == 5 &&
        gFakeSetVideoModeRestoreCalls == 1;

    g_zVideo_RendererType = savedRendererType;
    RestoreFunctionPatch(verifyLocksPatch);
    RestoreFunctionPatch(createDevicePatch);
    RestoreFunctionPatch(createSurfacesPatch);
    RestoreFunctionPatch(releasePatch);
    RestoreFunctionPatch(restorePatch);
    RestoreFunctionPatch(setDisplayModePatch);
    return softwareOk && hardwareOk && setDisplayFailureOk && createDeviceFailureOk
               ? 0
               : 2;
}

extern "C" int zvideo_dd_restore_display_surfaces_smoke(void) {
    const zVideo_SurfaceStatePartial savedDisplayState = g_zVideo_DisplayModeSurfaceState;
    const zVideo_SurfaceStatePartial savedPrimaryState = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;

    FakeDirectDrawSurface3Object displaySurface{};
    FakeDirectDrawSurface3Object primarySurface{};
    FakeDirectDrawSurface3Object swSurface{};

    InstallFakeDirectDrawSurface3(
        displaySurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    primarySurface.vtable = gFakeDirectDrawSurface3VTable;
    swSurface.vtable = gFakeDirectDrawSurface3VTable;
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf = (IDirectDrawSurface3 *)(&displaySurface);
    g_zVideo_PrimarySurfaceState.surf = (IDirectDrawSurface3 *)(&primarySurface);
    g_zVideo_SwSurfaceState.surf = (IDirectDrawSurface3 *)(&swSurface);
    const bool allPresentOk =
        zVideo_dd::RestoreDisplaySurfaces() == 0 &&
        gFakeDirectDrawSurface3RestoreCalls == 3 &&
        gFakeDirectDrawSurface3RestoreSurfaces[0] ==
            g_zVideo_DisplayModeSurfaceState.surf &&
        gFakeDirectDrawSurface3RestoreSurfaces[1] == g_zVideo_PrimarySurfaceState.surf &&
        gFakeDirectDrawSurface3RestoreSurfaces[2] == g_zVideo_SwSurfaceState.surf;

    InstallFakeDirectDrawSurface3(
        displaySurface,
        DD_OK,
        DD_OK,
        DDERR_GENERIC
    );
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf = (IDirectDrawSurface3 *)(&displaySurface);
    const bool displayFailureOk =
        zVideo_dd::RestoreDisplaySurfaces() != 0 &&
        gFakeDirectDrawSurface3RestoreCalls == 1 &&
        gFakeDirectDrawSurface3RestoreSurfaces[0] ==
            g_zVideo_DisplayModeSurfaceState.surf;

    InstallFakeDirectDrawSurface3(
        primarySurface,
        DD_OK,
        DD_OK,
        DDERR_GENERIC
    );
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState.surf = (IDirectDrawSurface3 *)(&primarySurface);
    const bool primaryFailureOk =
        zVideo_dd::RestoreDisplaySurfaces() != 0 &&
        gFakeDirectDrawSurface3RestoreCalls == 1 &&
        gFakeDirectDrawSurface3RestoreSurfaces[0] == g_zVideo_PrimarySurfaceState.surf;

    InstallFakeDirectDrawSurface3(
        swSurface,
        DD_OK,
        DD_OK,
        DDERR_GENERIC
    );
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.surf = (IDirectDrawSurface3 *)(&swSurface);
    const bool swFailureOk =
        zVideo_dd::RestoreDisplaySurfaces() != 0 &&
        gFakeDirectDrawSurface3RestoreCalls == 1 &&
        gFakeDirectDrawSurface3RestoreSurfaces[0] == g_zVideo_SwSurfaceState.surf;

    InstallFakeDirectDrawSurface3(
        displaySurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    const bool noSurfaceOk =
        zVideo_dd::RestoreDisplaySurfaces() == 0 &&
        gFakeDirectDrawSurface3RestoreCalls == 0;

    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_SwSurfaceState = savedSwState;
    return allPresentOk && displayFailureOk && primaryFailureOk && swFailureOk &&
                   noSurfaceOk
               ? 0
               : 1;
}

extern "C" int zvideo_dd_init_fullscreen_software_pixel_pack_smoke(void) {
    FakeDirectDrawSurface3Object displaySurface{};
    InstallFakeDirectDrawSurface3(displaySurface, DD_OK, DD_OK, DD_OK);
    IDirectDrawSurface3 *const displayInterface =
        reinterpret_cast<IDirectDrawSurface3 *>(&displaySurface);

    const int savedRShift = g_zVideo_PixelPack.packedBase;
    const int savedGShift = g_zVideo_PixelPack.sumMinus8;
    const int savedBShiftTo8 = g_zVideo_PixelPack.bShiftTo8;
    const int savedRMaskShifted = g_zVideo_PixelPack.rMaskShifted;
    const int savedGMaskShifted = g_zVideo_PixelPack.gMaskShifted;
    const int savedBMaskShifted = g_zVideo_PixelPack.bMaskShifted;
    const int savedRBits = g_zVideo_PixelPack.rBits;
    const int savedGBits = g_zVideo_PixelPack.gBits;
    const int savedBBits = g_zVideo_PixelPack.bBits;
    const unsigned int savedRMask = g_zVideo_PixelPack.rMask;
    const unsigned int savedGMask = g_zVideo_PixelPack.gMask;
    const unsigned int savedBMask = g_zVideo_PixelPack.bMask;

    gFakeDirectDrawSurface3PixelFormat.dwRBitMask = 0xf800;
    gFakeDirectDrawSurface3PixelFormat.dwGBitMask = 0x07e0;
    gFakeDirectDrawSurface3PixelFormat.dwBBitMask = 0x001f;
    const bool firstOk =
        zVideo_dd::InitFullscreenSoftwarePixelPack(displayInterface) == 0 &&
        gFakeDirectDrawSurface3GetPixelFormatCalls == 1 &&
        gFakeDirectDrawSurface3LastPixelFormatInputSize == sizeof(DDPIXELFORMAT) &&
        g_zVideo_PixelPack.rBits == 5 && g_zVideo_PixelPack.gBits == 6 &&
        g_zVideo_PixelPack.bBits == 5 && g_zVideo_PixelPack.rMask == 0xf800 &&
        g_zVideo_PixelPack.gMask == 0x07e0 && g_zVideo_PixelPack.bMask == 0x001f;

    gFakeDirectDrawSurface3PixelFormat.dwRBitMask = 0x7c00;
    gFakeDirectDrawSurface3PixelFormat.dwGBitMask = 0x03e0;
    gFakeDirectDrawSurface3PixelFormat.dwBBitMask = 0x001f;
    const bool secondOk =
        zVideo_dd::InitFullscreenSoftwarePixelPack(displayInterface) == 0 &&
        gFakeDirectDrawSurface3GetPixelFormatCalls == 2 &&
        g_zVideo_PixelPack.rBits == 5 && g_zVideo_PixelPack.gBits == 5 &&
        g_zVideo_PixelPack.bBits == 5 && g_zVideo_PixelPack.rMask == 0x7c00 &&
        g_zVideo_PixelPack.gMask == 0x03e0 && g_zVideo_PixelPack.bMask == 0x001f;

    gFakeDirectDrawSurface3PixelFormat.dwRBitMask = 0xff0000;
    gFakeDirectDrawSurface3PixelFormat.dwGBitMask = 0xff00;
    gFakeDirectDrawSurface3PixelFormat.dwBBitMask = 0x00ff;
    const bool thirdOk =
        zVideo_dd::InitFullscreenSoftwarePixelPack(displayInterface) == 0 &&
        gFakeDirectDrawSurface3GetPixelFormatCalls == 3 &&
        g_zVideo_PixelPack.rBits == 5 && g_zVideo_PixelPack.gBits == 6 &&
        g_zVideo_PixelPack.bBits == 5 && g_zVideo_PixelPack.rMask == 0xff0000 &&
        g_zVideo_PixelPack.gMask == 0xff00 && g_zVideo_PixelPack.bMask == 0x00ff;

    g_zVideo_PixelPack.packedBase = savedRShift;
    g_zVideo_PixelPack.sumMinus8 = savedGShift;
    g_zVideo_PixelPack.bShiftTo8 = savedBShiftTo8;
    g_zVideo_PixelPack.rMaskShifted = savedRMaskShifted;
    g_zVideo_PixelPack.gMaskShifted = savedGMaskShifted;
    g_zVideo_PixelPack.bMaskShifted = savedBMaskShifted;
    g_zVideo_PixelPack.rBits = savedRBits;
    g_zVideo_PixelPack.gBits = savedGBits;
    g_zVideo_PixelPack.bBits = savedBBits;
    g_zVideo_PixelPack.rMask = savedRMask;
    g_zVideo_PixelPack.gMask = savedGMask;
    g_zVideo_PixelPack.bMask = savedBMask;
    return firstOk && secondOk && thirdOk ? 0 : 1;
}

extern "C" int zvideo_dd_create_fullscreen_surfaces_for_renderer_smoke(void) {
    CodeFunctionPatch halfResPatch{};
    CodeFunctionPatch softwarePatch{};
    CodeFunctionPatch hardwarePatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo_dd::CreateHalfResBackbufferSurfaces),
            reinterpret_cast<void *>(&FakeCreateFullscreenHalfResSurfaces),
            halfResPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo_dd::CreateFullscreenSoftwareSurfaces),
            reinterpret_cast<void *>(&FakeCreateFullscreenSoftwareSurfaces),
            softwarePatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo_dd::CreateFullscreenHardwareSurfaces),
            reinterpret_cast<void *>(&FakeCreateFullscreenHardwareSurfaces),
            hardwarePatch
        )) {
        RestoreFunctionPatch(hardwarePatch);
        RestoreFunctionPatch(softwarePatch);
        RestoreFunctionPatch(halfResPatch);
        return 1;
    }

    const int savedUseHalfRes = g_zVideo_UseHalfResBackbuffer;
    const int savedRendererType = g_zVideo_RendererType;
    gFakeCreateFullscreenHalfResCalls = 0;
    gFakeCreateFullscreenSoftwareCalls = 0;
    gFakeCreateFullscreenHardwareCalls = 0;

    g_zVideo_UseHalfResBackbuffer = 1;
    g_zVideo_RendererType = 1;
    const int halfResResult = zVideo_dd::CreateFullscreenSurfacesForRenderer();

    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_RendererType = 1;
    const int hardwareResult = zVideo_dd::CreateFullscreenSurfacesForRenderer();

    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_RendererType = 0;
    const int softwareResult = zVideo_dd::CreateFullscreenSurfacesForRenderer();

    const bool ok =
        halfResResult == 11 &&
        hardwareResult == 33 &&
        softwareResult == 22 &&
        gFakeCreateFullscreenHalfResCalls == 1 &&
        gFakeCreateFullscreenHardwareCalls == 1 &&
        gFakeCreateFullscreenSoftwareCalls == 1;

    g_zVideo_UseHalfResBackbuffer = savedUseHalfRes;
    g_zVideo_RendererType = savedRendererType;
    RestoreFunctionPatch(hardwarePatch);
    RestoreFunctionPatch(softwarePatch);
    RestoreFunctionPatch(halfResPatch);
    return ok ? 0 : 2;
}

extern "C" int zvideo_dd_create_half_res_backbuffer_surfaces_smoke(void) {
    FakeDirectDraw2Object directDraw{};
    FakeDirectDrawSurfaceObject createdSurface{};
    FakeDirectDrawSurface3Object displaySurface{};
    FakeDirectDrawSurface3Object primarySurface{};
    FakeDirectDrawClipperObject clipper{};
    InstallFakeDirectDraw2(
        directDraw,
        createdSurface,
        displaySurface
    );
    InstallFakeDirectDrawSurface3(displaySurface, DD_OK, DD_OK, DD_OK);
    primarySurface.vtable = gFakeDirectDrawSurface3VTable;
    clipper.vtable = gFakeDirectDrawClipperVTable;

    IDirectDrawSurface3 *const displayInterface =
        reinterpret_cast<IDirectDrawSurface3 *>(&displaySurface);
    IDirectDrawSurface3 *const primaryInterface =
        reinterpret_cast<IDirectDrawSurface3 *>(&primarySurface);
    IDirectDrawClipper *const clipperInterface =
        reinterpret_cast<IDirectDrawClipper *>(&clipper);
    gFakeDirectDrawSurface3AttachedSurface = primaryInterface;
    gFakeDirectDraw2CreatedClipper = clipperInterface;
    gFakeDirectDrawSurface3PixelFormat.dwRBitMask = 0xf800;
    gFakeDirectDrawSurface3PixelFormat.dwGBitMask = 0x07e0;
    gFakeDirectDrawSurface3PixelFormat.dwBBitMask = 0x001f;

    IDirectDraw2 *const savedDirectDraw = g_zVideo_pDirectDraw2;
    IDirectDrawClipper *const savedClipper = g_zVideo_pClipper;
    const zVideo_SurfaceStatePartial savedDisplayState = g_zVideo_DisplayModeSurfaceState;
    const zVideo_SurfaceStatePartial savedPrimaryState = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;
    const int savedPrimaryAttached = g_zVideo_PrimaryHasAttachedBackbuffer;
    const int savedActiveRendererPath = g_zVideo_ActiveRendererPath;
    const int savedFullscreenOption = g_zVideo_FullscreenOption;
    zVidHwApiDeviceRecordPartial *const savedSelectedDevice =
        g_zVideo_pSelectedHwApiDeviceRecord;
    const HWND savedHwnd = g_zVideo_hWnd;
    const int savedRBits = g_zVideo_PixelPack.rBits;
    const int savedGBits = g_zVideo_PixelPack.gBits;
    const int savedBBits = g_zVideo_PixelPack.bBits;
    const unsigned int savedRMask = g_zVideo_PixelPack.rMask;
    const unsigned int savedGMask = g_zVideo_PixelPack.gMask;
    const unsigned int savedBMask = g_zVideo_PixelPack.bMask;

    zVidHwApiDeviceRecordPartial selectedDevice{};
    selectedDevice.m_deviceFeatureFlags = 0;
    g_zVideo_pDirectDraw2 = reinterpret_cast<IDirectDraw2 *>(&directDraw);
    g_zVideo_pClipper = nullptr;
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.width = 320;
    g_zVideo_SwSurfaceState.height = 240;
    g_zVideo_PrimaryHasAttachedBackbuffer = 0;
    g_zVideo_ActiveRendererPath = 0;
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    g_zVideo_hWnd = reinterpret_cast<HWND>(0x4321);

    const int result = zVideo_dd::CreateHalfResBackbufferSurfaces();
    const bool ok =
        result == 0 && gFakeDirectDraw2CreateSurfaceCalls == 2 &&
        gFakeDirectDraw2CreateSurfaceDescs[0].dwSize ==
            sizeof(DDSURFACEDESC) &&
        gFakeDirectDraw2CreateSurfaceDescs[0].dwBackBufferCount == 1 &&
        gFakeDirectDraw2CreateSurfaceDescs[0].dwFlags == 0x21 &&
        gFakeDirectDraw2CreateSurfaceDescs[0].ddsCaps.dwCaps == 0x218 &&
        gFakeDirectDraw2CreateSurfaceDescs[1].dwSize ==
            sizeof(DDSURFACEDESC) &&
        gFakeDirectDraw2CreateSurfaceDescs[1].dwFlags == 0x07 &&
        gFakeDirectDraw2CreateSurfaceDescs[1].dwWidth == 320 &&
        gFakeDirectDraw2CreateSurfaceDescs[1].dwHeight == 240 &&
        gFakeDirectDrawSurfaceQueryInterfaceCalls == 2 &&
        gFakeDirectDrawSurfaceReleaseCalls == 2 &&
        gFakeDirectDrawSurface3GetAttachedSurfaceCalls == 1 &&
        gFakeDirectDrawSurface3LastAttachedCapsValue.dwCaps == DDSCAPS_BACKBUFFER &&
        gFakeDirectDrawSurface3GetPixelFormatCalls == 1 &&
        gFakeDirectDraw2CreateClipperCalls == 1 &&
        gFakeDirectDraw2LastCreateClipperFlags == 0 &&
        gFakeDirectDraw2LastCreateClipperOuter == nullptr &&
        gFakeDirectDrawClipperSetHWndCalls == 1 &&
        gFakeDirectDrawClipperLastSetHWndFlags == 0 &&
        gFakeDirectDrawClipperLastSetHWnd == reinterpret_cast<HWND>(0x4321) &&
        gFakeDirectDrawSurface3SetClipperCalls == 1 &&
        gFakeDirectDrawSurface3LastSetClipper == clipperInterface &&
        g_zVideo_DisplayModeSurfaceState.surf == displayInterface &&
        g_zVideo_PrimarySurfaceState.surf == primaryInterface &&
        g_zVideo_SwSurfaceState.surf == displayInterface &&
        g_zVideo_PrimaryHasAttachedBackbuffer == 1 &&
        g_zVideo_pClipper == clipperInterface &&
        g_zVideo_PixelPack.rBits == 5 && g_zVideo_PixelPack.gBits == 6 &&
        g_zVideo_PixelPack.bBits == 5 && g_zVideo_PixelPack.rMask == 0xf800 &&
        g_zVideo_PixelPack.gMask == 0x07e0 && g_zVideo_PixelPack.bMask == 0x001f;

    int failCode = 0;
    if (result != 0) {
        failCode = 2;
    } else if (gFakeDirectDraw2CreateSurfaceCalls != 2) {
        failCode = 3;
    } else if (gFakeDirectDraw2CreateSurfaceDescs[0].dwFlags != 0x21 ||
               gFakeDirectDraw2CreateSurfaceDescs[0].ddsCaps.dwCaps != 0x218) {
        failCode = 4;
    } else if (gFakeDirectDraw2CreateSurfaceDescs[1].dwFlags != 0x07 ||
               gFakeDirectDraw2CreateSurfaceDescs[1].dwWidth != 320 ||
               gFakeDirectDraw2CreateSurfaceDescs[1].dwHeight != 240) {
        failCode = 5;
    } else if (gFakeDirectDrawSurfaceQueryInterfaceCalls != 2 ||
               gFakeDirectDrawSurfaceReleaseCalls != 2) {
        failCode = 6;
    } else if (gFakeDirectDrawSurface3GetAttachedSurfaceCalls != 1 ||
               gFakeDirectDrawSurface3LastAttachedCapsValue.dwCaps != DDSCAPS_BACKBUFFER) {
        failCode = 7;
    } else if (gFakeDirectDrawSurface3GetPixelFormatCalls != 1) {
        failCode = 8;
    } else if (gFakeDirectDraw2CreateClipperCalls != 1 ||
               gFakeDirectDraw2LastCreateClipperFlags != 0 ||
               gFakeDirectDraw2LastCreateClipperOuter != nullptr) {
        failCode = 9;
    } else if (gFakeDirectDrawClipperSetHWndCalls != 1 ||
               gFakeDirectDrawClipperLastSetHWndFlags != 0 ||
               gFakeDirectDrawClipperLastSetHWnd != reinterpret_cast<HWND>(0x4321)) {
        failCode = 10;
    } else if (gFakeDirectDrawSurface3SetClipperCalls != 1 ||
               gFakeDirectDrawSurface3LastSetClipper != clipperInterface) {
        failCode = 11;
    } else if (g_zVideo_DisplayModeSurfaceState.surf != displayInterface ||
               g_zVideo_PrimarySurfaceState.surf != primaryInterface ||
               g_zVideo_SwSurfaceState.surf != displayInterface ||
               g_zVideo_pClipper != clipperInterface) {
        failCode = 12;
    } else if (!ok) {
        failCode = 13;
    }

    g_zVideo_pDirectDraw2 = savedDirectDraw;
    g_zVideo_pClipper = savedClipper;
    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_SwSurfaceState = savedSwState;
    g_zVideo_PrimaryHasAttachedBackbuffer = savedPrimaryAttached;
    g_zVideo_ActiveRendererPath = savedActiveRendererPath;
    g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
    g_zVideo_hWnd = savedHwnd;
    g_zVideo_PixelPack.rBits = savedRBits;
    g_zVideo_PixelPack.gBits = savedGBits;
    g_zVideo_PixelPack.bBits = savedBBits;
    g_zVideo_PixelPack.rMask = savedRMask;
    g_zVideo_PixelPack.gMask = savedGMask;
    g_zVideo_PixelPack.bMask = savedBMask;
    return failCode;
}

extern "C" int zvideo_dd_create_fullscreen_software_surfaces_smoke(void) {
    FakeDirectDraw2Object directDraw{};
    FakeDirectDrawSurfaceObject createdSurface{};
    FakeDirectDrawSurface3Object displaySurface{};
    FakeDirectDrawSurface3Object primarySurface{};
    FakeDirectDrawSurface3Object swSurface{};
    FakeDirectDrawClipperObject clipper{};
    InstallFakeDirectDraw2(
        directDraw,
        createdSurface,
        displaySurface
    );
    InstallFakeDirectDrawSurface3(displaySurface, DD_OK, DD_OK, DD_OK);
    primarySurface.vtable = gFakeDirectDrawSurface3VTable;
    swSurface.vtable = gFakeDirectDrawSurface3VTable;
    clipper.vtable = gFakeDirectDrawClipperVTable;

    IDirectDrawSurface3 *const displayInterface =
        reinterpret_cast<IDirectDrawSurface3 *>(&displaySurface);
    IDirectDrawSurface3 *const primaryInterface =
        reinterpret_cast<IDirectDrawSurface3 *>(&primarySurface);
    IDirectDrawSurface3 *const swInterface =
        reinterpret_cast<IDirectDrawSurface3 *>(&swSurface);
    IDirectDrawClipper *const clipperInterface =
        reinterpret_cast<IDirectDrawClipper *>(&clipper);
    gFakeDirectDrawSurfaceQueryInterfaceValues[0] = displayInterface;
    gFakeDirectDrawSurfaceQueryInterfaceValues[1] = primaryInterface;
    gFakeDirectDrawSurfaceQueryInterfaceValues[2] = swInterface;
    gFakeDirectDrawSurfaceQueryInterfaceValueCount = 3;
    gFakeDirectDraw2CreatedClipper = clipperInterface;
    gFakeDirectDrawSurface3PixelFormat.dwRBitMask = 0xf800;
    gFakeDirectDrawSurface3PixelFormat.dwGBitMask = 0x07e0;
    gFakeDirectDrawSurface3PixelFormat.dwBBitMask = 0x001f;

    IDirectDraw2 *const savedDirectDraw = g_zVideo_pDirectDraw2;
    IDirectDrawClipper *const savedClipper = g_zVideo_pClipper;
    const zVideo_SurfaceStatePartial savedDisplayState = g_zVideo_DisplayModeSurfaceState;
    const zVideo_SurfaceStatePartial savedPrimaryState = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;
    const int savedPrimaryAttached = g_zVideo_PrimaryHasAttachedBackbuffer;
    const int savedActiveRendererPath = g_zVideo_ActiveRendererPath;
    const int savedFullscreenOption = g_zVideo_FullscreenOption;
    zVidHwApiDeviceRecordPartial *const savedSelectedDevice =
        g_zVideo_pSelectedHwApiDeviceRecord;
    const HWND savedHwnd = g_zVideo_hWnd;
    const int savedRShift = g_zVideo_PixelPack.packedBase;
    const int savedGShift = g_zVideo_PixelPack.sumMinus8;
    const int savedBShiftTo8 = g_zVideo_PixelPack.bShiftTo8;
    const int savedRMaskShifted = g_zVideo_PixelPack.rMaskShifted;
    const int savedGMaskShifted = g_zVideo_PixelPack.gMaskShifted;
    const int savedBMaskShifted = g_zVideo_PixelPack.bMaskShifted;
    const int savedRBits = g_zVideo_PixelPack.rBits;
    const int savedGBits = g_zVideo_PixelPack.gBits;
    const int savedBBits = g_zVideo_PixelPack.bBits;
    const unsigned int savedRMask = g_zVideo_PixelPack.rMask;
    const unsigned int savedGMask = g_zVideo_PixelPack.gMask;
    const unsigned int savedBMask = g_zVideo_PixelPack.bMask;

    zVidHwApiDeviceRecordPartial selectedDevice{};
    selectedDevice.m_deviceFeatureFlags = 0;
    g_zVideo_pDirectDraw2 = reinterpret_cast<IDirectDraw2 *>(&directDraw);
    g_zVideo_pClipper = nullptr;
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimaryHasAttachedBackbuffer = 1;
    g_zVideo_ActiveRendererPath = 0;
    g_zVideo_FullscreenOption = 1;
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    g_zVideo_hWnd = reinterpret_cast<HWND>(0x7654);

    const int result = zVideo_dd::CreateFullscreenSoftwareSurfaces();
    const bool ok =
        result == 0 &&
        gFakeDirectDraw2CreateSurfaceCalls == 3 &&
        gFakeDirectDraw2CreateSurfaceDescs[0].dwSize ==
            sizeof(DDSURFACEDESC) &&
        gFakeDirectDraw2CreateSurfaceDescs[0].dwFlags == 1 &&
        gFakeDirectDraw2CreateSurfaceDescs[0].ddsCaps.dwCaps == 0x0a00 &&
        gFakeDirectDraw2CreateSurfaceDescs[1].dwFlags == 7 &&
        gFakeDirectDraw2CreateSurfaceDescs[1].ddsCaps.dwCaps == 0x0840 &&
        gFakeDirectDraw2CreateSurfaceDescs[1].dwWidth == 640 &&
        gFakeDirectDraw2CreateSurfaceDescs[1].dwHeight == 480 &&
        gFakeDirectDraw2CreateSurfaceDescs[2].dwFlags == 7 &&
        gFakeDirectDraw2CreateSurfaceDescs[2].ddsCaps.dwCaps == 0x0840 &&
        gFakeDirectDraw2CreateSurfaceDescs[2].dwWidth == 640 &&
        gFakeDirectDraw2CreateSurfaceDescs[2].dwHeight == 480 &&
        gFakeDirectDrawSurfaceQueryInterfaceCalls == 3 &&
        gFakeDirectDrawSurfaceReleaseCalls == 3 &&
        gFakeDirectDrawSurface3LockCalls == 1 &&
        gFakeDirectDrawSurface3UnlockCalls == 1 &&
        gFakeDirectDrawSurface3ReleaseCalls == 0 &&
        gFakeDirectDrawSurface3GetPixelFormatCalls == 1 &&
        gFakeDirectDraw2CreateClipperCalls == 1 &&
        gFakeDirectDraw2LastCreateClipperFlags == 0 &&
        gFakeDirectDraw2LastCreateClipperOuter == nullptr &&
        gFakeDirectDrawClipperSetHWndCalls == 1 &&
        gFakeDirectDrawClipperLastSetHWndFlags == 0 &&
        gFakeDirectDrawClipperLastSetHWnd == reinterpret_cast<HWND>(0x7654) &&
        gFakeDirectDrawSurface3SetClipperCalls == 1 &&
        gFakeDirectDrawSurface3LastSetClipper == clipperInterface &&
        g_zVideo_DisplayModeSurfaceState.surf == displayInterface &&
        g_zVideo_PrimarySurfaceState.surf == primaryInterface &&
        g_zVideo_SwSurfaceState.surf == swInterface &&
        g_zVideo_DisplayModeSurfaceState.width == 640 &&
        g_zVideo_DisplayModeSurfaceState.height == 480 &&
        g_zVideo_DisplayModeSurfaceState.locked == 0 &&
        g_zVideo_PrimaryHasAttachedBackbuffer == 0 &&
        g_zVideo_pClipper == clipperInterface &&
        g_zVideo_PixelPack.rBits == 5 &&
        g_zVideo_PixelPack.gBits == 6 &&
        g_zVideo_PixelPack.bBits == 5 &&
        g_zVideo_PixelPack.rMask == 0xf800 &&
        g_zVideo_PixelPack.gMask == 0x07e0 &&
        g_zVideo_PixelPack.bMask == 0x001f;

    int failCode = 0;
    if (result != 0) {
        failCode = 2;
    } else if (gFakeDirectDraw2CreateSurfaceCalls != 3) {
        failCode = 3;
    } else if (gFakeDirectDraw2CreateSurfaceDescs[0].dwFlags != 1 ||
               gFakeDirectDraw2CreateSurfaceDescs[0].ddsCaps.dwCaps != 0x0a00) {
        failCode = 4;
    } else if (gFakeDirectDraw2CreateSurfaceDescs[1].dwFlags != 7) {
        failCode = 5;
    } else if (gFakeDirectDraw2CreateSurfaceDescs[1].ddsCaps.dwCaps != 0x0840) {
        failCode = 16;
    } else if (gFakeDirectDraw2CreateSurfaceDescs[1].dwWidth != 640 ||
               gFakeDirectDraw2CreateSurfaceDescs[1].dwHeight != 480) {
        failCode = 17;
    } else if (gFakeDirectDraw2CreateSurfaceDescs[2].dwFlags != 7 ||
               gFakeDirectDraw2CreateSurfaceDescs[2].ddsCaps.dwCaps != 0x0840 ||
               gFakeDirectDraw2CreateSurfaceDescs[2].dwWidth != 640 ||
               gFakeDirectDraw2CreateSurfaceDescs[2].dwHeight != 480) {
        failCode = 6;
    } else if (gFakeDirectDrawSurfaceQueryInterfaceCalls != 3 ||
               gFakeDirectDrawSurfaceReleaseCalls != 3) {
        failCode = 7;
    } else if (gFakeDirectDrawSurface3LockCalls != 1 ||
               gFakeDirectDrawSurface3UnlockCalls != 1 ||
               gFakeDirectDrawSurface3ReleaseCalls != 0) {
        failCode = 8;
    } else if (gFakeDirectDrawSurface3GetPixelFormatCalls != 1) {
        failCode = 9;
    } else if (gFakeDirectDraw2CreateClipperCalls != 1 ||
               gFakeDirectDraw2LastCreateClipperFlags != 0 ||
               gFakeDirectDraw2LastCreateClipperOuter != nullptr) {
        failCode = 10;
    } else if (gFakeDirectDrawClipperSetHWndCalls != 1 ||
               gFakeDirectDrawClipperLastSetHWndFlags != 0 ||
               gFakeDirectDrawClipperLastSetHWnd != reinterpret_cast<HWND>(0x7654)) {
        failCode = 11;
    } else if (gFakeDirectDrawSurface3SetClipperCalls != 1 ||
               gFakeDirectDrawSurface3LastSetClipper != clipperInterface) {
        failCode = 12;
    } else if (g_zVideo_DisplayModeSurfaceState.surf != displayInterface ||
               g_zVideo_PrimarySurfaceState.surf != primaryInterface ||
               g_zVideo_SwSurfaceState.surf != swInterface) {
        failCode = 13;
    } else if (g_zVideo_DisplayModeSurfaceState.width != 640 ||
               g_zVideo_DisplayModeSurfaceState.height != 480 ||
               g_zVideo_DisplayModeSurfaceState.locked != 0 ||
               g_zVideo_PrimaryHasAttachedBackbuffer != 0 ||
               g_zVideo_pClipper != clipperInterface) {
        failCode = 14;
    } else if (!ok) {
        failCode = 15;
    }

    g_zVideo_pDirectDraw2 = savedDirectDraw;
    g_zVideo_pClipper = savedClipper;
    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_SwSurfaceState = savedSwState;
    g_zVideo_PrimaryHasAttachedBackbuffer = savedPrimaryAttached;
    g_zVideo_ActiveRendererPath = savedActiveRendererPath;
    g_zVideo_FullscreenOption = savedFullscreenOption;
    g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
    g_zVideo_hWnd = savedHwnd;
    g_zVideo_PixelPack.packedBase = savedRShift;
    g_zVideo_PixelPack.sumMinus8 = savedGShift;
    g_zVideo_PixelPack.bShiftTo8 = savedBShiftTo8;
    g_zVideo_PixelPack.rMaskShifted = savedRMaskShifted;
    g_zVideo_PixelPack.gMaskShifted = savedGMaskShifted;
    g_zVideo_PixelPack.bMaskShifted = savedBMaskShifted;
    g_zVideo_PixelPack.rBits = savedRBits;
    g_zVideo_PixelPack.gBits = savedGBits;
    g_zVideo_PixelPack.bBits = savedBBits;
    g_zVideo_PixelPack.rMask = savedRMask;
    g_zVideo_PixelPack.gMask = savedGMask;
    g_zVideo_PixelPack.bMask = savedBMask;
    return failCode;
}

extern "C" int zvideo_dd_create_fullscreen_hw_surfaces_smoke(void) {
    FakeDirectDraw2Object directDraw{};
    FakeDirectDrawSurfaceObject createdSurface{};
    FakeDirectDrawSurface3Object displaySurface{};
    FakeDirectDrawSurface3Object primarySurface{};
    FakeDirectDrawSurface3Object swSurface{};
    FakeDirectDrawClipperObject clipper{};
    InstallFakeDirectDraw2(
        directDraw,
        createdSurface,
        displaySurface
    );
    InstallFakeDirectDrawSurface3(displaySurface, DD_OK, DD_OK, DD_OK);
    primarySurface.vtable = gFakeDirectDrawSurface3VTable;
    swSurface.vtable = gFakeDirectDrawSurface3VTable;
    clipper.vtable = gFakeDirectDrawClipperVTable;

    IDirectDrawSurface3 *const displayInterface =
        reinterpret_cast<IDirectDrawSurface3 *>(&displaySurface);
    IDirectDrawSurface3 *const primaryInterface =
        reinterpret_cast<IDirectDrawSurface3 *>(&primarySurface);
    IDirectDrawSurface3 *const swInterface =
        reinterpret_cast<IDirectDrawSurface3 *>(&swSurface);
    IDirectDrawClipper *const clipperInterface =
        reinterpret_cast<IDirectDrawClipper *>(&clipper);
    gFakeDirectDrawSurfaceQueryInterfaceValues[0] = displayInterface;
    gFakeDirectDrawSurfaceQueryInterfaceValues[1] = primaryInterface;
    gFakeDirectDrawSurfaceQueryInterfaceValueCount = 2;
    gFakeDirectDrawSurface3AttachedSurface = swInterface;
    gFakeDirectDraw2CreatedClipper = clipperInterface;
    gFakeDirectDrawSurface3PixelFormat.dwRBitMask = 0xf800;
    gFakeDirectDrawSurface3PixelFormat.dwGBitMask = 0x07e0;
    gFakeDirectDrawSurface3PixelFormat.dwBBitMask = 0x001f;

    IDirectDraw2 *const savedDirectDraw = g_zVideo_pDirectDraw2;
    IDirectDrawClipper *const savedClipper = g_zVideo_pClipper;
    const zVideo_SurfaceStatePartial savedDisplayState = g_zVideo_DisplayModeSurfaceState;
    const zVideo_SurfaceStatePartial savedPrimaryState = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;
    const int savedPrimaryAttached = g_zVideo_PrimaryHasAttachedBackbuffer;
    zVidHwApiDeviceRecordPartial *const savedSelectedDevice =
        g_zVideo_pSelectedHwApiDeviceRecord;
    const HWND savedHwnd = g_zVideo_hWnd;
    const int savedRShift = g_zVideo_PixelPack.packedBase;
    const int savedGShift = g_zVideo_PixelPack.sumMinus8;
    const int savedBShiftTo8 = g_zVideo_PixelPack.bShiftTo8;
    const int savedRMaskShifted = g_zVideo_PixelPack.rMaskShifted;
    const int savedGMaskShifted = g_zVideo_PixelPack.gMaskShifted;
    const int savedBMaskShifted = g_zVideo_PixelPack.bMaskShifted;
    const int savedRBits = g_zVideo_PixelPack.rBits;
    const int savedGBits = g_zVideo_PixelPack.gBits;
    const int savedBBits = g_zVideo_PixelPack.bBits;
    const unsigned int savedRMask = g_zVideo_PixelPack.rMask;
    const unsigned int savedGMask = g_zVideo_PixelPack.gMask;
    const unsigned int savedBMask = g_zVideo_PixelPack.bMask;

    zVidHwApiDeviceRecordPartial selectedDevice{};
    selectedDevice.m_deviceFeatureFlags = 0;
    g_zVideo_pDirectDraw2 = reinterpret_cast<IDirectDraw2 *>(&directDraw);
    g_zVideo_pClipper = nullptr;
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.width = 800;
    g_zVideo_DisplayModeSurfaceState.height = 600;
    g_zVideo_PrimaryHasAttachedBackbuffer = 0;
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    g_zVideo_hWnd = reinterpret_cast<HWND>(0x789a);

    const int result = zVideo_dd::CreateFullscreenHardwareSurfaces();
    const bool ok =
        result == 0 &&
        gFakeDirectDraw2CreateSurfaceCalls == 2 &&
        gFakeDirectDraw2CreateSurfaceDescs[0].dwSize ==
            sizeof(DDSURFACEDESC) &&
        gFakeDirectDraw2CreateSurfaceDescs[0].dwBackBufferCount == 1 &&
        gFakeDirectDraw2CreateSurfaceDescs[0].dwFlags == 0x21 &&
        gFakeDirectDraw2CreateSurfaceDescs[0].ddsCaps.dwCaps == 0x2218 &&
        gFakeDirectDraw2CreateSurfaceDescs[1].dwFlags == 7 &&
        gFakeDirectDraw2CreateSurfaceDescs[1].ddsCaps.dwCaps == 0x0840 &&
        gFakeDirectDraw2CreateSurfaceDescs[1].dwWidth == 800 &&
        gFakeDirectDraw2CreateSurfaceDescs[1].dwHeight == 600 &&
        gFakeDirectDrawSurfaceQueryInterfaceCalls == 2 &&
        gFakeDirectDrawSurfaceReleaseCalls == 2 &&
        gFakeDirectDrawSurface3GetAttachedSurfaceCalls == 1 &&
        gFakeDirectDrawSurface3LastAttachedCapsValue.dwCaps == DDSCAPS_BACKBUFFER &&
        gFakeDirectDrawSurface3GetPixelFormatCalls == 1 &&
        gFakeDirectDraw2CreateClipperCalls == 1 &&
        gFakeDirectDraw2LastCreateClipperFlags == 0 &&
        gFakeDirectDraw2LastCreateClipperOuter == nullptr &&
        gFakeDirectDrawClipperSetHWndCalls == 1 &&
        gFakeDirectDrawClipperLastSetHWndFlags == 0 &&
        gFakeDirectDrawClipperLastSetHWnd == reinterpret_cast<HWND>(0x789a) &&
        gFakeDirectDrawSurface3SetClipperCalls == 1 &&
        gFakeDirectDrawSurface3LastSetClipper == clipperInterface &&
        g_zVideo_DisplayModeSurfaceState.surf == displayInterface &&
        g_zVideo_PrimarySurfaceState.surf == primaryInterface &&
        g_zVideo_SwSurfaceState.surf == swInterface &&
        g_zVideo_DisplayModeSurfaceState.width == 800 &&
        g_zVideo_DisplayModeSurfaceState.height == 600 &&
        g_zVideo_PrimaryHasAttachedBackbuffer == 1 &&
        g_zVideo_pClipper == clipperInterface &&
        g_zVideo_PixelPack.rBits == 5 &&
        g_zVideo_PixelPack.gBits == 6 &&
        g_zVideo_PixelPack.bBits == 5 &&
        g_zVideo_PixelPack.rMask == 0xf800 &&
        g_zVideo_PixelPack.gMask == 0x07e0 &&
        g_zVideo_PixelPack.bMask == 0x001f;

    g_zVideo_pDirectDraw2 = savedDirectDraw;
    g_zVideo_pClipper = savedClipper;
    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_SwSurfaceState = savedSwState;
    g_zVideo_PrimaryHasAttachedBackbuffer = savedPrimaryAttached;
    g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
    g_zVideo_hWnd = savedHwnd;
    g_zVideo_PixelPack.packedBase = savedRShift;
    g_zVideo_PixelPack.sumMinus8 = savedGShift;
    g_zVideo_PixelPack.bShiftTo8 = savedBShiftTo8;
    g_zVideo_PixelPack.rMaskShifted = savedRMaskShifted;
    g_zVideo_PixelPack.gMaskShifted = savedGMaskShifted;
    g_zVideo_PixelPack.bMaskShifted = savedBMaskShifted;
    g_zVideo_PixelPack.rBits = savedRBits;
    g_zVideo_PixelPack.gBits = savedGBits;
    g_zVideo_PixelPack.bBits = savedBBits;
    g_zVideo_PixelPack.rMask = savedRMask;
    g_zVideo_PixelPack.gMask = savedGMask;
    g_zVideo_PixelPack.bMask = savedBMask;
    return ok ? 0 : 1;
}

extern "C" int zvideo_dd_present_display_mode_surface_smoke(void) {
    FakeDirectDrawSurface3Object displaySurface{};
    FakeDirectDrawSurface3Object primarySurface{};
    FakeDirectDrawSurface3Object swSurface{};
    InstallFakeDirectDrawSurface3(displaySurface, DD_OK, DD_OK, DD_OK);
    primarySurface.vtable = gFakeDirectDrawSurface3VTable;
    swSurface.vtable = gFakeDirectDrawSurface3VTable;

    const zVideo_SurfaceStatePartial savedDisplayState = g_zVideo_DisplayModeSurfaceState;
    const zVideo_SurfaceStatePartial savedPrimaryState = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;
    const int savedUseHalfRes = g_zVideo_UseHalfResBackbuffer;
    const int savedHalfResAdjustMode = g_zVideo_HalfResAdjustMode;

    zVidRect32 srcRect{1, 2, 9, 10};
    zVidRect32 dstRect{3, 4, 11, 12};
    IDirectDrawSurface3 *const displayInterface =
        reinterpret_cast<IDirectDrawSurface3 *>(&displaySurface);
    IDirectDrawSurface3 *const primaryInterface =
        reinterpret_cast<IDirectDrawSurface3 *>(&primarySurface);
    IDirectDrawSurface3 *const swInterface =
        reinterpret_cast<IDirectDrawSurface3 *>(&swSurface);

    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf = displayInterface;
    g_zVideo_PrimarySurfaceState.surf = primaryInterface;
    g_zVideo_SwSurfaceState.surf = swInterface;
    g_zVideo_UseHalfResBackbuffer = 1;
    g_zVideo_HalfResAdjustMode = 0;

    if (zVideo_dd::PresentDisplayModeSurface(&srcRect, &dstRect, 0, 0) != 0 ||
        gFakeDirectDrawSurface3BltCalls != 1 ||
        gFakeDirectDrawSurface3LastBltSource != primaryInterface ||
        gFakeDirectDrawSurface3LastBltFlags != (DDBLT_WAIT | DDBLT_ASYNC) ||
        gFakeDirectDrawSurface3LastBltDstRect.left != 3 ||
        gFakeDirectDrawSurface3LastBltSrcRect.top != 2 ||
        gFakeDirectDrawSurface3PageLockCalls != 0 ||
        gFakeDirectDrawSurface3PageUnlockCalls != 0) {
        return 1;
    }

    InstallFakeDirectDrawSurface3(displaySurface, DD_OK, DD_OK, DD_OK);
    primarySurface.vtable = gFakeDirectDrawSurface3VTable;
    swSurface.vtable = gFakeDirectDrawSurface3VTable;
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf = displayInterface;
    g_zVideo_PrimarySurfaceState.surf = primaryInterface;
    g_zVideo_SwSurfaceState.surf = swInterface;
    g_zVideo_PrimarySurfaceState.width = 640;
    g_zVideo_SwSurfaceState.width = 320;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_HalfResAdjustMode = 1;

    if (zVideo_dd::PresentDisplayModeSurface(&srcRect, &dstRect, 1, 0) != 0 ||
        gFakeDirectDrawSurface3PageLockCalls != 1 ||
        gFakeDirectDrawSurface3PageUnlockCalls != 0 ||
        gFakeDirectDrawSurface3LastPageLockSurface != primaryInterface ||
        gFakeDirectDrawSurface3BltCalls != 1 ||
        gFakeDirectDrawSurface3LastBltFlags != DDBLT_ASYNC ||
        g_zVideo_PrimarySurfaceState.surf != swInterface ||
        g_zVideo_SwSurfaceState.surf != primaryInterface ||
        g_zVideo_PrimarySurfaceState.width != 320 ||
        g_zVideo_SwSurfaceState.width != 640) {
        return 2;
    }

    InstallFakeDirectDrawSurface3(displaySurface, DD_OK, DD_OK, DD_OK);
    primarySurface.vtable = gFakeDirectDrawSurface3VTable;
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf = displayInterface;
    g_zVideo_PrimarySurfaceState.surf = primaryInterface;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_HalfResAdjustMode = 1;

    if (zVideo_dd::PresentDisplayModeSurface(&srcRect, &dstRect, 1, 1) != 0 ||
        gFakeDirectDrawSurface3PageLockCalls != 1 ||
        gFakeDirectDrawSurface3PageUnlockCalls != 1 ||
        gFakeDirectDrawSurface3LastPageUnlockSurface != primaryInterface ||
        g_zVideo_PrimarySurfaceState.pageLockActive != 0) {
        return 3;
    }

    InstallFakeDirectDrawSurface3(displaySurface, DD_OK, DD_OK, DD_OK);
    primarySurface.vtable = gFakeDirectDrawSurface3VTable;
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf = displayInterface;
    g_zVideo_PrimarySurfaceState.surf = primaryInterface;
    ConfigureFakeDirectDrawSurface3BltResults(DDERR_SURFACELOST, DD_OK);
    g_zVideo_UseHalfResBackbuffer = 1;
    g_zVideo_HalfResAdjustMode = 0;

    const bool restoredAndRetried =
        zVideo_dd::PresentDisplayModeSurface(&srcRect, &dstRect, 1, 0) == 0 &&
        gFakeDirectDrawSurface3BltCalls == 2 &&
        gFakeDirectDrawSurface3RestoreCalls == 1 &&
        gFakeDirectDrawSurface3LastBltFlags == DDBLT_WAIT;

    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_SwSurfaceState = savedSwState;
    g_zVideo_UseHalfResBackbuffer = savedUseHalfRes;
    g_zVideo_HalfResAdjustMode = savedHalfResAdjustMode;

    return restoredAndRetried ? 0 : 4;
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

extern "C" int zvideo_dd_image_populate_surface_from_heap_pixels_smoke(void) {
    FakeDirectDrawSurface3Object surface{};
    unsigned char lockedPixels[24];
    std::memset(lockedPixels, 0xcc, sizeof(lockedPixels));

    unsigned char *sourcePixels = (unsigned char *)std::malloc(12);
    if (sourcePixels == nullptr) {
        return 1;
    }
    for (int i = 0; i < 12; ++i) {
        sourcePixels[i] = (unsigned char)(i + 1);
    }

    InstallFakeDirectDrawSurface3(
        surface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    gFakeDirectDrawSurface3LockPixels = lockedPixels;
    gFakeDirectDrawSurface3LockPitch = 10;

    zVidImagePartial image{};
    image.width = 3;
    image.height = 2;
    image.pixels = sourcePixels;
    image.surface = (IDirectDrawSurface3 *)(&surface);

    const int successResult = zVideo_dd::Image_PopulateSurfaceFromHeapPixels(&image);
    const bool successOk =
        successResult == 1 &&
        gFakeDirectDrawSurface3LockCalls == 1 &&
        gFakeDirectDrawSurface3LastLockRect == nullptr &&
        gFakeDirectDrawSurface3LastLockFlags == DDLOCK_WAIT &&
        gFakeDirectDrawSurface3LastLockEvent == nullptr &&
        gFakeDirectDrawSurface3LockDescSize == sizeof(DDSURFACEDESC) &&
        gFakeDirectDrawSurface3UnlockCalls == 1 &&
        gFakeDirectDrawSurface3LastUnlockArg == gFakeDirectDrawSurface3LastLockDesc &&
        image.pixels == lockedPixels &&
        image.pitchWords == 5 &&
        lockedPixels[0] == 1 &&
        lockedPixels[1] == 2 &&
        lockedPixels[2] == 3 &&
        lockedPixels[3] == 4 &&
        lockedPixels[4] == 5 &&
        lockedPixels[5] == 6 &&
        lockedPixels[6] == 0xcc &&
        lockedPixels[7] == 0xcc &&
        lockedPixels[8] == 0xcc &&
        lockedPixels[9] == 0xcc &&
        lockedPixels[10] == 7 &&
        lockedPixels[11] == 8 &&
        lockedPixels[12] == 9 &&
        lockedPixels[13] == 10 &&
        lockedPixels[14] == 11 &&
        lockedPixels[15] == 12;

    unsigned char retryPixels[12];
    std::memset(retryPixels, 0xdd, sizeof(retryPixels));
    sourcePixels = (unsigned char *)std::malloc(4);
    if (sourcePixels == nullptr) {
        return 2;
    }
    sourcePixels[0] = 0x31;
    sourcePixels[1] = 0x32;
    sourcePixels[2] = 0x33;
    sourcePixels[3] = 0x34;

    InstallFakeDirectDrawSurface3(
        surface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    ConfigureFakeDirectDrawSurface3LockResults(
        DDERR_SURFACELOST,
        DD_OK
    );
    gFakeDirectDrawSurface3LockPixels = retryPixels;
    gFakeDirectDrawSurface3LockPitch = 6;
    image = {};
    image.width = 2;
    image.height = 1;
    image.pixels = sourcePixels;
    image.surface = (IDirectDrawSurface3 *)(&surface);

    const int retryResult = zVideo_dd::Image_PopulateSurfaceFromHeapPixels(&image);
    const bool retryOk =
        retryResult == 1 &&
        gFakeDirectDrawSurface3LockCalls == 2 &&
        gFakeDirectDrawSurface3RestoreCalls == 1 &&
        gFakeDirectDrawSurface3UnlockCalls == 1 &&
        image.pixels == retryPixels &&
        image.pitchWords == 3 &&
        retryPixels[0] == 0x31 &&
        retryPixels[1] == 0x32 &&
        retryPixels[2] == 0x33 &&
        retryPixels[3] == 0x34;

    sourcePixels = (unsigned char *)std::malloc(4);
    if (sourcePixels == nullptr) {
        return 3;
    }
    InstallFakeDirectDrawSurface3(
        surface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    ConfigureFakeDirectDrawSurface3LockResults(
        DDERR_GENERIC,
        DDERR_GENERIC
    );
    image = {};
    image.width = 2;
    image.height = 1;
    image.pixels = sourcePixels;
    image.surface = (IDirectDrawSurface3 *)(&surface);
    const int lockFailureResult = zVideo_dd::Image_PopulateSurfaceFromHeapPixels(&image);
    const bool lockFailureOk =
        lockFailureResult == 0 &&
        gFakeDirectDrawSurface3LockCalls == 1 &&
        gFakeDirectDrawSurface3RestoreCalls == 0 &&
        gFakeDirectDrawSurface3UnlockCalls == 0 &&
        image.pixels == sourcePixels;
    std::free(sourcePixels);

    return successOk && retryOk && lockFailureOk ? 0 : 4;
}

extern "C" int zvideo_dd_image_lazy_create_backing_surface_smoke(void) {
    FakeDirectDraw2Object directDraw{};
    FakeDirectDrawSurfaceObject baseSurface{};
    FakeDirectDrawSurface3Object surface3{};
    IDirectDraw2 *const savedDirectDraw2 = g_zVideo_pDirectDraw2;

    unsigned char lockedPixels[24];
    std::memset(lockedPixels, 0xcc, sizeof(lockedPixels));
    unsigned char *sourcePixels = (unsigned char *)std::malloc(12);
    if (sourcePixels == nullptr) {
        return 1;
    }
    for (int i = 0; i < 12; ++i) {
        sourcePixels[i] = (unsigned char)(0x41 + i);
    }

    InstallFakeDirectDraw2(
        directDraw,
        baseSurface,
        surface3
    );
    InstallFakeDirectDrawSurface3(
        surface3,
        DD_OK,
        DD_OK,
        DD_OK
    );
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    gFakeDirectDrawSurface3LockPixels = lockedPixels;
    gFakeDirectDrawSurface3LockPitch = 8;

    zVidImagePartial image{};
    image.width = 3;
    image.height = 2;
    image.pixels = sourcePixels;
    const DWORD requestedCaps = DDSCAPS_SYSTEMMEMORY;
    IDirectDrawSurface3 *const result =
        zVideo_dd::Image_LazyCreateBackingSurface(&image, requestedCaps);
    const bool successOk =
        result == (IDirectDrawSurface3 *)(&surface3) &&
        image.surface == (IDirectDrawSurface3 *)(&surface3) &&
        image.pixels == lockedPixels &&
        image.pitchWords == 4 &&
        gFakeDirectDraw2CreateSurfaceCalls == 1 &&
        gFakeDirectDraw2LastCreateSurfaceOut != nullptr &&
        gFakeDirectDraw2LastCreateSurfaceOuter == nullptr &&
        gFakeDirectDraw2CreateSurfaceDescs[0].dwSize == sizeof(DDSURFACEDESC) &&
        gFakeDirectDraw2CreateSurfaceDescs[0].dwFlags == 0x10007 &&
        gFakeDirectDraw2CreateSurfaceDescs[0].dwWidth == 3 &&
        gFakeDirectDraw2CreateSurfaceDescs[0].dwHeight == 2 &&
        gFakeDirectDraw2CreateSurfaceDescs[0].ddsCaps.dwCaps ==
            (requestedCaps | DDSCAPS_OFFSCREENPLAIN) &&
        gFakeDirectDrawSurfaceQueryInterfaceCalls == 1 &&
        IsEqualGUID(
            *gFakeDirectDrawSurfaceLastQueryInterfaceIid,
            IID_IDirectDrawSurface3
        ) &&
        gFakeDirectDrawSurfaceLastQueryInterfaceOut != nullptr &&
        gFakeDirectDrawSurfaceReleaseCalls == 0 &&
        gFakeDirectDrawSurface3LockCalls == 1 &&
        gFakeDirectDrawSurface3UnlockCalls == 1 &&
        lockedPixels[0] == 0x41 &&
        lockedPixels[1] == 0x42 &&
        lockedPixels[2] == 0x43 &&
        lockedPixels[3] == 0x44 &&
        lockedPixels[4] == 0x45 &&
        lockedPixels[5] == 0x46 &&
        lockedPixels[6] == 0xcc &&
        lockedPixels[7] == 0xcc &&
        lockedPixels[8] == 0x47 &&
        lockedPixels[9] == 0x48 &&
        lockedPixels[10] == 0x49 &&
        lockedPixels[11] == 0x4a &&
        lockedPixels[12] == 0x4b &&
        lockedPixels[13] == 0x4c;

    sourcePixels = (unsigned char *)std::malloc(4);
    if (sourcePixels == nullptr) {
        g_zVideo_pDirectDraw2 = savedDirectDraw2;
        return 2;
    }
    InstallFakeDirectDraw2(
        directDraw,
        baseSurface,
        surface3
    );
    InstallFakeDirectDrawSurface3(
        surface3,
        DD_OK,
        DD_OK,
        DD_OK
    );
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    gFakeDirectDraw2CreateSurfaceResult = DDERR_GENERIC;
    image = {};
    image.width = 2;
    image.height = 1;
    image.pixels = sourcePixels;
    const bool createFailureOk =
        zVideo_dd::Image_LazyCreateBackingSurface(&image, 0) == nullptr &&
        image.surface == nullptr &&
        image.pixels == sourcePixels &&
        gFakeDirectDraw2CreateSurfaceCalls == 1 &&
        gFakeDirectDrawSurfaceQueryInterfaceCalls == 0 &&
        gFakeDirectDrawSurface3LockCalls == 0;
    std::free(sourcePixels);

    sourcePixels = (unsigned char *)std::malloc(4);
    if (sourcePixels == nullptr) {
        g_zVideo_pDirectDraw2 = savedDirectDraw2;
        return 3;
    }
    InstallFakeDirectDraw2(
        directDraw,
        baseSurface,
        surface3
    );
    InstallFakeDirectDrawSurface3(
        surface3,
        DD_OK,
        DD_OK,
        DD_OK
    );
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    gFakeDirectDrawSurfaceQueryInterfaceResult = DDERR_GENERIC;
    image = {};
    image.width = 2;
    image.height = 1;
    image.pixels = sourcePixels;
    const bool queryFailureOk =
        zVideo_dd::Image_LazyCreateBackingSurface(&image, 0) == nullptr &&
        image.surface == nullptr &&
        image.pixels == sourcePixels &&
        gFakeDirectDraw2CreateSurfaceCalls == 1 &&
        gFakeDirectDrawSurfaceQueryInterfaceCalls == 1 &&
        gFakeDirectDrawSurface3LockCalls == 0;
    std::free(sourcePixels);

    g_zVideo_pDirectDraw2 = savedDirectDraw2;
    return successOk && createFailureOk && queryFailureOk ? 0 : 4;
}

extern "C" int zvideo_dd_image_lazy_create_video_memory_surface_smoke(void) {
    FakeDirectDraw2Object directDraw{};
    FakeDirectDrawSurfaceObject baseSurface{};
    FakeDirectDrawSurface3Object surface3{};
    zVidHwApiDeviceRecordPartial selectedDevice{};
    IDirectDraw2 *const savedDirectDraw2 = g_zVideo_pDirectDraw2;
    zVidHwApiDeviceRecordPartial *const savedSelectedDevice =
        g_zVideo_pSelectedHwApiDeviceRecord;
    const int savedUseHalfRes = g_zVideo_UseHalfResBackbuffer;

    InstallFakeDirectDraw2(
        directDraw,
        baseSurface,
        surface3
    );
    InstallFakeDirectDrawSurface3(
        surface3,
        DD_OK,
        DD_OK,
        DD_OK
    );
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    selectedDevice.m_deviceFeatureFlags = 0;
    g_zVideo_UseHalfResBackbuffer = 0;

    zVidImagePartial image{};
    image.width = 2;
    image.height = 1;
    image.surface = (IDirectDrawSurface3 *)(0x1234);
    image.pixels = std::malloc(4);
    if (image.pixels == nullptr) {
        return 1;
    }
    const bool skipOk =
        zVideo_dd::Image_LazyCreateVideoMemorySurface(&image) == nullptr &&
        gFakeDirectDraw2CreateSurfaceCalls == 0 &&
        image.surface == (IDirectDrawSurface3 *)(0x1234);
    std::free(image.pixels);

    unsigned char halfResPixels[8];
    std::memset(halfResPixels, 0xcc, sizeof(halfResPixels));
    unsigned char *sourcePixels = (unsigned char *)std::malloc(4);
    if (sourcePixels == nullptr) {
        g_zVideo_pDirectDraw2 = savedDirectDraw2;
        g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
        g_zVideo_UseHalfResBackbuffer = savedUseHalfRes;
        return 2;
    }
    sourcePixels[0] = 0x11;
    sourcePixels[1] = 0x12;
    sourcePixels[2] = 0x13;
    sourcePixels[3] = 0x14;
    InstallFakeDirectDraw2(
        directDraw,
        baseSurface,
        surface3
    );
    InstallFakeDirectDrawSurface3(
        surface3,
        DD_OK,
        DD_OK,
        DD_OK
    );
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    selectedDevice.m_deviceFeatureFlags = 0;
    g_zVideo_UseHalfResBackbuffer = 1;
    gFakeDirectDrawSurface3LockPixels = halfResPixels;
    gFakeDirectDrawSurface3LockPitch = 4;
    image = {};
    image.width = 2;
    image.height = 1;
    image.pixels = sourcePixels;
    IDirectDrawSurface3 *const halfResResult =
        zVideo_dd::Image_LazyCreateVideoMemorySurface(&image);
    const bool halfResCapsOk =
        halfResResult == (IDirectDrawSurface3 *)(&surface3) &&
        gFakeDirectDraw2CreateSurfaceCalls == 1 &&
        gFakeDirectDraw2CreateSurfaceDescs[0].ddsCaps.dwCaps ==
            (DDSCAPS_VIDEOMEMORY | DDSCAPS_OFFSCREENPLAIN) &&
        image.surface == (IDirectDrawSurface3 *)(&surface3) &&
        image.pixels == halfResPixels &&
        image.pitchWords == 2 &&
        halfResPixels[0] == 0x11 &&
        halfResPixels[1] == 0x12 &&
        halfResPixels[2] == 0x13 &&
        halfResPixels[3] == 0x14;

    unsigned char featurePixels[8];
    std::memset(featurePixels, 0xdd, sizeof(featurePixels));
    sourcePixels = (unsigned char *)std::malloc(4);
    if (sourcePixels == nullptr) {
        g_zVideo_pDirectDraw2 = savedDirectDraw2;
        g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
        g_zVideo_UseHalfResBackbuffer = savedUseHalfRes;
        return 3;
    }
    sourcePixels[0] = 0x21;
    sourcePixels[1] = 0x22;
    sourcePixels[2] = 0x23;
    sourcePixels[3] = 0x24;
    InstallFakeDirectDraw2(
        directDraw,
        baseSurface,
        surface3
    );
    InstallFakeDirectDrawSurface3(
        surface3,
        DD_OK,
        DD_OK,
        DD_OK
    );
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    selectedDevice.m_deviceFeatureFlags = 0x1357;
    g_zVideo_UseHalfResBackbuffer = 0;
    gFakeDirectDrawSurface3LockPixels = featurePixels;
    gFakeDirectDrawSurface3LockPitch = 4;
    image = {};
    image.width = 2;
    image.height = 1;
    image.pixels = sourcePixels;
    IDirectDrawSurface3 *const featureResult =
        zVideo_dd::Image_LazyCreateVideoMemorySurface(&image);
    const bool featureCapsOk =
        featureResult == (IDirectDrawSurface3 *)(&surface3) &&
        gFakeDirectDraw2CreateSurfaceCalls == 1 &&
        gFakeDirectDraw2CreateSurfaceDescs[0].ddsCaps.dwCaps ==
            (DDSCAPS_NONLOCALVIDMEM | DDSCAPS_VIDEOMEMORY |
             DDSCAPS_OFFSCREENPLAIN) &&
        image.surface == (IDirectDrawSurface3 *)(&surface3) &&
        image.pixels == featurePixels &&
        image.pitchWords == 2 &&
        featurePixels[0] == 0x21 &&
        featurePixels[1] == 0x22 &&
        featurePixels[2] == 0x23 &&
        featurePixels[3] == 0x24;

    g_zVideo_pDirectDraw2 = savedDirectDraw2;
    g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
    g_zVideo_UseHalfResBackbuffer = savedUseHalfRes;
    return skipOk && halfResCapsOk && featureCapsOk ? 0 : 4;
}

extern "C" int zvideo_dd_image_upload_pixels_to_surface_smoke(void) {
    const int savedRendererType = g_zVideo_RendererType;
    IDirectDraw2 *const savedDirectDraw2 = g_zVideo_pDirectDraw2;
    zVidHwApiDeviceRecordPartial *const savedSelectedDevice =
        g_zVideo_pSelectedHwApiDeviceRecord;
    zVidHwApiDeviceRecordPartial selectedDevice{};

    FakeDirectDrawSurface3Object surface{};
    InstallFakeDirectDrawSurface3(
        surface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    zVidImagePartial image{};
    HDC hdc = reinterpret_cast<HDC>(0x1111);
    image.surface = (IDirectDrawSurface3 *)(&surface);
    g_zVideo_RendererType = 2;
    const bool rendererSkipOk =
        zVideo_dd::Image_UploadPixelsToSurface(&image, &hdc) == 0 &&
        gFakeDirectDrawSurface3GetDCCalls == 0 &&
        hdc == reinterpret_cast<HDC>(0x1111);

    InstallFakeDirectDrawSurface3(
        surface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    hdc = nullptr;
    image = {};
    image.surface = (IDirectDrawSurface3 *)(&surface);
    g_zVideo_RendererType = 0;
    const bool existingSurfaceOk =
        zVideo_dd::Image_UploadPixelsToSurface(&image, &hdc) == 1 &&
        gFakeDirectDrawSurface3GetDCCalls == 1 &&
        gFakeDirectDrawSurface3LastGetDCSurface == image.surface &&
        gFakeDirectDrawSurface3LastGetDCOut == &hdc &&
        hdc == gFakeDirectDrawSurface3GetDCValue;

    InstallFakeDirectDrawSurface3(
        surface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    gFakeDirectDrawSurface3GetDCResult = DDERR_GENERIC;
    hdc = nullptr;
    image = {};
    image.surface = (IDirectDrawSurface3 *)(&surface);
    const bool getDcFailureOk =
        zVideo_dd::Image_UploadPixelsToSurface(&image, &hdc) == 0 &&
        gFakeDirectDrawSurface3GetDCCalls == 1 &&
        gFakeDirectDrawSurface3LastGetDCOut == &hdc &&
        hdc == nullptr;

    FakeDirectDraw2Object directDraw{};
    FakeDirectDrawSurfaceObject baseSurface{};
    FakeDirectDrawSurface3Object surface3{};
    unsigned char systemPixels[8];
    std::memset(systemPixels, 0xcc, sizeof(systemPixels));
    unsigned char *sourcePixels = (unsigned char *)std::malloc(4);
    if (sourcePixels == nullptr) {
        g_zVideo_RendererType = savedRendererType;
        g_zVideo_pDirectDraw2 = savedDirectDraw2;
        g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
        return 1;
    }
    sourcePixels[0] = 0x31;
    sourcePixels[1] = 0x32;
    sourcePixels[2] = 0x33;
    sourcePixels[3] = 0x34;
    InstallFakeDirectDraw2(
        directDraw,
        baseSurface,
        surface3
    );
    InstallFakeDirectDrawSurface3(
        surface3,
        DD_OK,
        DD_OK,
        DD_OK
    );
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    selectedDevice.m_deviceFeatureFlags = 0;
    gFakeDirectDrawSurface3LockPixels = systemPixels;
    gFakeDirectDrawSurface3LockPitch = 4;
    hdc = nullptr;
    image = {};
    image.width = 2;
    image.height = 1;
    image.pixels = sourcePixels;
    const bool lazySystemOk =
        zVideo_dd::Image_UploadPixelsToSurface(&image, &hdc) == 1 &&
        gFakeDirectDraw2CreateSurfaceCalls == 1 &&
        gFakeDirectDraw2CreateSurfaceDescs[0].ddsCaps.dwCaps ==
            (DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN) &&
        gFakeDirectDrawSurface3LockCalls == 1 &&
        gFakeDirectDrawSurface3UnlockCalls == 1 &&
        gFakeDirectDrawSurface3GetDCCalls == 1 &&
        image.surface == (IDirectDrawSurface3 *)(&surface3) &&
        image.pixels == systemPixels &&
        image.pitchWords == 2 &&
        hdc == gFakeDirectDrawSurface3GetDCValue &&
        systemPixels[0] == 0x31 &&
        systemPixels[1] == 0x32 &&
        systemPixels[2] == 0x33 &&
        systemPixels[3] == 0x34;

    unsigned char featurePixels[8];
    std::memset(featurePixels, 0xdd, sizeof(featurePixels));
    sourcePixels = (unsigned char *)std::malloc(4);
    if (sourcePixels == nullptr) {
        g_zVideo_RendererType = savedRendererType;
        g_zVideo_pDirectDraw2 = savedDirectDraw2;
        g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
        return 2;
    }
    sourcePixels[0] = 0x41;
    sourcePixels[1] = 0x42;
    sourcePixels[2] = 0x43;
    sourcePixels[3] = 0x44;
    InstallFakeDirectDraw2(
        directDraw,
        baseSurface,
        surface3
    );
    InstallFakeDirectDrawSurface3(
        surface3,
        DD_OK,
        DD_OK,
        DD_OK
    );
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    selectedDevice.m_deviceFeatureFlags = 0x2468;
    gFakeDirectDrawSurface3LockPixels = featurePixels;
    gFakeDirectDrawSurface3LockPitch = 4;
    hdc = nullptr;
    image = {};
    image.width = 2;
    image.height = 1;
    image.pixels = sourcePixels;
    const bool lazyFeatureOk =
        zVideo_dd::Image_UploadPixelsToSurface(&image, &hdc) == 1 &&
        gFakeDirectDraw2CreateSurfaceCalls == 1 &&
        gFakeDirectDraw2CreateSurfaceDescs[0].ddsCaps.dwCaps ==
            (DDSCAPS_NONLOCALVIDMEM | DDSCAPS_VIDEOMEMORY |
             DDSCAPS_OFFSCREENPLAIN) &&
        gFakeDirectDrawSurface3GetDCCalls == 1 &&
        image.surface == (IDirectDrawSurface3 *)(&surface3) &&
        image.pixels == featurePixels &&
        image.pitchWords == 2 &&
        hdc == gFakeDirectDrawSurface3GetDCValue &&
        featurePixels[0] == 0x41 &&
        featurePixels[1] == 0x42 &&
        featurePixels[2] == 0x43 &&
        featurePixels[3] == 0x44;

    sourcePixels = (unsigned char *)std::malloc(4);
    if (sourcePixels == nullptr) {
        g_zVideo_RendererType = savedRendererType;
        g_zVideo_pDirectDraw2 = savedDirectDraw2;
        g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
        return 3;
    }
    InstallFakeDirectDraw2(
        directDraw,
        baseSurface,
        surface3
    );
    InstallFakeDirectDrawSurface3(
        surface3,
        DD_OK,
        DD_OK,
        DD_OK
    );
    g_zVideo_pDirectDraw2 = (IDirectDraw2 *)(&directDraw);
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    selectedDevice.m_deviceFeatureFlags = 0;
    hdc = nullptr;
    image = {};
    image.alphaMap = reinterpret_cast<char *>(0x1234);
    image.width = 2;
    image.height = 1;
    image.pixels = sourcePixels;
    const bool lazyFailureOk =
        zVideo_dd::Image_UploadPixelsToSurface(&image, &hdc) == 0 &&
        gFakeDirectDraw2CreateSurfaceCalls == 0 &&
        gFakeDirectDrawSurface3GetDCCalls == 0 &&
        image.surface == nullptr &&
        image.pixels == sourcePixels;
    std::free(sourcePixels);

    g_zVideo_RendererType = savedRendererType;
    g_zVideo_pDirectDraw2 = savedDirectDraw2;
    g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedDevice;
    return rendererSkipOk && existingSurfaceOk && getDcFailureOk && lazySystemOk &&
                   lazyFeatureOk && lazyFailureOk
               ? 0
               : 4;
}

extern "C" int zvideo_dd_image_release_surface_smoke(void) {
    zVidImagePartial image{};
    HDC hdc = reinterpret_cast<HDC>(0x2468);
    const bool nullSurfaceOk =
        zVideo_dd::Image_ReleaseSurface(&image, hdc) == 0 &&
        gFakeDirectDrawSurface3ReleaseDCCalls == 0;

    FakeDirectDrawSurface3Object surface{};
    InstallFakeDirectDrawSurface3(
        surface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    hdc = reinterpret_cast<HDC>(0x1357);
    image.surface = (IDirectDrawSurface3 *)(&surface);
    const bool successOk =
        zVideo_dd::Image_ReleaseSurface(&image, hdc) == 1 &&
        gFakeDirectDrawSurface3ReleaseDCCalls == 1 &&
        gFakeDirectDrawSurface3LastReleaseDCSurface == image.surface &&
        gFakeDirectDrawSurface3LastReleaseDCHdc == hdc;

    InstallFakeDirectDrawSurface3(
        surface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    gFakeDirectDrawSurface3ReleaseDCResult = DDERR_GENERIC;
    hdc = reinterpret_cast<HDC>(0x9753);
    image.surface = (IDirectDrawSurface3 *)(&surface);
    const bool failureOk =
        zVideo_dd::Image_ReleaseSurface(&image, hdc) == 0 &&
        gFakeDirectDrawSurface3ReleaseDCCalls == 1 &&
        gFakeDirectDrawSurface3LastReleaseDCSurface == image.surface &&
        gFakeDirectDrawSurface3LastReleaseDCHdc == hdc;

    return nullSurfaceOk && successOk && failureOk ? 0 : 1;
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

extern "C" int zvideo_dd_blt_sw_to_primary_rect_smoke(void) {
    const zVideo_SurfaceStatePartial savedPrimaryState = g_zVideo_PrimarySurfaceState;
    const int savedFullscreenOption = g_zVideo_FullscreenOption;

    FakeDirectDrawSurface3Object primarySurface{};
    FakeDirectDrawSurface3Object sourceSurface{};
    InstallFakeDirectDrawSurface3(
        primarySurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    sourceSurface.vtable = gFakeDirectDrawSurface3VTable;

    g_zVideo_FullscreenOption = 1;
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_PrimarySurfaceState.width = 10;
    g_zVideo_PrimarySurfaceState.height = 8;
    g_zVideo_PrimarySurfaceState.locked = 1;
    g_zVideo_PrimarySurfaceState.surf = (IDirectDrawSurface3 *)(&primarySurface);

    zVidImagePartial image{};
    image.width = 6;
    image.height = 4;
    image.surface = (IDirectDrawSurface3 *)(&sourceSurface);

    zVidRect32 srcRect = {1, 2, 15, 12};
    zVidRect32 dstRect = {-2, -1, 12, 9};
    zVideo_dd::BltSwToPrimaryRect(
        &image,
        1,
        &srcRect,
        &dstRect
    );
    const bool clippedOk =
        gFakeDirectDrawSurface3BltCalls == 1 &&
        gFakeDirectDrawSurface3UnlockCalls == 1 &&
        gFakeDirectDrawSurface3LockCalls == 1 &&
        g_zVideo_PrimarySurfaceState.locked == 1 &&
        gFakeDirectDrawSurface3LastBltSource == image.surface &&
        gFakeDirectDrawSurface3LastBltFx == nullptr &&
        gFakeDirectDrawSurface3LastBltFlags ==
            (DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE | DDBLT_KEYSRC) &&
        gFakeDirectDrawSurface3LastBltDstRect.left == 0 &&
        gFakeDirectDrawSurface3LastBltDstRect.top == 0 &&
        gFakeDirectDrawSurface3LastBltDstRect.right == 10 &&
        gFakeDirectDrawSurface3LastBltDstRect.bottom == 8 &&
        gFakeDirectDrawSurface3LastBltSrcRect.left == 3 &&
        gFakeDirectDrawSurface3LastBltSrcRect.top == 3 &&
        gFakeDirectDrawSurface3LastBltSrcRect.right == 13 &&
        gFakeDirectDrawSurface3LastBltSrcRect.bottom == 11;

    InstallFakeDirectDrawSurface3(
        primarySurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    sourceSurface.vtable = gFakeDirectDrawSurface3VTable;
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_PrimarySurfaceState.width = 10;
    g_zVideo_PrimarySurfaceState.height = 8;
    g_zVideo_PrimarySurfaceState.locked = 0;
    g_zVideo_PrimarySurfaceState.surf = (IDirectDrawSurface3 *)(&primarySurface);
    image.width = 4;
    image.height = 3;
    image.surface = (IDirectDrawSurface3 *)(&sourceSurface);

    zVideo_dd::BltSwToPrimaryRect(
        &image,
        0,
        nullptr,
        nullptr
    );
    const bool defaultRectOk =
        gFakeDirectDrawSurface3BltCalls == 1 &&
        gFakeDirectDrawSurface3UnlockCalls == 0 &&
        gFakeDirectDrawSurface3LockCalls == 0 &&
        gFakeDirectDrawSurface3LastBltFlags ==
            (DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE) &&
        gFakeDirectDrawSurface3LastBltDstRect.left == 0 &&
        gFakeDirectDrawSurface3LastBltDstRect.top == 0 &&
        gFakeDirectDrawSurface3LastBltDstRect.right == 4 &&
        gFakeDirectDrawSurface3LastBltDstRect.bottom == 3 &&
        gFakeDirectDrawSurface3LastBltSrcRect.left == 0 &&
        gFakeDirectDrawSurface3LastBltSrcRect.top == 0 &&
        gFakeDirectDrawSurface3LastBltSrcRect.right == 4 &&
        gFakeDirectDrawSurface3LastBltSrcRect.bottom == 3;

    InstallFakeDirectDrawSurface3(
        primarySurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    sourceSurface.vtable = gFakeDirectDrawSurface3VTable;
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_PrimarySurfaceState.width = 10;
    g_zVideo_PrimarySurfaceState.height = 8;
    g_zVideo_PrimarySurfaceState.surf = (IDirectDrawSurface3 *)(&primarySurface);
    image.width = 4;
    image.height = 3;
    image.surface = (IDirectDrawSurface3 *)(&sourceSurface);
    dstRect = {20, 0, 24, 3};
    zVideo_dd::BltSwToPrimaryRect(
        &image,
        0,
        nullptr,
        &dstRect
    );
    const bool clippedOutOk = gFakeDirectDrawSurface3BltCalls == 0;

    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_FullscreenOption = savedFullscreenOption;
    return clippedOk && defaultRectOk && clippedOutOk ? 0 : 1;
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

extern "C" int zvideo_dd_clear_screen_and_zbuffer_rect_smoke(void) {
    const int savedClearScreenBufferEnabled = g_zVideo_ClearScreenBufferEnabled;
    const int savedClearColorPacked16 = g_zVideo_ClearColorPacked16;
    IDirectDrawSurface3 *const savedZBufferSurface = g_zVideo_pZBufferSurface;

    zVidRect32 rect{4, 5, 20, 24};
    zVideo_SurfaceStatePartial colorState{};
    FakeDirectDrawSurface3Object colorSurface{};
    FakeDirectDrawSurface3Object zBufferSurface{};

    InstallFakeDirectDrawSurface3(
        colorSurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    zBufferSurface.vtable = gFakeDirectDrawSurface3VTable;
    colorState.surf = (IDirectDrawSurface3 *)(&colorSurface);
    g_zVideo_pZBufferSurface = (IDirectDrawSurface3 *)(&zBufferSurface);
    g_zVideo_ClearScreenBufferEnabled = 1;
    g_zVideo_ClearColorPacked16 = 0x1ace;
    zVideo_dd::ClearScreenAndZBufferRect(&rect, &colorState);
    const bool colorThenZOk =
        gFakeDirectDrawSurface3BltCalls == 2 &&
        gFakeDirectDrawSurface3RestoreCalls == 0 &&
        gFakeDirectDrawSurface3BltSurfaces[0] == colorState.surf &&
        gFakeDirectDrawSurface3BltSurfaces[1] == g_zVideo_pZBufferSurface &&
        gFakeDirectDrawSurface3BltDstRectArgs[0] == (LPRECT)(&rect) &&
        gFakeDirectDrawSurface3BltDstRectArgs[1] == (LPRECT)(&rect) &&
        gFakeDirectDrawSurface3BltSrcRectArgs[0] == nullptr &&
        gFakeDirectDrawSurface3BltSrcRectArgs[1] == nullptr &&
        gFakeDirectDrawSurface3BltFlags[0] == (DDBLT_COLORFILL | DDBLT_WAIT) &&
        gFakeDirectDrawSurface3BltFlags[1] == DDBLT_DEPTHFILL &&
        gFakeDirectDrawSurface3BltFxValues[0].dwSize == sizeof(DDBLTFX) &&
        gFakeDirectDrawSurface3BltFxValues[0].dwFillColor == 0x1ace &&
        gFakeDirectDrawSurface3BltFxValues[1].dwSize == sizeof(DDBLTFX) &&
        gFakeDirectDrawSurface3BltFxValues[1].dwFillDepth == 0;

    InstallFakeDirectDrawSurface3(
        colorSurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    zBufferSurface.vtable = gFakeDirectDrawSurface3VTable;
    colorState.surf = (IDirectDrawSurface3 *)(&colorSurface);
    g_zVideo_pZBufferSurface = (IDirectDrawSurface3 *)(&zBufferSurface);
    g_zVideo_ClearScreenBufferEnabled = 0;
    g_zVideo_ClearColorPacked16 = 0x5eed;
    zVideo_dd::ClearScreenAndZBufferRect(&rect, &colorState);
    const bool disabledColorOk =
        gFakeDirectDrawSurface3BltCalls == 1 &&
        gFakeDirectDrawSurface3BltSurfaces[0] == g_zVideo_pZBufferSurface &&
        gFakeDirectDrawSurface3BltFlags[0] == DDBLT_DEPTHFILL &&
        gFakeDirectDrawSurface3BltFxValues[0].dwFillDepth == 0;

    InstallFakeDirectDrawSurface3(
        colorSurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    ConfigureFakeDirectDrawSurface3BltResults(DDERR_SURFACELOST, DD_OK);
    zBufferSurface.vtable = gFakeDirectDrawSurface3VTable;
    colorState.surf = (IDirectDrawSurface3 *)(&colorSurface);
    g_zVideo_pZBufferSurface = (IDirectDrawSurface3 *)(&zBufferSurface);
    g_zVideo_ClearScreenBufferEnabled = 1;
    g_zVideo_ClearColorPacked16 = 0x1357;
    zVideo_dd::ClearScreenAndZBufferRect(&rect, &colorState);
    const bool colorRestoreRetryOk =
        gFakeDirectDrawSurface3BltCalls == 3 &&
        gFakeDirectDrawSurface3RestoreCalls == 1 &&
        gFakeDirectDrawSurface3BltSurfaces[0] == colorState.surf &&
        gFakeDirectDrawSurface3BltSurfaces[1] == colorState.surf &&
        gFakeDirectDrawSurface3BltSurfaces[2] == g_zVideo_pZBufferSurface &&
        gFakeDirectDrawSurface3BltFlags[0] == (DDBLT_COLORFILL | DDBLT_WAIT) &&
        gFakeDirectDrawSurface3BltFlags[1] == (DDBLT_COLORFILL | DDBLT_WAIT) &&
        gFakeDirectDrawSurface3BltFlags[2] == DDBLT_DEPTHFILL &&
        gFakeDirectDrawSurface3BltFxValues[1].dwFillColor == 0x1357 &&
        gFakeDirectDrawSurface3BltFxValues[2].dwFillDepth == 0;

    InstallFakeDirectDrawSurface3(
        colorSurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    ConfigureFakeDirectDrawSurface3BltResults(DDERR_GENERIC, DD_OK);
    zBufferSurface.vtable = gFakeDirectDrawSurface3VTable;
    colorState.surf = (IDirectDrawSurface3 *)(&colorSurface);
    g_zVideo_pZBufferSurface = (IDirectDrawSurface3 *)(&zBufferSurface);
    g_zVideo_ClearScreenBufferEnabled = 1;
    g_zVideo_ClearColorPacked16 = 0x2468;
    zVideo_dd::ClearScreenAndZBufferRect(&rect, &colorState);
    const bool colorFailureSkipsZOk =
        gFakeDirectDrawSurface3BltCalls == 1 &&
        gFakeDirectDrawSurface3RestoreCalls == 0 &&
        gFakeDirectDrawSurface3BltSurfaces[0] == colorState.surf &&
        gFakeDirectDrawSurface3BltFlags[0] == (DDBLT_COLORFILL | DDBLT_WAIT);

    g_zVideo_ClearScreenBufferEnabled = savedClearScreenBufferEnabled;
    g_zVideo_ClearColorPacked16 = savedClearColorPacked16;
    g_zVideo_pZBufferSurface = savedZBufferSurface;
    return colorThenZOk && disabledColorOk && colorRestoreRetryOk &&
                   colorFailureSkipsZOk
               ? 0
               : 1;
}

extern "C" int zvideo_dd_clear_sw_backbuffer_and_zbuffer_rects_smoke(void) {
    const int savedClearScreenBufferEnabled = g_zVideo_ClearScreenBufferEnabled;
    const int savedClearColorPacked16 = g_zVideo_ClearColorPacked16;
    const zVideo_SurfaceStatePartial savedSwSurfaceState = g_zVideo_SwSurfaceState;
    IDirectDrawSurface3 *const savedZBufferSurface = g_zVideo_pZBufferSurface;

    zVidRect32 colorRect{1, 2, 9, 10};
    zVidRect32 zRect{3, 4, 11, 12};
    FakeDirectDrawSurface3Object swSurface{};
    FakeDirectDrawSurface3Object zBufferSurface{};

    InstallFakeDirectDrawSurface3(
        swSurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    zBufferSurface.vtable = gFakeDirectDrawSurface3VTable;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.surf = (IDirectDrawSurface3 *)(&swSurface);
    g_zVideo_pZBufferSurface = (IDirectDrawSurface3 *)(&zBufferSurface);
    g_zVideo_ClearScreenBufferEnabled = 1;
    g_zVideo_ClearColorPacked16 = 0x1234;
    zVideo_dd::ClearSwBackbufferAndZBufferRects(&colorRect, &zRect);
    const bool colorThenZOk =
        gFakeDirectDrawSurface3BltCalls == 2 &&
        gFakeDirectDrawSurface3RestoreCalls == 0 &&
        gFakeDirectDrawSurface3BltSurfaces[0] == g_zVideo_SwSurfaceState.surf &&
        gFakeDirectDrawSurface3BltSurfaces[1] == g_zVideo_pZBufferSurface &&
        gFakeDirectDrawSurface3BltDstRectArgs[0] == (LPRECT)(&colorRect) &&
        gFakeDirectDrawSurface3BltDstRectArgs[1] == (LPRECT)(&zRect) &&
        gFakeDirectDrawSurface3BltSrcRectArgs[0] == nullptr &&
        gFakeDirectDrawSurface3BltSrcRectArgs[1] == nullptr &&
        gFakeDirectDrawSurface3BltFlags[0] == (DDBLT_COLORFILL | DDBLT_WAIT) &&
        gFakeDirectDrawSurface3BltFlags[1] == DDBLT_DEPTHFILL &&
        gFakeDirectDrawSurface3BltFxValues[0].dwSize == sizeof(DDBLTFX) &&
        gFakeDirectDrawSurface3BltFxValues[0].dwFillColor == 0x1234 &&
        gFakeDirectDrawSurface3BltFxValues[1].dwSize == sizeof(DDBLTFX) &&
        gFakeDirectDrawSurface3BltFxValues[1].dwFillDepth == 0;

    InstallFakeDirectDrawSurface3(
        swSurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    zBufferSurface.vtable = gFakeDirectDrawSurface3VTable;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.surf = (IDirectDrawSurface3 *)(&swSurface);
    g_zVideo_pZBufferSurface = (IDirectDrawSurface3 *)(&zBufferSurface);
    g_zVideo_ClearScreenBufferEnabled = 0;
    g_zVideo_ClearColorPacked16 = 0x4567;
    zVideo_dd::ClearSwBackbufferAndZBufferRects(&colorRect, &zRect);
    const bool disabledColorOk =
        gFakeDirectDrawSurface3BltCalls == 1 &&
        gFakeDirectDrawSurface3BltSurfaces[0] == g_zVideo_pZBufferSurface &&
        gFakeDirectDrawSurface3BltDstRectArgs[0] == (LPRECT)(&zRect) &&
        gFakeDirectDrawSurface3BltFlags[0] == DDBLT_DEPTHFILL &&
        gFakeDirectDrawSurface3BltFxValues[0].dwFillDepth == 0;

    InstallFakeDirectDrawSurface3(
        swSurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    zBufferSurface.vtable = gFakeDirectDrawSurface3VTable;
    gFakeDirectDrawSurface3BltResults[0] = DD_OK;
    gFakeDirectDrawSurface3BltResults[1] = DDERR_SURFACELOST;
    gFakeDirectDrawSurface3BltResults[2] = DD_OK;
    gFakeDirectDrawSurface3BltResultCount = 3;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.surf = (IDirectDrawSurface3 *)(&swSurface);
    g_zVideo_pZBufferSurface = (IDirectDrawSurface3 *)(&zBufferSurface);
    g_zVideo_ClearScreenBufferEnabled = 1;
    g_zVideo_ClearColorPacked16 = 0x2468;
    zVideo_dd::ClearSwBackbufferAndZBufferRects(&colorRect, &zRect);
    const bool zRestoreRetryOk =
        gFakeDirectDrawSurface3BltCalls == 3 &&
        gFakeDirectDrawSurface3RestoreCalls == 1 &&
        gFakeDirectDrawSurface3BltSurfaces[0] == g_zVideo_SwSurfaceState.surf &&
        gFakeDirectDrawSurface3BltSurfaces[1] == g_zVideo_pZBufferSurface &&
        gFakeDirectDrawSurface3BltSurfaces[2] == g_zVideo_pZBufferSurface &&
        gFakeDirectDrawSurface3BltFlags[0] == (DDBLT_COLORFILL | DDBLT_WAIT) &&
        gFakeDirectDrawSurface3BltFlags[1] == DDBLT_DEPTHFILL &&
        gFakeDirectDrawSurface3BltFlags[2] == DDBLT_DEPTHFILL &&
        gFakeDirectDrawSurface3BltFxValues[0].dwFillColor == 0x2468 &&
        gFakeDirectDrawSurface3BltFxValues[2].dwFillDepth == 0;

    InstallFakeDirectDrawSurface3(
        swSurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    ConfigureFakeDirectDrawSurface3BltResults(DDERR_GENERIC, DD_OK);
    zBufferSurface.vtable = gFakeDirectDrawSurface3VTable;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.surf = (IDirectDrawSurface3 *)(&swSurface);
    g_zVideo_pZBufferSurface = (IDirectDrawSurface3 *)(&zBufferSurface);
    g_zVideo_ClearScreenBufferEnabled = 1;
    g_zVideo_ClearColorPacked16 = 0x3579;
    zVideo_dd::ClearSwBackbufferAndZBufferRects(&colorRect, &zRect);
    const bool colorFailureSkipsZOk =
        gFakeDirectDrawSurface3BltCalls == 1 &&
        gFakeDirectDrawSurface3RestoreCalls == 0 &&
        gFakeDirectDrawSurface3BltSurfaces[0] == g_zVideo_SwSurfaceState.surf &&
        gFakeDirectDrawSurface3BltDstRectArgs[0] == (LPRECT)(&colorRect) &&
        gFakeDirectDrawSurface3BltFlags[0] == (DDBLT_COLORFILL | DDBLT_WAIT);

    g_zVideo_ClearScreenBufferEnabled = savedClearScreenBufferEnabled;
    g_zVideo_ClearColorPacked16 = savedClearColorPacked16;
    g_zVideo_SwSurfaceState = savedSwSurfaceState;
    g_zVideo_pZBufferSurface = savedZBufferSurface;
    return colorThenZOk && disabledColorOk && zRestoreRetryOk &&
                   colorFailureSkipsZOk
               ? 0
               : 1;
}

extern "C" int zvideo_dd_zbuffer_depth_fill_rect_smoke(void) {
    IDirectDrawSurface3 *const savedZBufferSurface = g_zVideo_pZBufferSurface;
    zVidRect32 rect{2, 3, 7, 11};

    FakeDirectDrawSurface3Object zBufferSurface{};
    InstallFakeDirectDrawSurface3(
        zBufferSurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    g_zVideo_pZBufferSurface = nullptr;
    zVideo_dd::ZBuffer_DepthFillRect(&rect);
    const bool nullSurfaceOk = gFakeDirectDrawSurface3BltCalls == 0;

    InstallFakeDirectDrawSurface3(
        zBufferSurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    g_zVideo_pZBufferSurface = (IDirectDrawSurface3 *)(&zBufferSurface);
    zVideo_dd::ZBuffer_DepthFillRect(&rect);
    const bool successOk =
        gFakeDirectDrawSurface3BltCalls == 1 &&
        gFakeDirectDrawSurface3RestoreCalls == 0 &&
        gFakeDirectDrawSurface3LastBltDstRectArg == (LPRECT)(&rect) &&
        gFakeDirectDrawSurface3LastBltSrcRectArg == nullptr &&
        gFakeDirectDrawSurface3LastBltSource == nullptr &&
        gFakeDirectDrawSurface3LastBltFlags == DDBLT_DEPTHFILL &&
        gFakeDirectDrawSurface3LastBltDstRect.left == 2 &&
        gFakeDirectDrawSurface3LastBltDstRect.top == 3 &&
        gFakeDirectDrawSurface3LastBltDstRect.right == 7 &&
        gFakeDirectDrawSurface3LastBltDstRect.bottom == 11 &&
        gFakeDirectDrawSurface3LastBltFx != nullptr &&
        gFakeDirectDrawSurface3LastBltFxValue.dwSize == sizeof(DDBLTFX) &&
        gFakeDirectDrawSurface3LastBltFxValue.dwFillDepth == 0;

    InstallFakeDirectDrawSurface3(
        zBufferSurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    ConfigureFakeDirectDrawSurface3BltResults(DDERR_SURFACELOST, DD_OK);
    g_zVideo_pZBufferSurface = (IDirectDrawSurface3 *)(&zBufferSurface);
    zVideo_dd::ZBuffer_DepthFillRect(&rect);
    const bool restoreRetryOk =
        gFakeDirectDrawSurface3BltCalls == 2 &&
        gFakeDirectDrawSurface3RestoreCalls == 1 &&
        gFakeDirectDrawSurface3LastBltFlags == DDBLT_DEPTHFILL &&
        gFakeDirectDrawSurface3LastBltFxValue.dwFillDepth == 0;

    g_zVideo_pZBufferSurface = savedZBufferSurface;
    return nullSurfaceOk && successOk && restoreRetryOk ? 0 : 1;
}

extern "C" int zvideo_palette_set_entries_non8bpp_smoke(void) {
    PALETTEENTRY entries[2] = {};
    g_zVideo_DisplayModeBpp = 16;
    g_zVideo_pDDPalette = reinterpret_cast<IDirectDrawPalette *>(0x1234);
    const std::int32_t result = zVideo_dd::PaletteSetEntries(1, 2, entries);
    g_zVideo_pDDPalette = nullptr;
    return result;
}

extern "C" int zvideo_dd_palette_set_entries_smoke(void) {
    const int savedBpp = g_zVideo_DisplayModeBpp;
    IDirectDrawPalette *const savedPalette = g_zVideo_pDDPalette;

    FakeDirectDrawPaletteObject palette{};
    PALETTEENTRY entries[3] = {};
    entries[0].peRed = 10;
    entries[1].peGreen = 20;
    entries[2].peBlue = 30;

    InstallFakeDirectDrawPalette(palette);
    g_zVideo_DisplayModeBpp = 16;
    g_zVideo_pDDPalette = reinterpret_cast<IDirectDrawPalette *>(&palette);
    const bool non8BppOk =
        zVideo_dd::PaletteSetEntries(1, 2, entries) == 0 &&
        gFakeDirectDrawPaletteSetEntriesCalls == 0;

    InstallFakeDirectDrawPalette(palette);
    g_zVideo_DisplayModeBpp = 8;
    g_zVideo_pDDPalette = reinterpret_cast<IDirectDrawPalette *>(&palette);
    const bool successOk =
        zVideo_dd::PaletteSetEntries(5, 3, entries) == 0 &&
        gFakeDirectDrawPaletteSetEntriesCalls == 1 &&
        gFakeDirectDrawPaletteLastSetEntriesSelf ==
            reinterpret_cast<IDirectDrawPalette *>(&palette) &&
        gFakeDirectDrawPaletteLastSetEntriesFlags == 0 &&
        gFakeDirectDrawPaletteLastSetEntriesFirst == 5 &&
        gFakeDirectDrawPaletteLastSetEntriesCount == 3 &&
        gFakeDirectDrawPaletteLastSetEntriesEntries == entries;

    InstallFakeDirectDrawPalette(palette);
    g_zVideo_DisplayModeBpp = 8;
    g_zVideo_pDDPalette = reinterpret_cast<IDirectDrawPalette *>(&palette);
    gFakeDirectDrawPaletteSetEntriesResult = DDERR_GENERIC;
    const bool failureOk =
        zVideo_dd::PaletteSetEntries(7, 1, entries) == 0x5a56ffff &&
        gFakeDirectDrawPaletteSetEntriesCalls == 1 &&
        gFakeDirectDrawPaletteLastSetEntriesFlags == 0 &&
        gFakeDirectDrawPaletteLastSetEntriesFirst == 7 &&
        gFakeDirectDrawPaletteLastSetEntriesCount == 1 &&
        gFakeDirectDrawPaletteLastSetEntriesEntries == entries;

    g_zVideo_DisplayModeBpp = savedBpp;
    g_zVideo_pDDPalette = savedPalette;
    return non8BppOk && successOk && failureOk ? 0 : 1;
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

extern "C" int zvideo_flush_quad_batch_smoke(void) {
    FakeD3DDevice2Object fakeDevice = {};
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    const int savedQuadBatchCount = g_zVideo_QuadBatchCount;
    const int savedShadeMode = g_zVideo_D3DRenderState_ShadeMode;
    const int savedAlphaBlendEnable = g_zVideo_D3DRenderState_AlphaBlendEnable;
    const int savedZWriteEnable = g_zVideo_D3DRenderState_ZWriteEnable;
    const D3DTEXTUREHANDLE savedTextureHandle = g_zVideo_D3DRenderState_TextureHandle;

    std::memset(
        g_zVideo_QuadBatchItemsBase,
        0,
        sizeof(g_zVideo_QuadBatchItemsBase[0]) * 2
    );
    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_D3DRenderState_ShadeMode = 1;
    g_zVideo_D3DRenderState_AlphaBlendEnable = 0;
    g_zVideo_D3DRenderState_ZWriteEnable = 1;
    g_zVideo_D3DRenderState_TextureHandle = 0x2468;

    g_zVideo_QuadBatchItemsBase[0].vertices[0].sx = 10.0f;
    g_zVideo_QuadBatchItemsBase[0].vertices[1].sx = 11.0f;
    g_zVideo_QuadBatchItemsBase[0].vertices[2].sx = 12.0f;
    g_zVideo_QuadBatchItemsBase[0].vertices[3].sx = 13.0f;
    g_zVideo_QuadBatchItemsBase[1].vertices[0].sx = 20.0f;
    g_zVideo_QuadBatchItemsBase[1].vertices[1].sx = 21.0f;
    g_zVideo_QuadBatchItemsBase[1].vertices[2].sx = 22.0f;
    g_zVideo_QuadBatchItemsBase[1].vertices[3].sx = 23.0f;
    g_zVideo_QuadBatchCount = 2;

    zVideo_dd3d::FlushQuadBatch();

    const bool setupStateOk =
        gFakeD3DRenderStates[0] == D3DRENDERSTATE_SHADEMODE &&
        gFakeD3DRenderStateValues[0] == 2 &&
        gFakeD3DRenderStates[1] == D3DRENDERSTATE_ALPHABLENDENABLE &&
        gFakeD3DRenderStateValues[1] == 1 &&
        gFakeD3DRenderStates[2] == D3DRENDERSTATE_ZWRITEENABLE &&
        gFakeD3DRenderStateValues[2] == 0 &&
        gFakeD3DRenderStates[3] == D3DRENDERSTATE_TEXTUREHANDLE &&
        gFakeD3DRenderStateValues[3] == 0 &&
        gFakeD3DRenderStates[4] == D3DRENDERSTATE_ZFUNC &&
        gFakeD3DRenderStateValues[4] == D3DCMP_ALWAYS;

    const bool drawOk =
        gFakeD3DDrawPrimitiveCalls == 2 &&
        gFakeD3DDrawPrimitiveTypes[0] == (D3DPRIMITIVETYPE)(6) &&
        gFakeD3DDrawPrimitiveTypes[1] == (D3DPRIMITIVETYPE)(6) &&
        gFakeD3DDrawPrimitiveVertexTypes[0] == (D3DVERTEXTYPE)(3) &&
        gFakeD3DDrawPrimitiveVertexTypes[1] == (D3DVERTEXTYPE)(3) &&
        gFakeD3DDrawPrimitiveVertices[0] == g_zVideo_QuadBatchItemsBase[0].vertices &&
        gFakeD3DDrawPrimitiveVertices[1] == g_zVideo_QuadBatchItemsBase[1].vertices &&
        gFakeD3DDrawPrimitiveVertexCounts[0] == 4 &&
        gFakeD3DDrawPrimitiveVertexCounts[1] == 4 &&
        gFakeD3DDrawPrimitiveFlags[0] == 0 &&
        gFakeD3DDrawPrimitiveFlags[1] == 0;

    const bool restoreStateOk =
        gFakeD3DSetRenderStateCalls == 8 &&
        gFakeD3DRenderStates[5] == D3DRENDERSTATE_ZFUNC &&
        gFakeD3DRenderStateValues[5] == D3DCMP_GREATEREQUAL &&
        gFakeD3DRenderStates[6] == D3DRENDERSTATE_ALPHABLENDENABLE &&
        gFakeD3DRenderStateValues[6] == 0 &&
        gFakeD3DRenderStates[7] == D3DRENDERSTATE_ZWRITEENABLE &&
        gFakeD3DRenderStateValues[7] == 1 &&
        g_zVideo_QuadBatchCount == 0 &&
        g_zVideo_D3DRenderState_ShadeMode == 2 &&
        g_zVideo_D3DRenderState_AlphaBlendEnable == 0 &&
        g_zVideo_D3DRenderState_ZWriteEnable == 1 &&
        g_zVideo_D3DRenderState_TextureHandle == 0;

    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_D3DRenderState_ShadeMode = 2;
    g_zVideo_D3DRenderState_AlphaBlendEnable = 1;
    g_zVideo_D3DRenderState_ZWriteEnable = 0;
    g_zVideo_D3DRenderState_TextureHandle = 0;
    g_zVideo_QuadBatchCount = 1;
    zVideo_dd3d::FlushQuadBatch();
    const bool cacheHitOk =
        gFakeD3DSetRenderStateCalls == 4 &&
        gFakeD3DRenderStates[0] == D3DRENDERSTATE_ZFUNC &&
        gFakeD3DRenderStateValues[0] == D3DCMP_ALWAYS &&
        gFakeD3DRenderStates[1] == D3DRENDERSTATE_ZFUNC &&
        gFakeD3DRenderStateValues[1] == D3DCMP_GREATEREQUAL &&
        gFakeD3DRenderStates[2] == D3DRENDERSTATE_ALPHABLENDENABLE &&
        gFakeD3DRenderStateValues[2] == 0 &&
        gFakeD3DRenderStates[3] == D3DRENDERSTATE_ZWRITEENABLE &&
        gFakeD3DRenderStateValues[3] == 1 &&
        gFakeD3DDrawPrimitiveCalls == 1 &&
        g_zVideo_QuadBatchCount == 0 &&
        g_zVideo_D3DRenderState_ZWriteEnable == 1;

    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_QuadBatchCount = savedQuadBatchCount;
    g_zVideo_D3DRenderState_ShadeMode = savedShadeMode;
    g_zVideo_D3DRenderState_AlphaBlendEnable = savedAlphaBlendEnable;
    g_zVideo_D3DRenderState_ZWriteEnable = savedZWriteEnable;
    g_zVideo_D3DRenderState_TextureHandle = savedTextureHandle;

    if (!setupStateOk) {
        return 1;
    }
    if (!drawOk) {
        return 2;
    }
    if (!restoreStateOk) {
        return 3;
    }
    return cacheHitOk ? 0 : 4;
}

extern "C" int zvideo_flush_overwrite_polys_empty_smoke(void) {
    FakeD3DDevice2Object fakeDevice = {};
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    const int savedOverwriteCount = g_zVideo_OverwriteQueueCount;

    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_OverwriteQueueCount = 0;
    zVideo_dd3d::FlushOverwritePolys();

    const bool stateOk =
        gFakeD3DSetRenderStateCalls == 2 &&
        gFakeD3DRenderStates[0] == D3DRENDERSTATE_ZFUNC &&
        gFakeD3DRenderStateValues[0] == D3DCMP_ALWAYS &&
        gFakeD3DRenderStates[1] == D3DRENDERSTATE_ZFUNC &&
        gFakeD3DRenderStateValues[1] == D3DCMP_GREATEREQUAL &&
        gFakeD3DDrawPrimitiveCalls == 0 &&
        g_zVideo_OverwriteQueueCount == 0;

    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_OverwriteQueueCount = savedOverwriteCount;
    return stateOk ? 0 : 1;
}

extern "C" int zvideo_flush_overwrite_polys_smoke(void) {
    FakeD3DDevice2Object fakeDevice = {};
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    const int savedOverwriteCount = g_zVideo_OverwriteQueueCount;
    const int savedShadeMode = g_zVideo_D3DRenderState_ShadeMode;
    const int savedAlphaBlendEnable = g_zVideo_D3DRenderState_AlphaBlendEnable;
    const int savedZWriteEnable = g_zVideo_D3DRenderState_ZWriteEnable;
    const D3DTEXTUREHANDLE savedTextureHandle = g_zVideo_D3DRenderState_TextureHandle;
    const D3DTEXTUREBLEND savedTextureMapBlend =
        g_zVideo_D3DRenderState_TextureMapBlend;
    const D3DTEXTUREADDRESS savedTextureAddressU =
        g_zVideo_D3DRenderState_TextureAddressU;
    const D3DTEXTUREADDRESS savedTextureAddressV =
        g_zVideo_D3DRenderState_TextureAddressV;

    zVideo_RenderClass transparentClass{};
    transparentClass.textureHandle = 0x1111;
    transparentClass.textureMapBlend = (D3DTEXTUREBLEND)(2);
    transparentClass.textureAddressU = (D3DTEXTUREADDRESS)(1);
    transparentClass.textureAddressV = (D3DTEXTUREADDRESS)(2);

    std::memset(
        g_zVideo_OverwriteQueueBase,
        0,
        sizeof(g_zVideo_OverwriteQueueBase[0])
    );
    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_D3DRenderState_ShadeMode = 1;
    g_zVideo_D3DRenderState_AlphaBlendEnable = 0;
    g_zVideo_D3DRenderState_ZWriteEnable = 1;
    g_zVideo_D3DRenderState_TextureHandle = 0;
    g_zVideo_D3DRenderState_TextureMapBlend = (D3DTEXTUREBLEND)(1);
    g_zVideo_D3DRenderState_TextureAddressU = (D3DTEXTUREADDRESS)(0);
    g_zVideo_D3DRenderState_TextureAddressV = (D3DTEXTUREADDRESS)(0);
    g_zVideo_OverwriteQueueCount = 1;

    zVideo_OverwriteQueueEntry &transparentEntry = g_zVideo_OverwriteQueueBase[0];
    transparentEntry.type = 0;
    transparentEntry.vertexCount = 4;
    transparentEntry.renderClass = reinterpret_cast<std::int32_t>(&transparentClass);
    transparentEntry.vertices[0].sx = 10.0f;
    transparentEntry.vertices[0].color = 0x7f112233;
    zVideo_dd3d::FlushOverwritePolys();

    const bool transparentStateOk =
        gFakeD3DSetRenderStateCalls == 11 &&
        gFakeD3DRenderStates[0] == D3DRENDERSTATE_ZFUNC &&
        gFakeD3DRenderStateValues[0] == D3DCMP_ALWAYS &&
        gFakeD3DRenderStates[1] == D3DRENDERSTATE_SHADEMODE &&
        gFakeD3DRenderStateValues[1] == 2 &&
        gFakeD3DRenderStates[2] == D3DRENDERSTATE_ALPHABLENDENABLE &&
        gFakeD3DRenderStateValues[2] == 1 &&
        gFakeD3DRenderStates[3] == D3DRENDERSTATE_ZWRITEENABLE &&
        gFakeD3DRenderStateValues[3] == 0 &&
        gFakeD3DRenderStates[4] == D3DRENDERSTATE_TEXTUREHANDLE &&
        gFakeD3DRenderStateValues[4] == 0x1111 &&
        gFakeD3DRenderStates[5] == D3DRENDERSTATE_TEXTUREMAPBLEND &&
        gFakeD3DRenderStateValues[5] == 4 &&
        gFakeD3DRenderStates[6] == D3DRENDERSTATE_TEXTUREADDRESSU &&
        gFakeD3DRenderStateValues[6] == 1 &&
        gFakeD3DRenderStates[7] == D3DRENDERSTATE_TEXTUREADDRESSV &&
        gFakeD3DRenderStateValues[7] == 2 &&
        gFakeD3DRenderStates[8] == D3DRENDERSTATE_ALPHABLENDENABLE &&
        gFakeD3DRenderStateValues[8] == 0 &&
        gFakeD3DRenderStates[9] == D3DRENDERSTATE_ZWRITEENABLE &&
        gFakeD3DRenderStateValues[9] == 1 &&
        gFakeD3DRenderStates[10] == D3DRENDERSTATE_ZFUNC &&
        gFakeD3DRenderStateValues[10] == D3DCMP_GREATEREQUAL &&
        g_zVideo_D3DRenderState_TextureMapBlend == (D3DTEXTUREBLEND)(4) &&
        g_zVideo_OverwriteQueueCount == 0;

    const bool transparentDrawOk =
        gFakeD3DDrawPrimitiveCalls == 1 &&
        gFakeD3DDrawPrimitiveTypes[0] == (D3DPRIMITIVETYPE)(6) &&
        gFakeD3DDrawPrimitiveVertexTypes[0] == (D3DVERTEXTYPE)(3) &&
        gFakeD3DDrawPrimitiveVertices[0] == transparentEntry.vertices &&
        gFakeD3DDrawPrimitiveVertexCounts[0] == 4 &&
        gFakeD3DDrawPrimitiveFlags[0] == 0;

    zVideo_RenderClass texturedClass{};
    texturedClass.textureHandle = 0x4444;
    texturedClass.textureMapBlend = (D3DTEXTUREBLEND)(3);
    texturedClass.textureAddressU = (D3DTEXTUREADDRESS)(5);
    texturedClass.textureAddressV = (D3DTEXTUREADDRESS)(6);

    std::memset(
        g_zVideo_OverwriteQueueBase,
        0,
        sizeof(g_zVideo_OverwriteQueueBase[0])
    );
    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_D3DRenderState_ShadeMode = 2;
    g_zVideo_D3DRenderState_TextureHandle = 0;
    g_zVideo_D3DRenderState_TextureMapBlend = (D3DTEXTUREBLEND)(1);
    g_zVideo_D3DRenderState_TextureAddressU = (D3DTEXTUREADDRESS)(0);
    g_zVideo_D3DRenderState_TextureAddressV = (D3DTEXTUREADDRESS)(0);
    g_zVideo_OverwriteQueueCount = 1;

    zVideo_OverwriteQueueEntry &texturedEntry = g_zVideo_OverwriteQueueBase[0];
    texturedEntry.type = 4;
    texturedEntry.vertexCount = 3;
    texturedEntry.renderClass = reinterpret_cast<std::int32_t>(&texturedClass);
    texturedEntry.vertices[0].sx = 20.0f;
    zVideo_dd3d::FlushOverwritePolys();

    const bool texturedStateOk =
        gFakeD3DSetRenderStateCalls == 7 &&
        gFakeD3DRenderStates[0] == D3DRENDERSTATE_ZFUNC &&
        gFakeD3DRenderStateValues[0] == D3DCMP_ALWAYS &&
        gFakeD3DRenderStates[1] == D3DRENDERSTATE_SHADEMODE &&
        gFakeD3DRenderStateValues[1] == 1 &&
        gFakeD3DRenderStates[2] == D3DRENDERSTATE_TEXTUREHANDLE &&
        gFakeD3DRenderStateValues[2] == 0x4444 &&
        gFakeD3DRenderStates[3] == D3DRENDERSTATE_TEXTUREMAPBLEND &&
        gFakeD3DRenderStateValues[3] == 3 &&
        gFakeD3DRenderStates[4] == D3DRENDERSTATE_TEXTUREADDRESSU &&
        gFakeD3DRenderStateValues[4] == 5 &&
        gFakeD3DRenderStates[5] == D3DRENDERSTATE_TEXTUREADDRESSV &&
        gFakeD3DRenderStateValues[5] == 6 &&
        gFakeD3DRenderStates[6] == D3DRENDERSTATE_ZFUNC &&
        gFakeD3DRenderStateValues[6] == D3DCMP_GREATEREQUAL &&
        gFakeD3DDrawPrimitiveVertices[0] == texturedEntry.vertices &&
        gFakeD3DDrawPrimitiveVertexCounts[0] == 3 &&
        g_zVideo_OverwriteQueueCount == 0;

    zVideo_RenderClass modulateClass{};
    modulateClass.textureHandle = 0x5555;
    modulateClass.textureMapBlend = (D3DTEXTUREBLEND)(4);
    modulateClass.textureAddressU = (D3DTEXTUREADDRESS)(7);
    modulateClass.textureAddressV = (D3DTEXTUREADDRESS)(8);

    std::memset(
        g_zVideo_OverwriteQueueBase,
        0,
        sizeof(g_zVideo_OverwriteQueueBase[0])
    );
    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_D3DRenderState_ShadeMode = 1;
    g_zVideo_D3DRenderState_TextureHandle = 0;
    g_zVideo_D3DRenderState_TextureMapBlend = (D3DTEXTUREBLEND)(4);
    g_zVideo_D3DRenderState_TextureAddressU = (D3DTEXTUREADDRESS)(0);
    g_zVideo_D3DRenderState_TextureAddressV = (D3DTEXTUREADDRESS)(0);
    g_zVideo_OverwriteQueueCount = 1;

    zVideo_OverwriteQueueEntry &modulateEntry = g_zVideo_OverwriteQueueBase[0];
    modulateEntry.type = 6;
    modulateEntry.vertexCount = 5;
    modulateEntry.renderClass = reinterpret_cast<std::int32_t>(&modulateClass);
    modulateEntry.vertices[0].sx = 30.0f;
    zVideo_dd3d::FlushOverwritePolys();

    const bool modulateStateOk =
        gFakeD3DSetRenderStateCalls == 7 &&
        gFakeD3DRenderStates[0] == D3DRENDERSTATE_ZFUNC &&
        gFakeD3DRenderStateValues[0] == D3DCMP_ALWAYS &&
        gFakeD3DRenderStates[1] == D3DRENDERSTATE_SHADEMODE &&
        gFakeD3DRenderStateValues[1] == 2 &&
        gFakeD3DRenderStates[2] == D3DRENDERSTATE_TEXTUREHANDLE &&
        gFakeD3DRenderStateValues[2] == 0x5555 &&
        gFakeD3DRenderStates[3] == D3DRENDERSTATE_TEXTUREMAPBLEND &&
        gFakeD3DRenderStateValues[3] == 2 &&
        gFakeD3DRenderStates[4] == D3DRENDERSTATE_TEXTUREADDRESSU &&
        gFakeD3DRenderStateValues[4] == 7 &&
        gFakeD3DRenderStates[5] == D3DRENDERSTATE_TEXTUREADDRESSV &&
        gFakeD3DRenderStateValues[5] == 8 &&
        gFakeD3DRenderStates[6] == D3DRENDERSTATE_ZFUNC &&
        gFakeD3DRenderStateValues[6] == D3DCMP_GREATEREQUAL &&
        gFakeD3DDrawPrimitiveVertices[0] == modulateEntry.vertices &&
        gFakeD3DDrawPrimitiveVertexCounts[0] == 5 &&
        g_zVideo_D3DRenderState_TextureMapBlend == (D3DTEXTUREBLEND)(2) &&
        g_zVideo_OverwriteQueueCount == 0;

    std::memset(
        g_zVideo_OverwriteQueueBase,
        0,
        sizeof(g_zVideo_OverwriteQueueBase[0])
    );
    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_D3DRenderState_ShadeMode = 2;
    g_zVideo_D3DRenderState_TextureHandle = 0x9999;
    g_zVideo_OverwriteQueueCount = 1;

    zVideo_OverwriteQueueEntry &flatEntry = g_zVideo_OverwriteQueueBase[0];
    flatEntry.type = 2;
    flatEntry.vertexCount = 2;
    flatEntry.renderClass = 0;
    flatEntry.vertices[0].sx = 40.0f;
    zVideo_dd3d::FlushOverwritePolys();

    const bool flatStateOk =
        gFakeD3DSetRenderStateCalls == 4 &&
        gFakeD3DRenderStates[0] == D3DRENDERSTATE_ZFUNC &&
        gFakeD3DRenderStateValues[0] == D3DCMP_ALWAYS &&
        gFakeD3DRenderStates[1] == D3DRENDERSTATE_TEXTUREHANDLE &&
        gFakeD3DRenderStateValues[1] == 0 &&
        gFakeD3DRenderStates[2] == D3DRENDERSTATE_SHADEMODE &&
        gFakeD3DRenderStateValues[2] == 1 &&
        gFakeD3DRenderStates[3] == D3DRENDERSTATE_ZFUNC &&
        gFakeD3DRenderStateValues[3] == D3DCMP_GREATEREQUAL &&
        gFakeD3DDrawPrimitiveVertices[0] == flatEntry.vertices &&
        gFakeD3DDrawPrimitiveVertexCounts[0] == 2 &&
        g_zVideo_D3DRenderState_TextureHandle == 0 &&
        g_zVideo_D3DRenderState_ShadeMode == 1 &&
        g_zVideo_OverwriteQueueCount == 0;

    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_OverwriteQueueCount = savedOverwriteCount;
    g_zVideo_D3DRenderState_ShadeMode = savedShadeMode;
    g_zVideo_D3DRenderState_AlphaBlendEnable = savedAlphaBlendEnable;
    g_zVideo_D3DRenderState_ZWriteEnable = savedZWriteEnable;
    g_zVideo_D3DRenderState_TextureHandle = savedTextureHandle;
    g_zVideo_D3DRenderState_TextureMapBlend = savedTextureMapBlend;
    g_zVideo_D3DRenderState_TextureAddressU = savedTextureAddressU;
    g_zVideo_D3DRenderState_TextureAddressV = savedTextureAddressV;

    if (!transparentStateOk || !transparentDrawOk) {
        return 1;
    }
    if (!texturedStateOk) {
        return 2;
    }
    if (!modulateStateOk) {
        return 3;
    }
    return flatStateOk ? 0 : 4;
}

extern "C" int zvideo_flush_sorted_polys_empty_smoke(void) {
    g_zVideo_SortedPolyQueueCount = 0;
    g_zVideo_pD3DDevice = nullptr;
    zVideo_dd3d::FlushSortedPolys();
    return g_zVideo_SortedPolyQueueCount == 0 ? 0 : 1;
}

extern "C" int zvideo_flush_sorted_polys_smoke(void) {
    FakeD3DDevice2Object fakeDevice = {};
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    const int savedSortedCount = g_zVideo_SortedPolyQueueCount;
    const int savedShadeMode = g_zVideo_D3DRenderState_ShadeMode;
    const int savedAlphaBlendEnable = g_zVideo_D3DRenderState_AlphaBlendEnable;
    const int savedZWriteEnable = g_zVideo_D3DRenderState_ZWriteEnable;
    const D3DTEXTUREHANDLE savedTextureHandle = g_zVideo_D3DRenderState_TextureHandle;
    const D3DTEXTUREBLEND savedTextureMapBlend =
        g_zVideo_D3DRenderState_TextureMapBlend;
    const D3DTEXTUREADDRESS savedTextureAddressU =
        g_zVideo_D3DRenderState_TextureAddressU;
    const D3DTEXTUREADDRESS savedTextureAddressV =
        g_zVideo_D3DRenderState_TextureAddressV;

    std::memset(
        g_zVideo_SortedPolyQueueBase,
        0,
        sizeof(g_zVideo_SortedPolyQueueBase[0]) * 3
    );
    std::memset(g_zVideo_SortedPolyDrawOrder, 0, sizeof(g_zVideo_SortedPolyDrawOrder));
    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_D3DRenderState_ShadeMode = 1;
    g_zVideo_D3DRenderState_AlphaBlendEnable = 0;
    g_zVideo_D3DRenderState_ZWriteEnable = 1;
    g_zVideo_D3DRenderState_TextureHandle = 0x9999;
    g_zVideo_D3DRenderState_TextureMapBlend = (D3DTEXTUREBLEND)(2);
    g_zVideo_D3DRenderState_TextureAddressU = (D3DTEXTUREADDRESS)(0);
    g_zVideo_D3DRenderState_TextureAddressV = (D3DTEXTUREADDRESS)(0);

    zVideo_RenderClass farClass{};
    farClass.textureHandle = 0x1111;
    farClass.textureMapBlend = (D3DTEXTUREBLEND)(3);
    farClass.textureAddressU = (D3DTEXTUREADDRESS)(1);
    farClass.textureAddressV = (D3DTEXTUREADDRESS)(2);
    zVideo_RenderClass middleClass{};
    middleClass.textureHandle = 0x2222;
    middleClass.textureMapBlend = (D3DTEXTUREBLEND)(3);
    middleClass.textureAddressU = (D3DTEXTUREADDRESS)(5);
    middleClass.textureAddressV = (D3DTEXTUREADDRESS)(6);

    zVideo_SortedPolyQueueEntry &farEntry = g_zVideo_SortedPolyQueueBase[0];
    farEntry.vertexCount = 3;
    farEntry.renderClass = reinterpret_cast<std::int32_t>(&farClass);
    farEntry.vertices[0].sx = 30.0f;
    farEntry.vertices[0].sz = 30.0f;
    farEntry.vertices[0].color = 0xff445566;

    zVideo_SortedPolyQueueEntry &nearEntry = g_zVideo_SortedPolyQueueBase[1];
    nearEntry.vertexCount = 1;
    nearEntry.renderClass = 0;
    nearEntry.vertices[0].sx = 10.0f;
    nearEntry.vertices[0].sz = 10.0f;
    nearEntry.vertices[0].color = 0xff101010;

    zVideo_SortedPolyQueueEntry &middleEntry = g_zVideo_SortedPolyQueueBase[2];
    middleEntry.vertexCount = 2;
    middleEntry.renderClass = reinterpret_cast<std::int32_t>(&middleClass);
    middleEntry.vertices[0].sx = 20.0f;
    middleEntry.vertices[0].sz = 20.0f;
    middleEntry.vertices[0].color = 0x7f112233;

    g_zVideo_SortedPolyQueueCount = 3;
    zVideo_dd3d::FlushSortedPolys();

    const bool initialStateOk =
        gFakeD3DRenderStates[0] == D3DRENDERSTATE_SHADEMODE &&
        gFakeD3DRenderStateValues[0] == 2 &&
        gFakeD3DRenderStates[1] == D3DRENDERSTATE_ALPHABLENDENABLE &&
        gFakeD3DRenderStateValues[1] == 1 &&
        gFakeD3DRenderStates[2] == D3DRENDERSTATE_ZWRITEENABLE &&
        gFakeD3DRenderStateValues[2] == 0 &&
        gFakeD3DRenderStates[3] == D3DRENDERSTATE_TEXTUREHANDLE &&
        gFakeD3DRenderStateValues[3] == 0;

    const bool middleStateOk =
        gFakeD3DRenderStates[4] == D3DRENDERSTATE_TEXTUREHANDLE &&
        gFakeD3DRenderStateValues[4] == 0x2222 &&
        gFakeD3DRenderStates[5] == D3DRENDERSTATE_TEXTUREMAPBLEND &&
        gFakeD3DRenderStateValues[5] == 4 &&
        gFakeD3DRenderStates[6] == D3DRENDERSTATE_TEXTUREADDRESSU &&
        gFakeD3DRenderStateValues[6] == 5 &&
        gFakeD3DRenderStates[7] == D3DRENDERSTATE_TEXTUREADDRESSV &&
        gFakeD3DRenderStateValues[7] == 6;

    const bool farStateOk =
        gFakeD3DRenderStates[8] == D3DRENDERSTATE_TEXTUREHANDLE &&
        gFakeD3DRenderStateValues[8] == 0x1111 &&
        gFakeD3DRenderStates[9] == D3DRENDERSTATE_TEXTUREMAPBLEND &&
        gFakeD3DRenderStateValues[9] == 3 &&
        gFakeD3DRenderStates[10] == D3DRENDERSTATE_TEXTUREADDRESSU &&
        gFakeD3DRenderStateValues[10] == 1 &&
        gFakeD3DRenderStates[11] == D3DRENDERSTATE_TEXTUREADDRESSV &&
        gFakeD3DRenderStateValues[11] == 2;

    const bool restoreStateOk =
        gFakeD3DSetRenderStateCalls == 14 &&
        gFakeD3DRenderStates[12] == D3DRENDERSTATE_ALPHABLENDENABLE &&
        gFakeD3DRenderStateValues[12] == 0 &&
        gFakeD3DRenderStates[13] == D3DRENDERSTATE_ZWRITEENABLE &&
        gFakeD3DRenderStateValues[13] == 1 &&
        g_zVideo_D3DRenderState_ShadeMode == 2 &&
        g_zVideo_D3DRenderState_AlphaBlendEnable == 0 &&
        g_zVideo_D3DRenderState_ZWriteEnable == 1 &&
        g_zVideo_D3DRenderState_TextureHandle == 0x1111 &&
        g_zVideo_D3DRenderState_TextureMapBlend == (D3DTEXTUREBLEND)(3) &&
        g_zVideo_D3DRenderState_TextureAddressU == (D3DTEXTUREADDRESS)(1) &&
        g_zVideo_D3DRenderState_TextureAddressV == (D3DTEXTUREADDRESS)(2);

    const bool drawOrderOk =
        g_zVideo_SortedPolyDrawOrder[0] == 1 &&
        g_zVideo_SortedPolyDrawOrder[1] == 2 &&
        g_zVideo_SortedPolyDrawOrder[2] == 0 &&
        gFakeD3DDrawPrimitiveCalls == 3 &&
        gFakeD3DDrawPrimitiveVertices[0] == nearEntry.vertices &&
        gFakeD3DDrawPrimitiveVertexCounts[0] == 1 &&
        gFakeD3DDrawPrimitiveVertices[1] == middleEntry.vertices &&
        gFakeD3DDrawPrimitiveVertexCounts[1] == 2 &&
        gFakeD3DDrawPrimitiveVertices[2] == farEntry.vertices &&
        gFakeD3DDrawPrimitiveVertexCounts[2] == 3;

    const bool drawArgsOk =
        gFakeD3DDrawPrimitiveTypes[0] == (D3DPRIMITIVETYPE)(6) &&
        gFakeD3DDrawPrimitiveTypes[1] == (D3DPRIMITIVETYPE)(6) &&
        gFakeD3DDrawPrimitiveTypes[2] == (D3DPRIMITIVETYPE)(6) &&
        gFakeD3DDrawPrimitiveVertexTypes[0] == (D3DVERTEXTYPE)(3) &&
        gFakeD3DDrawPrimitiveVertexTypes[1] == (D3DVERTEXTYPE)(3) &&
        gFakeD3DDrawPrimitiveVertexTypes[2] == (D3DVERTEXTYPE)(3) &&
        gFakeD3DDrawPrimitiveFlags[0] == 0 &&
        gFakeD3DDrawPrimitiveFlags[1] == 0 &&
        gFakeD3DDrawPrimitiveFlags[2] == 0 &&
        g_zVideo_SortedPolyQueueCount == 0;

    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_SortedPolyQueueCount = savedSortedCount;
    g_zVideo_D3DRenderState_ShadeMode = savedShadeMode;
    g_zVideo_D3DRenderState_AlphaBlendEnable = savedAlphaBlendEnable;
    g_zVideo_D3DRenderState_ZWriteEnable = savedZWriteEnable;
    g_zVideo_D3DRenderState_TextureHandle = savedTextureHandle;
    g_zVideo_D3DRenderState_TextureMapBlend = savedTextureMapBlend;
    g_zVideo_D3DRenderState_TextureAddressU = savedTextureAddressU;
    g_zVideo_D3DRenderState_TextureAddressV = savedTextureAddressV;

    if (!initialStateOk) {
        return 1;
    }
    if (!middleStateOk) {
        return 2;
    }
    if (!farStateOk) {
        return 3;
    }
    if (!restoreStateOk) {
        return 4;
    }
    if (!drawOrderOk) {
        return 5;
    }
    return drawArgsOk ? 0 : 6;
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

extern "C" int zvideo_submit_poly_flat_color16_immediate_smoke(void) {
    FakeD3DDevice2Object fakeDevice = {};
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    const D3DTEXTUREHANDLE savedTextureHandle = g_zVideo_D3DRenderState_TextureHandle;
    const int savedShadeMode = g_zVideo_D3DRenderState_ShadeMode;

    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);
    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_D3DRenderState_TextureHandle = 0x2468;
    g_zVideo_D3DRenderState_ShadeMode = 2;

    zVideo_XyzVertex vertices[3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f},
    };

    zVideo_dd3d::SubmitPolyFlatColor16(vertices, 0x001f, 0xff, 0x1111, 3, 0);

    const bool renderStateOk =
        gFakeD3DSetRenderStateCalls == 2 &&
        gFakeD3DRenderStates[0] == D3DRENDERSTATE_TEXTUREHANDLE &&
        gFakeD3DRenderStateValues[0] == 0 &&
        gFakeD3DRenderStates[1] == D3DRENDERSTATE_SHADEMODE &&
        gFakeD3DRenderStateValues[1] == 1 &&
        g_zVideo_D3DRenderState_TextureHandle == 0 &&
        g_zVideo_D3DRenderState_ShadeMode == 1;

    const bool drawOk =
        gFakeD3DDrawPrimitiveCalls == 1 &&
        gFakeD3DLastPrimitiveType == (D3DPRIMITIVETYPE)(6) &&
        gFakeD3DLastVertexType == (D3DVERTEXTYPE)(3) &&
        gFakeD3DLastVertices == g_zVideo_D3DSubmitTempVertices &&
        gFakeD3DLastVertexCount == 3 &&
        gFakeD3DLastDrawFlags == 0;

    const D3DTLVERTEX &first = g_zVideo_D3DSubmitTempVertices[0];
    const D3DTLVERTEX &last = g_zVideo_D3DSubmitTempVertices[2];
    const bool verticesOk =
        first.sx == 7.0f &&
        first.sy == 8.0f &&
        first.sz == 9.0f &&
        first.rhw == 9.0f &&
        first.color == 0xff0000f8 &&
        first.specular == 0xff000000 &&
        last.sx == 1.0f &&
        last.sy == 2.0f &&
        last.sz == 3.0f &&
        last.rhw == 3.0f &&
        last.color == 0xff0000f8 &&
        last.specular == 0xff000000;

    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_D3DRenderState_TextureHandle = 0;
    g_zVideo_D3DRenderState_ShadeMode = 1;
    zVideo_dd3d::SubmitPolyFlatColor16(vertices, 0x07e0, 0xff, 0x2222, 2, 0);
    const bool cacheHitOk =
        gFakeD3DSetRenderStateCalls == 0 &&
        gFakeD3DDrawPrimitiveCalls == 1 &&
        gFakeD3DLastVertexCount == 2 &&
        g_zVideo_D3DSubmitTempVertices[0].color == 0xff00fc00;

    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_D3DRenderState_TextureHandle = savedTextureHandle;
    g_zVideo_D3DRenderState_ShadeMode = savedShadeMode;
    return renderStateOk && drawOk && verticesOk && cacheHitOk ? 0 : 1;
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

extern "C" int zvideo_submit_poly_gouraud_color16_immediate_smoke(void) {
    FakeD3DDevice2Object fakeDevice = {};
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    const D3DTEXTUREHANDLE savedTextureHandle = g_zVideo_D3DRenderState_TextureHandle;
    const int savedShadeMode = g_zVideo_D3DRenderState_ShadeMode;

    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);
    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_D3DRenderState_TextureHandle = 0x2468;
    g_zVideo_D3DRenderState_ShadeMode = 2;

    zVideo_XyzVertex vertices[3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f},
    };
    std::uint32_t colors[3] = {0xf800, 0x07e0, 0x001f};

    zVideo_dd3d::SubmitPolyGouraudColor16(vertices, colors, 0xff, 0x1111, 3, 0);

    const bool renderStateOk =
        gFakeD3DSetRenderStateCalls == 2 &&
        gFakeD3DRenderStates[0] == D3DRENDERSTATE_TEXTUREHANDLE &&
        gFakeD3DRenderStateValues[0] == 0 &&
        gFakeD3DRenderStates[1] == D3DRENDERSTATE_SHADEMODE &&
        gFakeD3DRenderStateValues[1] == 1 &&
        g_zVideo_D3DRenderState_TextureHandle == 0 &&
        g_zVideo_D3DRenderState_ShadeMode == 1;

    const bool drawOk =
        gFakeD3DDrawPrimitiveCalls == 1 &&
        gFakeD3DLastPrimitiveType == (D3DPRIMITIVETYPE)(6) &&
        gFakeD3DLastVertexType == (D3DVERTEXTYPE)(3) &&
        gFakeD3DLastVertices == g_zVideo_D3DSubmitTempVertices &&
        gFakeD3DLastVertexCount == 3 &&
        gFakeD3DLastDrawFlags == 0;

    const D3DTLVERTEX &first = g_zVideo_D3DSubmitTempVertices[0];
    const D3DTLVERTEX &last = g_zVideo_D3DSubmitTempVertices[2];
    const bool verticesOk =
        first.sx == 7.0f &&
        first.sy == 8.0f &&
        first.sz == 9.0f &&
        first.rhw == 9.0f &&
        first.color == 0xff0000f8 &&
        first.specular == 0xff000000 &&
        last.sx == 1.0f &&
        last.sy == 2.0f &&
        last.sz == 3.0f &&
        last.rhw == 3.0f &&
        last.color == 0xfff80000 &&
        last.specular == 0xff000000;

    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_D3DRenderState_TextureHandle = 0;
    g_zVideo_D3DRenderState_ShadeMode = 1;
    zVideo_dd3d::SubmitPolyGouraudColor16(vertices, colors, 0xff, 0x2222, 2, 0);
    const bool cacheHitOk =
        gFakeD3DSetRenderStateCalls == 0 &&
        gFakeD3DDrawPrimitiveCalls == 1 &&
        gFakeD3DLastVertexCount == 2 &&
        g_zVideo_D3DSubmitTempVertices[0].color == 0xff00fc00;

    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_D3DRenderState_TextureHandle = savedTextureHandle;
    g_zVideo_D3DRenderState_ShadeMode = savedShadeMode;
    return renderStateOk && drawOk && verticesOk && cacheHitOk ? 0 : 1;
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

extern "C" int zvideo_submit_poly_color_attr_immediate_smoke(void) {
    FakeD3DDevice2Object fakeDevice = {};
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    const D3DTEXTUREHANDLE savedTextureHandle = g_zVideo_D3DRenderState_TextureHandle;
    const int savedShadeMode = g_zVideo_D3DRenderState_ShadeMode;

    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;
    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_D3DRenderState_TextureHandle = 0x2468;
    g_zVideo_D3DRenderState_ShadeMode = 2;

    zVideo_XyzVertex vertices[3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f},
    };
    zVideo_ColorRgbFloat baseColor{16.0f, 32.0f, 48.0f};
    float attr1[1] = {0.0f};
    float attr2[3] = {0.0f, 0.5f, 1.0f};

    zVideo_dd3d::SubmitPolyColorAttr(
        vertices,
        0,
        &baseColor,
        attr1,
        nullptr,
        attr2,
        0xff,
        3,
        0x66,
        0
    );

    const bool renderStateOk =
        gFakeD3DSetRenderStateCalls == 2 &&
        gFakeD3DRenderStates[0] == D3DRENDERSTATE_TEXTUREHANDLE &&
        gFakeD3DRenderStateValues[0] == 0 &&
        gFakeD3DRenderStates[1] == D3DRENDERSTATE_SHADEMODE &&
        gFakeD3DRenderStateValues[1] == 1 &&
        g_zVideo_D3DRenderState_TextureHandle == 0 &&
        g_zVideo_D3DRenderState_ShadeMode == 1;

    const bool drawOk =
        gFakeD3DDrawPrimitiveCalls == 1 &&
        gFakeD3DLastPrimitiveType == (D3DPRIMITIVETYPE)(6) &&
        gFakeD3DLastVertexType == (D3DVERTEXTYPE)(3) &&
        gFakeD3DLastVertices == g_zVideo_D3DSubmitTempVertices &&
        gFakeD3DLastVertexCount == 3 &&
        gFakeD3DLastDrawFlags == 0 &&
        g_zVideo_OverwriteQueueCount == 0 &&
        g_zVideo_SortedPolyQueueCount == 0;

    const D3DTLVERTEX &first = g_zVideo_D3DSubmitTempVertices[0];
    const D3DTLVERTEX &last = g_zVideo_D3DSubmitTempVertices[2];
    const bool verticesOk =
        first.sx == 7.0f &&
        first.sy == 8.0f &&
        first.sz == 9.0f &&
        first.rhw == 9.0f &&
        first.color == 0xff102030 &&
        first.specular == 0x00000000 &&
        last.sx == 1.0f &&
        last.sy == 2.0f &&
        last.sz == 3.0f &&
        last.rhw == 3.0f &&
        last.color == 0xff102030 &&
        last.specular == 0xff000000;

    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_D3DRenderState_TextureHandle = 0;
    g_zVideo_D3DRenderState_ShadeMode = 1;
    zVideo_dd3d::SubmitPolyColorAttr(
        vertices,
        0,
        &baseColor,
        attr1,
        nullptr,
        nullptr,
        0xff,
        2,
        0x77,
        0
    );
    const bool cacheHitOk =
        gFakeD3DSetRenderStateCalls == 0 &&
        gFakeD3DDrawPrimitiveCalls == 1 &&
        gFakeD3DLastVertexCount == 2 &&
        g_zVideo_D3DSubmitTempVertices[0].sx == 4.0f &&
        g_zVideo_D3DSubmitTempVertices[0].color == 0xff102030 &&
        g_zVideo_D3DSubmitTempVertices[0].specular == 0xff000000;

    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_D3DRenderState_TextureHandle = savedTextureHandle;
    g_zVideo_D3DRenderState_ShadeMode = savedShadeMode;
    return renderStateOk && drawOk && verticesOk && cacheHitOk ? 0 : 1;
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

extern "C" int zvideo_submit_poly_render_class_immediate_smoke(void) {
    FakeD3DDevice2Object fakeDevice = {};
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    const int savedShadeMode = g_zVideo_D3DRenderState_ShadeMode;
    const D3DTEXTUREHANDLE savedTextureHandle = g_zVideo_D3DRenderState_TextureHandle;
    const D3DTEXTUREBLEND savedTextureMapBlend =
        g_zVideo_D3DRenderState_TextureMapBlend;
    const D3DTEXTUREADDRESS savedTextureAddressU =
        g_zVideo_D3DRenderState_TextureAddressU;
    const D3DTEXTUREADDRESS savedTextureAddressV =
        g_zVideo_D3DRenderState_TextureAddressV;

    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;
    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_D3DRenderState_ShadeMode = 2;
    g_zVideo_D3DRenderState_TextureHandle = 0;
    g_zVideo_D3DRenderState_TextureMapBlend = (D3DTEXTUREBLEND)(1);
    g_zVideo_D3DRenderState_TextureAddressU = (D3DTEXTUREADDRESS)(3);
    g_zVideo_D3DRenderState_TextureAddressV = (D3DTEXTUREADDRESS)(3);

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
    renderClass.textureMapBlend = (D3DTEXTUREBLEND)(3);
    renderClass.textureAddressU = (D3DTEXTUREADDRESS)(1);
    renderClass.textureAddressV = (D3DTEXTUREADDRESS)(2);

    zVideo_dd3d::SubmitPolyRenderClass(
        vertices,
        texCoords,
        3,
        &renderClass,
        0x77,
        1.0f,
        0
    );

    const bool renderStateOk =
        gFakeD3DSetRenderStateCalls == 5 &&
        gFakeD3DRenderStates[0] == D3DRENDERSTATE_SHADEMODE &&
        gFakeD3DRenderStateValues[0] == 1 &&
        gFakeD3DRenderStates[1] == D3DRENDERSTATE_TEXTUREHANDLE &&
        gFakeD3DRenderStateValues[1] == 0x1234 &&
        gFakeD3DRenderStates[2] == D3DRENDERSTATE_TEXTUREMAPBLEND &&
        gFakeD3DRenderStateValues[2] == 3 &&
        gFakeD3DRenderStates[3] == D3DRENDERSTATE_TEXTUREADDRESSU &&
        gFakeD3DRenderStateValues[3] == 1 &&
        gFakeD3DRenderStates[4] == D3DRENDERSTATE_TEXTUREADDRESSV &&
        gFakeD3DRenderStateValues[4] == 2 &&
        g_zVideo_D3DRenderState_ShadeMode == 1 &&
        g_zVideo_D3DRenderState_TextureHandle == 0x1234 &&
        g_zVideo_D3DRenderState_TextureMapBlend == (D3DTEXTUREBLEND)(3) &&
        g_zVideo_D3DRenderState_TextureAddressU == (D3DTEXTUREADDRESS)(1) &&
        g_zVideo_D3DRenderState_TextureAddressV == (D3DTEXTUREADDRESS)(2);

    const bool drawOk =
        gFakeD3DDrawPrimitiveCalls == 1 &&
        gFakeD3DLastPrimitiveType == (D3DPRIMITIVETYPE)(6) &&
        gFakeD3DLastVertexType == (D3DVERTEXTYPE)(3) &&
        gFakeD3DLastVertices == g_zVideo_D3DSubmitTempVertices &&
        gFakeD3DLastVertexCount == 3 &&
        gFakeD3DLastDrawFlags == 0 &&
        g_zVideo_OverwriteQueueCount == 0 &&
        g_zVideo_SortedPolyQueueCount == 0;

    const D3DTLVERTEX &first = g_zVideo_D3DSubmitTempVertices[0];
    const D3DTLVERTEX &last = g_zVideo_D3DSubmitTempVertices[2];
    const bool verticesOk =
        first.sx == 7.0f &&
        first.sy == 8.0f &&
        first.sz == 9.0f &&
        first.rhw == 9.0f &&
        first.color == 0xffffffff &&
        first.specular == 0xff000000 &&
        first.tu == 0.5f &&
        first.tv == 0.6f &&
        last.sx == 1.0f &&
        last.sy == 2.0f &&
        last.sz == 3.0f &&
        last.rhw == 3.0f &&
        last.color == 0xffffffff &&
        last.specular == 0xff000000 &&
        last.tu == 0.1f &&
        last.tv == 0.2f;

    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_D3DRenderState_ShadeMode = 1;
    g_zVideo_D3DRenderState_TextureHandle = 0x1234;
    g_zVideo_D3DRenderState_TextureMapBlend = (D3DTEXTUREBLEND)(3);
    g_zVideo_D3DRenderState_TextureAddressU = (D3DTEXTUREADDRESS)(1);
    g_zVideo_D3DRenderState_TextureAddressV = (D3DTEXTUREADDRESS)(2);
    zVideo_dd3d::SubmitPolyRenderClass(
        vertices,
        texCoords,
        2,
        &renderClass,
        0x88,
        1.0f,
        0
    );
    const bool cacheHitOk =
        gFakeD3DSetRenderStateCalls == 0 &&
        gFakeD3DDrawPrimitiveCalls == 1 &&
        gFakeD3DLastVertexCount == 2 &&
        g_zVideo_D3DSubmitTempVertices[0].sx == 4.0f &&
        g_zVideo_D3DSubmitTempVertices[0].tu == 0.3f &&
        g_zVideo_D3DSubmitTempVertices[0].tv == 0.4f;

    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_D3DRenderState_ShadeMode = savedShadeMode;
    g_zVideo_D3DRenderState_TextureHandle = savedTextureHandle;
    g_zVideo_D3DRenderState_TextureMapBlend = savedTextureMapBlend;
    g_zVideo_D3DRenderState_TextureAddressU = savedTextureAddressU;
    g_zVideo_D3DRenderState_TextureAddressV = savedTextureAddressV;
    return renderStateOk && drawOk && verticesOk && cacheHitOk ? 0 : 1;
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

extern "C" int zvideo_submit_polygon_immediate_smoke(void) {
    FakeD3DDevice2Object fakeDevice = {};
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    const int savedShadeMode = g_zVideo_D3DRenderState_ShadeMode;
    const D3DTEXTUREHANDLE savedTextureHandle = g_zVideo_D3DRenderState_TextureHandle;
    const D3DTEXTUREBLEND savedTextureMapBlend =
        g_zVideo_D3DRenderState_TextureMapBlend;
    const D3DTEXTUREADDRESS savedTextureAddressU =
        g_zVideo_D3DRenderState_TextureAddressU;
    const D3DTEXTUREADDRESS savedTextureAddressV =
        g_zVideo_D3DRenderState_TextureAddressV;
    const int savedAppendFanClose = g_zVideo_D3DAppendFanCloseVertexPending;
    const int savedNormalizeChannel = g_zVideo_D3DColorNormalizeChannelIndex;
    const float savedBiasR = g_zVideo_D3DColorAttrBiasR;
    const float savedBiasG = g_zVideo_D3DColorAttrBiasG;
    const float savedBiasB = g_zVideo_D3DColorAttrBiasB;

    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    std::memset(g_zVideo_OverwriteQueueBase, 0, sizeof(g_zVideo_OverwriteQueueBase[0]));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;
    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_D3DRenderState_ShadeMode = 1;
    g_zVideo_D3DRenderState_TextureHandle = 0;
    g_zVideo_D3DRenderState_TextureMapBlend = (D3DTEXTUREBLEND)(1);
    g_zVideo_D3DRenderState_TextureAddressU = (D3DTEXTUREADDRESS)(3);
    g_zVideo_D3DRenderState_TextureAddressV = (D3DTEXTUREADDRESS)(3);
    g_zVideo_D3DAppendFanCloseVertexPending = 1;

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
    renderClass.textureMapBlend = (D3DTEXTUREBLEND)(3);
    renderClass.textureAddressU = (D3DTEXTUREADDRESS)(1);
    renderClass.textureAddressV = (D3DTEXTUREADDRESS)(2);
    float attr1[1] = {0.0f};

    zVideo_dd3d::SubmitPolygon(
        vertices,
        uvPairs,
        attr1,
        nullptr,
        nullptr,
        3,
        &renderClass,
        0x66,
        1.0f,
        0
    );

    const bool renderStateOk =
        gFakeD3DSetRenderStateCalls == 5 &&
        gFakeD3DRenderStates[0] == D3DRENDERSTATE_SHADEMODE &&
        gFakeD3DRenderStateValues[0] == 2 &&
        gFakeD3DRenderStates[1] == D3DRENDERSTATE_TEXTUREHANDLE &&
        gFakeD3DRenderStateValues[1] == 0x2345 &&
        gFakeD3DRenderStates[2] == D3DRENDERSTATE_TEXTUREMAPBLEND &&
        gFakeD3DRenderStateValues[2] == 2 &&
        gFakeD3DRenderStates[3] == D3DRENDERSTATE_TEXTUREADDRESSU &&
        gFakeD3DRenderStateValues[3] == 1 &&
        gFakeD3DRenderStates[4] == D3DRENDERSTATE_TEXTUREADDRESSV &&
        gFakeD3DRenderStateValues[4] == 2 &&
        g_zVideo_D3DRenderState_ShadeMode == 2 &&
        g_zVideo_D3DRenderState_TextureHandle == 0x2345 &&
        g_zVideo_D3DRenderState_TextureMapBlend == (D3DTEXTUREBLEND)(2) &&
        g_zVideo_D3DRenderState_TextureAddressU == (D3DTEXTUREADDRESS)(1) &&
        g_zVideo_D3DRenderState_TextureAddressV == (D3DTEXTUREADDRESS)(2);

    const bool drawOk =
        gFakeD3DDrawPrimitiveCalls == 1 &&
        gFakeD3DLastPrimitiveType == (D3DPRIMITIVETYPE)(6) &&
        gFakeD3DLastVertexType == (D3DVERTEXTYPE)(3) &&
        gFakeD3DLastVertices == g_zVideo_D3DSubmitTempVertices &&
        gFakeD3DLastVertexCount == 4 &&
        gFakeD3DLastDrawFlags == 0 &&
        g_zVideo_OverwriteQueueCount == 0 &&
        g_zVideo_SortedPolyQueueCount == 0 &&
        g_zVideo_D3DAppendFanCloseVertexPending == 0;

    const D3DTLVERTEX &first = g_zVideo_D3DSubmitTempVertices[0];
    const D3DTLVERTEX &close = g_zVideo_D3DSubmitTempVertices[3];
    const bool verticesOk =
        first.sx == 7.0f &&
        first.sy == 8.0f &&
        first.sz == 9.0f &&
        first.rhw == 9.0f &&
        first.color == 0xffffffff &&
        first.specular == 0xff000000 &&
        first.tu == 0.5f &&
        first.tv == 0.6f &&
        close.sx == 4.0f &&
        close.sy == 5.0f &&
        close.sz == 6.0f &&
        close.rhw == 6.0f &&
        close.color == 0xffffffff &&
        close.specular == 0xff000000 &&
        close.tu == 0.3f &&
        close.tv == 0.4f;

    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_D3DRenderState_ShadeMode = 2;
    g_zVideo_D3DRenderState_TextureHandle = 0x2345;
    g_zVideo_D3DRenderState_TextureMapBlend = (D3DTEXTUREBLEND)(2);
    g_zVideo_D3DRenderState_TextureAddressU = (D3DTEXTUREADDRESS)(1);
    g_zVideo_D3DRenderState_TextureAddressV = (D3DTEXTUREADDRESS)(2);
    g_zVideo_D3DAppendFanCloseVertexPending = 0;
    zVideo_dd3d::SubmitPolygon(
        vertices,
        uvPairs,
        attr1,
        nullptr,
        nullptr,
        2,
        &renderClass,
        0x77,
        1.0f,
        0
    );
    const bool cacheHitOk =
        gFakeD3DSetRenderStateCalls == 0 &&
        gFakeD3DDrawPrimitiveCalls == 1 &&
        gFakeD3DLastVertexCount == 2 &&
        g_zVideo_D3DSubmitTempVertices[0].sx == 4.0f &&
        g_zVideo_D3DSubmitTempVertices[0].tu == 0.3f &&
        g_zVideo_D3DSubmitTempVertices[0].tv == 0.4f;

    std::memset(g_zVideo_OverwriteQueueBase, 0, sizeof(g_zVideo_OverwriteQueueBase[0]));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;
    g_zVideo_D3DAppendFanCloseVertexPending = 1;
    renderClass.textureMapBlend = (D3DTEXTUREBLEND)(4);
    attr1[0] = 0.5f;
    float attr0[2] = {0.0f, 0.5f};
    float attr2[2] = {0.0f, 1.0f};
    g_zVideo_D3DColorNormalizeChannelIndex = 2;
    g_zVideo_D3DColorAttrBiasR = 100.0f;
    g_zVideo_D3DColorAttrBiasG = 200.0f;
    g_zVideo_D3DColorAttrBiasB = 300.0f;
    zVideo_dd3d::SubmitPolygon(
        vertices,
        uvPairs,
        attr1,
        attr0,
        attr2,
        2,
        &renderClass,
        0x88,
        0.5f,
        1
    );

    const D3DTLVERTEX &transparentFirst = g_zVideo_OverwriteQueueBase[0].vertices[0];
    const D3DTLVERTEX &transparentClose = g_zVideo_OverwriteQueueBase[0].vertices[2];
    const bool transparentQueueOk =
        g_zVideo_OverwriteQueueCount == 1 &&
        g_zVideo_OverwriteQueueBase[0].type == 0 &&
        g_zVideo_OverwriteQueueBase[0].vertexCount == 3 &&
        g_zVideo_OverwriteQueueBase[0].renderClass ==
            reinterpret_cast<std::int32_t>(&renderClass) &&
        g_zVideo_OverwriteQueueBase[0].renderParam == 0x88 &&
        g_zVideo_SortedPolyQueueCount == 0 &&
        g_zVideo_D3DAppendFanCloseVertexPending == 0;
    const bool transparentFirstPositionOk = transparentFirst.sx == 4.0f;
    const bool transparentFirstColorOk = transparentFirst.color == 0x7fa3d1ff;
    const bool transparentFirstSpecOk = transparentFirst.specular == 0x00000000;
    const bool transparentFirstUvOk = transparentFirst.tu == 0.3f;
    const bool transparentCloseOk =
        transparentClose.sx == 1.0f &&
        transparentClose.color == 0x7f7f7f7f &&
        transparentClose.specular == 0xff000000 &&
        transparentClose.tu == 0.1f;

    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_D3DRenderState_ShadeMode = savedShadeMode;
    g_zVideo_D3DRenderState_TextureHandle = savedTextureHandle;
    g_zVideo_D3DRenderState_TextureMapBlend = savedTextureMapBlend;
    g_zVideo_D3DRenderState_TextureAddressU = savedTextureAddressU;
    g_zVideo_D3DRenderState_TextureAddressV = savedTextureAddressV;
    g_zVideo_D3DAppendFanCloseVertexPending = savedAppendFanClose;
    g_zVideo_D3DColorNormalizeChannelIndex = savedNormalizeChannel;
    g_zVideo_D3DColorAttrBiasR = savedBiasR;
    g_zVideo_D3DColorAttrBiasG = savedBiasG;
    g_zVideo_D3DColorAttrBiasB = savedBiasB;
    if (!renderStateOk) {
        return 1;
    }
    if (!drawOk) {
        return 2;
    }
    if (!verticesOk) {
        return 3;
    }
    if (!cacheHitOk) {
        return 4;
    }
    if (!transparentQueueOk) {
        return 5;
    }
    if (!transparentFirstPositionOk) {
        return 6;
    }
    if (!transparentFirstColorOk) {
        return 8;
    }
    if (!transparentFirstSpecOk) {
        return 9;
    }
    if (!transparentFirstUvOk) {
        return 10;
    }
    return transparentCloseOk ? 0 : 7;
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

extern "C" int zvideo_submit_polygon_lit_immediate_smoke(void) {
    FakeD3DDevice2Object fakeDevice = {};
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    const int savedShadeMode = g_zVideo_D3DRenderState_ShadeMode;
    const D3DTEXTUREHANDLE savedTextureHandle = g_zVideo_D3DRenderState_TextureHandle;
    const D3DTEXTUREBLEND savedTextureMapBlend =
        g_zVideo_D3DRenderState_TextureMapBlend;
    const D3DTEXTUREADDRESS savedTextureAddressU =
        g_zVideo_D3DRenderState_TextureAddressU;
    const D3DTEXTUREADDRESS savedTextureAddressV =
        g_zVideo_D3DRenderState_TextureAddressV;
    const int savedAppendFanClose = g_zVideo_D3DAppendFanCloseVertexPending;
    const int savedNormalizeChannel = g_zVideo_D3DColorNormalizeChannelIndex;
    const float savedBiasR = g_zVideo_D3DColorAttrBiasR;
    const float savedBiasG = g_zVideo_D3DColorAttrBiasG;
    const float savedBiasB = g_zVideo_D3DColorAttrBiasB;

    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    std::memset(g_zVideo_OverwriteQueueBase, 0, sizeof(g_zVideo_OverwriteQueueBase[0]));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;
    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_D3DRenderState_ShadeMode = 1;
    g_zVideo_D3DRenderState_TextureHandle = 0;
    g_zVideo_D3DRenderState_TextureMapBlend = (D3DTEXTUREBLEND)(1);
    g_zVideo_D3DRenderState_TextureAddressU = (D3DTEXTUREADDRESS)(3);
    g_zVideo_D3DRenderState_TextureAddressV = (D3DTEXTUREADDRESS)(3);
    g_zVideo_D3DAppendFanCloseVertexPending = 1;

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
    renderClass.textureHandle = 0x3456;
    renderClass.textureMapBlend = (D3DTEXTUREBLEND)(3);
    renderClass.textureAddressU = (D3DTEXTUREADDRESS)(1);
    renderClass.textureAddressV = (D3DTEXTUREADDRESS)(2);
    float attr1[3] = {0.0f, 0.5f, 1.0f};

    zVideo_dd3d::SubmitPolygonLit(
        vertices,
        uvPairs,
        attr1,
        nullptr,
        nullptr,
        3,
        &renderClass,
        0x66,
        1.0f,
        0
    );

    const bool renderStateOk =
        gFakeD3DSetRenderStateCalls == 5 &&
        gFakeD3DRenderStates[0] == D3DRENDERSTATE_SHADEMODE &&
        gFakeD3DRenderStateValues[0] == 2 &&
        gFakeD3DRenderStates[1] == D3DRENDERSTATE_TEXTUREHANDLE &&
        gFakeD3DRenderStateValues[1] == 0x3456 &&
        gFakeD3DRenderStates[2] == D3DRENDERSTATE_TEXTUREMAPBLEND &&
        gFakeD3DRenderStateValues[2] == 2 &&
        gFakeD3DRenderStates[3] == D3DRENDERSTATE_TEXTUREADDRESSU &&
        gFakeD3DRenderStateValues[3] == 1 &&
        gFakeD3DRenderStates[4] == D3DRENDERSTATE_TEXTUREADDRESSV &&
        gFakeD3DRenderStateValues[4] == 2 &&
        g_zVideo_D3DRenderState_ShadeMode == 2 &&
        g_zVideo_D3DRenderState_TextureHandle == 0x3456 &&
        g_zVideo_D3DRenderState_TextureMapBlend == (D3DTEXTUREBLEND)(2) &&
        g_zVideo_D3DRenderState_TextureAddressU == (D3DTEXTUREADDRESS)(1) &&
        g_zVideo_D3DRenderState_TextureAddressV == (D3DTEXTUREADDRESS)(2);

    const bool drawOk =
        gFakeD3DDrawPrimitiveCalls == 1 &&
        gFakeD3DLastPrimitiveType == (D3DPRIMITIVETYPE)(6) &&
        gFakeD3DLastVertexType == (D3DVERTEXTYPE)(3) &&
        gFakeD3DLastVertices == g_zVideo_D3DSubmitTempVertices &&
        gFakeD3DLastVertexCount == 4 &&
        gFakeD3DLastDrawFlags == 0 &&
        g_zVideo_OverwriteQueueCount == 0 &&
        g_zVideo_SortedPolyQueueCount == 0 &&
        g_zVideo_D3DAppendFanCloseVertexPending == 0;

    const D3DTLVERTEX &first = g_zVideo_D3DSubmitTempVertices[0];
    const D3DTLVERTEX &close = g_zVideo_D3DSubmitTempVertices[3];
    const bool verticesOk =
        first.sx == 7.0f &&
        first.sy == 8.0f &&
        first.sz == 9.0f &&
        first.rhw == 9.0f &&
        first.color == 0xff000000 &&
        first.specular == 0xff000000 &&
        first.tu == 0.5f &&
        first.tv == 0.6f &&
        close.sx == 4.0f &&
        close.sy == 5.0f &&
        close.sz == 6.0f &&
        close.rhw == 6.0f &&
        close.color == 0xff7f7f7f &&
        close.specular == 0xff000000 &&
        close.tu == 0.3f &&
        close.tv == 0.4f;

    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_D3DRenderState_ShadeMode = 2;
    g_zVideo_D3DRenderState_TextureHandle = 0x3456;
    g_zVideo_D3DRenderState_TextureMapBlend = (D3DTEXTUREBLEND)(2);
    g_zVideo_D3DRenderState_TextureAddressU = (D3DTEXTUREADDRESS)(1);
    g_zVideo_D3DRenderState_TextureAddressV = (D3DTEXTUREADDRESS)(2);
    g_zVideo_D3DAppendFanCloseVertexPending = 0;
    zVideo_dd3d::SubmitPolygonLit(
        vertices,
        uvPairs,
        attr1,
        nullptr,
        nullptr,
        2,
        &renderClass,
        0x77,
        1.0f,
        0
    );
    const bool cacheHitOk =
        gFakeD3DSetRenderStateCalls == 0 &&
        gFakeD3DDrawPrimitiveCalls == 1 &&
        gFakeD3DLastVertexCount == 2 &&
        g_zVideo_D3DSubmitTempVertices[0].sx == 4.0f &&
        g_zVideo_D3DSubmitTempVertices[0].color == 0xff7f7f7f &&
        g_zVideo_D3DSubmitTempVertices[0].tu == 0.3f &&
        g_zVideo_D3DSubmitTempVertices[0].tv == 0.4f;

    std::memset(g_zVideo_OverwriteQueueBase, 0, sizeof(g_zVideo_OverwriteQueueBase[0]));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;
    g_zVideo_D3DAppendFanCloseVertexPending = 1;
    renderClass.textureMapBlend = (D3DTEXTUREBLEND)(4);
    float attr0[2] = {0.0f, 0.5f};
    float attr2[2] = {0.0f, 1.0f};
    g_zVideo_D3DColorNormalizeChannelIndex = 2;
    g_zVideo_D3DColorAttrBiasR = 100.0f;
    g_zVideo_D3DColorAttrBiasG = 200.0f;
    g_zVideo_D3DColorAttrBiasB = 300.0f;
    zVideo_dd3d::SubmitPolygonLit(
        vertices,
        uvPairs,
        attr1,
        attr0,
        attr2,
        2,
        &renderClass,
        0x88,
        0.5f,
        1
    );

    const D3DTLVERTEX &transparentFirst = g_zVideo_OverwriteQueueBase[0].vertices[0];
    const D3DTLVERTEX &transparentClose = g_zVideo_OverwriteQueueBase[0].vertices[2];
    const bool transparentQueueOk =
        g_zVideo_OverwriteQueueCount == 1 &&
        g_zVideo_OverwriteQueueBase[0].type == 0 &&
        g_zVideo_OverwriteQueueBase[0].vertexCount == 3 &&
        g_zVideo_OverwriteQueueBase[0].renderClass ==
            reinterpret_cast<std::int32_t>(&renderClass) &&
        g_zVideo_OverwriteQueueBase[0].renderParam == 0x88 &&
        g_zVideo_SortedPolyQueueCount == 0 &&
        g_zVideo_D3DAppendFanCloseVertexPending == 0;
    const bool transparentFirstOk =
        transparentFirst.sx == 4.0f &&
        transparentFirst.color == 0x7fa3d1ff &&
        transparentFirst.specular == 0x00000000 &&
        transparentFirst.tu == 0.3f &&
        transparentFirst.tv == 0.4f;
    const bool transparentCloseOk =
        transparentClose.sx == 1.0f &&
        transparentClose.color == 0x7fffffff &&
        transparentClose.specular == 0xff000000 &&
        transparentClose.tu == 0.1f &&
        transparentClose.tv == 0.2f;

    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_D3DRenderState_ShadeMode = savedShadeMode;
    g_zVideo_D3DRenderState_TextureHandle = savedTextureHandle;
    g_zVideo_D3DRenderState_TextureMapBlend = savedTextureMapBlend;
    g_zVideo_D3DRenderState_TextureAddressU = savedTextureAddressU;
    g_zVideo_D3DRenderState_TextureAddressV = savedTextureAddressV;
    g_zVideo_D3DAppendFanCloseVertexPending = savedAppendFanClose;
    g_zVideo_D3DColorNormalizeChannelIndex = savedNormalizeChannel;
    g_zVideo_D3DColorAttrBiasR = savedBiasR;
    g_zVideo_D3DColorAttrBiasG = savedBiasG;
    g_zVideo_D3DColorAttrBiasB = savedBiasB;
    if (!renderStateOk) {
        return 1;
    }
    if (!drawOk) {
        return 2;
    }
    if (!verticesOk) {
        return 3;
    }
    if (!cacheHitOk) {
        return 4;
    }
    if (!transparentQueueOk) {
        return 5;
    }
    if (!transparentFirstOk) {
        return 6;
    }
    return transparentCloseOk ? 0 : 7;
}

extern "C" int zvideo_draw_point_color16_smoke(void) {
    FakeD3DDevice2Object fakeDevice = {};
    IDirect3DDevice2 *const savedDevice = g_zVideo_pD3DDevice;
    const D3DTEXTUREHANDLE savedTextureHandle = g_zVideo_D3DRenderState_TextureHandle;
    const int savedShadeMode = g_zVideo_D3DRenderState_ShadeMode;
    const int savedRShift = g_zVideo_PixelPack.packedBase;
    const int savedGShift = g_zVideo_PixelPack.sumMinus8;
    const int savedBShiftTo8 = g_zVideo_PixelPack.bShiftTo8;
    const int savedRMaskShifted = g_zVideo_PixelPack.rMaskShifted;
    const int savedGMaskShifted = g_zVideo_PixelPack.gMaskShifted;
    const int savedBMaskShifted = g_zVideo_PixelPack.bMaskShifted;
    const int savedRBits = g_zVideo_PixelPack.rBits;
    const int savedGBits = g_zVideo_PixelPack.gBits;
    const int savedBBits = g_zVideo_PixelPack.bBits;
    const unsigned int savedRMask = g_zVideo_PixelPack.rMask;
    const unsigned int savedGMask = g_zVideo_PixelPack.gMask;
    const unsigned int savedBMask = g_zVideo_PixelPack.bMask;

    zVideo::PixelPack_SetupFromMasks(
        5,
        6,
        5,
        0xf800,
        0x07e0,
        0x001f
    );
    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_D3DRenderState_TextureHandle = 0x2468;
    g_zVideo_D3DRenderState_ShadeMode = 2;

    zVideo_XyzVertex point = {1.0f, 2.0f, 3.0f};
    zVideo_dd3d::DrawPointColor16(
        &point,
        0x07e0,
        99
    );

    const bool renderStateOk =
        gFakeD3DSetRenderStateCalls == 2 &&
        gFakeD3DRenderStates[0] == D3DRENDERSTATE_TEXTUREHANDLE &&
        gFakeD3DRenderStateValues[0] == 0 &&
        gFakeD3DRenderStates[1] == D3DRENDERSTATE_SHADEMODE &&
        gFakeD3DRenderStateValues[1] == 1 &&
        g_zVideo_D3DRenderState_TextureHandle == 0 &&
        g_zVideo_D3DRenderState_ShadeMode == 1;

    const bool drawOk =
        gFakeD3DDrawPrimitiveCalls == 1 &&
        gFakeD3DLastPrimitiveType == (D3DPRIMITIVETYPE)(1) &&
        gFakeD3DLastVertexType == (D3DVERTEXTYPE)(3) &&
        gFakeD3DLastVertices == g_zVideo_D3DSubmitTempVertices &&
        gFakeD3DLastVertexCount == 1 &&
        gFakeD3DLastDrawFlags == 0;

    const D3DTLVERTEX &vertex = g_zVideo_D3DSubmitTempVertices[0];
    const bool vertexOk =
        vertex.sx == 1.0f &&
        vertex.sy == 2.0f &&
        vertex.sz == 3.0f &&
        vertex.rhw == 3.0f &&
        vertex.color == 0xff00fc00 &&
        vertex.specular == 0xff000000;

    InstallFakeD3DDevice2(fakeDevice);
    g_zVideo_D3DRenderState_TextureHandle = 0;
    g_zVideo_D3DRenderState_ShadeMode = 1;
    point.x = 4.0f;
    point.y = 5.0f;
    point.z = 6.0f;
    zVideo_dd3d::DrawPointColor16(
        &point,
        0xf800,
        3
    );
    const bool cacheHitOk =
        gFakeD3DSetRenderStateCalls == 0 &&
        gFakeD3DDrawPrimitiveCalls == 1 &&
        gFakeD3DLastVertexCount == 1 &&
        g_zVideo_D3DSubmitTempVertices[0].sx == 4.0f &&
        g_zVideo_D3DSubmitTempVertices[0].rhw == 6.0f &&
        g_zVideo_D3DSubmitTempVertices[0].color == 0xfff80000;

    g_zVideo_pD3DDevice = savedDevice;
    g_zVideo_D3DRenderState_TextureHandle = savedTextureHandle;
    g_zVideo_D3DRenderState_ShadeMode = savedShadeMode;
    g_zVideo_PixelPack.packedBase = savedRShift;
    g_zVideo_PixelPack.sumMinus8 = savedGShift;
    g_zVideo_PixelPack.bShiftTo8 = savedBShiftTo8;
    g_zVideo_PixelPack.rMaskShifted = savedRMaskShifted;
    g_zVideo_PixelPack.gMaskShifted = savedGMaskShifted;
    g_zVideo_PixelPack.bMaskShifted = savedBMaskShifted;
    g_zVideo_PixelPack.rBits = savedRBits;
    g_zVideo_PixelPack.gBits = savedGBits;
    g_zVideo_PixelPack.bBits = savedBBits;
    g_zVideo_PixelPack.rMask = savedRMask;
    g_zVideo_PixelPack.gMask = savedGMask;
    g_zVideo_PixelPack.bMask = savedBMask;
    return renderStateOk && drawOk && vertexOk && cacheHitOk ? 0 : 1;
}

extern "C" int zvideo_present_display_mode_surface_null_smoke(void) {
    g_zVideo_DisplayModeSurfaceState.surf = nullptr;
    zVidRect32 rect{};
    return zVideo_dd3d::PresentDisplayModeSurface(&rect, &rect, 0, 0) == 0x400 ? 0 : 1;
}

extern "C" int zvideo_dd3d_present_display_mode_surface_smoke(void) {
    const zVideo_SurfaceStatePartial savedDisplayState =
        g_zVideo_DisplayModeSurfaceState;
    const zVideo_SurfaceStatePartial savedPrimaryState = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial savedSwState = g_zVideo_SwSurfaceState;

    FakeDirectDrawSurface3Object displaySurface{};
    FakeDirectDrawSurface3Object primarySurface{};
    FakeDirectDrawSurface3Object swSurface{};
    zVidRect32 srcRect = {1, 2, 11, 12};
    zVidRect32 dstRect = {3, 4, 13, 14};

    InstallFakeDirectDrawSurface3(
        displaySurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf =
        reinterpret_cast<IDirectDrawSurface3 *>(&displaySurface);
    const bool simpleFlipOk =
        zVideo_dd3d::PresentDisplayModeSurface(&srcRect, &dstRect, 0, 0) == 0 &&
        gFakeDirectDrawSurface3FlipCalls == 1 &&
        gFakeDirectDrawSurface3FlipSurfaces[0] ==
            reinterpret_cast<IDirectDrawSurface3 *>(&displaySurface) &&
        gFakeDirectDrawSurface3LastFlipTarget == nullptr &&
        gFakeDirectDrawSurface3LastFlipFlags == 0 &&
        gFakeDirectDrawSurface3BltCalls == 0 &&
        gFakeDirectDrawSurface3RestoreCalls == 0;

    InstallFakeDirectDrawSurface3(
        displaySurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    primarySurface.vtable = gFakeDirectDrawSurface3VTable;
    swSurface.vtable = gFakeDirectDrawSurface3VTable;
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf =
        reinterpret_cast<IDirectDrawSurface3 *>(&displaySurface);
    g_zVideo_PrimarySurfaceState.surf =
        reinterpret_cast<IDirectDrawSurface3 *>(&primarySurface);
    g_zVideo_SwSurfaceState.surf =
        reinterpret_cast<IDirectDrawSurface3 *>(&swSurface);
    const bool blitAndWaitOk =
        zVideo_dd3d::PresentDisplayModeSurface(&srcRect, &dstRect, 1, 1) == 0 &&
        gFakeDirectDrawSurface3BltCalls == 1 &&
        gFakeDirectDrawSurface3BltSurfaces[0] ==
            reinterpret_cast<IDirectDrawSurface3 *>(&swSurface) &&
        gFakeDirectDrawSurface3LastBltDstRectArg == (RECT *)(&dstRect) &&
        gFakeDirectDrawSurface3LastBltSource ==
            reinterpret_cast<IDirectDrawSurface3 *>(&primarySurface) &&
        gFakeDirectDrawSurface3LastBltSrcRectArg == (RECT *)(&srcRect) &&
        gFakeDirectDrawSurface3LastBltFlags == DDBLT_WAIT &&
        gFakeDirectDrawSurface3LastBltFx == nullptr &&
        gFakeDirectDrawSurface3FlipCalls == 1 &&
        gFakeDirectDrawSurface3LastFlipTarget == nullptr &&
        gFakeDirectDrawSurface3LastFlipFlags == DDFLIP_WAIT;

    InstallFakeDirectDrawSurface3(
        displaySurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    ConfigureFakeDirectDrawSurface3FlipResults(
        DDERR_WASSTILLDRAWING,
        DD_OK
    );
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf =
        reinterpret_cast<IDirectDrawSurface3 *>(&displaySurface);
    const bool stillDrawingRetryOk =
        zVideo_dd3d::PresentDisplayModeSurface(&srcRect, &dstRect, 0, 0) == 0 &&
        gFakeDirectDrawSurface3FlipCalls == 2 &&
        gFakeDirectDrawSurface3RestoreCalls == 0;

    InstallFakeDirectDrawSurface3(
        displaySurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    ConfigureFakeDirectDrawSurface3FlipResults(
        DDERR_SURFACELOST,
        DD_OK
    );
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf =
        reinterpret_cast<IDirectDrawSurface3 *>(&displaySurface);
    const bool surfaceLostRetryOk =
        zVideo_dd3d::PresentDisplayModeSurface(&srcRect, &dstRect, 0, 0) == 0 &&
        gFakeDirectDrawSurface3FlipCalls == 2 &&
        gFakeDirectDrawSurface3RestoreCalls == 1 &&
        gFakeDirectDrawSurface3RestoreSurfaces[0] ==
            reinterpret_cast<IDirectDrawSurface3 *>(&displaySurface);

    InstallFakeDirectDrawSurface3(
        displaySurface,
        DD_OK,
        DD_OK,
        DDERR_GENERIC
    );
    ConfigureFakeDirectDrawSurface3FlipResults(
        DDERR_SURFACELOST,
        DD_OK
    );
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf =
        reinterpret_cast<IDirectDrawSurface3 *>(&displaySurface);
    const bool restoreFailureOk =
        zVideo_dd3d::PresentDisplayModeSurface(&srcRect, &dstRect, 0, 0) ==
            0x5a56ffff &&
        gFakeDirectDrawSurface3FlipCalls == 1 &&
        gFakeDirectDrawSurface3RestoreCalls == 1;

    InstallFakeDirectDrawSurface3(
        displaySurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    ConfigureFakeDirectDrawSurface3FlipResults(
        DDERR_GENERIC,
        DD_OK
    );
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf =
        reinterpret_cast<IDirectDrawSurface3 *>(&displaySurface);
    const bool flipFailureOk =
        zVideo_dd3d::PresentDisplayModeSurface(&srcRect, &dstRect, 0, 0) ==
            0x5a56ffff &&
        gFakeDirectDrawSurface3FlipCalls == 1 &&
        gFakeDirectDrawSurface3RestoreCalls == 0;

    g_zVideo_DisplayModeSurfaceState = savedDisplayState;
    g_zVideo_PrimarySurfaceState = savedPrimaryState;
    g_zVideo_SwSurfaceState = savedSwState;
    return simpleFlipOk && blitAndWaitOk && stillDrawingRetryOk &&
                   surfaceLostRetryOk && restoreFailureOk && flipFailureOk
               ? 0
               : 1;
}

extern "C" int zvideo_texture_record_release_upload_null_smoke(void) {
    zVideo_TextureRecordPartial textureRecord{};
    zVideo_dd3d::TextureRecord_ReleaseUploadSurfaceRef(&textureRecord);
    return textureRecord.m_uploadSurface == nullptr ? 0 : 1;
}

extern "C" int zvideo_texture_record_release_upload_surface_smoke(void) {
    zVideo_TextureRecordPartial textureRecord{};
    ResetFakeComReleaseTracking();
    zVideo_dd3d::TextureRecord_ReleaseUploadSurfaceRef(&textureRecord);
    const bool nullOk =
        textureRecord.m_uploadSurface == nullptr &&
        gFakeComReleaseCalls == 0;

    FakeComObject uploadSurface{};
    InstallFakeComObject(uploadSurface);
    ResetFakeComReleaseTracking();
    textureRecord.m_uploadSurface =
        reinterpret_cast<IDirectDrawSurface *>(&uploadSurface);
    zVideo_dd3d::TextureRecord_ReleaseUploadSurfaceRef(&textureRecord);
    const bool releaseOk =
        textureRecord.m_uploadSurface == nullptr &&
        gFakeComReleaseCalls == 1 &&
        gFakeComReleaseObjects[0] == &uploadSurface;

    return nullOk && releaseOk ? 0 : 1;
}

extern "C" int zvideo_texture_record_finalize_upload_smoke(void) {
    CodeFunctionPatch uploadPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo_dd3d::UploadImageToSurface),
            reinterpret_cast<void *>(&FakeUploadImageToSurface),
            uploadPatch
        )) {
        return 1;
    }

    FakeDirectDraw2Object directDraw{};
    FakeDirectDrawSurfaceObject uploadSurface{};
    FakeDirectDrawSurface3Object surface3{};
    FakeD3DTexture2Object uploadTexture{};
    FakeD3DTexture2Object targetTexture{};
    InstallFakeDirectDraw2(
        directDraw,
        uploadSurface,
        surface3
    );
    InstallFakeD3DTexture2(
        uploadTexture,
        targetTexture
    );

    zVideo_TextureRecordPartial textureRecord{};
    zVidImagePartial image{};
    image.formatFlagsPacked = 3;
    gFakeUploadImageToSurfaceCalls = 0;
    gFakeUploadImageToSurfaceSurface = nullptr;
    zVideo_dd3d::TextureRecord_FinalizeUpload(
        &textureRecord,
        &image
    );
    const bool nullUploadOk =
        gFakeUploadImageToSurfaceCalls == 0 &&
        gFakeDirectDrawSurfaceQueryInterfaceCalls == 0 &&
        gFakeD3DTexture2LoadCalls == 0 &&
        gFakeD3DTexture2ReleaseCalls == 0;

    textureRecord.m_uploadSurface =
        reinterpret_cast<IDirectDrawSurface *>(&uploadSurface);
    textureRecord.m_texture = reinterpret_cast<IDirect3DTexture2 *>(&targetTexture);
    gFakeDirectDrawSurfaceQueryInterfaceValue = &uploadTexture;
    gFakeUploadImageToSurfaceCalls = 0;
    gFakeUploadImageToSurfaceSurface = nullptr;
    gFakeUploadImageToSurfaceImage = nullptr;
    gFakeUploadImageToSurfaceUseAlpha = -1;
    gFakeUploadImageToSurfaceResult = 1;
    zVideo_dd3d::TextureRecord_FinalizeUpload(
        &textureRecord,
        &image
    );
    const bool successOk =
        gFakeUploadImageToSurfaceCalls == 1 &&
        gFakeUploadImageToSurfaceSurface ==
            reinterpret_cast<IDirectDrawSurface *>(&uploadSurface) &&
        gFakeUploadImageToSurfaceImage == &image &&
        gFakeUploadImageToSurfaceUseAlpha == 2 &&
        gFakeDirectDrawSurfaceQueryInterfaceCalls == 1 &&
        gFakeDirectDrawSurfaceLastQueryInterfaceIid != nullptr &&
        IsEqualGUID(*gFakeDirectDrawSurfaceLastQueryInterfaceIid, IID_IDirect3DTexture2) &&
        gFakeD3DTexture2LoadCalls == 1 &&
        gFakeD3DTexture2LastLoadSelf ==
            reinterpret_cast<IDirect3DTexture2 *>(&targetTexture) &&
        gFakeD3DTexture2LastLoadSource ==
            reinterpret_cast<IDirect3DTexture2 *>(&uploadTexture) &&
        gFakeD3DTexture2ReleaseCalls == 1 &&
        gFakeD3DTexture2ReleaseObjects[0] ==
            reinterpret_cast<IDirect3DTexture2 *>(&uploadTexture);

    InstallFakeDirectDraw2(
        directDraw,
        uploadSurface,
        surface3
    );
    InstallFakeD3DTexture2(
        uploadTexture,
        targetTexture
    );
    textureRecord.m_uploadSurface =
        reinterpret_cast<IDirectDrawSurface *>(&uploadSurface);
    textureRecord.m_texture = reinterpret_cast<IDirect3DTexture2 *>(&targetTexture);
    gFakeDirectDrawSurfaceQueryInterfaceValue = &uploadTexture;
    gFakeD3DTexture2LoadResult = DDERR_GENERIC;
    gFakeUploadImageToSurfaceCalls = 0;
    zVideo_dd3d::TextureRecord_FinalizeUpload(
        &textureRecord,
        nullptr
    );
    const bool loadFailureOk =
        gFakeUploadImageToSurfaceCalls == 0 &&
        gFakeDirectDrawSurfaceQueryInterfaceCalls == 1 &&
        gFakeD3DTexture2LoadCalls == 1 &&
        gFakeD3DTexture2ReleaseCalls == 0;

    RestoreFunctionPatch(uploadPatch);
    return nullUploadOk && successOk && loadFailureOk ? 0 : 2;
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

extern "C" int zvideo_texture_record_lock_upload_surface_smoke(void) {
    FakeDirectDrawSurface3Object uploadSurface{};
    zVideo_TextureRecordPartial textureRecord{};
    textureRecord.m_uploadSurface =
        reinterpret_cast<IDirectDrawSurface *>(&uploadSurface);

    unsigned char pixels[16] = {};
    void *outPixels = reinterpret_cast<void *>(0x12345678);
    int outPitchBytes = -1;
    InstallFakeDirectDrawSurface3(
        uploadSurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    gFakeDirectDrawSurface3LockPixels = pixels;
    gFakeDirectDrawSurface3LockPitch = 32;
    const bool successOk =
        zVideo_dd3d::TextureRecord_LockUploadSurface(
            &textureRecord,
            &outPixels,
            &outPitchBytes
        ) == 1 &&
        gFakeDirectDrawSurface3LockCalls == 1 &&
        gFakeDirectDrawSurface3LockSurfaces[0] ==
            reinterpret_cast<IDirectDrawSurface3 *>(&uploadSurface) &&
        gFakeDirectDrawSurface3LockDescSize == sizeof(DDSURFACEDESC) &&
        outPixels == pixels &&
        outPitchBytes == 32;

    outPixels = reinterpret_cast<void *>(0x12345678);
    outPitchBytes = -1;
    InstallFakeDirectDrawSurface3(
        uploadSurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    gFakeDirectDrawSurface3LockResults[0] = DDERR_GENERIC;
    const int failureResult = zVideo_dd3d::TextureRecord_LockUploadSurface(
        &textureRecord,
        &outPixels,
        &outPitchBytes
    );
    const bool failureOk =
        failureResult == 0 &&
        gFakeDirectDrawSurface3LockCalls == 1 &&
        outPixels == reinterpret_cast<void *>(0x12345678) &&
        outPitchBytes == -1;

    if (!successOk) {
        return 2;
    }
    if (!failureOk) {
        return 3;
    }
    return 0;
}

extern "C" int zvideo_texture_record_unlock_upload_surface_smoke(void) {
    FakeDirectDrawSurface3Object uploadSurface{};
    zVideo_TextureRecordPartial textureRecord{};
    textureRecord.m_uploadSurface =
        reinterpret_cast<IDirectDrawSurface *>(&uploadSurface);

    InstallFakeDirectDrawSurface3(
        uploadSurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    const bool successOk =
        zVideo_dd3d::TextureRecord_UnlockUploadSurface(&textureRecord) == 1 &&
        gFakeDirectDrawSurface3UnlockCalls == 1 &&
        gFakeDirectDrawSurface3UnlockSurfaces[0] ==
            reinterpret_cast<IDirectDrawSurface3 *>(&uploadSurface) &&
        gFakeDirectDrawSurface3LastUnlockArg == nullptr;

    InstallFakeDirectDrawSurface3(
        uploadSurface,
        DDERR_GENERIC,
        DD_OK,
        DD_OK
    );
    const bool failureOk =
        zVideo_dd3d::TextureRecord_UnlockUploadSurface(&textureRecord) == 0 &&
        gFakeDirectDrawSurface3UnlockCalls == 1 &&
        gFakeDirectDrawSurface3RestoreCalls == 0 &&
        gFakeDirectDrawSurface3LastUnlockArg == nullptr;

    return successOk && failureOk ? 0 : 1;
}

extern "C" int zvideo_texture_record_destroy_smoke(void) {
    zVideo_TextureRecordPartial *const savedDefaultRecord = g_zImage_DefaultTextureRecord;
    zVideo_TextureRecordPartial defaultRecord{};
    g_zImage_DefaultTextureRecord = &defaultRecord;
    ResetFakeComReleaseTracking();
    zVideo_dd3d::TextureRecord_Destroy(&defaultRecord);
    const bool defaultSkipOk = gFakeComReleaseCalls == 0;

    FakeComObject uploadSurface{};
    FakeComObject textureSurface{};
    FakeComObject texture{};
    InstallFakeComObject(uploadSurface);
    InstallFakeComObject(textureSurface);
    InstallFakeComObject(texture);
    ResetFakeComReleaseTracking();

    zVideo_TextureRecordPartial *textureRecord =
        (zVideo_TextureRecordPartial *)std::malloc(sizeof(zVideo_TextureRecordPartial));
    if (textureRecord == nullptr) {
        g_zImage_DefaultTextureRecord = savedDefaultRecord;
        return 1;
    }
    std::memset(textureRecord, 0, sizeof(*textureRecord));
    textureRecord->m_uploadSurface = reinterpret_cast<IDirectDrawSurface *>(&uploadSurface);
    textureRecord->m_textureSurface =
        reinterpret_cast<IDirectDrawSurface *>(&textureSurface);
    textureRecord->m_texture = reinterpret_cast<IDirect3DTexture2 *>(&texture);

    zVideo_dd3d::TextureRecord_Destroy(textureRecord);
    const bool releaseOk =
        gFakeComReleaseCalls == 3 &&
        gFakeComReleaseObjects[0] == &uploadSurface &&
        gFakeComReleaseObjects[1] == &textureSurface &&
        gFakeComReleaseObjects[2] == &texture;

    g_zImage_DefaultTextureRecord = savedDefaultRecord;
    return defaultSkipOk && releaseOk ? 0 : 2;
}

extern "C" int zvideo_dd_shutdown_video_system_smoke(void) {
    CodeFunctionPatch teardownPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo_dd::TeardownVideoSubsystem),
            reinterpret_cast<void *>(&FakeTeardownVideoSubsystem),
            teardownPatch
        )) {
        return 1;
    }

    zVideo_TextureRecordPartial *const savedDefaultRecord = g_zImage_DefaultTextureRecord;
    zVideo_TextureRecordPartial defaultRecord{};

    gFakeTeardownVideoSubsystemCalls = 0;
    ResetFakeComReleaseTracking();
    g_zImage_DefaultTextureRecord = &defaultRecord;
    const int defaultResult = zVideo_dd::ShutdownVideoSystem();
    const bool defaultOk =
        defaultResult == 0 &&
        g_zImage_DefaultTextureRecord == nullptr &&
        gFakeTeardownVideoSubsystemCalls == 1 &&
        gFakeComReleaseCalls == 0;

    gFakeTeardownVideoSubsystemCalls = 0;
    g_zImage_DefaultTextureRecord = nullptr;
    const int noDefaultResult = zVideo_dd::ShutdownVideoSystem();
    const bool noDefaultOk =
        noDefaultResult == 0 &&
        g_zImage_DefaultTextureRecord == nullptr &&
        gFakeTeardownVideoSubsystemCalls == 1;

    g_zImage_DefaultTextureRecord = savedDefaultRecord;
    RestoreFunctionPatch(teardownPatch);
    return defaultOk && noDefaultOk ? 0 : 2;
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

    if (dstPixels[0] != 0xff00 || dstPixels[1] != 0x80f0 || dstPixels[4] != 0x100f ||
        dstPixels[5] != 0) {
        return 2;
    }

    zVideo::PixelPack_SetupFromMasks(5, 5, 5, 0x7c00, 0x03e0, 0x001f);
    std::uint16_t src555 = 0x4210;
    std::uint8_t alpha555 = 0xa0;
    std::memset(dstPixels, 0, sizeof(dstPixels));
    image.width = 1;
    image.height = 1;
    image.pixels = &src555;
    image.alphaMap = reinterpret_cast<char *>(&alpha555);
    zVideo_dd3d::ConvertImagePixelsForTexture(dstPixels, &image, 8, 0);

    return dstPixels[0] == 0xa888 ? 0 : 3;
}

extern "C" int zvideo_dd3d_upload_image_to_surface_smoke(void) {
    const int savedDisplayModeBpp = g_zVideo_DisplayModeBpp;
    const int savedRShift = g_zVideo_PixelPack.packedBase;
    const int savedGShift = g_zVideo_PixelPack.sumMinus8;
    const int savedBShiftTo8 = g_zVideo_PixelPack.bShiftTo8;
    const int savedRMaskShifted = g_zVideo_PixelPack.rMaskShifted;
    const int savedGMaskShifted = g_zVideo_PixelPack.gMaskShifted;
    const int savedBMaskShifted = g_zVideo_PixelPack.bMaskShifted;
    const int savedRBits = g_zVideo_PixelPack.rBits;
    const int savedGBits = g_zVideo_PixelPack.gBits;
    const int savedBBits = g_zVideo_PixelPack.bBits;
    const unsigned int savedRMask = g_zVideo_PixelPack.rMask;
    const unsigned int savedGMask = g_zVideo_PixelPack.gMask;
    const unsigned int savedBMask = g_zVideo_PixelPack.bMask;

    FakeDirectDrawSurface3Object uploadSurface{};
    zVidImagePartial image{};
    std::uint8_t srcContiguous[16];
    std::uint8_t dstContiguous[24];
    for (int i = 0; i < 16; ++i) {
        srcContiguous[i] = (std::uint8_t)(0x20 + i);
    }
    std::memset(dstContiguous, 0xcc, sizeof(dstContiguous));

    InstallFakeDirectDrawSurface3(
        uploadSurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    g_zVideo_DisplayModeBpp = 16;
    gFakeDirectDrawSurface3LockPixels = dstContiguous;
    gFakeDirectDrawSurface3LockPitch = 4;
    image.width = 4;
    image.height = 2;
    image.pixels = srcContiguous;
    image.alphaMap = 0;
    const bool contiguousOk =
        zVideo_dd3d::UploadImageToSurface(
            reinterpret_cast<IDirectDrawSurface *>(&uploadSurface),
            &image,
            0
        ) == 1 &&
        gFakeDirectDrawSurface3LockCalls == 1 &&
        gFakeDirectDrawSurface3UnlockCalls == 1 &&
        gFakeDirectDrawSurface3UnlockSurfaces[0] ==
            reinterpret_cast<IDirectDrawSurface3 *>(&uploadSurface) &&
        std::memcmp(dstContiguous, srcContiguous, sizeof(srcContiguous)) == 0 &&
        dstContiguous[16] == 0xcc;

    std::uint8_t srcRows[8];
    std::uint8_t dstRows[20];
    for (int i = 0; i < 8; ++i) {
        srcRows[i] = (std::uint8_t)(0x40 + i);
    }
    std::memset(dstRows, 0xcc, sizeof(dstRows));

    InstallFakeDirectDrawSurface3(
        uploadSurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    g_zVideo_DisplayModeBpp = 16;
    gFakeDirectDrawSurface3LockPixels = dstRows;
    gFakeDirectDrawSurface3LockPitch = 8;
    image.width = 2;
    image.height = 2;
    image.pixels = srcRows;
    image.alphaMap = 0;
    const bool rowCopyOk =
        zVideo_dd3d::UploadImageToSurface(
            reinterpret_cast<IDirectDrawSurface *>(&uploadSurface),
            &image,
            0
        ) == 1 &&
        std::memcmp(dstRows, srcRows, 4) == 0 &&
        dstRows[4] == 0xcc &&
        std::memcmp(dstRows + 8, srcRows + 4, 4) == 0 &&
        dstRows[12] == 0xcc &&
        gFakeDirectDrawSurface3LockCalls == 1 &&
        gFakeDirectDrawSurface3UnlockCalls == 1;

    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);
    std::uint16_t srcAlpha[4] = {0xf800, 0x07e0, 0x001f, 0};
    std::uint8_t alphaMap[4] = {0xf0, 0x80, 0x10, 0};
    std::uint16_t dstAlpha[8] = {};

    InstallFakeDirectDrawSurface3(
        uploadSurface,
        DD_OK,
        DD_OK,
        DD_OK
    );
    gFakeDirectDrawSurface3LockPixels = dstAlpha;
    gFakeDirectDrawSurface3LockPitch = 8;
    image.width = 2;
    image.height = 2;
    image.pixels = srcAlpha;
    image.alphaMap = reinterpret_cast<char *>(alphaMap);
    const bool alphaConvertOk =
        zVideo_dd3d::UploadImageToSurface(
            reinterpret_cast<IDirectDrawSurface *>(&uploadSurface),
            &image,
            1
        ) == 1 &&
        dstAlpha[0] == 0xff00 &&
        dstAlpha[1] == 0x80f0 &&
        dstAlpha[4] == 0x100f &&
        dstAlpha[5] == 0 &&
        gFakeDirectDrawSurface3LockCalls == 1 &&
        gFakeDirectDrawSurface3UnlockCalls == 1;

    g_zVideo_DisplayModeBpp = savedDisplayModeBpp;
    g_zVideo_PixelPack.packedBase = savedRShift;
    g_zVideo_PixelPack.sumMinus8 = savedGShift;
    g_zVideo_PixelPack.bShiftTo8 = savedBShiftTo8;
    g_zVideo_PixelPack.rMaskShifted = savedRMaskShifted;
    g_zVideo_PixelPack.gMaskShifted = savedGMaskShifted;
    g_zVideo_PixelPack.bMaskShifted = savedBMaskShifted;
    g_zVideo_PixelPack.rBits = savedRBits;
    g_zVideo_PixelPack.gBits = savedGBits;
    g_zVideo_PixelPack.bBits = savedBBits;
    g_zVideo_PixelPack.rMask = savedRMask;
    g_zVideo_PixelPack.gMask = savedGMask;
    g_zVideo_PixelPack.bMask = savedBMask;
    return contiguousOk && rowCopyOk && alphaConvertOk ? 0 : 1;
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
    D3DDEVICEDESC *selectedDesc = &selectedD3DDevice.m_hwDesc;
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

extern "C" int zvideo_dd3d_create_texture_record_smoke(void) {
    CodeFunctionPatch uploadPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo_dd3d::UploadImageToSurface),
            reinterpret_cast<void *>(&FakeUploadImageToSurface),
            uploadPatch
        )) {
        return 1;
    }

    zVidD3DDriverRecordPartial *const savedSelectedD3D =
        g_zVideo_pSelectedD3DDeviceInfo;
    zVideo_TextureRecordPartial *const savedDefaultRecord =
        g_zImage_DefaultTextureRecord;
    IDirectDraw2 *const savedDirectDraw2 = g_zVideo_pDirectDraw2;
    IDirect3DDevice2 *const savedD3DDevice = g_zVideo_pD3DDevice;
    const D3DDEVICEDESC savedHalDesc = g_zVideo_D3DHalDeviceDesc;
    const int savedTextureRBits = g_zVideo_TexturePixelPack_RBits;
    const int savedTextureGBits = g_zVideo_TexturePixelPack_GBits;
    const int savedTextureBBits = g_zVideo_TexturePixelPack_BBits;
    const int savedTextureABits = g_zVideo_TexturePixelPack_ABits;
    const unsigned int savedTextureRMask = g_zVideo_TexturePixelPack_RMask;
    const unsigned int savedTextureGMask = g_zVideo_TexturePixelPack_GMask;
    const unsigned int savedTextureBMask = g_zVideo_TexturePixelPack_BMask;
    const unsigned int savedTextureAMask = g_zVideo_TexturePixelPack_AMask;
    const int savedTextureRgbBitsTotal = g_zVideo_TexturePixelPack_RGBBitsTotal;
    const int savedTextureRgbBitsTotalMinus8 =
        g_zVideo_TexturePixelPack_RGBBitsTotalMinus8;
    const int savedTextureGbBitsTotalMinus8 =
        g_zVideo_TexturePixelPack_GBBitsTotalMinus8;
    const int savedTextureBShiftTo8 = g_zVideo_TexturePixelPack_BShiftTo8;
    const int savedTextureRMaskShifted = g_zVideo_TexturePixelPack_RMaskShifted;
    const int savedTextureGMaskShifted = g_zVideo_TexturePixelPack_GMaskShifted;
    const int savedTextureBMaskShifted = g_zVideo_TexturePixelPack_BMaskShifted;
    const int savedTextureNonRgbMaskShifted =
        g_zVideo_TexturePixelPack_NonRgbMaskShifted;

    zVidD3DDriverRecordPartial selectedD3DDevice{};
    D3DDEVICEDESC *selectedDesc = &selectedD3DDevice.m_hwDesc;
    selectedDesc->dwMaxTextureWidth = 64;
    selectedDesc->dwMaxTextureHeight = 64;
    g_zVideo_pSelectedD3DDeviceInfo = &selectedD3DDevice;

    zVideo_TextureRecordPartial defaultRecord{};
    g_zImage_DefaultTextureRecord = &defaultRecord;

    FakeDirectDraw2Object directDraw{};
    FakeDirectDrawSurfaceObject createdSurface{};
    FakeDirectDrawSurface3Object surface3{};
    FakeDirectDrawPaletteObject palette{};
    FakeD3DDevice2Object d3dDevice{};
    FakeD3DTexture2Object uploadTexture{};
    FakeD3DTexture2Object texture{};
    InstallFakeDirectDraw2(
        directDraw,
        createdSurface,
        surface3
    );
    InstallFakeDirectDrawPalette(palette);
    InstallFakeD3DDevice2(d3dDevice);
    InstallFakeD3DTexture2(
        uploadTexture,
        texture
    );
    g_zVideo_pDirectDraw2 = reinterpret_cast<IDirectDraw2 *>(&directDraw);
    gFakeDirectDraw2CreatedPalette =
        reinterpret_cast<IDirectDrawPalette *>(&palette);
    gFakeDirectDrawSurfaceQueryInterfaceValues[0] = &uploadTexture;
    gFakeDirectDrawSurfaceQueryInterfaceValues[1] = &texture;
    gFakeDirectDrawSurfaceQueryInterfaceValueCount = 2;
    gFakeD3DTexture2HandleValue = 0x3579;

    zVidImagePartial image{};
    std::uint16_t pixels[32] = {};
    std::uint8_t alphaMap[32] = {};
    PALETTEENTRY deferredPalette[2] = {};
    deferredPalette[0].peRed = 10;
    deferredPalette[1].peBlue = 20;
    image.width = 8;
    image.height = 4;
    image.pixels = pixels;
    image.alphaMap = reinterpret_cast<char *>(alphaMap);
    image.palette = nullptr;
    image.paletteMetaPacked = 2 * sizeof(PALETTEENTRY);

    gFakeDirectDraw2MutateImageOnFirstCreateSurface = &image;
    gFakeDirectDraw2MutatedPalette = deferredPalette;
    gFakeDirectDraw2MutatedPaletteMetaPacked = image.paletteMetaPacked;
    g_zVideo_D3DHalDeviceDesc = {};
    g_zVideo_D3DHalDeviceDesc.dwDevCaps = D3DDEVCAPS_TEXTURENONLOCALVIDMEM;
    gFakeUploadImageToSurfaceCalls = 0;
    gFakeUploadImageToSurfaceSurface = nullptr;
    gFakeUploadImageToSurfaceImage = nullptr;
    gFakeUploadImageToSurfaceUseAlpha = -1;
    gFakeUploadImageToSurfaceResult = 1;

    zVideo_TextureRecordPartial *result =
        zVideo_dd3d::CreateTextureRecord(
            "success",
            &image,
            1,
            1,
            0
        );
    const DDSURFACEDESC &uploadDesc = gFakeDirectDraw2CreateSurfaceDescs[0];
    const DDSURFACEDESC &textureDesc = gFakeDirectDraw2CreateSurfaceDescs[1];
    const bool successOk =
        result != nullptr &&
        result != &defaultRecord &&
        gFakeDirectDraw2CreateSurfaceCalls == 2 &&
        uploadDesc.dwSize == sizeof(DDSURFACEDESC) &&
        uploadDesc.dwFlags == (DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT) &&
        uploadDesc.dwWidth == 8 &&
        uploadDesc.dwHeight == 4 &&
        uploadDesc.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY) &&
        uploadDesc.ddpfPixelFormat.dwFlags == (DDPF_RGB | DDPF_ALPHAPIXELS) &&
        uploadDesc.ddpfPixelFormat.dwRGBBitCount == 16 &&
        uploadDesc.ddpfPixelFormat.dwRBitMask == 0x0f00 &&
        uploadDesc.ddpfPixelFormat.dwGBitMask == 0x00f0 &&
        uploadDesc.ddpfPixelFormat.dwBBitMask == 0x000f &&
        uploadDesc.ddpfPixelFormat.dwRGBAlphaBitMask == 0xf000 &&
        textureDesc.ddsCaps.dwCaps ==
            (DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_ALLOCONLOAD |
             DDSCAPS_NONLOCALVIDMEM) &&
        gFakeDirectDraw2CreatePaletteCalls == 1 &&
        gFakeDirectDraw2LastCreatePaletteFlags ==
            (DDPCAPS_8BIT | DDPCAPS_ALLOW256) &&
        gFakeDirectDraw2LastCreatePaletteEntries == deferredPalette &&
        gFakeDirectDrawSurfaceSetPaletteCalls == 2 &&
        gFakeDirectDrawSurfaceSetPalettePalettes[0] ==
            reinterpret_cast<IDirectDrawPalette *>(&palette) &&
        gFakeDirectDrawSurfaceSetPalettePalettes[1] ==
            reinterpret_cast<IDirectDrawPalette *>(&palette) &&
        gFakeUploadImageToSurfaceCalls == 1 &&
        gFakeUploadImageToSurfaceSurface ==
            reinterpret_cast<IDirectDrawSurface *>(&createdSurface) &&
        gFakeUploadImageToSurfaceImage == &image &&
        gFakeUploadImageToSurfaceUseAlpha == 1 &&
        gFakeDirectDrawSurfaceQueryInterfaceCalls == 2 &&
        gFakeD3DTexture2LoadCalls == 1 &&
        gFakeD3DTexture2LastLoadSelf ==
            reinterpret_cast<IDirect3DTexture2 *>(&texture) &&
        gFakeD3DTexture2LastLoadSource ==
            reinterpret_cast<IDirect3DTexture2 *>(&uploadTexture) &&
        gFakeD3DTexture2GetHandleCalls == 1 &&
        gFakeD3DTexture2LastGetHandleDevice == g_zVideo_pD3DDevice &&
        gFakeD3DTexture2ReleaseCalls == 1 &&
        gFakeD3DTexture2ReleaseObjects[0] ==
            reinterpret_cast<IDirect3DTexture2 *>(&uploadTexture) &&
        result->m_uploadSurface == reinterpret_cast<IDirectDrawSurface *>(&createdSurface) &&
        result->m_textureSurface == reinterpret_cast<IDirectDrawSurface *>(&createdSurface) &&
        result->m_texture == reinterpret_cast<IDirect3DTexture2 *>(&texture) &&
        result->m_textureHandle == 0x3579 &&
        result->m_alphaMode == 4 &&
        result->m_uWrapMode == D3DTADDRESS_CLAMP &&
        result->m_vWrapMode == D3DTADDRESS_WRAP &&
        g_zVideo_TexturePixelPack_RBits == 4 &&
        g_zVideo_TexturePixelPack_GBits == 4 &&
        g_zVideo_TexturePixelPack_BBits == 4 &&
        g_zVideo_TexturePixelPack_ABits == 4;
    if (result != nullptr && result != &defaultRecord) {
        std::free(result);
    }

    InstallFakeDirectDraw2(
        directDraw,
        createdSurface,
        surface3
    );
    InstallFakeD3DTexture2(
        uploadTexture,
        texture
    );
    g_zVideo_pDirectDraw2 = reinterpret_cast<IDirectDraw2 *>(&directDraw);
    gFakeDirectDrawSurfaceQueryInterfaceValues[0] = &uploadTexture;
    gFakeDirectDrawSurfaceQueryInterfaceValues[1] = &texture;
    gFakeDirectDrawSurfaceQueryInterfaceValueCount = 2;
    gFakeD3DTexture2LoadResult = DDERR_GENERIC;
    gFakeUploadImageToSurfaceCalls = 0;
    image.palette = nullptr;
    image.paletteMetaPacked = 0;

    zVideo_TextureRecordPartial *failureResult =
        zVideo_dd3d::CreateTextureRecord(
            "load-failure",
            &image,
            1,
            0,
            1
        );
    const bool failureOk =
        failureResult == &defaultRecord &&
        gFakeDirectDraw2CreateSurfaceCalls == 2 &&
        gFakeDirectDraw2CreatePaletteCalls == 0 &&
        gFakeUploadImageToSurfaceCalls == 1 &&
        gFakeD3DTexture2LoadCalls == 1 &&
        gFakeD3DTexture2ReleaseCalls == 2 &&
        gFakeD3DTexture2ReleaseObjects[0] ==
            reinterpret_cast<IDirect3DTexture2 *>(&texture) &&
        gFakeD3DTexture2ReleaseObjects[1] ==
            reinterpret_cast<IDirect3DTexture2 *>(&uploadTexture) &&
        gFakeDirectDrawSurfaceReleaseCalls == 2;

    g_zVideo_pSelectedD3DDeviceInfo = savedSelectedD3D;
    g_zImage_DefaultTextureRecord = savedDefaultRecord;
    g_zVideo_pDirectDraw2 = savedDirectDraw2;
    g_zVideo_pD3DDevice = savedD3DDevice;
    g_zVideo_D3DHalDeviceDesc = savedHalDesc;
    g_zVideo_TexturePixelPack_RBits = savedTextureRBits;
    g_zVideo_TexturePixelPack_GBits = savedTextureGBits;
    g_zVideo_TexturePixelPack_BBits = savedTextureBBits;
    g_zVideo_TexturePixelPack_ABits = savedTextureABits;
    g_zVideo_TexturePixelPack_RMask = savedTextureRMask;
    g_zVideo_TexturePixelPack_GMask = savedTextureGMask;
    g_zVideo_TexturePixelPack_BMask = savedTextureBMask;
    g_zVideo_TexturePixelPack_AMask = savedTextureAMask;
    g_zVideo_TexturePixelPack_RGBBitsTotal = savedTextureRgbBitsTotal;
    g_zVideo_TexturePixelPack_RGBBitsTotalMinus8 =
        savedTextureRgbBitsTotalMinus8;
    g_zVideo_TexturePixelPack_GBBitsTotalMinus8 =
        savedTextureGbBitsTotalMinus8;
    g_zVideo_TexturePixelPack_BShiftTo8 = savedTextureBShiftTo8;
    g_zVideo_TexturePixelPack_RMaskShifted = savedTextureRMaskShifted;
    g_zVideo_TexturePixelPack_GMaskShifted = savedTextureGMaskShifted;
    g_zVideo_TexturePixelPack_BMaskShifted = savedTextureBMaskShifted;
    g_zVideo_TexturePixelPack_NonRgbMaskShifted =
        savedTextureNonRgbMaskShifted;
    RestoreFunctionPatch(uploadPatch);
    return successOk && failureOk ? 0 : 2;
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
        zVid_Image::SetHeaderFlagsByte(image, 0x7a) == 0 &&
        zVid_Image_SetPixels(image, pixels, alpha) == 0;

    image->widthScale = 3.5f;
    image->uShiftFrom20 = 77;
    image->uMask = 88;
    image->vMaskFixed20 = 99;
    const bool sized =
        zVid_Image::SetSize(image, 16, 8) == 0 &&
        image->headerFlagsByte == 0x7a && image->formatFlagsPacked == 3 &&
        image->pixels == pixels && image->alphaMap == alpha && image->width == 16 &&
        image->height == 8 && image->pixelCount == 128 && image->pitchWords == 16 &&
        image->widthScale == 3.5f &&
        image->uShiftFrom20 == 77 && image->uMask == 88 &&
        image->vMaskFixed20 == 99;

    const bool unpackedPixelDataBytes = zVid_Image::QueryPixelDataBytes(image) == 256;
    image->paletteMetaPacked = 32;
    const bool palettedPixelDataBytes = zVid_Image::QueryPixelDataBytes(image) == 128;
    image->paletteMetaPacked = 0;

    zVid_Image::Destroy(image);
    return createdZeroed && configured && sized && unpackedPixelDataBytes && palettedPixelDataBytes
               ? 0
               : 2;
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

    zVidImagePartial directReadImage{};
    const bool directHeaderRead =
        zVid_Image::ReadHeader(nullptr, &directReadImage) == -1 &&
        zVid_Image::ReadHeader(file, nullptr) == -1 &&
        zVid_Image::ReadHeader(file, &directReadImage) == 0 &&
        directReadImage.width == 2 && directReadImage.height == 1 &&
        directReadImage.pixelCount == 2 && directReadImage.pitchWords == 2 &&
        directReadImage.formatFlagsPacked == 1 && directReadImage.headerFlagsByte == 0x12 &&
        directReadImage.textureAddressFlagsPacked == 0x3456 &&
        directReadImage.paletteMetaPacked == 0;

    std::rewind(file);
    g_zVideo_PixelPack.rBits = 5;
    zVidImagePartial *image = zVid_Image::ReadFromFile(file);
    std::fclose(file);

    if (image == nullptr) {
        return 2;
    }

    std::uint16_t *readPixels = static_cast<std::uint16_t *>(image->pixels);
    const bool ok = directHeaderRead && image->width == 2 && image->height == 1 &&
                    image->pixelCount == 2 && image->pitchWords == 2 &&
                    image->headerFlagsByte == 0x12 &&
                    image->textureAddressFlagsPacked == 0x3456 &&
                    image->paletteMetaPacked == 0 && (image->formatFlagsPacked & 0x21) == 0x21 &&
                    readPixels[0] == 0x3fff && readPixels[1] == 0x001f;

    zVid_Image::Destroy(image);
    return ok ? 0 : 3;
}

extern "C" int zvideo_image_read_data_smoke(void) {
    unsigned char sourcePixels[4] = {0x10, 0x20, 0x30, 0x40};
    unsigned char sourceAlpha[4] = {1, 2, 3, 4};
    unsigned char sourcePalette[2] = {0xa0, 0xb0};

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        return 1;
    }

    std::fwrite(sourcePixels, 1, sizeof(sourcePixels), file);
    std::fwrite(sourceAlpha, 1, sizeof(sourceAlpha), file);
    std::fwrite(sourcePalette, 1, sizeof(sourcePalette), file);
    std::rewind(file);

    unsigned char pixels[4] = {};
    zVidImagePartial image{};
    image.pixelCount = 4;
    image.width = 2;
    image.height = 2;
    image.formatFlagsPacked = 0x08;
    image.paletteMetaPacked = 2;
    image.pixels = pixels;

    const int readResult = zVid_Image::ReadData(file, &image, 0);
    std::fclose(file);

    const bool dataOk =
        readResult == 0 && image.alphaMap != nullptr && image.palette != nullptr &&
        std::memcmp(pixels, sourcePixels, sizeof(sourcePixels)) == 0 &&
        std::memcmp(image.alphaMap, sourceAlpha, sizeof(sourceAlpha)) == 0 &&
        std::memcmp(image.palette, sourcePalette, sizeof(sourcePalette)) == 0 &&
        (image.formatFlagsPacked & 0xc8) == 0xc8;
    std::free(image.alphaMap);
    std::free(image.palette);
    if (!dataOk) {
        return 2;
    }

    unsigned char largerHintPixel = 0xaa;
    zVidImagePartial largerHintImage{};
    largerHintImage.pixelCount = 1;
    largerHintImage.formatFlagsPacked = 0;
    largerHintImage.pixels = &largerHintPixel;

    file = std::tmpfile();
    if (file == nullptr) {
        return 3;
    }
    const bool largerHintOk =
        zVid_Image::ReadData(file, &largerHintImage, 2) == 0 && largerHintPixel == 0xaa;
    std::fclose(file);
    if (!largerHintOk) {
        return 4;
    }

    unsigned char shortPixels[4] = {};
    zVidImagePartial shortReadImage{};
    shortReadImage.pixelCount = 4;
    shortReadImage.formatFlagsPacked = 0;
    shortReadImage.pixels = shortPixels;

    file = std::tmpfile();
    if (file == nullptr) {
        return 5;
    }
    std::fputc(0x7f, file);
    std::rewind(file);
    const bool shortReadOk = zVid_Image::ReadData(file, &shortReadImage, 0) == -1;
    std::fclose(file);

    return shortReadOk ? 0 : 6;
}

extern "C" int zvideo_palette_remap_no_recipes_smoke(void) {
    std::uint16_t palette[2] = {1, 2};
    g_zVid_PaletteRemapRecipeCount = 0;
    g_zVid_PaletteRemapRecipes = nullptr;
    return zVid_PaletteRemap_BuildAllRecipeVariantsForPalette(palette, 2) == palette ? 0 : 1;
}

extern "C" int zvideo_palette_remap_recipe_variants_smoke(void) {
    g_zVideo_PixelPack.rMaskShifted = 0xf8;
    g_zVideo_PixelPack.gMaskShifted = 0xfc;
    g_zVideo_PixelPack.packedBase = 8;
    g_zVideo_PixelPack.sumMinus8 = 3;
    g_zVideo_PixelPack.bShiftTo8 = 3;
    g_zVideo_PixelPack.rBits = 5;
    g_zVideo_PixelPack.gBits = 6;
    g_zVideo_PixelPack.bBits = 5;

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
    g_zVideo_PixelPack.rBits = 0;
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
    const int oldVideoInitialized = g_zVideo_IsInitialized;
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
        g_zVideo_IsInitialized = oldVideoInitialized;
        return 2;
    }

    FakeDirectDrawSurface3Object surface{};
    InstallFakeDirectDrawSurface3(surface, DD_OK, DD_OK, DD_OK);
    image.surface = reinterpret_cast<IDirectDrawSurface3 *>(&surface);
    image.pixels = reinterpret_cast<void *>(0x5678);
    g_zVideo_IsInitialized = 1;
    zVideo_dd::Image_EnsureSurfaceForCurrentDevice(&image);
    if (gFakeDirectDrawSurface3ReleaseCalls != 1 || image.surface != nullptr ||
        image.pixels != nullptr) {
        g_zVideo_IsInitialized = oldVideoInitialized;
        return 3;
    }

    HDC hdc = nullptr;
    g_zVideo_RendererType = 2;
    if (zVideo_dd::Image_UploadPixelsToSurface(&image, &hdc) != 0) {
        g_zVideo_IsInitialized = oldVideoInitialized;
        return 4;
    }

    const int result = zVideo_dd::Image_ReleaseSurface(&image, hdc) == 0 ? 0 : 5;
    g_zVideo_IsInitialized = oldVideoInitialized;
    return result;
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

extern "C" int zvideo_dd_prepare_window_for_mode_smoke(void) {
    const HWND savedHwnd = g_zVideo_hWnd;
    PALETTEENTRY savedSystemPalette[256];
    PALETTEENTRY expectedSystemPalette[256];
    std::memcpy(savedSystemPalette, g_zVideo_SystemPaletteEntries, sizeof(savedSystemPalette));
    std::memset(expectedSystemPalette, 0, sizeof(expectedSystemPalette));

    HDC screenDc = GetDC(nullptr);
    const bool paletteDesktop =
        screenDc != nullptr && (GetDeviceCaps(screenDc, RASTERCAPS) & RC_PALETTE) != 0;
    UINT expectedPaletteCount = 0;
    if (paletteDesktop) {
        expectedPaletteCount =
            GetSystemPaletteEntries(screenDc, 0, 256, expectedSystemPalette);
    }
    if (screenDc != nullptr) {
        ReleaseDC(nullptr, screenDc);
    }

    std::memset(g_zVideo_SystemPaletteEntries, 0x44, sizeof(g_zVideo_SystemPaletteEntries));
    HWND hwnd = CreateWindowExA(
        0,
        "STATIC",
        "recoil-video-prepare-window-test",
        WS_OVERLAPPEDWINDOW,
        20,
        30,
        160,
        120,
        nullptr,
        nullptr,
        GetModuleHandleA(nullptr),
        nullptr
    );
    if (hwnd == nullptr) {
        std::memcpy(g_zVideo_SystemPaletteEntries, savedSystemPalette, sizeof(savedSystemPalette));
        g_zVideo_hWnd = savedHwnd;
        return 1;
    }

    HMENU menu = CreateMenu();
    if (menu != nullptr) {
        SetMenu(hwnd, menu);
    }

    g_zVideo_hWnd = hwnd;
    const int result = zVideo_dd::PrepareWindowForMode();
    const LONG exStyle = GetWindowLongA(hwnd, GWL_EXSTYLE);
    const LONG style = GetWindowLongA(hwnd, GWL_STYLE);
    const bool fullscreenStyleOk =
        (style & (LONG)(WS_POPUP | WS_CLIPCHILDREN)) ==
            (LONG)(WS_POPUP | WS_CLIPCHILDREN) &&
        (style & (LONG)(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX |
                        WS_SYSMENU)) == 0;
    const bool paletteOk =
        paletteDesktop && expectedPaletteCount == 256
            ? std::memcmp(
                  g_zVideo_SystemPaletteEntries,
                  expectedSystemPalette,
                  sizeof(expectedSystemPalette)
              ) == 0
            : g_zVideo_SystemPaletteEntries[0].peRed == 0x44 &&
                  g_zVideo_SystemPaletteEntries[0].peGreen == 0x44 &&
                  g_zVideo_SystemPaletteEntries[0].peBlue == 0x44 &&
                  g_zVideo_SystemPaletteEntries[0].peFlags == 0x44;
    int failureCode = 0;
    if (result != 0) {
        failureCode = 2;
    } else if (GetMenu(hwnd) != nullptr) {
        failureCode = 3;
    } else if (exStyle != WS_EX_APPWINDOW) {
        failureCode = 4;
    } else if (!fullscreenStyleOk) {
        failureCode = 5;
    } else if (!paletteOk) {
        failureCode = 6;
    }

    if (menu != nullptr) {
        DestroyMenu(menu);
    }
    DestroyWindow(hwnd);
    std::memcpy(g_zVideo_SystemPaletteEntries, savedSystemPalette, sizeof(savedSystemPalette));
    g_zVideo_hWnd = savedHwnd;
    return failureCode;
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

void VideoShutdownFake() {
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
