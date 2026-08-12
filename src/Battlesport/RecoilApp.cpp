#include "Battlesport/recoil_app.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/about.h"
#include "Battlesport/briefing.h"
#include "Battlesport/game_net.h"
#include "Battlesport/hud.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/hud_ui_net_exit_panel.h"
#include "Battlesport/pickup.h"
#include "Battlesport/player.h"
#include "Battlesport/recoil_version.h"
#include "Battlesport/net_ui.h"
#include "Battlesport/wol_dialog.h"
#include "Battlesport/recoil_state_main_menu_transition.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "Battlesport/turret.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "GameZRecoil/zInput/zinput.h"
#include "zimage.h"

#include <math.h>
#include <commdlg.h>
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
extern const char g_zUtil_MissionZrdrSearchPathsFmt[0x3d] =
    "..\\data\\common\\zrdr;..\\data\\m%d\\zrdr;..\\data\\m%d\\zrdr\\aipath";
/**
 * Purpose: supplies common texture and effect texture search paths for mission resources.
 */
extern const char g_zImage_CommonTextureSearchPaths[0x38] =
    "..\\data\\common\\textures;..\\data\\common\\effects\\textures";
extern const char *g_RecoilApp_WndClassNamePtr;
extern int g_RecoilApp_WindowClassRegistered;
extern "C" HINSTANCE g_RecoilApp_hInstance;
extern "C" int g_RecoilApp_AttractFmvReloadMode;
extern "C" char g_HudSensorTracker_ObjectivesZrdPath[0x0e];
extern "C" const char g_HudUiMgr_HudArchiveName[0x07];
extern "C" const char g_HudLoading_StopAllSoundsMsg[0x10];

AFX_MODULE_STATE *__stdcall AfxGetModuleState();
BOOL __stdcall AfxRegisterClass(WNDCLASSA *wndClass);
HINSTANCE __stdcall AfxFindResourceHandle(
    LPCSTR resourceName,
    LPCSTR resourceType
);

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

/**
 * Purpose: return RecoilApp's authored MFC message map for runtime dispatch.
 */
const AFX_MSGMAP * RecoilApp::GetMessageMap() const {
    return &g_RecoilApp_MessageMap;
}

/**
 * Purpose: destroy embedded app states before the MFC/OLE module base.
 */
RecoilApp::~RecoilApp() {
}

/**
 * Purpose: Initializes application state after constructing the MFC module base.
 */
RecoilApp::RecoilApp()
    : RecoilApp_MfcOleModule() {
    m_transitionFadeTimer = 0.0f;
}

/**
 * Purpose: Allocates the application's main Recoil frame window object.
 */
CZRecoilFrame * RecoilApp::CreateMainWnd() {
    CZRecoilFrame *frame = new CZRecoilFrame;
    if (frame == 0) {
        return 0;
    }
    return frame;
}

namespace zInput {
/**
 * Purpose: Enables or disables joystick acquisition and gameplay axis ranges.
 */
int __fastcall DI_SetJoystickEnabled(
    int enable
) {
    if (enable != 0 && DI_IsJoystickDeviceReady() != 0) {
        if (DI_GetJoystickRefCount() == 0) {
            DI_AddJoystickRef();
        }
        JoystickAxisConfig &cfg = g_zInput_JoystickAxisConfig_Gameplay;
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
        DI_ApplyAxisConfig(&cfg);
        return 1;
    }
    if (DI_GetJoystickRefCount() != 0) {
        DI_ReleaseJoystickRef();
    }
    return 0;
}
} // namespace zInput

/**
 * Purpose: Initializes the engine and its startup subsystems for the application window.
 */
RECOIL_NO_GS int RecoilApp::StartEngine(
    HWND hwnd
) {
    EngineInit(hwnd);
    const int turretResult = zTurret_System::ResetIterationState();
    printf(
        g_RecoilApp_TurretStatusPrintfFmt,
        turretResult == 0 ? g_RecoilApp_StartupStatusPassed : g_RecoilApp_StartupStatusFailed
    );
    zSndSystem_Init(
        (RecoilPtr32)((unsigned int)hwnd),
        g_RecoilApp_SoundsZrdName
    );
    zSnd::SetAudioApiOption(zSnd::GetActiveBackend());
    if (InitializeDisplay(hwnd) == 0) {
        char caption[0x80];
        strcpy(
            caption,
            zLoc::GetMessageString(0x901)
        );
        MessageBoxExA(
            hwnd,
            zLoc::GetMessageString(0x1f),
            caption,
            MB_ICONHAND,
            0
        );
        return 0;
    }
    zInput::Init(
        hwnd,
        g_RecoilApp_hInstance
    );
    const int height = zOpt_DisplaySection_GetHeight();
    zInput::Mouse_SetClientSizeAndCenter(
        zOpt_DisplaySection_GetWidth(),
        height
    );
    zInput::DI_SetJoystickEnabled(zInp::GetJoystickOption());
    zOpt_ViewRectSection *const windowSection = zOpt::GetWindowSection();
    HudUiMgr::InitHudLayouts(
        (const HudUiRect *)(zOpt::GetDisplaySection()),
        (const HudUiRect *)(windowSection)
    );
    return 1;
}

/**
 * Purpose: Initializes the configured video mode and rendering surfaces.
 */
int __fastcall RecoilApp::InitializeDisplay(
    HWND hwnd
) {
    if (zVideo::InitVideoSystem(
            hwnd,
            zVid::GetHwApiOption(),
            zOpt::GetFullscreenOption(),
            zVid::GetVideoModeIndexFromOptions()
        ) != 0) {
        printf(g_RecoilApp_OpenVideoAbortMsg);
        fflush(stdout);
        return 0;
    }
    if (zVid::GetAccelerationOption() == 0 &&
        zRndr::SpanOcclusionInit(zOpt::GetWindowSectionHeight()) != 0) {
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
    zVideo::CallClearSwSurfaceAndZBuffer(
        0,
        0
    );
    zVideo::CallClearPrimarySurfaceAndZBuffer(0);
    zVideo::AdjustSurfacesIfEnabled(
        0,
        0,
        1,
        1
    );
    zVideo::CallClearPrimarySurfaceAndZBuffer(0);
    zVideo::AdjustSurfacesIfEnabled(
        0,
        0,
        1,
        1
    );
    zVideo::ExchangeClearScreenBufferEnabled(oldClearState);
    return 1;
}

/**
 * Purpose: Shuts down the active engine, rendering, audio, and gameplay subsystems.
 */
void RecoilApp::ShutdownEngine() {
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
 * Purpose: Mounts the startup archive when needed and starts the engine state flow.
 */
int RecoilApp::LoadZbdAndStartEngine() {
    if (g_HudSensorTracker.missionFlags != 0) {
        zArchive::MountIndexArchive(
            g_RecoilApp_StartupArchivePath,
            1
        );
    }

    StartEngineAndQueueStartupState();
    g_HudSensorTracker.RegisterMissionSectionHandlers();
    return 1;
}

/**
 * Purpose: Starts the engine and records the selected mission setup in the sensor tracker.
 */
int RecoilApp::LoadZbdAndSetupSensorTracker(
    int missionId,
    const char *zbdPath,
    int skipIntroFmvMode,
    int missionFlags
) {
    LoadZbdAndStartEngine();
    m_skipIntroFmv = skipIntroFmvMode;
    if (zbdPath != 0) {
        g_HudSensorTracker.SetZbdPath(zbdPath);
        return 1;
    }

    g_HudSensorTracker.InitMissionIdAndFlags(
        missionId,
        missionFlags
    );
    return 1;
}

/**
 * Purpose: Initializes the process window, application services, and initial game state.
 */
RECOIL_NO_GS int RecoilApp::InitInstance() {
    if (ActivateExistingInstance() == 0) {
        return 0;
    }

    WNDCLASSA wndClass = {0};
    wndClass.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
    wndClass.lpfnWndProc = DefWindowProcA;
    wndClass.hInstance = AfxGetModuleState()->m_hCurrentInstanceHandle;
    wndClass.hIcon =
        ::LoadIconA(
            AfxFindResourceHandle(
                (LPCSTR)0x97,
                (LPCSTR)0x0e
            ),
            (LPCSTR)0x97
        );
    wndClass.hCursor = ::LoadCursorA(
        AfxFindResourceHandle(
            (LPCSTR)0x7f00,
            (LPCSTR)0x0c
        ),
        (LPCSTR)0x7f00
    );
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
        char *systemErrorText = 0;
        sprintf(
            errorTextBuffer,
            g_RecoilApp_ExitAtFileLineFmt,
            g_RecoilApp_SourceFile_RecoilAppCpp,
            0x188
        );
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
        strcpy(
            errorTextBuffer,
            systemErrorText
        );
        strcat(
            errorTextBuffer,
            g_RecoilApp_DoubleNewline
        );
        strcat(
            errorTextBuffer,
            g_RecoilApp_MessagesDllName
        );
        LocalFree(systemErrorText);
        zVideo_dd::FlipToGDIIfAttached();
        MessageBeep(MB_ICONASTERISK);
        MessageBoxA(
            0,
            errorTextBuffer,
            "",
            MB_ICONASTERISK
        );
        ExitProcess(0);
    }

    zLoc::FormatMessage(
        messageCaptionBuffer,
        0x100,
        0x83
    );
    while (zSys::FindFileOnDriveType(
        5,
        g_RecoilApp_IntroFmvPath,
        0
    ) == 0) {
        MessageBeep(MB_ICONEXCLAMATION);
        if (MessageBoxA(
                g_RecoilApp_hWndMain,
                messageCaptionBuffer,
                zLoc::GetMessageString(0x901),
                MB_OKCANCEL | MB_ICONEXCLAMATION
            ) != IDOK) {
            ExitProcess(0);
        }
    }

    zSysVideoCapsLevel videoCaps = ZSYS_VIDEO_CAPS_NONE;
    zSysPlatformCapsLevel platformCaps = ZSYS_PLATFORM_CAPS_UNSUPPORTED;
    zSys::ProbePlatformAndVideoCaps(
        &videoCaps,
        &platformCaps
    );
    if ((unsigned int)(videoCaps) < (unsigned int)(ZSYS_VIDEO_CAPS_SURFACE4)) {
        zLoc::FormatMessage(
            messageCaptionBuffer,
            0x100,
            0x14
        );
        zLoc::FormatMessage(
            sharedTextBuffer,
            0x100,
            0x16
        );
        MessageBeep(MB_ICONHAND);
        MessageBoxA(
            g_RecoilApp_hWndMain,
            sharedTextBuffer,
            messageCaptionBuffer,
            MB_ICONHAND
        );
        ExitProcess(0);
    }

    zGame::ReturnOnlyStub();
    zUtil::ZBD_Init();
    zUtil::ZRDR_PreallocNodePool(0x200);
    zUtil::ZRDR_AddSearchPaths(
        0,
        g_zUtil_ZbdSearchPathLeaf
    );
    zUtil::ZRDR_Init(g_zUtil_ZrdrCommonDataPath);

    strncpy(
        registryCompanyNameBuffer,
        zLoc::GetMessageString(0x900),
        sizeof(registryCompanyNameBuffer)
    );
    strncpy(
        sharedTextBuffer,
        zLoc::GetMessageString(0x901),
        sizeof(sharedTextBuffer)
    );
    zGame::Options_InitRegistryContext(
        registryCompanyNameBuffer,
        sharedTextBuffer,
        RecoilVersion::GetString()
    );
    zInput::BindMapSystem_Init(0x2f);

    if (zGame::Options_LoadGameOptions() == 0) {
        zArchive::MountIndexArchive(
            g_RecoilApp_StartupArchivePath,
            1
        );
        if (zGame::Options_LoadGameOptions() == 0) {
            strcpy(
                sharedTextBuffer,
                zLoc::GetMessageString(0x901)
            );
            MessageBeep(MB_ICONHAND);
            MessageBoxA(
                g_RecoilApp_hWndMain,
                zLoc::GetMessageString(0x1e),
                sharedTextBuffer,
                MB_ICONHAND
            );
            ExitProcess(0);
        }
    }

    zVid::SetVideoModeIndex(zVid::GetVideoModeIndexFromOptions());
    CZRecoilFrame *const frame = (CZRecoilFrame *)((unsigned int)(GetMainWnd()));
    frame->ConfigureModeFeatureFlags();
    ((CZRecoilFrame *)((unsigned int)(GetMainWnd())))->InitStartupHwApiFromOptions();
    return 1;
}

/**
 * Purpose: Releases application resources and persists options during process shutdown.
 */
int RecoilApp::ExitInstance() {
    if (g_RecoilApp_WindowClassRegistered != 0) {
        HINSTANCE instanceHandle = AfxGetModuleState()->m_hCurrentInstanceHandle;
        UnregisterClassA(
            g_RecoilApp_WndClassNamePtr,
            instanceHandle
        );
        zGame::Options_SaveGameOptions();
        zGame::ReturnOnlyStub();
        zGame::Options_ShutdownRegistryContext();
        zUtil_ZRDR_Shutdown();
        zUtil_ZRDR_FreeNodePool();
        zUtil::ZBD_DestroyGlobalManager();
        zLoc::UnloadMessagesDll();
    }

    zInput::BindMapSystem_Shutdown();
    ((CWinApp *)(this))->CWinApp::ExitInstance();
    zSys::ExitProcessWithCleanup(0);
    return 0;
}

/**
 * Purpose: Activates an existing Recoil window or permits this instance to continue.
 */
int RecoilApp::ActivateExistingInstance() {
    CWnd *const existingWindow = CWnd::FromHandle(FindWindowA(
        g_RecoilApp_WndClassNamePtr,
        0
    ));
    if (existingWindow != 0) {
        CWnd *const popup = CWnd::FromHandle(GetLastActivePopup(existingWindow->m_hWnd));
        if (IsIconic(existingWindow->m_hWnd) != 0) {
            existingWindow->ShowWindow(SW_RESTORE);
        }

        SetForegroundWindow(popup->m_hWnd);
        return 0;
    }

    return 1;
}

/**
 * Purpose: Filters accelerated-mode system-key messages before normal MFC translation.
 */
int RecoilApp::PreTranslateMessage(
    tagMSG *msg
) {
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
 * Purpose: Configures rendering and prepares the intro FMV state for activation.
 */
int RecoilApp_IntroFmvState::OnTryBecomeCurrent() {
    zRndr::SetFrameBufferRegion(
        zVideo::GetPrimarySurfacePixels(),
        zOpt::GetWindowSection(),
        zOpt::GetDisplaySectionBitsPerPixel(),
        zVideo::GetPrimarySurfacePitch()
    );
    zRndr::SetVideoStrideMirrors(zOpt::GetVideoStrideValue());

    zVideo::Fx_SetSurfaceState(
        zVideo::GetPrimarySurfacePixels(),
        zVideo::GetPrimarySurfaceWidth(),
        zVideo::GetPrimarySurfaceHeight(),
        zVideo::GetPrimarySurfacePitch()
    );
    zVid::SetCachedClientRectUpdateMask(1);

    if (g_RecoilApp.m_skipIntroFmv == 0) {
        zFMV_Script *const script = &m_fmv;
        if (g_RecoilApp_hWndMain != 0) {
            script->m_hWnd = g_RecoilApp_hWndMain;
        }

        if (script->LoadActionsFromZrd(
            g_zFMV_ScriptFileName,
            g_RecoilApp_IntroFmvTag
        ) != -1) {
            script->BeginAtTime();
        }
    }

    return 1;
}

/**
 * Purpose: Advances or skips the intro FMV and queues the mission FMV state.
 */
int RecoilApp_IntroFmvState::OnUpdateShouldQuit() {
    if (g_RecoilApp.m_skipIntroFmv != 0) {
        g_RecoilApp.QueueSwitchCurrentState(
            &g_RecoilApp.m_missionFmvState,
            0
        );
        return 0;
    }

    zFMV_Script *const script = &m_fmv;
    const int stateParam = script->UpdateAtTime();
    if (stateParam == 0) {
        g_RecoilApp.QueueSwitchCurrentState(
            &g_RecoilApp.m_mainMenuPrepState,
            stateParam
        );
    }

    return 0;
}

/**
 * Purpose: Reports that the FMV state accepts the idle or dispatch callback.
 */
int RecoilApp_FmvState::OnIdleOrDispatch(
    unsigned int,
    unsigned int
) {
    return 1;
}

/**
 * Purpose: Applies the intro FMV's deactivation transition.
 */
void RecoilApp_IntroFmvState::OnDeactivate() {
    m_fmv.BeginNow(1);
}

/**
 * Purpose: Configures the video surface and resets main-menu preparation state.
 */
int RecoilApp_MainMenuPrepState::OnTryBecomeCurrent() {
    zVideo::Fx_SetSurfaceState(
        zVideo::GetPrimarySurfacePixels(),
        zVideo::GetPrimarySurfaceWidth(),
        zVideo::GetPrimarySurfaceHeight(),
        zVideo::GetPrimarySurfacePitch()
    );
    m_stateData04 = 0;
    return 1;
}

/**
 * Purpose: Queues entry to the front-end main menu.
 */
int RecoilApp_MainMenuPrepState::OnUpdateShouldQuit() {
    RecoilStateMainMenuTransition::QueueEnter(RECOIL_MAINMENU_ROUTE_FRONTEND);
    return 0;
}

/**
 * Purpose: Establishes the attract-mode FMV state object.
 */
RecoilApp_AttractFmvState::RecoilApp_AttractFmvState() {
}

/**
 * Purpose: Configures the display and prepares attract-mode FMV playback.
 */
int RecoilApp_AttractFmvState::OnTryBecomeCurrent() {
    zVideo::Fx_SetSurfaceState(
        zVideo::GetPrimarySurfacePixels(),
        zVideo::GetPrimarySurfaceWidth(),
        zVideo::GetPrimarySurfaceHeight(),
        zVideo::GetPrimarySurfacePitch()
    );

    GetClientRect(
        g_RecoilApp_hWndMain,
        (RECT *)(m_clientRect)
    );

    if (g_RecoilApp_AttractFmvReloadMode != 0) {
        m_fmv.LoadActionsFromZrd(
            g_zFMV_ScriptFileName,
            g_RecoilApp_AttractFmvTag
        );
        g_RecoilApp_AttractFmvReloadMode = 0;
    }

    zFMV_Script *const script = &m_fmv;
    if (g_RecoilApp_hWndMain != 0) {
        script->m_hWnd = g_RecoilApp_hWndMain;
    }

    if (script->LoadActionsFromZrd(
        g_zFMV_ScriptFileName,
        g_RecoilApp_AttractFmvTag
    ) != -1) {
        script->BeginAtTime();
    }

    return 1;
}

/**
 * Purpose: Advances attract-mode playback and returns to the menu when it finishes.
 */
int RecoilApp_AttractFmvState::OnUpdateShouldQuit() {
    zFMV_Script *const script = &m_fmv;
    const int stateParam = script->UpdateAtTime();
    if (stateParam == 0) {
        g_RecoilApp.QueueSwitchCurrentState(
            &g_RecoilApp.m_mainMenuPrepState,
            stateParam
        );
    }

    return 0;
}

/**
 * Purpose: Applies the attract-mode FMV's deactivation transition.
 */
void RecoilApp_AttractFmvState::OnDeactivate() {
    m_fmv.BeginNow(0);
}

namespace zUtil {
/**
 * Purpose: Rebuilds mission resource search paths and mounts the mission archive.
 */
int __fastcall SetMissionZrdrPathsAndMountZbd(
    int missionId
) {
    char pathText[256];

    zUtil_ZRDR_FreePathList(0);
    ZRDR_AddSearchPaths(
        0,
        "zbd"
    );
    zImage_InitMissionResources(g_zImage_CommonTextureSearchPaths);

    sprintf(
        pathText,
        g_zUtil_MissionZrdrSearchPathsFmt,
        missionId,
        missionId
    );
    zUtil_ZRDR_SetSearchPath(pathText);

    if (g_HudSensorTracker.missionFlags == 0) {
        return 0;
    }

    sprintf(
        pathText,
        g_zUtil_MissionZrdrArchivePathFmt,
        missionId
    );
    return zArchive::MountIndexArchive(
        pathText,
        0
    );
}
} // namespace zUtil

/**
 * Purpose: Initializes the mission FMV selection and skip state.
 */
RecoilApp_MissionFmvState::RecoilApp_MissionFmvState() {
    m_missionId = 0;
    m_skipMissionFmv = 0;
}

/**
 * Purpose: Selects the mission, mounts its resources, and prepares mission FMV playback.
 */
int RecoilApp_MissionFmvState::OnTryBecomeCurrent() {
    if (m_missionId == 0) {
        m_missionId = g_HudSensorTracker.GetMissionId();
    } else {
        g_HudSensorTracker.SetMissionId(m_missionId);
    }

    zUtil::SetMissionZrdrPathsAndMountZbd(m_missionId);

    char missionFmvTag[sizeof(g_RecoilApp_MissionFmvTagTemplate)];
    memcpy(
        missionFmvTag,
        g_RecoilApp_MissionFmvTagTemplate,
        sizeof(missionFmvTag)
    );
    missionFmvTag[1] = (char)(m_missionId + '0');

    if (m_skipMissionFmv == 0) {
        zFMV_Script *const script = &m_fmv;
        if (g_RecoilApp_hWndMain != 0) {
            script->m_hWnd = g_RecoilApp_hWndMain;
        }

        if (script->LoadActionsFromZrd(
            g_zFMV_ScriptFileName,
            missionFmvTag
        ) != -1) {
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
void RecoilApp_MissionFmvState::SetMissionId(
    int missionId
) {
    m_missionId = missionId;
}

/**
 * Purpose: Resets the mission selection and finalizes unskipped FMV playback.
 */
void RecoilApp_MissionFmvState::OnDeactivate() {
    const int skipMissionFmv = m_skipMissionFmv;
    m_missionId = 0;
    if (skipMissionFmv == 0) {
        m_fmv.BeginNow(1);
    }
}

/**
 * Purpose: Switches to gameplay when the mission FMV is skipped or finishes.
 */
int RecoilApp_MissionFmvState::OnUpdateShouldQuit() {
    if (m_skipMissionFmv != 0 || m_fmv.UpdateAtTime() == 0) {
        g_RecoilApp.QueueSwitchCurrentState(
            &g_RecoilApp.m_playState,
            0
        );
    }

    return 0;
}

/**
 * Purpose: Initializes transient gameplay transition and pending-load state.
 */
RecoilApp_PlayState::RecoilApp_PlayState() {
    m_transitionScratch = 0;
    pPendingLoadGameStartPath = 0;
}

/**
 * Purpose: Reactivates the current HUD layout when the gameplay window gains focus.
 */
void RecoilApp_PlayState::OnWndActivate(
    int bActivate
) {
    if (bActivate != 0) {
        HudUiMgr::TriggerCurrentLayoutOnActivated();
    }
}

/**
 * Purpose: Configures runtime state before the application enters active gameplay.
 */
int RecoilApp_PlayState::OnTryBecomeCurrent() {
    const int completedObjectiveCount = g_HudSensorTracker.completedObjectiveCount;

    if (zVid::GetAccelerationOption() != 0) {
        BOOL screenSaverRunning = FALSE;
        SystemParametersInfoA(
            SPI_SETSCREENSAVERRUNNING,
            1,
            &screenSaverRunning,
            0
        );
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

    HudUiMgr::EnsureHudLoaded(g_HudUiMgr_HudArchiveName);
    HudUiLoadingCheckpoint::InitTable();
    HudUiLoadingCheckpoint::AdvanceAndLog(g_RecoilApp_LoadingCommonSoundsMsg);
    zSndSampleSet_InitByName(g_RecoilApp_CommonSoundsSampleSetName);

    Briefing::StartForMission(g_HudSensorTracker.GetMissionId());

    char loadingMessage[0x100];
    zLoc::FormatMessage(
        loadingMessage,
        sizeof(loadingMessage),
        3,
        RecoilVersion::GetString()
    );
    HudUiLoadingCheckpoint::AdvanceAndLog(loadingMessage);

    zLoc::FormatMessage(
        loadingMessage,
        sizeof(loadingMessage),
        5,
        zVid::GetSelectedHwApiDescriptionOrDefault()
    );
    HudUiLoadingCheckpoint::AdvanceAndLog(loadingMessage);

    zLoc::FormatMessage(
        loadingMessage,
        sizeof(loadingMessage),
        6,
        zVid::GetSelectedD3DDeviceNameOrDefault()
    );
    HudUiLoadingCheckpoint::AdvanceAndLog(loadingMessage);

    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x10d));

    g_HudSensorTracker.LoadObjectivesFromPath(g_HudSensorTracker_ObjectivesZrdPath);
    Player::ZAR_RegisterSections();
    Briefing::BuildObjectiveActionsGlobal(completedObjectiveCount);

    if (g_HudSensorTracker.LoadMissionCoreResources() == 0) {
        return 0;
    }

    g_HudSensorTracker.InitMissionGameplaySystems();
    Briefing::StopAndShutdownThread(1);
    HudUiMgr::ApplyHudModeSwitch(ZOPT_HUD_TYPE_STANDARD);

    const char *startAnimNodeName;
    if (pPendingLoadGameStartPath != 0) {
        if (g_RecoilApp.m_transitionFadeTimer > 0.0) {
            g_RecoilApp.m_transitionFadeTimer += 5.0f;
        } else {
            g_RecoilApp.m_transitionFadeTimer = 5.0f;
            zOpt::SetMuteSoundOption(1);
        }

        char *const pendingLoadPath = pPendingLoadGameStartPath;
        zUtil::ZAR_LoadFileGlobal(pendingLoadPath);
        free(pendingLoadPath);
        pPendingLoadGameStartPath = 0;
        startAnimNodeName = g_RecoilApp_LoadGameStartAnimStateName;
    } else {
        startAnimNodeName = g_RecoilApp_NewGameStartAnimStateName;
    }

    g_HudSensorTracker.RunStartAnimsFromZrd(
        g_HudSensorTracker_StartAnimsZrdPath,
        startAnimNodeName
    );

    pRenderSection = zOpt::GetRenderSection();
    pDisplaySection = zOpt::GetDisplaySection();
    pWindowSection = zOpt::GetWindowSection();

    zInput::Keyboard_ResetTransitionState();
    zInput::Mouse_RecenterCursor();

    if (zVid::GetAccelerationOption() != 0) {
        zClass_Camera::SetActiveCamera(0);
        zClass_Camera::SetObjectHseTestEnabled(0);
    }

    if (g_RecoilApp.m_transitionFadeTimer > 0.0) {
        g_RecoilApp.m_transitionFadeTimer += 1.0f;
    } else {
        g_RecoilApp.m_transitionFadeTimer = 1.0f;
        zOpt::SetMuteSoundOption(1);
    }

    TickAndRenderFrame(0);
    zInput::Keyboard_ResetTransitionState();
    zInput::Mouse_RecenterCursor();

    g_zVideo_FrameTick = 0;
    g_RecoilApp.m_reserved148 = 1;
    zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_ENABLED);
    g_HudSensorTracker.ResetHudForMissionStart();

    if (zInput::Mouse_IsInitialized() != 0) {
        g_zInput_MouseActive = 0;
        zInput::Mouse_UpdateAcquireState();
    }

    zInput::ResetAllTransitionState();

    zOpt::SetGraphicsFlagsForCurrentHwMode(zOpt::GetGraphicsFlagsForCurrentHwMode());
    zInp::SetJoystickOption(zInput::DI_SetJoystickEnabled(zInp::GetJoystickOption()));
    zOpt::SetCursorMode(zOpt::GetCursorMode());
    zOpt::SetCameraMode(zOpt::GetCameraModePlayerState());
    zOpt::SetThrottleMode(zOpt::GetThrottleMode());
    zOpt::SetSteeringMode(zOpt::GetSteeringMode());

    if (zSnd::GetCDAudioOption() != 0) {
        const int missionId = g_HudSensorTracker.GetMissionId();
        const int trackCount = zSndCd::GetTrackCount();
        zSndCd::PlayTrackWithMode(
            (missionId % (trackCount - 2)) + 2,
            5
        );
    }

    if (zOpt::GetNetworkEnabled() != 0) {
        if (zNetwork::IsHost() == 0) {
            if (g_RecoilApp.m_transitionFadeTimer > 0.0) {
                g_RecoilApp.m_transitionFadeTimer += 5.0f;
            } else {
                g_RecoilApp.m_transitionFadeTimer = 5.0f;
                zOpt::SetMuteSoundOption(1);
            }
            HudUiMgr::EnableTopAndChatStacks();
            return 1;
        }

        HudUiMgr::EnableTopAndChatStacks();
    }

    return 1;
}

/**
 *
 * Purpose: tick input, simulation, rendering, HUD, audio, and presentation for
 * one active play-state frame.
 */
int RecoilApp_PlayState::TickAndRenderFrame(
    int shouldPresent
) {
    Time::Tick();

    if (g_Player_ActiveDebugScriptAsyncEntry != 0 && zInput::Keyboard_WaitForAnyKeyPress(0) != 0) {
        zEffectAnimEntry *const entry = g_Player_ActiveDebugScriptAsyncEntry;
        g_Player_ActiveDebugScriptAsyncEntry = 0;
        zEffect_Anim::NodeActionCallback(
            entry,
            0
        );
    }

    zInput::PollActiveDevices(1);

    pRenderSection = zOpt::GetRenderSection();
    pDisplaySection = zOpt::GetDisplaySection();
    pWindowSection = zOpt::GetWindowSection();
    zOpt_ViewRectSection *const renderSection = pRenderSection;
    zOpt_ViewRectSection *const displaySection = pDisplaySection;
    zOpt_ViewRectSection *const windowSection = pWindowSection;
    zClass_TypeList::UpdateAllBuckets();

    if (g_RecoilApp_QuitAfterCredits != 0) {
        return 1;
    }

    zSnd_Tick(0);

    if (g_Player_HorizonNodeFollowCameraEnabled != 0 && g_Player_HorizonNode != 0) {
        zVec3 cameraPosition = {0};
        gwNode::GetWorldPosition(
            g_MainCamera,
            &cameraPosition
        );
        zClass_Object3D::gwObject3DSetPosition(
            g_Player_HorizonNode,
            cameraPosition.x,
            cameraPosition.y,
            cameraPosition.z
        );
    }

    const int oldClearState = zVideo::GetClearScreenBufferEnabled();
    const int layoutDelay = HudUiMgr::TickLayoutDelay();
    const int savedClearState =
        zVideo::ExchangeClearScreenBufferEnabled(layoutDelay | oldClearState);
    zOpt_ViewRectSection *const clearRect = layoutDelay != 0 ? windowSection : renderSection;

    if (zVid::GetAccelerationOption() != 0) {
        zVideo::CallClearSwSurfaceAndZBuffer(
            (zVidRect32 *)(clearRect),
            (zVidRect32 *)(windowSection)
        );
    } else {
        zVideo::CallClearPrimarySurfaceAndZBuffer((zVidRect32 *)(clearRect));
    }
    zVideo::ExchangeClearScreenBufferEnabled(savedClearState);

    void *pixels;
    int pitchBytes;
    if (zOpt::GetReplicateMode() != 0) {
        pitchBytes = zVideo::GetSwSurfacePitch();
        pixels = zVideo::GetSwSurfacePixels();
    } else {
        pitchBytes = zVideo::GetPrimarySurfacePitch();
        pixels = zVideo::GetPrimarySurfacePixels();
    }

    zRndr::SetFrameBufferRegion(
        pixels,
        renderSection,
        zOpt::GetDisplaySectionBitsPerPixel(),
        pitchBytes
    );
    zClass_List::RenderActiveCameras();
    zVideo::FxPass3_SetInputRectByIndex(
        0,
        (HudUiRect *)(renderSection)
    );

    HudUiMgrSensor::GetFxRect(&g_HudUiMgrSensor_FxRectScratch);
    int fxTop = g_HudUiMgrSensor_FxRectScratch.top;
    int fxBottom = g_HudUiMgrSensor_FxRectScratch.bottom;
    if (zOpt::GetReplicateMode() != 0) {
        fxTop = fxTop / 2;
        fxBottom = fxBottom / 2;
        g_HudUiMgrSensor_FxRectScratch.left = g_HudUiMgrSensor_FxRectScratch.left / 2;
        g_HudUiMgrSensor_FxRectScratch.right = g_HudUiMgrSensor_FxRectScratch.right / 2;
        g_HudUiMgrSensor_FxRectScratch.top = fxTop;
        g_HudUiMgrSensor_FxRectScratch.bottom = fxBottom;
    }

    HudUiRect *fxRectOrNull = 0;
    if (fxBottom > renderSection->bottomExclusive) {
        if (fxTop < renderSection->bottomExclusive) {
            g_HudUiMgrSensor_FxRectScratch.top = renderSection->bottomExclusive;
        }
        fxRectOrNull = &g_HudUiMgrSensor_FxRectScratch;
    }
    zVideo::FxPass3_SetInputRectByIndex(
        1,
        fxRectOrNull
    );

    const int quitTransition = zInput::Keyboard_GetKeyTransitionState(1) & 3;

    if (zVid::GetAccelerationOption() != 0) {
        zRndr::LensFlare_ResetSampleQueue();
        g_HudSensorTracker.UpdateObjectiveFlow();
        HudUiMgrSensor::UpdateMarkersAndProgressFromVariantTag(&g_Variant_CurrentTag);
        zVideo::RunPostprocessOnSwBuffer();
        zVideo::FxPass3_UpdateLocal(g_FrameDeltaTimeSec);

        if (quitTransition != 0) {
            zVideo::Dispatch_UnlockSwSurfaceState();
            return 1;
        }

        zRndr::SetActiveRegionSizeFromRect((HudUiRect *)(windowSection));
        HudUiMgr::UpdateFrame();
        if (zOpt::GetNetworkEnabled() != 0) {
            HudUiNetExitPanel::Tick();
        }
        zVideo::Dispatch_UnlockSwSurfaceState();
    } else if (zOpt::GetReplicateMode() != 0) {
        zVideo::RunPostprocessOnSwBuffer();
        zVideo::FxPass3_UpdateLocal(g_FrameDeltaTimeSec);
        zVideo::Dispatch_UnlockSwSurfaceState();

        if (shouldPresent != 0) {
            g_zVideo_pfnBltSwToPrimaryRectDirect(
                (zVidRect32 *)(renderSection),
                (zVidRect32 *)(displaySection)
            );
        }

        zVideo::RunPostprocessOnPrimaryBuffer();
        if (quitTransition != 0) {
            zVideo::Dispatch_UnlockPrimarySurfaceState();
            zVideo::AdjustSurfacesIfEnabled(
                0,
                0,
                0,
                1
            );
            return 1;
        }

        g_HudSensorTracker.UpdateObjectiveFlow();
        zRndr::SetActiveRegionSizeFromRect((HudUiRect *)(windowSection));
        zRndr::LensFlare_DrawQueuedSamplesScaled16_ClippedFramebuffer(
            0,
            2.0f
        );
        HudUiMgrSensor::UpdateMarkersAndProgressFromVariantTag(&g_Variant_CurrentTag);
        HudUiMgr::UpdateFrame();
        if (zOpt::GetNetworkEnabled() != 0) {
            HudUiNetExitPanel::Tick();
        }
        zVideo::Dispatch_UnlockPrimarySurfaceState();
    } else {
        zVideo::RunPostprocessOnPrimaryBuffer();
        zVideo::FxPass3_UpdateLocal(g_FrameDeltaTimeSec);

        if (quitTransition != 0) {
            zVideo::Dispatch_UnlockPrimarySurfaceState();
            zVideo::AdjustSurfacesIfEnabled(
                0,
                0,
                0,
                1
            );
            return 1;
        }

        g_HudSensorTracker.UpdateObjectiveFlow();
        zRndr::SetActiveRegionSizeFromRect((HudUiRect *)(windowSection));
        zRndr::LensFlare_DrawQueuedSamplesScaled16_ClippedFramebuffer(
            0,
            1.0f
        );
        HudUiMgrSensor::UpdateMarkersAndProgressFromVariantTag(&g_Variant_CurrentTag);
        HudUiMgr::UpdateFrame();
        if (zOpt::GetNetworkEnabled() != 0) {
            HudUiNetExitPanel::Tick();
        }
        zVideo::Dispatch_UnlockPrimarySurfaceState();
    }

    if (shouldPresent != 0) {
        zVideo::AdjustSurfacesIfEnabled(
            (zVidRect32 *)(windowSection),
            (zVidRect32 *)(windowSection),
            0,
            0
        );
    }

    return 0;
}

/**
 * Purpose: Advances gameplay and completes any active fade-driven state transition.
 */
int RecoilApp_PlayState::OnUpdateShouldQuit() {
    if (g_RecoilApp.m_transitionFadeTimer > 0.0f) {
        g_zVideo_SoftwareModeHotkeyEnabled = ZVIDEO_SOFTWARE_MODE_HOTKEY_DISABLED;
        TickAndRenderFrame(0);

        zOpt_ViewRectSection *const windowSection = pWindowSection;
        if (g_RecoilApp.m_transitionFadeTimer >= 1.0f) {
            const int previousClearState =
                zVideo::ExchangeClearScreenBufferEnabled(ZVIDEO_CLEAR_SCREEN_BUFFER_ENABLED);
            ((zUtil_SaveGameState *)g_GameStateOrMapTable)
                ->playerState->transitionDamageSuppressed = 1;
            if (zVid::GetAccelerationOption() != 0) {
                zVideo::CallClearSwSurfaceAndZBuffer(
                    (zVidRect32 *)windowSection,
                    (zVidRect32 *)windowSection
                );
            } else {
                zVideo::CallClearPrimarySurfaceAndZBuffer((zVidRect32 *)windowSection);
            }
            zVideo::ExchangeClearScreenBufferEnabled(previousClearState);
        } else {
            const double overlayAlpha = g_RecoilApp.m_transitionFadeTimer > 0.0f
                                            ? (double)(g_RecoilApp.m_transitionFadeTimer)
                                            : 0.0;
            zRndr_OverlayRect_Submit(
                0,
                0,
                overlayAlpha
            );
        }

        zVideo::AdjustSurfacesIfEnabled(
            (zVidRect32 *)windowSection,
            (zVidRect32 *)windowSection,
            0,
            0
        );
        g_RecoilApp.m_transitionFadeTimer -= g_FrameDeltaTimeSec;

        if (g_RecoilApp.m_transitionFadeTimer <= 0.0f) {
            zOpt::SetMuteSoundOption(0);
            HudUiMgr::TriggerCurrentLayoutOnActivated();
            ((zUtil_SaveGameState *)g_GameStateOrMapTable)
                ->playerState->transitionDamageSuppressed = 0;
        }

        return 0;
    }

    if (g_RecoilApp_QuitAfterCredits != 0) {
        zSndPlayHandleSnapshot *const snapshot = zSndPlayHandleSnapshot::CreateFromActiveSamples();
        snapshot->StopAllIfPlaying();
        zSndCd::Stop();

        zFMV_Script fmvScript;
        fmvScript.Init(
            g_zFMV_ScriptFileName,
            g_zFMV_GrandPrizeScriptName,
            0
        );
        fmvScript.RunBlocking(0);

        if (g_zVideo_ActiveRendererPath != ZVID_RENDERER_BACKEND_SOFTWARE) {
            g_zVideo_pfnBltSwToPrimaryRectDirect(
                0,
                0
            );
        }

        zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_DISABLED);
        HudUi::SetInvalidateMode(0);

        {
            zFMV_ActionBlur blurAction(
                12,
                1
            );

            zFMV_Action *const action = &blurAction;
            action->Begin(0.0);
            while (action->Update(0.0) != 0) {
            }
            action->End();

            RecoilStateMainMenuTransition::QueueEnter(RECOIL_MAINMENU_ROUTE_FRONTEND);
            RecoilStateCredits::QueuePush();
        }
        fmvScript.Cleanup();
        return 0;
    }

    g_zVideo_SoftwareModeHotkeyEnabled = ZVIDEO_SOFTWARE_MODE_HOTKEY_ENABLED;
    if (TickAndRenderFrame(1) != 0) {
        if (zOpt::GetNetworkEnabled() != 0) {
            HudUiNetExitPanel::Show();
            return 0;
        }

        zRndr::SetActiveRegionSizeFromRect((HudUiRect *)pWindowSection);
        if (g_RecoilApp_QuitAfterCredits == 0) {
            RecoilStateMainMenuTransition::QueueEnter(RECOIL_MAINMENU_ROUTE_INGAME);
        }
    }

    return 0;
}

/**
 * Purpose: Restarts mission CD audio when gameplay resumes.
 */
void RecoilApp_PlayState::OnResume(
    int
) {
    if (zSnd::GetCDAudioOption() != 0) {
        const int missionId = g_HudSensorTracker.GetMissionId();
        const int trackCount = zSndCd::GetTrackCount();
        zSndCd::PlayTrackWithMode(
            (missionId % (trackCount - 2)) + 2,
            5
        );
    }
}

/**
 * Purpose: Restores system and engine state while leaving active gameplay.
 */
void RecoilApp_PlayState::OnDeactivate() {
    HudUiLoadingCheckpoint::AdvanceAndLog(g_RecoilApp_LeavingPlayStateMsg);

    if (zVid::GetAccelerationOption() != 0) {
        BOOL screenSaverRunning = FALSE;
        SystemParametersInfoA(
            SPI_SETSCREENSAVERRUNNING,
            0,
            &screenSaverRunning,
            0
        );
    }

    zSndCd::Stop();
    zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_DISABLED);

    if (zOpt::GetNetworkEnabled() != 0) {
        HudUiLoadingCheckpoint::AdvanceAndLog(g_RecoilApp_LeavingNetworkingMsg);
        HudUiNetExitPanel::DestroyGlobal();
    }

    if (zOpt::GetNetworkEnabled() == 0) {
        HudUiLoadingCheckpoint::AdvanceAndLog(g_HudLoading_StopAllSoundsMsg);
        zSndPlayHandleSnapshot *const snapshot = zSndPlayHandleSnapshot::CreateFromActiveSamples();
        snapshot->StopAllIfPlaying();
    }

    zFMV_Script fmvScript;
    fmvScript.Init(
        g_zFMV_ScriptFileName,
        g_RecoilApp_MissionOverFmvTag,
        0
    );
    fmvScript.RunBlocking(1);

    if (g_RecoilApp.m_missionShutdownMode == RECOILAPP_MISSION_SHUTDOWN_ON_EXIT) {
        g_HudSensorTracker.ShutdownMissionGameplaySystems();
    }

    zUtil_ZRDR_UnloadMountedArchives(0);
    fmvScript.Cleanup();
}

/**
 * Purpose: Tears down the local network player, engine, and sound backend.
 */
int RecoilApp_LeaveNetworkState::OnTryBecomeCurrent() {
    zNetwork_DPlay_DestroyCachedLocalPlayer();
    g_RecoilApp.ShutdownEngine();
    zSndBackend::Shutdown();
    return 1;
}

/**
 * Purpose: create the seven gameplay force-feedback effects and start the
 * steady steer and pitch force effects when creation succeeds.
 */
zInput_FFEffectSet *__fastcall zInput_DI_InitForceFeedbackEffectSet(
    zInput_FFEffectSet *effectSet
) {
    effectSet->PrimaryFire = zInput_DI_CreateConstantForceEffectScaled(0.25f);
    effectSet->AltFire = zInput_DI_CreateConstantForceEffectScaled(0.5f);
    effectSet->CollisionImpact = zInput_DI_CreateConstantForceEffectScaled(0.5f);
    effectSet->DamageHit = zInput_DI_CreateConstantForceEffectScaled(0.5f);
    effectSet->AmbientSine = zInput_DI_CreateSineEffectScaled(0.05f);
    effectSet->SteerForce = zInput_DI_CreateConstantForceEffectWithDirection(0x6978);
    effectSet->PitchForce = zInput_DI_CreateConstantForceEffectWithDirection(0x4650);

    if (effectSet->SteerForce != 0) {
        effectSet->SteerForce->Start(
            1,
            0
        );
    }
    if (effectSet->PitchForce != 0) {
        effectSet->PitchForce->Start(
            1,
            0
        );
    }
    return effectSet;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.zinput-di-is-force-feedback-enabled
 * @recoil-artifact defines .text recoil:function:0x42fa80: zInputDI::IsForceFeedbackEnabled.
 * Purpose: Reports whether joystick input and force feedback are both available.
 */
extern "C" int __cdecl zInput_DI_IsForceFeedbackEnabled() {
    if (zInp::GetJoystickOption() != 0 && zInput_DI_HasForceFeedback() != 0) {
        return 1;
    }
    return 0;
}

/**
 * Purpose: Stops and restarts the primary-fire force-feedback effect.
 */
void __fastcall zInput_DI_RestartPrimaryFireEffect(
    zInput_FFEffectSet *effectSet
) {
    zInput_DiEffect *const effect = effectSet->PrimaryFire;
    if (effect == 0) {
        return;
    }
    effect->Stop();
    effectSet->PrimaryFire->Start(
        1,
        0
    );
}

/**
 * Purpose: Applies the requested gain and starts the alternate-fire effect.
 */
void __fastcall zInput_DI_PlayAltFireEffect(
    zInput_FFEffectSet *effectSet,
    float gain
) {
    zInput_DiEffect *const effect = effectSet->AltFire;
    if (effect == 0) {
        return;
    }
    RECOIL_STATIC_ASSERT(sizeof(DIEFFECT) == 0x34);
    RECOIL_STATIC_ASSERT(offsetof(DIEFFECT, dwGain) == 0x10);
    DIEFFECT desc = {0};
    effect->Stop();
    if (gain > 1.0f) {
        gain = 1.0f;
    }
    if (gain < 0.25f) {
        gain = 0.25f;
    }
    desc.dwSize = sizeof(desc);
    desc.dwGain = (DWORD)(gain * 10000.0f);
    effectSet->AltFire->SetParameters(
        &desc,
        DIEP_GAIN
    );
    effectSet->AltFire->Start(
        1,
        0
    );
}

/**
 * Purpose: Plays a collision impulse directed relative to the player.
 */
void zInput_FFEffectSet::PlayCollisionImpactEffect(
    const zVec3 *impactWorldPosXZ,
    float gain
) {
    zInput_DiEffect *const effect = CollisionImpact;
    if (effect == 0) {
        return;
    }
    effect->Stop();
    int direction;
    {
        const float kPi = 3.14159274f;
        const zInput_PlayerStatePartial *const playerState =
            g_GameStateOrMapTable->playerState;
        const float sourceBearing = (float)(atan2(
            -impactWorldPosXZ->z,
            -impactWorldPosXZ->x
        ));
        const float playerBearing = (float)(atan2(
            -playerState->cameraDirNextZ,
            -playerState->cameraDirNextX
        ));
        float relativeBearing = kPi - (sourceBearing - playerBearing);
        {
            const float kTwoPi = 6.28318548f;
            if (relativeBearing < -kTwoPi) {
                relativeBearing += kTwoPi;
            } else if (relativeBearing > kTwoPi) {
                relativeBearing -= kTwoPi;
            }
        }
        {
            const double kRadToDeg = 57.295779513079999;
            direction = (int)(relativeBearing * kRadToDeg) * 100;
        }
    }
    if (gain > 1.0f) {
        gain = 1.0f;
    } else if (gain < 0.2f) {
        gain = 0.2f;
    }
    {
        LONG polarDirection[2] = {direction, 0};
        DIEFFECT desc = {0};
        desc.dwSize = sizeof(desc);
        desc.dwFlags = 0x20;
        desc.dwGain = (DWORD)(gain * 10000.0f);
        desc.cAxes = 2;
        desc.rglDirection = polarDirection;
        effect->SetParameters(
            &desc,
            0x44
        );
        effect->Start(
            1,
            0
        );
    }
}

/**
 * Purpose: Plays a damage impulse directed from the hit source toward the player.
 */
void zInput_FFEffectSet::PlayDamageHitEffect(
    const zVec3 *damageSourceWorldPosXZ,
    float gain
) {
    zInput_DiEffect *const effect = DamageHit;
    if (effect == 0) {
        return;
    }
    effect->Stop();
    int direction;
    {
        const float kPi = 3.14159274f;
        const zInput_PlayerStatePartial *const playerState =
            g_GameStateOrMapTable->playerState;
        const float sourceBearing = (float)(atan2(
            damageSourceWorldPosXZ->z,
            damageSourceWorldPosXZ->x
        ));
        const float playerBearing = (float)(atan2(
            -playerState->cameraDirNextZ,
            -playerState->cameraDirNextX
        ));
        float relativeBearing = kPi - (sourceBearing - playerBearing);
        {
            const float kTwoPi = 6.28318548f;
            if (relativeBearing < -kTwoPi) {
                relativeBearing += kTwoPi;
            } else if (relativeBearing > kTwoPi) {
                relativeBearing -= kTwoPi;
            }
        }
        {
            const double kRadToDeg = 57.295779513079999;
            direction = (int)(relativeBearing * kRadToDeg) * 100;
        }
    }
    if (gain > 1.0f) {
        gain = 1.0f;
    } else if (gain < 0.25f) {
        gain = 0.25f;
    }
    {
        LONG polarDirection[2] = {direction, 0};
        DIEFFECT desc = {0};
        desc.dwSize = sizeof(desc);
        desc.dwFlags = 0x20;
        desc.dwGain = (DWORD)(gain * 10000.0f);
        desc.cAxes = 2;
        desc.rglDirection = polarDirection;
        effect->SetParameters(
            &desc,
            0x44
        );
        effect->Start(
            1,
            0
        );
    }
}

/**
 * Purpose: Updates continuous steering and pitch forces from current player motion.
 */
void __fastcall zInput_DI_UpdateSteerAndPitchForceEffects(
    zInput_FFEffectSet *effectSet
) {
    zInput_PlayerStatePartial *const playerState = g_GameStateOrMapTable->playerState;
    zInput_DiEffect *const steerEffect = effectSet->SteerForce;
    if (steerEffect != 0) {
        float magnitude = playerState->angVelYaw / playerState->yawVelocityLimit;
        int direction = 0x6978;
        if (magnitude < 0.0f) {
            direction = 0x5a;
            magnitude = -magnitude;
        }
        if (magnitude > 0.75f) {
            magnitude = 0.75f;
        } else if (magnitude < 0.0f) {
            magnitude = 0.0f;
        }
        {
            LONG polarDirection[2] = {direction, 0};
            DIEFFECT desc = {0};
            desc.dwSize = sizeof(desc);
            desc.dwFlags = 0x20;
            desc.dwGain = (DWORD)(magnitude * 10000.0f);
            desc.cAxes = 2;
            desc.rglDirection = polarDirection;
            steerEffect->SetParameters(
                &desc,
                0x44
            );
            steerEffect->Start(
                1,
                0
            );
        }
    }
    zInput_DiEffect *const pitchEffect = effectSet->PitchForce;
    if (pitchEffect == 0) {
        return;
    }
    float lowpassFactor;
    {
        int bits = (int)(g_Player_DeltaTime * -3.0f * 12102200.0f);
        bits += 0x3f800000;
        float factor = 0.0f;
        memcpy(
            &factor,
            &bits,
            sizeof(factor)
        );
        lowpassFactor = factor;
    }
    g_zInput_DiPitchAngleLowpassRad = (g_zInput_DiPitchAngleLowpassRad * lowpassFactor) +
                                      ((1.0f - lowpassFactor) * playerState->pitchAngleRad);
    float residual = (playerState->pitchAngleRad - g_zInput_DiPitchAngleLowpassRad) * 8.0f;
    int direction = 0x4650;
    if (residual < 0.0f) {
        direction = 0;
        residual = -residual;
    }
    if (residual > 0.75f) {
        residual = 0.75f;
    } else if (residual < 0.0f) {
        residual = 0.0f;
    }
    {
        LONG polarDirection[2] = {direction, 0};
        DIEFFECT desc = {0};
        desc.dwSize = sizeof(desc);
        desc.dwFlags = 0x20;
        desc.dwGain = (DWORD)(residual * 10000.0f);
        desc.cAxes = 2;
        desc.rglDirection = polarDirection;
        pitchEffect->SetParameters(
            &desc,
            0x44
        );
        pitchEffect->Start(
            1,
            0
        );
    }
}

/**
 * Purpose: Creates a constant-force effect with a clamped gain.
 */
zInput_DiEffect *__stdcall zInput_DI_CreateConstantForceEffectScaled(
    float gain
) {
    DWORD axes[2] = {0, 4};
    LONG direction[2] = {0, 0};
    DICONSTANTFORCE constantForce = {10000};
    DIEFFECT effect = {0};
    effect.dwSize = sizeof(effect);
    effect.dwFlags = 0x22;
    effect.dwDuration = 100000;
    if (gain > 1.0f) {
        gain = 1.0f;
    } else if (gain < 0.0f) {
        gain = 0.0f;
    }
    effect.dwGain = (DWORD)(gain * 10000.0f);
    effect.dwTriggerButton = (DWORD)(-1);
    effect.cAxes = 2;
    effect.rgdwAxes = axes;
    effect.rglDirection = direction;
    effect.cbTypeSpecificParams = sizeof(constantForce);
    effect.lpvTypeSpecificParams = &constantForce;
    return zInput_DI_CreateForceFeedbackEffect(
        &GUID_ConstantForce,
        &effect
    );
}

/**
 * Purpose: Creates a sustained constant-force effect in the requested direction.
 */
zInput_DiEffect *__fastcall zInput_DI_CreateConstantForceEffectWithDirection(
    int directionValue
) {
    DWORD axes[2] = {0, 4};
    LONG direction[2] = {directionValue, 0};
    DICONSTANTFORCE constantForce = {10000};
    DIEFFECT effect = {0};
    effect.dwSize = sizeof(effect);
    effect.dwFlags = 0x22;
    effect.dwDuration = (DWORD)(-1);
    effect.dwTriggerButton = (DWORD)(-1);
    effect.cAxes = 2;
    effect.rgdwAxes = axes;
    effect.rglDirection = direction;
    effect.cbTypeSpecificParams = sizeof(constantForce);
    effect.lpvTypeSpecificParams = &constantForce;
    return zInput_DI_CreateForceFeedbackEffect(
        &GUID_ConstantForce,
        &effect
    );
}

/**
 * Purpose: Creates a periodic sine-force effect with a clamped gain.
 */
zInput_DiEffect *__stdcall zInput_DI_CreateSineEffectScaled(
    float gain
) {
    DWORD axes[2] = {0, 4};
    LONG direction[2] = {0, 0};
    DIPERIODIC periodic = {0};
    if (gain > 1.0f) {
        gain = 1.0f;
    } else if (gain < 0.0f) {
        gain = 0.0f;
    }
    periodic.dwMagnitude = (DWORD)(gain * 10000.0f);
    periodic.dwPeriod = 20000;
    DIEFFECT effect = {0};
    effect.dwSize = sizeof(effect);
    effect.dwFlags = 0x22;
    effect.dwDuration = (DWORD)(-1);
    effect.dwGain = 10000;
    effect.dwTriggerButton = (DWORD)(-1);
    effect.cAxes = 2;
    effect.rgdwAxes = axes;
    effect.rglDirection = direction;
    effect.cbTypeSpecificParams = sizeof(periodic);
    effect.lpvTypeSpecificParams = &periodic;
    return zInput_DI_CreateForceFeedbackEffect(
        &GUID_Sine,
        &effect
    );
}

extern "C" {
/**
 *
 * Purpose: provide the constructor-owned registry path used to detect the
 * Westwood Online API install.
 */
extern const char g_CZRecoilFrame_WolApiRegKey[] = "Software\\Westwood\\WOLAPI\\4352";
/**
 *
 * Purpose: name the recovered frame menu resource loaded during construction.
 */
extern const char g_CZRecoilFrame_MainMenuResourceName[] = "MYMENU";
/**
 *
 * Purpose: name the error log initialized by the frame constructor.
 */
extern const char g_RecoilError_LogFileName[] = "recoil.err";
/**
 *
 * Purpose: preserve the constructor command-line sentinel tested with strncmp.
 */
extern const char g_CZRecoilFrame_NumericDigits[] = "1234567890";
/**
 *
 * Purpose: preserve the constructor command-line campaign-mode switch prefix.
 */
extern const char g_CZRecoilFrame_CmdCampaigns[] = "/campaigns";
/**
 *
 * Purpose: provide the CZGameFrame constructor log/base name passed by the
 * Recoil frame constructor.
 */
extern const char g_CZRecoilFrame_LogBaseName[] = "recoil";
/**
 *
 * Purpose: provide the default Recoil main-window title used by the frame UI.
 */
extern const char g_RecoilApp_WindowTitle[0x7] = "RECOIL";
/**
 *
 * Purpose: provide the 3Dfx renderer main-window title used by the frame UI.
 */
extern const char g_RecoilApp_WindowTitle3Dfx[0xe] = "RECOIL (3Dfx)";
/**
 *
 * Purpose: preserve the common-dialog default extension for campaign files.
 */
extern const char g_CZRecoilFrame_DefaultFileExt[0x3] = "gs";
/**
 *
 * Purpose: format the hardware accelerator command label shown in the frame UI.
 */
extern "C" char g_CZRecoilFrame_AcceleratorMenuLabelFmt[0x16] = "Accelerator - %s (%s)";
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.symbol-0x4f3efc
 * @recoil-artifact defines .data recoil:data:0x4f3efc: Symbol.
 *
 * Purpose: remember whether the Westwood Online registry key was found during
 * frame construction.
 */
int g_CZRecoilFrame_HasWolApi = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.symbol-0x4f3f04
 * @recoil-artifact defines .data recoil:data:0x4f3f04: Symbol.
 *
 * Purpose: gate the one-time Winsock2 prompt before launching the Westwood
 * Online upgrade flow.
 */
int g_CZRecoilFrame_WestwoodOnlineWinsockChecked = 0;
/**
 * g_HudSensorTracker Symbol.
 * Source model: zero-initialized explicit storage for the CZRecoilFrame-owned
 * global instance. HudSensorTracker::ConstructGlobal and ShutdownGlobal own
 * the typed lifetime for this storage.
 * Purpose: Owns the global HUD sensor tracker state used by mission flow,
 * map/objective rendering, network timer sync, and frame-level HUD updates.
 */
#undef g_HudSensorTracker
HudSensorTrackerStorage g_HudSensorTracker = {0};
#define g_HudSensorTracker \
    (*(HudSensorTracker *)&g_HudSensorTracker)
}

namespace {
const UINT kMfcCommandUpdateCode = (UINT)-1;
const UINT kMfcMessageMapSigVoid = 12;
const UINT kMfcMessageMapSigVoidUIntIntInt = 17;
const UINT kMfcMessageMapSigCmdUi = 44;
const int kRendererBackend3dfx = 2;
const int kCmdUiDisabled = 1;
const int kCmdUiChecked = 8;
const int kNetworkOptionDisabled = 0;
const int kNetworkOptionEnabled = 1;
const int kFmvSkipEnabled = 1;
const int kMultiplayerMissionBase = 6;
const int kDefaultMultiplayerEventCode = 1;
const unsigned int kMaxDirectMultiplayerEventCode = 255;
const int kHudTimerAndFlagsSyncPacketType = 20;
const int kDispatchModeSession = 2;
const float kSecondsPerMinute = 60.0f;
const unsigned int kVidMem800x600Threshold = 0x2bf200;
const unsigned int kVidMem1024x768Threshold = 4718592;
const unsigned int kFullscreenMenuCommandId = 0x9c4e;
const DWORD kMainWindowStyle = 0x82ca0000;
const char *kRecoilWndClassName = "RecoilClass";

/**
 * Original helper evidence: no standalone retail function; observed in
 * CZRecoilFrame video-mode command UI callers.
 * Purpose: return the MFC checked-state flag when a video mode is active.
 */
inline int CommandCheckedIfMode(
    int currentMode,
    int targetMode
) {
    return currentMode == targetMode ? kCmdUiChecked : 0;
}

/**
 * Evidence: CZRecoilFrame video-mode update handlers inline this helper;
 * no standalone retail function is emitted.
 * Purpose: translate cached command state into CCmdUI enable/check calls.
 */
inline void UpdateCmdUiFromState(
    CCmdUI *cmdUi,
    int state
) {
    if (state == kCmdUiDisabled) {
        cmdUi->Enable(0);
        cmdUi->SetCheck(0);
        return;
    }

    cmdUi->Enable(1);
    cmdUi->SetCheck(state == kCmdUiChecked ? 1 : 0);
}

/**
 * Evidence: CZRecoilFrame constructor menu-pruning calls inline this helper;
 * no standalone retail function is emitted.
 * Purpose: fetch a submenu handle for command removal while matching MFC use.
 */
inline HMENU SubMenuHandleOrNull(
    HMENU menu,
    int position
) {
    return CMenu::FromHandle(GetSubMenu(
        menu,
        position
    ))->m_hMenu;
}
} // namespace

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.czrecoilframe-dynamic-create
 * @recoil-artifact emits .text recoil:function:0x4301e0: CZRecoilFrame::CreateObject.
 * @recoil-artifact emits .text recoil:function:0x430240: CZRecoilFrame::GetRuntimeClass.
 * @recoil-artifact emits .rdata recoil:data:0x4d0bf0: g_CZRecoilFrame_RuntimeClass.
 * @recoil-artifact emits .data recoil:data:0x4dccf0: g_CZRecoilFrame_RuntimeClassName.
 *
 * Purpose: use the original VC5SP3 MFC dynamic-creation region to emit the
 * Recoil frame factory, virtual runtime-class accessor, runtime-class record,
 * and class-name string in their natural form.
 */
IMPLEMENT_DYNCREATE(CZRecoilFrame, CZGameFrame)

/**
 *
 * Purpose: construct the MFC-derived Recoil frame, including the menu, window,
 * launch options, renderer menu state, and Westwood Online availability.
 */
CZRecoilFrame::CZRecoilFrame() : CZGameFrame(g_CZRecoilFrame_LogBaseName) {
    unsigned long
        titleStorage[(sizeof(CString) + sizeof(unsigned long) - 1) / sizeof(unsigned long)];
    CString *title = (CString *)(titleStorage);
    BuildWindowTitle(title);
    const int windowHeight = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYMENU) +
                             (GetSystemMetrics(SM_CYFRAME) << 1) + 0x1e0;
    const int windowWidth = (GetSystemMetrics(SM_CXFRAME) << 1) + 0x280;
    ((CWnd *)(this))->CreateEx(
        0x20000,
        kRecoilWndClassName,
        (const char *)(*title),
        kMainWindowStyle,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowWidth,
        windowHeight,
        0,
        0,
        0
    );
    title->~CString();

    m_cmdlineFlag = 1;
    m_campaignsOnlyMode = 0;
    char *commandLineCopy = _strdup(GetCommandLineA());
    for (char *token = strtok(
        commandLineCopy,
        " "
    ); token != 0; token = strtok(
        0,
        " "
    )) {
        if (strncmp(
            token,
            g_CZRecoilFrame_CmdCampaigns,
            4
        ) == 0) {
            m_campaignsOnlyMode = 1;
        } else if (strncmp(
            token,
            g_CZRecoilFrame_NumericDigits,
            4
        ) == 0) {
            m_cmdlineFlag = 0;
        }
    }
    free(commandLineCopy);

    zError::InitOutputContext(
        m_hWnd,
        0xe00,
        g_RecoilError_LogFileName
    );
    m_mainMenu.Attach(LoadMenuA(
        AfxFindResourceHandle(
            g_CZRecoilFrame_MainMenuResourceName,
            MAKEINTRESOURCEA(4)
        ),
        g_CZRecoilFrame_MainMenuResourceName
    ));
    ::SetMenu(
        m_hWnd,
        m_mainMenu.m_hMenu
    );

    if (m_campaignsOnlyMode != 0) {
        RemoveMenu(
            SubMenuHandleOrNull(
                m_mainMenu.m_hMenu,
                1
            ),
            0x9c6b,
            MF_BYCOMMAND
        );
        RemoveMenu(
            SubMenuHandleOrNull(
                m_mainMenu.m_hMenu,
                1
            ),
            0x9c7b,
            MF_BYCOMMAND
        );
    } else {
        RemoveMenu(
            m_mainMenu.m_hMenu,
            1,
            MF_BYPOSITION
        );
    }

    RemoveMenu(
        SubMenuHandleOrNull(
            m_mainMenu.m_hMenu,
            2
        ),
        kFullscreenMenuCommandId,
        MF_BYCOMMAND
    );

    g_RecoilApp_hInstance = (HINSTANCE)((unsigned int)(g_RecoilApp.m_hInstance));
    g_RecoilApp_hWndMain = m_hWnd;

    unsigned long formattedTitleStorage
        [(sizeof(CString) + sizeof(unsigned long) - 1) / sizeof(unsigned long)];
    CString *formattedTitle = (CString *)(formattedTitleStorage);
    new (formattedTitle) CString();
    unsigned long
        titleCopyStorage[(sizeof(CString) + sizeof(unsigned long) - 1) / sizeof(unsigned long)];
    CString *titleCopy = (CString *)(titleCopyStorage);
    BuildWindowTitle(titleCopy);
    formattedTitle->Format(
        "%s",
        (const char *)(*titleCopy)
    );
    titleCopy->~CString();
    ((CWnd *)(this))->SetWindowTextA((const char *)(*formattedTitle));
    formattedTitle->~CString();

    m_openZbdFilePath[0] = 0;
    m_useArchiveBanks = 1;
    m_hwApiCmdUiState[0] = 0;
    m_hwApiCmdUiState[1] = 0;
    m_hwApiCmdUiState[2] = 0;
    m_hwApiCmdUiState[3] = 0;
    m_hwApiMenuCommandIds[0] = 0x9c83;
    m_hwApiMenuCommandIds[1] = 0x9c72;
    m_hwApiMenuCommandIds[2] = 0x9c75;
    m_hwApiMenuCommandIds[3] = 0x9c76;

    CheckMenuItem(
        m_mainMenu.m_hMenu,
        0x9c7b,
        zVid::GetTexturePackLoadState() == 0 ? 0 : MF_CHECKED
    );

    g_HudSensorTracker.missionFlags = m_useArchiveBanks;
    zSnd::SetUseArchiveBanksFlag(m_useArchiveBanks);
    m_acceptedD3DDeviceCount = zVid::GetAcceptedHardwareRendererCount_Cached();

    HKEY wolApiRegKey = 0;
    if (RegOpenKeyExA(
            HKEY_LOCAL_MACHINE,
            g_CZRecoilFrame_WolApiRegKey,
            0,
            KEY_READ,
            &wolApiRegKey
        ) == ERROR_SUCCESS) {
        g_CZRecoilFrame_HasWolApi = 1;
        RegCloseKey(wolApiRegKey);
    }

    ((CWnd *)(this))->CenterWindow(0);
    SetCursor(LoadCursorA(
        0,
        IDC_ARROW
    ));
}

/**
 *
 * Purpose: let compiler-emitted MFC member and CZGameFrame base teardown
 * destroy the owned menu through the CMenu provider.
 */
CZRecoilFrame::~CZRecoilFrame() {
}

/**
 *
 * Purpose: attach or remove the recovered main menu and frame menu style.
 */
void CZRecoilFrame::SetMenuBarVisibility(
    int visible
) {
    LONG style = GetWindowLongA(
        m_hWnd,
        GWL_STYLE
    );
    HMENU menu = 0;
    if (visible != 0) {
        style |= (LONG)(0x82ca0000);
        menu = m_mainMenu.m_hMenu;
    } else {
        style &= (LONG)(0xfff7ffff);
    }

    SetWindowLongA(
        m_hWnd,
        GWL_STYLE,
        style
    );
    ::SetMenu(
        m_hWnd,
        menu
    );
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.czrecoilframe-message-map
 * @recoil-artifact emits .text recoil:function:0x4306e0: CZRecoilFrame::GetMessageMap.
 * @recoil-artifact emits .rdata recoil:data:0x4d0c08: g_CZRecoilFrame_MessageMap.
 * @recoil-artifact emits .rdata recoil:data:0x4d0c10: g_CZRecoilFrame_MessageEntries.
 *
 * Purpose: use the original VC5SP3 MFC message-map region to emit the Recoil
 * frame's virtual accessor, map record, and command entries in their natural
 * form.
 */
BEGIN_MESSAGE_MAP(CZRecoilFrame, CZGameFrame)
    ON_COMMAND(0x68, OnMenuStartSinglePlayer)
    ON_COMMAND(0x9c51, OnMenuOpenCampaign)
    ON_COMMAND(0x65, OnOpenFileDialog)
    ON_COMMAND(0x67, OnMenuExitGame)
    ON_COMMAND(0x206, OnMenuSetVideoMode2)
    ON_COMMAND(0x207, OnMenuSetVideoMode3)
    ON_COMMAND(0x208, OnMenuSetVideoMode4)
    ON_COMMAND(0x209, OnMenuSetVideoMode5)
    ON_COMMAND(0x9c4f, OnMenuToggleHud)
    ON_COMMAND(0x9c4e, OnMenuToggleFullscreen)
    ON_COMMAND(0x6a, OnMenuOpenHelpDocs)
    ON_COMMAND(0x6b, OnMenuAbout)
    ON_COMMAND(0x9c53, OnMenuOpenMultiplayerSessionBrowser)
    ON_COMMAND(0x9c55, OnMenuStartMultiplayer)
    ON_COMMAND(0x9c56, OnMenuStartCampaignMode)
    ON_COMMAND(0x9c57, OnMenuStartCampaignMode2)
    ON_COMMAND(0x9c58, OnMenuStartCampaignMode3)
    ON_COMMAND(0x9c59, OnMenuStartCampaignMode4)
    ON_COMMAND(0x9c5a, OnMenuStartCampaignMode5)
    ON_COMMAND(0x9c6b, OnMenuToggleArchiveBanks)
    ON_COMMAND(0x9c7b, OnMenuToggleTexturePacks)
    ON_COMMAND(0x210, OnMenuSetVideoMode7)
    ON_COMMAND(0x9c71, OnMenuSetVideoMode6)
    ON_UPDATE_COMMAND_UI(0x210, OnUpdateVideoMode7CmdUI)
    ON_UPDATE_COMMAND_UI(0x206, OnUpdateVideoMode2CmdUI)
    ON_UPDATE_COMMAND_UI(0x207, OnUpdateVideoMode3CmdUI)
    ON_UPDATE_COMMAND_UI(0x208, OnUpdateVideoMode4CmdUI)
    ON_UPDATE_COMMAND_UI(0x209, OnUpdateVideoMode5CmdUI)
    ON_UPDATE_COMMAND_UI(0x9c71, OnUpdateVideoMode6CmdUI)
    ON_COMMAND(0x9c83, OnMenuSelectHwApi0)
    ON_COMMAND(0x9c72, OnMenuSelectHwApi1)
    ON_COMMAND(0x9c75, OnMenuSelectHwApi2)
    ON_COMMAND(0x9c76, OnMenuSelectHwApi3)
    ON_UPDATE_COMMAND_UI(0x9c83, OnUpdateHwApi0CmdUI)
    ON_UPDATE_COMMAND_UI(0x9c72, OnUpdateHwApi1CmdUI)
    ON_UPDATE_COMMAND_UI(0x9c75, OnUpdateHwApi2CmdUI)
    ON_UPDATE_COMMAND_UI(0x9c76, OnUpdateHwApi3CmdUI)
    ON_UPDATE_COMMAND_UI(0x9c4e, OnUpdateFullscreenCmdUI)
    ON_COMMAND(0x9c7c, OnMenuToggleCDAudio)
    ON_UPDATE_COMMAND_UI(0x9c7c, OnUpdateCDAudioCmdUI)
    ON_COMMAND(0x9c7d, OnMenuToggleJoystick)
    ON_UPDATE_COMMAND_UI(0x9c7d, OnUpdateJoystickCmdUI)
    ON_COMMAND(0x9c7e, OnMenuWestwoodOnlineUpgrade)
    ON_UPDATE_COMMAND_UI(0x9c7f, OnUpdateAlwaysEnabledCmdUI)
    ON_UPDATE_COMMAND_UI(0x9c81, OnUpdateAlwaysEnabledCmdUI)
    ON_UPDATE_COMMAND_UI(0x9c84, OnUpdateAlwaysEnabledCmdUI)
    ON_UPDATE_COMMAND_UI(0x9c7e, OnUpdateNoOpCmdUI)
    ON_UPDATE_COMMAND_UI(0x9c4f, OnUpdateHudCmdUI)
    ON_COMMAND(0x9c80, OnMenuSelectDirectSound)
    ON_UPDATE_COMMAND_UI(0x9c80, OnUpdateDirectSoundCmdUI)
    ON_COMMAND(0x9c82, OnMenuSelectA3D)
    ON_UPDATE_COMMAND_UI(0x9c82, OnUpdateA3DCmdUI)
    ON_UPDATE_COMMAND_UI(0x9c53, OnUpdateNoOpCmdUI)
    ON_WM_SIZE()
END_MESSAGE_MAP()

/**
 *
 * Purpose: build the Recoil window title, including the 3Dfx renderer suffix.
 */
CString * CZRecoilFrame::BuildWindowTitle(
    CString *outTitle
) {
    volatile int constructedTitleState = 0;
    if (g_zVideo_ActiveRendererPath == kRendererBackend3dfx) {
        outTitle->CString::CString(g_RecoilApp_WindowTitle3Dfx);
        return outTitle;
    }

    outTitle->CString::CString(g_RecoilApp_WindowTitle);
    return outTitle;
}

/**
 *
 * Purpose: clear intro/mission FMV skips and start the default engine load.
 */
void CZRecoilFrame::OnMenuStartSinglePlayer() {
    g_RecoilApp.m_skipIntroFmv = 0;
    g_RecoilApp.m_missionFmvState.m_skipMissionFmv = 0;
    g_RecoilApp.LoadZbdAndStartEngine();
}

/**
 *
 * Purpose: enter campaign-open flow with the intro FMV skipped.
 */
void CZRecoilFrame::OnMenuOpenCampaign() {
    g_RecoilApp.m_skipIntroFmv = 1;
    OnOpenFileDialog();
}

/**
 *
 * Purpose: open a campaign ZBD file through the retail common dialog path and
 * launch the selected mission data.
 */
RECOIL_NO_GS void CZRecoilFrame::OnOpenFileDialog() {
    char filter[0x100];
    const int filterLength = LoadStringA(
        g_RecoilApp_hInstance,
        0xc8,
        filter,
        sizeof(filter)
    );
    if (filter[0] != '\0') {
        const char separator = filterLength > 0 ? filter[filterLength - 1] : '\0';
        for (char *cursor = filter; *cursor != '\0'; ++cursor) {
            if (*cursor == separator) {
                *cursor = '\0';
            }
        }
    }

    char fileTitle[0x100] = {0};
    OPENFILENAMEA ofn;
    memset(
        &ofn,
        0,
        sizeof(ofn)
    );
    ofn.lStructSize = 0x4c;
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = m_openZbdFilePath;
    ofn.nMaxFile = sizeof(m_openZbdFilePath);
    ofn.lpstrFileTitle = fileTitle;
    ofn.nMaxFileTitle = 0x200;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.lpstrDefExt = g_CZRecoilFrame_DefaultFileExt;

    if (GetOpenFileNameA((LPOPENFILENAMEA)(&ofn)) != 0) {
        strcpy(
            m_openZbdFilePath,
            ofn.lpstrFile
        );
        g_RecoilApp.LoadZbdAndSetupSensorTracker(
            0,
            m_openZbdFilePath,
            1,
            1
        );
    }

    ::InvalidateRect(
        m_hWnd,
        0,
        TRUE
    );
}

/**
 * Purpose: Posts a close request to the main Recoil frame.
 */
void CZRecoilFrame::OnMenuExitGame() {
    ::PostMessageA(
        m_hWnd,
        WM_CLOSE,
        0,
        0
    );
}

/**
 *
 * Purpose: cache menu command UI states for video modes based on acceleration
 * state and available video memory.
 */
void CZRecoilFrame::ConfigureModeFeatureFlags() {
    const int mode = zVid::GetVideoModeIndexFromOptions();

    if (zVid::GetAccelerationOption() == 0) {
        m_videoModeCmdUiState[0] = CommandCheckedIfMode(
            mode,
            2
        );
        m_videoModeCmdUiState[1] = CommandCheckedIfMode(
            mode,
            3
        );
        m_videoModeCmdUiState[2] = CommandCheckedIfMode(
            mode,
            4
        );
        m_videoModeCmdUiState[3] = CommandCheckedIfMode(
            mode,
            5
        );
        m_videoModeCmdUiState[4] = CommandCheckedIfMode(
            mode,
            6
        );
        m_videoModeCmdUiState[5] = CommandCheckedIfMode(
            mode,
            7
        );
        return;
    }

    m_videoModeCmdUiState[0] = kCmdUiDisabled;
    m_videoModeCmdUiState[1] = kCmdUiDisabled;
    m_videoModeCmdUiState[2] = CommandCheckedIfMode(
        mode,
        4
    );
    m_videoModeCmdUiState[3] = CommandCheckedIfMode(
        mode,
        5
    );

    if (m_vidMemFreeBytes > kVidMem800x600Threshold) {
        m_videoModeCmdUiState[4] = CommandCheckedIfMode(
            mode,
            6
        );
    } else {
        m_videoModeCmdUiState[4] = kCmdUiDisabled;
    }

    if (m_vidMemFreeBytes > kVidMem1024x768Threshold) {
        m_videoModeCmdUiState[5] = CommandCheckedIfMode(
            mode,
            7
        );
    } else {
        m_videoModeCmdUiState[5] = kCmdUiDisabled;
    }
}

/**
 *
 * Purpose: set video mode 2 and refresh the recovered mode command state.
 */
void CZRecoilFrame::OnMenuSetVideoMode2() {
    zVid::SetVideoModeIndex(2);
    ConfigureModeFeatureFlags();
}

/**
 *
 * Purpose: set video mode 3 and refresh the recovered mode command state.
 */
void CZRecoilFrame::OnMenuSetVideoMode3() {
    zVid::SetVideoModeIndex(3);
    ConfigureModeFeatureFlags();
}

/**
 *
 * Purpose: set video mode 4 and refresh the recovered mode command state.
 */
void CZRecoilFrame::OnMenuSetVideoMode4() {
    zVid::SetVideoModeIndex(4);
    ConfigureModeFeatureFlags();
}

/**
 *
 * Purpose: set video mode 5 and refresh the recovered mode command state.
 */
void CZRecoilFrame::OnMenuSetVideoMode5() {
    zVid::SetVideoModeIndex(5);
    ConfigureModeFeatureFlags();
}

/**
 *
 * Purpose: set video mode 6 and refresh the recovered mode command state.
 */
void CZRecoilFrame::OnMenuSetVideoMode6() {
    zVid::SetVideoModeIndex(6);
    ConfigureModeFeatureFlags();
}

/**
 *
 * Purpose: set video mode 7 and refresh the recovered mode command state.
 */
void CZRecoilFrame::OnMenuSetVideoMode7() {
    zVid::SetVideoModeIndex(7);
    ConfigureModeFeatureFlags();
}

/**
 *
 * Purpose: toggle the HUD visibility option from the frame menu.
 */
void CZRecoilFrame::OnMenuToggleHud() {
    zOpt::SetHudVisibilityOption(zOpt::GetHudVisibilityOption() == 0 ? 1 : 0);
}

/**
 *
 * Purpose: enable and check the HUD command from the current option state.
 */
void CZRecoilFrame::OnUpdateHudCmdUI(
    CCmdUI *cmdUi
) {
    cmdUi->Enable(1);
    cmdUi->SetCheck(zOpt::GetHudVisibilityOption());
}

/**
 *
 * Purpose: toggle the fullscreen option from the frame menu.
 */
void CZRecoilFrame::OnMenuToggleFullscreen() {
    zOpt::SetFullscreenOption(zOpt::GetFullscreenOption() == 0 ? 1 : 0);
}

/**
 *
 * Purpose: open the retail help index or report the associated shell error.
 */
RECOIL_NO_GS void CZRecoilFrame::OnMenuOpenHelpDocs() {
    static const unsigned char kFindExecutableErrorMap[0x20] = {0,
        4,
        1,
        1,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        2,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        4,
        3};

    char associatedExecutablePath[0x100];
    HINSTANCE findResult = FindExecutableA(
        "Docs\\Index.html",
        0,
        associatedExecutablePath
    );

    char messageBoxTitle[0x80];
    strcpy(
        messageBoxTitle,
        zLoc::GetMessageString(0x19)
    );

    const UINT resultCode = (UINT)((UINT_PTR)(findResult));
    if (resultCode <= 0x1f) {
        switch (kFindExecutableErrorMap[resultCode]) {
        case 0:
            ((CWnd *)(this))->MessageBoxA(
                zLoc::GetMessageString(0x20),
                messageBoxTitle,
                0x30
            );
            return;

        case 1:
            ((CWnd *)(this))->MessageBoxA(
                zLoc::GetMessageString(0x22),
                messageBoxTitle,
                0x30
            );
            return;

        case 2:
            ((CWnd *)(this))->MessageBoxA(
                zLoc::GetMessageString(0x24),
                messageBoxTitle,
                0x30
            );
            return;

        case 3:
            ((CWnd *)(this))->MessageBoxA(
                zLoc::GetMessageString(0x21),
                messageBoxTitle,
                0x30
            );
            return;

        default:
            break;
        }
    }

    ShellExecuteA(
        g_RecoilApp_hWndMain,
        "open",
        "Docs\\Index.html",
        0,
        0,
        SW_HIDE
    );
}

/**
 *
 * Purpose: display the recovered About dialog through the frame menu.
 */
RECOIL_NO_GS void CZRecoilFrame::OnMenuAbout() {
    CAboutDlg aboutDlg;
    aboutDlg.CDialog::DoModal();
}

/**
 * Purpose: Presents the fatal startup error and terminates the application.
 */
RECOIL_NO_GS void __fastcall RecoilApp::FatalErrorAndExit(
    int errorCode
) {
    if (errorCode != -1) {
        return;
    }

    char caption[0x80];
    char text[0x80];
    strcpy(
        caption,
        zLoc::GetMessageString(0x12)
    );
    strcpy(
        text,
        zLoc::GetMessageString(0x30)
    );

    Briefing::StopAndShutdownThread(0);
    zVideo_dd::FlipToGDIIfAttached();
    zSndSystem::Shutdown();
    zNetwork::ShutdownSessionRuntime();
    zVideo::ShutdownVideoSystem();
    printf(
        "%s: %s\n",
        caption,
        text
    );
    Sleep(1000);
    MessageBeep(MB_ICONHAND);
    MessageBoxA(
        g_RecoilApp_hWndMain,
        text,
        caption,
        MB_ICONHAND
    );
    zSys::ExitProcessWithCleanup(0);
}

/**
 * Purpose: run the DirectPlay session browser/host setup flow and launch the
 * selected multiplayer mission state.
 */
void CZRecoilFrame::OnMenuOpenMultiplayerSessionBrowser() {
    int shouldShutdownNetwork = 1;

    if (CoInitialize(0) >= 0) {
        unsigned long browserStorage
            [(sizeof(NetSessionBrowserDialog) + sizeof(unsigned long) - 1) / sizeof(unsigned long)];
        unsigned long configStorage
            [(sizeof(NetSessionConfigDialog) + sizeof(unsigned long) - 1) / sizeof(unsigned long)];
        NetSessionBrowserDialog *const browserDialog = (NetSessionBrowserDialog *)(browserStorage);
        NetSessionConfigDialog *const configDialog = (NetSessionConfigDialog *)(configStorage);

        browserDialog->Constructor(0);
        configDialog->Constructor(0);

        zNetwork::InitSessionRuntime(&g_zNetwork_RecoilAppGuid);
        zNetwork::SetFatalDisconnectCallback(&RecoilApp::FatalErrorAndExit);
        g_RecoilApp.m_skipIntroFmv = kFmvSkipEnabled;
        g_RecoilApp.m_missionFmvState.m_skipMissionFmv = kFmvSkipEnabled;

        if (((CDialog *)browserDialog)->CDialog::DoModal() == IDOK) {
            zOpt::SetPlayerName((const char *)(browserDialog->m_playerName));

            if (browserDialog->m_shouldEnterHostSetup != 0) {
                zOpt::SetNetworkEnabled(kNetworkOptionEnabled);
                g_RecoilApp.LoadZbdAndStartEngine();
                HudUiNetGameSetupOverlayOwner::QueueEnterWithReconfigureFlag(0);
                shouldShutdownNetwork = 0;
            } else {
                zNetworkSessionDescStatusFields statusFields;
                statusFields.selectedSessionIndex = browserDialog->m_selectedSessionIndex;

                if (zNetworkDPlay::OpenSelectedSessionAndReadStatusFields(&statusFields) != 0) {
                    zOpt::SetNetworkEnabled(kNetworkOptionEnabled);
                    zNetwork_DPlay::CreateLocalPlayerRecordAndRegister(
                        (char *)((const char *)(browserDialog->m_playerName))
                    );
                    zOpt::SetPlayerName((const char *)(browserDialog->m_playerName));

                    if ((unsigned int)(statusFields.eventCode) > kMaxDirectMultiplayerEventCode) {
                        g_RecoilApp.m_pendingState = &g_RecoilApp.m_mpExitDialogState;
                        statusFields.eventCode = kDefaultMultiplayerEventCode;
                        zNetwork::RegisterPacketHandler(
                            kHudTimerAndFlagsSyncPacketType,
                            (zNetworkPacketHandler)&GameNet::HandlePkt14_HudTimerAndFlagsSync,
                            kDispatchModeSession
                        );
                    }

                    GameNet::SetStatusBitsFromFlags(statusFields.statusFlags);

                    union TimerSecondsBits {
                        float seconds;
                        int raw;
                    } timerSeconds = {(float)(statusFields.valueOrTime) * kSecondsPerMinute};
                    g_HudSensorTracker.SetRuntimeTimerSecAndGoalValue(
                        timerSeconds.raw,
                        statusFields.auxParam
                    );

                    g_RecoilApp.LoadZbdAndSetupSensorTracker(
                        statusFields.eventCode + kMultiplayerMissionBase,
                        0,
                        kFmvSkipEnabled,
                        m_useArchiveBanks
                    );
                    shouldShutdownNetwork = 0;
                }
            }
        }

        configDialog->Destructor();
        browserDialog->Destructor();
    }

    if (shouldShutdownNetwork != 0) {
        zNetwork::ShutdownSessionRuntime();
        zOpt::SetNetworkEnabled(kNetworkOptionDisabled);
    }
}

/**
 *
 * Purpose: start the default multiplayer mission setup path.
 */
void CZRecoilFrame::OnMenuStartMultiplayer() {
    g_RecoilApp.LoadZbdAndSetupSensorTracker(
        1,
        0,
        1,
        m_useArchiveBanks
    );
}

/**
 *
 * Purpose: start campaign mission slot 2 with the current archive-bank flag.
 */
void CZRecoilFrame::OnMenuStartCampaignMode() {
    g_RecoilApp.LoadZbdAndSetupSensorTracker(
        2,
        0,
        1,
        m_useArchiveBanks
    );
}

/**
 *
 * Purpose: start campaign mission slot 3 with the current archive-bank flag.
 */
void CZRecoilFrame::OnMenuStartCampaignMode2() {
    g_RecoilApp.LoadZbdAndSetupSensorTracker(
        3,
        0,
        1,
        m_useArchiveBanks
    );
}

/**
 *
 * Purpose: start campaign mission slot 4 with the current archive-bank flag.
 */
void CZRecoilFrame::OnMenuStartCampaignMode3() {
    g_RecoilApp.LoadZbdAndSetupSensorTracker(
        4,
        0,
        1,
        m_useArchiveBanks
    );
}

/**
 *
 * Purpose: start campaign mission slot 5 with the current archive-bank flag.
 */
void CZRecoilFrame::OnMenuStartCampaignMode4() {
    g_RecoilApp.LoadZbdAndSetupSensorTracker(
        5,
        0,
        1,
        m_useArchiveBanks
    );
}

/**
 *
 * Purpose: start campaign mission slot 6 with the current archive-bank flag.
 */
void CZRecoilFrame::OnMenuStartCampaignMode5() {
    g_RecoilApp.LoadZbdAndSetupSensorTracker(
        6,
        0,
        1,
        m_useArchiveBanks
    );
}


/**
 *
 * Purpose: toggle archive-bank loading and mirror it into audio/HUD state.
 */
void CZRecoilFrame::OnMenuToggleArchiveBanks() {
    m_useArchiveBanks = m_useArchiveBanks == 0 ? 1 : 0;
    CheckMenuItem(
        m_mainMenu.m_hMenu,
        0x9c6b,
        m_useArchiveBanks == 0 ? MF_UNCHECKED : MF_CHECKED
    );
    g_HudSensorTracker.missionFlags = m_useArchiveBanks;
    zSnd::SetUseArchiveBanksFlag(m_useArchiveBanks);
}

/**
 *
 * Purpose: toggle texture-pack loading and update the menu check state.
 */
void CZRecoilFrame::OnMenuToggleTexturePacks() {
    if (zVid::GetTexturePackLoadState() != 0) {
        zVid::SetTexturePackLoadState(0);
        CheckMenuItem(
            m_mainMenu.m_hMenu,
            0x9c7b,
            MF_UNCHECKED
        );
        return;
    }

    zVid::SetTexturePackLoadState(1);
    CheckMenuItem(
        m_mainMenu.m_hMenu,
        0x9c7b,
        MF_CHECKED
    );
}

/**
 *
 * Purpose: apply cached command UI state for video mode 2.
 */
void CZRecoilFrame::OnUpdateVideoMode2CmdUI(
    CCmdUI *cmdUi
) {
    UpdateCmdUiFromState(
        cmdUi,
        m_videoModeCmdUiState[0]
    );
}

/**
 *
 * Purpose: apply cached command UI state for video mode 3.
 */
void CZRecoilFrame::OnUpdateVideoMode3CmdUI(
    CCmdUI *cmdUi
) {
    UpdateCmdUiFromState(
        cmdUi,
        m_videoModeCmdUiState[1]
    );
}

/**
 *
 * Purpose: apply cached command UI state for video mode 4.
 */
void CZRecoilFrame::OnUpdateVideoMode4CmdUI(
    CCmdUI *cmdUi
) {
    UpdateCmdUiFromState(
        cmdUi,
        m_videoModeCmdUiState[2]
    );
}

/**
 *
 * Purpose: apply cached command UI state for video mode 5.
 */
void CZRecoilFrame::OnUpdateVideoMode5CmdUI(
    CCmdUI *cmdUi
) {
    UpdateCmdUiFromState(
        cmdUi,
        m_videoModeCmdUiState[3]
    );
}

/**
 *
 * Purpose: apply cached command UI state for video mode 6.
 */
void CZRecoilFrame::OnUpdateVideoMode6CmdUI(
    CCmdUI *cmdUi
) {
    UpdateCmdUiFromState(
        cmdUi,
        m_videoModeCmdUiState[4]
    );
}

/**
 *
 * Purpose: apply cached command UI state for video mode 7.
 */
void CZRecoilFrame::OnUpdateVideoMode7CmdUI(
    CCmdUI *cmdUi
) {
    UpdateCmdUiFromState(
        cmdUi,
        m_videoModeCmdUiState[5]
    );
}



/**
 *
 * Purpose: select a hardware API, query video memory, force accelerated mode, and enter the default hardware video mode.
 */
void CZRecoilFrame::SetHwApiAndInitMode(
    int hwApiIndex
) {
    zVid::SetHwApiOption(zVideo::SelectHwApiDeviceOrFallback(hwApiIndex));
    g_zVideo_pfnQueryDeviceVideoMemoryBytes(
        hwApiIndex,
        &m_vidMemTotalBytes,
        (int *)(&m_vidMemFreeBytes)
    );
    m_fullscreenOption = zOpt::GetFullscreenOption();
    zOpt::SetFullscreenOption(1);
    zVid::SetAccelerationOption(1);
    m_videoModeIndex = zVid::GetVideoModeIndexFromOptions();
    OnMenuSetVideoMode5();
}

/**
 *
 * Purpose: restore software/fallback renderer options and rebuild mode command state.
 */
void CZRecoilFrame::InitFallbackMode() {
    zVid::SetHwApiOption(zVideo::SelectHwApiDeviceOrFallback(-1));
    zVid::SetAccelerationOption(0);
    zOpt::SetFullscreenOption(m_fullscreenOption);
    zVid::SetVideoModeIndex(m_videoModeIndex);
    ConfigureModeFeatureFlags();
}

/**
 *
 * Purpose: initialize the selected hardware API once and clear competing menu checks.
 */
void CZRecoilFrame::EnsureHwApiInitialized(
    int hwApiSelector
) {
    if (m_hwApiCmdUiState[hwApiSelector] != 0) {
        return;
    }

    if (hwApiSelector != 0) {
        (void)zVid::GetHwApiDescription(hwApiSelector - 1);
    }

    m_hwApiCmdUiState[hwApiSelector] = 8;
    if (hwApiSelector == 0) {
        InitFallbackMode();
    } else {
        SetHwApiAndInitMode(hwApiSelector - 1);
    }

    for (int i = 0; i < 4; ++i) {
        if (i != hwApiSelector) {
            m_hwApiCmdUiState[i] = 0;
        }
    }
}

/**
 *
 * Purpose: select the startup renderer path from saved options or fallback defaults.
 */
void CZRecoilFrame::InitStartupHwApiFromOptions() {
    if (zVid::GetHwApiOption() != 0) {
        const int acceptedDirectDrawDeviceCount = zVid::GetAcceptedDirectDrawDeviceCount();
        if (acceptedDirectDrawDeviceCount != 0) {
            m_hwApiCmdUiState[0] = 0;
            m_hwApiCmdUiState[acceptedDirectDrawDeviceCount] = 8;
            SetHwApiAndInitMode(acceptedDirectDrawDeviceCount - 1);
            return;
        }
    }

    m_hwApiCmdUiState[0] = 8;
    m_videoModeIndex = 5;
    m_fullscreenOption = zOpt::GetFullscreenOption();
    InitFallbackMode();
}

/**
 *
 * Purpose: select the software/fallback hardware API menu path.
 */
void CZRecoilFrame::OnMenuSelectHwApi0() {
    EnsureHwApiInitialized(0);
}

/**
 *
 * Purpose: select hardware API menu entry 1.
 */
void CZRecoilFrame::OnMenuSelectHwApi1() {
    EnsureHwApiInitialized(1);
}

/**
 *
 * Purpose: select hardware API menu entry 2.
 */
void CZRecoilFrame::OnMenuSelectHwApi2() {
    EnsureHwApiInitialized(2);
}

/**
 *
 * Purpose: select hardware API menu entry 3.
 */
void CZRecoilFrame::OnMenuSelectHwApi3() {
    EnsureHwApiInitialized(3);
}

/**
 *
 * Purpose: remove unavailable hardware API commands or update their label/check state.
 */
RECOIL_NO_GS void CZRecoilFrame::UpdateHwApiMenuItem(
    CCmdUI *cmdUi,
    int apiIndex
) {
    if (m_acceptedD3DDeviceCount < apiIndex) {
        RemoveMenu(
            cmdUi->m_pMenu->m_hMenu,
            m_hwApiMenuCommandIds[apiIndex],
            MF_BYCOMMAND
        );
        return;
    }

    cmdUi->SetCheck(m_hwApiCmdUiState[apiIndex] == kCmdUiChecked ? 1 : 0);

    char menuLabelText[0x40];
    sprintf(
        menuLabelText,
        g_CZRecoilFrame_AcceleratorMenuLabelFmt,
        zVid::GetHwApiDescription(apiIndex - 1),
        zVid::GetHwApiDriverName(apiIndex - 1)
    );
    cmdUi->SetText(menuLabelText);
}

/**
 *
 * Purpose: enable and check the software/fallback hardware API command.
 */
void CZRecoilFrame::OnUpdateHwApi0CmdUI(
    CCmdUI *cmdUi
) {
    cmdUi->Enable(1);
    cmdUi->SetCheck(m_hwApiCmdUiState[0] == kCmdUiChecked ? 1 : 0);
}

/**
 *
 * Purpose: update hardware API command UI entry 1.
 */
void CZRecoilFrame::OnUpdateHwApi1CmdUI(
    CCmdUI *cmdUi
) {
    UpdateHwApiMenuItem(
        cmdUi,
        1
    );
}

/**
 *
 * Purpose: update hardware API command UI entry 2.
 */
void CZRecoilFrame::OnUpdateHwApi2CmdUI(
    CCmdUI *cmdUi
) {
    UpdateHwApiMenuItem(
        cmdUi,
        2
    );
}

/**
 *
 * Purpose: update hardware API command UI entry 3.
 */
void CZRecoilFrame::OnUpdateHwApi3CmdUI(
    CCmdUI *cmdUi
) {
    UpdateHwApiMenuItem(
        cmdUi,
        3
    );
}

/**
 *
 * Purpose: remove the fullscreen command from the update menu path.
 */
void CZRecoilFrame::OnUpdateFullscreenCmdUI(
    CCmdUI *cmdUi
) {
    RemoveMenu(
        cmdUi->m_pMenu->m_hMenu,
        kFullscreenMenuCommandId,
        MF_BYCOMMAND
    );
}

/**
 * Original helper evidence: CZRecoilFrame message-map entries for command ids
 * 0x9c7f, 0x9c81, and 0x9c84 share the same one-argument enable handler.
 * Purpose: Enable command UI entries that have no authored state gate.
 */
void CZRecoilFrame::OnUpdateAlwaysEnabledCmdUI(
    CCmdUI *cmdUi
) {
    MfcCmdUI::EnableAlways(cmdUi);
}

/**
 *
 * Purpose: toggle the CD audio option from the frame menu.
 */
void CZRecoilFrame::OnMenuToggleCDAudio() {
    zSnd::SetCDAudioOption(zSnd::GetCDAudioOption() == 0 ? 1 : 0);
}

/**
 *
 * Purpose: enable and check the CD audio command from sound options.
 */
void CZRecoilFrame::OnUpdateCDAudioCmdUI(
    CCmdUI *cmdUi
) {
    cmdUi->Enable(1);
    cmdUi->SetCheck(zSnd::GetCDAudioOption() != 0 ? 1 : 0);
}

/**
 *
 * Purpose: toggle joystick input from the frame menu.
 */
void CZRecoilFrame::OnMenuToggleJoystick() {
    zInp::SetJoystickOption(zInp::GetJoystickOption() == 0 ? 1 : 0);
}

/**
 *
 * Purpose: enable and check the joystick command from input options.
 */
void CZRecoilFrame::OnUpdateJoystickCmdUI(
    CCmdUI *cmdUi
) {
    cmdUi->Enable(1);
    cmdUi->SetCheck(zInp::GetJoystickOption() != 0 ? 1 : 0);
}


/**
 * Purpose: gate the Westwood Online upgrade flow on Winsock2 readiness and
 * launch the selected mission.
 */
RECOIL_NO_GS void CZRecoilFrame::OnMenuWestwoodOnlineUpgrade() {
    int canShowUpgrade = 1;
    if (g_CZRecoilFrame_WestwoodOnlineWinsockChecked == 0) {
        char caption[0x100];
        char messageFormat[0x200];

        g_CZRecoilFrame_WestwoodOnlineWinsockChecked = 1;
        strcpy(
            caption,
            zLoc::GetMessageString(18)
        );
        strcpy(
            messageFormat,
            zLoc::GetMessageString(38)
        );
        if (NetUi::VerifyWinsock2OrPromptContinue(
            caption,
            messageFormat
        ) == 0) {
            canShowUpgrade = 0;
        }
    }

    if (canShowUpgrade == 0) {
        return;
    }

    g_RecoilApp.m_skipIntroFmv = 1;
    g_RecoilApp.m_missionFmvState.m_skipMissionFmv = 1;

    int selectedMissionIndex;
    if (WestwoodOnlineUpgradeDialog::ShowModalAndGetSelectedMissionIndex(&selectedMissionIndex) !=
        0) {
        g_RecoilApp.LoadZbdAndSetupSensorTracker(
            selectedMissionIndex + 6,
            0,
            1,
            g_HudSensorTracker.missionFlags
        );
    }
}

namespace MfcCmdUI {
/**
 * Purpose: Marks the associated MFC command as enabled.
 */
void __stdcall EnableAlways(
    CCmdUI *cmdUi
) {
    cmdUi->Enable(1);
}
} // namespace MfcCmdUI

/**
 *
 * Purpose: select DirectSound as the active audio API option.
 */
void CZRecoilFrame::OnMenuSelectDirectSound() {
    zSnd::SetAudioApiOption(0);
}

/**
 *
 * Purpose: enable and check the DirectSound command from audio options.
 */
void CZRecoilFrame::OnUpdateDirectSoundCmdUI(
    CCmdUI *cmdUi
) {
    cmdUi->Enable(1);
    cmdUi->SetCheck(zSnd::GetAudioApiOption() == 0 ? 1 : 0);
}

/**
 *
 * Purpose: select A3D as the active audio API option.
 */
void CZRecoilFrame::OnMenuSelectA3D() {
    zSnd::SetAudioApiOption(1);
}

/**
 *
 * Purpose: enable and check the A3D command from the active sound backend.
 */
void CZRecoilFrame::OnUpdateA3DCmdUI(
    CCmdUI *cmdUi
) {
    cmdUi->Enable(1);
    cmdUi->SetCheck(zSnd::GetActiveBackend() == 1 ? 1 : 0);
}

/**
 *
 * Purpose: forward sizing to CZGameFrame and deactivate the app on minimized/iconic states.
 */
void CZRecoilFrame::OnSize(
    unsigned int nType,
    int cx,
    int cy
) {
    CZGameFrame::OnSize(
        nType,
        cx,
        cy
    );

    if (nType == 4 || nType == 1) {
        m_app->OnAppDeactivate();
    }
}
#include "Battlesport/game_net.h"

#include "Battlesport/briefing.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/net_ui.h"
#include "Battlesport/recoil_app.h"
#include "Battlesport/pickup.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "Battlesport/mission.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "Battlesport/turret.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zVideo/zvid.h"

#if defined(_MSC_VER) && _MSC_VER < 1300 && !defined(_DEBUG)
/**
 * Original-source inline provider-boundary restore from MFC42 AFXCMN.INL:
 * _AFXCMN_INLINE CSpinButtonCtrl::CSpinButtonCtrl() { }.
 * No standalone Recoil-authored retail function exists; this source restores
 * the VC5/MFC42 common-control inline suppressed by Mfc42Abi.h so the config
 * dialog emits the retail local spin-control vftable reference.
 * Purpose: Construct embedded MFC42 spin-button controls with provider inline
 * behavior for NetSessionConfigDialog.
 */
inline CSpinButtonCtrl::CSpinButtonCtrl() {
}
#endif

#include <shellapi.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_MSC_VER) || _MSC_VER >= 1300
#include <new>
#endif

static const float kGameNetPkt06SendIntervalSec = 0.100000001f;
static const float kGameNetHudTimerWarningDurationSec = 5.0f;
static const float kGameNetHudTimerTenSecondThreshold = 10.0f;
static const float kGameNetHudTimerOneMinuteLeadSec = 60.0f;
static const unsigned int kGameNetPkt06InputBit16Flag = 0x10000u;
static const unsigned int kGameNetPkt06InputBit17Flag = 0x20000u;
static const unsigned int kGameNetPkt06ProgressTargetsFlag = 0x40000u;
static const unsigned int kGameNetRemoteAltGunDispatchFlag = 0x2000000u;
static const unsigned int kGameNetRemoteCloneNodeFlag = 0x400000u;
static const float kGameNetRemoteUnlimitedAmmo = 123456792.0f;

extern "C" NetPkt10_QSandEvent g_NetPkt10_QSandEventRelayBuf;

struct GameNetReaderArray {
    int countTag;
    int count;
    zReader::Node nodes[1];
};

namespace GameNetSpawnPointList {
/**
 * Purpose: Reset the GameNet-owned spawn-point list header to an empty state.
 */
void __cdecl InitGlobals() {
    g_GameNetSpawnPointList.flags = 0;
    g_GameNetSpawnPointTail = 0;
    g_GameNetSpawnPointHead = 0;
    g_GameNetSpawnPointCount = 0;
}
} // namespace GameNetSpawnPointList

namespace GameNetPlayerRowList {
/**
 * Purpose: Reset the GameNet-owned player-row list header to an empty state.
 */
void __cdecl Reset() {
    g_GameNetPlayerRowList.flags = 0;
    g_GameNetPlayerRowTail = 0;
    g_GameNetPlayerRowHead = 0;
    g_GameNetPlayerRowCount = 0;
}
} // namespace GameNetPlayerRowList

namespace GameNet {
/**
 * Purpose: Register gameplay packet handlers and option catalog callbacks once.
 */
void __cdecl RegisterGameplayHandlersAndOptCatalogCallbacks() {
    if (g_GameNet_HandlersRegistered == 0) {
        zNetwork::RegisterPacketHandler(
            6,
            (zNetworkPacketHandler)&HandlePkt06_PlayerStateSnapshot,
            2
        );
        zNetwork::RegisterPacketHandler(
            7,
            (zNetworkPacketHandler)&HandlePkt07_AltGunDispatch,
            2
        );
        zNetwork::RegisterPacketHandler(
            0x0a,
            (zNetworkPacketHandler)&OptCatalog::HandlePkt0A_RemoveRuntimeRelay,
            2
        );
        zNetwork::RegisterPacketHandler(
            1,
            (zNetworkPacketHandler)&ReassignPlayerColorsAndRefreshRows,
            2
        );
        zNetwork::RegisterPacketHandler(
            8,
            (zNetworkPacketHandler)&HandlePkt08_PlayerKillEvent,
            2
        );
        zNetwork::RegisterPacketHandler(
            9,
            (zNetworkPacketHandler)&HandlePkt09_PlayerScoreboardSnapshot,
            2
        );
        zNetwork::RegisterPacketHandler(
            0x0b,
            (zNetworkPacketHandler)&HandlePkt0B_ChatMessage,
            2
        );
        zNetwork::RegisterPacketHandler(
            0x0e,
            (zNetworkPacketHandler)&HandlePkt0E_PlayerLapProgress,
            2
        );
        zNetwork::RegisterPacketHandler(
            0x0c,
            (zNetworkPacketHandler)&HandlePkt0C_HudTimerStatusBits,
            2
        );
        zNetwork::RegisterPacketHandler(
            0x0d,
            (zNetworkPacketHandler)&HandlePkt0D_HudTimerPanelState,
            2
        );
        zNetwork::RegisterPacketHandler(
            0x0f,
            (zNetworkPacketHandler)&zDEClient_Crater::NetRelayCallback,
            2
        );
        zNetwork::RegisterPacketHandler(
            0x10,
            (zNetworkPacketHandler)&zDEClient_QSand::NetRelayCallback,
            2
        );
        zNetwork::RegisterPacketHandler(
            0x11,
            (zNetworkPacketHandler)&Pickup::HandlePkt11_SpawnDelta,
            2
        );
        zNetwork::RegisterPacketHandler(
            0x12,
            (zNetworkPacketHandler)&Pickup::HandlePkt12_AirdropSpawnChuteRelay,
            2
        );
        zNetwork::RegisterPacketHandler(
            0x13,
            (zNetworkPacketHandler)&HandlePkt13_EffectAnimActivationRecord,
            2
        );
        zNetwork::RegisterPacketHandler(
            0x14,
            (zNetworkPacketHandler)&HandlePkt14_HudTimerAndFlagsSync,
            2
        );
        zNetwork::RegisterPacketHandler(
            3,
            (zNetworkPacketHandler)&HandlePkt03_RemoveRemotePlayer,
            2
        );
        g_GameNet_HandlersRegistered = 1;
    }

    g_zDEClientCraterNetRelayCallback = (zDEClient_NetRelayCallback)&zDEClient_Crater::Execute;
    g_zDEClientQSandNetRelayCallback = (zDEClient_NetRelayCallback)&SendPkt10_QSandEvent;
    g_OptCatalog_AllocRuntimeGateCallback = &OptCatalog::AltGunDispatchAllocRuntimeGateCallback;
    g_OptCatalog_AltGunDispatchNoOpCallback = &AltGunDispatchNoOpCallback;
    g_OptCatalog_RemoveRuntimeRelayCallback = &OptCatalog::SendPkt0A_RemoveRuntimeRelay;
    zEffect_Anim::SetActivationDispatchContext(
        &SendPkt13_EffectAnimActivationRecord,
        0x0c
    );
}
} // namespace GameNet

namespace Net {
/**
 * Purpose: Initialize multiplayer mission state from net.zrd spawn points,
 * create the local player row, initialize host HUD timer state, and respawn
 * the local player.
 */
void __cdecl InitFromZrd() {
    zUtil_SaveGameState *saveState = g_PlayerSaveStateListHead;
    while (saveState != 0) {
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        if (playerState->lifecycleState == 2) {
            playerState->lifecycleState = 4;
            zClass_NodePartial *const rootNode = playerState->rootNode;
            zClass_Class::RemoveChild(
                rootNode->listA[0],
                rootNode
            );
        }
        saveState = saveState->next;
    }

    zTurret_System::DisableTickCallback();
    zReader::Node *const treeRoot = zReader::LoadNodeFromPath(
        "net.zrd",
        0,
        0
    );
    if (treeRoot != 0) {
        GameNetReaderArray *const rootArray = (GameNetReaderArray *)(treeRoot->value.ptr);
        GameNetReaderArray *const spawnArray =
            (GameNetReaderArray *)(rootArray->nodes[0].value.ptr);
        int spawnPointCount = spawnArray->count - 1;
        for (int index = 0; index < spawnPointCount; ++index) {
            GameNetSpawnPoint *const spawnPoint =
                (GameNetSpawnPoint *)(::operator new(sizeof(GameNetSpawnPoint)));
            memset(
                spawnPoint,
                0,
                sizeof(GameNetSpawnPoint)
            );
            if (g_GameNetSpawnPointCount == 0) {
                g_GameNetSpawnPointHead = spawnPoint;
            } else {
                g_GameNetSpawnPointTail->next = spawnPoint;
            }
            g_GameNetSpawnPointTail = spawnPoint;
            spawnPoint->next = 0;
            ++g_GameNetSpawnPointCount;

            GameNetReaderArray *const spawnValueArray =
                (GameNetReaderArray *)(spawnArray->nodes[index].value.ptr);
            spawnPoint->position.x = spawnValueArray->nodes[0].value.f32;
            spawnPoint->position.y = spawnValueArray->nodes[1].value.f32;
            spawnPoint->position.z = spawnValueArray->nodes[2].value.f32;
            spawnPoint->yawDegrees = spawnValueArray->nodes[3].value.f32;
        }
        zReader::FreeLoadedTree(treeRoot);
    }

    zUtil_SaveGameState *const localSaveState = (zUtil_SaveGameState *)(g_GameStateOrMapTable);
    GameNetPlayerRow *const playerRow = GameNetPlayerRowList::AppendNewRow(
        &g_GameNetPlayerRowList,
        1
    );
    playerRow->saveState = (GameNetPlayerSaveState *)(localSaveState);
    playerRow->playerKey = zNetwork_GetLocalPlayerKey();
    zNetwork::GetPlayerNameByKey(
        playerRow->playerKey,
        playerRow->displayName,
        sizeof(playerRow->displayName)
    );
    playerRow->playerColorIndex = zNetwork_GetPlayerColorIndexByKey(playerRow->playerKey);

    if (playerRow->playerColorIndex <= 0) {
        if (zNetwork::IsHost() == 0) {
            playerRow->playerColorIndex = GameNet::WaitForLocalPlayerColorIndex(60);
        }
        if (playerRow->playerColorIndex <= 0) {
            zVideo_dd::FlipToGDIIfAttached();
            Briefing::StopAndShutdownThread(0);
            zSndSystem::Shutdown();
            zNetwork::ShutdownSessionRuntime();
            zVideo::ShutdownVideoSystem();
            char fatalErrorCaption[0x80];
            char fatalErrorMessage[0x80];
            strcpy(
                fatalErrorCaption,
                zLoc::GetMessageString(18)
            );
            strcpy(
                fatalErrorMessage,
                zLoc::GetMessageString(26)
            );
            printf(
                "%s: %s\n",
                fatalErrorCaption,
                fatalErrorMessage
            );
            Sleep(1000);
            MessageBeep(MB_ICONHAND);
            MessageBoxA(
                g_RecoilApp_hWndMain,
                fatalErrorMessage,
                fatalErrorCaption,
                MB_ICONHAND
            );
            zSys::ExitProcessWithCleanup(2);
        }
    }

    if (zNetwork::IsHost() != 0) {
        const unsigned int styleColor =
            g_GameNetPlayerRowStyleColors_00RRGGBB[playerRow->playerColorIndex];
        playerRow->playerColorPackedRgb = styleColor;
        playerRow->hudWidget.textColor0 = styleColor;
        playerRow->hudWidget.textColor1 = styleColor;
        playerRow->hudWidget.textDirty = 1;
        playerRow->ApplyPlayerColorTint();
        if (g_HudSensorTracker.raceCheckpointMode == 0) {
            float runtimeTimerSec;
            memcpy(
                &runtimeTimerSec,
                &g_HudSensorTracker.runtimeTimerSecRaw,
                sizeof(runtimeTimerSec)
            );
            g_GameNetHostHudTimerInitFlag = 0;
            HudUiTimerPanel::SetSeconds(
                runtimeTimerSec,
                -1.0f
            );
            g_HudTimerPanelNetState.timerDirectionNeg = 1;
            g_HudTimerPanelNetState.statusBitsResendDeadline = 30.0f;
        }
    }

    g_HudTimerPanelNetState.timeWarningShown = 0;
    g_HudTimerPanelNetState.oneMinuteWarningShown = 0;
    GameNet::RefreshPlayerListMenu(playerRow);
    localSaveState->netPlayerRow = playerRow;
    GameNet::RespawnPlayerAndDropWeaponPickupIfAllowed(
        localSaveState,
        1
    );
    if (g_HudSensorTracker.raceCheckpointMode != 0) {
        GameNet::ResetHudTimerPanelNetStateLongCountdown();
    }
    g_GameNetPkt06InitialSyncGate = 1;
    g_GameNetPkt06NextSendTimeSec = 0.0f;
}
} // namespace Net

namespace GameNet {
/**
 * Purpose: Pump pending DirectPlay messages until the local player receives a
 * positive color index or the wait budget expires.
 */
int __fastcall WaitForLocalPlayerColorIndex(
    int maxWaitSeconds
) {
    int waitedSeconds = 0;
    while (waitedSeconds < maxWaitSeconds) {
        zNetworkDPlay::ReceivePendingMessages(-1);

        const int colorIndex = zNetwork_GetLocalPlayerColorIndex();
        if (colorIndex > 0) {
            return colorIndex;
        }

        Sleep(1000);
        ++waitedSeconds;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.gamenet-reset-remote-players-and-spawn-lists
 * @recoil-artifact defines .text recoil:function:0x4320f0: GameNet::ResetRemotePlayersAndSpawnLists.
 * Purpose: Clear remote player HUD rows and network spawn-point lists.
 */
void __cdecl ResetRemotePlayersAndSpawnLists() {
    GameNetPlayerRow *row = g_GameNetPlayerRowHead;
    while (row != 0) {
        HudUi::RemoveScoreboardEntryRow(row);
        g_HudUiTopMessageStack->RemoveChild((HudUiElement *)(&row->hudWidget));
        row = row->next;
    }

    GameNetSpawnPoint *spawnPoint = g_GameNetSpawnPointHead;
    while (spawnPoint != 0) {
        GameNetSpawnPoint *const next = spawnPoint->next;
        ::operator delete(spawnPoint);
        spawnPoint = next;
    }

    g_GameNetSpawnPointList.flags = 0;
    g_GameNetSpawnPointTail = 0;
    g_GameNetSpawnPointHead = 0;
    g_GameNetSpawnPointCount = 0;

    row = g_GameNetPlayerRowHead;
    while (row != 0) {
        GameNetPlayerRow *const next = row->next;
        row->DestroyEmbeddedPanel();
        ::operator delete(row);
        row = next;
    }

    g_GameNetPlayerRowList.flags = 0;
    g_GameNetPlayerRowTail = 0;
    g_GameNetPlayerRowHead = 0;
    g_GameNetPlayerRowCount = 0;
}
} // namespace GameNet

namespace GameNet {
/**
 * Purpose: Remove all gameplay packet handlers registered with zNetwork.
 */
void __cdecl UnregisterGameplayPacketHandlers() {
    zNetwork::UnregisterPacketHandler(
        6,
        (zNetworkPacketHandler)&HandlePkt06_PlayerStateSnapshot
    );
    zNetwork::UnregisterPacketHandler(
        7,
        (zNetworkPacketHandler)&HandlePkt07_AltGunDispatch
    );
    zNetwork::UnregisterPacketHandler(
        0x0a,
        (zNetworkPacketHandler)&OptCatalog::HandlePkt0A_RemoveRuntimeRelay
    );
    zNetwork::UnregisterPacketHandler(
        1,
        (zNetworkPacketHandler)&ReassignPlayerColorsAndRefreshRows
    );
    zNetwork::UnregisterPacketHandler(
        8,
        (zNetworkPacketHandler)&HandlePkt08_PlayerKillEvent
    );
    zNetwork::UnregisterPacketHandler(
        9,
        (zNetworkPacketHandler)&HandlePkt09_PlayerScoreboardSnapshot
    );
    zNetwork::UnregisterPacketHandler(
        0x0b,
        (zNetworkPacketHandler)&HandlePkt0B_ChatMessage
    );
    zNetwork::UnregisterPacketHandler(
        0x0e,
        (zNetworkPacketHandler)&HandlePkt0E_PlayerLapProgress
    );
    zNetwork::UnregisterPacketHandler(
        0x0c,
        (zNetworkPacketHandler)&HandlePkt0C_HudTimerStatusBits
    );
    zNetwork::UnregisterPacketHandler(
        0x0d,
        (zNetworkPacketHandler)&HandlePkt0D_HudTimerPanelState
    );
    zNetwork::UnregisterPacketHandler(
        0x0f,
        (zNetworkPacketHandler)&zDEClient_Crater::NetRelayCallback
    );
    zNetwork::UnregisterPacketHandler(
        0x10,
        (zNetworkPacketHandler)&zDEClient_QSand::NetRelayCallback
    );
    zNetwork::UnregisterPacketHandler(
        0x11,
        (zNetworkPacketHandler)&Pickup::HandlePkt11_SpawnDelta
    );
    zNetwork::UnregisterPacketHandler(
        0x12,
        (zNetworkPacketHandler)&Pickup::HandlePkt12_AirdropSpawnChuteRelay
    );
    zNetwork::UnregisterPacketHandler(
        0x13,
        (zNetworkPacketHandler)&HandlePkt13_EffectAnimActivationRecord
    );
    g_GameNet_HandlersRegistered = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.gamenet-reset-hud-timer-panel-net-state-long-countdown
 * @recoil-artifact defines .text recoil:function:0x4322a0: GameNet::ResetHudTimerPanelNetStateLongCountdown.
 * Purpose: Reset the replicated HUD timer state to the long race countdown
 * defaults and update the displayed timer panel.
 */
void __cdecl ResetHudTimerPanelNetStateLongCountdown() {
    g_HudTimerPanelNetState.timerSeconds = 36000.0f;
    HudUiTimerPanel::SetSeconds(
        36000.0f,
        -1.0f
    );
    g_HudTimerPanelNetState.startCountdownTriggered = 0;
    g_HudTimerPanelNetState.tenSecondWarningsEnabled = 0;
    g_HudTimerPanelNetState.timeWarningThresholdSec = 120.0f;
    g_HudTimerPanelNetState.timerDirectionNeg = 1;
    g_HudTimerPanelNetState.startGateTriggered = 0;
    g_HudTimerPanelNetState.raceFinishCountdownTriggered = 0;
    g_GameNetAllPlayersLapTargetCheckStarted = 0;
    memset(
        g_HudTimerPanelNetState.tailFlags,
        0,
        sizeof(g_HudTimerPanelNetState.tailFlags)
    );
    g_GameNetOneLapLeftMessageShown = 0;
}

/**
 * Purpose: Replicate the local pkt06 player-state snapshot and drive host HUD
 * timer warning/status packet updates.
 */
int __fastcall TickLocalPlayerPkt06ReplicationAndHudTimer(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerModalState *const primaryModalState = saveState->primaryModalState;

    if (zOpt::GetNetworkEnabled() == 0) {
        return 0;
    }

    if (zNetwork::IsHost() == 0 && g_GameNetPkt06InitialSyncGate != 0) {
        return 0;
    }

    g_GameNetPkt06InputBit16Latch |= playerState->netInputBit16Latch;
    g_GameNetPkt06InputBit17Latch |= playerState->netInputBit17Latch;
    if (g_Time_AccumulatedTimeSec < g_GameNetPkt06NextSendTimeSec) {
        return 1;
    }

    g_GameNetPkt06NextSendTimeSec = g_Time_AccumulatedTimeSec + ::kGameNetPkt06SendIntervalSec;

    NetPkt06_PlayerStateSnapshot *const packet = &g_NetPkt06_PlayerStateSnapshotBuf;
    packet->header.packetType = 0x06;
    packet->header.packetSizeBytes = 0x44;
    packet->header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    packet->cachedAltSelectionCode = (short)(playerState->cachedAltSelectionCode);
    packet->cachedPrimarySelectionCode = (short)(playerState->cachedPrimarySelectionCode);

    unsigned int packedFlags = packet->packedMasterTypeColorFlags;
    packedFlags = (packedFlags & ~0xffu) |
                  ((unsigned int)(primaryModalState->masterModalData->masterType) & 0xffu);
    packedFlags = (packedFlags & ~0xff00u) |
                  (((unsigned int)(GetLocalPlayerColorIndexOrZero()) & 0xffu) << 8);
    if ((g_GameNetPkt06InputBit16Latch & 1) != 0) {
        packedFlags |= 0x10000u;
    } else {
        packedFlags &= ~0x10000u;
    }
    g_GameNetPkt06InputBit16Latch = 0;
    if ((g_GameNetPkt06InputBit17Latch & 1) != 0) {
        packedFlags |= 0x20000u;
    } else {
        packedFlags &= ~0x20000u;
    }
    g_GameNetPkt06InputBit17Latch = 0;

    packet->altGunAimOrigin = playerState->altGunAimOrigin;
    packet->storedTargetPos = playerState->storedTargetPos;
    packet->worldPos = playerState->worldPos;
    packet->vehicleRotationAngles = playerState->vehicleRotationAngles;
    packet->statusMeterValue = playerState->statusMeterValue;

    if (playerState->progressTargetCount > 0) {
        packedFlags |= 0x40000u;
        packet->header.packetSizeBytes =
            (short)(0x44 + 4 + playerState->progressTargetCount * sizeof(zVec3));
        packet->progressTargetCount = playerState->progressTargetCount;
        for (int progressIndex = 0;
             progressIndex < playerState->progressTargetCount;
             ++progressIndex) {
            const zVec3 *const targetPos =
                playerState->progressTargetSlots[progressIndex].targetPos;
            packet->progressTargetPoints[progressIndex] = *targetPos;
        }
    } else {
        packedFlags &= ~0x40000u;
    }
    packet->packedMasterTypeColorFlags = packedFlags;

    const int sendResult = zNetwork_SendPacketUnreliable(&packet->header);
    const int raceCheckpointMode = g_HudSensorTracker.raceCheckpointMode;
    if (zNetwork::IsHost() != 0) {
        if (raceCheckpointMode != 0) {
            HudTimerPanelNetState timerState = g_HudTimerPanelNetState;
            const float timerSeconds = HudUiTimerPanel::GetSeconds();
            timerState.startCountdownTriggered = 0;
            timerState.timerSeconds = timerSeconds;

            if (timerState.startGateTriggered == 0) {
                if (timerSeconds <= 0.0f) {
                    timerState.startGateTriggered = 1;
                    timerState.timerDirectionNeg = 0;
                    timerState.timerSeconds = 0.0f;

                    /**
                     * GameNet startgate effect literal.
                     * Data owner gate remains pending; this docblock records
                     * source provenance only.
                     * Purpose: name the replicated start-gate effect animation
                     * stopped when the host race countdown reaches zero.
                     */
                    zEffectAnim::SetVelocity_Thunk(
                        zEffectAnim::FindEntryByName("startgate"),
                        0,
                        0.0f,
                        0.0f,
                        0.0f
                    );
                    SendPkt0D_HudTimerPanelState(&timerState);
                } else if (g_HudTimerPanelNetState.startCountdownTriggered == 0 &&
                           g_HudTimerPanelNetState.tenSecondWarningsEnabled != 0 &&
                           timerSeconds <= ::kGameNetHudTimerTenSecondThreshold) {
                    timerState.timerSeconds = ::kGameNetHudTimerTenSecondThreshold;
                    HudUiTimerPanel::SetSeconds(
                        g_FrameDeltaTimeSec + ::kGameNetHudTimerTenSecondThreshold,
                        -1.0f
                    );
                    timerState.startCountdownTriggered = 1;
                    SendPkt0D_HudTimerPanelState(&timerState);
                } else if (timerSeconds > ::kGameNetHudTimerTenSecondThreshold &&
                           (int)(timerSeconds) % 10 == 0) {
                    if (g_GameNetHudTimerTenSecondWarningArmed != 0) {
                        HudUi::ShowTopMessageLine(
                            zLoc::GetMessageString(0x32),
                            ::kGameNetHudTimerWarningDurationSec
                        );
                        HudUi::ShowTopMessageLine(
                            zLoc::GetMessageString(0x31),
                            ::kGameNetHudTimerWarningDurationSec
                        );
                        g_GameNetHudTimerTenSecondWarningArmed = 0;
                    }
                } else {
                    g_GameNetHudTimerTenSecondWarningArmed = 1;
                }
            }

            if (timerState.raceFinishCountdownTriggered != 0 && timerState.timeWarningShown == 0) {
                timerState.timeWarningShown = 1;
                SendPkt0D_HudTimerPanelState(&timerState);
                return sendResult;
            }
        } else {
            const float timerSeconds = HudUiTimerPanel::GetSeconds();
            g_HudTimerPanelNetState.timerSeconds = timerSeconds;
            HudTimerPanelNetState timerState = g_HudTimerPanelNetState;

            if (timerState.oneMinuteWarningShown == 0 &&
                timerSeconds < g_FrameDeltaTimeSec + ::kGameNetHudTimerOneMinuteLeadSec) {
                timerState.oneMinuteWarningShown = 1;
                SendPkt0C_HudTimerStatusBits(&timerState);
            }

            if (g_HudTimerPanelNetState.timeWarningShown == 0 &&
                timerSeconds < g_FrameDeltaTimeSec) {
                timerState.timeWarningShown = 1;
                SendPkt0C_HudTimerStatusBits(&timerState);
            }

            if (g_Time_AccumulatedTimeSec > g_HudTimerPanelNetState.statusBitsResendDeadline) {
                SendPkt0C_HudTimerStatusBits(&timerState);
                return sendResult;
            }
        }

        return sendResult;
    }

    if (raceCheckpointMode != 0) {
        HudTimerPanelNetState timerState = g_HudTimerPanelNetState;
        if (timerState.startGateTriggered != 0 || timerState.startCountdownTriggered != 0) {
            g_GameNetHudTimerPendingSaveReminderArmed = 1;
        } else if ((int)(HudUiTimerPanel::GetSeconds()) % 10 != 0) {
            g_GameNetHudTimerPendingSaveReminderArmed = 1;
        } else if (g_GameNetHudTimerPendingSaveReminderArmed != 0) {
            HudUi::ShowTopMessageLine(
                zLoc::GetMessageString(0x34),
                ::kGameNetHudTimerWarningDurationSec
            );
            HudUi::ShowTopMessageLine(
                zLoc::GetMessageString(0x33),
                ::kGameNetHudTimerWarningDurationSec
            );
            g_GameNetHudTimerPendingSaveReminderArmed = 0;
            return sendResult;
        }
    }

    return sendResult;
}

/**
 * Purpose: Dispatch an incoming player-state snapshot to row creation or
 * existing-row update handling.
 */
int __fastcall HandlePkt06_PlayerStateSnapshot(
    int senderPlayerId,
    NetPkt06_PlayerStateSnapshot *packet
) {
    if (packet == 0) {
        return -1;
    }

    GameNetPlayerRow *const row = FindPlayerRowByKey(packet->header.payloadDword0);
    if (g_GameNetPkt06InitialSyncGate != 0) {
        g_GameNetPkt06InitialSyncGate = 0;
    }

    if (packet->header.packetType == 6) {
        if (row == 0) {
            SpawnRemotePlayerFromPkt06_PlayerStateSnapshot(
                senderPlayerId,
                packet
            );
            return 0;
        }

        ApplyPkt06_PlayerStateSnapshotToRow(
            row,
            packet
        );
    }

    return 0;
}

/**
 * Purpose: Find the active GameNet remote-player row for a network player key.
 */
GameNetPlayerRow *__fastcall FindPlayerRowByKey(
    int playerKey
) {
    GameNetPlayerRow *row = g_GameNetPlayerRowHead;
    while (row != 0) {
        if (row->playerKey == playerKey) {
            return row;
        }

        row = row->next;
    }

    return 0;
}

/**
 * Purpose: Create a remote player row and cloned player node from an incoming
 * player-state snapshot packet.
 */
int __fastcall SpawnRemotePlayerFromPkt06_PlayerStateSnapshot(
    int senderPlayerId,
    NetPkt06_PlayerStateSnapshot *packet
) {
    char displayNameScratch[0x40];
    if (zNetwork::GetPlayerNameByKey(
            senderPlayerId,
            displayNameScratch,
            sizeof(displayNameScratch)
        ) == 0) {
        zNetwork_DPlay::EnumPlayers();
    }

    zClass_NodePartial *const sourceNode = zClass::FindByTypeAndName(
        6,
        "bft_99"
    );
    zClass_NodePartial *clonedNode = 0;
    if (sourceNode != 0) {
        clonedNode = zClass_cls_util::CopyNodeWithCloneOptions(
            sourceNode,
            1,
            1
        );
    }

    if (clonedNode == 0) {
        HudUi::ShowTopMessageLine(
            zLoc::GetMessageString(0x911),
            5.0f
        );
        return 0;
    }

    clonedNode->flags |= ::kGameNetRemoteCloneNodeFlag;

    char netNodeName[0x14];
    sprintf(
        netNodeName,
        "net%d",
        packet->header.payloadDword0
    );
    zClass_Class::gwNodeSetName(
        clonedNode,
        netNodeName
    );

    zUtil_SaveGameState *const saveState = Player::CreateFromNamesAtPoseGetState(
        &packet->worldPos,
        g_Player_NodeName_Bft,
        packet->vehicleRotationAngles.y,
        netNodeName
    );
    if (saveState != 0) {
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        playerState->lifecycleState = 3;
        playerState->amphibUnlocked = 1;
        playerState->hoverUnlocked = 1;
        playerState->subUnlocked = 1;
        for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
            PlayerAltWeaponBank &bank = playerState->altWeaponBanks[bankIndex];
            bank.controllerA.flags |= 4u;
            bank.controllerA.ammoOrCharge = ::kGameNetRemoteUnlimitedAmmo;
            bank.controllerB.flags |= 4u;
            bank.controllerB.ammoOrCharge = ::kGameNetRemoteUnlimitedAmmo;
        }
    }

    GameNetPlayerRowListState *const rowList = &g_GameNetPlayerRowList;
    GameNetPlayerRow *const row = GameNetPlayerRowList::AppendNewRow(
        rowList,
        0
    );
    row->playerKey = packet->header.payloadDword0;
    row->playerColorIndex = (int)((packet->packedMasterTypeColorFlags >> 8) & 0xffu);
    row->playerNode = clonedNode;
    row->score = 0;
    row->lapCount = 0;
    row->turretNode = zClass_Class::FindSubNodeByName(
        clonedNode,
        g_Player_NodeName_Turret
    );
    row->gunNode = zClass_Class::FindSubNodeByName(
        clonedNode,
        "gun"
    );
    row->saveState = (GameNetPlayerSaveState *)saveState;

    if (zNetwork::GetPlayerNameByKey(senderPlayerId, row->displayName, sizeof(row->displayName)) !=
        0) {
        HudUi::ShowTopMessageLine(
            row->displayName,
            5.0f
        );
    }
    HudUi::ShowTopMessageLine(
        zLoc::GetMessageString(0x912),
        5.0f
    );

    HudUiPanel *const hudWidget = &row->hudWidget;
    hudWidget->SetText(row->displayName);
    const unsigned int hudColor = g_GameNetPlayerRowStyleColors_00RRGGBB[0];
    hudWidget->textColor0 = hudColor;
    hudWidget->textColor1 = hudColor;
    hudWidget->textDirty = 1;
    hudWidget->SetVisible(0);
    g_HudUiTopMessageStack->AddChild((HudUiElement *)(hudWidget));

    if (saveState != 0) {
        saveState->netPlayerRow = row;
        if (row->playerNode->listCountA == 0) {
            zClass_Class::AddChild(
                g_Player_RuntimeDiScene,
                row->playerNode
            );
        }
        zClass_Class::gwNodeSetActive(
            row->playerNode,
            1
        );
    }

    RefreshPlayerListMenu(row);
    ReassignPlayerColorsAndRefreshRows(
        0,
        0
    );

    if (zNetwork::IsHost() != 0) {
        SendPkt09_PlayerScoreboardSnapshot();
        zDEClient::DispatchFeatureEventTemplates(
            HostSendPkt0F_CraterFeature,
            HostSendPkt10_QSandFeature
        );
        SendAllPkt13_EffectAnimActivationRecords();
        Pickup::ReconcilePrimaryAndNetworkCopySpawnLists();

        HudTimerPanelNetState timerState = g_HudTimerPanelNetState;
        if (g_HudSensorTracker.raceCheckpointMode != 0) {
            timerState.startCountdownTriggered = 0;
            if (g_HudTimerPanelNetState.startGateTriggered != 0) {
                timerState.ClearTailFlagsLocal();
            }
            SendPkt0D_HudTimerPanelState(&timerState);
        } else {
            SendPkt0C_HudTimerStatusBits(&timerState);
        }
    }

    ApplyPkt06_PlayerStateSnapshotToRow(
        row,
        packet
    );
    return 1;
}

/**
 * Purpose: Apply a replicated player-state snapshot packet to an existing
 * remote-player row and its save-state storage.
 */
int __fastcall ApplyPkt06_PlayerStateSnapshotToRow(
    GameNetPlayerRow *row,
    NetPkt06_PlayerStateSnapshot *packet
) {
    GameNetPlayerSaveState *const rowSaveState = row->saveState;
    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)rowSaveState;
    zUtil_PlayerStateStorage *const playerState = rowSaveState->playerState;
    PlayerMasterModalData *const masterModalData = rowSaveState->primaryModalState->masterModalData;
    const unsigned int packedFlags = packet->packedMasterTypeColorFlags;

    playerState->netUpdateReceived = 1;

    const int colorIndex = (int)((packedFlags >> 8) & 0xffu);
    if (row->playerColorIndex != colorIndex) {
        row->playerColorIndex = colorIndex;
        const unsigned int packedColor = g_GameNetPlayerRowStyleColors_00RRGGBB[colorIndex];
        row->playerColorPackedRgb = packedColor;
        row->hudWidget.textColor0 = packedColor;
        row->hudWidget.textColor1 = packedColor;
        row->hudWidget.textDirty = 1;
        HudUi::RefreshScoreboardEntryRow(row);
        row->ApplyPlayerColorTint();
    }

    const int masterType = (int)(packedFlags & 0xffu);
    if (masterType != masterModalData->masterType) {
        Player::ApplyMasterTypeTransition(
            saveState,
            masterType,
            0
        );
    }

    playerState->netReceivedPos = packet->worldPos;
    playerState->netReceivedAngles = packet->vehicleRotationAngles;

    const int inputBit16 = (packedFlags & ::kGameNetPkt06InputBit16Flag) != 0 ? 1 : 0;
    const int inputBit17 = (packedFlags & ::kGameNetPkt06InputBit17Flag) != 0 ? 1 : 0;
    if (playerState->netLastUpdateFrameTick == g_zVideo_FrameTick) {
        playerState->netInputBit16Latch |= inputBit16;
        playerState->netInputBit17Latch |= inputBit17;
    } else {
        playerState->netInputBit16Latch = inputBit16;
        playerState->netInputBit17Latch = inputBit17;
        playerState->netLastUpdateFrameTick = g_zVideo_FrameTick;
    }

    playerState->storedTargetPos = packet->storedTargetPos;

    const int altSelectionCode = (int)((unsigned short)(packet->cachedAltSelectionCode));
    if (altSelectionCode != playerState->cachedAltSelectionCode) {
        Player::ApplyAltWeaponSwitch(
            saveState,
            playerState->activeAltGunController,
            (&playerState->altWeaponBanks[altSelectionCode / 100].controllerA) +
                (altSelectionCode % 100)
        );
    }

    const int primarySelectionCode = (int)((unsigned short)(packet->cachedPrimarySelectionCode));
    if (primarySelectionCode != playerState->cachedPrimarySelectionCode) {
        Player::ApplyPrimaryWeaponSwitch(
            saveState,
            playerState->activePrimaryGunController,
            (&playerState->altWeaponBanks[primarySelectionCode / 100].controllerA) +
                (primarySelectionCode % 100)
        );
    }

    playerState->statusMeterValue = packet->statusMeterValue;
    for (int index = 0; index < 10; ++index) {
        playerState->progressTargetRuntimeSlots[index].targetPos = 0;
    }

    if ((packedFlags & ::kGameNetPkt06ProgressTargetsFlag) == 0) {
        playerState->progressTargetCount = 0;
        return 1;
    }

    playerState->progressTargetCount = packet->progressTargetCount;
    for (int progressIndex = 0; progressIndex < playerState->progressTargetCount; ++progressIndex) {
        playerState->progressTargetPointStorage[progressIndex] =
            packet->progressTargetPoints[progressIndex];
        playerState->progressTargetRuntimeSlots[progressIndex].targetPos =
            &playerState->progressTargetPointStorage[progressIndex];
    }

    return 1;
}

/**
 * Purpose: Project a remote player name-tag HUD widget into screen space.
 */
int __fastcall UpdateRemotePlayerHudWidgetScreenPos(
    zUtil_SaveGameState *saveState
) {
    if (GetStatusBitNameTags() == 0) {
        return 0;
    }

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    HudUiPanel *const hudWidget = &saveState->netPlayerRow->hudWidget;
    zVec3 labelWorldPos = playerState->worldPos;
    labelWorldPos.y += 3.0f;

    if (AINet::HasLineOfSightFromLocalPlayerFxOffset(playerState->rootNode, &labelWorldPos, 1) ==
        0) {
        hudWidget->SetVisible(0);
        return 0;
    }

    zVec3 projectedPoint = {0};
    const int clipped = zMath::ProjectPointAndClampToScreenClip(
        &labelWorldPos,
        &projectedPoint
    );
    const float replicateScale = zOpt::GetReplicateMode() != 0 ? 2.0f : 1.0f;
    const int screenX = (int)(projectedPoint.x * replicateScale);
    const int screenY = (int)(projectedPoint.y * replicateScale) - 10;

    if (screenY <= hudWidget->QueryTextHeight() + 26) {
        hudWidget->SetVisible(0);
        return 0;
    }

    if (clipped != 0) {
        hudWidget->SetVisible(0);
        return 0;
    }

    hudWidget->SetPos(
        screenX,
        screenY
    );
    hudWidget->SetVisible(1);
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.gamenet-reassign-player-colors-and-refresh-rows
 * @recoil-artifact defines .text recoil:function:0x432e70: GameNet::ReassignPlayerColorsAndRefreshRows.
 * Purpose: Refresh player-row colors after network color assignment changes.
 */
int __cdecl ReassignPlayerColorsAndRefreshRows(
    int,
    zNetworkPacketHeader *
) {
    GameNetPlayerRow *row = g_GameNetPlayerRowHead;
    while (row != 0) {
        const int colorIndex = zNetwork_GetPlayerColorIndexByKey(row->playerKey);
        row->playerColorIndex = colorIndex;

        const unsigned int color = g_GameNetPlayerRowStyleColors_00RRGGBB[colorIndex];
        row->playerColorPackedRgb = color;
        row->hudWidget.textColor0 = color;
        row->hudWidget.textColor1 = color;
        row->hudWidget.textDirty = 1;
        HudUi::RefreshScoreboardEntryRow(row);
        row->ApplyPlayerColorTint();

        row = row->next;
    }

    return 1;
}

/**
 * Purpose: Handle the remote-player remove packet by retiring the player's
 * runtime state, unlinking the HUD row, and deleting the player row.
 */
int __fastcall HandlePkt03_RemoveRemotePlayer(
    int senderPlayerId,
    zNetworkPacketHeader *
) {
    GameNetPlayerRow *const row = FindPlayerRowByKey(senderPlayerId);
    if (row == 0) {
        return 0;
    }

    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)row->saveState;
    if (saveState != 0) {
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        playerState->cameraTransitionTimer = 1;
        playerState->lifecycleState = 4;
        Player::ResetAltGunRuntimeState(saveState);
        Player::RemoveAllDeployedMines(saveState);
    }

    char message[0x80] = {0};
    zLoc::FormatMessage(
        message,
        sizeof(message),
        0x913,
        row->displayName
    );
    HudUi::ShowTopMessageLine(
        message,
        5.0f
    );
    HudUi::RemoveScoreboardEntryRow(row);
    HudUiPanel *const hudWidget = &row->hudWidget;
    hudWidget->SetVisible(0);
    g_HudUiTopMessageStack->RemoveChild((HudUiElement *)(hudWidget));

    if (g_GameNetPlayerRowCount == 0) {
        return 0;
    }

    if (row == g_GameNetPlayerRowHead) {
        --g_GameNetPlayerRowCount;
        GameNetPlayerRow *const next = row->next;
        g_GameNetPlayerRowHead = next;
        if (next == 0) {
            g_GameNetPlayerRowList.flags = 0;
            g_GameNetPlayerRowTail = 0;
        }
    } else {
        GameNetPlayerRow *previous = g_GameNetPlayerRowHead;
        while (previous != 0 && previous->next != row) {
            previous = previous->next;
        }

        if (previous == 0) {
            return 0;
        }

        --g_GameNetPlayerRowCount;
        previous->next = row->next;
        if (g_GameNetPlayerRowTail == row) {
            g_GameNetPlayerRowTail = previous;
        }
    }

    row->DestroyEmbeddedPanel();
    ::operator delete(row);
    return 0;
}

/**
 * Purpose: Build, send, and locally dispatch a packet-08 player kill event.
 */
void __fastcall SendPkt08_PlayerKillEvent(
    zUtil_SaveGameState *saveState,
    short killMethodOrOptCatalogEntryId
) {
    zUtil_SaveGameState *saveStateOrLocal = saveState;
    if (saveStateOrLocal == 0) {
        saveStateOrLocal = (zUtil_SaveGameState *)(g_GameStateOrMapTable);
    }

    NetPkt08_PlayerKillEvent packet;
    packet.header.packetType = 0x08;
    packet.header.packetSizeBytes = sizeof(NetPkt08_PlayerKillEvent);
    packet.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    packet.killMethodOrOptCatalogEntryId = killMethodOrOptCatalogEntryId;
    packet.targetPlayerKey = saveStateOrLocal->netPlayerRow->playerKey;

    zNetwork_SendPacketReliable(&packet.header);
    HandlePkt08_PlayerKillEvent(
        zNetwork_GetLocalPlayerKey(),
        &packet
    );
}

/**
 * Purpose: Apply an incoming packet-08 player kill event and host-side
 * scoreboard update.
 */
int __fastcall HandlePkt08_PlayerKillEvent(
    int localPlayerKey,
    NetPkt08_PlayerKillEvent *packet
) {
    GameNetPlayerRow *const killerRow = FindPlayerRowByKey(localPlayerKey);
    GameNetPlayerRow *const victimRow = FindPlayerRowByKey(packet->targetPlayerKey);
    if (killerRow == 0 || victimRow == 0) {
        return 0;
    }

    OptCatalogEntryDef *killEntry = 0;
    const short killEntryId = packet->killMethodOrOptCatalogEntryId;
    if (killEntryId != 0) {
        killEntry = OptCatalog::FindEntryById(killEntryId);
    }

    ShowPlayerKillMessage(
        victimRow,
        killEntry,
        killerRow
    );

    if (zNetwork::IsHost() != 0) {
        const int score = victimRow->score;
        if (victimRow != killerRow) {
            victimRow->score = score + 1;
        } else {
            victimRow->score = score - 1;
            if (victimRow->score < 0) {
                victimRow->score = 0;
            }
        }

        SendPkt09_PlayerScoreboardSnapshot();
    }

    return 1;
}

/**
 * Purpose: Publish the local player's packed lap count and lap time packet.
 */
void __fastcall SendPkt0E_PlayerLapProgress(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    NetPkt0E_PlayerLapProgress packet;
    packet.header.packetType = 0x0e;
    packet.header.packetSizeBytes = sizeof(NetPkt0E_PlayerLapProgress);
    packet.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    packet.lapCountPacked = (short)(playerState->lapCount);
    packet.reserved_0a = 0;
    packet.lapTimeSec = playerState->lapTimeSec;

    if (zNetwork::IsHost() != 0) {
        saveState->netPlayerRow->lapCount = playerState->lapCount;
        saveState->netPlayerRow->lapTimeSec = playerState->lapTimeSec;
        HandlePkt0E_PlayerLapProgress(
            packet.header.payloadDword0,
            &packet
        );
    } else {
        zNetwork_SendPacketReliable(&packet.header);
    }
}

/**
 * Purpose: Apply a host-side player lap-progress packet and refresh race HUD
 * state when the lap target is reached.
 */
int __fastcall HandlePkt0E_PlayerLapProgress(
    int senderPlayerId,
    NetPkt0E_PlayerLapProgress *packet
) {
    int result = zNetwork::IsHost();
    if (result == 0) {
        return result;
    }

    GameNetPlayerRow *const row = FindPlayerRowByKey(senderPlayerId);
    if (row == 0) {
        return 0;
    }

    row->lapCount = packet->lapCountPacked;
    row->lapTimeSec = packet->lapTimeSec;
    SendPkt09_PlayerScoreboardSnapshot();

    if (row->lapCount >= g_HudSensorTracker.runtimeGoalValue) {
        HudTimerPanelNetState timerState = g_HudTimerPanelNetState;
        if (AreAllPlayersAtLapTarget() != 0) {
            timerState.timeWarningShown = 1;
            timerState.raceFinishCountdownTriggered = 1;
        }

        SendPkt0D_HudTimerPanelState(&timerState);
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.gamenet-are-all-players-at-lap-target
 * @recoil-artifact defines .text recoil:function:0x433200: GameNet::AreAllPlayersAtLapTarget.
 * Purpose: Mark the multiplayer lap-target check as started and report
 * whether every player row has reached the race goal.
 */
int __cdecl AreAllPlayersAtLapTarget() {
    if (g_GameNetAllPlayersLapTargetCheckStarted == 0) {
        g_GameNetAllPlayersLapTargetCheckStarted = 1;
    }

    GameNetPlayerRow *row = g_GameNetPlayerRowHead;
    while (row != 0) {
        if (row->lapCount < g_HudSensorTracker.runtimeGoalValue) {
            return 0;
        }

        row = row->next;
    }

    return 1;
}

/**
 * Purpose: Apply the host HUD timer panel state packet to local timer state.
 */
int __fastcall HandlePkt0D_HudTimerPanelState(
    int,
    NetPkt0D_HudTimerPanelState *packet
) {
    const int statusBits = packet->hudTimerFlagsPacked;
    g_HudTimerPanelNetState.timerSeconds = packet->seconds;
    g_HudTimerPanelNetState.timerDirectionNeg = statusBits & 1;

    float secondsStep = -1.0f;
    if (g_HudTimerPanelNetState.timerDirectionNeg == 0) {
        secondsStep = 1.0f;
    }

    HudUiTimerPanel::SetSeconds(
        g_HudTimerPanelNetState.timerSeconds,
        secondsStep
    );

    if (g_HudTimerPanelNetState.startGateTriggered == 0 && (statusBits & 8) != 0) {
        g_HudTimerPanelNetState.startGateTriggered = 1;
    }

    if ((statusBits & 0x20) != 0) {
        zEffectAnim::SetVelocity_Thunk(
            /* Retail literal 0x4dcfd4 names the replicated start-countdown
               effect animation; data ownership remains blocked outside this
               source slice. */
            zEffectAnim::FindEntryByName("startcountdown"),
            0,
            0.0f,
            0.0f,
            0.0f
        );
        g_HudTimerPanelNetState.startCountdownTriggered = 1;
    }

    if (g_HudTimerPanelNetState.raceFinishCountdownTriggered == 0 && (statusBits & 0x10) != 0) {
        g_HudTimerPanelNetState.raceFinishCountdownTriggered = 1;
    }

    g_HudTimerPanelNetState.timeWarningShown = statusBits & 2;
    if (g_HudTimerPanelNetState.timeWarningShown != 0) {
        g_RecoilApp.QueueSwitchCurrentState(
            &g_RecoilApp.m_mpExitDialogState,
            0
        );
    }

    return 1;
}

/**
 * Purpose: Send and locally apply the host HUD timer panel state packet.
 */
void __fastcall SendPkt0D_HudTimerPanelState(
    HudTimerPanelNetState *timerState
) {
    if (zNetwork::IsHost() == 0) {
        return;
    }

    g_NetPkt0D_HudTimerPanelStateBuf.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    g_NetPkt0D_HudTimerPanelStateBuf.seconds = HudUiTimerPanel::GetSeconds();

    short statusBits = 0;
    if (timerState->timerDirectionNeg != 0) {
        statusBits = 1;
    }
    if (timerState->startGateTriggered != 0) {
        statusBits |= 8;
    }
    if (timerState->timeWarningShown != 0) {
        statusBits |= 2;
    }
    if (timerState->raceFinishCountdownTriggered != 0) {
        statusBits |= 0x10;
    }
    if (timerState->startCountdownTriggered != 0) {
        statusBits |= 0x20;
    }

    g_NetPkt0D_HudTimerPanelStateBuf.hudTimerFlagsPacked = statusBits;
    zNetwork_SendPacketReliable(&g_NetPkt0D_HudTimerPanelStateBuf.header);
    HandlePkt0D_HudTimerPanelState(
        g_NetPkt0D_HudTimerPanelStateBuf.header.payloadDword0,
        &g_NetPkt0D_HudTimerPanelStateBuf
    );
}

/**
 * Purpose: Send and locally apply replicated HUD timer status bits.
 */
int __fastcall SendPkt0C_HudTimerStatusBits(
    HudTimerPanelNetState *timerState
) {
    const int result = zNetwork::IsHost();
    if (result == 0) {
        return result;
    }

    g_NetPkt0C_HudTimerStatusBitsBuf.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    g_NetPkt0C_HudTimerStatusBitsBuf.timerSeconds = HudUiTimerPanel::GetSeconds();

    short statusBits = 0;
    if (timerState->timerDirectionNeg != 0) {
        statusBits = 1;
    }
    if (timerState->timeWarningShown != 0) {
        statusBits |= 2;
    }
    if (timerState->oneMinuteWarningShown != 0) {
        statusBits |= 4;
    }

    g_NetPkt0C_HudTimerStatusBitsBuf.statusBitsPackedHiWord = statusBits;
    g_HudTimerPanelNetState.statusBitsResendDeadline = g_Time_AccumulatedTimeSec + 30.0f;
    zNetwork_SendPacketReliable(&g_NetPkt0C_HudTimerStatusBitsBuf.header);
    return HandlePkt0C_HudTimerStatusBits(
        g_NetPkt0C_HudTimerStatusBitsBuf.header.payloadDword0,
        &g_NetPkt0C_HudTimerStatusBitsBuf
    );
}

/**
 * Purpose: Apply replicated HUD timer seconds and warning status bits.
 */
int __fastcall HandlePkt0C_HudTimerStatusBits(
    int,
    NetPkt0C_HudTimerStatusBits *packet
) {
    const int statusBits = packet->statusBitsPackedHiWord;
    g_HudTimerPanelNetState.timerSeconds = packet->timerSeconds;
    g_HudTimerPanelNetState.timerDirectionNeg = statusBits & 1;

    float secondsStep = -1.0f;
    if (g_HudTimerPanelNetState.timerDirectionNeg == 0) {
        secondsStep = 1.0f;
    }

    HudUiTimerPanel::SetSeconds(
        g_HudTimerPanelNetState.timerSeconds,
        secondsStep
    );

    if (g_HudSensorTracker.raceCheckpointMode == 0 &&
        g_HudTimerPanelNetState.oneMinuteWarningShown == 0 && (statusBits & 4) != 0) {
        g_HudTimerPanelNetState.oneMinuteWarningShown = 1;
        HudUi::ShowTopMessageLine(
            zLoc::GetMessageString(0x914),
            5.0f
        );
        if (zNetwork::IsHost() != 0) {
            HostUpdateSessionDescStatusFields(
                0x100,
                0,
                0,
                0
            );
        }
    }

    if (g_HudTimerPanelNetState.timeWarningShown == 0 && (statusBits & 2) != 0) {
        HudUi::ShowTopMessageLine(
            zLoc::GetMessageString(0x915),
            5.0f
        );
        g_HudTimerPanelNetState.timeWarningShown = 1;
        g_RecoilApp.QueueSwitchCurrentState(
            &g_RecoilApp.m_mpExitDialogState,
            0
        );
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.gamenet-send-pkt09-player-scoreboard-snapshot
 * @recoil-artifact defines .text recoil:function:0x4334f0: GameNet::SendPkt09_PlayerScoreboardSnapshot.
 * Purpose: Send the host's packed player score and lap snapshot to peers.
 */
void __cdecl SendPkt09_PlayerScoreboardSnapshot() {
    if (zNetwork::IsHost() == 0) {
        return;
    }

    const int entryCount = (int)(g_GameNetPlayerRowCount);
    const size_t packetSize = sizeof(zNetworkPacketHeader) + sizeof(int) +
                              (size_t)(entryCount) * sizeof(NetPkt09_PlayerScoreboardEntry);
    NetPkt09_PlayerScoreboardSnapshot *const packet =
        (NetPkt09_PlayerScoreboardSnapshot *)(malloc(packetSize));
    memset(
        packet,
        0,
        packetSize
    );

    packet->header.packetType = 0x09;
    packet->header.packetSizeBytes = (unsigned short)(packetSize);
    packet->header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    packet->entryCount = entryCount;

    GameNetPlayerRow *row = g_GameNetPlayerRowHead;
    NetPkt09_PlayerScoreboardEntry *entry = packet->entries;
    while (row != 0) {
        entry->playerKey = row->playerKey;
        entry->packedScoreAndLapCount =
            (unsigned short)((row->lapCount << 9) + (row->score & 0x1ff));
        row = row->next;
        ++entry;
    }

    zNetwork_SendPacketReliable(&packet->header);
    if (zNetwork::IsHost() != 0) {
        HandlePkt09_PlayerScoreboardSnapshot(
            zNetwork_GetLocalPlayerKey(),
            packet
        );
    }

    free(packet);
}

/**
 * Purpose: Apply packed player score and lap rows and trigger HUD warnings.
 */
int __fastcall HandlePkt09_PlayerScoreboardSnapshot(
    int,
    NetPkt09_PlayerScoreboardSnapshot *packet
) {
    GameNetPlayerRow *oneLapLeftRow = 0;
    const int entryCount = packet->entryCount;

    {
        for (int index = 0; index < entryCount; ++index) {
            NetPkt09_PlayerScoreboardEntry *const entry = &packet->entries[index];
            GameNetPlayerRow *const row = FindPlayerRowByKey(entry->playerKey);
            if (row == 0) {
                continue;
            }

            const unsigned short packed = entry->packedScoreAndLapCount;
            row->score = packed & 0x1ff;
            row->lapCount = ((short)(packed) >> 9) & 0x7f;
            HudUi::RefreshScoreboardEntryRow(row);

            if (g_GameNetOneLapLeftMessageShown == 0 && oneLapLeftRow == 0 &&
                row->lapCount == g_HudSensorTracker.runtimeGoalValue - 1) {
                oneLapLeftRow = row;
            }

            if (zNetwork::IsHost() != 0 && g_HudSensorTracker.raceCheckpointMode == 0 &&
                row->score >= g_HudSensorTracker.runtimeGoalValue) {
                HudTimerPanelNetState timerState = g_HudTimerPanelNetState;
                if (timerState.timeWarningShown == 0) {
                    timerState.timeWarningShown = 1;
                    SendPkt0C_HudTimerStatusBits(&timerState);
                }
            }
        }
    }

    if (g_HudSensorTracker.raceCheckpointMode != 0 && g_GameNetOneLapLeftMessageShown == 0 &&
        oneLapLeftRow != 0) {
        char message[0x80];
        zLoc::FormatMessage(
            message,
            sizeof(message),
            0x918,
            oneLapLeftRow->displayName
        );
        HudUi::ShowTopMessageLine(
            message,
            5.0f
        );
        g_GameNetOneLapLeftMessageShown = 1;
        if (zNetwork::IsHost() != 0) {
            HostUpdateSessionDescStatusFields(
                0x100,
                0,
                0,
                0
            );
        }
    }

    return 1;
}

/**
 * Purpose: Return the local GameNet player-row color index when the local
 * save-state row is available, or zero otherwise.
 */
int __cdecl GetLocalPlayerColorIndexOrZero() {
    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(g_GameStateOrMapTable);
    if (saveState == 0) {
        return 0;
    }

    GameNetPlayerRow *const netPlayerRow = saveState->netPlayerRow;
    if (netPlayerRow == 0) {
        return 0;
    }

    return netPlayerRow->playerColorIndex;
}

/**
 * Purpose: Decode host status flags into the cached allow-map and name-tag bits.
 */
void __fastcall SetStatusBitsFromFlags(
    unsigned int statusFlags
) {
    g_GameNetStatus_AllowMaps = statusFlags & 1u;
    g_GameNetStatus_NameTags = (statusFlags >> 1) & 1u;
}

/**
 * Purpose: Return the cached status bit controlling map availability.
 */
int __cdecl GetStatusBitAllowMaps() {
    return g_GameNetStatus_AllowMaps;
}

/**
 * Purpose: Return the cached status bit controlling remote player name tags.
 */
int __cdecl GetStatusBitNameTags() {
    return g_GameNetStatus_NameTags;
}

/**
 * Purpose: Build and send a packet-0B chat message for the local player.
 */
void __fastcall SendPkt0B_ChatMessage(
    const char *message
) {
    const int messageLength = (int)(strlen(message));
    const int packetSize = messageLength + 12;
    NetPkt0B_ChatMessage *const packet = (NetPkt0B_ChatMessage *)(malloc((size_t)(packetSize)));
    memset(
        packet,
        0,
        (size_t)(packetSize)
    );

    packet->header.packetType = 0x0b;
    packet->header.packetSizeBytes = (short)(packetSize);
    packet->header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    packet->messageLength = (short)(messageLength);
    if (messageLength > 0) {
        memcpy(
            packet->message,
            message,
            (size_t)(messageLength)
        );
    }

    zNetwork_SendPacketReliable(&packet->header);
    free(packet);
}

/**
 * Purpose: Copy an incoming chat payload into a bounded local string and show it.
 */
int __fastcall HandlePkt0B_ChatMessage(
    int,
    NetPkt0B_ChatMessage *packet
) {
    char message[0x51] = {0};
    int messageLength = packet->messageLength;
    if (messageLength >= 0x50) {
        messageLength = 0x50;
    }

    if (messageLength > 0) {
        memcpy(
            message,
            packet->message,
            (size_t)(messageLength)
        );
    }

    message[messageLength] = '\0';
    HudUi::ShowChatLine(
        message,
        5.0f
    );
    return 1;
}

/**
 * Purpose: Choose a multiplayer respawn point, optionally drop the player's
 * current weapon pickup, reset transient player state, and refresh mission
 * vehicle unlock flags.
 */
void __fastcall RespawnPlayerAndDropWeaponPickupIfAllowed(
    zUtil_SaveGameState *saveState,
    int useColorIndexedSpawn
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    const int localColorIndex = GetLocalPlayerColorIndexOrZero();
    GameNetSpawnPoint *spawnPoint = g_GameNetSpawnPointHead;
    GameNetSpawnPoint *selectedSpawn = spawnPoint;

    if (useColorIndexedSpawn != 0) {
        if (localColorIndex > 1) {
            int colorIndex = 1;
            while (spawnPoint != 0 && colorIndex < localColorIndex) {
                spawnPoint = spawnPoint->next;
                ++colorIndex;
            }
        }
        selectedSpawn = spawnPoint;
    } else if (g_HudSensorTracker.raceCheckpointMode != 0) {
        selectedSpawn = 0;
    } else {
        PickupType *const pickupType = Pickup::FindDroppableTypeForPlayerCurrentWeapon(saveState);
        PickupParsedZrdEntry entry = {0};
        entry.typeDesc = pickupType;
        entry.amount = pickupType->defaultAmount;
        entry.position = playerState->worldPos;
        entry.rotation = playerState->vehicleRotationAngles;
        PickupSpawnDef *const pickupSpawn = Pickup::SpawnFromParsedZrdEntry(&entry);
        if (pickupSpawn != 0) {
            Pickup::SendPkt11_CreateDelta(pickupSpawn);
        }

        selectedSpawn = 0;
        float bestNearestDistanceSq = 0.0f;
        GameNetPlayerSaveState *nearestSaveState = 0;
        while (spawnPoint != 0) {
            const float nearestDistanceSq =
                GetNearestOtherPlayerDistanceToSpawnPoint(
                    spawnPoint,
                    &nearestSaveState
                );
            if (nearestDistanceSq > bestNearestDistanceSq) {
                bestNearestDistanceSq = nearestDistanceSq;
                selectedSpawn = spawnPoint;
            }
            spawnPoint = spawnPoint->next;
        }
    }

    if (selectedSpawn != 0) {
        const double kDegreesToRadians = 0.017453292519943295;
        const float yawRad = (float)(selectedSpawn->yawDegrees * kDegreesToRadians);
        Player::SetWorldPoseAndRestartAnchor(
            saveState,
            &selectedSpawn->position,
            yawRad
        );
    }

    if (saveState->primaryModalState->masterModalData->masterType != 3 &&
        g_HudSensorTracker.raceCheckpointMode == 0) {
        Player::TransitionToMasterTypeTrack(
            saveState,
            1
        );
    }

    Player::ResetMouseControlStateAndRecenterCursor(saveState);
    Player::ResetMotionTransientState(saveState);
    playerState->amphibUnlocked =
        Player::IsMissionProbeType1EnabledById(g_HudSensorTracker.GetMissionId());
    playerState->hoverUnlocked = 0;
    playerState->subUnlocked = 0;
}

/**
 * Purpose: Measure the nearest player-row save state other than the active
 * game-state-table row for a candidate multiplayer spawn point.
 */
float __fastcall GetNearestOtherPlayerDistanceToSpawnPoint(
    GameNetSpawnPoint *spawnPoint,
    GameNetPlayerSaveState **outSaveState
) {
    float nearestDistanceSq = 1.0e23f;
    GameNetPlayerSaveState *const localSaveState =
        (GameNetPlayerSaveState *)(g_GameStateOrMapTable);
    GameNetPlayerRow *row = g_GameNetPlayerRowHead;
    while (row != 0) {
        GameNetPlayerSaveState *const saveState = row->saveState;
        if (saveState != localSaveState) {
            const float distanceSq =
                zMath::Vec3DeltaLengthSq(
                    &saveState->playerState->worldPos,
                    &spawnPoint->position
                );
            if (distanceSq < nearestDistanceSq) {
                nearestDistanceSq = distanceSq;
                *outSaveState = saveState;
            }
        }

        row = row->next;
    }

    return nearestDistanceSq;
}
} // namespace GameNet

/**
 * Purpose: Clears the locally cached HUD timer tail flags.
 */
void HudTimerPanelNetState::ClearTailFlagsLocal() {
    for (int index = 0; index < 8; ++index) {
        tailFlags[index] = 0;
    }
}

/**
 * Purpose: Applies the selected player color to the row's modal display.
 */
void GameNetPlayerRow::ApplyPlayerColorTint() {
    PlayerModalState *primaryModalState = saveState->primaryModalState;
    const unsigned int packedColor = g_GameNetPlayerRowStyleColors_00RRGGBB[playerColorIndex];
    zColorRgb color = {
        (float)(packedColor & 0xff),
        (float)((packedColor >> 8) & 0xff),
        (float)((packedColor >> 16) & 0xff),
    };
    zClass_Object3D::gwObject3DSetColorAlpha(
        primaryModalState->modalNode,
        &color,
        0.2f
    );
    zClass_Object3D::gwObject3DSetVisibleFlag(
        primaryModalState->modalNode,
        1
    );
}

namespace zDEClient_Crater {
/**
 * Purpose: Normalizes a crater event and relays locally owned events to the network.
 */
int __fastcall Execute(
    zDEClient_CraterEventTemplate *eventTemplate
) {
    if (eventTemplate->radius <= 0.0f) {
        eventTemplate->radius = -eventTemplate->radius;
        return 1;
    }
    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(g_GameStateOrMapTable);
    if (eventTemplate->damageOwnerNode != saveState->playerState->rootNode) {
        return 0;
    }
    g_NetPkt0F_CraterEventRelayBuf.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    g_NetPkt0F_CraterEventRelayBuf.craterTypeId =
        zModel_MatlSlot::IndexFromPtrOrMinus1(eventTemplate->craterMaterialSlot);
    g_NetPkt0F_CraterEventRelayBuf.center = eventTemplate->center;
    g_NetPkt0F_CraterEventRelayBuf.radius = eventTemplate->radius;
    if (zNetwork::IsHost() != 0) {
        NetRelayCallback(
            zNetwork_GetLocalPlayerKey(),
            &g_NetPkt0F_CraterEventRelayBuf
        );
        return 0;
    }
    zNetwork_SendPacketReliable(&g_NetPkt0F_CraterEventRelayBuf.header);
    return 0;
}

/**
 * Purpose: Reconstructs an incoming crater event and relays it when hosting.
 */
int __fastcall NetRelayCallback(
    int,
    NetPkt0F_CraterEvent *packet
) {
    zDEClient_CraterEventTemplate eventTemplate;
    InitEventTemplateDefaults(&eventTemplate);
    if (zNetwork::IsHost() != 0) {
        eventTemplate.craterMaterialSlot = zModel_Matl::GetPoolEntry(packet->craterTypeId);
        eventTemplate.center = packet->center;
        eventTemplate.radius = -packet->radius;
        if (InstanceEventMaybeRelay(&eventTemplate) == 0) {
            packet->header.payloadDword0 = zNetwork_GetLocalPlayerKey();
            packet->eventFlags |= 0x80u;
            zNetwork_SendPacketReliable(&packet->header);
        }
        return 1;
    }
    if ((packet->eventFlags & 0x80u) != 0) {
        eventTemplate.craterMaterialSlot = zModel_Matl::GetPoolEntry(packet->craterTypeId);
        eventTemplate.center = packet->center;
        eventTemplate.radius = -packet->radius;
        InstanceEventMaybeRelay(&eventTemplate);
    }
    return 1;
}
} // namespace zDEClient_Crater

namespace GameNet {
/**
 * Purpose: Relay a host-authored crater feature event to network peers.
 */
int __fastcall HostSendPkt0F_CraterFeature(
    zDEClient_CraterEventTemplate *eventTemplate
) {
    if (zNetwork::IsHost() == 0) {
        return 0;
    }

    g_NetPkt0F_CraterEventSendBuf.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    g_NetPkt0F_CraterEventSendBuf.craterTypeId =
        zModel_MatlSlot::IndexFromPtrOrMinus1(eventTemplate->craterMaterialSlot);
    g_NetPkt0F_CraterEventSendBuf.center = eventTemplate->center;
    g_NetPkt0F_CraterEventSendBuf.eventFlags |= 0x80u;
    g_NetPkt0F_CraterEventSendBuf.radius = eventTemplate->radius;
    zNetwork_SendPacketReliable(&g_NetPkt0F_CraterEventSendBuf.header);
    return 1;
}

/**
 * Purpose: relay local quicksand feature events through packet 0x10 after
 * validating local damage ownership.
 */
int __fastcall SendPkt10_QSandEvent(
    zDEClient_QSandEventTemplate *eventTemplate
) {
    if (eventTemplate->radius <= 0.0f) {
        eventTemplate->radius = -eventTemplate->radius;
        return 1;
    }

    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(g_GameStateOrMapTable);
    if (eventTemplate->damageOwnerNode != saveState->playerState->rootNode) {
        return 0;
    }

    ::g_NetPkt10_QSandEventRelayBuf.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    ::g_NetPkt10_QSandEventRelayBuf.center = eventTemplate->center;
    ::g_NetPkt10_QSandEventRelayBuf.radius = eventTemplate->radius;
    ::g_NetPkt10_QSandEventRelayBuf.eventFlags &= 0xffff0000u;

    if (zNetwork::IsHost() != 0) {
        zDEClient_QSand::NetRelayCallback(
            zNetwork_GetLocalPlayerKey(),
            &::g_NetPkt10_QSandEventRelayBuf
        );
        return 0;
    }

    zNetwork_SendPacketReliable(&::g_NetPkt10_QSandEventRelayBuf.header);
    return 0;
}
} // namespace GameNet

namespace zDEClient_QSand {
/**
 * Purpose: Reconstructs an incoming quicksand event and relays it when hosting.
 */
int __fastcall NetRelayCallback(
    int,
    NetPkt10_QSandEvent *packet
) {
    zDEClient_QSandEventTemplate eventTemplate;
    zDEClient::CopyQSandEventTemplateDefaults(&eventTemplate);
    if (zNetwork::IsHost() != 0) {
        eventTemplate.center = packet->center;
        eventTemplate.radius = -packet->radius;
        if (InstanceEventMaybeRelay(&eventTemplate) == 0) {
            packet->header.payloadDword0 = zNetwork_GetLocalPlayerKey();
            packet->eventFlags |= 0x80u;
            zNetwork_SendPacketReliable(&packet->header);
        }
        return 1;
    }
    if ((packet->eventFlags & 0x80u) != 0) {
        eventTemplate.center = packet->center;
        eventTemplate.radius = -packet->radius;
        InstanceEventMaybeRelay(&eventTemplate);
    }
    return 1;
}
} // namespace zDEClient_QSand

namespace GameNet {
/**
 * Purpose: Relay a host-authored quicksand feature event to network peers.
 */
int __fastcall HostSendPkt10_QSandFeature(
    zDEClient_QSandEventTemplate *eventTemplate
) {
    if (zNetwork::IsHost() == 0) {
        return 0;
    }

    g_NetPkt10_QSandEventSendBuf.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    g_NetPkt10_QSandEventSendBuf.center = eventTemplate->center;
    g_NetPkt10_QSandEventSendBuf.eventFlags |= 0x80u;
    g_NetPkt10_QSandEventSendBuf.radius = eventTemplate->radius;
    zNetwork_SendPacketReliable(&g_NetPkt10_QSandEventSendBuf.header);
    return 1;
}
} // namespace GameNet

namespace Pickup {
/**
 * Purpose: Sends the flag-2 state update for a pickup spawn.
 */
int __fastcall SendPkt11_Flag2Delta(
    PickupSpawnDef *spawn
) {
    g_PickupPkt11Flag2Delta.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    g_PickupPkt11Flag2Delta.flags = 2;
    g_PickupPkt11Flag2Delta.pickupId = spawn->pickupId;
    return zNetwork_SendPacketReliable(&g_PickupPkt11Flag2Delta.header);
}

/**
 * Purpose: Sends the flag-8 state update for a pickup spawn.
 */
int __fastcall SendPkt11_Flag8Delta(
    PickupSpawnDef *spawn
) {
    g_PickupPkt11Flag8Delta.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    g_PickupPkt11Flag8Delta.flags = 8;
    g_PickupPkt11Flag8Delta.pickupId = spawn->pickupId;
    return zNetwork_SendPacketReliable(&g_PickupPkt11Flag8Delta.header);
}

/**
 * Purpose: Builds and sends the network create-state packet for a pickup spawn.
 */
void __fastcall SendPkt11_CreateDelta(
    PickupSpawnDef *spawn
) {
    PickupPkt11CreateDelta *const packet =
        (PickupPkt11CreateDelta *)(malloc(sizeof(PickupPkt11CreateDelta)));
    memset(
        packet,
        0,
        sizeof(PickupPkt11CreateDelta)
    );
    packet->header.packetType = 0x11;
    packet->header.packetSizeBytes = sizeof(PickupPkt11CreateDelta);
    packet->header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    packet->flags = 1;
    packet->pickupId = spawn->pickupId;
    packet->typeKeyIndex =
        (unsigned short)(PickupTypeKeyTable::FindIndex(spawn->pickupType->logicalName));
    packet->amount = spawn->amount;
    packet->position = spawn->position;
    packet->rotation = spawn->rotation;
    packet->respawnDelay = spawn->respawnDelay;
    zNetwork_SendPacketReliable(&packet->header);
    free(packet);
}

/**
 * Purpose: Applies an incoming pickup creation or state-change packet.
 */
int __fastcall HandlePkt11_SpawnDelta(
    int,
    PickupPkt11CreateDelta *packet
) {
    PickupSpawnDef *const spawn = FindSpawnByPickupId(
        packet->pickupId,
        &g_PickupSpawnList_Primary
    );
    const unsigned int flags = packet->flags;
    if ((flags & 1u) != 0 && spawn == 0) {
        PickupParsedZrdEntry entry;
        memset(
            &entry,
            0,
            sizeof(entry)
        );
        entry.typeDesc = PickupType::GetByIndex((int)(packet->typeKeyIndex));
        entry.amount = packet->amount;
        entry.position = packet->position;
        entry.rotation = packet->rotation;
        entry.respawnDelay = packet->respawnDelay;
        PickupSpawnDef *const newSpawn = SpawnFromParsedZrdEntry(&entry);
        if (newSpawn != 0) {
            newSpawn->pickupId = packet->pickupId;
        }
        SetNextPickupId(packet->pickupId + 1);
        return 1;
    }
    if (spawn == 0 || spawn->refCount != 0) {
        return 1;
    }
    if ((flags & 2u) != 0) {
        PickupSpawnList::RemoveAndFreeNode(
            spawn,
            &g_PickupSpawnList_Primary
        );
        return 1;
    }
    if ((flags & 8u) != 0) {
        zClass_NodePartial *const pickupObj = spawn->pickupObj;
        zClass_Node::ClearPickupFlagsRecursive(pickupObj);
        zClass_Class::gwNodeSetRaycastable(
            pickupObj,
            0
        );
        zClass_Class::gwNodeSetPickable(
            pickupObj,
            0
        );
        RemoveObject(
            0,
            pickupObj,
            0
        );
    }
    return 1;
}

/**
 * Purpose: Sends an airdrop pickup spawn and chute update to network peers.
 */
void __fastcall SendPkt12_AirdropSpawnChuteRelay(
    int pickupTypeIndex,
    zVec3 *spawnPos,
    int nextPickupId
) {
    g_PickupPkt12AirdropSpawnChuteRelay.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    g_PickupPkt12AirdropSpawnChuteRelay.spawnPos = *spawnPos;
    g_PickupPkt12AirdropSpawnChuteRelay.pickupTypeIndex = (unsigned short)(pickupTypeIndex);
    g_PickupPkt12AirdropSpawnChuteRelay.nextPickupId = nextPickupId;
    zNetwork_SendPacketReliable(&g_PickupPkt12AirdropSpawnChuteRelay.header);
}

/**
 * Purpose: Applies an incoming airdrop pickup spawn and next-id update.
 */
int __fastcall HandlePkt12_AirdropSpawnChuteRelay(
    int,
    PickupPkt12AirdropSpawnChuteRelay *packet
) {
    SetNextPickupId(packet->nextPickupId);
    SpawnWithAirdropChute(
        (int)(packet->pickupTypeIndex),
        &packet->spawnPos
    );
    return 1;
}
} // namespace Pickup

namespace OptCatalog {
/**
 * Purpose: Determines whether an alternate-gun catalog entry may allocate runtime state.
 */
int __fastcall AltGunDispatchAllocRuntimeGateCallback(
    OptCatalogEntryDef *self,
    void **saveStateSlot
) {
    const int ordinalIndex = self->ordinalIndex;
    if (ordinalIndex == 0 || ordinalIndex == 1) {
        return 1;
    }
    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(*saveStateSlot);
    if (saveState == 0) {
        return 0;
    }
    if (saveState == (zUtil_SaveGameState *)(g_GameStateOrMapTable)) {
        *saveStateSlot = (void *)(zVideo::ReturnSuccessStub());
        GameNet::SendPkt07_AltGunDispatch(
            (short)(ordinalIndex),
            (unsigned int)(*saveStateSlot)
        );
        *saveStateSlot = (void *)((unsigned int)(*saveStateSlot) | 0x01000000u);
        return 1;
    }
    const unsigned int dispatchFlags =
        (unsigned int)(saveState->playerState->altGunDispatchFlags);
    if ((dispatchFlags & 0x02000000u) == 0) {
        return 0;
    }
    *saveStateSlot = (void *)(dispatchFlags);
    return 1;
}
} // namespace OptCatalog

namespace GameNet {
/**
 * Purpose: send the local alternate-gun dispatch packet to peers.
 */
void __fastcall SendPkt07_AltGunDispatch(
    short weaponId,
    unsigned int dispatchFlags
) {
    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(g_GameStateOrMapTable);
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    g_NetPkt07_AltGunDispatchBuf.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    g_NetPkt07_AltGunDispatchBuf.weaponId = weaponId;
    g_NetPkt07_AltGunDispatchBuf.dispatchFlags = dispatchFlags;
    g_NetPkt07_AltGunDispatchBuf.targetPos = playerState->storedTargetPos;
    zNetwork_SendPacketReliable(&g_NetPkt07_AltGunDispatchBuf.header);
}

/**
 * Purpose: apply a remote pkt07 alternate-gun dispatch to the matching player
 * row.
 */
int __fastcall HandlePkt07_AltGunDispatch(
    int,
    NetPkt07_AltGunDispatch *packet
) {
    GameNetPlayerRow *const row = FindPlayerRowByKey(packet->header.payloadDword0);
    if (row == 0) {
        return 0;
    }

    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)row->saveState;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    playerState->altGunDispatchFlags =
        (int)(packet->dispatchFlags | ::kGameNetRemoteAltGunDispatchFlag);
    playerState->storedTargetPos = packet->targetPos;

    PlayerGunFireController *const oldActiveAltGunController = playerState->activeAltGunController;
    playerState->activeAltGunController =
        Player::FindAltGunFireControllerForWeaponId(
            saveState,
            (int)(packet->weaponId)
        );

    OptCatalog::SetPendingSpawnTargetOverrides(
        &playerState->progressTargetCount,
        playerState->progressTargetSlots
    );
    Player::ProcessAltGunDispatchRequest(saveState);

    playerState->altGunDispatchFlags = 0;
    playerState->activeAltGunController = oldActiveAltGunController;
    OptCatalog::SetPendingSpawnTargetOverrides(
        0,
        0
    );
    return 1;
}

/**
 * Purpose: accept remote alternate-gun runtime allocation without local side
 * effects.
 */
int __fastcall AltGunDispatchNoOpCallback(
    OptCatalogEntryDef *,
    void **
) {
    return 1;
}
} // namespace GameNet

namespace OptCatalog {
/**
 * Purpose: Sends a network relay describing removal of a runtime catalog object.
 */
void __fastcall SendPkt0A_RemoveRuntimeRelay(
    OptCatalogEntryDef *self,
    zVec3 *pointOrVec3,
    zClass_NodePartial *ownerNode
) {
    if (g_OptCatalogProcessRuntimeRelayEnabled == 0 || ownerNode == 0) {
        return;
    }
    HudUiMgrSensorTrackNode *const ownerTrackContext =
        (HudUiMgrSensorTrackNode *)(ownerNode->callbackContext);
    if (ownerTrackContext == 0) {
        return;
    }
    zUtil_SaveGameState *const ownerSaveState =
        (zUtil_SaveGameState *)(ownerTrackContext->payload);
    g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.header.payloadDword0 =
        zNetwork_GetLocalPlayerKey();
    g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.optCatalogEntryId =
        (short)(self->ordinalIndex);
    if (pointOrVec3 != 0) {
        g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.pointOrVec3 = *pointOrVec3;
    } else {
        g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.pointOrVec3.x = 0.0f;
        g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.pointOrVec3.y = 0.0f;
        g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.pointOrVec3.z = 0.0f;
    }
    g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.ownerPlayerKey =
        ownerSaveState->netPlayerRow->playerKey;
    zNetwork_SendPacketReliable(&g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.header);
}

/**
 * Purpose: Resolves and applies an incoming runtime-object removal relay.
 */
int __fastcall HandlePkt0A_RemoveRuntimeRelay(
    int,
    NetPkt0A_RemoveRuntimeRelay *packet
) {
    OptCatalogEntryDef *const entry =
        OptCatalog::FindEntryById((int)(packet->optCatalogEntryId));
    zVec3 relayPointScratch;
    zVec3 *pointOrVec3 = &relayPointScratch;
    if (packet->pointOrVec3.x == 0.0f && packet->pointOrVec3.y == 0.0f &&
        packet->pointOrVec3.z == 0.0f) {
        pointOrVec3 = 0;
    }
    GameNetPlayerRow *const row = GameNet::FindPlayerRowByKey(packet->ownerPlayerKey);
    if (row == 0) {
        return 0;
    }
    zUtil_SaveGameState *const ownerSaveState = (zUtil_SaveGameState *)row->saveState;
    if (entry != 0 && ownerSaveState != 0) {
        g_OptCatalogProcessRuntimeRelayEnabled = 0;
        OptCatalog::RemoveRuntimeInstance(
            entry,
            pointOrVec3,
            ownerSaveState->playerState->rootNode
        );
        g_OptCatalogProcessRuntimeRelayEnabled = 1;
    }
    return 1;
}
} // namespace OptCatalog

namespace GameNet {
/**
 * Purpose: Send a reliable pkt13 effect-animation activation record unless
 * replay echo suppression is active.
 */
void __fastcall SendPkt13_EffectAnimActivationRecord(
    zEffectAnimActivationRecord *record
) {
    if (g_GameNetSuppressPkt13ActivationEcho != 0) {
        return;
    }

    const int packedRecordSize = zEffect_Anim::GetActivationRecordPackedSize(record);
    const int packetSize = (int)(sizeof(zNetworkPacketHeader)) + packedRecordSize;
    zNetworkPacketHeader *const packet = (zNetworkPacketHeader *)(malloc((size_t)(packetSize)));
    memset(
        packet,
        0,
        (size_t)(packetSize)
    );

    packet->packetType = 0x13;
    packet->packetSizeBytes = (short)(packetSize);
    packet->payloadDword0 = zNetwork_GetLocalPlayerKey();
    memcpy(
        ((unsigned char *)(packet)) + sizeof(zNetworkPacketHeader),
        record,
        (size_t)(packedRecordSize)
    );

    zNetwork_SendPacketReliable(packet);
    free(packet);
}

/**
 * Purpose: Apply a new remote effect-animation activation record while
 * suppressing replay echo.
 */
int __fastcall HandlePkt13_EffectAnimActivationRecord(
    int,
    zNetworkPacketHeader *packet
) {
    zEffectAnimActivationRecord *const record =
        (zEffectAnimActivationRecord *)((unsigned char *)(packet) + sizeof(zNetworkPacketHeader));
    if (zEffect_Anim::HasActivationRecord(record) == 0) {
        g_GameNetSuppressPkt13ActivationEcho = 1;
        zEffect_Anim::ProcessActivationRecord(record);
        g_GameNetSuppressPkt13ActivationEcho = 0;
    }

    return 1;
}

/**
 * Purpose: Broadcast every queued effect-animation activation record from the
 * host.
 */
void __cdecl SendAllPkt13_EffectAnimActivationRecords() {
    if (zNetwork::IsHost() == 0) {
        return;
    }

    const int recordCount = zEffect_Anim::GetActivationRecordCount();
    for (int index = 0; index < recordCount; ++index) {
        SendPkt13_EffectAnimActivationRecord(zEffect_Anim::GetActivationRecordAt(index));
    }
}

/**
 * Purpose: Send the reliable packet that synchronizes HUD timer and status flags.
 */
int __fastcall SendPkt14_HudTimerAndFlagsSync(
    int eventCode,
    unsigned int statusFlags,
    int valueOrTime,
    int auxParam
) {
    g_NetPkt14_HudTimerAndFlagsSyncBuf.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    g_NetPkt14_HudTimerAndFlagsSyncBuf.valueOrTime = valueOrTime;
    g_NetPkt14_HudTimerAndFlagsSyncBuf.eventCode = (short)(eventCode);
    g_NetPkt14_HudTimerAndFlagsSyncBuf.auxParam = (short)(auxParam);
    g_NetPkt14_HudTimerAndFlagsSyncBuf.statusFlags = statusFlags;
    return zNetwork_SendPacketReliable(&g_NetPkt14_HudTimerAndFlagsSyncBuf.header);
}

/**
 * Purpose: Receive the HUD timer/status sync packet and start the matching mission state.
 */
int __fastcall HandlePkt14_HudTimerAndFlagsSync(
    int senderPlayerId,
    NetPkt14_HudTimerAndFlagsSync *packet
) {
    (void)senderPlayerId;

    UnregisterGameplayPacketHandlers();
    ResetRemotePlayersAndSpawnLists();

    union TimerSecondsBits {
        float seconds;
        int raw;
    } timerSeconds = {(float)(packet->valueOrTime) * 60.0f};
    g_HudSensorTracker.SetRuntimeTimerSecAndGoalValue(
        timerSeconds.raw,
        packet->auxParam
    );

    CZRecoilFrame *const mainWnd = (CZRecoilFrame *)((unsigned int)(g_RecoilApp.GetMainWnd()));
    g_HudSensorTracker.InitMissionIdAndFlags(
        packet->eventCode + 6,
        mainWnd->m_useArchiveBanks
    );
    SetStatusBitsFromFlags(packet->statusFlags);

    g_RecoilApp.m_missionFmvState.m_missionId = 0;
    g_RecoilApp.QueueSwitchCurrentState(
        &g_RecoilApp.m_introFmvState,
        0
    );

    if (zNetwork::IsHost() != 0) {
        HostUpdateSessionDescStatusFields(
            packet->eventCode,
            packet->auxParam,
            packet->valueOrTime,
            packet->statusFlags
        );
    }

    return 1;
}

/**
 * Purpose: Let the host mirror timer and status fields into the session descriptor.
 */
int __fastcall HostUpdateSessionDescStatusFields(
    int eventCode,
    int auxParam,
    int valueOrTime,
    int statusFlags
) {
    if (zNetwork::IsHost() == 0) {
        return 0;
    }

    zNetworkSessionDescStatusFields statusFields;
    if (zNetwork_ExtractStatusFieldsFromSessionDesc(&statusFields) == 0) {
        return 0;
    }

    statusFields.valueOrTime = valueOrTime;
    statusFields.eventCode = eventCode;
    statusFields.statusFlags = statusFlags;
    statusFields.auxParam = auxParam;
    return zNetwork_ApplyStatusFieldsToSessionDesc(&statusFields);
}
} // namespace GameNet

namespace GameNetPlayerRowList {
/**
 * Purpose: Allocate a scoreboard player row and append it to the supplied
 * GameNet player-row list header.
 */
GameNetPlayerRow *__fastcall AppendNewRow(
    GameNetPlayerRowListState *self,
    int zeroInitializeRow
) {
    GameNetPlayerRow *const row = (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    row->hudWidget.ConstructorDefault(
        0,
        0,
        0
    );
    if (zeroInitializeRow != 0) {
        memset(
            row,
            0,
            sizeof(GameNetPlayerRow)
        );
    }

    row->next = 0;
    if (self->count == 0) {
        self->head = row;
    } else {
        self->tail->next = row;
    }

    self->tail = row;
    row->next = 0;
    ++self->count;
    return row;
}
} // namespace GameNetPlayerRowList

/**
 * Purpose: Destroys the player row's embedded HUD panel.
 */
void GameNetPlayerRow::DestroyEmbeddedPanel() {
    hudWidget.~HudUiPanel();
}

namespace GameNet {












































} // namespace GameNet

#if defined(_MSC_VER) && defined(_M_IX86)
typedef void (__cdecl *GameNetCrtInitializerFn)();
/* VC5 emits these GameNet.cpp startup callbacks as direct .CRT$XCU rows. */
#pragma data_seg(".CRT$XCU")
GameNetCrtInitializerFn s_GameNetCrtInit_RegisterMultiplayerMaps =
    Mission::RegisterMultiplayerMaps;
GameNetCrtInitializerFn s_GameNetCrtInit_SpawnPointListInitGlobals =
    GameNetSpawnPointList::InitGlobals;
GameNetCrtInitializerFn s_GameNetCrtInit_PlayerRowListReset =
    GameNetPlayerRowList::Reset;
#pragma data_seg()
#endif

#include "Battlesport/recoil_app.h"

#include "Battlesport/briefing.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/hud_ui_net_exit_panel.h"
#include "Battlesport/recoil_version.h"
#include "Battlesport/hud.h"
#include "Battlesport/player.h"
#include "Battlesport/recoil_state_main_menu_transition.h"
#include "GameZRecoil/Time/time.h"
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
#include "Battlesport/turret.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zUtil/zbd.h"
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
#if defined(RECOILAPP_VC5_STL_STATE_QUEUE_MEMBER) && defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86)
#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
#endif
#include <deque>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Provider-boundary accessor for imported MFC42 CWinApp protected members; this does not
 * reimplement CWinApp behavior.
 */
class RecoilMfcWinAppAccess : public CWinApp {
  public:
    static const AFX_MSGMAP *__stdcall GetMessageMapForRecoilApp();
};

/**
 * Provider-boundary 0x442890: RecoilMfcWinAppAccess::GetMessageMapForRecoilApp.
 * MFC provider-boundary accessor for imported CWinApp message-map metadata.
 * Purpose: exposes CWinApp::messageMap through the callback shape expected by
 * the RecoilApp module message map.
 */
const AFX_MSGMAP *__stdcall RecoilMfcWinAppAccess::GetMessageMapForRecoilApp() {
    return &CWinApp::messageMap;
}

/**
 * Provider-boundary 0x4428a0: RecoilApp_MfcOleModule::GetMessageMap.
 * Purpose: returns the app-module MFC message map used as RecoilApp's base map.
 */
const AFX_MSGMAP * RecoilApp_MfcOleModule::GetMessageMap() const {
    return &RecoilApp_MfcOleModule::messageMap;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-mfcolemodule-messageentries
 * @recoil-artifact defines .rdata recoil:data:0x4d2008: RecoilApp_MfcOleModule::messageEntries.
 *
 * Purpose: provide the terminal MFC message-map sentinel for the app-module base.
 */
AFX_MSGMAP_ENTRY const RecoilApp_MfcOleModule::messageEntries[] = {
    {0, 0, 0, 0, 0, 0},
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
    {0, 0, 0, 0, 0, 0},
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

AFX_MODULE_STATE *__stdcall AfxGetModuleState();
BOOL __stdcall AfxRegisterClass(WNDCLASSA *wndClass);
HINSTANCE __stdcall AfxFindResourceHandle(
    LPCSTR resourceName,
    LPCSTR resourceType
);

extern "C" char g_HudSensorTracker_ObjectivesZrdPath[0x0e];
extern "C" const char g_HudLoading_StopAllSoundsMsg[0x10];

namespace {
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.k-savegamenameallowedchars
 * @recoil-artifact defines .rdata recoil:data:0x4d1598: k_SaveGameNameAllowedChars.
 * Purpose: save-game name raw-key allowlist consumed by HudUiSaveLoadGameNameInput::OnRawKeyboardEvent.
 */
const char k_SaveGameNameAllowedChars[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIKJKLMNOPQRSTUVWXYZ0123456789_ \x1b\r\x08\x7f\x02\x06";
RECOIL_STATIC_ASSERT(sizeof(k_SaveGameNameAllowedChars) == 0x48);

/**
 * Original helper: source-local with no standalone retail function address.
 * Purpose: casts an option payload pointer to the view-rectangle section type.
 */
inline zOpt_ViewRectSection *ViewRectFromPtr(
    void *ptr
) {
    return (zOpt_ViewRectSection *)ptr;
}

/**
 * Evidence: this source-local Win32 resource conversion emits no standalone retail function.
 * Purpose: forms a Win32 integer resource pointer from a numeric identifier.
 */
inline LPCSTR IntResource(
    unsigned int value
) {
    return (LPCSTR)(value);
}

/**
 * Original inline helper with no standalone retail function address.
 * Purpose: returns the nullable save/load entry count for dialog navigation.
 */
inline int SaveLoadEntryCount(
    const HudUiSaveLoadDialog *dialog
) {
    return dialog->fileEntries.begin != 0
               ? (int)(dialog->fileEntries.end - dialog->fileEntries.begin)
               : 0;
}

} // namespace

/**
 *
 * Purpose: stores the process-wide Recoil application object and embedded states.
 * The explicit aligned storage preserves the original global symbol while the
 * CRT row below constructs and destroys the typed app object without VC5
 * emitting automatic global-constructor thunks for this definition.
 */
#undef g_RecoilApp
RecoilAppStorage g_RecoilApp = {0};
#define g_RecoilApp \
    (*(RecoilApp *)&g_RecoilApp)
/**
 *
 * Purpose: stores the zero-initialized singleton save/load app-state
 * transition object; retail evidence models this as the complete 0x1c-byte
 * owner data object for RecoilStateSaveLoadTransition. Explicit storage keeps
 * the original symbol and leaves construction to the recovered lifecycle
 * helpers instead of compiler-generated automatic startup thunks.
 */
#undef g_RecoilStateSaveLoadTransition
RecoilStateSaveLoadTransitionStorage g_RecoilStateSaveLoadTransition = {0};
#define g_RecoilStateSaveLoadTransition \
    (*(RecoilStateSaveLoadTransition *)&g_RecoilStateSaveLoadTransition)

/**
 * Original static-lifetime helper with no standalone authored retail symbol.
 * Purpose: destroy the explicitly stored process-wide Recoil application object
 * from the CRT at-exit list.
 */
static inline void __cdecl RecoilApp_AtExitDestructor() {
    g_RecoilApp.~RecoilApp();
}

/**
 * Evidence: this process-lifetime construction helper has no standalone authored retail symbol.
 * Purpose: construct the explicitly stored process-wide Recoil application
 * object and register its at-exit destructor without typed global storage.
 */
static inline void __cdecl RecoilApp_StaticInitAndRegisterAtExit() {
    new (&g_RecoilApp) RecoilApp;
    atexit(RecoilApp_AtExitDestructor);
}

#if defined(_MSC_VER) && defined(_M_IX86)
typedef void (__cdecl *RecoilAppCrtInitializerFn)();
typedef void (__cdecl *RecoilStateSaveLoadTransitionCrtInitializerFn)();
#pragma data_seg(".CRT$XCU")
static RecoilAppCrtInitializerFn s_RecoilAppCrtInit =
    RecoilApp_StaticInitAndRegisterAtExit;
static RecoilStateSaveLoadTransitionCrtInitializerFn s_RecoilStateSaveLoadTransitionCrtInit =
    RecoilStateSaveLoadTransition::StaticInitAndRegisterAtExit;
#pragma data_seg()
#endif

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
const char g_RecoilApp_SourceFile_RecoilAppCpp[0x22] =
    "D:\\Proj\\Battlesport\\RecoilApp.cpp";
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
 *
 * Purpose: names the common sound sample set loaded at play-state startup.
 */
extern "C" const char g_RecoilApp_CommonSoundsSampleSetName[0x06] = {
    'C', 'O', 'M', 'M', 'O', 'N'
};
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_CommonSoundsSampleSetName) == 0x06);
/**
 *
 * Purpose: labels the loading checkpoint logged before the common sound set
 * is initialized.
 */
extern "C" const char g_RecoilApp_LoadingCommonSoundsMsg[0x15] = {
    'L', 'o', 'a', 'd', 'i', 'n', 'g', ' ', 'c', 'o', 'm',
    'm', 'o', 'n', ' ', 's', 'o', 'u', 'n', 'd', 's'
};
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_LoadingCommonSoundsMsg) == 0x15);
/**
 *
 * Purpose: names the HUD archive loaded when play state becomes current.
 */
extern "C" const char g_HudUiMgr_HudArchiveName[0x07] = {
    'h', 'u', 'd', '.', 'z', 'r', 'd'
};
RECOIL_STATIC_ASSERT(sizeof(g_HudUiMgr_HudArchiveName) == 0x07);
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
char g_RecoilApp_Run_FatalGeneralErrorMessage[0x29] =
    "Fatal error, please contact tech support";
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
char g_RecoilApp_Run_FileErrorEndOfFileMessage[0x1d] =
    "The end of file was reached.";
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
char g_RecoilApp_Run_FileErrorLockViolationMessage[0x3f] =
    "There was an attempt to lock a region that was already locked.";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FileErrorLockViolationMessage) == 0x3f);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fileerrorsharingviolationmessage
 * @recoil-artifact defines .data recoil:data:0x4dd6cc: g_RecoilApp_Run_FileErrorSharingViolationMessage.
 *
 * Purpose: reports CFileException::sharingViolation in RecoilApp::Run.
 */
char g_RecoilApp_Run_FileErrorSharingViolationMessage[0x39] =
    "SHARE.EXE was not loaded, or a shared region was locked.";
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
char g_RecoilApp_Run_FileErrorBadSeekMessage[0x33] =
    "There was an error trying to set the file pointer.";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FileErrorBadSeekMessage) == 0x33);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fileerrordirectoryfullmessage
 * @recoil-artifact defines .data recoil:data:0x4dd758: g_RecoilApp_Run_FileErrorDirectoryFullMessage.
 *
 * Purpose: reports CFileException::directoryFull in RecoilApp::Run.
 */
char g_RecoilApp_Run_FileErrorDirectoryFullMessage[0x25] =
    "There are no more directory entries.";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FileErrorDirectoryFullMessage) == 0x25);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fileerrorremovecurrentdirmessage
 * @recoil-artifact defines .data recoil:data:0x4dd780: g_RecoilApp_Run_FileErrorRemoveCurrentDirMessage.
 *
 * Purpose: reports CFileException::removeCurrentDir in RecoilApp::Run.
 */
char g_RecoilApp_Run_FileErrorRemoveCurrentDirMessage[0x31] =
    "The current working directory cannot be removed.";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FileErrorRemoveCurrentDirMessage) == 0x31);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fileerrorinvalidfilemessage
 * @recoil-artifact defines .data recoil:data:0x4dd7b4: g_RecoilApp_Run_FileErrorInvalidFileMessage.
 *
 * Purpose: reports CFileException::invalidFile in RecoilApp::Run.
 */
char g_RecoilApp_Run_FileErrorInvalidFileMessage[0x34] =
    "There was an attempt to use an invalid file handle.";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FileErrorInvalidFileMessage) == 0x34);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fileerroraccessdeniedmessage
 * @recoil-artifact defines .data recoil:data:0x4dd7e8: g_RecoilApp_Run_FileErrorAccessDeniedMessage.
 *
 * Purpose: reports CFileException::accessDenied in RecoilApp::Run.
 */
char g_RecoilApp_Run_FileErrorAccessDeniedMessage[0x20] =
    "The file could not be accessed.";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FileErrorAccessDeniedMessage) == 0x20);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fileerrortoomanyopenfilesmessage
 * @recoil-artifact defines .data recoil:data:0x4dd808: g_RecoilApp_Run_FileErrorTooManyOpenFilesMessage.
 *
 * Purpose: reports CFileException::tooManyOpenFiles in RecoilApp::Run.
 */
char g_RecoilApp_Run_FileErrorTooManyOpenFilesMessage[0x31] =
    "The permitted number of open files was exceeded.";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FileErrorTooManyOpenFilesMessage) == 0x31);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fileerrorbadpathmessage
 * @recoil-artifact defines .data recoil:data:0x4dd83c: g_RecoilApp_Run_FileErrorBadPathMessage.
 *
 * Purpose: reports CFileException::badPath in RecoilApp::Run.
 */
char g_RecoilApp_Run_FileErrorBadPathMessage[0x23] =
    "All or part of the path is invalid";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_Run_FileErrorBadPathMessage) == 0x23);
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.g-recoilapp-run-fileerrorfilenotfoundmessage
 * @recoil-artifact defines .data recoil:data:0x4dd860: g_RecoilApp_Run_FileErrorFileNotFoundMessage.
 *
 * Purpose: reports CFileException::fileNotFound in RecoilApp::Run.
 */
char g_RecoilApp_Run_FileErrorFileNotFoundMessage[0x1e] =
    "The file could not be located";
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
char g_RecoilApp_Run_FatalOutOfMemoryMessage[0x3c] =
    "Fatal out-of-memory error, Freeing some disk space may help";
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
const char *g_RecoilApp_WndClassNamePtr = g_RecoilApp_WndClassName;
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
 * Provider-boundary 0x435fd0: VC5 std::vector<HudUiSaveLoadEntry>
 * insert helper emitted for HudUiSaveLoadDialog::fileEntries.
 * Purpose: provide the recovered vector insert instantiation needed by the
 * save/load file-list refresh path and final executable link.
 */
HudUiSaveLoadEntry * HudUiSaveLoadEntries::InsertCopiesAt(
    HudUiSaveLoadEntry *position,
    unsigned int count,
    const HudUiSaveLoadEntry *entry
) {
    const int oldSize = begin != 0 ? (int)(end - begin) : 0;
    const int oldCapacity = begin != 0 ? (int)(capacityEnd - begin) : 0;
    const int insertIndex = begin != 0 ? (int)(position - begin) : 0;
    const int newSize = oldSize + (int)count;

    if (newSize > oldCapacity) {
        int newCapacity = oldSize + ((int)count < oldSize ? oldSize : (int)count);
        if (newCapacity < newSize) {
            newCapacity = newSize;
        }

        HudUiSaveLoadEntry *const newBegin =
            (HudUiSaveLoadEntry *)::operator new(sizeof(HudUiSaveLoadEntry) * newCapacity);
        int i;
        for (i = 0; i < insertIndex; ++i) {
            newBegin[i] = begin[i];
        }
        for (i = 0; i < (int)count; ++i) {
            newBegin[insertIndex + i] = *entry;
        }
        for (i = insertIndex; i < oldSize; ++i) {
            newBegin[(int)count + i] = begin[i];
        }

        ::operator delete(begin);
        begin = newBegin;
        end = newBegin + newSize;
        capacityEnd = newBegin + newCapacity;
        return begin + insertIndex;
    }

    int i;
    for (i = oldSize - 1; i >= insertIndex; --i) {
        begin[i + (int)count] = begin[i];
    }
    for (i = 0; i < (int)count; ++i) {
        begin[insertIndex + i] = *entry;
    }
    end += count;
    return begin + insertIndex;
}



















/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-mfcolemodule-destructor-recoilapp-mfcolemodule
 * @recoil-artifact defines .text recoil:function:0x4428b0: RecoilApp_MfcOleModule::~RecoilApp_MfcOleModule.
 * Purpose: destroys the app state's chunked queue storage before chaining to the MFC base destructor.
 */
RecoilApp_MfcOleModule::~RecoilApp_MfcOleModule() {
#if defined(RECOILAPP_VC5_STL_STATE_QUEUE_MEMBER) && defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86)
    // VC5 emits the retail chunk-drain loop from the recovered deque member destructor.
#else
    if (m_stateQueue.m_chunkBaseList != 0) {
        RecoilApp_StateQueueItem ***slot = m_stateQueue.m_readBlock.m_chunkBaseSlot;
        RecoilApp_StateQueueItem ***const lastSlot = m_stateQueue.m_writeBlock.m_chunkBaseSlot;
        while (slot != 0 && slot <= lastSlot) {
            ::operator delete(*slot);
            ++slot;
        }

        ::operator delete(m_stateQueue.m_chunkBaseList);
        memset(
            &m_stateQueue,
            0,
            sizeof(m_stateQueue)
        );
    }
#endif
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-mfcolemodule-initinstance
 * @recoil-artifact defines .text recoil:function:0x4429d0: RecoilApp_MfcOleModule::InitInstance.
 * Purpose: create, connect, show, and update the primary Recoil frame window.
 */
int RecoilApp_MfcOleModule::InitInstance() {
    RecoilApp *const app = (RecoilApp *)this;

    Enable3dControls();

    m_pMainWnd = (CWnd *)app->CreateMainWnd();
    CZRecoilFrame *const mainWnd = app->GetMainWnd();
    mainWnd->m_app = app;
    m_pMainWnd->ShowWindow(SW_SHOW);
    UpdateWindow(m_pMainWnd->m_hWnd);
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-takeskipwaitmessage
 * @recoil-artifact defines .text recoil:function:0x442a10: RecoilApp::TakeSkipWaitMessage.
 * Purpose: consumes and clears the app-shell skip-wait-message flag.
 */
int RecoilApp::TakeSkipWaitMessage() {
    const int wasSkipped = m_skipWait;
    m_skipWait = 0;
    return wasSkipped;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-markskipwaitmessage
 * @recoil-artifact defines .text recoil:function:0x442a30: RecoilApp::MarkSkipWaitMessage.
 * Purpose: sets the app-shell skip-wait-message flag and returns its prior state.
 */
int RecoilApp::MarkSkipWaitMessage() {
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
inline void PrintEngineInitZeroStatus(
    const char *format,
    int result
) {
    printf(
        format,
        result == 0 ? g_RecoilApp_StartupStatusPassed : g_RecoilApp_StartupStatusFailed
    );
}

/**
 * Evidence: this inline/static status helper has no standalone retail function.
 * Caller evidence: 0x442a50 uses this nonzero-success variant, while 0x42e220
 * shares the same engine-startup status-printing source cluster through the
 * zero-success helper above.
 *
 * Purpose: print a subsystem startup status line for nonzero-valued success APIs.
 */
inline void PrintEngineInitNonzeroStatus(
    const char *format,
    int result
) {
    printf(
        format,
        result != 0 ? g_RecoilApp_StartupStatusPassed : g_RecoilApp_StartupStatusFailed
    );
}

} // namespace

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-engineinit
 * @recoil-artifact defines .text recoil:function:0x442a50: RecoilApp::EngineInit.
 * Purpose: initialize core engine subsystems and print their startup status
 * lines before frame timing and input state are reset.
 */
int RecoilApp::EngineInit(
    HWND hwnd
) {
    zUtil::ZRDR_PreallocNodePool(0);
    zUtil::ZRDR_Init(0);

    PrintEngineInitZeroStatus(
        g_RecoilApp_GModInitStatusFmt,
        zModel_Display_Init()
    );
    PrintEngineInitZeroStatus(
        g_RecoilApp_GClsInitStatusFmt,
        zVideo::ReturnSuccessStub()
    );
    PrintEngineInitZeroStatus(
        g_RecoilApp_ZEffInitStatusFmt,
        zEffect::Init()
    );
    PrintEngineInitZeroStatus(
        g_RecoilApp_ZRndrInitStatusFmt,
        zRndr::InitGlobals()
    );
    PrintEngineInitNonzeroStatus(
        g_RecoilApp_ZSndInitStatusFmt,
        zSnd_PreInitializeRuntimeState((RecoilPtr32)((unsigned int)hwnd))
    );
    PrintEngineInitZeroStatus(
        g_RecoilApp_ZUtlInitStatusFmt,
        zVideo::ReturnSuccessStub()
    );
    PrintEngineInitZeroStatus(
        g_RecoilApp_ZWepInitStatusFmt,
        zWepInit()
    );
    PrintEngineInitZeroStatus(
        g_RecoilApp_ZImgInitStatusFmt,
        zImage_Init(0)
    );

    if (g_zVideo_ActiveRendererPath == 2) {
        zInput::Mouse_SetCooperativeLevelFlags(5);
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
void __fastcall OnMciNotify(
    unsigned int wParam,
    unsigned int lParam
);
}

namespace zDEClient {
int __cdecl ShutdownGlobals();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-shutdownsubsystems
 * @recoil-artifact defines .text recoil:function:0x442bc0: RecoilApp::ShutdownSubsystems.
 * Purpose: tear down input, rendering resources, catalogs, models, sound, and
 * mounted ZRDR state during app engine shutdown.
 */
void RecoilApp::ShutdownSubsystems() {
    zInput::Shutdown();
    zImage::ShutdownSubsystem();
    zUtil_ZRDR_ShutdownWildcardPath();
    zVid::ShutdownFrameScratchBuffers();
    zEffect::ShutdownAll();
    OptCatalog::Shutdown();
    zClass::Shutdown();
    zModel_Display::ShutdownThunk();
    zSndSystem::Shutdown();
    zUtil_ZRDR_Shutdown();
    zUtil_ZRDR_FreeNodePool();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-getmainwnd
 * @recoil-artifact defines .text recoil:function:0x442c00: RecoilApp::GetMainWnd.
 * Purpose: returns the main window pointer as the concrete Recoil frame type.
 */
CZRecoilFrame * RecoilApp::GetMainWnd() const {
    return (CZRecoilFrame *)m_pMainWnd;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-startengineandqueuestartupstate
 * @recoil-artifact defines .text recoil:function:0x442c10: RecoilApp::StartEngineAndQueueStartupState.
 * Purpose: starts gameplay systems and queues the pending startup app state.
 */
int RecoilApp::StartEngineAndQueueStartupState() {
    CZRecoilFrame *const mainWnd = GetMainWnd();

    if (StartEngine(mainWnd->m_hWnd) == 0) {
        ShutdownEngine();
        return ExitInstance();
    }

    m_skipWait = 1;
    m_missionShutdownMode = RECOILAPP_MISSION_SHUTDOWN_ON_EXIT;
    QueueSwitchCurrentState(
        m_pendingState,
        0
    );
    return 1;
}

#if !(defined(RECOILAPP_VC5_STL_STATE_QUEUE_MEMBER) && defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86))
/**
 * Original inline member helper with no standalone retail function address.
 * Purpose: tests whether the recovered state queue has no pending transition items.
 */
inline bool RecoilApp_StateQueue::Empty() const {
    return m_itemCount == 0;
}

/**
 * Evidence: the recovered queue-front accessor is inline and has no standalone retail function.
 * Purpose: returns the pending transition item at the front of the recovered queue.
 */
inline RecoilApp_StateQueueItem *RecoilApp_StateQueue::Front() const {
    return *m_readBlock.m_cursor;
}

/**
 * Evidence: the recovered queue-pop operation is inline and has no standalone retail function.
 * Purpose: removes the pending transition item at the front of the recovered queue.
 */
inline void RecoilApp_StateQueue::PopFront() {
    ++m_readBlock.m_cursor;
    --m_itemCount;

    if (m_itemCount != 0 && m_readBlock.m_cursor == m_readBlock.m_chunkEnd) {
        ++m_readBlock.m_chunkBaseSlot;
        m_readBlock.InitFromCursor(
            *m_readBlock.m_chunkBaseSlot,
            m_readBlock.m_chunkBaseSlot
        );
    }
}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in queue entrypoint callers 0x443160, 0x443310, and 0x4434b0.
 *
 * Purpose: append one pending app-state transition item to the recovered queue.
 */
inline void RecoilApp_StateQueue::PushBack(
    RecoilApp_StateQueueItem *const &item
) {
    if (Empty() || m_writeBlock.m_cursor == m_writeBlock.m_chunkEnd) {
        RecoilApp_StateQueueItem **chunk =
            (RecoilApp_StateQueueItem **)::operator new(
                kRecoilAppStateQueueChunkSlotCount * sizeof(RecoilApp_StateQueueItem *)
            );

        if (Empty()) {
            m_chunkBaseCapacity = kRecoilAppStateQueueInitialChunkBaseCapacity;
            m_chunkBaseList = (RecoilApp_StateQueueItem ***)::operator new(
                kRecoilAppStateQueueInitialChunkBaseCapacity *
                    (int)(sizeof(RecoilApp_StateQueueItem **))
            );
            m_chunkBaseList[kRecoilAppStateQueueInitialChunkBaseCapacity - 1] = chunk;

            RecoilApp_StateQueueItem ***const chunkBaseSlot =
                m_chunkBaseList + kRecoilAppStateQueueInitialChunkBaseCapacity - 1;
            chunk += kRecoilAppStateQueueInitialCursorOffset;
            m_readBlock.m_chunkBegin = *chunkBaseSlot;
            m_readBlock.m_chunkEnd =
                m_readBlock.m_chunkBegin + kRecoilAppStateQueueChunkSlotCount;
            m_readBlock.m_cursor = chunk;
            m_readBlock.m_chunkBaseSlot = chunkBaseSlot;
            m_writeBlock.m_chunkBegin = m_readBlock.m_chunkBegin;
            m_writeBlock.m_chunkEnd = m_readBlock.m_chunkEnd;
            m_writeBlock.m_cursor = chunk;
            m_writeBlock.m_chunkBaseSlot = chunkBaseSlot;
        } else if (m_writeBlock.m_chunkBaseSlot <
                   m_chunkBaseList + m_chunkBaseCapacity - 1) {
            ++m_writeBlock.m_chunkBaseSlot;
            *m_writeBlock.m_chunkBaseSlot = chunk;
            m_writeBlock.m_chunkBegin = *m_writeBlock.m_chunkBaseSlot;
            m_writeBlock.m_chunkEnd =
                m_writeBlock.m_chunkBegin + kRecoilAppStateQueueChunkSlotCount;
            m_writeBlock.m_cursor = chunk;
        } else {
            const int activeChunkCount =
                (int)(m_writeBlock.m_chunkBaseSlot - m_readBlock.m_chunkBaseSlot) + 1;
            RecoilApp_StateQueueItem **const oldReadCursor = m_readBlock.m_cursor;
            RecoilApp_StateQueueItem ***const centeredSlot =
                GrowAndCenterChunkBaseList(activeChunkCount * 2);
            RecoilApp_StateQueueItem ***const newWriteSlot =
                centeredSlot + activeChunkCount;
            *newWriteSlot = chunk;
            m_readBlock.m_chunkBegin = *centeredSlot;
            m_readBlock.m_chunkEnd =
                m_readBlock.m_chunkBegin + kRecoilAppStateQueueChunkSlotCount;
            m_readBlock.m_cursor = oldReadCursor;
            m_readBlock.m_chunkBaseSlot = centeredSlot;
            RecoilApp_StateQueueBlock writeBlock;
            m_writeBlock = *writeBlock.InitFromCursor(
                chunk,
                newWriteSlot
            );
        }
    }

    RecoilApp_StateQueueItem **const slot = m_writeBlock.m_cursor;
    m_writeBlock.m_cursor = slot + 1;
    if (slot != 0) {
        *slot = item;
    }
    ++m_itemCount;
}
#else
/**
 * Original-source inline helper: VC5 owner verification uses the retail STL deque member.
 * Purpose: tests whether the recovered state queue has no pending transition items.
 */
inline bool RecoilApp_StateQueue::Empty() const {
    return empty();
}

/**
 * Evidence: VC5 owner verification uses the retail STL deque member for this inline queue-front accessor.
 * Purpose: returns the pending transition item at the front of the queue.
 */
inline RecoilApp_StateQueueItem *RecoilApp_StateQueue::Front() const {
    return front();
}

/**
 * Evidence: VC5 owner verification uses the retail STL deque member for this inline queue-pop operation.
 * Purpose: removes the pending transition item at the front of the queue.
 */
inline void RecoilApp_StateQueue::PopFront() {
    pop_front();
}

/**
 * Evidence: VC5 owner verification uses the retail STL deque member for this inline queue-append operation.
 * Purpose: appends one pending transition item to the queue.
 */
inline void RecoilApp_StateQueue::PushBack(
    RecoilApp_StateQueueItem *const &item
) {
    push_back(item);
}
#endif

/**
 * Purpose: constructs the MFC app subobject and initializes Recoil-owned state host fields.
 */
#if defined(RECOILAPP_VC5_STL_STATE_QUEUE_MEMBER) && defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86)
RecoilApp_MfcOleModule::RecoilApp_MfcOleModule()
    : CWinApp(0)
#if !defined(_AFXDLL)
      , m_recoilPad(0)
#endif
{
    m_skipWait = 0;
    m_pendingState = 0;
    m_currentStateIndex = -1;
    memset(
        m_stateStack,
        0,
        sizeof(m_stateStack)
    );
}
#else
/**
 * Purpose: constructs the MFC app subobject and initializes Recoil-owned state host fields.
 */
RecoilApp_MfcOleModule::RecoilApp_MfcOleModule()
    : CWinApp(0)
#if !defined(_AFXDLL)
      , m_recoilPad(0)
#endif
      , m_pendingState(0),
      m_currentStateIndex(-1),
      m_stateHostReserved(0),
      m_skipWait(0),
      m_missionShutdownMode(RECOILAPP_MISSION_SHUTDOWN_ON_EXIT),
      m_stateQueue(),
      m_reserved148(0) {
    memset(
        m_stateStack,
        0,
        sizeof(m_stateStack)
    );
}
#endif

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
 * Purpose: runs the app-shell message loop, queued state transitions, and exception dialogs.
 */
int RecoilApp_MfcOleModule::Run() {
    RecoilApp *const app = (RecoilApp *)this;
    int keepRunning = 1;

    try {
        CWinThread::SetThreadPriority(THREAD_PRIORITY_HIGHEST);

        while (keepRunning != 0) {
            while (PeekMessageA(
                &m_msgCur,
                0,
                0,
                0,
                PM_NOREMOVE
            ) != 0) {
                if (PumpMessage() == 0) {
                    keepRunning = 0;
                    break;
                }
            }

            if (keepRunning == 0) {
                break;
            }

            zNetworkDPlay::ReceivePendingMessages(-1);

            RecoilApp_IState *const currentState = app->GetCurrentState();
            if (m_skipWait == 0) {
                if (PeekMessageA(
                    &m_msgCur,
                    0,
                    0,
                    0,
                    PM_NOREMOVE
                ) == 0) {
                    WaitMessage();
                }
                continue;
            }

            if (!m_stateQueue.Empty()) {
                RecoilApp_StateQueueItem *const item = m_stateQueue.Front();
                m_stateQueue.PopFront();

                if (item->m_kind == RecoilApp_StateQueueKind_ExitCurrent) {
                    if (currentState != 0) {
                        currentState->OnDeactivate();
                    }

                    m_stateStack[m_currentStateIndex] = 0;
                    --m_currentStateIndex;
                    if (m_currentStateIndex < 0) {
                        m_currentStateIndex = 0;
                    }

                    if (m_stateStack[m_currentStateIndex] != 0) {
                        m_stateStack[m_currentStateIndex]->OnResume(item->m_param);
                    }
                } else if (item->m_kind == RecoilApp_StateQueueKind_PushState) {
                    if (item->m_stateObj != 0) {
                        if (m_stateStack[m_currentStateIndex] != 0) {
                            m_stateStack[m_currentStateIndex]->OnSuspend(item->m_param);
                        }

                        if (item->m_stateObj->OnTryBecomeCurrent() != 0) {
                            ++m_currentStateIndex;
                            if (m_currentStateIndex >= 16) {
                                m_currentStateIndex = 15;
                            }

                            m_stateStack[m_currentStateIndex] = item->m_stateObj;
                        }
                    }
                } else if (item->m_kind == RecoilApp_StateQueueKind_SwitchCurrent) {
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
                }

                delete item;
                continue;
            }

            if (currentState != 0 && currentState->OnUpdateShouldQuit() != 0) {
                app->OnAppDeactivate();
                PostQuitMessage(0);
            }
        }
    } catch (CMemoryException *memoryException) {
        ::MessageBoxA(
            0,
            g_RecoilApp_Run_FatalOutOfMemoryMessage,
            g_RecoilApp_Run_MemoryErrorTitle,
            MB_OK | MB_ICONSTOP
        );
        memoryException->Delete();
    } catch (CFileException *fileException) {
        const char *message = g_RecoilApp_Run_FileErrorUnknownMessage;
        switch (fileException->m_cause) {
          case CFileException::endOfFile:
            message = g_RecoilApp_Run_FileErrorEndOfFileMessage;
            break;
          case CFileException::diskFull:
            message = g_RecoilApp_Run_FileErrorDiskFullMessage;
            break;
          case CFileException::lockViolation:
            message = g_RecoilApp_Run_FileErrorLockViolationMessage;
            break;
          case CFileException::sharingViolation:
            message = g_RecoilApp_Run_FileErrorSharingViolationMessage;
            break;
          case CFileException::hardIO:
            message = g_RecoilApp_Run_FileErrorHardIoMessage;
            break;
          case CFileException::badSeek:
            message = g_RecoilApp_Run_FileErrorBadSeekMessage;
            break;
          case CFileException::directoryFull:
            message = g_RecoilApp_Run_FileErrorDirectoryFullMessage;
            break;
          case CFileException::removeCurrentDir:
            message = g_RecoilApp_Run_FileErrorRemoveCurrentDirMessage;
            break;
          case CFileException::invalidFile:
            message = g_RecoilApp_Run_FileErrorInvalidFileMessage;
            break;
          case CFileException::accessDenied:
            message = g_RecoilApp_Run_FileErrorAccessDeniedMessage;
            break;
          case CFileException::tooManyOpenFiles:
            message = g_RecoilApp_Run_FileErrorTooManyOpenFilesMessage;
            break;
          case CFileException::badPath:
            message = g_RecoilApp_Run_FileErrorBadPathMessage;
            break;
          case CFileException::fileNotFound:
            message = g_RecoilApp_Run_FileErrorFileNotFoundMessage;
            break;
          default:
            break;
        }
        ::MessageBoxA(
            0,
            message,
            g_RecoilApp_Run_FileErrorTitle,
            MB_OK | MB_ICONSTOP
        );
        fileException->Delete();
    } catch (CException *exception) {
        ::MessageBoxA(
            0,
            g_RecoilApp_Run_FatalGeneralErrorMessage,
            g_RecoilApp_Run_GeneralErrorTitle,
            MB_OK | MB_ICONSTOP
        );
        exception->Delete();
    }

    return ExitInstance();
}

/**
 * Provider boundary MFC message-map helper with no standalone retail function address.
 * Purpose: returns the imported CWinApp base message map for the Recoil app-module
 * message map.
 */
const AFX_MSGMAP *__stdcall RecoilApp_MfcOleModule::GetBaseMessageMapForMfc() {
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
const AFX_MSGMAP *__stdcall RecoilApp::GetBaseMessageMapForMfc() {
    return &RecoilApp_MfcOleModule::messageMap;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-getcurrentstate
 * @recoil-artifact defines .text recoil:function:0x443140: RecoilApp::GetCurrentState.
 * Purpose: returns the active app state when the state-stack index is valid.
 */
RecoilApp_IState * RecoilApp::GetCurrentState() const {
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
 * Purpose: enqueue a switch-current-state request and run the immediate exit/enter callbacks.
 */
RecoilApp_IState * RecoilApp::QueueSwitchCurrentState(
    RecoilApp_IState *state,
    int stateParam
) {
    RecoilApp_IState *const currentState = GetCurrentState();
    RecoilApp_StateQueueItem *item =
        new RecoilApp_StateQueueItem(
            RecoilApp_StateQueueKind_SwitchCurrent,
            state,
            stateParam
        );
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
 * Purpose: enqueue a push-state request and run the pushed state's enter callback.
 */
RecoilApp_IState * RecoilApp::QueuePushState(
    RecoilApp_IState *state,
    int suspendParam
) {
    RecoilApp_IState *const currentState = GetCurrentState();
    RecoilApp_StateQueueItem *item =
        new RecoilApp_StateQueueItem(
            RecoilApp_StateQueueKind_PushState,
            state,
            suspendParam
        );
    m_stateQueue.PushBack(item);

    state->OnEnter();
    return currentState;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-queueexitcurrentstate
 * @recoil-artifact defines .text recoil:function:0x4434b0: RecoilApp::QueueExitCurrentState.
 * Purpose: enqueue an exit-current-state request and run the current state's exit callback.
 */
RecoilApp_IState * RecoilApp::QueueExitCurrentState(
    int stateParam
) {
    RecoilApp_IState *const currentState = GetCurrentState();
    RecoilApp_StateQueueItem *item =
        new RecoilApp_StateQueueItem(
            RecoilApp_StateQueueKind_ExitCurrent,
            0,
            stateParam
        );
    m_stateQueue.PushBack(item);

    if (currentState != 0) {
        currentState->OnExit();
    }

    return currentState;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-onidleordispatch
 * @recoil-artifact defines .text recoil:function:0x443650: RecoilApp::OnIdleOrDispatch.
 * Purpose: handles idle/dispatch notifications for CD sound and the current state.
 */
int RecoilApp::OnIdleOrDispatch(
    unsigned int wParam,
    unsigned int lParam
) {
    RecoilApp_IState *const currentState = GetCurrentState();
    zSndCd::OnMciNotify(
        wParam,
        lParam
    );
    if (currentState == 0) {
        return 0;
    }

    return currentState->OnIdleOrDispatch(
        wParam,
        lParam
    );
}

#if !(defined(RECOILAPP_VC5_STL_STATE_QUEUE_MEMBER) && defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86))
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-statequeue-growandcenterchunkbaselist
 * @recoil-artifact defines .text recoil:function:0x443690: RecoilApp_StateQueue::GrowAndCenterChunkBaseList.
 * Purpose: Grows the chunk-map and recenters the active chunk-slot range in the new map.
 */
RecoilApp_StateQueueItem *** RecoilApp_StateQueue::GrowAndCenterChunkBaseList(
    int newCapacity
) {
    int byteCount = newCapacity * (int)(sizeof(RecoilApp_StateQueueItem **));
    if (byteCount < 0) {
        byteCount = 0;
    }

    RecoilApp_StateQueueItem ***const newList =
        (RecoilApp_StateQueueItem ***)::operator new(byteCount);
    RecoilApp_StateQueueItem ***const centeredSlot =
        newList + (((unsigned int)newCapacity) >> 2);
    RecoilApp_StateQueueItem ***readSlot = m_readBlock.m_chunkBaseSlot;
    RecoilApp_StateQueueItem ***const stopSlot = m_writeBlock.m_chunkBaseSlot + 1;
    RecoilApp_StateQueueItem ***writeSlot = centeredSlot;

    while (readSlot != stopSlot) {
        *writeSlot = *readSlot;
        ++readSlot;
        ++writeSlot;
    }

    ::operator delete(m_chunkBaseList);
    m_chunkBaseList = newList;
    m_chunkBaseCapacity = newCapacity;
    return centeredSlot;
}
#endif

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoilapp-statequeueblock-initfromcursor
 * @recoil-artifact defines .text recoil:function:0x443700: RecoilApp_StateQueueBlock::InitFromCursor.
 * Purpose: Initializes one chunk cursor descriptor from a slot in the queue chunk map.
 */
RecoilApp_StateQueueBlock * RecoilApp_StateQueueBlock::InitFromCursor(
    RecoilApp_StateQueueItem **cursor,
    RecoilApp_StateQueueItem ***chunkBaseSlot
) {
    m_chunkBegin = *chunkBaseSlot;
    m_chunkEnd = *chunkBaseSlot + kRecoilAppStateQueueChunkSlotCount;
    m_cursor = cursor;
    m_chunkBaseSlot = chunkBaseSlot;
    return this;
}

/**
 * Original helper: app-shell with no standalone retail function address.
 * Purpose: marks the message wait loop to skip after app activation.
 */
void RecoilApp::OnAppActivate() {
    MarkSkipWaitMessage();
}

/**
 * Evidence: this app-shell deactivation helper has no standalone retail function.
 * Purpose: clears the skip-wait flag when the app deactivates.
 */
void RecoilApp::OnAppDeactivate() {
    TakeSkipWaitMessage();
}

/**
 * Original helper: default state hook with no standalone retail function address.
 * Source model note: default hooks stay out-of-line; the inline interface
 * destructor in recoil_app.h is current implementation state, not ownership
 * evidence for the unresolved HUD 0x407170/0x4ccd50 table packet.
 * Purpose: accepts window-activation notifications for states that do not override them.
 */
void RecoilApp_IState::OnWndActivate(
    int
) {}

/**
 * Evidence: this default no-op enter hook has no standalone retail function.
 * Purpose: supplies the no-op enter callback for states without enter work.
 */
void RecoilApp_IState::OnEnter() {}

/**
 * Evidence: this default transition-permission hook has no standalone retail function.
 * Purpose: allows a state transition to become current by default.
 */
int RecoilApp_IState::OnTryBecomeCurrent() {
    return 1;
}

/**
 * Evidence: this default quit-query hook has no standalone retail function.
 * Purpose: reports that a default state does not request app shutdown.
 */
int RecoilApp_IState::OnUpdateShouldQuit() {
    return 0;
}

/**
 * Evidence: this default no-op exit hook has no standalone retail function.
 * Purpose: supplies the no-op exit callback for states without exit work.
 */
void RecoilApp_IState::OnExit() {}

/**
 * Evidence: this default no-op deactivate hook has no standalone retail function.
 * Purpose: supplies the no-op deactivate callback for states without deactivate work.
 */
void RecoilApp_IState::OnDeactivate() {}

/**
 * Evidence: this default suspend hook has no standalone retail function.
 * Purpose: accepts suspend notifications for states that do not override them.
 */
void RecoilApp_IState::OnSuspend(
    int
) {}

/**
 * Evidence: this default resume hook has no standalone retail function.
 * Purpose: accepts resume notifications for states that do not override them.
 */
void RecoilApp_IState::OnResume(
    int
) {}

// RecoilApp_AttractFmvState instances use the implicit VC5 destructor and RecoilApp_FmvScript member cleanup.
// RecoilApp_IntroFmvState instances use the implicit VC5 destructor and RecoilApp_FmvScript member cleanup.
// RecoilApp_MissionFmvState instances use the implicit VC5 destructor and RecoilApp_FmvScript member cleanup.

void __fastcall SortEntryRange(
    HudUiSaveLoadEntry *begin,
    HudUiSaveLoadEntry *end,
    int unused
);
void __fastcall InsertEntryIntoSortedPrefix(
    HudUiSaveLoadEntry *entryPosition,
    HudUiSaveLoadEntry entry
);
HudUiSaveLoadEntry *__fastcall PartitionEntriesByPivot(
    HudUiSaveLoadEntry *begin,
    HudUiSaveLoadEntry *end,
    HudUiSaveLoadEntry pivot
);

/**
 * operator<(HudUiSaveLoadEntry const &, HudUiSaveLoadEntry const &).
 * Purpose: Orders save-game file entries by most recent write time.
 */
int __fastcall operator<(
    const HudUiSaveLoadEntry &lhs,
    const HudUiSaveLoadEntry &rhs
) {
    return CompareFileTime(
        &lhs.ftLastWriteTime,
        &rhs.ftLastWriteTime
    ) > 0 ? 1 : 0;
}

/**
 * Purpose: Builds the save-game dialog controls from dialog.zrd and initializes list contents.
 */
HudUiSaveGameDialog::HudUiSaveGameDialog() {
    zReader::Node *const loadedSection = LoadFromZrd(
        "dialog.zrd",
        "SAVE_GAME_DIALOG",
        0
    );
    if (loadedSection != 0) {
        BindWidgetByName(
            loadedSection,
            &backButton,
            "BACK"
        );
        BindWidgetByName(
            loadedSection,
            &nextEntryButton,
            "NEXT_GAME_BTN"
        );
        BindWidgetByName(
            loadedSection,
            &prevEntryButton,
            "PREV_GAME_BTN"
        );
        BindWidgetByName(
            loadedSection,
            &deleteButton,
            "DELETE_BTN"
        );
        BindWidgetByName(
            loadedSection,
            &primaryActionButton,
            "SAVE"
        );
        BindWidgetByName(
            loadedSection,
            &gameNameInput,
            "GAMENAME"
        );

        char listNodeName[32];
        for (int i = 0; i < 9; ++i) {
            sprintf(
                listNodeName,
                "LIST_%d",
                i
            );
            BindPrimitiveNodeToElement(
                loadedSection,
                &entryWidgets[i],
                listNodeName
            );
        }

        FreeLoadedTreeRoots((int)(unsigned int)(loadedSection));
    }

    InitializeFileEntries();
    SetSelectedEntryIndex(-1);
}

/**
 * Purpose: Activates the save-game name input and moves the cursor to the end.
 */
void HudUiSaveLoadGameNameInput::OnActivate() {
    Update(GetBuffer());
    textInput.SetCursorPosition((int)(strlen(GetBuffer())));
    HudUiNumericTextInput::OnActivate();
}

/**
 * Purpose: Filters raw key input to the save-game filename character set.
 */
int HudUiSaveLoadGameNameInput::OnRawKeyboardEvent(
    int key
) {
    if (strchr(
        k_SaveGameNameAllowedChars,
        key
    ) != 0) {
        textInput.DispatchKeyAction(key);
    }

    return 0;
}

/**
 * Purpose: Initializes a save/load list row panel and clears its entry index.
 */
HudUiSaveLoadListItem::HudUiSaveLoadListItem()
    : HudUiPanel(
          0,
          0,
          0
) {
    layoutY = 32767;
    layoutX = -1;
}

/**
 * Purpose: Draws the list row panel and refreshes text bounds after rendering.
 */
void HudUiSaveLoadListItem::Draw() {
    HudUiPanel::Draw();
    UpdateTextBoundsFromContent();
}

/**
 * Purpose: Dispatches the load dialog primary action through the concrete dialog object.
 */
void HudUiLoadGameDialog::OnPrimaryActionThunk() {
    OnPrimaryAction();
}

/**
 * Purpose: Tears down common save/load dialog child widgets, entry storage, and background state.
 */
void HudUiSaveLoadDialog::Destructor() {
    ::operator delete(fileEntries.begin);
    fileEntries.begin = 0;
    fileEntries.end = 0;
    fileEntries.capacityEnd = 0;

    for (int index = 9; index > 0; --index) {
        entryWidgets[index - 1].HudUiPanel::~HudUiPanel();
    }

    gameNameInput.Destructor();
    prevEntryButton.DestructorCore();
    nextEntryButton.DestructorCore();
    backButton.DestructorCore();
    deleteButton.DestructorCore();
    this->HudUiBackground::~HudUiBackground();
}

/**
 * Purpose: Tears down save-game dialog child widgets, entry storage, and background state.
 */
void HudUiSaveGameDialog::Destructor() {
    primaryActionButton.DestructorCore();

    ::operator delete(fileEntries.begin);
    fileEntries.begin = 0;
    fileEntries.end = 0;
    fileEntries.capacityEnd = 0;

    for (int index = 9; index > 0; --index) {
        entryWidgets[index - 1].HudUiPanel::~HudUiPanel();
    }

    gameNameInput.Destructor();
    prevEntryButton.DestructorCore();
    nextEntryButton.DestructorCore();
    backButton.DestructorCore();
    deleteButton.DestructorCore();
    this->HudUiBackground::~HudUiBackground();
}

/**
 * Purpose: Builds the load-game dialog controls from dialog.zrd and initializes list contents.
 */
HudUiLoadGameDialog::HudUiLoadGameDialog() {
    zReader::Node *const loadedSection = LoadFromZrd(
        "dialog.zrd",
        "LOAD_GAME_DIALOG",
        0
    );
    if (loadedSection != 0) {
        BindWidgetByName(
            loadedSection,
            &backButton,
            "BACK"
        );
        BindWidgetByName(
            loadedSection,
            &nextEntryButton,
            "NEXT_GAME_BTN"
        );
        BindWidgetByName(
            loadedSection,
            &prevEntryButton,
            "PREV_GAME_BTN"
        );
        BindWidgetByName(
            loadedSection,
            &deleteButton,
            "DELETE_BTN"
        );
        BindWidgetByName(
            loadedSection,
            &primaryActionButton,
            "LOAD"
        );
        BindWidgetByName(
            loadedSection,
            &gameNameInput,
            "GAMENAME"
        );

        char listNodeName[32];
        for (int i = 0; i < 9; ++i) {
            sprintf(
                listNodeName,
                "LIST_%d",
                i
            );
            BindPrimitiveNodeToElement(
                loadedSection,
                &entryWidgets[i],
                listNodeName
            );
        }

        FreeLoadedTreeRoots((int)(unsigned int)(loadedSection));
    }

    InitializeFileEntries();
    SetSelectedEntryIndex(0);
}

/**
 * Purpose: Uses the common save/load result handler for the load-game dialog.
 */
void HudUiLoadGameDialog::ProcessDialogResult() {
    HudUiSaveLoadDialog::ProcessDialogResult();
}

/**
 * Purpose: Tears down load-game dialog child widgets, entry storage, and background state.
 */
void HudUiLoadGameDialog::Destructor() {
    primaryActionButton.DestructorCore();

    ::operator delete(fileEntries.begin);
    fileEntries.begin = 0;
    fileEntries.end = 0;
    fileEntries.capacityEnd = 0;

    for (int index = 9; index > 0; --index) {
        entryWidgets[index - 1].HudUiPanel::~HudUiPanel();
    }

    gameNameInput.Destructor();
    prevEntryButton.DestructorCore();
    nextEntryButton.DestructorCore();
    backButton.DestructorCore();
    deleteButton.DestructorCore();
    this->HudUiBackground::~HudUiBackground();
}

/**
 * Purpose: Seeds list-row layout metadata, loads saved-game entries, and binds visible rows.
 */
void HudUiSaveLoadDialog::InitializeFileEntries() {
    entryWidgets[0].layoutY = 0x2666;
    entryWidgets[1].layoutY = 0x3fff;
    entryWidgets[2].layoutY = 0x7fff;
    entryWidgets[3].layoutY = 0x7fff;
    entryWidgets[4].layoutY = 0x7fff;
    entryWidgets[5].layoutY = 29490;
    entryWidgets[6].layoutY = 22936;
    entryWidgets[7].layoutY = 0x3fff;
    entryWidgets[8].layoutY = 0x2666;

    RefreshSaveFileList();

    int index = 0;
    HudUiSaveLoadEntry *entry = fileEntries.begin;
    HudUiSaveLoadListItem *listItem = entryWidgets;
    while (entry != fileEntries.end && index < 9) {
        listItem->layoutX = index;
        listItem->SetTextFmt(
            "%s",
            entry->cFileName
        );
        listItem->SetVisible(
            1
        );

        ++entry;
        ++index;
        ++listItem;
    }
}

/**
 * Purpose: Deletes the selected saved-game file and refreshes the dialog list.
 */
void HudUiSaveLoadDialog::DeleteSaveFile(
    int confirmDelete
) {
    char *const gameName = gameNameInput.GetBuffer();
    if (gameName == 0 || gameName[0] == '\0') {
        return;
    }

    _mkdir("SavedGames");

    char saveGamePath[MAX_PATH];
    sprintf(
        saveGamePath,
        "SavedGames\\%s",
        gameName
    );
    if (zReader::FileExists(saveGamePath) == 0) {
        return;
    }

    int shouldDelete = 1;
    if (confirmDelete != 0) {
        char titleText[128];
        char messageText[128];
        strcpy(
            titleText,
            zLoc::GetMessageString(138)
        );
        strcpy(
            messageText,
            zLoc::GetMessageString(139)
        );
        shouldDelete = HudUi::ShowMessageBox(
            messageText,
            titleText,
            (void *)1
        ) == 1 ? 1 : 0;
    }

    if (shouldDelete == 0) {
        return;
    }

    remove(saveGamePath);
    gameNameInput.Update("");
    RefreshSaveFileList();

    int selectedIndex = selectedEntryIndex;
    const int entryCount = SaveLoadEntryCount(this);
    if ((unsigned int)(selectedIndex) >= (unsigned int)(entryCount - 1)) {
        selectedIndex = entryCount - 1;
    }

    SetSelectedEntryIndex(selectedIndex);
}

/**
 * Purpose: Runs widget activation behavior and asks the dialog to delete the selected file.
 */
void HudUiSaveLoadDeleteButton::OnActivate() {
    HudUiSaveLoadDialog *const dialog = (HudUiSaveLoadDialog *)(owner);
    HudUiZrdWidget::OnActivate();
    dialog->DeleteSaveFile(1);
}

/**
 * Purpose: Advances the selected save/load entry when another entry exists.
 */
void HudUiSaveLoadNextButton::OnActivate() {
    HudUiSaveLoadDialog *const dialog = (HudUiSaveLoadDialog *)(owner);
    HudUiZrdWidget::OnActivate();

    const int nextEntryIndex = dialog->selectedEntryIndex + 1;
    if (nextEntryIndex >= 0 && nextEntryIndex < SaveLoadEntryCount(dialog)) {
        dialog->SetSelectedEntryIndex(nextEntryIndex);
    }
}

/**
 * Purpose: Moves the selected save/load entry to the previous valid row.
 */
void HudUiSaveLoadPrevButton::OnActivate() {
    HudUiSaveLoadDialog *const dialog = (HudUiSaveLoadDialog *)(owner);
    HudUiZrdWidget::OnActivate();

    const int prevEntryIndex = dialog->selectedEntryIndex - 1;
    if (prevEntryIndex >= 0 && prevEntryIndex < SaveLoadEntryCount(dialog)) {
        dialog->SetSelectedEntryIndex(prevEntryIndex);
    }
}

/**
 * Purpose: Commits the load-game dialog result before running the widget activation path.
 */
void HudUiLoadGamePrimaryActionButton::OnActivate() {
    HudUiLoadGameDialog *const dialog = (HudUiLoadGameDialog *)(owner);
    if (dialog != 0) {
        dialog->OnPrimaryAction();
    }

    HudUiZrdWidget::OnActivate();
}

/**
 * Purpose: Commits the save-game dialog result before running the widget activation path.
 */
void HudUiSaveGamePrimaryActionButton::OnActivate() {
    HudUiSaveLoadDialog *const dialog = (HudUiSaveLoadDialog *)(owner);
    if (dialog != 0) {
        dialog->ProcessDialogResult();
    }

    HudUiZrdWidget::OnActivate();
}

/**
 * Purpose: Processes the selected file path through the global archive entry path and exits the dialog.
 */
void HudUiLoadGameDialog::OnPrimaryAction() {
    char *const gameName = gameNameInput.GetBuffer();
    if (gameName == 0 || gameName[0] == '\0') {
        g_RecoilApp.QueueExitCurrentState(0);
        return;
    }

    _mkdir("SavedGames");

    char saveGamePath[MAX_PATH];
    sprintf(
        saveGamePath,
        "SavedGames\\%s",
        gameName
    );
    if (zReader::FileExists(saveGamePath) != 0) {
        char titleText[128];
        char messageText[128];
        strcpy(
            titleText,
            zLoc::GetMessageString(136)
        );
        strcpy(
            messageText,
            zLoc::GetMessageString(137)
        );
        if (HudUi::ShowMessageBox(
            messageText,
            titleText,
            (void *)1
        ) == 2) {
            return;
        }
    }

    while (zUtil::ZBD_LoadEntriesGlobal(saveGamePath) == 0) {
        DeleteSaveFile(0);

        char titleText[128];
        char messageText[128];
        strcpy(
            titleText,
            zLoc::GetMessageString(136)
        );
        strcpy(
            messageText,
            zLoc::GetMessageString(140)
        );
        if (HudUi::ShowMessageBox(
            messageText,
            titleText,
            (void *)1
        ) == 2) {
            break;
        }
    }

    g_RecoilApp.QueueExitCurrentState(0);
}

/**
 * Purpose: commit the current save-game name through the owning dialog.
 */
void HudUiSaveLoadGameNameInput::OnAccept() {
    HudUiSaveLoadDialog *const dialog = (HudUiSaveLoadDialog *)(owner);
    if (dialog != 0) {
        dialog->ProcessDialogResult();
    }
}

/**
 * Purpose: Updates the selected save/load entry and repopulates visible list rows around it.
 */
void HudUiSaveLoadDialog::SetSelectedEntryIndex(
    int selectedEntryIndexValue
) {
    selectedEntryIndex = selectedEntryIndexValue;

    for (int row = 0; row < 3; ++row) {
        const int entryIndex = selectedEntryIndexValue + row - 3;
        HudUiSaveLoadListItem *listItem = &entryWidgets[row];
        if (entryIndex >= 0) {
            unsigned int entryCount;
            if (fileEntries.begin == 0) {
                entryCount = 0;
            } else {
                entryCount = (unsigned int)(fileEntries.end - fileEntries.begin);
            }

            if ((unsigned int)entryIndex < entryCount) {
                listItem->layoutX = entryIndex;
                listItem->SetTextFmt(
                    "%s",
                    fileEntries.begin[entryIndex].cFileName
                );
                listItem->SetVisible(
                    1
                );
                listItem->Invalidate();
            } else {
                listItem->SetVisible(
                    0
                );
            }
        } else {
            listItem->SetVisible(
                0
            );
        }
    }

    if (selectedEntryIndexValue >= 0) {
        unsigned int selectedEntryCount;
        if (fileEntries.begin == 0) {
            selectedEntryCount = 0;
        } else {
            selectedEntryCount = (unsigned int)(fileEntries.end - fileEntries.begin);
        }

        if ((unsigned int)selectedEntryIndexValue < selectedEntryCount) {
            gameNameInput.Update(fileEntries.begin[selectedEntryIndexValue].cFileName);
        }
    }

    for (int lowerRow = 3; lowerRow < 9; ++lowerRow) {
        const int entryIndex = selectedEntryIndexValue + lowerRow - 2;
        HudUiSaveLoadListItem *listItem = &entryWidgets[lowerRow];
        if (entryIndex >= 0) {
            unsigned int entryCount;
            if (fileEntries.begin == 0) {
                entryCount = 0;
            } else {
                entryCount = (unsigned int)(fileEntries.end - fileEntries.begin);
            }

            if ((unsigned int)entryIndex < entryCount) {
                listItem->layoutX = entryIndex;
                listItem->SetTextFmt(
                    "%s",
                    fileEntries.begin[entryIndex].cFileName
                );
                listItem->SetVisible(
                    1
                );
                listItem->Invalidate();
            } else {
                listItem->SetVisible(
                    0
                );
            }
        } else {
            listItem->SetVisible(
                0
            );
        }
    }
}

/**
 * Purpose: Rebuilds and sorts the saved-game file entry vector from the SavedGames directory.
 */
void HudUiSaveLoadDialog::RefreshSaveFileList() {
    HudUiSaveLoadEntries *entries = &fileEntries;
    entries->EraseRangeNoDestroyInline(
        entries->begin,
        entries->end
    );

    HudUiSaveLoadEntry findData;
    HANDLE findHandle = FindFirstFileA(
        "SavedGames\\*.*",
        &findData
    );
    if (findHandle != INVALID_HANDLE_VALUE) {
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            entries->InsertCopiesAt(
                entries->end,
                1,
                &findData
            );
        }

        while (FindNextFileA(
            findHandle,
            &findData
        ) != 0) {
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                entries->InsertCopiesAt(
                    entries->end,
                    1,
                    &findData
                );
            }
        }
    }

    HudUiSaveLoadEntry *begin = entries->begin;
    HudUiSaveLoadEntry *end = entries->end;
    const int entryCount = end - begin;

    if (entryCount > 16) {
        HudUiSaveLoadEntry *rangeBegin = begin;
        HudUiSaveLoadEntry *rangeEnd = end;
        int rangeCount = entryCount;

        do {
            HudUiSaveLoadEntry lastEntry = *(rangeEnd - 1);
            HudUiSaveLoadEntry middleEntry = rangeBegin[rangeCount / 2];
            HudUiSaveLoadEntry firstEntry = *rangeBegin;

            HudUiSaveLoadEntry *pivotSource;
            if (firstEntry < middleEntry) {
                if (middleEntry < lastEntry) {
                    pivotSource = &middleEntry;
                } else if (firstEntry < lastEntry) {
                    pivotSource = &lastEntry;
                } else {
                    pivotSource = &firstEntry;
                }
            } else {
                if (firstEntry < lastEntry) {
                    pivotSource = &firstEntry;
                } else if (middleEntry < lastEntry) {
                    pivotSource = &lastEntry;
                } else {
                    pivotSource = &middleEntry;
                }
            }

            HudUiSaveLoadEntry pivotStageCopy = *pivotSource;
            HudUiSaveLoadEntry pivot = pivotStageCopy;
            HudUiSaveLoadEntry *split = PartitionEntriesByPivot(
                rangeBegin,
                rangeEnd,
                pivot
            );
            const int leftCount = split - rangeBegin;
            const int rightCount = rangeEnd - split;
            if (rightCount > leftCount) {
                SortEntryRange(
                    rangeBegin,
                    split,
                    0
                );
                rangeBegin = split;
            } else {
                SortEntryRange(
                    split,
                    rangeEnd,
                    0
                );
                rangeEnd = split;
            }

            rangeCount = rangeEnd - rangeBegin;
        } while (rangeCount > 16);
    }

    if (entryCount <= 16) {
        if (begin == end) {
            return;
        }

        HudUiSaveLoadEntry *entryPosition = begin + 1;
        if (entryPosition == end) {
            return;
        }

        do {
            HudUiSaveLoadEntry entry = *entryPosition;
            if (entry < *begin) {
                HudUiSaveLoadEntry *writePosition = entryPosition;
                while (writePosition != begin) {
                    *writePosition = *(writePosition - 1);
                    --writePosition;
                }
                *begin = entry;
            } else {
                InsertEntryIntoSortedPrefix(
                    entryPosition,
                    entry
                );
            }
            ++entryPosition;
        } while (entryPosition != end);
        return;
    }

    HudUiSaveLoadEntry *firstBlockEnd = begin + 16;
    if (begin != firstBlockEnd) {
        HudUiSaveLoadEntry *entryPosition = begin + 1;
        if (entryPosition != firstBlockEnd) {
            do {
                HudUiSaveLoadEntry entry = *entryPosition;
                if (entry < *begin) {
                    HudUiSaveLoadEntry *writePosition = entryPosition;
                    while (writePosition != begin) {
                        *writePosition = *(writePosition - 1);
                        --writePosition;
                    }
                    *begin = entry;
                } else {
                    InsertEntryIntoSortedPrefix(
                        entryPosition,
                        entry
                    );
                }
                ++entryPosition;
            } while (entryPosition != firstBlockEnd);
        }
    }

    for (HudUiSaveLoadEntry *entryPosition = firstBlockEnd; entryPosition != end; ++entryPosition) {
        HudUiSaveLoadEntry entry = *entryPosition;
        HudUiSaveLoadEntry *previous = entryPosition - 1;
        HudUiSaveLoadEntry *writePosition = entryPosition;
        if (entry < *previous) {
            do {
                *writePosition = *previous;
                writePosition = previous;
                --previous;
            } while (entry < *previous);
            *writePosition = entry;
        }
    }
}

/**
 * Purpose: Selects this row's save/load entry in its parent dialog.
 */
void HudUiSaveLoadListItem::OnActivate() {
    HudUiSaveLoadDialog *const owner = (HudUiSaveLoadDialog *)(parent);
    if (owner != 0) {
        owner->SetSelectedEntryIndex(layoutX);
    }
}

/**
 * Purpose: Initializes the save/load transition singleton and registers its exit cleanup.
 */
void __cdecl RecoilStateSaveLoadTransition::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-state-save-load-transition-static-init
 * @recoil-artifact defines .text recoil:function:0x435a40: RecoilStateSaveLoadTransition::StaticInit.
 * Purpose: Constructs the global save/load transition object.
 */
RecoilStateSaveLoadTransition *__cdecl RecoilStateSaveLoadTransition::StaticInit() {
    return g_RecoilStateSaveLoadTransition.Constructor();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-state-save-load-transition-register-at-exit
 * @recoil-artifact defines .text recoil:function:0x435a50: RecoilStateSaveLoadTransition::RegisterAtExit.
 * Purpose: Registers the save/load transition singleton destructor with atexit.
 */
void __cdecl RecoilStateSaveLoadTransition::RegisterAtExit() {
    atexit(AtExitDestructor);
}

/**
 * Purpose: Tears down the global save/load transition during process exit.
 */
void __cdecl RecoilStateSaveLoadTransition::AtExitDestructor() {
    g_RecoilStateSaveLoadTransition.Destructor();
}

/**
 * Purpose: Loads the selected saved game and queues the appropriate game-state transition.
 */
void HudUiSaveLoadDialog::ProcessDialogResult() {
    char *const gameName = gameNameInput.GetBuffer();
    char saveGamePath[MAX_PATH];
    saveGamePath[0] = '\0';

    if (gameName == 0 || gameName[0] == '\0') {
        return;
    }

    sprintf(
        saveGamePath,
        "SavedGames\\%s",
        gameName
    );
    if (zReader::FileExists(saveGamePath) == 0) {
        return;
    }

    if (zUtil::ZAR_LoadFileGlobal(saveGamePath) == 0) {
        return;
    }

    RecoilStateMainMenuTransition::ClearPausedAudioSnapshot();
    zSndPlayHandleSnapshot *const snapshot = (zSndPlayHandleSnapshot
            *)((unsigned int)(g_RecoilStateSaveLoadTransition.m_pausedAudioSnapshot));
    if (snapshot != 0) {
        snapshot->Destroy();
        g_RecoilStateSaveLoadTransition.m_pausedAudioSnapshot = 0;
    }

    zInp::SetJoystickOption(zInput::DI_SetJoystickEnabled(zInp::GetJoystickOption()));
    zOpt::SetCursorMode(zOpt::GetCursorMode());
    zOpt::SetCameraMode(zOpt::GetCameraModePlayerState());
    zOpt::SetThrottleMode(zOpt::GetThrottleMode());
    zOpt::SetSteeringMode(zOpt::GetSteeringMode());

    switch (g_RecoilStateSaveLoadTransition.m_transitionMode) {
    case RECOIL_SAVELOAD_MODE_STANDARD:
        if (saveGamePath[0] != '\0') {
            g_RecoilApp.m_playState.pPendingLoadGameStartPath = _strdup(saveGamePath);
            g_RecoilApp.m_missionFmvState.m_skipMissionFmv = 1;
            g_RecoilApp.QueueExitCurrentState(1);
            g_RecoilApp.QueueSwitchCurrentState(
                &g_RecoilApp.m_missionFmvState,
                0
            );
        } else {
            g_RecoilApp.QueueExitCurrentState(0);
        }
        break;

    case RECOIL_SAVELOAD_MODE_FADE:
        if (g_RecoilApp.m_transitionFadeTimer > 0.0) {
            g_RecoilApp.m_transitionFadeTimer += 5.0f;
        } else {
            g_RecoilApp.m_transitionFadeTimer = 5.0f;
            zOpt::SetMuteSoundOption(1);
        }
        g_RecoilApp.QueueExitCurrentState(1);
        g_RecoilApp.QueueExitCurrentState(1);
        break;

    case RECOIL_SAVELOAD_MODE_QUICKLOAD:
        if (g_RecoilApp.m_transitionFadeTimer > 0.0) {
            g_RecoilApp.m_transitionFadeTimer += 5.0f;
        } else {
            g_RecoilApp.m_transitionFadeTimer = 5.0f;
            zOpt::SetMuteSoundOption(1);
        }
        g_RecoilApp.QueueExitCurrentState(0);
        break;
    }
}

/**
 * Purpose: Initializes the save/load transition to the default save-dialog state.
 */
RecoilStateSaveLoadTransition * RecoilStateSaveLoadTransition::Constructor() {
    m_dialogKind = RECOIL_SAVELOAD_DIALOG_SAVE;
    m_dialog = 0;
    return this;
}

/**
 * Purpose: Deletes the active save or load dialog owned by the transition.
 */
void RecoilStateSaveLoadTransition::Destructor() {
    HudUiSaveLoadDialog *dialog = (HudUiSaveLoadDialog *)m_dialog;
    if (dialog != 0) {
        if (m_dialogKind == RECOIL_SAVELOAD_DIALOG_SAVE) {
            delete (HudUiSaveGameDialog *)dialog;
        } else {
            delete (HudUiLoadGameDialog *)dialog;
        }
        m_dialog = 0;
    }
}

/**
 * Purpose: Captures presentation/audio state and opens the requested save/load dialog.
 */
int RecoilStateSaveLoadTransition::OnTryBecomeCurrent() {
    if (m_capturePresentationMode != RECOIL_SAVELOAD_CAPTURE_PRESENTATION_DISABLED) {
        if (g_zVideo_ActiveRendererPath != 0) {
            g_zVideo_pfnBltSwToPrimaryRectDirect(
                0,
                0
            );
        }

        m_savedHalfResAdjustMode =
            (zVideoHalfResAdjustMode)zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_DISABLED);
        HudUi::SetInvalidateMode(0);
        zSnd::ApplyMuteStateToActiveVoices(1);

        zSndPlayHandleSnapshot *const audioSnapshot =
            zSndPlayHandleSnapshot::CreateFromActiveSamples();
        m_pausedAudioSnapshot = (RecoilPtr32)(unsigned int)audioSnapshot;
        audioSnapshot->StopAllIfPlaying();

        zFMV_ActionBlur blurAction(
            4,
            1
        );
        blurAction.Begin(0.0);
        while (blurAction.Update(0.0) != 0) {
        }
        blurAction.End();

        zSndSampleSet_InitByName(g_HudUiDialogSampleSetName);
    }

    HudUiSaveLoadDialog *dialog = 0;
    if (m_dialogKind == RECOIL_SAVELOAD_DIALOG_SAVE) {
        HudUiSaveGameDialog *const storage =
            (HudUiSaveGameDialog *) ::operator new(sizeof(HudUiSaveGameDialog));
        if (storage != 0) {
            dialog = new (storage) HudUiSaveGameDialog;
        }
    } else {
        HudUiLoadGameDialog *const storage =
            (HudUiLoadGameDialog *) ::operator new(sizeof(HudUiLoadGameDialog));
        if (storage != 0) {
            dialog = new (storage) HudUiLoadGameDialog;
        }
    }

    m_dialog = (RecoilPtr32)(unsigned int)dialog;
    dialog->SetEnabled(1);
    return 1;
}

#include "Battlesport/recoil_state_main_menu_transition.h"

#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zVideo/zvid.h"

/**
 * (BN canonical folded body).
 *
 * Source owner: app_shell.folded_dialog_update_should_quit. BN shows the
 * retail body shared by DialogHost, MainMenuTransition, SaveLoadTransition,
 * and other dialog-hosted state vtable slots; this definition preserves the
 * MainMenuTransition typed participant.
 *
 * Original-source function evidence: folded retail body 0x435e80.
 * Purpose: update and present the active main-menu dialog each frame while the
 * transition state is current.
 */
int RecoilStateMainMenuTransition::OnUpdateShouldQuit() {
    zInput::PollActiveDevices(0);

    if (m_mainMenuDialog != 0) {
        Time::Tick();
        zVideo::RunPostprocessOnPrimaryBuffer();

        ((HudUiContainer *)m_mainMenuDialog)->UpdateAll(g_FrameDeltaTimeSec);

        zVideo::Dispatch_UnlockPrimarySurfaceState();
    }

    zVideo::AdjustSurfacesIfEnabled(
        (zVidRect32 *)zOpt::GetWindowSection(),
        (zVidRect32 *)zOpt::GetWindowSection(),
        1,
        1
    );
    return 0;
}

/**
 * (BN canonical folded body).
 * BN source-owner evidence for the SaveLoadTransition participant shows the
 * retail body shared by DialogHost, MainMenuTransition, SaveLoadTransition,
 * and other dialog-hosted state vtable slots; this definition preserves the
 * SaveLoadTransition typed participant.
 * The original-source function evidence is the folded retail body at 0x435e80.
 * Purpose: Updates the active save/load dialog and reports whether the transition should quit.
 */
int RecoilStateSaveLoadTransition::OnUpdateShouldQuit() {
    zInput::PollActiveDevices(0);

    if (m_dialog != 0) {
        Time::Tick();
        zVideo::RunPostprocessOnPrimaryBuffer();

        ((HudUiSaveLoadDialog *)((unsigned int)m_dialog))->UpdateAll(g_FrameDeltaTimeSec);

        zVideo::Dispatch_UnlockPrimarySurfaceState();
    }

    zOpt_ViewRectSection *const dstRect = zOpt::GetWindowSection();
    zOpt_ViewRectSection *const srcRect = zOpt::GetWindowSection();
    zVideo::AdjustSurfacesIfEnabled(
        (zVidRect32 *)srcRect,
        (zVidRect32 *)dstRect,
        1,
        1
    );
    return 0;
}

/**
 * Purpose: Restores captured presentation/audio state and deletes the active save/load dialog.
 */
void RecoilStateSaveLoadTransition::OnDeactivate() {
    if (m_dialog != 0) {
        zVideo::RunPostprocessOnPrimaryBuffer();

        HudUiSaveLoadDialog *dialog = (HudUiSaveLoadDialog *)((unsigned int)m_dialog);
        dialog->SetEnabled(0);

        ((HudUiDialogController *)((unsigned int)m_dialog))->BlitOwnedSurfaceToPrimary();
        zVideo::Dispatch_UnlockPrimarySurfaceState();

        dialog = (HudUiSaveLoadDialog *)((unsigned int)m_dialog);
        if (dialog != 0) {
            if (m_dialogKind == RECOIL_SAVELOAD_DIALOG_SAVE) {
                delete (HudUiSaveGameDialog *)dialog;
            } else {
                delete (HudUiLoadGameDialog *)dialog;
            }
        }

        m_dialog = 0;
    }

    if (m_capturePresentationMode == RECOIL_SAVELOAD_CAPTURE_PRESENTATION_DISABLED) {
        return;
    }

    zSndSampleSet_DestroyByName(g_HudUiDialogSampleSetName);

    zSndPlayHandleSnapshot *const audioSnapshot =
        (zSndPlayHandleSnapshot *)((unsigned int)m_pausedAudioSnapshot);
    if (audioSnapshot != 0) {
        audioSnapshot->RestoreAllWithGlobalVolumeDelta();
    }

    zSnd::ApplyMuteStateToActiveVoices(0);
    zVideo::SetHalfResAdjustMode(m_savedHalfResAdjustMode);
    HudUi::SetInvalidateMode(m_savedHalfResAdjustMode);
    HudUiMgr::TriggerCurrentLayoutOnActivated();
}

/**
 * Purpose: Configures and queues the save-dialog transition.
 */
void __fastcall RecoilStateSaveLoadTransition::QueueOpenSaveDialog(
    RecoilSaveLoadPresentationCaptureMode capturePresentationMode
) {
    if (HudUiMainMenuDialog::CanSaveGame() == 0) {
        return;
    }

    g_RecoilStateSaveLoadTransition.m_capturePresentationMode = capturePresentationMode;
    g_RecoilStateSaveLoadTransition.m_dialogKind = RECOIL_SAVELOAD_DIALOG_SAVE;
    g_RecoilApp.QueuePushState(
        &g_RecoilStateSaveLoadTransition,
        0
    );
}

/**
 * Purpose: Configures and queues the load-dialog transition.
 */
void __fastcall RecoilStateSaveLoadTransition::QueueOpenLoadDialog(
    RecoilSaveLoadTransitionMode transitionMode
) {
    if (HudUiMainMenuDialog::CanLoadGame() == 0) {
        return;
    }

    g_RecoilStateSaveLoadTransition.m_transitionMode = transitionMode;
    switch (transitionMode) {
    case RECOIL_SAVELOAD_MODE_STANDARD:
        break;
    case RECOIL_SAVELOAD_MODE_QUICKLOAD:
        g_RecoilStateSaveLoadTransition.m_capturePresentationMode =
            RECOIL_SAVELOAD_CAPTURE_PRESENTATION_ENABLED;
        break;
    }

    g_RecoilStateSaveLoadTransition.m_dialogKind = RECOIL_SAVELOAD_DIALOG_LOAD;
    g_RecoilApp.QueuePushState(
        &g_RecoilStateSaveLoadTransition,
        0
    );
}

/**
 * Purpose: Sorts a save/load entry range from newest to oldest using quicksort with insertion cleanup.
 */
void __fastcall SortEntryRange(
    HudUiSaveLoadEntry *begin,
    HudUiSaveLoadEntry *end,
    int unused
) {
    (void)unused;

    HudUiSaveLoadEntry *rangeBegin = begin;
    HudUiSaveLoadEntry *rangeEnd = end;
    int entryCount = rangeEnd - rangeBegin;
    if (entryCount <= 16) {
        return;
    }

    for (;;) {
        HudUiSaveLoadEntry lastEntry = *(rangeEnd - 1);
        HudUiSaveLoadEntry middleEntry = rangeBegin[entryCount / 2];
        HudUiSaveLoadEntry firstEntry = *rangeBegin;

        HudUiSaveLoadEntry *pivotSource;
        if (firstEntry < middleEntry) {
            if (middleEntry < lastEntry) {
                pivotSource = &middleEntry;
            } else if (firstEntry < lastEntry) {
                pivotSource = &lastEntry;
            } else {
                pivotSource = &firstEntry;
            }
        } else {
            if (firstEntry < lastEntry) {
                pivotSource = &firstEntry;
            } else if (middleEntry < lastEntry) {
                pivotSource = &lastEntry;
            } else {
                pivotSource = &middleEntry;
            }
        }

        HudUiSaveLoadEntry pivotStageCopy = *pivotSource;
        HudUiSaveLoadEntry pivotEntry = pivotStageCopy;
        HudUiSaveLoadEntry *left = rangeBegin;
        HudUiSaveLoadEntry *right = rangeEnd;

        for (;;) {
            while (*left < pivotEntry) {
                ++left;
            }

            --right;
            while (pivotEntry < *right) {
                --right;
            }

            if (right <= left) {
                break;
            }

            HudUiSaveLoadEntry swapTemp = *left;
            *left = *right;
            ++left;
            *right = swapTemp;
        }

        const int rightCount = rangeEnd - left;
        const int leftCount = left - rangeBegin;
        if (rightCount > leftCount) {
            SortEntryRange(
                rangeBegin,
                left,
                0
            );
            rangeBegin = left;
        } else {
            SortEntryRange(
                left,
                rangeEnd,
                0
            );
            rangeEnd = left;
        }

        entryCount = rangeEnd - rangeBegin;
        if (entryCount <= 16) {
            break;
        }
    }
}

/**
 * Purpose: Inserts one save/load entry into the already sorted prefix before it.
 */
void __fastcall InsertEntryIntoSortedPrefix(
    HudUiSaveLoadEntry *entryPosition,
    HudUiSaveLoadEntry entry
) {
    HudUiSaveLoadEntry *writePosition = entryPosition;
    HudUiSaveLoadEntry *previous = entryPosition - 1;

    while (entry < *previous) {
        *writePosition = *previous;
        writePosition = previous;
        --previous;
    }

    *writePosition = entry;
}

/**
 * Purpose: Partitions a save/load entry range around the selected pivot entry.
 */
HudUiSaveLoadEntry *__fastcall PartitionEntriesByPivot(
    HudUiSaveLoadEntry *begin,
    HudUiSaveLoadEntry *end,
    HudUiSaveLoadEntry pivot
) {
    HudUiSaveLoadEntry *right = end;
    HudUiSaveLoadEntry *left = begin;

    for (;;) {
        while (*left < pivot) {
            ++left;
        }

        --right;
        while (pivot < *right) {
            --right;
        }

        if (right <= left) {
            break;
        }

        HudUiSaveLoadEntry temp = *left;
        *left = *right;
        ++left;
        *right = temp;
    }

    return left;
}

/**
 * Source model note: the ordinary empty RecoilApp_MainMenuPrepState::OnDeactivate
 * identity represented by the zero-argument no-op fold group at 0x4076f0.
 * Original function address: 0x4076f0.
 * Purpose: accept deactivation after the main-menu preparation state has
 * completed its transition work.
 */
void RecoilApp_MainMenuPrepState::OnDeactivate() {
}
