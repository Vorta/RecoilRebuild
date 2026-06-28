#include "Battlesport/RecoilApp.h"

#include <stdio.h>
#include <string.h>

extern "C" {
extern HWND g_RecoilApp_hWndMain;
extern char g_zApp_DefaultStdoutLogName[0x0a];
extern char g_zApp_StdoutLogSuffix[0x05];
extern char g_zApp_LogFileStartBanner[0x12];
extern char g_zApp_DefaultStderrLogName[0x0a];
extern char g_zApp_StderrLogSuffix[0x05];

/**
 * Reimplements data 0x4da248: g_zApp_LogFileOpenMode.
 *
 * Purpose: supplies the freopen mode used when redirecting stdout and stderr
 * to startup log files.
 */
char g_zApp_LogFileOpenMode[0x02] = "w";
RECOIL_STATIC_ASSERT(sizeof(g_zApp_LogFileOpenMode) == 0x02);
}

/**
 * Reimplements 0x4a5780: RecoilApp::InitStdLogFiles.
 *
 * Purpose: redirects stdout and stderr to per-run log files and writes their
 * startup banners.
 */
RECOIL_NO_GS void __fastcall RecoilApp::InitStdLogFiles(
    const char *exePath
) {
    g_RecoilApp_hWndMain = 0;
    if (exePath == 0) {
        return;
    }

    char pathBuf[0x40];
    strcpy(
        pathBuf,
        exePath
    );
    strcat(
        pathBuf,
        g_zApp_StderrLogSuffix
    );
    FILE *stream = freopen(
        pathBuf,
        g_zApp_LogFileOpenMode,
        stderr
    );
    if (stream == 0 && GetTempPathA(
        sizeof(pathBuf),
        pathBuf
    ) != 0) {
        strcat(
            pathBuf,
            g_zApp_DefaultStderrLogName
        );
        stream = freopen(
            pathBuf,
            g_zApp_LogFileOpenMode,
            stderr
        );
    }
    if (stream != 0) {
        fprintf(
            stream,
            g_zApp_LogFileStartBanner
        );
        fflush(stream);
    }

    strcpy(
        pathBuf,
        exePath
    );
    strcat(
        pathBuf,
        g_zApp_StdoutLogSuffix
    );
    stream = freopen(
        pathBuf,
        g_zApp_LogFileOpenMode,
        stdout
    );
    if (stream == 0 && GetTempPathA(
        sizeof(pathBuf),
        pathBuf
    ) != 0) {
        strcat(
            pathBuf,
            g_zApp_DefaultStdoutLogName
        );
        stream = freopen(
            pathBuf,
            g_zApp_LogFileOpenMode,
            stdout
        );
    }
    if (stream != 0) {
        fprintf(
            stream,
            g_zApp_LogFileStartBanner
        );
        fflush(stream);
    }
}

#if !defined(RECOILAPP_LINK_SPLIT_EARLY_SHARD)
#include "Battlesport/RecoilApp_Late.cpp"
#endif
