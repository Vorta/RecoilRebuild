#include "GameZRecoil/zSound/zsnd.h"

#include "GameZRecoil/zReader/zreader.h"

#include <mmsystem.h>
#include <windows.h>

#include "recoil/recoil_types.h"
#include <algorithm>
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
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-cd.g-zsndcdstate
 * @recoil-artifact defines .data recoil:data:0x56b318: g_zSndCdState.trackListCount.
 * @recoil-artifact defines .data recoil:data:0x56b31c: g_zSndCdState.lastPlayMode.
 * @recoil-artifact defines .data recoil:data:0x56b324: g_zSndCdState.flags.
 * @recoil-artifact defines .data recoil:data:0x56b328: g_zSndCdState.deviceId.
 * @recoil-artifact defines .data recoil:data:0x56b32c: g_zSndCdState.auxDeviceId.
 * @recoil-artifact defines .data recoil:data:0x56b330: g_zSndCdState.auxVolumePrimary.
 * @recoil-artifact defines .data recoil:data:0x56b332: g_zSndCdState.auxVolumeSecondary.
 * @recoil-artifact defines .data recoil:data:0x56b334: g_zSndCdState.trackCountCached.
 * @recoil-artifact defines .data recoil:data:0x56b338: g_zSndCdState.discLengthMinute.
 * @recoil-artifact defines .data recoil:data:0x56b33c: g_zSndCdState.discLengthSecond.
 * Storage group: g_zSndCdState, retail [0x56b318, 0x56b340).
 * Purpose: Stores the CD-audio state record; zSndPreInitializeRuntimeState
 * resets its track-list count, flags, device ids and volumes.
 */
extern "C" zSndCdState g_zSndCdState = { 0 };
extern "C" zSndCdTrackState g_zSndCdPlayFrom = { 0 };
extern "C" zSndCdTrackState g_zSndCdCurrent = { 0 };
extern "C" zSndCdTrackState g_zSndCdPlayTo = { 0 };
extern "C" int g_zSnd_IsInitialized = 0;
extern "C" int g_zSnd_ActiveBackend = ZSND_AUDIO_API_DIRECTSOUND;
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
std::list<zSndCdTrackEntry*> g_zSndCdTrackList;
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
 *
 *
 * Purpose: Open the MCI CD device, cache track metadata, and build the CD track list.
 */
RECOIL_NO_GS int __fastcall Init(zReader::Node* cdTracksNode)
{
    if (g_zSndCdFlags.ready) {
        return 1;
    }

    MCI_OPEN_PARMSA openParms;
    memset(&openParms, 0, sizeof(openParms));
    openParms.lpstrDeviceType = "cdaudio";
    DWORD mciError = mciSendCommandA(0, MCI_OPEN, MCI_OPEN_TYPE, (DWORD_PTR)(&openParms));
    if (mciError != 0) {
        return zSnd::ReportMciError(mciError, "D:\Proj\GameZRecoil\zSound\zsnd_cd.cpp", 0x43);
    }

    g_zSndCdDeviceId = (unsigned short)(openParms.wDeviceID);

    MCI_STATUS_PARMS statusParms;
    memset(&statusParms, 0, sizeof(statusParms));
    statusParms.dwItem = MCI_STATUS_MEDIA_PRESENT;
    mciError = mciSendCommandA(
        (MCIDEVICEID)(g_zSndCdDeviceId),
        MCI_STATUS,
        MCI_WAIT | MCI_STATUS_ITEM,
        (DWORD_PTR)(&statusParms)
    );
    if (mciError != 0) {
        return zSnd::ReportMciError(mciError, "D:\Proj\GameZRecoil\zSound\zsnd_cd.cpp", 0x4d);
    }

    if (statusParms.dwReturn == 0) {
        Shutdown();
        return 0;
    }

    MCI_SET_PARMS setParms;
    memset(&setParms, 0, sizeof(setParms));
    setParms.dwTimeFormat = MCI_FORMAT_TMSF;
    mciError = mciSendCommandA(
        (MCIDEVICEID)(g_zSndCdDeviceId),
        MCI_SET,
        MCI_WAIT | MCI_SET_TIME_FORMAT,
        (DWORD_PTR)(&setParms)
    );
    if (mciError != 0) {
        return zSnd::ReportMciError(mciError, "D:\Proj\GameZRecoil\zSound\zsnd_cd.cpp", 0x5d);
    }

    memset(&statusParms, 0, sizeof(statusParms));
    statusParms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
    mciError = mciSendCommandA(
        (MCIDEVICEID)(g_zSndCdDeviceId),
        MCI_STATUS,
        MCI_WAIT | MCI_STATUS_ITEM,
        (DWORD_PTR)(&statusParms)
    );
    if (mciError != 0) {
        return zSnd::ReportMciError(mciError, "D:\Proj\GameZRecoil\zSound\zsnd_cd.cpp", 0x66);
    }

    g_zSndCdTrackCountCached = (int)(statusParms.dwReturn);
    statusParms.dwItem = MCI_STATUS_LENGTH;
    statusParms.dwTrack = 0;
    mciError = mciSendCommandA(
        (MCIDEVICEID)(g_zSndCdDeviceId),
        MCI_STATUS,
        MCI_WAIT | MCI_STATUS_ITEM,
        (DWORD_PTR)(&statusParms)
    );
    if (mciError != 0) {
        return zSnd::ReportMciError(mciError, "D:\Proj\GameZRecoil\zSound\zsnd_cd.cpp", 0x70);
    }

    const int auxCount = auxGetNumDevs();
    for (int deviceId = 0; deviceId < auxCount; ++deviceId) {
        AUXCAPSA caps;
        memset(&caps, 0, sizeof(caps)); // retail rep stosd, ecx = 0x0c
        if (auxGetDevCapsA(deviceId, &caps, sizeof(caps)) == 0 && caps.wTechnology == AUXCAPS_CDAUDIO
            && (caps.dwSupport & AUXCAPS_VOLUME) != 0) {
            if ((caps.dwSupport & AUXCAPS_LRVOLUME) != 0) {
                g_zSndCdFlags.stereoAux = 1;
            }
            g_zSndCdAuxDeviceId = deviceId;
            break;
        }
    }

    if (g_zSndCdAuxDeviceId != -1) {
        // Not pre-zeroed: retail has no store before auxGetVolume.
        DWORD volume;
        if (auxGetVolume((UINT)(g_zSndCdAuxDeviceId), &volume) == 0) {
            g_zSndCdAuxVolumePrimary = HIWORD(volume);
            g_zSndCdAuxVolumeSecondary = LOWORD(volume);
        }
    }

    g_zSndCdDiscLengthMinute = MCI_TMSF_MINUTE(statusParms.dwReturn);
    g_zSndCdDiscLengthSecond = MCI_TMSF_SECOND(statusParms.dwReturn);
    ResetTrackState();
    g_zSndCdFlags.ready = 1;

    if (cdTracksNode != 0) {
        for (int i = 1; i < cdTracksNode->value.nodes[0].value.i32; ++i) {
            zReader::Node* const trackConfig = cdTracksNode->value.nodes[i].value.nodes;
            g_zSndCdTrackList.push_back(new zSndCdTrackEntry(trackConfig[1].value.str, trackConfig[2].value.i32));
        }
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-cd.resettrackstate
 * @recoil-artifact defines .text recoil:function:0x4a2490: zSndCd::ResetTrackState.
 * @recoil-match byte
 *
 * Purpose: Reset cached CD play-from/current/play-to positions to track one.
 */
int __cdecl ResetTrackState()
{
    zSndCdTrackState state = { 1, 0, 0 };
    g_zSndCdPlayFrom = state;
    g_zSndCdCurrent = state;
    g_zSndCdPlayTo = state;
    return state.track;
}

namespace {
    /**
     * Original-source helper: std::transform functor that releases one CD track
     * entry and returns null; expanded inline into zSndCd::Shutdown (retail walks
     * the list with separate read and write node cursors).
     */
    struct zSndCdTrackEntryDelete {
        zSndCdTrackEntry* operator()(zSndCdTrackEntry* entry) const
        {
            if (entry != 0) {
                if (entry->archiveName != 0) {
                    free(entry->archiveName);
                    entry->archiveName = 0;
                }
                delete entry;
            }
            return 0;
        }
    };
} // namespace

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-cd.shutdown
 * @recoil-artifact defines .text recoil:function:0x4a24d0: zSndCd::Shutdown.
 *
 *
 * Purpose: stop CD playback, close the MCI CD device, clear ready state, and
 * release configured track-list entries.
 */
int __cdecl Shutdown()
{
    Stop();

    if (g_zSndCdDeviceId != 0) {
        MCI_GENERIC_PARMS closeParms = { 0 };
        mciSendCommandA((MCIDEVICEID)(g_zSndCdDeviceId), MCI_CLOSE, MCI_WAIT, (DWORD_PTR)(&closeParms));
        g_zSndCdDeviceId = 0;
    }

    g_zSndCdFlags.ready = 0;

    std::transform(
        g_zSndCdTrackList.begin(),
        g_zSndCdTrackList.end(),
        g_zSndCdTrackList.begin(),
        zSndCdTrackEntryDelete()
    );
    g_zSndCdTrackList.clear();

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-cd.playtrackwithmode
 * @recoil-artifact defines .text recoil:function:0x4a25e0: zSndCd::PlayTrackWithMode.
 * @recoil-match byte
 *
 * Purpose: Start a CD track and then apply the requested playback mode.
 */
int __fastcall PlayTrackWithMode(int trackIndex, int playbackMode)
{
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
 * @recoil-match byte
 *
 * Purpose: Apply the requested CD playback mode and issue the MCI play command.
 */
RECOIL_NO_GS int __fastcall ApplyPlaybackMode(int playbackMode)
{
    if (!g_zSndCdFlags.ready) {
        return 0;
    }

    const int currentTrack = g_zSndCdCurrentTrack;
    const int trackCount = g_zSndCdTrackCountCached;
    int playToTrack;
    if (playbackMode == 2 || playbackMode == 5) {
        playToTrack = currentTrack + 1;
    } else {
        playToTrack = trackCount + 1;
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

    const DWORD mciError = mciSendCommandA((MCIDEVICEID)(g_zSndCdDeviceId), 0x806, playFlags, (DWORD_PTR)(&playParms));
    if (mciError != 0) {
        return zSnd::ReportMciError(mciError, "D:\\Proj\\GameZRecoil\\zSound\\zsnd_cd.cpp", 0xf1);
    }

    g_zSndCdLastPlayMode = playbackMode;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-cd.onmcinotify
 * @recoil-artifact defines .text recoil:function:0x4a26b0: zSndCd::OnMciNotify.
 * @recoil-match byte
 *
 * Purpose: Restart looping CD playback when the MCI notify callback completes.
 */
void __fastcall OnMciNotify(unsigned int wParam, unsigned int lParam)
{
    if (!g_zSndCdFlags.ready || g_zSndCdLastPlayMode != 5 || lParam != (unsigned int)(g_zSndCdDeviceId)
        || wParam != 1) {
        return;
    }

    PlayTrackWithMode(g_zSndCdCurrentTrack, 5);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-cd.stop
 * @recoil-artifact defines .text recoil:function:0x4a26f0: zSndCd::Stop.
 * @recoil-match byte
 *
 * Purpose: stop the current MCI CD playback and reset the cached track state.
 */
RECOIL_NO_GS int __cdecl Stop()
{
    if (!g_zSndCdFlags.ready) {
        return 0;
    }

    MCI_GENERIC_PARMS stopParms;
    const DWORD mciError = mciSendCommandA((MCIDEVICEID)(g_zSndCdDeviceId), 0x808, 0x02, (DWORD_PTR)(&stopParms));
    if (mciError != 0) {
        return zSnd::ReportMciError(mciError, "D:\\Proj\\GameZRecoil\\zSound\\zsnd_cd.cpp", 0x10e);
    }

    g_zSndCdLastPlayMode = 0;
    ResetTrackState();
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-cd.playtrack
 * @recoil-artifact defines .text recoil:function:0x4a2750: zSndCd::PlayTrack.
 * @recoil-match byte
 *
 * Purpose: Seek to a CD track and reset cached playback state for that track.
 */
RECOIL_NO_GS int __fastcall PlayTrack(int trackIndex)
{
    if (!g_zSndCdFlags.ready) {
        return 0;
    }

    MCI_SEEK_PARMS seekParms;
    seekParms.dwTo = (DWORD)(trackIndex & 0xff);

    const DWORD mciError = mciSendCommandA((MCIDEVICEID)(g_zSndCdDeviceId), 0x807, 0x0a, (DWORD_PTR)(&seekParms));
    if (mciError != 0) {
        return zSnd::ReportMciError(mciError, "D:\\Proj\\GameZRecoil\\zSound\\zsnd_cd.cpp", 0x16e);
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
 * @recoil-match byte
 *
 * Purpose: report whether CD audio has an initialized stereo AUX mixer.
 */
int __cdecl IsStereoAuxEnabled()
{
    if (!g_zSndCdFlags.ready) {
        return 0;
    }

    if (g_zSndCdAuxDeviceId == -1) {
        return 0;
    }

    return g_zSndCdFlags.stereoAux;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-cd.getvolume
 * @recoil-artifact defines .text recoil:function:0x4a27f0: zSndCd::GetVolume.
 *
 *
 * Purpose: read the AUX mixer volume into mono or stereo output channels.
 */
int __fastcall GetVolume(unsigned short* primaryVolumeOut, unsigned short* secondaryVolumeOut)
{
    if (!g_zSndCdFlags.ready) {
        return 0;
    }

    const int stereoAuxEnabled = IsStereoAuxEnabled();
    DWORD volume;
    const DWORD mciError = auxGetVolume((UINT)(g_zSndCdAuxDeviceId), &volume);
    if (mciError != 0) {
        return zSnd::ReportMciError(mciError, "D:\\Proj\\GameZRecoil\\zSound\\zsnd_cd.cpp", 0x194);
    }

    g_zSndCdAuxVolumePrimary = (unsigned short)(volume & 0xffff);
    if (stereoAuxEnabled != 0) {
        *primaryVolumeOut = g_zSndCdAuxVolumePrimary;
        g_zSndCdAuxVolumeSecondary = (unsigned short)((volume >> 16) & 0xffff);
        *secondaryVolumeOut = g_zSndCdAuxVolumeSecondary;
    } else {
        g_zSndCdAuxVolumeSecondary = g_zSndCdAuxVolumePrimary;
        *secondaryVolumeOut = g_zSndCdAuxVolumeSecondary;
        *primaryVolumeOut = g_zSndCdAuxVolumeSecondary;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-cd.setvolume
 * @recoil-artifact defines .text recoil:function:0x4a2880: zSndCd::SetVolume.
 * @recoil-match byte
 *
 * Purpose: write mono or stereo AUX mixer volume from requested channel values.
 */
int __fastcall SetVolume(unsigned short primaryVolume, unsigned short secondaryVolume)
{
    if (!g_zSndCdFlags.ready) {
        return 0;
    }

    DWORD volume;
    if (IsStereoAuxEnabled() != 0) {
        volume = ((DWORD)(secondaryVolume) << 16) | (DWORD)(primaryVolume);
    } else {
        volume = (unsigned short)(((int)(primaryVolume) + (int)(secondaryVolume)) / 2);
    }

    const DWORD mciError = auxSetVolume((UINT)(g_zSndCdAuxDeviceId), volume);
    if (mciError != 0) {
        return zSnd::ReportMciError(mciError, "D:\\Proj\\GameZRecoil\\zSound\\zsnd_cd.cpp", 0x1b2);
    }

    g_zSndCdAuxVolumePrimary = primaryVolume;
    g_zSndCdAuxVolumeSecondary = secondaryVolume;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-cd.gettrackcount
 * @recoil-artifact defines .text recoil:function:0x4a2930: zSndCd::GetTrackCount.
 * @recoil-match byte
 *
 * Purpose: Return the cached number of CD tracks when the CD device is ready.
 */
int __cdecl GetTrackCount()
{
    if (!g_zSndCdFlags.ready) {
        return 0;
    }

    return g_zSndCdTrackCountCached;
}
} // namespace zSndCd
