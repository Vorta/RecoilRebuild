#include "GameZRecoil/zSound/zsnd.h"

#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zReader/zreader.h"

#include <mmsystem.h>
#include <windows.h>

#include "recoil/recoil_types.h"
#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct zSndCdTrackState {
    int track;
    int minute;
    int second;
};

/**
 * Purpose: Stores the pre-initialization CD track-list count reset by
 * zSnd_PreInitializeRuntimeState; distinct from the static CD track-list
 * lifecycle count.
 */
extern "C" int g_zSndCdTrackListCount = 0;
extern "C" int g_zSndCdLastPlayMode = 0;
extern "C" int g_zSndCdFlags = 0;
extern "C" int g_zSndCdDeviceId = 0;
extern "C" int g_zSndCdAuxDeviceId = 0;
extern "C" unsigned short g_zSndCdAuxVolumePrimary = 0;
extern "C" unsigned short g_zSndCdAuxVolumeSecondary = 0;
extern "C" int g_zSndCdTrackCountCached = 0;
extern "C" int g_zSndCdDiscLengthMinute = 0;
extern "C" int g_zSndCdDiscLengthSecond = 0;
extern "C" zSndCdTrackState g_zSndCdPlayFrom = {0};
extern "C" zSndCdTrackState g_zSndCdCurrent = {0};
extern "C" zSndCdTrackState g_zSndCdPlayTo = {0};
extern "C" int g_zSnd_IsInitialized = 0;
extern "C" int g_zSnd_ActiveBackend = 0;
extern "C" unsigned int g_zSnd_WindowHandle = 0;
/**
 * Purpose: Stores the archive-bank selector used by sound-bank loading and
 * CZRecoilFrame archive-bank menu state.
 */
extern "C" int g_zSnd_UseArchiveBanksFlag = 1;

#define g_zSndCdPlayFromTrack (g_zSndCdPlayFrom.track)
#define g_zSndCdPlayFromMinute (g_zSndCdPlayFrom.minute)
#define g_zSndCdPlayFromSecond (g_zSndCdPlayFrom.second)
#define g_zSndCdCurrentTrack (g_zSndCdCurrent.track)
#define g_zSndCdCurrentMinute (g_zSndCdCurrent.minute)
#define g_zSndCdCurrentSecond (g_zSndCdCurrent.second)
#define g_zSndCdPlayToTrack (g_zSndCdPlayTo.track)
#define g_zSndCdPlayToMinute (g_zSndCdPlayTo.minute)
#define g_zSndCdPlayToSecond (g_zSndCdPlayTo.second)

namespace {
const int ZSND_CD_FLAG_STEREO_AUX = 1;
const int ZSND_CD_FLAG_READY = 2;
const char kZSndCdSourceFile[] = "D:\\Proj\\GameZRecoil\\zSound\\zsnd_cd.cpp";
std::list<zSndCdTrackEntry *> g_zSndCdTrackList;
} // namespace

namespace zSnd {
} // namespace zSnd

namespace zSndCd {
int __cdecl ResetTrackState();
int __fastcall ApplyPlaybackMode(int playbackMode);
int __fastcall PlayTrack(int trackIndex);
int __cdecl Shutdown();

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-cd.init
 * @recoil-artifact defines .text recoil:function:0x4a20d0: zSndCd::Init.
 * Purpose: Open the MCI CD device, cache track metadata, and build the CD track list.
 */
RECOIL_NO_GS int __fastcall Init(
    zReader::Node *cdTracksNode
) {
    if ((g_zSndCdFlags & ZSND_CD_FLAG_READY) != 0) {
        return 1;
    }

    MCI_OPEN_PARMSA openParms = {0};
    openParms.lpstrDeviceType = "cdaudio";
    DWORD mciError = mciSendCommandA(
        0,
        MCI_OPEN,
        MCI_OPEN_TYPE,
        (DWORD_PTR)(&openParms)
    );
    if (mciError != 0) {
        return zSnd::ReportMciError(
            mciError,
            kZSndCdSourceFile,
            0x43
        );
    }

    g_zSndCdDeviceId = (g_zSndCdDeviceId & 0xffff0000) | (unsigned short)(openParms.wDeviceID);

    MCI_STATUS_PARMS statusParms = {0};
    statusParms.dwItem = 5;
    mciError = mciSendCommandA(
        (MCIDEVICEID)(g_zSndCdDeviceId & 0xffff),
        MCI_STATUS,
        MCI_WAIT | MCI_STATUS_ITEM,
        (DWORD_PTR)(&statusParms)
    );
    if (mciError != 0) {
        return zSnd::ReportMciError(
            mciError,
            kZSndCdSourceFile,
            0x4d
        );
    }

    if (statusParms.dwReturn == 0) {
        Shutdown();
        return 0;
    }

    MCI_SET_PARMS setParms = {0};
    setParms.dwTimeFormat = MCI_FORMAT_TMSF;
    mciError = mciSendCommandA(
        (MCIDEVICEID)(g_zSndCdDeviceId & 0xffff),
        MCI_SET,
        MCI_WAIT | MCI_SET_TIME_FORMAT,
        (DWORD_PTR)(&setParms)
    );
    if (mciError != 0) {
        return zSnd::ReportMciError(
            mciError,
            kZSndCdSourceFile,
            0x5d
        );
    }

    memset(
        &statusParms,
        0,
        sizeof(statusParms)
    );
    statusParms.dwItem = 3;
    mciError = mciSendCommandA(
        (MCIDEVICEID)(g_zSndCdDeviceId & 0xffff),
        MCI_STATUS,
        MCI_WAIT | MCI_STATUS_ITEM,
        (DWORD_PTR)(&statusParms)
    );
    if (mciError != 0) {
        return zSnd::ReportMciError(
            mciError,
            kZSndCdSourceFile,
            0x66
        );
    }

    g_zSndCdTrackCountCached = (int)(statusParms.dwReturn);
    statusParms.dwItem = 1;
    statusParms.dwTrack = 0;
    mciError = mciSendCommandA(
        (MCIDEVICEID)(g_zSndCdDeviceId & 0xffff),
        MCI_STATUS,
        MCI_WAIT | MCI_STATUS_ITEM,
        (DWORD_PTR)(&statusParms)
    );
    if (mciError != 0) {
        return zSnd::ReportMciError(
            mciError,
            kZSndCdSourceFile,
            0x70
        );
    }

    const UINT auxCount = auxGetNumDevs();
    for (UINT deviceId = 0; deviceId < auxCount; ++deviceId) {
        AUXCAPSA caps = {0};
        if (auxGetDevCapsA(deviceId, &caps, sizeof(caps)) == 0 &&
            caps.wTechnology == AUXCAPS_CDAUDIO && (caps.dwSupport & AUXCAPS_VOLUME) != 0) {
            if ((caps.dwSupport & AUXCAPS_LRVOLUME) != 0) {
                g_zSndCdFlags |= ZSND_CD_FLAG_STEREO_AUX;
            }
            g_zSndCdAuxDeviceId = (int)(deviceId);
            break;
        }
    }

    if (g_zSndCdAuxDeviceId != -1) {
        DWORD volume = 0;
        if (auxGetVolume(
            (UINT)(g_zSndCdAuxDeviceId),
            &volume
        ) == 0) {
            g_zSndCdAuxVolumeSecondary = (unsigned short)(volume & 0xffff);
            g_zSndCdAuxVolumePrimary = (unsigned short)((volume >> 16) & 0xffff);
        }
    }

    const DWORD discLength = statusParms.dwReturn;
    g_zSndCdDiscLengthMinute = (int)((discLength >> 8) & 0xff);
    g_zSndCdDiscLengthSecond = (int)((discLength >> 16) & 0xff);
    ResetTrackState();
    g_zSndCdFlags |= ZSND_CD_FLAG_READY;

    if (cdTracksNode != 0) {
        zReader::Node *tracks = cdTracksNode->value.nodes;
        for (int i = 1; i < cdTracksNode->value.nodes[0].value.i32; ++i) {
            zReader::Node *trackNode = &tracks[i];
            if (trackNode->type != zReader::ZRDR_NODE_ARRAY) {
                continue;
            }

            zReader::Node *trackConfig = trackNode->value.nodes;
            zSndCdTrackEntry *entry =
                (zSndCdTrackEntry *)(::operator new(sizeof(zSndCdTrackEntry)));
            if (entry != 0) {
                entry->trackNumber = trackConfig[2].value.i32;
                entry->archiveName = _strdup(trackConfig[1].value.str);
            }

            g_zSndCdTrackList.push_back(entry);
        }
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-cd.resettrackstate
 * @recoil-artifact defines .text recoil:function:0x4a2490: zSndCd::ResetTrackState.
 * Purpose: Reset cached CD play-from/current/play-to positions to track one.
 */
int __cdecl ResetTrackState() {
    zSndCdTrackState state = {1, 0, 0};
    g_zSndCdPlayFrom = state;
    g_zSndCdCurrent = state;
    g_zSndCdPlayTo = state;
    return state.track;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-cd.shutdown
 * @recoil-artifact defines .text recoil:function:0x4a24d0: zSndCd::Shutdown.
 * Purpose: stop CD playback, close the MCI CD device, clear ready state, and
 * release configured track-list entries.
 */
int __cdecl Shutdown() {
    Stop();

    if ((g_zSndCdDeviceId & 0xffff) != 0) {
        MCI_GENERIC_PARMS closeParms = {0};
        mciSendCommandA(
            (MCIDEVICEID)(g_zSndCdDeviceId & 0xffff),
            MCI_CLOSE,
            MCI_WAIT,
            (DWORD_PTR)(&closeParms)
        );
        g_zSndCdDeviceId &= 0xffff0000;
    }

    g_zSndCdFlags &= ~ZSND_CD_FLAG_READY;

    if (g_zSndCdTrackList.empty()) {
        return 1;
    }

    std::list<zSndCdTrackEntry *>::iterator entryIt =
        g_zSndCdTrackList.begin();
    while (entryIt != g_zSndCdTrackList.end()) {
        zSndCdTrackEntry *entry = *entryIt;
        free(entry->archiveName);
        entry->archiveName = 0;
        ::operator delete(entry);
        ++entryIt;
    }
    g_zSndCdTrackList.clear();

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-cd.playtrackwithmode
 * @recoil-artifact defines .text recoil:function:0x4a25e0: zSndCd::PlayTrackWithMode.
 * Purpose: Start a CD track and then apply the requested playback mode.
 */
int __fastcall PlayTrackWithMode(
    int trackIndex,
    int playbackMode
) {
    int result = 0;
    const int mode = playbackMode;
    if (PlayTrack(trackIndex) != 0) {
        result = ApplyPlaybackMode(mode);
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-cd.applyplaybackmode
 * @recoil-artifact defines .text recoil:function:0x4a2600: zSndCd::ApplyPlaybackMode.
 * Purpose: Apply the requested CD playback mode and issue the MCI play command.
 */
RECOIL_NO_GS int __fastcall ApplyPlaybackMode(
    int playbackMode
) {
    if ((g_zSndCdFlags & ZSND_CD_FLAG_READY) == 0) {
        return 0;
    }

    register int currentTrack = g_zSndCdCurrentTrack;
    register int trackCount = g_zSndCdTrackCountCached;
    register int playToTrack;
    if (playbackMode == 2) {
        playToTrack = currentTrack + 1;
    } else {
        playToTrack = trackCount + 1;
        if (playbackMode == 5) {
            playToTrack = currentTrack + 1;
        }
    }

    g_zSndCdPlayToTrack = playToTrack;

    MCI_PLAY_PARMS playParms;
    playParms.dwFrom = (DWORD)(currentTrack & 0xff);
    playParms.dwTo = (DWORD)(playToTrack & 0xff);
    playParms.dwCallback = g_zSnd_WindowHandle;

    DWORD playFlags = 0x5;
    if ((unsigned int)(playToTrack) <= (unsigned int)(trackCount)) {
        playFlags = 0x0d;
    }

    const DWORD mciError = mciSendCommandA(
        (MCIDEVICEID)(g_zSndCdDeviceId & 0xffff),
        0x806,
        playFlags,
        (DWORD_PTR)(&playParms)
    );
    if (mciError != 0) {
        return zSnd::ReportMciError(
            mciError,
            kZSndCdSourceFile,
            0xf1
        );
    }

    g_zSndCdLastPlayMode = playbackMode;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-cd.onmcinotify
 * @recoil-artifact defines .text recoil:function:0x4a26b0: zSndCd::OnMciNotify.
 * Purpose: Restart looping CD playback when the MCI notify callback completes.
 */
void __fastcall OnMciNotify(
    unsigned int wParam,
    unsigned int lParam
) {
    if ((g_zSndCdFlags & ZSND_CD_FLAG_READY) == 0 || g_zSndCdLastPlayMode != 5 ||
        lParam != (unsigned int)(g_zSndCdDeviceId & 0xffff) || wParam != 1) {
        return;
    }

    PlayTrackWithMode(
        g_zSndCdCurrentTrack,
        5
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-cd.stop
 * @recoil-artifact defines .text recoil:function:0x4a26f0: zSndCd::Stop.
 * Purpose: stop the current MCI CD playback and reset the cached track state.
 */
RECOIL_NO_GS int __cdecl Stop() {
    if ((g_zSndCdFlags & ZSND_CD_FLAG_READY) == 0) {
        return 0;
    }

    MCI_GENERIC_PARMS stopParms;
    const DWORD mciError = mciSendCommandA(
        (MCIDEVICEID)(g_zSndCdDeviceId & 0xffff),
        0x808,
        0x02,
        (DWORD_PTR)(&stopParms)
    );
    if (mciError != 0) {
        return zSnd::ReportMciError(
            mciError,
            kZSndCdSourceFile,
            0x10e
        );
    }

    g_zSndCdLastPlayMode = 0;
    ResetTrackState();
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-cd.playtrack
 * @recoil-artifact defines .text recoil:function:0x4a2750: zSndCd::PlayTrack.
 * Purpose: Seek to a CD track and reset cached playback state for that track.
 */
RECOIL_NO_GS int __fastcall PlayTrack(
    int trackIndex
) {
    if ((g_zSndCdFlags & ZSND_CD_FLAG_READY) == 0) {
        return 0;
    }

    MCI_SEEK_PARMS seekParms;
    seekParms.dwTo = (DWORD)(trackIndex & 0xff);

    const DWORD mciError = mciSendCommandA(
        (MCIDEVICEID)(g_zSndCdDeviceId & 0xffff),
        0x807,
        0x0a,
        (DWORD_PTR)(&seekParms)
    );
    if (mciError != 0) {
        return zSnd::ReportMciError(
            mciError,
            kZSndCdSourceFile,
            0x16e
        );
    }

    ResetTrackState();
    g_zSndCdCurrentTrack = trackIndex;
    g_zSndCdPlayToTrack = trackIndex;
    g_zSndCdPlayFromTrack = trackIndex;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-cd.isstereoauxenabled
 * @recoil-artifact defines .text recoil:function:0x4a27d0: zSndCd::IsStereoAuxEnabled.
 * Purpose: report whether CD audio has an initialized stereo AUX mixer.
 */
int __cdecl IsStereoAuxEnabled() {
    if ((g_zSndCdFlags & ZSND_CD_FLAG_READY) == 0) {
        return 0;
    }

    if (g_zSndCdAuxDeviceId == -1) {
        return 0;
    }

    return g_zSndCdFlags & ZSND_CD_FLAG_STEREO_AUX;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-cd.getvolume
 * @recoil-artifact defines .text recoil:function:0x4a27f0: zSndCd::GetVolume.
 * Purpose: read the AUX mixer volume into mono or stereo output channels.
 */
int __fastcall GetVolume(
    unsigned short *primaryVolumeOut,
    unsigned short *secondaryVolumeOut
) {
    if ((g_zSndCdFlags & ZSND_CD_FLAG_READY) == 0) {
        return 0;
    }

    const int stereoAuxEnabled = IsStereoAuxEnabled();
    DWORD volume = 0;
    const DWORD mciError = auxGetVolume(
        (UINT)(g_zSndCdAuxDeviceId),
        &volume
    );
    if (mciError != 0) {
        zSnd::ReportMciError(
            mciError,
            kZSndCdSourceFile,
            0x194
        );
        return 0;
    }

    const unsigned short primaryVolume = (unsigned short)(volume & 0xffff);
    g_zSndCdAuxVolumePrimary = primaryVolume;
    if (stereoAuxEnabled == 0) {
        g_zSndCdAuxVolumeSecondary = primaryVolume;
        *secondaryVolumeOut = primaryVolume;
        *primaryVolumeOut = primaryVolume;
        return 1;
    }

    *primaryVolumeOut = primaryVolume;
    const unsigned short secondaryVolume = (unsigned short)((volume >> 16) & 0xffff);
    g_zSndCdAuxVolumeSecondary = secondaryVolume;
    *secondaryVolumeOut = secondaryVolume;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-cd.setvolume
 * @recoil-artifact defines .text recoil:function:0x4a2880: zSndCd::SetVolume.
 * Purpose: write mono or stereo AUX mixer volume from requested channel values.
 */
int __fastcall SetVolume(
    unsigned short primaryVolume,
    unsigned short secondaryVolume
) {
    if ((g_zSndCdFlags & ZSND_CD_FLAG_READY) == 0) {
        return 0;
    }

    DWORD volume;
    if (IsStereoAuxEnabled() == 0) {
        volume = (DWORD)(((int)(primaryVolume) + (int)(secondaryVolume)) / 2);
    } else {
        volume = ((DWORD)(secondaryVolume) << 16) | (DWORD)(primaryVolume);
    }

    const DWORD mciError = auxSetVolume(
        (UINT)(g_zSndCdAuxDeviceId),
        volume
    );
    if (mciError != 0) {
        zSnd::ReportMciError(
            mciError,
            kZSndCdSourceFile,
            0x1b2
        );
        return 0;
    }

    g_zSndCdAuxVolumePrimary = primaryVolume;
    g_zSndCdAuxVolumeSecondary = secondaryVolume;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-cd.gettrackcount
 * @recoil-artifact defines .text recoil:function:0x4a2930: zSndCd::GetTrackCount.
 * Purpose: Return the cached number of CD tracks when the CD device is ready.
 */
int __cdecl GetTrackCount() {
    if ((g_zSndCdFlags & ZSND_CD_FLAG_READY) == 0) {
        return 0;
    }

    return g_zSndCdTrackCountCached;
}
} // namespace zSndCd
