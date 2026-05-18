#include "Battlesport/RecoilApp.h"

#include "Battlesport/Briefing.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/HudUiNetExitPanel.h"
#include "Battlesport/RecoilVersion.h"
#include "Battlesport/hud.h"
#include "Battlesport/player.h"
#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zSys/zSys.h"
#include "GameZRecoil/zTurret/zTurret.h"
#include "GameZRecoil/zUtil/zZbd.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "GameZRecoil/zWeapon/zWeapon.h"
#include "OptCatalog.h"
#include "pickup.h"
#include "zImage.h"

#include <windows.h>

#ifdef FormatMessage
#undef FormatMessage
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class CWinApp {
  public:
    CWinApp(const char *appName);
    virtual ~CWinApp();
    BOOL RECOIL_THISCALL Enable3dControls();
    virtual int ExitInstance();
};

BOOL RECOIL_THISCALL CWinApp::Enable3dControls() {
    return TRUE;
}

class CWnd {
  public:
    static CWnd *RECOIL_STDCALL FromHandle(HWND hwnd);
    BOOL ShowWindow(int showCommand);

    unsigned char unknown_00[0x20];
    HWND m_hWnd;
};

class AFX_MODULE_STATE {
  public:
    void *m_pCurrentWinApp;
    void *unknown_04;
    HINSTANCE m_hCurrentInstanceHandle;
};

AFX_MODULE_STATE *RECOIL_STDCALL AfxGetModuleState();
BOOL RECOIL_STDCALL AfxRegisterClass(WNDCLASSA *wndClass);
HINSTANCE RECOIL_STDCALL AfxFindResourceHandle(LPCSTR resourceName, LPCSTR resourceType);

namespace {
LPCSTR IntResource(unsigned int value) {
    return reinterpret_cast<LPCSTR>(value);
}

} // namespace

RecoilApp g_RecoilApp;
RecoilStateSaveLoadTransition g_RecoilStateSaveLoadTransition;

extern "C" HWND g_RecoilApp_hWndMain;
extern "C" HINSTANCE g_RecoilApp_hInstance;
extern "C" {
const char *g_RecoilApp_WndClassNamePtr = "RecoilClass";
int g_RecoilApp_WindowClassRegistered = 0;
int g_RecoilApp_AttractFmvReloadMode = 1;
}

// Reimplements 0x435f50: RecoilStateSaveLoadTransition::QueueOpenSaveDialog
// (D:\Proj\Battlesport\RecoilApp.cpp)
void RECOIL_FASTCALL
RecoilStateSaveLoadTransition::QueueOpenSaveDialog(
    RecoilSaveLoadPresentationCaptureMode capturePresentationMode)
{
    if (HudUiMainMenuDialog::CanSaveGame() == 0) {
        return;
    }

    g_RecoilStateSaveLoadTransition.m_capturePresentationMode = capturePresentationMode;
    g_RecoilStateSaveLoadTransition.m_dialogKind = RECOIL_SAVELOAD_DIALOG_SAVE;
    g_RecoilApp.QueuePushState(&g_RecoilStateSaveLoadTransition, 0);
}

// Reimplements 0x435f80: RecoilStateSaveLoadTransition::QueueOpenLoadDialog
// (D:\Proj\Battlesport\RecoilApp.cpp)
void RECOIL_FASTCALL
RecoilStateSaveLoadTransition::QueueOpenLoadDialog(RecoilSaveLoadTransitionMode transitionMode)
{
    if (HudUiMainMenuDialog::CanLoadGame() == 0) {
        return;
    }

    g_RecoilStateSaveLoadTransition.m_transitionMode = transitionMode;
    if (transitionMode != RECOIL_SAVELOAD_MODE_STANDARD &&
        transitionMode == RECOIL_SAVELOAD_MODE_QUICKLOAD) {
        g_RecoilStateSaveLoadTransition.m_capturePresentationMode =
            RECOIL_SAVELOAD_CAPTURE_PRESENTATION_ENABLED;
    }

    g_RecoilStateSaveLoadTransition.m_dialogKind = RECOIL_SAVELOAD_DIALOG_LOAD;
    g_RecoilApp.QueuePushState(&g_RecoilStateSaveLoadTransition, 0);
}

// Reimplements 0x430c90: RecoilApp::FatalErrorAndExit (D:\Proj\Battlesport\RecoilApp.cpp)
RECOIL_NOINLINE RECOIL_NO_GS void RECOIL_FASTCALL
RecoilApp::FatalErrorAndExit(int errorCode) {
    if (errorCode != -1) {
        return;
    }

    char caption[0x80];
    char text[0x80];
    strcpy(caption, zLoc::GetMessageString(0x12));
    strcpy(text, zLoc::GetMessageString(0x30));

    Briefing::StopAndShutdownThread(0);
    zVideo_dd::FlipToGDIIfAttached();
    zSndSystem::Shutdown();
    zNetwork::ShutdownSessionRuntime();
    zVideo::ShutdownVideoSystem();
    printf("%s: %s\n", caption, text);
    Sleep(1000);
    MessageBeep(MB_ICONHAND);
    MessageBoxA(g_RecoilApp_hWndMain, text, caption, MB_ICONHAND);
    zSys::ExitProcessWithCleanup(0);
}

// Reimplements 0x42e930: RecoilApp::ExitInstance
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp::ExitInstance() {
    if (g_RecoilApp_WindowClassRegistered != 0) {
        UnregisterClassA(g_RecoilApp_WndClassNamePtr,
                         AfxGetModuleState()->m_hCurrentInstanceHandle);
        zGame::Options_SaveGameOptions();
        zGame::ReturnOnlyStub();
        zGame::Options_ShutdownRegistryContext();
        zUtil_ZRDR_Shutdown();
        zUtil_ZRDR_FreeNodePool();
        zUtil::ZBD_DestroyGlobalManager();
        zLoc::UnloadMessagesDll();
    }

    zInput::BindMapSystem_Shutdown();
    reinterpret_cast<CWinApp *>(this)->CWinApp::ExitInstance();
    zSys::ExitProcessWithCleanup(0);
    return 0;
}

// Reimplements 0x42e520: RecoilApp::InitInstance
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_THISCALL RecoilApp::InitInstance() {
    if (ActivateExistingInstance() == 0) {
        return 0;
    }

    WNDCLASSA wndClass = {0};
    wndClass.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
    wndClass.lpfnWndProc = DefWindowProcA;
    wndClass.hInstance = AfxGetModuleState()->m_hCurrentInstanceHandle;
    wndClass.hIcon =
        LoadIconA(AfxFindResourceHandle(IntResource(0x97), IntResource(0x0e)), IntResource(0x97));
    wndClass.hCursor = LoadCursorA(AfxFindResourceHandle(IntResource(0x7f00), IntResource(0x0c)),
                                   IntResource(0x7f00));
    wndClass.hbrBackground = CreateSolidBrush(0);
    wndClass.lpszMenuName = 0;
    wndClass.lpszClassName = g_RecoilApp_WndClassNamePtr;

    if (AfxRegisterClass(&wndClass) == 0) {
        return 0;
    }

    g_RecoilApp_WindowClassRegistered = 1;
    InitMainWindow();
    m_reserved148 = 0;
    m_pendingState_0c4 =
        static_cast<RecoilPtr32>(reinterpret_cast<unsigned int>(&m_introFmvState_1a0));

    char errorTextBuffer[0x400];
    char messageCaptionBuffer[0x100];
    char sharedTextBuffer[0x100];
    char registryCompanyNameBuffer[0x100];

    if (zLoc::LoadMessagesDll("MESSAGES.DLL") == 0) {
        char *systemErrorText = 0;
        sprintf(errorTextBuffer, "Exit at %s:%d\n", "D:\\Proj\\Battlesport\\RecoilApp.cpp",
                     0x188);
        OutputDebugStringA(errorTextBuffer);
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0,
                       GetLastError(), 0x400, reinterpret_cast<LPSTR>(&systemErrorText), 0,
                       0);
        strcpy(errorTextBuffer, systemErrorText);
        strcat(errorTextBuffer, "\n\n");
        strcat(errorTextBuffer, "MESSAGES.DLL");
        LocalFree(systemErrorText);
        zVideo_dd::FlipToGDIIfAttached();
        MessageBeep(MB_ICONASTERISK);
        MessageBoxA(0, errorTextBuffer, "", MB_ICONASTERISK);
        ExitProcess(0);
    }

    zLoc::FormatMessage(messageCaptionBuffer, 0x100, 0x83);
    while (zSys::FindFileOnDriveType(5, "video\\intro_01.avi", 0) == 0) {
        MessageBeep(MB_ICONEXCLAMATION);
        if (MessageBoxA(g_RecoilApp_hWndMain, messageCaptionBuffer, zLoc::GetMessageString(0x901),
                        MB_OKCANCEL | MB_ICONEXCLAMATION) != IDOK) {
            ExitProcess(0);
        }
    }

    zSysVideoCapsLevel videoCaps = ZSYS_VIDEO_CAPS_NONE;
    zSysPlatformCapsLevel platformCaps = ZSYS_PLATFORM_CAPS_UNSUPPORTED;
    zSys::ProbePlatformAndVideoCaps(&videoCaps, &platformCaps);
    if (static_cast<unsigned int>(videoCaps) <
        static_cast<unsigned int>(ZSYS_VIDEO_CAPS_SURFACE4)) {
        zLoc::FormatMessage(messageCaptionBuffer, 0x100, 0x14);
        zLoc::FormatMessage(sharedTextBuffer, 0x100, 0x16);
        MessageBeep(MB_ICONHAND);
        MessageBoxA(g_RecoilApp_hWndMain, sharedTextBuffer, messageCaptionBuffer, MB_ICONHAND);
        ExitProcess(0);
    }

    zGame::ReturnOnlyStub();
    zUtil::ZBD_Init();
    zUtil::ZRDR_PreallocNodePool(0x200);
    zUtil::ZRDR_AddSearchPaths(0, "zbd");
    zUtil::ZRDR_Init("..\\data\\common\\zrdr");

    strncpy(registryCompanyNameBuffer, zLoc::GetMessageString(0x900),
                 sizeof(registryCompanyNameBuffer));
    strncpy(sharedTextBuffer, zLoc::GetMessageString(0x901), sizeof(sharedTextBuffer));
    zGame::Options_InitRegistryContext(registryCompanyNameBuffer, sharedTextBuffer,
                                       RecoilVersion::GetString());
    zInput::BindMapSystem_Init(0x2f);

    if (zGame::Options_LoadGameOptions() == 0) {
        zArchive::MountIndexArchive("zbd\\zrdr.zbd", 1);
        if (zGame::Options_LoadGameOptions() == 0) {
            strcpy(sharedTextBuffer, zLoc::GetMessageString(0x901));
            MessageBeep(MB_ICONHAND);
            MessageBoxA(g_RecoilApp_hWndMain, zLoc::GetMessageString(0x1e), sharedTextBuffer,
                        MB_ICONHAND);
            ExitProcess(0);
        }
    }

    zVid::SetVideoModeIndex(zVid::GetVideoModeIndexFromOptions());
    CZRecoilFrame *const frame = reinterpret_cast<CZRecoilFrame *>(static_cast<unsigned int>(GetMainWnd()));
    frame->ConfigureModeFeatureFlags();
    reinterpret_cast<CZRecoilFrame *>(static_cast<unsigned int>(GetMainWnd()))
        ->InitStartupHwApiFromOptions();
    return 1;
}

// Reimplements 0x42e110: RecoilApp::CreateMainWnd
RECOIL_NOINLINE CZRecoilFrame *RECOIL_THISCALL RecoilApp::CreateMainWnd() {
    CZRecoilFrame *frame = new CZRecoilFrame;
    if (frame == 0) {
        return 0;
    }

    return frame->Constructor();
}

// Reimplements 0x4429d0: RecoilApp::InitMainWindow
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp::InitMainWindow() {
    reinterpret_cast<CWinApp *>(this)->Enable3dControls();

    typedef CZRecoilFrame * (RECOIL_THISCALL *CreateMainWndMethod)(RecoilApp *);
    const CreateMainWndMethod *const createMainWnd = reinterpret_cast<const CreateMainWndMethod *>(static_cast<unsigned int>(vftable + 0xb8));
    m_pMainWnd = static_cast<RecoilPtr32>(reinterpret_cast<unsigned int>((*createMainWnd)(this)));
    CZRecoilFrame *const mainWnd = reinterpret_cast<CZRecoilFrame *>(static_cast<unsigned int>(GetMainWnd()));
    mainWnd->m_app = static_cast<RecoilPtr32>(reinterpret_cast<unsigned int>(this));
    reinterpret_cast<CWnd *>(static_cast<unsigned int>(m_pMainWnd))->ShowWindow(SW_SHOW);
    UpdateWindow(reinterpret_cast<CWnd *>(static_cast<unsigned int>(m_pMainWnd))->m_hWnd);
    return 1;
}

namespace {
typedef void (RECOIL_THISCALL *RecoilStateMethod)(RecoilApp_IState *);
typedef int (RECOIL_THISCALL *RecoilStateIdleMethod)(RecoilApp_IState *, unsigned int,
                                                              unsigned int);
typedef int (RECOIL_THISCALL *RecoilAppNoArgIntMethod)(RecoilApp *);
typedef void (RECOIL_THISCALL *RecoilAppNoArgVoidMethod)(RecoilApp *);
typedef int (RECOIL_THISCALL *RecoilAppStartEngineMethod)(RecoilApp *, RecoilPtr32);

const char *kEngineInitPassed = "PASSED";
const char *kEngineInitFailed = "FAILED";

void PrintEngineInitStatus(const char *format, bool passed) {
    printf(format, passed ? kEngineInitPassed : kEngineInitFailed);
}

void CallRecoilStateMethod(RecoilPtr32 stateValue, size_t vtableOffset) {
    RecoilApp_IState *const state = reinterpret_cast<RecoilApp_IState *>(static_cast<unsigned int>(stateValue));
    const RecoilPtr32 methodValue = *reinterpret_cast<const RecoilPtr32 *>(
        static_cast<unsigned int>(state->vftable + vtableOffset));
    RecoilStateMethod const method = reinterpret_cast<RecoilStateMethod>(static_cast<unsigned int>(methodValue));
    method(state);
}

RecoilFn32 ReadRecoilVtableSlot(RecoilPtr32 vftable, size_t offset) {
    return *reinterpret_cast<const RecoilFn32 *>(static_cast<unsigned int>(vftable + offset));
}
} // namespace

// Reimplements 0x442a50: RecoilApp::EngineInit
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp::EngineInit(RecoilPtr32 hwnd) {
    zUtil::ZRDR_PreallocNodePool(0);
    zUtil::ZRDR_Init(0);

    PrintEngineInitStatus("gModInit:  %s\n", zModel_Display_Init() == 0);
    PrintEngineInitStatus("gClsInit:  %s\n", zVideo::ReturnSuccessStub() == 0);
    PrintEngineInitStatus("zEffInit:  %s\n", zEffect::Init() == 0);
    PrintEngineInitStatus("zRndrInit: %s\n", zRndr::InitGlobals() == 0);
    PrintEngineInitStatus("zSndInit:  %s\n", zSnd_PreInitializeRuntimeState(hwnd) != 0);
    PrintEngineInitStatus("zUtlInit:  %s\n", zVideo::ReturnSuccessStub() == 0);
    PrintEngineInitStatus("zWepInit:  %s\n", zWepInit() == 0);
    PrintEngineInitStatus("zImgInit:  %s\n", zImage_Init(0) == 0);

    if (g_zVideo_ActiveRendererPath == 2) {
        zInput::Mouse_SetCooperativeLevelFlags(5);
    }

    PrintEngineInitStatus("zInInit:  %s\n",
                          zInput::Init(reinterpret_cast<HWND>(static_cast<unsigned int>(hwnd)),
                                       reinterpret_cast<HINSTANCE>(
                                           static_cast<unsigned int>(m_hInstance_6c))) == 0);
    Time::Reset();
    zVid::SetCachedClientRectUpdateMask(1);
    return 1;
}

// Reimplements 0x42e330: RecoilApp::InitializeDisplay
RECOIL_NOINLINE int RECOIL_FASTCALL RecoilApp::InitializeDisplay(RecoilPtr32 hwnd) {
    const int modeIndex = zVid::GetVideoModeIndexFromOptions();
    const int fullscreen = zOpt::GetFullscreenOption();
    if (zVideo::InitVideoSystem(reinterpret_cast<HWND>(static_cast<unsigned int>(hwnd)),
                                zVid::GetHwApiOption(), fullscreen, modeIndex) != 0) {
        printf("Error opening video... ABORTING RUN\n");
        fflush(stdout);
        return 0;
    }

    if (zVid::GetAccelerationOption() == 0 &&
        zRndr::SpanOcclusionInit(zOpt::GetWindowSectionHeight()) != 0) {
        printf("Error opening HSE... ABORTING RUN\n");
        fflush(stdout);
        return 0;
    }

    const int pitchBytes = zVideo::GetPrimarySurfacePitch();
    const int bitsPerPixel = zOpt::GetDisplaySectionBitsPerPixel();
    zOpt_ViewRectSection *const activeRegionRect = zOpt::GetDisplaySection();
    void *const primaryPixels = zVideo::GetPrimarySurfacePixels();
    zRndr::SetFrameBufferRegion(primaryPixels, activeRegionRect, bitsPerPixel, pitchBytes);
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

// Reimplements 0x42e220: RecoilApp::StartEngine
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_THISCALL RecoilApp::StartEngine(RecoilPtr32 hwnd) {
    EngineInit(hwnd);
    PrintEngineInitStatus("turret:    %s\n", zTurret_System::ResetIterationState() != 0);

    zSndSystem_Init(hwnd, "sounds.zrd");
    zSnd::SetAudioApiOption(zSnd::GetActiveBackend());

    if (InitializeDisplay(hwnd) == 0) {
        char caption[0x80];
        strcpy(caption, zLoc::GetMessageString(0x901));
        MessageBoxExA(reinterpret_cast<HWND>(static_cast<unsigned int>(hwnd)),
                      zLoc::GetMessageString(0x1f), caption, MB_ICONHAND, 0);
        return 0;
    }

    zInput::Init(reinterpret_cast<HWND>(static_cast<unsigned int>(hwnd)), g_RecoilApp_hInstance);
    const int height = zOpt_DisplaySection_GetHeight();
    zInput::Mouse_SetClientSizeAndCenter(zOpt_DisplaySection_GetWidth(), height);
    zInput::DI_SetJoystickEnabled(zInp::GetJoystickOption());

    zOpt_ViewRectSection *const windowSection = zOpt::GetWindowSection();
    HudUiMgr::InitHudLayouts(reinterpret_cast<const HudUiRect *>(zOpt::GetDisplaySection()),
                             reinterpret_cast<const HudUiRect *>(windowSection));
    return 1;
}

// Reimplements 0x42e990: RecoilApp::ActivateExistingInstance
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp::ActivateExistingInstance() {
    CWnd *const existingWindow =
        CWnd::FromHandle(FindWindowA(g_RecoilApp_WndClassNamePtr, 0));
    if (existingWindow == 0) {
        return 1;
    }

    CWnd *const popup = CWnd::FromHandle(GetLastActivePopup(existingWindow->m_hWnd));
    if (IsIconic(existingWindow->m_hWnd) != 0) {
        existingWindow->ShowWindow(SW_RESTORE);
    }

    SetForegroundWindow(popup->m_hWnd);
    return 0;
}

// Reimplements 0x42e9f0: RecoilApp::PreTranslateMessage
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp::PreTranslateMessage(tagMSG *msg) {
    if (zVid::GetAccelerationOption() != 0) {
        const UINT message = msg->message;
        if (message >= WM_SYSKEYDOWN && message <= WM_SYSKEYUP) {
            return 1;
        }
    }

    return 0;
}

namespace zSndCd {
void RECOIL_FASTCALL OnMciNotify(unsigned int wParam, unsigned int lParam);
}

namespace zDEClient {
RECOIL_NOINLINE int RECOIL_CDECL ShutdownGlobals();
}

// Reimplements 0x442bc0: RecoilApp::ShutdownSubsystems
RECOIL_NOINLINE int RECOIL_CDECL RecoilApp::ShutdownSubsystems() {
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
    return 0;
}

// Reimplements 0x42e430: RecoilApp::ShutdownEngine
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp::ShutdownEngine() {
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

// Reimplements 0x42e490: RecoilApp::LoadZbdAndStartEngine
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp::LoadZbdAndStartEngine() {
    if (g_HudSensorTracker.missionFlags != 0) {
        zArchive::MountIndexArchive("zbd\\zrdr.zbd", 1);
    }

    StartEngineAndQueueStartupState();
    g_HudSensorTracker.RegisterMissionSectionHandlers();
    return 1;
}

// Reimplements 0x42e4d0: RecoilApp::LoadZbdAndSetupSensorTracker
RECOIL_NOINLINE int RECOIL_THISCALL
RecoilApp::LoadZbdAndSetupSensorTracker(int missionId, const char *zbdPath,
                                        int skipIntroFmvMode, int missionFlags) {
    LoadZbdAndStartEngine();
    m_skipIntroFmv = skipIntroFmvMode;
    if (zbdPath != 0) {
        g_HudSensorTracker.SetZbdPath(zbdPath);
        return 1;
    }

    g_HudSensorTracker.InitMissionIdAndFlags(missionId, missionFlags);
    return 1;
}

extern const zMfcMsgMapEntry g_RecoilApp_MessageEntries[1] = {0};

extern const MfcMsgMap g_RecoilApp_MessageMap = {
    0x004428a0,
    0x004d0998,
};

// Reimplements 0x42de20: RecoilApp::StaticInitAndRegisterAtExit
RECOIL_NOINLINE void RECOIL_CDECL RecoilApp::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

// Reimplements 0x42de30: RecoilApp::StaticInit
RECOIL_NOINLINE RecoilApp *RECOIL_CDECL RecoilApp::StaticInit() {
    return g_RecoilApp.Constructor();
}

// Reimplements 0x42de40: RecoilApp::RegisterAtExit
RECOIL_NOINLINE void RECOIL_CDECL RecoilApp::RegisterAtExit() {
    atexit(AtExitDestructor);
}

// Reimplements 0x42de50: RecoilApp::AtExitDestructor
RECOIL_NOINLINE void RECOIL_CDECL RecoilApp::AtExitDestructor() {
    g_RecoilApp.Destructor();
}

// Reimplements 0x4a5780: RecoilApp::InitStdLogFiles
RECOIL_NOINLINE RECOIL_NO_GS void RECOIL_FASTCALL RecoilApp::InitStdLogFiles(const char *exePath) {
    g_RecoilApp_hWndMain = 0;
    if (exePath == 0) {
        return;
    }

    char pathBuf[0x40];
    strcpy(pathBuf, exePath);
    strcat(pathBuf, ".err");
    FILE *stream = freopen(pathBuf, "w", stderr);
    if (stream == 0 && GetTempPathA(sizeof(pathBuf), pathBuf) != 0) {
        strcat(pathBuf, "gamez.err");
        stream = freopen(pathBuf, "w", stderr);
    }
    if (stream != 0) {
        fprintf(stream, "File started\n---\n");
        fflush(stream);
    }

    strcpy(pathBuf, exePath);
    strcat(pathBuf, ".out");
    stream = freopen(pathBuf, "w", stdout);
    if (stream == 0 && GetTempPathA(sizeof(pathBuf), pathBuf) != 0) {
        strcat(pathBuf, "gamez.out");
        stream = freopen(pathBuf, "w", stdout);
    }
    if (stream != 0) {
        fprintf(stream, "File started\n---\n");
        fflush(stream);
    }
}

// Reimplements 0x42dfa0: RecoilApp::Constructor
RECOIL_NOINLINE RecoilApp *RECOIL_THISCALL RecoilApp::Constructor() {
    MfcOleModuleConstructor();
    m_attractFmvState_160.Constructor();

    m_introFmvState_1a0.base.vftable = kRecoilStateBase_VtblAddress;
    reinterpret_cast<zFMV_Script *>(m_introFmvState_1a0.m_fmv_08)->Init(0, 0, 0);
    m_introFmvState_1a0.base.vftable = kRecoilApp_IntroFmvState_VtblAddress;

    m_mainMenuPrepState_1c8.base.vftable = kRecoilApp_MainMenuPrepState_VtblAddress;
    m_leaveNetworkState_1d0.base.vftable = kRecoilApp_LeaveNetworkState_VtblAddress;

    m_missionFmvState_1d8.Constructor();
    m_playState_208.Constructor();
    *reinterpret_cast<volatile RecoilPtr32 *>(&m_mpExitDialogState_220.base.vftable) =
        kRecoilApp_MpExitDialogState_VtblAddress;
    *reinterpret_cast<volatile RecoilPtr32 *>(&vftable) = kRecoilApp_VtblAddress;
    *reinterpret_cast<volatile unsigned int *>(&m_transitionFadeTimer150) = 0;
    return this;
}

// Reimplements 0x42de60: RecoilApp::Destructor
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp::Destructor() {
    m_mpExitDialogState_220.base.vftable = kRecoilStateBase_VtblAddress;
    m_playState_208.base.vftable = kRecoilStateBase_VtblAddress;

    reinterpret_cast<zFMV_Script *>(m_missionFmvState_1d8.m_fmv_08)->Cleanup();
    m_missionFmvState_1d8.base.vftable = kRecoilStateBase_VtblAddress;
    m_leaveNetworkState_1d0.base.vftable = kRecoilStateBase_VtblAddress;
    m_mainMenuPrepState_1c8.base.vftable = kRecoilStateBase_VtblAddress;

    reinterpret_cast<zFMV_Script *>(m_introFmvState_1a0.m_fmv_08)->Cleanup();
    m_introFmvState_1a0.base.vftable = kRecoilStateBase_VtblAddress;

    reinterpret_cast<zFMV_Script *>(m_attractFmvState_160.m_fmv_10)->Cleanup();
    m_attractFmvState_160.base.vftable = kRecoilStateBase_VtblAddress;

    MfcOleModuleDestructor();
}

// Reimplements 0x42e0b0: RecoilApp::ScalarDeletingDestructor
RECOIL_NOINLINE RecoilApp *RECOIL_THISCALL
RecoilApp::ScalarDeletingDestructor(unsigned int flags) {
    Destructor();
    if ((flags & 1) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x443700: RecoilApp_StateQueueBlock::InitFromCursor
RecoilApp_StateQueueBlock *RECOIL_THISCALL
RecoilApp_StateQueueBlock::InitFromCursor(RecoilPtr32 cursor, RecoilPtr32 chunkPtrSlot) {
    RecoilPtr32 *const chunkSlot = (RecoilPtr32 *)chunkPtrSlot;
    const RecoilPtr32 chunkBegin = *chunkSlot;
    m_chunkBegin = chunkBegin;
    m_chunkEnd = chunkBegin + 0x1000;
    m_cursor = cursor;
    m_chunkPtrSlot = chunkPtrSlot;
    return this;
}

// Reimplements 0x443690: RecoilApp_StateQueue::GrowAndCenterChunkBaseList
RecoilPtr32 RECOIL_THISCALL
RecoilApp_StateQueue::GrowAndCenterChunkBaseList(int newCapacity) {
    int byteCount =
        static_cast<int>(static_cast<unsigned int>(newCapacity) << 2);
    if (byteCount < 0) {
        byteCount = 0;
    }

    RecoilPtr32 *const newList = static_cast<RecoilPtr32 *>(::operator new(static_cast<size_t>(byteCount)));
    RecoilPtr32 *const centerSlot = newList + (static_cast<unsigned int>(newCapacity) >> 2);

    const RecoilPtr32 * srcSlot = reinterpret_cast<const RecoilPtr32 *>(
        static_cast<unsigned int>(m_readBlock.m_chunkPtrSlot));
    const RecoilPtr32 *const srcEnd = reinterpret_cast<const RecoilPtr32 *>(
        static_cast<unsigned int>(m_writeBlock.m_chunkPtrSlot + 4));
    RecoilPtr32 *dstSlot = centerSlot;
    while (srcSlot != srcEnd) {
        *dstSlot = *srcSlot;
        ++srcSlot;
        ++dstSlot;
    }

    ::operator delete(reinterpret_cast<void *>(static_cast<unsigned int>(m_chunkPtrList)));
    m_chunkPtrList = static_cast<RecoilPtr32>(reinterpret_cast<unsigned int>(newList));
    m_chunkPtrCapacity = newCapacity;
    return static_cast<RecoilPtr32>(reinterpret_cast<unsigned int>(centerSlot));
}

// Reimplements 0x42de10: RecoilApp::GetMessageMap
const MfcMsgMap *RECOIL_THISCALL RecoilApp::GetMessageMap() const {
    return &g_RecoilApp_MessageMap;
}

// Reimplements 0x442c00: RecoilApp::GetMainWnd
RECOIL_NOINLINE RecoilPtr32 RECOIL_THISCALL RecoilApp::GetMainWnd() const {
    return m_pMainWnd;
}

// Reimplements 0x443140: RecoilApp::GetCurrentState
RecoilPtr32 RECOIL_THISCALL RecoilApp::GetCurrentState() const {
    if (m_currentStateIndex_0c8 < 0) {
        return 0;
    }

    if (m_currentStateIndex_0c8 >=
        static_cast<int>(sizeof(m_stateStack_0d8) / sizeof(m_stateStack_0d8[0]))) {
        return 0;
    }

    return m_stateStack_0d8[m_currentStateIndex_0c8];
}

// Reimplements 0x443160: RecoilApp::QueueSwitchCurrentState
RecoilPtr32 RECOIL_THISCALL RecoilApp::QueueSwitchCurrentState(RecoilApp_IState *state,
                                                               int stateParam) {
    const RecoilPtr32 currentState = GetCurrentState();
    const RecoilPtr32 newState = static_cast<RecoilPtr32>(reinterpret_cast<unsigned int>(state));

    RecoilApp_StateQueueItem *const item = static_cast<RecoilApp_StateQueueItem *>(::operator new(sizeof(RecoilApp_StateQueueItem)));
    RecoilPtr32 itemValue = 0;
    if (item != 0) {
        item->m_type = 0;
        item->m_kind = RecoilApp_StateQueueKind_SwitchCurrent;
        item->m_stateObj = newState;
        item->m_param = stateParam;
        itemValue = static_cast<RecoilPtr32>(reinterpret_cast<unsigned int>(item));
    }

    RecoilApp_StateQueue &queue = m_stateQueue_118;
    if (queue.m_itemCount == 0 || queue.m_writeBlock.m_cursor == queue.m_writeBlock.m_chunkEnd) {
        const RecoilPtr32 chunk =
            static_cast<RecoilPtr32>(reinterpret_cast<unsigned int>(::operator new(0x1000)));

        if (queue.m_itemCount == 0) {
            queue.m_chunkPtrCapacity = 2;
            RecoilPtr32 *const chunkList = static_cast<RecoilPtr32 *>(::operator new(2 * sizeof(RecoilPtr32)));
            queue.m_chunkPtrList =
                static_cast<RecoilPtr32>(reinterpret_cast<unsigned int>(chunkList));
            chunkList[1] = chunk;

            const RecoilPtr32 chunkSlot =
                static_cast<RecoilPtr32>(reinterpret_cast<unsigned int>(&chunkList[1]));
            const RecoilPtr32 centeredCursor = chunk + 0x800;
            queue.m_readBlock.InitFromCursor(centeredCursor, chunkSlot);
            queue.m_writeBlock.InitFromCursor(centeredCursor, chunkSlot);
        } else {
            RecoilPtr32 chunkSlot = queue.m_writeBlock.m_chunkPtrSlot;
            const RecoilPtr32 lastChunkSlot =
                queue.m_chunkPtrList + static_cast<RecoilPtr32>(queue.m_chunkPtrCapacity * 4 - 4);

            if (chunkSlot < lastChunkSlot) {
                chunkSlot += 4;
                *reinterpret_cast<RecoilPtr32 *>(static_cast<unsigned int>(chunkSlot)) = chunk;
                queue.m_writeBlock.InitFromCursor(chunk, chunkSlot);
            } else {
                const int activeChunkCount =
                    static_cast<int>(
                        (queue.m_writeBlock.m_chunkPtrSlot - queue.m_readBlock.m_chunkPtrSlot) /
                        4) +
                    1;
                const RecoilPtr32 newList = queue.GrowAndCenterChunkBaseList(activeChunkCount * 2);
                chunkSlot = newList + static_cast<RecoilPtr32>(activeChunkCount * 4);
                *reinterpret_cast<RecoilPtr32 *>(static_cast<unsigned int>(chunkSlot)) = chunk;
                queue.m_readBlock.InitFromCursor(queue.m_readBlock.m_cursor, newList);
                queue.m_writeBlock.InitFromCursor(chunk, chunkSlot);
            }
        }
    }

    const RecoilPtr32 writeCursor = queue.m_writeBlock.m_cursor;
    queue.m_writeBlock.m_cursor = writeCursor + 4;
    if (writeCursor != 0) {
        *reinterpret_cast<RecoilPtr32 *>(static_cast<unsigned int>(writeCursor)) = itemValue;
    }
    ++queue.m_itemCount;

    if (currentState != 0) {
        CallRecoilStateMethod(currentState, offsetof(RecoilApp_IState_Vtbl, OnExit));
    }
    CallRecoilStateMethod(newState, offsetof(RecoilApp_IState_Vtbl, OnEnter));

    return currentState;
}

// Reimplements 0x443310: RecoilApp::QueuePushState
RecoilPtr32 RECOIL_THISCALL RecoilApp::QueuePushState(RecoilApp_IState *state,
                                                      int suspendParam) {
    const RecoilPtr32 currentState = GetCurrentState();
    const RecoilPtr32 newState = static_cast<RecoilPtr32>(reinterpret_cast<unsigned int>(state));

    RecoilApp_StateQueueItem *const item = static_cast<RecoilApp_StateQueueItem *>(::operator new(sizeof(RecoilApp_StateQueueItem)));
    RecoilPtr32 itemValue = 0;
    if (item != 0) {
        item->m_type = 0;
        item->m_kind = RecoilApp_StateQueueKind_PushState;
        item->m_stateObj = newState;
        item->m_param = suspendParam;
        itemValue = static_cast<RecoilPtr32>(reinterpret_cast<unsigned int>(item));
    }

    RecoilApp_StateQueue &queue = m_stateQueue_118;
    if (queue.m_itemCount == 0 || queue.m_writeBlock.m_cursor == queue.m_writeBlock.m_chunkEnd) {
        const RecoilPtr32 chunk =
            static_cast<RecoilPtr32>(reinterpret_cast<unsigned int>(::operator new(0x1000)));

        if (queue.m_itemCount == 0) {
            queue.m_chunkPtrCapacity = 2;
            RecoilPtr32 *const chunkList = static_cast<RecoilPtr32 *>(::operator new(2 * sizeof(RecoilPtr32)));
            queue.m_chunkPtrList =
                static_cast<RecoilPtr32>(reinterpret_cast<unsigned int>(chunkList));
            chunkList[1] = chunk;

            const RecoilPtr32 chunkSlot =
                static_cast<RecoilPtr32>(reinterpret_cast<unsigned int>(&chunkList[1]));
            const RecoilPtr32 centeredCursor = chunk + 0x800;
            queue.m_readBlock.InitFromCursor(centeredCursor, chunkSlot);
            queue.m_writeBlock.InitFromCursor(centeredCursor, chunkSlot);
        } else {
            RecoilPtr32 chunkSlot = queue.m_writeBlock.m_chunkPtrSlot;
            const RecoilPtr32 lastChunkSlot =
                queue.m_chunkPtrList + static_cast<RecoilPtr32>(queue.m_chunkPtrCapacity * 4 - 4);

            if (chunkSlot < lastChunkSlot) {
                chunkSlot += 4;
                *reinterpret_cast<RecoilPtr32 *>(static_cast<unsigned int>(chunkSlot)) = chunk;
                queue.m_writeBlock.InitFromCursor(chunk, chunkSlot);
            } else {
                const int activeChunkCount =
                    static_cast<int>(
                        (queue.m_writeBlock.m_chunkPtrSlot - queue.m_readBlock.m_chunkPtrSlot) /
                        4) +
                    1;
                const RecoilPtr32 newList = queue.GrowAndCenterChunkBaseList(activeChunkCount * 2);
                chunkSlot = newList + static_cast<RecoilPtr32>(activeChunkCount * 4);
                *reinterpret_cast<RecoilPtr32 *>(static_cast<unsigned int>(chunkSlot)) = chunk;
                queue.m_readBlock.InitFromCursor(queue.m_readBlock.m_cursor, newList);
                queue.m_writeBlock.InitFromCursor(chunk, chunkSlot);
            }
        }
    }

    const RecoilPtr32 writeCursor = queue.m_writeBlock.m_cursor;
    queue.m_writeBlock.m_cursor = writeCursor + 4;
    if (writeCursor != 0) {
        *reinterpret_cast<RecoilPtr32 *>(static_cast<unsigned int>(writeCursor)) = itemValue;
    }
    ++queue.m_itemCount;

    CallRecoilStateMethod(newState, offsetof(RecoilApp_IState_Vtbl, OnEnter));
    return currentState;
}

// Reimplements 0x4434b0: RecoilApp::QueueExitCurrentState
RecoilPtr32 RECOIL_THISCALL RecoilApp::QueueExitCurrentState(int stateParam) {
    const RecoilPtr32 currentState = GetCurrentState();

    RecoilApp_StateQueueItem *const item = static_cast<RecoilApp_StateQueueItem *>(::operator new(sizeof(RecoilApp_StateQueueItem)));
    RecoilPtr32 itemValue = 0;
    if (item != 0) {
        item->m_type = 0;
        item->m_kind = RecoilApp_StateQueueKind_ExitCurrent;
        item->m_stateObj = 0;
        item->m_param = stateParam;
        itemValue = static_cast<RecoilPtr32>(reinterpret_cast<unsigned int>(item));
    }

    RecoilApp_StateQueue &queue = m_stateQueue_118;
    if (queue.m_itemCount == 0 || queue.m_writeBlock.m_cursor == queue.m_writeBlock.m_chunkEnd) {
        const RecoilPtr32 chunk =
            static_cast<RecoilPtr32>(reinterpret_cast<unsigned int>(::operator new(0x1000)));

        if (queue.m_itemCount == 0) {
            queue.m_chunkPtrCapacity = 2;
            RecoilPtr32 *const chunkList = static_cast<RecoilPtr32 *>(::operator new(2 * sizeof(RecoilPtr32)));
            queue.m_chunkPtrList =
                static_cast<RecoilPtr32>(reinterpret_cast<unsigned int>(chunkList));
            chunkList[1] = chunk;

            const RecoilPtr32 chunkSlot =
                static_cast<RecoilPtr32>(reinterpret_cast<unsigned int>(&chunkList[1]));
            const RecoilPtr32 centeredCursor = chunk + 0x800;
            queue.m_readBlock.InitFromCursor(centeredCursor, chunkSlot);
            queue.m_writeBlock.InitFromCursor(centeredCursor, chunkSlot);
        } else {
            RecoilPtr32 chunkSlot = queue.m_writeBlock.m_chunkPtrSlot;
            const RecoilPtr32 lastChunkSlot =
                queue.m_chunkPtrList + static_cast<RecoilPtr32>(queue.m_chunkPtrCapacity * 4 - 4);

            if (chunkSlot < lastChunkSlot) {
                chunkSlot += 4;
                *reinterpret_cast<RecoilPtr32 *>(static_cast<unsigned int>(chunkSlot)) = chunk;
                queue.m_writeBlock.InitFromCursor(chunk, chunkSlot);
            } else {
                const int activeChunkCount =
                    static_cast<int>(
                        (queue.m_writeBlock.m_chunkPtrSlot - queue.m_readBlock.m_chunkPtrSlot) /
                        4) +
                    1;
                const RecoilPtr32 newList = queue.GrowAndCenterChunkBaseList(activeChunkCount * 2);
                chunkSlot = newList + static_cast<RecoilPtr32>(activeChunkCount * 4);
                *reinterpret_cast<RecoilPtr32 *>(static_cast<unsigned int>(chunkSlot)) = chunk;
                queue.m_readBlock.InitFromCursor(queue.m_readBlock.m_cursor, newList);
                queue.m_writeBlock.InitFromCursor(chunk, chunkSlot);
            }
        }
    }

    const RecoilPtr32 writeCursor = queue.m_writeBlock.m_cursor;
    queue.m_writeBlock.m_cursor = writeCursor + 4;
    if (writeCursor != 0) {
        *reinterpret_cast<RecoilPtr32 *>(static_cast<unsigned int>(writeCursor)) = itemValue;
    }
    ++queue.m_itemCount;

    if (currentState != 0) {
        CallRecoilStateMethod(currentState, offsetof(RecoilApp_IState_Vtbl, OnExit));
    }

    return currentState;
}

// Reimplements 0x442c10: RecoilApp::StartEngineAndQueueStartupState
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp::StartEngineAndQueueStartupState() {
    const RecoilPtr32 appVtable = vftable;
    const RecoilPtr32 mainWnd = GetMainWnd();
    const RecoilPtr32 hwnd =
        *reinterpret_cast<const RecoilPtr32 *>(static_cast<unsigned int>(mainWnd + 0x20));

    RecoilAppStartEngineMethod const startEngine = reinterpret_cast<RecoilAppStartEngineMethod>(
        static_cast<unsigned int>(ReadRecoilVtableSlot(appVtable, 0xac)));
    if (startEngine(this, hwnd) == 0) {
        RecoilAppNoArgVoidMethod const shutdownEngine = reinterpret_cast<RecoilAppNoArgVoidMethod>(
            static_cast<unsigned int>(ReadRecoilVtableSlot(vftable, 0xb0)));
        shutdownEngine(this);

        RecoilAppNoArgIntMethod const exitInstance = reinterpret_cast<RecoilAppNoArgIntMethod>(
            static_cast<unsigned int>(ReadRecoilVtableSlot(vftable, 0x70)));
        return exitInstance(this);
    }

    RecoilApp_IState *const startupState =
        reinterpret_cast<RecoilApp_IState *>(static_cast<unsigned int>(m_pendingState_0c4));
    m_skipWait_0d0 = 1;
    m_reserved0d4 = 0;
    QueueSwitchCurrentState(startupState, 0);
    return 1;
}

// Reimplements 0x443650: RecoilApp::OnIdleOrDispatch
int RECOIL_THISCALL RecoilApp::OnIdleOrDispatch(unsigned int wParam,
                                                         unsigned int lParam) {
    const RecoilPtr32 currentState = GetCurrentState();
    zSndCd::OnMciNotify(wParam, lParam);
    if (currentState == 0) {
        return 0;
    }

    const RecoilFn32 methodValue = ReadRecoilVtableSlot(
        *reinterpret_cast<const RecoilPtr32 *>(static_cast<unsigned int>(currentState)),
        offsetof(RecoilApp_IState_Vtbl, OnIdleOrDispatch));
    RecoilStateIdleMethod const method = reinterpret_cast<RecoilStateIdleMethod>(static_cast<unsigned int>(methodValue));
    return method(reinterpret_cast<RecoilApp_IState *>(static_cast<unsigned int>(currentState)),
                  wParam, lParam);
}

// Reimplements 0x442a10: RecoilApp::TakeSkipWaitMessage
int RECOIL_THISCALL RecoilApp::TakeSkipWaitMessage() {
    const int wasSkipped = m_skipWait_0d0;
    m_skipWait_0d0 = 0;
    return wasSkipped;
}

// Reimplements 0x442a30: RecoilApp::MarkSkipWaitMessage
int RECOIL_THISCALL RecoilApp::MarkSkipWaitMessage() {
    const int wasSkipped = m_skipWait_0d0;
    m_skipWait_0d0 = 1;
    return wasSkipped;
}

// Reimplements 0x442c70: RecoilApp::MfcOleModuleConstructor
RECOIL_NOINLINE RecoilApp *RECOIL_THISCALL RecoilApp::MfcOleModuleConstructor() {
    new (this) CWinApp(0);

    const unsigned int selfValue = reinterpret_cast<unsigned int>(this);
    unsigned char *const ctorTagBytes = reinterpret_cast<unsigned char *>(&m_stateQueue_118.m_ctorTag_00);
    ctorTagBytes[0] = static_cast<unsigned char>((selfValue >> 24) & 0xff);

    memset(&m_stateQueue_118.m_readBlock, 0, 0x2c);
    m_skipWait_0d0 = 0;
    m_pendingState_0c4 = 0;
    vftable = kRecoilApp_MfcOleModule_VtblAddress;
    m_currentStateIndex_0c8 = -1;
    memset(m_stateStack_0d8, 0, sizeof(m_stateStack_0d8));
    return this;
}

// Reimplements 0x4428b0: RecoilApp::MfcOleModuleDestructor
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp::MfcOleModuleDestructor() {
    vftable = kRecoilApp_MfcOleModule_VtblAddress;

    while (m_stateQueue_118.m_itemCount != 0) {
        m_stateQueue_118.m_readBlock.m_cursor += 4;
        --m_stateQueue_118.m_itemCount;

        if (m_stateQueue_118.m_itemCount == 0 ||
            m_stateQueue_118.m_readBlock.m_cursor == m_stateQueue_118.m_readBlock.m_chunkEnd) {
            const RecoilPtr32 oldChunkSlot = m_stateQueue_118.m_readBlock.m_chunkPtrSlot;
            m_stateQueue_118.m_readBlock.m_chunkPtrSlot = oldChunkSlot + 4;

            const RecoilPtr32 chunk =
                *reinterpret_cast<RecoilPtr32 *>(static_cast<unsigned int>(oldChunkSlot));
            ::operator delete(reinterpret_cast<void *>(static_cast<unsigned int>(chunk)));

            if (m_stateQueue_118.m_itemCount == 0) {
                memset(&m_stateQueue_118.m_readBlock, 0, sizeof(m_stateQueue_118.m_readBlock));
                memset(&m_stateQueue_118.m_writeBlock, 0,
                            sizeof(m_stateQueue_118.m_writeBlock));
                ::operator delete(reinterpret_cast<void *>(
                    static_cast<unsigned int>(m_stateQueue_118.m_chunkPtrList)));
            } else {
                const RecoilPtr32 chunkSlot = m_stateQueue_118.m_readBlock.m_chunkPtrSlot;
                const RecoilPtr32 nextChunk =
                    *reinterpret_cast<RecoilPtr32 *>(static_cast<unsigned int>(chunkSlot));
                m_stateQueue_118.m_readBlock.m_chunkBegin = nextChunk;
                m_stateQueue_118.m_readBlock.m_chunkEnd = nextChunk + 0x1000;
                m_stateQueue_118.m_readBlock.m_cursor = nextChunk;
                m_stateQueue_118.m_readBlock.m_chunkPtrSlot = chunkSlot;
            }
        }
    }

    reinterpret_cast<CWinApp *>(this)->CWinApp::~CWinApp();
}

// Reimplements 0x4429b0: RecoilApp::MfcOleModuleScalarDeletingDestructor
RecoilApp *RECOIL_THISCALL RecoilApp::MfcOleModuleScalarDeletingDestructor(unsigned int flags) {
    MfcOleModuleDestructor();
    if ((flags & 1) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x42eea0: RecoilApp_PlayState::Constructor
RECOIL_NOINLINE RecoilApp_PlayState *RECOIL_THISCALL RecoilApp_PlayState::Constructor() {
    base.vftable = kRecoilApp_PlayState_VtblAddress;
    m_transitionScratch_10 = 0;
    pPendingLoadGameStartPath = 0;
    return this;
}

// Reimplements 0x42eec0: RecoilApp_PlayState::OnWndActivate
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp_PlayState::OnWndActivate(int bActivate) {
    if (bActivate != 0) {
        HudUiMgr::TriggerCurrentLayoutOnActivated();
    }
}

// Reimplements 0x42f8a0: RecoilApp_PlayState::OnResume
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp_PlayState::OnResume(int) {
    if (zSnd::GetCDAudioOption() != 0) {
        const int missionId = g_HudSensorTracker.GetMissionId();
        const int trackCount = zSndCd::GetTrackCount();
        zSndCd::PlayTrackWithMode((missionId % (trackCount - 2)) + 2, 5);
    }
}

// Reimplements 0x42f9d0: RecoilApp_LeaveNetworkState::OnTryBecomeCurrent
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp_LeaveNetworkState::OnTryBecomeCurrent() {
    zNetwork_DPlay_DestroyCachedLocalPlayer();
    g_RecoilApp.ShutdownEngine();
    zSndBackend::Shutdown();
    return 1;
}

// Reimplements 0x42eb70: RecoilApp_AttractFmvState::Constructor
RECOIL_NOINLINE RecoilApp_AttractFmvState *RECOIL_THISCALL
RecoilApp_AttractFmvState::Constructor() {
    base.vftable = kRecoilStateBase_VtblAddress;
    reinterpret_cast<zFMV_Script *>(m_fmv_10)->Init(0, 0, 0);
    base.vftable = kRecoilApp_AttractFmvState_VtblAddress;
    return this;
}

RECOIL_NOINLINE RecoilApp_IntroFmvState *RECOIL_THISCALL RecoilApp_IntroFmvState::Constructor() {
    base.vftable = kRecoilStateBase_VtblAddress;
    reinterpret_cast<zFMV_Script *>(m_fmv_08)->Init(0, 0, 0);
    base.vftable = kRecoilApp_IntroFmvState_VtblAddress;
    return this;
}

// Reimplements 0x42ea20: RecoilApp_IntroFmvState::OnTryBecomeCurrent
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp_IntroFmvState::OnTryBecomeCurrent() {
    const int pitchBytes = zVideo::GetPrimarySurfacePitch();
    const int bitsPerPixel = zOpt::GetDisplaySectionBitsPerPixel();
    zOpt_ViewRectSection *const windowSection = zOpt::GetWindowSection();
    zRndr::SetFrameBufferRegion(zVideo::GetPrimarySurfacePixels(), windowSection, bitsPerPixel,
                                pitchBytes);
    zRndr::SetVideoStrideMirrors(zOpt::GetVideoStrideValue());

    const int fxPitchBytes = zVideo::GetPrimarySurfacePitch();
    const int surfaceHeight = zVideo::GetPrimarySurfaceHeight();
    const int surfaceWidth = zVideo::GetPrimarySurfaceWidth();
    zVideo::Fx_SetSurfaceState(zVideo::GetPrimarySurfacePixels(), surfaceWidth, surfaceHeight,
                               fxPitchBytes);
    zVid::SetCachedClientRectUpdateMask(1);

    if (g_RecoilApp.m_skipIntroFmv == 0) {
        zFMV_Script *const script = reinterpret_cast<zFMV_Script *>(m_fmv_08);
        if (g_RecoilApp_hWndMain != 0) {
            script->m_hWnd = g_RecoilApp_hWndMain;
        }

        if (script->LoadActionsFromZrd("fmv.zrd", "INTRO") != -1) {
            script->BeginAtTime();
        }
    }

    return 1;
}

// Reimplements 0x42eac0: RecoilApp_IntroFmvState::OnUpdateShouldQuit
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp_IntroFmvState::OnUpdateShouldQuit() {
    if (g_RecoilApp.m_skipIntroFmv != 0) {
        g_RecoilApp.QueueSwitchCurrentState(&g_RecoilApp.m_missionFmvState_1d8.base, 0);
        return 0;
    }

    zFMV_Script *const script = reinterpret_cast<zFMV_Script *>(m_fmv_08);
    const int stateParam = script->UpdateAtTime();
    if (stateParam == 0) {
        g_RecoilApp.QueueSwitchCurrentState(&g_RecoilApp.m_mainMenuPrepState_1c8.base, stateParam);
    }

    return 0;
}

// Reimplements 0x42eb00: RecoilApp_FmvState::OnIdleOrDispatch
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp_FmvState::OnIdleOrDispatch(unsigned int,
                                                                                  unsigned int) {
    return 1;
}

// Reimplements 0x42eb10: RecoilApp_IntroFmvState::OnDeactivate
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp_IntroFmvState::OnDeactivate() {
    reinterpret_cast<zFMV_Script *>(m_fmv_08)->BeginNow(1);
}

// Reimplements 0x42eb20: RecoilApp_MainMenuPrepState::OnTryBecomeCurrent
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp_MainMenuPrepState::OnTryBecomeCurrent() {
    const int pitchBytes = zVideo::GetPrimarySurfacePitch();
    const int height = zVideo::GetPrimarySurfaceHeight();
    const int width = zVideo::GetPrimarySurfaceWidth();
    zVideo::Fx_SetSurfaceState(zVideo::GetPrimarySurfacePixels(), width, height, pitchBytes);
    m_stateData04 = 0;
    return 1;
}

// Reimplements 0x42eb60: RecoilApp_MainMenuPrepState::OnUpdateShouldQuit
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp_MainMenuPrepState::OnUpdateShouldQuit() {
    RecoilStateMainMenuTransition::QueueEnter(RECOIL_MAINMENU_ROUTE_FRONTEND);
    return 0;
}

// Reimplements 0x42ebf0: RecoilApp_AttractFmvState::OnTryBecomeCurrent
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp_AttractFmvState::OnTryBecomeCurrent() {
    const int pitchBytes = zVideo::GetPrimarySurfacePitch();
    const int height = zVideo::GetPrimarySurfaceHeight();
    const int width = zVideo::GetPrimarySurfaceWidth();
    zVideo::Fx_SetSurfaceState(zVideo::GetPrimarySurfacePixels(), width, height, pitchBytes);

    GetClientRect(g_RecoilApp_hWndMain, reinterpret_cast<RECT *>(m_clientRect_30));

    zFMV_Script *const script = reinterpret_cast<zFMV_Script *>(m_fmv_10);
    if (g_RecoilApp_AttractFmvReloadMode != 0) {
        script->LoadActionsFromZrd("fmv.zrd", "ATTRACT");
        g_RecoilApp_AttractFmvReloadMode = 0;
    }

    if (g_RecoilApp_hWndMain != 0) {
        script->m_hWnd = g_RecoilApp_hWndMain;
    }

    if (script->LoadActionsFromZrd("fmv.zrd", "ATTRACT") != -1) {
        script->BeginAtTime();
    }

    return 1;
}

// Reimplements 0x42ec80: RecoilApp_AttractFmvState::OnUpdateShouldQuit
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp_AttractFmvState::OnUpdateShouldQuit() {
    zFMV_Script *const script = reinterpret_cast<zFMV_Script *>(m_fmv_10);
    const int stateParam = script->UpdateAtTime();
    if (stateParam == 0) {
        g_RecoilApp.QueueSwitchCurrentState(&g_RecoilApp.m_mainMenuPrepState_1c8.base, stateParam);
    }

    return 0;
}

// Reimplements 0x42eca0: RecoilApp_AttractFmvState::OnDeactivate
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp_AttractFmvState::OnDeactivate() {
    reinterpret_cast<zFMV_Script *>(m_fmv_10)->BeginNow(0);
}

// Reimplements 0x42ed30: RecoilApp_MissionFmvState::Constructor
RECOIL_NOINLINE RecoilApp_MissionFmvState *RECOIL_THISCALL
RecoilApp_MissionFmvState::Constructor() {
    base.vftable = kRecoilStateBase_VtblAddress;
    reinterpret_cast<zFMV_Script *>(m_fmv_08)->Init(0, 0, 0);
    base.vftable = kRecoilApp_MissionFmvState_VtblAddress;
    m_missionId = 0;
    m_skipMissionFmv = 0;
    return this;
}

// Reimplements 0x42df90: RecoilApp_IState::Destructor
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp_IState::Destructor() {
    vftable = kRecoilStateBase_VtblAddress;
}

// Reimplements 0x42e0f0: RecoilApp_IState::ScalarDeletingDestructor
RecoilApp_IState *RECOIL_THISCALL RecoilApp_IState::ScalarDeletingDestructor(unsigned int flags) {
    Destructor();
    if ((flags & 1) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x42df10: RecoilApp_AttractFmvState::Destructor
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp_AttractFmvState::Destructor() {
    reinterpret_cast<zFMV_Script *>(m_fmv_10)->Cleanup();
    base.vftable = kRecoilStateBase_VtblAddress;
}

// Reimplements 0x42df50: RecoilApp_IntroFmvState::Destructor
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp_IntroFmvState::Destructor() {
    reinterpret_cast<zFMV_Script *>(m_fmv_08)->Cleanup();
    base.vftable = kRecoilStateBase_VtblAddress;
}

// Reimplements 0x42ebd0: RecoilApp_AttractFmvState::ScalarDeletingDestructor
RecoilApp_AttractFmvState *RECOIL_THISCALL
RecoilApp_AttractFmvState::ScalarDeletingDestructor(unsigned int flags) {
    Destructor();
    if ((flags & 1) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x42e0d0: RecoilApp_IntroFmvState::ScalarDeletingDestructor
RecoilApp_IntroFmvState *RECOIL_THISCALL
RecoilApp_IntroFmvState::ScalarDeletingDestructor(unsigned int flags) {
    Destructor();
    if ((flags & 1) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x42e070: RecoilApp_MissionFmvState::Destructor
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp_MissionFmvState::Destructor() {
    reinterpret_cast<zFMV_Script *>(m_fmv_08)->Cleanup();
    base.vftable = kRecoilStateBase_VtblAddress;
}

// Reimplements 0x42ed90: RecoilApp_MissionFmvState::ScalarDeletingDestructor
RecoilApp_MissionFmvState *RECOIL_THISCALL
RecoilApp_MissionFmvState::ScalarDeletingDestructor(unsigned int flags) {
    Destructor();
    if ((flags & 1) != 0) {
        ::operator delete(this);
    }

    return this;
}
