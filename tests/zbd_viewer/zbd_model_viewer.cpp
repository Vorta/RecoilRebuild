#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <windows.h>
#include <dbghelp.h>

#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <filesystem>
#include <algorithm>
#include <string>
#include <vector>

extern "C" {
extern unsigned int g_zVideo_pfnBltSwToPrimaryRect;
extern unsigned int g_zVideo_pfnImageLazyCreateVideoMemorySurface;
extern unsigned int g_zVideo_pfnSetFogEnable;
extern unsigned int g_zVideo_pfnSetFogStart;
extern unsigned int g_zVideo_pfnSetFogEnd;
extern unsigned int g_zVideo_pfnApplyFogStateFromGlobals;
extern unsigned int g_zVideo_pfnSubmitPolyFlatColor16;
extern unsigned int g_zVideo_pfnSubmitPolyGouraudColor16;
extern unsigned int g_zVideo_pfnSubmitPolyColorAttr;
extern unsigned int g_zVideo_pfnSubmitPolyRenderClass;
extern unsigned int g_zVideo_pfnSubmitPolygon;
extern unsigned int g_zVideo_pfnSubmitPolygonLit;
}

namespace {

enum ViewerRenderMode {
    kViewerRenderGallery = 0,
    kViewerRenderMission = 1
};

struct ViewerOptions {
    std::filesystem::path runtimeDir;
    std::string zbdPath;
    int mission;
    int modeIndex;
    int fullscreen;
    ViewerRenderMode renderMode;
    int assetIndex;
    int assetYawDeg;
    int faceIndex;
    int smokeFrames;
    int textureSwatches;
    std::filesystem::path dumpFramePath;
};

struct ViewerState {
    HWND window;
    bool running;
    bool reloadRequested;
    bool sceneLoaded;
    bool galleryReady;
    int frameCount;
    int lastFrameNonClearPixels;
    int lastFrameUniqueColors;
    int lastFrameNonWhitePixels;
    std::string phase;
    std::string zbdPath;
};

ViewerState g_viewer = {};
int g_viewerTextureCreateAttempts = 0;
int g_viewerTextureCreateSuccesses = 0;
int g_viewerTextureCreateFallbacks = 0;
zVideo_TextureRecordPartial g_viewerMissingTextureRecord = {};

struct ViewerSubmitCounters {
    int texturedSubmitCalls;
    int untexturedSubmitCalls;
    int polygonSubmitCalls;
    int sortedFlushCalls;
    int overwriteFlushCalls;
    int quadFlushCalls;
};

struct GalleryAsset {
    zClass_NodePartial *node;
    zDiPartial *displayInstance;
    int nodeIndex;
    int texturedMaterialCount;
    int validTexturedMaterialCount;
    int missingTextureCount;
    int zeroHandleTextureCount;
    int defaultTextureCount;
    int missingUvCount;
    int polygonCount;
};

struct SelectedHardwareRenderer {
    int deviceIndex;
    char directDrawDescription[0x60];
    char d3dDeviceName[0x20];
};

typedef void(*ViewerVideoFlushProc)();
typedef void(__fastcall *ViewerSubmitFlatColor16Proc)(
    zVideo_XyzVertex *vertices,
    unsigned int packedColor16,
    int alpha,
    int renderParam,
    int vertexCount,
    int queueMode
);
typedef void(__fastcall *ViewerSubmitGouraudColor16Proc)(
    zVideo_XyzVertex *vertices,
    unsigned int *packedColors16,
    int alpha,
    int renderParam,
    int vertexCount,
    int queueMode
);
typedef void(__fastcall *ViewerSubmitColorAttrProc)(
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
typedef void(__fastcall *ViewerSubmitRenderClassProc)(
    zVideo_XyzVertex *vertices,
    zVideo_TexCoord *texCoords,
    int vertexCount,
    zVideo_RenderClass *renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
);
typedef void(__fastcall *ViewerSubmitPolygonProc)(
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

const int kLoadedNodeActiveTag = 0x01000000;
const int kZClassNodeCamera = 1;
const int kZClassNodeWorld = 2;
const int kZClassNodeWindow = 3;
const int kZClassNodeDisplay = 4;
const int kZClassNodeObject3D = 5;
const int kZClassNodeLod = 6;
const int kZClassNodeLight = 9;
const int kZClassNodeSound = 10;

ViewerSubmitCounters g_viewerSubmitCounters = {};
std::vector<GalleryAsset> g_galleryAssets;
int g_gallerySelectedAsset = 0;
zClass_NodePartial g_galleryWindowNode = {};
zClass_WindowDataPartial g_galleryWindowData = {};
zClass_CameraDataPartial g_galleryCameraData = {};

void CycleActiveCamera();
void PrintGalleryTextureDiagnostics(
    const GalleryAsset &asset
);
void PrintSelectedGalleryAssetDiagnostics(
    const char *label
);

template<typename T>
unsigned int DispatchBits(T pointer) {
    return (unsigned int)(reinterpret_cast<uintptr_t>(pointer));
}

LONG WINAPI ViewerUnhandledExceptionFilter(
    EXCEPTION_POINTERS *exceptionInfo
) {
    char moduleName[MAX_PATH] = {};
    HMODULE module = 0;
    if (GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)(exceptionInfo->ExceptionRecord->ExceptionAddress),
            &module
        ) != 0) {
        GetModuleFileNameA(
            module,
            moduleName,
            sizeof(moduleName)
        );
    }

    fprintf(
        stderr,
        "ERROR: Unhandled exception 0x%08lx at %p phase=%s module=%s base=%p.\n",
        (unsigned long)(exceptionInfo->ExceptionRecord->ExceptionCode),
        exceptionInfo->ExceptionRecord->ExceptionAddress,
        g_viewer.phase.c_str(),
        moduleName[0] != 0 ? moduleName : "<unknown>",
        (void *)(module)
    );

    HANDLE const process = GetCurrentProcess();
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    if (SymInitialize(
            process,
            0,
            TRUE
        ) != 0) {
        CONTEXT context = *exceptionInfo->ContextRecord;
        STACKFRAME64 frame = {};
#if defined(_M_IX86)
        const DWORD machineType = IMAGE_FILE_MACHINE_I386;
        frame.AddrPC.Offset = context.Eip;
        frame.AddrFrame.Offset = context.Ebp;
        frame.AddrStack.Offset = context.Esp;
#elif defined(_M_X64)
        const DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
        frame.AddrPC.Offset = context.Rip;
        frame.AddrFrame.Offset = context.Rbp;
        frame.AddrStack.Offset = context.Rsp;
#else
        const DWORD machineType = 0;
#endif
        frame.AddrPC.Mode = AddrModeFlat;
        frame.AddrFrame.Mode = AddrModeFlat;
        frame.AddrStack.Mode = AddrModeFlat;
        fprintf(
            stderr,
            "Stack trace:\n"
        );
        for (int i = 0; i < 64; ++i) {
            if (machineType == 0 ||
                StackWalk64(
                    machineType,
                    process,
                    GetCurrentThread(),
                    &frame,
                    &context,
                    0,
                    SymFunctionTableAccess64,
                    SymGetModuleBase64,
                    0
                ) == 0 ||
                frame.AddrPC.Offset == 0) {
                break;
            }

            char storage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
            SYMBOL_INFO *symbol = (SYMBOL_INFO *)(storage);
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol->MaxNameLen = MAX_SYM_NAME;
            DWORD64 address = frame.AddrPC.Offset;
            DWORD64 displacement = 0;
            IMAGEHLP_LINE64 line = {};
            DWORD lineDisplacement = 0;
            line.SizeOfStruct = sizeof(line);
            if (SymFromAddr(
                    process,
                    address,
                    &displacement,
                    symbol
                ) != 0) {
                if (SymGetLineFromAddr64(
                        process,
                        address,
                        &lineDisplacement,
                        &line
                    ) != 0) {
                    fprintf(
                        stderr,
                        "  %02d %p %s + 0x%llx (%s:%lu)\n",
                        i,
                        (void *)((uintptr_t)(address)),
                        symbol->Name,
                        (unsigned long long)(displacement),
                        line.FileName,
                        (unsigned long)(line.LineNumber)
                    );
                } else {
                    fprintf(
                        stderr,
                        "  %02d %p %s + 0x%llx\n",
                        i,
                        (void *)((uintptr_t)(address)),
                        symbol->Name,
                        (unsigned long long)(displacement)
                    );
                }
            } else {
                fprintf(
                    stderr,
                    "  %02d %p <unknown>\n",
                    i,
                    (void *)((uintptr_t)(address))
                );
            }
        }
    }

    fflush(stderr);
    return EXCEPTION_EXECUTE_HANDLER;
}

bool IsKnownOriginalAddress(uintptr_t value) {
    static const uintptr_t knownAddresses[] = {
        0x004076f0,
        0x004a7d40,
        0x004a7d90,
        0x004a7e10,
        0x004a8220,
        0x004a82f0,
        0x004a84c0,
        0x004a8650,
        0x004a8680,
        0x004a86f0,
        0x004a9920,
        0x004aa8b0,
        0x004aa8f0,
        0x004aa900,
        0x004aa920,
        0x004aa980,
        0x004aa9e0,
        0x004aaa30,
        0x004aaa60,
        0x004aaa90,
        0x004aab30,
        0x004aab90,
        0x004aaef0,
        0x004ab320,
        0x004ab6d0,
        0x004abb20,
        0x004ac370,
        0x004acbd0,
        0x004ace30,
        0x004ad120,
        0x004ad250
    };

    for (size_t i = 0; i < sizeof(knownAddresses) / sizeof(knownAddresses[0]); ++i) {
        if (value == knownAddresses[i]) {
            return true;
        }
    }

    return false;
}

bool CheckDispatchPointer(
    const char *name,
    uintptr_t value,
    bool allowNull
) {
    if (value == 0) {
        if (allowNull) {
            return true;
        }

        fprintf(
            stderr,
            "ERROR: %s is null.\n",
            name
        );
        return false;
    }

    if (IsKnownOriginalAddress(value)) {
        fprintf(
            stderr,
            "ERROR: %s still points at original image address 0x%08Ix.\n",
            name,
            value
        );
        return false;
    }

    return true;
}

void __fastcall TextureRecordFinalizeUploadDispatchAdapter(
    zVideo_TextureRecordPartial *textureRecord,
    void *,
    zVidImagePartial *image
) {
    zVideo_dd3d::TextureRecord_FinalizeUpload(
        textureRecord,
        image
    );
}

zVideo_TextureRecordPartial *__fastcall TextureRecordCreateDispatchAdapter(
    const char *textureName,
    zVidImagePartial *image,
    int useAlpha,
    int clampU,
    int clampV
) {
    ++g_viewerTextureCreateAttempts;
    zVideo_TextureRecordPartial *const texture = zVideo_dd3d::CreateTextureRecord(
        textureName,
        image,
        useAlpha,
        clampU,
        clampV
    );
    if (texture != 0 &&
        (textureName == 0 || g_zImage_DefaultTextureRecord == 0 ||
            texture != g_zImage_DefaultTextureRecord) &&
        texture->m_textureHandle != 0) {
        ++g_viewerTextureCreateSuccesses;
        return texture;
    }

    ++g_viewerTextureCreateFallbacks;
    if (g_viewerTextureCreateAttempts <= 8) {
        fprintf(
            stderr,
            "WARNING: CreateTextureRecord returned null for %s (%dx%d flags=%u); using dummy no-texture record.\n",
            textureName != 0 ? textureName : "<default>",
            image != 0 ? image->width : 0,
            image != 0 ? image->height : 0,
            image != 0 ? (unsigned int)(image->formatFlagsPacked) : 0
        );
    }

    g_viewerMissingTextureRecord.m_textureHandle = 0;
    g_viewerMissingTextureRecord.m_alphaMode = 1;
    g_viewerMissingTextureRecord.m_uWrapMode = D3DTADDRESS_WRAP;
    g_viewerMissingTextureRecord.m_vWrapMode = D3DTADDRESS_WRAP;
    return &g_viewerMissingTextureRecord;
}

int __fastcall QueryTextureMemoryBytesForViewer(
    int,
    int *outTotalBytes,
    int *outFreeBytes
) {
    if (outTotalBytes != 0) {
        *outTotalBytes = 8 * 1024 * 1024;
    }
    if (outFreeBytes != 0) {
        *outFreeBytes = 8 * 1024 * 1024;
    }
    return 1;
}

void ResetSubmitCounters() {
    memset(
        &g_viewerSubmitCounters,
        0,
        sizeof(g_viewerSubmitCounters)
    );
}

void __fastcall SubmitPolyFlatColor16DispatchAdapter(
    zVideo_XyzVertex *vertices,
    unsigned int packedColor16,
    int alpha,
    int renderParam,
    int vertexCount,
    int queueMode
) {
    ++g_viewerSubmitCounters.untexturedSubmitCalls;
    zVideo_dd3d::SubmitPolyFlatColor16(
        vertices,
        packedColor16,
        alpha,
        renderParam,
        vertexCount,
        queueMode
    );
}

void __fastcall SubmitPolyGouraudColor16DispatchAdapter(
    zVideo_XyzVertex *vertices,
    unsigned int *packedColors16,
    int alpha,
    int renderParam,
    int vertexCount,
    int queueMode
) {
    ++g_viewerSubmitCounters.untexturedSubmitCalls;
    zVideo_dd3d::SubmitPolyGouraudColor16(
        vertices,
        packedColors16,
        alpha,
        renderParam,
        vertexCount,
        queueMode
    );
}

void __fastcall SubmitPolyColorAttrDispatchAdapter(
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
) {
    ++g_viewerSubmitCounters.untexturedSubmitCalls;
    zVideo_dd3d::SubmitPolyColorAttr(
        vertices,
        packedColor16,
        baseColor,
        attr1,
        attr0,
        attr2,
        alpha,
        vertexCount,
        renderParam,
        queueMode
    );
}

void __fastcall SubmitPolyRenderClassDispatchAdapter(
    zVideo_XyzVertex *vertices,
    zVideo_TexCoord *texCoords,
    int vertexCount,
    zVideo_RenderClass *renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
) {
    ++g_viewerSubmitCounters.texturedSubmitCalls;
    zVideo_dd3d::SubmitPolyRenderClass(
        vertices,
        texCoords,
        vertexCount,
        renderClass,
        renderParam,
        alpha,
        queueMode
    );
}

void __fastcall SubmitPolygonDispatchAdapter(
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
) {
    ++g_viewerSubmitCounters.polygonSubmitCalls;
    zVideo_dd3d::SubmitPolygon(
        vertices,
        uvPairs,
        attr1,
        attr0,
        attr2,
        vertexCount,
        renderClass,
        renderParam,
        alpha,
        queueMode
    );
}

void __fastcall SubmitPolygonLitDispatchAdapter(
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
) {
    ++g_viewerSubmitCounters.polygonSubmitCalls;
    zVideo_dd3d::SubmitPolygonLit(
        vertices,
        uvPairs,
        attr1,
        attr0,
        attr2,
        vertexCount,
        renderClass,
        renderParam,
        alpha,
        queueMode
    );
}

void FlushSortedPolysDispatchAdapter() {
    ++g_viewerSubmitCounters.sortedFlushCalls;
    zVideo_dd3d::FlushSortedPolys();
}

void FlushOverwritePolysDispatchAdapter() {
    ++g_viewerSubmitCounters.overwriteFlushCalls;
    zVideo_dd3d::FlushOverwritePolys();
}

void FlushQuadBatchDispatchAdapter() {
    ++g_viewerSubmitCounters.quadFlushCalls;
    zVideo_dd3d::FlushQuadBatch();
}

void InstallStandaloneD3DDispatchOverrides() {
    g_zVideo_pfnShutdownVideoSystem =
        (zVideo_ShutdownVideoSystemProc)(zVideo_dd::ShutdownVideoSystem);
    g_zVideo_pfnOpenVideoMode = zVideo_dd::OpenVideoMode;
    g_zVideo_pfnSetVideoMode = zVideo_dd::SetVideoMode;
    g_zVideo_pfnAdjustSurfaces = zVideo_dd3d::PresentDisplayModeSurface;
    g_zVideo_pfnLockSurfaceState = zVideo_dd::LockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = zVideo_dd::UnlockSurfaceState;
    g_zVideo_pfnClearZBufferRect = zVideo_dd::ZBuffer_DepthFillRect;
    g_zVideo_pfnClearSwSurfaceAndZBuffer = zVideo_dd::ClearSwBackbufferAndZBufferRects;
    g_zVideo_pfnClearStateSurfaceAndZBuffer = zVideo_dd::ClearScreenAndZBufferRect;
    g_zVideo_pfnUpdateFogColor = zVideo_dd3d::UpdateFogColor;
    g_zVideo_pfnQueryTextureMemoryBytes = QueryTextureMemoryBytesForViewer;
    g_zVideo_pfnQueryDeviceVideoMemoryBytes = zVid::QueryDeviceVideoMemoryBytes;
    g_zVideo_pfnBltSwToPrimaryRectDirect = zVideo_dd::BltSwToPrimaryRectDirect;
    g_zVideo_pfnBltPrimaryToSwRectDirect = zVideo_dd::BltPrimaryToSwRectDirect;
    g_zVideo_pfnBltSwToPrimaryRect = DispatchBits(zVideo_dd::BltSwToPrimaryRect);
    g_zVideo_pfnGetHwApiDeviceFeatureFlags = 0;
    g_zVideo_pfnImageUploadPixelsToSurface =
        DispatchBits(zVideo_dd::Image_UploadPixelsToSurface);
    g_zVideo_pfnImageReleaseSurface = DispatchBits(zVideo_dd::Image_ReleaseSurface);
    g_zVideo_pfnImageLazyCreateVideoMemorySurface =
        DispatchBits(zVideo_dd::Image_LazyCreateVideoMemorySurface);
    g_zVideo_pfnImageEnsureSurfaceForCurrentDevice =
        DispatchBits(zVideo_dd::Image_EnsureSurfaceForCurrentDevice);
    g_zVideo_pfnCreateTextureRecord = TextureRecordCreateDispatchAdapter;
    g_zVideo_pfnTextureRecordLockUploadSurface =
        DispatchBits(zVideo_dd3d::TextureRecord_LockUploadSurface);
    g_zVideo_pfnTextureRecordUnlockUploadSurface =
        DispatchBits(zVideo_dd3d::TextureRecord_UnlockUploadSurface);
    g_zVideo_pfnTextureRecordReleaseUploadSurfaceRef =
        DispatchBits(zVideo_dd3d::TextureRecord_ReleaseUploadSurfaceRef);
    g_zVideo_pfnTextureRecordFinalizeUpload =
        DispatchBits(TextureRecordFinalizeUploadDispatchAdapter);
    g_zVideo_pfnTextureRecordDestroy = DispatchBits(zVideo_dd3d::TextureRecord_Destroy);
    g_zVideo_pfnTextureRecordReleaseAllUploadSurfaces = 0;
    g_zVideo_pfnSetFogEnable = DispatchBits(zVideo_dd3d::SetFogEnable);
    g_zVideo_pfnSetFogStart = DispatchBits(zVideo_dd3d::SetFogStart);
    g_zVideo_pfnSetFogEnd = DispatchBits(zVideo_dd3d::SetFogEnd);
    g_zVideo_pfnApplyFogStateFromGlobals = DispatchBits(zVideo_dd3d::ApplyFogStateFromGlobals);
    g_zVideo_pfnSubmitPolyFlatColor16 = DispatchBits(SubmitPolyFlatColor16DispatchAdapter);
    g_zVideo_pfnSubmitPolyGouraudColor16 = DispatchBits(SubmitPolyGouraudColor16DispatchAdapter);
    g_zVideo_pfnSubmitPolyColorAttr = DispatchBits(SubmitPolyColorAttrDispatchAdapter);
    g_zVideo_pfnSubmitPolyRenderClass = DispatchBits(SubmitPolyRenderClassDispatchAdapter);
    g_zVideo_pfnSubmitPolygon = DispatchBits(SubmitPolygonDispatchAdapter);
    g_zVideo_pfnSubmitPolygonLit = DispatchBits(SubmitPolygonLitDispatchAdapter);
    g_zVideo_pfnDrawPointColor16 = DispatchBits(zVideo_dd3d::DrawPointColor16);
    g_zVideo_pfnFlushSortedPolys = DispatchBits(FlushSortedPolysDispatchAdapter);
    g_zVideo_pfnFlushOverwritePolys = DispatchBits(FlushOverwritePolysDispatchAdapter);
    g_zVideo_pfnFlushQuadBatch = DispatchBits(FlushQuadBatchDispatchAdapter);
}

bool ValidateStandaloneD3DDispatch() {
    bool ok = true;
    ok = CheckDispatchPointer(
             "g_zVideo_pfnShutdownVideoSystem",
             (uintptr_t)(g_zVideo_pfnShutdownVideoSystem),
             false
         ) &&
         ok;
    ok = CheckDispatchPointer(
             "g_zVideo_pfnClearStateSurfaceAndZBuffer",
             (uintptr_t)(g_zVideo_pfnClearStateSurfaceAndZBuffer),
             false
         ) &&
         ok;
    ok = CheckDispatchPointer(
             "g_zVideo_pfnBltSwToPrimaryRectDirect",
             (uintptr_t)(g_zVideo_pfnBltSwToPrimaryRectDirect),
             false
         ) &&
         ok;
    ok = CheckDispatchPointer(
             "g_zVideo_pfnImageUploadPixelsToSurface",
             g_zVideo_pfnImageUploadPixelsToSurface,
             false
         ) &&
         ok;
    ok = CheckDispatchPointer(
             "g_zVideo_pfnTextureRecordLockUploadSurface",
             g_zVideo_pfnTextureRecordLockUploadSurface,
             false
         ) &&
         ok;
    ok = CheckDispatchPointer(
             "g_zVideo_pfnSubmitPolyRenderClass",
             g_zVideo_pfnSubmitPolyRenderClass,
             false
         ) &&
         ok;
    ok = CheckDispatchPointer(
             "g_zVideo_pfnFlushSortedPolys",
             g_zVideo_pfnFlushSortedPolys,
             false
         ) &&
         ok;
    return ok;
}

void PrintUsage() {
    printf(
        "Usage: recoil_zbd_model_viewer [--runtime-dir PATH] [--mission N] "
        "[--zbd PATH] [--mode-index N] [--fullscreen 0|1] "
        "[--view gallery|mission] [--asset-index N] [--smoke-frames N] "
        "[--asset-yaw-deg N] [--face-index N] "
        "[--texture-swatches 0|1] [--dump-frame PATH]\n"
    );
}

bool ParseInteger(
    const char *text,
    int *outValue
) {
    char *end = 0;
    const long value = strtol(
        text,
        &end,
        10
    );
    if (end == text || *end != 0) {
        return false;
    }

    *outValue = (int)(value);
    return true;
}

std::filesystem::path FindRuntimeDir() {
    std::filesystem::path current = std::filesystem::current_path();
    for (;;) {
        const std::filesystem::path playgroundCandidate = current / "playground";
        if (std::filesystem::exists(playgroundCandidate / "zbd")) {
            return std::filesystem::absolute(playgroundCandidate);
        }

        const std::filesystem::path currentCandidate = current;
        if (std::filesystem::exists(currentCandidate / "zbd")) {
            return std::filesystem::absolute(currentCandidate);
        }

        if (!current.has_parent_path() || current.parent_path() == current) {
            break;
        }
        current = current.parent_path();
    }

    return std::filesystem::path();
}

bool ParseOptions(
    int argc,
    char **argv,
    ViewerOptions *options
) {
    options->mission = 1;
    options->modeIndex = 2;
    options->fullscreen = 1;
    options->renderMode = kViewerRenderGallery;
    options->assetIndex = 0;
    options->assetYawDeg = 0;
    options->faceIndex = -1;
    options->smokeFrames = 0;
    options->textureSwatches = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            PrintUsage();
            return false;
        }

        if (i + 1 >= argc) {
            fprintf(
                stderr,
                "ERROR: Missing value for %s.\n",
                argv[i]
            );
            return false;
        }

        const char *const value = argv[++i];
        if (strcmp(argv[i - 1], "--runtime-dir") == 0 ||
            strcmp(argv[i - 1], "--support-dir") == 0) {
            options->runtimeDir = std::filesystem::absolute(value);
        } else if (strcmp(argv[i - 1], "--mission") == 0) {
            if (!ParseInteger(
                    value,
                    &options->mission
                )) {
                fprintf(
                    stderr,
                    "ERROR: Invalid mission value: %s.\n",
                    value
                );
                return false;
            }
        } else if (strcmp(argv[i - 1], "--zbd") == 0) {
            options->zbdPath = value;
        } else if (strcmp(argv[i - 1], "--mode-index") == 0) {
            if (!ParseInteger(
                    value,
                    &options->modeIndex
                )) {
                fprintf(
                    stderr,
                    "ERROR: Invalid mode index: %s.\n",
                    value
                );
                return false;
            }
        } else if (strcmp(argv[i - 1], "--fullscreen") == 0) {
            if (!ParseInteger(
                    value,
                    &options->fullscreen
                )) {
                fprintf(
                    stderr,
                    "ERROR: Invalid fullscreen value: %s.\n",
                    value
                );
                return false;
            }
            options->fullscreen = options->fullscreen != 0 ? 1 : 0;
        } else if (strcmp(argv[i - 1], "--view") == 0) {
            if (strcmp(value, "gallery") == 0) {
                options->renderMode = kViewerRenderGallery;
            } else if (strcmp(value, "mission") == 0) {
                options->renderMode = kViewerRenderMission;
            } else {
                fprintf(
                    stderr,
                    "ERROR: Invalid view mode: %s.\n",
                    value
                );
                return false;
            }
        } else if (strcmp(argv[i - 1], "--asset-index") == 0) {
            if (!ParseInteger(
                    value,
                    &options->assetIndex
                )) {
                fprintf(
                    stderr,
                    "ERROR: Invalid asset index: %s.\n",
                    value
                );
                return false;
            }
            if (options->assetIndex < 0) {
                options->assetIndex = 0;
            }
        } else if (strcmp(argv[i - 1], "--asset-yaw-deg") == 0) {
            if (!ParseInteger(
                    value,
                    &options->assetYawDeg
                )) {
                fprintf(
                    stderr,
                    "ERROR: Invalid asset-yaw-deg value: %s.\n",
                    value
                );
                return false;
            }
        } else if (strcmp(argv[i - 1], "--face-index") == 0) {
            if (!ParseInteger(
                    value,
                    &options->faceIndex
                )) {
                fprintf(
                    stderr,
                    "ERROR: Invalid face-index value: %s.\n",
                    value
                );
                return false;
            }
            if (options->faceIndex < 0) {
                options->faceIndex = -1;
            }
        } else if (strcmp(argv[i - 1], "--smoke-frames") == 0) {
            if (!ParseInteger(
                    value,
                    &options->smokeFrames
                )) {
                fprintf(
                    stderr,
                    "ERROR: Invalid smoke frame count: %s.\n",
                    value
                );
                return false;
            }
            if (options->smokeFrames < 0) {
                options->smokeFrames = 0;
            }
        } else if (strcmp(argv[i - 1], "--texture-swatches") == 0) {
            if (!ParseInteger(
                    value,
                    &options->textureSwatches
                )) {
                fprintf(
                    stderr,
                    "ERROR: Invalid texture-swatches value: %s.\n",
                    value
                );
                return false;
            }
            options->textureSwatches = options->textureSwatches != 0 ? 1 : 0;
        } else if (strcmp(argv[i - 1], "--dump-frame") == 0) {
            options->dumpFramePath = value;
        } else {
            fprintf(
                stderr,
                "ERROR: Unknown option: %s.\n",
                argv[i - 1]
            );
            return false;
        }
    }

    if (options->runtimeDir.empty()) {
        options->runtimeDir = FindRuntimeDir();
    }
    if (options->runtimeDir.empty() || !std::filesystem::exists(options->runtimeDir / "zbd")) {
        fprintf(
            stderr,
            "ERROR: Could not locate runtime directory with zbd assets.\n"
        );
        return false;
    }

    if (options->textureSwatches == 0 &&
        (options->smokeFrames != 0 || !options->dumpFramePath.empty())) {
        options->textureSwatches = 1;
    }

    if (options->zbdPath.empty()) {
        char path[64] = {};
        snprintf(
            path,
            sizeof(path),
            "zbd\\m%d\\gamez.zbd",
            options->mission
        );
        options->zbdPath = path;
    }

    return true;
}

void WarnIfRuntimeDllsMissing() {
    char modulePath[MAX_PATH] = {};
    if (GetModuleFileNameA(
            0,
            modulePath,
            sizeof(modulePath)
        ) == 0) {
        return;
    }

    std::filesystem::path exeDir = std::filesystem::path(modulePath).parent_path();
    if (!std::filesystem::exists(exeDir / "DDraw.dll") ||
        !std::filesystem::exists(exeDir / "D3DImm.dll")) {
        fprintf(
            stderr,
            "WARNING: Viewer executable is not next to DDraw.dll and D3DImm.dll; "
            "run it from playground or build recoil_zbd_model_viewer_copy_to_playground.\n"
        );
    }
}

LRESULT CALLBACK ViewerWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wparam,
    LPARAM lparam
) {
    switch (message) {
    case WM_CLOSE:
        g_viewer.running = false;
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        g_viewer.running = false;
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE) {
            g_viewer.running = false;
            DestroyWindow(hwnd);
            return 0;
        }
        if (wparam == VK_LEFT && !g_galleryAssets.empty()) {
            --g_gallerySelectedAsset;
            if (g_gallerySelectedAsset < 0) {
                g_gallerySelectedAsset = (int)(g_galleryAssets.size()) - 1;
            }
            g_viewer.frameCount = 0;
            PrintSelectedGalleryAssetDiagnostics("Gallery asset");
            return 0;
        }
        if ((wparam == VK_RIGHT || wparam == VK_SPACE || wparam == 'C') &&
            !g_galleryAssets.empty()) {
            ++g_gallerySelectedAsset;
            if (g_gallerySelectedAsset >= (int)(g_galleryAssets.size())) {
                g_gallerySelectedAsset = 0;
            }
            g_viewer.frameCount = 0;
            PrintSelectedGalleryAssetDiagnostics("Gallery asset");
            return 0;
        }
        if (wparam == 'C') {
            if (g_galleryAssets.empty()) {
                CycleActiveCamera();
            }
            return 0;
        }
        if (wparam == 'R') {
            g_viewer.reloadRequested = true;
            return 0;
        }
        break;
    default:
        break;
    }

    return DefWindowProcA(
        hwnd,
        message,
        wparam,
        lparam
    );
}

HWND CreateViewerWindow(
    HINSTANCE instance,
    int modeIndex
) {
    const char *className = "RecoilZbdModelViewer";
    WNDCLASSA wndClass = {};
    wndClass.lpfnWndProc = ViewerWndProc;
    wndClass.hInstance = instance;
    wndClass.lpszClassName = className;
    wndClass.hCursor = LoadCursorA(
        0,
        IDC_ARROW
    );

    if (RegisterClassA(&wndClass) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return 0;
    }

    zVideo::Init_SetSurfaceGeometryFromModeIndex(modeIndex);
    RECT rect = {
        0,
        0,
        g_zVideo_DisplayModeSurfaceState.width,
        g_zVideo_DisplayModeSurfaceState.height
    };
    AdjustWindowRect(
        &rect,
        WS_OVERLAPPEDWINDOW,
        FALSE
    );

    HWND hwnd = CreateWindowExA(
        0,
        className,
        "Recoil ZBD Model Viewer",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        0,
        0,
        instance,
        0
    );
    return hwnd;
}

bool SelectFirstHardwareRenderer(
    SelectedHardwareRenderer *selection
) {
    zVideo_dd::StartupEnumerateAndDefaultSelect();
    printf(
        "Accepted DirectDraw devices: %d, accepted hardware renderers: %d\n",
        g_zVideo_NumAcceptedDirectDrawDevices,
        g_zVid_AcceptedHardwareRendererCount
    );
    fflush(stdout);
    for (int i = 0; i < g_zVideo_NumAcceptedDirectDrawDevices; ++i) {
        zVidHwApiDeviceRecordPartial *const record = &g_zVideo_HwApiDeviceTable[i];
        printf(
            "Device %d accepted D3D drivers: %d\n",
            i,
            record->m_acceptedD3DDeviceCount
        );
        fflush(stdout);
        if (record->m_acceptedD3DDeviceCount > 0) {
            selection->deviceIndex = i;
            strncpy(
                selection->directDrawDescription,
                record->m_driverDescription,
                sizeof(selection->directDrawDescription) - 1
            );
            strncpy(
                selection->d3dDeviceName,
                record->m_d3dDrivers[0].m_deviceName,
                sizeof(selection->d3dDeviceName) - 1
            );
            printf(
                "Selected DirectDraw device: %s\n",
                selection->directDrawDescription
            );
            printf(
                "Selected Direct3D device: %s\n",
                selection->d3dDeviceName
            );
            fflush(stdout);
            return true;
        }
    }

    fprintf(
        stderr,
        "ERROR: No accepted Direct3D hardware renderer was enumerated.\n"
    );
    return false;
}

void CommitSelectedHardwareRenderer(
    const SelectedHardwareRenderer *selection
) {
    zVidHwApiDeviceRecordPartial *const record =
        &g_zVideo_HwApiDeviceTable[selection->deviceIndex];
    g_zVideo_pSelectedHwApiDeviceRecord = record;
    g_zVideo_pSelectedD3DDeviceInfo = record->m_d3dDrivers;
}

void PrintHresultFailure(
    const char *phase,
    HRESULT hresult
) {
    fprintf(
        stderr,
        "ERROR: %s failed with HRESULT 0x%08lx.\n",
        phase,
        (unsigned long)(hresult)
    );
}

void ReleaseZBufferSurfacesForRetry() {
    if (g_zVideo_pZBufferAttachSurface != 0) {
        g_zVideo_pZBufferAttachSurface->Release();
        g_zVideo_pZBufferAttachSurface = 0;
    }
    if (g_zVideo_pZBufferSurface != 0) {
        g_zVideo_pZBufferSurface->Release();
        g_zVideo_pZBufferSurface = 0;
    }
}

int CreateAndAttachZBuffer(
    DDSURFACEDESC *zBufferDesc,
    const char *label
) {
    HRESULT hresult = g_zVideo_pDirectDraw2->CreateSurface(
        zBufferDesc,
        (IDirectDrawSurface **)(&g_zVideo_pZBufferSurface),
        0
    );
    if (hresult != DD_OK) {
        PrintHresultFailure(
            label,
            hresult
        );
        return 1;
    }

    hresult = g_zVideo_pZBufferSurface->QueryInterface(
        IID_IDirectDrawSurface,
        (void **)(&g_zVideo_pZBufferAttachSurface)
    );
    if (hresult != DD_OK) {
        PrintHresultFailure(
            "zVideo_dd3d::CreateDeviceState/QueryInterface z-buffer attach",
            hresult
        );
        return 1;
    }

    hresult = g_zVideo_SwSurfaceState.surf->AddAttachedSurface(
        (IDirectDrawSurface3 *)(g_zVideo_pZBufferAttachSurface)
    );
    if (hresult != DD_OK) {
        PrintHresultFailure(
            "zVideo_dd3d::CreateDeviceState/AddAttachedSurface z-buffer",
            hresult
        );
        hresult = g_zVideo_SwSurfaceState.surf->AddAttachedSurface(g_zVideo_pZBufferSurface);
        if (hresult != DD_OK) {
            PrintHresultFailure(
                "zVideo_dd3d::CreateDeviceState/AddAttachedSurface z-buffer surface3",
                hresult
            );
            return 1;
        }

        fprintf(
            stderr,
            "WARNING: Attached z-buffer through IDirectDrawSurface3 fallback.\n"
        );
    }

    return 0;
}

int CreateD3DDeviceStateWithDiagnostics() {
    DDSURFACEDESC zBufferDesc = {};
    zBufferDesc.dwWidth = (DWORD)(g_zVideo_SwSurfaceState.width);
    zBufferDesc.dwHeight = (DWORD)(g_zVideo_SwSurfaceState.height);
    g_zVideo_ClearScreenBufferEnabled = 1;
    zBufferDesc.dwSize = sizeof(zBufferDesc);
    zBufferDesc.dwFlags = 0x47;
    zBufferDesc.ddsCaps.dwCaps = 0x24000;
    zBufferDesc.dwMipMapCount = 0x10;

    if (CreateAndAttachZBuffer(
            &zBufferDesc,
            "zVideo_dd3d::CreateDeviceState/CreateSurface z-buffer video memory"
        ) != 0) {
        ReleaseZBufferSurfacesForRetry();
        zBufferDesc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | DDSCAPS_SYSTEMMEMORY;
        fprintf(
            stderr,
            "WARNING: Retrying z-buffer as system-memory surface for wrapper compatibility.\n"
        );
        if (CreateAndAttachZBuffer(
                &zBufferDesc,
                "zVideo_dd3d::CreateDeviceState/CreateSurface z-buffer system memory"
            ) != 0) {
            ReleaseZBufferSurfacesForRetry();
            fprintf(
                stderr,
                "WARNING: Continuing without an attached z-buffer.\n"
            );
        }
    }

    HRESULT hresult = g_zVideo_pDirectDraw2->QueryInterface(
        IID_IDirect3D2,
        (void **)(&g_zVideo_pD3D2)
    );
    if (hresult != DD_OK) {
        PrintHresultFailure(
            "zVideo_dd3d::CreateDeviceState/QueryInterface IDirect3D2",
            hresult
        );
        return 1;
    }

    hresult = g_zVideo_pD3D2->CreateDevice(
        *g_zVideo_pSelectedD3DDeviceInfo->pD3DDeviceGuid,
        (IDirectDrawSurface *)(g_zVideo_SwSurfaceState.surf),
        &g_zVideo_pD3DDevice
    );
    if (hresult != DD_OK) {
        PrintHresultFailure(
            "zVideo_dd3d::CreateDeviceState/CreateDevice",
            hresult
        );
        return 1;
    }

    hresult = g_zVideo_pD3D2->CreateViewport(
        &g_zVideo_pD3DViewport2,
        0
    );
    if (hresult != DD_OK) {
        PrintHresultFailure(
            "zVideo_dd3d::CreateDeviceState/CreateViewport",
            hresult
        );
        return 1;
    }

    hresult = g_zVideo_pD3DDevice->AddViewport(g_zVideo_pD3DViewport2);
    if (hresult != DD_OK) {
        PrintHresultFailure(
            "zVideo_dd3d::CreateDeviceState/AddViewport",
            hresult
        );
        return 1;
    }

    const int width = g_zVideo_UseHalfResBackbuffer != 0 ? g_zVideo_SwSurfaceState.width
                                                         : g_zVideo_DisplayModeSurfaceState.width;
    const int height = g_zVideo_UseHalfResBackbuffer != 0 ? g_zVideo_SwSurfaceState.height
                                                          : g_zVideo_DisplayModeSurfaceState.height;
    D3DVIEWPORT2 viewport2 = {};
    viewport2.dwSize = sizeof(viewport2);
    viewport2.dwX = 0;
    viewport2.dwY = 0;
    viewport2.dwWidth = (DWORD)(width);
    viewport2.dwHeight = (DWORD)(height);
    viewport2.dvClipX = 0.0f;
    viewport2.dvClipY = 0.0f;
    viewport2.dvClipWidth = (D3DVALUE)(width);
    viewport2.dvClipHeight = (D3DVALUE)(height);
    viewport2.dvMinZ = 0.0f;
    viewport2.dvMaxZ = 1.0f;

    hresult = g_zVideo_pD3DViewport2->SetViewport2(&viewport2);
    if (hresult != DD_OK) {
        PrintHresultFailure(
            "zVideo_dd3d::CreateDeviceState/SetViewport2",
            hresult
        );
        return 1;
    }

    hresult = g_zVideo_pD3DDevice->SetCurrentViewport(g_zVideo_pD3DViewport2);
    if (hresult != DD_OK) {
        PrintHresultFailure(
            "zVideo_dd3d::CreateDeviceState/SetCurrentViewport",
            hresult
        );
        return 1;
    }

    hresult = g_zVideo_pD3D2->CreateMaterial(
        &g_zVideo_pD3DMaterial2,
        0
    );
    if (hresult != DD_OK) {
        PrintHresultFailure(
            "zVideo_dd3d::CreateDeviceState/CreateMaterial",
            hresult
        );
        return 1;
    }

    D3DMATERIAL mat = {};
    mat.dwSize = sizeof(mat);
    mat.ambient.r = 1.0f;
    mat.ambient.g = 1.0f;
    mat.ambient.b = 1.0f;
    mat.dwRampSize = 0x100;

    hresult = g_zVideo_pD3DMaterial2->SetMaterial(&mat);
    if (hresult != DD_OK) {
        PrintHresultFailure(
            "zVideo_dd3d::CreateDeviceState/SetMaterial",
            hresult
        );
        return 1;
    }

    hresult = g_zVideo_pD3DMaterial2->GetHandle(
        g_zVideo_pD3DDevice,
        &g_zVideo_D3DMaterialHandle
    );
    if (hresult != DD_OK) {
        PrintHresultFailure(
            "zVideo_dd3d::CreateDeviceState/GetMaterialHandle",
            hresult
        );
        return 1;
    }

    hresult = g_zVideo_pD3DViewport2->SetBackground(g_zVideo_D3DMaterialHandle);
    if (hresult != DD_OK) {
        PrintHresultFailure(
            "zVideo_dd3d::CreateDeviceState/SetBackground",
            hresult
        );
        return 1;
    }

    g_zVideo_D3DHelDeviceDesc.dwSize = sizeof(g_zVideo_D3DHelDeviceDesc);
    g_zVideo_D3DHalDeviceDesc.dwSize = sizeof(g_zVideo_D3DHalDeviceDesc);
    hresult = g_zVideo_pD3DDevice->GetCaps(
        &g_zVideo_D3DHalDeviceDesc,
        &g_zVideo_D3DHelDeviceDesc
    );
    if (hresult != DD_OK) {
        PrintHresultFailure(
            "zVideo_dd3d::CreateDeviceState/GetCaps",
            hresult
        );
        return 1;
    }

    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_CULLMODE,
        1
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_ZENABLE,
        1
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_ZFUNC,
        7
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_SPECULARENABLE,
        0
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_SHADEMODE,
        1
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_TEXTUREPERSPECTIVE,
        1
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_TEXTUREMAG,
        2
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_TEXTUREMIN,
        2
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_SRCBLEND,
        5
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_DESTBLEND,
        6
    );

    g_zVideo_PendingWireframeState = -1;
    zVideo_dd3d::SetFogEnable(1);
    zVideo_dd3d::SetQuadBatchDepthAndRhw(0.99000001f);
    printf("D3D device state created.\n");
    fflush(stdout);
    return 0;
}

int PromoteHalfResRenderSurfaceForD3D() {
    if (g_zVideo_RendererType != 1 || g_zVideo_UseHalfResBackbuffer == 0) {
        return 0;
    }

    if (g_zVideo_SwSurfaceState.surf != 0) {
        g_zVideo_SwSurfaceState.surf->Release();
        g_zVideo_SwSurfaceState.surf = 0;
    }

    DDSURFACEDESC desc = {};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    desc.dwWidth = (DWORD)(g_zVideo_SwSurfaceState.width);
    desc.dwHeight = (DWORD)(g_zVideo_SwSurfaceState.height);
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY;

    const HRESULT hresult = zVideo_dd::CreateSurface3FromDesc(
        g_zVideo_pDirectDraw2,
        &desc,
        &g_zVideo_SwSurfaceState.surf
    );
    if (hresult != DD_OK) {
        PrintHresultFailure(
            "PromoteHalfResRenderSurfaceForD3D/CreateSurface3FromDesc",
            hresult
        );
        return 1;
    }

    fprintf(
        stderr,
        "WARNING: Recreated half-res render surface as D3D-capable video surface.\n"
    );
    return 0;
}

int RunSetVideoModeWithDiagnostics() {
    if (zVideo_dd::SetDisplayMode() == 0) {
        fprintf(
            stderr,
            "ERROR: SetVideoMode phase failed: zVideo_dd::SetDisplayMode.\n"
        );
        return 1;
    }

    if (zVideo_dd::RestoreDisplaySurfaces() != 0) {
        fprintf(
            stderr,
            "ERROR: SetVideoMode phase failed: zVideo_dd::RestoreDisplaySurfaces before reset.\n"
        );
        return 1;
    }

    if (zVideo_dd::ReleaseAllInterfacesAndSurfaces() != 0) {
        fprintf(
            stderr,
            "ERROR: SetVideoMode phase failed: zVideo_dd::ReleaseAllInterfacesAndSurfaces.\n"
        );
        return 1;
    }

    if (zVideo_dd::CreateFullscreenSurfacesForRenderer() != 0) {
        fprintf(
            stderr,
            "ERROR: SetVideoMode phase failed: zVideo_dd::CreateFullscreenSurfacesForRenderer.\n"
        );
        return 1;
    }

    if (PromoteHalfResRenderSurfaceForD3D() != 0) {
        fprintf(
            stderr,
            "ERROR: SetVideoMode phase failed: PromoteHalfResRenderSurfaceForD3D.\n"
        );
        return 1;
    }

    if (g_zVideo_RendererType == 1 && CreateD3DDeviceStateWithDiagnostics() != 0) {
        fprintf(
            stderr,
            "ERROR: SetVideoMode phase failed: zVideo_dd3d::CreateDeviceState.\n"
        );
        return 1;
    }

    if (zVideo_dd::RestoreDisplaySurfaces() != 0) {
        fprintf(
            stderr,
            "ERROR: SetVideoMode phase failed: zVideo_dd::RestoreDisplaySurfaces after device state.\n"
        );
        return 1;
    }

    if (zVideo_dd::VerifyFullscreenSurfaceLocks() != 0) {
        fprintf(
            stderr,
            "ERROR: SetVideoMode phase failed: zVideo_dd::VerifyFullscreenSurfaceLocks.\n"
        );
        return 1;
    }

    printf("Standalone D3D mode setup complete.\n");
    fflush(stdout);
    return 0;
}

int InitVideoStandalone(
    HWND hwnd,
    int fullscreen,
    int modeIndex
) {
    SelectedHardwareRenderer selection = {};
    if (!SelectFirstHardwareRenderer(&selection)) {
        return 1;
    }

    g_zVideo_hWnd = hwnd;
    g_zVideo_FrameTick = 0;
    g_zVideo_pSelectedHwApiDeviceRecord = 0;
    g_zVideo_pSelectedD3DDeviceInfo = 0;
    zVideo::BindRendererDispatch(
        1,
        fullscreen
    );
    InstallStandaloneD3DDispatchOverrides();
    CommitSelectedHardwareRenderer(&selection);

    if (!ValidateStandaloneD3DDispatch()) {
        return 2;
    }

    const int openResult = g_zVideo_pfnOpenVideoMode(modeIndex);
    if (openResult != 0) {
        fprintf(
            stderr,
            "ERROR: zVideo_dd::OpenVideoMode failed with %d.\n",
            openResult
        );
        return openResult;
    }

    ShowCursor(FALSE);
    g_zVideo_IsInitialized = 1;
    zVideo::Init_SetSurfaceGeometryFromModeIndex(modeIndex);
    const int setModeResult = RunSetVideoModeWithDiagnostics();
    if (setModeResult != 0) {
        fprintf(
            stderr,
            "ERROR: Standalone D3D mode setup failed with %d.\n",
            setModeResult
        );
        zVideo::ShutdownVideoSystem();
        return setModeResult;
    }

    zImage_Init(0);
    g_zImage_TextureMemoryDefault = 1;
    g_zImage_TextureMemoryOption = &g_zImage_TextureMemoryDefault;
    zVid::SetTexturePackLoadState(1);
    g_zImage_DefaultTextureRecord = g_zVideo_pfnCreateTextureRecord(
        0,
        &zVid_Image::g_zImage_DefaultImage,
        0,
        0,
        0
    );
    g_zVideo_QuadBatchCount = 0;
    for (int i = 0; i < 16; ++i) {
        g_zVideo_QuadBatchItemsBase[i].vertices[0].specular = 0xff000000;
        g_zVideo_QuadBatchItemsBase[i].vertices[1].specular = 0xff000000;
        g_zVideo_QuadBatchItemsBase[i].vertices[2].specular = 0xff000000;
        g_zVideo_QuadBatchItemsBase[i].vertices[3].specular = 0xff000000;
    }

    zVideo::UpdateCachedClientRectScreenCoords();
    printf(
        "Pixel pack: rBits=%d gBits=%d bBits=%d rMask=0x%04x gMask=0x%04x bMask=0x%04x.\n",
        g_zVideo_PixelPack.rBits,
        g_zVideo_PixelPack.gBits,
        g_zVideo_PixelPack.bBits,
        g_zVideo_PixelPack.rMask,
        g_zVideo_PixelPack.gMask,
        g_zVideo_PixelPack.bMask
    );
    fflush(stdout);
    if (g_zImage_DefaultTextureRecord == 0 ||
        g_zImage_DefaultTextureRecord == &g_viewerMissingTextureRecord) {
        fprintf(
            stderr,
            "WARNING: Standalone default texture record is unavailable; invalid material textures will use diagnostic colors.\n"
        );
    }
    return 0;
}

void ResetSceneGlobals() {
    g_galleryAssets.clear();
    g_viewer.galleryReady = false;
    zModel_Display::Shutdown();
    for (int bucket = 0; bucket < 16; ++bucket) {
        zClass_TypeList::Head(bucket) = 0;
        zClass_TypeList::Tail(bucket) = 0;
        zClass_TypeList::PendingRemovalDirty(bucket) = 0;
    }
    zClass_TypeList::FreeAll();
    g_zClass_TypeList_FreeLinkHead = 0;
    g_zClass_NodeList_PendingFreeHead = 0;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;
    if (g_zClass_NodeArray != 0) {
        free(g_zClass_NodeArray);
    }
    g_zClass_NodeArray = 0;
    g_zClass_NodeArraySize = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_NodeFreeHeadIndex = -1;
    g_zClass_CurrentZbdPath[0] = 0;
    if (g_zImage_MissionSearchPathList != 0) {
        zUtil_ZRDR_FreeSearchPathList(g_zImage_MissionSearchPathList);
        g_zImage_MissionSearchPathList = 0;
    }
    memset(
        g_zImage_TexDirEntries,
        0,
        sizeof(g_zImage_TexDirEntries)
    );
    g_zImage_TexDirEntryCount = 0;
    g_zModel_MatlPool = 0;
    g_zModel_MatlPoolCapacity = 0;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlFreeHeadIndex = -1;
    g_zModel_MatlActiveHeadIndex = -1;
    g_zModel_DiPoolBase = 0;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;
    gModel_RenderFn = zModel::RenderNodeHardware;
    g_zModel_SoftwarePathActive = 0;
}

void ConfigureTextureSearchPaths(
    const char *zbdPath
) {
    if (g_zUtil_ZRDR_FreePool == 0) {
        g_zUtil_ZRDR_FreePool = zArchiveList_CreateEmpty();
    }

    std::filesystem::path missionDir = std::filesystem::path(zbdPath).parent_path();
    std::string searchPaths;
    if (!missionDir.empty()) {
        searchPaths = missionDir.string();
        searchPaths += ";";
    }
    searchPaths += "zbd";

    zImage_InitMissionResources(searchPaths.c_str());
    printf(
        "Texture search paths: %s\n",
        searchPaths.c_str()
    );
    fflush(stdout);
}

int LoadedNodeIndex(
    const zClass_NodePartial *node
) {
    if (node == 0 || g_zClass_NodeArray == 0 || g_zClass_NodeArraySize <= 0) {
        return -1;
    }

    const uintptr_t base = (uintptr_t)(&g_zClass_NodeArray[0].node);
    const uintptr_t value = (uintptr_t)(node);
    if (value < base) {
        return -1;
    }

    const uintptr_t offset = value - base;
    if ((offset % sizeof(zClass_NodeFreeListSlot)) != 0) {
        return -1;
    }

    const int index = (int)(offset / sizeof(zClass_NodeFreeListSlot));
    if (index < 0 || index >= g_zClass_NodeArraySize) {
        return -1;
    }

    return index;
}

bool IsLoadedNodePointer(
    const zClass_NodePartial *node
) {
    return LoadedNodeIndex(node) >= 0;
}

bool IsLiveLoadedNode(
    const zClass_NodePartial *node
) {
    const int index = LoadedNodeIndex(node);
    return index >= 0 && (g_zClass_NodeArray[index].freeTag & kLoadedNodeActiveTag) != 0;
}

void PrintNodeArraySummary() {
    printf(
        "Node array: base=%p size=%d active=%d freeHead=%d slotSize=%u.\n",
        (void *)(g_zClass_NodeArray),
        g_zClass_NodeArraySize,
        g_zClass_ActiveNodeCount,
        g_zClass_NodeFreeHeadIndex,
        (unsigned int)(sizeof(zClass_NodeFreeListSlot))
    );
}

bool IsUsableRenderCamera(
    zClass_NodePartial *node
) {
    if (!IsLiveLoadedNode(node) || node->classId != kZClassNodeCamera || node->classData == 0) {
        return false;
    }

    zClass_CameraDataPartial *const cameraData = (zClass_CameraDataPartial *)(node->classData);
    if (!IsLiveLoadedNode(cameraData->worldNode) ||
        cameraData->worldNode->classId != kZClassNodeWorld ||
        cameraData->worldNode->classData == 0) {
        return false;
    }

    if (!IsLiveLoadedNode(cameraData->windowNode) ||
        cameraData->windowNode->classId != kZClassNodeWindow ||
        cameraData->windowNode->classData == 0) {
        return false;
    }

    return true;
}

void ClearTypeListBuckets() {
    for (int bucket = 0; bucket < 16; ++bucket) {
        zClass_TypeList::Head(bucket) = 0;
        zClass_TypeList::Tail(bucket) = 0;
        zClass_TypeList::PendingRemovalDirty(bucket) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = 0;
    g_zClass_NodeList_PendingFreeHead = 0;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;
}

void InsertLoadedNodeIntoRuntimeBuckets(
    zClass_NodePartial *node
) {
    zClass_TypeList::Insert(
        6,
        node
    );

    switch (node->classId) {
    case kZClassNodeCamera:
        zClass_TypeList::Insert(
            8,
            node
        );
        break;
    case kZClassNodeWorld:
        zClass_TypeList::Insert(
            13,
            node
        );
        break;
    case kZClassNodeWindow:
        zClass_TypeList::Insert(
            14,
            node
        );
        break;
    case kZClassNodeDisplay:
        zClass_TypeList::Insert(
            15,
            node
        );
        break;
    case kZClassNodeLight:
        zClass_TypeList::Insert(
            9,
            node
        );
        break;
    case kZClassNodeSound:
        zClass_TypeList::Insert(
            10,
            node
        );
        break;
    case kZClassNodeObject3D:
    case kZClassNodeLod:
    default:
        break;
    }
}

void RebuildTypeListsFromLoadedNodes() {
    ClearTypeListBuckets();

    int rebuiltActiveCount = 0;
    int rebuiltCameraCount = 0;
    int usableCameraCount = 0;
    for (int i = 0; i < g_zClass_NodeArraySize; ++i) {
        zClass_NodePartial *const node = &g_zClass_NodeArray[i].node;
        if ((g_zClass_NodeArray[i].freeTag & kLoadedNodeActiveTag) == 0 ||
            node->classData == 0 || node->classId == 0) {
            continue;
        }

        InsertLoadedNodeIntoRuntimeBuckets(node);
        ++rebuiltActiveCount;
        if (node->classId == kZClassNodeCamera) {
            ++rebuiltCameraCount;
            if (IsUsableRenderCamera(node)) {
                ++usableCameraCount;
            }
        }
    }

    printf(
        "Rebuilt type lists: active=%d cameras=%d usableCameras=%d liveLinks=%d.\n",
        rebuiltActiveCount,
        rebuiltCameraCount,
        usableCameraCount,
        g_zClass_TypeList_LiveLinkCount
    );
    fflush(stdout);
}

void PrintTextureRecordSummary() {
    int loadedEntries = 0;
    int entriesWithImages = 0;
    int entriesWithTextures = 0;
    for (int i = 0; i < g_zImage_TexDirEntryCount; ++i) {
        zImage_TexDirEntryPartial *const entry = &g_zImage_TexDirEntries[i];
        if (entry->loadState == 1) {
            ++loadedEntries;
        }
        if (entry->image != 0) {
            ++entriesWithImages;
        }
        if (entry->texture != 0) {
            ++entriesWithTextures;
        }
    }

    printf(
        "Texture records: loaded=%d images=%d textures=%d total=%d createAttempts=%d createSuccesses=%d dummyFallbacks=%d.\n",
        loadedEntries,
        entriesWithImages,
        entriesWithTextures,
        g_zImage_TexDirEntryCount,
        g_viewerTextureCreateAttempts,
        g_viewerTextureCreateSuccesses,
        g_viewerTextureCreateFallbacks
    );
    fflush(stdout);
}

void PrintTexturePackSummary() {
    printf(
        "Texture packs: builtin=%d default=%d.\n",
        g_zVid_BuiltinTexturePackCount,
        g_zVid_TexturePackCount
    );
    for (int i = 0; i < g_zVid_BuiltinTexturePackCount; ++i) {
        printf(
            "  builtin[%d]: %s records=%d handle=%p.\n",
            i,
            g_zVid_BuiltinTexturePacks[i].filePath,
            g_zVid_BuiltinTexturePacks[i].header.recordCount,
            (void *)(g_zVid_BuiltinTexturePacks[i].fileHandle)
        );
    }
    for (int i = 0; i < g_zVid_TexturePackCount; ++i) {
        printf(
            "  default[%d]: %s records=%d handle=%p.\n",
            i,
            g_zVid_TexturePacks[i].filePath,
            g_zVid_TexturePacks[i].header.recordCount,
            (void *)(g_zVid_TexturePacks[i].fileHandle)
        );
    }
    fflush(stdout);
}

bool IsFiniteFloat(
    float value
) {
    return value == value && value > -3.402823466e+38f && value < 3.402823466e+38f;
}

bool IsRenderableDisplayInstance(
    zDiPartial *displayInstance
) {
    return displayInstance != 0 && displayInstance->entryCount > 0 &&
           displayInstance->vertCount > 0 && displayInstance->entries != 0 &&
           displayInstance->verts != 0 && IsFiniteFloat(displayInstance->bboxCenter.x) &&
           IsFiniteFloat(displayInstance->bboxCenter.y) &&
           IsFiniteFloat(displayInstance->bboxCenter.z) &&
           IsFiniteFloat(displayInstance->bboxRadius) && displayInstance->bboxRadius > 0.01f;
}

bool IsDefaultOrDummyTextureRecord(
    zVideo_TextureRecordPartial *texture
) {
    return texture == 0 || texture == &g_viewerMissingTextureRecord ||
           texture == g_zImage_DefaultTextureRecord;
}

bool IsValidMaterialTexture(
    zModel_MaterialPartial *material,
    zDiEntryPartial *entry
) {
    if (material == 0 || (material->flags & 0x0100) == 0 || entry == 0 ||
        entry->uvPairs == 0 || material->currentTextureDirectoryEntry == 0) {
        return false;
    }

    zVideo_TextureRecordPartial *const texture =
        material->currentTextureDirectoryEntry->texture;
    return !IsDefaultOrDummyTextureRecord(texture) && texture->m_textureHandle != 0;
}

void CountTextureMaterials(
    zDiPartial *displayInstance,
    GalleryAsset *asset
) {
    asset->texturedMaterialCount = 0;
    asset->validTexturedMaterialCount = 0;
    asset->missingTextureCount = 0;
    asset->zeroHandleTextureCount = 0;
    asset->defaultTextureCount = 0;
    asset->missingUvCount = 0;

    for (int entryIndex = 0; entryIndex < displayInstance->entryCount; ++entryIndex) {
        zDiEntryPartial *const entry = &displayInstance->entries[entryIndex];
        zModel_MaterialPartial *const material = entry->material;
        if (material == 0 || (material->flags & 0x0100) == 0) {
            continue;
        }

        ++asset->texturedMaterialCount;
        if (entry->uvPairs == 0) {
            ++asset->missingUvCount;
        }
        if (material->currentTextureDirectoryEntry == 0 ||
            material->currentTextureDirectoryEntry->texture == 0) {
            ++asset->missingTextureCount;
            continue;
        }

        zVideo_TextureRecordPartial *const texture =
            material->currentTextureDirectoryEntry->texture;
        if (IsDefaultOrDummyTextureRecord(texture)) {
            ++asset->defaultTextureCount;
            continue;
        }
        if (texture->m_textureHandle == 0) {
            ++asset->zeroHandleTextureCount;
            continue;
        }
        if (entry->uvPairs != 0) {
            ++asset->validTexturedMaterialCount;
        }
    }
}

void AppendGalleryAsset(
    int nodeIndex,
    zClass_NodePartial *node,
    zDiPartial *displayInstance,
    std::vector<GalleryAsset> *texturedModelAssets,
    std::vector<GalleryAsset> *texturedOtherAssets,
    std::vector<GalleryAsset> *plainModelAssets,
    std::vector<GalleryAsset> *plainOtherAssets
) {
    if (node->classId != kZClassNodeObject3D || node->userDataOrDiRef == 0 ||
        (node->flags & 4) == 0) {
        return;
    }

    if (!IsRenderableDisplayInstance(displayInstance)) {
        return;
    }

    GalleryAsset asset = {};
    asset.node = node;
    asset.displayInstance = displayInstance;
    asset.nodeIndex = nodeIndex;
    CountTextureMaterials(
        displayInstance,
        &asset
    );
    asset.polygonCount = displayInstance->entryCount;

    if (asset.validTexturedMaterialCount > 0) {
        if (displayInstance->mode == 0) {
            texturedModelAssets->push_back(asset);
        } else {
            texturedOtherAssets->push_back(asset);
        }
    } else {
        if (displayInstance->mode == 0) {
            plainModelAssets->push_back(asset);
        } else {
            plainOtherAssets->push_back(asset);
        }
    }
}

bool BuildGalleryAssetList(
    int requestedAssetIndex
) {
    std::vector<GalleryAsset> texturedModelAssets;
    std::vector<GalleryAsset> texturedOtherAssets;
    std::vector<GalleryAsset> plainModelAssets;
    std::vector<GalleryAsset> plainOtherAssets;
    g_galleryAssets.clear();

    for (int i = 0; i < g_zClass_NodeArraySize; ++i) {
        zClass_NodePartial *const node = &g_zClass_NodeArray[i].node;
        if (!IsLiveLoadedNode(node)) {
            continue;
        }

        AppendGalleryAsset(
            i,
            node,
            (zDiPartial *)((unsigned int)(node->userDataOrDiRef)),
            &texturedModelAssets,
            &texturedOtherAssets,
            &plainModelAssets,
            &plainOtherAssets
        );
    }

    std::sort(
        texturedModelAssets.begin(),
        texturedModelAssets.end(),
        [](const GalleryAsset &lhs, const GalleryAsset &rhs) {
            return lhs.polygonCount > rhs.polygonCount;
        }
    );
    std::sort(
        texturedOtherAssets.begin(),
        texturedOtherAssets.end(),
        [](const GalleryAsset &lhs, const GalleryAsset &rhs) {
            return lhs.polygonCount > rhs.polygonCount;
        }
    );
    std::sort(
        plainModelAssets.begin(),
        plainModelAssets.end(),
        [](const GalleryAsset &lhs, const GalleryAsset &rhs) {
            return lhs.polygonCount > rhs.polygonCount;
        }
    );
    std::sort(
        plainOtherAssets.begin(),
        plainOtherAssets.end(),
        [](const GalleryAsset &lhs, const GalleryAsset &rhs) {
            return lhs.polygonCount > rhs.polygonCount;
        }
    );

    g_galleryAssets.insert(
        g_galleryAssets.end(),
        texturedModelAssets.begin(),
        texturedModelAssets.end()
    );
    g_galleryAssets.insert(
        g_galleryAssets.end(),
        texturedOtherAssets.begin(),
        texturedOtherAssets.end()
    );
    g_galleryAssets.insert(
        g_galleryAssets.end(),
        plainModelAssets.begin(),
        plainModelAssets.end()
    );
    g_galleryAssets.insert(
        g_galleryAssets.end(),
        plainOtherAssets.begin(),
        plainOtherAssets.end()
    );

    if (g_galleryAssets.empty()) {
        fprintf(
            stderr,
            "ERROR: No renderable Object3D display instances were loaded.\n"
        );
        return false;
    }

    g_gallerySelectedAsset = requestedAssetIndex;
    if (g_gallerySelectedAsset >= (int)(g_galleryAssets.size())) {
        g_gallerySelectedAsset = 0;
    }

    int texturedAssetCount = 0;
    for (size_t i = 0; i < g_galleryAssets.size(); ++i) {
        if (g_galleryAssets[i].validTexturedMaterialCount > 0) {
            ++texturedAssetCount;
        }
    }

    const GalleryAsset &asset = g_galleryAssets[g_gallerySelectedAsset];
    printf(
        "Gallery assets: total=%u textured=%d selected=%d nodeIndex=%d name=%s mode=%d entries=%d verts=%d texturedMaterials=%d validTexturedMaterials=%d missingTextures=%d zeroHandles=%d defaultTextures=%d missingUvs=%d radius=%.3f.\n",
        (unsigned int)(g_galleryAssets.size()),
        texturedAssetCount,
        g_gallerySelectedAsset,
        asset.nodeIndex,
        asset.node->name,
        asset.displayInstance->mode,
        asset.displayInstance->entryCount,
        asset.displayInstance->vertCount,
        asset.texturedMaterialCount,
        asset.validTexturedMaterialCount,
        asset.missingTextureCount,
        asset.zeroHandleTextureCount,
        asset.defaultTextureCount,
        asset.missingUvCount,
        asset.displayInstance->bboxRadius
    );
    fflush(stdout);
    PrintGalleryTextureDiagnostics(asset);
    return true;
}

void SetupGalleryViewContext(
    const GalleryAsset &asset
) {
    const int width = g_zVideo_PrimarySurfaceState.width > 0
                          ? g_zVideo_PrimarySurfaceState.width
                          : g_zVideo_DisplayModeSurfaceState.width;
    const int height = g_zVideo_PrimarySurfaceState.height > 0
                           ? g_zVideo_PrimarySurfaceState.height
                           : g_zVideo_DisplayModeSurfaceState.height;

    memset(
        &g_galleryWindowNode,
        0,
        sizeof(g_galleryWindowNode)
    );
    memset(
        &g_galleryWindowData,
        0,
        sizeof(g_galleryWindowData)
    );
    memset(
        &g_galleryCameraData,
        0,
        sizeof(g_galleryCameraData)
    );

    g_galleryWindowNode.classId = kZClassNodeWindow;
    g_galleryWindowNode.classData = &g_galleryWindowData;
    g_galleryWindowData.viewportWidth = width;
    g_galleryWindowData.viewportHeight = height;
    g_galleryWindowData.resolutionWidth = width;
    g_galleryWindowData.resolutionHeight = height;

    const float radius = asset.displayInstance->bboxRadius > 1.0f
                             ? asset.displayInstance->bboxRadius
                             : 1.0f;
    const float depth = radius * 2.4f + 6.0f;
    zMat4x3 cameraWorld = {};
    cameraWorld.xx = 1.0f;
    cameraWorld.yy = 1.0f;
    cameraWorld.zz = 1.0f;
    cameraWorld.posX = 0.0f;
    cameraWorld.posY = 0.0f;
    cameraWorld.posZ = 0.0f;

    g_galleryCameraData.windowNode = &g_galleryWindowNode;
    g_galleryCameraData.cameraPos = zVec3_Make(
        0.0f,
        0.0f,
        0.0f
    );
    g_galleryCameraData.forwardDir = zVec3_Make(
        0.0f,
        0.0f,
        -1.0f
    );
    memcpy(
        g_galleryCameraData.worldTransform,
        &cameraWorld,
        sizeof(cameraWorld)
    );
    g_galleryCameraData.nearClip = 1.0f;
    g_galleryCameraData.farClip = depth + radius * 4.0f + 100.0f;
    g_galleryCameraData.viewportWidth = 1.0f;
    g_galleryCameraData.viewportHeight = 1.0f;
    g_galleryCameraData.frustumWidth = 1.0f;
    g_galleryCameraData.frustumHeight = 0.75f;
    g_galleryCameraData.fovX = 1.0f;
    g_galleryCameraData.fovY = 0.75f;
    g_galleryCameraData.frustumYaw = 0.5f;
    g_galleryCameraData.frustumPitch = 0.375f;
    g_galleryCameraData.viewportScaleX = 1.0f / (float)(tan(0.5));
    g_galleryCameraData.viewportScaleY = 1.0f / (float)(tan(0.375));

    zMath_Camera_StageInverseRotation((zMat4x3 *)(g_galleryCameraData.worldTransform));
    zVideo_SetActiveViewContext(&g_galleryCameraData);
}

unsigned int DiagnosticFallbackColor16(
    int entryIndex
) {
    static const unsigned int colors[] = {
        0xf800,
        0x07e0,
        0x001f,
        0xffe0,
        0xf81f,
        0x07ff
    };
    return colors[entryIndex % (int)(sizeof(colors) / sizeof(colors[0]))];
}

float DegreesToRadians(
    int degrees
) {
    return (float)(degrees) * 0.017453292519943295f;
}

void PrintGalleryTextureDiagnostics(
    const GalleryAsset &asset
) {
    printf(
        "Selected texture diagnostics: texturedMaterials=%d valid=%d missingTextures=%d zeroHandles=%d defaultOrDummy=%d missingUvs=%d.\n",
        asset.texturedMaterialCount,
        asset.validTexturedMaterialCount,
        asset.missingTextureCount,
        asset.zeroHandleTextureCount,
        asset.defaultTextureCount,
        asset.missingUvCount
    );

    int printed = 0;
    for (int entryIndex = 0;
        entryIndex < asset.displayInstance->entryCount && printed < 8;
        ++entryIndex) {
        zDiEntryPartial *const entry = &asset.displayInstance->entries[entryIndex];
        zModel_MaterialPartial *const material = entry->material;
        if (material == 0 || (material->flags & 0x0100) == 0) {
            continue;
        }

        zImage_TexDirEntryPartial *const texDirEntry = material->currentTextureDirectoryEntry;
        zVideo_TextureRecordPartial *const texture =
            texDirEntry != 0 ? texDirEntry->texture : 0;
        printf(
            "  mat[%d]: tex=%s uv=%p record=%p handle=%lu blend=%d addr=%d/%d valid=%d.\n",
            entryIndex,
            texDirEntry != 0 ? texDirEntry->baseName : "<missing>",
            entry->uvPairs,
            (void *)(texture),
            texture != 0 ? (unsigned long)(texture->m_textureHandle) : 0,
            texture != 0 ? texture->m_alphaMode : 0,
            texture != 0 ? texture->m_uWrapMode : 0,
            texture != 0 ? texture->m_vWrapMode : 0,
            IsValidMaterialTexture(
                material,
                entry
            ) != 0
                ? 1
                : 0
        );
        ++printed;
    }
    fflush(stdout);
}

void PrintSelectedGalleryAssetDiagnostics(
    const char *label
) {
    if (g_galleryAssets.empty()) {
        return;
    }

    if (g_gallerySelectedAsset < 0) {
        g_gallerySelectedAsset = 0;
    } else if (g_gallerySelectedAsset >= (int)(g_galleryAssets.size())) {
        g_gallerySelectedAsset = (int)(g_galleryAssets.size()) - 1;
    }

    const GalleryAsset &asset = g_galleryAssets[g_gallerySelectedAsset];
    printf(
        "%s: selected=%d nodeIndex=%d name=%s mode=%d entries=%d verts=%d texturedMaterials=%d validTexturedMaterials=%d missingTextures=%d zeroHandles=%d defaultTextures=%d missingUvs=%d radius=%.3f.\n",
        label,
        g_gallerySelectedAsset,
        asset.nodeIndex,
        asset.node->name,
        asset.displayInstance->mode,
        asset.displayInstance->entryCount,
        asset.displayInstance->vertCount,
        asset.texturedMaterialCount,
        asset.validTexturedMaterialCount,
        asset.missingTextureCount,
        asset.zeroHandleTextureCount,
        asset.defaultTextureCount,
        asset.missingUvCount,
        asset.displayInstance->bboxRadius
    );
    fflush(stdout);
    PrintGalleryTextureDiagnostics(asset);
}

void DrawGalleryTextureSwatches(
    const GalleryAsset &asset,
    float targetHeight
) {
    std::vector<zVideo_TextureRecordPartial *> textures;
    for (int entryIndex = 0; entryIndex < asset.displayInstance->entryCount; ++entryIndex) {
        zDiEntryPartial *const entry = &asset.displayInstance->entries[entryIndex];
        zModel_MaterialPartial *const material = entry->material;
        if (!IsValidMaterialTexture(
                material,
                entry
            )) {
            continue;
        }

        zVideo_TextureRecordPartial *const texture =
            material->currentTextureDirectoryEntry->texture;
        if (std::find(
                textures.begin(),
                textures.end(),
                texture
            ) == textures.end()) {
            textures.push_back(texture);
            if (textures.size() >= 4) {
                break;
            }
        }
    }

    const float swatchSize = 36.0f;
    const float margin = 8.0f;
    for (size_t index = 0; index < textures.size(); ++index) {
        const float left = margin + (float)(index) * (swatchSize + 6.0f);
        const float top = targetHeight - swatchSize - margin;
        zVideo_XyzVertex vertices[4] = {};
        zVideo_TexCoord texCoords[4] = {};
        vertices[0].x = left;
        vertices[0].y = top;
        vertices[0].z = 1.0f;
        vertices[1].x = left + swatchSize;
        vertices[1].y = top;
        vertices[1].z = 1.0f;
        vertices[2].x = left + swatchSize;
        vertices[2].y = top + swatchSize;
        vertices[2].z = 1.0f;
        vertices[3].x = left;
        vertices[3].y = top + swatchSize;
        vertices[3].z = 1.0f;

        texCoords[0].u = 0.0f;
        texCoords[0].v = 0.0f;
        texCoords[1].u = 1.0f;
        texCoords[1].v = 0.0f;
        texCoords[2].u = 1.0f;
        texCoords[2].v = 1.0f;
        texCoords[3].u = 0.0f;
        texCoords[3].v = 1.0f;

        ((ViewerSubmitRenderClassProc)g_zVideo_pfnSubmitPolyRenderClass)(
            vertices,
            texCoords,
            4,
            (zVideo_RenderClass *)(textures[index]),
            0,
            1.0f,
            0
        );
    }
}

void RenderGalleryAsset(
    const GalleryAsset &asset,
    const ViewerOptions &options,
    int textureSwatches
) {
    SetupGalleryViewContext(asset);

    const float radius = asset.displayInstance->bboxRadius > 1.0f
                             ? asset.displayInstance->bboxRadius
                             : 1.0f;
    const float depth = radius * 2.4f + 6.0f;
    const float yaw =
        DegreesToRadians(options.assetYawDeg) + (float)(g_viewer.frameCount) * 0.02f;
    const float sinYaw = (float)(sin(yaw));
    const float cosYaw = (float)(cos(yaw));
    const float targetWidth = (float)(g_zVideo_SwSurfaceState.width);
    const float targetHeight = (float)(g_zVideo_SwSurfaceState.height);
    const float centerX = targetWidth * 0.5f;
    const float centerY = targetHeight * 0.5f;
    const float focalScale =
        targetHeight * 1.8f;

    g_zClass_LodDistanceStateStackTop = 0;
    g_zClass_RenderBoundsContextActive = 1;
    g_zClass_RenderVertexAlphaOverrideActive = 0;
    g_zClass_RenderAlphaScaleStackTop = -1;
    g_zClass_SoftwarePathStateStackTop = -1;
    zModel_RenderAlphaScale_SetCurrent(1.0f);
    zModel_RenderVertexAlphaEnabled_SetCurrent(0);
    gModel_HasActiveLights = 0;
    gModel_ActiveLightCount = 0;
    zModel_Fog_SetEnabled(0);

    g_viewer.phase = "gallery SceneEnter";
    zVideoD3D::SceneEnter();
    g_viewer.phase = "gallery direct mesh submit";
    int submittedPolygons = 0;
    int invalidPolygons = 0;
    for (int entryIndex = 0; entryIndex < asset.displayInstance->entryCount; ++entryIndex) {
        if (options.faceIndex >= 0 && entryIndex != options.faceIndex) {
            continue;
        }

        zDiEntryPartial *const entry = &asset.displayInstance->entries[entryIndex];
        zModel_MaterialPartial *const material = entry->material;
        const int vertexCount = (int)(entry->flagsAndIndexCount & 0xff);
        if (material == 0 || vertexCount < 3 || vertexCount > 64 ||
            entry->vertexIndices == 0) {
            continue;
        }

        zVideo_XyzVertex vertices[64] = {};
        zVideo_TexCoord texCoords[64] = {};
        int projectedCount = 0;
        int *const vertexIndices = (int *)(entry->vertexIndices);
        zVideo_TexCoord *const entryUvs = (zVideo_TexCoord *)(entry->uvPairs);
        for (int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
            const int sourceIndex = vertexIndices[vertexIndex];
            if (sourceIndex < 0 || sourceIndex >= asset.displayInstance->vertCount) {
                projectedCount = 0;
                break;
            }

            const zVec3 &source = asset.displayInstance->verts[sourceIndex];
            const float localX = source.x - asset.displayInstance->bboxCenter.x;
            const float localY = source.y - asset.displayInstance->bboxCenter.y;
            const float localZ = source.z - asset.displayInstance->bboxCenter.z;
            const float rotatedX = localX * cosYaw + localZ * sinYaw;
            const float rotatedZ = localZ * cosYaw - localX * sinYaw;
            const float viewX = -rotatedX;
            const float viewY = -localY;
            const float viewZ = rotatedZ + depth;
            if (viewZ <= 1.0f) {
                projectedCount = 0;
                break;
            }

            vertices[projectedCount].x = centerX + viewX * focalScale / viewZ;
            vertices[projectedCount].y = centerY + viewY * focalScale / viewZ;
            vertices[projectedCount].z = 1.0f / viewZ;
            if (entryUvs != 0) {
                texCoords[projectedCount] = entryUvs[vertexIndex];
            } else {
                texCoords[projectedCount].u = 0.0f;
                texCoords[projectedCount].v = 0.0f;
            }
            ++projectedCount;
        }

        if (projectedCount < 3) {
            ++invalidPolygons;
            continue;
        }

        if (IsValidMaterialTexture(
                material,
                entry
            )) {
            zVideo_RenderClass *const renderClass =
                (zVideo_RenderClass *)(material->currentTextureDirectoryEntry->texture);
            ((ViewerSubmitRenderClassProc)g_zVideo_pfnSubmitPolyRenderClass)(
                vertices,
                texCoords,
                projectedCount,
                renderClass,
                entry->drawFlags,
                1.0f,
                0
            );
            ++submittedPolygons;
        } else {
            const int declaredTextured = (material->flags & 0x0100) != 0;
            ((ViewerSubmitFlatColor16Proc)g_zVideo_pfnSubmitPolyFlatColor16)(
                vertices,
                declaredTextured != 0 ? DiagnosticFallbackColor16(entryIndex)
                                      : material->packedColor,
                255,
                entry->drawFlags,
                projectedCount,
                0
            );
            ++submittedPolygons;
        }
    }
    if (g_viewer.frameCount == 0) {
        printf(
            "Gallery polygons: submitted=%d invalid=%d yawDeg=%d faceIndex=%d.\n",
            submittedPolygons,
            invalidPolygons,
            options.assetYawDeg,
            options.faceIndex
        );
        fflush(stdout);
    }
    if (textureSwatches != 0) {
        DrawGalleryTextureSwatches(
            asset,
            targetHeight
        );
    }
    g_viewer.phase = "gallery FlushSortedPolys";
    ((ViewerVideoFlushProc)g_zVideo_pfnFlushSortedPolys)();
    g_viewer.phase = "gallery FlushOverwritePolys";
    ((ViewerVideoFlushProc)g_zVideo_pfnFlushOverwritePolys)();
    g_viewer.phase = "gallery FlushQuadBatch";
    ((ViewerVideoFlushProc)g_zVideo_pfnFlushQuadBatch)();
    g_viewer.phase = "gallery SceneLeave";
    zVideoD3D::SceneLeave();

    g_zClass_RenderBoundsContextActive = 0;
}

unsigned int ConvertRgb565ToRgb888(
    unsigned short value
) {
    const unsigned int r5 = (value >> 11) & 0x1f;
    const unsigned int g6 = (value >> 5) & 0x3f;
    const unsigned int b5 = value & 0x1f;
    const unsigned int r8 = (r5 << 3) | (r5 >> 2);
    const unsigned int g8 = (g6 << 2) | (g6 >> 4);
    const unsigned int b8 = (b5 << 3) | (b5 >> 2);
    return (r8 << 16) | (g8 << 8) | b8;
}

void CountSampledSurfacePixels(
    zVideo_SurfaceStatePartial *surfaceState,
    int *outNonClearPixels,
    int *outUniqueColors,
    int *outNonWhitePixels
) {
    *outNonClearPixels = 0;
    *outUniqueColors = 0;
    *outNonWhitePixels = 0;

    if (surfaceState->pixels == 0 || surfaceState->pitch <= 0 ||
        surfaceState->width <= 0 || surfaceState->height <= 0) {
        return;
    }

    const unsigned short clearColor = (unsigned short)(g_zVideo_ClearColorPacked16);
    unsigned short uniqueColors[64] = {};
    int uniqueCount = 0;
    const int xStep = surfaceState->width >= 32 ? surfaceState->width / 32 : 1;
    const int yStep = surfaceState->height >= 24 ? surfaceState->height / 24 : 1;

    for (int y = 0; y < surfaceState->height; y += yStep) {
        unsigned char *const rowBytes =
            (unsigned char *)(surfaceState->pixels) + y * surfaceState->pitch;
        unsigned short *const rowPixels = (unsigned short *)(rowBytes);
        for (int x = 0; x < surfaceState->width; x += xStep) {
            const unsigned short pixel = rowPixels[x];
            if (pixel != clearColor) {
                ++*outNonClearPixels;
                const unsigned int rgb = ConvertRgb565ToRgb888(pixel);
                const unsigned int red = (rgb >> 16) & 0xff;
                const unsigned int green = (rgb >> 8) & 0xff;
                const unsigned int blue = rgb & 0xff;
                const unsigned int maxChannel =
                    red > green ? (red > blue ? red : blue) : (green > blue ? green : blue);
                const unsigned int minChannel =
                    red < green ? (red < blue ? red : blue) : (green < blue ? green : blue);
                if (!(red > 216 && green > 216 && blue > 216 &&
                        maxChannel - minChannel < 24)) {
                    ++*outNonWhitePixels;
                }
            }

            int known = 0;
            for (int i = 0; i < uniqueCount; ++i) {
                if (uniqueColors[i] == pixel) {
                    known = 1;
                    break;
                }
            }
            if (known == 0 && uniqueCount < (int)(sizeof(uniqueColors) / sizeof(uniqueColors[0]))) {
                uniqueColors[uniqueCount++] = pixel;
            }
        }
    }

    *outUniqueColors = uniqueCount;
}

bool LockBestReadbackSurface(
    zVideo_SurfaceStatePartial **outSurfaceState
) {
    *outSurfaceState = &g_zVideo_DisplayModeSurfaceState;
    if (g_zVideo_pfnLockSurfaceState(*outSurfaceState) == 0) {
        return true;
    }

    *outSurfaceState = &g_zVideo_PrimarySurfaceState;
    if (g_zVideo_pfnLockSurfaceState(*outSurfaceState) == 0) {
        return true;
    }

    *outSurfaceState = &g_zVideo_SwSurfaceState;
    if (g_zVideo_pfnLockSurfaceState(*outSurfaceState) == 0) {
        return true;
    }

    *outSurfaceState = 0;
    return false;
}

void UnlockReadbackSurface(
    zVideo_SurfaceStatePartial *surfaceState
) {
    if (surfaceState != 0) {
        g_zVideo_pfnUnlockSurfaceState(surfaceState);
    }
}

bool WriteSurfaceBmp(
    const std::filesystem::path &path
) {
    zVideo_SurfaceStatePartial *surfaceState = 0;
    if (!LockBestReadbackSurface(&surfaceState)) {
        fprintf(
            stderr,
            "WARNING: Could not lock a surface for BMP dump.\n"
        );
        return false;
    }

    const int width = surfaceState->width;
    const int height = surfaceState->height;
    const int rowBytes = ((width * 3 + 3) / 4) * 4;
    const int pixelBytes = rowBytes * height;
    const int fileBytes = 14 + 40 + pixelBytes;

    FILE *file = fopen(
        path.string().c_str(),
        "wb"
    );
    if (file == 0) {
        UnlockReadbackSurface(surfaceState);
        fprintf(
            stderr,
            "WARNING: Could not open BMP dump path: %s.\n",
            path.string().c_str()
        );
        return false;
    }

    unsigned char fileHeader[14] = {};
    unsigned char infoHeader[40] = {};
    fileHeader[0] = 'B';
    fileHeader[1] = 'M';
    memcpy(
        &fileHeader[2],
        &fileBytes,
        sizeof(fileBytes)
    );
    const int pixelOffset = 14 + 40;
    memcpy(
        &fileHeader[10],
        &pixelOffset,
        sizeof(pixelOffset)
    );

    const int infoBytes = 40;
    const short planes = 1;
    const short bitsPerPixel = 24;
    memcpy(
        &infoHeader[0],
        &infoBytes,
        sizeof(infoBytes)
    );
    memcpy(
        &infoHeader[4],
        &width,
        sizeof(width)
    );
    memcpy(
        &infoHeader[8],
        &height,
        sizeof(height)
    );
    memcpy(
        &infoHeader[12],
        &planes,
        sizeof(planes)
    );
    memcpy(
        &infoHeader[14],
        &bitsPerPixel,
        sizeof(bitsPerPixel)
    );

    fwrite(
        fileHeader,
        1,
        sizeof(fileHeader),
        file
    );
    fwrite(
        infoHeader,
        1,
        sizeof(infoHeader),
        file
    );

    std::vector<unsigned char> row((size_t)(rowBytes));
    for (int y = height - 1; y >= 0; --y) {
        memset(
            row.data(),
            0,
            row.size()
        );
        unsigned char *const srcBytes =
            (unsigned char *)(surfaceState->pixels) + y * surfaceState->pitch;
        unsigned short *const srcPixels = (unsigned short *)(srcBytes);
        for (int x = 0; x < width; ++x) {
            const unsigned int rgb = ConvertRgb565ToRgb888(srcPixels[x]);
            row[(size_t)(x * 3)] = (unsigned char)(rgb & 0xff);
            row[(size_t)(x * 3 + 1)] = (unsigned char)((rgb >> 8) & 0xff);
            row[(size_t)(x * 3 + 2)] = (unsigned char)((rgb >> 16) & 0xff);
        }
        fwrite(
            row.data(),
            1,
            row.size(),
            file
        );
    }

    fclose(file);
    UnlockReadbackSurface(surfaceState);
    printf(
        "Frame dump: %s\n",
        path.string().c_str()
    );
    fflush(stdout);
    return true;
}

void SamplePresentedFrame() {
    zVideo_SurfaceStatePartial *surfaceState = 0;
    if (!LockBestReadbackSurface(&surfaceState)) {
        g_viewer.lastFrameNonClearPixels = 0;
        g_viewer.lastFrameUniqueColors = 0;
        g_viewer.lastFrameNonWhitePixels = 0;
        fprintf(
            stderr,
            "WARNING: Could not lock a surface for frame readback.\n"
        );
        return;
    }

    CountSampledSurfacePixels(
        surfaceState,
        &g_viewer.lastFrameNonClearPixels,
        &g_viewer.lastFrameUniqueColors,
        &g_viewer.lastFrameNonWhitePixels
    );
    UnlockReadbackSurface(surfaceState);

    printf(
        "Frame sample: nonClearPixels=%d nonWhitePixels=%d uniqueColors=%d clearColor=0x%04x.\n",
        g_viewer.lastFrameNonClearPixels,
        g_viewer.lastFrameNonWhitePixels,
        g_viewer.lastFrameUniqueColors,
        (unsigned int)((unsigned short)(g_zVideo_ClearColorPacked16))
    );
    fflush(stdout);
}

void NormalizeTextureEntriesForStandaloneLoad() {
    int normalizedCount = 0;
    for (int i = 0; i < g_zImage_TexDirEntryCount; ++i) {
        zImage_TexDirEntryPartial *const entry = &g_zImage_TexDirEntries[i];
        if (entry->texture == 0) {
            entry->image = 0;
            entry->loadState = 2;
            ++normalizedCount;
        }
    }

    if (normalizedCount != 0) {
        printf(
            "Normalized %d texture entries to standalone create-pending state.\n",
            normalizedCount
        );
        fflush(stdout);
    }
}

zClass_NodePartial *FindFirstCamera() {
    zClass_TypeListLink *const head = zClass_TypeList::Head(8);
    PrintNodeArraySummary();
    printf(
        "Camera bucket head: %p.\n",
        (void *)(head)
    );
    fflush(stdout);
    if (head == 0) {
        return 0;
    }

    for (zClass_TypeListLink *link = head; link != 0; link = link->next) {
        printf(
            "Camera candidate link=%p node=%p index=%d.\n",
            (void *)(link),
            (void *)(link->node),
            LoadedNodeIndex(link->node)
        );
        fflush(stdout);
        if (IsUsableRenderCamera(link->node) && (link->node->flags & 4) != 0) {
            return link->node;
        }
    }

    for (int i = 0; i < g_zClass_NodeArraySize; ++i) {
        zClass_NodePartial *const node = &g_zClass_NodeArray[i].node;
        if (IsUsableRenderCamera(node)) {
            node->flags |= 4;
            return node;
        }
    }

    return 0;
}

void CycleActiveCamera() {
    zClass_NodePartial *first = 0;
    zClass_NodePartial *active = 0;
    zClass_NodePartial *afterActive = 0;

    for (int i = 0; i < g_zClass_NodeArraySize; ++i) {
        zClass_NodePartial *const node = &g_zClass_NodeArray[i].node;
        if (!IsUsableRenderCamera(node)) {
            continue;
        }

        if (first == 0) {
            first = node;
        }
        if (active != 0 && afterActive == 0) {
            afterActive = node;
        }
        if ((node->flags & 4) != 0) {
            active = node;
        }
    }

    zClass_NodePartial *const next = afterActive != 0 ? afterActive : first;
    if (next == 0) {
        fprintf(
            stderr,
            "ERROR: No usable camera nodes are loaded.\n"
        );
        return;
    }

    if (active != 0) {
        active->flags &= ~4;
    }
    next->flags |= 4;
    printf(
        "Active camera: %s\n",
        next->name
    );
    fflush(stdout);
}

int RenderCameraBruteForceForViewer(
    zClass_NodePartial *camera
) {
    zClass_CameraDataPartial *const cameraData = (zClass_CameraDataPartial *)(camera->classData);
    zClass_NodePartial *const world = cameraData->worldNode;

    zMat4x3 slotBuffer = {};
    zMath::MatStackPushPtr((float *)(&slotBuffer));

    g_zVideo_pActiveViewContext = cameraData;
    g_viewer.phase = "viewer gwCameraUpdate";
    zClass_Camera::gwCameraUpdate(camera);
    g_viewer.phase = "viewer SyncViewContextPositions";
    zClass_Camera::SyncViewContextPositions();
    g_viewer.phase = "viewer zVideo_SetActiveViewContext";
    zVideo_SetActiveViewContext(g_zVideo_pActiveViewContext);
    g_viewer.phase = "viewer UpdateAllLights";
    zClass_World::UpdateAllLights(world);

    g_zClass_LodDistanceStateStackTop = 0;
    g_viewer.phase = "viewer SceneEnter";
    zVideoD3D::SceneEnter();
    g_viewer.phase = "viewer RenderOverlayNodes";
    zClass_Camera::RenderOverlayNodes(world);
    zMath::MatStackPopPtr();

    g_viewer.phase = "viewer FlushSortedPolys";
    ((ViewerVideoFlushProc)g_zVideo_pfnFlushSortedPolys)();
    g_viewer.phase = "viewer FlushOverwritePolys";
    ((ViewerVideoFlushProc)g_zVideo_pfnFlushOverwritePolys)();
    g_viewer.phase = "viewer FlushQuadBatch";
    ((ViewerVideoFlushProc)g_zVideo_pfnFlushQuadBatch)();
    g_viewer.phase = "viewer SceneLeave";
    zVideoD3D::SceneLeave();
    return 0;
}

int RenderActiveLoadedCameras() {
    int renderedCount = 0;
    for (int i = 0; i < g_zClass_NodeArraySize; ++i) {
        zClass_NodePartial *const camera = &g_zClass_NodeArray[i].node;
        if (!IsUsableRenderCamera(camera) || (camera->flags & 4) == 0) {
            continue;
        }

        if (g_zVideo_ActiveRendererPath != 0) {
            if (g_viewer.frameCount == 0) {
                printf(
                    "Rendering active camera index=%d name=%s.\n",
                    i,
                    camera->name
                );
                fflush(stdout);
            }
            g_viewer.phase = "zVideo_sw_RenderFrame";
            RenderCameraBruteForceForViewer(camera);
        } else {
            zClass_List::RenderActiveCameras();
        }
        ++renderedCount;
    }

    return renderedCount;
}

bool LoadScene(
    const char *zbdPath,
    const ViewerOptions &options
) {
    ResetSceneGlobals();
    if (zModel_Display_Init() != 0) {
        fprintf(
            stderr,
            "ERROR: zModel_Display_Init failed.\n"
        );
        return false;
    }
    ConfigureTextureSearchPaths(zbdPath);

    zClass_ZbdHeader header = {};
    FILE *headerFile = GameZ::OpenAndReadZBDHeader(
        zbdPath,
        &header
    );
    if (headerFile == 0) {
        fprintf(
            stderr,
            "ERROR: Failed to open ZBD header: %s\n",
            zbdPath
        );
        return false;
    }
    fclose(headerFile);

    printf(
        "ZBD: %s\n",
        zbdPath
    );
    printf(
        "Header: version=%d textures=%d nodes=%d nodeFreeHead=%d\n",
        header.version,
        header.texDirArg,
        header.nodeCount,
        header.nodeFreeHead
    );

    if (GameZ::ReadZBDFile(zbdPath) != 0) {
        fprintf(
            stderr,
            "ERROR: GameZ::ReadZBDFile failed for %s.\n",
            zbdPath
        );
        return false;
    }

    printf("GameZ ZBD read complete.\n");
    fflush(stdout);
    NormalizeTextureEntriesForStandaloneLoad();
    printf(
        "Loading pending texture entries: count=%d.\n",
        g_zImage_TexDirEntryCount
    );
    fflush(stdout);
    if (zImage::TexDir_LoadPendingEntries() != 0) {
        fprintf(
            stderr,
            "ERROR: zImage::TexDir_LoadPendingEntries failed.\n"
        );
        return false;
    }
    printf("Pending texture entries loaded.\n");
    fflush(stdout);
    PrintTexturePackSummary();
    PrintTextureRecordSummary();
    RebuildTypeListsFromLoadedNodes();
    g_Variant_FilterEnabled = 0;
    memset(
        &g_Variant_CurrentTag,
        0,
        sizeof(g_Variant_CurrentTag)
    );
    memset(
        &g_zVideo_ActiveViewVariantTag,
        0,
        sizeof(g_zVideo_ActiveViewVariantTag)
    );
    printf(
        "Viewer variant pick filter disabled for standalone render smoke.\n"
    );
    fflush(stdout);

    if (options.renderMode == kViewerRenderGallery) {
        if (!BuildGalleryAssetList(options.assetIndex)) {
            return false;
        }
        g_viewer.galleryReady = true;
    }

    zClass_NodePartial *const camera =
        options.renderMode == kViewerRenderMission ? FindFirstCamera() : 0;
    if (camera == 0 && options.renderMode == kViewerRenderMission) {
        fprintf(
            stderr,
            "ERROR: Loaded scene has no camera nodes.\n"
        );
        return false;
    }

    printf(
        "Loaded: activeNodes=%d nodeArray=%d textures=%d materials=%d/%d displayInstances=%d/%d\n",
        g_zClass_ActiveNodeCount,
        g_zClass_NodeArraySize,
        g_zImage_TexDirEntryCount,
        g_zModel_MatlPoolInUseCount,
        g_zModel_MatlPoolCapacity,
        g_zModel_DiPoolInUseCount,
        g_zModel_DiPoolCapacity
    );
    if (camera != 0) {
        printf(
            "Active camera: %s\n",
            camera->name
        );
    } else if (options.renderMode == kViewerRenderGallery) {
        printf(
            "Mission camera lookup skipped for gallery view.\n"
        );
    }
    fflush(stdout);

    g_viewer.sceneLoaded = true;
    return true;
}

int RenderGalleryFrame(
    const ViewerOptions &options
) {
    if (!g_viewer.galleryReady || g_galleryAssets.empty()) {
        fprintf(
            stderr,
            "ERROR: Gallery asset list is empty.\n"
        );
        return 0;
    }

    if (g_gallerySelectedAsset < 0 ||
        g_gallerySelectedAsset >= (int)(g_galleryAssets.size())) {
        g_gallerySelectedAsset = 0;
    }

    const GalleryAsset &asset = g_galleryAssets[g_gallerySelectedAsset];
    if (g_viewer.frameCount == 0) {
        printf(
            "Rendering gallery asset index=%d nodeIndex=%d name=%s.\n",
            g_gallerySelectedAsset,
            asset.nodeIndex,
            asset.node->name
        );
        fflush(stdout);
    }

    RenderGalleryAsset(
        asset,
        options,
        options.textureSwatches
    );
    return 1;
}

bool ClearAndRenderFrame(
    const ViewerOptions &options
) {
    zVidRect32 rect = {
        0,
        0,
        g_zVideo_DisplayModeSurfaceState.width,
        g_zVideo_DisplayModeSurfaceState.height
    };
    zVidRect32 renderRect = {
        0,
        0,
        g_zVideo_SwSurfaceState.width,
        g_zVideo_SwSurfaceState.height
    };

    g_viewer.phase = "CallClearPrimarySurfaceAndZBuffer";
    if (g_viewer.frameCount == 0) {
        printf("Frame 0: clearing surfaces.\n");
        fflush(stdout);
    }
    if (options.renderMode == kViewerRenderGallery) {
        zVideo::CallClearSwSurfaceAndZBuffer(
            &renderRect,
            &renderRect
        );
    } else {
        zVideo::CallClearPrimarySurfaceAndZBuffer(&rect);
    }
    g_viewer.phase = "zClass_TypeList::UpdateAllBuckets";
    if (g_viewer.frameCount == 0) {
        printf("Frame 0: updating type-list buckets.\n");
        fflush(stdout);
    }
    zClass_TypeList::UpdateAllBuckets();
    ResetSubmitCounters();
    g_viewer.phase = options.renderMode == kViewerRenderGallery ? "RenderGalleryFrame"
                                                                : "RenderActiveLoadedCameras";
    if (g_viewer.frameCount == 0) {
        printf(
            options.renderMode == kViewerRenderGallery
                ? "Frame 0: rendering gallery asset.\n"
                : "Frame 0: rendering active loaded cameras.\n"
        );
        fflush(stdout);
    }
    const int renderedCount = options.renderMode == kViewerRenderGallery
                                  ? RenderGalleryFrame(options)
                                  : RenderActiveLoadedCameras();
    if (renderedCount == 0) {
        fprintf(
            stderr,
            "ERROR: Nothing rendered this frame.\n"
        );
        g_viewer.running = false;
        return false;
    }
    g_viewer.phase = "AdjustSurfacesIfEnabled";
    if (g_viewer.frameCount == 0) {
        printf("Frame 0: presenting surfaces.\n");
        fflush(stdout);
    }
    if (options.renderMode == kViewerRenderGallery) {
        g_zVideo_pfnBltSwToPrimaryRectDirect(
            &renderRect,
            &rect
        );
    }
    zVideo::AdjustSurfacesIfEnabled(
        &rect,
        &rect,
        1,
        0
    );

    if (options.renderMode == kViewerRenderGallery &&
        (g_viewer.frameCount == 0 || options.smokeFrames != 0)) {
        SamplePresentedFrame();
        printf(
            "Frame submits: textured=%d untextured=%d polygon=%d flushSorted=%d flushOverwrite=%d flushQuad=%d.\n",
            g_viewerSubmitCounters.texturedSubmitCalls,
            g_viewerSubmitCounters.untexturedSubmitCalls,
            g_viewerSubmitCounters.polygonSubmitCalls,
            g_viewerSubmitCounters.sortedFlushCalls,
            g_viewerSubmitCounters.overwriteFlushCalls,
            g_viewerSubmitCounters.quadFlushCalls
        );
        fflush(stdout);
        if (!options.dumpFramePath.empty() && g_viewer.frameCount == 0) {
            WriteSurfaceBmp(options.dumpFramePath);
        }
    }

    ++g_zVideo_FrameTick;
    ++g_viewer.frameCount;
    return true;
}

int RunViewer(
    const ViewerOptions &options
) {
    WarnIfRuntimeDllsMissing();
    SetCurrentDirectoryA(options.runtimeDir.string().c_str());
    printf(
        "Runtime dir: %s\n",
        options.runtimeDir.string().c_str()
    );

    HINSTANCE const instance = GetModuleHandleA(0);
    HWND const hwnd = CreateViewerWindow(
        instance,
        options.modeIndex
    );
    if (hwnd == 0) {
        fprintf(
            stderr,
            "ERROR: Failed to create viewer window.\n"
        );
        return 1;
    }

    g_viewer.window = hwnd;
    g_viewer.running = true;
    g_viewer.zbdPath = options.zbdPath;

    const int videoResult = InitVideoStandalone(
        hwnd,
        options.fullscreen,
        options.modeIndex
    );
    if (videoResult != 0) {
        return 2;
    }

    if (!LoadScene(
            g_viewer.zbdPath.c_str(),
            options
        )) {
        return 3;
    }

    MSG message = {};
    while (g_viewer.running) {
        while (PeekMessageA(
            &message,
            0,
            0,
            0,
            PM_REMOVE
        )) {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        if (g_viewer.reloadRequested) {
            g_viewer.reloadRequested = false;
            if (!LoadScene(
                    g_viewer.zbdPath.c_str(),
                    options
                )) {
                return 4;
            }
        }

        if (!ClearAndRenderFrame(options)) {
            return 5;
        }

        if (options.smokeFrames != 0 && g_viewer.frameCount >= options.smokeFrames) {
            int selectedValidTextures = 0;
            if (options.renderMode == kViewerRenderGallery && !g_galleryAssets.empty() &&
                g_gallerySelectedAsset >= 0 &&
                g_gallerySelectedAsset < (int)(g_galleryAssets.size())) {
                selectedValidTextures =
                    g_galleryAssets[g_gallerySelectedAsset].validTexturedMaterialCount;
            }
            if (options.renderMode == kViewerRenderGallery &&
                (selectedValidTextures <= 0 ||
                 g_viewerSubmitCounters.texturedSubmitCalls <= 0 ||
                 g_viewerSubmitCounters.texturedSubmitCalls +
                         g_viewerSubmitCounters.untexturedSubmitCalls +
                         g_viewerSubmitCounters.polygonSubmitCalls <=
                     0 ||
                 g_viewer.lastFrameNonClearPixels <= 0 ||
                 g_viewer.lastFrameNonWhitePixels <= 0 ||
                 g_viewer.lastFrameUniqueColors <= 2)) {
                fprintf(
                    stderr,
                    "ERROR: Gallery smoke did not produce a visibly textured, non-white frame.\n"
                );
                return 6;
            }
            g_viewer.running = false;
        }
    }

    return 0;
}

void ShutdownViewer() {
    if (g_viewer.sceneLoaded) {
        ResetSceneGlobals();
        g_viewer.sceneLoaded = false;
    }
    if (g_zVideo_IsInitialized != 0) {
        zVideo::ShutdownVideoSystem();
    }
    ShowCursor(TRUE);
}

} // namespace

int main(
    int argc,
    char **argv
) {
    SetUnhandledExceptionFilter(ViewerUnhandledExceptionFilter);

    if (argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        PrintUsage();
        return 0;
    }

    ViewerOptions options = {};
    if (!ParseOptions(
            argc,
            argv,
            &options
        )) {
        return 1;
    }

    const int result = RunViewer(options);
    ShutdownViewer();
    return result;
}
