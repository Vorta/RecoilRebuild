#pragma once

#include <stddef.h>

#include "Battlesport/Mfc42Abi.h"
#include "Battlesport/WestwoodOnlineUpgradeRefCountAndLock.h"
#include "recoil/recoil_callconv.h"
#include "recoil/recoil_types.h"

struct WestwoodOnlineUpgradeDownloadReadyEntry;
struct WestwoodOnlineUpgradeBrowseRecord;
struct WestwoodOnlineUpgradeSessionRequest;

struct WestwoodOnlineUpgradeApiEventSinkVtable {
    void *slots[32];
};

struct WestwoodOnlineUpgradeApiEventSink {
    WestwoodOnlineUpgradeApiEventSinkVtable *m_vftable;
    WestwoodOnlineUpgradeRefCountAndLock m_refCountAndLock;

    RECOIL_NOINLINE static HRESULT RECOIL_STDCALL CreateInstance(
        WestwoodOnlineUpgradeApiEventSink **outSink
    );
    RECOIL_NOINLINE static HRESULT RECOIL_STDCALL QueryInterface(
        WestwoodOnlineUpgradeApiEventSink *self,
        REFIID iid,
        void **outInterface
    );
    RECOIL_NOINLINE static ULONG RECOIL_STDCALL Release(WestwoodOnlineUpgradeApiEventSink *self);
    RECOIL_NOINLINE void RECOIL_THISCALL Destructor();
    RECOIL_NOINLINE static int RECOIL_STDCALL OnDownloadReadyResult(
        void *callbackContext,
        int resultCode,
        WestwoodOnlineUpgradeDownloadReadyEntry *downloadReadyList
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL OnPendingSessionRequestRemoved(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL OnServerError(
        void *callbackContext,
        int status,
        const char *errorText
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL OnApiStatus(
        void *callbackContext,
        int statusCode,
        const char *statusText
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL OnStatusTextReceived(
        void *callbackContext,
        int status,
        const char *statusText
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL OnBrowseRecordAdded(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecord
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL OnBrowseRecordAndSessionResolved(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecord,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL OnSessionQueryFinished(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecord,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL OnSessionLaunchResult(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecord,
        WestwoodOnlineUpgradeSessionRequest *sessionNode,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL OnSessionListEnumerated(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecord,
        WestwoodOnlineUpgradeSessionRequest *sessionList
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL LaunchSelectedSession(
        void *callbackContext,
        int status,
        int reserved,
        WestwoodOnlineUpgradeSessionRequest *selectedSessionList,
        int reserved2
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL ApplyEncodedQueryString0(
        void *callbackContext,
        int status,
        int reserved,
        char *encodedQuery
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL ApplyEncodedQueryString1(
        void *callbackContext,
        int status,
        int reserved,
        int reserved2,
        char *encodedQuery
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL AppendSessionRequestStatus301B(
        void *callbackContext,
        int status,
        int reserved,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest,
        const char *statusText
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL AppendSessionRequestStatus301C(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest,
        const char *statusText
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL AppendSessionRequestStatus301C_Alt0(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest,
        int value
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL AppendSessionRequestStatus301C_Alt1(
        void *callbackContext,
        int status,
        int reserved,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest,
        int value
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL CallbackNoOp0(
        void *callbackContext,
        int reserved0,
        int reserved1,
        int reserved2
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL CallbackNoOp1(
        void *callbackContext,
        int reserved0,
        int reserved1
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL AppendTimeStatus302A(
        void *callbackContext,
        int status,
        long unixTime
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL AppendValueStatus302B_302C(
        void *callbackContext,
        int reserved,
        int value,
        int usePrimaryMessage
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL UpdateSessionResultItemFlags(
        void *callbackContext,
        int status,
        const char *sessionName,
        int flags,
        int reserved
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL AppendSessionRequestStatus301D(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest,
        const char *statusText
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL AppendConnectStatus301E_3021(
        void *callbackContext,
        int connectionStatusCode
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL AppendBrowseRecordStatus3022_3025(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecord
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL AppendValueStatus3026(
        void *callbackContext,
        int status,
        int value
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL OnNetworkStatusChanged(
        void *callbackContext,
        int connectionStatusCode
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL OnBrowseRecordListReceived(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecordList
    );
};

extern "C" LONG g_WestwoodOnlineUpgradeEventSinkLiveCount;
extern WestwoodOnlineUpgradeApiEventSinkVtable g_WestwoodOnlineUpgradeApiEventSink_Vtbl;

RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeApiEventSinkVtable) == 0x80);
RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeApiEventSink) == 0x20);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiEventSink,
        m_vftable
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiEventSink,
        m_refCountAndLock
    ) == 0x04
);
