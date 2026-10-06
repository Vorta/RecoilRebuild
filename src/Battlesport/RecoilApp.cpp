#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/about.h"
#include "Battlesport/briefing.h"
#include "Battlesport/game_net.h"
#include "Battlesport/hud.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/hud_ui_net_exit_panel.h"
#include "Battlesport/net_ui.h"
#include "Battlesport/pickup.h"
#include "Battlesport/player.h"
#include "Battlesport/recoil_app.h"
#include "Battlesport/recoil_state_main_menu_transition.h"
#include "Battlesport/recoil_version.h"
#include "Battlesport/turret.h"
#include "Battlesport/wol_dialog.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "zimage.h"

#include <algorithm>
#include <commdlg.h>
#include <math.h>
#include <objbase.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SPI_SETSCREENSAVERRUNNING
#define SPI_SETSCREENSAVERRUNNING 0x0061
#endif

extern const char g_RecoilApp_SoundsZrdName[0x0b];
extern const char g_RecoilApp_TurretStatusPrintfFmt[0x0f];
extern const char g_RecoilApp_StartupStatusFailed[0x07];
extern const char g_RecoilApp_StartupStatusPassed[0x07];
extern const char g_RecoilApp_OpenHseAbortMsg[0x23];
extern const char g_RecoilApp_OpenVideoAbortMsg[0x25];
extern const char g_RecoilApp_StartupArchivePath[0x0d];
extern const char g_zUtil_ZrdrCommonDataPath[0x14];
extern const char g_zUtil_ZbdSearchPathLeaf[0x04];
extern const char g_RecoilApp_IntroFmvPath[0x13];
extern const char g_RecoilApp_DoubleNewline[0x03];
extern const char g_RecoilApp_ExitAtFileLineFmt[0x0f];
extern const char g_RecoilApp_SourceFile_RecoilAppCpp[0x22];
extern const char g_RecoilApp_MessagesDllName[0x0d];
extern const char g_zFMV_ScriptFileName[0x08];
extern const char g_RecoilApp_IntroFmvTag[0x06];
extern const char g_RecoilApp_AttractFmvTag[0x08];
extern const char g_RecoilApp_MissionFmvTagTemplate[0x04];
extern const char g_zFMV_GrandPrizeScriptName[0x0b];
extern const char g_RecoilApp_MissionOverFmvTag[0x0c];
extern const char g_RecoilApp_LeavingNetworkingMsg[0x13];
extern const char g_RecoilApp_LeavingPlayStateMsg[0x13];
/**
 * Purpose: format the mission-specific ZRDR archive path mounted after search-path setup.
 */
extern const char g_zUtil_MissionZrdrArchivePathFmt[0x11] = "zbd\\m%d\\zrdr.zbd";
/**
 * Purpose: format the loose mission ZRDR search paths before mounting the archive.
 */
extern const char g_zUtil_MissionZrdrSearchPathsFmt[0x3d]
    = "..\\data\\common\\zrdr;..\\data\\m%d\\zrdr;..\\data\\m%d\\zrdr\\aipath";
/**
 * Purpose: supplies common texture and effect texture search paths for mission resources.
 */
extern const char g_zImage_CommonTextureSearchPaths[0x38]
    = "..\\data\\common\\textures;..\\data\\common\\effects\\textures";
extern const char* g_RecoilApp_WndClassNamePtr;
extern int g_RecoilApp_WindowClassRegistered;
extern "C" HINSTANCE g_RecoilApp_hInstance;
extern "C" int g_RecoilApp_AttractFmvReloadMode;
extern "C" char g_HudSensorTracker_ObjectivesZrdPath[0x0f];

extern "C" const char g_HudLoading_StopAllSoundsMsg[0x10];

AFX_MODULE_STATE* __stdcall AfxGetModuleState();
BOOL __stdcall AfxRegisterClass(WNDCLASSA* wndClass);
HINSTANCE __stdcall AfxFindResourceHandle(LPCSTR resourceName, LPCSTR resourceType);

struct RecoilStateCredits {
    static void QueuePush();
};

namespace {
enum zVideoRendererBackend {
    ZVID_RENDERER_BACKEND_SOFTWARE = 0,
};

enum zVideoSoftwareModeHotkeyState {
    ZVIDEO_SOFTWARE_MODE_HOTKEY_DISABLED = 0,
    ZVIDEO_SOFTWARE_MODE_HOTKEY_ENABLED = 1,
};

enum zVideoClearScreenBufferState {
    ZVIDEO_CLEAR_SCREEN_BUFFER_ENABLED = 1,
};
} // namespace

extern "C" {
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-zapp-defaultstdoutlogname
 * @recoil-artifact defines .data recoil:data:0x4e2fbc: g_zApp_DefaultStdoutLogName.
 *
 * Purpose: names the fallback stdout log file appended under the temp path.
 */
static char g_zApp_DefaultStdoutLogName[0x0a] = "gamez.out";
RECOIL_STATIC_ASSERT(sizeof(g_zApp_DefaultStdoutLogName) == 0x0a);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-zapp-stdoutlogsuffix
 * @recoil-artifact defines .data recoil:data:0x4e2fc8: g_zApp_StdoutLogSuffix.
 *
 * Purpose: supplies the stdout log suffix appended to the executable path.
 */
static char g_zApp_StdoutLogSuffix[0x05] = ".out";
RECOIL_STATIC_ASSERT(sizeof(g_zApp_StdoutLogSuffix) == 0x05);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-zapp-logfilestartbanner
 * @recoil-artifact defines .data recoil:data:0x4e2fd0: g_zApp_LogFileStartBanner.
 *
 * Purpose: writes the startup banner to each redirected standard log stream.
 */
static char g_zApp_LogFileStartBanner[0x12] = "File started\n---\n";
RECOIL_STATIC_ASSERT(sizeof(g_zApp_LogFileStartBanner) == 0x12);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-zapp-defaultstderrlogname
 * @recoil-artifact defines .data recoil:data:0x4e2fe4: g_zApp_DefaultStderrLogName.
 *
 * Purpose: names the fallback stderr log file appended under the temp path.
 */
static char g_zApp_DefaultStderrLogName[0x0a] = "gamez.err";
RECOIL_STATIC_ASSERT(sizeof(g_zApp_DefaultStderrLogName) == 0x0a);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-zapp-stderrlogsuffix
 * @recoil-artifact defines .data recoil:data:0x4e2ff0: g_zApp_StderrLogSuffix.
 *
 * Purpose: supplies the stderr log suffix appended to the executable path.
 */
static char g_zApp_StderrLogSuffix[0x05] = ".err";
RECOIL_STATIC_ASSERT(sizeof(g_zApp_StderrLogSuffix) == 0x05);

extern HWND g_RecoilApp_hWndMain;

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-zapp-logfileopenmode
 * @recoil-artifact defines .data recoil:data:0x4da248: g_zApp_LogFileOpenMode.
 *
 * Purpose: supplies the freopen mode used when redirecting stdout and stderr
 * to startup log files.
 */
char g_zApp_LogFileOpenMode[0x02] = "w";
RECOIL_STATIC_ASSERT(sizeof(g_zApp_LogFileOpenMode) == 0x02);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-initstdlogfiles
 * @recoil-artifact defines .text recoil:function:0x4a5780: RecoilApp::InitStdLogFiles.
 * @recoil-match byte
 *
 * Purpose: redirects stdout and stderr to per-run log files and writes their
 * startup banners.
 */
RECOIL_NO_GS void __fastcall RecoilApp::InitStdLogFiles(const char* exePath)
{
    g_RecoilApp_hWndMain = 0;
    if (exePath == 0) {
        return;
    }

    char pathBuf[0x40];
    strcpy(pathBuf, exePath);
    strcat(pathBuf, g_zApp_StderrLogSuffix);
    FILE* stream = freopen(pathBuf, g_zApp_LogFileOpenMode, stderr);
    if (stream == 0 && GetTempPathA(sizeof(pathBuf), pathBuf) != 0) {
        strcat(pathBuf, g_zApp_DefaultStderrLogName);
        stream = freopen(pathBuf, g_zApp_LogFileOpenMode, stderr);
    }
    if (stream != 0) {
        fprintf(stream, g_zApp_LogFileStartBanner);
        fflush(stream);
    }

    strcpy(pathBuf, exePath);
    strcat(pathBuf, g_zApp_StdoutLogSuffix);
    stream = freopen(pathBuf, g_zApp_LogFileOpenMode, stdout);
    if (stream == 0 && GetTempPathA(sizeof(pathBuf), pathBuf) != 0) {
        strcat(pathBuf, g_zApp_DefaultStdoutLogName);
        stream = freopen(pathBuf, g_zApp_LogFileOpenMode, stdout);
    }
    if (stream != 0) {
        fprintf(stream, g_zApp_LogFileStartBanner);
        fflush(stream);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-app-get-message-map
 * @recoil-artifact defines .text recoil:function:0x42de10: RecoilApp::GetMessageMap.
 * @recoil-match byte
 *
 * Purpose: return RecoilApp's authored MFC message map for runtime dispatch.
 */
const AFX_MSGMAP* RecoilApp::GetMessageMap() const
{
    return &g_RecoilApp_MessageMap;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.application-singleton
 * @recoil-artifact defines .data recoil:data:0x4f3ca8: Process-wide application object.
 * @recoil-artifact emits .text recoil:function:0x42de20: VC5 startup coordinator.
 * @recoil-artifact emits .text recoil:function:0x42de30: VC5 application initialization.
 * @recoil-artifact emits .text recoil:function:0x42de40: VC5 atexit registration.
 * @recoil-artifact emits .text recoil:function:0x42de50: VC5 application cleanup.
 * Purpose: Own the concrete application and its normal process lifetime.
 * Retail CRT slot 0x4da080 references the compiler-generated coordinator.
 */
RecoilApp g_RecoilApp;
// ~RecoilApp is compiler-generated: retail's destructor never resets the RecoilApp vptr.

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-app-recoil-app
 * @recoil-artifact defines .text recoil:function:0x42dfa0: RecoilApp::RecoilApp.
 * @recoil-match byte
 *
 * Purpose: Initializes application state after constructing the MFC module base.
 */
RecoilApp::RecoilApp()
    : RecoilApp_MfcOleModule()
{
    m_transitionFadeTimer = 0.0f;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-app-create-main-wnd
 * @recoil-artifact defines .text recoil:function:0x42e110: RecoilApp::CreateMainWnd.
 * @recoil-match byte
 *
 * Purpose: Allocates the application's main Recoil frame window object.
 */
CZRecoilFrame* RecoilApp::CreateMainWnd()
{
    CZRecoilFrame* frame = new CZRecoilFrame;
    if (frame == 0) {
        return 0;
    }
    return frame;
}

namespace zInput {
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.z-input-diset-joystick-enabled
 * @recoil-artifact defines .text recoil:function:0x42e170: zInput::DISetJoystickEnabled.
 * @recoil-match byte
 *
 * Purpose: Enables or disables joystick acquisition and gameplay axis ranges.
 */
int __fastcall DISetJoystickEnabled(int enable)
{
    int result = 0;
    if (enable != 0 && DIIsJoystickDeviceReady() != 0) {
        if (DIGetJoystickRefCount() == 0) {
            DIAddJoystickRef();
        }
        JoystickAxisConfig& cfg = g_zInput_JoystickAxisConfig_Gameplay;
        cfg.axes[0].lMin = -1000;
        cfg.axes[0].lMax = 1000;
        cfg.axes[2].lMax = 1000;
        cfg.axes[3].lMax = 1000;
        cfg.axes[2].lMin = -1000;
        cfg.axes[3].lMin = -1000;
        cfg.axes[1].lMin = -10000;
        cfg.axes[1].lMax = 10000;
        cfg.axes[0].deadzone = 2000;
        cfg.axes[1].deadzone = 3000;
        cfg.axes[2].deadzone = 1500;
        cfg.axes[3].deadzone = 2000;
        DIApplyAxisConfig(&cfg);
        result = 1;
    } else if (DIGetJoystickRefCount() != 0) {
        DIReleaseJoystickRef();
    }
    return result;
}
} // namespace zInput

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-app-start-engine
 * @recoil-artifact defines .text recoil:function:0x42e220: RecoilApp::StartEngine.
 * @recoil-match byte
 *
 * Purpose: Initializes the engine and its startup subsystems for the application window.
 */
RECOIL_NO_GS int RecoilApp::StartEngine(HWND hwnd)
{
    EngineInit(hwnd);
    const int turretResult = zTurret_System::ResetIterationState();
    printf(
        g_RecoilApp_TurretStatusPrintfFmt,
        turretResult == 0 ? g_RecoilApp_StartupStatusPassed : g_RecoilApp_StartupStatusFailed
    );
    zSndSystemInit((RecoilPtr32)((unsigned int)hwnd), g_RecoilApp_SoundsZrdName);
    zSnd::SetAudioApiOption(zSnd::GetActiveBackend());
    if (InitializeDisplay(hwnd) == 0) {
        char caption[0x80];
        strcpy(caption, zLoc::GetMessageString(0x901));
        MessageBoxExA(hwnd, zLoc::GetMessageString(0x1f), caption, MB_ICONHAND, 0);
        return 0;
    }
    zInput::Init(hwnd, g_RecoilApp_hInstance);
    const int height = zOptDisplaySectionGetHeight();
    zInput::MouseSetClientSizeAndCenter(zOptDisplaySectionGetWidth(), height);
    zInput::DISetJoystickEnabled(zInp::GetJoystickOption());
    zOpt_ViewRectSection* const windowSection = zOpt::GetWindowSection();
    HudUiMgr::InitHudLayouts((const HudUiRect*)(zOpt::GetDisplaySection()), (const HudUiRect*)(windowSection));
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-app-initialize-display
 * @recoil-artifact defines .text recoil:function:0x42e330: RecoilApp::InitializeDisplay.
 * @recoil-match byte
 *
 * Purpose: Initializes the configured video mode and rendering surfaces.
 */
int __fastcall RecoilApp::InitializeDisplay(HWND hwnd)
{
    if (zVideo::InitVideoSystem(
            hwnd,
            zVid::GetHwApiOption(),
            zOpt::GetFullscreenOption(),
            zVid::GetVideoModeIndexFromOptions()
        )
        != 0) {
        printf(g_RecoilApp_OpenVideoAbortMsg);
        fflush(stdout);
        return 0;
    }
    if (zVid::GetAccelerationOption() == 0 && zRndr::SpanOcclusionInit(zOpt::GetWindowSectionHeight()) != 0) {
        printf(g_RecoilApp_OpenHseAbortMsg);
        fflush(stdout);
        return 0;
    }
    zRndr::SetFrameBufferRegion(
        zVideo::GetPrimarySurfacePixels(),
        zOpt::GetDisplaySection(),
        zOpt::GetDisplaySectionBitsPerPixel(),
        zVideo::GetPrimarySurfacePitch()
    );
    zRndr::SetVideoStrideMirrors(zOpt::GetVideoStrideValue());
    zVid::InitFrameScratchBuffers();
    const int oldClearState = zVideo::ExchangeClearScreenBufferEnabled(1);
    zVideo::CallClearSwSurfaceAndZBuffer(0, 0);
    zVideo::CallClearPrimarySurfaceAndZBuffer(0);
    zVideo::AdjustSurfacesIfEnabled(0, 0, 1, 1);
    zVideo::CallClearPrimarySurfaceAndZBuffer(0);
    zVideo::AdjustSurfacesIfEnabled(0, 0, 1, 1);
    zVideo::ExchangeClearScreenBufferEnabled(oldClearState);
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-app-shutdown-engine
 * @recoil-artifact defines .text recoil:function:0x42e430: RecoilApp::ShutdownEngine.
 * @recoil-match byte
 *
 * Purpose: Shuts down the active engine, rendering, audio, and gameplay subsystems.
 */
void RecoilApp::ShutdownEngine()
{
    if (zSnd::GetCDAudioOption() != 0) {
        zSndCd::Stop();
    }

    zTurret_System::Shutdown();
    zDEClient::ShutdownGlobals();

    if (zVid::GetAccelerationOption() == 0) {
        zRndr::SpanOcclusionShutdown();
    }

    PickupTypeTable::FreeOptMeta();
    HudUiMgr::ShutdownResources();

    if (zOpt::GetNetworkEnabled() != 0) {
        zNetwork::ShutdownSessionRuntime();
    }

    ShutdownSubsystems();
    zVideo::ShutdownVideoSystem();
    zVideo::ReturnSuccessStub();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-app-load-zbd-and-start-engine
 * @recoil-artifact defines .text recoil:function:0x42e490: RecoilApp::LoadZbdAndStartEngine.
 * @recoil-match byte
 *
 * Purpose: Mounts the startup archive when needed and starts the engine state flow.
 */
int RecoilApp::LoadZbdAndStartEngine()
{
    if (g_HudSensorTracker.missionFlags != 0) {
        zArchive::Mount(g_RecoilApp_StartupArchivePath, 1);
    }

    StartEngineAndQueueStartupState();
    g_HudSensorTracker.RegisterMissionSectionHandlers();
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-app-load-zbd-and-setup-sensor-tracker
 * @recoil-artifact defines .text recoil:function:0x42e4d0: RecoilApp::LoadZbdAndSetupSensorTracker.
 * @recoil-match byte
 *
 * Purpose: Starts the engine and records the selected mission setup in the sensor tracker.
 */
int RecoilApp::LoadZbdAndSetupSensorTracker(int missionId, const char* zbdPath, int skipIntroFmvMode, int missionFlags)
{
    LoadZbdAndStartEngine();
    m_skipIntroFmv = skipIntroFmvMode;
    if (zbdPath != 0) {
        g_HudSensorTracker.SetZbdPath(zbdPath);
        return 1;
    }

    g_HudSensorTracker.InitMissionIdAndFlags(missionId, missionFlags);
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-app-init-instance
 * @recoil-artifact defines .text recoil:function:0x42e520: RecoilApp::InitInstance.
 * @recoil-match byte
 *
 * Purpose: Initializes the process window, application services, and initial game state.
 */
RECOIL_NO_GS int RecoilApp::InitInstance()
{
    if (ActivateExistingInstance() == 0) {
        return 0;
    }

    WNDCLASSA wndClass;
    memset(&wndClass, 0, sizeof(wndClass));
    wndClass.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
    wndClass.lpfnWndProc = DefWindowProcA;
    wndClass.hInstance = AfxGetInstanceHandle();
    wndClass.hIcon = ::LoadIconA(AfxFindResourceHandle((LPCSTR)0x97, (LPCSTR)0x0e), (LPCSTR)0x97);
    wndClass.hCursor = ::LoadCursorA(AfxFindResourceHandle((LPCSTR)0x7f00, (LPCSTR)0x0c), (LPCSTR)0x7f00);
    wndClass.hbrBackground = CreateSolidBrush(0);
    wndClass.lpszMenuName = 0;
    wndClass.lpszClassName = g_RecoilApp_WndClassNamePtr;

    if (AfxRegisterClass(&wndClass) == 0) {
        return 0;
    }

    g_RecoilApp_WindowClassRegistered = 1;
    RecoilApp_MfcOleModule::InitInstance();
    m_reserved148 = 0;
    m_pendingState = &m_introFmvState;

    char errorTextBuffer[0x400];
    char messageCaptionBuffer[0x100];
    char sharedTextBuffer[0x100];
    char registryCompanyNameBuffer[0x100];

    if (zLoc::LoadMessagesDll(g_RecoilApp_MessagesDllName) == 0) {
        char* systemErrorText;
        sprintf(errorTextBuffer, g_RecoilApp_ExitAtFileLineFmt, g_RecoilApp_SourceFile_RecoilAppCpp, 0x188);
        OutputDebugStringA(errorTextBuffer);
        FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            0,
            GetLastError(),
            0x400,
            (LPSTR)(&systemErrorText),
            0,
            0
        );
        strcpy(errorTextBuffer, systemErrorText);
        strcat(errorTextBuffer, g_RecoilApp_DoubleNewline);
        strcat(errorTextBuffer, g_RecoilApp_MessagesDllName);
        LocalFree(systemErrorText);
        zVideo_dd::FlipToGDIIfAttached();
        MessageBeep(MB_ICONASTERISK);
        MessageBoxA(0, errorTextBuffer, "", MB_ICONASTERISK);
        ExitProcess(0);
    }

    zLoc::FormatMessage(messageCaptionBuffer, 0x100, 0x83);
    while (1) {
        if (zSys::FindFileOnDriveType(5, g_RecoilApp_IntroFmvPath, 0) != 0) {
            break;
        }
        MessageBeep(MB_ICONEXCLAMATION);
        if (MessageBoxA(
                g_RecoilApp_hWndMain,
                messageCaptionBuffer,
                zLoc::GetMessageString(0x901),
                MB_OKCANCEL | MB_ICONEXCLAMATION
            )
            != IDOK) {
            ExitProcess(0);
        }
    }

    zSysVideoCapsLevel videoCaps;
    zSysPlatformCapsLevel platformCaps;
    zSys::ProbePlatformAndVideoCaps(&videoCaps, &platformCaps);
    if ((unsigned int)(videoCaps) < (unsigned int)(ZSYS_VIDEO_CAPS_SURFACE4)) {
        zLoc::FormatMessage(messageCaptionBuffer, 0x100, 0x14);
        zLoc::FormatMessage(sharedTextBuffer, 0x100, 0x16);
        MessageBeep(MB_ICONHAND);
        MessageBoxA(g_RecoilApp_hWndMain, sharedTextBuffer, messageCaptionBuffer, MB_ICONHAND);
        ExitProcess(0);
    }

    zGame::ReturnOnlyStub();
    zUtil::ZBDInit();
    zUtil::zRdrInitNodePool(0x200);
    zUtil::zRdrAddSearchPaths(0, g_zUtil_ZbdSearchPathLeaf);
    zUtil::zRdrInit(g_zUtil_ZrdrCommonDataPath);

    strncpy(registryCompanyNameBuffer, zLoc::GetMessageString(0x900), sizeof(registryCompanyNameBuffer));
    strncpy(sharedTextBuffer, zLoc::GetMessageString(0x901), sizeof(sharedTextBuffer));
    zGame::OptionsInitRegistryContext(registryCompanyNameBuffer, sharedTextBuffer, RecoilVersion::GetString());
    zInput::BindMapSystemInit(0x2f);

    if (zGame::OptionsLoadGameOptions() == 0) {
        zArchive::Mount(g_RecoilApp_StartupArchivePath, 1);
        if (zGame::OptionsLoadGameOptions() == 0) {
            strcpy(sharedTextBuffer, zLoc::GetMessageString(0x901));
            MessageBeep(MB_ICONHAND);
            MessageBoxA(g_RecoilApp_hWndMain, zLoc::GetMessageString(0x1e), sharedTextBuffer, MB_ICONHAND);
            ExitProcess(0);
        }
    }

    zVid::SetVideoModeIndex(zVid::GetVideoModeIndexFromOptions());
    CZRecoilFrame* const frame = (CZRecoilFrame*)((unsigned int)(GetMainWnd()));
    frame->ConfigureModeFeatureFlags();
    ((CZRecoilFrame*)((unsigned int)(GetMainWnd())))->InitStartupHwApiFromOptions();
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-app-exit-instance
 * @recoil-artifact defines .text recoil:function:0x42e930: RecoilApp::ExitInstance.
 * @recoil-match byte
 *
 * Purpose: Releases application resources and persists options during process shutdown.
 */
int RecoilApp::ExitInstance()
{
    if (g_RecoilApp_WindowClassRegistered != 0) {
        HINSTANCE instanceHandle = AfxGetModuleState()->m_hCurrentInstanceHandle;
        UnregisterClassA(g_RecoilApp_WndClassNamePtr, instanceHandle);
        zGame::OptionsSaveGameOptions();
        zGame::ReturnOnlyStub();
        zGame::OptionsShutdownRegistryContext();
        zRdrExit();
        zRdrFreeNodePool();
        zUtil::ZBDDestroyGlobalManager();
        zLoc::UnloadMessagesDll();
    }

    zInput::BindMapSystemShutdown();
    ((CWinApp*)(this))->CWinApp::ExitInstance();
    zSys::ExitProcessWithCleanup(0);
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-app-activate-existing-instance
 * @recoil-artifact defines .text recoil:function:0x42e990: RecoilApp::ActivateExistingInstance.
 * @recoil-match byte
 *
 * Purpose: Activates an existing Recoil window or permits this instance to continue.
 */
int RecoilApp::ActivateExistingInstance()
{
    CWnd* const existingWindow = CWnd::FindWindow(g_RecoilApp_WndClassNamePtr, 0);
    if (existingWindow != 0) {
        CWnd* const popup = existingWindow->GetLastActivePopup();
        if (existingWindow->IsIconic()) {
            existingWindow->ShowWindow(SW_RESTORE);
        }

        popup->SetForegroundWindow();
        return 0;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-app-pre-translate-message
 * @recoil-artifact defines .text recoil:function:0x42e9f0: RecoilApp::PreTranslateMessage.
 * @recoil-match byte
 *
 * Purpose: Filters accelerated-mode system-key messages before normal MFC translation.
 */
int RecoilApp::PreTranslateMessage(tagMSG* msg)
{
    int handled = 0;
    if (zVid::GetAccelerationOption() != 0) {
        const UINT message = msg->message;
        if (message >= WM_SYSKEYDOWN && message <= WM_SYSKEYUP) {
            handled = 1;
        }
    }

    return handled;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.crecoil-app-intro-fmv-state-on-try-become-current
 * @recoil-artifact defines .text recoil:function:0x42ea20: CRecoilAppIntroFmvState::OnTryBecomeCurrent.
 * @recoil-match byte
 *
 * Purpose: Configures rendering and prepares the intro FMV state for activation.
 */
int CRecoilAppIntroFmvState::OnTryBecomeCurrent()
{
    zRndr::SetFrameBufferRegion(
        zVideo::GetPrimarySurfacePixels(),
        zOpt::GetWindowSection(),
        zOpt::GetDisplaySectionBitsPerPixel(),
        zVideo::GetPrimarySurfacePitch()
    );
    zRndr::SetVideoStrideMirrors(zOpt::GetVideoStrideValue());

    zVideo::FxSetSurfaceState(
        zVideo::GetPrimarySurfacePixels(),
        zVideo::GetPrimarySurfaceWidth(),
        zVideo::GetPrimarySurfaceHeight(),
        zVideo::GetPrimarySurfacePitch()
    );
    zVid::SetCachedClientRectUpdateMask(1);

    if (g_RecoilApp.m_skipIntroFmv == 0) {
        zFMV_Script* const script = &m_fmv;
        if (g_RecoilApp_hWndMain != 0) {
            script->m_hWnd = g_RecoilApp_hWndMain;
        }

        if (script->LoadActionsFromZrd(g_zFMV_ScriptFileName, g_RecoilApp_IntroFmvTag) != -1) {
            script->BeginAtTime();
        }
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.crecoil-app-intro-fmv-state-on-update-should-quit
 * @recoil-artifact defines .text recoil:function:0x42eac0: CRecoilAppIntroFmvState::OnUpdateShouldQuit.
 * @recoil-match byte
 *
 * Purpose: Advances or skips the intro FMV and queues the mission FMV state.
 */
int CRecoilAppIntroFmvState::OnUpdateShouldQuit()
{
    if (g_RecoilApp.m_skipIntroFmv != 0) {
        g_RecoilApp.QueueSwitchCurrentState(&g_RecoilApp.m_missionFmvState, 0);
        return 0;
    }

    zFMV_Script* const script = &m_fmv;
    const int stateParam = script->UpdateAtTime();
    if (stateParam == 0) {
        g_RecoilApp.QueueSwitchCurrentState(&g_RecoilApp.m_mainMenuPrepState, stateParam);
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-app-fmv-state-on-idle-or-dispatch
 * @recoil-artifact defines .text recoil:function:0x42eb00: RecoilApp_FmvState::OnIdleOrDispatch.
 * @recoil-match byte
 *
 * Purpose: Reports that the FMV state accepts the idle or dispatch callback.
 */
int RecoilApp_FmvState::OnIdleOrDispatch(unsigned int, unsigned int)
{
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.crecoil-app-intro-fmv-state-on-deactivate
 * @recoil-artifact defines .text recoil:function:0x42eb10: CRecoilAppIntroFmvState::OnDeactivate.
 * @recoil-match byte
 *
 * Purpose: Applies the intro FMV's deactivation transition.
 */
void CRecoilAppIntroFmvState::OnDeactivate()
{
    m_fmv.BeginNow(1);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-app-main-menu-prep-state-on-try-become-current
 * @recoil-artifact defines .text recoil:function:0x42eb20: RecoilApp_MainMenuPrepState::OnTryBecomeCurrent.
 * @recoil-match byte
 *
 * Purpose: Configures the video surface and resets main-menu preparation state.
 */
int RecoilApp_MainMenuPrepState::OnTryBecomeCurrent()
{
    zVideo::FxSetSurfaceState(
        zVideo::GetPrimarySurfacePixels(),
        zVideo::GetPrimarySurfaceWidth(),
        zVideo::GetPrimarySurfaceHeight(),
        zVideo::GetPrimarySurfacePitch()
    );
    m_stateData04 = 0;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-app-main-menu-prep-state-on-update-should-quit
 * @recoil-artifact defines .text recoil:function:0x42eb60: RecoilApp_MainMenuPrepState::OnUpdateShouldQuit.
 * @recoil-match byte
 *
 * Purpose: Queues entry to the front-end main menu.
 */
int RecoilApp_MainMenuPrepState::OnUpdateShouldQuit()
{
    RecoilStateMainMenuTransition::QueueEnter(RECOIL_MAINMENU_ROUTE_FRONTEND);
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.crecoil-app-attract-fmv-state-crecoil-app-attract-fmv-state
 * @recoil-artifact defines .text recoil:function:0x42eb70: CRecoilAppAttractFmvState::CRecoilAppAttractFmvState.
 * @recoil-match byte
 *
 * Purpose: Establishes the attract-mode FMV state object.
 */
CRecoilAppAttractFmvState::CRecoilAppAttractFmvState() { }

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.crecoil-app-attract-fmv-state-on-try-become-current
 * @recoil-artifact defines .text recoil:function:0x42ebf0: CRecoilAppAttractFmvState::OnTryBecomeCurrent.
 * @recoil-match byte
 *
 * Purpose: Configures the display and prepares attract-mode FMV playback.
 */
int CRecoilAppAttractFmvState::OnTryBecomeCurrent()
{
    zVideo::FxSetSurfaceState(
        zVideo::GetPrimarySurfacePixels(),
        zVideo::GetPrimarySurfaceWidth(),
        zVideo::GetPrimarySurfaceHeight(),
        zVideo::GetPrimarySurfacePitch()
    );

    GetClientRect(g_RecoilApp_hWndMain, (RECT*)(m_clientRect));

    if (g_RecoilApp_AttractFmvReloadMode != 0) {
        m_fmv.LoadActionsFromZrd(g_zFMV_ScriptFileName, g_RecoilApp_AttractFmvTag);
        g_RecoilApp_AttractFmvReloadMode = 0;
    }

    zFMV_Script* const script = &m_fmv;
    if (g_RecoilApp_hWndMain != 0) {
        script->m_hWnd = g_RecoilApp_hWndMain;
    }

    if (script->LoadActionsFromZrd(g_zFMV_ScriptFileName, g_RecoilApp_AttractFmvTag) != -1) {
        script->BeginAtTime();
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.crecoil-app-attract-fmv-state-on-update-should-quit
 * @recoil-artifact defines .text recoil:function:0x42ec80: CRecoilAppAttractFmvState::OnUpdateShouldQuit.
 * @recoil-match byte
 *
 * Purpose: Advances attract-mode playback and returns to the menu when it finishes.
 */
int CRecoilAppAttractFmvState::OnUpdateShouldQuit()
{
    zFMV_Script* const script = &m_fmv;
    const int stateParam = script->UpdateAtTime();
    if (stateParam == 0) {
        g_RecoilApp.QueueSwitchCurrentState(&g_RecoilApp.m_mainMenuPrepState, stateParam);
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.crecoil-app-attract-fmv-state-on-deactivate
 * @recoil-artifact defines .text recoil:function:0x42eca0: CRecoilAppAttractFmvState::OnDeactivate.
 * @recoil-match byte
 *
 * Purpose: Applies the attract-mode FMV's deactivation transition.
 */
void CRecoilAppAttractFmvState::OnDeactivate()
{
    m_fmv.BeginNow(0);
}

namespace zUtil {
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.z-util-set-mission-zrdr-paths-and-mount-zbd
 * @recoil-artifact defines .text recoil:function:0x42ecb0: zUtil::SetMissionZrdrPathsAndMountZbd.
 * @recoil-match byte
 *
 * Purpose: Rebuilds mission resource search paths and mounts the mission archive.
 */
int __fastcall SetMissionZrdrPathsAndMountZbd(int missionId)
{
    char pathText[256];

    zRdrFreePathList(0);
    zRdrAddSearchPaths(0, g_zUtil_ZbdSearchPathLeaf);
    zImageInitMissionResources(g_zImage_CommonTextureSearchPaths);

    sprintf(pathText, g_zUtil_MissionZrdrSearchPathsFmt, missionId, missionId);
    zRdrSetPath(pathText);

    int result = g_HudSensorTracker.missionFlags;
    if (result != 0) {
        sprintf(pathText, g_zUtil_MissionZrdrArchivePathFmt, missionId);
        result = zArchive::Mount(pathText, 0);
    }

    return result;
}
} // namespace zUtil

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-app-mission-fmv-state-recoil-app-mission-fmv-state
 * @recoil-artifact defines .text recoil:function:0x42ed30: RecoilApp_MissionFmvState::RecoilApp_MissionFmvState.
 * @recoil-match byte
 *
 * Purpose: Initializes the mission FMV selection and skip state.
 */
RecoilApp_MissionFmvState::RecoilApp_MissionFmvState()
{
    m_missionId = 0;
    m_skipMissionFmv = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-app-mission-fmv-state-on-try-become-current
 * @recoil-artifact defines .text recoil:function:0x42edb0: RecoilApp_MissionFmvState::OnTryBecomeCurrent.
 * @recoil-match byte
 *
 * Purpose: Selects the mission, mounts its resources, and prepares mission FMV playback.
 */
int RecoilApp_MissionFmvState::OnTryBecomeCurrent()
{
    if (m_missionId != 0) {
        g_HudSensorTracker.SetMissionId(m_missionId);
    } else {
        m_missionId = g_HudSensorTracker.GetMissionId();
    }

    zUtil::SetMissionZrdrPathsAndMountZbd(m_missionId);

    char missionFmvTag[3];
    memcpy(missionFmvTag, g_RecoilApp_MissionFmvTagTemplate, sizeof(missionFmvTag));
    missionFmvTag[1] = (char)(m_missionId + '0');

    if (m_skipMissionFmv == 0) {
        zFMV_Script* const script = &m_fmv;
        if (g_RecoilApp_hWndMain != 0) {
            script->m_hWnd = g_RecoilApp_hWndMain;
        }

        if (script->LoadActionsFromZrd(g_zFMV_ScriptFileName, missionFmvTag) != -1) {
            script->BeginAtTime();
        }
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.mission-fmv-state-set-mission-id
 * @recoil-artifact defines .text recoil:logical-function:0x42ee40:mission-fmv-state-set-mission-id: RecoilApp_MissionFmvState::SetMissionId.
 * Purpose: Store the mission selected for the next mission-FMV transition.
 */
void RecoilApp_MissionFmvState::SetMissionId(int missionId)
{
    m_missionId = missionId;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-app-mission-fmv-state-on-deactivate
 * @recoil-artifact defines .text recoil:function:0x42ee50: RecoilApp_MissionFmvState::OnDeactivate.
 * @recoil-match byte
 *
 * Purpose: Resets the mission selection and finalizes unskipped FMV playback.
 */
void RecoilApp_MissionFmvState::OnDeactivate()
{
    const int skipMissionFmv = m_skipMissionFmv;
    m_missionId = 0;
    if (skipMissionFmv == 0) {
        m_fmv.BeginNow(1);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-app-mission-fmv-state-on-update-should-quit
 * @recoil-artifact defines .text recoil:function:0x42ee70: RecoilApp_MissionFmvState::OnUpdateShouldQuit.
 * @recoil-match byte
 *
 * Purpose: Switches to gameplay when the mission FMV is skipped or finishes.
 */
int RecoilApp_MissionFmvState::OnUpdateShouldQuit()
{
    if (m_skipMissionFmv != 0 || m_fmv.UpdateAtTime() == 0) {
        g_RecoilApp.QueueSwitchCurrentState(&g_RecoilApp.m_playState, 0);
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.crecoil-app-play-state-crecoil-app-play-state
 * @recoil-artifact defines .text recoil:function:0x42eea0: CRecoilAppPlayState::CRecoilAppPlayState.
 * @recoil-match byte
 *
 * Purpose: Initializes transient gameplay transition and pending-load state.
 */
CRecoilAppPlayState::CRecoilAppPlayState()
{
    m_transitionScratch = 0;
    pPendingLoadGameStartPath = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.crecoil-app-play-state-on-wnd-activate
 * @recoil-artifact defines .text recoil:function:0x42eec0: CRecoilAppPlayState::OnWndActivate.
 * @recoil-match byte
 *
 * Purpose: Reactivates the current HUD layout when the gameplay window gains focus.
 */
void CRecoilAppPlayState::OnWndActivate(int bActivate)
{
    if (bActivate != 0) {
        HudUiMgr::TriggerCurrentLayoutOnActivated();
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.crecoil-app-play-state-on-try-become-current
 * @recoil-artifact defines .text recoil:function:0x42eed0: CRecoilAppPlayState::OnTryBecomeCurrent.
 * @recoil-match byte
 *
 * Purpose: Configures runtime state before the application enters active gameplay.
 */
int CRecoilAppPlayState::OnTryBecomeCurrent()
{
    const int completedObjectiveCount = g_HudSensorTracker.completedObjectiveCount;

    if (zVid::GetAccelerationOption() != 0) {
        BOOL screenSaverRunning;
        SystemParametersInfoA(SPI_SETSCREENSAVERRUNNING, 1, &screenSaverRunning, 0);
    }

    Time::Reset();
    g_FrameDeltaTimeSec = 0.100000001f;

    if (zOpt::GetNetworkEnabled() != 0) {
        HudUiNetExitPanel::CreateGlobal();
    }

    int effectsLevel = zOpt::GetEffectsLevelForCurrentHwMode();
    if (zVid::GetAccelerationOption() == 0 && effectsLevel == 0) {
        effectsLevel = 1;
    }
    zOpt::SetEffectsLevelForCurrentHwMode(effectsLevel);

    HudUiMgr::EnsureHudLoaded("hud.zrd");
    HudUiLoadingCheckpoint::InitTable();
    HudUiLoadingCheckpoint::AdvanceAndLog("Loading common sounds");
    zSndSampleSetInitByName("COMMON");

    Briefing::StartForMission(g_HudSensorTracker.GetMissionId());

    char loadingMessage[0x100];
    zLoc::FormatMessage(loadingMessage, sizeof(loadingMessage), 3, RecoilVersion::GetString());
    HudUiLoadingCheckpoint::AdvanceAndLog(loadingMessage);

    zLoc::FormatMessage(loadingMessage, sizeof(loadingMessage), 5, zVid::GetSelectedHwApiDescriptionOrDefault());
    HudUiLoadingCheckpoint::AdvanceAndLog(loadingMessage);

    zLoc::FormatMessage(loadingMessage, sizeof(loadingMessage), 6, zVid::GetSelectedD3DDeviceNameOrDefault());
    HudUiLoadingCheckpoint::AdvanceAndLog(loadingMessage);

    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x10d));

    g_HudSensorTracker.LoadObjectivesFromPath(g_HudSensorTracker_ObjectivesZrdPath);
    Player::zZarRegisterSections();
    Briefing::BuildObjectiveActionsGlobal(completedObjectiveCount);

    if (g_HudSensorTracker.LoadMissionCoreResources() == 0) {
        return 0;
    }

    g_HudSensorTracker.InitMissionGameplaySystems();
    Briefing::StopAndShutdownThread(1);
    HudUiMgr::ApplyHudModeSwitch(ZOPT_HUD_TYPE_STANDARD);

    if (pPendingLoadGameStartPath != 0) {
        if (g_RecoilApp.m_transitionFadeTimer > 0.0) {
            g_RecoilApp.m_transitionFadeTimer += 5.0f;
        } else {
            g_RecoilApp.m_transitionFadeTimer = 5.0f;
            zOpt::SetMuteSoundOption(1);
        }

        zUtil::zZarLoadFileGlobal(pPendingLoadGameStartPath);
        free(pPendingLoadGameStartPath);
        pPendingLoadGameStartPath = 0;
        g_HudSensorTracker.RunStartAnimsFromZrd("StartAnims.zrd", "LOAD_GAME_START");
    } else {
        g_HudSensorTracker.RunStartAnimsFromZrd("StartAnims.zrd", g_RecoilApp_NewGameStartAnimStateName);
    }

    pRenderSection = zOpt::GetRenderSection();
    pDisplaySection = zOpt::GetDisplaySection();
    pWindowSection = zOpt::GetWindowSection();

    zInput::KeyboardResetTransitionState();
    zInput::MouseRecenterCursor();

    if (zVid::GetAccelerationOption() != 0) {
        CZCamera::SetActiveCamera(0);
        CZCamera::SetObjectHseTestEnabled(0);
    }

    if (g_RecoilApp.m_transitionFadeTimer > 0.0) {
        g_RecoilApp.m_transitionFadeTimer += 1.0f;
    } else {
        g_RecoilApp.m_transitionFadeTimer = 1.0f;
        zOpt::SetMuteSoundOption(1);
    }

    TickAndRenderFrame(0);
    zInput::KeyboardResetTransitionState();
    zInput::MouseRecenterCursor();

    g_zVideo_FrameTick = 0;
    g_RecoilApp.m_reserved148 = 1;
    zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_ENABLED);
    g_HudSensorTracker.ResetHudForMissionStart();

    if (zInput::MouseIsInitialized() != 0) {
        g_zInput_MouseActive = 0;
        zInput::MouseUpdateAcquireState();
    }

    zInput::ResetAllTransitionState();

    zOpt::SetGraphicsFlagsForCurrentHwMode(zOpt::GetGraphicsFlagsForCurrentHwMode());
    zInp::SetJoystickOption(zInput::DISetJoystickEnabled(zInp::GetJoystickOption()));
    zOpt::SetCursorMode(zOpt::GetCursorMode());
    zOpt::SetCameraMode(zOpt::GetCameraModePlayerState());
    zOpt::SetThrottleMode(zOpt::GetThrottleMode());
    zOpt::SetSteeringMode(zOpt::GetSteeringMode());

    if (zSnd::GetCDAudioOption() != 0) {
        const int missionId = g_HudSensorTracker.GetMissionId();
        zSndCd::PlayTrackWithMode((missionId % (zSndCd::GetTrackCount() - 2)) + 2, 5);
    }

    if (zOpt::GetNetworkEnabled() != 0) {
        if (zNetwork::IsHost() == 0) {
            if (g_RecoilApp.m_transitionFadeTimer > 0.0) {
                g_RecoilApp.m_transitionFadeTimer += 5.0f;
            } else {
                g_RecoilApp.m_transitionFadeTimer = 5.0f;
                zOpt::SetMuteSoundOption(1);
            }
        }
        HudUiMgr::EnableTopAndChatStacks();
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.crecoil-app-play-state-tick-and-render-frame
 * @recoil-artifact defines .text recoil:function:0x42f280: CRecoilAppPlayState::TickAndRenderFrame.
 *
 *
 * Purpose: tick input, simulation, rendering, HUD, audio, and presentation for
 * one active play-state frame.
 */
int CRecoilAppPlayState::TickAndRenderFrame(int shouldPresent)
{
    Time::Tick();

    if (g_Player_ActiveDebugScriptAsyncEntry != 0 && zInput::KeyboardWaitForAnyKeyPress(0) != 0) {
        zEffectAnimEntry* const entry = g_Player_ActiveDebugScriptAsyncEntry;
        g_Player_ActiveDebugScriptAsyncEntry = 0;
        zEffect_Anim::NodeActionCallback(entry, 0);
    }

    zInput::PollActiveDevices(1);

    pRenderSection = zOpt::GetRenderSection();
    pDisplaySection = zOpt::GetDisplaySection();
    pWindowSection = zOpt::GetWindowSection();
    CZTypeList::UpdateAllBuckets();

    if (g_RecoilApp_QuitAfterCredits != 0) {
        return 1;
    }

    zSndTick(0);

    if (g_Player_HorizonNodeFollowCameraEnabled != 0 && g_Player_HorizonNode != 0) {
        zVec3 cameraPosition;
        CZNode::GetWorldPosition(g_MainCamera, &cameraPosition);
        CZObject3D::gwObject3DSetPosition(g_Player_HorizonNode, cameraPosition.x, cameraPosition.y, cameraPosition.z);
    }

    const int oldClearState = zVideo::GetClearScreenBufferEnabled();
    const int layoutDelay = HudUiMgr::TickLayoutDelay();
    const int savedClearState = zVideo::ExchangeClearScreenBufferEnabled(oldClearState | layoutDelay);
    zOpt_ViewRectSection* const clearRect = layoutDelay != 0 ? pWindowSection : pRenderSection;

    if (zVid::GetAccelerationOption() != 0) {
        zVideo::CallClearSwSurfaceAndZBuffer((zVidRect32*)(clearRect), (zVidRect32*)(pWindowSection));
    } else {
        zVideo::CallClearPrimarySurfaceAndZBuffer((zVidRect32*)(clearRect));
    }
    zVideo::ExchangeClearScreenBufferEnabled(savedClearState);

    if (zOpt::GetReplicateMode() != 0) {
        zRndr::SetFrameBufferRegion(
            zVideo::GetSwSurfacePixels(),
            pRenderSection,
            zOpt::GetDisplaySectionBitsPerPixel(),
            zVideo::GetSwSurfacePitch()
        );
    } else {
        zRndr::SetFrameBufferRegion(
            zVideo::GetPrimarySurfacePixels(),
            pRenderSection,
            zOpt::GetDisplaySectionBitsPerPixel(),
            zVideo::GetPrimarySurfacePitch()
        );
    }
    CZList::RenderActiveCameras();
    zVideo::FxPass3SetInputRectByIndex(0, (HudUiRect*)(pRenderSection));

    HudUiMgrSensor::GetFxRect(&g_HudUiMgrSensor_FxRectScratch);
    int fxTop;
    int fxBottom;
    if (zOpt::GetReplicateMode() != 0) {
        fxTop = g_HudUiMgrSensor_FxRectScratch.top / 2;
        fxBottom = g_HudUiMgrSensor_FxRectScratch.bottom / 2;
        g_HudUiMgrSensor_FxRectScratch.left = g_HudUiMgrSensor_FxRectScratch.left / 2;
        g_HudUiMgrSensor_FxRectScratch.top = fxTop;
        g_HudUiMgrSensor_FxRectScratch.bottom = fxBottom;
        g_HudUiMgrSensor_FxRectScratch.right = g_HudUiMgrSensor_FxRectScratch.right / 2;
    } else {
        fxBottom = g_HudUiMgrSensor_FxRectScratch.bottom;
        fxTop = g_HudUiMgrSensor_FxRectScratch.top;
    }

    HudUiRect* fxRectOrNull;
    if (fxBottom > pRenderSection->bottomExclusive) {
        if (fxTop < pRenderSection->bottomExclusive) {
            g_HudUiMgrSensor_FxRectScratch.top = pRenderSection->bottomExclusive;
        }
        fxRectOrNull = &g_HudUiMgrSensor_FxRectScratch;
    } else {
        fxRectOrNull = 0;
    }
    zVideo::FxPass3SetInputRectByIndex(1, fxRectOrNull);

    const int quitTransition = zInput::KeyboardGetKeyTransitionState(1) & 3;

    if (zVid::GetAccelerationOption() != 0) {
        zRndr::LensFlareResetSampleQueue();
        g_HudSensorTracker.UpdateObjectiveFlow();
        HudUiMgrSensor::UpdateMarkersAndProgressFromVariantTag(&g_Variant_CurrentTag);
        zVideo::RunPostprocessOnSwBuffer();
        zVideo::FxPass3UpdateLocal(g_FrameDeltaTimeSec);

        if (quitTransition != 0) {
            zVideo::DispatchUnlockSwSurfaceState();
            return 1;
        }

        zRndr::SetActiveRegionSizeFromRect((HudUiRect*)(pWindowSection));
        HudUiMgr::UpdateFrame();
        if (zOpt::GetNetworkEnabled() != 0) {
            HudUiNetExitPanel::Tick();
        }
        zVideo::DispatchUnlockSwSurfaceState();
    } else if (zOpt::GetReplicateMode() != 0) {
        zVideo::RunPostprocessOnSwBuffer();
        zVideo::FxPass3UpdateLocal(g_FrameDeltaTimeSec);
        zVideo::DispatchUnlockSwSurfaceState();

        if (shouldPresent != 0) {
            g_zVideo_pfnBltSwToPrimaryRectDirect((zVidRect32*)(pRenderSection), (zVidRect32*)(pDisplaySection));
        }

        zVideo::RunPostprocessOnPrimaryBuffer();
        if (quitTransition != 0) {
            zVideo::DispatchUnlockPrimarySurfaceState();
            zVideo::AdjustSurfacesIfEnabled(0, 0, 0, 1);
            return 1;
        }

        g_HudSensorTracker.UpdateObjectiveFlow();
        zRndr::SetActiveRegionSizeFromRect((HudUiRect*)(pWindowSection));
        zRndr::LensFlareDrawQueuedSamplesScaled16ClippedFramebuffer(0, 2.0f);
        HudUiMgrSensor::UpdateMarkersAndProgressFromVariantTag(&g_Variant_CurrentTag);
        HudUiMgr::UpdateFrame();
        if (zOpt::GetNetworkEnabled() != 0) {
            HudUiNetExitPanel::Tick();
        }
        zVideo::DispatchUnlockPrimarySurfaceState();
    } else {
        zVideo::RunPostprocessOnPrimaryBuffer();
        zVideo::FxPass3UpdateLocal(g_FrameDeltaTimeSec);

        if (quitTransition != 0) {
            zVideo::DispatchUnlockPrimarySurfaceState();
            zVideo::AdjustSurfacesIfEnabled(0, 0, 0, 1);
            return 1;
        }

        g_HudSensorTracker.UpdateObjectiveFlow();
        zRndr::SetActiveRegionSizeFromRect((HudUiRect*)(pWindowSection));
        zRndr::LensFlareDrawQueuedSamplesScaled16ClippedFramebuffer(0, 1.0f);
        HudUiMgrSensor::UpdateMarkersAndProgressFromVariantTag(&g_Variant_CurrentTag);
        HudUiMgr::UpdateFrame();
        if (zOpt::GetNetworkEnabled() != 0) {
            HudUiNetExitPanel::Tick();
        }
        zVideo::DispatchUnlockPrimarySurfaceState();
    }

    if (shouldPresent != 0) {
        zVideo::AdjustSurfacesIfEnabled((zVidRect32*)(pWindowSection), (zVidRect32*)(pWindowSection), 0, 0);
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.crecoil-app-play-state-on-update-should-quit
 * @recoil-artifact defines .text recoil:function:0x42f5e0: CRecoilAppPlayState::OnUpdateShouldQuit.
 *
 *
 * Purpose: Advances gameplay and completes any active fade-driven state transition.
 */
int CRecoilAppPlayState::OnUpdateShouldQuit()
{
    if (g_RecoilApp.m_transitionFadeTimer > 0.0) {
        g_zVideo_SoftwareModeHotkeyEnabled = ZVIDEO_SOFTWARE_MODE_HOTKEY_DISABLED;
        TickAndRenderFrame(0);

        if (g_RecoilApp.m_transitionFadeTimer > 1.0) {
            const int previousClearState = zVideo::ExchangeClearScreenBufferEnabled(ZVIDEO_CLEAR_SCREEN_BUFFER_ENABLED);
            ((zUtil_SaveGameState*)g_GameStateOrMapTable)->playerState->transitionDamageSuppressed = 1;
            if (zVid::GetAccelerationOption() != 0) {
                zVideo::CallClearSwSurfaceAndZBuffer((zVidRect32*)pWindowSection, (zVidRect32*)pWindowSection);
            } else {
                zVideo::CallClearPrimarySurfaceAndZBuffer((zVidRect32*)pWindowSection);
            }
            zVideo::ExchangeClearScreenBufferEnabled(previousClearState);
        } else {
            const double overlayAlpha
                = g_RecoilApp.m_transitionFadeTimer > 0.0 ? (double)(g_RecoilApp.m_transitionFadeTimer) : 0.0;
            zRndrOverlayRectSubmit(0, 0, overlayAlpha);
        }

        zVideo::AdjustSurfacesIfEnabled((zVidRect32*)pWindowSection, (zVidRect32*)pWindowSection, 0, 0);
        g_RecoilApp.m_transitionFadeTimer -= g_FrameDeltaTimeSec;

        if (g_RecoilApp.m_transitionFadeTimer <= 0.0) {
            zOpt::SetMuteSoundOption(0);
            HudUiMgr::TriggerCurrentLayoutOnActivated();
            ((zUtil_SaveGameState*)g_GameStateOrMapTable)->playerState->transitionDamageSuppressed = 0;
        }

        return 0;
    }

    if (g_RecoilApp_QuitAfterCredits != 0) {
        zSndPlayHandleSnapshot* const snapshot = zSndPlayHandleSnapshot::CreateFromActiveSamples();
        snapshot->StopAllIfPlaying();
        zSndCd::Stop();

        // Retail constructs and destroys the script as an EH-tracked object.
        CRecoilAppFmvScript fmvScript(g_zFMV_ScriptFileName, g_zFMV_GrandPrizeScriptName, 0);
        fmvScript.RunBlocking(0);

        if (g_zVideo_ActiveRendererPath != ZVID_RENDERER_BACKEND_SOFTWARE) {
            g_zVideo_pfnBltSwToPrimaryRectDirect(0, 0);
        }

        zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_DISABLED);
        HudUi::SetInvalidateMode(0);

        {
            zFMV_ActionBlur blurAction(12, 1);

            zFMV_Action* const action = &blurAction;
            action->Begin(0.0);
            while (action->Update(0.0) != 0) { }
            action->End();

            RecoilStateMainMenuTransition::QueueEnter(RECOIL_MAINMENU_ROUTE_FRONTEND);
            RecoilStateCredits::QueuePush();
        }
        return 0;
    }

    g_zVideo_SoftwareModeHotkeyEnabled = ZVIDEO_SOFTWARE_MODE_HOTKEY_ENABLED;
    if (TickAndRenderFrame(1) != 0) {
        if (zOpt::GetNetworkEnabled() != 0) {
            HudUiNetExitPanel::Show();
            return 0;
        }

        zRndr::SetActiveRegionSizeFromRect((HudUiRect*)pWindowSection);
        if (g_RecoilApp_QuitAfterCredits == 0) {
            RecoilStateMainMenuTransition::QueueEnter(RECOIL_MAINMENU_ROUTE_INGAME);
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.crecoil-app-play-state-on-resume
 * @recoil-artifact defines .text recoil:function:0x42f8a0: CRecoilAppPlayState::OnResume.
 * @recoil-match byte
 *
 * Purpose: Restarts mission CD audio when gameplay resumes.
 */
void CRecoilAppPlayState::OnResume(int)
{
    if (zSnd::GetCDAudioOption() != 0) {
        const int missionId = g_HudSensorTracker.GetMissionId();
        zSndCd::PlayTrackWithMode((missionId % (zSndCd::GetTrackCount() - 2)) + 2, 5);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.crecoil-app-play-state-on-deactivate
 * @recoil-artifact defines .text recoil:function:0x42f8e0: CRecoilAppPlayState::OnDeactivate.
 * @recoil-match byte
 *
 * Purpose: Restores system and engine state while leaving active gameplay.
 */
void CRecoilAppPlayState::OnDeactivate()
{
    HudUiLoadingCheckpoint::AdvanceAndLog(g_RecoilApp_LeavingPlayStateMsg);

    if (zVid::GetAccelerationOption() != 0) {
        BOOL screenSaverRunning;
        SystemParametersInfoA(SPI_SETSCREENSAVERRUNNING, 0, &screenSaverRunning, 0);
    }

    zSndCd::Stop();
    zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_DISABLED);

    if (zOpt::GetNetworkEnabled() != 0) {
        HudUiLoadingCheckpoint::AdvanceAndLog(g_RecoilApp_LeavingNetworkingMsg);
        HudUiNetExitPanel::DestroyGlobal();
    }

    if (zOpt::GetNetworkEnabled() == 0) {
        HudUiLoadingCheckpoint::AdvanceAndLog(g_HudLoading_StopAllSoundsMsg);
        zSndPlayHandleSnapshot* const snapshot = zSndPlayHandleSnapshot::CreateFromActiveSamples();
        snapshot->StopAllIfPlaying();
    }

    CRecoilAppFmvScript fmvScript(g_zFMV_ScriptFileName, g_RecoilApp_MissionOverFmvTag, 0);
    fmvScript.RunBlocking(1);

    if (g_RecoilApp.m_missionShutdownMode == RECOILAPP_MISSION_SHUTDOWN_ON_EXIT) {
        g_HudSensorTracker.ShutdownMissionGameplaySystems();
    }

    zRdrUnmount(0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-app-leave-network-state-on-try-become-current
 * @recoil-artifact defines .text recoil:function:0x42f9d0: RecoilApp_LeaveNetworkState::OnTryBecomeCurrent.
 * @recoil-match byte
 *
 * Purpose: Tears down the local network player, engine, and sound backend.
 */
int RecoilApp_LeaveNetworkState::OnTryBecomeCurrent()
{
    zNetworkDPlayDestroyCachedLocalPlayer();
    g_RecoilApp.ShutdownEngine();
    zSndBackend::Shutdown();
    return 1;
}

/**
 * Purpose: finish the application loop after the exit state's shutdown work.
 * Retail dispatch uses the shared return-one callback in this slot.
 */
int RecoilApp_LeaveNetworkState::OnUpdateShouldQuit()
{
    return 1;
}

#include "Battlesport/recoil_app.h"

#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/briefing.h"
#include "Battlesport/hud.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/hud_ui_net_exit_panel.h"
#include "Battlesport/player.h"
#include "Battlesport/recoil_state_main_menu_transition.h"
#include "Battlesport/recoil_version.h"
#include "Battlesport/turret.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "GameZRecoil/zWeapon/zwep.h"
#include "opt_catalog.h"
#include "pickup.h"
#include "zimage.h"

#include <new>

#ifndef SPI_SETSCREENSAVERRUNNING
#define SPI_SETSCREENSAVERRUNNING 0x0061
#endif

#ifdef FormatMessage
#undef FormatMessage
#endif

#include <direct.h>
#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
#endif
#include <deque>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Provider-boundary accessor for imported MFC42 CWinApp protected members; this does not
 * reimplement CWinApp behavior.
 */
class RecoilMfcWinAppAccess : public CWinApp {
public:
    static const AFX_MSGMAP* __stdcall GetMessageMapForRecoilApp();
};

/**
 * Provider-boundary 0x442890: RecoilMfcWinAppAccess::GetMessageMapForRecoilApp.
 * MFC provider-boundary accessor for imported CWinApp message-map metadata.
 * Purpose: exposes CWinApp::messageMap through the callback shape expected by
 * the RecoilApp module message map.
 */
const AFX_MSGMAP* __stdcall RecoilMfcWinAppAccess::GetMessageMapForRecoilApp()
{
    return &CWinApp::messageMap;
}

/**
 * Provider-boundary 0x4428a0: RecoilApp_MfcOleModule::GetMessageMap.
 * Purpose: returns the app-module MFC message map used as RecoilApp's base map.
 */
const AFX_MSGMAP* RecoilApp_MfcOleModule::GetMessageMap() const
{
    return &RecoilApp_MfcOleModule::messageMap;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-mfcolemodule-messageentries
 * @recoil-artifact defines .rdata recoil:data:0x4d2008: RecoilApp_MfcOleModule::messageEntries.
 *
 * Purpose: provide the terminal MFC message-map sentinel for the app-module base.
 */
AFX_MSGMAP_ENTRY const RecoilApp_MfcOleModule::messageEntries[] = {
    { 0, 0, 0, 0, 0, 0 },
};

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-mfcolemodule-messagemap
 * @recoil-artifact defines .rdata recoil:data:0x4d2000: RecoilApp_MfcOleModule::messageMap.
 *
 * Purpose: links the Recoil app-module base to the provider-owned MFC
 * CWinApp message map.
 */
const AFX_MSGMAP RecoilApp_MfcOleModule::messageMap = {
#if defined(_AFXDLL)
    &RecoilApp_MfcOleModule::GetBaseMessageMapForMfc,
#else
    RecoilMfcWinAppAccess::GetMessageMapForRecoilApp(),
#endif
    &RecoilApp_MfcOleModule::messageEntries[0],
};

/**
 *
 * Purpose: provide RecoilApp's terminal MFC message-map sentinel entry.
 */
extern const AFX_MSGMAP_ENTRY g_RecoilApp_MessageEntries[1] = {
    { 0, 0, 0, 0, 0, 0 },
};

/**
 *
 * Purpose: link RecoilApp's message entries to the app-module base
 * message-map accessor used as the retail base-map callback.
 */
extern const AFX_MSGMAP g_RecoilApp_MessageMap = {
#if defined(_AFXDLL)
    &RecoilApp::GetBaseMessageMapForMfc,
#else
    &RecoilApp_MfcOleModule::messageMap,
#endif
    &g_RecoilApp_MessageEntries[0],
};

AFX_MODULE_STATE* __stdcall AfxGetModuleState();
BOOL __stdcall AfxRegisterClass(WNDCLASSA* wndClass);
HINSTANCE __stdcall AfxFindResourceHandle(LPCSTR resourceName, LPCSTR resourceType);

extern "C" char g_HudSensorTracker_ObjectivesZrdPath[0x0f];
extern "C" const char g_HudLoading_StopAllSoundsMsg[0x10];

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-soundszrdname
 * @recoil-artifact defines .data recoil:data:0x4dcad4: g_RecoilApp_SoundsZrdName.
 *
 * Purpose: names the startup sound archive passed to zSndSystem during engine
 * startup.
 */
const char g_RecoilApp_SoundsZrdName[0x0b] = "sounds.zrd";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_SoundsZrdName) == 0x0b);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-turretstatusprintffmt
 * @recoil-artifact defines .data recoil:data:0x4dcae0: g_RecoilApp_TurretStatusPrintfFmt.
 *
 * Purpose: formats the turret subsystem startup status line.
 */
const char g_RecoilApp_TurretStatusPrintfFmt[0x0f] = "turret:    %s\n";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_TurretStatusPrintfFmt) == 0x0f);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-startupstatusfailed
 * @recoil-artifact defines .data recoil:data:0x4dcaf0: g_RecoilApp_StartupStatusFailed.
 *
 * Purpose: supplies the shared failed startup status text.
 */
const char g_RecoilApp_StartupStatusFailed[0x07] = "FAILED";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_StartupStatusFailed) == 0x07);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-startupstatuspassed
 * @recoil-artifact defines .data recoil:data:0x4dcaf8: g_RecoilApp_StartupStatusPassed.
 *
 * Purpose: supplies the shared passed startup status text.
 */
const char g_RecoilApp_StartupStatusPassed[0x07] = "PASSED";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_StartupStatusPassed) == 0x07);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-openhseabortmsg
 * @recoil-artifact defines .data recoil:data:0x4dcb00: g_RecoilApp_OpenHseAbortMsg.
 *
 * Purpose: reports HSE startup failure before aborting display initialization.
 */
const char g_RecoilApp_OpenHseAbortMsg[0x23] = "Error opening HSE... ABORTING RUN\n";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_OpenHseAbortMsg) == 0x23);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-openvideoabortmsg
 * @recoil-artifact defines .data recoil:data:0x4dcb24: g_RecoilApp_OpenVideoAbortMsg.
 *
 * Purpose: reports video startup failure before aborting display initialization.
 */
const char g_RecoilApp_OpenVideoAbortMsg[0x25] = "Error opening video... ABORTING RUN\n";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_OpenVideoAbortMsg) == 0x25);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-startuparchivepath
 * @recoil-artifact defines .data recoil:data:0x4dcb4c: g_RecoilApp_StartupArchivePath.
 *
 * Purpose: names the startup ZRDR archive mounted during app initialization
 * and engine startup.
 */
const char g_RecoilApp_StartupArchivePath[0x0d] = "zbd\\zrdr.zbd";
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-zutil-zrdrcommondatapath
 * @recoil-artifact defines .data recoil:data:0x4dcb5c: g_zUtil_ZrdrCommonDataPath.
 *
 * Purpose: supplies the common ZRDR directory initialized before registry and
 * video setup.
 */
const char g_zUtil_ZrdrCommonDataPath[0x14] = "..\\data\\common\\zrdr";
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-zutil-zbdsearchpathleaf
 * @recoil-artifact defines .data recoil:data:0x4dcb70: g_zUtil_ZbdSearchPathLeaf.
 *
 * Purpose: supplies the leaf archive search path registered during app startup.
 */
const char g_zUtil_ZbdSearchPathLeaf[0x04] = "zbd";
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-introfmvpath
 * @recoil-artifact defines .data recoil:data:0x4dcb74: g_RecoilApp_IntroFmvPath.
 *
 * Purpose: names the startup FMV file probed before display and engine startup.
 */
const char g_RecoilApp_IntroFmvPath[0x13] = "video\\intro_01.avi";
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-doublenewline
 * @recoil-artifact defines .data recoil:data:0x4dcb88: g_RecoilApp_DoubleNewline.
 *
 * Purpose: separates the system failure text from the missing messages DLL name.
 */
const char g_RecoilApp_DoubleNewline[0x03] = "\n\n";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_DoubleNewline) == 0x03);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-exitatfilelinefmt
 * @recoil-artifact defines .data recoil:data:0x4dcb8c: g_RecoilApp_ExitAtFileLineFmt.
 *
 * Purpose: formats the debug trace emitted before the messages DLL failure box.
 */
const char g_RecoilApp_ExitAtFileLineFmt[0x0f] = "Exit at %s:%d\n";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_ExitAtFileLineFmt) == 0x0f);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-sourcefile-recoilappcpp
 * @recoil-artifact defines .data recoil:data:0x4dcb9c: g_RecoilApp_SourceFile_RecoilAppCpp.
 *
 * Purpose: preserves the original RecoilApp.cpp source path used by the failure trace.
 */
const char g_RecoilApp_SourceFile_RecoilAppCpp[0x22] = "D:\\Proj\\Battlesport\\RecoilApp.cpp";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_SourceFile_RecoilAppCpp) == 0x22);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-messagesdllname
 * @recoil-artifact defines .data recoil:data:0x4dcbc0: g_RecoilApp_MessagesDllName.
 *
 * Purpose: names the localization DLL loaded during app initialization.
 */
const char g_RecoilApp_MessagesDllName[0x0d] = "MESSAGES.DLL";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_MessagesDllName) == 0x0d);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-zfmv-scriptfilename
 * @recoil-artifact defines .data recoil:data:0x4dcbd0: g_zFMV_ScriptFileName.
 *
 * Purpose: supplies the FMV script archive path shared by RecoilApp FMV states.
 */
const char g_zFMV_ScriptFileName[0x08] = "fmv.zrd";
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-introfmvtag
 * @recoil-artifact defines .data recoil:data:0x4dcbd8: g_RecoilApp_IntroFmvTag.
 *
 * Purpose: identifies the intro sequence in the FMV script.
 */
const char g_RecoilApp_IntroFmvTag[0x06] = "INTRO";
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-attractfmvtag
 * @recoil-artifact defines .data recoil:data:0x4dcbe0: g_RecoilApp_AttractFmvTag.
 *
 * Purpose: identifies the attract-mode sequence in the FMV script.
 */
const char g_RecoilApp_AttractFmvTag[0x08] = "ATTRACT";
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-missionfmvtagtemplate
 * @recoil-artifact defines .data recoil:data:0x4dcc74: g_RecoilApp_MissionFmvTagTemplate.
 *
 * Purpose: initializes the stack mission-FMV tag before the mission digit is patched in.
 */
const char g_RecoilApp_MissionFmvTagTemplate[0x04] = "M0";
/**
 *
 * Purpose: selects the new-game start-animation node when play state starts
 * without a pending saved-game ZAR.
 */
extern "C" const char g_RecoilApp_NewGameStartAnimStateName[0x0f] = "NEW_GAME_START";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_NewGameStartAnimStateName) == 0x0f);

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-zfmv-grandprizescriptname
 * @recoil-artifact defines .data recoil:data:0x4dccb0: g_zFMV_GrandPrizeScriptName.
 *
 * Purpose: identifies the grand-prize credits FMV script action.
 */
const char g_zFMV_GrandPrizeScriptName[0x0b] = "GRANDPRIZE";
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-missionoverfmvtag
 * @recoil-artifact defines .data recoil:data:0x4dccbc: g_RecoilApp_MissionOverFmvTag.
 *
 * Purpose: identifies the mission-over FMV script action.
 */
const char g_RecoilApp_MissionOverFmvTag[0x0c] = "MISSIONOVER";
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-leavingnetworkingmsg
 * @recoil-artifact defines .data recoil:data:0x4dccc8: g_RecoilApp_LeavingNetworkingMsg.
 *
 * Purpose: labels the networking teardown checkpoint during play-state shutdown.
 */
const char g_RecoilApp_LeavingNetworkingMsg[0x13] = "Leaving Networking";
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-leavingplaystatemsg
 * @recoil-artifact defines .data recoil:data:0x4dccdc: g_RecoilApp_LeavingPlayStateMsg.
 *
 * Purpose: labels the play-state teardown checkpoint.
 */
const char g_RecoilApp_LeavingPlayStateMsg[0x13] = "Leaving Play State";
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-zininitstatusfmt
 * @recoil-artifact defines .data recoil:data:0x4dd520: g_RecoilApp_ZInInitStatusFmt.
 *
 * Purpose: formats the input subsystem startup status line.
 */
const char g_RecoilApp_ZInInitStatusFmt[0x0e] = "zInInit:  %s\n";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_ZInInitStatusFmt) == 0x0e);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-zimginitstatusfmt
 * @recoil-artifact defines .data recoil:data:0x4dd530: g_RecoilApp_ZImgInitStatusFmt.
 *
 * Purpose: formats the image subsystem startup status line.
 */
const char g_RecoilApp_ZImgInitStatusFmt[0x0f] = "zImgInit:  %s\n";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_ZImgInitStatusFmt) == 0x0f);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-zwepinitstatusfmt
 * @recoil-artifact defines .data recoil:data:0x4dd540: g_RecoilApp_ZWepInitStatusFmt.
 *
 * Purpose: formats the weapon subsystem startup status line.
 */
const char g_RecoilApp_ZWepInitStatusFmt[0x0f] = "zWepInit:  %s\n";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_ZWepInitStatusFmt) == 0x0f);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-zutlinitstatusfmt
 * @recoil-artifact defines .data recoil:data:0x4dd550: g_RecoilApp_ZUtlInitStatusFmt.
 *
 * Purpose: formats the utility subsystem startup status line.
 */
const char g_RecoilApp_ZUtlInitStatusFmt[0x0f] = "zUtlInit:  %s\n";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_ZUtlInitStatusFmt) == 0x0f);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-zsndinitstatusfmt
 * @recoil-artifact defines .data recoil:data:0x4dd560: g_RecoilApp_ZSndInitStatusFmt.
 *
 * Purpose: formats the sound subsystem startup status line.
 */
const char g_RecoilApp_ZSndInitStatusFmt[0x0f] = "zSndInit:  %s\n";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_ZSndInitStatusFmt) == 0x0f);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-zrndrinitstatusfmt
 * @recoil-artifact defines .data recoil:data:0x4dd570: g_RecoilApp_ZRndrInitStatusFmt.
 *
 * Purpose: formats the renderer subsystem startup status line.
 */
const char g_RecoilApp_ZRndrInitStatusFmt[0x0f] = "zRndrInit: %s\n";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_ZRndrInitStatusFmt) == 0x0f);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-zeffinitstatusfmt
 * @recoil-artifact defines .data recoil:data:0x4dd580: g_RecoilApp_ZEffInitStatusFmt.
 *
 * Purpose: formats the effect subsystem startup status line.
 */
const char g_RecoilApp_ZEffInitStatusFmt[0x0f] = "zEffInit:  %s\n";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_ZEffInitStatusFmt) == 0x0f);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-gclsinitstatusfmt
 * @recoil-artifact defines .data recoil:data:0x4dd590: g_RecoilApp_GClsInitStatusFmt.
 *
 * Purpose: formats the class subsystem startup status line.
 */
const char g_RecoilApp_GClsInitStatusFmt[0x0f] = "gClsInit:  %s\n";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_GClsInitStatusFmt) == 0x0f);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-gmodinitstatusfmt
 * @recoil-artifact defines .data recoil:data:0x4dd5a0: g_RecoilApp_GModInitStatusFmt.
 *
 * Purpose: formats the model subsystem startup status line.
 */
const char g_RecoilApp_GModInitStatusFmt[0x0f] = "gModInit:  %s\n";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_GModInitStatusFmt) == 0x0f);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fatalgeneralerrormessage
 * @recoil-artifact defines .data recoil:data:0x4dd610: g_RecoilApp_Run_FatalGeneralErrorMessage.
 *
 * Purpose: supplies the catch-all fatal exception dialog message in
 * RecoilApp::Run.
 */
char g_RecoilApp_Run_FatalGeneralErrorMessage[0x29] = "Fatal error, please contact tech support";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FatalGeneralErrorMessage) == 0x29);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-generalerrortitle
 * @recoil-artifact defines .data recoil:data:0x4dd63c: g_RecoilApp_Run_GeneralErrorTitle.
 *
 * Purpose: supplies the catch-all exception dialog title in RecoilApp::Run.
 */
char g_RecoilApp_Run_GeneralErrorTitle[0x0e] = "General Error";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_GeneralErrorTitle) == 0x0e);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fileerrortitle
 * @recoil-artifact defines .data recoil:data:0x4dd64c: g_RecoilApp_Run_FileErrorTitle.
 *
 * Purpose: supplies the CFileException dialog title in RecoilApp::Run.
 */
char g_RecoilApp_Run_FileErrorTitle[0x0b] = "File Error";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FileErrorTitle) == 0x0b);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fileerrorendoffilemessage
 * @recoil-artifact defines .data recoil:data:0x4dd658: g_RecoilApp_Run_FileErrorEndOfFileMessage.
 *
 * Purpose: reports CFileException::endOfFile in RecoilApp::Run.
 */
char g_RecoilApp_Run_FileErrorEndOfFileMessage[0x1d] = "The end of file was reached.";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FileErrorEndOfFileMessage) == 0x1d);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fileerrordiskfullmessage
 * @recoil-artifact defines .data recoil:data:0x4dd678: g_RecoilApp_Run_FileErrorDiskFullMessage.
 *
 * Purpose: reports CFileException::diskFull in RecoilApp::Run.
 */
char g_RecoilApp_Run_FileErrorDiskFullMessage[0x12] = "The disk is full.";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FileErrorDiskFullMessage) == 0x12);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fileerrorlockviolationmessage
 * @recoil-artifact defines .data recoil:data:0x4dd68c: g_RecoilApp_Run_FileErrorLockViolationMessage.
 *
 * Purpose: reports CFileException::lockViolation in RecoilApp::Run.
 */
char g_RecoilApp_Run_FileErrorLockViolationMessage[0x3f]
    = "There was an attempt to lock a region that was already locked.";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FileErrorLockViolationMessage) == 0x3f);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fileerrorsharingviolationmessage
 * @recoil-artifact defines .data recoil:data:0x4dd6cc: g_RecoilApp_Run_FileErrorSharingViolationMessage.
 *
 * Purpose: reports CFileException::sharingViolation in RecoilApp::Run.
 */
char g_RecoilApp_Run_FileErrorSharingViolationMessage[0x39]
    = "SHARE.EXE was not loaded, or a shared region was locked.";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FileErrorSharingViolationMessage) == 0x39);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fileerrorhardiomessage
 * @recoil-artifact defines .data recoil:data:0x4dd708: g_RecoilApp_Run_FileErrorHardIoMessage.
 *
 * Purpose: reports CFileException::hardIO in RecoilApp::Run.
 */
char g_RecoilApp_Run_FileErrorHardIoMessage[0x1c] = "There was a hardware error.";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FileErrorHardIoMessage) == 0x1c);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fileerrorbadseekmessage
 * @recoil-artifact defines .data recoil:data:0x4dd724: g_RecoilApp_Run_FileErrorBadSeekMessage.
 *
 * Purpose: reports CFileException::badSeek in RecoilApp::Run.
 */
char g_RecoilApp_Run_FileErrorBadSeekMessage[0x33] = "There was an error trying to set the file pointer.";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FileErrorBadSeekMessage) == 0x33);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fileerrordirectoryfullmessage
 * @recoil-artifact defines .data recoil:data:0x4dd758: g_RecoilApp_Run_FileErrorDirectoryFullMessage.
 *
 * Purpose: reports CFileException::directoryFull in RecoilApp::Run.
 */
char g_RecoilApp_Run_FileErrorDirectoryFullMessage[0x25] = "There are no more directory entries.";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FileErrorDirectoryFullMessage) == 0x25);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fileerrorremovecurrentdirmessage
 * @recoil-artifact defines .data recoil:data:0x4dd780: g_RecoilApp_Run_FileErrorRemoveCurrentDirMessage.
 *
 * Purpose: reports CFileException::removeCurrentDir in RecoilApp::Run.
 */
char g_RecoilApp_Run_FileErrorRemoveCurrentDirMessage[0x31] = "The current working directory cannot be removed.";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FileErrorRemoveCurrentDirMessage) == 0x31);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fileerrorinvalidfilemessage
 * @recoil-artifact defines .data recoil:data:0x4dd7b4: g_RecoilApp_Run_FileErrorInvalidFileMessage.
 *
 * Purpose: reports CFileException::invalidFile in RecoilApp::Run.
 */
char g_RecoilApp_Run_FileErrorInvalidFileMessage[0x34] = "There was an attempt to use an invalid file handle.";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FileErrorInvalidFileMessage) == 0x34);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fileerroraccessdeniedmessage
 * @recoil-artifact defines .data recoil:data:0x4dd7e8: g_RecoilApp_Run_FileErrorAccessDeniedMessage.
 *
 * Purpose: reports CFileException::accessDenied in RecoilApp::Run.
 */
char g_RecoilApp_Run_FileErrorAccessDeniedMessage[0x20] = "The file could not be accessed.";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FileErrorAccessDeniedMessage) == 0x20);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fileerrortoomanyopenfilesmessage
 * @recoil-artifact defines .data recoil:data:0x4dd808: g_RecoilApp_Run_FileErrorTooManyOpenFilesMessage.
 *
 * Purpose: reports CFileException::tooManyOpenFiles in RecoilApp::Run.
 */
char g_RecoilApp_Run_FileErrorTooManyOpenFilesMessage[0x31] = "The permitted number of open files was exceeded.";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FileErrorTooManyOpenFilesMessage) == 0x31);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fileerrorbadpathmessage
 * @recoil-artifact defines .data recoil:data:0x4dd83c: g_RecoilApp_Run_FileErrorBadPathMessage.
 *
 * Purpose: reports CFileException::badPath in RecoilApp::Run.
 */
char g_RecoilApp_Run_FileErrorBadPathMessage[0x23] = "All or part of the path is invalid";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FileErrorBadPathMessage) == 0x23);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fileerrorfilenotfoundmessage
 * @recoil-artifact defines .data recoil:data:0x4dd860: g_RecoilApp_Run_FileErrorFileNotFoundMessage.
 *
 * Purpose: reports CFileException::fileNotFound in RecoilApp::Run.
 */
char g_RecoilApp_Run_FileErrorFileNotFoundMessage[0x1e] = "The file could not be located";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FileErrorFileNotFoundMessage) == 0x1e);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fileerrorunknownmessage
 * @recoil-artifact defines .data recoil:data:0x4dd880: g_RecoilApp_Run_FileErrorUnknownMessage.
 *
 * Purpose: reports unmapped CFileException causes in RecoilApp::Run.
 */
char g_RecoilApp_Run_FileErrorUnknownMessage[0x0e] = "Unknown error";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FileErrorUnknownMessage) == 0x0e);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fataloutofmemorymessage
 * @recoil-artifact defines .data recoil:data:0x4dd890: g_RecoilApp_Run_FatalOutOfMemoryMessage.
 *
 * Purpose: supplies the CMemoryException dialog message in RecoilApp::Run.
 */
char g_RecoilApp_Run_FatalOutOfMemoryMessage[0x3c] = "Fatal out-of-memory error, Freeing some disk space may help";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FatalOutOfMemoryMessage) == 0x3c);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-memoryerrortitle
 * @recoil-artifact defines .data recoil:data:0x4dd8cc: g_RecoilApp_Run_MemoryErrorTitle.
 *
 * Purpose: supplies the CMemoryException dialog title in RecoilApp::Run.
 */
char g_RecoilApp_Run_MemoryErrorTitle[0x0d] = "Memory Error";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_MemoryErrorTitle) == 0x0d);

extern "C" {
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-hinstance
 * @recoil-artifact defines .data recoil:data:0x4f3ef8: g_RecoilApp_hInstance.
 *
 * Purpose: cache the Recoil application instance handle used by frame dialogs
 * and resource-loading paths.
 */
HINSTANCE g_RecoilApp_hInstance = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-wndclassname
 * @recoil-artifact defines .data recoil:data:0x4dcac8: g_RecoilApp_WndClassName.
 *
 * Purpose: owns the app-shell window class name storage used by the class-name
 * pointer global.
 */
char g_RecoilApp_WndClassName[] = "RecoilClass";
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-wndclassnameptr
 * @recoil-artifact defines .data recoil:data:0x4dcac0: g_RecoilApp_WndClassNamePtr.
 *
 * Purpose: points app-shell window registration and lookup paths at the Recoil
 * frame window class name.
 */
const char* g_RecoilApp_WndClassNamePtr = g_RecoilApp_WndClassName;
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-hwndmain
 * @recoil-artifact defines .data recoil:data:0x4f3eec: g_RecoilApp_hWndMain.
 *
 * Purpose: caches the main Recoil application window handle for app-shell,
 * networking, FMV, and dialog owner paths.
 */
HWND g_RecoilApp_hWndMain = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-windowclassregistered
 * @recoil-artifact defines .data recoil:data:0x4f3ed0: g_RecoilApp_WindowClassRegistered.
 *
 * Purpose: tracks whether RecoilClass has already been registered with MFC.
 */
int g_RecoilApp_WindowClassRegistered = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-attractfmvreloadmode
 * @recoil-artifact defines .data recoil:data:0x4dcac4: g_RecoilApp_AttractFmvReloadMode.
 *
 * Purpose: forces the first attract-mode entry to reload its FMV actions.
 */
int g_RecoilApp_AttractFmvReloadMode = 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-mfcolemodule-destructor-recoilapp-mfcolemodule
 * @recoil-artifact defines .text recoil:function:0x4428b0: RecoilApp_MfcOleModule::~RecoilApp_MfcOleModule.
 * @recoil-match byte
 *
 * Purpose: destroys the app state's chunked queue storage before chaining to the MFC base destructor.
 */
RecoilApp_MfcOleModule::~RecoilApp_MfcOleModule()
{
    // VC5 emits the retail chunk-drain loop from the recovered deque member destructor.
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-mfcolemodule-initinstance
 * @recoil-artifact defines .text recoil:function:0x4429d0: RecoilApp_MfcOleModule::InitInstance.
 * @recoil-match byte
 *
 * Purpose: create, connect, show, and update the primary Recoil frame window.
 */
int RecoilApp_MfcOleModule::InitInstance()
{
    RecoilApp* const app = (RecoilApp*)this;

    Enable3dControls();

    m_pMainWnd = (CWnd*)app->CreateMainWnd();
    CZRecoilFrame* const mainWnd = app->GetMainWnd();
    mainWnd->m_app = app;
    m_pMainWnd->ShowWindow(SW_SHOW);
    UpdateWindow(m_pMainWnd->m_hWnd);
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-takeskipwaitmessage
 * @recoil-artifact defines .text recoil:function:0x442a10: RecoilApp::TakeSkipWaitMessage.
 * @recoil-match byte
 *
 * Purpose: consumes and clears the app-shell skip-wait-message flag.
 */
int RecoilApp::TakeSkipWaitMessage()
{
    const int wasSkipped = m_skipWait;
    m_skipWait = 0;
    return wasSkipped;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-markskipwaitmessage
 * @recoil-artifact defines .text recoil:function:0x442a30: RecoilApp::MarkSkipWaitMessage.
 * @recoil-match byte
 *
 * Purpose: sets the app-shell skip-wait-message flag and returns its prior state.
 */
int RecoilApp::MarkSkipWaitMessage()
{
    const int wasSkipped = m_skipWait;
    m_skipWait = 1;
    return wasSkipped;
}

namespace {
/**
 * Original inline/static helper; no standalone retail function exists.
 * Observed in address-backed callers 0x442a50 and 0x42e220 as the repeated
 * VC5-emitted printf status pattern where zero means startup success.
 *
 * Purpose: print a subsystem startup status line for zero-valued success APIs.
 */
inline void PrintEngineInitZeroStatus(const char* format, int result)
{
    printf(format, result == 0 ? g_RecoilApp_StartupStatusPassed : g_RecoilApp_StartupStatusFailed);
}

/**
 * Evidence: this inline/static status helper has no standalone retail function.
 * Caller evidence: 0x442a50 uses this nonzero-success variant, while 0x42e220
 * shares the same engine-startup status-printing source cluster through the
 * zero-success helper above.
 *
 * Purpose: print a subsystem startup status line for nonzero-valued success APIs.
 */
inline void PrintEngineInitNonzeroStatus(const char* format, int result)
{
    printf(format, result != 0 ? g_RecoilApp_StartupStatusPassed : g_RecoilApp_StartupStatusFailed);
}

} // namespace

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-engineinit
 * @recoil-artifact defines .text recoil:function:0x442a50: RecoilApp::EngineInit.
 * @recoil-match byte
 *
 * Purpose: initialize core engine subsystems and print their startup status
 * lines before frame timing and input state are reset.
 */
int RecoilApp::EngineInit(HWND hwnd)
{
    zUtil::zRdrInitNodePool(0);
    zUtil::zRdrInit(0);

    PrintEngineInitZeroStatus(g_RecoilApp_GModInitStatusFmt, zModelDisplayInit());
    PrintEngineInitZeroStatus(g_RecoilApp_GClsInitStatusFmt, zVideo::ReturnSuccessStub());
    PrintEngineInitZeroStatus(g_RecoilApp_ZEffInitStatusFmt, zEffect::Init());
    PrintEngineInitZeroStatus(g_RecoilApp_ZRndrInitStatusFmt, zRndr::InitGlobals());
    PrintEngineInitNonzeroStatus(
        g_RecoilApp_ZSndInitStatusFmt,
        zSndPreInitializeRuntimeState((RecoilPtr32)((unsigned int)hwnd))
    );
    PrintEngineInitZeroStatus(g_RecoilApp_ZUtlInitStatusFmt, zVideo::ReturnSuccessStub());
    PrintEngineInitZeroStatus(g_RecoilApp_ZWepInitStatusFmt, zWepInit());
    PrintEngineInitZeroStatus(g_RecoilApp_ZImgInitStatusFmt, zImageInit(0));

    if (g_zVideo_ActiveRendererPath == 2) {
        zInput::MouseSetCooperativeLevelFlags(5);
    }

    PrintEngineInitZeroStatus(
        g_RecoilApp_ZInInitStatusFmt,
        zInput::Init((HWND)((unsigned int)(hwnd)), (HINSTANCE)((unsigned int)(m_hInstance)))
    );
    Time::Reset();
    zVid::SetCachedClientRectUpdateMask(1);
    return 1;
}

namespace zSndCd {
void __fastcall OnMciNotify(unsigned int wParam, unsigned int lParam);
}

namespace zDEClient {
int __cdecl ShutdownGlobals();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-shutdownsubsystems
 * @recoil-artifact defines .text recoil:function:0x442bc0: RecoilApp::ShutdownSubsystems.
 * @recoil-match byte
 *
 * Purpose: tear down input, rendering resources, catalogs, models, sound, and
 * mounted ZRDR state during app engine shutdown.
 */
void RecoilApp::ShutdownSubsystems()
{
    zInput::Shutdown();
    zImage::ShutdownSubsystem();
    zRdrShutdownWildcardPath();
    zVid::ShutdownFrameScratchBuffers();
    zEffect::ShutdownAll();
    OptCatalog::Shutdown();
    CZClass::Shutdown();
    zModel_Display::ShutdownThunk();
    zSndSystem::Shutdown();
    zRdrExit();
    zRdrFreeNodePool();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-getmainwnd
 * @recoil-artifact defines .text recoil:function:0x442c00: RecoilApp::GetMainWnd.
 * @recoil-match byte
 *
 * Purpose: returns the main window pointer as the concrete Recoil frame type.
 */
CZRecoilFrame* RecoilApp::GetMainWnd() const
{
    return (CZRecoilFrame*)m_pMainWnd;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-startengineandqueuestartupstate
 * @recoil-artifact defines .text recoil:function:0x442c10: RecoilApp::StartEngineAndQueueStartupState.
 * @recoil-match byte
 *
 * Purpose: starts gameplay systems and queues the pending startup app state.
 */
int RecoilApp::StartEngineAndQueueStartupState()
{
    if (StartEngine(GetMainWnd()->m_hWnd) == 0) {
        ShutdownEngine();
        return ExitInstance();
    }

    m_skipWait = 1;
    m_missionShutdownMode = RECOILAPP_MISSION_SHUTDOWN_ON_EXIT;
    QueueSwitchCurrentState(m_pendingState, 0);
    return 1;
}

/**
 * Inferred inline facade over the canonical VC5 deque empty operation.
 * Purpose: tests whether the recovered state queue has no pending transition items.
 */
inline bool RecoilApp_StateQueue::Empty() const
{
    return empty();
}

/**
 * Inferred inline facade over the canonical VC5 deque front accessor.
 * Purpose: returns the pending transition item at the front of the queue.
 */
inline RecoilApp_StateQueueItem* RecoilApp_StateQueue::Front() const
{
    return front();
}

/**
 * Inferred inline facade over the canonical VC5 deque removal operation.
 * Purpose: removes the pending transition item at the front of the queue.
 */
inline void RecoilApp_StateQueue::PopFront()
{
    pop_front();
}

/**
 * Inferred inline facade over the canonical VC5 deque append operation.
 * Purpose: appends one pending transition item to the queue.
 */
inline void RecoilApp_StateQueue::PushBack(RecoilApp_StateQueueItem* const& item)
{
    push_back(item);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-app-mfc-ole-module-recoil-app-mfc-ole-module
 * @recoil-artifact defines .text recoil:function:0x442c70: RecoilApp_MfcOleModule::RecoilApp_MfcOleModule.
 * @recoil-match byte
 *
 * Purpose: constructs the MFC app subobject and initializes Recoil-owned state host fields.
 */
RecoilApp_MfcOleModule::RecoilApp_MfcOleModule()
    : CWinApp(0)
#if !defined(_AFXDLL)
    , m_recoilPad(0)
#endif
{
    m_skipWait = 0;
    m_pendingState = 0;
    m_currentStateIndex = -1;
    memset(m_stateStack, 0, sizeof(m_stateStack));
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-mfcolemodule-run
 * @recoil-artifact defines .text recoil:function:0x442d00: RecoilApp_MfcOleModule::Run.
 * @recoil-artifact emits .text recoil:function:0x44300b: CMemoryException catch body.
 * @recoil-artifact emits .text recoil:function:0x443029: CMemoryException catch continuation.
 * @recoil-artifact emits .text recoil:function:0x443032: CFileException catch body.
 * @recoil-artifact emits .text recoil:function:0x4430c3: CFileException catch continuation.
 * @recoil-artifact emits .text recoil:function:0x4430cc: CException catch body.
 * @recoil-artifact emits .text recoil:function:0x4430ea: CException catch continuation.
 * @recoil-artifact emits .text recoil:function:0x4430f3: Common compiler-generated EH epilogue.
 *
 *
 * Purpose: runs the app-shell message loop, queued state transitions, and exception dialogs.
 */
int RecoilApp_MfcOleModule::Run()
{
    RecoilApp* const app = (RecoilApp*)this;

    CWinThread::SetThreadPriority(THREAD_PRIORITY_HIGHEST);
    try {
        for (;;) {
            while (PeekMessageA(&m_msgCur, 0, 0, 0, PM_NOREMOVE) != 0) {
                if (PumpMessage() == 0) {
                    return ExitInstance();
                }
            }

            zNetworkDPlay::ReceivePendingMessages(-1);

            RecoilApp_IState* const currentState = app->GetCurrentState();

            if (m_skipWait != 0) {
                if (!m_stateQueue.Empty()) {
                    RecoilApp_StateQueueItem* const item = m_stateQueue.Front();

                    switch (item->m_kind) {
                    case RecoilApp_StateQueueKind_SwitchCurrent:
                        if (item->m_stateObj != 0) {
                            if (currentState != 0) {
                                currentState->OnDeactivate();
                            }

                            if (m_currentStateIndex < 0) {
                                m_currentStateIndex = 0;
                            }
                            if (m_currentStateIndex >= 16) {
                                m_currentStateIndex = 15;
                            }

                            if (item->m_stateObj->OnTryBecomeCurrent() != 0) {
                                m_stateStack[m_currentStateIndex] = item->m_stateObj;
                            } else if (currentState != 0) {
                                currentState->OnTryBecomeCurrent();
                            }
                        }
                        break;

                    case RecoilApp_StateQueueKind_PushState:
                        if (item->m_stateObj != 0) {
                            m_stateStack[m_currentStateIndex]->OnSuspend(item->m_param);

                            if (item->m_stateObj->OnTryBecomeCurrent() != 0) {
                                ++m_currentStateIndex;
                                if (m_currentStateIndex >= 16) {
                                    m_currentStateIndex = 15;
                                }

                                m_stateStack[m_currentStateIndex] = item->m_stateObj;
                            }
                        }
                        break;

                    case RecoilApp_StateQueueKind_ExitCurrent:
                        if (currentState != 0) {
                            currentState->OnDeactivate();
                        }

                        m_stateStack[m_currentStateIndex] = 0;
                        --m_currentStateIndex;
                        if (m_currentStateIndex < 0) {
                            m_currentStateIndex = 0;
                        }

                        m_stateStack[m_currentStateIndex]->OnResume(item->m_param);
                        break;
                    }

                    m_stateQueue.PopFront();
                    delete item;
                    continue;
                }

                if (currentState != 0 && currentState->OnUpdateShouldQuit() != 0) {
                    app->OnAppDeactivate();
                    PostQuitMessage(0);
                }
            } else {
                if (PeekMessageA(&m_msgCur, 0, 0, 0, PM_NOREMOVE) == 0) {
                    WaitMessage();
                }
                continue;
            }
        }
    } catch (CMemoryException* memoryException) {
        ::MessageBoxExA(
            0,
            g_RecoilApp_Run_FatalOutOfMemoryMessage,
            g_RecoilApp_Run_MemoryErrorTitle,
            MB_OK | MB_ICONSTOP,
            0
        );
        ::exit(0);
    } catch (CFileException* fileException) {
        const char* message = g_RecoilApp_Run_FileErrorUnknownMessage;
        switch (fileException->m_cause) {
        case CFileException::none:
        case CFileException::generic:
            message = "";
            break;
        case CFileException::fileNotFound:
            message = g_RecoilApp_Run_FileErrorFileNotFoundMessage;
            break;
        case CFileException::badPath:
            message = g_RecoilApp_Run_FileErrorBadPathMessage;
            break;
        case CFileException::tooManyOpenFiles:
            message = g_RecoilApp_Run_FileErrorTooManyOpenFilesMessage;
            break;
        case CFileException::accessDenied:
            message = g_RecoilApp_Run_FileErrorAccessDeniedMessage;
            break;
        case CFileException::invalidFile:
            message = g_RecoilApp_Run_FileErrorInvalidFileMessage;
            break;
        case CFileException::removeCurrentDir:
            message = g_RecoilApp_Run_FileErrorRemoveCurrentDirMessage;
            break;
        case CFileException::directoryFull:
            message = g_RecoilApp_Run_FileErrorDirectoryFullMessage;
            break;
        case CFileException::badSeek:
            message = g_RecoilApp_Run_FileErrorBadSeekMessage;
            break;
        case CFileException::hardIO:
            message = g_RecoilApp_Run_FileErrorHardIoMessage;
            break;
        case CFileException::sharingViolation:
            message = g_RecoilApp_Run_FileErrorSharingViolationMessage;
            break;
        case CFileException::lockViolation:
            message = g_RecoilApp_Run_FileErrorLockViolationMessage;
            break;
        case CFileException::diskFull:
            message = g_RecoilApp_Run_FileErrorDiskFullMessage;
            break;
        case CFileException::endOfFile:
            message = g_RecoilApp_Run_FileErrorEndOfFileMessage;
            break;
        }
        ::MessageBoxExA(0, message, g_RecoilApp_Run_FileErrorTitle, MB_OK | MB_ICONSTOP, 0);
        ::exit(0);
    } catch (CException* exception) {
        ::MessageBoxExA(
            0,
            g_RecoilApp_Run_FatalGeneralErrorMessage,
            g_RecoilApp_Run_GeneralErrorTitle,
            MB_OK | MB_ICONSTOP,
            0
        );
        ::exit(0);
    }
}

/**
 * Provider boundary MFC message-map helper with no standalone retail function address.
 * Purpose: returns the imported CWinApp base message map for the Recoil app-module
 * message map.
 */
const AFX_MSGMAP* __stdcall RecoilApp_MfcOleModule::GetBaseMessageMapForMfc()
{
    return RecoilMfcWinAppAccess::GetMessageMapForRecoilApp();
}

/**
 * Evidence: the MFC message-map helper is provider-boundary code with no standalone retail function.
 * Source model note: the implicit RecoilApp destructor is modeled by the
 * surrounding class definition.
 * The implementation is the implicit VC5 destructor over embedded state members and the
 * MFC/OLE base.
 * Purpose: returns the imported CWinApp base message map for RecoilApp metadata.
 */
const AFX_MSGMAP* __stdcall RecoilApp::GetBaseMessageMapForMfc()
{
    return &RecoilApp_MfcOleModule::messageMap;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-getcurrentstate
 * @recoil-artifact defines .text recoil:function:0x443140: RecoilApp::GetCurrentState.
 * @recoil-match byte
 *
 * Purpose: returns the active app state when the state-stack index is valid.
 */
RecoilApp_IState* RecoilApp::GetCurrentState() const
{
    if (m_currentStateIndex < 0) {
        return 0;
    }

    if (m_currentStateIndex >= (int)(sizeof(m_stateStack) / sizeof(m_stateStack[0]))) {
        return 0;
    }

    return m_stateStack[m_currentStateIndex];
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-queueswitchcurrentstate
 * @recoil-artifact defines .text recoil:function:0x443160: RecoilApp::QueueSwitchCurrentState.
 * @recoil-match byte
 *
 * Purpose: enqueue a switch-current-state request and run the immediate exit/enter callbacks.
 */
RecoilApp_IState* RecoilApp::QueueSwitchCurrentState(RecoilApp_IState* state, int stateParam)
{
    RecoilApp_IState* const currentState = GetCurrentState();
    RecoilApp_StateQueueItem* item
        = new RecoilApp_StateQueueItem(RecoilApp_StateQueueKind_SwitchCurrent, state, stateParam);
    m_stateQueue.PushBack(item);

    if (currentState != 0) {
        currentState->OnExit();
    }
    state->OnEnter();

    return currentState;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-queuepushstate
 * @recoil-artifact defines .text recoil:function:0x443310: RecoilApp::QueuePushState.
 * @recoil-match byte
 *
 * Purpose: enqueue a push-state request and run the pushed state's enter callback.
 */
RecoilApp_IState* RecoilApp::QueuePushState(RecoilApp_IState* state, int suspendParam)
{
    RecoilApp_IState* const currentState = GetCurrentState();
    RecoilApp_StateQueueItem* item
        = new RecoilApp_StateQueueItem(RecoilApp_StateQueueKind_PushState, state, suspendParam);
    m_stateQueue.PushBack(item);

    state->OnEnter();
    return currentState;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-queueexitcurrentstate
 * @recoil-artifact defines .text recoil:function:0x4434b0: RecoilApp::QueueExitCurrentState.
 * @recoil-match byte
 *
 * Purpose: enqueue an exit-current-state request and run the current state's exit callback.
 */
RecoilApp_IState* RecoilApp::QueueExitCurrentState(int stateParam)
{
    RecoilApp_IState* const currentState = GetCurrentState();
    RecoilApp_StateQueueItem* item = new RecoilApp_StateQueueItem(RecoilApp_StateQueueKind_ExitCurrent, 0, stateParam);
    m_stateQueue.PushBack(item);

    if (currentState != 0) {
        currentState->OnExit();
    }

    return currentState;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-onidleordispatch
 * @recoil-artifact defines .text recoil:function:0x443650: RecoilApp::OnIdleOrDispatch.
 * @recoil-match byte
 *
 * Purpose: handles idle/dispatch notifications for CD sound and the current state.
 */
int RecoilApp::OnIdleOrDispatch(unsigned int wParam, unsigned int lParam)
{
    RecoilApp_IState* const currentState = GetCurrentState();
    zSndCd::OnMciNotify(wParam, lParam);
    if (currentState != 0) {
        return currentState->OnIdleOrDispatch(wParam, lParam);
    }

    return 0;
}

/**
 * Original helper: app-shell with no standalone retail function address.
 * Purpose: marks the message wait loop to skip after app activation.
 */
void RecoilApp::OnAppActivate()
{
    MarkSkipWaitMessage();
}

/**
 * Evidence: this app-shell deactivation helper has no standalone retail function.
 * Purpose: clears the skip-wait flag when the app deactivates.
 */
void RecoilApp::OnAppDeactivate()
{
    TakeSkipWaitMessage();
}

/**
 * Original helper: default state hook with no standalone retail function address.
 * Source model note: default hooks stay out-of-line; the inline interface
 * destructor in recoil_app.h is current implementation state, not ownership
 * evidence for the unresolved HUD 0x407170/0x4ccd50 table packet.
 * Purpose: accepts window-activation notifications for states that do not override them.
 */
void RecoilApp_IState::OnWndActivate(int) { }

/**
 * Evidence: this default no-op enter hook has no standalone retail function.
 * Purpose: supplies the no-op enter callback for states without enter work.
 */
void RecoilApp_IState::OnEnter() { }

/**
 * Evidence: this default transition-permission hook has no standalone retail function.
 * Purpose: allows a state transition to become current by default.
 */
int RecoilApp_IState::OnTryBecomeCurrent()
{
    return 1;
}

/**
 * Evidence: this default quit-query hook has no standalone retail function.
 * Purpose: reports that a default state does not request app shutdown.
 */
int RecoilApp_IState::OnUpdateShouldQuit()
{
    return 0;
}

/**
 * Evidence: this default no-op exit hook has no standalone retail function.
 * Purpose: supplies the no-op exit callback for states without exit work.
 */
void RecoilApp_IState::OnExit() { }

/**
 * Evidence: this default no-op deactivate hook has no standalone retail function.
 * Purpose: supplies the no-op deactivate callback for states without deactivate work.
 */
void RecoilApp_IState::OnDeactivate() { }

/**
 * Evidence: this default suspend hook has no standalone retail function.
 * Purpose: accepts suspend notifications for states that do not override them.
 */
void RecoilApp_IState::OnSuspend(int) { }

/**
 * Evidence: this default resume hook has no standalone retail function.
 * Purpose: accepts resume notifications for states that do not override them.
 */
void RecoilApp_IState::OnResume(int) { }

/**
 * Source model note: the ordinary empty RecoilApp_MainMenuPrepState::OnDeactivate
 * identity represented by the zero-argument no-op fold group at 0x4076f0.
 * Original function address: 0x4076f0.
 * Purpose: accept deactivation after the main-menu preparation state has
 * completed its transition work.
 */
void RecoilApp_MainMenuPrepState::OnDeactivate() { }
