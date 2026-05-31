#pragma once

#include <stddef.h>

#include "Battlesport/Mfc42Abi.h"
#include "Battlesport/WestwoodOnlineUpgradeRefCountAndLock.h"
#include "recoil/recoil_callconv.h"
#include "recoil/recoil_types.h"

struct WestwoodOnlineUpgradeDownloadReadyEntry;
struct WestwoodOnlineUpgradeBrowseRecord;
struct WestwoodOnlineUpgradeSessionRequest;

struct WestwoodOnlineUpgradeApiEventSinkVtable
{
    void *slots[32];
};

struct WestwoodOnlineUpgradeApiEventSink
{
    WestwoodOnlineUpgradeApiEventSinkVtable *m_vftable;
    WestwoodOnlineUpgradeRefCountAndLock m_refCountAndLock;

    RECOIL_NOINLINE static HRESULT RECOIL_STDCALL
    CreateInstance(WestwoodOnlineUpgradeApiEventSink **outSink);
    RECOIL_NOINLINE static int RECOIL_STDCALL
    OnDownloadReadyResult(
        void *callbackContext,
        int resultCode,
        WestwoodOnlineUpgradeDownloadReadyEntry *downloadReadyList
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL
    OnPendingSessionRequestRemoved(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL
    OnServerError(
        void *callbackContext,
        int status,
        const char *errorText
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL
    OnApiStatus(
        void *callbackContext,
        int statusCode,
        const char *statusText
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL
    OnStatusTextReceived(
        void *callbackContext,
        int status,
        const char *statusText
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL
    OnBrowseRecordAdded(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecord
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL
    OnBrowseRecordAndSessionResolved(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecord,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL
    OnSessionQueryFinished(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecord,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL
    OnSessionListEnumerated(
        void *callbackContext,
        int status,
        WestwoodOnlineUpgradeBrowseRecord *browseRecord,
        WestwoodOnlineUpgradeSessionRequest *sessionList
    );
};

extern "C" LONG g_WestwoodOnlineUpgradeEventSinkLiveCount;
extern WestwoodOnlineUpgradeApiEventSinkVtable g_WestwoodOnlineUpgradeApiEventSink_Vtbl;

RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeApiEventSinkVtable) == 0x80);
RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeApiEventSink) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeApiEventSink, m_vftable) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeApiEventSink, m_refCountAndLock) == 0x04);
