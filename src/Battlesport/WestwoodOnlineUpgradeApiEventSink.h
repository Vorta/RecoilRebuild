#pragma once

#include <stddef.h>

#include "Battlesport/Mfc42Abi.h"
#include "Battlesport/WestwoodOnlineUpgradeRefCountAndLock.h"
#include "recoil/recoil_callconv.h"
#include "recoil/recoil_types.h"

#include <unknwn.h>

struct WestwoodOnlineUpgradeDownloadReadyEntry;
struct WestwoodOnlineUpgradeBrowseRecord;
struct WestwoodOnlineUpgradeBootstrapServerRecord;
struct WestwoodOnlineUpgradeSessionRequest;

struct WestwoodOnlineUpgradeApiEventSink : IUnknown {
    WestwoodOnlineUpgradeRefCountAndLock m_refCountAndLock;

    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID iid,
        void **outInterface
    );
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    virtual int STDMETHODCALLTYPE OnBootstrapServerList(
        int resultCode,
        WestwoodOnlineUpgradeBootstrapServerRecord *serverList
    );
    virtual int STDMETHODCALLTYPE OnDownloadReadyResult(
        int resultCode,
        WestwoodOnlineUpgradeDownloadReadyEntry *downloadReadyList
    );
    virtual int STDMETHODCALLTYPE OnServerError(
        int status,
        const char *errorText
    );
    virtual int STDMETHODCALLTYPE OnApiStatus(
        int statusCode,
        const char *statusText
    );
    virtual int STDMETHODCALLTYPE OnStatusTextReceived(
        int status,
        const char *statusText
    );
    virtual int STDMETHODCALLTYPE OnBrowseRecordListReceived(
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecordList
    );
    virtual int STDMETHODCALLTYPE OnBrowseRecordAdded(
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecord
    );
    virtual int STDMETHODCALLTYPE OnBrowseRecordAndSessionResolved(
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecord,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest
    );
    virtual int STDMETHODCALLTYPE OnSessionQueryFinished(
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecord,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest
    );
    virtual int STDMETHODCALLTYPE CallbackNoOp0(
        int reserved0,
        int reserved1,
        int reserved2
    );
    virtual int STDMETHODCALLTYPE AppendSessionRequestStatus301C_Alt0(
        int status,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest,
        int value
    );
    virtual int STDMETHODCALLTYPE AppendSessionRequestStatus301C_Alt1(
        int status,
        int reserved,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest,
        int value
    );
    virtual int STDMETHODCALLTYPE OnSessionListEnumerated(
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecord,
        WestwoodOnlineUpgradeSessionRequest *sessionList
    );
    virtual int STDMETHODCALLTYPE AppendSessionRequestStatus301B(
        int status,
        int reserved,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest,
        const char *statusText
    );
    virtual int STDMETHODCALLTYPE AppendSessionRequestStatus301C(
        int status,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest,
        const char *statusText
    );
    virtual int STDMETHODCALLTYPE AppendValueStatus3026(
        int status,
        int value
    );
    virtual int STDMETHODCALLTYPE OnNetworkStatusChanged(
        int connectionStatusCode
    );
    virtual int STDMETHODCALLTYPE OnPendingSessionRequestRemoved(
        int status,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest
    );
    virtual int STDMETHODCALLTYPE ApplyEncodedQueryString0(
        int status,
        int reserved,
        char *encodedQuery
    );
    virtual int STDMETHODCALLTYPE ApplyEncodedQueryString1(
        int status,
        int reserved,
        int reserved2,
        char *encodedQuery
    );
    virtual int STDMETHODCALLTYPE LaunchSelectedSession(
        int status,
        int reserved,
        WestwoodOnlineUpgradeSessionRequest *selectedSessionList,
        int reserved2
    );
    virtual int STDMETHODCALLTYPE OnSessionLaunchResult(
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecord,
        WestwoodOnlineUpgradeSessionRequest *sessionNode,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest
    );
    virtual int STDMETHODCALLTYPE CallbackNoOp1(
        int reserved0,
        int reserved1
    );
    virtual int STDMETHODCALLTYPE AppendBrowseRecordStatus3022_3025(
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecord
    );
    virtual int STDMETHODCALLTYPE AppendConnectStatus301E_3021(
        int connectionStatusCode
    );
    virtual int STDMETHODCALLTYPE AppendSessionRequestStatus301D(
        int status,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest,
        const char *statusText
    );
    virtual int STDMETHODCALLTYPE AppendTimeStatus302A(
        int status,
        long unixTime
    );
    virtual int STDMETHODCALLTYPE UpdateSessionResultItemFlags(
        int status,
        const char *sessionName,
        int flags,
        int reserved
    );
    virtual int STDMETHODCALLTYPE AppendValueStatus302B_302C(
        int reserved,
        int value,
        int usePrimaryMessage
    );

    static HRESULT __stdcall CreateInstance(
        WestwoodOnlineUpgradeApiEventSink **outSink
    );
    static HRESULT __stdcall QueryInterface(
        WestwoodOnlineUpgradeApiEventSink *self,
        REFIID iid,
        void **outInterface
    );
    static ULONG __stdcall Release(WestwoodOnlineUpgradeApiEventSink *self);
    void Destructor();
    static int __stdcall OnDownloadReadyResult(
        void *callbackContext,
        int resultCode,
        WestwoodOnlineUpgradeDownloadReadyEntry *downloadReadyList
    );
    static int __stdcall OnPendingSessionRequestRemoved(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest
    );
    static int __stdcall OnBootstrapServerList(
        void *callbackContext,
        int resultCode,
        WestwoodOnlineUpgradeBootstrapServerRecord *serverList
    );
    static int __stdcall OnServerError(
        void *callbackContext,
        int status,
        const char *errorText
    );
    static int __stdcall OnApiStatus(
        void *callbackContext,
        int statusCode,
        const char *statusText
    );
    static int __stdcall OnStatusTextReceived(
        void *callbackContext,
        int status,
        const char *statusText
    );
    static int __stdcall OnBrowseRecordAdded(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecord
    );
    static int __stdcall OnBrowseRecordAndSessionResolved(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecord,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest
    );
    static int __stdcall OnSessionQueryFinished(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecord,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest
    );
    static int __stdcall OnSessionLaunchResult(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecord,
        WestwoodOnlineUpgradeSessionRequest *sessionNode,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest
    );
    static int __stdcall OnSessionListEnumerated(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecord,
        WestwoodOnlineUpgradeSessionRequest *sessionList
    );
    static int __stdcall LaunchSelectedSession(
        void *callbackContext,
        int status,
        int reserved,
        WestwoodOnlineUpgradeSessionRequest *selectedSessionList,
        int reserved2
    );
    static int __stdcall ApplyEncodedQueryString0(
        void *callbackContext,
        int status,
        int reserved,
        char *encodedQuery
    );
    static int __stdcall ApplyEncodedQueryString1(
        void *callbackContext,
        int status,
        int reserved,
        int reserved2,
        char *encodedQuery
    );
    static int __stdcall AppendSessionRequestStatus301B(
        void *callbackContext,
        int status,
        int reserved,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest,
        const char *statusText
    );
    static int __stdcall AppendSessionRequestStatus301C(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest,
        const char *statusText
    );
    static int __stdcall AppendSessionRequestStatus301C_Alt0(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest,
        int value
    );
    static int __stdcall AppendSessionRequestStatus301C_Alt1(
        void *callbackContext,
        int status,
        int reserved,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest,
        int value
    );
    static int __stdcall CallbackNoOp0(
        void *callbackContext,
        int reserved0,
        int reserved1,
        int reserved2
    );
    static int __stdcall CallbackNoOp1(
        void *callbackContext,
        int reserved0,
        int reserved1
    );
    static int __stdcall AppendTimeStatus302A(
        void *callbackContext,
        int status,
        long unixTime
    );
    static int __stdcall AppendValueStatus302B_302C(
        void *callbackContext,
        int reserved,
        int value,
        int usePrimaryMessage
    );
    static int __stdcall UpdateSessionResultItemFlags(
        void *callbackContext,
        int status,
        const char *sessionName,
        int flags,
        int reserved
    );
    static int __stdcall AppendSessionRequestStatus301D(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest,
        const char *statusText
    );
    static int __stdcall AppendConnectStatus301E_3021(
        void *callbackContext,
        int connectionStatusCode
    );
    static int __stdcall AppendBrowseRecordStatus3022_3025(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecord
    );
    static int __stdcall AppendValueStatus3026(
        void *callbackContext,
        int status,
        int value
    );
    static int __stdcall OnNetworkStatusChanged(
        void *callbackContext,
        int connectionStatusCode
    );
    static int __stdcall OnBrowseRecordListReceived(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecordList
    );
};

extern "C" LONG g_WestwoodOnlineUpgradeEventSinkLiveCount;

RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeApiEventSink) == 0x20);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiEventSink,
        m_refCountAndLock
    ) == 0x04
);
