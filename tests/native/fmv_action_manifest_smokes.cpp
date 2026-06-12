#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

extern "C" int g_zFMV_ActionImage_BlitRectX;
extern "C" int g_zFMV_ActionImage_BlitRectY;
extern "C" int g_zFMV_ActionImage_BlitRectW;
extern "C" int g_zFMV_ActionImage_BlitRectH;

namespace {
struct TestAction : zFMV_Action {
};

void WriteU32(
    HANDLE file,
    unsigned int value
) {
    DWORD written = 0;
    WriteFile(
        file,
        &value,
        sizeof(value),
        &written,
        0
    );
}

void WriteBytes(
    HANDLE file,
    const char *value,
    unsigned int length
) {
    DWORD written = 0;
    WriteFile(
        file,
        value,
        length,
        &written,
        0
    );
}

void WriteStringNode(
    HANDLE file,
    const char *value
) {
    const unsigned int length = (unsigned int)(strlen(value));
    WriteU32(
        file,
        zReader::ZRDR_NODE_STRING
    );
    WriteU32(
        file,
        length
    );
    WriteBytes(
        file,
        value,
        length
    );
}

void WriteIntNode(
    HANDLE file,
    int value
) {
    WriteU32(
        file,
        zReader::ZRDR_NODE_INT
    );
    WriteU32(
        file,
        (unsigned int)(value)
    );
}

void WriteFloatNode(
    HANDLE file,
    float value
) {
    union FloatBits {
        float f32;
        unsigned int u32;
    };

    FloatBits bits;
    bits.f32 = value;
    WriteU32(
        file,
        zReader::ZRDR_NODE_FLOAT
    );
    WriteU32(
        file,
        bits.u32
    );
}

void WriteArrayHeader(
    HANDLE file,
    int count
) {
    WriteU32(
        file,
        zReader::ZRDR_NODE_ARRAY
    );
    WriteU32(
        file,
        (unsigned int)(count)
    );
}

int StringEquals(
    const char *left,
    const char *right
) {
    return left != 0 && strcmp(left, right) == 0;
}

void SetActiveRegion(
    int width,
    int height
) {
    zRndr::g_activeRegionWidth = width;
    zRndr::g_activeRegionHeight = height;
    zRndr::g_pitchBytes = width * 2;
    zRndr::g_bytesPerPixel = 2;
}
} // namespace

extern "C" int zfmv_script_append_action_smoke(void) {
    zFMV_Script script = {};
    TestAction action1 = {};
    TestAction action2 = {};
    action1.next = (zFMV_Action *)(0x11111111);
    action2.next = (zFMV_Action *)(0x22222222);

    if (script.AppendAction(0) != 0 ||
        script.m_head != 0 ||
        script.m_tail != 0 ||
        script.m_cur != 0) {
        return 1;
    }

    if (script.AppendAction(&action1) != 1 ||
        action1.next != 0 ||
        script.m_head != &action1 ||
        script.m_tail != &action1 ||
        script.m_cur != &action1) {
        return 2;
    }

    if (script.AppendAction(&action2) != 1 ||
        action1.next != &action2 ||
        action2.next != 0 ||
        script.m_head != &action1 ||
        script.m_tail != &action2 ||
        script.m_cur != &action1) {
        return 3;
    }

    return 0;
}

extern "C" int zfmv_action_image_constructor_with_screen_rect_smoke(void) {
    g_zFMV_ActionImage_BlitRectX = -1;
    g_zFMV_ActionImage_BlitRectY = -1;
    g_zFMV_ActionImage_BlitRectW = 640;
    g_zFMV_ActionImage_BlitRectH = 480;

    zFMV_ActionImage action = {};
    action.next = (zFMV_Action *)(0x11111111);
    action.image = (void *)(0x22222222);

    zFMV_ActionImage *const returned = new (&action) zFMV_ActionImage(
        "screen.raw",
        7,
        32,
        48
    );

    return returned == &action &&
                   action.next == 0 &&
                   action.image == 0 &&
                   StringEquals(
                       action.imagePath,
                       "screen.raw"
                   ) &&
                   action.doAdjustSurfaces == 7 &&
                   action.forcePrimaryPostprocess == 1 &&
                   g_zFMV_ActionImage_BlitRectX == 32 &&
                   g_zFMV_ActionImage_BlitRectY == 48 &&
                   action.blitRect.left == 32 &&
                   action.blitRect.top == 48 &&
                   action.blitRect.right == 640 &&
                   action.blitRect.bottom == 480
               ? 0
               : 1;
}

extern "C" int zfmv_action_image_constructor_scaled_smoke(void) {
    SetActiveRegion(
        800,
        600
    );

    zFMV_ActionImage action = {};
    action.next = (zFMV_Action *)(0x11111111);
    action.image = (void *)(0x22222222);

    zFMV_ActionImage *const returned = new (&action) zFMV_ActionImage(
        "scaled.raw",
        3
    );

    return returned == &action &&
                   action.next == 0 &&
                   action.image == 0 &&
                   StringEquals(
                       action.imagePath,
                       "scaled.raw"
                   ) &&
                   action.doAdjustSurfaces == 3 &&
                   action.forcePrimaryPostprocess == 0 &&
                   action.blitRect.left == 0 &&
                   action.blitRect.top == 0 &&
                   action.blitRect.right == 800 &&
                   action.blitRect.bottom == 600
               ? 0
               : 1;
}

extern "C" int zfmv_action_fade_constructor_smoke(void) {
    zFMV_ActionFade action = {};
    action.next = (zFMV_Action *)(0x11111111);
    action.capturedFrame = (void *)(0x22222222);
    action.startSec = 12.5;

    zFMV_ActionFade *const returned = new (&action) zFMV_ActionFade(
        0xff,
        0x80,
        0x20,
        0x3fc00000,
        -1,
        128
    );

    const unsigned short expectedColor = (unsigned short)(zVid_PackColorRGB(
        0xff,
        0x80,
        0x20
    ));
    return returned == &action &&
                   action.next == 0 &&
                   action.fadeColorPacked16 == expectedColor &&
                   action.durationSecRaw == 0x3fc00000 &&
                   action.fadeDirectionSign == -1 &&
                   action.maxAlpha == 128 &&
                   action.capturedFrame == (void *)(0x22222222) &&
                   action.startSec == 12.5
               ? 0
               : 1;
}

extern "C" int zfmv_action_play_avi_constructor_existing_file_smoke(void) {
    const char *const fileName = "__recoil_fmv_existing_constructor.avi";
    HANDLE file = CreateFileA(
        fileName,
        GENERIC_WRITE,
        0,
        0,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY,
        0
    );
    if (file == INVALID_HANDLE_VALUE) {
        return 1;
    }
    CloseHandle(file);

    zFMV_ActionPlayAvi action = {};
    action.next = (zFMV_Action *)(0x11111111);
    zFMV_ActionPlayAvi *const returned = new (&action) zFMV_ActionPlayAvi(
        ".",
        fileName,
        5
    );

    char expectedPath[MAX_PATH] = {};
    sprintf(
        expectedPath,
        ".\\%s",
        fileName
    );

    const int ok = returned == &action &&
                   action.next == 0 &&
                   action.modeFlags == 5 &&
                   StringEquals(
                       action.mediaPath,
                       expectedPath
                   );
    DeleteFileA(fileName);
    return ok ? 0 : 2;
}

extern "C" int zfmv_action_play_avi_constructor_drive_fallback_smoke(void) {
    zFMV_ActionPlayAvi action = {};
    action.next = (zFMV_Action *)(0x11111111);

    zFMV_ActionPlayAvi *const returned = new (&action) zFMV_ActionPlayAvi(
        ".",
        "__recoil_missing_fmv_constructor_file__.avi",
        9
    );

    return returned == &action &&
                   action.next == 0 &&
                   action.modeFlags == 9 &&
                   action.mediaPath != 0 &&
                   strstr(
                       action.mediaPath,
                       "__recoil_missing_fmv_constructor_file__.avi"
                   ) != 0
               ? 0
               : 1;
}

extern "C" int zfmv_playback_constructor_smoke(void) {
    zFMV_Playback playback = {};
    playback.mciPutFlags = 0x77777777;
    playback.notifyHwnd = (HWND)(0x11111111);
    playback.mediaPathDup = (char *)(0x22222222);

    zFMV_Playback *const returned = new (&playback) zFMV_Playback(
        "movie.avi",
        (HWND)(0x12345678)
    );

    const bool ok =
        returned == &playback && playback.mediaPathDup != 0 &&
        strcmp(
            playback.mediaPathDup,
            "movie.avi"
        ) == 0 &&
        playback.notifyHwnd == (HWND)(0x12345678) && playback.mciPutFlags == 0;

    free(playback.mediaPathDup);
    return ok ? 0 : 1;
}

extern "C" int zfmv_action_play_mci_constructor_smoke(void) {
    SetActiveRegion(
        1024,
        768
    );

    zFMV_ActionPlayMci action = {};
    action.next = (zFMV_Action *)(0x11111111);
    zFMV_ActionPlayMci *const returned = new (&action) zFMV_ActionPlayMci(
        "movies",
        "intro.mpg",
        (HWND)(0x1234)
    );

    return returned == &action &&
                   action.next == 0 &&
                   StringEquals(
                       action.mediaPath,
                       "movies\\intro.mpg"
                   ) &&
                   action.playback != 0 &&
                   StringEquals(
                       action.playback->mediaPathDup,
                       "movies\\intro.mpg"
                   ) &&
                   action.playback->notifyHwnd == (HWND)(0x1234) &&
                   action.playback->destinationRect.left == 0 &&
                   action.playback->destinationRect.top == 0 &&
                   action.playback->destinationRect.right == 1024 &&
                   action.playback->destinationRect.bottom == 768
               ? 0
               : 1;
}

extern "C" int zfmv_action_blur_constructor_smoke(void) {
    zFMV_ActionBlur action = {};
    action.next = (zFMV_Action *)(0x11111111);

    zFMV_ActionBlur *const returned = new (&action) zFMV_ActionBlur(
        12,
        3
    );

    return returned == &action &&
                   action.next == 0 &&
                   action.framesRemaining == 12 &&
                   action.blurPassCount == 3
               ? 0
               : 1;
}

extern "C" int zfmv_script_load_actions_from_zrd_smoke(void) {
    char tempDir[MAX_PATH] = {};
    char tempPath[MAX_PATH] = {};
    if (GetTempPathA(
            sizeof(tempDir),
            tempDir
        ) == 0 ||
        GetTempFileNameA(
            tempDir,
            "fmv",
            0,
            tempPath
        ) == 0) {
        return 1;
    }

    HANDLE file = CreateFileA(
        tempPath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        0,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY,
        0
    );
    if (file == INVALID_HANDLE_VALUE) {
        DeleteFileA(tempPath);
        return 2;
    }

    WriteArrayHeader(
        file,
        7
    );
    WriteStringNode(
        file,
        "FMV_PATH"
    );
    WriteStringNode(
        file,
        "movies"
    );
    WriteStringNode(
        file,
        "IMAGE_PATH"
    );
    WriteStringNode(
        file,
        "images"
    );
    WriteStringNode(
        file,
        "INTRO"
    );
    WriteArrayHeader(
        file,
        4
    );
    WriteArrayHeader(
        file,
        3
    );
    WriteStringNode(
        file,
        "WAIT"
    );
    WriteFloatNode(
        file,
        1.25f
    );
    WriteArrayHeader(
        file,
        3
    );
    WriteStringNode(
        file,
        "BLURH"
    );
    WriteIntNode(
        file,
        4
    );
    WriteArrayHeader(
        file,
        3
    );
    WriteStringNode(
        file,
        "PLAYSOUND"
    );
    WriteStringNode(
        file,
        "intro_whoosh"
    );
    FlushFileBuffers(file);

    zZarFileRecord record = {};
    record.fileOffset = 0;
    record.fileSize = SetFilePointer(
        file,
        0,
        0,
        FILE_CURRENT
    );
    strcpy(
        record.name,
        "fmv.zrd"
    );

    zIndexArchive archive = {};
    archive.hFile = file;
    archive.recordCount = 1;
    archive.records = &record;

    zArchiveListNode node = {};
    node.payload = &archive;
    node.next = &node;
    node.prev = &node;

    zArchiveList list = {};
    list.count = 1;
    list.head = &node;
    g_zArchive_MountedList = &list;

    zFMV_Script script = {};
    script.Init(
        0,
        0,
        0
    );
    const int result = script.LoadActionsFromZrd(
        "C:\\dummy\\fmv.zrd",
        "INTRO"
    );

    zFMV_ActionWait *const wait = (zFMV_ActionWait *)(script.m_head);
    zFMV_ActionBlur *const blur =
        wait != 0 ? (zFMV_ActionBlur *)(wait->next) : 0;
    zFMV_ActionPlaySound *const sound =
        blur != 0 ? (zFMV_ActionPlaySound *)(blur->next) : 0;

    const int ok = result == 3 &&
                   StringEquals(
                       script.m_fmvPath,
                       "movies"
                   ) &&
                   wait != 0 &&
                   wait->durationSec == 1.25f &&
                   blur != 0 &&
                   blur->framesRemaining == 1 &&
                   blur->blurPassCount == 4 &&
                   sound != 0 &&
                   strcmp(
                       sound->sampleName,
                       "intro_whoosh"
                   ) == 0 &&
                   sound->voice == 0 &&
                   sound->next == 0;

    script.Cleanup();
    g_zArchive_MountedList = 0;
    CloseHandle(file);
    DeleteFileA(tempPath);
    return ok ? 0 : 3;
}
