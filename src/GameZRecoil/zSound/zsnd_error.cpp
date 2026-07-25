#include "zsnd.h"

#include "GameZRecoil/zError/zerr.h"

#include "recoil/recoil_types.h"
#include <mmsystem.h>
#include <stdio.h>

extern "C" char g_Player_MasterTypeName_Unknown[0x08];
extern char g_zSnd_A3DErrorFmt[];
extern char g_zSnd_DirectSoundErrorFmt[];
extern char g_zSnd_DsErrorName_BufferLost[];
extern char g_zSnd_DsErrorName_OtherAppHasPrio[];
extern char g_zSnd_DsErrorName_AlreadyInitialized[];
extern char g_zSnd_DsErrorName_NoDriver[];
extern char g_zSnd_DsErrorName_BadFormat[];
extern char g_zSnd_DsErrorName_PrioLevelNeeded[];
extern char g_zSnd_DsErrorName_InvalidCall[];
extern char g_zSnd_DsErrorName_ControlUnavail[];
extern char g_zSnd_DsErrorName_Allocated[];
extern char g_zSnd_DsErrorName_InvalidParam[];
extern char g_zSnd_DsErrorName_OutOfMemory[];
extern char g_zSnd_DsErrorName_NoAggregation[];
extern char g_zSnd_DsErrorName_Generic[];
extern char g_zSnd_DsErrorName_Unsupported[];

namespace zSnd {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-error-reportmcierror
 * @recoil-artifact defines .text recoil:function:0x4a3ea0: zSnd::ReportMciError.
 * Purpose: print a formatted MCI error message for a source-file line.
 */
RECOIL_NO_GS int __fastcall ReportMciError(
    unsigned int mciError,
    const char *sourceFile,
    int lineNumber
) {
    char errorText[0x100];
    mciGetErrorStringA(
        mciError,
        errorText,
        sizeof(errorText)
    );
    fprintf(
        stderr,
        "%s(%d): MCIError [%s]\n",
        sourceFile,
        lineNumber,
        errorText
    );
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-error-reporta3derror
 * @recoil-artifact defines .text recoil:function:0x4a3ef0: zSnd::ReportA3DError.
 *
 * Purpose: translate an A3D provider error code into the original diagnostic
 * text and report it through zError.
 */
int __fastcall ReportA3DError(
    int a3dError,
    const char *sourceFile,
    int sourceLine
) {
    char errorNameStorage[0x100];
    if (a3dError <= 0) {
        if (a3dError != 0) {
            switch ((unsigned int)(a3dError)) {
            case 0x80040001u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_MEMORY_ALLOCATION\t"
                );
                break;
            case 0x80040002u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_FAILED_CREATE_PRIMARY_BUFFER\t"
                );
                break;
            case 0x80040003u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_FAILED_CREATE_SECONDARY_BUFFER\t"
                );
                break;
            case 0x80040004u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_FAILED_INIT_A3D_DRIVER\t"
                );
                break;
            case 0x80040005u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_FAILED_QUERY_DIRECTSOUND\t"
                );
                break;
            case 0x80040006u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_FAILED_QUERY_A3D3\t"
                );
                break;
            case 0x80040007u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_FAILED_INIT_A3D3\t"
                );
                break;
            case 0x80040008u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_FAILED_QUERY_A3D2\t"
                );
                break;
            case 0x80040009u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_FAILED_FILE_OPEN\t"
                );
                break;
            case 0x8004000au:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_FAILED_CREATE_SOUNDBUFFER\t"
                );
                break;
            case 0x8004000bu:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_FAILED_QUERY_3DINTERFACE\t"
                );
                break;
            case 0x8004000cu:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_FAILED_LOCK_BUFFER\t"
                );
                break;
            case 0x8004000du:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_FAILED_UNLOCK_BUFFER\t"
                );
                break;
            case 0x8004000eu:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_UNRECOGNIZED_FORMAT\t"
                );
                break;
            case 0x8004000fu:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_NO_WAVE_DATA\t"
                );
                break;
            case 0x80040010u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_UNKNOWN_PLAYMODE\t"
                );
                break;
            case 0x80040011u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_FAILED_PLAY\t"
                );
                break;
            case 0x80040012u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_FAILED_STOP\t"
                );
                break;
            case 0x80040013u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_NEEDS_FORMAT_INFORMATION\t"
                );
                break;
            case 0x80040014u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_FAILED_ALLOCATE_WAVEDATA\t"
                );
                break;
            case 0x80040015u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_NOT_VALID_SOURCE\t"
                );
                break;
            case 0x80040016u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_FAILED_DUPLICATION\t"
                );
                break;
            case 0x80040017u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_FAILED_INIT\t"
                );
                break;
            case 0x80040018u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_FAILED_SETCOOPERATIVE_LEVEL\t"
                );
                break;
            case 0x80040019u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_FAILED_INIT_QUERIED_INTERFACE\t"
                );
                break;
            case 0x8004001au:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_GEOMETRY_INPUT_OUTSIDE_BEGIN_END_BLOCK\t"
                );
                break;
            case 0x8004001bu:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_INVALID_NORMAL\t"
                );
                break;
            case 0x8004001cu:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_END_BEFORE_VALID_BEGIN_BLOCK\t"
                );
                break;
            case 0x8004001du:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_INVALID_BEGIN_MODE\t"
                );
                break;
            case 0x8004001eu:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_INVALID_ARGUMENT\t"
                );
                break;
            case 0x8004001fu:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_INVALID_INDEX\t"
                );
                break;
            case 0x80040020u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_INVALID_VERTEX_INDEX\t"
                );
                break;
            case 0x80040021u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_INVALID_PRIMITIVE_INDEX\t"
                );
                break;
            case 0x80040022u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_MIXING_2D_AND_3D_MODES\t"
                );
                break;
            case 0x80040023u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_2DWALL_REQUIRES_EXACTLY_ONE_LINE\t"
                );
                break;
            case 0x80040024u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_NO_PRIMITIVES_DEFINED\t"
                );
                break;
            case 0x80040025u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_PRIMITIVES_NON_PLANAR\t"
                );
                break;
            case 0x80040026u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_PRIMITIVES_OVERLAPPING\t"
                );
                break;
            case 0x80040027u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_PRIMITIVES_NOT_ADJACENT\t"
                );
                break;
            case 0x80040028u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_OBJECT_NOT_FOUND\t"
                );
                break;
            case 0x80040029u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_ROOM_HAS_NO_SHELL_WALLS\t"
                );
                break;
            case 0x8004002au:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_WALLS_DO_NOT_ENCLOSE_ROOM\t"
                );
                break;
            case 0x8004002bu:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_INVALID_WALL\t"
                );
                break;
            case 0x8004002cu:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_ROOM_HAS_LESS_THAN_4SHELL_WALLS\t"
                );
                break;
            case 0x8004002du:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_ROOM_HAS_LESS_THAN_3UNIQUE_NORMALS\t"
                );
                break;
            case 0x8004002eu:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_INTERSECTING_WALL_EDGES\t"
                );
                break;
            case 0x8004002fu:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_INVALID_ROOM\t"
                );
                break;
            case 0x80040030u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_SCENE_HAS_ROOMS_INSIDE_ANOTHER_ROOMS\t"
                );
                break;
            case 0x80040031u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_SCENE_HAS_OVERLAPPING_STATIC_ROOMS\t"
                );
                break;
            case 0x80040032u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_DYNAMIC_OBJ_UNSUPPORTED\t"
                );
                break;
            case 0x80040033u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_DIR_AND_UP_VECTORS_NOT_PERPENDICULAR\t"
                );
                break;
            case 0x80040034u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_INVALID_ROOM_INDEX\t"
                );
                break;
            case 0x80040035u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_INVALID_WALL_INDEX\t"
                );
                break;
            case 0x80040036u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_SCENE_INVALID\t"
                );
                break;
            case 0x80040037u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_UNIMPLEMENTED_FUNCTION\t"
                );
                break;
            case 0x80040038u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_NO_ROOMS_IN_SCENE\t"
                );
                break;
            case 0x80040039u:
                sprintf(
                    errorNameStorage,
                    "\tA3DERROR_2D_GEOMETRY_UNIMPLEMENTED\t"
                );
                break;
            default:
                goto reportUnknownA3D;
            }

            goto reportA3D;
        }
        return 1;
    }

reportUnknownA3D:
    sprintf(
        errorNameStorage,
        g_Player_MasterTypeName_Unknown
    );

reportA3D:
    zError::ReportOld(
        0x400,
        sourceFile,
        sourceLine,
        g_zSnd_A3DErrorFmt,
        errorNameStorage
    );
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-error-reportdirectsounderror
 * @recoil-artifact defines .text recoil:function:0x4a4330: zSnd::ReportDirectSoundError.
 *
 * Purpose: translate a DirectSound provider error code into the original
 * diagnostic text and report it through zError.
 */
int __fastcall ReportDirectSoundError(
    int directSoundError,
    const char *sourceFile,
    int sourceLine
) {
    char errorNameStorage[0x100];
    switch ((HRESULT)(directSoundError)) {
    case DSERR_GENERIC:
        sprintf(
            errorNameStorage,
            g_zSnd_DsErrorName_Generic
        );
        break;
    case DSERR_UNSUPPORTED:
        sprintf(
            errorNameStorage,
            g_zSnd_DsErrorName_Unsupported
        );
        break;
    case DSERR_OUTOFMEMORY:
        sprintf(
            errorNameStorage,
            g_zSnd_DsErrorName_OutOfMemory
        );
        break;
    case DSERR_NOAGGREGATION:
        sprintf(
            errorNameStorage,
            g_zSnd_DsErrorName_NoAggregation
        );
        break;
    case DSERR_INVALIDPARAM:
        sprintf(
            errorNameStorage,
            g_zSnd_DsErrorName_InvalidParam
        );
        break;
    case DSERR_ALLOCATED:
        sprintf(
            errorNameStorage,
            g_zSnd_DsErrorName_Allocated
        );
        break;
    case DSERR_CONTROLUNAVAIL:
        sprintf(
            errorNameStorage,
            g_zSnd_DsErrorName_ControlUnavail
        );
        break;
    case DSERR_INVALIDCALL:
        sprintf(
            errorNameStorage,
            g_zSnd_DsErrorName_InvalidCall
        );
        break;
    case DSERR_PRIOLEVELNEEDED:
        sprintf(
            errorNameStorage,
            g_zSnd_DsErrorName_PrioLevelNeeded
        );
        break;
    case DSERR_BADFORMAT:
        sprintf(
            errorNameStorage,
            g_zSnd_DsErrorName_BadFormat
        );
        break;
    case DSERR_NODRIVER:
        sprintf(
            errorNameStorage,
            g_zSnd_DsErrorName_NoDriver
        );
        break;
    case DSERR_ALREADYINITIALIZED:
        sprintf(
            errorNameStorage,
            g_zSnd_DsErrorName_AlreadyInitialized
        );
        break;
    case DSERR_BUFFERLOST:
        sprintf(
            errorNameStorage,
            g_zSnd_DsErrorName_BufferLost
        );
        break;
    case DSERR_OTHERAPPHASPRIO:
        sprintf(
            errorNameStorage,
            g_zSnd_DsErrorName_OtherAppHasPrio
        );
        break;
    case DS_OK:
        return 1;
    default:
        sprintf(
            errorNameStorage,
            g_Player_MasterTypeName_Unknown
        );
        break;
    }

    zError::ReportOld(
        0x400,
        sourceFile,
        sourceLine,
        g_zSnd_DirectSoundErrorFmt,
        errorNameStorage
    );
    return 0;
}
} // namespace zSnd
