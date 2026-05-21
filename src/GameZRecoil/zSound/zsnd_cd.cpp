#include "zSound.h"

#include "GameZRecoil/zReader/zReader.h"

#include <mmsystem.h>
#include <windows.h>

#include "recoil/recoil_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER) && _MSC_VER <= 1200
extern "C" FILE *__imp___iob;
extern "C" int(__cdecl *__imp__fprintf)(FILE *, const char *, ...);
#endif

struct zSndCdTrackState
{
    int track;
    int minute;
    int second;
};

extern "C" int g_zSndCdFlags = 0;
extern "C" int g_zSndCdLastPlayMode = 0;
extern "C" int g_zSndCdDeviceId = 0;
extern "C" int g_zSndCdAuxDeviceId = -1;
extern "C" int g_zSndCdTrackCountCached = 0;
extern "C" zSndCdTrackState g_zSndCdPlayFrom = {1, 0, 0};
extern "C" zSndCdTrackState g_zSndCdCurrent = {1, 0, 0};
extern "C" zSndCdTrackState g_zSndCdPlayTo = {1, 0, 0};
extern "C" zSndCdTrackNode *g_zSndCd_TrackListHead = 0;
extern "C" int g_zSndCd_TrackCount = 0;
extern "C" int g_zSndCdDiscLengthMinute = 0;
extern "C" int g_zSndCdDiscLengthSecond = 0;
extern "C" unsigned short g_zSndCdAuxVolumePrimary = 0;
extern "C" unsigned short g_zSndCdAuxVolumeSecondary = 0;
extern "C" int *ZOPT_AUDIO_API = 0;
extern "C" int *ZOPT_SOUND_CDAUDIO = 0;
extern "C" int g_zSnd_IsInitialized = 0;
extern "C" int g_zSnd_ActiveBackend = 0;
extern "C" unsigned int g_zSnd_WindowHandle = 0;
extern "C" int g_zSnd_UseArchiveBanksFlag = 0;
extern "C" float g_zSndSpeedOfSoundMps = 0.0f;
extern "C" float g_zSndInvSpeedOfSoundMps = 0.0f;

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

zReader::Node *ArrayBase(zReader::Node *node) {
    return node->value.nodes;
}

int ArrayCount(zReader::Node *node) {
    return ArrayBase(node)[0].value.i32;
}

void AppendCdTrackEntry(zSndCdTrackEntry *entry) {
    if (g_zSndCd_TrackListHead == 0) {
        g_zSndCd_TrackListHead =
            static_cast<zSndCdTrackNode *>(::operator new(sizeof(zSndCdTrackNode)));
        g_zSndCd_TrackListHead->next = g_zSndCd_TrackListHead;
        g_zSndCd_TrackListHead->prev = g_zSndCd_TrackListHead;
        g_zSndCd_TrackListHead->entry = 0;
    }

    zSndCdTrackNode *head = g_zSndCd_TrackListHead;
    zSndCdTrackNode *tail = head->prev;
    zSndCdTrackNode *node = static_cast<zSndCdTrackNode *>(::operator new(sizeof(zSndCdTrackNode)));
    node->next = head;
    node->prev = tail;
    tail->next = node;
    head->prev = node;
    node->entry = entry;
    ++g_zSndCd_TrackCount;
}
} // namespace

namespace zSnd {
// Reimplements 0x4a1290: zSnd::SetActiveBackendPreInit
RECOIL_NOINLINE int RECOIL_FASTCALL SetActiveBackendPreInit(int backend) {
    if (g_zSnd_IsInitialized != 0) {
        return 0;
    }

    g_zSnd_ActiveBackend = backend;
    return 1;
}

// Reimplements 0x4a12b0: zSnd::GetActiveBackend
RECOIL_NOINLINE int RECOIL_CDECL GetActiveBackend() {
    return g_zSnd_ActiveBackend;
}

// Reimplements 0x4080a0: zSnd::SetAudioApiOption
RECOIL_NOINLINE int RECOIL_FASTCALL SetAudioApiOption(int apiType) {
    *ZOPT_AUDIO_API = apiType;
    return SetActiveBackendPreInit(apiType);
}

// Reimplements 0x4080b0: zSnd::GetAudioApiOption
RECOIL_NOINLINE int RECOIL_CDECL GetAudioApiOption() {
    return *ZOPT_AUDIO_API;
}

// Reimplements 0x408210: zSnd::SetCDAudioOption
RECOIL_NOINLINE void RECOIL_FASTCALL SetCDAudioOption(int cdAudioOption) {
    *ZOPT_SOUND_CDAUDIO = cdAudioOption;
}

// Reimplements 0x408220: zSnd::GetCDAudioOption
RECOIL_NOINLINE int RECOIL_CDECL GetCDAudioOption() {
    return *ZOPT_SOUND_CDAUDIO;
}

// Reimplements 0x4a07f0: zSnd::SetUseArchiveBanksFlag
RECOIL_NOINLINE void RECOIL_FASTCALL SetUseArchiveBanksFlag(int useArchiveBanks) {
    g_zSnd_UseArchiveBanksFlag = useArchiveBanks;
}

// Reimplements 0x4a2e80: zSnd::SetSpeedOfSoundMps
RECOIL_NOINLINE void RECOIL_FASTCALL SetSpeedOfSoundMps(float speedOfSoundMps) {
    g_zSndSpeedOfSoundMps = speedOfSoundMps;
    g_zSndInvSpeedOfSoundMps = 1.0f / speedOfSoundMps;
}

// Reimplements 0x4a3ea0: zSnd::ReportMciError
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_FASTCALL ReportMciError(unsigned int mciError,
                                                                         const char *sourceFile,
                                                                         int lineNumber) {
    char errorText[0x100];
    mciGetErrorStringA(mciError, errorText, sizeof(errorText));
#if defined(_MSC_VER) && _MSC_VER <= 1200
    FILE *stream = __imp___iob + 2;
    (*__imp__fprintf)(stream, "%s(%d): MCIError [%s]\n", sourceFile, lineNumber, errorText);
#else
    fprintf(stderr, "%s(%d): MCIError [%s]\n", sourceFile, lineNumber, errorText);
#endif
    return 0;
}
} // namespace zSnd

namespace zSndCd {
RECOIL_NOINLINE int RECOIL_CDECL ResetTrackState();
RECOIL_NOINLINE int RECOIL_CDECL Shutdown();

// Reimplements 0x4a20d0: zSndCd::Init
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_FASTCALL Init(zReader::Node *cdTracksNode) {
    if ((g_zSndCdFlags & ZSND_CD_FLAG_READY) != 0) {
        return 1;
    }

    MCI_OPEN_PARMSA openParms = {0};
    openParms.lpstrDeviceType = "cdaudio";
    DWORD mciError =
        mciSendCommandA(0, MCI_OPEN, MCI_OPEN_TYPE, (DWORD_PTR)(&openParms));
    if (mciError != 0) {
        return zSnd::ReportMciError(mciError, kZSndCdSourceFile, 0x43);
    }

    g_zSndCdDeviceId =
        (g_zSndCdDeviceId & 0xffff0000) | static_cast<unsigned short>(openParms.wDeviceID);

    MCI_STATUS_PARMS statusParms = {0};
    statusParms.dwItem = 5;
    mciError =
        mciSendCommandA(static_cast<MCIDEVICEID>(g_zSndCdDeviceId & 0xffff), MCI_STATUS,
                        MCI_WAIT | MCI_STATUS_ITEM, (DWORD_PTR)(&statusParms));
    if (mciError != 0) {
        return zSnd::ReportMciError(mciError, kZSndCdSourceFile, 0x4d);
    }

    if (statusParms.dwReturn == 0) {
        Shutdown();
        return 0;
    }

    MCI_SET_PARMS setParms = {0};
    setParms.dwTimeFormat = MCI_FORMAT_TMSF;
    mciError =
        mciSendCommandA(static_cast<MCIDEVICEID>(g_zSndCdDeviceId & 0xffff), MCI_SET,
                        MCI_WAIT | MCI_SET_TIME_FORMAT, (DWORD_PTR)(&setParms));
    if (mciError != 0) {
        return zSnd::ReportMciError(mciError, kZSndCdSourceFile, 0x5d);
    }

    memset(&statusParms, 0, sizeof(statusParms));
    statusParms.dwItem = 3;
    mciError =
        mciSendCommandA(static_cast<MCIDEVICEID>(g_zSndCdDeviceId & 0xffff), MCI_STATUS,
                        MCI_WAIT | MCI_STATUS_ITEM, (DWORD_PTR)(&statusParms));
    if (mciError != 0) {
        return zSnd::ReportMciError(mciError, kZSndCdSourceFile, 0x66);
    }

    g_zSndCdTrackCountCached = static_cast<int>(statusParms.dwReturn);
    statusParms.dwItem = 1;
    statusParms.dwTrack = 0;
    mciError =
        mciSendCommandA(static_cast<MCIDEVICEID>(g_zSndCdDeviceId & 0xffff), MCI_STATUS,
                        MCI_WAIT | MCI_STATUS_ITEM, (DWORD_PTR)(&statusParms));
    if (mciError != 0) {
        return zSnd::ReportMciError(mciError, kZSndCdSourceFile, 0x70);
    }

    const UINT auxCount = auxGetNumDevs();
    for (UINT deviceId = 0; deviceId < auxCount; ++deviceId) {
        AUXCAPSA caps = {0};
        if (auxGetDevCapsA(deviceId, &caps, sizeof(caps)) == 0 &&
            caps.wTechnology == AUXCAPS_CDAUDIO && (caps.dwSupport & AUXCAPS_VOLUME) != 0) {
            if ((caps.dwSupport & AUXCAPS_LRVOLUME) != 0) {
                g_zSndCdFlags |= ZSND_CD_FLAG_STEREO_AUX;
            }
            g_zSndCdAuxDeviceId = static_cast<int>(deviceId);
            break;
        }
    }

    if (g_zSndCdAuxDeviceId != -1) {
        DWORD volume = 0;
        if (auxGetVolume(static_cast<UINT>(g_zSndCdAuxDeviceId), &volume) == 0) {
            g_zSndCdAuxVolumeSecondary = static_cast<unsigned short>(volume & 0xffff);
            g_zSndCdAuxVolumePrimary = static_cast<unsigned short>((volume >> 16) & 0xffff);
        }
    }

    const DWORD discLength = statusParms.dwReturn;
    g_zSndCdDiscLengthMinute = static_cast<int>((discLength >> 8) & 0xff);
    g_zSndCdDiscLengthSecond = static_cast<int>((discLength >> 16) & 0xff);
    ResetTrackState();
    g_zSndCdFlags |= ZSND_CD_FLAG_READY;

    if (cdTracksNode != 0) {
        zReader::Node *tracks = ArrayBase(cdTracksNode);
        for (int i = 1; i < ArrayCount(cdTracksNode); ++i) {
            zReader::Node *trackNode = &tracks[i];
            if (trackNode->type != zReader::ZRDR_NODE_ARRAY) {
                continue;
            }

            zReader::Node *trackConfig = ArrayBase(trackNode);
            zSndCdTrackEntry *entry =
                static_cast<zSndCdTrackEntry *>(::operator new(sizeof(zSndCdTrackEntry)));
            if (entry != 0) {
                entry->trackNumber = trackConfig[2].value.i32;
                entry->archiveName = _strdup(trackConfig[1].value.str);
            }
            AppendCdTrackEntry(entry);
        }
    }

    return 1;
}

// Reimplements 0x4a2490: zSndCd::ResetTrackState
RECOIL_NOINLINE int RECOIL_CDECL ResetTrackState() {
    zSndCdTrackState state = {1, 0, 0};
    g_zSndCdPlayFrom = state;
    g_zSndCdCurrent = state;
    g_zSndCdPlayTo = state;
    return state.track;
}

// Reimplements 0x4a27d0: zSndCd::IsStereoAuxEnabled
int RECOIL_CDECL IsStereoAuxEnabled() {
    if ((g_zSndCdFlags & ZSND_CD_FLAG_READY) == 0) {
        return 0;
    }

    if (g_zSndCdAuxDeviceId == -1) {
        return 0;
    }

    return g_zSndCdFlags & ZSND_CD_FLAG_STEREO_AUX;
}

// Reimplements 0x4a2600: zSndCd::ApplyPlaybackMode
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_FASTCALL
ApplyPlaybackMode(int playbackMode) {
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
    playParms.dwFrom = static_cast<DWORD>(currentTrack & 0xff);
    playParms.dwTo = static_cast<DWORD>(playToTrack & 0xff);
    playParms.dwCallback = g_zSnd_WindowHandle;

    DWORD playFlags = 0x5;
    if (static_cast<unsigned int>(playToTrack) <= static_cast<unsigned int>(trackCount)) {
        playFlags = 0x0d;
    }

    const DWORD mciError =
        mciSendCommandA(static_cast<MCIDEVICEID>(g_zSndCdDeviceId & 0xffff), 0x806, playFlags,
                        (DWORD_PTR)(&playParms));
    if (mciError != 0) {
        return zSnd::ReportMciError(mciError, kZSndCdSourceFile, 0xf1);
    }

    g_zSndCdLastPlayMode = playbackMode;
    return 1;
}

// Reimplements 0x4a2930: zSndCd::GetTrackCount
RECOIL_NOINLINE int RECOIL_CDECL GetTrackCount() {
    if ((g_zSndCdFlags & ZSND_CD_FLAG_READY) == 0) {
        return 0;
    }

    return g_zSndCdTrackCountCached;
}

// Reimplements 0x4a2750: zSndCd::PlayTrack
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_FASTCALL PlayTrack(int trackIndex) {
    if ((g_zSndCdFlags & ZSND_CD_FLAG_READY) == 0) {
        return 0;
    }

    MCI_SEEK_PARMS seekParms;
    seekParms.dwTo = static_cast<DWORD>(trackIndex & 0xff);

    const DWORD mciError = mciSendCommandA(static_cast<MCIDEVICEID>(g_zSndCdDeviceId & 0xffff),
                                           0x807, 0x0a, (DWORD_PTR)(&seekParms));
    if (mciError != 0) {
        return zSnd::ReportMciError(mciError, kZSndCdSourceFile, 0x16e);
    }

    ResetTrackState();
    g_zSndCdCurrentTrack = trackIndex;
    g_zSndCdPlayToTrack = trackIndex;
    g_zSndCdPlayFromTrack = trackIndex;
    return 1;
}

// Reimplements 0x4a25e0: zSndCd::PlayTrackWithMode
RECOIL_NOINLINE int RECOIL_FASTCALL PlayTrackWithMode(int trackIndex,
                                                               int playbackMode) {
    int result = 0;
    const int mode = playbackMode;
    if (PlayTrack(trackIndex) != 0) {
        result = ApplyPlaybackMode(mode);
    }

    return result;
}

// Reimplements 0x4a26b0: zSndCd::OnMciNotify
RECOIL_NOINLINE void RECOIL_FASTCALL OnMciNotify(unsigned int wParam, unsigned int lParam) {
    if ((g_zSndCdFlags & ZSND_CD_FLAG_READY) == 0 || g_zSndCdLastPlayMode != 5 ||
        lParam != static_cast<unsigned int>(g_zSndCdDeviceId & 0xffff) || wParam != 1) {
        return;
    }

    PlayTrackWithMode(g_zSndCdCurrentTrack, 5);
}

// Reimplements 0x4a26f0: zSndCd::Stop
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_CDECL Stop() {
    if ((g_zSndCdFlags & ZSND_CD_FLAG_READY) == 0) {
        return 0;
    }

    MCI_GENERIC_PARMS stopParms;
    const DWORD mciError = mciSendCommandA(static_cast<MCIDEVICEID>(g_zSndCdDeviceId & 0xffff),
                                           0x808, 0x02, (DWORD_PTR)(&stopParms));
    if (mciError != 0) {
        return zSnd::ReportMciError(mciError, kZSndCdSourceFile, 0x10e);
    }

    g_zSndCdLastPlayMode = 0;
    ResetTrackState();
    return 1;
}

// Reimplements 0x4a24d0: zSndCd::Shutdown
RECOIL_NOINLINE int RECOIL_CDECL Shutdown() {
    Stop();

    if ((g_zSndCdDeviceId & 0xffff) != 0) {
        MCI_GENERIC_PARMS closeParms = {0};
        mciSendCommandA(static_cast<MCIDEVICEID>(g_zSndCdDeviceId & 0xffff), MCI_CLOSE, MCI_WAIT,
                        (DWORD_PTR)(&closeParms));
        g_zSndCdDeviceId &= 0xffff0000;
    }

    g_zSndCdFlags &= ~ZSND_CD_FLAG_READY;

    zSndCdTrackNode *head = g_zSndCd_TrackListHead;
    if (head != 0) {
        for (zSndCdTrackNode *node = head->next; node != head; node = node->next) {
            zSndCdTrackEntry *entry = node->entry;
            if (entry != 0) {
                if (entry->archiveName != 0) {
                    free(entry->archiveName);
                    entry->archiveName = 0;
                }
                ::operator delete(entry);
                node->entry = 0;
            }
        }

        zSndCdTrackNode *deleteNode = head->next;
        while (deleteNode != head) {
            zSndCdTrackNode *next = deleteNode->next;
            deleteNode->prev->next = deleteNode->next;
            deleteNode->next->prev = deleteNode->prev;
            ::operator delete(deleteNode);
            --g_zSndCd_TrackCount;
            deleteNode = next;
        }
    }

    return 1;
}
} // namespace zSndCd
