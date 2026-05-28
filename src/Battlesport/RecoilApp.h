#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include "recoil/recoil_callconv.h"

typedef unsigned int RecoilPtr32;
typedef unsigned int RecoilFn32;

const RecoilPtr32 kRecoilApp_VtblAddress = 0x004d09e0;
const RecoilPtr32 kRecoilApp_MpExitDialogState_VtblAddress = 0x004d0aa0;
const RecoilPtr32 kRecoilApp_LeaveNetworkState_VtblAddress = 0x004d0ac8;
const RecoilPtr32 kRecoilApp_MainMenuPrepState_VtblAddress = 0x004d0af0;
const RecoilPtr32 kRecoilApp_IntroFmvState_VtblAddress = 0x004d0b18;
const RecoilPtr32 kRecoilApp_AttractFmvState_VtblAddress = 0x004d0b40;
const RecoilPtr32 kRecoilApp_MissionFmvState_VtblAddress = 0x004d0b68;
const RecoilPtr32 kRecoilApp_PlayState_VtblAddress = 0x004d0b90;
const RecoilPtr32 kRecoilStateBase_VtblAddress = 0x004ccd50;
const RecoilPtr32 kRecoilApp_MfcOleModule_VtblAddress = 0x004d2020;

// Partial 32-bit mirror of the app-owned state framework recovered in Binary Ninja.
// Pointers and vtable slots stay as 32-bit scalars so the original PE offsets remain
// valid even though the rebuild currently targets x64.
enum RecoilApp_StateQueueKind {
    RecoilApp_StateQueueKind_ExitCurrent = 1,
    RecoilApp_StateQueueKind_PushState = 2,
    RecoilApp_StateQueueKind_SwitchCurrent = 3,
};

enum RecoilAppMissionShutdownMode {
    RECOILAPP_MISSION_SHUTDOWN_ON_EXIT = 0,
    RECOILAPP_MISSION_SHUTDOWN_SKIP_GAMEPLAY = 1,
};

struct RecoilApp_IState_Vtbl {
    RecoilFn32 ScalarDeletingDtor;
    RecoilFn32 OnWndActivate;
    RecoilFn32 OnEnter;
    RecoilFn32 OnCanBecomeCurrent;
    RecoilFn32 OnUpdateShouldQuit;
    RecoilFn32 OnExit;
    RecoilFn32 OnDeactivate;
    RecoilFn32 OnSuspend;
    RecoilFn32 OnResume;
    RecoilFn32 OnIdleOrDispatch;
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_IState_Vtbl) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(RecoilApp_IState_Vtbl, OnCanBecomeCurrent) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(RecoilApp_IState_Vtbl, OnUpdateShouldQuit) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(RecoilApp_IState_Vtbl, OnExit) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(RecoilApp_IState_Vtbl, OnDeactivate) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(RecoilApp_IState_Vtbl, OnSuspend) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(RecoilApp_IState_Vtbl, OnResume) == 0x20);

struct RecoilApp_IState {
    RecoilPtr32 vftable; // RecoilApp_IState_Vtbl*

    RECOIL_NOINLINE void RECOIL_THISCALL Destructor();
    RecoilApp_IState *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_IState) == 0x04);

struct RecoilApp_StateQueueItem {
    unsigned int m_type;
    RecoilApp_StateQueueKind m_kind;
    RecoilPtr32 m_stateObj; // RecoilApp_IState*
    int m_param;
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_StateQueueItem) == 0x10);

struct RecoilApp_StateQueueBlock {
    RecoilPtr32 m_chunkBegin;   // RecoilApp_StateQueueItem**
    RecoilPtr32 m_chunkEnd;     // RecoilApp_StateQueueItem**
    RecoilPtr32 m_cursor;       // RecoilApp_StateQueueItem**
    RecoilPtr32 m_chunkPtrSlot; // RecoilApp_StateQueueItem***

    RecoilApp_StateQueueBlock *RECOIL_THISCALL InitFromCursor(RecoilPtr32 cursor,
                                                              RecoilPtr32 chunkPtrSlot);
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_StateQueueBlock) == 0x10);

struct RecoilApp_StateQueue {
    int m_ctorTag_00;
    RecoilApp_StateQueueBlock m_readBlock;
    RecoilApp_StateQueueBlock m_writeBlock;
    RecoilPtr32 m_chunkPtrList; // RecoilApp_StateQueueItem***
    int m_chunkPtrCapacity;
    int m_itemCount;

    RecoilPtr32 RECOIL_THISCALL GrowAndCenterChunkBaseList(int newCapacity);
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_StateQueue) == 0x30);

struct zMfcMsgMapEntry {
    int nMessage;
    int nCode;
    int nID;
    int nLastID;
    int nSig;
    RecoilFn32 pfn;
};
RECOIL_STATIC_ASSERT(sizeof(zMfcMsgMapEntry) == 0x18);

struct MfcMsgMap {
    RecoilFn32 getBaseMap;
    RecoilPtr32 entries; // zMfcMsgMapEntry*
};
RECOIL_STATIC_ASSERT(sizeof(MfcMsgMap) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(MfcMsgMap, entries) == 0x04);

struct RecoilApp_AttractFmvState {
    RecoilApp_IState base;
    int _pad_04[3];
    int m_fmv_10[8]; // zFMV_Script pending mirror in src/GameZRecoil/zFMV.
    int m_clientRect_30[4];

    RECOIL_NOINLINE RecoilApp_AttractFmvState *RECOIL_THISCALL Constructor();
    RECOIL_NOINLINE void RECOIL_THISCALL Destructor();
    RECOIL_NOINLINE int RECOIL_THISCALL OnTryBecomeCurrent();
    RECOIL_NOINLINE int RECOIL_THISCALL OnUpdateShouldQuit();
    RECOIL_NOINLINE void RECOIL_THISCALL OnDeactivate();
    RecoilApp_AttractFmvState *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_AttractFmvState) == 0x40);

struct RecoilApp_FmvState {
    RECOIL_NOINLINE int RECOIL_THISCALL OnIdleOrDispatch(unsigned int wParam,
                                                                  unsigned int lParam);
};

struct RecoilApp_IntroFmvState {
    RecoilApp_IState base;
    int m_stateData04;
    int m_fmv_08[8]; // zFMV_Script pending mirror in src/GameZRecoil/zFMV.

    RECOIL_NOINLINE RecoilApp_IntroFmvState *RECOIL_THISCALL Constructor();
    RECOIL_NOINLINE void RECOIL_THISCALL Destructor();
    RECOIL_NOINLINE int RECOIL_THISCALL OnTryBecomeCurrent();
    RECOIL_NOINLINE int RECOIL_THISCALL OnUpdateShouldQuit();
    RECOIL_NOINLINE void RECOIL_THISCALL OnDeactivate();
    RecoilApp_IntroFmvState *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_IntroFmvState) == 0x28);

struct RecoilApp_MainMenuPrepState {
    RecoilApp_IState base;
    int m_stateData04;

    RECOIL_NOINLINE int RECOIL_THISCALL OnTryBecomeCurrent();
    RECOIL_NOINLINE int RECOIL_THISCALL OnUpdateShouldQuit();
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_MainMenuPrepState) == 0x08);

struct RecoilApp_LeaveNetworkState {
    RecoilApp_IState base;
    int m_stateData04;

    RECOIL_NOINLINE int RECOIL_THISCALL OnTryBecomeCurrent();
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_LeaveNetworkState) == 0x08);

struct RecoilApp_MissionFmvState {
    RecoilApp_IState base;
    int m_missionId;
    int m_fmv_08[8]; // zFMV_Script pending mirror in src/GameZRecoil/zFMV.
    int m_skipMissionFmv;
    int m_reserved2c;

    RECOIL_NOINLINE RecoilApp_MissionFmvState *RECOIL_THISCALL Constructor();
    RECOIL_NOINLINE void RECOIL_THISCALL Destructor();
    RecoilApp_MissionFmvState *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_MissionFmvState) == 0x30);

struct RecoilApp_PlayState {
    RecoilApp_IState base;
    RecoilPtr32 pWindowSection; // zOpt_ViewRectSection*
    RecoilPtr32 pDisplaySection;
    RecoilPtr32 pRenderSection;
    int m_transitionScratch_10;
    RecoilPtr32 pPendingLoadGameStartPath; // char*

    RECOIL_NOINLINE RecoilApp_PlayState *RECOIL_THISCALL Constructor();
    RECOIL_NOINLINE void RECOIL_THISCALL OnWndActivate(int bActivate);
    RECOIL_NOINLINE int RECOIL_THISCALL OnTryBecomeCurrent();
    RECOIL_NOINLINE int RECOIL_THISCALL TickAndRenderFrame(int shouldPresent);
    RECOIL_NOINLINE int RECOIL_THISCALL OnUpdateShouldQuit();
    RECOIL_NOINLINE void RECOIL_THISCALL OnDeactivate();
    RECOIL_NOINLINE void RECOIL_THISCALL OnResume(int param);
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_PlayState) == 0x18);

struct RecoilApp_MpExitDialogState {
    RecoilApp_IState base;
    int m_stateData04;
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_MpExitDialogState) == 0x08);

struct CZRecoilFrame;
struct tagMSG;

struct RecoilApp {
    RecoilPtr32 vftable; // RecoilApp_Vtbl*
    int _pad_04[7];
    RecoilPtr32 m_pMainWnd; // CZRecoilFrame*
    int _pad_24[2];
    RecoilPtr32 m_mainThreadHandle_2c;
    int _pad_30;
    int m_msg_34[7]; // tagMSG raw storage.
    int _pad_50[7];
    RecoilPtr32 m_hInstance_6c;
    int _pad_70[21];
    RecoilPtr32 m_pendingState_0c4; // RecoilApp_IState*
    int m_currentStateIndex_0c8;
    int _pad_0cc;
    int m_skipWait_0d0;
    RecoilAppMissionShutdownMode m_missionShutdownMode;
    RecoilPtr32 m_stateStack_0d8[16]; // RecoilApp_IState*
    RecoilApp_StateQueue m_stateQueue_118;
    int m_reserved148;
    int m_skipIntroFmv;
    float m_transitionFadeTimer150;
    int _pad_154[3];
    RecoilApp_AttractFmvState m_attractFmvState_160;
    RecoilApp_IntroFmvState m_introFmvState_1a0;
    RecoilApp_MainMenuPrepState m_mainMenuPrepState_1c8;
    RecoilApp_LeaveNetworkState m_leaveNetworkState_1d0;
    RecoilApp_MissionFmvState m_missionFmvState_1d8;
    RecoilApp_PlayState m_playState_208;
    RecoilApp_MpExitDialogState m_mpExitDialogState_220;

    RECOIL_NOINLINE static void RECOIL_CDECL StaticInitAndRegisterAtExit();
    RECOIL_NOINLINE static RecoilApp *RECOIL_CDECL StaticInit();
    RECOIL_NOINLINE static void RECOIL_CDECL RegisterAtExit();
    RECOIL_NOINLINE static void RECOIL_CDECL AtExitDestructor();
    RECOIL_NOINLINE RECOIL_NO_GS static void RECOIL_FASTCALL InitStdLogFiles(const char *exePath);
    RECOIL_NOINLINE RECOIL_NO_GS static void RECOIL_FASTCALL
    FatalErrorAndExit(int errorCode);

    RECOIL_NOINLINE RecoilApp *RECOIL_THISCALL Constructor();
    RECOIL_NOINLINE void RECOIL_THISCALL Destructor();
    RECOIL_NOINLINE RecoilApp *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
    RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_THISCALL InitInstance();
    RECOIL_NOINLINE int RECOIL_THISCALL Run();
    RECOIL_NOINLINE int RECOIL_THISCALL ExitInstance();
    RECOIL_NOINLINE CZRecoilFrame *RECOIL_THISCALL CreateMainWnd();
    RECOIL_NOINLINE int RECOIL_THISCALL InitMainWindow();
    RECOIL_NOINLINE int RECOIL_THISCALL EngineInit(RecoilPtr32 hwnd);
    RECOIL_NOINLINE static int RECOIL_FASTCALL InitializeDisplay(RecoilPtr32 hwnd);
    RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_THISCALL StartEngine(RecoilPtr32 hwnd);
    RECOIL_NOINLINE int RECOIL_THISCALL ActivateExistingInstance();
    RECOIL_NOINLINE void RECOIL_THISCALL ShutdownEngine();
    RECOIL_NOINLINE int RECOIL_THISCALL LoadZbdAndStartEngine();
    RECOIL_NOINLINE int RECOIL_THISCALL
    LoadZbdAndSetupSensorTracker(int missionId, const char *zbdPath,
                                 int skipIntroFmvMode, int missionFlags);
    RECOIL_NOINLINE static int RECOIL_CDECL ShutdownSubsystems();
    RECOIL_NOINLINE RecoilApp *RECOIL_THISCALL MfcOleModuleConstructor();
    RECOIL_NOINLINE void RECOIL_THISCALL MfcOleModuleDestructor();
    RecoilApp *RECOIL_THISCALL MfcOleModuleScalarDeletingDestructor(unsigned int flags);
    RecoilPtr32 RECOIL_THISCALL QueuePushState(RecoilApp_IState *state, int suspendParam);
    RecoilPtr32 RECOIL_THISCALL QueueSwitchCurrentState(RecoilApp_IState *state,
                                                        int stateParam);
    RecoilPtr32 RECOIL_THISCALL QueueExitCurrentState(int stateParam);
    RECOIL_NOINLINE int RECOIL_THISCALL StartEngineAndQueueStartupState();
    int RECOIL_THISCALL OnIdleOrDispatch(unsigned int wParam, unsigned int lParam);
    RECOIL_NOINLINE int RECOIL_THISCALL PreTranslateMessage(tagMSG *msg);
    const MfcMsgMap *RECOIL_THISCALL GetMessageMap() const;
    RECOIL_NOINLINE RecoilPtr32 RECOIL_THISCALL GetMainWnd() const;
    RecoilPtr32 RECOIL_THISCALL GetCurrentState() const;
    int RECOIL_THISCALL TakeSkipWaitMessage();
    int RECOIL_THISCALL MarkSkipWaitMessage();
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp) == 0x228);
RECOIL_STATIC_ASSERT(offsetof(RecoilApp, m_hInstance_6c) == 0x06c);
RECOIL_STATIC_ASSERT(offsetof(RecoilApp, m_pendingState_0c4) == 0x0c4);
RECOIL_STATIC_ASSERT(offsetof(RecoilApp, m_currentStateIndex_0c8) == 0x0c8);
RECOIL_STATIC_ASSERT(offsetof(RecoilApp, m_stateStack_0d8) == 0x0d8);
RECOIL_STATIC_ASSERT(offsetof(RecoilApp, m_stateQueue_118) == 0x118);
RECOIL_STATIC_ASSERT(offsetof(RecoilApp, m_skipIntroFmv) == 0x14c);
RECOIL_STATIC_ASSERT(offsetof(RecoilApp, m_transitionFadeTimer150) == 0x150);
RECOIL_STATIC_ASSERT(offsetof(RecoilApp, m_attractFmvState_160) == 0x160);
RECOIL_STATIC_ASSERT(offsetof(RecoilApp, m_mpExitDialogState_220) == 0x220);

extern RecoilApp g_RecoilApp;
extern const zMfcMsgMapEntry g_RecoilApp_MessageEntries[1];
extern const MfcMsgMap g_RecoilApp_MessageMap;
