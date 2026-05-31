#pragma once

#include <stddef.h>

#include "Battlesport/Mfc42Abi.h"
#include "recoil/recoil_callconv.h"
#include "recoil/recoil_types.h"

#include <unknwn.h>

struct WestwoodOnlineUpgradeBootstrapServerRecord
{
    int m_gameType;
    unsigned char reserved004[0x10];
    WestwoodOnlineUpgradeBootstrapServerRecord *m_next;
    char m_serverName[0x47];
    char m_serverType[0x05];
    char m_connectData[0x80];
    char m_playerName[0x0a];
    char m_connectString[0x0a];
    unsigned char reserved0f8[0x0c];
};

struct WestwoodOnlineUpgradeBrowseRecord
{
    int m_recordFlags;
    unsigned char reserved004[0x30];
    char m_sessionName[0xa3];
    char m_serverAddress[0x35];
};

struct WestwoodOnlineUpgradeConnectContext
{
    unsigned char reserved000[0x24];
    char m_requestText[0x34];
};

struct WestwoodOnlineUpgradeSessionRequest
{
    int m_rowFlags;
    unsigned char reserved004[0x1c];
    WestwoodOnlineUpgradeSessionRequest *m_next;
    char m_sessionName[0x34];
};

struct WestwoodOnlineUpgradeQueryRequest
{
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

struct WestwoodOnlineUpgradeApiInitState
{
    unsigned int structSize;
    HINSTANCE moduleHandlePrimary;
    HINSTANCE moduleHandleSecondary;
    HINSTANCE moduleHandleTertiary;
    HANDLE bootstrapServerListEvent;
    HANDLE statusTextEvent;
    HANDLE failureEvent;
    CRITICAL_SECTION criticalSection0;
    CRITICAL_SECTION criticalSection1;
    CRITICAL_SECTION criticalSection2;

    RECOIL_NOINLINE static HRESULT RECOIL_STDCALL
    Init(WestwoodOnlineUpgradeApiInitState *self,
         HANDLE bootstrapServerListEvent,
         HINSTANCE moduleHandle);
};

struct WestwoodOnlineUpgradeApi
{
    RECOIL_NOINLINE static int RECOIL_CDECL Init();
    RECOIL_NOINLINE int RECOIL_THISCALL
    CreateInstanceAndLoadConfig(HANDLE bootstrapServerListEvent);
    RECOIL_NOINLINE static void RECOIL_CDECL Shutdown();
};

void RECOIL_CDECL AfxEnableControlContainer(COccManager *manager);

extern "C" WestwoodOnlineUpgradeApiInitState g_WestwoodOnlineUpgradeApiInitState;
extern "C" IUnknown *g_pWestwoodOnlineUpgradeApi;
extern "C" void *g_pWestwoodOnlineUpgradeApiEventSink;
extern "C" DWORD g_WestwoodOnlineUpgradeApiConnectionCookie;
extern "C" int g_WestwoodOnlineUpgradeApiEventSinkConnectionOffset;
extern "C" int g_WestwoodOnlineUpgradeApiReadyFlag;
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
extern "C" WestwoodOnlineUpgradeBrowseRecord
    g_WestwoodOnlineUpgradeCachedBrowseRecordList[1024];
extern "C" HANDLE g_WestwoodOnlineUpgradeCloseHandle0;
extern "C" HANDLE g_WestwoodOnlineUpgradeCloseHandle1;
extern "C" HANDLE g_WestwoodOnlineUpgradeCloseHandle2;
extern const CLSID g_WestwoodOnlineUpgradeApi_CLSID;
extern const IID g_WestwoodOnlineUpgradeApi_IID;
extern const IID g_WestwoodOnlineUpgradeApiEventSink_IID;

RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeApiInitState) == 0x64);
RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeBootstrapServerRecord) == 0x104);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeBootstrapServerRecord, m_gameType) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeBootstrapServerRecord, m_next) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeBootstrapServerRecord, m_serverName) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeBootstrapServerRecord, m_serverType) == 0x5f);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeBootstrapServerRecord, m_connectData) == 0x64);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeBootstrapServerRecord, m_playerName) == 0xe4);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeBootstrapServerRecord, m_connectString) == 0xee);
RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeBrowseRecord) == 0x10c);
RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeConnectContext) == 0x58);
RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeSessionRequest) == 0x58);
RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeQueryRequest) == 0x10c);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeBrowseRecord, m_recordFlags) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeBrowseRecord, m_sessionName) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeBrowseRecord, m_serverAddress) == 0xd7);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeConnectContext, m_requestText) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeSessionRequest, m_rowFlags) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeSessionRequest, m_next) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeSessionRequest, m_sessionName) ==
                     0x24);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeQueryRequest, m_sessionName) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeQueryRequest, m_serverAddress) == 0xd7);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeApiInitState, structSize) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeApiInitState, moduleHandlePrimary) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeApiInitState, moduleHandleSecondary) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeApiInitState, moduleHandleTertiary) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeApiInitState, bootstrapServerListEvent) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeApiInitState, statusTextEvent) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeApiInitState, failureEvent) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeApiInitState, criticalSection0) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeApiInitState, criticalSection1) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeApiInitState, criticalSection2) == 0x4c);
