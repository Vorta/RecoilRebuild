/*
 * Reimplements 0x42dda0: WestwoodOnlineUpgradeApiInitState::Init.
 * The literal-backed player.cpp physical contribution owns the definition;
 * this translation unit retains the WOL source-owner provenance.
 *
 * The ordinary owner headers and this translation unit contribute declarations,
 * data, and source-local support.
 * Address-backed definitions follow in retail lexical order below.
 *
 * Reimplements 0x43d980: compiler-generated ordinary destructor lifecycle
 * contribution for WestwoodOnlineUpgradeDialog.
 */
#include "Battlesport/wol_api.h"

#include "Battlesport/recoil_app.h"
#include "Battlesport/wol_api_event_sink.h"
#include "Battlesport/wol_config_dialog.h"
#include "GameZRecoil/zCom/zCom.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zLoc/zloc.h"

#include <string.h>

/**
 * Reimplements data 0x4dd24c: g_WestwoodOnlineUpgradeAbortFlag.
 * Purpose: carries the upgrade dialog/provider disconnect abort state, starting
 * in the aborted state until the API init flow clears it.
 */
extern "C" int g_WestwoodOnlineUpgradeAbortFlag = 1;

// WestwoodOnline owns the WOL ActiveX API startup globals. Current BN evidence
// shows these as independent zero-initialized or immutable data symbols used by
// the API startup anchors; dialog/session-browser globals defined in this
// translation unit are tracked as a separate data-owner blocker.
// API startup wait/event state.
extern "C" HANDLE g_WestwoodOnlineUpgradeInitWaitEvents[3] = {0};
extern "C" HANDLE g_WestwoodOnlineUpgradeFailureEvent = 0;
extern "C" WestwoodOnlineUpgradeBootstrapServerRecord
    g_WestwoodOnlineUpgradeSelectedBootstrapServer = {0};

// Session-browser/dialog state: defined here by the recovered source file, but
// not part of the API startup data owner.
extern "C" int g_WestwoodOnlineUpgradeActiveListMode = 0;

// API callback pump state.
extern "C" int g_WestwoodOnlineUpgradeProcessCallbacksFlag = 0;

extern "C" WestwoodOnlineUpgradeApiInitState g_WestwoodOnlineUpgradeApiInitState = {0};
extern "C" int g_WestwoodOnlineUpgradeApiShutdownState = 0;
extern "C" HANDLE g_WestwoodOnlineUpgradeStatusTextEvent = 0;
extern "C" HANDLE g_WestwoodOnlineUpgradeBootstrapServerListEvent = 0;

// Session-browser cache state. 0x43d2e0 clears the current record at startup,
// but the broader browse/cache owner remains the dialog/event-sink slice.
extern "C" WestwoodOnlineUpgradeBrowseRecord g_WestwoodOnlineUpgradeCachedBrowseRecord = {0};
extern "C" WestwoodOnlineUpgradeBrowseRecord g_WestwoodOnlineUpgradeCachedBrowseRecordList[1024] = {
    {0}};

extern "C" int g_WestwoodOnlineUpgradeDisconnectInFlightFlag = 0;
extern "C" void *g_pWestwoodOnlineUpgradeApiEventSink = 0;
extern "C" int g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 0;
extern "C" DWORD g_WestwoodOnlineUpgradeApiAdviseCookie = 0;
extern "C" IUnknown *g_pWestwoodOnlineUpgradeApi = 0;

// More session-browser/dialog state.
extern "C" int g_WestwoodOnlineUpgradePendingSessionResultCount = 0;
extern "C" int g_WestwoodOnlineUpgradeCachedBrowseRecordListCount = 0;
extern "C" int g_WestwoodOnlineUpgradeVisibleSessionResultCount = 0;
extern "C" int g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 0;
extern "C" float g_WestwoodOnlineUpgradeNextAutoRefreshTime = 0.0f;

// BN observes the same COM identity bytes as g_CLSID_WestwoodOnlineUpgradeApi,
// g_IID_WestwoodOnlineUpgradeApi, and IID_WestwoodOnlineUpgradeApiEventSink.
// The source names below preserve the recovered API/event-sink naming used by
// this file and the event-sink interface map at 0x4d1ba0.
const CLSID g_WestwoodOnlineUpgradeApi_CLSID = {
    0x4dd3baf5,
    0x7579,
    0x11d1,
    {0xb1, 0xc6, 0x00, 0x60, 0x97, 0x17, 0x65, 0x56},
};
const IID g_WestwoodOnlineUpgradeApi_IID = {
    0x4dd3baf4,
    0x7579,
    0x11d1,
    {0xb1, 0xc6, 0x00, 0x60, 0x97, 0x17, 0x65, 0x56},
};
const IID g_WestwoodOnlineUpgradeApiEventSink_IID = {
    0x4dd3baf6,
    0x7579,
    0x11d1,
    {0xb1, 0xc6, 0x00, 0x60, 0x97, 0x17, 0x65, 0x56},
};

// BN 0x4d1ba0 owns the event-sink zCom interface map; 0x43d130 reads the
// first entry's offset field rather than a standalone source global.
extern const zCom::InterfaceMapEntry g_WestwoodOnlineUpgradeApiEventSink_InterfaceMap[2];

namespace {
const unsigned int kWestwoodOnlineUpgradeInitStateSize = sizeof(WestwoodOnlineUpgradeApiInitState);
const unsigned int kFailureMessageBufferSize = 128;
const unsigned int kWolApiFailureCaptionMessageId = 0x3030;
const unsigned int kWolApiFailureTextMessageId = 0x302f;
const UINT kWolApiFailureMessageBoxType = MB_ICONHAND;
const UINT kWestwoodOnlineUpgradeApi_ProgressDialogResourceId = 157;
const int kWestwoodOnlineUpgradeApi_ProgressStatusControlId = 1179;
const int kWestwoodOnlineUpgradeDialogServerControlId = 154;
const unsigned int kWolApiInitConnectingMessageId = 0x3033;
const unsigned int kWolApiInitReadyMessageId = 0x3034;
const unsigned int kWolApiInitFailureCaptionMessageId = 0x3032;
const unsigned int kWolApiInitFailureTextMessageId = 0x3035;
const unsigned int kWolApiFailureMessageBufferSize = 128;
const int kWolLanguageDefault = 0x1100;
const int kWolLanguageGerman = 0x1102;
const int kWolLanguageFrench = 0x1103;
const int kWolProductId = 0x10003;
const int kWolConnectTimeoutSeconds = 60;
const int kWolBootstrapTimeoutSeconds = 30;
const int kWolRequestListMode = 17;
const DWORD kInitialWaitTimeoutMs = 0;
const DWORD kCallbackSleepMs = 300;
const DWORD kBootstrapWaitTimeoutMs = 5;
const DWORD kFailureDisplaySleepMs = 1000;

/**
 * Original inline helper evidence: no standalone retail function exists.
 * Observed in callers 0x43d130 and 0x43d2e0 as inlined localized
 * failure-message copies.
 *
 * Purpose: copy a localized Westwood Online failure message into a stack buffer.
 */
void CopyFailureMessage(
    char *destination,
    const char *source
) {
    strcpy(
        destination,
        source
    );
}

/**
 * Original inline helper evidence: no standalone retail function exists.
 * Observed in caller 0x43d2e0 and dependent WOL dialog/event-sink source as a
 * typed access to the API COM pointer.
 *
 * Purpose: return the global WOL ActiveX API pointer as the recovered provider
 * interface type.
 */
IWestwoodOnlineUpgradeProviderApi *GetApiComObject() {
    return (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
}

/**
 * Original inline helper evidence: no standalone retail function exists.
 * Observed in caller 0x43d2e0 at the progress-dialog DestroyWindow dispatch.
 *
 * Purpose: destroy the modal Westwood Online progress dialog through its MFC
 * provider virtual slot.
 */
void DestroyProgressDialog() {
    CWnd *const progressWnd = (CWnd *)g_pWestwoodOnlineUpgradeProgressDialog;
    progressWnd->DestroyWindow();
}

/**
 * Original inline helper evidence: no standalone retail function exists.
 * Observed in caller 0x43d2e0 as the GetSystemDefaultLangID language switch.
 *
 * Purpose: choose the Westwood Online language id for German, French, or the
 * default localized startup path.
 */
int GetWolLanguageId() {
    const LANGID primaryLanguage = GetSystemDefaultLangID() & 0x3ff;
    if (primaryLanguage == LANG_GERMAN) {
        return kWolLanguageGerman;
    }
    if (primaryLanguage == LANG_FRENCH) {
        return kWolLanguageFrench;
    }
    return kWolLanguageDefault;
}

/**
 * Original inline helper evidence: no standalone retail function exists.
 * Observed in caller 0x43d2e0 as the initial WAIT_TIMEOUT callback pump.
 *
 * Purpose: process WOL API callbacks until the initial connect/status/failure
 * wait set leaves the timeout state.
 */
void PumpInitialCallbacksUntilEvent(
    DWORD *waitResult
) {
    while (*waitResult == WAIT_TIMEOUT) {
        if (g_WestwoodOnlineUpgradeProcessCallbacksFlag != 0 &&
            g_WestwoodOnlineUpgradeApiAsyncErrorFlag == 0) {
            IWestwoodOnlineUpgradeProviderApi *const api = GetApiComObject();
            api->ProcessCallbacks();
        }

        Sleep(kCallbackSleepMs);
        if (g_WestwoodOnlineUpgradeApiAsyncErrorFlag != 0) {
            return;
        }

        *waitResult = WaitForMultipleObjects(
            3,
            g_WestwoodOnlineUpgradeInitWaitEvents,
            FALSE,
            kInitialWaitTimeoutMs
        );
    }
}

/**
 * Original inline helper evidence: no standalone retail function exists.
 * Observed in caller 0x43d2e0 as the bootstrap-server wait loop.
 *
 * Purpose: process WOL API callbacks while waiting for the bootstrap server
 * list/status/failure events.
 */
DWORD PumpBootstrapCallbacksUntilEvent() {
    DWORD waitResult = WaitForMultipleObjects(
        3,
        g_WestwoodOnlineUpgradeInitWaitEvents,
        FALSE,
        kBootstrapWaitTimeoutMs
    );
    while (waitResult == WAIT_TIMEOUT) {
        IWestwoodOnlineUpgradeProviderApi *const api = GetApiComObject();
        api->ProcessCallbacks();
        if (g_WestwoodOnlineUpgradeApiAsyncErrorFlag != 0) {
            break;
        }

        waitResult = WaitForMultipleObjects(
            3,
            g_WestwoodOnlineUpgradeInitWaitEvents,
            FALSE,
            kBootstrapWaitTimeoutMs
        );
    }
    return waitResult;
}
} // namespace




#include "Battlesport/wol_api_event_sink.h"

#include "Battlesport/game_net.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/recoil_app.h"
#include "Battlesport/wol_api.h"
#include "Battlesport/wol_dialog.h"
#include "GameZRecoil/Time/time.h"
#include "Battlesport/wol_download.h"
#include "GameZRecoil/zCom/zCom.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zNetwork/znet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

namespace {
const int kPendingSessionRemovedStatusBufferSize = 128;
const unsigned int kPendingSessionRemovedStatusMessageId = 0x3004;
const int kWestwoodOnlineUpgradeApiEventSink_SessionResultsListId = 1137;
const char kWestwoodOnlineUpgradeServerErrorTitle[] = "ServerError";
const int kApiStatusTextBufferSize = 128;
const char kApiStatusLineDelimiter[] = "\r";
const int kApiStatusActiveListMode = 17;
const int kApiStatusNoResultsMessageId = 0x3005;
const int kApiStatusOneResultMessageId = 0x3006;
const int kApiStatusMultipleResultsMessageId = 0x3007;
const int kApiStatusFailureCaptionMessageId = 0x3003;
const int kApiStatusFailureDefaultMessageId = 0x300c;
const int kApiStatusFailure64MessageId = 0x3008;
const int kApiStatusFailure65MessageId = 0x3009;
const int kApiStatusFailure6aMessageId = 0x300b;
const int kApiStatusFailure72MessageId = 0x300a;
const int kApiStatusFailure64 = (int)0x80040064;
const int kApiStatusFailure65 = (int)0x80040065;
const int kApiStatusFailure6a = (int)0x8004006a;
const int kApiStatusFailure72 = (int)0x80040072;
const int kBrowseRecordAddedStatusBufferSize = 128;
const unsigned int kBrowseRecordAddedFailureMessageId = 0x300d;
const unsigned int kBrowseRecordAddedOpenMessageId = 0x300e;
const unsigned int kBrowseRecordAddedClosedMessageId = 0x300f;
const int kWestwoodOnlineUpgradeApiEventSink_BrowseRecordListId = 1136;
const int kWestwoodOnlineUpgradeGameButtonId = 1174;
const int kBrowseSessionResolvedStatusBufferSize = 128;
const unsigned int kBrowseSessionResolvedFailurePrefixMessageId = 0x3011;
const unsigned int kBrowseSessionResolvedFailure6cMessageId = 0x3012;
const unsigned int kBrowseSessionResolvedFailure70MessageId = 0x3013;
const unsigned int kBrowseSessionResolvedFailure72MessageId = 0x300a;
const unsigned int kBrowseSessionResolvedFailure71MessageId = 0x3014;
const unsigned int kBrowseSessionResolvedFailure6eMessageId = 0x3010;
const unsigned int kBrowseSessionResolvedStatusMessageId = 0x3015;
const unsigned int kSessionQueryFinishedStatusMessageId = 0x3016;
const unsigned int kSessionListEnumeratedReadyMessageId = 0x3017;
const unsigned int kSessionListEnumeratedClosedMessageId = 0x3018;
const unsigned int kSessionListEnumeratedPendingMessageId = 0x3019;
const int kBrowseSessionResolvedFailure6c = (int)0x8004006c;
const int kBrowseSessionResolvedFailure70 = (int)0x80040070;
const int kBrowseSessionResolvedFailure72 = (int)0x80040072;
const int kBrowseSessionResolvedFailure71 = (int)0x80040071;
const int kBrowseSessionResolvedFailure6e = (int)0x8004006e;
const int kSessionRequestRefreshCacheFlag = 0x8000;
const int kSessionRequestSkipDetailsFlag = 1;
const int kSessionListEnumeratedResultTextBufferSize = 256;
const int kSessionRequestStatusBufferSize = 128;
const unsigned int kSessionRequestStatus301BMessageId = 0x301b;
const unsigned int kSessionRequestStatus301CMessageId = 0x301c;
const unsigned int kSessionRequestStatus301DMessageId = 0x301d;
const unsigned int kConnectStatusDefaultMessageId = 0x301e;
const unsigned int kConnectStatusCode40134MessageId = 0x301f;
const unsigned int kConnectStatusCode40133MessageId = 0x3020;
const unsigned int kConnectStatusFallbackMessageId = 0x3021;
const int kConnectStatusCode40134 = 0x40134;
const int kConnectStatusCode40133 = 0x40133;
const unsigned int kBrowseRecordStatusNamedMessageId = 0x3022;
const unsigned int kBrowseRecordStatusCode40131MessageId = 0x3023;
const unsigned int kBrowseRecordStatusCode40130MessageId = 0x3020;
const unsigned int kBrowseRecordStatusCode40132MessageId = 0x3024;
const unsigned int kBrowseRecordStatusFallbackMessageId = 0x3025;
const int kBrowseRecordStatusCode40131 = 0x40131;
const int kBrowseRecordStatusCode40130 = 0x40130;
const int kBrowseRecordStatusCode40132 = 0x40132;
const unsigned int kValueStatus3026MessageId = 0x3026;
const unsigned int kTimeStatus302AMessageId = 0x302a;
const unsigned int kValueStatus302BMessageId = 0x302b;
const unsigned int kValueStatus302CMessageId = 0x302c;
const int kSessionResultRowTextBufferSize = 40;
const char kSessionResultReadySuffix[] = " +";
const char kSessionResultPendingSuffix[] = " *";
const unsigned int kSessionLaunchResultResetMessageId = 0x302d;
const unsigned int kSessionLaunchResultRemoveMessageId = 0x302e;
const int kNetworkStatusConnectError = (int)0x80040069;
const int kNetworkStatusNetworkDown = (int)0x80040067;
const int kNetworkStatusLookupFailed = (int)0x80040068;
const int kNetworkStatusTimeout = (int)0x8004006a;
const int kNetworkStatusConnecting = 0x4012c;
const int kNetworkStatusConnected = 0x4012d;
const int kNetworkStatusDisconnecting = 0x4012e;
const int kNetworkStatusDisconnected = 0x4012f;
const char kNetworkStatusDebugFormat[] = "Net Status = %s (%d)\n";
const char kNetworkStatusConnectErrorText[] = "CHAT_E_CON_ERROR";
const char kNetworkStatusNetworkDownText[] = "CHAT_E_CON_NETDOWN";
const char kNetworkStatusLookupFailedText[] = "CHAT_E_CON_LOOKUP_FAILED";
const char kNetworkStatusTimeoutText[] = "CHAT_E_TIMEOUT";
const char kNetworkStatusConnectingText[] = "CHAT_S_CON_CONNECTING";
const char kNetworkStatusConnectedText[] = "CHAT_S_CON_CONNECTED";
const char kNetworkStatusDisconnectingText[] = "CHAT_S_CON_DISCONNECTING";
const char kNetworkStatusDisconnectedText[] = "CHAT_S_CON_DISCONNECTED";
const char kNetworkStatusUnknownText[] = "CHAT_S_CON_UNKNOWN";
const char kSpaceDelimiter[] = " ";
const char kBrowseSessionResolvedStatusCodeFmt[] = "%s %x";
const int kBrowseRecordListRowTextBufferSize = 256;
const unsigned int kBrowseRecordListOpenMessageId = 0x3027;
const unsigned int kBrowseRecordListClosedMessageId = 0x3028;
const int kBrowseRecordLatencyUnknown = -1;
const int kBrowseRecordLatencyStep0Max = 250;
const int kBrowseRecordLatencyStep1Max = 500;
const int kBrowseRecordLatencyStep2Max = 750;
const int kBrowseRecordLatencyStep3Max = 1000;
const int kBrowseRecordLatencyStep4Max = 1250;
const int kBrowseRecordLatencyStep5Max = 1500;
const int kBrowseRecordLatencyStep6Max = 1750;
const char kBrowseRecordLatencyUnknownText[] = "??";
const char kBrowseRecordLatencyStep0Text[] = " | ";
const char kBrowseRecordLatencyStep1Text[] = " ||| ";
const char kBrowseRecordLatencyStep2Text[] = " ||||| ";
const char kBrowseRecordLatencyStep3Text[] = " ||||||| ";
const char kBrowseRecordLatencyStep4Text[] = " ||||||||| ";
const char kBrowseRecordLatencyStep5Text[] = " ||||||||||| ";
const char kBrowseRecordLatencyStep6Text[] = " ||||||||||||| ";
const char kBrowseRecordLatencyStep7Text[] = " ||||||||||||||| ";
const unsigned int kBootstrapServerCopiedBytes = 0xf8;
RECOIL_STATIC_ASSERT(
    sizeof(WestwoodOnlineUpgradeBootstrapServerRecord) == kBootstrapServerCopiedBytes
);

/**
 * Original helper evidence: no standalone retail function; observed in
 * event-sink callback callers in this source file.
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeApiEventSink.cpp
 * Purpose: Returns the shared Westwood Online upgrade provider API callback interface.
 */
IWestwoodOnlineUpgradeProviderApiCallbacks *GetCallbackApiComObject() {
    return (IWestwoodOnlineUpgradeProviderApiCallbacks *)g_pWestwoodOnlineUpgradeApi;
}
} // namespace

/**
 * Reimplements data 0x4d1ba0: g_WestwoodOnlineUpgradeApiEventSink_InterfaceMap.
 * Purpose: Describes the single direct COM interface exposed by the API event sink.
 */
extern const zCom::InterfaceMapEntry g_WestwoodOnlineUpgradeApiEventSink_InterfaceMap[2] = {
    {&g_WestwoodOnlineUpgradeApiEventSink_IID, 0, zCom::ZCOM_INTERFACE_MAP_DIRECT},
    {0, 0, zCom::ZCOM_INTERFACE_MAP_END},
};

/**
 * Original helper evidence: no standalone retail function; observed in the
 * 0x441660 COM QueryInterface body.
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeApiEventSink.cpp
 * Purpose: Performs the shared interface-map lookup for the event sink object.
 */
HRESULT __stdcall WestwoodOnlineUpgradeApiEventSink::QueryInterface(
    WestwoodOnlineUpgradeApiEventSink *self,
    REFIID iid,
    void **outInterface
) {
    return zCom::QueryInterfaceFromInterfaceMap(
        self,
        g_WestwoodOnlineUpgradeApiEventSink_InterfaceMap,
        &iid,
        outInterface
    );
}


#include "Battlesport/wol_ref_count_and_lock.h"



#include "Battlesport/wol_config_dialog.h"

#include "Battlesport/wol_api.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zLoc/zloc.h"

#include <string.h>

/**
 * Provider-boundary accessor for imported MFC42 CDialog members; this does not reimplement
 * provider behavior.
 */
class CDialogProviderAccessor : public CDialog {
  public:
    static const AFX_MSGMAP *__stdcall GetMessageMap();
    void CallOnOK();
};

RECOIL_STATIC_ASSERT(sizeof(CWnd) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CDialog) == 0x60);
RECOIL_STATIC_ASSERT(sizeof(CEdit) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CComboBox) == 0x40);

void __stdcall DDX_Control(
    CDataExchange *dataExchange,
    int controlId,
    CWnd &control
);
void __stdcall DDX_Text(
    CDataExchange *dataExchange,
    int controlId,
    CString &value
);
void __stdcall DDX_Check(
    CDataExchange *dataExchange,
    int controlId,
    int &value
);

namespace {
const UINT kWestwoodOnlineUpgradeConfigDialogResourceId = 156;
const char kEmptyString[] = "";
const int kWestwoodOnlineUpgradeConfigProfileComboId = 1192;
const int kWestwoodOnlineUpgradeConfigConnectStringEditId = 1173;
const int kWestwoodOnlineUpgradeConfigRememberPasswordCheckId = 1182;
const UINT kWestwoodOnlineUpgradeConfigDialog_MfcMessageMapSigVoid = 12;
const unsigned int kWestwoodOnlineUpgradeConfigUnnamedProfileMessageId = 0x3044;
const int kSelectedProfileTextBufferLength = 32;
const int kDialogOkResult = 1;
const unsigned int kStackStorageUnitSize = sizeof(unsigned int);

/**
 * Original helper evidence: no standalone retail function; observed at
 * callers 0x441cb0 and 0x441f40 source-cluster cleanup sites.
 * Purpose: centralizes config-dialog destructor dispatch for local stack objects.
 */
void DestructConfigDialog(
    WestwoodOnlineUpgradeConfigDialog *dialog
) {
    dialog->Destructor();
}
} // namespace

/**
 * Provider-boundary: imported MFC42 CDialog message map accessor.
 * Purpose: exposes the CDialog base message map for the recovered MFC map chain.
 */
const AFX_MSGMAP *__stdcall CDialogProviderAccessor::GetMessageMap() {
    return &CDialog::messageMap;
}

/**
 * Provider-boundary: imported MFC42 CDialog::OnOK member call.
 * Purpose: routes accepted dialog completion to the provider base class.
 */
void CDialogProviderAccessor::CallOnOK() {
    CDialog::OnOK();
}

/**
 * Original helper evidence: no standalone retail function; used by the MFC
 * message-map data for WestwoodOnlineUpgradeConfigDialog.
 * Purpose: returns the provider CDialog base message map.
 */
const AFX_MSGMAP *__stdcall WestwoodOnlineUpgradeConfigDialog::GetBaseMessageMapForMfc() {
    return CDialogProviderAccessor::GetMessageMap();
}

AFX_MSGMAP_ENTRY const WestwoodOnlineUpgradeConfigDialog::messageEntries[] = {
    {WM_COMMAND,
        EN_SETFOCUS,
        kWestwoodOnlineUpgradeConfigConnectStringEditId,
        kWestwoodOnlineUpgradeConfigConnectStringEditId,
        kWestwoodOnlineUpgradeConfigDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeConfigDialog::OnConnectStringEditSetFocusClear},
    {WM_COMMAND,
        CBN_KILLFOCUS,
        kWestwoodOnlineUpgradeConfigProfileComboId,
        kWestwoodOnlineUpgradeConfigProfileComboId,
        kWestwoodOnlineUpgradeConfigDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeConfigDialog::OnProfileComboKillFocus},
    {WM_COMMAND,
        CBN_SELCHANGE,
        kWestwoodOnlineUpgradeConfigProfileComboId,
        kWestwoodOnlineUpgradeConfigProfileComboId,
        kWestwoodOnlineUpgradeConfigDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeConfigDialog::OnProfileComboSelChange},
    {WM_COMMAND,
        CBN_EDITCHANGE,
        kWestwoodOnlineUpgradeConfigProfileComboId,
        kWestwoodOnlineUpgradeConfigProfileComboId,
        kWestwoodOnlineUpgradeConfigDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeConfigDialog::OnProfileComboEditChange},
    {WM_COMMAND,
        CBN_DROPDOWN,
        kWestwoodOnlineUpgradeConfigProfileComboId,
        kWestwoodOnlineUpgradeConfigProfileComboId,
        kWestwoodOnlineUpgradeConfigDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeConfigDialog::OnProfileComboDropdown},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeConfigRememberPasswordCheckId,
        kWestwoodOnlineUpgradeConfigRememberPasswordCheckId,
        kWestwoodOnlineUpgradeConfigDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeConfigDialog::OnConnectStringModeClicked},
    {WM_COMMAND,
        EN_KILLFOCUS,
        kWestwoodOnlineUpgradeConfigConnectStringEditId,
        kWestwoodOnlineUpgradeConfigConnectStringEditId,
        kWestwoodOnlineUpgradeConfigDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeConfigDialog::OnConnectStringEditKillFocus},
    {0, 0, 0, 0, 0, 0},
};

const AFX_MSGMAP WestwoodOnlineUpgradeConfigDialog::messageMap = {
    &WestwoodOnlineUpgradeConfigDialog::GetBaseMessageMapForMfc,
    &WestwoodOnlineUpgradeConfigDialog::messageEntries[0],
};

/**
 * Original helper evidence: no standalone retail function; reconstructed
 * callers still use the recovered Constructor helper spelling while the owner
 * model is the real C++ constructor above.
 * Purpose: placement-construct the config dialog and return self.
 */
WestwoodOnlineUpgradeConfigDialog * WestwoodOnlineUpgradeConfigDialog::Constructor(
    CWnd *parentWnd
) {
    new (this) WestwoodOnlineUpgradeConfigDialog(parentWnd);
    return this;
}

/**
 * Original helper evidence: no standalone retail function; reconstructed
 * callers still use the recovered destructor helper spelling while the owner
 * model is the real C++ destructor above.
 * Purpose: invoke the real config dialog destructor.
 */
void WestwoodOnlineUpgradeConfigDialog::Destructor() {
    this->WestwoodOnlineUpgradeConfigDialog::~WestwoodOnlineUpgradeConfigDialog();
}



#include "Battlesport/wol_dialog.h"

#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/recoil_app.h"
#include "Battlesport/wol_api.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zLoc/zloc.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Provider-boundary accessor for imported MFC42 CDialog::OnCancel; this does not reimplement
 * CDialog behavior.
 */
class CDialogCancelAccessor : public CDialog {
  public:
    void CallBaseOnCancel();
};

/**
 * Provider-boundary accessor for imported MFC42 CDialog metadata; this does not reimplement
 * CDialog behavior.
 */
class WestwoodOnlineUpgradeCDialogMessageMapAccessor : public CDialog {
  public:
    static const AFX_MSGMAP *__stdcall GetMessageMap();
};

/**
 * Provider-boundary accessor for imported MFC42 CWnd::Default; this does not reimplement
 * CWnd behavior.
 */
class WestwoodOnlineUpgradeCWndAccess : public CWnd {
  public:
    long CallDefault();
};

void __stdcall DDX_Control(
    CDataExchange *dataExchange,
    int controlId,
    CWnd &control
);
void __stdcall DDX_Text(
    CDataExchange *dataExchange,
    int controlId,
    int &value
);
void __stdcall DDX_Check(
    CDataExchange *dataExchange,
    int controlId,
    int &value
);

RECOIL_STATIC_ASSERT(sizeof(CWnd) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CDialog) == 0x60);
RECOIL_STATIC_ASSERT(sizeof(CEdit) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CButton) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CListBox) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CComboBox) == 0x40);

/**
 * Original helper evidence: no standalone retail function; observed in
 * WestwoodOnlineUpgradeDialog::SetAbortAndClose.
 * Purpose: call imported MFC42 CDialog::OnCancel for the dialog wrapper.
 */
void CDialogCancelAccessor::CallBaseOnCancel() {
    CDialog::OnCancel();
}

/**
 * Original helper evidence: no standalone retail function; observed in the
 * WestwoodOnlineUpgradeDialog MFC message-map chain.
 * Purpose: expose imported MFC42 CDialog message-map metadata to the dialog.
 */
const AFX_MSGMAP *__stdcall WestwoodOnlineUpgradeCDialogMessageMapAccessor::GetMessageMap() {
    return &CDialog::messageMap;
}

/**
 * Original helper evidence: no standalone retail function; observed in
 * WestwoodOnlineUpgradeDialog::OnRefreshListTimer.
 * Purpose: call imported MFC42 CWnd::Default for timer message fallthrough.
 */
long WestwoodOnlineUpgradeCWndAccess::CallDefault() {
    return CWnd::Default();
}

extern "C" {
/**
 * Reimplements data 0x4f5238: g_hWestwoodOnlineUpgradeModuleInstance.
 * Purpose: stores the module instance passed to
 * WestwoodOnlineUpgradeDialog::ShowModalAndGetSelectedMissionIndex for the
 * stack-owned upgrade dialog lifetime.
 */
HINSTANCE g_hWestwoodOnlineUpgradeModuleInstance = 0;
/**
 * Reimplements data 0x4f4230: g_pWestwoodOnlineUpgradeProgressDialog.
 * Purpose: stores the active stack-owned progress dialog while the
 * Westwood online upgrade modal flow is running.
 */
WestwoodOnlineUpgradeProgressDialog *g_pWestwoodOnlineUpgradeProgressDialog = 0;
/**
 * Reimplements data 0x538568: g_pWestwoodOnlineUpgradeDialog.
 * Purpose: stores the active stack-owned main upgrade dialog used by static
 * callbacks and member routines during the modal flow.
 */
WestwoodOnlineUpgradeDialog *g_pWestwoodOnlineUpgradeDialog = 0;
/**
 * Reimplements data 0x4f53c0: g_WestwoodOnlineUpgradeSelectedMissionIndex.
 * Purpose: stores the mission index selected by the Westwood online upgrade
 * dialog before ShowModalAndGetSelectedMissionIndex returns it.
 */
int g_WestwoodOnlineUpgradeSelectedMissionIndex = 0;
/**
 * Reimplements data 0x538990: g_WestwoodOnlineUpgradeStatusAppendBuffer.
 * Purpose: holds the reusable formatted status text buffer consumed by
 * WestwoodOnlineUpgradeDialog::AppendStatusTextFmt.
 */
char g_WestwoodOnlineUpgradeStatusAppendBuffer[1024] = "";
}

/**
 * Reimplements data 0x4dd25c: g_WestwoodOnlineUpgradeSessionQueryPayloadFmt.
 * Purpose: CString::Format payload used by
 * WestwoodOnlineUpgradeDialog::UpdateSessionListQueryFromControls to encode
 * the provider session-list query fields.
 */
char g_WestwoodOnlineUpgradeSessionQueryPayloadFmt[] = "%1d%4d%4d%1d%1d%1d";
/**
 * Reimplements data 0x4dd270: g_WestwoodOnlineUpgradeSessionQueryDisplayFmt.
 * Purpose: status-list format used when submitting typed status text without
 * selected session rows.
 */
char g_WestwoodOnlineUpgradeSessionQueryDisplayFmt[] = "{ %s } %s";
/**
 * Reimplements data 0x4dd27c: g_WestwoodOnlineUpgradeSingleDigitFieldMaxText_4.
 * Purpose: normalized max-player edit text for the upper single-digit query
 * bound.
 */
char g_WestwoodOnlineUpgradeSingleDigitFieldMaxText_4[] = "4";
/**
 * Reimplements data 0x4dd280: g_WestwoodOnlineUpgradeSingleDigitFieldMinText_2.
 * Purpose: normalized edit text for the lower single-digit query bound shared
 * by max-player and value/time controls.
 */
char g_WestwoodOnlineUpgradeSingleDigitFieldMinText_2[] = "2";
/**
 * Reimplements data 0x4dd284: g_WestwoodOnlineUpgradeMaxPlayersMaxText_1000.
 * Purpose: normalized auxiliary-parameter edit text for the 1000 upper bound.
 */
char g_WestwoodOnlineUpgradeMaxPlayersMaxText_1000[] = "1000";
/**
 * Reimplements data 0x4dd28c: g_WestwoodOnlineUpgradeMaxPlayersMinText_1.
 * Purpose: normalized auxiliary-parameter edit text for the lower query bound.
 */
char g_WestwoodOnlineUpgradeMaxPlayersMinText_1[] = "1";
/**
 * Reimplements data 0x4dd290: g_WestwoodOnlineUpgradeAuxParamMaxText_2000.
 * Purpose: normalized value/time edit text for the 2000 upper bound.
 */
char g_WestwoodOnlineUpgradeAuxParamMaxText_2000[] = "2000";

namespace {
const unsigned int kStatusAppendBufferSize = 1024;
const int kStatusListMaxLineCount = 100;
const int kStatusListScrollThreshold = 9;
const int kStatusListScrollBackCount = 8;
const unsigned int kConnectStatusBufferSize = 128;
const unsigned int kConnectStatusMessageId = 0x3036;
const UINT kWestwoodOnlineUpgradeDialogResourceId = 154;
const UINT kWestwoodOnlineUpgradeDialog_ProgressDialogResourceId = 157;
const int kWestwoodOnlineUpgradeDialog_ProgressStatusControlId = 1179;
const int kWestwoodOnlineUpgradeServerAddressEditId = 1176;
const int kWestwoodOnlineUpgradeStatusTokenEditId = 1139;
const int kWestwoodOnlineUpgradeQueryValueOrTimeEditId = 1168;
const int kWestwoodOnlineUpgradeQueryMaxPlayersEditId = 1170;
const int kWestwoodOnlineUpgradeQueryAuxParamEditId = 1169;
const int kWestwoodOnlineUpgradeQueryStatusFlag1CheckId = 1123;
const int kWestwoodOnlineUpgradeQueryStatusFlag0CheckId = 1122;
const int kWestwoodOnlineUpgradeSubmitPendingSessionListButtonId = 1151;
const int kWestwoodOnlineUpgradeConnectButtonId = 1150;
const int kWestwoodOnlineUpgradeQueryValueOrTimeLabelId = 1167;
const int kWestwoodOnlineUpgradeQuerySessionsByNameButtonId = 1160;
const int kWestwoodOnlineUpgradeQueueVisibleSessionRequestsButtonId = 1051;
const int kWestwoodOnlineUpgradeStatusListId = 1141;
const int kWestwoodOnlineUpgradeSessionModeComboId = 1171;
const int kWestwoodOnlineUpgradeDialog_SessionResultsListId = 1137;
const int kWestwoodOnlineUpgradeStatusServerEditId = 1138;
const int kWestwoodOnlineUpgradeSessionNameEditId = 1148;
const int kWestwoodOnlineUpgradeDialog_BrowseRecordListId = 1136;
const int kWestwoodOnlineUpgradeResetBrowseRecordButtonId = 1150;
const int kWestwoodOnlineUpgradeDisconnectButtonId = 1166;
const int kWestwoodOnlineUpgradeBeginConnectButtonId = 1163;
const int kWestwoodOnlineUpgradeCheckUpgradeButtonId = 1162;
const int kWestwoodOnlineUpgradeQueryStatusButtonId = 1161;
const int kWestwoodOnlineUpgradeActiveListModeButtonId = 1144;
const int kWestwoodOnlineUpgradeRefreshCurrentQueryButtonId = 1143;
const int kWestwoodOnlineUpgradeQuerySessionsByNameCommandId = 1145;
const int kWestwoodOnlineUpgradeQueueVisibleSessionRequestsCommandId = 1160;
const int kWestwoodOnlineUpgradeQueueVisibleLookupCommandId = 1051;
const int kWestwoodOnlineUpgradeListMode0ButtonId = 1154;
const int kWestwoodOnlineUpgradeListMode11ButtonId = 1155;
const int kWestwoodOnlineUpgradeStatusCaptionLabelId = 1125;
const int kWestwoodOnlineUpgradeAutoRefreshListMode = 17;
const float kWestwoodOnlineUpgradeAutoRefreshIntervalSec = 60.0f;
const unsigned int kWolDisconnectStatusMessageId = 0x301a;
const DWORD kWolDestroyCallbackSleepMs = 200;
const UINT_PTR kWolRefreshListTimerId = 4;
const UINT kWolRefreshListTimerMs = 100;
const int kWolConnectRequestTextMaxChars = 9;
const WPARAM kWolSessionNameEditLimit = 17;
const WPARAM kWolServerAddressEditLimit = 9;
const WPARAM kWolStatusServerEditLimit = 100;
const WPARAM kWolStatusTokenEditLimit = 10;
const WPARAM kWolStatusListHorizontalExtent = 1500;
const int kWolQueryStatusTokenTextBufferSize = 20;
const int kWolQueryStatusTokenTextMaxChars = 19;
const int kWolQueryStatusServerTextBufferSize = 80;
const int kWolQueryStatusServerTextMaxChars = 79;
const int kWolQuerySessionsByNameServerTextMaxChars = 8;
const int kWolVisibleSessionSelectionLimit = 1024;
const int kWolQueryStatusMessageBoxTextBufferSize = 128;
const int kWolQueryStatusMessageBoxTitleBufferSize = 128;
const char kWolQueryStatusTokenDelimiter[] = " ";
const int kWolQuerySessionNameCopyMaxChars = 16;
const int kWolQuerySessionsByNameListMode = 17;
const int kWolSessionModeCount = 7;
const unsigned int kWolSessionModeFirstMessageId = 0x2036;
const unsigned int kWolDialogDefaultSessionNameMessageId = 0x3034;
const unsigned int kWolStatusCaptionMessageId = 0x3037;
const int kWolMinPlayersPerSession = 2;
const int kWolMaxPlayersPerSession = 4;
const int kWolMinQueryAuxParam = 1;
const int kWolMaxQueryAuxParam = 1000;
const int kWolMinQueryValueOrTime = 2;
const int kWolMaxQueryValueOrTime = 2000;
const int kWolEditTextRadix = 10;
const unsigned int kWolEditTextBufferSize = 16;
const unsigned int kWolBeginConnectMode0MessageId = 0x303b;
const unsigned int kWolBeginConnectMode1MessageId = 0x303c;
const unsigned int kWolQueryStatusErrorTitleMessageId = 0x3003;
const unsigned int kWolQueryStatusMissingTokenMessageId = 0x303d;
const unsigned int kWolQuerySessionSubmitFailedMessageId = 0x303e;
const unsigned int kWolQuerySessionNameRequiredMessageId = 0x303f;
const unsigned int kWolQuerySessionModeTimeLabelMessageId = 0x3040;
const unsigned int kWolQuerySessionModeValueLabelMessageId = 0x3041;
const unsigned int kWolQuerySessionDuplicateMessageId = 0x3042;
const unsigned int kWolSubmitSessionRequestStatusMessageId = 0x3038;
const HRESULT kWolQueryDuplicateNameResult = 0x800401f7;
const HRESULT kWolQuerySubmitFailedResult = 0x800401f8;
const UINT kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid = 12;
const UINT kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoidUInt = 13;

RECOIL_STATIC_ASSERT(sizeof(g_WestwoodOnlineUpgradeStatusAppendBuffer) == kStatusAppendBufferSize);
RECOIL_STATIC_ASSERT(sizeof(g_WestwoodOnlineUpgradeSessionQueryPayloadFmt) == 0x13);
RECOIL_STATIC_ASSERT(sizeof(g_WestwoodOnlineUpgradeSessionQueryDisplayFmt) == 0x0a);
RECOIL_STATIC_ASSERT(sizeof(g_WestwoodOnlineUpgradeSingleDigitFieldMaxText_4) == 0x02);
RECOIL_STATIC_ASSERT(sizeof(g_WestwoodOnlineUpgradeSingleDigitFieldMinText_2) == 0x02);
RECOIL_STATIC_ASSERT(sizeof(g_WestwoodOnlineUpgradeMaxPlayersMaxText_1000) == 0x05);
RECOIL_STATIC_ASSERT(sizeof(g_WestwoodOnlineUpgradeMaxPlayersMinText_1) == 0x02);
RECOIL_STATIC_ASSERT(sizeof(g_WestwoodOnlineUpgradeAuxParamMaxText_2000) == 0x05);

} // namespace

/**
 * Original helper evidence: no standalone retail function; observed in the
 * WestwoodOnlineUpgradeDialog MFC message-map chain.
 * Purpose: return the imported CDialog base message map for MFC dispatch.
 */
const AFX_MSGMAP *__stdcall WestwoodOnlineUpgradeDialog::GetBaseMessageMapForMfc() {
    return WestwoodOnlineUpgradeCDialogMessageMapAccessor::GetMessageMap();
}

AFX_MSGMAP_ENTRY const WestwoodOnlineUpgradeDialog::messageEntries[] = {
    {WM_TIMER,
        0,
        0,
        0,
        kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoidUInt,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnRefreshListTimer},
    {WM_DESTROY, 0, 0, 0, kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid, (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnDestroy},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeResetBrowseRecordButtonId,
        kWestwoodOnlineUpgradeResetBrowseRecordButtonId,
        kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::ResetSelectedBrowseRecordAndRefreshList},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeDisconnectButtonId,
        kWestwoodOnlineUpgradeDisconnectButtonId,
        kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::BeginDisconnectAndShowProgress},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeBeginConnectButtonId,
        kWestwoodOnlineUpgradeBeginConnectButtonId,
        kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::BeginConnect},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeCheckUpgradeButtonId,
        kWestwoodOnlineUpgradeCheckUpgradeButtonId,
        kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::CheckAndApplyUpgrade},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeQueryStatusButtonId,
        kWestwoodOnlineUpgradeQueryStatusButtonId,
        kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::QueryStatus},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeActiveListModeButtonId,
        kWestwoodOnlineUpgradeActiveListModeButtonId,
        kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::RequestActiveListMode},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeRefreshCurrentQueryButtonId,
        kWestwoodOnlineUpgradeRefreshCurrentQueryButtonId,
        kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnRefreshCurrentQuery},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeQuerySessionsByNameCommandId,
        kWestwoodOnlineUpgradeQuerySessionsByNameCommandId,
        kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnQuerySessionsByName},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeQueueVisibleSessionRequestsCommandId,
        kWestwoodOnlineUpgradeQueueVisibleSessionRequestsCommandId,
        kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::QueueVisibleSessionRequests},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeQueueVisibleLookupCommandId,
        kWestwoodOnlineUpgradeQueueVisibleLookupCommandId,
        kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::QueueVisibleSessionRequestsAndLookupBrowseRecords},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeListMode0ButtonId,
        kWestwoodOnlineUpgradeListMode0ButtonId,
        kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::RequestListMode0},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeListMode11ButtonId,
        kWestwoodOnlineUpgradeListMode11ButtonId,
        kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::RequestListMode11},
    {WM_COMMAND,
        CBN_SELCHANGE,
        kWestwoodOnlineUpgradeSessionModeComboId,
        kWestwoodOnlineUpgradeSessionModeComboId,
        kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnSessionModeComboSelChange},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeQueryStatusFlag0CheckId,
        kWestwoodOnlineUpgradeQueryStatusFlag0CheckId,
        kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnQueryControlsChanged},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeQueryStatusFlag1CheckId,
        kWestwoodOnlineUpgradeQueryStatusFlag1CheckId,
        kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnQueryControlsChanged},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeSubmitPendingSessionListButtonId,
        kWestwoodOnlineUpgradeSubmitPendingSessionListButtonId,
        kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::SubmitPendingSessionListFromResults},
    {WM_COMMAND,
        EN_CHANGE,
        kWestwoodOnlineUpgradeQueryValueOrTimeEditId,
        kWestwoodOnlineUpgradeQueryValueOrTimeEditId,
        kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnQueryControlsChanged},
    {WM_COMMAND,
        EN_CHANGE,
        kWestwoodOnlineUpgradeQueryAuxParamEditId,
        kWestwoodOnlineUpgradeQueryAuxParamEditId,
        kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnQueryControlsChanged},
    {WM_COMMAND,
        EN_CHANGE,
        kWestwoodOnlineUpgradeQueryMaxPlayersEditId,
        kWestwoodOnlineUpgradeQueryMaxPlayersEditId,
        kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnMaxPlayersEditChange},
    {WM_COMMAND,
        LBN_DBLCLK,
        kWestwoodOnlineUpgradeDialog_BrowseRecordListId,
        kWestwoodOnlineUpgradeDialog_BrowseRecordListId,
        kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnBrowseRecordListDblClk},
    {WM_COMMAND,
        EN_KILLFOCUS,
        kWestwoodOnlineUpgradeQueryMaxPlayersEditId,
        kWestwoodOnlineUpgradeQueryMaxPlayersEditId,
        kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnMaxPlayersEditKillFocus},
    {WM_COMMAND,
        EN_KILLFOCUS,
        kWestwoodOnlineUpgradeQueryAuxParamEditId,
        kWestwoodOnlineUpgradeQueryAuxParamEditId,
        kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnAuxParamEditKillFocus},
    {WM_COMMAND,
        EN_KILLFOCUS,
        kWestwoodOnlineUpgradeQueryValueOrTimeEditId,
        kWestwoodOnlineUpgradeQueryValueOrTimeEditId,
        kWestwoodOnlineUpgradeDialog_MfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnValueOrTimeEditKillFocus},
    {0, 0, 0, 0, 0, 0},
};

const AFX_MSGMAP WestwoodOnlineUpgradeDialog::messageMap = {
    &WestwoodOnlineUpgradeDialog::GetBaseMessageMapForMfc,
    &WestwoodOnlineUpgradeDialog::messageEntries[0],
};

/**
 * Original helper evidence: no standalone retail function; recovered callers
 * still use this placement-constructor helper while the owner model is the
 * real C++ constructor above.
 * Purpose: placement-construct the main WOL upgrade dialog and return self.
 */
WestwoodOnlineUpgradeDialog * WestwoodOnlineUpgradeDialog::Constructor(
    CWnd *parentWnd
) {
    new (this) WestwoodOnlineUpgradeDialog(parentWnd);
    return this;
}



#include "Battlesport/wol_dialog.h"

#include "Battlesport/wol_download.h"
#include "GameZRecoil/zLoc/zloc.h"

#include <stdarg.h>
#include <stdio.h>

RECOIL_STATIC_ASSERT(sizeof(CWnd) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CDialog) == 0x60);

/**
 * Provider-boundary accessor for imported MFC42 CDialog metadata; this does not reimplement
 * CDialog behavior.
 */
class WestwoodOnlineUpgradeProgressCDialogMessageMapAccessor : public CDialog {
  public:
    static const AFX_MSGMAP *__stdcall GetMessageMap();
};

namespace {
const UINT kWestwoodOnlineUpgradeProgressDialog_ResourceId = 157;
const unsigned int kProgressStatusControlId = 1024;
const unsigned int kProgressStatusTextControlId = 1023;
const unsigned int kProgressTimerId = 1;
const unsigned int kProgressTimerMs = 50;
const unsigned int kDownloadPathBufferSize = 256;
const unsigned int kDownloadPromptBufferSize = 128;
const unsigned int kDownloadPromptMessageId = 0x3043;
const unsigned int kDownloadDialogResourceId = 162;
const char kDownloadSourcePathFormat[] = "%s\\%s";
const char kWestwoodOnlineUpgradeRegistryKey[] = "SOFTWARE\\Westwood\\Recoil";

} // namespace

extern "C" HINSTANCE g_RecoilApp_hInstance;
extern "C" HWND g_RecoilApp_hWndMain;
extern "C" char g_WestwoodOnlineUpgradeProgressStatusTextBuffer[0x40] = "";

/**
 * Provider-boundary: imported MFC42 CDialog message map accessor.
 * Purpose: exposes the CDialog base message map.
 */
const AFX_MSGMAP *__stdcall WestwoodOnlineUpgradeProgressCDialogMessageMapAccessor::GetMessageMap() {
    return &CDialog::messageMap;
}
/**
 * Original helper evidence: no standalone retail function; used by the progress-dialog MFC message-map data.
 * Purpose: returns the provider CDialog base map.
 */
const AFX_MSGMAP *__stdcall WestwoodOnlineUpgradeProgressDialog::GetBaseMessageMapForMfc() {
    return WestwoodOnlineUpgradeProgressCDialogMessageMapAccessor::GetMessageMap();
}
AFX_MSGMAP_ENTRY const WestwoodOnlineUpgradeProgressDialog::messageEntries[] = {
    {0, 0, 0, 0, 0, 0},
};

const AFX_MSGMAP WestwoodOnlineUpgradeProgressDialog::messageMap = {
    &WestwoodOnlineUpgradeProgressDialog::GetBaseMessageMapForMfc,
    &WestwoodOnlineUpgradeProgressDialog::messageEntries[0],
};

/**
 * Original helper evidence: no standalone retail function; constructor shape is observed in the placement-constructor body.
 * Purpose: constructs the imported CDialog base with resource 157 so the compiler installs the derived vftable.
 */


#include "recoil/Mfc42Abi.h"
#include "Battlesport/wol_download.h"

#include "Battlesport/wol_api.h"
#include "Battlesport/wol_dialog.h"
#include "GameZRecoil/zCom/zCom.h"

#include <commctrl.h>

extern "C" IWestwoodOnlineUpgradeDownload *g_pWestwoodOnlineUpgradeDownload = 0;
extern "C" WestwoodOnlineUpgradeDownloadEventSink *g_pWestwoodOnlineUpgradeDownloadEventSink = 0;
extern "C" DWORD g_WestwoodOnlineUpgradeDownloadAdviseCookie = 0;
extern "C" WestwoodOnlineUpgradeDownloadReadyEntry *g_pWestwoodOnlineUpgradeDownloadReadyList = 0;
extern "C" char g_WestwoodOnlineUpgradeDownloadReadyPromptText[0x80] = {0};
extern "C" char g_WestwoodOnlineUpgradeDownloadRestoreCwd[0x100] = {0};
extern "C" int g_WestwoodOnlineUpgradeDownloadDialogResult = 0;
extern "C" HWND g_hWestwoodOnlineUpgradeProgressDialog = 0;

// BN observes these COM identity bytes in the WOL download ActiveX path.
const CLSID g_CLSID_WestwoodOnlineUpgradeDownload = {
    0xbf6ea206,
    0x9e55,
    0x11d1,
    {0x9d, 0xc6, 0x00, 0x60, 0x97, 0xc5, 0x43, 0x21},
};
const IID g_IID_WestwoodOnlineUpgradeDownload = {
    0x0bf5fceb,
    0x9f03,
    0x11d1,
    {0x9d, 0xc7, 0x00, 0x60, 0x97, 0xc5, 0x43, 0x21},
};
const IID IID_WestwoodOnlineUpgradeDownloadEventSink = {
    0x6869e99d,
    0x9fb4,
    0x11d1,
    {0x9d, 0xc8, 0x00, 0x60, 0x97, 0xc5, 0x43, 0x21},
};

// Recovered interface map used by 0x4427d0. BN data at 0x4d1fc8 contains
// {IID_WestwoodOnlineUpgradeDownloadEventSink, offset 0, direct} followed by end.
const zCom::InterfaceMapEntry g_WestwoodOnlineUpgradeDownloadEventSink_InterfaceMap[2] = {
    {&IID_WestwoodOnlineUpgradeDownloadEventSink, 0, zCom::ZCOM_INTERFACE_MAP_DIRECT},
    {0, 0, zCom::ZCOM_INTERFACE_MAP_END},
};

namespace {
const char kDownloadFinishedStatusText[] = "Finished!";
const char kDownloadErrorStatusText[] = "ERROR";
const char kDownloadProgressStatusText[] = "Bytes read: %d / %d";
const char kDownloadProgressWithTimeStatusText[] = "Bytes read: %d / %d.    Time left: %d seconds";
const char kDownloadStateConnectingText[] = "Connecting...";
const char kDownloadStateFindingPatchText[] = "Finding patch...";
const char kDownloadStateDownloadingPatchText[] = "Downloading patch...";
const int kDownloadProgressControlId = 1021;
const unsigned int kDownloadProgressPercentScale = 100;
const DWORD kDownloadErrorStatusSleepMs = 1000;

struct WestwoodOnlineUpgradeSharedComRefCountOwner : IUnknown {
    WestwoodOnlineUpgradeRefCountAndLock m_refCountAndLock;
};
} // namespace

RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeSharedComRefCountOwner,
        m_refCountAndLock
    ) == 0x04
);




/**
 * Reimplements 0x43cf90: WestwoodOnlineUpgradeDialog::UpdateSessionListQueryFromControls
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: encode the global dialog's current query controls and submit the
 * query string to the provider.
 */
void WestwoodOnlineUpgradeDialog::UpdateSessionListQueryFromControls() {
    CString encodedQuery;
    ((CWnd *)g_pWestwoodOnlineUpgradeDialog)->UpdateData(1);
    encodedQuery.Empty();

    LRESULT const selectedMode = ::SendMessageA(
        g_pWestwoodOnlineUpgradeDialog->m_sessionModeCombo.m_hWnd,
        CB_GETCURSEL,
        0,
        0
    );
    WestwoodOnlineUpgradeDialog *const dialog = g_pWestwoodOnlineUpgradeDialog;
    encodedQuery.Format(
        g_WestwoodOnlineUpgradeSessionQueryPayloadFmt,
        selectedMode,
        dialog->m_queryValueOrTime,
        dialog->m_queryAuxParam,
        dialog->m_queryMaxPlayers,
        dialog->m_queryStatusFlagBit0,
        dialog->m_queryStatusFlagBit1
    );
    IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    api->SubmitEncodedQueryString(encodedQuery
    );
}

/**
 * Reimplements 0x43d060: WestwoodOnlineUpgradeDialog::AppendStatusTextFmt
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: format and append a status line while keeping the status list
 * bounded and scrolled.
 */
int WestwoodOnlineUpgradeDialog::AppendStatusTextFmt(
    const char *format,
    ...
) {
    va_list args;
    va_start(
        args,
        format
    );
    vsprintf(
        g_WestwoodOnlineUpgradeStatusAppendBuffer,
        format,
        args
    );
    va_end(args);

    const unsigned int formattedLength =
        (unsigned int)strlen(g_WestwoodOnlineUpgradeStatusAppendBuffer);
    char *appendText = g_WestwoodOnlineUpgradeStatusAppendBuffer;
    if (g_WestwoodOnlineUpgradeStatusAppendBuffer[0] == '\n') {
        if (formattedLength == 1) {
            g_WestwoodOnlineUpgradeStatusAppendBuffer[0] = '\0';
        } else {
            appendText = &g_WestwoodOnlineUpgradeStatusAppendBuffer[1];
        }
    }

    if (g_WestwoodOnlineUpgradeStatusAppendBuffer[formattedLength - 1] == '\n') {
        g_WestwoodOnlineUpgradeStatusAppendBuffer[formattedLength - 1] = '\0';
    }

    ::SendMessageA(
        m_statusList.m_hWnd,
        LB_ADDSTRING,
        0,
        (LPARAM)appendText
    );
    ++m_statusLineCount;
    if ((int)m_statusLineCount > kStatusListMaxLineCount) {
        --m_statusLineCount;
        ::SendMessageA(
            m_statusList.m_hWnd,
            LB_DELETESTRING,
            0,
            0
        );
    }

    if ((int)m_statusLineCount > kStatusListScrollThreshold) {
        return (int) ::SendMessageA(
            m_statusList.m_hWnd,
            LB_SETTOPINDEX,
            m_statusLineCount - kStatusListScrollBackCount,
            0
        );
    }

    return (int)m_statusLineCount;
}

/**
 * Reimplements 0x43d130: WestwoodOnlineUpgradeApi::CreateInstanceAndLoadConfig
 * (D:\Proj\Battlesport\WestwoodOnlineUpgradeApi.cpp).
 *
 * Purpose: initialize COM/MFC control hosting, create the WOL ActiveX API,
 * advise the event sink, and apply the selected upgrade profile.
 */
int WestwoodOnlineUpgradeApi::CreateInstanceAndLoadConfig(
    HANDLE bootstrapServerListEvent
) {
    char failureCaption[kFailureMessageBufferSize];
    char failureText[kFailureMessageBufferSize];

    CoInitialize(0);
    g_WestwoodOnlineUpgradeApiInitState.structSize = kWestwoodOnlineUpgradeInitStateSize;
    g_WestwoodOnlineUpgradeApiShutdownState = 0;
    WestwoodOnlineUpgradeApiInitState::Init(
        &g_WestwoodOnlineUpgradeApiInitState,
        bootstrapServerListEvent,
        0
    );
    AfxEnableControlContainer(0);
    CoCreateInstance(
        g_WestwoodOnlineUpgradeApi_CLSID,
        0,
        CLSCTX_INPROC_SERVER,
        g_WestwoodOnlineUpgradeApi_IID,
        (void **)&g_pWestwoodOnlineUpgradeApi
    );

    if (g_pWestwoodOnlineUpgradeApi == 0) {
        CopyFailureMessage(
            failureCaption,
            zLoc::GetMessageString(kWolApiFailureCaptionMessageId)
        );
        CopyFailureMessage(
            failureText,
            zLoc::GetMessageString(kWolApiFailureTextMessageId)
        );
        MessageBeep(kWolApiFailureMessageBoxType);
        ((CWnd *)((unsigned int)g_RecoilApp.GetMainWnd()))
            ->MessageBoxA(
                failureText,
                failureCaption,
                kWolApiFailureMessageBoxType
            );
        return 0;
    }

    g_WestwoodOnlineUpgradeProcessCallbacksFlag = 1;
    WestwoodOnlineUpgradeApiEventSink::CreateInstance(
        (WestwoodOnlineUpgradeApiEventSink **)&g_pWestwoodOnlineUpgradeApiEventSink
    );
    zCom::ConnectionPointContainer_Advise(
        g_pWestwoodOnlineUpgradeApi,
        (IUnknown *)((unsigned char *)g_pWestwoodOnlineUpgradeApiEventSink +
                     g_WestwoodOnlineUpgradeApiEventSink_InterfaceMap[0].interfaceOffset),
        g_WestwoodOnlineUpgradeApiEventSink_IID,
        &g_WestwoodOnlineUpgradeApiAdviseCookie
    );

    if (WestwoodOnlineUpgradeConfigDialog::ShowModalAndApplySelectedProfileValues()) {
        return 1;
    }

    WestwoodOnlineUpgradeApi::Shutdown();
    return 0;
}

/**
 * Reimplements 0x43d280: WestwoodOnlineUpgradeApi::Shutdown
 * (D:\Proj\Battlesport\WestwoodOnlineUpgradeApi.cpp).
 *
 * Purpose: unadvise the WOL event sink, release the API COM object, and close
 * the startup wait handles.
 */
void WestwoodOnlineUpgradeApi::Shutdown() {
    if (g_pWestwoodOnlineUpgradeApi == 0) {
        return;
    }

    zCom::ConnectionPointContainer_Unadvise(
        g_pWestwoodOnlineUpgradeApi,
        g_WestwoodOnlineUpgradeApiEventSink_IID,
        g_WestwoodOnlineUpgradeApiAdviseCookie
    );
    g_pWestwoodOnlineUpgradeApi->Release();
    g_pWestwoodOnlineUpgradeApi = 0;
    CloseHandle(g_WestwoodOnlineUpgradeBootstrapServerListEvent);
    CloseHandle(g_WestwoodOnlineUpgradeStatusTextEvent);
    CloseHandle(g_WestwoodOnlineUpgradeFailureEvent);
}

/**
 * Reimplements 0x43d2e0: WestwoodOnlineUpgradeApi::Init
 * (D:\Proj\Battlesport\WestwoodOnlineUpgradeApi.cpp).
 *
 * Purpose: create the WOL startup events and progress UI, begin the selected
 * profile connection, request bootstrap servers, and enter list mode on success.
 */
int WestwoodOnlineUpgradeApi::Init() {
    char failureCaption[kWolApiFailureMessageBufferSize];
    char failureText[kWolApiFailureMessageBufferSize];

    memset(
        &g_WestwoodOnlineUpgradeCachedBrowseRecord,
        0,
        sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord)
    );
    zGame::ReturnOnlyStub();

    WestwoodOnlineUpgradeApi api;
    if (api.CreateInstanceAndLoadConfig(g_hWestwoodOnlineUpgradeModuleInstance) == 0) {
        return 0;
    }

    g_WestwoodOnlineUpgradeBootstrapServerListEvent = CreateEventA(
        0,
        FALSE,
        FALSE,
        0
    );
    g_WestwoodOnlineUpgradeStatusTextEvent = CreateEventA(
        0,
        FALSE,
        FALSE,
        0
    );
    g_WestwoodOnlineUpgradeFailureEvent = CreateEventA(
        0,
        FALSE,
        FALSE,
        0
    );
    g_WestwoodOnlineUpgradeInitWaitEvents[0] = g_WestwoodOnlineUpgradeBootstrapServerListEvent;
    g_WestwoodOnlineUpgradeInitWaitEvents[1] = g_WestwoodOnlineUpgradeStatusTextEvent;
    g_WestwoodOnlineUpgradeInitWaitEvents[2] = g_WestwoodOnlineUpgradeFailureEvent;

    ((CDialog *)g_pWestwoodOnlineUpgradeProgressDialog)
        ->Create(
            (LPCSTR)kWestwoodOnlineUpgradeApi_ProgressDialogResourceId,
            0
        );
    ((CWnd *)g_pWestwoodOnlineUpgradeProgressDialog)
        ->SetDlgItemTextA(
            kWestwoodOnlineUpgradeApi_ProgressStatusControlId,
            zLoc::GetMessageString(kWolApiInitConnectingMessageId)
        );

    CString connectString = g_pWestwoodOnlineUpgradeDialog->GetSelectedProfileConnectString();
    CString playerName = g_pWestwoodOnlineUpgradeDialog->GetSelectedProfilePlayerName();

    IWestwoodOnlineUpgradeProviderApi *apiCom = GetApiComObject();
    apiCom->BeginConnect(
        GetWolLanguageId(),
        kWolProductId,
        (const char *)playerName,
        (const char *)connectString,
        kWolConnectTimeoutSeconds
    );

    g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 0;
    DWORD waitResult = WaitForMultipleObjects(
        3,
        g_WestwoodOnlineUpgradeInitWaitEvents,
        FALSE,
        kInitialWaitTimeoutMs
    );
    PumpInitialCallbacksUntilEvent(&waitResult);
    if (g_WestwoodOnlineUpgradeApiAsyncErrorFlag != 0 || waitResult == WAIT_OBJECT_0 + 2) {
        ((CWnd *)g_pWestwoodOnlineUpgradeProgressDialog)
            ->SetDlgItemTextA(
                kWestwoodOnlineUpgradeApi_ProgressStatusControlId,
                zLoc::GetMessageString(kWolApiInitFailureTextMessageId)
            );
        Sleep(kFailureDisplaySleepMs);
        DestroyProgressDialog();
        return 0;
    }

    g_WestwoodOnlineUpgradeAbortFlag = 0;
    ResetEvent(g_WestwoodOnlineUpgradeFailureEvent);
    g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 0;

    apiCom = GetApiComObject();
    apiCom->RequestBootstrapServerList(
        &g_WestwoodOnlineUpgradeSelectedBootstrapServer,
        kWolBootstrapTimeoutSeconds,
        g_pWestwoodOnlineUpgradeDialog->m_selectedProfileConnectStringMode == 0 ? 1 : 0
    );

    waitResult = PumpBootstrapCallbacksUntilEvent();
    DestroyProgressDialog();
    ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
        ->SetDlgItemTextA(
            kWestwoodOnlineUpgradeDialogServerControlId,
            zLoc::GetMessageString(kWolApiInitReadyMessageId)
        );

    if (g_WestwoodOnlineUpgradeApiAsyncErrorFlag != 0) {
        return 0;
    }

    if (waitResult != WAIT_OBJECT_0 + 2) {
        apiCom = GetApiComObject();
        apiCom->RequestListMode(
            kWolRequestListMode,
            1
        );
        return 1;
    }

    CopyFailureMessage(
        failureCaption,
        zLoc::GetMessageString(kWolApiInitFailureCaptionMessageId)
    );
    CopyFailureMessage(
        failureText,
        zLoc::GetMessageString(kWolApiInitFailureTextMessageId)
    );
    ((CWnd *)((unsigned int)g_RecoilApp.m_pMainWnd))
        ->MessageBoxA(
            failureText,
            failureCaption,
            kWolApiFailureMessageBoxType
        );
    return 0;
}

/**
 * Reimplements 0x43d650: WestwoodOnlineUpgradeDialog::AppendConnectStatusAndRefreshList
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: append connection status text and refresh the browse list.
 */
void WestwoodOnlineUpgradeDialog::AppendConnectStatusAndRefreshList(
    const char *sessionName
) {
    char statusText[kConnectStatusBufferSize];
    zLoc::FormatMessage(
        statusText,
        kConnectStatusBufferSize,
        kConnectStatusMessageId,
        sessionName
    );
    AppendStatusTextFmt(statusText);
    ResetSelectedBrowseRecordAndRefreshList();
}

/**
 * Reimplements 0x43d6a0: WestwoodOnlineUpgradeDialog::SetAbortAndClose
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: mark the upgrade flow aborted and close through MFC cancel.
 */
void WestwoodOnlineUpgradeDialog::SetAbortAndClose() {
    g_WestwoodOnlineUpgradeAbortFlag = 1;
    ((CDialogCancelAccessor *)this)->CallBaseOnCancel();
}

/**
 * Reimplements 0x43d6b0: WestwoodOnlineUpgradeDialog::EnableQueryControls
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: enable or disable the dialog controls used to build session
 * queries.
 */
void WestwoodOnlineUpgradeDialog::EnableQueryControls(
    int enable
) {
    ((CWnd *)&m_sessionModeCombo)->EnableWindow(enable);
    ((CWnd *)&m_queryStatusFlag0Check)->EnableWindow(enable);
    ((CWnd *)&m_queryStatusFlag1Check)->EnableWindow(enable);
    ((CWnd *)&m_queryAuxParamEdit)->EnableWindow(enable);
    ((CWnd *)&m_queryValueOrTimeEdit)->EnableWindow(enable);
    ((CWnd *)&m_submitPendingSessionListButton)->EnableWindow(enable);
    ((CWnd *)&m_connectButton)->EnableWindow(enable);
}

/**
 * Reimplements 0x43d720: WestwoodOnlineUpgradeDialog::EnableConnectButton
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: enable or disable the provider connect button.
 */
void WestwoodOnlineUpgradeDialog::EnableConnectButton(
    int enable
) {
    ((CWnd *)&m_connectButton)->EnableWindow(enable);
}

/**
 * Reimplements 0x43d740: WestwoodOnlineUpgradeDialog::WestwoodOnlineUpgradeDialog
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: construct the CDialog-derived upgrade dialog and its embedded MFC
 * controls.
 */
WestwoodOnlineUpgradeDialog::WestwoodOnlineUpgradeDialog(
    CWnd *parentWnd
) :
    CDialog(
        kWestwoodOnlineUpgradeDialogResourceId,
        parentWnd
    ),
    m_serverAddressEdit(),
    m_statusTokenEdit(),
    m_queryValueOrTimeEdit(),
    m_queryMaxPlayersEdit(),
    m_queryAuxParamEdit(),
    m_queryStatusFlag1Check(),
    m_queryStatusFlag0Check(),
    m_submitPendingSessionListButton(),
    m_connectButton(),
    m_querySessionsByNameButton(),
    m_queueVisibleSessionRequestsButton(),
    m_statusList(),
    m_sessionModeCombo(),
    m_sessionResultsList(),
    m_statusServerEdit(),
    m_sessionNameEdit(),
    m_browseRecordList(),
    m_selectedProfilePlayerName(),
    m_selectedProfileConnectString(),
    m_sessionName()
{
    m_queryAuxParam = 0;
    m_queryMaxPlayers = 0;
    m_queryValueOrTime = 0;
    m_queryStatusFlagBit0 = 0;
    m_queryStatusFlagBit1 = 0;
    m_selectedProfileConnectStringMode = 0;
}

/**
 * Reimplements 0x43d9a0: WestwoodOnlineUpgradeDialog::Destructor
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Evidence: retail matches VC5's implicit derived destructor; declaring a
 * source-level ~WestwoodOnlineUpgradeDialog body adds a non-retail derived
 * vftable reset before member cleanup.
 * Purpose: invoke the implicit dialog destructor for recovered teardown callers.
 */
void WestwoodOnlineUpgradeDialog::Destructor() {
    this->WestwoodOnlineUpgradeDialog::~WestwoodOnlineUpgradeDialog();
}

/**
 * Reimplements 0x43db20: WestwoodOnlineUpgradeDialog::DoDataExchange
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: bind MFC controls and synchronize query option fields.
 */
void WestwoodOnlineUpgradeDialog::DoDataExchange(
    CDataExchange *dataExchange
) {
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeServerAddressEditId,
        *((CWnd *)&m_serverAddressEdit)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeStatusTokenEditId,
        *((CWnd *)&m_statusTokenEdit)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeQueryValueOrTimeEditId,
        *((CWnd *)&m_queryValueOrTimeEdit)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeQueryMaxPlayersEditId,
        *((CWnd *)&m_queryMaxPlayersEdit)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeQueryAuxParamEditId,
        *((CWnd *)&m_queryAuxParamEdit)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeQueryStatusFlag1CheckId,
        *((CWnd *)&m_queryStatusFlag1Check)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeQueryStatusFlag0CheckId,
        *((CWnd *)&m_queryStatusFlag0Check)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeSubmitPendingSessionListButtonId,
        *((CWnd *)&m_submitPendingSessionListButton)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeConnectButtonId,
        *((CWnd *)&m_connectButton)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeQuerySessionsByNameButtonId,
        *((CWnd *)&m_querySessionsByNameButton)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeQueueVisibleSessionRequestsButtonId,
        *((CWnd *)&m_queueVisibleSessionRequestsButton)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeStatusListId,
        *((CWnd *)&m_statusList)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeSessionModeComboId,
        *((CWnd *)&m_sessionModeCombo)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeDialog_SessionResultsListId,
        *((CWnd *)&m_sessionResultsList)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeStatusServerEditId,
        *((CWnd *)&m_statusServerEdit)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeSessionNameEditId,
        *((CWnd *)&m_sessionNameEdit)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeDialog_BrowseRecordListId,
        *((CWnd *)&m_browseRecordList)
    );
    DDX_Text(
        dataExchange,
        kWestwoodOnlineUpgradeQueryAuxParamEditId,
        m_queryAuxParam
    );
    DDX_Text(
        dataExchange,
        kWestwoodOnlineUpgradeQueryMaxPlayersEditId,
        m_queryMaxPlayers
    );
    DDX_Text(
        dataExchange,
        kWestwoodOnlineUpgradeQueryValueOrTimeEditId,
        m_queryValueOrTime
    );
    DDX_Check(
        dataExchange,
        kWestwoodOnlineUpgradeQueryStatusFlag0CheckId,
        (int &)m_queryStatusFlagBit0
    );
    DDX_Check(
        dataExchange,
        kWestwoodOnlineUpgradeQueryStatusFlag1CheckId,
        (int &)m_queryStatusFlagBit1
    );
}

/**
 * Reimplements 0x43dcc0: WestwoodOnlineUpgradeDialog::GetMessageMap
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: return the recovered MFC message map for the upgrade dialog.
 */
const AFX_MSGMAP * WestwoodOnlineUpgradeDialog::GetMessageMap() const {
    return &WestwoodOnlineUpgradeDialog::messageMap;
}

/**
 * Reimplements 0x43dcd0: WestwoodOnlineUpgradeDialog::OnInitDialog
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: initialize dialog controls, defaults, provider state, and refresh
 * timer.
 */
BOOL WestwoodOnlineUpgradeDialog::OnInitDialog() {
    ((CDialog *)this)->CDialog::OnInitDialog();

    CString sessionModeNames[kWolSessionModeCount] = {
        zLoc::GetMessageString(kWolSessionModeFirstMessageId),
        zLoc::GetMessageString(kWolSessionModeFirstMessageId + 1),
        zLoc::GetMessageString(kWolSessionModeFirstMessageId + 2),
        zLoc::GetMessageString(kWolSessionModeFirstMessageId + 3),
        zLoc::GetMessageString(kWolSessionModeFirstMessageId + 4),
        zLoc::GetMessageString(kWolSessionModeFirstMessageId + 5),
        zLoc::GetMessageString(kWolSessionModeFirstMessageId + 6)};

    m_sessionName.Format(zLoc::GetMessageString(kWolDialogDefaultSessionNameMessageId));

    for (int index = 0; index < kWolSessionModeCount; ++index) {
        LRESULT const itemIndex = ::SendMessageA(
            m_sessionModeCombo.m_hWnd,
            CB_ADDSTRING,
            0,
            (LPARAM)(const char *)sessionModeNames[index]
        );
        ::SendMessageA(
            m_sessionModeCombo.m_hWnd,
            CB_SETITEMDATA,
            itemIndex,
            index
        );
    }
    ::SendMessageA(
        m_sessionModeCombo.m_hWnd,
        CB_SETCURSEL,
        0,
        0
    );

    ((CWnd *)this)
        ->SetDlgItemTextA(
            kWestwoodOnlineUpgradeStatusCaptionLabelId,
            zLoc::GetMessageString(kWolStatusCaptionMessageId)
        );
    m_queryValueOrTime = 15;
    m_queryAuxParam = 10;
    m_queryMaxPlayers = 4;
    m_queryStatusFlagBit0 = 1;
    m_queryStatusFlagBit1 = 0;
    ((CWnd *)this)->UpdateData(0);

    m_statusLineCount = 0;
    EnableQueryControls(0);

    if (WestwoodOnlineUpgradeApi::Init() == 0) {
        OnDestroy();
        SetAbortAndClose();
        return 0;
    }

    ::SetTimer(
        m_hWnd,
        kWolRefreshListTimerId,
        kWolRefreshListTimerMs,
        0
    );
    ::SendMessageA(
        m_sessionNameEdit.m_hWnd,
        EM_LIMITTEXT,
        kWolSessionNameEditLimit,
        0
    );
    ::SendMessageA(
        m_serverAddressEdit.m_hWnd,
        EM_LIMITTEXT,
        kWolServerAddressEditLimit,
        0
    );
    ::SendMessageA(
        m_statusServerEdit.m_hWnd,
        EM_LIMITTEXT,
        kWolStatusServerEditLimit,
        0
    );
    ::SendMessageA(
        m_statusTokenEdit.m_hWnd,
        EM_LIMITTEXT,
        kWolStatusTokenEditLimit,
        0
    );
    ::SendMessageA(
        m_statusList.m_hWnd,
        LB_SETHORIZONTALEXTENT,
        kWolStatusListHorizontalExtent,
        0
    );
    g_WestwoodOnlineUpgradeDisconnectInFlightFlag = 0;
    return 1;
}

/**
 * Reimplements 0x43dfe0: WestwoodOnlineUpgradeDialog::OnRefreshListTimer
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: tick time, pump provider callbacks, and request periodic list
 * refreshes.
 */
void WestwoodOnlineUpgradeDialog::OnRefreshListTimer(
    UINT_PTR
) {
    Time::Tick();
    if (g_WestwoodOnlineUpgradeProcessCallbacksFlag != 0) {
        IWestwoodOnlineUpgradeProviderApi *api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
        api->ProcessCallbacks();
        if (g_Time_UnscaledAccumulatedTimeSec > g_WestwoodOnlineUpgradeNextAutoRefreshTime) {
            api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
            g_WestwoodOnlineUpgradeNextAutoRefreshTime =
                g_Time_UnscaledAccumulatedTimeSec - (-60.0);
            api->RequestListMode(kWestwoodOnlineUpgradeAutoRefreshListMode,
                    1
                );
        }
    }

    ((WestwoodOnlineUpgradeCWndAccess *)this)->CallDefault();
}

/**
 * Reimplements 0x43e040: WestwoodOnlineUpgradeDialog::OnBrowseRecordListDblClk
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: load a selected browse record and update the dialog selection.
 */
void WestwoodOnlineUpgradeDialog::OnBrowseRecordListDblClk() {
    LRESULT const selectedIndex = ::SendMessageA(
        m_browseRecordList.m_hWnd,
        LB_GETCURSEL,
        0,
        0
    );
    if (selectedIndex != LB_ERR &&
        g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] != '\0') {
        if (strcmp(
                g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName,
                g_WestwoodOnlineUpgradeCachedBrowseRecordList[selectedIndex].m_sessionName
            ) == 0) {
            return;
        }
        ResetSelectedBrowseRecordAndRefreshList();
    }

    ((CWnd *)&m_serverAddressEdit)
        ->GetWindowTextA(
            g_WestwoodOnlineUpgradeCachedBrowseRecordList[selectedIndex].m_serverAddress,
            kWolQuerySessionsByNameServerTextMaxChars
        );
    ((CWnd *)&m_serverAddressEdit)->SetWindowTextA(
        g_WestwoodOnlineUpgradeCachedBrowseRecordList[selectedIndex].m_serverAddress
    );

    IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    WestwoodOnlineUpgradeBrowseRecord *const selectedRecord =
        &g_WestwoodOnlineUpgradeCachedBrowseRecordList[selectedIndex];
    int const result = api->LoadBrowseRecord(selectedRecord
    );
    if (result == kWolQueryDuplicateNameResult) {
        g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(
            zLoc::GetMessageString(kWolQuerySessionDuplicateMessageId)
        );
        return;
    }

    g_WestwoodOnlineUpgradeCachedBrowseRecord = *selectedRecord;
    if (g_WestwoodOnlineUpgradeCachedBrowseRecord.m_recordFlags != 0) {
        m_sessionName = g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName;
        ((CWnd *)this)->UpdateData(0);
    }
    ClearStatusList();
}

/**
 * Reimplements 0x43e160: WestwoodOnlineUpgradeDialog::OnDestroy
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: disconnect, stop refresh timing, and shut down the provider API.
 */
void WestwoodOnlineUpgradeDialog::OnDestroy() {
    if (g_WestwoodOnlineUpgradeAbortFlag == 0) {
        BeginDisconnectAndShowProgress();
        while (g_WestwoodOnlineUpgradeAbortFlag == 0) {
            IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
            api->ProcessCallbacks();
            Sleep(kWolDestroyCallbackSleepMs);
        }
    }

    ::KillTimer(
        m_hWnd,
        kWolRefreshListTimerId
    );
    WestwoodOnlineUpgradeApi::Shutdown();

    ((CWnd *)g_pWestwoodOnlineUpgradeProgressDialog)->DestroyWindow();
}

/**
 * Reimplements 0x43e1c0:
 *     WestwoodOnlineUpgradeDialog::SubmitVisibleSessionRequestsAndStatusText
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: submit visible session requests with pending status text.
 */
void WestwoodOnlineUpgradeDialog::SubmitVisibleSessionRequestsAndStatusText() {
    if (g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] == '\0') {
        return;
    }

    static CString pendingStatusText("");
    ((CWnd *)&m_statusServerEdit)->GetWindowTextA(pendingStatusText);
    if (pendingStatusText.IsEmpty()) {
        return;
    }

    ((CWnd *)&m_statusServerEdit)->SetWindowTextA("");

    g_WestwoodOnlineUpgradeVisibleSessionResultCount =
        (int) ::SendMessageA(
            m_sessionResultsList.m_hWnd,
            LB_GETSELCOUNT,
            0,
            0
        );
    if (g_WestwoodOnlineUpgradeVisibleSessionResultCount == 0) {
        ((IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi)->SubmitStatusText(
            pendingStatusText
        );
        AppendStatusTextFmt(
            g_WestwoodOnlineUpgradeSessionQueryDisplayFmt,
            (const char *)m_selectedProfilePlayerName,
            (const char *)pendingStatusText
        );
        return;
    }

    LPARAM selectedRowIndices[kWolVisibleSessionSelectionLimit];
    WestwoodOnlineUpgradeSessionRequest *sessionRequestList = 0;
    WestwoodOnlineUpgradeSessionRequest *sessionRequest;
    ::SendMessageA(
        m_sessionResultsList.m_hWnd,
        LB_GETSELITEMS,
        kWolVisibleSessionSelectionLimit,
        (LPARAM)selectedRowIndices
    );

    for (int rowIndex = 0; rowIndex < g_WestwoodOnlineUpgradeVisibleSessionResultCount;
        ++rowIndex) {
        sessionRequest = new WestwoodOnlineUpgradeSessionRequest;
        memset(
            sessionRequest,
            0,
            sizeof(WestwoodOnlineUpgradeSessionRequest)
        );
        ::SendMessageA(
            m_sessionResultsList.m_hWnd,
            LB_GETTEXT,
            selectedRowIndices[rowIndex],
            (LPARAM)sessionRequest->m_sessionName
        );
        WestwoodOnlineUpgrade::TruncateStringAtFirstSpace(sessionRequest->m_sessionName);
        sessionRequest->m_next = sessionRequestList;
        sessionRequestList = sessionRequest;
    }

    char statusFormatBuffer[kConnectStatusBufferSize];
    zLoc::FormatMessage(
        statusFormatBuffer,
        kConnectStatusBufferSize,
        kWolSubmitSessionRequestStatusMessageId,
        sessionRequest->m_sessionName,
        (const char *)pendingStatusText
    );
    AppendStatusTextFmt(statusFormatBuffer);

    ((IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi)->SubmitSessionRequestListAndStatusText(
        sessionRequest,
        pendingStatusText
    );

    while (sessionRequestList != 0) {
        WestwoodOnlineUpgradeSessionRequest *const nextSessionRequest = sessionRequestList->m_next;
        delete sessionRequestList;
        sessionRequestList = nextSessionRequest;
    }
}

/**
 * Reimplements 0x43e3b0: WestwoodOnlineUpgradeDialog::ResetSelectedBrowseRecordAndRefreshList
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: clear the selected browse record and request a refreshed list.
 */
void WestwoodOnlineUpgradeDialog::ResetSelectedBrowseRecordAndRefreshList() {
    if (g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] != '\0') {
        IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
        api->ResetQueryState();
        memset(
            &g_WestwoodOnlineUpgradeCachedBrowseRecord,
            0,
            sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord)
        );
    }

    ::SendMessageA(
        m_sessionResultsList.m_hWnd,
        LB_RESETCONTENT,
        0,
        0
    );
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 0;
    g_pWestwoodOnlineUpgradeDialog->EnableQueryControls(0);
    g_pWestwoodOnlineUpgradeDialog->EnableConnectButton(0);
    ((CWnd *)&m_querySessionsByNameButton)->EnableWindow(0);
    ((CWnd *)&m_queueVisibleSessionRequestsButton)->EnableWindow(0);

    IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    api->RequestListMode(g_WestwoodOnlineUpgradeActiveListMode,
        1
    );
}

/**
 * Reimplements 0x43e450: WestwoodOnlineUpgradeDialog::BeginDisconnectAndShowProgress
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: show the progress dialog and start provider disconnect processing.
 */
void WestwoodOnlineUpgradeDialog::BeginDisconnectAndShowProgress() {
    if (g_pWestwoodOnlineUpgradeApi == 0 || g_WestwoodOnlineUpgradeDisconnectInFlightFlag != 0) {
        return;
    }

    ((CDialog *)g_pWestwoodOnlineUpgradeProgressDialog)
        ->Create(
            (LPCSTR)kWestwoodOnlineUpgradeDialog_ProgressDialogResourceId,
            0
        );
    ((CWnd *)g_pWestwoodOnlineUpgradeProgressDialog)
        ->SetDlgItemTextA(
            kWestwoodOnlineUpgradeDialog_ProgressStatusControlId,
            zLoc::GetMessageString(kWolDisconnectStatusMessageId)
        );

    IWestwoodOnlineUpgradeProviderApi *api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    api->Disconnect();
    g_WestwoodOnlineUpgradeDisconnectInFlightFlag = 1;
}

/**
 * Reimplements 0x43e4b0: WestwoodOnlineUpgradeDialog::BeginConnect
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: gather connection text, update status, and start provider connect.
 */
void WestwoodOnlineUpgradeDialog::BeginConnect() {
    WestwoodOnlineUpgradeConnectContext context;
    ((CWnd *)&m_statusServerEdit)
        ->GetWindowTextA(
            context.m_requestText,
            kWolConnectRequestTextMaxChars
        );

    IWestwoodOnlineUpgradeProviderApi *api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    int mode;
    if (api->PrepareConnectContextAndMode(&context
    ) == 0) {
        mode = 0;
        AppendStatusTextFmt(zLoc::GetMessageString(kWolBeginConnectMode0MessageId));
    } else {
        mode = 1;
        AppendStatusTextFmt(zLoc::GetMessageString(kWolBeginConnectMode1MessageId));
    }

    api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    api->BeginConnectWithPreparedContext(&context,
        mode
    );
}

/**
 * Reimplements 0x43e520: WestwoodOnlineUpgradeDialog::CheckAndApplyUpgrade
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: ask the provider whether the requested upgrade download is ready.
 */
int WestwoodOnlineUpgradeDialog::CheckAndApplyUpgrade() {
    WestwoodOnlineUpgradeConnectContext context;
    ((CWnd *)&m_statusServerEdit)
        ->GetWindowTextA(
            context.m_requestText,
            kWolConnectRequestTextMaxChars
        );

    IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    return api->RequestUpgradeDownloadReadyResult(&context
    );
}

/**
 * Reimplements 0x43e550: WestwoodOnlineUpgradeDialog::QueryStatus
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: submit a status-token query or display the localized missing-token
 * error.
 */
int WestwoodOnlineUpgradeDialog::QueryStatus() {
    char tokenInputText[kWolQueryStatusTokenTextBufferSize];
    ((CWnd *)&m_statusTokenEdit)->GetWindowTextA(
        tokenInputText,
        kWolQueryStatusTokenTextMaxChars
    );
    ((CWnd *)&m_statusTokenEdit)->SetWindowTextA("");

    char *token = strtok(
        tokenInputText,
        kWolQueryStatusTokenDelimiter
    );
    if (token != 0) {
        WestwoodOnlineUpgradeConnectContext tokenContext;
        strcpy(
            tokenContext.m_requestText,
            token
        );

        char serverText[kWolQueryStatusServerTextBufferSize];
        ((CWnd *)&m_statusServerEdit)
            ->GetWindowTextA(
                serverText,
                kWolQueryStatusServerTextMaxChars
            );
        ((CWnd *)&m_statusServerEdit)->SetWindowTextA("");

        IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
        return api->QueryStatusWithTokenAndServer(&tokenContext,
                serverText
            );
    }

    char messageBoxTitle[kWolQueryStatusMessageBoxTitleBufferSize];
    strcpy(
        messageBoxTitle,
        zLoc::GetMessageString(kWolQueryStatusErrorTitleMessageId)
    );

    char messageBoxText[kWolQueryStatusMessageBoxTextBufferSize];
    strcpy(
        messageBoxText,
        zLoc::GetMessageString(kWolQueryStatusMissingTokenMessageId)
    );

    return ((CWnd *)this)->MessageBoxA(
        messageBoxText,
        messageBoxTitle,
        MB_ICONHAND
    );
}

/**
 * Reimplements 0x43e680: WestwoodOnlineUpgradeDialog::RequestActiveListMode
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: refresh the provider list using the dialog's active list mode.
 */
void WestwoodOnlineUpgradeDialog::RequestActiveListMode() {
    IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    api->RequestListMode(g_WestwoodOnlineUpgradeActiveListMode,
        1
    );
}

/**
 * Reimplements 0x43e6a0: WestwoodOnlineUpgradeDialog::OnRefreshCurrentQuery
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: validate the current session query and submit it to the provider.
 */
void WestwoodOnlineUpgradeDialog::OnRefreshCurrentQuery() {
    if (g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag != 0) {
        ResetSelectedBrowseRecordAndRefreshList();
    }

    CString sessionNameText;
    ((CWnd *)&m_sessionNameEdit)->GetWindowTextA(sessionNameText);
    sessionNameText.TrimLeft();
    sessionNameText.TrimRight();

    if (((const char *)sessionNameText)[0] == '\0') {
        char messageBoxTitle[kWolQueryStatusMessageBoxTitleBufferSize];
        char messageBoxText[kWolQueryStatusMessageBoxTextBufferSize];
        strcpy(
            messageBoxText,
            zLoc::GetMessageString(kWolQueryStatusErrorTitleMessageId)
        );
        strcpy(
            messageBoxTitle,
            zLoc::GetMessageString(kWolQuerySessionNameRequiredMessageId)
        );
        ((CWnd *)this)->MessageBoxA(
            messageBoxTitle,
            messageBoxText,
            0
        );
        return;
    }

    if (strstr(
        sessionNameText,
        kWolQueryStatusTokenDelimiter
    ) != 0) {
        char messageBoxTitle[kWolQueryStatusMessageBoxTitleBufferSize];
        char messageBoxText[kWolQueryStatusMessageBoxTextBufferSize];
        strcpy(
            messageBoxText,
            zLoc::GetMessageString(kWolQueryStatusErrorTitleMessageId)
        );
        strcpy(
            messageBoxTitle,
            zLoc::GetMessageString(kWolQuerySessionSubmitFailedMessageId)
        );
        ((CWnd *)this)->MessageBoxA(
            messageBoxTitle,
            messageBoxText,
            0
        );
        return;
    }

    m_sessionNameEdit.SetWindowTextA("");

    WestwoodOnlineUpgradeQueryRequest request;
    memset(
        &request,
        0,
        sizeof(request)
    );
    strncpy(
        request.m_sessionName,
        sessionNameText,
        kWolQuerySessionNameCopyMaxChars
    );
    request.m_listMode = 0;
    request.m_queryVariant = 2;
    request.m_queryMaxPlayers = m_queryMaxPlayers;
    request.m_queryFlags = 0;

    if (g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] != '\0') {
        IWestwoodOnlineUpgradeProviderApi *api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
        api->ResetQueryState();
    }

    IWestwoodOnlineUpgradeProviderApi *api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    int result = api->SubmitQueryRequest(&request
    );
    if (result < 0) {
        if (result != kWolQueryDuplicateNameResult) {
            g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(
                zLoc::GetMessageString(kWolQuerySessionSubmitFailedMessageId)
            );
        } else {
            g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(
                zLoc::GetMessageString(kWolQuerySessionDuplicateMessageId)
            );
        }
    }

    g_WestwoodOnlineUpgradeActiveListMode = 0;
}

/**
 * Reimplements 0x43e900: WestwoodOnlineUpgradeDialog::OnQuerySessionsByName
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: validate a named session query, submit it, and update dialog state.
 */
void WestwoodOnlineUpgradeDialog::OnQuerySessionsByName() {
    CString sessionNameText;
    m_sessionNameEdit.GetWindowTextA(sessionNameText);
    sessionNameText.TrimLeft();
    sessionNameText.TrimRight();
    CWnd *const serverAddressEdit = &m_serverAddressEdit;
    char messageBoxText[kWolQueryStatusMessageBoxTextBufferSize];
    char messageBoxTitle[kWolQueryStatusMessageBoxTitleBufferSize];
    if (sessionNameText.GetLength() == 0) {
        strcpy(
            messageBoxTitle,
            zLoc::GetMessageString(kWolQueryStatusErrorTitleMessageId)
        );
        strcpy(
            messageBoxText,
            zLoc::GetMessageString(kWolQuerySessionNameRequiredMessageId)
        );
        ((CWnd *)this)->MessageBoxA(
            messageBoxText,
            messageBoxTitle,
            0
        );
        return;
    }

    if (strstr(
        sessionNameText,
        kWolQueryStatusTokenDelimiter
    ) != 0) {
        strcpy(
            messageBoxTitle,
            zLoc::GetMessageString(kWolQueryStatusErrorTitleMessageId)
        );
        strcpy(
            messageBoxText,
            zLoc::GetMessageString(kWolQuerySessionSubmitFailedMessageId)
        );
        ((CWnd *)this)->MessageBoxA(
            messageBoxText,
            messageBoxTitle,
            0
        );
        return;
    }

    WestwoodOnlineUpgradeQueryRequest request;
    memset(
        &request,
        0,
        sizeof(request)
    );
    strncpy(
        request.m_sessionName,
        sessionNameText,
        kWolQuerySessionNameCopyMaxChars
    );
    request.m_listMode = kWolQuerySessionsByNameListMode;
    request.m_queryVariant = 2;
    request.m_queryMaxPlayers = m_queryMaxPlayers;
    request.m_queryExtraParam = 0;
    serverAddressEdit->GetWindowTextA(
        request.m_serverAddress,
        kWolQuerySessionsByNameServerTextMaxChars
    );
    serverAddressEdit->SetWindowTextA(request.m_serverAddress);

    if (g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] != '\0') {
        IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
        api->ResetQueryState();
    }

    IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    int result = api->SubmitQueryRequest(&request
    );
    if (result < 0) {
        strcpy(
            messageBoxTitle,
            zLoc::GetMessageString(kWolQueryStatusErrorTitleMessageId)
        );
        unsigned int messageId = kWolQueryStatusErrorTitleMessageId;
        if (result != kWolQueryDuplicateNameResult) {
            if (result == kWolQuerySubmitFailedResult) {
                messageId = kWolQuerySessionSubmitFailedMessageId;
            }
        } else {
            messageId = kWolQuerySessionDuplicateMessageId;
        }
        strcpy(
            messageBoxText,
            zLoc::GetMessageString(messageId)
        );
        ((CWnd *)this)->MessageBoxA(
            messageBoxText,
            messageBoxTitle,
            0
        );
        return;
    }

    m_sessionNameEdit.SetWindowTextA("");
    m_sessionName = sessionNameText;
    ((CWnd *)this)->UpdateData(0);
    ClearStatusList();
    g_WestwoodOnlineUpgradeActiveListMode = kWolQuerySessionsByNameListMode;
    m_querySessionsByNameButton.EnableWindow(1);
    m_queueVisibleSessionRequestsButton.EnableWindow(1);
}

/**
 * Reimplements 0x43ebd0: WestwoodOnlineUpgradeDialog::ClearStatusList
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: clear the visible status list and reset the dialog line counter.
 */
void WestwoodOnlineUpgradeDialog::ClearStatusList() {
    ::SendMessageA(
        m_statusList.m_hWnd,
        LB_RESETCONTENT,
        0,
        0
    );
    m_statusLineCount = 0;
}

/**
 * Reimplements 0x43ec00: WestwoodOnlineUpgradeDialog::QueueVisibleSessionRequests
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: queue selected visible session names with the provider.
 */
void WestwoodOnlineUpgradeDialog::QueueVisibleSessionRequests() {
    if (g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] == '\0') {
        return;
    }

    g_WestwoodOnlineUpgradeVisibleSessionResultCount =
        (int) ::SendMessageA(
            m_sessionResultsList.m_hWnd,
            LB_GETSELCOUNT,
            0,
            0
        );
    if (g_WestwoodOnlineUpgradeVisibleSessionResultCount == 0 ||
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag != 1) {
        return;
    }

    LPARAM selectedRowIndices[kWolVisibleSessionSelectionLimit];
    ::SendMessageA(
        m_sessionResultsList.m_hWnd,
        LB_GETSELITEMS,
        kWolVisibleSessionSelectionLimit,
        (LPARAM)selectedRowIndices
    );

    WestwoodOnlineUpgradeSessionRequest *sessionRequest = new WestwoodOnlineUpgradeSessionRequest;
    for (int rowIndex = 0; rowIndex < g_WestwoodOnlineUpgradeVisibleSessionResultCount;
        ++rowIndex) {
        memset(
            sessionRequest,
            0,
            sizeof(WestwoodOnlineUpgradeSessionRequest)
        );
        ::SendMessageA(
            m_sessionResultsList.m_hWnd,
            LB_GETTEXT,
            selectedRowIndices[rowIndex],
            (LPARAM)sessionRequest->m_sessionName
        );
        WestwoodOnlineUpgrade::TruncateStringAtFirstSpace(sessionRequest->m_sessionName);
        if (strstr(
            sessionRequest->m_sessionName,
            (const char *)m_selectedProfilePlayerName
        ) == 0) {
            IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
            api->QueueSessionRequest(sessionRequest
            );
        }
    }
    delete sessionRequest;
}

/**
 * Reimplements 0x43ed10:
 *     WestwoodOnlineUpgradeDialog::QueueVisibleSessionRequestsAndLookupBrowseRecords
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: look up selected browse records and queue matching session requests.
 */
void WestwoodOnlineUpgradeDialog::QueueVisibleSessionRequestsAndLookupBrowseRecords() {
    if (g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] == '\0') {
        return;
    }

    g_WestwoodOnlineUpgradeVisibleSessionResultCount =
        (int) ::SendMessageA(
            m_sessionResultsList.m_hWnd,
            LB_GETSELCOUNT,
            0,
            0
        );
    if (g_WestwoodOnlineUpgradeVisibleSessionResultCount == 0 ||
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag != 1) {
        return;
    }

    LPARAM selectedRowIndices[kWolVisibleSessionSelectionLimit];
    ::SendMessageA(
        m_sessionResultsList.m_hWnd,
        LB_GETSELITEMS,
        kWolVisibleSessionSelectionLimit,
        (LPARAM)selectedRowIndices
    );

    WestwoodOnlineUpgradeSessionRequest *sessionRequest = new WestwoodOnlineUpgradeSessionRequest;
    for (int rowIndex = 0; rowIndex < g_WestwoodOnlineUpgradeVisibleSessionResultCount;
        ++rowIndex) {
        memset(
            sessionRequest,
            0,
            sizeof(WestwoodOnlineUpgradeSessionRequest)
        );
        ::SendMessageA(
            m_sessionResultsList.m_hWnd,
            LB_GETTEXT,
            selectedRowIndices[rowIndex],
            (LPARAM)sessionRequest->m_sessionName
        );
        WestwoodOnlineUpgrade::TruncateStringAtFirstSpace(sessionRequest->m_sessionName);
        if (strstr(
            sessionRequest->m_sessionName,
            (const char *)m_selectedProfilePlayerName
        ) == 0) {
            IWestwoodOnlineUpgradeProviderApi *api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
            api->LookupBrowseRecordBySessionName(sessionRequest->m_sessionName,
                1
            );
            api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
            api->QueueSessionRequest(sessionRequest
            );
        }
    }
    delete sessionRequest;
}

/**
 * Reimplements 0x43ee40: WestwoodOnlineUpgradeDialog::RequestListMode0
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: switch to list mode 0 and request provider refresh.
 */
void WestwoodOnlineUpgradeDialog::RequestListMode0() {
    IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    g_WestwoodOnlineUpgradeActiveListMode = 0;
    api->RequestListMode(0,
        0
    );
}

/**
 * Reimplements 0x43ee60: WestwoodOnlineUpgradeDialog::RequestListMode11
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: switch to the auto-refresh list mode and request provider refresh.
 */
void WestwoodOnlineUpgradeDialog::RequestListMode11() {
    IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    g_WestwoodOnlineUpgradeActiveListMode = kWestwoodOnlineUpgradeAutoRefreshListMode;
    api->RequestListMode(kWestwoodOnlineUpgradeAutoRefreshListMode,
        1
    );
}

/**
 * Reimplements 0x43ee80: WestwoodOnlineUpgradeDialog::OnSessionModeComboSelChange
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: update query label/edit state after the session-mode selection
 * changes.
 */
void WestwoodOnlineUpgradeDialog::OnSessionModeComboSelChange() {
    LRESULT const selectedIndex = ::SendMessageA(
        m_sessionModeCombo.m_hWnd,
        CB_GETCURSEL,
        0,
        0
    );
    LRESULT const selectedModeKind =
        ::SendMessageA(
            m_sessionModeCombo.m_hWnd,
            CB_GETITEMDATA,
            selectedIndex,
            0
        );
    m_querySessionModeKind = selectedModeKind;
    if (selectedModeKind == 2) {
        ((CWnd *)this)
            ->SetDlgItemTextA(
                kWestwoodOnlineUpgradeQueryValueOrTimeLabelId,
                zLoc::GetMessageString(kWolQuerySessionModeTimeLabelMessageId)
            );
        ((CWnd *)&m_queryValueOrTimeEdit)->EnableWindow(0);
        UpdateSessionListQueryFromControls();
        return;
    }

    ((CWnd *)this)
        ->SetDlgItemTextA(
            kWestwoodOnlineUpgradeQueryValueOrTimeLabelId,
            zLoc::GetMessageString(kWolQuerySessionModeValueLabelMessageId)
        );
    ((CWnd *)&m_queryValueOrTimeEdit)->EnableWindow(1);
    UpdateSessionListQueryFromControls();
}

/**
 * Reimplements 0x43ef10: WestwoodOnlineUpgradeDialog::SubmitPendingSessionListFromResults
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: build and submit the pending session request list from result rows.
 */
void WestwoodOnlineUpgradeDialog::SubmitPendingSessionListFromResults() {
    if (g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] == '\0' ||
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag != 1) {
        return;
    }

    WestwoodOnlineUpgradeSessionRequest *sessionRequestList = 0;
    for (int rowIndex = 0; rowIndex < g_WestwoodOnlineUpgradePendingSessionResultCount;
        ++rowIndex) {
        WestwoodOnlineUpgradeSessionRequest *sessionRequest =
            new WestwoodOnlineUpgradeSessionRequest;
        memset(
            sessionRequest,
            0,
            sizeof(WestwoodOnlineUpgradeSessionRequest)
        );
        if (::SendMessageA(
                m_sessionResultsList.m_hWnd,
                LB_GETTEXT,
                rowIndex,
                (LPARAM)sessionRequest->m_sessionName
            ) == LB_ERR) {
            delete sessionRequest;
        } else {
            WestwoodOnlineUpgrade::TruncateStringAtFirstSpace(sessionRequest->m_sessionName);
            sessionRequest->m_next = sessionRequestList;
            sessionRequestList = sessionRequest;
        }
    }

    IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    api->SubmitPendingSessionList(sessionRequestList
    );

    while (sessionRequestList != 0) {
        WestwoodOnlineUpgradeSessionRequest *const nextSessionRequest = sessionRequestList->m_next;
        delete sessionRequestList;
        sessionRequestList = nextSessionRequest;
    }
}

/**
 * Reimplements 0x43efc0: WestwoodOnlineUpgradeDialog::OnQueryControlsChanged
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: refresh the encoded provider query after control changes.
 */
void WestwoodOnlineUpgradeDialog::OnQueryControlsChanged() {
    UpdateSessionListQueryFromControls();
}

/**
 * Reimplements 0x43efd0: WestwoodOnlineUpgradeDialog::OnMaxPlayersEditChange
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: synchronize max-player edit text into dialog data.
 */
void WestwoodOnlineUpgradeDialog::OnMaxPlayersEditChange() {
    ((CWnd *)g_pWestwoodOnlineUpgradeDialog)->UpdateData(1);
}

/**
 * Reimplements 0x43efe0: WestwoodOnlineUpgradeDialog::ShowModalAndGetSelectedMissionIndex
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: run the modal upgrade dialog and return the selected mission index.
 */
int __fastcall WestwoodOnlineUpgradeDialog::ShowModalAndGetSelectedMissionIndex(
    int *selectedMissionIndexOut
) {
    g_hWestwoodOnlineUpgradeModuleInstance =
        (HINSTANCE)((unsigned int)(g_RecoilApp.m_hInstance));
    WestwoodOnlineUpgradeDialog dialog(0);
    WestwoodOnlineUpgradeProgressDialog progressDialog(0);

    g_pWestwoodOnlineUpgradeProgressDialog = &progressDialog;
    g_pWestwoodOnlineUpgradeDialog = &dialog;
    ((CZRecoilFrame *)((unsigned int)(g_RecoilApp.GetMainWnd())))->SetMenuBarVisibility(0);

    g_WestwoodOnlineUpgradeSelectedMissionIndex = -1;
    g_pWestwoodOnlineUpgradeDialog->DoModal();

    ((CZRecoilFrame *)((unsigned int)(g_RecoilApp.GetMainWnd())))->SetMenuBarVisibility(1);

    const int selectedMissionIndex = g_WestwoodOnlineUpgradeSelectedMissionIndex;
    if (selectedMissionIndex != -1) {
        *selectedMissionIndexOut = selectedMissionIndex;
        return 1;
    }

    return 0;
}

/**
 * Reimplements 0x43f440: WestwoodOnlineUpgradeProgressDialog::Destructor (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp).
 * Purpose: delegates progress-dialog teardown to the imported MFC42 CDialog provider destructor.
 */
void WestwoodOnlineUpgradeProgressDialog::Destructor() {
    ((CDialog *)this)->CDialog::~CDialog();
}

/**
 * Reimplements 0x43f450: WestwoodOnlineUpgradeDialog::OnMaxPlayersEditKillFocus
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: clamp and normalize the max-player query edit value.
 */
void WestwoodOnlineUpgradeDialog::OnMaxPlayersEditKillFocus() {
    if ((int)m_queryMaxPlayers < kWolMinPlayersPerSession) {
        ((CWnd *)&m_queryMaxPlayersEdit)
            ->SetWindowTextA(g_WestwoodOnlineUpgradeSingleDigitFieldMinText_2);
        m_queryMaxPlayers = kWolMinPlayersPerSession;
        return;
    }

    if ((int)m_queryMaxPlayers > kWolMaxPlayersPerSession) {
        ((CWnd *)&m_queryMaxPlayersEdit)
            ->SetWindowTextA(g_WestwoodOnlineUpgradeSingleDigitFieldMaxText_4);
        m_queryMaxPlayers = kWolMaxPlayersPerSession;
        return;
    }

    char valueText[kWolEditTextBufferSize];
    _ltoa(
        (long)m_queryMaxPlayers,
        valueText,
        kWolEditTextRadix
    );
    ((CWnd *)&m_queryMaxPlayersEdit)->SetWindowTextA(valueText);
}

/**
 * Reimplements 0x43f4d0: WestwoodOnlineUpgradeDialog::OnAuxParamEditKillFocus
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: clamp and normalize the auxiliary query parameter edit value.
 */
void WestwoodOnlineUpgradeDialog::OnAuxParamEditKillFocus() {
    if ((int)m_queryAuxParam < kWolMinQueryAuxParam) {
        ((CWnd *)&m_queryAuxParamEdit)
            ->SetWindowTextA(g_WestwoodOnlineUpgradeMaxPlayersMinText_1);
        m_queryAuxParam = kWolMinQueryAuxParam;
        return;
    }

    if ((int)m_queryAuxParam > kWolMaxQueryAuxParam) {
        ((CWnd *)&m_queryAuxParamEdit)
            ->SetWindowTextA(g_WestwoodOnlineUpgradeMaxPlayersMaxText_1000);
        m_queryAuxParam = kWolMaxQueryAuxParam;
        return;
    }

    char valueText[kWolEditTextBufferSize];
    _ltoa(
        (long)m_queryAuxParam,
        valueText,
        kWolEditTextRadix
    );
    ((CWnd *)&m_queryAuxParamEdit)->SetWindowTextA(valueText);
}

/**
 * Reimplements 0x43f550: WestwoodOnlineUpgradeDialog::OnValueOrTimeEditKillFocus
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: clamp and normalize the value-or-time query edit value.
 */
void WestwoodOnlineUpgradeDialog::OnValueOrTimeEditKillFocus() {
    if ((int)m_queryValueOrTime < kWolMinQueryValueOrTime) {
        ((CWnd *)&m_queryValueOrTimeEdit)
            ->SetWindowTextA(g_WestwoodOnlineUpgradeSingleDigitFieldMinText_2);
        m_queryValueOrTime = kWolMinQueryValueOrTime;
        return;
    }

    if ((int)m_queryValueOrTime > kWolMaxQueryValueOrTime) {
        ((CWnd *)&m_queryValueOrTimeEdit)
            ->SetWindowTextA(g_WestwoodOnlineUpgradeAuxParamMaxText_2000);
        m_queryValueOrTime = kWolMaxQueryValueOrTime;
        return;
    }

    char valueText[kWolEditTextBufferSize];
    _ltoa(
        (long)m_queryValueOrTime,
        valueText,
        kWolEditTextRadix
    );
    ((CWnd *)&m_queryValueOrTimeEdit)->SetWindowTextA(valueText);
}

/**
 * Reimplements 0x43f5d0: WestwoodOnlineUpgrade::TruncateStringAtFirstSpace
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: trim list-box session text down to the first token before provider
 * requests.
 */
void __fastcall WestwoodOnlineUpgrade::TruncateStringAtFirstSpace(
    char *text
) {
    unsigned int index = 0;
    while (index < strlen(text)) {
        if (text[index] == ' ') {
            text[index] = '\0';
            return;
        }
        ++index;
    }
}

/**
 * Reimplements 0x43f610: WestwoodOnlineUpgradeApiEventSink::CreateInstance.
 * Purpose: Allocates and initializes the Westwood API event sink for COM callbacks.
 */
HRESULT __stdcall WestwoodOnlineUpgradeApiEventSink::CreateInstance(
    WestwoodOnlineUpgradeApiEventSink **outSink
) {
    HRESULT result = E_OUTOFMEMORY;
    WestwoodOnlineUpgradeApiEventSink *eventSink = new WestwoodOnlineUpgradeApiEventSink;

    if (eventSink != 0) {
        eventSink->m_refCountAndLock.Init();
        InterlockedIncrement(&g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount);
        result = S_OK;
    }

    *outSink = eventSink;
    return result;
}

/**
 * Reimplements 0x43f6b0: WestwoodOnlineUpgradeApiEventSink::OnBootstrapServerList.
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeApiEventSink.cpp
 * Purpose: Handles the API event-sink bootstrap-server callback.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::OnBootstrapServerList(
    int resultCode,
    WestwoodOnlineUpgradeBootstrapServerRecord *serverList
) {
    char debugText[4096];
    sprintf(
        debugText,
        /* Retail literal 0x4dd2dc is the compiler-emitted bootstrap-server
           callback result dump format. */
        "\nOnServerList:\n\tResult Code: %d\n",
        resultCode
    );
    zGame::ReturnOnlyStub();

    if (resultCode < 0) {
        SetEvent(g_WestwoodOnlineUpgradeFailureEvent);
        return 0;
    }

    int foundIrcServer = 0;
    WestwoodOnlineUpgradeBootstrapServerRecord *server = serverList;
    while (server != 0) {
        if (strcmp(
            server->m_serverType,
            /* Retail literal 0x4dd2d8 is the compiler-emitted bootstrap
               server type selector for IRC records. */
            "IRC"
        ) == 0 && foundIrcServer == 0) {
            memcpy(
                &g_WestwoodOnlineUpgradeSelectedBootstrapServer,
                server,
                kBootstrapServerCopiedBytes
            );
            foundIrcServer = 1;
        }

        sprintf(
            debugText,
            /* Retail literal 0x4dd298 is the compiler-emitted per-server
               bootstrap diagnostic dump format. */
            "\n\tServer:\n\t\tName: %s\n\t\tType: %s\n\t\tConndata: %s\n\t\tGameType: %d\n",
            server->m_serverName,
            server->m_serverType,
            server->m_connectData,
            server->m_gameType
        );
        zGame::ReturnOnlyStub();
        server = server->m_next;
    }

    {
        CString playerName = g_pWestwoodOnlineUpgradeDialog->GetSelectedProfilePlayerName();
        strcpy(
            g_WestwoodOnlineUpgradeSelectedBootstrapServer.m_playerName,
            (const char *)playerName
        );
    }

    {
        CString connectString = g_pWestwoodOnlineUpgradeDialog->GetSelectedProfileConnectString();
        strcpy(
            g_WestwoodOnlineUpgradeSelectedBootstrapServer.m_connectString,
            (const char *)connectString
        );
    }

    SetEvent(g_WestwoodOnlineUpgradeInitWaitEvents[0]);
    return 0;
}

/**
 * Reimplements 0x43f830: WestwoodOnlineUpgradeApiEventSink::OnDownloadReadyResult.
 * Purpose: Handles patch-download readiness results and opens the download-ready dialog.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::OnDownloadReadyResult(
    int resultCode,
    WestwoodOnlineUpgradeDownloadReadyEntry *downloadReadyList
) {
    char dialogCaption[128];
    char dialogMessage[128];

    if (g_WestwoodOnlineUpgradeApiAsyncErrorFlag != 0) {
        return 0;
    }

    if (resultCode < 0) {
        SetEvent(g_WestwoodOnlineUpgradeFailureEvent);
    }

    if (downloadReadyList == 0) {
        return 0;
    }

    g_WestwoodOnlineUpgradeProcessCallbacksFlag = 0;
    strcpy(
        dialogCaption,
        zLoc::GetMessageString(0x3001)
    );
    strcpy(
        dialogMessage,
        zLoc::GetMessageString(0x3002)
    );
    if (((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->MessageBoxA(
                dialogMessage,
                dialogCaption,
                MB_YESNO | MB_ICONQUESTION
            ) != IDYES) {
        SetEvent(g_WestwoodOnlineUpgradeFailureEvent);
        g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 1;
    } else if (WestwoodOnlineUpgradeDialog::ShowDownloadReadyList(downloadReadyList) != 0) {
        strcpy(
            dialogCaption,
            zLoc::GetMessageString(0x3001)
        );
        strcpy(
            dialogMessage,
            zLoc::GetMessageString(0x3046)
        );
        ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->MessageBoxA(
                dialogMessage,
                dialogCaption,
                MB_ICONEXCLAMATION
            );
        SetEvent(g_WestwoodOnlineUpgradeFailureEvent);
        g_pWestwoodOnlineUpgradeDialog->OnDestroy();
        ExitProcess(0);
    }

    g_WestwoodOnlineUpgradeProcessCallbacksFlag = 1;
    return 0;
}

/**
 * Reimplements 0x43f9d0: WestwoodOnlineUpgradeApiEventSink::OnPendingSessionRequestRemoved.
 * Purpose: Removes or updates pending-session UI state after a session request is removed.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::OnPendingSessionRequestRemoved(
    int status,
    WestwoodOnlineUpgradeSessionRequest *sessionRequest
) {
    char statusText[kPendingSessionRemovedStatusBufferSize];
    LRESULT sessionIndex;

    if (status < 0) {
        return 0;
    }

    zLoc::FormatMessage(
        statusText,
        kPendingSessionRemovedStatusBufferSize,
        kPendingSessionRemovedStatusMessageId,
        sessionRequest->m_sessionName
    );
    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusText);

    sessionIndex = ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
                       ->SendDlgItemMessageA(
                           kWestwoodOnlineUpgradeApiEventSink_SessionResultsListId,
                           LB_FINDSTRINGEXACT,
                           (WPARAM)-1,
                           (LPARAM)sessionRequest->m_sessionName
                       );
    if (sessionIndex != LB_ERR) {
        ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->SendDlgItemMessageA(
                kWestwoodOnlineUpgradeApiEventSink_SessionResultsListId,
                LB_DELETESTRING,
                (WPARAM)sessionIndex,
                0
            );
        --g_WestwoodOnlineUpgradePendingSessionResultCount;
    }

    return 0;
}

/**
 * Reimplements 0x43fa70: WestwoodOnlineUpgradeApiEventSink::OnServerError.
 * Purpose: Displays a Westwood Online server-error dialog for a received error string.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::OnServerError(
    int,
    const char *errorText
) {
    ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
        ->MessageBoxA(
            errorText,
            kWestwoodOnlineUpgradeServerErrorTitle,
            MB_ICONHAND
        );
    return 0;
}

/**
 * Reimplements 0x43fa90: WestwoodOnlineUpgradeApiEventSink::OnApiStatus.
 * Purpose: Formats API status results and appends them to the upgrade dialog result list.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::OnApiStatus(
    int statusCode,
    const char *statusText
) {
    int resultCount;
    char failureCaption[kApiStatusTextBufferSize];
    char failureMessage[kApiStatusTextBufferSize];
    char resultCountStatusText[kApiStatusTextBufferSize];
    int failureMessageId;
    UINT messageBoxFlags;
    char *statusLine;
    IWestwoodOnlineUpgradeProviderApiCallbacks *api;

    Time::Reset();

    if (statusCode == 0) {
        statusLine = strtok(
            _strdup(statusText),
            kApiStatusLineDelimiter
        );
        while (statusLine != 0) {
            g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusLine);
            statusLine = strtok(
                0,
                kApiStatusLineDelimiter
            );
        }

        SetEvent(g_WestwoodOnlineUpgradeStatusTextEvent);
        // Retail frees the final strtok result, which is NULL here, leaking the
        // duplicated status text buffer.
        free(statusLine);

        api = GetCallbackApiComObject();
        api->SetQueryMode(kApiStatusActiveListMode);
        g_WestwoodOnlineUpgradeActiveListMode = kApiStatusActiveListMode;
        api->GetQueryResultCount(&resultCount);

        if (resultCount == 0) {
            g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(
                zLoc::GetMessageString(kApiStatusNoResultsMessageId)
            );
        } else if (resultCount == 1) {
            g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(
                zLoc::GetMessageString(kApiStatusOneResultMessageId)
            );
        } else {
            zLoc::FormatMessage(
                resultCountStatusText,
                kApiStatusTextBufferSize,
                kApiStatusMultipleResultsMessageId,
                resultCount
            );
            g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(resultCountStatusText);
        }

        return 0;
    }

    ((CWnd *)g_pWestwoodOnlineUpgradeProgressDialog)->DestroyWindow();
    strcpy(
        failureCaption,
        zLoc::GetMessageString(kApiStatusFailureCaptionMessageId)
    );
    messageBoxFlags = MB_ICONEXCLAMATION;

    if (statusCode == kApiStatusFailure64) {
        failureMessageId = kApiStatusFailure64MessageId;
    } else if (statusCode == kApiStatusFailure65) {
        failureMessageId = kApiStatusFailure65MessageId;
    } else if (statusCode == kApiStatusFailure6a) {
        failureMessageId = kApiStatusFailure6aMessageId;
    } else if (statusCode == kApiStatusFailure72) {
        failureMessageId = kApiStatusFailure72MessageId;
    } else {
        failureMessageId = kApiStatusFailureDefaultMessageId;
        messageBoxFlags = MB_ICONHAND;
    }

    strcpy(
        failureMessage,
        zLoc::GetMessageString(failureMessageId)
    );
    ((CWnd *)((unsigned int)g_RecoilApp.m_pMainWnd))
        ->MessageBoxA(
            failureMessage,
            failureCaption,
            messageBoxFlags
        );
    g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 1;
    SetEvent(g_WestwoodOnlineUpgradeFailureEvent);
    return 0;
}

/**
 * Reimplements 0x43fde0: WestwoodOnlineUpgradeApiEventSink::OnStatusTextReceived.
 * Purpose: Writes received status text into the Westwood Online status display.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::OnStatusTextReceived(
    int status,
    const char *statusText
) {
    char *statusLine;

    if (status < 0) {
        return 0;
    }

    // Retail duplicates statusText for strtok and never frees the duplicate.
    statusLine = strtok(
        _strdup(statusText),
        kApiStatusLineDelimiter
    );
    while (statusLine != 0) {
        g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusLine);
        statusLine = strtok(
            0,
            kApiStatusLineDelimiter
        );
    }

    SetEvent(g_WestwoodOnlineUpgradeStatusTextEvent);
    return 0;
}

/**
 * Reimplements 0x43fe50: WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordAdded.
 * Purpose: Appends a browse-record row and enables the game button when appropriate.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordAdded(
    int status,
    WestwoodOnlineUpgradeBrowseRecord *browseRecord
) {
    char statusText[kBrowseRecordAddedStatusBufferSize];
    IWestwoodOnlineUpgradeProviderApiCallbacks *api;

    if (status < 0) {
        g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(
            zLoc::GetMessageString(kBrowseRecordAddedFailureMessageId)
        );
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 0;
        g_pWestwoodOnlineUpgradeDialog->EnableQueryControls(0);
        g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] = '\0';
        api = GetCallbackApiComObject();
        api->RequestListMode(
            g_WestwoodOnlineUpgradeActiveListMode,
            1
        );
        return 0;
    }

    const unsigned int messageId = browseRecord->m_recordFlags != 0
                                       ? kBrowseRecordAddedClosedMessageId
                                       : kBrowseRecordAddedOpenMessageId;
    zLoc::FormatMessage(
        statusText,
        kBrowseRecordAddedStatusBufferSize,
        messageId,
        browseRecord->m_sessionName
    );
    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusText);

    if (browseRecord->m_recordFlags != 0) {
        g_pWestwoodOnlineUpgradeDialog->EnableQueryControls(1);
        ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->SetDlgItemTextA(
                kWestwoodOnlineUpgradeGameButtonId,
                browseRecord->m_sessionName
            );
    }

    g_WestwoodOnlineUpgradeCachedBrowseRecord = *browseRecord;
    ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
        ->SendDlgItemMessageA(
            kWestwoodOnlineUpgradeApiEventSink_BrowseRecordListId,
            LB_INSERTSTRING,
            (WPARAM)-1,
            (LPARAM)browseRecord->m_sessionName
        );
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 1;
    return 0;
}

/**
 * Reimplements 0x43ff80: WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordAndSessionResolved.
 * Purpose: Resolves browse-record/session details and updates session row status text.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordAndSessionResolved(
    int status,
    WestwoodOnlineUpgradeBrowseRecord *browseRecord,
    WestwoodOnlineUpgradeSessionRequest *sessionRequest
) {
    char statusText[kBrowseSessionResolvedStatusBufferSize];
    unsigned int failureMessageId;
    IWestwoodOnlineUpgradeProviderApiCallbacks *api;

    if (status < 0) {
        strcpy(
            statusText,
            zLoc::GetMessageString(kBrowseSessionResolvedFailurePrefixMessageId)
        );
        strcat(
            statusText,
            kSpaceDelimiter
        );

        if (status == kBrowseSessionResolvedFailure6c) {
            failureMessageId = kBrowseSessionResolvedFailure6cMessageId;
        } else if (status == kBrowseSessionResolvedFailure70) {
            failureMessageId = kBrowseSessionResolvedFailure70MessageId;
        } else if (status == kBrowseSessionResolvedFailure72) {
            failureMessageId = kBrowseSessionResolvedFailure72MessageId;
        } else if (status == kBrowseSessionResolvedFailure71) {
            failureMessageId = kBrowseSessionResolvedFailure71MessageId;
        } else if (status == kBrowseSessionResolvedFailure6e) {
            failureMessageId = kBrowseSessionResolvedFailure6eMessageId;
        } else {
            g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(
                kBrowseSessionResolvedStatusCodeFmt,
                zLoc::GetMessageString(kBrowseSessionResolvedFailurePrefixMessageId),
                status
            );
            g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 0;
            g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] = '\0';
            api = GetCallbackApiComObject();
            api->RequestListMode(
                g_WestwoodOnlineUpgradeActiveListMode,
                1
            );
            return 0;
        }

        strcat(
            statusText,
            zLoc::GetMessageString(failureMessageId)
        );
        g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusText);
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 0;
        g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] = '\0';
        api = GetCallbackApiComObject();
        api->RequestListMode(
            g_WestwoodOnlineUpgradeActiveListMode,
            1
        );
        return 0;
    }

    if ((sessionRequest->m_rowFlags & kSessionRequestRefreshCacheFlag) != 0) {
        g_WestwoodOnlineUpgradeCachedBrowseRecord = *browseRecord;
        g_pWestwoodOnlineUpgradeDialog->EnableQueryControls(0);
    }

    if ((sessionRequest->m_rowFlags & kSessionRequestSkipDetailsFlag) == 0) {
        api = GetCallbackApiComObject();
        api->RequestListMode(
            g_WestwoodOnlineUpgradeActiveListMode,
            1
        );
        if (browseRecord->m_recordFlags != 0 &&
            g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag == 1) {
            WestwoodOnlineUpgradeDialog::UpdateSessionListQueryFromControls();
        }

        ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->SetDlgItemTextA(
                kWestwoodOnlineUpgradeGameButtonId,
                browseRecord->m_sessionName
            );
        zLoc::FormatMessage(
            statusText,
            kBrowseSessionResolvedStatusBufferSize,
            kBrowseSessionResolvedStatusMessageId,
            sessionRequest->m_sessionName,
            browseRecord->m_sessionName
        );
        g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusText);
        ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->SendDlgItemMessageA(
                kWestwoodOnlineUpgradeApiEventSink_SessionResultsListId,
                LB_ADDSTRING,
                0,
                (LPARAM)sessionRequest->m_sessionName
            );
        ++g_WestwoodOnlineUpgradePendingSessionResultCount;
        g_pWestwoodOnlineUpgradeDialog->EnableConnectButton(1);
        api = GetCallbackApiComObject();
        api->RequestSessionDetails(
            sessionRequest
        );
    }

    return 0;
}

/**
 * Reimplements 0x4401d0: WestwoodOnlineUpgradeApiEventSink::OnSessionQueryFinished.
 * Purpose: Finalizes session-query state and requests detail data for pending sessions.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::OnSessionQueryFinished(
    int status,
    WestwoodOnlineUpgradeBrowseRecord *browseRecord,
    WestwoodOnlineUpgradeSessionRequest *sessionRequest
) {
    char statusText[kBrowseSessionResolvedStatusBufferSize];
    LRESULT sessionIndex;
    IWestwoodOnlineUpgradeProviderApiCallbacks *api;

    if (status < 0) {
        api = GetCallbackApiComObject();
        api->RequestListMode(
            kApiStatusActiveListMode,
            1
        );
        api = GetCallbackApiComObject();
        api->CancelPendingSessionFlow();
        return 0;
    }

    zLoc::FormatMessage(
        statusText,
        kBrowseSessionResolvedStatusBufferSize,
        kSessionQueryFinishedStatusMessageId,
        sessionRequest->m_sessionName,
        browseRecord->m_sessionName
    );
    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusText);

    sessionIndex = ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
                       ->SendDlgItemMessageA(
                           kWestwoodOnlineUpgradeApiEventSink_SessionResultsListId,
                           LB_FINDSTRINGEXACT,
                           (WPARAM)-1,
                           (LPARAM)sessionRequest->m_sessionName
                       );
    if (sessionIndex != LB_ERR) {
        ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->SendDlgItemMessageA(
                kWestwoodOnlineUpgradeApiEventSink_SessionResultsListId,
                LB_DELETESTRING,
                (WPARAM)sessionIndex,
                0
            );
        --g_WestwoodOnlineUpgradePendingSessionResultCount;
    }

    if ((sessionRequest->m_rowFlags & kSessionRequestSkipDetailsFlag) != 0 &&
        (sessionRequest->m_rowFlags & kSessionRequestRefreshCacheFlag) == 0) {
        g_pWestwoodOnlineUpgradeDialog->AppendConnectStatusAndRefreshList(
            browseRecord->m_sessionName
        );
    }

    api = GetCallbackApiComObject();
    api->RequestListMode(
        g_WestwoodOnlineUpgradeActiveListMode,
        1
    );
    return 0;
}

/**
 * Reimplements 0x4402c0: WestwoodOnlineUpgradeApiEventSink::OnSessionListEnumerated.
 * Purpose: Enumerates session records and appends visible rows for the upgrade dialog.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::OnSessionListEnumerated(
    int status,
    WestwoodOnlineUpgradeBrowseRecord *browseRecord,
    WestwoodOnlineUpgradeSessionRequest *sessionList
) {
    int deferAutoConnect;
    int rowFlags;
    char sessionResultText[kSessionListEnumeratedResultTextBufferSize];
    LRESULT sessionIndex;
    WestwoodOnlineUpgradeSessionRequest *sessionNode;

    if (status < 0) {
        return 0;
    }

    ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
        ->SendDlgItemMessageA(
            kWestwoodOnlineUpgradeApiEventSink_SessionResultsListId,
            LB_RESETCONTENT,
            0,
            0
        );
    g_WestwoodOnlineUpgradePendingSessionResultCount = 0;
    deferAutoConnect = 0;
    sessionNode = sessionList;

    while (sessionNode != 0) {
        rowFlags = sessionNode->m_rowFlags;
        ++g_WestwoodOnlineUpgradePendingSessionResultCount;
        strcpy(
            sessionResultText,
            sessionNode->m_sessionName
        );

        if ((rowFlags & kSessionRequestRefreshCacheFlag) != 0) {
            if ((rowFlags & kSessionRequestSkipDetailsFlag) != 0) {
                strcat(
                    sessionResultText,
                    zLoc::GetMessageString(kSessionListEnumeratedReadyMessageId)
                );
                deferAutoConnect = 1;
                if (g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag == 0) {
                    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 1;
                    g_pWestwoodOnlineUpgradeDialog->EnableQueryControls(1);
                }
            } else {
                strcat(
                    sessionResultText,
                    zLoc::GetMessageString(kSessionListEnumeratedClosedMessageId)
                );
                g_pWestwoodOnlineUpgradeDialog->EnableQueryControls(0);
                g_pWestwoodOnlineUpgradeDialog->EnableConnectButton(1);
            }
        } else if ((rowFlags & kSessionRequestSkipDetailsFlag) != 0) {
            strcat(
                sessionResultText,
                zLoc::GetMessageString(kSessionListEnumeratedPendingMessageId)
            );
            deferAutoConnect = 1;
        }

        sessionIndex = ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
                           ->SendDlgItemMessageA(
                               kWestwoodOnlineUpgradeApiEventSink_SessionResultsListId,
                               LB_ADDSTRING,
                               0,
                               (LPARAM)sessionResultText
                           );
        ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->SendDlgItemMessageA(
                kWestwoodOnlineUpgradeApiEventSink_SessionResultsListId,
                LB_SETITEMDATA,
                (WPARAM)sessionIndex,
                rowFlags
            );
        sessionNode = sessionNode->m_next;
    }

    if (deferAutoConnect == 0) {
        g_pWestwoodOnlineUpgradeDialog->AppendConnectStatusAndRefreshList(
            browseRecord->m_sessionName
        );
    }

    return 0;
}

/**
 * Reimplements 0x4404c0: WestwoodOnlineUpgradeApiEventSink::LaunchSelectedSession.
 * Purpose: Applies selected-session connection details and starts the network launch path.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::LaunchSelectedSession(
    int status,
    int,
    WestwoodOnlineUpgradeSessionRequest *selectedSessionList,
    int
) {
    int launched;
    int showConnectionFailureMessage;
    char hostAddressText[20];
    zNetworkSessionDescStatusFields statusFields;
    char failureCaptionText[128];
    char failureMessageText[128];
    WestwoodOnlineUpgradeSessionRequest *selectedSessionNode;
    WestwoodOnlineUpgradeDialog *dialog;
    IWestwoodOnlineUpgradeProviderApiCallbacks *api;

    if (status < 0) {
        return 0;
    }

    launched = 0;
    showConnectionFailureMessage = 0;
    selectedSessionNode = selectedSessionList;

    ((CWnd *)g_pWestwoodOnlineUpgradeDialog)->UpdateData(TRUE);
    while (selectedSessionNode != 0 &&
           (selectedSessionNode->m_rowFlags & kSessionRequestSkipDetailsFlag) == 0) {
        selectedSessionNode = selectedSessionNode->m_next;
    }

    if (selectedSessionNode != 0) {
        zNetwork::InitSessionRuntime(&g_zNetwork_WestwoodOnlineAppGuid);
        Net::FormatIpv4Address(
            hostAddressText,
            selectedSessionNode->m_hostIpv4Packed
        );

        if (zNetworkDPlay::SelectTcpIpProviderAndEnumSessions(
                hostAddressText,
                g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag
            ) == 0) {
            showConnectionFailureMessage = 1;
        } else if (g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag != 0) {
            dialog = g_pWestwoodOnlineUpgradeDialog;
            statusFields.statusFlags = 0;
            if (dialog->m_queryStatusFlagBit0 != 0) {
                statusFields.statusFlags = 1;
            }
            if (dialog->m_queryStatusFlagBit1 != 0) {
                statusFields.statusFlags |= 2;
            }

            statusFields.eventCode =
                (int)SendMessageA(
                    dialog->m_sessionModeCombo.m_hWnd,
                    CB_GETCURSEL,
                    0,
                    0
                ) + 1;
            statusFields.valueOrTime = dialog->m_queryValueOrTime;
            statusFields.auxParam = dialog->m_queryAuxParam;
            statusFields.maxPlayers = dialog->m_queryMaxPlayers;
            strcpy(
                statusFields.sessionNameBuf,
                (const char *)dialog->m_sessionName
            );

            if (zNetwork_DPlay::CreateSessionFromStatusFields(&statusFields) != 0) {
                zOpt::SetNetworkEnabled(1);
                CString selectedPlayerName =
                    g_pWestwoodOnlineUpgradeDialog->GetSelectedProfilePlayerName();
                zNetwork_DPlay::CreateLocalPlayerRecordAndRegister(
                    (char *)(const char *)selectedPlayerName
                );
                launched = 1;
            }
        } else {
            statusFields.selectedSessionIndex = 0;
            if (zNetworkDPlay::OpenSelectedSessionAndReadStatusFields(&statusFields) == 0) {
                showConnectionFailureMessage = 1;
            } else {
                zOpt::SetNetworkEnabled(1);
                {
                    CString selectedPlayerName =
                        g_pWestwoodOnlineUpgradeDialog->GetSelectedProfilePlayerName();
                    zNetwork_DPlay::CreateLocalPlayerRecordAndRegister(
                        (char *)(const char *)selectedPlayerName
                    );
                }

                {
                    CString selectedPlayerName =
                        g_pWestwoodOnlineUpgradeDialog->GetSelectedProfilePlayerName();
                    zOpt::SetPlayerName((const char *)selectedPlayerName);
                }
                launched = 1;
            }
        }

        if (launched != 0) {
            union {
                float value;
                int raw;
            } timerSeconds = {(float)statusFields.valueOrTime * 60.0f};
            GameNet::SetStatusBitsFromFlags(statusFields.statusFlags);
            g_HudSensorTracker.SetRuntimeTimerSecAndGoalValue(
                timerSeconds.raw,
                statusFields.auxParam
            );
            g_WestwoodOnlineUpgradeSelectedMissionIndex = statusFields.eventCode;
        }
    }

    if (showConnectionFailureMessage != 0) {
        strcpy(
            failureCaptionText,
            zLoc::GetMessageString(kApiStatusFailureCaptionMessageId)
        );
        strcpy(
            failureMessageText,
            zLoc::GetMessageString(kApiStatusFailureDefaultMessageId)
        );
        MessageBeep(MB_ICONHAND);
        ((CWnd *)g_RecoilApp.m_pMainWnd)
            ->MessageBoxA(
                failureMessageText,
                failureCaptionText,
                MB_ICONHAND
            );
    }

    api = GetCallbackApiComObject();
    api->Disconnect();
    return 0;
}

/**
 * Reimplements 0x4407e0: WestwoodOnlineUpgradeApiEventSink::ApplyEncodedQueryString1.
 * Purpose: Parses encoded query data for the alternate session result callback shape.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::ApplyEncodedQueryString1(
    int status,
    int,
    int,
    char *encodedQuery
) {
    WestwoodOnlineUpgradeDialog *dialog;

    if (status < 0) {
        return 0;
    }

    CString encodedQueryText(encodedQuery);
    CString queryFieldText;
    dialog = g_pWestwoodOnlineUpgradeDialog;
    queryFieldText = encodedQueryText.Mid(
        0,
        1
    );
    SendMessageA(
        dialog->m_sessionModeCombo.m_hWnd,
        CB_SETCURSEL,
        atoi((const char *)queryFieldText),
        0
    );

    queryFieldText = encodedQueryText.Mid(
        1,
        4
    );
    dialog->m_queryValueOrTime = atoi((const char *)queryFieldText);

    queryFieldText = encodedQueryText.Mid(
        5,
        4
    );
    dialog->m_queryAuxParam = atoi((const char *)queryFieldText);

    queryFieldText = encodedQueryText.Mid(
        9,
        1
    );
    dialog->m_queryMaxPlayers = atoi((const char *)queryFieldText);

    queryFieldText = encodedQueryText.Mid(
        10,
        1
    );
    dialog->m_queryStatusFlagBit0 = atoi((const char *)queryFieldText);

    queryFieldText = encodedQueryText.Mid(
        11,
        1
    );
    dialog->m_queryStatusFlagBit1 = atoi((const char *)queryFieldText);

    ((CWnd *)dialog)->UpdateData(FALSE);
    return 0;
}

/**
 * Reimplements 0x440a30: WestwoodOnlineUpgradeApiEventSink::ApplyEncodedQueryString0.
 * Purpose: Parses encoded query data for one Westwood Online session result variant.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::ApplyEncodedQueryString0(
    int status,
    int,
    char *encodedQuery
) {
    WestwoodOnlineUpgradeDialog *dialog;

    if (status < 0) {
        return 0;
    }

    CString encodedQueryText(encodedQuery);
    CString queryFieldText;
    dialog = g_pWestwoodOnlineUpgradeDialog;
    queryFieldText = encodedQueryText.Mid(
        0,
        1
    );
    SendMessageA(
        dialog->m_sessionModeCombo.m_hWnd,
        CB_SETCURSEL,
        atoi((const char *)queryFieldText),
        0
    );

    queryFieldText = encodedQueryText.Mid(
        1,
        4
    );
    dialog->m_queryValueOrTime = atoi((const char *)queryFieldText);

    queryFieldText = encodedQueryText.Mid(
        5,
        4
    );
    dialog->m_queryAuxParam = atoi((const char *)queryFieldText);

    queryFieldText = encodedQueryText.Mid(
        9,
        1
    );
    dialog->m_queryMaxPlayers = atoi((const char *)queryFieldText);

    queryFieldText = encodedQueryText.Mid(
        10,
        1
    );
    dialog->m_queryStatusFlagBit0 = atoi((const char *)queryFieldText);

    queryFieldText = encodedQueryText.Mid(
        11,
        1
    );
    dialog->m_queryStatusFlagBit1 = atoi((const char *)queryFieldText);

    ((CWnd *)dialog)->UpdateData(FALSE);
    return 0;
}

/**
 * Reimplements 0x440c80: WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301B.
 * Purpose: Appends localized 0x301b status text for a session request.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301B(
    int status,
    int,
    WestwoodOnlineUpgradeSessionRequest *sessionRequest,
    const char *statusText
) {
    char statusMessageText[kSessionRequestStatusBufferSize];

    if (status < 0) {
        return 0;
    }

    zLoc::FormatMessage(
        statusMessageText,
        kSessionRequestStatusBufferSize,
        kSessionRequestStatus301BMessageId,
        sessionRequest->m_sessionName,
        statusText
    );
    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusMessageText);
    return 0;
}

/**
 * Reimplements 0x440ce0: WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301C.
 * Purpose: Appends localized 0x301c status text for a session request.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301C(
    int status,
    WestwoodOnlineUpgradeSessionRequest *sessionRequest,
    const char *statusText
) {
    char statusMessageText[kSessionRequestStatusBufferSize];

    if (status < 0) {
        return 0;
    }

    zLoc::FormatMessage(
        statusMessageText,
        kSessionRequestStatusBufferSize,
        kSessionRequestStatus301CMessageId,
        sessionRequest->m_sessionName,
        statusText
    );
    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusMessageText);
    return 0;
}

/**
 * Reimplements 0x440d40: WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301D.
 * Purpose: Appends localized 0x301d status text for a session request.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301D(
    int,
    WestwoodOnlineUpgradeSessionRequest *sessionRequest,
    const char *statusText
) {
    char statusMessageText[kSessionRequestStatusBufferSize];

    zLoc::FormatMessage(
        statusMessageText,
        kSessionRequestStatusBufferSize,
        kSessionRequestStatus301DMessageId,
        sessionRequest->m_sessionName,
        statusText
    );
    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusMessageText);
    return 0;
}

/**
 * Reimplements 0x440d90: WestwoodOnlineUpgradeApiEventSink::AppendConnectStatus301E_3021.
 * Purpose: Maps connection status codes to localized connect-status text.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::AppendConnectStatus301E_3021(
    int connectionStatusCode
) {
    unsigned int messageId;

    if (connectionStatusCode == 0) {
        messageId = kConnectStatusDefaultMessageId;
    } else if (connectionStatusCode == kConnectStatusCode40134) {
        messageId = kConnectStatusCode40134MessageId;
    } else if (connectionStatusCode == kConnectStatusCode40133) {
        messageId = kConnectStatusCode40133MessageId;
    } else {
        messageId = kConnectStatusFallbackMessageId;
    }

    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(zLoc::GetMessageString(messageId));
    return 0;
}

/**
 * Reimplements 0x440e10: WestwoodOnlineUpgradeApiEventSink::AppendBrowseRecordStatus3022_3025.
 * Purpose: Maps browse-record status codes to localized browse-status text.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::AppendBrowseRecordStatus3022_3025(
    int status,
    WestwoodOnlineUpgradeBrowseRecord *browseRecord
) {
    unsigned int messageId;
    char statusMessageText[kSessionRequestStatusBufferSize];

    if (browseRecord != 0) {
        zLoc::FormatMessage(
            statusMessageText,
            kSessionRequestStatusBufferSize,
            kBrowseRecordStatusNamedMessageId,
            browseRecord->m_sessionName
        );
        g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusMessageText);
        return 0;
    }

    if (status == kBrowseRecordStatusCode40131) {
        messageId = kBrowseRecordStatusCode40131MessageId;
    } else if (status == kBrowseRecordStatusCode40130) {
        messageId = kBrowseRecordStatusCode40130MessageId;
    } else if (status == kBrowseRecordStatusCode40132) {
        messageId = kBrowseRecordStatusCode40132MessageId;
    } else {
        messageId = kBrowseRecordStatusFallbackMessageId;
    }

    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(zLoc::GetMessageString(messageId));
    return 0;
}

/**
 * Reimplements 0x440ef0: WestwoodOnlineUpgradeApiEventSink::AppendValueStatus3026.
 * Purpose: Appends localized 0x3026 value text for a session status field.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::AppendValueStatus3026(
    int status,
    int value
) {
    char statusMessageText[kSessionRequestStatusBufferSize];

    if (status < 0) {
        return 0;
    }

    zLoc::FormatMessage(
        statusMessageText,
        kSessionRequestStatusBufferSize,
        kValueStatus3026MessageId,
        value
    );
    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusMessageText);
    return 0;
}

/**
 * Reimplements 0x440f40: WestwoodOnlineUpgradeApiEventSink::OnNetworkStatusChanged.
 * Purpose: Updates network status text and side effects for Westwood connection changes.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::OnNetworkStatusChanged(
    int connectionStatusCode
) {
    const char *statusName;
    char debugStatusText[kSessionRequestStatusBufferSize];

    if (connectionStatusCode == kNetworkStatusConnectError) {
        statusName = kNetworkStatusConnectErrorText;
    } else if (connectionStatusCode == kNetworkStatusNetworkDown) {
        statusName = kNetworkStatusNetworkDownText;
    } else if (connectionStatusCode == kNetworkStatusLookupFailed) {
        statusName = kNetworkStatusLookupFailedText;
    } else if (connectionStatusCode == kNetworkStatusTimeout) {
        statusName = kNetworkStatusTimeoutText;
    } else if (connectionStatusCode == kNetworkStatusConnecting) {
        statusName = kNetworkStatusConnectingText;
    } else if (connectionStatusCode == kNetworkStatusConnected) {
        statusName = kNetworkStatusConnectedText;
    } else if (connectionStatusCode == kNetworkStatusDisconnecting) {
        statusName = kNetworkStatusDisconnectingText;
    } else if (connectionStatusCode == kNetworkStatusDisconnected) {
        statusName = kNetworkStatusDisconnectedText;
    } else {
        statusName = kNetworkStatusUnknownText;
    }

    sprintf(
        debugStatusText,
        kNetworkStatusDebugFormat,
        statusName,
        connectionStatusCode
    );
    zGame::ReturnOnlyStub();

    if (connectionStatusCode == kNetworkStatusDisconnected &&
        g_WestwoodOnlineUpgradeAbortFlag == 0) {
        g_pWestwoodOnlineUpgradeDialog->SetAbortAndClose();
        return 0;
    }

    if (connectionStatusCode == kNetworkStatusConnectError ||
        connectionStatusCode == kNetworkStatusNetworkDown ||
        connectionStatusCode == kNetworkStatusTimeout) {
        g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 1;
    }

    return 0;
}

/**
 * Reimplements 0x441040: WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordListReceived.
 * Purpose: Rebuilds the browse-record list display from a received record list.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordListReceived(
    int status,
    WestwoodOnlineUpgradeBrowseRecord *browseRecordList
) {
    WestwoodOnlineUpgradeBrowseRecord *currentRecord;
    const char *latencyBarText;
    char rowText[kBrowseRecordListRowTextBufferSize];
    int latencyMs;
    int cacheIndex;

    g_WestwoodOnlineUpgradeCachedBrowseRecordListCount = 0;
    if (status < 0) {
        return 0;
    }

    ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
        ->SendDlgItemMessageA(
            kWestwoodOnlineUpgradeApiEventSink_BrowseRecordListId,
            LB_RESETCONTENT,
            0,
            0
        );

    currentRecord = browseRecordList;
    while (currentRecord != 0) {
        if (currentRecord->m_recordFlags == 0) {
            zLoc::FormatMessage(
                rowText,
                kBrowseRecordListRowTextBufferSize,
                kBrowseRecordListOpenMessageId,
                currentRecord->m_sessionName,
                currentRecord->m_displayMetric1
            );
        } else {
            latencyMs = currentRecord->m_latencyMs;
            if (latencyMs == kBrowseRecordLatencyUnknown) {
                latencyBarText = kBrowseRecordLatencyUnknownText;
            } else if (latencyMs <= kBrowseRecordLatencyStep0Max) {
                latencyBarText = kBrowseRecordLatencyStep0Text;
            } else if (latencyMs <= kBrowseRecordLatencyStep1Max) {
                latencyBarText = kBrowseRecordLatencyStep1Text;
            } else if (latencyMs <= kBrowseRecordLatencyStep2Max) {
                latencyBarText = kBrowseRecordLatencyStep2Text;
            } else if (latencyMs <= kBrowseRecordLatencyStep3Max) {
                latencyBarText = kBrowseRecordLatencyStep3Text;
            } else if (latencyMs <= kBrowseRecordLatencyStep4Max) {
                latencyBarText = kBrowseRecordLatencyStep4Text;
            } else if (latencyMs <= kBrowseRecordLatencyStep5Max) {
                latencyBarText = kBrowseRecordLatencyStep5Text;
            } else if (latencyMs <= kBrowseRecordLatencyStep6Max) {
                latencyBarText = kBrowseRecordLatencyStep6Text;
            } else {
                latencyBarText = kBrowseRecordLatencyStep7Text;
            }

            zLoc::FormatMessage(
                rowText,
                kBrowseRecordListRowTextBufferSize,
                kBrowseRecordListClosedMessageId,
                currentRecord->m_sessionName,
                currentRecord->m_displayMetric1,
                currentRecord->m_displayMetric0,
                latencyBarText
            );
        }

        ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->SendDlgItemMessageA(
                kWestwoodOnlineUpgradeApiEventSink_BrowseRecordListId,
                LB_INSERTSTRING,
                (WPARAM)-1,
                (LPARAM)rowText
            );
        cacheIndex = g_WestwoodOnlineUpgradeCachedBrowseRecordListCount;
        g_WestwoodOnlineUpgradeCachedBrowseRecordList[cacheIndex] = *currentRecord;
        g_WestwoodOnlineUpgradeCachedBrowseRecordList[cacheIndex].m_next = 0;
        g_WestwoodOnlineUpgradeCachedBrowseRecordListCount = cacheIndex + 1;
        currentRecord = currentRecord->m_next;
    }

    return 0;
}

/**
 * Reimplements 0x4411c0: WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301C_Alt0.
 * Purpose: Appends the first numeric 0x301c status variant for a session request.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301C_Alt0(
    int status,
    WestwoodOnlineUpgradeSessionRequest *sessionRequest,
    int value
) {
    if (status < 0) {
        return 0;
    }

    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(
        zLoc::GetMessageString(kSessionRequestStatus301CMessageId),
        sessionRequest->m_sessionName,
        value
    );
    return 0;
}

/**
 * Reimplements 0x441200: WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301C_Alt1.
 * Purpose: Appends the second numeric 0x301c status variant for a session request.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301C_Alt1(
    int status,
    int,
    WestwoodOnlineUpgradeSessionRequest *sessionRequest,
    int value
) {
    if (status < 0) {
        return 0;
    }

    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(
        zLoc::GetMessageString(kSessionRequestStatus301CMessageId),
        sessionRequest->m_sessionName,
        value
    );
    return 0;
}

/**
 * Reimplements 0x441240: WestwoodOnlineUpgradeApiEventSink::CallbackNoOp0.
 * Purpose: Handles an unused four-argument API event callback with a zero result.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::CallbackNoOp0(
    int,
    int,
    int
) {
    return 0;
}

/**
 * Reimplements 0x441250: WestwoodOnlineUpgradeApiEventSink::CallbackNoOp1.
 * Purpose: Handles an unused three-argument API event callback with a zero result.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::CallbackNoOp1(
    int,
    int
) {
    return 0;
}

/**
 * Reimplements 0x441260: WestwoodOnlineUpgradeApiEventSink::AppendTimeStatus302A.
 * Purpose: Converts a session timestamp to localized 0x302a status text.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::AppendTimeStatus302A(
    int status,
    long unixTime
) {
    char statusMessageText[kSessionRequestStatusBufferSize];
    time_t timeValue;

    if (status == 0) {
        timeValue = (time_t)unixTime;
        zLoc::FormatMessage(
            statusMessageText,
            kSessionRequestStatusBufferSize,
            kTimeStatus302AMessageId,
            ctime(&timeValue)
        );
        g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusMessageText);
    }

    return 0;
}

/**
 * Reimplements 0x4412c0: WestwoodOnlineUpgradeApiEventSink::AppendValueStatus302B_302C.
 * Purpose: Appends one of the localized value status messages for a numeric field.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::AppendValueStatus302B_302C(
    int,
    int value,
    int usePrimaryMessage
) {
    char statusMessageText[kSessionRequestStatusBufferSize];
    unsigned int messageId;

    messageId = usePrimaryMessage != 0 ? kValueStatus302BMessageId : kValueStatus302CMessageId;
    zLoc::FormatMessage(
        statusMessageText,
        kSessionRequestStatusBufferSize,
        messageId,
        value
    );
    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusMessageText);
    return 0;
}

/**
 * Reimplements 0x441350: WestwoodOnlineUpgradeApiEventSink::UpdateSessionResultItemFlags.
 * Purpose: Updates the visible suffix flags for a matching session result row.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::UpdateSessionResultItemFlags(
    int,
    const char *sessionName,
    int flags,
    int
) {
    char rowText[kSessionResultRowTextBufferSize];
    LRESULT sessionIndex;

    sessionIndex = g_pWestwoodOnlineUpgradeDialog->SendDlgItemMessageA(
        kWestwoodOnlineUpgradeApiEventSink_SessionResultsListId,
        LB_FINDSTRING,
        (WPARAM)-1,
        (LPARAM)sessionName
    );
    if (sessionIndex == LB_ERR) {
        return 0;
    }

    g_pWestwoodOnlineUpgradeDialog->SendDlgItemMessageA(
        kWestwoodOnlineUpgradeApiEventSink_SessionResultsListId,
        LB_GETITEMDATA,
        (WPARAM)sessionIndex,
        0
    );
    g_pWestwoodOnlineUpgradeDialog->SendDlgItemMessageA(
        kWestwoodOnlineUpgradeApiEventSink_SessionResultsListId,
        LB_SETITEMDATA,
        (WPARAM)sessionIndex,
        flags
    );
    g_pWestwoodOnlineUpgradeDialog->SendDlgItemMessageA(
        kWestwoodOnlineUpgradeApiEventSink_SessionResultsListId,
        LB_DELETESTRING,
        (WPARAM)sessionIndex,
        0
    );

    strcpy(
        rowText,
        sessionName
    );
    if ((flags & 1) != 0) {
        strcat(
            rowText,
            kSessionResultReadySuffix
        );
    }
    strcat(
        rowText,
        kSessionResultPendingSuffix
    );
    g_pWestwoodOnlineUpgradeDialog->SendDlgItemMessageA(
        kWestwoodOnlineUpgradeApiEventSink_SessionResultsListId,
        LB_ADDSTRING,
        0,
        (LPARAM)rowText
    );

    return 0;
}

/**
 * Reimplements 0x441480: WestwoodOnlineUpgradeApiEventSink::OnSessionLaunchResult.
 * Purpose: Handles launch success/failure state for a selected Westwood session.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::OnSessionLaunchResult(
    int status,
    WestwoodOnlineUpgradeBrowseRecord *browseRecord,
    WestwoodOnlineUpgradeSessionRequest *sessionNode,
    WestwoodOnlineUpgradeSessionRequest *sessionRequest
) {
    char statusText[kSessionRequestStatusBufferSize];
    LRESULT sessionIndex;
    IWestwoodOnlineUpgradeProviderApiCallbacks *api;

    if (status < 0) {
        api = GetCallbackApiComObject();
        api->CancelPendingSessionFlow();
        return 0;
    }

    if ((sessionNode->m_rowFlags & kSessionRequestRefreshCacheFlag) != 0) {
        zLoc::FormatMessage(
            statusText,
            kSessionRequestStatusBufferSize,
            kSessionLaunchResultResetMessageId,
            browseRecord->m_sessionName,
            sessionRequest->m_sessionName
        );
        g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusText);
        api = GetCallbackApiComObject();
        g_WestwoodOnlineUpgradePendingSessionResultCount = 0;
        g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] = '\0';
        api->RequestListMode(
            g_WestwoodOnlineUpgradeActiveListMode,
            1
        );
        ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->SendDlgItemMessageA(
                kWestwoodOnlineUpgradeApiEventSink_SessionResultsListId,
                LB_RESETCONTENT,
                0,
                0
            );
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 0;
        g_pWestwoodOnlineUpgradeDialog->EnableQueryControls(0);
        g_pWestwoodOnlineUpgradeDialog->EnableConnectButton(0);
        return 0;
    }

    zLoc::FormatMessage(
        statusText,
        kSessionRequestStatusBufferSize,
        kSessionLaunchResultRemoveMessageId,
        sessionNode->m_sessionName,
        browseRecord->m_sessionName,
        sessionRequest->m_sessionName
    );
    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusText);
    sessionIndex = ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
                       ->SendDlgItemMessageA(
                           kWestwoodOnlineUpgradeApiEventSink_SessionResultsListId,
                           LB_FINDSTRINGEXACT,
                           (WPARAM)-1,
                           (LPARAM)sessionNode->m_sessionName
                       );
    if (sessionIndex != LB_ERR) {
        ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->SendDlgItemMessageA(
                kWestwoodOnlineUpgradeApiEventSink_SessionResultsListId,
                LB_DELETESTRING,
                (WPARAM)sessionIndex,
                0
            );
        --g_WestwoodOnlineUpgradePendingSessionResultCount;
    }

    return 0;
}

/**
 * Reimplements 0x441600: WestwoodOnlineUpgradeRefCountAndLock::Init.
 * Purpose: Resets the embedded reference count and initializes its critical section.
 */
WestwoodOnlineUpgradeRefCountAndLock * WestwoodOnlineUpgradeRefCountAndLock::Init() {
    refCount = 0;
    InitializeCriticalSection(&lock);
    return this;
}

/**
 * Reimplements 0x441620: WestwoodOnlineUpgradeApiEventSink::Release.
 * Purpose: Decrements the COM reference count and destroys the API sink on final release.
 */
ULONG __stdcall WestwoodOnlineUpgradeApiEventSink::Release(
    WestwoodOnlineUpgradeApiEventSink *self
) {
    ULONG refCount;

    refCount = (ULONG)InterlockedDecrement(&self->m_refCountAndLock.refCount);
    if (refCount == 0 && self != 0) {
        self->Destructor();
        delete self;
    }

    return refCount;
}

/**
 * Original helper evidence: no standalone retail function; observed as the
 * COM Release vtable member body.
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeApiEventSink.cpp
 * Purpose: Delegates the vtable Release call to the recovered static release routine.
 */
ULONG STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::Release() {
    return WestwoodOnlineUpgradeApiEventSink::Release(this);
}

/**
 * Reimplements 0x441660: WestwoodOnlineUpgradeApiEventSink::QueryInterface.
 * Purpose: Resolves the API event sink interfaces through its recovered interface map.
 */
HRESULT STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::QueryInterface(
    REFIID iid,
    void **outInterface
) {
    return WestwoodOnlineUpgradeApiEventSink::QueryInterface(
        this,
        iid,
        outInterface
    );
}

/**
 * Reimplements 0x441680: WestwoodOnlineUpgradeApiEventSink::Destructor.
 * Purpose: Tears down the embedded lock and decrements the live Westwood event-sink count.
 */
void WestwoodOnlineUpgradeApiEventSink::Destructor() {
    m_refCountAndLock.refCount = 1;
    InterlockedDecrement(&g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount);
    DeleteCriticalSection(&m_refCountAndLock.lock);
}

/**
 * Reimplements 0x4416f0: WestwoodOnlineUpgradeDialog::GetSelectedProfilePlayerName
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: return the selected profile player name for callback use.
 */
CString WestwoodOnlineUpgradeDialog::GetSelectedProfilePlayerName() {
    return m_selectedProfilePlayerName;
}

/**
 * Reimplements 0x441720: WestwoodOnlineUpgradeDialog::GetSelectedProfileConnectString
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: return the selected profile connection string for callback use.
 */
CString WestwoodOnlineUpgradeDialog::GetSelectedProfileConnectString() {
    return m_selectedProfileConnectString;
}

/**
 * Reimplements 0x441750: WestwoodOnlineUpgradeConfigDialog::WestwoodOnlineUpgradeConfigDialog
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\WestwoodOnlineUpgradeConfigDialog.cpp.
 * Purpose: constructs the MFC dialog, child controls, CString profile arrays,
 * installs the derived dialog vftable, and seeds the WOL password flag.
 */
WestwoodOnlineUpgradeConfigDialog::WestwoodOnlineUpgradeConfigDialog(
    CWnd *parentWnd
) :
    CDialog(
        kWestwoodOnlineUpgradeConfigDialogResourceId,
        parentWnd
    ),
    m_profileCombo(),
    m_connectStringEdit(),
    m_reservedString(),
    m_connectStringEditText()
{
    m_connectStringEditText = kEmptyString;
    m_wolPasswordFlag = zOpt_GetWolPasswordFlagValue();
}

/**
 * Reimplements 0x4418b0: WestwoodOnlineUpgradeConfigDialog::~WestwoodOnlineUpgradeConfigDialog
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp.
 * Purpose: tears down the profile CString arrays and embedded MFC controls in
 * the reverse order established by the constructor.
 */
WestwoodOnlineUpgradeConfigDialog::~WestwoodOnlineUpgradeConfigDialog() {
}

/**
 * Reimplements 0x4419a0: WestwoodOnlineUpgradeConfigDialog::DoDataExchange
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp.
 * Purpose: binds the profile combo, connect-string edit box, edit text, and
 * remember-password flag to the dialog controls.
 */
void WestwoodOnlineUpgradeConfigDialog::DoDataExchange(
    CDataExchange *dataExchange
) {
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeConfigProfileComboId,
        *((CWnd *)&m_profileCombo)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeConfigConnectStringEditId,
        *((CWnd *)&m_connectStringEdit)
    );
    DDX_Text(
        dataExchange,
        kWestwoodOnlineUpgradeConfigConnectStringEditId,
        m_connectStringEditText
    );
    DDX_Check(
        dataExchange,
        kWestwoodOnlineUpgradeConfigRememberPasswordCheckId,
        m_wolPasswordFlag
    );
}

/**
 * Reimplements 0x441a10: WestwoodOnlineUpgradeConfigDialog::GetMessageMap
 * Provisional source-placement hypothesis: WestwoodOnlineUpgradeConfigDialog.cpp.
 * Purpose: returns the recovered MFC command notification map for this dialog.
 */
const AFX_MSGMAP * WestwoodOnlineUpgradeConfigDialog::GetMessageMap() const {
    return &WestwoodOnlineUpgradeConfigDialog::messageMap;
}

/**
 * Reimplements 0x441a20: WestwoodOnlineUpgradeConfigDialog::OnConnectStringEditSetFocusClear
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp.
 * Purpose: clears the selection in the connect-string edit control on focus.
 */
void WestwoodOnlineUpgradeConfigDialog::OnConnectStringEditSetFocusClear() {
    ::SendMessageA(
        m_connectStringEdit.m_hWnd,
        EM_SETSEL,
        0,
        0
    );
}

/**
 * Reimplements 0x441a40: WestwoodOnlineUpgradeConfigDialog::OnInitDialog
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp.
 * Purpose: loads both WOL profiles, fills the combo box, seeds connect-string
 * modes, and initializes the selected profile edit state.
 */
BOOL WestwoodOnlineUpgradeConfigDialog::OnInitDialog() {
    ((CDialog *)this)->CDialog::OnInitDialog();

    IWestwoodOnlineUpgradeProviderApi *const api =
        (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    char *playerName = 0;
    char *connectString = 0;

    if (api->LoadConnectProfileStrings(1, &playerName, &connectString) != 0) {
        playerName = (char *)kEmptyString;
        connectString = (char *)kEmptyString;
    }

    m_savedPlayerNames[0] = playerName;
    m_savedConnectStrings[0] = connectString;
    m_profilePlayerNames[0] = playerName;
    m_profileConnectStrings[0] = connectString;

    const char *displayName = playerName;
    if (displayName[0] == '\0') {
        displayName = zLoc::GetMessageString(kWestwoodOnlineUpgradeConfigUnnamedProfileMessageId);
    }
    LRESULT itemIndex =
        ::SendMessageA(
            m_profileCombo.m_hWnd,
            CB_INSERTSTRING,
            0,
            (LPARAM)displayName
        );
    ::SendMessageA(
        m_profileCombo.m_hWnd,
        CB_SETITEMDATA,
        itemIndex,
        0
    );

    if (api->LoadConnectProfileStrings(2, &playerName, &connectString) != 0) {
        playerName = (char *)kEmptyString;
        connectString = (char *)kEmptyString;
    }

    m_savedPlayerNames[1] = playerName;
    m_savedConnectStrings[1] = connectString;
    m_profilePlayerNames[1] = playerName;
    m_profileConnectStrings[1] = connectString;

    displayName = playerName;
    if (displayName[0] == '\0') {
        displayName = zLoc::GetMessageString(kWestwoodOnlineUpgradeConfigUnnamedProfileMessageId);
    }
    itemIndex = ::SendMessageA(
        m_profileCombo.m_hWnd,
        CB_INSERTSTRING,
        1,
        (LPARAM)displayName
    );
    ::SendMessageA(
        m_profileCombo.m_hWnd,
        CB_SETITEMDATA,
        itemIndex,
        1
    );

    m_profileConnectStringModes[0] = ((const char *)m_savedConnectStrings[0])[0] == '\0' ? 0 : 1;
    m_profileConnectStringModes[1] = ((const char *)m_savedConnectStrings[1])[0] == '\0' ? 0 : 1;
    m_selectedProfileIndex = 0;
    m_profileComboEditDirty = 0;
    ::SendMessageA(
        m_profileCombo.m_hWnd,
        CB_SETCURSEL,
        0,
        0
    );
    ((CWnd *)&m_connectStringEdit)->SetWindowTextA((const char *)m_profileConnectStrings[0]);
    return TRUE;
}

/**
 * Reimplements 0x441c60: WestwoodOnlineUpgradeConfigDialog::GetSelectedProfileValues
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\WestwoodOnlineUpgradeConfigDialog.cpp.
 * Purpose: exposes the selected profile name, connect string, and mode to the
 * parent upgrade dialog after the modal config dialog succeeds.
 */
void WestwoodOnlineUpgradeConfigDialog::GetSelectedProfileValues(
    char **playerNameOut,
    char **connectStringOut,
    int *connectStringModeOut
) {
    const int selectedIndex = m_selectedProfileIndex;
    *playerNameOut =
        m_profilePlayerNames[selectedIndex].GetBuffer(kSelectedProfileTextBufferLength);
    *connectStringOut =
        m_profileConnectStrings[selectedIndex].GetBuffer(kSelectedProfileTextBufferLength);
    *connectStringModeOut = m_profileConnectStringModes[selectedIndex];
}

/**
 * Reimplements 0x441cb0: WestwoodOnlineUpgradeConfigDialog::ShowModalAndApplySelectedProfileValues
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\WestwoodOnlineUpgradeConfigDialog.cpp.
 * Purpose: runs the stack-based config dialog and copies selected profile
 * values into the owning Westwood online upgrade dialog on OK.
 */
int WestwoodOnlineUpgradeConfigDialog::ShowModalAndApplySelectedProfileValues() {
    unsigned int dialogStorage
        [(sizeof(WestwoodOnlineUpgradeConfigDialog) + kStackStorageUnitSize - 1) /
            kStackStorageUnitSize];
    WestwoodOnlineUpgradeConfigDialog *const dialog =
        (WestwoodOnlineUpgradeConfigDialog *)dialogStorage;

    dialog->Constructor(0);
    if (((CDialog *)dialog)->CDialog::DoModal() != kDialogOkResult) {
        DestructConfigDialog(dialog);
        return 0;
    }

    char *playerName = 0;
    char *connectString = 0;
    int connectStringMode = 0;
    dialog->GetSelectedProfileValues(
        &playerName,
        &connectString,
        &connectStringMode
    );

    CString playerNameString(playerName);
    g_pWestwoodOnlineUpgradeDialog->SetSelectedProfilePlayerName(playerNameString);

    CString connectStringString(connectString);
    g_pWestwoodOnlineUpgradeDialog->SetSelectedProfileConnectString(connectStringString);
    g_pWestwoodOnlineUpgradeDialog->m_selectedProfileConnectStringMode = connectStringMode;

    DestructConfigDialog(dialog);
    return 1;
}

/**
 * Reimplements 0x441f40: WestwoodOnlineUpgradeConfigDialog::OnOK
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp.
 * Purpose: saves the selected WOL profile strings and password flag before
 * accepting the dialog through MFC.
 */
void WestwoodOnlineUpgradeConfigDialog::OnOK() {
    IWestwoodOnlineUpgradeProviderApi *const api =
        (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;

    if (m_wolPasswordFlag == 0) {
        api->SaveConnectProfileStrings(
            1,
            (const char *)m_profilePlayerNames[0],
            kEmptyString,
            0
        );
        api->SaveConnectProfileStrings(
            2,
            (const char *)m_profilePlayerNames[1],
            kEmptyString,
            0
        );
    } else {
        api->SaveConnectProfileStrings(
            1,
            (const char *)m_profilePlayerNames[0],
            (const char *)m_profileConnectStrings[0],
            m_profileConnectStringModes[0] == 0
        );
        api->SaveConnectProfileStrings(
            2,
            (const char *)m_profilePlayerNames[1],
            (const char *)m_profileConnectStrings[1],
            m_profileConnectStringModes[1] == 0
        );
    }

    zOpt::SetWolPasswordFlag(m_wolPasswordFlag);
    ((CDialogProviderAccessor *)this)->CallOnOK();
}

/**
 * Reimplements 0x442010: WestwoodOnlineUpgradeConfigDialog::OnProfileComboKillFocus
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp.
 * Purpose: commits edited profile text back into the combo box item.
 */
void WestwoodOnlineUpgradeConfigDialog::OnProfileComboKillFocus() {
    if (m_profileComboEditDirty == 0) {
        return;
    }

    ::SendMessageA(
        m_profileCombo.m_hWnd,
        CB_DELETESTRING,
        m_selectedProfileIndex,
        0
    );
    ((CWnd *)&m_profileCombo)->GetWindowTextA(m_profilePlayerNames[m_selectedProfileIndex]);
    ::SendMessageA(
        m_profileCombo.m_hWnd,
        CB_INSERTSTRING,
        m_selectedProfileIndex,
        (LPARAM)(const char *)m_profilePlayerNames[m_selectedProfileIndex]
    );
    m_profileComboEditDirty = 0;
}

/**
 * Reimplements 0x442080: WestwoodOnlineUpgradeConfigDialog::OnProfileComboSelChange
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp.
 * Purpose: tracks the selected profile and displays its connect string.
 */
void WestwoodOnlineUpgradeConfigDialog::OnProfileComboSelChange() {
    m_selectedProfileIndex = (int) ::SendMessageA(
        m_profileCombo.m_hWnd,
        CB_GETCURSEL,
        0,
        0
    );
    ((CWnd *)&m_connectStringEdit)
        ->SetWindowTextA((const char *)m_profileConnectStrings[m_selectedProfileIndex]);
}

/**
 * Reimplements 0x4420c0: WestwoodOnlineUpgradeConfigDialog::OnProfileComboEditChange
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp.
 * Purpose: records that the editable profile combo text has changed.
 */
void WestwoodOnlineUpgradeConfigDialog::OnProfileComboEditChange() {
    m_profileComboEditDirty = 1;
}

/**
 * Reimplements 0x4420d0: WestwoodOnlineUpgradeConfigDialog::OnProfileComboDropdown
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp.
 * Purpose: commits pending profile-combo edits before showing the drop-down.
 */
void WestwoodOnlineUpgradeConfigDialog::OnProfileComboDropdown() {
    OnProfileComboKillFocus();
}

/**
 * Reimplements 0x4420e0: WestwoodOnlineUpgradeConfigDialog::OnConnectStringModeClicked
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp.
 * Purpose: toggles whether saved WOL passwords/connect strings are retained.
 */
void WestwoodOnlineUpgradeConfigDialog::OnConnectStringModeClicked() {
    m_wolPasswordFlag = m_wolPasswordFlag == 0;
}

/**
 * Reimplements 0x442100: WestwoodOnlineUpgradeConfigDialog::OnConnectStringEditKillFocus
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp.
 * Purpose: stores edited connect-string text and marks the profile as custom
 * when it differs from the saved value.
 */
void WestwoodOnlineUpgradeConfigDialog::OnConnectStringEditKillFocus() {
    const int selectedIndex = m_selectedProfileIndex;
    ((CWnd *)&m_connectStringEdit)->GetWindowTextA(m_profileConnectStrings[selectedIndex]);

    if (strcmp(
            (const char *)m_profileConnectStrings[selectedIndex],
            (const char *)m_savedConnectStrings[selectedIndex]
        ) != 0) {
        m_profileConnectStringModes[selectedIndex] = 0;
    }
}

/**
 * Reimplements 0x442180: WestwoodOnlineUpgradeDialog::SetSelectedProfilePlayerName
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: store the selected Westwood Online profile player name.
 */
void WestwoodOnlineUpgradeDialog::SetSelectedProfilePlayerName(
    CString playerName
) {
    m_selectedProfilePlayerName = playerName;
}

/**
 * Reimplements 0x4421d0: WestwoodOnlineUpgradeDialog::SetSelectedProfileConnectString
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: store the selected Westwood Online profile connection string.
 */
void WestwoodOnlineUpgradeDialog::SetSelectedProfileConnectString(
    CString connectString
) {
    m_selectedProfileConnectString = connectString;
}

WestwoodOnlineUpgradeProgressDialog::WestwoodOnlineUpgradeProgressDialog(
    CWnd *parentWnd
) :
    CDialog(
        kWestwoodOnlineUpgradeProgressDialog_ResourceId,
        parentWnd
    )
{
}

/**
 * Reimplements 0x442220: WestwoodOnlineUpgradeProgressDialog::Constructor (D:\Proj\Battlesport\WestwoodOnlineUpgradeProgressDialog.cpp).
 * Purpose: placement-constructs the standalone WOL download progress dialog.
 */
WestwoodOnlineUpgradeProgressDialog * WestwoodOnlineUpgradeProgressDialog::Constructor(
    CWnd *parentWnd
) {
    new (this) WestwoodOnlineUpgradeProgressDialog(parentWnd);
    return this;
}

/**
 * Reimplements 0x442260: WestwoodOnlineUpgradeProgressDialog::GetMessageMap (D:\Proj\GameZRecoil\westwoodonline\WolapiProgressDialog.cpp).
 * Purpose: returns the sentinel-only MFC message-map record for the raw dialog proc.
 */
const AFX_MSGMAP * WestwoodOnlineUpgradeProgressDialog::GetMessageMap() const {
    return &WestwoodOnlineUpgradeProgressDialog::messageMap;
}

/**
 * Reimplements 0x442270: WestwoodOnlineUpgradeProgressDialog::SetStatusTextFmt (D:\Proj\GameZRecoil\westwoodonline\WolapiProgressDialog.cpp).
 * Purpose: formats text into the recovered 0x40-byte global buffer and writes the progress status control.
 */
BOOL WestwoodOnlineUpgradeProgressDialog::SetStatusTextFmt(
    const char *format,
    ...
) {
    va_list args;
    va_start(
        args,
        format
    );
    vsprintf(
        g_WestwoodOnlineUpgradeProgressStatusTextBuffer,
        format,
        args
    );
    va_end(args);

    return ::SetDlgItemTextA(
        g_hWestwoodOnlineUpgradeProgressDialog,
        kProgressStatusTextControlId,
        g_WestwoodOnlineUpgradeProgressStatusTextBuffer
    );
}

/**
 * Reimplements 0x4422a0: WestwoodOnlineUpgradeDownload::CreateInstanceAndAdvise.
 * Purpose: Creates the Westwood download COM object and advises the local event sink.
 */
HRESULT WestwoodOnlineUpgradeDownload::CreateInstanceAndAdvise() {
    CoCreateInstance(
        g_CLSID_WestwoodOnlineUpgradeDownload,
        0,
        CLSCTX_INPROC_SERVER,
        g_IID_WestwoodOnlineUpgradeDownload,
        (void **)&g_pWestwoodOnlineUpgradeDownload
    );
    WestwoodOnlineUpgradeDownloadEventSink::CreateInstance(
        &g_pWestwoodOnlineUpgradeDownloadEventSink
    );
    return zCom::ConnectionPointContainer_Advise(
        g_pWestwoodOnlineUpgradeDownload,
        g_pWestwoodOnlineUpgradeDownloadEventSink,
        IID_WestwoodOnlineUpgradeDownloadEventSink,
        &g_WestwoodOnlineUpgradeDownloadAdviseCookie
    );
}

/**
 * Reimplements 0x4422f0: WestwoodOnlineUpgradeDownload::UnadviseAndRelease.
 * Purpose: Unadvises the download event sink and releases the Westwood download COM object.
 */
ULONG WestwoodOnlineUpgradeDownload::UnadviseAndRelease() {
    zCom::ConnectionPointContainer_Unadvise(
        g_pWestwoodOnlineUpgradeDownload,
        IID_WestwoodOnlineUpgradeDownloadEventSink,
        g_WestwoodOnlineUpgradeDownloadAdviseCookie
    );
    return g_pWestwoodOnlineUpgradeDownload->Release();
}

/**
 * Reimplements 0x442320: WestwoodOnlineUpgradeProgressDialog::DlgProc (D:\Proj\Battlesport\WestwoodOnlineUpgradeProgressDialog.cpp).
 * Purpose: starts and pumps the WOL download progress dialog, handles cancel, and cleans up on destroy.
 */
BOOL CALLBACK WestwoodOnlineUpgradeProgressDialog::DlgProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM
) {
    char sourcePath[kDownloadPathBufferSize];
    WestwoodOnlineUpgradeDownloadReadyEntry *entry;
    IWestwoodOnlineUpgradeDownload *download;

    if (uMsg == WM_SETFONT) {
        return TRUE;
    }

    if (uMsg == WM_DESTROY) {
        ::KillTimer(
            hWnd,
            kProgressTimerId
        );
        WestwoodOnlineUpgradeDownload::UnadviseAndRelease();
        SetCurrentDirectoryA(g_WestwoodOnlineUpgradeDownloadRestoreCwd);
        ::EndDialog(
            hWnd,
            g_WestwoodOnlineUpgradeDownloadDialogResult
        );
        return TRUE;
    }

    if (uMsg == WM_INITDIALOG) {
        WestwoodOnlineUpgradeDownload::CreateInstanceAndAdvise();
        ::SetDlgItemTextA(
            hWnd,
            kProgressStatusControlId,
            g_WestwoodOnlineUpgradeDownloadReadyPromptText
        );
        GetCurrentDirectoryA(
            kDownloadPathBufferSize,
            g_WestwoodOnlineUpgradeDownloadRestoreCwd
        );

        entry = g_pWestwoodOnlineUpgradeDownloadReadyList;
        sprintf(
            sourcePath,
            kDownloadSourcePathFormat,
            entry->m_sourcePathBase,
            entry->m_fileName
        );
        if (SetCurrentDirectoryA(g_pWestwoodOnlineUpgradeDownloadReadyList->m_downloadDirectory) ==
            0) {
            CreateDirectoryA(
                g_pWestwoodOnlineUpgradeDownloadReadyList->m_downloadDirectory,
                0
            );
            SetCurrentDirectoryA(g_pWestwoodOnlineUpgradeDownloadReadyList->m_downloadDirectory);
        }

        ::SetDlgItemTextA(
            hWnd,
            kProgressStatusControlId,
            g_WestwoodOnlineUpgradeDownloadReadyPromptText
        );

        entry = g_pWestwoodOnlineUpgradeDownloadReadyList;
        download = (IWestwoodOnlineUpgradeDownload *)g_pWestwoodOnlineUpgradeDownload;
        download->BeginDownload(
            entry->m_descriptor0,
            entry->m_descriptor1,
            entry->m_descriptor2,
            sourcePath,
            entry->m_fileName,
            kWestwoodOnlineUpgradeRegistryKey
        );
        g_hWestwoodOnlineUpgradeProgressDialog = hWnd;
        g_WestwoodOnlineUpgradeDownloadDialogResult = 0;
        ::SetTimer(
            hWnd,
            kProgressTimerId,
            kProgressTimerMs,
            0
        );
        return TRUE;
    }

    if (uMsg == WM_COMMAND) {
        if (LOWORD(wParam) == IDCANCEL) {
            download = (IWestwoodOnlineUpgradeDownload *)g_pWestwoodOnlineUpgradeDownload;
            download->Abort();
            ::DestroyWindow(g_hWestwoodOnlineUpgradeProgressDialog);
            return TRUE;
        }
        return FALSE;
    }

    if (uMsg == WM_TIMER) {
        if (g_WestwoodOnlineUpgradeDownloadDialogResult != 0) {
            ::DestroyWindow(hWnd);
            return TRUE;
        }

        download = (IWestwoodOnlineUpgradeDownload *)g_pWestwoodOnlineUpgradeDownload;
        download->Pump();
        return TRUE;
    }

    return FALSE;
}

/**
 * Reimplements 0x442530: WestwoodOnlineUpgradeDialog::ShowDownloadReadyList (D:\Proj\Battlesport\WestwoodOnlineUpgradeProgressDialog.cpp).
 * Purpose: formats per-entry prompts and launches the standalone WOL download progress dialog for each ready-list entry.
 */
int __fastcall WestwoodOnlineUpgradeDialog::ShowDownloadReadyList(
    WestwoodOnlineUpgradeDownloadReadyEntry *readyListHead
) {
    WestwoodOnlineUpgradeDownloadReadyEntry *currentEntry = readyListHead;
    WestwoodOnlineUpgradeDownloadReadyEntry *countCursor = currentEntry;
    int totalEntryCount = 0;
    int entryOrdinal = 0;

    if (currentEntry != 0) {
        do {
            countCursor = countCursor->m_next;
            ++totalEntryCount;
        } while (countCursor != 0);
    }

    if (currentEntry != 0) {
        while (currentEntry != 0) {
            ++entryOrdinal;
            g_pWestwoodOnlineUpgradeDownloadReadyList = currentEntry;
            zLoc::FormatMessage(
                g_WestwoodOnlineUpgradeDownloadReadyPromptText,
                kDownloadPromptBufferSize,
                kDownloadPromptMessageId,
                entryOrdinal,
                totalEntryCount
            );
            if (DialogBoxParamA(
                    g_RecoilApp_hInstance,
                    (LPCSTR)kDownloadDialogResourceId,
                    g_RecoilApp_hWndMain,
                    WestwoodOnlineUpgradeProgressDialog::DlgProc,
                    0
                ) == -1) {
                return 0;
            }

            currentEntry = currentEntry->m_next;
        }
    }

    return 1;
}

/**
 * Reimplements 0x4425c0: WestwoodOnlineUpgradeDownloadEventSink::CreateInstance.
 * Purpose: Allocates and initializes a download event sink for connection-point advising.
 */
HRESULT __stdcall WestwoodOnlineUpgradeDownloadEventSink::CreateInstance(
    WestwoodOnlineUpgradeDownloadEventSink **outSink
) {
    HRESULT result = E_OUTOFMEMORY;
    WestwoodOnlineUpgradeDownloadEventSink *eventSink =
        new WestwoodOnlineUpgradeDownloadEventSink;

    if (eventSink != 0) {
        eventSink->m_refCountAndLock.Init();
        InterlockedIncrement(&g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount);
        result = S_OK;
    }

    *outSink = eventSink;
    return result;
}

/**
 * Reimplements 0x442660: WestwoodOnlineUpgradeDownloadEventSink::OnDownloadFinished.
 * Purpose: Marks the upgrade download dialog as finished and reports success to COM.
 */
HRESULT STDMETHODCALLTYPE WestwoodOnlineUpgradeDownloadEventSink::OnDownloadFinished() {
    WestwoodOnlineUpgradeProgressDialog::SetStatusTextFmt(kDownloadFinishedStatusText);
    g_WestwoodOnlineUpgradeDownloadDialogResult = 1;
    return S_OK;
}

/**
 * Reimplements 0x442680: WestwoodOnlineUpgradeDownloadEventSink::OnDownloadError.
 * Purpose: Shows the download error state, pauses briefly, and records dialog failure.
 */
HRESULT STDMETHODCALLTYPE WestwoodOnlineUpgradeDownloadEventSink::OnDownloadError(
    HRESULT
) {
    WestwoodOnlineUpgradeProgressDialog::SetStatusTextFmt(kDownloadErrorStatusText);
    Sleep(kDownloadErrorStatusSleepMs);
    g_WestwoodOnlineUpgradeDownloadDialogResult = -1;
    return S_OK;
}

/**
 * Reimplements 0x4426b0: WestwoodOnlineUpgradeDownloadEventSink::OnDownloadProgress.
 * Purpose: Updates the progress control and byte-count status for an active patch download.
 */
HRESULT STDMETHODCALLTYPE WestwoodOnlineUpgradeDownloadEventSink::OnDownloadProgress(
    unsigned int bytesRead,
    unsigned int totalBytes,
    int,
    int secondsLeft
) {
    SendDlgItemMessageA(
        g_hWestwoodOnlineUpgradeProgressDialog,
        kDownloadProgressControlId,
        PBM_SETPOS,
        (bytesRead * kDownloadProgressPercentScale) / totalBytes,
        0
    );
    if (secondsLeft > 0) {
        WestwoodOnlineUpgradeProgressDialog::SetStatusTextFmt(
            kDownloadProgressWithTimeStatusText,
            bytesRead,
            totalBytes,
            secondsLeft
        );
    } else {
        WestwoodOnlineUpgradeProgressDialog::SetStatusTextFmt(
            kDownloadProgressStatusText,
            bytesRead,
            totalBytes
        );
    }
    return S_OK;
}

/**
 * Reimplements 0x442720: WestwoodOnlineUpgradeDownloadEventSink::OnStateChanged.
 * Purpose: Maps selected Westwood download state codes to progress-dialog status text.
 */
HRESULT STDMETHODCALLTYPE WestwoodOnlineUpgradeDownloadEventSink::OnStateChanged(
    WestwoodOnlineUpgradeDownloadState stateCode
) {
    if (stateCode == WOL_DOWNLOAD_STATE_CONNECTING) {
        WestwoodOnlineUpgradeProgressDialog::SetStatusTextFmt(kDownloadStateConnectingText);
    } else if (stateCode == WOL_DOWNLOAD_STATE_FINDING_PATCH) {
        WestwoodOnlineUpgradeProgressDialog::SetStatusTextFmt(kDownloadStateFindingPatchText);
    } else if (stateCode == WOL_DOWNLOAD_STATE_DOWNLOADING_PATCH) {
        WestwoodOnlineUpgradeProgressDialog::SetStatusTextFmt(kDownloadStateDownloadingPatchText);
    }
    return S_OK;
}

/**
 * Reimplements 0x442770: shared COM AddRef address group.
 * The download sink is the selected representative and the API sink is its
 * proven identical-code-fold alias.
 */
ULONG STDMETHODCALLTYPE WestwoodOnlineUpgradeDownloadEventSink::AddRef() {
    return (ULONG)InterlockedIncrement(&m_refCountAndLock.refCount);
}

ULONG STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::AddRef() {
    return (ULONG)InterlockedIncrement(&m_refCountAndLock.refCount);
}

/**
 * Reimplements 0x442790: WestwoodOnlineUpgradeDownloadEventSink::Release.
 * Purpose: Decrements the COM reference count and destroys the sink on the final release.
 */
ULONG STDMETHODCALLTYPE WestwoodOnlineUpgradeDownloadEventSink::Release() {
    ULONG refCount;

    refCount = (ULONG)InterlockedDecrement(&m_refCountAndLock.refCount);
    if (refCount == 0) {
        delete this;
    }

    return refCount;
}

/**
 * Reimplements 0x4427d0: WestwoodOnlineUpgradeDownloadEventSink::QueryInterface.
 * Purpose: Resolves the download event sink interfaces through its recovered interface map.
 */
HRESULT STDMETHODCALLTYPE WestwoodOnlineUpgradeDownloadEventSink::QueryInterface(
    REFIID iid,
    void **outInterface
) {
    return zCom::QueryInterfaceFromInterfaceMap(
        this,
        g_WestwoodOnlineUpgradeDownloadEventSink_InterfaceMap,
        &iid,
        outInterface
    );
}

/**
 * Reimplements 0x4427f0: WestwoodOnlineUpgradeDownloadEventSink::~WestwoodOnlineUpgradeDownloadEventSink.
 * Purpose: Tears down the embedded lock and decrements the live Westwood event-sink count.
 */
WestwoodOnlineUpgradeDownloadEventSink::~WestwoodOnlineUpgradeDownloadEventSink() {
    m_refCountAndLock.refCount = 1;
    InterlockedDecrement(&g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount);
    DeleteCriticalSection(&m_refCountAndLock.lock);
}
