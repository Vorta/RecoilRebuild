#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "Battlesport/Mfc42Abi.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "recoil/recoil_callconv.h"

typedef recoil::Ptr32 RecoilPtr32;
typedef recoil::Fn32 RecoilFn32;

RECOIL_FORCEINLINE RecoilPtr32 RecoilSymbolPtr32(
    const void *symbol
) {
    return (RecoilPtr32)((unsigned int)(symbol));
}

// App-owned state transition request queued by RecoilApp::Run.
enum RecoilApp_StateQueueKind {
    RecoilApp_StateQueueKind_ExitCurrent = 1,
    RecoilApp_StateQueueKind_PushState = 2,
    RecoilApp_StateQueueKind_SwitchCurrent = 3,
};

enum RecoilAppMissionShutdownMode {
    RECOILAPP_MISSION_SHUTDOWN_ON_EXIT = 0,
    RECOILAPP_MISSION_SHUTDOWN_SKIP_GAMEPLAY = 1,
};

// Compatibility table for app-state wrappers that have not yet been promoted
// to ordinary virtual classes. The RecoilApp owner no longer uses these tables.
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

extern RecoilApp_IState_Vtbl g_RecoilStateBase_Vtbl;

// Authored app-state interface. Retail evidence shows constructor-owned state
// objects with a common vptr at offset zero and lifecycle calls through that
// table; the source model is a VC-era virtual interface, not copied table data.
struct RecoilApp_IState {
    virtual ~RecoilApp_IState();
    virtual void OnWndActivate(int activateCode);
    virtual void OnEnter();
    virtual int OnTryBecomeCurrent();
    virtual int OnUpdateShouldQuit();
    virtual void OnExit();
    virtual void OnDeactivate();
    virtual void OnSuspend(int suspendParam);
    virtual void OnResume(int resumeParam);
    virtual int OnIdleOrDispatch(
        unsigned int wParam,
        unsigned int lParam
    );
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_IState) == 0x04);

struct RecoilApp_StateQueueItem {
    unsigned int m_type;
    RecoilApp_StateQueueKind m_kind;
    RecoilApp_IState *m_stateObj;
    int m_param;
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_StateQueueItem) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilApp_StateQueueItem,
        m_kind
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilApp_StateQueueItem,
        m_stateObj
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilApp_StateQueueItem,
        m_param
    ) == 0x0c
);

// VC5 deque-shaped app-state queue recovered from the queueing functions.
// Retail stores 0x1000-byte chunks and a centered chunk-base list; queued
// items themselves are consumed and deleted by RecoilApp::Run.
struct RecoilApp_StateQueueBlock {
    RecoilApp_StateQueueItem **m_chunkBegin;
    RecoilApp_StateQueueItem **m_chunkEnd;
    RecoilApp_StateQueueItem **m_cursor;
    RecoilApp_StateQueueItem ***m_chunkBaseSlot;

    RecoilApp_StateQueueBlock * InitFromCursor(
        RecoilApp_StateQueueItem **cursor,
        RecoilApp_StateQueueItem ***chunkBaseSlot
    );
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_StateQueueBlock) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilApp_StateQueueBlock,
        m_cursor
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilApp_StateQueueBlock,
        m_chunkBaseSlot
    ) == 0x0c
);

struct RecoilApp_StateQueue {
    int m_allocatorPad;
    RecoilApp_StateQueueBlock m_readBlock;
    RecoilApp_StateQueueBlock m_writeBlock;
    RecoilApp_StateQueueItem ***m_chunkBaseList;
    int m_chunkBaseCapacity;
    int m_itemCount;

    RecoilApp_StateQueueItem *** GrowAndCenterChunkBaseList(
        int newCapacity
    );
    RECOIL_FORCEINLINE bool Empty() const;
    RECOIL_FORCEINLINE RecoilApp_StateQueueItem *Front() const;
    RECOIL_FORCEINLINE void PopFront();
    RECOIL_FORCEINLINE void PushBack(RecoilApp_StateQueueItem *const &item);
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_StateQueue) == 0x30);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilApp_StateQueue,
        m_readBlock
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilApp_StateQueue,
        m_writeBlock
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilApp_StateQueue,
        m_chunkBaseList
    ) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilApp_StateQueue,
        m_itemCount
    ) == 0x2c
);

struct RecoilApp_FmvState : RecoilApp_IState {
    int OnIdleOrDispatch(
        unsigned int wParam,
        unsigned int lParam
    );
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_FmvState) == 0x04);

// Embedded FMV script member used by RecoilApp states. Retail state
// constructors initialize this subobject before installing the final state vptr.
struct RecoilApp_FmvScript : zFMV_Script {
    RecoilApp_FmvScript() {
        Init(
            0,
            0,
            0
        );
    }
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_FmvScript) == 0x20);

struct RecoilApp_AttractFmvState : RecoilApp_FmvState {
    int m_reservedForFmvScriptAlign[3];
    RecoilApp_FmvScript m_fmv; // Embedded 0x20-byte FMV script subobject at retail offset 0x10.
    int m_clientRect[4];

    RecoilApp_AttractFmvState();
    ~RecoilApp_AttractFmvState();
    int OnTryBecomeCurrent();
    int OnUpdateShouldQuit();
    void OnDeactivate();
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_AttractFmvState) == 0x40);

struct RecoilApp_IntroFmvState : RecoilApp_FmvState {
    int m_stateData04;
    RecoilApp_FmvScript m_fmv; // Embedded 0x20-byte FMV script subobject at retail offset 0x08.

    RecoilApp_IntroFmvState();
    ~RecoilApp_IntroFmvState();
    int OnTryBecomeCurrent();
    int OnUpdateShouldQuit();
    void OnDeactivate();
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_IntroFmvState) == 0x28);

struct RecoilApp_MainMenuPrepState : RecoilApp_IState {
    int m_stateData04;

    int OnTryBecomeCurrent();
    int OnUpdateShouldQuit();
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_MainMenuPrepState) == 0x08);

struct RecoilApp_LeaveNetworkState : RecoilApp_IState {
    int m_stateData04;

    int OnTryBecomeCurrent();
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_LeaveNetworkState) == 0x08);

struct RecoilApp_MissionFmvState : RecoilApp_FmvState {
    int m_missionId;
    RecoilApp_FmvScript m_fmv; // Embedded 0x20-byte FMV script subobject at retail offset 0x08.
    int m_skipMissionFmv;
    int m_reserved2c;

    RecoilApp_MissionFmvState();
    ~RecoilApp_MissionFmvState();
    int OnTryBecomeCurrent();
    void OnDeactivate();
    int OnUpdateShouldQuit();
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_MissionFmvState) == 0x30);

struct RecoilApp_PlayState : RecoilApp_IState {
    void *pWindowSection; // zOpt_ViewRectSection*
    void *pDisplaySection;
    void *pRenderSection;
    int m_transitionScratch;
    char *pPendingLoadGameStartPath;

    RecoilApp_PlayState();
    void OnWndActivate(int bActivate);
    int OnTryBecomeCurrent();
    int TickAndRenderFrame(int shouldPresent);
    int OnUpdateShouldQuit();
    void OnDeactivate();
    void OnResume(int param);
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_PlayState) == 0x18);

struct RecoilApp_MpExitDialogState : RecoilApp_IState {
    int m_stateData04;

    void OnEnter();
    int OnTryBecomeCurrent();
    void OnDeactivate();
    int OnUpdateShouldQuit();
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_MpExitDialogState) == 0x08);

struct CZRecoilFrame;
struct tagMSG;

// Authored MFC app shell recovered from the message map, constructor,
// destructor, and run-state host. MFC base behavior stays provider-owned.
class RecoilApp : public CWinApp {
  public:
#if !defined(_AFXDLL)
    int m_recoilPad;
#endif
    RecoilApp_IState *m_pendingState;
    int m_currentStateIndex;
    int m_stateHostReserved;
    int m_skipWait;
    RecoilAppMissionShutdownMode m_missionShutdownMode;
    RecoilApp_IState *m_stateStack[16];
    RecoilApp_StateQueue m_stateQueue;
    int m_reserved148;
    int m_skipIntroFmv;
    float m_transitionFadeTimer;
    int m_transitionReserved[3];
    RecoilApp_AttractFmvState m_attractFmvState;
    RecoilApp_IntroFmvState m_introFmvState;
    RecoilApp_MainMenuPrepState m_mainMenuPrepState;
    RecoilApp_LeaveNetworkState m_leaveNetworkState;
    RecoilApp_MissionFmvState m_missionFmvState;
    RecoilApp_PlayState m_playState;
    RecoilApp_MpExitDialogState m_mpExitDialogState;

    RecoilApp();
    ~RecoilApp();
    RECOIL_NO_GS static void __fastcall InitStdLogFiles(const char *exePath);
    RECOIL_NO_GS static void __fastcall FatalErrorAndExit(int errorCode);

    RECOIL_NO_GS int InitInstance();
    int Run();
    int ExitInstance();
    virtual void OnAppActivate();
    virtual void OnAppDeactivate();
    RECOIL_NO_GS virtual int StartEngine(HWND hwnd);
    virtual void ShutdownEngine();
    virtual int OnIdleOrDispatch(
        unsigned int wParam,
        unsigned int lParam
    );
    virtual CZRecoilFrame * CreateMainWnd();
    int InitMainWindow();
    int EngineInit(HWND hwnd);
    static int __fastcall InitializeDisplay(HWND hwnd);
    int ActivateExistingInstance();
    int LoadZbdAndStartEngine();
    int LoadZbdAndSetupSensorTracker(
        int missionId,
        const char *zbdPath,
        int skipIntroFmvMode,
        int missionFlags
    );
    void ShutdownSubsystems();
    RecoilApp_IState * QueuePushState(
        RecoilApp_IState *state,
        int suspendParam
    );
    RecoilApp_IState * QueueSwitchCurrentState(
        RecoilApp_IState *state,
        int stateParam
    );
    RecoilApp_IState * QueueExitCurrentState(int stateParam);
    int StartEngineAndQueueStartupState();
    int PreTranslateMessage(tagMSG *msg);
    static const AFX_MSGMAP *__stdcall GetBaseMessageMapForMfc();
    const AFX_MSGMAP * GetMessageMap() const;
    CZRecoilFrame * GetMainWnd() const;
    RecoilApp_IState * GetCurrentState() const;
    int TakeSkipWaitMessage();
    int MarkSkipWaitMessage();
};
#if defined(_MSC_VER) && _MSC_VER < 1300 && defined(_M_IX86)
RECOIL_STATIC_ASSERT(sizeof(RecoilApp) == 0x228);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilApp,
        m_pendingState
    ) == 0x0c4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilApp,
        m_currentStateIndex
    ) == 0x0c8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilApp,
        m_stateStack
    ) == 0x0d8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilApp,
        m_stateQueue
    ) == 0x118
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilApp,
        m_skipIntroFmv
    ) == 0x14c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilApp,
        m_transitionFadeTimer
    ) == 0x150
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilApp,
        m_attractFmvState
    ) == 0x160
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilApp,
        m_mpExitDialogState
    ) == 0x220
);
#endif

extern RecoilApp g_RecoilApp;
extern const AFX_MSGMAP_ENTRY g_RecoilApp_MessageEntries[1];
extern const AFX_MSGMAP g_RecoilApp_MessageMap;
