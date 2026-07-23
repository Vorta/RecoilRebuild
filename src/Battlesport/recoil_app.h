#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "recoil/Mfc42Abi.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "recoil/recoil_callconv.h"

#if defined(RECOILAPP_VC5_STL_STATE_QUEUE_MEMBER) && defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86)
#include <deque>
#endif

typedef recoil::Ptr32 RecoilPtr32;

struct RecoilStateBase;

extern "C" {
extern const char g_HudSensorTracker_StartAnimsZrdPath[0x0e];
extern const char g_RecoilApp_LoadGameStartAnimStateName[0x10];
extern const char g_RecoilApp_NewGameStartAnimStateName[0x0f];
extern const char g_RecoilApp_CommonSoundsSampleSetName[0x06];
extern const char g_RecoilApp_LoadingCommonSoundsMsg[0x15];
}

/**
 * App-owned state transition request queued by RecoilApp_MfcOleModule::Run.
 */
enum RecoilApp_StateQueueKind {
    RecoilApp_StateQueueKind_ExitCurrent = 1,
    RecoilApp_StateQueueKind_PushState = 2,
    RecoilApp_StateQueueKind_SwitchCurrent = 3,
};

enum RecoilAppMissionShutdownMode {
    RECOILAPP_MISSION_SHUTDOWN_ON_EXIT = 0,
    RECOILAPP_MISSION_SHUTDOWN_SKIP_GAMEPLAY = 1,
};

/**
 * Authored app-state interface. Retail evidence shows constructor-owned state
 * objects with a common vptr at offset zero and lifecycle calls through that
 * table; the source model is a VC-era virtual interface, not copied table data.
 * Emits 0x42e0f0: VC5 compiler-generated scalar deleting-destructor contribution anchored to this complete type definition; not an authored body.
 */
struct RecoilApp_IState {
    /**
     * Reimplements 0x42df90: RecoilApp_IState::~RecoilApp_IState.
     * Purpose: Tear down the common app-state interface base.
     */
    virtual ~RecoilApp_IState();
    virtual void OnWndActivate(int activateCode);
    virtual void OnEnter();
    virtual int OnTryBecomeCurrent();
    virtual int OnUpdateShouldQuit();
    virtual void OnExit();
    virtual void OnDeactivate();
    virtual void OnSuspend(int suspendParam);
    virtual void OnResume(int resumeParam);
    /**
     * Original helper: default state hook with no standalone retail function address.
     * Purpose: lets default states keep the idle/dispatch loop alive.
     */
    virtual int OnIdleOrDispatch(
        unsigned int,
        unsigned int
    ) {
        return 1;
    }
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_IState) == 0x04);

struct RecoilApp_StateQueueItem {
    unsigned int m_type;
    RecoilApp_StateQueueKind m_kind;
    RecoilApp_IState *m_stateObj;
    int m_param;

    /**
     * Original inline helper; no standalone retail function exists.
     * Observed in queue entrypoint callers 0x443160, 0x443310, and 0x4434b0.
     *
     * Purpose: initialize one queued app-state transition request.
     */
    RecoilApp_StateQueueItem(
        RecoilApp_StateQueueKind kind,
        RecoilApp_IState *stateObj,
        int param
    ) : m_type(0),
        m_kind(kind),
        m_stateObj(stateObj),
        m_param(param) {
    }
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

/**
 * VC5 deque-shaped app-state queue recovered from the queueing functions.
 * Retail stores 0x1000-byte chunks and a centered chunk-base list; queued
 * items themselves are consumed and deleted by RecoilApp_MfcOleModule::Run.
 */
enum {
    kRecoilAppStateQueueChunkSlotCount = 1024,
    kRecoilAppStateQueueInitialCursorOffset =
        kRecoilAppStateQueueChunkSlotCount / 2,
    kRecoilAppStateQueueInitialChunkBaseCapacity = 2
};

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

#if defined(RECOILAPP_VC5_STL_STATE_QUEUE_MEMBER) && defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86)
struct RecoilApp_StateQueue : std::deque<RecoilApp_StateQueueItem *> {
    inline bool Empty() const;
    inline RecoilApp_StateQueueItem *Front() const;
    inline void PopFront();
    inline void PushBack(RecoilApp_StateQueueItem *const &item);
};
#else
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
    inline bool Empty() const;
    inline RecoilApp_StateQueueItem *Front() const;
    inline void PopFront();
    inline void PushBack(RecoilApp_StateQueueItem *const &item);
};
#endif
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_StateQueue) == 0x30);
#if !(defined(RECOILAPP_VC5_STL_STATE_QUEUE_MEMBER) && defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86))
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
#endif

struct RecoilApp_FmvState : RecoilApp_IState {
    int OnIdleOrDispatch(
        unsigned int wParam,
        unsigned int lParam
    );
};
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_FmvState) == 0x04);

/**
 * Embedded FMV script member used by RecoilApp states. Retail state
 * constructors initialize this subobject before installing the final state vptr.
 */
struct RecoilApp_FmvScript : zFMV_Script {
    /**
     * No standalone retail function; original inline helper observed in callers
     * 0x42eb70 and 0x42ed30 through RecoilApp FMV state construction.
     *
     * Purpose: initialize the embedded zFMV_Script member to its empty script
     * state before the owning RecoilApp state installs its final vptr.
     */
    RecoilApp_FmvScript() {
        Init(
            0,
            0,
            0
        );
    }

    /**
     * Original inline helper; no standalone retail function exists.
     * Observed in RecoilApp FMV-state destructor cleanup at 0x42df10,
     * 0x42df50, and 0x42e070.
     *
     * Purpose: clean up the embedded zFMV_Script member when an FMV state is destroyed.
     */
    ~RecoilApp_FmvScript() {
        Cleanup();
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

    /**
     * No standalone retail function; original inline helper observed in caller
     * 0x42dfa0, where VC5 emits the base-state vptr store and embedded FMV
     * script initialization directly in RecoilApp construction.
     *
     * Purpose: construct the intro FMV state as an embedded RecoilApp member.
     */
    RecoilApp_IntroFmvState() {
    }
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
    void OnDeactivate();
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
    struct zOpt_ViewRectSection *pWindowSection;
    struct zOpt_ViewRectSection *pDisplaySection;
    struct zOpt_ViewRectSection *pRenderSection;
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

/**
 * Authored MFC/OLE app shell recovered from the constructor/destructor pair at
 * 0x442c70/0x4428b0. MFC base behavior stays provider-owned.
 */
class RecoilApp_MfcOleModule : public CWinApp {
  public:
    static const AFX_MSGMAP messageMap;
    static const AFX_MSGMAP_ENTRY messageEntries[];
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

    RecoilApp_MfcOleModule();
    virtual ~RecoilApp_MfcOleModule();
    virtual int InitInstance();
    virtual int Run();
    static const AFX_MSGMAP *__stdcall GetBaseMessageMapForMfc();
    virtual const AFX_MSGMAP * GetMessageMap() const;
};

/**
 * Authored MFC app shell recovered from the message map, constructor,
 * destructor, and run-state host.
 */
class RecoilApp : public RecoilApp_MfcOleModule {
  public:
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
    virtual ~RecoilApp();
    RECOIL_NO_GS static void __fastcall InitStdLogFiles(const char *exePath);
    RECOIL_NO_GS static void __fastcall FatalErrorAndExit(int errorCode);

    RECOIL_NO_GS virtual int InitInstance();
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
RECOIL_STATIC_ASSERT(sizeof(RecoilApp_MfcOleModule) == 0x14c);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilApp_MfcOleModule,
        m_pendingState
    ) == 0x0c4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilApp_MfcOleModule,
        m_currentStateIndex
    ) == 0x0c8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilApp_MfcOleModule,
        m_stateStack
    ) == 0x0d8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilApp_MfcOleModule,
        m_stateQueue
    ) == 0x118
);
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

union RecoilAppStorage {
    unsigned long align;
    unsigned char bytes[sizeof(RecoilApp)];
};
RECOIL_STATIC_ASSERT(sizeof(RecoilAppStorage) == 0x228);

extern RecoilAppStorage g_RecoilApp;
#define g_RecoilApp \
    (*(RecoilApp *)&g_RecoilApp)
extern const AFX_MSGMAP_ENTRY g_RecoilApp_MessageEntries[1];
extern const AFX_MSGMAP g_RecoilApp_MessageMap;
