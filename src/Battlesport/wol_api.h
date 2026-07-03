#pragma once

#include <stddef.h>

#include "Battlesport/Mfc42Abi.h"
#include "recoil/recoil_callconv.h"
#include "recoil/recoil_types.h"

#include <unknwn.h>

struct WestwoodOnlineUpgradeBootstrapServerRecord {
    int m_gameType;
    unsigned char reserved004[0x10];
    WestwoodOnlineUpgradeBootstrapServerRecord *m_next;
    char m_serverName[0x47];
    char m_serverType[0x05];
    char m_connectData[0x80];
    char m_playerName[0x0a];
    char m_connectString[0x0a];
};

struct WestwoodOnlineUpgradeBrowseRecord {
    int m_recordFlags;
    unsigned char reserved004[0x04];
    int m_displayMetric0;
    int m_displayMetric1;
    unsigned char reserved010[0x18];
    int m_latencyMs;
    unsigned char reserved02c[0x04];
    WestwoodOnlineUpgradeBrowseRecord *m_next;
    char m_sessionName[0xa3];
    char m_serverAddress[0x35];
};

struct WestwoodOnlineUpgradeConnectContext {
    unsigned char reserved000[0x24];
    char m_requestText[0x34];
};

struct WestwoodOnlineUpgradeSessionRequest {
    int m_rowFlags;
    unsigned char reserved004[0x14];
    unsigned int m_hostIpv4Packed;
    unsigned char reserved01c[0x04];
    WestwoodOnlineUpgradeSessionRequest *m_next;
    char m_sessionName[0x34];
};

struct WestwoodOnlineUpgradeQueryRequest {
    int m_listMode;
    int m_queryVariant;
    int m_queryMaxPlayers;
    unsigned char reserved00c[0x04];
    int m_queryFlags;
    int m_queryExtraParam;
    unsigned char reserved018[0x1c];
    char m_sessionName[0xa3];
    char m_serverAddress[0x35];
};

/**
 * Westwood Online ActiveX provider boundary. The source keeps call-site views
 * typed by observed slot order without modeling provider storage or tables.
 */
struct IWestwoodOnlineUpgradeProviderApi : IUnknown {
    virtual void STDMETHODCALLTYPE ProcessCallbacks() = 0;
    virtual void STDMETHODCALLTYPE BeginConnect(
        int languageId,
        int productId,
        const char *playerName,
        const char *connectString,
        int timeoutSeconds
    ) = 0;
    virtual void STDMETHODCALLTYPE RequestBootstrapServerList(
        WestwoodOnlineUpgradeBootstrapServerRecord *selectedBootstrapServer,
        int timeoutSeconds,
        int useAlternateConnectString
    ) = 0;
    virtual void STDMETHODCALLTYPE RequestListMode(
        int listMode,
        int enabled
    ) = 0;
    virtual int STDMETHODCALLTYPE SubmitQueryRequest(
        WestwoodOnlineUpgradeQueryRequest *request
    ) = 0;
    virtual int STDMETHODCALLTYPE LoadBrowseRecord(
        WestwoodOnlineUpgradeBrowseRecord *record
    ) = 0;
    virtual void STDMETHODCALLTYPE ResetQueryState() = 0;
    virtual void STDMETHODCALLTYPE Reserved28() = 0;
    virtual void STDMETHODCALLTYPE SubmitStatusText(
        const char *statusText
    ) = 0;
    virtual void STDMETHODCALLTYPE SubmitSessionRequestListAndStatusText(
        WestwoodOnlineUpgradeSessionRequest *sessionRequestList,
        const char *statusText
    ) = 0;
    virtual void STDMETHODCALLTYPE Disconnect() = 0;
    virtual void STDMETHODCALLTYPE Reserved38() = 0;
    virtual void STDMETHODCALLTYPE SubmitEncodedQueryString(
        const char *encodedQuery
    ) = 0;
    virtual void STDMETHODCALLTYPE Reserved40() = 0;
    virtual void STDMETHODCALLTYPE Reserved44() = 0;
    virtual void STDMETHODCALLTYPE SubmitPendingSessionList(
        WestwoodOnlineUpgradeSessionRequest *sessionRequestList
    ) = 0;
    virtual void STDMETHODCALLTYPE Reserved4c() = 0;
    virtual void STDMETHODCALLTYPE Reserved50() = 0;
    virtual void STDMETHODCALLTYPE QueueSessionRequest(
        WestwoodOnlineUpgradeSessionRequest *request
    ) = 0;
    virtual void STDMETHODCALLTYPE Reserved58() = 0;
    virtual void STDMETHODCALLTYPE Reserved5c() = 0;
    virtual int STDMETHODCALLTYPE RequestUpgradeDownloadReadyResult(
        WestwoodOnlineUpgradeConnectContext *context
    ) = 0;
    virtual int STDMETHODCALLTYPE QueryStatusWithTokenAndServer(
        WestwoodOnlineUpgradeConnectContext *context,
        const char *serverText
    ) = 0;
    virtual void STDMETHODCALLTYPE Reserved68() = 0;
    virtual void STDMETHODCALLTYPE BeginConnectWithPreparedContext(
        WestwoodOnlineUpgradeConnectContext *context,
        int mode
    ) = 0;
    virtual int STDMETHODCALLTYPE PrepareConnectContextAndMode(
        WestwoodOnlineUpgradeConnectContext *context
    ) = 0;
    virtual void STDMETHODCALLTYPE Reserved74() = 0;
    virtual void STDMETHODCALLTYPE Reserved78() = 0;
    virtual void STDMETHODCALLTYPE Reserved7c() = 0;
    virtual void STDMETHODCALLTYPE LookupBrowseRecordBySessionName(
        const char *sessionName,
        int lookupMode
    ) = 0;
    virtual void STDMETHODCALLTYPE Reserved84() = 0;
    virtual void STDMETHODCALLTYPE Reserved88() = 0;
    virtual void STDMETHODCALLTYPE Reserved8c() = 0;
    virtual int STDMETHODCALLTYPE LoadConnectProfileStrings(
        int profileId,
        char **playerNameOut,
        char **connectStringOut
    ) = 0;
    virtual int STDMETHODCALLTYPE SaveConnectProfileStrings(
        int profileId,
        const char *playerName,
        const char *connectString,
        int connectStringMode
    ) = 0;
};

struct IWestwoodOnlineUpgradeProviderApiCallbacks : IUnknown {
    virtual void STDMETHODCALLTYPE Reserved0c() = 0;
    virtual void STDMETHODCALLTYPE Reserved10() = 0;
    virtual void STDMETHODCALLTYPE Reserved14() = 0;
    virtual void STDMETHODCALLTYPE RequestListMode(
        int listMode,
        int enabled
    ) = 0;
    virtual void STDMETHODCALLTYPE Reserved1c() = 0;
    virtual void STDMETHODCALLTYPE Reserved20() = 0;
    virtual void STDMETHODCALLTYPE Reserved24() = 0;
    virtual void STDMETHODCALLTYPE CancelPendingSessionFlow() = 0;
    virtual void STDMETHODCALLTYPE Reserved2c() = 0;
    virtual void STDMETHODCALLTYPE Reserved30() = 0;
    virtual void STDMETHODCALLTYPE Disconnect() = 0;
    virtual void STDMETHODCALLTYPE Reserved38() = 0;
    virtual void STDMETHODCALLTYPE Reserved3c() = 0;
    virtual void STDMETHODCALLTYPE Reserved40() = 0;
    virtual void STDMETHODCALLTYPE Reserved44() = 0;
    virtual void STDMETHODCALLTYPE Reserved48() = 0;
    virtual void STDMETHODCALLTYPE Reserved4c() = 0;
    virtual void STDMETHODCALLTYPE Reserved50() = 0;
    virtual void STDMETHODCALLTYPE Reserved54() = 0;
    virtual void STDMETHODCALLTYPE RequestSessionDetails(
        WestwoodOnlineUpgradeSessionRequest *sessionRequest
    ) = 0;
    virtual void STDMETHODCALLTYPE Reserved5c() = 0;
    virtual void STDMETHODCALLTYPE Reserved60() = 0;
    virtual void STDMETHODCALLTYPE Reserved64() = 0;
    virtual void STDMETHODCALLTYPE Reserved68() = 0;
    virtual void STDMETHODCALLTYPE Reserved6c() = 0;
    virtual void STDMETHODCALLTYPE Reserved70() = 0;
    virtual void STDMETHODCALLTYPE SetQueryMode(
        int listMode
    ) = 0;
    virtual void STDMETHODCALLTYPE Reserved78() = 0;
    virtual void STDMETHODCALLTYPE Reserved7c() = 0;
    virtual void STDMETHODCALLTYPE Reserved80() = 0;
    virtual void STDMETHODCALLTYPE Reserved84() = 0;
    virtual void STDMETHODCALLTYPE Reserved88() = 0;
    virtual void STDMETHODCALLTYPE Reserved8c() = 0;
    virtual void STDMETHODCALLTYPE Reserved90() = 0;
    virtual void STDMETHODCALLTYPE Reserved94() = 0;
    virtual void STDMETHODCALLTYPE GetQueryResultCount(
        int *outCount
    ) = 0;
};

struct WestwoodOnlineUpgradeApiInitState {
    unsigned int structSize;
    HINSTANCE moduleHandlePrimary;
    HINSTANCE moduleHandleSecondary;
    HINSTANCE moduleHandleTertiary;
    HANDLE bootstrapServerListEvent;
    LONG eventSinkLiveCount;
    HANDLE failureEvent;
    CRITICAL_SECTION criticalSection0;
    CRITICAL_SECTION criticalSection1;
    CRITICAL_SECTION criticalSection2;

    static HRESULT __stdcall Init(
        WestwoodOnlineUpgradeApiInitState *self,
        HANDLE bootstrapServerListEvent,
        HINSTANCE moduleHandle
    );
};

struct WestwoodOnlineUpgradeApi {
    static int Init();
    int CreateInstanceAndLoadConfig(
        HANDLE bootstrapServerListEvent
    );
    static void Shutdown();
};

void AfxEnableControlContainer(COccManager *manager);

extern "C" WestwoodOnlineUpgradeApiInitState g_WestwoodOnlineUpgradeApiInitState;
extern "C" IUnknown *g_pWestwoodOnlineUpgradeApi;
extern "C" void *g_pWestwoodOnlineUpgradeApiEventSink;
extern "C" DWORD g_WestwoodOnlineUpgradeApiAdviseCookie;
extern "C" int g_WestwoodOnlineUpgradeApiShutdownState;
extern "C" int g_WestwoodOnlineUpgradeApiAsyncErrorFlag;
extern "C" int g_WestwoodOnlineUpgradeAbortFlag;
extern "C" int g_WestwoodOnlineUpgradeActiveListMode;
extern "C" int g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag;
extern "C" int g_WestwoodOnlineUpgradePendingSessionResultCount;
extern "C" int g_WestwoodOnlineUpgradeVisibleSessionResultCount;
extern "C" int g_WestwoodOnlineUpgradeProcessCallbacksFlag;
extern "C" float g_WestwoodOnlineUpgradeNextAutoRefreshTime;
extern "C" int g_WestwoodOnlineUpgradeDisconnectInFlightFlag;
extern "C" HANDLE g_WestwoodOnlineUpgradeInitWaitEvents[3];
extern "C" HANDLE g_WestwoodOnlineUpgradeBootstrapServerListEvent;
extern "C" HANDLE g_WestwoodOnlineUpgradeStatusTextEvent;
extern "C" HANDLE g_WestwoodOnlineUpgradeFailureEvent;
extern "C" WestwoodOnlineUpgradeBootstrapServerRecord
    g_WestwoodOnlineUpgradeSelectedBootstrapServer;
extern "C" WestwoodOnlineUpgradeBrowseRecord g_WestwoodOnlineUpgradeCachedBrowseRecord;
extern "C" WestwoodOnlineUpgradeBrowseRecord g_WestwoodOnlineUpgradeCachedBrowseRecordList[1024];
extern "C" int g_WestwoodOnlineUpgradeCachedBrowseRecordListCount;
extern const CLSID g_WestwoodOnlineUpgradeApi_CLSID;
extern const IID g_WestwoodOnlineUpgradeApi_IID;
extern const IID g_WestwoodOnlineUpgradeApiEventSink_IID;

RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeApiInitState) == 0x64);
RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeBootstrapServerRecord) == 0xf8);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeBootstrapServerRecord,
        m_gameType
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeBootstrapServerRecord,
        m_next
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeBootstrapServerRecord,
        m_serverName
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeBootstrapServerRecord,
        m_serverType
    ) == 0x5f
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeBootstrapServerRecord,
        m_connectData
    ) == 0x64
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeBootstrapServerRecord,
        m_playerName
    ) == 0xe4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeBootstrapServerRecord,
        m_connectString
    ) == 0xee
);
RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeBrowseRecord) == 0x10c);
RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeConnectContext) == 0x58);
RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeSessionRequest) == 0x58);
RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeQueryRequest) == 0x10c);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeBrowseRecord,
        m_recordFlags
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeBrowseRecord,
        m_displayMetric0
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeBrowseRecord,
        m_displayMetric1
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeBrowseRecord,
        m_latencyMs
    ) == 0x28
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeBrowseRecord,
        m_next
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeBrowseRecord,
        m_sessionName
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeBrowseRecord,
        m_serverAddress
    ) == 0xd7
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeConnectContext,
        m_requestText
    ) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeSessionRequest,
        m_rowFlags
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeSessionRequest,
        m_hostIpv4Packed
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeSessionRequest,
        m_next
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeSessionRequest,
        m_sessionName
    ) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeQueryRequest,
        m_sessionName
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeQueryRequest,
        m_serverAddress
    ) == 0xd7
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiInitState,
        structSize
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiInitState,
        moduleHandlePrimary
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiInitState,
        moduleHandleSecondary
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiInitState,
        moduleHandleTertiary
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiInitState,
        bootstrapServerListEvent
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiInitState,
        eventSinkLiveCount
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiInitState,
        failureEvent
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiInitState,
        criticalSection0
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiInitState,
        criticalSection1
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiInitState,
        criticalSection2
    ) == 0x4c
);
