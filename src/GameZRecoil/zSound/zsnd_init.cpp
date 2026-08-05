#include "zsnd.h"

#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd_a3d_provider.h"
#include "GameZRecoil/zSys/zsys.h"

#include "recoil/recoil_types.h"
#include <stdio.h>

extern "C" char g_Player_MasterTypeName_Unknown[0x08];
extern "C" LPDIRECTSOUND g_zSnd_BackendDevice = 0;
extern "C" LPDIRECTSOUNDBUFFER g_zSnd_BackendListenerHandle = 0;
extern "C" DSCAPS g_zSnd_BackendAuxHandleOrConfig = {0};
extern "C" LPDIRECTSOUND g_zSnd_CachedDirectSound = 0;
extern "C" const GUID *g_zSnd_CachedDirectSoundGuid = 0;
extern "C" int g_zSndCdTrackListCount;
extern "C" int g_zSnd_PreInitialized = 0;
extern "C" int g_zSnd_SoundLodDefault = 0;
extern "C" void *g_zSnd_SoundLodValuePtr = 0;
/**
 * Purpose: Stores g zSnd MuteOptionDefault data used by engine.zsound.option_runtime_globals.
 */
extern "C" int g_zSnd_MuteOptionDefault = 0;
/**
 * Purpose: Stores g zSnd MuteOptionValuePtr data used by engine.zsound.option_runtime_globals.
 */
extern "C" void *g_zSnd_MuteOptionValuePtr = 0;
/**
 * Purpose: Stores g zSnd MuteDepth data used by engine.zsound.option_runtime_globals.
 */
extern "C" int g_zSnd_MuteDepth = 0;
/**
 * Purpose: Stores g zSnd VolumeScaleDefault data used by engine.zsound.option_runtime_globals.
 */
extern "C" float g_zSnd_VolumeScaleDefault = 0.0f;
/**
 * Purpose: Stores g zSnd GlobalVolumeScalePtr data used by engine.zsound.option_runtime_globals.
 */
extern "C" void *g_zSnd_GlobalVolumeScalePtr = 0;
extern "C" zSndSample *g_zSndLastSample = 0;
extern "C" zSndSample *g_zSndLastVoice = 0;
extern "C" zSndPlayHandle *g_zSndLastVoiceHandle = 0;
extern "C" int g_zSndLastVoiceMarkerIndex = 0;
extern "C" int g_zSndLastVoiceStopMarkerIndex = 0;
extern "C" int g_zSnd_Flag10PlaybackEnabled = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsndconfig-syntaxkey
 * @recoil-artifact defines .data recoil:data:0x4e2288: g_zSndConfig_SyntaxKey.
 * Owner data: zSound init config/startup literals.
 * Purpose: names the startup sound-config syntax selector.
 */
char g_zSndConfig_SyntaxKey[0x07] = "SYNTAX";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsnd-sourcefile-zsnd-init-cpp
 * @recoil-artifact defines .data recoil:data:0x4e2290: g_zSnd_SourceFile_zsnd_init_cpp.
 * Owner data: zSound init config/startup literals.
 * Purpose: supplies the recovered source-file path for zSound startup diagnostics.
 */
char g_zSnd_SourceFile_zsnd_init_cpp[0x29] =
    "D:\\Proj\\GameZRecoil\\zSound\\zsnd_init.cpp";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsndinitfailedtofindmenusoundsfmt
 * @recoil-artifact defines .data recoil:data:0x4e22bc: g_zSndInitFailedToFindMenuSoundsFmt.
 * Owner data: zSound init config/startup literals.
 * Purpose: formats the missing menu-sounds startup diagnostic.
 */
char g_zSndInitFailedToFindMenuSoundsFmt[0x20] =
    "Failed to find menu sounds (%s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsndconfig-soundgroupskey
 * @recoil-artifact defines .data recoil:data:0x4e22dc: g_zSndConfig_SoundGroupsKey.
 * Owner data: zSound config parser key literals.
 * Purpose: names the optional sound group list in the sound config tree.
 */
char g_zSndConfig_SoundGroupsKey[0x0d] = "SOUND_GROUPS";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsndconfig-setskey
 * @recoil-artifact defines .data recoil:data:0x4e22ec: g_zSndConfig_SetsKey.
 * Owner data: zSound config parser key literals.
 * Purpose: names the top-level sound set list in the sound config tree.
 */
char g_zSndConfig_SetsKey[0x05] = "SETS";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsndconfig-speedofsoundkey
 * @recoil-artifact defines .data recoil:data:0x4e22f4: g_zSndConfig_SpeedOfSoundKey.
 * Owner data: zSound config parser key literals.
 * Purpose: names the optional speed-of-sound config value.
 */
char g_zSndConfig_SpeedOfSoundKey[0x0f] = "SPEED_OF_SOUND";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsndconfig-soundpathkey
 * @recoil-artifact defines .data recoil:data:0x4e2304: g_zSndConfig_SoundPathKey.
 * Owner data: zSound config parser key literals.
 * Purpose: names the optional sound archive search path.
 */
char g_zSndConfig_SoundPathKey[0x0b] = "SOUND_PATH";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsndconfig-cdtrackskey
 * @recoil-artifact defines .data recoil:data:0x4e2310: g_zSndConfig_CdTracksKey.
 * Owner data: zSound config parser key literals.
 * Purpose: names the optional CD track config block.
 */
char g_zSndConfig_CdTracksKey[0x0a] = "CD_TRACKS";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsndconfig-qualitymedtoken
 * @recoil-artifact defines .data recoil:data:0x4e231c: g_zSndConfig_QualityMedToken.
 * Owner data: zSound config parser key literals.
 * Purpose: names the medium quality sample variant token.
 */
char g_zSndConfig_QualityMedToken[0x04] = "MED";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsndconfig-a3ddistancekey
 * @recoil-artifact defines .data recoil:data:0x4e2320: g_zSndConfig_A3dDistanceKey.
 * Owner data: zSound config parser key literals.
 * Purpose: names the optional A3D distance scale value.
 */
char g_zSndConfig_A3dDistanceKey[0x08] = "A3DDIST";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsndconfig-volumekey
 * @recoil-artifact defines .data recoil:data:0x4e2328: g_zSndConfig_VolumeKey.
 * Owner data: zSound config parser key literals.
 * Purpose: names the optional per-sample volume value.
 */
char g_zSndConfig_VolumeKey[0x07] = "VOLUME";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsndconfig-voicekey
 * @recoil-artifact defines .data recoil:data:0x4e2330: g_zSndConfig_VoiceKey.
 * Owner data: zSound config parser key literals.
 * Purpose: names the optional voice-playback sample flag.
 */
char g_zSndConfig_VoiceKey[0x06] = "VOICE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsndconfig-purgeablekey
 * @recoil-artifact defines .data recoil:data:0x4e2338: g_zSndConfig_PurgeableKey.
 * Owner data: zSound config parser key literals.
 * Purpose: names the optional purgeable sample flag.
 */
char g_zSndConfig_PurgeableKey[0x0a] = "PURGEABLE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsndconfig-hardwarekey
 * @recoil-artifact defines .data recoil:data:0x4e2344: g_zSndConfig_HardwareKey.
 * Owner data: zSound config parser key literals.
 * Purpose: names the optional hardware-buffer sample flag.
 */
char g_zSndConfig_HardwareKey[0x09] = "HARDWARE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsndconfig-frequencykey
 * @recoil-artifact defines .data recoil:data:0x4e2350: g_zSndConfig_FrequencyKey.
 * Owner data: zSound config parser key literals.
 * Purpose: names the optional frequency-control sample flag.
 */
char g_zSndConfig_FrequencyKey[0x0a] = "FREQUENCY";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsndconfig-loopedkey
 * @recoil-artifact defines .data recoil:data:0x4e235c: g_zSndConfig_LoopedKey.
 * Owner data: zSound config parser key literals.
 * Purpose: names the optional looped sample flag.
 */
char g_zSndConfig_LoopedKey[0x07] = "LOOPED";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsndconfig-3dkey
 * @recoil-artifact defines .data recoil:data:0x4e2364: g_zSndConfig_3dKey.
 * Owner data: zSound config parser key literals.
 * Purpose: names the optional 3D sample flag.
 */
char g_zSndConfig_3dKey[0x03] = "3D";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsnd-a3diniterror-aggregatemsg
 * @recoil-artifact defines .data recoil:data:0x4e2368: g_zSnd_A3DInitError_AggregateMsg.
 * Owner data: zSound init backend diagnostic literals.
 * Purpose: reports CLASS_E_NOAGGREGATION from A3D provider creation.
 */
char g_zSnd_A3DInitError_AggregateMsg[0x3c] =
    "A3D: This class cannot be created as part of an aggregate.\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsnd-a3diniterror-notregisteredmsg
 * @recoil-artifact defines .data recoil:data:0x4e23a4: g_zSnd_A3DInitError_NotRegisteredMsg.
 * Owner data: zSound init backend diagnostic literals.
 * Purpose: reports REGDB_E_CLASSNOTREG from A3D provider creation.
 */
char g_zSnd_A3DInitError_NotRegisteredMsg[0x33] =
    "A3D: Not registered in the registration database.\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsnd-a3diniterror-unknownmsg
 * @recoil-artifact defines .data recoil:data:0x4e23d8: g_zSnd_A3DInitError_UnknownMsg.
 * Owner data: zSound init backend diagnostic literals.
 * Purpose: reports an unclassified failure from A3D provider creation.
 */
char g_zSnd_A3DInitError_UnknownMsg[0x15] = "A3D: Unknown error.\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsnd-a3derrorfmt
 * @recoil-artifact defines .data recoil:data:0x4e24b8: g_zSnd_A3DErrorFmt.
 * Owner data: zSound init backend diagnostic literals.
 * Purpose: formats A3D provider error names for zError reporting.
 */
char g_zSnd_A3DErrorFmt[0x0f] = "A3D Error [%s]";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsnd-directsounderrorfmt
 * @recoil-artifact defines .data recoil:data:0x4e2c84: g_zSnd_DirectSoundErrorFmt.
 * Owner data: zSound init backend diagnostic literals.
 * Purpose: formats DirectSound provider error names for zError reporting.
 */
char g_zSnd_DirectSoundErrorFmt[0x17] = "DirectSound Error [%s]";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsnd-dserrorname-bufferlost
 * @recoil-artifact defines .data recoil:data:0x4e2c9c: g_zSnd_DsErrorName_BufferLost.
 * Owner data: zSound DirectSound error-name literals.
 * Purpose: names DSERR_BUFFERLOST in DirectSound diagnostics.
 */
char g_zSnd_DsErrorName_BufferLost[0x11] = "DSERR_BUFFERLOST";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsnd-dserrorname-otherapphasprio
 * @recoil-artifact defines .data recoil:data:0x4e2cb0: g_zSnd_DsErrorName_OtherAppHasPrio.
 * Owner data: zSound DirectSound error-name literals.
 * Purpose: names DSERR_OTHERAPPHASPRIO in DirectSound diagnostics.
 */
char g_zSnd_DsErrorName_OtherAppHasPrio[0x16] = "DSERR_OTHERAPPHASPRIO";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsnd-dserrorname-alreadyinitialized
 * @recoil-artifact defines .data recoil:data:0x4e2cc8: g_zSnd_DsErrorName_AlreadyInitialized.
 * Owner data: zSound DirectSound error-name literals.
 * Purpose: names DSERR_ALREADYINITIALIZED in DirectSound diagnostics.
 */
char g_zSnd_DsErrorName_AlreadyInitialized[0x19] = "DSERR_ALREADYINITIALIZED";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsnd-dserrorname-nodriver
 * @recoil-artifact defines .data recoil:data:0x4e2ce4: g_zSnd_DsErrorName_NoDriver.
 * Owner data: zSound DirectSound error-name literals.
 * Purpose: names DSERR_NODRIVER in DirectSound diagnostics.
 */
char g_zSnd_DsErrorName_NoDriver[0x0f] = "DSERR_NODRIVER";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsnd-dserrorname-badformat
 * @recoil-artifact defines .data recoil:data:0x4e2cf4: g_zSnd_DsErrorName_BadFormat.
 * Owner data: zSound DirectSound error-name literals.
 * Purpose: names DSERR_BADFORMAT in DirectSound diagnostics.
 */
char g_zSnd_DsErrorName_BadFormat[0x10] = "DSERR_BADFORMAT";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsnd-dserrorname-priolevelneeded
 * @recoil-artifact defines .data recoil:data:0x4e2d04: g_zSnd_DsErrorName_PrioLevelNeeded.
 * Owner data: zSound DirectSound error-name literals.
 * Purpose: names DSERR_PRIOLEVELNEEDED in DirectSound diagnostics.
 */
char g_zSnd_DsErrorName_PrioLevelNeeded[0x16] = "DSERR_PRIOLEVELNEEDED";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsnd-dserrorname-invalidcall
 * @recoil-artifact defines .data recoil:data:0x4e2d1c: g_zSnd_DsErrorName_InvalidCall.
 * Owner data: zSound DirectSound error-name literals.
 * Purpose: names DSERR_INVALIDCALL in DirectSound diagnostics.
 */
char g_zSnd_DsErrorName_InvalidCall[0x12] = "DSERR_INVALIDCALL";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsnd-dserrorname-controlunavail
 * @recoil-artifact defines .data recoil:data:0x4e2d30: g_zSnd_DsErrorName_ControlUnavail.
 * Owner data: zSound DirectSound error-name literals.
 * Purpose: names DSERR_CONTROLUNAVAIL in DirectSound diagnostics.
 */
char g_zSnd_DsErrorName_ControlUnavail[0x15] = "DSERR_CONTROLUNAVAIL";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsnd-dserrorname-allocated
 * @recoil-artifact defines .data recoil:data:0x4e2d48: g_zSnd_DsErrorName_Allocated.
 * Owner data: zSound DirectSound error-name literals.
 * Purpose: names DSERR_ALLOCATED in DirectSound diagnostics.
 */
char g_zSnd_DsErrorName_Allocated[0x10] = "DSERR_ALLOCATED";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsnd-dserrorname-invalidparam
 * @recoil-artifact defines .data recoil:data:0x4e2d58: g_zSnd_DsErrorName_InvalidParam.
 * Owner data: zSound DirectSound error-name literals.
 * Purpose: names DSERR_INVALIDPARAM in DirectSound diagnostics.
 */
char g_zSnd_DsErrorName_InvalidParam[0x13] = "DSERR_INVALIDPARAM";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsnd-dserrorname-outofmemory
 * @recoil-artifact defines .data recoil:data:0x4e2d6c: g_zSnd_DsErrorName_OutOfMemory.
 * Owner data: zSound DirectSound error-name literals.
 * Purpose: names DSERR_OUTOFMEMORY in DirectSound diagnostics.
 */
char g_zSnd_DsErrorName_OutOfMemory[0x12] = "DSERR_OUTOFMEMORY";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsnd-dserrorname-noaggregation
 * @recoil-artifact defines .data recoil:data:0x4e2d80: g_zSnd_DsErrorName_NoAggregation.
 * Owner data: zSound DirectSound error-name literals.
 * Purpose: names DSERR_NOAGGREGATION in DirectSound diagnostics.
 */
char g_zSnd_DsErrorName_NoAggregation[0x14] = "DSERR_NOAGGREGATION";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsnd-dserrorname-generic
 * @recoil-artifact defines .data recoil:data:0x4e2d94: g_zSnd_DsErrorName_Generic.
 * Owner data: zSound DirectSound error-name literals.
 * Purpose: names DSERR_GENERIC in DirectSound diagnostics.
 */
char g_zSnd_DsErrorName_Generic[0x0e] = "DSERR_GENERIC";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.g-zsnd-dserrorname-unsupported
 * @recoil-artifact defines .data recoil:data:0x4e2da4: g_zSnd_DsErrorName_Unsupported.
 * Owner data: zSound DirectSound error-name literals.
 * Purpose: names DSERR_UNSUPPORTED in DirectSound diagnostics.
 */
char g_zSnd_DsErrorName_Unsupported[0x12] = "DSERR_UNSUPPORTED";

namespace {
const GUID kCLSID_A3DApi = {0x92fa2c24,
    0x253c,
    0x11d2,
    {0x90, 0xfb, 0x00, 0x60, 0x08, 0xa1, 0xf4, 0x41}};
const GUID kIID_IA3d3 = {0xc398e560,
    0xd90b,
    0x11d1,
    {0x90, 0xfb, 0x00, 0x60, 0x08, 0xa1, 0xf4, 0x41}};
const GUID kIID_IA3dGeom = {0xc398e561,
    0xd90b,
    0x11d1,
    {0x90, 0xfb, 0x00, 0x60, 0x08, 0xa1, 0xf4, 0x41}};
const GUID kIID_IA3dListener = {0xc398e563,
    0xd90b,
    0x11d1,
    {0x90, 0xfb, 0x00, 0x60, 0x08, 0xa1, 0xf4, 0x41}};

} // namespace

/*
 * zgame_opt.c physical-contribution routing anchors. The ordinary definitions
 * now compile from the registered options/runtime-probe translation unit.
 */

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.zsnd-preinitializeruntimestate
 * @recoil-artifact defines .text recoil:function:0x4a12c0: zSnd_PreInitializeRuntimeState.
 *
 * Purpose: reset sound runtime globals, cache option pointers, and prepare the
 * selected backend for later initialization.
 */
extern "C" int __fastcall zSnd_PreInitializeRuntimeState(
    unsigned int hwnd
) {
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

    g_zSndCdFlags &= ~0x03;
    g_zSndCdTrackListCount = 0;
    g_zSnd_SearchPathList = 0;
    g_zSndCdDeviceId &= 0xffff0000;
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
    g_zSnd_MuteDepth = *(int *)(g_zSnd_MuteOptionValuePtr);

    g_zSndLastSample = 0;
    g_zSndLastVoice = 0;

    g_zSnd_VolumeScaleDefault = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = zGame::Options_FindOption("SoundVolume");
    if (g_zSnd_GlobalVolumeScalePtr == 0) {
        g_zSnd_GlobalVolumeScalePtr = &g_zSnd_VolumeScaleDefault;
    }

    g_zSnd_Flag10PlaybackEnabled = 1;
    return 1;
}

namespace zSndSystem {
/**
 * Evidence: BN 0x4a13d0 calls the sound subsystem shutdown routines, then
 * frees g_zSnd_ConfigRootNode and g_zSnd_SearchPathList when present.
 * Purpose: shut down sound subsystems and release sound config/search-path
 * resources.
 */
int __cdecl Shutdown() {
    zSndStreamMgr::Shutdown();
    zSndBackend::Shutdown();
    zSndCd::Shutdown();
    zSndSampleSetRegistry_DestroyAll();
    zSndFadeLists::StopAllAndShutdown();

    if (g_zSnd_ConfigRootNode != 0) {
        zReader::FreeLoadedTree(g_zSnd_ConfigRootNode);
        g_zSnd_ConfigRootNode = 0;
    }

    if (g_zSnd_SearchPathList != 0) {
        g_zSnd_SearchPathList = zUtil_ZRDR_FreeSearchPathList(g_zSnd_SearchPathList);
    }

    return 1;
}
} // namespace zSndSystem

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.zsndsystem-init
 * @recoil-artifact defines .text recoil:function:0x4a1420: zSndSystem_Init.
 *
 * Purpose: initialize the selected sound backend, load the sound configuration
 * tree, and dispatch the supported syntax parser.
 */
extern "C" int __fastcall zSndSystem_Init(
    unsigned int hwnd,
    const char *zrdPath
) {
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
            return zSndSystem_Init(
                hwnd,
                zrdPath
            );
        }
    }

    g_zSnd_IsInitialized = 1;
    g_zSnd_ConfigRootNode = zReader::LoadNodeFromPath(
        zrdPath,
        0,
        0
    );
    if (g_zSnd_ConfigRootNode == 0) {
        zError::ReportOld(
            0x200,
            g_zSnd_SourceFile_zsnd_init_cpp,
            0x2c9,
            g_zSndInitFailedToFindMenuSoundsFmt,
            zrdPath
        );
        return 0;
    }

    int syntax = 0;
    if (zReader::ReadNamedInt(
        g_zSnd_ConfigRootNode,
        g_zSndConfig_SyntaxKey,
        &syntax
    ) == 0) {
        syntax = 1;
    }

    if (syntax == 1) {
        zSndSystem_InitLegacySetsSyntax(g_zSnd_ConfigRootNode);
    } else if (syntax == 2) {
        zSndSystem_InitNamedSetsSyntax(g_zSnd_ConfigRootNode);
    }

    return 1;
}

/**
 * Evidence: BN 0x4a1510 expands the legacy positional SETS parser in this
 * source file, using the same namespace config/search-path state as 0x4a1870.
 * Purpose: load legacy sound sets, optional CD tracks, search paths, groups,
 * and speed-of-sound settings from the sound config tree.
 */
extern "C" int __fastcall zSndSystem_InitLegacySetsSyntax(
    zReader::Node *configRootNode
) {
    (void)configRootNode;

    zSndCd::Init(zReader_GetNamedNode(
        g_zSnd_ConfigRootNode,
        g_zSndConfig_CdTracksKey
    ));

    const char *pathText = zReader::ReadNamedString(
        g_zSnd_ConfigRootNode,
        g_zSndConfig_SoundPathKey
    );
    if (pathText != 0) {
        if (g_zSnd_SearchPathList == 0) {
            g_zSnd_SearchPathList = zUtil_ZRDR_CreateSearchPathList(pathText);
        } else {
            zUtil::ZRDR_AddSearchPaths(
                g_zSnd_SearchPathList,
                pathText
            );
        }
    }

    float speedOfSound = 0.0f;
    if (zReader::ReadNamedFloat(
        g_zSnd_ConfigRootNode,
        g_zSndConfig_SpeedOfSoundKey,
        &speedOfSound
    ) != 0) {
        zSnd::SetSpeedOfSoundMps(speedOfSound);
    }

    zReader::Node *setsNode = zReader_GetNamedNode(
        g_zSnd_ConfigRootNode,
        g_zSndConfig_SetsKey
    );
    zReader::Node *sets = setsNode->value.nodes;
    const int setCount = (sets[0].value.i32 - 1) / 2;
    for (int i = 0; i < setCount; ++i) {
        zReader::Node *setNameNode = &sets[(i * 2) + 1];
        zReader::Node *sampleListNode = &sets[(i * 2) + 2];
        const int sampleCount = sampleListNode->value.nodes[0].value.i32 - 1;
        zSndSampleSet *sampleSet =
            (zSndSampleSet *)(::operator new(sizeof(zSndSampleSet)));
        if (sampleSet != 0) {
            sampleSet = sampleSet->RegistryAddEntry(
                setNameNode->value.str,
                sampleCount
            );
        }

        zReader::Node *entries = sampleListNode->value.nodes;
        for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
            zSndSample *sample = sampleSet->GetSampleAt(sampleIndex);
            zReader::Node *entryNode = &entries[sampleIndex + 1];
            zReader::Node *entry = entryNode->value.nodes;

            sample->createGuard = 0;
            sample->replayFields.sampleId = entry[1].value.str;
            sample->replayFields.resourceName = entry[2].value.str;

            int tailIndex = 3;
            if (entry[tailIndex].type == zReader::ZRDR_NODE_FLOAT) {
                sample->replayFields.gain = entry[tailIndex].value.f32;
                ++tailIndex;
            } else {
                sample->replayFields.gain = 1.0f;
            }

            if (strcmp(
                entry[tailIndex].value.str,
                "TRUE"
            ) == 0) {
                sample->replayFields.flags |= 0x01;
            } else {
                sample->replayFields.flags &= ~0x01;
            }
            ++tailIndex;

            if (strcmp(
                entry[tailIndex].value.str,
                "TRUE"
            ) == 0) {
                sample->replayFields.flags |= 0x04;
            } else {
                sample->replayFields.flags &= ~0x04;
            }
            ++tailIndex;

            if (strcmp(
                entry[tailIndex].value.str,
                "TRUE"
            ) == 0) {
                sample->replayFields.flags |= 0x02;
            } else {
                sample->replayFields.flags &= ~0x02;
            }
            ++tailIndex;

            if (entry[0].value.i32 == tailIndex + 2) {
                sample->rangeMin = entry[tailIndex].value.f32;
                sample->rangeMax = entry[tailIndex + 1].value.f32;
            } else {
                sample->rangeMin = 50.0f;
                sample->rangeMax = 400.0f;
            }

            sample->playbackParam3 = 20000.0f;
            sample->playbackParam2 = 90000.0f;
            sample->highVariant.samplesPerSec = 44100;
            sample->highVariant.bitsPerSample = 16;
            sample->highVariant.channelCount = 2;
            sample->medVariant.samplesPerSec = 22050;
            sample->medVariant.bitsPerSample = 16;
            sample->medVariant.channelCount = 1;
            sample->lowVariant.samplesPerSec = 11025;
            sample->lowVariant.bitsPerSample = 8;
            sample->lowVariant.channelCount = 1;
        }
    }

    zReader::Node *groupsNode = zReader_GetNamedNode(
        g_zSnd_ConfigRootNode,
        g_zSndConfig_SoundGroupsKey
    );
    if (groupsNode != 0) {
        zSndGroup_QueuePendingLoadsFromConfigNode(groupsNode);
    }

    return 1;
}

/**
 * Evidence: BN 0x4a1870 expands the named SETS parser in this source file,
 * using g_zSnd_ConfigRootNode and g_zSnd_SearchPathList as namespace state.
 * Purpose: load named sound sets, optional CD tracks, search paths, groups,
 * and speed-of-sound settings from the sound config tree.
 */
extern "C" int __fastcall zSndSystem_InitNamedSetsSyntax(
    zReader::Node *configRootNode
) {
    (void)configRootNode;

    zSndCd::Init(zReader_GetNamedNode(
        g_zSnd_ConfigRootNode,
        g_zSndConfig_CdTracksKey
    ));

    const char *pathText = zReader::ReadNamedString(
        g_zSnd_ConfigRootNode,
        g_zSndConfig_SoundPathKey
    );
    if (pathText != 0) {
        if (g_zSnd_SearchPathList == 0) {
            g_zSnd_SearchPathList = zUtil_ZRDR_CreateSearchPathList(pathText);
        } else {
            zUtil::ZRDR_AddSearchPaths(
                g_zSnd_SearchPathList,
                pathText
            );
        }
    }

    float speedOfSound = 0.0f;
    if (zReader::ReadNamedFloat(
        g_zSnd_ConfigRootNode,
        g_zSndConfig_SpeedOfSoundKey,
        &speedOfSound
    ) != 0) {
        zSnd::SetSpeedOfSoundMps(speedOfSound);
    }

    zReader::Node *setsNode = zReader_GetNamedNode(
        g_zSnd_ConfigRootNode,
        g_zSndConfig_SetsKey
    );
    zReader::Node *sets = setsNode->value.nodes;
    const int setCount = (sets[0].value.i32 - 1) / 2;
    for (int i = 0; i < setCount; ++i) {
        zReader::Node *setNameNode = &sets[(i * 2) + 1];
        zReader::Node *sampleListNode = &sets[(i * 2) + 2];
        const int sampleCount = sampleListNode->value.nodes[0].value.i32 - 1;
        zSndSampleSet *sampleSet =
            (zSndSampleSet *)(::operator new(sizeof(zSndSampleSet)));
        if (sampleSet != 0) {
            sampleSet = sampleSet->RegistryAddEntry(
                setNameNode->value.str,
                sampleCount
            );
        }

        zReader::Node *samples = sampleListNode->value.nodes;
        for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
            zSndSample *sample = sampleSet->GetSampleAt(sampleIndex);
            zReader::Node *sampleNode = &samples[sampleIndex + 1];
            zReader::Node *sampleFields = sampleNode->value.nodes;

            sample->createGuard = 0;
            sample->replayFields.sampleId = sampleFields[1].value.str;
            sample->replayFields.resourceName = sampleFields[2].value.str;

            if (zReader_GetNamedNode(
                sampleNode,
                g_zSndConfig_3dKey
            ) != 0) {
                sample->replayFields.flags |= 0x04;
            } else {
                sample->replayFields.flags &= ~0x04;
            }

            if (zReader_GetNamedNode(
                sampleNode,
                g_zSndConfig_LoopedKey
            ) != 0) {
                sample->replayFields.flags |= 0x01;
            } else {
                sample->replayFields.flags &= ~0x01;
            }

            if (zReader_GetNamedNode(
                sampleNode,
                g_zSndConfig_FrequencyKey
            ) != 0) {
                sample->replayFields.flags |= 0x20;
            } else {
                sample->replayFields.flags &= ~0x20;
            }

            if (zReader_GetNamedNode(
                sampleNode,
                g_zSndConfig_HardwareKey
            ) != 0) {
                sample->replayFields.flags |= 0x40;
            } else {
                sample->replayFields.flags &= ~0x40;
            }

            if (zReader_GetNamedNode(
                sampleNode,
                g_zSndConfig_PurgeableKey
            ) != 0) {
                sample->replayFields.flags |= 0x02;
            } else {
                sample->replayFields.flags &= ~0x02;
            }

            if (zReader_GetNamedNode(
                sampleNode,
                g_zSndConfig_VoiceKey
            ) != 0) {
                sample->replayFields.flags |= 0x10;
            } else {
                sample->replayFields.flags &= ~0x10;
            }

            sample->replayFields.gain = 1.0f;
            zReader::ReadNamedFloat(
                sampleNode,
                g_zSndConfig_VolumeKey,
                &sample->replayFields.gain
            );
            if (sample->replayFields.gain > 1.0f) {
                sample->replayFields.gain = 1.0f;
            } else if (!(sample->replayFields.gain >= 0.0f)) {
                sample->replayFields.gain = 0.0f;
            }

            sample->a3dDistanceScale = 1.0f;
            zReader::ReadNamedFloat(
                sampleNode,
                g_zSndConfig_A3dDistanceKey,
                &sample->a3dDistanceScale
            );

            zReader::Node *rangeNode = zReader_GetNamedNode(
                sampleNode,
                g_zEffectAnim_TokenRange
            );
            if (rangeNode != 0) {
                sample->replayFields.flags |= 0x04;
                zReader::Node *range = rangeNode->value.nodes;
                sample->rangeMin = range[1].value.f32;
                sample->rangeMax = range[2].value.f32;
            } else {
                sample->rangeMin = 50.0f;
                sample->rangeMax = 400.0f;
            }

            zReader::Node *variantNode = zReader_GetNamedNode(
                sampleNode,
                "HIGH"
            );
            if (variantNode == 0) {
                sample->highVariant.sampleName = 0;
                sample->highVariant.samplesPerSec = 44100;
                sample->highVariant.bitsPerSample = 16;
                sample->highVariant.channelCount = 2;
            } else if (variantNode->type == zReader::ZRDR_NODE_STRING) {
                sample->highVariant.sampleName = _strdup(variantNode->value.str);
                sample->highVariant.samplesPerSec = 0;
                sample->highVariant.bitsPerSample = 0;
                sample->highVariant.channelCount = 0;
            } else {
                zReader::Node *format = variantNode->value.nodes;
                sample->highVariant.sampleName = 0;
                sample->highVariant.samplesPerSec = format[1].value.i32;
                sample->highVariant.bitsPerSample = format[2].value.i32;
                sample->highVariant.channelCount = format[3].value.i32;
            }

            variantNode = zReader_GetNamedNode(
                sampleNode,
                g_zSndConfig_QualityMedToken
            );
            if (variantNode == 0) {
                sample->medVariant.sampleName = 0;
                sample->medVariant.samplesPerSec = 22050;
                sample->medVariant.bitsPerSample = 16;
                sample->medVariant.channelCount = 1;
            } else if (variantNode->type == zReader::ZRDR_NODE_STRING) {
                sample->medVariant.sampleName = _strdup(variantNode->value.str);
                sample->medVariant.samplesPerSec = 0;
                sample->medVariant.bitsPerSample = 0;
                sample->medVariant.channelCount = 0;
            } else {
                zReader::Node *format = variantNode->value.nodes;
                sample->medVariant.sampleName = 0;
                sample->medVariant.samplesPerSec = format[1].value.i32;
                sample->medVariant.bitsPerSample = format[2].value.i32;
                sample->medVariant.channelCount = format[3].value.i32;
            }

            variantNode = zReader_GetNamedNode(
                sampleNode,
                "LOW"
            );
            if (variantNode == 0) {
                sample->lowVariant.sampleName = 0;
                sample->lowVariant.samplesPerSec = 11025;
                sample->lowVariant.bitsPerSample = 8;
                sample->lowVariant.channelCount = 1;
            } else if (variantNode->type == zReader::ZRDR_NODE_STRING) {
                sample->lowVariant.sampleName = _strdup(variantNode->value.str);
                sample->lowVariant.samplesPerSec = 0;
                sample->lowVariant.bitsPerSample = 0;
                sample->lowVariant.channelCount = 0;
            } else {
                zReader::Node *format = variantNode->value.nodes;
                sample->lowVariant.sampleName = 0;
                sample->lowVariant.samplesPerSec = format[1].value.i32;
                sample->lowVariant.bitsPerSample = format[2].value.i32;
                sample->lowVariant.channelCount = format[3].value.i32;
            }

            sample->playbackParam3 = 20000.0f;
            sample->playbackParam2 = 90000.0f;
        }
    }

    zReader::Node *groupsNode = zReader_GetNamedNode(
        g_zSnd_ConfigRootNode,
        g_zSndConfig_SoundGroupsKey
    );
    if (groupsNode != 0) {
        zSndGroup_QueuePendingLoadsFromConfigNode(groupsNode);
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.zsndbackend-inita3d
 * @recoil-artifact defines .text recoil:function:0x4a1d10: zSndBackend_InitA3D.
 *
 * Purpose: create the A3D provider object, query geometry/listener interfaces,
 * configure output mode, and validate buffer creation.
 */
extern "C" int zSndBackend_InitA3D() {
    if (CoInitialize(0) < 0) {
        return 0;
    }

    HRESULT a3dError = CoCreateInstance(
        kCLSID_A3DApi,
        0,
        CLSCTX_INPROC_SERVER,
        kIID_IA3d3,
        (void **)(&g_zSnd_BackendDevice)
    );
    if (a3dError < 0) {
        if (a3dError == CLASS_E_NOAGGREGATION) {
            printf(g_zSnd_A3DInitError_AggregateMsg);
            return 0;
        }

        if (a3dError == REGDB_E_CLASSNOTREG) {
            printf(g_zSnd_A3DInitError_NotRegisteredMsg);
            return 0;
        }

        printf(g_zSnd_A3DInitError_UnknownMsg);
        return 0;
    }

    zA3dProviderDevice *api = (zA3dProviderDevice *)(g_zSnd_BackendDevice);
    api->Init(
        0,
        0x28,
        0x0c
    );
    api->SetCooperativeLevel(
        (HWND)(g_zSnd_WindowHandle),
        1
    );

    a3dError =
        api->QueryInterface(
            kIID_IA3dGeom,
            (void **)(&g_zSnd_BackendAuxHandleOrConfig)
        );
    if (a3dError != 0) {
        return zSnd::ReportA3DError(
            a3dError,
            g_zSnd_SourceFile_zsnd_init_cpp,
            0x245
        );
    }

    a3dError =
        api->QueryInterface(
            kIID_IA3dListener,
            (void **)(&g_zSnd_BackendListenerHandle)
        );
    if (a3dError != 0) {
        return zSnd::ReportA3DError(
            a3dError,
            g_zSnd_SourceFile_zsnd_init_cpp,
            0x24b
        );
    }

    a3dError = api->SetResourceManagerMode(
        2
    );
    if (a3dError != 0) {
        return zSnd::ReportA3DError(
            a3dError,
            g_zSnd_SourceFile_zsnd_init_cpp,
            0x24e
        );
    }

    api->Clear();

    zA3dProviderSource *outBuffer = 0;
    api->NewSource(
        0,
        &outBuffer
    );
    if (outBuffer != 0) {
        outBuffer->Release();
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.zsndbackend-initdirectsound
 * @recoil-artifact defines .text recoil:function:0x4a1e50: zSndBackend_InitDirectSound.
 *
 * Purpose: create the DirectSound device, set cooperative level, cache device
 * caps, and create the primary listener buffer.
 */
extern "C" int zSndBackend_InitDirectSound() {
    HRESULT directSoundError = DirectSoundCreate(
        0,
        &g_zSnd_BackendDevice,
        0
    );
    if (directSoundError != DS_OK) {
        return zSnd::ReportDirectSoundError(
            directSoundError,
            g_zSnd_SourceFile_zsnd_init_cpp,
            0x26a
        );
    }

    directSoundError =
        g_zSnd_BackendDevice->SetCooperativeLevel(
            (HWND)(g_zSnd_WindowHandle),
            DSSCL_NORMAL
        );
    if (directSoundError != DS_OK) {
        return zSnd::ReportDirectSoundError(
            directSoundError,
            g_zSnd_SourceFile_zsnd_init_cpp,
            0x26d
        );
    }

    g_zSnd_BackendAuxHandleOrConfig.dwSize = sizeof(DSCAPS);
    directSoundError = g_zSnd_BackendDevice->GetCaps(&g_zSnd_BackendAuxHandleOrConfig);
    if (directSoundError != DS_OK) {
        return zSnd::ReportDirectSoundError(
            directSoundError,
            g_zSnd_SourceFile_zsnd_init_cpp,
            0x271
        );
    }

    DSBUFFERDESC desc = {0};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN;
    directSoundError =
        g_zSnd_BackendDevice->CreateSoundBuffer(
            &desc,
            &g_zSnd_BackendListenerHandle,
            0
        );
    if (directSoundError != DS_OK) {
        return zSnd::ReportDirectSoundError(
            directSoundError,
            g_zSnd_SourceFile_zsnd_init_cpp,
            0x28d
        );
    }

    return 1;
}


namespace zSndBackend {
namespace {
/**
 * Original static helper observed in caller 0x4a1f40.
 *
 * Purpose: release an A3D/COM-style provider object and clear the stored
 * pointer.
 */
void ReleaseUnknown(
    void *&object
) {
    if (object != 0) {
        ((IUnknown *)object)->Release();
        object = 0;
    }
}
} // namespace

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-init.shutdown
 * @recoil-artifact defines .text recoil:function:0x4a1f40: zSndBackend::Shutdown.
 *
 * Purpose: shut down CD, streaming, sample-set, and backend provider state for
 * the active sound system.
 */
int Shutdown() {
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
