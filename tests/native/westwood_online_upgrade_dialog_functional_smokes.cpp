#include "Battlesport/wol_api.h"
#include "Battlesport/wol_dialog.h"
#include "GameZRecoil/zLoc/zloc.h"

#include <new>
#include <string.h>

namespace {
enum DialogControlId {
    kServerAddressEditId = 1176,
    kStatusTokenEditId = 1139,
    kQueryValueOrTimeEditId = 1168,
    kQueryMaxPlayersEditId = 1170,
    kQueryAuxParamEditId = 1169,
    kQueryStatusFlag1CheckId = 1123,
    kQueryStatusFlag0CheckId = 1122,
    kSubmitPendingSessionListButtonId = 1151,
    kConnectButtonId = 1150,
    kQueryValueOrTimeLabelId = 1167,
    kQuerySessionsByNameButtonId = 1160,
    kQueueVisibleSessionRequestsButtonId = 1051,
    kStatusListId = 1141,
    kSessionModeComboId = 1171,
    kSessionResultsListId = 1137,
    kStatusServerEditId = 1138,
    kSessionNameEditId = 1148,
    kBrowseRecordListId = 1136,
    kStatusCaptionLabelId = 1125
};

struct TestProviderApi : IWestwoodOnlineUpgradeProviderApi {
    ULONG refCount;
    int processCallbacksCalls;
    int requestListModeCalls;
    int lastListMode;
    int lastListModeEnabled;
    int submitQueryCalls;
    int submitQueryResult;
    WestwoodOnlineUpgradeQueryRequest submittedQuery;
    int loadBrowseRecordCalls;
    int loadBrowseRecordResult;
    WestwoodOnlineUpgradeBrowseRecord loadedBrowseRecord;
    int resetQueryStateCalls;
    int submitEncodedQueryCalls;
    char submittedEncodedQuery[128];
    int submitPendingSessionListCalls;
    int submittedPendingCount;
    char submittedPendingNames[8][0x34];
    int queueSessionRequestCalls;
    char queuedSessionNames[8][0x34];
    int requestUpgradeCalls;
    int requestUpgradeResult;
    WestwoodOnlineUpgradeConnectContext upgradeContext;
    int queryStatusCalls;
    int queryStatusResult;
    WestwoodOnlineUpgradeConnectContext queryStatusContext;
    char queryStatusServer[80];
    int disconnectCalls;
    int lookupBrowseRecordCalls;
    char lookupBrowseRecordName[0x34];

    TestProviderApi()
        : refCount(1),
          processCallbacksCalls(0),
          requestListModeCalls(0),
          lastListMode(0),
          lastListModeEnabled(0),
          submitQueryCalls(0),
          submitQueryResult(0),
          loadBrowseRecordCalls(0),
          loadBrowseRecordResult(0),
          resetQueryStateCalls(0),
          submitEncodedQueryCalls(0),
          submitPendingSessionListCalls(0),
          submittedPendingCount(0),
          queueSessionRequestCalls(0),
          requestUpgradeCalls(0),
          requestUpgradeResult(0),
          queryStatusCalls(0),
          queryStatusResult(0),
          disconnectCalls(0),
          lookupBrowseRecordCalls(0) {
        memset(&submittedQuery, 0, sizeof(submittedQuery));
        memset(&loadedBrowseRecord, 0, sizeof(loadedBrowseRecord));
        memset(submittedEncodedQuery, 0, sizeof(submittedEncodedQuery));
        memset(submittedPendingNames, 0, sizeof(submittedPendingNames));
        memset(queuedSessionNames, 0, sizeof(queuedSessionNames));
        memset(&upgradeContext, 0, sizeof(upgradeContext));
        memset(&queryStatusContext, 0, sizeof(queryStatusContext));
        memset(queryStatusServer, 0, sizeof(queryStatusServer));
        memset(lookupBrowseRecordName, 0, sizeof(lookupBrowseRecordName));
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void **outInterface) {
        if (outInterface != 0) {
            *outInterface = 0;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() {
        return ++refCount;
    }

    ULONG STDMETHODCALLTYPE Release() {
        return refCount == 0 ? 0 : --refCount;
    }

    void STDMETHODCALLTYPE ProcessCallbacks() {
        ++processCallbacksCalls;
    }

    void STDMETHODCALLTYPE BeginConnect(
        int,
        int,
        const char *,
        const char *,
        int
    ) {
    }

    void STDMETHODCALLTYPE RequestBootstrapServerList(
        WestwoodOnlineUpgradeBootstrapServerRecord *,
        int,
        int
    ) {
    }

    void STDMETHODCALLTYPE RequestListMode(int listMode, int enabled) {
        ++requestListModeCalls;
        lastListMode = listMode;
        lastListModeEnabled = enabled;
    }

    int STDMETHODCALLTYPE SubmitQueryRequest(
        WestwoodOnlineUpgradeQueryRequest *request
    ) {
        ++submitQueryCalls;
        if (request != 0) {
            submittedQuery = *request;
        }
        return submitQueryResult;
    }

    int STDMETHODCALLTYPE LoadBrowseRecord(
        WestwoodOnlineUpgradeBrowseRecord *record
    ) {
        ++loadBrowseRecordCalls;
        if (record != 0) {
            loadedBrowseRecord = *record;
        }
        return loadBrowseRecordResult;
    }

    void STDMETHODCALLTYPE ResetQueryState() {
        ++resetQueryStateCalls;
    }

    void STDMETHODCALLTYPE Reserved28() {
    }

    void STDMETHODCALLTYPE SubmitStatusText(const char *) {
    }

    void STDMETHODCALLTYPE SubmitSessionRequestListAndStatusText(
        WestwoodOnlineUpgradeSessionRequest *,
        const char *
    ) {
    }

    void STDMETHODCALLTYPE Disconnect() {
        ++disconnectCalls;
    }

    void STDMETHODCALLTYPE Reserved38() {
    }

    void STDMETHODCALLTYPE SubmitEncodedQueryString(const char *encodedQuery) {
        ++submitEncodedQueryCalls;
        strncpy(
            submittedEncodedQuery,
            encodedQuery == 0 ? "" : encodedQuery,
            sizeof(submittedEncodedQuery) - 1
        );
        submittedEncodedQuery[sizeof(submittedEncodedQuery) - 1] = '\0';
    }

    void STDMETHODCALLTYPE Reserved40() {
    }

    void STDMETHODCALLTYPE Reserved44() {
    }

    void STDMETHODCALLTYPE SubmitPendingSessionList(
        WestwoodOnlineUpgradeSessionRequest *sessionRequestList
    ) {
        ++submitPendingSessionListCalls;
        submittedPendingCount = 0;
        while (sessionRequestList != 0 && submittedPendingCount < 8) {
            strncpy(
                submittedPendingNames[submittedPendingCount],
                sessionRequestList->m_sessionName,
                sizeof(submittedPendingNames[0]) - 1
            );
            submittedPendingNames[submittedPendingCount]
                                 [sizeof(submittedPendingNames[0]) - 1] = '\0';
            ++submittedPendingCount;
            sessionRequestList = sessionRequestList->m_next;
        }
    }

    void STDMETHODCALLTYPE Reserved4c() {
    }

    void STDMETHODCALLTYPE Reserved50() {
    }

    void STDMETHODCALLTYPE QueueSessionRequest(
        WestwoodOnlineUpgradeSessionRequest *request
    ) {
        if (queueSessionRequestCalls < 8 && request != 0) {
            strncpy(
                queuedSessionNames[queueSessionRequestCalls],
                request->m_sessionName,
                sizeof(queuedSessionNames[0]) - 1
            );
            queuedSessionNames[queueSessionRequestCalls]
                              [sizeof(queuedSessionNames[0]) - 1] = '\0';
        }
        ++queueSessionRequestCalls;
    }

    void STDMETHODCALLTYPE Reserved58() {
    }

    void STDMETHODCALLTYPE Reserved5c() {
    }

    int STDMETHODCALLTYPE RequestUpgradeDownloadReadyResult(
        WestwoodOnlineUpgradeConnectContext *context
    ) {
        ++requestUpgradeCalls;
        if (context != 0) {
            upgradeContext = *context;
        }
        return requestUpgradeResult;
    }

    int STDMETHODCALLTYPE QueryStatusWithTokenAndServer(
        WestwoodOnlineUpgradeConnectContext *context,
        const char *serverText
    ) {
        ++queryStatusCalls;
        if (context != 0) {
            queryStatusContext = *context;
        }
        strncpy(
            queryStatusServer,
            serverText == 0 ? "" : serverText,
            sizeof(queryStatusServer) - 1
        );
        queryStatusServer[sizeof(queryStatusServer) - 1] = '\0';
        return queryStatusResult;
    }

    void STDMETHODCALLTYPE Reserved68() {
    }

    void STDMETHODCALLTYPE BeginConnectWithPreparedContext(
        WestwoodOnlineUpgradeConnectContext *,
        int
    ) {
    }

    int STDMETHODCALLTYPE PrepareConnectContextAndMode(
        WestwoodOnlineUpgradeConnectContext *
    ) {
        return 0;
    }

    void STDMETHODCALLTYPE Reserved74() {
    }

    void STDMETHODCALLTYPE Reserved78() {
    }

    void STDMETHODCALLTYPE Reserved7c() {
    }

    void STDMETHODCALLTYPE LookupBrowseRecordBySessionName(
        const char *sessionName,
        int
    ) {
        ++lookupBrowseRecordCalls;
        strncpy(
            lookupBrowseRecordName,
            sessionName == 0 ? "" : sessionName,
            sizeof(lookupBrowseRecordName) - 1
        );
        lookupBrowseRecordName[sizeof(lookupBrowseRecordName) - 1] = '\0';
    }

    void STDMETHODCALLTYPE Reserved84() {
    }

    void STDMETHODCALLTYPE Reserved88() {
    }

    void STDMETHODCALLTYPE Reserved8c() {
    }

    int STDMETHODCALLTYPE LoadConnectProfileStrings(
        int,
        char **,
        char **
    ) {
        return 0;
    }

    int STDMETHODCALLTYPE SaveConnectProfileStrings(
        int,
        const char *,
        const char *,
        int
    ) {
        return 0;
    }
};

struct LocalizationScope {
    bool loadedHere;

    LocalizationScope() : loadedHere(false) {
        if (g_zLoc_MessagesDllHandle == 0) {
            loadedHere =
                zLoc::LoadMessagesDll("support\\messages.dll") != 0 ||
                zLoc::LoadMessagesDll("messages.dll") != 0;
        }
    }

    ~LocalizationScope() {
        if (loadedHere) {
            zLoc::UnloadMessagesDll();
        }
    }

    bool IsAvailable() const {
        return g_zLoc_MessagesDllHandle != 0;
    }
};

struct DialogFixture {
    WestwoodOnlineUpgradeDialog dialog;
    HWND host;
    HWND statusCaptionLabel;
    HWND queryValueOrTimeLabel;
    bool ok;

    DialogFixture()
        : dialog(0),
          host(0),
          statusCaptionLabel(0),
          queryValueOrTimeLabel(0),
          ok(false) {
        host = CreateWindowExA(
            0,
            "STATIC",
            "",
            WS_POPUP,
            0,
            0,
            320,
            240,
            0,
            0,
            GetModuleHandleA(0),
            0
        );
        if (host == 0) {
            return;
        }

        dialog.m_hWnd = host;
        dialog.m_serverAddressEdit.m_hWnd =
            CreateControl("EDIT", 0, kServerAddressEditId);
        dialog.m_statusTokenEdit.m_hWnd =
            CreateControl("EDIT", 0, kStatusTokenEditId);
        dialog.m_queryValueOrTimeEdit.m_hWnd =
            CreateControl("EDIT", 0, kQueryValueOrTimeEditId);
        dialog.m_queryMaxPlayersEdit.m_hWnd =
            CreateControl("EDIT", 0, kQueryMaxPlayersEditId);
        dialog.m_queryAuxParamEdit.m_hWnd =
            CreateControl("EDIT", 0, kQueryAuxParamEditId);
        dialog.m_queryStatusFlag1Check.m_hWnd =
            CreateControl("BUTTON", BS_AUTOCHECKBOX, kQueryStatusFlag1CheckId);
        dialog.m_queryStatusFlag0Check.m_hWnd =
            CreateControl("BUTTON", BS_AUTOCHECKBOX, kQueryStatusFlag0CheckId);
        dialog.m_submitPendingSessionListButton.m_hWnd =
            CreateControl("BUTTON", BS_PUSHBUTTON, kSubmitPendingSessionListButtonId);
        dialog.m_connectButton.m_hWnd =
            CreateControl("BUTTON", BS_PUSHBUTTON, kConnectButtonId);
        dialog.m_querySessionsByNameButton.m_hWnd =
            CreateControl("BUTTON", BS_PUSHBUTTON, kQuerySessionsByNameButtonId);
        dialog.m_queueVisibleSessionRequestsButton.m_hWnd =
            CreateControl("BUTTON", BS_PUSHBUTTON, kQueueVisibleSessionRequestsButtonId);
        dialog.m_statusList.m_hWnd =
            CreateControl("LISTBOX", LBS_NOTIFY, kStatusListId);
        dialog.m_sessionModeCombo.m_hWnd =
            CreateControl("COMBOBOX", CBS_DROPDOWNLIST, kSessionModeComboId);
        dialog.m_sessionResultsList.m_hWnd = CreateControl(
            "LISTBOX",
            LBS_EXTENDEDSEL | LBS_NOTIFY,
            kSessionResultsListId
        );
        dialog.m_statusServerEdit.m_hWnd =
            CreateControl("EDIT", 0, kStatusServerEditId);
        dialog.m_sessionNameEdit.m_hWnd =
            CreateControl("EDIT", 0, kSessionNameEditId);
        dialog.m_browseRecordList.m_hWnd =
            CreateControl("LISTBOX", LBS_NOTIFY, kBrowseRecordListId);
        statusCaptionLabel =
            CreateControl("STATIC", 0, kStatusCaptionLabelId);
        queryValueOrTimeLabel =
            CreateControl("STATIC", 0, kQueryValueOrTimeLabelId);

        ok =
            dialog.m_serverAddressEdit.m_hWnd != 0 &&
            dialog.m_statusTokenEdit.m_hWnd != 0 &&
            dialog.m_queryValueOrTimeEdit.m_hWnd != 0 &&
            dialog.m_queryMaxPlayersEdit.m_hWnd != 0 &&
            dialog.m_queryAuxParamEdit.m_hWnd != 0 &&
            dialog.m_queryStatusFlag1Check.m_hWnd != 0 &&
            dialog.m_queryStatusFlag0Check.m_hWnd != 0 &&
            dialog.m_submitPendingSessionListButton.m_hWnd != 0 &&
            dialog.m_connectButton.m_hWnd != 0 &&
            dialog.m_querySessionsByNameButton.m_hWnd != 0 &&
            dialog.m_queueVisibleSessionRequestsButton.m_hWnd != 0 &&
            dialog.m_statusList.m_hWnd != 0 &&
            dialog.m_sessionModeCombo.m_hWnd != 0 &&
            dialog.m_sessionResultsList.m_hWnd != 0 &&
            dialog.m_statusServerEdit.m_hWnd != 0 &&
            dialog.m_sessionNameEdit.m_hWnd != 0 &&
            dialog.m_browseRecordList.m_hWnd != 0 &&
            statusCaptionLabel != 0 &&
            queryValueOrTimeLabel != 0;
    }

    ~DialogFixture() {
        dialog.m_hWnd = 0;
        dialog.m_serverAddressEdit.m_hWnd = 0;
        dialog.m_statusTokenEdit.m_hWnd = 0;
        dialog.m_queryValueOrTimeEdit.m_hWnd = 0;
        dialog.m_queryMaxPlayersEdit.m_hWnd = 0;
        dialog.m_queryAuxParamEdit.m_hWnd = 0;
        dialog.m_queryStatusFlag1Check.m_hWnd = 0;
        dialog.m_queryStatusFlag0Check.m_hWnd = 0;
        dialog.m_submitPendingSessionListButton.m_hWnd = 0;
        dialog.m_connectButton.m_hWnd = 0;
        dialog.m_querySessionsByNameButton.m_hWnd = 0;
        dialog.m_queueVisibleSessionRequestsButton.m_hWnd = 0;
        dialog.m_statusList.m_hWnd = 0;
        dialog.m_sessionModeCombo.m_hWnd = 0;
        dialog.m_sessionResultsList.m_hWnd = 0;
        dialog.m_statusServerEdit.m_hWnd = 0;
        dialog.m_sessionNameEdit.m_hWnd = 0;
        dialog.m_browseRecordList.m_hWnd = 0;
        if (host != 0) {
            DestroyWindow(host);
        }
    }

    HWND CreateControl(const char *className, DWORD style, int controlId) {
        return CreateWindowExA(
            0,
            className,
            "",
            WS_CHILD | style,
            0,
            0,
            160,
            120,
            host,
            (HMENU)(INT_PTR)controlId,
            GetModuleHandleA(0),
            0
        );
    }
};

void SetApi(TestProviderApi &provider) {
    g_pWestwoodOnlineUpgradeApi =
        static_cast<IUnknown *>(
            static_cast<IWestwoodOnlineUpgradeProviderApi *>(&provider)
        );
}
} // namespace

extern "C" int westwood_online_upgrade_dialog_append_connect_status_smoke(void) {
    LocalizationScope localization;
    DialogFixture fixture;
    if (!localization.IsAvailable() || !fixture.ok) {
        return 1;
    }

    TestProviderApi provider;
    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    const int oldCreateFlag = g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag;
    const int oldActiveListMode = g_WestwoodOnlineUpgradeActiveListMode;
    const WestwoodOnlineUpgradeBrowseRecord oldCached =
        g_WestwoodOnlineUpgradeCachedBrowseRecord;

    SetApi(provider);
    g_pWestwoodOnlineUpgradeDialog = &fixture.dialog;
    memset(
        &g_WestwoodOnlineUpgradeCachedBrowseRecord,
        0,
        sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord)
    );
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 1;
    g_WestwoodOnlineUpgradeActiveListMode = 6;
    fixture.dialog.m_statusLineCount = 0;

    fixture.dialog.AppendConnectStatusAndRefreshList("Alpha");
    const int result =
        SendMessageA(fixture.dialog.m_statusList.m_hWnd, LB_GETCOUNT, 0, 0) == 1 &&
                fixture.dialog.m_statusLineCount == 1 &&
                provider.requestListModeCalls == 1 &&
                provider.lastListMode == 6 &&
                g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag == 0
            ? 0
            : 2;

    g_WestwoodOnlineUpgradeCachedBrowseRecord = oldCached;
    g_WestwoodOnlineUpgradeActiveListMode = oldActiveListMode;
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = oldCreateFlag;
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    return result;
}

extern "C" int westwood_online_upgrade_dialog_aux_param_edit_kill_focus_smoke(void) {
    DialogFixture fixture;
    if (!fixture.ok) {
        return 1;
    }

    fixture.dialog.m_queryAuxParam = 0;
    fixture.dialog.OnAuxParamEditKillFocus();
    char text[16] = "";
    GetWindowTextA(fixture.dialog.m_queryAuxParamEdit.m_hWnd, text, sizeof(text));
    if (fixture.dialog.m_queryAuxParam != 1 || strcmp(text, "1") != 0) {
        return 2;
    }

    fixture.dialog.m_queryAuxParam = 1001;
    fixture.dialog.OnAuxParamEditKillFocus();
    GetWindowTextA(fixture.dialog.m_queryAuxParamEdit.m_hWnd, text, sizeof(text));
    return fixture.dialog.m_queryAuxParam == 1000 && strcmp(text, "1000") == 0
               ? 0
               : 3;
}

extern "C" int westwood_online_upgrade_dialog_begin_disconnect_smoke(void) {
    WestwoodOnlineUpgradeDialog dialog(0);
    TestProviderApi provider;
    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    const int oldInFlight = g_WestwoodOnlineUpgradeDisconnectInFlightFlag;

    g_pWestwoodOnlineUpgradeApi = 0;
    g_WestwoodOnlineUpgradeDisconnectInFlightFlag = 0;
    dialog.BeginDisconnectAndShowProgress();
    int result = g_WestwoodOnlineUpgradeDisconnectInFlightFlag == 0 ? 0 : 1;

    SetApi(provider);
    g_WestwoodOnlineUpgradeDisconnectInFlightFlag = 1;
    dialog.BeginDisconnectAndShowProgress();
    if (result == 0 &&
        (provider.disconnectCalls != 0 ||
         g_WestwoodOnlineUpgradeDisconnectInFlightFlag != 1)) {
        result = 2;
    }

    g_WestwoodOnlineUpgradeDisconnectInFlightFlag = oldInFlight;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    return result;
}

extern "C" int westwood_online_upgrade_dialog_browse_record_dblclk_smoke(void) {
    DialogFixture fixture;
    if (!fixture.ok) {
        return 1;
    }

    TestProviderApi provider;
    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    const WestwoodOnlineUpgradeBrowseRecord oldCached =
        g_WestwoodOnlineUpgradeCachedBrowseRecord;
    const WestwoodOnlineUpgradeBrowseRecord oldRecord0 =
        g_WestwoodOnlineUpgradeCachedBrowseRecordList[0];

    SetApi(provider);
    g_pWestwoodOnlineUpgradeDialog = &fixture.dialog;
    memset(
        &g_WestwoodOnlineUpgradeCachedBrowseRecord,
        0,
        sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord)
    );
    memset(
        &g_WestwoodOnlineUpgradeCachedBrowseRecordList[0],
        0,
        sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecordList[0])
    );
    strcpy(
        g_WestwoodOnlineUpgradeCachedBrowseRecordList[0].m_sessionName,
        "Alpha"
    );
    g_WestwoodOnlineUpgradeCachedBrowseRecordList[0].m_recordFlags = 1;
    SendMessageA(
        fixture.dialog.m_browseRecordList.m_hWnd,
        LB_ADDSTRING,
        0,
        (LPARAM)"Alpha"
    );
    SendMessageA(
        fixture.dialog.m_browseRecordList.m_hWnd,
        LB_SETCURSEL,
        0,
        0
    );
    SetWindowTextA(fixture.dialog.m_serverAddressEdit.m_hWnd, "server");

    fixture.dialog.OnBrowseRecordListDblClk();
    const int result =
        provider.loadBrowseRecordCalls == 1 &&
                strcmp(provider.loadedBrowseRecord.m_sessionName, "Alpha") == 0 &&
                strcmp(g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName, "Alpha") == 0
            ? 0
            : 2;

    g_WestwoodOnlineUpgradeCachedBrowseRecordList[0] = oldRecord0;
    g_WestwoodOnlineUpgradeCachedBrowseRecord = oldCached;
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    return result;
}

extern "C" int westwood_online_upgrade_dialog_check_and_apply_upgrade_smoke(void) {
    DialogFixture fixture;
    if (!fixture.ok) {
        return 1;
    }

    TestProviderApi provider;
    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    provider.requestUpgradeResult = 37;
    SetApi(provider);
    SetWindowTextA(fixture.dialog.m_statusServerEdit.m_hWnd, "upgrade");

    const int callResult = fixture.dialog.CheckAndApplyUpgrade();
    const int result =
        callResult == 37 &&
                provider.requestUpgradeCalls == 1 &&
                strcmp(provider.upgradeContext.m_requestText, "upgrade") == 0
            ? 0
            : 2;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    return result;
}

extern "C" int westwood_online_upgrade_dialog_clear_status_list_smoke(void) {
    DialogFixture fixture;
    if (!fixture.ok) {
        return 1;
    }

    SendMessageA(fixture.dialog.m_statusList.m_hWnd, LB_ADDSTRING, 0, (LPARAM)"one");
    SendMessageA(fixture.dialog.m_statusList.m_hWnd, LB_ADDSTRING, 0, (LPARAM)"two");
    fixture.dialog.m_statusLineCount = 2;
    fixture.dialog.ClearStatusList();
    return SendMessageA(fixture.dialog.m_statusList.m_hWnd, LB_GETCOUNT, 0, 0) == 0 &&
                   fixture.dialog.m_statusLineCount == 0
               ? 0
               : 2;
}

extern "C" int westwood_online_upgrade_dialog_constructor_smoke(void) {
    void *const storage = ::operator new(sizeof(WestwoodOnlineUpgradeDialog));
    WestwoodOnlineUpgradeDialog *const dialog =
        static_cast<WestwoodOnlineUpgradeDialog *>(storage);
    WestwoodOnlineUpgradeDialog *const result = dialog->Constructor(0);
    const bool initialized =
        result == dialog &&
        dialog->m_queryAuxParam == 0 &&
        dialog->m_queryMaxPlayers == 0 &&
        dialog->m_queryValueOrTime == 0 &&
        dialog->m_queryStatusFlagBit0 == 0 &&
        dialog->m_queryStatusFlagBit1 == 0 &&
        dialog->m_selectedProfileConnectStringMode == 0;
    dialog->Destructor();
    ::operator delete(storage);
    return initialized ? 0 : 1;
}

extern "C" int westwood_online_upgrade_dialog_destructor_smoke(void) {
    WestwoodOnlineUpgradeDialog *const dialog =
        new WestwoodOnlineUpgradeDialog(0);
    dialog->m_selectedProfilePlayerName = "Pilot";
    dialog->m_selectedProfileConnectString = "Connect";
    dialog->m_sessionName = "Session";
    dialog->Destructor();
    ::operator delete(dialog);
    return 0;
}

extern "C" int westwood_online_upgrade_dialog_do_data_exchange_smoke(void) {
    DialogFixture fixture;
    if (!fixture.ok) {
        return 1;
    }

    SetWindowTextA(fixture.dialog.m_queryAuxParamEdit.m_hWnd, "22");
    SetWindowTextA(fixture.dialog.m_queryMaxPlayersEdit.m_hWnd, "3");
    SetWindowTextA(fixture.dialog.m_queryValueOrTimeEdit.m_hWnd, "44");
    SendMessageA(
        fixture.dialog.m_queryStatusFlag0Check.m_hWnd,
        BM_SETCHECK,
        BST_CHECKED,
        0
    );
    SendMessageA(
        fixture.dialog.m_queryStatusFlag1Check.m_hWnd,
        BM_SETCHECK,
        BST_UNCHECKED,
        0
    );

    CDataExchange exchange(&fixture.dialog, TRUE);
    fixture.dialog.DoDataExchange(&exchange);
    return fixture.dialog.m_queryAuxParam == 22 &&
                   fixture.dialog.m_queryMaxPlayers == 3 &&
                   fixture.dialog.m_queryValueOrTime == 44 &&
                   fixture.dialog.m_queryStatusFlagBit0 == 1 &&
                   fixture.dialog.m_queryStatusFlagBit1 == 0
               ? 0
               : 2;
}

extern "C" int westwood_online_upgrade_dialog_enable_controls_smoke(void) {
    DialogFixture fixture;
    if (!fixture.ok) {
        return 1;
    }

    fixture.dialog.EnableQueryControls(0);
    const bool disabled =
        !IsWindowEnabled(fixture.dialog.m_sessionModeCombo.m_hWnd) &&
        !IsWindowEnabled(fixture.dialog.m_queryStatusFlag0Check.m_hWnd) &&
        !IsWindowEnabled(fixture.dialog.m_queryStatusFlag1Check.m_hWnd) &&
        !IsWindowEnabled(fixture.dialog.m_queryAuxParamEdit.m_hWnd) &&
        !IsWindowEnabled(fixture.dialog.m_queryValueOrTimeEdit.m_hWnd) &&
        !IsWindowEnabled(fixture.dialog.m_submitPendingSessionListButton.m_hWnd) &&
        !IsWindowEnabled(fixture.dialog.m_connectButton.m_hWnd);
    fixture.dialog.EnableQueryControls(1);
    return disabled &&
                   IsWindowEnabled(fixture.dialog.m_sessionModeCombo.m_hWnd) &&
                   IsWindowEnabled(fixture.dialog.m_connectButton.m_hWnd)
               ? 0
               : 2;
}

extern "C" int westwood_online_upgrade_dialog_get_message_map_smoke(void) {
    WestwoodOnlineUpgradeDialog dialog(0);
    const AFX_MSGMAP *const messageMap = dialog.GetMessageMap();
    if (messageMap != &WestwoodOnlineUpgradeDialog::messageMap ||
        messageMap->pfnGetBaseMap == 0 ||
        messageMap->lpEntries != &WestwoodOnlineUpgradeDialog::messageEntries[0]) {
        return 1;
    }

    const AFX_MSGMAP_ENTRY *const entries =
        WestwoodOnlineUpgradeDialog::messageEntries;
    return entries[0].nMessage == WM_TIMER &&
                   entries[1].nMessage == WM_DESTROY &&
                   entries[21].nCode == LBN_DBLCLK &&
                   entries[22].nCode == EN_KILLFOCUS &&
                   entries[23].nCode == EN_KILLFOCUS &&
                   entries[24].nCode == EN_KILLFOCUS &&
                   entries[25].nMessage == 0 &&
                   entries[25].pfn == 0
               ? 0
               : 2;
}

extern "C" int westwood_online_upgrade_dialog_max_players_edit_change_smoke(void) {
    DialogFixture fixture;
    if (!fixture.ok) {
        return 1;
    }

    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    g_pWestwoodOnlineUpgradeDialog = &fixture.dialog;
    SetWindowTextA(fixture.dialog.m_queryMaxPlayersEdit.m_hWnd, "3");
    fixture.dialog.m_queryMaxPlayers = 0;
    fixture.dialog.OnMaxPlayersEditChange();
    const int result = fixture.dialog.m_queryMaxPlayers == 3 ? 0 : 2;
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    return result;
}

extern "C" int westwood_online_upgrade_dialog_max_players_edit_kill_focus_smoke(void) {
    DialogFixture fixture;
    if (!fixture.ok) {
        return 1;
    }

    fixture.dialog.m_queryMaxPlayers = 1;
    fixture.dialog.OnMaxPlayersEditKillFocus();
    char text[16] = "";
    GetWindowTextA(fixture.dialog.m_queryMaxPlayersEdit.m_hWnd, text, sizeof(text));
    if (fixture.dialog.m_queryMaxPlayers != 2 || strcmp(text, "2") != 0) {
        return 2;
    }

    fixture.dialog.m_queryMaxPlayers = 8;
    fixture.dialog.OnMaxPlayersEditKillFocus();
    GetWindowTextA(fixture.dialog.m_queryMaxPlayersEdit.m_hWnd, text, sizeof(text));
    return fixture.dialog.m_queryMaxPlayers == 4 && strcmp(text, "4") == 0
               ? 0
               : 3;
}

extern "C" int westwood_online_upgrade_dialog_on_destroy_smoke(void) {
    DialogFixture fixture;
    WestwoodOnlineUpgradeProgressDialog progressDialog(0);
    if (!fixture.ok) {
        return 1;
    }

    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    WestwoodOnlineUpgradeProgressDialog *const oldProgressDialog =
        g_pWestwoodOnlineUpgradeProgressDialog;
    const int oldAbortFlag = g_WestwoodOnlineUpgradeAbortFlag;
    g_pWestwoodOnlineUpgradeApi = 0;
    g_pWestwoodOnlineUpgradeProgressDialog = &progressDialog;
    g_WestwoodOnlineUpgradeAbortFlag = 1;

    fixture.dialog.OnDestroy();
    const int result =
        g_WestwoodOnlineUpgradeAbortFlag == 1 &&
                g_pWestwoodOnlineUpgradeApi == 0
            ? 0
            : 2;

    g_WestwoodOnlineUpgradeAbortFlag = oldAbortFlag;
    g_pWestwoodOnlineUpgradeProgressDialog = oldProgressDialog;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    return result;
}

extern "C" int westwood_online_upgrade_dialog_on_init_bootstrap_smoke(void) {
    WestwoodOnlineUpgradeDialog dialog(0);
    BOOL (WestwoodOnlineUpgradeDialog::*onInitDialog)() =
        &WestwoodOnlineUpgradeDialog::OnInitDialog;
    const AFX_MSGMAP *const messageMap = dialog.GetMessageMap();
    return onInitDialog != 0 &&
                   messageMap == &WestwoodOnlineUpgradeDialog::messageMap &&
                   dialog.m_queryValueOrTime == 0 &&
                   dialog.m_queryAuxParam == 0 &&
                   dialog.m_queryMaxPlayers == 0
               ? 0
               : 1;
}

extern "C" int westwood_online_upgrade_dialog_query_sessions_by_name_smoke(void) {
    DialogFixture fixture;
    if (!fixture.ok) {
        return 1;
    }

    TestProviderApi provider;
    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    const int oldActiveListMode = g_WestwoodOnlineUpgradeActiveListMode;
    const WestwoodOnlineUpgradeBrowseRecord oldCached =
        g_WestwoodOnlineUpgradeCachedBrowseRecord;

    SetApi(provider);
    g_pWestwoodOnlineUpgradeDialog = &fixture.dialog;
    memset(
        &g_WestwoodOnlineUpgradeCachedBrowseRecord,
        0,
        sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord)
    );
    fixture.dialog.m_queryMaxPlayers = 4;
    SetWindowTextA(fixture.dialog.m_sessionNameEdit.m_hWnd, "  Alpha  ");
    SetWindowTextA(fixture.dialog.m_serverAddressEdit.m_hWnd, "server");

    fixture.dialog.OnQuerySessionsByName();
    const int result =
        provider.submitQueryCalls == 1 &&
                provider.submittedQuery.m_listMode == 17 &&
                provider.submittedQuery.m_queryVariant == 2 &&
                provider.submittedQuery.m_queryMaxPlayers == 4 &&
                strcmp(provider.submittedQuery.m_sessionName, "Alpha") == 0 &&
                strcmp((const char *)fixture.dialog.m_sessionName, "Alpha") == 0 &&
                g_WestwoodOnlineUpgradeActiveListMode == 17
            ? 0
            : 2;

    g_WestwoodOnlineUpgradeCachedBrowseRecord = oldCached;
    g_WestwoodOnlineUpgradeActiveListMode = oldActiveListMode;
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    return result;
}

extern "C" int westwood_online_upgrade_dialog_query_status_smoke(void) {
    DialogFixture fixture;
    if (!fixture.ok) {
        return 1;
    }

    TestProviderApi provider;
    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    provider.queryStatusResult = 19;
    SetApi(provider);
    SetWindowTextA(fixture.dialog.m_statusTokenEdit.m_hWnd, "token42 extra");
    SetWindowTextA(fixture.dialog.m_statusServerEdit.m_hWnd, "server.example");

    const int callResult = fixture.dialog.QueryStatus();
    const int result =
        callResult == 19 &&
                provider.queryStatusCalls == 1 &&
                strcmp(provider.queryStatusContext.m_requestText, "token42") == 0 &&
                strcmp(provider.queryStatusServer, "server.example") == 0
            ? 0
            : 2;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    return result;
}

extern "C" int westwood_online_upgrade_dialog_queue_visible_session_requests_smoke(void) {
    DialogFixture fixture;
    if (!fixture.ok) {
        return 1;
    }

    TestProviderApi provider;
    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    const int oldCreateFlag = g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag;
    const int oldVisibleCount = g_WestwoodOnlineUpgradeVisibleSessionResultCount;
    const WestwoodOnlineUpgradeBrowseRecord oldCached =
        g_WestwoodOnlineUpgradeCachedBrowseRecord;

    SetApi(provider);
    memset(
        &g_WestwoodOnlineUpgradeCachedBrowseRecord,
        0,
        sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord)
    );
    strcpy(g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName, "Cached");
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 1;
    fixture.dialog.m_selectedProfilePlayerName = "PlayerOwned";
    SendMessageA(
        fixture.dialog.m_sessionResultsList.m_hWnd,
        LB_ADDSTRING,
        0,
        (LPARAM)"Alpha details"
    );
    SendMessageA(
        fixture.dialog.m_sessionResultsList.m_hWnd,
        LB_ADDSTRING,
        0,
        (LPARAM)"PlayerOwned details"
    );
    SendMessageA(
        fixture.dialog.m_sessionResultsList.m_hWnd,
        LB_SETSEL,
        TRUE,
        0
    );
    SendMessageA(
        fixture.dialog.m_sessionResultsList.m_hWnd,
        LB_SETSEL,
        TRUE,
        1
    );

    fixture.dialog.QueueVisibleSessionRequests();
    const int result =
        provider.queueSessionRequestCalls == 1 &&
                strcmp(provider.queuedSessionNames[0], "Alpha") == 0 &&
                g_WestwoodOnlineUpgradeVisibleSessionResultCount == 2
            ? 0
            : 2;

    g_WestwoodOnlineUpgradeCachedBrowseRecord = oldCached;
    g_WestwoodOnlineUpgradeVisibleSessionResultCount = oldVisibleCount;
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = oldCreateFlag;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    return result;
}

extern "C" int westwood_online_upgrade_dialog_refresh_current_query_smoke(void) {
    DialogFixture fixture;
    if (!fixture.ok) {
        return 1;
    }

    TestProviderApi provider;
    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    const int oldCreateFlag = g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag;
    const int oldActiveListMode = g_WestwoodOnlineUpgradeActiveListMode;
    const WestwoodOnlineUpgradeBrowseRecord oldCached =
        g_WestwoodOnlineUpgradeCachedBrowseRecord;

    SetApi(provider);
    g_pWestwoodOnlineUpgradeDialog = &fixture.dialog;
    memset(
        &g_WestwoodOnlineUpgradeCachedBrowseRecord,
        0,
        sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord)
    );
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 0;
    g_WestwoodOnlineUpgradeActiveListMode = 7;
    fixture.dialog.m_queryMaxPlayers = 4;
    SetWindowTextA(fixture.dialog.m_sessionNameEdit.m_hWnd, "  Gamma  ");

    fixture.dialog.OnRefreshCurrentQuery();
    const int result =
        provider.submitQueryCalls == 1 &&
                provider.submittedQuery.m_listMode == 0 &&
                provider.submittedQuery.m_queryVariant == 2 &&
                provider.submittedQuery.m_queryMaxPlayers == 4 &&
                strcmp(provider.submittedQuery.m_sessionName, "Gamma") == 0 &&
                g_WestwoodOnlineUpgradeActiveListMode == 0
            ? 0
            : 2;

    g_WestwoodOnlineUpgradeCachedBrowseRecord = oldCached;
    g_WestwoodOnlineUpgradeActiveListMode = oldActiveListMode;
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = oldCreateFlag;
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    return result;
}

extern "C" int westwood_online_upgrade_dialog_request_active_list_mode_smoke(void) {
    WestwoodOnlineUpgradeDialog dialog(0);
    TestProviderApi provider;
    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    const int oldActiveListMode = g_WestwoodOnlineUpgradeActiveListMode;
    SetApi(provider);
    g_WestwoodOnlineUpgradeActiveListMode = 29;

    dialog.RequestActiveListMode();
    const int result =
        provider.requestListModeCalls == 1 &&
                provider.lastListMode == 29 &&
                provider.lastListModeEnabled == 1
            ? 0
            : 1;

    g_WestwoodOnlineUpgradeActiveListMode = oldActiveListMode;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    return result;
}

extern "C" int westwood_online_upgrade_dialog_request_list_modes_smoke(void) {
    WestwoodOnlineUpgradeDialog dialog(0);
    TestProviderApi provider;
    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    const int oldActiveListMode = g_WestwoodOnlineUpgradeActiveListMode;
    SetApi(provider);

    dialog.RequestListMode0();
    const bool mode0 =
        provider.requestListModeCalls == 1 &&
        provider.lastListMode == 0 &&
        provider.lastListModeEnabled == 0 &&
        g_WestwoodOnlineUpgradeActiveListMode == 0;
    dialog.RequestListMode11();
    const bool mode11 =
        provider.requestListModeCalls == 2 &&
        provider.lastListMode == 11 &&
        provider.lastListModeEnabled == 1 &&
        g_WestwoodOnlineUpgradeActiveListMode == 11;

    g_WestwoodOnlineUpgradeActiveListMode = oldActiveListMode;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    return mode0 && mode11 ? 0 : 1;
}

extern "C" int westwood_online_upgrade_dialog_reset_selected_browse_record_smoke(void) {
    DialogFixture fixture;
    if (!fixture.ok) {
        return 1;
    }

    TestProviderApi provider;
    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    const int oldCreateFlag = g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag;
    const int oldActiveListMode = g_WestwoodOnlineUpgradeActiveListMode;
    const WestwoodOnlineUpgradeBrowseRecord oldCached =
        g_WestwoodOnlineUpgradeCachedBrowseRecord;

    SetApi(provider);
    g_pWestwoodOnlineUpgradeDialog = &fixture.dialog;
    memset(
        &g_WestwoodOnlineUpgradeCachedBrowseRecord,
        0,
        sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord)
    );
    strcpy(g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName, "Selected");
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 1;
    g_WestwoodOnlineUpgradeActiveListMode = 7;
    SendMessageA(
        fixture.dialog.m_sessionResultsList.m_hWnd,
        LB_ADDSTRING,
        0,
        (LPARAM)"row"
    );

    fixture.dialog.ResetSelectedBrowseRecordAndRefreshList();
    const int result =
        provider.resetQueryStateCalls == 1 &&
                provider.requestListModeCalls == 1 &&
                provider.lastListMode == 7 &&
                g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] == '\0' &&
                g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag == 0 &&
                SendMessageA(
                    fixture.dialog.m_sessionResultsList.m_hWnd,
                    LB_GETCOUNT,
                    0,
                    0
                ) == 0
            ? 0
            : 2;

    g_WestwoodOnlineUpgradeCachedBrowseRecord = oldCached;
    g_WestwoodOnlineUpgradeActiveListMode = oldActiveListMode;
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = oldCreateFlag;
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    return result;
}

extern "C" int westwood_online_upgrade_dialog_session_mode_sel_change_smoke(void) {
    LocalizationScope localization;
    DialogFixture fixture;
    if (!localization.IsAvailable() || !fixture.ok) {
        return 1;
    }

    TestProviderApi provider;
    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    SetApi(provider);
    g_pWestwoodOnlineUpgradeDialog = &fixture.dialog;
    SendMessageA(
        fixture.dialog.m_sessionModeCombo.m_hWnd,
        CB_ADDSTRING,
        0,
        (LPARAM)"mode"
    );
    SendMessageA(
        fixture.dialog.m_sessionModeCombo.m_hWnd,
        CB_SETITEMDATA,
        0,
        2
    );
    SendMessageA(
        fixture.dialog.m_sessionModeCombo.m_hWnd,
        CB_SETCURSEL,
        0,
        0
    );
    SetWindowTextA(fixture.dialog.m_queryValueOrTimeEdit.m_hWnd, "15");
    SetWindowTextA(fixture.dialog.m_queryAuxParamEdit.m_hWnd, "10");
    SetWindowTextA(fixture.dialog.m_queryMaxPlayersEdit.m_hWnd, "4");

    fixture.dialog.OnSessionModeComboSelChange();
    const int result =
        fixture.dialog.m_querySessionModeKind == 2 &&
                !IsWindowEnabled(fixture.dialog.m_queryValueOrTimeEdit.m_hWnd) &&
                provider.submitEncodedQueryCalls == 1
            ? 0
            : 2;

    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    return result;
}

extern "C" int westwood_online_upgrade_dialog_set_abort_and_close_smoke(void) {
    DialogFixture fixture;
    if (!fixture.ok) {
        return 1;
    }

    const int oldAbortFlag = g_WestwoodOnlineUpgradeAbortFlag;
    g_WestwoodOnlineUpgradeAbortFlag = 0;
    fixture.dialog.SetAbortAndClose();
    const int result = g_WestwoodOnlineUpgradeAbortFlag == 1 ? 0 : 2;
    g_WestwoodOnlineUpgradeAbortFlag = oldAbortFlag;
    return result;
}

extern "C" int westwood_online_upgrade_dialog_set_selected_profile_values_smoke(void) {
    WestwoodOnlineUpgradeDialog dialog(0);
    dialog.SetSelectedProfilePlayerName(CString("Pilot"));
    dialog.SetSelectedProfileConnectString(CString("Connect"));
    return strcmp((const char *)dialog.m_selectedProfilePlayerName, "Pilot") == 0 &&
                   strcmp((const char *)dialog.m_selectedProfileConnectString, "Connect") == 0
               ? 0
               : 1;
}

extern "C" int westwood_online_upgrade_dialog_submit_pending_session_list_smoke(void) {
    DialogFixture fixture;
    if (!fixture.ok) {
        return 1;
    }

    TestProviderApi provider;
    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    const int oldCreateFlag = g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag;
    const int oldPendingCount = g_WestwoodOnlineUpgradePendingSessionResultCount;
    const WestwoodOnlineUpgradeBrowseRecord oldCached =
        g_WestwoodOnlineUpgradeCachedBrowseRecord;

    SetApi(provider);
    memset(
        &g_WestwoodOnlineUpgradeCachedBrowseRecord,
        0,
        sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord)
    );
    strcpy(g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName, "Cached");
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 1;
    g_WestwoodOnlineUpgradePendingSessionResultCount = 2;
    SendMessageA(
        fixture.dialog.m_sessionResultsList.m_hWnd,
        LB_ADDSTRING,
        0,
        (LPARAM)"Alpha details"
    );
    SendMessageA(
        fixture.dialog.m_sessionResultsList.m_hWnd,
        LB_ADDSTRING,
        0,
        (LPARAM)"Beta details"
    );

    fixture.dialog.SubmitPendingSessionListFromResults();
    const int result =
        provider.submitPendingSessionListCalls == 1 &&
                provider.submittedPendingCount == 2 &&
                strcmp(provider.submittedPendingNames[0], "Beta") == 0 &&
                strcmp(provider.submittedPendingNames[1], "Alpha") == 0
            ? 0
            : 2;

    g_WestwoodOnlineUpgradeCachedBrowseRecord = oldCached;
    g_WestwoodOnlineUpgradePendingSessionResultCount = oldPendingCount;
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = oldCreateFlag;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    return result;
}

extern "C" int westwood_online_upgrade_dialog_update_session_list_query_smoke(void) {
    DialogFixture fixture;
    if (!fixture.ok) {
        return 1;
    }

    TestProviderApi provider;
    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    SetApi(provider);
    g_pWestwoodOnlineUpgradeDialog = &fixture.dialog;
    SendMessageA(
        fixture.dialog.m_sessionModeCombo.m_hWnd,
        CB_ADDSTRING,
        0,
        (LPARAM)"zero"
    );
    SendMessageA(
        fixture.dialog.m_sessionModeCombo.m_hWnd,
        CB_ADDSTRING,
        0,
        (LPARAM)"one"
    );
    SendMessageA(
        fixture.dialog.m_sessionModeCombo.m_hWnd,
        CB_SETCURSEL,
        1,
        0
    );
    SetWindowTextA(fixture.dialog.m_queryValueOrTimeEdit.m_hWnd, "5");
    SetWindowTextA(fixture.dialog.m_queryAuxParamEdit.m_hWnd, "6");
    SetWindowTextA(fixture.dialog.m_queryMaxPlayersEdit.m_hWnd, "4");
    SendMessageA(
        fixture.dialog.m_queryStatusFlag0Check.m_hWnd,
        BM_SETCHECK,
        BST_CHECKED,
        0
    );
    SendMessageA(
        fixture.dialog.m_queryStatusFlag1Check.m_hWnd,
        BM_SETCHECK,
        BST_UNCHECKED,
        0
    );

    WestwoodOnlineUpgradeDialog::UpdateSessionListQueryFromControls();
    const int result =
        provider.submitEncodedQueryCalls == 1 &&
                provider.submittedEncodedQuery[0] != '\0' &&
                fixture.dialog.m_queryValueOrTime == 5 &&
                fixture.dialog.m_queryAuxParam == 6 &&
                fixture.dialog.m_queryMaxPlayers == 4
            ? 0
            : 2;

    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    return result;
}

extern "C" int westwood_online_upgrade_dialog_value_or_time_edit_kill_focus_smoke(void) {
    DialogFixture fixture;
    if (!fixture.ok) {
        return 1;
    }

    fixture.dialog.m_queryValueOrTime = 1;
    fixture.dialog.OnValueOrTimeEditKillFocus();
    char text[16] = "";
    GetWindowTextA(fixture.dialog.m_queryValueOrTimeEdit.m_hWnd, text, sizeof(text));
    if (fixture.dialog.m_queryValueOrTime != 2 || strcmp(text, "2") != 0) {
        return 2;
    }

    fixture.dialog.m_queryValueOrTime = 2001;
    fixture.dialog.OnValueOrTimeEditKillFocus();
    GetWindowTextA(fixture.dialog.m_queryValueOrTimeEdit.m_hWnd, text, sizeof(text));
    return fixture.dialog.m_queryValueOrTime == 2000 &&
                   strcmp(text, "2000") == 0
               ? 0
               : 3;
}

extern "C" int westwood_online_upgrade_truncate_string_at_first_space_smoke(void) {
    char spaced[] = "Alpha Beta Gamma";
    char unspaced[] = "Delta";
    char leading[] = " Echo";
    WestwoodOnlineUpgrade::TruncateStringAtFirstSpace(spaced);
    WestwoodOnlineUpgrade::TruncateStringAtFirstSpace(unspaced);
    WestwoodOnlineUpgrade::TruncateStringAtFirstSpace(leading);
    return strcmp(spaced, "Alpha") == 0 &&
                   strcmp(unspaced, "Delta") == 0 &&
                   leading[0] == '\0'
               ? 0
               : 1;
}
