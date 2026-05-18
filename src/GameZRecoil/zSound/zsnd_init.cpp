#include "zSound.h"

#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zSys/zSys.h"

#include "recoil/recoil_types.h"
#include <stdio.h>

extern "C" LPDIRECTSOUND g_zSnd_BackendDevice = 0;
extern "C" LPDIRECTSOUNDBUFFER g_zSnd_BackendListenerHandle = 0;
extern "C" DSCAPS g_zSnd_BackendAuxHandleOrConfig = {0};
extern "C" LPDIRECTSOUND g_zSnd_CachedDirectSound = 0;
extern "C" LPCGUID g_zSnd_CachedDirectSoundGuid = 0;
extern "C" int g_zSnd_PreInitialized = 0;
extern "C" int g_zSnd_SoundLodDefault = 0;
extern "C" void *g_zSnd_SoundLodValuePtr = 0;
extern "C" int g_zSnd_MuteOptionDefault = 0;
extern "C" void *g_zSnd_MuteOptionValuePtr = 0;
extern "C" int g_zSnd_MuteDepth = 0;
extern "C" float g_zSnd_VolumeScaleDefault = 0.0f;
extern "C" void *g_zSnd_GlobalVolumeScalePtr = 0;
extern "C" zSndSample *g_zSndLastSample = 0;
extern "C" zSndSample *g_zSndLastVoice = 0;
extern "C" zSndPlayHandle *g_zSndLastVoiceHandle = 0;
extern "C" int g_zSndLastVoiceMarkerIndex = 0;
extern "C" int g_zSndLastVoiceStopMarkerIndex = 999;
extern "C" int g_zSnd_Flag10PlaybackEnabled = 0;

namespace {
const GUID kCLSID_A3DApi = {
    0x92fa2c24, 0x253c, 0x11d2, {0x90, 0xfb, 0x00, 0x60, 0x08, 0xa1, 0xf4, 0x41}};
const GUID kIID_IA3d3 = {
    0xc398e560, 0xd90b, 0x11d1, {0x90, 0xfb, 0x00, 0x60, 0x08, 0xa1, 0xf4, 0x41}};
const GUID kIID_IA3dGeom = {
    0xc398e561, 0xd90b, 0x11d1, {0x90, 0xfb, 0x00, 0x60, 0x08, 0xa1, 0xf4, 0x41}};
const GUID kIID_IA3dListener = {
    0xc398e563, 0xd90b, 0x11d1, {0x90, 0xfb, 0x00, 0x60, 0x08, 0xa1, 0xf4, 0x41}};
const char *kZSndInitSourceFile = "D:\\Proj\\GameZRecoil\\zSound\\zsnd_init.cpp";

struct A3DApi;

typedef HRESULT(__stdcall *A3DQueryInterfaceProc)(A3DApi *, REFIID, void **);
typedef ULONG(__stdcall *A3DAddRefProc)(A3DApi *);
typedef ULONG(__stdcall *A3DReleaseProc)(A3DApi *);
typedef HRESULT(__stdcall *A3DSetOutputModeProc)(A3DApi *, int);
typedef HRESULT(__stdcall *A3DTickProc)(A3DApi *);
typedef HRESULT(__stdcall *A3DConfigureOutputProc)(A3DApi *, int, int, int);
typedef HRESULT(__stdcall *A3DCreateBufferByKindProc)(A3DApi *, int, void **);
typedef HRESULT(__stdcall *A3DSetCooperativeLevelProc)(A3DApi *, HWND, int);

struct A3DApiVtable {
    A3DQueryInterfaceProc QueryInterface;
    A3DAddRefProc AddRef;
    A3DReleaseProc Release;
    void *slot0c;
    void *slot10;
    A3DSetOutputModeProc SetOutputMode;
    void *slots18_2c[6];
    A3DTickProc Tick;
    void *slot34;
    void *slot38;
    A3DConfigureOutputProc ConfigureOutput;
    void *slot40;
    A3DCreateBufferByKindProc CreateBufferByKind;
    void *slot48;
    A3DSetCooperativeLevelProc SetCooperativeLevel;
};

struct A3DApi {
    A3DApiVtable *vtbl;
};

typedef ULONG(__stdcall *UnknownReleaseProc)(void *);

const char *DirectSoundErrorName(int directSoundError) {
    switch (static_cast<HRESULT>(directSoundError)) {
    case DS_OK:
        return 0;
    case DSERR_GENERIC:
        return "DSERR_GENERIC";
    case DSERR_UNSUPPORTED:
        return "DSERR_UNSUPPORTED";
    case DSERR_OUTOFMEMORY:
        return "DSERR_OUTOFMEMORY";
    case DSERR_NOAGGREGATION:
        return "DSERR_NOAGGREGATION";
    case DSERR_INVALIDPARAM:
        return "DSERR_INVALIDPARAM";
    case DSERR_ALLOCATED:
        return "DSERR_ALLOCATED";
    case DSERR_CONTROLUNAVAIL:
        return "DSERR_CONTROLUNAVAIL";
    case DSERR_INVALIDCALL:
        return "DSERR_INVALIDCALL";
    case DSERR_PRIOLEVELNEEDED:
        return "DSERR_PRIOLEVELNEEDED";
    case DSERR_BADFORMAT:
        return "DSERR_BADFORMAT";
    case DSERR_NODRIVER:
        return "DSERR_NODRIVER";
    case DSERR_ALREADYINITIALIZED:
        return "DSERR_ALREADYINITIALIZED";
    case DSERR_BUFFERLOST:
        return "DSERR_BUFFERLOST";
    case DSERR_OTHERAPPHASPRIO:
        return "DSERR_OTHERAPPHASPRIO";
    default:
        return "UNKNOWN";
    }
}

const char *A3DErrorName(int a3dError) {
    if (a3dError == 0) {
        return 0;
    }

    switch (static_cast<unsigned int>(a3dError)) {
    case 0x80040001u:
        return "\tA3DERROR_MEMORY_ALLOCATION\t";
    case 0x80040002u:
        return "\tA3DERROR_FAILED_CREATE_PRIMARY_BUFFER\t";
    case 0x80040003u:
        return "\tA3DERROR_FAILED_CREATE_SECONDARY_BUFFER\t";
    case 0x80040004u:
        return "\tA3DERROR_FAILED_INIT_A3D_DRIVER\t";
    case 0x80040005u:
        return "\tA3DERROR_FAILED_QUERY_DIRECTSOUND\t";
    case 0x80040006u:
        return "\tA3DERROR_FAILED_QUERY_A3D3\t";
    case 0x80040007u:
        return "\tA3DERROR_FAILED_INIT_A3D3\t";
    case 0x80040008u:
        return "\tA3DERROR_FAILED_QUERY_A3D2\t";
    case 0x80040009u:
        return "\tA3DERROR_FAILED_FILE_OPEN\t";
    case 0x8004000au:
        return "\tA3DERROR_FAILED_CREATE_SOUNDBUFFER\t";
    case 0x8004000bu:
        return "\tA3DERROR_FAILED_QUERY_3DINTERFACE\t";
    case 0x8004000cu:
        return "\tA3DERROR_FAILED_LOCK_BUFFER\t";
    case 0x8004000du:
        return "\tA3DERROR_FAILED_UNLOCK_BUFFER\t";
    case 0x8004000eu:
        return "\tA3DERROR_UNRECOGNIZED_FORMAT\t";
    case 0x8004000fu:
        return "\tA3DERROR_NO_WAVE_DATA\t";
    case 0x80040010u:
        return "\tA3DERROR_UNKNOWN_PLAYMODE\t";
    case 0x80040011u:
        return "\tA3DERROR_FAILED_PLAY\t";
    case 0x80040012u:
        return "\tA3DERROR_FAILED_STOP\t";
    case 0x80040013u:
        return "\tA3DERROR_NEEDS_FORMAT_INFORMATION\t";
    case 0x80040014u:
        return "\tA3DERROR_FAILED_ALLOCATE_WAVEDATA\t";
    case 0x80040015u:
        return "\tA3DERROR_NOT_VALID_SOURCE\t";
    case 0x80040016u:
        return "\tA3DERROR_FAILED_DUPLICATION\t";
    case 0x80040017u:
        return "\tA3DERROR_FAILED_INIT\t";
    case 0x80040018u:
        return "\tA3DERROR_FAILED_SETCOOPERATIVE_LEVEL\t";
    case 0x80040019u:
        return "\tA3DERROR_FAILED_INIT_QUERIED_INTERFACE\t";
    case 0x8004001au:
        return "\tA3DERROR_GEOMETRY_INPUT_OUTSIDE_BEGIN_END_BLOCK\t";
    case 0x8004001bu:
        return "\tA3DERROR_INVALID_NORMAL\t";
    case 0x8004001cu:
        return "\tA3DERROR_END_BEFORE_VALID_BEGIN_BLOCK\t";
    case 0x8004001du:
        return "\tA3DERROR_INVALID_BEGIN_MODE\t";
    case 0x8004001eu:
        return "\tA3DERROR_INVALID_ARGUMENT\t";
    case 0x8004001fu:
        return "\tA3DERROR_INVALID_INDEX\t";
    case 0x80040020u:
        return "\tA3DERROR_INVALID_VERTEX_INDEX\t";
    case 0x80040021u:
        return "\tA3DERROR_INVALID_PRIMITIVE_INDEX\t";
    case 0x80040022u:
        return "\tA3DERROR_MIXING_2D_AND_3D_MODES\t";
    case 0x80040023u:
        return "\tA3DERROR_2DWALL_REQUIRES_EXACTLY_ONE_LINE\t";
    case 0x80040024u:
        return "\tA3DERROR_NO_PRIMITIVES_DEFINED\t";
    case 0x80040025u:
        return "\tA3DERROR_PRIMITIVES_NON_PLANAR\t";
    case 0x80040026u:
        return "\tA3DERROR_PRIMITIVES_OVERLAPPING\t";
    case 0x80040027u:
        return "\tA3DERROR_PRIMITIVES_NOT_ADJACENT\t";
    case 0x80040028u:
        return "\tA3DERROR_OBJECT_NOT_FOUND\t";
    case 0x80040029u:
        return "\tA3DERROR_ROOM_HAS_NO_SHELL_WALLS\t";
    case 0x8004002au:
        return "\tA3DERROR_WALLS_DO_NOT_ENCLOSE_ROOM\t";
    case 0x8004002bu:
        return "\tA3DERROR_INVALID_WALL\t";
    case 0x8004002cu:
        return "\tA3DERROR_ROOM_HAS_LESS_THAN_4SHELL_WALLS\t";
    case 0x8004002du:
        return "\tA3DERROR_ROOM_HAS_LESS_THAN_3UNIQUE_NORMALS\t";
    case 0x8004002eu:
        return "\tA3DERROR_INTERSECTING_WALL_EDGES\t";
    case 0x8004002fu:
        return "\tA3DERROR_INVALID_ROOM\t";
    case 0x80040030u:
        return "\tA3DERROR_SCENE_HAS_ROOMS_INSIDE_ANOTHER_ROOMS\t";
    case 0x80040031u:
        return "\tA3DERROR_SCENE_HAS_OVERLAPPING_STATIC_ROOMS\t";
    case 0x80040032u:
        return "\tA3DERROR_DYNAMIC_OBJ_UNSUPPORTED\t";
    case 0x80040033u:
        return "\tA3DERROR_DIR_AND_UP_VECTORS_NOT_PERPENDICULAR\t";
    case 0x80040034u:
        return "\tA3DERROR_INVALID_ROOM_INDEX\t";
    case 0x80040035u:
        return "\tA3DERROR_INVALID_WALL_INDEX\t";
    case 0x80040036u:
        return "\tA3DERROR_SCENE_INVALID\t";
    case 0x80040037u:
        return "\tA3DERROR_UNIMPLEMENTED_FUNCTION\t";
    case 0x80040038u:
        return "\tA3DERROR_NO_ROOMS_IN_SCENE\t";
    case 0x80040039u:
        return "\tA3DERROR_2D_GEOMETRY_UNIMPLEMENTED\t";
    default:
        return "UNKNOWN";
    }
}
} // namespace

namespace zSnd {
// Reimplements 0x4a3ef0: zSnd::ReportA3DError
RECOIL_NOINLINE int RECOIL_FASTCALL ReportA3DError(int a3dError,
                                                            const char *sourceFile,
                                                            int sourceLine) {
    const char *errorName = A3DErrorName(a3dError);
    if (errorName == 0) {
        return 1;
    }

    char errorNameStorage[0x100];
    sprintf(errorNameStorage, errorName);
    zError::ReportOld(0x400, sourceFile, sourceLine, "A3D Error [%s]", errorNameStorage);
    return 0;
}

// Reimplements 0x4a4330: zSnd::ReportDirectSoundError
RECOIL_NOINLINE int RECOIL_FASTCALL ReportDirectSoundError(int directSoundError,
                                                                    const char *sourceFile,
                                                                    int sourceLine) {
    const char *errorName = DirectSoundErrorName(directSoundError);
    if (errorName == 0) {
        return 1;
    }

    char errorNameStorage[0x100];
    sprintf(errorNameStorage, errorName);
    zError::ReportOld(0x400, sourceFile, sourceLine, "DirectSound Error [%s]", errorNameStorage);
    return 0;
}

// Reimplements 0x4b31f0: zSnd::HasMmxMixerSupport
RECOIL_NOINLINE int RECOIL_CDECL HasMmxMixerSupport() {
    if (zSys::HasCpuidSupport() == 0) {
        return 0;
    }

    return zCpu::HasMmxSupport();
}

// Reimplements 0x4b2f50: zSnd::AcquireCachedDirectSound
RECOIL_NOINLINE LPDIRECTSOUND RECOIL_FASTCALL AcquireCachedDirectSound(LPCGUID deviceGuid) {
    LPDIRECTSOUND cached = g_zSnd_CachedDirectSound;
    if (cached != 0) {
        if (deviceGuid == g_zSnd_CachedDirectSoundGuid) {
            return cached;
        }

        cached->Release();
        g_zSnd_CachedDirectSound = 0;
    }

    if (DirectSoundCreate(deviceGuid, &g_zSnd_CachedDirectSound, 0) != DS_OK) {
        return 0;
    }

    g_zSnd_CachedDirectSoundGuid = deviceGuid;
    return g_zSnd_CachedDirectSound;
}

// Reimplements 0x4b2fa0: zSnd::ReleaseCachedDirectSound
RECOIL_NOINLINE void RECOIL_CDECL ReleaseCachedDirectSound() {
    LPDIRECTSOUND cached = g_zSnd_CachedDirectSound;
    if (cached != 0) {
        cached->Release();
        g_zSnd_CachedDirectSound = 0;
    }
}

// Reimplements 0x4b2fc0: zSnd::CachedDirectSound_GetCaps
RECOIL_NOINLINE HRESULT RECOIL_FASTCALL CachedDirectSound_GetCaps(DSCAPS *caps) {
    caps->dwSize = sizeof(DSCAPS);
    return g_zSnd_CachedDirectSound->GetCaps(caps);
}
} // namespace zSnd

// Reimplements 0x4a12c0: zSnd_PreInitializeRuntimeState
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSnd_PreInitializeRuntimeState(unsigned int hwnd) {
    if (g_zSnd_PreInitialized != 0) {
        return 0;
    }

    const int activeBackend = g_zSnd_ActiveBackend;
    g_zSnd_PreInitialized = 1;
    g_zSnd_IsInitialized = 0;
    g_zSnd_WindowHandle = hwnd;

    if (activeBackend == 0 || activeBackend == 1) {
        g_zSnd_BackendDevice = 0;
        g_zSnd_BackendListenerHandle = 0;
    }

    g_zSndCd_TrackCount = 0;
    g_zSnd_SearchPathList = 0;
    g_zSndCdDeviceId &= 0xffff0000;
    g_zSndCdFlags &= ~0x03;
    g_zSndCdAuxDeviceId = 0;
    g_zSndCdAuxVolumePrimary = 0;
    g_zSndCdAuxVolumeSecondary = 0;
    g_zSndCdLastPlayMode = 2;

    g_zSnd_SoundLodDefault = 0;
    g_zSnd_SoundLodValuePtr = zGame::Options_FindOption("SoundLOD");
    if (g_zSnd_SoundLodValuePtr == 0) {
        g_zSnd_SoundLodValuePtr = &g_zSnd_SoundLodDefault;
    }

    g_zSnd_MuteOptionDefault = 0;
    g_zSnd_MuteOptionValuePtr = zGame::Options_FindOption("MuteSound");
    if (g_zSnd_MuteOptionValuePtr == 0) {
        g_zSnd_MuteOptionValuePtr = &g_zSnd_MuteOptionDefault;
    }
    g_zSnd_MuteDepth = *static_cast<int *>(g_zSnd_MuteOptionValuePtr);

    g_zSndLastSample = 0;
    g_zSndLastVoice = 0;
    g_zSndLastVoiceHandle = 0;
    g_zSndLastVoiceMarkerIndex = 0;
    g_zSndLastVoiceStopMarkerIndex = 999;

    g_zSnd_VolumeScaleDefault = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = zGame::Options_FindOption("SoundVolume");
    if (g_zSnd_GlobalVolumeScalePtr == 0) {
        g_zSnd_GlobalVolumeScalePtr = &g_zSnd_VolumeScaleDefault;
    }

    g_zSnd_Flag10PlaybackEnabled = 1;
    return 1;
}

// Reimplements 0x4a1420: zSndSystem_Init
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL zSndSystem_Init(unsigned int hwnd,
                                                                        const char *zrdPath) {
    if (g_zSnd_IsInitialized != 0 || g_zSnd_PreInitialized == 0) {
        return 0;
    }

    if (hwnd != 0) {
        g_zSnd_WindowHandle = hwnd;
    }

    if (g_zSnd_ActiveBackend == 0) {
        if (zSndBackend_InitDirectSound() == 0) {
            return 0;
        }
    } else if (g_zSnd_ActiveBackend == 1) {
        if (zSndBackend_InitA3D() == 0) {
            g_zSnd_ActiveBackend = 0;
            return zSndSystem_Init(hwnd, zrdPath);
        }
    }

    g_zSnd_IsInitialized = 1;
    g_zSnd_ConfigRootNode = zReader::LoadNodeFromPath(zrdPath, 0, 0);
    if (g_zSnd_ConfigRootNode == 0) {
        zError::ReportOld(0x200, "D:\\Proj\\GameZRecoil\\zSound\\zsnd_init.cpp", 0x2c9,
                          "Failed to find menu sounds (%s)", zrdPath);
        return 0;
    }

    int syntax = 0;
    if (zReader::ReadNamedInt(g_zSnd_ConfigRootNode, "SYNTAX", &syntax) == 0) {
        syntax = 1;
    }

    if (syntax == 1) {
        zSndSystem_InitLegacySetsSyntax(g_zSnd_ConfigRootNode);
    } else if (syntax == 2) {
        zSndSystem_InitNamedSetsSyntax(g_zSnd_ConfigRootNode);
    }

    return 1;
}

// Reimplements 0x4a1e50: zSndBackend_InitDirectSound
extern "C" RECOIL_NOINLINE int RECOIL_CDECL zSndBackend_InitDirectSound() {
    HRESULT directSoundError = DirectSoundCreate(0, &g_zSnd_BackendDevice, 0);
    if (directSoundError != DS_OK) {
        return zSnd::ReportDirectSoundError(directSoundError, kZSndInitSourceFile, 0x26a);
    }

    directSoundError = g_zSnd_BackendDevice->SetCooperativeLevel(
        reinterpret_cast<HWND>(g_zSnd_WindowHandle), DSSCL_NORMAL);
    if (directSoundError != DS_OK) {
        return zSnd::ReportDirectSoundError(directSoundError, kZSndInitSourceFile, 0x26d);
    }

    g_zSnd_BackendAuxHandleOrConfig.dwSize = sizeof(DSCAPS);
    directSoundError = g_zSnd_BackendDevice->GetCaps(&g_zSnd_BackendAuxHandleOrConfig);
    if (directSoundError != DS_OK) {
        return zSnd::ReportDirectSoundError(directSoundError, kZSndInitSourceFile, 0x271);
    }

    DSBUFFERDESC desc = {0};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN;
    directSoundError =
        g_zSnd_BackendDevice->CreateSoundBuffer(&desc, &g_zSnd_BackendListenerHandle, 0);
    if (directSoundError != DS_OK) {
        return zSnd::ReportDirectSoundError(directSoundError, kZSndInitSourceFile, 0x28d);
    }

    return 1;
}

// Reimplements 0x4a1d10: zSndBackend_InitA3D
extern "C" RECOIL_NOINLINE int RECOIL_CDECL zSndBackend_InitA3D() {
    if (CoInitialize(0) < 0) {
        return 0;
    }

    HRESULT a3dError = CoCreateInstance(kCLSID_A3DApi, 0, CLSCTX_INPROC_SERVER, kIID_IA3d3,
                                        reinterpret_cast<void **>(&g_zSnd_BackendDevice));
    if (a3dError < 0) {
        if (a3dError == CLASS_E_NOAGGREGATION) {
            printf("A3D: This class cannot be created as part of an aggregate.\n");
            return 0;
        }

        if (a3dError == REGDB_E_CLASSNOTREG) {
            printf("A3D: Not registered in the registration database.\n");
            return 0;
        }

        printf("A3D: Unknown error.\n");
        return 0;
    }

    A3DApi *api = reinterpret_cast<A3DApi *>(g_zSnd_BackendDevice);
    api->vtbl->ConfigureOutput(api, 0, 0x28, 0x0c);
    api->vtbl->SetCooperativeLevel(api, reinterpret_cast<HWND>(g_zSnd_WindowHandle), 1);

    a3dError = api->vtbl->QueryInterface(
        api, kIID_IA3dGeom, reinterpret_cast<void **>(&g_zSnd_BackendAuxHandleOrConfig));
    if (a3dError != 0) {
        return zSnd::ReportA3DError(a3dError, kZSndInitSourceFile, 0x245);
    }

    a3dError = api->vtbl->QueryInterface(api, kIID_IA3dListener,
                                         reinterpret_cast<void **>(&g_zSnd_BackendListenerHandle));
    if (a3dError != 0) {
        return zSnd::ReportA3DError(a3dError, kZSndInitSourceFile, 0x24b);
    }

    a3dError = api->vtbl->SetOutputMode(api, 2);
    if (a3dError != 0) {
        return zSnd::ReportA3DError(a3dError, kZSndInitSourceFile, 0x24e);
    }

    api->vtbl->Tick(api);

    void *outBuffer = 0;
    api->vtbl->CreateBufferByKind(api, 0, &outBuffer);
    if (outBuffer != 0) {
        void **vtbl = *reinterpret_cast<void ***>(outBuffer);
        reinterpret_cast<UnknownReleaseProc>(vtbl[2])(outBuffer);
    }

    return 1;
}

namespace zSndBackend {
namespace {
void ReleaseUnknown(void *&object) {
    if (object != 0) {
        void **vtable = *(void ***)object;
        ((UnknownReleaseProc)vtable[2])(object);
        object = 0;
    }
}
} // namespace

// Reimplements 0x4a1f40: zSndBackend::Shutdown
RECOIL_NOINLINE int RECOIL_CDECL Shutdown() {
    if (g_zSnd_IsInitialized == 0 || g_zSnd_PreInitialized == 0) {
        return 0;
    }

    zSndCd::Shutdown();
    zSndStreamMgr::Shutdown();
    zSndSampleSetRegistry_DestroyAll();

    if (g_zSnd_ActiveBackend == 1) {
        void *&auxObject = *(void **)&g_zSnd_BackendAuxHandleOrConfig;
        ReleaseUnknown(auxObject);
    }

    if (g_zSnd_BackendListenerHandle != 0) {
        g_zSnd_BackendListenerHandle->Release();
        g_zSnd_BackendListenerHandle = 0;
    }

    if (g_zSnd_BackendDevice != 0) {
        g_zSnd_BackendDevice->Release();
        g_zSnd_BackendDevice = 0;
    }

    if (g_zSnd_ActiveBackend == 1) {
        CoUninitialize();
    }

    g_zSnd_IsInitialized = 0;
    return 1;
}
} // namespace zSndBackend
