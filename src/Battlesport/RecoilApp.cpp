#include "Battlesport/recoil_app.h"

#include <stdio.h>
#include <string.h>

#include "Battlesport/recoil_app_std_log_file_literals_body.h"

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
#include "Battlesport/cz_recoil_frame_body.h"
#include "Battlesport/game_net_body.h"
#include "Battlesport/recoil_app_play_state_tick_and_render_frame_body.h"

/**
 * Reimplements 0x42df90: RecoilApp_IState::~RecoilApp_IState.
 *
 * Purpose: tears down the common app-state interface base.
 */
RecoilApp_IState::~RecoilApp_IState() {
}

#include "Battlesport/recoil_app_late_body.h"

/**
 * Reimplements the ordinary empty RecoilApp_MainMenuPrepState::OnDeactivate
 * identity represented by the zero-argument no-op fold group at 0x4076f0.
 * Original function address: 0x4076f0.
 * Purpose: accept deactivation after the main-menu preparation state has
 * completed its transition work.
 */
void RecoilApp_MainMenuPrepState::OnDeactivate() {
}

#include "Battlesport/recoil_state_main_menu_transition_on_update_should_quit_body.h"
#endif
