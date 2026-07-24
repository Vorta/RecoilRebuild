// Checked-in focused native smoke translation unit, formerly extracted from recoil_app_message_map.cpp.
// Emits only the InitStdLogFiles smoke needed by the functional manifest.

#include "Battlesport/recoil_app.h"

#include "Battlesport/briefing.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/game_net.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/player.h"
#include "Battlesport/hud.h"
#include "Battlesport/about.h"
#include "Battlesport/recoil_state_main_menu_transition.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <commdlg.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

extern "C" std::int32_t g_zSndCdFlags;
extern "C" HWND g_RecoilApp_hWndMain;
extern "C" HINSTANCE g_RecoilApp_hInstance;
extern "C" const char *g_RecoilApp_WndClassNamePtr;
extern "C" int g_RecoilApp_AttractFmvReloadMode;
extern "C" unsigned int g_HudUi_InvalidateMask;
BOOL __stdcall AfxWinInit(HINSTANCE instance, HINSTANCE previousInstance, LPSTR commandLine,
                               int showCommand);

struct RecoilStateCredits {
    RecoilPtr32 vftable;
    RecoilPtr32 dialog;

    RecoilStateCredits * Constructor();
    static void StaticInitAndRegisterAtExit();
    static void StaticInit();
    static void RegisterAtExit();
    void OnWndActivate(int activateCode);
    int OnTryBecomeCurrent();
    void OnDeactivate();
    ~RecoilStateCredits();
    static void QueuePush();
};

extern RecoilStateCredits g_RecoilStateCredits;

namespace {
bool ReadFilePrefix(const char *path, char *buffer, DWORD bufferSize) {
    HANDLE file = CreateFileA(
        path,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD bytesRead = 0;
    const BOOL ok = ReadFile(file, buffer, bufferSize - 1, &bytesRead, nullptr);
    CloseHandle(file);
    if (ok == 0) {
        return false;
    }

    buffer[bytesRead] = '\0';
    return true;
}
} // namespace

extern "C" int recoil_app_init_std_log_files_smoke(void) {
    g_RecoilApp_hWndMain = reinterpret_cast<HWND>(0x12345678);
    RecoilApp::InitStdLogFiles(nullptr);
    if (g_RecoilApp_hWndMain != nullptr) {
        return 1;
    }

    char tempPath[MAX_PATH];
    if (GetTempPathA(sizeof(tempPath), tempPath) == 0) {
        return 2;
    }

    char stem[MAX_PATH];
    if (GetTempFileNameA(tempPath, "rcl", 0, stem) == 0) {
        return 3;
    }
    DeleteFileA(stem);

    char errPath[MAX_PATH];
    char outPath[MAX_PATH];
    lstrcpyA(errPath, stem);
    lstrcatA(errPath, ".err");
    lstrcpyA(outPath, stem);
    lstrcatA(outPath, ".out");
    DeleteFileA(errPath);
    DeleteFileA(outPath);

    RecoilApp::InitStdLogFiles(stem);
    fflush(stderr);
    fflush(stdout);

    char errHeader[32];
    char outHeader[32];
    const bool errOk = ReadFilePrefix(errPath, errHeader, sizeof(errHeader));
    const bool outOk = ReadFilePrefix(outPath, outHeader, sizeof(outHeader));
    if (!errOk || !outOk) {
        return 4;
    }

    return errHeader[0] == 'F' && errHeader[5] == 's' && outHeader[0] == 'F' &&
                   outHeader[5] == 's'
               ? 0
               : 5;
}
