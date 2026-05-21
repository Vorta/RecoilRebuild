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

} // namespace

namespace zSnd {
// Reimplements 0x4a3ef0: zSnd::ReportA3DError
RECOIL_NOINLINE int RECOIL_FASTCALL ReportA3DError(int a3dError,
                                                            const char *sourceFile,
                                                            int sourceLine) {
    char errorNameStorage[0x100];
    if (a3dError <= 0) {
        if (a3dError != 0) {
            switch (static_cast<unsigned int>(a3dError)) {
    case 0x80040001u:
        sprintf(errorNameStorage, "\tA3DERROR_MEMORY_ALLOCATION\t");
        break;
    case 0x80040002u:
        sprintf(errorNameStorage, "\tA3DERROR_FAILED_CREATE_PRIMARY_BUFFER\t");
        break;
    case 0x80040003u:
        sprintf(errorNameStorage, "\tA3DERROR_FAILED_CREATE_SECONDARY_BUFFER\t");
        break;
    case 0x80040004u:
        sprintf(errorNameStorage, "\tA3DERROR_FAILED_INIT_A3D_DRIVER\t");
        break;
    case 0x80040005u:
        sprintf(errorNameStorage, "\tA3DERROR_FAILED_QUERY_DIRECTSOUND\t");
        break;
    case 0x80040006u:
        sprintf(errorNameStorage, "\tA3DERROR_FAILED_QUERY_A3D3\t");
        break;
    case 0x80040007u:
        sprintf(errorNameStorage, "\tA3DERROR_FAILED_INIT_A3D3\t");
        break;
    case 0x80040008u:
        sprintf(errorNameStorage, "\tA3DERROR_FAILED_QUERY_A3D2\t");
        break;
    case 0x80040009u:
        sprintf(errorNameStorage, "\tA3DERROR_FAILED_FILE_OPEN\t");
        break;
    case 0x8004000au:
        sprintf(errorNameStorage, "\tA3DERROR_FAILED_CREATE_SOUNDBUFFER\t");
        break;
    case 0x8004000bu:
        sprintf(errorNameStorage, "\tA3DERROR_FAILED_QUERY_3DINTERFACE\t");
        break;
    case 0x8004000cu:
        sprintf(errorNameStorage, "\tA3DERROR_FAILED_LOCK_BUFFER\t");
        break;
    case 0x8004000du:
        sprintf(errorNameStorage, "\tA3DERROR_FAILED_UNLOCK_BUFFER\t");
        break;
    case 0x8004000eu:
        sprintf(errorNameStorage, "\tA3DERROR_UNRECOGNIZED_FORMAT\t");
        break;
    case 0x8004000fu:
        sprintf(errorNameStorage, "\tA3DERROR_NO_WAVE_DATA\t");
        break;
    case 0x80040010u:
        sprintf(errorNameStorage, "\tA3DERROR_UNKNOWN_PLAYMODE\t");
        break;
    case 0x80040011u:
        sprintf(errorNameStorage, "\tA3DERROR_FAILED_PLAY\t");
        break;
    case 0x80040012u:
        sprintf(errorNameStorage, "\tA3DERROR_FAILED_STOP\t");
        break;
    case 0x80040013u:
        sprintf(errorNameStorage, "\tA3DERROR_NEEDS_FORMAT_INFORMATION\t");
        break;
    case 0x80040014u:
        sprintf(errorNameStorage, "\tA3DERROR_FAILED_ALLOCATE_WAVEDATA\t");
        break;
    case 0x80040015u:
        sprintf(errorNameStorage, "\tA3DERROR_NOT_VALID_SOURCE\t");
        break;
    case 0x80040016u:
        sprintf(errorNameStorage, "\tA3DERROR_FAILED_DUPLICATION\t");
        break;
    case 0x80040017u:
        sprintf(errorNameStorage, "\tA3DERROR_FAILED_INIT\t");
        break;
    case 0x80040018u:
        sprintf(errorNameStorage, "\tA3DERROR_FAILED_SETCOOPERATIVE_LEVEL\t");
        break;
    case 0x80040019u:
        sprintf(errorNameStorage, "\tA3DERROR_FAILED_INIT_QUERIED_INTERFACE\t");
        break;
    case 0x8004001au:
        sprintf(errorNameStorage, "\tA3DERROR_GEOMETRY_INPUT_OUTSIDE_BEGIN_END_BLOCK\t");
        break;
    case 0x8004001bu:
        sprintf(errorNameStorage, "\tA3DERROR_INVALID_NORMAL\t");
        break;
    case 0x8004001cu:
        sprintf(errorNameStorage, "\tA3DERROR_END_BEFORE_VALID_BEGIN_BLOCK\t");
        break;
    case 0x8004001du:
        sprintf(errorNameStorage, "\tA3DERROR_INVALID_BEGIN_MODE\t");
        break;
    case 0x8004001eu:
        sprintf(errorNameStorage, "\tA3DERROR_INVALID_ARGUMENT\t");
        break;
    case 0x8004001fu:
        sprintf(errorNameStorage, "\tA3DERROR_INVALID_INDEX\t");
        break;
    case 0x80040020u:
        sprintf(errorNameStorage, "\tA3DERROR_INVALID_VERTEX_INDEX\t");
        break;
    case 0x80040021u:
        sprintf(errorNameStorage, "\tA3DERROR_INVALID_PRIMITIVE_INDEX\t");
        break;
    case 0x80040022u:
        sprintf(errorNameStorage, "\tA3DERROR_MIXING_2D_AND_3D_MODES\t");
        break;
    case 0x80040023u:
        sprintf(errorNameStorage, "\tA3DERROR_2DWALL_REQUIRES_EXACTLY_ONE_LINE\t");
        break;
    case 0x80040024u:
        sprintf(errorNameStorage, "\tA3DERROR_NO_PRIMITIVES_DEFINED\t");
        break;
    case 0x80040025u:
        sprintf(errorNameStorage, "\tA3DERROR_PRIMITIVES_NON_PLANAR\t");
        break;
    case 0x80040026u:
        sprintf(errorNameStorage, "\tA3DERROR_PRIMITIVES_OVERLAPPING\t");
        break;
    case 0x80040027u:
        sprintf(errorNameStorage, "\tA3DERROR_PRIMITIVES_NOT_ADJACENT\t");
        break;
    case 0x80040028u:
        sprintf(errorNameStorage, "\tA3DERROR_OBJECT_NOT_FOUND\t");
        break;
    case 0x80040029u:
        sprintf(errorNameStorage, "\tA3DERROR_ROOM_HAS_NO_SHELL_WALLS\t");
        break;
    case 0x8004002au:
        sprintf(errorNameStorage, "\tA3DERROR_WALLS_DO_NOT_ENCLOSE_ROOM\t");
        break;
    case 0x8004002bu:
        sprintf(errorNameStorage, "\tA3DERROR_INVALID_WALL\t");
        break;
    case 0x8004002cu:
        sprintf(errorNameStorage, "\tA3DERROR_ROOM_HAS_LESS_THAN_4SHELL_WALLS\t");
        break;
    case 0x8004002du:
        sprintf(errorNameStorage, "\tA3DERROR_ROOM_HAS_LESS_THAN_3UNIQUE_NORMALS\t");
        break;
    case 0x8004002eu:
        sprintf(errorNameStorage, "\tA3DERROR_INTERSECTING_WALL_EDGES\t");
        break;
    case 0x8004002fu:
        sprintf(errorNameStorage, "\tA3DERROR_INVALID_ROOM\t");
        break;
    case 0x80040030u:
        sprintf(errorNameStorage, "\tA3DERROR_SCENE_HAS_ROOMS_INSIDE_ANOTHER_ROOMS\t");
        break;
    case 0x80040031u:
        sprintf(errorNameStorage, "\tA3DERROR_SCENE_HAS_OVERLAPPING_STATIC_ROOMS\t");
        break;
    case 0x80040032u:
        sprintf(errorNameStorage, "\tA3DERROR_DYNAMIC_OBJ_UNSUPPORTED\t");
        break;
    case 0x80040033u:
        sprintf(errorNameStorage, "\tA3DERROR_DIR_AND_UP_VECTORS_NOT_PERPENDICULAR\t");
        break;
    case 0x80040034u:
        sprintf(errorNameStorage, "\tA3DERROR_INVALID_ROOM_INDEX\t");
        break;
    case 0x80040035u:
        sprintf(errorNameStorage, "\tA3DERROR_INVALID_WALL_INDEX\t");
        break;
    case 0x80040036u:
        sprintf(errorNameStorage, "\tA3DERROR_SCENE_INVALID\t");
        break;
    case 0x80040037u:
        sprintf(errorNameStorage, "\tA3DERROR_UNIMPLEMENTED_FUNCTION\t");
        break;
    case 0x80040038u:
        sprintf(errorNameStorage, "\tA3DERROR_NO_ROOMS_IN_SCENE\t");
        break;
    case 0x80040039u:
        sprintf(errorNameStorage, "\tA3DERROR_2D_GEOMETRY_UNIMPLEMENTED\t");
        break;
    default:
        goto reportUnknownA3D;
            }

            goto reportA3D;
        }
        return 1;
    }

reportUnknownA3D:
    sprintf(errorNameStorage, "UNKNOWN");

reportA3D:
    zError::ReportOld(0x400, sourceFile, sourceLine, "A3D Error [%s]", errorNameStorage);
    return 0;
}

// Reimplements 0x4a4330: zSnd::ReportDirectSoundError
RECOIL_NOINLINE int RECOIL_FASTCALL ReportDirectSoundError(int directSoundError,
                                                                    const char *sourceFile,
                                                                    int sourceLine) {
    char errorNameStorage[0x100];
    switch (static_cast<HRESULT>(directSoundError)) {
    case DSERR_GENERIC:
        sprintf(errorNameStorage, "DSERR_GENERIC");
        break;
    case DSERR_UNSUPPORTED:
        sprintf(errorNameStorage, "DSERR_UNSUPPORTED");
        break;
    case DSERR_OUTOFMEMORY:
        sprintf(errorNameStorage, "DSERR_OUTOFMEMORY");
        break;
    case DSERR_NOAGGREGATION:
        sprintf(errorNameStorage, "DSERR_NOAGGREGATION");
        break;
    case DSERR_INVALIDPARAM:
        sprintf(errorNameStorage, "DSERR_INVALIDPARAM");
        break;
    case DSERR_ALLOCATED:
        sprintf(errorNameStorage, "DSERR_ALLOCATED");
        break;
    case DSERR_CONTROLUNAVAIL:
        sprintf(errorNameStorage, "DSERR_CONTROLUNAVAIL");
        break;
    case DSERR_INVALIDCALL:
        sprintf(errorNameStorage, "DSERR_INVALIDCALL");
        break;
    case DSERR_PRIOLEVELNEEDED:
        sprintf(errorNameStorage, "DSERR_PRIOLEVELNEEDED");
        break;
    case DSERR_BADFORMAT:
        sprintf(errorNameStorage, "DSERR_BADFORMAT");
        break;
    case DSERR_NODRIVER:
        sprintf(errorNameStorage, "DSERR_NODRIVER");
        break;
    case DSERR_ALREADYINITIALIZED:
        sprintf(errorNameStorage, "DSERR_ALREADYINITIALIZED");
        break;
    case DSERR_BUFFERLOST:
        sprintf(errorNameStorage, "DSERR_BUFFERLOST");
        break;
    case DSERR_OTHERAPPHASPRIO:
        sprintf(errorNameStorage, "DSERR_OTHERAPPHASPRIO");
        break;
    case DS_OK:
        return 1;
    default:
        sprintf(errorNameStorage, "UNKNOWN");
        break;
    }

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
        (HWND)(g_zSnd_WindowHandle), DSSCL_NORMAL);
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
                                        (void **)(&g_zSnd_BackendDevice));
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

    A3DApi *api = (A3DApi *)(g_zSnd_BackendDevice);
    api->vtbl->ConfigureOutput(api, 0, 0x28, 0x0c);
    api->vtbl->SetCooperativeLevel(api, (HWND)(g_zSnd_WindowHandle), 1);

    a3dError = api->vtbl->QueryInterface(
        api, kIID_IA3dGeom, (void **)(&g_zSnd_BackendAuxHandleOrConfig));
    if (a3dError != 0) {
        return zSnd::ReportA3DError(a3dError, kZSndInitSourceFile, 0x245);
    }

    a3dError = api->vtbl->QueryInterface(api, kIID_IA3dListener,
                                         (void **)(&g_zSnd_BackendListenerHandle));
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
        void **vtbl = *(void ***)(outBuffer);
        ((UnknownReleaseProc)(vtbl[2]))(outBuffer);
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
